/**CFile***********************************************************************

  FileName    [travIgr.c]

  PackageName [trav]

  Synopsis    [Interpolation based image routines]

  Description []

  SeeAlso   []

  Author    [Gianpiero Cabodi]

  Copyright [This file was created at the Politecnico di Torino,
    Torino, Italy.
    The  Politecnico di Torino makes no warranty about the suitability of
    this software for any purpose.
    It is presented on an AS IS basis.
  ]

  Revision  []

******************************************************************************/

#include "travInt.h"
#include "ddiInt.h"
#include "baigInt.h"
#include "fbv.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*--------------------------------------------------------------------------s-*/



/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#define tMgrO(travMgr) ((travMgr)->settings.stdout)
#define dMgrO(ddiMgr) ((ddiMgr)->settings.stdout)

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
setupCexWithGates(
  Ddi_Bdd_t *cex,
  Ddi_Bddarray_t *gates,
  float ratio
);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
setupCexWithGates(
  Ddi_Bdd_t *cex,
  Ddi_Bddarray_t *gates,
  float ratio
)
{
  int reversed = ratio<0;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(cex);
  ratio = fabs(ratio);
  if (gates == NULL || Ddi_BddarrayNum(gates) == 0)
    return;
  ratio = ratio*ratio;
  Ddi_Bddarray_t *gateConstr=Ddi_BddarrayDup(gates);
  Ddi_Bdd_t *newCex = Ddi_BddMakeConstAig(ddm,1); 
  Ddi_AigarrayConstrainCubeAcc(gateConstr,cex);  
  int bound = ratio*Ddi_BddarrayNum(gateConstr);
  if (ratio < 0.01) bound = Ddi_BddarrayNum(gateConstr)>1?2:1;
  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
    printf("\ncex to gates is using %d/%d gates\n", bound,
           Ddi_BddarrayNum(gateConstr));
  }
  for (int ii=0; ii<bound; ii++) {
    int i = reversed ? Ddi_BddarrayNum(gateConstr)-1-ii : ii;
    Ddi_Bdd_t *c_i = Ddi_BddarrayRead(gateConstr,i);
    if (Ddi_BddIsConstant(c_i)) {
      Ddi_Bdd_t *c =Ddi_BddarrayRead(gates,i);
      if (Ddi_BddIsZero(c_i))
        Ddi_BddDiffAcc(newCex,c);
      else
        Ddi_BddAndAcc(newCex,c);
    }
  }
  Ddi_DataCopy(cex,newCex);
  Ddi_Free(newCex);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
itpBwdRingPreimg(
  Trav_ItpMgr_t * itpMgr,
  Ddi_Bdd_t *bRing,
  Ddi_Bdd_t *prevRing,
  int ring_i,
  int bound
)
{
  Ddi_Mgr_t *ddm = itpMgr->ddiMgr;
  Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity(ddm);

  Trav_Mgr_t *travMgr = itpMgr->travMgr;
  int growCone = abs(Trav_MgrReadIgrGrowCone(travMgr));
  int myend, mystart;
  int useRingConstr = 0; // DISABLED!
  Ddi_Bdd_t *myCone = NULL;
  int boundK=Trav_MgrReadIgrGrowCone(travMgr)>2, sat, chk=0;
  Ddi_Bdd_t *splitB=NULL, *nextRingConstr=NULL;
  int i = ring_i+1;

  static int nc = 0;
  nc++;

  myend = ring_i+1;
  mystart = ring_i+bound-1;

  myCone = Ddi_BddDup(itpMgr->target);
  Ddi_BddSubstVarsAcc(myCone, itpMgr->ps, itpMgr->ns);
  Ddi_BddWriteMark(myCone, 0);

  Pdtutil_VerbosityLocalIf(verbosity, Pdtutil_VerbLevelUsrMax_c) {
    printf("\nPREIMG bwd ring %d\n", bound);
  }

  TravGrowConeBwd(itpMgr, myCone, mystart, myend, NULL,
	      NULL, 
	      useRingConstr, -1, boundK /*boundK */ );

  Ddi_Bdd_t *itp, *fromAndTr, *fromAndTrConstr;
  Ddi_Bdd_t *fwdFrom = Ddi_BddNot(bRing);

  Ddi_Bddarray_t *myDelta = Ddi_BddarraySubstVars(itpMgr->delta,
                                                  itpMgr->ps,
                                                  itpMgr->ns);

  if (itpMgr->invarConstr!=NULL && !Ddi_BddIsOne(itpMgr->invarConstr)) {
    Ddi_Bdd_t *myConstr = Ddi_BddDup(itpMgr->invarConstr);
    Ddi_BddSubstVarsAcc(myConstr, itpMgr->ps, itpMgr->ns);
    Ddi_BddAndAcc(fwdFrom,myConstr);
    Ddi_Free(myConstr);
  }

  if (useRingConstr > 0 && i > 0) {
    if (Ddi_BddarrayNum(itpMgr->eqRings) > i) {
      Ddi_Bdd_t *eqConstr = Ddi_BddarrayRead(itpMgr->eqRings, i);
      
      if (eqConstr != NULL) {
        Ddi_Vararray_t *vars = Ddi_BddReadEqVars(eqConstr);
        Ddi_Bddarray_t *subst = Ddi_BddReadEqSubst(eqConstr);
        
        Ddi_BddComposeAcc(fwdFrom, vars, subst);
        Ddi_BddarrayComposeAcc(myDelta, vars, subst);
      }
    }
  }

  Ddi_BddSubstVarsAcc(fwdFrom, itpMgr->ns, itpMgr->ps);
  Ddi_BddarraySubstVarsAcc(myDelta, itpMgr->ns, itpMgr->ps);
  Ddi_BddarraySubstVarsAcc(myDelta, itpMgr->pi, itpMgr->auxVarPis);

  fromAndTr = Ddi_BddRelMakeFromArray(myDelta, itpMgr->ns);
  if (1) {
    Ddi_Var_t *iv = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
    if (iv != NULL) {
      Ddi_Bdd_t *invarIn = Ddi_BddMakeLiteralAig(iv, 1);
      Ddi_BddAndAcc(fromAndTr, invarIn);
      Ddi_Free(invarIn);
    }
  }

  if (useRingConstr > 0 && i > 0) {
    if (Ddi_BddarrayNum(itpMgr->eqRings) > i + 1) {
      Ddi_Bdd_t *eqConstr = Ddi_BddarrayRead(itpMgr->eqRings, i + 1);
      
      if (eqConstr != NULL) {
        Ddi_Vararray_t *vars = Ddi_BddReadEqVars(eqConstr);
        Ddi_Bddarray_t *subst = Ddi_BddReadEqSubst(eqConstr);
        
        Ddi_BddComposeAcc(myCone, vars, subst);
        Ddi_BddComposeAcc(fromAndTr, vars, subst);
        nextRingConstr = Ddi_BddDup(eqConstr);
      }
    }
  }
  
  Ddi_BddPartInsertLast(fromAndTr, fwdFrom);
  Pdtutil_Assert(!chk || !Ddi_AigSatAnd(fromAndTr, myCone, NULL),
                 "unsat required");
  fromAndTrConstr = Ddi_BddDup(fromAndTr);
  if (prevRing!=NULL) {
    Ddi_BddPartInsertLast(fromAndTrConstr, prevRing);
  }
  
  int saveItpNnfAbstrAB = ddm->settings.aig.itpNnfAbstrAB;
  int saveItpReverse = ddm->settings.aig.itpReverse;
  // set just B
  ddm->settings.aig.itpNnfAbstrAB = 1;
  //  ddm->settings.aig.itpReverse = 1;
  itp = Ddi_AigSatAndWithInterpolant(fromAndTrConstr, myCone,
           itpMgr->nsvars, NULL, NULL, NULL, NULL, NULL,
                                     &sat, 0, 1, -1.0);
  ddm->settings.aig.itpNnfAbstrAB = saveItpNnfAbstrAB;
  ddm->settings.aig.itpReverse = saveItpReverse;

  if (nextRingConstr != NULL) {
    Ddi_BddSetAig(nextRingConstr);
    Ddi_BddAndAcc(itp, nextRingConstr);
    Ddi_Free(nextRingConstr);
  }
  
  Ddi_BddNotAcc(itp);
  if (prevRing != NULL) {
    int aigCnfLevel = ddm->settings.aig.aigCnfLevel;
    //    ddm->settings.aig.aigCnfLevel = 1;
    Ddi_BddSetAig(fromAndTr);
#if 0
    Ddi_BddNotAcc(prevRing);
    Ddi_BddOrAcc(fromAndTr,prevRing);
    Ddi_BddNotAcc(prevRing);
#endif
    Ddi_BddAndAcc(itp, prevRing);
    Ddi_AigOptByMonotoneCoreAcc(itp,fromAndTr,NULL,0,-1.0);
    ddm->settings.aig.aigCnfLevel = aigCnfLevel;
  }

  Ddi_Free(myCone);
  Ddi_Free(fwdFrom);
  Ddi_Free(myDelta);

  Ddi_Free(fromAndTr);
  Ddi_Free(fromAndTrConstr);

  return itp;
}


Ddi_Bdd_t *itpImgSplitConeCubeConstr(
  Trav_ItpTravMgr_t * itpTravMgr, 
  Ddi_Bdd_t * kConeRings,
  Ddi_Bdd_t *a,
  Ddi_Bdd_t *cone,
  Ddi_Bdd_t *itpPartial,
  Ddi_Bdd_t *outItp,
  int step,
  int iter,
  int *psat
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(a);
  Trav_ItpMgr_t *itpMgr = itpTravMgr->itpMgr;
  Trav_Mgr_t *travMgr = itpMgr->travMgr;
  int mark = Ddi_BddReadMark(cone);
  if (mark<6) return NULL;

  int split = (mark)*3/4;
  int split_i = step + split;
  Ddi_Bdd_t *coneAux = itpTravMgr->coneAuxSplit;

  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
    printf("\ngenerating constraining cone with split cone of bound %d\n",
           mark);
  }

  if (itpTravMgr->coneAuxSplit==NULL) {
    int fullK = step+mark;
    int start_i = fullK-1;
    int growCone = abs(Trav_MgrReadIgrGrowCone(travMgr));
    int boundK = (growCone > 2) ? 1 : 0;
    coneAux = Ddi_BddDup(Ddi_BddPartRead(kConeRings,fullK-1));
    TravGrowConeBwdDecomp(itpMgr, coneAux, start_i, step, split_i,  
			  1, 1, NULL,
			  itpMgr->initStub, 0/*useRingConstr*/, -1/*andWithRing_i*/, boundK);	
    itpTravMgr->coneAuxSplit = coneAux;
  }
  if (iter==0) {
    Ddi_Free(itpTravMgr->coneAuxSplitItp);
    itpTravMgr->coneAuxSplitItp = Ddi_BddMakeConstAig(ddm,0);
  }

  Ddi_Bddarray_t *splitU = NULL;
  Ddi_Vararray_t *splitV = NULL;
  Ddi_Bdd_t *splitConstr = NULL;
  Ddi_Varset_t *splitVars = NULL;
  Ddi_Bdd_t *myA = NULL, *myBl = NULL;
  Ddi_Vararray_t *v1=NULL,*v2=NULL, *glbA=NULL;
  Ddi_Bdd_t *coneConstr = NULL;	
  myBl = Ddi_BddDup(Ddi_BddReadComposeF(coneAux));
  splitU = Ddi_BddarrayDup(Ddi_BddReadComposeSubst(coneAux));
  splitV = Ddi_VararrayDup(Ddi_BddReadComposeVars(coneAux));
  splitConstr = Ddi_BddReadComposeConstr(coneAux);
  myA =  Ddi_BddRelMakeFromArray(splitU,splitV);
  splitVars = Ddi_VarsetMakeFromArray(splitV);
  Ddi_BddSetAig(myA);
  if (splitConstr!=NULL) {
    Ddi_BddAndAcc(myA, splitConstr);
  }
  Ddi_BddAndAcc(myA, itpPartial);
  //  Ddi_InfoCopy(myA,cone);
  //  Ddi_AigFilterLearningAigs(myA);
  
  Ddi_Bdd_t *itpSplit =
    //    Ddi_AigSat22AndWithInterpolant(NULL,myBl,myA,NULL,
    Ddi_AigSat22AndWithInterpolant(NULL,myBl,myA,NULL,
                                   splitVars, NULL,NULL,0,
                                   NULL,NULL,
                                   psat, 0, 1, 1, -1.0);
  if (itpSplit!=NULL) Ddi_BddNotAcc(itpSplit);
  Ddi_Bdd_t *cex=NULL;
  if (!(*psat) && !Ddi_BddIsOne(itpSplit)) {
    Ddi_AigOptByMonotoneCoreAcc(itpSplit,myBl,NULL,0,-1.0);
    Ddi_BddOrAcc(itpTravMgr->coneAuxSplitItp,itpSplit);
    coneConstr = Ddi_BddNot(itpSplit);
    Ddi_BddComposeAcc(coneConstr,splitV,splitU);
    Ddi_Bdd_t *cexA = Ddi_BddDup(a);
    Ddi_BddAndAcc(cexA,coneConstr);
    cex=Ddi_AigSatMinisat22WithCexAndAbortIncremental(
                        NULL,cexA,itpMgr->ns,0,-1,NULL);
    Ddi_Free(cexA);
    if (cex==NULL) {
      Ddi_Bdd_t *auxCone = Ddi_BddNot(itpTravMgr->coneAuxSplitItp);
      Ddi_BddComposeAcc(auxCone,splitV,splitU);
      Ddi_Varset_t *nsv = Ddi_VarsetMakeFromArray(itpMgr->ns); 
      int isSat;
      Ddi_Bdd_t *itpTot =
        //    Ddi_AigSat22AndWithInterpolant(NULL,myBl,myA,NULL,
        Ddi_AigSat22AndWithInterpolant(NULL,a,auxCone,NULL,
                                   nsv, NULL,NULL,0,
                                   NULL,NULL,
                                   &isSat, 0, 1, 1, -1.0);
      Pdtutil_Assert(!isSat, "unsat needed");
      Pdtutil_Assert(outItp!=NULL, "out itp needed");
      Ddi_DataCopy(outItp,itpTot);
      Ddi_Free(itpTot);
      Ddi_Free(nsv);
      Ddi_Free(auxCone);
      Ddi_Free(itpTravMgr->coneAuxSplitItp);
    }
  }
  Ddi_Free(itpSplit);
  Ddi_Free(myA);
  Ddi_Free(myBl);
  Ddi_Free(splitV);
  Ddi_Free(splitVars);
  Ddi_Free(splitU);

  Ddi_BddSetAig(coneConstr);
  Ddi_BddAndAcc(coneConstr,a);
  Ddi_Free(coneConstr);  
  return cex;
}


Ddi_Bdd_t *itpImgSplitConeConstr(
  Trav_ItpTravMgr_t * itpTravMgr, 
  Ddi_Bdd_t * kConeRings,
  Ddi_Bdd_t *a,
  Ddi_Bdd_t *cone,
  Ddi_Bdd_t *itpOut,
  int step,
  int *psat
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(a);
  Trav_ItpMgr_t *itpMgr = itpTravMgr->itpMgr;
  Trav_Mgr_t *travMgr = itpMgr->travMgr;
  int mark = Ddi_BddReadMark(cone), minMark=8;
  if (mark<minMark) return NULL;

  float coneSplitRatio = travMgr->settings.aig.igrConeSplitRatio;
  int diffBound = 4;
  int split = (mark-diffBound)*coneSplitRatio;
  int split_i = step + split;
  Ddi_Bdd_t *coneAux = itpTravMgr->coneAux;
  Ddi_Bdd_t *prevSplitItp = itpTravMgr->coneAuxSplitItp;
  
  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
    printf("\ngenerating constraining cone with split cone of bound %d\n",
           mark-diffBound);
  }

  if (itpTravMgr->coneAux==NULL) {
    int fullK = step+mark;
    int start_i = fullK-diffBound-1;
    coneAux = Ddi_BddDup(Ddi_BddPartRead(kConeRings,fullK-1));
    TravGrowConeBwdDecomp(itpMgr, coneAux, start_i, step, split_i,  
			  1, 1, NULL,
			  itpMgr->initStub, 0/*useRingConstr*/, -1/*andWithRing_i*/, 0);	
    itpTravMgr->coneAux = coneAux;
    prevSplitItp = itpTravMgr->coneAuxSplitItp = Ddi_BddMakeConstAig(ddm,0);
  }
  
  Ddi_Bddarray_t *splitU = NULL;
  Ddi_Vararray_t *splitV = NULL;
  Ddi_Varset_t *splitVars = NULL;
  Ddi_Bdd_t *myA = NULL, *myBl = NULL;
  Ddi_Vararray_t *v1=NULL,*v2=NULL, *glbA=NULL;
  Ddi_Bdd_t *coneConstr = NULL;	
  myBl = Ddi_BddDup(Ddi_BddReadComposeF(coneAux));
  splitU = Ddi_BddarrayDup(Ddi_BddReadComposeSubst(coneAux));
  splitV = Ddi_VararrayDup(Ddi_BddReadComposeVars(coneAux));
  myA =  Ddi_BddRelMakeFromArray(splitU,splitV);
  splitVars = Ddi_VarsetMakeFromArray(splitV);
  Ddi_BddSetAig(myA);
  Ddi_BddAndAcc(myA, a);
  Ddi_BddDiffAcc(myBl, prevSplitItp);

  int doLearn=1;
  if (doLearn) {
  }

  Ddi_Bdd_t *itpSplit =
    //    Ddi_AigSat22AndWithInterpolant(NULL,myBl,myA,NULL,
    Ddi_AigSat22AndWithInterpolant(NULL,myA,myBl,NULL,
                                   splitVars, NULL,NULL,0,
                                   NULL,NULL,
                                   psat, 0, 1, 1, -1.0);
  //  if (itpSplit!=NULL) Ddi_BddNotAcc(itpSplit);
  if (!(*psat) && !Ddi_BddIsOne(itpSplit)) {
    //    Ddi_BddOrAcc(itpSplit,prevSplitItp);
    Ddi_AigOptByMonotoneCoreAcc(itpSplit,myBl,NULL,0,100);
    //    Ddi_AigOptByMonotoneCoreAcc(itpSplit,myA,NULL,1,100);
    Ddi_Free(itpTravMgr->coneAuxSplitItp);
    itpTravMgr->coneAuxSplitItp = Ddi_BddDup(itpSplit);
    coneConstr = Ddi_BddDup(itpSplit);
    Ddi_BddComposeAcc(coneConstr,splitV,splitU);

    if (itpOut!=NULL) {
      Ddi_Bdd_t *bConstr = Ddi_BddNot(coneConstr);
      Ddi_Bdd_t *itp =
        //    Ddi_AigSat22AndWithInterpolant(NULL,myBl,myA,NULL,
        Ddi_AigSat22AndWithInterpolant(NULL,a,bConstr,NULL,
                                       itpMgr->nsvars,NULL,NULL,0,
                                       NULL,NULL,
                                       psat, 0, 1, 1, -1.0);
      Pdtutil_Assert(!(*psat)&&itp!=NULL,"unsat and itp expected");
      Ddi_DataCopy(itpOut,itp);
      Ddi_Free(itp);
      Ddi_Free(bConstr);
    }
  }


  Ddi_Free(itpSplit);
  Ddi_Free(myA);
  Ddi_Free(myBl);
  Ddi_Free(splitV);
  Ddi_Free(splitVars);
  Ddi_Free(splitU);

  return coneConstr;
}

void itpImgSplitConeBwdConstr(
  Trav_ItpTravMgr_t * itpTravMgr, 
  Ddi_Bdd_t * kConeRings,
  Ddi_Bdd_t *a,
  int step,
  int bound,
  int split,
  int delta,
  int *psat
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(a);
  Trav_ItpMgr_t *itpMgr = itpTravMgr->itpMgr;
  Trav_Mgr_t *travMgr = itpMgr->travMgr;

  int split_i = step + split;
  Ddi_Bdd_t *coneSplit = itpTravMgr->bwdConstr.coneSplit;
  Ddi_Bdd_t *itpSplit = itpTravMgr->bwdConstr.itpSplit;
  
  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
    printf("\ngenerating constraining cone with split cone of bound %d\n",
           bound);
  }

  if (itpTravMgr->bwdConstr.coneSplit==NULL) {
    int fullK = step+bound;
    int start_i = fullK-1;
    int np = Ddi_BddPartNum(kConeRings);
    coneSplit = Ddi_BddDup(Ddi_BddPartRead(kConeRings,np-1));
    TravGrowConeBwdDecomp(itpMgr, coneSplit, start_i, step, split_i,  
			  1, 1, NULL,
			  itpMgr->initStub, 0/*useRingConstr*/, -1/*andWithRing_i*/, 0);	
    itpTravMgr->bwdConstr.coneSplit = coneSplit;
    itpSplit = itpTravMgr->bwdConstr.itpSplit = Ddi_BddMakeConstAig(ddm,0);
    itpTravMgr->bwdConstr.bound = bound;
    itpTravMgr->bwdConstr.split = split+delta;
  }
  
  Ddi_Bddarray_t *splitU = NULL;
  Ddi_Vararray_t *splitV = NULL;
  Ddi_Varset_t *splitVars = NULL;
  Ddi_Bdd_t *myA = NULL, *myBl = NULL;
  Ddi_Vararray_t *v1=NULL,*v2=NULL, *glbA=NULL;
  Ddi_Bdd_t *coneConstr = NULL;	
  myBl = Ddi_BddDup(Ddi_BddReadComposeF(coneSplit));
  splitU = Ddi_BddarrayDup(Ddi_BddReadComposeSubst(coneSplit));
  splitV = Ddi_VararrayDup(Ddi_BddReadComposeVars(coneSplit));
  myA =  Ddi_BddRelMakeFromArray(splitU,splitV);
  splitVars = Ddi_VarsetMakeFromArray(splitV);
  Ddi_BddSetAig(myA);
  Ddi_BddAndAcc(myA, a);

  Ddi_Bdd_t *myItpSplit =
    Ddi_AigSat22AndWithInterpolant(NULL,myA,myBl,NULL,
                                   splitVars, NULL,NULL,0,
                                   NULL,NULL,
                                   psat, 0, 1, 1, -1.0);
  if (!(*psat) && !Ddi_BddIsOne(myItpSplit)) {
    //    Ddi_BddOrAcc(myItpSplit,itpSplit);
    Ddi_AigOptByMonotoneCoreAcc(myItpSplit,myBl,NULL,0,-1.0);
    Ddi_Free(itpTravMgr->bwdConstr.itpSplit);
    itpTravMgr->bwdConstr.itpSplit = Ddi_BddDup(myItpSplit);
    coneConstr = Ddi_BddNot(myItpSplit);
    Ddi_BddSubstVarsAcc(coneConstr, splitV, itpMgr->ns);
    Ddi_BddWriteMark(coneConstr,0);
    int fullK = step+bound;
    int start_i = fullK-1;
    TravGrowConeBwd(itpMgr, coneConstr, start_i+delta, step, 
                    itpMgr->delta, itpMgr->initStub,  0, -1, 1);
    Ddi_Free(itpTravMgr->bwdConstr.coneConstr);
    itpTravMgr->bwdConstr.coneConstr = coneConstr;
  }
  Ddi_Free(myItpSplit);
  Ddi_Free(myA);
  Ddi_Free(myBl);
  Ddi_Free(splitV);
  Ddi_Free(splitVars);
  Ddi_Free(splitU);

}

void itpImgSplitConeTrConstr(
  Trav_ItpTravMgr_t * itpTravMgr, 
  Ddi_Bdd_t * kConeRings,
  Ddi_Bdd_t *a,
  int step,
  int bound,
  int split,
  int delta,
  int *psat
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(a);
  Trav_ItpMgr_t *itpMgr = itpTravMgr->itpMgr;
  Trav_Mgr_t *travMgr = itpMgr->travMgr;

  int split_i = step + split;
  Ddi_Bdd_t *coneSplit = itpTravMgr->trConstr.coneSplit;
  Ddi_Bdd_t *itpSplit = itpTravMgr->trConstr.itpSplit;
  
  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
    printf("\ngenerating constraining cone with split cone of bound %d\n",
           bound);
  }

  if (itpTravMgr->trConstr.coneSplit==NULL) {
    int fullK = step+bound;
    int start_i = fullK-1;
    int np = Ddi_BddPartNum(kConeRings);
    coneSplit = Ddi_BddDup(Ddi_BddPartRead(kConeRings,np-1));
    TravGrowConeBwdDecomp(itpMgr, coneSplit, start_i, step, split_i,  
			  1, 1, NULL,
			  itpMgr->initStub, 0/*useRingConstr*/, -1/*andWithRing_i*/, 0);	
    itpTravMgr->trConstr.coneSplit = coneSplit;
    itpSplit = itpTravMgr->trConstr.itpSplit = Ddi_BddMakeConstAig(ddm,1);
    itpTravMgr->trConstr.bound = bound;
    itpTravMgr->trConstr.split = split;
  }
  
  Ddi_Bddarray_t *splitU = NULL;
  Ddi_Vararray_t *splitV = NULL;
  Ddi_Varset_t *splitVars = NULL;
  Ddi_Bdd_t *myA = NULL, *myBl = NULL;
  Ddi_Vararray_t *v1=NULL,*v2=NULL, *glbA=NULL;
  Ddi_Bdd_t *coneConstr = NULL;	
  myBl = Ddi_BddDup(Ddi_BddReadComposeF(coneSplit));
  splitU = Ddi_BddarrayDup(Ddi_BddReadComposeSubst(coneSplit));
  splitV = Ddi_VararrayDup(Ddi_BddReadComposeVars(coneSplit));
  myA =  Ddi_BddRelMakeFromArray(splitU,splitV);
  splitVars = Ddi_VarsetMakeFromArray(splitV);
  Ddi_BddSetAig(myA);
  Ddi_BddAndAcc(myBl, a);

  Ddi_Bdd_t *myItpSplit =
    Ddi_AigSat22AndWithInterpolant(NULL,myA,myBl,NULL,
                                   splitVars, NULL,NULL,0,
                                   NULL,NULL,
                                   psat, 0, 1, 1, -1.0);
  if (!(*psat) && !Ddi_BddIsOne(myItpSplit)) {
    //    Ddi_BddAndAcc(myItpSplit,itpSplit);
    Ddi_AigOptByMonotoneCoreAcc(myItpSplit,myA,NULL,1,-1.0);
    Ddi_Free(itpTravMgr->trConstr.itpSplit);
    itpTravMgr->trConstr.itpSplit = Ddi_BddDup(myItpSplit);
    coneConstr = Ddi_BddDup(myItpSplit);
    Ddi_BddComposeAcc(coneConstr,splitV,splitU);
    Ddi_Free(itpTravMgr->trConstr.coneConstr);
    itpTravMgr->trConstr.coneConstr = coneConstr;
  }
  Ddi_Free(myItpSplit);
  Ddi_Free(myA);
  Ddi_Free(myBl);
  Ddi_Free(splitV);
  Ddi_Free(splitVars);
  Ddi_Free(splitU);

}

Ddi_Bdd_t *itpImgExactBound(
  Trav_ItpTravMgr_t * itpTravMgr, 
  Ddi_Bdd_t * kConeRings,
  Ddi_Bdd_t *a,
  Ddi_Bdd_t *cone,
  Ddi_Bdd_t *prevTo,
  Ddi_Varset_t *globalVars,
  Ddi_Varset_t *domainVars,
  int step,
  int *psat
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(a);
  Trav_ItpMgr_t *itpMgr = itpTravMgr->itpMgr;
  Trav_Mgr_t *travMgr = itpMgr->travMgr;
  int mark = Ddi_BddReadMark(cone);
  Ddi_Bdd_t *coneAux = itpTravMgr->coneAux;

  if (mark<6) return NULL;
  
  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
    printf("\nusing exact bound cone\n");
  }

  if (itpTravMgr->coneAux==NULL) {
    int fullK = step+mark;
    int start_i = fullK-1;
    coneAux = Ddi_BddDup(Ddi_BddPartRead(kConeRings,fullK-1));
    int exactBoundPlusSave = travMgr->settings.aig.itpExactBoundPlus;
    travMgr->settings.aig.itpExactBoundPlus=1;
    TravGrowConeBwd(itpMgr, coneAux, start_i, step, 
                    itpMgr->delta, itpMgr->initStub,  0, -1, 0);
    travMgr->settings.aig.itpExactBoundPlus=exactBoundPlusSave;
    itpTravMgr->coneAux = coneAux;
  }
#if 1
  Ddi_Bdd_t *myCone = Ddi_BddDup(cone);
  Ddi_BddAndAcc(myCone,coneAux);
#else
  Ddi_Bdd_t *myCone = Ddi_BddDup(coneAux);
  if (prevTo!=NULL)
      Ddi_BddDiffAcc(myCone,prevTo);  
  Ddi_InfoCopy(myCone,cone);
#endif
  Ddi_Bdd_t *itpEB =
    //    Ddi_AigSat22AndWithInterpolant(NULL,myBl,myA,NULL,
    Ddi_AigSat22AndWithInterpolant(NULL,a,myCone,NULL,
                                   globalVars, domainVars, NULL,0,
                                   NULL,NULL,
                                   psat, 0, 1, 1, -1.0);
  if (!(*psat) && !Ddi_BddIsOne(itpEB)) {
    Ddi_AigOptByMonotoneCoreAcc(itpEB,a,NULL,1,-1.0);
    int s0 = Ddi_BddSize(cone);
    Ddi_BddDiffAcc(cone,coneAux);
    Ddi_BddAndAcc(cone, itpEB);
    Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
      printf("constraining cone (%d) with exactBoundCone (%d) and itp (%d) -> %d\n",
             s0, Ddi_BddSize(coneAux), Ddi_BddSize(itpEB), Ddi_BddSize(cone));
    }
  }
  Ddi_Free(myCone);
  return itpEB;
}


static Ddi_Bdd_t *itpImgWithPrevTo (
  Trav_ItpTravMgr_t * itpTravMgr,
  Ddi_Bdd_t * kConeRings,
  Ddi_Bdd_t *a,
  Ddi_Bdd_t *b,
  Ddi_Bdd_t *prevTo,
  Ddi_Bdd_t *optCare,
  Ddi_Bdd_t *itpPlus,
  Ddi_Bdd_t *partWindow,
  Ddi_Varset_t *globalVars,
  Ddi_Varset_t *domainVars,
  int step,
  int *psat,
  int itpPart,
  int itpOdc,
  float timeLimit
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(a);
  Trav_ItpMgr_t *itpMgr = itpTravMgr->itpMgr;
  Trav_Mgr_t *travMgr = itpMgr->travMgr;
  static int compareWithItp=0;
  int usePrevToWithA = 0; int useCareWithA = 0;
  int usePrevToWithB = 1; int useCareWithB = 1;
  int useDontCare = 0;
  int tryPrevImgLearning = 0;
  Ddi_Bdd_t *aNew = Ddi_BddDup(a);
  Ddi_Bdd_t *bNew = Ddi_BddDup(b);
  Ddi_Bdd_t *careAig = NULL;
  Ddi_Bdd_t *coneAux=NULL, *itp=NULL;
  Ddi_Vararray_t **tfPiVars = itpMgr->timeFrames->PiVars;
  int tfPiNum =  itpMgr->timeFrames->Num;

  if (usePrevToWithA) {
    Ddi_BddDiffAcc(aNew,prevTo);
    //      Ddi_BddOrAcc(aNew,prevTo);
    if (itpTravMgr->prevFrom!=NULL && !Ddi_BddIsConstant(itpTravMgr->prevFrom))
      Ddi_BddDiffAcc(aNew,itpTravMgr->prevFrom);
  }
  if (usePrevToWithB) {
    Ddi_BddDiffAcc(bNew,prevTo);
  }
  if (optCare!=NULL) {
    if (useCareWithA) {
      if (!Ddi_BddIncluded(aNew,optCare))
        Ddi_BddAndAcc(aNew,optCare);
    }
    if (useCareWithB) {
      Ddi_BddAndAcc(bNew,optCare);
    }
  }
  if (itpPlus!=NULL) {
    if (useCareWithA) {
      if (!Ddi_BddIncluded(aNew,itpPlus))
        Ddi_BddAndAcc(aNew,itpPlus);
    }
    if (useCareWithB) {
      Ddi_BddAndAcc(bNew,itpPlus);
    }
  }
  if (useDontCare) {
    careAig = Ddi_BddMakePartConjVoid(ddm);
    Ddi_BddNotAcc(prevTo);
    Ddi_BddPartInsertLast(careAig,prevTo);
    Ddi_BddNotAcc(prevTo);
    if (optCare!=NULL) {
      Ddi_BddPartInsertLast(careAig,optCare);
    }
    Ddi_BddSetAig(careAig);
    ddm->settings.aig.itpUseCare = 1;
  }

  int prevToSize = Ddi_BddSize(prevTo);
  int useTrConstr = 0;
  int enBwdSplitConstr = 0 && (prevToSize>1000);
  if (enBwdSplitConstr) {
    int mark = Ddi_BddReadMark(bNew);
    int delta = 4;
    int bound = mark - delta;
    int split = bound/(useTrConstr?1.5:4);
    if (split >= 1) {
      if (useTrConstr) {
        if (!itpTravMgr->trConstr.active)
          itpImgSplitConeTrConstr(itpTravMgr, kConeRings,aNew,step,bound,split,delta,psat);
        if (prevToSize > 7000)
          itpTravMgr->trConstr.active = 1;
      }
      else {
        if (!itpTravMgr->bwdConstr.active)
          itpImgSplitConeBwdConstr(itpTravMgr, kConeRings,aNew,step,bound,split,delta,psat);
        if (prevToSize > 8000)
          itpTravMgr->bwdConstr.active = 1;
      }
    }
  }
  
  float coneSplitRatio = travMgr->settings.aig.igrConeSplitRatio;
  int enOptB = coneSplitRatio<0.99;
  Ddi_Bdd_t *coneSplitItp = NULL;
  if (enOptB) {
    //    enPart = 0; // not Compatible as uses itpNew as care
    coneSplitItp = Ddi_BddMakeConstAig(ddm,1);
    Ddi_Bdd_t *constrCone = itpImgSplitConeConstr(
                              itpTravMgr,kConeRings,aNew,bNew,
                              coneSplitItp,step,psat);
    if (constrCone!=NULL) {
      int s0 = Ddi_BddSize(bNew);
      Ddi_BddAndAcc(bNew,constrCone);
      //      Ddi_BddNotAcc(constrCone);
      //      Ddi_BddOrAcc(bNew,constrCone);
      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
        printf("constraining cone (%d) with coneConstr (%d) -> %d\n",
               s0, Ddi_BddSize(constrCone), Ddi_BddSize(bNew));
      }
      Ddi_Free(constrCone);
    }
    else {
      Ddi_Free(coneSplitItp);
    }
  }
  int enExactBoundOpt = 0;
  Ddi_Bdd_t *itpExactBound=NULL;
  if (enExactBoundOpt) {
    itpExactBound = itpImgExactBound(
                                     itpTravMgr,kConeRings,aNew,bNew,prevTo,
                                  globalVars, domainVars,
                                  step,psat);
  }

  
  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
    printf("Computing ItpWithNew\n");
  }
  if (0 && itpTravMgr->prevFrom!=NULL && 
      !Ddi_BddIsOne(itpTravMgr->prevFrom)) {
    Ddi_BddDiffAcc(aNew,itpTravMgr->prevFrom);
  }

  float clungItpRatio = travMgr->settings.aig.igrClungItpRatio;
  int clungItpTh = travMgr->settings.aig.igrClungItpTh;
  Ddi_Bdd_t *clungItp=NULL;
  if (clungItpRatio>0.0 && Ddi_BddSize(bNew)>clungItpTh) {
    clungItp = Ddi_BddMakeConstAig(ddm,1);

    Pdtutil_Assert(itpTravMgr->stats.coneSizeNoCare>0,"missing cone size stat");
    float scaleStep = (1-clungItpRatio)/Ddi_BddSize(bNew);
    scaleStep *= itpTravMgr->stats.coneSizeNoCare;
    clungItpRatio += scaleStep;
    Pdtutil_Assert(clungItpRatio <=1.01,"wrong ratio");
  }
  else clungItpRatio = -1.0; // enforce disable

  int partTh = Ddi_MgrReadAigItpPartTh(ddm);
  int enPart = 0 && (Ddi_BddSize(prevTo)>partTh);
  int nPart = 4;
  int enWindow = 1;
  int enPartWindow = partWindow!=NULL && Ddi_BddPartNum(partWindow) > 2*nPart;
  int enPartWindow2 = partWindow==NULL && (Ddi_BddSize(prevTo)>partTh) &&
    itpTravMgr->imgPart.windowLits != NULL;
  int enSplitConstr = enPart;
  int again = 1;
  Ddi_Bdd_t *aCare = Ddi_BddNot(prevTo);
  Ddi_Bdd_t *itpNew=Ddi_BddMakeConstAig(ddm,(enPart)?0:1);
  int doOptNew = 1;
  Ddi_Vararray_t *filterv = Ddi_VararrayDup(itpMgr->ps);
  int nDiff = Ddi_VararrayNum(filterv)/3;
  Ddi_Bdd_t *constrCube=NULL;
  Ddi_Bdd_t *coneSplitOutItp=NULL;
  Ddi_Bddarray_t *windowLits=NULL;
  Ddi_Bdd_t *pW=NULL;
  Ddi_Bdd_t *pWtot = Ddi_BddMakePartConjVoid(ddm); 

  if (coneSplitItp!=NULL && !enPart) {
    Ddi_BddAndAcc(itpNew,coneSplitItp);
  }
  Ddi_Free(coneSplitItp);
  
  if (enPartWindow) {
    for (int j=0;j<nPart;j++) {
      pW = Ddi_BddMakePartConjVoid(ddm);
      Ddi_BddPartInsertLast(pWtot,pW);
      Ddi_Free(pW);
    }
    for (int j=0; j<Ddi_BddPartNum(partWindow); j++) {
      int jj=j%nPart;
      pW = Ddi_BddPartRead(pWtot,jj);
      Ddi_BddPartInsertLast(pW,Ddi_BddPartRead(partWindow,j));
    }
    for (int j=0;j<nPart;j++) {
      pW = Ddi_BddPartRead(pWtot,j);
      Ddi_BddSetAig(pW);
    }
  }

  if (partTh>0 && enWindow && (Ddi_BddSize(prevTo)<=partTh)) {
    Ddi_Free(itpTravMgr->imgPart.windowLits);
    windowLits = itpTravMgr->imgPart.windowLits = Ddi_BddarrayAlloc(ddm, 0); 
  }
  
  for (int ii=0; again; ii++) {
    again=0;
    Ddi_Bdd_t *myA = Ddi_BddDup(aNew);
    Ddi_Bdd_t *myB = Ddi_BddDup(bNew);
    Ddi_Bdd_t *aAndCare = Ddi_BddDup(myA), *cex=NULL;
    int n0=0;
    if (enPart) {
      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
        printf("\nA partitioned IMG iteration %d\n", ii);
      }
      Ddi_BddAndAcc(aAndCare,aCare);
      Ddi_Bdd_t *aAndCareConstrained = Ddi_BddDup(aAndCare);
      if (constrCube!=NULL)
        Ddi_BddAndAcc(aAndCareConstrained,constrCube);
      Ddi_Free(constrCube);
      cex=Ddi_AigSatMinisat22WithCexAndAbortIncremental(
                        NULL,aAndCareConstrained,filterv,0,-1,NULL);
      Ddi_Free(aAndCareConstrained);
      if (cex==NULL)
        break;
      Ddi_Varset_t *supp = Ddi_BddSupp(cex);
      n0 = Ddi_VarsetNum(supp);
      Ddi_Free(supp);
      nDiff = Ddi_VararrayNum(filterv)/4;
      //      if (nDiff <8) nDiff=8;
      again = 1;
      Ddi_AigAndCubeAcc(myA,cex);
      Ddi_BddDiffAcc(bNew,itpNew);
    }
    else {
      //      Ddi_BddAndAcc(myB,itpNew);
    }
    if (enPartWindow) {
      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
        printf("\nA partitioned IMG in window (iter: %d)\n", ii);
      }
      Ddi_Bdd_t *myPartWindow=NULL;
      if (ii==0) {
        again=1;
        myPartWindow = Ddi_BddMakeAig(pWtot);
      }
      else {
        again=ii<Ddi_BddPartNum(pWtot);
        myPartWindow = Ddi_BddNot(Ddi_BddPartRead(pWtot,(ii-1)));
      }
      Ddi_BddAndAcc(myB,myPartWindow);
#if 0
      // doesn't seem to improve
      Ddi_BddNotAcc(itpNew);
      Ddi_BddOrAcc(myB,itpNew);
      Ddi_BddDiffAcc(myB,prevTo);
      Ddi_BddNotAcc(itpNew);
#endif
      Ddi_Free(myPartWindow);
    }
    else if (enPartWindow2) {
      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
        printf("\nA partitioned IMG in window (iter: %d)\n", ii);
      }
      Ddi_Bdd_t *myPartWindow=Ddi_BddarrayRead(itpTravMgr->imgPart.windows,ii);
      Ddi_BddAndAcc(myB,myPartWindow);      
      again=ii<Ddi_BddarrayNum(itpTravMgr->imgPart.windows)-1;
      //      Ddi_BddNotAcc(itpNew);
      Ddi_BddAndAcc(myB,itpNew);
      //      Ddi_BddNotAcc(itpNew);
    }
    
    if (enPartWindow || enPartWindow2) {

      static int tryNonPart=0;
      if (tryNonPart && ii==0) {
        Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
          printf("\nA try nonPart\n");
        }
        Ddi_Bdd_t *itpNew_base = Ddi_AigSat22AndWithInterpolantAndClung(
                                                  itpTravMgr->incrSat,
                                                  myA,bNew,NULL,
                                                  globalVars, domainVars,
                                                  tfPiVars,tfPiNum,
                                                  careAig,prevTo,
                                                  NULL,
                                                  clungItp,clungItpRatio,
                                                  psat, itpPart, itpOdc, 
                                                  0,timeLimit);
        Ddi_Free(itpNew_base);
        Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
          printf("\nA now try part\n");
        }
      }

    }

    Ddi_Bdd_t *itpNew_i = Ddi_AigSat22AndWithInterpolantAndClung(
                                                  itpTravMgr->incrSat,
                                                  myA,myB,NULL,
                                                  globalVars, domainVars,
                                                  tfPiVars,tfPiNum,
                                                  careAig,prevTo,
                                                  windowLits,
                                                  clungItp,clungItpRatio,
                                                  psat, itpPart, itpOdc, 
                                                  0,timeLimit);

    if (windowLits!=NULL) {
      Ddi_Bddarray_t *windows = Ddi_BddarrayAlloc(ddm, 0);
      int np=3;
      Ddi_Bdd_t *wAnd = Ddi_BddMakePartConjFromArray(windowLits);
      for (int ii=0; ii<np; ii++) {
        Ddi_Bdd_t *w_ii = Ddi_BddMakePartDisjVoid(ddm);
        Ddi_BddarrayWrite(windows,ii,w_ii);
        Ddi_Free(w_ii);
      }
      Ddi_BddarrayWrite(windows,np,wAnd);
      Ddi_Free(wAnd);
      for (int ii=0; ii<Ddi_BddarrayNum(windowLits); ii++) {
        Ddi_Bdd_t *w_i = Ddi_BddNot(Ddi_BddarrayRead(windowLits,ii));
        int id = ii%np;
        Ddi_Bdd_t *w_id = Ddi_BddarrayRead(windows,id);
        Ddi_BddPartInsertLast(w_id,w_i);
        Ddi_Free(w_i);
      }
      for (int ii=0; ii<=np; ii++) {
        Ddi_Bdd_t *w_ii = Ddi_BddarrayRead(windows,ii);
        Ddi_BddSetAig(w_ii);
      }
      Ddi_Free(itpTravMgr->imgPart.windows);
      itpTravMgr->imgPart.windows = windows;
    }

    if (0 && itpNew_i != NULL && enPartWindow) {
      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
        printf("\nA partitioned IMG out of window\n");
      }
      Ddi_Free(myB);
      myB = Ddi_BddDup(bNew);
      Ddi_BddDiffAcc(myB,partWindow);
      Ddi_BddAndAcc(myB,itpNew_i);

      Ddi_Bdd_t *itpNew_i2 = Ddi_AigSat22AndWithInterpolantAndClung(
                                                  itpTravMgr->incrSat,
                                                  myA,myB,NULL,
                                                  globalVars, domainVars,
                                                  tfPiVars,tfPiNum,
                                                  careAig,prevTo,
                                                  NULL,
                                                  clungItp,clungItpRatio,
                                                  psat, itpPart, itpOdc, 
                                                  0,timeLimit);
      if (itpNew_i2!=NULL) {
        Ddi_BddAndAcc(itpNew_i,itpNew_i2);
        Ddi_Free(itpNew_i2);
      }
      else {
        Ddi_Free(itpNew_i);
      }
    }

    ddm->settings.aig.itpNoQuantify = 0;
    ddm->settings.aig.itpUseCare = 0;
    if (itpNew_i!=NULL) {
      if (itpExactBound!=NULL) {
        if (!Ddi_BddIncluded(itpNew_i,itpExactBound))
          Ddi_BddAndAcc(itpNew_i,itpExactBound);
      }
      if (doOptNew) {
        coneAux = Ddi_BddNot(itpNew_i);
        Ddi_BddDiffAcc(coneAux,prevTo);
        Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
          printf("Optimizing itp using aAndNew\n");
        }
        Ddi_Free(itpNew_i);
        itpNew_i = Ddi_AigSat22AndWithInterpolant(NULL,
                                                myA,coneAux,NULL,
                                                globalVars, domainVars,
                                                tfPiVars,tfPiNum,
                                                optCare,NULL,
                                                psat, 0, itpOdc, 
                                                0,timeLimit);
        Ddi_Free(coneAux);
      }
      if (!enPart)
        Ddi_BddAndAcc(itpNew,itpNew_i);
      else
        Ddi_BddOrAcc(itpNew,itpNew_i);
      Ddi_Free(constrCube);
      if (n0==0 && !(enPartWindow || enPartWindow2)) {
        again=0;
      }
      else if (enPart) {
        Ddi_BddDiffAcc(aCare,itpNew_i);
        Ddi_BddDiffAcc(aAndCare,itpNew_i);
        int sat1 = Ddi_AigSatMinisatWithAbortAndFinal (aAndCare,
                                                       cex, -1, 0);
        Pdtutil_Assert (!sat1,"unsat needed");
        Ddi_Vararray_t *suppA = Ddi_BddSuppVararray(cex);
        int n1 = Ddi_VararrayNum(suppA);
        while (n1>=1 && ((n1+nDiff)>=n0)) {
          Ddi_VararrayRemove(suppA,n1-1); n1--;
        }
        Ddi_VararrayIntersectAcc(filterv,suppA);
        Ddi_Free(suppA);
        Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
          printf("constraining cube abstracted from %d to %d vars\n",
                 n0, n1);
        }
        if (enSplitConstr && itpNew_i!=NULL) {
          if (coneSplitOutItp==NULL)
            coneSplitOutItp=Ddi_BddMakeConstAig(ddm,0);
          constrCube = itpImgSplitConeCubeConstr(
             itpTravMgr,kConeRings,aAndCare,bNew,itpNew_i,
             coneSplitOutItp,step,ii,psat);
          if (constrCube==NULL || *psat) {
            again=0;
            if (!*psat) {
              Pdtutil_Assert(constrCube==NULL,"no cube expected");
              Ddi_BddOrAcc(itpNew,coneSplitOutItp);
            }
            else {
              Ddi_Free(itpNew);
            }
          }
        }
      }
    }
    else {
      again=0;
      Ddi_Free(itpNew);
    }
    Ddi_Free(itpNew_i);
    Ddi_Free(aAndCare);
    Ddi_Free(myA);
    Ddi_Free(myB);
    Ddi_Free(cex);
  } 
  Ddi_Free(pWtot);
  Ddi_Free(filterv);
  Ddi_Free(coneSplitOutItp);
  
  Ddi_Free(constrCube);
  Ddi_Free(aCare);
  Ddi_Free(aNew);
  Ddi_Free(bNew);
  Ddi_Free(careAig);
  if (itpNew!=NULL) {
    if (optCare!=NULL && useCareWithB) {
      if (!Ddi_BddIncluded(itpNew,optCare))
        Ddi_BddAndAcc(itpNew,optCare);
    }
    if (itpPlus!=NULL && useCareWithB) {
      if (!Ddi_BddIncluded(itpNew,itpPlus))
        Ddi_BddAndAcc(itpNew,itpPlus);
    }

    int doOpt = 0;
    if (doOpt) {
      static int cutTh0 = 50000;
      char log[20];
      Ddi_Bdd_t *myCare = Ddi_BddNot(prevTo);
      int size0=Ddi_BddSize(itpNew);
      if (size0>cutTh0) {
        int cutTh=cutTh0/2+size0/4;
        for (int ii=0; ii<3 && (Ddi_BddSize(itpNew) > cutTh); ii++) {
          sprintf(log,"itpNew (iter: %d)", ii);
          (void) Ddi_AigOptNnfWithCut(itpNew,myCare,0,1,cutTh,-1,log);
          (void) Ddi_AigOptNnfWithCut(itpNew,myCare,0,0,cutTh,-1,log);
          cutTh *= 2;
        }
      }
      (void) Ddi_AigOptNnfWithCut(itpNew,myCare,0,1,-1,-1,"itpNew");
      (void) Ddi_AigOptNnfWithCut(itpNew,myCare,0,0,-1,-1,"itpNew");
      Ddi_Free(myCare);
    }
    coneAux = Ddi_BddNot(itpNew);
    if (tryPrevImgLearning && !Ddi_BddIsConstant(coneAux)) {
      Ddi_Free(itpTravMgr->careForBwdCone);
      itpTravMgr->careForBwdCone = Ddi_BddDup(coneAux);
    }
    
    Ddi_BddDiffAcc(coneAux,prevTo);
    if (0&&optCare!=NULL) {
      Ddi_Bdd_t *notCare = Ddi_BddNot(optCare);
      Ddi_BddOrAcc(coneAux,notCare);
      Ddi_Free(notCare);
    }
    //      Ddi_Free(itpNew);
    //      Ddi_BddAndAcc(coneAux,b);
    Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
      printf("Re-Computing itp using ItpWithNew\n");
    }
    itp = Ddi_AigSat22AndWithInterpolant(NULL,a,coneAux,NULL,
                                         globalVars, domainVars,
                                         tfPiVars,tfPiNum,
                                           optCare,NULL,
                                           psat, 0, itpOdc, 
                                           0,timeLimit);
    Ddi_Free(itpNew);
    int doWeaken = 0&itp!=NULL && travMgr->settings.aig.itpWeaken;
    doWeaken &= Ddi_BddSize(itp)>travMgr->settings.aig.itpWeaken;
    // disabled for now as it weakens too much (convergence): no, just hits cone earlier
    if (doWeaken) {
      Ddi_AigOptByMonotoneCoreAcc(itp,coneAux,NULL,0,-1.0);
    }
    Ddi_Free(coneAux);
    if (compareWithItp) {
      Pdtutil_Assert(Ddi_BddIncluded(a,itp),"problem with NEW");
      Pdtutil_Assert(!Ddi_AigSatAnd(itp,b,optCare),"problem witn NEW");
      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
        printf("Comparing ItpWithNew to standard ITP\n");
      }
      Ddi_Bdd_t *itpRef = Ddi_AigSat22AndWithInterpolant(NULL,a,b,NULL,
                                                         globalVars, domainVars,
                                                         tfPiVars,tfPiNum,
                                                         optCare,NULL,
                                                         psat, 0, itpOdc, 
                                                         0,timeLimit);
      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
        printf("ItpWithNew / std Itp: %d / %d\n", Ddi_BddSize(itp),
               Ddi_BddSize(itpRef));
      }
      Ddi_Free(itpRef);
    }
  
    if (clungItp!=NULL /*&& Ddi_BddSize(clungItp)<2000000*/) {
      Ddi_BddAndAcc(clungItp,itp);
      Ddi_Free(itpTravMgr->careForBwdCone);
      itpTravMgr->careForBwdCone = Ddi_BddNot(clungItp);
    }
  }
  Ddi_Free(clungItp);
  Ddi_Free(itpExactBound);
  return itp;
}



/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************
  Synopsis    [Interpolant based image]
  Description [Interpolant besed image]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
TravItpImgPart (
  Trav_ItpTravMgr_t * itpTravMgr,
  Ddi_Bdd_t * kCone,
  Ddi_Bdd_t * kConeRings,
  Ddi_Bdd_t *a,
  Ddi_Bdd_t *b,
  Ddi_Bdd_t *prevTo,
  int step,
  int doSplit,
  Ddi_Varset_t *globalVars,
  Ddi_Varset_t *domainVars,
  Ddi_Bdd_t *optCare,
  Ddi_Bdd_t *itpPlus,
  Ddi_Bdd_t *toPlusCube,
  Ddi_Bdd_t *partWindow,
  int *psat,
  int itpPart,
  int itpOdc,
  float timeLimit
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(a);
  Trav_ItpMgr_t *itpMgr = itpTravMgr->itpMgr;
  Trav_Mgr_t *travMgr = itpMgr->travMgr;
  int i, chkresSat=0, chkincr=0, 
    chkres = 0, thVars = itpPart>8 ? 8*itpPart : 1<<itpPart;
  int tryCexCore = 0;
  int doForall = 0;
  long startTime, cexTime, itpTime;
  int mark = Ddi_BddReadMark(b);
  int isSat=0, preSplit=1;
  int nnfSubset = 0, genClauses=0;
  int genCubes = 1;
  int doItpFromGenClauses = 0;
  int doItpFromGenClausesAux = 0;
  int doItpStrengthen = 0;
  Ddi_Bdd_t *itp_i=NULL, *itpTot = NULL;
  Ddi_Bdd_t *careTot = NULL, *careTotSplit = NULL, *toMinusSplit=NULL;
  Ddi_Bdd_t *careTotForCex =  NULL, *careFinal = NULL,
    *careTotForA=NULL, *careForPreimg=NULL;
  Ddi_Bdd_t *bDup = NULL;

  Ddi_Bdd_t *splitB = NULL, *splitRel = NULL, *splitRel2 = NULL, 
    *splitCare = NULL, *splitConstr = NULL;
  Ddi_Vararray_t *piAux = NULL;
  Ddi_Vararray_t *splitVaux = NULL;
  Ddi_Vararray_t *splitVaux2 = NULL;
  Ddi_Vararray_t *splitV2 = NULL;
  Ddi_Bddarray_t *splitU = NULL;
  Ddi_Vararray_t *splitV = NULL;
  Ddi_Vararray_t *splitRefV;
  Ddi_Varset_t *splitVars = NULL;
  Ddi_Bdd_t *itpNextRing = NULL;
  int itpPartTh = 0; // Ddi_MgrReadAigItpPartTh(ddm);
  int doSplit2 = 0 && (doSplit/2);
  int doSplitCare = 0, doFwdBwd = 0;
  int enLowerBoundk = 0;
  Ddi_Vararray_t **tfPiVars = itpMgr->timeFrames->PiVars;
  int tfPiNum =  itpMgr->timeFrames->Num;
  int andWithRing_i = -1;
  int growCone = abs(Trav_MgrReadIgrGrowCone(travMgr));
  int useRingConstr = growCone != 1 ? 2 : 0;
  static int nCalls=0;
  int enFwdCare = 0;
  int fullK = step+mark;
  int safe = Trav_ItpMgrReadConeBoundOK(itpMgr,step);
  int split_i = step+doSplit;
  int dLB = doSplit/2;
  int genNextRing = 1;
  int incrementalSat = 1;
  Ddi_IncrSatMgr_t *ddiS = NULL;
  int enPartA = 0, enDisjDecomp = 1 && itpTravMgr->observedGates==NULL;
  int completeOnSplit = 1;
  float itpRefTime = 100; // 10; // -1.0;
  int doFixedPart = 1;
  int checkStall = 0;
  int cexWithGates = 16; //itpPart*2;
  
  if (dLB>4) dLB=4;

  nCalls++;
  int tryIncrSat = 0;
  if (tryIncrSat && itpTravMgr->incrSat==NULL) {
    Ddi_Mgr_t *ddmDup = Ddi_MgrDup(ddm);
    itpTravMgr->incrSat =
      //      Ddi_IncrSatMgrAlloc(ddmDup, 1/*Minisat22*/, 0, 0);
      Ddi_IncrSatMgrAlloc(ddmDup, -1/*lgl*/, 0, 0);
    DdiAig2CnfIdInit(ddmDup);
  }

  if (0 && optCare!=NULL && Ddi_BddSize(optCare)> 100) {
    if (1 && Ddi_BddSize(b) > 10) {
      //      DdiAigRedRemovalControlAcc(b, optCare, 10, 30.0);
      if (!Ddi_AigSatConstrain(b,optCare,-1.0, NULL))
        return Ddi_BddDup(optCare);
    }
  }
  
  if (itpTravMgr->imgPartVars != NULL && itpMgr->hints.hintsEnabled) {
    Ddi_Bdd_t *a0, *a1, *b0, *b1, *a00, *a01,
      *itpPlus0=NULL, *itpPlus1=NULL, 
      *optCare0=NULL, *optCare1=NULL, 
      *itp0=NULL, *itp00=NULL, *itp01=NULL, *itp1=NULL, *itp=NULL;
    int iConstr0 = itpMgr->hints.invar0_i;
    int sat0=0, sat1=0;
    Ddi_Var_t *cvar0Ns = Ddi_VararrayRead(itpMgr->ns,iConstr0); 
    Ddi_Var_t *cvar0Ps = Ddi_VararrayRead(itpMgr->ps,iConstr0); 
    Ddi_Vararray_t *savePartVars = itpTravMgr->imgPartVars;
    int prevImgOk = 0, prevImgSafe = 0;
    Ddi_Bdd_t *deltaConstr0 = Ddi_BddarrayRead(itpMgr->delta,iConstr0); 
    int enNo10 = 1 && step>1 && !Ddi_BddIsZero(deltaConstr0);
    itpTravMgr->imgPartVars = NULL;
    a0 = Ddi_BddCofactor(a,cvar0Ns,0);
    a00 = Ddi_BddCofactor(a0,cvar0Ps,0);
    a01 = Ddi_BddCofactor(a0,cvar0Ps,1);
    b0 = Ddi_BddCofactor(b,cvar0Ns,0);
    a1 = Ddi_BddCofactor(a,cvar0Ns,1);
    b1 = Ddi_BddCofactor(b,cvar0Ns,1);

    if (enNo10) {
      int chk10 = 1;
      if (chk10) {
	Ddi_Bdd_t *a10 = Ddi_BddCofactor(a1,cvar0Ps,0); // no 0 -> 1 edge in tr
	//      Ddi_BddAndAcc(a10,b1);
	Ddi_Bdd_t *cex = Ddi_AigSatWithCex(a10);
	if (cex!=NULL) {
	  Ddi_Free(cex);
	}
	Ddi_Bdd_t *a11 = Ddi_BddCofactor(a1,cvar0Ps,1); // no 1 -> 1 edge in tr
	if (!Ddi_AigSat(a11)) {
	  printf("not sat a11 (within prev hint space)\n");
	}
	Ddi_Free(a10);
	Ddi_Free(a11);
      }
      
      //      Ddi_BddCofactorAcc(a1,cvar0Ps,1); // no 0 -> 1 edge in tr
    }
    if (itpPlus!=NULL) {
      itpPlus0 = Ddi_BddCofactor(itpPlus,cvar0Ns,0);
      itpPlus1 = Ddi_BddCofactor(itpPlus,cvar0Ns,1);
    }
    if (optCare!=NULL) {
      optCare0 = Ddi_BddCofactor(optCare,cvar0Ns,0);
      optCare1 = Ddi_BddCofactor(optCare,cvar0Ns,1);
      prevImgOk = Ddi_BddIncluded(a1,optCare1);
      prevImgSafe = 0 && !Ddi_AigSatAnd(b1,optCare1,NULL);
    }
    if (psat!=NULL) *psat = 0;
    Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
      printf("\nITP IMG partitioned by hints ->1 (to prev hint space)\n");
    }
    if (prevImgOk && prevImgSafe) {
      itp1 = Ddi_BddDup(optCare1);
    }
    else if (!Ddi_AigSat(a1)) {
      itp1 = Ddi_BddMakeConstAig(ddm,0);
    }
    else {
      itp1 = TravItpImgPart (itpTravMgr,kCone,kConeRings,a1,b1,prevTo,
			 step,doSplit,globalVars,domainVars,optCare1,
                         itpPlus1,toPlusCube,NULL,&sat1,itpPart,itpOdc,timeLimit);
    }
    if (sat1) {
      if (psat!=NULL) *psat = sat1;
      itp = NULL;
    }
    else {
      //      Ddi_BddDiffAcc(a01,itp1);
      //      Ddi_BddDiffAcc(a00,itp1);
      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
	printf("\nITP IMG partitioned by hints 1->0 (out from prev hint space)\n");
      }
      if (!Ddi_AigSat(a01)) {
	itp01 = Ddi_BddMakeConstAig(ddm,0);
      }
      else {
	itp01 = TravItpImgPart (itpTravMgr,kCone,kConeRings,a01,b0,prevTo,
			  step,doSplit,globalVars,domainVars,optCare0,
                          itpPlus0,toPlusCube,NULL,&sat0,itpPart,itpOdc,timeLimit);
      }
      if (sat0) {
	if (psat!=NULL) *psat = sat0;
	itp = NULL;
	Ddi_Free(itp1);
      }
    }
    if (itp01!=NULL) {
      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
	printf("\nITP IMG partitioned by hints 0->0 (within new hint, out of prev hint)\n");
      }
      if (!Ddi_AigSat(a00)) {
	itp00 = Ddi_BddMakeConstAig(ddm,0);
      }
      else {
	itp00 = TravItpImgPart (itpTravMgr,kCone,kConeRings,a00,b0,prevTo,
			   step,doSplit,globalVars,domainVars,optCare0,
                           itpPlus0,toPlusCube,NULL,&sat0,itpPart,itpOdc,timeLimit);
      }
      if (sat0) {
	if (psat!=NULL) *psat = sat0;
	itp = NULL;
	Ddi_Free(itp1);
	Ddi_Free(itp01);
      }
      else {
	itp0 = Ddi_BddOr(itp00,itp01);
      }
      Ddi_Free(itp00);
      Ddi_Free(itp01);
      if (itp0!=NULL) {
	static int chk=0;
	int doItePart = 1;
	Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(cvar0Ns, 1);
	//       	Ddi_BddOrAcc(itp0,itp1);
	if (doItePart) {
	  Ddi_Bdd_t *itp0Part = Ddi_AigPartitionTop(itp0,0);
	  Ddi_Bdd_t *itp1Part = Ddi_AigPartitionTop(itp1,0);
	  int i;
	  for (i=0; i<Ddi_BddPartNum(itp0Part); i++)
	    Ddi_BddOrAcc(Ddi_BddPartRead(itp0Part,i),lit);
	  Ddi_BddNotAcc(lit);
	  for (i=0; i<Ddi_BddPartNum(itp1Part); i++) {
	    Ddi_Bdd_t *p_i = Ddi_BddPartRead(itp1Part,i);
	    Ddi_BddOrAcc(p_i,lit);
	    Ddi_BddPartInsertLast(itp0Part,p_i);
	  }
	  Ddi_BddNotAcc(lit);
	  itp = Ddi_BddDup(itp0Part);
	  Ddi_Free(itp0Part);
	  Ddi_Free(itp1Part);
	}
	else {
	  itp = Ddi_BddIte(lit,itp1,itp0);
	}
	if (chk) {
	  Ddi_Bdd_t *itpChk = Ddi_BddMakeAig(itp);
	  if (!Ddi_BddIncluded(a,itpChk)) {
	    Ddi_Varset_t *psv = Ddi_VarsetMakeFromArray(itpMgr->ns);
	    Ddi_VarsetSetArray(psv);
	    Ddi_Bdd_t *d = Ddi_BddDiff(a,itpChk);
	    Ddi_Bdd_t *cex = Ddi_AigSatWithCex(d);
	    Ddi_BddExistProjectAcc(cex,psv);
	    Ddi_BddCofactorAcc(cex,cvar0Ns,0);
	    Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(cvar0Ns, 1);
	    Ddi_BddAndAcc(cex,lit);
	    Ddi_Free(d);
	    Ddi_Free(lit);
	    Ddi_Free(psv);
	    Ddi_Free(cex);
	  }
	  Pdtutil_Assert(Ddi_BddIncluded(a,itpChk),"unsound ITP");
	  if (0 && Ddi_AigSatAnd(b,itpChk,optCare)) {
	    Ddi_Varset_t *psv = Ddi_VarsetMakeFromArray(itpMgr->ns);
	    Ddi_VarsetSetArray(psv);
	    Ddi_Bdd_t *cex = Ddi_AigSatAndWithCexAndAbort(b,
	      itpChk, optCare, NULL, 100, NULL);
	    Ddi_BddExistProjectAcc(cex,psv);
	    Ddi_BddCofactorAcc(cex,cvar0Ns,0);
	    Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(cvar0Ns, 1);
	    Ddi_BddAndAcc(cex,lit);
	    Ddi_Free(lit);
	    Ddi_Free(psv);
	    Ddi_Free(cex);
	  }
	  Pdtutil_Assert(!Ddi_AigSatAnd(b,itpChk,optCare),"wrong ITP");
	  Ddi_Free(itpChk);
	}
	Ddi_Free(lit);
      }
    }
    Ddi_Free(optCare0);
    Ddi_Free(optCare1);
    Ddi_Free(itpPlus0);
    Ddi_Free(itpPlus1);
    Ddi_Free(itp0);
    Ddi_Free(itp1);
    Ddi_Free(a00);
    Ddi_Free(a01);
    Ddi_Free(a0);
    Ddi_Free(b0);
    Ddi_Free(a1);
    Ddi_Free(b1);
    itpTravMgr->imgPartVars = savePartVars;
    return itp;
  }
  else if (itpTravMgr->imgPartVars != NULL) {
    int doIte = 1;
    int bTh = 1000, window = 8;
    int nPart = 4;
    int i, itpDone=0;
    if (Ddi_BddSize(b)>bTh) {
      int sat_i;
      Ddi_Bdd_t *leftover = Ddi_BddMakeConstAig(ddm,1);
      Ddi_Bdd_t *itp = Ddi_BddMakeConstAig(ddm,0);
      for (i=0; !itpDone && itp!=NULL && i<nPart-1; i++) {
	Ddi_Bdd_t *b_i, *a_i, *itp_i;
	Ddi_Bdd_t *cube = NULL, *seed = Ddi_BddDup(a);
	Ddi_Varset_t *supp = Ddi_BddSupp(b);
	Ddi_Varset_t *suppC = 
	  Ddi_VarsetMakeFromArray(itpTravMgr->imgPartVars);
	Ddi_Free(itpTravMgr->imgPartVars);
	Ddi_VarsetIntersectAcc(supp,suppC);
	Ddi_Free(suppC);
	if (prevTo!=NULL) {
	  Ddi_BddDiffAcc(seed,prevTo);
	}
	Ddi_BddDiffAcc(seed,itp);
	cube = Ddi_AigSatWithCex(seed);
	Ddi_Free(seed);
	if (cube==NULL) {
	  itpDone = 1;
	  Ddi_Free(supp);
	  Ddi_Free(cube);
	  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
	    printf("\nITP IMG ended [%d]: NO leftover\n", i);
	  }
	  break;
	}
	else {
	  if (Ddi_VarsetIsVoid(supp)) {
	    Ddi_Vararray_t *sA = Ddi_BddSuppVararray(cube);
	    while (Ddi_VararrayNum(sA) > window) {
	      int n = Ddi_VararrayNum(sA);
	      Ddi_VararrayRemove(sA,n-1);
	      Ddi_Free(supp); 
	      supp = Ddi_VarsetMakeFromArray(sA);
	      Ddi_Free(sA);
	    }
	  }
	  Ddi_Bdd_t *myCex = Ddi_BddDup(cube);
	  if (!Ddi_AigSatMinisatWithAbortAndFinal(b,myCex,-1.0,0)) {
	    Ddi_Varset_t *sm = Ddi_BddSupp(myCex);
	    Ddi_VarsetSetArray(sm);
	    Ddi_VarsetIntersectAcc(sm,supp);
	    if (Ddi_VarsetNum(sm)>window/2) {
	      Ddi_DataCopy(cube,myCex);
	    }
	    Ddi_Free(sm);
	  }
	  Ddi_Free(myCex);

	  Ddi_BddExistProjectAcc(cube,supp);
	}
	Ddi_Free(supp);
	b_i = Ddi_BddDup(b); 
	a_i = Ddi_BddDup(a); 
	if (doIte) {
	  Ddi_AigConstrainCubeAcc(b_i,cube);
	  Ddi_AigConstrainCubeAcc(a_i,cube);
	}
	else {
	  Ddi_AigAndCubeAcc(a_i,cube);
	}
	Ddi_BddDiffAcc(leftover,cube);

	Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
	  printf("\nITP IMG partitioned by cube [%d]\n", i);
	  DdiLogBdd(cube,0);
	}

	itp_i = TravItpImgPart (itpTravMgr,kCone,kConeRings,a_i,b_i,prevTo,
			   step,doSplit,globalVars,domainVars,optCare,
                           itpPlus,toPlusCube,NULL,&sat_i,itpPart,itpOdc,timeLimit);
	if (sat_i) {
	  if (psat!=NULL) *psat = sat_i;
	  Ddi_Free(itp);
	  itp = NULL;
	  itpDone = 1;
	}
	else {
	  if (doIte) {
	    Ddi_BddAndAcc(itp_i,cube);
	  }
	  Ddi_BddOrAcc(itp,itp_i);
	}
	Ddi_Free(a_i);
	Ddi_Free(b_i);

	if (itp_i!=NULL) {
	  int j, nv;
	  Ddi_Vararray_t *suppA = Ddi_BddSuppVararray(itp_i);
	  DdiAigVararraySortByFanout(suppA,itp_i);
	  itpTravMgr->imgPartVars = Ddi_VararrayAlloc(ddm, 0);
	  nv = Ddi_VararrayNum(suppA);
#if 1
	  for (j=0; j<nv && j<window; j++)
#else
	  for (j=nv-1; j>=0 && j>=(nv-window); j--)
#endif
	  {
	    Ddi_Var_t *v = Ddi_VararrayRead(suppA,j);
	    Ddi_VararrayInsertLast(itpTravMgr->imgPartVars,v);
	  }
	  //	  Ddi_VararrayPrint(suppA);
	  Ddi_Free(suppA);
	}
	  
	Ddi_Free(itp_i);
	Ddi_Free(cube);
      }
      if (!itpDone) {
	Ddi_Bdd_t *itpLeft=NULL, 
	  *bLeft=NULL, *aLeft = Ddi_BddAnd(a,leftover);
	if (!Ddi_AigSat(aLeft)) {
	  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
	    printf("\nITP IMG partitioned: NO leftover\n");
	  }
	}
	else { 
	  int sat1;
	  Ddi_Vararray_t *save = itpTravMgr->imgPartVars; 
	  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
	    printf("\nITP IMG partitioned on cube leftover\n");
	  }
	  bLeft = Ddi_BddAnd(b,leftover);
	  itpTravMgr->imgPartVars = NULL;
	  itpLeft = TravItpImgPart (itpTravMgr,kCone,kConeRings,aLeft,bLeft,
				prevTo,
				step,doSplit,globalVars,domainVars,optCare,
                                itpPlus,toPlusCube,NULL,&sat1,itpPart,itpOdc,
				timeLimit);
	  itpTravMgr->imgPartVars = save;
	  if (sat1) {
	    if (psat!=NULL) *psat = sat1;
	    itp = NULL;
	  }
	  else {
	    if (doIte) {
	      Ddi_BddAndAcc(itpLeft,leftover);
	    }
	    Ddi_BddOrAcc(itp,itpLeft);
	    if (psat!=NULL) *psat = 0;
	  }
	}
	Ddi_Free(aLeft);
	Ddi_Free(bLeft);
	Ddi_Free(itpLeft);

      }
      return itp;
    }
  }

  int freeA = 0;
  if (itpMgr->sccs.inCoreScc!=NULL) {
    Ddi_Bdd_t *a1 = Ddi_BddDup(a);
    Ddi_Bdd_t *a2 = Ddi_BddDup(a);
    Ddi_Vararray_t *psSmoothA1 = 
      Ddi_VararrayDiff(itpMgr->ps,itpMgr->sccs.inCoreScc);
    Ddi_Varset_t *nsSmooth1 = Ddi_VarsetMakeFromArray(psSmoothA1);
    Ddi_Vararray_t *psSmoothA2 = 
      Ddi_VararrayDup(itpMgr->sccs.inCoreScc);
    Ddi_Varset_t *nsSmooth2 = Ddi_VarsetMakeFromArray(psSmoothA2);
    Ddi_VarsetSubstVarsAcc(nsSmooth1,itpMgr->ps,itpMgr->ns);
    Ddi_VarsetSubstVarsAcc(nsSmooth2,itpMgr->ps,itpMgr->ns);
    Ddi_BddExistAcc(a1,nsSmooth1);
    //    Ddi_BddExistAcc(a2,nsSmooth2);
    int sat1 = Ddi_AigSatAnd(a1,b,optCare); 
    //    int sat2 = Ddi_AigSatAnd(a2,b,optCare); 
    if (!sat1) {
      printf("sound abstraction\n");
      Ddi_DataCopy(a,a1);
    }
    Ddi_Free(nsSmooth1);
    Ddi_Free(psSmoothA1);
    Ddi_Free(nsSmooth2);
    Ddi_Free(psSmoothA2);
    Ddi_Free(a1);
    Ddi_Free(a2);
    freeA = 1;
  }

  if (checkStall) {
    int i;
    Ddi_Bdd_t *myA = Ddi_BddDup(a); 
    Ddi_Bddarray_t *psLits = Ddi_BddarrayMakeLiteralsAig(itpMgr->ps, 1);
    Ddi_Bdd_t *myEq = Ddi_BddMakeEq(itpMgr->ns, psLits);
    if (prevTo!=NULL) Ddi_BddDiffAcc(myA,prevTo);
    prevTo = NULL;
    printf("CHECKING STALLED latches\n");
    Ddi_Bdd_t *eq =
      Ddi_AigInductiveImgPlus(myA, myEq, optCare,
			      NULL, 1 /*doInductiveToPlus*/);
    if (eq != NULL) {
      Ddi_Free(eq);
    }
    Ddi_Free(myEq);
    Ddi_Free(myA);
  }

  if (1 && prevTo!=NULL && itpPart<=8) {

    return itpImgWithPrevTo (itpTravMgr,kConeRings,a,b,prevTo,optCare,itpPlus,partWindow,
                             globalVars,domainVars,step,psat,
                             itpPart,itpOdc,timeLimit);
  }

  if (Ddi_BddIsPartDisj(b) && Ddi_BddPartNum(b)>1) {
    Ddi_Bdd_t *b0, *b1, *bPart = Ddi_BddDup(b); 
    int m = Ddi_BddReadMark(b);

    Ddi_BddPartSortBySizeAcc(bPart,0); // decreasing size
    int largeFirst=0;
    int np = Ddi_BddPartNum(bPart);
    int n0 = np/3;
    b0 = Ddi_BddPartExtract(bPart,0);
    for (int i=1; i<n0; i++) {
      Ddi_Bdd_t *p = Ddi_BddPartExtract(bPart,0);
      Ddi_BddOrAcc(b0,p);
      Ddi_Free(p);
    }
    b1 = Ddi_BddDup(bPart);
    if (largeFirst) {
      Ddi_Bdd_t *t=b0; b0=b1; b1=t;
    }

    Ddi_BddSetAig(b0);
    Ddi_BddSetAig(b1);
    //    Ddi_BddDiffAcc(b0,b1);
    Ddi_AigStructRedRemAcc (b0,NULL);

    Ddi_Free(bPart);
    Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
      printf("\nITP by disj CONE decomp: %d -> (%d,%d)\n",
	     Ddi_BddSize(b), Ddi_BddSize(b0), Ddi_BddSize(b1));
    }
    static int trySingle = 0;
    Ddi_BddWriteMark(b0,m);
    Ddi_BddWriteMark(b1,m);

    if (trySingle) {
      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
	printf("ITP by disj CONE decomp: try single\n");
      }
      Ddi_Bdd_t * tmpItp = Ddi_AigSat22AndWithInterpolant(NULL,a,b,NULL,
					  globalVars, domainVars,
					  tfPiVars,tfPiNum,
					  optCare,prevTo,
					  psat, 0, itpOdc, 
					  0,timeLimit);
      Ddi_Free(tmpItp);
      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
	printf("ITP by disj CONE decomp: now partitioned\n");
      }
    }
    Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
      printf("\nITP by disj CONE decomp part 0: %d\n",
	     Ddi_BddSize(b1));
    }
    Ddi_Bdd_t *itp0, *itp1 = 
       Ddi_AigSat22AndWithInterpolant(NULL,a,b1,NULL,
					  globalVars, domainVars,
					  tfPiVars,tfPiNum,
					  optCare,prevTo,
					  psat, 0, itpOdc, 
					  0,timeLimit);
    if (itp1==NULL) {
      Ddi_Free(b0);
      Ddi_Free(b1);
      return NULL;
    }
    else {
      int refineByItp=1;
      if (refineByItp) {
        printf("\nRe-Computing itp using itp1\n");
        Ddi_Bdd_t *coneAux = Ddi_BddNot(itp1);
        Ddi_Bdd_t *itp1b = Ddi_AigSat22AndWithInterpolant(NULL,a,coneAux,NULL,
					  globalVars, domainVars,
					   tfPiVars,tfPiNum,
					   NULL,NULL,
					   psat, 0, itpOdc, 
					   0,timeLimit);
	Ddi_DataCopy(itp1,itp1b);
	Ddi_Free(itp1b);
      }
      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
        printf("\nITP by disj CONE decomp part 1: %d\n",
               Ddi_BddSize(b0));
      }
      Ddi_BddAndAcc(b0,itp1);
      Ddi_Bdd_t *myCare = Ddi_BddDup(itp1);
      if (optCare!=NULL) {
        Ddi_BddAndAcc(myCare,optCare);
      }
      if (itpPlus!=NULL) {
        Ddi_BddAndAcc(myCare,itpPlus);
      }
      Ddi_BddAndAcc(b0,myCare);
      itp0 = 
       Ddi_AigSat22AndWithInterpolant(NULL,a,b0,NULL,
					  globalVars, domainVars,
					  tfPiVars,tfPiNum,
					  myCare,NULL,
					  psat, 0, itpOdc, 
					  0,timeLimit);
      Ddi_Free(b0);
      Ddi_Free(b1);
      if (itp0==NULL) {
        Ddi_Free(itp1);
	return NULL;
      }
      else {
        Ddi_BddAndAcc(itp0,myCare);
        printf("\nRe-Computing itp using itp0\n");
        Ddi_Bdd_t *coneAux = Ddi_BddNot(itp0);
        Ddi_Bdd_t *itpRet = Ddi_AigSat22AndWithInterpolant(NULL,a,coneAux,NULL,
					  globalVars, domainVars,
					   tfPiVars,tfPiNum,
					   NULL,NULL,
					   psat, 0, itpOdc, 
					   0,timeLimit);
        
        int s0 = Ddi_BddSize(itp0), s1 = Ddi_BddSize(itp1);
	Ddi_BddAndAcc(itp1,itp0);
	Ddi_Free(itp0);
        if (Ddi_BddSize(itpRet)<Ddi_BddSize(itp1)) {
          Ddi_DataCopy(itp1,itpRet);
          Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
            printf("ITP by disj CONE decomp result: ITP(%d,%d)->%d\n",
                 s0,s1,Ddi_BddSize(itp1));
          }
        }
        else {
          Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
            printf("ITP by disj CONE decomp result: %d+%d=%d\n",
                 s0,s1,Ddi_BddSize(itp1));
          }
        }
	return itp1;
      }
      Ddi_Free(myCare);
    }
  }
  else if (/*step<=1 ||*/ itpPart<=1 || Ddi_BddSize(b)<=itpPartTh) {
    Ddi_Bdd_t *b2 = Ddi_BddDup(b);
    //Ddi_Bdd_t *b2 = Ddi_BddMakeAig(b);
    int saveItpReverse = ddm->settings.aig.itpReverse;
    if (0 && step<=2 && ddm->settings.aig.itpReverse)
      ddm->settings.aig.itpReverse = -1;
#define REVERSE 0
#if REVERSE
    int saveItpNnfAbstrAB = ddm->settings.aig.itpNnfAbstrAB;
    // set just B
    ddm->settings.aig.itpNnfAbstrAB = 3;
    Ddi_Bdd_t *myB2 = Ddi_BddDup(b2);
    if (optCare!=NULL) {
      Ddi_BddNotAcc(optCare);
      Ddi_BddOrAcc(myB2,optCare);
      Ddi_BddNotAcc(optCare);
    }
    Ddi_Bdd_t *itp = Ddi_AigSat22AndWithInterpolant(NULL,myB2,a,NULL,
					  globalVars, domainVars,
					  tfPiVars,tfPiNum,
					  NULL,NULL,
					  psat, 0, itpOdc, 
                                          0,timeLimit);
    Ddi_Free(myB2);
    ddm->settings.aig.itpNnfAbstrAB = saveItpNnfAbstrAB;
    if (itp!=NULL) {
      Ddi_BddNotAcc(itp);
    }
#else
    Ddi_Bdd_t *itp = NULL;
    static int tryThis = 0;
    if (tryThis) {
      Ddi_Bdd_t *b2b = Ddi_BddDup(b2); 
      Ddi_Varset_t *sm = Ddi_VarsetVoid(ddm);
      char *base[] = {"i_d[0]","i_d[1]","i_d[2]","i_d[3]",
                      "i_d[4]","i_d[5]","i_d[6]","i_d[7]",
                      "i_reset","i_v","i_clk","i_ce",
                      "i26","i28","i30","i32","i34",
                      "i36","i38","i40","i42","i44",
                      NULL};
      char name[100];
      int tfMax = 6;
      Ddi_VarsetSetArray(sm);
      for (int i = 0; base[i]!=NULL; i++) {
        // sprintf(name,"i_d[%d]", i);
        // Ddi_Var_t *v = Ddi_VarFromName(ddm,name);
        //        Ddi_VarsetAddAcc(sm,v);
        for (int j=tfMax; j>=0; j--) {
          sprintf(name,"PDT_TF_VAR_%s_%d", base[i],j);
          Ddi_Var_t *v = Ddi_VarFromName(ddm,name);
          if (v!=NULL)
            Ddi_VarsetAddAcc(sm,v);
        }
      }
      Ddi_BddExistAcc(b2b,sm);
      Ddi_Bdd_t *itp0 = Ddi_AigSat22AndWithInterpolant(NULL,a,b2b,NULL,
					  globalVars, domainVars,
					  tfPiVars,tfPiNum,
					  optCare,prevTo,
					  psat, 0, itpOdc, 
					  0,timeLimit);
      itp = Ddi_BddDup(itp0);
      Ddi_Free(itp0);
      Ddi_Free(b2b);
      Ddi_Free(sm);
    }
    if (itp==NULL) {
      int doCompute = 1;
      int iteOptTh = ddm->settings.aig.itpIteOptTh;
      int tryTwice = ddm->settings.aig.itpTwice;
      ddm->settings.aig.itpTwice = 0;
      if (tryTwice != 0) {
        //        ddm->settings.aig.itpNoQuantify = 1000;
        ddm->settings.aig.itpIteOptTh = 5000000;
        itp = Ddi_AigSat22AndWithInterpolant(NULL,a,b2,NULL,
					  globalVars, domainVars,
					  tfPiVars,tfPiNum,
					  optCare,prevTo,
					  psat, 0, itpOdc, 
					  0,timeLimit);
        //        ddm->settings.aig.itpNoQuantify = 0;
        ddm->settings.aig.itpIteOptTh = iteOptTh;
        if (itp != NULL) {
          Ddi_BddNotAcc(itp);
          //          Ddi_Free(b2);
          // b2 = Ddi_BddNot(itp);
          Ddi_BddOrAcc(b2,itp);
          Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
            printf("\nRecomputing itp: original size: %d\n",
                   Ddi_BddSize(itp));
          }
          Ddi_Free(itp);
        }
        else doCompute = 0;
      }
      if (doCompute) {
        if (ddm->aig.actVars!=NULL) {
          Ddi_Bdd_t *itp0=NULL; 
          Ddi_Bdd_t *b2Constr;
          Ddi_Bdd_t *constr =
            Ddi_BddMakePartConjFromArray(ddm->aig.actVars);
          int nv = Ddi_BddarrayNum(ddm->aig.actVars);
          Ddi_Unlock(ddm->aig.actVars);
          Ddi_Free(ddm->aig.actVars);
          b2Constr = Ddi_BddDup(constr);
          Ddi_BddPartInsertLast(b2Constr,b2);
          Ddi_BddSetFlattened(b2Constr);
          Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
            printf("\nComputing itp on %d Act Vars hint\n", nv);
          }
          Ddi_BddSetAig(b2Constr);
          int genActLits = ddm->settings.aig.itpActiveVars;
          ddm->settings.aig.itpActiveVars=0; // disable
          itp0 = Ddi_AigSat22AndWithInterpolant(NULL,a,b2Constr,NULL,
					  globalVars, domainVars,
					  tfPiVars,tfPiNum,
					  optCare,prevTo,
					  psat, 0, itpOdc, 
					  0,timeLimit);
          ddm->settings.aig.itpActiveVars=genActLits;
          Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
            printf("\nNow Computing itp leftover for %d hints\n",
                   nv);
          }
          if (itp0!=NULL) {
            Ddi_BddSetAig(constr);
            Ddi_BddDiffAcc(b2,constr);
            Ddi_BddNotAcc(itp0);
            Ddi_BddOrAcc(b2,itp0);
          }
          Ddi_Free(itp0);
          Ddi_Free(b2Constr);
          Ddi_Free(constr);
        }

        float clungItpRatio = travMgr->settings.aig.igrClungItpRatio;
        int clungItpTh = travMgr->settings.aig.igrClungItpTh;
        Ddi_Bdd_t *clungItp=NULL;
        if (clungItpRatio>0.0 && Ddi_BddSize(b2)>clungItpTh) {
          clungItp = Ddi_BddMakeConstAig(ddm,0);
          Ddi_Bdd_t *partb2 = Ddi_AigPartitionTop(b2,0);
          Ddi_DataCopy(b2,partb2);
          Ddi_Free(partb2);
          Pdtutil_Assert(itpTravMgr->stats.coneSizeNoCare>0,"missing cone size stat");
          float scaleStep = (1-clungItpRatio)/Ddi_BddSize(b2);
          scaleStep *= itpTravMgr->stats.coneSizeNoCare;
          clungItpRatio = 1.0 - scaleStep;
          Pdtutil_Assert(clungItpRatio <=1.01,"wrong ratio");
        }
        else clungItpRatio = -1.0; // enforce disable
        itp = Ddi_AigSat22AndWithInterpolantAndClung(NULL,a,b2,NULL,
					  globalVars, domainVars,
					  tfPiVars,tfPiNum,
					  optCare,prevTo,
                                          NULL,
                                          clungItp,clungItpRatio,
                                          psat, itpPart, itpOdc, 
					  0,timeLimit);
        if (itp!=NULL && clungItp!=NULL  && Ddi_BddSize(clungItp)<2000000) {
          Ddi_Free(itpTravMgr->careForBwdCone);
          itpTravMgr->careForBwdCone = Ddi_BddNot(clungItp);
        }
        Ddi_Free(clungItp);
      }
      ddm->settings.aig.itpTwice = tryTwice;
    }
#endif
    Ddi_Free(b2);
    ddm->settings.aig.itpReverse = saveItpReverse;
    return itp;
  }
  else if (0 && itpPart<=4 && doSplit==0) {
    Ddi_Bdd_t *bPart = Ddi_BddDup(b);        
    Ddi_Bdd_t *bDup = Ddi_BddDup(b);        
    Ddi_Bdd_t *iPart = NULL, *itp=NULL,
      *myCare=Ddi_BddMakeConstAig(ddm,1);
    if (optCare!=NULL) {
      Ddi_BddAndAcc(myCare,optCare);
    }
    if (Ddi_BddPartNum(bPart)==2) {
      Ddi_BddPartSortBySizeAcc(bPart,1); // increasing size
      Ddi_Bdd_t *b0 = Ddi_BddPartRead(bPart,0);
      Ddi_Bdd_t *b1 = Ddi_BddPartRead(bPart,1);
      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
        printf("ITP by disj CONE decomp: %d -> (%d,%d)\n",
	     Ddi_BddSize(b), Ddi_BddSize(b0), Ddi_BddSize(b1));
      }
      Ddi_Free(bDup);
      bDup = Ddi_BddDup(Ddi_BddPartRead(bPart,1));
      iPart = Ddi_AigSat22AndWithInterpolant(NULL,a,b0,NULL,
					  globalVars, domainVars,
					  tfPiVars,tfPiNum,
					  optCare,itpPlus,
					  psat, 0, itpOdc, 
					  0,timeLimit);
      if (iPart!=NULL) {
        Ddi_BddNotAcc(iPart);
        Ddi_BddOrAcc(bDup,iPart);
      }
      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
        printf("Computing main ITP\n",
               Ddi_BddSize(b), Ddi_BddSize(b0), Ddi_BddSize(b1));
      }
    }
    Ddi_Free(bPart);
    itp = Ddi_AigSat22AndWithInterpolant(itpTravMgr->incrSat,
                                         a,bDup,NULL,
                                         globalVars, domainVars,
                                         tfPiVars,tfPiNum,
                                         myCare,itpPlus,
                                         psat, itpPart, itpOdc, 
                                         0,timeLimit);
    if (0 && itp!=NULL && iPart!=NULL) {
      Ddi_BddAndAcc(itp,iPart);
    }
    Ddi_Free(myCare);
    Ddi_Free(bDup);
    Ddi_Free(iPart);
    return itp;
  }
  else if (enDisjDecomp) {
    Ddi_Bdd_t *b0, *b1, *bPart = Ddi_AigPartitionTop(b,1); 
    int m = Ddi_BddReadMark(b);
    if (Ddi_BddPartNum(bPart) == 1) {
      Ddi_Bdd_t *conjPart, *cPart0, *cPart00;
      Ddi_Free(bPart);
      conjPart = Ddi_AigPartitionTop(b,0); 
      Ddi_BddPartSortBySizeAcc(conjPart,0); // decreasing size
      cPart0 = Ddi_BddPartRead(conjPart,0);
      bPart = Ddi_AigPartitionTop(cPart0,1); 
      Ddi_BddPartSortBySizeAcc(bPart,0); // decreasing size
      b0 = Ddi_BddDup(conjPart);
      b1 = Ddi_BddDup(conjPart);
      Ddi_Free(conjPart);
      cPart00 = Ddi_BddPartExtract(bPart,0);
      if (Ddi_BddPartNum(bPart)>10) {
	Ddi_Bdd_t *p1 = Ddi_BddPartExtract(bPart,0);
	Ddi_BddOrAcc(cPart00,p1);
	Ddi_Free(p1);
      }
      Ddi_BddSetAig(bPart);
      Ddi_BddPartWrite(b0,0,cPart00);
      Ddi_BddPartWrite(b1,0,bPart);
      Ddi_Free(cPart00);
      Ddi_Free(bPart);
      Ddi_BddSetAig(b0);
      Ddi_BddSetAig(b1);
      Ddi_BddDiffAcc(b0,b1);
      Ddi_AigStructRedRemAcc (b0,NULL);
    }
    else {
      Ddi_BddPartSortBySizeAcc(bPart,0); // decreasing size
      b0 = Ddi_BddPartExtract(bPart,0);
      b1 = Ddi_BddDup(bPart);
      Ddi_BddSetAig(b0);
      Ddi_BddSetAig(b1);
      Ddi_BddDiffAcc(b0,b1);
      Ddi_AigStructRedRemAcc (b0,NULL);
    }
    Ddi_Free(bPart);
    Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
      printf("ITP by disj CONE decomp: %d -> (%d,%d)\n",
	     Ddi_BddSize(b), Ddi_BddSize(b0), Ddi_BddSize(b1));
    }
    static int trySingle = 0;
    Ddi_BddWriteMark(b0,m);
    Ddi_BddWriteMark(b1,m);

    if (trySingle) {
      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
	printf("ITP by disj CONE decomp: try single\n");
      }
      Ddi_Bdd_t * tmpItp = Ddi_AigSat22AndWithInterpolant(NULL,a,b,NULL,
					  globalVars, domainVars,
					  tfPiVars,tfPiNum,
					  optCare,itpPlus,
					  psat, 0, itpOdc, 
					  0,timeLimit);
      Ddi_Free(tmpItp);
      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
	printf("ITP by disj CONE decomp: now partitioned\n");
      }
    }
    Ddi_Bdd_t *itp0, *itp1 = 
       Ddi_AigSat22AndWithInterpolant(NULL,a,b1,NULL,
					  globalVars, domainVars,
					  tfPiVars,tfPiNum,
					  optCare,NULL,
					  psat, 1, itpOdc, 
					  0,timeLimit);
    if (itp1==NULL) {
      Ddi_Free(b0);
      Ddi_Free(b1);
      return NULL;
    }
    else {
      Ddi_BddAndAcc(b0,itp1);
      itp0 = 
       Ddi_AigSat22AndWithInterpolant(NULL,a,b0,NULL,
					  globalVars, domainVars,
					  tfPiVars,tfPiNum,
					  optCare,NULL,
					  psat, 1, itpOdc, 
					  0,timeLimit);
      Ddi_Free(b0);
      Ddi_Free(b1);
      if (itp0==NULL) {
        Ddi_Free(itp1);
	return NULL;
      }
      else {
	Ddi_BddAndAcc(itp1,itp0);
	Ddi_Free(itp0);
	return itp1;
      }
    }
  }
  else if (enPartA) {
    Ddi_Bdd_t *itpRes = Ddi_AigSat22AndWithInterpolantPartNnfA(NULL,a,b,
					  itpMgr->bckReachedRings,prevTo,
					  globalVars, domainVars,
					  itpMgr->ns, 
					  tfPiVars,tfPiNum,
					  optCare,NULL,
					  psat, itpPart, itpOdc, 
					  timeLimit);
    if (itpRes!=NULL) {
      // strengthen bwd ring
      TravItpStrengthenBwdRing(itpMgr,b);
    }
    return itpRes;
  }

  // 1: just split - 2: split and cut preimg
  int saveSplitConeInfo = 1 && Ddi_BddReadComposeF(b)!=NULL;
  int useCareForA = 1;
  
  itpTot = Ddi_BddMakeConstAig(ddm,1);
  careTot = Ddi_BddMakeConstAig(ddm,1);
  careTotSplit = Ddi_BddMakeConstAig(ddm,1);
  careTotForCex = Ddi_BddMakeConstAig(ddm,1);
  careFinal = Ddi_BddMakeConstAig(ddm,1);
  bDup = Ddi_BddDup(b);

  itpPlus = NULL;

  if (Ddi_BddReadComposeF(b)!=NULL) {
    splitB = Ddi_BddDup(Ddi_BddReadComposeF(b));
    splitCare = Ddi_BddDup(Ddi_BddReadComposeCare(b));
    splitConstr = Ddi_BddDup(Ddi_BddReadComposeConstr(b));
    splitU = Ddi_BddarrayDup(Ddi_BddReadComposeSubst(b));
    splitV = Ddi_VararrayDup(Ddi_BddReadComposeVars(b));
    splitRefV = Ddi_BddReadComposeRefVars(b);
    splitRel = Ddi_BddRelMakeFromArray(splitU,splitV);
    splitVars = Ddi_VarsetMakeFromArray(splitV);

    if (splitConstr!=NULL) {
      int m = Ddi_BddReadMark(splitConstr);
      Ddi_BddAndAcc(splitRel,splitConstr);
      if (m>0) {
	andWithRing_i = m;
      }
    }

    Ddi_BddSetAig(splitRel);

    splitV2 =
      Ddi_VararrayMakeNewVars(itpMgr->ps, "PDT_ITP_SPLITV_", NULL, 1);
    splitVaux =
      Ddi_VararrayMakeNewVars(itpMgr->ps, "PDT_ITP_SPLITV_AUX_", NULL, 1);
    splitVaux2 =
      Ddi_VararrayMakeNewVars(itpMgr->ps, "PDT_ITP_SPLITV_AUX2_", NULL, 1);
    piAux =
      Ddi_VararrayMakeNewVars(itpMgr->pi, "PDT_ITP_PI_AUX_", NULL, 1);
    if (doSplit2>2) {
      int splitCare_i = step+doSplit2;
      int start_i = step+doSplit;
      int coneTopMark = Ddi_BddReadMark(splitB);
      doItpFromGenClausesAux = 0;
      splitRel2 = TravGrowUnrollRelation(itpMgr, itpMgr->ns, 
	            start_i - 1, splitCare_i,
		    itpMgr->delta, itpMgr->initStub, 
		    useRingConstr, andWithRing_i, 0, coneTopMark);
      Ddi_BddSubstVarsAcc(splitRel2, itpMgr->ns, splitV2);
      Ddi_BddSubstVarsAcc(splitRel2, itpMgr->ps, itpMgr->ns);
      if (!Ddi_AigSat(splitRel2)) {
	Ddi_Free(splitRel2);
      }
    }
    else {
      doSplitCare=0;
    }
  }

  if (mark>0 && mark < tfPiNum) {
    tfPiNum = mark;
  }

  Pdtutil_Assert(itpPart>0,"itpPart required");

  if (saveSplitConeInfo &&
      (itpTravMgr->splitConeInfo.preImg!=NULL
       || itpTravMgr->splitConeInfo.ring!=NULL)) {
    if (saveSplitConeInfo>1) {
      Ddi_Free(careTotForCex);
      careTotForCex = Ddi_BddNot(itpTravMgr->splitConeInfo.preImg);
      Ddi_BddSubstVarsAcc(careTotForCex,splitRefV,splitV);
    }
    if (useCareForA) {
      careTotForA = Ddi_BddDup(itpTravMgr->splitConeInfo.ring);
      Ddi_BddSubstVarsAcc(careTotForA,splitRefV,splitV);
    }
  }
  
  if (optCare!=NULL) {
    Ddi_BddAndAcc(careTot,optCare);
    //    Ddi_BddAndAcc(careTotForCex,optCare);
  }

#define EXTRACT_MAXP 1

  if (incrementalSat) {
    ddiS = Ddi_IncrSatMgrAlloc(ddm, 1, 1, 0);
#if 0
    Ddi_Var_t *a = Ddi_VarNewBaig(ddm,"A");
    Ddi_Var_t *b = Ddi_VarNewBaig(ddm,"B");
    Ddi_Var_t *c = Ddi_VarNewBaig(ddm,"C");
    Ddi_Var_t *d = Ddi_VarNewBaig(ddm,"D");
    Ddi_Bdd_t *la = Ddi_BddMakeLiteralAig(a, 1);
    Ddi_Bdd_t *lb = Ddi_BddMakeLiteralAig(b, 1);
    Ddi_Bdd_t *lc = Ddi_BddMakeLiteralAig(c, 1);
    Ddi_Bdd_t *ld = Ddi_BddMakeLiteralAig(d, 1);

    Ddi_Bdd_t *p0 = Ddi_BddAnd(la,lb);
    Ddi_Bdd_t *p1 = Ddi_BddOr(p0,lc);

    Ddi_IncrSatMgrResume(ddiS);
    Ddi_Bdd_t *cex0 = Ddi_AigSatMinisat22WithCexAndAbortIncremental(ddiS,
		  p0, NULL, 0, -1, NULL);
    Ddi_IncrSatMgrSuspend(ddiS);
    int x = Ddi_AigSatAnd(la,lb,lc);
    Ddi_IncrSatMgrResume(ddiS);
    Ddi_Bdd_t *cex1 = Ddi_AigSatMinisat22WithCexAndAbortIncremental(ddiS,
		  p1, NULL, 0, -1, NULL);
    Ddi_IncrSatMgrSuspend(ddiS);
    Ddi_Free(p0);
    Ddi_Free(p1);
    Ddi_Free(cex0);
    Ddi_Free(cex1);
    Ddi_Free(a);
    Ddi_Free(b);
    Ddi_Free(c);
#endif
  }

  int itpEnded=0;
  for (i=0; i<itpPart && !itpEnded; i++) {
    Ddi_Bdd_t *myB, *myBforCex, *cex=NULL, *toMinus = NULL;
    int tryLowerBoundk = enLowerBoundk && i%4==0;
    int split_i = step+doSplit;
    int d;
    int tryOK = 0;
    if (tryLowerBoundk) {
      d = dLB--;
      if (d<=1 || fullK-split_i <= 2 ||
	  Ddi_BddPartNum(kConeRings)<fullK) {
	tryLowerBoundk = enLowerBoundk = 0; // disable it
      }
      if (fullK-d-1<=split_i) tryLowerBoundk=0;
    }

    myB = Ddi_BddDup(bDup);
    myBforCex = Ddi_BddDup(bDup);

    Ddi_BddAndAcc(myB,careFinal);
    if (splitB!=NULL) {
      Ddi_Free(myBforCex);
      myBforCex = Ddi_BddDup(splitB);
      if (1&&(splitCare!=NULL)) {
	Ddi_BddAndAcc(careTotForCex,splitCare);
      }
      itpPlus = NULL;
#if 0
      Ddi_BddNotAcc(careTotSplit);
      Ddi_BddOrAcc(myBforCex,careTotSplit);
      Ddi_BddNotAcc(careTotSplit);
#endif
    }
    if (i>=0 && preSplit && splitRel) {
      //      genClauses=1; 
      if (enFwdCare) genCubes=1;
    }

    nnfSubset = 0 && (i>=1) && (i<itpPart-1);

    Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
      printf("ITP part%s: %d/%d\n", splitB?"(split)":"", i, itpPart);
    }
    if (i==itpPart) {
      //      handle final part
      itp_i =
	Ddi_AigSat22AndWithInterpolant(NULL,a,myB,NULL,
				       globalVars, domainVars,NULL,0,
				       itpTot,NULL,
				       psat, 0, itpOdc, 0, timeLimit);
    }
    else {
      Ddi_Bdd_t *itpSplit = NULL; int itpSplitDone=0;
      startTime = util_cpu_time();
      int forceSplit = 1, checkPrev=1;
      if ((forceSplit||saveSplitConeInfo) && splitRel!=NULL && i==itpPart-1) {
	/* let as it is */
        Pdtutil_Assert (preSplit && splitRel!=NULL,
                        "Wrong split cone setup");
        Ddi_BddAndAcc(myBforCex,careTotForCex);
        if (checkPrev &&
            !Ddi_BddIsOne(careTotForCex)&&!Ddi_AigSat(myBforCex)) {
          itpSplit = Ddi_BddDup(careTotForCex);
          itpSplitDone = 1;
          Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
            fprintf(dMgrO(ddm),
                    "REUSING PREVIOUS SPLIT ITP BWD RING: %d\n",
                    Ddi_BddSize(itpSplit));
          }
        }
      }
      else if (i==itpPart-1) {
	/* let as it is */
	if (preSplit && splitRel!=NULL) {
	  Ddi_Free(myB);
	  myB = Ddi_BddDup(myBforCex);
	  Ddi_BddAndAcc(myB,careTotForCex);
	  Ddi_BddComposeAcc(myB,splitV,splitU);
	  if (splitConstr!=NULL) {
	    Ddi_BddAndAcc(myB,splitConstr);
	  }
	  preSplit = 0;
	}
	Ddi_Bdd_t *bFromItp = Ddi_BddNot(itpTot);
	Ddi_BddOrAcc(myB,bFromItp);
	Ddi_Free(bFromItp);
        int tryGenCof = 0;
        itp_i = NULL;
        if (tryGenCof) {
          if (!Ddi_AigSatConstrain(myB,
                                   itpTot, -1.0, NULL)) {
            itp_i = Ddi_BddDup(itpTot);
          }
        }
        if (itp_i==NULL) 
          itp_i =
	    Ddi_AigSat22AndWithInterpolant(NULL,a,myB,NULL,
				       globalVars, domainVars,NULL,0,
				       itpTot,NULL,
				       psat, 0, itpOdc, 0, timeLimit);
	doSplitCare=0; itpEnded = 1;
      }
      else if (tryLowerBoundk) {
	Ddi_Bdd_t *target = Ddi_BddDup(Ddi_BddPartRead(kConeRings, 
						       fullK-1));
	Ddi_Bddarray_t *splitU3 = NULL;
	Ddi_Vararray_t *splitV3 = NULL;
	Ddi_Varset_t *splitVars3 = NULL;
	Ddi_Bdd_t *splitConstr3 = NULL;
	Ddi_Bdd_t *myA = NULL, *myBl = NULL;
	Ddi_Vararray_t *v1=NULL,*v2=NULL, *glbA=NULL;
	TravGrowConeBwdDecomp(itpMgr, target, fullK-d-1, step, split_i,  
			  1, 1, NULL,
			  itpMgr->initStub, useRingConstr, andWithRing_i, 0);	
	if (itpTravMgr->constrainVars != NULL && 
	    Ddi_VararrayNum(itpTravMgr->constrainVars)>0) {
	  Ddi_BddComposeAcc(target, itpTravMgr->constrainVars,
			    itpTravMgr->constrainSubstLits);
	}
	if (toPlusCube!=NULL && !Ddi_BddIsOne(toPlusCube)) {
	  Ddi_AigConstrainCubeAcc(target,toPlusCube);
	}
	
	if (Ddi_AigSat(target)) {	    
	  myBl = Ddi_BddDup(Ddi_BddReadComposeF(target));
	  splitU3 = Ddi_BddarrayDup(Ddi_BddReadComposeSubst(target));
	  splitV3 = Ddi_VararrayDup(Ddi_BddReadComposeVars(target));
	  myA =  Ddi_BddRelMakeFromArray(splitU3,splitV3);
	  splitConstr3 = Ddi_BddReadComposeConstr(target);
	  if (splitConstr3!=NULL) {
	    Ddi_BddAndAcc(myA, splitConstr);
	  }
	  splitVars3 = Ddi_VarsetMakeFromArray(splitV3);
	  Ddi_BddSetAig(myA);
	  Ddi_BddAndAcc(myA, a);
	  
	  itpSplit =
	    Ddi_AigSat22AndWithInterpolant(NULL,myA,myBl,NULL,
					   splitVars, NULL,NULL,0,
					   NULL,NULL,
					   psat, 0, itpOdc, 1, timeLimit);
	  itpSplitDone=1;
	  if (!Ddi_BddIsOne(itpSplit)) {
	    tryOK = 1;
	    if (genNextRing && fullK-d-1 > split_i+1) {
	      Ddi_Bddarray_t *d1;
	      Ddi_Bdd_t *d1Rel;
	      Ddi_Bdd_t *tNext = Ddi_BddDup(Ddi_BddPartRead(kConeRings, 
						       fullK-1));
	      TravGrowConeBwd(itpMgr, tNext, fullK-d-1, split_i+1, 
			  itpMgr->delta, itpMgr->initStub, 
			  useRingConstr, andWithRing_i, 0);

	      d1 = Ddi_BddarrayDup(itpMgr->delta);
	      Ddi_BddarraySubstVarsAcc(d1,itpMgr->ps,itpMgr->ns);

	      if (Ddi_BddarrayNum(itpMgr->eqRings) > split_i) {
		Ddi_Bdd_t *eqConstr = Ddi_BddarrayRead(itpMgr->eqRings, split_i);
		if (eqConstr != NULL) {
		  Ddi_Vararray_t *vars = Ddi_BddReadEqVars(eqConstr);
		  Ddi_Bddarray_t *subst = Ddi_BddReadEqSubst(eqConstr);
		  
		  Ddi_BddarrayComposeAcc(d1, vars, subst);
		}
	      }
	      
	      Ddi_BddarraySubstVarsAcc(d1,itpMgr->ns,splitV2);
	      Ddi_BddarraySubstVarsAcc(d1,itpMgr->pi,piAux);
	      d1Rel = Ddi_BddRelMakeFromArray(d1,itpMgr->ns);
	      Ddi_BddPartInsertLast(d1Rel,itpSplit);
	      itpNextRing =
		Ddi_AigSat22AndWithInterpolant(NULL,d1Rel,tNext,NULL,
					   itpMgr->nsvars, NULL,NULL,0,
					   NULL,NULL,
					   psat, 0, itpOdc, 1, timeLimit);
	      Pdtutil_Assert(itpNextRing!=NULL,"NULL itp");
	      genNextRing = 0;
	      Ddi_Free(d1);
	      Ddi_Free(d1Rel);
	      Ddi_Free(tNext);
	    }
	  }
	  else {
	    Ddi_Free(itpSplit);
	  }
	}
	else if (itpNextRing == NULL) {
	  if (itpMgr->fromRings!=NULL && 
	      Ddi_BddarrayNum(itpMgr->fromRings)>split_i+1) {
	    itpNextRing = 
	      Ddi_BddDup(Ddi_BddarrayRead(itpMgr->fromRings, split_i+1));
	    genNextRing = 0;
	  }

	}
	Ddi_Free(myA);
	Ddi_Free(myBl);
	Ddi_Free(splitV3);
	Ddi_Free(splitVars3);
	Ddi_Free(splitU3);
	Ddi_Free(target);

	if (!tryOK) {
	  Ddi_Free(myB);
	  Ddi_Free(myBforCex);
	  continue;
	}
      }
      else if (nnfSubset) {
	Ddi_Bdd_t *myBSubs = Ddi_AigSatNnfSubset(myBforCex,
						 careTotForCex,itpPlus);
	if (myBSubs == NULL) {
	  Ddi_Free(myB);
	  Ddi_Free(myBforCex);
	  break;
	}
	Ddi_Free(myBforCex);
	myBforCex = myBSubs;
      }
      else if (genClauses) {
	int maxIter = 10; // i==0? 10 : 20;
	int res=0;
	Pdtutil_Assert (preSplit && splitRel!=NULL,"wrong setting");
	Ddi_Bdd_t *aSplit = Ddi_BddAnd(a,splitRel);
	Ddi_Bdd_t *aCare = Ddi_BddAnd(careTot,splitRel);
	Ddi_Bdd_t *aCare2 = NULL;
	Ddi_Vararray_t *v1=NULL,*v2=NULL, *glbA=NULL;
	
	if (splitRel2!=NULL) {
	  Ddi_Bdd_t *aCare2aux = Ddi_BddAnd(careTotSplit,splitRel2);
	  aCare2 = Ddi_AigPartitionTop(aCare2aux,0);
	  Ddi_Free(aCare2aux);
	}
	else {
	  aCare2 = Ddi_AigPartitionTop(aCare,0);
	}
	v1 = Ddi_VararrayMakeFromVarset(splitVars,1);
	v2 =
	  Ddi_VararrayMakeNewAigVars(v1, "shadow", NULL);
	glbA = Ddi_VararrayDup(v1);
	if (genCubes && enFwdCare) {
	  Ddi_Bdd_t *careFwd = NULL, *myToMinus=NULL;
	  if (0 && itpMgr->fromRings!=NULL && 
	      step>1 &&
	      Ddi_BddarrayNum(itpMgr->fromRings)>=step) {
	    myToMinus = Ddi_BddarrayRead(itpMgr->fromRings, 
						  step-1);
	    careFwd = Ddi_BddDiff(splitRel,myToMinus);
	  }
	  //	  enFwdCare = 0;
	  itpSplit =
	    Ddi_AigInterpolantByGenClauses(aSplit,myBforCex,careTotForCex,
					   careFwd, 
					   v2, v1, NULL, glbA, 
					   NULL, NULL, NULL, maxIter, 
					   0, &res);
	  Ddi_Free(careFwd);
	  if (res<0) {
	    genCubes=0;
	    Pdtutil_Assert(itpSplit==NULL,"null expected");
	  }
	  else if (res==1) {
	    //	    Ddi_Free(itpSplit);
	    if (careFwd!=NULL) {
	      toMinus = myToMinus;
	    }
	    itpEnded = 1;
	  }
	  else {
	    if (toMinusSplit==NULL) {
	      toMinusSplit = itpSplit;
	      Ddi_BddDiffAcc(aSplit,toMinusSplit); 
	    }
	  }
	}
	if (genClauses) { /* always enabled */
	  Ddi_Bdd_t *aPart = Ddi_AigPartitionTop(aSplit,0);
	  itpSplit =
	    Ddi_AigInterpolantByGenClauses(myBforCex,aPart,careTotForCex,
					   /*NULL*/aCare2, 
					   v2, v1, NULL, glbA, 
					   NULL, NULL, NULL, maxIter, 
					   0, &res);
	  Ddi_Free(aPart);
	  if (res==1) {
	    //	    Ddi_Free(itpSplit);
	    itpEnded = 1;
	  }
	  if (itpSplit!=NULL) {
	    Ddi_BddNotAcc(itpSplit);
	    if (!Ddi_BddIsOne(itpSplit) && !Ddi_BddIsOne(careTotForCex)) {
	      Ddi_BddAndAcc(itpSplit,careTotForCex);
	    }
	    if (toMinusSplit!=NULL) {
	      Ddi_BddOrAcc(itpSplit,toMinusSplit);
	    }
	  }
	}
	itpSplitDone=1;
	Ddi_Free(glbA);
	Ddi_Free(v1);
	Ddi_Free(v2);
	Ddi_Free(aSplit);
	Ddi_Free(aCare);
	Ddi_Free(aCare2);
	if (itpSplit!=NULL && Ddi_BddIsOne(itpSplit)) {
	  Ddi_Free(myB);
	  Ddi_Free(myBforCex);
	  Ddi_Free(itpSplit);
	  break;
	}
      }
      else {
	int undef = 0;
        int checkOnSplit = 1 && (splitVars!=NULL);
        int useCex = 1;
        int doCexWithGates = 1&&(i>0);
        if (0 && (i<1) && i<(itpPart-1)) {
          Ddi_Bdd_t *notB = Ddi_BddNot(myBforCex); 
          Ddi_Bdd_t *bPart = Ddi_AigConjDecomp (notB, 8, 0);
          int sizeMax = 3*Ddi_BddSize(myBforCex)/4;
          Ddi_Free(bDup);
          Ddi_BddNotAcc(bPart);
          Ddi_Free(notB);
          Ddi_BddPartSortBySizeAcc(bPart, 1);    // incr. size	
#if EXTRACT_MAXP
          Ddi_Free(myB);
          myB = Ddi_BddMakeConstAig(ddm, 0);
          int ii;
          for (ii=0; ii<1 && Ddi_BddPartNum(bPart)>4; ii++) {
            int n = Ddi_BddPartNum(bPart);
            Ddi_Bdd_t *p_n = Ddi_BddPartExtract(bPart,n-1);
            Ddi_BddOrAcc(myB,p_n);
            Ddi_Free(p_n);
          }
          Ddi_DataCopy(myBforCex,myB);
          Ddi_BddSetAig(bPart);
          bDup = Ddi_BddDup(bPart);
#else
          bDup = Ddi_BddMakeConstAig(ddm, 0);
          while (Ddi_BddPartNum(bPart)>4 &&
                 Ddi_BddSize(bPart)>sizeMax) {
            int i = Ddi_BddPartNum(bPart);
            Ddi_Bdd_t *p_i = Ddi_BddPartExtract(bPart,i-1);
            Ddi_BddOrAcc(bDup,p_i);
            Ddi_Free(p_i);
          }
          Ddi_BddSetAig(bPart);
          Ddi_DataCopy(myBforCex,bPart);
          Ddi_DataCopy(myB,bPart);
#endif
          useCex = 0;
          Ddi_Free(bPart);
        }
	else if (incrementalSat) {
          int invertCare = 1;
	  Ddi_Bdd_t *myCheck = Ddi_BddAnd(itpTot,bDup);
          if (checkOnSplit) {
            Ddi_Bdd_t *aSplit = Ddi_BddAnd(itpTot,splitRel);
            Ddi_Free(myCheck);
            if (invertCare) {
              Ddi_BddNotAcc(careTotForCex);
              myCheck = Ddi_BddOr(myBforCex,careTotForCex);
              //Pdtutil_Assert(!Ddi_AigSatAnd(aSplit,careTotForCex,
              //                            NULL),"Unsound care");
              Ddi_BddNotAcc(careTotForCex);
            }
            else {
              myCheck = Ddi_BddAnd(myBforCex,careTotForCex);
            }
            Ddi_BddAndAcc(myCheck,aSplit);
            if (optCare!=NULL) {
              Ddi_BddAndAcc(myCheck,optCare);
            }
            Ddi_Free(aSplit);
          }
	  if (itpPlus!=NULL) {
	    Ddi_BddAndAcc(myCheck,itpPlus);
	  }
	  Ddi_IncrSatMgrResume(ddiS);
          if (0 && doCexWithGates && cexWithGates>0) {
            cex = Ddi_AigSatMinisat22WithCexAigAndAbortIncremental(ddiS,
              myCheck, cexWithGates, 0, itpRefTime*2, NULL);
          }
          else {
            cex = Ddi_AigSatMinisat22WithCexAndAbortIncremental(ddiS,
		  myCheck, NULL, 1, itpRefTime*2, &undef);
          }
          Ddi_IncrSatMgrSuspend(ddiS);
	  if (chkincr && (cex!=NULL)) {
	    Pdtutil_Assert(Ddi_AigSatAnd(myBforCex,careTotForCex,
					 itpPlus),"SAT needed");
	    Pdtutil_Assert(Ddi_AigSatAnd(cex,myCheck,NULL),"BAD cex");
	  }
	  else if (chkincr && (cex==NULL)) {
	    Pdtutil_Assert(!Ddi_AigSatAnd(itpTot,b,
					 itpPlus),"UNSAT needed");
	  }
	  Ddi_Free(myCheck);
	}
	else {
          if (!checkOnSplit) {
            if (doCexWithGates && cexWithGates>0) {
              Ddi_Bdd_t *check = Ddi_BddDup(itpTot);
              Ddi_BddSetPartConj(itpTot);
              Ddi_BddPartInsertLast(check,b);
              if (itpPlus != NULL) {
                Ddi_BddPartInsertLast(check,b);
              }
              cex = Ddi_AigSatMinisat22WithCexAigAndAbortIncremental(NULL,
               check, cexWithGates, 0, -1, NULL);
              Ddi_Free(check);
            }       
            else {
              cex = Ddi_AigSatAndWithCexAndAbort(itpTot,b,
		  itpPlus,NULL,itpRefTime*2,&undef);
            }
          }
          else {
            int again = 1;
            int iter, maxIter=itpPart;
            int doPartial = 0;
            for (iter=0; again; iter++) {
              again = 0;
              if (iter>=maxIter) {
                if (doPartial) {
                  cex = NULL;
                }
                else {
                  cex = Ddi_AigSatAndWithCexAndAbort(itpTot,b,
                    itpPlus,NULL,itpRefTime*2,&undef);
                }
              }
              else {
                cex = Ddi_AigSatAndWithCexAndAbort(careTotForCex,
                       myBforCex,NULL,NULL,itpRefTime*2,&undef);
              }
              if (cex!=NULL) {
                Ddi_Bdd_t *bTmp = Ddi_BddDup(myB);
                Ddi_AigConstrainCubeAcc(bTmp, cex);
                if (doPartial && iter==(maxIter-1) && 
                    !Ddi_AigSatAnd(itpTot,bTmp,itpPlus)) {
                  Ddi_Free(cex);
                }
                else if (!Ddi_AigSatAnd(itpTot,bTmp,itpPlus)) {
                  Ddi_Bdd_t *itpRefine;
                  Ddi_Bdd_t *aSplit = Ddi_BddAnd(itpTot,splitRel);
                  Ddi_Bdd_t *aPart = Ddi_AigPartitionTop(aSplit,0);
                  Ddi_Bdd_t *bTmp2 = Ddi_BddDup(myBforCex);
                  Ddi_Bdd_t *cex2 = Ddi_BddExist(cex,splitVars);
                  Ddi_AigConstrainCubeAcc(bTmp2, cex2);
                  Ddi_Free(cex2);
                  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
                    fprintf(dMgrO(ddm),
                            "REFINING SPLIT ITP (iter: %d)\n",iter);
                  }
                  if (optCare!=NULL) {
                    Ddi_BddAndAcc(aSplit,optCare);
                  }
                  int chk2 = 1;
                  if (chk2) {
                    Pdtutil_Assert(Ddi_AigSatAnd(bTmp2,careTotForCex,
                             NULL),"sat required");
                  }
                  itpRefine =
                    Ddi_AigSat22AndWithInterpolant(NULL,aPart,
                         bTmp2,NULL,
                         splitVars, NULL,NULL,0,
                         NULL,NULL,
                         psat, 0, itpOdc, 1, timeLimit);
                  Ddi_Free(bTmp2);
                  Ddi_BddAndAcc(careTotForCex, itpRefine);
                  Ddi_Free(itpRefine);
                  Ddi_Free(cex);
                  again = 1;
                  Ddi_Free(aPart);
                  Ddi_Free(aSplit);
                }
                Ddi_Free(bTmp);
              }
            }
          }
        }
        if (useCex) {
          if (undef) {
            startTime = util_cpu_time();
            Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
              fprintf(dMgrO(ddm),
                      "undef CEX with b - retrying on split cone\n");
              cex = Ddi_AigSatAndWithCexAndAbort(myBforCex,careTotForCex,
                                                 itpPlus,NULL,-1,NULL);
            }
          }
          cexTime = util_cpu_time() - startTime;
          Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
            fprintf(dMgrO(ddm),"CEX time %s= %s\n", cex==NULL ? "(unsat) ":"",
                    util_print_time (cexTime));
            if (cex!=NULL) {
              fprintf(dMgrO(ddm),"CEX size = %d\n",
                      Ddi_BddSize(cex));
            }
          }
          if (2*cexTime > itpRefTime) {
            itpRefTime = 2*cexTime;
          }
          if (cex==NULL) {
            Ddi_Free(myB);
            Ddi_Free(myBforCex);
            break;
          }
          if (doCexWithGates && cexWithGates>0) {
            float ratio = ((float) (itpPart-i+1)) / itpPart;
            int reversed = 0;
            if (ratio > 1) ratio = 1.0;
            //            if (i==itpPart-2) ratio = 0;
            if (reversed) ratio = -ratio;
            setupCexWithGates(cex,itpTravMgr->observedGates,ratio);
            Ddi_BddAndAcc(myB, cex); 
            Ddi_BddAndAcc(myBforCex, cex);
            Ddi_AigStructRedRemAcc (myB,NULL);
            Ddi_AigStructRedRemAcc (myBforCex,NULL);
          }
          else {
            tfPiVars = itpMgr->timeFrames->PiVars;
	
            Ddi_BddExistAcc(cex,globalVars);
            if (splitVars!=NULL) {
              Ddi_BddExistAcc(cex,splitVars);
            }
            if (tfPiVars!=NULL && tfPiNum>0 && splitB!=NULL) {
              int j;
              int coneTopMark = Ddi_BddReadMark(splitB);
              int nSmooth = tfPiNum - 2*coneTopMark/2;
              for (j=0; j<nSmooth && j<tfPiNum; j++) {
                Ddi_Varset_t *smPi = Ddi_VarsetMakeFromArray(tfPiVars[j]);
                Ddi_BddExistAcc(cex,smPi);
                Ddi_Free(smPi);
              }
            }
            Ddi_AigConstrainCubeAcc(myB, cex); 
            Ddi_AigConstrainCubeAcc(myBforCex, cex);
          }
        }
        else {
          if (undef) {
            itp_i = Ddi_BddMakeConstAig(ddm, 1);
            i = itpPart-2;
          }
        }
      }
      if (preSplit && splitRel!=NULL) {
        int doReverse = 1;
	Ddi_Bdd_t *aSplit = Ddi_BddAnd(a,splitRel);
	int checkTime=0;
	Ddi_Bdd_t *aPart = Ddi_AigPartitionTop(aSplit,0);
	if (optCare!=NULL) {
          Ddi_BddAndAcc(aSplit,optCare);
          Ddi_BddPartInsertLast(aPart,optCare);
	}
	if (!itpSplitDone) {
          if (careTotForA!=NULL) {
            if (doReverse) 
              Ddi_BddPartInsertLast(aPart,careTotForA);
            else 
              Ddi_BddAndAcc(myBforCex,careTotForA);
          }
          int saveItpNnfAbstrAB = ddm->settings.aig.itpNnfAbstrAB;
          int saveItpReverse = ddm->settings.aig.itpReverse;
          // set just A
          ddm->settings.aig.itpNnfAbstrAB = 2;
          ddm->settings.aig.itpReverse = doReverse;
	  itpSplit =
	    Ddi_AigSat22AndWithInterpolant(NULL,aPart,myBforCex,NULL,
					   splitVars, NULL,NULL,0,
					   NULL,NULL,
					   psat, 0, itpOdc, 1, timeLimit);
          ddm->settings.aig.itpNnfAbstrAB = saveItpNnfAbstrAB;
          ddm->settings.aig.itpReverse = saveItpReverse;
          if (1 && itpSplit!=NULL && careTotForA!=NULL) {
            int doItp = 1;
            int doNnfAbstrA = 1;
            int doNnfAbstrB = 1;
            if (careForPreimg==NULL) {
              careForPreimg = Ddi_BddMakeConstAig(ddm, 1);
            }
            Ddi_BddAndAcc(careForPreimg,itpSplit);
            Ddi_BddNotAcc(careTotForA);
            Ddi_BddOrAcc(itpSplit,careTotForA);
            Ddi_BddNotAcc(careTotForA);
            int size0 = Ddi_BddSize(itpSplit);
            Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
              printf("\nNNF abstr split ITP\n");
            }
            if (doItp) {
              Ddi_Bdd_t *refB = Ddi_BddNot(itpSplit);
              Ddi_Bdd_t *itpSplit1 =
                Ddi_AigSat22AndWithInterpolant(NULL,aSplit,
                       refB,NULL,splitVars, NULL,NULL,0,
                       NULL,NULL,psat, 0, itpOdc, 1, timeLimit);
              Pdtutil_Assert(itpSplit1!=NULL,"Missing itp");
              if (Ddi_BddSize(itpSplit1) < Ddi_BddSize(itpSplit)) {
                Ddi_DataCopy(itpSplit,itpSplit1);
                doNnfAbstrB = 0;
              }
              Ddi_Free(itpSplit1);
              Ddi_Free(refB);
            }
            if (doNnfAbstrA) {
              Ddi_AigOptByMonotoneCoreAcc(itpSplit,aSplit,NULL,1,
                                          -1.0);
            }
            if (doNnfAbstrB) {
              Ddi_AigOptByMonotoneCoreAcc(itpSplit,myBforCex,NULL,0,
                                        -1.0);
            }
            Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
              printf("NNF abstr split ITP done: %d->%d\n",
                     size0, Ddi_BddSize(itpSplit));
            }
          }
        }
	else if (doItpFromGenClauses && (itpSplit!=NULL)) {
	  Ddi_Free(myBforCex);
	  myBforCex = Ddi_BddNot(itpSplit);
	  Ddi_Free(itpSplit);
	  itpSplit =
	    Ddi_AigSat22AndWithInterpolant(NULL,aPart,myBforCex,NULL,
					   splitVars, NULL,NULL,0,
					   NULL,NULL,
					   psat, 0, itpOdc, 1, timeLimit);
	}
	if (doItpFromGenClausesAux && (itpSplit!=NULL)) {
	  Ddi_Bddarray_t *d0,*d1;
	  Ddi_Bdd_t *itpSplit2, *itpSplit3;
	  Ddi_Varset_t *itpVars = Ddi_VarsetMakeFromArray(splitV2);
	  Ddi_Bdd_t *miter=NULL;
	  int tfShift = doSplit;
	  Ddi_Vararray_t *tfpi = itpMgr->timeFrames->PiVars[tfShift];
	  Ddi_Bdd_t *nextRing = NULL;
	  Ddi_Bdd_t *auxCone = NULL;
	  Ddi_Bdd_t *savedSeedStates = NULL;
	  int useSeedStates = 1; // itpNextRing==NULL

	  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
	    printf("Enlarging SPLIT ITP (%d) by fwd-bwd delta\n", 
		   Ddi_BddSize(itpSplit));
	  }
	  Ddi_Free(myBforCex);
	  myBforCex = Ddi_BddNot(itpSplit);
	  Ddi_BddSubstVarsAcc(myBforCex,splitV2,splitVaux);

	  if (itpMgr->fromRings!=NULL && 
	      Ddi_BddarrayNum(itpMgr->fromRings)>split_i) {
	    Ddi_Bdd_t *ring = 
	      Ddi_BddDup(Ddi_BddarrayRead(itpMgr->fromRings, split_i));
	    Ddi_BddSubstVarsAcc(ring,itpMgr->ns,splitVaux);
	    Ddi_BddAndAcc(myBforCex,ring);
	    Ddi_Free(ring);
	    if (Ddi_BddarrayNum(itpMgr->fromRings)>split_i+1) {
	      nextRing = Ddi_BddarrayRead(itpMgr->fromRings, split_i+1);
	    }
	  }
	  savedSeedStates = Ddi_BddDup(myBforCex);

	  d0 = Ddi_BddarrayDup(itpMgr->delta);
	  d1 = Ddi_BddarrayDup(itpMgr->delta);
	  Ddi_BddarraySubstVarsAcc(d0,itpMgr->ps,itpMgr->ns);
	  Ddi_BddarraySubstVarsAcc(d1,itpMgr->ps,itpMgr->ns);

	  if (Ddi_BddarrayNum(itpMgr->eqRings) > split_i) {
	    Ddi_Bdd_t *eqConstr = Ddi_BddarrayRead(itpMgr->eqRings, split_i);
	    if (eqConstr != NULL) {
	      Ddi_Vararray_t *vars = Ddi_BddReadEqVars(eqConstr);
	      Ddi_Bddarray_t *subst = Ddi_BddReadEqSubst(eqConstr);
	      
	      Ddi_BddarrayComposeAcc(d0, vars, subst);
	      Ddi_BddarrayComposeAcc(d1, vars, subst);
	    }
	  }

	  Ddi_BddarraySubstVarsAcc(d0,itpMgr->ns,splitVaux);
	  Ddi_BddarraySubstVarsAcc(d0,itpMgr->pi,piAux);
	  Ddi_BddarraySubstVarsAcc(d1,itpMgr->ns,splitV2);
	  Ddi_BddarraySubstVarsAcc(d1,itpMgr->pi,tfpi);

	  if (itpNextRing != NULL) {
	    auxCone = Ddi_BddNot(itpNextRing);
	    Ddi_BddSubstVarsAcc(auxCone,itpMgr->ns,splitVaux2);
	  } 
	  else if (nextRing != NULL) {
	    auxCone = Ddi_BddNot(nextRing);
	    Ddi_BddSubstVarsAcc(auxCone,itpMgr->ns,splitVaux2);
	  }
	  if (auxCone!=NULL) {
	    Ddi_Bdd_t *auxA = Ddi_BddDup(aPart), *auxApart;
	    Ddi_Bdd_t *d0Rel = Ddi_BddRelMakeFromArray(d0,splitVaux2);
	    Ddi_Bdd_t *d1Rel = Ddi_BddRelMakeFromArray(d1,splitVaux2);
	    Ddi_Bdd_t *auxItp = NULL;
	    Ddi_BddPartInsertLast(auxA,d1Rel);
            if (useSeedStates) { 
	      Ddi_BddPartInsertLast(auxA,d0Rel);
	      Ddi_BddPartInsertLast(auxA,myBforCex);
	    }
	    Ddi_BddSetAig(auxA);
	    auxApart = Ddi_AigPartitionTop(auxA,0);
	    Ddi_Free(auxA);
	    if (Ddi_BddSize(auxCone)==1) {
	      Ddi_Free(myBforCex);
	      myBforCex = Ddi_BddDup(auxCone);
	      Ddi_BddComposeAcc(myBforCex, splitVaux2, d1);
	    }
	    else if (Ddi_AigSat(auxApart)) {
	      auxItp = 
		Ddi_AigSat22AndWithInterpolant(NULL,auxApart,auxCone,NULL,
					   itpMgr->nsvars, NULL,NULL,0,
					   itpSplit,NULL,
					   psat, 0, itpOdc, 1, timeLimit);
	      Pdtutil_Assert(auxItp!=NULL,"NULL itp");
	      Ddi_BddNotAcc(auxItp);
	      Ddi_BddComposeAcc(auxItp, splitVaux2, d1);
	      Ddi_Free(myBforCex);
	      myBforCex = Ddi_BddDup(auxItp);
	    }
	    {
	      miter = Ddi_BddMiterMakeFromArray(d0, d1);
	      Ddi_BddSetAig(miter);
	    
	      Ddi_AigConstrainCubeAcc(miter,savedSeedStates);
	      Ddi_BddAndAcc(miter,savedSeedStates);
	      Ddi_BddAndAcc(myBforCex,miter);
	    }

	    Ddi_Free(auxItp);
	    Ddi_Free(d1Rel);
	    Ddi_Free(d0Rel);
	    Ddi_Free(auxApart);
	    Ddi_Free(auxCone);
	  }
	  else {
	    miter = Ddi_BddMiterMakeFromArray(d0, d1);
	    Ddi_BddSetAig(miter);
	    
	    Ddi_AigConstrainCubeAcc(miter,myBforCex);

	    Ddi_BddAndAcc(myBforCex,miter);
	    Ddi_BddAndAcc(myBforCex,splitB);
	  }
	  Ddi_Free(savedSeedStates);

	  itpSplit2 =
	    Ddi_AigSat22AndWithInterpolant(NULL,aPart,myBforCex,NULL,
					   itpVars, NULL,NULL,0,
					   itpSplit,NULL,
					   psat, 0, itpOdc, 1, timeLimit);
	  if (itpSplit2==NULL) {
	    // SAT !
	    Ddi_Free(itpSplit);
	  } 
	  else {
	    Ddi_BddAndAcc(itpSplit,itpSplit2);
#if 0
	    itpSplit3 =
	      Ddi_AigSat22AndWithInterpolant(NULL,aPart,splitB,NULL,
					   itpVars, NULL,NULL,0,
					   NULL,NULL,
					   psat, 0, itpOdc, 1, timeLimit);
	    Ddi_Free(itpSplit3);
#endif
	  }
	  Ddi_Free(auxCone);
	  Ddi_Free(itpSplit2);
	  Ddi_Free(myBforCex);
	  Ddi_Free(miter);
	  Ddi_Free(itpVars);
	  Ddi_Free(d0);
	  Ddi_Free(d1);
	}
	else if (doItpStrengthen && 
		 (itpSplit!=NULL) && (itpNextRing != NULL)) {
	  Ddi_Bddarray_t *d0;
	  Ddi_Bdd_t *d0Rel;
	  Ddi_Bdd_t *itpSplit2, *itpSplit3;
	  Ddi_Bdd_t *itpCurr = NULL, *itpNext = NULL;
	  Ddi_Varset_t *itpVars = Ddi_VarsetMakeFromArray(splitV2);
	  Ddi_Varset_t *itpVarsAux = Ddi_VarsetMakeFromArray(splitVaux);
	  Ddi_Bdd_t *miter=NULL;
	  int tfShift = doSplit;
	  Ddi_Vararray_t *tfpi = itpMgr->timeFrames->PiVars[tfShift];
	  Ddi_Bdd_t *nextRing = NULL;
	  Ddi_Bdd_t *auxCone = NULL;
	  Ddi_Bdd_t *savedSeedStates = NULL;
	  int useSeedStates = 1; // itpNextRing==NULL
	  Ddi_Bdd_t *auxA = NULL, *auxApart;

	  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
	    printf("Strengthening SPLIT ITP (%d) by fwd-bwd delta\n", 
		   Ddi_BddSize(itpSplit));
	  }
	  itpCurr = Ddi_BddDup(itpSplit);
	  itpNext = Ddi_BddDup(itpNextRing);
	  if (!Ddi_BddIsOne(careTotForCex)) {
	    Ddi_BddAndAcc(itpCurr,careTotForCex);
	  }

	  if (itpMgr->fromRings!=NULL && 
	      Ddi_BddarrayNum(itpMgr->fromRings)>split_i) {
	    Ddi_Bdd_t *ring = 
	      Ddi_BddDup(Ddi_BddarrayRead(itpMgr->fromRings, split_i));
	    Ddi_BddSubstVarsAcc(ring,itpMgr->ns,splitV2);
	    //	    Ddi_BddAndAcc(itpCurr,ring);
	    Ddi_Free(ring);
	    if (Ddi_BddarrayNum(itpMgr->fromRings)>split_i+1) {
	      nextRing = Ddi_BddarrayRead(itpMgr->fromRings, split_i+1);
	      Ddi_BddAndAcc(itpNext,nextRing);
	    }
	  }
	  Ddi_BddSubstVarsAcc(itpNext,itpMgr->ns,splitVaux);

	  /* strengthen itpNext */

	  d0 = Ddi_BddarrayDup(itpMgr->delta);
	  Ddi_BddarraySubstVarsAcc(d0,itpMgr->ps,itpMgr->ns);
	  Ddi_BddarraySubstVarsAcc(d0,itpMgr->pi,piAux);
	  d0Rel = Ddi_BddRelMakeFromArray(d0,splitVaux);

	  TravItpMgrApplyRingEq(itpMgr,itpCurr,NULL,splitV2,split_i,0);
	  TravItpMgrApplyRingEq(itpMgr,d0Rel,NULL,NULL,split_i,0);
	  TravItpMgrApplyRingEq(itpMgr,d0Rel,NULL,splitVaux,split_i+1,0);
	  TravItpMgrApplyRingEq(itpMgr,itpNext,NULL,splitVaux,split_i+1,0);

	  Ddi_BddSubstVarsAcc(d0Rel,itpMgr->ns,splitV2);
	  Ddi_BddarraySubstVarsAcc(d0,itpMgr->ns,splitV2);

	  auxA = Ddi_BddDup(itpCurr);
	  Ddi_BddSetPartConj(auxA);
	  Ddi_BddPartInsertLast(auxA,d0Rel);
	  Ddi_BddSetAig(auxA);
	  auxApart = Ddi_AigPartitionTop(auxA,0);
	  Ddi_Free(auxA);

	  auxCone = Ddi_BddNot(itpNext);

	  if (Ddi_AigSat(auxApart)) {
	    Ddi_Bdd_t *auxItp = 
	      Ddi_AigSat22AndWithInterpolant(NULL,auxApart,auxCone,NULL,
					   itpVarsAux, NULL,NULL,0,
					   NULL,NULL,
					   psat, 0, itpOdc, 1, timeLimit);
	    Pdtutil_Assert(auxItp!=NULL,"NULL itp");
	    Ddi_Free(myBforCex);
	    myBforCex = Ddi_BddNot(auxItp);
	    Ddi_BddSubstVarsAcc(auxItp,splitVaux,itpMgr->ns);
	    Ddi_BddAndAcc(itpNextRing,auxItp);
	    Ddi_BddSetAig(d0Rel);
	    Ddi_BddAndAcc(myBforCex,d0Rel);
	    //	    Ddi_BddComposeAcc(myBforCex, itpMgr->ns, d0);
	    Ddi_Free(auxItp);
	  }
	  Ddi_Free(itpCurr);
	  Ddi_Free(itpNext);
	  Ddi_Free(d0Rel);
	  Ddi_Free(auxApart);
	  Ddi_Free(auxCone);

	  itpSplit2 =
	    Ddi_AigSat22AndWithInterpolant(NULL,aPart,myBforCex,NULL,
					   itpVars, NULL,NULL,0,
					   itpSplit,NULL,
					   psat, 0, itpOdc, 1, timeLimit);
	  Ddi_Free(auxApart);

	  if (itpSplit2==NULL) {
	    // SAT !
	    Ddi_Free(itpSplit);
	  } 
	  else {
	    Ddi_BddAndAcc(itpSplit,itpSplit2);

	  }
	  Ddi_Free(auxCone);
	  Ddi_Free(itpSplit2);
	  Ddi_Free(itpVarsAux);
	  Ddi_Free(myBforCex);
	  Ddi_Free(miter);
	  Ddi_Free(itpVars);
	  Ddi_Free(d0);
	}

	Ddi_Free(aPart);
	if (itpSplit!=NULL) {
	  printf("\nSPLIT ITP: %d (now computing part ITP)\n", 
		 Ddi_BddSize(itpSplit));
	}
	if (itpSplit!=NULL) {
	  Ddi_Bdd_t *myB2;
          //	  Ddi_AigOptByMonotoneCoreAcc(itpSplit,aSplit,careTotForCex,
          //			      1,-1.0);
	  if (cex!=NULL) {
	    int sat;
	    Ddi_Bdd_t *prod = Ddi_BddDup(itpSplit);
	    if (splitB!=NULL) {
	      Ddi_BddAndAcc(prod,splitB);
	    }
	    else {
	      Ddi_BddAndAcc(prod,bDup);
	    }
	    Ddi_BddAndAcc(prod,careTotForCex);
	    sat = Ddi_AigSatMinisatWithAbortAndFinal(prod,cex,-1.0,1);
	    Ddi_BddSetAig(cex);
	    Pdtutil_Assert(!sat,"unsat required");
	    Ddi_BddDiffAcc(careFinal,cex);
	    Ddi_Free(prod);
	  }
	  myB2 = Ddi_BddNot(itpSplit);
	  Ddi_BddAndAcc(careTotForCex, itpSplit);
          if (i==itpPart-1) {
            if ((saveSplitConeInfo>1) &&
                itpTravMgr->splitConeInfo.preImg!=NULL) {
              Ddi_Bdd_t *prevBwd =
                Ddi_BddDup(itpTravMgr->splitConeInfo.preImg);
              Ddi_BddSubstVarsAcc(prevBwd,splitRefV,splitV);
              Ddi_BddOrAcc(myB2,prevBwd);
              Ddi_Free(prevBwd);
            }
          }
          //	  Ddi_BddAndAcc(careTotSplit, itpSplit);
	  Ddi_BddComposeAcc(myB2,splitV,splitU);
	  if (splitConstr!=NULL) {
	    Ddi_BddAndAcc(myB2, splitConstr);
	  }
	  itp_i =
	    Ddi_AigSat22AndWithInterpolant(NULL,a,myB2,NULL,
					   globalVars, domainVars,NULL,0,
					   optCare,NULL,
					   psat, 0, itpOdc, 0, timeLimit);
	  //Pdtutil_Assert(itp_i!=NULL,"NULL itpSplit");
	  Ddi_Free(aSplit);
	  Ddi_Free(myB2);
	}
	else {
	  itp_i = NULL;
	}
	Ddi_Free(aSplit);
      }
      else if (itp_i==NULL) {
#if 1
        startTime = util_cpu_time();
	itp_i =
	  Ddi_AigSat22AndWithInterpolant(NULL,a,myB,NULL,
					 globalVars, domainVars,NULL,0,
					 optCare,NULL,
					 psat, 0, itpOdc, 0, timeLimit);
        itpTime = util_cpu_time() - startTime;
        if (itpTime > itpRefTime) {
          itpRefTime = itpTime;
        }
#else
	itp_i = Ddi_BddNot(myB);
#endif
      }
      Ddi_Free(cex);
      if ((itp_i!=NULL)&&toMinus!=NULL) {
	Ddi_BddOrAcc(itp_i,toMinus);
      }
      if ((itp_i!=NULL)&&Ddi_BddIsOne(itp_i)) {
	// skip
	// go to next iteration
      }
      else if ((itp_i!=NULL)&&
	       (Ddi_AigOptByMonotoneCoreAcc(itp_i,a,careTot,1,-1.0)==NULL)) {
	// SAT
	Ddi_Free(itp_i);
      }  
      else if (itp_i!=NULL && doSplitCare) {
	//    Ddi_Bdd_t *myCone = Ddi_BddDup(f_j0);
	Ddi_Bdd_t *myB2Ref = Ddi_BddNot(itpSplit);
	Ddi_Bdd_t *myB3 = Ddi_BddNot(itpSplit);
	//	Ddi_Bdd_t *myB4 = Ddi_BddDup(splitB);
	Ddi_Bdd_t *myB4 = Ddi_BddNot(itpSplit);
	Ddi_Bdd_t *myB2 = Ddi_BddNot(itpSplit);
	Ddi_Bdd_t *myConeRel=NULL, *itpSplitCare_i=NULL;
	Ddi_Bdd_t *myA = NULL;
      
	Ddi_Vararray_t *nsSupp = NULL;
	int splitCare_i = step+doSplit2;
	int start_i = step+doSplit, coneTopMark = 0;
      
	if (0 && !Ddi_BddIsOne(careTotForCex)) {
	  Ddi_BddAndAcc(myB4,careTotForCex);
	  Ddi_BddAndAcc(myB2,careTotForCex);
	  Ddi_BddAndAcc(myB2Ref,careTotForCex);
	  Ddi_BddAndAcc(myB3,careTotForCex);
	}
	Ddi_BddSubstVarsAcc(myB4, splitV2, itpMgr->ns);
	Ddi_BddSubstVarsAcc(myB2, splitV2, itpMgr->ns);
	//	Ddi_BddSubstVarsAcc(myB2Ref, splitV2, itpMgr->ns);
      
	//	Ddi_BddWriteMark(myB2, doSplit+1);
	Ddi_BddWriteMark(myB2, 0);
	Ddi_BddWriteMark(myB2Ref, 0);
	Ddi_BddWriteMark(myB4, 0);
      
	TravGrowConeBwd(itpMgr, myB2, start_i - 1, splitCare_i, itpMgr->delta,
		    itpMgr->initStub, useRingConstr, andWithRing_i, 1);
	coneTopMark = Ddi_BddReadMark(myB2);
	//	growConeBwd(itpMgr, myB2Ref, start_i-1, step, itpMgr->delta,
	//	    itpMgr->initStub, 1, 1);
	TravGrowConeBwd(itpMgr, myB4, start_i-1, step, itpMgr->delta,
		    itpMgr->initStub, useRingConstr, andWithRing_i, 1);
	Ddi_BddComposeAcc(myB2Ref,splitV,splitU);
	Ddi_BddComposeAcc(myB3,splitV,splitU);
      
	nsSupp = Ddi_BddSuppVararray(myB2);
	Ddi_VararrayIntersectAcc(nsSupp, itpMgr->ns);
      
	//problema cone
	myConeRel = TravGrowUnrollRelation(itpMgr, nsSupp, 
				       splitCare_i - 1, step,
				       itpMgr->delta, itpMgr->initStub, useRingConstr, 
				       andWithRing_i, 1, coneTopMark);
      
	if (Ddi_AigSat(myConeRel)) {
	  if (itpTravMgr->constrainVars != NULL && 
	      Ddi_VararrayNum(itpTravMgr->constrainVars)>0) {
	    Ddi_Vararray_t *v = Ddi_VararrayDup(itpTravMgr->constrainVars);
	    Ddi_Bddarray_t *s = Ddi_BddarrayDup(itpTravMgr->constrainSubstLits);
	    Ddi_BddarraySubstVarsAcc(s, itpMgr->ns, itpMgr->ps);
	    Ddi_VararraySubstVarsAcc(v, itpMgr->ns, itpMgr->ps);
	    Ddi_BddComposeAcc(myConeRel, v, s);
	    Ddi_Free(v);
	    Ddi_Free(s);
	  }
	  if (toPlusCube!=NULL && !Ddi_BddIsOne(toPlusCube)) {
	    Ddi_Bdd_t *t = Ddi_BddDup(toPlusCube);
	    Ddi_BddSubstVarsAcc(t, itpMgr->ns, itpMgr->ps);
	    Ddi_AigConstrainCubeAcc(myConeRel,t);
	    Ddi_Free(t);
	  }
	
	  myA = Ddi_BddDup(itp_i);
	  Ddi_BddSubstVarsAcc(myA, itpMgr->ns, itpMgr->ps);
	  //    chk = Ddi_BddCofactor(f_jj,pvarNs,1);
	  Ddi_BddAndAcc(myA, myConeRel);
	
	  if (!Ddi_AigSat(myA)) {
	    itpSplitCare_i = Ddi_BddMakeConstAig(ddm, 0);
	    itpEnded = 1;
	  }
	  else {
	    itpSplitCare_i =
	      Ddi_AigSat22AndWithInterpolant(NULL,myA,myB2,NULL,
					     globalVars, domainVars,NULL,0,
					     NULL,NULL,
					     psat, 0, itpOdc, 0, timeLimit);
	  }
	  Pdtutil_Assert(itpSplitCare_i != NULL, "UNSAT REQUIRED");
	  Ddi_BddAndAcc(careTotSplit, itpSplitCare_i);
	}
      
	Ddi_Free(myA);
	Ddi_Free(myConeRel);
	Ddi_Free(itpSplitCare_i);
	Ddi_Free(myB4);
	Ddi_Free(myB2);
	Ddi_Free(myB3);
	Ddi_Free(myB2Ref);
	Ddi_Free(nsSupp);  
      
      }
      if (completeOnSplit) {
	if (itpSplit!=NULL) {
	  Ddi_BddAndAcc(careTotForCex, itpSplit);
	}    
      }
      Ddi_Free(itpSplit);
      Ddi_Free(myB);
    }
  
    if (itp_i != NULL && i<itpPart-1) {
      if (!completeOnSplit && splitRel==NULL) {
	Ddi_BddAndAcc(careTotForCex, itp_i);
      }
      else if (!preSplit) {
	Ddi_Bdd_t *itpSplit, *aSplit = Ddi_BddAnd(itp_i,splitRel);
	int checkTime=0;
	if (optCare!=NULL) {
	  Ddi_BddAndAcc(aSplit,optCare);
	}
	itpSplit =
	  Ddi_AigSat22AndWithInterpolant(NULL,aSplit,myBforCex,NULL,
					 splitVars, NULL,NULL,0,
					 NULL,NULL,
					 psat, 0, itpOdc, 0, timeLimit);
	Pdtutil_Assert(itpSplit!=NULL,"NULL itpSplit");
	Ddi_AigOptByMonotoneCoreAcc(itpSplit,aSplit,careTotForCex,
				    1,-1.0);
	if (checkTime) {
	  long time0, time1;
	  Ddi_Bdd_t *itp2, *bSplit = Ddi_BddAnd(itpSplit,splitRel);
	  time0 = util_cpu_time();
	
	  itp2 =
	    Ddi_AigSat22AndWithInterpolant(NULL,a,bSplit,NULL,
					   splitVars, NULL,NULL,0,
					   optCare,NULL,
					   psat, 0, itpOdc, 0, timeLimit);
	  time1 = util_cpu_time() - time0;
	  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
	    fprintf(dMgrO(ddm),"ITP2 time = %s\n", util_print_time (time1));
	  }
	  Ddi_Free(bSplit);
	  Ddi_Free(itp2);
	}
      
	Ddi_BddAndAcc(careTotForCex, itpSplit);
	Ddi_Free(itpSplit);
	Ddi_Free(aSplit);
      }
    }
  
    Ddi_Free(toMinusSplit);
    Ddi_Free(myB);
    Ddi_Free(myBforCex);
  
    if (itp_i != NULL) {
      Ddi_BddAndAcc(itpTot, itp_i);
      Ddi_BddAndAcc(careTot, itp_i);
    
      if (doFwdBwd) {
	Ddi_Bdd_t *itpSplit2, *itp_i2;
	Ddi_Bdd_t *myA2 = Ddi_BddAnd(itpTot,splitRel);
	Ddi_Bdd_t *myB2 = Ddi_BddNot(careTotForCex);	
	itpSplit2 =
	    Ddi_AigSat22AndWithInterpolant(NULL,myA2,myB2,NULL,
					   globalVars, NULL,NULL,0,
					   NULL,NULL,
					   psat, 0, itpOdc, 0, timeLimit);
	
        Pdtutil_Assert(itpSplit2!=NULL,"NULL ITP");
	Ddi_Free(myA2);
	Ddi_Free(myB2);
	
	myB2 = Ddi_BddNot(itpSplit2);
	Ddi_BddAndAcc(careTotForCex,itpSplit2);
	Ddi_BddComposeAcc(myB2,splitV,splitU);
	Ddi_Free(itpSplit2);
	if (splitConstr!=NULL) {
	  Ddi_BddAndAcc(myB2, splitConstr);
	}
	itp_i2 =
	    Ddi_AigSat22AndWithInterpolant(NULL,a,myB2,NULL,
					   globalVars, domainVars,NULL,0,
					   optCare,NULL,
					   psat, 0, itpOdc, 0, timeLimit);


	Pdtutil_Assert(itp_i2!=NULL,"NULL itpSplit");
	Ddi_Free(myB2);

	Ddi_AigOptByMonotoneCoreAcc(itp_i,a,careTot,1,-1.0);

	Ddi_BddAndAcc(itpTot, itp_i2);
	Ddi_BddAndAcc(careTot, itp_i2);
      
      }

      Ddi_Free(itp_i);

    }
    else {
      Ddi_Free(itpTot);
      break;
    }
  
  }

  static int chkItp = saveSplitConeInfo>1;
  if (saveSplitConeInfo && itpTot!=NULL) {
    Ddi_Bdd_t *bwdRing = Ddi_BddNot(careTotForCex);
    Ddi_Bdd_t *prevRing = NULL;
    int bound = Ddi_BddReadMark(splitB), ring_i = step+doSplit;
    Ddi_BddSubstVarsAcc(bwdRing,splitV,splitRefV);
    Ddi_Free(itpTravMgr->splitConeInfo.ring);
    itpTravMgr->splitConeInfo.ring = Ddi_BddDup(bwdRing);
    if (saveSplitConeInfo>1) {
      if (itpTravMgr->splitConeInfo.preImg!=NULL) {
        prevRing = Ddi_BddDup(itpTravMgr->splitConeInfo.preImg);
        Ddi_Free(itpTravMgr->splitConeInfo.preImg);
      }
      int saveItpReverse = ddm->settings.aig.itpReverse;
      //ddm->settings.aig.itpReverse = 0;
      if (careForPreimg!=NULL) {
        Ddi_Free(bwdRing);
        bwdRing = Ddi_BddNot(careForPreimg);
        Ddi_BddSubstVarsAcc(bwdRing,splitV,splitRefV);
        Ddi_BddOrAcc(bwdRing,prevRing);
      }
      itpTravMgr->splitConeInfo.preImg =
        itpBwdRingPreimg(itpMgr,bwdRing,prevRing,ring_i,bound);
      ddm->settings.aig.itpReverse = saveItpReverse;
      Ddi_Free(prevRing);
    }
    if (chkItp) {
      Ddi_Bdd_t *chkA = Ddi_BddDup(a);
      if (optCare!=NULL) {
        Ddi_BddAndAcc(chkA,optCare);
      }
      Pdtutil_Assert(Ddi_BddIncluded(chkA,itpTot),"wrong itp - A side");
      Ddi_Free(chkA);
      Pdtutil_Assert(!Ddi_AigSatAnd(b,itpTot,optCare),
                     "wrong itp - B side");
      Ddi_Bdd_t *chkRingFwd = Ddi_BddDup(itpTot);
      if (optCare!=NULL) {
        Ddi_BddAndAcc(chkRingFwd,optCare);
      }
      int res = TravItpCheckConeAtRing(itpMgr,chkRingFwd,bwdRing,step,doSplit-1,0);
      int res1 = TravItpCheckConeAtRing(itpMgr,chkRingFwd,
        itpTravMgr->splitConeInfo.preImg,step,doSplit,0);
      assert(0 || !res && !res1);
      Ddi_Free(chkRingFwd);
    }
    Ddi_Free(bwdRing);
  }
  
  Ddi_Free(itpNextRing);
  Ddi_Free(splitV2);
  Ddi_Free(piAux);
  Ddi_Free(splitVaux);
  Ddi_Free(splitVaux2);
  Ddi_Free(splitB);
  Ddi_Free(splitCare);
  Ddi_Free(splitConstr);
  Ddi_Free(splitU);
  Ddi_Free(splitV);
  Ddi_Free(splitRel);
  Ddi_Free(splitRel2);
  Ddi_Free(splitVars);

  Ddi_Free(bDup);
  Ddi_Free(careTot);
  Ddi_Free(careFinal);
  Ddi_Free(careTotSplit);
  Ddi_Free(careTotForCex);
  Ddi_Free(careTotForA);
  Ddi_Free(careForPreimg);

  Ddi_IncrSatMgrQuitKeepDdi(ddiS);

  if (itpTot!=NULL && !Ddi_BddIsConstant(itpTot)) {
    Ddi_AigOptByMonotoneCoreAcc (itpTot,a,careTot,1,-1.0);
  }

  if (itpTot==NULL && psat!=NULL) {
    *psat=1;
  }

  if (chkres && itpTot!=NULL) {
    if (0 && Ddi_AigSatAnd(itpTot,b,optCare)) {
      printf("WARNING: partial interpolant\n");
    }
    Pdtutil_Assert(!Ddi_AigSatAnd(itpTot,b,optCare),"wrong part itp");
    Ddi_BddNotAcc(itpTot);
    Pdtutil_Assert(!Ddi_AigSatAnd(itpTot,a,optCare),"wrong part itp");
    Ddi_BddNotAcc(itpTot);
  }
  else if (chkresSat && itpTot!=NULL && psat!=NULL && *psat) {
    Pdtutil_Assert(Ddi_AigSatAnd(itpTot,b,optCare),"wrong itp sat");
  }
  else if (chkresSat && itpTot==NULL && psat!=NULL && *psat) {
    Pdtutil_Assert(Ddi_AigSatAnd(a,b,optCare),"wrong itp sat");
  }

  return itpTot;
}




/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


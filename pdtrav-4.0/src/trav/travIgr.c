/**CFile***********************************************************************

  FileName    [travIgr.c]

  PackageName [trav]

  Synopsis    [Aig/SAT based verif/traversal routines]

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

static int
igrItpRingsInit(
  Trav_ItpMgr_t *itpMgr
);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Trav_TravIgrPdrVerif(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Fsm_Mgr_t * fsmMgr,
  int timeLimit
)
{
  int res=-1;

  Ddi_Bdd_t *invarspec = Fsm_MgrReadInvarspecBDD(fsmMgr);
  Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
  Ddi_Bdd_t *invar = Fsm_MgrReadConstraintBDD(fsmMgr);
  Ddi_Bdd_t *care = Fsm_MgrReadCareBDD(fsmMgr);
  int again = 1, k=0;
  int enPdr=1, enIgr=0, initPdr=-1; 
  Trav_Mgr_t * travMgrItp=NULL;
  Fsm_Mgr_t * fsmMgr2 = Fsm_MgrDupWithNewDdiMgr(fsmMgr);
  Ddi_Bdd_t *invarspec2 = Fsm_MgrReadInvarspecBDD(fsmMgr2);
  Ddi_Bdd_t *init2 = Fsm_MgrReadInitBDD(fsmMgr2);
  Ddi_Bdd_t *invar2 = Fsm_MgrReadConstraintBDD(fsmMgr2);
  Ddi_Bdd_t *care2 = Fsm_MgrReadCareBDD(fsmMgr2);

  Trav_MgrSetOption(travMgr, Pdt_TravPdrReuseRings_c, inum, 2);
  travMgr->settings.ints.suspendFrame = 1;

  do {
    if (initPdr>=0) {
      enPdr = k <= initPdr;
      enIgr = (k==0) || (k >= initPdr);
    }
    k++;
    if (enPdr) {
      Trav_TravPdrVerif(travMgr, fsmMgr,
	init, invarspec, invar, care, -1, timeLimit);
      res = Trav_MgrReadAssertFlag(travMgr);
    }
    if (res>=0) again = 0;
    else if (enIgr) {
      if (travMgrItp==NULL) {
	travMgrItp = Trav_MgrInit("itp", Fsm_MgrReadDdManager(fsmMgr2));
	travMgrItp->settings.aig = travMgr->settings.aig;
	travMgrItp->settings.aig.satSolver = 
	  Pdtutil_StrDup(travMgr->settings.aig.satSolver);
	travMgrItp->settings.aig.igrMaxIter=1;
	Trav_MgrSetFromSelect(travMgrItp, Trav_FromSelectReached_c);
	Trav_MgrSetItpUseReached(travMgrItp, 1);
	Trav_MgrSetIgrSide(travMgrItp, 0);
	Trav_MgrSetItpCheckCompleteness(travMgrItp, 1);
	Trav_MgrSetOption(travMgrItp, Pdt_TravIgrRewind_c, inum, 0);
      }
      if (enPdr && k>1) {
	Trav_PdrToIgr(travMgr,travMgrItp);
      }
      res = Trav_TravSatIgrVerif(travMgrItp,
	      fsmMgr2, init2, invarspec2, invar2, care2, 
	      -1, -1, 1.0, 0, timeLimit);
      if (res>=0) {
	again = 0;
	Trav_MgrSetAssertFlag (travMgr, res);
      }
      else if (enPdr && k>=1) {

	Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMax_c) {
	  fprintf(tMgrO(travMgr), 
		  "\nIGR suspended\n");
	}

	Trav_IgrToPdr(travMgrItp,travMgr);
	k = travMgrItp->settings.ints.itpMgr->nRings-1;
	travMgr->settings.ints.suspendFrame = k+100;
      }
    }
    timeLimit *= 2;
    travMgr->settings.ints.suspendFrame++;

  } while (again);

  if (travMgrItp!=NULL) {
    Trav_MgrQuit(travMgrItp);
  }

  Fsm_MgrQuit(fsmMgr2);

  return res;
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Trav_TravSatIgrVerif(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  int step,
  int bound,
  float lazyRate,
  int firstFwdIter,
  int timeLimit
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);


  Trav_ItpMgr_t *itpMgr = NULL;
  Ddi_Bdd_t *itpCare, *itpCone;
  long initTime;
  int activeInterpolant = 0, abort=0, sat=0, res=0, fixPoint=0;
  int itpConstrLevel = Trav_MgrReadItpConstrLevel(travMgr);
  int optLevel = 0, i;

  /* initialization steps */

  Ddi_MgrSetOption(ddm, Pdt_DdiLazyRate_c, fnum, lazyRate);
  Trav_MgrSetItpReuseRings(travMgr,2);

  if (travMgr->settings.ints.itpMgr == NULL) {
    itpMgr = travMgr->settings.ints.itpMgr =
      Trav_ItpMgrInit(travMgr, fsmMgr, 
		    itpConstrLevel > 1 ? invar : 0,
		    invarspec, 1, optLevel);

    igrItpRingsInit(itpMgr);

    initTime = travMgr->travTime = util_cpu_time();
    if (itpMgr->time_limit > 0) {
      itpMgr->time_limit = initTime + itpMgr->time_limit;
    }
  }

  itpMgr = travMgr->settings.ints.itpMgr;
  activeInterpolant = -itpMgr->nRings;

  itpCare = Ddi_BddMakeConstAig(ddm, 1);
  itpCone = Ddi_BddDup(itpMgr->target);
  Ddi_BddWriteMark(itpCone, 0);

  fixPoint = Trav_IgrTrav(travMgr, fsmMgr,
    itpMgr, itpCone, itpCare,
    NULL, NULL, 0, 0, &activeInterpolant, &abort, &sat);

  res = fixPoint;
  if (res>=0) res = !res;

  Pdtutil_Assert(abort < 2, "invalid abort code");

  if (abort > 0) {
    activeInterpolant = -1;

    if (sat) {
      Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMax_c) {
	fprintf(tMgrO(travMgr), "BMC failure at bound %d.\n", i);
      }
    }
  }
  if (!sat) {
    if (!fixPoint) {
      res = -1;
    }
    else {
      Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMax_c) {
	fprintf(tMgrO(travMgr), "IGR Pass\n", i);
      } 
   }
  }

  //Trav_MgrSetAssertFlag (travMgr, !res);
  Trav_MgrSetAssertFlag(travMgr, sat);

  if (res>=0) {
    Trav_ItpMgrQuit(itpMgr);
    travMgr->settings.ints.itpMgr = NULL;
  }

  Ddi_Free(itpCare);
  Ddi_Free(itpCone);

  //Trav_MgrSetAssertFlag(travMgr, !res);
  return (res);

}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Trav_PdrToIgr (
  Trav_Mgr_t * travMgrPdr,
  Trav_Mgr_t * travMgrIgr
)
{
  TravPdrMgr_t *pdrMgr = travMgrPdr->settings.ints.pdrMgr;
  Trav_ItpMgr_t *itpMgr = travMgrIgr->settings.ints.itpMgr;
  Ddi_Mgr_t *ddm = itpMgr->travMgr->ddiMgrTr;
  Ddi_Bdd_t *myOne = Ddi_BddMakeConstAig(ddm, 1);
  Ddi_Bdd_t *reachedAig = Ddi_BddMakeConstAig(ddm, 1);

  int i, k, kMax = pdrMgr->nTimeFrames - 1;

  while (Ddi_BddarrayNum(itpMgr->fromRings)<=kMax) {
    Ddi_BddarrayInsertLast(itpMgr->fromRings,myOne);
  }
  while (Ddi_BddarrayNum(itpMgr->pdrReachedRings)<=kMax) {
    Ddi_BddarrayInsertLast(itpMgr->pdrReachedRings,myOne);
  }

  for (k = kMax; k >= 1; k--) {
    TravPdrTimeFrame_t *tf = &(pdrMgr->timeFrames[k]);
    Ddi_Bdd_t *currRing = Ddi_BddarrayRead(itpMgr->fromRings, k);
    Ddi_Bdd_t *pdrRing = Ddi_BddarrayRead(itpMgr->pdrReachedRings, k);
    // Ddi_Bdd_t *currRing = Ddi_BddarrayRead(itpMgr->pdrReachedRings, k);
    for (i = 0; i < Ddi_ClauseArrayNum(tf->clauses); i++) {
      Ddi_Clause_t *cl = Ddi_ClauseArrayRead(tf->clauses, i);
      Ddi_Bdd_t *clAig = Ddi_BddMakeFromClauseWithVars(cl, itpMgr->ns);
      Ddi_BddAndAcc(reachedAig,clAig);
      Ddi_Free(clAig);
    }
    
    Ddi_BddAndAcc(currRing,reachedAig);
    Ddi_DataCopy(pdrRing,reachedAig);
  }

  if (kMax>1) {
    travMgrIgr->settings.ints.doAbstrRefinedTo = 1;
  }
  
  itpMgr->nRings = Ddi_BddarrayNum(itpMgr->fromRings);
  itpMgr->igr.sameConeFail = -1;

  Ddi_Free(myOne);
  Ddi_Free(reachedAig);

}



/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
igrItpRingsInit(
  Trav_ItpMgr_t *itpMgr
)
{
  Ddi_Mgr_t *ddm = itpMgr->travMgr->ddiMgrTr;
  Ddi_Bdd_t *myConst = Ddi_BddMakeConstAig(ddm, 0);
  Ddi_Bdd_t *myInvar = Ddi_BddNot(itpMgr->target);
  Ddi_Bdd_t *myInit = Ddi_BddDup(itpMgr->init);

  Ddi_BddSwapVarsAcc(myInvar, itpMgr->ps, itpMgr->ns);
  if (myInit!=NULL)
    Ddi_BddSwapVarsAcc(myInit, itpMgr->ps, itpMgr->ns);

  itpMgr->nRings = 2;
  Ddi_BddarrayWrite(itpMgr->fromRings,0,myInit);
  Ddi_BddarrayWrite(itpMgr->reachedRings,0,myConst);
  Ddi_BddarrayWrite(itpMgr->fromRings,1,myInvar);

  Ddi_Free(myInit);
  Ddi_Free(myInvar);
  Ddi_Free(myConst);

  return 2;
}

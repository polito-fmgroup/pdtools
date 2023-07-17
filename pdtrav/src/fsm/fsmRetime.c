/**CFile***********************************************************************

  FileName    [fsmRetime.c]

  PackageName [fsm]

  Synopsis    [Functions to retiming a FSM for reachability analysis.]

  Description [External procedures included in this module:
    <ul>
    <li> Fsm_ComputeOptimalRetiming ()
    </ul>
    ]

  SeeAlso     []

  Author      [Gianpiero Cabodi and Stefano Quer]

  Copyright [  Copyright (c) 2001 by Politecnico di Torino.
    All Rights Reserved. This software is for educational purposes only.
    Permission is given to academic institutions to use, copy, and modify
    this software and its documentation provided that this introductory
    message is not removed, that this software and its documentation is
    used for the institutions' internal research and educational purposes,
    and that no monies are exchanged. No guarantee is expressed or implied
    by the distribution of this code.
    Send bug-reports and/or questions to: {cabodi,quer}@polito.it. ]

  Revision    []

******************************************************************************/

#include "fsmInt.h"
#include "ddiInt.h"
#include "baigInt.h"
#include "tr.h"
#include "part.h"

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
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#define fMgrO(fsmMgr) ((fsmMgr)->settings.stdout)
#define dMgrO(ddiMgr) ((ddiMgr)->settings.stdout)

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static void fsmCoiReduction(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Varset_t * coiVars
);
static void writeCoiShells(
  char *wrCoi,
  Ddi_Varsetarray_t * coi,
  Ddi_Vararray_t * vars
);
static Ddi_Varsetarray_t *computeFsmCoiVars(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * p,
  int maxIter
);
static Ddi_Bdd_t *bmcAbstrCheck(
  Ddi_Bddarray_t * delta,
  Ddi_Bdd_t * cone,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * coneAndInit,
  Ddi_Bddarray_t * initStub,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * pi,
  int maxBound,
  Ddi_Varset_t * psRefNew,
  int *prevSatBound
);
static Ddi_Varset_t *refineWithCex(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * coneAndInit,
  Ddi_Bdd_t * cex,
  Ddi_Varset_t * psIn,
  Ddi_Varset_t * psOut
);
static Ddi_Bdd_t *bmcPba(
  Ddi_Bddarray_t * delta,
  Ddi_Bdd_t * cone,
  Ddi_Bdd_t * init,
  Ddi_Bddarray_t * initStub,
  Ddi_Varset_t * piv,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * pi,
  int maxBound,
  Ddi_Varset_t * psRefNew
);
static int
retimePeriferalLatches(
  Fsm_Mgr_t * fsmMgr,
  int force
);

static int
addCutLatches(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t *cuts,
  int retimePis
);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Fsm_AigCexToRefPis(
  Ddi_Bdd_t * tfCex,
  int jStub,
  int jTe,
  int phaseAbstr,
  int phase2
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(tfCex);
  Ddi_Bdd_t *cexPart = Ddi_AigPartitionTop(tfCex, 0);
  Ddi_Bdd_t *normCex = Ddi_BddMakeConstAig(ddm, 1);
  int j, k, n = Ddi_BddPartNum(cexPart);
  static char buf[1000] = "";

  //if (Ddi_BddIsOne(tfCex)) continue;
  for (j = 0; j < n; j++) {
    Ddi_Bdd_t *lit, *p_i = Ddi_BddPartRead(cexPart, j);
    Ddi_Var_t *v_i, *v_i_tf = Ddi_BddTopVar(p_i);
    char *buf2 = NULL, *buf3 = NULL;
    int val = Ddi_BddIsComplement(p_i) ? -1 : 1;

    strcpy(buf, Ddi_VarName(v_i_tf));

    if (jStub >= 0) {
      int istub;

      if (strncmp(buf, "PDT_STUB_PI_", 12) == 0) {
        buf2 = Pdtutil_StrSkipPrefix(buf, "PDT_STUB_PI_");
        Pdtutil_Assert(buf2 != NULL, "missing stub vars in cex");
        istub = Pdtutil_StrRemoveNumSuffix(buf2, '_');
        Pdtutil_Assert(istub >= 0, "wrong stub index");
        if (istub != jStub)
          continue;
      }
      else {
        Pdtutil_Assert(strncmp(buf, "PDT_STUB_PS_", 12) == 0,
                       "missing stub state vars in cex");
        if (jStub==0) {
          Ddi_BddAndAcc(normCex, p_i);
        }
        continue;
      }
    } else {
      buf2 = Pdtutil_StrSkipPrefix(buf, "PDT_TF_VAR_");
      if (buf2 != NULL) {
        Pdtutil_StrRemoveNumSuffix(buf2, '_');
      } else {
        buf2 = Pdtutil_StrSkipPrefix(buf, "PDT_BWD_TF_VAR_");
        if (buf2 != NULL) {
          Pdtutil_StrRemoveNumSuffix(buf2, '_');
        } else {
	  buf2 = Pdtutil_StrSkipPrefix(buf, "PDT_BMC_TARGET_CONE_PI_");
	  if (buf2 != NULL) {
	    Pdtutil_StrRemoveNumSuffix(buf2, '_');
	  } else {
	    buf2 = buf;
	  }
	}
      }
    }
    if (phaseAbstr) {
      buf3 = Pdtutil_StrSkipPrefix(buf2, "PDT_ITP_TWOPHASE_PI_");
      if (buf3 == NULL) {
        // skip base vars in second phase 
        if (phase2 == 1)
          continue;
      } else {
        int s = Pdtutil_StrRemoveNumSuffix(buf3, '_');

        Pdtutil_Assert(s >= 0, "wrong phase var suffix");
        // skip aux vars in first phase 
        if (phase2 == 0)
          continue;
        buf2 = buf3;
      }
    }
    buf3 = Pdtutil_StrSkipPrefix(buf2, "PDT_TARGET_EN_PI_");
    if (buf3 != NULL) {
      if (jTe<0) {
	/* skip TE vars */
	continue;
      }
      /* use te vars */
      Pdtutil_StrRemoveNumSuffix(buf3, '_');
      buf2 = buf3;
    } else {
      if (jTe>=0) {
	/* skip non TE vars */
	continue;
      }
    }
    v_i = Ddi_VarFromName(ddm, buf2);

    Pdtutil_Assert(v_i != NULL, "error looking for ref PI");
    Pdtutil_Assert(Ddi_BddSize(p_i) == 1, "not a cube");

    lit = Ddi_BddMakeLiteralAig(v_i, val > 0 ? 1 : 0);
    Ddi_BddAndAcc(normCex, lit);
    Ddi_Free(lit);
  }
  Ddi_Free(cexPart);

  return normCex;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
int
Fsm_CexNormalize(
  Fsm_Mgr_t * fsmMgr
)
{
  int pi = Ddi_VararrayNum(Fsm_MgrReadVarI(fsmMgr));
  int ps = Ddi_VararrayNum(Fsm_MgrReadVarPS(fsmMgr));
  int i = 0, iOut = 0;
  int useInitStub = 0;
  int targetEnSteps = fsmMgr->stats.targetEnSteps;
  Ddi_Vararray_t *vpi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *vps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);
  int nStubIter = fsmMgr->stats.initStubSteps;
  int phaseAbstrStub = fsmMgr->stats.initStubStepsAtPhaseAbstr;
  int phaseAbstr = fsmMgr->stats.phaseAbstr;
  Ddi_Bdd_t *cex = Fsm_MgrReadCexBDD(fsmMgr);
  Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
  Ddi_Var_t *cvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
  int cexStart = 0;
  int targetInPhase = 0;

  if (cex == NULL)
    return 1;

  Ddi_Bdd_t *myCex = Ddi_BddDup(cex);
  Ddi_Bdd_t *cexOut = Ddi_BddMakePartConjVoid(ddm);

  Pdtutil_Array_t *vpsInteger = Ddi_AigCubeMakeIntegerArrayFromBdd(init, vps);
  int part;

  useInitStub = Fsm_MgrReadInitStubBDD(fsmMgr) != NULL;
  targetEnSteps = Fsm_MgrReadTargetEnSteps(fsmMgr);

  if (Ddi_BddPartNum(cex) > 0) {
    Ddi_Bdd_t *cex0 = Ddi_BddPartRead(myCex, 0);
    char *name = Ddi_ReadName(cex0);

    if (name != NULL && strcmp(name, "PDT_CEX_IS") == 0) {
      cexStart = 1;
      Ddi_BddPartWrite(cexOut, iOut++, cex0);
    }
  }

  if (useInitStub) {
    Ddi_Bdd_t *tfCex = Ddi_BddPartRead(myCex, cexStart);

    if (fsmMgr->initStubPiConstr != NULL) {
      Ddi_BddAndAcc(tfCex, fsmMgr->initStubPiConstr);
    }
    Pdtutil_Assert(nStubIter > 0, "Missing stub iteration #");
    for (i = 1; i < nStubIter; i++) {
      Ddi_BddPartInsert(myCex, cexStart + i, tfCex);
    }
  }

  part = Ddi_BddPartNum(myCex);

  for (i = cexStart; i < part; i++) {
    int j, nLocalIter = 1;
    int jStub = (i < cexStart + nStubIter) ? (i-cexStart) : -1;
    int myPhAbstr = phaseAbstr && (i >= cexStart + phaseAbstrStub);

    if (myPhAbstr) {
      nLocalIter = 2;
      if (targetEnSteps >= 0) targetInPhase = 1;
    }

    for (j = 0; j < nLocalIter; j++) {
      int phase2 = j > 0;
      Ddi_Bdd_t *tfCex = Ddi_BddPartRead(myCex, i);
      Ddi_Bdd_t *normCex = Fsm_AigCexToRefPis(tfCex, jStub, -1, myPhAbstr, phase2);

      Ddi_BddPartWrite(cexOut, iOut++, normCex);
      Ddi_Free(normCex);
    }
  }

  while (--targetEnSteps >= 0) {
    Ddi_Bdd_t *tfCex = Ddi_BddPartRead(myCex, part-1);
    Ddi_Bdd_t *normCex = Fsm_AigCexToRefPis(tfCex, -1, 0, targetInPhase, 1);
    Pdtutil_Assert(targetEnSteps==0,
		   "multiple TE steps not supported in cex");

    Ddi_BddPartWrite(cexOut, iOut++, normCex);
    Ddi_Free(normCex);
  }

  Pdtutil_Assert((Ddi_BddPartNum(cexOut) == (nStubIter + (part - nStubIter) *
                 phaseAbstr) ? 2 : 1), "wrong cex len");
  Ddi_Free(myCex);

  if (fsmMgr->retimedPis != NULL) {
    int j;
    Ddi_Varsetarray_t *rPis = fsmMgr->retimedPis;
    Ddi_Bdd_t *myOne = Ddi_BddMakeConstAig(ddm, 1);
    int nr = Ddi_VarsetarrayNum(rPis);

    for (i = cexStart+1; i < Ddi_BddPartNum(cexOut); i++) {
      Ddi_Bdd_t *tfCex = Ddi_BddPartRead(cexOut, i);
      Ddi_Bdd_t *cexPart = Ddi_AigPartitionTop(tfCex, 0);

      Ddi_DataCopy(tfCex, myOne);
      int n = Ddi_BddPartNum(cexPart);
      if (i<=nr) {
	Ddi_Varset_t *vs_j = Ddi_VarsetarrayRead(rPis, i-1);
	Ddi_VarsetIncrMark(vs_j, 1);
      }

      for (j = 0; j < n; j++) {
        Ddi_Bdd_t *lit, *p_j = Ddi_BddPartRead(cexPart, j);
        Ddi_Var_t *v_j = Ddi_BddTopVar(p_j);
        int m = Ddi_VarReadMark(v_j);

        if (m > 0) {
          Pdtutil_Assert(m <= i, "wrong retimed pi index");
	  Ddi_Bdd_t *prevCex = Ddi_BddPartRead(cexOut, i - m);
	  Ddi_BddAndAcc(prevCex, p_j);
        } else {
          Ddi_BddAndAcc(tfCex, p_j);
        }
      }
      Ddi_Free(cexPart);
    }

    for (j = 0; j < Ddi_VarsetarrayNum(rPis); j++) {
      Ddi_Varset_t *vs_j = Ddi_VarsetarrayRead(rPis, j);
      Ddi_VarsetWriteMark(vs_j, 0);
    }

    Ddi_Free(myOne);
  }

  Fsm_MgrSetCexBDD(fsmMgr, cexOut);

  Ddi_Free(cexOut);

  return 0;
}

/**Function********************************************************************

  Synopsis     []

  Description  []

  SideEffects  []

  SeeAlso      []

******************************************************************************/

Fsm_Mgr_t *
Fsm_RetimeGateCuts(
  Fsm_Mgr_t * fsmMgr,
  int mode
)
{
  Fsm_Mgr_t *fsmMgrRetimed = Fsm_MgrDup(fsmMgr);
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgrRetimed);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgrRetimed);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgrRetimed);
  int minimal = 0;
  int propDecomp = 0;

  if (mode>=100) {
    mode -= 100;
    propDecomp=1;
  }
  if (mode>=10) {
    mode -= 10;
    minimal=1;
  }

  Ddi_Bddarray_t *enables = Ddi_BddarrayAlloc(ddm,0);
  if (propDecomp) {
    int iProp = Ddi_BddarrayNum(delta)-1;
    Ddi_Bdd_t *dProp = Ddi_BddarrayRead(delta,iProp);
    Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
    Ddi_Var_t *cvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
    Ddi_Bddarray_t *roots = Ddi_AigDisjDecompRoots (dProp, pvarPs, cvarPs,
                                                    0/*Ddi_BddSize(dProp)/4*/);
    Ddi_BddarrayAppend(enables,roots);
    Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
      printf("Prop decomp found %d roots\n",
             Ddi_BddarrayNum(roots));
    }
    Ddi_Free(roots);
  }

  if (mode>0) {
    Ddi_Bddarray_t *ite = Ddi_BddarrayAlloc(ddm, 0);
    Ddi_Bddarray_t *iteEnables = Ddi_FindIteFull(delta,ps,ite,-1);
    //Ddi_BddarraySortBySizeAcc (iteEnables,0);//decreasing size keeèp fanout order
    Ddi_BddarrayAppend(enables,iteEnables);
    Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
      printf("ITE search found %d enables and %d non latch ITE\n",
           Ddi_BddarrayNum(iteEnables), Ddi_BddarrayNum(ite));
    }
    Ddi_Free(iteEnables);
    if (!minimal)
      Ddi_BddarrayAppend(enables,ite);
    Ddi_Free(ite);
  }
  if (mode > 1) {
    Ddi_Bddarray_t *xors = Ddi_AigarrayFindXors(delta,NULL,minimal);
    Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
      printf("XOR search found %d xors\n",
           Ddi_BddarrayNum(xors));
    }
    if (mode > 2)
      Ddi_DataCopy(enables,xors);
    else
      Ddi_BddarrayAppend(enables,xors);
    Ddi_Free(xors);
  }
  int ne = addCutLatches(fsmMgrRetimed,enables,0);

  Ddi_Free(enables);
  return (fsmMgrRetimed);
}


/**Function********************************************************************

  Synopsis     []

  Description  []

  SideEffects  []

  SeeAlso      []

******************************************************************************/

Fsm_Mgr_t *
Fsm_RetimeForRACompute(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  Fsm_Retime_t * retimeStrPtr
)
{
  Fsm_Mgr_t *fsmMgrRetimed;

  Fsm_EnableDependencyCompute(fsmMgr, retimeStrPtr);

  Fsm_CommonEnableCompute(fsmMgr, retimeStrPtr);

  Fsm_RetimeCompute(fsmMgr, retimeStrPtr);

  fsmMgrRetimed = Fsm_RetimeApply(fsmMgr, retimeStrPtr);

  return (fsmMgrRetimed);
}

/**Function********************************************************************

  Synopsis     []

  Description  []

  SideEffects  []

  SeeAlso      []

******************************************************************************/

void
Fsm_EnableDependencyCompute(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  Fsm_Retime_t * retimeStrPtr   /* Retime Structure */
)
{
  Ddi_Bddarray_t *delta;
  Ddi_Vararray_t *piarray;
  Ddi_Vararray_t *sarray;
  Ddi_Vararray_t *yarray;
  Ddi_Var_t *s, *y;
  Ddi_Varset_t *supp;
  int i, ns;
  Ddi_Mgr_t *dd;
  Ddi_Bdd_t *d, *t, *en, *enbar, *tmp, *ten, *tenbar, *yeqs;

  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  piarray = Fsm_MgrReadVarI(fsmMgr);
  sarray = Fsm_MgrReadVarPS(fsmMgr);
  yarray = Fsm_MgrReadVarNS(fsmMgr);
  ns = Ddi_BddarrayNum(delta);

  dd = Ddi_ReadMgr(Ddi_BddarrayRead(delta, 0));

  retimeStrPtr->dataArray = Ddi_BddarrayAlloc(dd, ns);
  retimeStrPtr->enableArray = Ddi_BddarrayAlloc(dd, ns);

  for (i = 0; i < ns; i++) {

    s = Ddi_VararrayRead(sarray, i);
    y = Ddi_VararrayRead(yarray, i);
    d = Ddi_BddarrayRead(delta, i);

#if 0
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "VAR: %s ", Ddi_VarName(s))
      );
#endif

    t = Ddi_BddXnor((tmp = Ddi_BddMakeLiteral(y, 1)), d);
    Ddi_Free(tmp);
    supp = Ddi_VarsetVoid(dd);
    Ddi_VarsetAddAcc(supp, s);
    ten = Ddi_BddForall(t, supp);

#if 0
    aux = Ddi_BddSupp(ten);
    Ddi_VarsetPrint(aux, 1, NULL, stdout);
    Ddi_BddPrintCubes(ten, aux, 100, NULL, stdout);
#endif

    tenbar = Ddi_BddAnd(t, (tmp = Ddi_BddNot(ten)));
    Ddi_Free(tmp);

    yeqs = Ddi_BddMakeLiteral(y, 1);
    yeqs = Ddi_BddEvalFree(Ddi_BddXnor((tmp =
          Ddi_BddMakeLiteral(s, 1)), yeqs), yeqs);
    Ddi_Free(tmp);

#if 1
    if (Ddi_BddIncluded(tenbar, yeqs)) {
#else
    if (0) {
#endif
#if 0
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "Included.\n")
        );
#endif
      Ddi_VarsetAddAcc(supp, y);
      enbar = Ddi_BddExist(tenbar, supp);
      en = Ddi_BddNot(enbar);
      Ddi_Free(enbar);
      d = Ddi_BddConstrain(ten, (tmp = Ddi_BddMakeLiteral(y, 1)));
      Ddi_Free(tmp);
      Ddi_BddConstrainAcc(d, en);
    } else {
#if 0
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "Not Included.\n")
        );
      d = Ddi_BddAnd(tenbar, tmp = Ddi_BddNot(yeqs));
      aux = Ddi_BddSupp(d);
      Ddi_VarsetPrint(aux, 1, NULL, stdout);
      Ddi_BddPrintCubes(d, aux, 100, NULL, stdout);
#endif

      en = Ddi_BddDup(Ddi_MgrReadOne(dd));
      d = Ddi_BddDup(d);
    }
    Ddi_Free(ten);
    Ddi_Free(tenbar);
    Ddi_Free(t);
    Ddi_Free(supp);

    Ddi_BddarrayWrite(retimeStrPtr->dataArray, i, d);
    Ddi_BddarrayWrite(retimeStrPtr->enableArray, i, en);

    Ddi_Free(d);
    Ddi_Free(en);

  }

  return;
}

/**Function********************************************************************

  Synopsis     []

  Description  []

  SideEffects  []

  SeeAlso      []

******************************************************************************/

void
Fsm_CommonEnableCompute(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  Fsm_Retime_t * retimeStrPtr   /* Retime Structure */
)
{
  Ddi_Bddarray_t *delta;
  Ddi_Vararray_t *piarray;
  Ddi_Vararray_t *sarray;
  Ddi_Vararray_t *yarray;
  int i, j, ns;
  Ddi_Mgr_t *dd;

  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  piarray = Fsm_MgrReadVarI(fsmMgr);
  sarray = Fsm_MgrReadVarPS(fsmMgr);
  yarray = Fsm_MgrReadVarNS(fsmMgr);
  ns = Ddi_BddarrayNum(delta);

  dd = Ddi_ReadMgr(Ddi_BddarrayRead(delta, 0));

  retimeStrPtr->set = Pdtutil_Alloc(int,
    ns
  );
  retimeStrPtr->refEn = Pdtutil_Alloc(int,
    ns
  );
  retimeStrPtr->enCnt = Pdtutil_Alloc(int,
    ns
  );

  for (i = 0; i < ns; i++) {
    retimeStrPtr->set[i] = 1;
    retimeStrPtr->refEn[i] = i;
    retimeStrPtr->enCnt[i] = 1;
  }

  for (i = 0; i < ns; i++) {
    if (retimeStrPtr->set[i]) {
      for (j = i + 1; j < ns; j++) {
        if ((retimeStrPtr->set[j]) && (retimeStrPtr->enCnt[j]) &&
          (Ddi_BddEqual(Ddi_BddarrayRead(retimeStrPtr->enableArray, i),
              Ddi_BddarrayRead(retimeStrPtr->enableArray, j)))) {
          retimeStrPtr->enCnt[j] = 0;
          retimeStrPtr->enCnt[i]++;
          retimeStrPtr->refEn[j] = i;
        }
      }
    }
  }

  return;
}

/**Function********************************************************************

  Synopsis     []

  Description  []

  SideEffects  []

  SeeAlso      []

******************************************************************************/

void
Fsm_RetimeCompute(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  Fsm_Retime_t * retimeStrPtr   /* Retime Structure */
)
{
  Ddi_Mgr_t *dd;
  Ddi_Bddarray_t *delta;
  Ddi_Vararray_t *piarray, *sarray, *yarray;
  Ddi_Var_t *s;
  Ddi_Varset_t *supp, *aux, *aux0, *latches, *inputs,
    *retimed, *notRetimed, *retimedInputs;
  Ddi_Varset_t **deltaSupp, **enableSupp, **retimeLatchSets, **auxSupp;
  Ddi_Bdd_t *en, *enk, *common;
  Ddi_Vararray_t *auxVararray;
  Ddi_Var_t *auxVar;
  Ddi_Bddarray_t *auxFunArray;
  Pdtutil_VerbLevel_e verbosity;
  int *retimeDeltas, *removableLatches, *appear, *disappear;
  int i, j, k, ns, max, imax, nret, nAuxVar, again;

  verbosity = Fsm_MgrReadVerbosity(fsmMgr);

  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  piarray = Fsm_MgrReadVarI(fsmMgr);
  sarray = Fsm_MgrReadVarPS(fsmMgr);
  yarray = Fsm_MgrReadVarNS(fsmMgr);
  ns = Ddi_BddarrayNum(delta);

  dd = Ddi_ReadMgr(Ddi_BddarrayRead(delta, 0));

  max = 0;
  imax = -1;

  removableLatches = Pdtutil_Alloc(int,
    ns
  );
  retimeDeltas = Pdtutil_Alloc(int,
    ns
  );

  retimeLatchSets = Pdtutil_Alloc(Ddi_Varset_t *, ns);
  inputs = Ddi_VarsetMakeFromArray(piarray);
  deltaSupp = Ddi_BddarraySuppArray(retimeStrPtr->dataArray);
  enableSupp = Ddi_BddarraySuppArray(retimeStrPtr->enableArray);

  nAuxVar = Fsm_MgrReadNAuxVar(fsmMgr);
  if (nAuxVar > 0) {
    auxFunArray = Fsm_MgrReadAuxVarBDD(fsmMgr);
    auxVararray = Fsm_MgrReadVarAuxVar(fsmMgr);
    auxSupp = Ddi_BddarraySuppArray(auxFunArray);

    do {
      again = 0;
      for (i = 0; i < nAuxVar; i++) {
        auxVar = Ddi_VararrayRead(auxVararray, i);
        for (j = 0; j < ns; j++) {
          Pdtutil_Assert((!Ddi_VarInVarset(enableSupp[j], auxVar)),
            "NOT supported aux VAR in enable function");
          supp = deltaSupp[j];
          if (Ddi_VarInVarset(supp, auxVar)) {
            aux = Ddi_VarsetUnion(supp, auxSupp[i]);
            Ddi_Free(supp);
            deltaSupp[j] = Ddi_VarsetRemove(aux, auxVar);
            Ddi_Free(aux);
            again = 1;
          }
        }
      }
    } while (again);
  }

  Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "DATA and ENABLE supports.\n"); for (i = 0; i < ns; i++) {
    s = Ddi_VararrayRead(sarray, i);
      fprintf(stdout, "S[%d]: %s.\n", i, Ddi_VarName(s));
      fprintf(stdout, "D[%d].\n", i);
      Ddi_VarsetPrint(deltaSupp[i], 1, NULL, stdout);
      fprintf(stdout, "E[%d].\n", i);
      Ddi_VarsetPrint(enableSupp[i], 1, NULL, stdout);
      fprintf(stdout, "------------------------------.\n");}
    fprintf(stdout, "DATA and ENABLE supports - END.\n")
    ) ;

  /* allocate data structures to generate retimed FSM */

  appear = Pdtutil_Alloc(int,
    ns
  );
  disappear = Pdtutil_Alloc(int,
    ns
  );

  for (i = 0; i < ns; i++) {
    appear[i] = -1;
    disappear[i] = 0;
  }

  for (i = 0; i < ns; i++) {
    retimeLatchSets[i] = NULL;
    if ((retimeStrPtr->set[i]) && (retimeStrPtr->refEn[i] == i)) {
      /* this is a set of latches with common enable */
      /* initialize set with reference latch */
      latches = Ddi_VarsetVoid(dd);
      s = Ddi_VararrayRead(sarray, i);
      Ddi_VarsetAddAcc(latches, s);

      if (retimeStrPtr->enCnt[i] > max) {
        max = retimeStrPtr->enCnt[i];
        imax = i;
      }
      en = Ddi_BddarrayRead(retimeStrPtr->enableArray, i);

      /* now add latches with common enable */

      Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "Reference: %d (size: %d).\n",
          i, retimeStrPtr->enCnt[i])
        );
      for (j = 0; j < ns; j++) {
        if ((retimeStrPtr->set[j]) && (retimeStrPtr->refEn[j] == i)) {
          s = Ddi_VararrayRead(sarray, j);
          Ddi_VarsetAddAcc(latches, s);

          Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "%s ", Ddi_VarName(s))
            );
        }
      }
      Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "\n")
        );

      retimeLatchSets[i] = latches;

      Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "Latches: "); Ddi_VarsetPrint(latches, 1, NULL, stdout)
        );

      /* search candidates for retiming: deltas with support including
         PIs and FFs in set "latches". Other latches are allowed
         with enable excluded by enabling function of present set ("latches")
       */

      for (j = 0; j < ns; j++) {
        supp = deltaSupp[j];
        retimeDeltas[j] = 0;
        aux0 = Ddi_VarsetDiff(supp, inputs);
        if (!Ddi_VarsetIsVoid(aux0)) {
          /* isolate FFs not in "latches" */
          aux = Ddi_VarsetDiff(aux0, latches);

          /* support of delta is in latches or inputs with the exception
             of latches disabled when "latches" enabled */

          if (Ddi_VarsetIsVoid(aux)) {
            /* support of delta is in latches or inputs */
            retimeDeltas[j] = 1;
          } else if (Ddi_VarsetNum(aux) < Ddi_VarsetNum(aux0)) {
            /* support of delta is in latches or inputs plus other latches */

            retimeDeltas[j] = 2;
            /* check enable of other latches: null intersection with present
               enable */
            while (!Ddi_VarsetIsVoid(aux)) {
              for (k = 0; k < ns; k++) {
                s = Ddi_VararrayRead(sarray, k);
                if (Ddi_VarIndex(s) == Ddi_VarIndex(Ddi_VarsetTop(aux))) {
                  enk = Ddi_BddarrayRead(retimeStrPtr->enableArray, k);
                  common = Ddi_BddAnd(en, enk);
                  if (!Ddi_BddIsZero(common)) {
                    retimeDeltas[j] = 0;
                    Ddi_Free(common);
                    break;
                  } else
                    Ddi_Free(common);
                }
              }
              Ddi_VarsetNextAcc(aux);
            }
          }
          Ddi_Free(aux);

          if (retimeDeltas[j] != 0) {
            Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMax_c,
              s = Ddi_VararrayRead(sarray, j);
              fprintf(stdout, "DeltaSupp[%d](%s): ", j, Ddi_VarName(s));
              Ddi_VarsetPrint(supp, 1, NULL, stdout););
          }
        }
        Ddi_Free(aux0);
      }

      notRetimed = Ddi_BddarraySupp(retimeStrPtr->enableArray);
      for (j = 0; j < ns; j++) {
        if (retimeDeltas[j] == 0) {

          Ddi_VarsetUnionAcc(notRetimed, deltaSupp[j]);
        }
      }

      Fsm_OptimalRetimingCompute(dd, verbosity, retimeStrPtr->retimeEqualFlag,
        retimeDeltas, deltaSupp, retimeStrPtr->enableArray, en, sarray, ns,
        latches, inputs, notRetimed);

      Ddi_Free(notRetimed);

      /* setup data for true retiming */

      retimed = Ddi_VarsetVoid(dd);
      /* notRetimed = Ddi_VarsetVoid(dd); */
      notRetimed = Ddi_BddarraySupp(retimeStrPtr->enableArray);

      for (j = 0, nret = 0; j < ns; j++) {
        if ((retimeDeltas[j]) && (appear[j] < 0)) {

          Ddi_VarsetUnionAcc(retimed, deltaSupp[j]);
          nret++;
        } else {

          Ddi_VarsetUnionAcc(notRetimed, deltaSupp[j]);
        }
      }
      retimedInputs = Ddi_VarsetIntersect(retimed, inputs);

      Ddi_VarsetIntersectAcc(retimed, latches);

      if (!Ddi_VarsetIsVoid(retimed)) {

        aux = Ddi_VarsetIntersect(retimed, notRetimed);
        aux0 = Ddi_VarsetIntersect(retimedInputs, notRetimed);
        if (Ddi_VarsetIsVoid(aux) && Ddi_VarsetIsVoid(aux0)) {
          Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "Full Retiming Done: %d -> %d, gain = %d.\n",
              Ddi_VarsetNum(retimed), nret, Ddi_VarsetNum(retimed) - nret)
            );

          for (j = 0; j < ns; j++) {
            s = Ddi_VararrayRead(sarray, j);
            if (Ddi_VarInVarset(retimed, s)) {
              disappear[j] = 1;
            }
            if ((retimeDeltas[j]) && (appear[j] < 0)) {
              appear[j] = i;
            }
          }

        } else {
          Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout,
              "Partial Retiming Done: %d[shL: %d / shI: %d] -> %d, gain = %d.\n",
              Ddi_VarsetNum(retimed), Ddi_VarsetNum(aux), Ddi_VarsetNum(aux0),
              nret, Ddi_VarsetNum(retimed) - nret)
            );
        }
        Ddi_Free(aux);
        Ddi_Free(aux0);
      } else {
        Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "All Retimed Latches are Shared.\n")
          );
      }

      Ddi_Free(retimed);
      Ddi_Free(notRetimed);

    }

  }

  if (retimeStrPtr->removeLatches == 1) {
    Fsm_FindRemovableLatches(dd, removableLatches, delta, sarray, inputs, ns);
  }

  retimeStrPtr->retimeVector = Pdtutil_Alloc(int,
    ns
  );

  retimeStrPtr->retimeGraph = Pdtutil_Alloc(Ddi_Bddarray_t *, ns);

  for (j = 0; j < ns; j++) {
    retimeStrPtr->retimeGraph[j] = Ddi_BddarrayAlloc(dd, 0);
    if ((disappear[j] == 0) &&
      ((retimeStrPtr->removeLatches == 0) || (removableLatches[j] == 0))) {
      /* keep current latch */
      Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "RA-INS[%d].\n", j)
        );
      en = Ddi_BddarrayRead(retimeStrPtr->enableArray, j);
      Ddi_BddarrayInsert(retimeStrPtr->retimeGraph[j], 0, en);
    }
    if (appear[j] >= 0) {
      /* a new retimed latch appears */
      Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "RA-INS2[%d].\n", j)
        );
      Pdtutil_Assert((appear[j] >= 0)
        && (appear[j] < ns), "invalid retime ID");
      en = Ddi_BddarrayRead(retimeStrPtr->enableArray, appear[j]);
      Ddi_BddarrayInsert(retimeStrPtr->retimeGraph[j], 0, en);
      retimeStrPtr->retimeVector[j] = 1;
    } else {
      retimeStrPtr->retimeVector[j] = 0;
    }
  }

  return;
}


/**Function*******************************************************************

  Synopsis    []

  Description []

  SideEffects [none]

  SeeAlso     []

*****************************************************************************/

void
Fsm_OptimalRetimingCompute(
  Ddi_Mgr_t * dd,
  Pdtutil_VerbLevel_e verbosity,
  int retimeEqual,
  int *retimeDeltas,
  Ddi_Varset_t ** deltaSupp,
  Ddi_Bddarray_t * enableArray,
  Ddi_Bdd_t * en,
  Ddi_Vararray_t * sarray,
  int ns,
  Ddi_Varset_t * latches,
  Ddi_Varset_t * inputs,
  Ddi_Varset_t * notRetimed
)
{
  int i, again, nret;
  int *retime0;
  Ddi_Var_t *s;
  Ddi_Varset_t *aux, *joinsupp = NULL, *common;

  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "Computing Optimal Retiming.\n")
    );

  retime0 = Pdtutil_Alloc(int,
    ns
  );

  common = Ddi_VarsetUnion(inputs, latches);

  for (i = 0; i < ns; i++) {
    retime0[i] = 0;
  }

  again = 1;
  nret = 0;
  while (again) {
    for (i = 0, again = 0; i < ns; i++) {
      if (retimeDeltas[i]) {
        if (joinsupp == NULL) {
          joinsupp = Ddi_VarsetIntersect(deltaSupp[i], common);
        }
        Pdtutil_Assert(deltaSupp[i] != NULL, "NULL deltasupp");
        aux = Ddi_VarsetIntersect(deltaSupp[i], joinsupp);
        if (!Ddi_VarsetIsVoid(aux)) {
          /* a common support exists */
          retime0[i] = retimeDeltas[i];
          retimeDeltas[i] = 0;
          nret++;

          Ddi_VarsetUnionAcc(joinsupp, deltaSupp[i]);

          Ddi_VarsetIntersectAcc(joinsupp, common);
          again = 1;
        }
        Ddi_Free(aux);

      }
    }
  }

  if (nret) {
    /* progress: some retime attempted */

    /* recur */
    Fsm_OptimalRetimingCompute(dd, verbosity, retimeEqual, retimeDeltas,
      deltaSupp, enableArray, en, sarray, ns, latches, inputs, notRetimed);

    /* check whether retiming local set decreases latch num */
    Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "#Latches %d - #Deltas: %d.\n",
        Ddi_VarsetNum(joinsupp), nret)
      );

    aux = Ddi_VarsetIntersect(joinsupp, notRetimed);
    if ((Ddi_VarsetIsVoid(aux)) && (
        ((retimeEqual == 2)) ||
        ((retimeEqual == 1) && (Ddi_VarsetNum(joinsupp) >= nret)) ||
        ((retimeEqual == 0) && (Ddi_VarsetNum(joinsupp) > nret))
      )) {

      /* more input latches than output ones */
      Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "Retiming Group: ")
        );

      for (i = 0; i < ns; i++) {
        switch (retime0[i]) {
          case 1:
            retimeDeltas[i] = 1;
            s = Ddi_VararrayRead(sarray, i);
            Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
              fprintf(stdout, "[%s] ", Ddi_VarName(s))
              );
            break;
          case 2:
            retimeDeltas[i] = 1;
            s = Ddi_VararrayRead(sarray, i);
            Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
              fprintf(stdout, "{%s} ", Ddi_VarName(s))
              );
            break;
        }
      }
      Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "Retimed latches:.\n")
        );
      Ddi_VarsetPrint(joinsupp, 1, NULL, stdout);
    } else {
#if 0
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "NO Retiming for group: ")
        );
      Ddi_VarsetPrint(latches, 1, NULL, stdout);
#endif
    }
    Ddi_Free(aux);
  }

  Pdtutil_Free(retime0);

  Ddi_Free(common);

  return;
}



/**Function*******************************************************************

  Synopsis    []

  Description []

  SideEffects [none]

  SeeAlso     []

*****************************************************************************/

void
Fsm_FindRemovableLatches(
  Ddi_Mgr_t * dd,
  int *removableLatches,
  Ddi_Bddarray_t * delta,
  Ddi_Vararray_t * sarray,
  Ddi_Varset_t * inputs,
  int ns
)
{
  Ddi_Var_t *s;
  Ddi_Varset_t *supp, *supp2, *aux, *aux2;
  int j, k, again, remove, rinp, self, inp;
  Ddi_Varset_t **deltaSupp;

  remove = 0;
  rinp = 0;
  inp = 0;
  self = 0;

  for (j = 0; j < ns; j++) {
    removableLatches[j] = 0;
  }

#if 1
  deltaSupp = Ddi_BddarraySuppArray(delta);

  for (j = 0; j < ns; j++) {

    s = Ddi_VararrayRead(sarray, j);
    supp = deltaSupp[j];

    aux = Ddi_VarsetDiff(supp, inputs);
    if (Ddi_VarsetIsVoid(aux)) {
      inp++;
      removableLatches[j] = 1;
    }

    Ddi_Free(aux);

  }

  do {

    again = 0;
    for (j = 0; j < ns; j++) {

      if (removableLatches[j] == 1) {

        supp = deltaSupp[j];

        for (k = 0; ((k < ns) && (removableLatches[j])); k++) {
          if (removableLatches[k] == 0) {
            supp2 = deltaSupp[k];
            aux2 = Ddi_VarsetIntersect(supp2, supp);
            if (!Ddi_VarsetIsVoid(aux2)) {
              removableLatches[j] = 0;
              again = 1;
            }
            Ddi_Free(aux2);
          }
        }

      }
    }

  } while (again);


  for (j = 0; j < ns; j++) {
    if (removableLatches[j] == 1) {
      rinp++;
    }
  }

#endif

#if 0
  Ddi_Bdd_t *d, *s0, *s1, *eq, *totDep, *dep, *cof0, *cof1;

  //else
  if (0) {

    s0 = Ddi_BddMakeLiteral(s, 0);
    s1 = Ddi_BddMakeLiteral(s, 1);
    eq = Ddi_BddXnor(s1, d);

    totDep = Ddi_MgrReadZero(dd);

    for (k = 0; k < ns; k++) {
      if (k != j) {
        d = Ddi_BddarrayRead(delta, k);
        cof0 = Ddi_BddConstrain(d, s0);
        cof1 = Ddi_BddConstrain(d, s1);
        dep = Ddi_BddXor(cof0, cof1);
        Ddi_Free(cof0);
        Ddi_Free(cof1);
        Ddi_BddOrAcc(totDep, dep);
        Ddi_Free(dep);
      }
    }

    Ddi_Free(s0);
    Ddi_Free(s1);

    if (Ddi_BddIncluded(totDep, eq)) {
      remove++;
      removableLatches[j] = 1;
    }

    Ddi_Free(eq);
    Ddi_Free(totDep);

  }


  if ((removableLatches[j] == 1) && (Ddi_VarInVarset(supp, s))) {
    self++;
  }

  //}
#endif

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "REMOVABLE LATCHES: (%d+%d/%d)(self: %d)/%d.\n",
      remove, rinp, inp, self, ns)
    );

  return;
}

/**Function********************************************************************

  Synopsis     [Given a Retiming Array Apply it to the FSM Manager.
    Return a new FSM Manager.]

  Description  [There should be a 1 to 1 correspondence between the
    retiming array (retimeStrPtr->retimeArray) and the delta array.]

  SideEffects  []

  SeeAlso      []

******************************************************************************/

Fsm_Mgr_t *
Fsm_RetimeApply(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  Fsm_Retime_t * retimeStrPtr   /* Retime Structure */
)
{
  Ddi_Mgr_t *ddMgr;
  Ddi_Bddarray_t *deltaOldArray, *deltaArray, *dataArray, *enableArray,
    *auxFunArray;
  Ddi_Bddarray_t **retimeGraph, *retimeArray, *recodeDelta;
  Ddi_Vararray_t *psOldArray, *nsOldArray, *psArray, *nsArray, *auxVararray;
  Ddi_Var_t *auxVar, *psVar, *nsVar;
  Ddi_Bdd_t *one, *delta, *data, *enable, *lit = NULL, *s0, *recodeTR;
  Fsm_Mgr_t *fsmMgrRetimed;
  Ddi_Varset_t *quantify, *pis;
  int i, j, level, level1, psOldNumber, psNumber, nFF, auxVarNumber;
  char varname[FSM_MAX_STR_LEN];

  Ddi_Mgr_t *dd = Fsm_MgrReadDdManager(fsmMgr);

  /*----------------------- Take Fields From FSM Manager --------------------*/

  ddMgr = Fsm_MgrReadDdManager(fsmMgr);

  one = Ddi_MgrReadOne(ddMgr);

  psOldArray = Fsm_MgrReadVarPS(fsmMgr);
  nsOldArray = Fsm_MgrReadVarNS(fsmMgr);
  psOldNumber = Fsm_MgrReadNL(fsmMgr);
  deltaOldArray = Fsm_MgrReadDeltaBDD(fsmMgr);

  /*---------------------- Make a Copy of the FSM Manager -------------------*/

  fsmMgrRetimed = Fsm_MgrDup(fsmMgr);

  /*----------- PATCH:  Set Fields I Want into Retime Structure -------------*/

#if 0
  retimeStrPtr->dataArray = Ddi_BddarrayAlloc(psOldNumber);
  retimeStrPtr->enableArray = Ddi_BddarrayAlloc(psOldNumber);
  retimeStrPtr->retimeGraph = Pdtutil_Alloc(Ddi_Bddarray_t *, psOldNumber);

  for (i = 0; i < psOldNumber; i++) {
    delta = Ddi_BddarrayRead(deltaOldArray, i);
    Ddi_BddarrayWrite(retimeStrPtr->dataArray, i, delta);
    Ddi_BddarrayWrite(retimeStrPtr->enableArray, i, one);
    retimeStrPtr->retimeGraph[i] = Ddi_BddarrayAlloc(0);
    Ddi_BddarrayWrite(retimeStrPtr->retimeGraph[i], 0, one);
  }
#endif

  /*-------------------------- Take Retiming Options ------------------------*/

  retimeGraph = retimeStrPtr->retimeGraph;
  dataArray = retimeStrPtr->dataArray;
  enableArray = retimeStrPtr->enableArray;

  /*---------------------------- Apply Retiming -----------------------------*/

  /* Auxiliary Structures */
  auxVarNumber = Fsm_MgrReadNAuxVar(fsmMgr);
  if (auxVarNumber == 0) {
    auxFunArray = Ddi_BddarrayAlloc(dd, 0);
    auxVararray = Ddi_VararrayAlloc(dd, 0);
  } else {
    auxFunArray = Ddi_BddarrayDup(Fsm_MgrReadAuxVarBDD(fsmMgr));
    auxVararray = Ddi_VararrayDup(Fsm_MgrReadVarAuxVar(fsmMgr));
  }

  /* Present State and Next State Variables, Next State Functions */
  psNumber = 0;
  deltaArray = Ddi_BddarrayAlloc(dd, 0);
  psArray = Ddi_VararrayAlloc(dd, 0);
  nsArray = Ddi_VararrayAlloc(dd, 0);

  recodeDelta = Ddi_BddarrayAlloc(dd, 0);

  /*
   *  Main Cycle
   */

  for (i = 0; i < psOldNumber; i++) {
    retimeArray = retimeGraph[i];

    nFF = Ddi_BddarrayNum(retimeArray);

    /*
     *  Empty Retime Array: Create Auxiliary Variable
     */

    if (retimeArray == NULL || nFF == 0) {
      data = Ddi_BddarrayRead(dataArray, i);
      auxVar = Ddi_VararrayRead(psOldArray, i);

      Ddi_VararrayWrite(auxVararray, auxVarNumber, auxVar);
      Ddi_BddarrayInsertLast(auxFunArray, data);

      auxVarNumber++;

      continue;
    }

    /*
     *  Create Delta Functions with psArray and nsArray
     */

    for (j = 0; j < nFF; j++) {
      enable = Ddi_BddarrayRead(retimeArray, j);

      /* First Element: Use Usual Data */
      if (j == 0) {
        data = Ddi_BddarrayRead(dataArray, i);
      } else {
        data = Ddi_BddDup(lit);
      }

      /* Last Element: Use Usual Present State Variable */
      if (j == (nFF - 1)) {
        psVar = Ddi_VararrayRead(psOldArray, i);
        nsVar = Ddi_VararrayRead(nsOldArray, i);
      } else {
        psVar = Ddi_VararrayRead(psOldArray, i);
        nsVar = Ddi_VararrayRead(nsOldArray, i);

        sprintf(varname, "PdtAuxVar_%s", Ddi_VarName(psVar));

        level = Ddi_VarCurrPos(psVar);
        level1 = Ddi_VarCurrPos(nsVar);
        if (level1 > level) {
          level = level1;
        }

        nsVar = Ddi_VarNewAtLevel(ddMgr, level + 1);
        psVar = Ddi_VarNewAtLevel(ddMgr, level + 1);

        Ddi_VarAttachName(psVar, varname);
        strcat(varname, "$NS");
        Ddi_VarAttachName(nsVar, varname);

      }

      lit = Ddi_BddMakeLiteral(psVar, 1);
      delta = Ddi_BddIte(enable, data, lit);

      Ddi_VararrayWrite(psArray, psNumber, psVar);
      Ddi_VararrayWrite(nsArray, psNumber, nsVar);
      Ddi_BddarrayInsertLast(deltaArray, delta);

      if ((j == 0) && (retimeStrPtr->retimeVector[i] == 1)) {
        Ddi_BddarrayInsertLast(recodeDelta, data);
      } else {
        Pdtutil_Assert(j == nFF - 1, "wrong data for initial state");
        Ddi_BddarrayInsertLast(recodeDelta, lit);
      }

      psNumber++;
    }
  }

  /*--------------------- Compute new init state set  ----------------------*/

  recodeTR = Ddi_BddRelMakeFromArray(recodeDelta, nsArray);

  quantify = Ddi_VarsetMakeFromArray(Fsm_MgrReadVarPS(fsmMgrRetimed));
  pis = Ddi_VarsetMakeFromArray(Fsm_MgrReadVarI(fsmMgrRetimed));
  Ddi_VarsetUnionAcc(pis, quantify);
  Ddi_Free(pis);

  Ddi_BddPartInsert(recodeTR, 0, Fsm_MgrReadInitBDD(fsmMgrRetimed));
  s0 = Part_BddMultiwayLinearAndExist(recodeTR, quantify, -1);
  Ddi_BddSwapVarsAcc(s0, psArray, nsArray);

  Ddi_Free(quantify);

  /*----------------------- Free Old Functions, ... ------------------------*/

  Ddi_Free(fsmMgrRetimed->var.i);
  Ddi_Free(fsmMgrRetimed->var.ps);
  Ddi_Free(fsmMgrRetimed->var.ns);
  Ddi_Free(fsmMgrRetimed->delta.bdd);
  Ddi_Free(fsmMgrRetimed->init.bdd);

  /*--------- Modify the FSM Manager Following Retiming Results  ------------*/

  Fsm_MgrSetVarPS(fsmMgrRetimed, psArray);
  Fsm_MgrSetVarNS(fsmMgrRetimed, nsArray);
  Fsm_MgrSetDeltaBDD(fsmMgrRetimed, deltaArray);
  Fsm_MgrSetVarAuxVar(fsmMgrRetimed, auxVararray);
  Fsm_MgrSetAuxVarBDD(fsmMgrRetimed, auxFunArray);

#if 0
  s0 = Ddi_MgrReadOne(ddMgr);
  for (i = 0; i < psNumber; i++) {
    var = Ddi_VararrayRead(psArray, i);
    lit = Ddi_BddMakeLiteral(var, 1);
    Ddi_BddAndAcc(s0, lit);
  }
#endif

  Fsm_MgrSetInitBDD(fsmMgrRetimed, s0);


#if 0
Problema:
  ora Ddi_MgrOrdWrite cerca di scrivere piu ` variabili di quante ne
    ha la FSM in quanto alcune variabili possono essere create durante
    il retiming Fsm_MgrStore(fsmMgr, "fsmOld", NULL, 1, DDDMP_MODE_TEXT, 1);
  Fsm_MgrStore(fsmMgrRetimed, "fsmNew", NULL, 1, DDDMP_MODE_TEXT, 1);
#endif

  /*------------------------- Return New FSM Manager ------------------------*/

  /*
   *  Alla fine di questo ambaradan ho una FSM con auxVar e auxFun ...
   *  devo tenere in conto di questo in tr_init o simili ...
   */

  return (fsmMgrRetimed);
}




/**Function********************************************************************

  Synopsis     [Given a Retiming Array Apply it to the FSM Manager.
    Return a new FSM Manager.]

  Description  [There should be a 1 to 1 correspondence between the
    retiming array (retimeStrPtr->retimeArray) and the delta array.]

  SideEffects  []

  SeeAlso      []

******************************************************************************/

int
Fsm_MgrRetimePis(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * stubFuncs
)
{
  int j;
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);
  Ddi_Varsetarray_t *rPis = Fsm_MgrReadRetimedPis(fsmMgr);
  Ddi_Varset_t *vs = NULL;

  if (stubFuncs == NULL || Ddi_BddarrayNum(stubFuncs) == 0)
    return 0;

  vs = Ddi_BddarraySupp(stubFuncs);
  if (rPis == NULL) {
    rPis = Ddi_VarsetarrayAlloc(ddm, 0);
    Fsm_MgrSetRetimedPis(fsmMgr, rPis);
    Ddi_Free(rPis);
    rPis = Fsm_MgrReadRetimedPis(fsmMgr);
  }
  Pdtutil_Assert(Ddi_VarsetarrayNum(rPis) < fsmMgr->stats.initStubSteps,
    "too many retimed pi sets");
  Ddi_VarsetarrayInsertLast(rPis, vs);
  Pdtutil_Assert(Ddi_VarsetarrayNum(rPis) <= fsmMgr->stats.initStubSteps,
    "wrong num of retimed pi sets");
  Ddi_Free(vs);
  return 1;
}


/**Function********************************************************************

  Synopsis     [Given a Retiming Array Apply it to the FSM Manager.
    Return a new FSM Manager.]

  Description  [There should be a 1 to 1 correspondence between the
    retiming array (retimeStrPtr->retimeArray) and the delta array.]

  SideEffects  []

  SeeAlso      []

******************************************************************************/

int
Fsm_RetimePeriferalLatchesForced(
  Fsm_Mgr_t * fsmMgr
)
{
  return retimePeriferalLatches(fsmMgr,0);
}

/**Function********************************************************************

  Synopsis     [Given a Retiming Array Apply it to the FSM Manager.
    Return a new FSM Manager.]

  Description  [There should be a 1 to 1 correspondence between the
    retiming array (retimeStrPtr->retimeArray) and the delta array.]

  SideEffects  []

  SeeAlso      []

******************************************************************************/

int
Fsm_RetimePeriferalLatches(
  Fsm_Mgr_t * fsmMgr
)
{
  return retimePeriferalLatches(fsmMgr,0);
}


/**Function********************************************************************

  Synopsis     [Given a Retiming Array Apply it to the FSM Manager.
    Return a new FSM Manager.]

  Description  [There should be a 1 to 1 correspondence between the
    retiming array (retimeStrPtr->retimeArray) and the delta array.]

  SideEffects  []

  SeeAlso      []

******************************************************************************/

Ddi_Bdd_t *
Fsm_ReduceTerminalScc(
  Fsm_Mgr_t * fsmMgr,
  int *sccNumP,
  int *provedP,
  int constrLevel
)
{
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);

  /*----------------------- Take Fields From FSM Manager --------------------*/

  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  int nstate = Ddi_VararrayNum(ps);
  int nVars = Ddi_MgrReadNumVars(ddm);
  int nRed = 0;
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);

  int i, useAig = 0, initSize, addInitStub = 0;
  int again, okInit = 1;
  Ddi_Bdd_t *sccConstr = NULL;
  int provedProperty = 0;
  int weakScc = 0;
  int enforcePropRetime = 1;

  int forceTrans = 1, forcedTrans = 0;
  int constrSize = Ddi_BddSize(Fsm_MgrReadConstraintBDD(fsmMgr));
  Ddi_Var_t *vc = Ddi_VarFromName(ddm,"AIGUNCONSTRAINT_INVALID_STATE");
  int hasConstraints = vc!=NULL || constrSize>1;
  
  /*-------------------------- Take Retiming Options ------------------------*/

  Pdtutil_Assert(nstate == Ddi_BddarrayNum(delta), "Wrong num of latches");
  Pdtutil_Assert(nstate == Ddi_VararrayNum(ps), "Wrong num of latches");
  Pdtutil_Assert(nstate == Ddi_VararrayNum(ns), "Wrong num of latches");

  useAig = Ddi_BddIsAig(Ddi_BddarrayRead(delta, 0));
  if (!useAig) {
    Ddi_AigInit(ddm, 100);
  }

  if (nstate > 30000 && !hasConstraints)
    return NULL;

  if (1) {
    Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
    Ddi_Var_t *pvarNs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$NS");
    Ddi_Var_t *cvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
    Ddi_Var_t *cvarNs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$NS");
    int iProp, iConstr = -1, iScc = -1;
    Ddi_Bdd_t *deltaProp;
    Ddi_Bdd_t *eqTr = Ddi_BddMakePartConjVoid(ddm);
    Ddi_Bdd_t *one = Ddi_BddMakeConstAig(ddm, 1);
    Ddi_Varset_t *pSupp = NULL;
    Ddi_Vararray_t *pSuppA = NULL;
    int retimeProp = 0, enSearch=1;

    for (i = 0; i < nstate; i++) {
      Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);

      if (v_i == pvarPs) {
        iProp = i;
      }
      if (v_i == cvarPs) {
        iConstr = i;
      }
    }
    Pdtutil_Assert(iProp < nstate, "PDT_BDD_INVARSPEC_VAR$PS var not found");
    Pdtutil_Assert(iConstr < nstate && iConstr >= 0,
      "PDT_BDD_INVAR_VAR$PS var not found");
    deltaProp = Ddi_BddCofactor(Ddi_BddarrayRead(delta, iProp), pvarPs, 1);
    nstate = Ddi_VararrayNum(ps);
    pSupp = Ddi_BddSupp(deltaProp);
    pSuppA = Ddi_VararrayMakeFromVarset(pSupp, 1);
    Ddi_Free(pSupp);
    for (i = 0; i < Ddi_VararrayNum(pSuppA); i++) {
      Ddi_Var_t *v = Ddi_VararrayRead(pSuppA, i);

      Pdtutil_Assert(Ddi_VarReadMark(v) == 0, "0 var mark required");
      Ddi_VarWriteMark(v, 1);
    }
    for (i = 0; i < nstate && !provedProperty && enSearch; i++) {
      int j, allowTrans = 0, transOkProp = 0;
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);
      Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);
      int scc[2];

      // skip large deltas
      if (0 && (Ddi_BddSize(d_i) > 200))
        continue;
      if (0) {
        Ddi_Bdd_t *d_i_0 = Ddi_BddCofactor(d_i, v_i, 0);
        Ddi_Bdd_t *d_i_1 = Ddi_BddCofactor(d_i, v_i, 1);
        Ddi_Bdd_t *eq_i = Ddi_BddDiff(d_i_1, d_i_0);

        Ddi_BddSetAig(eq_i);
        Ddi_BddPartInsertLast(eqTr, eq_i);
        Ddi_Free(eq_i);
        Ddi_Free(d_i_0);
        Ddi_Free(d_i_1);
      }

      if (i == iProp)
        continue;
      if (i == iConstr)
        continue;
      if (Ddi_VarReadMark(v_i) == 0)
        continue;

      for (j = 0; j < 2; j++) {
        Ddi_Bdd_t *cof_j = Ddi_BddCofactor(d_i, v_i, j);

        if (j == 1)
          Ddi_BddNotAcc(cof_j);
        Ddi_BddSetAig(cof_j);
        scc[j] = !Ddi_AigSat(cof_j);
        if (scc[j]) {
          /* check prop containment */
          Ddi_Bdd_t *prop = Ddi_BddCofactor(deltaProp, v_i, j);

          //          Ddi_BddCofactorAcc(prop,pvarPs,1);
          Ddi_BddNotAcc(prop);
          // inductive P * C
          //        Ddi_BddAndAcc(prop,deltaProp);
          Ddi_BddSetAig(prop);
          scc[j] = scc[j] && !Ddi_AigSat(prop);
          Ddi_Free(prop);
        }
        Ddi_Free(cof_j);
        if (scc[j]) {
          Ddi_Bdd_t *prop = Ddi_BddCofactor(deltaProp, v_i, !j);
          Ddi_Varset_t *supp_c, *supp_p;

          /* check property on transition */
          Ddi_BddCofactorAcc(prop, cvarPs, 1);
          allowTrans = (Ddi_BddSize(prop) == 1); // check if state var ???
	  enforcePropRetime = allowTrans;
          cof_j = Ddi_BddCofactor(d_i, v_i, !j);    /* from outside scc */
          supp_p = Ddi_BddSupp(prop);
          supp_c = Ddi_BddSupp(cof_j);
          Ddi_VarsetIntersectAcc(supp_c, supp_p);
          Ddi_Free(supp_p);
          if (j == 0)
            Ddi_BddNotAcc(cof_j);   /* transition to SCC */
          Ddi_BddNotAcc(prop);
          Ddi_BddSetAig(prop);
          Ddi_BddAndAcc(prop, cof_j);
          transOkProp = !Ddi_AigSat(prop);
          allowTrans |= Ddi_VarsetIsVoid(supp_c) || transOkProp;
          weakScc = !Ddi_VarsetIsVoid(supp_c);
	  //	  printf("WEAK SCC: %d\n", weakScc);

          if (!transOkProp && forceTrans) {
            transOkProp = allowTrans = forcedTrans = retimeProp = 1;
          }
          
          if (0 && Ddi_BddSize(cof_j)==1) {
	    scc[j]=0;
	  }
          Ddi_Free(supp_c);
          Ddi_Free(prop);
          Ddi_Free(cof_j);
          if (weakScc) {
	    //            constrLevel = 0;
	    //            scc[j]=0;          
          }
          retimeProp |= !allowTrans;    // allowTrans=1;
          //          if (!allowTrans) scc[j]=0;          
#if 1
	  if (enforcePropRetime) {
	    // force retime prop (TE) and CONSTR with transizion 
	    if (!transOkProp) {
	      transOkProp = allowTrans = 1;
	      retimeProp = 1;
	    }
	  }
#endif


          if (retimeProp /*|| allowTrans*/) {
            int okInit = 1;
            Ddi_Bdd_t *chk = Ddi_BddNot(Ddi_BddarrayRead(delta, iProp));

            if (initStub != NULL) {
              Ddi_BddComposeAcc(chk, ps, initStub);
            } else {
              Ddi_AigConstrainCubeAcc(chk, init);
            }
            if (Ddi_AigSat(chk)) {
              /* failure on init state. disable ! */
              scc[j] = 0;
	      retimeProp = 0;
            }
            Ddi_Free(chk);
          }

        }

        if (scc[j]) {
          Ddi_Bdd_t *check;

          /* check init state containment */
          if (initStub == NULL) {
            /* check for init states out of scc */
            check = Ddi_BddMakeLiteralAig(v_i, !j);
            Ddi_BddAndAcc(check, init);
          } else {
            /* stub states out of scc */
            check = Ddi_BddDup(Ddi_BddarrayRead(initStub, i));
            if (j == 1) {
              Ddi_BddNotAcc(check);
            }
          }
          if (!Ddi_AigSat(check)) {
            provedProperty = 1;
            Pdtutil_VerbosityLocal
              (Pdtutil_VerbLevelNone_c,
              Pdtutil_VerbLevelNone_c,
              fprintf(stdout, "Structural Induction Found for Property.\n")
              );
            scc[j] = 0;
          }
          Ddi_Free(check);
        }

        if (scc[j]) {
          int k;

          // Ddi_Bdd_t *constDelta = Ddi_BddMakeConstAig(ddm,!j);
          //    Ddi_Bdd_t *constDelta = Ddi_BddCofactor(d_i,v_i,!j);
          Ddi_Bdd_t *constDelta = constrLevel > 0 ?
            Ddi_BddCofactor(d_i, v_i, !j) : Ddi_BddDup(d_i);
          Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(v_i, !j);
          Ddi_Bdd_t *invar = Ddi_BddDup(d_i);

          if (!useAig) {
            Ddi_BddSetMono(constDelta);
            Ddi_BddSetMono(lit);
          }
          if (j == 1)
            Ddi_BddNotAcc(invar);
          Ddi_BddCofactorAcc(invar, v_i, !j);
          nRed++;
          iScc = i;
	  enSearch=0;
          Pdtutil_VerbosityLocal
            (Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, "SCC%d: %s.\n", j, Ddi_VarName(v_i))
            );
          if (sccConstr == NULL) {
            sccConstr = Ddi_BddDup(lit);
          } else {
            Ddi_BddAndAcc(sccConstr, lit);
          }
          for (k = 0; allowTrans && k < nstate; k++) {
            Ddi_Bdd_t *d_k = Ddi_BddarrayRead(delta, k);
            Ddi_Var_t *v_k = Ddi_VararrayRead(ps, k);
            Ddi_Bdd_t *lit_k = Ddi_BddMakeLiteralAig(v_k, 1);

            if (!useAig) {
              Ddi_BddSetMono(lit_k);
            }
            if (0 && k == i) {
              Ddi_BddarrayWrite(delta, k, constDelta);
              d_i = Ddi_BddarrayRead(delta, i);
            } else if (1) {
              Ddi_BddCofactorAcc(d_k, v_i, !j);
              if (0 && k != iProp) {
                Ddi_Bdd_t *newD_k = Ddi_BddIte(invar, d_k, lit_k);

                // NO      Ddi_Bdd_t *newD_k = Ddi_BddIte(lit,d_k,lit_k);
                //  Ddi_AigAndCubeAcc(d_k,lit);
                // rif (k!=i)
                //              Ddi_BddAndAcc(d_k,invar);
                Ddi_BddarrayWrite(delta, k, newD_k);
                Ddi_Free(newD_k);
              }
              if (1 && k == iProp && retimeProp) {
                Ddi_BddNotAcc(lit);
                Ddi_BddOrAcc(d_k, lit);
              }
            }
            Ddi_Free(lit_k);
          }
          if (allowTrans && transOkProp) {
            Ddi_Bdd_t *dConstr = Ddi_BddarrayRead(delta, iConstr);

            if (j)
              Ddi_BddNotAcc(constDelta);
            if (0) {
              Ddi_Varset_t *dsupp = Ddi_BddSupp(constDelta);
              Ddi_Varset_t *psupp = Ddi_BddSupp(deltaProp);

              Ddi_VarsetIntersectAcc(dsupp, psupp);
              if (!Ddi_VarsetIsVoid(dsupp)) {
                Ddi_Bdd_t *dp = Ddi_BddCofactor(deltaProp, pvarPs, 1);

                Ddi_BddNotAcc(dp);
                Ddi_BddOrAcc(constDelta, dp);
                Ddi_Free(dp);
                weakScc = 1;
              }
              Ddi_Free(dsupp);
              Ddi_Free(psupp);
            }
            fsmMgr->stats.retimedConstr = 1;
	    if (0) {
	      Ddi_Bdd_t *cPart = Ddi_AigPartitionTop(constDelta, 0);
	      Ddi_Bdd_t *dPart0 = Ddi_AigPartitionTop(Ddi_BddPartRead(cPart,0),1);
	      Ddi_Bdd_t *dPart1 = Ddi_AigPartitionTop(Ddi_BddPartRead(cPart,1),1);
	      Ddi_Bdd_t *cPart1 = Ddi_AigPartitionTop(Ddi_BddPartRead(dPart1,1),0);
	      Ddi_Free(cPart);
	      Ddi_Free(cPart1);
	      Ddi_Free(dPart0);
	      Ddi_Free(dPart1);
	    }
            Ddi_BddAndAcc(dConstr, constDelta);
          }

          Ddi_Free(constDelta);
          Ddi_Free(lit);
          Ddi_Free(invar);
          if (initStub != NULL) {
            Ddi_Bddarray_t *cubeArray = Ddi_BddarrayDup(initStub);
            Ddi_Bdd_t *stubValRef_i =
              Ddi_BddDup(Ddi_BddarrayRead(initStub, i));
            Ddi_Bdd_t *stubVal_i = Ddi_BddMakeAig(stubValRef_i);
            Ddi_Varset_t *totSupp = Ddi_BddarraySupp(initStub);
            Ddi_Varset_t *supp_i = Ddi_BddSupp(stubVal_i);

            Ddi_VarsetDiffAcc(totSupp, supp_i);
            if (j == 0) {
              Ddi_BddNotAcc(stubValRef_i);
              Ddi_BddNotAcc(stubVal_i);
            }
            if (Ddi_AigSat(stubVal_i)) {
              Ddi_Bdd_t *cex;

              Ddi_BddNotAcc(stubValRef_i);
              Ddi_BddNotAcc(stubVal_i);
#if 0
              /* reconsider when accepting inductive P * C */
              if (Ddi_BddIsOne(stubVal_i)) {
                cex = Ddi_BddMakeConstAig(ddm, 1);
                if (!useAig) {
                  Ddi_BddSetMono(cex);
                }
              } else
#endif
              {
                cex = Ddi_AigSatWithCex(stubVal_i);
              }
              Pdtutil_Assert(cex != NULL, "cex is zero");
              if (!Ddi_VarsetIsVoid(totSupp)) {
                int l;
                Ddi_Vararray_t *vA = Ddi_VararrayMakeFromVarset(totSupp, 1);

                for (l = 0; l < Ddi_VararrayNum(vA); l++) {
                  Ddi_Var_t *v_l = Ddi_VararrayRead(vA, l);
                  Ddi_Bdd_t *lit_l = Ddi_BddMakeLiteralAig(v_l, 0);

                  Ddi_BddAndAcc(cex, lit_l);
                  Ddi_Free(lit_l);
                }
                Ddi_Free(vA);
              }
              if (useAig) {
                Ddi_AigarrayConstrainCubeAcc(cubeArray, cex);
              } else {
                Ddi_BddSetMono(cex);
                for (k = 0; k < nstate; k++) {
                  Ddi_Bdd_t *s_k = Ddi_BddarrayRead(cubeArray, k);

                  Ddi_BddConstrainAcc(s_k, cex);
                }
              }
              Ddi_Free(cex);

              for (k = 0; 0 && k < nstate; k++) {
                if (k != iProp) {
                  Ddi_Bdd_t *cube_k = Ddi_BddarrayRead(cubeArray, k);
                  Ddi_Bdd_t *s_k = Ddi_BddarrayRead(initStub, k);
                  Ddi_Bdd_t *newS_k = Ddi_BddIte(stubValRef_i, s_k, cube_k);

                  Pdtutil_Assert(Ddi_BddSize(cube_k) == 0, "wrong constant");
                  if (k == i) {
                    Ddi_BddNotAcc(newS_k);
                  }
                  Ddi_BddarrayWrite(initStub, k, newS_k);
                  Ddi_Free(newS_k);
                }
              }
	      fprintf(stdout, "TSCC with INIT STUB\n");
              {
                Ddi_Bdd_t *constInitStub = NULL;
                Ddi_Bdd_t *stubConstr = Ddi_BddarrayRead(initStub, iConstr);

                constInitStub = Ddi_BddDup(stubValRef_i);
		// already set at right polarity.
		//                if (j)
		//                  Ddi_BddNotAcc(constInitStub);
                Ddi_BddAndAcc(stubConstr, constInitStub);
                Ddi_Free(constInitStub);
              }
            }

            Ddi_Free(totSupp);
            Ddi_Free(supp_i);
            Ddi_Free(cubeArray);
            Ddi_Free(stubVal_i);
            Ddi_Free(stubValRef_i);
          }
        }
      }
    }
    for (i = 0; i < Ddi_VararrayNum(pSuppA); i++) {
      Ddi_Var_t *v = Ddi_VararrayRead(pSuppA, i);

      Ddi_VarWriteMark(v, 0);
    }
    Ddi_Free(pSuppA);

    for (i = 0; 0 && i < nstate; i++) {
      Ddi_Bdd_t *stall;
      Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);
      Ddi_Bdd_t *eq_i = Ddi_BddDup(Ddi_BddPartRead(eqTr, i));

      Ddi_BddPartWrite(eqTr, i, one);
      stall = Ddi_BddMakeAig(eqTr);
      Ddi_BddPartWrite(eqTr, i, eq_i);
      Ddi_BddNotAcc(eq_i);
      Ddi_BddAndAcc(eq_i, stall);
      if (Ddi_AigSat(eq_i)) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Disjoint dpart Possible With: %d (%s).\n",
            i, Ddi_VarName(v_i))
          );
      }
      Ddi_Free(eq_i);
      Ddi_Free(stall);
    }

    if (sccConstr != NULL && !provedProperty) {
      Ddi_Bdd_t *dConstr = Ddi_BddarrayRead(delta, iConstr);
      Ddi_Bdd_t *sccDelta = Ddi_BddarrayRead(delta, iScc);

      Ddi_BddAndAcc(dConstr, sccConstr);
      int doStructRed=0;      
      if (doStructRed) {
        Ddi_Bdd_t *c = Ddi_BddDup(dConstr);
        Ddi_AigarrayStructRedRemWithConstrAcc (delta,NULL,c);
        Ddi_BddarrayWrite(delta, iConstr, c);
        Ddi_Free(c);
      }
      //      Ddi_BddAndAcc(dConstr, sccDelta); /* backward retiming constr */
      
      //      Ddi_BddAndAcc(sccDelta, sccConstr);
      //Ddi_DataCopy(sccConstr, dConstr); // CHECK for consistency !!! INTEL/BEEM
    }

    Ddi_Free(deltaProp);
    Ddi_Free(eqTr);
    Ddi_Free(one);

    if (1 && retimeProp) {
      char suffix[20];
      Ddi_Var_t *vConstr = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
      Ddi_Bddarray_t *deltaPrev = Ddi_BddarrayDup(delta);
      Ddi_Bdd_t *deltaPropPrev = Ddi_BddarrayRead(deltaPrev, iProp);

      sprintf(suffix, "%d", fsmMgr->stats.targetEnSteps++);
      deltaProp = Ddi_BddDup(Ddi_BddarrayRead(delta, iProp));

      if (0 && vConstr!=NULL) {
	// disabled: TE with out oring with prop literal is enough
	Ddi_Bdd_t *litConstr = Ddi_BddMakeLiteralAig(vConstr, 0);
	Ddi_BddOrAcc(deltaPropPrev,litConstr);
	Ddi_Free(litConstr);
      }

      if (0 && (Ddi_BddSize(sccConstr)==1)) {
	Ddi_BddNotAcc(sccConstr);
	Ddi_BddOrAcc(deltaProp,sccConstr);
	Ddi_BddNotAcc(sccConstr);
      }
      Ddi_Vararray_t *newPis = Ddi_BddNewVarsAcc(deltaProp, pi,
        "PDT_TARGET_EN_PI", suffix, 1);

      //      Ddi_BddComposeAcc(deltaProp, ps, delta);
      Ddi_VararrayAppend(pi,newPis);
      if (0&&forcedTrans) {
        Ddi_Free(newPis);
        newPis = Ddi_BddarrayNewVarsAcc(deltaPrev, pi,
            "PDT_PROP_RETIME_PI", suffix, 0);
        Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMin_c) {
          fprintf(dMgrO(ddm), "TSCC FORCED CONSTR RETIMING\n");
        }
      }
      Ddi_BddComposeAcc(deltaProp, ps, deltaPrev);
      Ddi_BddarrayWrite(delta, iProp, deltaProp);
      Ddi_Free(deltaPrev);
      Ddi_Free(deltaProp);
      Ddi_Free(newPis);
    }

  }

  if (nRed > 0 && weakScc) {
    nRed = -nRed;
  }

  if (sccNumP != NULL) {
    *sccNumP = nRed;
  }
  if (provedP != NULL) {
    *provedP = provedProperty;
  }

  return (sccConstr);
}



/**Function********************************************************************

  Synopsis     [Given a Retiming Array Apply it to the FSM Manager.
    Return a new FSM Manager.]

  Description  [There should be a 1 to 1 correspondence between the
    retiming array (retimeStrPtr->retimeArray) and the delta array.]

  SideEffects  []

  SeeAlso      []

******************************************************************************/

Ddi_Bdd_t *
Fsm_ReduceImpliedConstr(
  Fsm_Mgr_t * fsmMgr,
  int decompId,
  Ddi_Bdd_t * outInvar,
  int *impliedConstrNumP
)
{
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);

  //  bAig_Manager_t *bmgr = ddm->aig.mgr;

  /*----------------------- Take Fields From FSM Manager --------------------*/

  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *lambda = Fsm_MgrReadLambdaBDD(fsmMgr);

  Ddi_Vararray_t *substVars = Ddi_VararrayAlloc(ddm, 0);
  Ddi_Bddarray_t *substFuncs = Ddi_BddarrayAlloc(ddm, 0);

  int nstate = Ddi_VararrayNum(ps);
  int nVars = Ddi_MgrReadNumVars(ddm);
  int nRed = 0;
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
  Ddi_Bdd_t *invar = NULL;
  Ddi_Bdd_t *invarConstr = NULL;

  int i, useAig = 0, initSize, addInitStub = 0;
  int again, okInit = 1, nImpl = 0;
  static int abstrCnt = 0;

  /*-------------------------- Take Retiming Options ------------------------*/

  Pdtutil_Assert(nstate == Ddi_BddarrayNum(delta), "Wrong num of latches");
  Pdtutil_Assert(nstate == Ddi_VararrayNum(ps), "Wrong num of latches");
  Pdtutil_Assert(nstate == Ddi_VararrayNum(ns), "Wrong num of latches");

  useAig = Ddi_BddIsAig(Ddi_BddarrayRead(delta, 0));
  if (!useAig) {
    Ddi_AigInit(ddm, 100);
  }


  if (1) {
    Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
    Ddi_Var_t *pvarNs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$NS");
    Ddi_Var_t *cvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
    Ddi_Var_t *cvarNs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$NS");
    int iProp = -1, iConstr = -1, size;
    Ddi_Bdd_t *deltaProp;
    Ddi_Bdd_t *one = Ddi_BddMakeConstAig(ddm, 1);
    int chkInit=1;

    for (i = 0; i < nstate; i++) {
      Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);

      if (v_i == pvarPs) {
        iProp = i;
      }
      if (v_i == cvarPs) {
        iConstr = i;
      }
    }
    Pdtutil_Assert(iProp < nstate, "PDT_BDD_INVARSPEC_VAR$PS var not found");
    deltaProp = Ddi_BddCofactor(Ddi_BddarrayRead(delta, iProp), pvarPs, 1);
    if (iConstr >= 0) {
      Ddi_BddCofactorAcc(deltaProp, cvarPs, 1);
    }
    nstate = Ddi_VararrayNum(ps);
    size = Ddi_BddSize(deltaProp);

    if (chkInit) {
      Ddi_Bdd_t *chk = Ddi_BddNot(deltaProp);
      if (iConstr >= 0) {
	Ddi_Bdd_t *deltaConstr = Ddi_BddarrayRead(delta, iConstr);
	Ddi_BddAndAcc(chk, deltaConstr);
      }

      if (initStub != NULL) {
	Ddi_BddComposeAcc(chk, ps, initStub);
      } else {
	Ddi_AigConstrainCubeAcc(chk, init);
      }
      if (0) {
	int i, j, done = 0;
	Ddi_Bddarray_t *xorA = Ddi_AigPartitionTopXorOrXnor(chk,1);
	while ((xorA != NULL) && !done) {
	  Ddi_Bddarray_t *newA = Ddi_BddarrayAlloc(ddm, 0);
	  done = 1;
	  for (i=0; i<Ddi_BddarrayNum(xorA); i++) {
	    Ddi_Bdd_t *p_i = Ddi_BddarrayRead(xorA,i);
	    Ddi_Bddarray_t *decomp = Ddi_AigPartitionTopXorOrXnor(p_i,1);
	    if (decomp!=NULL) {
	      done = 0;
	      Ddi_BddarrayAppend(newA,decomp);
	      Ddi_Free(decomp);
	    }
	    else {
	      Ddi_BddarrayInsertLast(newA,p_i);
	    }
	  }
	  Ddi_Free(xorA); xorA = newA;
	  for (i=0; i<Ddi_BddarrayNum(xorA); i++) {
	    Ddi_Bdd_t *p_i = Ddi_BddarrayRead(xorA,i);
	    for (j=0; j<i; j++) {
	      Ddi_Bdd_t *p_j = Ddi_BddarrayRead(xorA,j);
	      if (Ddi_BddEqual(p_i,p_j)) {
		Ddi_BddarrayRemove(xorA,i);
		i--;
	      }
	    }
	  }
	}
	if (xorA != NULL) {
	  long startTime, tile0, stepTime = 0;
	  Ddi_IncrSatMgr_t *ddiS = NULL;
	  int i, j, num = Ddi_BddarrayNum(xorA);
	  int split=0;
	  DdiAig2CnfIdInit(ddm);
	  ddiS = Ddi_IncrSatMgrAlloc(NULL,1,0,0);
	  for (split=1; split<=num; split++) {
	    Ddi_Bdd_t *cex=NULL;
	    int abort;
	    Ddi_Bdd_t *a = Ddi_BddDup(Ddi_BddarrayRead(xorA,0));
	    for (i=1; i<num; i++) {
	      Ddi_Bdd_t *p_i = Ddi_BddarrayRead(xorA,i);
	      if (i<split)
		Ddi_BddXorAcc(a,p_i);
	      else 
		Ddi_BddDiffAcc(a,p_i);
	    }
	    startTime = util_cpu_time();
	    if (0) {
	      Ddi_MgrSiftSuspend(ddm);
	      int sat = Ddi_AigSat(a);
	      Ddi_MgrSiftResume(ddm);

	      stepTime = (util_cpu_time() - startTime);
	      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMin_c) {
		fprintf(dMgrO(ddm), "SAT step %d time: %s\n", split,
			util_print_time(stepTime));
	      }
	    }
	    stepTime = (util_cpu_time() - startTime);
	    Ddi_BddSetPartConj(a);
	    Ddi_AigSatMinisatLoadClausesIncremental(ddiS, a,NULL);
	    //	    Ddi_AigSatMinisat22PrintStats(ddm,ddiS,0);    
	    cex = Ddi_AigSatMinisat22WithCexAndAbortIncremental(ddiS,
		  a, NULL,0,-1.0, &abort);
	    stepTime = (util_cpu_time() - startTime);
	    Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMin_c) {
	      fprintf(dMgrO(ddm), "INCR SAT step %d time: %s\n", split,
		      util_print_time(stepTime));
	    }
	    Ddi_Free(cex);
	    //	    Ddi_Free(a);
	  }
	  Ddi_Free(xorA);
	  Ddi_IncrSatMgrQuit(ddiS);
	  DdiAig2CnfIdCloseNoCheck(ddm);
	}
	Ddi_Bdd_t *a = Ddi_BddMakeFromBaig(ddm,(bAigEdge_t)22084);
	Ddi_Bdd_t *b = Ddi_BddMakeFromBaig(ddm,(bAigEdge_t)37993);
	Ddi_Bdd_t *chkPartCa = Ddi_AigPartitionTop(a, 0);
	Ddi_Bdd_t *chkPartCb = Ddi_AigPartitionTop(b, 0);
	Ddi_Bdd_t *chkPartDa = Ddi_AigPartitionTop(a, 1);
	Ddi_Bdd_t *chkPartDb = Ddi_AigPartitionTop(b, 1);
	Ddi_Bdd_t *chkPartX = Ddi_AigPartitionTopWithXor(chk, 0, 1);
	Ddi_Free(chkPartX);
	Ddi_Free(chkPartCa);
	Ddi_Free(chkPartDa);
	Ddi_Free(chkPartCb);
	Ddi_Free(chkPartDb);
	Ddi_Free(xorA);
      }
      if (Ddi_AigSat(chk)) {
	/* failure on init state. disable ! */
	okInit = 0;
      }
      Ddi_Free(chk);
    }


    if (okInit && !Ddi_BddIsConstant(deltaProp) && 
	Ddi_BddSize(deltaProp) > 1) {
      Ddi_Bdd_t *dPart = Ddi_AigPartitionTop(deltaProp, 1);
      Ddi_Free(dPart);
    }
    if (okInit && !Ddi_BddIsConstant(deltaProp) && 
	Ddi_BddSize(deltaProp) == 1) {

      Ddi_Varset_t *pSupp = Ddi_BddSupp(deltaProp);
      Ddi_Vararray_t *pSuppA = Ddi_VararrayMakeFromVarset(pSupp, 1);

      Ddi_Free(pSupp);
      for (i = 0; i < Ddi_VararrayNum(pSuppA); i++) {
        Ddi_Var_t *v = Ddi_VararrayRead(pSuppA, i);

        Pdtutil_Assert(Ddi_VarReadMark(v) == 0, "0 var mark required");
        Ddi_VarWriteMark(v, 1);
      }

      for (i = 0; i < nstate; i++) {
        int j;
        Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);
        Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);
        int scc[2] = { 0, 0 };

        if (i == iProp)
          continue;
        if (i == iConstr)
          continue;
        if (nImpl > 0)
          continue;
        for (j = 1; j >= 0; j--) {
          Ddi_Bdd_t *cof_j = Ddi_BddCofactor(deltaProp, v_i, j);

          Ddi_BddSetAig(cof_j);
          scc[j] = !Ddi_AigSat(cof_j);
          if (scc[j]) {
            int k;
            Ddi_Vararray_t *substVar = Ddi_VararrayAlloc(ddm, 1);
            Ddi_Bddarray_t *substFunc = Ddi_BddarrayAlloc(ddm, 1);

            Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
              Pdtutil_VerbLevelNone_c,
              fprintf(stdout, "Implied Constraint %s%s.\n", j ? "!" : "",
                Ddi_VarName(v_i))
              );
            nImpl++;
            if (0 && initStub != NULL) {
              /* check init stub: should hold implied = CONST */
              int okStub;
              Ddi_Bdd_t *stubVal_i = Ddi_BddDup(Ddi_BddarrayRead(initStub, i));
              Ddi_Bdd_t *stubConst_i = Ddi_BddDup(one);

              if (!useAig) {
                Ddi_BddSetMono(stubConst_i);
              }
              Ddi_BddSetAig(stubVal_i);
              if (!j) {
                Ddi_BddNotAcc(stubVal_i);
              } else {
                Ddi_BddNotAcc(stubConst_i);
              }
              okStub = !Ddi_AigSat(stubVal_i);
              if (okStub) {
                Ddi_BddarrayWrite(initStub, i, stubConst_i);
              } else {
                Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
                  Pdtutil_VerbLevelNone_c,
                  fprintf(stdout, "Property Fails on Init Stub.\n")
                  );
              }
              Ddi_Free(stubVal_i);
              Ddi_Free(stubConst_i);
            }

            /* backward retime property */
            Ddi_VararrayWrite(substVar, 0, v_i);
            Ddi_BddarrayWrite(substFunc, 0, d_i);
            Ddi_BddComposeAcc(Ddi_BddarrayRead(delta, iProp), substVar,
              substFunc);

            Ddi_BddComposeAcc(deltaProp, substVar, substFunc);
            Ddi_BddCofactorAcc(deltaProp, v_i, !j);

            fsmMgr->stats.targetEnSteps++;

            for (k = 0; k < nstate; k++) {
              Ddi_Bdd_t *d_k = Ddi_BddarrayRead(delta, k);

              Ddi_BddCofactorAcc(d_k, v_i, !j);
            }

            if (!fsmMgr->stats.retimedConstr && (outInvar != NULL)) {
              // GPC
              // BUGGY @ HWMCC2014 on beemrshr2f1 and intels
              // overconstraining
              // fixed in terminal scc, avoiding constr retiming
#if 0
              Ddi_BddComposeAcc(outInvar, ps, delta);
#else
              Ddi_Bdd_t *prevInvar = Ddi_BddCompose(outInvar, ps, delta);

	      // Ddi_Bdd_t *inv0 = Ddi_BddMakeLiteralAig(cvarPs, 0);
	      // Ddi_BddOrAcc(Ddi_BddarrayRead(delta, iProp), inv0);
	      // Ddi_Free(inv0);

	      int tryFullRetimeConstr = 1;
	      if (tryFullRetimeConstr) {
		Ddi_Bdd_t *deltaConstr = 
		  Ddi_BddDup(Ddi_BddarrayRead(delta, iConstr));
		Ddi_BddComposeAcc(outInvar, ps, delta);
		Ddi_BddComposeAcc(deltaConstr, ps, delta);
		Ddi_BddarrayWrite(delta,iConstr,deltaConstr);
		Ddi_Free(deltaConstr);
	      }

              Ddi_BddNotAcc(prevInvar);
              //          Ddi_BddarrayWrite(delta,iProp,deltaProp);
              Ddi_BddOrAcc(Ddi_BddarrayRead(delta, iProp), prevInvar);
              Ddi_Free(prevInvar);
#endif
            }

            Ddi_Free(substVar);
            Ddi_Free(substFunc);
          }
          Ddi_Free(cof_j);
          if (scc[j])
            break;
        }
      }
      for (i = 0; i < Ddi_VararrayNum(pSuppA); i++) {
        Ddi_Var_t *v = Ddi_VararrayRead(pSuppA, i);

        Ddi_VarWriteMark(v, 0);
      }
      Ddi_Free(pSuppA);
    }
    else if (0 && okInit && !Ddi_BddIsConstant(deltaProp)) {
      // check on 6s157 SAT not working - its retiming !!!
      Ddi_Vararray_t *pSuppA = Ddi_BddSuppVararray(deltaProp);
      Ddi_VararrayDiffAcc(pSuppA,ps);
      if (Ddi_VararrayNum(pSuppA)<10) {
	int j;
	Ddi_Bdd_t *constr = Ddi_BddMakeConstAig(ddm, 1);
	Ddi_Bdd_t *dPart = Ddi_AigPartitionTopWithXor(deltaProp, 0, 1);
	for (j=0; j<Ddi_BddPartNum(dPart); j++) {
	  Ddi_Bdd_t *p_j = Ddi_BddPartRead(dPart, j);
	  if (Ddi_AigIsXorOrXnor(p_j) && Ddi_BddSize(p_j)<=6) {
	    Ddi_Vararray_t *s =Ddi_BddSuppVararray(p_j);
	    if (Ddi_VararrayNum(s)==2) {
	      Ddi_VararrayDiffAcc(s,ps);
	      if (Ddi_VararrayNum(s)==0) {
		Ddi_BddAndAcc(constr,p_j);
	      }
	    }
	    Ddi_Free(s);
	  }
	}
	Ddi_Free(dPart);
	if (!Ddi_BddIsOne(constr)) {
	  /* retime: */
	  Ddi_BddComposeAcc(deltaProp,ps,delta);
	  Ddi_BddAndAcc(Ddi_BddarrayRead(delta,iProp),deltaProp);
	  Ddi_BddAndAcc(Ddi_BddarrayRead(delta,iConstr),constr);
	}
	Ddi_Free(constr);
      }
      Ddi_Free(pSuppA);
    }
    Ddi_Free(deltaProp);

    Ddi_Free(invar);

    Ddi_Free(one);

  }
  Ddi_Free(substVars);
  Ddi_Free(substFuncs);

  if (impliedConstrNumP != NULL) {
    *impliedConstrNumP = nImpl;
  }

#if 0
  printf("DELTA SIZE: ");
  for (i = 0; i < nstate; i++) {
    Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta,i);
    printf("[%d] %d\n", i, Ddi_BddSize(d_i));
  }
  printf("\n");
#endif

  return (invarConstr);
}


/**Function********************************************************************

  Synopsis     [Given a Retiming Array Apply it to the FSM Manager.
    Return a new FSM Manager.]

  Description  [There should be a 1 to 1 correspondence between the
    retiming array (retimeStrPtr->retimeArray) and the delta array.]

  SideEffects  []

  SeeAlso      []

******************************************************************************/
Ddi_Bdd_t *
Fsm_RetimeWithConstr(
  Fsm_Mgr_t * fsmMgr
)
{
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);
  bAig_Manager_t *bmgr = ddm->aig.mgr;

  /*----------------------- Take Fields From FSM Manager --------------------*/

  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  Ddi_Bdd_t *fsmConstr = Fsm_MgrReadConstraintBDD(fsmMgr);

  int nstate = Ddi_VararrayNum(ps);
  int nVars = Ddi_MgrReadNumVars(ddm);
  int i, nRed = 0, cnt2, cnt1;
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
  Ddi_Bdd_t *invar = NULL;
  Ddi_Bdd_t *invarConstr = NULL;
  int *foCnt;
  
  bAig_array_t *visitedNodes = bAigArrayAlloc();
  
  Pdtutil_Assert(nstate == Ddi_BddarrayNum(delta), "Wrong num of latches");
  Pdtutil_Assert(nstate == Ddi_VararrayNum(ps), "Wrong num of latches");
  Pdtutil_Assert(nstate == Ddi_VararrayNum(ns), "Wrong num of latches");

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Retime Reduction: Delta size %d (num = %d).\n",
      Ddi_BddarraySize(delta), nstate)
    );

  for (i=0; i<Ddi_BddarrayNum(delta); i++) {
    Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta,i);
    Ddi_PostOrderBddAigVisitIntern(d_i,visitedNodes,-1);
  }
  for (i=0; i<Ddi_BddarrayNum(lambda); i++) {
    Ddi_Bdd_t *l_i = Ddi_BddarrayRead(delta,i);
    Ddi_PostOrderBddAigVisitIntern(l_i,visitedNodes,-1);
  }
  if (fsmConstr!=NULL)  
    Ddi_PostOrderBddAigVisitIntern(fsmConstr,visitedNodes,-1);

  foCnt = Pdtutil_Alloc(int,visitedNodes->num);
  
  Ddi_PostOrderAigClearVisitedIntern(bmgr,visitedNodes);

  for (i=0; i<visitedNodes->num; i++) {
    bAigEdge_t baig = visitedNodes->nodes[i];
    bAig_AuxInt(bmgr, baig) = i;
    if (!bAig_NodeIsConstant(baig) && !bAig_isVarNode(bmgr,baig)) {
      int ir, il;
      bAigEdge_t right, left;
      right = bAig_NodeReadIndexOfRightChild(bmgr, baig);
      left = bAig_NodeReadIndexOfLeftChild(bmgr, baig);
      ir = bAig_AuxInt(bmgr, right);
      il = bAig_AuxInt(bmgr, left);
      foCnt[il]++;
      foCnt[ir]++;
    }
  }

  int iConstr = -1;
  if (1) {
    Ddi_Var_t *cvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
    iConstr = Ddi_VararrayNum(ps)-2;
    Pdtutil_Assert (Ddi_VararrayRead(ps,iConstr)==cvarPs,
                    "Missing constr");
    invarConstr = Ddi_BddarrayRead(delta,iConstr);
  }
  
  cnt1 = cnt2 = 0;
  Ddi_VararrayWriteMarkWithIndex (ps, 1);
  int redL=0, redPi=0, redPi1;
  Ddi_Bddarray_t *oldA = Ddi_BddarrayAlloc(ddm,0);
  Ddi_Bddarray_t *newA = Ddi_BddarrayAlloc(ddm,0);
  
  for (i=0; i<visitedNodes->num; i++) {
    bAigEdge_t baig = visitedNodes->nodes[i];
    if (!bAig_NodeIsConstant(baig) && !bAig_isVarNode(bmgr,baig)) {
      int ir, il;
      bAigEdge_t right, left;
      right = bAig_NodeReadIndexOfRightChild(bmgr, baig);
      left = bAig_NodeReadIndexOfLeftChild(bmgr, baig);
      ir = bAig_AuxInt(bmgr, right);
      il = bAig_AuxInt(bmgr, left);
      if (bAig_isVarNode(bmgr,right)&&bAig_isVarNode(bmgr,left)){
        Ddi_Var_t *vr = Ddi_VarFromBaig(ddm, right);
        Ddi_Var_t *vl = Ddi_VarFromBaig(ddm, left);
        int rCompl = bAig_NodeIsInverted(right);
        int lCompl = bAig_NodeIsInverted(left);
        char *namer = bAig_NodeReadName(bmgr,right);
        char *namel = bAig_NodeReadName(bmgr,left);
        int lr = Ddi_VarReadMark(vr);
        int ll = Ddi_VarReadMark(vl);
        Pdtutil_Assert(vr!=NULL&&vl!=NULL,"missing var");

        if (foCnt[ir]==1 && foCnt[il]==1) {
          printf("RET2: %s %s\n", namer, namel); cnt2++;
          if (lr>0&&ll>0) {
            // two latches
            Ddi_Bdd_t *rLit = Ddi_BddMakeLiteralAig(vr, !rCompl);
            Ddi_Bdd_t *lLit = Ddi_BddMakeLiteralAig(vl, !lCompl);
            Ddi_Bdd_t *old = Ddi_BddAnd(rLit,lLit);
            Ddi_Bdd_t *New = Ddi_BddMakeLiteralAig(vr,1);
            Ddi_Bdd_t *d_r = Ddi_BddarrayRead(delta,lr-1);
            Ddi_Bdd_t *d_l = Ddi_BddarrayRead(delta,ll-1);
            if (rCompl) Ddi_BddNotAcc(d_r);
            if (lCompl) Ddi_BddNotAcc(d_l);
            Ddi_BddAndAcc(d_r,d_l);
            Ddi_VarWriteMark(Ddi_VararrayRead(ns,ll-1),1); // mark delete
            if (initStub!=NULL) {
              Ddi_Bdd_t *is_r = Ddi_BddarrayRead(initStub,lr-1);
              Ddi_Bdd_t *is_l = Ddi_BddarrayRead(initStub,ll-1);
              if (rCompl) Ddi_BddNotAcc(is_r);
              if (lCompl) Ddi_BddNotAcc(is_l);
              Ddi_BddAndAcc(is_r,is_l);
            }
            else {
              
            }
            Ddi_BddarrayInsertLast(oldA,old);
            Ddi_BddarrayInsertLast(newA,New);
            Ddi_Free(rLit);
            Ddi_Free(lLit);
            Ddi_Free(old);
            Ddi_Free(New);
            redL++;
          }
          else if (lr>0||ll>0) {
            // one latch - one pi
          }
          else {
            // two pis
            Ddi_Bdd_t *rLit = Ddi_BddMakeLiteralAig(vr, !rCompl);
            Ddi_Bdd_t *lLit = Ddi_BddMakeLiteralAig(vl, !lCompl);
            Ddi_Bdd_t *old = Ddi_BddAnd(rLit,lLit);
            Ddi_Bdd_t *New = Ddi_BddMakeLiteralAig(vr,1);
            Ddi_BddarrayInsertLast(oldA,old);
            Ddi_BddarrayInsertLast(newA,New);
            Ddi_Free(rLit);
            Ddi_Free(lLit);
            Ddi_Free(old);
            Ddi_Free(New);
            redPi++;
          }
        }
        else if (foCnt[ir]==1 || foCnt[il]==1) {
          printf("RET1: %s %s\n", namer, namel); cnt1++;
          if (0 && (foCnt[ir]==1 && lr==0 || foCnt[il]==1 && ll==0)) {
            Ddi_Bdd_t *rLit = Ddi_BddMakeLiteralAig(vr, !rCompl);
            Ddi_Bdd_t *lLit = Ddi_BddMakeLiteralAig(vl, !lCompl);
            Ddi_Bdd_t *old = Ddi_BddAnd(rLit,lLit);
            Ddi_Bdd_t *New, *constr;
            if (foCnt[ir]==1) {
              New = Ddi_BddDup(rLit);
              Ddi_BddNotAcc(rLit);
            }
            else {
              New = Ddi_BddDup(lLit);
              Ddi_BddNotAcc(lLit);
            }
            constr = Ddi_BddOr(rLit,lLit);
            Ddi_BddAndAcc(invarConstr,constr);
            Ddi_BddarrayInsertLast(oldA,old);
            Ddi_BddarrayInsertLast(newA,New);
            Ddi_Free(constr);
            Ddi_Free(rLit);
            Ddi_Free(lLit);
            Ddi_Free(old);
            Ddi_Free(New);
            redPi1++;
          }
        }
      }
    }
  }

  for (i=Ddi_VararrayNum(ns)-1; i>=0; i--) {
    Ddi_Var_t *ns_i = Ddi_VararrayRead(ns,i);
    if (Ddi_VarReadMark(ns_i) > 0) {
      Ddi_VararrayRemove(ns,i);
      Ddi_VararrayRemove(ps,i);
      Ddi_BddarrayRemove(delta,i);
      Ddi_BddarrayRemove(initStub,i);
    }
  }
  // substiture new for old in delta/lambda/constr
  Ddi_AigarrayComposeFuncAcc (delta, oldA, newA);
  Ddi_AigarrayComposeFuncAcc (lambda, oldA, newA);

  printf("TOTAL RET CANDIDATES 1: %d - 2: %d\n", cnt1, cnt2);
  printf("TOTAL RET REDUCTIONS Latch: %d - Pi: %d - Pi1: %d\n",
         redL, redPi, redPi1);
  Ddi_VararrayWriteMark (ps, 0);

  Ddi_Free(oldA);
  Ddi_Free(newA);
  
  Ddi_AigArrayClearAuxInt(bmgr,visitedNodes);
  Pdtutil_Free(foCnt);
  bAigArrayFree(visitedNodes);
  
  return NULL;
}
 
/**Function********************************************************************

  Synopsis     [Given a Retiming Array Apply it to the FSM Manager.
    Return a new FSM Manager.]

  Description  [There should be a 1 to 1 correspondence between the
    retiming array (retimeStrPtr->retimeArray) and the delta array.]

  SideEffects  []

  SeeAlso      []

******************************************************************************/

Ddi_Bdd_t *
Fsm_ReduceCustom(
  Fsm_Mgr_t * fsmMgr
)
{
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);
  bAig_Manager_t *bmgr = ddm->aig.mgr;

  /*----------------------- Take Fields From FSM Manager --------------------*/

  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  Ddi_Bdd_t *fsmConstr = Fsm_MgrReadConstraintBDD(fsmMgr);

  int nstate = Ddi_VararrayNum(ps);
  int nVars = Ddi_MgrReadNumVars(ddm);
  int nRed = 0;
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Bdd_t *invar = NULL;
  Ddi_Bdd_t *invarConstr = NULL;

  int i, useAig = 0, initSize, adIdnitStub = 0;
  int again, okInit = 1;
  static int abstrCnt = 0;
  int firstEq = -1;
  Ddi_Var_t *firstEqVar = NULL;
  int doReduce = 1;
  int customConstraint = 1;
  int searClock = 0;
  int disjDecomp = 0;
  int beemCustom = 0;
  int piComp = 0, nEquiv = 0, nEquiv0 = 0, nImpl = 0;

  /*-------------------------- Take Retiming Options ------------------------*/


  Pdtutil_Assert(nstate == Ddi_BddarrayNum(delta), "Wrong num of latches");
  Pdtutil_Assert(nstate == Ddi_VararrayNum(ps), "Wrong num of latches");
  Pdtutil_Assert(nstate == Ddi_VararrayNum(ns), "Wrong num of latches");

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Custom Reduction: Delta size %d (num = %d).\n",
      Ddi_BddarraySize(delta), nstate)
    );

  useAig = Ddi_BddIsAig(Ddi_BddarrayRead(delta, 0));
  if (!useAig) {
    Ddi_AigInit(ddm, 100);
  }

  {
    Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(pi);
    Ddi_Varset_t *psv = Ddi_VarsetMakeFromArray(ps);
    Ddi_Varset_t *dsupp = Ddi_BddarraySupp(delta);

    Ddi_VarsetDiffAcc(dsupp, piv);
    Ddi_VarsetDiffAcc(dsupp, psv);

    if (Ddi_VarsetNum(dsupp) > 0) {
      Ddi_Vararray_t *newPis = Ddi_VararrayMakeFromVarset(dsupp, 1);

      Ddi_VararrayAppend(pi, newPis);
      Ddi_Free(newPis);
    }

    Ddi_Free(dsupp);
    Ddi_Free(piv);
    Ddi_Free(psv);
  }

  if (beemCustom) {
    for (i = 0; i < nstate; i++) {
      Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);

      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMin_c) {
        fprintf(stdout, "[%d]%d ", i, Ddi_BddSize(d_i));
      }
    }
    Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMin_c) {
      fprintf(stdout, "\n");
    }
  }

  if (searClock) {
    for (i = 0; i < nstate; i++) {
      Ddi_Bdd_t *dClk0, *dClk1;
      Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);

      dClk0 = Ddi_BddCofactor(d_i, v_i, 0);
      Ddi_BddNotAcc(dClk0);
      dClk1 = Ddi_BddCofactor(d_i, v_i, 1);
      if (!Ddi_AigSat(dClk1) && !Ddi_AigSat(dClk0)) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Clock Found: %s.\n", Ddi_VarName(v_i))
          );
      }
      Ddi_Free(dClk0);
      Ddi_Free(dClk1);
    }
  }


  int customZipcpu = 0;
  if (customZipcpu) {
    Ddi_Var_t *reset = Ddi_VarFromName(ddm, "i_reset");
    Ddi_Var_t *count = Ddi_VarFromName(ddm, "i_ce");
    if (reset!=NULL) {
      Ddi_BddarrayCofactorAcc(delta, reset, 0);
    }
    if (count!=NULL) {
      Ddi_BddarrayCofactorAcc(delta, count, 1);
      //return NULL;
    }
  }
  
  if (1) {
    Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
    Ddi_Var_t *pvarNs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$NS");
    Ddi_Var_t *cvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
    Ddi_Var_t *cvarNs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$NS");
    int iProp = -1, iConstr = -1;
    Ddi_Bdd_t *deltaProp = NULL, *deltaConstr = NULL;
    Ddi_Bdd_t *one = Ddi_BddMakeConstAig(ddm, 1);

    for (i = 0; i < nstate; i++) {
      Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);

      if (v_i == pvarPs) {
        iProp = i;
      }
      if (v_i == cvarPs) {
        iConstr = i;
      }
    }
    Pdtutil_Assert(iProp < nstate, "PDT_BDD_INVARSPEC_VAR$PS var not found");
    deltaProp = Ddi_BddCofactor(Ddi_BddarrayRead(delta, iProp), pvarPs, 1);
    if (iConstr >= 0) {
      Ddi_BddCofactorAcc(deltaProp, cvarPs, 1);
      deltaConstr = Ddi_BddDup(Ddi_BddarrayRead(delta, iConstr));
      Ddi_BddCofactorAcc(deltaConstr, cvarPs, 1);
    }
    nstate = Ddi_VararrayNum(ps);

    extern int fbvCustom;
    disjDecomp = fbvCustom > 0;
    if (disjDecomp) {
      int nRed = 0;
      Ddi_Bdd_t *dPart = Ddi_AigPartitionTop(deltaProp, 1);
      Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(pi);
      Ddi_Varset_t *psv = Ddi_VarsetMakeFromArray(ps);

      if (Ddi_BddPartNum(dPart) > 1) {
        Ddi_Bdd_t *deltaProp2 = Ddi_BddDup(deltaProp);
        Ddi_Bdd_t *auxPart = Ddi_BddMakePartDisjVoid(ddm);
        Ddi_Varset_t *constrSupp = Ddi_BddSupp(deltaConstr);

        Ddi_VarsetDiffAcc(piv, constrSupp);
        Ddi_Free(constrSupp);

	int custom=0, custom1=1, custom2=0;
	int n=Ddi_BddPartNum(dPart);
	int limit = fbvCustom;
	if (limit > 10000) {
	  custom2 = 1;
	  custom1 = 0;
	  limit-=10000;
	}
	else if (limit > 1000) {
	  custom1 = 2;
	  limit-=1000;
	}
	if (custom2>=1) {
	  int size1, size0 = Ddi_BddSize(deltaConstr);
	  int r,l;
	  Ddi_Bdd_t *not_entered_lasso = NULL;
          Ddi_BddPartSortBySizeAcc(dPart, 1);
	  for (i=Ddi_BddPartNum(dPart)-1; i>=limit; i--) {
	    Ddi_Bdd_t *p_i = Ddi_BddPartRead(dPart,i);
	    printf("removed part %d/%d - size: %d\n",
		   i, n, Ddi_BddSize(p_i));
	    Ddi_BddPartRemove(dPart, i);
	  }
	  Ddi_BddSetAig(dPart);
	  Ddi_DataCopy(deltaProp2,dPart);
	}
	if (custom1>=1) {
	  int size1, size0 = Ddi_BddSize(deltaConstr);
	  int r,l;
	  Ddi_Bdd_t *not_entered_lasso = NULL;
          Ddi_BddPartSortBySizeAcc(dPart, 1);
	  for (r=0; r<n; r++) {
	    if (Ddi_BddSize(Ddi_BddPartRead(dPart,r))==3) break;
	  }
	  for (l=n-1; l>0; l--) {
	    if (Ddi_BddSize(Ddi_BddPartRead(dPart,l))==3) break;
	  }
	  if (custom1>1) {
	    Ddi_Var_t *elv = Ddi_VarFromName(ddm,"entered_lasso");
	    not_entered_lasso = Ddi_BddMakeLiteralAig(elv, 0);
	  }
	  for (i=r; i<l && i<r+limit; i++) {
	    Ddi_Bdd_t *p_i = Ddi_BddNot(Ddi_BddPartRead(dPart,i));
	    if (not_entered_lasso!=NULL) 
	      Ddi_BddOrAcc(p_i,not_entered_lasso);
	    Ddi_BddAndAcc(deltaConstr,p_i);
	    Ddi_Free(p_i);
	  }
	  size1 = Ddi_BddSize(deltaConstr);
	  printf("assumed %d/%d mirror eq - size: %d -> %d\n",
		 fbvCustom, n, size0, size1);
	  Ddi_Free(not_entered_lasso);
	}
	if (custom) {
	  Ddi_Bdd_t *p_i = Ddi_BddPartRead(dPart, Ddi_BddPartNum(dPart)-1);
	  Ddi_Bdd_t *cPart = Ddi_AigPartitionTop(p_i, 0);
	  Ddi_Bdd_t *p_0 = Ddi_BddPartRead(cPart, 0);
	  Ddi_Bdd_t *dPart1 = Ddi_AigPartitionTop(p_0, 1);
	  int k;
	  printf("\nOR prop:\n\n");
	  for (k=0; k<Ddi_BddPartNum(dPart); k++) {
	    Ddi_Bdd_t *p_k = Ddi_BddPartRead(dPart, k);
	    if (Ddi_BddSize(p_k)<4)
	      DdiLogBdd(p_k, 0);
	  }
	  printf("\nconatr component:\n\n");
	  for (k=0; k<Ddi_BddPartNum(dPart1); k++) {
	    Ddi_Bdd_t *p_k = Ddi_BddPartRead(dPart1, k);
	    DdiLogBdd(p_k, 0);
	  }
	  Ddi_Free(cPart);
	  Ddi_Free(dPart1);
	  //	  Ddi_Var_t *v = Ddi_VarFromName(ddm,"I_G_And_And_And_And_And_And_And_And_And_And_And_And_And_And...330");
	  Ddi_Var_t *v = Ddi_VarFromName(ddm,"transformNet_710_1");
	  for (k=0; k<Ddi_BddarrayNum(delta); k++) {
	    Ddi_Bdd_t *d_k = Ddi_BddarrayRead(delta, k);
	    Ddi_Varset_t *s = Ddi_BddSupp(d_k);
	    if (Ddi_VarInVarset(s, v)) {
	      printf("delta[%d]\n",k);
	    } 
	    Ddi_Free(s);
	  }

	}
	if (!custom1 && !custom2)
        while (Ddi_BddPartNum(dPart) > 1) {
          int i;

          Ddi_BddPartSortBySizeAcc(dPart, 1);
          for (i = 0; i < Ddi_BddPartNum(dPart); i++) {
            Ddi_Bdd_t *p_i = Ddi_BddPartRead(dPart, i);
            Ddi_Var_t *v_i;
            int isNeg_i;

            if (Ddi_BddSize(p_i) > 1) {
              break;
            }
            isNeg_i = Ddi_BddIsComplement(p_i);
            v_i = Ddi_BddTopVar(p_i);
            Ddi_BddCofactorAcc(deltaProp2, v_i, isNeg_i);
            if (!Ddi_VarInVarset(piv, v_i)) {
              Ddi_BddPartInsertLast(auxPart, p_i);
              DdiLogBdd(p_i, 0);
            }
            nRed++;
          }
          if (i == 0)
            break;
          Ddi_Free(dPart);
          dPart = Ddi_AigPartitionTop(deltaProp2, 1);
        }
	if (!custom1 && !custom2)
        while (Ddi_BddPartNum(dPart) > 1) {
          int i, neq = 0;

          Ddi_BddPartSortBySizeAcc(dPart, 1);
          for (i = 0; i < Ddi_BddPartNum(dPart); i++) {
            Ddi_Bdd_t *p_i = Ddi_BddPartRead(dPart, i);
            Ddi_Bdd_t *p_i_part;
            Ddi_Bdd_t *l0, *l1;
            Ddi_Var_t *v0, *v1;
            Ddi_Varset_t *s_i;
            int j;

            if (Ddi_BddSize(p_i) != 3) {
              break;
            }
            s_i = Ddi_BddSupp(p_i);
            Pdtutil_Assert(Ddi_VarsetNum(s_i) == 2,
              "two vars required in supp");
            p_i_part = Ddi_AigPartitionTop(p_i, 0);
            Pdtutil_Assert(Ddi_BddPartNum(p_i_part) == 2, "wrong partitions");
            l0 = Ddi_BddPartRead(p_i_part, 0);
            l1 = Ddi_BddPartRead(p_i_part, 1);
            v0 = Ddi_BddTopVar(l0);
            v1 = Ddi_BddTopVar(l1);
            for (j = i + 1; j < Ddi_BddPartNum(dPart); j++) {
              Ddi_Bdd_t *p_j = Ddi_BddPartRead(dPart, j);
              Ddi_Varset_t *s_j;
              Ddi_Varset_t *vCommon;
              int nCommon;

              if (Ddi_BddSize(p_j) != 3) {
                break;
              }
              s_j = Ddi_BddSupp(p_j);
              vCommon = Ddi_VarsetUnion(s_i, s_j);
              nCommon = Ddi_VarsetNum(vCommon);
              Ddi_Free(s_j);
              Ddi_Free(vCommon);
              if (nCommon == 2) {
                Ddi_Bdd_t *p_j_part = Ddi_AigPartitionTop(p_j, 0);

                Pdtutil_Assert(Ddi_BddPartNum(p_j_part) == 2,
                  "wrong partitions");
                if (v0 != Ddi_BddTopVar(Ddi_BddPartRead(p_j_part, 0))) {
                  Ddi_BddPartSwap(p_j_part, 0, 1);
                }
                Ddi_BddNotAcc(Ddi_BddPartRead(p_j_part, 0));
                Ddi_BddNotAcc(Ddi_BddPartRead(p_j_part, 1));
                if (Ddi_BddEqual(l0, Ddi_BddPartRead(p_j_part, 0)) &&
                  Ddi_BddEqual(l1, Ddi_BddPartRead(p_j_part, 1))) {
                  Ddi_Bddarray_t *subst = Ddi_BddarrayAlloc(ddm, 0);
                  Ddi_Vararray_t *vars = Ddi_VararrayAlloc(ddm, 0);

                  if (Ddi_VarInVarset(psv, v1) && Ddi_VarInVarset(psv, v0)) {
                    Ddi_BddPartInsertLast(auxPart, p_i);
                    Ddi_BddPartInsertLast(auxPart, p_j);
                    DdiLogBdd(p_i, 0);
                  }
                  Ddi_BddPartRemove(dPart, j);
                  if (Ddi_VarInVarset(piv, v1) && Ddi_VarInVarset(psv, v0)) {
                    Ddi_Var_t *vt = v0;
                    Ddi_Bdd_t *lt = l0;

                    v0 = v1;
                    v1 = vt;
                    l0 = l1;
                    l1 = lt;
                  }
                  /* prop = eq | other */
                  /* complemented equality is taken !!! */
                  if (!Ddi_BddIsComplement(l0)) {
                    Ddi_BddNotAcc(l1);
                  }
                  Ddi_BddarrayInsert(subst, 0, l1);
                  Ddi_VararrayInsert(vars, 0, v0);
                  Ddi_BddComposeAcc(dPart, vars, subst);
                  Ddi_BddComposeAcc(deltaProp2, vars, subst);
                  Ddi_Free(subst);
                  Ddi_Free(vars);
                  neq++;
                  nRed++;
                }
                Ddi_Free(p_j_part);
                Ddi_Free(s_j);
                break;
              }
            }
            Ddi_Free(s_i);
            Ddi_Free(p_i_part);
          }
          if (neq == 0)
            break;
          Ddi_Free(dPart);
          dPart = Ddi_AigPartitionTop(deltaProp2, 1);
        }
        Ddi_BddSetAig(auxPart);
        Ddi_BddSetAig(deltaProp2);
        Ddi_BddOrAcc(deltaProp2, auxPart);
        Ddi_DataCopy(deltaProp, deltaProp2);

        if (1) {
          Ddi_Bdd_t *litConstr = Ddi_BddMakeLiteralAig(cvarPs, 1);
          Ddi_Bdd_t *litProp = Ddi_BddMakeLiteralAig(pvarPs, 1);

          Ddi_BddAndAcc(deltaProp2, litProp);
          Ddi_BddNotAcc(litConstr);
          Ddi_BddOrAcc(deltaProp2, litConstr);
          Ddi_Free(litConstr);
          Ddi_Free(litProp);
          Ddi_BddarrayWrite(delta, iProp, deltaProp2);
        }
        Ddi_Free(auxPart);
        Ddi_Free(deltaProp2);
      }
      Ddi_Free(dPart);
      Ddi_Free(piv);
      Ddi_Free(psv);
    }

    int constrWithProp = 0;
    if (constrWithProp) {
      int i;
      for (i=0; i<Ddi_BddarrayNum(delta); i++) {
	Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);
	if (i==iProp) continue;
	if (i==iConstr) continue;
	Ddi_BddAndAcc(d_i,deltaProp);
      }
    }

    if (0 && useAig) {
      int enOpt = 0;
      int i, iP = -1, iP0 = -1;
      Ddi_Bdd_t *myPart = NULL, *myPartDecomp = NULL, *myProp = NULL;
      Ddi_Bdd_t *savePartProp = NULL;
      int partPropIsCompl = 0;

      Ddi_Bdd_t *a, *b, *c, *d;

      for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
        Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);
        Ddi_Var_t *v = Ddi_VararrayRead(ps, i);
        Ddi_Bdd_t *cof0 = Ddi_BddCofactor(d_i, v, 0);
        Ddi_Bdd_t *cof1 = Ddi_BddCofactor(d_i, v, 1);

        Ddi_BddNotAcc(cof1);
        if (!Ddi_AigSat(cof0)) {
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, "Fixed Variable: %s @ 0.\n", Ddi_VarName(v))
            );
        }
        if (!Ddi_AigSat(cof1)) {
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, "Fixed Variable: %s @ 1.\n", Ddi_VarName(v))
            );
        }
        Ddi_Free(cof0);
        Ddi_Free(cof1);

      }
      useAig = 0;
    }

    if (useAig && deltaConstr != NULL && Ddi_BddSize(deltaConstr) >= 1) {
      Ddi_Bdd_t *partConstr = Ddi_AigPartitionTop(deltaConstr, 1);
      Ddi_Bdd_t *partProp2 = NULL;
      Ddi_Varset_t *s = Ddi_BddSupp(deltaConstr);
      int nSupp = Ddi_VarsetNum(s);
      int enOpt = 1;
      int i, iP = -1, iP0 = -1;
      Ddi_Bdd_t *myPart = NULL, *myPartDecomp = NULL, *myProp = NULL;
      Ddi_Bdd_t *savePartProp = NULL;
      int partPropIsCompl = 0;

      Ddi_Bdd_t *a, *b, *c, *d;

      Ddi_Free(s);
      if (Ddi_BddPartNum(partConstr) == 1) {
        Ddi_Free(partConstr);
	// do not retime constraint HERE !!!!!! 
	// done in terminalScc & impliedConstr
        if (0 && (nSupp == 1)) {
          Ddi_Varset_t *s = Ddi_BddSupp(deltaConstr);

          Ddi_BddComposeAcc(deltaConstr, ps, delta);
          for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
            Ddi_Var_t *v = Ddi_VararrayRead(ps, i);

            if (Ddi_VarInVarset(s, v)) {
              iP = i;
            }
          }
          Ddi_Free(s);
        } else {
          iP = iConstr;
        }
        enOpt = 1;
        partConstr = Ddi_AigPartitionTop(deltaConstr, 0);
        if (0) {
          Ddi_Bdd_t *p0 = Ddi_BddPartRead(partConstr, 0);
          Ddi_Bdd_t *p0d = Ddi_AigPartitionTop(p0, 1);
          Ddi_Bdd_t *p12 = Ddi_BddPartRead(p0d, 12);
          Ddi_Bdd_t *p12c = Ddi_AigPartitionTop(p12, 0);
          Ddi_Bdd_t *p3 = Ddi_BddPartRead(p12c, 3);
          Ddi_Bdd_t *p3d = Ddi_AigPartitionTop(p3, 1);
          Ddi_Bdd_t *p00 = Ddi_BddPartRead(p3d, 0);
          Ddi_Bdd_t *p00c = Ddi_AigPartitionTop(p00, 0);
          Ddi_Bdd_t *p_0 = Ddi_BddPartRead(p00c, 0);
          Ddi_Bdd_t *p_1 = Ddi_BddPartRead(p00c, 1);

          printf("%d\n", Ddi_BddSize(p3d));
        }
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Constraint: AND[%d].\n", Ddi_BddPartNum(partConstr))
          );
        Ddi_Free(partConstr);
      }

      if (enOpt && doReduce) {
        int simplDone = 0;
        int nRun = 0;
        int enSharedSplit = 0;
        int totSimpl = 0;

        int nEqLatch = 0, nRemLatch = 0;

        do {
          int nFreePi = 0, totPi = 0;
          Ddi_Bdd_t *a = Ddi_AigPartitionTopWithXor(deltaConstr, 0, 1);
          int np = Ddi_BddPartNum(a);
          int ni = Ddi_VararrayNum(pi);
          int nl = Ddi_VararrayNum(ps);
          char **suppMap = Pdtutil_Alloc(char *, np
          );
          char *remPart = Pdtutil_Alloc(char, np
          );
          int *piExist = Pdtutil_Alloc(int, ni
          );
          int *piStats = Pdtutil_Alloc(int, ni
          );
          int *piSort = Pdtutil_Alloc(int, ni
          );
          int *psStats = Pdtutil_Alloc(int, nl
          );
          int *psSort = Pdtutil_Alloc(int, nl
          );
          Ddi_Var_t *topPi, *topPs;
          Ddi_Bdd_t *cof0, *cof1, *dTot;
          Ddi_Bddarray_t *residual;
          Ddi_Varset_t *sR;
          Ddi_Varset_t *psv = Ddi_VarsetMakeFromArray(ps);
          Ddi_Bdd_t *retimedConstr = NULL;
          Ddi_Bdd_t *initState = Fsm_MgrReadInitBDD(fsmMgr);
          Ddi_Bddarray_t *implPi[2] = { NULL, NULL };
          Ddi_Bdd_t *myZero = Ddi_BddMakeConstAig(ddm, 0);
          Ddi_Bdd_t *myOne = Ddi_BddMakeConstAig(ddm, 1);
          Ddi_Bdd_t *newConstr = Ddi_BddMakeConstAig(ddm, 1);
          Ddi_Varsetarray_t *suppA;
          int splitSharedImpl = enSharedSplit, splitNum = 1,
            checkImpl = 1 && (nRun > 0), splitIter = 0;
          retimedConstr = deltaConstr;

          simplDone = 0;
          nRun++;


          //        if (nRun==1)
          while (nRun > 0 && splitSharedImpl && splitNum) {
            Ddi_Bdd_t *b = Ddi_BddMakePartConjVoid(ddm);
            Ddi_Bdd_t *c = Ddi_BddMakePartConjVoid(ddm);
            int chkSplit = 0;   //splitIter>4;

            splitIter++;
            splitNum = 0;
            for (i = 0; i < Ddi_VararrayNum(pi); i++) {
              Ddi_Var_t *v = Ddi_VararrayRead(pi, i);

              Pdtutil_Assert(Ddi_VarReadMark(v) == 0, "0 var mark required");
              Ddi_VarWriteMark(v, 1);
            }
            for (i = 0; i < Ddi_BddPartNum(a); i++) {
              Ddi_Bdd_t *p_i = Ddi_BddPartRead(a, i);
              Ddi_Bdd_t *pp = Ddi_AigPartitionTop(p_i, 1);
              int splitDone = 0;

              if (pp == NULL) {
                /* redundant term found */
                p_i = Ddi_BddPartExtract(a, i);
                i--;
                Ddi_BddNotAcc(p_i);
                Pdtutil_Assert(!Ddi_AigSat(p_i), "wrong redundancy detected");
                Ddi_Free(p_i);
                continue;
              }
              if (Ddi_BddPartNum(pp) > 1) {
                int k;

                if (chkSplit > 2) {
                  Ddi_Bdd_t *aux1 = Ddi_BddMakeAig(pp);

                  Pdtutil_Assert(Ddi_AigEqualSat(aux1, p_i), "wrong split 0");
                  Ddi_Free(aux1);
                }
                for (k = 0; !splitDone && k < Ddi_BddPartNum(pp); k++) {
                  Ddi_Bdd_t *pp_k = Ddi_BddPartRead(pp, k);
                  Ddi_Bdd_t *ppp = Ddi_AigPartitionTop(pp_k, 0);

                  if (ppp == NULL ||
                    (Ddi_BddPartNum(ppp) == 1 &&
                      Ddi_BddIsZero(Ddi_BddPartRead(ppp, 0)))) {
                    /* redundant term found */
                    pp_k = Ddi_BddPartExtract(pp, k);
                    k--;
                    Pdtutil_Assert(!Ddi_AigSat(pp_k),
                      "wrong redundancy detected");
                    Ddi_Free(pp_k);
                    continue;
                  }
                  if (Ddi_BddPartNum(ppp) > 1) {
                    int kk;
                    Ddi_Bdd_t *extr = Ddi_BddMakePartConjVoid(ddm);

                    for (kk = Ddi_BddPartNum(ppp) - 1; kk >= 0; kk--) {
                      Ddi_Bdd_t *ppp_k = Ddi_BddPartRead(ppp, kk);

                      if (Ddi_BddSize(ppp_k) == 1) {
                        Ddi_Var_t *v = Ddi_BddTopVar(ppp_k);

                        if (Ddi_VarReadMark(v) != 0) {
                          ppp_k = Ddi_BddPartExtract(ppp, kk);
                          Ddi_BddPartInsertLast(extr, ppp_k);
                          Ddi_Free(ppp_k);
                        }
                      }
                    }
                    if (Ddi_BddPartNum(extr) > 0) {
                      /* recompute ppp */
                      int residualPart = Ddi_BddPartNum(ppp) > 0;

                      if (chkSplit > 1) {
                        Ddi_Bdd_t *aux = Ddi_BddMakeAig(extr);
                        Ddi_Bdd_t *aux1 = Ddi_BddMakeAig(ppp);

                        Ddi_BddAndAcc(aux, aux1);
                        Pdtutil_Assert(Ddi_AigEqualSat(aux, pp_k),
                          "wrong split");
                        Ddi_Free(aux1);
                        aux1 = Ddi_BddMakeAig(pp);
                        Pdtutil_Assert(Ddi_AigEqualSat(aux1, p_i),
                          "wrong spli 0t");
                        Ddi_Free(aux);
                        Ddi_Free(aux1);
                      }

                      Ddi_BddSetAig(ppp);
                      pp_k = Ddi_BddPartExtract(pp, k);
                      Ddi_BddSetAig(pp);
                      Ddi_Free(pp_k);

                      if (chkSplit > 1) {
                        Ddi_Bdd_t *aux = Ddi_BddMakeAig(extr);
                        Ddi_Bdd_t *aux1 = Ddi_BddMakeAig(ppp);

                        Ddi_BddAndAcc(aux, aux1);
                        Ddi_Free(aux1);
                        aux1 = Ddi_BddOr(pp, pp_k);
                        Pdtutil_Assert(Ddi_AigEqualSat(aux1, p_i),
                          "wrong spli 0t");
                        Ddi_BddOrAcc(aux, pp);
                        Pdtutil_Assert(Ddi_AigEqualSat(aux, p_i),
                          "wrong split 1");
                        Ddi_Free(aux);
                        Ddi_Free(aux1);
                      }

                      for (kk = 0; kk < Ddi_BddPartNum(extr); kk++) {
                        Ddi_Bdd_t *term =
                          Ddi_BddDup(Ddi_BddPartRead(extr, kk));
                        Ddi_BddOrAcc(term, pp);
                        Ddi_BddPartInsertLast(b, term);
                        Ddi_Free(term);
                      }
                      if (residualPart) {
                        Ddi_BddOrAcc(pp, ppp);
                        Ddi_BddPartInsertLast(b, pp);
                      }
                      splitDone = 1;
                      splitNum++;
                    }
                    Ddi_Free(extr);
                  }
                  Ddi_Free(ppp);
                }
              }
              Ddi_Free(pp);
              if (!splitDone) {
                Ddi_BddPartInsertLast(b, p_i);
              }
#if 1
              Ddi_BddPartInsertLast(c, p_i);
              if ((chkSplit > 1) && splitDone) {
                Ddi_Bdd_t *aa = Ddi_BddMakeAig(c);
                Ddi_Bdd_t *bb = Ddi_BddMakeAig(b);

                Pdtutil_Assert(Ddi_AigEqualSat(aa, bb), "wrong split");
                Ddi_Free(aa);
                Ddi_Free(bb);
              }
#endif
            }
            for (i = 0; i < Ddi_VararrayNum(pi); i++) {
              Ddi_Var_t *v = Ddi_VararrayRead(pi, i);

              Ddi_VarWriteMark(v, 0);
            }
            if (chkSplit) {
              Ddi_Bdd_t *aa = Ddi_BddMakeAig(a);
              Ddi_Bdd_t *bb = Ddi_BddMakeAig(b);

              Pdtutil_Assert(Ddi_AigEqualSat(aa, bb), "wrong split");
              Ddi_Free(aa);
              Ddi_Free(bb);
            }
            Ddi_DataCopy(a, b);
            Ddi_Free(b);
            Ddi_Free(c);
          }

          suppA = Ddi_VarsetarrayAlloc(ddm, Ddi_BddPartNum(a));
          for (i = 0; i < Ddi_BddPartNum(a); i++) {
            Ddi_Bdd_t *p_i = Ddi_BddPartRead(a, i);
            Ddi_Varset_t *si = Ddi_BddSupp(p_i);

            Ddi_VarsetSetArray(si);
            Ddi_VarsetarrayWrite(suppA, i, si);
            Ddi_Free(si);
          }

	  int nEqGate=0;
          for (i = Ddi_BddPartNum(a) - 1; 1 && i >= 0; i--) {
            int j, l;
            Ddi_Bdd_t *p_i = Ddi_BddPartRead(a, i);
            Ddi_Varset_t *si = Ddi_VarsetarrayRead(suppA, i);
            int ni = Ddi_VarsetNum(si);

            for (j = i - 1; j >= 0 && (nl < 1500 || j >= i - 100); j--) {
              Ddi_Bdd_t *p_j = Ddi_BddPartRead(a, j);
              Ddi_Varset_t *sj = Ddi_VarsetarrayRead(suppA, j);
              int nj = Ddi_VarsetNum(sj);

              if (ni == nj) {
                Ddi_Varset_t *s0 = Ddi_VarsetDiff(sj, si);

                if (Ddi_VarsetIsVoid(s0)) {
                  int isEq = Ddi_AigCheckEqGate(ddm, bAig_NULL,
                    Ddi_BddToBaig(p_j), Ddi_BddToBaig(p_i), NULL, NULL, 0);

                  if (isEq) {
                    Ddi_BddAndAcc(p_j, p_i);
                    Ddi_BddPartRemove(a, i);
		    nEqGate++;
                  }
                  Ddi_Free(s0);
                  break;
                }
                Ddi_Free(s0);
              }
            }
          }
          Ddi_Free(suppA);

          for (i = 0; i < Ddi_VararrayNum(ps); i++) {
            Ddi_Var_t *v = Ddi_VararrayRead(ps, i);

            Pdtutil_Assert(Ddi_VarReadMark(v) == 0, "0 var mark required");
            Ddi_VarWriteMark(v, 1);
          }

#define COMPOSE_TOT 0
#if COMPOSE_TOT
	  Ddi_Bddarray_t *substTot = Ddi_BddarrayAlloc(ddm, 0);
	  Ddi_Vararray_t *varsTot = Ddi_VararrayAlloc(ddm, 0);
#endif
          for (i = Ddi_BddPartNum(a) - 1; 1 && ( /*nRun==1 && */ i >= 0); i--) {
            int j, l;
            Ddi_Bdd_t *p = Ddi_BddPartRead(a, i);
            Ddi_Varset_t *si = Ddi_BddSupp(p);
            Ddi_Vararray_t *siA = Ddi_VararrayMakeFromVarset(si, 1);
            int nextPart = 0;

            if (Ddi_VarsetNum(si) == 2 &&
              //            Ddi_VarInVarset(psv,Ddi_VararrayRead(siA,0)) &&
              //            Ddi_VarInVarset(psv,Ddi_VararrayRead(siA,1))
              Ddi_VarReadMark(Ddi_VararrayRead(siA, 0)) &&
              Ddi_VarReadMark(Ddi_VararrayRead(siA, 1))
              ) {
              Ddi_Var_t *v0 = Ddi_VararrayRead(siA, 0);
              Ddi_Var_t *v1 = Ddi_VararrayRead(siA, 1);

              // if (Ddi_VarInVarset(psv,v0) && Ddi_VarInVarset(psv,v1)) {
              if (v0!=firstEqVar && v1!=firstEqVar &&
		  Ddi_VarReadMark(v0) && Ddi_VarReadMark(v1)) {
                Ddi_Bdd_t *cof0 = Ddi_BddCofactor(p, v0, 0);
                Ddi_Bdd_t *cof1 = Ddi_BddCofactor(p, v0, 1);

                if (!Ddi_AigSatAnd(cof0, cof1, NULL)) {
                  Ddi_Bdd_t *cof01 = Ddi_BddOr(cof0, cof1);

                  Ddi_BddNotAcc(cof01);
                  if (!Ddi_AigSat(cof01)) {
                    /* v0=v1 found */
                    int complement = 0;
                    int remLatch = -1;
                    Ddi_Bdd_t *v0Next = NULL;
                    Ddi_Bdd_t *v1Next = NULL;
                    int j0, j1;
                    Ddi_Bddarray_t *subst = Ddi_BddarrayAlloc(ddm, 0);
                    Ddi_Vararray_t *vars = Ddi_VararrayAlloc(ddm, 0);
                    Ddi_Bdd_t *diffP = NULL;

		    Pdtutil_VerbosityMgr(fsmMgr,
		      Pdtutil_VerbLevelDevMed_c,
                      fprintf(stdout, "Equivalence Found For %s-%s.\n",
                        Ddi_VarName(v0), Ddi_VarName(v1))
                      );
                    //DdiLogBdd(p,0);
                    nEquiv++;
		    nRemLatch++;

                    if (1 || (nEquiv < 10)) {
                      //if (nEquiv==4 || nEquiv==5 || nEquiv==9 || nEquiv==10 ||
                      //  (nEquiv==18) || nEquiv >= 30) {
                      //     fprintf(stdout, "!!! %d.\n", nEquiv);
#if 1
                      Ddi_BddarrayWrite(subst, 0, cof1);
                      Ddi_VararrayWrite(vars, 0, v0);
#endif
                      if (Ddi_BddIsComplement(cof1)) {
			Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
			  Pdtutil_VerbLevelNone_c,
                          fprintf(stdout, "Complement.\n")
                          );
                        complement = 1;
                      }
                      for (j = 0; j < Ddi_VararrayNum(ps); j++) {
                        Ddi_Bdd_t *p_j = Ddi_BddarrayRead(delta, j);

                        if (Ddi_VararrayRead(ps, j) == v0) {
                          v0Next = Ddi_BddDup(p_j);
			  remLatch = j0 = j;
                        } else if (Ddi_VararrayRead(ps, j) == v1) {
                          v1Next = Ddi_BddDup(p_j);
                          j1 = j;
                        }
                      }
                      /* check stub */

                      if (1 && v0Next != NULL && v1Next != NULL) {
                        //DdiLogBdd(Ddi_BddarrayRead(delta,jv),0);
                        //DdiLogBdd(Ddi_BddarrayRead(delta,jv),0);
                        /* violated constraint and !retimedProp */
                        diffP = Ddi_BddXor(v0Next, v1Next);
                        if (complement)
                          Ddi_BddNotAcc(diffP);
                      }

		      // USE for dbg
		      // if (nRemLatch>10)  remLatch = -1;
		      if (0&(remLatch >= 0)) {
			// disabled - this check is not needed 
			//as this is indeed a constraint on init
			Ddi_Bdd_t *check = Ddi_BddMakeConstAig(ddm,1);
			if (initStub == NULL) {
			  /* check for init states out of scc */
			  Ddi_AigAndCubeAcc(check, initState);
			} else {
			  Ddi_BddComposeAcc(diffP,ps,initStub);
			}
			if (Ddi_AigSat(check)) {
			  remLatch = -1;
			}
			Ddi_Free(check);
		      }

                      Ddi_Free(v0Next);
                      Ddi_Free(v1Next);

                      p = Ddi_BddPartExtract(a, i);
                      np--;

#if COMPOSE_TOT
		      Ddi_VararrayAppend(varsTot,vars);
		      Ddi_BddarrayAppend(substTot,subst);
		      Ddi_VarWriteMark(v0,0);
		      Ddi_VarWriteMark(v1,0);
#else
                      Ddi_BddComposeAcc(a, vars, subst);
                      Ddi_BddComposeAcc(newConstr, vars, subst);
                      if (retimedConstr != NULL)
                        Ddi_BddComposeAcc(retimedConstr, vars, subst);
                      Ddi_AigarrayComposeAcc(delta, vars, subst);
                      if (diffP != NULL)
                        Ddi_BddComposeAcc(diffP, vars, subst);
#endif

#if 0
		      // already done in firstEq
		      if (nRemLatch == 1) {
			/* first latch: 
			   keep and set as retimed constr latch */
			remLatchConstrVar = v0;
			remLatchConstr = Ddi_BddNot(diffP);
		      }
		      else {
			Ddi_BddDiffAcc(remLatchConstr,diffP);
		      }
#endif


		      /* retime difference check on previous state */
		      Ddi_Bdd_t *stubOK = NULL;

		      if (initStub != NULL) {
			stubOK = Ddi_BddXnor(Ddi_BddarrayRead(initStub, j0),
				  Ddi_BddarrayRead(initStub, j1));
			if (complement)
			  Ddi_BddNotAcc(stubOK);
			// if (complementRetimed) Ddi_BddNotAcc(stubOK);
		      }

		      if (!(remLatch>=0)) {
			Ddi_BddAndAcc(newConstr, p);
			Ddi_BddAndAcc(Ddi_BddarrayRead(initStub, iConstr),
				    stubOK);
		      }
		      if (remLatch>=0) {
			// Ddi_BddAndAcc(Ddi_BddarrayRead(delta,iP),eqOK);
			if (firstEq < 0) {
			  Ddi_Bdd_t *eqOK = Ddi_BddMakeLiteralAig(v0, 1);
			  
			  firstEq = remLatch;
			  firstEqVar = v0;
			  // removed delta replaced with EQ constrain
			  Ddi_BddAndAcc(newConstr, eqOK);
			  if (initStub == NULL) {
			    // first remove latch with original fphase
			    Ddi_BddComposeAcc(initState, vars, subst);
			    if (Ddi_BddIsZero(initState)) {
			      printf("init state violates a constrain\n");
			    }
			    // then set init value at 1
			    Ddi_BddAndAcc(initState, eqOK);
			  }
			  Ddi_Free(eqOK);
			  
			  Ddi_BddarrayWrite(delta, firstEq, myOne);
			  if (initStub != NULL) {
			    Ddi_BddarrayWrite(initStub, firstEq, myOne);
			  }
			}
                        /* delta[firstEq] &= !diffP */

                        if (initStub != NULL) {
                          // Ddi_BddarrayWrite(initStub,jv,myOne);
                          Ddi_BddAndAcc(Ddi_BddarrayRead(initStub, firstEq),
                            stubOK);
                        }
		      
                        Ddi_BddDiffAcc(Ddi_BddarrayRead(delta, firstEq),
                          diffP);
		      }

		      Ddi_Free(stubOK);

                      Ddi_Free(diffP);
                      Ddi_Free(p);
                    }
                    Ddi_Free(subst);
                    Ddi_Free(vars);
                  }
                  Ddi_Free(cof01);
                }
                Ddi_Free(cof0);
                Ddi_Free(cof1);
              }
            } else if (Ddi_VarsetNum(si) == 1 && 1 &&
              //            Ddi_VarInVarset(psv,Ddi_VararrayRead(siA,0)) &&
              //            Ddi_VarInVarset(psv,Ddi_VararrayRead(siA,1))
              Ddi_VarReadMark(Ddi_VararrayRead(siA, 0))) {
              // implied var
              Ddi_Var_t *v0 = Ddi_VararrayRead(siA, 0);

              // if (Ddi_VarInVarset(psv,v0) && Ddi_VarInVarset(psv,v1)) {
              if (Ddi_VarReadMark(v0) && v0!=firstEqVar) {
                /* v0=v1 found */
                int complement = Ddi_BddIsComplement(p);

                int complementRetimed = 0;
                int remLatch = -1;
                Ddi_Bdd_t *v0Next = NULL;
                int j0;
                Ddi_Bddarray_t *subst = Ddi_BddarrayAlloc(ddm, 0);
                Ddi_Vararray_t *vars = Ddi_VararrayAlloc(ddm, 0);
                Ddi_Bdd_t *diffP = NULL;
                Ddi_Bdd_t *replConst = Ddi_BddMakeConstAig(ddm,
                  !complement);

		Pdtutil_VerbosityMgr(fsmMgr,
		  Pdtutil_VerbLevelDevMed_c,
                  fprintf(stdout, "equivalence found for %s-%d.\n",
                    Ddi_VarName(v0), !complement)
                  );
                //DdiLogBdd(p,0);
                nEquiv0++;
                if (1 && (1 || (nEquiv0 < 10))) {
                  //if (nEquiv==4 || nEquiv==5 || nEquiv==9 || nEquiv==10 ||
                  //  (nEquiv==18) || nEquiv >= 30) {
                  //     fprintf(stdout, "!!! %d.\n", nEquiv);
#if 1
                  Ddi_BddarrayWrite(subst, 0, replConst);
                  Ddi_VararrayWrite(vars, 0, v0);
#endif
		  for (j = 0; j < Ddi_VararrayNum(ps); j++) {
		    Ddi_Bdd_t *p_j = Ddi_BddarrayRead(delta, j);
		    
		    if (Ddi_VararrayRead(ps, j) == v0) {
		      v0Next = Ddi_BddDup(p_j);
		      remLatch = j0 = j;
		    }
		  }

		  if (1 && v0Next != NULL) {
		    //DdiLogBdd(Ddi_BddarrayRead(delta,jv),0);
		    //DdiLogBdd(Ddi_BddarrayRead(delta,jv),0);
		    /* violated constraint and !retimedProp */
		    diffP = Ddi_BddNot(v0Next);
		    if (complement)
		      Ddi_BddNotAcc(diffP);
		  }
		  
		  // USE for dbg
		  // if (nRemLatch>10) 
		  //		  remLatch = -1;
		  if (0&(remLatch >= 0)) {
		    // disabled - this check is not needed 
		    //as this is indeed a constraint on init
		    Ddi_Bdd_t *check = Ddi_BddMakeConstAig(ddm,1);
		    if (initStub == NULL) {
		      /* check for init states out of scc */
		      Ddi_AigAndCubeAcc(check, initState);
		    } else {
		      Ddi_BddComposeAcc(diffP,ps,initStub);
		    }
		    if (Ddi_AigSat(check)) {
		      remLatch = -1;
		    }
		    Ddi_Free(check);
		  }
		  
		  Ddi_Free(v0Next);
		  
		  p = Ddi_BddPartExtract(a, i);
		  np--;
		  
#if COMPOSE_TOT
		  Ddi_VararrayAppend(varsTot,vars);
		  Ddi_BddarrayAppend(substTot,subst);
		  Ddi_VarWriteMark(v0,0);
#else
                  Ddi_BddComposeAcc(a, vars, subst);
                  Ddi_BddComposeAcc(newConstr, vars, subst);
                  if (retimedConstr != NULL)
                    Ddi_BddComposeAcc(retimedConstr, vars, subst);
                  Ddi_AigarrayComposeAcc(delta, vars, subst);
		  // Ddi_BddComposeAcc(initState, vars, subst);
#endif

		  /* retime difference check on previous state */
		  Ddi_Bdd_t *stubOK = NULL;
		  
		  if (initStub != NULL) {
		    stubOK = Ddi_BddDup(Ddi_BddarrayRead(initStub, j0));
		    if (complement)
		      Ddi_BddNotAcc(stubOK);
		    // if (complementRetimed) Ddi_BddNotAcc(stubOK);
		  }
		  
		  if (!(remLatch>=0)) {
		    Ddi_BddAndAcc(newConstr, p);
		    Ddi_BddAndAcc(Ddi_BddarrayRead(initStub, iConstr),
				  stubOK);
		  }
		  if (remLatch>=0) {
		    // Ddi_BddAndAcc(Ddi_BddarrayRead(delta,iP),eqOK);
		    if (firstEq < 0) {
		      Ddi_Bdd_t *eqOK = Ddi_BddMakeLiteralAig(v0, 1);
			  
		      firstEq = remLatch;
		      firstEqVar = v0;
		      // removed delta replaced with EQ constrain
		      Ddi_BddAndAcc(newConstr, eqOK);
		      if (initStub == NULL) {
			// first remove latch with original phase
			Ddi_BddComposeAcc(initState, vars, subst);
			if (Ddi_BddIsZero(initState)) {
			  printf("init state violates a constrain\n");
			}
			// then set init value at 1
			Ddi_BddAndAcc(initState, eqOK);
		      }
		      Ddi_Free(eqOK);
		      
		      Ddi_BddarrayWrite(delta, firstEq, myOne);
		      if (initStub != NULL) {
			Ddi_BddarrayWrite(initStub, firstEq, myOne);
		      }
		    }
		    /* delta[firstEq] &= !diffP */
		    
		    if (initStub != NULL) {
		      // Ddi_BddarrayWrite(initStub,jv,myOne);
		      Ddi_BddAndAcc(Ddi_BddarrayRead(initStub, firstEq),
				    stubOK);
		    }
		    
		    Ddi_BddDiffAcc(Ddi_BddarrayRead(delta, firstEq),
				   diffP);
		  }
		  
		  Ddi_Free(stubOK);
		  
                  Ddi_Free(diffP);
                  Ddi_Free(p);
                }
                Ddi_Free(subst);
                Ddi_Free(vars);
                Ddi_Free(replConst);
              }
            }
            Ddi_Free(si);
            Ddi_Free(siA);
          }

#if COMPOSE_TOT
	  Ddi_BddComposeAcc(a, varsTot, substTot);
	  //	  Ddi_BddComposeAcc(newConstr, varsTot, substTot);
	  if (retimedConstr != NULL)
	    Ddi_BddComposeAcc(retimedConstr, varsTot, substTot);
	  Ddi_AigarrayComposeAcc(delta, varsTot, substTot);
	  //	  Ddi_BddComposeAcc(initState, varsTot, substTot);
	  Ddi_Free(varsTot);
	  Ddi_Free(substTot);
#endif

          for (i = 0; i < Ddi_VararrayNum(ps); i++) {
            Ddi_Var_t *v = Ddi_VararrayRead(ps, i);

            Ddi_VarWriteMark(v, -1);
          }

          implPi[0] = Ddi_BddarrayAlloc(ddm, Ddi_VararrayNum(pi));
          implPi[1] = Ddi_BddarrayAlloc(ddm, Ddi_VararrayNum(pi));
          for (i = 0; i < Ddi_VararrayNum(pi); i++) {
            Ddi_Var_t *v = Ddi_VararrayRead(pi, i);

            Pdtutil_Assert(Ddi_VarReadMark(v) == 0, "0 var mark required");
            Ddi_VarWriteMark(v, i + 1);

            Ddi_BddarrayWrite(implPi[0], i, myZero);
            Ddi_BddarrayWrite(implPi[1], i, myZero);
          }

#if COMPOSE_TOT
	  Ddi_Bddarray_t *substTot0 = Ddi_BddarrayAlloc(ddm, 0);
	  Ddi_Vararray_t *varsTot0 = Ddi_VararrayAlloc(ddm, 0);
	  substTot = Ddi_BddarrayAlloc(ddm, 0);
	  varsTot = Ddi_VararrayAlloc(ddm, 0);
#endif
          for (i = Ddi_BddPartNum(a) - 1; /*totSimpl<3 && */ i >= 0; i--) {
            int j, l;
            Ddi_Bdd_t *p = Ddi_BddPartRead(a, i);
            Ddi_Varset_t *si = NULL;
            Ddi_Vararray_t *siA = NULL;
            int eqLatchFound = 0;
            int eqFound = 0;
            int implFound = 0;
            Ddi_Bdd_t *residualP = NULL;
            bAigEdge_t eq0, eq1;
            Ddi_Var_t *vEq = NULL, *vImpl = NULL, *vEqL=NULL;
            Ddi_Bdd_t *cof0 = NULL, *cof1 = NULL, *cofL=NULL;
            int isEq = Ddi_AigCheckEqGate(ddm, Ddi_BddToBaig(p),
              NULL, NULL, &eq0, &eq1, 0);

            if (isEq && bAig_isVarNode(bmgr, eq0)) {
              vEq = Ddi_VarFromBaig(ddm, eq0);
              if (Ddi_VarReadMark(vEq) > 0) {
                cof1 = Ddi_BddMakeFromBaig(ddm, eq1);
                if (bAig_NodeIsInverted(eq0))
                  Ddi_BddNotAcc(cof1);
                eqFound = 1;
		eqLatchFound = 0;
              }
	      else if (Ddi_VarReadMark(vEq) == 1) {
		vEqL = vEq;
		cofL = Ddi_BddMakeFromBaig(ddm, eq1);
                if (bAig_NodeIsInverted(eq0))
                  Ddi_BddNotAcc(cofL);
		eqLatchFound = 1;
	      }
            }
            if (isEq && !eqFound && bAig_isVarNode(bmgr, eq1)
              && !bAig_NodeIsConstant(eq1)) {
              vEq = Ddi_VarFromBaig(ddm, eq1);
              if (Ddi_VarReadMark(vEq) > 0) {
                cof1 = Ddi_BddMakeFromBaig(ddm, eq0);
		Ddi_Varset_t *s = Ddi_BddSupp(cof1);
		if (!Ddi_VarInVarset(s,vEq)) {
		  if (bAig_NodeIsInverted(eq1))
		    Ddi_BddNotAcc(cof1);
		  eqFound = 1;
		  eqLatchFound = 0;
		}
		Ddi_Free(s);
              }
	      else if (Ddi_VarReadMark(vEq) == 1) {
		vEqL = vEq;
		Ddi_Free(cofL);
		cofL = Ddi_BddMakeFromBaig(ddm, eq0);
                if (bAig_NodeIsInverted(eq1))
                  Ddi_BddNotAcc(cofL);
		eqLatchFound = 1;
	      }
            }
	    if (eqLatchFound) {
	      nEqLatch++;
	    }
            if (!isEq && !eqLatchFound && checkImpl) {
              Ddi_Bdd_t *pp = Ddi_AigPartitionTop(p, 1);

              if (Ddi_BddPartNum(pp) > 1) {
                int k;
#if 0
		Ddi_Bdd_t *cubeConstr = Ddi_BddMakeConstAig(ddm, 1);
                for (k = 0; k < Ddi_BddPartNum(pp); k++) {
                  Ddi_Bdd_t *pp_k = Ddi_BddPartRead(pp, k);
		  if (Ddi_BddSize(pp_k)==1) {
		    Ddi_BddDiffAcc(cubeConstr,pp_k);
		  }
		}
		if (!Ddi_BddIsOne(cubeConstr)) {
		  for (k = 0; k < Ddi_BddPartNum(pp); k++) {
		    Ddi_Bdd_t *pp_k = Ddi_BddPartRead(pp, k);
		    if (Ddi_BddSize(pp_k)>1) {
		      Ddi_AigConstrainCubeAcc(pp_k, cubeConstr);
		    }
		  }
		  Ddi_AigConstrainCubeAcc(p, cubeConstr);
		  Ddi_BddNotAcc(cubeConstr);
		  Ddi_BddOrAcc(p,cubeConstr);
		}
		Ddi_Free(cubeConstr);
#endif
                for (k = 0; k < Ddi_BddPartNum(pp); k++) {
                  Ddi_Bdd_t *pp_k = Ddi_BddPartRead(pp, k);

                  if (Ddi_BddSize(pp_k) == 1) {
                    vImpl = Ddi_BddTopVar(pp_k);
                    if (Ddi_VarReadMark(vImpl) > 0) {
                      int ii = (int)(Ddi_VarReadMark(vImpl) - 1);

                      pp_k = Ddi_BddPartExtract(pp, k);
                      Ddi_BddSetAig(pp);
                      if (!Ddi_BddIsComplement(pp_k)) {
                        /* constrain is pp_k | pp (!pp -> vImpl) */
                        cof1 = Ddi_BddNot(pp);
                        Ddi_BddarrayWrite(implPi[1], ii, cof1);
                        Ddi_Free(cof1);
                      } else {
                        /* constrain is !pp_k | pp (!pp -> !vImpl) */
                        Ddi_BddarrayWrite(implPi[0], ii, pp);
                      }
                      Ddi_Free(pp_k);
		      Pdtutil_VerbosityMgr(fsmMgr,
			 Pdtutil_VerbLevelDevMed_c,
			 printf("implication for pi %d\n", ii)
		      );
		      nImpl++;
                      implFound = 1;
                      break;
                    }
                  }
                }
              }
              Ddi_Free(pp);
            }
            si = Ddi_BddSupp(p);
            siA = Ddi_VararrayMakeFromVarset(si, 1);
            for (j = 0; 0 && nRun > 0 && !eqFound && !implFound &&
              j < Ddi_VararrayNum(siA); j++) {
              Ddi_Var_t *v = Ddi_VararrayRead(siA, j);

              // for (l=0; !nextPart && l<ni; l++) {
              // if (v == Ddi_VararrayRead(pi,l)) {
              if (Ddi_VarReadMark(v) > 0) {
                cof0 = Ddi_BddCofactor(p, v, 0);
                cof1 = Ddi_BddCofactor(p, v, 1);
                if (!Ddi_AigSatAnd(cof0, cof1, NULL)) {
                  Ddi_Bdd_t *cof01 = Ddi_BddOr(cof0, cof1);

                  Ddi_BddNotAcc(cof01);
                  if (0 && Ddi_AigSat(cof01)) {
                    /* this part requires debugging: intel003 fails */
                    int jj;
                    Ddi_Bdd_t *pp = Ddi_AigPartitionTop(p, 0);

                    for (jj = Ddi_BddPartNum(pp) - 1; jj >= 0; jj--) {
                      Ddi_Bdd_t *pp_jj = Ddi_BddPartRead(pp, jj);
                      Ddi_Varset_t *s_jj = Ddi_BddSupp(pp_jj);

                      if (!Ddi_VarInVarset(s_jj, v)) {
                        if (residualP == NULL) {
                          residualP = Ddi_BddMakeConstAig(ddm, 1);
                        }
                        Ddi_BddAndAcc(residualP, pp_jj);
                        Ddi_BddPartRemove(pp, jj);
                      }
                      Ddi_Free(s_jj);
                    }
                    if (residualP != NULL) {
                      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
                        Pdtutil_VerbLevelNone_c,
                        fprintf(stdout,
                          "potential composition Uncovered for %s.\n",
                          Ddi_VarName(v))
                        );
                      Ddi_BddSetAig(pp);
                      Ddi_Free(cof0);
                      Ddi_Free(cof1);
                      Ddi_Free(cof01);
                      cof0 = Ddi_BddCofactor(pp, v, 0);
                      cof1 = Ddi_BddCofactor(pp, v, 1);
                      cof01 = Ddi_BddOr(cof0, cof1);
                      Ddi_BddNotAcc(cof01);
                    } else {
                      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
                        Pdtutil_VerbLevelUsrMax_c,
                        fprintf(stdout, "Potential Composition Hidden.\n")
                        );
                    }
                    Ddi_Free(pp);
                  }
                  if (!Ddi_AigSat(cof01)) {
                    vEq = v;
                    eqFound = 1;
                    Ddi_Free(cof0);
                    Ddi_Free(cof01);
                    break;
                  }
                  Ddi_Free(cof01);
                }
                Ddi_Free(cof0);
                Ddi_Free(cof1);
              }
            }

            if (eqFound)
              totSimpl++;

            if (1 && eqLatchFound) {
#if COMPOSE_TOT
              Ddi_BddarrayInsertLast(substTot0, cofL);
              Ddi_VararrayInsertLast(varsTot0, vEqL);
#endif
	    }
            else if (eqFound) {
              int checkEq = 0;

              /* v=f found */
              Ddi_Bddarray_t *subst = Ddi_BddarrayAlloc(ddm, 0);
              Ddi_Vararray_t *vars = Ddi_VararrayAlloc(ddm, 0);

              Pdtutil_VerbosityMgr(fsmMgr,
                Pdtutil_VerbLevelDevMed_c,
                fprintf(stdout, "Composition Found for %s.\n",
                  Ddi_VarName(vEq))
                );
              piComp++;
              simplDone = 1;
              Ddi_BddarrayWrite(subst, 0, cof1);
              Ddi_VararrayWrite(vars, 0, vEq);
              if (checkEq) {
                Ddi_Bdd_t *p1 = Ddi_BddMakeLiteralAig(vEq, 1);

                Ddi_BddXnorAcc(p1, cof1);
                Ddi_BddXorAcc(p1, p);
                Pdtutil_Assert(!Ddi_AigSat(p1), "wrong pi composition");
                Ddi_Free(p1);
              }
              if (residualP != NULL) {
                Ddi_BddPartWrite(a, i, residualP);
              } else {
                Ddi_BddPartRemove(a, i);
              }
              np--;

#if COMPOSE_TOT
	      Ddi_VararrayAppend(varsTot,vars);
	      Ddi_BddarrayAppend(substTot,subst);
	      Ddi_VarWriteMark(vEq,0);
#else
              Ddi_BddComposeAcc(a, vars, subst);
              Ddi_BddComposeAcc(newConstr, vars, subst);
              if (retimedConstr != NULL)
                Ddi_BddComposeAcc(retimedConstr, vars, subst);
              Ddi_BddarrayComposeAcc(delta, vars, subst);
              if (checkImpl) {
                Ddi_BddarrayComposeAcc(implPi[0], vars, subst);
                Ddi_BddarrayComposeAcc(implPi[1], vars, subst);
              }
#endif

              Ddi_Free(subst);
              Ddi_Free(vars);
              Ddi_Free(residualP);
            } else if (implFound) {
              Ddi_BddPartRemove(a, i);
              np--;
            }
            Ddi_Free(cof0);
            Ddi_Free(cof1);
	    Ddi_Free(cofL);
            Ddi_Free(residualP);
            Ddi_Free(si);
            Ddi_Free(siA);
            if (1 && implFound) {
            }
          }

          if (checkImpl) {
            Ddi_Bddarray_t *subst = Ddi_BddarrayAlloc(ddm, 0);
            Ddi_Vararray_t *vars = Ddi_VararrayAlloc(ddm, 0);

            for (i = 0; i < Ddi_VararrayNum(pi); i++) {
              Ddi_Bdd_t *im0 = Ddi_BddarrayRead(implPi[0], i);
              Ddi_Bdd_t *im1 = Ddi_BddarrayRead(implPi[1], i);
	      Ddi_Bdd_t *im1b = Ddi_BddNot(im1);
              Ddi_Var_t *v_i = Ddi_VararrayRead(pi, i);
              Ddi_Bdd_t *s_i = Ddi_BddMakeLiteralAig(v_i, 1);
              int implied = 0;

              if (!Ddi_BddIsZero(im0) && !Ddi_BddIsZero(im1)) {
                if (Ddi_BddEqual(im0, im1b)) {
                  Ddi_VararrayInsertLast(vars, v_i);
                  Ddi_BddarrayInsertLast(subst, im1);
                  Ddi_DataCopy(im0, myZero);
                  Ddi_DataCopy(im1, myZero);
                }
              }
              if (!Ddi_BddIsZero(im0)) {
                Ddi_Bdd_t *im;
		Ddi_BddNotAcc(s_i);
                im = Ddi_BddOr(s_i, im0);
		Ddi_BddNotAcc(s_i);

                Ddi_BddAndAcc(newConstr, im);
                Ddi_Free(im);
              }
              if (!Ddi_BddIsZero(im1)) {
                Ddi_Bdd_t *im = Ddi_BddOr(im1b, s_i);
                Ddi_BddAndAcc(newConstr, im);
                Ddi_Free(im);
              }
	      Ddi_Free(im1b);
              Ddi_Free(s_i);
            }
            if (Ddi_VararrayNum(vars) > 0) {
              Ddi_BddComposeAcc(a, vars, subst);
              Ddi_BddComposeAcc(newConstr, vars, subst);
              Ddi_BddarrayComposeAcc(delta, vars, subst);
            }
            Ddi_Free(vars);
            Ddi_Free(subst);
          }

          for (i = 0; i < Ddi_VararrayNum(pi); i++) {
            Ddi_Var_t *v = Ddi_VararrayRead(pi, i);

            Ddi_VarWriteMark(v, 0);
          }
          for (i = 0; i < Ddi_VararrayNum(ps); i++) {
            Ddi_Var_t *v = Ddi_VararrayRead(ps, i);

            Ddi_VarWriteMark(v, 0);
          }

#if COMPOSE_TOT
	  if (Ddi_VararrayNum(varsTot0)>0) {
	    Ddi_BddarrayComposeAcc(substTot0, varsTot, substTot);
	    Ddi_VararrayAppend(varsTot,varsTot0);
	    Ddi_BddarrayAppend(substTot,substTot0);
	  }
	  Ddi_BddComposeAcc(a, varsTot, substTot);
	  //	  Ddi_BddComposeAcc(newConstr, varsTot, substTot);
	  if (retimedConstr != NULL)
	    Ddi_BddComposeAcc(retimedConstr, varsTot, substTot);
	  Ddi_AigarrayComposeAcc(delta, varsTot, substTot);

	  Ddi_Free(varsTot);
	  Ddi_Free(substTot);
	  Ddi_Free(varsTot0);
	  Ddi_Free(substTot0);
#endif

          Ddi_Free(myZero);
          Ddi_Free(myOne);
          Ddi_Free(implPi[0]);
          Ddi_Free(implPi[1]);
          if (0) {
            Ddi_Bdd_t *a0 = Ddi_BddCofactor(Ddi_BddPartRead(a, 9),
              Ddi_VarFromName(ddm, "i164"), 0);
            Ddi_Bdd_t *a1 = Ddi_BddCofactor(Ddi_BddPartRead(a, 9),
              Ddi_VarFromName(ddm, "i164"), 1);
            Ddi_Bdd_t *a01 = Ddi_BddAnd(a0, a1);
            Ddi_Bdd_t *o01 = Ddi_BddOr(a0, a1);

            Ddi_Free(a01);
            Ddi_Free(o01);
            Ddi_Free(a0);
            Ddi_Free(a1);
          }
          if (iP0 >= 0) {
            Ddi_Bdd_t *p0 = Ddi_BddarrayRead(delta, iP0);

            if (Ddi_BddSize(p0) == 1) {
              Ddi_Var_t *v = Ddi_BddTopVar(p0);

              for (i = 0; i < Ddi_BddPartNum(a); i++) {
                Ddi_Bdd_t *a_i = Ddi_BddPartRead(a, i);
                Ddi_Bdd_t *c0 = Ddi_BddMakeAig(a_i);
                Ddi_Bdd_t *c1 = Ddi_BddMakeAig(a_i);

                Ddi_BddCofactorAcc(c0, v, 0);
                Ddi_BddCofactorAcc(c1, v, 1);
                if (!Ddi_AigSatAnd(c0, c1, NULL)) {
                  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
                    Pdtutil_VerbLevelUsrMax_c,
                    fprintf(stdout, "New Composition.\n")
                    );
                }
                Ddi_Free(c0);
                Ddi_Free(c1);
              }
            }
          }



	  if (0 && !simplDone) {
	    //	    Ddi_Bdd_t *eq = DdiAigEquivVarsAcc(a,NULL,NULL,NULL,
	    //					       NULL,NULL,NULL);
	    Ddi_Bdd_t *y = a;
	    Ddi_Bddarray_t *subst = Ddi_BddarrayAlloc(ddm, 0);
	    Ddi_Vararray_t *vars = Ddi_VararrayAlloc(ddm, 0);
	    Ddi_Bdd_t *eq = DdiAigEquivVarsInConstrain(y,pi,
						       NULL,vars,subst);
	    if (eq!=NULL && !Ddi_BddIsOne(eq)) {
	      Ddi_BddComposeAcc(newConstr, vars, subst);
	      if (retimedConstr != NULL)
		Ddi_BddComposeAcc(retimedConstr, vars, subst);
	      Ddi_BddarrayComposeAcc(delta, vars, subst);
	      Ddi_BddAndAcc(newConstr,eq);
	      simplDone = 1;
	    }
	    else {
	      Ddi_Varset_t *psv = Ddi_VarsetMakeFromArray(ps);
	      Ddi_Free(eq);
	      eq = DdiAigEquivVarsAcc(y,NULL,NULL,psv,
				      NULL,vars,subst);
	      Ddi_Free(psv);
	      if (eq!=NULL && !Ddi_BddIsOne(eq)) {
		Ddi_BddComposeAcc(newConstr, vars, subst);
		if (retimedConstr != NULL)
		  Ddi_BddComposeAcc(retimedConstr, vars, subst);
		Ddi_BddarrayComposeAcc(delta, vars, subst);
		Ddi_BddAndAcc(newConstr,eq);
		simplDone = 1;
	      }
	    }
	    Ddi_Free(vars);
	    Ddi_Free(subst);
	    Ddi_Free(eq);
	  }

          Ddi_BddSetAig(a);
          if (!Ddi_BddIsOne(newConstr)) {
            Ddi_BddAndAcc(a, newConstr);
            Ddi_BddAndAcc(deltaConstr, newConstr);
          }
          {
            Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(cvarPs, 1);

            Ddi_BddAndAcc(a, lit);
            Ddi_Free(lit);
          }

          for (i = 0; i < np; i++) {
            int j;

            remPart[i] = 0;
            suppMap[i] = Pdtutil_Alloc(char,
              ni
            );

            for (j = 0; j < ni; j++) {
              suppMap[i][j] = 0;
            }
          }

	  int doStructRed = 0;
	  if (doStructRed) {
	    Ddi_AigStructRedRemAcc (a,NULL);
	    Ddi_AigarrayStructRedRemWithConstrAcc (delta,NULL,a);
	  }
          Ddi_BddarrayWrite(delta, iConstr, a);
	  if (fsmConstr!=NULL) {
	    /* NOT WORKING WITH INTEL benchmarks */
	    Ddi_BddCofactorAcc(a,cvarPs,1);
	    Ddi_DataCopy(fsmConstr,a);
	  }
          Ddi_Free(newConstr);

          Pdtutil_Free(remPart);
          Pdtutil_Free(piExist);
          Pdtutil_Free(piStats);
          Pdtutil_Free(piSort);
          Pdtutil_Free(psStats);
          Pdtutil_Free(psSort);
          for (i = 0; i < np; i++) {
            Pdtutil_Free(suppMap[i]);
          }
          Pdtutil_Free(suppMap);
          Ddi_Free(psv);
          //        Ddi_Free(retimedConstr);

          Ddi_Free(a);

          //        exit(1);
        } while (simplDone);
      }
      Ddi_Free(partConstr);
      //      Ddi_Free(deltaConstr);

#if 0
      if (0 && customConstraint) {
        Ddi_Bdd_t *myInvarspec = Ddi_BddarrayRead(lambda, 0);
        Ddi_Bdd_t *myPropDelta = Ddi_BddarrayRead(delta, iProp);
        Ddi_Var_t *constrV = Ddi_VararrayRead(ps, iP);
        Ddi_Bdd_t *constrLit = Ddi_BddMakeLiteralAig(constrV, 0);
        int l;

        //  extern Ddi_Var_t *constraintGuard;

        constraintGuard = constrV;

        for (l = 0; l < Ddi_BddarrayNum(delta); l++) {
          if (l != iP) {
            Ddi_Bdd_t *d_l = Ddi_BddarrayRead(delta, l);

            Ddi_BddCofactorAcc(d_l, constrV, 1);
          }
        }
        Ddi_BddOrAcc(myInvarspec, constrLit);
        Ddi_Free(constrLit);
      }
#endif
    }

    if (0) {
      int i, nRel = 0, nNoRel=0;
      Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(pi);

      for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
        Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);

        if (Ddi_BddSize(d_i) == 1) {
          Ddi_Var_t *v = Ddi_BddTopVar(d_i);

          if (Ddi_VarInVarset(piv, v)) {
            nRel++;
            if (1) {
              int j;
              int kk = 0, jj=0;
              Ddi_Var_t *ps_i = Ddi_VararrayRead(ps,i);
              if (strcmp(Ddi_VarName(v),"i1678")==0) {
                printf("!!!\n");
              }
              for (j = 0; j < Ddi_BddarrayNum(delta); j++) {
                if (j != i) {
                  Ddi_Bdd_t *d_j = Ddi_BddarrayRead(delta, j);
                  Ddi_Varset_t *s = Ddi_BddSupp(d_j);

                  if (Ddi_VarInVarset(s, v)) {
                    kk++;
                  }
                  if (Ddi_VarInVarset(s, ps_i)) {
                    jj++;
                  }
                  Ddi_Free(s);
                }
              }
              printf("found pi->ps: %s->%s | %d -> %d-%d\n",
                     Ddi_VarName(v), Ddi_VarName(ps_i), i, kk, jj);
            }
            //            printf("ns=pi left[%d]: %s\n", i, Ddi_VarName(v));
          }
        }
	else {
	  nNoRel++;
	}
      }
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
	Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, 
	    "pi compositions: %d - latch equiv: %d - impl: %d.\n",
	    piComp, nEquiv+nEquiv0, nImpl)
      );
      if (nRel > 0) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
          Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "%d/%d relational latches left.\n", nRel, nstate)
          );
      }
      Ddi_Free(piv);
      if (nRel > 10) {
        int aol = Ddi_MgrReadAigAbcOptLevel(ddm);
        
        Ddi_MgrSetOption(ddm, Pdt_DdiAigAbcOpt_c, inum, 3);
        Ddi_AigarrayAbcOptAcc (delta,-1);
        //      Ddi_AigarrayAbcOptAcc (newStub0,-1);
        Ddi_MgrSetOption(ddm, Pdt_DdiAigAbcOpt_c, inum, aol);
      }  

    }

    if (0) {
      Ddi_Bdd_t *aa = Ddi_AigPartitionTopWithXor(deltaConstr, 0, 1);

      printf("%d constr PARTITIONS left\n", Ddi_BddPartNum(aa));
      Ddi_Bdd_t *f = Ddi_BddPartRead(aa, 8);
      Ddi_Varset_t *sm = Ddi_VarsetVoid(ddm);
      Ddi_Var_t *i0 = Ddi_VarFromName(ddm, "i742");
      Ddi_Var_t *i1 = Ddi_VarFromName(ddm, "i748");
      Ddi_Var_t *i2 = Ddi_VarFromName(ddm, "i924");

      Ddi_VarsetAddAcc(sm, i0);
      Ddi_VarsetAddAcc(sm, i1);
      Ddi_BddExistAcc(f, sm);
      Ddi_Bdd_t *cof0 = Ddi_BddCofactor(f, i2, 0);
      Ddi_Bdd_t *cof1 = Ddi_BddCofactor(f, i2, 1);

      Ddi_BddNotAcc(cof0);
      Ddi_Free(aa);
    }
    Ddi_Free(deltaProp);
    Ddi_Free(deltaConstr);

    Ddi_Free(one);

  }

  if (piComp > 0) {
    Fsm_MgrSetRemovedPis(fsmMgr, 1);
  }

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
    Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "custom reduction DONE - delta size %d (num: %d).\n",
      Ddi_BddarraySize(delta), nstate)
    );

  // Fsm_RetimeWithConstr(fsmMgr);

  return (invarConstr);
}


/**Function********************************************************************

  Synopsis     [Given a Retiming Array Apply it to the FSM Manager.
    Return a new FSM Manager.]

  Description  [There should be a 1 to 1 correspondence between the
    retiming array (retimeStrPtr->retimeArray) and the delta array.]

  SideEffects  []

  SeeAlso      []

******************************************************************************/

Ddi_Bdd_t *
Fsm_ReduceCegar(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * invarspec,
  int maxBound,
  int enPba
)
{
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);

  /*----------------------- Take Fields From FSM Manager --------------------*/

  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *lambda = Fsm_MgrReadLambdaBDD(fsmMgr);

  int nstate = Ddi_VararrayNum(ps);
  int nVars = Ddi_MgrReadNumVars(ddm);
  int nRed = 0, prevSatBound = -1;
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
  Ddi_Bdd_t *invar = NULL;
  Ddi_Bdd_t *invarConstr = NULL;
  Ddi_Bdd_t *cone, *cex;
  int doRefine;
  Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(pi);
  Ddi_Varset_t *psv = Ddi_VarsetMakeFromArray(ps);
  Ddi_Varset_t *psIn, *psOut, *psRefNew;
  int nPsVars = Ddi_VararrayNum(ps);
  int cegarStrategy = Fsm_MgrReadCegarStrategy(fsmMgr);

  Pdtutil_Assert(nstate == Ddi_BddarrayNum(delta), "Wrong num of latches");
  Pdtutil_Assert(nstate == Ddi_VararrayNum(ps), "Wrong num of latches");
  Pdtutil_Assert(nstate == Ddi_VararrayNum(ns), "Wrong num of latches");

  /* initial abstraction */
  psIn = Ddi_BddSupp(invarspec);
  Ddi_VarsetIntersectAcc(psIn, psv);

  psRefNew = Ddi_VarsetVoid(ddm);

  doRefine = 1;
  while (doRefine) {
    Ddi_Bddarray_t *myDelta;
    Ddi_Bdd_t *coneAndInit = Ddi_BddMakeConstAig(ddm, 1);
    Ddi_Vararray_t *psOutA, *piOutSubst;
    Ddi_Bddarray_t *piOutSubstLits;
    int nRefNew = Ddi_VarsetNum(psRefNew);
    Ddi_Varset_t *psRefNew1;

    psOut = Ddi_VarsetDiff(psv, psIn);

    psOutA = Ddi_VararrayMakeFromVarset(psOut, 1);
    piOutSubst = Ddi_VararrayMakeNewVars(psOutA, "PDT_CEGAR_ABSTR", "", 0);
    piOutSubstLits = Ddi_BddarrayMakeLiteralsAig(piOutSubst, 1);
    myDelta = Ddi_BddarrayDup(delta);

    Ddi_AigarrayComposeNoMultipleAcc(myDelta, psOutA, piOutSubstLits);

    Ddi_Free(psOutA);
    Ddi_Free(piOutSubst);
    Ddi_Free(piOutSubstLits);

    /* add initial state or init stub */
    cone = Ddi_BddNot(invarspec);
    /* rename abstract state vars in delta */
    psRefNew1 = Ddi_VarsetDup(psRefNew);
    cex = bmcAbstrCheck(myDelta, cone, init, coneAndInit,
      initStub, ps, pi, maxBound, psRefNew1, enPba ? &prevSatBound : NULL);
    if (enPba && (Ddi_VarsetNum(psRefNew1) < nRefNew)) {
      /* reduced by PBA */
      Ddi_VarsetDiffAcc(psRefNew, psRefNew1);
      Ddi_VarsetUnionAcc(psOut, psRefNew);
      Ddi_VarsetDiffAcc(psIn, psRefNew);
      Ddi_Free(psRefNew);
      psRefNew = Ddi_VarsetVoid(ddm);
      Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "PBA refined vars: %d/%d.\n", Ddi_VarsetNum(psIn),
          nPsVars)
        );
    }
    Ddi_Free(psRefNew1);

    Ddi_Free(myDelta);
    if (cex == NULL) {
      doRefine = 0;
    } else {
      int nref;
      Ddi_Varset_t *refinedPs =
        refineWithCex(fsmMgr, coneAndInit, cex, psIn, psOut);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelUsrMax_c, fprintf(stdout,
          "Refining with %d state vars -> ", Ddi_VarsetNum(refinedPs))
        );
      Ddi_VarsetUnionAcc(psIn, refinedPs);
      Ddi_VarsetUnionAcc(psRefNew, refinedPs);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "%d/%d.\n", nref = Ddi_VarsetNum(psIn), nPsVars)
        );
      Ddi_Free(refinedPs);
      if (nref >= nPsVars * 0.98 && !enPba) {
        /* abandon if refinement is almost total */
        Ddi_DataCopy(psIn, psv);
        doRefine = 0;
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
          Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "No Further Refinement; All PS Variables Taken.\n")
          );
      }
    }

    Ddi_Free(psOut);
    Ddi_Free(coneAndInit);
    Ddi_Free(cone);
    Ddi_Free(cex);
  }

  if (0) {
    Ddi_Varset_t *abstrVars = Ddi_VarsetDiff(psv, psIn);
    FILE *fp = fopen("abstrVars.txt", "w");

    Ddi_VarsetPrint(abstrVars, 0, 0, fp);
    fclose(fp);
    Ddi_Free(abstrVars);
  }

  fsmCoiReduction(fsmMgr, psIn);

  Ddi_Free(psRefNew);
  Ddi_Free(psIn);
  Ddi_Free(piv);
  Ddi_Free(psv);
  return NULL;
}

/**Function********************************************************************

  Synopsis     [.]

  Description  [.]

  SideEffects  []

******************************************************************************/
int
Fsm_MgrAbcReduce(
  Fsm_Mgr_t * fsmMgr,
  float compactTh
)
{
  int ret = 0;
  Fsm_Fsm_t *fsmFsm = Fsm_FsmMakeFromFsmMgr(fsmMgr);
  Fsm_Fsm_t *fsmOpt;
  Ddi_Bdd_t *target;

  if (fsmMgr->stats.xInitLatches>0) {
    if (Fsm_MgrReadInitStubBDD(fsmMgr)==NULL) 
      return 0;
  }
  
  fsmOpt = Fsm_FsmAbcReduce(fsmFsm, compactTh);

  if (fsmOpt != NULL) {
    Fsm_FsmWriteToFsmMgr(fsmMgr, fsmOpt);
    Fsm_FsmFree(fsmOpt);
    ret = 1;
  }
  Fsm_FsmFree(fsmFsm);

  return ret;
}

/**Function********************************************************************

  Synopsis     [.]

  Description  [.]

  SideEffects  []

******************************************************************************/
int
Fsm_MgrAbcScorr(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * pArray,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * latchEq,
  int indK
)
{
  int ret = 0, nSpec = 0, i;
  Fsm_Fsm_t *fsmFsm = Fsm_FsmMakeFromFsmMgr(fsmMgr);
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);
  Fsm_Fsm_t *fsmOpt;
  Ddi_Bdd_t *target;
  Ddi_Bdd_t *invarspec = NULL;

  if (pArray != NULL) {
    invarspec = Fsm_MgrReadInvarspecBDD(fsmMgr);
    if (invarspec == NULL) {
      invarspec = Ddi_BddMakePartConjVoid(ddm);
    } else {
      invarspec = Ddi_BddDup(invarspec);
    }
    Ddi_BddSetPartConj(invarspec);
    nSpec = Ddi_BddPartNum(invarspec);
    for (i = 0; i < Ddi_BddarrayNum(pArray); i++) {
      Ddi_Bdd_t *p_i = Ddi_BddMakeAig(Ddi_BddarrayRead(pArray, i));

      Ddi_BddPartInsertLast(invarspec, p_i);
      Ddi_Free(p_i);
    }
  }

  fsmOpt = Fsm_FsmAbcScorr(fsmFsm, invarspec, care, latchEq, indK);

  if (pArray != NULL) {
    for (i = 0; i < Ddi_BddarrayNum(pArray); i++) {
      Ddi_BddarrayWrite(pArray, i, Ddi_BddPartRead(invarspec, nSpec + i));
    }
    for (i = Ddi_BddarrayNum(pArray) - 1; i >= 0; i--) {
      Ddi_BddPartRemove(invarspec, nSpec + i);
    }
  }

  if (fsmOpt != NULL) {
    Fsm_FsmWriteToFsmMgr(fsmMgr, fsmOpt);
    Fsm_FsmFree(fsmOpt);
    ret = 1;
  }
  Fsm_FsmFree(fsmFsm);

  return ret;
}

/**Function********************************************************************

  Synopsis     [.]

  Description  [.]

  SideEffects  []

******************************************************************************/
int
Fsm_MgrAbcReduceMulti(
  Fsm_Mgr_t * fsmMgr,
  float compactTh
)
{
  int ret = 0;
  Fsm_Fsm_t *fsmFsm = Fsm_FsmMakeFromFsmMgr(fsmMgr);
  Fsm_Fsm_t *fsmOpt;
  Ddi_Bdd_t *target;

  fsmOpt = Fsm_FsmAbcReduceMulti(fsmFsm, compactTh);

  if (fsmOpt != NULL) {
    Fsm_FsmWriteToFsmMgr(fsmMgr, fsmOpt);
    Fsm_FsmFree(fsmOpt);
    ret = 1;
  }
  Fsm_FsmFree(fsmFsm);

  return ret;
}

/**Function********************************************************************

  Synopsis     [Given a Retiming Array Apply it to the FSM Manager.
    Return a new FSM Manager.]

  Description  [There should be a 1 to 1 correspondence between the
    retiming array (retimeStrPtr->retimeArray) and the delta array.]

  SideEffects  []

  SeeAlso      []

******************************************************************************/

int
Fsm_RetimeMinreg(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * care,
  int strategy
)
{
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);

  /*----------------------- Take Fields From FSM Manager --------------------*/

  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *lambda = Fsm_MgrReadLambdaBDD(fsmMgr);

  int i, nstate = Ddi_VararrayNum(ps);
  int nVars = Ddi_MgrReadNumVars(ddm);
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);

  Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(pi);
  Ddi_Varset_t *psv = Ddi_VarsetMakeFromArray(ps);
  Ddi_Varset_t *psIn, *psOut;
  Ddi_Vararray_t *retimeCand = Ddi_VararrayDup(ps);
  Ddi_Vararray_t *lockedVars = Ddi_VararrayAlloc(ddm, 0);

  int nPsVars = Ddi_VararrayNum(ps);

  Ddi_Bddarray_t *substF = Ddi_BddarrayAlloc(ddm, 0);
  Ddi_Vararray_t *substV = Ddi_VararrayAlloc(ddm, 0);
  Ddi_Bddarray_t *substF0 = Ddi_BddarrayAlloc(ddm, 0);
  Ddi_Vararray_t *substV0 = Ddi_VararrayAlloc(ddm, 0);
  Ddi_Bddarray_t *newD;
  int doRetime = 0;
  int checkRetime = 1;
  int remCare = 0;
  int disablePiFlow = (strategy == 2);
  int newInvarPropHandling = 1;
  Ddi_Var_t *psProp = NULL, *psConstr = NULL;
  int iProp = -1, iConstr = -1;
  Ddi_Bdd_t *deltaConstr = NULL, *deltaProp = NULL;

  static int ncalls = 0;

  ncalls++;

  /*-------------------------- Take Options ------------------------*/

  Pdtutil_Assert(nstate == Ddi_BddarrayNum(delta), "Wrong num of latches");
  Pdtutil_Assert(nstate == Ddi_VararrayNum(ps), "Wrong num of latches");
  Pdtutil_Assert(nstate == Ddi_VararrayNum(ns), "Wrong num of latches");

  for (i = Ddi_VararrayNum(ps) - 1; i >= 0; i--) {
    Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);

    if (strcmp(Ddi_VarName(v_i), "PDT_BDD_INVARSPEC_VAR$PS") == 0) {
      iProp = i;
      deltaProp = Ddi_BddarrayRead(delta, i);
      psProp = Ddi_VararrayRead(ps, i);
    } else if (strcmp(Ddi_VarName(v_i), "PDT_BDD_INVAR_VAR$PS") == 0) {
      iConstr = i;
      deltaConstr = Ddi_BddarrayRead(delta, i);
      psConstr = Ddi_VararrayRead(ps, i);
    }
  }

  if (deltaConstr != NULL) {
    Ddi_Bdd_t *dNorm = Ddi_BddCofactor(deltaConstr, psConstr, 1);
    int isOne = Ddi_BddIsOne(dNorm);

    Ddi_Free(dNorm);
    if (!isOne)
      return 0;
  }

  if (newInvarPropHandling) {
    Pdtutil_Assert(iConstr >= 0
      && iProp >= 0, "prop or constr latch not found");
    Ddi_BddCofactorAcc(deltaProp, psConstr, 1);
    Ddi_BddCofactorAcc(deltaProp, psProp, 1);
    Ddi_BddCofactorAcc(deltaConstr, psConstr, 1);
  } else {
    for (i = Ddi_VararrayNum(ps) - 1; i >= 0; i--) {
      Ddi_Var_t *v_i = Ddi_VararrayRead(retimeCand, i);

      if (strncmp(Ddi_VarName(v_i), "PDT_BDD_", 8) == 0) {
        Ddi_VararrayInsertLast(lockedVars, v_i);
        Ddi_VararrayRemove(retimeCand, i);
      }
    }
  }

  newD = NULL;
  if (Ddi_VararrayNum(retimeCand) > 0) {
    if (care != NULL) {
      remCare = 1;
      Ddi_BddarrayInsertLast(delta, care);
    }
    newD = Ddi_BddarrayFindMinCut(delta, NULL, retimeCand, lockedVars,
      substF, substV, disablePiFlow, strategy == 1);
  }
  Ddi_Free(retimeCand);
  Ddi_Free(lockedVars);

  if (newD != NULL && Ddi_BddarrayNum(substF) > 0) {
    Ddi_Varset_t *va = Ddi_BddarraySupp(newD);
    Ddi_Varset_t *vb = Ddi_BddarraySupp(substF);
    int nsh;

    Ddi_VarsetIntersectAcc(va, vb);
    Ddi_Free(vb);
    doRetime = Ddi_VarsetNum(va) == 0;
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "Shared Variables between Cut Regions: %d.\n",
        Ddi_VarsetNum(va))
      );
    nsh = Ddi_VarsetNum(va);
    if (nsh > 0) {
      char name[100];
      Ddi_Vararray_t *vaA = Ddi_VararrayMakeFromVarset(va, 1);
      Ddi_Bddarray_t *subst = Ddi_BddarrayAlloc(ddm, nsh);

      for (i = 0; i < nsh; i++) {
        Ddi_Var_t *newv, *v_i = Ddi_VararrayRead(vaA, i);
        Ddi_Bdd_t *lit, *litPi;
        int isConstr = Ddi_VarName(v_i) != NULL &&
          (strcmp(Ddi_VarName(v_i), "PDT_BDD_INVAR_VAR$PS") == 0);
        int isProp = Ddi_VarName(v_i) != NULL &&
          (strcmp(Ddi_VarName(v_i), "PDT_BDD_INVARSPEC_VAR$PS") == 0);
        if (isProp || isConstr) {
          int j;

          Pdtutil_Assert(!newInvarPropHandling,
            "prop or constr var shared in retiming");
          lit = Ddi_BddMakeLiteralAig(v_i, 1);
          Ddi_BddarrayWrite(subst, i, lit);
          Ddi_BddarrayInsertLast(substF, lit);
          Ddi_VararrayInsertLast(substV, v_i);
          Ddi_Free(lit);
#if 0
          for (j = Ddi_VararrayNum(ps) - 1; j >= 0; j--) {
            if (Ddi_VararrayRead(ps, j) == v_i) {
              /* retime -1: subst state var with delta */
              Ddi_Bdd_t *delta_j = Ddi_BddarrayRead(delta, j);

              Ddi_BddarrayInsertLast(substF, delta_j);
              break;
            }
          }
#endif
        } else {
          sprintf(name, "Pdtrav_MinCut_Pi_Reg_%d_%s", ncalls,
            Ddi_VarName(v_i));
          newv = Ddi_VarFromName(ddm, name);
          if (newv == NULL) {
            newv = Ddi_VarNew(ddm);
            Ddi_VarAttachName(newv, name);
          }
          lit = Ddi_BddMakeLiteralAig(newv, 1);
          litPi = Ddi_BddMakeLiteralAig(v_i, 1);
          Ddi_VararrayInsertLast(substV, newv);
          Ddi_BddarrayInsertLast(substF, litPi);
          Ddi_BddarrayWrite(subst, i, lit);
          Ddi_Free(lit);
          Ddi_Free(litPi);
        }
      }
      Ddi_AigarrayComposeNoMultipleAcc(newD, vaA, subst);
      Ddi_Free(vaA);
      Ddi_Free(subst);
      doRetime = 1;
    }
    Ddi_Free(va);
    Ddi_Free(vb);
  }
#if 0
  if (substF == NULL && Ddi_BddarrayNum(substF) > 0 &&
    Ddi_BddarrayNum(substF0) > 0) {
    Ddi_AigarrayComposeNoMultipleAcc(substF, substV0, substF0);
  }
#endif

  if (newD != NULL && Ddi_BddarrayNum(substF) > 0 && checkRetime) {
    Ddi_Bddarray_t *d = Ddi_BddarrayDup(newD);

    Ddi_AigarrayComposeNoMultipleAcc(d, substV, substF);
    for (i = 0; i < Ddi_BddarrayNum(d); i++) {
      Ddi_Bdd_t *delta_i = Ddi_BddarrayRead(delta, i);
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(d, i);

      Pdtutil_Assert(Ddi_AigEqualSat(delta_i, d_i), "wrong retiming");
    }
    Ddi_Free(d);
  }

  if (remCare && care != NULL) {
    int l = Ddi_BddarrayNum(newD) - 1;
    Ddi_Bdd_t *newCare = Ddi_BddarrayExtract(newD, l);

    Ddi_BddarrayRemove(delta, l);
    Ddi_DataCopy(care, newCare);
    Ddi_Free(newCare);
  }

  if (doRetime) {
    Ddi_Varset_t *stubPi, *remPs = Ddi_BddarraySupp(substF);
    Ddi_Vararray_t *stubPiA;
    Ddi_Bddarray_t *newStub0, *auxReplace;
    Ddi_Bddarray_t *newStub = Ddi_BddarrayDup(substF);
    Ddi_Bddarray_t *deltaStub = Ddi_BddarrayDup(substF);
    int n0 = Ddi_BddarrayNum(delta);
    int nRem, nSubst, j, k;

    stubPi = Ddi_VarsetIntersect(remPs, piv);
    stubPiA = Ddi_VararrayMakeFromVarset(stubPi, 1);
    Ddi_Free(stubPi);

    Ddi_VarsetIntersectAcc(remPs, psv);

    nRem = Ddi_VarsetNum(remPs);
    nSubst = Ddi_VararrayNum(substV);

    if (nSubst >= nRem) {
      doRetime = 0;
    } else {

      Pdtutil_Assert(nSubst < nRem, "WRONG MIN_REG retiming");

      auxReplace = Ddi_BddarrayAlloc(ddm, nSubst);
      Ddi_AigarrayComposeNoMultipleAcc(deltaStub, ps, newD);

      if (Ddi_VararrayNum(stubPiA) > 0) {
        Ddi_Vararray_t *newPiVars;
        Ddi_Bddarray_t *newPiLits;
        char suffix[4];

        sprintf(suffix, "%d", ncalls);
        newPiVars = Ddi_VararrayMakeNewVars(stubPiA,
          "PDT_RETIME_STUB_PI", suffix, 1);
        newPiLits = Ddi_BddarrayMakeLiteralsAig(newPiVars, 1);
        Ddi_BddarrayComposeAcc(newStub, stubPiA, newPiLits);
        //      Ddi_VararrayAppend(pi,newPiVars);
        Ddi_Free(newPiVars);
        Ddi_Free(newPiLits);
      }

      if (initStub != NULL) {
        Ddi_AigarrayComposeNoMultipleAcc(newStub, ps, initStub);
        newStub0 = Ddi_BddarrayDup(initStub);
      } else {
        Ddi_Bdd_t *initPart = Ddi_AigPartitionTop(init, 0);

        newStub0 = Ddi_BddarrayMakeLiteralsAig(ps, 1);
        if (Ddi_BddIsCube(initPart)) {
          Ddi_AigarrayConstrainCubeAcc(newStub, init);
          Ddi_AigarrayConstrainCubeAcc(newStub0, init);
        } else {
          Pdtutil_Assert(0, "init is not a cube - not supported");
        }
        Ddi_Free(initPart);
      }

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "MINREG retiming from %d to", Ddi_VararrayNum(ps))
        );

      for (j = n0 - 1, k = nRem; j >= 0; j--) {
        Ddi_Var_t *v_j = Ddi_VararrayRead(ps, j);
        int isPropOrConstr = strncmp(Ddi_VarName(v_j), "PDT_BDD_", 8) == 0;

        if ((isPropOrConstr && newInvarPropHandling) ||
          Ddi_VarInVarset(remPs, v_j)) {
          /* retimed latch */
          if (isPropOrConstr) {
            /* keep latch and update retimed function */
            if (newInvarPropHandling) {
              Ddi_BddarrayWrite(delta, j, Ddi_BddarrayRead(newD, j));
            } else {
              k--;
            }
          } else if (k-- > nSubst) {
            /* removed latch */
            Ddi_VararrayRemove(ps, j);
            Ddi_VararrayRemove(ns, j);
            Ddi_BddarrayRemove(delta, j);
            Ddi_BddarrayRemove(newStub0, j);
          } else {
            /* replaced latch */
            Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(v_j, 1);

            Ddi_BddarrayWrite(auxReplace, k, lit);
            Ddi_BddarrayWrite(delta, j, Ddi_BddarrayRead(deltaStub, k));
            Ddi_BddarrayWrite(newStub0, j, Ddi_BddarrayRead(newStub, k));
            Ddi_Free(lit);
          }
        } else {
          Ddi_BddarrayWrite(delta, j, Ddi_BddarrayRead(newD, j));
        }
      }

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "to %d regs.\n", Ddi_VararrayNum(ps))
        );

      Ddi_AigarrayComposeNoMultipleAcc(delta, substV, auxReplace);
      if (remCare && care != NULL) {
        Ddi_BddComposeAcc(care, substV, auxReplace);
      }

      if (newInvarPropHandling) {
        /* restore prop & constr sequential circuitry */
        Ddi_Bdd_t *litConstr, *litProp;

        Pdtutil_Assert(iConstr >= 0
          && iProp >= 0, "prop or constr latch not found");

        for (i = Ddi_VararrayNum(ps) - 1; i >= 0; i--) {
          Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);

          if (strcmp(Ddi_VarName(v_i), "PDT_BDD_INVARSPEC_VAR$PS") == 0) {
            iProp = i;
            deltaProp = Ddi_BddarrayRead(delta, i);
            psProp = Ddi_VararrayRead(ps, i);
          } else if (strcmp(Ddi_VarName(v_i), "PDT_BDD_INVAR_VAR$PS") == 0) {
            iConstr = i;
            deltaConstr = Ddi_BddarrayRead(delta, i);
            psConstr = Ddi_VararrayRead(ps, i);
          }
        }

        Pdtutil_Assert(iConstr >= 0
          && iProp >= 0, "prop or constr latch not found");

        litConstr = Ddi_BddMakeLiteralAig(psConstr, 1);
        litProp = Ddi_BddMakeLiteralAig(psProp, 1);

        Ddi_BddAndAcc(deltaConstr, litConstr);

        Ddi_BddAndAcc(deltaProp, litProp);
        Ddi_BddNotAcc(litConstr);
        Ddi_BddOrAcc(deltaProp, litConstr);

        Ddi_Free(litConstr);
        Ddi_Free(litProp);
      }

      {
        int aol = Ddi_MgrReadAigAbcOptLevel(ddm);

        Ddi_MgrSetOption(ddm, Pdt_DdiAigAbcOpt_c, inum, 1);

        //      Ddi_AigarrayAbcOptAcc (delta,-1);
        //      Ddi_AigarrayAbcOptAcc (newStub0,-1);

        Ddi_MgrSetOption(ddm, Pdt_DdiAigAbcOpt_c, inum, aol);
      }

      Fsm_MgrSetInitStubBDD(fsmMgr, newStub0);

      Ddi_Free(newStub0);
      Ddi_Free(auxReplace);
    }

    Ddi_Free(newStub);
    Ddi_Free(deltaStub);
    Ddi_Free(stubPiA);
    Ddi_Free(remPs);

  }


  Ddi_Free(newD);
  Ddi_Free(substF);
  Ddi_Free(substV);
  Ddi_Free(substF0);
  Ddi_Free(substV0);


  Ddi_Free(piv);
  Ddi_Free(psv);

  return (doRetime);
}

/**Function********************************************************************

  Synopsis     [Given a Retiming Array Apply it to the FSM Manager.
    Return a new FSM Manager.]

  Description  [There should be a 1 to 1 correspondence between the
    retiming array (retimeStrPtr->retimeArray) and the delta array.]

  SideEffects  []

  SeeAlso      []

******************************************************************************/

void
Fsm_RetimeInitStub2Init(
  Fsm_Mgr_t * fsmMgr
)
{
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bdd_t *initState = Fsm_MgrReadInitBDD(fsmMgr);
  int i, useAig = Ddi_BddIsAig(Ddi_BddarrayRead(delta, 0));

  if (initStub != NULL) {

    Ddi_Bdd_t *newInit = Ddi_BddMakeConstAig(ddm, 1);
    Ddi_Varset_t *nsVars = Ddi_VarsetMakeFromArray(ns);

    for (i = 0; i < Ddi_BddarrayNum(initStub); i++) {
      Ddi_Bdd_t *nsLit_i = Ddi_BddMakeLiteralAig(Ddi_VararrayRead(ns, i), 1);
      Ddi_Bdd_t *tr_i = Ddi_BddXnor(nsLit_i, Ddi_BddarrayRead(initStub, i));

      Ddi_BddAndAcc(newInit, tr_i);
      Ddi_Free(tr_i);
      Ddi_Free(nsLit_i);
    }
    {
      int rrl = Ddi_MgrReadAigRedRemLevel(ddm);

      Ddi_MgrSetOption(ddm, Pdt_DdiAigRedRem_c, inum, -2);
      Ddi_BddExistProjectAcc(newInit, nsVars);
      Ddi_MgrSetOption(ddm, Pdt_DdiAigRedRem_c, inum, rrl);
    }

    Ddi_Free(nsVars);
    Ddi_BddSetMono(newInit);
    if (1 || Ddi_BddIsCube(newInit)) {
      Ddi_Bddarray_t *psLits = Ddi_BddarrayMakeLiterals(ps, 1);

      Ddi_BddComposeAcc(newInit, ns, psLits);
      Ddi_Free(psLits);
      if (useAig) {
        Ddi_BddSetAig(newInit);
      }
      Fsm_MgrSetInitBDD(fsmMgr, newInit);
      Fsm_MgrSetInitStubBDD(fsmMgr, NULL);
    }
    Ddi_Free(newInit);
  }

}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
void
Fsm_MgrCoiReduction_old(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * invarspec,
  int coiLimit,
  char *wrCoi
)
{
  Ddi_Varsetarray_t *coi;
  int nCoi;
  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);

  coi = computeFsmCoiVars(fsmMgr, invarspec, coiLimit);
  if (wrCoi != NULL) {
    writeCoiShells(wrCoi, coi, ps);
  }
  nCoi = Ddi_VarsetarrayNum(coi);
  fsmCoiReduction(fsmMgr, Ddi_VarsetarrayRead(coi, nCoi - 1));

  Ddi_Free(coi);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
int
Fsm_FsmStructReduction(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Var_t * clkVar
)
{
  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(lambda);
  bAig_Manager_t *bmgr = ddiMgr->aig.mgr;
  Ddi_Varset_t *dsupp, *stubsupp;
  int i, n, nconst = 0, neq = 0;
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Bdd_t *invarspec = Fsm_MgrReadInvarspecBDD(fsmMgr);
  Ddi_Bdd_t *invar = Fsm_MgrReadConstraintBDD(fsmMgr);
  Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
  Ddi_Bddarray_t *substF;
  Ddi_Vararray_t *substV;
  int addStub, enAddStub = invar==NULL || Ddi_BddIsOne(invar);
  int *enEq;
  int stubAigVars = 0;

  n = Ddi_BddarrayNum(delta);

  Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Original Delta: Size=%d; #Partitions=%d.\n",
      Ddi_BddarraySize(delta), n));

  do {
    addStub = 0;
    n = Ddi_BddarrayNum(delta);
    for (i = n - 1; i >= 0; i--) {
      Ddi_Var_t *pv_i = Ddi_VararrayRead(ps, i);
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);
      int isOne = Ddi_BddIsOne(d_i);
      int isZero = !isOne && Ddi_BddIsZero(d_i);
      int isConst = isOne || isZero;

      if (isConst && pv_i != clkVar &&
        strncmp(Ddi_VarName(pv_i), "PDT_BDD_", 8) != 0) {
        int removeVar = 0;
        Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(pv_i, isOne);

        /* take opposite phase */
        Ddi_Bdd_t *litBar = Ddi_BddNot(lit);

        /* check if initial state out of const value exist */
        if ((initStub == NULL) && !Ddi_AigSatAnd(init, litBar, NULL)) {
          removeVar = 1;
        } else if (initStub != NULL) {
          Ddi_Bdd_t *stub_i = Ddi_BddDup(Ddi_BddarrayRead(initStub, i));

          /* take opposite phase */
          if (isOne)
            Ddi_BddNotAcc(stub_i);
          if (!Ddi_AigSat(stub_i)) {
            /* init stub confirms constant value: reduce */
            removeVar = 1;
            Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMax_c) {
              fprintf(stdout, "Remove var %s\n", Ddi_VarName(pv_i));
            }
            Ddi_BddarrayRemove(initStub, i);
          } else if (enAddStub) {
            /* init stub does not confirm value - but next state yes.
               add one more time frame to stub */
            addStub = 1;
          }
          Ddi_Free(stub_i);
        }
        if (removeVar) {
          /* take init cofactor */
          if (initStub == NULL) {
            Ddi_BddCofactorAcc(init, pv_i, isOne);
          }
          Ddi_BddarrayRemove(delta, i);
          Ddi_VararrayRemove(ps, i);
          Ddi_VararrayRemove(ns, i);
          Ddi_AigarrayConstrainCubeAcc(delta, lit);
          Ddi_AigarrayConstrainCubeAcc(lambda, lit);
          if (invarspec != NULL) {
            Ddi_AigConstrainCubeAcc(invarspec, lit);
          }
          if (invar != NULL) {
            Ddi_AigConstrainCubeAcc(invar, lit);
          }
          nconst++;
        }
        Ddi_Free(lit);
        Ddi_Free(litBar);
      }
    }
    if (addStub) {
      char suffix[4];
      Ddi_Vararray_t *newPiVars;
      Ddi_Bddarray_t *newPiLits;
      Ddi_Bddarray_t *myInitStub = Ddi_BddarrayDup(delta);

      sprintf(suffix, "%d", fsmMgr->stats.initStubSteps++);
      if (stubAigVars)
        newPiVars = Ddi_VararrayMakeNewAigVars(pi, "PDT_STUB_PI", suffix);
      else
        newPiVars = Ddi_VararrayMakeNewVars(pi, "PDT_STUB_PI", suffix, 1);
      newPiLits = Ddi_BddarrayMakeLiteralsAig(newPiVars, 1);
      Ddi_BddarrayComposeAcc(myInitStub, pi, newPiLits);
      Pdtutil_Assert(Fsm_MgrReadInitStubBDD(fsmMgr) != NULL,
        "missing init stub");
      Ddi_BddarrayComposeAcc(myInitStub, ps, Fsm_MgrReadInitStubBDD(fsmMgr));
      Ddi_Free(newPiVars);
      Ddi_Free(newPiLits);
      Fsm_MgrSetInitStubBDD(fsmMgr, myInitStub);
      Ddi_Free(myInitStub);
      initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
      //Fsm_MgrIncrInitStubSteps (fsmMgr,1);
      Pdtutil_WresIncrInitStubSteps(1);
    }
  } while (addStub);


  if (nconst > 0) {
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "Found %d Constants; ", nconst);
      fprintf(stdout, "Reduced Delta: Size=%d; #Partitions=%d.\n",
        Ddi_BddarraySize(delta), n));
  }

  do {

    n = Ddi_BddarrayNum(delta);
    neq = 0;
    enEq = Pdtutil_Alloc(int,
      n
    );

    for (i = 0; i < n; i++) {
      Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);

      enEq[i] = strncmp(Ddi_VarName(v_i), "PDT_BDD_", 8) != 0;
    }

    substV = Ddi_VararrayAlloc(ddiMgr, 0);
    substF = Ddi_BddarrayAlloc(ddiMgr, 0);

    for (i = 0; i < n; i++) {
      if (enEq[i] == 1) {
        Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);
        Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);
        bAigEdge_t baig = Ddi_BddToBaig(d_i);
        int ref_i = bAig_AuxInt(bmgr, baig);

        if (ref_i == -1) {
          /* no equiv */
          ref_i = bAig_AuxInt(bmgr, baig) = 2 * i;
          if (Ddi_BddIsComplement(d_i))
            ref_i++;
          bAig_AuxInt(bmgr, baig) = ref_i;
        } else {
          int j = ref_i / 2;
          int equal;
          int complEq = ref_i % 2 ^ Ddi_BddIsComplement(d_i);
          Ddi_Bdd_t *d_j = Ddi_BddarrayRead(delta, j);
          Ddi_Var_t *v_j = Ddi_VararrayRead(ps, j);

          if (enEq[j]) {
            /* equal deltas found ! Check init state */
            if (initStub != NULL) {
              Ddi_Bdd_t *s_j = Ddi_BddDup(Ddi_BddarrayRead(initStub, j));

              if (complEq)
                Ddi_BddNotAcc(s_j);
              equal = Ddi_BddEqual(Ddi_BddarrayRead(initStub, i), s_j);
              Ddi_Free(s_j);
            } else {
              Ddi_Bdd_t *eq = Ddi_BddMakeLiteralAig(v_i, 1);
              Ddi_Bdd_t *l_j = Ddi_BddMakeLiteralAig(v_j, !complEq);

              Ddi_BddXnorAcc(eq, l_j);
              Ddi_Free(l_j);
              equal = Ddi_BddIncluded(init, eq);
              Ddi_Free(eq);
            }
            if (equal) {
              Ddi_Bdd_t *l_j = Ddi_BddMakeLiteralAig(v_j, !complEq);

              Ddi_BddarrayInsertLast(substF, l_j);
              Ddi_VararrayInsertLast(substV, v_i);
              Ddi_Free(l_j);
              enEq[i] = -1;
              neq++;

              Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
                fprintf(stdout,
                  "  SEQ equiv found(%d <- %d): %s <- %s.\n", j, i,
                  Ddi_VarName(v_j), Ddi_VarName(v_i)));
            }
          }
        }
      }
    }

    for (i = 0; i < n; i++) {
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);
      bAigEdge_t baig = Ddi_BddToBaig(d_i);

      bAig_AuxInt(bmgr, baig) = -1;
    }

    if (neq > 0) {
      for (i = n - 1; i >= 0; i--) {
        if (enEq[i] == -1) {
          if (initStub != NULL) {
            Ddi_BddarrayRemove(initStub, i);
          } else {
            Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);
            Ddi_Bdd_t *l_i = Ddi_BddMakeLiteralAig(v_i, 1);
            int pos = Ddi_BddIncluded(init, l_i);

            Ddi_BddCofactorAcc(init, v_i, pos);
            Ddi_Free(l_i);
          }
          Ddi_VararrayRemove(ps, i);
          Ddi_VararrayRemove(ns, i);
          Ddi_BddarrayRemove(delta, i);

        }
      }
      Ddi_BddarrayComposeAcc(delta, substV, substF);
      Ddi_AigarrayComposeAcc(lambda, substV, substF);
      if (invarspec != NULL) {
        Ddi_BddComposeAcc(invarspec, substV, substF);
      }
      if (invar != NULL) {
        Ddi_BddComposeAcc(invar, substV, substF);
      }
    }

    if (neq > 0) {
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
        fprintf(stdout,
          "Found %d seq EQ - Reduced DELTA - size: %d - components: %d\n",
          neq, Ddi_BddarraySize(delta), n));
    }

    Ddi_Free(substV);
    Ddi_Free(substF);

    Pdtutil_Free(enEq);
  }
  while (neq > 0);

  return (neq + nconst);
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
void
FsmCreateInitStub(
  Fsm_Mgr_t * fsmMgr
)
{
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);
  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Bdd_t *invarspec = Fsm_MgrReadInvarspecBDD(fsmMgr);
  Ddi_Bdd_t *invar = Fsm_MgrReadConstraintBDD(fsmMgr);
  Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
  int nstate = Ddi_VararrayNum(ps);
  int genAigVars = Ddi_VarIsAig(Ddi_VararrayRead(ps,0));


  if (initStub != NULL) return;
  
  initStub = Ddi_BddarrayDup(delta);
  char suffix[5];
  Ddi_Vararray_t *newPiVars;

  if (invar!=NULL && !Ddi_BddIsOne(invar)) {
    int iConstr = Ddi_BddarrayNum(delta)-2;
    int iProp = Ddi_BddarrayNum(delta)-1;
    Ddi_Var_t *v1 = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
    if (Ddi_VararrayRead(ps,iConstr)==v1) {
      Ddi_Bdd_t *dConstr = Ddi_BddarrayRead(initStub,iConstr);
      Ddi_Bdd_t *dProp = Ddi_BddarrayRead(initStub,iProp);
      Ddi_Bdd_t *outOfConstr = Ddi_BddNot(dConstr);
      //	    Ddi_BddAndAcc(dConstr,invar);
      Ddi_BddOrAcc(dProp,outOfConstr);
      Ddi_Free(outOfConstr);
    }
  }
  Pdtutil_Assert(fsmMgr->stats.initStubSteps==0,"invalid init stub steps");
  sprintf(suffix, "%d", fsmMgr->stats.initStubSteps++);
  newPiVars = genAigVars ?
    Ddi_VararrayMakeNewAigVars(pi, "PDT_STUB_PI", suffix)
    : Ddi_VararrayMakeNewVars(pi, "PDT_STUB_PI", suffix, 1);
  Ddi_BddarraySubstVarsAcc(initStub, pi, newPiVars);
  Ddi_AigarrayConstrainCubeAcc(initStub, init);
  Ddi_Vararray_t *freePs = Ddi_BddarraySuppVararray(initStub);
  Ddi_VararrayIntersectAcc(freePs,ps);
  if (Ddi_VararrayNum(freePs)>0) {
    /* add stub init state vars */
    Ddi_Vararray_t *newPsVars;
    newPsVars = genAigVars ?
      Ddi_VararrayMakeNewAigVars(freePs,"PDT_STUB_PS","") :
      Ddi_VararrayMakeNewVars(freePs,"PDT_STUB_PS", "", 1);
    Ddi_BddarraySubstVarsAcc(initStub, freePs, newPsVars);
    Ddi_Free(newPsVars);
  }
  Ddi_Free(freePs);
 
  Ddi_Free(newPiVars);
  Fsm_MgrSetInitStubBDD(fsmMgr, initStub);
  Ddi_Free(initStub);

}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
void
FsmMgrAddNewLatches(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Vararray_t *newPs,
  Ddi_Vararray_t *newNs,
  Ddi_Bddarray_t *newDelta,
  Ddi_Bddarray_t *initStubDelta
)
{
  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Bdd_t *invarspec = Fsm_MgrReadInvarspecBDD(fsmMgr);
  Ddi_Bdd_t *invar = Fsm_MgrReadConstraintBDD(fsmMgr);
  Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
  int nstate = Ddi_VararrayNum(ps);
  int nNew = Ddi_VararrayNum(newPs);
  
  Pdtutil_Assert(initStub!=NULL, "fsm without init stub not supported");

  Ddi_Vararray_t *stubSupp = Ddi_BddarraySuppVararray(initStub);
  int nsv = Ddi_VararrayNum(stubSupp);
    
  int genAigVars = Ddi_VarIsAig(Ddi_VararrayRead(ps,0));
  Ddi_Bddarray_t *addedInitStub = NULL;
  if (initStubDelta!=NULL) {
    char suffix[5];
    Ddi_Vararray_t *newPiVars=NULL;
    Ddi_Vararray_t *stubDeltaSupp = Ddi_BddarraySuppVararray(initStubDelta);
    Ddi_Vararray_t *stubPi = Ddi_VararrayIntersect(stubDeltaSupp,pi);
    addedInitStub = Ddi_BddarrayDup(initStubDelta);
    if (Ddi_VararrayNum(stubPi)>0) {
      sprintf(suffix, "%d", fsmMgr->stats.initStubSteps-1);
      newPiVars = genAigVars ?
        Ddi_VararrayMakeNewAigVars(stubPi, "PDT_STUB_CUT_PI", suffix)
        : Ddi_VararrayMakeNewVars(stubPi, "PDT_STUB_CUT_PI", suffix, 1);
      Ddi_BddarraySubstVarsAcc(addedInitStub, stubPi, newPiVars);
    }
    Ddi_Free(stubPi);
    Ddi_Free(newPiVars);
    Ddi_Free(stubDeltaSupp);
    Ddi_BddarrayComposeAcc(addedInitStub,ps,initStub);
  }
  int ii, i;
  for (ii=nstate-2,i=0; i<nNew; ii++, i++) {
    Ddi_VararrayInsert(ps,ii,Ddi_VararrayRead(newPs,i));
    Ddi_VararrayInsert(ns,ii,Ddi_VararrayRead(newNs,i));
    Ddi_BddarrayInsert(delta,ii,Ddi_BddarrayRead(newDelta,i));
    Ddi_BddarrayInsert(initStub,ii,Ddi_BddarrayRead(addedInitStub,i));
  }
  Ddi_Free(stubSupp);
  Ddi_Free(addedInitStub);

}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis     [Given a Retiming Array Apply it to the FSM Manager.
    Return a new FSM Manager.]

  Description  [There should be a 1 to 1 correspondence between the
    retiming array (retimeStrPtr->retimeArray) and the delta array.]

  SideEffects  []

  SeeAlso      []

******************************************************************************/

static int
retimePeriferalLatches(
  Fsm_Mgr_t * fsmMgr,
  int force
)
{
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);

  /*----------------------- Take Fields From FSM Manager --------------------*/

  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  Ddi_Bdd_t *initState = Fsm_MgrReadInitBDD(fsmMgr);
  Ddi_Bdd_t *invarspec = Fsm_MgrReadInvarspecBDD(fsmMgr);
  Ddi_Bdd_t *constraint = Fsm_MgrReadConstraintBDD(fsmMgr);
  Ddi_Vararray_t *substVars = Ddi_VararrayAlloc(ddm, 0);
  Ddi_Bddarray_t *substFuncs = Ddi_BddarrayAlloc(ddm, 0);
  Ddi_Varset_t *piVars = Ddi_VarsetMakeFromArray(pi);
  int nstate = Ddi_VararrayNum(ps);
  int nVars = Ddi_MgrReadNumVars(ddm);
  int nRed, nExclusiveInput;
  Ddi_Bddarray_t *initStub = NULL;

  int i, n, useAig = 0, initSize, addInitStub = 0;
  int again, okInit = 1;

  Ddi_Bdd_t *one;
  Ddi_Vararray_t **latchSupps = Pdtutil_Alloc(Ddi_Vararray_t *, nstate);
  int *doRetime = Pdtutil_Alloc(int, nstate
  );
  int *suppCnt = Pdtutil_Alloc(int, nVars
  );
  int nOneSupp = 0, *oneSuppCnt = Pdtutil_Alloc(int, nVars
  );
  int makeAigVars = (nstate > 5500);
  Ddi_Vararray_t *ps0 = Ddi_VararrayDup(ps);
  
  for (i = 0; i < nVars; i++) {
    suppCnt[i] = 0;
  }

  initSize = Ddi_BddarraySize(delta);

  /*-------------------------- Take Retiming Options ------------------------*/

  Pdtutil_Assert(nstate == Ddi_BddarrayNum(delta), "Wrong num of latches");
  Pdtutil_Assert(nstate == Ddi_VararrayNum(ps), "Wrong num of latches");
  Pdtutil_Assert(nstate == Ddi_VararrayNum(ns), "Wrong num of latches");

  useAig = Ddi_BddIsAig(Ddi_BddarrayRead(delta, 0));

  initStub = Ddi_BddarrayDup(delta);
  n = Ddi_BddarrayNum(initStub);
  if (!Ddi_BddIsAig(Ddi_BddarrayRead(initStub, n - 1))) {
    Ddi_BddSetAig(Ddi_BddarrayRead(initStub, n - 1));
  }
  if (Fsm_MgrReadInitStubBDD(fsmMgr) != NULL) {
    Ddi_BddarrayComposeAcc(initStub, ps, Fsm_MgrReadInitStubBDD(fsmMgr));
  } else if (useAig) {
    Ddi_Bdd_t *initPart = Ddi_AigPartitionTop(initState, 0);

    if (Ddi_BddIsCube(initPart)) {
      Ddi_AigarrayConstrainCubeAcc(initStub, initState);
    } else {
      Ddi_Free(initStub);
    }
    Ddi_Free(initPart);
  } else {
    for (i = 0; i < Ddi_BddarrayNum(initStub); i++) {
      Ddi_BddConstrainAcc(Ddi_BddarrayRead(initStub, i), initState);
    }
  }

  for (i = nstate - 1; i >= 0; i--) {
    Ddi_Varset_t *supp_i = Ddi_BddSupp(Ddi_BddarrayRead(delta, i)); /* @ */
    Ddi_Vararray_t *vA = Ddi_VararrayMakeFromVarset(supp_i, 1);
    int j;

    for (j = 0; j < Ddi_VararrayNum(vA); j++) {
      Ddi_Var_t *v = Ddi_VararrayRead(vA, j);
      int varId = Ddi_VarIndex(v);

      Pdtutil_Assert(varId < nVars, "Wrong var id");
      suppCnt[varId]++;
    }
    Ddi_Free(vA);
    Ddi_Free(supp_i);
  }
  for (i = 0; i < nVars; i++) {
    oneSuppCnt[i] = suppCnt[i] == 1;
    if (0 && (suppCnt[i] > 0)) {
      printf("supp[%s] = %d\n",
        Ddi_VarName(Ddi_VarFromIndex(ddm, i)), suppCnt[i]);
    }
  }

  Ddi_VarsetSetArray(piVars);

  Ddi_VararrayWriteMark (pi, 1);

  for (i = nstate - 1; i >= 0; i--) {
    Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);
    Ddi_Vararray_t *supp_i = Ddi_BddSuppVararray(d_i);  
    int j, allPis=1;

    latchSupps[i] = NULL;

    for (j=0; j<Ddi_VararrayNum(supp_i);j++) {
      if (Ddi_VarReadMark(Ddi_VararrayRead(supp_i,j))!=1) {
	allPis = 0;
	break;
      }
    }

    doRetime[i] = 0;

    if (allPis) {
      int j;
      Ddi_Vararray_t *vA = latchSupps[i] =
        Ddi_VararrayDup(supp_i);
      doRetime[i] = 2;
      for (j = 0; j < Ddi_VararrayNum(vA); j++) {
        Ddi_Var_t *v = Ddi_VararrayRead(vA, j);
        int varId = Ddi_VarIndex(v);
        Pdtutil_Assert(varId < nVars, "Wrong var id");
        Pdtutil_Assert(suppCnt[varId] > 0, "Wrong supp cnt");
        suppCnt[varId]--;
      }
    }
    Ddi_Free(supp_i);
  }

  Ddi_VararrayWriteMark (pi, 0);

  do {
    again = 0;
    for (i = nstate - 1; i >= 0; i--) {
      if (doRetime[i]) {
        int j, exclusiveSupp = 1, oneSupp = 1;
        Ddi_Vararray_t *vA = latchSupps[i];

        for (j = 0; j < Ddi_VararrayNum(vA); j++) {
          int varId = Ddi_VarIndex(Ddi_VararrayRead(vA, j));

          if (suppCnt[varId] > 0)
            exclusiveSupp = 0;
          if (!oneSuppCnt[varId])
            oneSupp = 0;
        }
        if (!exclusiveSupp) {
          doRetime[i] = 0;
          again = 1;
          for (j = 0; j < Ddi_VararrayNum(vA); j++) {
            int varId = Ddi_VarIndex(Ddi_VararrayRead(vA, j));

            suppCnt[varId]++;
          }
        } else {
          if (oneSupp)
            doRetime[i] = 1;
        }
      }
    }
  }
  while (again);

  for (i = 0; i < nstate; i++) {
    Ddi_Free(latchSupps[i]);
  }
  Pdtutil_Free(latchSupps);

  for (i = nstate - 1; i >= 0; i--) {
    Ddi_Bddarray_t *fsmInitStub = Fsm_MgrReadInitStubBDD(fsmMgr);
    Ddi_Bdd_t *d_i = Ddi_BddDup(Ddi_BddarrayRead(delta, i));
    Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);
    Ddi_Vararray_t *supp_i = Ddi_BddSuppVararray(d_i);  
    int j, nSupp=Ddi_VararrayNum(supp_i);

    if (nSupp == 0) {
      int isOne;
      Ddi_Bdd_t *initPhase;

      Pdtutil_Assert(Ddi_BddIsConstant(d_i), "CONSTANT delta required");
      isOne = Ddi_BddIsOne(d_i);
      if (useAig) {
        initPhase = Ddi_BddMakeLiteralAig(v_i, isOne);
      } else {
        initPhase = Ddi_BddMakeLiteral(v_i, isOne);
      }

      if (fsmInitStub == NULL) {
        okInit = okInit && Ddi_BddIncluded(initState, initPhase);
      } else {
        if (isOne && !Ddi_BddIsOne(Ddi_BddarrayRead(fsmInitStub, i))) {
          okInit = 0;
        }
        if (!isOne && !Ddi_BddIsZero(Ddi_BddarrayRead(fsmInitStub, i))) {
          okInit = 0;
        }
        okInit = 0;
      }

      if (!okInit) {
        addInitStub = 1;
        Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelDevMin_c) {
          fprintf(stdout,
            "Warning: %s has Constant Input with Different Init Value.\n",
            Ddi_VarName(v_i));
        }
      }
      Ddi_Free(initPhase);
    }

    if (doRetime[i]) {
      /* exclusive support */
      //      printf("perif latch [%d] %s\n", i, Ddi_VarName(Ddi_VararrayRead(ps,i)));
      Ddi_BddarrayInsertLast(substFuncs, d_i);
      Ddi_VararrayInsertLast(substVars, Ddi_VararrayRead(ps, i));
      Ddi_BddarrayRemove(delta, i);
      Ddi_VararrayRemove(ps, i);
      Ddi_VararrayRemove(ns, i);
      addInitStub = 1;
      if (initStub != NULL) {
        Ddi_BddarrayRemove(initStub, i);
      }
      if (fsmInitStub != NULL) {
        Ddi_BddarrayRemove(fsmInitStub, i);
      }
      nOneSupp += doRetime[i] == 1;
    }
    Ddi_Free(d_i);
    Ddi_Free(supp_i);

  }

  if ( /* nRed>0 && */ addInitStub) {
    char suffix[4];
    Ddi_Vararray_t *newPiVars;
    Ddi_Bddarray_t *newPiLits;

    sprintf(suffix, "%d", fsmMgr->stats.initStubSteps);
    newPiVars = makeAigVars ?
      Ddi_VararrayMakeNewAigVars(pi, "PDT_STUB_PI", suffix) :
      Ddi_VararrayMakeNewVars(pi, "PDT_STUB_PI", suffix, 1);
    newPiLits = Ddi_BddarrayMakeLiteralsAig(newPiVars, 1);
    Ddi_BddarrayComposeAcc(initStub, pi, newPiLits);
    Ddi_Free(newPiVars);
    Ddi_Free(newPiLits);
    if (fsmMgr->stats.initStubSteps==0) {
      Ddi_Vararray_t *freePs = Ddi_BddarraySuppVararray(initStub);
      Ddi_VararrayIntersectAcc(freePs,ps0);
      if (Ddi_VararrayNum(freePs)>0) {
        /* add stub init state vars */
        Ddi_Vararray_t *newPsVars;
        Ddi_Bddarray_t *newPsLits;
        newPsVars = makeAigVars ?
          Ddi_VararrayMakeNewAigVars(freePs,"PDT_STUB_PS","") :
          Ddi_VararrayMakeNewVars(freePs,"PDT_STUB_PS", "", 1);
        newPsLits = Ddi_BddarrayMakeLiteralsAig(newPsVars, 1);
        Ddi_BddarrayComposeAcc(initStub, freePs, newPsLits);
        Ddi_Free(newPsVars);
        Ddi_Free(newPsLits);
      }
      Ddi_Free(freePs);
    }
    Fsm_MgrSetInitStubBDD(fsmMgr, initStub);
    Pdtutil_WresIncrInitStubSteps(1);
    fsmMgr->stats.initStubSteps++;
  }

  nRed = Ddi_VararrayNum(substVars);
  Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelDevMin_c) {
    fprintf(stdout,
      "Found %d Peripheral Latches (%d 1 s.) - %d Latches Remaining.\n", nRed,
      nOneSupp, Ddi_VararrayNum(ps) - 1);
  }

  if (Ddi_VararrayNum(substVars) > 0) {
    Fsm_MgrRetimePis(fsmMgr, substFuncs);
    if (useAig) {
      Ddi_Bdd_t *dProp = NULL;

      n = Ddi_BddarrayNum(delta);
      if (!Ddi_BddIsAig(Ddi_BddarrayRead(delta, n - 1))) {
        dProp = Ddi_BddarrayExtract(delta, n - 1);
        Ddi_BddComposeAcc(dProp, substVars, substFuncs);
      }
      Ddi_AigarrayComposeAcc(delta, substVars, substFuncs);
      if (dProp != NULL) {
        Ddi_BddarrayInsert(delta, n - 1, dProp);
        Ddi_Free(dProp);
      }
      Ddi_AigarrayComposeAcc(lambda, substVars, substFuncs);
    } else {
      for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
        Ddi_BddComposeAcc(Ddi_BddarrayRead(delta, i), substVars, substFuncs);
      }
      for (i = 0; i < Ddi_BddarrayNum(lambda); i++) {
        Ddi_BddComposeAcc(Ddi_BddarrayRead(lambda, i), substVars, substFuncs);
      }
    }
    if (invarspec != NULL) {
      Ddi_BddComposeAcc(invarspec, substVars, substFuncs);
    }
    if (constraint != NULL) {
      Ddi_BddComposeAcc(constraint, substVars, substFuncs);
    }
  }

  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelDevMin_c) {
    fprintf(stdout, "  Peripheral Latch Reduction; Delta Size: %d -> %d.\n",
      initSize, Ddi_BddarraySize(delta));
  }

  int checkStall = 0;
  if (checkStall) {
    int nStalled = 0;
    for (int i=0; i<Ddi_BddarrayNum(delta); i++) {
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta,i);
      Ddi_Var_t *ps_i = Ddi_VararrayRead(ps,i);
      if (Ddi_BddSize(d_i) == 1) {
	if (Ddi_BddTopVar(d_i)==ps_i) {
	  nStalled++;
	}
      }
    }
    printf("%d latched stalled adter init found\n", nStalled);
  }


  if (0 && initStub!=NULL && fsmMgr->stats.initStubSteps==1) {
    int ns = Ddi_BddarrayNum(initStub);
    for (int i=0; i<ns; i++) {
      Ddi_Bdd_t *is_i = Ddi_BddarrayRead(initStub,i);
      if (Ddi_BddSize(is_i) > 0) {
        printf("init stub[%d] -> size: %d\n", i, Ddi_BddSize(is_i));
      }
    }
  }  
    
  Ddi_Free(ps0);
  Ddi_Free(initStub);
  Ddi_Free(substVars);
  Ddi_Free(substFuncs);
  Ddi_Free(piVars);
  Pdtutil_Free(oneSuppCnt);
  Pdtutil_Free(suppCnt);
  Pdtutil_Free(doRetime);

  return (nRed);
}

/**Function********************************************************************

  Synopsis     [Insert redundant latches at cut points.
    Return a new FSM Manager.]

  Description  [There should be a 1 to 1 correspondence between the
    retiming array (retimeStrPtr->retimeArray) and the delta array.]

  SideEffects  []

  SeeAlso      []

******************************************************************************/

static int
addCutLatches(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t *cuts0,
  int retimePis
)
{
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);

  /*----------------------- Take Fields From FSM Manager --------------------*/

  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  Ddi_Bdd_t *initState = Fsm_MgrReadInitBDD(fsmMgr);
  Ddi_Bdd_t *invarspec = Fsm_MgrReadInvarspecBDD(fsmMgr);
  Ddi_Bdd_t *constraint = Fsm_MgrReadConstraintBDD(fsmMgr);
  int genAigVars = Ddi_VarIsAig(Ddi_VararrayRead(ps,0));

  // filter out cuts on variables or latch input
  Ddi_Bddarray_t *cuts = Ddi_BddarrayAlloc(ddm, 0);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  for (int i=0; i<Ddi_BddarrayNum(cuts0); i++) {
    Ddi_Bdd_t *c_i = Ddi_BddarrayRead(cuts0,i);
    bAigEdge_t baig_i = Ddi_BddToBaig(c_i);
    bAig_AuxInt(bmgr, baig_i) = -1; 
  }
  for (int i=0; i<Ddi_BddarrayNum(delta); i++) {
    Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta,i);
    bAigEdge_t baig_i = Ddi_BddToBaig(d_i);
    bAig_AuxInt(bmgr, baig_i) = i; // set
  }
  for (int i=0; i<Ddi_BddarrayNum(cuts0); i++) {
    Ddi_Bdd_t *c_i = Ddi_BddarrayRead(cuts0,i);
    bAigEdge_t baig_i = Ddi_BddToBaig(c_i);
    int aux = bAig_AuxInt(bmgr, baig_i); 
    if (aux < 0 && !bAig_isVarNode(bmgr,baig_i)) {
      Ddi_BddarrayInsertLast(cuts,c_i);
      bAig_AuxInt(bmgr, baig_i) = i; // set
    }
  }
  for (int i=0; i<Ddi_BddarrayNum(delta); i++) {
    Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta,i);
    bAigEdge_t baig_i = Ddi_BddToBaig(d_i);
    bAig_AuxInt(bmgr, baig_i) = -1; // reset
  }
  for (int i=0; i<Ddi_BddarrayNum(cuts0); i++) {
    Ddi_Bdd_t *c_i = Ddi_BddarrayRead(cuts0,i);
    bAigEdge_t baig_i = Ddi_BddToBaig(c_i);
    bAig_AuxInt(bmgr, baig_i) = -1; 
  }
  // filter done
  
  int nc = Ddi_BddarrayNum(cuts);
  Ddi_Vararray_t *cutPsVars = Ddi_VararrayAlloc(ddm, nc);
  Ddi_Vararray_t *cutNsVars = Ddi_VararrayAlloc(ddm, nc);

  char name[100];
  int chk=1;
  int i;

  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
    fprintf(dMgrO(ddm),
            "retiming %d/%d gate cuts with redundant latches\n",
            nc, Ddi_BddarrayNum(cuts0));
  }

  for (i=0; i<nc; i++) {
    Ddi_Var_t *cutPs, *cutNs;
    Ddi_Bdd_t *cutPsLit, *cutDelta;

    sprintf(name,"retime_CUT_VAR_%d$PS", i);
    cutPs = Ddi_VarFindOrAdd(ddm, name, !genAigVars);
    Ddi_VararrayWrite(cutPsVars,i,cutPs);
    sprintf(name,"retime_CUT_VAR_%d$NS", i);
    cutNs = Ddi_VarFindOrAdd(ddm, name, !genAigVars);
    Ddi_VararrayWrite(cutNsVars,i,cutNs);

  }
  // This is the delta part after cuts
  Ddi_Bddarray_t *newD = Ddi_AigarrayInsertCuts (delta,
                           cuts, cutPsVars);

  // 

  Fsm_MgrSetVarAuxVar(fsmMgr,cutPsVars);
  Fsm_MgrSetAuxVarBDD(fsmMgr,cuts);

  // replace deltas
  // append ps/ns vars
  // append new deltas
  
  Ddi_Vararray_t *piDelta = Ddi_BddarraySuppVararray(newD);
  Ddi_Vararray_t *piRetimed = Ddi_BddarraySuppVararray(cuts);
  Ddi_Vararray_t *piToLatch = retimePis ?
    Ddi_VararrayIntersect(piRetimed,piDelta) :
    Ddi_VararrayDup(piRetimed);
  Ddi_VararrayDiffAcc(piToLatch,ps);
  int nPiLatches = Ddi_VararrayNum(piToLatch);

  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);

  if (initStub==NULL) {
    FsmCreateInitStub(fsmMgr);
    initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  }
  
  Ddi_Vararray_t *ps0 = Ddi_VararrayDup(ps);
  if (nPiLatches > 0) {
    if (retimePis) {
      Ddi_Vararray_t *newPsVars, *newNsVars;
      newPsVars = genAigVars ?
        Ddi_VararrayMakeNewAigVars(piToLatch, "PDT_RETIMED_PI_PS", "")
        : Ddi_VararrayMakeNewVars(piToLatch, "PDT_RETIMED_PI_PS", "", 1);
      newNsVars = genAigVars ?
        Ddi_VararrayMakeNewAigVars(piToLatch, "PDT_RETIMED_PI_NS", "")
        : Ddi_VararrayMakeNewVars(piToLatch, "PDT_RETIMED_PI_NS", "", 1);
      Ddi_Bddarray_t *newLatchDelta =
        Ddi_BddarrayMakeLiteralsAig(piToLatch, 1);
      Ddi_Bddarray_t *newLatchStub =
        Ddi_BddarrayMakeConstAig(ddm,nPiLatches,0);
      Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
        printf("Retiming inserted %d latches on retimed PIs\n", nPiLatches);
      }
      
      FsmMgrAddNewLatches(fsmMgr,newPsVars,newNsVars,
                          newLatchDelta,newLatchStub);
      Ddi_BddarraySubstVarsAcc(newD,piToLatch,newPsVars);
      Ddi_Free(newLatchDelta);
      Ddi_Free(newLatchStub);
      Ddi_Free(newPsVars);
      Ddi_Free(newNsVars);
    }
    else {
      
      Ddi_Bdd_t *myConstr = Ddi_BddRelMakeFromArray(cuts,cutPsVars);
      int iConstr = Ddi_BddarrayNum(delta)-2;
      Ddi_Bdd_t *constr0 = Ddi_BddarrayRead(delta,iConstr);
      Ddi_BddSetAig(myConstr);
      Ddi_BddAndAcc(constr0,myConstr);
      Ddi_BddarrayWrite(newD,iConstr,constr0);
      Ddi_Bdd_t *constrFsm = Fsm_MgrReadConstraintBDD(fsmMgr);
      if (constrFsm!=NULL)
        Ddi_BddAndAcc(myConstr,constrFsm);
      Fsm_MgrSetConstraintBDD(fsmMgr,myConstr);
      Ddi_Free(myConstr);

      Ddi_Vararray_t *newPiVars;
      newPiVars = genAigVars ?
        Ddi_VararrayMakeNewAigVars(piToLatch, "PDT_RETIMED_PI_AUX", "")
      : Ddi_VararrayMakeNewVars(piToLatch, "PDT_RETIMED_PI_AUX", "", 1);
      Ddi_BddarraySubstVarsAcc(cuts,piToLatch,newPiVars);
      Ddi_VararrayAppend(pi,newPiVars);

      Ddi_Free(newPiVars);
    }
  }

  // handle extra latches
  Ddi_Bddarray_t *cutDelta = Ddi_BddarrayCompose(cuts,ps0,newD);
  for (int i=Ddi_BddarrayNum(newD)-2; i<Ddi_BddarrayNum(delta)-2; i++) {
    Ddi_Bdd_t *copy_i = Ddi_BddarrayRead(delta,i);
    Ddi_BddarrayInsert(newD,i,copy_i);
  }
  Ddi_DataCopy(delta,newD); 
  FsmMgrAddNewLatches(fsmMgr,cutPsVars,cutNsVars,cutDelta,cuts);
  
  Ddi_Free(cuts);
  Ddi_Free(newD);
  Ddi_Free(ps0);
  Ddi_Free(cutDelta);
  Ddi_Free(piDelta);
  Ddi_Free(piRetimed);
  Ddi_Free(piToLatch);
  Ddi_Free(cutPsVars);
  Ddi_Free(cutNsVars);
  
  return (nc);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static void
fsmCoiReduction(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Varset_t * coiVars
)
{
  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Varset_t *dsupp, *stubsupp;
  int i, n;
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);

  n = Ddi_BddarrayNum(delta);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
    Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "Original Delta: size=%d; #partitions=%d.\n",
      Ddi_BddarraySize(delta), n)
    );

  for (i = n - 1; i >= 0; i--) {
    Ddi_Var_t *pv_i = Ddi_VararrayRead(ps, i);
    int isConstr = Ddi_VarName(pv_i) != NULL &&
      (strcmp(Ddi_VarName(pv_i), "PDT_BDD_INVAR_VAR$PS") == 0);
    if (!Ddi_VarInVarset(coiVars, pv_i) && !isConstr) {
      Ddi_VararrayRemove(ps, i);
      Ddi_VararrayRemove(ns, i);
      Ddi_BddarrayRemove(delta, i);
      Ddi_VararrayInsertLast(pi, pv_i);
      if (initStub != NULL) {
        Ddi_BddarrayRemove(initStub, i);
      }
    }
  }

  n = Ddi_BddarrayNum(delta);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
    Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "Reduced Delta: Size=%d; #Partitions=%d.\n",
      Ddi_BddarraySize(delta), n)
    );

  dsupp = Ddi_BddarraySupp(delta);
  if (initStub != NULL) {
    stubsupp = Ddi_BddarraySupp(initStub);
    Ddi_VarsetUnionAcc(dsupp, stubsupp);
    Ddi_Free(stubsupp);
  }
  for (i = Ddi_VararrayNum(pi) - 1; i >= 0; i--) {
    Ddi_Var_t *pi_i = Ddi_VararrayRead(pi, i);

    if (!Ddi_VarInVarset(dsupp, pi_i)) {
      Ddi_VararrayRemove(pi, i);
    }
  }
  if (initStub == NULL) {
    Ddi_Varset_t *proj = Ddi_VarsetMakeFromArray(ps);
    Ddi_Varset_t *proj_pi = Ddi_VarsetMakeFromArray(pi);
    Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);

    Ddi_VarsetUnionAcc(proj, proj_pi);

    Ddi_BddExistProjectAcc(init, proj);
    Ddi_Free(proj);
    Ddi_Free(proj_pi);
  }
  Ddi_Free(dsupp);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Varsetarray_t *
computeFsmCoiVars(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * p,
  int maxIter
)
{
  Ddi_Varsetarray_t *coirings;
  Ddi_Varset_t *ps, *ns, *supp, *cone, *New, *newnew;
  Ddi_Bddarray_t *delta;
  Ddi_Vararray_t *psv;
  Ddi_Vararray_t *nsv;
  Ddi_Varsetarray_t *domain, *range;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(p);

  int i, j, np;

  psv = Fsm_MgrReadVarPS(fsmMgr);
  nsv = Fsm_MgrReadVarNS(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);

  ps = Ddi_VarsetMakeFromArray(psv);
  ns = Ddi_VarsetMakeFromArray(nsv);

  coirings = Ddi_VarsetarrayAlloc(ddm, 0);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
    Pdtutil_VerbLevelUsrMax_c, fprintf(stdout, "COI Reduction.\n")
    );

  np = Ddi_BddarrayNum(delta);
  domain = Ddi_VarsetarrayAlloc(ddm, np);
  range = Ddi_VarsetarrayAlloc(ddm, np);
  for (i = 0; i < np; i++) {
    supp = Ddi_BddSupp(Ddi_BddarrayRead(delta, i));
    Ddi_VarsetIntersectAcc(supp, ps);
    Ddi_VarsetarrayWrite(domain, i, supp);
    Ddi_Free(supp);
    supp = Ddi_VarsetMakeFromVar(Ddi_VararrayRead(nsv, i));
    Ddi_VarsetarrayWrite(range, i, supp);
    Ddi_Free(supp);
  }

  cone = Ddi_BddSupp(p);
  Ddi_VarsetIntersectAcc(cone, ps); /* remove PI vars */

  New = Ddi_VarsetDup(cone);

  j = 0;
  while (((j++ < maxIter) || (maxIter <= 0)) && (!Ddi_VarsetIsVoid(New))) {
    Ddi_VarsetUnionAcc(cone, New);
    Ddi_VarsetarrayInsertLast(coirings, cone);
    Ddi_VarsetSwapVarsAcc(New, psv, nsv);
    newnew = Ddi_VarsetVoid(ddm);
    for (i = 0; i < np; i++) {
      Ddi_Varset_t *common;

      supp = Ddi_VarsetarrayRead(range, i);
      common = Ddi_VarsetIntersect(supp, New);
      if (!Ddi_VarsetIsVoid(common)) {
#if 0
        if ((coiSets != NULL) && (coiSets[i] != NULL)) {
          Ddi_VarsetUnionAcc(cone, coiSets[i]);
        } else
#endif
        {
          Ddi_VarsetUnionAcc(newnew, Ddi_VarsetarrayRead(domain, i));
        }
      }
      Ddi_Free(common);
    }
    Ddi_Free(New);
    New = Ddi_VarsetDiff(newnew, cone);
    Ddi_Free(newnew);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelUsrMax_c, fprintf(stdout, ".(%d)", Ddi_VarsetNum(cone));
      fflush(stdout)
      );
  }

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
    Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "COI Reduction has kept %d State Variables out of %d.\n",
      Ddi_VarsetNum(cone), Ddi_VarsetNum(ps))
    );

  Ddi_Free(New);
  Ddi_Free(domain);
  Ddi_Free(range);
  Ddi_Free(ps);
  Ddi_Free(ns);

  Ddi_Free(cone);

  return (coirings);

}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
writeCoiShells(
  char *wrCoi,
  Ddi_Varsetarray_t * coi,
  Ddi_Vararray_t * vars
)
{
  Ddi_Varset_t *curr, *old, *New, *varSet;
  FILE *fp = fopen(wrCoi, "w");
  int i;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(coi);

  varSet = Ddi_VarsetMakeFromArray(vars);
  old = NULL;
  for (i = 0; i < Ddi_VarsetarrayNum(coi); i++) {
    curr = Ddi_VarsetarrayRead(coi, i);
    if (old == NULL) {
      New = Ddi_VarsetDup(curr);
    } else {
      New = Ddi_VarsetDiff(curr, old);
    }
    Ddi_VarsetIntersectAcc(New, varSet);
    old = curr;
    Ddi_VarsetPrint(New, 0, NULL, fp);
    if (1 || i < Ddi_VarsetarrayNum(coi) - 1) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelUsrMax_c, fprintf(fp, "#\n")
        );
    }

    Ddi_Free(New);
  }

  fclose(fp);
  Ddi_Free(varSet);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
bmcAbstrCheck(
  Ddi_Bddarray_t * delta,
  Ddi_Bdd_t * cone,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * coneAndInit,
  Ddi_Bddarray_t * initStub,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * pi,
  int maxBound,
  Ddi_Varset_t * psRefNew,
  int *prevSatBound
)
{
  int i, initIsCube = 0;
  Ddi_Bdd_t *cex, *myCone, *cubeInit;
  Ddi_Varset_t *piv2, *piv, *psv;

  psv = Ddi_VarsetMakeFromArray(ps);
  piv = Ddi_BddarraySupp(delta);
  Ddi_VarsetDiffAcc(piv, psv);
  myCone = Ddi_BddDup(cone);

  if (initStub == NULL) {
    cubeInit = Ddi_BddMakeMono(init);
    initIsCube = Ddi_BddIsCube(cubeInit);
    Ddi_Free(cubeInit);
  }

  for (i = 0; i < maxBound; i++) {
    Ddi_Bdd_t *check = Ddi_BddDup(myCone);
    Ddi_Vararray_t *pi2, *piSubst;
    Ddi_Bddarray_t *piSubstLits;
    char suffix[20];

    sprintf(suffix, "%d", i + 1);
    if (initStub != NULL) {
      Ddi_BddComposeAcc(check, ps, initStub);
    } else if (initIsCube) {
      Ddi_AigAndCubeAcc(check, init);
    } else {
      Ddi_BddAndAcc(check, init);
    }
    cex = Ddi_AigSatWithCex(check);
    if (cex != NULL) {
      if (prevSatBound != NULL) {
        Pdtutil_Assert(*prevSatBound <= i, "wrong val for prevSatBound");
        *prevSatBound = i;
      }
      Ddi_DataCopy(coneAndInit, check);
      Ddi_Free(check);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelUsrMax_c, fprintf(stdout, "BMC SAT at Bound %d.\n", i)
        );
      break;
    } else if (prevSatBound != NULL && (i == *prevSatBound)) {
      /* refinement OK for previous SAT bound */
      /* try proof based abstraction */

      Ddi_Bdd_t *abstrCone =
        bmcPba(delta, cone, init, initStub, piv, ps, pi, i, psRefNew);
      if (abstrCone != NULL) {
        /* refined varset has been abstracted */
        Ddi_DataCopy(myCone, abstrCone);
        Ddi_Free(abstrCone);
        Ddi_Free(piv);
        piv = Ddi_BddarraySupp(delta);
        Ddi_VarsetDiffAcc(piv, psv);
      }
    }
    Ddi_Free(check);

    piv2 = Ddi_BddSupp(myCone);
    Ddi_VarsetIntersectAcc(piv2, piv);
    pi2 = Ddi_VararrayMakeFromVarset(piv2, 1);
    piSubst = Ddi_VararrayMakeNewVars(pi2, "PDT_CEGAR_TF_PI", suffix, 1);
    piSubstLits = Ddi_BddarrayMakeLiteralsAig(piSubst, 1);
    Ddi_BddComposeAcc(myCone, pi2, piSubstLits);
    Ddi_BddComposeAcc(myCone, ps, delta);

    Ddi_Free(piSubst);
    Ddi_Free(piSubstLits);
    Ddi_Free(pi2);
    Ddi_Free(piv2);
  }

  Ddi_Free(myCone);
  Ddi_Free(piv);
  Ddi_Free(psv);

  return (cex);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Varset_t *
refineWithCex(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * coneAndInit,
  Ddi_Bdd_t * cex,
  Ddi_Varset_t * psIn,
  Ddi_Varset_t * psOut
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(cex);
  Ddi_Varset_t *refinedPs = Ddi_VarsetVoid(ddm);
  Ddi_Bdd_t *cexPart = Ddi_AigPartitionTop(cex, 0);
  Ddi_Bddarray_t *cexArray = Ddi_BddarrayMakeFromBddPart(cexPart);
  Ddi_Bdd_t *myOne = Ddi_BddMakeConstAig(ddm, 1);
  Ddi_Bdd_t *myZero = Ddi_BddMakeConstAig(ddm, 0);
  Ddi_Vararray_t *psInA = Ddi_VararrayMakeFromVarset(psIn, 1);
  int i, n = Ddi_BddarrayNum(cexArray);

  Ddi_Vararray_t *vA = Ddi_VararrayAlloc(ddm, n);
  Ddi_Bddarray_t *vSubst = Ddi_BddarrayAlloc(ddm, n);
  Pdtutil_Array_t *cexArrayTot, *cexArrayPs, *simArray;
  int cegarStrategy = Fsm_MgrReadCegarStrategy(fsmMgr);
  int useSim = cegarStrategy > 1;
  int useX = cegarStrategy > 2;
  Fsm_XsimMgr_t *xMgr = NULL;

  Ddi_Free(psInA);

  for (i = 0; i < n; i++) {
    Ddi_Bdd_t *p = Ddi_BddarrayRead(cexArray, i);
    Ddi_Var_t *v = Ddi_BddTopVar(p);

    Ddi_VararrayWrite(vA, i, v);
    if (Ddi_BddIsComplement(p)) {
      Ddi_BddarrayWrite(vSubst, i, myZero);
    } else {
      Ddi_BddarrayWrite(vSubst, i, myOne);
    }
  }

  Ddi_BddNotAcc(coneAndInit);   /* target is complement of invarspec parameter */
  xMgr = Fsm_XsimInit(fsmMgr, NULL, coneAndInit, vA, NULL, 0);
  Ddi_BddNotAcc(coneAndInit);

  cexArrayTot = Ddi_AigCubeMakeIntegerArrayFromBdd(cex, vA);
  cexArrayPs = Ddi_AigCubeMakeIntegerArrayFromBdd(cex, psInA);

  simArray = Fsm_XsimSimulate(xMgr, 1, cexArrayTot, 1, NULL);
  Pdtutil_Assert(simArray != NULL
    && Pdtutil_IntegerArrayRead(simArray, 0) > 0, "wrong sim val");
  Pdtutil_IntegerArrayFree(simArray);

  for (i = 0; i < n; i++) {
    Ddi_Bdd_t *p = Ddi_BddarrayRead(cexArray, i);
    Ddi_Var_t *v = Ddi_BddTopVar(p);
    char *vname = Ddi_VarName(v);
    int bitRes, bitVal = Ddi_BddIsOne(p);

    if (strncmp(vname, "PDT_CEGAR", 9) == 0) {
      Ddi_Bdd_t *res;
      Ddi_Var_t *refV = NULL;
      Ddi_Bdd_t *lit = NULL;

      if (strncmp(vname, "PDT_CEGAR_ABSTR", 15) == 0) {
        char refName[100];
        int k;

        sscanf(vname, "PDT_CEGAR_ABSTR_%s", refName);
        // refName[strlen(refName)-1]='\0';
        refV = Ddi_VarFromName(ddm, refName);
      } else if (strncmp(vname, "PDT_CEGAR_TF_PI", 15) == 0) {
        char refName[100];
        int k;

        if (sscanf(vname, "PDT_CEGAR_TF_PI_PDT_CEGAR_ABSTR_%s", refName) == 1) {
          for (k = strlen(refName) - 1; refName[k] != '_'; k--) ;
          //          refName[--k]='\0'; // wrong k decrement;
          refName[k] = '\0';
          refV = Ddi_VarFromName(ddm, refName);
        }
#if 0
        else if (sscanf(vname, "PDT_CEGAR_TF_PI_%s", refName) == 1) {
          for (k = strlen(refName) - 1; refName[k] != '_'; k--) ;
          refName[k] = '\0';
        }
#endif
      }
      if (refV == NULL || Ddi_VarInVarset(refinedPs, refV)) {
        /* already refined */
        continue;
      }

      simArray =
        Fsm_XsimSimulatePsBit(xMgr, i,
        useX ? 2 : (bitVal ? 0 : 1 /*complement */ ), 1, NULL);
      Pdtutil_Assert(simArray != NULL, "wrong sim val");
      bitRes = Pdtutil_IntegerArrayRead(simArray, 0) == 1;
      Pdtutil_IntegerArrayFree(simArray);
      /* reset xsim */
      //      simArray = Fsm_XsimSimulatePsBit(xMgr,i,bitRes?2:bitVal,0,NULL);
      simArray = Fsm_XsimSimulatePsBit(xMgr, i, bitVal, 0, NULL);
      Pdtutil_Assert(simArray == NULL, "wrong sim array");

      lit = Ddi_BddMakeLiteralAig(v, 1);
      Ddi_BddarrayWrite(vSubst, i, lit);
      if (useSim) {
        if (!bitRes) {
          Ddi_VarsetAddAcc(refinedPs, refV);
        } else if (useX) {
          simArray = Fsm_XsimSimulatePsBit(xMgr, i, 2, 0, NULL);
          Pdtutil_Assert(simArray == NULL, "wrong sim array");
        }
      } else {
        res = Ddi_BddCompose(coneAndInit, vA, vSubst);
        //  Pdtutil_Assert(bitRes==Ddi_BddIsOne(res),"xsim - aigeval mismatch");
        if (!Ddi_BddIsOne(res)) {
          Ddi_VarsetAddAcc(refinedPs, refV);
          if (Ddi_BddIsComplement(p)) {
            Ddi_BddarrayWrite(vSubst, i, myZero);
          } else {
            Ddi_BddarrayWrite(vSubst, i, myOne);
          }
        }
        Ddi_Free(res);
      }
      Ddi_Free(lit);
    }
  }

  Pdtutil_IntegerArrayFree(cexArrayTot);
  Pdtutil_IntegerArrayFree(cexArrayPs);
  Fsm_XsimQuit(xMgr);

  Ddi_Free(myOne);
  Ddi_Free(myZero);

  Ddi_Free(vA);
  Ddi_Free(vSubst);
  Ddi_Free(cexArray);
  Ddi_Free(cexPart);

  return refinedPs;
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
bmcPba(
  Ddi_Bddarray_t * delta,
  Ddi_Bdd_t * cone,
  Ddi_Bdd_t * init,
  Ddi_Bddarray_t * initStub,
  Ddi_Varset_t * piv,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * pi,
  int maxBound,
  Ddi_Varset_t * psRefNew
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(delta);
  Ddi_Bddarray_t *deltaMux = Ddi_BddarrayDup(delta);
  Ddi_Vararray_t *psRefNewA = Ddi_VararrayMakeFromVarset(psRefNew, 1);
  Ddi_Vararray_t *psRefNewSubst =
    Ddi_VararrayMakeNewVars(psRefNewA, "PDT_CEGAR_ABSTR", "", 0);
  Ddi_Vararray_t *psRefNewPba =
    Ddi_VararrayMakeNewVars(psRefNewA, "PDT_PBA_ABSTR", "", 0);
  Ddi_Bddarray_t *psRefNewSubstLits =
    Ddi_BddarrayMakeLiteralsAig(psRefNewSubst, 1);
  Ddi_Bddarray_t *psRefNewLits = Ddi_BddarrayMakeLiteralsAig(psRefNewA, 1);
  Ddi_Bdd_t *myCone = Ddi_BddDup(cone);
  Ddi_Bdd_t *check;
  Ddi_Bdd_t *assumeLatch = Ddi_BddMakeConstAig(ddm, 1);
  Ddi_Bdd_t *assumeAbstr = Ddi_BddMakeConstAig(ddm, 1);
  Ddi_Varset_t *psPba;

  int k, sat;

  int initIsCube = 0;

  if (initStub == NULL) {
    Ddi_Bdd_t *cubeInit = Ddi_BddMakeMono(init);

    initIsCube = Ddi_BddIsCube(cubeInit);
    Ddi_Free(cubeInit);
  }


  for (k = 0; k < Ddi_BddarrayNum(psRefNewLits); k++) {
    Ddi_Var_t *pba_k = Ddi_VararrayRead(psRefNewPba, k);
    Ddi_Bdd_t *psNew_k = Ddi_BddarrayRead(psRefNewLits, k);
    Ddi_Bdd_t *psNewSubst_k = Ddi_BddarrayRead(psRefNewSubstLits, k);
    Ddi_Bdd_t *pbaAssume = Ddi_BddMakeLiteralAig(pba_k, 1);
    Ddi_Bdd_t *mux_k = Ddi_BddIte(pbaAssume, psNew_k, psNewSubst_k);

    Ddi_BddarrayWrite(psRefNewLits, k, mux_k);
    Ddi_BddAndAcc(assumeLatch, pbaAssume);
    Ddi_BddDiffAcc(assumeAbstr, pbaAssume);
    Ddi_Free(pbaAssume);
    Ddi_Free(mux_k);
  }

  Ddi_BddarrayComposeAcc(deltaMux, psRefNewA, psRefNewLits);

  /* generate unrolling */

  for (k = 0; k < maxBound; k++) {
    Ddi_Vararray_t *pi2, *piSubst;
    Ddi_Varset_t *piv2;
    Ddi_Bddarray_t *piSubstLits;
    char suffix[20];

    sprintf(suffix, "%d", k + 1);

    piv2 = Ddi_BddSupp(myCone);
    Ddi_VarsetIntersectAcc(piv2, piv);
    pi2 = Ddi_VararrayMakeFromVarset(piv2, 1);
    piSubst = Ddi_VararrayMakeNewVars(pi2, "PDT_CEGAR_TF_PI", suffix, 1);
    piSubstLits = Ddi_BddarrayMakeLiteralsAig(piSubst, 1);
    Ddi_BddComposeAcc(myCone, pi2, piSubstLits);
    Ddi_Free(piSubst);
    Ddi_Free(piSubstLits);
    Ddi_BddComposeAcc(myCone, ps, deltaMux);

    piSubst =
      Ddi_VararrayMakeNewVars(psRefNewSubst, "PDT_CEGAR_TF_PI", suffix, 1);
    piSubstLits = Ddi_BddarrayMakeLiteralsAig(piSubst, 1);
    Ddi_BddComposeAcc(myCone, psRefNewSubst, piSubstLits);
    Ddi_Free(piSubst);
    Ddi_Free(piSubstLits);

    Ddi_Free(pi2);
    Ddi_Free(piv2);
  }

  check = Ddi_BddDup(myCone);

  if (initStub != NULL) {
    Ddi_BddComposeAcc(check, ps, initStub);
  }
  if (initIsCube) {
    Ddi_AigAndCubeAcc(check, init);
  } else {
    Ddi_BddAndAcc(check, init);
  }

  sat = Ddi_AigSatMinisatWithAbortAndFinal(check, assumeLatch, -1, 0);
  Pdtutil_Assert(!sat, "UNSAT required for final core");

  psPba = Ddi_BddSupp(assumeLatch);
  if (Ddi_VarsetNum(psPba) < Ddi_BddarrayNum(psRefNewLits)) {
    Ddi_Varset_t *myPsRef;

    Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "PBA refinement selected %d/%d state vars\n",
        Ddi_VarsetNum(psPba), Ddi_BddarrayNum(psRefNewLits))
      );

    /* set assumed controls to 1 */
    Ddi_AigarrayConstrainCubeAcc(deltaMux, assumeLatch);
    Ddi_AigConstrainCubeAcc(myCone, assumeLatch);
    /* set other (abstracted) controls to 0 */
    Ddi_AigarrayConstrainCubeAcc(deltaMux, assumeAbstr);
    Ddi_AigConstrainCubeAcc(myCone, assumeAbstr);

    /* find set of abstr latches */
    myPsRef = Ddi_BddarraySupp(deltaMux);
    Ddi_VarsetIntersectAcc(psRefNew, myPsRef);
    Ddi_Free(myPsRef);

    Ddi_DataCopy(delta, deltaMux);
  }

  Ddi_Free(psPba);

  Ddi_Free(check);

  /* abstract delta */

  Ddi_Free(assumeLatch);
  Ddi_Free(assumeAbstr);
  //  Ddi_Free(myCone);
  Ddi_Free(psRefNewA);
  Ddi_Free(psRefNewSubst);
  Ddi_Free(psRefNewPba);
  Ddi_Free(psRefNewSubstLits);
  Ddi_Free(psRefNewLits);

  Ddi_Free(deltaMux);

  return (myCone);
}

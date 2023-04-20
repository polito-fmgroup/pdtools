/**CFile***********************************************************************

  FileName    [travApprox.c]

  PackageName [trav]

  Synopsis    [Functions to perform approximate traversals]

  Description []

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

#include "travInt.h"
#include "trInt.h"
#include "ddiInt.h"

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

#define LOG_CONST(f,msg) \
if (Ddi_BddIsOne(f)) {\
  fprintf(stdout, "%s = 1\n", msg);            \
}\
else if (Ddi_BddIsZero(f)) {\
  fprintf(stdout, "%s = 0\n", msg);            \
}


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static Ddi_Bdd_t *PartitionTrav(
  Trav_Mgr_t * travMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * care
);

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of external functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Trav_ApproxMBM(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Ddi_Bddarray_t * deltaAig
)
{
  Ddi_Mgr_t *ddiMgr, *ddR;
  Tr_Tr_t *trActive = Trav_MgrReadTr(travMgr);
  int i, np, again, step;
  Ddi_Bdd_t *init, *reached, *newReached, *trBdd;
  Ddi_Varsetarray_t *partVars;
  Ddi_Varset_t *nsv;
  Tr_Mgr_t *trMgr = Tr_TrMgr(trActive);
  Ddi_Vararray_t *ps = Tr_MgrReadPS(trMgr);
  Ddi_Vararray_t *ns = Tr_MgrReadNS(trMgr);
  int itpAppr = Trav_MgrReadItpAppr(travMgr);
  int satRefine = itpAppr > 0 ? itpAppr : 0;
  Ddi_Bdd_t *trAig = NULL;
  Ddi_Bdd_t *initMono = NULL;

  Pdtutil_Assert(travMgr != NULL, "NULL traversal manager");

  ddiMgr = travMgr->ddiMgrTr;
  ddR = travMgr->ddiMgrR;
  Pdtutil_Assert(ddiMgr == ddR,
    "dual DDI manager not supported by Approx MBM");

  trBdd = Tr_TrBdd(trActive);
  nsv = Ddi_VarsetMakeFromArray(ns);

  np = Ddi_BddPartNum(trBdd);

  initMono = Ddi_BddDup(travMgr->from);

  init = Ddi_BddMakePartConjVoid(ddiMgr);
  reached = Ddi_BddMakePartConjVoid(ddiMgr);
  newReached = Ddi_BddMakePartConjVoid(ddiMgr);

  partVars = Ddi_VarsetarrayAlloc(ddiMgr, np);

  if (satRefine) {
    Pdtutil_Assert(deltaAig != NULL, "missing delta aig");
    trAig = Ddi_BddRelMakeFromArray(deltaAig, ns);
    Ddi_BddSetAig(trAig);
  }

  for (i = 0; i < np; i++) {
    Ddi_Varset_t *supp_i = Ddi_BddSupp(Ddi_BddPartRead(trBdd, i));
    Ddi_Bdd_t *init_i;

    Ddi_VarsetIntersectAcc(supp_i, nsv);
    Ddi_VarsetSwapVarsAcc(supp_i, ps, ns);
    Ddi_VarsetarrayInsert(partVars, i, supp_i);

    init_i = Ddi_BddExistProject(travMgr->from, supp_i);
    Ddi_BddPartWrite(init, i, init_i);
    Ddi_BddPartWrite(reached, i, Ddi_MgrReadOne(ddiMgr));

    Ddi_Free(init_i);
    Ddi_Free(supp_i);
  }

  step = 0;
  do {
    again = 0;
    Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "MBM Iteration %d (#p:%d) : ", step, np)
      );
    for (i = 0; i < np; i++) {
      Tr_Tr_t *TR_i = Tr_TrMakeFromRel(trMgr, Ddi_BddPartRead(trBdd, i));
      Ddi_Bdd_t *init_i, *care_i, *reached_i, *newReached_i;

      init_i = Ddi_BddPartRead(init, i);
      reached_i = Ddi_BddPartRead(reached, i);

      Ddi_BddRestrictAcc(Tr_TrBdd(TR_i), reached);
      care_i = reached_i;
      newReached_i = PartitionTrav(travMgr, TR_i, init_i, NULL);
      Ddi_BddOrAcc(newReached_i, init_i);
      Ddi_BddAndAcc(newReached_i, care_i);

      Tr_TrFree(TR_i);

      if (!Ddi_BddEqual(reached_i, newReached_i)) {
        again = 1;
      }
      Ddi_BddPartWrite(newReached, i, newReached_i);
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "[%d]", Ddi_BddSize(newReached_i))
        );
      Ddi_Free(newReached_i);
    }
    for (; i < Ddi_BddPartNum(reached); i++) {
      Ddi_BddPartInsertLast(newReached, Ddi_BddPartRead(reached, i));
    }
    Ddi_Free(reached);
    reached = Ddi_BddDup(newReached);
    Ddi_BddSetPartConj(reached);

    if (satRefine) {
      int nv = Ddi_VararrayNum(ps);
      double cnt0, cnt1;
      Ddi_Bdd_t *refine, *r = Ddi_BddDup(reached);
      Ddi_Bdd_t *rCnt = Ddi_BddMakeMono(r);

      cnt0 = Ddi_BddCountMinterm(rCnt, nv);
      Pdtutil_Assert(Ddi_BddIncluded(initMono, rCnt), "wrong base chk");
      Ddi_Free(rCnt);
      refine =
        Trav_ApproxSatRefine(r, r, trAig, NULL, ps, ns, initMono, satRefine,
        1);
      rCnt = Ddi_BddMakeMono(r);
      cnt1 = Ddi_BddCountMinterm(rCnt, nv);
      Ddi_Free(rCnt);
      Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMax_c) {
        fprintf(stdout, "#R states: %g -> %g (sat tightening: %g)\n",
          cnt0, cnt1, cnt0 / cnt1);
        if (refine != NULL) {
          Pdtutil_Assert(Ddi_BddIncluded(initMono, refine), "wrong base chk");
          Ddi_BddPartInsertLast(reached, refine);
          Ddi_Free(refine);
          if (cnt0 / cnt1 > 1.05)
            again = 1;
        }
        Ddi_Free(r);
      }
    }


    Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "\n")
      );
    step++;
  }
  while (again);

  Trav_MgrSetReached(travMgr, reached);

  Ddi_Free(trAig);
  Ddi_Free(partVars);
  Ddi_Free(init);
  Ddi_Free(initMono);
  Ddi_Free(newReached);
  Ddi_Free(reached);
  Ddi_Free(nsv);

  return (Trav_MgrReadReached(travMgr));
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Trav_ApproxLMBM(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Ddi_Bddarray_t * deltaAig
)
{
  Ddi_Mgr_t *ddiMgr, *ddR;
  Tr_Tr_t *trActive = Trav_MgrReadTr(travMgr);
  int i, np, again, step;
  Ddi_Bdd_t *init, *reached, *newReached, *trBdd;
  Ddi_Varsetarray_t *partVars;
  Ddi_Varset_t *nsv;
  Tr_Mgr_t *trMgr = Tr_TrMgr(trActive);
  Ddi_Vararray_t *ps = Tr_MgrReadPS(trMgr);
  Ddi_Vararray_t *ns = Tr_MgrReadNS(trMgr);
  int itpAppr = Trav_MgrReadItpAppr(travMgr);
  int satRefine = itpAppr > 0 ? itpAppr : 0;
  Ddi_Bdd_t *trAig = NULL;
  Ddi_Bdd_t *initMono = NULL;

  Pdtutil_Assert(travMgr != NULL, "NULL traversal manager");

  ddiMgr = travMgr->ddiMgrTr;
  ddR = travMgr->ddiMgrR;
  Pdtutil_Assert(ddiMgr == ddR,
    "dual DDI manager not supported by Approx MBM");

  trBdd = Tr_TrBdd(trActive);
  nsv = Ddi_VarsetMakeFromArray(ns);

  np = Ddi_BddPartNum(trBdd);

  initMono = Ddi_BddDup(travMgr->from);

  init = Ddi_BddMakePartConjVoid(ddiMgr);
  reached = Ddi_BddMakePartConjVoid(ddiMgr);
  newReached = Ddi_BddMakePartConjVoid(ddiMgr);

  partVars = Ddi_VarsetarrayAlloc(ddiMgr, np);

  if (satRefine) {
    Pdtutil_Assert(deltaAig != NULL, "missing delta aig");
    trAig = Ddi_BddRelMakeFromArray(deltaAig, ns);
    Ddi_BddSetAig(trAig);
  }

  for (i = 0; i < np; i++) {
    Ddi_Varset_t *supp_i = Ddi_BddSupp(Ddi_BddPartRead(trBdd, i));
    Ddi_Bdd_t *init_i;

    Ddi_VarsetIntersectAcc(supp_i, nsv);
    Ddi_VarsetSwapVarsAcc(supp_i, ps, ns);
    Ddi_VarsetarrayInsert(partVars, i, supp_i);

    init_i = Ddi_BddExistProject(travMgr->from, supp_i);
    Ddi_BddPartWrite(init, i, init_i);
    //    Ddi_BddPartWrite(reached,i,Ddi_MgrReadOne(ddiMgr));
    Ddi_BddPartWrite(reached, i, init_i);

    Ddi_Free(init_i);
    Ddi_Free(supp_i);
  }

  step = 0;
  do {
    again = 0;
    Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "MBM Iteration %d (#p:%d) : ", step, np)
      );
    for (i = 0; i < np; i++) {
      Tr_Tr_t *TR_i = Tr_TrMakeFromRel(trMgr, Ddi_BddPartRead(trBdd, i));
      Ddi_Bdd_t *init_i, *care_i, *reached_i, *newReached_i;

      init_i = Ddi_BddPartRead(init, i);
      reached_i = Ddi_BddPartRead(reached, i);

      care_i = Ddi_BddDup(reached);
      Ddi_BddPartRemove(care_i, i);
      Ddi_BddRestrictAcc(Tr_TrBdd(TR_i), care_i);

      newReached_i = PartitionTrav(travMgr, TR_i, init_i, NULL);
      Ddi_BddOrAcc(newReached_i, init_i);
      //      Ddi_BddAndAcc(newReached_i,care_i);

      Tr_TrFree(TR_i);

      if (!Ddi_BddEqual(reached_i, newReached_i)) {
        again = 1;
      }
      Ddi_BddPartWrite(newReached, i, newReached_i);
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "[%d]", Ddi_BddSize(newReached_i))
        );
      Ddi_Free(care_i);
      Ddi_Free(newReached_i);
    }
    for (; i < Ddi_BddPartNum(reached); i++) {
      Ddi_BddPartInsertLast(newReached, Ddi_BddPartRead(reached, i));
    }
    Ddi_Free(reached);
    reached = Ddi_BddDup(newReached);
    Ddi_BddSetPartConj(reached);

    if (satRefine) {
      int nv = Ddi_VararrayNum(ps);
      double cnt0, cnt1;
      Ddi_Bdd_t *refine, *r = Ddi_BddDup(reached);
      Ddi_Bdd_t *rCnt = Ddi_BddMakeMono(r);

      cnt0 = Ddi_BddCountMinterm(rCnt, nv);
      Pdtutil_Assert(Ddi_BddIncluded(initMono, rCnt), "wrong base chk");
      Ddi_Free(rCnt);
      refine =
        Trav_ApproxSatRefine(r, r, trAig, NULL, ps, ns, initMono, satRefine,
        1);
      rCnt = Ddi_BddMakeMono(r);
      cnt1 = Ddi_BddCountMinterm(rCnt, nv);
      Ddi_Free(rCnt);
      Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMax_c) {
        fprintf(stdout, "#R states: %g -> %g (sat tightening: %g)\n",
          cnt0, cnt1, cnt0 / cnt1);
        if (refine != NULL) {
          Pdtutil_Assert(Ddi_BddIncluded(initMono, refine), "wrong base chk");
          Ddi_BddPartInsertLast(reached, refine);
          Ddi_Free(refine);
          if (cnt0 / cnt1 > 1.05)
            again = 1;
        }
        Ddi_Free(r);
      }
    }


    Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "\n")
      );
    step++;
  }
  while (again);

  Trav_MgrSetReached(travMgr, reached);

  Ddi_Free(trAig);
  Ddi_Free(partVars);
  Ddi_Free(init);
  Ddi_Free(initMono);
  Ddi_Free(newReached);
  Ddi_Free(reached);
  Ddi_Free(nsv);

  return (Trav_MgrReadReached(travMgr));
}

/**Function********************************************************************
  Synopsis    [Apply top-down reduction of ones to bottom layer]
  Description [Apply top-down reduction of ones to bottom layer. This reduces
    a Meta BDD to a conjunctive form]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Trav_ApproxSatRefine(
  Ddi_Bdd_t * to,
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * trAig,
  Ddi_Bdd_t * prevRef,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * ns,
  Ddi_Bdd_t * init,
  int refineIter,
  int useInduction
)
{
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(to);
  int res;
  Ddi_Bdd_t *toGen;
  int nState = Ddi_VararrayNum(ps);
  Ddi_Bdd_t *a = Ddi_BddMakeAig(to);
  Ddi_Bdd_t *b = Ddi_BddMakeAig(from);
  Ddi_Vararray_t *glbA;
  Ddi_Varset_t *glbVars;
  Ddi_Vararray_t *dynAC = NULL, *dynAA = NULL;
  Ddi_Bddarray_t *dynACL = NULL;
  Ddi_Varset_t *nsvars = Ddi_VarsetMakeFromArray(ns);
  Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity(ddiMgr);
  int doPush = !useInduction && prevRef != NULL;
  Ddi_Bdd_t *ref = NULL;

  Ddi_BddSwapVarsAcc(a, ps, ns);
  Ddi_BddAndAcc(b, trAig);

  if (doPush && !Ddi_BddIsOne(prevRef)) {
    unsigned long t = Ddi_MgrReadAigSatTimeLimit(ddiMgr);

    Ddi_MgrSetAigSatTimeLimit(ddiMgr, 2);
    Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
      fprintf(dMgrO(ddiMgr), "\n"));

    ref = Ddi_AigInductiveImgPlus(b, prevRef, NULL, NULL, 2);
    Ddi_MgrSetAigSatTimeLimit(ddiMgr, t);
    if (ref != NULL) {
      Ddi_DataCopy(prevRef, ref);
      if (!Ddi_BddIsOne(ref)) {
        Ddi_BddAndAcc(a, ref);
      }
    }
  }

  Ddi_BddNotAcc(a);

  if (init != NULL && useInduction) {
    Ddi_Bdd_t *initNs = Ddi_BddSwapVars(init, ns, ps);

    Ddi_BddSetAig(initNs);
    Ddi_BddOrAcc(b, initNs);
    Ddi_Free(initNs);
  }

  glbVars = Ddi_BddSupp(a);
  Ddi_VarsetIntersectAcc(glbVars, nsvars);
  Ddi_Free(nsvars);
  glbA = Ddi_VararrayMakeFromVarset(glbVars, 1);

  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
    fprintf(dMgrO(ddiMgr), "|to+GENeralization:%d|.\n", Ddi_BddSize(to))
    );
  toGen = Ddi_AigInterpolantByGenClauses(a, b, NULL, NULL,
    ps, ns, init, glbA, dynAC, dynAA, dynACL, refineIter, useInduction, &res);
  Ddi_Free(glbA);
  Ddi_Free(glbVars);

  if (res >= 0) {
    Ddi_BddNotAcc(toGen);
    if (doPush) {
      Ddi_BddAndAcc(prevRef, toGen);
      if (ref != NULL) {
        Ddi_BddAndAcc(toGen, ref);
      }
    }
    Ddi_BddSwapVarsAcc(toGen, ps, ns);
    Ddi_BddSetMono(toGen);
    if (!Ddi_BddIncluded(to, toGen)) {
      if (Ddi_BddIsPartConj(to)) {
        Ddi_BddPartInsertLast(to, toGen);
      } else {
        Ddi_BddAndAcc(to, toGen);
      }
      Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
        fprintf(dMgrO(ddiMgr), "|to+GEN:%d|\n", Ddi_BddSize(to))
        );
    }
  }

  Ddi_Free(a);
  Ddi_Free(b);
  Ddi_Free(ref);

  return toGen;

}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Trav_ApproxPreimg(
  Tr_Tr_t * TR,
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * window,
  int doApprox,
  Trav_Settings_t * opt
)
{
  Ddi_Varset_t *ps, *ns, *smoothV, *pi, *abstract, *mySmooth;
  Tr_Mgr_t *trMgr = Tr_TrMgr(TR);
  Ddi_Bdd_t *trBdd, *to, *toPlus, *myFrom, *myCare, *myConflict = NULL;
  Ddi_Vararray_t *psv;
  Ddi_Vararray_t *nsv;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(from);
  char msg[101];
  Ddi_Bdd_t *clk_part = NULL;

  psv = Tr_MgrReadPS(trMgr);
  nsv = Tr_MgrReadNS(trMgr);

  trBdd = Tr_TrBdd(TR);

  if (Ddi_BddIsPartDisj(trBdd)) {
    int i, np = Ddi_BddPartNum(trBdd);
    Ddi_Bdd_t *to_i;
    Ddi_Bdd_t *toDisj = Ddi_BddMakePartDisjVoid(ddiMgr);

    if (ddiMgr->exist.conflictProduct != NULL) {
      myConflict = Ddi_BddMakePartDisjVoid(ddiMgr);
    }

    for (i = 0; i < np; i++) {
      Tr_Tr_t *trP = Tr_TrDup(TR);

      TrTrRelWrite(trP, Ddi_BddPartRead(trBdd, i));
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "Dpart PreImg. %d/%d\n", i, np));

      to_i = Trav_ApproxPreimg(trP, from, care, window, doApprox, opt);
      if (!Ddi_BddIsZero(to_i)) {
        Ddi_BddPartInsertLast(toDisj, to_i);
      }
      Ddi_Free(to_i);
      Tr_TrFree(trP);
      if (opt->clk && (ddiMgr->exist.conflictProduct != NULL)) {
        Ddi_Bdd_t *clk_lit;

        Pdtutil_Assert(i <= 1, "wrong clocked partition");
        clk_lit = Ddi_BddMakeLiteral(Ddi_VarFromName(ddiMgr, opt->clk), 1 - i);
        if (Ddi_BddIsOne(ddiMgr->exist.conflictProduct)) {
          Ddi_BddNotAcc(ddiMgr->exist.conflictProduct);
        }
        Ddi_BddSetPartConj(ddiMgr->exist.conflictProduct);
        Ddi_BddPartInsert(ddiMgr->exist.conflictProduct, 0, clk_lit);
        Ddi_Free(clk_lit);
        Ddi_BddPartInsertLast(myConflict, ddiMgr->exist.conflictProduct);
        Ddi_Free(ddiMgr->exist.conflictProduct);
        ddiMgr->exist.conflictProduct = Ddi_BddMakeConst(ddiMgr, 1);
      }
    }
    if (ddiMgr->exist.conflictProduct != NULL) {
      Ddi_Free(ddiMgr->exist.conflictProduct);
      if (Ddi_BddPartNum(myConflict) == 2) {
        ddiMgr->exist.conflictProduct = myConflict;
      } else {
        ddiMgr->exist.conflictProduct = Ddi_BddMakeConst(ddiMgr, 1);
      }
    }
    if (Ddi_BddPartNum(toDisj) == 1) {
      to_i = Ddi_BddPartExtract(toDisj, 0);
      Ddi_Free(toDisj);
      toDisj = to_i;
    } else if (Ddi_BddPartNum(toDisj) == 0) {
      Ddi_Free(toDisj);
      toDisj = Ddi_BddMakeConst(ddiMgr, 0);
    }

    if (!Ddi_BddIsZero(toDisj)) {
      Ddi_Free(ddiMgr->exist.conflictProduct);
      ddiMgr->exist.conflictProduct = NULL;
    }

    return (toDisj);
  }

  if (1 && opt->bdd.imgDpartTh > 0) {
    Ddi_MgrSetExistDisjPartTh(ddiMgr, opt->bdd.imgDpartTh /* *5 */ );
  }

  if (opt->bdd.imgDpartTh > 0 && opt->bdd.imgDpartTh < Ddi_BddSize(from)) {
    int i;
    Ddi_Bdd_t *to_i;
    Ddi_Bdd_t *toDisj = Ddi_BddMakePartDisjVoid(ddiMgr);
    int saveImgDpartTh = opt->bdd.imgDpartTh;

    myFrom = Part_PartitionDisjSet(from, NULL, NULL,
      Part_MethodEstimate_c, opt->bdd.imgDpartTh, 3,
      Pdtutil_VerbLevelDevMed_c);

    Ddi_BddSetFlattened(myFrom);
    Ddi_BddSetPartDisj(myFrom);

    if (ddiMgr->exist.conflictProduct != NULL) {
      myConflict = Ddi_BddMakePartDisjVoid(ddiMgr);
    }
    opt->bdd.imgDpartTh = -1;

    for (i = 0; i < Ddi_BddPartNum(myFrom); i++) {
      to_i = Trav_ApproxPreimg(TR, Ddi_BddPartRead(myFrom, i),
        care, window, doApprox, opt);
      if (!Ddi_BddIsZero(to_i)) {
        Ddi_BddPartInsertLast(toDisj, to_i);
      }
      Ddi_Free(to_i);
      if (opt->clk && (ddiMgr->exist.conflictProduct != NULL)) {
        Ddi_Bdd_t *clk_lit;

        Pdtutil_Assert(i <= 1, "wrong clocked partition");
        clk_lit = Ddi_BddMakeLiteral(Ddi_VarFromName(ddiMgr, opt->clk), 1 - i);
        if (Ddi_BddIsOne(ddiMgr->exist.conflictProduct)) {
          Ddi_BddNotAcc(ddiMgr->exist.conflictProduct);
        }
        Ddi_BddSetPartConj(ddiMgr->exist.conflictProduct);
        Ddi_BddPartInsert(ddiMgr->exist.conflictProduct, 0, clk_lit);
        Ddi_Free(clk_lit);
        Ddi_BddPartInsertLast(myConflict, ddiMgr->exist.conflictProduct);
        Ddi_Free(ddiMgr->exist.conflictProduct);
        ddiMgr->exist.conflictProduct = Ddi_BddMakeConst(ddiMgr, 1);
      }
    }
    opt->bdd.imgDpartTh = saveImgDpartTh;

    if (ddiMgr->exist.conflictProduct != NULL) {
      Ddi_Free(ddiMgr->exist.conflictProduct);
      if (Ddi_BddPartNum(myConflict) == 2) {
        ddiMgr->exist.conflictProduct = myConflict;
      } else {
        ddiMgr->exist.conflictProduct = Ddi_BddMakeConst(ddiMgr, 1);
      }
    }
    if (Ddi_BddPartNum(toDisj) == 1) {
      to_i = Ddi_BddPartExtract(toDisj, 0);
      Ddi_Free(toDisj);
      toDisj = to_i;
    } else if (Ddi_BddPartNum(toDisj) == 0) {
      Ddi_Free(toDisj);
      toDisj = Ddi_BddMakeConst(ddiMgr, 0);
    }

    if (!Ddi_BddIsZero(toDisj)) {
      Ddi_Free(ddiMgr->exist.conflictProduct);
      ddiMgr->exist.conflictProduct = NULL;
    }
    Ddi_Free(myFrom);
    return (toDisj);
  }

  ps = Ddi_VarsetMakeFromArray(psv);
  ns = Ddi_VarsetMakeFromArray(nsv);
  pi = Ddi_VarsetMakeFromArray(Tr_MgrReadI(trMgr));

  toPlus = NULL;
  myCare = Ddi_BddDup(care);

  abstract = NULL;
  if (doApprox) {
    strcpy(msg, "APPROX");
    /* myFrom = Tr_TrProjectOverPartitions(from,TR,NULL,0,opt->bdd.apprPreimgTh); */
    myFrom = Ddi_BddDup(from);
#if 0
    abstract = Ddi_BddSupp(myFrom);
    Ddi_VarsetRemoveAcc(abstract, Tr_TrClk(TR));
#endif
  } else {
    strcpy(msg, "Exact");
    myFrom = Ddi_BddDup(from);
    if ((opt->bdd.optPreimg > 0)
      && (Ddi_BddSize(from) > opt->bdd.optPreimg)) {
      toPlus = Trav_ApproxPreimg(TR, from, care, NULL, 1, opt);
      if (myCare != NULL) {
        Ddi_BddSetPartConj(myCare);
        Ddi_BddPartInsertLast(myCare, toPlus);
      }
    }
  }
#if 0
  if (Ddi_BddSize(myFrom) > 2000) {
    myFrom = Ddi_BddEvalFree(Part_PartitionDisjSet(myFrom, NULL, NULL,
        Part_MethodEstimateFast_c, 2000, Pdtutil_VerbLevelDevMed_c), myFrom);
  }
#endif

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMax_c,
    fprintf(stdout, "%s PreImg.\n", msg));

  Ddi_BddSwapVarsAcc(myFrom, psv, nsv);

  {
    int thresh;

    smoothV = Ddi_VarsetUnion(ns, pi);

    if (doApprox) {
      /*thresh = Ddi_MgrReadSiftThresh(ddiMgr); */
      /*thresh = 10*opt->bdd.apprPreimgTh; */
      thresh = -1;
#if 0
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
        fprintf(stdout, "Abstracting %d vars\n", Ddi_VarsetNum(abstract)));
      Ddi_VarsetUnionAcc(smoothV, abstract);
      Ddi_Free(abstract);
#endif
    } else {
      thresh = -1;
    }

    if (1 && myCare != NULL /*&& !Ddi_BddIsOne(myCare) */ ) {
      Ddi_Bdd_t *overAppr = NULL;
      Ddi_Bdd_t *prod = Ddi_BddDup(trBdd);
      Ddi_Bdd_t *prod1 = Ddi_BddMakePartConjVoid(ddiMgr);
      Ddi_Varset_t *suppFrom = Ddi_BddSupp(myFrom);
      int i;

      if (window != NULL) {
        Ddi_BddSuppAttach(window);
        Ddi_VarsetUnionAcc(suppFrom, Ddi_BddSuppRead(window));
        Ddi_BddSuppDetach(window);
      }
      Ddi_BddSetPartConj(prod);

      if (opt->clk != 0) {
        clk_part = Ddi_BddPartExtract(prod, 0);
      }

      for (i = 0; i < Ddi_BddPartNum(prod); i++) {
        Ddi_Bdd_t *p = Ddi_BddPartRead(prod, i);
        Ddi_Varset_t *suppP = Ddi_BddSupp(p);
        Ddi_Varset_t *supp = Ddi_BddSupp(p);
        Ddi_Varset_t *suppProd;
        int auxVarsUsed = Tr_MgrReadAuxVars(trMgr) != NULL;

        Ddi_VarsetIntersectAcc(supp, suppFrom);
        if (!auxVarsUsed && Ddi_VarsetIsVoid(supp)) {
          mySmooth = Ddi_VarsetIntersect(smoothV, suppP);
          suppProd = Ddi_BddSupp(prod);
          Ddi_VarsetDiffAcc(mySmooth, suppProd);
          p = Ddi_BddPartExtract(prod, i);
          if (!Ddi_VarsetIsVoid(mySmooth)) {
            Ddi_Free(suppProd);
            suppProd = Ddi_BddSupp(prod1);
            Ddi_VarsetDiffAcc(mySmooth, suppProd);
            if (!Ddi_VarsetIsVoid(mySmooth)) {
              Ddi_BddExistAcc(p, mySmooth);
            }
          }
          if (!Ddi_BddIsOne(p)) {
	    // @@@@ GC: check for bwd... 
            // Ddi_BddPartInsertLast(prod1,p);
          }
          Ddi_Free(p);
          i--;
          Ddi_Free(suppProd);
          Ddi_Free(mySmooth);
        }
        Ddi_Free(supp);
        Ddi_Free(suppP);
      }
      Ddi_Free(suppFrom);
      if (0 && opt->bdd.useMeta) {
        Ddi_Bdd_t *fmeta = Ddi_BddDup(myFrom);

#if 0
        Ddi_MetaInit(ddiMgr, Ddi_Meta_Size,
          trBdd, 0, psv, nsv, opt->bdd.useMeta);
        opt->bdd.useMeta = 1;
#endif

        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
          fprintf(stdout, "\nPart=%d,", Ddi_BddSize(myFrom)));
        Ddi_BddSetMono(myFrom);
        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
          fprintf(stdout, "Mono=%d,", Ddi_BddSize(myFrom)));
        Ddi_BddSetMeta(myFrom);
        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
          fprintf(stdout, "Meta=%d\n", Ddi_BddSize(myFrom)));
        Ddi_Free(fmeta);
      }

      Ddi_BddRestrictAcc(prod, myCare);
      Ddi_BddRestrictAcc(prod1, myCare);
      Ddi_BddRestrictAcc(myFrom, myCare);

      if (opt->tr.trPreimgSort) {
        int verbosityLocal = Tr_MgrReadVerbosity(trMgr);
        double saveW5 = Tr_MgrReadSortW(trMgr, 5);

        Ddi_BddSetFlattened(prod);
        Ddi_BddSetPartConj(prod);
        Tr_MgrSetSortW(trMgr, 5, 100.0);

        Tr_MgrSetVerbosity(trMgr, Pdtutil_VerbLevelDevMin_c);
        Tr_TrBddSortIwls95(trMgr, prod, smoothV, ps);
        Tr_MgrSetVerbosity(trMgr, (Pdtutil_VerbLevel_e) verbosityLocal);
        Tr_MgrSetSortW(trMgr, 5, saveW5);
      }

      if (1 && opt->bdd.useMeta) {
        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
          fprintf(stdout, "\nPart=%d,", Ddi_BddSize(myFrom)));
        Ddi_BddSetMeta(myFrom);
        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
          fprintf(stdout, "\nMeta=%d,", Ddi_BddSize(myFrom)));
      }

      Ddi_BddPartInsert(prod, 0, myFrom);

      if (1 && doApprox) {
#if 0
        myCare = Ddi_MgrReadOne(ddiMgr);
#endif
        overAppr = myFrom;
      }

      if (0 /*doApprox */ ) {
        Ddi_Varset_t *smoothPlus = Ddi_VarsetVoid(ddiMgr);
        Ddi_Bdd_t *lit;
        int j;

        for (i = j = 0, Ddi_VarsetWalkStart(ps);
          !Ddi_VarsetWalkEnd(ps); Ddi_VarsetWalkStep(ps), i++) {
          if (i == 166 /*Ddi_VarsetNum(ps)*10/11 */ ) {
            Ddi_VarsetAddAcc(smoothPlus, Ddi_VarsetWalkCurr(ps));
            if (j == 0) {
              lit = Ddi_BddMakeLiteral(Ddi_VarsetWalkCurr(ps), 0);
              Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
                fprintf(stdout, "\nS+[%d]:%s\n", i,
                  Ddi_VarName(Ddi_VarsetWalkCurr(ps))));
            }
            j++;
          }
        }
        for (i = 0; i < Ddi_BddPartNum(prod); i++) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(prod, i);

          Ddi_BddConstrainAcc(p, lit);
        }
        Ddi_Free(smoothPlus);
        Ddi_Free(lit);
      }


      ddiMgr->exist.clipLevel = opt->ints.saveClipLevel;

      if (1 || ddiMgr->exist.clipLevel < 2) {
        if (window != NULL) {
          ddiMgr->exist.activeWindow = Ddi_BddDup(window);
          if (!Ddi_BddIsOne(window)) {
            Ddi_BddRestrictAcc(prod, window);
            Ddi_BddPartInsert(prod, 0, window);
          }
        } else {
          ddiMgr->exist.activeWindow = NULL;
        }
        if (clk_part != NULL) {
          Ddi_BddPartInsert(prod, 0, clk_part);
          Ddi_Free(clk_part);
        }
        if (Tr_MgrReadCheckFrozenVars(trMgr)) {
          to = Ddi_BddMultiwayRecursiveAndExistOverWithFrozenCheck(prod,
            smoothV, myFrom, psv, nsv, myCare, overAppr, -1, 0);
        } else {
#if 0
          to = Ddi_BddMultiwayRecursiveAndExistOver(prod, ns,
            myCare, overAppr, thresh);
          Ddi_BddExistAcc(to, smoothV);
#else
          to = Ddi_BddMultiwayRecursiveAndExistOver(prod, smoothV,
            myCare, overAppr, thresh);
#endif
        }
        Ddi_Free(ddiMgr->exist.activeWindow);
      } else {
        int again = 0;
        int myClipTh = ddiMgr->exist.clipThresh;

        if (clk_part != NULL) {
          Ddi_BddPartInsert(prod, 0, clk_part);
          Ddi_Free(clk_part);
        }
        do {
          to = Ddi_BddMultiwayRecursiveAndExistOver(prod, smoothV,
            myCare, overAppr, thresh);
          if (Ddi_BddIsZero(to) && ddiMgr->exist.clipDone) {
            ddiMgr->exist.clipDone = 0;
            ddiMgr->exist.clipThresh *= 1.5;
            Ddi_Free(ddiMgr->exist.clipWindow);
            again = 1;
            Ddi_Free(to);
          }
        } while (again);
        ddiMgr->exist.clipThresh = myClipTh;
      }

      if (1 && opt->bdd.useMeta) {
        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
          fprintf(stdout, "\nMeta=%d,", Ddi_BddSize(to)));
        Ddi_BddSetPartConj(to);
        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
          fprintf(stdout, "Part=%d\n", Ddi_BddSize(to)));
      }

      Ddi_Free(prod);
      if (Ddi_BddIsPartConj(to)) {
        int i;

        for (i = 0; i < Ddi_BddPartNum(to); i++) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(to, i);

          Ddi_BddExistAcc(p, smoothV);
        }
      }
      Ddi_BddSetPartConj(to);
      for (i = 0; i < Ddi_BddPartNum(prod1); i++) {
        Ddi_Bdd_t *p = Ddi_BddPartRead(prod1, i);

        Ddi_BddPartInsertLast(to, p);
      }
      Ddi_Free(prod1);

      ddiMgr->exist.clipLevel = 0;
      Ddi_Free(clk_part);
    } else if (1) {
      Ddi_Bdd_t *prod = Ddi_BddDup(trBdd);

      Ddi_BddPartInsert(prod, 0, myFrom);
      to = Ddi_BddMultiwayRecursiveAndExistOver(prod, smoothV,
        myCare, NULL, -1);
      Ddi_Free(prod);
    } else {
      to = Ddi_BddAndExist(trBdd, myFrom, smoothV);
    }
    Ddi_Free(smoothV);
    Ddi_BddSetClustered(to, opt->tr.imgClustTh);
    Ddi_BddSetFlattened(to);
    Pdtutil_Assert(!(Ddi_BddIsPartConj(to) && (Ddi_BddPartNum(to) == 1)),
      "Invalid one partition part conj");
    Ddi_BddRestrictAcc(to, myCare);

    if (0 && doApprox) {
      to = Ddi_BddEvalFree(Tr_TrProjectOverPartitions(to, TR, myCare, 0,
          opt->bdd.apprPreimgTh), to);
    }
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "\n%d\n", Ddi_BddSize(to)));
  }

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "%s PreImg Done.\n", msg));

  Ddi_Free(myFrom);
  Ddi_Free(ps);
  Ddi_Free(ns);
  Ddi_Free(pi);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelDevMin_c,
    LOG_CONST(to, "APPR-to")
    );

#if 0
  if ((doApprox > 1) && (Ddi_BddSize(to) < 100)) {
    Ddi_BddSetMono(to);
    if (Ddi_BddIsOne(to)) {
      Ddi_Free(to);
      to = Trav_ApproxPreimg(TR, from, myCare, NULL, 0, opt);
    }
  }
#endif

  Ddi_Free(myCare);

  if (toPlus != NULL) {
    Ddi_BddSetPartConj(to);
    Ddi_BddPartInsertLast(to, toPlus);
    Ddi_Free(toPlus);
  }

  if (opt->bdd.univQuantifyTh > 0 && Ddi_BddSize(to) > opt->bdd.univQuantifyTh) {
    int i;

#if 0
    Ddi_Varset_t *supp = Ddi_BddSupp(to);
#else
    Ddi_Varset_t *supp = Ddi_VarsetDup(opt->ints.univQuantifyVars);
#endif
    Ddi_Vararray_t *q = Ddi_VararrayMakeFromVarset(supp, 1);
    int th = Ddi_BddSize(to) / 4;

    Ddi_Free(supp);
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "Forall %d=", Ddi_BddSize(to)));
    for (i = Ddi_VararrayNum(q) - 1; i >= 0; i--) {
      Ddi_Bdd_t *newTo;

      supp = Ddi_VarsetMakeFromVar(Ddi_VararrayRead(q, i));
      newTo = Ddi_BddForall(to, supp);
      Ddi_Free(supp);
      if (!Ddi_BddIsZero(newTo) && (Ddi_BddSize(newTo) < Ddi_BddSize(to))) {
        Ddi_Free(to);
        to = newTo;
        printf("%d,", Ddi_BddSize(to));
        fflush(stdout);
        opt->ints.univQuantifyDone = 1;
        if (Ddi_BddSize(to) < th) {
#if 0
          break;
#endif
        }
      } else {
        if (Ddi_BddIsZero(newTo)) {
          Ddi_Free(newTo);
          break;
        } else {
          Ddi_Free(newTo);
        }
      }
    }
    printf("\n");
    Ddi_Free(q);
  }

  return (to);
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static Ddi_Bdd_t *
PartitionTrav(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * care
)
{
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Ddi_Vararray_t *psv = Tr_MgrReadPS(trMgr);
  Ddi_Vararray_t *nsv = Tr_MgrReadNS(trMgr);
  Ddi_Bdd_t *careNS, *to, *reached, *from;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(init);
  Pdtutil_VerbLevel_e ddiVerb = Ddi_MgrReadVerbosity(ddiMgr);
  Pdtutil_VerbLevel_e trVerb = Tr_MgrReadVerbosity(trMgr);

  int step, toIncluded, travContinue = 1;

  Ddi_MgrSetOption(ddiMgr, Pdt_DdiVerbosity_c, inum,
    (Pdtutil_VerbLevel_e) ((int)ddiVerb - 1));
  Tr_MgrSetOption(trMgr, Pdt_TrVerbosity_c, inum,
    (Pdtutil_VerbLevel_e) ((int)trVerb - 1));

  Pdtutil_Assert(ddiMgr == travMgr->ddiMgrR,
    "Dual BDD manager nor supported by approx trav");

  from = Ddi_BddDup(init);
  reached = Ddi_BddDup(init);

  if (travMgr->care != NULL) {
    Ddi_BddRestrictAcc(from, care);
    Ddi_BddRestrictAcc(reached, care);
  }

  step = 0;

  while (travContinue) {

    if (care != NULL) {
      careNS = Ddi_BddSwapVars(care, psv, nsv);
      Ddi_BddNotAcc(careNS);
    } else {
      careNS = Ddi_BddMakeConst(ddiMgr, 1);
    }

    to = Tr_ImgWithConstrain(tr, from, careNS);
    Ddi_BddSetMono(to);

    Ddi_Free(careNS);

    /*
     *  Check Fixt Point
     */

    if (Ddi_BddIncluded(to, reached)) {
      toIncluded = 1;
    } else {
      toIncluded = 0;
    }

    Ddi_Free(from);
    from = TravFromCompute(to, reached, Trav_MgrReadFromSelect(travMgr));

    if (travMgr->care != NULL) {
      Ddi_BddRestrictAcc(from, care);
      Ddi_BddRestrictAcc(to, care);
    }

    /* Update the Set of Reached States */
    Ddi_BddOrAcc(reached, to);

    if (travMgr->care != NULL) {
      Ddi_BddRestrictAcc(reached, care);
    }

    /*
     *  Free the to set
     */

    Ddi_Free(to);

    /*
     *  Check End Of Cycle: Fix Point for Closure, etc.
     */

    travContinue = (!toIncluded);

    step++;

  }

  Ddi_Free(from);

  Ddi_MgrSetOption(ddiMgr, Pdt_DdiVerbosity_c, inum, ddiVerb);
  Tr_MgrSetOption(trMgr, Pdt_TrVerbosity_c, inum, trVerb);

  return (reached);

}

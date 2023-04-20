/**CFile***********************************************************************

  FileName    [fbvMc.c]

  PackageName [fbv]

  Synopsis    [Model check routines]

  Description []

  SeeAlso   []

  Author    [Gianpiero Cabodi and Stefano Quer]

  Copyright [This file was created at the Politecnico di Torino,
    Torino, Italy.
    The  Politecnico di Torino makes no warranty about the suitability of
    this software for any purpose.
    It is presented on an AS IS basis.
  ]

  Revision  []

******************************************************************************/

#include "fbvInt.h"
#include "ddiInt.h"
#include "mcInt.h"
#include "trav.h"
#include "expr.h"

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

#define ENABLE_CONFLICT 0

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static Ddi_Bdd_t *BckInvarCheckRecurStep(
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * target,
  Ddi_Bdd_t * rings,
  Ddi_Bdd_t * Rplus,
  Ddi_Bdd_t * careRings,
  Tr_Tr_t * TR,
  int level,
  int nestedFull,
  int nestedPart,
  int innerFail,
  int nestedCalls,
  int noPartition,
  Fbv_Globals_t * opt
);
static void analyzeConflict(
  Tr_Tr_t * TR,
  Ddi_Bdd_t * inner,
  Ddi_Bdd_t * same,
  Ddi_Bdd_t * outer,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * from,
  Fbv_Globals_t * opt
);
static void analyzeConflictOld(
  Tr_Tr_t * TR,
  Ddi_Bdd_t * Rplus,
  Fbv_Globals_t * opt
);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of external functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Fbv_RunBddMc(
  Mc_Mgr_t * mcMgr
)
{
  Tr_Tr_t *tr = NULL;
  Fbv_Globals_t *opt;
  Ddi_Bdd_t *init, *invarspec, *invar, *care;
  Ddi_Bddarray_t *delta = NULL, *deltaAig = NULL;;
  int verificationOK, verificationResult = -1;

  opt = FbvDupSettings((Fbv_Globals_t *) mcMgr->auxPtr);

  invarspec = Fsm_MgrReadInvarspecBDD(mcMgr->fsmMgr);

  Pdtutil_Assert(opt != NULL, "missing fbv options in mc mgr");

  mcMgr->settings.bddTimeLimit = 900;
  mcMgr->settings.bddNodeLimit = 9000000;

  Fsm_MgrSetOption(mcMgr->fsmMgr, Pdt_FsmCutThresh_c, inum, opt->fsm.cut);
  if (mcMgr->settings.method == Mc_VerifExactBck_c) {
    Fsm_MgrSetOption(mcMgr->fsmMgr, Pdt_FsmCutThresh_c, inum, -1);
  }

  //  Disabled: was introduced for hints
  //  deltaAig = Ddi_BddarrayDup(Fsm_MgrReadDeltaBDD(mcMgr->fsmMgr));
  if (Fsm_MgrAigToBdd(mcMgr->fsmMgr) == 0) {
    /* unable to generate BDDs */
    verificationOK = -1;
    Ddi_Free(deltaAig);
    return verificationOK;
  }

  FbvSetTrMgrOpt(mcMgr->trMgr, mcMgr->fsmMgr, opt, 0);

  invarspec = Fsm_MgrReadInvarspecBDD(mcMgr->fsmMgr);
  init = Fsm_MgrReadInitBDD(mcMgr->fsmMgr);
  invar = Fsm_MgrReadConstraintBDD(mcMgr->fsmMgr);
  care = Fsm_MgrReadCareBDD(mcMgr->fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(mcMgr->fsmMgr);

  tr = Tr_TrMakePartConjFromFunsWithAuxVars(mcMgr->trMgr,
    Fsm_MgrReadDeltaBDD(mcMgr->fsmMgr),
    Fsm_MgrReadVarNS(mcMgr->fsmMgr),
    Fsm_MgrReadAuxVarBDD(mcMgr->fsmMgr), 
      Fsm_MgrReadVarAuxVar(mcMgr->fsmMgr));

  Tr_TrCoiReduction(tr, invarspec);
  if (1) {
    Ddi_Vararray_t *ps = Tr_MgrReadPS(Tr_TrMgr(tr));
    Ddi_Vararray_t *ns = Tr_MgrReadNS(Tr_TrMgr(tr));
    Ddi_Varset_t *nsvars = Ddi_VarsetMakeFromArray(ns);
    Ddi_Varset_t *coiSupp = Ddi_BddSupp(Tr_TrBdd(tr));

    Ddi_VarsetIntersectAcc(coiSupp, nsvars);
    Ddi_VarsetSwapVarsAcc(coiSupp, ps, ns);

    Ddi_BddExistProjectAcc(init, coiSupp);
    Ddi_Free(nsvars);
    Ddi_Free(coiSupp);
  }

  FbvFsmCheckComposePi(mcMgr->fsmMgr, tr);
  if (opt->ddi.siftTravTh > 0) {
    Ddi_MgrSetSiftThresh(mcMgr->ddiMgr, opt->ddi.siftTravTh);
  }
  if (opt->ddi.siftMaxTh > 0) {
    Ddi_MgrSetSiftMaxThresh(mcMgr->ddiMgr, opt->ddi.siftMaxTh);
  }
  if (mcMgr->settings.method == Mc_VerifExactBck_c) {
    opt->trav.rPlus = Pdtutil_StrDup("1");
    if (TRAV_BDD_VERIF) {
      Pdtutil_Assert(mcMgr->travMgr != NULL, "NULL trav manager!");
      Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravRPlus_c, pchar,
        opt->trav.rPlus);
      verificationOK =
        Trav_TravBddApproxForwExactBckFixPointVerify(mcMgr->travMgr,
        mcMgr->fsmMgr, tr, init, invarspec, invar, care, delta);
    } else {
      verificationOK = FbvApproxForwExactBckFixPointVerify(mcMgr->fsmMgr, tr,
        init, invarspec, invar, care, delta, opt);
    }
  } else if (mcMgr->settings.method == Mc_VerifApproxFwdExactBck_c) {
    if (TRAV_BDD_VERIF) {
      Pdtutil_Assert(mcMgr->travMgr != NULL, "NULL trav manager!");
      verificationOK =
        Trav_TravBddApproxForwExactBckFixPointVerify(mcMgr->travMgr,
        mcMgr->fsmMgr, tr, init, invarspec, invar, care, delta);
    } else {
      verificationOK = FbvApproxForwExactBckFixPointVerify(mcMgr->fsmMgr,
        tr, init, invarspec, invar, care, delta, opt);
    }
  } else {
    opt->trav.cex = 1;

    // opt->trav.clustTh = 1200;
    if (TRAV_BDD_VERIF) {
      Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravBddTimeLimit_c, inum,
        mcMgr->settings.bddTimeLimit);
      Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravBddNodeLimit_c, inum,
        mcMgr->settings.bddNodeLimit);
      Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravDoCex_c, inum, 1);
      verificationOK =
        Trav_TravBddExactForwVerify(mcMgr->travMgr, mcMgr->fsmMgr, tr, init,
        invarspec, invar, delta);
    } else {
      verificationOK = FbvExactForwVerify(mcMgr->travMgr, mcMgr->fsmMgr,
        tr, init, invarspec, invar, delta, mcMgr->settings.bddTimeLimit,
        mcMgr->settings.bddNodeLimit, opt);
    }
  }
  Ddi_Free(deltaAig);
  Tr_TrFree(tr);
  Tr_MgrQuit(mcMgr->trMgr);
  FbvFreeSettings(opt);
  mcMgr->trMgr = NULL;

  return verificationOK;
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
FbvBckInvarCheckRecur(
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * target,
  Tr_Tr_t * TR,
  Ddi_Bdd_t * rings,
  Fbv_Globals_t * opt
)
{
  Ddi_Bdd_t *trace, *tmp, *Rplus;
  Ddi_Bdd_t *careRings;
  Ddi_Bdd_t *care;
  int level = 0;

  opt->ddi.saveClipLevel = 2;

  careRings = Ddi_BddDup(rings);
  care = Ddi_BddPartRead(careRings, Ddi_BddPartNum(careRings) - 1);
  Rplus = Ddi_BddDup(Ddi_BddPartRead(rings, Ddi_BddPartNum(rings) - 1));
  Ddi_BddRestrictAcc(Rplus, care);

  while ((level < Ddi_BddPartNum(rings)) && (!FbvCheckSetIntersect(from,
        Ddi_BddPartRead(rings, level), Rplus, NULL, 0, opt))) {
    level++;
  }
  if (level >= Ddi_BddPartNum(rings)) {
    trace = NULL;
  } else {
    if (level < Ddi_BddPartNum(rings) - 1)
      level++;
    trace = BckInvarCheckRecurStep(from, target, rings, Rplus, careRings, TR,
      level, 0 /*nested full */ , 0 /*nested part */ , 0 /*innerFail */ ,
      0 /*nestedCalls */ , 0 /*OK part */ , opt);
  }

  Ddi_Free(careRings);

  if (trace != NULL) {
    /* revert order */
    tmp = trace;
    trace = Ddi_BddPartExtract(tmp, Ddi_BddPartNum(tmp) - 1);
    Ddi_BddSetPartDisj(trace);
    while (Ddi_BddPartNum(tmp) > 0) {
      Ddi_Bdd_t *p = Ddi_BddPartExtract(tmp, Ddi_BddPartNum(tmp) - 1);

      Ddi_BddPartInsertLast(trace, p);
      Ddi_Free(p);
    }
    Ddi_Free(tmp);
  }

  Ddi_Free(Rplus);

  return (trace);
}

/**Function*******************************************************************
  Synopsis    [Check for set intersection.]
  Description [Check for set intersection. Partitioned Bdds and input care
               are supported.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
FbvCheckSetIntersect(
  Ddi_Bdd_t * set,
  Ddi_Bdd_t * target,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * care2,
  int doAnd,
  Fbv_Globals_t * opt
)
{
  int intersect = 0, again, myClipTh, myClipLevel;
  Ddi_Bdd_t *targetAux = Ddi_BddDup(target);
  Ddi_Bdd_t *setAux = Ddi_BddDup(set), *myClipWindow;
  Ddi_Varset_t *s0;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(set);
  Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity(ddm);

  Ddi_MgrSetOption(ddm, Pdt_DdiVerbosity_c, inum, Pdtutil_VerbLevelUsrMin_c);

  if (doAnd) {
    Ddi_BddAndAcc(targetAux, set);
    if (care != NULL) {
      Ddi_BddAndAcc(targetAux, care);
    }
    if (care2 != NULL) {
      Ddi_BddAndAcc(targetAux, care2);
    }
    Ddi_BddSetMono(targetAux);
  } else {
    Ddi_BddRestrictAcc(setAux, targetAux);
    Ddi_BddRestrictAcc(targetAux, setAux);
    Ddi_BddSetPartConj(targetAux);
    if (Ddi_BddIsPartConj(setAux)) {
      int i, j;

      for (i = 0; i < Ddi_BddPartNum(setAux); i++) {
        j = i * 2;
        if (j >= Ddi_BddPartNum(targetAux)) {
          j = Ddi_BddPartNum(targetAux);
        }
        Ddi_BddPartInsert(targetAux, j, Ddi_BddPartRead(set, i));
      }
    } else {
      Ddi_BddPartInsert(targetAux, 0, setAux);
    }

    if (care != NULL && !Ddi_BddIsOne(care)) {
      Ddi_Bdd_t *careAux = Ddi_BddDup(care);

      Ddi_BddRestrictAcc(targetAux, careAux);
      Ddi_BddRestrictAcc(careAux, targetAux);
      if (Ddi_BddIsZero(careAux)) {
        Ddi_Free(targetAux);
        targetAux = Ddi_BddMakeConst(ddm, 0);
      } else {
        Ddi_BddPartInsert(targetAux, 0, careAux);
      }
      Ddi_Free(careAux);
    }

    if (!Ddi_BddIsZero(targetAux)) {

      if (care2 != NULL && !Ddi_BddIsOne(care2)) {
        Ddi_BddRestrictAcc(targetAux, care2);
        Ddi_BddPartInsert(targetAux, 0, care2);
      }

      s0 = Ddi_BddSupp(targetAux);

      Ddi_BddSetFlattened(targetAux);
      Ddi_BddSetClustered(targetAux, opt->trav.imgClustTh);

      Pdtutil_Assert(ddm->exist.clipDone == 0, "Wrong clipDone setting");
      Pdtutil_Assert(ddm->exist.clipWindow == NULL,
        "Wrong clipWindow setting");

      myClipWindow = Ddi_BddMakeConst(ddm, 1);
      myClipTh = ddm->exist.clipThresh;
      myClipLevel = ddm->exist.clipLevel;

      ddm->exist.clipLevel = 2;

      do {
        Ddi_Bdd_t *newTargetAux;

        ddm->exist.clipDone = again = 0;

        Ddi_BddSetPartConj(targetAux);
        newTargetAux =
          Ddi_BddMultiwayRecursiveAndExist(targetAux, s0, myClipWindow, -1);
        Ddi_BddSetMono(newTargetAux);

        again = ddm->exist.clipDone &&
          (ddm->exist.clipWindow != NULL) && Ddi_BddIsZero(newTargetAux);

        if (again) {
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
              Pdtutil_VerbLevelNone_c, fprintf(stdout, ",")
            );
            );
          Ddi_BddSetMono(ddm->exist.clipWindow);

          Ddi_BddNotAcc(ddm->exist.clipWindow);
          Ddi_BddAndAcc(myClipWindow, ddm->exist.clipWindow);
          Ddi_Free(newTargetAux);
        } else {
          Ddi_Free(targetAux);
          targetAux = newTargetAux;
        }

        Ddi_Free(ddm->exist.clipWindow);
        ddm->exist.clipWindow = NULL;
        ddm->exist.clipThresh *= 1.5;

      } while (again);

      ddm->exist.clipLevel = myClipLevel;
      ddm->exist.clipThresh = myClipTh;
      Ddi_Free(myClipWindow);

      Ddi_Free(s0);

    }

  }

  intersect = !Ddi_BddIsZero(targetAux);

  Ddi_Free(setAux);
  Ddi_Free(targetAux);

  Ddi_MgrSetOption(ddm, Pdt_DdiVerbosity_c, inum, verbosity);

  return (intersect);
}

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
BckInvarCheckRecurStep(
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * target,
  Ddi_Bdd_t * rings,
  Ddi_Bdd_t * Rplus,
  Ddi_Bdd_t * careRings,
  Tr_Tr_t * TR,
  int level,
  int nestedFull,
  int nestedPart,
  int innerFail,
  int nestedCalls,
  int noPartition,
  Fbv_Globals_t * opt
)
{
  Ddi_Bdd_t *trace, *to, *myFrom, *myFrom2, *myFrom3,
    *outer, *inner, *myOuter, *care, *careTot, *outerCare, *tmp, *myClipWindow;
  int nRings = Ddi_BddPartNum(rings);
  int localPart, sameLevel;
  int travAgain, countClip, myClipTh;
  Tr_Mgr_t *trMgr = Tr_TrMgr(TR);
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(from);

  static int callId = 0;
  int id = callId++;

  if (noPartition) {
    care = Ddi_BddPartRead(careRings, level);
  } else {
    care = Ddi_BddPartRead(careRings, Ddi_BddPartNum(careRings) - 1);
  }
  care = Ddi_BddPartRead(careRings, Ddi_BddPartNum(careRings) - 1);
  careTot = Ddi_BddPartRead(careRings, Ddi_BddPartNum(careRings) - 1);


  /* check termination */
  if (FbvCheckSetIntersect(from, target, Rplus, care, 1, opt)) {
    trace = Ddi_BddDup(target);
    Ddi_BddSetPartDisj(trace);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "TARGET REACHED.\n")
      );
    return (trace);
  }
  if (level <= 0) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "DEAD END.\n")
      );
    return (NULL);
  }

  Pdtutil_Assert(level > 0,
    "Innermost level reached in recursive Invar Check");

  Ddi_BddSetClustered(from, opt->trav.imgClustTh);
  Ddi_BddRestrictAcc(from, care);

  fprintf(stdout,
    "rec step %d (|f|=%d,|R+|=%d) (id:%d/if:%d/nc:%d/np:%d/nf:%d).\n", level,
    Ddi_BddSize(from), Ddi_BddSize(Rplus), id, innerFail, nestedCalls,
    nestedPart, nestedFull);
  if (level < opt->trav.nExactIter || (level == opt->trav.nExactIter
      && opt->trav.nExactIter < nRings - 1)) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "EXACT region reached.\n")
      );
  }

  /* generate local from as from intersected with level ring */
  if (level < Ddi_BddPartNum(careRings) - 1) {
    sameLevel = level;
  } else {
    sameLevel = level;
  }

  outer = Ddi_BddPartRead(rings, sameLevel);
  myOuter = Ddi_BddDup(Ddi_BddPartRead(rings, sameLevel));
  outerCare = Ddi_BddPartRead(careRings, sameLevel);
  inner = Ddi_BddDup(Ddi_BddPartRead(rings, level - 1));


  /* exclude myFrom from Rplus */
#if 1
  /* @ $$ @ */
  Ddi_BddSetPartConj(Rplus);
  tmp = Ddi_BddDup(from);
  Ddi_BddSetClustered(tmp, opt->trav.imgClustTh);
  Ddi_BddNotAcc(tmp);
  Ddi_BddRestrictAcc(tmp, care);
  Ddi_BddRestrictAcc(Rplus, tmp);
  Ddi_BddPartInsertLast(Rplus, tmp);
  Ddi_BddSetFlattened(Rplus);
  Ddi_BddSetClustered(Rplus, opt->trav.imgClustTh);
  Ddi_BddRestrictAcc(Rplus, care);
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "|R+|=%d.\n", Ddi_BddSize(Rplus))
    );
  /*Ddi_BddPartInsertLast(inner,tmp); */
  Ddi_Free(tmp);
#endif


#if 1
  if ((level < opt->trav.nExactIter)
    || ((Ddi_BddSize(from) > opt->trav.pmcPrioThresh)
      && (nestedFull < opt->trav.pmcMaxNestedFull)
      && (nestedPart < opt->trav.pmcMaxNestedPart))) {

    myFrom = Ddi_BddDup(from);
    Ddi_BddRestrictAcc(myFrom, care);
    Ddi_BddRestrictAcc(myFrom, outerCare);


#if 0
    Ddi_BddSetPartConj(inner);
    tmp = Ddi_BddRestrict(Rplus, inner);
    Ddi_BddPartInsertLast(inner, tmp);
    Ddi_BddSetFlattened(inner);
    Ddi_BddSetClustered(inner, opt->trav.imgClustTh);
    Ddi_Free(tmp);
#endif

#if 0
    Ddi_BddSetPartConj(outer);
    tmp = Ddi_BddDup(from);
    Ddi_BddSetClustered(tmp, opt->trav.imgClustTh);
    Ddi_BddNotAcc(tmp);

    Ddi_BddPartInsertLast(outer, tmp);
    Ddi_Free(tmp);
    Ddi_BddSetClustered(outer, opt->trav.imgClustTh);
    Ddi_BddRestrictAcc(outer, care);
#endif

    fprintf(stdout, "L: %d (id: %d) -> inner (|from| = %d).\n", level, id,
      Ddi_BddSize(myFrom));
    fprintf(stdout, "Live nodes (peak): %u(%u).\n",
      Ddi_MgrReadLiveNodes(Ddi_ReadMgr(from)),
      Ddi_MgrReadPeakLiveNodeCount(Ddi_ReadMgr(from)));

    myClipWindow = Ddi_BddMakeConst(Ddi_ReadMgr(from), 1);
    myClipTh = ddm->exist.clipThresh;

    countClip = 0;
    do {
      Ddi_Bdd_t *localFrom;

      ddm->exist.clipDone = travAgain = 0;
#if ENABLE_CONFLICT
      conflictProduct = Ddi_BddMakeConst(ddm, 1);
#endif
      /* try preimg to inner ring */
      to = FbvApproxPreimg(TR, myFrom, inner, myClipWindow, 0, opt);

      travAgain = ddm->exist.clipDone && (ddm->exist.clipWindow != NULL);

      localFrom = NULL;
      if (travAgain) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c, fprintf(stdout, "Clip INNER.\n")
          );
        Ddi_BddSetMono(ddm->exist.clipWindow);

        Ddi_BddNotAcc(ddm->exist.clipWindow);
        Ddi_BddAndAcc(myClipWindow, ddm->exist.clipWindow);
        Ddi_Free(ddm->exist.clipWindow);
        ddm->exist.clipWindow = NULL;
        ddm->exist.clipDone = 0;
        countClip++;
      } else if (Ddi_BddIsZero(to)) {
        localFrom = Ddi_BddDup(myFrom);
      }
#if ENABLE_CONFLICT
      if (sameLevel < Ddi_BddPartNum(careRings) - 1) {
        analyzeConflict(opt->auxFwdTr, inner, outer,
          Ddi_BddPartRead(rings, sameLevel + 1), care, localFrom, opt);
      } else {
        analyzeConflict(opt->auxFwdTr, inner, outer, care, care, localFrom,
          opt);
      }
#endif
      Ddi_Free(localFrom);


      if (!Ddi_BddIsZero(to)) {
        Ddi_BddSetPartConj(to);
        ddm->exist.clipThresh = myClipTh;

#if 0
        {
          int i;
          Ddi_Bdd_t *innerPlus;
          Ddi_Varset_t *smooth = Ddi_BddSupp(inner);
          Ddi_Varset_t *supp = Ddi_BddSupp(to);

          Ddi_VarsetDiffAcc(smooth, supp);
          innerPlus = Ddi_BddDup(inner);
          if (Ddi_BddIsPartDisj(innerPlus)) {
            for (i = 0; i < Ddi_BddPartNum(innerPlus); i++) {
              Ddi_Bdd_t *p = Ddi_BddPartRead(innerPlus, i);

              if (Ddi_BddIsPartConj(p)) {
                int j;

                for (j = 0; j < Ddi_BddPartNum(p); j++) {
                  Ddi_BddExistAcc(Ddi_BddPartRead(p, j), smooth);
                }
              } else {
                Ddi_BddExistAcc(p, smooth);
              }
            }
          } else if (Ddi_BddIsPartConj(innerPlus)) {
            for (i = 0; i < Ddi_BddPartNum(innerPlus); i++) {
              Ddi_BddExistAcc(Ddi_BddPartRead(innerPlus, i), smooth);
            }
          } else {
            Ddi_BddExistAcc(innerPlus, smooth);
          }
          fprintf(stdout, "[%d]|%d|=>|%d|.\n", Ddi_VarsetNum(smooth),
            Ddi_BddSize(inner), Ddi_BddSize(innerPlus));
          Ddi_Free(supp);
          Ddi_Free(smooth);

#if 0
          Ddi_BddRestrictAcc(to, innerPlus);
          Ddi_BddRestrictAcc(innerPlus, to);
          Ddi_BddPartInsertLast(to, innerPlus);
          Ddi_BddSetFlattened(to);
#endif
          Ddi_Free(innerPlus);
        }
#else
        {
          Ddi_Bdd_t *innerPlus = Ddi_BddDup(inner);

          Ddi_BddRestrictAcc(to, innerPlus);
          Ddi_BddRestrictAcc(innerPlus, to);
          Ddi_BddPartInsertLast(to, innerPlus);
          Ddi_BddSetFlattened(to);
          Ddi_Free(innerPlus);
        }
#endif


        trace = BckInvarCheckRecurStep(to, target, rings, Rplus, careRings, TR,
          level - 1, nestedFull, nestedPart + 1, innerFail,
          nestedCalls + 1, 0 /*no part */ , opt);
        Ddi_Free(to);
        if (trace != NULL) {
          Ddi_BddPartInsertLast(trace, myFrom);
          Ddi_Free(myFrom);
          Ddi_Free(myOuter);
          Ddi_Free(myClipWindow);
          Ddi_Free(inner);
          return (trace);
        }
      }
      Ddi_Free(to);
      ddm->exist.clipThresh *= 1.5;

    } while (travAgain && countClip < 3);

    ddm->exist.clipThresh = myClipTh;
    Ddi_Free(myClipWindow);
    Ddi_Free(myFrom);

    if (noPartition) {
      Ddi_Free(myOuter);
      Ddi_Free(inner);
      return (NULL);
    }

    innerFail++;

#endif

  }

  Ddi_Free(inner);


  myFrom = Ddi_BddDup(from);
  myFrom2 = Ddi_BddDup(from);


#if 0
  if ((Ddi_BddSize(from) > opt->trav.pmcPrioThresh) &&
    (nestedPart < opt->trav.pmcMaxNestedPart)) {
    /* take inner partition of myFrom */
    Ddi_BddSetPartConj(myFrom);
#if 1
    {
      int i;
      Ddi_Bdd_t *outerPlus;
      Ddi_Varset_t *smooth = Ddi_BddSupp(myOuter);
      Ddi_Varset_t *supp = Ddi_BddSupp(myFrom);

      Ddi_VarsetDiffAcc(smooth, supp);
      outerPlus = Ddi_BddDup(myOuter);
      if (Ddi_BddIsPartConj(outerPlus)) {
        for (i = 0; i < Ddi_BddPartNum(outerPlus); i++) {
          Ddi_BddExistAcc(Ddi_BddPartRead(outerPlus, i), smooth);
        }
      }
      fprintf(stdout, "(%d)|%d|=>|%d|.\n", Ddi_VarsetNum(smooth),
        Ddi_BddSize(myOuter), Ddi_BddSize(outerPlus));
      Ddi_Free(supp);
      Ddi_Free(smooth);
      Ddi_BddPartInsertLast(myFrom, outerPlus);
      Ddi_Free(outerPlus);
    }
#else
    Ddi_BddPartInsertLast(myFrom, myOuter);
#endif
    localPart = 1;

    tmp = Ddi_BddDup(myOuter);
    Ddi_BddSetClustered(tmp, opt->trav.imgClustTh);
    Ddi_BddNotAcc(tmp);
    Ddi_BddSetPartConj(myFrom2);
    Ddi_BddPartInsertLast(myFrom2, tmp);
    Ddi_Free(tmp);
    Ddi_BddSetClustered(myOuter, opt->trav.imgClustTh);
    Ddi_BddRestrictAcc(myOuter, care);

  } else {
    localPart = 0;
  }
#else
  localPart = 0;
#endif

  Ddi_Free(myOuter);


  Ddi_BddRestrictAcc(myFrom, care);
  Ddi_BddRestrictAcc(myFrom2, care);

  if ((!opt->trav.pmcNoSame) && (nestedFull < opt->trav.pmcMaxNestedFull) &&
    (Ddi_BddSize(from) > opt->trav.pmcPrioThresh) &&
    (level < Ddi_BddPartNum(careRings) - 1)) {

    fprintf(stdout, "L: %d (id: %d) -> same(%d) (|from| = %d).\n", level, id,
      localPart, Ddi_BddSize(myFrom));

    /* try same ring */

    myClipWindow = Ddi_BddMakeConst(Ddi_ReadMgr(from), 1);
    if (sameLevel < Ddi_BddPartNum(careRings) - 1) {
      sameLevel++;
    }
    myClipTh = ddm->exist.clipThresh;

    countClip = 0;
    do {

      ddm->exist.clipDone = travAgain = 0;

      myFrom3 = Ddi_BddDup(myFrom);
      myOuter = Ddi_BddDup(outer);
      Ddi_BddSetPartConj(myOuter);
      Ddi_BddPartInsertLast(myOuter, care);
#if 0
      Ddi_BddNotAcc(myFrom);
      Ddi_BddPartInsertLast(myOuter, myFrom);
      Ddi_BddNotAcc(myFrom);
#else
      /* @ $$ @ */
      Ddi_BddPartInsertLast(myOuter, Rplus);
      Ddi_BddSetFlattened(myOuter);
#endif

      if (sameLevel < Ddi_BddPartNum(careRings) - 1) {
        outerCare = Ddi_BddPartRead(careRings, sameLevel + 1);
        Ddi_BddRestrictAcc(myFrom3, care);
        Ddi_BddRestrictAcc(myFrom3, outerCare);
      }


      if (countClip > 0) {
        opt->trav.opt_preimg = 1000;
      }
      to = FbvApproxPreimg(TR, myFrom3, myOuter, myClipWindow, 0, opt);
      if (countClip > 0) {
        opt->trav.opt_preimg = 0;
      }
      Ddi_BddSetFlattened(to);

      travAgain = ddm->exist.clipDone && (ddm->exist.clipWindow != NULL);

      if (travAgain) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c, fprintf(stdout, "Clip SAME.\n")
          );
        Ddi_BddSetMono(ddm->exist.clipWindow);

        Ddi_BddNotAcc(ddm->exist.clipWindow);
        Ddi_BddAndAcc(myClipWindow, ddm->exist.clipWindow);
        Ddi_Free(ddm->exist.clipWindow);
        ddm->exist.clipWindow = NULL;
        ddm->exist.clipDone = 0;
        countClip++;
      }

      Ddi_Free(myOuter);
      Ddi_Free(myFrom3);

      if (!Ddi_BddIsZero(to)) {
        Ddi_BddSetPartConj(to);
        tmp = Ddi_BddRestrict(outer, to);
        Ddi_BddRestrictAcc(to, tmp);
        Ddi_BddPartInsertLast(to, outer);
        Ddi_BddSetFlattened(to);
        Ddi_Free(tmp);
        ddm->exist.clipThresh = myClipTh;

        trace = BckInvarCheckRecurStep(to, target, rings, Rplus, careRings, TR,
          sameLevel, nestedFull, nestedPart + localPart, innerFail,
          nestedCalls + 1, 0 /*no part */ , opt);
        Ddi_Free(to);
        if (trace != NULL) {
          Ddi_BddPartInsertLast(trace, myFrom);
          Ddi_Free(myFrom);
          Ddi_Free(myFrom2);
          Ddi_Free(myClipWindow);
          return (trace);
        }
      }

      Ddi_Free(to);
      ddm->exist.clipThresh *= 1.5;

    } while (travAgain && countClip < 2);

    ddm->exist.clipThresh = myClipTh;

    Ddi_Free(myClipWindow);

  }

  fprintf(stdout, "L: %d (id: %d) -> full(%d) (|from| = %d).\n", level, id,
    localPart, Ddi_BddSize(myFrom));
  fprintf(stdout, "Live nodes (peak): %u(%u).\n",
    Ddi_MgrReadLiveNodes(Ddi_ReadMgr(from)),
    Ddi_MgrReadPeakLiveNodeCount(Ddi_ReadMgr(from)));

  /* try preimg to full Rplus */

  myOuter = Ddi_BddDup(Rplus);
  Ddi_BddSetPartConj(myOuter);
  Ddi_BddPartInsertLast(myOuter, care);
  myClipWindow = Ddi_BddMakeConst(Ddi_ReadMgr(from), 1);
  myClipTh = ddm->exist.clipThresh;

  countClip = 0;
  do {
    Ddi_Bdd_t *localFrom;

    ddm->exist.clipDone = travAgain = 0;

    Pdtutil_Assert(ddm->exist.conflictProduct == NULL,
      "non NULL conflict product !");
#if ENABLE_CONFLICT
    ddm->exist.conflictProduct = Ddi_BddMakeConst(ddm, 1);
#endif

#if 1
    if (countClip > 0) {
      opt->trav.opt_preimg = 1000;
    }
    {
      Ddi_Varset_t *supp = Ddi_BddSupp(myFrom);
      int doApprox = (Ddi_BddSize(myFrom) > 1000 && Ddi_VarsetNum(supp) > 200);

      Ddi_Free(supp);
      to = FbvApproxPreimg(TR, myFrom, care, myClipWindow, doApprox, opt);
    }
    if (countClip > 0) {
      opt->trav.opt_preimg = 0;
    }
    Ddi_BddRestrictAcc(to, myOuter);
#else
    to = FbvApproxPreimg(TR, myFrom, myOuter, myClipWindow, 0, opt);
#endif


    travAgain = ddm->exist.clipDone && (ddm->exist.clipWindow != NULL);

    localFrom = NULL;
    if (travAgain) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "Clip FULL.\n")
        );
      Ddi_BddSetMono(ddm->exist.clipWindow);

      if (0 && Ddi_BddIsZero(to)) {
        Ddi_Bdd_t *auxTo;
        int auxClipLevel = opt->ddi.saveClipLevel;

        ddm->exist.clipLevel = opt->ddi.saveClipLevel = 0;
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c, fprintf(stdout, "Trying clipWindow.\n")
          );
        auxTo = FbvApproxPreimg(TR, Ddi_MgrReadOne(Ddi_ReadMgr(from)),
          myOuter, ddm->exist.clipWindow, 0, opt);
        if (Ddi_BddIsZero(auxTo)) {
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c, fprintf(stdout, "Found Overapprox.\n")
            );
        }
        Ddi_Free(auxTo);
        opt->ddi.saveClipLevel = auxClipLevel;
      }

      Ddi_BddNotAcc(ddm->exist.clipWindow);
      Ddi_BddAndAcc(myClipWindow, ddm->exist.clipWindow);
      Ddi_Free(ddm->exist.clipWindow);
      ddm->exist.clipWindow = NULL;
      ddm->exist.clipDone = 0;
      countClip++;
    } else if (Ddi_BddIsZero(to)) {
      localFrom = Ddi_BddDup(myFrom);
    }
#if ENABLE_CONFLICT
    analyzeConflict(opt->auxFwdTr, NULL, care, care, care, localFrom, opt);
#endif
    Ddi_Free(localFrom);

    if (!Ddi_BddIsZero(to)) {
      int myLevel;

      ddm->exist.clipThresh = myClipTh;

      myLevel = (level >= 2) ? level - 2 : 0;
      while (myLevel < nRings) {
        if (Ddi_BddIsOne(Ddi_BddPartRead(rings, myLevel)) ||
          FbvCheckSetIntersect(to, Ddi_BddPartRead(rings, myLevel),
            Rplus, care, 0, opt)) {
          break;
        }
        myLevel++;
      }
      if (myLevel >= nRings) {
        trace = NULL;
      } else {
        if (myLevel < nRings - 1)
          /*myLevel++ */ ;
#if 1
        myLevel += innerFail;
        if (myLevel >= nRings) {
          myLevel = nRings - 1;
        }
#endif
#if 0
        Ddi_BddSetPartConj(to);
        Ddi_BddPartInsertLast(to, Rplus);
        Ddi_BddRestrictAcc(to, care);
#endif

        trace = BckInvarCheckRecurStep(to, target, rings, Rplus, careRings, TR,
          myLevel, nestedFull + 1, nestedPart + localPart, innerFail,
          nestedCalls + 1, 0 /*part OK */ , opt);
      }
      if (trace != NULL) {
        Ddi_BddPartInsertLast(trace, myFrom);
        Ddi_Free(myFrom);
        Ddi_Free(myFrom2);
        Ddi_Free(myOuter);
        Ddi_Free(myClipWindow);
        Ddi_Free(to);
        return (trace);
      }
    }

    Ddi_Free(to);
    ddm->exist.clipThresh *= 1.5;

  } while (travAgain /* && countClip < 2 */ );

  if (travAgain) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "FULL trav ABORTED.\n")
      );
  }

  ddm->exist.clipThresh = myClipTh;

  Ddi_Free(myClipWindow);
  Ddi_Free(myFrom);
  Ddi_Free(myOuter);


#if 1
  if (localPart == 1) {

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "L: %d (id: %d) -> outer.\n", level, id)
      );

    /* try from outer ring */
    if (!noPartition) {
      if (level < nRings - 1) {
        int outerLevel = level + /* 1 */ (nRings - level - 1) / 2;

        if (outerLevel == level)
          outerLevel++;
        trace =
          BckInvarCheckRecurStep(myFrom2, target, rings, Rplus, careRings, TR,
          outerLevel, nestedFull, nestedPart, innerFail, nestedCalls + 1,
          0 /*part OK */ , opt);
        Ddi_Free(myFrom2);
        return (trace);
      }
    }
  }
#endif
  Ddi_Free(myFrom2);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "L: %d (id: %d) -> NULL.\n", level, id)
    );

  return (NULL);

}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
analyzeConflict(
  Tr_Tr_t * TR,
  Ddi_Bdd_t * inner,
  Ddi_Bdd_t * same,
  Ddi_Bdd_t * outer,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * from,
  Fbv_Globals_t * opt
)
{
  int i, full;
  Ddi_Varset_t *ps, *pi, *smooth, *supp, *supp_i;
  Tr_Mgr_t *trMgr = Tr_TrMgr(TR);
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(Tr_TrBdd(TR));
  int clk_phase = 0;

  Ddi_Bdd_t *auxTo, *prod, *myConflict;
  int auxClipLevel = opt->ddi.saveClipLevel, propagateConflict = 0;
  int saveClipTh = ddm->exist.clipThresh;

  if ((ddm->exist.conflictProduct != NULL) &&
    !Ddi_BddIsOne(ddm->exist.conflictProduct)) {

    myConflict = Ddi_BddDup(ddm->exist.conflictProduct);

    if (!Ddi_BddIsConstant(myConflict) && (outer != NULL)) {

      ddm->exist.clipLevel = opt->ddi.saveClipLevel = 0;

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Trying conflictProduct (%d).\n",
          Ddi_BddSize(myConflict))
        );

      /* take complement of conflict product */

      if (Ddi_BddIsPartDisj(ddm->exist.conflictProduct)) {
        /* partitioned NOT */
        Pdtutil_Assert(Ddi_BddPartNum(myConflict) == 2,
          "unsupported disj part in conflict product");
        for (i = 0; i < Ddi_BddPartNum(myConflict); i++) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(myConflict, i);
          Ddi_Bdd_t *clk_lit = Ddi_BddPartExtract(p, 0);

          Ddi_BddNotAcc(p);
          if (!Ddi_BddIsOne(p)) {
            propagateConflict = 1;
          }
          Ddi_BddSetPartConj(p);
          Ddi_BddPartInsert(p, 0, clk_lit);
          Ddi_Free(clk_lit);
        }

      } else {
        Ddi_BddNotAcc(myConflict);
      }

      if (propagateConflict) {
        ddm->exist.clipThresh = 1000000;
        auxTo = Tr_ImgApprox(TR, myConflict, NULL);

        ddm->exist.clipThresh = saveClipTh;

        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Conflict set: %d.\n", Ddi_BddSize(auxTo))
          );

        Ddi_BddRestrictAcc(auxTo, care);
        Ddi_BddSetPartConj(same);
        Ddi_BddRestrictAcc(auxTo, same);
        Ddi_BddRestrictAcc(same, auxTo);
        Ddi_BddPartInsert(same, 0, auxTo);
        Ddi_BddSetFlattened(same);
        Ddi_BddSetClustered(same, opt->trav.imgClustTh);
        Ddi_Free(auxTo);
      }
    }

    Ddi_Free(myConflict);

  }


  if ((outer != NULL) && (from != NULL) && !Ddi_BddIsZero(from)) {
    ddm->exist.clipThresh = 1000000;
    Ddi_BddNotAcc(from);
    auxTo = Tr_ImgApprox(TR, from, NULL);
    Ddi_BddSetPartConj(auxTo);
    if (Ddi_BddIsPartDisj(from) && (Ddi_BddPartNum(from) == 1)) {
      Ddi_BddPartInsertLast(auxTo, Ddi_BddPartRead(from, 0));
    } else if (!Ddi_BddIsPartDisj(from)) {
      Ddi_BddPartInsertLast(auxTo, from);
    }
    Ddi_BddNotAcc(from);

    ddm->exist.clipThresh = saveClipTh;

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Conflict outer set: %d ", Ddi_BddSize(auxTo))
      );

    Ddi_BddRestrictAcc(auxTo, care);
    Ddi_BddSetPartConj(outer);
    Ddi_BddRestrictAcc(auxTo, outer);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "-> %d.\n", Ddi_BddSize(auxTo))
      );
    Ddi_BddRestrictAcc(outer, auxTo);
    Ddi_BddPartInsert(outer, 0, auxTo);
    Ddi_BddSetFlattened(outer);
    Ddi_BddSetClustered(outer, opt->trav.imgClustTh);
    Ddi_Free(auxTo);
  }


  opt->ddi.saveClipLevel = ddm->exist.clipLevel = auxClipLevel;
  Ddi_Free(ddm->exist.conflictProduct);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
analyzeConflictOld(
  Tr_Tr_t * TR,
  Ddi_Bdd_t * Rplus,
  Fbv_Globals_t * opt
)
{

  int i, full;
  Ddi_Varset_t *ps, *pi, *smooth, *supp, *supp_i;
  Tr_Mgr_t *trMgr = Tr_TrMgr(TR);
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(Tr_TrBdd(TR));

  if (1 || ((ddm->exist.conflictProduct != NULL) &&
      (Ddi_BddSize(ddm->exist.conflictProduct) > 100000))) {
    Ddi_Free(ddm->exist.conflictProduct);
    return;
  }

  if (ddm->exist.conflictProduct != NULL) {
    Ddi_Bdd_t *auxTo, *prod,
      *myConflict = Ddi_BddDup(ddm->exist.conflictProduct);
    int auxClipLevel = opt->ddi.saveClipLevel;

    Ddi_Free(ddm->exist.conflictProduct);
    if (!Ddi_BddIsConstant(myConflict)) {

      ddm->exist.clipLevel = opt->ddi.saveClipLevel = 0;

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Trying conflictProduct (%d).\n",
          Ddi_BddSize(myConflict))
        );

#if 0
#else
      {
        int saveClipTh = ddm->exist.clipThresh;

        ddm->exist.clipThresh = 1000000;
        auxTo = FbvApproxPreimg(TR, Ddi_MgrReadOne(ddm), Rplus,
          myConflict, 1, opt);
        ddm->exist.clipThresh = saveClipTh;
      }
#endif

      full = 0;
      if (Ddi_BddIsZero(auxTo)) {
        full = 1;
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "FULL conflict %d.\n", Ddi_BddSize(myConflict))
          );
        Ddi_BddNotAcc(myConflict);
        Ddi_BddPartInsert(Tr_TrBdd(TR), 0, myConflict);
        Ddi_BddNotAcc(myConflict);
      } else if (Ddi_BddIsOne(auxTo)) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "NO conflict %d.\n", Ddi_BddSize(myConflict))
          );
      } else {
        fprintf(stdout, "Partial conflict %d-%d = ",
          Ddi_BddSize(myConflict), Ddi_BddSize(auxTo));
        /*        Ddi_BddDiffAcc(myConflict,auxTo); */
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "%d.\n", Ddi_BddSize(myConflict))
          );
#if 0
        Ddi_BddNotAcc(myConflict);
        Ddi_BddPartInsert(Tr_TrBdd(TR), 0, myConflict);
        Ddi_BddNotAcc(myConflict);
#endif
      }
      Ddi_Free(auxTo);

      if (1 && full) {
        Ddi_BddNotAcc(myConflict);
        prod = Ddi_BddMakePartConjVoid(ddm);

        supp = Ddi_BddSupp(myConflict);
        ps = Ddi_VarsetMakeFromArray(Tr_MgrReadPS(trMgr));
        pi = Ddi_VarsetMakeFromArray(Tr_MgrReadI(trMgr));
        smooth = Ddi_VarsetUnion(ps, pi);
        Ddi_VarsetIntersectAcc(supp, ps);

        for (i =
          (Ddi_BddPartNum(Tr_TrBdd(TR)) - ddm->exist.ddiExistRecurLevel) - 4;
          i < Ddi_BddPartNum(Tr_TrBdd(TR)); i++) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(Tr_TrBdd(TR), i);

          supp_i = Ddi_BddSupp(p);
          Ddi_VarsetIntersectAcc(supp_i, supp);
          if (!Ddi_VarsetIsVoid(supp_i)) {
            Ddi_BddPartInsertLast(prod, p);
          }
          Ddi_Free(supp_i);
        }
#if 1
        auxTo = Ddi_BddMakeConst(ddm, 0);
        {
          int j;
          Ddi_Bdd_t *auxTo1;

          Ddi_BddSetPartDisj(myConflict);
          for (j = 0; j < Ddi_BddPartNum(myConflict); j++) {
            Ddi_Bdd_t *prod1 = Ddi_BddDup(prod);

            Ddi_BddPartInsert(prod1, 0, Ddi_BddPartRead(myConflict, j));
            auxTo1 = Ddi_BddMultiwayRecursiveAndExistOver(prod1, smooth,
              Rplus, Ddi_BddPartRead(myConflict, j), 10000);
#if 1
            if (Ddi_BddIsMono(auxTo1)) {
              Ddi_BddExistAcc(auxTo1, smooth);
            } else {
              for (i = 0; i < Ddi_BddPartNum(auxTo1); i++) {
                Ddi_Bdd_t *p = Ddi_BddPartRead(auxTo1, i);

                Ddi_BddExistAcc(p, smooth);
              }
              Ddi_BddSetMono(auxTo1);
            }
            /*Ddi_BddSetClustered(auxTo,opt->trav.imgClustTh); */
#endif
            Ddi_BddOrAcc(auxTo, auxTo1);
            Ddi_Free(prod1);
            Ddi_Free(auxTo1);
          }
        }
#else
        for (i = 0; i < Ddi_BddPartNum(prod); i++) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(prod, i);

          Ddi_BddAndExistAcc(p, myConflict, smooth);
        }
        auxTo = Ddi_BddMakeClustered(prod, opt->trav.imgClustTh);
#endif

        Ddi_Free(supp);
        Ddi_Free(ps);
        Ddi_Free(pi);
        Ddi_Free(smooth);

        fprintf(stdout, "FULL FWD conflict %d->%d.\n",
          Ddi_BddSize(myConflict), Ddi_BddSize(auxTo));

        Ddi_Free(auxTo);
      }

      opt->ddi.saveClipLevel = auxClipLevel;
    }
    Ddi_Free(myConflict);

  }

}

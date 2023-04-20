/**CFile***********************************************************************

  FileName    [trImg.c]

  PackageName [tr]

  Synopsis    [Functions for image/preimage computations]

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

#define APPROX_BY_RESTRICT 0

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static Ddi_Bdd_t *TrImgConjPartTr(
  Tr_Mgr_t * trMgr,
  Ddi_Bdd_t * TR,
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * constrain,
  Ddi_Vararray_t * psv,
  Ddi_Vararray_t * nsv,
  Ddi_Varset_t * smoothV
);
static Ddi_Bdd_t *TrImgDisjPartTr(
  Tr_Mgr_t * trMgr,
  Ddi_Bdd_t * TR,
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * constrain,
  Ddi_Vararray_t * psv,
  Ddi_Vararray_t * nsv,
  Ddi_Varset_t * smoothV
);
static Ddi_Bdd_t *TrImgDisjPartSet(
  Tr_Mgr_t * trMgr,
  Ddi_Bdd_t * TR,
  Ddi_Bdd_t * part_from,
  Ddi_Bdd_t * constrain,
  Ddi_Vararray_t * psv,
  Ddi_Vararray_t * nsv,
  Ddi_Varset_t * smoothV
);
static Ddi_Bdd_t *TrImgApproxConjPartTr(
  Tr_Mgr_t * trMgr,
  Ddi_Bdd_t * TR,
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * constrain,
  Ddi_Vararray_t * psv,
  Ddi_Vararray_t * nsv,
  Ddi_Varset_t * smoothV
);
static Ddi_Bdd_t *TrImgApproxImgAux(
  Tr_Mgr_t * trMgr,
  Ddi_Bdd_t * TR_i,
  Ddi_Bdd_t * from,
  Ddi_Vararray_t * psv,
  Ddi_Vararray_t * nsv,
  Ddi_Varset_t * smoothV,
  int *abortDoneP
);
static Ddi_Bdd_t *TrImgClockedTr(
  Tr_Tr_t * TR,
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * constrain
);

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of external functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Compute image of a conjunctively partitioned transition
    relation.]

  Description []

  SideEffects [None]

  SeeAlso     [Part_BddarrayMultiwayAndExist]

******************************************************************************/

Ddi_Bdd_t *
Tr_Img(
  Tr_Tr_t * TR /* Partitioned TR */ ,
  Ddi_Bdd_t * from              /* Input constrain */
)
{
  return (Tr_ImgWithConstrain(TR, from, NULL));
}

/**Function********************************************************************

  Synopsis    [Compute image of a conjunctively partitioned transition
    relation.]

  Description []

  SideEffects [None]

  SeeAlso     [Part_BddarrayMultiwayAndExist]

******************************************************************************/

Ddi_Bdd_t *
Tr_ImgWithConstrain(
  Tr_Tr_t * TR /* Partitioned TR */ ,
  Ddi_Bdd_t * from /* domain set */ ,
  Ddi_Bdd_t * constrain         /* range constrain */
)
{
  Tr_Mgr_t *trMgr = Tr_TrMgr(TR);
  Ddi_Vararray_t *psv;
  Ddi_Vararray_t *nsv;
  Ddi_Varset_t *smoothV, *pis;
  Ddi_Bdd_t *to = NULL, *trBdd, *myTr, *myFrom;
  Ddi_Mgr_t *ddTR = Ddi_ReadMgr(Tr_TrBdd(TR));  /* TR DD manager */
  Ddi_Mgr_t *ddR = Ddi_ReadMgr(from);   /* Reached DD manager */
  Pdtutil_VerbLevel_e verbosity = Tr_MgrReadVerbosity(trMgr);
  int setMonoTo = 0;
  int extRef, nodeId;

  if (Tr_TrClk(TR) != NULL && (TR->clkTrFormat == 0)) {
    return (TrImgClockedTr(TR, from, constrain));
  }

  trMgr = TR->trMgr;

  extRef = Ddi_MgrReadExtRef(ddR);
  nodeId = Ddi_MgrReadCurrNodeId(ddR);

  psv = trMgr->ps;
  nsv = trMgr->ns;

  smoothV = Ddi_VarsetMakeFromArray(trMgr->ps);

  if ((trMgr->settings.image.smoothPi) && (trMgr->i != NULL)) {
    pis = Ddi_VarsetMakeFromArray(trMgr->i);
    Ddi_VarsetUnionAcc(smoothV, pis);
    Ddi_Free(pis);
  }
  if (trMgr->auxVars != NULL) {
    Ddi_Varset_t *auxvSet = Ddi_VarsetMakeFromArray(trMgr->auxVars);

    Ddi_VarsetUnionAcc(smoothV, auxvSet);
    Ddi_Free(auxvSet);
  }

  trBdd = Tr_TrBdd(TR);


  if (trMgr->settings.image.partThTr >= 0) {
    myTr = Part_PartitionDisjSet(trBdd, NULL, NULL,
      trMgr->settings.image.partitionMethod,
      trMgr->settings.image.partThTr, -1, verbosity);
    setMonoTo = 1;
  } else {
#if 0
    if (!Ddi_BddIsMono(trBdd) && (Ddi_BddPartNum(trBdd) == 1)) {
      myTr = Ddi_BddDup(Ddi_BddPartRead(trBdd, 0));
    } else
#endif
    {
      myTr = Ddi_BddDup(trBdd);
    }
  }
  if (trMgr->settings.image.partThFrom >= 0) {
    myFrom = Part_PartitionDisjSet(from, NULL, NULL,
      trMgr->settings.image.partitionMethod,
      trMgr->settings.image.partThFrom, -1, verbosity);
    setMonoTo = 1;
  } else {
    myFrom = Ddi_BddDup(from);
  }

  if (Ddi_BddIsPartDisj(myTr)) {
    to = TrImgDisjPartTr(trMgr, myTr, myFrom, constrain, psv, nsv, smoothV);
    setMonoTo = 0;
  } else if (Ddi_BddIsPartDisj(myFrom) &&
    (trMgr->settings.image.method != Tr_ImgMethodApprox_c)) {
    to = TrImgDisjPartSet(trMgr, myTr, myFrom, constrain, psv, nsv, smoothV);
  } else {
    Pdtutil_Assert(Ddi_BddIsPartConj(myTr) || Ddi_BddIsMono(myTr),
      "Unexpected TR decomposition in image computation");

    switch (trMgr->settings.image.method) {
      case Tr_ImgMethodCofactor_c:
        Ddi_MgrSiftSuspend(ddTR);
        Ddi_BddConstrainAcc(myTr, myFrom);
        Ddi_MgrSiftResume(ddTR);
        Ddi_Free(myFrom);
        myFrom = Ddi_BddMakeConst(ddR, 1);
      case Tr_ImgMethodMonolithic_c:
      case Tr_ImgMethodIwls95_c:
        to =
          TrImgConjPartTr(trMgr, myTr, myFrom, constrain, psv, nsv, smoothV);
        break;
      case Tr_ImgMethodPartSat_c:
        to = Ddi_ImgDisjSat(myTr, myFrom, constrain, psv, nsv, smoothV);
        break;
      case Tr_ImgMethodApprox_c:
#if 0
        Ddi_MgrSiftSuspend(ddTR);
        Ddi_BddConstrainAcc(myTr, myFrom);
        Ddi_MgrSiftResume(ddTR);
        Ddi_Free(myFrom);
        myFrom = Ddi_BddMakeConst(ddR, 1);
#endif
        to = TrImgApproxConjPartTr(trMgr, myTr, myFrom,
          constrain, psv, nsv, smoothV);
        if (0) {
          Ddi_Bdd_t *myTr2 = Ddi_BddDup(trBdd);
          Ddi_Bdd_t *myTo = TrImgConjPartTr(trMgr, myTr2,
            myFrom, constrain, psv, nsv, smoothV);
          Ddi_Bdd_t *myTo0 = Ddi_BddMakeMono(to);

          Ddi_BddSetMono(myTo);
          Pdtutil_Assert(Ddi_BddIncluded(myTo, myTo0), "Underapprox");
          Ddi_Free(myTo0);
          Ddi_Free(myTo);
          Ddi_Free(myTr2);
        }

        if (!Ddi_BddEqual(myTr, trBdd)) {
          /* updated within image */
          TrTrRelWrite(TR, myTr);
        }
        break;
    }
  }

  Ddi_Free(myFrom);
  Ddi_Free(myTr);
  Ddi_Free(smoothV);

#if 0
  if (Ddi_MgrCheckExtRef(ddR, extRef + 1) == 0) {
    Ddi_MgrPrintExtRef(ddR, nodeId);
  }
#endif

  if (setMonoTo) {
    Ddi_BddSetMono(to);
  } else {
    if (!Ddi_BddIsMeta(to)) {
      Ddi_BddSetClustered(to, trMgr->settings.image.clusterThreshold);
    }
  }

  return (to);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Tr_ImgOpt(
  Tr_Tr_t * TR,
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * care
)
{
  Ddi_Varset_t *ps, *ns, *smoothF, *smoothB, *pi;
  Tr_Mgr_t *trMgr = Tr_TrMgr(TR);
  Ddi_Bdd_t *trBdd, *trGroup, *to, *toPlus, *pre, *myFrom, *to_i, *tr_i;
  Ddi_Vararray_t *psv;
  Ddi_Vararray_t *nsv;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(from);
  int i;

  psv = Tr_MgrReadPS(trMgr);
  nsv = Tr_MgrReadNS(trMgr);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Image Opt.\n"));

  trBdd = Tr_TrBdd(TR);

  ps = Ddi_VarsetMakeFromArray(psv);
  ns = Ddi_VarsetMakeFromArray(nsv);
  pi = Ddi_VarsetMakeFromArray(Tr_MgrReadI(trMgr));

  myFrom = Ddi_BddDup(from);

#if 1
  pre = Ddi_BddMakePartConjVoid(ddiMgr);
#endif
  toPlus = Ddi_BddMakePartConjVoid(ddiMgr);
  Ddi_BddSetPartConj(trBdd);

  trGroup = Ddi_BddDup(trBdd);

#if 0
  {
    int j;

    for (i = 1, j = 0; i < Ddi_BddPartNum(trGroup);) {
      Ddi_Bdd_t *group, *p;

      group = Ddi_BddPartRead(trGroup, j);
      Ddi_BddSetPartConj(group);
      p = Ddi_BddPartRead(trGroup, i);
      Ddi_BddPartInsertLast(group, p);
      if (Ddi_BddSize(group) > opt->trav.apprGrTh) {
        /* resume from group insert */
        p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
        Ddi_Free(p);
        i++;
        j++;
      } else {
        p = Ddi_BddPartExtract(trGroup, i);
        Ddi_Free(p);
      }
    }
  }
#endif

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "[%d]\n", Ddi_BddPartNum(trGroup)));

  smoothF = Ddi_VarsetUnion(ps, pi);
  smoothB = Ddi_VarsetUnion(ns, pi);

  for (i = 0, Ddi_VarsetWalkStart(ns);
    !Ddi_VarsetWalkEnd(ns); Ddi_VarsetWalkStep(ns), i++) {
    if (i > Ddi_VarsetNum(ns) * 5 / 6) {
      Ddi_VarsetAddAcc(smoothF, Ddi_VarsetWalkCurr(ns));
    }
  }


  for (i = 0; i < Ddi_BddPartNum(trGroup); i++) {
    tr_i = Ddi_BddPartRead(trGroup, i);
    to_i = Ddi_BddAndExist(tr_i, myFrom, smoothF);
#if 0
    pre_i = Ddi_BddAndExist(tr_i, to_i, smoothB);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "[F:%d|B:%d]", Ddi_BddSize(to_i), Ddi_BddSize(pre_i)));

    Ddi_BddPartInsertLast(pre, pre_i);
    Ddi_Free(pre_i);
#else
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "[F:%d]", Ddi_BddSize(to_i)));
#endif
    Ddi_BddPartInsertLast(toPlus, to_i);
    Ddi_Free(to_i);
  }

  Ddi_Free(trGroup);

  Ddi_BddSwapVarsAcc(toPlus, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
  Ddi_BddSetClustered(toPlus, 1000);

#if 0
#if 0
  pre = FbvApproxPreimg(TR, toPlus, Ddi_MgrReadOne(ddiMgr), NULL, 1, opt);
#else
  pre = FbvApproxPreimg(TR, toPlus, myFrom, NULL, 1, opt);
#endif
#else
  /*pre = Ddi_BddAndExist(trBdd,toPlus,smoothB); */
#endif
  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "PRE -> %d\n", Ddi_BddSize(pre)));

  Ddi_BddRestrictAcc(myFrom, pre);
  Ddi_Free(pre);

  Ddi_Free(smoothF);
  Ddi_Free(smoothB);

  if (care != NULL) {
    Ddi_BddRestrictAcc(toPlus, care);
    Ddi_BddPartInsert(toPlus, 0, care);
  }

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "\nTO+ -> %d\n", Ddi_BddSize(toPlus)));

  Ddi_BddSwapVarsAcc(toPlus, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));

  to = Tr_ImgWithConstrain(TR, myFrom, toPlus);

  Ddi_BddSwapVarsAcc(toPlus, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));

  Ddi_BddPartInsertLast(toPlus, to);
  Ddi_BddSetClustered(toPlus, 5000);
  Ddi_Free(to);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "\nTO -> %d\n", Ddi_BddSize(toPlus)));

  Ddi_BddSetMono(toPlus);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "TOmono -> %d.\n", Ddi_BddSize(toPlus));
    fprintf(stdout, "OPT IMG DONE.\n"));

  Ddi_Free(myFrom);
  Ddi_Free(ps);
  Ddi_Free(ns);
  Ddi_Free(pi);

  return (toPlus);
}

/**Function********************************************************************

  Synopsis    [Compute image of a conjunctively partitioned transition
    relation.]

  Description []

  SideEffects [None]

  SeeAlso     [Part_BddarrayMultiwayAndExist]

******************************************************************************/

Ddi_Bdd_t *
Tr_ImgConjPartTr(
  Tr_Mgr_t * trMgr,
  Ddi_Bdd_t * TR,
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * constrain,
  Ddi_Vararray_t * psv,
  Ddi_Vararray_t * nsv,
  Ddi_Varset_t * smoothV
)
{
  return TrImgConjPartTr(trMgr, TR, from, constrain, psv, nsv, smoothV);
}



/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Tr_ImgApprox(
  Tr_Tr_t * TR,
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * care
)
{
  Ddi_Varset_t *ps, *ns, *smoothV, *pi;
  Tr_Mgr_t *trMgr = Tr_TrMgr(TR);
  Ddi_Bdd_t *trBdd, *to, *myFrom, *overAppr;
  Ddi_Vararray_t *psv;
  Ddi_Vararray_t *nsv;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(from);
  int i;
  Pdtutil_VerbLevel_e verbosityDd = Ddi_MgrReadVerbosity(ddiMgr);

  Ddi_MgrSetOption(ddiMgr, Pdt_DdiVerbosity_c, inum, ((int)verbosityDd - 1));

  psv = Tr_MgrReadPS(trMgr);
  nsv = Tr_MgrReadNS(trMgr);

  trBdd = Tr_TrBdd(TR);

  ps = Ddi_VarsetMakeFromArray(psv);
  ns = Ddi_VarsetMakeFromArray(nsv);
  pi = Ddi_VarsetMakeFromArray(Tr_MgrReadI(trMgr));

  smoothV = Ddi_VarsetUnion(ps, pi);

  if (Tr_TrClk(TR) != NULL) {
    Ddi_Var_t *clk = Tr_TrClk(TR);
    Ddi_Bdd_t *trBdd_i, *trBddPhase, *from_i, *to_i, *clk_lit;

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "Approximate Clocked Img.\n"));

    to = Ddi_BddMakePartDisjVoid(ddiMgr);
    for (i = 0; i < 2; i++) {
      Pdtutil_Assert(Ddi_BddIsPartDisj(trBdd), "Wrong format for clocked TR");
      trBdd_i = Ddi_BddPartRead(trBdd, i);
      Pdtutil_Assert(Ddi_BddIsPartConj(trBdd_i),
        "Wrong format for clocked TR");
      Pdtutil_Assert(Ddi_BddPartNum(trBdd_i) == 2,
        "Wrong format for clocked TR");
      trBddPhase = Ddi_BddPartRead(trBdd_i, 1);

      from_i = Ddi_BddCofactor(from, clk, i);
      Ddi_BddSetFlattened(from_i);

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
        fprintf(stdout, "\nApproximate Clocked Part %d.\n", i));

      if (Ddi_BddIsZero(from_i)) {
        to_i = Ddi_BddMakeConst(ddiMgr, 0);
      } else {
#if 0
        to_i = approxImgAux(trBddPhase, from_i, smoothV);
        Ddi_BddSwapVarsAcc(to_i, psv, nsv);
#else
        Tr_Tr_t *TR_i = Tr_TrMakeFromRel(trMgr, trBddPhase);

        Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodApprox_c);

        to_i = Tr_Img(TR_i, from_i);
        if (!Ddi_BddEqual(trBddPhase, Tr_TrBdd(TR_i))) {
          Ddi_BddPartWrite(trBdd_i, 1, Tr_TrBdd(TR_i));
        }

        Tr_TrFree(TR_i);
#endif
      }

      clk_lit = Ddi_BddMakeLiteral(clk, 1 - i);
      Ddi_BddSetPartConj(to_i);
      Ddi_BddPartInsert(to_i, 0, clk_lit);

      Ddi_BddPartWrite(to, 1 - i, to_i);

      Ddi_Free(clk_lit);
      Ddi_Free(from_i);
      Ddi_Free(to_i);
    }

  } else {

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMax_c,
      fprintf(stdout, "Approximate Image.\n"));

#if 1
    Ddi_Free(smoothV);
    Ddi_Free(ps);
    Ddi_Free(ns);
    Ddi_Free(pi);
    Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodApprox_c);
    to = Tr_Img(TR, from);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiVerbosity_c, inum, verbosityDd);
    return (to);
#endif

    overAppr = Ddi_BddDup(from);
    myFrom = Ddi_BddDup(from);

    {
      Ddi_Bdd_t *prod = Ddi_BddDup(trBdd);

      Ddi_BddPartInsert(prod, 0, myFrom);
      to = Ddi_BddMultiwayRecursiveAndExistOver(prod, smoothV,
        care, overAppr, 10000);

      Ddi_Free(prod);
    }

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "\n->%d", Ddi_BddSize(to)));

    Ddi_BddSwapVarsAcc(to, psv, nsv);

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
      printf("\nApproximate Img Computed.\n");
      );

    Ddi_Free(myFrom);
    Ddi_Free(overAppr);

  }

  Ddi_Free(smoothV);
  Ddi_Free(ps);
  Ddi_Free(ns);
  Ddi_Free(pi);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiVerbosity_c, inum, verbosityDd);

  return (to);
}



/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Internal image computation function.]

  Description [Internal image computation function.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static Ddi_Bdd_t *
TrImgConjPartTr(
  Tr_Mgr_t * trMgr /* Tr manager */ ,
  Ddi_Bdd_t * TR /* Partitioned TR */ ,
  Ddi_Bdd_t * from /* domain set */ ,
  Ddi_Bdd_t * constrain /* range constrain */ ,
  Ddi_Vararray_t * psv /* Array of present state variables */ ,
  Ddi_Vararray_t * nsv /* Array of next state variables */ ,
  Ddi_Varset_t * smoothV        /* Variables to be abstracted */
)
{
  Ddi_Bdd_t *to;
  Ddi_Mgr_t *ddTR = Ddi_ReadMgr(TR);    /* TR DD manager */
  Ddi_Mgr_t *ddR = Ddi_ReadMgr(from);   /* Reached DD manager */
  Ddi_Varset_t *mySmooth, *suppProd, *auxvSet = NULL;
  int i, nLoops;

  if (Ddi_BddIsPartDisj(TR)) {
    to = TrImgDisjPartTr(trMgr, TR, from, constrain, psv, nsv, smoothV);
    return (to);
  } else if (Ddi_BddIsPartDisj(from)) {
    to = TrImgDisjPartSet(trMgr, TR, from, constrain, psv, nsv, smoothV);
    return (to);
  }

  Pdtutil_Assert(Ddi_BddIsPartConj(TR) || Ddi_BddIsMono(TR),
    "Unexpected TR decomposition in image computation");

  from = Ddi_BddCopy(ddTR, from);

  smoothV = Ddi_VarsetDup(smoothV);
  if (0 && trMgr->auxVars != NULL) {
    Ddi_Varset_t *piSet = Ddi_VarsetMakeFromArray(Tr_MgrReadI(trMgr));

    auxvSet = Ddi_VarsetMakeFromArray(trMgr->auxVars);
    Ddi_VarsetUnionAcc(auxvSet, piSet);
    Ddi_Free(piSet);
    Ddi_VarsetDiffAcc(smoothV, auxvSet);
  }
#if 0
  if (constrain == NULL) {
    to = Ddi_BddAndExist(TR, from, smoothV);
  } else
#endif

  {
    Ddi_Bdd_t *prod = Ddi_BddDup(TR);
    Ddi_Bdd_t *prod1 = Ddi_BddMakePartConjVoid(ddTR);
    Ddi_Bdd_t *clk_part = NULL;
    int np, again;

    Ddi_BddSetPartConj(prod);
    if (Ddi_BddSize(Ddi_BddPartRead(prod, 0)) < 4) {
      clk_part = Ddi_BddPartExtract(prod, 0);
    }

    nLoops = 0;
    do {

      if (Ddi_BddPartNum(prod) > 1) {
#if 0
        Ddi_Varsetarray_t *tightSupp;
#endif
        Ddi_Varset_t *suppFrom = Ddi_BddSupp(from);
        Ddi_Varset_t *clk_supp = NULL;
        Ddi_Bdd_t *prodNew = Ddi_BddMakePartConjVoid(ddTR);

        Ddi_VarsetIntersectAcc(suppFrom, smoothV);
        Ddi_BddSetPartConj(prod);
        if (clk_part != NULL) {
          clk_supp = Ddi_BddSupp(clk_part);
        }
#if 0
        tightSupp = Ddi_BddTightSupports(from);
#if 0
        for (i = 0; i < Ddi_VarsetarrayNum(tightSupp); i++) {
#else
        for (i = Ddi_VarsetarrayNum(tightSupp) - 1; i >= 0; i--) {
#endif
          int j, k;
          Ddi_Varset_t *s = Ddi_VarsetarrayRead(tightSupp, i);
          Ddi_Vararray_t *vars = Ddi_VararrayMakeFromVarset(s, 1);

          for (j = Ddi_VararrayNum(vars) - 1; j >= 0; j--) {
            Ddi_Var_t *v = Ddi_VararrayRead(vars, j);

            for (k = Ddi_BddPartNum(prod) - 1; k >= 0; k--) {
              Ddi_Bdd_t *p = Ddi_BddPartRead(prod, k);
              Ddi_Varset_t *suppP = Ddi_BddSupp(p);

              if (Ddi_VarInVarset(suppP, v)) {
                p = Ddi_BddPartExtract(prod, k);
                Ddi_BddPartInsertLast(prodNew, p);
                Ddi_Free(p);
              }
              Ddi_Free(suppP);
            }
          }
          Ddi_Free(vars);
        }
        for (i = 0; i < Ddi_BddPartNum(prod); i++) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(prod, i);

          Ddi_BddPartInsertLast(prodNew, p);
        }
        Ddi_Free(tightSupp);
#else
        Ddi_Free(prodNew);
        prodNew = Ddi_BddDup(prod);
#endif
        Ddi_Free(prod);
        prod = Ddi_BddDup(prodNew);

        Ddi_Free(prodNew);

        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMax_c,
          fprintf(stdout, "FROM: %d (", Ddi_BddSize(from))
          );

#if 0
        tightSupp = Ddi_BddTightSupports(from);
#if 0
        for (i = 0; i < Ddi_VarsetarrayNum(tightSupp); i++) {
#else
        for (i = Ddi_VarsetarrayNum(tightSupp) - 1; i >= 0; i--) {
#endif
          int j, k;
          Ddi_Varset_t *s = Ddi_VarsetarrayRead(tightSupp, i);
          Ddi_Vararray_t *vars = Ddi_VararrayMakeFromVarset(s, 1);

          for (j = Ddi_VararrayNum(vars) - 1; j >= 0; j--) {
            Ddi_Var_t *v = Ddi_VararrayRead(vars, j);

            for (k = Ddi_BddPartNum(prod) - 1; k >= 0; k--) {
              Ddi_Bdd_t *p = Ddi_BddPartRead(prod, k);
              Ddi_Varset_t *suppP = Ddi_BddSupp(p);

              if (Ddi_VarInVarset(suppP, v)) {
                p = Ddi_BddPartExtract(prod, k);
                Ddi_BddPartInsertLast(prodNew, p);
                Ddi_Free(p);
              }
              Ddi_Free(suppP);
            }
          }
          Ddi_Free(vars);
        }
        for (i = 0; i < Ddi_BddPartNum(prod); i++) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(prod, i);

          Ddi_BddPartInsertLast(prodNew, p);
        }
        Ddi_Free(tightSupp);
#else
        Ddi_Free(prodNew);
        prodNew = Ddi_BddDup(prod);
#endif
        Ddi_Free(prod);
        prod = Ddi_BddDup(prodNew);

        Ddi_Free(prodNew);

        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
          fprintf(stdout, "FROM: %d (", Ddi_BddSize(from))
          );

        for (i = 0; 0 && i < Ddi_BddPartNum(prod); i++) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(prod, i);
          Ddi_Varset_t *suppP = Ddi_BddSupp(p);
          Ddi_Varset_t *supp = Ddi_VarsetIntersect(suppP, suppFrom);

          if (1 && Ddi_VarsetIsVoid(supp)) {
            p = Ddi_BddPartExtract(prod, i);
            mySmooth = Ddi_VarsetIntersect(smoothV, suppP);
            suppProd = Ddi_BddSupp(prod);
            Ddi_VarsetDiffAcc(mySmooth, suppProd);
            if (clk_supp != NULL)
              Ddi_VarsetDiffAcc(mySmooth, clk_supp);
            if (!Ddi_VarsetIsVoid(mySmooth)) {
              Ddi_Free(suppProd);
              suppProd = Ddi_BddSupp(prod1);
              Ddi_VarsetDiffAcc(mySmooth, suppProd);
              if (!Ddi_VarsetIsVoid(mySmooth)) {
                Ddi_BddExistAcc(p, mySmooth);
              }
            }
            if (!Ddi_BddIsOne(p)) {
              Ddi_BddPartInsertLast(prod1, p);
            }
            Ddi_Free(p);
            i--;
            Ddi_Free(suppProd);
            Ddi_Free(mySmooth);
          } else {
#if 1
            mySmooth = Ddi_VarsetDiff(suppP, suppFrom);
            Ddi_VarsetIntersectAcc(mySmooth, smoothV);
            if (clk_supp != NULL) {
              Ddi_VarsetDiffAcc(mySmooth, clk_supp);
              Ddi_VarsetUnionAcc(supp, clk_supp);
            }
            if (!Ddi_VarsetIsVoid(mySmooth)) {
              Ddi_Bdd_t *p0 = Ddi_BddExist(p, mySmooth);
              Ddi_Bdd_t *p1 = Ddi_BddExist(p, supp);
              Ddi_Bdd_t *p01 = Ddi_BddAnd(p0, p1);

              if (Ddi_BddEqual(p01, p)) {
                Ddi_BddPartInsertLast(prod1, p1);
                Ddi_BddPartWrite(prod, i, p0);
              }
              Ddi_Free(p0);
              Ddi_Free(p1);
              Ddi_Free(p01);
            }
            Ddi_Free(mySmooth);
#else
            /* correct, but restricted BDD can make diverging BDD sizes in and-ex */
            Ddi_Bdd_t *fromProj;

            fromProj = Ddi_BddExistProject(from, supp);
            Ddi_BddRestrictAcc(from, fromProj);
#if 1
            Ddi_BddSetPartConj(p);
            Ddi_BddPartInsert(p, 0, fromProj);
#else
            Ddi_BddAndAcc(p, fromProj);
#endif
            Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
              fprintf(stdout, "{%d}", Ddi_BddSize(fromProj))
              );
            Ddi_Free(fromProj);
#endif
          }
          Ddi_Free(supp);
          Ddi_Free(suppP);
        }
        Ddi_Free(suppFrom);
        Ddi_Free(clk_supp);
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
          fprintf(stdout, ") -> %d.\n", Ddi_BddSize(from))
          );
      }

      if (Ddi_BddIsPartConj(from)) {
        Ddi_Bdd_t *f2 = Ddi_BddDup(from);
        Ddi_Varset_t *suppTR = Ddi_BddSupp(TR);
        int i;

        for (i = Ddi_BddPartNum(f2) - 1; i >= 0; i--) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(f2, i);
          Ddi_Varset_t *supp = Ddi_BddSupp(p);

          Ddi_VarsetIntersectAcc(supp, suppTR);
          if (Ddi_VarsetIsVoid(supp)) {
            p = Ddi_BddPartExtract(f2, i);
            Ddi_Free(p);
          }
          Ddi_Free(supp);
        }
        Ddi_Free(suppTR);
        Ddi_BddPartInsert(prod, 0, f2);
        Ddi_Free(f2);
      } else {
        Ddi_BddPartInsert(prod, 0, from);
      }

      if (clk_part != NULL) {
        Ddi_BddPartInsert(prod, 0, clk_part);
        Ddi_Free(clk_part);
      }

      if (Ddi_BddPartNum(prod1) > 0) {
        suppProd = Ddi_BddSupp(prod1);
        mySmooth = Ddi_VarsetDiff(smoothV, suppProd);
        Ddi_Free(suppProd);
      } else {
        mySmooth = Ddi_VarsetDup(smoothV);
      }

#if 0
      Ddi_MgrSiftSuspend(ddTR);
      prod =
        Ddi_BddEvalFree(Part_BddMultiwayLinearAndExist(prod, mySmooth, 500),
        prod);
      Ddi_MgrSiftResume(ddTR);
#if 0
      {
        Ddi_Varset_t *ns = Ddi_VarsetMakeFromArray(nsv);

        Tr_TrBddSortIwls95(trMgr, prod, smoothV, ns);
        Ddi_Free(ns);
      }
#endif
#endif

      if (trMgr->settings.image.checkFrozenVars) {
        to =
          Ddi_BddMultiwayRecursiveAndExistOverWithFrozenCheck(prod, mySmooth,
          from, psv, nsv, constrain, NULL, -1, 1);
      } else {

#if 1
        to = Ddi_BddMultiwayRecursiveAndExist(prod, mySmooth,
          constrain, trMgr->settings.image.andExistTh);
#else
        to = Part_BddMultiwayLinearAndExist(prod, mySmooth, -1);
#endif

      }

      Ddi_Free(mySmooth);
      Ddi_Free(prod);

      again = 0;
      if (!Ddi_BddIsZero(to) && Ddi_BddPartNum(prod1) > 0) {
        int nSuppSmoothV;
        Ddi_Varset_t *suppTo = Ddi_BddSupp(to);

        suppProd = Ddi_BddSupp(prod1);
        Ddi_VarsetIntersectAcc(suppTo, smoothV);
        nSuppSmoothV = Ddi_VarsetNum(suppTo);
        Ddi_VarsetIntersectAcc(suppTo, suppProd);
        Ddi_Free(suppProd);
        if (!Ddi_VarsetIsVoid(suppTo)) {
          again = 1;
          prod = prod1;
          Ddi_Free(from);
          from = to;
          prod1 = Ddi_BddMakePartConjVoid(ddTR);
          Ddi_BddSetPartConj(prod);
        } else {
          if (nSuppSmoothV > 0) {
            Ddi_BddExistAcc(to, smoothV);
          }
        }
        Ddi_Free(suppTo);
      }

    } while (again /* && (nLoops++ < 1) */ );

    if (trMgr->settings.image.assumeTrRangeIs1 || Ddi_BddIsZero(to)) {
      Ddi_Free(prod1);
    } else
#if 0
    if (Ddi_BddIsMeta(to)) {
      if (Ddi_BddPartNum(prod1) > 0) {
        Ddi_BddPartInsert(prod1, 0, to);
        Ddi_Free(to);
        to = Ddi_BddMultiwayRecursiveAndExist(prod1, smoothV, constrain, -1);
      }
      Ddi_Free(prod1);
    } else
#endif
    {
      Ddi_Bdd_t *myTo = NULL;

      Ddi_BddSetPartConj(prod1);
      suppProd = Ddi_BddSupp(prod1);
      mySmooth = Ddi_VarsetIntersect(suppProd, smoothV);
      Ddi_VarsetDiffAcc(suppProd, smoothV);
      if (!Ddi_VarsetIsVoid(suppProd) && (Ddi_VarsetNum(mySmooth) > 0)) {
        np = Ddi_BddPartNum(prod1);
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
          fprintf(stdout, "TR IMG SM1 (%d) extra part: %d.\n",
            Ddi_VarsetNum(mySmooth), np)
          );
        myTo = Ddi_BddMultiwayRecursiveAndExist(prod1, mySmooth,
          constrain, -1);
        Ddi_BddSetPartConj(myTo);
      } else if (!Ddi_VarsetIsVoid(suppProd)) {
        /* no vars to smooth */
        myTo = Ddi_BddDup(prod1);
      }
      Ddi_Free(prod1);
      Ddi_Free(suppProd);
      if (myTo != NULL) {
        for (i = 0; i < Ddi_BddPartNum(myTo); i++) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(myTo, i);

          Ddi_BddSetPartConj(to);
          Ddi_BddPartInsertLast(to, p);
        }
      }
      Ddi_Free(myTo);
      Ddi_Free(mySmooth);
    }

  }
#if DEVEL_OPT_ANDEXIST
  if (Ddi_BddIsMono(to)) {
    Pdtutil_Assert((!trMgr->settings.image.checkFrozenVars),
      "Frozen check not compatible with DEVEL_OPT_ANDEXIST");
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "IMG0 = %d ->", Ddi_BddSize(to))
      );
    Ddi_BddExistAcc(to, smoothV);
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "IMG1 = %d.\n", Ddi_BddSize(to))
      );
  }
#endif
  Ddi_BddExistAcc(to, smoothV);


  Ddi_MgrSiftSuspend(ddR);
  /* Swap present state variables (PS) next state variables (NS) */
  if (1 || Ddi_BddIsPartConj(to)) {
    Ddi_BddSwapVarsAcc(to, nsv, psv);
  } else {
    Ddi_BddSubstVarsAcc(to, nsv, psv);
  }
  Ddi_MgrSiftResume(ddR);

  /* transfer to reached manager */
  to = Ddi_BddEvalFree(Ddi_BddCopy(ddR, to), to);

  if (auxvSet != NULL) {
    Ddi_BddExistAcc(to, auxvSet);
    Ddi_Free(auxvSet);
  }

  Ddi_Free(from);
  Ddi_Free(smoothV);

  return (to);
}


/**Function********************************************************************

  Synopsis    [Compute image of a disjunctively partitioned transition
    relation]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static Ddi_Bdd_t *
TrImgDisjPartTr(
  Tr_Mgr_t * trMgr /* Tr manager */ ,
  Ddi_Bdd_t * TR /* Partitioned TR */ ,
  Ddi_Bdd_t * from /* domain set */ ,
  Ddi_Bdd_t * constrain /* range constrain */ ,
  Ddi_Vararray_t * psv /* Array of present state variables */ ,
  Ddi_Vararray_t * nsv /* Array of next state variables */ ,
  Ddi_Varset_t * smoothV        /* Variables to be abstracted */
)
{
  Ddi_Bdd_t *to, *to_i, *TR_i;
  int i;
  Ddi_Mgr_t *ddR = Ddi_ReadMgr(from);   /* dd manager */

  if (Ddi_BddIsPartConj(TR)) {
    return (TrImgConjPartTr(trMgr, TR, from, constrain, psv, nsv, smoothV));
  }

  if (Ddi_BddIsMeta(from)) {
    to = Ddi_BddMakeConst(ddR, 0);
    Ddi_BddSetMeta(to);
  } else {
    to = Ddi_BddMakePartDisjVoid(ddR);
  }

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "DIsj TR IMG (%d partitions).\n", Ddi_BddPartNum(TR))
    );

  for (i = 0; i < Ddi_BddPartNum(TR); i++) {
    TR_i = Ddi_BddPartRead(TR, i);
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "Part[%d].\n", i);
      fflush(stdout)
      );
    to_i = TrImgConjPartTr(trMgr, TR_i, from, constrain, psv, nsv, smoothV);
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "|To[%d]|=%d.\n", i, Ddi_BddSize(to_i))
      );

    if (Ddi_BddIsMeta(from)) {
      Ddi_BddOrAcc(to, to_i);
    } else {
      Ddi_BddSetMono(to_i);
      Ddi_BddPartInsertLast(to, to_i);
    }
    Ddi_Free(to_i);
  }

  return (to);
}



/**Function********************************************************************

  Synopsis    [Compute image of a disjunctively partitioned from set.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static Ddi_Bdd_t *
TrImgDisjPartSet(
  Tr_Mgr_t * trMgr /* Tr manager */ ,
  Ddi_Bdd_t * TR /* Partitioned TR */ ,
  Ddi_Bdd_t * part_from /* Input state set */ ,
  Ddi_Bdd_t * constrain /* range constrain */ ,
  Ddi_Vararray_t * psv /* Array of present state variables */ ,
  Ddi_Vararray_t * nsv /* Array of next state variables */ ,
  Ddi_Varset_t * smoothV        /* Variables to be abstracted */
)
{
  Ddi_Bdd_t *to, *to_i, *from_i;
  int i;
  Ddi_Mgr_t *ddR = Ddi_ReadMgr(part_from);  /* dd manager */

  Pdtutil_Assert(Ddi_BddIsPartDisj(part_from),
    "The from set of a partitioned image is not partitioned");

  to = Ddi_BddMakePartDisjVoid(ddR);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "DIsj FROM (%d partitions).\n",
      Ddi_BddPartNum(part_from)););

  for (i = 0; i < Ddi_BddPartNum(part_from); i++) {
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "Part[%d].\n", i););
    from_i = Ddi_BddPartRead(part_from, i);
    to_i = TrImgConjPartTr(trMgr, TR, from_i, constrain, psv, nsv, smoothV);
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "|To[%d]|=%d.\n", i, Ddi_BddSize(to_i))
      );
    Ddi_BddPartInsertLast(to, to_i);
    Ddi_Free(to_i);
  }

  return (to);
}

/**Function********************************************************************

  Synopsis    [Compute approx image of a conjunctively partitioned transition
    relation.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static Ddi_Bdd_t *
TrImgApproxConjPartTr(
  Tr_Mgr_t * trMgr /* Tr manager */ ,
  Ddi_Bdd_t * TR /* Partitioned TR */ ,
  Ddi_Bdd_t * from /* domain set */ ,
  Ddi_Bdd_t * constrain /* range constrain */ ,
  Ddi_Vararray_t * psv /* Array of present state variables */ ,
  Ddi_Vararray_t * nsv /* Array of next state variables */ ,
  Ddi_Varset_t * smoothV        /* Variables to be abstracted */
)
{
  Ddi_Bdd_t *to, *to_i, *TR_i;
  int i, j, k, iter, np, *imgStats, again, *abortDone;
  Ddi_Mgr_t *ddR = Ddi_ReadMgr(from);   /* dd manager */

  if (!Ddi_BddIsPartConj(TR)) {
    Pdtutil_Warning(1, "No conjunctive TR for approx img - exact image done.");
    return (TrImgConjPartTr(trMgr, TR, from, constrain, psv, nsv, smoothV));
  }

  if (Ddi_BddIsMeta(from)) {
    to = Ddi_BddMakeConst(ddR, 1);
    Ddi_BddSetMeta(to);
  } else {
    to = Ddi_BddMakePartConjVoid(ddR);
  }
  np = Ddi_BddPartNum(TR);
  abortDone = Pdtutil_Alloc(int,
    np
  );

  for (i = 0; i < np; i++) {
    TR_i = Ddi_BddDup(Ddi_BddPartRead(TR, i));
    to_i =
      TrImgApproxImgAux(trMgr, TR_i, from, psv, nsv, smoothV, &abortDone[i]);
    Ddi_Free(TR_i);
    if (Ddi_BddIsMeta(to)) {
      Ddi_BddAndAcc(to, to_i);
    } else {
      Ddi_BddPartInsert(to, i, to_i);
    }
    Ddi_Free(to_i);
  }

  Pdtutil_Assert(Ddi_BddIsMeta(to) || (Ddi_BddPartNum(to) == np),
    "Wrong num of partitions in approx img");

  for (iter = 1, again = 1;
    again && (iter < trMgr->settings.image.approxMaxIter); iter++) {

    np = Ddi_BddPartNum(TR);
    imgStats = Pdtutil_Alloc(int,
      np
    );

    again = 0;

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "Approx iter %d.\n", iter + 1)
      );

    for (i = np - 1; i >= 0; i--) {
      int size;

      to_i = Ddi_BddPartRead(to, i);
      size = Ddi_BddSize(to_i);
      if (abortDone[i]) {
        imgStats[i] = 1;
      } else if (Ddi_BddIsOne(Ddi_BddPartRead(TR, i))) {
        imgStats[i] = 0;
      } else if ((trMgr->settings.image.approxMinSize >= 0) &&
        (size < trMgr->settings.image.approxMinSize)) {
        imgStats[i] = -1;
      } else if ((trMgr->settings.image.approxMaxSize >= 0) &&
        (size > trMgr->settings.image.approxMaxSize)) {
        imgStats[i] = 1;
      } else {
        imgStats[i] = 0;
      }
    }

    /* try joins */
    for (i = 0; i < np; i++) {
      to_i = Ddi_BddPartRead(to, i);
      if (imgStats[i] < 0) {
        for (j = i + 1; j < np; j++) {
          if (imgStats[j] < 0) {
            Ddi_Bdd_t *p0 = Ddi_BddDup(Ddi_BddPartRead(TR, i));
            Ddi_Bdd_t *p1 = Ddi_BddPartRead(TR, j);

            Ddi_BddSetPartConj(p0);
            Ddi_BddSetPartConj(p1);
            for (k = 0; k < Ddi_BddPartNum(p1); k++) {
              Ddi_BddPartInsertLast(p0, Ddi_BddPartRead(p1, k));
            }
            TR_i = Ddi_BddDup(p0);
            to_i = TrImgApproxImgAux(trMgr, TR_i,
              from, psv, nsv, smoothV, &abortDone[i]);
            Ddi_Free(TR_i);
            if ((Ddi_BddSize(to_i) < trMgr->settings.image.approxMaxSize) ||
              (trMgr->settings.image.approxMaxSize < 0)) {
              Ddi_BddPartWrite(TR, j, Ddi_MgrReadOne(ddR));
              Ddi_BddPartWrite(to, j, Ddi_MgrReadOne(ddR));
              Ddi_BddPartWrite(TR, i, p0);
              Ddi_BddPartWrite(to, i, to_i);
              imgStats[j] = 0;
              Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
                fprintf(stdout, "*")
                );
              Ddi_Free(p0);
              Ddi_Free(to_i);
              again = 1;
              break;
            }
            Ddi_Free(to_i);
            Ddi_Free(p0);
          }
        }
      }
    }

    for (i = np - 1; i >= 0; i--) {
      if (Ddi_BddIsOne(Ddi_BddPartRead(TR, i))) {
        Ddi_Bdd_t *tmp = Ddi_BddPartExtract(TR, i);

        Ddi_Free(tmp);
        tmp = Ddi_BddPartExtract(to, i);
        Pdtutil_Assert(Ddi_BddIsOne(tmp), "missing one partition");
        Ddi_Free(tmp);
        np--;
        continue;
      } else if (imgStats[i] > 0) {
        /* split */
        Ddi_Bdd_t *p0 = Ddi_BddPartRead(TR, i);

        if (Ddi_BddIsPartConj(p0) && (Ddi_BddPartNum(p0) > 1)) {
          Ddi_Bdd_t *p1 = Ddi_BddMakePartConjVoid(ddR);

          for (k = Ddi_BddPartNum(p0) / 2; k < Ddi_BddPartNum(p0); k++) {
            Ddi_Bdd_t *tmp = Ddi_BddPartExtract(p0, k);

            Ddi_BddPartInsertLast(p1, tmp);
            Ddi_Free(tmp);
          }
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "{")
            );
          Ddi_BddPartInsert(TR, i + 1, p1);
          np++;
          TR_i = Ddi_BddDup(p0);
          to_i = TrImgApproxImgAux(trMgr, TR_i,
            from, psv, nsv, smoothV, &abortDone[i]);
          Ddi_Free(TR_i);
          Ddi_BddPartWrite(to, i, to_i);
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "+")
            );
          Ddi_Free(to_i);
          TR_i = Ddi_BddDup(p1);
          to_i = TrImgApproxImgAux(trMgr, TR_i,
            from, psv, nsv, smoothV, &abortDone[i]);
          Ddi_Free(TR_i);
          Ddi_BddPartInsert(to, i + 1, to_i);
          Ddi_Free(to_i);
          Ddi_Free(p1);
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "}")
            );
          again = 1;
        }
      }
    }

    Pdtutil_Free(imgStats);
    Pdtutil_Free(abortDone);
    abortDone = Pdtutil_Alloc(int,
      np
    );

    for (i = np - 1; i >= 0; i--) {
      abortDone[i] = 0;
    }

  }

  Pdtutil_Free(abortDone);

  Ddi_BddSetClustered(to, trMgr->settings.image.clusterThreshold);

  return (to);
}

/**Function********************************************************************

  Synopsis    [Compute approx image of a transition relation partition.]
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static Ddi_Bdd_t *
TrImgApproxImgAux(
  Tr_Mgr_t * trMgr /* Tr manager */ ,
  Ddi_Bdd_t * TR_i /* TR partition */ ,
  Ddi_Bdd_t * from /* domain set */ ,
  Ddi_Vararray_t * psv /* Array of present state variables */ ,
  Ddi_Vararray_t * nsv /* Array of next state variables */ ,
  Ddi_Varset_t * smoothV /* Variables to be abstracted */ ,
  int *abortDoneP
)
{
  Ddi_Bdd_t *to_i = NULL;
  int j;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(from);   /* dd manager */

  *abortDoneP = 0;
  Ddi_BddSetPartConj(TR_i);
  if (Ddi_BddIsPartConj(from)) {
    int j;
    Ddi_Varset_t *suppTR = Ddi_BddSupp(TR_i);
    Ddi_Bdd_t *myFrom = Ddi_BddMakePartConjVoid(ddm);

    for (j = 0; j < Ddi_BddPartNum(from); j++) {
      Ddi_Varset_t *supp;
      Ddi_Bdd_t *p = Ddi_BddPartExtract(from, j);

      supp = Ddi_BddSupp(from);
      Ddi_BddPartInsert(from, j, p);
      Ddi_VarsetUnionAcc(supp, suppTR);
#if 1
      {
        /* int s = Ddi_BddSize(p); */
        Ddi_BddExistProjectAcc(p, supp);
        /*
           if (Ddi_BddSize(p) < s) {
           Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
           fprintf(stdout, "{%d->%d}", s, Ddi_BddSize(p))
           );
           }
         */
      }
#endif
      if (!Ddi_BddIsOne(p)) {
        Ddi_BddPartInsertLast(myFrom, p);
      }
      Ddi_Free(p);
      Ddi_Free(supp);
    }
    Ddi_Free(suppTR);
    j = Ddi_BddPartNum(from) - Ddi_BddPartNum(myFrom);
    if (j > 0) {
      /*printf("{%d}",j); */
    }
    if (Ddi_BddPartNum(myFrom) == 0) {
      Ddi_Free(myFrom);
      myFrom = Ddi_BddMakeConst(ddm, 1);
    } else {
      Ddi_BddRestrictAcc(TR_i, myFrom);
#if !APPROX_BY_RESTRICT
      Ddi_BddPartInsert(TR_i, 0, myFrom);
#endif
    }
    /*to_i = TrImgConjPartTr (trMgr,TR_i,myFrom,constrain,psv,nsv,smoothV); */
    Ddi_Free(myFrom);
  } else if (Ddi_BddIsPartDisj(from)) {
    to_i = Ddi_BddMakeConst(ddm, 0);
    for (j = 0; j < Ddi_BddPartNum(from); j++) {
      Ddi_Bdd_t *to_i_j =
        TrImgApproxImgAux(trMgr, TR_i, Ddi_BddPartRead(from, j),
        psv, nsv, smoothV, abortDoneP);

      Ddi_BddOrAcc(to_i, to_i_j);
      Ddi_Free(to_i_j);
    }
  } else if (Ddi_BddIsMeta(from)) {
    Ddi_BddPartInsert(TR_i, 0, from);
  } else {
    Ddi_BddRestrictAcc(TR_i, from);
#if !APPROX_BY_RESTRICT
    Ddi_BddPartInsert(TR_i, 0, from);
#endif
    /*to_i = TrImgConjPartTr (trMgr,TR_i,from,constrain,psv,nsv,smoothV); */
  }
  if (!Ddi_BddIsPartDisj(from)) {
    if (!Ddi_BddIsMeta(from)) {
#if !APPROX_BY_RESTRICT
      Ddi_Bdd_t *overApprox = Ddi_BddDup(Ddi_BddPartRead(TR_i, 0));
#else
      Ddi_Bdd_t *overApprox = Ddi_BddMakeConst(ddm, 1);
#endif
      to_i = Ddi_BddMultiwayRecursiveAndExistOver(TR_i, smoothV,
        NULL, overApprox, trMgr->settings.image.approxMaxSize * 100);
      Ddi_Free(overApprox);
    } else {
      to_i = Ddi_BddExist(TR_i, smoothV);
    }
  }
  if (Ddi_BddIsPartConj(to_i)) {
    *abortDoneP = 1;
    for (j = 0; j < Ddi_BddPartNum(to_i); j++) {
      Ddi_BddExistAcc(Ddi_BddPartRead(to_i, j), smoothV);
    }
  } else {
    int size = Ddi_BddSize(to_i);

    Ddi_BddExistAcc(to_i, smoothV);
    if (size != Ddi_BddSize(to_i)) {
      *abortDoneP = 1;
    }
  }
  Ddi_BddSubstVarsAcc(to_i, nsv, psv);
  if (Ddi_BddIsPartConj(to_i)) {
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "[%d:", Ddi_BddPartNum(to_i)); Ddi_BddSetMono(to_i);
      fprintf(stdout, "%d]", Ddi_BddSize(to_i));
      );
  } else {
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "[%d]", Ddi_BddSize(to_i));
      );
  }

  return (to_i);

}


/**Function********************************************************************

  Synopsis    [Compute image of a clocked transition relation.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static Ddi_Bdd_t *
TrImgClockedTr(
  Tr_Tr_t * TR /* Partitioned TR */ ,
  Ddi_Bdd_t * from /* domain set */ ,
  Ddi_Bdd_t * constrain         /* range constrain */
)
{
  int i;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(Tr_TrBdd(TR));   /* TR DD manager */
  Ddi_Var_t *clk = Tr_TrClk(TR);
  Ddi_Bdd_t *trBdd_i, *trBddPhase, *from_i, *to_i, *clk_lit;
  Ddi_Bdd_t *to = NULL, *trBdd;
  Tr_Mgr_t *trMgr = Tr_TrMgr(TR);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Clocked Image.\n")
    );

  trBdd = Tr_TrBdd(TR);

  to = Ddi_BddMakePartDisjVoid(ddm);
  for (i = 0; i < 2; i++) {
    Pdtutil_Assert(Ddi_BddIsPartDisj(trBdd), "Wrong format for clocked TR");
    trBdd_i = Ddi_BddPartRead(trBdd, i);
    Pdtutil_Assert(Ddi_BddIsPartConj(trBdd_i), "Wrong format for clocked TR");
    Pdtutil_Assert(Ddi_BddPartNum(trBdd_i) == 2,
      "Wrong format for clocked TR");
    trBddPhase = Ddi_BddPartRead(trBdd_i, 1);

    from_i = Ddi_BddCofactor(from, clk, i);
    Ddi_BddSetFlattened(from_i);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Clocked part %d.\n", i)
      );

    if (Ddi_BddIsZero(from_i)) {
      to_i = Ddi_BddMakeConst(ddm, 0);
    } else {
      Tr_Tr_t *TR_i = Tr_TrMakeFromRel(trMgr, trBddPhase);

      to_i = Tr_ImgWithConstrain(TR_i, from_i, constrain);
      if (!Ddi_BddEqual(trBddPhase, Tr_TrBdd(TR_i))) {
        Ddi_BddPartWrite(trBdd_i, 1, Tr_TrBdd(TR_i));
      }

      Tr_TrFree(TR_i);
    }

    clk_lit = Ddi_BddMakeLiteral(clk, 1 - i);
    if (Ddi_BddIsMeta(to_i)) {
      Ddi_BddAndAcc(to_i, clk_lit);
    } else {
      Ddi_BddSetPartConj(to_i);
      Ddi_BddPartInsert(to_i, 0, clk_lit);
    }
    Ddi_BddPartWrite(to, 1 - i, to_i);

    Ddi_Free(clk_lit);
    Ddi_Free(from_i);
    Ddi_Free(to_i);
  }

  if (Ddi_BddIsZero(Ddi_BddPartRead(to, 0))) {
    Ddi_Bdd_t *tmp = to;

    to = Ddi_BddPartExtract(tmp, 1);
    Ddi_Free(tmp);
  } else if (Ddi_BddIsZero(Ddi_BddPartRead(to, 0))) {
    Ddi_Bdd_t *tmp = to;

    to = Ddi_BddPartExtract(tmp, 1);
    Ddi_Free(tmp);
  }

  if (Ddi_BddIsMeta(from)) {
    if (Ddi_BddIsPartDisj(to)) {
#if 1
      Ddi_Bdd_t *tmp1 = Ddi_BddPartExtract(to, 1);
      Ddi_Bdd_t *tmp0 = Ddi_BddPartExtract(to, 0);

      Ddi_Free(to);
      if (!Ddi_BddIsMeta(tmp0)) {
        Ddi_BddSetMono(tmp0);
      }
      if (!Ddi_BddIsMeta(tmp1)) {
        Ddi_BddSetMono(tmp1);
      }
      to = Ddi_BddOr(tmp0, tmp1);
      Ddi_Free(tmp0);
      Ddi_Free(tmp1);
#endif
    }
  } else {
    Ddi_BddSetMono(to);
  }

  return (to);
}

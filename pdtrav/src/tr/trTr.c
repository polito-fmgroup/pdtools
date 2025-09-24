/**CFile***********************************************************************

  FileName    [trTr.c]

  PackageName [tr]

  Synopsis    [Functions to handle a TR object]

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

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

typedef struct {
  Ddi_Varset_t *vars;
  int nFrom;
  int *from;
  int nTo;
  int *to;
} sccNode_t;

typedef struct {
  int nEdges;
  int nNodes;
  sccNode_t *nodes;
} sccGraph_t;

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static Tr_Tr_t *trManualPartitionsIds(
  Tr_Tr_t * tr,
  char *file
);
static Ddi_Varsetarray_t *computeCoiVars(
  Tr_Tr_t * tr,
  Ddi_Bdd_t * p,
  int maxDepth
);
static void storeDependancyGraph(
  char *fname,
  Tr_Tr_t * tr,
  Ddi_Varset_t * core
);
static sccGraph_t *loadSCCGraph(
  char *fname,
  Tr_Tr_t * tr
);
static Ddi_Varset_t *suppWithAuxVars(
  Ddi_Bdd_t * f,
  Tr_Mgr_t * trMgr
);
static Ddi_Bdd_t *bddAndInterleaved(
  Ddi_Bdd_t * f,
  Ddi_Bdd_t * g
);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Create a TR from relation.]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Tr_TrMakeFromRel(
  Tr_Mgr_t * trMgr,
  Ddi_Bdd_t * bdd
)
{
  Tr_Tr_t *tr;

  tr = TrAlloc(trMgr);
  tr->ddiTr = Ddi_ExprMakeFromBdd(bdd);
  Ddi_Lock(tr->ddiTr);

  return (tr);
}

/**Function********************************************************************
  Synopsis    [Create a TR from expression.]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Tr_TrMakeFromExpr(
  Tr_Mgr_t * trMgr,
  Ddi_Expr_t * expr
)
{
  Tr_Tr_t *tr;

  tr = TrAlloc(trMgr);
  tr->ddiTr = Ddi_ExprDup(expr);
  Ddi_Lock(tr->ddiTr);

  return (tr);
}

/**Function********************************************************************
  Synopsis    [Create a conjunctively partitioned TR from array of functions.]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Tr_TrMakePartConjFromFuns(
  Tr_Mgr_t * trMgr,
  Ddi_Bddarray_t * Fa,
  Ddi_Vararray_t * Va
)
{
  Tr_Tr_t *tr;
  Ddi_Bdd_t *rel;

  rel = Ddi_BddRelMakeFromArray(Fa, Va);
  tr = Tr_TrMakeFromRel(trMgr, rel);
  Ddi_Free(rel);
  tr->bitTr = 1;

  return (tr);
}

/**Function********************************************************************
  Synopsis    [Create a conjunctively partitioned TR from array of functions.]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Tr_TrMakePartConjFromFunsWithAuxVars(
  Tr_Mgr_t * trMgr,
  Ddi_Bddarray_t * Fa,
  Ddi_Vararray_t * Va,
  Ddi_Bddarray_t * auxFa,
  Ddi_Vararray_t * auxVa
)
{
  Tr_Tr_t *tr;
  Ddi_Bdd_t *rel, *auxrel;
  int i;

  rel = Ddi_BddRelMakeFromArray(Fa, Va);
  if (auxFa != NULL) {
    auxrel = Ddi_BddRelMakeFromArray(auxFa, auxVa);
    for (i = 0; i < Ddi_BddPartNum(auxrel); i++) {
      Ddi_BddPartInsertLast(rel, Ddi_BddPartRead(auxrel, i));
    }
    Ddi_Free(auxrel);
  }
  tr = Tr_TrMakeFromRel(trMgr, rel);
  Ddi_Free(rel);
  tr->bitTr = 1;

  return (tr);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Tr_TrAuxVars(
  Tr_Tr_t * tr,
  char *filename
)
{
  FILE *fp;
  char varname[200], buf[200];
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(Tr_TrBdd(tr));
  Ddi_Bdd_t *tr_0;

  fp = fopen(filename, "r");
  if (fp == NULL) {
    return tr;
  }
  tr_0 = Ddi_BddMakeConst(ddiMgr, 1);
  while (fscanf(fp, "%s", varname) == 1) {
    Ddi_Bdd_t *aux, *aux1;
    Ddi_Var_t *v = Ddi_VarFromName(ddiMgr, varname);

    if (v != NULL) {
      Ddi_Var_t *vs, *vy;

      vy = Ddi_VarNewAfterVar(v);
      sprintf(buf, "%s$AUX$NS", varname);
      Ddi_VarAttachName(vy, buf);
      vs = Ddi_VarNewAfterVar(v);

      Ddi_VarAttachName(vs, buf);
      Ddi_VarMakeGroup(ddiMgr, vs, 2);

      aux = Ddi_BddMakeLiteral(v, 1);
      aux1 = Ddi_BddMakeLiteral(vs, 1);
      Ddi_BddXnorAcc(aux, aux1);
      Ddi_Free(aux1);
      Ddi_BddAndAcc(tr_0, aux);
      Ddi_Free(aux);
    }
  }

  if (Ddi_BddIsPartConj(Tr_TrBdd(tr))) {
    Ddi_BddPartInsert(Tr_TrBdd(tr), 0, tr_0);
  } else if (Ddi_BddIsPartDisj(Tr_TrBdd(tr))) {
    Ddi_BddPartInsert(Ddi_BddPartRead(Tr_TrBdd(tr), 0), 1, tr_0);
    Ddi_BddPartInsert(Ddi_BddPartRead(Tr_TrBdd(tr), 1), 1, tr_0);
  } else {
    Ddi_BddAndAcc(Tr_TrBdd(tr), tr_0);
  }

  Ddi_Free(tr_0);
  return tr;
}

/**Function********************************************************************
  Synopsis    [Reduce tr from partitioned/clustered to monolithic.]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Tr_TrSetMono(
  Tr_Tr_t * tr
)
{
  Ddi_Varset_t *qvars;

  if (tr->trMgr->settings.cluster.smoothPi) {
    qvars = Ddi_VarsetMakeFromArray(tr->trMgr->i);
    Ddi_BddExistAcc(Tr_TrBdd(tr), qvars);
    Ddi_Free(qvars);
  } else {
    Ddi_BddSetMono(Tr_TrBdd(tr));
  }
  return (tr);
}

/**Function********************************************************************
  Synopsis    [Transform tr to clustered form.]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Tr_TrSetClustered(
  Tr_Tr_t * tr
)
{
  Tr_Mgr_t *trMgr;
  int threshold, i;
  Ddi_Varset_t *qvars;          /* Set of quantifying variables */
  Ddi_Bdd_t *rel, *oldRel;
  long trTime;

  trMgr = tr->trMgr;
  threshold = trMgr->settings.cluster.threshold;

  trTime = util_cpu_time();

  /* no clustering done if threshold <= 0 */
  if (threshold > 0) {

    if (trMgr->settings.cluster.smoothPi) {
      qvars = Ddi_VarsetMakeFromArray(trMgr->i);
    } else {
      qvars = Ddi_VarsetVoid(trMgr->DdiMgr);
    }
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "TR Clustering: ");
      fprintf(stdout, "|q|=%d; ", Ddi_VarsetNum(qvars));
      fprintf(stdout, "Threshold=%d.\n", threshold)
      );

    oldRel = Tr_TrBdd(tr);

    if (Ddi_BddIsPartDisj(oldRel)) {
      rel = Ddi_BddDup(oldRel);
      for (i = 0; i < Ddi_BddPartNum(oldRel); i++) {
        Ddi_Bdd_t *p = Ddi_BddPartRead(oldRel, i);

        if (Ddi_BddIsPartConj(p)) {
          Ddi_Bdd_t *clk_p = NULL;
          Ddi_Bdd_t *newP;

          if (tr->clk != NULL) {
            clk_p = Ddi_BddPartExtract(p, 0);
          }
          newP = Part_BddMultiwayLinearAndExist(p, qvars, threshold);
          Ddi_BddSetPartConj(newP);
          if (clk_p != NULL) {
            Ddi_BddPartInsert(newP, 0, clk_p);
            Ddi_Free(clk_p);
          }
          Ddi_BddPartWrite(rel, i, newP);
          Ddi_Free(newP);
        }
      }
    } else {

      if (Ddi_BddIsPartConj(oldRel)) {
        int j;

        for (j = 0; j < Ddi_BddPartNum(oldRel); j++) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(oldRel, j);

          if (Ddi_BddIsPartConj(p)) {
            Ddi_Bdd_t *newP = Part_BddMultiwayLinearAndExist(p,
              qvars, threshold);

            Ddi_BddPartWrite(oldRel, j, newP);
            Ddi_Free(newP);
          }
        }
      }
#if 1
      rel = Part_BddMultiwayLinearAndExist(oldRel, qvars, threshold);
#else
#if 0
      rel = Part_BddMultiwayRecursiveAndExist(oldRel,
        qvars, Ddi_MgrReadOne(trMgr->DdiMgr), threshold);
#else
      {
        Ddi_Varset_t *rvars = Ddi_VarsetMakeFromArray(trMgr->ns);

        rel = Part_BddMultiwayRecursiveTreeAndExist(oldRel,
          qvars, rvars, threshold);
        Ddi_Free(rvars);
      }
#endif
#endif

      Ddi_BddSetPartConj(rel);

      if ((Ddi_BddPartNum(rel) == 1)
        && (Ddi_BddIsMeta(Ddi_BddPartRead(rel, 0)))) {
        Ddi_Bdd_t *p;

        p = Ddi_BddPartExtract(rel, 0);
        Ddi_BddSetPartConjFromMeta(p);
        Ddi_Free(rel);
        rel = p;
        Ddi_MetaQuit(trMgr->DdiMgr);
      }
    }

    TrTrRelWrite(tr, rel);
    Ddi_Free(rel);
    Ddi_Free(qvars);
  }

  for (i = 0; i < Ddi_BddPartNum(Tr_TrBdd(tr)); i++) {
    char name[PDTUTIL_MAX_STR_LEN];

    sprintf(name, "TR_CLUSTER[%d]", i);
    Ddi_SetName(Ddi_BddPartRead(Tr_TrBdd(tr), i), name);
  }

  trTime = util_cpu_time() - trTime;
  trMgr->trTime += trTime;

  return (tr);
}

/**Function********************************************************************
  Synopsis    [Transform tr to clustered form.]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Tr_TrSetClusteredApprGroups(
  Tr_Tr_t * tr
)
{
  Tr_Mgr_t *trMgr;
  int threshold, i, j;
  Ddi_Varset_t *qvars;          /* Set of quantifying variables */
  Ddi_Bdd_t *rel, *oldRel;
  long trTime;

  trMgr = tr->trMgr;
  threshold = trMgr->settings.cluster.threshold;

  trTime = util_cpu_time();

  /* no clustering done if threshold <= 0 */
  if (threshold > 0) {

    if (trMgr->settings.cluster.smoothPi) {
      qvars = Ddi_VarsetMakeFromArray(trMgr->i);
    } else {
      qvars = Ddi_VarsetVoid(trMgr->DdiMgr);
    }
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "TR Clustering: ");
      fprintf(stdout, "|q|=%d; ", Ddi_VarsetNum(qvars));
      fprintf(stdout, "Threshold=%d.\n", threshold)
      );

    oldRel = Tr_TrBdd(tr);

    Pdtutil_Assert(Ddi_BddIsPartConj(oldRel), "conj tr expected");
    int j;

    for (j = 0; j < Ddi_BddPartNum(oldRel); j++) {
      Ddi_Bdd_t *p = Ddi_BddPartRead(oldRel, j);

      if (Ddi_BddIsPartConj(p)) {
        Ddi_Bdd_t *newP = Part_BddMultiwayLinearAndExist(p,
          qvars, threshold);

        Ddi_BddPartWrite(oldRel, j, newP);
        Ddi_Free(newP);
      }
    }

    Ddi_Free(qvars);
  }

  for (i = 0; i < Ddi_BddPartNum(Tr_TrBdd(tr)); i++) {
    char name[PDTUTIL_MAX_STR_LEN];

    sprintf(name, "TR_CLUSTER[%d]", i);
    Ddi_SetName(Ddi_BddPartRead(Tr_TrBdd(tr), i), name);
  }

  trTime = util_cpu_time() - trTime;
  trMgr->trTime += trTime;

  return (tr);
}


/**Function********************************************************************
  Synopsis    [Transform tr to clustered form.]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Tr_TrGroupForApprox(
  Tr_Tr_t * tr,
  Ddi_Bdd_t * clock_tr,
  int threshold /* group size threshold */ ,
  int apprOverlap               /* enable overlapping partitions */
)
{
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Ddi_Bdd_t *group = 0, *p;
  Ddi_Varset_t *suppGroup;
  Ddi_Vararray_t *ps = Tr_MgrReadPS(trMgr);
  int i, j;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(Tr_TrBdd(tr));
  int nsupp, nv = Ddi_VararrayNum(ps) + Ddi_VararrayNum(Tr_MgrReadI(trMgr));

  trMgr = tr->trMgr;

  if (!Ddi_BddIsPartConj(Tr_TrBdd(tr))) {
    return (tr);
  }

  for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(tr));) {
    group = Ddi_BddPartRead(Tr_TrBdd(tr), j);
    Ddi_BddSetPartConj(group);
    p = Ddi_BddPartRead(Tr_TrBdd(tr), i);
    Ddi_BddPartInsertLast(group, p);
    suppGroup = Ddi_BddSupp(group);
    nsupp = Ddi_VarsetNum(suppGroup);
    Ddi_Free(suppGroup);
    if ((Ddi_BddSize(group) * nsupp) / nv > threshold) {
      /* resume from group insert */
      p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
      Ddi_Free(p);
      i++;
      j++;
      if (clock_tr != NULL) {
        Ddi_BddPartInsert(group, 0, clock_tr);
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c, fprintf(stdout, "#")
          );
      }
    } else {
      p = Ddi_BddPartExtract(Tr_TrBdd(tr), i);
      Ddi_Free(p);
    }
  }
  if (clock_tr != NULL) {
    Ddi_BddPartInsert(group, 0, clock_tr);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "#")
      );
  }

  if (apprOverlap) {
    Ddi_Bdd_t *group0, *group1, *overlap, *p;
    int np;

    for (i = Ddi_BddPartNum(Tr_TrBdd(tr)) - 1; i > 0; i--) {
      group0 = Ddi_BddPartRead(Tr_TrBdd(tr), i - 1);
      group1 = Ddi_BddPartRead(Tr_TrBdd(tr), i);
      overlap = Ddi_BddMakePartConjVoid(ddm);
      if (Ddi_BddIsPartConj(group0)) {
        np = Ddi_BddPartNum(group0);
        for (j = np / 2; j < np; j++) {
          p = Ddi_BddPartRead(group0, j);
          Ddi_BddPartInsertLast(overlap, p);
        }
      } else {
        Ddi_BddPartInsertLast(overlap, group0);
      }
      if (Ddi_BddIsPartConj(group1)) {
        np = Ddi_BddPartNum(group1);
        for (j = 0; j < (np + 1) / 2; j++) {
          p = Ddi_BddPartRead(group1, j);
          Ddi_BddPartInsertLast(overlap, p);
        }
      } else {
        Ddi_BddPartInsertLast(overlap, group1);
      }
      Ddi_BddPartInsert(Tr_TrBdd(tr), i, overlap);
      Ddi_Free(overlap);
    }
  }

  return (tr);
}


/**Function********************************************************************
  Synopsis    [Duplicate a TR]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Tr_TrDup(
  Tr_Tr_t * old
)
{
  Tr_Tr_t *newt;

  newt = TrAlloc(old->trMgr);
  newt->ddiTr = Ddi_ExprDup(old->ddiTr);
  newt->clk = old->clk;
  if (old->clkTr == NULL) {
    newt->clkTr = NULL;
  } else {
    newt->clkTr = Ddi_BddDup(old->clkTr);
  }
  newt->clkTrFormat = old->clkTrFormat;
  newt->bitTr = old->bitTr;
  Ddi_Lock(newt->ddiTr);
  Ddi_Lock(newt->clkTr);

  return (newt);
}

/**Function********************************************************************
  Synopsis    [Return tr Manager]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Tr_Mgr_t *
Tr_TrMgr(
  Tr_Tr_t * tr
)
{
  return (tr->trMgr);
}

/**Function********************************************************************
  Synopsis    [Return Bdd relation field]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Tr_TrBdd(
  Tr_Tr_t * tr
)
{
  return (Ddi_ExprToBdd(tr->ddiTr));
}

/**Function********************************************************************
  Synopsis    [Return Bdd relation field]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Var_t *
Tr_TrClk(
  Tr_Tr_t * tr
)
{
  return (tr->clk);
}

/**Function********************************************************************
  Synopsis    [Return Bdd relation field]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Tr_TrClkTr(
  Tr_Tr_t * tr
)
{
  return (tr->clkTr);
}

/**Function********************************************************************
  Synopsis    [Release a TR.]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Tr_TrFree(
  Tr_Tr_t * tr
)
{

  if (tr == NULL) {
    return;
  }
  Ddi_Unlock(tr->ddiTr);
  Ddi_Free(tr->ddiTr);
  if (tr->prev != NULL) {
    tr->prev->next = tr->next;
  } else {
    tr->trMgr->trList = tr->next;
  }
  if (tr->next != NULL) {
    tr->next->prev = tr->prev;
  }
  if (tr->clk != NULL) {
    Ddi_Unlock(tr->clkTr);
    Ddi_Free(tr->clkTr);
  }
  Pdtutil_Free(tr);
}


/**Function********************************************************************
  Synopsis    [Reverse a TR by swapping present/next state variables]
  Description [Reverse a TR by swapping present/next state variables]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Tr_TrReverse(
  Tr_Tr_t * old
)
{
  Tr_Tr_t *newt;

  newt = Tr_TrDup(old);
  Tr_TrReverseAcc(newt);

  return (newt);
}

/**Function********************************************************************
  Synopsis    [Reverse a TR by swapping present/next state variables]
  Description [Reverse a TR by swapping present/next state variables]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Tr_TrReverseAcc(
  Tr_Tr_t * tr
)
{
  Ddi_BddSwapVarsAcc(Tr_TrBdd(tr), tr->trMgr->ps, tr->trMgr->ns);

  if (tr->clk != 0) {
    int i;

    for (i = 0; i < Ddi_VararrayNum(tr->trMgr->ps); i++) {
      if (tr->clk == Ddi_VararrayRead(tr->trMgr->ps, i)) {
        tr->clk = Ddi_VararrayRead(tr->trMgr->ns, i);
        break;
      } else if (tr->clk == Ddi_VararrayRead(tr->trMgr->ns, i)) {
        tr->clk = Ddi_VararrayRead(tr->trMgr->ps, i);
        break;
      }
    }
    if (Tr_TrClkTr(tr) != NULL) {
      Ddi_BddSwapVarsAcc(Tr_TrClkTr(tr), tr->trMgr->ps, tr->trMgr->ns);
    }
  }

  return (tr);
}

/**Function********************************************************************
  Synopsis    [insert a TR in TR manager list.]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/

Tr_Tr_t *
TrAlloc(
  Tr_Mgr_t * trMgr
)
{
  Tr_Tr_t *tr;

  tr = Pdtutil_Alloc(Tr_Tr_t, 1);
  tr->trMgr = trMgr;
  tr->ddiTr = NULL;
  tr->clk = NULL;
  tr->clkTr = NULL;
  tr->clkTrFormat = 0;
  tr->bitTr = 0;
  tr->prev = NULL;
  tr->next = trMgr->trList;
  if (trMgr->trList != NULL) {
    trMgr->trList->prev = tr;
  }
  trMgr->trList = tr;

  return (tr);
}

/**Function********************************************************************
  Synopsis    [Write (replace) Bdd relation.]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
TrTrRelWrite(
  Tr_Tr_t * tr,
  Ddi_Bdd_t * bdd
)
{
  Ddi_Unlock(tr->ddiTr);
  Ddi_Free(tr->ddiTr);
  tr->ddiTr = Ddi_ExprMakeFromBdd(bdd);
  Ddi_Lock(tr->ddiTr);

  return (tr);
}



/**Function********************************************************************
  Synopsis    [Disjunctive Partition Transition Relation according to variable]
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/

Ddi_Var_t *
Tr_TrSetClkStateVar(
  Tr_Tr_t * tr,
  char *varname,
  int toggle
)
{
  Ddi_Bdd_t *trBdd, *split, *splitPS, *splitNS, *constr;
  char buf[PDTUTIL_MAX_STR_LEN];
  Ddi_Var_t *v = NULL, *vy = NULL, *vs = NULL;
  Ddi_Vararray_t *va;
  Ddi_Varset_t *pi;
  Tr_Mgr_t *trMgr = tr->trMgr;
  Ddi_Mgr_t *dd = Ddi_ReadMgr(Tr_TrBdd(tr));

  if ((varname == NULL) || (strcmp(varname, "NULL") == 0)) {
    Pdtutil_Warning(1, "Null clock variable.");
    return (NULL);
  }

  /* Get Split Variable */
  v = Ddi_VarFromName(dd, varname);

  if (v == NULL) {
    fprintf(stderr, "Variable %s not found - no clock located.\n", varname);
    return (NULL);
  }

  pi = Ddi_VarsetMakeFromArray(Tr_MgrReadI(trMgr));

  trBdd = Tr_TrBdd(tr);

  if (Ddi_VarInVarset(pi, v)) {
    /* Create Present (vs) and Next (vy) State Variable for the Clock ...
       with Names */
    vy = Ddi_VarNewAfterVar(v);
    sprintf(buf, "%s$LATCH$NS", varname);
    Ddi_VarAttachName(vy, buf);
    vs = Ddi_VarNewAfterVar(v);
    sprintf(buf, "%s$LATCH", varname);
    Ddi_VarAttachName(vs, buf);

    /* Group Present and Next State Variable for Clock */
    Ddi_VarMakeGroup(dd, v, 3);

    /* Put Present/Next For Clock into Present/Next Variables */
    va = Tr_MgrReadPS(trMgr);
    Ddi_VararrayWrite(va, Ddi_VararrayNum(va), vs);
    va = Tr_MgrReadNS(trMgr);
    Ddi_VararrayWrite(va, Ddi_VararrayNum(va), vy);

#if 0
    /* Put Next Clock into AuxVar */
    /* GpC: disabled - was used just to count states */
    va = Ddi_VararrayAlloc(dd, 1);
    Ddi_VararrayWrite(va, 0, vs);
    Tr_MgrSetAuxVars(trMgr, va);
    Ddi_Free(va);
#endif

    /*
     *  Gen clock TR component
     */

    split = Ddi_BddMakeLiteral(v, 1);
    splitNS = Ddi_BddMakeLiteral(vy, 1);
    constr = Ddi_BddXnor(split, splitNS);

    {
      Ddi_Vararray_t *varray = Ddi_VararrayAlloc(dd, 1);
      Ddi_Bddarray_t *barray = Ddi_BddarrayAlloc(dd, 1);

      Ddi_VararrayWrite(varray, 0, v);
      Ddi_BddarrayWrite(barray, 0, splitNS);

      Ddi_BddComposeAcc(trBdd, varray, barray);

      Ddi_Free(barray);
      Ddi_Free(varray);
    }

    /* If Toggle Force the Present to be the Opposite */
    if (toggle) {
      splitPS = Ddi_BddMakeLiteral(vs, 1);
      Ddi_BddXorAcc(splitPS, splitNS);
      Ddi_BddAndAcc(constr, splitPS);
      Ddi_Free(splitPS);
    }

    tr->clkTr = Ddi_BddDup(constr);
    Ddi_Lock(tr->clkTr);
    Ddi_BddPartInsert(trBdd, 0, constr);
    Ddi_Free(constr);
    Ddi_Free(split);
    Ddi_Free(splitNS);

    v = vs;

  }
  Ddi_Free(pi);
  tr->clk = v;
  return (v);

}

/**Function********************************************************************
  Synopsis    [Disjunctive Partition Transition Relation according to variable]
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Tr_TrPartition(
  Tr_Tr_t * tr,
  char *varname,
  int toggle
)
{
  Tr_Tr_t *trTrNew;
  Ddi_Bdd_t *trBdd, *tr0, *tr1, *split, *splitPS, *splitNS, *constr;
  char buf[PDTUTIL_MAX_STR_LEN];
  Ddi_Var_t *v = NULL, *vy = NULL, *vs = NULL;
  Ddi_Vararray_t *va;
  Ddi_Varset_t *pi;
  Tr_Mgr_t *trMgr = tr->trMgr;
  Ddi_Mgr_t *dd = Ddi_ReadMgr(Tr_TrBdd(tr));

  if ((varname == NULL) || (strcmp(varname, "NULL") == 0)) {
    Pdtutil_Warning(1, "Null variable for TR partition.");
    return (NULL);
  }

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "Splitting TR with variable %s.\n", varname)
    );

  /* Get Split Variable */
  v = Ddi_VarFromName(dd, varname);

  if (v == NULL) {
    fprintf(stderr, "Variable %s not found - TR is NOT partitioned.\n",
      varname);
    return (NULL);
  }

  pi = Ddi_VarsetMakeFromArray(Tr_MgrReadI(trMgr));

  trBdd = Tr_TrBdd(tr);

  if (Ddi_VarInVarset(pi, v)) {
    /* Create Present (vs) and Next (vy) State Variable for the Clock ...
       with Names */
    vy = Ddi_VarNewAfterVar(v);
    sprintf(buf, "%s$LATCH$NS", varname);
    Ddi_VarAttachName(vy, buf);
    vs = Ddi_VarNewAfterVar(v);
    sprintf(buf, "%s$LATCH", varname);
    Ddi_VarAttachName(vs, buf);

    /* Group Present and Next State Variable for Clock */
    Ddi_VarMakeGroup(dd, v, 3);

    /* Put Present/Next For Clock into Present/Next Variables */
    va = Tr_MgrReadPS(trMgr);
    Ddi_VararrayWrite(va, Ddi_VararrayNum(va), vs);
    va = Tr_MgrReadNS(trMgr);
    Ddi_VararrayWrite(va, Ddi_VararrayNum(va), vy);

#if 0
    /* Put Next Clock into AuxVar */
    /* GpC: disabled - was used just to count states */
    va = Ddi_VararrayAlloc(dd, 1);
    Ddi_VararrayWrite(va, 0, vs);
    Tr_MgrSetAuxVars(trMgr, va);
    Ddi_Free(va);
#endif

    /*
     *  Get TR - Phase Zero
     */

    split = Ddi_BddMakeLiteral(v, 0);
    splitNS = Ddi_BddMakeLiteral(vy, 0);
    constr = Ddi_BddAnd(split, splitNS);

    /* If Toggle Force the Present to be the Opposite */
    if (toggle) {
      splitPS = Ddi_BddMakeLiteral(vs, 1);
      Ddi_BddAndAcc(constr, splitPS);
      Ddi_Free(splitPS);
    }

    tr0 = Ddi_BddConstrain(trBdd, split);
    Ddi_BddPartInsert(tr0, 0, constr);
    Ddi_Free(constr);
    Ddi_Free(split);
    Ddi_Free(splitNS);

    /*
     *  Get TR - Phase One
     */

    split = Ddi_BddMakeLiteral(v, 1);
    splitNS = Ddi_BddMakeLiteral(vy, 1);
    constr = Ddi_BddAnd(split, splitNS);

    /* If Toggle Force the Present to be the Opposite */
    if (toggle) {
      splitPS = Ddi_BddMakeLiteral(vs, 0);
      Ddi_BddAndAcc(constr, splitPS);
      Ddi_Free(splitPS);
    }

    tr1 = Ddi_BddConstrain(trBdd, split);
    Ddi_BddPartInsert(tr1, 0, constr);
    Ddi_Free(constr);
    Ddi_Free(split);
    Ddi_Free(splitNS);

  } else {
    int i;
    Ddi_Vararray_t *psv = Tr_MgrReadPS(trMgr);
    Ddi_Vararray_t *nsv = Tr_MgrReadNS(trMgr);

    vs = v;
    vy = NULL;
    for (i = 0; i < Ddi_VararrayNum(psv); i++) {
      if (Ddi_VarCuddIndex(Ddi_VararrayRead(psv, i)) == Ddi_VarCuddIndex(v)) {
        vy = Ddi_VararrayRead(nsv, i);
        break;
      }
    }
    Pdtutil_Assert(vy != NULL, "split variable not found !");

    /*
     *  Get TR - Phase Zero
     */

    split = Ddi_BddMakeLiteral(v, 0);
    constr = Ddi_BddDup(split);

    /* If Toggle Force the Present to be the Opposite */
    if (toggle) {
      splitNS = Ddi_BddMakeLiteral(vy, 1);
      Ddi_BddAndAcc(constr, splitNS);
      Ddi_Free(splitNS);
    }

    tr0 = Ddi_BddConstrain(trBdd, split);
    Ddi_BddPartInsert(tr0, 0, constr);
    Ddi_Free(constr);
    Ddi_Free(split);

    /*
     *  Get TR - Phase One
     */

    split = Ddi_BddMakeLiteral(v, 1);
    constr = Ddi_BddDup(split);

    /* If Toggle Force the Present to be the Opposite */
    if (toggle) {
      splitNS = Ddi_BddMakeLiteral(vy, 0);
      Ddi_BddAndAcc(constr, splitNS);
      Ddi_Free(splitNS);
    }

    tr1 = Ddi_BddConstrain(trBdd, split);
    Ddi_BddPartInsert(tr1, 0, constr);
    Ddi_Free(constr);
    Ddi_Free(split);

  }


#if 0

  pi0 = Ddi_BddSupp(tr0);
  Ddi_VarsetIntersectAcc(pi, pi0);
  pi1 = Ddi_BddSupp(tr1);
  Ddi_VarsetIntersectAcc(pi, pi1);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "PI support for TR0: ")
    );
  Ddi_VarsetPrint(pi0, 60, NULL, stdout);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "PI support for TR1: ")
    );
  Ddi_VarsetPrint(pi1, 60, NULL, stdout);

  Ddi_Free(pi0);
  Ddi_Free(pi1);

#endif

  Ddi_Free(pi);

  trBdd = Ddi_BddMakePartDisjFromMono(tr0);
  Ddi_BddPartInsertLast(trBdd, tr1);
  Ddi_Free(tr0);
  Ddi_Free(tr1);

  trTrNew = Tr_TrMakeFromRel(trMgr, trBdd);
  trTrNew->clk = vs;
  trTrNew->clkTrFormat = 1;     /* old format: clock literal not extracted */

  Ddi_Free(trBdd);

  return (trTrNew);
}


/**Function********************************************************************
  Synopsis    [Disjunctive Partition Transition Relation according to variable]
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Tr_TrPartitionWithClock(
  Tr_Tr_t * tr,
  Ddi_Var_t * clk,
  int checkImplications
)
{
  Tr_Tr_t *trTrNew;
  Ddi_Bdd_t *trBdd, *newTrBdd, *clk_lit, *tr_i;
  Ddi_Var_t *clk_ns = NULL;
  Tr_Mgr_t *trMgr = tr->trMgr;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(Tr_TrBdd(tr));
  Ddi_Vararray_t *psv = Tr_MgrReadPS(trMgr);
  Ddi_Vararray_t *nsv = Tr_MgrReadNS(trMgr);
  Ddi_Varsetarray_t *suppArray;
  Ddi_Varset_t *supp, *psVars;

  int i;

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "Splitting TR with variable %s.\n", Ddi_VarName(clk))
    );

  trBdd = Tr_TrBdd(tr);

  clk_ns = NULL;
  for (i = 0; i < Ddi_VararrayNum(psv); i++) {
    if (Ddi_VarCuddIndex(Ddi_VararrayRead(psv, i)) == Ddi_VarCuddIndex(clk)) {
      clk_ns = Ddi_VararrayRead(nsv, i);
      break;
    }
  }

  Pdtutil_Assert(clk_ns != NULL, "next state CLK variable not found !");

  newTrBdd = Ddi_BddMakePartDisjVoid(ddm);

  psVars = Ddi_VarsetMakeFromArray(psv);

  suppArray = Ddi_VarsetarrayAlloc(ddm, 2);

  for (i = 0; i < 2; i++) {
    int again;
    Ddi_Bdd_t *totImplications = Ddi_BddMakeConst(ddm, 1);

    clk_lit = Ddi_BddMakeLiteral(clk, i);
    Ddi_BddSetPartConj(clk_lit);

    tr_i = Ddi_BddCofactor(trBdd, clk, i);
    if (tr->clkTr != NULL) {
      int j;
      Ddi_Bdd_t *clkTrPhase = Ddi_BddCofactor(tr->clkTr, clk, i);

      Ddi_BddSetPartConj(tr_i);
      for (j = 0; j < Ddi_BddPartNum(tr_i); j++) {
        Ddi_BddAndAcc(Ddi_BddPartRead(tr_i, j), clkTrPhase);
      }
      Ddi_Free(clkTrPhase);
    }

    if (checkImplications) {
      do {
        Ddi_Bdd_t *implications = Ddi_BddImpliedCube(tr_i, NULL);

        again = 0;
        if (!Ddi_BddIsOne(implications)) {
          Ddi_BddConstrainAcc(tr_i, implications);
          Ddi_BddAndAcc(totImplications, implications);
          again = 1;
        }
        Ddi_Free(implications);
      } while (again);
    }
    if (!Ddi_BddIsOne(totImplications)) {
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "Found %d implied vars in %d cofactor.\n",
          Ddi_BddSize(totImplications) - 1, i)
        );
      if (Ddi_BddIsPartConj(tr_i)) {
        Ddi_BddPartInsert(tr_i, 0, totImplications);
      } else {
        Ddi_BddAndAcc(tr_i, totImplications);
      }
    }
    Ddi_Free(totImplications);

    Ddi_BddPartWrite(clk_lit, 1, tr_i);
    Ddi_BddPartWrite(newTrBdd, i, clk_lit);

    supp = Ddi_BddSupp(tr_i);
    Ddi_VarsetIntersectAcc(supp, psVars);
    Ddi_VarsetarrayWrite(suppArray, i, supp);
    Ddi_Free(supp);

    Ddi_Free(tr_i);
    Ddi_Free(clk_lit);

  }

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "TR supp: [%d,%d] (tot vars: %d).\n",
      Ddi_VarsetNum(Ddi_VarsetarrayRead(suppArray, 0)),
      Ddi_VarsetNum(Ddi_VarsetarrayRead(suppArray, 1)), Ddi_VarsetNum(psVars))
    );

  supp = Ddi_VarsetIntersect(Ddi_VarsetarrayRead(suppArray, 0),
    Ddi_VarsetarrayRead(suppArray, 1));

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "%d Common vars between clocked TR partitions.\n",
      Ddi_VarsetNum(supp))
    );

  Ddi_Free(supp);
  Ddi_Free(suppArray);
  Ddi_Free(psVars);

  trTrNew = Tr_TrMakeFromRel(trMgr, newTrBdd);
  Pdtutil_Assert(tr->clk == clk, "Wrong TR clk found");
  trTrNew->clk = clk;
  if (tr->clkTr != NULL) {
    trTrNew->clkTr = Ddi_BddDup(tr->clkTr);
    Ddi_Lock(trTrNew->clkTr);
  }
  Ddi_Free(newTrBdd);

  return (trTrNew);
}

/**Function*******************************************************************
  Synopsis    [manual partitioning of TR based on partition file]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Tr_TrManualPartitions(
  Tr_Tr_t * tr,
  char *file
)
{
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Ddi_Bdd_t *trBdd, *trBdd2;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(Tr_TrBdd(tr));
  Tr_Tr_t *tr2;

  FILE *fp;
  char buf[PDTUTIL_MAX_STR_LEN + 1];
  int i;
  Ddi_Bdd_t *part, *part2;

  Ddi_Var_t *v;
  Ddi_Varset_t *supp;

  return trManualPartitionsIds(tr, file);

  trBdd = Ddi_BddDup(Tr_TrBdd(tr));

  if (!Ddi_BddIsPartConj(trBdd)) {
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "NO conj part TR for manual partitioning.\n"));

    Ddi_Free(trBdd);
    return (Tr_TrDup(tr));
  }

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Reading partitions file %s.\n", file));

  for (i = 0; i < Ddi_BddPartNum(trBdd); i++) {
    Ddi_BddSuppAttach(Ddi_BddPartRead(trBdd, i));
  }
  trBdd2 = Ddi_BddMakePartConjVoid(ddiMgr);

  fp = fopen(file, "r");
  if (fp == NULL) {
    Ddi_Free(trBdd);
    return (Tr_TrDup(tr));
  }

  part2 = NULL;
  while (fscanf(fp, "%s", buf) == 1) {
    if (buf[0] == '#') {
      if (part2 != NULL) {
        Ddi_BddPartInsertLast(trBdd2, part2);
        Ddi_Free(part2);
        part2 = NULL;
      }
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
        fprintf(stdout, "\n"));
      continue;
    }
    if (strstr(buf, "$NS") == NULL) {
      strcat(buf, "$NS");
    }
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "%s ", buf));
    v = Ddi_VarFromName(ddiMgr, buf);
    if (v != NULL) {
      for (i = 0; i < Ddi_BddPartNum(trBdd); i++) {
        supp = Ddi_BddSuppRead(Ddi_BddPartRead(trBdd, i));
        if (Ddi_VarInVarset(supp, v)) {
          part = Ddi_BddPartExtract(trBdd, i);
          Ddi_BddSuppDetach(part);
          if (part2 == NULL) {
            part2 = Ddi_BddDup(part);
          } else {
            Ddi_BddSetPartConj(part2);
            Ddi_BddPartInsertLast(part2, part);
          }
          Ddi_Free(part);
          break;
        }
      }
    }
  }

  for (i = 0; i < Ddi_BddPartNum(trBdd2); i++) {
    part = Ddi_BddPartRead(trBdd2, i);
    supp = Ddi_BddSupp(part);
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "Partitione TR; #Partition=%d; Size=%d.\n", i,
        Ddi_BddSize(part)));
    Ddi_VarsetPrint(supp, 0, NULL, stdout);
    Ddi_Free(supp);
  }

  tr2 = Tr_TrMakeFromRel(trMgr, trBdd2);
  Ddi_Free(trBdd);
  Ddi_Free(trBdd2);

  return (tr2);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Tr_TrRepartition(
  Tr_Tr_t * tr,
  int clustTh
)
{
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Ddi_Bdd_t *trBdd = Tr_TrBdd(tr), *newTrBdd, *prev, *p, *p1, *pj, *p1j;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(trBdd);
  Ddi_Varset_t *ps, *pi, *pspi, *smooth, *supp;
  int i, np, j;

  if (!Ddi_BddIsPartConj(trBdd)) {
    return tr;
  }

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "TR repartition: %d\n", Ddi_BddSize(trBdd)));

  ps = Ddi_VarsetMakeFromArray(Tr_MgrReadPS(trMgr));
  pi = Ddi_VarsetMakeFromArray(Tr_MgrReadI(trMgr));
  pspi = Ddi_VarsetUnion(ps, pi);
  Ddi_Free(ps);
  Ddi_Free(pi);

  np = Ddi_BddPartNum(trBdd);

  newTrBdd = Ddi_BddMakePartConjVoid(ddiMgr);
  prev = Ddi_BddMakePartConjVoid(ddiMgr);

  for (i = 0; Ddi_BddPartNum(trBdd) > 0; i++) {
    p = Ddi_BddPartExtract(trBdd, 0);
    Ddi_BddSetPartConj(p);
    for (j = 0; j < Ddi_BddPartNum(p); j++) {
      Ddi_BddPartInsertLast(prev, Ddi_BddPartRead(p, j));
    }
    Ddi_Free(p);
    p = prev;
    prev = Ddi_BddMakePartConjVoid(ddiMgr);

    smooth = Ddi_BddSupp(p);
    Ddi_VarsetIntersectAcc(smooth, pspi);
    supp = Ddi_BddSupp(trBdd);
    Ddi_VarsetDiffAcc(smooth, supp);
    Ddi_Free(supp);

    if (!Ddi_VarsetIsVoid(smooth) && (Ddi_BddPartNum(trBdd) > 0)) {
      p1 = Ddi_BddPartRead(trBdd, 0);
      Ddi_BddSetPartConj(p1);

      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "[%3d/%3d][%d] {%3d} %d -> ",
          i, np, Ddi_VarsetNum(smooth), Ddi_BddPartNum(p1), Ddi_BddSize(p)));

      while (Ddi_BddPartNum(p) > 0) {
        pj = Ddi_BddPartExtract(p, 0);
#if 0
        p1j = Ddi_BddMakeFromCU(ddiMgr,
          Fbv_bddExistAbstractWeak(Ddi_MgrReadMgrCU(ddiMgr),
            Ddi_BddToCU(pj), Ddi_VarsetToCU(smooth)));
#else
        p1j = Ddi_BddExist(pj, smooth);
#endif
#if 0
        if (!Ddi_BddIsZero(p1j)) {
          Ddi_BddNotAcc(p1j);
          Ddi_BddRestrictAcc(pj, p1j);
          Ddi_BddSetPartDisj(pj);

          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
            fprintf(stdout, "(%d,%d) ", Ddi_BddSize(pj), Ddi_BddSize(p1j)));

          Ddi_BddPartInsertLast(pj, p1j);
        }
#else
        if (!Ddi_BddIsOne(p1j)) {
          Ddi_BddRestrictAcc(pj, p1j);
          Ddi_BddPartInsertLast(p1, p1j);

          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
            fprintf(stdout, "(%d,%d) ", Ddi_BddSize(pj), Ddi_BddSize(p1j)));
        }
#endif
        else {
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
            fprintf(stdout, "(%d) ", Ddi_BddSize(pj)));
        }
        Ddi_BddPartInsertLast(newTrBdd, pj);
        Ddi_Free(pj);
        Ddi_Free(p1j);
      }

      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "\n"));

      Ddi_BddSetClustered(p1, clustTh);
      if (Ddi_BddPartNum(p1) == 1) {
        Ddi_BddSetMono(p1);
      }
    } else {
      for (j = 0; j < Ddi_BddPartNum(p); j++) {
        pj = Ddi_BddPartRead(p, j);
        if (!Ddi_BddIsOne(pj)) {
          Ddi_BddPartInsertLast(prev, pj);
        }
      }
    }
    Ddi_Free(smooth);
    Ddi_Free(p);

  }

  for (j = 0; j < Ddi_BddPartNum(prev); j++) {
    Ddi_BddPartInsertLast(newTrBdd, Ddi_BddPartRead(prev, j));
  }
  Ddi_Free(prev);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "|New TR|=%d: [ ", Ddi_BddSize(newTrBdd)));

  for (i = 0; i < Ddi_BddPartNum(newTrBdd); i++) {
    p = Ddi_BddPartRead(newTrBdd, i);
    Ddi_BddPartInsertLast(trBdd, p);
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "%d ", Ddi_BddSize(p)));
  }

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "\n"));

  Ddi_Free(newTrBdd);
  Ddi_Free(prev);
  Ddi_Free(pspi);

  return tr;
}

/**Function********************************************************************
  Synopsis    [Slave elimination in Master-Slave latch couples]
  Description [Slave elimination in Master-Slave latch couples. Bit level
               TR is assumed.]
  SideEffects [Return eliminated vars in reducedVars set]
  SeeAlso     []
******************************************************************************/

Tr_Tr_t *
Tr_TrReduceMasterSlave(
  Tr_Tr_t * tr,
  Ddi_Varset_t * reducedVars,
  Ddi_Bddarray_t * lambda,
  Ddi_Bdd_t * init,
  Ddi_Var_t * clk
)
{
  Tr_Tr_t *trTrNew = NULL;
  Ddi_Bdd_t *trBdd;
  Tr_Mgr_t *trMgr = tr->trMgr;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(Tr_TrBdd(tr));
  Ddi_Vararray_t *psv = Tr_MgrReadPS(trMgr);
  Ddi_Vararray_t *nsv = Tr_MgrReadNS(trMgr);
  Ddi_Vararray_t *piv = Tr_MgrReadI(trMgr);
  Ddi_Varset_t *removed, *piSet;
  Ddi_Vararray_t *master, *slave;
  Ddi_Bddarray_t *masterBdd, *data, *enable;
  Ddi_Varset_t *lsupp = NULL;
  int *refEnables;
  int again;
  int i, j, nbit, nvars, *slaveIds, *masterIds, nred,
    perif_l = 0, perif_l_fo1 = 0, ps_en = 0, perif_clk = 0,
    tot_clk = 0, tot_clk_inv = 0,
    eq_l = 0, edge_clk_l = 0, io_l = 0, max_level = 0;
  int *fanoutCnt, *level, *perifIds, nMgrVars = Ddi_MgrReadNumCuddVars(ddm);


  Pdtutil_Assert(clk != NULL, "Clock var required by MS reduction");

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "Reducing Master-Slave latches.\n")
    );

  fanoutCnt = Pdtutil_Alloc(int,
    nMgrVars
  );
  level = Pdtutil_Alloc(int,
    nMgrVars
  );

  for (i = 0; i < nMgrVars; i++) {
    fanoutCnt[i] = 0;
    level[i] = 0;
  }

  piSet = Ddi_VarsetMakeFromArray(piv);

  trBdd = Tr_TrBdd(tr);

  nvars = Ddi_VararrayNum(psv);
  nbit = Ddi_BddPartNum(trBdd);

  if (trMgr->auxVars != NULL) {
    Ddi_Varset_t *auxvSet = Ddi_VarsetMakeFromArray(trMgr->auxVars);

    Ddi_VarsetDiffAcc(piSet, auxvSet);
    Ddi_Free(auxvSet);
    nbit = nbit - Ddi_VararrayNum(trMgr->auxVars);
  }
  Pdtutil_Assert(nvars == nbit,
    "unaligned BDD and var arrays in MS reduction");

  perifIds = Pdtutil_Alloc(int,
    nbit
  );

  slaveIds = Pdtutil_Alloc(int,
    nbit
  );
  masterIds = Pdtutil_Alloc(int,
    nbit
  );
  refEnables = Pdtutil_Alloc(int,
    nbit
  );

  data = Ddi_BddarrayAlloc(ddm, nbit);
  enable = Ddi_BddarrayAlloc(ddm, nbit);

  for (i = 0; i < nbit; i++) {
    refEnables[i] = -1;
    masterIds[i] = 0;
    slaveIds[i] = 0;
    perifIds[i] = 0;
    Ddi_BddarrayWrite(data, i, NULL);
    Ddi_BddarrayWrite(enable, i, NULL);
  }

  if (lambda != NULL) {
    lsupp = Ddi_BddarraySupp(lambda);
    Ddi_VarsetDiffAcc(reducedVars, lsupp);

    for (Ddi_VarsetWalkStart(lsupp);
      !Ddi_VarsetWalkEnd(lsupp); Ddi_VarsetWalkStep(lsupp)) {
      int id = Ddi_VarCuddIndex(Ddi_VarsetWalkCurr(lsupp));

      fanoutCnt[id]++;
    }

    Ddi_Free(lsupp);
  }

  for (i = 0; i < Ddi_BddPartNum(trBdd); i++) {
    Ddi_Varset_t *supp = Ddi_BddSupp(Ddi_BddPartRead(trBdd, i));

    for (Ddi_VarsetWalkStart(supp);
      !Ddi_VarsetWalkEnd(supp); Ddi_VarsetWalkStep(supp)) {
      int id = Ddi_VarCuddIndex(Ddi_VarsetWalkCurr(supp));

      fanoutCnt[id]++;
    }

    Ddi_Free(supp);
  }

  /* find enables */

  for (i = 0; i < nbit; i++) {
    Ddi_Bdd_t *en, *d;
    Ddi_Bdd_t *delta_i, *deltabar_i;
    Ddi_Var_t *p_i = Ddi_VararrayRead(psv, i);
    Ddi_Var_t *n_i = Ddi_VararrayRead(nsv, i);

    if (!Ddi_VarInVarset(reducedVars, p_i)) {
      /* skip variables not in reducedVars set */
      continue;
    }
    /* delta = en * d + !en * q */
    delta_i = Ddi_BddCofactor(Ddi_BddPartRead(trBdd, i), n_i, 1);
    /* deltabar = en * !d + !en * !q */
    deltabar_i = Ddi_BddCofactor(Ddi_BddPartRead(trBdd, i), n_i, 0);
    /* isolate transparent behavior */
    /* delta = en * d */
    Ddi_BddCofactorAcc(delta_i, p_i, 0);
    /* deltabar = en * !d */
    Ddi_BddCofactorAcc(deltabar_i, p_i, 1);
    en = Ddi_BddOr(delta_i, deltabar_i);
    Ddi_BddConstrainAcc(delta_i, en);
    Ddi_BddConstrainAcc(deltabar_i, en);
    Ddi_BddNotAcc(deltabar_i);
    if (Ddi_BddEqual(delta_i, deltabar_i)) {
      d = delta_i;
      /* check memory behavior */
      delta_i = Ddi_BddCofactor(Ddi_BddPartRead(trBdd, i), n_i, 1);
      Ddi_BddNotAcc(en);
      Ddi_BddCofactorAcc(delta_i, p_i, 1);
      Ddi_BddConstrainAcc(delta_i, en);
      Ddi_BddNotAcc(en);
      if (Ddi_BddIsOne(delta_i)) {
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "found enable for latch: %s.\n", Ddi_VarName(p_i));
          fprintf(stdout, "ENABLE: "); fprintf(stdout, "DATA: ")
          );
        Ddi_BddarrayWrite(data, i, d);
        Ddi_BddarrayWrite(enable, i, en);
      }
    }

    Ddi_Free(en);
    Ddi_Free(d);
    Ddi_Free(delta_i);
    Ddi_Free(deltabar_i);
  }


  /* levelize */
  do {
    again = 0;
    for (i = 0; i < nbit; i++) {
      //Ddi_Bdd_t *en_i = Ddi_BddarrayRead(enable,i);
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(data, i);

      if (d_i != NULL) {
        Ddi_Varset_t *dSupp = suppWithAuxVars(d_i, trMgr);
        int l_in = 0;
        int refId = Ddi_VarCuddIndex(Ddi_VararrayRead(psv, i));
        int l_out = level[refId];

        for (Ddi_VarsetWalkStart(dSupp);
          !Ddi_VarsetWalkEnd(dSupp); Ddi_VarsetWalkStep(dSupp)) {
          int id = Ddi_VarCuddIndex(Ddi_VarsetWalkCurr(dSupp));

          if (level[id] > l_in) {
            l_in = level[id];
          }
          if (l_in + 1 > l_out) {
            again = 1;
            l_out = level[refId] = l_in + 1;
            if (l_out > max_level) {
              max_level = l_out;
            }
          }
        }
        Ddi_Free(dSupp);
      }
    }
  } while (again);

  for (i = 0; i < nbit; i++) {
    Ddi_Bdd_t *en_i = Ddi_BddarrayRead(enable, i);
    Ddi_Bdd_t *d_i = Ddi_BddarrayRead(data, i);
    int eq = 0;

    if (en_i == NULL || d_i == NULL) {
      continue;
    }
    for (j = i + 1; j < nbit; j++) {
      Ddi_Bdd_t *en_j = Ddi_BddarrayRead(enable, j);
      Ddi_Bdd_t *d_j = Ddi_BddarrayRead(data, j);

      if (en_j == NULL || d_j == NULL) {
        continue;
      }
      if (Ddi_BddEqual(d_i, d_j) && Ddi_BddEqual(en_i, en_j)) {
        eq = 1;
      }
    }
    eq_l += eq;
  }

  masterBdd = Ddi_BddarrayAlloc(ddm, 0);
  master = Ddi_VararrayAlloc(ddm, 0);
  slave = Ddi_VararrayAlloc(ddm, 0);
  removed = Ddi_VarsetVoid(ddm);

  for (i = 0; i < nbit; i++) {

    Ddi_Varset_t *supp;
    Ddi_Bdd_t *en_i = Ddi_BddarrayRead(enable, i);
    Ddi_Bdd_t *d_i = Ddi_BddarrayRead(data, i);

    if (en_i == NULL || d_i == NULL) {
      continue;
    }
    supp = Ddi_BddSupp(d_i);
    if (Ddi_VarsetNum(supp) != 1) {
      Ddi_Free(supp);
      continue;
    }
    {
      Ddi_Varset_t *supp_en_i = Ddi_BddSupp(en_i);
      int l, pi_included = 0;

      for (l = 0; l < Ddi_VararrayNum(piv); l++) {
        if (Ddi_VararrayRead(piv, l) != clk) {
          if (Ddi_VarInVarset(supp_en_i, Ddi_VararrayRead(piv, l))) {
            pi_included = 1;
          }
        }
      }
      if (!pi_included) {
        ps_en++;
      }
      Ddi_Free(supp_en_i);
    }
    if ((slaveIds[i] != 0) || (masterIds[i] != 0)) {
      Ddi_Free(supp);
      continue;
    }
    for (j = 0; j < Ddi_VararrayNum(psv); j++) {
      if (Ddi_VarInVarset(supp, Ddi_VararrayRead(psv, j))) {
        /* candidate s master-slave (j->i) found */
        break;
      }
    }
    if (j < Ddi_VararrayNum(psv)) {
      Ddi_Bdd_t *en_j = Ddi_BddarrayRead(enable, j);
      Ddi_Bdd_t *d_j = Ddi_BddarrayRead(data, j);

      if ((masterIds[j] != 0 || slaveIds[i] != 0)
        || (masterIds[i] != 0 || slaveIds[j] != 0)) {
        Ddi_Free(supp);
        continue;
      }
      if (i != j && en_j != NULL && d_j != NULL) {
        Ddi_Varset_t *supp_en_i = Ddi_BddSupp(en_i);
        Ddi_Varset_t *supp_en_j = Ddi_BddSupp(en_j);
        Ddi_Bdd_t *aux = Ddi_BddAnd(en_i, en_j);
        int en_i_is_clk = ((Ddi_VarsetNum(supp_en_i) == 1) &&
          Ddi_VarInVarset(supp_en_i, clk));
        Ddi_VarsetDiffAcc(supp_en_i, supp_en_j);
        if (en_i_is_clk) {
          /* toggling clock required. This is a safe condition.
             A tighter condition should be inserted. We just need
             to guarantee that enable is not 0 (memory) for two cycles
           */
          int k;

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMed_c,
            fprintf(stdout, "found MS couple: %s -> %s.\n",
              Ddi_VarName(Ddi_VararrayRead(psv, j)),
              Ddi_VarName(Ddi_VararrayRead(psv, i)))
            );
          Ddi_VararrayInsertLast(master, Ddi_VararrayRead(psv, j));
          Ddi_VararrayInsertLast(slave, Ddi_VararrayRead(psv, i));
          Ddi_VarsetAddAcc(removed, Ddi_VararrayRead(psv, i));
          Ddi_BddarrayInsertLast(masterBdd, d_i);
          masterIds[j] = 1;
          slaveIds[i] = 1;
          for (k = 0; k < Ddi_VararrayNum(psv); k++) {
            Ddi_Bdd_t *d_k = Ddi_BddarrayRead(data, k);
            Ddi_Bdd_t *en_k = Ddi_BddarrayRead(enable, j);  /* GpC ? */

            if (d_k == NULL) {
              d_k = Ddi_BddPartRead(trBdd, k);
              en_k = NULL;
            }
            if (k != i) {
              Ddi_Varset_t *supp_d_k = Ddi_BddSupp(d_k);

#if 0
              if (Ddi_VarInVarset(supp_d_k, Ddi_VararrayRead(psv, i))) {
                Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
                  Pdtutil_VerbLevelNone_c,
                  fprintf(stdout, "fed latch[%d].\n", k)
                  );
              }
#endif
              Ddi_Free(supp_d_k);
            }
          }
          /* check compatibility with initial state */
          if (init != NULL) {
            Ddi_Bdd_t *newInit = Ddi_BddCompose(init, slave, masterBdd);
            Ddi_Bdd_t *newInit2 = Ddi_BddExist(init, removed);

            if (!Ddi_BddEqual(newInit, newInit2)) {
              Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMed_c,
                fprintf(stdout,
                  "MS couple not compatible with init st.: not reduced.\n")
                );
              masterIds[j] = 0;
              slaveIds[i] = 0;
              Ddi_VararrayRemove(master, Ddi_VararrayNum(master) - 1);
              Ddi_VararrayRemove(slave, Ddi_VararrayNum(slave) - 1);
              Ddi_VarsetRemoveAcc(removed, Ddi_VararrayRead(psv, i));
              Ddi_BddarrayRemove(masterBdd, Ddi_BddarrayNum(masterBdd) - 1);
            } else {
              Ddi_BddExistAcc(init, removed);
            }
            Ddi_Free(newInit);
            Ddi_Free(newInit2);
          }
        } else if (Ddi_BddIsZero(aux) && Ddi_VarsetIsVoid(supp_en_i)) {
          Ddi_BddNotAcc(en_i);
          if (Ddi_BddEqual(en_i, en_j)) {
          }
          edge_clk_l++;
          Ddi_BddNotAcc(en_i);
        } else if (Ddi_BddIsZero(aux)) {
          io_l++;
        }
        Ddi_Free(aux);
        Ddi_Free(supp_en_i);
        Ddi_Free(supp_en_j);
      }
    } else if (Ddi_VarInVarset(piSet, Ddi_VarsetTop(supp))) {
      int id = Ddi_VarCuddIndex(Ddi_VarsetTop(supp));

      if (fanoutCnt[id] == 1) {
        perif_l_fo1++;
#if 0
        /* GpC - Nov 2004: not enabled so far perferal latch removal,
           as it is an abstraction (latch is clocked) */
        slaveIds[i] = 1;
        Ddi_VararrayInsertLast(master, Ddi_VarsetTop(supp));
        Ddi_VararrayInsertLast(slave, Ddi_VararrayRead(psv, i));
        Ddi_VarsetAddAcc(removed, Ddi_VararrayRead(psv, i));
        Ddi_BddarrayInsertLast(masterBdd, d_i);
#endif
      }
      perif_l++;
      perifIds[i] = 1;
#if 1
      if ((slaveIds[i] == 0) && (masterIds[i] == 0)) {
        /* GpC - Nov 2004: not enabled so far perferal latch removal,
           as it is an abstraction (latch is clocked) */
        slaveIds[i] = 1;
        Ddi_VararrayInsertLast(master, Ddi_VarsetTop(supp));
        Ddi_VararrayInsertLast(slave, Ddi_VararrayRead(psv, i));
        Ddi_VarsetAddAcc(removed, Ddi_VararrayRead(psv, i));
        Ddi_BddarrayInsertLast(masterBdd, d_i);
      }
#endif
    }
    Ddi_Free(supp);

  }

  for (i = 0; i < nbit; i++) {
    if (perifIds[i]) {
      int newClk = 1;
      Ddi_Bdd_t *en_i = Ddi_BddarrayRead(enable, i);

      for (j = 0; j < i; j++) {
        if (perifIds[j]) {
          Ddi_Bdd_t *en_j = Ddi_BddarrayRead(enable, j);

          if (Ddi_BddEqual(en_i, en_j)) {
            newClk = 0;
          }
        }
      }
      perif_clk += newClk;
    }
  }

  for (i = 0; i < nbit; i++) {
    Ddi_Bdd_t *en_i = Ddi_BddarrayRead(enable, i);

    if (en_i != NULL) {
      int newClk = 1, newClkInv = 1;
      Ddi_Bdd_t *not_en_i = Ddi_BddNot(en_i);

      for (j = 0; j < i; j++) {
        Ddi_Bdd_t *en_j = Ddi_BddarrayRead(enable, j);

        if (en_j != NULL) {
          if (Ddi_BddEqual(en_i, en_j)) {
            newClk = 0;
          } else if (Ddi_BddEqual(not_en_i, en_j)) {
            newClkInv = 0;
          }
        }
      }
      Ddi_Free(not_en_i);
      tot_clk += newClk;
      tot_clk_inv += (newClkInv && newClk);
    }
  }

  nred = 0;
#if 1
  for (i = nbit - 1; i >= 0; i--) {
    if (slaveIds[i]) {
      Pdtutil_Assert(masterIds[i] == 0, "Master-Slave mismatch");
      Ddi_BddPartWrite(trBdd, i, Ddi_MgrReadOne(ddm));
      nred++;
    }
#if 0
    else if (masterIds[i]) {
      /* disabled for now. We assume toggling clock
         so no check on next state for enable needed */
      Ddi_Bdd_t *newt, *tmp;
      Ddi_Bdd_t *en_i = Ddi_BddarrayRead(enable, i);
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(data, i);
      Ddi_Bdd_t *cond = Ddi_BddSwapVars(en_i, psv, nsv);

      /* first make delta: !en&next(en)&d + !(!en&next(en))&q */
      Ddi_BddDiffAcc(cond, en_i);
      newt = Ddi_BddAnd(cond, d_i);
      tmp = Ddi_BddMakeLiteral(Ddi_VararrayRead(psv, i), 1);
      Ddi_BddDiffAcc(tmp, cond);
      Ddi_BddOrAcc(newt, tmp);
      Ddi_Free(tmp);
      Ddi_Free(cond);
      /* now make tr_i */
      tmp = Ddi_BddMakeLiteral(Ddi_VararrayRead(nsv, i), 1);
      Ddi_BddXnorAcc(newt, tmp);
      Ddi_BddPartWrite(trBdd, i, newt);
      Ddi_Free(tmp);
      Ddi_Free(cond);
    }
#endif
  }

  Ddi_BddComposeAcc(trBdd, slave, masterBdd);
  for (i = 0; i < Ddi_BddarrayNum(lambda); i++) {
    Ddi_BddComposeAcc(Ddi_BddarrayRead(lambda, i), slave, masterBdd);
  }
#endif

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout,
      "MS REDUCTION: %d (out of %d) MS couples of latches reduced.\n", nred,
      nvars);
    fprintf(stdout,
      "MS REDUCTION: %d PS en - %d edge - %d io - tot ck(inv): %d(%d).\n",
      ps_en, edge_clk_l, io_l, tot_clk, tot_clk_inv);
    fprintf(stdout,
      "MS REDUCTION: %d(%d)/%d Inp. perif(fo 1)/clk - %d eq latches.\n",
      perif_l, perif_l_fo1, perif_clk, eq_l);
    fprintf(stdout, "MS REDUCTION: max level: %d.\n", max_level)
    );
#if 0
  /* merge common enables and count them */
  for (i = 0; i < nbit - 1; i++) {
    Ddi_Bdd_t *en_i_bar, *en_i = Ddi_BddarrayRead(enable, i);

    if (en_i == NULL || refEnables[i] >= 0) {
      continue;
    }
    en_i_bar = Ddi_BddNot(en_i);
    refEnables[i] = i;
    cntEnables++;
    for (j = i + 1; j < nbit; j++) {
      Ddi_Bdd_t *en_j = Ddi_BddarrayRead(enable, j);

      if (en_j != NULL) {
        if (Ddi_BddEqual(en_j, en_i) || Ddi_BddEqual(en_j, en_i_bar)) {
          Pdtutil_Assert(refEnables[j] == -1, "wrong refEnables entry");
          refEnables[j] = i;
        }
      }
    }
    Ddi_Free(en_i_bar);
  }
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "COUNT ENABLES: %d.\n", cntEnables)
    );
  for (i = 0; i < nbit; i++) {
    if (refEnables[i] == i) {
      int cnt1 = 1, cnt0 = 0, cntd = 0;
      Ddi_Vararray_t *va;
      Ddi_Bdd_t *tr_new;
      char buf[1000];
      Ddi_Bdd_t *en_i = Ddi_BddarrayRead(enable, i);
      Ddi_Varset_t *supp = Ddi_BddSupp(en_i);
      Ddi_Var_t *vs, *vy, *v = Ddi_VarsetBottom(supp);

      Ddi_Free(supp);

      /* Create Present (vs) and Next (vy) State Variable for the Clock ...
         with Names */
      vy = Ddi_VarNewAfterVar(v);
      sprintf(buf, "%s$LATCH$ENABLE$NS", Ddi_VarName(v));
      Ddi_VarAttachName(vy, buf);
      vs = Ddi_VarNewAfterVar(v);
      sprintf(buf, "%s$LATCH$ENABLE", Ddi_VarName(v));
      Ddi_VarAttachName(vs, buf);

      /* Group Present and Next State Variable for Clock */
      Ddi_VarMakeGroup(ddm, vy, 2);

      /* Put new vars into Present/Next Variables */
      va = Tr_MgrReadPS(trMgr);
      Ddi_VararrayWrite(va, Ddi_VararrayNum(va), vs);
      va = Tr_MgrReadNS(trMgr);
      Ddi_VararrayWrite(va, Ddi_VararrayNum(va), vy);

#if 0
      /* Put Next Clock into AuxVar */
      /* GpC: disabled - was used just to count states */
      va = Ddi_VararrayAlloc(ddm, 1);
      Ddi_VararrayWrite(va, 0, vs);
      Tr_MgrSetAuxVars(trMgr, va);
      Ddi_Free(va);
#endif

      /*
       *  Gen TR component
       */

      tr_new = Ddi_BddMakeLiteral(vs, 1);
      Ddi_BddXnorAcc(tr_new, en_i);
#if 0
      Ddi_BddSwapVarsAcc(tr_new, psv, nsv);
#endif
      Ddi_BddPartInsert(trBdd, 0, tr_new);
      Ddi_Free(tr_new);

      v = vs;

      if (Ddi_BddSize(Ddi_BddarrayRead(enable, i)) < 10) {
        logBddFull(Ddi_BddarrayRead(enable, i));
      } else {
        logBdd(Ddi_BddarrayRead(enable, i));
      }

      for (j = i + 1; j < nbit; j++) {
        if (refEnables[j] == i) {
          Ddi_Bdd_t *en_j = Ddi_BddarrayRead(enable, j);

          if (Ddi_BddEqual(en_j, en_i)) {
            cnt1++;
          } else {
            cnt0++;
          }
          if (Ddi_BddSize(Ddi_BddarrayRead(data, j)) == 2) {
            cntd++;
          }
        }
      }
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "ENABLE[%d] - num: ->{%d} (1:%d,0:%d)/%d.\n", i, cntd,
          cnt1, cnt0, nbit)
        );
    }
  }
#endif

  Ddi_Free(removed);
  Ddi_Free(masterBdd);
  Ddi_Free(master);
  Ddi_Free(slave);
  Ddi_Free(data);
  Ddi_Free(enable);
  Ddi_Free(piSet);

  Pdtutil_Free(level);
  Pdtutil_Free(refEnables);
  Pdtutil_Free(slaveIds);
  Pdtutil_Free(masterIds);
  Pdtutil_Free(fanoutCnt);

  return (trTrNew);

}


/**Function********************************************************************
  Synopsis    [Slave elimination in Master-Slave latch couples]
  Description [Slave elimination in Master-Slave latch couples. Bit level
               TR is assumed.]
  SideEffects [Return eliminated vars in reducedVars set]
  SeeAlso     []
******************************************************************************/

Tr_Tr_t *
Tr_TrReduceMasterSlave0(
  Tr_Tr_t * tr,
  Ddi_Varset_t * reducedVars,
  Ddi_Bddarray_t * lambda,
  Ddi_Var_t * clk
)
{
  Tr_Tr_t *trTrNew = NULL;
  Ddi_Bdd_t *trBdd, *trBddClk[2], *psLit, *nsLit, *bitTr;
  Tr_Mgr_t *trMgr = tr->trMgr;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(Tr_TrBdd(tr));
  Ddi_Vararray_t *psv = Tr_MgrReadPS(trMgr);
  Ddi_Vararray_t *nsv = Tr_MgrReadNS(trMgr);
  Ddi_Varset_t *removed;
  Ddi_Vararray_t *master, *slave;
  Ddi_Bddarray_t *masterBdd, *delta;
  Ddi_Varset_t *lsupp = NULL;

  int i, j, phase, nbit, *slaveIds, *masterIds;

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "Reducing Master-Slave couples in TR using clock %s.\n",
      Ddi_VarName(clk))
    );

  trBdd = Tr_TrBdd(tr);

  trBddClk[0] = Ddi_BddCofactor(trBdd, clk, 0);
  trBddClk[1] = Ddi_BddCofactor(trBdd, clk, 1);

  nbit = Ddi_BddPartNum(trBdd);
  slaveIds = Pdtutil_Alloc(int,
    nbit
  );
  masterIds = Pdtutil_Alloc(int,
    nbit
  );

  delta = Ddi_BddarrayAlloc(ddm, nbit);

  for (i = 0; i < nbit; i++) {
    Ddi_Bdd_t *d_i = Ddi_BddCofactor(Ddi_BddPartRead(trBdd, i),
      Ddi_VararrayRead(nsv, i), 1);

    masterIds[i] = 0;
    slaveIds[i] = 0;
    Ddi_BddarrayWrite(delta, i, d_i);
    Ddi_Free(d_i);
  }

  masterBdd = Ddi_BddarrayAlloc(ddm, 0);
  master = Ddi_VararrayAlloc(ddm, 0);
  slave = Ddi_VararrayAlloc(ddm, 0);
  removed = Ddi_VarsetVoid(ddm);

  if (1 && lambda != NULL) {
    lsupp = Ddi_BddarraySupp(lambda);
    Ddi_VarsetDiffAcc(reducedVars, lsupp);
    Ddi_Free(lsupp);
  }
#if 1
  for (i = 0; i < nbit; i++) {

    psLit = Ddi_BddMakeLiteral(Ddi_VararrayRead(psv, i), 1);
    nsLit = Ddi_BddMakeLiteral(Ddi_VararrayRead(nsv, i), 1);
    bitTr = Ddi_BddXnor(psLit, nsLit);
    Ddi_Free(psLit);

    if (Ddi_BddEqual(bitTr, Ddi_BddPartRead(trBddClk[0], i)) ||
      Ddi_BddEqual(bitTr, Ddi_BddPartRead(trBddClk[1], i))) {
      phase = Ddi_BddEqual(bitTr, Ddi_BddPartRead(trBddClk[1], i));
      Ddi_Free(bitTr);
      for (j = 0; j < nbit; j++) {
        if (i != j) {
          psLit = Ddi_BddMakeLiteral(Ddi_VararrayRead(psv, j), 1);
          bitTr = Ddi_BddXnor(psLit, nsLit);
          if (Ddi_BddEqual(bitTr, Ddi_BddPartRead(trBddClk[!phase], i))) {
            if (Ddi_VarInVarset(reducedVars, Ddi_VararrayRead(psv, i))) {
              if (masterIds[i] != 0 || slaveIds[j] != 0) {
                continue;
              }
              Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
                fprintf(stdout, "found MS couple(%d): %s -> %s.\n", phase,
                  Ddi_VarName(Ddi_VararrayRead(psv, j)),
                  Ddi_VarName(Ddi_VararrayRead(psv, i)))
                );
              Ddi_BddarrayInsertLast(masterBdd, psLit);
              Ddi_VararrayInsertLast(master, Ddi_VararrayRead(psv, j));
              Ddi_VararrayInsertLast(slave, Ddi_VararrayRead(psv, i));
              Ddi_VarsetAddAcc(removed, Ddi_VararrayRead(psv, i));
              masterIds[j] = 1;
              slaveIds[i] = 1;
            }
            Ddi_Free(bitTr);
            Ddi_Free(psLit);
            break;
          }
          Ddi_Free(bitTr);
          Ddi_Free(psLit);
        }
      }
    } else {
      Ddi_Free(bitTr);
    }
    Ddi_Free(nsLit);

  }

#endif

  Ddi_Free(trBddClk[0]);
  Ddi_Free(trBddClk[1]);

#if 0
  for (i = 0; i < nbit; i++) {
    Ddi_Var_t *psQ = Ddi_VararrayRead(psv, i);
    Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);

    if (masterIds[i] != 0) {
      continue;
    }
    for (j = 0; j < nbit; j++) {
      if (j != i && slaveIds[j] == 0) {
        Ddi_Var_t *psD = Ddi_VararrayRead(psv, j);
        Ddi_Bdd_t *ck, *ckBar;

        ck = Ddi_BddCofactor(d_i, psQ, 0);
        Ddi_BddCofactorAcc(ck, psD, 1);
        ckBar = Ddi_BddCofactor(d_i, psQ, 1);
        Ddi_BddCofactorAcc(ckBar, psD, 0);
        Ddi_BddNotAcc(ckBar);
        if (Ddi_BddEqual(ck, ckBar)) {
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "Found Master-slave Couple %s => %s |CK|=%d.\n",
              Ddi_VarName(psD), Ddi_VarName(psQ), Ddi_BddSize(ck))
            );
          masterIds[i] = 1;
          slaveIds[j] = 1;
        }
        Ddi_Free(ckBar);
        Ddi_Free(ck);
      }

    }
  }
#endif

#if 1
  for (i = nbit - 1; i >= 0; i--) {
    if (slaveIds[i]) {
      bitTr = Ddi_BddPartExtract(trBdd, i);
      Ddi_Free(bitTr);
    }
  }
  Ddi_BddComposeAcc(trBdd, slave, masterBdd);
  for (i = 0; i < Ddi_BddarrayNum(lambda); i++) {
    Ddi_BddComposeAcc(Ddi_BddarrayRead(lambda, i), slave, masterBdd);
  }
#endif

  Ddi_Free(removed);
  Ddi_Free(masterBdd);
  Ddi_Free(master);
  Ddi_Free(slave);

  Ddi_Free(delta);

  Pdtutil_Free(slaveIds);
  Pdtutil_Free(masterIds);

  return (trTrNew);
}


/**Function********************************************************************
  Synopsis    [TR Cone Of Influence reduction.]
  Description [TR Cone Of Influence reduction. Transitive fanin of property
               is kept.]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
void
Tr_TrCoiReduction(
  Tr_Tr_t * tr /* transition relation */ ,
  Ddi_Bdd_t * spec              /* property */
)
{
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  int maxDepth = trMgr->settings.coi.coiLimit;

  return Tr_TrCoiReductionWithAssume(tr, spec, NULL, maxDepth);
}


/**Function********************************************************************
  Synopsis    [TR Cone Of Influence reduction (with assumptions).]
  Description [TR Cone Of Influence reduction (with assumptions).
               Transitive fanin of property and is kept. Variables outside
               COI are existentially quantified out from Assume condition.]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
void
Tr_TrCoiReductionWithAssume(
  Tr_Tr_t * tr /* transition relation */ ,
  Ddi_Bdd_t * spec /* property */ ,
  Ddi_Bdd_t * assume /* assume */ ,
  int maxDepth                  /* maximun allowed depth of COI */
)
{
  Ddi_Varset_t *nocoi, *nocoiNS;
  Ddi_Bdd_t *reducedTRBdd;
  Ddi_Varsetarray_t *coiVars;
  Ddi_Vararray_t *ps = Tr_MgrReadPS(Tr_TrMgr(tr));
  Ddi_Vararray_t *ns = Tr_MgrReadNS(Tr_TrMgr(tr));
  Ddi_Vararray_t *pi = Tr_MgrReadI(Tr_TrMgr(tr));
  int n;
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);


  Ddi_Mgr_t *ddm;

  coiVars = computeCoiVars(tr, spec, maxDepth);
  n = Ddi_VarsetarrayNum(coiVars);

  //  storeDependancyGraph("depgr.txt", tr, NULL);

  if (0) {
    sccGraph_t *sccGraph;

    sccGraph = loadSCCGraph("depgr.txt.scc", tr);
  }

  ddm = Ddi_ReadMgr(coiVars);

  if (n > 0) {

    nocoi = Ddi_VarsetMakeFromArray(ps);
    for (int i=0; i<n; i++) 
      Ddi_VarsetDiffAcc(nocoi, Ddi_VarsetarrayRead(coiVars, i));
    nocoiNS = Ddi_VarsetSwapVars(nocoi, ps, ns);

#if 0
    Ddi_VarsetUnionAcc(nocoi, nocoiNS);
#else
    Ddi_Free(nocoi);
    nocoi = Ddi_VarsetDup(nocoiNS);
#endif
    Ddi_Free(nocoiNS);

    reducedTRBdd = Part_BddMultiwayLinearAndExist(Tr_TrBdd(tr), nocoi, 1);

    Ddi_Free(nocoi);

    if (1 && (assume != NULL)) {
      int i;

      if (0 && trMgr->auxVars != NULL) {
        Ddi_Varset_t *nsV = Ddi_VarsetMakeFromArray(ns);

        /* remove the tr_i components. Just keep auxv=auxF */
        Ddi_Bdd_t *trNoDelta =
          Part_BddMultiwayLinearAndExist(reducedTRBdd, nsV, 1);
        Ddi_BddSetPartConj(assume);
        Ddi_BddPartInsertLast(assume, trNoDelta);
        Ddi_Free(nsV);
      }
      for (i = Ddi_VararrayNum(ps) - 1; i >= 0; i--) {
        Ddi_Varset_t *psInCoi = Ddi_VarsetMakeFromArray(ps);
        Ddi_Var_t *p = Ddi_VararrayRead(ps, i);

        Ddi_VarsetIntersectAcc(psInCoi, Ddi_VarsetarrayRead(coiVars, n - 1));
        if (!Ddi_VarInVarset(psInCoi, p)) {
          Ddi_VararrayInsertLast(pi, p);
          Ddi_VararrayRemove(ps, i);
          Ddi_VararrayRemove(ns, i);
        }
        Ddi_Free(psInCoi);
      }
      // Disabled for BDD blow up
      // Ddi_BddExistProject(assume,Ddi_VarsetarrayRead(coiVars,n-1));
    }

  } else {
    reducedTRBdd = Ddi_BddMakeConst(ddm, 1);
    Ddi_BddSetPartConj(reducedTRBdd);
  }

  Ddi_Free(coiVars);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "COI Original TR: Size=%d; #Partitions=%d.\n",
      Ddi_BddSize(Tr_TrBdd(tr)), Ddi_BddPartNum(Tr_TrBdd(tr)))
    );

  TrTrRelWrite(tr, reducedTRBdd);
  Ddi_Free(reducedTRBdd);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "COI Reduced TR: Size=%d; #Partitions=%d.\n",
      Ddi_BddSize(Tr_TrBdd(tr)), Ddi_BddPartNum(Tr_TrBdd(tr)))
    );
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
Ddi_Varsetarray_t *
Tr_TrComputeCoiVars(
  Tr_Tr_t * tr,
  Ddi_Bdd_t * p,
  int maxIter
)
{
  Ddi_Varsetarray_t *coirings;
  Ddi_Varset_t *ps, *ns, *supp, *cone, *New, *newnew;
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Ddi_Bdd_t *trBdd;
  Ddi_Vararray_t *psv;
  Ddi_Vararray_t *nsv;
  Ddi_Varsetarray_t *domain, *range;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(p);

  int i, j, np;

  return computeCoiVars(tr,p,maxIter);

  // disabled for aux var handling
  
  psv = Tr_MgrReadPS(trMgr);
  nsv = Tr_MgrReadNS(trMgr);
  ps = Ddi_VarsetMakeFromArray(psv);
  ns = Ddi_VarsetMakeFromArray(nsv);

  coirings = Ddi_VarsetarrayAlloc(ddiMgr, 0);

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "COI Reduction: ")
    );

  trBdd = Ddi_BddDup(Tr_TrBdd(tr));
  if (!(Ddi_BddIsPartConj(trBdd) || Ddi_BddIsPartDisj(trBdd))) {
    Pdtutil_Warning(1, "Partitioned TR required for COI reduction\n");
    Ddi_BddSetPartConj(trBdd);
  }
  np = Ddi_BddPartNum(trBdd);
  domain = Ddi_VarsetarrayAlloc(ddiMgr, np);
  range = Ddi_VarsetarrayAlloc(ddiMgr, np);
  for (i = 0; i < np; i++) {
    supp = Ddi_BddSupp(Ddi_BddPartRead(trBdd, i));
    Ddi_VarsetIntersectAcc(supp, ps);
    Ddi_VarsetarrayWrite(domain, i, supp);
    Ddi_Free(supp);
    supp = Ddi_BddSupp(Ddi_BddPartRead(trBdd, i));
    Ddi_VarsetIntersectAcc(supp, ns);
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
    newnew = Ddi_VarsetVoid(ddiMgr);
    for (i = 0; i < np; i++) {
      Ddi_Varset_t *common;

      supp = Ddi_VarsetarrayRead(range, i);
      common = Ddi_VarsetIntersect(supp, New);
      if (!Ddi_VarsetIsVoid(common)) {
#if 0
        if ((opt->pre.coiSets != NULL) && (opt->pre.coiSets[i] != NULL)) {
          Ddi_VarsetUnionAcc(cone, opt->pre.coiSets[i]);
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

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, ".(%d)", Ddi_VarsetNum(cone))
      );
  }
  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c, fprintf(stdout, "\n")
    );

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "COI Reduction Performed: Kept %d State Vars out of %d.\n",
      Ddi_VarsetNum(cone), Ddi_VarsetNum(ps)));

  Ddi_Free(New);
  Ddi_Free(domain);
  Ddi_Free(range);
  Ddi_Free(trBdd);
  Ddi_Free(ps);
  Ddi_Free(ns);

  Ddi_Free(cone);

  return (coirings);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Tr_TrCoiReductionWithVars(
  Tr_Tr_t * tr,
  Ddi_Varset_t * coiVars,
  int maxIter
)
{
  Ddi_Varset_t *nocoi, *nocoiNS;
  Ddi_Bdd_t *reducedTRBdd;
  Tr_Tr_t *reducedTr;
  Ddi_Vararray_t *ps = Tr_MgrReadPS(Tr_TrMgr(tr));
  Ddi_Vararray_t *ns = Tr_MgrReadNS(Tr_TrMgr(tr));

  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(coiVars);

  nocoi = Ddi_VarsetMakeFromArray(ps);
  Ddi_VarsetDiffAcc(nocoi, coiVars);
  nocoiNS = Ddi_VarsetSwapVars(nocoi, ps, ns);
#if 1
  Ddi_VarsetUnionAcc(nocoi, nocoiNS);
#else
  Ddi_Free(nocoi);
  nocoi = Ddi_VarsetDup(nocoiNS);

  Ddi_Free(nocoi);
  nocoi = Ddi_VarsetVoid(ddiMgr);
#endif
  Ddi_Free(nocoiNS);

  reducedTRBdd = Part_BddMultiwayLinearAndExist(Tr_TrBdd(tr), nocoi, 1);

  Ddi_Free(nocoi);

#if 0
  nocoi = Ddi_BddSupp(reducedTRBdd);
  opt->trav.univQuantifyVars = Ddi_VarsetMakeFromArray(ps);
  Ddi_VarsetIntersectAcc(opt->trav.univQuantifyVars, nocoi);
  Ddi_Free(nocoi);
  Ddi_VarsetDiffAcc(opt->trav.univQuantifyVars, coiVars);
#endif

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Original TR: Size=%d; #Partitions=%d.\n",
      Ddi_BddSize(Tr_TrBdd(tr)), Ddi_BddPartNum(Tr_TrBdd(tr)))
    );

  reducedTr = Tr_TrMakeFromRel(Tr_TrMgr(tr), reducedTRBdd);

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Reduced TR: Size=%d; #Partitions=%d\n",
      Ddi_BddSize(reducedTRBdd), Ddi_BddPartNum(reducedTRBdd)));

  Ddi_Free(reducedTRBdd);

  return (reducedTr);
}

/**Function********************************************************************
  Synopsis    [TR Cone Of Influence reduction.]
  Description [TR Cone Of Influence reduction. Transitive fanin of property
               is kept.]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
void
Tr_TrEquivReduction(
  Tr_Tr_t * tr /* transition relation */ ,
  Ddi_Bdd_t * initState /* initial state */ ,
  int *eqLemmas /* lemmas for equivalent vars */ ,
  int assumeLemmas              /* if 1 lemmas are assumed true */
)
{
  Ddi_Vararray_t *ps = Tr_MgrReadPS(Tr_TrMgr(tr));
  Ddi_Vararray_t *ns = Tr_MgrReadNS(Tr_TrMgr(tr));
  int n, n1, i, j, assumedEquiv, provedEquiv;
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Ddi_Bdd_t *trBdd = Tr_TrBdd(tr);

  Ddi_Mgr_t *ddm = Ddi_ReadMgr(trBdd);

  Pdtutil_Assert(Ddi_BddIsPartConj(trBdd),
    "Conj. part TR required by Eq reduction");
  n = n1 = Ddi_BddPartNum(trBdd);
  Pdtutil_Assert(n == Ddi_VararrayNum(ps),
    "Bit level TR part required by Eq. reduction");

  do {
    Ddi_Vararray_t *refVars = Ddi_VararrayAlloc(ddm, 0);
    Ddi_Bddarray_t *mergeBdds = Ddi_BddarrayAlloc(ddm, 0);
    Ddi_Bdd_t *mergedTrBdd = Ddi_BddDup(trBdd);

    assumedEquiv = 0;
    for (j = 0; j < n; j++) {
      if ((i = eqLemmas[j]) >= 0) {
        Ddi_Bdd_t *merge = Ddi_BddMakeLiteral(Ddi_VararrayRead(ps, i), 1);

        Ddi_VararrayInsertLast(refVars, Ddi_VararrayRead(ps, j));
        Ddi_BddarrayInsertLast(mergeBdds, merge);
        Ddi_Free(merge);
#if 1
        merge = Ddi_BddMakeLiteral(Ddi_VararrayRead(ns, i), 1);
        Ddi_VararrayInsertLast(refVars, Ddi_VararrayRead(ns, j));
        Ddi_BddarrayInsertLast(mergeBdds, merge);
        Ddi_Free(merge);
#endif
        assumedEquiv++;
      }
    }
    Ddi_BddComposeAcc(mergedTrBdd, refVars, mergeBdds);
    provedEquiv = 0;
    for (j = 0; j < n; j++) {
      if ((i = eqLemmas[j]) >= 0) {
        Ddi_Bdd_t *p_i = Ddi_BddPartRead(mergedTrBdd, i);
        Ddi_Bdd_t *p_j = Ddi_BddPartRead(mergedTrBdd, j);

        if (assumeLemmas || Ddi_BddEqual(p_i, p_j)) {
          provedEquiv++;
        } else {
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "Not equivalent DISS: %s != %s.\n",
              Ddi_VarName(Ddi_VararrayRead(ps, j)),
              Ddi_VarName(Ddi_VararrayRead(ps, eqLemmas[j])))
            );
          eqLemmas[j] = -1;
        }
      }
    }

    Ddi_Free(mergedTrBdd);
    if (provedEquiv == assumedEquiv && provedEquiv > 0) {
      Ddi_BddComposeAcc(trBdd, refVars, mergeBdds);
      if (initState != NULL) {
        Ddi_BddComposeAcc(initState, refVars, mergeBdds);
      }
      for (j = 0; j < n; j++) {
        if (eqLemmas[j] >= 0) {
          Ddi_BddPartWrite(trBdd, j, Ddi_MgrReadOne(ddm));
        }
      }
    } else {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Equivalence Reduction: Next iteration required.\n")
        );
    }
    Ddi_Free(refVars);
    Ddi_Free(mergeBdds);
  } while (provedEquiv < assumedEquiv);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "Eq. Reduced TR: Size=%d; #Partitions=%d->%d.\n",
      Ddi_BddSize(Tr_TrBdd(tr)), n, n - provedEquiv)
    );
}

/**Function*******************************************************************
  Synopsis    [write TR partitions to file]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Tr_TrWritePartitions(
  Tr_Tr_t * tr,
  char *file
)
{
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Ddi_Bdd_t *trBdd = Tr_TrBdd(tr);
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(trBdd);
  FILE *fp;
  int i, j;
  Ddi_Vararray_t *nsv = Tr_MgrReadNS(trMgr);

  if (!Ddi_BddIsPartConj(trBdd)) {
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "NO conj part TR for manual partitioning.\n"));

    return;
  }

  fp = fopen(file, "w");
  if (fp == NULL) {
    return;
  }

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Writing TR partitions to file %s.\n", file));

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(fp, "# tr total BDD size: %d\n", Ddi_BddSize(trBdd)));

  for (i = 0; i < Ddi_BddPartNum(trBdd); i++) {
    Ddi_Bdd_t *tr_i = Ddi_BddPartRead(trBdd, i);
    Ddi_Varset_t *supp = Ddi_BddSupp(tr_i);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(fp, "#\n# tr component %d - BDD size: %d\n", i,
        Ddi_BddSize(tr_i)));

    for (j = 0; j < Ddi_VararrayNum(nsv); j++) {
      Ddi_Var_t *v = Ddi_VararrayRead(nsv, j);

      if (Ddi_VarInVarset(supp, v)) {
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
          fprintf(fp, "%s\n", Ddi_VarName(v)));
      }
    }
    Ddi_Free(supp);
  }

  // Ddi_Free(nsv);

  fclose(fp);

}



/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Tr_TrProjectOverPartitions(
  Ddi_Bdd_t * f,
  Tr_Tr_t * TR,
  Ddi_Bdd_t * care,
  int nPart,
  int approxPreimgTh
)
{
  Ddi_Varset_t *ps, *ns, *smooth0, *smooth1, *supp;
  Tr_Mgr_t *trMgr;
  Ddi_Bdd_t *trBdd, *r, *r0, *r1;
  Ddi_Vararray_t *psv;
  Ddi_Vararray_t *nsv;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(f);
  int i;

  r = Ddi_BddDup(f);

  if (TR == NULL) {
    int l, n;

    supp = Ddi_BddSupp(f);
    n = Ddi_VarsetNum(supp);

    for (i = 0; i < nPart; i++) {

      if (Ddi_BddSize(r) < approxPreimgTh)
        break;

      smooth1 = Ddi_VarsetVoid(ddiMgr);

      for (l = 0, Ddi_VarsetWalkStart(supp); !Ddi_VarsetWalkEnd(supp);
        Ddi_VarsetWalkStep(supp)) {
        if ((l < (i * n) / nPart) || (l >= ((i + 1) * n) / nPart)) {
          Ddi_VarsetAddAcc(smooth1, Ddi_VarsetWalkCurr(supp));
        }
        l++;
      }
      smooth0 = Ddi_VarsetDiff(supp, smooth1);

      r0 = Ddi_BddExistOverApprox(r, smooth1);
      r1 = Ddi_BddExistOverApprox(r, smooth0);
      Ddi_Free(smooth0);
      Ddi_Free(smooth1);

      if (Ddi_BddIsOne(r0) && Ddi_BddIsOne(r1)) {
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "@"));
      } else {
        Ddi_Free(r);
        r = bddAndInterleaved(r0, r1);
        if (care != NULL) {
          Ddi_BddRestrictAcc(r, care);
        }
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "#"));
      }
      fflush(stdout);
      Ddi_Free(r0);
      Ddi_Free(r1);

    }
    Ddi_Free(supp);
  } else {

    if (Ddi_BddIsPartConj(r) && (Ddi_BddPartNum(r) > 1)) {
      return (r);
    }

    trMgr = Tr_TrMgr(TR);
    psv = Tr_MgrReadPS(trMgr);
    nsv = Tr_MgrReadNS(trMgr);

    trBdd = Tr_TrBdd(TR);

    ps = Ddi_VarsetMakeFromArray(psv);
    ns = Ddi_VarsetMakeFromArray(nsv);

    for (i = 0; i < Ddi_BddPartNum(trBdd); i++) {

      if (Ddi_BddSize(r) < approxPreimgTh)
        break;

      supp = Ddi_BddSupp(Ddi_BddPartRead(trBdd, i));
      Ddi_VarsetSwapVarsAcc(supp, psv, nsv);
      smooth1 = Ddi_VarsetIntersect(supp, ps);
      smooth0 = Ddi_VarsetDiff(ps, supp);
      Ddi_Free(supp);

      r0 = Ddi_BddExistOverApprox(r, smooth1);
      r1 = Ddi_BddExistOverApprox(r, smooth0);
      Ddi_Free(smooth0);
      Ddi_Free(smooth1);

      if (Ddi_BddIsOne(r0) && Ddi_BddIsOne(r1)) {
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "@"));
      } else {
        Ddi_Free(r);
        r = bddAndInterleaved(r0, r1);
        if (care != NULL) {
          Ddi_BddRestrictAcc(r, care);
        }
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "#"));
      }
      fflush(stdout);
      Ddi_Free(r0);
      Ddi_Free(r1);

    }
    Ddi_Free(ps);
    Ddi_Free(ns);
  }

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "\n%d\n", Ddi_BddSize(r)));

  return (r);
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************
  Synopsis    [manual partitioning of TR based on partition file]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Tr_Tr_t *
trManualPartitionsIds(
  Tr_Tr_t * tr,
  char *file
)
{
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Ddi_Bdd_t *trBdd = Tr_TrBdd(tr), *trBdd2;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(trBdd);
  Tr_Tr_t *tr2;

  FILE *fp;
  char buf[PDTUTIL_MAX_STR_LEN + 1];
  int i, np, fileNp, nCl, nClMax;
  Ddi_Bdd_t *part, *part2;

  Ddi_Var_t *v;
  Ddi_Varset_t *supp;

  if (!Ddi_BddIsPartConj(trBdd)) {
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "NO conj part TR for manual partitioning.\n"));

    return (Tr_TrDup(tr));
  }

  np = Ddi_BddPartNum(trBdd);
  nCl = 0;
  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Reading partitions file %s.\n", file));

  fp = fopen(file, "r");
  if (fp == NULL) {
    return (Tr_TrDup(tr));
  }


  fscanf(fp, "%d%d", &fileNp, &nClMax);
  nCl = 0;

  Pdtutil_Assert(fileNp <= np, "file part num does not match TR part num");

  trBdd2 = Ddi_BddMakePartConjVoid(ddiMgr);

  for (i = 0; i < fileNp; i++) {
    int nread, p, c;

    nread = fscanf(fp, "%d%d", &p, &c);
    //MATLAB clustering
    //p--;
    //c--;
    Pdtutil_Assert(nread == 2, "error reading partition from file");
    Pdtutil_Assert(c > 0 && p > 0, "wrong part index");
    c--;
    p--;
    Pdtutil_Assert(p < Ddi_BddPartNum(trBdd), "wrong part index");
    while (nCl <= c) {
      part2 = Ddi_BddMakePartConjVoid(ddiMgr);
      Ddi_BddPartInsertLast(trBdd2, part2);
      Ddi_Free(part2);
      nCl++;
    }
    // get cluster 
    while (c >= nCl) {
      part2 = Ddi_BddMakePartConjVoid(ddiMgr);
      Ddi_BddPartInsertLast(trBdd2, part2);
      Ddi_Free(part2);
      nCl++;
    }
    part2 = Ddi_BddPartRead(trBdd2, c);
    // add new partition
    Ddi_BddPartInsertLast(part2, Ddi_BddPartRead(trBdd, p));
  }

  Pdtutil_Assert(nCl <= nClMax, "problem in cluster file: too many clusters");

  if (np > fileNp) {
    part2 = Ddi_BddMakePartConjVoid(ddiMgr);
    for (i = fileNp; i < np; i++) {
      Ddi_BddPartInsertLast(part2, Ddi_BddPartRead(trBdd, i));
    }
    Ddi_BddPartInsertLast(trBdd2, part2);
    Ddi_Free(part2);
  }

  tr2 = Tr_TrMakeFromRel(trMgr, trBdd2);
  Ddi_Free(trBdd2);

  return (tr2);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Varsetarray_t *
computeCoiVars(
  Tr_Tr_t * tr,
  Ddi_Bdd_t * p,
  int maxDepth                  /* maximun allowed depth of COI */
)
{
  Ddi_Varsetarray_t *coirings;
  Ddi_Varset_t *ps, *ns, *ps_aux, *aux, *supp, *cone, *newvs, *newnew;
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Ddi_Bdd_t *trBdd;
  Ddi_Vararray_t *psv;
  Ddi_Vararray_t *nsv;
  Ddi_Varsetarray_t *domain, *range;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(p);

  int i, np, nv, naux, niter;

  Ddi_MgrSiftSuspend(ddm);

  psv = Tr_MgrReadPS(trMgr);
  nsv = Tr_MgrReadNS(trMgr);
  ps = Ddi_VarsetMakeFromArray(psv);
  ns = Ddi_VarsetMakeFromArray(nsv);


  coirings = Ddi_VarsetarrayAlloc(ddm, 0);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "COI Reduction: ")
    );

  trBdd = Ddi_BddDup(Tr_TrBdd(tr));
  if (!(Ddi_BddIsPartConj(trBdd) || Ddi_BddIsPartDisj(trBdd))) {
    Pdtutil_Warning(1, "Partitioned TR required for COI reduction.\n");
    Ddi_BddSetPartConj(trBdd);
  }
  np = Ddi_BddPartNum(trBdd);
  domain = Ddi_VarsetarrayAlloc(ddm, np);
  range = Ddi_VarsetarrayAlloc(ddm, np);

  nv = Ddi_VararrayNum(psv);
  naux = 0;
  aux = NULL;
  if (trMgr->auxVars != NULL) {
    naux = Ddi_VararrayNum(trMgr->auxVars);
    aux = Ddi_VarsetMakeFromArray(trMgr->auxVars);
    Pdtutil_Assert(Ddi_VarsetNum(aux) == naux, "wrong set of aux vars");
    ps_aux = Ddi_VarsetUnion(aux, ps);
  } else {
    ps_aux = Ddi_VarsetDup(ps);
  }
  Pdtutil_Assert(tr->bitTr == 1, "Bit TR required by COI reduction");
  Pdtutil_Assert((nv + naux) == np,
    "Wrong # of partitions for bit TR in COI reduction");

  for (i = 0; i < nv; i++) {
    supp = Ddi_BddSupp(Ddi_BddPartRead(trBdd, i));
    Ddi_VarsetIntersectAcc(supp, ps_aux);
    Ddi_VarsetarrayWrite(domain, i, supp);
    Ddi_Free(supp);
    supp = Ddi_BddSupp(Ddi_BddPartRead(trBdd, i));
    Ddi_VarsetIntersectAcc(supp, ns);
    Ddi_VarsetarrayWrite(range, i, supp);
    Ddi_Free(supp);
  }
  for (i = 0; i < naux; i++) {
    Ddi_Varset_t *auxVarset =
      Ddi_VarsetMakeFromVar(Ddi_VararrayRead(trMgr->auxVars, i));
    supp = Ddi_BddSupp(Ddi_BddPartRead(trBdd, nv + i));
    Ddi_VarsetIntersectAcc(supp, ps_aux);
    Ddi_VarsetarrayWrite(domain, nv + i, supp);
    Ddi_Free(supp);
    supp = Ddi_BddSupp(Ddi_BddPartRead(trBdd, nv + i));
    Ddi_VarsetIntersectAcc(supp, auxVarset);
    Pdtutil_Assert(!Ddi_VarsetIsVoid(supp), "Wrong aux part in bit TR");
    Ddi_VarsetarrayWrite(range, nv + i, supp);
    Ddi_Free(supp);
    Ddi_Free(auxVarset);
  }

  cone = Ddi_BddSupp(p);
  // Keep PIs as COI is made by PIs and PSs
  //  Ddi_VarsetIntersectAcc(cone,ps_aux); /* remove PI vars */

  newvs = Ddi_VarsetDup(cone);
  niter = 0;
  while (!Ddi_VarsetIsVoid(newvs) && ((maxDepth <= 0) || (niter++ < maxDepth))) {
    int newAux = 0;

    Ddi_VarsetUnionAcc(cone, newvs);
    Ddi_VarsetarrayInsertLast(coirings, cone);
    Ddi_VarsetarrayInsertLast(coirings, newvs);
    Ddi_VarsetSwapVarsAcc(newvs, psv, nsv);
    newnew = Ddi_VarsetVoid(ddm);
    do {
      for (i = 0; i < np; i++) {
        Ddi_Varset_t *common;

        supp = Ddi_VarsetarrayRead(range, i);
        common = Ddi_VarsetIntersect(supp, newvs);
        if (!Ddi_VarsetIsVoid(common)) {
          Ddi_VarsetUnionAcc(newnew, Ddi_VarsetarrayRead(domain, i));
        }
        Ddi_Free(common);
      }
      Ddi_VarsetDiffAcc(newnew, cone);
#if 1
      if (aux != NULL) {
        Ddi_Free(newvs);
        newvs = Ddi_VarsetIntersect(newnew, aux);
        Ddi_VarsetDiffAcc(newvs, cone);
        Ddi_VarsetUnionAcc(cone, newvs);
        newAux = !Ddi_VarsetIsVoid(newvs);
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
          fprintf(stdout, "a[%d]", Ddi_VarsetNum(newvs));
          fflush(stdout)
          );
      }
#endif
    } while (newAux);
    Ddi_Free(newvs);
    newvs = Ddi_VarsetDiff(newnew, cone);
    Ddi_Free(newnew);
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, ".(%d)", Ddi_VarsetNum(cone)););
  }

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
    int npscone;
    int ncone = Ddi_VarsetNum(cone);
    Ddi_VarsetIntersectAcc(cone, ps);
    npscone = Ddi_VarsetNum(cone);
    fprintf(stdout, "\n");
    fprintf(stdout,
      "COI Reduction Performed: Kept %d(%d St. +%d I) Vars out of %d(%d+%d).\n",
      ncone, npscone, ncone - npscone, nv + naux, nv, naux)
    );

  Ddi_MgrSiftResume(ddm);

  Ddi_Free(newvs);
  Ddi_Free(domain);
  Ddi_Free(range);
  Ddi_Free(trBdd);
  Ddi_Free(ps);
  Ddi_Free(ns);
  Ddi_Free(ps_aux);
  Ddi_Free(aux);

  Ddi_Free(cone);

  return (coirings);

}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static void
storeDependancyGraph(
  char *fname,
  Tr_Tr_t * tr,
  Ddi_Varset_t * core
)
{
  Ddi_Varset_t *ps, *ns, *supp;
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Ddi_Bdd_t *trBdd;
  Ddi_Vararray_t *psv;
  Ddi_Vararray_t *nsv;

  int i, id_i, np, nv, nMgrVars, *var2psId; // ,id_j;
  Ddi_Mgr_t *ddm = trMgr->DdiMgr;
  FILE *fp = fopen(fname, "w");

  psv = Tr_MgrReadPS(trMgr);
  nsv = Tr_MgrReadNS(trMgr);
  ps = Ddi_VarsetMakeFromArray(psv);
  ns = Ddi_VarsetMakeFromArray(nsv);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "TR Dependency Graph Store.\n")
    );

  nv = Ddi_VararrayNum(psv);
  nMgrVars = Ddi_MgrReadNumCuddVars(ddm);

  var2psId = Pdtutil_Alloc(int,
    nMgrVars
  );

  for (i = 0; i < nMgrVars; i++) {
    var2psId[i] = -1;
  }
  for (i = 0; i < nv; i++) {
    id_i = Ddi_VarCuddIndex(Ddi_VararrayRead(psv, i));
    var2psId[id_i] = i;
    //id_i = Ddi_VarIndex(Ddi_VararrayRead(nsv,i));
    //var2psId[id_i] = i;
  }
  fprintf(fp, "%d.\n", nv - 1);

  trBdd = Ddi_BddDup(Tr_TrBdd(tr));
  if (!(Ddi_BddIsPartConj(trBdd) || Ddi_BddIsPartDisj(trBdd))) {
    Pdtutil_Warning(1, "Partitioned TR required for COI reduction.\n");
    Ddi_BddSetPartConj(trBdd);
  }
  np = Ddi_BddPartNum(trBdd);

  Pdtutil_Assert(tr->bitTr == 1, "Bit TR required by DEP GR. STORE");
  Pdtutil_Assert((nv) == np,
    "Wrong # of partitions for bit TR in DEP GR. STORE");

  //Ddi_VarsetUnionAcc(ps, ns);
  for (i = 0; i < nv; i++) {
    supp = Ddi_BddSupp(Ddi_BddPartRead(trBdd, i));
    Ddi_VarsetIntersectAcc(supp, ps);
    id_i = Ddi_VarCuddIndex(Ddi_VararrayRead(psv, i));

    for (Ddi_VarsetWalkStart(supp);
      !Ddi_VarsetWalkEnd(supp); Ddi_VarsetWalkStep(supp)) {
      int id_j = Ddi_VarCuddIndex(Ddi_VarsetWalkCurr(supp));

      fprintf(fp, "%d %d\n", var2psId[id_j], var2psId[id_i]);
    }
    Ddi_Free(supp);
  }

  fclose(fp);
  Pdtutil_Free(var2psId);
  Ddi_Free(trBdd);
  Ddi_Free(ps);
  Ddi_Free(ns);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static sccGraph_t *
loadSCCGraph(
  char *fname,
  Tr_Tr_t * tr
)
{
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Ddi_Vararray_t *psv = Tr_MgrReadPS(trMgr);
  sccGraph_t *sccGraph;
  int i, j, nn, ne, id;
  Ddi_Mgr_t *ddm = trMgr->DdiMgr;
  FILE *fp = fopen(fname, "r");

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "SCC Dependency Graph Load.\n")
    );

  fscanf(fp, "%d", &nn);
  sccGraph = Pdtutil_Alloc(sccGraph_t, 1);
  sccGraph->nEdges = 0;
  sccGraph->nNodes = nn;
  sccGraph->nodes = Pdtutil_Alloc(sccNode_t, nn);

  for (i = 0; i < nn; i++) {
    /* variables */
    sccGraph->nodes[i].vars = Ddi_VarsetVoid(ddm);
    fscanf(fp, "%d", &ne);
    for (j = 0; j < ne; j++) {
      fscanf(fp, "%d", &id);
      Ddi_VarsetAddAcc(sccGraph->nodes[i].vars, Ddi_VararrayRead(psv, id));
    }
    /* in-coming edges */
    fscanf(fp, "%d", &ne);
    sccGraph->nEdges += ne;
    sccGraph->nodes[i].nFrom = ne;
    sccGraph->nodes[i].from = Pdtutil_Alloc(int,
      ne
    );

    for (j = 0; j < ne; j++) {
      fscanf(fp, "%d", &id);
      sccGraph->nodes[i].from[j] = id;
    }
    /* out-going edges */
    fscanf(fp, "%d", &ne);
    sccGraph->nodes[i].nTo = ne;
    sccGraph->nodes[i].to = Pdtutil_Alloc(int,
      ne
    );

    for (j = 0; j < ne; j++) {
      fscanf(fp, "%d", &id);
      sccGraph->nodes[i].to[j] = id;
    }

  }
  fclose(fp);
  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "# Parsed SCCs: %d.\n", sccGraph->nNodes)
    );

  return sccGraph;

}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Varset_t *
suppWithAuxVars(
  Ddi_Bdd_t * f,
  Tr_Mgr_t * trMgr
)
{
  Ddi_Vararray_t *psv;
  Ddi_Vararray_t *nsv;
  Ddi_Vararray_t *auxv;
  Ddi_Bddarray_t *auxF;
  Ddi_Varset_t *supp;

  //Ddi_Varsetarray_t *domain, *range;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);

  int i, again;                 //,np,nv,naux,niter;

  Ddi_MgrSiftSuspend(ddm);

  psv = Tr_MgrReadPS(trMgr);
  nsv = Tr_MgrReadNS(trMgr);

  supp = Ddi_BddSupp(f);
  if ((auxv = trMgr->auxVars) == NULL) {
    return (supp);
  }
  auxF = trMgr->auxFuns;
  do {
    again = 0;
    for (i = 0; i < Ddi_VararrayNum(auxv); i++) {
      Ddi_Var_t *v = Ddi_VararrayRead(auxv, i);

      if (Ddi_VarInVarset(supp, v)) {
        Ddi_Varset_t *newvs = Ddi_BddSupp(Ddi_BddarrayRead(auxF, i));

        Ddi_VarsetRemoveAcc(supp, v);
        Ddi_VarsetUnionAcc(supp, newvs);
        Ddi_Free(newvs);
        again = 1;
      }
    }
  } while (again);

  return (supp);

}



/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
bddAndInterleaved(
  Ddi_Bdd_t * f,
  Ddi_Bdd_t * g
)
{
  Ddi_Bdd_t *pf, *pg, *r;
  int i;

  if (!Ddi_BddIsPartConj(f) || !Ddi_BddIsPartConj(g)) {
    return (Ddi_BddAnd(f, g));
  }

  r = Ddi_BddDup(f);

  for (i = 0; i < Ddi_BddPartNum(g); i++) {
    pg = Ddi_BddPartRead(g, i);
    if (i >= Ddi_BddPartNum(f)) {
      Ddi_BddPartInsertLast(r, pg);
    } else {
      pf = Ddi_BddPartRead(r, i);
      Ddi_BddAndAcc(pf, pg);
    }
  }

  return (r);

}

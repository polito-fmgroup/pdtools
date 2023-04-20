/**CFile**********************************************************************

  FileName    [trSort.c]

  PackageName [Tr]

  Synopsis    [Sort a BDDs'array]

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
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

typedef struct PartitionInfo_t {
  Ddi_Bdd_t *F;                 /* BDD root of the function */
  Ddi_Varset_t *SmoothV;        /* Variables that will be smoothed in img/preimg */
  Ddi_Varset_t *RangeV;         /* Variables in the range of F */
  int id,                       /* function identifier in a struct array */
   size,                        /* BDD size of F */
   SmoothN,                     /* number of smoothing variables */
   LocalSmoothN,                /* number of local smoothing variables */
   RangeN,                      /* number of range variables */
   TopRangePos,                 /* top range variable (pos. in the ordering) */
   TopSmoothPos,                /* top smooth variable (pos. in the ordering) */
   BottomSmoothPos,             /* bottom smooth variable (pos. in the ordering) */
   AverageSmoothPos;            /* average smooth variable pos. in the ordering */
} PartitionInfo_t;

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/* shell to qsort used for MaxMin and Size sorting */
#define PartitionInfoArraySort(PartInfoArray,n,CompareFunction) \
  qsort((void **)PartInfoArray,n,sizeof(PartitionInfo_t *),CompareFunction)

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static int MaxMinVSupportCompare(
  const void *s1,
  const void *s2
);
static int BddOrdCompare(
  const void *s1,
  const void *s2
);
static int BddSizeCompare(
  const void *s1,
  const void *s2
);
static int WeightedSort(
  Tr_Mgr_t * trMgr,
  Ddi_Mgr_t * dd,
  PartitionInfo_t ** FSa,
  int nPart,
  Tr_Sort_e method
);
static int WeightedSortPdt(
  Tr_Mgr_t * trMgr,
  Ddi_Mgr_t * dd,
  PartitionInfo_t ** FSa,
  int nPart,
  Tr_Sort_e method
);

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of internal function                                           */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of external function                                           */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************

  Synopsis    [Sorting an array of BDDs]
  Description [Given an BDDs'array , the function sorts the BDDs in the
    array in order to find the best sort for compute esistential
    abstraction of BDDs product. The smoothing variable set should
    include the quantifying ones, too.
    It returns 1 if successfully sorted, 0 otherwise.]
  SideEffects [none]
  SeeAlso     []

*****************************************************************************/

int
Tr_TrSortIwls95(
  Tr_Tr_t * tr
)
{
  int retval;                   /* default is correct return */
  Ddi_Varset_t *pivars;         /* Set of smoothing variables */
  Ddi_Varset_t *svars;          /* Set of smoothing variables */
  Ddi_Varset_t *rvars;          /* Set of range variables */
  Ddi_Bdd_t *F;                 /* Partitioned TR rel */
  Tr_Mgr_t *trMgr;
  Ddi_Mgr_t *dd;                /* dd manager */

  trMgr = tr->trMgr;
  F = Tr_TrBdd(tr);
  dd = Ddi_ReadMgr(F);

  pivars = Ddi_VarsetMakeFromArray(trMgr->i);
#if 1
  svars = Ddi_VarsetMakeFromArray(trMgr->ps);
#if 0
  Ddi_VarsetUnionAcc(svars, pivars);
#endif
#else
  svars = Ddi_VarsetVoid(dd);
#endif
  Ddi_Free(pivars);
  rvars = Ddi_VarsetMakeFromArray(trMgr->ns);

  if (Ddi_BddIsPartDisj(F)) {
    int i, ret;

    retval = 1;
    for (i = 0; i < Ddi_BddPartNum(F); i++) {
      Ddi_Bdd_t *clk_p = NULL;
      Ddi_Bdd_t *p = Ddi_BddPartRead(F, i);

      if (tr->clk != NULL) {
        Pdtutil_Assert(Ddi_BddIsPartConj(p), "no conj. decomp for TR");
        clk_p = Ddi_BddPartExtract(p, 0);
      }
      ret = !Ddi_BddIsPartConj(p)
        || Tr_TrBddSortIwls95(trMgr, p, svars, rvars);
      if (clk_p != NULL) {
        Ddi_BddPartInsert(p, 0, clk_p);
        Ddi_Free(clk_p);
      }
      retval &= ret;
    }
  } else {
    int i, ret;

    retval = Tr_TrBddSortIwls95(trMgr, F, svars, rvars);
    for (i = 0; retval && (i < Ddi_BddPartNum(F)); i++) {
      Ddi_Bdd_t *p = Ddi_BddPartRead(F, i);

      if (Ddi_BddIsPartConj(p)) {
        ret = Tr_TrBddSortIwls95(trMgr, p, svars, rvars);
        retval &= ret;
      }
    }
  }

  Ddi_Free(svars);
  Ddi_Free(rvars);

  return (retval);

}


/**Function*******************************************************************

  Synopsis    [Sorting an array of BDDs]
  Description [Given an BDDs'array , the function sorts the BDDs in the
    array in order to find the best sort for compute esistential
    abstraction of BDDs product. The smoothing variable set should
    include the quantifying ones, too.
    It returns 1 if successfully sorted, 0 otherwise.]
  SideEffects [none]
  SeeAlso     []

*****************************************************************************/

int
Tr_TrBddSortIwls95(
  Tr_Mgr_t * trMgr,
  Ddi_Bdd_t * F /* Partitioned TR rel */ ,
  Ddi_Varset_t * svars /* Set of smoothing variables */ ,
  Ddi_Varset_t * rvars          /* Set of range variables */
)
{
  PartitionInfo_t **FSa;        /* pointer to a vector of struct pointers */
  PartitionInfo_t *FS;          /* struct pointer */
  Ddi_Varset_t *supp;           /* function support */
  int n, i;
  int retval = 1;               /* default is correct return */
  Tr_Sort_e method;
  Ddi_Mgr_t *dd;                /* dd manager */

  if (trMgr == NULL) {
    method = Tr_SortWeight_c;
  } else {
    method = Tr_MgrReadSortMethod(trMgr);
  }
  dd = Ddi_ReadMgr(F);

  if (method == Tr_SortNone_c) {
    return (1);
  }

  /* disable dynamic ordering if enabled */
  Ddi_MgrSiftSuspend(dd);

  n = Ddi_BddPartNum(F);

  FSa = Pdtutil_Alloc(PartitionInfo_t *, n);
  if (FSa == NULL) {
    fprintf(stderr, "Error: Out of memory.\n");
    return 0;
  }
  /* init with NULL pointers for safe free in case of failure */
  for (i = 0; i < n; i++) {
    FSa[i] = NULL;
  }

  /*
   *  Load array of partition infos
   */

  for (i = 0; i < n; i++) {
    FS = Pdtutil_Alloc(PartitionInfo_t, 1);
    if (FS == NULL) {
      fprintf(stderr, "Error: Out of memory.\n");
      retval = 0;
      goto end_sort;
    }

    FS->F = Ddi_BddDup(Ddi_BddPartRead(F, i));  /* F function */
    FS->id = i;                 /* id = original array index */
    FS->size = Ddi_BddSize(FS->F);  /* BDD size of F */

    supp = Ddi_BddSupp(FS->F);  /* support of F */
    FS->SmoothV = Ddi_VarsetIntersect(supp, svars);
    FS->RangeV = Ddi_VarsetIntersect(supp, rvars);
    Ddi_Free(supp);

    FS->SmoothN = Ddi_VarsetNum(FS->SmoothV);
    FS->RangeN = Ddi_VarsetNum(FS->RangeV);
    if (!Ddi_VarsetIsVoid(FS->SmoothV)) {
      FS->TopSmoothPos = Ddi_VarCurrPos(Ddi_VarsetTop(FS->SmoothV));
      FS->BottomSmoothPos = Ddi_VarCurrPos(Ddi_VarsetBottom(FS->SmoothV));
      FS->AverageSmoothPos = 0;
      for (Ddi_VarsetWalkStart(FS->SmoothV);
        !Ddi_VarsetWalkEnd(FS->SmoothV); Ddi_VarsetWalkStep(FS->SmoothV)) {
        FS->AverageSmoothPos +=
          Ddi_VarCurrPos(Ddi_VarsetWalkCurr(FS->SmoothV));
      }
      FS->AverageSmoothPos /= Ddi_VarsetNum(FS->SmoothV);
    } else {
      FS->TopSmoothPos = -1;
      FS->BottomSmoothPos = -1;
      FS->AverageSmoothPos = -1;
    }
    if (!Ddi_VarsetIsVoid(FS->RangeV)) {
      FS->TopRangePos = Ddi_VarCurrPos(Ddi_VarsetTop(FS->RangeV));
    } else {
      FS->TopRangePos = -1;
    }


    FSa[i] = FS;

  }                             /* end lood info array */

  /*
   * Select sorting method
   */

  switch (method) {

    case Tr_SortNone_c:
    case Tr_SortDefault_c:
      /* default is no sorting */
      break;

    case Tr_SortMinMax_c:
      PartitionInfoArraySort(FSa, n, MaxMinVSupportCompare);
      break;

    case Tr_SortOrd_c:
      PartitionInfoArraySort(FSa, n, BddOrdCompare);
      break;

    case Tr_SortSize_c:
      PartitionInfoArraySort(FSa, n, BddSizeCompare);
      break;

    case Tr_SortWeight_c:
      if (WeightedSort(trMgr, dd, FSa, n, method) > 0) {
        break;
      } else {
        retval = 0;
        goto end_sort;
      }

    case Tr_SortWeightPdt_c:
    case Tr_SortWeightDac99_c:
      if (WeightedSortPdt(trMgr, dd, FSa, n, method) > 0) {
        break;
      } else {
        retval = 0;
        goto end_sort;
      }

    default:
      fprintf(stderr, "Error: Invalid TR sorting method selected.\n");
      retval = 0;
      goto end_sort;
  }

  for (i = 0; i < n; i++) {
    FS = FSa[i];
    Ddi_BddPartWrite(F, i, FS->F);
    Ddi_Free(FS->F);
  }

  if (trMgr != NULL) {
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "Sorted Partitions: ["); for (i = 0; i < n; i++) {
      fprintf(stdout, " %d", FSa[i]->id);}
      fprintf(stdout, " ].\n")
      ) ;
  }

end_sort:

  Ddi_MgrSiftResume(dd);

  for (i = 0; i < n; i++) {
    Ddi_Free(FSa[i]->SmoothV);
    Ddi_Free(FSa[i]->RangeV);
    Pdtutil_Free(FSa[i]);
  }
  Pdtutil_Free(FSa);

  return (retval);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
void
Tr_TrSortForBck(
  Tr_Tr_t * tr
)
{
#if 1

  Tr_TrReverseAcc(tr);
  Tr_TrSortIwls95(tr);
  Tr_TrReverseAcc(tr);

#else

  Ddi_Varset_t *ns, *supp, *suppq;
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Ddi_Bdd_t *trBdd = Tr_TrBdd(tr);
  Ddi_Bdd_t *p, *q;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(trBdd);

  int i, j, np;

  ns = Ddi_VarsetMakeFromArray(Tr_MgrReadNS(trMgr));

  np = Ddi_BddPartNum(trBdd);

  for (i = 0; i < np; i++) {
    p = Ddi_BddPartRead(trBdd, i);
    Ddi_BddSuppAttach(p);
    supp = Ddi_BddSuppRead(p);
    if (abs(opt->sort_for_bck) == 1)
      Ddi_VarsetIntersectAcc(supp, ns);
  }

  for (i = 0; i < np - 1; i++) {
    for (j = i + 1; j < np; j++) {
      int swap;

      p = Ddi_BddPartRead(trBdd, i);
      supp = Ddi_BddSuppRead(p);
      q = Ddi_BddPartRead(trBdd, j);
      suppq = Ddi_BddSuppRead(q);
      if (Ddi_VarsetIsVoid(supp) || Ddi_VarsetIsVoid(suppq)) {
        continue;
      }
      swap = (Ddi_MgrReadPerm(ddiMgr, Ddi_VarCuddIndex(Ddi_VarsetTop(supp))) >
        Ddi_MgrReadPerm(ddiMgr, Ddi_VarCuddIndex(Ddi_VarsetTop(suppq))));
      if (opt->sort_for_bck < 0) {
        swap = !swap;
      }
      if (swap) {
        p = Ddi_BddDup(p);
        q = Ddi_BddDup(q);
        Ddi_BddPartWrite(trBdd, i, q);
        Ddi_BddPartWrite(trBdd, j, p);
        Ddi_Free(p);
        Ddi_Free(q);
      }
    }
  }

  for (i = 0; i < np; i++) {
    p = Ddi_BddPartRead(trBdd, i);
    supp = Ddi_BddSuppRead(p);
    Ddi_BddSuppDetach(p);
  }

  Ddi_Free(ns);
#endif
}


/*---------------------------------------------------------------------------*/
/* Definition of static function                                             */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************

  Synopsis    [Compares BDDs for MAX-MIN-V-SORTING method.]

  Description []

  SideEffects [none]

  SeeAlso     [Tr_BddarraySort()]

*****************************************************************************/

static int
MaxMinVSupportCompare(
  const void *s1,
  const void *s2
)
{
  int a, b, r;
  PartitionInfo_t *pFS1 = *((PartitionInfo_t **) s1);
  PartitionInfo_t *pFS2 = *((PartitionInfo_t **) s2);

  r = 0;

  if ((pFS1->SmoothN == 0) || (pFS2->SmoothN == 0)) {
    a = pFS1->SmoothN;
    b = pFS2->SmoothN;
  } else if (pFS1->BottomSmoothPos != pFS2->BottomSmoothPos) {
    a = pFS1->BottomSmoothPos;
    b = pFS2->BottomSmoothPos;
  } else if (pFS1->TopSmoothPos != pFS2->TopSmoothPos) {
    b = pFS1->TopSmoothPos;
    a = pFS2->TopSmoothPos;
  } else {
    a = pFS1->SmoothN;
    b = pFS2->SmoothN;
  }

  if (a > b)
    r = 1;
  else if (a == b)
    r = 0;
  else if (a < b)
    r = -1;

  return (r);
}


/**Function*******************************************************************

  Synopsis    [Compares BDDs for RANGE TOP var ordering]

  Description [The function compares two BDDs and return:<BR>
    1, if top var ordering position of 1st BDD is greater then 2nd<BR>
    -1, if size of 1st BDD is smaller then 2nd<BR>
    0, if both 1st and 2nd BDD have the same top var.]

  SideEffects [none]

  SeeAlso     [Tr_BddarraySort()]

*****************************************************************************/

static int
BddOrdCompare(
  const void *s1,
  const void *s2
)
{
  int a, b, r;
  PartitionInfo_t *pFS1 = *((PartitionInfo_t **) s1);
  PartitionInfo_t *pFS2 = *((PartitionInfo_t **) s2);

  r = 0;
  a = pFS1->TopRangePos;
  b = pFS2->TopRangePos;

  if (a > b)
    r = 1;
  else if (a == b)
    r = 0;
  else if (a < b)
    r = -1;

  return (r);
}


/**Function*******************************************************************

  Synopsis    [Compares BDDs for SIZE-SORTING method]

  Description [The function compares two BDDs and return:<BR>
    1, if size of 1st BDD is greater then 2nd<BR>
    -1, if size of 1st BDD is smaller then 2nd<BR>
    0, if both 1st and 2nd BDD have the same size.]

  SideEffects [none]

  SeeAlso     [Tr_BddarraySort()]

*****************************************************************************/

static int
BddSizeCompare(
  const void *s1,
  const void *s2
)
{
  int a, b, r;
  PartitionInfo_t *pFS1 = *((PartitionInfo_t **) s1);
  PartitionInfo_t *pFS2 = *((PartitionInfo_t **) s2);

  r = 0;
  a = pFS1->size;
  b = pFS2->size;

  if (a > b)
    r = 1;
  else if (a == b)
    r = 0;
  else if (a < b)
    r = -1;

  return (r);
}


/**Function*******************************************************************

  Synopsis    [Compares BDDs for WEIGTHED-SORT method]

  Description [The method of sorting is the heuristic method of Ranjan]

  SideEffects [none]

  SeeAlso     [Tr_BddarraySort()]

*****************************************************************************/

static int
WeightedSort(
  Tr_Mgr_t * trMgr /* Tr manager */ ,
  Ddi_Mgr_t * dd /* dd manager */ ,
  PartitionInfo_t ** FSa /* array of Partition Infos */ ,
  int nPart /* number of partitions */ ,
  Tr_Sort_e method              /* sorting method */
)
{
  Ddi_Varset_t *supp;
  PartitionInfo_t *FS, *FS1;
  int i, j, k, nvars, pos;
  int *var_cnt;                 /* variable occourrences */
  int maxLocalSmoothN, maxSmoothN, totSmoothN, maxBottomSmoothPos, maxRangeN;
  double benefit,               /* cost function */
   best;                        /* the best cost function */

  /* Read the weights for WEIGHTED-SORT method */

  double w1, w2, w3, w4, w5;

  if (trMgr != NULL) {
    w1 = Tr_MgrReadSortW(trMgr, 1);
    w2 = Tr_MgrReadSortW(trMgr, 2);
    w3 = Tr_MgrReadSortW(trMgr, 3);
    w4 = Tr_MgrReadSortW(trMgr, 4);
    w5 = Tr_MgrReadSortW(trMgr, 5);
  } else {
    w1 = TRSORT_DEFAULT_W1;
    w2 = TRSORT_DEFAULT_W2;
    w3 = TRSORT_DEFAULT_W3;
    w4 = TRSORT_DEFAULT_W4;
    w5 = TRSORT_DEFAULT_W5;
  }

  nvars = Ddi_MgrReadSize(dd);

  var_cnt = Pdtutil_Alloc(int,
    nvars
  );

  if (var_cnt == NULL) {
    fprintf(stderr, "Error: Out of memory.\n");
    return 0;
  }

  for (i = 0; i < nvars; i++)
    var_cnt[i] = 0;

  maxLocalSmoothN = maxSmoothN = maxRangeN = 0;
  totSmoothN = 0;

  maxBottomSmoothPos = -1;

  for (i = 0; i < nPart; i++) {
    FS = FSa[i];
    if (FS->SmoothN > maxSmoothN)
      maxSmoothN = FS->SmoothN;

    if (FS->BottomSmoothPos >= 0) {
      if ((maxBottomSmoothPos < 0)
        || (FS->BottomSmoothPos < maxBottomSmoothPos))
        maxBottomSmoothPos = FS->BottomSmoothPos;
    }
    if (FS->RangeN > maxRangeN)
      maxRangeN = FS->RangeN;

    supp = Ddi_VarsetDup(FS->SmoothV);
    /* upgrade var_cnt with variable occourrences in supp */
    while (!Ddi_VarsetIsVoid(supp)) {
      pos = Ddi_VarCurrPos(Ddi_VarsetTop(supp));
      if (var_cnt[pos] == 0)
        totSmoothN++;
      var_cnt[pos]++;

      Ddi_VarsetNextAcc(supp);

    }
    Ddi_Free(supp);
  }

  if (maxBottomSmoothPos < 0)
    maxBottomSmoothPos = 0;

  /*
   * Do sorting
   */
  for (i = 0; i < nPart; i++) {
    /* dynamically upgrade maxBottomSmoothPos with remaining partitions */
    Pdtutil_Assert(maxBottomSmoothPos < nvars, "array access out of bounds");
    while (maxBottomSmoothPos > 0 && var_cnt[maxBottomSmoothPos] == 0)
      maxBottomSmoothPos--;

    maxLocalSmoothN = 0;
    for (j = i; j < nPart; j++) {
      FS = FSa[j];

      FS->LocalSmoothN = 0;

      supp = Ddi_VarsetDup(FS->SmoothV);
      /* upgrade var_cnt with variable occourrences in supp */
      while (!Ddi_VarsetIsVoid(supp)) {
        pos = Ddi_VarCurrPos(Ddi_VarsetTop(supp));
        if (var_cnt[pos] == 1)  /* local variable */
          FS->LocalSmoothN++;

        Ddi_VarsetNextAcc(supp);

      }
      Ddi_Free(supp);

      if (FS->LocalSmoothN > maxLocalSmoothN)
        maxLocalSmoothN = FS->LocalSmoothN;
    }

    best = -1.0e20;             /* a safely small number */

    for (k = j = nPart - 1; j >= i; j--) {
      FS = FSa[j];

      if (trMgr != NULL) {
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMax_c,
          fprintf(stdout, "Sorting benefit.\n");
          fprintf(stdout, "Sort p[%d]: %f * %d / %d.\n", j,
            w1, FSa[j]->LocalSmoothN, maxLocalSmoothN);
          fprintf(stdout, "Sort p[%d]: %f * %d / %d.\n", j,
            w2, FSa[j]->SmoothN, maxSmoothN);
          fprintf(stdout, "Sort p[%d]: %f * %d / %d.\n", j,
            w3, FSa[j]->BottomSmoothPos, maxBottomSmoothPos);
          fprintf(stdout, "Sort p[%d]: %f * %d / %d.\n", j,
            -w4, FSa[j]->RangeN, maxRangeN);
          fprintf(stdout, "Sort p[%d]: %f * %d / %d.\n", j,
            w5, FSa[j]->AverageSmoothPos, maxBottomSmoothPos)
          );
      }

      benefit = 0.0;

      if (maxLocalSmoothN > 0)
        benefit += (w1 * FSa[j]->LocalSmoothN) / maxLocalSmoothN;

      if (maxSmoothN > 0) {
        benefit += (w2 * FSa[j]->SmoothN) / maxSmoothN;
      }

      if ((maxBottomSmoothPos > 0) && (FSa[j]->BottomSmoothPos > 0)) {
        benefit += (w3 * FSa[j]->BottomSmoothPos) / maxBottomSmoothPos;
      }

      if (maxRangeN > 0) {
        benefit -= (w4 * FSa[j]->RangeN) / maxRangeN;
      }

      if ((maxBottomSmoothPos > 0) && (FSa[j]->AverageSmoothPos > 0)) {
        benefit += (w5 * FSa[j]->AverageSmoothPos) / maxBottomSmoothPos;
      }

      if (trMgr != NULL) {
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMax_c,
          fprintf(stdout, "Sorting benefit.\n");
          fprintf(stdout, "Sort p[%d]: Benefit: %f\n.\n", j, benefit)
          );
      }

      if (benefit > best) {
        best = benefit;
        k = j;
      }
    }                           /* end-for (j) */

    /* swap k (function with best benefit) and i */

    FS1 = FSa[i];
    FS = FSa[k];
    FSa[i] = FS;
    FSa[k] = FS1;

    /* upgrade counters , x and z */

    supp = Ddi_VarsetDup(FS->SmoothV);
    /* upgrade var_cnt with variable occourrences in supp */
    while (!Ddi_VarsetIsVoid(supp)) {
      pos = Ddi_VarCurrPos(Ddi_VarsetTop(supp));
      var_cnt[pos]--;
      Pdtutil_Assert(var_cnt[pos] >= 0, "var count is < 0");

      Ddi_VarsetNextAcc(supp);
    }
    Ddi_Free(supp);

  }                             /* end-for (i) */

  free(var_cnt);
  return (1);
}


/**Function*******************************************************************

  Synopsis    [Compares BDDs for WEIGTHED-SORT method]

  Description [The method of sorting is the heuristic method of Ranjan]

  SideEffects [none]

  SeeAlso     [Tr_BddarraySort()]

*****************************************************************************/

static int
WeightedSortPdt(
  Tr_Mgr_t * trMgr /* Tr manager */ ,
  Ddi_Mgr_t * dd /* dd manager */ ,
  PartitionInfo_t ** FSa /* array of Partition Infos */ ,
  int nPart /* number of partitions */ ,
  Tr_Sort_e method              /* sorting method */
)
{
  Ddi_Varset_t *supp;
  PartitionInfo_t *FS, *FS1;
  int i, j, k, nvars, pos;
  int *var_cnt;                 /* variable occourrences */
  int maxLocalSmoothN, maxSmoothN, totSmoothN, maxBottomSmoothPos, maxRangeN;
  double benefit,               /* cost function */
   best;                        /* the best cost function */

  /* Read the weights for WEIGHTED-SORT method */
  double w1 = Tr_MgrReadSortW(trMgr, 1);
  double w2 = Tr_MgrReadSortW(trMgr, 2);
  double w3 = Tr_MgrReadSortW(trMgr, 3);
  double w4 = Tr_MgrReadSortW(trMgr, 4);
  double w5 = Tr_MgrReadSortW(trMgr, 5);

  nvars = Ddi_MgrReadSize(dd);

  var_cnt = Pdtutil_Alloc(int,
    nvars
  );

  if (var_cnt == NULL) {
    fprintf(stderr, "Error: Out of memory.\n");
    return 0;
  }

  for (i = 0; i < nvars; i++)
    var_cnt[i] = 0;

  maxLocalSmoothN = maxSmoothN = maxBottomSmoothPos = maxRangeN = 0;
  totSmoothN = 0;

  for (i = 0; i < nPart; i++) {
    FS = FSa[i];
    if (FS->SmoothN > maxSmoothN)
      maxSmoothN = FS->SmoothN;
    if (FS->BottomSmoothPos > maxBottomSmoothPos)
      maxBottomSmoothPos = FS->BottomSmoothPos;
    if (FS->RangeN > maxRangeN)
      maxRangeN = FS->RangeN;

    supp = Ddi_VarsetDup(FS->SmoothV);
    /* upgrade var_cnt with variable occourrences in supp */
    while (!Ddi_VarsetIsVoid(supp)) {
      pos = Ddi_VarCurrPos(Ddi_VarsetTop(supp));
      if (var_cnt[pos] == 0)
        totSmoothN++;
      var_cnt[pos]++;

      if (method == Tr_SortWeightPdt_c) {
        Ddi_VarsetNextAcc(supp);
      } else {
        supp = Ddi_VarsetNext(supp);
      }

    }
    Ddi_Free(supp);
  }

  /*
   * Do sorting
   */
  for (i = 0; i < nPart; i++) {
    /* dynamically upgrade maxBottomSmoothPos with remaining partitions */
    while (var_cnt[maxBottomSmoothPos] == 0)
      maxBottomSmoothPos--;

    maxLocalSmoothN = 0;
    for (j = i; j < nPart; j++) {
      FS = FSa[j];

      FS->LocalSmoothN = 0;

      supp = Ddi_VarsetDup(FS->SmoothV);
      /* upgrade var_cnt with variable occourrences in supp */
      while (!Ddi_VarsetIsVoid(supp)) {
        pos = Ddi_VarCurrPos(Ddi_VarsetTop(supp));
        if (var_cnt[pos] == 1)  /* local variable */
          FS->LocalSmoothN++;

        if (method == Tr_SortWeightPdt_c) {
          Ddi_VarsetNextAcc(supp);
        } else {
          supp = Ddi_VarsetNext(supp);
        }

      }
      Ddi_Free(supp);

      if (FS->LocalSmoothN > maxLocalSmoothN)
        maxLocalSmoothN = FS->LocalSmoothN;
    }

    best = 0.0;
    for (k = j = i; j < nPart; j++) {
      FS = FSa[j];

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMax_c,
        fprintf(stdout, "Sorting benefit.\n");
        fprintf(stdout, "Sort p[%d]: %f * %d / %d.\n", j,
          w1, FSa[j]->LocalSmoothN, maxLocalSmoothN);
        fprintf(stdout, "Sort p[%d]: %f * %d / %d.\n", j,
          w2, FSa[j]->SmoothN, maxSmoothN);
        fprintf(stdout, "Sort p[%d]: %f * %d / %d.\n", j,
          w3, FSa[j]->BottomSmoothPos, maxBottomSmoothPos);
        fprintf(stdout, "Sort p[%d]: %f * %d / %d.\n", j,
          -w4, FSa[j]->RangeN, maxRangeN);
        fprintf(stdout, "Sort p[%d]: %f * %d / %d.\n", j,
          w5, FSa[j]->SmoothN, totSmoothN)
        );

      benefit = 0, 0;

      if (maxLocalSmoothN > 0)
        benefit += (w1 * FSa[j]->LocalSmoothN) / maxLocalSmoothN;

      if (maxSmoothN > 0) {
        benefit += (w2 * FSa[j]->SmoothN) / maxSmoothN;
      }

      if (maxBottomSmoothPos > 0) {
        benefit += (w3 * FSa[j]->BottomSmoothPos) / maxBottomSmoothPos;
      }

      if (maxRangeN > 0) {
        benefit -= (w4 * FSa[j]->RangeN) / maxRangeN;
      }

      if (totSmoothN > 0) {
        benefit += (w5 * FSa[j]->SmoothN) / totSmoothN;
      }

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMax_c,
        fprintf(stdout, "sorting benefit.\n");
        fprintf(stdout, "Sort p[%d]: Benefit: %f\n.\n", j, benefit)
        );

      if (benefit > best) {
        best = benefit;
        k = j;
      }
    }                           /* end-for (j) */

    /* swap k (function with best benefit) and i */

    FS1 = FSa[i];
    FS = FSa[k];
    FSa[i] = FS;
    FSa[k] = FS1;

    /* upgrade counters , x and z */

    supp = Ddi_VarsetDup(FS->SmoothV);
    /* upgrade var_cnt with variable occourrences in supp */
    while (!Ddi_VarsetIsVoid(supp)) {
      pos = Ddi_VarCurrPos(Ddi_VarsetTop(supp));
      var_cnt[pos]--;
      Pdtutil_Assert(var_cnt[pos] >= 0, "var count is < 0");

      if (method == Tr_SortWeightPdt_c) {
        Ddi_VarsetNextAcc(supp);
      } else {
        supp = Ddi_VarsetNext(supp);
      }

    }
    Ddi_Free(supp);

  }                             /* end-for (i) */

  free(var_cnt);
  return (1);
}

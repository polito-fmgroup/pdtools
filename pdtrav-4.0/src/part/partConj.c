/**CFile**********************************************************************

  FileName    [partConj.c]

  PackageName [part]

  Synopsis    [Conjunctively partitioned functions]

  Description []

  SeeAlso     []

  Author    [Gianpiero Cabodi and Stefano Quer]

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

#include "partInt.h"
#include "ddiInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define OPERATION_CONST 0

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

int enableAbort = 0;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static Ddi_Bdd_t * PrintGroupsStats(Ddi_Bdd_t *fOld, Ddi_Varset_t *rvars, char *fileName);
static int getSmaller(int *size, int nop);
static Ddi_Bdd_t * MultiwayRecursiveAndExistStep(Ddi_Dd_t *FPart, Ddi_Varset_t *smoothV, Ddi_Bdd_t *constrain, float maxGrowth, Ddi_Bdd_t *overApprTerm, int threshold, int disjPartEna, int metaDecompEna);
static Ddi_Bdd_t * MultiwayRecursiveTreeAndExistStep(Ddi_Dd_t *FPart, Ddi_Varset_t *smoothV, Ddi_Varset_t *rvars, float maxGrowth, int threshold);
static Ddi_Bdd_t * PartConstrainOpt(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
static Ddi_Bdd_t * PartAndExistWithAbort(Ddi_Bdd_t *f, Ddi_Bdd_t *g, Ddi_Varset_t *smoothV, Ddi_Bdd_t *care, int thresh);
static Ddi_Bdd_t * WeakExist(Ddi_Bdd_t *f, Ddi_Varset_t *smoothV, float ratio);
#if OPERATION_CONST
static Ddi_Bdd_t * SizeOperation(Ddi_Bdd_t *fOld, Ddi_Varset_t *rvars);
#endif

/**AutomaticEnd***************************************************************/


/*----------------------------------------------------------------------------*/
/* Definition of internal function                                            */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Definition of External function                                            */
/*----------------------------------------------------------------------------*/

/**Function*******************************************************************

  Synopsis    [Compute the multiway and-exist over a conjunctively partitioned
    function]

  Description []

  SideEffects [none]

  SeeAlso     []

*****************************************************************************/

Ddi_Bdd_t *
Part_BddMultiwayLinearAndExist (
  Ddi_Bdd_t *FPart          /* Input partitioned function */,
  Ddi_Varset_t *smoothV     /* Var Set */,
  int threshold             /* Size threshold for result factor */
  )
{
  Ddi_Bdd_t *Res;
  Ddi_Bdd_t *F,                /* an element in the BDDs'array */
           *P,*Pnew;           /* product of BDDs */
  Ddi_Varset_t *smoothF, *smoothPF, *suppF, *suppP, *suppRes,
    *suppNext, *smooth;
  Ddi_Varset_t **EarlySm;   /* early smoothed variable sets */
  Ddi_Mgr_t *dd;
  int i, size, n;
  int extRef;

  /* dd Manager */
  dd = Ddi_ReadMgr(FPart);
  Pdtutil_Assert(dd==Ddi_ReadMgr(smoothV),
    "Bug: different DDI managers in multiway and-exist");

  /*Ddi_MgrPrintExtRef(dd);*/

  extRef = Ddi_MgrReadExtRef(dd);
  Ddi_MgrPeakProdLocalReset(dd);

  if (Ddi_BddIsMono(FPart)) {
    return (Ddi_BddExist (FPart, smoothV));
  }

  Ddi_MgrSiftSuspend(dd);

  n = Ddi_BddPartNum (FPart);
  Pdtutil_Assert (n>=1, "Bug: no partitions in multiway and-exist");

  /*
   *  Generate vector of early smoothable variable sets, analyzing
   *  Partitions in reverse order.
   */

  EarlySm = Pdtutil_Alloc (Ddi_Varset_t *, n);

  suppNext = Ddi_VarsetVoid (dd);

  /*----------------------------- Get Supports -----------------------------*/

  for (i=n-1; i>=0; i--) {

    /* take the support of i-th function */
    suppF = Ddi_BddSupp (Ddi_BddPartRead(FPart,i));

    Ddi_VarsetIntersectAcc(suppF,smoothV);

    /* Isolate variables not in the following functions for early smoothing */
    EarlySm[i] = Ddi_VarsetDiff(suppF,suppNext);

    /* Update support of following functions */

    Ddi_VarsetUnionAcc(suppNext,suppF);

    Ddi_Free (suppF);
  } /* End for i */

  Ddi_Free (suppNext);

  Res = Ddi_BddMakePartConjVoid(dd);
  P = Ddi_BddDup(Ddi_MgrReadOne(dd));
  suppP = Ddi_VarsetVoid(dd);
  suppRes = Ddi_VarsetVoid(dd);

  /*-------------------------------- Main Cycle -----------------------------*/

  for (i=0; i<n; i++) {

    F = Ddi_BddDup (Ddi_BddPartRead (FPart, i));
    while ((Ddi_BddIsPartConj(F)||Ddi_BddIsPartDisj(F))&&
         (Ddi_BddPartNum(F)==1)) {
      Ddi_Bdd_t *t = F;
      F = Ddi_BddPartExtract(t,0);
      Ddi_Free(t);
    }
    if (Ddi_BddIsPartDisj(F)) {
       /*Ddi_BddSetMono(F);*/
       Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
         fprintf(stdout, "@");
       );
       if (Ddi_BddIsPartDisj(P)) {
         Ddi_BddSetMono(P);
       }
    }

    Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "{%d", Ddi_BddSize(F))
    );

    /* compute smooth varsets */
    smooth = Ddi_VarsetDiff (EarlySm[i], suppRes);

    /*Ddi_VarsetPrint(smooth, 0, NULL, stdout);*/

    smoothF = Ddi_VarsetDiff (smooth, suppP);
    smoothPF = Ddi_VarsetIntersect(smooth, suppP);
    Ddi_Free (smooth);
    Ddi_Free (EarlySm[i]);

    /* Early smoothing on F */

    if (!(Ddi_VarsetIsVoid (smoothF))&&
      (Ddi_BddIsMono(F)||Ddi_BddIsMeta(F))) {
      Ddi_MgrSiftResume(dd);
      Ddi_BddExistAcc (F, smoothF);
      Ddi_MgrSiftSuspend(dd);

      size = Ddi_BddSize (F);
      Ddi_MgrPeakProdUpdate(dd,size);

      Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "<E%d>%d}", Ddi_VarsetNum(smoothF), size)
      );

    } else {
      Ddi_VarsetUnionAcc(smoothPF,smoothF);
      Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "}")
      );
    }
    /* ... END Early smoothing on F */

    Ddi_Free (smoothF);

    size = Ddi_BddSize (F);
    Ddi_MgrPeakProdUpdate(dd,size);

    if ((size>threshold)&&(threshold>=0)) {
      Ddi_Free (smoothPF);
      Ddi_Free (EarlySm[i]);
      Pnew = Ddi_BddDup(P);
    }
    else {

#if 0
    if (!(Ddi_VarsetIsVoid (smoothPF))) {
#else
    if (1) {
#endif
      /* Early smoothing on P and F */
      Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
          fprintf(stdout, "<E%d>", Ddi_VarsetNum(smoothPF))
      );

      /*
      Ddi_ProfileSetCurrentPart (F, i-1);
      Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "##%d-%d##", i, Ddi_ProfileReadCurrentPart (F))
      );
      */
      if (Ddi_BddIsMeta(P)) {
        if (threshold < 0) {
          Pnew = P;
      }
      else {
          Pnew = Ddi_BddDup(P);
      }
        Ddi_MgrSiftResume(dd);
        DdiMetaAndExistAcc(Pnew,F,smoothPF,NULL);
        Ddi_MgrSiftSuspend(dd);
        size = Ddi_BddSize (Pnew);
      }
      else {
        Ddi_MgrSiftResume(dd);
        if (enableAbort && (threshold>=0)) {
          Ddi_MgrAbortOnSiftEnableWithThresh(dd,threshold,1);
        }
        if (threshold < 0) {
          Pnew = P;
          Ddi_BddAndExistAcc (Pnew, F, smoothPF);
      }
      else {
          Pnew = Ddi_BddAndExist (P, F, smoothPF);
      }
        if (dd->abortedOp) {
          size = threshold+1;
        }
        else {
          size = Ddi_BddSize (Pnew);
        }
        if (enableAbort && (threshold>=0)) {
          Ddi_MgrAbortOnSiftDisable(dd);
        }
        Ddi_MgrSiftSuspend(dd);
        if (Ddi_BddIsPartDisj(Pnew)) {
          Ddi_BddSetClustered(Pnew,threshold);
      }
      }
#if 0
      if (Ddi_MetaActive(dd)/*&&(Ddi_BddSize(P)>800000)*/) {
        Ddi_Bdd_t *PM = Ddi_BddMakeMeta(P);
        Ddi_MgrSiftResume(dd);
        DdiMetaAndExistAcc(PM,F,smoothPF);
        Ddi_MgrSiftSuspend(dd);
        Ddi_BddFromMeta(PM);
        Pdtutil_Assert(Ddi_BddEqual(PM,Pnew),
          "Wrong result of META-AND-EXIST");
        Ddi_Free(PM);
      }
#endif
    } else {
      /* No early smoothing on P and F */
      /*Ddi_ProfileSetCurrentPart (F, i-1);*/
      Ddi_MgrSiftResume(dd);
      if (enableAbort && (threshold>=0)) {
        Ddi_MgrAbortOnSiftEnableWithThresh(dd,threshold,1);
      }
      if (threshold < 0) {
        Pnew = P;
        Ddi_BddAndAcc (Pnew, F);
      }
      else {
        Pnew = Ddi_BddAnd (P, F);
      }
      if (dd->abortedOp) {
        size = threshold+1;
      }
      else {
        size = Ddi_BddSize (Pnew);
      }
      if (enableAbort && (threshold>=0)) {
        Ddi_MgrAbortOnSiftDisable(dd);
      }
      Ddi_MgrSiftSuspend(dd);
    }

    Ddi_Free (smoothPF);
    }

    if (threshold < 0) {
      Ddi_MgrPeakProdUpdate(dd,size);

      Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
          fprintf(stdout, "=%d*", size)
      );
    }
    else if (size>threshold) {

      Ddi_VarsetUnionAcc(suppRes,suppP);

      if (!Ddi_BddIsOne(P)) {
        Ddi_BddPartInsertLast(Res,P);
      }

      Ddi_Free (Pnew);
      Ddi_Free (P);
      P = Ddi_BddDup(F);

      Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
          fprintf(stdout, "(+%d)-", size-threshold)
      );

    } else {

      Ddi_Free (P);
      P = Pnew;

      Ddi_MgrPeakProdUpdate(dd,size);
      Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
          fprintf(stdout, "=%d*", size)
      );

    }

    Ddi_Free(suppP);
    suppP = Ddi_BddSupp(P);

    Ddi_Free (F);

  } /* end-for(i) */

  Pdtutil_Free (EarlySm);

  if (!Ddi_BddIsOne(P)) {
    Ddi_BddPartInsertLast(Res,P);
  }
  Ddi_Free (P);
  Ddi_Free (suppP);
  Ddi_Free (suppRes);

#if 0
  if ((!Ddi_BddIsMono(Res))&&(Ddi_BddPartNum(Res)==1)) {
    Ddi_BddSetMono (Res);
  }
#endif

  /*Ddi_MgrPrintExtRef(dd);*/
  Ddi_MgrCheckExtRef(dd,extRef+1);

  Ddi_MgrSiftResume(dd);

  if (Ddi_BddPartNum(Res)==0) {
    Ddi_BddPartInsertLast(Res,Ddi_MgrReadOne(dd));
  }
  return (Res);
}

/**Function*******************************************************************

  Synopsis    [Compute the multiway and-exist over a conjunctively partitioned
    function]

  Description []

  SideEffects [none]

  SeeAlso     []

*****************************************************************************/

Ddi_Bdd_t *
Part_BddMultiwayRecursiveAndExist (
  Ddi_Bdd_t *FPart           /* Input partitioned function */,
  Ddi_Varset_t *smoothV     /* Var Set */,
  Ddi_Bdd_t *constrain      /* factor to be AND-ed with FPart */,
  int threshold             /* Size threshold for result factor */
)
{
   return (Part_BddMultiwayRecursiveAndExistOver(
     FPart,smoothV,constrain,NULL,threshold));
}

/**Function*******************************************************************

  Synopsis    [Compute the multiway and-exist over a conjunctively partitioned
    function]

  Description []

  SideEffects [none]

  SeeAlso     []

*****************************************************************************/

Ddi_Bdd_t *
Part_BddMultiwayRecursiveAndExistOver (
  Ddi_Bdd_t *FPart           /* Input partitioned function */,
  Ddi_Varset_t *smoothV     /* Var Set */,
  Ddi_Bdd_t *constrain      /* factor to be AND-ed with FPart */,
  Ddi_Bdd_t *overApprTerm   /* factor to be replicated with FPart
                               when clustering */,
  int threshold             /* Size threshold for result factor */
)
{
  Ddi_Bdd_t *r, *part, *constrInt;
  int i;

  Pdtutil_Assert(Ddi_BddIsPartConj(FPart),"Conj. part required");
  part = Ddi_BddDup(FPart);
#if 0
  if (!Ddi_BddIsOne(constrain)) {
    Ddi_BddPartInsert(part,0,constrain);
  }
  for (i=0; i<Ddi_BddPartNum(part); i++) {
    Ddi_Varset_t *supp;
    Ddi_BddSuppAttach(Ddi_BddPartRead(part,i));
    supp = Ddi_BddSupp(Ddi_BddPartRead(part,i));
    Ddi_Free(supp);
  }
#endif
  for (i=0; i<Ddi_BddPartNum(part); i++) {
    Ddi_Varset_t *supp;
    Ddi_BddSuppAttach(Ddi_BddPartRead(part,i));
    supp = Ddi_BddSupp(Ddi_BddPartRead(part,i));
    Ddi_Free(supp);
  }
  if (constrain != NULL) {
    constrInt = Ddi_BddDup(constrain);
    Ddi_BddSetPartConj(constrInt);
  }
  else
    constrInt = NULL;
  r = MultiwayRecursiveAndExistStep(part,smoothV,constrInt,
        1.5,overApprTerm,threshold,1,1);

  Ddi_BddSuppDetach(r);

#if 0
  Ddi_BddSetMono(r);
#else
#if 1
  Ddi_BddSetClustered(r,5000);
#endif
#endif

  Ddi_Free(part);
  Ddi_Free(constrInt);
  return(r);
}

/**Function*******************************************************************

  Synopsis    [Compute the multiway and-exist over a conjunctively partitioned
    function]

  Description []

  SideEffects [none]

  SeeAlso     []

*****************************************************************************/

Ddi_Bdd_t *
Part_BddMultiwayRecursiveTreeAndExist (
  Ddi_Bdd_t *FPart           /* Input partitioned function */,
  Ddi_Varset_t *smoothV     /* Var Set */,
  Ddi_Varset_t *rvars,
  int threshold             /* Size threshold for result factor */
)
{
  Ddi_Bdd_t *r;

  r = MultiwayRecursiveTreeAndExistStep(FPart,smoothV,rvars,1.5,threshold);

  return(r);
}


/**Function*******************************************************************

  Synopsis    []

  Description []

  SideEffects [none]

  SeeAlso     []

*****************************************************************************/

Ddi_Bdd_t *
Part_BddDisjSuppPart (
  Ddi_Bdd_t *f              /* a BDD */,
  Ddi_Bdd_t *TR             /* a Clustered Transition Relation */,
  Ddi_Vararray_t *psv      /* array of present state variables */,
  Ddi_Vararray_t *nsv      /* array of next state variables */,
  int verbosity            /* level of verbosity */
)
{
  int i, n;
  Ddi_Bdd_t *f2, *fh, *fl;
  Ddi_Varset_t *suppf, *suppl, *supph;

  Pdtutil_Assert (Ddi_BddIsPartConj(TR),
    "Conjuntion of BDDs expected");

  suppf = Ddi_BddSupp(f);
  suppl = Ddi_VarsetDup(suppf);

  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "%d support vars found\nHow many keep ? ",
      Ddi_VarsetNum(suppf))
  );
  scanf ("%d",&n);
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "-> %d.\n", n)
  );
  for(i=0;i<n;i++) {
    suppl = Ddi_VarsetEvalFree(Ddi_VarsetRemove(suppl,
             Ddi_VarsetTop(suppl)),suppl);
  }
  supph = Ddi_VarsetDiff(suppf,suppl);

  fh = Ddi_BddExist(f,suppl);
  fl = Ddi_BddExist(f,supph);
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "FH: [%d] - FL: [%d].\n", Ddi_BddSize(fh),Ddi_BddSize(fl))
  );
  f2 = Ddi_BddConstrain(f,fh);
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "F|FH: [%d].\n", Ddi_BddSize(f2))
  );
  Ddi_BddConstrainAcc(f2,fl);
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "F|FL: [%d].\n", Ddi_BddSize(f2))
  );
  Ddi_Free (f2);

  f2 = Ddi_BddNot(f);

  fh = Ddi_BddExist(f2,suppl);
  fl = Ddi_BddExist(f2,supph);
  Ddi_Free (f2);
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "~FH: [%d] - ~FH: [%d].\n", Ddi_BddSize(fh), Ddi_BddSize(fl))
  );
  f2 = Ddi_BddConstrain(f,fh);
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "F|~FH: [%d].\n", Ddi_BddSize(f2))
  );
  Ddi_Free (f2);
  f2 = Ddi_BddConstrain(f,fl);
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "F|~FL: [%d].\n", Ddi_BddSize(f2))
  );
  Ddi_Free (f2);

  Ddi_Free (f);

#if 0
  f2 = Ddi_BddDup(f);

  for (i=0;i<Ddi_BddPartNum(TR);i++) {
    t = Ddi_BddSwapVars(Ddi_BddPartRead(TR,i),nsv,psv);
    supp = Ddi_BddSupp(t);
    smooth = Ddi_VarsetDiff(suppf,supp);
    Ddi_Free (t);
    part = Ddi_BddExist(f2,smooth);
    Ddi_BddConstrainAcc(f2,part);
    Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "Partition %d: (%d) - [%d].\n", i,
        Ddi_BddSize(part),Ddi_BddSize(f2))
    );
    Ddi_Free (part);
    Ddi_Free (supp);
    Ddi_Free (smooth);
  }

  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "Trying Disj on ~F.\n")
  );

  f2 = Ddi_BddNot(f);

  for (i=0;i<Ddi_BddPartNum(TR);i++) {
    t = Ddi_BddSwapVars(Ddi_BddPartRead(TR,i),nsv,psv);
    supp = Ddi_BddSupp(t);
    smooth = Ddi_VarsetDiff(suppf,supp);
    Ddi_Free (t);
    part = Ddi_BddExist(f2,smooth);
    Ddi_BddConstrainAcc(f2,part);

    Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "Partition %d: (%d) - [%d].\n", i,
        Ddi_BddSize(part),Ddi_BddSize(f2))
    );

    Ddi_Free (part);
    Ddi_Free (supp);
    Ddi_Free (smooth);
  }
#endif

  Ddi_Free (suppf);
  return (NULL);
}

/**Function********************************************************************

  Synopsis    [Partition in connected components]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Part_PartDomainCC (
  Ddi_Bdd_t      *f        /* Input partitioned/clustered relation */,
  Ddi_Vararray_t *piv,
  Ddi_Vararray_t *psv,
  Ddi_Vararray_t *nsv,
  int           clustth,
  int           maxcommon,
  int           minshare
)
{
  Ddi_Varset_t *rvars, *supp, *common, *suppCC;
  Ddi_Bdd_t *r, *fi, *CC, *fdup;
  int again, i, j, n0, n1, nv, id, shareOK, maxj;
  int *sharedvars;
  float *partratios, aspectRatio;
  Ddi_Mgr_t *dd = Ddi_ReadMgr(f);
  rvars = Ddi_VarsetMakeFromArray(nsv);

  nv = Ddi_MgrReadNumCuddVars(dd);
  sharedvars = Pdtutil_Alloc (int, nv);
  for (i=0; i<nv; i++) {
    sharedvars[i] = 0;
  }

  fdup = Ddi_BddDup(f);

  for (i=0; i<Ddi_BddPartNum(fdup); i++) {
    fi = Ddi_BddPartRead(fdup,i);
    supp = Ddi_BddSupp(fi);
    for (Ddi_VarsetWalkStart(supp);
       !Ddi_VarsetWalkEnd(supp);
       Ddi_VarsetWalkStep(supp)) {
      id = Ddi_VarCuddIndex(Ddi_VarsetWalkCurr(supp));
      sharedvars[id]++;
    }
    Ddi_Free(supp);
  }

  again = 0;

#if 1
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Finding connected components.\n")
  );
#endif

  j = n0 = n1 = 0;
  r = Ddi_BddMakePartConjVoid(dd);

  while (Ddi_BddPartNum(fdup)>0) {

    CC = Ddi_BddPartExtract(fdup,0);
    Ddi_BddSetPartConj(CC);
    suppCC = Ddi_BddSupp(CC);
    Ddi_VarsetDiffAcc(suppCC,rvars);

    do {
      again=0;
      for (i=Ddi_BddPartNum(fdup)-1; i>=0; i--) {
        fi = Ddi_BddPartRead(fdup,i);
        supp = Ddi_BddSupp(fi);
        Ddi_VarsetDiffAcc(supp,rvars);

        common = Ddi_VarsetIntersect(supp, suppCC);

        shareOK=0;

        /* put together components with shared supp vars > maxcommon
           or <=maxcommon and at least one highly shared var ??? */
        if ((Ddi_VarsetNum(common)>0)&&(Ddi_VarsetNum(common)<=maxcommon)) {

          for (Ddi_VarsetWalkStart(common);
             !Ddi_VarsetWalkEnd(common);
             Ddi_VarsetWalkStep(common)) {
            id = Ddi_VarCuddIndex(Ddi_VarsetWalkCurr(common));
            if (sharedvars[id] > minshare)
              shareOK = 1;
          }

      }

        if ((Ddi_VarsetNum(common)>maxcommon) || shareOK) {
          Ddi_VarsetUnionAcc(suppCC,supp);
          fi = Ddi_BddPartExtract(fdup,i);
          Ddi_BddPartInsertLast(CC,fi);
          Ddi_Free(fi);
          again=1;
        }

        Ddi_Free(supp);
        Ddi_Free(common);
      }

    } while (again != 0);

#if 0
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "[%3d] ", j++)
    );
    Ddi_VarSetPrint (suppCC, 1000, NULL, stdout);
#endif
    if (Ddi_VarsetIsVoid(suppCC)) {
      n0++;
    }
    else if (Ddi_VarsetNum(suppCC) == 1) {
      n1++;
    }

    Ddi_BddPartInsertLast(r,CC);
    Ddi_Free(CC);
    Ddi_Free(suppCC);

  }

  Ddi_Free(fdup);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout,
      ">> %3d    Connected components found.\n",Ddi_BddPartNum(r));
    fprintf(stdout, ">> %3d 0  bit Connected components found.\n",n0);
    fprintf(stdout, ">> %3d 1  bit Connected components found.\n",n1);
    fprintf(stdout, ">> %3d >1 bit Connected components found.\n",
      Ddi_BddPartNum(r)-(n0+n1));
  );

  partratios = Pdtutil_Alloc (float, Ddi_BddPartNum(r));
  for (j=0; j<Ddi_BddPartNum(r); j++) {
    CC = Ddi_BddPartRead(r,j);
    supp = Ddi_BddSupp(CC);
    aspectRatio = (float)Ddi_VarsetNum(supp);
    Ddi_VarsetDiffAcc(supp,rvars);
    aspectRatio = ((float)Ddi_VarsetNum(supp))
      / (aspectRatio - Ddi_VarsetNum(supp));
    partratios[j] = aspectRatio;
    Ddi_Free(supp);
  }
  for (i=0; i<Ddi_BddPartNum(r)-1; i++) {
    maxj = i;
    for (j=i+1; j<Ddi_BddPartNum(r); j++) {
      if (partratios[j] > partratios[maxj])
        maxj = j;
    }
    if (maxj != i) {
      fi = Ddi_BddPartExtract(r,maxj);
      Ddi_BddPartInsert(r,maxj,Ddi_BddPartRead(r,i));
      Ddi_BddPartWrite(r,i,fi);
      Ddi_Free(fi);
      aspectRatio = partratios[maxj];
      partratios[maxj] = partratios[i];
      partratios[i] = aspectRatio;
    }
  }
  Pdtutil_Free(partratios);

  for (j=0; j<Ddi_BddPartNum(r); j++) {

    CC = Ddi_BddPartExtract(r,j);

    if (Ddi_BddIsPartConj(CC)) {

      Ddi_Varset_t *active;
      Ddi_Bdd_t *sortedCC;

      active = Ddi_BddSupp(CC);

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "component %d",j)
      );

      Ddi_VarsetDiffAcc(active, rvars);

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, " - size: %d.\n",Ddi_VarsetNum(active))
      );
      /*Ddi_VarsetPrint(active, 0, NULL, stdout);*/

      sortedCC = Ddi_BddMakePartConjVoid(dd);

      /* MLP: move singleton rows up */

      while (Ddi_BddPartNum(CC)>0) {
        int max, maxId = (-1), min;

        for (i=0; i<Ddi_BddPartNum(CC);) {

          fi = Ddi_BddPartRead(CC,i);
          supp = Ddi_BddSupp(fi);
          Ddi_VarsetIntersectAcc(supp,active);
          if (Ddi_VarsetNum(supp)<=1) {
            fi = Ddi_BddPartExtract(CC,i);
            Ddi_BddPartInsertLast(sortedCC,fi);
            Ddi_Free(fi);
            Ddi_VarsetDiffAcc(active, supp);
          }
          else {
            i++;
        }
          Ddi_Free(supp);

        }

        min = Ddi_VarsetNum(active);

        for (i=0; i<Ddi_BddPartNum(CC);i++) {
          fi = Ddi_BddPartRead(CC,i);
          supp = Ddi_BddSupp(fi);
          Ddi_VarsetIntersectAcc(supp,active);
          if (Ddi_VarsetNum(supp)<min) {
            min = Ddi_VarsetNum(supp);
        }
          Ddi_Free(supp);
        }
        for (i=0; i<nv; i++) {
          sharedvars[i] = 0;
        }
        max = 0;
        for (i=0; i<Ddi_BddPartNum(CC);i++) {
          fi = Ddi_BddPartRead(CC,i);
          supp = Ddi_BddSupp(fi);
          Ddi_VarsetIntersectAcc(supp,active);
          if (Ddi_VarsetNum(supp)==min) {
            for (Ddi_VarsetWalkStart(supp); !Ddi_VarsetWalkEnd(supp);
              Ddi_VarsetWalkStep(supp)) {
              id = Ddi_VarCuddIndex(Ddi_VarsetWalkCurr(supp));
              sharedvars[id]++;
              if (sharedvars[id]>max) {
                max = sharedvars[id];
                maxId = id;
            }
            }
        }
          Ddi_Free(supp);
        }
        if (max > 0) {
          Ddi_VarsetRemoveAcc(active,Ddi_VarFromIndex(dd,maxId));
      }
      }

      Ddi_Free(active);
      Ddi_Free(CC);

      Ddi_BddPartInsert(r,j,sortedCC);

      active = Ddi_BddSupp(sortedCC);
      Ddi_VarsetDiffAcc(active, rvars);

      for (i=0; i<Ddi_BddPartNum(sortedCC);i++) {
        fi = Ddi_BddPartRead(sortedCC,i);
        supp = Ddi_BddSupp(fi);
        for (Ddi_VarsetWalkStart(active); !Ddi_VarsetWalkEnd(active);
          Ddi_VarsetWalkStep(active)) {
#if 0
          if (Ddi_VarInVarset(supp,Ddi_VarsetWalkCurr(active))) {
            Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
              Pdtutil_VerbLevelNone_c,
              fprintf(stdout, "*")
            );
        } else {
            Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
              Pdtutil_VerbLevelNone_c,
              fprintf(stdout, ".")
            );
        }
#endif
        }
        Ddi_Free(supp);
      }
      Ddi_Free(active);

      Ddi_Free(sortedCC);

    }

  }

  Pdtutil_Free(sharedvars);
  Ddi_Free(rvars);
  return(r);

}

/**Function********************************************************************

  Synopsis    [Partition in connected components]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Part_PartManualGroups (
  Ddi_Bdd_t      *f,
  char           *fileName
)
{
  FILE *fp=NULL;
  char buf[PDTUTIL_MAX_STR_LEN];
  int flagFile, i, *var2part, nv, id;
  Ddi_Bdd_t *newf, *grp, *fi;
  Ddi_Varset_t *supp;
  Ddi_Mgr_t *dd = Ddi_ReadMgr(f);
  Ddi_Var_t *v;

  fp = Pdtutil_FileOpen (fp, fileName, "r", &flagFile);
  if (fp==NULL) {
    Pdtutil_Warning (1, "Error opening file");
    return (NULL);
  }

  nv = Ddi_MgrReadNumCuddVars(dd);

  var2part = Pdtutil_Alloc (int, nv);
  for (i=0; i<nv; i++) {
    var2part[i] = -1;
  }

  for (i=Ddi_BddPartNum(f)-1; i>=0; i--) {
    fi = Ddi_BddPartRead(f,i);
    supp = Ddi_BddSupp(fi);
    for (Ddi_VarsetWalkStart(supp);
       !Ddi_VarsetWalkEnd(supp);
       Ddi_VarsetWalkStep(supp)) {
      id = Ddi_VarCuddIndex(Ddi_VarsetWalkCurr(supp));
      var2part[id]=i;
    }
    Ddi_Free(supp);
  }

  newf = Ddi_BddMakePartConjVoid(dd);
  grp = NULL;

  while (fscanf(fp,"%s",buf)==1) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "%s.\n", buf)
    );
    if (strcmp(buf,".group")==0) {
      if (grp != NULL) {
        Ddi_BddPartInsertLast(newf,grp);
        Ddi_Free(grp);
      }
      grp = Ddi_BddMakePartConjVoid(dd);
    }
    else {
      v = Ddi_VarFromName(dd,buf);
      if (v == NULL) {
        Pdtutil_Warning(1,"Variable not found");
        fprintf(stderr," %s.\n", buf);
      }
      id = Ddi_VarCuddIndex(v);
      i = var2part[id];
      if (i<0) {
        Pdtutil_Warning(1,"Variable not found in any partition");
        fprintf(stderr," %s.\n", buf);
      }
      else {
        fi = Ddi_BddPartRead(f,i);
        Ddi_BddPartInsertLast(grp,fi);
      }
    }
  }
  if (grp != NULL) {
    Ddi_BddPartInsertLast(newf,grp);
    Ddi_Free(grp);
  }

  Pdtutil_Free(var2part);
  Pdtutil_FileClose (fp, &flagFile);

  return(newf);
}

/**Function********************************************************************

  Synopsis    [Partition in connected components]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void
Part_PartPrintGroups (
  Ddi_Bdd_t      *f,
  Ddi_Varset_t   *rvars,
  char           *fileName
)
{
  FILE *fp=NULL;
  int flagFile, i;
  Ddi_Bdd_t *fi;
  Ddi_Varset_t *supp;
  Ddi_Var_t *v;

  fp = Pdtutil_FileOpen (fp, fileName, "w", &flagFile);
  if (fp==NULL) {
    Pdtutil_Warning (1, "Error opening file");
    return;
  }

  for (i=0; i<Ddi_BddPartNum(f); i++) {
    fprintf(fp, ".group.\n");
    fi = Ddi_BddPartRead(f,i);
    supp = Ddi_BddSupp(fi);
    for (Ddi_VarsetWalkStart(supp);
       !Ddi_VarsetWalkEnd(supp);
       Ddi_VarsetWalkStep(supp)) {
      v = Ddi_VarsetWalkCurr(supp);
      if (Ddi_VarInVarset(rvars,v)) {
        fprintf(fp, "%s.\n",Ddi_VarName(v));
      }
    }
    Ddi_Free(supp);
  }

  Pdtutil_FileClose (fp, &flagFile);

  return;
}

/**Function********************************************************************

  Synopsis    [Partition in connected components]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Part_PartPrintGroupsStats (
  Ddi_Bdd_t      *f,
  Ddi_Varset_t   *rvars,
  char           *fileName
  )
{
  Ddi_Bdd_t *fNew1;

#if 1
  fNew1 = PrintGroupsStats (f, rvars, fileName);
  /*
  fNew2 = SizeOperation (fNew1, rvars);
  */
  return (fNew1);
#else
  fNew1 = SizeOperation (f, rvars);
  return (fNew1);
#endif
}

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Partition in connected components]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

static Ddi_Bdd_t *
PrintGroupsStats (
  Ddi_Bdd_t      *fOld,
  Ddi_Varset_t   *rvars,
  char           *fileName
  )
{
  const float THRESHOLD = 10000.0;
  FILE *fp=NULL;
  int flagFile, i, j;
  float n;
  Ddi_Bdd_t *f, *fi, *fj, *fipj;
  Ddi_Varset_t *supp;
  Ddi_Var_t *v;

  f = Ddi_BddDup (fOld);
  /*Ddi_MgrSiftDisable(dd);*/

  fp = Pdtutil_FileOpen (fp, fileName, "w", &flagFile);
  if (fp==NULL) {
    Pdtutil_Warning (1, "Error opening file");
    return (NULL);
  }

#if 0
  for (i=0; i<Ddi_BddPartNum(f); i++) {
    fi = Ddi_BddMakeMono (Ddi_BddPartRead (f,i));

    if (Ddi_BddSize(fi) > PART_THRESHOLD) {
      Ddi_Bdd_t *fPart, *fTmp;

      fPart = Part_PartitionSetCudd (fi,
        /*Part_MethodAppCon_c*/
        /*Part_MethodIteCon_c*/
        /*Part_MethodGenCon_c*/
        Part_MethodVarCon_c
        , PART_THRESHOLD, Pdtutil_VerbLevelDevMax_c );

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "PartNum = %d.\n", Ddi_BddPartNum (fPart))
      );

      if (Ddi_BddPartNum(fPart) > 1) {
         fTmp = Ddi_BddPartExtract(f, i);
         Ddi_Free (fTmp);
         for (j=0; j<Ddi_BddPartNum(fPart); j++) {
           fTmp = Ddi_BddPartRead (fPart, j);
#if 1
           Ddi_BddPartInsert (f, i+j, fTmp);
#else
           if (j%2 == 0) {
             Ddi_BddPartInsert (f, i, fTmp);
           } else {
             Ddi_BddPartInsertLast (f, fTmp);
         }
#endif
       }
      }
      Ddi_Free (fPart);
    }
  }
#endif

#if 1
  /*fp = stdout;*/
  fprintf(fp, "FileName = %s.\n", fileName);
  fprintf(fp, "THRESHOLD (unity for measure) = %f.\n", THRESHOLD);
  for (i=0; i<Ddi_BddPartNum(f); i++) {
    fi = Ddi_BddMakeMono (Ddi_BddPartRead (f,i));
    supp = Ddi_BddSupp(fi);
    fprintf(fp, "%2dsize=%5dsupp=%2d", i+1, Ddi_BddSize(fi),
       Ddi_VarsetNum(supp));

    for (Ddi_VarsetWalkStart(supp);
       !Ddi_VarsetWalkEnd(supp);
       Ddi_VarsetWalkStep(supp)) {
      v = Ddi_VarsetWalkCurr(supp);
      if (Ddi_VarInVarset(rvars,v)) {
        fprintf(fp, " %s",Ddi_VarName(v));
      }
    }
    Ddi_Free(supp);

    fprintf(fp, "");
    for (j=0; j<=i; j++) {
      fprintf(fp, "#");
    }

    for (j=i+1; j<Ddi_BddPartNum(f); j++) {
      fj = Ddi_BddMakeMono (Ddi_BddPartRead (f,j));
      fipj = Ddi_BddAnd (fi, fj);
      n = ((float) Ddi_BddSize(fipj))/THRESHOLD;
      if (n<=15) {
        fprintf(fp, "%1x", (int) n);
      } else {
        if (n>16) {
        fprintf(fp, ">");
        }
      }
      fflush (fp);
      Ddi_Free (fipj);
      Ddi_Free (fj);
    }

    Ddi_Free (fi);
    fprintf(fp, "");
    fflush (fp);
  }

  Pdtutil_FileClose (fp, &flagFile);

  /*Ddi_MgrSiftEnable (dd, 100000);*/
#endif

  return (f);
}

#if OPERATION_CONST

/**Function********************************************************************

  Synopsis    [Partition in connected components]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

static Ddi_Bdd_t *
SizeOperation (
  Ddi_Bdd_t      *fOld,
  Ddi_Varset_t   *rvars
  )
{
  int i, j1, j2, n, *size;
  Ddi_Bdd_t *f, *fi, *res = NULL, **opBddArray;
  Ddi_Mgr_t *dd;

  /*Ddi_MgrSiftDisable(dd);*/
  /*Ddi_MgrSiftEnable (dd, 50000);*/
  /*Ddi_MgrAbortOnSiftDisable(dd);*/

  f = Ddi_BddDup (fOld);
  dd = Ddi_ReadMgr(f);

  n = Ddi_BddPartNum(f);
  size = Pdtutil_Alloc (int, n);
  opBddArray = Pdtutil_Alloc (Ddi_Bdd_t *, n);

  for (i=0; i<n; i++) {
    fi = Ddi_BddMakeMono (Ddi_BddPartRead (f,i));
    opBddArray[i] = Ddi_BddDup (fi);
    size[i] = Ddi_BddSize (fi);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "(%d)%d ", i, size[i])
    );
    Ddi_Free (fi);
  }
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "\n")
  );

  /* Free Original TR .. to save space */
  Ddi_Free (f);

  for (i=0; i<n; i++) {
    j1 = getSmaller (size, n);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "[%d/%d](%d,", i+1, n-1, size[j1])
    );
    size[j1] = (-1);
    j2 = getSmaller (size, n);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "%d=", size[j2])
    );

    res = Ddi_BddAnd (opBddArray[j1], opBddArray[j2]);

    size[j2] = Ddi_BddSize (res);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "%d), ", size[j2])
    );

    Ddi_Free (opBddArray[j1]);
    Ddi_Free (opBddArray[j2]);
    opBddArray[j1] = NULL;
    opBddArray[j2] = res;
  }

  Ddi_MgrOrdWrite (dd, "neword.ord", NULL, Pdtutil_VariableOrderDefault_c);
  /*Ddi_MgrSiftEnable (dd, 100000);*/

  return (res);
}
#endif

/**Function********************************************************************
  Synopsis           []
  Description        []
  SideEffects        []
  SeeAlso            []
******************************************************************************/

static int
getSmaller (
  int *size,
  int nop
  )
{
  int i, min;

  min = -1;
  for (i=0; i<nop; i++) {
    if (size[i] == (-1)) {
      continue;
    } else {
      if (min == (-1)) {
        min = i;
      } else {
        if (size[i] < size[min]) {
          min = i;
        }
      }
    }
  }

  Pdtutil_Assert (min!=(-1), "Wrong min.");

  return (min);
}

/**Function*******************************************************************

  Synopsis    [Recursively compute multiway and-exist over a conjunctively
    partitioned function]

  Description []

  SideEffects [none]

  SeeAlso     []

*****************************************************************************/

static Ddi_Bdd_t *
MultiwayRecursiveAndExistStep (
  Ddi_Dd_t *FPart           /* Input partitioned function */,
  Ddi_Varset_t *smoothV     /* Var Set */,
  Ddi_Bdd_t *constrain,
  float maxGrowth,
  Ddi_Bdd_t *overApprTerm   /* factor to be replicated with FPart
                               when clustering */,
  int threshold             /* Size threshold for result factor */,
  int disjPartEna,
  int metaDecompEna         /* enable meta BDD decomposition */
)
{
  const int PART_THRESHOLD = 3500000;
  Ddi_Bdd_t *Res;
  Ddi_Bdd_t *F, *P, *Pnew, *Prod, *clust, *over;
  Ddi_Varset_t *recSmooth, *smoothF, *smoothPF, *smoothP,
    *suppF, *suppP, *suppProd;
  Ddi_Mgr_t *dd;
  int i, n;
  int extRef;

  dd = Ddi_ReadMgr(FPart);    /* dd manager */

  if (Ddi_BddPartNum(FPart)<=2) {
    Ddi_Bdd_t *p2;
    if (constrain != NULL) {
      if (Ddi_BddPartNum(FPart)==2) {
        F = Ddi_BddPartExtract (FPart, 1);
        F = Ddi_BddEvalFree(PartConstrainOpt(F,constrain),F);
        Ddi_BddPartInsert(FPart,1,F);
        Ddi_Free(F);
      }
      P = Ddi_BddPartExtract (FPart, 0);
      P = Ddi_BddEvalFree(PartConstrainOpt(P,constrain),P);
      Ddi_BddPartInsert(FPart,0,P);
      Ddi_Free(P);
    }
    if (Ddi_BddPartNum(FPart)==2) {
      p2 = Ddi_BddPartRead(FPart,1);
    }
    else p2 = Ddi_MgrReadOne(dd);
    n = Ddi_VarsetNum(smoothV);
    if (n>0)
      Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
          fprintf(stdout, "L:<%d>", n)
      );

    Pnew = PartAndExistWithAbort (Ddi_BddPartRead(FPart,0),
      p2,smoothV,constrain,threshold);
    if (Pnew == NULL) {
      Pnew = Ddi_BddDup(FPart);
    }
    Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "|=%d|", Ddi_BddSize(Pnew))
    );
    if (constrain != NULL) {
      Pnew = Ddi_BddEvalFree(PartConstrainOpt(Pnew,constrain),Pnew);
    }

  if (overApprTerm != NULL) {
    Ddi_Varset_t *supp = Ddi_BddSupp(Pnew);
    Ddi_VarsetIntersectAcc(supp,smoothV);
    if (!Ddi_VarsetIsVoid(supp)) {
      Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "NOT VOID 0 (%d).\n", Ddi_VarsetNum(supp))
      );
      Ddi_BddExistAcc(Pnew,supp);
    }
    Ddi_Free(supp);
  }

    return(Pnew);
  }
  /* now FPart has at least 3 partitions */

  n = Ddi_BddPartNum(FPart);
  Pdtutil_Assert (n>=1, "Bug: no partitions in multiway and-exist");

  extRef = Ddi_MgrReadExtRef(dd);

  Prod = FPart;

  P = Ddi_BddPartExtract (Prod, 0);

#if 0
  Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "{Proj:%d->", Ddi_BddSize(P))
  );
  P = Ddi_BddEvalFree(Ddi_BddProject(P,Prod,smoothV),P);
  Pdtutil_VerbosityLocal(dd, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "%d}", Ddi_BddSize(P))
  );
#endif

  F = Ddi_BddPartExtract (Prod, 0);

  /* compute smooth varsets */

  suppP = Ddi_BddSupp(P);
  suppF = Ddi_BddSupp(F);
  suppProd = Ddi_BddSupp(Prod);

  smoothPF = Ddi_VarsetUnion(suppF,suppP);
  Ddi_VarsetIntersectAcc(smoothPF,smoothV);

  Ddi_VarsetDiffAcc(smoothPF,suppProd);

  /* Early smoothing on P */
  smoothP = Ddi_VarsetDiff(smoothPF, suppF);

  if (Ddi_BddIsPartConj(P)) {
#if 0
     /* attenzione a constrain - non so se funziona ! */
    P = Ddi_BddEvalFree(MultiwayRecursiveAndExistStep(P,smoothP,
    constrain,maxGrowth,overApprTerm,threshold,disjPartEna,metaDecompEna),P);
#endif
  }
  else if (!(Ddi_VarsetIsVoid (smoothP))) {
#if 0
    Ddi_BddExistAcc(P,smoothP);
    Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "{P:<%d>%d}", Ddi_VarsetNum(smoothP),Ddi_BddSize(P))
    );
#endif
  }
  Ddi_Free(smoothP);

  /* Early smoothing on F */
  smoothF = Ddi_VarsetDiff(smoothPF, suppP);
  if (Ddi_BddIsPartConj(F)) {
    if (threshold>0) {
      F = Ddi_BddEvalFree(MultiwayRecursiveAndExistStep(F,smoothF,
      constrain,maxGrowth,overApprTerm,threshold,disjPartEna,metaDecompEna),F);
    }
  }
  else if (!(Ddi_VarsetIsVoid (smoothF))) {
    Ddi_BddExistAcc(F,smoothF);
    Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "{F:<%d>%d}", Ddi_VarsetNum(smoothF),Ddi_BddSize(F))
    );
  }
  Ddi_Free(smoothF);

  n = Ddi_VarsetNum(smoothPF);
  if (n>0)
    Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "PF:|%d|", n)
    );

  if (constrain != NULL) {
    P = Ddi_BddEvalFree(PartConstrainOpt(P,constrain),P);
    F = Ddi_BddEvalFree(PartConstrainOpt(F,constrain),F);
  }

#if 0
  Pnew = PartAndExistWithAbort (P, F, smoothPF, constrain,
    (int)(maxGrowth*(Ddi_BddSize(P)+Ddi_BddSize(F))));
#else
  if (Ddi_BddIsOne(F)) {
    Pnew = Ddi_BddDup(P);
    Ddi_Free(smoothPF);
    smoothPF = Ddi_VarsetVoid(dd);
  }
  else {
    Pnew = PartAndExistWithAbort (P, F, smoothPF, constrain, threshold);
  }
#endif

#if 1
  if (overApprTerm == NULL) {
    recSmooth = Ddi_VarsetDiff(smoothV,smoothPF);
  }
  else {
    recSmooth = Ddi_VarsetDup(smoothV);
  }
#else
  recSmooth = Ddi_VarsetDup(smoothV);
#endif
  Ddi_Free (smoothPF);

  clust = NULL;
  if (Pnew == NULL) {
    if (overApprTerm == NULL) {
       /* no approximation done. Handle recsmooth */
      Ddi_Varset_t *suppP = Ddi_BddSupp(P);
      Ddi_VarsetDiffAcc(recSmooth,suppP);
      Ddi_Free(suppP);
      clust = Ddi_BddDup(P);
      Pnew = Ddi_BddDup(F);
    }
    else {
      Ddi_BddPartInsert(Prod,0,F);
      Pnew = WeakExist(P,recSmooth,0.7);
#if 1
      Ddi_BddSetPartConj(Pnew);
      Ddi_BddPartInsertLast(Pnew,overApprTerm);
#endif
    }

    Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "@")
    );
  }

  Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "|=%d|", Ddi_BddSize(Pnew))
  );

  Ddi_Free (suppP);
  Ddi_Free (suppF);
  Ddi_Free (suppProd);
  Ddi_Free (F);
  Ddi_Free (P);

  Pnew = Ddi_BddEvalFree(PartConstrainOpt(Pnew,constrain),Pnew);
#if 1
  if ((metaDecompEna)&&(constrain != NULL)&&(Ddi_BddSize(Pnew)>100)) {
    Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity(dd);

    Ddi_MgrSetOption(dd,Pdt_DdiVerbosity_c,inum,Pdtutil_VerbLevelUsrMin_c);
    over = Ddi_BddExistOverApprox(Pnew,smoothV);
    Ddi_MgrSetOption(dd,Pdt_DdiVerbosity_c,inum,verbosity);

    over = Ddi_BddEvalFree(PartConstrainOpt(over,constrain),over);
    Pnew = Ddi_BddEvalFree(PartConstrainOpt(Pnew,over),Pnew);
#if 0
    Ddi_BddAndAcc(Ddi_BddPartRead(constrain,Ddi_BddPartNum(constrain)-1),over);
#else
    {
      int l;

      Ddi_Bdd_t *pp = Ddi_BddPartExtract(constrain,
      Ddi_BddPartNum(constrain)-1);
      Ddi_BddSetPartConj(pp);
      Ddi_BddPartInsertLast(pp,over);

      Ddi_MgrSetOption(dd,Pdt_DdiVerbosity_c,inum,Pdtutil_VerbLevelUsrMin_c);
      Ddi_BddSetClustered(pp,5000);
      Ddi_MgrSetOption(dd,Pdt_DdiVerbosity_c,inum,verbosity);
      for (l=0; l<Ddi_BddPartNum(pp); l++) {
        Ddi_BddPartInsertLast(constrain,Ddi_BddPartRead(pp,l));
      }
      Ddi_Free(pp);
    }
#endif
    /*Ddi_BddSetClustered(constrain,5000);*/
#if 0
    Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "[=%d,%d]", Ddi_BddSize(tmp),Ddi_BddSize(Pnew))
    );
#endif
  }
  else {
    over = NULL;
  }
#endif

  if ((disjPartEna>0)&&(Ddi_BddSize(Pnew) > PART_THRESHOLD)) {
    Ddi_Bdd_t *fPart, *fi, *ri;

    disjPartEna--;

    fPart = Part_PartitionSetCudd (Pnew,
      /*Part_MethodAppCon_c*/
      /*Part_MethodIteCon_c*/
      /*Part_MethodGenCon_c*/
      Part_MethodVarDis_c
      , Ddi_BddSize(Pnew)/3, Pdtutil_VerbLevelDevMax_c );

    Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "PartNum = %d.\n", Ddi_BddPartNum (fPart))
    );
    Ddi_Free(Pnew);
    Pnew = fPart;

    Res = Ddi_BddMakeConst(dd,0);
    for (i=Ddi_BddPartNum(Pnew)-1; i>=0; i--) {
      fi = Ddi_BddPartExtract(Pnew,i);
      Ddi_BddPartInsert(Prod,0,fi);
      Ddi_Free(fi);
      ri = MultiwayRecursiveAndExistStep(Prod,recSmooth,
        constrain,maxGrowth,overApprTerm,threshold,disjPartEna,metaDecompEna);
      Ddi_BddOrAcc(Res,ri);
      Ddi_Free(ri);
      fi = Ddi_BddPartExtract(Prod,0);
      Ddi_Free(fi);
    }
    Ddi_Free(Pnew);
  }
  else {
    if (!Ddi_BddIsOne(Pnew)) {
      Ddi_BddPartInsert(Prod,0,Pnew);
    }
    Ddi_Free(Pnew);
    Res = MultiwayRecursiveAndExistStep(Prod,recSmooth,
      constrain,maxGrowth,overApprTerm,threshold,disjPartEna,metaDecompEna);
  }

  Ddi_Free(recSmooth);

  if (clust!=NULL) {
    Ddi_BddSetPartConj(Res);
    Ddi_BddPartInsert(Res,0,clust);
    Ddi_Free(clust);
  }

  if (over != NULL) {
    Ddi_BddSetPartConj(Res);
#if 0
    Ddi_BddPartInsert(Res,0,over);
#else
    Ddi_BddPartInsertLast(Res,over);
#endif
    Ddi_Free(over);
  }

  Ddi_MgrCheckExtRef(dd,extRef+1);

  if (0)
  {
    Ddi_Varset_t *supp = Ddi_BddSupp(Res);
    Ddi_VarsetIntersectAcc(supp,smoothV);
    if (!Ddi_VarsetIsVoid(supp)) {
      Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "NOT VOID (%d).\n", Ddi_VarsetNum(supp))
      );
    }
    Ddi_Free(supp);
  }

  return (Res);
}


/**Function*******************************************************************

  Synopsis    [Recursively compute multiway and-exist over a conjunctively
    partitioned function]

  Description []

  SideEffects [none]

  SeeAlso     []

*****************************************************************************/

static Ddi_Bdd_t *
MultiwayRecursiveTreeAndExistStep (
  Ddi_Dd_t *FPart           /* Input partitioned function */,
  Ddi_Varset_t *smoothV     /* Var Set */,
  Ddi_Varset_t *rvars,
  float maxGrowth,
  int threshold             /* Size threshold for result factor */
)
{
  Ddi_Bdd_t *Res;
  Ddi_Bdd_t *F, *P, *tmp, *Pnew, *Prod;
  Ddi_Varset_t *recSmooth, *smoothF, *smoothPF, *smoothP,
    *suppF, *suppP, *suppProd, *supp0, *supp1, *common, *tot;
  Ddi_Mgr_t *dd;
  int i, n, besti, doAndExist;
  Pdtutil_VerbLevel_e verbosity;
  int extRef;
  float affinity, bestAffinity = (-1.0);

  dd = Ddi_ReadMgr(FPart);    /* dd manager */
  verbosity = Ddi_MgrReadVerbosity(dd);

  if (Ddi_BddPartNum(FPart)==0) {
    return(Ddi_BddMakeConst(dd,1));
  }

  if (Ddi_BddPartNum(FPart)==1) {
    Ddi_Bdd_t *p2;
    if (Ddi_BddPartNum(FPart)==2) {
      p2 = Ddi_BddPartRead(FPart,1);
    }
    else p2 = Ddi_MgrReadOne(dd);
    n = Ddi_VarsetNum(smoothV);
    if (n>0)
      Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "L:<%d>", n)
      );

    if (Ddi_BddIsMono(Ddi_BddPartRead(FPart,0))) {
      Pnew = PartAndExistWithAbort (Ddi_BddPartRead(FPart,0),
        p2,smoothV,NULL,threshold);
    } else {
      Pnew = MultiwayRecursiveTreeAndExistStep(Ddi_BddPartRead(FPart,0),
        smoothV,rvars,maxGrowth,threshold);
    }
    if (Pnew == NULL) {
      Pnew = Ddi_BddDup(FPart);
    }
    Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "|=%d|", Ddi_BddSize(Pnew))
    );
    return(Pnew);
  }
  /* now FPart has at least 2 partitions */

  n = Ddi_BddPartNum(FPart);
  Pdtutil_Assert (n>=1, "Bug: no partitions in multiway and-exist");

  extRef = Ddi_MgrReadExtRef(dd);

  Prod = Ddi_BddDup(FPart);

  i = 0;
  if (threshold<0)
    i=1;
  besti = -1;
  for (; i<Ddi_BddPartNum(Prod)-1; i++) {
    if (threshold>=0)
      supp0 = Ddi_BddSupp(Ddi_BddPartRead(Prod,i));
    else
      supp0 = Ddi_BddSupp(Ddi_BddPartRead(Prod,0));
    Ddi_VarsetDiffAcc(supp0,rvars);
    /*Ddi_VarsetPrint(supp0, 0, NULL, stdout);*/
    supp1 = Ddi_BddSupp(Ddi_BddPartRead(Prod,i+1));
    Ddi_VarsetDiffAcc(supp1,rvars);
    common = Ddi_VarsetIntersect(supp0,supp1);
    /*Ddi_VarsetPrint(common, 0, NULL, stdout);*/
    tot = Ddi_VarsetUnion(supp0,supp1);
    affinity = ((float)Ddi_VarsetNum(common))
      / (Ddi_VarsetNum(supp0)+Ddi_VarsetNum(supp1));
    if ((besti<0)||(affinity>bestAffinity)) {
      besti = i;
      bestAffinity = affinity;
    }
    Ddi_Free(supp0);
    Ddi_Free(supp1);
    Ddi_Free(common);
    Ddi_Free(tot);
  }
  Pdtutil_Assert(besti>=0,"Wrong MAX affinity computation");

  if (threshold>=0)
    P = Ddi_BddPartExtract (Prod, besti);
  else {
    P = Ddi_BddPartExtract (Prod, 0);
    Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "<<%d>>",besti)
    );
  }
  F = Ddi_BddPartExtract (Prod, besti);

  if (threshold<0)
    besti = 0;

  /* compute smooth varsets */

  suppP = Ddi_BddSupp(P);
  suppF = Ddi_BddSupp(F);
  if (Ddi_BddPartNum(Prod)==0)
    suppProd = Ddi_VarsetVoid(dd);
  else
    suppProd = Ddi_BddSupp(Prod);

  smoothPF = Ddi_VarsetUnion(suppF,suppP);
  Ddi_VarsetIntersectAcc(smoothPF,smoothV);
  Ddi_VarsetDiffAcc(smoothPF,suppProd);

  /* Early smoothing on P */
  smoothP = Ddi_VarsetDiff(smoothPF, suppF);

  doAndExist = 1;
  if (Ddi_BddIsPartConj(P)) {
    doAndExist = 0;
    P = Ddi_BddEvalFree(MultiwayRecursiveTreeAndExistStep(P,smoothP,
    rvars,maxGrowth,threshold),P);
  }
  else if (!(Ddi_VarsetIsVoid (smoothP))) {
#if 0
    Ddi_BddExistAcc(P,smoothP);
    Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "{P:<%d>%d}", Ddi_VarsetNum(smoothP),Ddi_BddSize(P))
    );
#endif
  }
  Ddi_Free(smoothP);

  /* Early smoothing on F */
  smoothF = Ddi_VarsetDiff(smoothPF, suppP);
  if (Ddi_BddIsPartConj(F)) {
    doAndExist = 0;
    if (threshold>0) {
      F = Ddi_BddEvalFree(MultiwayRecursiveTreeAndExistStep(F,smoothF,
      rvars,maxGrowth,threshold),F);
    }
  }
  else if (!(Ddi_VarsetIsVoid (smoothF))) {
    Ddi_BddExistAcc(F,smoothF);
    Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "{F:<%d>%d}", Ddi_VarsetNum(smoothF), Ddi_BddSize(F))
    );
  }
  Ddi_Free(smoothF);

  n = Ddi_VarsetNum(smoothPF);
  if (n>0)
    Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "PF:<%d>", n)
    );

  if (doAndExist) {
    Pnew = PartAndExistWithAbort (P, F, smoothPF, NULL, threshold);
  }
  else
    Pnew = NULL;

  Ddi_Free (smoothPF);

  Ddi_Free (suppP);
  Ddi_Free (suppF);
  Ddi_Free (suppProd);

  if (Pnew == NULL) {
    Ddi_Bdd_t *Prod1, *Res1;
    Ddi_Varset_t *suppP;

    /* no clustering possible. Split ! */

    Prod1 = F;
    Ddi_BddSetPartConj(Prod1);
    while (besti<Ddi_BddPartNum(Prod)) {
      tmp = Ddi_BddPartExtract (Prod, besti);
      Ddi_BddPartInsertLast(Prod1,tmp);
      Ddi_Free(tmp);
    }

    Ddi_BddPartInsertLast(Prod,P);
    Ddi_Free(P);

    recSmooth = Ddi_VarsetDup(smoothV);

    suppP = Ddi_BddSupp(Prod1);
    Ddi_VarsetDiffAcc(recSmooth,suppP);
    Ddi_Free(suppP);

    Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "@")
    );

    Res = MultiwayRecursiveTreeAndExistStep(Prod,recSmooth,
      rvars,maxGrowth,threshold);

    Ddi_Free(recSmooth);

    recSmooth = Ddi_VarsetDup(smoothV);

    suppP = Ddi_BddSupp(Prod);
    Ddi_VarsetDiffAcc(recSmooth,suppP);
    Ddi_Free(suppP);

    Ddi_Free(Prod);

    Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "@")
    );

    Res1 = MultiwayRecursiveTreeAndExistStep(Prod1,recSmooth,
      rvars,maxGrowth,threshold);

    Ddi_BddSetPartConj(Res);
    Ddi_BddSetPartConj(Res1);

    Ddi_Free(Prod1);
    Ddi_Free(recSmooth);

    for (i=0; i<Ddi_BddPartNum(Res1); i++) {
      tmp = Ddi_BddPartRead (Res1, i);
      Ddi_BddPartInsertLast(Res,tmp);
    }
    Ddi_Free(Res1);

  }
  else {
    Ddi_Free (F);
    Ddi_Free (P);
    if (!Ddi_BddIsOne(Pnew)) {
      Ddi_BddPartInsert(Prod,besti,Pnew);

      Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "|=%d|", Ddi_BddSize(Pnew))
      );
    }
    Ddi_Free(Pnew);
    Res = MultiwayRecursiveTreeAndExistStep(Prod,smoothV,
      rvars,maxGrowth,threshold);
    Ddi_Free(Prod);
  }

  Ddi_MgrCheckExtRef(dd,extRef+1);

  return (Res);
}

/**Function********************************************************************
  Synopsis    [Optimized constrain cofactor.]
  Description [return constrain only if size reduced]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
PartConstrainOpt (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  Ddi_Bdd_t *r, *p, *pc;
  int i, size0, size1;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity(ddm);

  Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,Pdtutil_VerbLevelUsrMin_c);
  size0 = Ddi_BddSize(f);

  if (Ddi_BddIsPartConj(f)) {

    r = Ddi_BddDup(f);
    for (i=0; i<Ddi_BddPartNum(f); i++) {
      p = Ddi_BddPartRead(r,i);
      pc = PartConstrainOpt(p,g);
      Ddi_BddPartWrite(r,i,pc);
      Ddi_Free(pc);
    }
  }
  else if (Ddi_BddIsPartConj(g)) {
    r = Ddi_BddDup(f);
    for (i=0; i<Ddi_BddPartNum(g); i++) {
      p = Ddi_BddPartRead(g,i);
      r = Ddi_BddEvalFree(PartConstrainOpt(r,p),r);
    }
  }
  else {
    Ddi_MgrAbortOnSiftEnableWithThresh(ddm,2*Ddi_BddSize(f),1);

    r = Ddi_BddRestrict(f,g);
    if (Ddi_BddSize(r)>Ddi_BddSize(f)) {
      Ddi_Free(r);
      r = Ddi_BddDup(f);
    }

    Ddi_MgrAbortOnSiftDisable(ddm);
  }

  Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,verbosity);

  Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelDevMax_c,
    size1 = Ddi_BddSize(r);
    if (size1 < size0) {
      fprintf(stdout, "((CO:%d->%d))",size0,size1);
    }
  );

  Ddi_BddSuppDetach(r);
  return (r);
}

/**Function********************************************************************
  Synopsis    [Optimized constrain cofactor.]
  Description [return constrain only if size reduced]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
PartAndExistWithAbort (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g,
  Ddi_Varset_t *smoothV,
  Ddi_Bdd_t  *care,
  int thresh
)
{
  Ddi_Bdd_t *r;
  int i, abortThresh;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);

  if (thresh<0) {
    if (Ddi_BddIsPartConj(g)||Ddi_BddIsPartConj(f)) {
      Ddi_Bdd_t *pp, *ov, *gg;
      Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity(ddm);

      ov = Ddi_BddDup(f);
      if (Ddi_BddIsPartDisj(ov)) {
       /*Ddi_BddSetMono(ov);*/
        if (care!=NULL)
          ov = Ddi_BddEvalFree(PartConstrainOpt(ov,care),ov);
      }
      Ddi_BddSetPartConj(ov);
      gg = Ddi_BddDup(g);
      if (Ddi_BddIsPartDisj(gg)) {
       /*Ddi_BddSetMono(gg);*/ /*@@@@*/
        if (care!=NULL)
          gg = Ddi_BddEvalFree(PartConstrainOpt(gg,care),gg);
      }
      Ddi_BddSetPartConj(gg);
      pp = Ddi_BddMakePartConjVoid(ddm);
      for (i=0; i<Ddi_BddPartNum(gg); i++) {
      Ddi_BddPartInsertLast(ov,Ddi_BddPartRead(gg,i));
      }
      Ddi_Free(gg);
      for (i=0; i<Ddi_BddPartNum(ov); ) {
        Ddi_Bdd_t *p = Ddi_BddPartRead(ov,i);
        Ddi_Varset_t *supp = Ddi_BddSupp(p);
        Ddi_VarsetIntersectAcc(supp,smoothV);
        if (!Ddi_VarsetIsVoid(supp)) {
          p = Ddi_BddPartExtract(ov,i);
            Ddi_BddPartInsertLast(pp,p);
          Ddi_Free(p);
      }
        else {
        i++;
      }
        Ddi_Free(supp);
      }

      if (Ddi_BddPartNum(pp)>0) {
      Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,Pdtutil_VerbLevelUsrMin_c);
#if 0
        Ddi_BddExistAcc(pp,smoothV);
#else
#if 1
        pp = Ddi_BddEvalFree(
          Part_BddMultiwayRecursiveAndExist(pp,smoothV,
          care,-1),pp);
#else
        pp = Ddi_BddEvalFree(
          MultiwayRecursiveAndExistStep(pp,smoothV,
          care,1.5,NULL,-1,1,1),pp);
#endif
#endif
        Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,verbosity);
        Ddi_BddPartInsert(ov,0,pp);
      }
      Ddi_Free(pp);
      return(ov);
    }
    else {
      return(Ddi_BddAndExist(f,g,smoothV));
    }
  }
#if 0
  if (thresh<300000)
    thresh = 300000;
#endif

  if (Ddi_BddIsMono(f)&&Ddi_BddIsMono(g)) {
    Ddi_MgrAbortOnSiftEnableWithThresh(ddm,2*thresh,1);

    r = Ddi_BddAndExist(f,g,smoothV);

    if ((ddm->abortedOp)||(Ddi_BddSize(r)>thresh)) {
      Ddi_Free(r);
      r = NULL;
    }

    Ddi_MgrAbortOnSiftDisable(ddm);
  }
  else {
    abortThresh = ddm->settings.part.existClustThresh;
    ddm->settings.part.existClustThresh = thresh;

    r = Ddi_BddAndExist(f,g,smoothV);

    ddm->settings.part.existClustThresh = abortThresh;
  }

  return (r);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
WeakExist(
  Ddi_Bdd_t  *f,
  Ddi_Varset_t *smoothV,
  float ratio
)
{
  Ddi_Bdd_t *r, *tmp;
  Ddi_Varset_t *supp = Ddi_BddSupp(f), *sm;
  int initSize = Ddi_BddSize(f);

  Ddi_VarsetIntersectAcc(supp,smoothV);

  r = Ddi_BddDup(f);
  for (Ddi_VarsetWalkStart(supp);
    !Ddi_VarsetWalkEnd(supp);
    Ddi_VarsetWalkStep(supp)) {
      sm = Ddi_VarsetMakeFromVar(Ddi_VarsetWalkCurr(supp));
      tmp = Ddi_BddExist(r,sm);
      Ddi_Free(sm);
      if (Ddi_BddIsOne(tmp)) {
        Ddi_Free(tmp);
        continue;
      }
      else {
        Ddi_Free(r);
        r = tmp;
      }
      if (Ddi_BddSize(r) < ratio*initSize)
        break;
  }

  Ddi_Free(supp);

  return(r);
}

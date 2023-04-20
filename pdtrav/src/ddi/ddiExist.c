 /**CFile**********************************************************************

  FileName    [ddiExist.c]

  PackageName [ddi]

  Synopsis    [multiway and exist procedure]

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

#include "ddiInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define WEAK_EXIST 0

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

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static Ddi_Bdd_t * MultiwayRecursiveAndExistStep(Ddi_Dd_t *FPart, Ddi_Varset_t *smoothV, Ddi_Bdd_t *constrain, Ddi_Bdd_t *dpartConstr, float maxGrowth, Ddi_Bdd_t *overApprTerm, int threshold, int disjPartEna, int metaDecompEna, int constrainOptFail);
static int ConstrainOptAcc(Ddi_Bdd_t *f, Ddi_Bdd_t *g, int logVerbose);
static int ConstrainOptAccIntern(Ddi_Bdd_t *f, Ddi_Bdd_t *g, int logVerbose);
static Ddi_Bdd_t * AndExistWithAbort(Ddi_Bdd_t *f, Ddi_Bdd_t *g, Ddi_Varset_t *smoothf, Ddi_Varset_t *smoothV, Ddi_Bdd_t *care, int thresh, Ddi_Bdd_t *orTerm, int aboThresh, int metaDecompEna);
//static Ddi_Bdd_t * WeakExist(Ddi_Bdd_t *f, Ddi_Varset_t *smoothV, float ratio);
static void TestPart(Ddi_Bdd_t *f);

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of internal function                                           */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of External function                                           */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Close and abstr special purpose handling in DDI manager]
  Description [Close and abstr special purpose handling in DDI manager]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_AndAbsQuit (
  Ddi_Mgr_t *ddm
)
{
  /* ddi exist info */

  Ddi_Free(ddm->exist.clipWindow);
  Ddi_Free(ddm->exist.activeWindow);
  Ddi_Free(ddm->exist.conflictProduct);
  if (ddm->exist.infoPtr!=NULL) {
    Cuplus_bddAndAbstractInfoFree(ddm->exist.infoPtr);
  }


}

/**Function********************************************************************
  Synopsis    [Close and abstr special purpose handling in DDI manager]
  Description [Close and abstr special purpose handling in DDI manager]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_AndAbsFreeClipWindow (
  Ddi_Mgr_t *ddm
)
{

  Ddi_Free(ddm->exist.clipWindow);
}

/**Function*******************************************************************

  Synopsis    [Multiway and-exist over a conjunctively partitioned function]
  Description [Multiway and-exist over a conjunctively partitioned function.
               Recursive approach]
  SideEffects [none]
  SeeAlso     []

*****************************************************************************/

Ddi_Bdd_t *
Ddi_BddMultiwayRecursiveAndExist (
  Ddi_Bdd_t *FPart          /* Input partitioned function */,
  Ddi_Varset_t *smoothV     /* Var Set */,
  Ddi_Bdd_t *constrain      /* factor to be AND-ed with FPart */,
  int threshold             /* Size threshold for result factor */
)
{
   return (Ddi_BddMultiwayRecursiveAndExistOver(
     FPart,smoothV,constrain,NULL,threshold));
}

/**Function*******************************************************************

  Synopsis    []
  Description []
  SideEffects [none]
  SeeAlso     []

*****************************************************************************/

Ddi_Bdd_t *
Ddi_BddMultiwayRecursiveAndExistOver (
  Ddi_Bdd_t *FPart          /* Input partitioned function */,
  Ddi_Varset_t *smoothV     /* Var Set */,
  Ddi_Bdd_t *constrain      /* factor to be AND-ed with FPart */,
  Ddi_Bdd_t *overApprTerm   /* factor to be replicated with FPart
                               when clustering */,
  int threshold             /* Size threshold for result factor */
  )
{
  Ddi_Bdd_t *r, *part, *constrInt;
  int i, freeClipWindow, useAig=0;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(FPart);    /* dd manager */
  /* duplicated so that it may be modified within recursions */
  Ddi_Varset_t *mySmoothV = Ddi_VarsetDup(smoothV);

  /* Ddi_BddSetPartConj(FPart); */
  Pdtutil_Assert(Ddi_BddIsPartConj(FPart),"Conj. part required");

  r = Ddi_BddMakeConst(ddiMgr,0);

  useAig = Ddi_BddPartNum(FPart)>0&&Ddi_BddIsAig(Ddi_BddPartRead(FPart,0));

  freeClipWindow = (ddiMgr->exist.clipWindow == NULL);

  do {
    Ddi_Bdd_t *rTot;

  part = Ddi_BddDup(FPart);

#if 0
  if (!Ddi_BddIsOne(constrain)) {
    Ddi_BddPartInsert(part,0,constrain);
  }
#endif
  for (i=0; i<Ddi_BddPartNum(part); i++) {
    Ddi_BddSuppAttach(Ddi_BddPartRead(part,i));
  }
  Ddi_BddNotAcc(r);
  if (constrain != NULL) {
    constrInt = Ddi_BddDup(constrain);
    Ddi_BddSetPartConj(constrInt);
    if (!Ddi_BddIsOne(r)) {
      Ddi_BddPartInsertLast(constrInt,r);
    }
    if (useAig) {
      Ddi_BddSetAig(constrInt);
    }
  }
  else {
    if (Ddi_BddIsOne(r)) {
      constrInt = NULL;
    }
    else {
      constrInt = Ddi_BddDup(r);
      Ddi_BddSetPartConj(constrInt);
    }
  }
  Ddi_BddNotAcc(r);

  if (ddiMgr->exist.clipLevel != 2) {
    ddiMgr->exist.clipDone = 0;
  }
  ddiMgr->exist.infoPtr->initElseFirst = 1;
  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "\n")
  );
  if (ddiMgr->exist.clipLevel > 0) {
  if (ddiMgr->exist.clipWindow != NULL) {
#if 0
    int j,k,n;
    Ddi_Bdd_t *p, *t = part;
    Ddi_Varset_t *supp1, *supp2;
    supp1 = Ddi_BddSupp(ddiMgr->exist.clipWindow);
    for (j=0,k=0; j<Ddi_BddPartNum(t); j++) {
      p = Ddi_BddPartRead(t,j);
      supp2 = Ddi_BddSupp(p);
      n = Ddi_VarsetNum(supp2);
      Ddi_VarsetIntersectAcc(supp2,supp1);
      if (Ddi_VarsetNum(supp2) > n/3) {
        p = Ddi_BddPartExtract(t,j);
        Ddi_BddPartInsert(t,k++,p);
        Ddi_Free(p);
      }
      Ddi_Free(supp2);
    }
    Ddi_Free(supp1);
#endif
    if (ddiMgr->exist.clipLevel == 1 && 
        !Ddi_BddIsOne(ddiMgr->exist.clipWindow)) {
      Ddi_BddNotAcc(ddiMgr->exist.clipWindow);
      Ddi_BddPartInsert(part,0,ddiMgr->exist.clipWindow);
      Ddi_Free(ddiMgr->exist.clipWindow);
      ddiMgr->exist.clipWindow = Ddi_BddMakeConst(ddiMgr,1);
    }
  }
  else {
    ddiMgr->exist.clipWindow = Ddi_BddMakeConst(ddiMgr,1);
  }
  }

  ddiMgr->exist.ddiExistRecurLevel = -1;

  rTot = MultiwayRecursiveAndExistStep(part,mySmoothV,constrInt,NULL,
        1.5,overApprTerm,threshold,ddiMgr->settings.part.existDisjPartTh,
        (ddiMgr->settings.part.existOptLevel>2)/* meta decomp */,0);

#if 0
    if (Ddi_BddIsZero(rTot) && (ddiExistRecurLevel>0)) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout,
          "ZERO result of AND-EXIST at step %d.\n", ddiExistRecurLevel)
      );
    }
#endif

    if (Ddi_BddIsZero(r)) {
      Ddi_Free(r);
      r = rTot;
    }
    else {
      Ddi_BddOrAcc(r,rTot);
      Ddi_Free(rTot);
    }


  Ddi_BddSetFlattened(r);

  Ddi_BddSuppDetach(r);
#if 1
  if (!Ddi_BddIsMeta(r)) {
    if (ddiMgr->settings.part.existOptClustThresh >= 0) {
      Ddi_BddSetClustered(r,ddiMgr->settings.part.existOptClustThresh);
    }
    else {
      Ddi_BddSetMono(r);
    }
  }
#endif

  if (0)
  {
    Ddi_Varset_t *supp = Ddi_BddSupp(r);
    Ddi_VarsetIntersectAcc(supp,smoothV);
    if (!Ddi_VarsetIsVoid(supp)) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "NOT VOID (%d).\n", Ddi_VarsetNum(supp))
      );
    }
    Ddi_Free(supp);
  }

  Ddi_Free(part);
  Ddi_Free(constrInt);

  } while (((ddiMgr->exist.clipLevel == 1) && (ddiMgr->exist.clipDone != 0)) ||
           ((ddiMgr->exist.clipLevel == 3) && 
	    (ddiMgr->exist.clipDone != 0) && Ddi_BddIsZero(r)) );

  if (freeClipWindow) {
    if (ddiMgr->exist.clipLevel%2 == 1 ) {
      Ddi_Free(ddiMgr->exist.clipWindow);
    }
    else if ((ddiMgr->exist.clipWindow!=NULL) && 
	     Ddi_BddIsOne(ddiMgr->exist.clipWindow)) {
      Ddi_Free(ddiMgr->exist.clipWindow);
      ddiMgr->exist.clipWindow = NULL;
    }
  }

  Ddi_Free(mySmoothV);
  return(r);
}


/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

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
  Ddi_Bdd_t *dpartConstr,
  float maxGrowth,
  Ddi_Bdd_t *overApprTerm   /* factor to be replicated with FPart
                               when clustering */,
  int threshold             /* Size threshold for result factor */,
  int disjPartEna,
  int metaDecompEna         /* enable meta BDD decomposition */,
  int constrainOptFail      /* allowed failures for constrain opt */
)
{
  static int callCnt = 0;
  Ddi_Mgr_t *dd = Ddi_ReadMgr(FPart);    /* dd manager */

  int myCallCnt;
  int myClipDone = dd->exist.clipDone;

  Ddi_Bdd_t *Res;
  Ddi_Bdd_t *F, *P, *Pnew, *Prod, *clust, *over=NULL;
  Ddi_Varset_t *recSmooth, *smoothF, *smoothPF, *smoothP,
    *suppF, *suppP, *suppProd;
  int i, n, jConstr=-1;

  Pdtutil_VerbLevel_e verbosity;
  int extRef, clipRef;

  myCallCnt = callCnt++;

  verbosity = Ddi_MgrReadVerbosity(dd);

  if (dpartConstr!=NULL) {
    int np = Ddi_BddPartNum(FPart);
    jConstr = Ddi_BddPartNum(dpartConstr)-Ddi_BddPartNum(FPart);
    if (np>1 && jConstr>=0) {
      Ddi_Bdd_t *newConstr=NULL;
      Ddi_Bdd_t *constr_j = Ddi_BddPartRead(dpartConstr,jConstr);
      Ddi_Bdd_t *part_j = Ddi_BddPartRead(FPart,0);
      ConstrainOptAcc(part_j,constr_j,1);
      newConstr = Ddi_BddNot(part_j);
      ConstrainOptAcc(constr_j,newConstr,1);
      Ddi_BddAndAcc(constr_j,newConstr);
      Ddi_Free(newConstr);
    }
  }

  if (Ddi_BddPartNum(FPart)<=2) {
    Ddi_Bdd_t *p2;

    ConstrainOptAcc(FPart,constrain,1);
    if (Ddi_BddPartNum(FPart)==2) {
      p2 = Ddi_BddPartRead(FPart,1);
    }
    else {
      p2 = Ddi_MgrReadOne(dd);
    }

    n = Ddi_VarsetNum(smoothV);
    if (n>0) {
      Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "L:<%d>", n)
      );
    }

    if (dd->exist.clipDone) {
      Ddi_BddAndAcc(Ddi_BddPartRead(FPart,0),dd->exist.clipWindow);
    }

    Pnew = AndExistWithAbort (Ddi_BddPartRead(FPart,0),
      p2,NULL,smoothV,constrain,threshold,NULL,-1,metaDecompEna);

    if (Pnew == NULL) {
      Pnew = Ddi_BddDup(FPart);
    }
    Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "|=%d|", Ddi_BddSize(Pnew))
    );
    ConstrainOptAcc(Pnew,constrain,1);

    if (n>0 && (overApprTerm != NULL) && (threshold < 0)) {
      Ddi_Varset_t *supp = Ddi_BddSupp(Pnew);
      Ddi_VarsetIntersectAcc(supp,smoothV);
      if (!Ddi_VarsetIsVoid(supp)) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
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
  clipRef = (dd->exist.clipWindow != NULL);

  Prod = FPart;

  P = Ddi_BddPartExtract (Prod, 0);

#if 0
  if (Ddi_BddSize(P) > 100000) {
    /* force conversion to meta !!! Disabled !!!*/
     /*TestPart(P);*/
#if 0
     if (dd->meta.groups!=NULL) {
      Ddi_Bdd_t *a = Ddi_BddDup(P);
        dd->settings.meta.method = Ddi_Meta_Size;
        Ddi_BddSetMeta(a);
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "META1: %d(0:%d(%d),1:%d(%d)).\n", Ddi_BddSize(a),
            Ddi_BddarraySize(a->data.meta->zero),
            Ddi_BddSize(Ddi_BddarrayRead(a->data.meta->zero,
            Ddi_BddarrayNum(a->data.meta->zero)-1)),
            Ddi_BddarraySize(a->data.meta->one),
            Ddi_BddSize(Ddi_BddarrayRead(a->data.meta->one,
            Ddi_BddarrayNum(a->data.meta->one)-1)))
        );
      Ddi_Free(a);
        dd->settings.meta.method = Ddi_Meta_Size_Window;
      a = Ddi_BddDup(P);
        Ddi_BddSetMeta(a);
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "META2: %d(0:%d(%d),1:%d(%d)).\n", Ddi_BddSize(a),
            Ddi_BddarraySize(a->data.meta->zero),
            Ddi_BddSize(Ddi_BddarrayRead(a->data.meta->zero,
            Ddi_BddarrayNum(a->data.meta->zero)-1)),
            Ddi_BddarraySize(a->data.meta->one),
            Ddi_BddSize(Ddi_BddarrayRead(a->data.meta->one,
          Ddi_BddarrayNum(a->data.meta->one)-1)))
        );
      Ddi_Free(a);
     }
#else
     if (dd->meta.groups!=NULL) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "\n")
        );
        Ddi_BddSetMeta(P);
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "META: %d.\n", Ddi_BddSize(P))
        );
     }
#endif
  }
#endif


#if 0
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "{Proj:%d->", Ddi_BddSize(P))
  );
  P = Ddi_BddEvalFree(Ddi_BddProject(P,Prod,smoothV),P);
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMin_c,
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

  Ddi_BddSuppDetach(P);
  Ddi_BddSuppDetach(F);

  /* Early smoothing on P */
  smoothP = Ddi_VarsetDiff(smoothPF, suppF);

  if (!(Ddi_VarsetIsVoid (smoothP))) {
    if (Ddi_BddIsPartConj(P)&&(Ddi_BddPartNum(P)==1)) {
#if 0
      Ddi_Bdd_t *constrain2 = NULL;
      if (constrain != NULL) {
        constrain2 = Ddi_BddDup(constrain);
      }
      P = Ddi_BddEvalFree(MultiwayRecursiveAndExistStep(P,smoothP,constrain2,
	dpartConstr,maxGrowth,overApprTerm,threshold,disjPartEna,
        metaDecompEna,constrainOptFail),P);
      Ddi_Free(constrain2);
#endif
    }
#if 1
    else if (Ddi_BddIsPartConj(P)) {
      int k;
      Ddi_Bdd_t *over;

#if 0
      if (constrain != NULL) {
        constrain2 = Ddi_BddDup(constrain);
      }
      P = Ddi_BddEvalFree(MultiwayRecursiveAndExistStep(P,smoothP,constrain2,
        dpartConstr,maxGrowth,overApprTerm,threshold,disjPartEna,
        metaDecompEna,constrainOptFail),P);
      Ddi_Free(constrain2);
#endif
      if (Ddi_BddIsPartConj(P)) {
        over = Ddi_BddExistOverApprox(P,smoothP); /* @@@@@@@@ opt ! @@@@ */
        if (!Ddi_BddIsOne(over)) {
#if 0
        Ddi_BddRestrictAcc(P,over);
#else
          ConstrainOptAcc(P,over,0);
#endif
          Ddi_BddSetPartConj(over);
          for (k=0;k<Ddi_BddPartNum(over);k++) {
            if (!Ddi_BddIsOne(Ddi_BddPartRead(over,k))) {
              Ddi_BddPartInsertLast(P,Ddi_BddPartRead(over,k));
          }
          }
        }
        Ddi_Free(over);
      }
    }
#endif
    else {
#if 1
      Ddi_BddExistAcc(P,smoothP);
      Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "{P:<%d>%d}", Ddi_VarsetNum(smoothP),Ddi_BddSize(P))
      );
#endif
    }
    ConstrainOptAcc(P,constrain,1);
  }

  /* Early smoothing on F */
  smoothF = Ddi_VarsetDiff(smoothPF, suppP);
  if (!(Ddi_VarsetIsVoid (smoothF))) {
    if (Ddi_BddIsPartConj(F)/*&&(Ddi_BddPartNum(F)==1)*/) {
      if (threshold>0) {
        Ddi_Bdd_t *constrain2 = NULL;
        if (constrain != NULL) {
          constrain2 = Ddi_BddDup(constrain);
        }
        F = Ddi_BddEvalFree(MultiwayRecursiveAndExistStep(F,smoothF,constrain2,
          dpartConstr,maxGrowth,overApprTerm,threshold,disjPartEna,
          metaDecompEna,constrainOptFail),F);
        Ddi_Free(constrain2);
      }
    }
    else if (!(Ddi_VarsetIsVoid (smoothF))) {
      Ddi_BddExistAcc(F,smoothF);
      Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "{F:<%d>%d}", Ddi_VarsetNum(smoothF),Ddi_BddSize(F))
      );
    }
    ConstrainOptAcc(F,constrain,1);
  }
  Ddi_Free(smoothF);

  n = Ddi_VarsetNum(smoothPF);
  if (n>0)
    Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "PF(-%d):|%d|", Ddi_BddPartNum(Prod),n)
    );

#if 0
  Pnew = PartAndExistWithAbort (P, F, smoothPF, constrain,
    (int)(maxGrowth*(Ddi_BddSize(P)+Ddi_BddSize(F))),NULL,-1);
#else
  if (Ddi_BddIsOne(F)&&n<=0) {
    Pnew = Ddi_BddDup(P);
    Ddi_Free(smoothPF);
    smoothPF = Ddi_VarsetVoid(dd);
  }
  else {
    if (dd->exist.clipDone) {
      Ddi_Bdd_t *constr;
      Ddi_Varset_t *localSmooth = Ddi_VarsetDiff(smoothV, suppProd);
      Ddi_VarsetDiffAcc(localSmooth, smoothPF);
      constr = Ddi_BddMakeMono(dd->exist.clipWindow);
      Ddi_BddExistAcc(constr,localSmooth);
      Ddi_BddAndAcc(F,constr);
      Ddi_Free(localSmooth);
      Ddi_Free(constr);
    }
    Pnew = AndExistWithAbort (P, F, NULL, smoothPF,
      constrain, threshold,NULL,-1,metaDecompEna);
    if ((dd->exist.conflictProduct!=NULL)&&(Pnew!=NULL)&&Ddi_BddIsZero(Pnew)&&
      (Ddi_BddSize(P)<500)) {
      Ddi_Bdd_t *myConflict = Ddi_BddDup(P);
      if (constrain != NULL) {
        Ddi_BddSetPartConj(myConflict);
        Ddi_BddPartInsertLast(myConflict,constrain);
      }
      if (Ddi_BddIsOne(dd->exist.conflictProduct)) {
        Ddi_Free(dd->exist.conflictProduct);
        dd->exist.conflictProduct = myConflict;
      }
      else {
        Ddi_BddSetPartDisj(dd->exist.conflictProduct);
        Ddi_BddPartInsertLast(dd->exist.conflictProduct,myConflict);
        Ddi_Free(myConflict);
      }
    }
  }
#endif

#if 0
  if (overApprTerm == NULL) {
    recSmooth = Ddi_VarsetDiffAcc(smoothV,smoothPF);
  }
  else {
    recSmooth = smoothV;
  }
#else
  recSmooth = smoothV;
#endif
  Ddi_Free (smoothP);
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
#if !WEAK_EXIST
      Ddi_Bdd_t *myOver;
      Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity(dd);

      myOver = Ddi_BddExistOverApprox(P,recSmooth);
      if (over == NULL) {
        over = Ddi_BddMakeConst(dd,1);
      }
      Ddi_BddSetPartConj(over);
      Ddi_BddPartInsertLast(over,myOver);
      Ddi_BddSetFlattened(over);
      Ddi_BddSetClustered(over,dd->settings.part.existOptClustThresh);

      /*Pdtutil_Assert(!metaDecompEna,
        "not compatible overappr trem with meta decomp");*/

      Pnew = Ddi_BddDup(overApprTerm);

      /* bring overlapping variables when overapproximating */
      Ddi_BddExistProjectAcc(myOver,suppProd);
      if (!Ddi_BddIsOne(myOver)) {
      Ddi_BddAndAcc(F,myOver);
      }
      Ddi_Free(myOver);

      Ddi_BddSetPartConj(Pnew);
      Ddi_BddPartInsertLast(Pnew,F);
#else
      Pnew = WeakExist(P,recSmooth,0.7);
      Ddi_BddSetPartConj(Pnew);
      Ddi_BddPartInsertLast(Pnew,overApprTerm);
#endif
    }
    Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "@")
    );
  }

  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "|=%d|", Ddi_BddSize(Pnew))
  );

  Ddi_Free (suppP);
  Ddi_Free (suppF);
  Ddi_Free (suppProd);
  if (0&&!dd->exist.clipDone) {
    Ddi_Free (F);
    Ddi_Free (P);
  }
  ConstrainOptAcc(Pnew,constrain,1);

#if 1
  if ((over==NULL)&&(metaDecompEna)&&(constrain != NULL)&&
      (dd->settings.part.existOptLevel >= 2) &&
          (Ddi_BddSize(Pnew)>dd->settings.part.existOptClustThresh) &&
     (Ddi_BddSize(Pnew)>dd->settings.part.existOptClustThresh)) {
    Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity(dd);

    over = Ddi_BddExistOverApprox(Pnew,smoothV);

    if (!Ddi_BddIsOne(over)) {
      int reduced;
      ConstrainOptAcc(over,constrain,1);
      reduced = ConstrainOptAcc(Pnew,over,1);
#if 0
      Ddi_BddAndAcc(
        Ddi_BddPartRead(constrain,Ddi_BddPartNum(constrain)-1),over);
#else
      if (reduced) {
        int l;

        Ddi_Bdd_t *pp = Ddi_BddPartExtract(constrain,
              Ddi_BddPartNum(constrain)-1);
        Ddi_BddSetPartConj(pp);
        Ddi_BddPartInsertLast(pp,over);

        if (dd->settings.part.existOptClustThresh >= 0) {
          Ddi_BddSetClustered(pp,dd->settings.part.existOptClustThresh);
          for (l=0; l<Ddi_BddPartNum(pp); l++) {
            if (!Ddi_BddIsOne(Ddi_BddPartRead(pp,l))) {
              Ddi_BddPartInsertLast(constrain,Ddi_BddPartRead(pp,l));
          }
          }
        }
        else {
          Ddi_BddSetMono(pp);
          if (!Ddi_BddIsOne(pp)) {
            Ddi_BddPartInsertLast(constrain,pp);
        }
        }
        if (Ddi_BddPartNum(constrain) == 0) {
          Ddi_BddPartInsertLast(constrain,Ddi_MgrReadOne(dd));
      }
        Ddi_Free(pp);
        /*Ddi_BddSetClustered(constrain,5000);*/
#if 1
        Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMin_c,
          fprintf(stdout, "[=%d,%d]", Ddi_BddSize(over),Ddi_BddSize(Pnew))
        );
#endif
      }
#endif
      else {
        Ddi_Free(over);
        over = NULL;
      }
    }
    else {
      Ddi_Free(over);
      over = NULL;
    }
  }
#endif

  if ((disjPartEna>0)&&(Ddi_BddSize(Pnew) > disjPartEna)&&
      (Ddi_BddPartNum(Prod)>2)) {
    Ddi_Bdd_t *fPart, *fi, *ri;
    int isMono = Ddi_BddIsMono(Pnew);
    int np;
    Ddi_Bdd_t *myConstrain = Ddi_BddMakeMono(constrain);
    int varSplit = 1;
    int standardSat = 1;
    int constrCovered = 0;
    int freeDpartConstr = 0;

    //    Ddi_BddSetPartConj(myConstrain);
    disjPartEna *= 4;

    Ddi_Free(F);
    Ddi_Free(P);

    Res = Ddi_BddMakeConst(dd,0);
    Ddi_BddSetPartDisj(Res);

    if (constrCovered && dpartConstr==NULL) {
      Ddi_Bdd_t *myOne = Ddi_BddMakeConst(dd,1);
      dpartConstr = Ddi_BddMakePartConjVoid(dd);
      for (i=0; i<Ddi_BddPartNum(Prod); i++) {
	Ddi_BddPartWrite(dpartConstr,i,myOne);
      }
      freeDpartConstr = 1;
      Ddi_Free(myOne);
    }

    if (varSplit) {
      int siftThreshIncr = Ddi_MgrReadLiveNodes(dd);

#if 1
      fPart = Part_PartitionDisjSetWindow (Pnew, NULL,NULL,
	    isMono?Part_MethodEstimate_c:Part_MethodEstimateFast_c,
            Ddi_BddSize(Pnew)/3,5,verbosity);
    //      Part_MethodCofactorSupport_c,Ddi_BddSize(Pnew)/3,5,verbosity);
#else
      fPart = Part_PartitionSetCudd (Pnew,
				     /*Part_MethodAppCon_c*/
				     /*Part_MethodIteCon_c*/
				     /*Part_MethodGenCon_c*/
				     Part_MethodVarDis_c,
         Ddi_BddSize(Pnew)/3, Pdtutil_VerbLevelDevMax_c );
#endif
      np = Ddi_BddPartNum(fPart);
      Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "PartNum = %d.\n", Ddi_BddPartNum (fPart))
      );

      Ddi_BddPartInsert(Prod,0,Pnew);
      Ddi_Free(Pnew);
      Pnew = fPart;

      Ddi_BddSetPartDisj(Pnew);
      for (i=Ddi_BddPartNum(Pnew)-1; i>=0; i--) {
	Ddi_Bdd_t *myProd = Ddi_BddDup(Prod);
	fi = Ddi_BddPartExtract(Pnew,i);
	//	Ddi_BddPartInsert(myProd,0,fi);
	Ddi_BddConstrainAcc(myProd,fi);
	Ddi_BddExistAcc(fi,smoothV);
	if (!Ddi_BddIsOne(fi)) {
	  if (constrCovered) {
	    Ddi_BddPartInsert(myProd,0,fi);
	  }
	  else {
	    Ddi_BddPartInsertLast(myProd,fi);
	  }
	}
	Ddi_Free(fi);
	Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMin_c,
	  fprintf(stdout, "\nPart #: %d/%d (level: -%d).\n", 
	  np-i-1, np, Ddi_BddPartNum(myProd)-1)
	);
	ri = MultiwayRecursiveAndExistStep(myProd,recSmooth,
	      myConstrain,dpartConstr,maxGrowth,
	      overApprTerm,threshold,disjPartEna,
              metaDecompEna,constrainOptFail);
	Ddi_BddPartInsert(Res,0,ri);
	Ddi_BddNotAcc(ri);
	//      Ddi_BddPartInsertLast(myConstrain,ri);
	Ddi_Free(ri);
	Ddi_Free(myProd);
      }
    }
    else {
      Ddi_Bdd_t *PnewAig = Ddi_BddMakeAig(Pnew);
      Ddi_Bdd_t *ProdAig = Ddi_BddMakeAig(Prod);
      Ddi_Bdd_t *ProdBdd = Ddi_BddDup(Prod);
      int endPart=0, maxPartNum=8;
      Ddi_Varset_t *mySupp=NULL;
      int maxSize = Ddi_BddSize(Pnew)/3;
      Ddi_Bdd_t *residualWindow = Ddi_BddMakeConst(dd,1);

      mySupp = Ddi_BddSupp(PnewAig);

      Ddi_BddSetPartConj(ProdAig);
      Ddi_BddPartInsertLast(ProdAig,PnewAig);
      Ddi_BddPartInsertLast(ProdBdd,myConstrain);

      Ddi_Free(PnewAig);
      //      Ddi_BddSetAig(ProdAig);

      for (i=0; !endPart; i++) {
	Ddi_Bdd_t *cex=NULL;
	if (i>=maxPartNum) {
	  Ddi_Bdd_t * myProd = Ddi_BddConstrain(Prod,residualWindow); 
	  Ddi_Bdd_t * myPnew = Ddi_BddConstrain(Pnew,residualWindow); 
	  endPart = 1;

	  Ddi_BddPartInsert(myProd,0,myPnew);
	  Ddi_BddPartInsert(myProd,0,residualWindow);

	  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMin_c,
            fprintf(stdout, "\nPart #: %d (level: -%d) [last].\n", 
		    i, Ddi_BddPartNum(myProd)-1)
          );

	  ri = MultiwayRecursiveAndExistStep(myProd,recSmooth,
	     myConstrain,dpartConstr,maxGrowth,overApprTerm,
             threshold,disjPartEna,
	     metaDecompEna,constrainOptFail);
	  Ddi_BddPartInsert(Res,0,ri);

	  Ddi_Free(ri);
	  Ddi_Free(myProd);
	  Ddi_Free(myPnew);
	}
	else {
	  Ddi_Bdd_t *constrAig = Ddi_BddMakeAig(myConstrain);
	  Ddi_Bdd_t *myProdAig = Ddi_BddDup(ProdAig);
	  //	  Ddi_BddPartInsertLast(myProdAig,constrAig);
	  Ddi_Varset_t *cVars = Ddi_BddSupp(myConstrain);

	  Ddi_Bdd_t *pivotCube = Ddi_BddPickOneMinterm(myConstrain,cVars); 

	  Ddi_Free(cVars);
	  Ddi_BddSetAig(myProdAig);
	  Ddi_BddSetAig(pivotCube);

	  if (standardSat) {
	    int abo;
	    cex = Ddi_AigSatAndWithCexAndAbort(myProdAig,
					       constrAig,NULL,NULL,5,&abo);
	    if (abo) {
	      cex = Ddi_AigSatWithCexAndAbort(myProdAig,NULL,-1,&abo);
	    }
	  }
	  else {
	    Ddi_Bdd_t *w = NULL;
	    cex = NULL;
	    w = Ddi_AigSatWindow(ProdBdd,Pnew,pivotCube,NULL,-1.0);
	    if (w != NULL) {
	      cex = Ddi_BddDup(pivotCube);
	    }
	  }
	  Ddi_Free(myProdAig);
	  Ddi_Free(constrAig);
	  Ddi_Free(pivotCube);
	  if (cex==NULL) {
	    endPart = 1;
	    printf("partitioned AE ended\n");
	  }
	}
	if (!endPart) {
	  int jj, nvars=0;
	  Ddi_Bdd_t *cexPart=NULL;
	  Ddi_Bdd_t *myProd = Ddi_BddConstrain(Prod,residualWindow); 
	  Ddi_Bdd_t *myPnew = Ddi_BddConstrain(Pnew,residualWindow); 
	  Ddi_Bdd_t *myWindow = NULL;
	  Ddi_Bdd_t *myProdLast = 
            Ddi_BddPartRead(myProd,Ddi_BddPartNum(myProd)-1);
	  Ddi_BddSetMono(cex);
	  Ddi_BddExistProjectAcc(cex,mySupp);
	  //	  cexPart = Ddi_BddMakePartConjFromCube(cex);

          fPart = Part_PartitionDisjSet (Pnew, cex, mySupp,
	    isMono?Part_MethodEstimate_c:Part_MethodEstimateFast_c,
            maxSize/2,8,verbosity);

	  if (Ddi_BddIsPartConj(fPart)) {
	    myWindow = Ddi_BddDup(Ddi_BddPartRead(fPart,0));
	    Ddi_DataCopy(myPnew,Ddi_BddPartRead(fPart,1));
	    Ddi_BddConstrainAcc(myPnew,myWindow);
	    Ddi_BddConstrainAcc(myProd,myWindow);
	    Ddi_BddAndAcc(myPnew,myWindow);
	  }
	  else {
	    myWindow = Ddi_BddMakeConst(dd,1);
	  }
	  nvars = Ddi_BddSize(myWindow);
	  Ddi_Free(fPart);
#if 0
	  for (jj = 0; jj<Ddi_BddPartNum(cexPart); jj++) {
	    Ddi_Bdd_t *lit = Ddi_BddPartRead(cexPart,jj);
	    Ddi_Var_t *v = Ddi_BddTopVar(lit);
	    Ddi_BddAndAcc(myWindow,lit);
	    if (!Ddi_VarInVarset(recSmooth,v)) {
	      Ddi_BddAndAcc(myProdLast,lit);
	    }
	    nvars++;
	    if (Ddi_BddSize(myPnew) < maxSize) {
	      break;
	    }
	  }
#endif
	  Ddi_BddPartInsert(myProd,0,myPnew);
	  Ddi_BddPartInsert(myProd,0,residualWindow);

	  Ddi_BddDiffAcc(residualWindow,myWindow);
	  Ddi_BddDiffAcc(Pnew,myWindow);
	  Ddi_BddSetAig(myWindow);
	  Ddi_BddDiffAcc(ProdAig,myWindow);
	  Ddi_Free(myWindow);

	  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMin_c,
            fprintf(stdout, "\nPart #: %d (level: -%d) [nvars: %d].\n", 
		    i, Ddi_BddPartNum(myProd)-1, nvars)
          );

	  ri = MultiwayRecursiveAndExistStep(myProd,recSmooth,
	     myConstrain,dpartConstr,maxGrowth,overApprTerm,
             threshold,disjPartEna,
	     metaDecompEna,constrainOptFail);
	  Ddi_BddPartInsert(Res,0,ri);
	  Ddi_BddNotAcc(ri);
	  Ddi_BddConstrainAcc(myConstrain,ri);
	  Ddi_BddAndAcc(myConstrain,ri);
	  Ddi_BddNotAcc(ri);
	  Ddi_Free(cex);
	  Ddi_BddSetAig(ri);
	  //	  Ddi_BddDiffAcc(ProdAig,ri);
	  Ddi_Free(ri);
	  Ddi_BddNotAcc(myPnew);
	  //	  Ddi_BddConstrainAcc(Pnew,myPnew);
	  Ddi_Free(myProd);
	  Ddi_Free(myPnew);
	}
      }

      Ddi_Free(mySupp);
      Ddi_Free(residualWindow);
      Ddi_Free(ProdAig);
      Ddi_Free(ProdBdd);
    }

    if (freeDpartConstr) {
      Ddi_Free(dpartConstr);
    }

    Ddi_Free(myConstrain);
    Ddi_Free(Pnew);
    while (Ddi_BddPartNum(Prod) > 0) {
      fi = Ddi_BddPartExtract(Prod,0);
      Ddi_Free(fi);
    }
    if (isMono) {
      Ddi_BddSetMono(Res);
    }
  }
  else {
    Ddi_Bdd_t *saveProd = NULL;

#if 0
    if (clipDone) {
      clipDone = 0;
      saveProd = Ddi_BddDup(Prod);
      Ddi_BddNotAcc(clipWindow);
      Ddi_BddPartInsert(saveProd,0,F);
      Ddi_BddPartInsert(saveProd,0,P);
      Ddi_Free(F);
      Ddi_Free(P);
      Ddi_BddPartInsert(saveProd,0,clipWindow);
      Ddi_Free(clipWindow);
    }
#endif

    if (0 && dd->exist.clipDone) {
      saveProd = Ddi_BddDup(Prod);
      Ddi_BddPartInsert(saveProd,0,F);
      Ddi_BddPartInsert(saveProd,0,P);
    }
    Ddi_Free(F);
    Ddi_Free(P);

    if (!(Ddi_BddIsOne(Pnew)||Ddi_BddIsZero(Pnew))) {
      if ((dd->settings.part.existOptLevel >= 3) &&
          (Ddi_BddSize(Pnew)>dd->settings.part.existOptClustThresh )) {
        ConstrainOptAcc(Pnew,Prod,0);
#if 0
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "<<<%d+%d",Ddi_BddSize(Pnew),Ddi_BddSize(Prod))
        );
        if (over!=NULL)
         Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
           Pdtutil_VerbLevelNone_c,
           fprintf(stdout, "(O:%d)",Ddi_BddSize(over))
         );
        if (clust!=NULL)
         Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
           Pdtutil_VerbLevelNone_c,
           fprintf(stdout, "(C:%d)",Ddi_BddSize(clust))
         );
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, ">>>")
      );
#endif
      }
      Ddi_BddSuppAttach(Pnew);
      if (dd->settings.part.existOptLevel >= 2
        && !Ddi_BddIsMeta(Pnew) && !Ddi_BddIsAig(Pnew)) {
        if (constrainOptFail) {
        if (ConstrainOptAcc(Prod,Pnew,1)) {
          constrainOptFail = 3;
        }
        else {
          constrainOptFail--;
        }
      }
      }
      Ddi_BddPartInsert(Prod,0,Pnew);
    }

    if (Ddi_BddIsZero(Pnew)) {
      Res = Pnew;
      dd->exist.ddiExistRecurLevel = Ddi_BddPartNum(Prod);
    }
    else {

      Ddi_Free(Pnew);

      Res = MultiwayRecursiveAndExistStep(Prod,recSmooth,
        constrain,dpartConstr,maxGrowth,overApprTerm,threshold,disjPartEna,
        metaDecompEna,constrainOptFail);
    }

    if ((saveProd != NULL) && (dd->exist.clipRetry < 3) &&
        (Ddi_BddIsZero(Res) && (dd->exist.clipDone>myClipDone))) {
      int myClipThresh = dd->exist.clipThresh;
      Ddi_Bdd_t *window;
      dd->exist.clipRetry++;
      if (Ddi_BddIsMono(dd->exist.clipWindow) || 
	  (Ddi_BddPartNum(dd->exist.clipWindow) == 1)) {
        Ddi_BddSetMono(dd->exist.clipWindow);
	window = dd->exist.clipWindow;
	dd->exist.clipWindow = Ddi_BddMakeConst(dd,1);
      }
      else {
	window = Ddi_BddPartExtract(dd->exist.clipWindow,
				  Ddi_BddPartNum(dd->exist.clipWindow)-1);
      }
      dd->exist.clipDone--;
      Pdtutil_Assert(dd->exist.clipDone >= 0, "Bug: clipDone < 0");
      Ddi_BddNotAcc(window);
      Ddi_BddPartInsert(saveProd,0,window);
      Ddi_Free(window);
      Ddi_Free(Res);
      dd->exist.clipThresh = (int) (dd->exist.clipThresh * 1.5);
      Res = MultiwayRecursiveAndExistStep(saveProd,recSmooth,
        constrain,dpartConstr,maxGrowth,overApprTerm,threshold,disjPartEna,
        metaDecompEna,constrainOptFail);
      dd->exist.clipThresh = myClipThresh;
      dd->exist.clipRetry--;
    }
    Ddi_Free(saveProd);

    if (saveProd != NULL) {
      Ddi_Bdd_t *myCare, *res2;
      if (constrain == NULL) {
        myCare = Ddi_BddNot(Res);
        Ddi_BddSetPartConj(myCare);
      }
      else {
        myCare = Ddi_BddDup(constrain);
        Ddi_BddNotAcc(Res);
        Ddi_BddSetPartConj(myCare);
        Ddi_BddPartInsertLast(myCare,Res);
        Ddi_BddNotAcc(Res);
      }
      res2 = MultiwayRecursiveAndExistStep(saveProd,recSmooth,
	myCare,NULL,maxGrowth,overApprTerm,threshold,disjPartEna,
        metaDecompEna,constrainOptFail);
      Ddi_Free(myCare);
      Ddi_Free(saveProd);
      Ddi_BddOrAcc(Res, res2);
      Ddi_Free(res2);
    }

  }

  /* Ddi_Free(recSmooth); no more dup ! recSmooth is now smoothV */

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

    if ((dd->exist.conflictProduct!=NULL)&&
	(!Ddi_BddIsConstant(dd->exist.conflictProduct))) {
#if 1
      Ddi_BddSetPartConj(dd->exist.conflictProduct);
      Ddi_BddPartInsertLast(dd->exist.conflictProduct,over);
#else
      Ddi_BddAndAcc(dd->exist.conflictProduct,over);
      if (Ddi_BddIsZero(dd->exist.conflictProduct)) {
        Ddi_Free(dd->exist.conflictProduct);
        dd->exist.conflictProduct = Ddi_BddDup(over);
      }
#endif
    }

    Ddi_Free(over);
  }

  if ((dd->exist.clipWindow != NULL)&&(!clipRef)) {
    extRef++;
  }
  if ((dd->exist.clipWindow == NULL)&&(clipRef)) {
    extRef--;
  }
  Ddi_MgrCheckExtRef(dd,extRef+1);

  if (0)
  {
    Ddi_Varset_t *supp = Ddi_BddSupp(Res);
    Ddi_VarsetIntersectAcc(supp,smoothV);
    if (!Ddi_VarsetIsVoid(supp)) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "#")
      );
    }
    Ddi_Free(supp);
  }


  return (Res);
}

/**Function********************************************************************
  Synopsis    [Optimized constrain cofactor.]
  Description [return constrain only if size reduced]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
static int
ConstrainOptAcc (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g,
  int logVerbose
)
{
  int reduced;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(f);

  static int reduceFailCnt = 0;
  static int activityCnt = 0;
  static int activityStep = 1;

  if (g==NULL || Ddi_BddIsOne(g)) {
    return(0);
  }

  if (ddiMgr->settings.part.existOptLevel <= 0) {
    return(0);
  }

  if (0&&(Ddi_MgrReadLiveNodes(ddiMgr) < ((3 * ddiMgr->autodynNextDyn) / 4))) {

    if (activityCnt < activityStep) {
      activityCnt++;
      return(0);
    }

    activityCnt++;
    activityCnt %= activityStep;

  }

  Ddi_MgrAbortOnSiftEnableWithThresh(ddiMgr,2*Ddi_BddSize(f),1);

  reduced = ConstrainOptAccIntern(f,g,logVerbose);

  Ddi_MgrAbortOnSiftDisable(ddiMgr);

  if (!reduced) {
    reduceFailCnt++;
  }
  else {
    reduceFailCnt = 0;
  }

  if (reduceFailCnt > 3 && activityStep < 30) {
    reduceFailCnt = 0;
    activityStep++;
  }
  else if (reduced && activityStep > 1) {
    activityStep /= 2;
  }

  return(reduced);
}


/**Function********************************************************************
  Synopsis    [Optimized constrain cofactor.]
  Description [return constrain only if size reduced]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
static int
ConstrainOptAccIntern (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g,
  int logVerbose
)
{
  Ddi_Bdd_t *r, *p;
  int i;
  int size0 = (-1);
  int size1 = (-1);
  int reduced;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(f);
  Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity(ddiMgr);

  //  return 0;

  if (logVerbose && (Ddi_MgrReadVerbosity(ddiMgr) >= Pdtutil_VerbLevelDevMax_c)) {
    size0 = Ddi_BddSize(f);
  }

  reduced = 0;

  if (Ddi_BddIsPartConj(f)) {
    for (i=0; i<Ddi_BddPartNum(f); i++) {
      p = Ddi_BddPartRead(f,i);
      reduced = ConstrainOptAccIntern(p,g,0) || reduced;
    }
  }
  else if (Ddi_BddIsPartConj(g)) {
    for (i=0; i<Ddi_BddPartNum(g); i++) {
      p = Ddi_BddPartRead(g,i);
      reduced = ConstrainOptAccIntern(f,p,0) || reduced;
    }
  }
  else {

    r = Ddi_BddRestrict(f,g);
    if ((!Ddi_BddEqual(r,f))&&(Ddi_BddSize(r)< /*0.99* */Ddi_BddSize(f))) {
      DdiGenericDataCopy((Ddi_Generic_t *)f,(Ddi_Generic_t *)r);
      reduced = 1;
    }

    Ddi_Free(r);

  }

  if (logVerbose && (Ddi_MgrReadVerbosity(ddiMgr) >= Pdtutil_VerbLevelDevMax_c)) {
    size1 = Ddi_BddSize(f);
    if (size1 < 0.9*size0) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "({CO:%d->%d})",size0,size1)
      );
    }
  }

  return(reduced);

}


/**Function********************************************************************
  Synopsis    [Optimized constrain cofactor.]
  Description [return constrain only if size reduced]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
AndExistWithAbort (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g,
  Ddi_Varset_t *smoothf,
  Ddi_Varset_t *smoothV,
  Ddi_Bdd_t  *care,
  int thresh,
  Ddi_Bdd_t *orTerm,
  int aboThresh,
  int metaDecompEna         /* enable meta BDD decomposition */
)
{
  Ddi_Bdd_t *r;
  int i, abortThresh;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(f);

  while ((Ddi_BddIsPartConj(f)||Ddi_BddIsPartDisj(f))&&
         (Ddi_BddPartNum(f)==1)) {
     f = Ddi_BddPartRead(f,0);
     Pdtutil_Assert(f!=NULL,"NULL partition");
  }
  while ((Ddi_BddIsPartConj(g)||Ddi_BddIsPartDisj(g))&&
         (Ddi_BddPartNum(g)==1)) {
     g = Ddi_BddPartRead(g,0);
     Pdtutil_Assert(g!=NULL,"NULL partition");
  }

  if (0 && ((ddiMgr->exist.activeWindow != NULL && 
	     !Ddi_BddIsOne(ddiMgr->exist.activeWindow)) ||
      (ddiMgr->exist.clipWindow != NULL && 
       !Ddi_BddIsOne(ddiMgr->exist.clipWindow))) &&
      (Ddi_BddIsMono(f) || Ddi_BddIsMono(g))) {
    Ddi_Varset_t *suppf, *suppg;
    Ddi_Bdd_t *myWindow = Ddi_BddMakeConst(ddiMgr,1);
    if (ddiMgr->exist.activeWindow != NULL) {
      Ddi_BddAndAcc(myWindow,ddiMgr->exist.activeWindow);
    }
    if (ddiMgr->exist.clipWindow != NULL) {
      Ddi_BddAndAcc(myWindow,ddiMgr->exist.clipWindow);
    }
    suppf = Ddi_BddSupp(f);
    suppg = Ddi_BddSupp(g);
    Ddi_VarsetUnionAcc(suppf,suppg);
    Ddi_Free(suppg);
    Ddi_BddExistProjectAcc(myWindow,suppf);
    Ddi_Free(suppf);
    if (Ddi_BddIsMono(f)) {
      Ddi_BddAndAcc(f,myWindow);
    }
    else if (Ddi_BddIsMono(g)) {
      Ddi_BddAndAcc(g,myWindow);
    }
    Ddi_Free(myWindow);
  }

  if (thresh<0) {

    if (Ddi_BddIsPartConj(g)||Ddi_BddIsPartConj(f)) {
      Ddi_Bdd_t *pp, *ov, *gg;
      Ddi_Varset_t *mySmooth;
      Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity(ddiMgr);

      ov = Ddi_BddDup(f);
      if (Ddi_BddIsPartDisj(ov)) {
      /*Ddi_BddSetMono(ov);*/
      if (care!=NULL) {
          ConstrainOptAcc(ov,care,1);
      }
      }
      Ddi_BddSetPartConj(ov);
      gg = Ddi_BddDup(g);
      if (Ddi_BddIsPartDisj(gg)) {
       /*Ddi_BddSetMono(gg);*/ /*@@@@*/
        if (care!=NULL) {
          ConstrainOptAcc(gg,care,1);
      }
      }
      Ddi_BddSetPartConj(gg);
      pp = Ddi_BddMakePartConjVoid(ddiMgr);
      for (i=0; i<Ddi_BddPartNum(gg); i++) {
        Ddi_Varset_t *supp_gg = Ddi_BddSupp(Ddi_BddPartRead(gg,i));
      int j;
        for (j=0; j<Ddi_BddPartNum(ov); j++) {
          Ddi_Varset_t *supp_ov = Ddi_BddSupp(Ddi_BddPartRead(ov,j));
          int nv = Ddi_VarsetNum(supp_ov);
        Ddi_VarsetIntersectAcc(supp_ov,supp_gg);
        if (3*Ddi_VarsetNum(supp_ov) >= nv) {
               Ddi_Free(supp_ov);
          break;
        }
        else {
             Ddi_Free(supp_ov);
        }
      }
      Ddi_Free(supp_gg);
        if (0||j>Ddi_BddPartNum(ov)) {
        j=Ddi_BddPartNum(ov);
      }
      Ddi_BddPartInsert(ov,j,Ddi_BddPartRead(gg,i));
      }
      Ddi_Free(gg);

      mySmooth = Ddi_VarsetDup(smoothV);
      if (smoothf != NULL) {
        Ddi_VarsetDiffAcc(mySmooth,smoothf);
      }

      for (i=0; i<Ddi_BddPartNum(ov); ) {
        Ddi_Bdd_t *p = Ddi_BddPartRead(ov,i);
        Ddi_Varset_t *supp = Ddi_BddSupp(p);
        Ddi_VarsetIntersectAcc(supp,mySmooth);
        if (!Ddi_VarsetIsVoid(supp)) {
          p = Ddi_BddPartExtract(ov,i);
          if (ddiMgr->settings.part.existOptLevel >= 2) {
            Ddi_Bdd_t *p_ov = Ddi_BddExistOverApprox(p,mySmooth);
          if (ConstrainOptAcc(p,p_ov,0)) {
                   Ddi_BddPartInsert(ov,i,p_ov);
            i++;
          }
          Ddi_Free(p_ov);
        }
            Ddi_BddPartInsertLast(pp,p);
          Ddi_Free(p);
      }
        else {
        i++;
      }
        Ddi_Free(supp);
      }

      if (Ddi_BddPartNum(pp)>0) {
#if 0
        pp = Ddi_BddEvalFree(
          Ddi_BddMultiwayRecursiveAndExist(pp,mySmooth,
          care,-1),pp);
#else
      {
        Ddi_Bdd_t *care2;

          Ddi_BddSetFlattened(ov);
          Ddi_BddSetFlattened(pp);

          if (care == NULL) {
            care2 = NULL;
          }
          else {
            care2 = Ddi_BddDup(care);
          }
        if (0 && ov != NULL) {
          if (care2 == NULL) {
            care2 = Ddi_BddDup(ov);
          }
          else {
            Ddi_BddPartInsertLast(care2,ov);
            Ddi_BddSetFlattened(care2);
          }
        }
          Ddi_BddSetPartConj(pp);
          pp = Ddi_BddEvalFree(
            MultiwayRecursiveAndExistStep(pp,mySmooth,
            care2,NULL,1.5,NULL,-1,-1,metaDecompEna && 0,3),pp);
        Ddi_Free(care2);
        if (ov != NULL) {
          ConstrainOptAcc(pp,ov,0);
          ConstrainOptAcc(ov,pp,0);
        }
        }
#endif
      Ddi_BddSetPartConj(ov);
        if (!Ddi_BddIsOne(pp) || Ddi_BddPartNum(ov)==0) {
          Ddi_BddPartInsert(ov,0,pp);
      }

        if (ddiMgr->settings.part.existOptClustThresh >= 0) {
          Ddi_BddSetClustered(ov,ddiMgr->settings.part.existOptClustThresh);
        Ddi_BddSetFlattened(ov);
        }
        else {
          Ddi_BddSetMono(ov);
        }

      }
      Ddi_Free(pp);
      Ddi_Free(mySmooth);
      r = ov;
    }
    else if (0/*Ddi_BddIsMeta(f)*/) {
      Ddi_Bdd_t *fp, *fp2, *gp, *f2, *g2;
      fp = Ddi_BddExist(f,smoothV);
      gp = Ddi_BddExist(g,smoothV);
      fp2 = Ddi_BddDup(fp);
      Ddi_BddSetPartConj(fp2);
      Ddi_BddPartInsertLast(fp2,gp);
      f2 = Ddi_BddRestrict(f,fp2);
      g2 = Ddi_BddRestrict(g,fp2);
      r = Ddi_BddAndExist(f2,g2,smoothV);
      Ddi_Free(f2);
      Ddi_Free(g2);
      Ddi_BddRestrictAcc(r,fp2);
      Ddi_Free(fp2);
      Ddi_BddAndAcc(r,fp);
      Ddi_BddAndAcc(r,gp);
      Ddi_Free(fp);
      Ddi_Free(gp);
    }
    else {
      if (0&&(Ddi_BddSize(f) > 20000) && (Ddi_BddSize(g) > 20000)) {
        Ddi_Bdd_t *f2 = Ddi_BddExist(f,smoothV);
        Ddi_Bdd_t *g2 = Ddi_BddExist(g,smoothV);
        r = Ddi_BddAnd(f2,g2);
        Ddi_Free(f2);
        Ddi_Free(g2);
        f2 = Ddi_BddDup(f);
        g2 = Ddi_BddDup(g);
        if (care != NULL) {
          ConstrainOptAcc(r,care,1);
          ConstrainOptAcc(f2,care,1);
          ConstrainOptAcc(g2,care,1);
      }
        ConstrainOptAcc(f2,r,1);
        ConstrainOptAcc(g2,r,1);
        Ddi_BddAndExistAcc(f2,g2,smoothV);
        Ddi_Free(g2);
        if (care != NULL) {
          ConstrainOptAcc(f2,care,1);
      }
        Ddi_BddAndAcc(r,f2);
        if (care != NULL) {
          ConstrainOptAcc(r,care,1);
      }
        Ddi_Free(f2);
      }
      else if (Ddi_BddIsMono(f)&&Ddi_BddIsMono(g)) {
        int aborted, again;
        Ddi_Bdd_t *f1 = Ddi_BddDup(f);
        Ddi_Bdd_t *g1 = Ddi_BddDup(g);

        if (aboThresh < 0) {
          aboThresh = ddiMgr->exist.clipThresh;
      }

      do {
        Ddi_Bdd_t *rcare;

	//          Ddi_MgrAbortOnSiftEnableWithThresh(ddiMgr,aboThresh,1);
          r = Ddi_BddAndExist(f1,g1,smoothV);
          aborted = 0&&ddiMgr->abortedOp;
	  //   Ddi_MgrAbortOnSiftDisable(ddiMgr);

          again = 0;
          if (0&&aborted) {
            rcare = Ddi_BddDup(r);
            ConstrainOptAcc(rcare,care,1);
            if (Ddi_BddIsZero(rcare)) {
            Ddi_Bdd_t *carePlus;
            Ddi_Varset_t *smooth = Ddi_BddSupp(care);
            Ddi_Varset_t *keepf = Ddi_BddSupp(f1);
            Ddi_Varset_t *keepg = Ddi_BddSupp(g);
            Ddi_VarsetDiffAcc(smooth,keepf);
            Ddi_VarsetDiffAcc(smooth,keepg);
            Ddi_Free(keepf);
            Ddi_Free(keepg);
              carePlus = Ddi_BddExistOverApprox(care,smooth);
            Ddi_Free(smooth);
            if (Ddi_BddSize(f1) < Ddi_BddSize(g1)) {
            Ddi_BddAndAcc(f1,carePlus);
            }
            else {
            Ddi_BddAndAcc(g1,carePlus);
            }
             Ddi_Free(carePlus);
            again = 1;
            Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
              Pdtutil_VerbLevelNone_c,
              fprintf(stdout, "#")
            );
#if 0
              Ddi_BddNotAcc(r);
            Ddi_BddAndAcc(f1,r);
              if (clipWindow == NULL) {
                clipWindow = Ddi_BddDup(r);
              }
              else {
                Ddi_BddAndAcc(clipWindow,r);
            }
#endif
             Ddi_Free(r);
           }
             Ddi_Free(rcare);
        }

      } while (again);

        if (aborted) {
          /* r is abort frontier */
          Ddi_Bdd_t *r2, *f2;
          f2 = Ddi_BddAnd(f1,r);
          if (care != NULL) {
            ConstrainOptAcc(f2,care,1);
             }
          if (Ddi_BddEqual(f2,f1)) {
            Ddi_Free(f2);
            Ddi_Free(r);
            r = Ddi_BddAndExist(f1,g1,smoothV);
        }
        else {
#if 0
          r2 = Ddi_BddAndExist(f2,g1,smoothV);
          if (orTerm != NULL) {
            Ddi_BddOrAcc(r2,orTerm);
            Ddi_Free(orTerm);
        }
#else
          r2 = AndExistWithAbort(f2,g1,NULL,smoothV,care,thresh,
              orTerm,(int)(1.3*aboThresh),metaDecompEna);
#endif
          orTerm = NULL;
          if (care != NULL) {
            ConstrainOptAcc(r2,care,1);
          }
          Ddi_Free(f2);
          if (ddiMgr->exist.clipLevel == 0) {
            if (orTerm != NULL) {
              Ddi_BddOrAcc(r2,orTerm);
              Ddi_Free(orTerm);
              orTerm = NULL;
              if (care != NULL) {
                ConstrainOptAcc(r2,care,1);
              }
            }
            f2 = Ddi_BddDiff(f1,r);
          Ddi_Free(r);
            if (care != NULL) {
              ConstrainOptAcc(f2,care,1);
                 }
            Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
              Pdtutil_VerbLevelNone_c,
              fprintf(stdout, "?A?")
            );
            r = AndExistWithAbort(f2,g1,NULL,smoothV,
              care,thresh,r2,(int)(1.2*aboThresh),metaDecompEna);
            Ddi_Free(f2);
        }
        else {
            if (ddiMgr->exist.clipWindow == NULL) {
              ddiMgr->exist.clipWindow = Ddi_BddDup(r);
          }
            else if (Ddi_BddIsOne(ddiMgr->exist.clipWindow)) {
              Ddi_BddAndAcc(ddiMgr->exist.clipWindow,r);
          }
            else {
            Ddi_BddSetPartConj(ddiMgr->exist.clipWindow);
              Ddi_BddPartInsertLast(ddiMgr->exist.clipWindow,r);
          }
            Ddi_Free(r);
            r = r2;
            Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
              Pdtutil_VerbLevelNone_c,
              fprintf(stdout, "?C:%d?",Ddi_BddSize(r))
            );
            ddiMgr->exist.clipDone++;
        }
        }
        }
      Ddi_Free(f1);
      Ddi_Free(g1);
      }
      else if (Ddi_BddIsPartDisj(f)) {
        r = Ddi_BddMakePartDisjVoid(ddiMgr);
      for (i=0; i<Ddi_BddPartNum(f); i++) {
          Ddi_Bdd_t *p = AndExistWithAbort(Ddi_BddPartRead(f,i),g,
              smoothf,smoothV,care,thresh,orTerm,aboThresh,metaDecompEna);
          Ddi_BddPartWrite(r,i,p);
        Ddi_Free(p);
        }
      }
      else if (Ddi_BddIsPartDisj(g)) {
         r = AndExistWithAbort(g,f,smoothf,smoothV,
              care,thresh,orTerm,aboThresh,metaDecompEna);
      }
      else if (Ddi_BddIsAig(f)) {
        r = Ddi_BddMakeAig(g);
        Ddi_BddAndAcc(r,f);
      if (Ddi_VarsetNum(smoothV) > 0) {
        int rrl = Ddi_MgrReadAigRedRemLevel(ddiMgr);
        Ddi_MgrSetOption(ddiMgr,Pdt_DdiAigRedRem_c,inum,rrl-1);
          Ddi_AigExistAcc(r,smoothV,care,0,0,-1);
        Ddi_MgrSetOption(ddiMgr,Pdt_DdiAigRedRem_c,inum,rrl);
      }
      }
      else if (Ddi_BddIsMeta(f)) {
      r = Ddi_BddDup(f);
      if (0 && Ddi_VarsetNum(smoothV) > 20 && Ddi_BddSize(f) > 2000) {
        Ddi_Varset_t *smoothV2 = NULL;
        Ddi_Varset_t *smoothV1 = Ddi_VarsetDup(smoothV);
        int i;
        Ddi_Vararray_t *sA = Ddi_VararrayMakeFromVarset(smoothV,1);
        smoothV2 = Ddi_VarsetVoid(ddiMgr);
        for (i=0; i<2*Ddi_VararrayNum(sA)/4; i++) {
          Ddi_Var_t *v_i = Ddi_VararrayRead(sA,i);
          Ddi_VarsetAddAcc(smoothV2,v_i);
        }
        Ddi_Free(sA);
        Ddi_VarsetDiffAcc(smoothV1,smoothV2);
        DdiMetaAndExistAcc(r,g,smoothV1,care);
        Ddi_BddExistAcc(r,smoothV2);
        Ddi_Free(smoothV1);
        Ddi_Free(smoothV2);
      }
      else {
        DdiMetaAndExistAcc(r,g,smoothV,care);
      }
      }
      else if (Ddi_BddIsMeta(g)) {
      r = Ddi_BddDup(g);
        DdiMetaAndExistAcc(r,f,smoothV,care);
      }
      else {
         r = Ddi_BddAndExist(f,g,smoothV);
      }
    }
  }


#if 0
  if (thresh<300000)
    thresh = 300000;
#endif

  else if (Ddi_BddIsMono(f)&&Ddi_BddIsMono(g)) {

    Ddi_MgrAbortOnSiftEnableWithThresh(ddiMgr,(int)(1.1*thresh),1);

    r = Ddi_BddAndExist(f,g,smoothV);

    if ((ddiMgr->abortedOp)||(Ddi_BddSize(r)>thresh)) {
      Ddi_Free(r);
      r = NULL;
    }

    Ddi_MgrAbortOnSiftDisable(ddiMgr);
  }
  else {
    Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity(ddiMgr);

    abortThresh = ddiMgr->settings.part.existClustThresh;
    ddiMgr->settings.part.existClustThresh = thresh;

    r = Ddi_BddAndExist(f,g,smoothV);

    ddiMgr->settings.part.existClustThresh = abortThresh;
    if ((Ddi_BddIsPartConj(r)&&Ddi_BddPartNum(r)>1)
         ||(Ddi_BddSize(r) > thresh)) {
      Ddi_Free(r);
    }
  }

  if (orTerm != NULL) {
    Ddi_BddOrAcc(r,orTerm);
    Ddi_Free(orTerm);
    if (care != NULL) {
      ConstrainOptAcc(r,care,1);
    }
  }

  return (r);
}

#if WEAK_EXIST
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
#endif

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/

static void
TestPart(
  Ddi_Bdd_t  *f
)
{
  int i, best[4], curr[4], size0;
  int bestiE = (-1);
  int bestiA = (-1);
  int bestsizeE = (-1);
  int bestsizeA = (-1);
  Ddi_Varset_t *supp, *smooth;
  Ddi_Var_t *v;
  Ddi_Var_t *bestvE = NULL;
  Ddi_Var_t *bestvA = NULL;
  Ddi_Bdd_t *fe0,*fe1,*fa0,*fa1;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(f);

  Ddi_MgrSiftSuspend(ddiMgr);

  supp = Ddi_BddSupp(f);
  size0 = Ddi_BddSize(f);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "testpart: %d [%d].\n",Ddi_BddSize(f), Ddi_VarsetNum(supp))
  );

  for (i=0,Ddi_VarsetWalkStart(supp);
       !Ddi_VarsetWalkEnd(supp);
       Ddi_VarsetWalkStep(supp),i++) {

    v = Ddi_VarsetWalkCurr(supp);

    smooth = Ddi_VarsetMakeFromVar(v);

    fe1 = Ddi_BddExist(f,smooth);
    if (Ddi_BddSize(fe1)>=Ddi_BddSize(f)) {
       Ddi_Free(fe1);
       fe1 = Ddi_BddMakeConst(ddiMgr,1);
    }
    fe0 = Ddi_BddRestrict(f,fe1);
    fa1 = Ddi_BddForall(f,smooth);
    if (Ddi_BddSize(fa1)>=Ddi_BddSize(f)) {
       Ddi_Free(fa1);
       fa1 = Ddi_BddMakeConst(ddiMgr,0);
    }
    Ddi_BddNotAcc(fa1);
    fa0 = Ddi_BddRestrict(f,fa1);

    curr[0] = Ddi_BddSize(fe0);
    curr[1] = Ddi_BddSize(fe1);
    curr[2] = Ddi_BddSize(fa0);
    curr[3] = Ddi_BddSize(fa1);

    if (i==0 ||
      (curr[0]+curr[1] < bestsizeE)) {
       best[0] = curr[0];
       best[1] = curr[1];
       bestsizeE = curr[0]+curr[1];
       bestiE = i;
       bestvE = v;
    }
    if (i==0 ||
      (curr[2]+curr[3] < bestsizeA)) {
       best[2] = curr[2];
       best[3] = curr[3];
       bestsizeA = curr[2]+curr[3];
       bestiA = i;
       bestvA = v;
    }

    Ddi_Free(fe1);
    Ddi_Free(fe0);
    Ddi_Free(fa1);
    Ddi_Free(fa0);
    Ddi_Free(smooth);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, ".")
    );
  }

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "bestE [%d/%d] %s: E(%d) * %d.\n",
      bestiE, i, Ddi_VarName(bestvE), best[1], best[0]);
    fprintf(stdout, "bestA [%d/%d] %s: A(%d) * %d.\n",
      bestiA, i, Ddi_VarName(bestvA), best[3], best[2])
  );

  Ddi_Free(supp);

  if (bestsizeE < 0.9*size0 || bestsizeA < 0.9*size0) {
    if (bestsizeE < bestsizeA) {
      smooth = Ddi_VarsetMakeFromVar(bestvE);
      fe1 = Ddi_BddExist(f,smooth);
      fe0 = Ddi_BddRestrict(f,fe1);
      Ddi_Free(smooth);
    }
    else {
      smooth = Ddi_VarsetMakeFromVar(bestvA);
      fe1 = Ddi_BddForall(f,smooth);
      Ddi_BddNotAcc(fe1);
      fe0 = Ddi_BddRestrict(f,fe1);
      Ddi_BddNotAcc(fe1);
      Ddi_Free(smooth);
    }
    if (Ddi_BddSize(fe1)<Ddi_BddSize(fe0)) {
       TestPart(fe0);
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, "| %d", Ddi_BddSize(fe1))
       );
    }
    else {
       TestPart(fe1);
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, "| %d", Ddi_BddSize(fe0))
       );
    }
  }
  else {
    if (bestsizeE < bestsizeA) {
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, "| %d # %d", best[1], best[0])
       );
    }
    else {
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, "| %d # %d", best[3], best[2])
       );
    }
  }



  Ddi_MgrSiftResume(ddiMgr);

}



/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMultiwayRecursiveAndExistOverWithFrozenCheck(
  Ddi_Bdd_t *FPart          /* Input partitioned function */,
  Ddi_Varset_t *smoothV     /* Var Set */,
  Ddi_Bdd_t *from          /*from set */,
  Ddi_Vararray_t *psv        /* Array of present state variables */,
  Ddi_Vararray_t *nsv        /* Array of next state variables */,
  Ddi_Bdd_t *constrain      /* factor to be AND-ed with FPart */,
  Ddi_Bdd_t *overApprTerm   /* factor to be replicated with FPart
                               when clustering */,
  int threshold             /* Size threshold for result factor */,
  int fwd
)
{
    Ddi_Vararray_t *eqp, *eqn;
    Ddi_Varset_t *mySmoothV, *allVars, *supp;
    Ddi_Bdd_t *myConstrain=NULL, *res, *FPart1;
    Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(FPart);    /* dd manager */

    int nremoved = 0, initSize = Ddi_BddSize(FPart);
    int i;
    int maxSm = ddiMgr->settings.meta.maxSmoothVars;

    FPart1 = Ddi_BddMakeConst(ddiMgr,1);
    Ddi_BddSetPartConj(FPart1);

    ddiMgr->settings.meta.maxSmoothVars = 0;


    mySmoothV = Ddi_VarsetDup(smoothV);
    eqp = Ddi_VararrayAlloc(ddiMgr,0);
    eqn = Ddi_VararrayAlloc(ddiMgr,0);
#if 1
    allVars = Ddi_BddSupp(FPart);
    supp = Ddi_VarsetMakeFromArray(psv);
    Ddi_VarsetUnionAcc(allVars,supp);
    Ddi_Free(supp);
    supp = Ddi_VarsetMakeFromArray(nsv);
    Ddi_VarsetUnionAcc(allVars,supp);
    Ddi_Free(supp);

    for (i=0; i<Ddi_BddPartNum(FPart); i++) {
      Ddi_BddSuppAttach(Ddi_BddPartRead(FPart,i));
    }

    for (i=0; i<Ddi_VararrayNum(psv); i++) {
      Ddi_Bdd_t *litp = Ddi_BddMakeLiteral(Ddi_VararrayRead(psv,i),1);
      Ddi_Bdd_t *litn = Ddi_BddMakeLiteral(Ddi_VararrayRead(nsv,i),1);
      Ddi_Bdd_t *diff = Ddi_BddXor(litp,litn);
      int j, eq=0;

      Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity (ddiMgr);
      Ddi_Free(litp);
      Ddi_Free(litn);
      for (j=Ddi_BddPartNum(FPart)-1; j>1; j--) {
      Ddi_Bdd_t *p_j = Ddi_BddPartRead(FPart,j);
        if (Ddi_VarInVarset(Ddi_BddSuppRead(p_j),Ddi_VararrayRead(nsv,i))) {
          Ddi_Bdd_t *aux = Ddi_BddAnd(Ddi_BddPartRead(FPart,j),diff);
          if (!Ddi_BddIsZero(aux)) {
          DdiBddMetaAlign(from);
        }
          if (Ddi_BddIsZero(aux)) {
          if (fwd) {
            Ddi_Varset_t *y_j = Ddi_VarsetDiff(Ddi_BddSuppRead(p_j),smoothV);
          if (Ddi_VarsetNum(y_j)==1) {
            Pdtutil_Assert(Ddi_VarInVarset(y_j,Ddi_VararrayRead(nsv,i)),
                       "unmatched NS var");
            Ddi_BddPartRemove(FPart,j);
               nremoved++;
          }
          else {
              Ddi_Varset_t *y = Ddi_VarsetMakeFromVar(Ddi_VararrayRead(nsv,i));
            Ddi_BddExistAcc(Ddi_BddPartRead(FPart,j),y);
            Ddi_BddSuppDetach(Ddi_BddPartRead(FPart,j));
            Ddi_BddSuppAttach(Ddi_BddPartRead(FPart,j));
            Ddi_Free(y);
          }
            Ddi_Free(y_j);
          }
            Ddi_Free(aux);
          eq=1;
          break;
          }
        Ddi_Free(aux);
      }
      }

      if (eq) {
        Ddi_VararrayInsertLast(eqp,Ddi_VararrayRead(psv,i));
        Ddi_VararrayInsertLast(eqn,Ddi_VararrayRead(nsv,i));
      }
      Ddi_Free(diff);
    }
    Ddi_Free(allVars);

    for (i=0; i<Ddi_BddPartNum(FPart); i++) {
      Ddi_BddSuppDetach(Ddi_BddPartRead(FPart,i));
    }

#endif

    if (constrain != NULL) {
      myConstrain = Ddi_BddDup(constrain);
    }

    if (Ddi_VararrayNum(eqp) > 0) {
      Ddi_Varset_t *noSmooth = Ddi_VarsetMakeFromArray(eqp);

      Ddi_VarsetDiffAcc(mySmoothV,noSmooth);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Removed %d/%d partitions.\n",
          nremoved,nremoved+Ddi_BddPartNum(FPart));
        fprintf(stdout, "%d Unchanging State Variables Found (%d smoothing vars).\n",
        Ddi_VararrayNum(eqp), Ddi_VarsetNum(mySmoothV))
      );
      if (!fwd) {
        Ddi_BddSubstVarsAcc(FPart,eqn,eqp);
      }
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Size reduction %d -> %d.\n",
          initSize, Ddi_BddSize(FPart))
      );
      Ddi_Free(noSmooth);
      initSize = Ddi_BddSize(FPart);

#if 0
#if 0
      for (i=0; i<Ddi_BddPartNum(FPart); i++) {
        over = Ddi_BddExist(Ddi_BddPartRead(FPart,i),mySmoothV);
      if (!Ddi_BddIsOne(over)) {
          Ddi_BddRestrictAcc(Ddi_BddPartRead(FPart,i),over);
          Ddi_BddPartInsertLast(FPart1,over);
#if 0
          if (myConstrain!=NULL) {
          Ddi_BddSetPartConj(myConstrain);
            Ddi_BddPartInsertLast(myConstrain,over);
          }
#endif
      }
        Ddi_Free(over);
      }
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Over reduction %d -> %d.\n",
          initSize, Ddi_BddSize(FPart))
     );
#else
      over = Ddi_BddMultiwayRecursiveAndExist(FPart,smoothV,myConstrain,-1);
      Ddi_BddSetPartConj(over);
      if (Ddi_BddIsMeta(over)) {
        Ddi_BddSetPartConjFromMeta(over);
      }
      else if (Ddi_BddIsAig(over)) {
      Pdtutil_Assert(0,"conj from aig")
      }
      for (i=0; i<Ddi_BddPartNum(over); i++) {
        Ddi_BddPartInsertLast(FPart1,Ddi_BddPartRead(over,i));
      }
      Ddi_BddRestrictAcc(FPart,over);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Over reduction %d -> %d.\n",
          initSize, Ddi_BddSize(FPart))
      );
      if (myConstrain!=NULL) {
        Ddi_BddSetPartConj(myConstrain);
        Ddi_BddPartInsertLast(myConstrain,over);
      }
      Ddi_Free(over);
      {
        Ddi_Bdd_t *p0 = Ddi_BddPartExtract(FPart,1);
        Ddi_Varset_t *suppFrom = Ddi_BddSupp(from);
        Ddi_Varset_t *mySuppFrom;
      Ddi_VarsetIntersectAcc(suppFrom,smoothV);
      mySuppFrom = Ddi_VarsetIntersect(suppFrom,mySmoothV);
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "SM from:%d -> %d.\n", Ddi_VarsetNum(suppFrom),
            Ddi_VarsetNum(mySuppFrom))
        );
        supp = Ddi_BddSupp(FPart);
      Ddi_VarsetDiffAcc(suppFrom,supp);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Only from: %d ", Ddi_VarsetNum(suppFrom))
      );
      Ddi_VarsetDiffAcc(suppFrom,mySuppFrom);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "(Keep %d).\n", Ddi_VarsetNum(suppFrom))
      );
      Ddi_VarsetIntersectAcc(supp,smoothV);
      Ddi_VarsetIntersectAcc(suppFrom,supp);
      Ddi_VarsetIntersectAcc(mySuppFrom,supp);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "True smooth: %d", Ddi_VarsetNum(supp))
      );
      Ddi_VarsetIntersectAcc(supp,mySmoothV);
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "(%d) - F:%d - myF:%d.\n", Ddi_VarsetNum(supp),
            Ddi_VarsetNum(suppFrom),Ddi_VarsetNum(mySuppFrom))
        );
      Ddi_Free(mySuppFrom);
      Ddi_Free(suppFrom);
      Ddi_Free(supp);
      Ddi_BddPartInsert(FPart,1,p0);
      Ddi_Free(p0);
      }
#endif
#endif
    }
    Ddi_Free(eqp);
    Ddi_Free(eqn);

    ddiMgr->settings.meta.maxSmoothVars = maxSm;

    res = Ddi_BddMultiwayRecursiveAndExist(FPart,mySmoothV,
       myConstrain,-1);

    if (Ddi_BddIsMeta(res)||Ddi_BddIsAig(res)) {
      if (Ddi_BddPartNum(FPart1) > 0) {
        Ddi_BddAndAcc(res,FPart1);
      }
    }
    else {
      Ddi_BddSetPartConj(res);
      for (i=0; i<Ddi_BddPartNum(FPart1); i++) {
        Ddi_Bdd_t *p = Ddi_BddPartRead(FPart1,i);
        Ddi_BddPartInsertLast(res,p);
      }
    }
    Ddi_Free(FPart1);

    Ddi_Free(myConstrain);
    Ddi_Free(mySmoothV);


    return(res);

}




/**Function********************************************************************
  Synopsis    [Dynamic abstraction based on high density.]
  Description [Dynamic abstraction based on high density. Abstracted
               variables are selected in order to minimixe the error
               w.r.t. the original BDD. In other words, the goal is to
               minimize (exist) or maximize (forall) the number of minterms.]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddDynamicAbstract(
  Ddi_Bdd_t *f,
  int maxSize,
  int maxAbstract,
  int exist_or_forall
)
{
  Ddi_Bdd_t *newF = Ddi_BddDup(f);
  Ddi_BddDynamicAbstract(newF,maxSize,maxAbstract,exist_or_forall);
  return(newF);
}

/**Function********************************************************************
  Synopsis    [Dynamic abstraction based on high density.]
  Description [Dynamic abstraction based on high density. Abstracted
               variables are selected in order to minimixe the error
               w.r.t. the original BDD. In other words, the goal is to
               minimize (over_approx) or maximize (under_approx) the
               number of minterms.]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddDynamicAbstractAcc(
  Ddi_Bdd_t *f,
  int maxSize,                /* target BDD size */
  int maxAbstract             /* max num of abstracted vars */,
  int over_under_approx       /* 1: over - 0: under approx */
)
{
  Ddi_Varset_t *supp;
  Ddi_Bdd_t *newF;
  int i,j,nv,bestI,initSize;
  Ddi_Vararray_t *va;
  double refCnt,w,bestW;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(f);    /* dd manager */

  if (maxSize <= 0 && maxAbstract <= 0) {
    return (f);
  }

  supp = Ddi_BddSupp(f);
  nv = Ddi_VarsetNum(supp);
  refCnt = Ddi_BddCountMinterm(f,nv);

  newF = f;
  va = Ddi_VararrayMakeFromVarset(supp,1);
  Ddi_Free(supp);

  j = 0;
  initSize = Ddi_BddSize(f);

  do {
    Ddi_Var_t *v;
    Ddi_Varset_t *vs;
    Ddi_Bdd_t *f0;
    Ddi_Bdd_t *f1;
    Ddi_Bdd_t *f01;
    bestI = -1;
    bestW = over_under_approx ? -1.0 : 0.0;
    for (i=0; i<Ddi_VararrayNum(va);i++) {
      v   = Ddi_VararrayRead(va,i);
      f0  = Ddi_BddCofactor(newF,v,0);
      f1  = Ddi_BddCofactor(newF,v,1);
      if (over_under_approx) {
        f01 = Ddi_BddOr(f0,f1);
      }
      else {
        f01 = Ddi_BddAnd(f0,f1);
      }
      Ddi_Free(f0);
      Ddi_Free(f1);
      w = Ddi_BddCountMinterm(f01,nv)/refCnt;
      if (bestI < 0 || ((Ddi_BddSize(f01) < Ddi_BddSize(newF)) &&
          (over_under_approx ? w < bestW : w > bestW))) {
      bestI = i;
      bestW = w;
      }
      Ddi_Free(f01);
    }
    Pdtutil_Assert (bestI >= 0, "bestI not found");
    v   = Ddi_VararrayRead(va,bestI);
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMax_c,
      fprintf(stdout, "best abstract var found: %s - cost: %f.\n",
        Ddi_VarName(v), bestW)
    );
    vs = Ddi_VarsetMakeFromVar(v);
    if (over_under_approx) {
      Ddi_BddExistAcc(newF,vs);
    }
    else {
      Ddi_BddForallAcc(newF,vs);
    }
    Ddi_Free(vs);
    Ddi_VararrayRemove(va,bestI);
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMax_c,
      fprintf(stdout, "new size: %d.\n", Ddi_BddSize(newF))
    );
    j++;

  } while ((maxSize<=0 || maxSize < Ddi_BddSize(newF)) &&
           (maxAbstract<=0 || j<maxAbstract));

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMax_c,
    fprintf(stdout, "dynamic Abstract (%sapprox): %d(nv:%d) -> %d(nv:%d).\n",
       over_under_approx ? "over" : "under",
       initSize, nv, Ddi_BddSize(newF), nv-j);
    w = Ddi_BddCountMinterm(newF,nv);
    fprintf(stdout, "dynamic Abstract minterms: %g -> %g (%f).\n",
       refCnt, w, w/refCnt)
  );

  Ddi_Free(va);

  return(newF);

}

/**Function********************************************************************

  Synopsis    [Internal image computation function.]

  Description [Internal image computation function.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Ddi_ImgConjPartTr (
  Ddi_Bdd_t *TR              /* Partitioned TR */,
  Ddi_Bdd_t *from            /* domain set */,
  Ddi_Bdd_t *constrain       /* range constrain */,
  Ddi_Varset_t *smoothV      /* Variables to be abstracted */
  )
{
  Ddi_Bdd_t *to;
  Ddi_Mgr_t *ddTR = Ddi_ReadMgr(TR);     /* TR DD manager */
  Ddi_Mgr_t *ddR = Ddi_ReadMgr(from);  /* Reached DD manager */
  Ddi_Varset_t *mySmooth, *suppProd, *auxvSet = NULL;
  int i, nLoops;

  Pdtutil_Assert (Ddi_BddIsPartConj(TR)||Ddi_BddIsMono(TR),
    "Unexpected TR decomposition in image computation");

  from = Ddi_BddCopy (ddTR, from);

  smoothV = Ddi_VarsetDup(smoothV);

  {
    Ddi_Bdd_t *prod = Ddi_BddDup(TR);
    Ddi_Bdd_t *prod1 = Ddi_BddMakePartConjVoid(ddTR);
    Ddi_Bdd_t *clk_part=NULL;
    int np, again;

    Ddi_BddSetPartConj(prod);
    if (Ddi_BddSize(Ddi_BddPartRead(prod,0)) < 4) {
      clk_part = Ddi_BddPartExtract(prod,0);
    }

    nLoops = 0;
    do {

    if (Ddi_BddPartNum(prod)>1) {
      Ddi_Varset_t *suppFrom = Ddi_BddSupp(from);
      Ddi_Varset_t *clk_supp=NULL;
      Ddi_Bdd_t *prodNew = Ddi_BddMakePartConjVoid(ddTR);

      Ddi_VarsetIntersectAcc(suppFrom,smoothV);
      Ddi_BddSetPartConj(prod);
      if (clk_part != NULL) {
        clk_supp = Ddi_BddSupp(clk_part);
      }
      Ddi_Free(prodNew);
      prodNew = Ddi_BddDup(prod);
      Ddi_Free(prod);
      prod = Ddi_BddDup(prodNew);

      Ddi_Free(prodNew);

      Pdtutil_VerbosityMgr(ddR, Pdtutil_VerbLevelDevMax_c,
        fprintf(stdout, "FROM: %d (",Ddi_BddSize(from))
      );

      Ddi_Free(prodNew);
      prodNew = Ddi_BddDup(prod);

      Ddi_Free(prod);
      prod = Ddi_BddDup(prodNew);

      Ddi_Free(prodNew);

      Pdtutil_VerbosityMgr(ddR, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "FROM: %d (",Ddi_BddSize(from))
      );

      Ddi_Free(suppFrom);
      Ddi_Free(clk_supp);
      Pdtutil_VerbosityMgr(ddR, Pdtutil_VerbLevelDevMin_c,
       fprintf(stdout, ") -> %d.\n", Ddi_BddSize(from))
      );
    }

    if (Ddi_BddIsPartConj(from)) {
      Ddi_Bdd_t *f2 = Ddi_BddDup(from);
      Ddi_Varset_t *suppTR = Ddi_BddSupp(TR);
      int i;
      for (i=Ddi_BddPartNum(f2)-1; i>=0; i--) {
        Ddi_Bdd_t *p = Ddi_BddPartRead(f2,i);
        Ddi_Varset_t *supp = Ddi_BddSupp(p);
        Ddi_VarsetIntersectAcc(supp,suppTR);
        if (Ddi_VarsetIsVoid(supp)) {
          p = Ddi_BddPartExtract(f2,i);
          Ddi_Free(p);
      }
        Ddi_Free(supp);
      }
      Ddi_Free(suppTR);
      Ddi_BddPartInsert(prod,0,f2);
      Ddi_Free(f2);
    }
    else {
      Ddi_BddPartInsert(prod,0,from);
    }

    if (clk_part != NULL) {
      Ddi_BddPartInsert(prod,0,clk_part);
      Ddi_Free(clk_part);
    }

    if (Ddi_BddPartNum(prod1) > 0) {
      suppProd = Ddi_BddSupp(prod1);
      mySmooth = Ddi_VarsetDiff(smoothV,suppProd);
      Ddi_Free(suppProd);
    }
    else {
      mySmooth = Ddi_VarsetDup(smoothV);
    }


#if 1
      to = Ddi_BddMultiwayRecursiveAndExist(prod,mySmooth,
         constrain,-1);
#else
      to = Part_BddMultiwayLinearAndExist(prod,mySmooth,-1);
#endif

    Ddi_Free(mySmooth);
    Ddi_Free(prod);

    again = 0;
    if (!Ddi_BddIsZero(to) && Ddi_BddPartNum(prod1) > 0) {
      int nSuppSmoothV;
      Ddi_Varset_t *suppTo = Ddi_BddSupp(to);
      suppProd = Ddi_BddSupp(prod1);
      Ddi_VarsetIntersectAcc(suppTo,smoothV);
      nSuppSmoothV = Ddi_VarsetNum(suppTo);
      Ddi_VarsetIntersectAcc(suppTo,suppProd);
      Ddi_Free(suppProd);
      if (!Ddi_VarsetIsVoid(suppTo)) {
      again = 1;
      prod = prod1;
        Ddi_Free(from);
      from = to;
        prod1 = Ddi_BddMakePartConjVoid(ddTR);
        Ddi_BddSetPartConj(prod);
      }
      else {
      if (nSuppSmoothV > 0) {
        Ddi_BddExistAcc(to,smoothV);
      }
      }
      Ddi_Free(suppTo);
    }

    } while (again /* && (nLoops++ < 1)*/);

    if (Ddi_BddIsZero(to)) {
      Ddi_Free(prod1);
    }
    else
    {
      Ddi_Bdd_t *myTo=NULL;
      Ddi_BddSetPartConj(prod1);
      suppProd = Ddi_BddSupp(prod1);
      mySmooth = Ddi_VarsetIntersect(suppProd,smoothV);
      Ddi_VarsetDiffAcc(suppProd,smoothV);
      if (!Ddi_VarsetIsVoid(suppProd) && (Ddi_VarsetNum(mySmooth) > 0)) {
        np = Ddi_BddPartNum(prod1);
        Pdtutil_VerbosityMgr(ddR, Pdtutil_VerbLevelDevMin_c,
          fprintf(stdout, "TR IMG SM1 (%d) extra part: %d.\n",
            Ddi_VarsetNum(mySmooth), np)
        );
        myTo = Ddi_BddMultiwayRecursiveAndExist(prod1,mySmooth,
          constrain,-1);
          Ddi_BddSetPartConj(myTo);
      }
      else if (!Ddi_VarsetIsVoid(suppProd)) {
      /* no vars to smooth */
      myTo = Ddi_BddDup(prod1);
      }
      Ddi_Free(prod1);
      Ddi_Free(suppProd);
      if (myTo != NULL) {
        for (i=0; i<Ddi_BddPartNum(myTo); i++) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(myTo,i);
          Ddi_BddSetPartConj(to);
          Ddi_BddPartInsertLast(to,p);
        }
      }
      Ddi_Free(myTo);
      Ddi_Free(mySmooth);
    }

  }
#if DEVEL_OPT_ANDEXIST
  if (Ddi_BddIsMono(to)) {
    Pdtutil_VerbosityMgr(ddR, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "IMG0 = %d ->", Ddi_BddSize(to))
    );
    Ddi_BddExistAcc(to,smoothV);
    Pdtutil_VerbosityMgr (ddR, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "IMG1 = %d.\n", Ddi_BddSize(to))
    );
  }
#endif
  Ddi_BddExistAcc(to,smoothV);

  /* transfer to reached manager */
  to = Ddi_BddEvalFree(Ddi_BddCopy(ddR,to),to);

  if (auxvSet != NULL) {
    Ddi_BddExistAcc(to,auxvSet);
    Ddi_Free(auxvSet);
  }

  Ddi_Free (from);
  Ddi_Free(smoothV);

  return (to);
}

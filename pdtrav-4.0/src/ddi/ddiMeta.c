/**CFile***********************************************************************

  FileName    [ddiMeta.c]

  PackageName [ddi]

  Synopsis    [Functions working on Meta BDDs]

  Description [Functions working on Meta BDDs]

  SeeAlso   []  

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
  
  Revision  []

******************************************************************************/

#include "ddiInt.h"
#include "part.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define META_REVERSE 0
#define HEURISTIC1 0
#define HEURISTIC2 1
#define HEURISTIC3 1
#define META_AE_META_METHOD1 1
#define META_OVER 0
#define META_CONST 0

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/**Enum************************************************************************
  Synopsis    [Meta convert direction.]
******************************************************************************/

typedef enum {
  Meta2Cpart_c,
  Meta2Bdd_c,
  Meta2Aig_c,
  Bdd2Meta_c
}
Meta_Convert_e;

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

static int enableUpdate = 1;
static int enableSkipLayer = 1;
static int strongSimplify = 1;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#define META_MAX(a,b) (((a)<(b))?(b):(a))
#define META_MIN(a,b) (((a)<(b))?(a):(b))


#define LOG_BDD(f,msg) \
\
    {\
      Ddi_Varset_t *supp;\
\
      printf(msg);\
      printf("\n");\
      if (f==NULL) \
        printf("NULL!\n");\
      else if (Ddi_BddIsZero(f)) \
        printf("ZERO!\n");\
      else if (Ddi_BddIsOne(f)) \
        printf("ONE!\n");\
      else {\
        printf("Size: %d\nSupport\n",Ddi_BddSize(f));\
        /* print support variables */\
        supp = Ddi_BddSupp(f);\
        Ddi_VarsetPrint(supp, 0, NULL, stdout);\
        Ddi_Free(supp);\
      }\
    }

#define LOG_BDD_FULL(f,msg) \
\
    {\
      Ddi_Varset_t *supp;\
\
      printf(msg);\
      printf("\n");\
      if (f==NULL) \
        printf("NULL!\n");\
      else if (Ddi_BddIsZero(f)) \
        printf("ZERO!\n");\
      else if (Ddi_BddIsOne(f)) \
        printf("ONE!\n");\
      else {\
        printf("Size: %d\nSupport\n",Ddi_BddSize(f));\
        /* print support variables */\
        supp = Ddi_BddSupp(f);\
        Ddi_VarsetPrint(supp, 0, NULL, stdout);\
        Ddi_BddPrintCubes(f, supp, 100, 0, NULL, stdout);\
        Ddi_Free(supp);\
      }\
    }

#define CHECK_META(f_meta,f_bdd) {\
  Ddi_Bdd_t *tmp;\
  tmp = Ddi_BddMakeFromMeta(f_meta);\
  Pdtutil_Assert(Ddi_BddEqual(tmp,f_bdd),\
    "Re-converted Meta BDD does not match original one");\
  Ddi_Free(tmp);\
}
  
/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static Ddi_Generic_t * MetaConvert(Ddi_Generic_t *f, Meta_Convert_e sel);
static void MetaConvertBdd(Ddi_Generic_t *f, Meta_Convert_e sel);
static void MetaFromMono(Ddi_Bdd_t *f);
static void MetaReduce(Ddi_Bdd_t *f, int init);
static void MetaSimplify(Ddi_Bdd_t *f, int init, int end);
//static void MetaSimplifyUp(Ddi_Bdd_t *f);
static void MetaSetConj(Ddi_Bdd_t *f);
static int MetaIsConj(Ddi_Bdd_t *f);
//static void MetaAlignLayers(Ddi_Bdd_t *f);
static void MetaAndAcc(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
static void MetaToAig(Ddi_Bdd_t *f);
static void MetaToMono(Ddi_Bdd_t *f);
static void MetaToCpart(Ddi_Bdd_t *f);
static int MetaUpdate(Ddi_Mgr_t *ddm, int enableInit);
//static Ddi_Bdd_t * MetaLinearAndExistAcc(Ddi_Bdd_t *fMeta, Ddi_Bdd_t *gBdd, Ddi_Varset_t *smooth);
static Ddi_Bdd_t * MetaConstrainOpt(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
static Ddi_Bdd_t * MetaConstrainOptIntern(Ddi_Bdd_t *f, Ddi_Bdd_t *g, int sizef, int sizeg);
static Ddi_Bdd_t * MetaForall(Ddi_Bdd_t *f, Ddi_Varset_t *smooth);
static void MetaAndExistAfterUpdate(Ddi_Bdd_t *fMeta, Ddi_Bdd_t *ff, Ddi_Bdd_t *gg, Ddi_Varset_t *smooth, Ddi_Bdd_t *care, int curr_layer, int n_layers, Pdtutil_VerbLevel_e verbosity);
static Ddi_Bdd_t * MetaOrAcc(Ddi_Bdd_t *fMeta, Ddi_Bdd_t *gMeta, int disj);
//static Ddi_Bdd_t * MetaOrAccOld(Ddi_Bdd_t *fMeta, Ddi_Bdd_t *gMeta, int disj);
static int MetaAlign(Ddi_Bdd_t *f, Ddi_Bdd_t *care);

/**AutomaticEnd***************************************************************/


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
void 
logBddSupp (
  Ddi_Bdd_t *f
)
{
  Ddi_Varset_t *s = Ddi_BddSupp(f);
  Ddi_VarsetPrint(s,0,0,stdout);
  Ddi_Free(s);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
void 
logBddarraySupp (
  Ddi_Bddarray_t *fA
)
{
  Ddi_Varset_t *s = Ddi_BddarraySupp(fA);
  Ddi_VarsetPrint(s,0,0,stdout);
  Ddi_Free(s);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
void 
logBdd (
  Ddi_Bdd_t *f
)
{
  if (Ddi_BddIsAig(f)) {
    Ddi_Bdd_t *f1 = Ddi_BddMakeMono(f);
    logBdd (f1);
    Ddi_Free(f1);
  }
  else 
    LOG_BDD(f,"BDD:");
}
void 
logBddFull (
  Ddi_Bdd_t *f
)
{
  if (Ddi_BddIsAig(f)) {
    Ddi_Bdd_t *f1 = Ddi_BddMakeMono(f);
    logBddFull (f1);
    Ddi_Free(f1);
  }
  else 
    LOG_BDD_FULL(f,"BDD:");
}
void 
logSet (
  Ddi_Varset_t *v
)
{
  Ddi_VarsetPrint(v, 0, NULL, stdout);
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************
  Synopsis    [Free meta struct and pointed arrays: one, zero and dc.]
  Description [Free meta struct and pointed arrays: one, zero and dc.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiLogBdd (
  Ddi_Bdd_t *p,
  int s
)
{
  if (s==0) logBddFull(p);
}

/**Function********************************************************************
  Synopsis    [Free meta struct and pointed arrays: one, zero and dc.]
  Description [Free meta struct and pointed arrays: one, zero and dc.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiMetaFree (
  Ddi_Meta_t *m
)
{
  Ddi_Unlock(m->one);
  Ddi_Unlock(m->zero);
  Ddi_Unlock(m->dc);
  Ddi_Free(m->one);
  Ddi_Free(m->zero);
  Ddi_Free(m->dc);
  Pdtutil_Free(m);
}

/**Function********************************************************************
  Synopsis    [Duplicate meta struct]
  Description [Duplicate meta struct]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Meta_t *
DdiMetaDup (
  Ddi_Meta_t *m
)
{
  Ddi_Meta_t *newm;

  newm = Pdtutil_Alloc(Ddi_Meta_t,1);
  newm->one = Ddi_BddarrayDup(m->one);
  newm->zero = Ddi_BddarrayDup(m->zero);
  newm->dc = Ddi_BddarrayDup(m->dc);
  newm->metaId = m->metaId;
  Ddi_Lock(newm->one);
  Ddi_Lock(newm->zero);
  Ddi_Lock(newm->dc);

  return(newm);
}

/**Function********************************************************************
  Synopsis    [Return support of Meta BDD]
  Description [Return support of Meta BDD]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Varset_t *
DdiMetaSupp (
  Ddi_Meta_t *m
)
{
  Ddi_Varset_t *supp, *aux;

  supp = Ddi_BddarraySupp(m->one);
  aux = Ddi_BddarraySupp(m->zero);
  Ddi_VarsetUnionAcc(supp,aux);
  Ddi_Free(aux);
  aux = Ddi_BddarraySupp(m->dc);
  Ddi_VarsetUnionAcc(supp,aux);
  Ddi_Free(aux);
  return(supp);

}


/**Function********************************************************************
  Synopsis    [Complement (not) meta BDD. Result accumulated]
  Description [Complement (not) meta BDD. Result accumulated. Operation is
    achieved by swapping the one/zero arrays]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiMetaDoCompl (
  Ddi_Meta_t *m
)
{
  Ddi_Bddarray_t *tmp;

  tmp = m->one;
  m->one = m->zero;
  m->zero = tmp;
}


/**Function********************************************************************
  Synopsis    [Return true if meta BDD is constant]
  Description [Return true if meta BDD is constant. Phase = 1 for one, 0 for
    zero constant.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
DdiMetaIsConst (
  Ddi_Bdd_t *fMeta,
  int phase
)
{
  Ddi_Bddarray_t *tmp;
  int i;
  Ddi_Mgr_t *ddm =  Ddi_ReadMgr(fMeta);

  if (phase == 0) {
#if 0
    Ddi_Bdd_t *mono = Ddi_BddMakeMono(fMeta);
    int iszero = Ddi_BddIsZero(mono);
    Ddi_Free(mono);
    return(iszero);
#endif
    tmp = fMeta->data.meta->one;
  }
  else {
#if 0
    Ddi_Bdd_t *mono = Ddi_BddMakeMono(fMeta);
    int isone = Ddi_BddIsOne(mono);
    Ddi_Free(mono);
    return(isone);
#endif
    tmp = fMeta->data.meta->zero;
  }

  for (i=0; i<ddm->meta.groupNum; i++) {
    if (!Ddi_BddIsZero(Ddi_BddarrayRead(tmp,i))) {
      return(0);
    }
  }
  return(1);
}

/**Function********************************************************************
  Synopsis    [Operate And-Exist between Meta BDD and monolithic BDD]
  Description [Operate And-Exist between Meta BDD and monolithic BDD]
  SideEffects []
  SeeAlso     []
******************************************************************************/

Ddi_Bdd_t *
DdiMetaAndExistAcc (
  Ddi_Bdd_t *fMeta,
  Ddi_Bdd_t *gBdd,
  Ddi_Varset_t *smooth,
  Ddi_Bdd_t *care
)
{
  int i, j, ng, peak=0, size, initSize, preFetched = 0, gMono, forceSmoothUp;
  Ddi_Bdd_t *p, *dc, *gg, *ff, *tmp, *up, *myCare;
  Ddi_Varset_t *metaGrp, *smooth_i, *downTot, *smooth_down, 
    *smooth_down_up, *aux, *range;
  Ddi_Varsetarray_t *downGroups;
  Ddi_Mgr_t *ddm; 
  Ddi_Meta_t *m;
  int extRef, nodeId, lastSkip=-1;
  Pdtutil_VerbLevel_e verbosity;

  Pdtutil_Assert(fMeta->common.code == Ddi_Bdd_Meta_c,
    "Operation requires Meta BDD!");
#if 0
  Pdtutil_Assert((gBdd == NULL)||(gBdd->common.code == Ddi_Bdd_Mono_c)
    ||(gBdd->common.code == Ddi_Bdd_Part_Conj_c),
    "Operation requires monolithic or cpart BDD!");
#endif
  ddm = Ddi_ReadMgr(fMeta);
  verbosity = Ddi_MgrReadVerbosity(ddm);

  if (care != NULL) {
#if 0
    Ddi_Varset_t *supp = Ddi_BddSupp(fMeta);
    Ddi_Varset_t *careSupp = Ddi_BddSupp(care);
    if (gBdd != NULL) {
      Ddi_Varset_t *gSupp = Ddi_BddSupp(gBdd);
      Ddi_VarsetUnionAcc(supp,gSupp);
      Ddi_Free(gSupp);
    }
    Ddi_VarsetDiffAcc(careSupp,supp);

    Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,Pdtutil_VerbLevelUsrMin_c);
    myCare = Ddi_BddExistOverApprox(care,careSupp);
    Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,verbosity);

    Ddi_Free(careSupp);
    Ddi_Free(supp);
#else
    myCare = Ddi_BddDup(care);
#endif
  }
  else {
    myCare = NULL;
  }

  if (smooth == NULL) {
    smooth = Ddi_VarsetVoid(ddm);
  }
  else {
    Ddi_Varset_t *mySmooth, *supp;
    smooth = Ddi_VarsetDup(smooth);
    supp = Ddi_BddSupp(fMeta);
    if (gBdd != NULL) {
      mySmooth = Ddi_BddSupp(gBdd);
      Ddi_VarsetUnionAcc(supp,mySmooth);
      Ddi_Free(mySmooth);
    }
    Ddi_VarsetIntersectAcc(smooth,supp);
    Ddi_Free(supp);

    if ((ddm->settings.meta.maxSmoothVars > 0) &&
        (ddm->settings.meta.maxSmoothVars < Ddi_VarsetNum(smooth)) &&
        (Ddi_BddSize(fMeta) > 1000)) {
      int saveN = ddm->settings.meta.maxSmoothVars;
      Ddi_Varsetarray_t *smArray;
      Ddi_Bdd_t *myG=NULL;

      ddm->settings.meta.maxSmoothVars = 0;
      smArray = Ddi_VarsetarrayAlloc(ddm,0);
      if (gBdd != NULL) {
        myG = Ddi_BddDup(gBdd);
      }
      mySmooth = Ddi_VarsetVoid(ddm);
      if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
        fprintf (stdout, "[[$Layered-AE-M:");
        fflush(stdout);
      }

#if 0
      for (i=ddm->meta.groupNum-1; i>=0; i--) {
#else
      for (i=0; i<ddm->meta.groupNum; i++) {
#endif
        Ddi_Varset_t *localSmooth = Ddi_VarsetIntersect(
          Ddi_VarsetarrayRead(ddm->meta.groups,i),smooth);
	if ((Ddi_VarsetNum(mySmooth) + Ddi_VarsetNum(localSmooth)) <
            saveN) {
	  Ddi_VarsetUnionAcc(mySmooth,localSmooth);
  	  Ddi_Free(localSmooth);
	}
	else {
          if (Ddi_VarsetNum(mySmooth) > 0) {
            Ddi_VarsetarrayInsertLast(smArray,mySmooth);
          }
	  Ddi_Free(mySmooth);
          mySmooth = localSmooth;
	}
      }
      if (Ddi_VarsetNum(mySmooth) > 0) {
        Ddi_VarsetarrayInsertLast(smArray,mySmooth);
      }
      Ddi_Free(mySmooth);

      for (i=0; i<Ddi_VarsetarrayNum(smArray); i++) {
	mySmooth = Ddi_VarsetarrayRead(smArray,i);
        DdiMetaAndExistAcc (fMeta,myG,mySmooth,myCare);
        Ddi_Free(myG);
	myG = NULL;
      }
      Ddi_Free(smArray);

      if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
        fprintf (stdout, "]]");
        fflush(stdout);
      }

      Pdtutil_Assert(myG==NULL,"NULL G expected");
      ddm->settings.meta.maxSmoothVars = saveN;
      Ddi_Free(smooth);
      Ddi_Free(myCare);
      return(fMeta);
    }
  }

  if ((gBdd!=NULL)&&(!Ddi_VarsetIsVoid(smooth))
       &&(fMeta->data.meta->metaId != ddm->meta.id)) {
    fMeta->data.meta->metaId = ddm->meta.id;
    DdiMetaAndExistAcc (fMeta,NULL,NULL,myCare);
  }

  initSize = Ddi_BddSize(fMeta);

#if 0
  if ((gBdd!=NULL)&&(Ddi_BddIsPartConj(gBdd))) {
    Ddi_Free(myCare);
    return (MetaLinearAndExistAcc (fMeta,gBdd,smooth));
  }
#endif

  extRef = Ddi_MgrReadExtRef(ddm);
  nodeId = Ddi_MgrReadCurrNodeId(ddm);
  ng = ddm->meta.groupNum;

  if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
    fprintf (stdout, "$AE-M:");
    fflush(stdout);
  }

  if (gBdd == NULL) {
    gg = Ddi_BddMakeConst(ddm,1);
  }
  else {
    if (Ddi_BddIsMeta(gBdd)) {
      if (gBdd->data.meta->metaId != ddm->meta.id) {
        gBdd->data.meta->metaId = ddm->meta.id;
        DdiMetaAndExistAcc (gBdd,NULL,NULL,myCare);
      }

#if META_AE_META_METHOD1
      MetaSetConj(gBdd);
      gg = Ddi_BddMakeConst(ddm,1);
      printf("@");
#else
      MetaSetConj(gBdd);
      gg = Ddi_BddMakePartConjVoid(ddm);
      aa = gBdd->data.meta->zero;
      for (i=0; i<ng; i++) {
        dc = Ddi_BddNot(Ddi_BddarrayRead(aa,i));
        Ddi_BddPartInsertLast(gg,dc);
        Ddi_Free(dc);
      }
#endif
    }
    else {
      gg = Ddi_BddDup(gBdd);
    }
  }

  if (Ddi_BddIsMono(gg)||(Ddi_BddIsPartConj(gg)&&(Ddi_BddPartNum(gg)==1))) {
    gMono = 1;
  }
  else {
    gMono = 0;
  }

  MetaSetConj(fMeta);
  m = fMeta->data.meta;

#if HEURISTIC1
  if (Ddi_VarsetNum(smooth) == 0) {
#else
#if HEURISTIC2
    if (0) {
#else
    if (gBdd!=NULL) {
#endif
#endif
    if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
      fprintf (stdout, "[MA(%d):", Ddi_VarsetNum(smooth));
      fflush(stdout);
    }
    dc = Ddi_BddNot(Ddi_BddarrayRead(m->zero,ng-1));
    dc = Ddi_BddAndAcc(dc,gg);
    if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
      fprintf (stdout, "%d]", Ddi_BddSize(dc));
      fflush(stdout);
    }
    Ddi_BddarrayWrite(m->dc,ng-1,dc);
    Ddi_BddarrayWrite(m->zero,ng-1,Ddi_BddNotAcc(dc));
    Ddi_Free(dc);
    Ddi_Free(gg);

    if (Ddi_VarsetNum(smooth) == 0) {
      MetaReduce(fMeta,-1);
      MetaSimplify(fMeta,0,ng-1);
      Ddi_Free(smooth);

      Ddi_MgrCheckExtRef(ddm,extRef);

      Ddi_Free(myCare);
      return (fMeta);
    }
    else {
#if 0
      MetaAlign (fMeta,myCare);
#endif
      /*Ddi_VarsetPrint(smooth, 0, NULL, stdout);*/
      MetaReduce(fMeta,-1);
      /*MetaSimplify(fMeta,0);*/
    }
    gg = Ddi_BddMakeConst(ddm,1);
  }

  ff = Ddi_BddMakePartConjVoid(ddm);

  range = Ddi_BddSupp(fMeta);
  aux = Ddi_BddSupp(gg);
  Ddi_VarsetUnionAcc(range,aux);
  Ddi_Free(aux);
  Ddi_VarsetDiffAcc(range,smooth);

  downGroups = Ddi_VarsetarrayAlloc(ddm,ng);
  downTot = Ddi_VarsetVoid(ddm);


  if (ddm->settings.meta.method == Ddi_Meta_Size_Window) {
    for (i=ng-1; i>0; i--) {
      metaGrp = Ddi_VarsetarrayRead(ddm->meta.groups,i);
      Ddi_VarsetarrayWrite(downGroups,i-1,metaGrp);
    }
    Ddi_VarsetarrayWrite(downGroups,ng-1,downTot);
    Ddi_Free(downTot);
  }
  else {
    for (i=ng-1; i>0; i--) {
      Ddi_VarsetarrayWrite(downGroups,i,downTot);
      metaGrp = Ddi_VarsetarrayRead(ddm->meta.groups,i);
      Ddi_VarsetUnionAcc(downTot,metaGrp);

#if 0
      zero = Ddi_BddarrayRead(m->zero,i);
      aux = Ddi_VarsetIntersect(metaGrp,smooth);
      printf ("/SM[%d]=%d|f|=%d:",i,Ddi_VarsetNum(aux),Ddi_BddSize(zero));
      Ddi_Free(aux);
      aux = Ddi_BddSupp(zero);
      Ddi_VarsetIntersectAcc(aux,smooth);
      printf ("%d|",Ddi_VarsetNum(aux));
      Ddi_Free(aux);
      aux = Ddi_BddSupp(zero);
      Ddi_VarsetPrint(aux, 0, NULL, stdout);
      Ddi_Free(aux);
#endif
    }
    Ddi_VarsetarrayWrite(downGroups,0,downTot);
    Ddi_Free(downTot);
  }

#if 0
  for (i=0; i<ng; i++) {

      Ddi_Varset_t *s = Ddi_BddSupp(Ddi_BddarrayRead(m->zero,i));
      Ddi_VarsetIntersectAcc(s,smooth);
      if (Ddi_VarsetIsVoid(s)) {
	printf("=");
      }
      else {
	printf("@");
      }
      fflush(stdout);
      Ddi_Free(s);
  }
#endif

  for (i=0; i<ng; i++) {
    dc = Ddi_BddNot(Ddi_BddarrayRead(m->zero,i));
#if META_AE_META_METHOD1
    if ((gBdd!=NULL)&&(Ddi_BddIsMeta(gBdd))) {
      Ddi_Bddarray_t *aa;
      Ddi_Bdd_t *p;
      aa = gBdd->data.meta->zero;
      p = Ddi_BddNot(Ddi_BddarrayRead(aa,i));
      Ddi_BddSetPartConj(dc);
      Ddi_BddPartInsertLast(dc,p);
      Ddi_Free(p);
    }
#endif

#if META_OVER
    Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,Pdtutil_VerbLevelUsrMin_c);
    tmp = Ddi_BddExistOverApprox(dc,smooth);
    Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,verbosity);
#if 0
    Ddi_BddRestrictAcc(dc,tmp);
#else
    dc = Ddi_BddEvalFree(MetaConstrainOpt(dc,tmp),dc);
    gg = Ddi_BddEvalFree(MetaConstrainOpt(gg,tmp),gg);
#endif
    Ddi_BddNotAcc(tmp);
    Ddi_BddarrayWrite(m->zero,i,tmp);
    Ddi_Free(tmp);
#else
    Ddi_BddarrayWrite(m->zero,i,Ddi_MgrReadZero(ddm));
#endif

    Ddi_BddPartInsertLast(ff,dc);
    Ddi_Free(dc);

    tmp = Ddi_BddNot(Ddi_BddarrayRead(m->zero,i));
    if (i<ng-1) {
      Ddi_BddarrayWrite(m->one,i,Ddi_MgrReadZero(ddm));
      Ddi_BddarrayWrite(m->dc,i,tmp);
    }
    else {
      Ddi_BddarrayWrite(m->one,i,tmp);
      Ddi_BddarrayWrite(m->dc,i,Ddi_MgrReadZero(ddm));
    }
    Ddi_Free(tmp);
  }

  for (i=0; i<ng; i++) {

    if (verbosity >= Pdtutil_VerbLevelDevMax_c) {
      fprintf (stdout, " MAE[%d] ", i);
      fflush(stdout);
    }

    if (Ddi_BddIsPartConj(gg)) {
      preFetched = Ddi_BddPartNum(gg)-1;
    }

    if (Ddi_BddIsMono(gg)||(Ddi_BddIsPartConj(gg)&&(Ddi_BddPartNum(gg)==1))) {
      gMono = 1;
    }
    else {
      gMono = 0;
    }
    forceSmoothUp = 0;
#if 1
    if ((enableUpdate)&&/*(i<ng-1)&&*/(MetaUpdate(ddm,1))) {
      Ddi_Free(range);
      Ddi_Free(downGroups);
      Ddi_Free(myCare);
      MetaAndExistAfterUpdate(fMeta,ff,gg,smooth,care,i,ng,verbosity);
      return (NULL);
    }
#endif

 
    dc = Ddi_BddPartExtract(ff,0);

    if ((enableSkipLayer)&&Ddi_BddIsOne(dc)&&(i<ng-1)&&(!preFetched)) {
#if !META_OVER
      Ddi_BddarrayWrite(m->one,i,Ddi_MgrReadZero(ddm));
      Ddi_BddarrayWrite(m->zero,i,Ddi_MgrReadZero(ddm));
      Ddi_BddarrayWrite(m->dc,i,Ddi_MgrReadOne(ddm));
#endif
      Ddi_Free(dc);
      lastSkip = i;
      continue;
    }

    if (i<ng-1) {
      downTot = Ddi_BddSupp(ff);
    }
    else {
      downTot = Ddi_VarsetVoid(ddm);
    }
    smooth_i = Ddi_VarsetDiff(smooth,downTot);
    /* Ddi_VarsetDiffAcc(smooth,smooth_i); */

    smooth_down = Ddi_VarsetarrayRead(downGroups,i);
    smooth_down = Ddi_VarsetUnion(smooth_down,smooth);

#if 0
    tmp = Ddi_BddAndExist(gg,dc,smooth_i);
    if (((size=Ddi_BddSize(tmp)) < 3*Ddi_BddSize(gg))||(size<3*initSize)) {
      Ddi_Free(gg);
      gg = tmp;
      Ddi_Free(dc);
      dc = Ddi_BddMakeConst(ddm,1);
      if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
        printf("#");fflush(stdout);
      }
    }
    else {
      Ddi_Free(tmp);
#else 
    {
#endif
#if 0
      Ddi_MgrSiftDisable(ddm);
      tmp = Ddi_BddCofexist(gg,dc,smooth);
      if (((size=Ddi_BddSize(tmp)) < 5*Ddi_BddSize(gg))||(size<1*initSize)) {
        Ddi_BddExistAcc(dc,smooth);
        Ddi_Free(gg);
        gg = tmp;
        Ddi_BddConstrainAcc(ff,dc); /* !!! ???*/
	/*printf("@");fflush(stdout);*/
      }
      else {
      printf("<@%d(%d)>",Ddi_BddSize(tmp)/Ddi_BddSize(gg),Ddi_BddSize(tmp));
        Ddi_Free(tmp);
        Ddi_BddSetPartConj(gg);
        Ddi_BddPartInsert(gg,0,dc);
        Ddi_Free(dc);
        dc = Ddi_BddMakeConst(ddm,1);
printf("?");fflush(stdout);
      }
      Ddi_MgrSiftEnable(ddm, Ddi_ReorderingMethodString2Enum("sift"));
#else
      Ddi_BddSetPartConj(gg);
      if (!Ddi_BddIsOne(dc)) {
        int l;
        Ddi_BddSetPartConj(dc);
        l = Ddi_BddPartNum(dc)-1;
        if (l>1)
          printf("^%d^",l);
        while (l>=0) {
          Ddi_BddPartInsert(gg,1,Ddi_BddPartRead(dc,l));
          l--;
	}
        Ddi_Free(dc);
        dc = Ddi_BddMakeConst(ddm,1);
      }

#if 1

#if HEURISTIC3
      if (strongSimplify) {
        if (myCare != NULL) {
          gg = Ddi_BddEvalFree(MetaConstrainOpt(gg,myCare),gg);
        }
        for (j=0; j<i; j++) {
          tmp = Ddi_BddarrayRead(m->dc,j);
          gg = Ddi_BddEvalFree(MetaConstrainOpt(gg,tmp),gg);
        }
      }
#endif
      if (1) /*(Ddi_BddIsMono(gg)||(Ddi_BddPartNum(gg)<3))*/ {
      /*Ddi_MgrSiftSuspend(ddm);*/

      int np=1;
      if (Ddi_BddIsPartConj(gg)) {
        np = Ddi_BddPartNum(gg);
        if (np>3) {
          printf(":%d:",np);
          /*np=2;*/
	}
      }

      Ddi_MgrSiftSuspend(ddm);
      Ddi_MgrSetExistClustThresh(ddm,
        META_MAX(7*Ddi_BddSize(gg)/np,2*initSize));
      Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,Pdtutil_VerbLevelUsrMin_c);
      Ddi_BddExistAcc(gg,smooth_i);
      Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,verbosity);
      Ddi_MgrSetExistClustThresh(ddm,-1);
      Ddi_MgrSiftResume(ddm);

      if (Ddi_BddIsMono(gg)) {
        if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
          if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
            printf("#");fflush(stdout);
          }
        }
      }
      else {
        int j, nj;
        if ((i<ng-1)&&(i>(3*ng)/6)) {
          if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
            printf("!");fflush(stdout);
          }
          nj = META_MIN(0,Ddi_BddPartNum(ff));
          for (j=0; j<nj; j++) {
            tmp = Ddi_BddPartExtract(ff,0);
            Ddi_BddPartInsertLast(ff,Ddi_MgrReadOne(ddm));
            if (preFetched < (Ddi_BddPartNum(gg)-1))
              preFetched++;
            else
              preFetched = Ddi_BddPartNum(gg)-1;
            if (0/*gMono*/)
              Ddi_BddPartInsertLast(gg,tmp);
            else
              Ddi_BddPartInsert(gg,1/*+preFetched*/,tmp);
            Ddi_Free(tmp);
  	  }
	}
        else {
          if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
            printf("&");fflush(stdout);
            if (i==ng-1) {
              forceSmoothUp = 1;
	    }
          }
        }
      }
      }
#if HEURISTIC3
      if (myCare != NULL) {
        gg = Ddi_BddEvalFree(MetaConstrainOpt(gg,myCare),gg);
      }
      for (j=0; j<i; j++) {
        tmp = Ddi_BddarrayRead(m->dc,j);
        gg = Ddi_BddEvalFree(MetaConstrainOpt(gg,tmp),gg);
      }
#endif

#endif
#endif
    }

    Ddi_Free(downTot);
  

    if ((0||forceSmoothUp || (lastSkip == i-1))&&(i>ng/2)) {
      int size1, size2;
      int np=1;

      if (Ddi_BddIsPartConj(gg)) {
        np = Ddi_BddPartNum(gg);
        if (np>3) {
          printf(";:%d:;",np);
          /*np=2;*/
	}
      }

      smooth_down_up = Ddi_VarsetarrayRead(downGroups,i-1);
      smooth_down_up = Ddi_VarsetUnion(smooth_down_up,smooth);

      size = Ddi_BddSize(gg);

      if (forceSmoothUp) {
        if (Ddi_BddIsMono(gg)) {
          up = Ddi_BddExist(gg,smooth_down_up);
	}
        else {
#if 1
	  Ddi_Bdd_t *dcup;
          if (myCare != NULL) {
            dcup = Ddi_BddDup(myCare);
          }
          else {
            dcup = Ddi_BddMakeConst(ddm,1);
          }
          Ddi_BddSetPartConj(dcup);
          for (j=0; j<i; j++) {
            Ddi_BddPartInsertLast(dcup,Ddi_BddarrayRead(m->dc,j));
	  }
          up = Ddi_BddMultiwayRecursiveAndExist(gg,smooth_down_up,dcup,-1);
          Ddi_Free(dcup);
#else
          up = Ddi_BddExistOverApprox(gg,smooth_down_up);
#endif
	}
      }
      else {
        Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,Pdtutil_VerbLevelUsrMin_c);
        Ddi_MgrSetExistClustThresh(ddm,
          META_MAX(7*size/np,2*initSize));
        up = Ddi_BddExist(gg,smooth_down_up);
        Ddi_MgrSetExistClustThresh(ddm,-1);
      }
      if (MetaUpdate(ddm,0)) {
        if (myCare != NULL) {
          up = Ddi_BddEvalFree(MetaConstrainOpt(up,myCare),up);
        }
        for (j=0; j<i; j++) {
          up = Ddi_BddEvalFree(
            MetaConstrainOpt(up,Ddi_BddarrayRead(m->dc,j)),up);
        }
      }

      if (Ddi_BddIsMono(up)&&(!Ddi_BddIsOne(up))) {
#if 1
        if (myCare != NULL) {
          up = Ddi_BddEvalFree(MetaConstrainOpt(up,myCare),up);
        }
        for (j=0; j<i-1; j++) {
          tmp = Ddi_BddarrayRead(m->dc,j);
          up = Ddi_BddEvalFree(MetaConstrainOpt(up,tmp),up);
        }
#endif
        tmp = MetaConstrainOpt(gg,up);

        size1 = Ddi_BddSize(up);
        size2 = Ddi_BddSize(tmp);

        if (MetaUpdate(ddm,0)) {
	  /* recompute size after reordering */
          size = Ddi_BddSize(gg);
	}

        if ((size2<0.99*size)||(size1+size2<0.99*size)) {
          if (verbosity >= Pdtutil_VerbLevelDevMax_c) {
            fprintf(stdout,
              "\nREDUCED UP: %d > (%d,%d)\n", size, size1, size2); 
          }
          Ddi_Free(gg); gg = tmp;

          if (MetaUpdate(ddm,0)) {
            Ddi_Bdd_t *dcUp;
            dcUp = Ddi_BddarrayExtract(m->dc,i-1);
            if (myCare != NULL) {
              dcUp = Ddi_BddEvalFree(MetaConstrainOpt(dcUp,myCare),dcUp);
            }
            for (j=0; j<i-1; j++) {
              tmp = Ddi_BddarrayRead(m->dc,j);
              dcUp = Ddi_BddEvalFree(MetaConstrainOpt(dcUp,tmp),dcUp);
            }
            Ddi_BddarrayInsert(m->dc,i-1,dcUp);
            Ddi_Free(dcUp);
	  }
	  if (0 && i==(ng-1) && Ddi_BddSize(up) > 10000) {
	    Ddi_BddSetPartConj(Ddi_BddarrayRead(m->dc,i-1));
            Ddi_BddPartInsertLast(Ddi_BddarrayRead(m->dc,i-1),up);
	  }
	  else {
            Ddi_BddAndAcc(Ddi_BddarrayRead(m->dc,i-1),up);
	  }
#if 0
	  /* ALREADY DONE WHILE SIMPLIFYING UP */
#if HEURISTIC3
          MetaSimplify(fMeta,0,i-1);
#endif
#endif
          Ddi_Free(up);
          up = Ddi_BddNot(Ddi_BddarrayRead(m->dc,i-1));
          Ddi_BddarrayWrite(m->zero,i-1,up);
        }
        else {
          Ddi_Free(tmp);
        }
      }
      Ddi_Free(up);
      Ddi_Free(smooth_down_up);
    }
    tmp = Ddi_BddDup(gg);
#if 0
    if (Ddi_BddIsPartConj(tmp)) {
      int nj = Ddi_BddPartNum(tmp);
      /* revert order from 2-nd to last component */
      for (j=1;j<=(nj-1)/2;j++) {
        Ddi_Bdd_t *p1 = Ddi_BddDup(Ddi_BddPartRead(tmp,j));
        Ddi_Bdd_t *p2 = Ddi_BddDup(Ddi_BddPartRead(tmp,nj-j));
        Ddi_BddPartWrite(tmp,nj-j,p1);
        Ddi_BddPartWrite(tmp,j,p2);
        Ddi_Free(p1);
        Ddi_Free(p2);
      }
    }
#endif
    if (Ddi_VarsetNum(smooth_down) > 0)
    {
      int np=1;
      if (Ddi_BddIsPartConj(tmp)) {
        np = Ddi_BddPartNum(tmp);
        if (np>3) {
          printf("?:%d:?",np);
          /*np=2;*/
	}
      }
      Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,Pdtutil_VerbLevelUsrMin_c);
      Ddi_MgrSetExistClustThresh(ddm,
        META_MAX(7*Ddi_BddSize(tmp)/np,2*initSize));
      Ddi_BddExistAcc(tmp,smooth_down);
      Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,verbosity);
      Ddi_MgrSetExistClustThresh(ddm,-1);
    }
    if (MetaUpdate(ddm,0)) {
      if (myCare != NULL) {
        tmp = Ddi_BddEvalFree(MetaConstrainOpt(tmp,myCare),tmp);
      }
      for (j=0; j<i; j++) {
        tmp = Ddi_BddEvalFree(
          MetaConstrainOpt(tmp,Ddi_BddarrayRead(m->dc,j)),tmp);
      }
    }

    if ((!MetaUpdate(ddm,0))||(i>=ng-1)) {
      if (Ddi_BddIsMono(tmp)) {
        Ddi_BddAndAcc(dc,tmp);
      }
      else {
	int k;
        for (k=0;k<Ddi_BddPartNum(tmp);k++) {
          Ddi_Bdd_t *dc2; 
          p = Ddi_BddDup(Ddi_BddPartRead(tmp,k));
          Ddi_BddPartWrite(tmp,k,Ddi_MgrReadOne(ddm));

          Ddi_MgrAbortOnSiftEnableWithThresh(ddm,2*Ddi_BddSize(p),0);
          Ddi_BddExistAcc(p,smooth_down);
          if (ddm->abortedOp) {
            Ddi_MgrAbortOnSiftDisable(ddm);
            Ddi_Free(p);
            continue;
          }
          else {
            Ddi_MgrAbortOnSiftDisable(ddm);
	  }
#if HEURISTIC3
          if (myCare != NULL) {
            p = Ddi_BddEvalFree(MetaConstrainOpt(p,myCare),p);
          }
          for (j=0; j<i; j++) {
            p = Ddi_BddEvalFree(
	      MetaConstrainOpt(p,Ddi_BddarrayRead(m->dc,j)),p);
          }
#endif
          dc2 = Ddi_BddDup(dc);

          Ddi_MgrAbortOnSiftEnableWithThresh(ddm,
            Ddi_BddSize(dc2)+Ddi_BddSize(p),0);
          Ddi_BddAndAcc(dc2,p);
          if (ddm->abortedOp) {
            Ddi_MgrAbortOnSiftDisable(ddm);
            Ddi_Free(dc2);
            Ddi_Free(p);
            continue;
          }
          else {
	    Ddi_Free(dc);
            dc = dc2;
            Ddi_MgrAbortOnSiftDisable(ddm);
	  }
#if HEURISTIC3
          if (myCare != NULL) {
            dc = Ddi_BddEvalFree(MetaConstrainOpt(dc,myCare),dc);
          }
          for (j=0; j<i; j++) {
            dc = Ddi_BddEvalFree(
	      MetaConstrainOpt(dc,Ddi_BddarrayRead(m->dc,j)),dc);
          }
#endif
          gg = Ddi_BddEvalFree(
	    MetaConstrainOpt(gg,p),gg);
          gg = Ddi_BddEvalFree(
	    MetaConstrainOpt(gg,dc),gg);
          Ddi_Free(p);
          if (MetaUpdate(ddm,1)) {
            Ddi_Free(range);
            Ddi_Free(tmp);
            Ddi_Free(downGroups);
            Ddi_BddPartInsert(ff,0,dc);
            Ddi_Free(dc);
            Ddi_Free(smooth_down);
            Ddi_Free(smooth_i);
            Ddi_Free(myCare);
            MetaAndExistAfterUpdate(fMeta,ff,gg,smooth,care,i,ng,verbosity);
            return (NULL);
	  }
        }

        if (i == ng-1) {
	  Ddi_Bdd_t *dcup;
          if (myCare != NULL) {
            dcup = Ddi_BddDup(myCare);
          }
          else {
            dcup = Ddi_BddMakeConst(ddm,1);
          }
          Ddi_BddSetPartConj(dcup);
          for (j=0; j<i; j++) {
            Ddi_BddPartInsertLast(dcup,Ddi_BddarrayRead(m->dc,j));
	  }
#if META_OVER
          auxf = Ddi_BddNot(Ddi_BddarrayRead(m->zero,i));
          Ddi_BddPartInsertLast(dcup,auxf);
          Ddi_Free(auxf);
#endif
          if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
	    printf("<<(E%d.%d[%d]->",
	      Ddi_VarsetNum(smooth_down), Ddi_BddSize(gg), Ddi_BddPartNum(gg));
            fflush(stdout);
	  }
#if 0
          Ddi_BddPartInsert(gg,0,dc);
          Ddi_Free(dc);
          dc = Ddi_BddMakeConst(ddm,1);
#else
          Ddi_BddPartInsertLast(dcup,dc);
#endif
#if 1
          gg = Ddi_BddEvalFree(
            Ddi_BddMultiwayRecursiveAndExist(gg,smooth_down,
              dcup,-1),gg);
#else
          Ddi_BddExistAcc(gg,smooth_down);
#endif
          if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
  	    printf("->%d",Ddi_BddSize(gg));
          }
          fflush(stdout);
#if HEURISTIC3
          if (myCare != NULL) {
            gg = Ddi_BddEvalFree(MetaConstrainOpt(gg,myCare),gg);
            dc = Ddi_BddEvalFree(MetaConstrainOpt(dc,myCare),dc);
          }
          for (j=0; j<i; j++) {
            gg = Ddi_BddEvalFree(
	      MetaConstrainOpt(gg,Ddi_BddarrayRead(m->dc,j)),gg);
            dc = Ddi_BddEvalFree(
	      MetaConstrainOpt(dc,Ddi_BddarrayRead(m->dc,j)),dc);
          }
#endif
          if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
	    printf("|->%d*(dc:%d)->",Ddi_BddSize(gg),Ddi_BddSize(dc));
	  }
          fflush(stdout);
	  if (Ddi_BddIsMono(gg))
          {
	    Ddi_Bdd_t *up;
            smooth_down_up = Ddi_VarsetarrayRead(downGroups,i-1);
	    up = Ddi_BddAndExist(dc,gg,smooth_down_up);
            gg = Ddi_BddEvalFree(MetaConstrainOpt(gg,up),gg);
            Ddi_BddAndAcc(Ddi_BddarrayRead(m->dc,i-1),up);
	    Ddi_Free(up);
            up = Ddi_BddNot(Ddi_BddarrayRead(m->dc,i-1));
            Ddi_BddarrayWrite(m->zero,i-1,up);
	    Ddi_Free(up);
	  }
          Ddi_BddAndAcc(dc,gg);
          if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
  	    printf("->%d",Ddi_BddSize(dc));
	  }
          fflush(stdout);
#if HEURISTIC3
          if (myCare != NULL) {
            dc = Ddi_BddEvalFree(MetaConstrainOpt(dc,myCare),dc);
          }
          for (j=0; j<i; j++) {
            dc = Ddi_BddEvalFree(
	      MetaConstrainOpt(dc,Ddi_BddarrayRead(m->dc,j)),dc);
          }
#endif
          if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
	    printf("|->%d>>",Ddi_BddSize(dc));
            fflush(stdout);
	  }
          Ddi_Free(dcup);
       }
      }
    }
    Ddi_Free(tmp);

#if META_OVER
    auxf = Ddi_BddNot(Ddi_BddarrayRead(m->zero,i));
    Ddi_BddSetPartConj(dc);
    Ddi_BddPartInsertLast(dc,auxf);
    Ddi_Free(auxf);
#endif

#if HEURISTIC3
    if (!MetaUpdate(ddm,0)) {
      for (j=0; j<i; j++) {
        tmp = Ddi_BddarrayRead(m->dc,j);
        if (MetaUpdate(ddm,1)) {
          Ddi_Free(range);
          Ddi_Free(smooth_i);
          Ddi_Free(smooth_down);
          Ddi_Free(downGroups);
          Ddi_BddPartInsert(ff,0,dc);
          Ddi_Free(dc);
          Ddi_Free(myCare);
          MetaAndExistAfterUpdate(fMeta,ff,gg,smooth,care,i,ng,verbosity);
          return (NULL);
	}
        if (myCare != NULL) {
          dc = Ddi_BddEvalFree(MetaConstrainOpt(dc,myCare),dc);
        }
        dc = Ddi_BddEvalFree(MetaConstrainOpt(dc,tmp),dc);
      }
    }
#endif

#if 0
#if META_OVER
    Ddi_BddSetMono(dc);
#endif
#endif

    if (i<ng-1) {

      /*Ddi_MgrSiftSuspend(ddm);*/
      Ddi_BddarrayWrite(m->dc,i,dc);
      Ddi_BddarrayWrite(m->one,i,Ddi_MgrReadZero(ddm));
#if !HEURISTIC3

#if 0
      tmp = Ddi_BddRestrict(ff,dc);
      if (Ddi_BddSize(tmp) < Ddi_BddSize(ff)) {
        Ddi_Free(ff);
        ff = tmp;
      }
      else {
        Ddi_Free(tmp);
      }
#endif

#if 0
      for (j=i/2; j<=i; j++) {
        tmp = Ddi_BddarrayRead(m->dc,j);
        gg = Ddi_BddEvalFree(MetaConstrainOpt(gg,tmp),gg);
      }
#else
      if (myCare != NULL) {
        gg = Ddi_BddEvalFree(MetaConstrainOpt(gg,myCare),gg);
      }
      gg = Ddi_BddEvalFree(MetaConstrainOpt(gg,dc),gg);
#endif

#endif
      
      /*Ddi_MgrSiftResume(ddm);*/

#if 0
#if 0
      Ddi_BddSetMono(gg);
#else
      Ddi_MgrSetExistClustThresh(ddm,META_MAX(2*Ddi_BddSize(gg),1*initSize));
      Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,Pdtutil_VerbLevelUsrMin_c);
      Ddi_BddExistAcc(gg,smooth_i);
      Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,verbosity);
      Ddi_MgrSetExistClustThresh(ddm,-1);
      if (Ddi_BddIsMono(gg)) {
        if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
          printf("#");fflush(stdout);
        }
      }
      else {
        if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
          printf("&");fflush(stdout);
        }
      }
#endif
#endif
    }
    else {
      Ddi_BddarrayWrite(m->one,i,dc);
      Ddi_BddarrayWrite(m->dc,i,Ddi_MgrReadZero(ddm));
    }

    Ddi_Free(smooth_i);
    Ddi_BddarrayWrite(m->zero,i,Ddi_BddNotAcc(dc));

    Ddi_Free(dc);
    Ddi_Free(smooth_down);

    if ((size = Ddi_BddSize(gg))>peak) {
      peak = size;
    }
    if (verbosity >= Pdtutil_VerbLevelDevMax_c) {
      fprintf (stdout, "<%d>", size);
      fflush(stdout);
    }

  }

  if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
    fprintf (stdout, "(max:%d)%d->", peak, Ddi_BddSize(fMeta));
    fflush(stdout);
  }

  MetaReduce(fMeta,-1);
  Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,verbosity);

#if 0
  if (strongSimplify) {
    if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
      fprintf (stdout, "%d|", Ddi_BddSize(fMeta));
      fflush(stdout);
    }

    MetaSimplify(fMeta,0,ng-1);
  }
#endif

  if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
    fprintf (stdout, "%d$", Ddi_BddSize(fMeta));
    fflush(stdout);
  }

  Ddi_Free(range);
  Ddi_Free(ff);
  Ddi_Free(gg);
  Ddi_Free(smooth);
  Ddi_Free(downGroups);
  Ddi_Free(myCare);
#if 0
  if (Ddi_MgrCheckExtRef(ddm,extRef)==0) {
    Ddi_MgrPrintExtRef(ddm,nodeId);
  }
#endif

  if (0)
  {
    Ddi_Bdd_t *mono = Ddi_BddMakeFromMeta(fMeta);
    printf("<<%d>>",Ddi_BddSize(mono));
    Ddi_Free(mono);
  }


  return(fMeta);

}

/**Function********************************************************************
  Synopsis    [Operate compose]
  Description [Operate compose]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
DdiMetaComposeAcc (
  Ddi_Bdd_t *fMeta,
  Ddi_Vararray_t *v,
  Ddi_Bddarray_t *g
)
{
  Ddi_BddarrayOp(Ddi_BddCompose_c,fMeta->data.meta->one,v,g);
  Ddi_BddarrayOp(Ddi_BddCompose_c,fMeta->data.meta->zero,v,g);
  Ddi_BddarrayOp(Ddi_BddCompose_c,fMeta->data.meta->dc,v,g);

  return(fMeta);
}

/**Function********************************************************************
  Synopsis    [Operate compose]
  Description [Operate compose]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
DdiMetaConstrainAcc (
  Ddi_Bdd_t *fMeta,
  Ddi_Bdd_t *c
)
{
  Ddi_Bdd_t *tmp;
  int ng, i;

  Pdtutil_Assert(fMeta->common.code == Ddi_Bdd_Meta_c,
    "MetaConstrain requires Meta BDD!");
#if 0
  Pdtutil_Assert(c->common.code != Ddi_Bdd_Meta_c,
    "MetaConstrain requires non Meta c term!");
#endif
  Ddi_BddarrayOp2(Ddi_BddConstrain_c,fMeta->data.meta->one,c);
  Ddi_BddarrayOp2(Ddi_BddConstrain_c,fMeta->data.meta->zero,c);
  Ddi_BddarrayOp2(Ddi_BddConstrain_c,fMeta->data.meta->dc,c);
  ng = Ddi_BddarrayNum(fMeta->data.meta->one);
  tmp = Ddi_BddDup(Ddi_BddarrayRead(fMeta->data.meta->zero,ng-1));
  Ddi_BddNotAcc(tmp);
  Ddi_BddarrayWrite(fMeta->data.meta->one,ng-1,tmp);
  Ddi_Free(tmp);
  for (i=0; i<ng; i++) {
    tmp = Ddi_BddDup(Ddi_BddarrayRead(fMeta->data.meta->zero,i));
    Ddi_BddOrAcc(tmp,Ddi_BddarrayRead(fMeta->data.meta->one,i));
    Ddi_BddNotAcc(tmp);
    Ddi_BddarrayWrite(fMeta->data.meta->dc,i,tmp);
    Ddi_Free(tmp);
  }

  return(fMeta);
}

/**Function********************************************************************
  Synopsis    [Operate compose]
  Description [Operate compose]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
DdiMetaRestrictAcc (
  Ddi_Bdd_t *fMeta,
  Ddi_Bdd_t *c
)
{
  Ddi_Bdd_t *tmp, *c2;
  int ng, i;

#if 0
  Pdtutil_Assert(c->common.code != Ddi_Bdd_Meta_c,
    "MetaRestrict requires non Meta c term!");
#endif
  if (Ddi_BddIsMeta(c)) {
    if (MetaIsConj(c)) {
      c2 = Ddi_BddMakePartConjFromArray(c->data.meta->dc);
      Ddi_BddPartWrite(c2, Ddi_BddPartNum(c2)-1, 
       Ddi_BddarrayRead(c->data.meta->one,
       Ddi_BddarrayNum(c->data.meta->one)-1));
    }
    else {
      return(fMeta);
    }
  }
  else {
    c2 = Ddi_BddDup(c);
  }

  if (!Ddi_BddIsMeta(fMeta)) {
    Ddi_BddRestrictAcc(fMeta,c2);
    Ddi_Free(c2);
    return(fMeta);
  }

  Pdtutil_Assert(fMeta->common.code == Ddi_Bdd_Meta_c,
    "MetaRestrict requires Meta BDD!");

  Ddi_BddarrayOp2(Ddi_BddRestrict_c,fMeta->data.meta->one,c2);
  Ddi_BddarrayOp2(Ddi_BddRestrict_c,fMeta->data.meta->zero,c2);
  ng = Ddi_BddarrayNum(fMeta->data.meta->one);
  tmp = Ddi_BddDup(Ddi_BddarrayRead(fMeta->data.meta->zero,ng-1));
  Ddi_BddNotAcc(tmp);
  Ddi_BddarrayWrite(fMeta->data.meta->one,ng-1,tmp);
  Ddi_Free(tmp);
  for (i=0; i<ng; i++) {
    if (i<ng-1) {
      Ddi_BddDiffAcc(Ddi_BddarrayRead(fMeta->data.meta->one,i),
		     Ddi_BddarrayRead(fMeta->data.meta->zero,i));
    }
    tmp = Ddi_BddDup(Ddi_BddarrayRead(fMeta->data.meta->zero,i));
    Ddi_BddOrAcc(tmp,Ddi_BddarrayRead(fMeta->data.meta->one,i));
    Ddi_BddNotAcc(tmp);
    Ddi_BddarrayWrite(fMeta->data.meta->dc,i,tmp);
    Ddi_Free(tmp);
  }
  Ddi_Free(c2);
  return(fMeta);
}


/**Function********************************************************************
  Synopsis    [Operate variable swap]
  Description [Operate variable swap]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
DdiMetaSwapVarsAcc (
  Ddi_Bdd_t *fMeta,
  Ddi_Vararray_t *v1,
  Ddi_Vararray_t *v2
)
{
  Ddi_BddarrayOp(Ddi_BddSwapVars_c,fMeta->data.meta->one,v1,v2);
  Ddi_BddarrayOp(Ddi_BddSwapVars_c,fMeta->data.meta->zero,v1,v2);
  Ddi_BddarrayOp(Ddi_BddSwapVars_c,fMeta->data.meta->dc,v1,v2);

  return(fMeta);
}


/**Function********************************************************************
  Synopsis    [Operate variable substitution]
  Description [Operate variable substitution]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
DdiMetaSubstVarsAcc (
  Ddi_Bdd_t *fMeta,
  Ddi_Vararray_t *v1,
  Ddi_Vararray_t *v2
)
{
  Ddi_BddarrayOp(Ddi_BddSubstVars_c,fMeta->data.meta->one,v1,v2);
  Ddi_BddarrayOp(Ddi_BddSubstVars_c,fMeta->data.meta->zero,v1,v2);
  Ddi_BddarrayOp(Ddi_BddSubstVars_c,fMeta->data.meta->dc,v1,v2);

  return(fMeta);
}

/**Function********************************************************************
  Synopsis    [Operate And between two Meta BDDs]
  Description [Operate And between two Meta BDDs]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
DdiMetaAndAcc (
  Ddi_Bdd_t *fMeta,
  Ddi_Bdd_t *gMeta
)
{
#if 1
  /* @@@@ GP 03 */
  if (gMeta->common.code != Ddi_Bdd_Meta_c) {
    if (Ddi_BddIsPartDisj(gMeta)) {
      int i;
      Ddi_Bdd_t *fDup, *f_i;
      Ddi_BddSetPartDisj(fMeta);
      fDup = Ddi_BddPartExtract(fMeta,0);
      for (i=0; i<Ddi_BddPartNum(gMeta); i++) {
	f_i = Ddi_BddDup(fDup);
        DdiMetaAndExistAcc(f_i,Ddi_BddPartRead(gMeta,i),NULL,NULL);
	Ddi_BddPartInsertLast(fMeta,f_i);
	Ddi_Free(f_i);
      }
      Ddi_Free(fDup);
      return(fMeta);
    }
    else if (Ddi_BddIsPartConj(gMeta)) {
      Ddi_Bdd_t *fDup = Ddi_BddDup(gMeta);
      Ddi_BddPartInsert(fDup,0,fMeta);
      Ddi_DataCopy(fMeta,fDup);
      Ddi_Free(fDup);
      return(fMeta);
    }
    else {
      return(DdiMetaAndExistAcc(fMeta,gMeta,NULL,NULL));
    }
  }
#endif
  Ddi_BddNotAcc(fMeta);
  Ddi_BddNotAcc(gMeta);
  DdiMetaOrAcc(fMeta,gMeta);
  Ddi_BddNotAcc(fMeta);
  Ddi_BddNotAcc(gMeta);

  return(fMeta);
}

/**Function********************************************************************
  Synopsis    [Operate Or between two Meta BDDs]
  Description [Operate Or between two Meta BDDs]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
DdiMetaOrAcc (
  Ddi_Bdd_t *fMeta,
  Ddi_Bdd_t *gMeta
)
{
  return(MetaOrAcc(fMeta,gMeta,0));
}

/**Function********************************************************************
  Synopsis    [Operate Or between two disjoined Meta BDDs]
  Description [Operate Or between two disjoined Meta BDDs 
               (i.e. with NULL intersection)]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
DdiMetaOrDisjAcc (
  Ddi_Bdd_t *fMeta,
  Ddi_Bdd_t *gMeta
)
{
  return(MetaOrAcc(fMeta,gMeta,1));
}

/**Function********************************************************************
  Synopsis    [Align Meta BDD to new ordering if required.]
  Description [Align Meta BDD to new ordering if required.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
DdiBddMetaAlign (
  Ddi_Bdd_t *fMeta
)
{
  if (Ddi_BddIsMeta(fMeta)) {
    MetaAlign(fMeta,NULL);
  }
  return (fMeta);
}

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

#define DdiMetaCopy(m,f) \
  (fprintf(stderr,"DdiMetaCopy still to implement\n"),NULL)
/*
#define DdiMetaAnd(f,g) \
  (fprintf(stderr,"DdiMetaAnd still to implement\n"),NULL)
#define DdiMetaConstrain(f,g) \
  (fprintf(stderr,"DdiMetaConstrain still to implement\n"),NULL)
#define DdiMetaRestrict(f,g) \
  (fprintf(stderr,"DdiMetaRestrict still to implement\n"),NULL)
#define DdiMetaExist(f,g) \
  (fprintf(stderr,"DdiMetaExist still to implement\n"),NULL)
#define DdiMetaSwapVars(f,g,h) \
  (fprintf(stderr,"DdiMetaAnd still to implement\n"),NULL)
#define DdiMetaSupp(f) \
  (fprintf(stderr,"DdiMetaSupp still to implement\n"),NULL)
*/


/**Function********************************************************************
  Synopsis    [Return true if Meta handling active (Ddi_MetaInit done)]
  Description [Return true if Meta handling active (Ddi_MetaInit done)]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_MetaActive (
  Ddi_Mgr_t *ddm
)
{
  return (ddm->meta.groups != NULL);
}

/**Function********************************************************************
  Synopsis    [Initialize Meta BDD handling in DDI manager]
  Description [Initialize Meta BDD handling in DDI manager]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_MetaInit (
  Ddi_Mgr_t *ddm,
  Ddi_Meta_Method_e method,
  Ddi_Bdd_t *ref,
  Ddi_Varset_t *firstGroupOrFilter,
  Ddi_Vararray_t *ps,
  Ddi_Vararray_t *ns,
  int sizeMin
)
{
  int nvar, nMgrVars, nGroupVars, ng, i, j, np, min;
  Ddi_Var_t *v;
  Ddi_Varset_t *metaGrp, *selectedVars, *psv, *aux; 
  int *used=NULL, freeOrd = 0;
  Ddi_Varsetarray_t *groups;

  if (ddm->meta.groups!=NULL) {
    Ddi_Unlock(ddm->meta.groups);
    Ddi_Free(ddm->meta.groups);
    freeOrd = 1;
  }

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
    printf("Meta-Init\n");
  }

  ddm->meta.id = ddm->stats.siftRunNum;

  nvar = nMgrVars = Ddi_MgrReadNumCuddVars(ddm);

  if (method == Ddi_Meta_FilterVars) {
    used = Pdtutil_Alloc(int,nMgrVars);
    for (i=0; i<nMgrVars; i++) {
      if (firstGroupOrFilter == NULL && ddm->meta.nvar==0) {
	used[i]=1;
      }
      else {
	used[i]=0;
      }
    }
    if (firstGroupOrFilter == NULL && ddm->meta.nvar>0) {
      Pdtutil_Assert(ddm->meta.ord!=NULL,"missing meta ord");
      nvar = ddm->meta.nvar;
      for (i=0; i<nvar; i++) {
        used[ddm->meta.ord[i]]=1;
      }
    }
    else if (firstGroupOrFilter != NULL) {
      Ddi_Vararray_t *vA = Ddi_VararrayMakeFromVarset(firstGroupOrFilter,1);
      nvar = Ddi_VarsetNum(firstGroupOrFilter);
      for (i=0; i<nvar; i++) {
	int id = Ddi_VarCuddIndex(Ddi_VararrayRead(vA,i));
        used[id]=1;
      }
      Ddi_Free(vA);
    }
  }

  if (freeOrd) {
    Pdtutil_Free(ddm->meta.ord);
  }

  if (sizeMin > 0) {
    ddm->settings.meta.groupSizeMin = sizeMin;
  }
  ddm->settings.meta.method = method;

  ddm->meta.nvar = nvar;
  ddm->meta.ord = Pdtutil_Alloc(int,nvar);
  for (i=j=0; i<nMgrVars; i++) {
    int id = Ddi_MgrReadInvPerm(ddm,i);
    if (used == NULL || used[id] == 1) {
      ddm->meta.ord[j++] = id;
    }
  }
  Pdtutil_Assert(j==nvar,"wrong var num in meta init");
  ng = 0;
  groups = Ddi_VarsetarrayAlloc(ddm,0); 
  nGroupVars = 0;

  switch (method) {
  case Ddi_Meta_Size:
  case Ddi_Meta_Size_Window:
  case Ddi_Meta_McM:
  case Ddi_Meta_FilterVars:
    for (i=0; i<nvar; i++) {
      //      v = Ddi_VarAtLevel(ddm,i);
      v = Ddi_VarFromIndex(ddm,ddm->meta.ord[i]);
        if (Ddi_VarIsGrouped(v)) {
          /* if variable is in variable group take entire group */
          metaGrp = Ddi_VarReadGroup(v);
	  nGroupVars += Ddi_VarsetNum(metaGrp);
#if META_REVERSE
          Ddi_VarsetarrayInsert(groups,0,metaGrp);ng++;
#else
          Ddi_VarsetarrayWrite(groups,ng++,metaGrp);
#endif
	}
        else {
          /* variable is not grouped: add to group */
          metaGrp = Ddi_VarsetVoid(ddm);
          Ddi_VarsetAddAcc(metaGrp,v);
	  nGroupVars++;
#if META_REVERSE
          Ddi_VarsetarrayInsert(groups,0,metaGrp);ng++;
#else
          Ddi_VarsetarrayWrite(groups,ng++,metaGrp);
#endif
          Ddi_Free(metaGrp);
	}
    }
    break;
  case Ddi_Meta_EarlySched:
    nGroupVars = nvar;
    psv = Ddi_VarsetMakeFromArray(ps);
    Pdtutil_Assert(Ddi_BddIsPartConj(ref)||Ddi_BddIsPartDisj(ref),
      "Partitioned BDD required to bias early-sched");
    selectedVars = Ddi_VarsetVoid(ddm);
    if (firstGroupOrFilter != NULL) {
      Ddi_VarsetUnionAcc(selectedVars,firstGroupOrFilter);
      Ddi_VarsetarrayWrite(groups,ng++,firstGroupOrFilter);
    }
    np = Ddi_BddPartNum(ref);
    for (i=0;i<np;i++) {
#if META_REVERSE
      metaGrp = Ddi_BddSupp(Ddi_BddPartRead(ref,np-1-i));
#else
      metaGrp = Ddi_BddSupp(Ddi_BddPartRead(ref,i));
#endif
      Ddi_VarsetDiffAcc(metaGrp,selectedVars);
#if 1
      Ddi_VarsetDiffAcc(metaGrp,psv);
      aux = Ddi_VarsetSwapVars(metaGrp,ps,ns);
      Ddi_VarsetUnionAcc(metaGrp,aux);
      Ddi_Free(aux);
#endif
      Ddi_VarsetUnionAcc(selectedVars,metaGrp);

      Ddi_VarsetarrayWrite(groups,ng++,metaGrp);

      Ddi_Free(metaGrp);
    }
    Ddi_Free(selectedVars);
    Ddi_Free(psv);
    break;
  default:
    Pdtutil_Assert(0,"unknown or not supported meta BDD method");
    break;
  }
  
  ddm->meta.groups = Ddi_VarsetarrayAlloc(ddm,0); 
  Ddi_Lock(ddm->meta.groups);

  metaGrp = Ddi_VarsetVoid(ddm);
  min = ddm->settings.meta.groupSizeMin;
  if (ddm->meta.groupNum >0 && 
      method == Ddi_Meta_FilterVars && firstGroupOrFilter != NULL) {
    ddm->meta.groupNum = nGroupVars/min + 1;
  }
  for (i=Ddi_VarsetarrayNum(groups)-1; i>=0; i--) {
    Ddi_VarsetUnionAcc(metaGrp,Ddi_VarsetarrayRead(groups,i));
    if (Ddi_VarsetNum(metaGrp) >= min) {
      if ((ddm->meta.groupNum <= 0) || 
          (Ddi_VarsetarrayNum(ddm->meta.groups) < ddm->meta.groupNum-1)) {
        Ddi_VarsetarrayInsert(ddm->meta.groups,0,metaGrp);
        Ddi_Free(metaGrp);
        metaGrp = Ddi_VarsetVoid(ddm);
        min *= 1.0;
      }
    }
  }
  if (!Ddi_VarsetIsVoid(metaGrp)) {
    Ddi_VarsetarrayInsert(ddm->meta.groups,0,metaGrp);
  }

  Pdtutil_Free(used);
  Ddi_Free(metaGrp);
  Ddi_Free(groups);

  if (ddm->meta.groupNum >0) {
    while (Ddi_VarsetarrayNum(ddm->meta.groups) < ddm->meta.groupNum) {
      metaGrp = Ddi_VarsetVoid(ddm);
      Ddi_VarsetarrayInsertLast(ddm->meta.groups,metaGrp);
      Ddi_Free(metaGrp);
    }
  } 

  if (method == Ddi_Meta_FilterVars && firstGroupOrFilter != NULL) {
    for (i=Ddi_VarsetarrayNum(ddm->meta.groups)-1; i>=0; i--) {
      if (Ddi_VarsetIsVoid(Ddi_VarsetarrayRead(ddm->meta.groups,i))) {
        Ddi_VarsetarrayRemove(ddm->meta.groups,i);
      }
    }
  }

  ddm->meta.groupNum = Ddi_VarsetarrayNum(ddm->meta.groups);

  if (0 ||Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMax_c) {
    int i;
    printf("Meta-Groups\n");
    printf ("Meta Variable Groups:\n");
    for (i=0; i<Ddi_VarsetarrayNum(ddm->meta.groups); i++){
      printf("Group[%d] (size: %d)\n",
	     i, Ddi_VarsetNum(Ddi_VarsetarrayRead(ddm->meta.groups,i)));
      Ddi_VarsetPrint(
        Ddi_VarsetarrayRead(ddm->meta.groups,i), 20, NULL, stdout);
    }
  }


}

/**Function********************************************************************
  Synopsis    [Close Meta BDD handling in DDI manager]
  Description [Close Meta BDD handling in DDI manager. This enables further
    opening of Meta BDD management with different method/parameters]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_MetaQuit (
  Ddi_Mgr_t *ddm
)
{
  if (ddm->meta.groups != NULL) {

    Ddi_Unlock(ddm->meta.groups);
    Ddi_Free(ddm->meta.groups);
    Pdtutil_Free(ddm->meta.ord);
    ddm->meta.nvar = 0;
    ddm->meta.groupNum = 0;
    /* check if Meta BDDs are still present (not freed) */
    if (ddm->meta.bddNum != 0) {
      char buf[300];
      sprintf(buf, 
        "%d Non freed Meta BDDs present when quitting Meta handling\n",
        ddm->meta.bddNum);
      strcat(buf,"This may cause wrong Meta BDD operations\n");
      strcat(buf,"with new Meta BDDs\n");
      strcat(buf,"if meta Handling re-opened with different settings");
      Pdtutil_Warning(1,buf);
    }
  }
}

/**Function********************************************************************
  Synopsis    [Transform a BDD to Meta BDD. Result generated]
  Description [Transform a BDD to Meta BDD. Result generated]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeMeta (
  Ddi_Bdd_t *f
)
{
  return((Ddi_Bdd_t *)MetaConvert(
    DdiGenericDup((Ddi_Generic_t *)f),Bdd2Meta_c));
}

/**Function********************************************************************
  Synopsis    [Transform a BDD to Meta BDD. Result accumulated]
  Description [Transform a BDD to Meta BDD. Result accumulated]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddSetMeta (
  Ddi_Bdd_t *f
)
{
  return((Ddi_Bdd_t *)MetaConvert((Ddi_Generic_t *)f,Bdd2Meta_c));
}


/**Function********************************************************************
  Synopsis    [Transform a BDD array to Meta BDD. Result generated]
  Description [Transform a BDD array to Meta BDD. Result generated]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayMakeMeta (
  Ddi_Bddarray_t *f
)
{
  return((Ddi_Bddarray_t *)MetaConvert(
    DdiGenericDup((Ddi_Generic_t *)f),Bdd2Meta_c));
}

/**Function********************************************************************
  Synopsis    [Transform a BDD array to Meta BDD. Result accumulated]
  Description [Transform a BDD array to Meta BDD. Result accumulated]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddArraySetMeta (
  Ddi_Bddarray_t *f
)
{
  return((Ddi_Bddarray_t *)MetaConvert((Ddi_Generic_t *)f,Bdd2Meta_c));
}

/**Function********************************************************************
  Synopsis    [Apply top-down reduction of ones to bottom layer]
  Description [Apply top-down reduction of ones to bottom layer. This reduces
    a Meta BDD to a conjunctive form]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMetaBfvSetConj(
  Ddi_Bdd_t * f
)
{
  int i, j, ng;
  Ddi_Bdd_t *zero, *zero_j, *one, *dc, *dc_j, *dcnew, *dccof, *zeroConst;
  Ddi_Mgr_t *ddiMgr;
  Ddi_Meta_t *m;

  DdiConsistencyCheck(f, Ddi_Bdd_c);
  Pdtutil_Assert(f->common.code == Ddi_Bdd_Meta_c,
    "Operation requires Meta BDD!");

  m = f->data.meta;
  ddiMgr = Ddi_ReadMgr(f);
  ng = Ddi_BddarrayNum(m->one);
  zeroConst = Ddi_MgrReadZero(ddiMgr);

  Pdtutil_Assert(ng == ddiMgr->meta.groupNum,
    "Wrong number of groups in Meta BDD");

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "Meta-Set-Conj\nInitial size: %d.\n", Ddi_BddSize(f)));

  for (i = 0; i < ng - 1; i++) {

    zero = Ddi_BddarrayRead(m->zero, i);
    dc = Ddi_BddarrayRead(m->dc, i);

    /* one is pushed down. So new dc is !zero */
    dcnew = Ddi_BddNot(zero);
    dccof = Ddi_BddConstrain(dc, dcnew);

    Ddi_BddarrayWrite(m->one, i, zeroConst);
    Ddi_BddarrayWrite(m->dc, i, dcnew);

    for (j = i + 1; j < ng; j++) {
      zero_j = Ddi_BddConstrain(Ddi_BddarrayRead(m->zero, j), dcnew);
      Ddi_BddAndAcc(zero_j, dccof);
      dc_j = Ddi_BddConstrain(Ddi_BddarrayRead(m->dc, j), dcnew);
      Ddi_BddAndAcc(dc_j, dccof);
      Ddi_BddarrayWrite(m->zero, j, zero_j);
      Ddi_BddarrayWrite(m->dc, j, dc_j);
      Ddi_Free(zero_j);
      Ddi_Free(dc_j);
    }

    Ddi_Free(dcnew);
    Ddi_Free(dccof);

  }
  zero = Ddi_BddarrayRead(m->zero, ng - 1);
  one = Ddi_BddNot(zero);
  Ddi_BddarrayWrite(m->dc, ng - 1, zeroConst);
  Ddi_BddarrayWrite(m->one, ng - 1, one);
  Ddi_Free(one);

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "Final size: %d.\n", Ddi_BddSize(f)));

  return f;
}

/**Function********************************************************************
  Synopsis    [Transform a BDD to Meta BDD. Result generated]
  Description [Transform a BDD to Meta BDD. Result generated]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeFromMeta (
  Ddi_Bdd_t *f
)
{
  return((Ddi_Bdd_t *)MetaConvert(
    DdiGenericDup((Ddi_Generic_t *)f),Meta2Bdd_c));
}

/**Function********************************************************************
  Synopsis    [Transform a BDD to Meta BDD. Result generated]
  Description [Transform a BDD to Meta BDD. Result generated]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_AigMakeFromMeta (
  Ddi_Bdd_t *f
)
{
  return((Ddi_Bdd_t *)MetaConvert(
    DdiGenericDup((Ddi_Generic_t *)f),Meta2Aig_c));
}

/**Function********************************************************************
  Synopsis    [Transform a BDD to Meta BDD. Result accumulated]
  Description [Transform a BDD to Meta BDD. Result accumulated]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddFromMeta (
  Ddi_Bdd_t *f
)
{
  return((Ddi_Bdd_t *)MetaConvert((Ddi_Generic_t *)f,Meta2Bdd_c));
}
/**Function********************************************************************
  Synopsis    [Transform a BDD to Meta BDD. Result accumulated]
  Description [Transform a BDD to Meta BDD. Result accumulated]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_AigFromMeta (
  Ddi_Bdd_t *f
)
{
  return((Ddi_Bdd_t *)MetaConvert((Ddi_Generic_t *)f,Meta2Aig_c));
}

/**Function********************************************************************
  Synopsis    [Transform a BDD to Meta BDD. Result accumulated]
  Description [Transform a BDD to Meta BDD. Result accumulated]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddSetPartConjFromMeta (
  Ddi_Bdd_t *f
)
{
  return((Ddi_Bdd_t *)MetaConvert((Ddi_Generic_t *)f,Meta2Cpart_c));
}

/**Function********************************************************************
  Synopsis    [Transform a BDD to Meta BDD. Result accumulated]
  Description [Transform a BDD to Meta BDD. Result accumulated]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakePartConjFromMeta (
  Ddi_Bdd_t *f
)
{
  Ddi_Bdd_t *r = Ddi_BddDup(f);
  return((Ddi_Bdd_t *)MetaConvert((Ddi_Generic_t *)r,Meta2Cpart_c));
}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Transform between generic DDI node and Meta BDD. 
    Result accumulated]
  Description [Transform between generic DDI node and Meta BDD. 
    Result accumulated. The transformation is applied to leaf BDDs, 
    whereas the DDI structure is kept as it is]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Generic_t *
MetaConvert (
  Ddi_Generic_t *f,
  Meta_Convert_e sel
)
{
  Ddi_Mgr_t *ddm; 
  Ddi_ArrayData_t *array;
  int i;

  ddm = f->common.mgr;

  switch (f->common.type) {

  case Ddi_Bdd_c:
    switch (f->common.code) {
    case Ddi_Bdd_Aig_c:
      {
	int i;
        Ddi_Bdd_t *fPart = Ddi_AigPartitionTop((Ddi_Bdd_t *)f,0);
	if (Ddi_BddPartNum(fPart)==0) {
	  Ddi_BddSetMono((Ddi_Bdd_t *)f);
	  MetaConvertBdd(f,sel);
	}
	else {
	  for (i=0; i<Ddi_BddPartNum(fPart); i++) {
	    Ddi_BddSetMono(Ddi_BddPartRead(fPart,i));
  	  }
          MetaConvert((Ddi_Generic_t *)fPart,sel);
	  Ddi_DataCopy(f,fPart);
	}
	Ddi_Free(fPart);
      }
      break;
    case Ddi_Bdd_Mono_c:
    case Ddi_Bdd_Meta_c:
      MetaConvertBdd(f,sel);
      break;
    case Ddi_Bdd_Part_Conj_c:
    case Ddi_Bdd_Part_Disj_c:
      array = f->Bdd.data.part;
#if 0
      for (i=0;i<DdiArrayNum(array);i++) {
        MetaConvertBdd(DdiArrayRead(array,i),sel);
      }
      /*Ddi_BddSetMono((Ddi_Bdd_t *)f);*/
#else
      MetaConvertBdd(DdiArrayRead(array,0),sel);
      {      
        Ddi_Varset_t *noVars = Ddi_VarsetVoid(ddm);
        Ddi_BddExistAcc((Ddi_Bdd_t *)f,noVars);
        Ddi_Free(noVars);
      }
#endif
      break;
    default:
      Pdtutil_Assert (0, "Wrong DDI node type");
    }
    break;
  case Ddi_Var_c:
  case Ddi_Vararray_c:
    Pdtutil_Assert (0, "Variables cannot be transformed to meta");
    break;
  case Ddi_Varset_c:
  case Ddi_Varsetarray_c:
    Pdtutil_Assert (0, "Varsets cannot be transformed to meta");
    break;
  case Ddi_Expr_c:
    switch (f->common.code) {
    case Ddi_Expr_Bdd_c:
      MetaConvertBdd(f->Expr.data.bdd,sel);
      break;
    case Ddi_Expr_String_c:
      break;
    case Ddi_Expr_Bool_c:
    case Ddi_Expr_Ctl_c:
      array = f->Expr.data.sub;
      for (i=0;i<DdiArrayNum(array);i++) {
        MetaConvertBdd(DdiArrayRead(array,i),sel);
      }
      break;
    default:
      Pdtutil_Assert (0, "Wrong DDI node type");
      break;
    }
    break;
  case Ddi_Bddarray_c:
  case Ddi_Exprarray_c:
    array = f->Bddarray.array;
    for (i=0;i<DdiArrayNum(array);i++) {
      MetaConvertBdd(DdiArrayRead(array,i),sel);
    }
    break;
  default:
    Pdtutil_Assert (0, "Wrong DDI node type");
  }

  return (f);
}

/**Function********************************************************************
  Synopsis    [Conversion between BDD and Meta BDD]
  Description [Conversion between BDD and Meta BDD]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
MetaConvertBdd (
  Ddi_Generic_t *f,
  Meta_Convert_e sel
)
{
  switch (sel) {
  case Meta2Aig_c:
    if (f->common.code == Ddi_Bdd_Meta_c) {
      MetaToAig((Ddi_Bdd_t *)f);
    }
    else {
       /* Pdtutil_Warning(1,"BDD is already in BDD format"); */
    }
    break;
  case Meta2Bdd_c:
    if (f->common.code == Ddi_Bdd_Meta_c) {
      MetaToMono((Ddi_Bdd_t *)f);
    }
    else {
       /* Pdtutil_Warning(1,"BDD is already in BDD format"); */
    }
    break;
  case Meta2Cpart_c:
    if (f->common.code == Ddi_Bdd_Meta_c) {
      MetaToCpart((Ddi_Bdd_t *)f);
    }
    else {
       /* Pdtutil_Warning(1,"BDD is already in BDD format"); */
    }
    break;
  case Bdd2Meta_c:
    if (f->common.code == Ddi_Bdd_Mono_c) {
      MetaFromMono((Ddi_Bdd_t *)f);
    }
    else {
       /* Pdtutil_Warning(1,"BDD is already in Meta format"); */
    }
    break;
  default:
    Pdtutil_Assert(0,"invalid conversion selection btw. BDD and Meta");
  }
}

/**Function********************************************************************
  Synopsis    [Transform a BDD to Meta BDD]
  Description [Transform a BDD to Meta BDD]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
MetaFromMono (
  Ddi_Bdd_t *f
)
{
  int ng, i;
  Ddi_Meta_t *m;
  Ddi_Mgr_t *ddm;
  Ddi_Bdd_t *one, *zero;
  Ddi_Bdd_t *fBdd;

  ddm = Ddi_ReadMgr(f);
  ng = ddm->meta.groupNum;
  one = Ddi_MgrReadOne(ddm);
  zero = Ddi_MgrReadZero(ddm);

  fBdd = Ddi_BddDup(f);

  m = Pdtutil_Alloc(Ddi_Meta_t,1);
  m->one = Ddi_BddarrayAlloc(ddm,ng);
  m->zero = Ddi_BddarrayAlloc(ddm,ng);
  m->dc = Ddi_BddarrayAlloc(ddm,ng);
  m->metaId = ddm->meta.id;

  for (i=0;i<ng-1;i++) {
    Ddi_BddarrayWrite(m->one,i,zero);
    Ddi_BddarrayWrite(m->zero,i,zero);
    Ddi_BddarrayWrite(m->dc,i,one);
  }

  Ddi_BddarrayWrite(m->one,ng-1,f);
  Ddi_BddarrayWrite(m->zero,ng-1,Ddi_BddNotAcc(f));
  Ddi_BddNotAcc(f);
  Ddi_BddarrayWrite(m->dc,ng-1,zero);

  Ddi_Lock(m->one);
  Ddi_Lock(m->zero);
  Ddi_Lock(m->dc);

  Cudd_RecursiveDeref (Ddi_MgrReadMgrCU(ddm),f->data.bdd);
  f->common.code = Ddi_Bdd_Meta_c;
  f->data.meta = m;

  /* apply bottom-up reduction */
  MetaReduce(f,-1);

  /* CHECK_META(f,fBdd); */

  MetaSimplify(f,0,ng-1);

  /* CHECK_META(f,fBdd); */

#if 0

  MetaSetConj(f);
  CHECK_META(f,fBdd);
#endif

  Ddi_Free(fBdd);

}

/**Function********************************************************************
  Synopsis    [Apply bottom-up reduction process to meta BDD]
  Description [Apply bottom-up reduction process to meta BDD]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
MetaReduce (
  Ddi_Bdd_t *f,
  int        init  /* start from this layer: < 0 for full reduction */
)
{
  int i, j, k, ng, oldSize, newSize, sizeJoin, noReduce, aborted;
  Ddi_Bdd_t *zero, *one, *dc, *tmp, *tmp2, *dcup, *zeroup, *oneup, 
            *onejoin, *zerojoin, *dcjoin;
  Ddi_Varset_t *smooth;
  Ddi_Mgr_t *ddm; 
  Ddi_Meta_t *m;
  Ddi_Bddarray_t *auxReduce, *auxJoin;
  Pdtutil_VerbLevel_e verbosity;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  Pdtutil_Assert(f->common.code == Ddi_Bdd_Meta_c,
    "Operation requires Meta BDD!");

  ddm = Ddi_ReadMgr(f);
  if (MetaUpdate(ddm,1)) {
    printf("START UPDATE (reduce)\n");
    MetaAlign (f,NULL);
    printf("\nEND UPDATE (reduce)\n");
    return;
  }

  verbosity = Ddi_MgrReadVerbosity(ddm);

  Ddi_MgrSiftSuspend(ddm); 

  m = f->data.meta;
  ng = Ddi_BddarrayNum(m->one);

  if (ng==1) {
    return;
  }

  while (ng > ddm->meta.groupNum) {
    Ddi_Varset_t *voidGrp;
    ddm->meta.groupNum++;
    voidGrp = Ddi_VarsetVoid(ddm);
    Ddi_VarsetarrayInsertLast(ddm->meta.groups,voidGrp);
    Ddi_Free(voidGrp);
  }
  while (ng < ddm->meta.groupNum) {
    Ddi_BddarrayWrite(m->dc,ng,Ddi_MgrReadOne(ddm));
    Ddi_BddarrayWrite(m->one,ng,Ddi_MgrReadZero(ddm));
    Ddi_BddarrayWrite(m->zero,ng,Ddi_MgrReadZero(ddm));
    ng++;
  }

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMax_c) {
    printf("Meta-Reduce\nInitial size: %d\n",Ddi_BddSize(f));
  }

  smooth = Ddi_VarsetVoid(ddm);

  Pdtutil_Assert(ng>1,"At least 2 groups required for meta reduce");

#if 0
  for (i=0; i<ng; i++) {
    Ddi_BddSetMono(Ddi_BddarrayRead(m->dc,i));
    Ddi_BddSetMono(Ddi_BddarrayRead(m->one,i));
    Ddi_BddSetMono(Ddi_BddarrayRead(m->zero,i));
  }
#endif
  for (j=ng-2; j>=0; j--) {

    if (ddm->settings.meta.method == Ddi_Meta_Size_Window) {
      Ddi_Free(smooth);
      smooth = Ddi_VarsetDup(Ddi_VarsetarrayRead(ddm->meta.groups,j+1));
    }
    else {
      Ddi_VarsetUnionAcc(smooth,Ddi_VarsetarrayRead(ddm->meta.groups,j+1));
    }
    if ((init>=0)&&(j>=init)) {
      continue;
    }

    i = j+1;
    while ((i<ng-1)&&(Ddi_BddIsOne(Ddi_BddarrayRead(m->dc,i)))) {
      i++;
    }

    if (Ddi_BddIsOne(Ddi_BddarrayRead(m->one,i))) {
      continue;
    }

    noReduce = 1;

    auxReduce = Ddi_BddarrayAlloc(ddm,4);

    one = Ddi_BddarrayRead(m->one,i);
    zero = Ddi_BddarrayRead(m->zero,i);
    dc = Ddi_BddarrayRead(m->dc,i);

    oneup = Ddi_BddarrayRead(m->one,j);
    zeroup = Ddi_BddarrayRead(m->zero,j);

#if 0
    Ddi_BddSetMono(one);
    Ddi_BddSetMono(zero);
    Ddi_BddSetMono(dc);
    Ddi_BddSetMono(oneup);
    Ddi_BddSetMono(zeroup);
#endif

    Ddi_BddarrayWrite(auxReduce,0,one);
    Ddi_BddarrayWrite(auxReduce,1,zero);
    Ddi_BddarrayWrite(auxReduce,2,oneup);
    Ddi_BddarrayWrite(auxReduce,3,zeroup);
    oldSize = Ddi_BddarraySize(auxReduce);
    
    if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMax_c) {
      printf("Meta-Reduce[%d] (%d|%d|%d) -> ",i,
        Ddi_BddSize(one),Ddi_BddSize(zero),Ddi_BddSize(dc)); 
    }

    dcup = Ddi_BddarrayRead(m->dc,j);
#if 0
    Ddi_BddSetMono(dcup);
#endif

    /*Ddi_MgrAbortOnSiftEnable(ddm);*/
    tmp = MetaForall(one,smooth);

    aborted = ddm->abortedOp;
    /*Ddi_MgrAbortOnSiftDisable(ddm);*/
    noReduce = noReduce && Ddi_BddIsZero(tmp);

    if (aborted|| !Ddi_BddIsMono(tmp) || !Ddi_BddIsMono(dcup) || 
        !Ddi_BddIsMono(Ddi_BddarrayRead(m->one,j))) {
      oneup = Ddi_BddDup(Ddi_BddarrayRead(m->one,j));
    }
    else {
      Ddi_MgrAbortOnSiftEnable(ddm);
      oneup = Ddi_BddIte(dcup,tmp,Ddi_BddarrayRead(m->one,j));
      aborted = ddm->abortedOp;
      Ddi_MgrAbortOnSiftDisable(ddm);
      if (aborted) {
        Ddi_Free(oneup);
        oneup = Ddi_BddDup(Ddi_BddarrayRead(m->one,j));
      }
      else {
        for (k=0; k<j; k++) {
          tmp2 = Ddi_BddarrayRead(m->dc,k);
          oneup = Ddi_BddEvalFree(MetaConstrainOpt(oneup,tmp2),oneup);
        }
      }
    }
    Ddi_Free(tmp);

    /*Ddi_MgrAbortOnSiftEnable(ddm);*/
    tmp = MetaForall(zero,smooth);
    aborted = ddm->abortedOp;
    /*Ddi_MgrAbortOnSiftDisable(ddm);*/

    noReduce = noReduce && Ddi_BddIsZero(tmp);

    if (aborted|| !Ddi_BddIsMono(tmp) || !Ddi_BddIsMono(dcup) || 
        !Ddi_BddIsMono(Ddi_BddarrayRead(m->zero,j))) {
      zeroup = Ddi_BddDup(Ddi_BddarrayRead(m->zero,j));
    }
    else {
      Ddi_MgrAbortOnSiftEnable(ddm);
      zeroup = Ddi_BddIte(dcup,tmp,Ddi_BddarrayRead(m->zero,j));
      aborted = ddm->abortedOp;
      Ddi_MgrAbortOnSiftDisable(ddm);
      if (aborted) {
        Ddi_Free(zeroup);
        zeroup = Ddi_BddDup(Ddi_BddarrayRead(m->zero,j));
      }
      else {
        for (k=0; k<j; k++) {
          Ddi_Bdd_t *dck;
          dck = Ddi_BddarrayRead(m->dc,k);
          zeroup = Ddi_BddEvalFree(MetaConstrainOpt(zeroup,dck),zeroup);
        }
      }
    }
    Ddi_Free(tmp);

    dcup = Ddi_BddNotAcc(Ddi_BddOr(oneup,zeroup));

    zero = MetaConstrainOpt(zero,dcup);
    one = MetaConstrainOpt(one,dcup);
    dc = Ddi_BddNotAcc(Ddi_BddOr(one,zero));

    Ddi_BddarrayWrite(auxReduce,0,one);
    Ddi_BddarrayWrite(auxReduce,1,zero);
    Ddi_BddarrayWrite(auxReduce,2,oneup);
    Ddi_BddarrayWrite(auxReduce,3,zeroup);
    newSize = Ddi_BddarraySize(auxReduce);
    Ddi_Free(auxReduce);
    
#if 1
    if ((!noReduce)&&(newSize < 0.99*oldSize)) {
#else
    if (1) {
#endif

      Ddi_BddarrayWrite(m->one,j,oneup);
      Ddi_BddarrayWrite(m->zero,j,zeroup);
      Ddi_BddarrayWrite(m->dc,j,dcup);

      Ddi_BddarrayWrite(m->one,i,one);
      Ddi_BddarrayWrite(m->zero,i,zero);
      Ddi_BddarrayWrite(m->dc,i,dc);

      if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMax_c) {
        printf ("  1: %5d - 0: %5d - dc: %5d \n",
          Ddi_BddSize(one),Ddi_BddSize(zero),Ddi_BddSize(dc));
      }
      else if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
        printf("-");fflush(stdout);
      }

      Ddi_Free(oneup);
      Ddi_Free(zeroup);
      Ddi_Free(dcup);
      Ddi_Free(one);
      Ddi_Free(zero);
      Ddi_Free(dc);

    }
    else {

      Ddi_Free(one);
      Ddi_Free(zero);
      Ddi_Free(dc);
      Ddi_Free(oneup);
      Ddi_Free(zeroup);
      Ddi_Free(dcup);

      dcup = Ddi_BddarrayRead(m->dc,j);
      one = Ddi_BddarrayRead(m->one,i);
      zero = Ddi_BddarrayRead(m->zero,i);
      dc = Ddi_BddarrayRead(m->dc,i);

      if (Ddi_BddIsMono(dcup)&&Ddi_BddIsMono(one)&&
          Ddi_BddIsMono(Ddi_BddarrayRead(m->one,j))) {
        Ddi_MgrAbortOnSiftEnable(ddm);
        onejoin = Ddi_BddIte(dcup,one,Ddi_BddarrayRead(m->one,j));
        if (ddm->abortedOp) {
          Ddi_Free(onejoin);
          onejoin = NULL;
        }
        Ddi_MgrAbortOnSiftDisable(ddm);
      }
      else {
        onejoin = NULL;
      }
      if (Ddi_BddIsMono(dcup)&&Ddi_BddIsMono(zero)&&
          Ddi_BddIsMono(Ddi_BddarrayRead(m->zero,j))) {
        Ddi_MgrAbortOnSiftEnable(ddm);
        zerojoin = Ddi_BddIte(dcup,zero,Ddi_BddarrayRead(m->zero,j));
        if (ddm->abortedOp) {
          Ddi_Free(zerojoin);
          zerojoin = NULL;
        }
        Ddi_MgrAbortOnSiftDisable(ddm);
      }
      else {
        zerojoin = NULL;
      }
      
      sizeJoin = 2*oldSize;
      if ((onejoin != NULL)&&(zerojoin != NULL)) {
        auxJoin = Ddi_BddarrayAlloc(ddm,2);
        dcjoin = Ddi_BddNotAcc(Ddi_BddOr(onejoin,zerojoin));
        Ddi_BddarrayWrite(auxJoin,0,onejoin);
        Ddi_BddarrayWrite(auxJoin,1,zerojoin);
        sizeJoin = Ddi_BddarraySize(auxJoin);
        Ddi_Free(auxJoin);
      }
      else {
        dcjoin = NULL;
      }

      if (/*(!noReduce)&&*/(sizeJoin < 1.01*oldSize)) {
        Ddi_BddarrayWrite(m->one,i,onejoin);
        Ddi_BddarrayWrite(m->zero,i,zerojoin);
        Ddi_BddarrayWrite(m->dc,i,dcjoin);

        Ddi_BddarrayWrite(m->one,j,Ddi_MgrReadZero(ddm));
        Ddi_BddarrayWrite(m->zero,j,Ddi_MgrReadZero(ddm));
        Ddi_BddarrayWrite(m->dc,j,Ddi_MgrReadOne(ddm));

        if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
          printf("_");fflush(stdout);
        }
      }
      else {
        if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
          printf(".");fflush(stdout);
        }
      }

      Ddi_Free(onejoin);
      Ddi_Free(zerojoin);
      Ddi_Free(dcjoin);

    }


  }

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMax_c) {
    printf("Meta-Reduce[%d] (%d|%d|%d) ",0,
      Ddi_BddSize(Ddi_BddarrayRead(m->one,0)),
      Ddi_BddSize(Ddi_BddarrayRead(m->zero,0)),
      Ddi_BddSize(Ddi_BddarrayRead(m->dc,0))); 
  }

  Ddi_Free(smooth);

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMax_c) {
    printf("Final size: %d\n",Ddi_BddSize(f));
  }

#if 0
  MetaSimplifyUp(f);

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMax_c) {
    fprintf (stdout, "<%d>->", Ddi_BddSize(f));
    fflush(stdout);
  }
#endif

  Ddi_MgrSiftResume(ddm);
  /*
  MetaAlignLayers(f);
  */
}

/**Function********************************************************************
  Synopsis    [Apply top-down cofactor based simplification]
  Description [Apply top-down cofactor based simplification]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
MetaSimplify (
  Ddi_Bdd_t *f,
  int  init  /* apply simplification starting from this layer */,
  int  end
)
{
  int i, ng, limit;
  Ddi_Bdd_t *dc, *zero_i, *one_i, *dc_i;
  Ddi_Mgr_t *ddm; 
  Ddi_Meta_t *m;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  Pdtutil_Assert(f->common.code == Ddi_Bdd_Meta_c,
    "Operation requires Meta BDD!");

  m = f->data.meta;
  ddm = Ddi_ReadMgr(f);
  ng = Ddi_BddarrayNum(m->one);
  limit = META_MIN(end,ng-1);

  Pdtutil_Assert(ng == ddm->meta.groupNum,
    "Wrong number of groups in Meta BDD");
  Pdtutil_Assert(init>=0,
    "Wrong initial layer in meta-simplify");

  if (init>=limit) 
    return;

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMax_c) {
    printf("Meta-Simplify (init: %d)\nInitial size: %d\n",
      init,Ddi_BddSize(f));
  }

  dc = Ddi_BddarrayRead(m->dc,init);

  /* Ddi_MgrSiftSuspend(ddm); */

  for (i=init+1; i<=limit; i++) {
    Ddi_Bdd_t *tmp;
    int size;

    one_i = Ddi_BddarrayRead(m->one,i);
    zero_i = Ddi_BddarrayRead(m->zero,i);
    dc_i = Ddi_BddarrayRead(m->dc,i);

    if (Ddi_BddIsOne(dc_i)) {
      continue;
    }
    
    tmp = Ddi_BddMakePartConjVoid(ddm);
    Ddi_BddPartInsert(tmp,0,one_i);
    Ddi_BddPartInsert(tmp,1,zero_i);
    Ddi_BddPartInsert(tmp,2,dc_i);
    size = Ddi_BddSize(tmp);
    /* Ddi_BddConstrainAcc(tmp,dc); */                                 
    tmp = Ddi_BddEvalFree(MetaConstrainOpt(tmp,dc),tmp); 
    if (Ddi_BddSize(tmp) < size) {
      one_i = Ddi_BddPartRead(tmp,0);
      Ddi_BddarrayWrite(m->one,i,one_i);
      zero_i = Ddi_BddPartRead(tmp,1);
      Ddi_BddarrayWrite(m->zero,i,zero_i);
      dc_i = Ddi_BddPartRead(tmp,2);
      Ddi_BddarrayWrite(m->dc,i,dc_i);
    }
    Ddi_Free(tmp);

  }

  /* Ddi_MgrSiftResume(ddm); */

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMax_c) {
    printf("Meta-Simplify (init: %d)\nFinal size: %d\n",
      init,Ddi_BddSize(f));
  }

  MetaSimplify (f,init+1,end);

}

#if META_CONST

/**Function********************************************************************
  Synopsis    [Apply top-down cofactor based simplification]
  Description [Apply top-down cofactor based simplification]
  SideEffects []
  SeeAlso     []
******************************************************************************/

static void
MetaSimplifyUp (
  Ddi_Bdd_t *f
  )
{
  int i, k, ng;
  Ddi_Mgr_t *ddm; 
  Ddi_Meta_t *m;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  Pdtutil_Assert(f->common.code == Ddi_Bdd_Meta_c,
    "Operation requires Meta BDD!");

  MetaSetConj(f);

  m = f->data.meta;
  ddm = Ddi_ReadMgr(f);
  ng = Ddi_BddarrayNum(m->one);

  Pdtutil_Assert(ng == ddm->meta.groupNum,
    "Wrong number of groups in Meta BDD");

  for (i=ng-1; i>0; i--) {
    Ddi_Bdd_t *nzi = Ddi_BddNot(Ddi_BddarrayRead(m->zero,i));
    for (k=0; k<i; k++) {
      Ddi_Bdd_t *zk;
      zk = MetaConstrainOpt(Ddi_BddarrayRead(m->zero,k),nzi);
      Ddi_BddarrayWrite(m->zero,k,zk);
      Ddi_Free(zk);
    }
    Ddi_Free(nzi);
  }

}

#endif

/**Function********************************************************************
  Synopsis    [Apply top-down reduction of ones to bottom layer]
  Description [Apply top-down reduction of ones to bottom layer. This reduces
    a Meta BDD to a conjunctive form]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
MetaSetConj (
  Ddi_Bdd_t *f
)
{
  int i, j, ng;
  Ddi_Bdd_t *zero, *zero_j, *one, *dc, *dc_j, *dcnew, *dccof, *zeroConst;
  Ddi_Mgr_t *ddm; 
  Ddi_Meta_t *m;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  Pdtutil_Assert(f->common.code == Ddi_Bdd_Meta_c,
    "Operation requires Meta BDD!");

  m = f->data.meta;
  ddm = Ddi_ReadMgr(f);
  ng = Ddi_BddarrayNum(m->one);
  zeroConst = Ddi_MgrReadZero(ddm);

  Pdtutil_Assert(ng == ddm->meta.groupNum,
    "Wrong number of groups in Meta BDD");

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMax_c) {
    printf("Meta-Set-Conj\nInitial size: %d\n",Ddi_BddSize(f));
  }

  for (i=0; i<ng-1; i++) {
    if (!Ddi_BddIsZero(Ddi_BddarrayRead(m->one,i)))
      break;
  }
  for (; i<ng-1; i++) {

    zero = Ddi_BddarrayRead(m->zero,i);
    dc = Ddi_BddarrayRead(m->dc,i);

    /* one is pushed down. So new dc is !zero */
    dcnew = Ddi_BddNot(zero);
    dccof = Ddi_BddConstrain(dc,dcnew);

    Ddi_BddarrayWrite(m->one,i,zeroConst);
    Ddi_BddarrayWrite(m->dc,i,dcnew);

    for (j=i+1; j<ng; j++) {
      zero_j = MetaConstrainOpt(Ddi_BddarrayRead(m->zero,j),dcnew);
      Ddi_BddAndAcc(zero_j,dccof);
      dc_j = MetaConstrainOpt(Ddi_BddarrayRead(m->dc,j),dcnew);
      Ddi_BddAndAcc(dc_j,dccof);
      Ddi_BddarrayWrite(m->zero,j,zero_j);
      Ddi_BddarrayWrite(m->dc,j,dc_j);
      Ddi_Free(zero_j);
      Ddi_Free(dc_j);
    }

    Ddi_Free(dcnew);
    Ddi_Free(dccof);

  }
  zero = Ddi_BddarrayRead(m->zero,ng-1);
  one = Ddi_BddNot(zero);
  Ddi_BddarrayWrite(m->dc,ng-1,zeroConst);
  Ddi_BddarrayWrite(m->one,ng-1,one);
  Ddi_Free(one);

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMax_c) {
    printf("Final size: %d\n",Ddi_BddSize(f));
  }

}



/**Function********************************************************************
  Synopsis    [return true if Meta BDD is conjoined]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
MetaIsConj (
  Ddi_Bdd_t *f
)
{
  int i, ng, isConj;
  Ddi_Mgr_t *ddm; 
  Ddi_Meta_t *m;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  Pdtutil_Assert(f->common.code == Ddi_Bdd_Meta_c,
    "Operation requires Meta BDD!");

  m = f->data.meta;
  ddm = Ddi_ReadMgr(f);
  ng = Ddi_BddarrayNum(m->one);

  Pdtutil_Assert(ng == ddm->meta.groupNum,
    "Wrong number of groups in Meta BDD");

  isConj = 1;

  for (i=0; i<ng-1; i++) {
    if (!Ddi_BddIsZero(Ddi_BddarrayRead(m->one,i)))
      isConj = 0;
      break;
  }

  return(isConj);

}

#if META_CONST

/**Function********************************************************************
  Synopsis    [move Meta components to proper layer]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

static void
MetaAlignLayers (
  Ddi_Bdd_t *f
  )
{
  int i, j, ng;
  Ddi_Bdd_t *zero, *one, *dc;
  Ddi_Varset_t *layer, *supp, *aux;
  Ddi_Mgr_t *ddm; 
  Ddi_Meta_t *m;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  Pdtutil_Assert(f->common.code == Ddi_Bdd_Meta_c,
    "Operation requires Meta BDD!");

  m = f->data.meta;
  ddm = Ddi_ReadMgr(f);
  ng = Ddi_BddarrayNum(m->one);

  Pdtutil_Assert(ng == ddm->meta.groupNum,
    "Wrong number of groups in Meta BDD");

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMax_c) {
    printf("Meta-Align-Layers\n");
  }

#if 0
  printf("\n<<");
  for (i=ng-1; i>=0; i--) {
    if (i<ng-1) {
      dc = Ddi_BddarrayRead(m->dc,i);
    }
    else {
      dc = Ddi_BddarrayRead(m->one,i);
    }
    if (Ddi_BddIsOne(dc))
      printf(".");
    else
      printf("*");
  }
  printf("\n");
#endif
  for (i=ng-2; i>0; i--) {

    dc = Ddi_BddarrayRead(m->dc,i);

    if (!Ddi_BddIsOne(dc))
      continue;

    layer = Ddi_VarsetarrayRead(ddm->meta.groups,i);

    for (j=i-1; j>=0; j--) {
      zero = Ddi_BddarrayRead(m->zero,j);
      one = Ddi_BddarrayRead(m->one,j);
      dc = Ddi_BddarrayRead(m->dc,j);
      if (!Ddi_BddIsOne(dc)) {

        supp = Ddi_BddSupp(dc);
        aux = Ddi_BddSupp(one);
        Ddi_VarsetUnionAcc(supp,aux);
        Ddi_Free(aux);
        aux = Ddi_BddSupp(zero);
        Ddi_VarsetUnionAcc(supp,aux);
        Ddi_Free(aux);
        Ddi_VarsetIntersectAcc(supp,layer);
        
        if (!Ddi_VarsetIsVoid(supp)) {
          /* swap layers i, j */
          printf ("[L:%d->%d]",j,i);fflush(stdout);
          Ddi_BddarrayWrite(m->dc,i,dc);
          Ddi_BddarrayWrite(m->one,i,one);
          Ddi_BddarrayWrite(m->zero,i,zero);
          Ddi_BddarrayWrite(m->one,j,Ddi_MgrReadZero(ddm));
          Ddi_BddarrayWrite(m->zero,j,Ddi_MgrReadZero(ddm));
          Ddi_BddarrayWrite(m->dc,j,Ddi_MgrReadOne(ddm));
        }
        Ddi_Free(supp);
        break;
      }
    }

  }
#if 0
  printf("\n>>");
  for (i=ng-1; i>=0; i--) {
    if (i<ng-1) {
      dc = Ddi_BddarrayRead(m->dc,i);
    }
    else {
      dc = Ddi_BddarrayRead(m->one,i);
    }
    if (Ddi_BddIsOne(dc))
      printf(".");
    else
      printf("*");
  }
  printf("\n");
#endif
}

#endif

/**Function********************************************************************
  Synopsis    [move Meta components to proper layer]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
MetaAndAcc (
  Ddi_Bdd_t *f,
  Ddi_Bdd_t *g
)
{
  int i, j, ng, sizef, sizeg;
  Ddi_Bdd_t *ff, *gg, *dc_j;
  Ddi_Mgr_t *ddm; 
  Ddi_Meta_t *mf, *mg;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  Pdtutil_Assert(f->common.code == Ddi_Bdd_Meta_c,
    "Operation requires Meta BDD!");

  ddm = Ddi_ReadMgr(f);

  if (Ddi_BddIsMeta(f) && (f->data.meta->metaId != ddm->meta.id)) {
    f->data.meta->metaId = ddm->meta.id;
    DdiMetaAndExistAcc (f,NULL,NULL,NULL);
  }
  if (Ddi_BddIsMeta(g) && (g->data.meta->metaId != ddm->meta.id)) {
    g->data.meta->metaId = ddm->meta.id;
    DdiMetaAndExistAcc (g,NULL,NULL,NULL);
  }

  mf = f->data.meta;
  mg = g->data.meta;
  ng = Ddi_BddarrayNum(mf->one);

  Pdtutil_Assert(ng == ddm->meta.groupNum,
    "Wrong number of groups in Meta BDD");

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMax_c) {
    printf("Meta-And\n");
  }

  for (i=0; i<ng; i++) {

    ff = Ddi_BddNot(Ddi_BddarrayRead(mf->zero,i));
    gg = Ddi_BddNot(Ddi_BddarrayRead(mg->zero,i));

    for (j=0; j<i; j++) {
      dc_j = Ddi_BddarrayRead(mf->dc,j);
      gg = Ddi_BddEvalFree(MetaConstrainOpt(gg,dc_j),gg);
      ff = Ddi_BddEvalFree(MetaConstrainOpt(ff,dc_j),ff);
    }
    if ((Ddi_BddIsPartConj(ff)||Ddi_BddIsPartConj(gg)) &&
        (ddm->settings.part.existOptClustThresh >= 0)) {
      int k;
      Ddi_BddSetPartConj(ff);
      Ddi_BddSetPartConj(gg);
      for (k=0; k<Ddi_BddPartNum(gg); k++) {
        Ddi_BddPartInsertLast(ff,Ddi_BddPartRead(gg,k));
      }
      Ddi_BddSetClustered(ff,ddm->settings.part.existOptClustThresh);
    }
    else if (0 && (sizef=Ddi_BddSize(ff))>1000 && (sizeg=Ddi_BddSize(gg))>1000){
      Ddi_Bdd_t *fg=NULL;
      Ddi_MgrAbortOnSiftEnableWithThresh(ddm,(sizef+sizeg),1);

      fg=Ddi_BddAnd(ff,gg);
      if ((ddm->abortedOp)||(Ddi_BddSize(fg)>(sizef+sizeg)*4)) {
	Ddi_Free(fg);
      }

      Ddi_MgrAbortOnSiftDisable(ddm);
      if (fg==NULL) {
	Ddi_BddSetPartConj(ff);
	Ddi_BddPartInsertLast(ff,gg);
      }
      else {
	Ddi_DataCopy(ff,fg);
	Ddi_Free(fg);
      }
    }
    else {
      Ddi_BddAndAcc(ff,gg);
    }
    Ddi_Free(gg);
    for (j=0; j<i; j++) {
      dc_j = Ddi_BddarrayRead(mf->dc,j);
      ff = Ddi_BddEvalFree(MetaConstrainOpt(ff,dc_j),ff);
    }
    if (i==ng-1) {
      Ddi_BddarrayWrite(mf->one,i,ff);
      Ddi_BddarrayWrite(mf->dc,i,Ddi_MgrReadZero(ddm));
    }
    else {
      Ddi_BddarrayWrite(mf->one,i,Ddi_MgrReadZero(ddm));
      Ddi_BddarrayWrite(mf->dc,i,ff);
    }
    Ddi_BddNotAcc(ff);
    Ddi_BddarrayWrite(mf->zero,i,ff);
    Ddi_Free(ff);

  }

  MetaReduce(f,-1);

}



/**Function********************************************************************
  Synopsis    [Transform a Meta BDD to monolitic BDD]
  Description [Transform a Meta BDD to monolitic BDD]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
MetaToMono (
  Ddi_Bdd_t *f
)
{
  int ng, i;
  Ddi_Meta_t *m;
  Ddi_Mgr_t *ddm;
  Ddi_Bdd_t *one, *zero, *dc, *fBdd;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  Pdtutil_Assert(f->common.code == Ddi_Bdd_Meta_c,
    "Operation requires MEta BDD!");

  m = f->data.meta;
  ddm = Ddi_ReadMgr(f);
  ng = Ddi_BddarrayNum(m->one);

  Pdtutil_Assert(ng == ddm->meta.groupNum,
    "Wrong number of groups in Meta BDD");

  one = Ddi_BddarrayRead(m->one,ng-1);
  fBdd = Ddi_BddDup(one);
  Ddi_BddSetMono(fBdd);

  for (i=ng-2; i>=0; i--) {

    one = Ddi_BddarrayRead(m->one,i);
    zero = Ddi_BddarrayRead(m->zero,i);
    dc = Ddi_BddarrayRead(m->dc,i);

    Ddi_BddSetMono(one);
    Ddi_BddSetMono(zero);
    Ddi_BddSetMono(dc);

    fBdd = Ddi_BddEvalFree(Ddi_BddIte(dc,fBdd,one),fBdd);

  }

  DdiMetaFree(f->data.meta);
  f->data.bdd = fBdd->data.bdd;
  Cudd_Ref (f->data.bdd);
  f->common.code = Ddi_Bdd_Mono_c;
  Ddi_Free(fBdd);

}


/**Function********************************************************************
  Synopsis    [Transform a Meta BDD to monolitic BDD]
  Description [Transform a Meta BDD to monolitic BDD]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
MetaToAig (
  Ddi_Bdd_t *f
)
{
  int ng, i;
  Ddi_Meta_t *m;
  Ddi_Mgr_t *ddm;
  Ddi_Bdd_t *one, *zero, *dc, *fAig;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  Pdtutil_Assert(f->common.code == Ddi_Bdd_Meta_c,
    "Operation requires MEta BDD!");

  m = f->data.meta;
  ddm = Ddi_ReadMgr(f);
  ng = Ddi_BddarrayNum(m->one);

  Pdtutil_Assert(ng == ddm->meta.groupNum,
    "Wrong number of groups in Meta BDD");

  one = Ddi_BddarrayRead(m->one,ng-1);
  fAig = Ddi_BddDup(one);
  Ddi_BddSetAig(fAig);

  for (i=ng-2; i>=0; i--) {

    one = Ddi_BddarrayRead(m->one,i);
    zero = Ddi_BddarrayRead(m->zero,i);
    dc = Ddi_BddarrayRead(m->dc,i);

    Ddi_BddSetAig(one);
    Ddi_BddSetAig(zero);
    Ddi_BddSetAig(dc);

    fAig = Ddi_BddEvalFree(Ddi_BddIte(dc,fAig,one),fAig);

  }

  Ddi_DataCopy(f,fAig);

  Ddi_Free(fAig);

}


/**Function********************************************************************
  Synopsis    [Transform a Meta BDD to monolitic BDD]
  Description [Transform a Meta BDD to monolitic BDD]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
MetaToCpart (
  Ddi_Bdd_t *f
)
{
  int ng, i;
  Ddi_Meta_t *m;
  Ddi_Mgr_t *ddm;
  Ddi_Bdd_t *one, *fBdd;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  Pdtutil_Assert(f->common.code == Ddi_Bdd_Meta_c,
    "Operation requires MEta BDD!");

  MetaSetConj(f);

  m = f->data.meta;
  ddm = Ddi_ReadMgr(f);
  ng = Ddi_BddarrayNum(m->one);

  Pdtutil_Assert(ng == ddm->meta.groupNum,
    "Wrong number of groups in Meta BDD");

  one = Ddi_BddarrayRead(m->one,ng-1);

  fBdd = Ddi_BddDup(one);
  Ddi_BddSetPartConj(fBdd);

  for (i=ng-2; i>=0; i--) {

    one = Ddi_BddNot(Ddi_BddarrayRead(m->zero,i));
    if (!Ddi_BddIsOne(one)) {
      Ddi_BddPartInsertLast(fBdd,one);
    }
    Ddi_Free(one);

  }

  DdiGenericDataCopy((Ddi_Generic_t *)f,(Ddi_Generic_t *)fBdd);
  Ddi_Free(fBdd);

}


/**Function********************************************************************
  Synopsis    [Update Meta handling by calling Init]
  Description [Update Meta handling by calling Init. This is activated if 
    new variables have been created or reordering has taken place from 
    previous. Return true (non 0) if activated]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
MetaUpdate (
  Ddi_Mgr_t *ddm, 
  int enableInit
)
{
  int nvar, doInit;

  doInit = 0;
  if (ddm->settings.meta.method != Ddi_Meta_Size
      && ddm->settings.meta.method != Ddi_Meta_FilterVars)
    return(doInit);

  nvar = Ddi_MgrReadNumVars(ddm);
  if (0 && nvar > ddm->meta.nvar) {
    doInit = 1;
  }
  else {
#if 0
    /* old strategy */
    for (i=0; i<nvar; i++) {
      if (ddm->meta.ord[i] != Ddi_MgrReadInvPerm(ddm,i)) {
	doInit = 1;
        break;
      }
    }
#else
    doInit = (ddm->meta.id < ddm->stats.siftRunNum);
#endif
  }

  if (doInit&&enableInit) {
    Ddi_MetaInit(ddm,ddm->settings.meta.method,NULL,NULL,NULL,NULL,
      ddm->settings.meta.groupSizeMin);
  }

  return(doInit);

}

#if META_CONST

/**Function********************************************************************
  Synopsis    [Operate And-Exist between Meta BDD and conj. part. BDD]
  Description [Operate And-Exist between Meta BDD and conj. part. BDD]
  SideEffects []
  SeeAlso     []
******************************************************************************/

static Ddi_Bdd_t *
MetaLinearAndExistAcc (
  Ddi_Bdd_t *fMeta,
  Ddi_Bdd_t *gBdd,
  Ddi_Varset_t *smooth
  )
{
  int i, ng, peak=0, size;
  Ddi_Bdd_t *dc, *gg, *ff, *tmp;
  Ddi_Varset_t *metaGrp, *downTot, *smooth_down, *aux, *range;
  Ddi_Varsetarray_t *downGroups;
  Ddi_Mgr_t *ddm; 
  Ddi_Meta_t *m;

  Pdtutil_Assert(fMeta->common.code == Ddi_Bdd_Meta_c,
    "Operation requires Meta BDD!");
  Pdtutil_Assert(gBdd->common.code == Ddi_Bdd_Part_Conj_c,
    "Operation requires cpart BDD!");

  ddm = Ddi_ReadMgr(fMeta);

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
    fprintf (stdout, "$LAE-M:");
    fflush(stdout);
  }

  if (smooth == NULL) {
    smooth = Ddi_VarsetVoid(ddm);
  }
  else {
    smooth = Ddi_VarsetDup(smooth);
  }

  MetaSetConj(fMeta);
  ff = Ddi_BddMakeFromMeta(fMeta);
  ng = ddm->meta.groupNum;
  m = fMeta->data.meta;

  range = Ddi_BddSupp(fMeta);
  aux = Ddi_BddSupp(gBdd);
  Ddi_VarsetUnionAcc(range,aux);
  Ddi_Free(aux);
  Ddi_VarsetDiffAcc(range,smooth);

  downGroups = Ddi_VarsetarrayAlloc(ddm,ng);
  downTot = Ddi_VarsetVoid(ddm);
  gg = Ddi_BddDup(gBdd);

#if 0
  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMax_c) {
    fprintf (stdout, "\nRepartitioning [%d]->[", Ddi_BddSize(gBdd));
    fflush(stdout);
  }
#endif
  for (i=ng-1; i>=0; i--) {
    Ddi_VarsetarrayWrite(downGroups,i,downTot);
    metaGrp = Ddi_VarsetarrayRead(ddm->meta.groups,i);
    Ddi_VarsetUnionAcc(downTot,metaGrp);
#if 0
    smooth_i = Ddi_VarsetDiff(range,metaGrp);
    /*    p = Ddi_BddExist(gBdd,smooth_i); */
    Ddi_VarsetPrint(smooth_i, 0, NULL, stdout);
    p = Part_BddMultiwayLinearAndExist(gBdd,smooth_i,100);
    Ddi_BddPartInsert(gg,0,p);
    if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMax_c) {
      if (i>0) {
        fprintf (stdout, "[%d,", Ddi_BddSize(p));
      }
      else {
        fprintf (stdout, "%d]=[(%d):%d]", Ddi_BddSize(p), 
          Ddi_BddPartNum(gg), Ddi_BddSize(gg));
      }
      fflush(stdout);
    }
    Ddi_Free(p);
    Ddi_Free(smooth_i);
#endif
  }
  Ddi_Free(downTot);

  Ddi_BddPartInsert(gg,0,ff);
  Ddi_Free(ff);

  for (i=0; i<ng; i++) {

    if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
      fprintf (stdout, "\nMLAE[%d] ", i);
      fflush(stdout);
    }

    if (MetaUpdate(ddm,1)) {
      if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
        fprintf (stdout, "(%d)", peak);
        fflush(stdout);
      }
      Ddi_Free(downGroups);
      DdiMetaAndExistAcc (fMeta,NULL,NULL,NULL);
      DdiMetaAndExistAcc (fMeta,gg,smooth,NULL);
      Ddi_Free(gg);
      Ddi_Free(smooth);
      return(fMeta);
    }

    smooth_down = Ddi_VarsetarrayRead(downGroups,i);
    smooth_down = Ddi_VarsetUnion(smooth_down,smooth);

    dc = Ddi_BddExist(gg,smooth_down);

    if (i<ng-1) {
 
#if 1
      tmp = Ddi_BddConstrain(gg,dc);
      if (Ddi_BddSize(tmp) < Ddi_BddSize(gg)) {
        Ddi_Free(gg);
        gg = tmp;
        printf("|-|"); fflush(stdout);
      }
      else {
        Ddi_Free(tmp);
        Ddi_BddPartInsert(gg,0,dc);
        Ddi_Free(dc);
        dc = Ddi_BddMakeConst(ddm,1);
        printf("|+|"); fflush(stdout);
      }
#endif
      Ddi_BddarrayWrite(m->dc,i,dc);
    }
    else {
      /*Ddi_VarsetPrint(smooth_down, 0, NULL, stdout);*/
      Ddi_BddarrayWrite(m->one,i,dc);
    }

    if ((size = Ddi_BddSize(gg))>peak) {
      peak = size;
    }

    Ddi_BddarrayWrite(m->zero,i,Ddi_BddNotAcc(dc));

    Ddi_Free(dc);
    Ddi_Free(smooth_down);
  }

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
    fprintf (stdout, "(%d)%d->", peak, Ddi_BddSize(fMeta));
    fflush(stdout);
  }

  MetaReduce(fMeta,-1);

#if 1
  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
    fprintf (stdout, "%d|", Ddi_BddSize(fMeta));
    fflush(stdout);
  }

  MetaSimplify(fMeta,0,ng-1);
#endif

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
    fprintf (stdout, "%d$", Ddi_BddSize(fMeta));
    fflush(stdout);
  }

  Ddi_Free(range);
  Ddi_Free(ff);
  Ddi_Free(gg);
  Ddi_Free(smooth);
  Ddi_Free(downGroups);

  return(fMeta);

}

#endif

/**Function********************************************************************
  Synopsis    [Optimized constrain cofactor.]
  Description [return constrain only if size reduced]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
MetaConstrainOpt (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  Ddi_Bdd_t *r;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  int sizef, sizeg;

  if (Ddi_BddIsOne(g)) {
    return(Ddi_BddDup(f));
  }

  sizef = f->common.size;
  if (sizef == 0 || (ddm->stats.siftCurrOrdNodeId >= f->common.nodeid)) {
    sizef = Ddi_BddSize(f);
  }
#if 0
  else {
    Pdtutil_Assert(sizef == Ddi_BddSize(f),"Wrong Size");
  }
#endif
  sizeg = 0;

  Ddi_MgrSiftSuspend(ddm);
  Ddi_MgrAbortOnSiftEnableWithThresh(ddm,2*sizef+sizeg,0);

  r = MetaConstrainOptIntern (f,g, sizef, sizeg);
  if (Ddi_BddEqual(r,f)) {
    r->common.size = sizef;
  }

  Ddi_MgrAbortOnSiftDisable(ddm);
  Ddi_MgrSiftResume(ddm);

  return (r);
}

/**Function********************************************************************
  Synopsis    [Optimized constrain cofactor.]
  Description [return constrain only if size reduced]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
MetaConstrainOptIntern (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g,
  int sizef,
  int sizeg
)
{
  Ddi_Bdd_t *r, *p, *pc;
  int i;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);

  if (Ddi_BddIsOne(g)) {
    return(Ddi_BddDup(f));
  }

  if (Ddi_BddIsPartConj(f)) {
    r = Ddi_BddDup(f);
    for (i=0; i<Ddi_BddPartNum(f); i++) {
      p = Ddi_BddPartRead(r,i);
#if 0 
      pc = Ddi_BddConstrain(p,g);
#else
      pc = MetaConstrainOptIntern(p,g,0,sizeg);
#endif
      if (Ddi_BddSize(pc)<Ddi_BddSize(p)) {
        Ddi_BddPartWrite(r,i,pc);
      }
      Ddi_Free(pc);
    }
  }
  else if (Ddi_BddIsPartConj(g)) {
    r = Ddi_BddDup(f);
    for (i=0; i<Ddi_BddPartNum(g); i++) {
      p = Ddi_BddPartRead(g,i);
      r = Ddi_BddEvalFree(MetaConstrainOptIntern(r,p,sizef,0),r);
    }
  }
  else {
    if (sizef == 0) {
      sizef = Ddi_BddSize(f);
    }
    if (sizeg == 0) {
      sizeg = Ddi_BddSize(g);
    }
#if 0
    /* buggy supp-attach ! */
    if (Ddi_BddSuppRead(g)==NULL) {
      Ddi_BddSuppAttach(g);
    }
    if (Ddi_BddSuppRead(f)==NULL) {
      Ddi_BddSuppAttach(f);
    }
#endif
    if (1&(sizef > 100) && (sizeg > 100)) {
      int totg;
      Ddi_Bdd_t *myG;
      Ddi_Varset_t *sm, *keepf;

      Ddi_MgrAbortOnSiftSuspend(ddm);

      sm = Ddi_BddSupp(g);
      keepf = Ddi_BddSupp(f);
      totg = Ddi_VarsetNum(sm);
      Ddi_VarsetDiffAcc(sm,keepf);
      myG = Ddi_BddDup(g);
      if (Ddi_VarsetNum(sm)*10 > totg) {
        Ddi_BddExistAcc(myG,sm);
      }
      Ddi_Free(sm);
      Ddi_Free(keepf);
      Ddi_MgrAbortOnSiftResume(ddm);

      r = Ddi_BddConstrain(f,myG);
      Ddi_Free(myG);
    }
    else {
      r = Ddi_BddConstrain(f,g);
    }
    if (Ddi_BddSize(r)>sizef) {
      Ddi_Free(r);
      r = Ddi_BddDup(f);
    }
  }

  return (r);
}


/**Function********************************************************************
  Synopsis    [Universal Abstract.]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
MetaForall (
  Ddi_Bdd_t  *f,
  Ddi_Varset_t  *smooth
)
{
  Ddi_Bdd_t *r;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity(ddm);

#if 0
  printf ("\nEW: %d",Ddi_BddSize(f));
  {
  DdNode *fCU, *smCU;

  fCU = Cudd_Not(Ddi_BddToCU(f));
  smCU = Ddi_VarsetToCU(smooth);
  r = Ddi_BddMakeFromCU(ddm,Cudd_Not(
    Cuplus_bddExistAbstractWeak(Ddi_MgrReadMgrCU(ddm),fCU,smCU)));
  }
  if (verbosity >= Pdtutil_VerbLevelDevMin_c) {
    printf ("->%d\n",Ddi_BddSize(r));
  }
#else
#if 0
  r = Ddi_BddForall(f,smooth);
#else
  r = Ddi_BddNot(f);
  Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,Pdtutil_VerbLevelUsrMin_c);
  Ddi_BddExistOverApproxAcc(r,smooth);
  Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,verbosity);
  Ddi_BddNotAcc(r);
  if (ddm->settings.part.existOptClustThresh >= 0) {
    Ddi_BddSetClustered(r,ddm->settings.part.existOptClustThresh);
    if (Ddi_BddIsPartConj(r)||Ddi_BddIsPartDisj(r)) {
      if (Ddi_BddPartNum(r)==1) {
        Ddi_BddSetMono(r);
      }
    }
  }
  else {
    Ddi_BddSetMono(r);
  }
#endif
#endif


  return (r);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
static void
MetaAndExistAfterUpdate (
  Ddi_Bdd_t *fMeta,
  Ddi_Bdd_t *ff,
  Ddi_Bdd_t *gg,
  Ddi_Varset_t *smooth,
  Ddi_Bdd_t *care,
  int curr_layer,
  int n_layers,
  Pdtutil_VerbLevel_e verbosity
)
{
  Ddi_Bdd_t *fMeta2, *dc, *myCare;
  int k, esl;
  Ddi_Meta_t *m, *m2;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(fMeta);

#if 0
  /* here just for DEBUG ! */
  Ddi_Bdd_t *a, *b, *c;
  a = Ddi_BddMakeMono(fMeta);
  b = Ddi_BddMakeMono(ff);
  c = Ddi_BddMakeMono(gg);
  Ddi_BddAndAcc(a,b);
  Ddi_BddAndAcc(a,c);
  Ddi_BddExistAcc(a,smooth);
  Ddi_Free(b);
  Ddi_Free(c);
#endif

  printf("START UPDATE\n");
  
  esl = enableSkipLayer;
  enableSkipLayer = 0;
  
  /*enableUpdate = 0;*/
  
  m = fMeta->data.meta;


#if 1

#if 0
  for (k=curr_layer;k<n_layers-1;k++) {
    Ddi_BddarrayWrite(m->one,k,Ddi_MgrReadZero(ddm));
    Ddi_BddarrayWrite(m->zero,k,Ddi_MgrReadZero(ddm));
    Ddi_BddarrayWrite(m->dc,k,Ddi_MgrReadOne(ddm));
  }
  Ddi_BddarrayWrite(m->dc,k,Ddi_MgrReadZero(ddm));
  Ddi_BddarrayWrite(m->zero,k,Ddi_MgrReadZero(ddm));
  Ddi_BddarrayWrite(m->one,k,Ddi_MgrReadOne(ddm));
#endif


#if 1

  for (k=0; k<n_layers;k++) {
    if (k<n_layers-1) {
      dc = Ddi_BddarrayRead(m->dc,k);
    }
    else {
      dc = Ddi_BddarrayRead(m->one,k);
    }
    gg = Ddi_BddEvalFree(MetaConstrainOpt(gg,dc),gg);
    ff = Ddi_BddEvalFree(MetaConstrainOpt(ff,dc),ff);
  }

#endif

  /* copy ff to fMeta2 and update to new ordering */

  fMeta2 = Ddi_BddMakeConst(ddm,1);
  Ddi_BddSetMeta(fMeta2);
  /* we need this to enable align in fMeta2 */
  fMeta2->data.meta->metaId = fMeta->data.meta->metaId;
  m2 = fMeta2->data.meta;
  
#if 0
  DdiMetaAndExistAcc(fMeta2,ff,NULL,care);
#else
  for (k=curr_layer;k<n_layers-1;k++) {
    Ddi_BddarrayWrite(m2->one,k,Ddi_MgrReadZero(ddm));
    dc = Ddi_BddPartExtract(ff,0);
    Ddi_BddarrayWrite(m2->dc,k,dc);
    Ddi_BddarrayWrite(m2->zero,k,Ddi_BddNotAcc(dc));
    Ddi_Free(dc);
  }
  dc = Ddi_BddPartExtract(ff,0);
  Ddi_BddarrayWrite(m2->one,k,dc);
  Ddi_BddarrayWrite(m2->dc,k,Ddi_MgrReadZero(ddm));
  Ddi_BddarrayWrite(m2->zero,k,Ddi_BddNotAcc(dc));
  Ddi_Free(dc);

  Pdtutil_Assert(Ddi_BddPartNum(ff)==0,"ff is not void");
#endif

  Ddi_Free(ff);
  
  MetaAlign (fMeta,care);
  MetaAlign (fMeta2,care);

  if (care != NULL) {
    myCare = Ddi_BddDup(care);
  }
  else {
    myCare = Ddi_BddMakeConst(ddm,1);
  }

  Ddi_BddSetPartConj(myCare);
  MetaSetConj(fMeta);

  for (k=0; k<n_layers-1;k++) {
    dc = Ddi_BddarrayRead(m->dc,k);
    Ddi_BddPartInsertLast(myCare,dc);
  }
  dc = Ddi_BddarrayRead(m->one,k);
  Ddi_BddPartInsertLast(myCare,dc);

#if 0

  dc = NULL;

  Ddi_BddSetPartConj(myCare);
  Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,Pdtutil_VerbLevelUsrMin_c);
  dc = Ddi_BddExistOverApprox(gg,smooth);
  Ddi_MgrSetOption(ddm,Pdt_DdiVerbosity_c,inum,verbosity);
  Ddi_BddRestrictAcc(gg,dc);
  if (!Ddi_BddIsOne(dc)) {
    Ddi_BddPartInsertLast(myCare,dc);
  }

  if (dc != NULL) {
    DdiMetaAndExistAcc (fMeta,dc,NULL,NULL);
    Ddi_Free(dc);
  }
#endif

#if 0
  {
    int j;
    Ddi_Varset_t *partVars = Ddi_BddSupp(gg);
    Ddi_Bdd_t *fMeta3 = Ddi_BddDup(fMeta2);
    Ddi_Bdd_t *ggDpart;
    /* partition on non smooth vars only */
    Ddi_VarsetDiffAcc(partVars,smooth);
    ggDpart = Part_PartitionDisjSet(gg,NULL,partVars,Part_MethodEstimateFast_c,
        Ddi_BddSize(gg)/2,3,Pdtutil_VerbLevelDevMax_c);
    Ddi_Free(gg);
    gg = ggDpart;
    Ddi_Free(partVars);
    DdiMetaAndExistAcc (fMeta2,Ddi_BddPartRead(gg,0),smooth,myCare);
    for (j=1; j<Ddi_BddPartNum(gg); j++) {
      Ddi_Bdd_t *fMeta3b = Ddi_BddDup(fMeta3);
      DdiMetaAndExistAcc (fMeta3b,Ddi_BddPartRead(gg,j),smooth,myCare);
      DdiMetaOrDisjAcc(fMeta2,fMeta3b);
      Ddi_Free(fMeta3b);
    }
    Ddi_Free(fMeta3);
  }
#else
  if (1&&Ddi_BddIsPartConj(gg) && Ddi_BddPartNum(gg) > 1) {
    int i,n;
    n = Ddi_BddPartNum(gg);
    for (i=0; i<n; i++) {
      Ddi_Bdd_t *p = Ddi_BddPartExtract(gg,0);
      Ddi_Varset_t *supp, *sm;
      Ddi_Bdd_t *c = Ddi_BddDup(myCare);
      sm = Ddi_VarsetDup(smooth);
      if (i<n-1) {
        supp = Ddi_BddSupp(gg);
	Ddi_VarsetDiffAcc(sm,supp);
	Ddi_BddPartInsertLast(c,gg);
	Ddi_Free(supp);
      }
      DdiMetaAndExistAcc (fMeta2,p,sm,c);
      Ddi_Free(sm);
      Ddi_Free(p);
      Ddi_Free(c);
    }
  }
  else {
    DdiMetaAndExistAcc (fMeta2,gg,smooth,myCare);
  }
#endif

  Ddi_Free(myCare);
  Ddi_Free(gg);
  Ddi_Free(smooth);

  MetaSetConj(fMeta);
  MetaSetConj(fMeta2);
  MetaAlign (fMeta,care);
  MetaAlign (fMeta2,care);

  MetaAndAcc(fMeta,fMeta2);
  
  Ddi_Free(fMeta2);
  MetaAlign (fMeta,care);

#else

  for (k=curr_layer;k<n_layers-1;k++) {
    dc = Ddi_BddPartExtract(ff,0);
    Ddi_BddarrayWrite(m->dc,k,dc);
    Ddi_BddarrayWrite(m->zero,k,Ddi_BddNotAcc(dc));
    Ddi_Free(dc);
  }
  dc = Ddi_BddPartExtract(ff,0);
  Ddi_BddarrayWrite(m->one,k,dc);
  Ddi_BddarrayWrite(m->dc,k,Ddi_MgrReadZero(ddm));
  Ddi_BddarrayWrite(m->zero,k,Ddi_BddNotAcc(dc));
  Ddi_Free(dc);
  Pdtutil_Assert(Ddi_BddPartNum(ff)==0,"ff is not void");
  Ddi_Free(ff);

  MetaAlign (fMeta,care);
  DdiMetaAndExistAcc (fMeta,gg,smooth,care);
  Ddi_Free(gg);
  Ddi_Free(smooth);

#endif
  
  printf("\nEND UPDATE\n");
  
#if 0
  b = Ddi_BddMakeMono(fMeta);
        Pdtutil_Assert(Ddi_BddEqual(a,b),"WRONG UPDATE");
        Ddi_Free(a);
        Ddi_Free(b);
#endif

  enableSkipLayer = esl;
  enableUpdate = 1;
  return;
}

/**Function********************************************************************
  Synopsis    [Operate Or between two Meta BDDs]
  Description [Operate Or between two Meta BDDs]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
MetaOrAcc (
  Ddi_Bdd_t *fMeta,
  Ddi_Bdd_t *gMeta,
  int disj
)
{
  int i, j, ng, jmax;
  Ddi_Bdd_t *fg0, *f0, *g0, *fg1, *f1, *g1, *dcup, *f0up, *g0up;
  Ddi_Bdd_t *care;
  Ddi_Mgr_t *ddm; 
  Ddi_Meta_t *mf, *mg;

  Pdtutil_Assert(fMeta->common.code == Ddi_Bdd_Meta_c,
    "Operation requires Meta BDD!");
#if 0
  Pdtutil_Assert(gMeta->common.code == Ddi_Bdd_Meta_c,
    "Operation requires Meta BDD!");
#endif

  if (Ddi_BddIsZero(fMeta)) {
    Ddi_BddSetMeta(fMeta);
    Ddi_BddSetMeta(gMeta);
    DdiGenericDataCopy((Ddi_Generic_t *)fMeta,(Ddi_Generic_t *)gMeta);
    return(fMeta);
  }
  if (Ddi_BddIsZero(gMeta)) {
    Ddi_BddSetMeta(fMeta);
    return(fMeta);
  }

  ddm = Ddi_ReadMgr(fMeta);

  if (Ddi_BddIsMeta(fMeta) && (fMeta->data.meta->metaId != ddm->meta.id)) {
    fMeta->data.meta->metaId = ddm->meta.id;
    DdiMetaAndExistAcc (fMeta,NULL,NULL,NULL);
  }
  if (Ddi_BddIsMeta(gMeta) && (gMeta->data.meta->metaId != ddm->meta.id)) {
    gMeta->data.meta->metaId = ddm->meta.id;
    DdiMetaAndExistAcc (gMeta,NULL,NULL,NULL);
  }

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
    fprintf (stdout, "$OR-M:");
    fflush(stdout);
  }

#if 0
  MetaSetConj(fMeta);
  MetaSetConj(gMeta);
#endif
  /*
  MetaAlignLayers(fMeta);
  MetaAlignLayers(gMeta);
  */
  ng = ddm->meta.groupNum;
  mf = fMeta->data.meta;
  if (Ddi_BddIsMono(gMeta)) {
    mg = NULL;
  }
  else {
    mg = gMeta->data.meta;
  }

  f0up = Ddi_BddMakeConst(ddm,0);
  g0up = Ddi_BddMakeConst(ddm,0);
  care = Ddi_BddMakeConst(ddm,1);
  Ddi_BddSetPartConj(care);

  for (i=0; i<ng; i++) {

    if ((i<ng-1)&&Ddi_BddIsOne(Ddi_BddarrayRead(mf->dc,i))
        &&(mg!=NULL)&&Ddi_BddIsOne(Ddi_BddarrayRead(mg->dc,i))) {
      Ddi_BddarrayWrite(mf->one,i,Ddi_MgrReadZero(ddm));
      Ddi_BddarrayWrite(mf->zero,i,Ddi_MgrReadZero(ddm));
      Ddi_BddarrayWrite(mf->dc,i,Ddi_MgrReadOne(ddm));
      continue;
    }

    if (i<ng-1) {
      f1 = Ddi_BddDup(Ddi_BddarrayRead(mf->one,i));
      if (mg == NULL) {  
        if (i<(ng-1)) {
      	 g1 = Ddi_BddMakeConst(ddm,0);
        }
        else {
      	 g1 = Ddi_BddDup(gMeta);
        }
      }
      else {
        g1 = Ddi_BddDup(Ddi_BddarrayRead(mg->one,i));
      }
          
          
      Ddi_BddDiffAcc(f1,f0up);
      Ddi_BddDiffAcc(g1,g0up);
          
      f1 = Ddi_BddEvalFree(MetaConstrainOpt(f1,care),f1);
      g1 = Ddi_BddEvalFree(MetaConstrainOpt(g1,care),g1);
          
      fg1 = Ddi_BddOr(f1,g1);
      Ddi_Free(f1);
      Ddi_Free(g1);
    }
    else {
      fg1 = Ddi_BddMakeConst(ddm,0);
    }

    f0 = Ddi_BddDup(Ddi_BddarrayRead(mf->zero,i));
    if (mg == NULL) {  
      if (i<(ng-1)) {
	 g0 = Ddi_BddMakeConst(ddm,0);
      }
      else {
	 g0 = Ddi_BddNot(gMeta);
      }
    }
    else {
      g0 = Ddi_BddDup(Ddi_BddarrayRead(mg->zero,i));
    }

    f0 = Ddi_BddEvalFree(MetaConstrainOpt(f0,care),f0);
    g0 = Ddi_BddEvalFree(MetaConstrainOpt(g0,care),g0);

    Ddi_BddSetPartDisj(f0up);
    Ddi_BddSetPartDisj(g0up);
    Ddi_BddPartInsertLast(f0up,f0);
    Ddi_BddPartInsertLast(g0up,g0);

    Ddi_BddSetClustered(f0up,50000);
    Ddi_BddSetClustered(g0up,50000);

    f0up = Ddi_BddEvalFree(MetaConstrainOpt(f0up,care),f0up);
    g0up = Ddi_BddEvalFree(MetaConstrainOpt(g0up,care),g0up);

    Ddi_Free(f0);
    Ddi_Free(g0);
    if (Ddi_BddIsMono(f0up) && Ddi_BddIsMono(g0up)) {
      fg0 = Ddi_BddAnd(f0up,g0up);
      if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
        printf("+");fflush(stdout);
      }
    }
    else {

      Ddi_BddSetPartDisj(f0up);
      Ddi_BddSetPartDisj(g0up);

      f0 = Ddi_BddPartExtract(f0up,Ddi_BddPartNum(f0up)-1);
      g0 = Ddi_BddPartExtract(g0up,Ddi_BddPartNum(g0up)-1);

#if 0
      fg0 = Ddi_BddAnd(f0,g0);
      Ddi_BddRestrictAcc(fg0,care);
#else
      fg0 = Ddi_BddMakeConst(ddm,0);
#endif

      jmax = DDI_MAX(Ddi_BddPartNum(f0up),Ddi_BddPartNum(g0up));
      for (j=0;j<jmax;j++) {
        Ddi_Bdd_t *myCare, *p;
        if (j<Ddi_BddPartNum(f0up)&&
            (!Ddi_BddIsZero(Ddi_BddPartRead(f0up,j)))) {

          myCare = Ddi_BddNot(fg0);
      	  Ddi_BddRestrictAcc(g0,myCare);
      	  p = Ddi_BddRestrict(Ddi_BddPartRead(f0up,j),myCare);

          Ddi_BddAndAcc(p,g0);
      
          Ddi_BddSetPartDisj(fg0);
      	  Ddi_BddPartInsertLast(fg0,p);
      
          Ddi_Free(p);
          Ddi_Free(myCare); 
        }
        if (j<Ddi_BddPartNum(g0up)&&
            (!Ddi_BddIsZero(Ddi_BddPartRead(g0up,j)))) {

          myCare = Ddi_BddNot(fg0);
      	  Ddi_BddRestrictAcc(f0,myCare);
      	  p = Ddi_BddRestrict(Ddi_BddPartRead(g0up,j),myCare);

          Ddi_BddAndAcc(p,f0);
      
          Ddi_BddSetPartDisj(fg0);
       	  Ddi_BddPartInsertLast(fg0,p);

          Ddi_Free(p);
          Ddi_Free(myCare); 
        }
      }

      if (1)
      {
        Ddi_Bdd_t *myCare;
        Ddi_Bdd_t *fg0aux;
#if 1
        myCare = Ddi_BddNot(fg0);
        fg0aux = Ddi_BddAnd(f0,g0);
        Ddi_BddRestrictAcc(fg0aux,myCare);
        Ddi_BddRestrictAcc(fg0aux,care);
        Ddi_Free(myCare); 

        Ddi_BddSetPartDisj(fg0);
        Ddi_BddPartInsertLast(fg0,fg0aux);

        Ddi_Free(fg0aux); 
        Ddi_BddRestrictAcc(fg0,care);
#endif
        Ddi_BddSetClustered(fg0,50000);
        Ddi_BddRestrictAcc(fg0,care);
        Ddi_BddSetPartDisj(fg0);
        fg0aux = Ddi_BddMakeConst(ddm,0);
        while (Ddi_BddPartNum(fg0)>0) {
          Ddi_Bdd_t *p = Ddi_BddPartExtract(fg0,0);
          if (Ddi_BddPartNum(fg0)>0) {
            Ddi_Bdd_t *myCare = Ddi_BddNot(fg0);
            Ddi_BddRestrictAcc(p,myCare);
    	    Ddi_Free(myCare);
	  }
  	  Ddi_BddOrAcc(fg0aux,p);
          Ddi_BddRestrictAcc(fg0aux,care);
	  Ddi_Free(p); 
        }
        Ddi_Free(fg0);
        fg0 = fg0aux;
      }

      Ddi_BddRestrictAcc(f0,care);
      Ddi_BddRestrictAcc(g0,care);
      Ddi_BddPartInsertLast(f0up,f0); /* !!!! */
      Ddi_BddPartInsertLast(g0up,g0); /* !!!! */

      Ddi_Free(f0);
      Ddi_Free(g0);

      if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
        printf("^");fflush(stdout);
      }

    }

    fg0 = Ddi_BddEvalFree(MetaConstrainOpt(fg0,care),fg0);
    fg1 = Ddi_BddEvalFree(MetaConstrainOpt(fg1,care),fg1);

    Ddi_BddarrayWrite(mf->zero,i,fg0);

    if (i<ng-1) {

      dcup = Ddi_BddNotAcc(Ddi_BddOr(fg0,fg1));
      dcup = Ddi_BddEvalFree(MetaConstrainOpt(dcup,care),dcup);

      Ddi_BddarrayWrite(mf->dc,i,dcup);
      Ddi_BddarrayWrite(mf->one,i,fg1);
      if (!Ddi_BddIsOne(dcup)) {
        Ddi_BddPartInsertLast(care,dcup);
      }
      Ddi_Free(dcup);

    }
    else {
      Ddi_BddarrayWrite(mf->one,i,Ddi_BddNotAcc(fg0));
      Ddi_BddarrayWrite(mf->dc,i,Ddi_MgrReadZero(ddm));
    }

    f0up = Ddi_BddEvalFree(MetaConstrainOpt(f0up,care),f0up);
    g0up = Ddi_BddEvalFree(MetaConstrainOpt(g0up,care),g0up);

    Ddi_Free(fg0);
    Ddi_Free(fg1);

    if (i<ng-1 && MetaUpdate(ddm,1)) {

      int align;

      Ddi_Free(f0up);
      Ddi_Free(g0up);

      printf("START UPDATE (or)\n");

      MetaReduce(fMeta,-1);

      do {

        align = MetaAlign (fMeta,care);
        align += MetaAlign (gMeta,NULL);

      } while (align);

      DdiMetaRestrictAcc(gMeta,care);

      Ddi_Free(care);

      MetaOrAcc(fMeta,gMeta,disj);

      printf("END UPDATE (or)\n");

      return (fMeta);
    }

  }

  Ddi_Free(f0up);
  Ddi_Free(g0up);
  Ddi_Free(care);

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
    fprintf (stdout, "%d->", Ddi_BddSize(fMeta));
    fflush(stdout);
  }

#if 0
  MetaSimplifyUp(fMeta);

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
    fprintf (stdout, "<%d>->", Ddi_BddSize(fMeta));
    fflush(stdout);
  }
#endif

  MetaReduce(fMeta,-1);

#if 1
  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
    fprintf (stdout, "%d|", Ddi_BddSize(fMeta));
    fflush(stdout);
  }

  MetaSimplify(fMeta,0,ng-1);
#endif

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
    fprintf (stdout, "%d$", Ddi_BddSize(fMeta));
    fflush(stdout);
  }

  return(fMeta);

}


#if META_CONST

/**Function********************************************************************
  Synopsis    [Operate Or between two Meta BDDs]
  Description [Operate Or between two Meta BDDs]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
MetaOrAccOld (
  Ddi_Bdd_t *fMeta,
  Ddi_Bdd_t *gMeta,
  int disj
)
{
  int i, j, ng;
  Ddi_Bdd_t *fg0, *f0, *g0, *fg1, *f1, *g1, *dcup, *f0up, *g0up, *dc;
  Ddi_Mgr_t *ddm; 
  Ddi_Meta_t *mf, *mg;

  Pdtutil_Assert(fMeta->common.code == Ddi_Bdd_Meta_c,
    "Operation requires Meta BDD!");
#if 0
  Pdtutil_Assert(gMeta->common.code == Ddi_Bdd_Meta_c,
    "Operation requires Meta BDD!");
#endif

  if (Ddi_BddIsZero(fMeta)) {
    Ddi_BddSetMeta(fMeta);
    Ddi_BddSetMeta(gMeta);
    DdiGenericDataCopy((Ddi_Generic_t *)fMeta,(Ddi_Generic_t *)gMeta);
    return(fMeta);
  }
  if (Ddi_BddIsZero(gMeta)) {
    Ddi_BddSetMeta(fMeta);
    return(fMeta);
  }

  ddm = Ddi_ReadMgr(fMeta);

  if (Ddi_BddIsMeta(fMeta) && (fMeta->data.meta->metaId != ddm->meta.id)) {
    fMeta->data.meta->metaId = ddm->meta.id;
    DdiMetaAndExistAcc (fMeta,NULL,NULL,NULL);
  }
  if (Ddi_BddIsMeta(gMeta) && (gMeta->data.meta->metaId != ddm->meta.id)) {
    gMeta->data.meta->metaId = ddm->meta.id;
    DdiMetaAndExistAcc (gMeta,NULL,NULL,NULL);
  }

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
    fprintf (stdout, "$OR-M:");
    fflush(stdout);
  }

#if 0
  MetaSetConj(fMeta);
  MetaSetConj(gMeta);
#endif
  /*
  MetaAlignLayers(fMeta);
  MetaAlignLayers(gMeta);
  */
  ng = ddm->meta.groupNum;
  mf = fMeta->data.meta;
  if (Ddi_BddIsMono(gMeta)) {
    mg = NULL;
  }
  else {
    mg = gMeta->data.meta;
  }

  f0up = Ddi_BddMakeConst(ddm,0);
  g0up = Ddi_BddMakeConst(ddm,0);
  dcup = Ddi_BddMakeConst(ddm,1);

  for (i=0; i<ng; i++) {

    if ((i<ng-1)&&Ddi_BddIsOne(Ddi_BddarrayRead(mf->dc,i))
        &&(mg!=NULL)&&Ddi_BddIsOne(Ddi_BddarrayRead(mg->dc,i))) {
      Ddi_BddarrayWrite(mf->one,i,Ddi_MgrReadZero(ddm));
      Ddi_BddarrayWrite(mf->zero,i,Ddi_MgrReadZero(ddm));
      Ddi_BddarrayWrite(mf->dc,i,Ddi_MgrReadOne(ddm));
      continue;
    }

    f1 = Ddi_BddDup(Ddi_BddarrayRead(mf->one,i));
    if (mg == NULL) {  
      if (i<(ng-1)) {
	 g1 = Ddi_BddMakeConst(ddm,0);
      }
      else {
	 g1 = Ddi_BddDup(gMeta);
      }
    }
    else {
      g1 = Ddi_BddDup(Ddi_BddarrayRead(mg->one,i));
    }

    Ddi_BddDiffAcc(f1,f0up);
    Ddi_BddDiffAcc(g1,g0up);

    for (j=0; j<i; j++) {
      dc = Ddi_BddarrayRead(mf->dc,j);
      f1 = Ddi_BddEvalFree(MetaConstrainOpt(f1,dc),f1);
      g1 = Ddi_BddEvalFree(MetaConstrainOpt(g1,dc),g1);
    }

    fg1 = Ddi_BddOr(f1,g1);
    Ddi_Free(f1);
    Ddi_Free(g1);

    f0 = Ddi_BddDup(Ddi_BddarrayRead(mf->zero,i));
    if (mg == NULL) {  
      if (i<(ng-1)) {
	 g0 = Ddi_BddMakeConst(ddm,0);
      }
      else {
	 g0 = Ddi_BddNot(gMeta);
      }
    }
    else {
      g0 = Ddi_BddDup(Ddi_BddarrayRead(mg->zero,i));
    }

    for (j=0; j<i; j++) {
      dc = Ddi_BddarrayRead(mf->dc,j);
      f0 = Ddi_BddEvalFree(MetaConstrainOpt(f0,dc),f0);
      g0 = Ddi_BddEvalFree(MetaConstrainOpt(g0,dc),g0);
    }
    Ddi_BddOrAcc(f0,f0up); /* @@@@ */
    Ddi_BddOrAcc(g0,g0up);
    for (j=0; j<i; j++) {
      dc = Ddi_BddarrayRead(mf->dc,j);
      f0 = Ddi_BddEvalFree(MetaConstrainOpt(f0,dc),f0);
      g0 = Ddi_BddEvalFree(MetaConstrainOpt(g0,dc),g0);
    }
    fg0 = Ddi_BddAnd(f0,g0);

    Ddi_Free(dcup);
    dcup = Ddi_BddNotAcc(Ddi_BddOr(fg0,fg1));

    Ddi_Free(f0up);
    Ddi_Free(g0up);
    f0up = MetaConstrainOpt(f0,dcup);
    g0up = MetaConstrainOpt(g0,dcup);

    Ddi_Free(f0);
    Ddi_Free(g0);

    Ddi_BddarrayWrite(mf->zero,i,fg0);
    if (i<ng-1) {
      Ddi_BddarrayWrite(mf->dc,i,dcup);
      Ddi_BddarrayWrite(mf->one,i,fg1);
    }
    else {
      Ddi_BddarrayWrite(mf->one,i,Ddi_BddNotAcc(fg0));
      Ddi_BddarrayWrite(mf->dc,i,Ddi_MgrReadZero(ddm));
    }

    Ddi_Free(fg0);
    Ddi_Free(fg1);
  }

  Ddi_Free(f0up);
  Ddi_Free(g0up);
  Ddi_Free(dcup);

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
    fprintf (stdout, "%d->", Ddi_BddSize(fMeta));
    fflush(stdout);
  }

#if 0
  MetaSimplifyUp(fMeta);

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
    fprintf (stdout, "<%d>->", Ddi_BddSize(fMeta));
    fflush(stdout);
  }
#endif

  MetaReduce(fMeta,-1);

#if 1
  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
    fprintf (stdout, "%d|", Ddi_BddSize(fMeta));
    fflush(stdout);
  }

  MetaSimplify(fMeta,0,ng-1);
#endif

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
    fprintf (stdout, "%d$", Ddi_BddSize(fMeta));
    fflush(stdout);
  }

  return(fMeta);

}

#endif

/**Function********************************************************************
  Synopsis    [Optimized constrain cofactor.]
  Description [return constrain only if size reduced]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
static int
MetaAlign (
  Ddi_Bdd_t *f,
  Ddi_Bdd_t *care
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);

  /* check if meta management needs re-init */
  if (MetaUpdate(ddm,1) || (f->data.meta->metaId != ddm->meta.id)) {
    f->data.meta->metaId = ddm->meta.id;
    DdiMetaAndExistAcc (f,NULL,NULL,care);
    return(1);
  }

  return(0);
}



/**CFile***********************************************************************

  FileName    [fbvComb.c]

  PackageName [fbv]

  Synopsis    [Verification of combinational circuits]

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

#include "fbvInt.h"
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


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static void ApproxCheckMultiplier(
  Fsm_Mgr_t * fsmMgr
);
static void ApproxBckCheckMultiplier(
  Fsm_Mgr_t * fsmMgr
);
static void CheckMultiplier(
  Fsm_Mgr_t * fsmMgr
);
static void CollapseAuxVars(
  Ddi_Bddarray_t * funs,
  Ddi_Bddarray_t * auxf,
  Ddi_Vararray_t * auxv,
  Ddi_Bdd_t * constrain
);
static Ddi_Bdd_t *InputDecompose(
  Ddi_Bdd_t * f
);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
FbvCombCheck(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Comb_Check_e check
)
{
  switch (check) {
    case Fbv_Mult_c:
#if 0
      CheckMultiplier(fsmMgr);
#else
      ApproxBckCheckMultiplier(fsmMgr);
#endif
    case Fbv_None_c:{
      }
    default:{
      }
  }


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
static void
ApproxCheckMultiplier(
  Fsm_Mgr_t * fsmMgr
)
{
  extern int nAuxSlices;
  extern Ddi_Vararray_t *AuxSliceVars[];
  extern Ddi_Bddarray_t *AuxSliceFuns[];
  extern Ddi_Vararray_t *manualAuxVars;

  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);
  Ddi_Bddarray_t *lambda, *lambda2;
  int k, j, i;
  char buf[1000];
  Ddi_Var_t *v;
  Ddi_Bdd_t *sliceRel;
  Ddi_Varset_t *smooth, *slicev, *piv;
  Ddi_Bddarray_t *AuxSliceSets = Ddi_BddarrayAlloc(ddm, nAuxSlices);
  Ddi_Bdd_t *currSliceSet, *prevSliceSet;

  piv = Ddi_VarsetMakeFromArray(Fsm_MgrReadVarI(fsmMgr));
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "input vars:.\n")
    );
  Ddi_VarsetPrint(piv, 0, NULL, stdout);
  if (manualAuxVars != NULL) {
    smooth = Ddi_VarsetMakeFromArray(manualAuxVars);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "manual aux vars:.\n")
      );
    Ddi_VarsetPrint(smooth, 0, NULL, stdout);
    Ddi_VarsetUnionAcc(piv, smooth);
    Ddi_Free(smooth);
  }
  prevSliceSet = Ddi_BddMakeConst(ddm, 1);

  lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  lambda2 = Ddi_BddarrayDup(lambda);
  for (i = 0; i < nAuxSlices; i++) {
    smooth = Ddi_VarsetDup(piv);
    if (i > 0) {
      slicev = Ddi_VarsetMakeFromArray(AuxSliceVars[i - 1]);
      Ddi_VarsetUnionAcc(smooth, slicev);
      Ddi_Free(slicev);
    }
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "Computing %d Slice Set: ", i)
      ););
    if (Ddi_VararrayNum(AuxSliceVars[i]) > 0) {
      sliceRel = Ddi_BddRelMakeFromArray(AuxSliceFuns[i], AuxSliceVars[i]);
      /*Ddi_BddSetMono(sliceRel); */
      Ddi_BddSetClustered(sliceRel, 1000);
#if 1
      Ddi_BddExistAcc(sliceRel, piv);
#endif
      currSliceSet = Ddi_BddAndExist(sliceRel, prevSliceSet, smooth);
      Ddi_Free(sliceRel);
#if 0
      Ddi_BddarrayOp3(Ddi_BddCompose_c,
        lambda2, AuxSliceVars[i], AuxSliceFuns[i]);
#endif
    } else {
      currSliceSet = Ddi_BddMakeConst(ddm, 1);
    }

    Ddi_BddAndExistAcc(Ddi_BddarrayRead(lambda, i), prevSliceSet, smooth);

    Ddi_Free(prevSliceSet);
    prevSliceSet = currSliceSet;
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "slice set size: %d.\n", Ddi_BddSize(prevSliceSet))
      );
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "lambda[%d]: %d.\n", i,
        !Ddi_BddIsZero(Ddi_BddarrayRead(lambda, i)))
      );
    Ddi_BddarrayWrite(AuxSliceSets, i, currSliceSet);
    Ddi_Free(smooth);
  }
  Ddi_Free(prevSliceSet);

  Ddi_Free(piv);

}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
ApproxBckCheckMultiplier(
  Fsm_Mgr_t * fsmMgr
)
{
  extern int nAuxSlices;
  extern Ddi_Vararray_t *AuxSliceVars[];
  extern Ddi_Bddarray_t *AuxSliceFuns[];
  extern Ddi_Vararray_t *manualAuxVars;

  //  extern int meta;

  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);
  Ddi_Bddarray_t *lambda, *lambda2;
  int k, j, i;
  char buf[1000];
  Ddi_Var_t *v;
  Ddi_Bdd_t *sliceRel;
  Ddi_Varset_t *smooth, *slicev, *piv;
  Ddi_Bddarray_t *AuxSliceSets = Ddi_BddarrayAlloc(ddm, nAuxSlices);
  Ddi_Bdd_t *currSliceSet, *prevSliceSet;

  piv = Ddi_VarsetMakeFromArray(Fsm_MgrReadVarI(fsmMgr));
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "input vars:.\n")
    );
  Ddi_VarsetPrint(piv, 0, NULL, stdout);
  if (manualAuxVars != NULL) {
    smooth = Ddi_VarsetMakeFromArray(manualAuxVars);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "manual aux vars:.\n")
      );
    Ddi_VarsetPrint(smooth, 0, NULL, stdout);
    Ddi_VarsetUnionAcc(piv, smooth);
    Ddi_Free(smooth);
  }
  lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  prevSliceSet = Ddi_BddDup(Ddi_BddarrayRead(lambda, nAuxSlices - 1));

  for (i = nAuxSlices - 2; i > 0; i--) {
    smooth = Ddi_VarsetDup(piv);
    slicev = Ddi_VarsetMakeFromArray(AuxSliceVars[i]);
    Ddi_VarsetUnionAcc(smooth, slicev);
    Ddi_Free(slicev);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Computing Back %d Slice Set: ", i)
      ););
    if (Ddi_VararrayNum(AuxSliceVars[i]) > 0) {
      sliceRel = Ddi_BddRelMakeFromArray(AuxSliceFuns[i], AuxSliceVars[i]);
      /*Ddi_BddSetMono(sliceRel); */
      Ddi_BddSetClustered(sliceRel, 1000);
#if 0
      Ddi_BddExistAcc(sliceRel, piv);
#endif
      Ddi_BddSetPartConj(sliceRel);
#if 0
      if (meta) {
        Ddi_BddSetMeta(prevSliceSet);
      }
#endif
      Ddi_BddPartInsert(sliceRel, 0, prevSliceSet);
      currSliceSet = Ddi_BddMultiwayRecursiveAndExist(sliceRel, smooth,
        Ddi_MgrReadOne(ddm), -1);
      Ddi_Free(sliceRel);

      Ddi_BddSetMono(currSliceSet);
    } else {
      currSliceSet = Ddi_BddMakeConst(ddm, 1);
    }

    if (Ddi_BddIsZero(currSliceSet)) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "curr slice is zero.\n")
        );
    }
    Ddi_Free(prevSliceSet);
    prevSliceSet = currSliceSet;
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "slice set size: %d.\n", Ddi_BddSize(prevSliceSet))
      );
    Ddi_BddarrayWrite(AuxSliceSets, i, currSliceSet);
    Ddi_Free(smooth);
  }
  Ddi_Free(prevSliceSet);

  Ddi_Free(piv);

}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
CheckMultiplier(
  Fsm_Mgr_t * fsmMgr
)
{
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);
  Ddi_Bddarray_t *lambda;
  Ddi_Bdd_t *constrain;
  int k, j, i;
  char buf[1000];
  Ddi_Var_t *v;
  Ddi_Bdd_t *lit;

  if (Fsm_MgrReadAuxVarBDD(fsmMgr) != NULL) {
    Ddi_Vararray_t *auxv = Ddi_VararrayDup(Fsm_MgrReadVarAuxVar(fsmMgr));
    Ddi_Bddarray_t *auxf = Ddi_BddarrayDup(Fsm_MgrReadAuxVarBDD(fsmMgr));

    Fsm_MgrSetVarAuxVar(fsmMgr, NULL);
    Fsm_MgrSetAuxVarBDD(fsmMgr, NULL);
    lambda = Fsm_MgrReadLambdaBDD(fsmMgr);

    constrain = Ddi_BddMakeConst(ddm, 1);

    {
      int ii = Ddi_BddarrayNum(lambda) - 1;
      Ddi_Bddarray_t *auxf0 = Ddi_BddarrayDup(auxf);

      CollapseAuxVars(lambda, auxf0, auxv, constrain);
      Ddi_Free(auxf0);
      fprintf(stdout, "l[%d]: %d.\n", ii,
        Ddi_BddSize(Ddi_BddarrayRead(lambda, ii)));
      return;
    }

    Ddi_BddSetPartConj(constrain);
    for (k = 4; k < 16; k += 4) {
      Ddi_Bdd_t *newauxf;
      Ddi_Vararray_t *newauxv;
      Ddi_Bddarray_t *auxf0 = Ddi_BddarrayDup(auxf);
      Ddi_Bddarray_t *lambda0 = Ddi_BddarrayDup(lambda);

#if 0
      for (j = k; j < 16; j++) {
        sprintf(buf, "b_%d_", j);
        v = Ddi_VarFromName(ddm, buf);
        if (v != NULL) {
          lit = Ddi_BddMakeLiteral(v, 0);
          Ddi_BddarrayOp2(Ddi_BddConstrain_c, lambda0, lit);
          Ddi_BddarrayOp2(Ddi_BddConstrain_c, auxf0, lit);
          Ddi_Free(lit);
        }
      }
#endif
      CollapseAuxVars(lambda0, auxf0, auxv, constrain);
      newauxv = Ddi_VararrayAlloc(ddm, 0);
      for (i = Ddi_BddarrayNum(lambda0) - 1; i >= 0; i--) {
        Ddi_Var_t *v = Ddi_VarNewAtLevel(ddm, 0);

        Ddi_VararrayInsert(newauxv, 0, v);
      }
      newauxf = Ddi_BddRelMakeFromArray(lambda0, newauxv);
      Ddi_Free(newauxv);
      Ddi_Free(lambda0);
      Ddi_Free(auxf0);
      Ddi_BddSetClustered(newauxf, 1000);
#if 1
      Ddi_BddPartInsertLast(constrain, newauxf);
#endif
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "NewA: %d - l0: %d - a0: %d -> ",
          Ddi_BddSize(newauxf),
          Ddi_BddarraySize(lambda), Ddi_BddarraySize(auxf))
        );
#if 1
      Ddi_BddarrayOp2(Ddi_BddConstrain_c, auxf, newauxf);
      Ddi_BddarrayOp2(Ddi_BddConstrain_c, lambda, newauxf);
#endif
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "l1: %d - a1: %d.\n",
          Ddi_BddarraySize(lambda), Ddi_BddarraySize(auxf))
        );
      Ddi_Free(newauxf);
    }

    Fsm_MgrSetVarAuxVar(fsmMgr, auxv);
    Fsm_MgrSetAuxVarBDD(fsmMgr, auxf);
  }
#if 0
  else {
    Ddi_Bdd_t *newf =
      InputDecompose(Ddi_BddarrayRead(Fsm_MgrReadLambdaBDD(fsmMgr), 0));
    Ddi_Free(newf);
  }
#endif

}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
CollapseAuxVars(
  Ddi_Bddarray_t * funs,
  Ddi_Bddarray_t * auxf,
  Ddi_Vararray_t * auxv,
  Ddi_Bdd_t * constrain
)
{
  int i, j, k, nf, na, totf, tota;
  int **depf, **depa, *rowf, *rowa;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(funs);
  Ddi_Vararray_t *auxv2 = Ddi_VararrayAlloc(ddm, 0);
  Ddi_Bddarray_t *auxf2 = Ddi_BddarrayAlloc(ddm, 0);

  nf = Ddi_BddarrayNum(funs);
  na = Ddi_BddarrayNum(auxf);

  rowf = Pdtutil_Alloc(int,
    nf
  );
  rowa = Pdtutil_Alloc(int,
    na
  );
  depf = Pdtutil_Alloc(int *,
    na
  );
  depa = Pdtutil_Alloc(int *,
    na
  );

#if 1
  for (i = 0; i < na; i++) {
    Ddi_Varset_t *supp;
    Ddi_Var_t *v = Ddi_VararrayRead(auxv, i);

    totf = tota = 0;
    for (j = 0; j < nf; j++) {
      supp = Ddi_BddSupp(Ddi_BddarrayRead(funs, j));
      if (Ddi_VarInVarset(supp, v)) {
        rowf[j] = 1;
        totf++;
      } else {
        rowf[j] = 0;
      }
      Ddi_Free(supp);
    }
    for (j = 0; j < na; j++) {
      supp = Ddi_BddSupp(Ddi_BddarrayRead(auxf, j));
      if (Ddi_VarInVarset(supp, v)) {
        Pdtutil_Assert(j >= i, "Wrong aux var dependency");
        rowa[j] = 1;
        tota++;
      } else {
        rowa[j] = 0;
      }
      Ddi_Free(supp);
    }
    depf[i] = Pdtutil_Alloc(int,
      totf + 1
    );
    depa[i] = Pdtutil_Alloc(int,
      tota + 1
    );

    for (j = 0, k = 0; j < nf; j++) {
      if (rowf[j]) {
        depf[i][k++] = j;
      }
    }
    Pdtutil_Assert(k == totf, "wrong dependency count");
    depf[i][k++] = -1;          /* hook */
    for (j = 0, k = 0; j < na; j++) {
      if (rowa[j]) {
        depa[i][k++] = j;
      }
    }
    Pdtutil_Assert(k == tota, "wrong dependency count");
    depa[i][k++] = -1;          /* hook */

  }
#endif

  for (i = 0; i < Ddi_BddarrayNum(auxf); i++) {
    if (1 || Ddi_BddSize(Ddi_BddarrayRead(auxf, i)) > 10) {
      Ddi_Bddarray_t *oldl, *olda;
      Ddi_Bdd_t *f = Ddi_BddDup(Ddi_BddarrayRead(auxf, i));
      Ddi_Var_t *v = Ddi_VararrayRead(auxv, i);
      Ddi_Vararray_t *composev = Ddi_VararrayAlloc(ddm, 0);
      Ddi_Bddarray_t *composef = Ddi_BddarrayAlloc(ddm, 0);
      Ddi_Varset_t *smooth = NULL;
      Ddi_Bdd_t *rel = NULL;

      Ddi_BddarrayWrite(auxf, i, Ddi_MgrReadOne(ddm));
      Ddi_BddarrayInsert(composef, 0, f);
      Ddi_VararrayInsert(composev, 0, v);
      smooth = Ddi_VarsetMakeFromVar(v);
      rel = Ddi_BddMakeLiteral(v, 1);
      Ddi_BddXnorAcc(rel, f);
      Ddi_BddConstrainAcc(rel, constrain);

      oldl = Ddi_BddarrayDup(funs);
      olda = Ddi_BddarrayDup(auxf2);
#if 0
      Ddi_BddarrayOp3(Ddi_BddCompose_c, funs, composev, composef);
#else
#if 1
      for (k = 0; depf[i][k] >= 0; k++) {
        j = depf[i][k];
        Ddi_BddAndExistAcc(Ddi_BddarrayRead(funs, j), rel, smooth);
        Ddi_BddConstrainAcc(Ddi_BddarrayRead(funs, j), constrain);
      }
#else
      Ddi_BddarrayOp3(Ddi_BddAndExist_c, funs, rel, smooth);
#endif
      Ddi_BddarrayOp3(Ddi_BddAndExist_c, auxf2, rel, smooth);

#if 1
      for (k = 0; depa[i][k] >= 0; k++) {
        j = depa[i][k];
        if (j == i)
          continue;
        Ddi_BddAndExistAcc(Ddi_BddarrayRead(auxf, j), rel, smooth);
        Ddi_BddConstrainAcc(Ddi_BddarrayRead(auxf, j), constrain);
      }
#endif
#endif
#if 0
      if ((Ddi_BddarraySize(oldl) + Ddi_BddarraySize(olda))
        < 0.8 * (Ddi_BddarraySize(funs) + Ddi_BddarraySize(auxf2))) {
        Ddi_Bdd_t *f0;
        Ddi_Var_t *v0;

        Ddi_Free(auxf2);
        auxf2 = olda;
        Fsm_MgrSetLambdaBDD(fsmMgr, oldl);
        lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
        Ddi_Free(oldl);
        f0 = Ddi_BddarrayExtract(auxf, i);
        v0 = Ddi_VararrayExtract(auxv, i);
        Ddi_BddarrayInsert(auxf2, 0, f0);
        Ddi_VararrayInsert(auxv2, 0, v0);
        Ddi_Free(f0);
      } else
#endif
      {
        fprintf(stdout, "composed var: %s - lambda: %d - aux/aux2: %d/%d.\n",
          Ddi_VarName(v), Ddi_BddarraySize(funs), Ddi_BddarraySize(auxf),
          Ddi_BddarraySize(auxf2));
      }
      for (j = 0; j < na; j++) {
        Ddi_Varset_t *supp = Ddi_BddSupp(Ddi_BddarrayRead(auxf, j));

        if (Ddi_VarInVarset(supp, v)) {
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c, fprintf(stdout, "WRONG AUXV.\n")
            );
        }
        Ddi_Free(supp);
      }
      for (j = 0; j < nf; j++) {
        Ddi_Varset_t *supp = Ddi_BddSupp(Ddi_BddarrayRead(funs, j));

        if (Ddi_VarInVarset(supp, v)) {
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c, fprintf(stdout, "WRONG AUXV.\n")
            );
        }
        Ddi_Free(supp);
      }

      Ddi_Free(composef);
      Ddi_Free(composev);
      Ddi_Free(smooth);
      Ddi_Free(rel);
    }
  }

  if (Ddi_BddarrayNum(auxf2) > 0) {
    auxf = auxf2;
    auxv = auxv2;
    auxv2 = Ddi_VararrayAlloc(ddm, 0);
    auxf2 = Ddi_BddarrayAlloc(ddm, 0);
  }

  Pdtutil_Free(rowf);
  Pdtutil_Free(rowa);
#if 1
  for (i = 0; i < na; i++) {
    Pdtutil_Free(depf[i]);
    Pdtutil_Free(depa[i]);
  }
#endif
  Pdtutil_Free(depf);
  Pdtutil_Free(depa);

}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
InputDecompose(
  Ddi_Bdd_t * f
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  Ddi_Var_t *v, *newaux;
  Ddi_Varset_t *supp;
  Ddi_Bdd_t *rel, *newf, *lit, *f0, *f2;
  int k, j;
  char buf[1000];

  newf = Ddi_BddMakePartConjVoid(ddm);
  f0 = Ddi_BddDup(f);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "input decomp initial size: %d.\n", Ddi_BddSize(f))
    );

  for (k = 1; k < 16; k += 1) {

    f2 = Ddi_BddDup(f0);
    supp = Ddi_BddSupp(f2);
    for (j = k; j < 16; j++) {
      sprintf(buf, "b_%d_", j);
      v = Ddi_VarFromName(ddm, buf);
      if (v != NULL) {
        lit = Ddi_BddMakeLiteral(v, 0);
        Ddi_BddConstrainAcc(f2, lit);
        Ddi_Free(lit);
      }
    }
    v = Ddi_VarsetTop(supp);
    Ddi_Free(supp);
    newaux = Ddi_VarNewAfterVar(v);
    rel = Ddi_BddMakeLiteral(newaux, 1);
    Ddi_BddXnorAcc(rel, f2);
    Ddi_BddPartInsertLast(newf, rel);
    Ddi_BddConstrainAcc(f0, rel);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "after %d level: f2=%d - f0=%d - tot=%d.\n", k,
        Ddi_BddSize(f2), Ddi_BddSize(f0), Ddi_BddSize(newf))
      );

    Ddi_Free(rel);
    f2 = Ddi_BddCofactor(f0, newaux, 0);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "0 cof = %d - ", Ddi_BddSize(f2))
      );
    Ddi_Free(f2);
    f2 = Ddi_BddCofactor(f0, newaux, 1);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "1 cof = %d.\n", Ddi_BddSize(f2))
      );
    Ddi_Free(f2);
  }

  Ddi_BddPartInsertLast(newf, rel);
  Ddi_Free(f0);

  return (newf);
}

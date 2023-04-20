/**CFile***********************************************************************

  FileName    [trClosure.c]

  PackageName [tr]

  Synopsis    [Functions doing transitive closure]

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

static Ddi_Bdd_t *TrCompose(
  Tr_Mgr_t * trMgr,
  Ddi_Bdd_t * f,
  Ddi_Bdd_t * g,
  Ddi_Vararray_t * s,
  Ddi_Vararray_t * y,
  Ddi_Vararray_t * z,
  Ddi_Varset_t * suppZ
);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of external functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Build transitive closure of a Transition Relation
    by means of iterative squaring]

  Description [Transitive closure of a Monolithic or Disjunctively
    partitioned Transition Relation is operated using the iterative squaring
    method, expressed by the following recurrence equations:
    <p>
    T(0)(s,y) = TR(s,y) <br>
    T(i+1)(s,y)= TR(s,y) + Exist(z) ( T(i)(s,z) * T(i)(z,y) )
    <p>
    The least fixed point is T* (<em>transitive closure</em>)
    and the number of iterations required to compute T* is
    logarithmic in the sequential depth of TR (the diameter of
    the state transition graph represented by TR).<br>
    The transitive closure describes the pairs of states that are
    connected by at least one path in the state graph of FSM.
    This function is a shell to TrBuildTransClosure, where the
    job is really done. Nothing is done here if the array of
    "intermediate" z variables is not NULL, whereas
    a temporary BDD manager is created (and destroyed when
    the job is done) if z is NULL, to avoid
    creating new variables in the original manager.
    Primary input variables (NON quantifying and NON state
    variables) are not taken into account. This means that they
    are NOT duplicated at every step as in standart squaring.
    They should (since they could) be quantifyed out from a
    monolithic or disjunctively partitioned TR. Otherwise the
    algorithm only closes paths with constant input values.
    ]

  SideEffects [A Ddi Manager (a BDD manager) is temporarily allocated
    if required for all the operations involved, if
    the auxiliary set of variables (Zs) must be created.]

  SeeAlso     [TrBuildTransClosure]

******************************************************************************/

Tr_Tr_t *
Tr_TransClosure(
  Tr_Tr_t * tr
)
{
  Tr_Mgr_t *trMgr;              /* Tr manager */
  Ddi_Bdd_t *Tsq, *TsqAux;
  Ddi_Bdd_t *trAux;
  Ddi_Var_t *var;
  int i, level;
  int ns;                       /* number of state vars */

  Ddi_Bdd_t *trTr;              /* Transition Relation */

  Ddi_Mgr_t *ddTr;              /* original dd manager */
  Ddi_Mgr_t *ddAux;             /* aux dd manager */
  Ddi_Vararray_t *sAux;         /* present state vars */
  Ddi_Vararray_t *yAux;         /* next state vars */
  Ddi_Vararray_t *zAux;         /* compositional state vars */
  int extRef;

  trMgr = tr->trMgr;
  trTr = Tr_TrBdd(tr);

  ddTr = Ddi_ReadMgr(trTr);

  extRef = Ddi_MgrReadExtRef(ddTr);

  ns = Ddi_VararrayNum(trMgr->ps);

  /* an auxiliary dd manager is created */
  ddAux = Ddi_MgrDup(ddTr);

  /* temporarily disable dynamic reordering */
  Ddi_MgrSiftSuspend(ddAux);

  /* input parameters are copied to the ddAux manager */
  trAux = Ddi_BddCopy(ddAux, trTr);
  sAux = Ddi_VararrayCopy(ddAux, trMgr->ps);
  yAux = Ddi_VararrayCopy(ddAux, trMgr->ns);

  /* create new array of variables (z) */
  zAux = Ddi_VararrayAlloc(ddAux, ns);
  for (i = 0; i < ns; i++) {
    level = Ddi_VarCurrPos(Ddi_VararrayRead(sAux, i));
    var = Ddi_VarNewAtLevel(ddAux, level);
    Ddi_VararrayWrite(zAux, i, var);
    if (level > Ddi_VarCurrPos(Ddi_VararrayRead(yAux, i)))
      var = Ddi_VararrayRead(yAux, i);
    Ddi_VarMakeGroup(ddAux, var, 3);
  }

  Ddi_MgrSiftResume(ddAux);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "Tr Closure - created z vars: "); Ddi_PrintVararray(zAux)
    );

  TsqAux = TrBuildTransClosure(trMgr, trAux, sAux, yAux, zAux);

  Ddi_MgrAlign(ddTr, ddAux);

  Tsq = Ddi_BddCopy(ddTr, TsqAux);

  Ddi_Free(TsqAux);
  Ddi_Free(trAux);
  Ddi_Free(sAux);
  Ddi_Free(yAux);
  Ddi_Free(zAux);

  Ddi_MgrQuit(ddAux);

  TrTrRelWrite(tr, Tsq);

  Ddi_Free(Tsq);

  Ddi_MgrCheckExtRef(ddTr, extRef);

  return (tr);
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Build transitive closure of a Transition Relation
    by means of iterative squaring]

  Description [Internal procedure to build the transitive closure of
    a transition relation. This can now handle a Monolithic TR
    as well as a disjunctively partitioned one. In the latter case
    closure is only partial: it is operated within single partitions.
    The iterative squaring method, expressed by
    the following recurrence equations:
    <p>
    T(0)(s,y) = TR(s,y) <br>
    T(i+1)(s,y)= TR(s,y) + Exist(z) ( T(i)(s,z) * T(i)(z,y) )
    <p>
    The least fixed point is T* (<em>transitive closure</em>)
    and the number of iterations required to compute T* is
    logarithmic in the sequential depth of TR (the diameter of
    the state transition graph represented by TR).<br>
    The transitive closure describes the pairs of states that are
    connected by at least one path in the state graph of FSM.]

  SideEffects []

  SeeAlso     [TrCompose]

******************************************************************************/

Ddi_Bdd_t *
TrBuildTransClosure(
  Tr_Mgr_t * trMgr /* TR manager */ ,
  Ddi_Bdd_t * TR /* Transition Relation */ ,
  Ddi_Vararray_t * s /* Present state vars */ ,
  Ddi_Vararray_t * y /* Next state vars */ ,
  Ddi_Vararray_t * z            /* Intermediate state vars */
)
{
  Ddi_Varset_t *suppZ;          /* Set of new variables z */
  Ddi_Bdd_t *TR_i, *TC, *TC0, *Tsq, *Tsq_i;
  Ddi_Bdd_t *temp;
  int i, size,                  /* size of T.C. */
   peak = 0;                    /* max size of T.C. */
  long step;                    /* number of iterations */
  int maxIter;                  /* max allowed squaring iterations */
  Ddi_Mgr_t *dd;                /* dd manager */

  int extRef;

  dd = Ddi_ReadMgr(TR);

  extRef = Ddi_MgrReadExtRef(dd);

  TC0 = NULL;

  switch (Ddi_ReadCode(TR)) {
    case Ddi_Bdd_Part_Disj_c:
      /*
       *  Disjunctively partitioned TR - local closure of partition
       *  Only partial closure computed !
       */
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "Squaring of a disjunctively partitioned TR.\n");
        fprintf(stdout, "Each partition is closed locally.\n");
        fprintf(stdout, "Only a partial closure is globally obtained.\n")
        );
      Tsq = Ddi_BddMakePartDisjVoid(dd);
      for (i = 0; i < Ddi_BddPartNum(TR); i++) {
        TR_i = Ddi_BddPartRead(TR, i);
        Tsq_i = TrBuildTransClosure(trMgr, TR_i, s, y, z);
        Ddi_BddPartInsertLast(Tsq, Tsq_i);
        Ddi_Free(Tsq_i);
      }
      Ddi_MgrCheckExtRef(dd, extRef + 1);
      return (Tsq);
      break;
    case Ddi_Bdd_Meta_c:
      Pdtutil_Assert(0, "Closure: operation not supported for meta BDD");
      break;
    case Ddi_Bdd_Mono_c:
    case Ddi_Bdd_Part_Conj_c:
      break;
    default:
      Pdtutil_Assert(0, "Wrong DDI node code");
      break;
  }

  suppZ = Ddi_VarsetMakeFromArray(z);

  /* TC is the relation to be composed */
  TC = Ddi_BddMakeMono(TR);

  /* TC0 is the relation to be or-ed after composition */
  switch (Tr_MgrReadSquaringMethod(trMgr)) {
    case TR_SQUARING_FULL:
      TC0 = Ddi_BddDup(TC);
      break;
    case TR_SQUARING_POWER:
      TC0 = Ddi_BddDup(Ddi_MgrReadZero(dd));
      break;
  }

  /*-------------------- START Iterative squaring --------------------------*/

  maxIter = Tr_MgrReadMaxIter(trMgr);
  Tsq = Ddi_BddDup(Ddi_MgrReadOne(dd));

  for (step = 1; (maxIter < 0) || (step <= maxIter); step++) {

    /* composition */
    temp = TrCompose(trMgr, TC, TC, s, y, z, suppZ);
    /* or with TC0 */
    Ddi_BddOrAcc(temp, TC0);
    if (Ddi_BddEqual(temp, TC)) {   /* fix point reached */
      Ddi_Free(temp);
      break;
    }

    if (Tr_MgrReadVerbosity(trMgr) >= Pdtutil_VerbLevelDevMed_c) {
      size = Ddi_BddSize(TC);
      if (size > peak) {
        peak = size;            /* remember the max value of size */
      }
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "It.Squaring step %ld : [ T.C. = %d[%d] ].\n", step,
          size, Ddi_BddSize(Tsq))
        );
    }

    Ddi_Free(TC);
    TC = temp;

    /* Tsq is the upgraded closure */
    switch (Tr_MgrReadSquaringMethod(trMgr)) {
      case TR_SQUARING_FULL:
        Ddi_Free(Tsq);
        Tsq = Ddi_BddDup(TC);
        break;
      case TR_SQUARING_POWER:
        Ddi_BddOrAcc(TC, Tsq);
        break;
    }

  }
  /*----------------------- END Iterative squaring -------------------------*/

  Ddi_Free(suppZ);

  Ddi_Free(TC);
  Ddi_Free(TC0);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMed_c,
    Pdtutil_ChrPrint(stdout, '*', 50);
    fprintf(stdout, "Iterative Squaring Results Summary:.\n");
    fprintf(stdout, "# Size of Transitive Closure = %d.\n", Ddi_BddSize(Tsq));
    fprintf(stdout, "# Number of iterations = %ld.\n", step - 1);
    fprintf(stdout, "# |TC| peak = %d\n.\n", peak);
    Pdtutil_ChrPrint(stdout, '*', 50); fprintf(stdout, "\n")
    );

  Ddi_MgrCheckExtRef(dd, extRef + 1);

  return (Tsq);
}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Compute : EXIST(z) (f(s,z) * g(z,y) )]

  Description [Computes the relational composition of f(s,z) and g(z,y),
    after proper variable substitutions on the initial
    f(s,y): initially both f and g are expressed as functions
    of (s,y).
    This is the basic step of iterative squaring using monolithic
    transition relations.
    ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static Ddi_Bdd_t *
TrCompose(
  Tr_Mgr_t * trMgr /* Tr Manager */ ,
  Ddi_Bdd_t * f /* first function */ ,
  Ddi_Bdd_t * g /* second function */ ,
  Ddi_Vararray_t * s /* array of s variables */ ,
  Ddi_Vararray_t * y /* array of y variables */ ,
  Ddi_Vararray_t * z /* array of z variables */ ,
  Ddi_Varset_t * suppZ          /* z variables as a set */
)
{
  Ddi_Bdd_t *f1, *g1, *fSq;
  int extRef = Ddi_MgrReadExtRef(Ddi_ReadMgr(f));

  f1 = Ddi_BddSwapVars(f, z, y);
  g1 = Ddi_BddSwapVars(g, z, s);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMax_c,
    fprintf(stdout, "Composing: [%d]*[%d] -> ",
      Ddi_BddSize(f1), Ddi_BddSize(g1))
    );

  fSq = Ddi_BddAndExist(f1, g1, suppZ);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMax_c,
    fprintf(stdout, "[%d].\n", Ddi_BddSize(fSq))
    );

  Ddi_Free(f1);
  Ddi_Free(g1);

  Ddi_MgrCheckExtRef(Ddi_ReadMgr(f), extRef + 1);

  return (fSq);
}

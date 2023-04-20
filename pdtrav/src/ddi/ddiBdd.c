/**CFile***********************************************************************

  FileName    [ddiBdd.c]

  PackageName [ddi]

  Synopsis    [Functions working on Boolean functions (Ddi_Bdd_t)]

  Description [Functions working on Boolean functions represented by the
    Ddi_Bdd_t type. Type Ddi_Bdd_t is used for BDDs in monolitic,
    partitioned (conjunctive/disjunctive) and metaBDD formats.
    Internally, functions are implemented using handles (wrappers) pointing
    to BDD roots. Externally, they are accessed only by pointers.
    Type Ddi_Bdd_t is cast to Ddi_Generic_t (generic DDI node) for internal
    operations.
    All the results obtained by operations are <B>implicitly</B>
    allocated or referenced (CUDD nodes), so explicit freeing is required.<br>
    External procedures in this module include
    <ul>
    <li> Basic Boolean operations: And, Or, ..., ITE
    <li> Specific BDD operators: Constrain, Restrict
    <li> Quantification operators: Exist, And-Exist (relational product)
    <li> Comparison operators: Equality, Inclusion, Tautology checks
    <li> Manipulation of (disjunctively and/or conjunctively)
         partitioned BDDs: create, add/remove partitions,
    <li> translation from/to CUDD BDDs.
    <li> Dumping on file, based on the DDDMP format, distributed
         with CUDD.
    </ul>
    ]

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

static Ddi_Bdd_t * DdiBddVarFunctionalDep(Ddi_Bdd_t *f, Ddi_Var_t *v);
static Ddi_Bdd_t * DdiBddExistOverApproxAccIntern(Ddi_Bdd_t *f, Ddi_Varset_t *sm, int threshold);
static Ddi_Varsetarray_t * DdiBddTightOverapprox(Ddi_Bdd_t *f, int thresh);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/*
 *  Conversion, creation (make) and release (free) functions
 */

/**Function********************************************************************
  Synopsis    [Build a Ddi_Bdd_t from a given CUDD node.]
  Description [Build the Ddi_Bdd_t structure (by means of DdiGenericAlloc)
               from manager and CUDD node.
               The reference count of the node is increased.]
  SideEffects []
  SeeAlso     [DdiGenericAlloc Ddi_BddToCU]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeFromCU (
  Ddi_Mgr_t *mgr,
  DdNode *bdd
)
{
  Ddi_Bdd_t *r;

  Pdtutil_Assert((mgr!=NULL)&&(bdd!=NULL),
    "NULL manager or BDD when generating DDI node");

  r = (Ddi_Bdd_t *)DdiGenericAlloc(Ddi_Bdd_c,mgr);
  r->common.code = Ddi_Bdd_Mono_c;
  r->data.bdd = bdd;
  Cudd_Ref(bdd);

  return (r);
}

/**Function********************************************************************
  Synopsis    [Convert a DDI function to the corresponding Cudd Node]
  Description [Convert a DDI function to the corresponding Cudd Node.
    This is done by reading the proper field (pointing to a cudd node) in the
    DDI node. No ref is done on the returned node.]
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
Ddi_BddNode *
Ddi_BddToCU(
  Ddi_Bdd_t *f
)
{
  DdiConsistencyCheck(f,Ddi_Bdd_c);
  Pdtutil_Assert(f->common.code == Ddi_Bdd_Mono_c,
    "converting to CU a non monolitic BDD. Use proper MakeMono function!");
  return (f->data.bdd);
}

/**Function********************************************************************
  Synopsis    [Generate a literal from a variable]
  Description [Generate a literal from a variable.
    The literal can be either affirmed or complemented.
    ]
  SideEffects [none]
  SeeAlso     [Ddi_Bdd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeLiteral (
  Ddi_Var_t *v,
  int polarity    /* non 0: affirmed (v), 0: complemented literal (!v) */
)
{
  DdiConsistencyCheck(v,Ddi_Var_c);
  if (Ddi_VarIsAig (v)) {
    DdiVarSetCU(v);
  }
  if (polarity) {
    return (Ddi_BddMakeFromCU(Ddi_ReadMgr(v),Ddi_VarToCU(v)));
  }
  else {
    return (Ddi_BddMakeFromCU(Ddi_ReadMgr(v),Cudd_Not(Ddi_VarToCU(v))));
  }
}

/**Function********************************************************************
  Synopsis    [Generate a literal from a variable]
  Description [Generate a literal from a variable.
    The literal can be either affirmed or complemented.
    ]
  SideEffects [none]
  SeeAlso     [Ddi_Bdd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeLiteralAigOrBdd (
  Ddi_Var_t *v,
  int polarity    /* non 0: affirmed (v), 0: complemented literal (!v) */,
  int doAig
)
{
  if (doAig) return Ddi_BddMakeLiteralAig (v,polarity);
  else return Ddi_BddMakeLiteral (v,polarity);
}

/**Function********************************************************************
  Synopsis    [Build a Ddi_Bdd_t from a given BAIG node.]
  Description [Build the Ddi_Bdd_t structure (by means of DdiGenericAlloc)
               from manager and BAIG node.
               The reference count of the node is increased.]
  SideEffects []
  SeeAlso     [DdiGenericAlloc Ddi_BddToBaig]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeFromBaig (
  Ddi_Mgr_t *mgr,
  bAigEdge_t bAigNode
)
{
  Ddi_Bdd_t *r;

  Pdtutil_Assert((mgr!=NULL),
    "NULL manager or BDD when generating DDI node");

  r = (Ddi_Bdd_t *)DdiGenericAlloc(Ddi_Bdd_c,mgr);
  r->common.code = Ddi_Bdd_Aig_c;
  r->data.aig = DdiAigMakeFromBaig(mgr->aig.mgr,bAigNode);

  return (r);
}

/**Function********************************************************************
  Synopsis    [Convert a DDI function to the corresponding Cudd Node]
  Description [Convert a DDI function to the corresponding Cudd Node.
    This is done by reading the proper field (pointing to a cudd node) in the
    DDI node. No ref is done on the returned node.]
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
bAigEdge_t
Ddi_BddToBaig(
  Ddi_Bdd_t *f
)
{
  DdiConsistencyCheck(f,Ddi_Bdd_c);
  Pdtutil_Assert(f->common.code == Ddi_Bdd_Aig_c,
    "converting to Baig a non AIG BDD !");
  return (DdiAigToBaig(f->data.aig));
}

/**Function********************************************************************
  Synopsis    [Generate an Aig literal from a variable]
  Description [Generate an Aig literal from a variable.
    The literal can be either affirmed or complemented.
    ]
  SideEffects [none]
  SeeAlso     [Ddi_Bdd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeLiteralAig (
  Ddi_Var_t *v,
  int polarity    /* non 0: affirmed (v), 0: complemented literal (!v) */
)
{
  bAigEdge_t litBaig;
  Ddi_Bdd_t *lit;
  DdiConsistencyCheck(v,Ddi_Var_c);
  Pdtutil_Assert(Ddi_VarName(v)!=NULL,"Var name required for AIG");
  litBaig = bAig_CreateVarNode(Ddi_ReadMgr(v)->aig.mgr,Ddi_VarName(v));
  lit = Ddi_BddMakeFromBaig(Ddi_ReadMgr(v),litBaig);
  if (!polarity) {
    Ddi_BddNotAcc(lit);
  }
  return(lit);
}


/**Function********************************************************************
  Synopsis    [Generate an Aig literal from a variable]
  Description [Generate an Aig literal from a variable.
    The literal can be either affirmed or complemented.
    ]
  SideEffects [none]
  SeeAlso     [Ddi_Bdd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeConstAig (
  Ddi_Mgr_t *mgr,
  int value    /* non 0: true (one), 0: false (zero) */
)
{
  bAigEdge_t rBaig;
  Ddi_Bdd_t *r;
  rBaig = value ? bAig_One : bAig_Zero;
  r = Ddi_BddMakeFromBaig(mgr,rBaig);
  return(r);
}

/**Function********************************************************************
  Synopsis    [Generate a Ddi_Bdd_t constant node (BDD zero or one)]
  Description [Generate a Ddi_Bdd_t constant node (BDD zero or one).
    The proper constant node within the manager is duplicated.
  ]
  SideEffects []
  SeeAlso     [Ddi_BddToCU DdiBddMakeLiteral]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeConst (
  Ddi_Mgr_t *mgr,
  int value    /* non 0: true (one), 0: false (zero) */
)
{
  if (value) {
    return (Ddi_BddDup(Ddi_MgrReadOne(mgr)));
  }
  else {
    return (Ddi_BddDup(Ddi_MgrReadZero(mgr)));
  }
}

/**Function********************************************************************
  Synopsis    [Build a conjunctively partitioned BDD from a monolithic BDD]
  Description [Build a conjunctively partitioned BDD from a monolithic BDD]
  SideEffects []
  SeeAlso     [Ddi_BddMakePartDisjFromMono Ddi_BddMakePartConjFromArray
    Ddi_BddMakePartDisjFromArray]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakePartConjFromMono (
  Ddi_Bdd_t *mono
)
{
  Ddi_Bdd_t *r;
  Ddi_Mgr_t *mgr;

  mgr = Ddi_ReadMgr(mono);

  r = (Ddi_Bdd_t *)DdiGenericAlloc(Ddi_Bdd_c,mgr);
  r->common.code = Ddi_Bdd_Part_Conj_c;
  r->data.part = DdiArrayAlloc(1);
  DdiArrayWrite(r->data.part,0,(Ddi_Generic_t *)mono,Ddi_Dup_c);

  return(r);
}

/**Function********************************************************************
  Synopsis    [Build a conjunctively partitioned BDD with 0 partitions]
  Description [Build a conjunctively partitioned BDD with 0 partitions]
  SideEffects []
  SeeAlso     [Ddi_BddMakePartDisjVoid Ddi_BddMakePartConjFromMono
    Ddi_BddMakePartConjFromArray]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakePartConjVoid (
  Ddi_Mgr_t *mgr
)
{
  Ddi_Bdd_t *r;

  r = (Ddi_Bdd_t *)DdiGenericAlloc(Ddi_Bdd_c,mgr);
  r->common.code = Ddi_Bdd_Part_Conj_c;
  r->data.part = DdiArrayAlloc(0);

  return(r);
}

/**Function********************************************************************
  Synopsis    [Build a disjunctively partitioned BDD with 0 partitions]
  Description [Build a disjunctively partitioned BDD with 0 partitions]
  SideEffects []
  SeeAlso     [Ddi_BddMakePartConjVoid Ddi_BddMakePartDisjFromMono
    Ddi_BddMakePartDisjFromArray]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakePartDisjVoid (
  Ddi_Mgr_t *mgr
)
{
  Ddi_Bdd_t *r;

  r = (Ddi_Bdd_t *)DdiGenericAlloc(Ddi_Bdd_c,mgr);
  r->common.code = Ddi_Bdd_Part_Disj_c;
  r->data.part = DdiArrayAlloc(0);

  return(r);
}

/**Function********************************************************************
  Synopsis    [Build a disjunctively partitioned BDD from a monolithic BDD]
  Description [Build a disjunctively partitioned BDD from a monolithic BDD]
  SideEffects []
  SeeAlso     [Ddi_BddMakePartConjFromMono Ddi_BddMakePartConjFromArray
    Ddi_BddMakePartDisjFromArray]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakePartDisjFromMono (
  Ddi_Bdd_t *mono
)
{
  Ddi_Bdd_t *r;
  Ddi_Mgr_t *mgr;

  mgr = Ddi_ReadMgr(mono);

  r = (Ddi_Bdd_t *)DdiGenericAlloc(Ddi_Bdd_c,mgr);
  r->common.code = Ddi_Bdd_Part_Disj_c;
  r->data.part = DdiArrayAlloc(1);
  DdiArrayWrite(r->data.part,0,(Ddi_Generic_t *)mono,Ddi_Dup_c);

  return(r);
}

/**Function********************************************************************
  Synopsis    [Build a conjunctively partitioned BDD from array of partitions]
  Description [Build a conjunctively partitioned BDD from array of partitions]
  SideEffects []
  SeeAlso     [Ddi_BddMakePartConjFromMono Ddi_BddMakePartDisjFromMono
    Ddi_BddMakePartDisjFromArray]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakePartConjFromArray (
  Ddi_Bddarray_t *array
)
{
  Ddi_Bdd_t *r;

  r = (Ddi_Bdd_t *)DdiGenericAlloc(Ddi_Bdd_c,Ddi_ReadMgr(array));
  r->common.code = Ddi_Bdd_Part_Conj_c;
  r->data.part = DdiArrayDup(array->array);

  return(r);
}

/**Function********************************************************************
  Synopsis    [Build a disjunctively partitioned BDD from array of BDDs]
  Description [Build a disjunctively partitioned BDD from array of BDDs]
  SideEffects []
  SeeAlso     [Ddi_BddMakePartConjFromMono Ddi_BddMakePartDisjFromMono
    Ddi_BddMakePartConjFromArray]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakePartDisjFromArray (
  Ddi_Bddarray_t *array
)
{
  Ddi_Bdd_t *r;

  r = (Ddi_Bdd_t *)DdiGenericAlloc(Ddi_Bdd_c,Ddi_ReadMgr(array));
  r->common.code = Ddi_Bdd_Part_Disj_c;
  r->data.part = DdiArrayDup(array->array);

  return(r);
}

/**Function********************************************************************
  Synopsis    [Convert a BDD to conjunctively partitioned (if required).
               Result accumulated]
  Description [Convert a BDD to conjunctively partitioned (if required).
               Result accumulated]
  SideEffects []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddSetPartConj (
  Ddi_Bdd_t *f
)
{
  Ddi_Bdd_t *m;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  switch (f->common.code) {
  case Ddi_Bdd_Part_Conj_c:
    return (f);
  case Ddi_Bdd_Meta_c:
    Ddi_BddSetPartConjFromMeta(f);
    break;
  case Ddi_Bdd_Aig_c:
    m = Ddi_BddDup(f);
    DdiAigFree(f->data.aig);
    f->common.code = Ddi_Bdd_Part_Conj_c;
    f->data.part = DdiArrayAlloc(0);
    Ddi_BddPartInsertLast(f,m);
    Ddi_Free(m);
    break;
  case Ddi_Bdd_Mono_c:
    m = Ddi_BddMakePartConjFromMono(f);
    Cudd_RecursiveDeref (Ddi_MgrReadMgrCU(Ddi_ReadMgr(f)), f->data.bdd);
    f->common.code = Ddi_Bdd_Part_Conj_c;
    f->data.part = DdiArrayDup(m->data.part);
    Ddi_Free(m);
    break;
  case Ddi_Bdd_Part_Disj_c:
    m = Ddi_BddMakePartConjFromMono(f);
    DdiArrayFree (f->data.part);
    f->common.code = Ddi_Bdd_Part_Conj_c;
    f->data.part = DdiArrayDup(m->data.part);
    Ddi_Free(m);
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }
  return(f);
}

/**Function********************************************************************
  Synopsis    [Convert a BDD to disjunctively partitioned (if required).
               Result accumulated]
  Description [Convert a BDD to disjunctively partitioned (if required).
               Result accumulated]
  SideEffects []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddSetPartDisj (
  Ddi_Bdd_t *f
)
{
  Ddi_Bdd_t *m;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  switch (f->common.code) {
  case Ddi_Bdd_Part_Disj_c:
    return (f);
  case Ddi_Bdd_Aig_c:
    m = Ddi_BddDup(f);
    DdiAigFree(f->data.aig);
    f->common.code = Ddi_Bdd_Part_Disj_c;
    f->data.part = DdiArrayAlloc(0);
    Ddi_BddPartInsertLast(f,m);
    Ddi_Free(m);
    break;
  case Ddi_Bdd_Meta_c:
    m = Ddi_BddMakePartDisjFromMono(f);
    DdiMetaFree(f->data.meta);
    f->common.code = Ddi_Bdd_Part_Disj_c;
    f->data.part = DdiArrayDup(m->data.part);
    Ddi_Free(m);
    break;
  case Ddi_Bdd_Mono_c:
    m = Ddi_BddMakePartDisjFromMono(f);
    Cudd_RecursiveDeref (Ddi_MgrReadMgrCU(Ddi_ReadMgr(f)), f->data.bdd);
    f->common.code = Ddi_Bdd_Part_Disj_c;
    f->data.part = DdiArrayDup(m->data.part);
    Ddi_Free(m);
    break;
  case Ddi_Bdd_Part_Conj_c:
    m = Ddi_BddMakePartDisjFromMono(f);
    DdiArrayFree (f->data.part);
    f->common.code = Ddi_Bdd_Part_Disj_c;
    f->data.part = DdiArrayDup(m->data.part);
    Ddi_Free(m);
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }
  return(f);
}

/**Function********************************************************************
  Synopsis    [Generate a Ddi_Bdd_t relation from array of functions]
  Description [Generate a Ddi_Bdd_t relation from array of functions.
    Relation is generated considering function variables domain,
    and range variables as co-domain.
    I-th range variable corresponds to i-th function.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddRelMakeFromArray (
  Ddi_Bddarray_t *Fa        /* array of functions */,
  Ddi_Vararray_t *Va        /* array of range variables */
)
{
  Ddi_Mgr_t *ddiMgr;
  Ddi_Bdd_t *rel, *r_i, *lit;
  int i, n;
  int isAig = 0;

  DdiConsistencyCheck(Fa,Ddi_Bddarray_c);
  DdiConsistencyCheck(Va,Ddi_Vararray_c);

  n = Ddi_BddarrayNum(Fa);
  ddiMgr = Ddi_ReadMgr(Fa);
  Pdtutil_Assert (n == Ddi_VararrayNum(Va),
    "Number of range variables does not match number of functions.");
  Pdtutil_Assert (ddiMgr == Ddi_ReadMgr(Va),
    "Different managers found while generating relation from functions");

  rel = Ddi_BddMakePartConjVoid(ddiMgr);
  if (n>0) {
    isAig = Ddi_BddIsAig(Ddi_BddarrayRead(Fa,0));
  }

  for (i=0; i<n; i++) {
    lit = isAig ? Ddi_BddMakeLiteralAig(Ddi_VararrayRead(Va,i),1)
                : Ddi_BddMakeLiteral(Ddi_VararrayRead(Va,i),1);
    r_i = Ddi_BddXnor(lit,Ddi_BddarrayRead(Fa,i));
    Ddi_Free (lit);
    Ddi_BddPartInsertLast(rel,r_i);
    Ddi_Free (r_i);
  }

  return (rel);
}

/**Function********************************************************************
  Synopsis    [Generate a Ddi_Bdd_t relation from array of functions]
  Description [Generate a Ddi_Bdd_t relation from array of functions.
    Relation is generated considering function variables domain,
    and range variables as co-domain.
    I-th range variable corresponds to i-th function.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddRelMakeFromArrayFiltered (
  Ddi_Bddarray_t *Fa        /* array of functions */,
  Ddi_Vararray_t *Va        /* array of range variables */,
  Ddi_Vararray_t *filter    /* array of filter variables */
)
{
  Ddi_Mgr_t *ddiMgr;
  Ddi_Bdd_t *rel, *r_i, *lit;
  int i, n;
  int isAig = 0;

  DdiConsistencyCheck(Fa,Ddi_Bddarray_c);
  DdiConsistencyCheck(Va,Ddi_Vararray_c);

  n = Ddi_BddarrayNum(Fa);
  ddiMgr = Ddi_ReadMgr(Fa);
  Pdtutil_Assert (n == Ddi_VararrayNum(Va),
    "Number of range variables does not match number of functions.");
  Pdtutil_Assert (ddiMgr == Ddi_ReadMgr(Va),
    "Different managers found while generating relation from functions");

  rel = Ddi_BddMakePartConjVoid(ddiMgr);
  if (n>0) {
    isAig = Ddi_BddIsAig(Ddi_BddarrayRead(Fa,0));
  }

  Ddi_VararrayWriteMark (filter, 1);
  for (i=0; i<n; i++) {
    Ddi_Var_t *v_i = Ddi_VararrayRead(Va,i);
    if (Ddi_VarReadMark(v_i)==0) continue;
    lit = isAig ? Ddi_BddMakeLiteralAig(v_i,1)
                : Ddi_BddMakeLiteral(v_i,1);
    r_i = Ddi_BddXnor(lit,Ddi_BddarrayRead(Fa,i));
    Ddi_Free (lit);
    Ddi_BddPartInsertLast(rel,r_i);
    Ddi_Free (r_i);
  }
  Ddi_VararrayWriteMark (filter, 0);

  return (rel);
}

/**Function********************************************************************
  Synopsis    [Generate a Ddi_Bdd_t relation from array of functions]
  Description [Generate a Ddi_Bdd_t relation from array of functions.
    Relation is generated considering function variables domain,
    and range variables as co-domain.
    I-th range variable corresponds to i-th function.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMiterMakeFromArray (
  Ddi_Bddarray_t *Fa        /* array of functions */,
  Ddi_Bddarray_t *Fb        /* array of functions */
)
{
  Ddi_Mgr_t *ddiMgr;
  Ddi_Bdd_t *rel, *r_i, *lit;
  int i, n;
  int isAig = 0;

  DdiConsistencyCheck(Fa,Ddi_Bddarray_c);
  DdiConsistencyCheck(Fb,Ddi_Bddarray_c);

  n = Ddi_BddarrayNum(Fa);
  ddiMgr = Ddi_ReadMgr(Fa);
  Pdtutil_Assert (n == Ddi_BddarrayNum(Fb),
    "Number of array functions does not match.");
  Pdtutil_Assert (ddiMgr == Ddi_ReadMgr(Fb),
    "Different managers found while generating relation from functions");

  rel = Ddi_BddMakePartConjVoid(ddiMgr);

  for (i=0; i<n; i++) {
    r_i = Ddi_BddXnor(Ddi_BddarrayRead(Fa,i),Ddi_BddarrayRead(Fb,i));
    Ddi_BddPartInsertLast(rel,r_i);
    Ddi_Free (r_i);
  }

  return (rel);
}

/**Function********************************************************************
  Synopsis    [Generate an equivalence function from an array of vars and funcs]
  Description [Generate an equivalence function from an array of vars and funcs.
The Bdd represents the equiv relation. vars and subst arrays are kept in the 
info structure.
]
  SideEffects [none]
  SeeAlso     [Ddi_Bdd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeEq (
  Ddi_Vararray_t *vars        /* array of variables */,
  Ddi_Bddarray_t *subst       /* array of functions */
)
{
  Ddi_Bdd_t *eqBdd;
  DdiConsistencyCheck(vars,Ddi_Vararray_c);
  DdiConsistencyCheck(subst,Ddi_Bddarray_c);

  eqBdd = Ddi_BddRelMakeFromArray (subst,vars);
  eqBdd->common.info = Pdtutil_Alloc(Ddi_Info_t,1);
  eqBdd->common.info->infoCode = Ddi_Info_Eq_c;
  eqBdd->common.info->eq.mark = 0;
  eqBdd->common.info->eq.vars = Ddi_VararrayDup(vars);
  Ddi_Lock(eqBdd->common.info->eq.vars);
  eqBdd->common.info->eq.subst = Ddi_BddarrayDup(subst);
  Ddi_Lock(eqBdd->common.info->eq.subst);

  return(eqBdd);
}

/**Function********************************************************************
  Synopsis    [Generate a composed function from decomposed components: 
  func, array of vars and funcs]
  Description [Generate a composed function from decomposed components: 
  func, array of vars and funcs. Decomposed func, vars and subst arrays 
  are kept in the info structure.]
  SideEffects [none]
  SeeAlso     [Ddi_Bdd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeCompose (
  Ddi_Bdd_t *f                /* top level compose */,
  Ddi_Vararray_t *refVars     /* array of reference variables */,
  Ddi_Vararray_t *vars        /* array of variables */,
  Ddi_Bddarray_t *subst       /* array of functions */,
  Ddi_Bdd_t *care,
  Ddi_Bdd_t *constr,
  Ddi_Bdd_t *cone
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  Ddi_Bdd_t *composedBdd;
  Ddi_Vararray_t *varsSupp, *vars2=NULL, *refv2=NULL;
  Ddi_Bddarray_t *subst2=NULL;
  int i;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  DdiConsistencyCheck(subst,Ddi_Bddarray_c);

  if (vars!=NULL) {
    DdiConsistencyCheck(vars,Ddi_Vararray_c);

    varsSupp = Ddi_BddSuppVararray (f);
    Ddi_VararrayWriteMark (varsSupp,1);
    vars2 = Ddi_VararrayAlloc (ddm,0);
    subst2 = Ddi_BddarrayAlloc(ddm,0);
    if (refVars!=NULL) {
      refv2 = Ddi_VararrayAlloc (ddm,0);
    }
    for (i=0; i<Ddi_VararrayNum(vars); i++) {
      Ddi_Var_t *v_i = Ddi_VararrayRead(vars,i);
      Ddi_Bdd_t *s_i = Ddi_BddarrayRead(subst,i);
      if (Ddi_VarReadMark(v_i)==1) {
        Ddi_VararrayInsertLast(vars2,v_i);
        Ddi_BddarrayInsertLast(subst2,s_i);
        if (refv2!=NULL) {
          Ddi_Var_t *rv_i = Ddi_VararrayRead(refVars,i);
          Ddi_VararrayInsertLast(refv2,rv_i);
        }
      }
    }
    Ddi_VararrayWriteMark (varsSupp,0);
    Ddi_Free(varsSupp);
    composedBdd = Ddi_BddCompose (f,vars2,subst2);
  }
  else {
    composedBdd = Ddi_BddDup(f);
    subst2 = Ddi_BddarrayDup(subst);
  }
  
  if (constr!=NULL && !Ddi_BddIsOne(constr)) {
    Ddi_BddAndAcc(composedBdd,constr);
  }
  if (cone!=NULL && !Ddi_BddIsZero(cone)) {
    Ddi_BddOrAcc(composedBdd,cone);
  }
  composedBdd->common.info = Pdtutil_Alloc(Ddi_Info_t,1);
  composedBdd->common.info->infoCode = Ddi_Info_Compose_c;
  composedBdd->common.info->compose.mark = 0;
  composedBdd->common.info->compose.f = Ddi_BddDup(f);
  Ddi_Lock(composedBdd->common.info->compose.f);

  composedBdd->common.info->compose.care = NULL;
  composedBdd->common.info->compose.constr = NULL;
  composedBdd->common.info->compose.cone = NULL;
  composedBdd->common.info->compose.vars = NULL;
  composedBdd->common.info->compose.refVars = NULL;

  if (care != NULL) {
    composedBdd->common.info->compose.care = Ddi_BddDup(care);
    Ddi_Lock(composedBdd->common.info->compose.care);
  }
  if (constr != NULL) {
    composedBdd->common.info->compose.constr = Ddi_BddDup(constr);
    Ddi_Lock(composedBdd->common.info->compose.constr);
  }
  if (cone!=NULL) {
    composedBdd->common.info->compose.cone = Ddi_BddDup(cone);
    Ddi_Lock(composedBdd->common.info->compose.cone);
  }
  else {
    composedBdd->common.info->compose.cone = NULL;
  }
  if (vars != NULL) {
    composedBdd->common.info->compose.vars = Ddi_VararrayDup(vars2);
    Ddi_Lock(composedBdd->common.info->compose.vars);
  }
  if (refv2!=NULL) {
    composedBdd->common.info->compose.refVars = Ddi_VararrayDup(refv2);
    Ddi_Lock(composedBdd->common.info->compose.refVars);
  }
  composedBdd->common.info->compose.subst = Ddi_BddarrayDup(subst2);
  Ddi_Lock(composedBdd->common.info->compose.subst);

  Ddi_Free(refv2);
  Ddi_Free(vars2);
  Ddi_Free(subst2);

  return(composedBdd);
}



/**Function********************************************************************
  Synopsis    [Build a conjunctively partitioned BDD from array of partitions]
  Description [Build a conjunctively partitioned BDD from array of partitions]
  SideEffects []
  SeeAlso     [Ddi_BddMakePartConjFromMono Ddi_BddMakePartDisjFromMono
    Ddi_BddMakePartDisjFromArray]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakePartConjFromCube (
  Ddi_Bdd_t *cube
)
{
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(cube);
  int i, n, isAig = Ddi_BddIsAig(cube);
  Ddi_Bdd_t *litBdd, *cubeBdd = Ddi_BddMakeMono(cube);
  Ddi_Varset_t *supp;
  Ddi_Vararray_t *vars;
  Ddi_Bdd_t *r;

  Pdtutil_Assert(Ddi_BddIsCube(cubeBdd),"CUBE required");
  supp = Ddi_BddSupp(cubeBdd);
  vars = Ddi_VararrayMakeFromVarset(supp,1);
  Ddi_Free(supp);
  n = Ddi_VararrayNum(vars);
  r = Ddi_BddMakePartConjVoid(ddiMgr);
  for (i=0; i<n; i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(vars,i);
    Ddi_Bdd_t *c0 = Ddi_BddCofactor(cubeBdd,v,0);
    Ddi_Bdd_t *c1 = Ddi_BddCofactor(cubeBdd,v,1);
    if (Ddi_BddIsZero(c0) && !Ddi_BddIsZero(cubeBdd)) {
      litBdd = isAig ? Ddi_BddMakeLiteralAig(v,1):Ddi_BddMakeLiteral(v,1);
    }
    else {
      Pdtutil_Assert(Ddi_BddIsZero(c1),"zero c1 required");
      litBdd = isAig ? Ddi_BddMakeLiteralAig(v,0):Ddi_BddMakeLiteral(v,0);
    }
    Ddi_BddPartInsertLast(r,litBdd);
    Ddi_Free(c0);
    Ddi_Free(c1);
    Ddi_Free(litBdd);
  }
  Ddi_Free(cubeBdd);
  Ddi_Free(vars);

  return(r);

}

/**Function********************************************************************
  Synopsis     [Duplicate a Ddi_Bdd_t]
  Description  [Duplicate a Ddi_Bdd_t. All pointed objects are recursively
    duplicated. In case of partitioned
    BDDs, array of partitions are duplicated. Cudd BDDs are referenced.]
  SideEffects  []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddDup (
  Ddi_Bdd_t *f    /* BDD to be duplicated */
)
{
  Ddi_Bdd_t *r = NULL;

  if (f != NULL) {
    DdiConsistencyCheck(f,Ddi_Bdd_c);
    r = (Ddi_Bdd_t *)DdiGenericDup((Ddi_Generic_t *)f);
  }
  return (r);
}

/**Function********************************************************************
  Synopsis     [Copy a Ddi_Bdd_t to a destination DDI manager]
  Description  [Copy a Ddi_Bdd_t to a destination DDI manager.
    Variable correspondence is established "by index", i.e.
    variables with same index in different manager correspond.
    Bdd is simply duplicated if destination manager is equal to the
    source one.]
  SideEffects  []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddCopy (
  Ddi_Mgr_t *ddiMgr  /* destination manager */,
  Ddi_Bdd_t *old   /* BDD to be duplicated */
)
{
  Ddi_Bdd_t *newBdd;

  DdiConsistencyCheck(old,Ddi_Bdd_c);
  newBdd = (Ddi_Bdd_t *)DdiGenericCopy(ddiMgr,(Ddi_Generic_t *)old,NULL,NULL);

  return (newBdd);
}

/**Function********************************************************************
  Synopsis     [Copy a Ddi_Bdd_t to a destination DDI manager]
  Description  [Copy a Ddi_Bdd_t to a destination DDI manager.
    Variable correspondence is established "by index", i.e.
    variables with same index in different manager correspond.
    Bdd is simply duplicated if destination manager is equal to the
    source one.]
  SideEffects  []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddCopyRemapVars (
  Ddi_Mgr_t *ddiMgr            /* destination manager */,
  Ddi_Bdd_t *old            /* BDD to be duplicated */,
  Ddi_Vararray_t *varsOld   /* old variable array */,
  Ddi_Vararray_t *varsNew   /* new variable array */
)
{
  Ddi_Bdd_t *newBdd;

  DdiConsistencyCheck(old,Ddi_Bdd_c);
  newBdd = (Ddi_Bdd_t *)DdiGenericCopy(ddiMgr,(Ddi_Generic_t *)old,
    varsOld,varsNew);

  return (newBdd);
}

/**Function********************************************************************
  Synopsis     [Return BDD size (total amount of BDD nodes) of f]
  Description  [Return BDD size (total amount of BDD nodes) of f.
    In case of partitioned or meta BDDs the sharing size is returned
    (shared subgraphs are counted once).]
  SideEffects  []
******************************************************************************/
int
Ddi_BddSizeWithBound(
  Ddi_Bdd_t  *f,
  int bound
)
{
  DdiConsistencyCheck(f,Ddi_Bdd_c);
  return (DdiGenericBddSize((Ddi_Generic_t *)f,bound));
}

/**Function********************************************************************
  Synopsis     [Return BDD size (total amount of BDD nodes) of f]
  Description  [Return BDD size (total amount of BDD nodes) of f.
    In case of partitioned or meta BDDs the sharing size is returned
    (shared subgraphs are counted once).]
  SideEffects  []
******************************************************************************/
int
Ddi_BddSize(
  Ddi_Bdd_t  *f
)
{
  DdiConsistencyCheck(f,Ddi_Bdd_c);
  return (DdiGenericBddSize((Ddi_Generic_t *)f,-1));
}

/**Function********************************************************************
  Synopsis    [Return the top BDD variable of f]
  Description [Return the top BDD variable of f]
  SideEffects []
******************************************************************************/
Ddi_Var_t *
Ddi_BddTopVar (
  Ddi_Bdd_t *f
)
{
  Ddi_Mgr_t *ddiMgr;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  ddiMgr=Ddi_ReadMgr(f);
  if (Ddi_BddIsAig(f)) {
    Ddi_Varset_t *s = Ddi_BddSupp(f);
    Ddi_Var_t *v = Ddi_VarsetTop(s);
    Ddi_Free(s);
    return v;
  }
  else {
    int *cuToIds = Ddi_MgrReadVarIdsFromCudd(ddiMgr);
    int id = cuToIds[Cudd_NodeReadIndex(Ddi_BddToCU(f))];
    return (Ddi_VarFromIndex(ddiMgr,id));
  }
}

/**Function********************************************************************
  Synopsis    [Return the top BDD variable of f]
  Description [Return the top BDD variable of f]
  SideEffects []
******************************************************************************/
Ddi_Var_t *
Ddi_BddLitVar (
  Ddi_Bdd_t *f
)
{
  Ddi_Mgr_t *ddiMgr;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  ddiMgr=Ddi_ReadMgr(f);
  if (Ddi_BddIsAig(f)) {
    bAigEdge_t bv = Ddi_BddToBaig(f);
    return Ddi_VarFromBaig(ddiMgr,bv);
  }
  else {
    return Ddi_BddTopVar(f);
  }
}

/**Function********************************************************************
  Synopsis     [Evaluate expression and free BDD node]
  Description  [Useful for accumulator like expressions (g=f(g,h)), i.e.
    computing a new value for a variable
    and the old value must be freed. Avoids using temporary
    variables. Since the f expression is evalued before
    passing actual parameters, freeing of g occurs as last
    operation.
    <pre>
    E.g.
          g=Ddi_BddEvalFree(Ddi_BddAnd(g,h),g).
    </pre>
    The "accumulator" style operations introduced from version 2.0 of
    pdtrav should stongly reduce the need for this technique. The above
    example can now be written as:
    <pre>
          Ddi_BddAndAcc(g,h).
    </pre>
    ]
  SideEffects  [none]
  SeeAlso      [Ddi_BddAndAcc]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddEvalFree(
  Ddi_Bdd_t *f    /* expression */,
  Ddi_Bdd_t *g    /* BDD to be freed */
)
{
  Ddi_Free (g);
  return (f);
}

/*
 *  relational operations
 */

/**Function********************************************************************
  Synopsis    [Return true (non 0) if the two DDs are equal (f==g).]
  Description [Return true (non 0) if the two DDs are equal (f==g).
    This test is presently limited to monolithic BDDs.]
  SideEffects []
******************************************************************************/
int
Ddi_BddEqual (
  Ddi_Bdd_t *f  /* first dd */,
  Ddi_Bdd_t *g  /* second dd */)
{
  int i;
  DdiConsistencyCheck(f,Ddi_Bdd_c);

  if (f->common.code!=g->common.code)
    return (0);

  switch (f->common.code) {
  case Ddi_Bdd_Part_Disj_c:
  case Ddi_Bdd_Part_Conj_c:
    if (Ddi_BddPartNum(f) != Ddi_BddPartNum(g))
      return(0);
    for(i=0; i<Ddi_BddPartNum(f); i++) {
      if (!Ddi_BddEqual(Ddi_BddPartRead(f,i),Ddi_BddPartRead(g,i)))
        return(0);
    }
    return (1);
    break;
  case Ddi_Bdd_Meta_c:
    {
      int i, ng = Ddi_ReadMgr(f)->meta.groupNum;
      for (i=0; i<ng; i++) {
        if (!Ddi_BddEqual(Ddi_BddarrayRead(f->data.meta->one,i),
                          Ddi_BddarrayRead(g->data.meta->one,i))) {
        return(0);
      }
        if (!Ddi_BddEqual(Ddi_BddarrayRead(f->data.meta->zero,i),
                          Ddi_BddarrayRead(g->data.meta->zero,i))) {
        return(0);
      }
        if (!Ddi_BddEqual(Ddi_BddarrayRead(f->data.meta->dc,i),
                          Ddi_BddarrayRead(g->data.meta->dc,i))) {
        return(0);
      }
      }
      return(1);
    }
    break;
  case Ddi_Bdd_Mono_c:
    return (Ddi_BddToCU(f)==Ddi_BddToCU(g));
    break;
  case Ddi_Bdd_Aig_c:
    if (Ddi_BddToBaig(f)==Ddi_BddToBaig(g)) {
      return 1;
    }
    else {
      return 0;
    }
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }
  return(0);

}

/**Function********************************************************************
  Synopsis    [Return true (non 0) if the two DDs are equal (f==g).]
  Description [Return true (non 0) if the two DDs are equal (f==g).
    This test is presently limited to monolithic BDDs.]
  SideEffects []
******************************************************************************/
int
Ddi_BddEqualSat (
  Ddi_Bdd_t *f  /* first dd */,
  Ddi_Bdd_t *g  /* second dd */)
{
  int i;
  DdiConsistencyCheck(f,Ddi_Bdd_c);

  if (f->common.code!=g->common.code)
    return (0);

  switch (f->common.code) {
  case Ddi_Bdd_Part_Disj_c:
  case Ddi_Bdd_Part_Conj_c:
    if (Ddi_BddPartNum(f) != Ddi_BddPartNum(g))
      return(0);
    for(i=0; i<Ddi_BddPartNum(f); i++) {
      if (!Ddi_BddEqual(Ddi_BddPartRead(f,i),Ddi_BddPartRead(g,i)))
        return(0);
    }
    return (1);
    break;
  case Ddi_Bdd_Meta_c:
    {
      int i, ng = Ddi_ReadMgr(f)->meta.groupNum;
      for (i=0; i<ng; i++) {
        if (!Ddi_BddEqual(Ddi_BddarrayRead(f->data.meta->one,i),
                          Ddi_BddarrayRead(g->data.meta->one,i))) {
        return(0);
      }
        if (!Ddi_BddEqual(Ddi_BddarrayRead(f->data.meta->zero,i),
                          Ddi_BddarrayRead(g->data.meta->zero,i))) {
        return(0);
      }
        if (!Ddi_BddEqual(Ddi_BddarrayRead(f->data.meta->dc,i),
                          Ddi_BddarrayRead(g->data.meta->dc,i))) {
        return(0);
      }
      }
      return(1);
    }
    break;
  case Ddi_Bdd_Mono_c:
    return (Ddi_BddToCU(f)==Ddi_BddToCU(g));
    break;
  case Ddi_Bdd_Aig_c:
    if (Ddi_BddToBaig(f)==Ddi_BddToBaig(g)) {
      return 1;
    }
    else {
      Ddi_Bdd_t *diff = Ddi_BddXor(f,g);
      int res = !Ddi_AigSat(diff);
      Ddi_Free(diff);
      return (res);
    }

    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }
  return(0);

}

/**Function********************************************************************
  Synopsis    [Return true (non 0) if f is the zero constant.]
  Description [Return true (non 0) if f is the zero constant.
    This test is presently limited to monolithic BDDs.]
  SideEffects [none]
  SeeAlso     [Ddi_BddIsOne]
******************************************************************************/
int
Ddi_BddIsZero (
  Ddi_Bdd_t *f
)
{
  if (Ddi_BddIsMono(f)) {
    return (Ddi_BddEqual (f, Ddi_MgrReadZero(Ddi_ReadMgr(f))));
  }
  else if (Ddi_BddIsMeta(f)) {
    return (DdiMetaIsConst(f,0));
  }
  else if (Ddi_BddIsAig(f)) {
    return (DdiAigIsConst(f,0));
  }
  else if (Ddi_BddIsPartConj(f)) {
    /* one 0 partition enough */
    int i;
    for (i=0; i<Ddi_BddPartNum(f); i++) {
      if (Ddi_BddIsZero(Ddi_BddPartRead(f,i))) {
        return (1);
      }
    }
    return (0);
  }
  else if (Ddi_BddIsPartDisj(f)) {
    /* all 0 partitions required */
    int i;
    for (i=0; i<Ddi_BddPartNum(f); i++) {
      if (!Ddi_BddIsZero(Ddi_BddPartRead(f,i))) {
        return (0);
      }
    }
    return (1);
  }
  return (0);
}

/**Function********************************************************************
  Synopsis    [Return true (non 0) if f is the one constant.]
  Description [Return true (non 0) if f is the one constant.
    This test is presently limited to monolithic BDDs.]
  SideEffects [none]
  SeeAlso     [Ddi_BddIsZero]
******************************************************************************/
int
Ddi_BddIsOne (
  Ddi_Bdd_t *f
)
{
  if (f==NULL) return 0;
  if (Ddi_BddIsMono(f)) {
    return (Ddi_BddEqual (f, Ddi_MgrReadOne(Ddi_ReadMgr(f))));
  }
  else if (Ddi_BddIsMeta(f)) {
    return (DdiMetaIsConst(f,1));
  }
  else if (Ddi_BddIsAig(f)) {
    return (DdiAigIsConst(f,1));
  }
  else if (Ddi_BddIsPartDisj(f)) {
    /* one 1 partition enough */
    int i;
    for (i=0; i<Ddi_BddPartNum(f); i++) {
      if (Ddi_BddIsOne(Ddi_BddPartRead(f,i))) {
        return (1);
      }
    }
    return (0);
  }
  else if (Ddi_BddIsPartConj(f)) {
    /* all 1 partitions required */
    int i;
    for (i=0; i<Ddi_BddPartNum(f); i++) {
      if (!Ddi_BddIsOne(Ddi_BddPartRead(f,i))) {
        return (0);
      }
    }
    return (1);
  }
  return (0);
}

/**Function********************************************************************
  Synopsis     [Check for inclusion (f in g). Return non 0 if true]
  Description  [Check for inclusion (f in g). Return non 0 if true.
    This test requires the second operand (g) to be monolithic, whereas
    monolithic and disjunctively partitioned forms are allowed for first
    operand (f).]
  SideEffects  []
******************************************************************************/
double
Ddi_BddCountMinterm (
  Ddi_Bdd_t *f,
  int nvar
)
{
  if (Ddi_BddIsPartConj(f)) {
    double res = 1.0;
    int n, i;
    for (i=n=0; i<Ddi_BddPartNum(f); i++) {
      int nv;
      double part_res;
      Ddi_Bdd_t *f_i = Ddi_BddPartRead(f,i);
      Ddi_Varset_t *partSupp = Ddi_BddSupp(f_i);
      nv = Ddi_VarsetNum(partSupp);
      n+=nv;
      part_res = Ddi_BddCountMinterm (f_i,nv);
      res*=part_res;
      Ddi_Free(partSupp);
    }
    res *= pow(2,nvar-n);
    return (res);
  }
  else if (f->common.code != Ddi_Bdd_Mono_c) {
    return (-1.0);
  }
  else {
    return (
      Cudd_CountMinterm(Ddi_MgrReadMgrCU(Ddi_ReadMgr(f)),Ddi_BddToCU(f),nvar)
    );
  }
}

/**Function********************************************************************
  Synopsis     [Check for inclusion (f in g). Return non 0 if true]
  Description  [Check for inclusion (f in g). Return non 0 if true.
    This test requires the second operand (g) to be monolithic, whereas
    monolithic and disjunctively partitioned forms are allowed for first
    operand (f).]
  SideEffects  []
******************************************************************************/
int
Ddi_BddIncluded (
  Ddi_Bdd_t *f,
  Ddi_Bdd_t *g
)
{
  Ddi_Bdd_t *p, *gg, *ff;
  int r=0, sf, sg;
  Ddi_Mgr_t *ddiMgr;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  DdiConsistencyCheck(g,Ddi_Bdd_c);

  sf = Ddi_BddSize(f);
  sg = Ddi_BddSize(g);
  if (f->common.code == Ddi_Bdd_Aig_c) {
    if (1 || (sf<20000 && sg<5000)) {
      ff = Ddi_BddDup(f);
      gg = Ddi_BddDup(g);
      Ddi_BddSetAig(gg);
      Ddi_BddDiffAcc(ff,gg);
      r = !Ddi_AigSat(ff);
      Ddi_Free(ff);
      Ddi_Free(gg);
    }
    else {
      int i;
      if (!Ddi_BddIsAig(g)) {
        Ddi_Bdd_t *g1 = Ddi_BddMakeAig(g);
        gg = Ddi_AigPartitionTop(g1,0); /* conj part */
        Ddi_Free(g1);
      }
      else {
        gg = Ddi_AigPartitionTop(g,0); /* conj part */
      }
      Ddi_BddNotAcc(gg);
#if 1
      r = !Ddi_AigSatAnd(f,gg,NULL);
#else
      r=1;
      for (i=0; i<Ddi_BddPartNum(gg); i++) {
        r = r && !Ddi_AigSatAnd(f,Ddi_BddPartRead(gg,i),NULL);
      }
#endif
      Ddi_Free(gg);
    }
    return (r);
  }

  if (f->common.code == Ddi_Bdd_Meta_c) {
    return (0);
  }
  if (g->common.code == Ddi_Bdd_Meta_c) {
    return (0);
  }

  ddiMgr = Ddi_ReadMgr(f);

  if (g->common.code!=Ddi_Bdd_Mono_c && g->common.code!=Ddi_Bdd_Aig_c) {
    ff = Ddi_BddMakeAig(f);
    gg = Ddi_BddMakeAig(g);
    r = Ddi_BddIncluded(ff,gg);
    Ddi_Free(ff);
    Ddi_Free(gg);
    return r;
  }

  if (g->common.code==Ddi_Bdd_Aig_c) {
    gg = Ddi_BddDup(g);
  }
  else {
    gg = Ddi_BddMakeMono(g);
  }

  switch (f->common.code) {
  case Ddi_Bdd_Mono_c:
    r = (Cudd_bddIteConstant(Ddi_MgrReadMgrCU(ddiMgr),
      Ddi_BddToCU(f),Ddi_BddToCU(gg),Ddi_BddToCU(Ddi_MgrReadOne(ddiMgr)))
       == Ddi_BddToCU(Ddi_MgrReadOne(ddiMgr)));
    break;
  case Ddi_Bdd_Part_Conj_c:
    /* Pdtutil_Warning(1,
       "conj part in inclusion test (approx result). MakeMono forced"); */
    ff = Ddi_BddDup(f);
    p = Ddi_BddPartExtract(ff,0);
    r = Ddi_BddIncluded(p,gg);
    if (Ddi_BddPartNum(ff)>0) {
      r = r || Ddi_BddIncluded(ff,gg);
    }
    Ddi_Free(ff);
    Ddi_Free(p);
    break;
  case Ddi_Bdd_Meta_c:
    Pdtutil_Warning(1,
    "meta 1-st operand not allowed by inclusion test. MakeMono forced");
    ff = Ddi_BddMakeMono(f);
    r = Ddi_BddIncluded(ff,gg);
    Ddi_Free(ff);
    break;
  case Ddi_Bdd_Part_Disj_c:
    ff = Ddi_BddDup(f);
    p = Ddi_BddPartExtract(ff,0);
    r = Ddi_BddIncluded(p,gg);
    if (Ddi_BddPartNum(ff)>0) {
      r = r && Ddi_BddIncluded(ff,gg);
    }
    Ddi_Free(ff);
    Ddi_Free(p);
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }

  Ddi_Free(gg);

  return (r);
}

/*
 *  Booleand operations & co.
 */

/**Function********************************************************************
  Synopsis    [Boolean NOT. New result is generated]
  Description[Boolean NOT. New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddNot (
  Ddi_Bdd_t *f
)
{
  return (Ddi_BddOp1(Ddi_BddNot_c,f));
}

/**Function********************************************************************
  Synopsis    [Boolean NOT. Result is accumulated]
  Description[Boolean NOT. Result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAndAcc]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddNotAcc (
  Ddi_Bdd_t *f
)
{
  return (Ddi_BddOpAcc1(Ddi_BddNot_c,f));
}

/**Function********************************************************************
  Synopsis    [Boolean AND. New result is generated]
  Description [Compute f & g. A new result is generated and returned.
               Input parameters are NOT changed]
  SideEffects [none]
  SeeAlso     [Ddi_BddNot Ddi_BddOr ]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddAnd (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOp2(Ddi_BddAnd_c,f,g));
}

/**Function********************************************************************
  Synopsis    [Boolean AND. Result is accumulated]
  Description [Compute f & g. Previous content of f is freed and new result
               is copyed to f. Since f points to a handle, it can be passed
               by value: the handle is kept when freeing old content.
               Accumulate type operations are useful to avoid temporary
               variables and explicit free of old data.
               The pointer to f (old handle) is returned so that the function
               may be used as operand for other functions.]
  SideEffects [none]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddAndAcc (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOpAcc2(Ddi_BddAnd_c,f,g));
}

/**Function********************************************************************
  Synopsis    [Boolean difference (f & !g). New result is generated]
  Description[Boolean difference (f & !g). New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddDiff (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOp2(Ddi_BddDiff_c,f,g));
}

/**Function********************************************************************
  Synopsis    [Boolean difference (f & !g). Result is accumulated]
  Description[Boolean difference (f & !g). Result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAndAcc]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddDiffAcc (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOpAcc2(Ddi_BddDiff_c,f,g));
}

/**Function********************************************************************
  Synopsis    [Boolean NAND (!(f&g)). New result is generated]
  Description[Boolean NAND (!(f&g)). New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddNand (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOp2(Ddi_BddNand_c,f,g));
}

/**Function********************************************************************
  Synopsis    [Boolean NAND (!(f&g)). New result is accumulated]
  Description[Boolean NAND (!(f&g)). New result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAndAcc]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddNandAcc (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOpAcc2(Ddi_BddNand_c,f,g));
}

/**Function********************************************************************
  Synopsis    [Boolean OR (f|g). New result is generated]
  Description[Boolean OR (f|g). New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddOr (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOp2(Ddi_BddOr_c,f,g));
}

/**Function********************************************************************
  Synopsis    [Boolean OR (f|g). New result is accumulated]
  Description[Boolean OR (f|g). New result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAndAcc]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddOrAcc (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOpAcc2(Ddi_BddOr_c,f,g));
}

/**Function********************************************************************
  Synopsis    [Boolean NOR (!(f|g)). New result is generated]
  Description[Boolean NOR (!(f|g)). New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddNor (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOp2(Ddi_BddNor_c,f,g));
}

/**Function********************************************************************
  Synopsis    [Boolean NOR (!(f|g)). New result is accumulated]
  Description[Boolean NOR (!(f|g)). New result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAndAcc]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddNorAcc (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOpAcc2(Ddi_BddNor_c,f,g));
}


/**Function********************************************************************
  Synopsis    [Boolean XOR (f^g). New result is generated]
  Description[Boolean XOR (f^g). New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddXor (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOp2(Ddi_BddXor_c,f,g));
}

/**Function********************************************************************
  Synopsis    [Boolean XOR (f^g). New result is accumulated]
  Description[Boolean XOR (f^g). New result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAndAcc]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddXorAcc (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOpAcc2(Ddi_BddXor_c,f,g));
}

/**Function********************************************************************
  Synopsis    [Boolean XNOR (!(f^g)). New result is generated]
  Description[Boolean XNOR (!(f^g)). New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddXnor (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOp2(Ddi_BddXnor_c,f,g));
}

/**Function********************************************************************
  Synopsis    [Boolean XNOR (!(f^g)). New result is accumulated]
  Description[Boolean XNOR (!(f^g)). New result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAndAcc]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddXnorAcc (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOpAcc2(Ddi_BddXnor_c,f,g));
}

/**Function********************************************************************
  Synopsis    [If-Then-Else (ITE(f,g,h)). New result is generated]
  Description[If-Then-Else (ITE(f,g,h)). New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddIte (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g,
  Ddi_Bdd_t  *h
)
{
  return (Ddi_BddOp3(Ddi_BddIte_c,f,g,h));
}

/**Function********************************************************************
  Synopsis    [If-Then-Else (ITE(f,g,h)). New result is accumulated]
  Description[If-Then-Else (ITE(f,g,h)). New result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAndAcc]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddIteAcc (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g,
  Ddi_Bdd_t  *h
)
{
  return (Ddi_BddOpAcc3(Ddi_BddIte_c,f,g,h));
}

/**Function********************************************************************
  Synopsis    [Existential abstraction. New result is generated]
  Description[Existential abstraction. New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddExist (
  Ddi_Bdd_t  *f,
  Ddi_Varset_t *vars
)
{
  return (Ddi_BddOp2(Ddi_BddExist_c,f,vars));
}

/**Function********************************************************************
  Synopsis    [Existential abstraction. New result is accumulated]
  Description[Existential abstraction. New result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddExistAcc (
  Ddi_Bdd_t  *f,
  Ddi_Varset_t *vars
)
{
  return (Ddi_BddOpAcc2(Ddi_BddExist_c,f,vars));
}

/**Function********************************************************************
  Synopsis    [Existential projection. New result is generated]
  Description [Existential abstraction of all variables outside vars.
               New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddExistProject (
  Ddi_Bdd_t  *f,
  Ddi_Varset_t *vars
)
{
  Ddi_Bdd_t *r = Ddi_BddDup(f);
  Ddi_BddExistProjectAcc(r,vars);
  return(r);
}

/**Function********************************************************************
  Synopsis    [Existential projection. New result is generated]
  Description [Existential abstraction of all variables outside vars.
               New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddCubeExist (
  Ddi_Bdd_t  *f,
  Ddi_Varset_t *vars
)
{
  Ddi_Bdd_t *r;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(f);

  if (Ddi_VarsetIsArray(vars)) {
    int i;
    Ddi_Bdd_t *fp = Ddi_AigPartitionTop(f,0);
    r = Ddi_BddMakeConstAig(ddiMgr,1);
    Ddi_VarsetWriteMark (vars, 1);
    for (i=0; i<Ddi_BddPartNum(fp); i++) {
      Ddi_Bdd_t *p = Ddi_BddPartRead(fp,i);
      Pdtutil_Assert(Ddi_BddSize(p)==1,"Wrong partition cube");
      if (Ddi_VarReadMark(Ddi_BddTopVar(p))==0) {
        Ddi_BddAndAcc(r,p);
      }
    }
    Ddi_VarsetWriteMark (vars, 0);
    Ddi_Free(fp);
  }
  else {
    r = Ddi_BddDup(f);
    Ddi_BddExistAcc(r,vars);
  }
  return(r);
}

/**Function********************************************************************
  Synopsis    [Existential abstraction. New result is accumulated]
  Description[Existential abstraction. New result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddCubeExistAcc (
  Ddi_Bdd_t  *f,
  Ddi_Varset_t *vars
)
{
  Ddi_Bdd_t *r = Ddi_BddCubeExist(f,vars);
  Ddi_DataCopy(f,r);
  Ddi_Free(r);
  return (f);
}

/**Function********************************************************************
  Synopsis    [Existential projection. New result is generated]
  Description [Existential abstraction of all variables outside vars.
               New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddCubeExistProject (
  Ddi_Bdd_t  *f,
  Ddi_Varset_t *vars
)
{
  Ddi_Bdd_t *r;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(f);
  Ddi_Varset_t *myVars=NULL;
  int freeVars = 0;
  if (!Ddi_VarsetIsArray(vars)) {
    myVars = Ddi_VarsetMakeArray(vars);
    freeVars = 1;
  }
  else {
    myVars = vars;
  }
  {
    int i;
    Ddi_Bdd_t *fp = Ddi_AigPartitionTop(f,0);
    r = Ddi_BddMakeConstAig(ddiMgr,1);
    Ddi_VarsetWriteMark (myVars, 1);
    for (i=0; i<Ddi_BddPartNum(fp); i++) {
      Ddi_Bdd_t *p = Ddi_BddPartRead(fp,i);
      Pdtutil_Assert(Ddi_BddSize(p)==1,"Wrong partition cube");
      if (Ddi_VarReadMark(Ddi_BddTopVar(p))==1) {
        Ddi_BddAndAcc(r,p);
      }
    }
    Ddi_VarsetWriteMark (myVars, 0);
    Ddi_Free(fp);
  }
  if (freeVars) {
    Ddi_Free(myVars);
  }

  return(r);
}

/**Function********************************************************************
  Synopsis    [Existential projection. New result is accumulated]
  Description [Existential abstraction of all variables outside vars.
               New result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddCubeExistProjectAcc (
  Ddi_Bdd_t  *f,
  Ddi_Varset_t *vars
)
{
  Ddi_Varset_t *smooth = Ddi_BddSupp(f);
  Ddi_VarsetDiffAcc(smooth,vars);
  if (Ddi_VarsetNum(smooth) > 0) {
    Ddi_BddCubeExistAcc(f,smooth);
  }
  Ddi_Free(smooth);
  return(f);
}

/**Function********************************************************************
  Synopsis    [Existential projection. New result is accumulated]
  Description [Existential abstraction of all variables outside vars.
               New result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddExistProjectAcc (
  Ddi_Bdd_t  *f,
  Ddi_Varset_t *vars
)
{
  Ddi_Varset_t *smooth = Ddi_BddSupp(f);
  if (Ddi_VarsetIsArray(vars)) {
    Ddi_VarsetSetArray(smooth);
  }

  Ddi_VarsetDiffAcc(smooth,vars);
  if (Ddi_VarsetNum(smooth) > 0) {
    Ddi_BddExistAcc(f,smooth);
  }
  Ddi_Free(smooth);
  return(f);
}

/**Function********************************************************************
  Synopsis    [Universal abstraction. New result is generated]
  Description[Universal abstraction. New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddForall (
  Ddi_Bdd_t  *f,
  Ddi_Varset_t *vars
)
{
  return (Ddi_BddOp2(Ddi_BddForall_c,f,vars));
}

/**Function********************************************************************
  Synopsis    [Universal abstraction. New result is accumulated]
  Description[Universal abstraction. New result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddForallAcc (
  Ddi_Bdd_t  *f,
  Ddi_Varset_t *vars
)
{
  return (Ddi_BddOpAcc2(Ddi_BddForall_c,f,vars));
}

/**Function********************************************************************
  Synopsis    [Relational product (Exist(f&g,vars)). New result is generated]
  Description[Relational product (Exist(f&g,vars)). New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddAndExist (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g,
  Ddi_Varset_t *vars
)
{
  Ddi_Bdd_t *r;

  if (Ddi_BddIsPartConj(g)&&!Ddi_BddIsPartConj(f)) {
    r = Ddi_BddOp3(Ddi_BddAndExist_c,g,f,vars);
  }
  else {
    r = Ddi_BddOp3(Ddi_BddAndExist_c,f,g,vars);
  }

  return(r);
}

/**Function********************************************************************
  Synopsis    [Relational product (Exist(f&g,vars)). New result is accumulated]
  Description[Relational product (Exist(f&g,vars)). New result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddAndExistAcc (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g,
  Ddi_Varset_t *vars
)
{
  return (Ddi_BddOpAcc3(Ddi_BddAndExist_c,
    f,g,vars));
}


/**Function********************************************************************
  Synopsis    [Exist with overapproximation]
  Description [Exist with overapproximation]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddExistOverApprox (
  Ddi_Bdd_t *f,
  Ddi_Varset_t *sm
)
{
  Ddi_Bdd_t *r;

  r = Ddi_BddDup(f);
  return (Ddi_BddExistOverApproxAcc(r,sm));
}


/**Function********************************************************************
  Synopsis    [Exist with overapproximation]
  Description [Exist with overapproximation]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddExistOverApproxAcc (
  Ddi_Bdd_t *f,
  Ddi_Varset_t *sm
)
{
  int s, i;
  if (Ddi_BddIsPartConj(f)) {
    s = 0;
    for (i=0; i< Ddi_BddPartNum(f); i++) {
      int si = Ddi_BddSize(Ddi_BddPartRead(f,i));
      if (si > s) {
      s = si;
      }
    }
  }
  else {
    s = Ddi_BddSize(f);
  }

  return (DdiBddExistOverApproxAccIntern(f,sm,(int)(1.1*s)));

}


/**Function********************************************************************
  Synopsis    [Cofactor with variable. New result is generated]
  Description [Cofactor with variable. New result is generated]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddCofactor (
  Ddi_Bdd_t  *f,
  Ddi_Var_t  *v,
  int phase
)
{
  Ddi_Bdd_t *r, *lit;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  DdiConsistencyCheck(v,Ddi_Var_c);

  if (f->common.code == Ddi_Bdd_Aig_c) {
    r = Ddi_BddDup(f);
    return (DdiAigCofactorAcc(r,v,phase));
  }
  if (Ddi_BddIsPartConj(f)) {
    r = Ddi_BddDup(f);
    return (Ddi_BddCofactorAcc(r,v,phase));
  }

  lit = Ddi_BddMakeLiteral(v,phase);
  r = Ddi_BddConstrain(f,lit);
  Ddi_Free(lit);

  return (r);
}

/**Function********************************************************************
  Synopsis    [Cofactor with variable array. New result is accumulated]
  Description [Cofactor with variable array. New result is accumulated]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddVararrayCofactorAcc (
  Ddi_Bdd_t  *f,
  Ddi_Vararray_t  *vA,
  int phase
)
{
  Ddi_BddMgr *ddm = Ddi_ReadMgr(f);
  int i, n = Ddi_VararrayNum(vA);

  Ddi_Bddarray_t *subst = Ddi_BddarrayMakeConstAig(ddm,n,phase);

  Ddi_BddComposeAcc(f,vA,subst);

  Ddi_Free(subst);

  return (f);
}

/**Function********************************************************************
  Synopsis    [Cofactor with variable array. New result is generated]
  Description [Cofactor with variable array. New result is generated]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddVararrayCofactor (
  Ddi_Bdd_t  *f,
  Ddi_Vararray_t  *vA,
  int phase
)
{
  Ddi_Bdd_t *newF = Ddi_BddDup(f);

  Ddi_BddVararrayCofactorAcc (newF,vA,phase);

  return (newF);
}

/**Function********************************************************************
  Synopsis    [Cofactor with variable. New result is accumulated]
  Description [Cofactor with variable. New result is accumulated]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddCofactorAcc (
  Ddi_Bdd_t  *f,
  Ddi_Var_t  *v,
  int phase
)
{
  Ddi_Bdd_t *lit;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  DdiConsistencyCheck(v,Ddi_Var_c);

  if (Ddi_BddIsConstant(f)) {
    return f;
  }
    
    if (f->common.code == Ddi_Bdd_Aig_c) {
    return (DdiAigCofactorAcc(f,v,phase));
  }

  if (f->common.info != NULL &&
      f->common.info->infoCode == Ddi_Info_Eq_c) {
    Ddi_Vararray_t *vars = Ddi_BddReadEqVars(f);
    Ddi_Bddarray_t *subst = Ddi_BddReadEqSubst(f);
    int i;
    for (i=0;i<Ddi_VararrayNum(vars); i++) {
      if (Ddi_VararrayRead(vars,i)==v) {
	Ddi_VararrayRemove(vars,i);
	Ddi_BddarrayRemove(subst,i);
	break;
      }
    }
  }

  if ((Ddi_BddIsPartConj(f) || Ddi_BddIsPartDisj(f)) 
      && Ddi_BddIsAig(Ddi_BddPartRead(f,0))) {
    int i;
    for (i=0; i<Ddi_BddPartNum(f); i++) {
      Ddi_BddCofactorAcc (Ddi_BddPartRead(f,i),v,phase);
    }
    return(f);
  }

  lit = Ddi_BddMakeLiteral(v,phase);
  Ddi_BddConstrainAcc(f,lit);
  Ddi_Free(lit);

  return (f);
}


/**Function********************************************************************
  Synopsis    [Constrain cofactor. New result is generated]
  Description[Constrain cofactor. New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddConstrain (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  Ddi_Bdd_t *r;

  if (Ddi_BddIsOne(g)||Ddi_BddIsConstant(f)) {
    return(Ddi_BddDup(f));
  }

#if 1
    r = Ddi_BddOp2(Ddi_BddConstrain_c,f,g);
#else
  Ddi_MgrSiftSuspend(Ddi_ReadMgr(f));

  sf = Ddi_BddSupp(f);
  sg = Ddi_BddSupp(g);
  common = Ddi_VarsetIntersect(sf,sg);

  Ddi_MgrSiftResume(Ddi_ReadMgr(f));

  if (Ddi_VarsetIsVoid(common)) {
    r = Ddi_BddDup(f);
  }
  else {
    r = Ddi_BddOp2(Ddi_BddConstrain_c,f,g);
  }

  Ddi_Free(sf);
  Ddi_Free(sg);
  Ddi_Free(common);
#endif
  return (r);
}

/**Function********************************************************************
  Synopsis    [Constrain cofactor. New result is accumulated]
  Description[Constrain cofactor. New result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAndAcc]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddConstrainAcc (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  if (Ddi_BddIsOne(g)||Ddi_BddIsConstant(f)) {
    return(f);
  }
  return (Ddi_BddOpAcc2(Ddi_BddConstrain_c,f,g));
}

/**Function********************************************************************
  Synopsis    [Restrict cofactor. New result is generated]
  Description[Restrict cofactor. New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddRestrict (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOp2(Ddi_BddRestrict_c,f,g));
}

/**Function********************************************************************
  Synopsis    [Restrict cofactor. New result is accumulated]
  Description[Restrict cofactor. New result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAndAcc]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddRestrictAcc (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOpAcc2(Ddi_BddRestrict_c,f,g));
}

/**Function********************************************************************
  Synopsis    [Cofexist cofactor. New result is generated]
  Description[Constrain cofactor. New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddCofexist (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g,
  Ddi_Varset_t *smooth
)
{
  Ddi_Bdd_t *r, *gMask;
  DdNode *cuMask;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(f);

  Pdtutil_Assert(Ddi_BddIsMono(g),"cofexist cofactoring term must be mono");

  Ddi_MgrSiftSuspend(ddiMgr);
  Ddi_MgrReadMgrCU(ddiMgr)->reordered = 0;

  cuMask = Cuplus_CofexistMask(Ddi_MgrReadMgrCU(ddiMgr),
    Ddi_BddToCU(g),Ddi_VarsetToCU(smooth));
  gMask = Ddi_BddMakeFromCU (ddiMgr, cuMask);

  r = Ddi_BddConstrain(f,g);
#if 0
  Ddi_BddSetPartConj(r);
  Ddi_BddPartInsert(r,0,gMask);
#else
  Ddi_BddAndAcc(r,gMask);
#endif

  Ddi_Free(gMask);

  Ddi_MgrSiftResume(ddiMgr);

  return (r);
}

/**Function********************************************************************
  Synopsis    [Compatible projector. New result is generated]
  Description[Compatible projector. New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddCproject (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOp2(Ddi_BddCproject_c,f,g));
}

/**Function********************************************************************
  Synopsis    [Compatible projector. New result is accumulated]
  Description[Compatible projector. New result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAndAcc]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddCprojectAcc (
  Ddi_Bdd_t  *f,
  Ddi_Bdd_t  *g
)
{
  return (Ddi_BddOpAcc2(Ddi_BddCproject_c,f,g));
}

/**Function********************************************************************
  Synopsis    [Swap x and y variables in f. New result is generated]
  Description[Swap x and y variables in f. New result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddSwapVars (
  Ddi_Bdd_t  *f,
  Ddi_Vararray_t *x  /* 1-st array of variables */,
  Ddi_Vararray_t *y  /* 2-nd array of variables */
)
{
  return (Ddi_BddOp3(Ddi_BddSwapVars_c,f,x,y));
}

/**Function********************************************************************
  Synopsis    [Swap x and y variables in f. Result is accumulated]
  Description[Swap x and y variables in f. Result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddSwapVarsAcc (
  Ddi_Bdd_t  *f,
  Ddi_Vararray_t *x  /* 1-st array of variables */,
  Ddi_Vararray_t *y  /* 2-nd array of variables */
)
{
  return (Ddi_BddOpAcc3(Ddi_BddSwapVars_c,f,x,y));
}

/**Function********************************************************************
  Synopsis    [Variable substitution x <- y in f. New result is generated]
  Description [Variable substitution x <- y in f. New result is generated.
               Variable correspondence is established by position in x, y.
               Substitution is done by compose. This differs from variable
               swapping since some y vars may be present in x as well as in
               the support of f.]
  SideEffects [none]
  SeeAlso     [Ddi_BddSwapVars Ddi_BddSubstVarsAcc]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddSubstVars (
  Ddi_Bdd_t  *f,
  Ddi_Vararray_t *x  /* 1-st array of variables */,
  Ddi_Vararray_t *y  /* 2-nd array of variables */
)
{
  return (Ddi_BddOp3(Ddi_BddSubstVars_c,f,x,y));
}

/**Function********************************************************************
  Synopsis    [Variable substitution x <- y in f. New result is accumulated]
  Description [Variable substitution x <- y in f. New result is accumulated.
               Variable correspondence is established by position in x, y.
               Substitution is done by compose. This differs from variable
               swapping since some y vars may be present in x as well as in
               the support of f.]
  SideEffects [none]
  SeeAlso     [Ddi_BddSwapVars Ddi_BddSubstVars]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddSubstVarsAcc (
  Ddi_Bdd_t  *f,
  Ddi_Vararray_t *x  /* 1-st array of variables */,
  Ddi_Vararray_t *y  /* 2-nd array of variables */
)
{
  return (Ddi_BddOpAcc3(Ddi_BddSubstVars_c,f,x,y));
}

/**Function********************************************************************
  Synopsis    [Function composition x <- g in f. New result is generated]
  Description [Function composition x <- g in f. New result is generated.
               Vector composition algorithm is used.]
  SideEffects [none]
  SeeAlso     [Ddi_BddSwapVars Ddi_BddSubstVars]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddCompose (
  Ddi_Bdd_t  *f,
  Ddi_Vararray_t *x  /* array of variables */,
  Ddi_Bddarray_t *g  /* array of functions */
)
{
  if (x==NULL || Ddi_VararrayNum(x)==0) {
    return Ddi_BddDup(f);
  }
  return (Ddi_BddOp3(Ddi_BddCompose_c,f,x,g));
}

/**Function********************************************************************
  Synopsis    [Function composition x <- g in f. New result is accumulated]
  Description [Function composition x <- g in f. New result is accumulated.
               Vector composition algorithm is used.]
  SideEffects [none]
  SeeAlso     [Ddi_BddSwapVars Ddi_BddSubstVars]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddComposeAcc (
  Ddi_Bdd_t  *f,
  Ddi_Vararray_t *x  /* array of variables */,
  Ddi_Bddarray_t *g  /* array of functions */
)
{
  if (x==NULL || Ddi_VararrayNum(x)==0) {
    return f;
  }

  if (Ddi_BddReadComposeF(f)!=NULL) {
    Ddi_BddComposeAcc (Ddi_BddReadComposeF(f),x,g);
    Ddi_BddarrayComposeAcc(Ddi_BddReadComposeSubst(f),x,g);
    if (Ddi_BddReadComposeConstr(f)!=NULL) {
      Ddi_BddComposeAcc (Ddi_BddReadComposeConstr(f),x,g);
    }
    if (Ddi_BddReadComposeCone(f)!=NULL) {
      Ddi_BddComposeAcc (Ddi_BddReadComposeCone(f),x,g);
    }
  }

  return (Ddi_BddOpAcc3(Ddi_BddCompose_c,f,x,g));
}


/**Function********************************************************************
  Synopsis    [Function composition x <- newv in f. New result is accumulated]
  Description [Function composition x <- newv in f. New result is accumulated.
               New fresh vars are generated.]
  SideEffects [none]
  SeeAlso     [Ddi_BddSwapVars Ddi_BddSubstVars]
******************************************************************************/
Ddi_Vararray_t *
Ddi_BddNewVarsAcc(
  Ddi_Bdd_t *f,
  Ddi_Vararray_t *x  /* array of OLD variables */,
  char *prefix,
  char *suffix,
  int useBddVars
)
{
  Ddi_BddMgr *ddm = Ddi_ReadMgr(f);
  Ddi_Bddarray_t *fA = Ddi_BddarrayAlloc (ddm, 1);
  Ddi_Vararray_t *newVars;

  Ddi_BddarrayWrite(fA,0,f);

  newVars = Ddi_BddarrayNewVarsAcc(fA,x,prefix,suffix,useBddVars);
  Ddi_DataCopy(f,Ddi_BddarrayRead(fA,0));
  Ddi_Free(fA);

  return newVars;
}

/**Function********************************************************************
  Synopsis    [Return true (non 0) if f is a cube.]
  Description [Return true (non 0) if f is a cube. Monolithic BDD required.]
  SideEffects [none]
  SeeAlso     [Ddi_BddIsOne]
******************************************************************************/
int
Ddi_BddIsCube (
  Ddi_Bdd_t *f
)
{
  DdiConsistencyCheck(f,Ddi_Bdd_c);
  if (!Ddi_BddIsMono(f)) {
    if (Ddi_BddIsPartConj(f)) {
      int i;
      for (i=0; i<Ddi_BddPartNum(f); i++) {
	if (!Ddi_BddIsLiteralAig(Ddi_BddPartRead(f,i))) return 0;
      }
      return 1;
    }
    else if (Ddi_BddIsAig(f)) {
      if (Ddi_BddIsLiteralAig(f)) return 1;
      else {
	Ddi_Bdd_t *fPart = Ddi_AigPartitionTop(f,0);
	int res = Ddi_BddIsCube(fPart);
	Ddi_Free(fPart);
	return res;
      }
    }
    return(0);
  }
  return(Cudd_CheckCube(Ddi_MgrReadMgrCU(Ddi_ReadMgr(f)),Ddi_BddToCU(f)));
}

/**Function********************************************************************
  Synopsis    [Pick one random on-set cube. Result is generated]
  Description[Pick one random on-set cube. Result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddPickOneCube (
  Ddi_Bdd_t  *f
)
{
  Ddi_Bdd_t *cubeDd, *literal=NULL;
  int i;
  char *cube;
  Ddi_Mgr_t *ddiMgr;
  int nMgrVars;
  DdNode *f_cu;
  DdManager *mgrCU;
  int ret;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  ddiMgr = Ddi_ReadMgr(f);
  mgrCU = Ddi_MgrReadMgrCU(ddiMgr);
  nMgrVars = Ddi_MgrReadNumVars(ddiMgr);

  if (Ddi_BddIsZero(f))
    return (Ddi_BddDup(f));

  Pdtutil_Assert(f->common.code == Ddi_Bdd_Mono_c,
    "Monolitic DD required for Cube selection. Use proper MakeMono function!");

  cube = Pdtutil_Alloc(char, nMgrVars);
  f_cu = Ddi_BddToCU(f);

  ret = Cudd_bddPickOneCube(mgrCU,f_cu,cube);
  Pdtutil_Assert(ret!=0,"Error returned by Cudd_bddPickOneCube");

  /* Build result BDD. */
  cubeDd = Ddi_BddDup(Ddi_MgrReadOne(ddiMgr));

  for (i=0; i<nMgrVars; i++) {
    switch (cube[i]) {
      case 0:
        literal = Ddi_BddMakeLiteral(Ddi_VarFromIndex(ddiMgr,i),0);
        break;
      case 1:
        literal = Ddi_BddMakeLiteral(Ddi_VarFromIndex(ddiMgr,i),1);
        break;
      case 2:
        literal = Ddi_BddDup(Ddi_MgrReadOne(ddiMgr));
        break;
      default:
        Pdtutil_Warning (1,"Wrong code found in cube string");
        literal = Ddi_BddDup(Ddi_MgrReadOne(ddiMgr));
        break;
    }

    cubeDd = Ddi_BddEvalFree(Ddi_BddAnd(cubeDd,literal),cubeDd);
    Ddi_Free(literal);
  }

  Pdtutil_Free(cube);

  return (cubeDd);
}

/**Function********************************************************************
  Synopsis    [Pick one random on-set cube. Result is accumulated]
  Description[Pick one random on-set cube. Result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAndAcc]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddPickOneCubeAcc (
  Ddi_Bdd_t  *f
)
{
  Ddi_Bdd_t *tmp;

  tmp = Ddi_BddPickOneCube(f);
  DdiGenericDataCopy((Ddi_Generic_t *)f,(Ddi_Generic_t *)tmp);
  Ddi_Free(tmp);
  return (f);
}

/**Function********************************************************************
  Synopsis    [Pick one random on-set minterm. Result is generated]
  Description[Pick one random on-set minterm. Result is generated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddPickOneMinterm (
  Ddi_Bdd_t  *f,
  Ddi_Varset_t *vars   /* set of variables defining the minterm space */
)
{
  Ddi_Bdd_t *mintermDd;
  int i, nMgrVars;
  Ddi_Mgr_t *ddiMgr;
  DdNode **vars_cu;
  DdNode *f_cu, *minterm_cu, *scan_cu;
  DdManager *mgrCU;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  ddiMgr = Ddi_ReadMgr(f);
  mgrCU = Ddi_MgrReadMgrCU(ddiMgr);
  nMgrVars = Ddi_MgrReadNumVars(ddiMgr);

  if (Ddi_BddIsZero(f))
    return (Ddi_BddDup(f));

  Pdtutil_Assert(f->common.code == Ddi_Bdd_Mono_c,
    "Monolitic DD required for Cube selection. Use proper MakeMono function!");

  f_cu = Ddi_BddToCU(f);
  vars_cu = Pdtutil_Alloc(DdNode *, Ddi_VarsetNum(vars));

  scan_cu = Ddi_VarsetToCU(vars);
  for (i=0;i<Ddi_VarsetNum(vars);i++) {
    vars_cu[i] = Cudd_bddIthVar(mgrCU,Cudd_NodeReadIndex(scan_cu));
    scan_cu = Cudd_T(scan_cu);
  }

  minterm_cu = Cudd_bddPickOneMinterm(mgrCU,f_cu,vars_cu,Ddi_VarsetNum(vars));
  free(vars_cu);
  Pdtutil_Assert(minterm_cu!=NULL,"Error returned by Cudd_bddPickOneMinterm");
  mintermDd = Ddi_BddMakeFromCU (ddiMgr, minterm_cu);

  return (mintermDd);
}

/**Function********************************************************************
  Synopsis    [Pick one random on-set minterm. Result is accumulated]
  Description[Pick one random on-set minterm. Result is accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddPickOneMintermAcc (
  Ddi_Bdd_t  *f,
  Ddi_Varset_t *vars   /* set of variables defining the minterm space */
)
{
  Ddi_Bdd_t *tmp;

  tmp = Ddi_BddPickOneMinterm(f,vars);
  DdiGenericDataCopy((Ddi_Generic_t *)f,(Ddi_Generic_t *)tmp);
  Ddi_Free(tmp);
  return (f);
}


/**Function********************************************************************
  Synopsis    [Gen cover as a sum of prime implicants of a BDD.]

  Description [Gen cover as a sum of prime implicants of a BDD. 
  Basically taken/copied from Cudd_bddPrintCover( in cuddutil.c]

  SideEffects [None]

  SeeAlso     [Cudd_bddPrintCover]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddCover (
  Ddi_Bdd_t  *onset,
  Ddi_Bdd_t  *offset,
  int *np
)
{
  Ddi_Mgr_t *ddiMgr;
  DdNode **vars_cu;
  DdNode *f_cu, *minterm_cu, *scan_cu;
  DdManager *dd;
  DdNode *l;
  DdNode *u;
  DdNode *lb;
  Ddi_Bdd_t *fCover;

  DdiConsistencyCheck(onset,Ddi_Bdd_c);
  DdiConsistencyCheck(offset,Ddi_Bdd_c);
  ddiMgr = Ddi_ReadMgr(onset);
  dd = Ddi_MgrReadMgrCU(ddiMgr);

  if (Ddi_BddIsConstant(onset))
    return (Ddi_BddDup(onset));
  if (Ddi_BddIsConstant(offset))
    return (Ddi_BddNot(offset));

  l = Ddi_BddToCU(onset);
  u = Cudd_Not(Ddi_BddToCU(offset));

  int n=0;
  DdNode *cover;
  
  lb = l;
  cuddRef(lb);
  cover = Cudd_ReadLogicZero(dd);
  cuddRef(cover);

  while (lb != Cudd_ReadLogicZero(dd)) {
    DdNode *implicant, *prime, *tmp;
    int length;
    implicant = Cudd_LargestCube(dd,lb,&length);
    if (implicant == NULL) {
      Cudd_RecursiveDeref(dd,lb);
      Cudd_RecursiveDeref(dd,cover);
      return NULL;
    }
    cuddRef(implicant);
    prime = Cudd_bddMakePrime(dd,implicant,u);
    if (prime == NULL) {
      Cudd_RecursiveDeref(dd,lb);
      Cudd_RecursiveDeref(dd,implicant);
      Cudd_RecursiveDeref(dd,cover);
      return(NULL);
    }
    cuddRef(prime);
    Cudd_RecursiveDeref(dd,implicant);
    tmp = Cudd_bddAnd(dd,lb,Cudd_Not(prime));
    if (tmp == NULL) {
      Cudd_RecursiveDeref(dd,lb);
      Cudd_RecursiveDeref(dd,prime);
      Cudd_RecursiveDeref(dd,cover);
      return(NULL);
    }
    cuddRef(tmp);
    Cudd_RecursiveDeref(dd,lb);
    lb = tmp;
    tmp = Cudd_bddOr(dd,prime,cover);
    if (tmp == NULL) {
      Cudd_RecursiveDeref(dd,cover);
      Cudd_RecursiveDeref(dd,lb);
      Cudd_RecursiveDeref(dd,prime);
      Cudd_RecursiveDeref(dd,cover);
      return(NULL);
    }
    cuddRef(tmp);
    Cudd_RecursiveDeref(dd,cover);
    cover = tmp;
    Cudd_RecursiveDeref(dd,prime);

    n++;
  }
  Cudd_RecursiveDeref(dd,lb);
  if (!Cudd_bddLeq(dd,cover,u) || !Cudd_bddLeq(dd,l,cover)) {
    Cudd_RecursiveDeref(dd,cover);
    return(NULL);
  }

  fCover = Ddi_BddMakeFromCU (ddiMgr, cover);
  Cudd_Deref(cover);
  if (np!=NULL) *np = n;
  return(fCover);

} 



/**Function********************************************************************
  Synopsis    [Find implied variables in f.]
  Description [Find implied variables in f. An implied variable is a variable
               whose value is fully determined (either 0 or 1).]
  SideEffects [none]
  SeeAlso     [Ddi_BddAnd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddImpliedCube (
  Ddi_Bdd_t  *f,
  Ddi_Varset_t *vars   /* set of variables defining the searched vars */
)
{
  Ddi_Varset_t *myVars;
  Ddi_Var_t *var;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(f);
  Ddi_Bdd_t *res = Ddi_BddMakeConst(ddiMgr,1);

  if (vars == NULL) {
    myVars = Ddi_BddSupp (f);
  }
  else {
    myVars = Ddi_VarsetDup(vars);
  }

  for (Ddi_VarsetWalkStart(myVars);
       !Ddi_VarsetWalkEnd(myVars);
       Ddi_VarsetWalkStep(myVars)) {
    Ddi_Bdd_t *lit;
    var = Ddi_VarsetWalkCurr(myVars);

    lit = Ddi_BddMakeLiteral(var,0);
    if (Ddi_BddIncluded(f,lit)) {
      Ddi_BddAndAcc(res,lit);
    }
    else if (Ddi_BddIncluded(f,Ddi_BddNotAcc(lit))) {
      Ddi_BddAndAcc(res,lit);
    }
    Ddi_Free(lit);
  }

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMax_c,
    int n = Ddi_BddSize(res)-1;
    if (n>0) {
        fprintf(stdout, "{%d implied vars found}", n);
    }
  );


  Ddi_Free(myVars);

  return (res);
}

/**Function********************************************************************
  Synopsis    [Support of f. New result is generated]
  Description [The support of f is the set of variables f depends on.
               This function has no "accumulated" version, but a related
               function (Ddi_BddSuppAttach) which attaches the support to
               a function, so that no BDD traversal is done in further
               calls of Ddi_BddSupp.]
  SideEffects [none]
  SeeAlso     [Ddi_BddSuppAttach]
******************************************************************************/
Ddi_Varset_t *
Ddi_BddSupp (
  Ddi_Bdd_t *f
)
{
  return ((Ddi_Varset_t *)DdiGenericOp1(Ddi_BddSupp_c,Ddi_Generate_c,f));
}

/**Function********************************************************************
  Synopsis    [Support of f. New result is generated]
  Description [The support of f is the set of variables f depends on.
               This function has no "accumulated" version, but a related
               function (Ddi_BddSuppAttach) which attaches the support to
               a function, so that no BDD traversal is done in further
               calls of Ddi_BddSupp.]
  SideEffects [none]
  SeeAlso     [Ddi_BddSuppAttach]
******************************************************************************/
Ddi_Vararray_t *
Ddi_BddSuppVararray (
  Ddi_Bdd_t *f
)
{
  Ddi_Varset_t *supp = Ddi_BddSupp(f);
  Ddi_Vararray_t *vA = Ddi_VararrayMakeFromVarset(supp,1);
  Ddi_Free(supp);
  return vA;
}

/**Function********************************************************************
  Synopsis    [Attach support of f to f. Return pointer to f]
  Description [The support of f is the set of variables f depends on.
               This function generates the support of f
               and hooks it to proper field of f, so that no BDD traversal
               is done in further calls of Ddi_BddSupp.]
  SideEffects [support is attached to f]
  SeeAlso     [Ddi_BddSupp Ddi_BddSuppDetach]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddSuppAttach (
  Ddi_Bdd_t *f
)
{
  return ((Ddi_Bdd_t*)DdiGenericOp1(Ddi_BddSuppAttach_c,Ddi_Accumulate_c,f));
}

/**Function********************************************************************
  Synopsis    [Read the support attached to a Bdd.]
  Description [Read the support attached to a Bdd. The support is not
    duplicated, as would Ddi_BddSupp with attached support. Return NULL if
    support is not attached.]
  SideEffects [none]
  SeeAlso     [Ddi_BddSupp Ddi_BddSuppAttach]
******************************************************************************/
Ddi_Varset_t *
Ddi_BddSuppRead(
  Ddi_Bdd_t *f
)
{
  return ((Ddi_Varset_t*)(f->common.supp));
}
/**Function********************************************************************
  Synopsis    [Detach (and free) support attached to f. Return pointer to f]
  Description[Detach (and free) support attached to f. Return pointer to f]
  SideEffects [none]
  SeeAlso     [Ddi_BddSupp Ddi_BddSuppAttach]
******************************************************************************/

Ddi_Bdd_t *
Ddi_BddSuppDetach(
  Ddi_Bdd_t *f
)
{
  return ((Ddi_Bdd_t*)DdiGenericOp1(Ddi_BddSuppDetach_c,Ddi_Accumulate_c,f));
}

/*
 *  Partition management
 */

/**Function********************************************************************
  Synopsis    [Return true (non 0) if f is a monolithic BDD.]
  Description [Return true (non 0) if f is a monolithic BDD.]
  SideEffects [none]
  SeeAlso     [Ddi_BddIsPartConj Ddi_BddIsPartDisj]
******************************************************************************/
int
Ddi_BddIsMono (
  Ddi_Bdd_t *f
)
{
  return (f->common.code == Ddi_Bdd_Mono_c);
}

/**Function********************************************************************
  Synopsis    [Return true (non 0) if f is a meta BDD.]
  Description [Return true (non 0) if f is a meta BDD.]
  SideEffects [none]
  SeeAlso     [Ddi_BddIsPartConj Ddi_BddIsPartDisj]
******************************************************************************/
int
Ddi_BddIsMeta (
  Ddi_Bdd_t *f
)
{
  return (f->common.code == Ddi_Bdd_Meta_c);
}


/**Function********************************************************************
  Synopsis    [Return true (non 0) if f is an Aig.]
  Description [Return true (non 0) if f is an Aig.]
  SideEffects [none]
  SeeAlso     [Ddi_BddIsMono Ddi_BddIsMeta Ddi_BddIsPartConj Ddi_BddIsPartDisj]
******************************************************************************/
int
Ddi_BddIsAig (
  Ddi_Bdd_t *f
)
{
  return (f->common.code == Ddi_Bdd_Aig_c);
}

/**Function********************************************************************
  Synopsis    [Return true (non 0) if f is an Aig.]
  Description [Return true (non 0) if f is an Aig.]
  SideEffects [none]
  SeeAlso     [Ddi_BddIsMono Ddi_BddIsMeta Ddi_BddIsPartConj Ddi_BddIsPartDisj]
******************************************************************************/
int
Ddi_BddIsLiteralAig (
  Ddi_Bdd_t *f
)
{
  if (f->common.code == Ddi_Bdd_Aig_c) {
    Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(f);
    bAig_Manager_t  *bmgr = ddiMgr->aig.mgr;
    return (bAig_isVarNode(bmgr,Ddi_BddToBaig(f)));
  }
  return 0;
}

/**Function********************************************************************
  Synopsis    [Return true (non 0) if f is a conjunctively partitioned BDD.]
  Description [Return true (non 0) if f is a conjunctively partitioned BDD.]
  SideEffects [none]
  SeeAlso     [Ddi_BddIsMono Ddi_BddIsPartDisj]
******************************************************************************/
int
Ddi_BddIsPartConj (
  Ddi_Bdd_t *f
)
{
  return (f->common.code == Ddi_Bdd_Part_Conj_c);
}

/**Function********************************************************************
  Synopsis    [Return true (non 0) if f is a disjunctively partitioned BDD.]
  Description [Return true (non 0) if f is a disjunctively partitioned BDD.]
  SideEffects [none]
  SeeAlso     [Ddi_BddIsMono Ddi_BddIsPartConj]
******************************************************************************/
int
Ddi_BddIsPartDisj (
  Ddi_Bdd_t *f
)
{
  return (f->common.code == Ddi_Bdd_Part_Disj_c);
}


/**Function********************************************************************
  Synopsis    [Read the number of partitions (conj/disj).]
  Description [Read the number of partitions (conj/disj).
    In case of monolithic BDD, 1 is returned, in case of partitioned
    BDDs, the number of partitions.]
  SideEffects []
******************************************************************************/
int
Ddi_BddPartNum (
  Ddi_Bdd_t * f
)
{
  DdiConsistencyCheck(f,Ddi_Bdd_c);
  switch (f->common.code) {
  case Ddi_Bdd_Mono_c:
  case Ddi_Bdd_Meta_c:
  case Ddi_Bdd_Aig_c:
    //    Pdtutil_Assert(0,"operation requires partitioned BDD");
    // now extended. returns 1.
    return 1;
    break;
  case Ddi_Bdd_Part_Conj_c:
  case Ddi_Bdd_Part_Disj_c:
    return (DdiArrayNum(f->data.part));
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }
  return (-1);
}

/**Function********************************************************************
  Synopsis    [Read the i-th partition (conj/disj) of f.]
  Description [Read the i-th partition (conj/disj) of f.]
  SideEffects [] exit

******************************************************************************/
Ddi_Bdd_t *
Ddi_BddPartRead(
  Ddi_Bdd_t * f,
  int i
)
{
  DdiConsistencyCheck(f,Ddi_Bdd_c);
  Pdtutil_Assert((i>=0)&&(i<Ddi_BddPartNum(f)),
    "partition index out of bounds");
  switch (f->common.code) {
  case Ddi_Bdd_Mono_c:
  case Ddi_Bdd_Meta_c:
  case Ddi_Bdd_Aig_c:
    // now extended. OK if i==0
    Pdtutil_Assert(i==0,"operation requires partitioned BDD");
    return f;
    break;
  case Ddi_Bdd_Part_Conj_c:
  case Ddi_Bdd_Part_Disj_c:
    return ((Ddi_Bdd_t *)DdiArrayRead(f->data.part,i));
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }
  return(NULL);
}

/**Function********************************************************************
  Synopsis    [Write i-th partition. Result accumulated]
  Description [Write new partition at position i. Result accumulated.
    Same as insert if position is PartNum+1. Otherwise i-th partition is
    freed and overwritten, so Write is acrually a partition "replace" or
    "rewrite" operation.]
  SideEffects []
  SeeAlso     [Ddi_BddPartInsert]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddPartWrite (
  Ddi_Bdd_t *f        /* partitioned BDD */,
  int i               /* position of new partition */,
  Ddi_Bdd_t *newp     /* new partition */
)
{
  DdiConsistencyCheck(f,Ddi_Bdd_c);
  switch (f->common.code) {
  case Ddi_Bdd_Mono_c:
  case Ddi_Bdd_Meta_c:
    Pdtutil_Assert(0,"operation requires partitioned BDD");
    break;
  case Ddi_Bdd_Part_Conj_c:
  case Ddi_Bdd_Part_Disj_c:
    DdiArrayWrite(f->data.part,i,(Ddi_Generic_t *)newp,Ddi_Dup_c);
#if 0
    if (Ddi_BddIsOne(newp) && (f->common.code==Ddi_Bdd_Part_Conj_c)) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "^1^")
      );
    }
    if (Ddi_BddIsZero(newp) && (f->common.code==Ddi_Bdd_Part_Disj_c)) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "^0^")
      );
    }
#endif
    f->common.size = 0;
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }
  return(f);
}

/**Function********************************************************************
  Synopsis    [Return the i-th partition (conj/disj), and remove it from f.]
  Description [Return the i-th partition (conj/disj), and remove it from f.]
  SideEffects []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddPartExtract(
  Ddi_Bdd_t * f,
  int i
)
{
  DdiConsistencyCheck(f,Ddi_Bdd_c);
  Pdtutil_Assert((i>=0)&&(i<Ddi_BddPartNum(f)),
    "partition index out of bounds");
  switch (f->common.code) {
  case Ddi_Bdd_Mono_c:
  case Ddi_Bdd_Meta_c:
    Pdtutil_Assert(0,"partition extract requires partitioned BDD");
    break;
  case Ddi_Bdd_Part_Conj_c:
  case Ddi_Bdd_Part_Disj_c:
    f->common.size = 0;
    return ((Ddi_Bdd_t *)DdiArrayExtract(f->data.part,i));
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }
  return(NULL);
}

/**Function********************************************************************
  Synopsis    [Return the i-th partition (conj/disj), and remove it from f.]
  Description [Return the i-th partition (conj/disj), and remove it from f.]
  SideEffects []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddPartSwap(
  Ddi_Bdd_t * f,
  int i,
  int j
)
{
  Ddi_Bdd_t *part;

  if (i != j) {
    DdiConsistencyCheck(f,Ddi_Bdd_c);
    Pdtutil_Assert((i>=0)&&(i<Ddi_BddPartNum(f)),
               "partition index out of bounds");
    Pdtutil_Assert((j>=0)&&(j<Ddi_BddPartNum(f)),
               "partition index out of bounds");

    part = Ddi_BddDup(Ddi_BddPartRead(f, j));
    Ddi_BddPartWrite(f, j, Ddi_BddPartRead(f, i));
    Ddi_BddPartWrite(f, i, part);
    Ddi_Free(part);
  }

  return f;
}

/**Function********************************************************************
  Synopsis    [Return the i-th partition (conj/disj), and remove it from f.]
  Description [Return the i-th partition (conj/disj), and remove it from f.]
  SideEffects []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddPartSortBySizeAcc(
  Ddi_Bdd_t * f,
  int increasing
)
{
  int i, j, n;
  Ddi_Bdd_t *part;

  if (!Ddi_BddIsPartConj(f) && !Ddi_BddIsPartDisj(f)) return NULL;

  n = Ddi_BddPartNum(f);
  for (i=0; i<n; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddPartRead(f,i);
    f_i->common.size = Ddi_BddSize(f_i);
  }
  for (i=0; i<n-1; i++) {
    int jMin=i;
    Ddi_Bdd_t *fMin = Ddi_BddPartRead(f,i);
    for (j=i+1; j<n; j++) {
      Ddi_Bdd_t *f_j = Ddi_BddPartRead(f,j);
      if (increasing ? (f_j->common.size < fMin->common.size) : 
          (f_j->common.size > fMin->common.size)) {
        jMin = j; fMin = f_j;
      }
    } 
    if (jMin != i) {
      Ddi_BddPartSwap(f,i,jMin);
    }
  }
  for (i=0; i<n; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddPartRead(f,i);
    f_i->common.size = 0;
  }

  return f;
}

/**Function********************************************************************
  Synopsis    [Return the i-th partition (conj/disj), and remove it from f.]
  Description [Return the i-th partition (conj/disj), and remove it from f.]
  SideEffects []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddPartSortByTopVarAcc(
  Ddi_Bdd_t * f,
  int increasing
)
{
  int i, j, n;
  Ddi_Bdd_t *part;

  if (!Ddi_BddIsPartConj(f) && !Ddi_BddIsPartDisj(f)) return NULL;

  n = Ddi_BddPartNum(f);
  for (i=0; i<n; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddPartRead(f,i);
    f_i->common.size = Ddi_VarCurrPos(Ddi_BddTopVar(f_i));
  }
  for (i=0; i<n-1; i++) {
    int jMin=i;
    Ddi_Bdd_t *fMin = Ddi_BddPartRead(f,i);
    for (j=i+1; j<n; j++) {
      Ddi_Bdd_t *f_j = Ddi_BddPartRead(f,j);
      if (increasing ? (f_j->common.size < fMin->common.size) : 
          (f_j->common.size > fMin->common.size)) {
        jMin = j; fMin = f_j;
      }
    } 
    if (jMin != i) {
      Ddi_BddPartSwap(f,i,jMin);
    }
  }
  for (i=0; i<n; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddPartRead(f,i);
    f_i->common.size = 0;
  }

  return f;
}

/**Function********************************************************************
  Synopsis    [Return the i-th partition (conj/disj), and remove it from f.]
  Description [Return the i-th partition (conj/disj), and remove it from f.]
  SideEffects []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddPartQuickExtract(
  Ddi_Bdd_t * f,
  int i
)
{
  Ddi_Bdd_t *result, *last;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  Pdtutil_Assert((i>=0)&&(i<Ddi_BddPartNum(f)),
    "partition index out of bounds");

  last = Ddi_BddPartExtract(f, Ddi_BddPartNum(f)-1);
  if (i == Ddi_BddPartNum(f)) /* nothing to do */
     return last;

  result = Ddi_BddDup(Ddi_BddPartRead(f, i));
  Ddi_BddPartWrite(f, i, last);
  Ddi_Free(last);

  return result;
}

/**Function********************************************************************
  Synopsis    [Add i-th partition. Result accumulated]
  Description [Add new partition at position i. Result accumulated.
    Higher partitions are shifted.]
  SideEffects []
  SeeAlso     [Ddi_BddPartWrite]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddPartInsert (
  Ddi_Bdd_t *f        /* partitioned BDD */,
  int i               /* position of new partition */,
  Ddi_Bdd_t *newp     /* new partition */
)
{
  DdiConsistencyCheck(f,Ddi_Bdd_c);
  switch (f->common.code) {
  case Ddi_Bdd_Mono_c:
  case Ddi_Bdd_Meta_c:
    Pdtutil_Assert(0,"operation requires partitioned BDD");
    break;
  case Ddi_Bdd_Part_Conj_c:
  case Ddi_Bdd_Part_Disj_c:
    DdiArrayInsert(f->data.part,i,(Ddi_Generic_t *)newp,Ddi_Dup_c);
#if 0
    if (Ddi_BddIsOne(newp) && (f->common.code==Ddi_Bdd_Part_Conj_c)) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "^1^")
      );
    }
    if (Ddi_BddIsZero(newp) && (f->common.code==Ddi_Bdd_Part_Disj_c)) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "^0^")
      );
    }
#endif
    f->common.size = 0;
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }
  return(f);
}

/**Function********************************************************************
  Synopsis    [Add last partition. Result accumulated]
  Description [Add new partition at last position. Result accumulated]
  SideEffects []
  SeeAlso     [Ddi_BddPartInsert]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddPartInsertLast (
  Ddi_Bdd_t *f        /* partitioned BDD */,
  Ddi_Bdd_t *newp     /* new partition */
)
{
  return(Ddi_BddPartInsert(f,Ddi_BddPartNum(f),newp));
}


/**Function********************************************************************
  Synopsis    [Add last partition. Result accumulated. No duplicate inserted]
  Description [Add new partition at last position. Result accumulated.
               Partition is not inserted if already present]
  SideEffects []
  SeeAlso     [Ddi_BddPartInsert]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddPartInsertLastNoDup (
  Ddi_Bdd_t *f        /* partitioned BDD */,
  Ddi_Bdd_t *newp     /* new partition */
)
{
  int k;
  for (k=0; k<Ddi_BddPartNum(f); k++) {
    if (Ddi_BddEqual(Ddi_BddPartRead(f,k),newp)) {
      break;
    }
  }
  if (k==Ddi_BddPartNum(f)) {
    return(Ddi_BddPartInsert(f,Ddi_BddPartNum(f),newp));
  }
  else {
    return (NULL);
  }
}

/**Function********************************************************************
  Synopsis    [Remove the i-th partition (conj/disj).]
  Description [Remove the i-th partition (conj/disj).
               Equivalent to extract + free.]
  SideEffects []
******************************************************************************/
void
Ddi_BddPartRemove(
  Ddi_Bdd_t * f,
  int i
)
{
  Ddi_Bdd_t *p;
  DdiConsistencyCheck(f,Ddi_Bdd_c);
  Pdtutil_Assert((i>=0)&&(i<Ddi_BddPartNum(f)),
    "partition index out of bounds");
  switch (f->common.code) {
  case Ddi_Bdd_Mono_c:
  case Ddi_Bdd_Meta_c:
    Pdtutil_Assert(0,"partition removal requires partitioned BDD");
    break;
  case Ddi_Bdd_Part_Conj_c:
  case Ddi_Bdd_Part_Disj_c:
    f->common.size = 0;
    p = Ddi_BddPartExtract(f,i);
    Ddi_Free(p);
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }
}

/**Function********************************************************************
  Synopsis    [Remove the i-th partition (conj/disj).]
  Description [Remove the i-th partition (conj/disj).
               Equivalent to extract + free.]
  SideEffects []
******************************************************************************/
void
Ddi_BddPartQuickRemove(
  Ddi_Bdd_t * f,
  int i
)
{
  Ddi_Bdd_t *last;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  Pdtutil_Assert((i>=0)&&(i<Ddi_BddPartNum(f)),
    "partition index out of bounds");

  last = Ddi_BddPartExtract(f, Ddi_BddPartNum(f)-1);
  if (i == Ddi_BddPartNum(f)) {
     Ddi_Free(last);
     return;
  }
  Ddi_BddPartWrite(f, i, last);
  Ddi_Free(last);
}

/**Function********************************************************************
  Synopsis    [Create a monolithic BDD from a partitioned one]
  Description [Create a monolithic BDD from a partitioned one]
  SideEffects []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeMono (
  Ddi_Bdd_t *f        /* input function */
)
{
  Ddi_Bdd_t *r=NULL, *p;
  int i;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  switch (f->common.code) {
  case Ddi_Bdd_Mono_c:
    return (Ddi_BddDup(f));
    break;
  case Ddi_Bdd_Meta_c:
    r = Ddi_BddDup(f);
    Ddi_BddSetMono (r);
    break;
  case Ddi_Bdd_Aig_c:
    r = Ddi_BddMakeFromAig (f);
    break;
  case Ddi_Bdd_Part_Conj_c:
    if (Ddi_BddPartNum(f) == 0) {
      Pdtutil_Warning(1,"no partitions in conj part");
      r = Ddi_BddMakeConst(Ddi_ReadMgr(f),1);
      break;
    }
    r = Ddi_BddMakeMono(Ddi_BddPartRead(f,0));
    for (i=1;i<Ddi_BddPartNum(f);i++) {
       p = Ddi_BddMakeMono(Ddi_BddPartRead(f,i));
       Ddi_BddAndAcc(r,p);
       Ddi_Free(p);
    }
    break;
  case Ddi_Bdd_Part_Disj_c:
    if (Ddi_BddPartNum(f) == 0) {
      Pdtutil_Warning(1,"no partitions in disj part");
      r = Ddi_BddMakeConst(Ddi_ReadMgr(f),0);
      break;
    }
    r = Ddi_BddMakeMono(Ddi_BddPartRead(f,0));
    for (i=1;i<Ddi_BddPartNum(f);i++) {
       p = Ddi_BddMakeMono(Ddi_BddPartRead(f,i));
       Ddi_BddOrAcc(r,p);
       Ddi_Free(p);
    }
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }

  return(r);
}

/**Function********************************************************************
  Synopsis    [Convert a BDD to monolitic (if required). Result accumulated]
  Description [Convert a BDD to monolitic (if required). Result accumulated]
  SideEffects []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddSetMono (
  Ddi_Bdd_t *f        /* input function */
)
{
  Ddi_Bdd_t *m;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  switch (f->common.code) {
  case Ddi_Bdd_Mono_c:
    return (f);
  case Ddi_Bdd_Meta_c:
    Ddi_BddFromMeta(f);
    break;
  case Ddi_Bdd_Aig_c:
    m = Ddi_BddMakeMono(f);
    DdiGenericDataCopy((Ddi_Generic_t *)f,(Ddi_Generic_t *)m);
    Ddi_Free(m);
    break;
  case Ddi_Bdd_Part_Conj_c:
  case Ddi_Bdd_Part_Disj_c:
    m = Ddi_BddMakeMono(f);
    DdiArrayFree(f->data.part);
    f->common.code = m->common.code;
    switch (f->common.code) {
      case Ddi_Bdd_Mono_c:
        Cudd_Ref(m->data.bdd);
        f->data.bdd = m->data.bdd;
      break;
      case Ddi_Bdd_Meta_c:
        f->data.meta = DdiMetaDup(m->data.meta);
      break;
    default:
      Pdtutil_Assert(0,"Wrong DDI node code");
      break;
    }
    Ddi_Free(m);
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }

  f->common.size = 0;
  return(f);
}

/**Function********************************************************************
  Synopsis    [Create an AIG from BDD]
  Description [Create an AIG from BDD]
  SideEffects []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeAig (
  Ddi_Bdd_t *f        /* input function */
)
{
  Ddi_Bdd_t *r=NULL; //, *p;
  //int i;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  switch (f->common.code) {
  case Ddi_Bdd_Aig_c:
    r = Ddi_BddDup(f);
    break;
  case Ddi_Bdd_Mono_c:
    r = Ddi_AigMakeFromBdd(f);
    break;
  case Ddi_Bdd_Meta_c:
  case Ddi_Bdd_Part_Conj_c:
  case Ddi_Bdd_Part_Disj_c:
    r = Ddi_BddDup(f);
    Ddi_BddSetAig (r);
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }

  return(r);
}

/**Function********************************************************************
  Synopsis    [Convert a BDD to AIG. Result accumulated]
  Description [Convert a BDD to AIG. Result accumulated]
  SideEffects []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddSetAig (
  Ddi_Bdd_t *f        /* input function */
)
{
  Ddi_Bdd_t *m, *p;
  Ddi_Mgr_t *ddiMgr=Ddi_ReadMgr(f);
  int i;
  
  DdiConsistencyCheck(f,Ddi_Bdd_c);
  switch (f->common.code) {
  case Ddi_Bdd_Aig_c:
    break;
  case Ddi_Bdd_Mono_c:
    m = Ddi_AigMakeFromBdd(f);
    DdiGenericDataCopy((Ddi_Generic_t *)f,(Ddi_Generic_t *)m);
    Ddi_Free(m);
    break;
  case Ddi_Bdd_Meta_c:
    Ddi_AigFromMeta(f);
    break;
  case Ddi_Bdd_Part_Conj_c:
    m = Ddi_BddMakeConstAig(ddiMgr,1);
    for (i=0; i<Ddi_BddPartNum(f); i++) {
      p = Ddi_BddPartRead(f,i);
      Ddi_BddSetAig(p);
      Ddi_BddAndAcc(m,p);
    }
    DdiGenericDataCopy((Ddi_Generic_t *)f,(Ddi_Generic_t *)m);
    Ddi_Free(m);
    break;
  case Ddi_Bdd_Part_Disj_c:
    m = Ddi_BddMakeConstAig(ddiMgr,0);
    while (Ddi_BddIsPartDisj(f)&&Ddi_BddPartNum(f)!=0) {
      p = Ddi_BddPartExtract(f,0);
      Ddi_BddSetAig(p);
      Ddi_BddOrAcc(m,p);
      Ddi_Free(p);
    }
    DdiGenericDataCopy((Ddi_Generic_t *)f,(Ddi_Generic_t *)m);
    Ddi_Free(m);
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }

  return(f);
}

/**Function********************************************************************
  Synopsis    [Flatten recursive partitions]
  Description []
  SideEffects []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeFlattened (
  Ddi_Bdd_t *f        /* input function */
)
{
  Ddi_Bdd_t *r=NULL, *p;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(f);
  int i,j;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  switch (f->common.code) {
  case Ddi_Bdd_Mono_c:
  case Ddi_Bdd_Meta_c:
  case Ddi_Bdd_Aig_c:
    r = Ddi_BddDup(f);
    break;
  case Ddi_Bdd_Part_Conj_c:
    i = 0;
    do {
      if (i>=Ddi_BddPartNum(f)) {
        p = NULL;
        break;
      }
      p = Ddi_BddPartRead(f,i);
      i++;
    } while (Ddi_BddIsOne(p));

    if (p == NULL) {
      return (Ddi_BddMakeConst(ddiMgr,1));
    }
    r = Ddi_BddMakeFlattened(p);
    Ddi_BddSetPartConj(r);
    for (;i<Ddi_BddPartNum(f);i++) {
      p = Ddi_BddMakeFlattened(Ddi_BddPartRead(f,i));
      while ((Ddi_BddIsPartConj(p)||Ddi_BddIsPartDisj(p))
        &&(Ddi_BddPartNum(p)==1)) {
      Ddi_Bdd_t *aux = p;
      p = Ddi_BddPartExtract(aux,0);
      Ddi_Free(aux);
      }
      if (Ddi_BddIsPartConj(p)) {
        for (j=0;j<Ddi_BddPartNum(p);j++) {
          if (!Ddi_BddIsOne(Ddi_BddPartRead(p,j))) {
            Ddi_BddPartInsertLastNoDup(r,Ddi_BddPartRead(p,j));
        }
      }
      }
      else {
        if (!Ddi_BddIsOne(p)) {
          Ddi_BddPartInsertLastNoDup(r,p);
      }
      }
      Ddi_Free(p);
    }
    while ((Ddi_BddIsPartConj(r) || Ddi_BddIsPartConj(r)) &&
           (Ddi_BddPartNum(r) == 1)) {
      Ddi_Bdd_t *r0 = Ddi_BddPartExtract(r,0);
      Ddi_Free(r);
      r = r0;
    }
    break;
  case Ddi_Bdd_Part_Disj_c:
    {
      Ddi_Bdd_t *aux = Ddi_BddNot(f);
      r = Ddi_BddNotAcc(Ddi_BddMakeFlattened(aux));
      Ddi_Free(aux);
      if (Ddi_BddIsPartDisj(r)&&(Ddi_BddPartNum(r)==1)) {
        Ddi_BddSetMono(r);
      }
    }
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }

  Ddi_BddSuppDetach(r);

  return(r);
}


/**Function********************************************************************
  Synopsis    [Flatten recursive partitions]
  Description []
  SideEffects []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddSetFlattened (
  Ddi_Bdd_t *f        /* input function */
)
{
  Ddi_Bdd_t *m;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  switch (f->common.code) {
  case Ddi_Bdd_Mono_c:
  case Ddi_Bdd_Meta_c:
  case Ddi_Bdd_Aig_c:
    return (f);
    break;
  case Ddi_Bdd_Part_Conj_c:
  case Ddi_Bdd_Part_Disj_c:
    m = Ddi_BddMakeFlattened(f);
    DdiGenericDataCopy((Ddi_Generic_t *)f,(Ddi_Generic_t *)m);
    Ddi_Free(m);
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }

  return(f);
}


/**Function********************************************************************
  Synopsis    [Create a clustered BDD from a partitioned one]
  Description [Create a clustered BDD from a partitioned one.
    Conjunctions/disjunctions are executed up to the size threshold
    (sizes greater than threshold are aborted).]
  SideEffects []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeClustered (
  Ddi_Bdd_t *f        /* input function */,
  int threshold       /* size threshold */
)
{
  Ddi_Bdd_t *r=NULL, *p, *lastP, *newP;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(f);
  int i;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  switch (f->common.code) {
  case Ddi_Bdd_Mono_c:
  case Ddi_Bdd_Meta_c:
  case Ddi_Bdd_Aig_c:
    return (Ddi_BddDup(f));
    break;
  case Ddi_Bdd_Part_Conj_c:
    p = Ddi_BddPartRead(f,0);
    while (Ddi_BddIsPartConj(p)&&Ddi_BddPartNum(p)==0) {
      p = Ddi_BddPartExtract(f,0);
      Ddi_Free(p);
      p = Ddi_BddPartRead(f,0);
    }
    r = Ddi_BddMakeClustered(Ddi_BddPartRead(f,0),threshold);
    Ddi_BddSetPartConj(r);
    for (i=1;i<Ddi_BddPartNum(f);i++) {
      p = Ddi_BddMakeClustered(Ddi_BddPartRead(f,i),threshold);
      if ((Ddi_BddIsPartConj(p)||Ddi_BddIsPartDisj(p))
        &&(Ddi_BddPartNum(p)==1)) {
        Ddi_BddSetMono(p);
      }
      if (Ddi_BddIsMono(p)||Ddi_BddIsAig(p)) {
        Ddi_MgrAbortOnSiftEnableWithThresh(ddiMgr,2*threshold,1);

        lastP = Ddi_BddPartRead(r,Ddi_BddPartNum(r)-1);
        newP = Ddi_BddAnd(lastP,p);

        if ((ddiMgr->abortedOp)||
            ((threshold>=0)&&(Ddi_BddSize(newP)>threshold))) {
          Ddi_BddPartInsertLast(r,p);
        }
        else {
          if (!Ddi_BddIsOne(newP)||(Ddi_BddPartNum(r)==1)) {
            Ddi_BddPartWrite(r,Ddi_BddPartNum(r)-1,newP);
        }
          else {
            lastP = Ddi_BddPartExtract(r,Ddi_BddPartNum(r)-1);
            Ddi_Free(lastP);
        }
      }
        Ddi_Free(newP);
        Ddi_MgrAbortOnSiftDisable(ddiMgr);
      }
      else {
        Ddi_BddPartInsertLast(r,p);
      }
      Ddi_Free(p);
    }
    if (Ddi_BddPartNum(r) == 0) {
      Ddi_BddPartInsertLast(r,Ddi_MgrReadOne(ddiMgr));
    }
    else if (Ddi_BddIsPartDisj(r)&&(Ddi_BddPartNum(r)==1)) {
      Ddi_BddSetMono(r);
    }
    break;
  case Ddi_Bdd_Part_Disj_c:
    {
      Ddi_Bdd_t *aux = Ddi_BddNot(f);
      r = Ddi_BddNotAcc(Ddi_BddMakeClustered(aux,threshold));
      Ddi_Free(aux);
      if (Ddi_BddIsPartDisj(r)&&(Ddi_BddPartNum(r)==1)) {
        Ddi_BddSetMono(r);
      }
    }
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }

  return(r);
}

/**Function********************************************************************
  Synopsis    [Create a clustered BDD from a partitioned one]
  Description [Create a clustered BDD from a partitioned one.
    Conjunctions/disjunctions are executed up to the size threshold
    (sizes greater than threshold are aborted).]
  SideEffects []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddSetClustered (
  Ddi_Bdd_t *f        /* input function */,
  int threshold       /* size threshold */
)
{
  Ddi_Bdd_t *m;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  switch (f->common.code) {
  case Ddi_Bdd_Mono_c:
  case Ddi_Bdd_Aig_c:
    return (f);
  case Ddi_Bdd_Meta_c:
    /* Ddi_BddFromMeta(f); Keep meta */
    break;
  case Ddi_Bdd_Part_Conj_c:
  case Ddi_Bdd_Part_Disj_c:
    m = Ddi_BddMakeClustered(f,threshold);
    DdiGenericDataCopy((Ddi_Generic_t *)f,(Ddi_Generic_t *)m);
    Ddi_Free(m);
    break;
  default:
    Pdtutil_Assert(0,"Wrong DDI node code");
    break;
  }

  return(f);
}

/**Function********************************************************************
  Synopsis    [Return the variable index (CUDD variable index)]
  SideEffects [none]
  SeeAlso     [Ddi_VarFromIndex]
******************************************************************************/
void
Ddi_BddWriteMark (
  Ddi_Bdd_t *f,
  int val
)
{
  if (f->common.info == NULL) {
    f->common.info = Pdtutil_Alloc(Ddi_Info_t, 1);
    f->common.info->infoCode = Ddi_Info_Mark_c;
    f->common.info->bdd.auxPtr = NULL;
  }
  switch (f->common.info->infoCode) {
  case Ddi_Info_Mark_c:
    f->common.info->bdd.mark = val;
    break;
  case Ddi_Info_Compose_c:
    f->common.info->compose.mark = val;
    break;
  case Ddi_Info_Eq_c:
    f->common.info->eq.mark = val;
    break;
  case Ddi_Info_Var_c:
  default:
    Pdtutil_Assert(0,"illegal mark write");
  }

}

/**Function********************************************************************
  Synopsis    [Return the variable index (CUDD variable index)]
  SideEffects [none]
  SeeAlso     [Ddi_VarFromIndex]
******************************************************************************/
int
Ddi_BddReadMark (
  Ddi_Bdd_t *f 
)
{
  Pdtutil_Assert(f->common.info != NULL,"bdd info required");
  switch (f->common.info->infoCode) {
  case Ddi_Info_Mark_c:
    return (f->common.info->bdd.mark);
    break;
  case Ddi_Info_Compose_c:
    return (f->common.info->compose.mark);
    break;
  case Ddi_Info_Eq_c:
    return (f->common.info->eq.mark);
    break;
  case Ddi_Info_Var_c:
  default:
    Pdtutil_Assert(0,"illegal mark read");
  }
}

/**Function********************************************************************
  Synopsis    [Return the var array of an equivalence function
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Vararray_t *
Ddi_BddReadEqVars (
  Ddi_Bdd_t *f 
)
{
  Pdtutil_Assert(f->common.info != NULL,"bdd info required");
  return (f->common.info->eq.vars);
}

/**Function********************************************************************
  Synopsis    [Return the subst array of an equivalence function
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddReadEqSubst (
  Ddi_Bdd_t *f 
)
{
  if (f->common.info==NULL) return NULL;
  if (f->common.info->infoCode != Ddi_Info_Eq_c) return NULL;
  return (f->common.info->eq.subst);
}

/**Function********************************************************************
  Synopsis    [Return the var array of an equivalence function
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Vararray_t *
Ddi_BddReadComposeVars (
  Ddi_Bdd_t *f 
)
{
  Pdtutil_Assert(f->common.info != NULL,"bdd info required");
  return (f->common.info->compose.vars);
}
/**Function********************************************************************
  Synopsis    [Return the var array of an equivalence function
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Vararray_t *
Ddi_BddReadComposeRefVars (
  Ddi_Bdd_t *f 
)
{
  Pdtutil_Assert(f->common.info != NULL,"bdd info required");
  return (f->common.info->compose.refVars);
}

/**Function********************************************************************
  Synopsis    [Return the var array of an equivalence function
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddReadComposeF (
  Ddi_Bdd_t *f 
)
{
  if (f->common.info==NULL) return NULL;
  if (f->common.info->infoCode != Ddi_Info_Compose_c) return NULL;
  return (f->common.info->compose.f);
}

/**Function********************************************************************
  Synopsis    [Return the var array of an equivalence function
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddReadComposeCare (
  Ddi_Bdd_t *f 
)
{
  if (f->common.info==NULL) return NULL;
  if (f->common.info->infoCode != Ddi_Info_Compose_c) return NULL;
  return (f->common.info->compose.care);
}

/**Function********************************************************************
  Synopsis    [Return the var array of an equivalence function
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddReadComposeConstr (
  Ddi_Bdd_t *f 
)
{
  if (f->common.info==NULL) return NULL;
  if (f->common.info->infoCode != Ddi_Info_Compose_c) return NULL;
  return (f->common.info->compose.constr);
}

/**Function********************************************************************
  Synopsis    [Return the var array of an equivalence function
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddReadComposeCone (
  Ddi_Bdd_t *f 
)
{
  if (f->common.info==NULL) return NULL;
  if (f->common.info->infoCode != Ddi_Info_Compose_c) return NULL;
  return (f->common.info->compose.cone);
}

/**Function********************************************************************
  Synopsis    [Return the subst array of an equivalence function
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddReadComposeSubst (
  Ddi_Bdd_t *f 
)
{
  Pdtutil_Assert(f->common.info != NULL,"bdd info required");
  return (f->common.info->compose.subst);
}

/**Function********************************************************************
  Synopsis     [Prints a BDD]
  Description  []
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_BddPrint (
  Ddi_Bdd_t  *f,
  FILE *fp
)
{
  DdiConsistencyCheck(f,Ddi_Bdd_c);
  DdiGenericPrint((Ddi_Generic_t *)f,fp);
}

/**Function********************************************************************
  Synopsis     [Prints Statistics of a BDD]
  Description  []
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_BddPrintStats (
  Ddi_Bdd_t  *f,
  FILE *fp
)
{
  DdiConsistencyCheck(f,Ddi_Bdd_c);
  DdiGenericPrintStats((Ddi_Generic_t *)f,fp);
}

/**Function********************************************************************
  Synopsis    [Output a cube to string. Return true if succesful.]
  Description [Output a cube to string. Return true if succesful.
    The set of variables to be considered
    is given as input (if NULL, true support is used).
    Variables are sorted by absolute index (which is constant across sifting),
    NOT by variable ordering.
    The procedure allows omitting variables in the true support, which
    are existentially quantified out by cube.
    ]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
int
Ddi_BddPrintCubeToString (
  Ddi_Bdd_t *f         /* BDD */,
  Ddi_Varset_t *vars  /* Variables */,
  char *string        /* output string */
)
{
  int i, j=0;
  Ddi_BddMgr *dd;
  int nMgrVars;
  DdNode *f_cu;
  DdManager *mgrCU;

  if (!Ddi_BddIsCube(f)) {
    return(0);
  }

  if (vars == NULL) {
    vars = Ddi_BddSupp(f);
  } else {
    vars = Ddi_VarsetDup(vars);
  }

  dd = Ddi_ReadMgr(f);
  nMgrVars = Ddi_MgrReadNumVars(dd);
  mgrCU = Ddi_MgrReadMgrCU(dd);

  /*
   *  Here we work in CUDD !
   */

  f_cu = Ddi_BddToCU(f);
  {
    int *cube;
    CUDD_VALUE_TYPE value;
    DdGen *gen;

    Cudd_ForeachCube(mgrCU,f_cu,gen,cube,value) {
      for (i=0,j=0; i<mgrCU->size; i++) {
        /* skip variables not in varset */
      if (!Ddi_VarInVarset(vars,Ddi_VarFromIndex(dd,i))) {
          continue;
        }
        switch (cube[i]) {
          case 0:
          string[j++]='0';
          break;
        case 1:
          string[j++]='1';
          break;
        case 2:
          string[j++]='-';
          break;
        default:
          string[j++]='?';
          break;
        }
      }
    }
    string[j++]='\0';
  }

  /*
   *  End working in CUDD !
   */

  Ddi_Free(vars);

  return (1);
}

/**Function********************************************************************
  Synopsis    [Outputs the cubes of a BDD on file]
  Description [This function outputs the cubes of a BDD on file.
    Only monolithic BDDs are supported. The set of variables to be considered
    is given as input (if NULL the true support is used), to allow generating
    don't cares, data are sorted by absolute index, NOT by variable ordering.
    The procedure allows omitting variables in the true support, which
    are existentially quantified out before generating cubes, to avoid
    repetitions.
    A limit on the number of cubes generated can be specified.
    Use a negative value for no bound.
    ]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
int
Ddi_BddPrintCubes (
  Ddi_Bdd_t *f         /* BDD */,
  Ddi_Varset_t *vars  /* Variables */,
  int cubeNumberMax   /* Maximum number of cubes printed */,
  int formatPla       /* Prints a 1 at the end of the cube (PLA format) */,
  char *filename      /* File Name */,
  FILE *fp            /* Pointer to the store file */
  )
{
  int i, flagFile, ncubes, id, ret = 1;
  Ddi_Varset_t *aux;
  char *printvars;
  Ddi_BddMgr *dd;
  int nMgrVars;
  DdNode *f_cu;
  DdManager *mgrCU;

  DdiConsistencyCheck(f,Ddi_Bdd_c);

  if (vars == NULL) {
    vars = Ddi_BddSupp(f);
  } else {
    vars = Ddi_VarsetDup(vars);
  }
  fp = Pdtutil_FileOpen (fp, filename, "w", &flagFile);

  switch (f->common.code) {
  case Ddi_Bdd_Meta_c:
    Pdtutil_Assert(0,"operation not supported for meta BDD");
    break;
  case Ddi_Bdd_Mono_c:

    dd = Ddi_ReadMgr(f);
    nMgrVars = Ddi_MgrReadNumCuddVars(dd);
    mgrCU = Ddi_MgrReadMgrCU(dd);

    printvars = Pdtutil_Alloc(char, nMgrVars);

    aux = Ddi_VarsetDiffAcc(Ddi_BddSupp(f),vars);
    f = Ddi_BddExist(f,aux);
    Ddi_Free(aux);

    for (i=0; i<nMgrVars; i++)
      printvars[i] = 0;

    while ( !Ddi_VarsetIsVoid (vars) ) {
      id = Ddi_VarCuddIndex(Ddi_VarsetTop(vars));
      printvars[id] = 1;
      Ddi_VarsetNextAcc(vars);
    }

    /*
     *  Here we work in CUDD !
     */

    f_cu = Ddi_BddToCU(f);
    {
      int *cube;
      CUDD_VALUE_TYPE value;
      DdGen *gen;

      ncubes = 0;
      Cudd_ForeachCube(mgrCU,f_cu,gen,cube,value) {
        if ((cubeNumberMax>0) && (++ncubes>cubeNumberMax)) {
          break;
        }
        for (i=0; i<mgrCU->size; i++) {
          /* skip variables not to be printed */
          if (printvars[i]==0) {
            continue;
          }

          switch (cube[i]) {
            case 0:
              fprintf(fp, "0");
              break;
            case 1:
              fprintf(fp, "1");
              break;
            case 2:
              fprintf(fp, "-");
              break;
            default:
              fprintf(fp, "?");
              break;
          }
        }
        if (formatPla == 1) {
          fprintf(fp, " 1");
        }
        fprintf(fp,"\n");
      }
    }

    /*
     *  End working in CUDD !
     */

    Pdtutil_Free (printvars);
    Ddi_Free (f);

    break;
  case Ddi_Bdd_Part_Conj_c:
  case Ddi_Bdd_Part_Disj_c:
    fprintf(fp, "Partitioned BDD.\n");
    for (i=0; i<Ddi_BddPartNum(f); i++) {
      fprintf(fp, "Partition %d.\n", i);
      Ddi_BddPrintCubes (Ddi_BddPartRead(f,i),
        vars,cubeNumberMax,formatPla,NULL,fp);
    }
    break;
    default:
      Pdtutil_Assert(0,"Wrong DDI node code");
      break;
  }

  Ddi_Free(vars);

  Pdtutil_FileClose (fp, &flagFile);

  return (ret);
}

/**Function********************************************************************
  Synopsis    [Store BDD on file]
  Description [This function stores only a BDD (not a BDD array).
    The BDD is stored in the DDDMP format. The parameter "mode"
    can be DDDMP_MODE_TEXT, DDDMP_MODE_BINARY or
    DDDMP_MODE_DEFAULT.<br>
    The function returns 1 if succefully stored, 0 otherwise.
    ]
  SideEffects [None]
  SeeAlso     [Ddi_BddLoad,Dddmp_cuddBddStore]
******************************************************************************/

int
Ddi_BddStore (
  Ddi_Bdd_t  *f      /* BDD */,
  char *ddname       /* dd name (or NULL) */,
  char mode          /* storing mode */,
  char *filename     /* file name */,
  FILE *fp           /* pointer to the store file */ )
{
  int flagFile, ret, i;
  Ddi_Bdd_t *g=NULL;
  Ddi_BddMgr *dd;
  Ddi_Var_t *v;
  char **vnames;
  int *vauxids;


  DdiConsistencyCheck(f,Ddi_Bdd_c);

  dd = Ddi_ReadMgr(f);    /* dd manager */
  vnames = Ddi_MgrReadVarnames(dd);
  vauxids = Ddi_MgrReadVarauxids(dd);

  fp = Pdtutil_FileOpen (fp, filename, "w", &flagFile);
  if (fp == NULL) {
     return(0);
  }

  switch (f->common.code) {
  case Ddi_Bdd_Aig_c:
    Ddi_AigNetStore (f, NULL, fp, Pdtutil_Aig2BenchName_c);
    break;
  case Ddi_Bdd_Mono_c:
    fprintf(fp,"# MONO\n");
    ret = Dddmp_cuddBddStore(dd->mgrCU,ddname,Ddi_BddToCU(f),
      vnames,vauxids,mode,DDDMP_VARNAMES,filename,fp);
    break;
  case Ddi_Bdd_Meta_c:
    {
      Ddi_Bdd_t *f2 = Ddi_BddDup(f);
      Ddi_BddSetPartConj(f);
      Ddi_BddStore (f2,ddname,mode,filename,fp);
    }
    break;
  case Ddi_Bdd_Part_Conj_c:
    fprintf(fp,"# CPART\n");
    if (0&&(v=Ddi_VarFromName(dd, "PDT_BDD_INVARSPEC_VAR$PS"))) {
       g = Ddi_BddCofactor(f, v, 1);
       ret = DdiArrayStore (g->data.part,ddname,vnames,
               NULL,vauxids,mode,filename,fp);
       Ddi_Free(g);
    } else {
       ret = DdiArrayStore (f->data.part,ddname,vnames,
               NULL,vauxids,mode,filename,fp);
    }
    break;
  case Ddi_Bdd_Part_Disj_c:
    if (dd->settings.dump.partOnSingleFile) {
      fprintf(fp,"# DPART\n");
      ret = DdiArrayStore (f->data.part,ddname,vnames,
        NULL,vauxids,mode,filename,fp);
    }
    else {
      char name[PDTUTIL_MAX_STR_LEN];
      fprintf(fp,"# FILEDPART\n");
      for (i=0;i<Ddi_BddPartNum(f);i++) {
        fprintf(fp,"%s-p%d\n",filename,i);
        Ddi_BddStore(Ddi_BddPartRead(f,i),NULL,mode,name,NULL);
      }
    }
    break;
  default:
    Pdtutil_Assert(0,"illegal bdd node code");
  }

  Pdtutil_FileClose (fp, &flagFile);

  return(1);
}

/**Function********************************************************************
  Synopsis    [Load BDD from file]
  Description [This function loads only a BDD. If the file contain a BDDs'
    array, then will be load only the first BDD.<br>
    The BDD on file must be in the DDDMP format. The parameter
    "mode" can be DDDMP_MODE_TEXT, DDDMP_MODE_BINARY or
    DDDMP_MODE_DEFAULT.<br>
    The function returns the pointer of BDD root if succefully
    loaded, NULL otherwise.]
  SideEffects [None]
  SeeAlso     [Ddi_BddStore,Dddmp_cuddBddLoad]
******************************************************************************/

Ddi_Bdd_t *
Ddi_BddLoad (
  Ddi_BddMgr *dd      /* dd manager */,
  int varmatchmode   /* variable matching mode */,
  char mode          /* loading mode */,
  char *filename     /* file name */,
  FILE *fp           /* file pointer */
  )
{
  Ddi_Bdd_t *f, *p;
  int flagFile;
  char code[200], auxname[200];
  Ddi_Bddarray_t *array;    /* descriptor of array */
  char **vnames = Ddi_MgrReadVarnames(dd);
  int *vauxids = Ddi_MgrReadVarauxids(dd);
  int firstC;

  f = NULL;

  fp = Pdtutil_FileOpen (fp, filename, "r", &flagFile);
  if (fp == NULL) {
    return (0);
  }

  firstC = fgetc(fp);
  ungetc (firstC, fp);
  if (((char)firstC) != '#') {
    /*
     *  No format comment: default is mono (1 component)
     *  or conjunctive part (>1 components)
     */
    array = Ddi_BddarrayLoad(dd,
             vnames,vauxids,mode,filename,fp);
    if (Ddi_BddarrayNum(array)==1) {
      f = Ddi_BddDup(Ddi_BddarrayRead(array,0));
      Ddi_Free(array);
    }
    else {
      f = Ddi_BddMakePartConjFromArray(array);
      Ddi_Free(array);
    }
  }
  else {
    if (fgets(code,199,fp)==NULL)
      return NULL;
    if ((strncmp(code,"# MONO",6)==0)
      || (strncmp(code,"MONO",4)==0)) {
      /*
       * monolithic (1 component)
       */
      f = Ddi_BddMakeFromCU(dd,
        Dddmp_cuddBddLoad(dd->mgrCU,(Dddmp_VarMatchType)varmatchmode,
        vnames,vauxids,NULL,mode,filename,fp));
      /* Deref is required since dddmp references loaded BDDs */
      Cudd_Deref(Ddi_BddToCU(f));
    }
    if ((strncmp(code,"# CPART",7)==0)
       || (strncmp(code,"CPART",5)==0)) {
      /*
       * conjunctive part
       */
      array = Ddi_BddarrayLoad(dd,
        vnames,vauxids,mode,filename,fp);
      f = Ddi_BddMakePartConjFromArray(array);
      Ddi_Free(array);
    }
    if ((strncmp(code,"# DPART",7)==0)
      || (strncmp(code,"DPART",5)==0)) {
      /*
       * disjunctive part
       */
      array = Ddi_BddarrayLoad(dd,
               vnames,vauxids,mode,filename,fp);
      f = Ddi_BddMakePartDisjFromArray(array);
      Ddi_Free(array);
    }
    if ((strncmp(code,"# FILEDPART",11)==0)
      || (strncmp(code,"FILEDPART",9)==0)) {
      /*
       * disjunctive part - partitions on separate files
       */
      f = NULL;
      while (fscanf(fp,"%s",auxname)!=EOF) {
        p = Ddi_BddLoad(dd,varmatchmode,mode,auxname,NULL);
        if (f==NULL) {
          f = Ddi_BddMakePartDisjFromMono(p);
      }
        else {
          Ddi_BddPartInsertLast(f,p);
        }
        Ddi_Free(p);
      }
    }
    if ((strncmp(code,"# EXPR",6)==0)
      || (strncmp(code,"EXPR",4)==0)) {
      /*
       * expression
       */
      f = Ddi_BddLoadCube(dd,fp,0);
    }
  }

  Pdtutil_FileClose (fp, &flagFile);

  return (f);
}


/**Function********************************************************************

  Synopsis    [Store BDD on file in CNF format]

  Description []

  SideEffects [None]

  SeeAlso     [Ddi_BddLoad,Dddmp_cuddBddStore]

******************************************************************************/

int
Ddi_BddStoreCnf (
  Ddi_Bdd_t *f                   /* IN: BDD */,
  Dddmp_DecompCnfStoreType mode  /* IN: storing mode */,
  int noHeader                   /* IN: Header print out flag */,
  char **vnames                  /* IN: variables names */,
  int *vbddIds                   /* IN: BDD variables Ids */,
  int *vauxIds                   /* IN: variables auxIds */,
  int *vcnfIds                   /* IN: CNF variables Ids */,
  int idInitial                  /* IN: initial Id for auxiliary variables */,
  int edgeInMaxTh                /* IN: Max # Incoming Edges */,
  int pathLengthMaxTh            /* IN: Max Path Length */,
  char *filename                 /* IN: file name */,
  FILE *fp                       /* IN: pointer to the store file */,
  int *clauseNPtr                /* OUT: number of added clauses */,
  int *varNewNPtr                /* OUT: number of added variables */
  )
{
  int flagFile, ret, i;
  Ddi_BddMgr *dd;
  int addedClauses, addedVars;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  dd = Ddi_ReadMgr(f);

  fp = Pdtutil_FileOpen (fp, filename, "w", &flagFile);
  if (fp == NULL) {
    return(0);
  }

  switch (f->common.code) {
  case Ddi_Bdd_Mono_c:
    ret = Dddmp_cuddBddStoreCnf (dd->mgrCU, Ddi_BddToCU(f), mode, noHeader,
      vnames, vbddIds, vauxIds, vcnfIds, idInitial, edgeInMaxTh, pathLengthMaxTh,
      filename, fp, &addedClauses, &addedVars);
    break;
  case Ddi_Bdd_Meta_c:
    Pdtutil_Assert(0,"operation not supported for meta BDD");
    break;
  case Ddi_Bdd_Part_Conj_c:
    ret = DdiArrayStoreCnf (f->data.part, mode, noHeader, vnames,
      vbddIds, vauxIds, vcnfIds, idInitial, edgeInMaxTh, pathLengthMaxTh,
      filename, fp, &addedClauses, &addedVars);
    break;
  case Ddi_Bdd_Part_Disj_c:
    if (dd->settings.dump.partOnSingleFile) {
      fprintf(fp,"c DPART\n");
      ret = DdiArrayStoreCnf (f->data.part, mode, noHeader, vnames,
        vbddIds, vauxIds, vcnfIds, idInitial, edgeInMaxTh, pathLengthMaxTh,
        filename, fp, &addedClauses, &addedVars);
    }
    else {
      char name[PDTUTIL_MAX_STR_LEN];
      int auxClauses, auxVars;
      addedClauses = addedVars = 0;
      fprintf(fp,"c FILEDPART\n");
      for (i=0;i<Ddi_BddPartNum(f);i++) {
        fprintf(fp,"%s-p%d\n",filename,i);
        Ddi_BddStoreCnf (Ddi_BddPartRead(f,i), mode, noHeader, vnames,
         vbddIds, vauxIds, vcnfIds, idInitial, edgeInMaxTh, pathLengthMaxTh,
         name, NULL, &auxClauses, &auxVars);
      addedClauses += auxClauses;
      addedVars += auxVars;
      }
    }
    break;
  default:
    Pdtutil_Assert(0,"illegal bdd node code");
  }

  Pdtutil_FileClose (fp, &flagFile);
  *clauseNPtr = addedClauses;
  *varNewNPtr = addedVars;

  return(1);
}

/**Function********************************************************************

  Synopsis    [Writes a single BDD in a dump file in Blif format.]

  Description [This function stores a single BDD.
    It calls the DdiArrayStoreBlif function which stores an array of
    BDDs. To do that it creates a BDD array with a single BDD entry.
    The function returns 1 if the array is succefully stored.
    It returns 0 otherwise.
  ]

  SideEffects [None]

  SeeAlso     [Ddi_BddLoad,Dddmp_cuddBddStore,DdiArrayStoreBlif]

******************************************************************************/

int
Ddi_BddStoreBlif (
  Ddi_Bdd_t *f         /* IN: BDD to be Stored */,
  char **inputNames    /* IN: Array of variable names (or NULL) */,
  char *outputName     /* IN: Array of root names (or NULL) */,
  char *modelName      /* IN: Model Name */,
  char *fname          /* IN: File name */,
  FILE *fp             /* IN: Pointer to the store file */
  )
{
  int retValue, i;
  Ddi_BddMgr *dd;
  Ddi_Bddarray_t *tmpArray;
  int fileClose = 0;

  dd = Ddi_ReadMgr(f);

  fp = Pdtutil_FileOpen (fp, fname, "w", &fileClose);

  tmpArray = Ddi_BddarrayAlloc (dd, 0);
  switch (f->common.code) {
    case Ddi_Bdd_Mono_c:
      Ddi_BddarrayInsertLast (tmpArray, f);
      if (outputName == NULL) {
        retValue = DdiArrayStoreBlif (tmpArray->array, inputNames,
          NULL, modelName, fname, fp);
      } else {
        retValue = DdiArrayStoreBlif (tmpArray->array, inputNames,
          &outputName, modelName, fname, fp);
      }

      break;
    case Ddi_Bdd_Meta_c:
      Pdtutil_Assert (0, "Operation not supported for meta BDD");
      retValue = 0;
      break;
    case Ddi_Bdd_Part_Conj_c:
      for (i=0; i<Ddi_BddPartNum(f); i++) {
      Ddi_Bdd_t *p = Ddi_BddPartRead(f,i);
        Pdtutil_Assert (Ddi_BddIsMono(p),
          "Operation not supported for multilevel partitioned BDD.");
      Ddi_BddarrayInsertLast(tmpArray,p);
      }
      retValue = DdiArrayStoreBlif (tmpArray->array, inputNames,
        NULL, modelName, fname, fp);
      break;
    case Ddi_Bdd_Part_Disj_c:
      for (i=0; i<Ddi_BddPartNum(f); i++) {
      Ddi_Bdd_t *p = Ddi_BddPartRead(f,i);
        Pdtutil_Assert (Ddi_BddIsMono(p),
          "Operation not supported for multilevel partitioned BDD.");
      Ddi_BddarrayInsertLast(tmpArray,p);
      }
      retValue = DdiArrayStoreBlif (tmpArray->array, inputNames,
        NULL, modelName, fname, fp);
      break;
    default:
      retValue = 0;
      Pdtutil_Assert (0, "Illegal bdd node code");
  }

  Ddi_Free (tmpArray);
  Pdtutil_FileClose (fp, &fileClose);

  return (retValue);
}

/**Function********************************************************************

  Synopsis    [Writes a single BDD in a dump file in prefix notation
    format.]

  Description [This function stores a single BDD.
    It calls the DdiArrayStorePrefix function which stores an array of
    BDDs. To do that it creates a BDD array with a single BDD entry.
    The function returns 1 if the array is succefully stored.
    It returns 0 otherwise.
  ]

  SideEffects [None]

  SeeAlso     [Ddi_BddLoad,Dddmp_cuddBddStore]

******************************************************************************/

int
Ddi_BddStorePrefix (
  Ddi_Bdd_t *f         /* IN: BDD to be Stored */,
  char **inputNames    /* IN: Array of variable names (or NULL) */,
  char *outputName     /* IN: Array of root names (or NULL) */,
  char *modelName      /* IN: Model Name */,
  char *fname          /* IN: File name */,
  FILE *fp             /* IN: Pointer to the store file */
  )
{
  int retValue;
  Ddi_BddMgr *dd;
  Ddi_Bddarray_t *tmpArray;

  dd = Ddi_ReadMgr(f);

  switch (f->common.code) {
    case Ddi_Bdd_Mono_c:
      tmpArray = Ddi_BddarrayAlloc (dd, 0);
      Ddi_BddarrayInsertLast (tmpArray, f);

      if (outputName == NULL) {
        retValue = DdiArrayStorePrefix (tmpArray->array, inputNames,
          NULL, modelName, fname, fp);
      } else {
        retValue = DdiArrayStorePrefix (tmpArray->array, inputNames,
          &outputName, modelName, fname, fp);
      }

      Ddi_Free (tmpArray);
      break;
    case Ddi_Bdd_Meta_c:
      Pdtutil_Assert (0, "Operation not supported for meta BDD");
      retValue = 0;
      break;
    case Ddi_Bdd_Part_Conj_c:
      Pdtutil_Assert (0, "Operation not supported for conjoint BDD.");
      retValue = 0;
      break;
    case Ddi_Bdd_Part_Disj_c:
      Pdtutil_Assert (0, "Operation not supported for disjoint BDD");
      retValue = 0;
      break;
    default:
      retValue = 0;
      Pdtutil_Assert (0, "Illegal bdd node code");
  }

  return (retValue);
}

/**Function********************************************************************

  Synopsis    [Writes a single BDD in a dump file in Smv format.]

  Description [This function stores a single BDD.
    It calls the DdiArrayStoreSmv function which stores an array of
    BDDs. To do that it creates a BDD array with a single BDD entry.
    The function returns 1 if the array is succefully stored.
    It returns 0 otherwise.
  ]

  SideEffects [None]

  SeeAlso     [Ddi_BddLoad,Dddmp_cuddBddStore,DdiArrayStoreBlif]

******************************************************************************/

int
Ddi_BddStoreSmv (
  Ddi_Bdd_t *f         /* IN: BDD to be Stored */,
  char **inputNames    /* IN: Array of variable names (or NULL) */,
  char *outputName     /* IN: Array of root names (or NULL) */,
  char *modelName      /* IN: Model Name */,
  char *fname          /* IN: File name */,
  FILE *fp             /* IN: Pointer to the store file */
  )
{
  int retValue;
  Ddi_BddMgr *dd;
  Ddi_Bddarray_t *tmpArray;

  dd = Ddi_ReadMgr(f);

  switch (f->common.code) {
    case Ddi_Bdd_Mono_c:
      tmpArray = Ddi_BddarrayAlloc (dd, 0);
      Ddi_BddarrayInsertLast (tmpArray, f);

      if (outputName == NULL) {
        retValue = DdiArrayStoreSmv (tmpArray->array, inputNames,
          NULL, modelName, fname, fp);
      } else {
        retValue = DdiArrayStoreSmv (tmpArray->array, inputNames,
          &outputName, modelName, fname, fp);
      }

      Ddi_Free (tmpArray);
      break;
    case Ddi_Bdd_Meta_c:
      Pdtutil_Assert (0, "Operation not supported for meta BDD");
      retValue = 0;
      break;
    case Ddi_Bdd_Part_Conj_c:
      Pdtutil_Assert (0, "Operation not supported for conjoinct BDD.");
      retValue = 0;
      break;
    case Ddi_Bdd_Part_Disj_c:
      Pdtutil_Assert (0, "Operation not supported for disjoinct BDD");
      retValue = 0;
      break;
    default:
      retValue = 0;
      Pdtutil_Assert (0, "Illegal bdd node code");
  }

  return (retValue);
}

/**Function********************************************************************

  Synopsis    [Compute the Dense Super or Subset of a Boolean functions]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Ddi_BddDenseSet (
  Ddi_DenseMethod_e method      /* Operation Code */,
  Ddi_Bdd_t *f                   /* Operand */,
  int threshold,
  int safe,
  int quality,
  double hardlimit
 )
{
  int suppSize;
  DdNode *sourceCudd, *resultCudd;
  Ddi_Bdd_t *result;
  Ddi_BddMgr *dd = Ddi_ReadMgr(f);
  DdManager *cuddMgr = Ddi_MgrReadMgrCU (dd);
  Ddi_Varset_t *supp;

  supp = Ddi_BddSupp (f);
  suppSize = Ddi_VarsetNum(supp);
  sourceCudd = Ddi_BddToCU(f);

  switch (method) {
    case Ddi_DenseNone_c:
      resultCudd = sourceCudd;
      break;
    case Ddi_DenseSubHeavyBranch_c:
      resultCudd = Cudd_SubsetHeavyBranch (cuddMgr, sourceCudd, suppSize,
        threshold);
      break;
    case Ddi_DenseSupHeavyBranch_c:
      resultCudd = Cudd_SupersetHeavyBranch (cuddMgr, sourceCudd, suppSize,
        threshold);
      break;
    case Ddi_DenseSubShortPath_c:
      resultCudd = Cudd_SubsetShortPaths (cuddMgr, sourceCudd, suppSize,
        threshold, (int)hardlimit);
      break;
    case Ddi_DenseSupShortPath:
      resultCudd = Cudd_SupersetShortPaths (cuddMgr, sourceCudd, suppSize,
        threshold, (int)hardlimit);
      break;
    case Ddi_DenseUnderApprox_c:
      resultCudd = Cudd_UnderApprox (cuddMgr, sourceCudd, suppSize, threshold,
        safe, quality);
      break;
    case Ddi_DenseOverApprox_c:
      resultCudd = Cudd_OverApprox (cuddMgr, sourceCudd, suppSize, threshold,
        safe, quality);
      break;
    case Ddi_DenseRemapUnder_c:
      resultCudd = Cudd_RemapUnderApprox (cuddMgr, sourceCudd, suppSize,
        threshold, quality);
      break;
    case Ddi_DenseRemapOver_c:
      resultCudd = Cudd_RemapOverApprox (cuddMgr, sourceCudd, suppSize,
        threshold, quality);
      break;
    case Ddi_DenseSubCompress_c:
      resultCudd = Cudd_SubsetCompress (cuddMgr, sourceCudd, suppSize,
        threshold);
      break;
    case Ddi_DenseSupCompress_c:
      resultCudd = Cudd_SupersetCompress (cuddMgr, sourceCudd, suppSize,
        threshold);
      break;
    default:
      resultCudd = sourceCudd;
      break;
  }

  result = Ddi_BddMakeFromCU (dd, resultCudd);

  Ddi_Free (supp);
  return (result);
}

/**Function********************************************************************
  Synopsis    [Compute the Dense Super or Subset of a Boolean functions]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddClippingAndAbstract (
  Ddi_Bdd_t *f                   /* First Operand */,
  Ddi_Bdd_t *g                   /* Second Operand */,
  Ddi_Varset_t *smoothV          /* Smooth Var Set, if NULL simple AND */,
  int recurThreshold             /* Max Recur Depth */,
  int direction                  /* 0 = under, 1 = over */
 )
{
  DdNode *opCudd1, *opCudd2, *resultCudd;
  Ddi_Bdd_t *result;
  Ddi_BddMgr *dd = Ddi_ReadMgr(f);
  DdManager *cuddMgr = Ddi_MgrReadMgrCU (dd);

  opCudd1 = Ddi_BddToCU (f);
  opCudd2 = Ddi_BddToCU (g);

  if (smoothV == NULL) {
    resultCudd =  Cudd_bddClippingAnd (cuddMgr, opCudd1,
      opCudd2, recurThreshold, direction);
  } else {
    resultCudd =  Cudd_bddClippingAndAbstract (cuddMgr, opCudd1,
      opCudd2, Ddi_VarsetToCU (smoothV), recurThreshold, direction);
  }

  result = Ddi_BddMakeFromCU (dd, resultCudd);

  return (result);
}


/**Function********************************************************************
  Synopsis    [Find overapproximation based on mutual variable implications]
  Description [Find overapproximation based on mutual variable implications.
               care is the subspace where implications are required to hold.
               eqFileName is file of equivalences where to append newly found
               equivalences. implFileName is file where to add newly found
               functional dependencies as BDDs. Clock is clock state variable:
               both equivalences and functional implications are first
               checked on full state space, then on either clock phase.
               Two variables are equivalent iff var1 <-> var2 or
               var1 <-> !var2 (complemented equivalence).  A functional
               implication is var = f(other vars).]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddImplications (
  Ddi_Bdd_t *f                   /* First Operand */,
  Ddi_Bdd_t *care                /* care set */,
  char *eqFileName               /* file storing equivalent variables */,
  char *implFileName             /* file storing equivalent variables */,
  char *clkStateVarName          /* clock state variable name */
 )
{
  Ddi_Bdd_t *impl, *myF;
  Ddi_Var_t *clkStateVar = NULL;
  Ddi_Varset_t *suppf;
  Ddi_Vararray_t *vars;
  FILE *eqFile=NULL, *implFile=NULL;
  int i, n;
  Ddi_BddMgr *ddm = Ddi_ReadMgr(f);
  Ddi_Bdd_t *impl_i;

  Ddi_MgrSiftSuspend(ddm);

  impl = Ddi_BddMakeConst(ddm,1);
  suppf = Ddi_BddSupp(f);
  vars = Ddi_VararrayMakeFromVarset(suppf,1);
  n = Ddi_VararrayNum(vars);

  /* internal copy of f */
  myF = Ddi_BddDup(f);

  if (clkStateVarName != NULL) {
    clkStateVar = Ddi_VarFromName(ddm,clkStateVarName);
  }
  if (eqFileName != NULL) {
    eqFile = Pdtutil_FileOpen (NULL, eqFileName, "a", NULL);
  }
  if (implFileName != NULL) {
    implFile = Pdtutil_FileOpen (NULL, implFileName, "a", NULL);
  }

  Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "Extracting implications (|f| = %d - part: %d).\n",
      Ddi_BddSize(f), !Ddi_BddIsMono(f))
  );

  for (i=n-1; i>=0; i--) {
#if 0
    Ddi_Bdd_t *lit_i, *lit_j;

    lit_i = Ddi_BddMakeLiteral(Ddi_VararrayRead(vars,i),0);
    for (j=i+1; j<n; j++) {
      Ddi_Bdd_t *impl_ij;
      int impl_00, impl_01, impl_10, impl_11;

      impl_00 = impl_01 = impl_10 = impl_11 = 0;
      lit_j = Ddi_BddMakeLiteral(Ddi_VararrayRead(vars,j),0);

      /* vars[i] => vars[j] */
      impl_ij = Ddi_BddNot(lit_i);
      Ddi_BddOrAcc(impl_ij,lit_j);
      if (Ddi_BddIncluded(myF,impl_ij)) {
        Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelDevMax_c,
        fprintf(stdout, "implication: %s => %s.\n",
             Ddi_VarName(Ddi_VararrayRead(vars,i)),
             Ddi_VarName(Ddi_VararrayRead(vars,j)))
        );
      impl_11 = 1;
      }
      Ddi_Free(impl_ij);

      /* !vars[i] => vars[j] */
      impl_ij = Ddi_BddDup(lit_i);
      Ddi_BddOrAcc(impl_ij,lit_j);
      if (Ddi_BddIncluded(myF,impl_ij)) {
        Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelDevMax_c,
          fprintf(stdout, "implication: !(%s) => %s.\n",
             Ddi_VarName(Ddi_VararrayRead(vars,i)),
             Ddi_VarName(Ddi_VararrayRead(vars,j)))
        );
      impl_01 = 1;
      }
      Ddi_Free(impl_ij);

      Ddi_BddNotAcc(lit_j);

      /* vars[i] => !vars[j] */
      impl_ij = Ddi_BddNot(lit_i);
      Ddi_BddOrAcc(impl_ij,lit_j);
      if (Ddi_BddIncluded(myF,impl_ij)) {
        Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelDevMax_c,
        fprintf(stdout, "implication: %s => !(%s).\n",
             Ddi_VarName(Ddi_VararrayRead(vars,i)),
             Ddi_VarName(Ddi_VararrayRead(vars,j)))
        );
      impl_10 = 1;
      }
      Ddi_Free(impl_ij);

      /* !vars[i] => !vars[j] */
      impl_ij = Ddi_BddDup(lit_i);
      Ddi_BddOrAcc(impl_ij,lit_j);
      if (Ddi_BddIncluded(myF,impl_ij)) {
        Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelDevMax_c,
          fprintf(stdout, "implication: !(%s) => !(%s).\n",
             Ddi_VarName(Ddi_VararrayRead(vars,i)),
             Ddi_VarName(Ddi_VararrayRead(vars,j)))
        );
      impl_00 = 1;
      }
      Ddi_Free(impl_ij);

      Ddi_Free(lit_j);

      if (impl_00 && impl_11) {
      Ddi_Varset_t *remove = Ddi_VarsetMakeFromVar(Ddi_VararrayRead(vars,j));
        Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelDevMin_c,
          fprintf(stdout, "equivalent vars: (%s) == (%s).\n",
            Ddi_VarName(Ddi_VararrayRead(vars,i)),
            Ddi_VarName(Ddi_VararrayRead(vars,j)))
        );
      if (eqFile != NULL) {
        fprintf(eqFile, "%s = %s\n",
             Ddi_VarName(Ddi_VararrayRead(vars,j)),
             Ddi_VarName(Ddi_VararrayRead(vars,i)));
      }
      /* remove var_j from myF */
      Ddi_BddExistAcc(myF,remove);
      Ddi_Free(remove);
      }
      else if (impl_01 && impl_10) {
      Ddi_Varset_t *remove = Ddi_VarsetMakeFromVar(Ddi_VararrayRead(vars,j));
        Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelDevMax_c,
        fprintf(stdout, "equivalent vars (inverted): (%s) != (%s).\n",
             Ddi_VarName(Ddi_VararrayRead(vars,i)),
             Ddi_VarName(Ddi_VararrayRead(vars,j)))
        );
      if (eqFile != NULL) {
        fprintf(eqFile, "%s = !%s\n",
             Ddi_VarName(Ddi_VararrayRead(vars,j)),
             Ddi_VarName(Ddi_VararrayRead(vars,i)));
      }
      /* remove var_j from myF */
      Ddi_BddExistAcc(myF,remove);
      Ddi_Free(remove);
      }

    }

    Ddi_Free(lit_i);

#endif

#if 1

    impl_i = DdiBddVarFunctionalDep (myF,Ddi_VararrayRead(vars,i));
    if (impl_i != NULL) {
      if (Ddi_BddSize(impl_i) == 2) {
      int isCompl = Ddi_BddIsComplement(impl_i);
      Ddi_Varset_t *remove = Ddi_VarsetMakeFromVar(Ddi_VararrayRead(vars,i));
      if (eqFile != NULL) {
        fprintf(eqFile, "%s = %s%s\n",
             Ddi_VarName(Ddi_VararrayRead(vars,i)),
             isCompl ? "!" : "",
             Ddi_VarName(Ddi_BddTopVar(impl_i)));
      }
        Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelDevMax_c,
        fprintf(stdout, "equivalent vars%s: (%s) != (%s).\n",
             isCompl ? " (inverted)" : "",
             Ddi_VarName(Ddi_VararrayRead(vars,i)),
             Ddi_VarName(Ddi_BddTopVar(impl_i)))
        );
      /* remove var_i from myF */
      Ddi_BddExistAcc(myF,remove);
      Ddi_Free(remove);
      }

      Ddi_Free(impl_i);
    }
    else if (clkStateVar != NULL) {
      Ddi_Bdd_t *f0 = Ddi_BddCofactor(myF,clkStateVar,0);
      Ddi_Bdd_t *f1 = Ddi_BddCofactor(myF,clkStateVar,1);
      Ddi_Bdd_t *impl0 = DdiBddVarFunctionalDep (f0,Ddi_VararrayRead(vars,i));
      Ddi_Bdd_t *impl1 = DdiBddVarFunctionalDep (f1,Ddi_VararrayRead(vars,i));
      if (impl0 != NULL || impl1 != NULL) {
      if (impl0 == NULL) {
        impl0 = Ddi_BddMakeLiteral(Ddi_VararrayRead(vars,i),1);
      }
      if (impl1 == NULL) {
        impl1 = Ddi_BddMakeLiteral(Ddi_VararrayRead(vars,i),1);
      }

        if ((Ddi_BddSize(impl0) == 2)&&(Ddi_BddSize(impl1) == 2)) {
        int compl0 = Ddi_BddIsComplement(impl0);
        int compl1 = Ddi_BddIsComplement(impl1);
         Ddi_Varset_t *remove =
            Ddi_VarsetMakeFromVar(Ddi_VararrayRead(vars,i));
        if (eqFile != NULL) {
          fprintf(eqFile, "%s = %s ? %s%s : %s%s\n",
             Ddi_VarName(Ddi_VararrayRead(vars,i)),
             clkStateVarName,
             compl1 ? "!" : "",
             Ddi_VarName(Ddi_BddTopVar(impl1)),
             compl0 ? "!" : "",
             Ddi_VarName(Ddi_BddTopVar(impl0)));
          }
          /* remove var_i from myF */
          Ddi_BddExistAcc(myF,remove);
        Ddi_Free(remove);
      }

        Ddi_Free(impl0);
        Ddi_Free(impl1);
      }

      Ddi_Free(f0);
      Ddi_Free(f1);
    }

#endif

  }

  Ddi_Free(myF);
  Ddi_Free(vars);
  Ddi_Free(suppf);

  if (eqFile != NULL) {
    Pdtutil_FileClose(eqFile,NULL);
  }
  if (implFile != NULL) {
    Pdtutil_FileClose(implFile,NULL);
  }

  Ddi_MgrSiftResume(ddm);

  return (impl);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddOverapprConj (
  Ddi_Bdd_t *f,
  Ddi_Bdd_t *care,
  int nPart,
  int th
)
{
  Ddi_Varset_t *smooth0, *smooth1, *supp;
  Ddi_Bdd_t *r, *r0, *r1;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  int i;

  Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Over-Apprimation Conj: %d -> ", Ddi_BddSize(f))
  );

  r = Ddi_BddDup(f);

  if (nPart > 1)
  {
    int l, n;
    supp = Ddi_BddSupp(f);
    n = Ddi_VarsetNum(supp);

    for (i=0; i<nPart; i++) {

      if (Ddi_BddSize(r) < th)
        break;

      smooth1 = Ddi_VarsetVoid(ddm);

      for (l=0, Ddi_VarsetWalkStart(supp); !Ddi_VarsetWalkEnd(supp);
          Ddi_VarsetWalkStep(supp)) {
          if ((l < (i*n)/nPart) || (l >= ((i+1)*n)/nPart)){
            Ddi_VarsetAddAcc(smooth1,Ddi_VarsetWalkCurr(supp));
        }
          l++;
      }
      smooth0 = Ddi_VarsetDiff(supp,smooth1);

      r0 = Ddi_BddExistOverApprox(r,smooth1);
      r1 = Ddi_BddExistOverApprox(r,smooth0);
      Ddi_Free(smooth0);
      Ddi_Free(smooth1);

      if (Ddi_BddIsOne(r0)&&Ddi_BddIsOne(r1)) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "@")
        );
      }
      else {
        Ddi_Free(r);
        r = Ddi_BddAnd(r0,r1);
        if (care != NULL)
          Ddi_BddRestrictAcc(r,care);
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "#")
        );
      }
      fflush(stdout);
      Ddi_Free(r0);
      Ddi_Free(r1);

    }
    Ddi_Free(supp);
  }
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "%d\n", Ddi_BddSize(r))
  );

  return(r);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
Ddi_Varsetarray_t *
Ddi_BddTightSupports (
  Ddi_Bdd_t *f
)
{
  return (DdiBddTightOverapprox(f,0));
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Compute functional dependence of var within function]
  Description [Compute functional dependence of var within function.
               A variable is functionally dependent on other variables
               in right and left cofactors do not overlap.]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
DdiBddVarFunctionalDep (
  Ddi_Bdd_t *f,
  Ddi_Var_t *v
)
{
  Ddi_Bdd_t *cof0 = Ddi_BddCofactor(f,v,0);
  Ddi_Bdd_t *cof1 = Ddi_BddCofactor(f,v,1);
  Ddi_Bdd_t *cof1bar = Ddi_BddNot(cof1);

  if (Ddi_BddIncluded(cof0,cof1bar)) {
    Ddi_Varset_t *supp;
    Ddi_Bdd_t *care = Ddi_BddOr(cof0,cof1);
    Ddi_BddConstrainAcc(cof1,care);
    supp = Ddi_BddSupp(cof1);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "functional dependence found (%s) <- |%d|.\n",
        Ddi_VarName(v),  Ddi_BddSize(cof1))
    );
    Ddi_VarsetPrint(supp, 0, NULL, stdout);
    Ddi_Free(supp);
    Ddi_Free(care);
  }
  else {
    Ddi_Free(cof1);
    cof1 = NULL;
  }
  Ddi_Free(cof0);
  Ddi_Free(cof1bar);
  return(cof1);

}


/**Function********************************************************************
  Synopsis    [Exist with overapproximation]
  Description [Exist with overapproximation]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
DdiBddExistOverApproxAccIntern (
  Ddi_Bdd_t *f,
  Ddi_Varset_t *sm,
  int threshold
)
{
  Ddi_Bdd_t *p, *r;
  int i, nf;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);

  if (Ddi_BddIsMono(f)) {
    Ddi_MgrAbortOnSiftEnableWithThresh(ddm,threshold,1);

    r = Ddi_BddExist(f,sm);
    if (ddm->abortedOp) {
      Ddi_Free(r);
      r = Ddi_BddMakeConst(ddm,1);
    }

    Ddi_MgrAbortOnSiftDisable(ddm);

    DdiGenericDataCopy((Ddi_Generic_t *)f,(Ddi_Generic_t *)r);
    Ddi_Free(r);
  }
  else if (Ddi_BddIsPartDisj(f)) {
    for (i=0; i<Ddi_BddPartNum(f); i++) {
      DdiBddExistOverApproxAccIntern(Ddi_BddPartRead(f,i),sm,threshold);
    }
  }
  else if (Ddi_BddIsPartConj(f)) {
    int saveExistClustTh = ddm->settings.part.existClustThresh;
    int saveExistOptLevel = ddm->settings.part.existOptLevel;
    ddm->settings.part.existClustThresh = threshold;
    ddm->settings.part.existOptLevel = 1;
    Ddi_BddExistAcc(f,sm);
    ddm->settings.part.existClustThresh = saveExistClustTh;
    ddm->settings.part.existOptLevel = saveExistOptLevel;

    Ddi_BddSetPartConj(f);
    nf = Ddi_BddPartNum(f);

    for (i=0; i<nf; i++) {
      p = Ddi_BddPartRead(f,i);
      Ddi_BddExistOverApproxAcc(p,sm);
    }
  }
  else {
    Pdtutil_Warning(1,"DD Format not supported by Exist overapprox - 1 used");
    r = Ddi_MgrReadOne(ddm);
    DdiGenericDataCopy((Ddi_Generic_t *)f,(Ddi_Generic_t *)r);
  }

  return(f);
}



/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Varsetarray_t *
DdiBddTightOverapprox (
  Ddi_Bdd_t *f,
  int thresh
)
{
  Ddi_Bdd_t *r;
  Ddi_Varsetarray_t *partSupp;
  Ddi_Varset_t *smooth, *supp, *core;
  Ddi_Vararray_t *vars;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  int i;
  struct infoStruct {
    int sizeE, sizeF;
    double cntE, cntF, cost;
    int id;
  } *info, infoTmp;

  r = Ddi_BddDup(f);
  supp = Ddi_BddSupp(r);
  vars = Ddi_VararrayMakeFromVarset(supp,1/*sort by order*/);

  partSupp = Ddi_VarsetarrayAlloc(ddm,0);
  core = Ddi_VarsetVoid(ddm);
#if 1
  for (i=Ddi_VararrayNum(vars)-1; i>=0; i--) {
    Ddi_Bdd_t *tmp;
    smooth = Ddi_VarsetMakeFromVar(Ddi_VararrayRead(vars,i));
    tmp = Ddi_BddExist(r,smooth);
    Ddi_Free(smooth);
    if (Ddi_BddIsOne(tmp)) {
      Ddi_VarsetAddAcc(core,Ddi_VararrayRead(vars,i));
    }
    Ddi_Free(tmp);
  }
#endif

  /*  Ddi_VarsetDiffAcc(supp,core);*/
  Ddi_Free(vars);
  vars = Ddi_VararrayMakeFromVarset(supp,1/*sort by order*/);

  info = Pdtutil_Alloc(struct infoStruct,Ddi_VararrayNum(vars));

#if 0
  do {
   double bestCost;
   int bestSize,best_i;

    best_i = -1;
    bestSize = 0;
    bestCost = -1.0;
    for (i=Ddi_VararrayNum(vars)-1; i>=0; i--) {
      Ddi_Bdd_t *tmpE, *tmpF;
      smooth = Ddi_VarsetMakeFromVar(Ddi_VararrayRead(vars,i));
      tmpE = Ddi_BddExist(r,smooth);
      tmpF = Ddi_BddForall(r,smooth);
      Ddi_Free(smooth);
      info[i].sizeE = Ddi_BddSize(tmpE);
      info[i].sizeF = Ddi_BddSize(tmpF);
      info[i].cntE = Ddi_BddCountMinterm(tmpE,Ddi_VararrayNum(vars));
      info[i].cntF = Ddi_BddCountMinterm(tmpF,Ddi_VararrayNum(vars));
      info[i].id = i;
      info[i].cost = info[i].cntF/info[i].cntE;
      if (info[i].cntF/info[i].cntE > bestCost) {
      best_i = i;
      bestSize = Ddi_BddSize(tmpE);
      bestCost = info[i].cntF/info[i].cntE;
      }
      Ddi_Free(tmpE);
      Ddi_Free(tmpF);
    }
#if 0
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "\n")
    );
    for (i=0; i<Ddi_VararrayNum(vars); i++) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "F:%d |E:%d - %.3f=%.0f/%.0f -> %s.\n",
          info[i].sizeF,info[i].sizeE,
          info[i].cntF/info[i].cntE,info[i].cntF,info[i].cntE,
          Ddi_VarName(Ddi_VararrayRead(vars,i)))
     );
    }
#endif
    if (best_i >= 0) {
      smooth = Ddi_VarsetMakeFromVar(Ddi_VararrayRead(vars, best_i));
      Ddi_BddExistAcc(r,smooth);
      Ddi_VarsetarrayInsertLast(partSupp,smooth);
      Ddi_Free(smooth);
      Ddi_VararrayExtract(vars,best_i);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "{%d:%d}", best_i, Ddi_BddSize(r));fflush(stdout)
      );
    }

  } while (Ddi_BddSize(r) > thresh && Ddi_VararrayNum(vars) > 0);
#else

  for (i=Ddi_VararrayNum(vars)-1; i>=0; i--) {
    Ddi_Bdd_t *tmpE, *tmpF;
    smooth = Ddi_VarsetMakeFromVar(Ddi_VararrayRead(vars,i));
    tmpE = Ddi_BddExist(r,smooth);
    tmpF = Ddi_BddForall(r,smooth);
    Ddi_Free(smooth);
    info[i].sizeE = Ddi_BddSize(tmpE);
    info[i].sizeF = Ddi_BddSize(tmpF);
    info[i].cntE = Ddi_BddCountMinterm(tmpE,Ddi_VararrayNum(vars));
    info[i].cntF = Ddi_BddCountMinterm(tmpF,Ddi_VararrayNum(vars));
    info[i].id = i;
    info[i].cost = info[i].cntF/info[i].cntE;
    Ddi_Free(tmpE);
    Ddi_Free(tmpF);
  }
  /* sort by decreasing cost */
  for (i=Ddi_VararrayNum(vars)-1; i>0; i--) {
    int j;
    for (j=0;j<i;j++) {
      if (info[j].cost <= info[i].cost) {
        infoTmp = info[j];
      info[j] = info[i];
      info[i] = infoTmp;
      }
    }
  }
  for (i=0; i<Ddi_VararrayNum(vars); i++) {
    smooth = Ddi_VarsetMakeFromVar(Ddi_VararrayRead(vars, info[i].id));
    Ddi_VarsetarrayInsertLast(partSupp,smooth);
    Ddi_Free(smooth);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "{%d}", info[i].id);fflush(stdout)
    );
  }

#endif

  Pdtutil_Free(info);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "size: %d -> %d.\n", Ddi_BddSize(f), Ddi_BddSize(r));
    fprintf(stdout, "vars: %d(core:%d) ->",
      Ddi_VarsetNum(supp)+Ddi_VarsetNum(core), Ddi_VarsetNum(core))
  );
  Ddi_Free(supp);
  supp = Ddi_BddSupp(r);
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "%d %d.\n", Ddi_VarsetNum(supp), i)
  );

  Ddi_VarsetarrayInsertLast(partSupp,core);
  Ddi_Free(core);
  Ddi_Free(supp);
  Ddi_Free(vars);

  Ddi_Free(r);

  return(partSupp);
}

void searchPartBddVar (Ddi_Bdd_t *f, Ddi_Var_t *v) {
  if (v==NULL)  {
    printf("no var\n");
    return;
  }
  for (int i=0; i<Ddi_BddPartNum(f); i++) {
    Ddi_Varset_t *s_i = Ddi_BddSupp(Ddi_BddPartRead(f,i));
    int ret = Ddi_VarInVarset(s_i,v);
    Ddi_Free(s_i);
    if (ret) {
      printf("%d ", i);
    }
  }
  printf("\n");
}

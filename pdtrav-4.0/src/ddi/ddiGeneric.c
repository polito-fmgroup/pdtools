/**CFile***********************************************************************

  FileName    [ddiGeneric.c]

  PackageName [ddi]

  Synopsis    [Functions working on generic DDI type Ddi_Generic_t]

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

#define DDI_VARSET_DBG 1

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

static void GenericFreeIntern(Ddi_Generic_t *f);
static Ddi_Generic_t * GenericDupIntern(Ddi_Generic_t *r, Ddi_Generic_t *f);
static Ddi_Generic_t * ArraySupp(Ddi_ArrayData_t *array);
static Ddi_Generic_t * ArraySupp2(Ddi_Generic_t *f);
static void ArrayOpIterate(Ddi_OpCode_e opcode, Ddi_ArrayData_t *array, Ddi_Generic_t *g, Ddi_Generic_t *h);
static Ddi_ArrayData_t * GenBddRoots(Ddi_Generic_t *f);
static void GenBddRootsRecur(Ddi_Generic_t *f, Ddi_ArrayData_t *roots);
static void SupportWithAuxArrayCU(DdNode ** nodeArray, int n, char *auxArray);
static int myDdDagIntForSupport(DdNode * n, char *auxArray);
static void myClearFlag(DdNode * f);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of external functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Compute generic operation. Result generated]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Generic_t *
Ddi_GenericOp (
  Ddi_OpCode_e   opcode   /* operation code */,
  Ddi_Generic_t  *f       /* first operand */,
  Ddi_Generic_t  *g       /* first operand */,
  Ddi_Generic_t  *h       /* first operand */
)
{
  return(DdiGenericOp(opcode,Ddi_Generate_c,f,g,h));
}

/**Function********************************************************************
  Synopsis    [Compute generic operation. Result accumulated]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Generic_t *
Ddi_GenericOpAcc (
  Ddi_OpCode_e   opcode   /* operation code */,
  Ddi_Generic_t  *f       /* first operand */,
  Ddi_Generic_t  *g       /* first operand */,
  Ddi_Generic_t  *h       /* first operand */
)
{
  return(DdiGenericOp(opcode,Ddi_Accumulate_c,f,g,h));
}

/**Function********************************************************************
  Synopsis    [Generic dup]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Generic_t *
Ddi_GenericDup (
  Ddi_Generic_t  *f
)
{
  return(DdiGenericDup(f));
}

/**Function********************************************************************
  Synopsis    [Lock DDI node.]
  Description [Lock DDI node so that cannot be freed unless unlocked.
    Used as a protection mechanism for internal objects (array entries,
    partitions, ...]
  SideEffects []
  SeeAlso     [Ddi_GenericUnlock]
******************************************************************************/
void
Ddi_GenericLock (
  Ddi_Generic_t *f
)
{
  if ((f!=NULL)/*&&(DdiType(f)!=Ddi_Var_c)*/) {
    if (f->common.status==Ddi_Locked_c) {
      /*
       * only variables are may be locked several times as they are not
       * duplicated
       */
      Pdtutil_Assert(DdiType(f)==Ddi_Var_c,
        "Unlocked node expected");
    }
    else {
      f->common.mgr->lockedNum++;
      f->common.mgr->typeLockedNum[DdiType(f)]++;
      f->common.status = Ddi_Locked_c;
    }
  }
}

/**Function********************************************************************
  Synopsis    [Unlock DDI node.]
  Description [Unlock DDI node so that can be freed again.]
  SideEffects []
  SeeAlso     [Ddi_GenericLock]
******************************************************************************/
void
Ddi_GenericUnlock (
  Ddi_Generic_t *f
)
{
  if ((f!=NULL)/*&&(DdiType(f)!=Ddi_Var_c)*/) {
    Pdtutil_Assert(f->common.status==Ddi_Locked_c,"Locked node expected");
    f->common.mgr->lockedNum--;
    f->common.mgr->typeLockedNum[DdiType(f)]--;
    f->common.status = Ddi_Unlocked_c;
  }
}

/**Function********************************************************************
  Synopsis     [Free the content of a generic DDI node]
  Description  [Free the content of a generic DDI node]
  SideEffects  []
  SeeAlso      []
******************************************************************************/

void
Ddi_GenericFree (
  Ddi_Generic_t *f    /* block to be freed */
)
{
  DdiGenericFree(f);
}

/**Function********************************************************************
  Synopsis     [Set name field of DDI node]
  Description  [Set name field of DDI node]
  SideEffects  []
  SeeAlso      []
******************************************************************************/
void
Ddi_GenericSetName (
  Ddi_Generic_t *f    /* block to be freed */,
  char *name
)
{
  if (f->common.name != NULL) {
    Pdtutil_Free(f->common.name);
  }
  f->common.name = Pdtutil_StrDup(name);
}


/*
 *  Read fields and components
 */

/**Function********************************************************************
  Synopsis    [called through Ddi_ReadCode.]
  SideEffects [Ddi_ReadCode]
******************************************************************************/
Ddi_Code_e
Ddi_GenericReadCode(
  Ddi_Generic_t *f
)
{
  Pdtutil_Assert (f!=NULL, "Accessing NULL DDI node");
  return (f->common.code);
}

/**Function********************************************************************
  Synopsis    [called through Ddi_ReadMgr.]
  SideEffects [Ddi_ReadMgr]
******************************************************************************/
Ddi_Mgr_t *
Ddi_GenericReadMgr (
  Ddi_Generic_t * f
)
{

  Pdtutil_Assert (f!=NULL, "Accessing NULL DDI node");
  return (f->common.mgr);
}

/**Function********************************************************************
  Synopsis    [called through Ddi_ReadName.]
  SideEffects [Ddi_ReadName]
******************************************************************************/
char *
Ddi_GenericReadName (
  Ddi_Generic_t * f
)
{
  Pdtutil_Assert (f!=NULL, "Accessing NULL DDI node");
  return (f->common.name);
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************
  Synopsis    [Allocate and initialize a new DDI block]
  Description [Allocation and initialization of generic DDI block.
               owner DDI manager always required.]
  SideEffects [none]
  SeeAlso     [DdiGenericNew DdiGenericFree]
******************************************************************************/

Ddi_Generic_t *
DdiGenericAlloc (
  Ddi_Type_e type           /* type of allocated object */,
  Ddi_Mgr_t *mgr            /* DDI manager */
)
{
  Ddi_Generic_t *r;
  static int looking = 0;

  Pdtutil_Assert (mgr!=NULL, "non NULL DDI manager required for node alloc");

  /*
   *  fields common to all node types
   */

  r = DdiMgrNodeAlloc (mgr);

  r->common.type = type;
  r->common.code = Ddi_Null_code_c;
  r->common.status = Ddi_Unlocked_c;
  r->common.name = NULL;
  r->common.mgr = mgr;
  r->common.info = NULL;
  r->common.supp = NULL;
  r->common.size = 0;
  r->common.nodeid = mgr->currNodeId++;
  if (looking>0 && r->common.nodeid==looking) {
    printf("found\n");
  }
  r->common.next = mgr->nodeList;
  mgr->allocatedNum++;
  mgr->genericNum++;
  mgr->typeNum[type]++;
  mgr->nodeList = (Ddi_Generic_t *) r;

  switch (type) {

  case Ddi_Bdd_c:
    r->Bdd.data.bdd = NULL;
    break;
  case Ddi_Var_c:
    r->Var.data.p.bdd = NULL;
    r->Var.data.index = -1;
    r->common.info = Pdtutil_Alloc(Ddi_Info_t, 1);
    r->common.info->infoCode = Ddi_Info_Var_c;
    r->common.info->var.activity = 0.0;
    r->common.info->var.mark = 0;
    break;
  case Ddi_Varset_c:
    r->Varset.data.cu.bdd = NULL;
    r->Varset.data.cu.curr = NULL;
    break;
  case Ddi_Expr_c:
    r->Expr.data.bdd = NULL;
    r->Expr.opcode = 0;
    break;
  case Ddi_Bddarray_c:
    r->Bddarray.array = NULL;
    break;
  case Ddi_Vararray_c:
    r->Vararray.array = NULL;
    break;
  case Ddi_Varsetarray_c:
    r->Varsetarray.array = NULL;
    break;
  case Ddi_Exprarray_c:
    r->Exprarray.array = NULL;
    break;
  default:
    Pdtutil_Assert (0, "Wrong DDI node type");
  }

  if ((mgr->tracedId>=0)&&(r->common.nodeid >= mgr->tracedId)) {
    DdiTraceNodeAlloc(r);
  }


  return(r);

}

/**Function*******************************************************************
  Synopsis    [Trace allocation of DDI node]
  Description [Trace allocation of DDI node. Put a debugger breakpoint on this
               routine to watch allocation of a node, given its ID. The id is
               retrieved from a previous run (e.g. logged as non freed DDI
               node in MgrCheck by Ddi_MgrPrintExtRef.]
  SideEffects [none]
  SeeAlso     [Ddi_MgrSetTracedId Ddi_MgrPrintExtRef]
******************************************************************************/

void
DdiTraceNodeAlloc (
  Ddi_Generic_t *r
)
{
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "DDI Node with ID=%d Creatd Now.\n",
      r->common.nodeid)
  );
}

/**Function********************************************************************

  Synopsis     [Free the content of a generic DDI node]
  Description  [Free the content of a generic DDI node and activates DDI
    manager garbage collection (of DDI handles) if required. ]
  Description  [Frees a generic block.]
  SideEffects  [none]
  SeeAlso      []

******************************************************************************/

void
DdiGenericFree (
  Ddi_Generic_t *f    /* block to be freed */
)
{
  Ddi_Mgr_t *ddiMgr;  /* dd manager */

  if (f == NULL) {
     /* this may happen with NULL entries in freed arrays */
    return;
  }

  ddiMgr = f->common.mgr;

  if (f->common.name != NULL) {
    Pdtutil_Free(f->common.name);
  }

  GenericFreeIntern(f);

  if (f->common.info != NULL) {
    if (f->common.info->infoCode == Ddi_Info_Eq_c) {
      Ddi_Unlock(f->common.info->eq.vars);
      Ddi_Free(f->common.info->eq.vars);
      Ddi_Unlock(f->common.info->eq.subst);
      Ddi_Free(f->common.info->eq.subst);
    }
    else if (f->common.info->infoCode == Ddi_Info_Compose_c) {
      Ddi_Unlock(f->common.info->compose.f);
      Ddi_Free(f->common.info->compose.f);
      Ddi_Unlock(f->common.info->compose.care);
      Ddi_Free(f->common.info->compose.care);
      Ddi_Unlock(f->common.info->compose.constr);
      Ddi_Free(f->common.info->compose.constr);
      Ddi_Unlock(f->common.info->compose.cone);
      Ddi_Free(f->common.info->compose.cone);
      Ddi_Unlock(f->common.info->compose.refVars);
      Ddi_Free(f->common.info->compose.refVars);
      Ddi_Unlock(f->common.info->compose.vars);
      Ddi_Free(f->common.info->compose.vars);
      Ddi_Unlock(f->common.info->compose.subst);
      Ddi_Free(f->common.info->compose.subst);
    }
    Pdtutil_Free(f->common.info);
  }
  if (ddiMgr->freeNum > DDI_GARBAGE_THRESHOLD) {
    DdiMgrGarbageCollect(ddiMgr);
  }

}

/**Function********************************************************************

  Synopsis     [Schedule free of a generic block]

  Description  [Schedule free a generic block.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

Ddi_Generic_t *
DdiDeferredFree (
  Ddi_Generic_t *f    /* block to be freed */
)
{

  Pdtutil_Assert (f!=NULL, "deferred free of NULL DDI node");
  f->common.status = Ddi_SchedFree_c;
  return(f);
}

/**Function********************************************************************
  Synopsis     [Duplicate a DDI node]
  Description  []
  SideEffects  []
  SeeAlso      []
******************************************************************************/
Ddi_Generic_t *
DdiGenericDup (
  Ddi_Generic_t *f    /* BDD to be duplicated */
  )
{
  Ddi_Generic_t *r;
  Ddi_Mgr_t *ddiMgr;

  if (f == NULL) {
    return (NULL);
  }

  if (f->common.type == Ddi_Var_c) {
    return (f);
  }

  ddiMgr = f->common.mgr;
  r = DdiGenericAlloc (f->common.type, ddiMgr);

  GenericDupIntern(r,f);

  return(r);
}

/**Function********************************************************************
  Synopsis     [Copy a DDI node to a destination manager]
  Description  [Copy a DDI node to a destination manager]
  SideEffects  []
  SeeAlso      []
******************************************************************************/
Ddi_Generic_t *
DdiGenericCopy (
  Ddi_Mgr_t *ddiMgr           /* destination manager */,
  Ddi_Generic_t *f         /* BDD to be duplicated */,
  Ddi_Vararray_t *varsOld  /* old variable array */,
  Ddi_Vararray_t *varsNew  /* new variable array */
)
{
  Ddi_Generic_t *r;
  Ddi_Mgr_t *ddiMgr0;

  if (f == NULL) {
    return (NULL);
  }

  ddiMgr0 = f->common.mgr;

  if (varsOld != NULL) {
    Ddi_Vararray_t *varsOld2;
    int nv,i;
    Ddi_Var_t *vOld;

    Pdtutil_Assert(varsNew != NULL,"Missing variable array for var remap");
    nv = Ddi_VararrayNum(varsOld);
    Pdtutil_Assert(nv == Ddi_VararrayNum(varsNew),
      "Var array sizes do not match");

    varsOld2 = Ddi_VararrayAlloc(ddiMgr,nv);
    for (i=0; i<nv; i++) {
      vOld = Ddi_VararrayRead(varsOld,i);
      Ddi_VararrayWrite(varsOld2,i,Ddi_VarFromIndex(ddiMgr,Ddi_VarIndex(vOld)));
    }
    r = DdiGenericCopy(ddiMgr,f,NULL,NULL);
    Ddi_BddSubstVarsAcc ((Ddi_Bdd_t *)r,varsOld2,varsNew);
    return(r);
  }

  /* Same manager: call dup */
  if (ddiMgr == ddiMgr0) {
    return (DdiGenericDup(f));
  }

  r = DdiGenericAlloc (f->common.type, ddiMgr);
  r->common.type = f->common.type;
  r->common.code = f->common.code;
  r->common.status = Ddi_Unlocked_c;
  r->common.name = Pdtutil_StrDup(f->common.name);
  r->common.supp = NULL;

  switch (f->common.type) {

  case Ddi_Bdd_c:
    switch (f->common.code) {
    case Ddi_Bdd_Mono_c:
      r->Bdd.data.bdd = Cudd_bddTransfer(ddiMgr0->mgrCU,ddiMgr->mgrCU,
        f->Bdd.data.bdd);
      Cudd_Ref(r->Bdd.data.bdd);
      break;
    case Ddi_Bdd_Part_Conj_c:
    case Ddi_Bdd_Part_Disj_c:
      if (DdiArrayNum(f->Bdd.data.part)>0 && 
	Ddi_BddIsAig((Ddi_Bdd_t *)DdiArrayRead(
                                        f->Bdd.data.part,0))) {
        r->Bdd.data.part =
          DdiAigArrayCopy(ddiMgr0,ddiMgr,f->Bdd.data.part);
      }
      else 
        r->Bdd.data.part = DdiArrayCopy(ddiMgr,f->Bdd.data.part);
      break;
    case Ddi_Bdd_Meta_c:
        fprintf(stderr, "Error: DdiMetaCopy Not Implemented.\n");
      /*
        r->Bdd.data.meta = DdiMetaCopy(ddiMgr,f->Bdd.data.meta);
      */
      break;
    case Ddi_Bdd_Aig_c:
      r->Bdd.data.aig = DdiAigCopy (ddiMgr0,ddiMgr,f->Bdd.data.aig);
      break;
    default:
      Pdtutil_Assert(0,"Wrong DDI node code");
      break;
    }
    break;
  case Ddi_Var_c:
    DdiGenericFree(r->common.supp);
    r->Var.data.p.bdd = NULL;
    DdiGenericFree(r); /*@@@@*/
    if (f->common.code == Ddi_Var_Bdd_c) {
      r = (Ddi_Generic_t *)Ddi_VarFromCuddIndex(ddiMgr,
           Ddi_VarCuddIndex((Ddi_Var_t *)f));
      if (Ddi_VarName((Ddi_Var_t *)r)==NULL) {
	char *name = Ddi_VarName((Ddi_Var_t *)f);
	Ddi_VarAttachName((Ddi_Var_t *)r,name);
      }
    }
    else if (f->common.code == Ddi_Var_Aig_c) {
      char *name = Ddi_VarName((Ddi_Var_t *)f);
      r = (Ddi_Generic_t *)Ddi_VarFromName(ddiMgr,name);
      if (r==NULL) {
	r = (Ddi_Generic_t *)Ddi_VarNewBaig(ddiMgr,name);
      }
    }

    break;
  case Ddi_Varset_c:
    if (f->common.code==Ddi_Varset_Bdd_c) {
      r->Varset.data.cu.bdd =
	Cudd_bddTransfer(ddiMgr0->mgrCU,ddiMgr->mgrCU,f->Varset.data.cu.bdd);
      Cudd_Ref(r->Varset.data.cu.bdd);
    }
    else if (f->common.code==Ddi_Varset_Array_c) {
      r->Varset.data.array.va = 
	Ddi_VararrayCopy(ddiMgr,f->Varset.data.array.va);
      Ddi_Lock(r->Varset.data.array.va);
    }
    break;
  case Ddi_Expr_c:
    switch (f->common.code) {
    case Ddi_Expr_Bdd_c:
      r->Expr.data.bdd = DdiGenericCopy(ddiMgr,f->Expr.data.bdd,NULL,NULL);
      break;
    case Ddi_Expr_String_c:
      r->Expr.data.string = Pdtutil_StrDup(f->Expr.data.string);
      break;
    case Ddi_Expr_Bool_c:
    case Ddi_Expr_Ctl_c:
      r->Expr.data.sub = DdiArrayCopy(ddiMgr,f->Expr.data.sub);
      break;
    default:
      Pdtutil_Assert(0,"Wrong DDI node code");
      break;
    }
    r->Expr.opcode = f->Expr.opcode;
    break;
  case Ddi_Bddarray_c:
    if (DdiArrayNum(f->Bddarray.array)>0 && 
	Ddi_BddIsAig((Ddi_Bdd_t *)DdiArrayRead(f->Bddarray.array,0))) {
      r->Bddarray.array = DdiAigArrayCopy(ddiMgr0,ddiMgr,f->Bddarray.array);
      break;
    }
  case Ddi_Vararray_c:
  case Ddi_Varsetarray_c:
  case Ddi_Exprarray_c:
    r->Bddarray.array = DdiArrayCopy(ddiMgr,f->Bddarray.array);
    break;
  default:
    Pdtutil_Assert (0, "Wrong DDI node type");
  }

  return(r);
}


/**Function********************************************************************
  Synopsis     [Copy the content of a DDI node to another one]
  Description  [Copy the content of a DDI node to another one.
    Freing is done before copy. Data are duplicated]
  SideEffects  []
  SeeAlso      []
******************************************************************************/

void
DdiGenericDataCopy (
  Ddi_Generic_t *d    /* destination */,
  Ddi_Generic_t *s    /* source */
)
{
  Ddi_Mgr_t *ddiMgr;  /* dd manager */
  int locked=0;

  ddiMgr = s->common.mgr;

  if (d->common.status==Ddi_Locked_c) {
    locked = 1;
    Ddi_Unlock(d);
  }

  GenericFreeIntern(d);

  /* resume block */
  ddiMgr->genericNum++;
  ddiMgr->typeNum[d->common.type]++;
  ddiMgr->freeNum--;
  d->common.status = Ddi_Unlocked_c;

  GenericDupIntern(d,s);


  if (locked) {
    Ddi_Lock(d);
  }

}

/**Function********************************************************************
  Synopsis    [Compute operation]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Generic_t *
DdiGenericOp (
  Ddi_OpCode_e   opcode   /* operation code */,
  Ddi_OpType_e   optype   /* operation type (accumulate/generate) */,
  Ddi_Generic_t  *f       /* first operand */,
  Ddi_Generic_t  *g       /* second operand */,
  Ddi_Generic_t  *h       /* third operand */
)
{
  int i;
  DdNode *rCU = NULL;                 /* CUDD result */
  Ddi_Generic_t *r=NULL;              /* result */
  Ddi_Generic_t *tmp;
  Ddi_BddMgr *ddiMgr;                      /* dd manager */
  DdManager *mgrCU;                    /* CUDD manager */
  int attachSupport = 0;

  Pdtutil_Assert (f!=NULL, "NULL first operand of DDI operation");

  ddiMgr = f->common.mgr;
  mgrCU = Ddi_MgrReadMgrCU(ddiMgr);

  switch (optype) {
  case Ddi_Accumulate_c:
    r = f;
    if (r->common.supp != NULL) {
      /* support must be recomputed after operation */
      attachSupport = 1;
    }
    if ((g!=NULL)&&(g->common.code==Ddi_Bdd_Meta_c)
      &&(r->common.code==Ddi_Bdd_Mono_c)) {
      tmp = DdiGenericOp(opcode,Ddi_Generate_c,g,r,h);
      DdiGenericDataCopy(r,tmp);
      DdiGenericFree(tmp);
      return(r);
    }
    break;
  case Ddi_Generate_c:
    /* swap f and g if g is meta */
    if ((g!=NULL)&&(g->common.code==Ddi_Bdd_Meta_c)
      &&(f->common.code==Ddi_Bdd_Mono_c)&&
        (opcode==Ddi_BddAndExist_c || opcode==Ddi_BddAnd_c ||
         opcode==Ddi_BddOr_c)) {
      tmp = g; g = f; f = tmp;
    }
    r = DdiGenericDup(f);
    Ddi_Unlock(r->common.supp);
    Ddi_Free(r->common.supp);
    break;
  default:
    Pdtutil_Assert (0,
      "Wrong DDI operation type (accumulate/generate accepted)");
  }

  /*
   * DdArray operations are iterated on all array items
   */
  if ((DdiType(f)==Ddi_Bddarray_c) &&
      (opcode != Ddi_BddSupp_c)) {
    ArrayOpIterate(opcode,r->Bddarray.array,g,h);
    return(r);
  }

  switch (opcode) {

  /*
   *  Boolean operators
   */

  case Ddi_BddNot_c:
    DdiConsistencyCheck(r,Ddi_Bdd_c);
    switch (r->common.code) {
    case Ddi_Bdd_Mono_c:
      /* no ref/deref required */
      r->Bdd.data.bdd = Cudd_Not(r->Bdd.data.bdd);
      break;
    case Ddi_Bdd_Part_Conj_c:
      r->common.code = Ddi_Bdd_Part_Disj_c;
      ArrayOpIterate(opcode,r->Bdd.data.part,NULL,NULL);
      break;
    case Ddi_Bdd_Part_Disj_c:
      r->common.code = Ddi_Bdd_Part_Conj_c;
      ArrayOpIterate(opcode,r->Bdd.data.part,NULL,NULL);
      break;
    case Ddi_Bdd_Meta_c:
      DdiMetaDoCompl(r->Bdd.data.meta);
      break;
    case Ddi_Bdd_Aig_c:
      {
        Ddi_Aig_t *a = DdiAigNot(r->Bdd.data.aig);
	DdiAigFree(r->Bdd.data.aig);
        r->Bdd.data.aig = a;
      }
      break;
    default:
      Pdtutil_Assert(0,"Wrong DDI node code");
      break;
    }

    break;

  case Ddi_BddAnd_c:
    DdiConsistencyCheck(r,Ddi_Bdd_c);
    DdiConsistencyCheck(g,Ddi_Bdd_c);
    switch (r->common.code) {
    case Ddi_Bdd_Mono_c:
      if (g->common.code==Ddi_Bdd_Part_Conj_c) {
        Ddi_ArrayData_t *a = g->Bdd.data.part;
        Ddi_Generic_t *p;
        for (i=0; i<DdiArrayNum(a);i++) {
          p = DdiArrayRead(a,i);
          (void *)DdiGenericOp(opcode,Ddi_Accumulate_c,r,p,NULL);
      }
      }
      else if (g->common.code==Ddi_Bdd_Part_Disj_c) {
        Ddi_ArrayData_t *a = g->Bdd.data.part;
        Ddi_Generic_t *p, *pa, *raux;
        raux = DdiGenericDup((Ddi_Generic_t *)Ddi_MgrReadZero(ddiMgr));
        for (i=0; i<DdiArrayNum(a);i++) {
          p = DdiArrayRead(a,i);
          pa = DdiGenericOp(opcode,Ddi_Generate_c,r,p,NULL);
          (void *)DdiGenericOp(Ddi_BddOr_c,Ddi_Accumulate_c,raux,pa,NULL);
          Ddi_Free(pa);
      }
        DdiGenericDataCopy(r,raux);
        Ddi_Free(raux);
      }
      else {



        rCU = Cudd_bddAnd(mgrCU,r->Bdd.data.bdd,g->Bdd.data.bdd);
        if (rCU != NULL) {
          Cudd_Ref(rCU);
          Cudd_RecursiveDeref (mgrCU, r->Bdd.data.bdd);
          r->Bdd.data.bdd = rCU;
        }
        else {
          Pdtutil_Assert(ddiMgr->abortOnSift,
            "NULL result only allowed with abortOnSift set");
          ddiMgr->abortedOp = 1;
        }
      }
      break;
    case Ddi_Bdd_Part_Conj_c:
      if (g->common.code!=Ddi_Bdd_Part_Conj_c) {
        /* AND is applied to first partition */
        (void *)DdiGenericOp(opcode,Ddi_Accumulate_c,
          DdiArrayRead(r->Bdd.data.part,0),g,NULL);
      }
      else {
        /*
         * partitions sharing support vars with g are moved at first positions
         */
        Ddi_ArrayData_t *a = r->Bdd.data.part;
        Ddi_ArrayData_t *b = g->Bdd.data.part;
        Ddi_Generic_t *p;
#if 1
        for (i=0; i<DdiArrayNum(b); i++) {
          int j = 2*i+1;
          p = DdiArrayRead(b,i);
        if (j>=DdiArrayNum(a)) {
          j = DdiArrayNum(a);
        }
          DdiArrayInsert(a,j,p,Ddi_Dup_c);
      }
        Ddi_BddSetMono((Ddi_Bdd_t *)r);
#else
        Ddi_Varset_t *suppg, *suppp, *common;
        suppg = Ddi_BddSupp((Ddi_Bdd_t *)g);
        for (i=DdiArrayNum(a)-1;i>=0;i--) {
          suppp = Ddi_BddSupp((Ddi_Bdd_t *)DdiArrayRead(a,i));
          common = Ddi_VarsetIntersect(suppg,suppp);
          if (!Ddi_VarsetIsVoid(common)) {
            p = DdiArrayExtract(a,i);
            DdiArrayInsert(a,0,p,Ddi_Mov_c);
        }
          Ddi_Free(suppp);
          Ddi_Free(common);
      }
        Ddi_Free(suppg);
#endif
      }
      break;
    case Ddi_Bdd_Part_Disj_c:
      /* AND is distributed over disjunction */
      ArrayOpIterate(opcode,r->Bdd.data.part,g,NULL);
      break;
    case Ddi_Bdd_Meta_c:
      DdiMetaAndAcc((Ddi_Bdd_t *)r,(Ddi_Bdd_t *)g);
      break;
    case Ddi_Bdd_Aig_c:
      Pdtutil_Assert(g->common.code == Ddi_Bdd_Aig_c, "AIG required");
      {
        Ddi_Aig_t *a = DdiAigAnd(r->Bdd.data.aig,g->Bdd.data.aig);
	DdiAigFree(r->Bdd.data.aig);
        r->Bdd.data.aig = a;
      }
      break;
    default:
      Pdtutil_Assert(0,"Wrong DDI node code");
      break;
    }
    break;

  case Ddi_BddDiff_c:
    (void *)DdiGenericOp(Ddi_BddNot_c,Ddi_Accumulate_c,g,NULL,NULL);
    (void *)DdiGenericOp(Ddi_BddAnd_c,Ddi_Accumulate_c,r,g,NULL);
    (void *)DdiGenericOp(Ddi_BddNot_c,Ddi_Accumulate_c,g,NULL,NULL);
    break;

  case Ddi_BddOr_c:
    /* bring to And operator using DeMorgan transformation */
    (void *)DdiGenericOp(Ddi_BddNot_c,Ddi_Accumulate_c,r,NULL,NULL);
    (void *)DdiGenericOp(Ddi_BddNot_c,Ddi_Accumulate_c,g,NULL,NULL);

    (void *)DdiGenericOp(Ddi_BddAnd_c,Ddi_Accumulate_c,r,g,NULL);

    (void *)DdiGenericOp(Ddi_BddNot_c,Ddi_Accumulate_c,r,NULL,NULL);
    (void *)DdiGenericOp(Ddi_BddNot_c,Ddi_Accumulate_c,g,NULL,NULL);
    break;

  case Ddi_BddXor_c:
    DdiConsistencyCheck(r,Ddi_Bdd_c);
    DdiConsistencyCheck(g,Ddi_Bdd_c);
    switch (r->common.code) {
    case Ddi_Bdd_Mono_c:
      Pdtutil_Assert(!ddiMgr->abortOnSift,
                 "Abort on sift not supported by this operator");
      rCU = Cudd_bddXor(mgrCU,r->Bdd.data.bdd,g->Bdd.data.bdd);
      Cudd_Ref(rCU);
      Cudd_RecursiveDeref (mgrCU, r->Bdd.data.bdd);
      r->Bdd.data.bdd = rCU;
      break;
    case Ddi_Bdd_Aig_c:
      {
        Ddi_Aig_t *a = DdiAigXor(r->Bdd.data.aig,g->Bdd.data.aig);
      DdiAigFree(r->Bdd.data.aig);
        r->Bdd.data.aig = a;
      }
      break;
    case Ddi_Bdd_Part_Conj_c:
    case Ddi_Bdd_Part_Disj_c:
    case Ddi_Bdd_Meta_c:
      /* XOR not allowed with partitioned and meta BDDs */
      Pdtutil_Warning (1,
        "XOR not supported with partitioned/meta BDDs\nTransformed to mono");
     (void *)DdiGenericOp(Ddi_BddMakeMono_c,Ddi_Accumulate_c,r,NULL,NULL);
     (void *)DdiGenericOp(opcode,Ddi_Accumulate_c,r,g,NULL);
      break;
    default:
      Pdtutil_Assert(0,"Wrong DDI node code");
      break;
    }
    break;

  case Ddi_BddNand_c:
    (void *)DdiGenericOp(Ddi_BddAnd_c,Ddi_Accumulate_c,r,g,NULL);
    (void *)DdiGenericOp(Ddi_BddNot_c,Ddi_Accumulate_c,r,NULL,NULL);
    break;

  case Ddi_BddNor_c:
    (void *)DdiGenericOp(Ddi_BddOr_c,Ddi_Accumulate_c,r,g,NULL);
    (void *)DdiGenericOp(Ddi_BddNot_c,Ddi_Accumulate_c,r,NULL,NULL);
    break;

  case Ddi_BddXnor_c:
    (void *)DdiGenericOp(Ddi_BddXor_c,Ddi_Accumulate_c,r,g,NULL);
    (void *)DdiGenericOp(Ddi_BddNot_c,Ddi_Accumulate_c,r,NULL,NULL);
    break;

  case Ddi_BddIte_c:
    DdiConsistencyCheck(r,Ddi_Bdd_c);
    DdiConsistencyCheck(g,Ddi_Bdd_c);
    DdiConsistencyCheck(h,Ddi_Bdd_c);
    switch (r->common.code) {
    case Ddi_Bdd_Mono_c:
      rCU = Cudd_bddIte(mgrCU,r->Bdd.data.bdd,g->Bdd.data.bdd,h->Bdd.data.bdd);
      if (rCU != NULL) {
        Cudd_Ref(rCU);
        Cudd_RecursiveDeref (mgrCU, r->Bdd.data.bdd);
        r->Bdd.data.bdd = rCU;
      }
      else {
        Pdtutil_Assert(ddiMgr->abortOnSift,
          "NULL result only allowed with abortOnSift set");
        ddiMgr->abortedOp = 1;
      }
      break;
    case Ddi_Bdd_Aig_c:
      {
        Ddi_Aig_t *a, *b;
        a = DdiAigAnd(r->Bdd.data.aig,g->Bdd.data.aig);
        (void *)DdiGenericOp(Ddi_BddNot_c,Ddi_Accumulate_c,r,NULL,NULL);
        b = DdiAigAnd(r->Bdd.data.aig,h->Bdd.data.aig);
	DdiAigFree(r->Bdd.data.aig);
        r->Bdd.data.aig = DdiAigOr(a,b);
	DdiAigFree(a);
	DdiAigFree(b);
      }
      break;
    case Ddi_Bdd_Part_Conj_c:
    case Ddi_Bdd_Part_Disj_c:
    case Ddi_Bdd_Meta_c:
      /* ITE not allowed with partitioned and meta BDDs */
      Pdtutil_Warning (1,
        "ITE not supported with partitioned/meta BDDs\nTransformed to mono");
      (void *)DdiGenericOp(Ddi_BddMakeMono_c,Ddi_Accumulate_c,r,NULL,NULL);
      (void *)DdiGenericOp(opcode,Ddi_Accumulate_c,r,g,h);
      break;
    default:
      Pdtutil_Assert(0,"Wrong DDI node code");
      break;
    }
    break;

  /*
   *  Generalized cofactors
   */

  case Ddi_BddConstrain_c:
    DdiConsistencyCheck(r,Ddi_Bdd_c);
    DdiConsistencyCheck(g,Ddi_Bdd_c);
    switch (r->common.code) {
    case Ddi_Bdd_Mono_c:
      if (g->common.code == Ddi_Bdd_Part_Conj_c) {
        int n,j;
        n = DdiArrayNum(g->Bdd.data.part);
        for (j=0;j<n;j++) {
           (void *)DdiGenericOp(opcode,Ddi_Accumulate_c,r,
             DdiArrayRead(g->Bdd.data.part,j),NULL);
      }
        break;
      }
      else if (g->common.code == Ddi_Bdd_Part_Disj_c) {
        int n,done=0;
        n = DdiArrayNum(g->Bdd.data.part);
        if (n==2) {
          Ddi_Generic_t *p0, *p1, *w0, *w1, *common, *r0;
          p0 = DdiArrayRead(g->Bdd.data.part,0);
          p1 = DdiArrayRead(g->Bdd.data.part,1);
          if ((p0->common.code == Ddi_Bdd_Part_Conj_c)&&
              (p1->common.code == Ddi_Bdd_Part_Conj_c)) {
            w0 = DdiArrayRead(p0->Bdd.data.part,0);
            w1 = DdiArrayRead(p1->Bdd.data.part,0);
            common = DdiGenericOp(Ddi_BddAnd_c,Ddi_Generate_c,w0,w1,NULL);
            if (Ddi_BddIsZero((Ddi_Bdd_t *)common)) {
              r0 = DdiGenericOp(opcode,Ddi_Generate_c,r,p0,NULL);
              DdiGenericOp(opcode,Ddi_Accumulate_c,r,p1,NULL);
              DdiGenericOp(Ddi_BddAnd_c,Ddi_Accumulate_c,r0,w0,NULL);
              DdiGenericOp(Ddi_BddAnd_c,Ddi_Accumulate_c,r,w1,NULL);
              DdiGenericOp(Ddi_BddOr_c,Ddi_Accumulate_c,r,r0,NULL);
              Ddi_Free(r0);
            done = 1;
          }
          Ddi_Free(common);
        }
      }
      if (!done) {
#if 0
          Pdtutil_Warning (1,
           "This format of disj cofactoring term is not supported");
#endif
      }
        break;
      }
      else if (g->common.code == Ddi_Bdd_Mono_c) {
        rCU = Cudd_bddConstrain(mgrCU,r->Bdd.data.bdd,g->Bdd.data.bdd);
        if (rCU != NULL) {
          Cudd_Ref(rCU);
          Cudd_RecursiveDeref (mgrCU, r->Bdd.data.bdd);
          r->Bdd.data.bdd = rCU;
        }
        else {
          Pdtutil_Assert(ddiMgr->abortOnSift,
            "NULL result only allowed with abortOnSift set");
          ddiMgr->abortedOp = 1;
        }
      }
      break;
    case Ddi_Bdd_Part_Conj_c:
    case Ddi_Bdd_Part_Disj_c:
      ArrayOpIterate(opcode,r->Bdd.data.part,g,NULL);
      break;
    case Ddi_Bdd_Meta_c:
      DdiMetaConstrainAcc((Ddi_Bdd_t *)r,(Ddi_Bdd_t *)g);
      break;
    case Ddi_Bdd_Aig_c:
      //      break;
    default:
      Pdtutil_Assert(0,"Wrong DDI node code");
      break;
    }
    break;

  case Ddi_BddRestrict_c:
    DdiConsistencyCheck(r,Ddi_Bdd_c);
    DdiConsistencyCheck(g,Ddi_Bdd_c);
    if (g->common.code == Ddi_Bdd_Meta_c) {
      DdiMetaRestrictAcc((Ddi_Bdd_t *)r,(Ddi_Bdd_t *)g);
      break;
    }
    switch (r->common.code) {
    case Ddi_Bdd_Mono_c:
      if (g->common.code == Ddi_Bdd_Part_Conj_c) {
        int n,j;
        n = DdiArrayNum(g->Bdd.data.part);
        for (j=0;j<n;j++) {
           (void *)DdiGenericOp(opcode,Ddi_Accumulate_c,r,
             DdiArrayRead(g->Bdd.data.part,j),NULL);
      }
        break;
      }
      else if (g->common.code == Ddi_Bdd_Part_Disj_c) {
        int n,done=0;
        n = DdiArrayNum(g->Bdd.data.part);
        if (n==2) {
          Ddi_Generic_t *p0, *p1, *w0, *w1, *common, *r0, *r1;
          p0 = DdiArrayRead(g->Bdd.data.part,0);
          p1 = DdiArrayRead(g->Bdd.data.part,1);
          if (p0->common.code == Ddi_Bdd_Part_Conj_c) {
            w0 = DdiArrayRead(p0->Bdd.data.part,0);
        }
        else {
            w0 = p0;
        }
          if (p1->common.code == Ddi_Bdd_Part_Conj_c) {
            w1 = DdiArrayRead(p1->Bdd.data.part,0);
        }
        else {
            w1 = p1;
        }
          if ((Ddi_BddSize((Ddi_Bdd_t *)w0) < 10) &&
              (Ddi_BddSize((Ddi_Bdd_t *)w1) < 10)) {
            Cudd_ReorderingType dynordMethod;
            int autodyn = Ddi_MgrReadReorderingStatus (ddiMgr,&dynordMethod);
            if (autodyn) {
              Cudd_AutodynDisable(ddiMgr->mgrCU);
          }
            common = DdiGenericOp(Ddi_BddAnd_c,Ddi_Generate_c,w0,w1,NULL);
            if (Ddi_BddIsZero((Ddi_Bdd_t *)common)) {
             r0 = DdiGenericOp(opcode,Ddi_Generate_c,r,p0,NULL);
             r1 = DdiGenericOp(opcode,Ddi_Generate_c,r,p1,NULL);
             if ((Ddi_BddSize((Ddi_Bdd_t *)r0)<Ddi_BddSize((Ddi_Bdd_t *)r))&&
               (Ddi_BddSize((Ddi_Bdd_t *)r1)<Ddi_BddSize((Ddi_Bdd_t *)r))) {
               DdiGenericOp(Ddi_BddAnd_c,Ddi_Accumulate_c,r0,w0,NULL);
               DdiGenericOp(Ddi_BddAnd_c,Ddi_Accumulate_c,r1,w1,NULL);
               DdiGenericOp(Ddi_BddOr_c,Ddi_Accumulate_c,r0,r1,NULL);
               if (Ddi_BddSize((Ddi_Bdd_t *)r0)<Ddi_BddSize((Ddi_Bdd_t *)r)) {
                 DdiGenericDataCopy(r,r0);
             }
           }
             Ddi_Free(r1);
             Ddi_Free(r0);
           done = 1;
          }
            if (autodyn) {
              Cudd_AutodynEnable(ddiMgr->mgrCU,dynordMethod);
          }
          Ddi_Free(common);
        }
      }
      if (!done) {
#if 0
          Pdtutil_Warning (1,
           "This format of disj cofactoring term is not supported");
#endif
      }
        break;
      }
      else if (g->common.code == Ddi_Bdd_Meta_c) {
        DdiMetaRestrictAcc((Ddi_Bdd_t *)r,(Ddi_Bdd_t *)g);
      break;
      }
      Ddi_MgrSiftSuspend(ddiMgr);
      Ddi_MgrAbortOnSiftEnableWithThresh(ddiMgr,
        2*(Ddi_BddSize((Ddi_Bdd_t *)r)+Ddi_BddSize((Ddi_Bdd_t *)g)),1);
      rCU = Cudd_bddRestrict(mgrCU,r->Bdd.data.bdd,g->Bdd.data.bdd);
      if (rCU != NULL) {
        Cudd_Ref(rCU);
        Cudd_RecursiveDeref (mgrCU, r->Bdd.data.bdd);
        r->Bdd.data.bdd = rCU;
      }
      else {
        Pdtutil_Assert(ddiMgr->abortOnSift,
          "NULL result only allowed with abortOnSift set");
      rCU = r->Bdd.data.bdd;
      }
      Ddi_MgrAbortOnSiftDisable(ddiMgr);
      Ddi_MgrSiftResume(ddiMgr);
      break;
    case Ddi_Bdd_Part_Conj_c:
    case Ddi_Bdd_Part_Disj_c:
      ArrayOpIterate(opcode,r->Bdd.data.part,g,NULL);
      break;
    case Ddi_Bdd_Meta_c:
      DdiMetaRestrictAcc((Ddi_Bdd_t *)r,(Ddi_Bdd_t *)g);
      break;
    default:
      Pdtutil_Assert(0,"Wrong DDI node code");
      break;
    }
    break;

  /*
   *  Quantifiers
   */

  case Ddi_BddForall_c:
    /* bring to And operator using DeMorgan transformation */
    (void *)DdiGenericOp(Ddi_BddNot_c,Ddi_Accumulate_c,r,NULL,NULL);
    (void *)DdiGenericOp(Ddi_BddExist_c,Ddi_Accumulate_c,r,g,NULL);
    (void *)DdiGenericOp(Ddi_BddNot_c,Ddi_Accumulate_c,r,NULL,NULL);
    break;

  case Ddi_BddExist_c:
    DdiConsistencyCheck(r,Ddi_Bdd_c);
    DdiConsistencyCheck(g,Ddi_Varset_c);
    switch (r->common.code) {
    case Ddi_Bdd_Mono_c:
      rCU = Cudd_bddExistAbstract(mgrCU,r->Bdd.data.bdd,
                                        g->Varset.data.cu.bdd);
      if (rCU != NULL) {
        Cudd_Ref(rCU);
        Cudd_RecursiveDeref (mgrCU, r->Bdd.data.bdd);
        r->Bdd.data.bdd = rCU;
      }
      else {
        Pdtutil_Assert(ddiMgr->abortOnSift,
          "NULL result only allowed with abortOnSift set");
        ddiMgr->abortedOp = 1;
      }
      break;
    case Ddi_Bdd_Part_Conj_c:
      if (Ddi_BddPartNum((Ddi_Bdd_t *)r)==0)
        break;
#if 0
      tmp = (Ddi_Generic_t *)
        Part_BddMultiwayLinearAndExist((Ddi_Bdd_t*)r,(Ddi_Varset_t*)g,
          ddiMgr->settings.part.existClustThresh);
#else
#if 0
      {
      int saveClipDone = clipDone;
      int saveClipLevel = clipLevel;
      int saveClipThresh = clipThresh;
        Ddi_Bdd_t *saveClipWindow = clipWindow;
      clipDone = 0;
      clipLevel = 0;
      clipThresh = 10000000;
        clipWindow = NULL;
        tmp = (Ddi_Generic_t *)
          Ddi_BddMultiwayRecursiveAndExist((Ddi_Bdd_t*)r,(Ddi_Varset_t*)g,
            Ddi_MgrReadOne(ddiMgr),ddiMgr->settings.part.existClustThresh);
      clipDone = saveClipDone;
      clipLevel = saveClipLevel;
      clipThresh = saveClipThresh;
        clipWindow = saveClipWindow;
      }
#else
        tmp = (Ddi_Generic_t *)
          Ddi_BddMultiwayRecursiveAndExist((Ddi_Bdd_t*)r,(Ddi_Varset_t*)g,
            Ddi_MgrReadOne(ddiMgr),ddiMgr->settings.part.existClustThresh);
#endif
      /*
      tmp = (Ddi_Generic_t *)
        Part_BddMultiwayRecursiveTreeAndExist((Ddi_Bdd_t*)r,(Ddi_Varset_t*)g,
          Ddi_VarsetVoid(ddiMgr),ddiMgr->settings.part.existClustThresh);
      */
#endif

#if DEVEL_OPT_ANDEXIST
      {
        if (Cuplus_AuxVarset != NULL) {
          Ddi_Varset_t *supp=Ddi_BddSupp((Ddi_Bdd_t *)tmp);

          Ddi_VarsetIntersectAcc(supp,Cuplus_AuxVarset);
          if (!Ddi_VarsetIsVoid(supp)) {
            Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMed_c,
              fprintf(stdout, "<<%d->",Ddi_BddSize((Ddi_Bdd_t *)tmp))
            );
        }
#if 0
          Ddi_BddForallAcc((Ddi_Bdd_t *)tmp,Cuplus_AuxVarset);
#else
          rCU = Cuplus_bddExistAbstractAux(mgrCU,
            tmp->Bdd.data.bdd,g->Bdd.data.bdd);
          Cudd_Ref(rCU);
          Cudd_RecursiveDeref (mgrCU, tmp->Bdd.data.bdd);
          tmp->Bdd.data.bdd = rCU;
#endif
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMed_c,
            fprintf(stdout, "%d>>",Ddi_BddSize((Ddi_Bdd_t *)tmp))
          );
          }
          Ddi_Free(supp);
        }
      }

#endif
      /* result data is copied to the *r node */
      DdiGenericDataCopy(r,tmp);
      DdiGenericFree(tmp);

      if (ddiMgr->settings.part.existOptClustThresh >= 0) {
        Ddi_BddSetClustered((Ddi_Bdd_t *)r,
          ddiMgr->settings.part.existOptClustThresh);
      }
      if ((Ddi_BddIsPartConj((Ddi_Bdd_t *)r)||
        Ddi_BddIsPartDisj((Ddi_Bdd_t *)r))&&
        (Ddi_BddPartNum((Ddi_Bdd_t *)r)==1)) {
        Ddi_BddSetMono((Ddi_Bdd_t *)r);
      }
      break;
    case Ddi_Bdd_Part_Disj_c:
      /* EXIST is distributed over disjunction */
      ArrayOpIterate(opcode,r->Bdd.data.part,g,NULL);
      break;
    case Ddi_Bdd_Meta_c:
      DdiMetaAndExistAcc((Ddi_Bdd_t *)r,NULL,(Ddi_Varset_t *)g,NULL);
      break;
    case Ddi_Bdd_Aig_c:
      Ddi_AigExistAcc((Ddi_Bdd_t *)r,(Ddi_Varset_t *)g,NULL,0,0, -1);
      break;
    default:
      Pdtutil_Assert(0,"Wrong DDI node code");
      break;
    }
    break;

  case Ddi_BddAndExist_c:
    DdiConsistencyCheck(r,Ddi_Bdd_c);
    DdiConsistencyCheck(g,Ddi_Bdd_c);
    DdiConsistencyCheck(h,Ddi_Varset_c);
    switch (r->common.code) {
    case Ddi_Bdd_Mono_c:
#if DEVEL_OPT_ANDEXIST
#if 0
      if (!Ddi_VarsetIsVoid((Ddi_Varset_t *)h))
      {
        if (Cuplus_AuxVarset != NULL) {
          Ddi_Varset_t *supp=Ddi_BddSupp((Ddi_Bdd_t *)r);

          Ddi_VarsetIntersectAcc(supp,Cuplus_AuxVarset);
          if (!Ddi_VarsetIsVoid(supp)) {
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, "<<%d->",Ddi_BddSize((Ddi_Bdd_t *)r))
          );
          Ddi_BddForallAcc((Ddi_Bdd_t *)r,Cuplus_AuxVarset);
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, "%d>>",Ddi_BddSize((Ddi_Bdd_t *)r))
          );
          fflush(stdout);
          }
          Ddi_Free(supp);
        }
      }
#endif
#endif
      if (Ddi_BddIsPartConj((Ddi_Bdd_t *)g)) {
        tmp = DdiGenericOp(opcode,Ddi_Generate_c,g,r,h);
        DdiGenericDataCopy(r,tmp);
        DdiGenericFree(tmp);
      }
      else if (Ddi_BddIsPartDisj((Ddi_Bdd_t *)g)) {
        tmp = DdiGenericOp(opcode,Ddi_Generate_c,g,r,h);
        DdiGenericDataCopy(r,tmp);
        DdiGenericFree(tmp);
      }
      else {
        int *abortFrontier = NULL;
        int i,nvars = Ddi_MgrReadNumVars(ddiMgr);
        if (ddiMgr->abortOnSift) {
          abortFrontier = Pdtutil_Alloc(int,nvars);
          for (i=0; i<nvars; i++) {
            abortFrontier[i] = -1;
          }
      }
        rCU = Cuplus_bddAndAbstract(mgrCU,
          r->Bdd.data.bdd,g->Bdd.data.bdd,h->Varset.data.cu.bdd,
				    abortFrontier,ddiMgr->exist.infoPtr);
        if (rCU != NULL) {
          Cudd_Ref(rCU);
          Cudd_RecursiveDeref (mgrCU, r->Bdd.data.bdd);
          r->Bdd.data.bdd = rCU;
        }
        else {
          int nv;
          Ddi_Bdd_t *window = Ddi_BddMakeConst(ddiMgr,1);
          Pdtutil_Assert(ddiMgr->abortOnSift,
            "NULL result only allowed with abortOnSift set");
          ddiMgr->abortedOp = 1;
          for (nv=0,i=nvars-1; i>=0; i--) {
            if (abortFrontier[i]>=0)
              nv++;
        }
          nv = (1*nv)/2;
          for (i=nvars-1; i>=0; i--) {
            Ddi_Bdd_t *lit;
            int j = Ddi_MgrReadInvPerm(ddiMgr,i);
            int phase = (ddiMgr->exist.infoPtr->elseFirst != NULL) ? 
	      ddiMgr->exist.infoPtr->elseFirst[j] : 0;
            switch (abortFrontier[j]) {
          case 0:
              lit = Ddi_BddMakeLiteral(Ddi_VarFromCuddIndex(ddiMgr,j),phase);
              Ddi_BddAndAcc(window,lit);
              Ddi_BddNotAcc(lit);
              Ddi_BddOrAcc(window,lit);
              Ddi_Free(lit);
              break;
          case 1:
#if 0
              if (nv<0)
                break;
#endif
              lit = Ddi_BddMakeLiteral(Ddi_VarFromCuddIndex(ddiMgr,j),!phase);
              Ddi_BddAndAcc(window,lit);
              Ddi_Free(lit);
              break;
            case -1:
            break;
          default:
            break;
          }
#if 0
            if (abortFrontier[j]>=0) {
              nv--;
          }
#endif
          }
          DdiGenericDataCopy(r,(Ddi_Generic_t *)window);
          Ddi_Free(window);
        }
        Pdtutil_Free(abortFrontier);
      }
#if DEVEL_OPT_ANDEXIST
#if 0
      {
        if (Cuplus_AuxVarset != NULL) {
          Ddi_Varset_t *supp=Ddi_BddSupp((Ddi_Bdd_t *)r);

          Ddi_VarsetIntersectAcc(supp,Cuplus_AuxVarset);
          if (!Ddi_VarsetIsVoid(supp)) {
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, "<<%d->",Ddi_BddSize((Ddi_Bdd_t *)r))
          );
#if 0
          Ddi_BddForallAcc((Ddi_Bdd_t *)r,Cuplus_AuxVarset);
#else
          rCU = Cuplus_bddExistAbstractAux(mgrCU,
            r->Bdd.data.bdd,h->Bdd.data.bdd);
          Cudd_Ref(rCU);
          Cudd_RecursiveDeref (mgrCU, r->Bdd.data.bdd);
          r->Bdd.data.bdd = rCU;
#endif
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, "%d>>",Ddi_BddSize((Ddi_Bdd_t *)r))
          );
          fflush(stdout);
          }
          Ddi_Free(supp);
        }
      }
#endif
#endif
      break;
    case Ddi_Bdd_Part_Conj_c:
#if 1
      DdiArrayInsert(r->Bdd.data.part,0,g,Ddi_Dup_c);
      (void *)DdiGenericOp(Ddi_BddExist_c,Ddi_Accumulate_c,r,h,NULL);
#else
      {
      DdiArrayInsert(r->Bdd.data.part,0,g,Ddi_Dup_c);
      tmp = (Ddi_Generic_t *)
        Part_BddMultiwayRecursiveAndExist((Ddi_Bdd_t*)r,(Ddi_Varset_t*)h,
          Ddi_MgrReadOne(ddiMgr),-1);

      /* result data is copied to the *r node */
      DdiGenericDataCopy(r,tmp);
      DdiGenericFree(tmp);
      if ((Ddi_BddIsPartConj((Ddi_Bdd_t *)r)||
        Ddi_BddIsPartDisj((Ddi_Bdd_t *)r))&&
        (Ddi_BddPartNum((Ddi_Bdd_t *)r)==1)) {
        Ddi_BddSetMono((Ddi_Bdd_t *)r);
      }
      }
#endif
#if 0
      {
        Ddi_Varset_t *check = Ddi_BddSupp((Ddi_Bdd_t *)r);
        Ddi_VarsetIntersectAcc(check,(Ddi_Varset_t*)h);
        Pdtutil_Assert(Ddi_VarsetIsVoid(check),"Non quantified variables");
        Ddi_Free(check);
      }
#endif
      break;
    case Ddi_Bdd_Part_Disj_c:
      /* AND-EXIST is distributed over disjunction */
      ArrayOpIterate(opcode,r->Bdd.data.part,g,h);
      break;
    case Ddi_Bdd_Meta_c:
      DdiMetaAndExistAcc((Ddi_Bdd_t *)r,(Ddi_Bdd_t *)g,(Ddi_Varset_t *)h,NULL);
      break;
    case Ddi_Bdd_Aig_c:
      tmp = DdiGenericOp(opcode,Ddi_Generate_c,g,r,h);
      DdiGenericDataCopy(r,tmp);
      DdiGenericFree(tmp);
      break;
    default:
      Pdtutil_Assert(0,"Wrong DDI node code");
      break;
    }
    break;

  /*
   *  Compatible projector
   */

  case Ddi_BddCproject_c:
    DdiConsistencyCheck(r,Ddi_Bdd_c);
    DdiConsistencyCheck(g,Ddi_Bdd_c);
    switch (r->common.code) {
    case Ddi_Bdd_Mono_c:
      rCU = Cudd_CProjection(mgrCU,r->Bdd.data.bdd,g->Bdd.data.bdd);
      Cudd_Ref(rCU);
      Cudd_RecursiveDeref (mgrCU, r->Bdd.data.bdd);
      r->Bdd.data.bdd = rCU;
      break;
    case Ddi_Bdd_Part_Conj_c:
    case Ddi_Bdd_Part_Disj_c:
    case Ddi_Bdd_Meta_c:
      /* CPROJECT not allowed with partitioned and meta BDDs */
      Pdtutil_Warning (1,
        "CPROJECT not supported with part/meta BDDs\nTransformed to mono");
      (void *)DdiGenericOp(Ddi_BddMakeMono_c,Ddi_Accumulate_c,r,NULL,NULL);
      (void *)DdiGenericOp(opcode,Ddi_Accumulate_c,r,g,h);
      break;
    default:
      Pdtutil_Assert(0,"Wrong DDI node code");
      break;
    }
    break;

  /*
   *  Composition and variable substitution
   */

  case Ddi_BddCompose_c:
    DdiConsistencyCheck(r,Ddi_Bdd_c);
    DdiConsistencyCheck(g,Ddi_Vararray_c);
    DdiConsistencyCheck(h,Ddi_Bddarray_c);
    switch (r->common.code) {
    case Ddi_Bdd_Mono_c:
      {
        DdNode **Fv;
        int n, nMgrVars, j, id;

        nMgrVars = Ddi_MgrReadNumCuddVars(ddiMgr);
        /* the length of the two arrays must be the same */
        n = DdiArrayNum(g->Vararray.array);
        Pdtutil_Assert(n==DdiArrayNum(h->Bddarray.array),
          "different length of var/func arrays in compose");
        Fv = Pdtutil_Alloc(DdNode *,nMgrVars);
        for (j=0;j<nMgrVars;j++) {
           Fv[j] = Ddi_VarToCU(Ddi_VarFromCuddIndex(ddiMgr,j));
      }
        for (j=0;j<n;j++) {
           id = Ddi_VarCuddIndex(Ddi_VararrayRead((Ddi_Vararray_t *)g,j));
           Fv[id] = Ddi_BddToCU(Ddi_BddarrayRead((Ddi_Bddarray_t *)h,j));
      }
        Pdtutil_Assert(!ddiMgr->abortOnSift,
                 "Abort on sift not supported by this operator");
        rCU = Cudd_bddVectorCompose(mgrCU,r->Bdd.data.bdd,Fv);
        Pdtutil_Free(Fv);
        Cudd_Ref(rCU);
        Cudd_RecursiveDeref (mgrCU, r->Bdd.data.bdd);
        r->Bdd.data.bdd = rCU;
      }
      break;
    case Ddi_Bdd_Part_Conj_c:
    case Ddi_Bdd_Part_Disj_c:
      if (1) {
	int i;
	Ddi_Bdd_t *rPart = (Ddi_Bdd_t *)r;
	Ddi_Bddarray_t *rA = Ddi_BddarrayMakeFromBddPart(rPart); 
	Ddi_BddarrayComposeAcc(rA,(Ddi_Vararray_t*)g,(Ddi_Bddarray_t*)h);
        for (i=0; i<Ddi_BddarrayNum(rA); i++) {
          Ddi_Bdd_t *p_i = Ddi_BddarrayRead(rA,i);
          Ddi_BddPartWrite((Ddi_Bdd_t *)r,i,p_i);
        }
        Ddi_Free(rA);
      }
      else {
	ArrayOpIterate(opcode,r->Bdd.data.part,g,h); /* @@@ */
      }
      break;
    case Ddi_Bdd_Meta_c:
      DdiMetaComposeAcc((Ddi_Bdd_t *)r,
        (Ddi_Vararray_t*)g,(Ddi_Bddarray_t*)h);
      break;
    case Ddi_Bdd_Aig_c:
      DdiAigComposeAcc((Ddi_Bdd_t *)r,
        (Ddi_Vararray_t*)g,(Ddi_Bddarray_t*)h);
      break;
    default:
      Pdtutil_Assert(0,"Wrong DDI node code");
      break;
    }
    break;


  case Ddi_BddSwapVars_c:
  case Ddi_VarsetSwapVars_c:
    DdiConsistencyCheck2(r,Ddi_Bdd_c,Ddi_Varset_c);
    DdiConsistencyCheck(g,Ddi_Vararray_c);
    DdiConsistencyCheck(h,Ddi_Vararray_c);
    switch (r->common.code) {
    case Ddi_Varset_Bdd_c:
    case Ddi_Bdd_Mono_c:
      {
        DdNode **Xv,**Yv;
        int n;

        /* the length of the two arrays of variables must be the same */
        n = DdiArrayNum(g->Vararray.array);
        Pdtutil_Assert(n==DdiArrayNum(h->Vararray.array),
          "different length of var arrays in var swap");
        Xv = Ddi_VararrayToCU((Ddi_Vararray_t *)g);
        Yv = Ddi_VararrayToCU((Ddi_Vararray_t *)h);
        Pdtutil_Assert(!ddiMgr->abortOnSift,
                  "Abort on sift not supported by this operator");
        rCU = Cudd_bddSwapVariables(mgrCU,r->Bdd.data.bdd,Xv,Yv,n);
        Pdtutil_Free(Xv);
        Pdtutil_Free(Yv);
        Cudd_Ref(rCU);
        Cudd_RecursiveDeref (mgrCU, r->Bdd.data.bdd);
        r->Bdd.data.bdd = rCU;
      }
      break;
    case Ddi_Bdd_Aig_c:
      {
      int i, n = DdiArrayNum(g->Vararray.array);
      Ddi_Bddarray_t *lits=Ddi_BddarrayAlloc(ddiMgr,2*n);
      Ddi_Vararray_t *vars=Ddi_VararrayAlloc(ddiMgr,2*n);
      for (i=0; i<n; i++) {
        Ddi_Var_t *v0 = (Ddi_Var_t *)DdiArrayRead(g->Vararray.array,i);
        Ddi_Var_t *v1 = (Ddi_Var_t *)DdiArrayRead(h->Vararray.array,i);
        Ddi_Bdd_t *l0 = Ddi_BddMakeLiteralAig(v1,1);
        Ddi_Bdd_t *l1 = Ddi_BddMakeLiteralAig(v0,1);
        Ddi_VararrayWrite(vars,i,v0);
        Ddi_VararrayWrite(vars,i+n,v1);
        Ddi_BddarrayWrite(lits,i,l0);
        Ddi_BddarrayWrite(lits,i+n,l1);
        Ddi_Free(l0); Ddi_Free(l1);
      }
      DdiAigComposeAcc ((Ddi_Bdd_t *)r,vars,lits);
      Ddi_Free(lits); Ddi_Free(vars);
      }
      break;
    case Ddi_Bdd_Part_Conj_c:
    case Ddi_Bdd_Part_Disj_c:
      {
      Ddi_Bdd_t *rPart = (Ddi_Bdd_t *)r;
        if (Ddi_BddPartNum(rPart)>0 && Ddi_BddIsAig(Ddi_BddPartRead(rPart,0))) {
        int i, n = DdiArrayNum(g->Vararray.array);
        Ddi_Bddarray_t *lits=Ddi_BddarrayAlloc(ddiMgr,2*n);
        Ddi_Vararray_t *vars=Ddi_VararrayAlloc(ddiMgr,2*n);
        Ddi_Bddarray_t *rArray = Ddi_BddarrayMakeFromBddPart(rPart);
        for (i=0; i<n; i++) {
          Ddi_Var_t *v0 = (Ddi_Var_t *)DdiArrayRead(g->Vararray.array,i);
          Ddi_Var_t *v1 = (Ddi_Var_t *)DdiArrayRead(h->Vararray.array,i);
          Ddi_Bdd_t *l0 = Ddi_BddMakeLiteralAig(v1,1);
          Ddi_Bdd_t *l1 = Ddi_BddMakeLiteralAig(v0,1);
          Ddi_VararrayWrite(vars,i,v0);
          Ddi_VararrayWrite(vars,i+n,v1);
          Ddi_BddarrayWrite(lits,i,l0);
          Ddi_BddarrayWrite(lits,i+n,l1);
          Ddi_Free(l0); Ddi_Free(l1);
        }
        Ddi_AigarrayComposeNoMultipleAcc(rArray,vars,lits);
        Ddi_Free(lits); Ddi_Free(vars);
        for (i=0; i<Ddi_BddarrayNum(rArray); i++) {
          Ddi_Bdd_t *p_i = Ddi_BddarrayRead(rArray,i);
          Ddi_BddPartWrite((Ddi_Bdd_t *)r,i,p_i);
        }
        Ddi_Free(rArray);
      }
      else {
        ArrayOpIterate(opcode,r->Bdd.data.part,g,h);
      }
      }
      break;
    case Ddi_Bdd_Meta_c:
      DdiMetaSwapVarsAcc((Ddi_Bdd_t *)r,
        (Ddi_Vararray_t*)g,(Ddi_Vararray_t*)h);
      break;
    case Ddi_Varset_Array_c:
    case Ddi_Vararray_c:
      {
	Ddi_Vararray_t *va = (r->common.code==Ddi_Varset_Array_c) ?
          r->Varset.data.array.va :
          (Ddi_Vararray_t *)r;
	int i, n=Ddi_VararrayNum(va);
        int nv=DdiArrayNum(g->Vararray.array);
	int mark;

        for (i=0; i<nv; i++) {
          Ddi_Var_t *v0 = (Ddi_Var_t *)DdiArrayRead(g->Vararray.array,i);
          Ddi_Var_t *v1 = (Ddi_Var_t *)DdiArrayRead(h->Vararray.array,i);
	  Pdtutil_Assert(Ddi_VarReadMark(v0)==0,"0 var mark required");
	  Pdtutil_Assert(Ddi_VarReadMark(v1)==0,"0 var mark required");
	  Ddi_VarWriteMark(v0,i+1);
	  Ddi_VarWriteMark(v1,i+nv+1);
	}

	for (i=0; i<n; i++) {
	  Ddi_Var_t *v1, *v = Ddi_VararrayRead(va,i);
	  mark = Ddi_VarReadMark(v);
	  if (mark==0) continue; /* do not change */
	  if (mark<=nv) {
	    /* first array: swap with second */
	    Pdtutil_Assert(mark>0&&mark<=nv,"wrong mark");
	    v1 = (Ddi_Var_t *)DdiArrayRead(h->Vararray.array,mark-1);
	  }
	  else {
	    Pdtutil_Assert(mark<=2*nv,"wrong mark");
	    v1 = (Ddi_Var_t *)DdiArrayRead(g->Vararray.array,mark-nv-1);
	  }
	  Ddi_VararrayWrite(va,i,v1);
	}

        for (i=0; i<nv; i++) {
          Ddi_Var_t *v0 = (Ddi_Var_t *)DdiArrayRead(g->Vararray.array,i);
          Ddi_Var_t *v1 = (Ddi_Var_t *)DdiArrayRead(h->Vararray.array,i);
	  Ddi_VarWriteMark(v0,0);
	  Ddi_VarWriteMark(v1,0);
	}
        if (r->common.code==Ddi_Varset_Array_c)
          Ddi_VararraySort (va, 0);
      }
      break;
    default:
      Pdtutil_Assert(0,"Wrong DDI node code");
      break;
    }
    break;

  case Ddi_VararraySubstVars_c:
      {
	Ddi_Vararray_t *va = (Ddi_Vararray_t *)r;
	int i, n=Ddi_VararrayNum(va);
        int nv=DdiArrayNum(g->Vararray.array);
	int mark;

        for (i=0; i<nv; i++) {
          Ddi_Var_t *v0 = (Ddi_Var_t *)DdiArrayRead(g->Vararray.array,i);
          Ddi_Var_t *v1 = (Ddi_Var_t *)DdiArrayRead(h->Vararray.array,i);
	  Pdtutil_Assert(Ddi_VarReadMark(v0)==0,"0 var mark required");
	  Pdtutil_Assert(Ddi_VarReadMark(v1)==0,"0 var mark required");
	  Ddi_VarWriteMark(v0,i+1);
	  Ddi_VarWriteMark(v1,i+nv+1);
	}

	for (i=0; i<n; i++) {
	  Ddi_Var_t *v1, *v = Ddi_VararrayRead(va,i);
	  mark = Ddi_VarReadMark(v);
	  if (mark==0) continue; /* do not change */
	  if (mark<=nv) {
	    /* first array: swap with second */
	    Pdtutil_Assert(mark>=0&&mark<=nv,"wrong mark");
	    v1 = (Ddi_Var_t *)DdiArrayRead(h->Vararray.array,mark-1);
            Ddi_VararrayWrite(va,i,v1);
	  }
	}

        for (i=0; i<nv; i++) {
          Ddi_Var_t *v0 = (Ddi_Var_t *)DdiArrayRead(g->Vararray.array,i);
          Ddi_Var_t *v1 = (Ddi_Var_t *)DdiArrayRead(h->Vararray.array,i);
	  Ddi_VarWriteMark(v0,0);
	  Ddi_VarWriteMark(v1,0);
	}
      }
    break;

  case Ddi_BddSubstVars_c:
  case Ddi_VarsetSubstVars_c:
    DdiConsistencyCheck3(r,Ddi_Bdd_c,Ddi_Varset_c,Ddi_Vararray_c);
    DdiConsistencyCheck(g,Ddi_Vararray_c);
    DdiConsistencyCheck(h,Ddi_Vararray_c);
    switch (r->common.code) {
    case Ddi_Varset_Bdd_c:
    case Ddi_Bdd_Mono_c:
      {
        DdNode **Fv;
        int n, nMgrVars, j, id;

        nMgrVars = Ddi_MgrReadNumCuddVars(ddiMgr);
        /* the length of the two arrays must be the same */
        n = DdiArrayNum(g->Vararray.array);
        Pdtutil_Assert(n==DdiArrayNum(h->Vararray.array),
          "different length of var arrays in var subst");
        Fv = Pdtutil_Alloc(DdNode *,nMgrVars);
        for (j=0;j<nMgrVars;j++) {
           Fv[j] = Ddi_VarToCU(Ddi_VarFromCuddIndex(ddiMgr,j));
	}
        for (j=0;j<n;j++) {
           id = Ddi_VarCuddIndex(Ddi_VararrayRead((Ddi_Vararray_t *)g,j));
           Fv[id] = Ddi_VarToCU(Ddi_VararrayRead((Ddi_Vararray_t *)h,j));
	}
        Pdtutil_Assert(!ddiMgr->abortOnSift,
                 "Abort on sift not supported by this operator");
        rCU = Cudd_bddVectorCompose(mgrCU,r->Bdd.data.bdd,Fv);
        Pdtutil_Free(Fv);
        Cudd_Ref(rCU);
        Cudd_RecursiveDeref (mgrCU, r->Bdd.data.bdd);
        r->Bdd.data.bdd = rCU;
      }
      break;
    case Ddi_Bdd_Aig_c:
      {
	int i, n = DdiArrayNum(g->Vararray.array);
	Ddi_Bddarray_t *lits=Ddi_BddarrayAlloc(ddiMgr,n);
	Ddi_Vararray_t *vars=Ddi_VararrayAlloc(ddiMgr,n);
	for (i=0; i<n; i++) {
	  Ddi_Var_t *v0 = (Ddi_Var_t *)DdiArrayRead(g->Vararray.array,i);
	  Ddi_Var_t *v1 = (Ddi_Var_t *)DdiArrayRead(h->Vararray.array,i);
	  Ddi_Bdd_t *l1 = Ddi_BddMakeLiteralAig(v1,1);
	  Ddi_VararrayWrite(vars,i,v0);
	  Ddi_BddarrayWrite(lits,i,l1);
	  Ddi_Free(l1);
	}
        DdiAigComposeAcc ((Ddi_Bdd_t *)r,vars,lits);
	Ddi_Free(lits); Ddi_Free(vars);
      }
      break;
    case Ddi_Bdd_Part_Conj_c:
    case Ddi_Bdd_Part_Disj_c:
      {
	Ddi_Bdd_t *rPart = (Ddi_Bdd_t *)r;
        if (Ddi_BddPartNum(rPart)>0 && Ddi_BddIsAig(Ddi_BddPartRead(rPart,0))) {
	  int i, n = DdiArrayNum(g->Vararray.array);
	  Ddi_Bddarray_t *lits=Ddi_BddarrayAlloc(ddiMgr,n);
	  Ddi_Vararray_t *vars=Ddi_VararrayAlloc(ddiMgr,n);
	  Ddi_Bddarray_t *rArray = Ddi_BddarrayMakeFromBddPart(rPart);
	  for (i=0; i<n; i++) {
	    Ddi_Var_t *v0 = (Ddi_Var_t *)DdiArrayRead(g->Vararray.array,i);
	    Ddi_Var_t *v1 = (Ddi_Var_t *)DdiArrayRead(h->Vararray.array,i);
	    Ddi_Bdd_t *l1 = Ddi_BddMakeLiteralAig(v1,1);
	    Ddi_VararrayWrite(vars,i,v0);
	    Ddi_BddarrayWrite(lits,i,l1);
	    Ddi_Free(l1);
	  }
          Ddi_AigarrayComposeNoMultipleAcc(rArray,vars,lits);
	  Ddi_Free(lits); Ddi_Free(vars);
	  for (i=0; i<Ddi_BddarrayNum(rArray); i++) {
	    Ddi_Bdd_t *p_i = Ddi_BddarrayRead(rArray,i);
	    Ddi_BddPartWrite((Ddi_Bdd_t *)r,i,p_i);
	  }
	  Ddi_Free(rArray);
	}
	else {
	  ArrayOpIterate(opcode,r->Bdd.data.part,g,h);
	}
      }
      break;
    case Ddi_Bdd_Meta_c:
      DdiMetaSubstVarsAcc((Ddi_Bdd_t *)r,
        (Ddi_Vararray_t*)g,(Ddi_Vararray_t*)h);
      break;
    default:
      Pdtutil_Assert(0,"Wrong DDI node code");
      break;
    }
    break;

  /*
   *  Support handling
   */

  case Ddi_BddSupp_c:
    Pdtutil_Assert(optype==Ddi_Generate_c,
      "support computation must be a generate function");
    DdiGenericFree(r);
    if (f->common.supp!=NULL) {
      return(DdiGenericDup(f->common.supp));
    }
    switch (f->common.type) {
    case Ddi_Bdd_c:
      switch (f->common.code) {
      case Ddi_Bdd_Mono_c:
        r = (Ddi_Generic_t *) Ddi_VarsetMakeFromCU(ddiMgr,
          Cudd_Support(mgrCU,f->Bdd.data.bdd));
        break;
      case Ddi_Bdd_Part_Conj_c:
      case Ddi_Bdd_Part_Disj_c:
        if (DdiArrayNum(f->Bdd.data.part)==0) {
          r = (Ddi_Generic_t *)Ddi_VarsetVoid(ddiMgr);
        }
        else if (DdiArrayNum(f->Bdd.data.part)==1) {
          r = ArraySupp(f->Bdd.data.part);
        }
        else {
          r = ArraySupp2(f);
        }
        break;
      case Ddi_Bdd_Meta_c:
        r = (Ddi_Generic_t *) DdiMetaSupp(f->Bdd.data.meta);
        break;
      case Ddi_Bdd_Aig_c:
        r = (Ddi_Generic_t *) DdiAigSupp((Ddi_Bdd_t *)f);
        Pdtutil_Assert(r!=NULL,"NULL support");
#if DDI_VARSET_DBG
	if (r->common.code==Ddi_Varset_Array_c) {
	  int i;
	  Ddi_Vararray_t *va = r->Varset.data.array.va;
	  int n = Ddi_VararrayNum(va);
	  for (i=1; i<n; i++) {
	    Ddi_Var_t *v0 = Ddi_VararrayRead(va,i-1);
	    Ddi_Var_t *v1 = Ddi_VararrayRead(va,i);
	    Pdtutil_Assert(!DdiVarIsAfterVar(v0,v1),"wrong var ord in supp");
	  }
	}
#endif
        break;
      default:
        Pdtutil_Assert(0,"Wrong DDI node code");
      break;
      }
      break;
    case Ddi_Bddarray_c:
      r = ArraySupp(f->Bddarray.array);
      break;
    case Ddi_Var_c:
      r = (Ddi_Generic_t *)Ddi_VarsetMakeFromCU(ddiMgr,f->Var.data.p.bdd);
      break;
    case Ddi_Vararray_c:
      r = ArraySupp(f->Vararray.array);
      break;
    case Ddi_Varset_c:
      r = (Ddi_Generic_t *) Ddi_VarsetMakeFromCU(ddiMgr,
        Cudd_Support(mgrCU,f->Bdd.data.bdd));
      break;
    case Ddi_Varsetarray_c:
      r = ArraySupp(f->Varsetarray.array);
      break;
    default:
      Pdtutil_Assert (0, "Wrong DDI node type");
    }
    break;

  case Ddi_BddSuppAttach_c:
    Pdtutil_Assert(optype==Ddi_Accumulate_c,
      "support computation must be an accumulate function");
    /* support computed after all operations */
    attachSupport = 1;
    break;
  case Ddi_BddSuppDetach_c:
    Pdtutil_Assert(optype==Ddi_Accumulate_c,
      "support detach must be an accumulate function");
    Ddi_Unlock(r->common.supp);
    DdiGenericFree(r->common.supp);
    r->common.supp = NULL;
    attachSupport = 0;
    break;

  /*
   *  Variable array (as set) operations
   */

  case Ddi_VararrayUnion_c:
  case Ddi_VararrayIntersect_c:
  case Ddi_VararrayDiff_c:

    DdiConsistencyCheck(r,Ddi_Vararray_c);
    DdiConsistencyCheck(g,Ddi_Vararray_c);
    {
      Ddi_ArrayData_t *a0, *a1;
      Ddi_Var_t *v0, *v1;
      int mark;
      Ddi_ArrayData_t *res = DdiArrayAlloc(0);

      a0 = r->Vararray.array;
      a1 = g->Vararray.array;

      for (i=0;i<DdiArrayNum(a0);i++) {
	v0 = (Ddi_Var_t *)DdiArrayRead(a0,i);
	Pdtutil_Assert(Ddi_VarReadMark(v0)==0,"0 var mark required");
	Ddi_VarWriteMark(v0,1);
      }
      for (i=0;i<DdiArrayNum(a1);i++) {
	v1 = (Ddi_Var_t *)DdiArrayRead(a1,i);
	mark = Ddi_VarReadMark(v1);

	Pdtutil_Assert(mark<=1,"0 var mark required");
	Ddi_VarWriteMark(v1,mark+2);
      }

      for (i=0;i<DdiArrayNum(a0);i++) {
	int selected = 0;
	v0 = (Ddi_Var_t *)DdiArrayRead(a0,i);
	/* mark = 1 (in a0), 2 (in a1), 3 (in a0&a1) */
	switch (opcode) {
	case Ddi_VararrayIntersect_c:
	  selected = Ddi_VarReadMark(v0)==3;
	  break;
	case Ddi_VararrayDiff_c:
	  selected = Ddi_VarReadMark(v0)==1;
	  break;
	case Ddi_VararrayUnion_c:
	  selected = Ddi_VarReadMark(v0)>0;
	  break;
	}
	if (selected) {
	  DdiArrayWrite(res,DdiArrayNum(res),(Ddi_Generic_t *)v0,Ddi_Mov_c);
	}
	Ddi_VarWriteMark(v0,0);
      }
      for (i=0;i<DdiArrayNum(a1);i++) {
	int selected = 0;
	v1 = (Ddi_Var_t *)DdiArrayRead(a1,i);
	/* mark = 1 (in a0), 2 (in a1), 3 (in a0&a1) */
	switch (opcode) {
	case Ddi_VararrayIntersect_c:
	case Ddi_VararrayDiff_c:
	  break;
	case Ddi_VararrayUnion_c:
	  selected = Ddi_VarReadMark(v1)==2;
	  break;
	}
	if (selected) {
	  DdiArrayWrite(res,DdiArrayNum(res),(Ddi_Generic_t *)v1,Ddi_Mov_c);
	}
	Ddi_VarWriteMark(v1,0);
      }

      /* link res to r */
      DdiArrayFree(r->Vararray.array);
      r->Vararray.array = res;

    }

    break;

  /*
   *  Variable set operations
   */

  case Ddi_VarsetNext_c:
    DdiConsistencyCheck(r,Ddi_Varset_c);
    rCU = Cudd_T(r->Varset.data.cu.bdd);
    Cudd_Ref(rCU);
    Cudd_RecursiveDeref (mgrCU, r->Varset.data.cu.bdd);
    r->Varset.data.cu.bdd = rCU;
    break;
  case Ddi_VarsetUnion_c:
  case Ddi_VarsetAdd_c:
    DdiConsistencyCheck(r,Ddi_Varset_c);
    DdiConsistencyCheck2(g,Ddi_Var_c,Ddi_Varset_c);
    switch (r->common.code) {
    case Ddi_Varset_Bdd_c:
      Ddi_MgrAbortOnSiftSuspend(ddiMgr);
      if (g->common.type==Ddi_Var_c) {
        Pdtutil_Assert (g->common.code==Ddi_Var_Bdd_c, "wrong var type");
        rCU = Cudd_bddAnd(mgrCU,r->Varset.data.cu.bdd,g->Var.data.p.bdd);
      }
      else {
        Pdtutil_Assert (g->common.code==Ddi_Varset_Bdd_c, "wrong varset type");
        rCU = Cudd_bddAnd(mgrCU,r->Varset.data.cu.bdd,g->Varset.data.cu.bdd);
      }
      Cudd_Ref(rCU);
      Cudd_RecursiveDeref (mgrCU, r->Varset.data.cu.bdd);
      r->Varset.data.cu.bdd = rCU;
      Ddi_MgrAbortOnSiftResume(ddiMgr);

      break;
    case Ddi_Varset_Array_c:
      if (g->common.type==Ddi_Var_c) {
	int n, i1, i0, i, found=0;
	Ddi_Vararray_t *va;
	va = r->Varset.data.array.va;
	n = Ddi_VararrayNum(va);
        if (n==0) {
          i0=0;
        }
        else if (DdiVarIsAfterVar((Ddi_Var_t *)g,Ddi_VararrayRead(va,n-1))) {
          i0=n;
        }
        else {
          for (i0=0,i1=n-1; i1>=i0; ) {
            i = (i0+i1)/2;
            if (Ddi_VararrayRead(va,i)==(Ddi_Var_t *)g) {
              found = 1; break;
            }
            else if (DdiVarIsAfterVar(Ddi_VararrayRead(va,i),(Ddi_Var_t *)g)) {
              i1 = i-1;
            }
            else {
              i0 = i+1;
            }
          }
        }
	if (!found) {
	  Pdtutil_Assert(i0>=0&i0<=n,"error in binary search");
	  Ddi_VararrayInsert(va,i0,(Ddi_Var_t *)g);
	}
      }
      else {
	int n0, n1, i0, i1, i;
	Ddi_Vararray_t *va0, *va1, *vaNew;
	Ddi_Var_t *v0, *v1;
        Pdtutil_Assert (g->common.code==Ddi_Varset_Array_c,
          "wrong varset type");
	va0 = r->Varset.data.array.va;
	va1 = g->Varset.data.array.va;
	n0 = Ddi_VararrayNum(va0);
	n1 = Ddi_VararrayNum(va1);
	Ddi_Unlock(r->Varset.data.array.va);
	r->Varset.data.array.va = vaNew = Ddi_VararrayAlloc(ddiMgr,0);
	Ddi_Lock(vaNew);

#if DDI_VARSET_DBG
	for (i=1; i<n0; i++) {
	  v0 = Ddi_VararrayRead(va0,i-1);
	  v1 = Ddi_VararrayRead(va0,i);
	  Pdtutil_Assert(!DdiVarIsAfterVar(v0,v1),"wrong var ord");
	}
	for (i=1; i<n1; i++) {
	  v0 = Ddi_VararrayRead(va1,i-1);
	  v1 = Ddi_VararrayRead(va1,i);
	  Pdtutil_Assert(!DdiVarIsAfterVar(v0,v1),"wrong var ord");
	}
#endif

	for (i=i0=i1=0; i0<n0&&i1<n1; i++) {
	  v0 = Ddi_VararrayRead(va0,i0);
	  v1 = Ddi_VararrayRead(va1,i1);
	  if (v0 == v1) {
	    Ddi_VararrayWrite(vaNew,i,v0); i0++; i1++;
	  }
	  else if (DdiVarIsAfterVar(v1,v0)) {
	    Ddi_VararrayWrite(vaNew,i,v0); i0++;
	  }
	  else {
          Ddi_VararrayWrite(vaNew,i,v1); i1++;
	  }
	}
	for (; i0<n0; i0++, i++) {
	  v0 = Ddi_VararrayRead(va0,i0);
	  Ddi_VararrayWrite(vaNew,i,v0);
	}
	for (; i1<n1; i1++, i++) {
        v1 = Ddi_VararrayRead(va1,i1);
        Ddi_VararrayWrite(vaNew,i,v1);
	}
	Ddi_Free(va0);
	Pdtutil_Assert(Ddi_VararrayNum(vaNew)<=(n0+n1),
		       "Wrong size of UNION array");
      }

      break;
    default:
      Pdtutil_Assert (0, "Wrong DDI node type");
    }

    break;
  case Ddi_VarsetIntersect_c:
    DdiConsistencyCheck(r,Ddi_Varset_c);
    DdiConsistencyCheck2(g,Ddi_Var_c,Ddi_Varset_c);

    switch (r->common.code) {

    case Ddi_Varset_Bdd_c:
      Ddi_MgrAbortOnSiftSuspend(ddiMgr);
      if (g->common.type==Ddi_Var_c) {
	Pdtutil_Assert(g->common.code==Ddi_Var_Bdd_c,"BDD var required");
        rCU = Cudd_bddLiteralSetIntersection(mgrCU,
          r->Varset.data.cu.bdd,g->Var.data.p.bdd);
      }
      else {
	Pdtutil_Assert(g->common.code==Ddi_Varset_Bdd_c,"BDD varset required");
        rCU = Cudd_bddLiteralSetIntersection(mgrCU,
          r->Varset.data.cu.bdd,g->Varset.data.cu.bdd);
      }
      Cudd_Ref(rCU);
      Cudd_RecursiveDeref (mgrCU, r->Varset.data.cu.bdd);
      r->Varset.data.cu.bdd = rCU;
      Ddi_MgrAbortOnSiftResume(ddiMgr);
      break;

    case Ddi_Varset_Array_c:

      {
      int n0, n1, i0, i1, i, freeVa1=0;
      Ddi_Vararray_t *va0, *va1, *vaNew;
      Ddi_Var_t *v0, *v1;
      va0 = r->Varset.data.array.va;
        if (g->common.code==Ddi_Varset_Array_c) {
        va1 = g->Varset.data.array.va;
      }
      else {
        va1 = Ddi_VararrayMakeFromVarset((Ddi_Varset_t *)g,0);
        freeVa1 = 1;
      }
      n0 = Ddi_VararrayNum(va0);
      n1 = Ddi_VararrayNum(va1);
      Ddi_Unlock(r->Varset.data.array.va);
      r->Varset.data.array.va = vaNew = Ddi_VararrayAlloc(ddiMgr,0);
      Ddi_Lock(vaNew);
#if DDI_VARSET_DBG
      for (i=1; i<n0; i++) {
	v0 = Ddi_VararrayRead(va0,i-1);
	v1 = Ddi_VararrayRead(va0,i);
	Pdtutil_Assert(!DdiVarIsAfterVar(v0,v1),"wrong var ord");
      }
      for (i=1; i<n1; i++) {
	v0 = Ddi_VararrayRead(va1,i-1);
	v1 = Ddi_VararrayRead(va1,i);
	Pdtutil_Assert(!DdiVarIsAfterVar(v0,v1),"wrong var ord");
      }
#endif
      for (i=i0=i1=0; i0<n0&&i1<n1; ) {
        v0 = Ddi_VararrayRead(va0,i0);
        v1 = Ddi_VararrayRead(va1,i1);
        if (DdiVarIsAfterVar(v1,v0)) {
          i0++;
        }
        else if (DdiVarIsAfterVar(v0,v1)) {
          i1++;
        }
        else {
          Pdtutil_Assert(v0==v1,"wrong eq variable test");
          /* equal vars */
          Ddi_VararrayWrite(vaNew,i,v0);
          i0++; i1++; i++;
        }
      }
      if (freeVa1) Ddi_Free(va1);
      Ddi_Free(va0);
      Pdtutil_Assert(Ddi_VararrayNum(vaNew)<=n0&&Ddi_VararrayNum(vaNew)<=n1,
          "Wrong size of INTERSECTION array");
      }
      break;

    default:
      Pdtutil_Assert (0, "Wrong DDI node type");
    }

    break;
  case Ddi_VarsetDiff_c:
  case Ddi_VarsetRemove_c:
    DdiConsistencyCheck(r,Ddi_Varset_c);
    DdiConsistencyCheck2(g,Ddi_Var_c,Ddi_Varset_c);

    switch (r->common.code) {

    case Ddi_Varset_Bdd_c:

      Ddi_MgrAbortOnSiftSuspend(ddiMgr);
      if (g->common.type==Ddi_Var_c) {
	Pdtutil_Assert(g->common.code==Ddi_Var_Bdd_c,"BDD var required");
        rCU = Cudd_bddExistAbstract(mgrCU,
                r->Varset.data.cu.bdd,g->Var.data.p.bdd);
      }
      else {
	Pdtutil_Assert(g->common.code==Ddi_Varset_Bdd_c,"BDD varset required");
        rCU = Cudd_bddExistAbstract(mgrCU,
                r->Varset.data.cu.bdd,g->Varset.data.cu.bdd);
      }
      Cudd_Ref(rCU);
      Cudd_RecursiveDeref (mgrCU, r->Varset.data.cu.bdd);
      r->Varset.data.cu.bdd = rCU;
      Ddi_MgrAbortOnSiftResume(ddiMgr);
      break;

    case Ddi_Varset_Array_c:

      {
      int n0, n1, i0, i1, i, freeVa1=0;
      Ddi_Vararray_t *va0, *va1, *vaNew;
      Ddi_Var_t *v0, *v1;
      va0 = r->Varset.data.array.va;
        if (g->common.code==Ddi_Varset_Array_c) {
        va1 = g->Varset.data.array.va;
      }
        else if (g->common.code==Ddi_Varset_Bdd_c) {
        va1 = Ddi_VararrayMakeFromVarset((Ddi_Varset_t *)g,0);
        freeVa1 = 1;
      }
      else {
        va1 = Ddi_VararrayAlloc(ddiMgr,1);
        Ddi_VararrayWrite(va1,0,(Ddi_Var_t *)g);
        freeVa1 = 1;
      }
      n0 = Ddi_VararrayNum(va0);
      n1 = Ddi_VararrayNum(va1);
      Ddi_Unlock(r->Varset.data.array.va);
      r->Varset.data.array.va = vaNew = Ddi_VararrayAlloc(ddiMgr,0);
      Ddi_Lock(vaNew);
      for (i=i0=i1=0; i0<n0&&i1<n1; ) {
        v0 = Ddi_VararrayRead(va0,i0);
        v1 = Ddi_VararrayRead(va1,i1);
        if (DdiVarIsAfterVar(v1,v0)) {
          /* write var in v0 and not in v1 */
          Ddi_VararrayWrite(vaNew,i,v0); i++; i0++;
        }
        else if (DdiVarIsAfterVar(v0,v1)) {
          i1++;
        }
        else {
          Pdtutil_Assert(v0==v1,"wrong eq variable test");
          /* equal vars: not written in result */
          i0++; i1++;
        }
      }
      for (; i0<n0; i0++, i++) {
        /* write residual v0 vars */
        v0 = Ddi_VararrayRead(va0,i0);
        Ddi_VararrayWrite(vaNew,i,v0);
      }

      if (freeVa1) Ddi_Free(va1);
      Ddi_Free(va0);
      Pdtutil_Assert(Ddi_VararrayNum(vaNew)<=n0,
          "Wrong size of DIFFERENCE array");
      }
      break;

    default:
      Pdtutil_Assert (0, "Wrong DDI node type");
    }

    break;

  default:
    Pdtutil_Assert(0,"Unknown operation code in generic op");
    break;
  }

  r->common.size = 0;
  if (attachSupport) {
    if (r->common.supp != NULL) {
      Ddi_Unlock(r->common.supp);
      DdiGenericFree(r->common.supp);
      r->common.supp = NULL;
    }
    Ddi_MgrAbortOnSiftSuspend(ddiMgr);
    r->common.supp = DdiGenericOp(Ddi_BddSupp_c,Ddi_Generate_c,r,NULL,NULL);
    Ddi_Lock(r->common.supp);
    Ddi_MgrAbortOnSiftResume(ddiMgr);
  }

  if (0 && 
      r->common.type == Ddi_Varset_c && r->common.code == Ddi_Varset_Array_c) {
    Ddi_Vararray_t *va = r->Varset.data.array.va;
    int i;
    for (i=1; i<Ddi_VararrayNum(va); i++) {
      Pdtutil_Assert(Ddi_VararrayRead(va,i-1) != Ddi_VararrayRead(va,i),
        "Duplicates in varset");
      Pdtutil_Assert(DdiVarIsAfterVar(Ddi_VararrayRead(va,i),
                                      Ddi_VararrayRead(va,i-1)),
        "Wrong order in varset");
    }
  }

  return (r);
}

/**Function********************************************************************
  Synopsis     [Compute BDD size]
  Description  [Compute BDD size. Sharing size is used, so shared nodes are
    counted only once.]
  SideEffects  []
  SeeAlso      []
******************************************************************************/
int
DdiGenericBddSize (
  Ddi_Generic_t *f    /* BDD to be duplicated */,
  int bound
  )
{
  Ddi_ArrayData_t *roots;
  DdNode **data;
  int size;

  if (f->common.type == Ddi_Bddarray_c &&
      Ddi_BddarrayNum((Ddi_Bddarray_t *)f) == 0) {
      return (0);
  }
  if (f->common.type == Ddi_Bddarray_c &&
      Ddi_BddIsAig(Ddi_BddarrayRead((Ddi_Bddarray_t *)f,0))) {
      return (DdiAigArraySize((Ddi_Bddarray_t *)f));
  }
  if (f->common.code == Ddi_Bdd_Aig_c) {
    if (bound>0) {
      return (bAig_NodeCountWithBound(f->Bdd.data.aig->mgr,
				      f->Bdd.data.aig->aigNode,bound));
    }
    else {
      return (bAig_NodeCount(f->Bdd.data.aig->mgr,
			     f->Bdd.data.aig->aigNode));
    }
  }

  /*
   *  generate an array of monolithic BDDs from CUDD BDD roots
   */
  roots = GenBddRoots(f);
  if ((DdiArrayNum(roots)>0) &&
      Ddi_BddIsAig((Ddi_Bdd_t *)DdiArrayRead(roots,0))) {
    Ddi_Bddarray_t *aigRoots = Ddi_BddarrayMakeFromBddRoots((Ddi_Bdd_t *)f);
    size = DdiAigArraySize(aigRoots);
    Ddi_Free(aigRoots);
  }
  else {
    /*
     *  convert to CUDD array, call CUDD
     */
    data = DdiArrayToCU (roots);
    size = Cudd_SharingSize (data, DdiArrayNum (roots));
    Pdtutil_Free (data);
  }

  DdiArrayFree (roots);

  return(size);
}



/**Function********************************************************************
  Synopsis     [Generate root pointers of leave BDDs for partitioned DDs]
  Description  [Recursively visits a partitioned Dd structure and builds
                the array of leaves bdds. The array is used by functions
                counting BDD nodes, computing support, printing/storing]
  SideEffects  [none]
  SeeAlso      [GenPartRootsRecur]
******************************************************************************/
Ddi_ArrayData_t *
DdiGenBddRoots(
  Ddi_Generic_t  *f
)
{
  return (GenBddRoots(f));
}

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis     [Frees a generic block]
  Description  [Frees a generic block. Internal procedure]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
static void
GenericFreeIntern (
  Ddi_Generic_t *f    /* block to be freed */
)
{
  Ddi_Mgr_t *ddiMgr;  /* dd manager */

  if (f == NULL) {
     /* this may happen with NULL entries in freed arrays */
    return;
  }

  ddiMgr = f->common.mgr;

  Pdtutil_Assert (ddiMgr!=NULL, "DDI with NULL DDI manager");
  Pdtutil_Assert (f->common.status==Ddi_Unlocked_c,
    "Wrong status for freed node. Must be unlocked");

  Ddi_Unlock(f->common.supp);
  DdiGenericFree(f->common.supp);

  switch (f->common.type) {

  case Ddi_Bdd_c:
    switch (f->common.code) {
    case Ddi_Bdd_Mono_c:
      Cudd_RecursiveDeref (ddiMgr->mgrCU, f->Bdd.data.bdd);
      break;
    case Ddi_Bdd_Part_Conj_c:
    case Ddi_Bdd_Part_Disj_c:
        DdiArrayFree(f->Bdd.data.part);
      break;
    case Ddi_Bdd_Meta_c:
        DdiMetaFree(f->Bdd.data.meta);
      break;
    case Ddi_Bdd_Aig_c:
        DdiAigFree(f->Bdd.data.aig);
      break;
    default:
      Pdtutil_Assert (0, "Wrong DDI node type");
    }
    break;
  case Ddi_Var_c:
    if (f->common.code == Ddi_Var_Bdd_c) {
      if (f->Var.data.p.bdd != NULL) {
	Cudd_RecursiveDeref (ddiMgr->mgrCU, f->Var.data.p.bdd);
      }
    }
    else if (f->common.code == Ddi_Var_Aig_c) {
      bAig_RecursiveDeref(ddiMgr->aig.mgr, f->Var.data.p.aig);
    }
    break;
  case Ddi_Varset_c:
    switch (f->common.code) {
    case Ddi_Varset_Bdd_c:
      Cudd_RecursiveDeref (ddiMgr->mgrCU, f->Varset.data.cu.bdd);
      break;
    case Ddi_Varset_Array_c:
      Ddi_Unlock(f->Varset.data.array.va);
      Ddi_Free(f->Varset.data.array.va);
      break;
    default:
      Pdtutil_Assert (0, "Wrong DDI node type");
    }
    break;
  case Ddi_Expr_c:
    switch (f->common.code) {
    case Ddi_Expr_Bdd_c:
      Ddi_Unlock(f->Expr.data.bdd);
      GenericFreeIntern(f->Expr.data.bdd);
      break;
    case Ddi_Expr_String_c:
      Pdtutil_Free(f->Expr.data.string);
      break;
    case Ddi_Expr_Bool_c:
    case Ddi_Expr_Ctl_c:
      DdiArrayFree(f->Expr.data.sub);
      break;
    default:
      Pdtutil_Assert (0, "Wrong DDI node type");
    }
    break;
  case Ddi_Bddarray_c:
  case Ddi_Vararray_c:
  case Ddi_Varsetarray_c:
  case Ddi_Exprarray_c:
    DdiArrayFree(f->Bddarray.array);
    break;
  default:
    Pdtutil_Assert (0, "Wrong DDI node type");
  }

  ddiMgr->genericNum--;
  ddiMgr->freeNum++;
  ddiMgr->typeNum[f->common.type]--;
  f->common.status = Ddi_Free_c;

}

/**Function********************************************************************
  Synopsis     [Duplicate a DDI node]
  Description  []
  SideEffects  []
  SeeAlso      []
******************************************************************************/
static Ddi_Generic_t *
GenericDupIntern (
  Ddi_Generic_t *r    /* destination */,
  Ddi_Generic_t *f    /* source */
  )
{
  Ddi_Mgr_t *ddiMgr;

  ddiMgr = f->common.mgr;

  r->common.type = f->common.type;
  r->common.code = f->common.code;
  r->common.status = Ddi_Unlocked_c;
  r->common.name = Pdtutil_StrDup(f->common.name);
  r->common.supp = DdiGenericDup(f->common.supp);
  r->common.size = f->common.size;
  Ddi_Lock(r->common.supp);

  switch (f->common.type) {

  case Ddi_Bdd_c:
    switch (f->common.code) {
    case Ddi_Bdd_Mono_c:
      r->Bdd.data.bdd = f->Bdd.data.bdd;
      Cudd_Ref(r->Bdd.data.bdd);
      break;
    case Ddi_Bdd_Part_Conj_c:
    case Ddi_Bdd_Part_Disj_c:
      r->Bdd.data.part = DdiArrayDup(f->Bdd.data.part);
      break;
    case Ddi_Bdd_Meta_c:
      r->Bdd.data.meta = DdiMetaDup(f->Bdd.data.meta);
      break;
    case Ddi_Bdd_Aig_c:
      r->Bdd.data.aig = DdiAigDup(f->Bdd.data.aig);
      break;
    default:
      Pdtutil_Assert (0, "Wrong DDI node type");
    }
    break;
  case Ddi_Var_c:
    Pdtutil_Assert (0, "Variables cannot be duplicated");
    break;
  case Ddi_Varset_c:
    switch (f->common.code) {
    case Ddi_Varset_Bdd_c:
      r->Varset.data.cu.bdd = f->Varset.data.cu.bdd;
      Cudd_Ref(r->Varset.data.cu.bdd);
      break;
    case Ddi_Varset_Array_c:
      r->Varset.data.array.va = Ddi_VararrayDup(f->Varset.data.array.va);
      Ddi_Lock(r->Varset.data.array.va);
      break;
    default:
      Pdtutil_Assert (0, "Wrong DDI node type");
    }
    break;
  case Ddi_Expr_c:
    switch (f->common.code) {
    case Ddi_Expr_Bdd_c:
      r->Expr.data.bdd = DdiGenericDup(f->Expr.data.bdd);
      Ddi_Lock(r->Expr.data.bdd);
      break;
    case Ddi_Expr_String_c:
      r->Expr.data.string = Pdtutil_StrDup(f->Expr.data.string);
      break;
    case Ddi_Expr_Bool_c:
    case Ddi_Expr_Ctl_c:
      r->Expr.data.sub = DdiArrayDup(f->Expr.data.sub);
      break;
    default:
      Pdtutil_Assert (0, "Wrong DDI node type");
    }
    r->Expr.opcode = f->Expr.opcode;
    break;
  case Ddi_Bddarray_c:
  case Ddi_Vararray_c:
  case Ddi_Varsetarray_c:
  case Ddi_Exprarray_c:
    r->Bddarray.array = DdiArrayDup(f->Bddarray.array);
    break;
  default:
    Pdtutil_Assert (0, "Wrong DDI node type");
  }

  if (f->common.info != NULL) {
    r->common.info = Pdtutil_Alloc(Ddi_Info_t, 1);
    *r->common.info = *f->common.info;
    if (f->common.info->infoCode == Ddi_Info_Eq_c) {
      r->common.info->eq.vars = Ddi_VararrayDup(f->common.info->eq.vars);
      Ddi_Lock(r->common.info->eq.vars);
      r->common.info->eq.subst = Ddi_BddarrayDup(f->common.info->eq.subst);
      Ddi_Lock(r->common.info->eq.subst);
    }
    if (f->common.info->infoCode == Ddi_Info_Compose_c) {
      r->common.info->compose.f = Ddi_BddDup(f->common.info->compose.f);
      Ddi_Lock(r->common.info->compose.f);
      if (f->common.info->compose.care!=NULL) {
        r->common.info->compose.care = Ddi_BddDup(f->common.info->compose.care);
        Ddi_Lock(r->common.info->compose.care);
      }
      if (f->common.info->compose.constr!=NULL) {
        r->common.info->compose.constr = Ddi_BddDup(f->common.info->compose.constr);
        Ddi_Lock(r->common.info->compose.constr);
      }
      r->common.info->compose.cone = NULL;
      if (f->common.info->compose.cone!=NULL) {
	r->common.info->compose.cone =
	  Ddi_BddDup(f->common.info->compose.cone);
	Ddi_Lock(r->common.info->compose.cone);
      }
      r->common.info->compose.refVars = NULL;
      if (f->common.info->compose.refVars!=NULL) {
	r->common.info->compose.refVars =
	  Ddi_VararrayDup(f->common.info->compose.refVars);
	Ddi_Lock(r->common.info->compose.refVars);
      }
      if (f->common.info->compose.vars!=NULL) {
        r->common.info->compose.vars = 
          Ddi_VararrayDup(f->common.info->compose.vars);
        Ddi_Lock(r->common.info->compose.vars);
      }
      r->common.info->compose.subst = 
        Ddi_BddarrayDup(f->common.info->compose.subst);
      Ddi_Lock(r->common.info->compose.subst);
    }
  }

  return(r);
}


/**Function*******************************************************************
  Synopsis    [Generate support by iterating on array entries]
  SideEffects [Generate support by iterating on array entries.
               Intermediate supports are computed]
  SeeAlso     []
******************************************************************************/
static Ddi_Generic_t *
ArraySupp (
  Ddi_ArrayData_t *array
)
{
  Ddi_Generic_t *supp, *totsupp, *f;
  int i;

  f = DdiArrayRead(array,0);

  totsupp = DdiGenericOp1(Ddi_BddSupp_c,Ddi_Generate_c,f);

  for (i=1;i<DdiArrayNum(array);i++) {
    f = DdiArrayRead(array,i);
    supp = DdiGenericOp1(Ddi_BddSupp_c,Ddi_Generate_c,f);
    if (supp->common.code==Ddi_Varset_Array_c) {
      Ddi_VarsetSetArray((Ddi_Varset_t *)totsupp);
    }
    if (totsupp->common.code==Ddi_Varset_Array_c) {
      Ddi_VarsetSetArray((Ddi_Varset_t *)supp);
    }
    (void *)DdiGenericOp2(Ddi_VarsetUnion_c,Ddi_Accumulate_c,totsupp,supp);
    DdiGenericFree(supp);
  }

  return (totsupp);
}


/**Function*******************************************************************
  Synopsis    [Compute array support with auxiliary array.]
  SideEffects [Compute array support with auxiliary array.
               Aux array has one entry for each manager variable.
               The method avoid creating intermediate supports,
               and it is useful for large BDD arrays]
  SeeAlso     []
******************************************************************************/
static Ddi_Generic_t *
ArraySupp2 (
  Ddi_Generic_t *f
)
{
  Ddi_ArrayData_t *roots,*array;
  DdNode **data;
  DdNode *suppCU;
  Ddi_Mgr_t *ddiMgr;  /* dd manager */
  int nv, i, j;
  char *auxArray;
  Ddi_Generic_t *supp;

  array = f->Bdd.data.part;
  if (DdiArrayRead(array,0)->common.code==Ddi_Bdd_Aig_c) {
    if (f->common.type == Ddi_Bdd_c) {
      Ddi_Bddarray_t *fA = Ddi_BddarrayMakeFromBddPart((Ddi_Bdd_t *)f); 
      Ddi_Varset_t *vs = Ddi_BddarraySupp(fA);
      Ddi_Free(fA);
      return (Ddi_Generic_t *)vs;
    }
    else
    return ArraySupp (array);
  }


  ddiMgr = f->common.mgr;
  nv = Ddi_MgrReadNumVars(ddiMgr);
  auxArray = Pdtutil_Alloc(char,nv);
  for (i=0; i<nv;i++) {
    auxArray[i] = (char)0;
  }

  /*
   *  generate an array of monolithic BDDs from CUDD BDD roots
   */
  roots = GenBddRoots(f);

  /*
   *  convert to CUDD array, call CUDD
   */

  Ddi_MgrAbortOnSiftSuspend(ddiMgr);
  Ddi_MgrSiftSuspend(ddiMgr);

  data = DdiArrayToCU (roots);
  SupportWithAuxArrayCU (data, DdiArrayNum (roots), auxArray);

  suppCU = Ddi_BddToCU(Ddi_MgrReadOne(ddiMgr));
  Cudd_Ref(suppCU);

  for (i=nv-1; i>=0; i--) {
    j = Ddi_MgrReadInvPerm(ddiMgr,i);
    if (auxArray[j]) {
      DdNode *varCU = Cudd_bddIthVar(ddiMgr->mgrCU,j);
      DdNode *tmpCU = Cudd_bddAnd(ddiMgr->mgrCU,suppCU,varCU);
      Cudd_Ref(tmpCU);
      Cudd_RecursiveDeref (ddiMgr->mgrCU, suppCU);
      suppCU = tmpCU;
    }
  }

  Pdtutil_Free (data);
  Pdtutil_Free (auxArray);
  DdiArrayFree (roots);

  supp = (Ddi_Generic_t *) Ddi_VarsetMakeFromCU(ddiMgr,suppCU);
  Cudd_RecursiveDeref (ddiMgr->mgrCU, suppCU);

  Ddi_MgrSiftResume(ddiMgr);
  Ddi_MgrAbortOnSiftResume(ddiMgr);

  return (supp);
}

/**Function*******************************************************************
  Synopsis    [Iterate operation on array entries (accumulate mode used)]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
static void
ArrayOpIterate (
  Ddi_OpCode_e   opcode   /* operation code */,
  Ddi_ArrayData_t *array   /* array */,
  Ddi_Generic_t  *g       /* first operand */,
  Ddi_Generic_t  *h       /* first operand */
)
{
  int i;
  for (i=0;i<DdiArrayNum(array);i++) {
    (void *)DdiGenericOp(opcode,Ddi_Accumulate_c,DdiArrayRead(array,i),g,h);
  }
}


/**Function********************************************************************
  Synopsis     [Generate root pointers of leave BDDs for partitioned DDs]
  Description  [Recursively visits a partitioned Dd structure and builds
                the array of leaves bdds. The array is used by functions
                counting BDD nodes, computing support, printing/storing]
  SideEffects  [none]
  SeeAlso      [GenPartRootsRecur]
******************************************************************************/
static Ddi_ArrayData_t *
GenBddRoots(
  Ddi_Generic_t  *f
)
{
  Ddi_ArrayData_t *roots;

  roots = DdiArrayAlloc(0);
  GenBddRootsRecur(f, roots);

  return (roots);
}


/**Function********************************************************************
  Synopsis     [Recursive step of root pointers generation]
  SideEffects  [none]
  SeeAlso      [GenBddRoots]
******************************************************************************/
static void
GenBddRootsRecur(
  Ddi_Generic_t  *f,
  Ddi_ArrayData_t *roots
)
{
  int i;

  if (f==NULL)
    return;

  switch (f->common.type) {

  case Ddi_Bdd_c:
    switch (f->common.code) {
    case Ddi_Bdd_Mono_c:
    case Ddi_Bdd_Aig_c:
      DdiArrayWrite(roots,DdiArrayNum(roots),f,Ddi_Dup_c);
      break;
    case Ddi_Bdd_Part_Conj_c:
    case Ddi_Bdd_Part_Disj_c:
      for (i=0;i<DdiArrayNum(f->Bdd.data.part);i++) {
        GenBddRootsRecur(DdiArrayRead(f->Bdd.data.part,i),roots);
      }
      break;
    case Ddi_Bdd_Meta_c:
      for (i=0;i<DdiArrayNum(f->Bdd.data.meta->one->array);i++) {
        GenBddRootsRecur(DdiArrayRead(f->Bdd.data.meta->one->array,i),roots);
      }
      for (i=0;i<DdiArrayNum(f->Bdd.data.meta->zero->array);i++) {
        GenBddRootsRecur(DdiArrayRead(f->Bdd.data.meta->zero->array,i),roots);
      }
      for (i=0;i<DdiArrayNum(f->Bdd.data.meta->dc->array);i++) {
        GenBddRootsRecur(DdiArrayRead(f->Bdd.data.meta->dc->array,i),roots);
      }
      break;
    default:
      Pdtutil_Assert (0, "Wrong DDI node type");
    }
    break;
  case Ddi_Var_c:
  case Ddi_Varset_c:
    DdiArrayWrite(roots,DdiArrayNum(roots),f,Ddi_Dup_c);
    break;
  case Ddi_Expr_c:
    switch (f->common.code) {
    case Ddi_Expr_Bdd_c:
      DdiArrayWrite(roots,DdiArrayNum(roots),f,Ddi_Dup_c);
      break;
    case Ddi_Expr_String_c:
      break;
    case Ddi_Expr_Bool_c:
    case Ddi_Expr_Ctl_c:
      for (i=0;i<DdiArrayNum(f->Expr.data.sub);i++) {
      GenBddRootsRecur(DdiArrayRead(f->Expr.data.sub,i),roots);
      }
      break;
    default:
      Pdtutil_Assert (0, "Wrong DDI node type");
    }
    break;
  case Ddi_Bddarray_c:
  case Ddi_Vararray_c:
  case Ddi_Varsetarray_c:
  case Ddi_Exprarray_c:
    for (i=0;i<DdiArrayNum(f->Bddarray.array);i++) {
      GenBddRootsRecur(DdiArrayRead(f->Bddarray.array,i),roots);
    }
    break;
  default:
    Pdtutil_Assert (0, "Wrong DDI node type");

  }

}

/**Function********************************************************************

  Synopsis    [Counts the number of nodes in an array of DDs.]

  Description [Counts the number of nodes in an array of DDs. Shared
  nodes are counted only once.  Returns the total number of nodes.]

  SideEffects [None]

  SeeAlso     [Cudd_DagSize]

******************************************************************************/
static void
SupportWithAuxArrayCU (
  DdNode ** nodeArray,
  int  n,
  char *auxArray
)
{
    int      j;

    for (j = 0; j < n; j++) {
      myDdDagIntForSupport(Cudd_Regular(nodeArray[j]),auxArray);
    }
    for (j = 0; j < n; j++) {
      myClearFlag(Cudd_Regular(nodeArray[j]));
    }

} /* end of Cudd_SharingSize */

/**Function********************************************************************

  Synopsis    [Performs the recursive step of Cudd_DagSize.]

  Description [Performs the recursive step of Cudd_DagSize. Returns the
  number of nodes in the graph rooted at n.]

  SideEffects [None]

******************************************************************************/
static int
myDdDagIntForSupport(
  DdNode * n,
  char *auxArray
)
{
    int tval, eval;

    if (Cudd_IsComplement(n->next)) {
      return(0);
    }
    n->next = Cudd_Not(n->next);
    if (cuddIsConstant(n)) {
      return(1);
    }
    auxArray[n->index] = (char)1;
    tval = myDdDagIntForSupport(cuddT(n),auxArray);
    eval = myDdDagIntForSupport(Cudd_Regular(cuddE(n)),auxArray);
    return(1 + tval + eval);

} /* end of ddDagInt */



/**Function********************************************************************

  Synopsis    [Performs a DFS from f, clearing the LSB of the next
  pointers.]

  Description []

  SideEffects [None]

  SeeAlso     [ddSupportStep ddDagInt]

******************************************************************************/
static void
myClearFlag(
  DdNode * f)
{
    if (!Cudd_IsComplement(f->next)) {
      return;
    }
    /* Clear visited flag. */
    f->next = Cudd_Regular(f->next);
    if (cuddIsConstant(f)) {
      return;
    }
    myClearFlag(cuddT(f));
    myClearFlag(Cudd_Regular(cuddE(f)));
    return;

} /* end of ddClearFlag */


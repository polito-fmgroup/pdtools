/**CFile***********************************************************************

  FileName    [ddiVar.c]

  PackageName [ddi]

  Synopsis    [Functions to manipulate BDD variables]

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
#include "baigInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

// #define VAR_ARRAY_HIGH_BIT_MASK (1<<30)

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


/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Create a variable from existing CU var]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
DdiVarNewFromCU(
  Ddi_BddMgr   *ddm,
  DdNode      *varCU
)
{
  Ddi_Var_t *v;
  int id, cuddId;
  int *cuToIds;
#if 0
  if (varCU->ref == 0)
#endif


    Cudd_Ref(varCU);


  cuddId = Cudd_NodeReadIndex(varCU);
  id = Ddi_VararrayNum(ddm->variables);
  v = (Ddi_Var_t *)DdiGenericAlloc(Ddi_Var_c,ddm);
  v->common.code = Ddi_Var_Bdd_c;
  v->data.p.bdd = varCU;
  v->data.index = id;

  Ddi_VararrayWrite(ddm->variables,id,v);
  Ddi_VararrayWrite(ddm->cuddVariables,cuddId,v);

  cuToIds = Ddi_MgrReadVarIdsFromCudd(ddm);
  Pdtutil_Assert(cuToIds[cuddId]==-1,
    "wrong initialization or duplicate cudd id");
  cuToIds[cuddId] = id;
#if 0
  if (ddm->meta.groups != NULL) {
    int nvar;
    Ddi_Varset_t *metaGrp = 
      Ddi_VarsetarrayRead(ddm->meta.groups,
        Ddi_VarsetarrayNum(ddm->meta.groups)-1);

    Ddi_VarsetAddAcc(metaGrp,v);
    ddm->meta.nvar++;
    nvar = ddm->meta.nvar;
    ddm->meta.ord = Pdtutil_Realloc(int,ddm->meta.ord,nvar);
    ddm->meta.ord[nvar-1] = Ddi_MgrReadInvPerm(ddm,id);
    
    if (Ddi_VarsetNum(metaGroup) > ddm->settings.meta.groupSizeMin) {
    }

  }
#endif

  /* no variable group initially present */     
  Ddi_VarsetarrayWrite(ddm->varGroups,id,NULL);

  DdiMgrCheckVararraySize(ddm);

}

/**Function********************************************************************
  Synopsis    [Create a variable from existing CU var]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
DdiVarSetCU(
  Ddi_Var_t *v
)
{
  Ddi_BddMgr   *ddm = Ddi_ReadMgr(v);
  DdNode      *varCU;
  int id, cuddId;
  int *cuToIds;
  char *name;

  if (Ddi_VarIsBdd(v)) {
    return;
  }

  name = Ddi_VarName(v);

  varCU = Cudd_bddNewVar(ddm->mgrCU);
  Cudd_Ref(varCU);

  id = Ddi_VarIndex(v);
  cuddId = Cudd_NodeReadIndex(varCU);
  v->common.code = Ddi_Var_Bdd_c;

  v->data.p.bdd = varCU;

  Ddi_VararrayWrite(ddm->cuddVariables,cuddId,v);

  cuToIds = Ddi_MgrReadVarIdsFromCudd(ddm);
  Pdtutil_Assert(cuToIds[cuddId]==-1,
    "wrong initialization or duplicate cudd id");
  cuToIds[cuddId] = id;

  /* no variable group initially present */     
  Ddi_VarsetarrayWrite(ddm->varGroups,id,NULL);

  if (name!=NULL) {
    Ddi_VarAttachName (v,name);
  }

  DdiMgrCheckVararraySize(ddm);

}

/**Function********************************************************************
  Synopsis    [Create a variable from existing Baig var]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Var_t *
DdiVarNewFromBaig(
  Ddi_BddMgr   *ddm,
  bAigEdge_t    varBaig
)
{
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  Ddi_Var_t *v;
  int id, baigId;

  bAig_Ref(bmgr, varBaig);

  id = Ddi_VararrayNum(ddm->variables);
  baigId = Ddi_VararrayNum(ddm->baigVariables);
  v = (Ddi_Var_t *)DdiGenericAlloc(Ddi_Var_c,ddm);
  v->common.code = Ddi_Var_Aig_c;
  v->data.p.aig = varBaig;
  v->data.index = id;

  Ddi_VararrayWrite(ddm->baigVariables,baigId,v);
  Ddi_VararrayWrite(ddm->variables,id,v);
  bAig_VarPtr(bmgr, varBaig) = (void *)v;

  DdiMgrCheckVararraySize(ddm);

  return v;
}

/**Function********************************************************************
  Synopsis    [Create a variable from existing Baig var]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
int
DdiVarCompare(
  Ddi_Var_t *v0,
  Ddi_Var_t *v1
)
{
  if (v0==v1) return 0;
  
  if (v1->common.code==Ddi_Var_Aig_c && v0->common.code==Ddi_Var_Bdd_c) 
    return -1;
  else if (v0->common.code==Ddi_Var_Aig_c && v1->common.code==Ddi_Var_Bdd_c) 
    return 1;
  else if (v0->common.code==Ddi_Var_Aig_c)
    return (Ddi_VarIndex(v0)-Ddi_VarIndex(v1));
  else if (v0->common.code==Ddi_Var_Bdd_c)
    return (Ddi_VarCurrPos(v0)-Ddi_VarCurrPos(v1));

  Pdtutil_Assert(0,"wronf selection for var compare");

  return -1;
}

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************
  Synopsis    [Return the variable index (CUDD variable index)]
  SideEffects [none]
  SeeAlso     [Ddi_VarFromIndex]
******************************************************************************/
int
Ddi_VarIndex (
  Ddi_Var_t *var 
)
{
  return (var->data.index);
}


/**Function********************************************************************
  Synopsis    [Return the variable index (CUDD variable index)]
  SideEffects [none]
  SeeAlso     [Ddi_VarFromIndex]
******************************************************************************/
int
Ddi_VarCuddIndex (
  Ddi_Var_t *var 
)
{
  Pdtutil_Assert (Ddi_VarIsBdd(var),"Cudd var required");
  return Cudd_NodeReadIndex(var->data.p.bdd);
}

/**Function********************************************************************
  Synopsis    [Return the variable of a given index]
  SideEffects [none]
  SeeAlso     [Ddi_VarIndex Ddi_VarAtLevel]
******************************************************************************/
Ddi_Var_t *
Ddi_VarFromIndex (
  Ddi_Mgr_t   *ddm,
  int index 
)
{
  return (Ddi_VararrayRead(ddm->variables,index));
}

/**Function********************************************************************
  Synopsis    [Return the variable of a given index]
  SideEffects [none]
  SeeAlso     [Ddi_VarIndex Ddi_VarAtLevel]
******************************************************************************/
Ddi_Var_t *
Ddi_VarFromCuddIndex (
  Ddi_BddMgr   *ddm,
  int index 
)
{
  DdNode *vCU;

  Pdtutil_Assert(index >= 0,"wrong variable index (must be >= 0)");
  Pdtutil_Assert(index < DDI_MAX_CU_VAR_NUM, "wrong variable index (must be < 32000)");
  if (index >= Ddi_MgrReadNumCuddVars(ddm)) {
    vCU = Cudd_bddIthVar(ddm->mgrCU,index);
    DdiVarNewFromCU(ddm,vCU);
  }

  return (Ddi_VararrayRead(ddm->cuddVariables,index));
}

/**Function********************************************************************
  Synopsis    [Return variable at a given level in the order]
  SideEffects [none]
  SeeAlso     [Ddi_VarFromIndex]
******************************************************************************/
Ddi_Var_t *
Ddi_VarAtLevel(
  Ddi_BddMgr   *ddm,
  int          lev
)
{
  return(Ddi_VarFromCuddIndex(ddm,Ddi_MgrReadInvPerm(ddm,lev)));
}

/**Function********************************************************************
  Synopsis    [Create a new variable before (in the variable order)
    the given variable.]
  SideEffects [none]
  SeeAlso     [Ddi_NewVarAtLevel]
******************************************************************************/
Ddi_Var_t *
Ddi_VarNewBeforeVar (
  Ddi_Var_t *var 
)
{
  Ddi_BddMgr *ddMgr;
  int level;

  ddMgr = var->common.mgr;
  level = Ddi_VarCurrPos (var);

  return (Ddi_VarNewAtLevel (ddMgr, level));
}

/**Function********************************************************************
  Synopsis    [Create a new variable after (in the variable order)
    the given variable.]
  SideEffects [none]
  SeeAlso     [Ddi_NewVarAtLevel]
******************************************************************************/
Ddi_Var_t *
Ddi_VarNewAfterVar (
  Ddi_Var_t *var 
)
{
  Ddi_BddMgr *ddMgr;
  int level;

  ddMgr = var->common.mgr;
  level = Ddi_VarCurrPos (var) + 1;

  return (Ddi_VarNewAtLevel (ddMgr, level));
}

/**Function********************************************************************
  Synopsis    [FInd/Create new var (generated within a CUDD/BAIG manager)]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Var_t *
Ddi_VarFindOrAdd(
  Ddi_Mgr_t   *ddm,
  char        *name,
  int          useBddVars 
)
{
  Ddi_Var_t *var;

  var = Ddi_VarFromName (ddm,name);
  if (var!=NULL) return var;

  if (useBddVars) {
    var = Ddi_VarNew (ddm);
    Ddi_VarAttachName(var,name);
  }
  else {
    var = Ddi_VarNewBaig(ddm,name);
  }

  return var;
}

/**Function********************************************************************
  Synopsis    [FInd/Create new var (generated within a CUDD/BAIG manager)]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Var_t *
Ddi_VarFindOrAddAfterVar(
  Ddi_Mgr_t   *ddm,
  char        *name,
  int          useBddVars,
  Ddi_Var_t   *refVar
)
{
  Ddi_Var_t *var;

  if (!useBddVars) {
    return Ddi_VarFindOrAdd (ddm,name,useBddVars);
  }

  var = Ddi_VarFromName (ddm,name);
  if (var!=NULL) return var;

  var = Ddi_VarNewAfterVar (refVar);
  Ddi_VarAttachName(var,name);

  return var;
}

/**Function********************************************************************
  Synopsis    [FInd/Create new var (generated within a CUDD/BAIG manager)]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Var_t *
Ddi_VarFindOrAddBeforeVar(
  Ddi_Mgr_t   *ddm,
  char        *name,
  int          useBddVars,
  Ddi_Var_t   *refVar
)
{
  Ddi_Var_t *var;

  if (!useBddVars) {
    return Ddi_VarFindOrAdd (ddm,name,useBddVars);
  }

  var = Ddi_VarFromName (ddm,name);
  if (var!=NULL) return var;

  var = Ddi_VarNewBeforeVar (refVar);
  Ddi_VarAttachName(var,name);

  return var;
}

/**Function********************************************************************
  Synopsis    [Create a new variable (generated within a CUDD manager)]
  SideEffects [none]
  SeeAlso     [Ddi_VarNewAtLevel Ddi_VarFromCU]
******************************************************************************/
Ddi_Var_t *
Ddi_VarNewBaig(
  Ddi_BddMgr   *ddm,
  char *name
)
{
  bAigEdge_t varBaig;
  Ddi_Var_t *v;

  varBaig = bAig_CreateVarNode(ddm->aig.mgr,name);
  v = DdiVarNewFromBaig(ddm,varBaig);
  Ddi_VarAttachName (v, name);

  return (v);
}

/**Function********************************************************************
  Synopsis    [Create a new variable (generated within a CUDD manager)]
  SideEffects [none]
  SeeAlso     [Ddi_VarNewAtLevel Ddi_VarFromCU]
******************************************************************************/
Ddi_Var_t *
Ddi_VarNew(
  Ddi_BddMgr   *ddm
)
{
  DdNode *varCU;

  varCU = Cudd_bddNewVar(ddm->mgrCU);
  DdiVarNewFromCU(ddm,varCU);

  return(Ddi_VarFromCuddIndex(ddm,Cudd_NodeReadIndex(varCU)));
}


/**Function********************************************************************
  Synopsis    [Returns a new variable at a given level in the order]
  SideEffects [none]
  SeeAlso     [Ddi_VarNew]
******************************************************************************/
Ddi_Var_t *
Ddi_VarNewAtLevel(
  Ddi_BddMgr   *ddm,
  int          lev
)
{
  DdNode *varCU;

  varCU = Cudd_bddNewVarAtLevel(ddm->mgrCU,lev);
  DdiVarNewFromCU(ddm,varCU);

  return(Ddi_VarFromCuddIndex(ddm,Cudd_NodeReadIndex(varCU)));
}


/**Function********************************************************************
  Synopsis    [Return current position of var in variable order]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
int
Ddi_VarCurrPos (
  Ddi_Var_t *var
)
{
  return Ddi_MgrReadPerm(var->common.mgr,Ddi_VarCuddIndex(var));
}

/**Function********************************************************************
  Synopsis    [Return the name of a variable]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
char *
Ddi_VarName (
  Ddi_Var_t *var 
)
{
  return (var->common.name);
}

/**Function********************************************************************
  Synopsis    [Attach a given name to the variable]
  SideEffects [none]
  SeeAlso     [Ddi_VarDetachName ]
******************************************************************************/
void
Ddi_VarAttachName (
  Ddi_Var_t *var,
  char *name
)
{
  int index;
  char **varnames;
  Ddi_Mgr_t *ddm;
  char *key;
  int idx;
  char *freeName=NULL;

  ddm = var->common.mgr;
  
  if (var->common.name != NULL) {
    freeName = var->common.name;
  } 
  var->common.name = Pdtutil_StrDup(name);
  if (freeName) Pdtutil_Free(freeName);

  DdiMgrCheckVararraySize(ddm);

  varnames=Ddi_MgrReadVarnames(ddm);
  Pdtutil_Assert(varnames!=NULL, "null varnames array in dd manager");

  index = Ddi_VarIndex(var);
#if 0
  if (varnames[index]!=NULL) {
    key = varnames[index];
    Pdtutil_Assert(st_lookup_int(ddm->varNamesIdxHash, 
				 (char *)key, &idx), 
     "variable name not found and it should have been, misaligned hash table");
    st_delete(ddm->varNamesIdxHash, (char **) &key, (char *) &idx);
    Pdtutil_Free(varnames[index]);
  }
  varnames[index] = Pdtutil_StrDup(name);
  key = varnames[index];
#else
  key = var->common.name;
  if (var->common.code == Ddi_Var_Bdd_c) {
    if (varnames[index]!=NULL) {
      freeName = varnames[index];
    }
    varnames[index] = Pdtutil_StrDup(name);
    if (freeName) Pdtutil_Free(freeName);
  }
  else {
    //    index |= VAR_ARRAY_HIGH_BIT_MASK;
  }
#endif
  Pdtutil_Assert(!st_lookup_int(ddm->varNamesIdxHash, 
				(char *) key, &idx),
     "variable name found and it should not have been, misaligned hash table");
  st_insert(ddm->varNamesIdxHash, (char *) key, (char *) index);

  if (ddm->aig.mgr != NULL) {
    bAig_CreateVarNode (ddm->aig.mgr,name);
  }


}

/**Function********************************************************************
  Synopsis    [Clear the name of a variable]
  SideEffects [none]
  SeeAlso     [Ddi_VarAttachName]
******************************************************************************/
void
Ddi_VarDetachName (
  Ddi_Var_t *var
)
{
  int index;
  char **varnames;
  Ddi_Mgr_t *ddm;
  char *key;
  int idx;

  ddm = var->common.mgr;

  index = Ddi_VarIndex(var);
  key = var->common.name;
  if (key != NULL) {
    Pdtutil_Assert(st_lookup(ddm->varNamesIdxHash, 
			     (char *) key, (char *) &idx), 
	 "variable name not found and it should be, misaligned hash table");
    st_delete(ddm->varNamesIdxHash, (char **) &key, (char *) &idx);
    Pdtutil_Free (var->common.name);
  }
  var->common.name = NULL;

  return;
}

/**Function********************************************************************
  Synopsis    [Return the variable index (CUDD variable index)]
  SideEffects [none]
  SeeAlso     [Ddi_VarFromIndex]
******************************************************************************/
int
Ddi_VarReadMark (
  Ddi_Var_t *v 
)
{
  Pdtutil_Assert(v->common.info != NULL,"var info required");
  return (v->common.info->var.mark);
}

/**Function********************************************************************
  Synopsis    [Return the variable index (CUDD variable index)]
  SideEffects [none]
  SeeAlso     [Ddi_VarFromIndex]
******************************************************************************/
void
Ddi_VarWriteMark (
  Ddi_Var_t *v,
  int val
)
{
  Pdtutil_Assert(v->common.info != NULL,"var info required");
  v->common.info->infoCode = Ddi_Info_Var_c;
  v->common.info->var.mark = val;
}

/**Function********************************************************************
  Synopsis    [Return the variable index (CUDD variable index)]
  SideEffects [none]
  SeeAlso     [Ddi_VarFromIndex]
******************************************************************************/
void
Ddi_VarIncrMark (
  Ddi_Var_t *v,
  int val
)
{
  Pdtutil_Assert(v->common.info != NULL,"var info required");
  v->common.info->infoCode = Ddi_Info_Var_c;
  v->common.info->var.mark += val;
}


/**Function********************************************************************
  Synopsis    [Return the variable auxid (-1 if auxids not defined)]
  SideEffects [none]
  SeeAlso     [Ddi_VarName]
******************************************************************************/
int
Ddi_VarAuxid (
  Ddi_Var_t *var
)
{
  Ddi_BddMgr   *ddm;
  int index, *varauxids;

  ddm = var->common.mgr;
  index = Ddi_VarIndex(var);

  varauxids=Ddi_MgrReadVarauxids(ddm);
  Pdtutil_Assert(varauxids!=NULL, "null varauxids array in dd manager");

  return (varauxids[index]);
}

/**Function********************************************************************
  Synopsis    [Set the variable auxid of a variable]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Ddi_VarAttachAuxid (
  Ddi_Var_t *var,
  int auxid
)
{
  Ddi_BddMgr   *ddm;
  int index, *varauxids;

  ddm = var->common.mgr;
  index = Ddi_VarIndex(var);

  varauxids=Ddi_MgrReadVarauxids(ddm);
  Pdtutil_Assert(varauxids!=NULL, "null varauxids array in dd manager");

  Pdtutil_Assert(varauxids[index]==-1,"setting auxid to a variable twice");
  varauxids[index] = auxid;
}

/**Function********************************************************************
  Synopsis    [Return the variable auxid (-1 if auxids not defined)]
  SideEffects [none]
  SeeAlso     [Ddi_VarName]
******************************************************************************/
bAigEdge_t
Ddi_VarBaigId (
  Ddi_Var_t *var
)
{
  Ddi_BddMgr   *ddm = var->common.mgr;
  int index, *varBaigIds;
  bAig_Manager_t *bmgr = ddm->aig.mgr;

  Pdtutil_Assert(bmgr!=NULL,"baig manager required for var baig id");

  index = Ddi_VarIndex(var);

  varBaigIds=Ddi_MgrReadVarBaigIds(ddm);
  Pdtutil_Assert(varBaigIds!=NULL, "null varauxids array in dd manager");
  if (varBaigIds[index]==bAig_NULL) {
    bAigEdge_t varIndex = bAig_VarNodeFromName(bmgr,Ddi_VarName(var));
    varBaigIds[index] = varIndex;
  }

  return (varBaigIds[index]);
}

/**Function********************************************************************
  Synopsis    [Set the variable auxid of a variable]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Ddi_VarAttachBaigId (
  Ddi_Var_t *var,
  int baigId
)
{
  Ddi_BddMgr   *ddm;
  int index, *varBaigIds;

  ddm = var->common.mgr;
  index = Ddi_VarIndex(var);

  varBaigIds=Ddi_MgrReadVarBaigIds(ddm);
  Pdtutil_Assert(varBaigIds!=NULL, "null varauxids array in dd manager");

  Pdtutil_Assert(varBaigIds[index]==bAig_NULL,"setting baig id to a variable twice");
  varBaigIds[index] = baigId;
}

/**Function********************************************************************
  Synopsis    [Search a variable given the name]
  Description [Still a linear search !] 
  SideEffects [none]
  SeeAlso     [Ddi_VarFromAuxid]
******************************************************************************/
Ddi_Var_t *
Ddi_VarFromName (
  Ddi_BddMgr *ddm,
  char *name
)
{
  int i;
  char *key;
  char **varnames=Ddi_MgrReadVarnames(ddm);
  Pdtutil_Assert(varnames!=NULL, "null varnames array in dd manager");

  if (name == NULL) {
    return (NULL);
  }
  
  if (st_lookup_int(ddm->varNamesIdxHash, (char *) name, &i)){
#if 0
    if (i & VAR_ARRAY_HIGH_BIT_MASK) {
      i &= ~VAR_ARRAY_HIGH_BIT_MASK;
      return Ddi_VararrayRead(ddm->baigVariables,i);
    }
    else 
#endif
    {
      return (Ddi_VarFromIndex(ddm,i));
    }
  }

  return (NULL);
}

/**Function********************************************************************
  Synopsis    [Search a variable given the auxid]
  Description [Still a linear search !] 
  SideEffects [none]
  SeeAlso     [Ddi_VArFromName]
******************************************************************************/
Ddi_Var_t *
Ddi_VarFromAuxid (
  Ddi_BddMgr   *ddm,
  int auxid
)
{
  int i;
  int *varauxids=Ddi_MgrReadVarauxids(ddm);
  Pdtutil_Assert(varauxids!=NULL, "null varauxids array in dd manager");

  for (i=0;i<Ddi_MgrReadSize(ddm);i++) {
    if (varauxids[i]==auxid)
      return (Ddi_VarFromIndex(ddm,i));
  }
					 
  return (NULL);
}

/**Function********************************************************************
  Synopsis    [Return the CUDD bdd node of a variable]
  Description [Return the CUDD bdd node of a variable]
  SideEffects [none]
******************************************************************************/
DdNode *
Ddi_VarToCU(
  Ddi_Var_t * v
)
{
  return (v->data.p.bdd);
}

/**Function********************************************************************
  Synopsis    [Return the CUDD bdd node of a variable]
  Description [Return the CUDD bdd node of a variable]
  SideEffects [none]
******************************************************************************/
bAigEdge_t 
Ddi_VarToBaig(
  Ddi_Var_t * v
)
{
  if (v->common.code == Ddi_Var_Aig_c)
    return (v->data.p.aig);
  else {
    bAigEdge_t b;
    bAig_Manager_t *bmgr = v->common.mgr->aig.mgr;
    Pdtutil_Assert(bmgr!=NULL,"Baig manager required");
    b = bAig_VarNodeFromName(bmgr,Ddi_VarName(v));
    if (b==bAig_NULL) {
      Ddi_BddMgr   *ddm = Ddi_ReadMgr(v);
      bAig_Manager_t *bmgr = ddm->aig.mgr;
      char *name = Ddi_VarName(v);
      if (bmgr!=NULL && name != NULL) {
	b = bAig_CreateVarNode(bmgr,name);
      }
    }
    return (b);
  }
}

/**Function********************************************************************
  Synopsis    [Convert a CUDD variable to a DDI variable]
  Description [Convert a CUDD variable to a DDI variable]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Var_t *
Ddi_VarFromCU (
  Ddi_BddMgr   *ddm,
  DdNode *  v
)
{
  return(Ddi_VarFromCuddIndex(ddm,Cudd_NodeReadIndex(v)));
}

/**Function********************************************************************
  Synopsis    [Convert a CUDD variable to a DDI variable]
  Description [Convert a CUDD variable to a DDI variable]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Var_t *
Ddi_VarFromBaig (
  Ddi_BddMgr   *ddm,
  bAigEdge_t bv
)
{
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  Ddi_Var_t *v = (Ddi_Var_t *) bAig_VarPtr(bmgr, bv);
  char *name;

  if (v != NULL) return v;
  name = bAig_NodeReadName(bmgr,bv);
  return (Ddi_VarFromName(ddm,name));
}

/**Function********************************************************************
  Synopsis     [Copy a variable to a destination dd manager]
  Description  [Find the variable corresponding to v in the destination
               manager. Variable correspondence is for now limited to
               index matching.] 
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
Ddi_Var_t *
Ddi_VarCopy(
  Ddi_BddMgr *dd2 /* destination manager */,
  Ddi_Var_t *v    /* variable to be copied */ 
)
{
  Ddi_Var_t *v2;
  int id;

  if (v==NULL) return NULL;
  DdiConsistencyCheck(v,Ddi_Var_c);

  v2 = (Ddi_Var_t *)DdiGenericCopy(dd2,(Ddi_Generic_t *)v,NULL,NULL);

  return (v2);
}


/**Function********************************************************************
  Synopsis    [Create a variable group]
  Description [A group of variables is created for group sifting. 
               The group starts at v and contains grpSize variables
               (following v in the ordering. Sifting is allowed within the
               group]
  SideEffects [none]
  SeeAlso     [Ddi_VarMakeGroupFixed]
******************************************************************************/
void
Ddi_VarMakeGroup(
  Ddi_BddMgr   *dd,
  Ddi_Var_t   *v,
  int grpSize
)
{
  DdiMgrMakeVarGroup(dd,v,grpSize,MTR_DEFAULT);
}

/**Function********************************************************************
  Synopsis    [Create a variable group with fixed inner order]
  Description [Same as Ddi_VarMakeGroup but no dynamic reordering allowed
               within group]
  SideEffects [none]
  SeeAlso     [Ddi_VarMakeGroupFixed]
******************************************************************************/
void
Ddi_VarMakeGroupFixed(
  Ddi_BddMgr   *dd,
  Ddi_Var_t   *v,
  int grpSize
)
{
  DdiMgrMakeVarGroup(dd,v,grpSize,MTR_FIXED);
}

/**Function********************************************************************
  Synopsis    [Return variable group including v. NULL if v is not in a group]
  Description [Return variable group including v. NULL if v is not in a group]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Varset_t *
Ddi_VarReadGroup(
  Ddi_Var_t   *v
)
{
  int i;
  Ddi_Mgr_t *ddm;

  ddm = Ddi_ReadMgr(v);
  i = Ddi_VarCuddIndex(v);

  return(Ddi_VarsetarrayRead(ddm->varGroups,i));
}

/**Function********************************************************************
  Synopsis    [Return true (non 0) if variable is in variable group]
  Description [Return true (non 0) if variable is in variable group]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
int
Ddi_VarIsGrouped(
  Ddi_Var_t   *v
)
{
  return(Ddi_VarReadGroup(v)!=NULL);
}

/**Function********************************************************************
  Synopsis    [Return true (non 0) if var is CU Bdd]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/

int
Ddi_VarIsBdd (
  Ddi_Var_t *var
)
{
  return (var->common.code == Ddi_Var_Bdd_c);
}


/**Function********************************************************************
  Synopsis    [Return true (non 0) if var is Aig]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/

int
Ddi_VarIsAig (
  Ddi_Var_t *var
)
{
  return (var->common.code == Ddi_Var_Aig_c);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
Ddi_Var_t *
Ddi_VarFindRefVar (
  Ddi_Var_t *v,
  char *prefix,
  char tailSeparator
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(v);
  char buf[200], *name = Ddi_VarName(v);
  int len = strlen(name), plen = strlen(prefix);
  int k;

  Pdtutil_Assert(len<199,"name is too long");

  if (strncmp(name,prefix,plen)!=0) {
    return NULL;
  }

  strcpy(buf,name+plen);
  for (k=strlen(buf)-1; buf[k]!=tailSeparator; k--);
  buf[k]='\0';

  return Ddi_VarFromName(ddm, buf);

}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/






/**CFile****************************************************************

  FileName    [ddiAiger.c]

  PackageName [ddi]

  Synopsis    [Aig interface with Aiger package]

  Description []

  SeeAlso     []

  Author      [Gianpiero Cabodi]

  Affiliation []

  Date        []

  Copyright [This file was created at the Politecnico di Torino,
    Torino, Italy.
    The  Politecnico di Torino makes no warranty about the suitability of
    this software for any purpose.
    It is presented on an AS IS basis.
  ]

  Revision  []

***********************************************************************/

#include "aiger.h"
#include "ddi.h"
#include "baigInt.h"


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

aiger *BddarrayCopyToAiger (Ddi_Bddarray_t *fA, int badOrOutput);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis           []

  Description        []

  SideEffects        []

  SeeAlso            []

******************************************************************************/
aiger*
BddarrayCopyToAiger (
  Ddi_Bddarray_t *fA,   
  int badOrOutput
  )
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(fA);
  bAig_Manager_t  *bmgr = Ddi_MgrReadMgrBaig(ddm);
  int i;
  int m, ni, nl, no, na, nb, nc, ntotv;

  Ddi_Vararray_t *vars;
  Ddi_Var_t *v;
  int *auxids, flagError;
  char **names;
  char *nameext;

  aiger *mgr;
  Ddi_Bdd_t *litBdd;
  char buf[100];
  int sortDfs = 1;
  Ddi_Bdd_t *bddZero = NULL;
  bAig_array_t *visitedNodes;
  Ddi_Bdd_t *f;
  unsigned iAnd;

  mgr = aiger_init();

  // build aig nodes array with proper node numbering
  visitedNodes = bAigArrayAlloc();

  for (i=0; i<Ddi_BddarrayNum(fA); i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    Ddi_PostOrderAigVisitIntern(bmgr,Ddi_BddToBaig(f_i),visitedNodes,-1);
  }
  Ddi_PostOrderAigClearVisitedIntern(bmgr,visitedNodes);

  vars = Ddi_BddarraySuppVararray(fA); 
  ni = Ddi_VararrayNum(vars); 
  for (i=0; i<ni; i++) {
    int index, i1 = i+1;
    Ddi_Var_t *var;
    char *s, *name; 
    bAigEdge_t varIndex;
    unsigned lit=2*i1;
    var = Ddi_VararrayRead (vars, i);
    index = Ddi_VarIndex(var);
    name = Ddi_VarName (var);
    if (name==NULL) {
      sprintf(buf, "i%d", lit);
      name = buf;
    }
    aiger_add_input (mgr, lit, name);
    varIndex = Ddi_VarToBaig(var);
    bAig_AuxInt(bmgr,varIndex) = i1;
  }
  
  iAnd = ni+1;

  for (i=0; i<visitedNodes->num; i++) {
    bAigEdge_t baig, right, left;
    unsigned rhs0, rhs1, lhs;

    baig = visitedNodes->nodes[i];
    if (!bAig_NodeIsConstant(baig) && !bAig_isVarNode(bmgr,baig)) {
      bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
      bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);
      unsigned ir = bAig_AuxInt(bmgr,right);
      unsigned il = bAig_AuxInt(bmgr,left);
      bAig_AuxInt(bmgr,baig) = iAnd;
      lhs = iAnd*2;
      rhs0 = bAig_NodeIsInverted(right) ? 2*ir+1 : 2*ir; 
      rhs1 = bAig_NodeIsInverted(left) ? 2*il+1 : 2*il; 
      aiger_add_and (mgr, lhs, rhs0, rhs1);
      iAnd++;
    }
  }

  for (i=0; i<Ddi_BddarrayNum(fA); i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    bAigEdge_t baig = Ddi_BddToBaig(f_i);
    unsigned lit = bAig_NodeIsConstant(baig) ? 
      aiger_false : bAig_AuxInt(bmgr,baig);
    lit*=2;
    if (bAig_NodeIsInverted(baig)) lit++;
    if (badOrOutput)
      aiger_add_bad (mgr, lit, Ddi_ReadName (f_i));
    else
      aiger_add_output (mgr, lit, Ddi_ReadName (f_i));
  }
  
  for (i=0; i<ni; i++) {
    Ddi_Var_t *var = Ddi_VararrayRead (vars, i);
    bAigEdge_t varIndex = Ddi_VarToBaig(var);
    bAig_AuxInt(bmgr,varIndex) = -1;
  }
  for (i=0; i<visitedNodes->num; i++) {
    bAigEdge_t baig = visitedNodes->nodes[i];
    if (!bAig_NodeIsConstant(baig) && !bAig_isVarNode(bmgr,baig)) {
      bAig_AuxInt(bmgr,baig) = -1;
    }
  }

  bAigArrayFree(visitedNodes);
  Ddi_Free(vars);

  return mgr;
}


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis           []

  Description        []

  SideEffects        []

  SeeAlso            []

******************************************************************************/
Ddi_Bdd_t *
Ddi_AigNetLoadAiger(
  Ddi_Mgr_t *dd,
  Ddi_Vararray_t *vars,
  char *filename                 /* IN: file name */
)
{
  Ddi_Bdd_t *f;
  Ddi_Bddarray_t *fA = Ddi_AigarrayNetLoadAiger(dd,vars,filename);
  if (fA==NULL) return NULL;
  Pdtutil_Warning(Ddi_BddarrayNum(fA)>1,"aiger file has more than 1 output");
  f = Ddi_BddDup(Ddi_BddarrayRead(fA,0));
  Ddi_Free(fA);
  return f;
}

/**Function********************************************************************

  Synopsis           []

  Description        []

  SideEffects        []

  SeeAlso            []

******************************************************************************/
Ddi_Bddarray_t *
Ddi_AigarrayNetLoadAiger(
  Ddi_Mgr_t *dd,
  Ddi_Vararray_t *vars,
  char *filename                 /* IN: file name */
)
{
  int i;
  int m, ni, nl, no, na, nb, nc, ntotv;
  Ddi_Var_t *v;
  Ddi_Bddarray_t *fA=NULL;
  int *auxids, flagError;
  char **names;
  char *nameext;
  long loadTime;

  aiger *mgr;
  const char *error;
  Ddi_Bddarray_t *nodesArray;
  Ddi_Bdd_t *litBdd;
  char buf[100];
  int sortDfs = 1;
  Ddi_Bdd_t *bddZero = NULL;
  aiger_symbol *targets;
  aiger_symbol *constraints;

  loadTime = util_cpu_time();

  /*------------------------ Check For FSM Structure ------------------------*/

  if (Ddi_MgrReadMgrBaig(dd)==NULL) {
    Ddi_AigInit(dd,100);
  }

  /*-------------------------- Read AIGER File  --------------------------*/

  mgr = aiger_init ();
  error = aiger_open_and_read_from_file (mgr, filename);

  /*--------------- Check for Errors and Perform Final Copies ---------------*/

  if (error!=NULL) {
    fprintf(stderr,"Error Loading AIGER file %s - error %s.\n", 
	    filename, error);
    aiger_reset (mgr);
    return 1;
  }


  /*--------------- Setup data structures   ---------------*/

  m = mgr->maxvar;
  ni = mgr->num_inputs;
  nl = mgr->num_latches;
  no = mgr->num_outputs;
  nb = mgr->num_bad;
  na = mgr->num_ands;
  nc = mgr->num_constraints;

  Pdtutil_Assert(nl==0, "0 latches needed for combinational aiger");
  //  Pdtutil_Assert(nb==0, "0 bads needed for combinational aiger");
  Pdtutil_Assert(nc>=0, "0 constraints needed for combinational aiger");

  nodesArray = Ddi_BddarrayAlloc(dd,m+1);
  fA = Ddi_BddarrayAlloc(dd,no+nc);
  bddZero = Ddi_BddMakeConstAig(dd,0);
  Ddi_BddarrayWrite(nodesArray,0,bddZero);
  Ddi_Free(bddZero);

  targets = mgr->outputs;
  constraints = mgr->constraints;
  if (nb>0) {
    /* ignore outputs - read targets from bad array */
    no = nb;
    targets = mgr->bad;
  }

  /*--------------- Setup input and latch variables  ---------------*/

  ntotv = ni;


  /*------------------ Translate Array of Input Variables -------------------*/

  /* GpC: force void input array when no input found */

  for (i=0; i<ni; i++) {
    unsigned lit = mgr->inputs[i].lit;
    int index, i1 = i+1;
    Ddi_Var_t *var;
    char *s, *name = aiger_get_symbol (mgr, lit);

    if (name == NULL) {
      sprintf(buf, "i%d", lit);
      name = buf;
    }

    Pdtutil_Assert(lit==2*i1,"Wrong variable id in AIGER manager");

    for (s=name; *s!='\0'; s++) {
      if (*s=='#' || *s==':' || *s=='(' || *s==')') *s='_';
    }

    var = Ddi_VarFromName(dd,name);
    if (var==NULL) {
      var = Ddi_VarNew (dd);
      Ddi_VarAttachName(var,name);
    }

    if (vars!=NULL) Ddi_VararrayWrite (vars, i, var);

    litBdd = Ddi_BddMakeLiteralAig(var,1);
    Ddi_BddarrayWrite(nodesArray,i+1,litBdd);
    Ddi_Free(litBdd);
  }


  /*---------------- Translate Array of And Gates  ---------------------*/

  for (i = 0; i < mgr->num_ands; i++) {
    aiger_and *n = &(mgr->ands[i]);
    unsigned rhs0 = n->rhs0;
    unsigned rhs1 = n->rhs1;
    unsigned lit = n->lhs;

    Ddi_Bdd_t *r0, *r1, *r;

    Pdtutil_Assert(lit==2*(i+ni+1),"Wrong node id in AIGER manager");
    Pdtutil_Assert(rhs0<lit&&rhs1<lit,"Wrong node order in AIGER manager");

    r0 = Ddi_BddDup(Ddi_BddarrayRead(nodesArray,rhs0/2));
    r1 = Ddi_BddDup(Ddi_BddarrayRead(nodesArray,rhs1/2));
    if (aiger_sign(rhs0)) Ddi_BddNotAcc(r0);
    if (aiger_sign(rhs1)) Ddi_BddNotAcc(r1);

    r = Ddi_BddAnd(r0,r1);
    Ddi_BddarrayWrite(nodesArray,ni+i+1,r);
    Ddi_Free(r);
    Ddi_Free(r0);
    Ddi_Free(r1);
  }

  /*---------------- Translate Array of Output Function ---------------------*/

  for (i=0;i<no;i++) {
    unsigned lit = targets[i].lit;
    int index, i1=i+1;
    char *name=targets[i].name;
    Ddi_Bdd_t *oBdd;

    oBdd = Ddi_BddDup(Ddi_BddarrayRead(nodesArray,lit/2));
    if (aiger_sign(lit)) Ddi_BddNotAcc(oBdd);

    if (name == NULL) {
      sprintf(buf, "o%d", 2*i1);
    }

    Ddi_BddarrayWrite (fA, i, oBdd);
    Ddi_SetName(Ddi_BddarrayRead(fA,i),name);

    Ddi_Free(oBdd);
  }

  /*---------------- Translate Array of Constr Function ---------------------*/

  for (i=0;i<nc;i++) {
    unsigned lit = constraints[i].lit;
    int index, i1=i+1;
    char *name=constraints[i].name;
    Ddi_Bdd_t *oBdd;

    oBdd = Ddi_BddDup(Ddi_BddarrayRead(nodesArray,lit/2));
    if (aiger_sign(lit)) Ddi_BddNotAcc(oBdd);

    if (name == NULL) {
      sprintf(buf, "o%d", 2*i1);
    }

    Ddi_BddarrayWrite (fA, i+no, oBdd);
    Ddi_SetName(Ddi_BddarrayRead(fA,i+no),name);

    Ddi_Free(oBdd);
  }

  Ddi_Free(nodesArray);
  aiger_reset (mgr);

  loadTime = util_cpu_time() - loadTime;

  return (fA);
}



/**Function********************************************************************

  Synopsis           []

  Description        []

  SideEffects        []

  SeeAlso            []

******************************************************************************/
int
Ddi_AigarrayNetStoreAiger(
  Ddi_Bddarray_t *fA,   
  int badOrOutput,
  char *filename                 /* OUT: file name */
)
{
  aiger* mgr;

  mgr = BddarrayCopyToAiger (fA,badOrOutput);
  
  if (!aiger_open_and_write_to_file (mgr,filename))
      return (1);	

  aiger_reset (mgr);

  return (0);
}


/**Function********************************************************************

  Synopsis           []

  Description        []

  SideEffects        []

  SeeAlso            []

******************************************************************************/
int
Ddi_AigNetStoreAiger(
  Ddi_Bdd_t *f,   
  int badOrOutput,
  char *filename                 /* OUT: file name */
)
{
  int ret;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  Ddi_Bddarray_t *fA = Ddi_BddarrayAlloc(ddm,1);
  Ddi_BddarrayWrite(fA,0,f);
  ret = Ddi_AigarrayNetStoreAiger(fA,badOrOutput,filename);
  Ddi_Free(fA);

  return ret;
}

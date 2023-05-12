/**CFile***********************************************************************

  FileName    [ddiSat.c]

  PackageName [ddi]

  Synopsis    [Functions working on top of SAT solvers]

  Description [Functions working on top of SAT solvers]

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

#include "ddiInt.h"
#include "baigInt.h"

#include "Solver.h"
#include "Proof.h"
#include "File.h"

#include <time.h>
#include <stdlib.h>

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

static Ddi_Clause_t *chkClause=NULL;

static int clauseNum = 0;
static int iteNum = 0;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#define dMgrO(ddiMgr) ((ddiMgr)->settings.stdout)

#define rightChildAuxInt(bmgr,baig) \
  bAig_AuxInt((bmgr),bAig_NodeReadIndexOfRightChild((bmgr),(baig)))
#define leftChildAuxInt(bmgr,baig) \
  bAig_AuxInt((bmgr),bAig_NodeReadIndexOfLeftChild((bmgr),(baig)))

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static int ClauseLitCompare(const void *l0, const void *l1);
static int ClauseCompare(const void *l0, const void *l1);
static int tSimStackPush(Ddi_TernarySimMgr_t *xMgr, int gId);
static int tSimStackFoPush(Ddi_TernarySimMgr_t *xMgr, int gId);
static int tSimStackPop(Ddi_TernarySimMgr_t *xMgr);
static void tSimStackReset(Ddi_TernarySimMgr_t *xMgr);
static Ddi_Clause_t *eqClassesSplitClauses(Pdtutil_EqClasses_t *eqCl, Ddi_SatSolver_t *solver);
static int findIteOrXor(bAig_Manager_t *bmgr, bAig_array_t *visitedNodes, Ddi_AigCnfInfo_t *aigCnfInfo, bAigEdge_t baig, bAigEdge_t right, bAigEdge_t left, int i, int ir, int il);
static Ddi_ClauseArray_t *aigClausesIntern(Ddi_Bdd_t *f, int makeRel, Ddi_Clause_t *assumeCl, Ddi_ClauseArray_t *justify);
static Ddi_Bdd_t *aigInterpolantByGenClausesIntern(Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Bdd_t *care, Ddi_Bdd_t *careA, Ddi_Vararray_t *ps, Ddi_Vararray_t *ns, Ddi_Bdd_t *init, Ddi_Vararray_t *globVars, Ddi_Vararray_t *dynAbstrCut, Ddi_Vararray_t *dynAbstrAux, Ddi_Bddarray_t *dynAbstrCutLits, int maxIter, int randomCubes, int useInduction, int *result);
static Ddi_SatSolver_t *
Ddi_SatSolverAllocIntern(
  int useMinisat22
);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_ClauseAddLiteral(
  Ddi_Clause_t *cl,
  int lit
)
{
  DdiClauseAddLiteral(cl,lit);
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiClauseAddLiteral(
  Ddi_Clause_t *cl,
  int lit
)
{
  unsigned int mask = (1 << (abs(lit)%31));

  if (cl->nLits>=cl->size) {
    cl->size *= 2;
    cl->lits = Pdtutil_Realloc(int,cl->lits,cl->size);
  }
  if (cl->nLits>0 && (abs(lit)<abs(cl->lits[cl->nLits-1]))) {
    cl->sorted = 0;
  }

  cl->lits[cl->nLits++] = lit;

  cl->signature0 |= mask;
  if (lit<0) {
    cl->signature1 |= mask;
  }
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiClauseRemoveLiteral(
  Ddi_Clause_t *cl,
  int i
)
{
  int j, k;
  int lit;
  unsigned int mask;
  Pdtutil_Assert(i>=0&&i<cl->nLits,"invalid literal id");

  cl->signature0 = cl->signature1 = 0;
  for (j=k=0; j<cl->nLits; j++) {
    if (j==i) continue;
    lit = cl->lits[j];
    cl->lits[k++] = lit;
    mask = (1 << (abs(lit)%31));
    cl->signature0 |= mask;
    if (lit<0) {
      cl->signature1 |= mask;
    }
  }

  cl->nLits--;
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiClauseClear(
  Ddi_Clause_t *cl
)
{
  cl->signature0 = cl->signature1 = 0;
  cl->nLits = 0;
}


/**Function********************************************************************
  Synopsis    [obtain a free var from a SAT solver]
  Description [obtain a free var from a SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
DdiSatSolverGetFreeVar(
  Ddi_SatSolver_t *solver
)
{
  int v;
  if (1&&(solver->nFreeVars > 0)) {
    v = solver->freeVars[--solver->nFreeVars];
    solver->vars[v] = 1; /* undo var free */
  }
  else {
    if (solver->freeVarMinId>solver->nVars) {
      v = solver->freeVarMinId;
    }
    else {
      v = solver->nVars;
    }
    DdiSolverSetVar(solver,v);
  }
  solver->nFreeTot++;
  return v;
}

/**Function********************************************************************
  Synopsis    [obtain a free var from a SAT solver]
  Description [obtain a free var from a SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiSatSolverSetFreeVarMinId(
  Ddi_SatSolver_t *solver,
  int minId
)
{
  solver->freeVarMinId = minId;
}

/**Function********************************************************************
  Synopsis    [obtain a free var from a SAT solver]
  Description [obtain a free var from a SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
DdiSatSolverReadFreeVarMinId(
  Ddi_SatSolver_t *solver
)
{
  return solver->freeVarMinId;
}

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_ClauseAlloc(
  int nLits,
  int initSize
)
{
  Ddi_Clause_t *cl = Pdtutil_Alloc(Ddi_Clause_t,1);
  cl->link = NULL;
  cl->id = clauseNum++;
  cl->nRef = 0;
  cl->actLit = 0;
  cl->nLits = cl->size = nLits;
  if (initSize>nLits) cl->size = initSize;
  if (cl->size == 0) cl->size++;
  cl->lits = Pdtutil_Alloc(int,cl->size);
  cl->signature0 = cl->signature1 = 0;
  cl->sorted = 1;
  return cl;
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_ClauseDup(
  Ddi_Clause_t *cl
)
{
  int i;
  Ddi_Clause_t *cl2 = Pdtutil_Alloc(Ddi_Clause_t,1);
  cl2->link = NULL;

  cl2->id = clauseNum++;

  cl2->actLit = 0;
  cl2->nRef = 0;
  cl2->nLits = cl->nLits;
  cl2->size = cl->size;
  cl2->lits = Pdtutil_Alloc(int,cl2->size);
  for (i=0; i<cl->nLits; i++) {
    cl2->lits[i] = cl->lits[i];
  }
  cl2->signature0 = cl->signature0;
  cl2->signature1 = cl->signature1;
  cl2->sorted = cl->sorted;
  return cl2;
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_ClauseArray_t *
Ddi_ClauseArrayDup(
  Ddi_ClauseArray_t *ca
)
{
  int i, n = Ddi_ClauseArrayNum(ca);
  Ddi_ClauseArray_t *ca2 = Ddi_ClauseArrayAlloc(n);

  for (i=0;i<n;i++) {
    Ddi_ClauseArrayPush(ca2,ca->clauses[i]);
  }

  return ca2;
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_ClauseArray_t *
Ddi_ClauseArrayToGblVarIds(
  Ddi_PdrMgr_t *pdrMgr,
  Ddi_ClauseArray_t *ca
)
{
  int i, n = Ddi_ClauseArrayNum(ca);
  Ddi_ClauseArray_t *ca2 = Ddi_ClauseArrayAlloc(n);

  for (i=0;i<n;i++) {
    Ddi_Clause_t *cl2 = Ddi_ClauseToGblVarIds(pdrMgr,ca->clauses[i]);
    if (cl2!=NULL)
      Ddi_ClauseArrayPush(ca2,cl2);

  }

  return ca2;
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeFromClauseArrayWithVars(
  Ddi_ClauseArray_t *ca,
  Ddi_Vararray_t *vars
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(vars);
  int i, n = Ddi_ClauseArrayNum(ca);
  Ddi_Bdd_t *res = Ddi_BddMakePartConjVoid(ddm);

  for (i=0;i<n;i++) {
    Ddi_Bdd_t *p_i = Ddi_BddMakeFromClauseWithVars(ca->clauses[i],vars);
    Ddi_BddPartInsertLast(res,p_i);
    Ddi_Free(p_i);
  }

  return res;
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_ClauseArray_t *
Ddi_ClauseArrayFromGblVarIds(
  Ddi_PdrMgr_t *pdrMgr,
  Ddi_ClauseArray_t *ca
)
{
  int i, n = Ddi_ClauseArrayNum(ca);
  Ddi_ClauseArray_t *ca2 = Ddi_ClauseArrayAlloc(n);

  for (i=0;i<n;i++) {
    Ddi_Clause_t *cl2 = Ddi_ClauseFromGblVarIds(pdrMgr,ca->clauses[i]);
    Ddi_ClauseArrayPush(ca2,cl2);
  }

  return ca2;
}



/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_ClauseCompact(
  Ddi_Clause_t *cl
)
{
  int i;
  Ddi_Clause_t *cl2 = Ddi_ClauseAlloc(0,1);
  for (i=0; i<cl->nLits; i++) {
    if (cl->lits[i]==0) continue;
    DdiClauseAddLiteral(cl2,cl->lits[i]);
  }
  return cl2;
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_ClauseRandLits(
  Ddi_Clause_t *vars
)
{
  int i, nLits=vars->nLits;
  Ddi_Clause_t *cl2 = Ddi_ClauseAlloc(0,nLits);
  for (i=0; i<nLits; i++) {
    DdiClauseAddLiteral(cl2,(rand()%2)?-vars->lits[i]:vars->lits[i]);
  }
  return cl2;
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_ClauseArrayNum(
  Ddi_ClauseArray_t *ca
)
{
  return (ca->nClauses);
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_ClauseNumLits(
  Ddi_Clause_t *cl
)
{
  return (cl->nLits);
}



/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Descri.ption [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_ClauseNegLits(
  Ddi_Clause_t *cl
)
{
  int i, nLits=cl->nLits;
  Ddi_Clause_t *cl2 = Ddi_ClauseAlloc(0,nLits);
  for (i=0; i<cl->nLits; i++) {
    DdiClauseAddLiteral(cl2,-(cl->lits[i]));
  }
  return cl2;
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Descri.ption [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_ClauseNegLitsAcc(
  Ddi_Clause_t *cl
)
{
  int i, nLits=cl->nLits;
  for (i=0; i<cl->nLits; i++) {
    cl->lits[i] = -(cl->lits[i]);
  }
  cl->signature1 = cl->signature0 & ~(cl->signature1);

  return cl;
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_ClauseToGblVarIds(
  Ddi_PdrMgr_t *pdrMgr,
  Ddi_Clause_t *cl
)
{
  int j, id, nLits=cl->nLits;
  Ddi_Clause_t *cl2 = Ddi_ClauseAlloc(0,nLits);

  for (j=0; j<cl->nLits; j++) {
    int lit = cl->lits[j];
    int vCnf = abs(lit);
    Ddi_Var_t *v;
    int i;
    if (vCnf%2) {
      i=vCnf/2;
      Pdtutil_Assert(i<pdrMgr->nState,"wrong pdr var");
      v = Ddi_VararrayRead(pdrMgr->ps,i);
    }
    else {
      i=vCnf/2-1;
      Pdtutil_Assert(i<pdrMgr->nState,"wrong pdr var");
      v = Ddi_VararrayRead(pdrMgr->ns,i);
    }
    if (strstr(Ddi_VarName(v),"PDT_BDD_INVAR")!=NULL) {
      // take positive cofactor
      if (lit>=0) {
	Ddi_ClauseFree(cl2);
	return NULL;
      }
    }
    Pdtutil_Assert(strstr(Ddi_VarName(v),"PDT_BDD_INVARSPEC")==NULL,"not supported PDT");
    id = Ddi_VarIndex(v)+1;
    Pdtutil_Assert(id>0,"0 var id");
    if (lit<0) id = -id;
    DdiClauseAddLiteral(cl2,id);
  }

  return cl2;
}


/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_PdrRemapVarMark(
  Ddi_PdrMgr_t *pdrMgr
)
{
  int i;
  Pdtutil_Assert(pdrMgr->varRemapMarked==0,"var remap already active");
  pdrMgr->varRemapMarked=1;
  Ddi_VararrayCheckMark(pdrMgr->ps,0);
  Ddi_VararrayCheckMark(pdrMgr->ns,0);
  Ddi_VararrayCheckMark(pdrMgr->pi,0);
  for (i=0;i<Ddi_VararrayNum(pdrMgr->ps);i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(pdrMgr->ps,i);
    int m = pdrMgr->psClause->lits[i];
    Pdtutil_Assert(m>0,"wrong lit");
    Ddi_VarWriteMark(v,m);
  }  
  for (i=0;i<Ddi_VararrayNum(pdrMgr->ns);i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(pdrMgr->ns,i);
    int m = pdrMgr->nsClause->lits[i];
    Pdtutil_Assert(m>0,"wrong lit");
    Ddi_VarWriteMark(v,m);
  }  
  for (i=0;i<Ddi_VararrayNum(pdrMgr->pi);i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(pdrMgr->pi,i);
    int m = pdrMgr->piClause->lits[i];
    Pdtutil_Assert(m>0,"wrong lit");
    Ddi_VarWriteMark(v,m);
  }  

  return 1;
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_PdrRemapVarUnmark(
  Ddi_PdrMgr_t *pdrMgr
)
{
  Pdtutil_Assert(pdrMgr->varRemapMarked==1,"var remap not active");
  pdrMgr->varRemapMarked=0;
  Ddi_VararrayWriteMark(pdrMgr->ps,0);
  Ddi_VararrayWriteMark(pdrMgr->ns,0);
  Ddi_VararrayWriteMark(pdrMgr->pi,0);
  return 1;
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_ClauseFromGblVarIds(
  Ddi_PdrMgr_t *pdrMgr,
  Ddi_Clause_t *cl
)
{
  Ddi_Mgr_t *ddm = pdrMgr->ddiMgr;
  int j, id, nLits=cl->nLits;
  Ddi_Clause_t *cl2 = Ddi_ClauseAlloc(0,nLits);

  Pdtutil_Assert(pdrMgr->varRemapMarked,"var remap not active");
  for (j=0; j<cl->nLits; j++) {
    int idSigned = cl->lits[j];
    int id = abs(idSigned)-1;
    Ddi_Var_t *v = Ddi_VarFromIndex(ddm, id);
    int lit = Ddi_VarReadMark (v);
    if (idSigned<0) lit = -lit;
    DdiClauseAddLiteral(cl2,lit);
  }

  return cl2;
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_ClauseRemapVars(
  Ddi_Clause_t *cl,
  int offset
)
{
  int i, nLits=cl->nLits;
  Ddi_Clause_t *cl2 = Ddi_ClauseAlloc(0,nLits);
  for (i=0; i<cl->nLits; i++) {
    int lit = cl->lits[i];
    lit += (lit>0) ? offset: -offset;
    DdiClauseAddLiteral(cl2,lit);
  }
  return cl2;
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_ClauseRemapVarsNegLits(
  Ddi_Clause_t *cl,
  int offset
)
{
  int i, nLits=cl->nLits;
  Ddi_Clause_t *cl2 = Ddi_ClauseAlloc(0,nLits);
  for (i=0; i<cl->nLits; i++) {
    int lit = cl->lits[i];
    lit += (lit>0) ? offset: -offset;
    DdiClauseAddLiteral(cl2,-lit);
  }
  return cl2;
}

/**Function********************************************************************
  Synopsis    [Ref Clause]
  Description [Ref Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_ClauseRef(
  Ddi_Clause_t *cl
)
{
  Pdtutil_Assert(cl->nRef>=0,"Wrong ref count for clause");
  return (++cl->nRef);
}

/**Function********************************************************************
  Synopsis    [Deref Clause]
  Description [Deref Clause. Free if cnt == 0.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_ClauseDeref(
  Ddi_Clause_t *cl
)
{
  int ret;
  Pdtutil_Assert(cl->nRef>0,"Wrong ref count for clause");
  if ((ret = --cl->nRef) == 0) {
    Ddi_ClauseFree(cl);
  }
  return ret;
}

/**Function********************************************************************
  Synopsis    [Deref Clause]
  Description [Deref Clause. Free if cnt == 0.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_ClauseSetLink(
  Ddi_Clause_t *cl,
  Ddi_Clause_t *cl2
)
{
  int ret=0;
  if (cl->link != NULL) {
    Ddi_ClauseClearLink(cl);
    ret = 1;
  }
  cl->link = cl2;
  Ddi_ClauseRef(cl2);
  return ret;
}

/**Function********************************************************************
  Synopsis    [Deref Clause]
  Description [Deref Clause. Free if cnt == 0.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_ClauseClearLink(
  Ddi_Clause_t *cl
)
{
  int ret=0;
  if (cl->link != NULL) {
    Ddi_ClauseDeref(cl->link);
    cl->link = NULL;
    ret = 1;
  }
  return ret;
}

/**Function********************************************************************
  Synopsis    [Deref Clause]
  Description [Deref Clause. Free if cnt == 0.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_ClauseReadLink(
  Ddi_Clause_t *cl
)
{
  return cl->link;
}

/**Function********************************************************************
  Synopsis    [Free Clause]
  Description [Free Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_ClauseFree(
  Ddi_Clause_t *cl
)
{
  if (cl==NULL) return;
  if (cl->lits != NULL)
    Pdtutil_Free(cl->lits);
  if (cl->link!=NULL) {
    Ddi_ClauseDeref(cl->link);
  }
  Pdtutil_Free(cl);
}

/**Function********************************************************************
  Synopsis    [Alloc Clause]
  Description [Alloc Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_ClauseArray_t *
Ddi_ClauseArrayAlloc(
  int initSize
)
{
  Ddi_ClauseArray_t *ca = Pdtutil_Alloc(Ddi_ClauseArray_t,1);
  ca->size = initSize;
  ca->nClauses = 0;
  if (initSize == 0) ca->size++;
  ca->clauses = Pdtutil_Alloc(Ddi_Clause_t *,ca->size);
  return ca;
}

/**Function********************************************************************
  Synopsis    [Free Clause Array]
  Description [Free Clause Array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_ClauseArrayFree(
  Ddi_ClauseArray_t *ca
)
{
  if (ca==NULL) return;
  if (ca->clauses != NULL) {
    int i;
    for (i=0; i<ca->nClauses; i++) {
      Ddi_ClauseDeref(ca->clauses[i]);
    }
    Pdtutil_Free(ca->clauses);
  }
  Pdtutil_Free(ca);
}

/**Function********************************************************************
  Synopsis    [Add new entry in AIG array]
  Description [Add new entry in AIG array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_ClauseArrayWriteLast(
  Ddi_ClauseArray_t *ca,
  Ddi_Clause_t *cl
)
{
  if (ca->nClauses >= ca->size) {
    if (ca->size==0) {
      ca->size = 4;
      ca->clauses = Pdtutil_Alloc(Ddi_Clause_t *,ca->size);
    }
    else {
      ca->size *= 2;
      ca->clauses =
        Pdtutil_Realloc(Ddi_Clause_t *,ca->clauses,ca->size);
    }
  }
  cl->nRef++;
  Ddi_ClauseSort(cl);
  ca->clauses[ca->nClauses++] = cl;
}

/**Function********************************************************************
  Synopsis    [Read entry in clause array]
  Description [Read entry in clause array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_ClauseArrayRemove(
  Ddi_ClauseArray_t *ca,
  int i
)
{
  Pdtutil_Assert(i<ca->nClauses,"wrong index of clause array");
  ca->clauses[i] = ca->clauses[ca->nClauses-1];
  ca->nClauses--;
}

/**Function********************************************************************
  Synopsis    [Read entry in clause array]
  Description [Read entry in clause array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_ClauseArrayReset(
  Ddi_ClauseArray_t *ca
)
{
  ca->nClauses=0;
}

/**Function********************************************************************
  Synopsis    [Read entry in clause array]
  Description [Read entry in clause array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_ClauseArrayRead(
  Ddi_ClauseArray_t *ca,
  int i
)
{
  Pdtutil_Assert(i<ca->nClauses,"wrong index of clause array");
  return ca->clauses[i];
}

/**Function********************************************************************
  Synopsis    [Add new entry in AIG array]
  Description [Add new entry in AIG array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_ClauseArrayPush(
  Ddi_ClauseArray_t *ca,
  Ddi_Clause_t *cl
)
{
  Ddi_ClauseArrayWriteLast(ca,cl);
}

/**Function********************************************************************
  Synopsis    [Extract last entry in clause array]
  Description [Extract last entry in clause array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_ClauseArrayPop(
  Ddi_ClauseArray_t *ca
)
{
  Pdtutil_Assert(ca->nClauses>0,"wrong index of clause array");
  ca->nClauses--;
  return ca->clauses[ca->nClauses];
}


/**Function********************************************************************
  Synopsis    [Generate clauses for transition relation]
  Description [Generate clauses for transition relation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_PdrMgr_t *
Ddi_PdrMgrAlloc(
  void
)
{
  Ddi_PdrMgr_t *pdrMgr = Pdtutil_Alloc(Ddi_PdrMgr_t,1);
  pdrMgr->ps = pdrMgr->ns = NULL;
  pdrMgr->psClause = pdrMgr->nsClause = NULL;
  return pdrMgr;
}


/**Function********************************************************************
  Synopsis    [Generate clauses for transition relation]
  Description [Generate clauses for transition relation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_PdrMgrQuit(
  Ddi_PdrMgr_t *pdrMgr
)
{

  DdiAig2CnfIdClose(pdrMgr->ddiMgr);

  Ddi_Free(pdrMgr->pi);
  Ddi_Free(pdrMgr->ps);
  Ddi_Free(pdrMgr->ns);
  Ddi_Free(pdrMgr->isVars);
  Ddi_Free(pdrMgr->bddSave);

  Pdtutil_Free(pdrMgr->piCnf);
  Pdtutil_Free(pdrMgr->psCnf);
  Pdtutil_Free(pdrMgr->nsCnf);
  Pdtutil_Free(pdrMgr->coiBuf);
  Pdtutil_Free(pdrMgr->varActivity);
  Pdtutil_Free(pdrMgr->varMark);

  Pdtutil_Free(pdrMgr->trNetlist.roots);
  Pdtutil_Free(pdrMgr->trNetlist.fiIds[0]);
  Pdtutil_Free(pdrMgr->trNetlist.fiIds[1]);
  Pdtutil_Free(pdrMgr->trNetlist.gateId2cnfId);
  Pdtutil_Free(pdrMgr->trNetlist.isRoot);
  Pdtutil_Free(pdrMgr->trNetlist.clauseMapIndex);
  Pdtutil_Free(pdrMgr->trNetlist.clauseMapNum);

  int i;
  for(i=0; i<pdrMgr->trNetlist.nGate; i++){
    if(pdrMgr->trNetlist.leaders[i] != NULL) Pdtutil_IntegerArrayFree(pdrMgr->trNetlist.leaders[i]);
  }
  Pdtutil_Free(pdrMgr->trNetlist.leaders);

  Ddi_ClauseFree(pdrMgr->isClause);
  Ddi_ClauseFree(pdrMgr->piClause);
  Ddi_ClauseFree(pdrMgr->psClause);
  Ddi_ClauseFree(pdrMgr->nsClause);
  Ddi_ClauseFree(pdrMgr->actTarget);
  Ddi_ClauseArrayFree(pdrMgr->latchSupps);
  Ddi_ClauseArrayFree(pdrMgr->trClausesInit);
  Ddi_ClauseArrayFree(pdrMgr->trClauses);
  Ddi_ClauseArrayFree(pdrMgr->trClausesTseitin);
  Ddi_ClauseArrayFree(pdrMgr->invarClauses);
  Ddi_ClauseArrayFree(pdrMgr->initClauses);
  Ddi_ClauseArrayFree(pdrMgr->targetClauses);

  Pdtutil_Free(pdrMgr);
}

/**Function********************************************************************
  Synopsis    [Generate clauses for transition relation]
  Description [Generate clauses for transition relation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_PdrMgr_t *
Ddi_PdrMake(
  Ddi_Bddarray_t *delta,
  Ddi_Vararray_t *pi,
  Ddi_Vararray_t *ps,
  Ddi_Vararray_t *ns,
  Ddi_Bdd_t *init,
  Ddi_Bddarray_t *initStub,
  Ddi_Bdd_t *invar,
  Ddi_Bdd_t *invarspec,
  int enUnfold
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(delta);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  Ddi_Var_t *cv=Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
  Ddi_Var_t *pv=Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
  Ddi_Bddarray_t *deltaInit = Ddi_BddarrayDup(delta);

  int i, nRoots = Ddi_BddarrayNum(delta);
  int nPi = Ddi_VararrayNum(pi);

  Ddi_PdrMgr_t *pdrMgr = Ddi_PdrMgrAlloc();

  Ddi_Bdd_t *target = Ddi_BddNot(invarspec);
  Ddi_Bdd_t *initPart = NULL;
  Ddi_Bdd_t *initPart2 = NULL;

  if (initStub!=NULL) {
    Ddi_Bdd_t *litConstr = Ddi_BddMakeLiteralAig(cv, 1);
    initPart = Ddi_BddRelMakeFromArray(initStub,ps);
    Ddi_BddarrayComposeAcc(deltaInit,ps,initStub);
    // force constraint true at initstub boundary,
    // Ddi_BddPartInsertLast(initPart,litConstr);
    Ddi_Free(litConstr);
    Ddi_Bdd_t *initPart2 = Ddi_BddRelMakeFromArray(initStub,ns);
    Ddi_Bdd_t *deltaInit2 = Ddi_BddRelMakeFromArray(deltaInit,ns);
    Ddi_BddSetAig(initPart2);
    Ddi_BddSetAig(deltaInit2);
    Ddi_BddDiffAcc(initPart2,deltaInit2);
    //    Pdtutil_Assert(!Ddi_AigSat(initPart2),"F_1 does not include init stub");
    Ddi_Free(initPart2);
    Ddi_Free(deltaInit2);
  }
  else {
    initPart = Ddi_AigPartitionTop(init,0);
    if (Ddi_BddIsCube(initPart)) {
      Ddi_AigarrayConstrainCubeAcc (deltaInit,init);
    }
  }

  if (enUnfold) {
    Ddi_Bdd_t *myOne = Ddi_BddMakeConstAig(ddm,1);

#if 0
    Ddi_BddarrayCofactorAcc(delta,pv,1);
    Ddi_BddarrayCofactorAcc(delta,cv,1);
    Ddi_BddCofactorAcc(initPart,cv,1);
#endif

    Ddi_BddComposeAcc(target,ps,delta);

    Ddi_BddCofactorAcc(target,pv,1);
    //    Ddi_BddCofactorAcc(initPart,pv,1);
    Ddi_BddarrayWrite(delta,nRoots-1,myOne);
    Ddi_Free(myOne);
#if 0
    Ddi_BddCofactorAcc(target,cv,1);
    Ddi_BddCofactorAcc(Ddi_BddarrayRead(delta,nRoots-1),cv,1);
    Ddi_BddCofactorAcc(Ddi_BddarrayRead(delta,nRoots-2),cv,1);
    Ddi_BddCofactorAcc(initPart,cv,1);
#endif
    Ddi_BddCofactorAcc(Ddi_BddarrayRead(delta,nRoots-1),pv,1);
  }
  //  Ddi_BddCofactorAcc(Ddi_BddarrayRead(delta,nRoots-1),pv,1);

  if (invar!=NULL) {
    Ddi_BddAndAcc(target,invar);
  }

  Ddi_BddNotAcc(target);
  Ddi_DataCopy(invarspec,target);
  Ddi_BddNotAcc(target);

  /* set up var mapping */

  pdrMgr->ddiMgr = ddm;
  pdrMgr->nState = nRoots;
  pdrMgr->nInput = nPi;
  pdrMgr->pi = Ddi_VararrayDup(pi);
  pdrMgr->ps = Ddi_VararrayDup(ps);
  pdrMgr->ns = Ddi_VararrayDup(ns);
  pdrMgr->isVars = NULL;

  pdrMgr->piCnf = Pdtutil_Alloc(int,nPi);
  pdrMgr->psCnf = Pdtutil_Alloc(int,nRoots);
  pdrMgr->nsCnf = Pdtutil_Alloc(int,nRoots);

  pdrMgr->initLits = NULL;
  pdrMgr->coiBuf = Pdtutil_Alloc(int,nRoots);
  pdrMgr->varActivity = Pdtutil_Alloc(int,nRoots);
  pdrMgr->varMark = Pdtutil_Alloc(int,nRoots);

  pdrMgr->isClause = NULL;
  pdrMgr->piClause = Ddi_ClauseAlloc(0,nPi);
  pdrMgr->psClause = Ddi_ClauseAlloc(0,nRoots);
  pdrMgr->nsClause = Ddi_ClauseAlloc(0,nRoots);

  pdrMgr->useInitStub = (initStub!=NULL);

  pdrMgr->trNetlist.roots = 0;
  pdrMgr->bddSave = NULL;
  
  pdrMgr->varRemapMarked = 0;

  DdiAig2CnfIdInit(ddm);

  for (i=0; i<nRoots; i++) {
    Ddi_Var_t *ps_i = Ddi_VararrayRead(ps,i);
    Ddi_Var_t *ns_i = Ddi_VararrayRead(ns,i);
    bAigEdge_t psBaig = Ddi_VarToBaig(ps_i);
    bAigEdge_t nsBaig = Ddi_VarToBaig(ns_i);
    int psCnf, nsCnf;

    pdrMgr->coiBuf[i] = 0;

    Pdtutil_Assert(psBaig!=bAig_NULL,"no baig var");
    Pdtutil_Assert(nsBaig!=bAig_NULL,"no baig var");
    psCnf = DdiAig2CnfId(bmgr,psBaig);
    nsCnf = DdiAig2CnfId(bmgr,nsBaig);

    Pdtutil_Assert(psCnf==2*(i)+1,"Wrong cnf id");
    Pdtutil_Assert(nsCnf==2*(i+1),"Wrong cnf id");
    pdrMgr->psCnf[i] = psCnf;
    pdrMgr->nsCnf[i] = nsCnf;
    pdrMgr->varActivity[i] = 0;
    pdrMgr->varMark[i] = 0;
    DdiClauseAddLiteral(pdrMgr->psClause,psCnf);
    DdiClauseAddLiteral(pdrMgr->nsClause,nsCnf);
  }

#if 0
  chkClause = Ddi_ClauseAlloc(0,1);
  int lit = pdrMgr->psClause->lits[175];
  DdiClauseAddLiteral(chkClause,lit);
#endif

  for (i=0; i<nPi; i++) {
    Ddi_Var_t *pi_i = Ddi_VararrayRead(pi,i);
    bAigEdge_t piBaig = Ddi_VarToBaig(pi_i);
    int piCnf = DdiAig2CnfId(bmgr,piBaig);
    int req = DdiPdrPiIndex2Var(pdrMgr,i);

    Pdtutil_Assert(piCnf==DdiPdrPiIndex2Var(pdrMgr,i),"Wrong cnf id");
    pdrMgr->piCnf[i] = piCnf;
    DdiClauseAddLiteral(pdrMgr->piClause,piCnf);
  }

  pdrMgr->latchSupps = Ddi_ClauseArrayAlloc(nRoots);
  pdrMgr->trClauses = Ddi_ClausesMakeRel(delta,ns,&pdrMgr->trNetlist);

  if (0 && ddm->settings.aig.aigCnfLevel) {
    int i;
    Ddi_SatSolver_t *s0 = Ddi_SatSolverAlloc();
    Ddi_SatSolver_t *s1 = Ddi_SatSolverAlloc();
    Ddi_ClauseArray_t *auxCl;
    Ddi_Clause_t *vars0 = Ddi_ClauseJoin(pdrMgr->psClause,pdrMgr->piClause);
    Ddi_Clause_t *vars = Ddi_ClauseJoin(pdrMgr->nsClause,vars0);

    ddm->settings.aig.aigCnfLevel = 0;
    auxCl = Ddi_ClausesMakeRel(delta,ns,NULL);

    Ddi_SatSolverAddClauses(s1,pdrMgr->trClauses);
    Ddi_SatSolverAddClauses(s0,auxCl);
    printf("checking eq of %d - %d clause arrays\n",
	   Ddi_ClauseArrayNum(pdrMgr->trClauses),
	   Ddi_ClauseArrayNum(auxCl));

    for (i=0; i<1000000; i++) {
      int sat = Ddi_SatSolve(s0,NULL,-1);
      if (sat) {
	Ddi_Clause_t *cexpi = Ddi_SatSolverGetCexCube(s0,pdrMgr->piClause);
	Ddi_Clause_t *cexps = Ddi_SatSolverGetCexCube(s0,pdrMgr->psClause);
	Ddi_Clause_t *cex0 = Ddi_SatSolverGetCexCube(s0,vars0);
	Ddi_Clause_t *cex1 = Ddi_SatSolverGetCexCube(s0,vars);
	if (!Ddi_SatSolve(s1,cex1,-1)) {
	  Ddi_PdrPrintClause(pdrMgr,cex1);
	  if (Ddi_SatSolve(s1,cex0,-1)) {
	    Ddi_Clause_t *cex2 = Ddi_SatSolverGetCexCube(s1,vars);
	    Ddi_PdrPrintClause(pdrMgr,cex2);
	  }
	  else {
	    do {
	      Ddi_Clause_t *cl2 = Ddi_ClauseAlloc(0,1);
	      int k;
	      for (k=0; k<Ddi_ClauseNumLits(cex0)-1; k++) {
		DdiClauseAddLiteral(cl2,cex0->lits[k]);
	      }
	      Ddi_ClauseFree(cex0);
	      cex0 = cl2;
	      printf("checking cube of size %d\n", Ddi_ClauseNumLits(cex0));
	    } while (!Ddi_SatSolve(s1,cex0,-1));
	    Ddi_Clause_t *cex2 = Ddi_SatSolverGetCexCube(s1,vars);
	    Ddi_PdrPrintClause(pdrMgr,cex2);
	  }
	  if (Ddi_SatSolve(s1,cexpi,-1)) {
	    Ddi_Clause_t *cex2 = Ddi_SatSolverGetCexCube(s1,vars);
	    Ddi_PdrPrintClause(pdrMgr,cex2);
	  }
	  else {
	    do {
	      Ddi_Clause_t *cl2 = Ddi_ClauseAlloc(0,1);
	      int k;
	      for (k=0; k<Ddi_ClauseNumLits(cexpi)-1; k++) {
		DdiClauseAddLiteral(cl2,cexpi->lits[k]);
	      }
	      Ddi_ClauseFree(cexpi);
	      cexpi = cl2;
	      printf("checking cube of size %d\n", Ddi_ClauseNumLits(cexpi));
	    } while (!Ddi_SatSolve(s1,cexpi,-1));
	    Ddi_Clause_t *cex2 = Ddi_SatSolverGetCexCube(s1,vars);
	    Ddi_PdrPrintClause(pdrMgr,cex2);
	  }
	  if (Ddi_SatSolve(s1,cexps,-1)) {
	    Ddi_Clause_t *cex2 = Ddi_SatSolverGetCexCube(s1,vars);
	    Ddi_PdrPrintClause(pdrMgr,cex2);
	  }
	  Pdtutil_Assert (0,"problem with encoding");
	}
	Ddi_SatSolverAddClauseCustomized(s0,cex0,1,0,0);

	Ddi_ClauseFree(cex0);
      }
      else break;
    }

    // pdrMgr->trClauses = auxCl;
    //    ddm->settings.aig.aigCnfLevel = 1;
    Ddi_SatSolverQuit(s0);
    Ddi_SatSolverQuit(s1);
  }
  pdrMgr->trClausesInit = Ddi_ClausesMakeRel(deltaInit,ns,NULL);
  if (invar != NULL) {
    pdrMgr->invarClauses = Ddi_AigClauses(invar,0,NULL);
  }
  else {
    pdrMgr->invarClauses = Ddi_ClauseArrayAlloc(0);
  }
  pdrMgr->initClauses = Ddi_AigClauses(initPart,0,NULL);

  if (initStub!=NULL) {
    int j;
    pdrMgr->isVars = Ddi_BddarraySuppVararray(initStub);
    int nv = Ddi_VararrayNum(pdrMgr->isVars);
    pdrMgr->isClause = Ddi_ClauseAlloc(0,nv);
    for (j=0; j<nv; j++) {
      Ddi_Var_t *v = Ddi_VararrayRead(pdrMgr->isVars,j);
      bAigEdge_t vBaig = Ddi_VarToBaig(v);
      int vCnf = DdiAig2CnfId(bmgr,vBaig);
      DdiClauseAddLiteral(pdrMgr->isClause,vCnf);
    }
  }

  pdrMgr->actTarget = Ddi_ClauseAlloc(0,1);
  pdrMgr->targetClauses = Ddi_AigClauses(target,1,pdrMgr->actTarget);

  for (i=0; i<nRoots; i++) {
    Ddi_Varset_t *s_i = Ddi_BddSupp(Ddi_BddarrayRead(delta,i));
    Ddi_Vararray_t *vA = Ddi_VararrayMakeFromVarset(s_i,1);
    int j, n = Ddi_VararrayNum(vA);
    Ddi_Clause_t *suppClause = Ddi_ClauseAlloc(0,n);

    for (j=0;j<n;j++) {
      Ddi_Var_t *v_j = Ddi_VararrayRead(vA,j);
      bAigEdge_t vBaig = Ddi_VarToBaig(v_j);
      int vCnf = DdiAig2CnfId(bmgr,vBaig);
      if (vCnf<=2*nRoots) {
	Pdtutil_Assert(vCnf%2,"Wrong cnf id");
	DdiClauseAddLiteral(suppClause,vCnf);
      }
    }

    Ddi_ClauseArrayWriteLast(pdrMgr->latchSupps,suppClause);

    Ddi_Free(s_i);
    Ddi_Free(vA);
  }

  pdrMgr->targetSuppClause =
    Ddi_PdrClauseArraySupp(pdrMgr,pdrMgr->targetClauses);
  if (pdrMgr->targetSuppClause->nLits==0) {
    /* target is just one var literal == actLiteral */
    int lit = pdrMgr->actTarget->lits[0];
    int var = abs(lit);
    DdiClauseAddLiteral(pdrMgr->targetSuppClause,var);
  }

  /* save to keep cnf encoding */
  pdrMgr->bddSave = Ddi_BddarrayDup(deltaInit);
  Ddi_BddarrayInsertLast(pdrMgr->bddSave,target);
  Ddi_BddarrayInsertLast(pdrMgr->bddSave,initPart);


  if (1) {
    int aigCnfLevel = ddm->settings.aig.aigCnfLevel;
    ddm->settings.aig.aigCnfLevel = 0;
    pdrMgr->trClausesTseitin = Ddi_ClausesMakeRel(delta,ns,NULL);
    ddm->settings.aig.aigCnfLevel = aigCnfLevel;
  }
  else {
    pdrMgr->trClausesTseitin = NULL;
  }

  Ddi_Free(target);
  Ddi_Free(initPart);
  Ddi_Free(deltaInit);

  pdrMgr->maxAigVar = ddm->cnf.maxCnfId;
  pdrMgr->freeCnfVar = ddm->cnf.maxCnfId+1;

  return (pdrMgr);
}

/**Function********************************************************************
  Synopsis    [Gen BDD from Clause]
  Description [Gen BDD from Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeFromClause(
  Ddi_PdrMgr_t *pdrMgr,
  Ddi_Clause_t *cl
)
{
  int j;
  Ddi_Mgr_t *ddm = pdrMgr->ddiMgr;
  Ddi_Bdd_t *resBdd = Ddi_BddMakeConstAig(ddm,0);

  for (j=0; j<cl->nLits; j++) {
    Ddi_Bdd_t *litBdd;
    int lit = cl->lits[j];
    int vCnf = abs(lit);
    Ddi_Var_t *v;
    int i;
    if (vCnf%2) {
      i=vCnf/2;
      Pdtutil_Assert(i<pdrMgr->nState,"wrong pdr var");
      v = Ddi_VararrayRead(pdrMgr->ps,i);
    }
    else {
      i=vCnf/2-1;
      Pdtutil_Assert(i<pdrMgr->nState,"wrong pdr var");
      v = Ddi_VararrayRead(pdrMgr->ns,i);
    }
    litBdd = Ddi_BddMakeLiteralAig(v,lit>0);
    Ddi_BddOrAcc(resBdd,litBdd);
    Ddi_Free(litBdd);
  }

  return resBdd;

}

/**Function********************************************************************
  Synopsis    [Gen BDD from Clause]
  Description [Gen BDD from Clause]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddMakeFromClauseWithVars(
  Ddi_Clause_t *cl,
  Ddi_Vararray_t *vars
)
{
  int j;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(vars);
  Ddi_Bdd_t *resBdd = Ddi_BddMakeConstAig(ddm,0);

  for (j=0; j<cl->nLits; j++) {
    Ddi_Bdd_t *litBdd;
    int lit = cl->lits[j];
    int vCnf = abs(lit);
    Ddi_Var_t *v;
    int i;
    if (vCnf%2) {
      i=vCnf/2;
    }
    else {
      i=vCnf/2-1;
    }
    v = Ddi_VararrayRead(vars,i);
    litBdd = Ddi_BddMakeLiteralAig(v,lit>0);
    Ddi_BddOrAcc(resBdd,litBdd);
    Ddi_Free(litBdd);
  }

  return resBdd;
}

/**Function********************************************************************
  Synopsis    [Print clause with pdr var mapping]
  Description [Print clause with pdr var mapping]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_PdrPrintClauseArray(
  Ddi_PdrMgr_t *pdrMgr,
  Ddi_ClauseArray_t *ca
)
{
  int j;
  for (j=0; j<ca->nClauses; j++) {
    printf("CA[%2d]: ",j);
    Ddi_PdrPrintClause(pdrMgr,ca->clauses[j]);
  }
}


#define PRINT_LIT_NAMES 1

/**Function********************************************************************
  Synopsis    [Print clause with pdr var mapping]
  Description [Print clause with pdr var mapping]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_PdrPrintClause(
  Ddi_PdrMgr_t *pdrMgr,
  Ddi_Clause_t *cl
)
{
  int j;
  char *buf0 = Pdtutil_Alloc(char,pdrMgr->nState+2);
  char *buf1 = Pdtutil_Alloc(char,pdrMgr->nState+2);
  char *buf2 = Pdtutil_Alloc(char,pdrMgr->nInput+2);
  int en0=0, en1=0, en2=0;
  int printLitNames = 0;

  for (j=0; j<pdrMgr->nState; j++) buf0[j]=buf1[j]='-';
  for (j=0; j<pdrMgr->nInput; j++) buf2[j]='-';

#if PRINT_LIT_NAMES
  //  printLitNames = 1;
  for (j=0; 1&&(j<cl->nLits); j++) {
    int lit = cl->lits[j];
    int vCnf = abs(lit);
    Ddi_Var_t *v;
    int i;
    if (DdiPdrVarIsPs(pdrMgr,vCnf)) {
      i=DdiPdrVarPsIndex(pdrMgr,vCnf);
      Pdtutil_Assert(i<pdrMgr->nState,"wrong pdr var");
      v = Ddi_VararrayRead(pdrMgr->ps,i);
    }
    else if (DdiPdrVarIsNs(pdrMgr,vCnf)) {
      i=DdiPdrVarNsIndex(pdrMgr,vCnf);
      Pdtutil_Assert(i<pdrMgr->nState,"wrong pdr var");
      v = Ddi_VararrayRead(pdrMgr->ns,i);
    }
    else if (DdiPdrVarIsPi(pdrMgr,vCnf)) {
      i=DdiPdrVarPiIndex(pdrMgr,vCnf);
      Pdtutil_Assert(i<pdrMgr->nInput,"wrong pdr var");
      v = Ddi_VararrayRead(pdrMgr->pi,i);
    }
    printf("%s ", Ddi_VarName(v));
  }
  printf ("\n");
#endif
  for (j=0; j<cl->nLits; j++) {
    int lit = cl->lits[j];
    int vCnf = abs(lit);
    Ddi_Var_t *v;
    int i;
    if (DdiPdrVarIsPs(pdrMgr,vCnf)) {
      i=DdiPdrVarPsIndex(pdrMgr,vCnf);
      Pdtutil_Assert(i<pdrMgr->nState,"wrong pdr var");
      v = Ddi_VararrayRead(pdrMgr->ps,i);
      buf1[i]=lit>0?'1':'0';
      en1=1;
    }
    else if (DdiPdrVarIsNs(pdrMgr,vCnf)) {
      i=DdiPdrVarNsIndex(pdrMgr,vCnf);
      Pdtutil_Assert(i<pdrMgr->nState,"wrong pdr var");
      v = Ddi_VararrayRead(pdrMgr->ns,i);
      buf0[i]=lit>0?'1':'0';
      en0=1;
    }
    else if (DdiPdrVarIsPi(pdrMgr,vCnf)) {
      i=DdiPdrVarPiIndex(pdrMgr,vCnf);
      Pdtutil_Assert(i<pdrMgr->nInput,"wrong pdr var");
      v = Ddi_VararrayRead(pdrMgr->pi,i);
      buf2[i]=lit>0?'1':'0';
      en2=1;
    }
#if 0
    printf("%c", lit>0?'1':'0');
#endif
  }

  if (!printLitNames) {
  if (en1) {
    printf ("PS: "); for (j=0; j<pdrMgr->nState; j++) {
      if (buf1[j]!='-') printf("%d ", buf1[j]=='0' ? -(j+1) : j+1);
    }
    printf ("0\n");
  }
  if (en0) {
    printf ("NS: "); for (j=0; j<pdrMgr->nState; j++) {
      if (buf0[j]!='-') printf("%d ", buf0[j]=='0' ? -(j+1) : j+1);
    }
    printf ("0\n");
  }
  if (en2) {
    printf ("PI: "); for (j=0; j<pdrMgr->nInput; j++) {
      if (buf2[j]!='-') printf("%d ", buf2[j]=='0' ? -(j+1) : j+1);
    }
    printf ("0\n");
  }
  }
  else {
  if (en1) {
    printf ("PS: "); for (j=0; j<pdrMgr->nState; j++) printf("%c",buf1[j]);
    printf ("\n");
  }
  if (en0) {
    printf ("NS: "); for (j=0; j<pdrMgr->nState; j++) printf("%c",buf0[j]);
    printf ("\n");
  }
  if (en2) {
    printf ("PI: "); for (j=0; j<pdrMgr->nInput; j++) printf("%c",buf2[j]);
    printf ("\n");
  }
  }
  Pdtutil_Free(buf0);
  Pdtutil_Free(buf1);
  Pdtutil_Free(buf2);


}

/**Function********************************************************************
  Synopsis    [Print clause with pdr var mapping]
  Description [Print clause with pdr var mapping]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_PdrReadStateClause(
  Ddi_PdrMgr_t *pdrMgr,
  char *buf,
  int ps
)
{
  int j, lit, n, v;
  char *s=buf;
  Ddi_Clause_t *cl = Ddi_ClauseAlloc(0,1);

  for (j=0; j<pdrMgr->nState && sscanf(s,"%d%n", &lit, &n)==1; j++) {
    s += n;
    if (lit==0) break;
    if (ps) {
      v = DdiPdrPsIndex2Var(pdrMgr,abs(lit)-1);
    }
    else {
      v = DdiPdrNsIndex2Var(pdrMgr,abs(lit)-1);
    }
    if (lit<0) v = -v;
    DdiClauseAddLiteral(cl,v);
  }
  return (cl);
}


/**Function********************************************************************
  Synopsis    [Generate clauses for transition relation]
  Description [Generate clauses for transition relation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_PdrMgrGetFreeCnfVar(
  Ddi_PdrMgr_t *pdrMgr
)
{
  int v = pdrMgr->freeCnfVar++;
  return v;
}


/**Function********************************************************************
  Synopsis    [Generate clause support]
  Description [Generate clause support]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_PdrClauseArraySupp(
  Ddi_PdrMgr_t *pdrMgr,
  Ddi_ClauseArray_t *ca
)
{
  int i, j;
  Ddi_Clause_t *clSupp = Ddi_ClauseAlloc(0,1);

  for (i=0; i<ca->nClauses; i++) {
    Ddi_Clause_t *cl = ca->clauses[i];
    for (j=0; j<cl->nLits; j++) {
      int var = abs(cl->lits[j]);
      if (var>2*pdrMgr->nState) continue;
      if (var%2) {
	int ii=var/2;
	pdrMgr->coiBuf[ii] = 1;
      }
      else {
	Pdtutil_Assert(0,"PDR coi for ns var");
      }
    }
  }
  for (i=0; i<pdrMgr->nState; i++) {
    if (pdrMgr->coiBuf[i]) {
      int var = pdrMgr->psCnf[i];
      DdiClauseAddLiteral(clSupp,var);
      pdrMgr->coiBuf[i]=0;
    }
  }

  return clSupp;

}

/**Function********************************************************************
  Synopsis    [Update clause activity]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_PdrVarHasActivity(
  Ddi_PdrMgr_t *pdrMgr,
  int lit
)
{
  int i;

  int var = abs(lit);
  if (var>2*pdrMgr->nState) return(0);
  Pdtutil_Assert(DdiPdrVarIsPs(pdrMgr,var),"Ps var required");
  i = DdiPdrVarPsIndex(pdrMgr,var);
  return (pdrMgr->varActivity[i]>0);

}

/**Function********************************************************************
  Synopsis    [Update clause activity]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_PdrClauseUpdateActivity(
  Ddi_PdrMgr_t *pdrMgr,
  Ddi_Clause_t *cl
)
{
  int j, i;

  for (j=0; j<cl->nLits; j++) {
    int var = abs(cl->lits[j]);
    if (var==0 || var>2*pdrMgr->nState) continue;
    Pdtutil_Assert(DdiPdrVarIsPs(pdrMgr,var),"Ps var required");
    i = DdiPdrVarPsIndex(pdrMgr,var);
    pdrMgr->varActivity[i]++;
  }
}

/**Function********************************************************************
  Synopsis    [Update clause activity]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int *
Ddi_PdrClauseActivityOrder(
  Ddi_PdrMgr_t *pdrMgr,
  Ddi_Clause_t *cl
)
{
  int j, i, ii, jj, *order;
  order = Pdtutil_Alloc(int,cl->nLits);
  for (i=0; i<cl->nLits; i++) {
    int v_i = abs(cl->lits[i]);
    Pdtutil_Assert(DdiPdrVarIsPs(pdrMgr,v_i),"Ps var required");
    order[i]=i;
  }
  //    return order;
#if 0
  for (i=0; i<cl->nLits-1; i++) {
    int tmp, best_i = i;
    int v_i = abs(cl->lits[order[i]]);
    ii = DdiPdrVarPsIndex(pdrMgr,v_i);
    for (j=i+1; j<cl->nLits; j++) {
      int v_j = abs(cl->lits[order[j]]);
      jj = DdiPdrVarPsIndex(pdrMgr,v_j);
      if (pdrMgr->varActivity[ii]>pdrMgr->varActivity[jj]) {
	best_i = j;
	v_i = abs(cl->lits[order[j]]);
	ii = DdiPdrVarPsIndex(pdrMgr,v_i);
      }
    }
    tmp = order[i];
    order[i] = order[best_i];
    order[best_i] = tmp;
  }
#else
  for (i=1; i<cl->nLits; i++) {
    int tmp, x = order[i];
    int v_i = abs(cl->lits[x]);
    ii = DdiPdrVarPsIndex(pdrMgr,v_i);
    for (j=i; j>0; j--) {
      int v_j = abs(cl->lits[order[j-1]]);
      jj = DdiPdrVarPsIndex(pdrMgr,v_j);
      if (pdrMgr->varActivity[ii]>pdrMgr->varActivity[jj]) {
	break;
      }
      order[j]=order[j-1];
    }
    order[j] = x;
  }
#endif
  for (i=0; 0&&i<cl->nLits-1; i++) {
    int v_i = abs(cl->lits[order[i]]);
    int v_j = abs(cl->lits[order[i+1]]);
    ii = DdiPdrVarPsIndex(pdrMgr,v_i);
    jj = DdiPdrVarPsIndex(pdrMgr,v_j);
    Pdtutil_Assert(!(pdrMgr->varActivity[ii]>pdrMgr->varActivity[jj]),
		   "error in activity sorting");
  }
  return order;
}

/**Function********************************************************************
  Synopsis    [Update clause activity]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_PdrVarReadMark(
  Ddi_PdrMgr_t *pdrMgr,
  int lit
)
{
  int i;

  int var = abs(lit);
  if (var>2*pdrMgr->nState) return(0);
  if (DdiPdrVarIsNs(pdrMgr,var)) var--;
  Pdtutil_Assert(DdiPdrVarIsPs(pdrMgr,var),"Ps var required");
  i = DdiPdrVarPsIndex(pdrMgr,var);
  return (pdrMgr->varMark[i]);

}

/**Function********************************************************************
  Synopsis    [Update clause activity]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_PdrVarWriteMark(
  Ddi_PdrMgr_t *pdrMgr,
  int lit,
  int mark
)
{
  int i;

  int var = abs(lit);
  if (var>2*pdrMgr->nState) return;
  if (DdiPdrVarIsNs(pdrMgr,var)) var--;
  Pdtutil_Assert(DdiPdrVarIsPs(pdrMgr,var),"Ps var required");
  i = DdiPdrVarPsIndex(pdrMgr,var);
  pdrMgr->varMark[i] = mark;

}

/**Function********************************************************************
  Synopsis    [Generate clause coi]
  Description [Generate clause coi]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_PdrClauseCoi(
  Ddi_PdrMgr_t *pdrMgr,
  Ddi_Clause_t *cl
)
{
  int i, j;
  Ddi_Clause_t *clSupp = Ddi_ClauseAlloc(0,1);

  for (j=0; j<cl->nLits; j++) {
    int var = abs(cl->lits[j]);
    if (var>2*pdrMgr->nState) continue;
    if (var%2) {
      int ii=var/2;
      pdrMgr->coiBuf[ii] = 1;
    }
    else {
      Pdtutil_Assert(0,"PDR coi for ns var");
    }
  }
  for (i=0; i<pdrMgr->nState; i++) {
    if (pdrMgr->coiBuf[i]) {
      Ddi_Clause_t *clSupp1 = Ddi_ClauseJoin(clSupp,
					     pdrMgr->latchSupps->clauses[i]);
      pdrMgr->coiBuf[i]=0;
      Ddi_ClauseFree(clSupp);
      clSupp = clSupp1;
    }
  }

  return clSupp;

}


/**Function********************************************************************
  Synopsis    [Count cube lits out of init state]
  Description [Count cube lits out of init state]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_PdrCubeNoInitLitNum(
  Ddi_PdrMgr_t *pdrMgr,
  Ddi_Clause_t *cube,
  int neg
)
{
  int i, cnt=0;
  if (pdrMgr->initLits == NULL) return -1;

  for (i=cnt=0; i<Ddi_ClauseNumLits(cube); i++) {
    int lit = neg ? -cube->lits[i]: cube->lits[i];
    int pos = (lit>0);
    int var = abs(lit);
    Pdtutil_Assert(var%2==1,"wrong cube var");
    var = var/2;
    if (pdrMgr->initLits[var] != pos) {
      /* found difference with init */
      cnt++;
    }
  }
  return cnt;

}

/**Function********************************************************************
  Synopsis    [Count clause lits out of init state]
  Description [Count clause lits out of init state]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_PdrCubeLitIsNoInit(
  Ddi_PdrMgr_t *pdrMgr,
  int lit
)
{
  int i, pos, var;
  if (pdrMgr->initLits == NULL) return -1;

  pos = (lit>0);
  var = abs(lit);
  Pdtutil_Assert(var%2==1,"wrong cube var");
  var = var/2;
  if (pdrMgr->initLits[var] != pos) {
    /* found difference with init */
    return 1;
  }
  return 0;

}



/**Function********************************************************************
  Synopsis    [Generate clauses for transition relation]
  Description [Generate clauses for transition relation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_ClauseArray_t *
Ddi_ClausesMakeRel(
  Ddi_Bddarray_t *fA,
  Ddi_Vararray_t *vA,
  Ddi_NetlistClausesInfo_t *netCl
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(fA);
  bAig_Manager_t *bmgr = ddm->aig.mgr;

  bAig_array_t *visitedNodes;
  int i, j, nRoots = Ddi_BddarrayNum(fA);
  Ddi_ClauseArray_t *caTot;
  int aigCnfLevel = ddm->settings.aig.aigCnfLevel;
  Ddi_AigCnfInfo_t *aigCnfInfo;

  Pdtutil_Assert(nRoots==Ddi_VararrayNum(vA),"wrong array num");

  if (aigCnfLevel == 6) {
    /* abc cnf */
    return Ddi_ClausesMakeRelAbc(fA,vA,netCl);
  }

  visitedNodes = bAigArrayAlloc();
  caTot = Ddi_ClauseArrayAlloc(nRoots);

  for (i=0; i<nRoots; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    bAigEdge_t fBaig = Ddi_BddToBaig(f_i);

    if (!bAig_NodeIsConstant(fBaig)) {
      DdiPostOrderAigVisitIntern(bmgr,fBaig,visitedNodes,-1);
    }
  }
  DdiPostOrderAigClearVisitedIntern(bmgr,visitedNodes);

  if (netCl != NULL) {
    netCl->nGate = visitedNodes->num;
    netCl->nRoot = nRoots;
    netCl->roots = Pdtutil_Alloc(int,nRoots);
    netCl->gateId2cnfId = Pdtutil_Alloc(int,netCl->nGate);
    netCl->leaders = Pdtutil_Alloc(Pdtutil_Array_t*, netCl->nGate);
    netCl->isRoot = Pdtutil_Alloc(int,netCl->nGate);
    netCl->fiIds[0] = Pdtutil_Alloc(int,netCl->nGate);
    netCl->fiIds[1] = Pdtutil_Alloc(int,netCl->nGate);
    netCl->clauseMapIndex = Pdtutil_Alloc(int,netCl->nGate+nRoots);
    netCl->clauseMapNum = Pdtutil_Alloc(int,netCl->nGate+nRoots);
  }

  if (aigCnfLevel>0) {
    aigCnfInfo = DdiGenAigCnfInfo(bmgr,visitedNodes,fA, 1); //TODO: riattivare i blocchi
  }

  for (j=visitedNodes->num-1; j>=0; j--) {
    bAigEdge_t baig = visitedNodes->nodes[j];

    if(netCl!=NULL){
  		netCl->gateId2cnfId[j] = -1;
  		netCl->isRoot[j] = aigCnfLevel>0 ? aigCnfInfo[j].isRoot : 0;
  		netCl->leaders[j] = NULL;
  	}

    if (!bAig_isVarNode(bmgr,baig) && !bAig_NodeIsConstant(baig)) {
      if (aigCnfLevel==0 || aigCnfInfo[j].isRoot) {
        int a = DdiAig2CnfId(bmgr,baig);
        if (netCl != NULL) {
        	netCl->gateId2cnfId[j] = a;
        }
      }
    }

    if(bAig_isVarNode(bmgr,baig) && netCl != NULL){
    	int a = DdiAig2CnfId(bmgr,baig);
      netCl->gateId2cnfId[j] = a;
    }
  }

  for (j=0; j<visitedNodes->num; j++) {

    bAigEdge_t baig = visitedNodes->nodes[j];
    bAig_AuxInt(bmgr,baig) = j;

    if (netCl != NULL) {
      /* init gate info */
      netCl->fiIds[0][j] = netCl->fiIds[1][j] = netCl->clauseMapIndex[j] = 0;
      netCl->clauseMapNum[j] = 0;
    }
    if (!bAig_isVarNode(bmgr,baig) && !bAig_NodeIsConstant(baig)) {
      int f, a, b, ir, il;
      bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
      bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);

      Pdtutil_Assert(left!=bAig_NULL && right!=bAig_NULL,"NULL child");

      ir = bAig_AuxInt(bmgr,right);
      il = bAig_AuxInt(bmgr,left);

      if (aigCnfLevel==0) {
	/* standard tseitin transformation */
	/* convert AIG node to cnf clauses */
	/* f = a&b --> (f+!a+!b)(!f+a)(!f+b) */
	f = DdiAig2CnfId(bmgr,baig);
	a = bAig_NodeIsInverted(right) ? -DdiAig2CnfId(bmgr,right) :
	                                  DdiAig2CnfId(bmgr,right);
	b = bAig_NodeIsInverted(left) ? -DdiAig2CnfId(bmgr,left) :
	                                 DdiAig2CnfId(bmgr,left);
	if (netCl != NULL) {
		netCl->fiIds[0][j] = bAig_NodeIsInverted(right) ? -(ir + 1) : (ir + 1);
		netCl->fiIds[1][j] = bAig_NodeIsInverted(left) ? -(il + 1) : (il + 1);
	  netCl->clauseMapIndex[j] = caTot->nClauses;
	  netCl->clauseMapNum[j] = 3;
	}
	/* f -a -b */
	DdiClauseArrayAddClause3(caTot,f,-a,-b);
	/* -f a */
	DdiClauseArrayAddClause2(caTot,-f,a);
	/* -f b */
	DdiClauseArrayAddClause2(caTot,-f,b);
      }
      else {
	if (netCl != NULL) {
		netCl->fiIds[0][j] = bAig_NodeIsInverted(right) ? -(ir + 1) : (ir + 1);
		netCl->fiIds[1][j] = bAig_NodeIsInverted(left) ? -(il + 1) : (il + 1);
	  netCl->clauseMapIndex[j] = caTot->nClauses;
	}
	if (aigCnfInfo[j].isRoot == 3) {
	  /* Xor */
	  int iA = aigCnfInfo[j].opIds[0];
	  int iB = aigCnfInfo[j].opIds[1];
	  bAigEdge_t baigA = visitedNodes->nodes[abs(iA)-1];
	  bAigEdge_t baigB = visitedNodes->nodes[abs(iB)-1];
	  f = DdiAig2CnfId(bmgr,baig);
	  a = (iA<0) ? -DdiAig2CnfId(bmgr,baigA) :
	                DdiAig2CnfId(bmgr,baigA);
	  b = (iB<0) ? -DdiAig2CnfId(bmgr,baigB) :
	                DdiAig2CnfId(bmgr,baigB);
	  if (aigCnfInfo[j].ca==NULL)
	    aigCnfInfo[j].ca = Ddi_ClauseArrayAlloc(4);
	  /* !f !a !b */
	  DdiClauseArrayAddClause3(aigCnfInfo[j].ca,-f,-a,-b);
	  /* !f a b */
	  DdiClauseArrayAddClause3(aigCnfInfo[j].ca,-f,a,b);
	  /* f a !b */
	  DdiClauseArrayAddClause3(aigCnfInfo[j].ca,f,a,-b);
	  /* f !a b */
	  DdiClauseArrayAddClause3(aigCnfInfo[j].ca,f,-a,b);
	}
	else if (aigCnfInfo[j].isRoot == 4) {
	  /* ITE */
	  int s;
	  int iS = aigCnfInfo[j].opIds[0];
	  int iA = aigCnfInfo[j].opIds[1];
	  int iB = aigCnfInfo[j].opIds[2];
	  bAigEdge_t baigS = visitedNodes->nodes[abs(iS)-1];
	  bAigEdge_t baigA = visitedNodes->nodes[abs(iA)-1];
	  bAigEdge_t baigB = visitedNodes->nodes[abs(iB)-1];
	  f = DdiAig2CnfId(bmgr,baig);
	  s = (iS<0) ? -DdiAig2CnfId(bmgr,baigS) :
	                DdiAig2CnfId(bmgr,baigS);
	  a = (iA<0) ? -DdiAig2CnfId(bmgr,baigA) :
	                DdiAig2CnfId(bmgr,baigA);
	  b = (iB<0) ? -DdiAig2CnfId(bmgr,baigB) :
	                DdiAig2CnfId(bmgr,baigB);
	  if (aigCnfInfo[j].ca==NULL)
	    aigCnfInfo[j].ca = Ddi_ClauseArrayAlloc(4);
	  /* s f !b */
	  DdiClauseArrayAddClause3(aigCnfInfo[j].ca,s,f,-b);
	  /* s !f b */
	  DdiClauseArrayAddClause3(aigCnfInfo[j].ca,s,-f,b);
	  /* !s f !a */
	  DdiClauseArrayAddClause3(aigCnfInfo[j].ca,-s,f,-a);
	  /* !s !f a */
	  DdiClauseArrayAddClause3(aigCnfInfo[j].ca,-s,-f,a);
	}
	else if (aigCnfInfo[ir].isRoot || aigCnfInfo[il].isRoot) {
	  /* cut gate, i.e. not an internal one */
	  int ldr = aigCnfInfo[j].ldr;
	  if (aigCnfInfo[ldr].isRoot < 3) {
	    bAigEdge_t baigLdr = visitedNodes->nodes[ldr];
	    Pdtutil_Assert(aigCnfInfo[ldr].isRoot,"wrong leader");
	    f = DdiAig2CnfId(bmgr,baigLdr);
	    if (aigCnfInfo[ir].isRoot) {
	      Ddi_Clause_t *cl;
	      Pdtutil_Assert(aigCnfInfo[ldr].ca!=NULL,"missing clause array");
	      cl = Ddi_ClauseArrayRead(aigCnfInfo[ldr].ca,0);
	      a = bAig_NodeIsInverted(right) ? -DdiAig2CnfId(bmgr,right) :
		DdiAig2CnfId(bmgr,right);
	      /* -f a */
	      DdiClauseArrayAddClause2(aigCnfInfo[ldr].ca,-f,a);
	      /* f -a ... */
	      Pdtutil_Assert(ldr>=ir,"wrong leader");
	      DdiClauseAddLiteral(cl,-a);
	    }
	    if (aigCnfInfo[il].isRoot) {
	      Ddi_Clause_t *cl;
	      Pdtutil_Assert(aigCnfInfo[ldr].ca!=NULL,"missing clause array");
	      cl = Ddi_ClauseArrayRead(aigCnfInfo[ldr].ca,0);
	      b = bAig_NodeIsInverted(left) ? -DdiAig2CnfId(bmgr,left) :
		DdiAig2CnfId(bmgr,left);
	      /* -f b */
	      DdiClauseArrayAddClause2(aigCnfInfo[ldr].ca,-f,b);
	      /* f -b ... */
	      DdiClauseAddLiteral(cl,-b);
	    }
	  }
	}
	if (aigCnfInfo[j].isRoot) {
	  /* push ldr clauses: f -a -b .... */
	  Ddi_ClauseArray_t *ca = aigCnfInfo[j].ca;
	  int k;
	  bAigEdge_t baig = visitedNodes->nodes[j];
	  int a = DdiAig2CnfId(bmgr,baig);
	  int mapId = aigCnfInfo[j].mapLdr;
	  int reMapped=0;
	  for (k=0; k<Ddi_ClauseArrayNum(ca); k++) {
	    Ddi_Clause_t *cl = Ddi_ClauseArrayRead(ca,k);
	    Pdtutil_Assert(Ddi_ClauseNumLits(cl)>=2,"wrong n. lits");
	    if (aigCnfInfo[j].isMapped) {
	      Ddi_Var_t *v = Ddi_VararrayRead(vA,abs(mapId)-1);
	      bAigEdge_t vBaig = Ddi_VarToBaig(v);
	      int i;
	      int vCnf = DdiAig2CnfId(bmgr,vBaig);
	      if (mapId<0) vCnf = -vCnf;
	      for (i=0;i<cl->nLits; i++) {
		int lit = cl->lits[i];
		if (abs(lit)==a) {
		  lit = (lit<0) ? -vCnf : vCnf;
		  cl->lits[i] = lit;
		  reMapped = 1;
		}
	      }
	      if (reMapped)
		Ddi_ClauseSort(cl);
	    }
	    Ddi_ClauseArrayPush(caTot,cl);
	  }
	}
	if (netCl != NULL) {
	  netCl->clauseMapNum[j] = caTot->nClauses - netCl->clauseMapIndex[j];
	}
      }

    }

  }

  for (i=0; i<nRoots; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    Ddi_Var_t *v_i = Ddi_VararrayRead(vA,i);
    bAigEdge_t fBaig = Ddi_BddToBaig(f_i);
    bAigEdge_t vBaig = Ddi_VarToBaig(v_i);

    if (bAig_NodeIsConstant(fBaig)) {
      int vLit;
      /* vLit */
      vLit = (fBaig==bAig_Zero) ? -DdiAig2CnfId(bmgr,vBaig) :
                                   DdiAig2CnfId(bmgr,vBaig);
      if (netCl != NULL) {
	netCl->roots[i] = 0;
	netCl->clauseMapIndex[netCl->nGate+i] = caTot->nClauses;
	netCl->clauseMapNum[netCl->nGate+i] = 1;
      }
      DdiClauseArrayAddClause1(caTot,vLit);
    }
    else {
      int fLit, vCnf;
      int ii = bAig_AuxInt(bmgr,fBaig);
      // insert clauses anyway as constraints for other funcs
      int mapped = 0 && (abs(aigCnfInfo[ii].isMapped) == i+1);
      vCnf = DdiAig2CnfId(bmgr,vBaig);
      fLit = bAig_NodeIsInverted(fBaig) ? -DdiAig2CnfId(bmgr,fBaig) :
	                                   DdiAig2CnfId(bmgr,fBaig);
      if (netCl != NULL) {
	netCl->roots[i] = bAig_NodeIsInverted(fBaig) ? -(bAig_AuxInt(bmgr,fBaig) + 1) : (bAig_AuxInt(bmgr,fBaig) + 1);
	netCl->clauseMapIndex[netCl->nGate+i] = caTot->nClauses;
	netCl->clauseMapNum[netCl->nGate+i] = mapped?0:2;
      }
      /* cl: vCnf == fLit */
      if (!mapped) {
	DdiClauseArrayAddClause2(caTot,-vCnf,fLit);
	DdiClauseArrayAddClause2(caTot,vCnf,-fLit);
      }
    }
  }

  for (j=0; j<visitedNodes->num; j++) {
    bAigEdge_t baig = visitedNodes->nodes[j];
    bAig_AuxInt(bmgr,baig) = -1;
    if (aigCnfLevel>0) {
      Ddi_ClauseArrayFree(aigCnfInfo[j].ca);
    }
  }

  if (aigCnfLevel>0) {
    Pdtutil_Free(aigCnfInfo);
  }
  bAigArrayFree(visitedNodes);

  return (caTot);
}

#if 1

/**Function********************************************************************
  Synopsis    [Generate clauses for transition relation]
  Description [Generate clauses for transition relation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_ClauseArray_t *
Ddi_ClausesMakeRelAbc(
  Ddi_Bddarray_t *fA,
  Ddi_Vararray_t *vA,
  Ddi_NetlistClausesInfo_t *netCl
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(fA);
  bAig_Manager_t  *bmgr = Ddi_MgrReadMgrBaig(ddm);

  bAig_array_t *visitedNodes;
  int i, j, nRoots = Ddi_BddarrayNum(fA), maxAbcCnf=0;
  Ddi_ClauseArray_t *caTot = Ddi_ClauseArrayAlloc(nRoots);
  int aigCnfLevel = Ddi_MgrReadAigCnfLevel(ddm);
  Ddi_AigCnfInfo_t *aigCnfInfo;
  Ddi_AigAbcInfo_t *aigAbcInfo;                             //Vettore di strutture AbcInfo
  Ddi_Vararray_t *pIs;

  Pdtutil_Assert(nRoots==Ddi_VararrayNum(vA),"wrong array num");

  Ddi_AbcLock ();

  //Genero l'oggetto che mantiene le informazioni sul mapping con abc
  pIs = Ddi_BddarraySuppVararray(fA);
  aigAbcInfo = Ddi_BddarrayToAbcCnfInfo (fA, pIs, NULL);
  Ddi_Free(pIs);

  //Prendo il vettore di gate del baig
  visitedNodes = Ddi_AbcCnfInfoReadVisitedNodes(aigAbcInfo);

  //Inizializzo la struttura netlist
  if (netCl != NULL) {
    netCl->nGate = visitedNodes->num;
    netCl->nRoot = nRoots;
    netCl->roots = Pdtutil_Alloc(int,nRoots);
    netCl->gateId2cnfId = Pdtutil_Alloc(int,netCl->nGate);
    netCl->leaders = Pdtutil_Alloc(Pdtutil_Array_t*,netCl->nGate);
    netCl->isRoot = Pdtutil_Alloc(int,netCl->nGate);
    netCl->fiIds[0] = Pdtutil_Alloc(int,netCl->nGate);
    netCl->fiIds[1] = Pdtutil_Alloc(int,netCl->nGate);
    netCl->clauseMapIndex = Pdtutil_Alloc(int,netCl->nGate+nRoots);
    netCl->clauseMapNum = Pdtutil_Alloc(int,netCl->nGate+nRoots);
  }

  //Etichetto il baig identificando i gate root
  aigCnfInfo = DdiGenAigCnfInfo(bmgr,visitedNodes,fA,0);

  //Prendo il massimo cnf id di abc
  maxAbcCnf = Ddi_AigAbcCnfMaxId(aigAbcInfo);

  //Visitando i nodi del baig in pre order
  int maxCnfId = 0;
  int rootNodes = 0;
  for (j=visitedNodes->num-1; j>=0; j--) {
    bAigEdge_t baig = visitedNodes->nodes[j];

    //Ad ogni nodo assegno un mio cnf id
    int a = DdiAig2CnfId(bmgr,baig);
    if(a > maxCnfId) maxCnfId = a;
    if(netCl != NULL) {
      netCl->gateId2cnfId[j] = a;
      netCl->leaders[j] = NULL;
    }

    //Se il nodo non è costante prendo il cnf id ad esso assegnato da ABC
    if (bAig_NodeIsConstant(baig)) continue;
    int abcCnfId = Ddi_AigAbcCnfId(aigAbcInfo,j);

    //Se il nodo non è assegnato ad un cnf id in abc setto isRoot a 0 e continuo
    if (abcCnfId == 0) {
      if (aigCnfInfo[j].isRoot == 2) {
	Pdtutil_Assert (bAig_isVarNode(bmgr,baig),"output node with no cnf");
      }
      else {
	aigCnfInfo[j].isRoot = 0;
      }
      continue;
    }

    rootNodes++;

    //Se no imposto il mapping tra il cnf id di ABC e il nostro
    Pdtutil_Assert(abcCnfId<=maxAbcCnf,"wrong abc cnf id");
    Ddi_AigAbcCnfSetAbc2PdtMap(aigAbcInfo,abcCnfId,a);

    //Se il nodo non è una variabile o una costante lo setto come root (se non è già settato come nodo di output)
    if (!bAig_isVarNode(bmgr,baig) && !bAig_NodeIsConstant(baig)) {
      if (aigCnfInfo[j].isRoot != 2) {
	aigCnfInfo[j].isRoot = 1;
      }
    }
  }


  //Set up leaders information
  if(netCl != NULL){
    for(j=visitedNodes->num-1; j>=0; j--){
      bAigEdge_t baig = visitedNodes->nodes[j];
      int rootCnfId = DdiAig2CnfId(bmgr,baig);
      Pdtutil_Array_t * faninsGateIds = getFaninsGateIds(aigAbcInfo, j, rootCnfId);
      if(faninsGateIds != NULL){
        //Set leader for fanin gates
        int k;
        for(k=0; k < Pdtutil_IntegerArrayNum(faninsGateIds); k++){
          int gateId = Pdtutil_IntegerArrayRead(faninsGateIds, k);
          if(netCl->leaders[gateId] == NULL){
            netCl->leaders[gateId] = Pdtutil_IntegerArrayAlloc(1);
          }
          Pdtutil_IntegerArrayInsertLast(netCl->leaders[gateId], j);
        }
        Pdtutil_IntegerArrayFree(faninsGateIds);
      }
    }
  }

  /*for (j=0; j<visitedNodes->num; j++) {
    if(netCl != NULL && netCl->leaders[j] != NULL){
      bAigEdge_t baig = visitedNodes->nodes[j];
      fprintf(stderr, "Leaders of %d: ", DdiAig2CnfId(bmgr,baig));
      int k;
      for(k=0; k < Pdtutil_IntegerArrayNum(netCl->leaders[j]); k++){
        int gateId = Pdtutil_IntegerArrayRead(netCl->leaders[j], k);
        bAigEdge_t ldrBaig = visitedNodes->nodes[gateId];
        fprintf(stderr, " %d: ", DdiAig2CnfId(bmgr,ldrBaig));
      }
      fprintf(stderr, "\n");
    }
  }*/

  //Scandisce i nodi in post order
  for (j=0; j<visitedNodes->num; j++) {

    //Setta il gateId del nodo e inizializza i campi della netlist
    bAigEdge_t baig = visitedNodes->nodes[j];
    bAig_AuxInt(bmgr,baig) = j;
    if (netCl != NULL) {
      /* init gate info */
      netCl->fiIds[0][j] = netCl->fiIds[1][j] = netCl->clauseMapIndex[j] = 0;
      netCl->clauseMapNum[j] = 0;
      netCl->isRoot[j] = aigCnfInfo[j].isRoot; //0-->interno, 1-->root
    }

    //Per i nodi non variabili o costanti
    if (!bAig_isVarNode(bmgr,baig) && !bAig_NodeIsConstant(baig)) {
      int f, a, b, ir, il;
      bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
      bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);

      Pdtutil_Assert(left!=bAig_NULL && right!=bAig_NULL,"NULL child");

      ir = bAig_AuxInt(bmgr,right);
      il = bAig_AuxInt(bmgr,left);

      //Sistemo i fanin e l'indice delle clausole
      if (netCl != NULL) {
				netCl->fiIds[0][j] = bAig_NodeIsInverted(right) ? -(ir + 1) : (ir + 1);
				netCl->fiIds[1][j] = bAig_NodeIsInverted(left) ? -(il + 1) : (il + 1);
				netCl->clauseMapIndex[j] = caTot->nClauses;
      }

      //Se il nodo è root
      if (aigCnfInfo[j].isRoot) {
	      /* push ldr clauses: f -a -b .... */
	      int k;
	      bAigEdge_t baig = visitedNodes->nodes[j];

	      //Prendo il gate id assegnato da pdtrav
	      int a = DdiAig2CnfId(bmgr,baig);
	      if(netCl != NULL) netCl->gateId2cnfId[j] = a; //M

	      //Prendo l'id della componente della TR a cui l'uscita del gate è mappata (se lo è)
	      int mapId = aigCnfInfo[j].mapLdr;
	      int reMapped=0;

	      //Prendo il vettore di clausole con cui ABC ha codificato il gate rimappandole nei cnf id
	      //di pdtrav e sostituendolo a quello generato, dalla mia funzione di codifica
	      Ddi_ClauseArray_t *ca = Ddi_AigClausesFromAbcCnf (aigAbcInfo,j);
	      Ddi_ClauseArrayFree(aigCnfInfo[j].ca);
	      aigCnfInfo[j].ca = ca;

        //Scandisco il vettore di clausole
	      for (k=0; k<Ddi_ClauseArrayNum(ca); k++) {
	        Ddi_Clause_t *cl = Ddi_ClauseArrayRead(ca,k);
	        Pdtutil_Assert(Ddi_ClauseNumLits(cl)>=1,"wrong n. lits");

	        //Se il nodo corrente è mappato ad una uscita della TR (next state)
	        if (aigCnfInfo[j].isMapped) {

	          //Prendo il baig della variabile di stato futuro e il suo cnf id con segno
	          Ddi_Var_t *v = Ddi_VararrayRead(vA,abs(mapId)-1);
	          bAigEdge_t vBaig = Ddi_VarToBaig(v);
	          int i;
	          int vCnf = DdiAig2CnfId(bmgr,vBaig);
	          if (mapId<0) vCnf = -vCnf;

	          //Scandisco la clausola sostituendo ad ogni letterale nella variabile associata al gate, un corrispondente letterale nella variabile associata allo stato futuro
	          for (i=0;i<cl->nLits; i++) {
	            int lit = cl->lits[i];
	            if (abs(lit)==a) {
		            lit = (lit<0) ? -vCnf : vCnf;
		            cl->lits[i] = lit;
		            reMapped = 1;
	            }
	          }
	          if (reMapped)
	            Ddi_ClauseSort(cl);
	        }

	        //Aggiungo la clausola, eventualmente rimappata, al vettore delle clausole complessivo
	        Ddi_ClauseArrayPush(caTot,cl);
	      }
      }

      //Setto il numero di clausole per il gate (sia che il nodo sia root che non lo sia: se è root num è > 0 se no è 0)
      if (netCl != NULL) {
	      netCl->clauseMapNum[j] = caTot->nClauses - netCl->clauseMapIndex[j];
      }
    }

    //MARCO
    if(bAig_isVarNode(bmgr,baig) && netCl != NULL){
	     	int a = DdiAig2CnfId(bmgr,baig);
        netCl->gateId2cnfId[j] = a;
     }
  }

  //Per ogni stato prendo il nodo del baig per la variabile di stato futuro
  for (i=0; i<nRoots; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    Ddi_Var_t *v_i = Ddi_VararrayRead(vA,i);
    bAigEdge_t fBaig = Ddi_BddToBaig(f_i);
    bAigEdge_t vBaig = Ddi_VarToBaig(v_i);

    //Se lo stato futuro è costante aggiungo alle clausole la clausola unitaria corrispondente
    if (bAig_NodeIsConstant(fBaig)) {
      int vLit;
      /* vLit */
      vLit = (fBaig==bAig_Zero) ? -DdiAig2CnfId(bmgr,vBaig) :
                                   DdiAig2CnfId(bmgr,vBaig);
      if (netCl != NULL) {
	netCl->roots[i] = 0;
	netCl->clauseMapIndex[netCl->nGate+i] = caTot->nClauses;
	netCl->clauseMapNum[netCl->nGate+i] = 1;
      }
      DdiClauseArrayAddClause1(caTot,vLit);
    }
    else {

      //Se non è costante carico le clausole di linking
      int fLit, vCnf;
      vCnf = DdiAig2CnfId(bmgr,vBaig);
      fLit = bAig_NodeIsInverted(fBaig) ? -DdiAig2CnfId(bmgr,fBaig) :
	                                   DdiAig2CnfId(bmgr,fBaig);
      if (netCl != NULL) {
	netCl->roots[i] = bAig_NodeIsInverted(fBaig) ? -(bAig_AuxInt(bmgr,fBaig) + 1) : (bAig_AuxInt(bmgr,fBaig) + 1);
	netCl->clauseMapIndex[netCl->nGate+i] = caTot->nClauses;
	netCl->clauseMapNum[netCl->nGate+i] = 2;
      }
      /* cl: vCnf == fLit */
      DdiClauseArrayAddClause2(caTot,-vCnf,fLit);
      DdiClauseArrayAddClause2(caTot,vCnf,-fLit);
    }
  }

  //Pulisco e ritorno il vettore di clausole
  for (j=0; j<visitedNodes->num; j++) {
    bAigEdge_t baig = visitedNodes->nodes[j];
    bAig_AuxInt(bmgr,baig) = -1;
    Ddi_ClauseArrayFree(aigCnfInfo[j].ca);
  }

  Pdtutil_Free(aigCnfInfo);

  Ddi_AigAbcInfoFree (aigAbcInfo);

  Ddi_AbcUnlock ();

  return (caTot);
}

#endif

/**Function********************************************************************
  Synopsis    [Convert a DDI AIG to a monolitic BDD]
  Description [Convert a DDI AIG to a monolitic BDD]
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
Ddi_ClauseArray_t *
Ddi_AigClauses(
  Ddi_Bdd_t *f,
  int makeRel,
  Ddi_Clause_t *assumeCl
)
{
  return aigClausesIntern(f,makeRel,assumeCl,NULL);
}

/**Function********************************************************************
  Synopsis    [Convert a DDI AIG to a monolitic BDD]
  Description [Convert a DDI AIG to a monolitic BDD]
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
Ddi_ClauseArray_t *
Ddi_AigClausesWithJustify(
  Ddi_Bdd_t *f,
  int makeRel,
  Ddi_Clause_t *assumeCl,
  Ddi_ClauseArray_t *justify
)
{
  return aigClausesIntern(f,makeRel,assumeCl,justify);
}

/**Function********************************************************************
  Synopsis    [Convert a DDI AIG to a monolitic BDD]
  Description [Convert a DDI AIG to a monolitic BDD]
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
static Ddi_ClauseArray_t *
aigClausesIntern(
  Ddi_Bdd_t *f,
  int makeRel,
  Ddi_Clause_t *assumeCl,
  Ddi_ClauseArray_t *justify
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  bAig_Manager_t *bmgr = ddm->aig.mgr;

  Ddi_ClauseArray_t *caTot = Ddi_ClauseArrayAlloc(4);

  bAig_array_t *visitedNodes = bAigArrayAlloc();
  int i, j;
  int aigCnfLevel = 0 && ddm->settings.aig.aigCnfLevel;
  Ddi_AigCnfInfo_t *aigCnfInfo;

  if (Ddi_BddIsPartConj(f)) {
    for (i=0; i<Ddi_BddPartNum(f); i++) {
      Ddi_Bdd_t *f_i = Ddi_BddPartRead(f,i);
      DdiPostOrderAigVisitIntern(bmgr,Ddi_BddToBaig(f_i),visitedNodes,-1);
    }
  }
  else {
    DdiPostOrderAigVisitIntern(bmgr,Ddi_BddToBaig(f),visitedNodes,-1);
  }
  DdiPostOrderAigClearVisitedIntern(bmgr,visitedNodes);


  if (Ddi_BddIsPartConj(f)) {
    for (i=0; i<Ddi_BddPartNum(f); i++) {
      Ddi_Bdd_t *f_i = Ddi_BddPartRead(f,i);
      bAigEdge_t baig = f_i->data.aig->aigNode;
      int fCnf =
        bAig_NodeIsInverted(baig) ? -DdiAig2CnfId(ddm->aig.mgr,baig) :
                             	     DdiAig2CnfId(ddm->aig.mgr,baig);
      if (!makeRel) {
        DdiClauseArrayAddClause1(caTot,fCnf);
      }
      else if (assumeCl!=NULL) {
        DdiClauseAddLiteral(assumeCl,fCnf);
      }
    }
  }
  else {
    bAigEdge_t baig = f->data.aig->aigNode;
    int fCnf = bAig_NodeIsInverted(baig) ? -DdiAig2CnfId(ddm->aig.mgr,baig) :
      DdiAig2CnfId(ddm->aig.mgr,baig);
    if (!makeRel) {
      DdiClauseArrayAddClause1(caTot,fCnf);
    }
    else if (assumeCl!=NULL) {
      DdiClauseAddLiteral(assumeCl,fCnf);
    }
  }

  if (aigCnfLevel>0) {
    aigCnfInfo = DdiGenAigCnfInfo(bmgr,visitedNodes,NULL,1); //TODO: riattivare blocchi
  }

  for (j=visitedNodes->num-1; j>=0; j--) {
    bAigEdge_t baig = visitedNodes->nodes[j];

    if (!bAig_isVarNode(bmgr,baig) && !bAig_NodeIsConstant(baig)) {
      if (aigCnfLevel==0 || aigCnfInfo[j].isRoot) {
        int a = DdiAig2CnfId(bmgr,baig);
      }
    }
  }

  for (j=0; j<visitedNodes->num; j++) {
    bAigEdge_t baig = visitedNodes->nodes[j];

    if (!bAig_isVarNode(bmgr,baig) && !bAig_NodeIsConstant(baig)) {
      int f, a, b;
      bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
      bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);

      Pdtutil_Assert(left!=bAig_NULL && right!=bAig_NULL,"NULL child");

      if (aigCnfLevel==0) {
	/* convert AIG node to cnf clauses */
	/* f = a&b --> (f+!a+!b)(!f+a)(!f+b) */
	f = DdiAig2CnfId(bmgr,baig);
	a = bAig_NodeIsInverted(right) ? -DdiAig2CnfId(bmgr,right) :
                                          DdiAig2CnfId(bmgr,right);
	b = bAig_NodeIsInverted(left) ? -DdiAig2CnfId(bmgr,left) :
                                         DdiAig2CnfId(bmgr,left);

	/* f -a -b */
	DdiClauseArrayAddClause3(caTot,f,-a,-b);
	/* -f a */
	DdiClauseArrayAddClause2(caTot,-f,a);
	/* -f b */
	DdiClauseArrayAddClause2(caTot,-f,b);
	if (justify != NULL) {
	  DdiClauseArrayAddClause1(justify,f);
	  DdiClauseArrayAddClause2(justify,-a,-b);
	}
      }
      else {
	int ir = bAig_AuxInt(bmgr,right);
	int il = bAig_AuxInt(bmgr,left);

	if (aigCnfInfo[ir].isRoot || aigCnfInfo[il].isRoot) {
	  /* cut gate, i.e. not an internal one */
	  int ldr = aigCnfInfo[j].ldr;
	  bAigEdge_t baigLdr = visitedNodes->nodes[ldr];
	  Pdtutil_Assert(aigCnfInfo[ldr].isRoot,"wrong leader");
	  f = DdiAig2CnfId(bmgr,baigLdr);
	  if (aigCnfInfo[ir].isRoot) {
	    Ddi_Clause_t *cl;
	    Pdtutil_Assert(aigCnfInfo[ldr].ca!=NULL,"missing clause array");
	    cl = Ddi_ClauseArrayRead(aigCnfInfo[ldr].ca,0);
	    a = bAig_NodeIsInverted(right) ? -DdiAig2CnfId(bmgr,right) :
	                                      DdiAig2CnfId(bmgr,right);
	    /* -f a */
	    DdiClauseArrayAddClause2(aigCnfInfo[ldr].ca,-f,a);
	    /* f -a ... */
	    Pdtutil_Assert(ldr>=ir,"wrong leader");
	    DdiClauseAddLiteral(cl,-a);
	  }
	  if (aigCnfInfo[il].isRoot) {
	    Ddi_Clause_t *cl;
	    Pdtutil_Assert(aigCnfInfo[ldr].ca!=NULL,"missing clause array");
	    cl = Ddi_ClauseArrayRead(aigCnfInfo[ldr].ca,0);
	    b = bAig_NodeIsInverted(left) ? -DdiAig2CnfId(bmgr,left) :
	                                     DdiAig2CnfId(bmgr,left);
	    /* -f b */
	    DdiClauseArrayAddClause2(aigCnfInfo[ldr].ca,-f,b);
	    /* f -b ... */
	    DdiClauseAddLiteral(cl,-b);
	  }
	}
	if (aigCnfInfo[j].isRoot) {
	  /* push ldr clauses: f -a -b .... */
	  Ddi_ClauseArray_t *ca = aigCnfInfo[j].ca;
	  int k;
	  for (k=0; k<Ddi_ClauseArrayNum(ca); k++) {
	    Ddi_Clause_t *cl = Ddi_ClauseArrayRead(ca,k);
	    Pdtutil_Assert(Ddi_ClauseNumLits(cl)>=2,"wrong n. lits");
	    Ddi_ClauseArrayPush(caTot,cl);
	  }
	  if (justify != NULL) {
	    Ddi_Clause_t *cl0 = Ddi_ClauseArrayRead(aigCnfInfo[j].ca,0);
	    Ddi_ClauseArrayPush(caTot,cl0);
	  }
	}
      }
    }

  }

  if (aigCnfLevel>0) {
    for (j=0; j<visitedNodes->num; j++) {
      bAigEdge_t baig = visitedNodes->nodes[j];
      bAig_AuxInt(bmgr,baig) = -1;
      Ddi_ClauseArrayFree(aigCnfInfo[j].ca);
    }
    Pdtutil_Free(aigCnfInfo);
  }
  bAigArrayFree(visitedNodes);

  return (caTot);
}

/**Function********************************************************************
  Synopsis    [Convert a DDI AIG to a monolitic BDD]
  Description [Convert a DDI AIG to a monolitic BDD]
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
Ddi_ClauseArray_t *
Ddi_bAigArrayClauses(
  Ddi_Mgr_t *ddm,
  bAig_array_t *visitedNodes,
  int i0
)
{
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  int i;
  Ddi_ClauseArray_t *ca = Ddi_ClauseArrayAlloc(4);

  if (i0<0) i0=0;


  for (i=i0; i<visitedNodes->num; i++) {
    bAigEdge_t baig = visitedNodes->nodes[i];

    if (bAig_isVarNode(bmgr,baig)) {
      /* just gen aig2cnf code */
      int f = DdiAig2CnfId(bmgr,baig);
    }
    else if (bAig_NodeIsConstant(baig)) {
      /* do nothing */
    }
    else {
      int f, a, b;
      bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
      bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);

      Pdtutil_Assert(left!=bAig_NULL && right!=bAig_NULL,"NULL child");

      /* convert AIG node to cnf clauses */
      /* f = a&b --> (f+!a+!b)(!f+a)(!f+b) */
      f = DdiAig2CnfId(bmgr,baig);
      a = bAig_NodeIsInverted(right) ? -DdiAig2CnfId(bmgr,right) :
                                        DdiAig2CnfId(bmgr,right);
      b = bAig_NodeIsInverted(left) ? -DdiAig2CnfId(bmgr,left) :
                                       DdiAig2CnfId(bmgr,left);

      /* f -a -b */
      DdiClauseArrayAddClause3(ca,f,-a,-b);
      /* -f a */
      DdiClauseArrayAddClause2(ca,-f,a);
      /* -f b */
      DdiClauseArrayAddClause2(ca,-f,b);
    }

  }

  return (ca);
}

/**Function********************************************************************
  Synopsis    [Convert a DDI AIG to a monolitic BDD]
  Description [Convert a DDI AIG to a monolitic BDD]
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
int
Ddi_ClauseInClauseArray(
  Ddi_ClauseArray_t *ca,
  Ddi_Clause_t *cl
)
{
  int i;
  Ddi_ClauseSort(cl);
  for (i=0; i<ca->nClauses; i++) {
    // Ddi_ClauseSort(ca->clauses[i]);
    if (Ddi_ClauseSubsumed(cl,ca->clauses[i])) {
      return 1;
    }
  }
  return 0;
}

/**Function********************************************************************
  Synopsis    [Convert a DDI AIG to a monolitic BDD]
  Description [Convert a DDI AIG to a monolitic BDD]
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
int
Ddi_ClauseArrayCompact(
  Ddi_ClauseArray_t *ca
)
{
  int i, j, ret=0;

  for (i=0; 0&&i<ca->nClauses; i++) {
    Ddi_ClauseSort(ca->clauses[i]);
  }
  for (i=0; i<ca->nClauses; i++) {
    if (ca->clauses[i]==NULL) continue;
    for (j=i+1; j<ca->nClauses; j++) {
      if (ca->clauses[j]==NULL) continue;
      if (Ddi_ClauseSubsumed(ca->clauses[j],ca->clauses[i])) {
	//	Ddi_ClauseArrayRemove(ca,j);
	//	j--;
	Ddi_ClauseFree(ca->clauses[j]);
	ca->clauses[j]=NULL;
	ret++;
      }
      else if (Ddi_ClauseSubsumed(ca->clauses[i],ca->clauses[j])) {
	Pdtutil_Assert(0,"error in subsumed sorted array");
	Ddi_ClauseArrayRemove(ca,i);
	i--;
	ret++;
	break;
      }
    }
  }
  /* compact NULL pointers */
  for (i=j=0; i<ca->nClauses; i++) {
    if (ca->clauses[i]==NULL) continue;
    ca->clauses[j++] = ca->clauses[i];
  }
  ca->nClauses = j;

  return ret;
}

/**Function********************************************************************
  Synopsis    [Convert a DDI AIG to a monolitic BDD]
  Description [Convert a DDI AIG to a monolitic BDD]
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
int
Ddi_ClauseArrayCompactWithClause(
  Ddi_ClauseArray_t *ca,
  Ddi_Clause_t *c
)
{
  int i, j, ret=0;

  for (j=0; j<ca->nClauses; j++) {
    if (ca->clauses[j]==NULL) continue;
    if (Ddi_ClauseSubsumed(ca->clauses[j],c)) {
      //	Ddi_ClauseArrayRemove(ca,j);
      //	j--;
      Ddi_ClauseFree(ca->clauses[j]);
      ca->clauses[j]=NULL;
      ret++;
    }
    else if (0 && Ddi_ClauseSubsumed(c,ca->clauses[j])) {
      Pdtutil_Assert(0,"error in subsumed sorted array");
    }
  }
  /* compact NULL pointers */
  for (i=j=0; i<ca->nClauses; i++) {
    if (ca->clauses[i]==NULL) continue;
    ca->clauses[j++] = ca->clauses[i];
  }
  ca->nClauses = j;

  return ret;
}

/**Function********************************************************************
  Synopsis    [Convert a DDI AIG to a monolitic BDD]
  Description [Convert a DDI AIG to a monolitic BDD]
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
void
Ddi_ClauseArrayCheckCompact(
  Ddi_ClauseArray_t *ca
)
{
  int i, j, ret=0;
  for (i=0; i<ca->nClauses; i++) {
    for (j=i+1; j<ca->nClauses; j++) {
      Pdtutil_Assert (!Ddi_ClauseSubsumed(ca->clauses[i],ca->clauses[j]),
		      "subsumed redundant clause in clause array");
      Pdtutil_Assert (!Ddi_ClauseSubsumed(ca->clauses[j],ca->clauses[i]),
		      "subsumed redundant clause in clause array");
    }
  }
}

/**Function********************************************************************
  Synopsis    [Check if clause c0 is subsumed by c1]
  Description [Check if clause c0 is subsumed by c1. A clause c0 is
  subsumed by c1 when the literals of c1 are a subset of c0 literals]
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
int
Ddi_ClauseSubsumed(
  Ddi_Clause_t *c0,
  Ddi_Clause_t *c1
)
{
  int i0, i1;
  //  int s=1;

  //  if (c0->nLits<c1->nLits) s=0;
  //  if (c1->signature0 & ~(c0->signature0)) s=0;
  //  if (c1->signature1 & ~(c0->signature1)) s=0;
  if (c0->nLits<c1->nLits) return 0;
  if (c1->signature0 & ~(c0->signature0)) return 0;
  if (c1->signature1 & ~(c0->signature1)) return 0;

  for (i0=i1=0; i0<c0->nLits && i1<c1->nLits;) {
    Pdtutil_Assert(i0==(c0->nLits-1) || abs(c0->lits[i0])<abs(c0->lits[i0+1]),
		   "wrong lit order in clause");
    Pdtutil_Assert(i1==(c1->nLits-1) || abs(c1->lits[i1])<abs(c1->lits[i1+1]),
		   "wrong lit order in clause");
    if (abs(c0->lits[i0])<abs(c1->lits[i1])) {
      i0++;
    }
    else if (abs(c1->lits[i1])<abs(c0->lits[i0])) {
      /* c1 has an extra literal: c0 not subsumed */
      return 0;
    }
    else if (c1->lits[i1] != c0->lits[i0]) {
      /* if literals with different sign return 0 */
      return 0;
    }
    else {
      i0++; i1++;
    }
  }
  /* subsumed if c1 lits ended */
  //  Pdtutil_Assert(!(!s && (i1==c1->nLits)),"subsume chk err");

  return (i1==c1->nLits);
}

/**Function********************************************************************
  Synopsis    [Join clauses by lit union]
  Description [Join clauses by lit union, NULL returned if clauses are
  not compatible: lit conflict]
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
Ddi_Clause_t *
Ddi_ClauseJoin(
  Ddi_Clause_t *c0,
  Ddi_Clause_t *c1
)
{
  Ddi_Clause_t *cl = Ddi_ClauseAlloc(0,c0->nLits);
  int i0, i1;

  Ddi_ClauseSort(c0);
  Ddi_ClauseSort(c1);

  for (i0=i1=0; i0<c0->nLits && i1<c1->nLits;) {
    if (abs(c0->lits[i0])<abs(c1->lits[i1])) {
      /* c0 has an extra literal */
      DdiClauseAddLiteral(cl,c0->lits[i0]);
      i0++;
    }
    else if (abs(c1->lits[i1])<abs(c0->lits[i0])) {
      /* c1 has an extra literal */
      DdiClauseAddLiteral(cl,c1->lits[i1]);
      i1++;
    }
    else if (c1->lits[i1] != c0->lits[i0]) {
      Ddi_ClauseFree(cl);
      /* if literals with different sign return 0 */
      return NULL;
    }
    else {
      DdiClauseAddLiteral(cl,c1->lits[i1]);
      i0++; i1++;
    }
  }
  for (; i0<c0->nLits; i0++) {
    DdiClauseAddLiteral(cl,c0->lits[i0]);
  }
  for (; i1<c1->nLits; i1++) {
    DdiClauseAddLiteral(cl,c1->lits[i1]);
  }

  return (cl);
}

/**Function********************************************************************
  Synopsis    [sort clause by increasing order]
  Description []
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
int
Ddi_ClauseSort(
  Ddi_Clause_t *cl
)
{
  if (cl->sorted) return 0;

  qsort((void **)(cl->lits),cl->nLits,sizeof(int),ClauseLitCompare);
  cl->sorted = 1;

  return 1;
}

/**Function********************************************************************
  Synopsis    [sort clause by increasing order]
  Description []
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
int
Ddi_ClauseArraySort(
  Ddi_ClauseArray_t *ca
)
{
  qsort((void **)(ca->clauses),ca->nClauses,sizeof(Ddi_Clause_t *),
	ClauseCompare);
  return 1;
}

/**Function********************************************************************
  Synopsis    [Convert a DDI AIG to a monolitic BDD]
  Description [Convert a DDI AIG to a monolitic BDD]
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
int
Ddi_ClauseIsBlocked(
  Ddi_SatSolver_t *solver,
  Ddi_Clause_t *cube
)
{
  int ret = (Ddi_SatSolveCustomized(solver,cube,0,0,-1) == 0);
  return ret;
}


/**Function********************************************************************
  Synopsis    [Create a SAT solver]
  Description [Create a SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_SatSolver_t *
Ddi_SatSolverAlloc(
  void
)
{
  return Ddi_SatSolverAllocIntern(0);
}

/**Function********************************************************************
  Synopsis    [Create a SAT solver]
  Description [Create a SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_SatSolver_t *
Ddi_SatSolver22Alloc(
  void
)
{
  return Ddi_SatSolverAllocIntern(1);
}


/**Function********************************************************************
  Synopsis    [Load clause into a SAT solver]
  Description [Load clause into a SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_SatSolverAddClause(
  Ddi_SatSolver_t *solver,
  Ddi_Clause_t *cl
)
{
  int j;
  vec<Lit> minisatClause;
  DdiSatClause2Minisat(solver,minisatClause,cl);
  solver->S->addClause(minisatClause);

#if 0
  if (chkClause!=NULL && !Ddi_SatSolveCustomized(solver, chkClause, 0, 0, -1)) {
    printf("preso\n");
  }
#endif
}

/**Function********************************************************************
  Synopsis    [Load clause into a SAT solver]
  Description [Load clause into a SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_SatSolverAddClauseCustomized(
  Ddi_SatSolver_t *solver,
  Ddi_Clause_t *cl,
  int neg,
  int offset,
  int actLit
)
{
  int j;
  vec<Lit> minisatClause;
  minisatClause.clear();

  if (actLit != 0) {
    /* actLit => cl: -actLit | cl */
    DdiSolverSetVar(solver,actLit);
    minisatClause.push(DdiLit2Minisat(solver,-actLit));
  }
  for (j=0; j<cl->nLits; j++) {
    int lit = cl->lits[j];
    if (lit==0) continue;
    if (neg) lit = -lit;
    if (offset != 0) {
      lit += (lit>0) ? offset: -offset;
    }
    DdiSolverSetVar(solver,lit);
    minisatClause.push(DdiLit2Minisat(solver,lit));
  }

  solver->S->addClause(minisatClause);
}


/**Function********************************************************************
  Synopsis    [Call SAT solver]
  Description [Call SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_SatSolverOkay(
  Ddi_SatSolver_t *solver
)
{
  return (solver->S->okay()==true);
}

/**Function********************************************************************
  Synopsis    [Call SAT solver]
  Description [Call SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_SatSolveCustomized(
  Ddi_SatSolver_t *solver,
  Ddi_Clause_t *assume,
  int neg,
  int offset,
  int timeLimit
)
{
  int ret;
  int j;
  vec<Lit> assumeClause;

  assumeClause.clear();

  if (assume!=NULL) {
    for (j=0; j<assume->nLits; j++) {
      int lit = assume->lits[j];
      if (lit==0) continue;
      if (neg) lit = -lit;
      if (offset != 0) {
        lit += (lit>0) ? offset: -offset;
      }
      DdiSolverSetVar(solver,lit);
      assumeClause.push(DdiLit2Minisat(solver,lit));
    }
  }

  Pdtutil_Assert (solver->S->okay()==true, "solver is not OK");
  ret = solver->S->solve(assumeClause,timeLimit);

  return ret;

}

/**Function********************************************************************
  Synopsis    [Call SAT solver]
  Description [Call SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_SatSolverSimplify(
  Ddi_SatSolver_t *solver
)
{

  solver->S->simplifyDB();

  return 1;
}

/**Function********************************************************************
  Synopsis    [Call SAT solver]
  Description [Call SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_SatCexIncludedInLit(
  Ddi_SatSolver_t *solver,
  int lit
)
{
  int v = abs(lit);
  lbool val;
  Pdtutil_Assert(v>0,"wrong literal in var clause");
  val = DdiSolverReadModel(solver,v);
  if (val == l_Undef) {
    Pdtutil_Assert(0,"undef var in Sat cex");
  }
  else if (val == l_True) {
    return lit>0;
  }
  else if (val == l_False) {
    return lit<0;
  }
  Pdtutil_Assert(0,"error in cex inclusion");
}

/**Function********************************************************************
  Synopsis    [Call SAT solver]
  Description [Call SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_SatSolverGetCexCube(
  Ddi_SatSolver_t *solver,
  Ddi_Clause_t *vars
)
{
  int j;
  Ddi_Clause_t *cl = Ddi_ClauseAlloc(0,vars->nLits);

  for (j=0; j<vars->nLits; j++) {
    int v = vars->lits[j];
    lbool val;
    Pdtutil_Assert(v>0,"wrong literal in var clause");
    val = DdiSolverReadModel(solver,v);
    if (val == l_Undef) {
      // Pdtutil_Assert(0,"undef var in Sat cex");
    }
    else if (val == l_True) {
    }
    else if (val == l_False) {
      v = -v;
    }
    DdiClauseAddLiteral(cl,v);
  }

  return cl;
}

/**Function********************************************************************
  Synopsis    [Call SAT solver]
  Description [Call SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_SatSolverGetCexCubeAig(
  Ddi_SatSolver_t *solver,
  Ddi_Clause_t *vars,
  Ddi_Vararray_t *varsAig
)
{
  int j;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(varsAig);
  Ddi_Bdd_t *resBdd = Ddi_BddMakeConstAig(ddm,1);

  for (j=0; j<vars->nLits; j++) {
    int v = vars->lits[j];
    lbool val;
    Ddi_Bdd_t *litBdd;
    Ddi_Var_t *vAig = Ddi_VararrayRead(varsAig,j);
    Pdtutil_Assert(v>0,"wrong literal in var clause");
    val = DdiSolverReadModel(solver,v);
    if (val == l_Undef) {
      //Pdtutil_Assert(0,"undef var in Sat cex");
      //litBdd = Ddi_BddMakeLiteralAig(vAig,0);
      continue;
    }
    else if (val == l_True) {
      litBdd = Ddi_BddMakeLiteralAig(vAig,1);
      Ddi_BddAndAcc(resBdd,litBdd);
    }
    else if (val == l_False) {
      litBdd = Ddi_BddMakeLiteralAig(vAig,0);
      Ddi_BddAndAcc(resBdd,litBdd);
    }
    Ddi_Free(litBdd);
  }

  return resBdd;
}

/**Function********************************************************************
  Synopsis    [Call SAT solver]
  Description [Call SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_SatSolverGetCexCubeWithJustify(
  Ddi_SatSolver_t *solver,
  Ddi_Clause_t *vars,
  Ddi_ClauseArray_t *justify,
  Ddi_Clause_t *implied
)
{
  int j, k, nv = solver->nVars;
  Ddi_Clause_t *cl = Ddi_ClauseAlloc(0,vars->nLits);
  char *justified = Pdtutil_Alloc(char, nv+1);

  for (j=1; j<=nv; j++) {
    justified[j] = (char)0;
  }

  for (j=0; j<implied->nLits; j++) {
    int lit = implied->lits[j];
    Pdtutil_Assert(abs(lit)<=nv,"invalid implied literal");
    justified[abs(lit)] = (char)(lit<0 ? -1 : 1);
  }

  for (j=justify->nClauses-2; j>=0; j-=2) {
    Ddi_Clause_t *cl = justify->clauses[j];
    int refLit = cl->lits[0];
    cl = justify->clauses[j+1];
    Pdtutil_Assert(abs(refLit)<=nv,"invalid implied literal");
    if (justified[abs(refLit)] == 0) continue;
    if (refLit * justified[abs(refLit)] > 0) {
      /* same sign: f + a + b * ... , f implied */
      /* imply all terms: !a !b ... */
      for (k=0; k<cl->nLits; k++) {
	int lit = -cl->lits[k]; /* take complement of literal */
	int v = abs(lit);
	lbool val;
	Pdtutil_Assert(abs(lit)<=nv,"invalid implied literal");
	/* check solver value */
	Pdtutil_Assert(v>0,"wrong literal in var clause");
	val = DdiSolverReadModel(solver,v);
	if (val == l_Undef) {
	  Pdtutil_Assert(0,"undef var in Sat cex");
	}
	else if (val == l_True) {
	  Pdtutil_Assert(lit>0,"error in sat lit phase");
	}
	else if (val == l_False) {
	  Pdtutil_Assert(lit<0,"error in sat lit phase");
	}
	justified[abs(lit)] = (char)(lit<0 ? -1 : 1);
      }
    }
    else {
      /* diff sign: f + a + b * ... , !f implied */
      /* imply first term set by solver */
      for (k=0; k<cl->nLits; k++) {
	int lit = cl->lits[k]; /* take regular literal */
	int v = abs(lit);
	lbool val;
	int useLit = 0;
	Pdtutil_Assert(abs(lit)<=nv,"invalid implied literal");
	/* check solver value */
	Pdtutil_Assert(v>0,"wrong literal in var clause");
	val = DdiSolverReadModel(solver,v);
	if (val == l_Undef) {
	  Pdtutil_Assert(0,"undef var in Sat cex");
	}
	else if (val == l_True) {
	  useLit = (lit>0);
	}
	else if (val == l_False) {
	  useLit = (lit<0);
	}
	if (useLit) {
	  justified[abs(lit)] = (char)(lit<0 ? -1 : 1);
	  break;
	}
      }
      Pdtutil_Assert(k==1 || k<cl->nLits,"missing impl literal");
    }
  }

  for (j=0; j<vars->nLits; j++) {
    int v = vars->lits[j];
    lbool val;
    Pdtutil_Assert(v<=nv,"invalid cex var");
    Pdtutil_Assert(v>0,"wrong literal in var clause");
    if (justified[v] == 0) continue;
    val = DdiSolverReadModel(solver,v);
    if (val == l_Undef) {
      Pdtutil_Assert(0,"undef var in Sat cex");
    }
    else if (val == l_True) {
    }
    else if (val == l_False) {
      v = -v;
    }
    DdiClauseAddLiteral(cl,v);
  }

  Pdtutil_Free(justified);

  return cl;
}

/**Function********************************************************************
  Synopsis    [Call SAT solver]
  Description [Call SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_SatSolverCexCore (
  Ddi_SatSolver_t *solver,
  Ddi_Clause_t *cexPs,
  Ddi_Clause_t *cexPi,
  Ddi_Clause_t *cubePs,
  int maxVar
)
{
  int sat, actLit = DdiSatSolverGetFreeVar(solver);
  Ddi_Clause_t *assume = Ddi_ClauseJoin(cexPi,cexPs);
  Ddi_Clause_t *core;

  Ddi_SatSolverAddClauseCustomized(solver,cubePs,1,1,actLit);

  DdiClauseAddLiteral(assume, actLit);

  sat = Ddi_SatSolve(solver,assume,-1);
  Pdtutil_Assert(!sat,"unsat needed for cex core");

  core = Ddi_SatSolverFinal(solver,maxVar);

  Ddi_ClauseNegLitsAcc(core);

  Ddi_ClauseFree(assume);

  return core;
}

/**Function********************************************************************
  Synopsis    [Call SAT solver]
  Description [Call SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_SatSolverCubeConstrain (
  Ddi_SatSolver_t *solver,
  Ddi_Clause_t *cube
)
{
  int sat, actLit = DdiSatSolverGetFreeVar(solver);
  Ddi_Clause_t *assume = Ddi_ClauseDup(cube);
  Ddi_Clause_t *cubeConstr;
  int nRemoved=0;
  int j;

  for (j=assume->nLits-1; j>=0; j--) {
    assume->lits[j] = -assume->lits[j]; 
    if (Ddi_SatSolve(solver,assume,-1)) {
      assume->lits[j] = -assume->lits[j]; 
    }
    else {
      // flipping literal leads to UNSAT
      // remove literal: becomes don't care
      assume->lits[j] = assume->lits[assume->nLits-1];
      assume->nLits--;
      nRemoved++;
    }
  }

  Pdtutil_Assert(assume->nLits>0,"no literal left in constraint");

  cubeConstr = Ddi_ClauseCompact(assume);
  Ddi_ClauseSort(cubeConstr);

  Ddi_ClauseFree(assume);

  return cubeConstr;
}


/**Function********************************************************************
  Synopsis    [ternary evaluation]
  Description [ternary evaluation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_TernaryPdrSimulate
(
  Ddi_TernarySimMgr_t *xMgr,
  int restart,                            //Restarts simulation
  Ddi_Clause_t *piBinaryVals,
  Ddi_Clause_t *psBinaryVals,
  Ddi_Clause_t *poBinaryVals,
  int useTarget,
  int xLit
)
{
  int i, enSim=0, okX;
  int evDriven=1;
  int nPi = xMgr->nPi;
  int nPs = xMgr->nPs;

  static int ncalls=0;

  if (restart) {
    for (i=0; i<xMgr->nGate; i++) {
      xMgr->ternaryGates[i].observed = 0;
      if (restart>1) {
	xMgr->ternaryGates[i].val = xMgr->ternaryGates[i].saveVal = 'x';
	if (xMgr->ternaryGates[i].type == Ddi_Const_c) {
	  xMgr->ternaryGates[i].val = xMgr->ternaryGates[i].saveVal = '0';
	}
      }
    }
    if (evDriven) tSimStackReset(xMgr);
  }
  if (piBinaryVals != NULL) {
    enSim = 1;
    for (i=0; i<piBinaryVals->nLits; i++) {
      int piId, gateId;
      int lit = piBinaryVals->lits[i];
      int var = DdiPdrLit2Var(lit);
      Pdtutil_Assert(DdiVarIsPi(nPs,nPi,var),"Pi var required");
      piId = DdiVarPiIndex(nPs,nPi,var);
      Pdtutil_Assert(piId>=0 && piId<xMgr->nPi,"wrong Pi inex");
      gateId = xMgr->pi[piId];
      if (gateId==-1) continue; // skip non used input
      Pdtutil_Assert(gateId>=0 && gateId<xMgr->nGate,"wrong Gate inex");
      xMgr->ternaryGates[gateId].val = DdiPdrLitIsCompl(lit)?'0':'1';
      if (evDriven) tSimStackFoPush(xMgr,gateId);
    }
  }
  if (psBinaryVals != NULL) {
    enSim = 1;
    for (i=0; i<psBinaryVals->nLits; i++) {
      int psId, gateId;
      int lit = psBinaryVals->lits[i];
      int var = DdiPdrLit2Var(lit);
      Pdtutil_Assert(DdiVarIsPs(nPs,var),"Ps var required");
      psId = DdiVarPsIndex(nPs,var);
      Pdtutil_Assert(psId>=0 && psId<xMgr->nPs,"wrong Ps inex");
      gateId = xMgr->ps[psId];
      Pdtutil_Assert(gateId>=0 && gateId<xMgr->nGate,"wrong Gate inex");
      xMgr->ternaryGates[gateId].val = DdiPdrLitIsCompl(lit)?'0':'1';
      if (evDriven) tSimStackFoPush(xMgr,gateId);
    }
  }

  if (useTarget) {
    char reqVal;
    int gateId;
    Pdtutil_Assert(poBinaryVals==NULL,"usetarget not compatible with po cube");
    gateId = abs(xMgr->target)-1;
    reqVal = (xMgr->target<0) ? '0':'1';
    Pdtutil_Assert(gateId>=0 && gateId<xMgr->nGate,"wrong Gate index");
    xMgr->ternaryGates[gateId].reqVal = reqVal;
    xMgr->ternaryGates[gateId].observed = 1;
    enSim = 1;
  }
  else if (poBinaryVals != NULL) {
    enSim = 1;
    for (i=0; i<poBinaryVals->nLits; i++) {
      int poId, gateId;
      int lit = poBinaryVals->lits[i];
      int var = DdiPdrLit2Var(lit);
      char reqVal;
      Pdtutil_Assert(DdiVarIsPs(nPs,var),"Ps var required");
      poId = DdiVarPsIndex(nPs,var);
      Pdtutil_Assert(poId>=0 && poId<xMgr->nPo,"wrong Ps index");
      gateId = abs(xMgr->po[poId])-1;
      if (xMgr->po[poId]<0) lit = -lit;
      reqVal = lit<0 ? '0':'1';
      Pdtutil_Assert(gateId>=0 && gateId<xMgr->nGate,"wrong Gate index");
      xMgr->ternaryGates[gateId].reqVal = reqVal;
      xMgr->ternaryGates[gateId].observed = 1;
    }
  }

  okX = 1;

  if (xLit!=0) {
    int psId, gateId;
    int var = DdiPdrLit2Var(xLit);
    Pdtutil_Assert(DdiVarIsPs(nPs,var),"Ps var required");
    psId = DdiVarPsIndex(nPs,var);
    Pdtutil_Assert(psId>=0 && psId<xMgr->nPs,"wrong Ps inex");
    gateId = xMgr->ps[psId];
    Pdtutil_Assert(gateId>=0 && gateId<xMgr->nGate,"wrong Gate inex");

    if (xMgr->ternaryGates[gateId].observed) {
      okX = 0;
      enSim = 0;
    }
    else {
      xMgr->ternaryGates[gateId].saveVal = xMgr->ternaryGates[gateId].val;
      xMgr->ternaryGates[gateId].val = 'x';
      xMgr->undoList[xMgr->undoN++] = gateId;
      enSim = 2;
      if (evDriven) tSimStackFoPush(xMgr,gateId);
    }
  }

  if (enSim) {

    ncalls++;

    for (i=evDriven?tSimStackPop(xMgr):0;
	 i>=0 && i<xMgr->nGate;
	 i=evDriven?tSimStackPop(xMgr):i+1) {
      char newVal;
      if (xMgr->ternaryGates[i].type == Ddi_Const_c) {
	xMgr->ternaryGates[i].val = '0';
      }
      else if (xMgr->ternaryGates[i].type == Ddi_And_c) {
	int ir = xMgr->ternaryGates[i].fiIds[0];
	int il = xMgr->ternaryGates[i].fiIds[1];
	char vr = xMgr->ternaryGates[abs(ir)-1].val;
	char vl = xMgr->ternaryGates[abs(il)-1].val;
	if (vr!='x' && ir<0) vr = (vr=='0'?'1':'0');
	if (vl!='x' && il<0) vl = (vl=='0'?'1':'0');
	if (vl=='0' || vr=='0') {
	  newVal = '0';
	}
	else if (vl=='x' || vr=='x') {
	  newVal = 'x';
	}
	else {
	  Pdtutil_Assert(vl=='1' && vr=='1',"wrong and vals");
	  newVal = '1';
	}
	if (newVal!=xMgr->ternaryGates[i].val) {
	  if (xMgr->ternaryGates[i].observed &&
	      // xMgr->ternaryGates[i].val!='x' &&
	      newVal!=xMgr->ternaryGates[i].reqVal) {
	    Pdtutil_Assert(!restart,"error resetting simulation");
	    okX = 0;
	    if (enSim==2) {
	      /* checking x propagation. stop at first difference */
	      break;
	    }
	  }
	  xMgr->ternaryGates[i].saveVal = xMgr->ternaryGates[i].val;
	  xMgr->ternaryGates[i].val = newVal;
	  if (enSim==2) {
	    xMgr->undoList[xMgr->undoN++] = i;
	  }
	  if (evDriven) tSimStackFoPush(xMgr,i);
	}
	else {
	  //	  printf("AAA\n");
	}
	if (xMgr->ternaryGates[i].observed) {
	  Pdtutil_Assert(
	    xMgr->ternaryGates[i].reqVal=='x' ||
	    xMgr->ternaryGates[i].reqVal==xMgr->ternaryGates[i].val,
	    "problems with ternary sim");
	}
      }
      if (enSim==1 && xMgr->ternaryGates[i].observed) {
	Pdtutil_Assert(xMgr->ternaryGates[i].val==xMgr->ternaryGates[i].reqVal,
		       "error on observed gate");
      }

    }

    if (evDriven) tSimStackReset(xMgr);

    if (enSim==2) {
      if (okX) {
	xMgr->undoN = 0;
      }
      else while (xMgr->undoN > 0) {
	i = xMgr->undoList[--xMgr->undoN];
	xMgr->ternaryGates[i].val = xMgr->ternaryGates[i].saveVal;
	//	xMgr->ternaryGates[i].observed = 1; !!!!
      }
    }

  }

  if (poBinaryVals != NULL) {
    okX = 1;
    for (i=0; i<poBinaryVals->nLits; i++) {
      int poId, gateId;
      int lit = poBinaryVals->lits[i];
      int var = DdiPdrLit2Var(lit);
      char reqVal;
      Pdtutil_Assert(DdiVarIsPs(nPs,var),"Ps var required");
      poId = DdiVarPsIndex(nPs,var);
      Pdtutil_Assert(poId>=0 && poId<xMgr->nPo,"wrong Ps index");
      gateId = abs(xMgr->po[poId])-1;
      if (xMgr->po[poId]<0) lit = -lit;
      reqVal = lit<0 ? '0':'1';
      Pdtutil_Assert(gateId>=0 && gateId<xMgr->nGate,"wrong Gate inex");
      Pdtutil_Assert(xMgr->ternaryGates[gateId].val == reqVal,
		     "error on observed output");
      if (xMgr->ternaryGates[gateId].val != reqVal) {
	okX=0;
        Pdtutil_Assert(xMgr->ternaryGates[gateId].val == 'x',
		     "error on simulation of observed output");
	break;
      }
    }
  }

  return okX;

}


/**Function********************************************************************
  Synopsis    [ternary evaluation]
  Description [ternary evaluation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_TernaryPdrJustify
(
 Ddi_PdrMgr_t *pdrMgr,
 Ddi_TernarySimMgr_t *xMgr,
 Ddi_Clause_t *psBinaryVals,
 int evDriven
)
{
  int i;
  Ddi_Clause_t *clMin = Ddi_ClauseAlloc(0,1);

  //  if (evDriven) tSimStackReset(xMgr);

  for (i=xMgr->nGate-1; i>=0; i--) {
    char myVal = xMgr->ternaryGates[i].val;
    if (!xMgr->ternaryGates[i].observed) continue;
    if (xMgr->ternaryGates[i].type == Ddi_And_c) {
      int ir = xMgr->ternaryGates[i].fiIds[0];
      int il = xMgr->ternaryGates[i].fiIds[1];
      char vr = xMgr->ternaryGates[abs(ir)-1].val;
      char vl = xMgr->ternaryGates[abs(il)-1].val;
      if (vr!='x' && ir<0) vr = (vr=='0'?'1':'0');
      if (vl!='x' && il<0) vl = (vl=='0'?'1':'0');
      switch (myVal) {
      case '0':
	if ((vr=='0' && xMgr->ternaryGates[abs(ir)-1].observed) ||
	    (vl=='0' && xMgr->ternaryGates[abs(il)-1].observed)) {
	  /* already justified: keep as it is */
	}
	else if (0&&vr=='0'&&vl=='0') {
	  /* justify right or left at choice: choose lowest level */
	  if (xMgr->ternaryGates[abs(ir)-1].level <
	      xMgr->ternaryGates[abs(il)-1].level)
	    xMgr->ternaryGates[abs(ir)-1].observed = 2;
	  else
	    xMgr->ternaryGates[abs(il)-1].observed = 2;
	}
	else if (vr=='0') {
	  /* justify right */
	  xMgr->ternaryGates[abs(ir)-1].observed = 2;
          //          xMgr->undoList[xMgr->undoN++] = ir;
          //          tSimStackPush(xMgr,ir);
	}
	else if (vl=='0') {
	  /* justify left */
	  xMgr->ternaryGates[abs(il)-1].observed = 2;
          //          xMgr->undoList[xMgr->undoN++] = il;
          //          tSimStackPush(xMgr,il);
	}
	else {
	  Pdtutil_Assert(0,"cannot justify 0-and");
	}
	break;
      case '1':
	Pdtutil_Assert(vr=='1'&&vl=='1',"cannot justify 1-and");
	if (!xMgr->ternaryGates[abs(ir)-1].observed) {
	  xMgr->ternaryGates[abs(ir)-1].observed = 2;
          //          xMgr->undoList[xMgr->undoN++] = ir;
          //          tSimStackPush(xMgr,ir);
        }
	if (!xMgr->ternaryGates[abs(il)-1].observed) {
	  xMgr->ternaryGates[abs(il)-1].observed = 2;
          //          xMgr->undoList[xMgr->undoN++] = il;
          //          tSimStackPush(xMgr,il);
        }
	break;
      case 'x':
	Pdtutil_Assert(0,"cannot justify x-and");
	break;
      }
    }
  }

  for (i=0; i<xMgr->nGate; i++) {
    if (!xMgr->ternaryGates[i].observed) {
      xMgr->ternaryGates[i].val = 'x';
    }
    else {
      if (xMgr->ternaryGates[i].observed>1) {
	/* reset justification flag */
	xMgr->ternaryGates[i].observed = 0;
      }
    }
  }

  Pdtutil_Assert (psBinaryVals != NULL,"missing domain clause in justify");
  for (i=0; i<psBinaryVals->nLits; i++) {
    int psId, gateId;
    int lit = psBinaryVals->lits[i];
    int var = DdiPdrLit2Var(lit);
    Pdtutil_Assert(DdiPdrVarIsPs(pdrMgr,var),"Ps var required");
    psId = DdiPdrVarPsIndex(pdrMgr,var);
    Pdtutil_Assert(psId>=0 && psId<xMgr->nPs,"wrong Ps inex");
    gateId = xMgr->ps[psId];
    Pdtutil_Assert(gateId>=0 && gateId<xMgr->nGate,"wrong Gate inex");
    if (xMgr->ternaryGates[gateId].val != 'x') {
      /* justified */
      DdiClauseAddLiteral(clMin,psBinaryVals->lits[i]);
    }
  }

  return clMin;

}

/**Function********************************************************************

  Synopsis    [Inner steps of aig to BDD conversion]
  Description [Inner steps of aig to BDD conversion]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_TernarySimMgr_t *
Ddi_TernarySimInit
(
  Ddi_Bddarray_t *fA,
  Ddi_Bdd_t *invarspec,
  Ddi_Vararray_t *pi,
  Ddi_Vararray_t *ps
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(fA);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  Ddi_TernarySimMgr_t *xMgr;
  bAig_array_t *visitedNodes;
  int i;


  xMgr = Pdtutil_Alloc(Ddi_TernarySimMgr_t,1);
  xMgr->ddiMgr = ddm;
  xMgr->nPo = Ddi_BddarrayNum(fA);
  xMgr->nPi = Ddi_VararrayNum(pi);
  xMgr->nPs = Ddi_VararrayNum(ps);

  visitedNodes = bAigArrayAlloc();

  for (i=0; i<xMgr->nPo; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    bAigEdge_t nodeIndex = Ddi_BddToBaig(f_i);
    DdiPostOrderAigVisitIntern(bmgr,nodeIndex,visitedNodes,-1);
  }
  if (invarspec!=NULL) {
    bAigEdge_t nodeIndex = Ddi_BddToBaig(invarspec);
    DdiPostOrderAigVisitIntern(bmgr,nodeIndex,visitedNodes,-1);
  }
  DdiPostOrderAigClearVisitedIntern(bmgr,visitedNodes);

  xMgr->nGate = visitedNodes->num;
  xMgr->ternaryGates = Pdtutil_Alloc(ternaryGateInfo,xMgr->nGate);
  xMgr->po = Pdtutil_Alloc(int,xMgr->nPo);
  xMgr->pi = Pdtutil_Alloc(int,xMgr->nPi);
  xMgr->ps = Pdtutil_Alloc(int,xMgr->nPs);
  xMgr->undoList = Pdtutil_Alloc(int,xMgr->nGate+xMgr->nPo);
  xMgr->undoN = 0;
  xMgr->nLevels = 0;
  xMgr->currLevel = 0;
  xMgr->target = 0;

  for (i=0; i<visitedNodes->num; i++) {
    bAigEdge_t baig = visitedNodes->nodes[i];
    bAig_AuxInt(bmgr,baig) = i;
    xMgr->ternaryGates[i].foIds = NULL;
    xMgr->ternaryGates[i].inStack = 0;
    xMgr->ternaryGates[i].foCnt = 0;
    xMgr->ternaryGates[i].observed = 0;
    xMgr->ternaryGates[i].saveVal = 'x';
    xMgr->ternaryGates[i].reqVal = 'x';
    xMgr->ternaryGates[i].val = 'x';
    xMgr->ternaryGates[i].level = 0;
    if (bAig_NodeIsConstant(baig)) {
      xMgr->ternaryGates[i].type = Ddi_Const_c;
      /* non complemented constant is 0 */
      xMgr->ternaryGates[i].val = '0';
    }
    else if (bAig_isVarNode(bmgr,baig)) {
      xMgr->ternaryGates[i].type = Ddi_FreePi_c;
    }
    else {
      int levelR, levelL;
      bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
      bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);
      int ir = bAig_AuxInt(bmgr,right);
      int il = bAig_AuxInt(bmgr,left);
      xMgr->ternaryGates[i].type = Ddi_And_c;
      Pdtutil_Assert(ir>=0 && il>=0,"Invalid aux int");
      Pdtutil_Assert(xMgr->ternaryGates[ir].foCnt>=0,"fo cnt not initialized");
      Pdtutil_Assert(xMgr->ternaryGates[il].foCnt>=0,"fo cnt not initialized");
      xMgr->ternaryGates[ir].foCnt++;
      xMgr->ternaryGates[il].foCnt++;
      xMgr->ternaryGates[i].fiIds[0]=bAig_NodeIsInverted(right)?-(ir+1):(ir+1);
      xMgr->ternaryGates[i].fiIds[1]=bAig_NodeIsInverted(left)?-(il+1):(il+1);
      levelR = xMgr->ternaryGates[ir].level;
      levelL = xMgr->ternaryGates[il].level;
      xMgr->ternaryGates[i].level = (levelR<levelL) ? levelL+1 : levelR+1;
      if (xMgr->ternaryGates[i].level >= xMgr->nLevels) {
	xMgr->nLevels = xMgr->ternaryGates[i].level+1;
      }
    }
  }

  for (i=0; i<xMgr->nPi; i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(pi,i);
    bAigEdge_t varIndex = Ddi_VarBaigId(v);
    int id = bAig_AuxInt(bmgr,varIndex);
    xMgr->pi[i] = id;
    if (id<0) continue;
    Pdtutil_Assert(id>=0 && id<=xMgr->nGate,"pi error in ternary init");
    Pdtutil_Assert(xMgr->ternaryGates[id].type==Ddi_FreePi_c,
      "pi error in ternary init");
    xMgr->ternaryGates[id].type=Ddi_Pi_c;
  }

  for (i=0; i<xMgr->nPs; i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(ps,i);
    bAigEdge_t varIndex = Ddi_VarBaigId(v);
    int id = bAig_AuxInt(bmgr,varIndex);
    xMgr->ps[i] = id;
    if (id<0) continue;
    Pdtutil_Assert(id>=0 && id<=xMgr->nGate,"pi error in ternary init");
    Pdtutil_Assert(xMgr->ternaryGates[id].type==Ddi_FreePi_c,
      "pi error in ternary init");
    xMgr->ternaryGates[id].type=Ddi_Ps_c;
  }

  for (i=0; i<xMgr->nPo; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    bAigEdge_t nodeIndex = Ddi_BddToBaig(f_i);
    int id = bAig_AuxInt(bmgr,nodeIndex);

    Pdtutil_Assert(id>=0 && id<xMgr->nGate,"pi error in ternary init");
    xMgr->po[i] = bAig_NodeIsInverted(nodeIndex)?-(id+1):(id+1);
  }
  if (invarspec!=NULL) {
    bAigEdge_t nodeIndex = Ddi_BddToBaig(invarspec);
    int id = bAig_AuxInt(bmgr,nodeIndex);
    Pdtutil_Assert(id>=0 && id<xMgr->nGate,"pi error in ternary init");
    /* target is complemented w.r.t. invarspec */
    xMgr->target = bAig_NodeIsInverted(nodeIndex)?(id+1):-(id+1);
  }

  xMgr->levelSize = Pdtutil_Alloc(int,xMgr->nLevels);
  xMgr->levelStackId = Pdtutil_Alloc(int,xMgr->nLevels);
  xMgr->levelStack = Pdtutil_Alloc(int *,xMgr->nLevels);

  for (i=0; i<xMgr->nLevels; i++) {
    xMgr->levelSize[i] = 0;
  }

  /* alloc fanout arrays and setup fanout info */
  for (i=0; i<xMgr->nGate; i++) {
    int ir, il, level;
    if (xMgr->ternaryGates[i].foCnt>0) {
      xMgr->ternaryGates[i].foIds =
	Pdtutil_Alloc(int,xMgr->ternaryGates[i].foCnt);
      xMgr->ternaryGates[i].foCnt = 0;
    }

    if (xMgr->ternaryGates[i].type!=Ddi_And_c) continue;

    ir = abs(xMgr->ternaryGates[i].fiIds[0])-1;
    il = abs(xMgr->ternaryGates[i].fiIds[1])-1;
    Pdtutil_Assert(ir>=0 && il>=0,"wrong fanin id");
    Pdtutil_Assert((ir<xMgr->nGate)&&(il<xMgr->nGate),"error in x init");
    Pdtutil_Assert(xMgr->ternaryGates[ir].foCnt>=0,"fo cnt not initialized");
    Pdtutil_Assert(xMgr->ternaryGates[il].foCnt>=0,"fo cnt not initialized");
    xMgr->ternaryGates[ir].foIds[xMgr->ternaryGates[ir].foCnt]=i;
    xMgr->ternaryGates[il].foIds[xMgr->ternaryGates[il].foCnt]=i;
    xMgr->ternaryGates[ir].foCnt++;
    xMgr->ternaryGates[il].foCnt++;

    level = xMgr->ternaryGates[i].level;
    Pdtutil_Assert(level<xMgr->nLevels,"error in gate levels");
    xMgr->levelSize[level]++;
  }

  for (i=0; i<xMgr->nLevels; i++) {
    int lSize = xMgr->levelSize[i];
    xMgr->levelStack[i] = Pdtutil_Alloc(int,lSize);
    xMgr->levelStackId[i] = 0;
  }

  for (i=0; i<visitedNodes->num; i++) {
    bAigEdge_t baig = visitedNodes->nodes[i];
    bAig_AuxInt(bmgr,baig) = -1;
  }

  bAigArrayFree(visitedNodes);

  return (xMgr);
}

/**Function********************************************************************

  Synopsis    [Inner steps of aig to BDD conversion]
  Description [Inner steps of aig to BDD conversion]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_TernarySimQuit
(
  Ddi_TernarySimMgr_t *xMgr
)
{
  int i;

  Pdtutil_Free(xMgr->ternaryGates);

  Pdtutil_Free(xMgr->po);
  Pdtutil_Free(xMgr->pi);
  Pdtutil_Free(xMgr->ps);
  Pdtutil_Free(xMgr->undoList);

  for (i=0; i<xMgr->nLevels; i++) {
    Pdtutil_Free(xMgr->levelStack[i]);
  }

  Pdtutil_Free(xMgr->levelSize);
  Pdtutil_Free(xMgr->levelStack);
  Pdtutil_Free(xMgr->levelStackId);

  Pdtutil_Free(xMgr);
}


/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_EqClassesRefine(
  Ddi_PdrMgr_t *pdrMgr,
  Pdtutil_EqClasses_t *eqCl,
  Ddi_SatSolver_t *solver,
  Ddi_Vararray_t *vars,
  Ddi_Clause_t *clauseVars
)
{
  int nVars = Ddi_VararrayNum(vars);
  int i, eqFound=0, nEq=0, nEqc=0, refined=0, refinementDone;

  Ddi_Clause_t *assumeClause, *assumeSat;
  Ddi_ClauseArray_t *splitClasses[2];
  Ddi_Clause_t *splitClass0;
  Ddi_Clause_t *splitClass1;

  if (nVars==0) return 0;

  /* generate activation vars and clause */
  assumeClause = Ddi_ClauseAlloc(0,nVars+1);

  for (i=0; i<=nVars; i++) {
    int actV = DdiSatSolverGetFreeVar(solver);
    Pdtutil_EqClassCnfActWrite(eqCl,i,actV);
    /* at least one class split */
    if (Pdtutil_EqClassState(eqCl,i) != Pdtutil_EqClass_Singleton_c) {
      /* not singleton class generated by split: enable split */
      DdiClauseAddLiteral(assumeClause, actV);
    }
  }
  Ddi_SatSolverAddClause(solver,assumeClause);
  Ddi_ClauseFree(assumeClause);

  do {
    int id, sat;
    refinementDone = 0;

    assumeSat = eqClassesSplitClauses(eqCl,solver);

    /* call SAT solver assuming at least one refinement */
    if (sat = Ddi_SatSolverOkay(solver)) {
      sat = Ddi_SatSolve(solver,assumeSat,-1);
    }

    Ddi_ClauseFree(assumeSat);

    if (sat) {
      int vSolver, vCnf, actCnf;
      int splitNum = Pdtutil_EqClassesSplitNum(eqCl);
      Ddi_Clause_t *cex = Ddi_SatSolverGetCexCube(solver,clauseVars);

      Pdtutil_Assert(Ddi_ClauseNumLits(cex)==nVars,"wrong cex size")

      for (i=0; i<nVars; i++) {
	int lit = cex->lits[i];
	Pdtutil_EqClassSetVal(eqCl,i+1,(lit>0)?1:0);
      }

      for (i=1; i<=nVars; i++) {
	Pdtutil_EqClassUpdate(eqCl,i);
      }

      splitNum = Pdtutil_EqClassesSplitNum(eqCl) - splitNum;

      Pdtutil_EqClassesSplitLdrReset(eqCl);

      Pdtutil_Assert(splitNum>0, "no split with SAT");
      refinementDone = refined = 1;
    }
  } while (refinementDone);

  for (i=1; i<=nVars; i++) {
    if (!Pdtutil_EqClassIsLdr(eqCl,i)) {
      int ldr_i = Pdtutil_EqClassLdr(eqCl,i);
      if (ldr_i==0) nEqc++;
      else nEq++;
      eqFound = 1;
    }
  }

#if 0
  Pdtutil_VerbosityMgr(pdrMgr, Pdtutil_VerbLevelNone_c,
    if (nEq>0 || nEqc>0) {
      printf("FOUND %d(%d const) EQUIVALENCES\n", nEq+nEqc, nEqc);
    }
  );
#endif

  return(refined);

}

/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_ClauseArray_t *
Ddi_EqClassesAsClauses(
  Ddi_PdrMgr_t *pdrMgr,
  Pdtutil_EqClasses_t *eqCl,
  Ddi_Clause_t *clauseVars
)
{
  int i, nVars = Ddi_ClauseNumLits(clauseVars);
  Ddi_ClauseArray_t *ca;
  int eqFound = 0;

  if (nVars==0) return NULL;

  ca = Ddi_ClauseArrayAlloc(1);

  for (i=1; i<=nVars; i++) {
    if (!Pdtutil_EqClassIsLdr(eqCl,i)) {
      int lit = clauseVars->lits[i-1];
      int ldr_i = Pdtutil_EqClassLdr(eqCl,i);
      int isCompl = Pdtutil_EqClassIsCompl(eqCl,i);
      if (ldr_i==0) {
	Ddi_Clause_t *cl = Ddi_ClauseAlloc(0,1);
	if (isCompl) lit = -lit;
	DdiClauseAddLiteral(cl, lit);
	Ddi_ClauseArrayPush(ca,cl);
      }
      else {
	int ldrLit = clauseVars->lits[ldr_i-1];
	if (isCompl) lit = -lit;
	DdiClauseArrayAddClause2(ca,lit,-ldrLit);
	DdiClauseArrayAddClause2(ca,-lit,ldrLit);
      }
      eqFound++;
    }
  }

  if (eqFound) {
    //      Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelNone_c,
    //  fprintf(stdout, "encoded %d EQUIVALENCES\n", eqFound));
  } else {
    Ddi_ClauseArrayFree(ca);
    ca = NULL;
  }

  return(ca);

}



/**Function*******************************************************************
  Synopsis    [generalize clause usinn SAT calls]
  Description [generalize clause usinn SAT calls]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_GeneralizeClause(
  Ddi_SatSolver_t *solver,
  Ddi_Clause_t *cl,
  unsigned char *enAbstr,
  int nAbstr,
  int *initLits,
  int useInduction
)
{
  Ddi_Clause_t *clMin, *cube, *cubeMin, *actClause;
  int actLit;
  int i, noGen=1, nLits=cl->nLits;
  int noInitCnt=0;
   static int nCalls=0;

   nCalls++;
  cube = Ddi_ClauseNegLits(cl);

  if (useInduction) {
    Pdtutil_Assert(initLits!=NULL,"null init array");
    for (i=0; i<Ddi_ClauseNumLits(cube); i++) {
      int lit = cube->lits[i];
      int pos = (lit>0);
      if (initLits[abs(lit)/2] != 0 &&
          (initLits[abs(lit)/2] > 0 && !pos ||
	   initLits[abs(lit)/2] < 0 && pos)) {
	noInitCnt++;
      }
    }
  }

  for (i=0; i<Ddi_ClauseNumLits(cube); i++) {
    Ddi_Clause_t *cubeAux;
    int sat;
    int lit = cube->lits[i];
    int pos = (lit>0);
    int initDiff=0;

    if (useInduction) {
      if (initLits[abs(lit)/2] != 0 &&
          (initLits[abs(lit)/2] > 0 && !pos ||
	   initLits[abs(lit)/2] < 0 && pos)) {
	initDiff = 1;
	Pdtutil_Assert(noInitCnt>0,"init state != is wrong");
	if (noInitCnt==1) {
	  /* skip this literal */
	  continue;
	}
      }
    }

    if (enAbstr!=NULL) {
      int id = abs(lit)-1;
      if (id>=nAbstr) continue;
      Pdtutil_Assert(id>=0 && id<nAbstr,"invalid enabstr id");
      if (!enAbstr[id]) continue;
    }

    cube->lits[i]=0;

    cubeAux = Ddi_ClauseCompact(cube);

    if (useInduction) {
      actLit = DdiSatSolverGetFreeVar(solver);
      Ddi_SatSolverAddClauseCustomized(solver,cubeAux,1,-1/*PS*/,actLit);
      Pdtutil_Assert(Ddi_SatSolverOkay(solver),"UNSAT after clause add");
      DdiClauseAddLiteral(cubeAux, actLit);
    }

    if (!Ddi_SatSolve(solver,cubeAux,-1)) {
      nLits--;
      noGen=0;
      if (useInduction) {
	Ddi_Clause_t *fi = Ddi_SatSolverFinal(solver,-1);
	int k, isInductive=0;
	for (k=0; k<fi->nLits; k++) {
	  if (fi->lits[k]==-actLit) {
	    /* lit is complemented in final clause */
	    isInductive=1;
	    break;
	  }
	}
	if (initDiff) {
	  /* initial state */
	  noInitCnt--;
	}
	actClause = Ddi_ClauseAlloc(0,1);
	if (!isInductive) actLit = -actLit;
	DdiClauseAddLiteral(actClause, actLit);
	Ddi_SatSolverAddClause(solver,actClause);
	Ddi_ClauseFree(actClause);
      }
    }
    else {
      cube->lits[i]=lit;
      if (useInduction) {
	/* disable clause */
	actClause = Ddi_ClauseAlloc(0,1);
	DdiClauseAddLiteral(actClause, -actLit);
	Ddi_SatSolverAddClause(solver,actClause);
	Ddi_ClauseFree(actClause);
      }
    }
    Ddi_ClauseFree(cubeAux);
  }
  Pdtutil_Assert(nLits>0,"no left lit in clause generalization");

  if (noGen) {
    Ddi_ClauseFree(cube);
    return NULL;
  }

  cubeMin = Ddi_ClauseCompact(cube);
  Ddi_ClauseSort(cubeMin);

  Pdtutil_Assert(!Ddi_SatSolve(solver,cubeMin,-1),"wrong generalization");

  clMin = Ddi_ClauseNegLits(cubeMin);

  Ddi_ClauseFree(cube);
  Ddi_ClauseFree(cubeMin);

  return clMin;
}


/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_UnderEstimateUnsatCoreByCexGen(
  Ddi_Bdd_t *f,
  Ddi_Bdd_t *cone,
  Ddi_Vararray_t *vars,
  unsigned char *enAbstr,
  int maxIter,
  int *result
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  int j, jj, Sat, Fail=0, nVars = Ddi_VararrayNum(vars);

  Ddi_SatSolver_t *solver = Ddi_SatSolverAlloc();
  Ddi_SatSolver_t *solver2 = Ddi_SatSolverAlloc();
  Ddi_ClauseArray_t *fClauses;
  Ddi_ClauseArray_t *coneClauses;
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  Ddi_Clause_t *varClause = Ddi_ClauseAlloc(0,nVars);
  Ddi_Varset_t *fVars = Ddi_BddSupp(f);
  Ddi_Varset_t *commonVars = Ddi_BddSupp(cone);
  Ddi_Vararray_t *cvA;
  int nKept=0;

  Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity (ddm);

  Ddi_VarsetIntersectAcc(commonVars,fVars);
  Ddi_Free(fVars);
  cvA = Ddi_VararrayMakeFromVarset(commonVars,1);
  Ddi_Free(commonVars);

  DdiAig2CnfIdInit(ddm);
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
    fprintf(dMgrO(ddm), "|coreGEN:%d,%d|.\n",
	    Ddi_BddSize(f), Ddi_BddSize(cone))
  );

  for (j=0; j<nVars; j++) {
    Ddi_Var_t *v_j = Ddi_VararrayRead(vars,j);
    bAigEdge_t vBaig = Ddi_VarToBaig(v_j);
    int vCnf, vDummy;

    vCnf = DdiAig2CnfId(bmgr,vBaig);

    if (enAbstr[j]) {
      DdiClauseAddLiteral(varClause,vCnf);
    }

    Pdtutil_Assert(vCnf==j+1,"Wrong cnf id");
  }

  if (Ddi_VararrayNum(cvA)>0) {
    for (j=0; j<Ddi_VararrayNum(cvA); j++) {
      Ddi_Var_t *v_j = Ddi_VararrayRead(cvA,j);
      bAigEdge_t vBaig = Ddi_VarToBaig(v_j);
      int vCnf;

      vCnf = DdiAig2CnfId(bmgr,vBaig);

      if (vCnf > nVars) {
        DdiClauseAddLiteral(varClause,vCnf);
      }
    }
  }
  Ddi_Free(cvA);

  fClauses = Ddi_AigClauses(f,0,NULL);
  coneClauses = Ddi_AigClauses(cone,0,NULL);

  DdiAig2CnfIdClose(ddm);

  Ddi_SatSolverAddClauses(solver,coneClauses);
  Ddi_SatSolverAddClauses(solver2,fClauses);

  Ddi_ClauseArrayFree(fClauses);
  Ddi_ClauseArrayFree(coneClauses);

  Sat=1;

  for (jj=0; (maxIter<0 || jj<maxIter) && Sat; jj++) {
    /* enumerator */
    Sat = Ddi_SatSolve(solver,NULL,-1);
    if (Sat) {
      Ddi_Clause_t *cex = Ddi_SatSolverGetCexCube(solver,varClause);
      if (!Ddi_SatSolve(solver2,cex,-1)) {
	Ddi_Clause_t *cl = Ddi_SatSolverFinal(solver2,nVars);
        if (Ddi_ClauseNumLits(cl)==0) {
          /* no refinement possible, give up */
          Fail = 2;
          Sat = 0;
        }
        else {
          Ddi_Clause_t *clMin =
	    Ddi_GeneralizeClause(solver2,cl,enAbstr,nVars,NULL,0);
          if (clMin != NULL) {
            Ddi_ClauseFree(cl);
            cl = clMin;
          }

          /* generalization of complement of assume cube returned */

          Ddi_SatSolverAddClause(solver,cl);

          for (j=0; j<Ddi_ClauseNumLits(cl); j++) {
            int lit = cl->lits[j];
            int id = abs(lit)-1;
            Pdtutil_Assert(id>=0 && id<nVars,"invalid enabstr id");
            if (enAbstr[id]!=0) {
              enAbstr[id] = 0;
              nKept++;
            }
          }
        }
	Ddi_ClauseFree(cl);
      }
      else {
	Fail = 1;
	Sat = 0;
      }
      Ddi_ClauseFree(cex);
    }
  }

  Ddi_ClauseFree(varClause);

  Ddi_SatSolverQuit(solver);
  Ddi_SatSolverQuit(solver2);

  if (Fail==1) {
    if (result!=NULL) {
      *result = -1;
    }
    nKept = -1;
  }
  else if (Fail==2) {
    if (result!=NULL) {
      *result = -1;
    }
    nKept = 0;
  }
  else if (maxIter>=0 && jj>=maxIter) {
    /* partial */
    if (result!=NULL) {
      *result = 0;
    }
  }
  else {
    if (result!=NULL) {
      *result = 1;
    }
  }

  return nKept;

}


/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_AigInterpolantByGenClauses(
  Ddi_Bdd_t *a,
  Ddi_Bdd_t *b,
  Ddi_Bdd_t *care,
  Ddi_Bdd_t *careA,
  Ddi_Vararray_t *ps,
  Ddi_Vararray_t *ns,
  Ddi_Bdd_t *init,
  Ddi_Vararray_t *globVars,
  Ddi_Vararray_t *dynAbstrCut,
  Ddi_Vararray_t *dynAbstrAux,
  Ddi_Bddarray_t *dynAbstrCutLits,
  int maxIter,
  int useInduction,
  int *result
)
{
  return aigInterpolantByGenClausesIntern(a,b,care,careA,ps,ns,init,globVars,
    dynAbstrCut,dynAbstrAux,dynAbstrCutLits,maxIter,0,useInduction,result);
}

/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
aigInterpolantByGenClausesIntern(
  Ddi_Bdd_t *a,
  Ddi_Bdd_t *b,
  Ddi_Bdd_t *care,
  Ddi_Bdd_t *careA,
  Ddi_Vararray_t *ps,
  Ddi_Vararray_t *ns,
  Ddi_Bdd_t *init,
  Ddi_Vararray_t *globVars,
  Ddi_Vararray_t *dynAbstrCut,
  Ddi_Vararray_t *dynAbstrAux,
  Ddi_Bddarray_t *dynAbstrCutLits,
  int maxIter,
  int randomCubes,
  int useInduction,
  int *result
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(a);
  int j, jj, Sat, Fail=0, nVars=0, nPsVars = Ddi_VararrayNum(ps);
  int totLits=0;

  Ddi_SatSolver_t *solver = Ddi_SatSolverAlloc();
  Ddi_SatSolver_t *solver2 = Ddi_SatSolverAlloc();
  Ddi_SatSolver_t *solver3 = NULL;
  Ddi_ClauseArray_t *aClauses=NULL;
  Ddi_ClauseArray_t *bClauses;
  Ddi_ClauseArray_t *justifyClauses;
  Ddi_ClauseArray_t *careClauses=NULL;
  Ddi_ClauseArray_t *careAClauses=NULL;
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  Ddi_Clause_t *varClause = Ddi_ClauseAlloc(0,0);
  Ddi_Clause_t *varTotClause = Ddi_ClauseAlloc(0,0);
  Ddi_Clause_t *piClause = Ddi_ClauseAlloc(0,0);
  Ddi_Clause_t *impliedLit, *cexCuts=NULL;
  Ddi_TernarySimMgr_t *xMgr=NULL;
  Ddi_Bdd_t *itp=NULL, *itpCirc=NULL, *aCut=NULL;
  Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity (ddm);
  int coreTwice=1;
  Ddi_Varset_t *suppVars = Ddi_BddSupp(a);
  Ddi_Varset_t *commonVars=NULL;
  Ddi_Varset_t *bVars = Ddi_BddSupp(b);
  Ddi_Vararray_t *totVars;
  Ddi_Vararray_t *commonVarsA;
  Ddi_Vararray_t *piAA=NULL;
  Ddi_Vararray_t *piA=NULL;
  Ddi_Vararray_t *psA=NULL;
  Ddi_Bdd_t *initLits = NULL;
  int doTernary = 0 && !randomCubes;
  int doCircuitCof = 0 && (dynAbstrCut!=NULL), doCircuitStartIter=2;
  int check=0, isSat=0, isSat1=0, nRandOK=0, nRedundant=0, nAbort=0;
  //  if (maxIter>100) maxIter = 100;
  Pdtutil_Array_t *dynAbstrRemap = NULL;
  int *initLitsInt = NULL;
  long startTime, currTime;
  static int nLong=0;
  int disableGen = 0;

  if (maxIter<0) {
    maxIter = -maxIter;
    disableGen = 1;
  }

  if (check) {
    isSat = Ddi_AigSatAnd(a,b,care);
  }

  startTime = util_cpu_time();

  if (useInduction) {
    Ddi_Bdd_t *initAig = Ddi_BddMakeAig(init);
    Pdtutil_Assert(init!=NULL,"init state needed for inductive checks");
    initLits = Ddi_AigPartitionTop(initAig,0);
    initLitsInt = Pdtutil_Alloc(int,nPsVars);
    Ddi_Free(initAig);
  }

  DdiAig2CnfIdInit(ddm);
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
    fprintf(dMgrO(ddm), "|itpGEN:%d,%d(%d)|.\n",
	    Ddi_BddSize(a), Ddi_BddSize(b), careA?Ddi_BddSize(careA):0)
  );

  if (Ddi_VarsetIsArray(suppVars)) {
    Ddi_VarsetSetArray(bVars);
  }
  if (careA != NULL && !Ddi_BddIsOne(careA)) {
    Ddi_Varset_t *suppCare = Ddi_BddSupp(careA);
    if (Ddi_VarsetIsArray(bVars)) {
      Ddi_VarsetSetArray(suppCare);
    }
    Ddi_VarsetUnionAcc(bVars,suppCare);
    Ddi_Free(suppCare);
  }
  commonVars = Ddi_VarsetDup(suppVars);
  if (Ddi_VarsetIsArray(bVars)) {
    Ddi_VarsetSetArray(commonVars);
    Ddi_VarsetSetArray(suppVars);
  }

  Ddi_VarsetIntersectAcc(commonVars,bVars);
  commonVarsA = Ddi_VararrayMakeFromVarset(commonVars,1);
  Ddi_Free(commonVars);
  
  Ddi_VarsetUnionAcc(suppVars,bVars);
  totVars = Ddi_VararrayMakeFromVarset(suppVars,1);
  Ddi_VarsetDiffAcc(suppVars,bVars);
  piAA = Ddi_VararrayMakeFromVarset(suppVars,1);
  Ddi_Free(bVars);
  Ddi_Free(suppVars);

  piA = Ddi_VararrayAlloc (ddm,0);
  psA = Ddi_VararrayAlloc (ddm,0);

  if (dynAbstrCut!=NULL) {
    solver3 = Ddi_SatSolverAlloc();
  }

  for (j=0; j<Ddi_VararrayNum(globVars); j++) {
    Ddi_Var_t *v = Ddi_VararrayRead(globVars,j);
    Pdtutil_Assert(Ddi_VarReadMark(v) == 0,"0 var mark required");
    Ddi_VarWriteMark(v,1);
  }

  for (j=nVars=0; j<nPsVars; j++) {
    Ddi_Var_t *ps_j = Ddi_VararrayRead(ps,j);
    Ddi_Var_t *ns_j = Ddi_VararrayRead(ns,j);
    if (Ddi_VarReadMark(ps_j)!=0 || Ddi_VarReadMark(ns_j)!=0) {
      bAigEdge_t psBaig = Ddi_VarToBaig(ps_j);
      bAigEdge_t nsBaig = Ddi_VarToBaig(ns_j);
      int psCnf, nsCnf;

      /* set mark on ps var */
      Ddi_VarWriteMark(ps_j,1);

      nsCnf = DdiAig2CnfId(bmgr,nsBaig);
      psCnf = DdiAig2CnfId(bmgr,psBaig);

      Pdtutil_Assert(nsCnf==2*(nVars)+1,"Wrong cnf id");
      Pdtutil_Assert(psCnf==2*(nVars+1),"Wrong cnf id");
      Ddi_VararrayInsertLast(psA,ns_j);
      DdiClauseAddLiteral(varTotClause,nsCnf);
      nVars++;
    }
  }

  if (useInduction) {
    for (j=0; j<nPsVars; j++) {
      initLitsInt[j] = 0;
    }
    for (j=0; j<Ddi_BddPartNum(initLits); j++) {
      Ddi_Bdd_t *l_j = Ddi_BddPartRead(initLits,j);
      Ddi_Var_t *ps_j = Ddi_BddTopVar(l_j);
      if (Ddi_VarReadMark(ps_j)!=0) {
	bAigEdge_t psBaig = Ddi_BddToBaig(l_j);
	int psCnf = DdiAig2CnfId(bmgr,bAig_NonInvertedEdge(psBaig));
	int jjj = psCnf/2-1;
	//	printf("N: %s\n", Ddi_VarName(Ddi_BddTopVar(l_j)));
	Pdtutil_Assert(jjj>=0&&jjj<nVars,"wrong ps lit");
	Pdtutil_Assert(initLitsInt[jjj]==0,
		       "double assignment in init lit array");
	initLitsInt[jjj] = Ddi_BddIsComplement(l_j) ? -1 : 1;
      }
    }
  }

  for (j=0; j<Ddi_VararrayNum(piAA); j++) {
    Ddi_Var_t *v = Ddi_VararrayRead(piAA,j);
    bAigEdge_t vBaig = Ddi_VarToBaig(v);
    int vCnf = DdiAig2CnfId(bmgr,vBaig);
    DdiClauseAddLiteral(piClause,vCnf);
  }
  for (j=0; j<Ddi_VararrayNum(totVars); j++) {
    Ddi_Var_t *v = Ddi_VararrayRead(totVars,j);
    if (Ddi_VarReadMark(v) == 0) {
      bAigEdge_t vBaig = Ddi_VarToBaig(v);
      int vCnf = DdiAig2CnfId(bmgr,vBaig);
      //      Pdtutil_Assert(vCnf>2*nVars,"Wrong cnf id");
      if (vCnf>2*nVars) {
	Ddi_VararrayInsertLast(piA,v);
      }
    }
  }

  for (j=0; j<Ddi_VararrayNum(globVars); j++) {
    Ddi_Var_t *v_j = Ddi_VararrayRead(globVars,j);
    bAigEdge_t vBaig = Ddi_VarToBaig(v_j);
    int vCnf;

    Ddi_VarWriteMark(v_j,0);

    vCnf = DdiAig2CnfId(bmgr,vBaig);
    DdiClauseAddLiteral(varClause,vCnf);

    Pdtutil_Assert(vCnf<=2*nVars+1,"Wrong cnf id");
  }

  for (j=0; j<nPsVars; j++) {
    Ddi_Var_t *ps_j = Ddi_VararrayRead(ps,j);
    Ddi_VarWriteMark(ps_j,0);
  }

  Ddi_Free(totVars);


  justifyClauses = Ddi_ClauseArrayAlloc(0);
  if (!randomCubes) {
    aClauses = Ddi_AigClausesWithJustify(a,0,NULL,justifyClauses);
  }
  bClauses = Ddi_AigClauses(b,0,NULL);

  if (doTernary) {
    Ddi_Bddarray_t *fA = Ddi_BddarrayAlloc(ddm,0);
    Ddi_Bdd_t *dummyDelta = Ddi_BddMakeConstAig(ddm,1);
    //Ddi_BddMakeLiteralAig(Ddi_VararrayRead(ps,nVars-2),1);
    Ddi_Bdd_t *mySpec = Ddi_BddNot(a);
    Ddi_BddarrayWrite(fA,0,dummyDelta);
    xMgr = Ddi_TernarySimInit(fA,mySpec,piAA,psA);
    Ddi_Free(mySpec);
    Ddi_Free(dummyDelta);
    Ddi_Free(fA);
  }

  if (Ddi_BddIsPartConj(a)) {
    int i;
    for (i=0; i<Ddi_BddPartNum(a); i++) {
      Ddi_Bdd_t *a_i = Ddi_BddPartRead(a,i); 
      bAigEdge_t baig = Ddi_BddToBaig(a_i);
      int vCnf = bAig_NodeIsInverted(baig) ? -DdiAig2CnfId(bmgr,baig) :
                                            DdiAig2CnfId(bmgr,baig);
      impliedLit = Ddi_ClauseAlloc(0,nVars);
      DdiClauseAddLiteral(impliedLit,vCnf);
      Ddi_ClauseFree(impliedLit);
    }
  }
  else {
    bAigEdge_t baig = Ddi_BddToBaig(a);
    int vCnf = bAig_NodeIsInverted(baig) ? -DdiAig2CnfId(bmgr,baig) :
                                            DdiAig2CnfId(bmgr,baig);
    impliedLit = Ddi_ClauseAlloc(0,nVars);
    DdiClauseAddLiteral(impliedLit,vCnf);
    Ddi_ClauseFree(impliedLit);
  }

  if (care!=NULL) {
    careClauses = Ddi_AigClauses(care,0,NULL);
  }
  if (careA!=NULL) {
    careAClauses = Ddi_AigClauses(careA,0,NULL);
  }

  if (!randomCubes) {
    Ddi_SatSolverAddClauses(solver,aClauses);
  }
  Ddi_SatSolverAddClauses(solver2,bClauses);
  if (careClauses!=NULL) {
    Ddi_SatSolverAddClauses(solver,careClauses);
    Ddi_SatSolverAddClauses(solver2,careClauses);
  }
  if (careAClauses!=NULL) {
    Ddi_SatSolverAddClauses(solver,careAClauses);
  }

  if (solver3!=NULL) {
    Ddi_ClauseArray_t *aCutClauses;
    Ddi_ClauseArray_t *newClauses = Ddi_ClauseArrayAlloc(0);
    Ddi_Bdd_t *aCut = Ddi_BddCompose(a,ns,dynAbstrCutLits);

    dynAbstrRemap = Pdtutil_IntegerArrayAlloc (0);

    cexCuts = Ddi_ClauseAlloc(0,1);

    Pdtutil_Assert(dynAbstrAux!=NULL && dynAbstrCutLits!=NULL,
		   "missing cut vars/lits");
    aCutClauses = Ddi_AigClauses(aCut,0,NULL);
    Ddi_SatSolverAddClauses(solver3,bClauses);
    Ddi_SatSolverAddClauses(solver3,aCutClauses);
    //    Ddi_SatSolverAddClauses(solver3,aClauses);
    if (careClauses!=NULL) {
      Ddi_SatSolverAddClauses(solver3,careClauses);
    }
    Ddi_ClauseArrayFree(aCutClauses);
    Ddi_Free(aCut);

    for (j=0; j<Ddi_VararrayNum(commonVarsA); j++) {
      Ddi_Var_t *v_j = Ddi_VararrayRead(commonVarsA,j);
      Ddi_VarWriteMark(v_j,1);
    }

    for (j=0; j<Ddi_VararrayNum(dynAbstrCut); j++) {
      Ddi_Var_t *ns_j = Ddi_VararrayRead(ns,j);
      if (Ddi_VarReadMark(ns_j) == 1) {
	Ddi_Var_t *vC = Ddi_VararrayRead(dynAbstrCut,j);
	Ddi_Var_t *vA = Ddi_VararrayRead(dynAbstrAux,j);
	bAigEdge_t vBaigA = Ddi_VarToBaig(vA);
	bAigEdge_t vBaigC = Ddi_VarToBaig(vC);
	bAigEdge_t vBaigNs = Ddi_VarToBaig(ns_j);
	int vCnfC = DdiAig2CnfId(bmgr,vBaigC);
	int vCnfA = DdiAig2CnfId(bmgr,vBaigA);
	int vCnfNs = DdiAig2CnfId(bmgr,vBaigNs);
	DdiClauseAddLiteral(cexCuts,vCnfA);
	/* add implication clause */
	/* vCnfA => (vCnfC == vCnfNs) */
	/* (!vCnfA + vCnfC + !vCnfNs)(!vCnfA + !vCnfC + vCnfNs) */
	DdiClauseArrayAddClause3(newClauses,-vCnfA,vCnfC,-vCnfNs);
	DdiClauseArrayAddClause3(newClauses,-vCnfA,-vCnfC,vCnfNs);
	Pdtutil_IntegerArrayInsertLast (dynAbstrRemap, j);

	//	printf("CUT %s - AUX %s - NS %s\n",
	//       Ddi_VarName(vC), Ddi_VarName(vA), Ddi_VarName(ns_j));

      }
    }

    for (j=0; j<Ddi_VararrayNum(commonVarsA); j++) {
      Ddi_Var_t *v_j = Ddi_VararrayRead(commonVarsA,j);
      Ddi_VarWriteMark(v_j,0);
    }

    Ddi_SatSolverAddClauses(solver3,newClauses);
    Ddi_ClauseArrayFree(newClauses);
  }

  Ddi_ClauseArrayFree(careAClauses);
  Ddi_ClauseArrayFree(careClauses);
  Ddi_ClauseArrayFree(aClauses);
  Ddi_ClauseArrayFree(bClauses);

  Sat=1;
  itp = Ddi_BddMakeConstAig(ddm,0);
  itpCirc = Ddi_BddMakeConstAig(ddm,0);

  if (1 && randomCubes) {
    srand(12345);
  }

  if (!Ddi_SatSolve(solver2,NULL,-1)) {
    Ddi_BddNotAcc(itp); // constant 1
    jj=0;
  }
  else if (!Ddi_SatSolve(solver,NULL,-1)) {
    jj=0;
  }
  else 
  for (jj=0; (maxIter<0 || jj<maxIter) && Sat; jj++) {
    /* enumerator */

    int useRand = randomCubes; //? jj%2 : 0;

    Ddi_Clause_t *cex = NULL;

    //    printf("JJ: %d\n",jj);
    if (!useRand) {
      Sat = Ddi_SatSolve(solver,NULL,50);
      if (Sat<0) {
	Sat = 0;
	jj = maxIter;
      }
    }
    else {
      int maxRetry = 5, nFlip=0;
      int ok;
      if (jj>10 && nRandOK==0) {
	jj = maxIter;
	nAbort++;
	break;
      }
      Sat = 1;
      cex = Ddi_ClauseRandLits(varTotClause);
      ok = !Ddi_SatSolve(solver2,cex,-1);
      if (!ok) {
	Ddi_ClauseFree(cex);
	cex = NULL;
	nAbort++;
      }
      while (0 && cex!=NULL && !Ddi_SatSolve(solver,cex,-1)) {
	int j, found=0;
	Ddi_Clause_t *cl = Ddi_SatSolverFinal(solver,2*nVars);
	Pdtutil_Assert(cl!=NULL,"final required");

	for (j=0; j<cl->nLits; j++) {
	  int vj = abs(cl->lits[j]);
	  Pdtutil_Assert(vj<ddm->cnf.cnf2aigSize,"invalid cnf 2 aig");
	  bAigEdge_t baig = ddm->cnf.cnf2aig[vj];
	  Ddi_Var_t *v = Ddi_VarFromBaig(ddm,baig);
	  //printf("FIN[%d] = %s\n", j, Ddi_VarName(v));
	  Ddi_VarWriteMark(v,1);
	}

	for (j=cex->nLits-1; j>=0; j--) {
	  int vj = abs(cex->lits[j]);
	  Pdtutil_Assert(vj<ddm->cnf.cnf2aigSize,"invalid cnf 2 aig");
	  bAigEdge_t baig = ddm->cnf.cnf2aig[vj];
	  Ddi_Var_t *v = Ddi_VarFromBaig(ddm,baig);
	  //printf("FIN[%d] = %s\n", j, Ddi_VarName(v));
	  if (Ddi_VarReadMark(v) == 1) {
	    //	    DdiClauseRemoveLiteral(cex, j);
	    cex->lits[j] = -cex->lits[j]; nFlip++;
	    if (Ddi_SatSolve(solver2,cex,-1)) {
	      printf("invalid literal flip\n");
	      cex->lits[j] = -cex->lits[j]; nFlip++;
	    }
	    else if (Ddi_SatSolve(solver,cex,-1)) {
	      found=1;
	      break;
	    }
	  }
	}

	for (j=0; j<cl->nLits; j++) {
	  int vj = abs(cl->lits[j]);
	  Pdtutil_Assert(vj<ddm->cnf.cnf2aigSize,"invalid cnf 2 aig");
	  bAigEdge_t baig = ddm->cnf.cnf2aig[vj];
	  Ddi_Var_t *v = Ddi_VarFromBaig(ddm,baig);
	  //printf("FIN[%d] = %s\n", j, Ddi_VarName(v));
	  Ddi_VarWriteMark(v,0);
	}

	Ddi_ClauseFree(cl);

	if (maxRetry--<=0 && !found) {
	  Ddi_ClauseFree(cex);
	  cex = NULL;
	  nAbort++;
	}
	nRedundant++;
      }

    }

    if (Sat && !useRand) {
      cex = Ddi_SatSolverGetCexCube(solver,varClause);
      //cex = Ddi_SatSolverGetCexCubeWithJustify(solver,
      //		    varClause,justifyClauses,impliedLit);
      Ddi_Clause_t *cexPi = Ddi_SatSolverGetCexCube(solver,piClause);
      Ddi_Bdd_t *circCof = NULL;
      if (doCircuitCof && jj<doCircuitStartIter) {
	Ddi_Clause_t *cexWithCuts = Ddi_ClauseJoin(cexPi,cexCuts);

	if (Ddi_SatSolve(solver3,cexWithCuts,-1)) {
	  Fail = 1;
	  Sat = 0;
	  //	  Pdtutil_Assert(0,"SAT on cex\n");
	}
	else {
	  Ddi_Clause_t *cexStateClauseVars = Ddi_ClauseAlloc(0,1);
	  Ddi_Vararray_t *cexStateVars = Ddi_VararrayAlloc (ddm,0);

	  Ddi_Clause_t *cl = Ddi_SatSolverFinal(solver3,-1);

	  for (j=0; j<cl->nLits; j++) {
	    int vj = abs(cl->lits[j]);
	    Pdtutil_Assert(vj<ddm->cnf.cnf2aigSize,"invalid cnf 2 aig");
	    bAigEdge_t baig = ddm->cnf.cnf2aig[vj];
	    Ddi_Var_t *v = Ddi_VarFromBaig(ddm,baig);
	    //printf("FIN[%d] = %s\n", j, Ddi_VarName(v));
	    Ddi_VarWriteMark(v,1);
	  }
	  for (j=0; j<Pdtutil_IntegerArrayNum(dynAbstrRemap); j++) {
	    int k = Pdtutil_IntegerArrayRead(dynAbstrRemap,j);
	    Pdtutil_Assert(k>=0 && k<Ddi_VararrayNum(ns),"wrong remap index");
	    Ddi_Var_t *ns_j = Ddi_VararrayRead(ns,k);
	    Ddi_Var_t *vA = Ddi_VararrayRead(dynAbstrAux,k);
	    if (Ddi_VarReadMark(vA) == 0) {
	      /* not in final - take cex val */
	      bAigEdge_t vBaig = Ddi_VarToBaig(ns_j);
	      int vCnf = DdiAig2CnfId(bmgr,vBaig);
	      DdiClauseAddLiteral(cexStateClauseVars,vCnf);
	      Ddi_VararrayInsertLast(cexStateVars,ns_j);
	    }
	  }
	  for (j=0; j<cl->nLits; j++) {
	    int vj = abs(cl->lits[j]);
	    Pdtutil_Assert(vj<ddm->cnf.cnf2aigSize,"invalid cnf 2 aig");
	    bAigEdge_t baig = ddm->cnf.cnf2aig[vj];
	    Ddi_Var_t *v = Ddi_VarFromBaig(ddm,baig);
	    Ddi_VarWriteMark(v,0);
	  }

	  Ddi_Bdd_t *cexStateAig = Ddi_SatSolverGetCexCubeAig(solver,
			  cexStateClauseVars,cexStateVars);

	  Ddi_Bdd_t *cexPiAig = Ddi_SatSolverGetCexCubeAig(solver,
							    piClause,piAA);
	  Ddi_Bdd_t *myCof = Ddi_BddDup(a);

	  Ddi_Free(cexStateVars);
	  Ddi_ClauseFree(cexStateClauseVars);
	  Ddi_ClauseFree(cl);

	  Ddi_AigConstrainCubeAcc(myCof,cexPiAig);
	  Ddi_AigConstrainCubeAcc(myCof,cexStateAig);
	  Ddi_BddOrAcc(itpCirc,myCof);
	  Ddi_BddNotAcc(myCof);
	  aClauses = Ddi_AigClauses(myCof,0,NULL);
	  Ddi_SatSolverAddClauses(solver,aClauses);
	  Ddi_ClauseArrayFree(aClauses);
	  Ddi_Free(cexPiAig);
	  Ddi_Free(cexStateAig);
	  Ddi_Free(myCof);
	}
	Ddi_ClauseFree(cexWithCuts);
      }

      if (!Fail && doTernary) {
	Ddi_Clause_t *reducedCex = Ddi_ClauseAlloc(0,1);
	int i, rem=0, enTernary = Ddi_TernaryPdrSimulate(xMgr,2,cexPi,cex,NULL,1,0);

	Ddi_Clause_t *cexDup = Ddi_ClauseDup(cex);
	int isSatIntern=0;
	if (check) {
	  isSatIntern = Ddi_SatSolve(solver2,cex,-1);
	}

	Pdtutil_Assert(enTernary,"wrong cex justify");

	for (i=0; i<cex->nLits; i++) {
	  if (!Ddi_TernaryPdrSimulate(xMgr, 0, NULL, NULL, NULL, 0,
				      cex->lits[i])) {
	    DdiClauseAddLiteral(reducedCex,cex->lits[i]);
	  }
	  else {
	    rem++;
	    cexDup->lits[i] = 0;
	    if (check && !isSat) {
	      if (isSatIntern != Ddi_SatSolve(solver2,cexDup,-1)) {
		cexDup->lits[i] = -cex->lits[i];
		Ddi_Clause_t *cexTot = Ddi_ClauseJoin(cexPi,cexDup);
		Ddi_Clause_t *cexTot1 = Ddi_ClauseCompact(cexTot);
		Ddi_ClauseSort(cexTot1);
		if (Ddi_SatSolve(solver,cexTot1,-1)) {
		  printf("problem\n");
		}
		Pdtutil_Assert(0,"error in cex enlargement");
	      }
	    }
	  }
	}
	if (cex->nLits > reducedCex->nLits) {
	  int nl = cex->nLits;
	  Ddi_ClauseSort(reducedCex);
	  Ddi_ClauseFree(cex);
	  cex = reducedCex;
	}
	else {
	  Ddi_ClauseFree(reducedCex);
	}
	Ddi_ClauseFree(cexDup);
      }
      Ddi_ClauseFree(cexPi);
    }

    if (!Fail && cex!=NULL && !Ddi_SatSolve(solver2,cex,-1)) {
      Ddi_Clause_t *cl = Ddi_SatSolverFinal(solver2,2*nVars);
      int enGen=!coreTwice || 1;
      if (useRand) {
	nRandOK++;
      }
      if (cl->nLits>40) {
	nLong++;
      }
      if (coreTwice && cl->nLits>2) {
	Ddi_ClauseFree(cex);
	cex = Ddi_ClauseNegLits(cl);
	if (Ddi_SatSolve(solver2,cex,-1)) {
	  Pdtutil_Assert(0,"wrong unsat core");
	}
	Ddi_Clause_t *cl2 = Ddi_SatSolverFinal(solver2,2*nVars);
	if (Ddi_ClauseNumLits(cl2) < Ddi_ClauseNumLits(cl)) {
	  Ddi_ClauseFree(cl);
	  cl = cl2;
	  enGen=2;
	}
	else {
	  Ddi_ClauseFree(cl2);
	}
      }
      if (!disableGen && enGen && (cl->nLits>3)) {
	Ddi_Clause_t *clMin = Ddi_GeneralizeClause(solver2,
				cl,NULL,0,initLitsInt,useInduction);
	//       Ddi_Clause_t *clMin = NULL;
	if (clMin != NULL) {
	  Ddi_ClauseFree(cl);
	  cl = clMin;
	}
      }
      totLits += cl->nLits;

      /* generalization of complement of assume cube returned */
      Ddi_Clause_t *cube = Ddi_ClauseNegLits(cl);

      if (!useRand || Ddi_SatSolve(solver,cube,-1)) {

	Ddi_Bdd_t *cubeAig = Ddi_BddMakeFromClauseWithVars(cl,psA);
	Ddi_BddNotAcc(cubeAig);
	//	Pdtutil_Assert(!Ddi_SatSolve(solver2,cube,-1),
	//	       "wrong cube enlargement");
	//	Pdtutil_Assert(!Ddi_AigSatAnd(cubeAig,b,care),
	//	       "wrong itp by generalization");

	Ddi_BddOrAcc(itp,cubeAig);
	Ddi_Free(cubeAig);

	Ddi_SatSolverAddClause(solver,cl);
	/////
      }
      else {
	nRedundant++;
      }

      Ddi_ClauseFree(cube);
      Ddi_ClauseFree(cl);
    }
    else if (cex!=NULL && useInduction) {
      Ddi_Clause_t *cl = Ddi_ClauseNegLits(cex);
      Ddi_Clause_t *clMin = Ddi_GeneralizeClause(solver2,
				cl,NULL,0,initLitsInt,1);
	//       Ddi_Clause_t *clMin = NULL;
      if (clMin != NULL) {
        Ddi_Clause_t *cube = Ddi_ClauseNegLits(clMin);
	Ddi_Bdd_t *cubeAig = Ddi_BddMakeFromClauseWithVars(cl,psA);
	Ddi_BddNotAcc(cubeAig);
	Ddi_BddOrAcc(itp,cubeAig);
	Ddi_Free(cubeAig);
	Ddi_SatSolverAddClause(solver,cl);
	/////
        Ddi_ClauseFree(clMin);
      }
      Ddi_ClauseFree(cl);
    }
    else if (!useRand) {
      if (cex!=NULL) Fail = 1;
      Sat = 0;
    }

    if (cex!=NULL) Ddi_ClauseFree(cex);
  }

  DdiAig2CnfIdClose(ddm);

  Ddi_Free(commonVarsA);
  Ddi_Free(piAA);
  Ddi_Free(piA);
  Ddi_Free(psA);
  Ddi_Free(initLits);
  Pdtutil_Free(initLitsInt);

  if (xMgr != NULL) {
    Ddi_TernarySimQuit(xMgr);
  }

  if (dynAbstrRemap!=NULL) Pdtutil_IntegerArrayFree (dynAbstrRemap);

  Ddi_ClauseFree(cexCuts);
  Ddi_ClauseFree(varClause);
  Ddi_ClauseFree(varTotClause);
  Ddi_ClauseFree(piClause);
  Ddi_ClauseArrayFree(justifyClauses);

  Ddi_SatSolverQuit(solver);
  Ddi_SatSolverQuit(solver2);

  if (solver3!=NULL) Ddi_SatSolverQuit(solver3);

  if (Fail) {
    if (result!=NULL) {
      *result = -1;
    }
    Ddi_Free(itp);
    Ddi_Free(itpCirc);
  }
  else if (maxIter>=0 && jj>=maxIter) {
    /* partial */
    if (result!=NULL) {
      *result = 0;
    }
  }
  else {
    if (result!=NULL) {
      *result = 1;
    }
  }

  currTime = util_cpu_time();

  if (itp!=NULL) {
    // Pdtutil_Assert(!Ddi_AigSatAnd(itp,b,care),"wrong itp by generalization");
    int size0 = Ddi_BddSize(itp);
    int size1 = Ddi_BddSize(itpCirc);

    if (size1>0) {
      Ddi_BddOrAcc(itp,itpCirc);
      Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
	fprintf(dMgrO(ddm), "|itpCIRC:%d+%d| ", size0, size1)
      );
      if (size1 > 10000) {
        ddiAbcOptAcc (itp,10);
      }
    }
    Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
       fprintf(dMgrO(ddm), "|itpGEN:%d| - nGen:%d%s (avg-#lits: %4.1f).\n",
             Ddi_BddSize(itp),jj,(jj>=maxIter)?"(partial)":"",
               jj>1?((float)totLits/(jj-1)):0.0)
    );
    if (randomCubes) {
      Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
	 fprintf(dMgrO(ddm), "rand OK %d/%d (redundant: %d / abort: %d)\n",
		 nRandOK,jj,nRedundant,nAbort));
    }
  }
  Ddi_Free(itpCirc);

  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
     fprintf(dMgrO(ddm), "itpGEN time: %s\n ",
	     util_print_time(currTime-startTime))
  );

  if (check && itp!=NULL) {
    Pdtutil_Assert(randomCubes || jj>=maxIter || !isSat,
		   "sat problem in itpgen");
    Pdtutil_Assert(!Ddi_AigSatAnd(b,itp,care),"wrong itpgen");
  }

  return itp;

}


/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Ddi_AigAbstrRefinePba(
  Ddi_Bdd_t *f,
  Ddi_Vararray_t *abstrVars,
  Ddi_Bddarray_t *noAbstr,
  Ddi_Bddarray_t *prevAbstr,
  int *result
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  int j, jj, nPrev=0, Sat, nAbstr=0, nVars = Ddi_VararrayNum(abstrVars);
  Ddi_Clause_t *assume=NULL;

  Ddi_SatSolver_t *solver = Ddi_SatSolver22Alloc();
  Ddi_ClauseArray_t *fClauses;
  Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity (ddm);
  Ddi_Bddarray_t *noAbstrOut=NULL;
  long startTime, currTime;

  if (prevAbstr != NULL) {
    Ddi_Vararray_t *vSubst = Ddi_VararrayAlloc (ddm,0);
    Ddi_Bddarray_t *fSubst = Ddi_BddarrayAlloc (ddm,0);
    for (j=0; j<Ddi_BddarrayNum(prevAbstr); j++) {
      Ddi_Bdd_t *s_j = Ddi_BddarrayRead(prevAbstr,j);
      if (Ddi_BddIsZero(s_j)) {
	Ddi_Var_t *v_j = Ddi_VararrayRead(abstrVars,j);
	Ddi_VararrayInsertLast(vSubst,v_j);
	Ddi_BddarrayInsertLast(fSubst,s_j);
	nPrev++;
      }
    }
    Ddi_BddComposeAcc(f,vSubst,fSubst);
    Ddi_Free(fSubst);
    Ddi_Free(vSubst);
  }

  startTime = util_cpu_time();

  DdiAig2CnfIdInit(ddm);
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
    fprintf(dMgrO(ddm), "|abstrRefPba(prev:%d):%d->", nPrev, Ddi_BddSize(f))
  );

  fClauses = Ddi_AigClauses(f,0,NULL);
  Ddi_SatSolverAddClauses(solver,fClauses);
  Ddi_ClauseArrayFree(fClauses);

  assume = Ddi_ClauseAlloc(0,1);

  for (j=0; j<Ddi_VararrayNum(abstrVars); j++) {
    Ddi_Var_t *a_j = Ddi_VararrayRead(abstrVars,j);
    Ddi_Bdd_t *c_j = Ddi_BddarrayRead(noAbstr,j);
    Ddi_Bdd_t *p_j = prevAbstr==NULL ? NULL : Ddi_BddarrayRead(prevAbstr,j);
    if (Ddi_BddIsZero(c_j) && (p_j==NULL || Ddi_BddIsOne(p_j))) {
      bAigEdge_t aBaig = Ddi_VarToBaig(a_j);
      int vCnf = DdiAig2CnfId(bmgr,aBaig);
      DdiClauseAddLiteral(assume,-vCnf);
    }
  }

  *result = 0;
  Sat = Ddi_SatSolve(solver,assume,-1);

  if (Sat) {
    *result = 1;
  }
  else {
    Ddi_Clause_t *cl = Ddi_SatSolverFinal(solver,-1);
    noAbstrOut = Ddi_BddarrayDup(noAbstr);

    for (j=0; j<cl->nLits; j++) {
      int vj = abs(cl->lits[j]);
      Pdtutil_Assert(vj<ddm->cnf.cnf2aigSize,"invalid cnf 2 aig");
      bAigEdge_t baig = ddm->cnf.cnf2aig[vj];
      Ddi_Var_t *v = Ddi_VarFromBaig(ddm,baig);
      //printf("FIN[%d] = %s\n", j, Ddi_VarName(v));
      Ddi_VarWriteMark(v,1);
    }
    for (j=0; j<Ddi_VararrayNum(abstrVars); j++) {
      Ddi_Var_t *a_j = Ddi_VararrayRead(abstrVars,j);
      Ddi_Bdd_t *c_j = Ddi_BddarrayRead(noAbstrOut,j);
      Ddi_Bdd_t *p_j = prevAbstr==NULL ? NULL : Ddi_BddarrayRead(prevAbstr,j);
      if (p_j!=NULL && Ddi_BddIsZero(p_j)) {
	Ddi_BddarrayWrite(noAbstrOut,j,p_j);
      }
      else if (Ddi_BddIsZero(c_j)) {
	if (Ddi_VarReadMark(a_j) == 0) {
	  /* not in final - force abstr */
	  Ddi_BddNotAcc(c_j);
	  nAbstr++;
	}
      }
    }
    for (j=0; j<cl->nLits; j++) {
      int vj = abs(cl->lits[j]);
      Pdtutil_Assert(vj<ddm->cnf.cnf2aigSize,"invalid cnf 2 aig");
      bAigEdge_t baig = ddm->cnf.cnf2aig[vj];
      Ddi_Var_t *v = Ddi_VarFromBaig(ddm,baig);
      //printf("FIN[%d] = %s\n", j, Ddi_VarName(v));
      Ddi_VarWriteMark(v,0);
    }

  }

  currTime = util_cpu_time();

  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
     fprintf(dMgrO(ddm), "(%d/%d/%d abstr/ref/tot)(time: %s)| ",
	     nAbstr, nVars-nAbstr, nVars, util_print_time(currTime-startTime))
  );

  Ddi_ClauseFree(assume);

  DdiAig2CnfIdClose(ddm);

  Ddi_SatSolverQuit(solver);

  return noAbstrOut;

}


/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Ddi_AigAbstrRefineCegarPba(
  Ddi_Bdd_t *f,
  Ddi_Vararray_t *abstrVars,
  Ddi_Bddarray_t *noAbstr,
  Ddi_Bddarray_t *prevAbstr,
  int *result
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  int j, jj, nPrev=0, nIter=0, Sat, nRef=0, nVars = 0, nVarsTot;
  Ddi_Clause_t *assume=NULL, *assumeWithAbstr=NULL;
  Ddi_Varset_t *fSupp = NULL;
  Ddi_Varset_t *controlVars = NULL;
  Ddi_Vararray_t *fVars=NULL;
  Ddi_Bdd_t *fDup=NULL;
  Ddi_SatSolver_t *solver = Ddi_SatSolverAlloc();
  Ddi_ClauseArray_t *fClauses;
  Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity (ddm);
  Ddi_Bddarray_t *noAbstrOut=NULL;
  Ddi_Clause_t *piVars = Ddi_ClauseAlloc(0,0);
  Ddi_Clause_t *assumeVars = Ddi_ClauseAlloc(0,0);
  long startTime, currTime;
  int timeLimit=ddm->settings.aig.satTimeLimit;
  int ret, incrementalAbstr=1; 
  int tryJustify = 0;

  if (Ddi_BddIsConstant(f)) {
    *result = Ddi_BddIsOne(f);
    Ddi_ClauseFree(piVars);
    Ddi_ClauseFree(assumeVars);
    Ddi_SatSolverQuit(solver);
    return NULL;
  }

  startTime = util_cpu_time();

  fDup = Ddi_BddDup(f);

  if (prevAbstr != NULL) {
    Ddi_Vararray_t *vSubst = Ddi_VararrayAlloc (ddm,0);
    Ddi_Bddarray_t *fSubst = Ddi_BddarrayAlloc (ddm,0);
    for (j=0; j<Ddi_BddarrayNum(prevAbstr); j++) {
      Ddi_Bdd_t *s_j = Ddi_BddarrayRead(prevAbstr,j);
      if (Ddi_BddIsZero(s_j)) {
	Ddi_Var_t *v_j = Ddi_VararrayRead(abstrVars,j);
	Ddi_VararrayInsertLast(vSubst,v_j);
	Ddi_BddarrayInsertLast(fSubst,s_j);
	nPrev++;
      }
    }
    Ddi_BddComposeAcc(fDup,vSubst,fSubst);
    Ddi_Free(fSubst);
    Ddi_Free(vSubst);
  }

  if (Ddi_BddIsConstant(fDup)) {
    *result = Ddi_BddIsOne(fDup);
    Ddi_ClauseFree(piVars);
    Ddi_ClauseFree(assumeVars);
    Ddi_Free(fDup);
    Ddi_SatSolverQuit(solver);
    return NULL;
  }

  fSupp = Ddi_BddSupp(fDup);
  controlVars = Ddi_VarsetMakeFromArray(abstrVars);

  DdiAig2CnfIdInit(ddm);
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
    fprintf(dMgrO(ddm), "|abstrRefCegarPba(prev:%d):%d->",
	    nPrev, Ddi_BddSize(f))
  );

  Ddi_VarsetSetArray(fSupp);
  Ddi_VarsetDiffAcc(fSupp,controlVars);
  fVars = Ddi_VararrayMakeFromVarset(fSupp,1);
  Ddi_Free(fSupp);
  Ddi_Free(controlVars);

  fClauses = Ddi_AigClauses(fDup,0,NULL);
  Ddi_SatSolverAddClauses(solver,fClauses);
  Ddi_ClauseArrayFree(fClauses);

  assume = Ddi_ClauseAlloc(0,1);
  assumeWithAbstr = Ddi_ClauseAlloc(0,1);

  noAbstrOut = Ddi_BddarrayDup(noAbstr);

  for (j=0; j<Ddi_VararrayNum(abstrVars); j++) {
    Ddi_Var_t *a_j = Ddi_VararrayRead(abstrVars,j);
    Ddi_Bdd_t *c_j = Ddi_BddarrayRead(noAbstr,j);
    Ddi_Bdd_t *p_j = prevAbstr==NULL ? NULL : Ddi_BddarrayRead(prevAbstr,j);
    if (Ddi_BddIsZero(c_j) && (p_j==NULL || Ddi_BddIsOne(p_j))) {
      bAigEdge_t aBaig = Ddi_VarToBaig(a_j);
      int vCnf = DdiAig2CnfId(bmgr,aBaig);
      Ddi_Bdd_t *cOut_j = Ddi_BddarrayRead(noAbstrOut,j);
      Ddi_BddNotAcc(cOut_j);
      DdiClauseAddLiteral(assumeVars,vCnf);
      DdiClauseAddLiteral(assume,-vCnf);
      DdiClauseAddLiteral(assumeWithAbstr,vCnf);
      nVars++;
    }
  }
  nVarsTot = Ddi_VararrayNum(abstrVars);

  for (j=0; j<Ddi_VararrayNum(fVars); j++) {
    Ddi_Var_t *v_j = Ddi_VararrayRead(fVars,j);
    bAigEdge_t aBaig = Ddi_VarToBaig(v_j);
    int vCnf = DdiAig2CnfId(bmgr,aBaig);
    DdiClauseAddLiteral(piVars,vCnf);
  }
  Ddi_Free(fVars);

  *result = 0;

  while (!(*result)) {
    Ddi_Clause_t *cex;
    Ddi_Clause_t *assumeWithCex;
    if (incrementalAbstr) {
      ret = Ddi_SatSolve(solver,assumeWithAbstr,timeLimit);
      if (!ret) break;
    }
    else {
      ret = -1;
    }
    if (ret < 0) {
      /* undefined */
      Ddi_SatSolver_t *solver2 = Ddi_SatSolverAlloc();
      Ddi_Bdd_t *fDupAbstr = Ddi_BddCompose(fDup,abstrVars,noAbstrOut);
      incrementalAbstr=0; // for next iterations
      fClauses = Ddi_AigClauses(fDupAbstr,0,NULL);
      Ddi_SatSolverAddClauses(solver2,fClauses);
      ret = Ddi_SatSolve(solver2,NULL,-1);
      if (ret) {
        cex = Ddi_SatSolverGetCexCube(solver2,piVars);
      }
      Ddi_ClauseArrayFree(fClauses);
      Ddi_SatSolverQuit(solver2);
      Ddi_Free(fDupAbstr);
      if (!ret) {
        break;
      }
    }
    else {
      if (!tryJustify) {
        cex = Ddi_SatSolverGetCexCube(solver,piVars);
      }
      else {
        Ddi_ClauseArray_t *fClauses;
        Ddi_ClauseArray_t *justifyClauses;
        Ddi_Clause_t *impliedLit;
        justifyClauses = Ddi_ClauseArrayAlloc(0);
        fClauses = Ddi_AigClausesWithJustify(f,0,NULL,
                                             justifyClauses);
        bAigEdge_t baig = Ddi_BddToBaig(f);
        int vCnf = bAig_NodeIsInverted(baig) ?
          -DdiAig2CnfId(bmgr,baig) :
          DdiAig2CnfId(bmgr,baig);
        impliedLit = Ddi_ClauseAlloc(0,1);
        DdiClauseAddLiteral(impliedLit,vCnf);
        cex = Ddi_SatSolverGetCexCubeWithJustify(solver,
       		    piVars,justifyClauses,impliedLit);
        Ddi_ClauseFree(impliedLit);
        Ddi_ClauseArrayFree(justifyClauses);
        Ddi_ClauseArrayFree(fClauses);
      }
    }
    assumeWithCex = Ddi_ClauseJoin(assume,cex);
    int myAbstrCnt = 0;
    if (tryJustify) {
      nIter++;
      for (j=0; j<Ddi_VararrayNum(abstrVars); j++) {
        Ddi_Var_t *a_j = Ddi_VararrayRead(abstrVars,j);
        Ddi_Bdd_t *c_j = Ddi_BddarrayRead(noAbstrOut,j);
        Ddi_Bdd_t *p_j = prevAbstr==NULL ? NULL : Ddi_BddarrayRead(prevAbstr,j);
        bAigEdge_t aBaig = Ddi_VarToBaig(a_j);
        int vCnf = DdiAig2CnfId(bmgr,aBaig);
        if (Ddi_BddIsOne(c_j)) {
          if (Ddi_VarReadMark(a_j) == 1) {
            /* in final - force refinement */
            Ddi_BddNotAcc(c_j);
            //            printf("%sREF[%d] = %s\n", nRef?"":"\n",
            //       nRef, Ddi_VarName(a_j));
            nRef++; myAbstrCnt++;
            DdiClauseAddLiteral(assumeWithAbstr,-vCnf);
          }
          else if (p_j==NULL || Ddi_BddIsOne(p_j)) {
            DdiClauseAddLiteral(assumeWithAbstr,vCnf);
          }
        }
        else {
          DdiClauseAddLiteral(assumeWithAbstr,-vCnf);
        }
      }
    }
    else {
      Ddi_Clause_t *cl = NULL;
      Sat = Ddi_SatSolve(solver,assumeWithCex,-1);
      Ddi_ClauseFree(assumeWithAbstr);
      assumeWithAbstr = Ddi_ClauseAlloc(0,1);
      if (Sat) {
        *result=1;
      }
      else {
        cl = Ddi_SatSolverFinal(solver,-1);
      }
      nIter++;
      for (j=0; cl!=NULL && j<cl->nLits; j++) {
        int vj = abs(cl->lits[j]);
        Pdtutil_Assert(vj<ddm->cnf.cnf2aigSize,"invalid cnf 2 aig");
        bAigEdge_t baig = ddm->cnf.cnf2aig[vj];
        Ddi_Var_t *v = Ddi_VarFromBaig(ddm,baig);
        //printf("FIN[%d] = %s\n", j, Ddi_VarName(v));
        Ddi_VarWriteMark(v,1);
      }
      for (j=0; j<Ddi_VararrayNum(abstrVars); j++) {
        Ddi_Var_t *a_j = Ddi_VararrayRead(abstrVars,j);
        Ddi_Bdd_t *c_j = Ddi_BddarrayRead(noAbstrOut,j);
        Ddi_Bdd_t *p_j = prevAbstr==NULL ? NULL : Ddi_BddarrayRead(prevAbstr,j);
        bAigEdge_t aBaig = Ddi_VarToBaig(a_j);
        int vCnf = DdiAig2CnfId(bmgr,aBaig);
        if (Ddi_BddIsOne(c_j)) {
          if (Ddi_VarReadMark(a_j) == 1) {
            /* in final - force refinement */
            Ddi_BddNotAcc(c_j);
            //            printf("%sREF[%d] = %s\n", nRef?"":"\n",
            //       nRef, Ddi_VarName(a_j));
            nRef++; myAbstrCnt++;
            DdiClauseAddLiteral(assumeWithAbstr,-vCnf);
          }
          else if (p_j==NULL || Ddi_BddIsOne(p_j)) {
            DdiClauseAddLiteral(assumeWithAbstr,vCnf);
          }
        }
        else {
          DdiClauseAddLiteral(assumeWithAbstr,-vCnf);
        }
      }
      for (j=0; cl!=NULL && j<cl->nLits; j++) {
        int vj = abs(cl->lits[j]);
        Pdtutil_Assert(vj<ddm->cnf.cnf2aigSize,"invalid cnf 2 aig");
        bAigEdge_t baig = ddm->cnf.cnf2aig[vj];
        Ddi_Var_t *v = Ddi_VarFromBaig(ddm,baig);
        //printf("FIN[%d] = %s\n", j, Ddi_VarName(v));
        Ddi_VarWriteMark(v,0);
      }
      Pdtutil_Assert(myAbstrCnt>0,"missing refinement");
      Ddi_ClauseFree(cl);
      Ddi_ClauseFree(cex);
      Ddi_ClauseFree(assumeWithCex);
    }
  }    
  currTime = util_cpu_time();

  //  nRef += nPrev;

  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
    fprintf(dMgrO(ddm),
      "[%d(%d prev)/%d/%d/%d ref/abstr/tot/TOT - %d iter](time: %s)| ",
	    nRef+nPrev, nPrev, nVars-nRef, nVars, nVarsTot, nIter,
    util_print_time(currTime-startTime))
  );

  Ddi_Free(fDup);

  DdiAig2CnfIdClose(ddm);

  Ddi_ClauseFree(piVars);
  Ddi_ClauseFree(assumeVars);
  Ddi_ClauseFree(assume);
  Ddi_ClauseFree(assumeWithAbstr);

  Ddi_SatSolverQuit(solver);

  return noAbstrOut;

}

/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_AigExistProjectBySubset(
  Ddi_Bdd_t *f,
  Ddi_Bdd_t *gHit,
  Ddi_Varset_t *projVars,
  int minCube,
  int maxIter,
  int *result
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  int j, jj, Sat, isSat=1, nVars = Ddi_VarsetNum(projVars);
  Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity (ddm);

  Ddi_SatSolver_t *solver = Ddi_SatSolverAlloc();

  Ddi_ClauseArray_t *justifyClauses;
  Ddi_ClauseArray_t *fClauses;
  Ddi_ClauseArray_t *gClauses;
  Ddi_Varset_t *suppVars = Ddi_BddSupp(f);
  Ddi_Varset_t *gVars = Ddi_BddSupp(gHit);

  Ddi_Clause_t *varClause = Ddi_ClauseAlloc(0,0);
  Ddi_Clause_t *piClause = Ddi_ClauseAlloc(0,0);

  Ddi_Clause_t *impliedLit;
  Ddi_Bdd_t *resAig=NULL;
  Ddi_Bdd_t *cubeAig=NULL;

  Ddi_Vararray_t *projA;
  Ddi_Vararray_t *piAA=NULL;
  Ddi_Vararray_t *piB=NULL;

  int check=1, isSat1=0, cubeSize=0, single=0, endSearch=0;
  //  if (maxIter>100) maxIter = 100;

  DdiAig2CnfIdInit(ddm);
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
    fprintf(dMgrO(ddm), "|existProjSUBSET:%d,%d|.\n",
	    Ddi_BddSize(f), Ddi_BddSize(gHit))
  );


  Ddi_VarsetSetArray(gVars);
  Ddi_VarsetSetArray(suppVars);
  Ddi_VarsetSetArray(projVars);

  Ddi_VarsetDiffAcc(suppVars,gVars);
  Ddi_VarsetDiffAcc(suppVars,projVars);

  piAA = Ddi_VararrayMakeFromVarset(suppVars,1);
  Ddi_VarsetDiffAcc(gVars,projVars);
  piB = Ddi_VararrayMakeFromVarset(gVars,1);
  Ddi_Free(gVars);
  Ddi_Free(suppVars);
  projA = Ddi_VararrayMakeFromVarset(projVars,1);

  for (j=0; j<nVars; j++) {
    Ddi_Var_t *ns_j = Ddi_VararrayRead(projA,j);
    bAigEdge_t nsBaig = Ddi_VarToBaig(ns_j);
    int psCnf, nsCnf;

    nsCnf = DdiAig2CnfId(bmgr,nsBaig);
    psCnf = DdiAig2CnfNewId(ddm);

    DdiClauseAddLiteral(varClause,nsCnf);

    Pdtutil_Assert(nsCnf==2*(j)+1,"Wrong cnf id");
    Pdtutil_Assert(psCnf==2*(j+1),"Wrong cnf id");
  }

  for (j=0; j<Ddi_VararrayNum(piAA); j++) {
    Ddi_Var_t *v = Ddi_VararrayRead(piAA,j);
    bAigEdge_t vBaig = Ddi_VarToBaig(v);
    int vCnf = DdiAig2CnfId(bmgr,vBaig);
    DdiClauseAddLiteral(piClause,vCnf);
    Ddi_VarWriteMark(v,0);
  }

  for (j=0; j<Ddi_VararrayNum(piB); j++) {
    Ddi_Var_t *v = Ddi_VararrayRead(piB,j);
    bAigEdge_t vBaig = Ddi_VarToBaig(v);
    int vCnf = DdiAig2CnfId(bmgr,vBaig);
  }

  justifyClauses = Ddi_ClauseArrayAlloc(0);
  fClauses = Ddi_AigClausesWithJustify(f,0,NULL,justifyClauses);
  //  fClauses = Ddi_AigClauses(f,0,NULL);
  gClauses = Ddi_AigClauses(gHit,0,NULL);

  Ddi_SatSolverAddClauses(solver,fClauses);
  Ddi_SatSolverAddClauses(solver,gClauses);
  Ddi_ClauseArrayFree(fClauses);
  Ddi_ClauseArrayFree(gClauses);

  if (1) {
    bAigEdge_t baig = Ddi_BddToBaig(f);
    int vCnf = bAig_NodeIsInverted(baig) ? -DdiAig2CnfId(bmgr,baig) :
                                            DdiAig2CnfId(bmgr,baig);
    impliedLit = Ddi_ClauseAlloc(0,nVars);
    DdiClauseAddLiteral(impliedLit,vCnf);
  }
  
  Sat=1;
  cubeAig = Ddi_BddMakeConstAig(ddm,1);

  *result = 0;

  for (jj=0; !endSearch && (maxIter<0 || jj<maxIter) && Sat; jj++) {

   /* enumerator */

    Sat = Ddi_SatSolve(solver,NULL,-1);

    if (Sat<0) {
      Sat = 0;
      jj = maxIter;
    }
    if (Sat==0) {
      isSat=0;
    }

    endSearch=1;

    if (Sat) {
      //    Ddi_Clause_t *cex = Ddi_SatSolverGetCexCube(solver,piClause);
      Ddi_Clause_t *cex = Ddi_SatSolverGetCexCubeWithJustify(solver,
       		    piClause,justifyClauses,impliedLit);
      // try complement
      Ddi_Clause_t *cexNeg = Ddi_ClauseNegLits(cex);

      int Sat1 = Ddi_SatSolve(solver,cexNeg,-1);
      if (!Sat1) {
	// unsat found. Work on solver final

	Ddi_Clause_t *cl = Ddi_SatSolverFinal(solver,-1);
	Ddi_Clause_t *cube = Ddi_ClauseNegLits(cl);

	Pdtutil_Assert(!Ddi_SatSolve(solver,cube,-1),"UNSAT needed");

	//	jj=maxIter;

	if (cube->nLits==1) {
	  Ddi_Clause_t *cube1 = Ddi_ClauseNegLits(cube);
	  Ddi_ClauseFree(cube);
	  cube = cube1;
	  endSearch=0;
	  single++;
	}
	else {
	  do {
	    DdiClauseRemoveLiteral(cube, cube->nLits-1);
	    Sat1 = Ddi_SatSolve(solver,cube,-1);
	    if (!Sat1 && cube->nLits==1) {
	      Ddi_Clause_t *cube1 = Ddi_ClauseNegLits(cube);
	      Ddi_ClauseFree(cube);
	      cube = cube1;
	      endSearch=0;
	      single++;
	      Sat1=1;
	    }
	    Pdtutil_Assert(cube->nLits>0,"no lits in cube");
	  } while (!Sat1);
	}
	if (cube->nLits==0) {
	  endSearch=1;
	}

	for (j=0; j<cube->nLits; j++) {
	  int vj = abs(cl->lits[j]);
	  Pdtutil_Assert(vj<ddm->cnf.cnf2aigSize,"invalid cnf 2 aig");
	  bAigEdge_t baig = ddm->cnf.cnf2aig[vj];
	  Ddi_Var_t *v = Ddi_VarFromBaig(ddm,baig);
	  Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(v,cl->lits[j]>0);
	  Ddi_Clause_t *constrClause = Ddi_ClauseAlloc(0,1);
	  Ddi_BddAndAcc(cubeAig,lit);
	  Ddi_Free(lit);
	  DdiClauseAddLiteral(constrClause,cl->lits[j]);
	  Ddi_SatSolverAddClause(solver,constrClause);
	  Ddi_ClauseFree(constrClause);
	  if (!endSearch) Ddi_VarWriteMark(v,1);
	}
	cubeSize += cube->nLits;

	if (!endSearch) {

	  DdiClauseClear(piClause);
	  for (j=Ddi_VararrayNum(piAA)-1; j>=0; j--) {
	    Ddi_Var_t *v = Ddi_VararrayRead(piAA,j);
	    if (Ddi_VarReadMark(v) == 1) {
	      Ddi_VarWriteMark(v,0);
	      Ddi_VararrayRemove(piAA,j);
	      continue;
	    }
	    bAigEdge_t vBaig = Ddi_VarToBaig(v);
	    int vCnf = DdiAig2CnfId(bmgr,vBaig);
	    DdiClauseAddLiteral(piClause,vCnf);
	  }
	}

	Ddi_ClauseFree(cl);
	Ddi_ClauseFree(cube);

      }
      Ddi_ClauseFree(cexNeg);
      Ddi_ClauseFree(cex);

      if (minCube>0 && cubeSize<minCube && Ddi_VararrayNum(piAA)>0) {
	endSearch=0;
      }
    }

  }

  DdiAig2CnfIdClose(ddm);

  Ddi_Free(projA);
  Ddi_Free(piAA);
  Ddi_Free(piB);

  Ddi_ClauseFree(varClause);
  Ddi_ClauseFree(piClause);
  Ddi_ClauseArrayFree(justifyClauses);
  Ddi_ClauseFree(impliedLit);

  Ddi_SatSolverQuit(solver);

  if (!isSat) {
    if (result!=NULL) {
      *result = -1;
    }
    Ddi_Free(cubeAig);
  }
  else if (maxIter>=0 && jj>=maxIter) {
    /* partial */
    if (result!=NULL) {
      *result = 0;
    }
  }
  else {
    if (result!=NULL) {
      *result = 1;
    }
  }

  if (cubeAig!=NULL) {
    // Pdtutil_Assert(!Ddi_AigSatAnd(itp,b,care),"wrong itp by generalization");
    resAig = Ddi_BddDup(f);
    Ddi_AigConstrainCubeAcc(resAig,cubeAig);
    Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
	fprintf(dMgrO(ddm),
	"|existProjSUBS:%d| - |cube vars|=%d (single: %d) - size = %d).\n",
		Ddi_BddSize(f),cubeSize,single,Ddi_BddSize(resAig))
    );
  }

  if (check && resAig!=NULL) {
    Pdtutil_Assert(Ddi_AigSatAnd(resAig,gHit,NULL),"wrong epSUBS");
  }

  Ddi_Free(cubeAig);

  return resAig;

}

#if 1
/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_AigExistProjectByGenClauses(
  Ddi_Bdd_t *a,
  Ddi_Bdd_t *b,
  Ddi_Bdd_t *care,
  Ddi_Varset_t *projVars,
  int maxIter,
  int doConstr,
  int *result
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(a);
  int j, jj, Sat, isSat=0, nVars = Ddi_VarsetNum(projVars);
  int totLits=0;

  Ddi_SatSolver_t *solver = Ddi_SatSolverAlloc();
  Ddi_ClauseArray_t *aClauses;
  Ddi_ClauseArray_t *bClauses;
  Ddi_ClauseArray_t *justifyClauses;
  Ddi_ClauseArray_t *careClauses=NULL;
  Ddi_ClauseArray_t *careAClauses=NULL;
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  Ddi_Clause_t *varClause = Ddi_ClauseAlloc(0,0);
  Ddi_Clause_t *piClause = Ddi_ClauseAlloc(0,0);
  Ddi_Clause_t *impliedLit;
  Ddi_TernarySimMgr_t *xMgr=NULL;
  Ddi_Bdd_t *itp=NULL;
  Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity (ddm);
  int coreTwice=1;
  Ddi_Varset_t *suppVars = Ddi_BddSupp(a);
  Ddi_Varset_t *bVars = Ddi_BddSupp(b);
  Ddi_Vararray_t *projA;
  Ddi_Vararray_t *piAA=NULL;
  Ddi_Vararray_t *piB=NULL;
  int doTernary = 0;
  int check=0, isSat1=0;
  int countLits = 0;
  //  if (maxIter>100) maxIter = 100;

  if (check) {
    isSat1 = Ddi_AigSatAnd(a,b,care);
  }

  DdiAig2CnfIdInit(ddm);
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
    fprintf(dMgrO(ddm), "|existProjGEN:%d,%d(%d)|.\n",
	    Ddi_BddSize(a), Ddi_BddSize(b), care?Ddi_BddSize(care):0)
  );


  Ddi_VarsetSetArray(bVars);
  Ddi_VarsetSetArray(suppVars);
  Ddi_VarsetSetArray(projVars);

  if (care != NULL && !Ddi_BddIsOne(care)) {
    Ddi_Varset_t *suppCare = Ddi_BddSupp(care);
    Ddi_VarsetSetArray(suppCare);
    Ddi_VarsetUnionAcc(bVars,suppCare);
    Ddi_Free(suppCare);
  }
  Ddi_VarsetDiffAcc(suppVars,bVars);
  Ddi_VarsetDiffAcc(suppVars,projVars);
  piAA = Ddi_VararrayMakeFromVarset(suppVars,1);
  Ddi_VarsetDiffAcc(bVars,projVars);
  piB = Ddi_VararrayMakeFromVarset(bVars,1);
  Ddi_Free(bVars);
  Ddi_Free(suppVars);
  projA = Ddi_VararrayMakeFromVarset(projVars,1);

  for (j=0; j<nVars; j++) {
    Ddi_Var_t *ns_j = Ddi_VararrayRead(projA,j);
    bAigEdge_t nsBaig = Ddi_VarToBaig(ns_j);
    int psCnf, nsCnf;

    nsCnf = DdiAig2CnfId(bmgr,nsBaig);
    psCnf = DdiAig2CnfNewId(ddm);

    DdiClauseAddLiteral(varClause,nsCnf);

    Pdtutil_Assert(nsCnf==2*(j)+1,"Wrong cnf id");
    Pdtutil_Assert(psCnf==2*(j+1),"Wrong cnf id");
  }

  for (j=0; j<Ddi_VararrayNum(piAA); j++) {
    Ddi_Var_t *v = Ddi_VararrayRead(piAA,j);
    bAigEdge_t vBaig = Ddi_VarToBaig(v);
    int vCnf = DdiAig2CnfId(bmgr,vBaig);
    DdiClauseAddLiteral(piClause,vCnf);
  }

  for (j=0; j<Ddi_VararrayNum(piB); j++) {
    Ddi_Var_t *v = Ddi_VararrayRead(piB,j);
    if (Ddi_VarReadMark(v) == 0) {
      bAigEdge_t vBaig = Ddi_VarToBaig(v);
      int vCnf = DdiAig2CnfId(bmgr,vBaig);
      //      Pdtutil_Assert(vCnf>2*nVars,"Wrong cnf id");
    }
  }

  justifyClauses = Ddi_ClauseArrayAlloc(0);
  aClauses = Ddi_AigClausesWithJustify(a,0,NULL,justifyClauses);
  bClauses = Ddi_AigClauses(b,0,NULL);

  if (doTernary) {
    Ddi_Bddarray_t *fA = Ddi_BddarrayAlloc(ddm,0);
    Ddi_Bdd_t *dummyDelta = Ddi_BddMakeConstAig(ddm,1);
    //Ddi_BddMakeLiteralAig(Ddi_VararrayRead(ps,nVars-2),1);
    Ddi_Bdd_t *mySpec = Ddi_BddNot(a);
    Ddi_BddarrayWrite(fA,0,dummyDelta);
    xMgr = Ddi_TernarySimInit(fA,mySpec,piAA,projA);
    Ddi_Free(mySpec);
    Ddi_Free(dummyDelta);
    Ddi_Free(fA);
  }

  if (1) {
    bAigEdge_t baig = Ddi_BddToBaig(a);
    int vCnf = bAig_NodeIsInverted(baig) ? -DdiAig2CnfId(bmgr,baig) :
                                            DdiAig2CnfId(bmgr,baig);
    impliedLit = Ddi_ClauseAlloc(0,nVars);
    DdiClauseAddLiteral(impliedLit,vCnf);
  }

  if (care!=NULL) {
    careClauses = Ddi_AigClauses(care,0,NULL);
  }
  DdiAig2CnfIdClose(ddm);

  Ddi_SatSolver_t *solver2 = NULL;

  Ddi_SatSolverAddClauses(solver,aClauses);
  Ddi_SatSolverAddClauses(solver,bClauses);
  if (careClauses!=NULL) {
    Ddi_SatSolverAddClauses(solver,careClauses);
    if (doConstr) {
      solver2 = Ddi_SatSolverAlloc();
      Ddi_SatSolverAddClauses(solver2,careClauses);
    }
    Ddi_ClauseArrayFree(careClauses);
  }
  Ddi_ClauseArrayFree(aClauses);
  Ddi_ClauseArrayFree(bClauses);

  Sat=1;
  itp = Ddi_BddMakeConstAig(ddm,0);

  *result = 0;
  float totOrigLits=0, totConstrLits=0;
  int nCl=0;

  for (jj=0; (maxIter<0 || jj<maxIter) && Sat; jj++) {
    /* enumerator */
    Sat = Ddi_SatSolve(solver,NULL,-1);
    if (Sat<0) {
      Sat = 0;
      jj = maxIter;
    }

    if (Sat) {
      //      Ddi_Clause_t *cex = Ddi_SatSolverGetCexCube(solver,varClause);
      Ddi_Clause_t *cex = Ddi_SatSolverGetCexCubeWithJustify(solver,
       		    varClause,justifyClauses,impliedLit);
      isSat=1;
      if (doTernary) {
	Ddi_Clause_t *cexPi = Ddi_SatSolverGetCexCube(solver,piClause);
        if (cexPi->nLits>0) {
          Ddi_Clause_t *reducedCex = Ddi_ClauseAlloc(0,1);
          int i, rem=0, enTernary = Ddi_TernaryPdrSimulate(xMgr,2,cexPi,cex,NULL,1,0);

          Ddi_Clause_t *cexDup = Ddi_ClauseDup(cex);
          int isSatIntern=0;

          Pdtutil_Assert(enTernary,"wrong cex justify");
          Ddi_ClauseFree(cexPi);

          for (i=0; i<cex->nLits; i++) {
            if (!Ddi_TernaryPdrSimulate(xMgr, 0, NULL, NULL, NULL, 0, cex->lits[i])) {
              DdiClauseAddLiteral(reducedCex,cex->lits[i]);
            }
            else {
              rem++;
              cexDup->lits[i] = 0;
            }
          }
          if (cex->nLits > reducedCex->nLits) {
            int nl = cex->nLits;
            Ddi_ClauseSort(reducedCex);
            Ddi_ClauseFree(cex);
            cex = reducedCex;
          }
          else {
            Ddi_ClauseFree(reducedCex);
          }
          Ddi_ClauseFree(cexDup);
        }
      }
      if (countLits) {
	int i;
	for (i=0; i<cex->nLits; i++) {
	  cex->lits[i];
	}
      }
      if (doConstr) {
        Ddi_Clause_t *cex0 = cex;
        cex = Ddi_SatSolverCubeConstrain(solver2,cex0);
        totOrigLits += cex0->nLits;
        totConstrLits += cex->nLits;
        Ddi_ClauseFree(cex0);
        nCl++;
      }

      if (1) {
	Ddi_Clause_t *cl = Ddi_ClauseNegLits(cex);
	Ddi_Bdd_t *cubeAig = Ddi_BddMakeFromClauseWithVars(cl,projA);
        totLits += cl->nLits;
	Ddi_BddNotAcc(cubeAig);
	//	Pdtutil_Assert(!Ddi_SatSolve(solver2,cube,-1),
	//	       "wrong cube enlargement");
	//	Pdtutil_Assert(!Ddi_AigSatAnd(cubeAig,b,care),
	//	       "wrong itp by generalization");

	Ddi_BddOrAcc(itp,cubeAig);
	Ddi_Free(cubeAig);

	Ddi_SatSolverAddClause(solver,cl);
        /////
	Ddi_ClauseFree(cl);
      }
      Ddi_ClauseFree(cex);
    }

  }

  if (doConstr) {
    Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
        fprintf(dMgrO(ddm),
              "clauseConstr - compacting from %4.1f to %4.1f avg lits.\n",
                totOrigLits/nCl, totConstrLits/nCl));
    Ddi_SatSolverQuit(solver2);
  }
  
  Ddi_Free(projA);
  Ddi_Free(piAA);
  Ddi_Free(piB);

  if (xMgr != NULL) {
    Ddi_TernarySimQuit(xMgr);
  }

  Ddi_ClauseFree(varClause);
  Ddi_ClauseFree(piClause);
  Ddi_ClauseFree(impliedLit);
  Ddi_ClauseArrayFree(justifyClauses);

  Ddi_SatSolverQuit(solver);

  if (!isSat) {
    if (result!=NULL) {
      *result = -1;
    }
    Ddi_Free(itp);
  }
  else if (maxIter>=0 && jj>=maxIter) {
    /* partial */
    if (result!=NULL) {
      *result = 0;
    }
  }
  else {
    if (result!=NULL) {
      *result = 1;
    }
  }

  if (itp!=NULL) {
    // Pdtutil_Assert(!Ddi_AigSatAnd(itp,b,care),"wrong itp by generalization");

    if (jj<maxIter) jj--;
    Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
	fprintf(dMgrO(ddm), "|existProjGEN:%d| - nGen:%d%s (avg-#lits: %4.1f).\n",
                Ddi_BddSize(itp),jj,(jj>=maxIter)?"(partial)":"",
                (float)totLits/(jj))
    );
  }

  if (check && itp!=NULL) {
    Pdtutil_Assert(Ddi_AigSatAnd(b,itp,care),"wrong itpgen");
  }


  return itp;

}

#endif


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void * lglNew (void * state, size_t bytes) {
  void * res;
  (void) state;
  res = malloc (bytes);
  if (!res) exit(1);
  return res;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

static void lglDel (void * state, void * ptr, size_t bytes) {
  (void) state;
  free (ptr);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

static void * lglRsz (void * state, void * ptr, size_t o, size_t n) {
  void * res;
  (void) state;
  res = realloc (ptr, n);
  if (!res) exit(1);
  return res;
}



/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
ClauseLitCompare(
  const void *l0p,
  const void *l1p
)
{
  return (abs(*((int *)l0p))-abs(*((int *)l1p)));
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
ClauseCompare(
  const void *c0p,
  const void *c1p
)
{
  Ddi_Clause_t *cl0 = *((Ddi_Clause_t **)c0p);
  Ddi_Clause_t *cl1 = *((Ddi_Clause_t **)c1p);

  int i;


  // if (cl0->nLits != cl1->nLits) return (cl0->nLits-cl1->nLits);


  for ( i=0; i < cl0->nLits && i<cl1->nLits; i++ ) {
    if (abs(cl0->lits[i]) > abs(cl1->lits[i]))
      return -1;
    if (abs(cl0->lits[i]) < abs(cl1->lits[i]))
      return 1;
    /* same var: check sign: neg lit considered greater */
    if (cl0->lits[i] < cl1->lits[i])
      return -1;
    if (cl0->lits[i] > cl1->lits[i])
      return 1;
  }
  if ( i == cl0->nLits && i < cl1->nLits )
    return -1;
  if ( i < cl0->nLits && i == cl1->nLits )
    return 1;
  return 0;
}



/**Function********************************************************************
  Synopsis    [ternary evaluation]
  Description [ternary evaluation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int tSimStackPush (
  Ddi_TernarySimMgr_t *xMgr,
  int gId
)
{
  int l;

  if (xMgr->ternaryGates[gId].inStack) return -1;

  l = xMgr->ternaryGates[gId].level;
  Pdtutil_Assert(xMgr->levelStackId[l]<(xMgr->levelSize[l])&&
		 xMgr->levelStackId[l]>=0,"error in level stack push");
  xMgr->levelStack[l][xMgr->levelStackId[l]++] = gId;
  xMgr->ternaryGates[gId].inStack = 1;

  return l;
}

/**Function********************************************************************
  Synopsis    [ternary evaluation]
  Description [ternary evaluation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int tSimStackFoPush (
  Ddi_TernarySimMgr_t *xMgr,
  int gId
)
{
  int fo, i;

  fo = xMgr->ternaryGates[gId].foCnt;
  for (i=0; i<fo; i++) {
    int gFo = xMgr->ternaryGates[gId].foIds[i];
    Pdtutil_Assert(xMgr->ternaryGates[gId].level<xMgr->ternaryGates[gFo].level,
		   "wrong levelizing");
    tSimStackPush(xMgr,gFo);
  }

  return fo;
}

/**Function********************************************************************
  Synopsis    [ternary evaluation]
  Description [ternary evaluation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int tSimStackPop (
  Ddi_TernarySimMgr_t *xMgr
)
{
  int g, l;

  Pdtutil_Assert(xMgr->currLevel>=0,
		 "error in level pop");
  if (xMgr->currLevel>=xMgr->nLevels) return -1;
  for(; (xMgr->currLevel<xMgr->nLevels) &&
	(xMgr->levelStackId[xMgr->currLevel]==0);) {
    xMgr->currLevel++;
  }
  if (xMgr->currLevel>=xMgr->nLevels) return -1;

  l = xMgr->currLevel;
  g = xMgr->levelStack[l][--xMgr->levelStackId[l]];
  xMgr->ternaryGates[g].inStack = 0;

  return g;
}

/**Function********************************************************************
  Synopsis    [ternary evaluation]
  Description [ternary evaluation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void tSimStackReset (
  Ddi_TernarySimMgr_t *xMgr
)
{
  while (tSimStackPop(xMgr) >= 0);
  xMgr->currLevel=0;
}



/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Clause_t *
eqClassesSplitClauses(
  Pdtutil_EqClasses_t *eqCl,
  Ddi_SatSolver_t *solver
)
{
  int nVars = Pdtutil_EqClassesNum(eqCl)-1;
  int i, actCnf, vCnf;

  Ddi_Clause_t *assumeSat=NULL;
  Ddi_ClauseArray_t *splitClasses[2];
  Ddi_Clause_t *splitClass0;
  Ddi_Clause_t *splitClass1;

  if (nVars<=0) return 0;

  assumeSat = Ddi_ClauseAlloc(0,nVars+1);

  actCnf = Pdtutil_EqClassCnfActRead(eqCl,0);
  vCnf = Pdtutil_EqClassCnfVarRead(eqCl,0);

  /* gen void split clauses */
  splitClasses[0] = Ddi_ClauseArrayAlloc(nVars+1);
  splitClasses[1] = Ddi_ClauseArrayAlloc(nVars+1);
  for (i=0; i<=nVars; i++) {
    splitClass0 = Ddi_ClauseAlloc(0,1);
    splitClass1 = Ddi_ClauseAlloc(0,1);
    Ddi_ClauseArrayPush(splitClasses[0],splitClass0);
    Ddi_ClauseArrayPush(splitClasses[1],splitClass1);
  }

  for (i=0; i<=nVars; i++) {
    int myCnf, ldr_i;

    actCnf = Pdtutil_EqClassCnfActRead(eqCl,i);
    vCnf = Pdtutil_EqClassCnfVarRead(eqCl,i);

    switch (Pdtutil_EqClassState(eqCl,i)) {

    case Pdtutil_EqClass_Unassigned_c:
    case Pdtutil_EqClass_Singleton_c:
      /* nothing to do */
      break;
    case Pdtutil_EqClass_Ldr_c:
      if (i==0 && Pdtutil_EqClassesRefineStepsNum(eqCl)==0) {
	/* first time in constant class */
	/* do nothing */
      }
      else if (Pdtutil_EqClassNum(eqCl,i)==1) {
	/* not a class: disable split guard */
	Ddi_Clause_t *cl = Ddi_ClauseAlloc(0,1);
	DdiClauseAddLiteral(cl, -actCnf);
	Ddi_SatSolverAddClause(solver,cl);
	Ddi_ClauseFree(cl);
	Pdtutil_EqClassSetState(eqCl,i,Pdtutil_EqClass_Singleton_c);
      }
      else {
	/* existing or new class leader */
	/* init split activation clauses */
	/* actCnf => (!vCnf => (... | ... | ...)) */
	DdiClauseAddLiteral(splitClasses[0]->clauses[i], -actCnf);
	if (i>0) {
	  DdiClauseAddLiteral(splitClasses[0]->clauses[i], vCnf);
	  /* actCnfA[i] => (vCnfA[i] => (... | ... | ...)) */
	  DdiClauseAddLiteral(splitClasses[1]->clauses[i], -actCnf);
	  DdiClauseAddLiteral(splitClasses[1]->clauses[i], -vCnf);
	}
      }
      break;
    case Pdtutil_EqClass_Member_c:
      ldr_i = Pdtutil_EqClassLdr(eqCl,i);
      Pdtutil_Assert(ldr_i>=0&&ldr_i<=nVars,"wrong class leader index");
      myCnf = Pdtutil_EqClassIsCompl(eqCl,i) ? -vCnf : vCnf;
      /* ask value different from leader (in OR with other members) */
      DdiClauseAddLiteral(splitClasses[0]->clauses[ldr_i], myCnf);
      if (ldr_i>0) {
	DdiClauseAddLiteral(splitClasses[1]->clauses[ldr_i], -myCnf);
      }
      /* disable activation literal */
      DdiClauseAddLiteral(assumeSat, -actCnf);
      break;
    case Pdtutil_EqClass_None_c:
    default:
      Pdtutil_Assert(0, "wrong class state");
      break;
    }
  }

  for (i=0; i<=nVars; i++) {
    int k;
    for (k=0; k<2; k++) {
      Ddi_Clause_t *cl = splitClasses[k]->clauses[i];
      if (Ddi_ClauseNumLits(cl)>0) {
	Ddi_SatSolverAddClause(solver,cl);
      }
    }
  }
  Ddi_ClauseArrayFree(splitClasses[0]);
  Ddi_ClauseArrayFree(splitClasses[1]);

  return assumeSat;
}


/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_AigCnfInfo_t *
DdiGenAigCnfInfo(
  bAig_Manager_t *bmgr,
  bAig_array_t *visitedNodes,
  Ddi_Bddarray_t *roots,
  int enBlocks
)
{
  Ddi_AigCnfInfo_t *aigCnfInfo = Pdtutil_Alloc(Ddi_AigCnfInfo_t,visitedNodes->num);
  int j, last;
  int doUseMapped = 0;

  last = visitedNodes->num-1;
  for (j=0; j<visitedNodes->num; j++) {
    bAigEdge_t baig = visitedNodes->nodes[j];
    bAig_AuxInt(bmgr,baig) = j;
    aigCnfInfo[j].foCnt=0;
    aigCnfInfo[j].isRoot=1;
    aigCnfInfo[j].isMapped=0;
    aigCnfInfo[j].mapLdr=0;
    aigCnfInfo[j].ldr=j;
    aigCnfInfo[j].ca=NULL;
    if (!bAig_isVarNode(bmgr,baig) && !bAig_NodeIsConstant(baig)) {
      int ir, il;
      bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
      bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);

      Pdtutil_Assert(left!=bAig_NULL && right!=bAig_NULL,"NULL child");

      ir = bAig_AuxInt(bmgr,right);
      il = bAig_AuxInt(bmgr,left);
      aigCnfInfo[ir].foCnt++;
      aigCnfInfo[il].foCnt++;
    }
  }

  if (0) {
    int cnt2=0, cnt1=0;
    for (j=0; j<visitedNodes->num; j++) {
      bAigEdge_t baig = visitedNodes->nodes[j];
      if (bAig_isVarNode(bmgr,baig) && !bAig_NodeIsConstant(baig)) {
        char *name = bAig_NodeReadName(bmgr,baig);
        int ii = bAig_AuxInt(bmgr,baig);
        //        printf("VAR %s -> fanout: %d\n", name, aigCnfInfo[ii].foCnt);
      }
    }
    for (j=0; j<visitedNodes->num; j++) {
      bAigEdge_t baig = visitedNodes->nodes[j];
      if (!bAig_isVarNode(bmgr,baig) && !bAig_NodeIsConstant(baig)) {
        int ir, il;
        bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
        bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);
        
        Pdtutil_Assert(left!=bAig_NULL && right!=bAig_NULL,"NULL child");
        if (bAig_isVarNode(bmgr,right) && bAig_isVarNode(bmgr,left)) {
        
          char *namer = bAig_NodeReadName(bmgr,right);
          char *namel = bAig_NodeReadName(bmgr,left);
          ir = bAig_AuxInt(bmgr,right);
          il = bAig_AuxInt(bmgr,left);
          if (aigCnfInfo[ir].foCnt==1 && aigCnfInfo[il].foCnt==1) {
            printf("OK: %s %s\n", namer, namel); cnt2++;
          }
          else if (aigCnfInfo[ir].foCnt==1 || aigCnfInfo[il].foCnt==1) {
            int oneFoL = aigCnfInfo[il].foCnt==1;
            printf("ok: %s%s %s%s\n", namer,oneFoL?"":"(1)",
                   namel,oneFoL?"(1)":""); cnt1++;
          }
        }
      }
    }
    printf("CNT: %d - >CNT1: %d\n", cnt2, cnt1);
  }

  if (roots!=NULL) {
    for (j=0; j<Ddi_BddarrayNum(roots); j++) {
      Ddi_Bdd_t *f_j = Ddi_BddarrayRead(roots,j);
      bAigEdge_t fBaig = Ddi_BddToBaig(f_j);
      int i = bAig_AuxInt(bmgr,fBaig);
      if (bAig_NodeIsConstant(fBaig)) continue;
      Pdtutil_Assert(i>=0&&i<visitedNodes->num,"wrong array index");
      aigCnfInfo[i].isRoot=2;
      if (!doUseMapped) continue;
      if (bAig_isVarNode(bmgr,fBaig)) continue;
      if (aigCnfInfo[i].isMapped) {
	aigCnfInfo[i].isMapped++;
	continue;
      }
      aigCnfInfo[i].isMapped = 1;
      aigCnfInfo[i].mapLdr=bAig_NodeIsInverted(fBaig)?-(j+1):(j+1);
    }
  }

  Pdtutil_Assert(last<0 || aigCnfInfo[last].foCnt==0,"wrong fanout cnt");
  for (j=0; j<visitedNodes->num; j++) {
    bAigEdge_t baig = visitedNodes->nodes[j];
    if (!bAig_isVarNode(bmgr,baig) && !bAig_NodeIsConstant(baig)) {
      int ir, il, funcBlkFound;
      bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
      bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);

      Pdtutil_Assert(left!=bAig_NULL && right!=bAig_NULL,"NULL child");

      ir = bAig_AuxInt(bmgr,right);
      il = bAig_AuxInt(bmgr,left);
      if (enBlocks) {
	funcBlkFound = findIteOrXor(bmgr,visitedNodes,aigCnfInfo,
				  baig,right,left,j,ir,il);
	if (!funcBlkFound) {
	  if (!bAig_isVarNode(bmgr,right) && !bAig_NodeIsConstant(right)) {
	    if (aigCnfInfo[ir].foCnt==1 && !bAig_NodeIsInverted(right)) {
	      if (aigCnfInfo[ir].isRoot==1) aigCnfInfo[ir].isRoot=0;
	    }
	  }
	  if (!bAig_isVarNode(bmgr,left) && !bAig_NodeIsConstant(left)) {
	    if (aigCnfInfo[il].foCnt==1 && !bAig_NodeIsInverted(left)) {
	      if (aigCnfInfo[il].isRoot==1) aigCnfInfo[il].isRoot=0;
	    }
	  }
	}
      }
    }
  }
  Pdtutil_Assert(last<0||aigCnfInfo[last].isRoot,"wrong root info");
  if (enBlocks) {
  for (j=visitedNodes->num-1; j>=0; j--) {
    bAigEdge_t baig = visitedNodes->nodes[j];
    if (!bAig_isVarNode(bmgr,baig) && !bAig_NodeIsConstant(baig)) {
      Ddi_Clause_t *cl=NULL;
      int ir, il, ldr;
      bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
      bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);

      Pdtutil_Assert(left!=bAig_NULL && right!=bAig_NULL,"NULL child");

      ir = bAig_AuxInt(bmgr,right);
      il = bAig_AuxInt(bmgr,left);

      if (aigCnfInfo[j].isRoot>2) continue; /* skip xor/ITE */

      if (aigCnfInfo[j].isRoot) {
	int f = DdiAig2CnfId(bmgr,baig);
	Pdtutil_Assert(aigCnfInfo[j].ldr == j,"wrong leader");
	if (aigCnfInfo[j].ca==NULL) aigCnfInfo[j].ca = Ddi_ClauseArrayAlloc(1);
	cl = Ddi_ClauseAlloc(0,1);
	DdiClauseAddLiteral(cl,f);
	Ddi_ClauseArrayPush(aigCnfInfo[j].ca,cl);
      }
      if (!aigCnfInfo[ir].isRoot) {
	Pdtutil_Assert (aigCnfInfo[ir].foCnt==1,"wrong root info");
	ldr = aigCnfInfo[ir].ldr = aigCnfInfo[j].ldr;
      }
      if (!aigCnfInfo[il].isRoot) {
	Pdtutil_Assert (aigCnfInfo[il].foCnt==1,"wrong root info");
	ldr = aigCnfInfo[il].ldr = aigCnfInfo[j].ldr;
      }
    }
  }
  }

  return aigCnfInfo;
}

/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
findIteOrXor(
  bAig_Manager_t *bmgr,
  bAig_array_t *visitedNodes,
  Ddi_AigCnfInfo_t *aigCnfInfo,
  bAigEdge_t baig,
  bAigEdge_t right,
  bAigEdge_t left,
  int i,
  int ir,
  int il
)
{

  bAigEdge_t rr, rl, lr, ll, tmp;
  int irr, irl, ilr, ill;
  char isInvR, isInvL, isInvRr, isInvRl, isInvLr, isInvLl;
  int enIte = 1;

  if (bAig_isVarNode(bmgr,baig) || bAig_NodeIsConstant(baig)) {
    Pdtutil_Assert(0,"var of const node in func check");
  }

  if (bAig_isVarNode(bmgr,right) || bAig_NodeIsConstant(right)) return 0;
  if (bAig_isVarNode(bmgr,left) || bAig_NodeIsConstant(left)) return 0;
  if (!bAig_NodeIsInverted(right)) return 0;
  if (!bAig_NodeIsInverted(left)) return 0;

  rr=rl=lr=ll = bAig_NULL;
  isInvR = isInvL = isInvRr = isInvRl = isInvLr = isInvLl = 0;

  /* negation taken to account for OR = !AND(! ,! ) */
  rr = bAig_Not(bAig_NodeReadIndexOfRightChild(bmgr,right));
  rl = bAig_Not(bAig_NodeReadIndexOfLeftChild(bmgr,right));
  lr = bAig_Not(bAig_NodeReadIndexOfRightChild(bmgr,left));
  ll = bAig_Not(bAig_NodeReadIndexOfLeftChild(bmgr,left));

  if (bAig_NonInvertedEdge(rr) > bAig_NonInvertedEdge(rl)) {
    tmp = rr; rr = rl; rl = tmp;
  }
  if (bAig_NonInvertedEdge(lr) > bAig_NonInvertedEdge(ll)) {
    tmp = lr; lr = ll; ll = tmp;
  }

  isInvRr = bAig_NodeIsInverted(rr);
  isInvRl = bAig_NodeIsInverted(rl);
  isInvLr = bAig_NodeIsInverted(lr);
  isInvLl = bAig_NodeIsInverted(ll);
  irr = bAig_AuxInt(bmgr,rr);
  irl = bAig_AuxInt(bmgr,rl);
  ilr = bAig_AuxInt(bmgr,lr);
  ill = bAig_AuxInt(bmgr,ll);
  if (aigCnfInfo[irr].isRoot==0 || aigCnfInfo[irl].isRoot==0 ||
      aigCnfInfo[ilr].isRoot==0 || aigCnfInfo[ill].isRoot==0 ) return 0;
  if (aigCnfInfo[irr].isRoot==2 || aigCnfInfo[irl].isRoot==2 ||
      aigCnfInfo[ilr].isRoot==2 || aigCnfInfo[ill].isRoot==2 ) return 0;

#if 0
  if (rr == bAig_Not(lr) && rl == bAig_Not(ll)) {
    /* xnor (rr + rl) (!rr + !rl) */
    aigCnfInfo[i].isRoot = 3;
    aigCnfInfo[i].opIds[0] = ((isInvRr)?-1:1)*(irr+1);
    aigCnfInfo[i].opIds[1] = ((isInvRl)?-1:1)*(irl+1);
    if (aigCnfInfo[irr].isRoot<=1) aigCnfInfo[irr].isRoot=2;
    if (aigCnfInfo[irl].isRoot<=1) aigCnfInfo[irl].isRoot=2;
  }
    else
#endif
#if 1
    if (rr == bAig_Not(lr)) {
    /*  ite(rr,rl,ll) */
    if (enIte) {
    aigCnfInfo[i].isRoot = 4;
    aigCnfInfo[i].opIds[0] = ((isInvRr)?-1:1)*(irr+1);
    aigCnfInfo[i].opIds[2] = ((isInvRl)?-1:1)*(irl+1);
    aigCnfInfo[i].opIds[1] = ((isInvLl)?-1:1)*(ill+1);
    if (aigCnfInfo[irr].isRoot<=1) aigCnfInfo[irr].isRoot=2;
    if (aigCnfInfo[irl].isRoot<=1) aigCnfInfo[irl].isRoot=2;
    if (aigCnfInfo[ill].isRoot<=1) aigCnfInfo[ill].isRoot=2;
    }
  }
  else if (rr == bAig_Not(ll)) {
    /*  ite(rr,rl,lr) */
    aigCnfInfo[i].isRoot = 4;
    aigCnfInfo[i].opIds[0] = ((isInvRr)?-1:1)*(irr+1);
    aigCnfInfo[i].opIds[2] = ((isInvRl)?-1:1)*(irl+1);
    aigCnfInfo[i].opIds[1] = ((isInvLr)?-1:1)*(ilr+1);
    if (aigCnfInfo[irr].isRoot<=1) aigCnfInfo[irr].isRoot=2;
    if (aigCnfInfo[irl].isRoot<=1) aigCnfInfo[irl].isRoot=2;
    if (aigCnfInfo[ilr].isRoot<=1) aigCnfInfo[ilr].isRoot=2;
  }
  else if (rl == bAig_Not(lr)) {
    /*  ite(rl,rr,ll) */
    aigCnfInfo[i].isRoot = 4;
    aigCnfInfo[i].opIds[0] = ((isInvRl)?-1:1)*(irl+1);
    aigCnfInfo[i].opIds[2] = ((isInvRr)?-1:1)*(irr+1);
    aigCnfInfo[i].opIds[1] = ((isInvLl)?-1:1)*(ill+1);
    if (aigCnfInfo[irl].isRoot<=1) aigCnfInfo[irl].isRoot=2;
    if (aigCnfInfo[irr].isRoot<=1) aigCnfInfo[irr].isRoot=2;
    if (aigCnfInfo[ill].isRoot<=1) aigCnfInfo[ill].isRoot=2;
  }
  else if (rl == bAig_Not(ll)) {
    /*  ite(rl,rr,lr) */
    aigCnfInfo[i].isRoot = 4;
    aigCnfInfo[i].opIds[0] = ((isInvRl)?-1:1)*(irl+1);
    aigCnfInfo[i].opIds[2] = ((isInvRr)?-1:1)*(irr+1);
    aigCnfInfo[i].opIds[1] = ((isInvLr)?-1:1)*(ilr+1);
    if (aigCnfInfo[irr].isRoot<=1) aigCnfInfo[irr].isRoot=2;
    if (aigCnfInfo[irl].isRoot<=1) aigCnfInfo[irl].isRoot=2;
    if (aigCnfInfo[ilr].isRoot<=1) aigCnfInfo[ilr].isRoot=2;
  }
#endif
  if (aigCnfInfo[i].isRoot > 2) {
    if (aigCnfInfo[ir].isRoot != 2 && aigCnfInfo[ir].foCnt==1) {
      aigCnfInfo[ir].isRoot = 0;
      aigCnfInfo[ir].ldr = i;
    }
    if (aigCnfInfo[il].isRoot != 2 && aigCnfInfo[il].foCnt==1) {
      aigCnfInfo[il].isRoot = 0;
      aigCnfInfo[il].ldr = i;
    }
    iteNum++;
    return 1;
  }

  return 0;
}

#include "minisat22/coreDir/Solver.h"
#include "minisat22/simp/SimpSolver.h"
#include "minisat22/coreDir/PdtLink.h"

#define DdiSolverCheckVar22(solver,lit) \
{\
  while (abs(lit) >= (solver)->nVarsMax-1) {	\
    int i;\
    (solver)->nVarsMax *= 2;\
    (solver)->vars = Pdtutil_Realloc(int,(solver)->vars,(solver)->nVarsMax);\
    (solver)->freeVars = \
	Pdtutil_Realloc(int,(solver)->freeVars,(solver)->nVarsMax);\
    (solver)->map = Pdtutil_Realloc(int,(solver)->map,(solver)->nVarsMax);\
    (solver)->mapInv = \
       Pdtutil_Realloc(int,(solver)->mapInv,(solver)->nVarsMax);\
    for (i=(solver)->nVarsMax/2; i<(solver)->nVarsMax; i++) {\
      (solver)->map[i] = (solver)->mapInv[i] = -1;\
    }\
  }\
  if (abs(lit) >= (solver)->nVars) {		\
    /* printf("incrVars\n");	 */		\
    (solver)->nVars=abs(lit)+1;			\
  }\
  /* printf("chk: %d (v: %d, vm: %d)\n",lit, (solver)->nVars, (solver)->nVarsMax); */  \
  if ((solver)->map[abs(lit)]<0) {\
    Minisat22Solver *stmp = (Minisat22Solver *)((solver)->S22);\
    int v = stmp->nVars();		\
    stmp->newVar();                 \
    Pdtutil_Assert(stmp->nVars()<=(solver)->nVarsMax,	\
      "max num of solver vars exceeded");	\
    /* printf("mapping: %d -> %d\n", abs(lit), v); */	\
    (solver)->map[abs(lit)] = v;\
    (solver)->mapInv[v] = abs(lit);\
  }\
}
inline void DdiSolverSetVar22(Ddi_SatSolver_t *solver, int lit) {
  DdiSolverCheckVar22(solver,lit);
  (solver)->vars[abs(lit)] = 1; /* non free */
}
inline Minisat::Lit DdiLit2Minisat22(Ddi_SatSolver_t *s, int l) { 
  int ls = s->map[abs(l)];
  Pdtutil_Assert(ls>0,"not mapped literal");
  return ((l)<0 ? ~Minisat::mkLit(abs(ls)) : Minisat::mkLit(ls)); 
}
#define DdiSatClause2Minisat22(solver,minisatClause,cl)   \
{\
  int j;\
  minisatClause.clear();\
  for (j=0; j<(cl)->nLits; j++) {\
    int lit = (cl)->lits[j];\
    if (lit==0) continue; \
    /*   printf("lit: %d\n",lit); */		\
    DdiSolverSetVar22(solver,lit);\
    minisatClause.push(DdiLit2Minisat22(solver,lit));	\
  }\
}

/**Function********************************************************************
  Synopsis    [Create a SAT solver]
  Description [Create a SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_SatSolver_t *
Ddi_SatSolverAllocIntern(
  int useMinisat22
)
{
  Ddi_SatSolver_t *solver = Pdtutil_Alloc(Ddi_SatSolver_t,1);
  int i;

  solver->S = NULL;
  solver->S22 = NULL;
  if (!useMinisat22) {
    solver->S = new Solver();
    solver->S->minisat20_opt = true;
    solver->S->bin_clause_opt = true;
    solver->S->newVar(); // dummy
  }
  else {
    Minisat22Solver *stmp = new Minisat22Solver();
    solver->S22 = stmp;
    stmp->newVar(); // dummy
  }
  solver->nVarsMax = 1024;	//TODO: cambiare 1024
  solver->vars = Pdtutil_Alloc(int,solver->nVarsMax);
  solver->freeVars = Pdtutil_Alloc(int,solver->nVarsMax);
  solver->map = Pdtutil_Alloc(int,solver->nVarsMax);
  solver->mapInv = Pdtutil_Alloc(int,solver->nVarsMax);
  solver->nVars = solver->nFreeVars = 0;
  solver->nFreeTot = 0;
  solver->freeVarMinId = 0;
  solver->settings.nFreeMax = 300; //TODO: cambiare 300
  solver->cpuTime = 0;

  for (i=0; i<solver->nVarsMax; i++) {
    solver->map[i] = solver->mapInv[i] = -1;
  }


  solver->incrSat.doIncr = 0;
  solver->incrSat.clauses = NULL;
  solver->incrSat.mapping = NULL;

  return solver;
}

/**Function********************************************************************
  Synopsis    [Release SAT solver]
  Description [Release SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_SatSolverQuit(
  Ddi_SatSolver_t *solver
)
{
  if (solver->S !=NULL)
    delete solver->S;
  if (solver->S22 !=NULL)
    delete (Minisat22Solver *)(solver->S22);
  Pdtutil_Free(solver->vars);
  Pdtutil_Free(solver->freeVars);
  Pdtutil_Free(solver->map);
  Pdtutil_Free(solver->mapInv);
  Pdtutil_Free(solver);
}

/**Function********************************************************************
  Synopsis    [Load clauses into a SAT solver]
  Description [Load clauses into a SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_SatSolverAddClauses(
  Ddi_SatSolver_t *solver,
  Ddi_ClauseArray_t *ca
)
{
  int i, j;
  if (ca==NULL) return;
  if (solver->S!=NULL) {
    for (i=0; i<ca->nClauses; i++) {
      Ddi_Clause_t *cl = ca->clauses[i];
      vec<Lit> minisatClause;
      DdiSatClause2Minisat(solver,minisatClause,cl);
      solver->S->addClause(minisatClause);
    }
  }
  else {
    Pdtutil_Assert(solver->S22!=NULL,"missing 22 solver");
    for (i=0; i<ca->nClauses; i++) {
      Ddi_Clause_t *cl = ca->clauses[i];
      Minisat::vec<Minisat::Lit> minisatClause;
      DdiSatClause2Minisat22(solver,minisatClause,cl);
      Minisat22Solver *stmp = (Minisat22Solver *)((solver)->S22);
      stmp->addClause(minisatClause);
    }
  }

#if 0
  if (chkClause!=NULL && !Ddi_SatSolveCustomized(solver, chkClause, 0, 0, -1)) {
    printf("preso\n");
  }
#endif

}

/**Function********************************************************************
  Synopsis    [Call SAT solver]
  Description [Call SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_SatSolve(
  Ddi_SatSolver_t *solver,
  Ddi_Clause_t *assume,
  int timeLimit
)
{
  int ret;
  long startTime, currTime;

  if (solver->S!=NULL) {

    vec<Lit> assumeClause;

    if (assume!=NULL) {
      DdiSatClause2Minisat(solver,assumeClause,assume);
    }

    if (solver->S->okay()==false) return 0;
    Pdtutil_Assert (solver->S->okay()==true, "solver is not OK");

    startTime = util_cpu_time();
    ret = solver->S->solve(assumeClause,timeLimit);
    if (solver->S->undefined()) ret = -1;
  }
  else {
    Pdtutil_Assert(solver->S22!=NULL,"missing 22 solver");
    Minisat::vec<Minisat::Lit> assumeClause;

    if (assume!=NULL) {
      DdiSatClause2Minisat22(solver,assumeClause,assume);
    }

    Minisat22Solver *stmp = (Minisat22Solver *)((solver)->S22);
    if (stmp->okay()==false) return 0;
    Pdtutil_Assert (stmp->okay()==true, "solver is not OK");

    startTime = util_cpu_time();
    if (timeLimit>0) {
      stmp->setTimeBudget((double)timeLimit);
      ret = stmp->solveLimitedInt(assumeClause);
    }
    else {
      ret = stmp->solve(assumeClause);
    }
  }
  currTime = util_cpu_time();
  solver->cpuTime += ((float)(currTime-startTime))/1000;

  return ret;
}

/**Function********************************************************************
  Synopsis    [Call SAT solver]
  Description [Call SAT solver]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Clause_t *
Ddi_SatSolverFinal(
  Ddi_SatSolver_t *solver,
  int maxVar
)
{
  int j;
  Ddi_Clause_t *cl = Ddi_ClauseAlloc(0,1);

  if (solver->S!=NULL) {

    for (j=0; j<solver->S->conflict.size(); j++) {
      Lit jLit = solver->S->conflict[j];
      int vs = var(jLit);
      Pdtutil_Assert(vs>0&&vs<solver->nVarsMax,"invalid solver var");
      int v = solver->mapInv[vs];
      Pdtutil_Assert(v>0&&v<solver->nVarsMax,"invalid var");
      if (maxVar < 0 || v <= maxVar) {
        // Pdtutil_Assert(DdiPdrVarIsNs(pdrMgr,v),"ns var needed in conflict");
        /* conflict has complemented var w.r.t. assump !*/
        /* result is negated w.r.t. assump !!! */
        int lit = sign(jLit) ? -v : v;
        //      int lit = sign(jLit) ? v : -v;
        DdiClauseAddLiteral(cl,lit);
      }
    }
  }
  else {
    Pdtutil_Assert(solver->S22!=NULL,"missing 22 solver");
    Minisat22Solver *stmp = (Minisat22Solver *)((solver)->S22);
    for (j=0; j<stmp->conflict.size(); j++) {
      Minisat::Lit jLit = stmp->conflict[j];
      int vs = Minisat::var(jLit);
      Pdtutil_Assert(vs>0&&vs<solver->nVarsMax,"invalid solver var");
      int v = solver->mapInv[vs];
      Pdtutil_Assert(v>0&&v<solver->nVarsMax,"invalid var");
      if (maxVar < 0 || v <= maxVar) {
        // Pdtutil_Assert(DdiPdrVarIsNs(pdrMgr,v),"ns var needed in conflict");
        /* conflict has complemented var w.r.t. assump !*/
        /* result is negated w.r.t. assump !!! */
        int lit = Minisat::sign(jLit) ? -v : v;
        //      int lit = sign(jLit) ? v : -v;
        DdiClauseAddLiteral(cl,lit);
      }
    }
  }

  //  Pdtutil_Assert(cl->nLits>0,"void final conflict");
  if (cl->nLits>0) {
    Ddi_ClauseSort(cl);
  }

  return cl;
}


/**Function********************************************************************
  Synopsis    [Create an incremental SAT mgr]
  Description [Create an incremental SAT mgr]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_IncrSatMgr_t *
Ddi_IncrSatMgrAlloc(
  Ddi_Mgr_t *ddm,
  int Minisat22orLgl,
  int enableProofLogging,
  int enableSimplify
)
{
  Ddi_IncrSatMgr_t *mgr = Pdtutil_Alloc(Ddi_IncrSatMgr_t,1);
  int i;

  mgr->S = new Solver();
  mgr->S22 = NULL;
  mgr->S22aux = NULL;
  mgr->S22simp = NULL;
  mgr->lgl = NULL;

  mgr->simpSolver = 0;
  
  if (Minisat22orLgl) {
    if (enableProofLogging) {
      Minisat22Solver *stmp = new Minisat22Solver();
      stmp->proofLogging(enableProofLogging);
      mgr->S22 = (void *) stmp;
    }
    else if (Minisat22orLgl<0) {
      mgr->lgl = lglminit (0,lglNew,lglRsz,lglDel);
      lglsetout (mgr->lgl, stdout);
    }
    else {
      Minisat22Solver *stmp = new Minisat22Solver();
      stmp->proofLogging(enableProofLogging);
      mgr->S22 = (void *) stmp;
      //      mgr->simpSolver = 1;
    }
  }
  else {
    if (enableProofLogging) {
      mgr->S->proof = new Proof();
      mgr->S->minisat20_opt = true;
    }
  }
  mgr->ddiS = NULL;
  mgr->aigCnfMgr = NULL;
  mgr->ddiMgr = ddm;
  mgr->cnfMappedVars = NULL;
  mgr->enProofLog = enableProofLogging;
  mgr->enSimplify = enableSimplify;
  mgr->dummyRefs = NULL;
  mgr->implied = NULL;
  mgr->refA = NULL;
  mgr->eqA = NULL;
  if (ddm!=NULL) {
    mgr->dummyRefs = Ddi_BddarrayAlloc(ddm,0);
  }
  mgr->suspended = 0;
  mgr->lglMaxFrozen = 0;
  mgr->enLglLearn = 0;
  mgr->lglLearned123 = NULL;
  
  return mgr;
}

/**Function********************************************************************
  Synopsis    [Create an incremental SAT mgr]
  Description [Create an incremental SAT mgr]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_IncrSatMgrLockAig(
  Ddi_IncrSatMgr_t *mgr,
  Ddi_Bdd_t *f
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  if (mgr->dummyRefs==NULL) {
    mgr->dummyRefs = Ddi_BddarrayAlloc(ddm,0);
  }
  if (Ddi_BddIsPartConj(f)) {
    int i;
    for (i=0; i<Ddi_BddPartNum(f); i++) {
      Ddi_Bdd_t *f_i = Ddi_BddPartRead(f,i);
      Ddi_BddarrayInsertLast(mgr->dummyRefs,f);
    }
  }
  else {
    Ddi_BddarrayInsertLast(mgr->dummyRefs,f);
  }
}


/**Function********************************************************************
  Synopsis    [Create an incremental SAT mgr]
  Description [Create an incremental SAT mgr]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_IncrSatMgrRestart(
  Ddi_IncrSatMgr_t *mgr
)
{
  if (mgr->S!=NULL) {
    delete mgr->S;
    mgr->S = new Solver();
  }
  if (mgr->S22!=NULL) {
    delete (Minisat22Solver *)(mgr->S22);
    Minisat22Solver *stmp = new Minisat22Solver();
    stmp->proofLogging(mgr->enProofLog);
    mgr->S22 = (void *) stmp;
  }
  if (mgr->ddiS!=NULL) {
    Ddi_SatSolverQuit(mgr->ddiS);
    mgr->ddiS = Ddi_SatSolverAlloc();
  }
  if (mgr->lgl!=NULL) {
    lglrelease (mgr->lgl);
    mgr->lgl = lglminit (0,lglNew,lglRsz,lglDel);
    lglsetout (mgr->lgl, stdout);
  }
  if (mgr->aigCnfMgr!=NULL) {
    DdiMinisatAigCnfMgrFree(mgr->aigCnfMgr);
    mgr->aigCnfMgr = NULL;
  }
  if (mgr->cnfMappedVars!=NULL) {
    int i;
    for (i=0; i<mgr->cnfMappedVars->num; i++) {
      bAigEdge_t baig = mgr->cnfMappedVars->nodes[i];
      if (baig != bAig_NULL) {
	bAig_RecursiveDeref(mgr->ddiMgr->aig.mgr,baig);
      }
    }
    bAigArrayFree(mgr->cnfMappedVars);
    mgr->cnfMappedVars = NULL;
  }
}

/**Function********************************************************************
  Synopsis    [Release incr SAT mgr]
  Description [Release incr SAT mgr]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_IncrSatMgrQuitIntern(
  Ddi_IncrSatMgr_t *mgr,
  int freeDdi
)
{
  if (mgr==NULL) return;

  if (mgr->S!=NULL) {
    if (mgr->S->proof!=NULL) 
      mgr->S->deleteProofTemps();
    delete mgr->S;
  }
  if (mgr->S22!=NULL) {
    delete (Minisat22Solver *)(mgr->S22);
  }
  if (mgr->S22aux!=NULL) {
    delete (Minisat22SolverPdt *)(mgr->S22aux);
  }
  if (mgr->S22simp!=NULL) {
    delete (Minisat::SimpSolver *)(mgr->S22simp);
  }
  if (mgr->ddiS!=NULL) {
    Ddi_SatSolverQuit(mgr->ddiS);
  }
  if (mgr->aigCnfMgr!=NULL) {
    DdiMinisatAigCnfMgrFree(mgr->aigCnfMgr);
  }
  if (mgr->cnfMappedVars!=NULL) {
    int i;
    for (i=0; i<mgr->cnfMappedVars->num; i++) {
      bAigEdge_t baig = mgr->cnfMappedVars->nodes[i];
      if (baig != bAig_NULL) {
	bAig_RecursiveDeref(mgr->ddiMgr->aig.mgr,baig);
      }
    }
    bAigArrayFree(mgr->cnfMappedVars);
  }

  Ddi_Free(mgr->dummyRefs);
  Ddi_Free(mgr->implied);
  Ddi_Free(mgr->eqA);
  Ddi_Free(mgr->refA);

  if (mgr->ddiMgr!=NULL) {
    Ddi_Free(mgr->dummyRefs);
    if (freeDdi) {
      Ddi_MgrQuit(mgr->ddiMgr);
    }
  }
  
  if (mgr->lglLearned123!=NULL) {
    delete mgr->lglLearned123;
  }

  Pdtutil_Free(mgr);
}

/**Function********************************************************************
  Synopsis    [Release incr SAT mgr]
  Description [Release incr SAT mgr]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_IncrSatMgrQuit(
  Ddi_IncrSatMgr_t *mgr
)
{
  Ddi_IncrSatMgrQuitIntern(mgr,1);
}

/**Function********************************************************************
  Synopsis    [Release incr SAT mgr]
  Description [Release incr SAT mgr]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_IncrSatMgrQuitKeepDdi(
  Ddi_IncrSatMgr_t *mgr
)
{
  Ddi_IncrSatMgrQuitIntern(mgr,0);
}


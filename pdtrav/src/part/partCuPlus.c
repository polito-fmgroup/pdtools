/**CFile***********************************************************************

  FileName    [partCuPlus.c]

  PackageName [part]

  Synopsis    [Recursive Functions for Disjunctive Partitioning.
    This functions rely on the ddi package.]

  Description [External procedures included in this file:
    <ul>
    <li> Part_EstimateCofactor
    <li> Part_EstimateCofactorComplex
    <li> Part_EstimateCofactorFast
    <li> Part_EstimateCofactorFreeOrder
    </ul>
    Static procedures included in this module:
    <ul>
    <li> partEstimateCofactor
    <li> partEstimateCofactorComplex
    <li> partEstimateCofactorFast
    <li> partEstimateCofactorFreeOrder
    <li> ddFlagClear 
    </ul>]

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

#include "partInt.h"                                          

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

static int partEstimateCofactor(Ddi_CuddNode *f, Ddi_Bdd_t *topVar);
static struct fs_struct partEstimateCofactorComplex(Ddi_CuddNode *f, Ddi_Bdd_t *topVar);
static int partEstimateCofactorFast(Ddi_CuddNode *f, int LeftRight, Ddi_Varset_t *supp, int suppSize, int *totalArray, int *thenArray, int *elseArray);
static int partEstimateCofactorFreeOrder(Ddi_CuddNode *f, Ddi_Bdd_t *topVar, Ddi_Varset_t *supp, int *vet);
static void ddFlagClear(Ddi_CuddNode *f);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Counts the number of nodes of the cofactors: 
    Stefano Quer solution.]

  Description []

  SideEffects [None]

  SeeAlso     [partEstimateCofactor]

******************************************************************************/

int
Part_EstimateCofactor (
  Ddi_Bdd_t *f        /* Input BDD */,
  Ddi_Bdd_t *topVar   /* Top Variable */
  )
{
  int i;	

  i = partEstimateCofactor (Ddi_BddToCU (f), topVar);
  ddFlagClear (Ddi_BddToCU (f));
  
  return(i);
} 


/**Function********************************************************************

  Synopsis    [Counts the number of nodes of the cofactors:
    Fabio Somenzi Private Commnunication Solution.]

  Description []

  SideEffects [none]

  SeeAlso     [partEstimateCofactor]

******************************************************************************/

int
Part_EstimateCofactorComplex (
  Ddi_Bdd_t *f         /* Input BDD */,
  Ddi_Bdd_t *topVar    /* Top Variable */
  )
{
  struct fs_struct s;	
  
  s = partEstimateCofactorComplex (Ddi_BddToCU (f), topVar);
  ddFlagClear (Ddi_BddToCU (f));
  
  return (s.n);
} 

/**Function********************************************************************

  Synopsis    [Count the number of nodes of the cofactors using the fast
    heuristic (TCAD'99)]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int
Part_EstimateCofactorFast (
  Ddi_Bdd_t *f,
  int LeftRight,
  Ddi_Varset_t *supp,
  int suppSize,
  int *totalArray,
  int *thenArray,
  int *elseArray
  )
{
  int i, n;
  Ddi_Bddarray_t *roots;
  DdNode **rootsCU;

  roots = Ddi_BddarrayMakeFromBddRoots(f);
  rootsCU = Ddi_BddarrayToCU (roots);

  n = 0;
  for (i=0; rootsCU[i] != NULL; i++) {
    Pdtutil_Assert(i<Ddi_BddarrayNum(roots),"Invalid root access");
    n += partEstimateCofactorFast (rootsCU[i], LeftRight, supp,
      suppSize, totalArray, thenArray, elseArray);
  }
  Pdtutil_Assert(i==Ddi_BddarrayNum(roots),"Invalid root num");

  for (i=0; rootsCU[i] != NULL; i++) {
    ddFlagClear (rootsCU[i]);
  }

  Pdtutil_Free(rootsCU);
  Ddi_Free(roots);

  return (n);
}

/**Function********************************************************************

  Synopsis    [Count the number of nodes of the cofactors using the Free
    Order heuristic (TCAD'99).]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int
Part_EstimateCofactorFreeOrder (
  Ddi_Bdd_t *f          /* Input BDD */,
  Ddi_Bdd_t *topVar     /* Top Variable */,
  Ddi_Varset_t *supp,
  int *vet
  )
{
  int i;	

  i = partEstimateCofactorFreeOrder (Ddi_BddToCU (f), topVar, supp, vet);
  ddFlagClear (Ddi_BddToCU (f));
  
  return(i);
} 

/**Function********************************************************************

  Synopsis    [Performs the recursive step of Part_EstimateCofactor]

  Description []

  SideEffects [None]

  SeeAlso     [Part_EstimateCofactor]

******************************************************************************/

static int
partEstimateCofactor (
  Ddi_CuddNode *f     /* Input BDD */,
  Ddi_Bdd_t *topVar  /* Top Variable */
  )
{
  int thenVal, elseVal, val;

  if (Ddi_CuddNodeIsVisited (f)) {
    return (0);
  }

  if (Ddi_CuddNodeIsConstant (f)) {
    return (0);
  }

  Ddi_CuddNodeSetVisited(f);
  
  if (Ddi_CuddNodeReadIndex (f) == Ddi_CuddNodeReadIndex
    (Ddi_BddToCU (topVar))) {
    if (Ddi_BddIsComplement(topVar)) {
      val = partEstimateCofactor (Ddi_CuddNodeElse (f), topVar);
    } else {
      val = partEstimateCofactor (Ddi_CuddNodeThen (f), topVar);
    }
  } else {
    thenVal = partEstimateCofactor (Ddi_CuddNodeThen (f), topVar);
    elseVal = partEstimateCofactor (Ddi_CuddNodeElse (f), topVar);
    val = thenVal + elseVal +1;
  }
  
  return (val);
} 


/**Function********************************************************************

  Synopsis    [Performs the recursive step of Part_EstimateCofactor]

  Description []

  SideEffects [none]

  SeeAlso     [Part_EstimateCofactor]

******************************************************************************/

static struct fs_struct 
partEstimateCofactorComplex (
  Ddi_CuddNode *f     /* Input BDD */,
  Ddi_Bdd_t *topVar  /* Top Variable */
  )
{
  struct fs_struct s0, s, st, se;
  
  s0.p = f;
  s0.n = 0;
  
  if (Ddi_CuddNodeIsVisited (f)) {
    return(s0);
  }
  if (Ddi_CuddNodeIsConstant (f)) {
    return (s0);
  }
  Ddi_CuddNodeSetVisited(f);
  
  if (Ddi_CuddNodeReadIndex (f) ==
    Ddi_CuddNodeReadIndex (Ddi_BddToCU (topVar))) {
    if (Ddi_BddIsComplement(topVar)) {
      s = partEstimateCofactorComplex (Ddi_CuddNodeElse (f), topVar);
    } else {
      s = partEstimateCofactorComplex (Ddi_CuddNodeThen (f), topVar);
    }
  } else {
    st = partEstimateCofactorComplex (Ddi_CuddNodeThen (f), topVar);
    se = partEstimateCofactorComplex (Ddi_CuddNodeElse (f), topVar);
    if (st.p == se.p) {
      s = st;
    } else {
      s.p = f;
      s.n = st.n + se.n + 1;
    }
  }
  
  return (s);
}

/**Function********************************************************************

  Synopsis    [Performs the recursive step of fsCntNodesVar]

  Description []

  SideEffects [none]

  SeeAlso     [fsCntNodesVar]

******************************************************************************/

static int
partEstimateCofactorFast (
  Ddi_CuddNode *f       /* input BDD */,
  int LeftRight,
  Ddi_Varset_t *supp,
  int suppSize,
  int *totalArray,
  int *thenArray,
  int *elseArray
  )
{
  int i, totalVal, thenVal, elseVal;
  Ddi_Mgr_t *dd = Ddi_ReadMgr (supp);

  if (Ddi_CuddNodeIsVisited (f)) {
    return (0);
  }

  if (Ddi_CuddNodeIsConstant (f)) {
    return (0);
  }

  Ddi_CuddNodeSetVisited(f);

#if 0
  i=0;
  scan = Ddi_VarsetDup (supp);
  var = Ddi_VarsetTop (scan);
  while (Ddi_CuddNodeReadIndex (f) != Ddi_VarCuddIndex (var)) {
    Pdtutil_Assert (!Ddi_VarsetIsVoid (scan), "Support Empty!");
    tmp = Ddi_VarsetNext (scan);
    Ddi_Free (scan);
    scan = tmp;
    var = Ddi_VarsetTop (scan);
    i++;
  }
#else 
  i = Ddi_MgrReadPerm(dd, Ddi_CuddNodeReadIndex (f));
#endif

  if (LeftRight==1) {
    elseVal = partEstimateCofactorFast (Ddi_CuddNodeElse (f),
      LeftRight, supp, suppSize, totalArray, thenArray, elseArray);
    thenVal = partEstimateCofactorFast (Ddi_CuddNodeThen (f), LeftRight,
      supp, suppSize, totalArray, thenArray, elseArray);
  } else {
    thenVal = partEstimateCofactorFast (Ddi_CuddNodeThen (f), LeftRight,
      supp, suppSize, totalArray, thenArray, elseArray);
    elseVal = partEstimateCofactorFast (Ddi_CuddNodeElse (f),
      LeftRight, supp, suppSize, totalArray, thenArray, elseArray);
  }

  totalVal = thenVal + elseVal + 1;
  totalArray[i] += 1;
  thenArray[i] += thenVal;
  elseArray[i] += elseVal;

  return (totalVal);
}

/**Function********************************************************************

  Synopsis    [Performs the recursive step of fsCntNodesVar]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

static int
partEstimateCofactorFreeOrder (
  Ddi_CuddNode *f            /* Input BDD */,
  Ddi_Bdd_t *topVar         /* Top Variable */,
  Ddi_Varset_t *supp,
  int *vet
  )
{
  int i, thenVal, elseVal, val;
  Ddi_Varset_t *scan, *tmp;
  Ddi_Var_t *var;

  if (Ddi_CuddNodeIsVisited (f)) {
    return(0);
  }

  if (Ddi_CuddNodeIsConstant (f)) {
    return(0);
  }

  Ddi_CuddNodeSetVisited (f);

  i=0;
  scan = Ddi_VarsetDup (supp);
  var = Ddi_VarsetTop (scan);
  while (Ddi_CuddNodeReadIndex (f) != Ddi_VarCuddIndex (var)) {
    Pdtutil_Assert (!Ddi_VarsetIsVoid (scan), "Support Empty!");
    tmp = Ddi_VarsetNext (scan);
    Ddi_Free (scan);
    scan = tmp;
    var = Ddi_VarsetTop (scan);
    i++;
  }
  vet[i]++;

  if (Ddi_CuddNodeReadIndex (f) == Ddi_CuddNodeReadIndex (Ddi_BddToCU
    (topVar))) {
    if (Ddi_BddIsComplement (topVar)) {
      val = partEstimateCofactorFreeOrder (Ddi_CuddNodeElse (f),
        topVar, supp, vet);
    } else {
      val = partEstimateCofactorFreeOrder (Ddi_CuddNodeThen (f),
        topVar, supp, vet);
    }
  } else {
    thenVal = partEstimateCofactorFreeOrder (Ddi_CuddNodeThen (f),
      topVar, supp, vet);
    elseVal = partEstimateCofactorFreeOrder (Ddi_CuddNodeElse (f),
      topVar, supp, vet);
    val = thenVal + elseVal + 1;
  }

  return (val);
}

/**Function********************************************************************

  Synopsis    [Performs a DFS from f, clearing the LSB of the next
    pointers.]

  Description [Static function copied from cudd_Util.c]

  SideEffects [None]

  SeeAlso     [Part_EstimateCofactor, Part_EstimateCofactor]

******************************************************************************/

static void
ddFlagClear (
  Ddi_CuddNode *f                     /* input BDD */
  )
{
  if (!Ddi_CuddNodeIsVisited (f)) { 
    return;
  }
  
  /* Clear visited flag. */
  Ddi_CuddNodeClearVisited (f);
  if (Ddi_CuddNodeIsConstant (f)) { 
    return;
  }
  ddFlagClear (Ddi_CuddNodeThen (f));
  ddFlagClear (Ddi_CuddNodeElse (f));

  return;
} 

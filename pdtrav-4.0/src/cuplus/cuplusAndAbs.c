/**CFile***********************************************************************

  FileName    [cuplusAndAbs.c]

  PackageName [cuplus]

  Synopsis    [Optimized AndAbstract.]

  Description []

  SeeAlso     []

  Author      [Gianpiero Cabodi]

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

#include "cuplusInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define DD_BDD_AND_ABS_PROJECT_TAG              0x6e
#define DD_BDD_AND_ABSTRACT_TAG2              0x6f
#define QUANTIFY_BEFORE_RECUR                   0

#define CHECK_SIZE_AND   1
#define CHECK_SIZE_EX   1
#define CHECK_MIN_SIZE   50000

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/**Enum************************************************************************

  Synopsis    [Type for selecting pruning heuristics.]

  Description []

******************************************************************************/

typedef enum
  {
    ProjectQuantify_c,
    ProjectKeep_c,
    ProjectQuantifyTree_c,
    ProjectKeepTree_c,
    ProjectZero_c,
    ProjectCanceled_c
  }
Cuplus_ProjectNodeCode_e;

typedef struct project_info_item {
  Cuplus_ProjectNodeCode_e code;
  DdNode *projected_bdd;
} project_info_item_t;

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/


long int aaa=0;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static DdNode * cuplusBddAndAbstractProjectRecur(DdManager *manager, DdNode *f, DdNode *g, DdNode *cube, st_table *table, DdNode *projectDummyPtr);
static DdNode * cuplusBddProjectRecur(DdManager *manager, DdNode *f, st_table *table);
static project_info_item_t * hashSet(st_table *table, DdNode *f, Cuplus_ProjectNodeCode_e code);
static enum st_retval ProjectInfoItemFree(char * key, char * value, char * arg);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis [Generate cuplus and abstract info structure.]

  Description [Generate cuplus and abstract info structure. the
  structure collects global variables to be used for special purpose
  and exist.]

  SideEffects [None]

  SeeAlso     [Cudd_bddAndAbstract]

******************************************************************************/
cuplus_and_abs_info_t *
Cuplus_bddAndAbstractInfoInit()
{
  cuplus_and_abs_info_t *infoPtr = 
    Pdtutil_Alloc(cuplus_and_abs_info_t,1);

#if DEVEL_OPT_ANDEXIST
  Cuplus_AuxVarset = NULL;
#endif

  infoPtr->Cuplus_EnableAndAbs = 0;

  infoPtr->newvarSizeIncr=0;
  infoPtr->newvarSizeRepl=0;
  infoPtr->keepvarSizeIncr=0;
  infoPtr->newvarSizeNum=0;
  infoPtr->keepvarSizeNum=0;

  infoPtr->elseFirst = NULL;
  infoPtr->elseFirstSize = 0;
  infoPtr->initElseFirst = 0;

  return infoPtr;

}

/**Function********************************************************************

  Synopsis [Free cuplus and abstract info structure.]

  Description [Free cuplus and abstract info structure. the
  structure collects global variables to be used for special purpose
  and exist.]

  SideEffects [None]

  SeeAlso     [Cudd_bddAndAbstract]

******************************************************************************/
void
Cuplus_bddAndAbstractInfoFree(
  cuplus_and_abs_info_t *infoPtr
)
{
  if (infoPtr->elseFirst != NULL) {
    Pdtutil_Free(infoPtr->elseFirst);
  }
#if DEVEL_OPT_ANDEXIST
  Ddi_Free(infoPtr->Cuplus_AuxVarset);
#endif
  Pdtutil_Free(infoPtr);
}

/**Function********************************************************************

  Synopsis [Takes the AND of two BDDs and simultaneously abstracts the
  variables in cube.]

  Description [Takes the AND of two BDDs and simultaneously abstracts
  the variables in cube. The variables are existentially abstracted.
  Returns a pointer to the result is successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_bddAndAbstract]

******************************************************************************/
DdNode *
Cuplus_bddAndAbstract(
  DdManager * manager,
  DdNode * f,
  DdNode * g,
  DdNode * cube,
  int *abortFrontier,
  cuplus_and_abs_info_t *infoPtr
)
{
    DdNode *res;
    int i;
    if (1&&abortFrontier != NULL) {
      do {
	manager->reordered = 0;
        if (infoPtr->initElseFirst) {
          infoPtr->initElseFirst = 0;
          if (infoPtr->elseFirst == NULL) {
	    infoPtr->elseFirstSize = Cudd_ReadSize(manager);
            infoPtr->elseFirst = Pdtutil_Alloc(int,Cudd_ReadSize(manager));
	  }
          if (infoPtr->elseFirstSize < Cudd_ReadSize(manager)) {
	    infoPtr->elseFirstSize = Cudd_ReadSize(manager);
            infoPtr->elseFirst = 
	      Pdtutil_Realloc(int,infoPtr->elseFirst,infoPtr->elseFirstSize);
	  }
          for (i=0; i<Cudd_ReadSize(manager); i++) {
            infoPtr->elseFirst[i] = (rand()%2);
          }
	}
	res = cuplusBddAndAbstractRecur2(manager, f, g, cube,
					 abortFrontier, infoPtr->elseFirst);
      } while (manager->reordered == 1);
    }
    else if (!infoPtr->Cuplus_EnableAndAbs)
      return Cudd_bddAndAbstract(manager,f,g,cube);
    else
      do {
	manager->reordered = 0;
	res = cuplusBddAndAbstractRecur(manager, f, g, cube);
      } while (manager->reordered == 1);
    return(res);

} /* end of Cuplus_bddAndAbstract */



/**Function********************************************************************

  Synopsis [Takes the AND of two BDDs and simultaneously abstracts the
  variables in cube.]

  Description [Takes the AND of two BDDs and simultaneously abstracts
  the variables in cube. The variables are existentially abstracted.
  Returns a pointer to the result is successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_bddAndAbstract]

******************************************************************************/
DdNode *
Cuplus_bddAndAbstractProject(
  DdManager * manager,
  DdNode * f,
  DdNode ** g,
  int       ng,
  DdNode * cube)
{
  DdNode *res;
  st_table *table;
  int i;
  DdNode *projectDummyPtr;

  manager->reordered = 0;

  projectDummyPtr = Pdtutil_Alloc(DdNode,1);
  projectDummyPtr->ref = 1;

  table = st_init_table (st_ptrcmp, st_ptrhash);
  if (table == NULL) {
    Pdtutil_Warning (1, "Out-of-memory, couldn't alloc project table.");
    return (NULL);
  }

  for (i=0; i<ng; i++) {
    if (cuplusBddAndAbstractProjectRecur(manager,f,g[i],cube,table,
      projectDummyPtr)==NULL) {
      return(NULL);
    }
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, ".");
    );
  }

  res = cuplusBddProjectRecur(manager, f, table);
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "!");
  );
  cuddRef(res);

  st_foreach (table, ProjectInfoItemFree, (char *)manager);
  st_free_table (table);

  assert (manager->reordered == 0);

  cuddDeref(res);

  return(res);

} /* end of Cuplus_bddAndAbstractProject */



/**Function********************************************************************

  Synopsis [Existentially abstracts all the variables in cube from f.]

  Description [Existentially abstracts all the variables in cube from f.
  Returns the abstracted BDD if successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_bddUnivAbstract Cudd_addExistAbstract]

******************************************************************************/
DdNode *
Cuplus_bddExistAbstractWeak(
  DdManager * manager,
  DdNode * f,
  DdNode * cube)
{
    DdNode *res;

    do {
      manager->reordered = 0;
      res = cuplusBddExistAbstractWeakRecur(manager, f, cube);
    } while (manager->reordered == 1);

    return(res);

} /* end of Cudd_bddExistAbstract */


/**Function********************************************************************

  Synopsis [Existentially abstracts all the variables in cube from f.]

  Description [Existentially abstracts all the variables in cube from f.
  Returns the abstracted BDD if successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_bddUnivAbstract Cudd_addExistAbstract]

******************************************************************************/
DdNode *
Cuplus_bddExistAbstractAux(
  DdManager * manager,
  DdNode * f,
  DdNode * cube)
{
    DdNode *res;

    do {
      manager->reordered = 0;
      res = cuplusBddExistAbstractAuxRecur(manager, f, cube);
    } while (manager->reordered == 1);

    return(res);

} /* end of Cudd_bddExistAbstract */


/**Function********************************************************************

  Synopsis [Existentially abstracts all the variables in cube from f.]

  Description [Existentially abstracts all the variables in cube from f.
  Returns the abstracted BDD if successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_bddUnivAbstract Cudd_addExistAbstract]

******************************************************************************/
DdNode *
Cuplus_bddExistAbstract(
  DdManager * manager,
  DdNode * f,
  DdNode * cube)
{
    DdNode *res;

    do {
      manager->reordered = 0;
      res = cuplusBddExistAbstractRecur(manager, f, cube);
    } while (manager->reordered == 1);

    return(res);

} /* end of Cudd_bddExistAbstract */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Performs the recursive steps of Cudd_bddExistAbstract.]

  Description [Performs the recursive steps of Cudd_bddExistAbstract.
  Returns the BDD obtained by abstracting the variables
  of cube from f if successful; NULL otherwise. It is also used by
  Cudd_bddUnivAbstract.]

  SideEffects [None]

  SeeAlso     [Cudd_bddExistAbstract Cudd_bddUnivAbstract]

******************************************************************************/
DdNode *
cuplusBddExistAbstractRecur(
  DdManager * manager,
  DdNode * f,
  DdNode * cube
)
{
    DdNode      *F, *T, *E, *res, *res1, *res2, *one;

    statLine(manager);
    one = DD_ONE(manager);
    F = Cudd_Regular(f);

    /* Cube is guaranteed to be a cube at this point. */
    if (cube == one || F == one) {
        return(f);
    }
    /* From now on, f and cube are non-constant. */

    /* Abstract a variable that does not appear in f. */
    while (manager->perm[F->index] > manager->perm[cube->index]) {
      cube = cuddT(cube);
      if (cube == one) return(f);
    }

    /* Check the cache. */
    if (F->ref != 1 && (res = cuddCacheLookup2(manager,
        cuplusBddExistAbstractRecur, f, cube)) != NULL) {
      return(res);
    }

    /* Compute the cofactors of f. */
    T = cuddT(F); E = cuddE(F);
    if (f != F) {
      T = Cudd_Not(T); E = Cudd_Not(E);
    }

    /* If the two indices are the same, so are their levels. */
    if (F->index == cube->index) {
        int keys0;
      if (T == one || E == one || T == Cudd_Not(E)) {
          return(one);
      }
      res1 = cuplusBddExistAbstractRecur(manager, T, cuddT(cube));
      if (res1 == NULL) {
          return(NULL);
      }
      if (res1 == one) {
        if (F->ref != 1)
          cuddCacheInsert2(manager,
              cuplusBddExistAbstractRecur, f, cube, one);
          return(one);
      }
        cuddRef(res1);
      res2 = cuplusBddExistAbstractRecur(manager, E, cuddT(cube));
      if (res2 == NULL) {
        Cudd_IterDerefBdd(manager,res1);
        return(NULL);
      }
        cuddRef(res2);
        keys0 = Cudd_ReadKeys(manager)-Cudd_ReadDead(manager);
      res = cuddBddAndRecur(manager, Cudd_Not(res1), Cudd_Not(res2));
      if (res == NULL) {
          Cudd_IterDerefBdd(manager, res1);
          Cudd_IterDerefBdd(manager, res2);
          return(NULL);
      }
      res = Cudd_Not(res);
      cuddRef(res);

        {
            DdNode *shared[2];
            shared[0]=res1;
            shared[1]=res2;
#if CHECK_SIZE_EX
            if ((keys0+CHECK_MIN_SIZE >
              Cudd_ReadKeys(manager)-Cudd_ReadDead(manager))
              || (Cudd_SharingSize(shared,2) >= Cudd_DagSize(res))) {
#else
          if (1) {
#endif
	    Cudd_IterDerefBdd(manager, res1);
            Cudd_IterDerefBdd(manager, res2);
	    //  infoPtr->absWithNodeDecr[F->index]++;
          }
	  else {
	    //  infoPtr->absWithNodeIncr[F->index]++;
	    Cudd_IterDerefBdd(manager, res);
	    res = cuddBddIteRecur(manager, manager->vars[F->index],
                res1, res2);
            if (res == NULL) {
              Cudd_IterDerefBdd(manager, res1);
              Cudd_IterDerefBdd(manager, res2);
              return(NULL);
            }
              cuddRef(res);
            cuddDeref(res1);
            cuddDeref(res2);
          }

      }
      if (F->ref != 1)
          cuddCacheInsert2(manager,
              cuplusBddExistAbstractRecur, f, cube, res);
      cuddDeref(res);
        return(res);
    } else { /* if (cuddI(manager,F->index) < cuddI(manager,cube->index)) */

#if DEVEL_OPT_ANDEXIST
	  /* avoid quantification with aux vars */
        if (Cuplus_IsAux[F->index]==1) {
          return(f);
        }
#endif

      res1 = cuplusBddExistAbstractRecur(manager, T, cube);
      if (res1 == NULL) return(NULL);
        cuddRef(res1);
      res2 = cuplusBddExistAbstractRecur(manager, E, cube);
      if (res2 == NULL) {
          Cudd_IterDerefBdd(manager, res1);
          return(NULL);
      }
        cuddRef(res2);
      /* ITE takes care of possible complementation of res1 */
      res = cuddBddIteRecur(manager, manager->vars[F->index], res1, res2);
      if (res == NULL) {
          Cudd_IterDerefBdd(manager, res1);
          Cudd_IterDerefBdd(manager, res2);
          return(NULL);
      }
      cuddDeref(res1);
      cuddDeref(res2);
      if (F->ref != 1)
          cuddCacheInsert2(manager, cuplusBddExistAbstractRecur, f, cube, res);
        return(res);
    }

} /* end of cuplusBddExistAbstractRecur */


/**Function********************************************************************

  Synopsis    [Performs the recursive steps of Cudd_bddExistAbstract.]

  Description [Performs the recursive steps of Cudd_bddExistAbstract.
  Returns the BDD obtained by abstracting the variables
  of cube from f if successful; NULL otherwise. It is also used by
  Cudd_bddUnivAbstract.]

  SideEffects [None]

  SeeAlso     [Cudd_bddExistAbstract Cudd_bddUnivAbstract]

******************************************************************************/
DdNode *
cuplusBddExistAbstractWeakRecur(
  DdManager * manager,
  DdNode * f,
  DdNode * cube)
{
    DdNode      *F, *T, *E, *res, *res1, *res2, *one;

    statLine(manager);
    one = DD_ONE(manager);
    F = Cudd_Regular(f);

    /* Cube is guaranteed to be a cube at this point. */
    if (cube == one || F == one) {
        return(f);
    }
    /* From now on, f and cube are non-constant. */

    /* Abstract a variable that does not appear in f. */
#if 0
    while (manager->perm[F->index] > manager->perm[cube->index]) {
      cube = cuddT(cube);
      if (cube == one) return(f);
    }
#endif
    /* Check the cache. */
    if (F->ref != 1 && (res = cuddCacheLookup2(manager,
        cuplusBddExistAbstractWeakRecur, f, cube)) != NULL) {
      return(res);
    }

    /* Compute the cofactors of f. */
    T = cuddT(F); E = cuddE(F);
    if (f != F) {
      T = Cudd_Not(T); E = Cudd_Not(E);
    }

    /* If the two indices are the same, so are their levels. */

    if (manager->perm[F->index] > manager->perm[cube->index]) {
        int keys0;

        if (F==f)
          return(one);
        else
          return(Cudd_Not(one));

      if (T == one || E == one || T == Cudd_Not(E)) {
          return(one);
      }
#if 1


        if (cuddT(cube)!=one) {
          cube = cuddT(cube);
          res1 = T;
      }
      else
      res1 = cuplusBddExistAbstractWeakRecur(manager, T, cuddT(cube));
#else
      res1 = cuplusBddExistAbstractWeakRecur(manager, T, cuddT(cube));
#endif
      if (res1 == NULL) return(NULL);
      if (res1 == one) {
          if (F->ref != 1)
            cuddCacheInsert2(manager, cuplusBddExistAbstractWeakRecur, f, cube, one);
          return(one);
      }
        cuddRef(res1);
#if 0
        if (cuddT(cube)!=one) {
          cube = cuddT(cube);
          res2 = one;
      }
      else
      res2 = cuplusBddExistAbstractWeakRecur(manager, E, cuddT(cube));
#else
      res2 = cuplusBddExistAbstractWeakRecur(manager, E, cuddT(cube));
#endif
      if (res2 == NULL) {
          Cudd_IterDerefBdd(manager,res1);
          return(NULL);
      }
        cuddRef(res2);
        keys0 = Cudd_ReadKeys(manager)-Cudd_ReadDead(manager);
#if 1
      res = cuddBddAndRecur(manager, Cudd_Not(res1), Cudd_Not(res2));
#else
      res = cuddBddIteRecur(manager, manager->vars[F->index], res1, res2);
#endif
      if (res == NULL) {
          Cudd_IterDerefBdd(manager, res1);
          Cudd_IterDerefBdd(manager, res2);
          return(NULL);
      }
      res = Cudd_Not(res);
      cuddRef(res);

        {
	  DdNode *shared[2];
	  shared[0]=res1;
	  shared[1]=res2;
          if (1) {
	    Cudd_IterDerefBdd(manager, res1);
            Cudd_IterDerefBdd(manager, res2);
	    //  infoPtr->absWithNodeDecr[F->index]++;
          }

      }
      if (F->ref != 1)
	cuddCacheInsert2(manager,
			 cuplusBddExistAbstractWeakRecur, f, cube, res);
      cuddDeref(res);
      return(res);
    } else { /* if (cuddI(manager,F->index) < cuddI(manager,cube->index)) */

#if DEVEL_OPT_ANDEXIST
      /* avoid quantification with aux vars */
      if (Cuplus_IsAux[F->index]==1) {
	return(f);
      }
#endif
      res1 = cuplusBddExistAbstractWeakRecur(manager, T, cube);
      if (res1 == NULL) return(NULL);
        cuddRef(res1);
      res2 = cuplusBddExistAbstractWeakRecur(manager, E, cube);
#if 1
        if ((res2 == res1)&&(res2==Cudd_Not(one))) {
          res1 = one;
        }
#endif
      if (res2 == NULL) {
          Cudd_IterDerefBdd(manager, res1);
          return(NULL);
      }
        cuddRef(res2);
      /* ITE takes care of possible complementation of res1 */
      res = cuddBddIteRecur(manager, manager->vars[F->index], res1, res2);
      if (res == NULL) {
          Cudd_IterDerefBdd(manager, res1);
          Cudd_IterDerefBdd(manager, res2);
          return(NULL);
      }
      cuddDeref(res1);
      cuddDeref(res2);
      if (F->ref != 1)
          cuddCacheInsert2(manager, cuplusBddExistAbstractWeakRecur, f, cube, res);
        return(res);
    }

} /* end of cuplusBddExistAbstractWeakRecur */


/**Function********************************************************************

  Synopsis    [Performs the recursive steps of Cudd_bddExistAbstract.]

  Description [Performs the recursive steps of Cudd_bddExistAbstract.
  Returns the BDD obtained by abstracting the variables
  of cube from f if successful; NULL otherwise. It is also used by
  Cudd_bddUnivAbstract.]

  SideEffects [None]

  SeeAlso     [Cudd_bddExistAbstract Cudd_bddUnivAbstract]

******************************************************************************/
DdNode *
cuplusBddExistAbstractAuxRecur(
  DdManager * manager,
  DdNode * f,
  DdNode * cube)
{
    DdNode      *F, *T, *E, *res, *res1, *res2, *one;

    statLine(manager);
    one = DD_ONE(manager);
    F = Cudd_Regular(f);

    /* Cube is guaranteed to be a cube at this point. */
    if (cube == one || F == one) {
        return(f);
    }
    /* From now on, f and cube are non-constant. */

    /* Abstract a variable that does not appear in f. */
    while (manager->perm[F->index] > manager->perm[cube->index]) {
      cube = cuddT(cube);
      if (cube == one) return(f);
    }

    /* Check the cache. */
    if (F->ref != 1 && (res = cuddCacheLookup2(manager,
        cuplusBddExistAbstractAuxRecur, f, cube)) != NULL) {
      return(res);
    }

    /* Compute the cofactors of f. */
    T = cuddT(F); E = cuddE(F);
    if (f != F) {
      T = Cudd_Not(T); E = Cudd_Not(E);
    }

    /* If the two indices are the same, so are their levels. */
    if (F->index == cube->index) {
        int keys0;
      if (T == one || E == one || T == Cudd_Not(E)) {
          return(one);
      }
      res1 = cuplusBddExistAbstractAuxRecur(manager, T, cuddT(cube));
      if (res1 == NULL) return(NULL);
      if (res1 == one) {
          if (F->ref != 1)
            cuddCacheInsert2(manager, cuplusBddExistAbstractAuxRecur,
                  f, cube, one);
          return(one);
      }
        cuddRef(res1);
      res2 = cuplusBddExistAbstractAuxRecur(manager, E, cuddT(cube));
      if (res2 == NULL) {
          Cudd_IterDerefBdd(manager,res1);
          return(NULL);
      }
        cuddRef(res2);
        keys0 = Cudd_ReadKeys(manager)-Cudd_ReadDead(manager);
      res = cuddBddAndRecur(manager, Cudd_Not(res1), Cudd_Not(res2));
      if (res == NULL) {
          Cudd_IterDerefBdd(manager, res1);
          Cudd_IterDerefBdd(manager, res2);
          return(NULL);
      }
      res = Cudd_Not(res);
      cuddRef(res);

        Cudd_IterDerefBdd(manager, res1);
        Cudd_IterDerefBdd(manager, res2);

      if (F->ref != 1)
          cuddCacheInsert2(manager,
              cuplusBddExistAbstractAuxRecur, f, cube, res);
      cuddDeref(res);
        return(res);
    } else { /* if (cuddI(manager,F->index) < cuddI(manager,cube->index)) */

#if DEVEL_OPT_ANDEXIST
        /* this is an aux var ! */
        if (Cuplus_IsAux[F->index]==1) {
          res1 = cuddBddAndRecur(manager, T, E);
	  if (res1 == NULL) {
	    return(NULL);
	  }
          cuddRef(res1);
          res = cuplusBddExistAbstractAuxRecur(manager, res1, cube);
          if (res == NULL) {
	    Cudd_IterDerefBdd(manager, res1);
	    return(NULL);
	  }
          cuddRef(res);
          Cudd_IterDerefBdd(manager, res1);
          cuddDeref(res);
        }
        else 
#endif
	{
          res1 = cuplusBddExistAbstractAuxRecur(manager, T, cube);
          if (res1 == NULL) return(NULL);
          cuddRef(res1);
          res2 = cuplusBddExistAbstractAuxRecur(manager, E, cube);
          if (res2 == NULL) {
              Cudd_IterDerefBdd(manager, res1);
              return(NULL);
          }
          cuddRef(res2);

          /* ITE takes care of possible complementation of res1 */
          res = cuddBddIteRecur(manager, manager->vars[F->index], res1, res2);
          if (res == NULL) {
              Cudd_IterDerefBdd(manager, res1);
              Cudd_IterDerefBdd(manager, res2);
              return(NULL);
          }
          cuddDeref(res1);
          cuddDeref(res2);
        }
      if (F->ref != 1)
          cuddCacheInsert2(manager, cuplusBddExistAbstractAuxRecur, f, cube, res);
        return(res);
    }

} /* end of cuplusBddExistAbstractRecur */


/**Function********************************************************************

  Synopsis [Takes the AND of two BDDs and simultaneously abstracts the
  variables in cube.]

  Description [Takes the AND of two BDDs and simultaneously abstracts
  the variables in cube. The variables are existentially abstracted.
  Returns a pointer to the result is successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_bddAndAbstract]

******************************************************************************/

DdNode *
cuplusBddAndAbstractRecur(
  DdManager * manager,
  DdNode * f,
  DdNode * g,
  DdNode * cube
  )
{
    DdNode *oldCube;
    DdNode *F, *ft, *fe, *G, *gt, *ge;
    DdNode *one, *zero;
    DdNode *r = NULL;
    DdNode *t = NULL;
    DdNode *e = NULL;
    unsigned int topf, topg, topcube, top, index;
    unsigned int keys0, keys1, keys2;
    int quantify, newvar;

    statLine (manager);
    one = DD_ONE(manager);
    zero = Cudd_Not(one);

    /* Terminal cases. */
    if (f == zero || g == zero || f == Cudd_Not(g)) return(zero);
    if (f == one && g == one)      return(one);

#if 0
    if (cube == one) {
      return(cuddBddAndRecur(manager, f, g));
    }
#endif

    if (f == one || f == g) {
      return(cuplusBddExistAbstractRecur(manager, g, cube));
    }
    if (g == one) {
      return(cuplusBddExistAbstractRecur(manager, f, cube));
    }
    /* At this point f, g, and cube are not constant. */

    if (f > g) { /* Try to increase cache efficiency. */
      DdNode *tmp = f;
      f = g;
      g = tmp;
    }

    /* Here we can skip the use of cuddI, because the operands are known
    ** to be non-constant.
    */

    F = Cudd_Regular(f);
    G = Cudd_Regular(g);

    topf = manager->perm[F->index];
    topg = manager->perm[G->index];
    top = ddMin(topf, topg);
    if (cube == one)
      topcube = cube->index;
    else
      topcube = manager->perm[cube->index];

    while (topcube < top) {
      cube = cuddT(cube);
#if 0
      if (cube == one) {
          return(cuddBddAndRecur(manager, f, g));
      }
      topcube = manager->perm[cube->index];
#else
        if (cube == one)
          topcube = cube->index;
        else
          topcube = manager->perm[cube->index];
#endif
    }
    /* Now, topcube >= top. */

    oldCube = cube;

    /* Check cache. */
    if (topf == top) {
      index = F->index;
    } else {
      index = G->index;
    }

#if DEVEL_OPT_ANDEXIST
    if ((Cuplus_AuxVars[index]==NULL)||
        (!Cuplus_DisabledAux[Cuplus_AuxVars[index]->index]))
#endif
    if (F->ref != 1 || G->ref != 1) {
      r = cuddCacheLookup(manager, DD_BDD_AND_ABSTRACT_TAG, f, g, cube);
      if (r != NULL) {
          return(r);
      }
    }

    /* GP PATCH */

#if QUANTIFY_BEFORE_RECUR

    if (topcube == top) {
      if ((topf != top)&&(topg == top)) {
      DdNode *tmp = f;
      f = g;
      g = tmp;
        F = Cudd_Regular(f);
        G = Cudd_Regular(g);
        topf = manager->perm[F->index];
        topg = manager->perm[G->index];
      }
      if ((topf == top)&&(topg != top)) {
        /* quantify top var from f */
      DdNode *fabs, *Cube;
      ft = cuddT(F);
      fe = cuddE(F);
      if (Cudd_IsComplement(f)) {
          ft = Cudd_Not(ft);
          fe = Cudd_Not(fe);
      }
        fabs = cuddBddAndRecur(manager, Cudd_Not(ft), Cudd_Not(fe));
      if (fabs == NULL) {
         return(NULL);
      }
      fabs = Cudd_Not(fabs);
      cuddRef(fabs);

        /* recur */
      Cube = cuddT(cube);
      r = cuplusBddAndAbstractRecur(manager, fabs, g, Cube);
      if (r == NULL) return(NULL);
      cuddRef(r);

      Cudd_DelayedDerefBdd(manager, fabs);
      cuddDeref(r);

        if (F->ref != 1 || G->ref != 1)
           cuddCacheInsert(manager, DD_BDD_AND_ABSTRACT_TAG, f, g, cube, r);
        return (r);
      }
    }
#endif
    /* end GP PATCH */

    if (F->index == G->index) {
      newvar = 0;
    }
    else {
      if (manager->perm[F->index] == top)
        newvar = 1;
      else
        newvar = -1;
    }

    if (topf == top) {
      index = F->index;
      ft = cuddT(F);
      fe = cuddE(F);
      if (Cudd_IsComplement(f)) {
          ft = Cudd_Not(ft);
          fe = Cudd_Not(fe);
      }
    } else {
      index = G->index;
      ft = fe = f;
    }

    if (topg == top) {
      gt = cuddT(G);
      ge = cuddE(G);
      if (Cudd_IsComplement(g)) {
          gt = Cudd_Not(gt);
          ge = Cudd_Not(ge);
      }
    } else {
      gt = ge = g;
    }

    quantify = 0;
    if (topcube == top) {      /* quantify */
      DdNode *Cube = cuddT(cube);
        quantify = 1;

      t = cuplusBddAndAbstractRecur(manager, ft, gt, Cube);
      if (t == NULL) return(NULL);
      /* Special case: 1 OR anything = 1. Hence, no need to compute
      ** the else branch if t is 1. Likewise t + t * anything == t.
      ** Notice that t == fe implies that fe does not depend on the
      ** variables in Cube. Likewise for t == ge.
      */
      if (t == one || t == fe || t == ge) {
          if (F->ref != 1 || G->ref != 1)
            cuddCacheInsert(manager, DD_BDD_AND_ABSTRACT_TAG,
                        f, g, cube, t);
          return(t);
      }
      cuddRef(t);
      /* Special case: t + !t * anything == t + anything. */
      if (t == Cudd_Not(fe)) {
          e = cuplusBddExistAbstractRecur(manager, ge, Cube);
      } else if (t == Cudd_Not(ge)) {
          e = cuplusBddExistAbstractRecur(manager, fe, Cube);
      } else {
          e = cuplusBddAndAbstractRecur(manager, fe, ge, Cube);
      }
      if (e == NULL) {
          Cudd_IterDerefBdd(manager, t);
          return(NULL);
      }
      if (t == e) {
          r = t;
          cuddDeref(t);
      } else {
            DdNode *shared[2];
          cuddRef(e);
            keys0 = Cudd_ReadKeys(manager)-Cudd_ReadDead(manager);
          r = cuddBddAndRecur(manager, Cudd_Not(t), Cudd_Not(e));
          if (r == NULL) {
            Cudd_IterDerefBdd(manager, t);
            Cudd_IterDerefBdd(manager, e);
            return(NULL);
          }
          r = Cudd_Not(r);
          cuddRef(r);
            shared[0]=t;
            shared[1]=e;
#if CHECK_SIZE_EX
            if ((keys0+CHECK_MIN_SIZE >
              Cudd_ReadKeys(manager)-Cudd_ReadDead(manager))
              || (Cudd_SharingSize(shared,2) >= Cudd_DagSize(r))) {
#else
          if (1) {
#endif
	    Cudd_DelayedDerefBdd(manager, t);
            Cudd_DelayedDerefBdd(manager, e);
	    //	    absWithNodeDecr[index]++;
	    cuddDeref(r);
          }
            else {
              // absWithNodeIncr[index]++;
	      Cudd_IterDerefBdd(manager, r);
	      if (Cudd_IsComplement(t)) {
		r = cuddUniqueInter(manager, (int) index,
                                  Cudd_Not(t), Cudd_Not(e));
                  if (r == NULL) {
                      Cudd_IterDerefBdd(manager, t);
                      Cudd_IterDerefBdd(manager, e);
                      return(NULL);
                  }
                  r = Cudd_Not(r);
            } else {
                  r = cuddUniqueInter(manager,(int)index,t,e);
                  if (r == NULL) {
                      Cudd_IterDerefBdd(manager, t);
                      Cudd_IterDerefBdd(manager, e);
                      return(NULL);
                  }
            }
            cuddDeref(e);
            cuddDeref(t);
          }
      }
    }
    if (!quantify) {
        DdNode *shared[4];
        int sizeR = (-1);

        shared[0]=ft;
        shared[1]=fe;
        shared[2]=gt;
        shared[3]=ge;

        keys0 = Cudd_ReadKeys(manager)-Cudd_ReadDead(manager);
#if DEVEL_OPT_ANDEXIST
        if (Cuplus_IsAux[index]==1) {
          if (newvar!=0) {
            DdNode *at, *ae, *b, *supp, *cube2, *a;
            Cudd_ReorderingType dynordMethod;
            int dynord = Cudd_ReorderingStatus (manager, &dynordMethod);

            if (newvar == 1) {
              at = ft; ae = fe; b = g;
	    }
            else {
              at = gt; ae = ge; b = f;
	    }
            if ((cube != one)
              /*&&(Cudd_NodeReadIndex(at)==cube->index)*/) {
              Cudd_AutodynDisable(manager);

              supp = Cudd_Support(manager,b);
              cuddRef(supp);
              cube2 = cuddBddExistAbstractRecur(manager,cube,supp);
              cuddRef(cube2);
	      Cudd_IterDerefBdd(manager, supp);
              if (dynord)
                Cudd_AutodynEnable(manager,dynordMethod);
              Cuplus_DisabledAux[index] = 1;
              a = cuplusBddAndAbstractRecur(manager, at, ae, cube2);
              Cuplus_DisabledAux[index] = 0;
              if (a == NULL) {
		Cudd_IterDerefBdd(manager, cube2);
		return(NULL);
	      }
              cuddRef(a);
	      Cudd_IterDerefBdd(manager, cube2);
              Cuplus_DisabledAux[index] = 0;
              r = cuplusBddAndAbstractRecur(manager, a, b, cube);
              Cuplus_DisabledAux[index] = 0;
              if (r == NULL) {
		Cudd_IterDerefBdd(manager, a);
		return(NULL);
	      }
              cuddRef(r);
	      Cudd_IterDerefBdd(manager, a);
              cuddDeref(r);
#if 1
              if (F->ref != 1 || G->ref != 1)
		cuddCacheInsert(manager, DD_BDD_AND_ABSTRACT_TAG,
				f, g, cube, r);
#endif
              return(r);
            }
          }
          oldCube = cube;
          cube = one;
          Cuplus_DisabledAux[index] = 1;
	}
#endif
#if 0
	myaaa = aaa++;
	if (myaaa==7994208)
	  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
				 Pdtutil_VerbLevelNone_c,
				 fprintf(stdout, "OK!.\n")
				 );
#endif
      t = cuplusBddAndAbstractRecur(manager, ft, gt, cube);
#if DEVEL_OPT_ANDEXIST
      if (Cuplus_IsAux[index]==1) {
	Cuplus_DisabledAux[index] = 0;
      }
#endif
      if (t == NULL) return(NULL);
      keys1 = Cudd_ReadKeys(manager)-Cudd_ReadDead(manager);
      cuddRef(t);

#if DEVEL_OPT_ANDEXIST
      if (Cuplus_IsAux[index]==1) {
	Cuplus_DisabledAux[index] = 1;
      }
#endif
      e = cuplusBddAndAbstractRecur(manager, fe, ge, cube);
#if DEVEL_OPT_ANDEXIST
      if (Cuplus_IsAux[index]==1) {
	Cuplus_DisabledAux[index] = 0;
      }
#endif
      if (e == NULL) {
          Cudd_IterDerefBdd(manager, t);
          return(NULL);
      }
      keys2 = Cudd_ReadKeys(manager)-Cudd_ReadDead(manager);
      if (t == e) {
	r = t;
	cuddDeref(t);
      } else {

	cuddRef(e);
#if DEVEL_OPT_ANDEXIST
	if ((newvar!=0)&&(Cuplus_IsAux[index]==1)) {
	  if (1) {
	    DdNode *a, *b;
	    if ((keys1-keys0)<(keys2-keys1)) {
	      if (newvar == 1) {
		a = t; b = fe;
	      }
	      else {
		a = t; b = ge;
	      }
	    }
	    else {
	      if (newvar == 1) {
		a = ft; b = e;
	      }
	      else {
		a = gt; b = e;
	      }
	    }
	    if ((oldCube == one)
		||(Cudd_NodeReadIndex(a)!=oldCube->index)) {
	      r = cuddBddIteRecur(manager, manager->vars[index], a, b);
	    }
	    else {
	      if (Cudd_NodeReadIndex(a)==index)
		Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
				       Pdtutil_VerbLevelNone_c,
				       fprintf(stdout, "#.\n")
				       );
	      if (Cudd_NodeReadIndex(b)==index)
		Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
				       Pdtutil_VerbLevelNone_c,
				       fprintf(stdout, "^.\n")
				       );
	      r = cuplusBddAndAbstractRecur(manager, a, b, oldCube);
            }
	  }
	  else {
	    Cudd_IterDerefBdd(manager, t);
	    Cudd_IterDerefBdd(manager, e);
	    if (newvar == 1) {
	      t = cuddBddAndRecur(manager, fe, g);
	      e = ft;
	    }
	    else {
	      t = cuddBddAndRecur(manager, ge, f);
	      e = gt;
	    }
	    if (t == NULL) {
	      return(NULL);
	    }
	    cuddRef(t);
	    cuddRef(e);
#if 0
	    r = cuddBddIteRecur(manager, manager->vars[index], t, e);
#else
	    r = cuplusBddAndAbstractRecur(manager, t, e, oldCube);
#endif
	    if (r == NULL) {
	      Cudd_IterDerefBdd(manager, t);
	      Cudd_IterDerefBdd(manager, e);
	      return(NULL);
	    }
#if 0
	    cuddRef(r);
	    Cudd_IterDerefBdd(manager, t);
	    Cudd_IterDerefBdd(manager, e);
	    cuddDeref(r);
	    return (r);
#endif
	  }
	}
	else
#endif
	{
	  r = cuddBddIteRecur(manager, manager->vars[index], t, e);
	}
	if (r == NULL) {
	  Cudd_IterDerefBdd(manager, t);
	  Cudd_IterDerefBdd(manager, e);
	  return(NULL);
	}
	
	cuddRef(r);
#if CHECK_SIZE_AND
	keys2 = Cudd_ReadKeys(manager)-Cudd_ReadDead(manager);
	if ((keys0+CHECK_MIN_SIZE < keys2)
#if 0
	    && (top>manager->size/3)
#endif
#if 1
	    && (1)) {
#else
	  && (2*Cudd_SharingSize(shared,4) < (sizeR=Cudd_DagSize(r)))) {
#endif
	  if (newvar!=0) {
#if DEVEL_OPT_ANDEXIST
            {
	    int enableAux = 0;
	    newvarSizeIncr += sizeR;
	    newvarSizeNum++;

	    if (Cuplus_AuxVars[index]!=NULL) {
	      if (!Cuplus_DisabledAux[Cuplus_AuxVars[index]->index]) {
		DdNode *ff, *gg;
		ff = cuddCofactorRecur(manager,
				       f,Cuplus_AuxVars[index]);
		if (ff == NULL) {
		  Cudd_IterDerefBdd(manager, t);
		  Cudd_IterDerefBdd(manager, e);
		  Cudd_IterDerefBdd(manager, r);
		  return(NULL);
		}
		cuddRef(ff);
		gg = cuddCofactorRecur(manager,
				       g,Cuplus_AuxVars[index]);
		if (gg == NULL) {
		  Cudd_IterDerefBdd(manager, t);
		  Cudd_IterDerefBdd(manager, e);
		  Cudd_IterDerefBdd(manager, r);
		  Cudd_IterDerefBdd(manager, ff);
		  return(NULL);
		}
		cuddRef(gg);
		if ((f == ff)&&(g == gg)) {
		  enableAux = 1;
		}
		Cudd_IterDerefBdd(manager, ff);
		Cudd_IterDerefBdd(manager, gg);
	      }
	    }
	    }
#endif
#if 1
#if DEVEL_OPT_ANDEXIST
	    if (enableAux) {
	      Cudd_IterDerefBdd(manager, t);
	      Cudd_IterDerefBdd(manager, e);
	      Cudd_IterDerefBdd(manager, r);
	      if (newvar == 1) {
		r = cuddBddIteRecur(manager,
				    Cuplus_AuxVars[index], f, g);
	      }
	      else {
		r = cuddBddIteRecur(manager,
				    Cuplus_AuxVars[index], g, f);
	      }
	      if (r == NULL) {
		return(NULL);
	      }
	      cuddRef(r);
	    }
	    else 
#endif
	    {
	      Cudd_IterDerefBdd(manager, t);
	      Cudd_IterDerefBdd(manager, e);
	    }
#else
	    {
	      DdNode *tmp0, *tmp1, *tmp2, *ff, *gg;
	      if (newvar == 1) {
		ff = f; gg = g;
	      }
	      else {
		ff = g; gg = f;
	      }
	      tmp0 = cuplusBddExistAbstractRecur(manager, gg, cube);
	      if (tmp0 == NULL) {
		Cudd_IterDerefBdd(manager, t);
		Cudd_IterDerefBdd(manager, e);
		Cudd_IterDerefBdd(manager, r);
		return(NULL);
	      }
	      cuddRef(tmp0);
	      tmp1 = cuddBddConstrainRecur(manager,r,tmp0);
	      if (tmp1 == NULL) {
		Cudd_IterDerefBdd(manager, t);
		Cudd_IterDerefBdd(manager, e);
		Cudd_IterDerefBdd(manager, r);
		Cudd_IterDerefBdd(manager, tmp0);
		return(NULL);
	      }
	      cuddRef(tmp1);
	      
	      shared[0]=tmp0;
	      shared[1]=tmp1;
	      newvarSizeRepl += (sizeNew = Cudd_SharingSize(shared,2));
	      
	      if ((sizeNew+1 < sizeR)&&(enableAux)) {
		Cudd_IterDerefBdd(manager, r);
		
		r = cuddBddIteRecur(manager,
				    Cuplus_AuxVars[index], tmp1, tmp0);
		if (r == NULL) {
		  Cudd_IterDerefBdd(manager, t);
		  Cudd_IterDerefBdd(manager, e);
		  Cudd_IterDerefBdd(manager, tmp0);
		  Cudd_IterDerefBdd(manager, tmp1);
		  return(NULL);
		}
		cuddRef(r);
	      }
	      Cudd_IterDerefBdd(manager, tmp0);
	      Cudd_IterDerefBdd(manager, tmp1);
	      
	    }
	    Cudd_IterDerefBdd(manager, t);
	    Cudd_IterDerefBdd(manager, e);
#endif
	  }
	  else {
#if DEVEL_OPT_ANDEXIST
	    keepvarSizeIncr += sizeR;
	    keepvarSizeNum++;
#endif
	    Cudd_IterDerefBdd(manager, t);
	    Cudd_IterDerefBdd(manager, e);
	  }
	}
	else {
	  Cudd_IterDerefBdd(manager, t);
	  Cudd_IterDerefBdd(manager, e);
	}
#else
	Cudd_IterDerefBdd(manager, t);
	Cudd_IterDerefBdd(manager, e);
#endif
        cuddDeref(r);
      }
    }
    
#if DEVEL_OPT_ANDEXIST
    if ((Cuplus_AuxVars[index]==NULL)||
        (!Cuplus_DisabledAux[Cuplus_AuxVars[index]->index]))
      if (F->ref != 1 || G->ref != 1)
	cuddCacheInsert(manager, DD_BDD_AND_ABSTRACT_TAG, f, g, oldCube, r);
#endif

    return (r);

} /* end of cuddBddAndAbstractRecur */


/**Function********************************************************************

  Synopsis [Takes the AND of two BDDs and simultaneously abstracts the
  variables in cube.]

  Description [Takes the AND of two BDDs and simultaneously abstracts
  the variables in cube. The variables are existentially abstracted.
  Returns a pointer to the result is successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_bddAndAbstract]

******************************************************************************/
DdNode *
cuplusBddAndAbstractRecur2(
  DdManager * manager,
  DdNode * f,
  DdNode * g,
  DdNode * cube,
  int *abortFrontier,
  int *elseFirst
)
{
    DdNode *oldCube;
    DdNode *F, *ft, *fe, *G, *gt, *ge;
    DdNode *one, *zero, *r, *t, *e;
    unsigned int topf, topg, topcube, top, index;
    int else_first;

    statLine(manager);
    one = DD_ONE(manager);
    zero = Cudd_Not(one);

    /* Terminal cases. */
    if (f == zero || g == zero || f == Cudd_Not(g)) return(zero);
    if (f == one && g == one)      return(one);

    if (f == one || f == g) {
      return(cuplusBddExistAbstractRecur(manager, g, cube));
    }
    if (g == one) {
      return(cuplusBddExistAbstractRecur(manager, f, cube));
    }
    /* At this point f, g, and cube are not constant. */

    if (f > g) { /* Try to increase cache efficiency. */
      DdNode *tmp = f;
      f = g;
      g = tmp;
    }

    /* Here we can skip the use of cuddI, because the operands are known
    ** to be non-constant.
    */

    F = Cudd_Regular(f);
    G = Cudd_Regular(g);

    topf = manager->perm[F->index];
    topg = manager->perm[G->index];
    top = ddMin(topf, topg);
    if (cube == one)
      topcube = cube->index;
    else
      topcube = manager->perm[cube->index];

    while (topcube < top) {
      cube = cuddT(cube);
        if (cube == one)
          topcube = cube->index;
        else
          topcube = manager->perm[cube->index];
    }
    /* Now, topcube >= top. */

    oldCube = cube;

    /* Check cache. */
    if (topf == top) {
      index = F->index;
    } else {
      index = G->index;
    }

    if (F->ref != 1 || G->ref != 1) {
      r = cuddCacheLookup(manager, DD_BDD_AND_ABSTRACT_TAG2, f, g, cube);
      if (r != NULL) {
          return(r);
      }
    }

    if (topf == top) {
      index = F->index;
      ft = cuddT(F);
      fe = cuddE(F);
      if (Cudd_IsComplement(f)) {
          ft = Cudd_Not(ft);
          fe = Cudd_Not(fe);
      }
    } else {
      index = G->index;
      ft = fe = f;
    }

    if (topg == top) {
      gt = cuddT(G);
      ge = cuddE(G);
      if (Cudd_IsComplement(g)) {
          gt = Cudd_Not(gt);
          ge = Cudd_Not(ge);
      }
    } else {
      gt = ge = g;
    }

    else_first = (elseFirst != NULL) && (elseFirst[index]);

    if (else_first) {
      DdNode *tmp = ft;
      ft = fe; fe = tmp;
      tmp = gt; gt = ge; ge = tmp;
    }

    if (topcube == top) {      /* quantify */
      DdNode *Cube = cuddT(cube);

      t = cuplusBddAndAbstractRecur2(manager, ft, gt, Cube,
            abortFrontier, elseFirst);
      if (t == NULL) {
          if (abortFrontier != NULL) {
            abortFrontier[index] = 1;
        }
          return(NULL);
      }
      /* Special case: 1 OR anything = 1. Hence, no need to compute
      ** the else branch if t is 1. Likewise t + t * anything == t.
      ** Notice that t == fe implies that fe does not depend on the
      ** variables in Cube. Likewise for t == ge.
      */
      if (t == one || t == fe || t == ge) {
          if (F->ref != 1 || G->ref != 1)
            cuddCacheInsert(manager, DD_BDD_AND_ABSTRACT_TAG,
                        f, g, cube, t);
          return(t);
      }
      cuddRef(t);

        e = cuplusBddAndAbstractRecur2(manager, fe, ge, Cube,
            abortFrontier, elseFirst);
      if (e == NULL) {
            if (abortFrontier != NULL) {
              abortFrontier[index] = 0;
          }
          Cudd_IterDerefBdd(manager, t);
          return(NULL);
      }
      if (t == e) {
          r = t;
          cuddDeref(t);
      } else {
          cuddRef(e);
#if 1
            r = cuplusBddAndAbstractRecur2(manager,
                 Cudd_Not(t), Cudd_Not(e), one, abortFrontier, elseFirst);
#else
          r = cuddBddAndRecur(manager, Cudd_Not(t), Cudd_Not(e));
#endif
          if (r == NULL) {
            Cudd_IterDerefBdd(manager, t);
            Cudd_IterDerefBdd(manager, e);
                if (abortFrontier != NULL) {
                /* abort or sift when OR-ing. Put frontier after 1 cofactor */
              /* abortFrontier[index] = 1; */
                }
            return(NULL);
          }
          r = Cudd_Not(r);
          cuddRef(r);
            Cudd_DelayedDerefBdd(manager, t);
          Cudd_DelayedDerefBdd(manager, e);
#if DEVEL_OPT_ANDEXIST
	  absWithNodeDecr[index]++;
#endif
	  cuddDeref(r);
      }
    }
    else {
      t = cuplusBddAndAbstractRecur2(manager, ft, gt, cube,
            abortFrontier, elseFirst);
      if (t == NULL) {
          if (abortFrontier != NULL) {
            abortFrontier[index] = 1;
        }
          return(NULL);
      }
      cuddRef(t);
      e = cuplusBddAndAbstractRecur2(manager, fe, ge, cube,
            abortFrontier, elseFirst);
      if (e == NULL) {
          Cudd_IterDerefBdd(manager, t);
            if (abortFrontier != NULL) {
              abortFrontier[index] = 0;
             }
          return(NULL);
      }
      if (t == e) {
          r = t;
          cuddDeref(t);
      } else {
          cuddRef(e);
            if (else_first) {
              DdNode *tmp = t;
              t = e; e = tmp;
            }
              r = cuddBddIteRecur(manager, manager->vars[index], t, e);
            if (r == NULL) {
               Cudd_IterDerefBdd(manager, t);
               Cudd_IterDerefBdd(manager, e);
               return(NULL);
            }
          cuddRef(r);
             Cudd_IterDerefBdd(manager, t);
          Cudd_IterDerefBdd(manager, e);
          cuddDeref(r);
      }
    }

    if (F->ref != 1 || G->ref != 1)
      cuddCacheInsert(manager, DD_BDD_AND_ABSTRACT_TAG2, f, g, cube, r);
    return (r);

} /* end of cuddBddAndAbstractRecur2 */



/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Recursive stap of AndAbstract with profile handling.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static DdNode *
cuplusBddAndAbstractProjectRecur (
  DdManager *manager,
  DdNode *f,
  DdNode *g,
  DdNode *cube,
  st_table *table,
  DdNode *projectDummyPtr
  )
{
  DdNode *F, *fv, *fnv, *G, *gv, *gnv;
  DdNode *one, *zero, *Cube;
  unsigned int topf, topg, topcube, top, index;

  one = DD_ONE(manager);
  zero = Cudd_Not(one);

  /* Terminal cases. */
  if (f == zero || f == one) {
    /* f is terminal: no simplification done */
    return (f);
  }

  if (g == zero) {
    /* f not required: can be simplified (f could be set to 0 !!!) */
    if (hashSet(table,f,ProjectZero_c)==NULL) {
      return(NULL);
    }
    return(f);
  }
  if (cube == one) {
    /* no more variables to quantify */
    if (hashSet(table,f,ProjectKeepTree_c)==NULL) {
      return(NULL);
    }
    return(f);
  }
  if (f == Cudd_Not(g)) {
    /* no simplifications decided at this point,
       recur to terminals, so that simplifications are possible
       (f could be set to 0 !!!) */
    if (hashSet(table,f,ProjectZero_c)==NULL) {
      return(NULL);
    }
  }
  if (g == one || f == g) {
    /*
     * full exist simplification possible on f force g = one
     * so that full exist simplification done
     */
    g = one;
  }

  /* Check cache. */
  if (cuddCacheLookup(manager, DD_BDD_AND_ABS_PROJECT_TAG, f, g, cube)
    ==projectDummyPtr) {
    return(f);
  }

  /* At this point f, g, and cube are not constant. */

  F = Cudd_Regular(f);
  G = Cudd_Regular(g);

  /*
   *  Here we can skip the use of cuddI, because the operands are known
   *  to be non-constant.
   */

  topf = manager->perm[F->index];
  if (g == one) {
    topg = G->index;
  }
  else {
    topg = manager->perm[G->index];
  }
  top = ddMin(topf, topg);

  if (cube == one) {
    topcube = cube->index;
  }
  else {
    topcube = manager->perm[cube->index];
  }
  if (topcube < top) {
    return(cuplusBddAndAbstractProjectRecur(manager,
      f, g, cuddT(cube), table, projectDummyPtr));
  }
  /* Now, topcube >= top. */

  if (topf == top) {
    index = F->index;
    fv = cuddT(F);
    fnv = cuddE(F);
    if (Cudd_IsComplement(f)) {
      fv = Cudd_Not(fv);
      fnv = Cudd_Not(fnv);
    }
  } else {
    index = G->index;
    fv = fnv = f;
  }

  if (topg == top) {
    gv = cuddT(G);
    gnv = cuddE(G);
    if (Cudd_IsComplement(g)) {
      gv = Cudd_Not(gv);
      gnv = Cudd_Not(gnv);
    }
  } else {
    gv = gnv = g;
  }

  if (topcube == top) {
    Cube = cuddT(cube);
  } else {
    Cube = cube;
  }

  if (cuplusBddAndAbstractProjectRecur(manager,
    fv,gv,Cube,table,projectDummyPtr)==NULL) {
    return(NULL);
  }
  if (cuplusBddAndAbstractProjectRecur(manager,
    fnv,gnv,Cube,table,projectDummyPtr)==NULL) {
    return(NULL);
  }

  if ((top == topf)&&((topcube!=top)||(top==topg))) {
    /*
     * f node required since no quantification done or var common to g
     */
    if (hashSet(table,f,ProjectKeep_c)==NULL) {
      return(NULL);
    }
  }
  else {
    /*
     * f node can be quantified
     */
    if (hashSet(table,f,ProjectQuantify_c)==NULL) {
      return(NULL);
    }
  }

#if 1
  cuddCacheInsert(manager, DD_BDD_AND_ABS_PROJECT_TAG, f, g, cube,
    projectDummyPtr);
#endif

  return (NULL);
}




/**Function********************************************************************

  Synopsis    [Recursive step of BDD pruning based on profiles.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static DdNode *
cuplusBddProjectRecur (
  DdManager *manager,
  DdNode *f,
  st_table *table
)
{
  DdNode *F, *T, *E, *res, *res1, *res2, *one, *zero;
  struct project_info_item *info_item_ptr;
  int doQuantify = (-1);

  one = DD_ONE (manager);
  zero = Cudd_Not (one);

  if (f == zero || f == one) {
    return (f);
  }

  F = Cudd_Regular (f);
  if (!st_lookup (table,(char *)f,(char **)&info_item_ptr)) {
    return(one);
  }
  else {
    switch(info_item_ptr->code) {
      case ProjectKeepTree_c:
        return(f);
        break;
      case ProjectZero_c:
        return(f);
        break;
      case ProjectKeep_c:
        doQuantify = 0;
        break;
      case ProjectQuantify_c:
        doQuantify = 1;
        break;
      default:
        fprintf(stderr, "Exception in switch.\n");
        exit (1);
        break;
    }
  }

  /*
   * Check if already computed: no cache table used,
   * caching done in project info.
   */

  if (info_item_ptr->projected_bdd != NULL)
    return (info_item_ptr->projected_bdd);

  /* Compute the cofactors of f and recur */

  T = cuddT (F);
  E = cuddE (F);
  if (f != F) {
    T = Cudd_Not(T); E = Cudd_Not(E);
  }

  /* recur on cofactors */

  res1 = cuplusBddProjectRecur(manager, T, table);
  if (res1 == NULL) {
    return(NULL);
  }
  cuddRef(res1);

  res2 = cuplusBddProjectRecur(manager, E, table);
  if (res2 == NULL) {
    Cudd_IterDerefBdd (manager, res1);
    return (NULL);
  }
  cuddRef(res2);

  /* ITE takes care of possible complementation of res1 */

  if (doQuantify) {
    res = cuddBddAndRecur(manager, Cudd_Not(res1), Cudd_Not(res2));
    if (res == NULL) {
      Cudd_IterDerefBdd(manager, res1);
      Cudd_IterDerefBdd(manager, res2);
      return(NULL);
    }
    cuddRef(res);
    res = Cudd_Not(res);
  }
  else {
    res = cuddBddIteRecur(manager,Cudd_bddIthVar(manager,F->index),res1,res2);
    if (res == NULL) {
      Cudd_IterDerefBdd(manager, res1);
      Cudd_IterDerefBdd(manager, res2);
      return(NULL);
    }
    cuddRef(res);
  }

  Cudd_IterDerefBdd (manager, res1);
  Cudd_IterDerefBdd (manager, res2);
  cuddDeref(res);

  if (F->ref != 1) {
    cuddRef(res);
    info_item_ptr->projected_bdd = res;
  }

  return (res);
}


/**Function********************************************************************

  Synopsis    [Recursive step of AndAbstract with profile handling.]

  Description [Takes the AND of two BDDs and simultaneously abstracts
    the variables in cube. The variables are existentially abstracted.
    Returns a pointer to the result is successful; NULL otherwise.
    Profile statistics are upgraded. Profile is associated to the g operand.]

  SideEffects [None]

  SeeAlso     [Cudd_bddAndAbstract]

******************************************************************************/

static project_info_item_t *
hashSet (
  st_table *table,
  DdNode *f,
  Cuplus_ProjectNodeCode_e code
)
{
  struct project_info_item *info_item_ptr;

  if (!st_lookup(table,(char *)f,(char **)&info_item_ptr)) {
    info_item_ptr = Pdtutil_Alloc(struct project_info_item,1);
    if (info_item_ptr == NULL) {
      return(NULL);
    }
    if (st_add_direct(table,(char *)f,(char *)info_item_ptr)
      ==ST_OUT_OF_MEM) {
      Pdtutil_Free(info_item_ptr);
      return(NULL);
    }
    info_item_ptr->code = code;
    info_item_ptr->projected_bdd = NULL;
  }
  else {
    switch(info_item_ptr->code) {
      case ProjectKeepTree_c:
        break;
      case ProjectZero_c:
        info_item_ptr->code = code;
        break;
      case ProjectKeep_c:
        if (code == ProjectKeepTree_c)
          info_item_ptr->code = code;
        break;
      case ProjectQuantify_c:
        if (code != ProjectZero_c)
          info_item_ptr->code = code;
        break;
      default:
        fprintf(stderr, "Exception in switch.\n");
        exit (1);
        break;
    }
  }
  return(info_item_ptr);

}

/**Function********************************************************************

  Synopsis    [Frees the memory used for info items.]

  Description []

  SideEffects [None]

******************************************************************************/

static enum st_retval
ProjectInfoItemFree(
  char * key,
  char * value,
  char * arg
  )
{
  struct project_info_item *info_item_ptr;
  DdManager * manager = (DdManager *)arg;

  info_item_ptr = (struct project_info_item *)value;
  if (info_item_ptr->projected_bdd != NULL)
    Cudd_IterDerefBdd(manager, info_item_ptr->projected_bdd);
  Pdtutil_Free(info_item_ptr);

  return(ST_CONTINUE);
}

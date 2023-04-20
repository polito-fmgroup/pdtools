/**CFile***********************************************************************

  FileName    [cuplusCofexist.c]

  PackageName [cuplus]

  Synopsis    [Managing exist cofactor.]

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

static DdNode * cuplusCofexistMaskRecur(DdManager *manager, DdNode *f, DdNode *cube);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

DdNode *
Cuplus_CofexistMask (
  DdManager *manager,
  DdNode *f,
  DdNode *cube
)
{
  DdNode *res;

  /* Reordering NOT allowed */
  assert (manager->reordered == 0);

  res = cuplusCofexistMaskRecur (manager, f, cube);

  return(res);
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Recursive step of cofexist mask generation.]

  Description [.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static DdNode *
cuplusCofexistMaskRecur (
  DdManager *manager,
  DdNode *f,
  DdNode *cube
  )
{
    DdNode	*F, *T, *E, *res, *res1, *res2, *one, *zero;

    statLine(manager);
    one = DD_ONE(manager);
    zero = Cudd_Not (one);
    F = Cudd_Regular(f);

    /* Cube is guaranteed to be a cube at this point. */	
    if (cube == one || F == one) {  
        return(f);
    }
    /* From now on, f and cube are non-constant. */

    /* Skip a variable that does not appear in f. */
    while (manager->perm[F->index] > manager->perm[cube->index]) {
	cube = cuddT(cube);
	if (cube == one) return(f);
    }

    /* Check the cache. */
    if (F->ref != 1 && (res = cuddCacheLookup2(manager, 
        Cuplus_CofexistMask, f, cube)) != NULL) {
	return(res);
    }

    /* Compute the cofactors of f. */
    T = cuddT(F); E = cuddE(F);
    if (f != F) {
	T = Cudd_Not(T); E = Cudd_Not(E);
    }

    if (T == zero) {
      /* If the two indices are the same, so are their levels. */
      if (F->index == cube->index) {
        return (cuplusCofexistMaskRecur(manager, E, cube));
      }
    }
    else if (E == zero) {
      /* If the two indices are the same, so are their levels. */
      if (F->index == cube->index) {
        return (cuplusCofexistMaskRecur(manager, T, cube));
      }
    }

    /* recur on two cofactors */
    res1 = cuplusCofexistMaskRecur(manager, T, cube);
    assert(res1 != NULL);
    cuddRef(res1);
    res2 = cuplusCofexistMaskRecur(manager, E, cube);
    assert(res2 != NULL);
    cuddRef(res2);
    /* ITE takes care of possible complementation of res1 */
    res = cuddBddIteRecur(manager, manager->vars[F->index], res1, res2);
    assert(res != NULL);
    cuddDeref(res1);
    cuddDeref(res2);
    if (F->ref != 1)
      cuddCacheInsert2(manager, Cuplus_CofexistMask, f, cube, res);
    return(res);

}




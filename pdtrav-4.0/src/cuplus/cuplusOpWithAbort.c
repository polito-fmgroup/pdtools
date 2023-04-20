/**CFile***********************************************************************

  FileName    [cuplusOpWithAbort.c]

  PackageName [cuplus]

  Synopsis    [Enable operation abort within recursive calls]

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

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************
  Synopsis []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
void
Cuplus_bddOpEnableAbortOnSift(
  DdManager * manager
)
{
  Cuplus_DoAbortHookFun(manager,"ENABLE",NULL);
}


/**Function********************************************************************
  Synopsis []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
void
Cuplus_bddOpDisableAbortOnSift(
  DdManager * manager
)
{
  Cuplus_DoAbortHookFun(manager,"DISABLE",NULL);
}


/**Function********************************************************************
  Synopsis []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
int
Cuplus_DoAbortHookFun(
  DdManager * manager,
  const char *str,
  void *heuristic
)
{
  Pdtutil_Assert(str!=NULL,"Non NULL string required by Abort hook fun");
  

  if (strcmp(str,"BDD")==0) {
    /* Called by recursive procedure: test for abort */
    return(1);
  }
  else if (strcmp(str,"ENABLE")==0) {
    Cudd_AddHook (manager,Cuplus_DoAbortHookFun,CUDD_PRE_REORDERING_HOOK);
  }
  else if (strcmp(str,"DISABLE")==0) {
    Cudd_RemoveHook (manager,Cuplus_DoAbortHookFun,CUDD_PRE_REORDERING_HOOK);
    return(1);
  }
  return(1);
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/






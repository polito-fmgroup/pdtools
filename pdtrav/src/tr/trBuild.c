/**CFile***********************************************************************

  FileName    [trBuild.c]

  PackageName [tr]

  Synopsis    [Functions to Build a partitioned, clustered or monolithic
    transition relation]

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


/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of external functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Remove Lambda Latches]

  Description [Remove the so-called lambda-latches following the strategy
    presented at ICCAD'96 by Cabodi, Camurati, Quer.]

  SideEffects []

  SeeAlso     [Trav_BuildClusteredTR]

******************************************************************************/


void
Tr_RemoveLambdaLatches(
  Tr_Tr_t * tr
)
{
  Ddi_Varset_t *LLs, *LLy, *suppTR;
  Tr_Mgr_t *trMgr = tr->trMgr;
  Ddi_Bdd_t *rel;
  int i = 0;

  rel = Ddi_ExprToBdd(tr->ddiTr);

  suppTR = Ddi_BddSupp(rel);

  /* Create a variable set with all PS vars */
  LLs = Ddi_VarsetMakeFromArray(Tr_MgrReadPS(trMgr));

  /* Remove support of TR: only lambda latches survive */
  Ddi_VarsetDiffAcc(LLs, suppTR);
  Ddi_Free(suppTR);

  if (!(Ddi_VarsetIsVoid(LLs))) {
    if (1 /*Tr_MgrReadVerbosity (trMgr) >= Pdtutil_VerbLevelDevMin_c */ ) {
      fprintf(stdout, "Removing %d Lambda Latches\n", Ddi_VarsetNum(LLs));
      Ddi_VarsetPrint(LLs, 100, NULL, stdout);
    }

    /* the following is not formally correct for a varset! */
    LLy = Ddi_VarsetSwapVars(LLs, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));

    for (i = 0; i < Ddi_BddPartNum(rel); i++) {
      Ddi_BddExistAcc(Ddi_BddPartRead(rel, i), LLy);
    }

    Ddi_Free(LLy);

    /* from and reached could be simplified in trav with Exist(set,LLs) */

  }
  Ddi_Free(LLs);

  return;
}

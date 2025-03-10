/**CFile***********************************************************************

  FileName    [appAiger.c]

  PackageName [app]

  Synopsis    [Functions to handle Aiger file manipulation]

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
    Send bug-reports and/or questions to: cabodi@polito.it. ]

  Revision    []

******************************************************************************/

#include "appInt.h"

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
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Main program of certify app.]
  Description [Main program of certify app.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int App_Aiger (
  int argc,
  char *argv[]
) {
  App_Mgr_t *appMgr;
  int complement = 0;
  int symbMap = 0;
  int constrAdd = 0;
  
  char *aigInName = NULL;
  char *aigOutName = NULL;
  char *fsmInName = NULL;
  char *fsmOutName = NULL;

  
  printf("aiger app:");
  for (int i=0; i<argc; i++) {
    printf(" %s", argv[i]);
    if (strcmp(argv[i],"-s")==0) {
      i++;
      symbMap = 1;
    }
    if (strcmp(argv[i],"-c")==0) {
      i++;
      constrAdd = 1;
    }
    if (strcmp(argv[i],"-i")==0) {
      i++;
      if (i>=argc) {
        printf("\nmissing file name for combinational in (-i)\n");
        return 0;
      }
      aigInName = argv[i];
    }
    else if (strcmp(argv[i],"-n")==0) {
      complement = 1;
    }
    else if (fsmInName==NULL) {
      fsmInName = argv[i];
    }
    else if (fsmOutName==NULL) {
      fsmOutName = argv[i];
    }
    else {
      printf("\nunknown argument/option: %s\n", argv[i]);
    }
  }
  printf("\n");

  if (symbMap) {
    aigOutName = fsmOutName;
    fsmOutName = NULL;
  }
  
  appMgr = App_MgrInit("aiger", App_TaskAiger_c);

  Ddi_Vararray_t *mapVars = NULL;
  
  if (Fsm_MgrLoadAiger(&appMgr->fsmMgr, appMgr->ddiMgr,
                       fsmInName, NULL, NULL,
                       Pdtutil_VariableOrderDefault_c) == 1) {
      fprintf(stderr, "-- FSM Loading Error.\n");
      exit(EXIT_FAILURE);
  }
  mapVars = Ddi_VararrayDup(Fsm_MgrReadVarI(appMgr->fsmMgr));
  Ddi_VararrayAppend(mapVars,Fsm_MgrReadVarPS(appMgr->fsmMgr));

  Ddi_Bddarray_t *aigArray = Ddi_AigarrayNetLoadAigerMapVars(
			      appMgr->ddiMgr,
			      NULL, mapVars, aigInName);
  if (complement) {
    for (int i=0; i<Ddi_BddarrayNum(aigArray); i++)
      Ddi_BddNotAcc(Ddi_BddarrayRead(aigArray,i));
  }
  if (aigOutName)
    Ddi_AigarrayNetStoreAiger(aigArray, 0, aigOutName);
  else {
    Fsm_Fsm_t *fsmFsm = Fsm_FsmMakeFromFsmMgr(appMgr->fsmMgr);
    Ddi_Bdd_t *constr = Fsm_FsmReadConstraint(fsmFsm);
    if (constr!=NULL)
      Ddi_BddarrayInsertLast(aigArray,constr);
    Ddi_Bdd_t *newC = Ddi_BddMakePartConjFromArray(aigArray);
    Ddi_BddSetAig(newC);
    Fsm_FsmWriteConstraint(fsmFsm,newC);
    Ddi_Free(newC);
    Fsm_FsmMiniWriteAiger(fsmFsm, fsmOutName);
    Fsm_FsmFree(fsmFsm);
  }

  Ddi_Free(aigArray);
  Ddi_Free(mapVars);

  App_MgrQuit(appMgr);
  
  return 1;
}

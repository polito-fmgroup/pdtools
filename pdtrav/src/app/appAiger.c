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

  char *aigInName = NULL;
  char *aigOutName = NULL;
  char *aigSymbName = NULL;

  
  printf("aiger app:");
  for (int i=0; i<argc; i++) {
    printf(" %s", argv[i]);
    if (strcmp(argv[i],"-s")==0) {
      i++;
      if (i>=argc) {
        printf("\nmissing file name for aig synbols (-s)\n");
        return 0;
      }
      aigSymbName = argv[i];
    }
    else if (strcmp(argv[i],"-c")==0) {
      complement = 1;
    }
    else if (aigInName==NULL) {
      aigInName = argv[i];
    }
    else if (aigOutName==NULL) {
      aigOutName = argv[i];
    }
    else {
      printf("\nunknown argument/option: %s\n", argv[i]);
    }
  }
  printf("\n");

  if (argc<2) {
    printf("missing arguments - aiger app needs at least two parameters\n");
    printf("<aiger in> [-s <symbol aiger>] <aiger out>\n");
    return 0;
  }

  appMgr = App_MgrInit("aiger", App_TaskAiger_c);

  Ddi_Vararray_t *mapVars = NULL;
  if (aigSymbName != NULL) {
    Fsm_Mgr_t *fsmMgrSymb = Fsm_MgrInit("fsmMgrSymb", appMgr->ddiMgr);
    Fsm_MgrSetUseAig(fsmMgrSymb, 1);
    if (Fsm_MgrLoadAiger(&fsmMgrSymb, appMgr->ddiMgr, aigSymbName, NULL, NULL,
			 Pdtutil_VariableOrderDefault_c) == 1) {
      fprintf(stderr, "-- FSM Loading Error.\n");
      exit(EXIT_FAILURE);
    }
    mapVars = Ddi_VararrayDup(Fsm_MgrReadVarI(fsmMgrSymb));
    Ddi_VararrayAppend(mapVars,Fsm_MgrReadVarPS(fsmMgrSymb));
    //    mapVars = Ddi_VararrayDup(Fsm_MgrReadVarPS(fsmMgrSymb));
    //    Ddi_VararrayAppend(mapVars,Fsm_MgrReadVarI(fsmMgrSymb));
    Fsm_MgrQuit(fsmMgrSymb);
  }

  Ddi_Bddarray_t *aigArray = Ddi_AigarrayNetLoadAigerMapVars(
			      appMgr->ddiMgr,
			      NULL, mapVars, aigInName);
  if (complement) {
    for (int i=0; i<Ddi_BddarrayNum(aigArray); i++)
      Ddi_BddNotAcc(Ddi_BddarrayRead(aigArray,i));
  }
  Ddi_AigarrayNetStoreAiger(aigArray, 0, aigOutName);

  Ddi_Free(aigArray);
  Ddi_Free(mapVars);

  App_MgrQuit(appMgr);
  
  return 1;
}

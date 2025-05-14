/**CFile***********************************************************************

  FileName    [appIso.c]

  PackageName [app]

  Synopsis    [Functions to handle latch mapping based on graph isomorphism]

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
  Synopsis    [Main program of iso app.]
  Description [Main program of iso app.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int App_Iso (
  int argc,
  char *argv[]
) {
  App_Mgr_t *appMgr;
  int iter=1;

  char *fsm1Name = NULL;
  char *fsm2Name = NULL;
  char *mapName = NULL;
  Fsm_Mgr_t *fsm2Mgr = NULL;
  Ddi_Mgr_t *dd2Mgr = NULL;

  printf("iso app:");
  for (int i=0; i<argc; i++) {
    printf(" %s", argv[i]);
    if (strcmp(argv[i],"-h")==0) {
      printf("usage\n");
      printf("iso <model1.aig> <model2.aig> <map-out.aig>\n");
    }
    else if (fsm1Name==NULL) {
      fsm1Name = argv[i];
    }
    else if (fsm2Name==NULL) {
      fsm2Name = argv[i];
    }
    else if (mapName==NULL) {
      mapName = argv[i];
    }
    else {
      printf("\nunknown argument/option: %s\n", argv[i]);
    }
  }
  printf("\n");

  appMgr = App_MgrInit("iso", App_TaskIso_c);

  if (Fsm_MgrLoadAiger(&appMgr->fsmMgr, appMgr->ddiMgr, fsm1Name,
        NULL, NULL,Pdtutil_VariableOrderDefault_c) == 1) {
    fprintf(stderr, "-- FSM Loading Error.\n");
    exit(EXIT_FAILURE);
  }

  dd2Mgr = Ddi_MgrInit("ddi2Mgr", NULL, 0, DDI_UNIQUE_SLOTS,
      DDI_CACHE_SLOTS * 10, 0, -1, -1);
  fsm2Mgr = Fsm_MgrInit("fsm2Mgr", NULL);
  Fsm_MgrSetUseAig(fsm2Mgr, 1);

  if (Fsm_MgrLoadAiger(&fsm2Mgr, dd2Mgr, fsm2Name,
        NULL, NULL,Pdtutil_VariableOrderDefault_c) == 1) {
    fprintf(stderr, "-- FSM Loading Error.\n");
    exit(EXIT_FAILURE);
  }
                        
  Fsm_IsoMapLatches(appMgr->fsmMgr,fsm2Mgr,mapName);

  App_MgrQuit(appMgr);
  Fsm_MgrQuit(fsm2Mgr);
  Ddi_MgrQuit(dd2Mgr);
  
  return 1;
}

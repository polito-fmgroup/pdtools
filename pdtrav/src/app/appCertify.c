/**CFile***********************************************************************

  FileName    [appCertify.c]

  PackageName [app]

  Synopsis    [Functions to handle a proof certification]

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
int App_Certify (
  int argc,
  char *argv[]
) {
  App_Mgr_t *appMgr;
  
  char *fsmName = NULL;
  char *invarName = NULL;
  char *simpInvarName = NULL;

  printf("certify app:");
  for (int i=0; i<argc; i++) {
    printf(" %s", argv[i]);
    if (strcmp(argv[i],"-s")==0) {
      i++;
      if (i>=argc) {
        printf("\nmissing file name for simplification invariant (-s)\n");
        return 0;
      }
      simpInvarName = argv[i];
    }
    else if (fsmName==NULL) {
      fsmName = argv[i];
    }
    else if (invarName==NULL) {
      invarName = argv[i];
    }
    else {
      printf("\nunknown argument/option: %s\n", argv[i]);
    }
  }
  printf("\n");

  if (argc<2) {
    printf("missing arguments - certify app needs at least two parameters\n");
    printf("<model (aiger)> <invar (aiger)>\n");
    return 0;
  }

  appMgr = App_MgrInit("certify", App_TaskCertify_c);
  if (Fsm_MgrLoadAiger(&appMgr->fsmMgr, appMgr->ddiMgr, fsmName, NULL,
                       Pdtutil_VariableOrderDefault_c) == 1) {
    fprintf(stderr, "-- FSM Loading Error.\n");
    exit(EXIT_FAILURE);
  }
  Ddi_BddNotAcc(Ddi_BddarrayRead(Fsm_MgrReadLambdaBDD(appMgr->fsmMgr),0));

  appMgr->travMgr = Trav_MgrInit(fsmName, appMgr->ddiMgr);
  //  FbvSetTravMgrOpt(travMgr, opt);
  //  FbvSetTravMgrFsmData(appMgr->travMgr, appMgr->fsmMgr);

  Ddi_Bddarray_t *invarArray = Ddi_AigarrayNetLoadAiger(appMgr->ddiMgr,
                           NULL, invarName);
  if (simpInvarName != NULL) {
    Ddi_Vararray_t *vars = Ddi_VararrayDup(Fsm_MgrReadVarI(appMgr->fsmMgr));
    Ddi_VararrayAppend(vars,Fsm_MgrReadVarPS(appMgr->fsmMgr));
    Ddi_Bddarray_t *extraInvarArray = Ddi_AigarrayNetLoadAigerMapVars(appMgr->ddiMgr,
                                                          NULL, vars, simpInvarName);
    Ddi_BddarrayAppend(invarArray,extraInvarArray);
    Ddi_Free(extraInvarArray);
    Ddi_Free(vars);
  }

  Pdtutil_Assert(Ddi_BddarrayNum(invarArray)==1,"problem with invar");
  Ddi_Bdd_t *myInvar = Ddi_BddMakePartConjFromArray(invarArray);
  Ddi_BddSetAig(myInvar);
  Ddi_Free(invarArray);
  int chk, fp;
  fp = Trav_TravSatCheckInvar(appMgr->travMgr,appMgr->fsmMgr,
                                  myInvar,&chk);
  Ddi_Free(myInvar);

  App_MgrQuit(appMgr);
  
  return chk;
}

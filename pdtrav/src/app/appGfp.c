/**CFile***********************************************************************

  FileName    [appGfp.c]

  PackageName [app]

  Synopsis    [Functions to handle GFP-based invar strengthening]

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
  Synopsis    [Main program of gfp app.]
  Description [Main program of gfp app.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int App_Gfp (
  int argc,
  char *argv[]
) {
  App_Mgr_t *appMgr;
  int iter=1;

  char *aigInName = NULL;
  char *aigOutName = NULL;
  char *fsmName = NULL;
  int doStrengthen = 1; 
  
  printf("gfp app:");
  for (int i=0; i<argc; i++) {
    printf(" %s", argv[i]);
    if (strcmp(argv[i],"-h")==0) {
      printf("usage\n");
      printf("gfp [-i <num passes>] <model.aig> <inv-in.aig> inv-out.aig>\n");
    }
    else if (strcmp(argv[i],"-i")==0) {
      i++;
      if (i>=argc) {
        printf("\nmissing integer num of gfp passes (-i)\n");
        return 0;
      }
      iter = atoi(argv[i]);
    }
    else if (strcmp(argv[i],"-w")==0) {
      doStrengthen = 0;
    }
    else if (fsmName==NULL) {
      fsmName = argv[i];
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

  appMgr = App_MgrInit("gfp", App_TaskGfp_c);

  if (Fsm_MgrLoadAiger(&appMgr->fsmMgr, appMgr->ddiMgr, fsmName,
        NULL, NULL,Pdtutil_VariableOrderDefault_c) == 1) {
    fprintf(stderr, "-- FSM Loading Error.\n");
    exit(EXIT_FAILURE);
  }
  appMgr->travMgr = Trav_MgrInit(fsmName, appMgr->ddiMgr);
  Ddi_Bdd_t *invarspec =
    Ddi_BddNot(Ddi_BddarrayRead(Fsm_MgrReadLambdaBDD(appMgr->fsmMgr),0));
  Fsm_MgrSetInvarspecBDD(appMgr->fsmMgr, invarspec);
  Fsm_MgrFold(appMgr->fsmMgr);
                        
  Ddi_Bddarray_t *
    invarArray = Ddi_AigarrayNetLoadAiger(
                   appMgr->ddiMgr, NULL, aigInName);
  Ddi_Bdd_t *myInvar = Ddi_BddMakePartConjFromArray(invarArray);
  Ddi_BddSetAig(myInvar);
  Ddi_Free(invarArray);
  Trav_MgrSetReached(appMgr->travMgr,myInvar);
  Ddi_Free(myInvar);
  Trav_TravSatItpGfp(appMgr->travMgr,appMgr->fsmMgr,iter,doStrengthen,0);
  Ddi_Bdd_t *invOut = Fsm_MgrReadReachedBDD(appMgr->fsmMgr);
  Ddi_Var_t *cvarPs = Ddi_VarFromName(appMgr->ddiMgr,
                                      "PDT_BDD_INVAR_VAR$PS");
  Ddi_BddCofactorAcc(invOut,cvarPs,1);
  Ddi_Var_t *pvarPs = Ddi_VarFromName(appMgr->ddiMgr,
                                      "PDT_BDD_INVARSPEC_VAR$PS");
  Ddi_BddCofactorAcc(invOut,pvarPs,1);

  Ddi_AigNetStoreAiger(invOut, 0, aigOutName);

  Ddi_Free(invarspec);
  App_MgrQuit(appMgr);
  
  return 1;
}

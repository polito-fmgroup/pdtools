/**CFile***********************************************************************

  FileName    [appMgr.c]

  PackageName [mc]

  Synopsis    [Functions to handle an APP Manager]

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

  Synopsis    [Creates a MC Manager.]

  Description []

  SideEffects [none]

  SeeAlso     [Ddi_MgrInit, Fsm_MgrInit, Trav_MgrInit]

******************************************************************************/

App_Mgr_t *
App_MgrInit(
  char *appName /* Name of the APP */,
  App_TaskSelection_e task
)
{
  App_Mgr_t *appMgr;

  appMgr = Pdtutil_Alloc(App_Mgr_t, 1);
  Pdtutil_Assert(appMgr != NULL, "Out of memory.");

  /* app specific settings */
  appMgr->settings.verbosity = Pdtutil_VerbLevelUsrMax_c;

  Pdtutil_VerbosityMgr(appMgr,
    Pdtutil_VerbLevelUsrMax_c, fprintf(stdout, "-- App Manager Init.\n")
    );

  appMgr->appName = Pdtutil_StrDup(appName);

  appMgr->ddiMgr = Ddi_MgrInit("ddiMgr", NULL, 0, DDI_UNIQUE_SLOTS,
      DDI_CACHE_SLOTS * 10, 0, -1, -1);
  Ddi_MgrSetOption(appMgr->ddiMgr, Pdt_DdiItpDrup_c, inum, 3);
  appMgr->fsmMgr = Fsm_MgrInit("fsmMgr", NULL);

  Fsm_MgrSetDdManager(appMgr->fsmMgr, appMgr->ddiMgr);
  Fsm_MgrSetUseAig(appMgr->fsmMgr, 1);

  appMgr->travMgr = NULL;
  appMgr->trMgr = NULL;
  

  appMgr->task = task;

  appMgr->auxPtr = NULL;

  return appMgr;
}

/**Function********************************************************************

  Synopsis     [Closes a Model Checking Manager.]

  Description  [Closes a Model Checking Manager freeing all the
    correlated fields.]

  SideEffects  [none]

  SeeAlso      [Ddi_BddiMgrInit]

******************************************************************************/

void
App_MgrQuit(
  App_Mgr_t * appMgr
)
{
  if (appMgr == NULL) {
    return;
  }

  Pdtutil_VerbosityMgr(appMgr,
    Pdtutil_VerbLevelUsrMax_c, fprintf(stdout, "-- App Manager Quit.\n")
    );

  Pdtutil_Free(appMgr->appName);
  if (appMgr->travMgr!=NULL)
    Trav_MgrQuit(appMgr->travMgr);
  Fsm_MgrQuit(appMgr->fsmMgr);
  Ddi_MgrQuit(appMgr->ddiMgr);

  Pdtutil_Free(appMgr);
}


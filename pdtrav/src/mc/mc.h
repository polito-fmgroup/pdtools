/**CHeaderFile*****************************************************************

  FileName    [mc.h]

  PackageName [Mc]

  Synopsis    [Funtions to handle model checking tasks]

  Description [
    ]

  SeeAlso     []  

  Author    [Gianpiero Cabodi]

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

#ifndef _MC
#define _MC

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdarg.h>
#include "pdtutil.h"
#include "ddi.h"
#include "trav.h"
#include "tr.h"
#include "fsm.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

typedef struct Mc_Mgr_s Mc_Mgr_t;

typedef enum {
  Mc_TaskBmc_c,
  Mc_TaskBmc2_c,
  Mc_TaskBmcb_c,
  Mc_TaskBdd_c,
  Mc_TaskBdd2_c,
  Mc_TaskItp_c,
  Mc_TaskItp2_c,
  Mc_TaskItpl_c,
  Mc_TaskIgr_c,
  Mc_TaskIgr2_c,
  Mc_TaskIgrPdr_c,
  Mc_TaskLms_c,
  Mc_TaskPdr_c,
  Mc_TaskPdr2_c,
  Mc_TaskPdrs_c,
  Mc_TaskSim_c,
  Mc_TaskSyn_c,
  Mc_TaskBddPfl_c,
  Mc_TaskBmcPfl_c,
  Mc_TaskItpPfl_c,
  Mc_TaskPdrPfl_c,
  Mc_TaskNone_c
} Mc_TaskSelection_e;

typedef enum {
  Mc_VerifABCLink_c,
  Mc_VerifTravFwd_c,
  Mc_VerifTravBwd_c,
  Mc_VerifExactFwd_c,
  Mc_VerifExactBck_c,
  Mc_VerifApproxBckExactFwd_c,
  Mc_VerifApproxFwdExactBck_c,
  Mc_VerifApproxFwdApproxBckExactFwd_c,
  Mc_VerifPrintNtkStats_c,
  Mc_VerifMethodNone_c
} Mc_VerifMethod_e;


/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**Macro***********************************************************************
  Synopsis    [Wrapper for setting an option value]
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/

#define Mc_MgrSetOption(mgr,tag,dfield,data) {	\
  Pdtutil_OptItem_t optItem;\
  optItem.optTag.eMcOpt = tag;\
  optItem.optData.dfield = data;\
  Mc_MgrSetOptionItem(mgr, optItem);\
}

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EXTERN Mc_Mgr_t *Mc_MgrInit(
  char *mcName,
  Fsm_Mgr_t * fsmMgr
);
EXTERN void Mc_MgrQuit(
  Mc_Mgr_t * McMgr
);
EXTERN void Mc_MgrSetMcName(
  Mc_Mgr_t * mcMgr,
  char *mcName
);
EXTERN char *Mc_MgrReadMcName(
  Mc_Mgr_t * mcMgr
);
EXTERN int Mc_McRun(
  Mc_Mgr_t * mcMgr,
  Trav_Xchg_t **xchgP
);
EXTERN void Mc_MgrSetMgrOptList(
  Mc_Mgr_t * mcMgr,
  Pdtutil_OptList_t * optList
);
EXTERN void Mc_MgrSetOptionList(
  Mc_Mgr_t * mcMgr,
  Pdtutil_OptList_t * optList
);
EXTERN void Mc_MgrSetOptionItem(
  Mc_Mgr_t * mcMgr,
  Pdtutil_OptItem_t optItem
);
EXTERN void Mc_MgrReadOption(
  Mc_Mgr_t * mcMgr,
  Pdt_OptTagMc_e mcOpt,
  void *optRet
);

/**AutomaticEnd***************************************************************/


#endif                          /* _MC */

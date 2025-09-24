/**CHeaderFile*****************************************************************

  FileName    [app.h]

  PackageName [App]

  Synopsis    [Functions to handle pdtrav apps as single programs]

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
    Send bug-reports and/or questions to: gianpiero.cabodi@polito.it. ]
  
  Revision    []

******************************************************************************/

#ifndef _APP
#define _APP

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

typedef struct App_Mgr_s App_Mgr_t;

typedef enum {
  App_TaskAiger_c,
  App_TaskCertify_c,
  App_TaskGfp_c,
  App_TaskIso_c,
  App_TaskNone_c
} App_TaskSelection_e;

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

App_Mgr_t *
App_MgrInit(
  char *appName /* Name of the APP */,
  App_TaskSelection_e task
);
EXTERN void
App_MgrQuit(
  App_Mgr_t * appMgr
);
EXTERN int App_Aiger (
  int argc,
  char *argv[]
);
EXTERN int App_Certify (
  int argc,
  char *argv[]
);
EXTERN int App_Gfp (
  int argc,
  char *argv[]
);
EXTERN int App_Iso (
  int argc,
  char *argv[]
);

/**AutomaticEnd***************************************************************/


#endif                          /* _APP */

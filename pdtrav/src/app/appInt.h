/**CHeaderFile*****************************************************************

  FileName    [appInt.h]

  PackageName [Mc]

  Synopsis    [Internal header file]

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

#ifndef _APPINT
#define _APPINT

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include "ddi.h"
#include "fsm.h"
#include "tr.h"
#include "trav.h"
#include "app.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

typedef int (* AppMainFunc_t) (int, char **);

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                     */
/*---------------------------------------------------------------------------*/

/**Struct*********************************************************************

 Synopsis    [App Manager]

 Description [The app manager structure]

******************************************************************************/

struct App_Mgr_s {
  char *appName;
  App_TaskSelection_e task;

  Ddi_Mgr_t *ddiMgr;
  Fsm_Mgr_t *fsmMgr;
  Trav_Mgr_t *travMgr;
  Tr_Mgr_t *trMgr;

  struct {
    long appTime;
  } stats;

  struct {
    Pdtutil_VerbLevel_e verbosity;
  } settings;
    
  union {
    struct {
    } certify;
    struct {
    } none;
  } app;
  
  // Pdtutil_OptList_t *optList;
  void *auxPtr;

};

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
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/


/**AutomaticEnd***************************************************************/


#endif                          /* _MCINT */

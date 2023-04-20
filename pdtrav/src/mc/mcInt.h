/**CHeaderFile*****************************************************************

  FileName    [mcInt.h]

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

#ifndef _MCINT
#define _MCINT

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include "ddi.h"
#include "fsm.h"
#include "tr.h"
#include "trav.h"
#include "mc.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

typedef struct McSettings Mc_Settings_t;

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                     */
/*---------------------------------------------------------------------------*/

struct McSettings {
  Pdtutil_VerbLevel_e verbosity;
  Mc_TaskSelection_e task;
  Mc_VerifMethod_e method;

  int pdr;
  int aig;
  int inductive;
  int interpolant;
  int custom;
  int lemmas;
  int lazy;
  float lazyRate;
  int qbf;
  int diameter;
  int checkMult;
  int exactTrav;

  int bmc;
  int bmcStrategy;
  int interpolantBmcSteps;
  int bmcTr;
  int bmcStep;
  int bmcTe;
  int bmcLearnStep;
  int bmcFirst;

  int itpSeq;
  int itpSeqGroup;
  float itpSeqSerial;
  int initBound;
  int enFastRun;
  int fwdBwdFP;


  /* questi non hanno neanche le costanti */
  int bddNodeLimit;
  int bddMemoryLimit;
  int bddTimeLimit;
  int itpBoundLimit;
  int bddPeakNodes;
  int pdrTimeLimit;
  int itpTimeLimit;
  int itpMemoryLimit;
  int bmcTimeLimit;
  int bmcMemoryLimit;
  int indTimeLimit;
};

/**Struct*********************************************************************

 Synopsis    [Model Checking Manager]

 Description [The Transition Relation manager structure]

******************************************************************************/

struct Mc_Mgr_s {
  char *mcName;

  Ddi_Mgr_t *ddiMgr;
  Fsm_Mgr_t *fsmMgr;
  Trav_Mgr_t *travMgr;
  Tr_Mgr_t *trMgr;

  struct {
    long mcTime;
  } stats;

  Mc_Settings_t settings;
  Pdtutil_OptList_t *optList;

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

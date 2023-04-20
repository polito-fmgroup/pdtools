/**CHeaderFile*****************************************************************

  FileName    [cuplus.h]

  PackageName [cuplus] 

  Synopsis    [Extensions of the CU package]

  Description []

  SeeAlso     []  

  Author    [Gianpiero Cabodi and Stefano Quer]

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

#ifndef _CUPLUS
#define _CUPLUS

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include "cudd.h"
#include "st.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EXTERN
#   ifdef __cplusplus
#       define EXTERN extern "C"
#   else
#       define EXTERN extern
#   endif
#endif 

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Structure & type declarations                                             */
/*---------------------------------------------------------------------------*/

/* only accessed by pointer */
typedef struct profile_info_summary cuplus_profile_info_t; 
typedef struct profile_info_item2 profile_info_item2_t;

typedef struct cuplus_and_abs_info {


  int Cuplus_EnableAndAbs;

#if DEVEL_OPT_ANDEXIST
  Ddi_Varset_t *Cuplus_AuxVarset;
  DdNode *Cuplus_AuxVars[10000];
  int Cuplus_IsAux[10000];
  int Cuplus_DisabledAux[10000];
#endif

  //  int absWithNodeIncr[10000];
  //  int absWithNodeDecr[10000];
  int newvarSizeIncr;
  int newvarSizeRepl;
  int keepvarSizeIncr;
  int newvarSizeNum;
  int keepvarSizeNum;

  int *elseFirst;
  int elseFirstSize;
  int initElseFirst;

} cuplus_and_abs_info_t;


/**Enum************************************************************************

  Synopsis    [Type for selecting pruning heuristics.]

  Description []

******************************************************************************/

typedef enum
  {
  Cuplus_ActiveRecursions,
  Cuplus_Recursions,
  Cuplus_RecursionsWithSharing,
  Cuplus_NormSizePrune,
  Cuplus_NormSizePruneLight,
  Cuplus_NormSizePruneHeavy,
  Cuplus_SizePrune,
  Cuplus_SizePruneLight,
  Cuplus_SizePruneHeavy,
  Cuplus_SizePruneLightNoRecur,
  Cuplus_SizePruneHeavyNoRecur,
  Cuplus_None
  }
Cuplus_PruneHeuristic_e;

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

extern cuplus_and_abs_info_t *Cuplus_bddAndAbstractInfoInit();
extern void Cuplus_bddAndAbstractInfoFree(cuplus_and_abs_info_t *infoPtr);
  extern DdNode * Cuplus_bddAndAbstract(DdManager * manager, DdNode * f, DdNode * g, DdNode * cube, int *abortFrontier, cuplus_and_abs_info_t *infoPtr);
extern DdNode * Cuplus_bddAndAbstractProject(DdManager * manager, DdNode * f, DdNode ** g, int ng, DdNode * cube);
extern DdNode * Cuplus_bddExistAbstractWeak(DdManager * manager, DdNode * f, DdNode * cube);
extern DdNode * Cuplus_bddExistAbstractAux(DdManager * manager, DdNode * f, DdNode * cube);
extern DdNode * Cuplus_bddExistAbstract(DdManager * manager, DdNode * f, DdNode * cube);
extern DdNode * Cuplus_CofexistMask(DdManager *manager, DdNode *f, DdNode *cube);
extern void Cuplus_bddOpEnableAbortOnSift(DdManager * manager);
extern void Cuplus_bddOpDisableAbortOnSift(DdManager * manager);
extern int Cuplus_DoAbortHookFun(DdManager * manager, const char *str, void *heuristic);
extern DdNode * Cuplus_ProfileAndAbstract(DdManager *manager, DdNode *f, DdNode *g, DdNode *cube, cuplus_profile_info_t *profile);
extern void Cuplus_ProfileSetCurrentPart(cuplus_profile_info_t *profile, int currentPart);
extern int Cuplus_ProfileReadCurrentPart(cuplus_profile_info_t *profile);
extern DdNode * Cuplus_ProfilePrune(DdManager *manager, DdNode *f, cuplus_profile_info_t *profile, Cuplus_PruneHeuristic_e pruneHeuristic, int threshold);
extern cuplus_profile_info_t * Cuplus_ProfileInfoGen(unsigned char dac99Compatible, int nVar, int nPart);
extern void Cuplus_ProfileInfoFree(cuplus_profile_info_t *profile);
extern void Cuplus_ProfileInfoPrint(cuplus_profile_info_t *profile);
extern void Cuplus_ProfileInfoPrintOriginal(cuplus_profile_info_t *profile);
extern void Cuplus_ProfileInfoPrintByVar(cuplus_profile_info_t *profile);

/**AutomaticEnd***************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _CUPLUS */

 






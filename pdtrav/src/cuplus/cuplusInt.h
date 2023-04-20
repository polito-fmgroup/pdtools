/**CHeaderFile*****************************************************************

  FileName    [cuplusInt.h]

  PackageName [cuplus] 

  Synopsis    []

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

#ifndef _CUPLUSINT
#define _CUPLUSINT

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include "util.h"
#include "st.h"
#include "cuddInt.h"
#include "cuplus.h"
#include "pdtutil.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Structure & type declarations                                             */
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

extern DdNode * cuplusBddExistAbstractRecur(DdManager * manager, DdNode * f, DdNode * cube);
extern DdNode * cuplusBddExistAbstractWeakRecur(DdManager * manager, DdNode * f, DdNode * cube);
extern DdNode * cuplusBddExistAbstractAuxRecur(DdManager * manager, DdNode * f, DdNode * cube);
extern DdNode * cuplusBddAndAbstractRecur(DdManager * manager, DdNode * f, DdNode * g, DdNode * cube);
extern DdNode * cuplusBddAndAbstractRecur2(DdManager * manager, DdNode * f, DdNode * g, DdNode * cube, int *abortFrontier, int *elseFirst);

/**AutomaticEnd***************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _CUPLUSINT */

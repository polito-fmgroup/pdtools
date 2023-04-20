/**CFile***********************************************************************

  FileName    [ddiAig.c]

  PackageName [ddi]

  Synopsis    [Functions working on AIGs]

  Description [Functions working on AIGs]

  SeeAlso   []  

  Author    [Gianpiero Cabodi]

  Copyright [This file was created at the Politecnico di Torino,
    Torino, Italy. 
    The  Politecnico di Torino makes no warranty about the suitability of 
    this software for any purpose.  
    It is presented on an AS IS basis.
  ]
  
  Revision  []

******************************************************************************/

#include "ddiInt.h"
#include "baigInt.h"
#include "tr.h"
#include "fsm.h"

#define __STDC_LIMIT_MACROS
#define __STDC_FORMAT_MACROS
#include <stdint.h>

#include <time.h>
#include "minisat22/core/Solver.h"

using namespace Minisat;


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/


/*----------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#define dMgrO(ddiMgr) ((ddiMgr)->settings.stdout)



/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/


/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/



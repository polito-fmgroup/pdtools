/**CFile****************************************************************

  FileName    [utilInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Memory recycling utilities.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: utilMem.h,v 1.1.1.1 2021/09/11 09:57:46 cabodi Exp $]

***********************************************************************/
 
#ifndef ABC__misc__util__utilMem_h
#define ABC__misc__util__utilMem_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


extern void * s_vAllocs;   // storage of allocated pointers
extern void * s_vFrees;    // storage of deallocated pointers
extern int s_fInterrupt;   // set to 1 when it is time to backout

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== utilMem.c ==========================================================*/
extern void *       Util_MemRecAlloc( void * pMem );
extern void *       Util_MemRecFree( void * pMem );
extern void         Util_MemStart();
extern void         Util_MemQuit();
extern void         Util_MemRecycle();
extern int          Util_MemRecIsSet();



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


/**CFile****************************************************************

  FileName    [super.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Pre-computation of supergates (delay-limited gate combinations).]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: super.h,v 1.1.1.1 2021/09/11 09:57:47 cabodi Exp $]

***********************************************************************/

#ifndef ABC__map__super__super_h
#define ABC__map__super__super_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////
 
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== superAnd.c =============================================================*/
extern void        Super2_Precompute( int nInputs, int nLevels, int fVerbose );
/*=== superGate.c =============================================================*/
extern Vec_Str_t * Super_PrecomputeStr( Mio_Library_t * pLibGen, int nVarsMax, int nLevels, int nGatesMax, float tDelayMax, float tAreaMax, int TimeLimit, int fSkipInv, int fVerbose );
extern void        Super_Precompute( Mio_Library_t * pLibGen, int nVarsMax, int nLevels, int nGatesMax, float tDelayMax, float tAreaMax, int TimeLimit, int fSkipInv, int fVerbose, char * pFileName );


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

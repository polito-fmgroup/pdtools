/**CFile****************************************************************

  FileName    [mfs.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [The good old minimization with complete don't-cares.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: mfs.h,v 1.1.1.1 2021/09/11 09:57:46 cabodi Exp $]

***********************************************************************/
 
#ifndef ABC__opt__mfs__mfs_h
#define ABC__opt__mfs__mfs_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Mfs_Par_t_ Mfs_Par_t;
struct Mfs_Par_t_
{
    // general parameters
    int           nWinTfoLevs;   // the maximum fanout levels
    int           nFanoutsMax;   // the maximum number of fanouts
    int           nDepthMax;     // the maximum number of logic levels
    int           nDivMax;       // the maximum number of divisors
    int           nWinSizeMax;   // the maximum size of the window
    int           nGrowthLevel;  // the maximum allowed growth in level
    int           nBTLimit;      // the maximum number of conflicts in one SAT run
    int           fResub;        // performs resubstitution
    int           fArea;         // performs optimization for area
    int           fMoreEffort;   // performs high-affort minimization
    int           fSwapEdge;     // performs edge swapping
    int           fOneHotness;   // adds one-hotness conditions
    int           fDelay;        // performs optimization for delay
    int           fPower;        // performs power-aware optimization
    int           fGiaSat;       // use new SAT solver
    int           fVerbose;      // enable basic stats
    int           fVeryVerbose;  // enable detailed stats
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== mfsCore.c ==========================================================*/
extern void        Abc_NtkMfsParsDefault( Mfs_Par_t * pPars );
extern int         Abc_NtkMfs( Abc_Ntk_t * pNtk, Mfs_Par_t * pPars );

 


ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


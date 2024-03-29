/**CFile****************************************************************

  FileName    [simSym.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Simulation to determine two-variable symmetries.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: simSym.c,v 1.1.1.1 2021/09/11 09:57:46 cabodi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "sim.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes two variable symmetries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_ComputeTwoVarSymms( Abc_Ntk_t * pNtk, int fVerbose )
{
    Sym_Man_t * p;
    Vec_Ptr_t * vResult;
    int Result;
    int i;
    clock_t clk, clkTotal = clock();

    srand( 0xABC );

    // start the simulation manager
    p = Sym_ManStart( pNtk, fVerbose );
    p->nPairsTotal = p->nPairsRem = Sim_UtilCountAllPairs( p->vSuppFun, p->nSimWords, p->vPairsTotal );
    if ( fVerbose )
        printf( "Total = %8d.  Sym = %8d.  NonSym = %8d.  Remaining = %8d.\n", 
               p->nPairsTotal, p->nPairsSymm, p->nPairsNonSymm, p->nPairsRem );

    // detect symmetries using circuit structure
clk = clock();
    Sim_SymmsStructCompute( pNtk, p->vMatrSymms, p->vSuppFun );
p->timeStruct = clock() - clk;

    Sim_UtilCountPairsAll( p );
    p->nPairsSymmStr = p->nPairsSymm;
    if ( fVerbose )
        printf( "Total = %8d.  Sym = %8d.  NonSym = %8d.  Remaining = %8d.\n", 
            p->nPairsTotal, p->nPairsSymm, p->nPairsNonSymm, p->nPairsRem );

    // detect symmetries using simulation
    for ( i = 1; i <= 1000; i++ )
    {
        // simulate this pattern
        Sim_UtilSetRandom( p->uPatRand, p->nSimWords );
        Sim_SymmsSimulate( p, p->uPatRand, p->vMatrNonSymms );
        if ( i % 50 != 0 )
            continue;
        // check disjointness
        assert( Sim_UtilMatrsAreDisjoint( p ) );
        // count the number of pairs
        Sim_UtilCountPairsAll( p );
        if ( i % 500 != 0 )
            continue;
        if ( fVerbose )
            printf( "Total = %8d.  Sym = %8d.  NonSym = %8d.  Remaining = %8d.\n", 
                p->nPairsTotal, p->nPairsSymm, p->nPairsNonSymm, p->nPairsRem );
    }

    // detect symmetries using SAT
    for ( i = 1; Sim_SymmsGetPatternUsingSat( p, p->uPatRand ); i++ )
    {
        // simulate this pattern in four polarities
        Sim_SymmsSimulate( p, p->uPatRand, p->vMatrNonSymms );
        Sim_XorBit( p->uPatRand, p->iVar1 );
        Sim_SymmsSimulate( p, p->uPatRand, p->vMatrNonSymms );
        Sim_XorBit( p->uPatRand, p->iVar2 );
        Sim_SymmsSimulate( p, p->uPatRand, p->vMatrNonSymms );
        Sim_XorBit( p->uPatRand, p->iVar1 );
        Sim_SymmsSimulate( p, p->uPatRand, p->vMatrNonSymms );
        Sim_XorBit( p->uPatRand, p->iVar2 );
/*
        // try the previuos pair
        Sim_XorBit( p->uPatRand, p->iVar1Old );
        Sim_SymmsSimulate( p, p->uPatRand, p->vMatrNonSymms );
        Sim_XorBit( p->uPatRand, p->iVar2Old );
        Sim_SymmsSimulate( p, p->uPatRand, p->vMatrNonSymms );
        Sim_XorBit( p->uPatRand, p->iVar1Old );
        Sim_SymmsSimulate( p, p->uPatRand, p->vMatrNonSymms );
*/
        if ( i % 10 != 0 )
            continue;
        // check disjointness
        assert( Sim_UtilMatrsAreDisjoint( p ) );
        // count the number of pairs
        Sim_UtilCountPairsAll( p );
        if ( i % 50 != 0 )
            continue;
        if ( fVerbose )
            printf( "Total = %8d.  Sym = %8d.  NonSym = %8d.  Remaining = %8d.\n", 
                p->nPairsTotal, p->nPairsSymm, p->nPairsNonSymm, p->nPairsRem );
    }

    // count the number of pairs
    Sim_UtilCountPairsAll( p );
    if ( fVerbose )
        printf( "Total = %8d.  Sym = %8d.  NonSym = %8d.  Remaining = %8d.\n", 
            p->nPairsTotal, p->nPairsSymm, p->nPairsNonSymm, p->nPairsRem );
//    Sim_UtilCountPairsAllPrint( p );

    Result = p->nPairsSymm;
    vResult = p->vMatrSymms;  
p->timeTotal = clock() - clkTotal;
    //  p->vMatrSymms = NULL;
    Sym_ManStop( p );
    return Result;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END


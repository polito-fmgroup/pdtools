/**CFile****************************************************************

  FileName    [ifCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [The central part of the mapper.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifCore.c,v 1.1.1.1 2021/09/11 09:57:47 cabodi Exp $]

***********************************************************************/

#include "if.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern clock_t s_MappingTime;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManPerformMapping( If_Man_t * p )
{
    p->pPars->fAreaOnly = p->pPars->fArea; // temporary

    // create the CI cutsets
    If_ManSetupCiCutSets( p );
    // allocate memory for other cutsets
    If_ManSetupSetAll( p, If_ManCrossCut(p) );
    // derive reverse top order
    p->vObjsRev = If_ManReverseOrder( p );

    // try sequential mapping
    if ( p->pPars->fSeqMap )
    {
//        if ( p->pPars->fVerbose )
            Abc_Print( 1, "Performing sequential mapping without retiming.\n" );
        return If_ManPerformMappingSeq( p );
    }
    return If_ManPerformMappingComb( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManPerformMappingComb( If_Man_t * p )
{
    If_Obj_t * pObj;
    clock_t clkTotal = clock();
    int i;

    // set arrival times and fanout estimates
    If_ManForEachCi( p, pObj, i )
    {
        If_ObjSetArrTime( pObj, p->pPars->pTimesArr ? p->pPars->pTimesArr[i] : (float)0.0 );
        pObj->EstRefs = (float)1.0;
    }

    // delay oriented mapping
    if ( p->pPars->fPreprocess && !p->pPars->fArea )
    {
        // map for delay
        If_ManPerformMappingRound( p, p->pPars->nCutsMax, 0, 1, "Delay" );
        // map for delay second option
        p->pPars->fFancy = 1;
        If_ManResetOriginalRefs( p );
        If_ManPerformMappingRound( p, p->pPars->nCutsMax, 0, 1, "Delay-2" );
        p->pPars->fFancy = 0;
        // map for area
        p->pPars->fArea = 1;
        If_ManResetOriginalRefs( p );
        If_ManPerformMappingRound( p, p->pPars->nCutsMax, 0, 1, "Area" );
        p->pPars->fArea = 0;
    }
    else
        If_ManPerformMappingRound( p, p->pPars->nCutsMax, 0, 0, "Delay" );

    // try to improve area by expanding and reducing the cuts
    if ( p->pPars->fExpRed )
        If_ManImproveMapping( p );

    // area flow oriented mapping
    for ( i = 0; i < p->pPars->nFlowIters; i++ )
    {
        If_ManPerformMappingRound( p, p->pPars->nCutsMax, 1, 0, "Flow" );
        if ( p->pPars->fExpRed )
            If_ManImproveMapping( p );
    }

    // area oriented mapping
    for ( i = 0; i < p->pPars->nAreaIters; i++ )
    {
        If_ManPerformMappingRound( p, p->pPars->nCutsMax, 2, 0, "Area" );
        if ( p->pPars->fExpRed )
            If_ManImproveMapping( p );
    }

    if ( p->pPars->fVerbose )
    {
//        Abc_Print( 1, "Total memory = %7.2f MB. Peak cut memory = %7.2f MB.  ", 
//            1.0 * (p->nObjBytes + 2*sizeof(void *)) * If_ManObjNum(p) / (1<<20), 
//            1.0 * p->nSetBytes * Mem_FixedReadMaxEntriesUsed(p->pMemSet) / (1<<20) );
        Abc_PrintTime( 1, "Total time", clock() - clkTotal );
    }
//    Abc_Print( 1, "Cross cut memory = %d.\n", Mem_FixedReadMaxEntriesUsed(p->pMemSet) );
    s_MappingTime = clock() - clkTotal;
//    Abc_Print( 1, "Special POs = %d.\n", If_ManCountSpecialPos(p) );

/*
    {
        static char * pLastName = NULL;
        FILE * pTable = fopen( "fpga/ucsb/stats.txt", "a+" );
        if ( pLastName == NULL || strcmp(pLastName, p->pName) )
        {
            fprintf( pTable, "\n" );
            fprintf( pTable, "%s ", p->pName );

            fprintf( pTable, "%d ", If_ManCiNum(p) );
            fprintf( pTable, "%d ", If_ManCoNum(p) );
            fprintf( pTable, "%d ", If_ManAndNum(p) );

            ABC_FREE( pLastName );
            pLastName = Abc_UtilStrsav( p->pName );
        }

        fprintf( pTable, "%d ", (int)p->AreaGlo );
        fprintf( pTable, "%d ", (int)p->RequiredGlo );
        fclose( pTable );
    }
*/
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END


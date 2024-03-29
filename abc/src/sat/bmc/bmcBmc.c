/**CFile****************************************************************

  FileName    [bmcBmc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [Simple BMC package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcBmc.c,v 1.1.1.1 2021/09/11 09:57:46 cabodi Exp $]

***********************************************************************/

#include "proof/fra/fra.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satStore.h"
#include "bmc.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Create timeframes of the manager for BMC.]

  Description [The resulting manager is combinational. POs correspond to \
  the property outputs in each time-frame.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManFramesBmc( Aig_Man_t * pAig, int nFrames )
{
    Aig_Man_t * pFrames;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i, f;
    assert( Saig_ManRegNum(pAig) > 0 );
    pFrames = Aig_ManStart( Aig_ManNodeNum(pAig) * nFrames );
    // map the constant node
    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pFrames );
    // create variables for register outputs
    Saig_ManForEachLo( pAig, pObj, i )
        pObj->pData = Aig_ManConst0( pFrames );
    // add timeframes
    for ( f = 0; f < nFrames; f++ )
    {
        // create PI nodes for this frame
        Saig_ManForEachPi( pAig, pObj, i )
            pObj->pData = Aig_ObjCreateCi( pFrames );
        // add internal nodes of this frame
        Aig_ManForEachNode( pAig, pObj, i )
            pObj->pData = Aig_And( pFrames, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
        // create POs for this frame
        Saig_ManForEachPo( pAig, pObj, i )
            Aig_ObjCreateCo( pFrames, Aig_ObjChild0Copy(pObj) );
        if ( f == nFrames - 1 )
            break;
        // save register inputs
        Saig_ManForEachLi( pAig, pObj, i )
            pObj->pData = Aig_ObjChild0Copy(pObj);
        // transfer to register outputs
        Saig_ManForEachLiLo(  pAig, pObjLi, pObjLo, i )
            pObjLo->pData = pObjLi->pData;
    }
    Aig_ManCleanup( pFrames );
    return pFrames;
}

/**Function*************************************************************

  Synopsis    [Returns the number of internal nodes that are not counted yet.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManFramesCount_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    if ( !Aig_ObjIsNode(pObj) )
        return 0;
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return 0;
    Aig_ObjSetTravIdCurrent(p, pObj);
    return 1 + Saig_ManFramesCount_rec( p, Aig_ObjFanin0(pObj) ) + 
        Saig_ManFramesCount_rec( p, Aig_ObjFanin1(pObj) );
}

/**Function*************************************************************

  Synopsis    [Create timeframes of the manager for BMC.]

  Description [The resulting manager is combinational. POs correspond to 
  the property outputs in each time-frame.
  The unrolling is stopped as soon as the number of nodes in the frames
  exceeds the given maximum size.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManFramesBmcLimit( Aig_Man_t * pAig, int nFrames, int nSizeMax )
{
    Aig_Man_t * pFrames;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo, * pObjPo;
    int i, f, Counter = 0;
    assert( Saig_ManRegNum(pAig) > 0 );
    pFrames = Aig_ManStart( nSizeMax );
    Aig_ManIncrementTravId( pFrames );
    // map the constant node
    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pFrames );
    // create variables for register outputs
    Saig_ManForEachLo( pAig, pObj, i )
        pObj->pData = Aig_ManConst0( pFrames );
    // add timeframes
    Counter = 0;
    for ( f = 0; f < nFrames; f++ )
    {
        // create PI nodes for this frame
        Saig_ManForEachPi( pAig, pObj, i )
            pObj->pData = Aig_ObjCreateCi( pFrames );
        // add internal nodes of this frame
        Aig_ManForEachNode( pAig, pObj, i )
            pObj->pData = Aig_And( pFrames, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
        // create POs for this frame
        Saig_ManForEachPo( pAig, pObj, i )
        {
            pObjPo = Aig_ObjCreateCo( pFrames, Aig_ObjChild0Copy(pObj) );
            Counter += Saig_ManFramesCount_rec( pFrames, Aig_ObjFanin0(pObjPo) );
        }
        if ( Counter >= nSizeMax )
        {
            Aig_ManCleanup( pFrames );
            return pFrames;
        }
        if ( f == nFrames - 1 )
            break;
        // save register inputs
        Saig_ManForEachLi( pAig, pObj, i )
            pObj->pData = Aig_ObjChild0Copy(pObj);
        // transfer to register outputs
        Saig_ManForEachLiLo(  pAig, pObjLi, pObjLo, i )
            pObjLo->pData = pObjLi->pData;
    }
    Aig_ManCleanup( pFrames );
    return pFrames;
}

ABC_NAMESPACE_IMPL_END

#include "misc/util/utilMem.h"

ABC_NAMESPACE_IMPL_START

 
/**Function*************************************************************

  Synopsis    [Performs BMC for the given AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManBmcSimple( Aig_Man_t * pAig, int nFrames, int nSizeMax, int nConfLimit, int fRewrite, int fVerbose, int * piFrame, int nCofFanLit )
{
    extern Aig_Man_t * Gia_ManCofactorAig( Aig_Man_t * p, int nFrames, int nCofFanLit );
    sat_solver * pSat;
    Cnf_Dat_t * pCnf;
    Aig_Man_t * pFrames, * pAigTemp;
    Aig_Obj_t * pObj;
    int status, Lit, i, RetValue = -1;
    clock_t clk;

    // derive the timeframes
    clk = clock();
    if ( nCofFanLit )
    {
        pFrames = Gia_ManCofactorAig( pAig, nFrames, nCofFanLit );
        if ( pFrames == NULL )
            return -1;
    }
    else if ( nSizeMax > 0 )
    {
        pFrames = Saig_ManFramesBmcLimit( pAig, nFrames, nSizeMax );
        nFrames = Aig_ManCoNum(pFrames) / Saig_ManPoNum(pAig) + ((Aig_ManCoNum(pFrames) % Saig_ManPoNum(pAig)) > 0);
    }
    else
        pFrames = Saig_ManFramesBmc( pAig, nFrames );
    if ( piFrame )
        *piFrame = nFrames;
    if ( fVerbose )
    {
        printf( "Running \"bmc\". AIG:  PI/PO/Reg = %d/%d/%d.  Node = %6d. Lev = %5d.\n", 
            Saig_ManPiNum(pAig), Saig_ManPoNum(pAig), Saig_ManRegNum(pAig),
            Aig_ManNodeNum(pAig), Aig_ManLevelNum(pAig) );
        printf( "Time-frames (%d):  PI/PO = %d/%d.  Node = %6d. Lev = %5d.  ", 
            nFrames, Aig_ManCiNum(pFrames), Aig_ManCoNum(pFrames), 
            Aig_ManNodeNum(pFrames), Aig_ManLevelNum(pFrames) );
        ABC_PRT( "Time", clock() - clk );
        fflush( stdout );
    }
    // rewrite the timeframes
    if ( fRewrite )
    {
        clk = clock();
//        pFrames = Dar_ManBalance( pAigTemp = pFrames, 0 );
        pFrames = Dar_ManRwsat( pAigTemp = pFrames, 1, 0 );
        Aig_ManStop( pAigTemp );
        if ( fVerbose )
        {
            printf( "Time-frames after rewriting:  Node = %6d. Lev = %5d.  ", 
                Aig_ManNodeNum(pFrames), Aig_ManLevelNum(pFrames) );
            ABC_PRT( "Time", clock() - clk );
            fflush( stdout );
        }
    }
    // create the SAT solver
    clk = clock();
    pCnf = Cnf_Derive( pFrames, Aig_ManCoNum(pFrames) );  
//if ( s_fInterrupt )
//return -1;
    pSat = sat_solver_new();
    sat_solver_setnvars( pSat, pCnf->nVars );
    for ( i = 0; i < pCnf->nClauses; i++ )
    {
        if ( !sat_solver_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1] ) )
            assert( 0 );
    }
    if ( fVerbose )
    {
        printf( "CNF: Variables = %6d. Clauses = %7d. Literals = %8d. ", pCnf->nVars, pCnf->nClauses, pCnf->nLiterals );
        ABC_PRT( "Time", clock() - clk );
        fflush( stdout );
    }
    status = sat_solver_simplify(pSat);
    if ( status == 0 )
    {
        if ( fVerbose )
        {
            printf( "The BMC problem is trivially UNSAT\n" );
            fflush( stdout );
        }
    }
    else
    {
        clock_t clkPart = clock();
        Aig_ManForEachCo( pFrames, pObj, i )
        {
//if ( s_fInterrupt )
//return -1;
            Lit = toLitCond( pCnf->pVarNums[pObj->Id], 0 );
            if ( fVerbose )
            {
                printf( "Solving output %2d of frame %3d ... \r", 
                    i % Saig_ManPoNum(pAig), i / Saig_ManPoNum(pAig) );
            }
            clk = clock();
            status = sat_solver_solve( pSat, &Lit, &Lit + 1, (ABC_INT64_T)nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
            if ( fVerbose && (i % Saig_ManPoNum(pAig) == Saig_ManPoNum(pAig) - 1) )
            {
                printf( "Solved %2d outputs of frame %3d.  ", 
                    Saig_ManPoNum(pAig), i / Saig_ManPoNum(pAig) );
                printf( "Conf =%8.0f. Imp =%11.0f. ", (double)pSat->stats.conflicts, (double)pSat->stats.propagations );
                ABC_PRT( "T", clock() - clkPart );
                clkPart = clock();
                fflush( stdout );
            }
            if ( status == l_False )
            {
/*
                Lit = lit_neg( Lit );
                RetValue = sat_solver_addclause( pSat, &Lit, &Lit + 1 );
                assert( RetValue );
                if ( pSat->qtail != pSat->qhead )
                {
                    RetValue = sat_solver_simplify(pSat);
                    assert( RetValue );
                }
*/
            }
            else if ( status == l_True )
            {
                Vec_Int_t * vCiIds = Cnf_DataCollectPiSatNums( pCnf, pFrames );
                int * pModel = Sat_SolverGetModel( pSat, vCiIds->pArray, vCiIds->nSize );
                pModel[Aig_ManCiNum(pFrames)] = pObj->Id;
                pAig->pSeqModel = Fra_SmlCopyCounterExample( pAig, pFrames, pModel );
                ABC_FREE( pModel );
                Vec_IntFree( vCiIds );

                if ( piFrame )
                    *piFrame = i / Saig_ManPoNum(pAig);
                RetValue = 0;
                break;
            }
            else
            {
                if ( piFrame )
                    *piFrame = i / Saig_ManPoNum(pAig);
                RetValue = -1;
                break;
            }
        }
    }
    sat_solver_delete( pSat );
    Cnf_DataFree( pCnf );
    Aig_ManStop( pFrames );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END


/**CFile****************************************************************

  FileName    [cnfUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG-to-CNF conversion.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: cnfUtil.c,v 1.1.1.1 2021/09/11 09:57:46 cabodi Exp $]

***********************************************************************/

#include "cnf.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes area, references, and nodes used in the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManScanMapping_rec( Cnf_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vMapped )
{
    Aig_Obj_t * pLeaf;
    Dar_Cut_t * pCutBest;
    int aArea, i;
    if ( pObj->nRefs++ || Aig_ObjIsCi(pObj) || Aig_ObjIsConst1(pObj) )
        return 0;
    assert( Aig_ObjIsAnd(pObj) );
    // collect the node first to derive pre-order
    if ( vMapped )
        Vec_PtrPush( vMapped, pObj );
    // visit the transitive fanin of the selected cut
    if ( pObj->fMarkB )
    {
        Vec_Ptr_t * vSuper = Vec_PtrAlloc( 100 );
        Aig_ObjCollectSuper( pObj, vSuper );
        aArea = Vec_PtrSize(vSuper) + 1;
        Vec_PtrForEachEntry( Aig_Obj_t *, vSuper, pLeaf, i )
            aArea += Aig_ManScanMapping_rec( p, Aig_Regular(pLeaf), vMapped );
        Vec_PtrFree( vSuper );
        ////////////////////////////
        pObj->fMarkB = 1;
    }
    else
    {
        pCutBest = Dar_ObjBestCut( pObj );
        aArea = Cnf_CutSopCost( p, pCutBest );
        Dar_CutForEachLeaf( p->pManAig, pCutBest, pLeaf, i )
            aArea += Aig_ManScanMapping_rec( p, pLeaf, vMapped );
    }
    return aArea;
}

/**Function*************************************************************

  Synopsis    [Computes area, references, and nodes used in the mapping.]

  Description [Collects the nodes in reverse topological order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManScanMapping( Cnf_Man_t * p, int fCollect )
{
    Vec_Ptr_t * vMapped = NULL;
    Aig_Obj_t * pObj;
    int i;
    // clean all references
    Aig_ManForEachObj( p->pManAig, pObj, i )
        pObj->nRefs = 0;
    // allocate the array
    if ( fCollect )
        vMapped = Vec_PtrAlloc( 1000 );
    // collect nodes reachable from POs in the DFS order through the best cuts
    p->aArea = 0;
    Aig_ManForEachCo( p->pManAig, pObj, i )
        p->aArea += Aig_ManScanMapping_rec( p, Aig_ObjFanin0(pObj), vMapped );
//    printf( "Variables = %6d. Clauses = %8d.\n", vMapped? Vec_PtrSize(vMapped) + Aig_ManCiNum(p->pManAig) + 1 : 0, p->aArea + 2 );
    return vMapped;
}

/**Function*************************************************************

  Synopsis    [Computes area, references, and nodes used in the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cnf_ManScanMapping_rec( Cnf_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vMapped, int fPreorder )
{
    Aig_Obj_t * pLeaf;
    Cnf_Cut_t * pCutBest;
    int aArea, i;
    if ( pObj->nRefs++ || Aig_ObjIsCi(pObj) || Aig_ObjIsConst1(pObj) )
        return 0;
    assert( Aig_ObjIsAnd(pObj) );
    assert( pObj->pData != NULL );
    // add the node to the mapping
    if ( vMapped && fPreorder )
         Vec_PtrPush( vMapped, pObj );
    // visit the transitive fanin of the selected cut
    if ( pObj->fMarkB )
    {
        Vec_Ptr_t * vSuper = Vec_PtrAlloc( 100 );
        Aig_ObjCollectSuper( pObj, vSuper );
        aArea = Vec_PtrSize(vSuper) + 1;
        Vec_PtrForEachEntry( Aig_Obj_t *, vSuper, pLeaf, i )
            aArea += Cnf_ManScanMapping_rec( p, Aig_Regular(pLeaf), vMapped, fPreorder );
        Vec_PtrFree( vSuper );
        ////////////////////////////
        pObj->fMarkB = 1;
    }
    else
    {
        pCutBest = (Cnf_Cut_t *)pObj->pData;
//        assert( pCutBest->nFanins > 0 );
        assert( pCutBest->Cost < 127 );
        aArea = pCutBest->Cost;
        Cnf_CutForEachLeaf( p->pManAig, pCutBest, pLeaf, i )
            aArea += Cnf_ManScanMapping_rec( p, pLeaf, vMapped, fPreorder );
    }
    // add the node to the mapping
    if ( vMapped && !fPreorder )
         Vec_PtrPush( vMapped, pObj );
    return aArea;
}

/**Function*************************************************************

  Synopsis    [Computes area, references, and nodes used in the mapping.]

  Description [Collects the nodes in reverse topological order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Cnf_ManScanMapping( Cnf_Man_t * p, int fCollect, int fPreorder )
{
    Vec_Ptr_t * vMapped = NULL;
    Aig_Obj_t * pObj;
    int i;
    // clean all references
    Aig_ManForEachObj( p->pManAig, pObj, i )
        pObj->nRefs = 0;
    // allocate the array
    if ( fCollect )
        vMapped = Vec_PtrAlloc( 1000 );
    // collect nodes reachable from POs in the DFS order through the best cuts
    p->aArea = 0;
    Aig_ManForEachCo( p->pManAig, pObj, i )
        p->aArea += Cnf_ManScanMapping_rec( p, Aig_ObjFanin0(pObj), vMapped, fPreorder );
//    printf( "Variables = %6d. Clauses = %8d.\n", vMapped? Vec_PtrSize(vMapped) + Aig_ManCiNum(p->pManAig) + 1 : 0, p->aArea + 2 );
    return vMapped;
}

/**Function*************************************************************

  Synopsis    [Returns the array of CI IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Cnf_DataCollectCiSatNums( Cnf_Dat_t * pCnf, Aig_Man_t * p )
{
    Vec_Int_t * vCiIds;
    Aig_Obj_t * pObj;
    int i;
    vCiIds = Vec_IntAlloc( Aig_ManCiNum(p) );
    Aig_ManForEachCi( p, pObj, i )
        Vec_IntPush( vCiIds, pCnf->pVarNums[pObj->Id] );
    return vCiIds;
}

/**Function*************************************************************

  Synopsis    [Returns the array of CI IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Cnf_DataCollectCoSatNums( Cnf_Dat_t * pCnf, Aig_Man_t * p )
{
    Vec_Int_t * vCoIds;
    Aig_Obj_t * pObj;
    int i;
    vCoIds = Vec_IntAlloc( Aig_ManCoNum(p) );
    Aig_ManForEachCo( p, pObj, i )
        Vec_IntPush( vCoIds, pCnf->pVarNums[pObj->Id] );
    return vCoIds;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END


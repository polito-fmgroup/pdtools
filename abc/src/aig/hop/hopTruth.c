/**CFile****************************************************************

  FileName    [hopTruth.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Minimalistic And-Inverter Graph package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: hopTruth.c,v 1.1.1.1 2021/09/11 09:57:46 cabodi Exp $]

***********************************************************************/

#include "hop.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline int  Hop_ManTruthWordNum( int nVars )  { return nVars <= 5 ? 1 : (1 << (nVars - 5)); }

static inline void Hop_ManTruthCopy( unsigned * pOut, unsigned * pIn, int nVars )
{
    int w;
    for ( w = Hop_ManTruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = pIn[w];
}
static inline void Hop_ManTruthClear( unsigned * pOut, int nVars )
{
    int w;
    for ( w = Hop_ManTruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = 0;
}
static inline void Hop_ManTruthFill( unsigned * pOut, int nVars )
{
    int w;
    for ( w = Hop_ManTruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = ~(unsigned)0;
}
static inline void Hop_ManTruthNot( unsigned * pOut, unsigned * pIn, int nVars )
{
    int w;
    for ( w = Hop_ManTruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = ~pIn[w];
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Construct BDDs and mark AIG nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hop_ManConvertAigToTruth_rec1( Hop_Obj_t * pObj )
{
    int Counter = 0;
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || Hop_ObjIsMarkA(pObj) )
        return 0;
    Counter += Hop_ManConvertAigToTruth_rec1( Hop_ObjFanin0(pObj) ); 
    Counter += Hop_ManConvertAigToTruth_rec1( Hop_ObjFanin1(pObj) );
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
    return Counter + 1;
}

/**Function*************************************************************

  Synopsis    [Computes truth table of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Hop_ManConvertAigToTruth_rec2( Hop_Obj_t * pObj, Vec_Int_t * vTruth, int nWords )
{
    unsigned * pTruth, * pTruth0, * pTruth1;
    int i;
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || !Hop_ObjIsMarkA(pObj) )
        return (unsigned *)pObj->pData;
    // compute the truth tables of the fanins
    pTruth0 = Hop_ManConvertAigToTruth_rec2( Hop_ObjFanin0(pObj), vTruth, nWords );
    pTruth1 = Hop_ManConvertAigToTruth_rec2( Hop_ObjFanin1(pObj), vTruth, nWords );
    // creat the truth table of the node
    pTruth  = Vec_IntFetch( vTruth, nWords );
    if ( Hop_ObjIsExor(pObj) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] ^ pTruth1[i];
    else if ( !Hop_ObjFaninC0(pObj) && !Hop_ObjFaninC1(pObj) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] & pTruth1[i];
    else if ( !Hop_ObjFaninC0(pObj) && Hop_ObjFaninC1(pObj) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] & ~pTruth1[i];
    else if ( Hop_ObjFaninC0(pObj) && !Hop_ObjFaninC1(pObj) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ~pTruth0[i] & pTruth1[i];
    else // if ( Hop_ObjFaninC0(pObj) && Hop_ObjFaninC1(pObj) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ~pTruth0[i] & ~pTruth1[i];
    assert( Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjClearMarkA( pObj );
    pObj->pData = pTruth;
    return pTruth;
}

/**Function*************************************************************

  Synopsis    [Computes truth table of the node.]

  Description [Assumes that the structural support is no more than 8 inputs.
  Uses array vTruth to store temporary truth tables. The returned pointer should 
  be used immediately.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Hop_ManConvertAigToTruth( Hop_Man_t * p, Hop_Obj_t * pRoot, int nVars, Vec_Int_t * vTruth, int fMsbFirst )
{
    static unsigned uTruths[8][8] = { // elementary truth tables
        { 0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA },
        { 0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC },
        { 0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0 },
        { 0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00 },
        { 0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000 }, 
        { 0x00000000,0xFFFFFFFF,0x00000000,0xFFFFFFFF,0x00000000,0xFFFFFFFF,0x00000000,0xFFFFFFFF }, 
        { 0x00000000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00000000,0xFFFFFFFF,0xFFFFFFFF }, 
        { 0x00000000,0x00000000,0x00000000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF } 
    };
    Hop_Obj_t * pObj;
    unsigned * pTruth, * pTruth2;
    int i, nWords, nNodes;
    Vec_Ptr_t * vTtElems;

    // if the number of variables is more than 8, allocate truth tables
    if ( nVars > 8 )
        vTtElems = Vec_PtrAllocTruthTables( nVars );
    else
        vTtElems = NULL;

    // clear the data fields and set marks
    nNodes = Hop_ManConvertAigToTruth_rec1( Hop_Regular(pRoot) );
    // prepare memory
    nWords = Hop_TruthWordNum( nVars );
    Vec_IntClear( vTruth );
    Vec_IntGrow( vTruth, nWords * (nNodes+1) );
    pTruth = Vec_IntFetch( vTruth, nWords );
    // check the case of a constant
    if ( Hop_ObjIsConst1( Hop_Regular(pRoot) ) )
    {
        assert( nNodes == 0 );
        if ( Hop_IsComplement(pRoot) )
            Hop_ManTruthClear( pTruth, nVars );
        else
            Hop_ManTruthFill( pTruth, nVars );
        return pTruth;
    }
    // set elementary truth tables at the leaves
    assert( nVars <= Hop_ManPiNum(p) );
//    assert( Hop_ManPiNum(p) <= 8 ); 
    if ( fMsbFirst )
    {
//        Hop_ManForEachPi( p, pObj, i )
        for ( i = 0; i < nVars; i++ )
        {
            pObj = Hop_ManPi( p, i );
            if ( vTtElems )
                pObj->pData = Vec_PtrEntry(vTtElems, nVars-1-i);
            else               
                pObj->pData = (void *)uTruths[nVars-1-i];
        }
    }
    else
    {
//        Hop_ManForEachPi( p, pObj, i )
        for ( i = 0; i < nVars; i++ )
        {
            pObj = Hop_ManPi( p, i );
            if ( vTtElems )
                pObj->pData = Vec_PtrEntry(vTtElems, i);
            else               
                pObj->pData = (void *)uTruths[i];
        }
    }
    // clear the marks and compute the truth table
    pTruth2 = Hop_ManConvertAigToTruth_rec2( Hop_Regular(pRoot), vTruth, nWords );
    // copy the result
    Hop_ManTruthCopy( pTruth, pTruth2, nVars );
    if ( Hop_IsComplement(pRoot) )
        Hop_ManTruthNot( pTruth, pTruth, nVars );
    if ( vTtElems )
        Vec_PtrFree( vTtElems );
    return pTruth;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END


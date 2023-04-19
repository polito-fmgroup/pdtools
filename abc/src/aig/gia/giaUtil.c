/**CFile****************************************************************

  FileName    [giaUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Various utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaUtil.c,v 1.1.1.1 2021/09/11 09:57:46 cabodi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
#define NUMBER1  3716960521u
#define NUMBER2  2174103536u

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates a sequence or random numbers.]

  Description []
               
  SideEffects []

  SeeAlso     [http://www.codeproject.com/KB/recipes/SimpleRNG.aspx]

***********************************************************************/
unsigned Gia_ManRandom( int fReset )
{
    static unsigned int m_z = NUMBER1;
    static unsigned int m_w = NUMBER2;
    if ( fReset )
    {
        m_z = NUMBER1;
        m_w = NUMBER2;
    }
    m_z = 36969 * (m_z & 65535) + (m_z >> 16);
    m_w = 18000 * (m_w & 65535) + (m_w >> 16);
    return (m_z << 16) + m_w;
}


/**Function*************************************************************

  Synopsis    [Creates random info for the primary inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManRandomInfo( Vec_Ptr_t * vInfo, int iInputStart, int iWordStart, int iWordStop )
{
    unsigned * pInfo;
    int i, w;
    Vec_PtrForEachEntryStart( unsigned *, vInfo, pInfo, i, iInputStart )
        for ( w = iWordStart; w < iWordStop; w++ )
            pInfo[w] = Gia_ManRandom(0);
}


/**Function*************************************************************

  Synopsis    [Returns the time stamp.]

  Description [The file should be closed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Gia_TimeStamp()
{
    static char Buffer[100];
	char * TimeStamp;
	time_t ltime;
    // get the current time
	time( &ltime );
	TimeStamp = asctime( localtime( &ltime ) );
	TimeStamp[ strlen(TimeStamp) - 1 ] = 0;
    strcpy( Buffer, TimeStamp );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    [Returns the composite name of the file.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Gia_FileNameGenericAppend( char * pBase, char * pSuffix )
{
    static char Buffer[1000];
    char * pDot;
    strcpy( Buffer, pBase );
    if ( (pDot = strrchr( Buffer, '.' )) )
        *pDot = 0;
    strcat( Buffer, pSuffix );
    if ( (pDot = strrchr( Buffer, '\\' )) || (pDot = strrchr( Buffer, '/' )) )
        return pDot+1;
    return Buffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManIncrementTravId( Gia_Man_t * p )                     
{ 
    if ( p->pTravIds == NULL )
    {
        p->nTravIdsAlloc = Gia_ManObjNum(p) + 100; 
        p->pTravIds = ABC_CALLOC( int, p->nTravIdsAlloc ); 
        p->nTravIds = 0;  
    }
    while ( p->nTravIdsAlloc < Gia_ManObjNum(p) )
    {
        p->nTravIdsAlloc *= 2;
        p->pTravIds = ABC_REALLOC( int, p->pTravIds, p->nTravIdsAlloc );
        memset( p->pTravIds + p->nTravIdsAlloc/2, 0, sizeof(int) * p->nTravIdsAlloc/2 );
    }
    p->nTravIds++;                                                    
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCleanMark01( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        pObj->fMark0 = pObj->fMark1 = 0;
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSetMark0( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        pObj->fMark0 = 1;
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCleanMark0( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        pObj->fMark0 = 0;
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCheckMark0( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        assert( pObj->fMark0 == 0 );
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSetMark1( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        pObj->fMark1 = 1;
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCleanMark1( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        pObj->fMark1 = 0;
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCheckMark1( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        assert( pObj->fMark1 == 0 );
}

/**Function*************************************************************

  Synopsis    [Cleans the value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCleanValue( Gia_Man_t * p )  
{
    int i;
    for ( i = 0; i < p->nObjs; i++ )
        p->pObjs[i].Value = 0;
}

/**Function*************************************************************

  Synopsis    [Cleans the value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManFillValue( Gia_Man_t * p )  
{
    int i;
    for ( i = 0; i < p->nObjs; i++ )
        p->pObjs[i].Value = ~0;
}

/**Function*************************************************************

  Synopsis    [Sets the phase of one object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ObjSetPhase( Gia_Obj_t * pObj )  
{
    if ( Gia_ObjIsAnd(pObj) )
        pObj->fPhase = (Gia_ObjPhase(Gia_ObjFanin0(pObj)) ^ Gia_ObjFaninC0(pObj)) & 
                       (Gia_ObjPhase(Gia_ObjFanin1(pObj)) ^ Gia_ObjFaninC1(pObj));
    else if ( Gia_ObjIsCo(pObj) )
        pObj->fPhase = (Gia_ObjPhase(Gia_ObjFanin0(pObj)) ^ Gia_ObjFaninC0(pObj));
    else
        pObj->fPhase = 0;
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSetPhase( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        Gia_ObjSetPhase( pObj );
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSetPhase1( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachCi( p, pObj, i )
        pObj->fPhase = 1;
    Gia_ManForEachObj( p, pObj, i )
        if ( !Gia_ObjIsCi(pObj) )
            Gia_ObjSetPhase( pObj );
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCleanPhase( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        pObj->fPhase = 0;
}

/**Function*************************************************************

  Synopsis    [Prepares copies for the model.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCleanLevels( Gia_Man_t * p, int Size )
{
    if ( p->vLevels == NULL )
        p->vLevels = Vec_IntAlloc( Size );
    Vec_IntFill( p->vLevels, Size, 0 );
}
/**Function*************************************************************

  Synopsis    [Prepares copies for the model.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCleanTruth( Gia_Man_t * p )
{
    if ( p->vTruths == NULL )
        p->vTruths = Vec_IntAlloc( Gia_ManObjNum(p) );
    Vec_IntFill( p->vTruths, Gia_ManObjNum(p), -1 );
}

/**Function*************************************************************

  Synopsis    [Assigns levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManLevelNum( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManCleanLevels( p, Gia_ManObjNum(p) );
    p->nLevels = 0;
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            Gia_ObjSetAndLevel( p, pObj );
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ObjSetCoLevel( p, pObj );
        else
            Gia_ObjSetLevel( p, pObj, 0 );
        p->nLevels = Abc_MaxInt( p->nLevels, Gia_ObjLevel(p, pObj) );
    }
    return p->nLevels;
}

/**Function*************************************************************

  Synopsis    [Assigns levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCreateValueRefs( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
    {
        pObj->Value = 0;
        if ( Gia_ObjIsAnd(pObj) )
        {
            Gia_ObjFanin0(pObj)->Value++;
            Gia_ObjFanin1(pObj)->Value++;
        }
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ObjFanin0(pObj)->Value++;
    }
}

/**Function*************************************************************

  Synopsis    [Assigns references.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCreateRefs( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    assert( p->pRefs == NULL );
    p->pRefs = ABC_CALLOC( int, Gia_ManObjNum(p) );
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
        {
            Gia_ObjRefFanin0Inc( p, pObj );
            Gia_ObjRefFanin1Inc( p, pObj );
        }
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ObjRefFanin0Inc( p, pObj );
    }
}

/**Function*************************************************************

  Synopsis    [Assigns references.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Gia_ManCreateMuxRefs( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj, * pCtrl, * pFan0, * pFan1;
    int i, * pMuxRefs;
    pMuxRefs = ABC_CALLOC( int, Gia_ManObjNum(p) );
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Gia_ObjRecognizeExor( pObj, &pFan0, &pFan1 ) )
            continue;
        if ( !Gia_ObjIsMuxType(pObj) )
            continue;
        pCtrl = Gia_ObjRecognizeMux( pObj, &pFan0, &pFan1 );
        pMuxRefs[ Gia_ObjId(p, Gia_Regular(pCtrl)) ]++;
    }
    return pMuxRefs;
}

/**Function*************************************************************

  Synopsis    [Computes the maximum frontier size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDfsForCrossCut_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vNodes )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
    {
        Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
        return;
    }
    if ( Gia_ObjIsCo(pObj) )
    {
        Gia_ObjFanin0(pObj)->Value++;
        Gia_ManDfsForCrossCut_rec( p, Gia_ObjFanin0(pObj), vNodes );
        Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ObjFanin0(pObj)->Value++;
    Gia_ObjFanin1(pObj)->Value++;
    Gia_ManDfsForCrossCut_rec( p, Gia_ObjFanin0(pObj), vNodes );
    Gia_ManDfsForCrossCut_rec( p, Gia_ObjFanin1(pObj), vNodes );
    Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
}
Vec_Int_t * Gia_ManDfsForCrossCut( Gia_Man_t * p, int fReverse )
{
    Vec_Int_t * vNodes;
    Gia_Obj_t * pObj;
    int i;
    Gia_ManCleanValue( p );
    vNodes = Vec_IntAlloc( Gia_ManObjNum(p) );
    Gia_ManIncrementTravId( p );
    if ( fReverse )
    {
        Gia_ManForEachCoReverse( p, pObj, i )
            if ( !Gia_ObjIsConst0(Gia_ObjFanin0(pObj)) )
                Gia_ManDfsForCrossCut_rec( p, pObj, vNodes );
    }
    else
    {
        Gia_ManForEachCo( p, pObj, i )
            if ( !Gia_ObjIsConst0(Gia_ObjFanin0(pObj)) )
                Gia_ManDfsForCrossCut_rec( p, pObj, vNodes );
    }
    return vNodes;
}
int Gia_ManCrossCut( Gia_Man_t * p, int fReverse )
{
    Vec_Int_t * vNodes;
    Gia_Obj_t * pObj;
    int i, nCutCur = 0, nCutMax = 0;
    vNodes = Gia_ManDfsForCrossCut( p, fReverse );
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
    {
        if ( pObj->Value )
            nCutCur++;
        if ( nCutMax < nCutCur )
            nCutMax = nCutCur;
        if ( Gia_ObjIsAnd(pObj) )
        {
            if ( --Gia_ObjFanin0(pObj)->Value == 0 )
                nCutCur--;
            if ( --Gia_ObjFanin1(pObj)->Value == 0 )
                nCutCur--;
        }
        else if ( Gia_ObjIsCo(pObj) )
        {
            if ( --Gia_ObjFanin0(pObj)->Value == 0 )
                nCutCur--;
        }
    }
    Vec_IntFree( vNodes );
    Gia_ManForEachObj( p, pObj, i )
        assert( pObj->Value == 0 );
    return nCutMax;
}

/**Function*************************************************************

  Synopsis    [Makes sure the manager is normalized.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManIsNormalized( Gia_Man_t * p )  
{
    int i, nOffset;
    nOffset = 1;
    for ( i = 0; i < Gia_ManCiNum(p); i++ )
        if ( !Gia_ObjIsCi( Gia_ManObj(p, nOffset+i) ) )
            return 0;
    nOffset = 1 + Gia_ManCiNum(p) + Gia_ManAndNum(p);
    for ( i = 0; i < Gia_ManCoNum(p); i++ )
        if ( !Gia_ObjIsCo( Gia_ManObj(p, nOffset+i) ) )
            return 0;
    return 1;
}


/**Function*************************************************************

  Synopsis    [Collects PO Ids into one array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManCollectPoIds( Gia_Man_t * p )
{
    Vec_Int_t * vStart;
    int Entry, i;
    vStart = Vec_IntAlloc( Gia_ManPoNum(p) );
    Vec_IntForEachEntryStop( p->vCos, Entry, i, Gia_ManPoNum(p) )
        Vec_IntPush( vStart, Entry );
    return vStart;
}


/**Function*************************************************************

  Synopsis    [Returns 1 if the node is the root of MUX or EXOR/NEXOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ObjIsMuxType( Gia_Obj_t * pNode )
{
    Gia_Obj_t * pNode0, * pNode1;
    // check that the node is regular
    assert( !Gia_IsComplement(pNode) );
    // if the node is not AND, this is not MUX
    if ( !Gia_ObjIsAnd(pNode) )
        return 0;
    // if the children are not complemented, this is not MUX
    if ( !Gia_ObjFaninC0(pNode) || !Gia_ObjFaninC1(pNode) )
        return 0;
    // get children
    pNode0 = Gia_ObjFanin0(pNode);
    pNode1 = Gia_ObjFanin1(pNode);
    // if the children are not ANDs, this is not MUX
    if ( !Gia_ObjIsAnd(pNode0) || !Gia_ObjIsAnd(pNode1) )
        return 0;
    // otherwise the node is MUX iff it has a pair of equal grandchildren
    return (Gia_ObjFanin0(pNode0) == Gia_ObjFanin0(pNode1) && (Gia_ObjFaninC0(pNode0) ^ Gia_ObjFaninC0(pNode1))) || 
           (Gia_ObjFanin0(pNode0) == Gia_ObjFanin1(pNode1) && (Gia_ObjFaninC0(pNode0) ^ Gia_ObjFaninC1(pNode1))) ||
           (Gia_ObjFanin1(pNode0) == Gia_ObjFanin0(pNode1) && (Gia_ObjFaninC1(pNode0) ^ Gia_ObjFaninC0(pNode1))) ||
           (Gia_ObjFanin1(pNode0) == Gia_ObjFanin1(pNode1) && (Gia_ObjFaninC1(pNode0) ^ Gia_ObjFaninC1(pNode1)));
}


/**Function*************************************************************

  Synopsis    [Recognizes what nodes are inputs of the EXOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ObjRecognizeExor( Gia_Obj_t * pObj, Gia_Obj_t ** ppFan0, Gia_Obj_t ** ppFan1 )
{
    Gia_Obj_t * p0, * p1;
    assert( !Gia_IsComplement(pObj) );
    if ( !Gia_ObjIsAnd(pObj) )
        return 0;
    assert( Gia_ObjIsAnd(pObj) );
    p0 = Gia_ObjChild0(pObj);
    p1 = Gia_ObjChild1(pObj);
    if ( !Gia_IsComplement(p0) || !Gia_IsComplement(p1) )
        return 0;
    p0 = Gia_Regular(p0);
    p1 = Gia_Regular(p1);
    if ( !Gia_ObjIsAnd(p0) || !Gia_ObjIsAnd(p1) )
        return 0;
    if ( Gia_ObjFanin0(p0) != Gia_ObjFanin0(p1) || Gia_ObjFanin1(p0) != Gia_ObjFanin1(p1) )
        return 0;
    if ( Gia_ObjFaninC0(p0) == Gia_ObjFaninC0(p1) || Gia_ObjFaninC1(p0) == Gia_ObjFaninC1(p1) )
        return 0;
    *ppFan0 = Gia_ObjChild0(p0);
    *ppFan1 = Gia_ObjChild1(p0);
    return 1;
}

/**Function*************************************************************

  Synopsis    [Recognizes what nodes are control and data inputs of a MUX.]

  Description [If the node is a MUX, returns the control variable C.
  Assigns nodes T and E to be the then and else variables of the MUX. 
  Node C is never complemented. Nodes T and E can be complemented.
  This function also recognizes EXOR/NEXOR gates as MUXes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Obj_t * Gia_ObjRecognizeMux( Gia_Obj_t * pNode, Gia_Obj_t ** ppNodeT, Gia_Obj_t ** ppNodeE )
{
    Gia_Obj_t * pNode0, * pNode1;
    assert( !Gia_IsComplement(pNode) );
    assert( Gia_ObjIsMuxType(pNode) );
    // get children
    pNode0 = Gia_ObjFanin0(pNode);
    pNode1 = Gia_ObjFanin1(pNode);

    // find the control variable
    if ( Gia_ObjFanin1(pNode0) == Gia_ObjFanin1(pNode1) && (Gia_ObjFaninC1(pNode0) ^ Gia_ObjFaninC1(pNode1)) )
    {
//        if ( FrGia_IsComplement(pNode1->p2) )
        if ( Gia_ObjFaninC1(pNode0) )
        { // pNode2->p2 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild0(pNode1));//pNode2->p1);
            *ppNodeE = Gia_Not(Gia_ObjChild0(pNode0));//pNode1->p1);
            return Gia_ObjChild1(pNode1);//pNode2->p2;
        }
        else
        { // pNode1->p2 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild0(pNode0));//pNode1->p1);
            *ppNodeE = Gia_Not(Gia_ObjChild0(pNode1));//pNode2->p1);
            return Gia_ObjChild1(pNode0);//pNode1->p2;
        }
    }
    else if ( Gia_ObjFanin0(pNode0) == Gia_ObjFanin0(pNode1) && (Gia_ObjFaninC0(pNode0) ^ Gia_ObjFaninC0(pNode1)) )
    {
//        if ( FrGia_IsComplement(pNode1->p1) )
        if ( Gia_ObjFaninC0(pNode0) )
        { // pNode2->p1 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild1(pNode1));//pNode2->p2);
            *ppNodeE = Gia_Not(Gia_ObjChild1(pNode0));//pNode1->p2);
            return Gia_ObjChild0(pNode1);//pNode2->p1;
        }
        else
        { // pNode1->p1 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild1(pNode0));//pNode1->p2);
            *ppNodeE = Gia_Not(Gia_ObjChild1(pNode1));//pNode2->p2);
            return Gia_ObjChild0(pNode0);//pNode1->p1;
        }
    }
    else if ( Gia_ObjFanin0(pNode0) == Gia_ObjFanin1(pNode1) && (Gia_ObjFaninC0(pNode0) ^ Gia_ObjFaninC1(pNode1)) )
    {
//        if ( FrGia_IsComplement(pNode1->p1) )
        if ( Gia_ObjFaninC0(pNode0) )
        { // pNode2->p2 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild0(pNode1));//pNode2->p1);
            *ppNodeE = Gia_Not(Gia_ObjChild1(pNode0));//pNode1->p2);
            return Gia_ObjChild1(pNode1);//pNode2->p2;
        }
        else
        { // pNode1->p1 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild1(pNode0));//pNode1->p2);
            *ppNodeE = Gia_Not(Gia_ObjChild0(pNode1));//pNode2->p1);
            return Gia_ObjChild0(pNode0);//pNode1->p1;
        }
    }
    else if ( Gia_ObjFanin1(pNode0) == Gia_ObjFanin0(pNode1) && (Gia_ObjFaninC1(pNode0) ^ Gia_ObjFaninC0(pNode1)) )
    {
//        if ( FrGia_IsComplement(pNode1->p2) )
        if ( Gia_ObjFaninC1(pNode0) )
        { // pNode2->p1 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild1(pNode1));//pNode2->p2);
            *ppNodeE = Gia_Not(Gia_ObjChild0(pNode0));//pNode1->p1);
            return Gia_ObjChild0(pNode1);//pNode2->p1;
        }
        else
        { // pNode1->p2 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild0(pNode0));//pNode1->p1);
            *ppNodeE = Gia_Not(Gia_ObjChild1(pNode1));//pNode2->p2);
            return Gia_ObjChild1(pNode0);//pNode1->p2;
        }
    }
    assert( 0 ); // this is not MUX
    return NULL;
}


/**Function*************************************************************

  Synopsis    [Dereferences the node's MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_NodeDeref_rec( Gia_Man_t * p, Gia_Obj_t * pNode )
{
    Gia_Obj_t * pFanin;
    int Counter = 0;
    if ( Gia_ObjIsCi(pNode) )
        return 0;
    assert( Gia_ObjIsAnd(pNode) );
    pFanin = Gia_ObjFanin0(pNode);
    assert( Gia_ObjRefNum(p, pFanin) > 0 );
    if ( Gia_ObjRefDec(p, pFanin) == 0 )
        Counter += Gia_NodeDeref_rec( p, pFanin );
    pFanin = Gia_ObjFanin1(pNode);
    assert( Gia_ObjRefNum(p, pFanin) > 0 );
    if ( Gia_ObjRefDec(p, pFanin) == 0 )
        Counter += Gia_NodeDeref_rec( p, pFanin );
    return Counter + 1;
}

/**Function*************************************************************

  Synopsis    [References the node's MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_NodeRef_rec( Gia_Man_t * p, Gia_Obj_t * pNode )
{
    Gia_Obj_t * pFanin;
    int Counter = 0;
    if ( Gia_ObjIsCi(pNode) )
        return 0;
    assert( Gia_ObjIsAnd(pNode) );
    pFanin = Gia_ObjFanin0(pNode);
    if ( Gia_ObjRefInc(p, pFanin) == 0 )
        Counter += Gia_NodeRef_rec( p, pFanin );
    pFanin = Gia_ObjFanin1(pNode);
    if ( Gia_ObjRefInc(p, pFanin) == 0 )
        Counter += Gia_NodeRef_rec( p, pFanin );
    return Counter + 1;
}


/**Function*************************************************************

  Synopsis    [References the node's MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManPoMffcSize( Gia_Man_t * p )
{
    Gia_ManCreateRefs( p );
    return Gia_NodeDeref_rec( p, Gia_ObjFanin0(Gia_ManPo(p, 0)) );
}

/**Function*************************************************************

  Synopsis    [Returns the number of internal nodes in the MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_NodeMffcSize( Gia_Man_t * p, Gia_Obj_t * pNode )
{
    int ConeSize1, ConeSize2;
    assert( !Gia_IsComplement(pNode) );
    assert( Gia_ObjIsCand(pNode) );
    ConeSize1 = Gia_NodeDeref_rec( p, pNode );
    ConeSize2 = Gia_NodeRef_rec( p, pNode );
    assert( ConeSize1 == ConeSize2 );
    assert( ConeSize1 >= 0 );
    return ConeSize1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if AIG has dangling nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManHasDangling( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, Counter = 0;
    Gia_ManForEachObj( p, pObj, i )
    {
        pObj->fMark0 = 0;
        if ( Gia_ObjIsAnd(pObj) )
        {
            Gia_ObjFanin0(pObj)->fMark0 = 1;
            Gia_ObjFanin1(pObj)->fMark0 = 1;
        }
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ObjFanin0(pObj)->fMark0 = 1;
    }
    Gia_ManForEachAnd( p, pObj, i )
        Counter += !pObj->fMark0;
    Gia_ManCleanMark0( p );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if AIG has dangling nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManMarkDangling( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, Counter = 0;
    Gia_ManForEachObj( p, pObj, i )
    {
        pObj->fMark0 = 0;
        if ( Gia_ObjIsAnd(pObj) )
        {
            Gia_ObjFanin0(pObj)->fMark0 = 1;
            Gia_ObjFanin1(pObj)->fMark0 = 1;
        }
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ObjFanin0(pObj)->fMark0 = 1;
    }
    Gia_ManForEachAnd( p, pObj, i )
        Counter += !pObj->fMark0;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if AIG has dangling nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManGetDangling( Gia_Man_t * p )
{
    Vec_Int_t * vDangles;
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
    {
        pObj->fMark0 = 0;
        if ( Gia_ObjIsAnd(pObj) )
        {
            Gia_ObjFanin0(pObj)->fMark0 = 1;
            Gia_ObjFanin1(pObj)->fMark0 = 1;
        }
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ObjFanin0(pObj)->fMark0 = 1;
    }
    vDangles = Vec_IntAlloc( 100 );
    Gia_ManForEachAnd( p, pObj, i )
        if ( !pObj->fMark0 )
            Vec_IntPush( vDangles, i );
    Gia_ManCleanMark0( p );
    return vDangles;
}
/**Function*************************************************************

  Synopsis    [Verbose printing of the AIG node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ObjPrint( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( pObj == NULL )
    {
        printf( "Object is NULL." );
        return;
    }
    if ( Gia_IsComplement(pObj) )
    {
        printf( "Compl " );
        pObj = Gia_Not(pObj);
    }
    assert( !Gia_IsComplement(pObj) );
    printf( "Obj %4d : ", Gia_ObjId(p, pObj) );
    if ( Gia_ObjIsConst0(pObj) )
        printf( "constant 0" );
    else if ( Gia_ObjIsPi(p, pObj) )
        printf( "PI" );
    else if ( Gia_ObjIsPo(p, pObj) )
        printf( "PO( %4d%s )", Gia_ObjFaninId0p(p, pObj), (Gia_ObjFaninC0(pObj)? "\'" : " ") );
    else if ( Gia_ObjIsCi(pObj) )
        printf( "RO( %4d%s )", Gia_ObjFaninId0p(p, Gia_ObjRoToRi(p, pObj)), (Gia_ObjFaninC0(Gia_ObjRoToRi(p, pObj))? "\'" : " ") );
    else if ( Gia_ObjIsCo(pObj) )
        printf( "RI( %4d%s )", Gia_ObjFaninId0p(p, pObj), (Gia_ObjFaninC0(pObj)? "\'" : " ") );
//    else if ( Gia_ObjIsBuf(pObj) )
//        printf( "BUF( %d%s )", Gia_ObjFaninId0p(p, pObj), (Gia_ObjFaninC0(pObj)? "\'" : " ") );
    else
        printf( "AND( %4d%s, %4d%s )", 
            Gia_ObjFaninId0p(p, pObj), (Gia_ObjFaninC0(pObj)? "\'" : " "), 
            Gia_ObjFaninId1p(p, pObj), (Gia_ObjFaninC1(pObj)? "\'" : " ") );
    if ( p->pRefs )
        printf( " (refs = %3d)", Gia_ObjRefNum(p, pObj) );
    printf( "\n" );
/*
    if ( p->pRefs )
    {
        Gia_Obj_t * pFanout;
        int i;
        int iFan = -1; // Suppress "might be used uninitialized"
        printf( "\nFanouts:\n" );
        Gia_ObjForEachFanout( p, pObj, pFanout, iFan, i )
        {
            printf( "    " );
            printf( "Node %4d : ", Gia_ObjId(pFanout) );
            if ( Gia_ObjIsPo(pFanout) )
                printf( "PO( %4d%s )", Gia_ObjFanin0(pFanout)->Id, (Gia_ObjFaninC0(pFanout)? "\'" : " ") );
            else if ( Gia_ObjIsBuf(pFanout) )
                printf( "BUF( %d%s )", Gia_ObjFanin0(pFanout)->Id, (Gia_ObjFaninC0(pFanout)? "\'" : " ") );
            else
                printf( "AND( %4d%s, %4d%s )", 
                    Gia_ObjFanin0(pFanout)->Id, (Gia_ObjFaninC0(pFanout)? "\'" : " "), 
                    Gia_ObjFanin1(pFanout)->Id, (Gia_ObjFaninC1(pFanout)? "\'" : " ") );
            printf( "\n" );
        }
        return;
    }
*/
}
void Gia_ManPrint( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        Gia_ObjPrint( p, pObj );
}
void Gia_ManPrintCo_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( Gia_ObjIsAnd(pObj) )
    {
        Gia_ManPrintCo_rec( p, Gia_ObjFanin0(pObj) );
        Gia_ManPrintCo_rec( p, Gia_ObjFanin1(pObj) );
    }
    Gia_ObjPrint( p, pObj );
}
void Gia_ManPrintCo( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    assert( Gia_ObjIsCo(pObj) );
    printf( "TFI cone of CO number %d:\n", Gia_ObjCioId(pObj) );
    Gia_ManPrintCo_rec( p, Gia_ObjFanin0(pObj) );
    Gia_ObjPrint( p, pObj );
}

/**Function*************************************************************

  Synopsis    [Complements the constraint outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManInvertConstraints( Gia_Man_t * pAig )
{
    Gia_Obj_t * pObj;
    int i;
    if ( Gia_ManConstrNum(pAig) == 0 )
        return;
    Gia_ManForEachPo( pAig, pObj, i )
    {
        if ( i >= Gia_ManPoNum(pAig) - Gia_ManConstrNum(pAig) )
            Gia_ObjFlipFaninC0( pObj );
    }
}

/**Function*************************************************************

  Synopsis    [Testing the speedup due to grouping POs into batches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCollectObjs_rec( Gia_Man_t * p, int iObjId, Vec_Int_t * vObjs, int Limit )
{
    Gia_Obj_t * pObj;
    if ( Vec_IntSize(vObjs) == Limit )
        return;
    if ( Gia_ObjIsTravIdCurrentId(p, iObjId) )
        return;
    Gia_ObjSetTravIdCurrentId(p, iObjId);
    pObj = Gia_ManObj( p, iObjId );
    if ( Gia_ObjIsAnd(pObj) )
    {
        Gia_ManCollectObjs_rec( p, Gia_ObjFaninId0p(p, pObj), vObjs, Limit );
        if ( Vec_IntSize(vObjs) == Limit )
            return;
        Gia_ManCollectObjs_rec( p, Gia_ObjFaninId1p(p, pObj), vObjs, Limit );
        if ( Vec_IntSize(vObjs) == Limit )
            return;
    }
    Vec_IntPush( vObjs, iObjId );
}
unsigned * Gia_ManComputePoTruthTables( Gia_Man_t * p, int nBytesMax )
{
    int nVars = Gia_ManPiNum(p);
    int nTruthWords = Abc_TruthWordNum( nVars );
    int nTruths = nBytesMax / (sizeof(unsigned) * nTruthWords);
    int nTotalNodes = 0, nRounds = 0;
    Vec_Int_t * vObjs;
    Gia_Obj_t * pObj;
    clock_t clk = clock();
    int i;
    printf( "Var = %d. Words = %d. Truths = %d.\n", nVars, nTruthWords, nTruths );
    vObjs = Vec_IntAlloc( nTruths );
    Gia_ManIncrementTravId( p );
    Gia_ManForEachPo( p, pObj, i )
    {
        Gia_ManCollectObjs_rec( p, Gia_ObjFaninId0p(p, pObj), vObjs, nTruths );
        if ( Vec_IntSize(vObjs) == nTruths )
        {
            nRounds++;
//            printf( "%d ", i );
            nTotalNodes += Vec_IntSize( vObjs );
            Vec_IntClear( vObjs );
            Gia_ManIncrementTravId( p );
        }
    }
//    printf( "\n" );
    nTotalNodes += Vec_IntSize( vObjs );
    Vec_IntFree( vObjs );

    printf( "Rounds = %d. Objects = %d. Total = %d.   ", nRounds, Gia_ManObjNum(p), nTotalNodes );
    Abc_PrintTime( 1, "Time", clock() - clk );

    return NULL;
}


/**Function*************************************************************

  Synopsis    [Returns 1 if the manager are structural identical.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCompare( Gia_Man_t * p1, Gia_Man_t * p2 )
{
    Gia_Obj_t * pObj1, * pObj2;
    int i;
    if ( Gia_ManObjNum(p1) != Gia_ManObjNum(p2) )
    {
        printf( "AIGs have different number of objects.\n" );
        return 0;
    }
    Gia_ManCleanValue( p1 );
    Gia_ManCleanValue( p2 );
    Gia_ManForEachObj( p1, pObj1, i )
    {
        pObj2 = Gia_ManObj( p2, i );
        if ( memcmp( pObj1, pObj2, sizeof(Gia_Obj_t) ) )
        {
            printf( "Objects %d are different.\n", i );
            return 0;
        }
        if ( p1->pReprs && p2->pReprs )
        {
            if ( memcmp( &p1->pReprs[i], &p2->pReprs[i], sizeof(Gia_Rpr_t) ) )
            {
                printf( "Representatives of objects %d are different.\n", i );
                return 0;
            }
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Marks nodes that appear as faninis of other nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManMarkFanoutDrivers( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManCleanMark0( p );
    Gia_ManForEachObj( p, pObj, i )
        if ( Gia_ObjIsAnd(pObj) )
        {
            Gia_ObjFanin0(pObj)->fMark0 = 1;
            Gia_ObjFanin1(pObj)->fMark0 = 1;
        }
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ObjFanin0(pObj)->fMark0 = 1;
}
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END


/**CFile****************************************************************

  FileName    [abcTiming.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Computation of timing info for mapped circuits.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcTiming.c,v 1.1.1.1 2021/09/11 09:57:46 cabodi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "map/mio/mio.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Abc_ManTime_t_
{
    Abc_Time_t     tArrDef;
    Abc_Time_t     tReqDef;
    Vec_Ptr_t  *   vArrs;
    Vec_Ptr_t  *   vReqs;
};

// static functions
static Abc_ManTime_t *     Abc_ManTimeStart();
static void                Abc_ManTimeExpand( Abc_ManTime_t * p, int nSize, int fProgressive );

// accessing the arrival and required times of a node
static inline Abc_Time_t * Abc_NodeArrival( Abc_Obj_t * pNode )  {  return (Abc_Time_t *)pNode->pNtk->pManTime->vArrs->pArray[pNode->Id];  }
static inline Abc_Time_t * Abc_NodeRequired( Abc_Obj_t * pNode ) {  return (Abc_Time_t *)pNode->pNtk->pManTime->vReqs->pArray[pNode->Id];  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the arrival time of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Time_t * Abc_NodeReadArrival( Abc_Obj_t * pNode )
{
    assert( pNode->pNtk->pManTime );
    return Abc_NodeArrival(pNode);
}

/**Function*************************************************************

  Synopsis    [Reads the arrival time of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Time_t * Abc_NodeReadRequired( Abc_Obj_t * pNode )
{
    assert( pNode->pNtk->pManTime );
    return Abc_NodeRequired(pNode);
}

/**Function*************************************************************

  Synopsis    [Reads the arrival time of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Time_t * Abc_NtkReadDefaultArrival( Abc_Ntk_t * pNtk )
{
    assert( pNtk->pManTime );
    return &pNtk->pManTime->tArrDef;
}

/**Function*************************************************************

  Synopsis    [Reads the arrival time of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Time_t * Abc_NtkReadDefaultRequired( Abc_Ntk_t * pNtk )
{
    assert( pNtk->pManTime );
    return &pNtk->pManTime->tReqDef;
}

/**Function*************************************************************

  Synopsis    [Reads average arrival time of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Abc_NodeReadArrivalAve( Abc_Obj_t * pNode )
{
    return 0.5 * Abc_NodeArrival(pNode)->Rise + 0.5 * Abc_NodeArrival(pNode)->Fall;
}

/**Function*************************************************************

  Synopsis    [Reads average required time of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Abc_NodeReadRequiredAve( Abc_Obj_t * pNode )
{
    return 0.5 * Abc_NodeReadRequired(pNode)->Rise + 0.5 * Abc_NodeReadRequired(pNode)->Fall;
}

/**Function*************************************************************

  Synopsis    [Sets the default arrival time for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTimeSetDefaultArrival( Abc_Ntk_t * pNtk, float Rise, float Fall )
{
    if ( Rise == 0.0 && Fall == 0.0 )
        return;
    if ( pNtk->pManTime == NULL )
        pNtk->pManTime = Abc_ManTimeStart();
    pNtk->pManTime->tArrDef.Rise  = Rise;
    pNtk->pManTime->tArrDef.Fall  = Fall;
    pNtk->pManTime->tArrDef.Worst = Abc_MaxFloat( Rise, Fall );
}

/**Function*************************************************************

  Synopsis    [Sets the default arrival time for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTimeSetDefaultRequired( Abc_Ntk_t * pNtk, float Rise, float Fall )
{
    if ( Rise == 0.0 && Fall == 0.0 )
        return;
    if ( pNtk->pManTime == NULL )
        pNtk->pManTime = Abc_ManTimeStart();
    pNtk->pManTime->tReqDef.Rise  = Rise;
    pNtk->pManTime->tReqDef.Fall  = Fall;
    pNtk->pManTime->tReqDef.Worst = Abc_MaxFloat( Rise, Fall );
}

/**Function*************************************************************

  Synopsis    [Sets the arrival time for an object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTimeSetArrival( Abc_Ntk_t * pNtk, int ObjId, float Rise, float Fall )
{
    Vec_Ptr_t * vTimes;
    Abc_Time_t * pTime;
    if ( pNtk->pManTime == NULL )
        pNtk->pManTime = Abc_ManTimeStart();
    if ( pNtk->pManTime->tArrDef.Rise == Rise && pNtk->pManTime->tArrDef.Fall == Fall )
        return;
    Abc_ManTimeExpand( pNtk->pManTime, ObjId + 1, 1 );
    // set the arrival time
    vTimes = pNtk->pManTime->vArrs;
    pTime = (Abc_Time_t *)vTimes->pArray[ObjId];
    pTime->Rise  = Rise;
    pTime->Fall  = Fall;
    pTime->Worst = Abc_MaxFloat( Rise, Fall );
}

/**Function*************************************************************

  Synopsis    [Sets the arrival time for an object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTimeSetRequired( Abc_Ntk_t * pNtk, int ObjId, float Rise, float Fall )
{
    Vec_Ptr_t * vTimes;
    Abc_Time_t * pTime;
    if ( pNtk->pManTime == NULL )
        pNtk->pManTime = Abc_ManTimeStart();
    if ( pNtk->pManTime->tReqDef.Rise == Rise && pNtk->pManTime->tReqDef.Fall == Fall )
        return;
    Abc_ManTimeExpand( pNtk->pManTime, ObjId + 1, 1 );
    // set the required time
    vTimes = pNtk->pManTime->vReqs;
    pTime = (Abc_Time_t *)vTimes->pArray[ObjId];
    pTime->Rise  = Rise;
    pTime->Fall  = Fall;
    pTime->Worst = Abc_MaxFloat( Rise, Fall );
}

/**Function*************************************************************

  Synopsis    [Finalizes the timing manager after setting arr/req times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTimeInitialize( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkOld )
{
    Abc_Obj_t * pObj;
    Abc_Time_t ** ppTimes, * pTime;
    int i;
    assert( pNtkOld == NULL || pNtkOld->pManTime != NULL );
    assert( pNtkOld == NULL || Abc_NtkPiNum(pNtk) == Abc_NtkPiNum(pNtkOld) );
    assert( pNtkOld == NULL || Abc_NtkPoNum(pNtk) == Abc_NtkPoNum(pNtkOld) );
    if ( pNtk->pManTime == NULL )
        return;
    Abc_ManTimeExpand( pNtk->pManTime, Abc_NtkObjNumMax(pNtk), 0 );
    // set global defaults
    if ( pNtkOld )
    {
        pNtk->pManTime->tArrDef = pNtkOld->pManTime->tArrDef;
        pNtk->pManTime->tReqDef = pNtkOld->pManTime->tReqDef;
        pNtk->AndGateDelay = pNtkOld->AndGateDelay;
    }
    // set the default timing
    ppTimes = (Abc_Time_t **)pNtk->pManTime->vArrs->pArray;
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        pTime = ppTimes[pObj->Id];
        if ( pTime->Worst != -ABC_INFINITY )
            continue;
        *pTime = pNtkOld ? *Abc_NodeReadArrival(Abc_NtkPi(pNtkOld, i)) : pNtk->pManTime->tArrDef;
    }
    // set the default timing
    ppTimes = (Abc_Time_t **)pNtk->pManTime->vReqs->pArray;
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        pTime = ppTimes[pObj->Id];
        if ( pTime->Worst != -ABC_INFINITY )
            continue;
        *pTime = pNtkOld ? *Abc_NodeReadRequired(Abc_NtkPo(pNtkOld, i)) : pNtk->pManTime->tReqDef;
    }
    // set the 0 arrival times for latch outputs and constant nodes
    ppTimes = (Abc_Time_t **)pNtk->pManTime->vArrs->pArray;
    Abc_NtkForEachLatchOutput( pNtk, pObj, i )
    {
        pTime = ppTimes[pObj->Id];
        pTime->Fall = pTime->Rise = pTime->Worst = 0.0;
    }
}

/**Function*************************************************************

  Synopsis    [Prepares the timing manager for delay trace.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTimePrepare( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    Abc_Time_t ** ppTimes, * pTime;
    int i;
    // if there is no timing manager, allocate and initialize
    if ( pNtk->pManTime == NULL )
    {
        pNtk->pManTime = Abc_ManTimeStart();
        Abc_NtkTimeInitialize( pNtk, NULL );
        return;
    }
    // if timing manager is given, expand it if necessary
    Abc_ManTimeExpand( pNtk->pManTime, Abc_NtkObjNumMax(pNtk), 0 );
    // clean arrivals except for PIs
    ppTimes = (Abc_Time_t **)pNtk->pManTime->vArrs->pArray;
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        pTime = ppTimes[pObj->Id];
        pTime->Fall = pTime->Rise = pTime->Worst = -ABC_INFINITY;
    }
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        pTime = ppTimes[pObj->Id];
        pTime->Fall = pTime->Rise = pTime->Worst = -ABC_INFINITY;
    }
    // clean required except for POs
    ppTimes = (Abc_Time_t **)pNtk->pManTime->vReqs->pArray;
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        pTime = ppTimes[pObj->Id];
        pTime->Fall = pTime->Rise = pTime->Worst = -ABC_INFINITY;
    }
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        pTime = ppTimes[pObj->Id];
        pTime->Fall = pTime->Rise = pTime->Worst = -ABC_INFINITY;
    }
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_ManTime_t * Abc_ManTimeStart()
{
    Abc_ManTime_t * p;
    p = ABC_ALLOC( Abc_ManTime_t, 1 );
    memset( p, 0, sizeof(Abc_ManTime_t) );
    p->vArrs = Vec_PtrAlloc( 0 );
    p->vReqs = Vec_PtrAlloc( 0 );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ManTimeStop( Abc_ManTime_t * p )
{
    if ( p->vArrs->nSize > 0 )
    {
        ABC_FREE( p->vArrs->pArray[0] );
        Vec_PtrFree( p->vArrs );
    }
    if ( p->vReqs->nSize > 0 )
    {
        ABC_FREE( p->vReqs->pArray[0] );
        Vec_PtrFree( p->vReqs );
    }
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Duplicates the timing manager with the PI/PO timing info.]

  Description [The PIs/POs of the new network should be allocated.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ManTimeDup( Abc_Ntk_t * pNtkOld, Abc_Ntk_t * pNtkNew )
{
    Abc_Obj_t * pObj;
    Abc_Time_t ** ppTimesOld, ** ppTimesNew;
    int i;
    if ( pNtkOld->pManTime == NULL )
        return;
    assert( Abc_NtkPiNum(pNtkOld) == Abc_NtkPiNum(pNtkNew) );
    assert( Abc_NtkPoNum(pNtkOld) == Abc_NtkPoNum(pNtkNew) );
    assert( Abc_NtkLatchNum(pNtkOld) == Abc_NtkLatchNum(pNtkNew) );
    // create the new timing manager
    pNtkNew->pManTime = Abc_ManTimeStart();
    Abc_ManTimeExpand( pNtkNew->pManTime, Abc_NtkObjNumMax(pNtkNew), 0 );
    // set the default timing
    pNtkNew->pManTime->tArrDef = pNtkOld->pManTime->tArrDef;
    pNtkNew->pManTime->tReqDef = pNtkOld->pManTime->tReqDef;
    // set the CI timing
    ppTimesOld = (Abc_Time_t **)pNtkOld->pManTime->vArrs->pArray;
    ppTimesNew = (Abc_Time_t **)pNtkNew->pManTime->vArrs->pArray;
    Abc_NtkForEachCi( pNtkOld, pObj, i )
        *ppTimesNew[ Abc_NtkCi(pNtkNew,i)->Id ] = *ppTimesOld[ pObj->Id ];
    // set the CO timing
    ppTimesOld = (Abc_Time_t **)pNtkOld->pManTime->vReqs->pArray;
    ppTimesNew = (Abc_Time_t **)pNtkNew->pManTime->vReqs->pArray;
    Abc_NtkForEachCo( pNtkOld, pObj, i )
        *ppTimesNew[ Abc_NtkCo(pNtkNew,i)->Id ] = *ppTimesOld[ pObj->Id ];
}

/**Function*************************************************************

  Synopsis    [Expends the storage for timing information.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ManTimeExpand( Abc_ManTime_t * p, int nSize, int fProgressive )
{
    Vec_Ptr_t * vTimes;
    Abc_Time_t * ppTimes, * ppTimesOld, * pTime;
    int nSizeOld, nSizeNew, i;

    nSizeOld = p->vArrs->nSize;
    if ( nSizeOld >= nSize )
        return;
    nSizeNew = fProgressive? 2 * nSize : nSize;
    if ( nSizeNew < 100 )
        nSizeNew = 100;

    vTimes = p->vArrs;
    Vec_PtrGrow( vTimes, nSizeNew );
    vTimes->nSize = nSizeNew;
    ppTimesOld = ( nSizeOld == 0 )? NULL : (Abc_Time_t *)vTimes->pArray[0];
    ppTimes = ABC_REALLOC( Abc_Time_t, ppTimesOld, nSizeNew );
    for ( i = 0; i < nSizeNew; i++ )
        vTimes->pArray[i] = ppTimes + i;
    for ( i = nSizeOld; i < nSizeNew; i++ )
    {
        pTime = (Abc_Time_t *)vTimes->pArray[i];
        pTime->Rise  = -ABC_INFINITY;
        pTime->Fall  = -ABC_INFINITY;
        pTime->Worst = -ABC_INFINITY;    
    }

    vTimes = p->vReqs;
    Vec_PtrGrow( vTimes, nSizeNew );
    vTimes->nSize = nSizeNew;
    ppTimesOld = ( nSizeOld == 0 )? NULL : (Abc_Time_t *)vTimes->pArray[0];
    ppTimes = ABC_REALLOC( Abc_Time_t, ppTimesOld, nSizeNew );
    for ( i = 0; i < nSizeNew; i++ )
        vTimes->pArray[i] = ppTimes + i;
    for ( i = nSizeOld; i < nSizeNew; i++ )
    {
        pTime = (Abc_Time_t *)vTimes->pArray[i];
        pTime->Rise  = -ABC_INFINITY;
        pTime->Fall  = -ABC_INFINITY;
        pTime->Worst = -ABC_INFINITY;    
    }
}






/**Function*************************************************************

  Synopsis    [Sets the CI node levels according to the arrival info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkSetNodeLevelsArrival( Abc_Ntk_t * pNtkOld )
{
    Abc_Obj_t * pNodeOld, * pNodeNew;
    float tAndDelay;
    int i;
    if ( pNtkOld->pManTime == NULL )
        return;
    if ( Abc_FrameReadLibGen() == NULL || Mio_LibraryReadNand2((Mio_Library_t *)Abc_FrameReadLibGen()) == NULL )
        return;
    tAndDelay = Mio_LibraryReadDelayNand2Max((Mio_Library_t *)Abc_FrameReadLibGen());
    Abc_NtkForEachPi( pNtkOld, pNodeOld, i )
    {
        pNodeNew = pNodeOld->pCopy;
        pNodeNew->Level = (int)(Abc_NodeArrival(pNodeOld)->Worst / tAndDelay);
    }
}

/**Function*************************************************************

  Synopsis    [Sets the CI node levels according to the arrival info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Time_t * Abc_NtkGetCiArrivalTimes( Abc_Ntk_t * pNtk )
{
    Abc_Time_t * p;
    Abc_Obj_t * pNode;
    int i;
    p = ABC_CALLOC( Abc_Time_t, Abc_NtkCiNum(pNtk) );
    if ( pNtk->pManTime == NULL )
        return p;
    // set the PI arrival times
    Abc_NtkForEachPi( pNtk, pNode, i )
        p[i] = *Abc_NodeArrival(pNode);
    return p;
}
Abc_Time_t * Abc_NtkGetCoRequiredTimes( Abc_Ntk_t * pNtk )
{
    Abc_Time_t * p;
    Abc_Obj_t * pNode;
    int i;
    p = ABC_CALLOC( Abc_Time_t, Abc_NtkCoNum(pNtk) );
    if ( pNtk->pManTime == NULL )
        return p;
    // set the PO required times
    Abc_NtkForEachPo( pNtk, pNode, i )
        p[i] = *Abc_NodeRequired(pNode);
    return p;
}


/**Function*************************************************************

  Synopsis    [Sets the CI node levels according to the arrival info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float * Abc_NtkGetCiArrivalFloats( Abc_Ntk_t * pNtk )
{
    float * p;
    Abc_Obj_t * pNode;
    int i;
    p = ABC_CALLOC( float, Abc_NtkCiNum(pNtk) );
    if ( pNtk->pManTime == NULL )
        return p;
    // set the PI arrival times
    Abc_NtkForEachPi( pNtk, pNode, i )
        p[i] = Abc_NodeArrival(pNode)->Worst;
    return p;
}
float * Abc_NtkGetCoRequiredFloats( Abc_Ntk_t * pNtk )
{
    float * p;
    Abc_Obj_t * pNode;
    int i;
    if ( pNtk->pManTime == NULL )
        return NULL;
    // set the PO required times
    p = ABC_CALLOC( float, Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachPo( pNtk, pNode, i )
        p[i] = Abc_NodeRequired(pNode)->Worst;
    return p;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkDelayTraceSlackStart( Abc_Ntk_t * pNtk )
{
    Vec_Int_t * vSlacks;
    Abc_Obj_t * pObj;
    int i, k;
    vSlacks = Vec_IntAlloc( Abc_NtkObjNumMax(pNtk) + Abc_NtkGetTotalFanins(pNtk) );
    Vec_IntFill( vSlacks, Abc_NtkObjNumMax(pNtk), -1 );
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        Vec_IntWriteEntry( vSlacks, i, Vec_IntSize(vSlacks) );
        for ( k = 0; k < Abc_ObjFaninNum(pObj); k++ )
            Vec_IntPush( vSlacks, -1 );
    }
//    assert( Abc_MaxInt(16, Vec_IntSize(vSlacks)) == Vec_IntCap(vSlacks) );
    return vSlacks;
}

/**Function*************************************************************

  Synopsis    [Read/write edge slacks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline float Abc_NtkDelayTraceSlack( Vec_Int_t * vSlacks, Abc_Obj_t * pObj, int iFanin )
{
    return Abc_Int2Float( Vec_IntEntry( vSlacks, Vec_IntEntry(vSlacks, Abc_ObjId(pObj)) + iFanin ) );
}
static inline void Abc_NtkDelayTraceSetSlack( Vec_Int_t * vSlacks, Abc_Obj_t * pObj, int iFanin, float Num )
{
    Vec_IntWriteEntry( vSlacks, Vec_IntEntry(vSlacks, Abc_ObjId(pObj)) + iFanin, Abc_Float2Int(Num) );
}

/**Function*************************************************************

  Synopsis    [Find most-critical path (the path with smallest slacks).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDelayTraceCritPath_rec( Vec_Int_t * vSlacks, Abc_Obj_t * pNode, Abc_Obj_t * pLeaf, Vec_Int_t * vBest )
{
    Abc_Obj_t * pFanin, * pFaninBest = NULL;
    float SlackMin = ABC_INFINITY;
    int i;
    // check primary inputs
    if ( Abc_ObjIsCi(pNode) )
        return (pLeaf == NULL || pLeaf == pNode);
    assert( Abc_ObjIsNode(pNode) );
    // check visited
    if ( Abc_NodeIsTravIdCurrent( pNode ) )
        return Vec_IntEntry(vBest, Abc_ObjId(pNode)) >= 0;
    Abc_NodeSetTravIdCurrent( pNode );
    // check the node
    assert( Abc_ObjIsNode(pNode) );
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        if ( !Abc_NtkDelayTraceCritPath_rec( vSlacks, pFanin, pLeaf, vBest ) )
            continue;
        if ( pFaninBest == NULL || SlackMin > Abc_NtkDelayTraceSlack(vSlacks, pNode, i) )
        {
            pFaninBest = pFanin;
            SlackMin = Abc_NtkDelayTraceSlack(vSlacks, pNode, i);
        }
    }
    if ( pFaninBest != NULL )
        Vec_IntWriteEntry( vBest, Abc_ObjId(pNode), Abc_NodeFindFanin(pNode, pFaninBest) );
    return (pFaninBest != NULL);
}

/**Function*************************************************************

  Synopsis    [Find most-critical path (the path with smallest slacks).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDelayTraceCritPathCollect_rec( Vec_Int_t * vSlacks, Abc_Obj_t * pNode, Vec_Int_t * vBest, Vec_Ptr_t * vPath )
{
    assert( Abc_ObjIsCi(pNode) || Abc_ObjIsNode(pNode) );
    if ( Abc_ObjIsNode(pNode) )
    {
        int iFanin = Vec_IntEntry( vBest, Abc_ObjId(pNode) );
        assert( iFanin >= 0 );
        Abc_NtkDelayTraceCritPathCollect_rec( vSlacks, Abc_ObjFanin(pNode, iFanin), vBest, vPath );
    }
    Vec_PtrPush( vPath, pNode );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeDelayTraceArrival( Abc_Obj_t * pNode, Vec_Int_t * vSlacks )
{
    Abc_Obj_t * pFanin;
    Abc_Time_t * pTimeIn, * pTimeOut;
    float tDelayBlockRise, tDelayBlockFall;
    Mio_PinPhase_t PinPhase;
    Mio_Pin_t * pPin;
    int i;

    // start the arrival time of the node
    pTimeOut = Abc_NodeArrival(pNode);
    pTimeOut->Rise = pTimeOut->Fall = -ABC_INFINITY; 
    // go through the pins of the gate
    pPin = Mio_GateReadPins((Mio_Gate_t *)pNode->pData);
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        pTimeIn = Abc_NodeArrival(pFanin);
        // get the interesting parameters of this pin
        PinPhase = Mio_PinReadPhase(pPin);
        tDelayBlockRise = (float)Mio_PinReadDelayBlockRise( pPin );  
        tDelayBlockFall = (float)Mio_PinReadDelayBlockFall( pPin );  
        // compute the arrival times of the positive phase
        if ( PinPhase != MIO_PHASE_INV )  // NONINV phase is present
        {
            if ( pTimeOut->Rise < pTimeIn->Rise + tDelayBlockRise )
                pTimeOut->Rise = pTimeIn->Rise + tDelayBlockRise;
            if ( pTimeOut->Fall < pTimeIn->Fall + tDelayBlockFall )
                pTimeOut->Fall = pTimeIn->Fall + tDelayBlockFall;
        }
        if ( PinPhase != MIO_PHASE_NONINV )  // INV phase is present
        {
            if ( pTimeOut->Rise < pTimeIn->Fall + tDelayBlockRise )
                pTimeOut->Rise = pTimeIn->Fall + tDelayBlockRise;
            if ( pTimeOut->Fall < pTimeIn->Rise + tDelayBlockFall )
                pTimeOut->Fall = pTimeIn->Rise + tDelayBlockFall;
        }
        pPin = Mio_PinReadNext(pPin);
    }
    pTimeOut->Worst = Abc_MaxFloat( pTimeOut->Rise, pTimeOut->Fall );

    // compute edge slacks
    if ( vSlacks )
    {
        float Slack;
        // go through the pins of the gate
        pPin = Mio_GateReadPins((Mio_Gate_t *)pNode->pData);
        Abc_ObjForEachFanin( pNode, pFanin, i )
        {
            pTimeIn = Abc_NodeArrival(pFanin);
            // get the interesting parameters of this pin
            PinPhase = Mio_PinReadPhase(pPin);
            tDelayBlockRise = (float)Mio_PinReadDelayBlockRise( pPin );  
            tDelayBlockFall = (float)Mio_PinReadDelayBlockFall( pPin );  
            // compute the arrival times of the positive phase
            Slack = ABC_INFINITY;
            if ( PinPhase != MIO_PHASE_INV )  // NONINV phase is present
            {
//                if ( pTimeOut->Rise < pTimeIn->Rise + tDelayBlockRise )
//                    pTimeOut->Rise = pTimeIn->Rise + tDelayBlockRise;
//                if ( pTimeOut->Fall < pTimeIn->Fall + tDelayBlockFall )
//                    pTimeOut->Fall = pTimeIn->Fall + tDelayBlockFall;
                Slack = Abc_MinFloat( Slack, Abc_AbsFloat(pTimeIn->Rise + tDelayBlockRise - pTimeOut->Rise) );
                Slack = Abc_MinFloat( Slack, Abc_AbsFloat(pTimeIn->Fall + tDelayBlockFall - pTimeOut->Fall) );
            }
            if ( PinPhase != MIO_PHASE_NONINV )  // INV phase is present
            {
//                if ( pTimeOut->Rise < pTimeIn->Fall + tDelayBlockRise )
//                    pTimeOut->Rise = pTimeIn->Fall + tDelayBlockRise;
//                if ( pTimeOut->Fall < pTimeIn->Rise + tDelayBlockFall )
//                    pTimeOut->Fall = pTimeIn->Rise + tDelayBlockFall;
                Slack = Abc_MinFloat( Slack, Abc_AbsFloat(pTimeIn->Fall + tDelayBlockRise - pTimeOut->Rise) );
                Slack = Abc_MinFloat( Slack, Abc_AbsFloat(pTimeIn->Rise + tDelayBlockFall - pTimeOut->Fall) );
            }
            pPin = Mio_PinReadNext(pPin);
            Abc_NtkDelayTraceSetSlack( vSlacks, pNode, i, Slack );
        }
    }
}


/**Function*************************************************************

  Synopsis    [Performs delay-trace of the network. If input (pIn) or 
  output (pOut) are given, finds the most-timing-critical path between 
  them and prints it to the standard output. If input and/or output are 
  not given, finds the most-critical path in the network and prints it.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Abc_NtkDelayTrace( Abc_Ntk_t * pNtk, Abc_Obj_t * pOut, Abc_Obj_t * pIn, int fPrint )
{
    Vec_Int_t * vSlacks = NULL;
    Abc_Obj_t * pNode, * pDriver;
    Vec_Ptr_t * vNodes;
    Abc_Time_t * pTime;
    float tArrivalMax;
    int i;

    assert( Abc_NtkIsMappedLogic(pNtk) );
    assert( pOut == NULL || Abc_ObjIsCo(pOut) );
    assert( pIn == NULL || Abc_ObjIsCi(pIn) );

    // create slacks (need slacks if printing is requested even if pIn/pOut are not given)
    if ( pOut || pIn || fPrint )
        vSlacks = Abc_NtkDelayTraceSlackStart( pNtk );

    // compute the timing
    Abc_NtkTimePrepare( pNtk );
    vNodes = Abc_NtkDfs( pNtk, 1 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
        Abc_NodeDelayTraceArrival( pNode, vSlacks );
    Vec_PtrFree( vNodes );

    // get the latest arrival times
    tArrivalMax = -ABC_INFINITY;
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        pDriver = Abc_ObjFanin0(pNode);
        pTime   = Abc_NodeArrival(pDriver);
        if ( tArrivalMax < pTime->Worst )
            tArrivalMax = pTime->Worst;
    }

    // determine the output to print
    if ( fPrint && pOut == NULL )
    {
        Abc_NtkForEachCo( pNtk, pNode, i )
        {
            pDriver = Abc_ObjFanin0(pNode);
            pTime   = Abc_NodeArrival(pDriver);
            if ( tArrivalMax == pTime->Worst )
                pOut = pNode;
        }
        assert( pOut != NULL );
    }

    if ( fPrint )
    {
        Vec_Ptr_t * vPath = Vec_PtrAlloc( 100 );
        Vec_Int_t * vBest = Vec_IntStartFull( Abc_NtkObjNumMax(pNtk) );
        // traverse to determine the critical path
        Abc_NtkIncrementTravId( pNtk );
        if ( !Abc_NtkDelayTraceCritPath_rec( vSlacks, Abc_ObjFanin0(pOut), pIn, vBest ) )
        {
            if ( pIn == NULL )
                printf( "The logic cone of PO \"%s\" has no primary inputs.\n", Abc_ObjName(pOut) );
            else
                printf( "There is no combinational path between PI \"%s\" and PO \"%s\".\n", Abc_ObjName(pIn), Abc_ObjName(pOut) );
        }
        else
        {
            float Slack = 0.0, SlackAdd;
            int k, iFanin, Length = 0;
            Abc_Obj_t * pFanin;
            // check the additional slack
            SlackAdd = Abc_NodeRequired(pOut)->Worst - Abc_NodeArrival(Abc_ObjFanin0(pOut))->Worst;
            // collect the critical path
            Abc_NtkDelayTraceCritPathCollect_rec( vSlacks, Abc_ObjFanin0(pOut), vBest, vPath );
            if ( pIn == NULL )
                pIn = (Abc_Obj_t *)Vec_PtrEntry( vPath, 0 );
            // find the longest gate name
            Vec_PtrForEachEntry( Abc_Obj_t *, vPath, pNode, i )
                if ( Abc_ObjIsNode(pNode) )
                    Length = Abc_MaxInt( Length, strlen(Mio_GateReadName((Mio_Gate_t *)pNode->pData)) );
            // print critical path
            Abc_NtkLevel( pNtk );
            printf( "Critical path from PI \"%s\" to PO \"%s\":\n", Abc_ObjName(pIn), Abc_ObjName(pOut) ); 
            Vec_PtrForEachEntry( Abc_Obj_t *, vPath, pNode, i )
            {
                printf( "Level %3d : ", Abc_ObjLevel(pNode) );
                if ( Abc_ObjIsCi(pNode) )
                {
                    printf( "Primary input \"%s\".   ", Abc_ObjName(pNode) );
                    printf( "Arrival time =%6.1f. ", Abc_NodeReadArrival(pNode)->Worst );
                    printf( "\n" );
                    continue;
                }
                if ( Abc_ObjIsCo(pNode) )
                {
                    printf( "Primary output \"%s\".   ", Abc_ObjName(pNode) );
                    printf( "Arrival =%6.1f. ", Abc_NodeReadArrival(pNode)->Worst );
                }
                else
                {
                    assert( Abc_ObjIsNode(pNode) );
                    iFanin = Abc_NodeFindFanin( pNode, (Abc_Obj_t *)Vec_PtrEntry(vPath,i-1) );
                    Slack = Abc_NtkDelayTraceSlack(vSlacks, pNode, iFanin);
                    printf( "%10s/", Abc_ObjName(pNode) );
                    printf( "%-4s", Mio_GateReadPinName((Mio_Gate_t *)pNode->pData, iFanin) );
                    printf( " (%s)", Mio_GateReadName((Mio_Gate_t *)pNode->pData) );
                    for ( k = strlen(Mio_GateReadName((Mio_Gate_t *)pNode->pData)); k < Length; k++ )
                        printf( " " );
                    printf( "   " );
                    printf( "Arrival =%6.1f.   ", Abc_NodeReadArrival(pNode)->Worst );
                    printf( "I/O times: (" );
                    Abc_ObjForEachFanin( pNode, pFanin, k )
                        printf( "%s%.1f", (k? ", ":""), Abc_NodeReadArrival(pFanin)->Worst );
//                    printf( " -> %.1f)", Abc_NodeReadArrival(pNode)->Worst + Slack + SlackAdd );
                    printf( " -> %.1f)", Abc_NodeReadArrival(pNode)->Worst );
                }
                printf( "\n" );
            }
            printf( "Level %3d : ", Abc_ObjLevel(Abc_ObjFanin0(pOut)) + 1 );
            printf( "Primary output \"%s\".   ", Abc_ObjName(pOut) );
            printf( "Required time = %6.1f.  ", Abc_NodeRequired(pOut)->Worst );
            printf( "Path slack = %6.1f.\n", SlackAdd );
        }
        Vec_PtrFree( vPath );
        Vec_IntFree( vBest );
    }

    Vec_IntFreeP( &vSlacks );
    return tArrivalMax;
}



/**Function*************************************************************

  Synopsis    [Computes the level of the node using its fanin levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjLevelNew( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanin;
    int i, Level = 0;
    Abc_ObjForEachFanin( pObj, pFanin, i )
        Level = Abc_MaxFloat( Level, Abc_ObjLevel(pFanin) );
    return Level + 1;
}

/**Function*************************************************************

  Synopsis    [Computes the reverse level of the node using its fanout levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjReverseLevelNew( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanout;
    int i, LevelCur, Level = 0;
    Abc_ObjForEachFanout( pObj, pFanout, i )
    {
        LevelCur = Abc_ObjReverseLevel( pFanout );
        Level = Abc_MaxFloat( Level, LevelCur );
    }
    return Level + 1;
}

/**Function*************************************************************

  Synopsis    [Returns required level of the node.]

  Description [Converts the reverse levels of the node into its required 
  level as follows: ReqLevel(Node) = MaxLevels(Ntk) + 1 - LevelR(Node).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjRequiredLevel( Abc_Obj_t * pObj )
{
    Abc_Ntk_t * pNtk = pObj->pNtk;
    assert( pNtk->vLevelsR );
    return pNtk->LevelMax + 1 - Abc_ObjReverseLevel(pObj);
}

/**Function*************************************************************

  Synopsis    [Returns the reverse level of the node.]

  Description [The reverse level is the level of the node in reverse
  topological order, starting from the COs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjReverseLevel( Abc_Obj_t * pObj )
{
    Abc_Ntk_t * pNtk = pObj->pNtk;
    assert( pNtk->vLevelsR );
    Vec_IntFillExtra( pNtk->vLevelsR, pObj->Id + 1, 0 );
    return Vec_IntEntry(pNtk->vLevelsR, pObj->Id);
}

/**Function*************************************************************

  Synopsis    [Sets the reverse level of the node.]

  Description [The reverse level is the level of the node in reverse
  topological order, starting from the COs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ObjSetReverseLevel( Abc_Obj_t * pObj, int LevelR )
{
    Abc_Ntk_t * pNtk = pObj->pNtk;
    assert( pNtk->vLevelsR );
    Vec_IntFillExtra( pNtk->vLevelsR, pObj->Id + 1, 0 );
    Vec_IntWriteEntry( pNtk->vLevelsR, pObj->Id, LevelR );
}

/**Function*************************************************************

  Synopsis    [Prepares for the computation of required levels.]

  Description [This procedure should be called before the required times
  are used. It starts internal data structures, which records the level 
  from the COs of the network nodes in reverse topologogical order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkStartReverseLevels( Abc_Ntk_t * pNtk, int nMaxLevelIncrease )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj;
    int i;
    // remember the maximum number of direct levels
    pNtk->LevelMax = Abc_NtkLevel(pNtk) + nMaxLevelIncrease;
    // start the reverse levels
    pNtk->vLevelsR = Vec_IntAlloc( 0 );
    Vec_IntFill( pNtk->vLevelsR, 1 + Abc_NtkObjNumMax(pNtk), 0 );
    // compute levels in reverse topological order
    vNodes = Abc_NtkDfsReverse( pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        Abc_ObjSetReverseLevel( pObj, Abc_ObjReverseLevelNew(pObj) );
    Vec_PtrFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Cleans the data structures used to compute required levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkStopReverseLevels( Abc_Ntk_t * pNtk )
{
    assert( pNtk->vLevelsR );
    Vec_IntFree( pNtk->vLevelsR );
    pNtk->vLevelsR = NULL;
    pNtk->LevelMax = 0;

}

/**Function*************************************************************

  Synopsis    [Incrementally updates level of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkUpdateLevel( Abc_Obj_t * pObjNew, Vec_Vec_t * vLevels )
{
    Abc_Obj_t * pFanout, * pTemp;
    int LevelOld, Lev, k, m;
//    int Counter = 0, CounterMax = 0;
    // check if level has changed
    LevelOld = Abc_ObjLevel(pObjNew);
    if ( LevelOld == Abc_ObjLevelNew(pObjNew) )
        return;
    // start the data structure for level update
    // we cannot fail to visit a node when using this structure because the 
    // nodes are stored by their _old_ levels, which are assumed to be correct
    Vec_VecClear( vLevels );
    Vec_VecPush( vLevels, LevelOld, pObjNew );
    pObjNew->fMarkA = 1;
    // recursively update level
    Vec_VecForEachEntryStart( Abc_Obj_t *, vLevels, pTemp, Lev, k, LevelOld )
    {
//        Counter--;
        pTemp->fMarkA = 0;
        assert( Abc_ObjLevel(pTemp) == Lev );
        Abc_ObjSetLevel( pTemp, Abc_ObjLevelNew(pTemp) );
        // if the level did not change, no need to check the fanout levels
        if ( Abc_ObjLevel(pTemp) == Lev )
            continue;
        // schedule fanout for level update
        Abc_ObjForEachFanout( pTemp, pFanout, m )
        {
            if ( !Abc_ObjIsCo(pFanout) && !pFanout->fMarkA )
            {
                assert( Abc_ObjLevel(pFanout) >= Lev );
                Vec_VecPush( vLevels, Abc_ObjLevel(pFanout), pFanout );
//                Counter++;
//                CounterMax = Abc_MaxFloat( CounterMax, Counter );
                pFanout->fMarkA = 1;
            }
        }
    }
//    printf( "%d ", CounterMax );
}

/**Function*************************************************************

  Synopsis    [Incrementally updates level of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkUpdateReverseLevel( Abc_Obj_t * pObjNew, Vec_Vec_t * vLevels )
{
    Abc_Obj_t * pFanin, * pTemp;
    int LevelOld, LevFanin, Lev, k, m;
    // check if level has changed
    LevelOld = Abc_ObjReverseLevel(pObjNew);
    if ( LevelOld == Abc_ObjReverseLevelNew(pObjNew) )
        return;
    // start the data structure for level update
    // we cannot fail to visit a node when using this structure because the 
    // nodes are stored by their _old_ levels, which are assumed to be correct
    Vec_VecClear( vLevels );
    Vec_VecPush( vLevels, LevelOld, pObjNew );
    pObjNew->fMarkA = 1;
    // recursively update level
    Vec_VecForEachEntryStart( Abc_Obj_t *, vLevels, pTemp, Lev, k, LevelOld )
    {
        pTemp->fMarkA = 0;
        LevelOld = Abc_ObjReverseLevel(pTemp); 
        assert( LevelOld == Lev );
        Abc_ObjSetReverseLevel( pTemp, Abc_ObjReverseLevelNew(pTemp) );
        // if the level did not change, no need to check the fanout levels
        if ( Abc_ObjReverseLevel(pTemp) == Lev )
            continue;
        // schedule fanins for level update
        Abc_ObjForEachFanin( pTemp, pFanin, m )
        {
            if ( !Abc_ObjIsCi(pFanin) && !pFanin->fMarkA )
            {
                LevFanin = Abc_ObjReverseLevel( pFanin );
                assert( LevFanin >= Lev );
                Vec_VecPush( vLevels, LevFanin, pFanin );
                pFanin->fMarkA = 1;
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    [Replaces the node and incrementally updates levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkUpdate( Abc_Obj_t * pObj, Abc_Obj_t * pObjNew, Vec_Vec_t * vLevels )
{
    // replace the old node by the new node
    pObjNew->Level = pObj->Level;
    Abc_ObjReplace( pObj, pObjNew );
    // update the level of the node
    Abc_NtkUpdateLevel( pObjNew, vLevels );
    Abc_ObjSetReverseLevel( pObjNew, 0 );
    Abc_NtkUpdateReverseLevel( pObjNew, vLevels );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END


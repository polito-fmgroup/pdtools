/**CFile****************************************************************

  FileName    [mem2.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Memory management.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: mem2.h,v 1.1.1.1 2021/09/11 09:57:46 cabodi Exp $]

***********************************************************************/

#ifndef ABC__aig__mem__mem2_h
#define ABC__aig__mem__mem2_h

#include "misc/vec/vec.h"

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Mmr_Flex_t_     Mmr_Flex_t;     
typedef struct Mmr_Fixed_t_    Mmr_Fixed_t;    
typedef struct Mmr_Step_t_     Mmr_Step_t;     

struct Mmr_Flex_t_
{
    int           nPageBase;     // log2 page size in words
    int           PageMask;      // page mask
    int           nEntries;      // entries allocated
    int           iNext;         // next word to be used
    Vec_Ptr_t     vPages;        // memory pages
};

struct Mmr_Fixed_t_
{
    int           nPageBase;     // log2 page size in words
    int           PageMask;      // page mask
    int           nEntryWords;   // entry size in words
    int           nEntries;      // entries allocated
    Vec_Ptr_t     vPages;        // memory pages
    Vec_Int_t     vFrees;        // free entries
};

struct Mmr_Step_t_
{
    Vec_Ptr_t     vLarge;        // memory pages
    int           nMems;         // the number of fixed memory managers employed
    Mmr_Fixed_t * pMems[0];      // memory managers: 2^1 words, 2^2 words, etc
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Mmr_Flex_t * Mmr_FlexStart( int nPageBase )
{
    Mmr_Flex_t * p;
    p = ABC_CALLOC( Mmr_Flex_t, 1 );
    p->nPageBase = nPageBase;
    p->PageMask  = (1 << nPageBase) - 1;
    p->iNext     = (1 << nPageBase);
    return p;
}
static inline void Mmr_FlexStop( Mmr_Flex_t * p )
{
    word * pPage;
    int i;
    if ( 1 )
        printf( "Using %d pages of %d words each. Total memory %.2f MB.\n", 
            Vec_PtrSize(&p->vPages), 1 << p->nPageBase, 
            1.0 * Vec_PtrSize(&p->vPages) * (1 << p->nPageBase) * 8 / (1 << 20) );
    Vec_PtrForEachEntry( word *, &p->vPages, pPage, i )
        ABC_FREE( pPage );
    ABC_FREE( p->vPages.pArray );
    ABC_FREE( p );
}
static inline word * Mmr_FlexEntry( Mmr_Flex_t * p, int h )
{
    assert( h > 0 && h < p->iNext );
    return (word *)Vec_PtrEntry(&p->vPages, (h >> p->nPageBase)) + (h & p->PageMask);
}
static inline int Mmr_FlexFetch( Mmr_Flex_t * p, int nWords )
{
    int hEntry;
    assert( nWords > 0 && nWords < p->PageMask );
    if ( p->iNext + nWords >= p->PageMask )
    {
        Vec_PtrPush( &p->vPages, ABC_FALLOC( word, p->PageMask + 1 ) );
        p->iNext = 1;
    }
    hEntry = ((Vec_PtrSize(&p->vPages) - 1) << p->nPageBase) | p->iNext;
    p->iNext += nWords;
    p->nEntries++;
    return hEntry;
}
static inline void Mmr_FlexRelease( Mmr_Flex_t * p, int h )
{
    assert( h > 0 && h < p->iNext );
    if ( (h >> p->nPageBase) && Vec_PtrEntry(&p->vPages, (h >> p->nPageBase) - 1) )
    {
        word * pPage = (word *)Vec_PtrEntry(&p->vPages, (h >> p->nPageBase) - 1);
        Vec_PtrWriteEntry( &p->vPages, (h >> p->nPageBase) - 1, NULL );
        ABC_FREE( pPage );
    }
}

 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Mmr_Fixed_t * Mmr_FixedStart( int nPageBase, int nEntryWords )
{
    Mmr_Fixed_t * p;
    assert( nEntryWords > 0 && nEntryWords < (1 << nPageBase) );
    p = ABC_CALLOC( Mmr_Fixed_t, 1 );
    p->nPageBase   = nPageBase;
    p->PageMask    = (1 << nPageBase) - 1;
    p->nEntryWords = nEntryWords;
    return p;
}
static inline void Mmr_FixedStop( Mmr_Fixed_t * p )
{
    word * pPage;
    int i;
    if ( 1 )
        printf( "Using %d pages of %d words each. Total memory %.2f MB.\n", 
            Vec_PtrSize(&p->vPages), 1 << p->nPageBase, 
            1.0 * Vec_PtrSize(&p->vPages) * (1 << p->nPageBase) * 8 / (1 << 20) );
    Vec_PtrForEachEntry( word *, &p->vPages, pPage, i )
        ABC_FREE( pPage );
    ABC_FREE( p->vPages.pArray );
    ABC_FREE( p->vFrees.pArray );
    ABC_FREE( p );
}
static inline word * Mmr_FixedEntry( Mmr_Fixed_t * p, int h )
{
    assert( h > 0 && h < ((Vec_PtrSize(&p->vPages) - 1) << p->nPageBase) );
    return (word *)Vec_PtrEntry(&p->vPages, (h >> p->nPageBase)) + (h & p->PageMask);
}
static inline int Mmr_FixedFetch( Mmr_Fixed_t * p )
{
    if ( Vec_IntSize(&p->vFrees) == 0 )
    {
        int i, hEntry = Vec_PtrSize(&p->vPages) << p->nPageBase;
        Vec_PtrPush( &p->vPages, ABC_FALLOC( word, p->PageMask + 1 ) );
        for ( i = 1; i + p->nEntryWords <= p->PageMask; i += p->nEntryWords )
            Vec_IntPush( &p->vFrees, hEntry | i );
        Vec_IntReverseOrder( &p->vFrees );
    }
    p->nEntries++;
    return Vec_IntPop( &p->vFrees );
}
static inline void Mmr_FixedRecycle( Mmr_Fixed_t * p, int h )
{
    memset( Mmr_FixedEntry(p, h), 0xFF, sizeof(word) * p->nEntryWords );
    Vec_IntPush( &p->vFrees, h );
}

 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Mmr_Step_t * Mmr_StepStart( int nWordsMax )
{
    char * pMemory = ABC_CALLOC( char, sizeof(Mmr_Step_t) + sizeof(void *) * (nWordsMax + 1) );
    Mmr_Step_t * p = (Mmr_Step_t *)pMemory;
    p->nMems = nWordsMax + 1;
    return p;
}
static inline void Mmr_StepStop( Mmr_Step_t * p )
{
    word * pPage;
    int i;
    Vec_PtrForEachEntry( word *, &p->vLarge, pPage, i )
        ABC_FREE( pPage );
    ABC_FREE( p->vLarge.pArray );
    ABC_FREE( p );
}
static inline word * Mmr_StepEntry( Mmr_Step_t * p, int nWords, int h )
{
    if ( nWords < p->nMems )
        return Mmr_FixedEntry( p->pMems[nWords], h );
    return (word *)Vec_PtrEntry(&p->vLarge, h);
}
static inline int Mmr_StepFetch( Mmr_Step_t * p, int nWords )
{
    if ( nWords < p->nMems )
        return Mmr_FixedFetch( p->pMems[nWords] );
    Vec_PtrPush( &p->vLarge, ABC_FALLOC( word, nWords ) );
    return Vec_PtrSize( &p->vLarge ) - 1;
}
static inline void Mmr_StepRecycle( Mmr_Step_t * p, int nWords, int h )
{
    void * pPage;
    if ( nWords < p->nMems )
    {
        Mmr_FixedRecycle( p->pMems[nWords], h );
        return;
    }
    pPage = Vec_PtrEntry( &p->vLarge, h );
    ABC_FREE( pPage );
    Vec_PtrWriteEntry( &p->vLarge, h, NULL );
}


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



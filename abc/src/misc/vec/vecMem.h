/**CFile****************************************************************

  FileName    [vecMem.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resizable arrays.]

  Synopsis    [Resizable array of memory pieces.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 20, 2012.]

  Revision    [$Id: vecMem.h,v 1.1.1.1 2021/09/11 09:57:46 cabodi Exp $]

***********************************************************************/
 
#ifndef ABC__misc__vec__vecMem_h
#define ABC__misc__vec__vecMem_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>

ABC_NAMESPACE_HEADER_START

/* 
   This vector stores pieces of memory of the given size.
   It is useful for representing truth tables and any other objects
   of the fixed size.  It is better that Extra_MmFixed because the
   entry IDs can be used as handles to retrieve memory pieces without 
   the need for an array of pointers from entry IDs into memory pieces
   (this can save 8(4) bytes per object on a 64(32)-bit platform).
*/

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Vec_Mem_t_       Vec_Mem_t;
struct Vec_Mem_t_ 
{
    int              nEntrySize;  // entry size (in terms of 8-byte words)
    int              nEntries;    // number of entries currently used
    int              LogPageSze;  // log2 of page size (in terms of entries)
    int              PageMask;    // page mask
    int              nPageAlloc;  // number of pages currently allocated
    int              iPage;       // the number of a page currently used   
    word **          ppPages;     // memory pages
    Vec_Int_t *      vTable;      // hash table
    Vec_Int_t *      vNexts;      // next pointers
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define Vec_MemForEachEntry( p, pEntry, i )                                              \
    for ( i = 0; (i < Vec_MemEntryNum(p)) && ((pEntry) = Vec_MemReadEntry(p, i)); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a memory vector.]

  Description [Entry size is in terms of 8-byte words. Page size is log2
  of the number of entries on one page.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Mem_t * Vec_MemAlloc( int nEntrySize, int LogPageSze )
{
    Vec_Mem_t * p;
    p = ABC_CALLOC( Vec_Mem_t, 1 );
    p->nEntrySize = nEntrySize;
    p->LogPageSze = LogPageSze;
    p->PageMask   = (1 << p->LogPageSze) - 1;
    p->iPage      = -1;
    return p;
}
static inline void Vec_MemFree( Vec_Mem_t * p )
{
    int i;
    for ( i = 0; i <= p->iPage; i++ )
        ABC_FREE( p->ppPages[i] );
    ABC_FREE( p->ppPages );
    ABC_FREE( p );
}
static inline void Vec_MemFreeP( Vec_Mem_t ** p )
{
    if ( *p == NULL )
        return;
    Vec_MemFree( *p );
    *p = NULL;
}
static inline Vec_Mem_t * Vec_MemDup( Vec_Mem_t * pVec )
{
    Vec_Mem_t * p = NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicates the integer array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_MemFill( Vec_Mem_t * pVec, int nEntries )
{
}
static inline void Vec_MemClean( Vec_Mem_t * pVec, int nEntries )
{
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_MemEntrySize( Vec_Mem_t * p )
{
    return p->nEntrySize;
}
static inline int Vec_MemEntryNum( Vec_Mem_t * p )
{
    return p->nEntries;
}
static inline int Vec_MemPageSize( Vec_Mem_t * p )
{
    return p->LogPageSze;
}
static inline int Vec_MemPageNum( Vec_Mem_t * p )
{
    return p->iPage+1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline double Vec_MemMemory( Vec_Mem_t * p )
{
    return (double)sizeof(word) * p->nEntrySize * (1 << p->LogPageSze) * (p->iPage + 1) + (double)sizeof(word *) * p->nPageAlloc + (double)sizeof(Vec_Mem_t);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word * Vec_MemReadEntry( Vec_Mem_t * p, int i )
{
    assert( i >= 0 && i < p->nEntries );
    return p->ppPages[i >> p->LogPageSze] + p->nEntrySize * (i & p->PageMask);
}
static inline word * Vec_MemReadEntryLast( Vec_Mem_t * p )
{
    assert( p->nEntries > 0 );
    return Vec_MemReadEntry( p, p->nEntries-1 );
}
static inline void Vec_MemWriteEntry( Vec_Mem_t * p, int i, word * pEntry )
{
    word * pPlace = Vec_MemReadEntry( p, i );
    memmove( pPlace, pEntry, sizeof(word) * p->nEntrySize );
}
static inline word * Vec_MemGetEntry( Vec_Mem_t * p, int i )
{
    assert( i >= 0 );
    if ( i >= p->nEntries )
    {
        int k, iPageNew = (i >> p->LogPageSze);
        if ( p->iPage < iPageNew )
        {
            // realloc page pointers if needed
            if ( iPageNew >= p->nPageAlloc )
                p->ppPages = ABC_REALLOC( word *, p->ppPages, (p->nPageAlloc = p->nPageAlloc ? 2 * p->nPageAlloc : iPageNew + 32) );
            // allocate new pages if needed
            for ( k = p->iPage + 1; k <= iPageNew; k++ )
                p->ppPages[k] = ABC_ALLOC( word, p->nEntrySize * (1 << p->LogPageSze) );
            // update page counter
            p->iPage = iPageNew;
        }
        // update entry counter
        p->nEntries = i + 1;
    }
    return Vec_MemReadEntry( p, i );
}
static inline void Vec_MemSetEntry( Vec_Mem_t * p, int i, word * pEntry )
{
    word * pPlace = Vec_MemGetEntry( p, i );
    memmove( pPlace, pEntry, sizeof(word) * p->nEntrySize );
}
static inline void Vec_MemPush( Vec_Mem_t * p, word * pEntry )
{
    word * pPlace = Vec_MemGetEntry( p, p->nEntries );
    memmove( pPlace, pEntry, sizeof(word) * p->nEntrySize );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_MemShrink( Vec_Mem_t * p, int nEntriesNew )
{
    int i, iPageOld = p->iPage;
    assert( nEntriesNew <= p->nEntries );
    p->nEntries = nEntriesNew;
    p->iPage = (nEntriesNew >> p->LogPageSze);
    for ( i = p->iPage + 1; i <= iPageOld; i++ )
        ABC_FREE( p->ppPages[i] );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_MemPrint( Vec_Mem_t * p )
{
    word * pEntry;
    int i;
    printf( "Memory vector has %d entries: ", Vec_MemEntryNum(p) );
    Vec_MemForEachEntry( p, pEntry, i )
    {
        printf( "%3d : ", i );
        // add printout here
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Hashing entries in the memory vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_MemHashAlloc( Vec_Mem_t * p, int nTableSize )
{
    assert( p->vTable == NULL && p->vNexts == NULL );
    p->vTable = Vec_IntStartFull( Abc_PrimeCudd(nTableSize) );
    p->vNexts = Vec_IntAlloc( nTableSize );
}
static inline void Vec_MemHashFree( Vec_Mem_t * p )
{
    Vec_IntFreeP( &p->vTable );
    Vec_IntFreeP( &p->vNexts );
}
static inline unsigned Vec_MemHashKey( Vec_Mem_t * p, word * pEntry )
{
    static int s_Primes[8] = { 1699, 4177, 5147, 5647, 6343, 7103, 7873, 8147 };
    int i, nData = 2 * p->nEntrySize;
    unsigned * pData = (unsigned *)pEntry;
    unsigned uHash = 0;
    for ( i = 0; i < nData; i++ )
        uHash += pData[i] * s_Primes[i & 0x7];
    return uHash % Vec_IntSize(p->vTable);
}
static int * Vec_MemHashLookup( Vec_Mem_t * p, word * pEntry )
{
    int * pSpot = Vec_IntEntryP( p->vTable, Vec_MemHashKey(p, pEntry) );
    for ( ; *pSpot != -1; pSpot = Vec_IntEntryP(p->vNexts, *pSpot) )
        if ( !memcmp( Vec_MemReadEntry(p, *pSpot), pEntry, sizeof(word) * p->nEntrySize ) ) // equal
            return pSpot;
    return pSpot;
}
static void Vec_MemHashResize( Vec_Mem_t * p )
{
    word * pEntry;
    int i, * pSpot;
    Vec_IntFill( p->vTable, Abc_PrimeCudd(2 * Vec_IntSize(p->vTable)), -1 );
    Vec_IntClear( p->vNexts );
    Vec_MemForEachEntry( p, pEntry, i )
    {
        pSpot = Vec_MemHashLookup( p, pEntry );
        assert( *pSpot == -1 );
        *pSpot = Vec_IntSize(p->vNexts);
        Vec_IntPush( p->vNexts, -1 );
    }
    assert( p->nEntries == Vec_IntSize(p->vNexts) );
}
static int Vec_MemHashInsert( Vec_Mem_t * p, word * pEntry )
{
    int * pSpot;
    if ( p->nEntries > Vec_IntSize(p->vTable) )
        Vec_MemHashResize( p );
    pSpot = Vec_MemHashLookup( p, pEntry );
    if ( *pSpot != -1 )
        return *pSpot;
    *pSpot = Vec_IntSize(p->vNexts);
    Vec_IntPush( p->vNexts, -1 );
    Vec_MemPush( p, pEntry );
    assert( p->nEntries == Vec_IntSize(p->vNexts) );
    return Vec_IntSize(p->vNexts) - 1;
}


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


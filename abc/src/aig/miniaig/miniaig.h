/**CFile****************************************************************

  FileName    [miniaig.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Minimalistic AIG package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 29, 2012.]

  Revision    [$Id: miniaig.h,v 1.1.1.1 2021/09/11 09:57:46 cabodi Exp $]

***********************************************************************/
 
#ifndef MINI_AIG__mini_aig_h
#define MINI_AIG__mini_aig_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define MINI_AIG_NULL       (0x7FFFFFFF)
#define MINI_AIG_START_SIZE (0x000000FF)

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Mini_Aig_t_       Mini_Aig_t;
struct Mini_Aig_t_ 
{
    int           nCap;
    int           nSize;
    int           nRegs;
    int *         pArray;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

// memory management
#define MINI_AIG_ALLOC(type, num)     ((type *) malloc(sizeof(type) * (num)))
#define MINI_AIG_CALLOC(type, num)    ((type *) calloc((num), sizeof(type)))
#define MINI_AIG_FALLOC(type, num)    ((type *) memset(malloc(sizeof(type) * (num)), 0xff, sizeof(type) * (num)))
#define MINI_AIG_FREE(obj)            ((obj) ? (free((char *) (obj)), (obj) = 0) : 0)
#define MINI_AIG_REALLOC(type, obj, num) \
        ((obj) ? ((type *) realloc((char *)(obj), sizeof(type) * (num))) : \
         ((type *) malloc(sizeof(type) * (num))))

// internal procedures
static void Mini_AigGrow( Mini_Aig_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = MINI_AIG_REALLOC( int, p->pArray, nCapMin ); 
    assert( p->pArray );
    p->nCap   = nCapMin;
}
static void Mini_AigPush( Mini_Aig_t * p, int Lit0, int Lit1 )
{
    if ( p->nSize + 2 > p->nCap )
    {
        if ( p->nCap < MINI_AIG_START_SIZE )
            Mini_AigGrow( p, MINI_AIG_START_SIZE );
        else
            Mini_AigGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Lit0;
    p->pArray[p->nSize++] = Lit1;
}

// accessing fanins
static int Mini_AigNodeFanin0( Mini_Aig_t * p, int Id )
{
    assert( Id >= 0 && 2*Id < p->nSize );
    return p->pArray[2*Id];
}
static int Mini_AigNodeFanin1( Mini_Aig_t * p, int Id )
{
    assert( Id >= 0 && 2*Id < p->nSize );
    return p->pArray[2*Id+1];
}

// working with variables and literals
static int      Mini_AigVar2Lit( int Var, int fCompl )         { return Var + Var + fCompl;   }
static int      Mini_AigLit2Var( int Lit )                     { return Lit >> 1;             }
static int      Mini_AigLitIsCompl( int Lit )                  { return Lit & 1;              }
static int      Mini_AigLitNot( int Lit )                      { return Lit ^ 1;              }
static int      Mini_AigLitNotCond( int Lit, int c )           { return Lit ^ (int)(c > 0);   }
static int      Mini_AigLitRegular( int Lit )                  { return Lit & ~01;            }

static int      Mini_AigLitConst0()                            { return 0;                    }
static int      Mini_AigLitConst1()                            { return 1;                    }
static int      Mini_AigLitIsConst0( int Lit )                 { return Lit == 0;             }
static int      Mini_AigLitIsConst1( int Lit )                 { return Lit == 1;             }
static int      Mini_AigLitIsConst( int Lit )                  { return Lit == 0 || Lit == 1; }

static int      Mini_AigNodeNum( Mini_Aig_t * p )              { return p->nSize/2;           }
static int      Mini_AigNodeIsConst( Mini_Aig_t * p, int Id )  { assert( Id >= 0 ); return Id == 0; }
static int      Mini_AigNodeIsPi( Mini_Aig_t * p, int Id )     { assert( Id >= 0 ); return Id > 0 && Mini_AigNodeFanin0( p, Id ) == MINI_AIG_NULL; }
static int      Mini_AigNodeIsPo( Mini_Aig_t * p, int Id )     { assert( Id >= 0 ); return Id > 0 && Mini_AigNodeFanin0( p, Id ) != MINI_AIG_NULL && Mini_AigNodeFanin1( p, Id ) == MINI_AIG_NULL; }
static int      Mini_AigNodeIsAnd( Mini_Aig_t * p, int Id )    { assert( Id >= 0 ); return Id > 0 && Mini_AigNodeFanin0( p, Id ) != MINI_AIG_NULL && Mini_AigNodeFanin1( p, Id ) != MINI_AIG_NULL; }

// working with sequential AIGs
static int      Mini_AigRegNum( Mini_Aig_t * p )               { return p->nRegs;             }
static void     Mini_AigSetRegNum( Mini_Aig_t * p, int n )     { p->nRegs = n;                }

// iterators through objects
#define Mini_AigForEachPi( p, i )  for (i = 1; i < Mini_AigNodeNum(p); i++) if ( !Mini_AigNodeIsPi(p, i) ) {} else 
#define Mini_AigForEachPo( p, i )  for (i = 1; i < Mini_AigNodeNum(p); i++) if ( !Mini_AigNodeIsPo(p, i) ) {} else 
#define Mini_AigForEachAnd( p, i ) for (i = 1; i < Mini_AigNodeNum(p); i++) if ( !Mini_AigNodeIsAnd(p, i) ) {} else


// constructor/destructor
static Mini_Aig_t * Mini_AigStart()
{
    Mini_Aig_t * p;
    p = MINI_AIG_CALLOC( Mini_Aig_t, 1 );
    p->nCap   = MINI_AIG_START_SIZE;
    p->pArray = MINI_AIG_ALLOC( int, p->nCap );
    Mini_AigPush( p, MINI_AIG_NULL, MINI_AIG_NULL );
    return p;
}
static void Mini_AigStop( Mini_Aig_t * p )
{
    MINI_AIG_FREE( p->pArray );
    MINI_AIG_FREE( p );
}
static void Mini_AigPrintStats( Mini_Aig_t * p )
{
    int i, nPis, nPos, nNodes;
    nPis = 0;
    Mini_AigForEachPi( p, i )
        nPis++;
    nPos = 0;
    Mini_AigForEachPo( p, i )
        nPos++;
    nNodes = 0;
    Mini_AigForEachAnd( p, i )
        nNodes++;
    printf( "PI = %d. PO = %d. Node = %d.\n", nPis, nPos, nNodes );
}

// serialization
static void Mini_AigDump( Mini_Aig_t * p, char * pFileName )
{
    FILE * pFile;
    int RetValue;
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file for writing \"%s\".\n", pFileName );
        return;
    }
    RetValue = fwrite( &p->nSize, sizeof(int), 1, pFile );
    RetValue = fwrite( &p->nRegs, sizeof(int), 1, pFile );
    RetValue = fwrite( p->pArray, sizeof(int), p->nSize, pFile );
    fclose( pFile );
}
static Mini_Aig_t * Mini_AigLoad( char * pFileName )
{
    Mini_Aig_t * p;
    FILE * pFile;
    int RetValue, nSize;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file for reading \"%s\".\n", pFileName );
        return NULL;
    }
    RetValue = fread( &nSize, sizeof(int), 1, pFile );
    p = MINI_AIG_CALLOC( Mini_Aig_t, 1 );
    p->nSize = p->nCap = nSize;
    p->pArray = MINI_AIG_ALLOC( int, p->nCap );
    RetValue = fread( &p->nRegs, sizeof(int), 1, pFile );
    RetValue = fread( p->pArray, sizeof(int), p->nSize, pFile );
    fclose( pFile );
    return p;
}


// creating nodes 
// (constant node is created when AIG manager is created)
static int Mini_AigCreatePi( Mini_Aig_t * p )
{
    int Lit = p->nSize;
    Mini_AigPush( p, MINI_AIG_NULL, MINI_AIG_NULL );
    return Lit;
}
static int Mini_AigCreatePo( Mini_Aig_t * p, int Lit0 )
{
    int Lit = p->nSize;
    assert( Lit0 >= 0 && Lit0 < Lit );
    Mini_AigPush( p, Lit0, MINI_AIG_NULL );
    return Lit;
}

// boolean operations
static int Mini_AigAnd( Mini_Aig_t * p, int Lit0, int Lit1 )
{
    int Lit = p->nSize;
    assert( Lit0 >= 0 && Lit0 < Lit );
    assert( Lit1 >= 0 && Lit1 < Lit );
    Mini_AigPush( p, Lit0, Lit1 );
    return Lit;
}
static int Mini_AigOr( Mini_Aig_t * p, int Lit0, int Lit1 )
{
    return Mini_AigLitNot( Mini_AigAnd( p, Mini_AigLitNot(Lit0), Mini_AigLitNot(Lit1) ) );
}
static int Mini_AigMux( Mini_Aig_t * p, int LitC, int Lit1, int Lit0 )
{
    int Res0 = Mini_AigAnd( p, LitC, Lit1 );
    int Res1 = Mini_AigAnd( p, Mini_AigLitNot(LitC), Lit0 );
    return Mini_AigOr( p, Res0, Res1 );
}
static int Mini_AigXor( Mini_Aig_t * p, int Lit0, int Lit1 )
{
    return Mini_AigMux( p, Lit0, Mini_AigLitNot(Lit1), Lit1 );
}

// procedure to check the topological order during AIG construction
static int Mini_AigCheck( Mini_Aig_t * p )
{
    int status = 1;
    int i, iFaninLit0, iFaninLit1;
    Mini_AigForEachAnd( p, i )
    {
        // get the fanin literals of this AND node
        iFaninLit0 = Mini_AigNodeFanin0( p, i );
        iFaninLit1 = Mini_AigNodeFanin1( p, i );
        // compare the fanin literals with the literal of the current node (2 * i)
        if ( iFaninLit0 >= 2 * i )
            printf( "Fanin0 of AND node %d is not in a topological order.\n", i ), status = 0;
        if ( iFaninLit1 >= 2 * i )
            printf( "Fanin0 of AND node %d is not in a topological order.\n", i ), status = 0;
    }
    Mini_AigForEachPo( p, i )
    {
        // get the fanin literal of this PO node
        iFaninLit0 = Mini_AigNodeFanin0( p, i );
        // compare the fanin literal with the literal of the current node (2 * i)
        if ( iFaninLit0 >= 2 * i )
            printf( "Fanin0 of PO node %d is not in a topological order.\n", i ), status = 0;
    }
    return status;
}



////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


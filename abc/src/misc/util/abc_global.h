/**CFile****************************************************************

  FileName    [abc_global.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Global declarations.]

  Synopsis    [Global declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Jan 30, 2009.]

  Revision    [$Id: abc_global.h,v 1.1.1.1 2021/09/11 09:57:46 cabodi Exp $]

***********************************************************************/

#ifndef ABC__misc__util__abc_global_h
#define ABC__misc__util__abc_global_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#define inline __inline // compatible with MS VS 6.0
#pragma warning(disable : 4152) // warning C4152: nonstandard extension, function/data pointer conversion in expression
#pragma warning(disable : 4200) // warning C4200: nonstandard extension used : zero-sized array in struct/union
#pragma warning(disable : 4244) // warning C4244: '+=' : conversion from 'int ' to 'unsigned short ', possible loss of data
#pragma warning(disable : 4514) // warning C4514: 'Vec_StrPop' : unreferenced inline function has been removed
#pragma warning(disable : 4710) // warning C4710: function 'Vec_PtrGrow' not inlined
//#pragma warning( disable : 4273 )
#endif

#ifdef WIN32
  #ifdef WIN32_NO_DLL
    #define ABC_DLLEXPORT
    #define ABC_DLLIMPORT
  #else
    #define ABC_DLLEXPORT __declspec(dllexport)
    #define ABC_DLLIMPORT __declspec(dllimport)
  #endif
#else  /* defined(WIN32) */
#define ABC_DLLIMPORT
#endif /* defined(WIN32) */

#ifndef ABC_DLL
#define ABC_DLL ABC_DLLIMPORT
#endif

// catch memory leaks in Visual Studio
#ifdef WIN32
  #ifdef _DEBUG
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
  #endif
#endif

#if !defined(___unused)
#if defined(__GNUC__)
#define ___unused __attribute__ ((__unused__))
#else
#define ___unused
#endif
#endif

/*
#ifdef __cplusplus
#error "C++ code"
#else
#error "C code"
#endif
*/

#include <time.h>
#include <stdarg.h>



////////////////////////////////////////////////////////////////////////
///                         NAMESPACES                               ///
////////////////////////////////////////////////////////////////////////

//#define ABC_NAMESPACE xxx

#ifdef __cplusplus
#  ifdef ABC_NAMESPACE
#    define ABC_NAMESPACE_HEADER_START namespace ABC_NAMESPACE {
#    define ABC_NAMESPACE_HEADER_END }
#    define ABC_NAMESPACE_IMPL_START namespace ABC_NAMESPACE {
#    define ABC_NAMESPACE_IMPL_END }
#    define ABC_NAMESPACE_PREFIX ABC_NAMESPACE::
#  else
#    define ABC_NAMESPACE_HEADER_START extern "C" {
#    define ABC_NAMESPACE_HEADER_END }
#    define ABC_NAMESPACE_IMPL_START
#    define ABC_NAMESPACE_IMPL_END
#    define ABC_NAMESPACE_PREFIX
#  endif // #ifdef ABC_NAMESPACE
#else
#  define ABC_NAMESPACE_HEADER_START
#  define ABC_NAMESPACE_HEADER_END
#  define ABC_NAMESPACE_IMPL_START
#  define ABC_NAMESPACE_IMPL_END
#  define ABC_NAMESPACE_PREFIX
#endif // #ifdef __cplusplus

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

/**
 * Pointer difference type; replacement for ptrdiff_t.
 * This is a signed integral type that is the same size as a pointer.
 * NOTE: This type may be different sizes on different platforms.
 */
#if       defined(__ccdoc__)
typedef platform_dependent_type ABC_PTRDIFF_T;
#elif     defined(LIN64)
typedef long ABC_PTRDIFF_T;
#elif     defined(NT64)
typedef long long ABC_PTRDIFF_T;
#elif     defined(NT) || defined(LIN) || defined(WIN32)
typedef int ABC_PTRDIFF_T;
#else
   #error unknown platform
#endif /* defined(PLATFORM) */

/**
 * Unsigned integral type that can contain a pointer.
 * This is an unsigned integral type that is the same size as a pointer.
 * NOTE: This type may be different sizes on different platforms.
 */
#if       defined(__ccdoc__)
typedef platform_dependent_type ABC_PTRUINT_T;
#elif     defined(LIN64)
typedef unsigned long ABC_PTRUINT_T;
#elif     defined(NT64)
typedef unsigned long long ABC_PTRUINT_T;
#elif     defined(NT) || defined(LIN) || defined(WIN32)
typedef unsigned int ABC_PTRUINT_T;
#else
   #error unknown platform
#endif /* defined(PLATFORM) */

/**
 * Signed integral type that can contain a pointer.
 * This is a signed integral type that is the same size as a pointer.
 * NOTE: This type may be different sizes on different platforms.
 */
#if       defined(__ccdoc__)
typedef platform_dependent_type ABC_PTRINT_T;
#elif     defined(LIN64)
typedef long ABC_PTRINT_T;
#elif     defined(NT64)
typedef long long ABC_PTRINT_T;
#elif     defined(NT) || defined(LIN) || defined(WIN32)
typedef int ABC_PTRINT_T;
#else
   #error unknown platform
#endif /* defined(PLATFORM) */

/**
 * 64-bit signed integral type.
 */
#if       defined(__ccdoc__)
typedef platform_dependent_type ABC_INT64_T;
#elif     defined(LIN64)
typedef long ABC_INT64_T;
#elif     defined(NT64) || defined(LIN)
typedef long long ABC_INT64_T;
#elif     defined(WIN32) || defined(NT)
typedef signed __int64 ABC_INT64_T;
#else
   #error unknown platform
#endif /* defined(PLATFORM) */

/**
 * 64-bit unsigned integral type.
 */
#if       defined(__ccdoc__)
typedef platform_dependent_type ABC_UINT64_T;
#elif     defined(LIN64)
typedef unsigned long ABC_UINT64_T;
#elif     defined(NT64) || defined(LIN)
typedef unsigned long long ABC_UINT64_T;
#elif     defined(WIN32) || defined(NT)
typedef unsigned __int64 ABC_UINT64_T;
#else
   #error unknown platform
#endif /* defined(PLATFORM) */

#ifdef LIN
  #define ABC_CONST(number) number ## ULL 
#else // LIN64 and windows
  #define ABC_CONST(number) number
#endif

typedef ABC_UINT64_T word;
typedef ABC_INT64_T iword;

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define ABC_INFINITY    (100000000)

#define ABC_SWAP(Type, a, b)  { Type t = a; a = b; b = t; }

#define ABC_PRT(a,t)    (Abc_Print(1, "%s =", (a)), Abc_Print(1, "%9.2f sec\n", 1.0*(t)/(CLOCKS_PER_SEC)))
#define ABC_PRTr(a,t)   (Abc_Print(1, "%s =", (a)), Abc_Print(1, "%9.2f sec\r", 1.0*(t)/(CLOCKS_PER_SEC)))
#define ABC_PRTn(a,t)   (Abc_Print(1, "%s =", (a)), Abc_Print(1, "%9.2f sec  ", 1.0*(t)/(CLOCKS_PER_SEC)))
#define ABC_PRTP(a,t,T) (Abc_Print(1, "%s =", (a)), Abc_Print(1, "%9.2f sec (%6.2f %%)\n", 1.0*(t)/(CLOCKS_PER_SEC), (T)? 100.0*(t)/(T) : 0.0))
#define ABC_PRM(a,f)    (Abc_Print(1, "%s =", (a)), Abc_Print(1, "%10.3f MB\n",    1.0*(f)/(1<<20)))
#define ABC_PRMr(a,f)   (Abc_Print(1, "%s =", (a)), Abc_Print(1, "%10.3f MB\r",    1.0*(f)/(1<<20)))
#define ABC_PRMn(a,f)   (Abc_Print(1, "%s =", (a)), Abc_Print(1, "%10.3f MB  ",    1.0*(f)/(1<<20)))
#define ABC_PRMP(a,f,F) (Abc_Print(1, "%s =", (a)), Abc_Print(1, "%10.3f MB (%6.2f %%)\n",  (1.0*(f)/(1<<20)), ((F)? 100.0*(f)/(F) : 0.0) ) )

#define ABC_ALLOC(type, num)     ((type *) malloc(sizeof(type) * (num)))
#define ABC_CALLOC(type, num)    ((type *) calloc((num), sizeof(type)))
#define ABC_FALLOC(type, num)    ((type *) memset(malloc(sizeof(type) * (num)), 0xff, sizeof(type) * (num)))
#define ABC_FREE(obj)            ((obj) ? (free((char *) (obj)), (obj) = 0) : 0)
#define ABC_REALLOC(type, obj, num) \
        ((obj) ? ((type *) realloc((char *)(obj), sizeof(type) * (num))) : \
         ((type *) malloc(sizeof(type) * (num))))

static inline int      Abc_AbsInt( int a        )             { return a < 0 ? -a : a; }
static inline int      Abc_MaxInt( int a, int b )             { return a > b ?  a : b; }
static inline int      Abc_MinInt( int a, int b )             { return a < b ?  a : b; }
static inline word     Abc_MaxWord( word a, word b )          { return a > b ?  a : b; }
static inline word     Abc_MinWord( word a, word b )          { return a < b ?  a : b; }
static inline float    Abc_AbsFloat( float a          )       { return a < 0 ? -a : a; }
static inline float    Abc_MaxFloat( float a, float b )       { return a > b ?  a : b; }
static inline float    Abc_MinFloat( float a, float b )       { return a < b ?  a : b; }
static inline double   Abc_AbsDouble( double a           )    { return a < 0 ? -a : a; }
static inline double   Abc_MaxDouble( double a, double b )    { return a > b ?  a : b; }
static inline double   Abc_MinDouble( double a, double b )    { return a < b ?  a : b; }

static inline int      Abc_Float2Int( float Val )             { union { int x; float y; } v; v.y = Val; return v.x;         }
static inline float    Abc_Int2Float( int Num )               { union { int x; float y; } v; v.x = Num; return v.y;         }
static inline int      Abc_Base2Log( unsigned n )             { int r; if ( n < 2 ) return n; for ( r = 0, n--; n; n >>= 1, r++ ); return r; }
static inline int      Abc_Base10Log( unsigned n )            { int r; if ( n < 2 ) return n; for ( r = 0, n--; n; n /= 10, r++ ); return r; }
static inline int      Abc_Base16Log( unsigned n )            { int r; if ( n < 2 ) return n; for ( r = 0, n--; n; n /= 16, r++ ); return r; }
static inline char *   Abc_UtilStrsav( char * s )             { return s ? strcpy(ABC_ALLOC(char, strlen(s)+1), s) : NULL;  }
static inline int      Abc_BitWordNum( int nBits )            { return (nBits>>5) + ((nBits&31) > 0);                       }
static inline int      Abc_TruthWordNum( int nVars )          { return nVars <= 5 ? 1 : (1 << (nVars - 5));                 }
static inline int      Abc_Truth6WordNum( int nVars )         { return nVars <= 6 ? 1 : (1 << (nVars - 6));                 }
static inline int      Abc_InfoHasBit( unsigned * p, int i )  { return (p[(i)>>5] & (1<<((i) & 31))) > 0;                   }
static inline void     Abc_InfoSetBit( unsigned * p, int i )  { p[(i)>>5] |= (1<<((i) & 31));                               }
static inline void     Abc_InfoXorBit( unsigned * p, int i )  { p[(i)>>5] ^= (1<<((i) & 31));                               }
static inline unsigned Abc_InfoMask( int nVar )               { return (~(unsigned)0) >> (32-nVar);                         }

static inline int      Abc_Var2Lit( int Var, int fCompl )     { return Var + Var + fCompl; }
static inline int      Abc_Lit2Var( int Lit )                 { return Lit >> 1;           }
static inline int      Abc_LitIsCompl( int Lit )              { return Lit & 1;            }
static inline int      Abc_LitNot( int Lit )                  { return Lit ^ 1;            }
static inline int      Abc_LitNotCond( int Lit, int c )       { return Lit ^ (int)(c > 0); }
static inline int      Abc_LitRegular( int Lit )              { return Lit & ~01;          }
static inline int      Abc_Lit2LitV( int * pMap, int Lit )    { return Abc_Var2Lit( pMap[Abc_Lit2Var(Lit)], Abc_LitIsCompl(Lit) );   }
static inline int      Abc_Lit2LitL( int * pMap, int Lit )    { return Abc_LitNotCond( pMap[Abc_Lit2Var(Lit)], Abc_LitIsCompl(Lit) );   }

enum Abc_VerbLevel
{
    ABC_PROMPT   = -2,
    ABC_ERROR    = -1,
    ABC_WARNING  =  0,
    ABC_STANDARD =  1,
    ABC_VERBOSE  =  2
};

#define BRIDGE_NETLIST           106
#define BRIDGE_ABS_NETLIST       107

// string printing
extern char * vnsprintf(const char* format, va_list args);
extern char * nsprintf(const char* format, ...);
extern int Gia_ManToBridgeText( FILE * pFile, int Size, unsigned char * pBuffer );
extern int Gia_ManToBridgeAbsNetlist( FILE * pFile, void * p, int pkg_type );

// misc printing procedures

// GpC: use pdtrav output (done fopr HWMCC competitions 
extern FILE *pdtStdout;
#ifdef stdout
#undef stdout
#endif
#define stdout pdtStdout

static inline void Abc_Print( int level, const char * format, ... )
{
    extern ABC_DLL int Abc_FrameIsBridgeMode();
    va_list args;

    if ( ! Abc_FrameIsBridgeMode() ){
        if ( level == ABC_ERROR )
	  fprintf(stdout, "Error: " );
        else if ( level == ABC_WARNING )
	  fprintf(stdout, "Warning: " );
    }else{
        if ( level == ABC_ERROR )
            Gia_ManToBridgeText( stdout, strlen("Error: "), (unsigned char*)"Error: " );
        else if ( level == ABC_WARNING )
            Gia_ManToBridgeText( stdout, strlen("Warning: "), (unsigned char*)"Warning: " );
    }

    va_start( args, format );
    if ( Abc_FrameIsBridgeMode() )
    {
        char * tmp = vnsprintf( format, args );
        Gia_ManToBridgeText( stdout, strlen(tmp), (unsigned char*)tmp );
        free( tmp );
    }
    else
      vfprintf(stdout, format, args );
    va_end( args );
}

static inline void Abc_PrintInt( int i )
{
    double v3 = (double)i/1000;
    double v6 = (double)i/1000000;

    Abc_Print( 1,  "  " );

    if ( i > -1000 && i < 1000 )
        Abc_Print( 1, " %4d", i );

    else if ( v3 > -9.995 && v3 < 9.995 )
        Abc_Print( 1, "%4.2fk", v3 );
    else if ( v3 > -99.95 && v3 < 99.95 )
        Abc_Print( 1, "%4.1fk", v3 );
    else if ( v3 > -999.5 && v3 < 999.5 )
        Abc_Print( 1, "%4.0fk", v3 );

    else if ( v6 > -9.995 && v6 < 9.995 )
        Abc_Print( 1, "%4.2fm", v6 );
    else if ( v6 > -99.95 && v6 < 99.95 )
        Abc_Print( 1, "%4.1fm", v6 );
    else if ( v6 > -999.5 && v6 < 999.5 )
        Abc_Print( 1, "%4.0fm", v6 );
}

static inline void Abc_PrintTime( int level, const char * pStr, clock_t time )
{
    ABC_PRT( pStr, time );
}

static inline void Abc_PrintTimeP( int level, const char * pStr, clock_t time, clock_t Time )
{
    ABC_PRTP( pStr, time, Time );
}

static inline void Abc_PrintMemoryP( int level, const char * pStr, int mem, int Mem )
{
    ABC_PRMP( pStr, mem, Mem );
}

// Returns the next prime >= p
static inline int Abc_PrimeCudd( unsigned int p )
{
    int i,pn;
    p--;
    do {
        p++;
        if (p&1)
        {
            pn = 1;
            i = 3;
            while ((unsigned) (i * i) <= p)
            {
                if (p % i == 0) {
                    pn = 0;
                    break;
                }
                i += 2;
            }
        }
        else
            pn = 0;
    } while (!pn);
    return(p);

} // end of Cudd_Prime 

// sorting
extern void   Abc_MergeSort( int * pInput, int nSize );
extern int *  Abc_MergeSortCost( int * pCosts, int nSize );
extern void   Abc_QuickSort1( word * pData, int nSize, int fDecrease );
extern void   Abc_QuickSort2( word * pData, int nSize, int fDecrease );
extern void   Abc_QuickSort3( word * pData, int nSize, int fDecrease );
extern void   Abc_QuickSortCostData( int * pCosts, int nSize, int fDecrease, word * pData, int * pResult );
extern int *  Abc_QuickSortCost( int * pCosts, int nSize, int fDecrease );



ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

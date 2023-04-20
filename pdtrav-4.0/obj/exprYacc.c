/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.5.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         ExprYyparse
#define yylex           ExprYylex
#define yyerror         ExprYyerror
#define yydebug         ExprYydebug
#define yynerrs         ExprYynerrs
#define yylval          ExprYylval
#define yychar          ExprYychar

/* First part of user prologue.  */
#line 1 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"

/**CFile***********************************************************************

  FileName    [expr.y]

  PackageName [expr]

  Synopsis    [Yacc file for PdTrav expressions.]

  Description []

  SeeAlso     []

  Author      [Gianpiero Cabodi and Stefano Quer]

  Copyright [  Copyright (c) 2001 by Politecnico di Torino.
    All Rights Reserved. This software is for educational purposes only.
    Permission is given to academic institutions to use, copy, and modify
    this software and its documentation provided that this introductory
    message is not removed, that this software and its documentation is
    used for the institutions' internal research and educational purposes,
    and that no monies are exchanged. No guarantee is expressed or implied
    by the distribution of this code.
    Send bug-reports and/or questions to: pdtrav@polito.it. ]

******************************************************************************/

#include "exprInt.h"

/*
 * The following is a workaround for a bug in bison, which generates code
 * that uses alloca().  Their lengthy sequence of #ifdefs for defining
 * alloca() does the wrong thing for HPUX (it should do what is defined below)
 */
#ifdef __hpux
#  include <alloca.h>
#endif
 

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

extern Ddi_Expr_t *ExprYySpecif;
extern Ddi_Mgr_t *ExprYyDdmgr;
extern Ddi_Expr_t *ExprYySpecif;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#define verbose(s) fprintf(stdout,s)

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static Ddi_Expr_t *new_node(Expr_Ctl_Code_e opcode, 
  Ddi_Expr_t * op1, Ddi_Expr_t * op2);
static Ddi_Expr_t *new_leaf(char *s);

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

static void ExprYyerror(char *s)
{
  fprintf(stderr, "%s\n", s);
}

static int ExprYywrap(void)
{
  return(1);
}                                                                               


#line 184 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int ExprYydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    PROCESS = 258,
    EU = 259,
    AU = 260,
    EBU = 261,
    ABU = 262,
    MINU = 263,
    MAXU = 264,
    DEFINE = 265,
    SPEC = 266,
    COMPUTE = 267,
    INVARSPEC = 268,
    ASSIGN = 269,
    INPUT = 270,
    OUTPUT = 271,
    BOOLEAN = 272,
    ARRAY = 273,
    OF = 274,
    SCALAR = 275,
    SEMI = 276,
    LP = 277,
    RP = 278,
    LB = 279,
    RB = 280,
    LCB = 281,
    RCB = 282,
    LLCB = 283,
    RRCB = 284,
    EQDEF = 285,
    TWODOTS = 286,
    FALSEEXP = 287,
    TRUEEXP = 288,
    SELF = 289,
    SIGMA = 290,
    CASE = 291,
    ESAC = 292,
    COLON = 293,
    ATOM = 294,
    NUMBER = 295,
    QUOTE = 296,
    COMMA = 297,
    IMPLIES = 298,
    IFF = 299,
    OR = 300,
    AND = 301,
    NOT = 302,
    EX = 303,
    AX = 304,
    EF = 305,
    AF = 306,
    EG = 307,
    AG = 308,
    EE = 309,
    AA = 310,
    UNTIL = 311,
    EBF = 312,
    EBG = 313,
    ABF = 314,
    ABG = 315,
    BUNTIL = 316,
    MMIN = 317,
    MMAX = 318,
    EQUAL = 319,
    LT = 320,
    GT = 321,
    LE = 322,
    GE = 323,
    UNION = 324,
    SETIN = 325,
    MOD = 326,
    PLUS = 327,
    MINUS = 328,
    TIMES = 329,
    DIVIDE = 330,
    UMINUS = 331,
    NEXT = 332,
    DOT = 333
  };
#endif
/* Tokens.  */
#define PROCESS 258
#define EU 259
#define AU 260
#define EBU 261
#define ABU 262
#define MINU 263
#define MAXU 264
#define DEFINE 265
#define SPEC 266
#define COMPUTE 267
#define INVARSPEC 268
#define ASSIGN 269
#define INPUT 270
#define OUTPUT 271
#define BOOLEAN 272
#define ARRAY 273
#define OF 274
#define SCALAR 275
#define SEMI 276
#define LP 277
#define RP 278
#define LB 279
#define RB 280
#define LCB 281
#define RCB 282
#define LLCB 283
#define RRCB 284
#define EQDEF 285
#define TWODOTS 286
#define FALSEEXP 287
#define TRUEEXP 288
#define SELF 289
#define SIGMA 290
#define CASE 291
#define ESAC 292
#define COLON 293
#define ATOM 294
#define NUMBER 295
#define QUOTE 296
#define COMMA 297
#define IMPLIES 298
#define IFF 299
#define OR 300
#define AND 301
#define NOT 302
#define EX 303
#define AX 304
#define EF 305
#define AF 306
#define EG 307
#define AG 308
#define EE 309
#define AA 310
#define UNTIL 311
#define EBF 312
#define EBG 313
#define ABF 314
#define ABG 315
#define BUNTIL 316
#define MMIN 317
#define MMAX 318
#define EQUAL 319
#define LT 320
#define GT 321
#define LE 322
#define GE 323
#define UNION 324
#define SETIN 325
#define MOD 326
#define PLUS 327
#define MINUS 328
#define TIMES 329
#define DIVIDE 330
#define UMINUS 331
#define NEXT 332
#define DOT 333

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 114 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"

  Ddi_Expr_t *node;
  char *id;

#line 394 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE ExprYylval;

int ExprYyparse (void);





#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))

/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  41
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   750

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  79
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  10
/* YYNRULES -- Number of rules.  */
#define YYNRULES  83
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  178

#define YYUNDEFTOK  2
#define YYMAXUTOK   333


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   166,   166,   167,   168,   169,   171,   175,   176,   179,
     186,   191,   203,   206,   207,   210,   217,   218,   219,   220,
     221,   222,   225,   228,   231,   234,   237,   240,   243,   246,
     249,   252,   255,   258,   261,   264,   267,   270,   273,   276,
     279,   282,   286,   287,   292,   295,   303,   304,   305,   306,
     307,   308,   311,   314,   317,   320,   323,   326,   329,   332,
     335,   338,   341,   344,   347,   350,   353,   356,   359,   362,
     367,   370,   373,   376,   379,   382,   385,   388,   391,   394,
     397,   400,   403,   406
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "PROCESS", "EU", "AU", "EBU", "ABU",
  "MINU", "MAXU", "DEFINE", "SPEC", "COMPUTE", "INVARSPEC", "ASSIGN",
  "INPUT", "OUTPUT", "BOOLEAN", "ARRAY", "OF", "SCALAR", "SEMI", "LP",
  "RP", "LB", "RB", "LCB", "RCB", "LLCB", "RRCB", "EQDEF", "TWODOTS",
  "FALSEEXP", "TRUEEXP", "SELF", "SIGMA", "CASE", "ESAC", "COLON", "ATOM",
  "NUMBER", "QUOTE", "COMMA", "IMPLIES", "IFF", "OR", "AND", "NOT", "EX",
  "AX", "EF", "AF", "EG", "AG", "EE", "AA", "UNTIL", "EBF", "EBG", "ABF",
  "ABG", "BUNTIL", "MMIN", "MMAX", "EQUAL", "LT", "GT", "LE", "GE",
  "UNION", "SETIN", "MOD", "PLUS", "MINUS", "TIMES", "DIVIDE", "UMINUS",
  "NEXT", "DOT", "$accept", "constant", "subrange", "number", "spec",
  "var_id", "simple_expr", "atom_set", "s_case_list", "ctl_expr", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333
};
# endif

#define YYPACT_NINF (-103)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      14,   361,   403,    26,   361,   111,  -103,  -103,  -103,  -103,
    -103,   361,   361,   361,   361,   361,   361,   361,     6,     7,
     -36,   -36,   -36,   -36,    -8,    -5,  -103,     8,   -23,   588,
     403,   111,  -103,  -103,    16,   403,   403,  -103,     8,   -23,
     621,  -103,   274,  -103,  -103,  -103,  -103,  -103,   -25,   663,
     663,   663,   663,   663,   663,   663,   361,   361,   361,     8,
     361,   361,   361,  -103,  -103,   -36,   403,    -1,   361,   361,
     361,   361,   361,   361,   361,   361,   361,   361,   361,   361,
     361,   361,   361,   361,   307,   -20,    11,   290,    15,   675,
     403,   403,   403,   403,   403,   403,   403,   403,   403,   403,
     403,   403,   403,   403,   403,   403,  -103,  -103,   111,   527,
     574,   663,   663,   663,   663,  -103,   415,  -103,   588,   187,
     635,   663,   -61,   -61,   -61,   -61,   -61,    59,   -13,    65,
     -69,   -69,  -103,  -103,  -103,  -103,   -19,   403,  -103,   621,
     605,   651,   675,    10,    10,    10,    10,    10,   194,    74,
      95,   -59,   -59,  -103,  -103,  -103,   361,   -36,   361,   -36,
    -103,   -36,     3,   427,   361,   478,   361,    28,   403,  -103,
     490,  -103,   541,   403,  -103,  -103,  -103,   621
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,    49,    50,    13,    12,
       7,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    48,    47,    46,    10,
       0,     0,    19,    20,     0,    44,     0,    18,    17,    16,
      11,     1,     0,     4,     5,     2,    42,     3,     0,    56,
      70,    71,    72,    73,    74,    75,     0,     0,     0,     0,
       0,     0,     0,     8,     9,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    26,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    51,    67,     0,     0,
       0,    80,    82,    81,    83,     6,     0,    14,    52,    53,
      54,    55,    62,    63,    64,    65,    66,    68,    69,    61,
      57,    58,    59,    60,    21,    37,     0,     0,    40,    22,
      23,    24,    25,    32,    33,    34,    35,    36,    38,    39,
      31,    27,    28,    29,    30,    43,     0,     0,     0,     0,
      15,     0,     0,     0,     0,     0,     0,     0,    44,    77,
       0,    76,     0,     0,    45,    79,    78,    41
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -103,   -54,    21,    -2,  -103,   106,   126,    34,  -102,   233
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    46,    26,    27,     3,    28,    87,    48,    88,    29
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      38,    66,   107,    47,    10,    82,    83,   135,    77,    78,
      79,    80,    81,    82,    83,   104,   105,   108,    59,    59,
      59,    59,   108,    37,   168,     1,    41,     2,    38,    47,
      56,    57,    63,    38,    38,    64,    24,    25,   117,    65,
      86,    58,    60,    61,    62,   161,    90,    91,    92,    93,
     136,    37,   138,   173,   155,    67,    37,    37,    79,    80,
      81,    82,    83,   115,    38,    85,   174,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,    99,
     100,   101,   102,   103,   104,   105,     0,    37,    38,    38,
      38,    38,    38,    38,    38,    38,    38,    38,    38,    38,
      38,    38,    38,    38,     0,     0,    47,     0,    39,     0,
       0,    37,    37,    37,    37,    37,    37,    37,    37,    37,
      37,    37,    37,    37,    37,    37,    37,     0,    40,    78,
      79,    80,    81,    82,    83,    38,    39,    80,    81,    82,
      83,    39,    39,    43,    44,   101,   102,   103,   104,   105,
      45,    10,     0,     0,     0,    59,    84,    59,    37,    59,
       0,     0,    89,     0,     0,     0,    38,   102,   103,   104,
     105,    38,    39,     0,     0,     0,     0,     0,   164,     0,
     166,     0,   167,    24,    25,     0,     0,     0,     0,    37,
       0,     0,   116,     0,    37,     0,    39,    39,    39,    39,
      39,    39,    39,    39,    39,    39,    39,    39,    39,    39,
      39,    39,     0,     0,     0,     0,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   152,
     153,   154,    70,    71,     0,     0,     0,    42,     0,     0,
       0,     0,     0,    39,    49,    50,    51,    52,    53,    54,
      55,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,   162,   100,   101,   102,   103,   104,   105,
       0,     0,     0,     0,    39,     0,     0,     0,     0,    39,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   109,
     110,   111,     0,   112,   113,   114,     0,   106,     0,   177,
       0,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,    68,    69,    70,
      71,     0,     0,     0,     0,     0,     0,     0,   137,     0,
     134,     0,     0,    90,    91,    92,    93,     0,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,     0,     0,     0,     0,
       0,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,     4,     0,     0,     0,     5,     0,   163,
       0,   165,     0,     6,     7,     8,     0,   170,     0,   172,
       9,    10,     0,     0,     0,     0,     0,     0,    11,    12,
      13,    14,    15,    16,    17,    18,    19,     0,    20,    21,
      22,    23,     0,     0,     0,    30,     0,     0,     0,    31,
       0,     0,     0,    24,    25,    32,    33,     8,    34,    35,
     160,     0,     9,    10,     0,     0,     0,     0,     0,     0,
      36,     0,   169,     0,     0,     0,     0,     0,    90,    91,
      92,    93,     0,     0,     0,     0,     0,     0,     0,     0,
      68,    69,    70,    71,     0,    24,    25,     0,     0,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,   171,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   175,     0,     0,     0,     0,
       0,    68,    69,    70,    71,     0,     0,     0,     0,     0,
       0,     0,     0,    68,    69,    70,    71,     0,     0,     0,
       0,     0,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,   176,     0,     0,     0,
      68,    69,    70,    71,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   156,    68,    69,    70,    71,   157,     0,
       0,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,     0,     0,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    68,    69,    70,
      71,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     158,    68,    69,    70,    71,   159,     0,     0,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      92,    93,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    90,    91,    92,    93,     0,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,    71,     0,     0,     0,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,    93,     0,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,     0,     0,     0,     0,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105
};

static const yytype_int16 yycheck[] =
{
       2,    24,    27,     5,    40,    74,    75,    27,    69,    70,
      71,    72,    73,    74,    75,    74,    75,    42,    20,    21,
      22,    23,    42,     2,    21,    11,     0,    13,    30,    31,
      24,    24,    40,    35,    36,    40,    72,    73,    39,    31,
      24,    20,    21,    22,    23,    64,    43,    44,    45,    46,
      39,    30,    37,    25,   108,    78,    35,    36,    71,    72,
      73,    74,    75,    65,    66,    31,   168,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    69,
      70,    71,    72,    73,    74,    75,    -1,    66,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,    -1,    -1,   108,    -1,     2,    -1,
      -1,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,    -1,     2,    70,
      71,    72,    73,    74,    75,   137,    30,    72,    73,    74,
      75,    35,    36,    32,    33,    71,    72,    73,    74,    75,
      39,    40,    -1,    -1,    -1,   157,    30,   159,   137,   161,
      -1,    -1,    36,    -1,    -1,    -1,   168,    72,    73,    74,
      75,   173,    66,    -1,    -1,    -1,    -1,    -1,   157,    -1,
     159,    -1,   161,    72,    73,    -1,    -1,    -1,    -1,   168,
      -1,    -1,    66,    -1,   173,    -1,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,    -1,    -1,    -1,    -1,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,    45,    46,    -1,    -1,    -1,     4,    -1,    -1,
      -1,    -1,    -1,   137,    11,    12,    13,    14,    15,    16,
      17,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,   137,    70,    71,    72,    73,    74,    75,
      -1,    -1,    -1,    -1,   168,    -1,    -1,    -1,    -1,   173,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    56,
      57,    58,    -1,    60,    61,    62,    -1,    23,    -1,   173,
      -1,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    43,    44,    45,
      46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,
      23,    -1,    -1,    43,    44,    45,    46,    -1,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      43,    44,    45,    46,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    -1,    -1,    -1,    -1,
      -1,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    22,    -1,    -1,    -1,    26,    -1,   156,
      -1,   158,    -1,    32,    33,    34,    -1,   164,    -1,   166,
      39,    40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    -1,    57,    58,
      59,    60,    -1,    -1,    -1,    22,    -1,    -1,    -1,    26,
      -1,    -1,    -1,    72,    73,    32,    33,    34,    35,    36,
      25,    -1,    39,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      47,    -1,    25,    -1,    -1,    -1,    -1,    -1,    43,    44,
      45,    46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      43,    44,    45,    46,    -1,    72,    73,    -1,    -1,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    25,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    -1,    -1,    -1,
      -1,    43,    44,    45,    46,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    43,    44,    45,    46,    -1,    -1,    -1,
      -1,    -1,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    25,    -1,    -1,    -1,
      43,    44,    45,    46,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    56,    43,    44,    45,    46,    61,    -1,
      -1,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    -1,    -1,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    43,    44,    45,
      46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      56,    43,    44,    45,    46,    61,    -1,    -1,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      45,    46,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    43,    44,    45,    46,    -1,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    46,    -1,    -1,    -1,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    46,    -1,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    -1,    -1,    -1,    -1,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    11,    13,    83,    22,    26,    32,    33,    34,    39,
      40,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      57,    58,    59,    60,    72,    73,    81,    82,    84,    88,
      22,    26,    32,    33,    35,    36,    47,    81,    82,    84,
      85,     0,    88,    32,    33,    39,    80,    82,    86,    88,
      88,    88,    88,    88,    88,    88,    24,    24,    81,    82,
      81,    81,    81,    40,    40,    31,    24,    78,    43,    44,
      45,    46,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    85,    86,    24,    85,    87,    85,
      43,    44,    45,    46,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    23,    27,    42,    88,
      88,    88,    88,    88,    88,    82,    85,    39,    88,    88,
      88,    88,    88,    88,    88,    88,    88,    88,    88,    88,
      88,    88,    88,    88,    23,    27,    39,    38,    37,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      85,    85,    85,    85,    85,    80,    56,    61,    56,    61,
      25,    64,    85,    88,    81,    88,    81,    81,    21,    25,
      88,    25,    88,    25,    87,    25,    25,    85
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int8 yyr1[] =
{
       0,    79,    80,    80,    80,    80,    81,    82,    82,    82,
      83,    83,    84,    84,    84,    84,    85,    85,    85,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      85,    85,    86,    86,    87,    87,    88,    88,    88,    88,
      88,    88,    88,    88,    88,    88,    88,    88,    88,    88,
      88,    88,    88,    88,    88,    88,    88,    88,    88,    88,
      88,    88,    88,    88,    88,    88,    88,    88,    88,    88,
      88,    88,    88,    88
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     3,     1,     2,     2,
       2,     2,     1,     1,     3,     4,     1,     1,     1,     1,
       1,     3,     3,     3,     3,     3,     2,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     7,     1,     3,     0,     5,     1,     1,     1,     1,
       1,     3,     3,     3,     3,     3,     2,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       2,     2,     2,     2,     2,     2,     6,     6,     7,     7,
       3,     3,     3,     3
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[+yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
#  else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                yy_state_t *yyssp, int yytoken)
{
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Actual size of YYARG. */
  int yycount = 0;
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[+*yyssp];
      YYPTRDIFF_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
      yysize = yysize0;
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYPTRDIFF_T yysize1
                    = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    /* Don't count the "%s"s in the final size, but reserve room for
       the terminator.  */
    YYPTRDIFF_T yysize1 = yysize + (yystrlen (yyformat) - 2 * yycount) + 1;
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss;
    yy_state_t *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYPTRDIFF_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2:
#line 166 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                     { verbose("constant -> ATOM\n"); }
#line 1805 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 3:
#line 167 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                       { verbose("constant -> number\n"); }
#line 1811 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 4:
#line 168 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                         { verbose("constant -> FALSEEXP\n"); }
#line 1817 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 5:
#line 169 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                        { verbose("constant -> TRUEEXP\n"); }
#line 1823 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 6:
#line 171 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                      { (yyval.node) = new_node(Expr_Ctl_TWODOTS_c, (yyvsp[-2].node), (yyvsp[0].node));
                  verbose("subrange -> number TWODOTS number\n"); 
              }
#line 1831 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 7:
#line 175 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                       { verbose("number -> NUMBER\n"); }
#line 1837 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 8:
#line 176 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                         { (yyval.node) = (yyvsp[0].node);
		verbose("number -> PLUS NUMBER\n"); 
	      }
#line 1845 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 9:
#line 179 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                          { (yyval.node) = (yyvsp[0].node);  /*** da sistemare ****/
		verbose("number -> MINUS NUMBER\n"); 
	      }
#line 1853 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 10:
#line 186 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                              { ExprYySpecif = 
                  new_node(Expr_Ctl_SPEC_c, (yyvsp[0].node), NULL);
                verbose("    spec -> SPEC ctl_expr\n");
                verbose("Valid specification.\n");
              }
#line 1863 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 11:
#line 191 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                      { ExprYySpecif = 
                  new_node(Expr_Ctl_INVARSPEC_c, (yyvsp[0].node), NULL);
                verbose("    invspec -> INVSPEC simple_expr\n");
                verbose("Valid specification.\n");
              }
#line 1873 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 12:
#line 203 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                     { (yyval.node) = new_leaf((yyvsp[0].id));
                verbose("var_id -> ATOM\n"); 
              }
#line 1881 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 13:
#line 206 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                     { verbose("var_id -> SELF\n");}
#line 1887 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 14:
#line 207 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                { (yyval.node) = new_node(Expr_Ctl_DOT_c, (yyvsp[-2].node), new_leaf((yyvsp[0].id)));
		verbose("var_id -> var_id DOT ATOM\n");
	      }
#line 1895 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 15:
#line 210 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                         { (yyval.node) = new_node(Expr_Ctl_ARRAY_c, (yyvsp[-3].node), (yyvsp[-1].node));
		verbose("var_id -> var_id LB simple_expr RB\n");}
#line 1902 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 16:
#line 217 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                       { verbose("simple_expr -> var_id\n"); }
#line 1908 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 17:
#line 218 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                       { verbose("simple_expr -> number\n"); }
#line 1914 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 18:
#line 219 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                         { verbose("simple_expr -> subrange\n"); }
#line 1920 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 19:
#line 220 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                         { verbose("simple_expr -> FALSEEXP\n"); }
#line 1926 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 20:
#line 221 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                        { verbose("simple_expr -> TRUEEXP\n"); }
#line 1932 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 21:
#line 222 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                  { (yyval.node) = (yyvsp[-1].node);
		verbose("simple_expr -> LP simple_expr RP\n"); 
	      }
#line 1940 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 22:
#line 225 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                                { (yyval.node) = new_node(Expr_Ctl_IMPLIES_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("simple_expr -> simple_expr IMPLIES simple_expr\n"); 
	      }
#line 1948 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 23:
#line 228 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                            { (yyval.node) = new_node(Expr_Ctl_IFF_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("simple_expr -> simple_expr IFF simple_expr\n"); 
	      }
#line 1956 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 24:
#line 231 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                           {  (yyval.node) = new_node(Expr_Ctl_OR_c,(yyvsp[-2].node), (yyvsp[0].node)); 
		verbose("simple_expr -> simple_expr OR simple_expr\n"); 
	      }
#line 1964 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 25:
#line 234 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                            { (yyval.node) = new_node(Expr_Ctl_AND_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("simple_expr -> simple_expr AND simple_expr\n"); 
	      }
#line 1972 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 26:
#line 237 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                { (yyval.node) = new_node(Expr_Ctl_NOT_c, (yyvsp[0].node), NULL);
		verbose("simple_expr -> NOT simple_expr\n"); 
	      }
#line 1980 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 27:
#line 240 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                             { (yyval.node) = new_node(Expr_Ctl_PLUS_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("simple_expr -> simple_expr PLUS simple_expr\n"); 
	      }
#line 1988 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 28:
#line 243 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                              { (yyval.node) = new_node(Expr_Ctl_MINUS_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("simple_expr -> simple_expr MINUS simple_expr\n"); 
	      }
#line 1996 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 29:
#line 246 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                              { (yyval.node) = new_node(Expr_Ctl_TIMES_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("simple_expr -> simple_expr TIMES simple_expr\n"); 
	      }
#line 2004 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 30:
#line 249 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                               { (yyval.node) = new_node(Expr_Ctl_DIVIDE_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("simple_expr -> simple_expr DIVIDE simple_expr\n"); 
	      }
#line 2012 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 31:
#line 252 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                            { (yyval.node) = new_node(Expr_Ctl_MOD_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("simple_expr -> simple_expr MOD simple_expr\n"); 
	      }
#line 2020 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 32:
#line 255 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                              { (yyval.node) = new_node(Expr_Ctl_EQUAL_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("simple_expr -> simple_expr EQUAL simple_expr\n"); 
	      }
#line 2028 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 33:
#line 258 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                           { (yyval.node) = new_node(Expr_Ctl_LT_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("simple_expr -> simple_expr LT simple_expr\n"); 
	      }
#line 2036 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 34:
#line 261 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                           { (yyval.node) = new_node(Expr_Ctl_GT_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("simple_expr -> simple_expr GT simple_expr\n"); 
	      }
#line 2044 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 35:
#line 264 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                           { (yyval.node) = new_node(Expr_Ctl_LE_c, (yyvsp[-2].node), (yyvsp[0].node)); 
		verbose("simple_expr -> simple_expr LE simple_expr\n"); 
	      }
#line 2052 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 36:
#line 267 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                           { (yyval.node) = new_node(Expr_Ctl_GE_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("simple_expr -> simple_expr GE simple_expr\n"); 
	      }
#line 2060 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 37:
#line 270 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                 { (yyval.node) = (yyvsp[-1].node);
		verbose("simple_expr -> LCB atom_set RCB\n"); 
	      }
#line 2068 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 38:
#line 273 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                              { (yyval.node) = new_node(Expr_Ctl_UNION_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("simple_expr -> simple_expr UNION simple_expr\n"); 
	      }
#line 2076 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 39:
#line 276 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                              { (yyval.node) = new_node(Expr_Ctl_SETIN_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("simple_expr -> simple_expr SETIN simple_expr\n"); 
	      }
#line 2084 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 40:
#line 279 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                      { (yyval.node) = (yyvsp[-1].node);  
		verbose("simple_expr -> CASE s_case_list ESAC\n"); 
	      }
#line 2092 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 41:
#line 282 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                                            { (yyval.node) = new_node(Expr_Ctl_SIGMA_c, new_node(Expr_Ctl_EQUAL_c, new_leaf((yyvsp[-4].id)), (yyvsp[-2].node)), (yyvsp[0].node));
		verbose("simple_expr -> SIGMA LB ATOM EQUAL subrange RB simple_expr\n"); 
	      }
#line 2100 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 42:
#line 286 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                         { verbose("atom_set -> constant\n");}
#line 2106 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 43:
#line 287 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                        { (yyval.node) = new_node(Expr_Ctl_UNION_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("atom_set -> atom_set COMMA constant\n");
	      }
#line 2114 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 44:
#line 292 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                { (yyval.node)=new_node(Expr_Ctl_TRUEEXP_c, NULL, NULL);
		verbose("s_case_list ->		/* empty */\n");
              }
#line 2122 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 45:
#line 295 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                                               { (yyval.node) = new_node(Expr_Ctl_CASE_c, new_node(Expr_Ctl_COLON_c, (yyvsp[-4].node), (yyvsp[-2].node)), (yyvsp[0].node));
	      verbose("s_case_list -> simple_expr COLON simple_expr SEMI s_case_list\n");
	      }
#line 2130 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 46:
#line 303 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                       { verbose("  ctl_expr -> var_id\n"); }
#line 2136 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 47:
#line 304 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                       { verbose("  ctl_expr -> number\n"); }
#line 2142 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 48:
#line 305 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                         { verbose("  ctl_expr -> subrange\n"); }
#line 2148 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 49:
#line 306 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                         { verbose("  ctl_expr -> FALSEEXP\n"); }
#line 2154 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 50:
#line 307 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                        { verbose("  ctl_expr -> TRUEEXP\n"); }
#line 2160 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 51:
#line 308 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                               { (yyval.node) = (yyvsp[-1].node); 
		verbose("  ctl_expr -> LP ctl_expr RP\n"); 
	      }
#line 2168 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 52:
#line 311 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                          { (yyval.node) = new_node(Expr_Ctl_IMPLIES_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("  ctl_expr -> ctl_expr IMPLIES ctl_expr\n"); 
	      }
#line 2176 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 53:
#line 314 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                      { (yyval.node) = new_node(Expr_Ctl_IFF_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("  ctl_expr -> ctl_expr IFF ctl_expr\n"); 
	      }
#line 2184 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 54:
#line 317 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                     { (yyval.node) = new_node(Expr_Ctl_OR_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("  ctl_expr -> ctl_expr OR ctl_expr\n"); 
	      }
#line 2192 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 55:
#line 320 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                      { (yyval.node) = new_node(Expr_Ctl_AND_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("  ctl_expr -> ctl_expr AND ctl_expr\n"); 
	      }
#line 2200 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 56:
#line 323 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                             { (yyval.node) = new_node(Expr_Ctl_NOT_c, (yyvsp[0].node), NULL); 
		verbose("  ctl_expr -> NOT ctl_expr\n"); 
	      }
#line 2208 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 57:
#line 326 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                       { (yyval.node) = new_node(Expr_Ctl_PLUS_c, (yyvsp[-2].node), (yyvsp[0].node));  
		verbose("  ctl_expr -> ctl_expr PLUS ctl_expr\n"); 
	      }
#line 2216 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 58:
#line 329 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                        { (yyval.node) = new_node(Expr_Ctl_MINUS_c, (yyvsp[-2].node), (yyvsp[0].node)); 
		verbose("  ctl_expr -> ctl_expr MINUS ctl_expr\n"); 
	      }
#line 2224 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 59:
#line 332 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                        { (yyval.node) = new_node(Expr_Ctl_TIMES_c, (yyvsp[-2].node), (yyvsp[0].node)); 
		verbose("  ctl_expr -> ctl_expr TIMES ctl_expr\n"); 
	      }
#line 2232 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 60:
#line 335 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                         { (yyval.node) = new_node(Expr_Ctl_DIVIDE_c, (yyvsp[-2].node), (yyvsp[0].node)); 
		verbose("  ctl_expr -> ctl_expr DIVIDE ctl_expr\n"); 
	      }
#line 2240 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 61:
#line 338 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                      {  (yyval.node) = new_node(Expr_Ctl_MOD_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("  ctl_expr -> ctl_expr MOD ctl_expr\n"); 
	      }
#line 2248 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 62:
#line 341 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                        { (yyval.node) = new_node(Expr_Ctl_EQUAL_c, (yyvsp[-2].node), (yyvsp[0].node)); 
		verbose("  ctl_expr -> ctl_expr EQUAL ctl_expr\n"); 
	      }
#line 2256 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 63:
#line 344 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                     { (yyval.node) = new_node(Expr_Ctl_LT_c, (yyvsp[-2].node), (yyvsp[0].node)); 
		verbose("  ctl_expr -> ctl_expr LT ctl_expr\n"); 
	      }
#line 2264 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 64:
#line 347 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                     { (yyval.node) = new_node(Expr_Ctl_GT_c, (yyvsp[-2].node), (yyvsp[0].node)); 
		verbose("  ctl_expr -> ctl_expr GT ctl_expr\n"); 
	      }
#line 2272 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 65:
#line 350 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                     {  (yyval.node) = new_node(Expr_Ctl_LE_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("  ctl_expr -> ctl_expr LE ctl_expr\n"); 
	      }
#line 2280 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 66:
#line 353 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                     {  (yyval.node) = new_node(Expr_Ctl_GE_c, (yyvsp[-2].node), (yyvsp[0].node));
		verbose("  ctl_expr -> ctl_expr GE ctl_expr\n"); 
	      }
#line 2288 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 67:
#line 356 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                 { (yyval.node) = (yyvsp[-1].node); 
		verbose("  ctl_expr -> LCB atom_set RCB\n"); 
	      }
#line 2296 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 68:
#line 359 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                        { (yyval.node) = new_node(Expr_Ctl_UNION_c, (yyvsp[-2].node), (yyvsp[0].node)); 
		verbose("  ctl_expr -> ctl_expr UNION ctl_expr\n"); 
	      }
#line 2304 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 69:
#line 362 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                        { (yyval.node) = new_node(Expr_Ctl_SETIN_c, (yyvsp[-2].node), (yyvsp[0].node)); 
		verbose("  ctl_expr -> ctl_expr SETIN ctl_expr\n"); 
	      }
#line 2312 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 70:
#line 367 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                            {  (yyval.node) = new_node(Expr_Ctl_EX_c, (yyvsp[0].node), NULL);
		verbose("  ctl_expr -> EX ctl_expr\n"); 
	      }
#line 2320 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 71:
#line 370 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                            { (yyval.node) = new_node(Expr_Ctl_AX_c, (yyvsp[0].node), NULL); 
		verbose("  ctl_expr -> AX ctl_expr\n"); 
	      }
#line 2328 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 72:
#line 373 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                            { (yyval.node) = new_node(Expr_Ctl_EF_c, (yyvsp[0].node), NULL); 
		verbose("  ctl_expr -> EF ctl_expr\n"); 
	      }
#line 2336 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 73:
#line 376 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                            {  (yyval.node) = new_node(Expr_Ctl_AF_c, (yyvsp[0].node), NULL);
		verbose("  ctl_expr -> AF ctl_expr\n"); 
	      }
#line 2344 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 74:
#line 379 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                            {  (yyval.node) = new_node(Expr_Ctl_EG_c, (yyvsp[0].node), NULL);
		verbose("  ctl_expr -> EG ctl_expr\n"); 
	      }
#line 2352 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 75:
#line 382 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                            { (yyval.node) = new_node(Expr_Ctl_AG_c, (yyvsp[0].node), NULL); 
		verbose("  ctl_expr -> AG ctl_expr\n"); 
	      }
#line 2360 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 76:
#line 385 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                                 { (yyval.node) = new_node(Expr_Ctl_AU_c, (yyvsp[-3].node), (yyvsp[-1].node)); 
		verbose("  ctl_expr -> AA LB ctl_expr UNTIL ctl_expr RB\n"); 
	      }
#line 2368 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 77:
#line 388 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                                 { (yyval.node) = new_node(Expr_Ctl_EU_c, (yyvsp[-3].node), (yyvsp[-1].node));  
		verbose("  ctl_expr -> EE LB ctl_expr UNTIL ctl_expr RB\n"); 
	      }
#line 2376 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 78:
#line 391 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                                           { (yyval.node) = new_node(Expr_Ctl_ABU_c, new_node(Expr_Ctl_AU_c, (yyvsp[-4].node), (yyvsp[-1].node)), (yyvsp[-2].node));
		verbose("  ctl_expr -> AA LB ctl_expr BUNTIL subrange ctl_expr RB\n"); 
	      }
#line 2384 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 79:
#line 394 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                                           { (yyval.node) = new_node(Expr_Ctl_EBU_c, new_node(Expr_Ctl_EU_c, (yyvsp[-4].node), (yyvsp[-1].node)), (yyvsp[-2].node)); 
		verbose("  ctl_expr -> EE LB ctl_expr BUNTIL subrange ctl_expr RB\n"); 
	      }
#line 2392 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 80:
#line 397 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                      { (yyval.node) = new_node(Expr_Ctl_EBF_c, (yyvsp[0].node), (yyvsp[-1].node));
		verbose("  ctl_expr -> EBF subrange ctl_expr\n"); 
	      }
#line 2400 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 81:
#line 400 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                      {  (yyval.node) = new_node(Expr_Ctl_ABF_c, (yyvsp[0].node), (yyvsp[-1].node));
		verbose("  ctl_expr -> ABF subrange ctl_expr\n"); 
	      }
#line 2408 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 82:
#line 403 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                      { (yyval.node) = new_node(Expr_Ctl_EBG_c, (yyvsp[0].node), (yyvsp[-1].node)); 
		verbose("  ctl_expr -> EBG subrange ctl_expr\n"); 
	      }
#line 2416 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;

  case 83:
#line 406 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"
                                      { (yyval.node) = new_node(Expr_Ctl_ABG_c, (yyvsp[0].node), (yyvsp[-1].node)); 
		verbose("  ctl_expr -> ABG subrange ctl_expr\n"); 
	      }
#line 2424 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"
    break;


#line 2428 "/home/cabodi/pdtools-2.0/pdtrav-4.0/obj/exprYacc.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *, YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[+*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 411 "/home/cabodi/pdtools-2.0/pdtrav-4.0/src/expr/expr.y"


/* Additional source code */

#include "exprLex.c"

static Ddi_Expr_t *new_node(Expr_Ctl_Code_e opcode, 
  Ddi_Expr_t * op1, Ddi_Expr_t * op2)
{
  Ddi_Expr_t *r;

  verbose("------new--node------\n");

  r = Ddi_ExprCtlMake(ExprYyDdmgr,(int)opcode,op1,op2,NULL); 
  Ddi_Free(op1);
  Ddi_Free(op2);

  return (r);
}

static Ddi_Expr_t *new_leaf(char *s)
{
  Ddi_Expr_t *leaf;

  leaf = Ddi_ExprMakeFromString(ExprYyDdmgr,s);
  Pdtutil_Free(s);

  return leaf;
}


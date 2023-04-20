%{
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

%}

/*
  The number of conflicts: shift/reduce expected.
  If more, than a warning message is printed out.
*/
/*%expect 1*/

%union {
  Ddi_Expr_t *node;
  char *id;
}

%left PROCESS EU AU EBU ABU MINU MAXU
%left DEFINE SPEC COMPUTE 
%left INVARSPEC ASSIGN INPUT OUTPUT
%left BOOLEAN ARRAY OF SCALAR
%left SEMI LP RP LB RB LCB RCB LLCB RRCB
%left EQDEF TWODOTS
%left <node> FALSEEXP TRUEEXP
%left SELF SIGMA
%left CASE ESAC COLON

%left <id> ATOM
%left <node> NUMBER
%left <node> QUOTE

%left  COMMA
%right IMPLIES
%left  IFF
%left  OR
%left  AND
%left  NOT
%left  EX AX EF AF EG AG EE AA UNTIL EBF EBG ABF ABG BUNTIL MMIN MMAX
%left  EQUAL LT GT LE GE
%left  UNION
%left  SETIN
%left  MOD
%left  PLUS MINUS
%left  TIMES DIVIDE
%left  UMINUS		/* supplies precedence for unary minus */
%left  NEXT
%left  DOT

/* all nonterminals return a parse tree node */

%type <node> spec 
%type <node> var_id subrange 
%type <node> number simple_expr  
%type <node> ctl_expr  s_case_list  constant
%type <node> atom_set


%start spec
%%

/*
 Variable declarations:
*/

constant      : ATOM { verbose("constant -> ATOM\n"); } 
              | number { verbose("constant -> number\n"); }
	      | FALSEEXP { verbose("constant -> FALSEEXP\n"); }
	      | TRUEEXP { verbose("constant -> TRUEEXP\n"); }
              ;
subrange      : number TWODOTS number { $$ = new_node(Expr_Ctl_TWODOTS_c, $1, $3);
                  verbose("subrange -> number TWODOTS number\n"); 
              }
              ;
number        : NUMBER { verbose("number -> NUMBER\n"); }
	      | PLUS NUMBER %prec UMINUS { $$ = $2;
		verbose("number -> PLUS NUMBER\n"); 
	      }
              | MINUS NUMBER %prec UMINUS { $$ = $2;  /*** da sistemare ****/
		verbose("number -> MINUS NUMBER\n"); 
	      }
              ;

/* Specifications and computation of min and max distance */

spec          : SPEC ctl_expr { ExprYySpecif = 
                  new_node(Expr_Ctl_SPEC_c, $2, NULL);
                verbose("    spec -> SPEC ctl_expr\n");
                verbose("Valid specification.\n");
              }
	      | INVARSPEC simple_expr { ExprYySpecif = 
                  new_node(Expr_Ctl_INVARSPEC_c, $2, NULL);
                verbose("    invspec -> INVSPEC simple_expr\n");
                verbose("Valid specification.\n");
              }
              ;

/* Variable identifiers.
   decl_var_id is used for declarations, array-like syntax not allowed.
   var_id is used to reference variables, includes array-like syntax.
 */

var_id        : ATOM { $$ = new_leaf($1);
                verbose("var_id -> ATOM\n"); 
              }
              | SELF { verbose("var_id -> SELF\n");}
              | var_id DOT ATOM { $$ = new_node(Expr_Ctl_DOT_c, $1, new_leaf($3));
		verbose("var_id -> var_id DOT ATOM\n");
	      }
              | var_id LB simple_expr RB { $$ = new_node(Expr_Ctl_ARRAY_c, $1, $3);
		verbose("var_id -> var_id LB simple_expr RB\n");}
              ;


/* Simple expressions. Do not involve next state variables or CLT. */

simple_expr   : var_id { verbose("simple_expr -> var_id\n"); }
              | number { verbose("simple_expr -> number\n"); }
              | subrange { verbose("simple_expr -> subrange\n"); }
              | FALSEEXP { verbose("simple_expr -> FALSEEXP\n"); }
              | TRUEEXP { verbose("simple_expr -> TRUEEXP\n"); }
              | LP simple_expr RP { $$ = $2;
		verbose("simple_expr -> LP simple_expr RP\n"); 
	      }
	      | simple_expr IMPLIES simple_expr { $$ = new_node(Expr_Ctl_IMPLIES_c, $1, $3);
		verbose("simple_expr -> simple_expr IMPLIES simple_expr\n"); 
	      }
	      | simple_expr IFF simple_expr { $$ = new_node(Expr_Ctl_IFF_c, $1, $3);
		verbose("simple_expr -> simple_expr IFF simple_expr\n"); 
	      }
	      | simple_expr OR simple_expr {  $$ = new_node(Expr_Ctl_OR_c,$1, $3); 
		verbose("simple_expr -> simple_expr OR simple_expr\n"); 
	      }
	      | simple_expr AND simple_expr { $$ = new_node(Expr_Ctl_AND_c, $1, $3);
		verbose("simple_expr -> simple_expr AND simple_expr\n"); 
	      }
	      | NOT simple_expr { $$ = new_node(Expr_Ctl_NOT_c, $2, NULL);
		verbose("simple_expr -> NOT simple_expr\n"); 
	      }
              | simple_expr PLUS simple_expr { $$ = new_node(Expr_Ctl_PLUS_c, $1, $3);
		verbose("simple_expr -> simple_expr PLUS simple_expr\n"); 
	      }
              | simple_expr MINUS simple_expr { $$ = new_node(Expr_Ctl_MINUS_c, $1, $3);
		verbose("simple_expr -> simple_expr MINUS simple_expr\n"); 
	      }
              | simple_expr TIMES simple_expr { $$ = new_node(Expr_Ctl_TIMES_c, $1, $3);
		verbose("simple_expr -> simple_expr TIMES simple_expr\n"); 
	      }
              | simple_expr DIVIDE simple_expr { $$ = new_node(Expr_Ctl_DIVIDE_c, $1, $3);
		verbose("simple_expr -> simple_expr DIVIDE simple_expr\n"); 
	      }
              | simple_expr MOD simple_expr { $$ = new_node(Expr_Ctl_MOD_c, $1, $3);
		verbose("simple_expr -> simple_expr MOD simple_expr\n"); 
	      }
              | simple_expr EQUAL simple_expr { $$ = new_node(Expr_Ctl_EQUAL_c, $1, $3);
		verbose("simple_expr -> simple_expr EQUAL simple_expr\n"); 
	      }
              | simple_expr LT simple_expr { $$ = new_node(Expr_Ctl_LT_c, $1, $3);
		verbose("simple_expr -> simple_expr LT simple_expr\n"); 
	      }
              | simple_expr GT simple_expr { $$ = new_node(Expr_Ctl_GT_c, $1, $3);
		verbose("simple_expr -> simple_expr GT simple_expr\n"); 
	      }
              | simple_expr LE simple_expr { $$ = new_node(Expr_Ctl_LE_c, $1, $3); 
		verbose("simple_expr -> simple_expr LE simple_expr\n"); 
	      }
              | simple_expr GE simple_expr { $$ = new_node(Expr_Ctl_GE_c, $1, $3);
		verbose("simple_expr -> simple_expr GE simple_expr\n"); 
	      }
              | LCB atom_set RCB { $$ = $2;
		verbose("simple_expr -> LCB atom_set RCB\n"); 
	      } 
              | simple_expr UNION simple_expr { $$ = new_node(Expr_Ctl_UNION_c, $1, $3);
		verbose("simple_expr -> simple_expr UNION simple_expr\n"); 
	      }
              | simple_expr SETIN simple_expr { $$ = new_node(Expr_Ctl_SETIN_c, $1, $3);
		verbose("simple_expr -> simple_expr SETIN simple_expr\n"); 
	      }
              | CASE s_case_list ESAC { $$ = $2;  
		verbose("simple_expr -> CASE s_case_list ESAC\n"); 
	      }
              | SIGMA LB ATOM EQUAL subrange RB simple_expr { $$ = new_node(Expr_Ctl_SIGMA_c, new_node(Expr_Ctl_EQUAL_c, new_leaf($3), $5), $7);
		verbose("simple_expr -> SIGMA LB ATOM EQUAL subrange RB simple_expr\n"); 
	      }
	      ;
atom_set      : constant { verbose("atom_set -> constant\n");}
              | atom_set COMMA constant { $$ = new_node(Expr_Ctl_UNION_c, $1, $3);
		verbose("atom_set -> atom_set COMMA constant\n");
	      }
              ; 

s_case_list   : { $$=new_node(Expr_Ctl_TRUEEXP_c, NULL, NULL);
		verbose("s_case_list ->		/* empty */\n");
              }
              | simple_expr COLON simple_expr SEMI s_case_list { $$ = new_node(Expr_Ctl_CASE_c, new_node(Expr_Ctl_COLON_c, $1, $3), $5);
	      verbose("s_case_list -> simple_expr COLON simple_expr SEMI s_case_list\n");
	      }
              ;


/* (Bounded) CTL formulas. Contain CTL operators, but no next variables. */

ctl_expr      : var_id { verbose("  ctl_expr -> var_id\n"); }
              | number { verbose("  ctl_expr -> number\n"); }
              | subrange { verbose("  ctl_expr -> subrange\n"); }
              | FALSEEXP { verbose("  ctl_expr -> FALSEEXP\n"); }
              | TRUEEXP { verbose("  ctl_expr -> TRUEEXP\n"); }
              | LP ctl_expr RP { $$ = $2; 
		verbose("  ctl_expr -> LP ctl_expr RP\n"); 
	      }
	      | ctl_expr IMPLIES ctl_expr { $$ = new_node(Expr_Ctl_IMPLIES_c, $1, $3);
		verbose("  ctl_expr -> ctl_expr IMPLIES ctl_expr\n"); 
	      }
	      | ctl_expr IFF ctl_expr { $$ = new_node(Expr_Ctl_IFF_c, $1, $3);
		verbose("  ctl_expr -> ctl_expr IFF ctl_expr\n"); 
	      }
	      | ctl_expr OR ctl_expr { $$ = new_node(Expr_Ctl_OR_c, $1, $3);
		verbose("  ctl_expr -> ctl_expr OR ctl_expr\n"); 
	      }
	      | ctl_expr AND ctl_expr { $$ = new_node(Expr_Ctl_AND_c, $1, $3);
		verbose("  ctl_expr -> ctl_expr AND ctl_expr\n"); 
	      }
	      | NOT ctl_expr { $$ = new_node(Expr_Ctl_NOT_c, $2, NULL); 
		verbose("  ctl_expr -> NOT ctl_expr\n"); 
	      }
              | ctl_expr PLUS ctl_expr { $$ = new_node(Expr_Ctl_PLUS_c, $1, $3);  
		verbose("  ctl_expr -> ctl_expr PLUS ctl_expr\n"); 
	      }
              | ctl_expr MINUS ctl_expr { $$ = new_node(Expr_Ctl_MINUS_c, $1, $3); 
		verbose("  ctl_expr -> ctl_expr MINUS ctl_expr\n"); 
	      }
              | ctl_expr TIMES ctl_expr { $$ = new_node(Expr_Ctl_TIMES_c, $1, $3); 
		verbose("  ctl_expr -> ctl_expr TIMES ctl_expr\n"); 
	      }
              | ctl_expr DIVIDE ctl_expr { $$ = new_node(Expr_Ctl_DIVIDE_c, $1, $3); 
		verbose("  ctl_expr -> ctl_expr DIVIDE ctl_expr\n"); 
	      }
              | ctl_expr MOD ctl_expr {  $$ = new_node(Expr_Ctl_MOD_c, $1, $3);
		verbose("  ctl_expr -> ctl_expr MOD ctl_expr\n"); 
	      }
              | ctl_expr EQUAL ctl_expr { $$ = new_node(Expr_Ctl_EQUAL_c, $1, $3); 
		verbose("  ctl_expr -> ctl_expr EQUAL ctl_expr\n"); 
	      }
              | ctl_expr LT ctl_expr { $$ = new_node(Expr_Ctl_LT_c, $1, $3); 
		verbose("  ctl_expr -> ctl_expr LT ctl_expr\n"); 
	      }
              | ctl_expr GT ctl_expr { $$ = new_node(Expr_Ctl_GT_c, $1, $3); 
		verbose("  ctl_expr -> ctl_expr GT ctl_expr\n"); 
	      }
              | ctl_expr LE ctl_expr {  $$ = new_node(Expr_Ctl_LE_c, $1, $3);
		verbose("  ctl_expr -> ctl_expr LE ctl_expr\n"); 
	      }
              | ctl_expr GE ctl_expr {  $$ = new_node(Expr_Ctl_GE_c, $1, $3);
		verbose("  ctl_expr -> ctl_expr GE ctl_expr\n"); 
	      }
              | LCB atom_set RCB { $$ = $2; 
		verbose("  ctl_expr -> LCB atom_set RCB\n"); 
	      } 
              | ctl_expr UNION ctl_expr { $$ = new_node(Expr_Ctl_UNION_c, $1, $3); 
		verbose("  ctl_expr -> ctl_expr UNION ctl_expr\n"); 
	      }
              | ctl_expr SETIN ctl_expr { $$ = new_node(Expr_Ctl_SETIN_c, $1, $3); 
		verbose("  ctl_expr -> ctl_expr SETIN ctl_expr\n"); 
	      }
/*            | CASE ctl_case_list ESAC { ; } */
/*            | SIGMA LB ATOM EQUAL subrange RB ctl_expr { ; } */
              | EX ctl_expr {  $$ = new_node(Expr_Ctl_EX_c, $2, NULL);
		verbose("  ctl_expr -> EX ctl_expr\n"); 
	      }
              | AX ctl_expr { $$ = new_node(Expr_Ctl_AX_c, $2, NULL); 
		verbose("  ctl_expr -> AX ctl_expr\n"); 
	      }
              | EF ctl_expr { $$ = new_node(Expr_Ctl_EF_c, $2, NULL); 
		verbose("  ctl_expr -> EF ctl_expr\n"); 
	      }
              | AF ctl_expr {  $$ = new_node(Expr_Ctl_AF_c, $2, NULL);
		verbose("  ctl_expr -> AF ctl_expr\n"); 
	      }
              | EG ctl_expr {  $$ = new_node(Expr_Ctl_EG_c, $2, NULL);
		verbose("  ctl_expr -> EG ctl_expr\n"); 
	      }
              | AG ctl_expr { $$ = new_node(Expr_Ctl_AG_c, $2, NULL); 
		verbose("  ctl_expr -> AG ctl_expr\n"); 
	      }
	      | AA LB ctl_expr UNTIL ctl_expr RB { $$ = new_node(Expr_Ctl_AU_c, $3, $5); 
		verbose("  ctl_expr -> AA LB ctl_expr UNTIL ctl_expr RB\n"); 
	      }
	      | EE LB ctl_expr UNTIL ctl_expr RB { $$ = new_node(Expr_Ctl_EU_c, $3, $5);  
		verbose("  ctl_expr -> EE LB ctl_expr UNTIL ctl_expr RB\n"); 
	      }
	      | AA LB ctl_expr BUNTIL subrange ctl_expr RB { $$ = new_node(Expr_Ctl_ABU_c, new_node(Expr_Ctl_AU_c, $3, $6), $5);
		verbose("  ctl_expr -> AA LB ctl_expr BUNTIL subrange ctl_expr RB\n"); 
	      }
	      | EE LB ctl_expr BUNTIL subrange ctl_expr RB { $$ = new_node(Expr_Ctl_EBU_c, new_node(Expr_Ctl_EU_c, $3, $6), $5); 
		verbose("  ctl_expr -> EE LB ctl_expr BUNTIL subrange ctl_expr RB\n"); 
	      }
              | EBF subrange ctl_expr { $$ = new_node(Expr_Ctl_EBF_c, $3, $2);
		verbose("  ctl_expr -> EBF subrange ctl_expr\n"); 
	      }
              | ABF subrange ctl_expr {  $$ = new_node(Expr_Ctl_ABF_c, $3, $2);
		verbose("  ctl_expr -> ABF subrange ctl_expr\n"); 
	      }
              | EBG subrange ctl_expr { $$ = new_node(Expr_Ctl_EBG_c, $3, $2); 
		verbose("  ctl_expr -> EBG subrange ctl_expr\n"); 
	      }
              | ABG subrange ctl_expr { $$ = new_node(Expr_Ctl_ABG_c, $3, $2); 
		verbose("  ctl_expr -> ABG subrange ctl_expr\n"); 
	      }
	      ;

%%

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


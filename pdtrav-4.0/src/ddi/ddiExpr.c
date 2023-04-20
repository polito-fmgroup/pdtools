/**CFile***********************************************************************

  FileName    [ddiExpr.c]

  PackageName [ddi]

  Synopsis    [Functions working on Expressions (Ddi_Expr_t)]

  Description [Functions working on Expressions (Ddi_Expr_t)]

  SeeAlso   []

  Author    [Gianpiero Cabodi and Stefano Quer]

  Copyright [  Copyright (c) 2001 by Politecnico di Torino.
    All Rights Reserved. This software is for educational purposes only.
    Permission is given to academic institutions to use, copy, and modify
    this software and its documentation provided that this introductory
    message is not removed, that this software and its documentation is
    used for the institutions' internal research and educational purposes,
    and that no monies are exchanged. No guarantee is expressed or implied
    by the distribution of this code.
    Send bug-reports and/or questions to: {cabodi,quer}@polito.it. ]

  Revision  []

******************************************************************************/

#include "ddiInt.h"
#include "expr.h"

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

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/


/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/*
 *  Conversion, creation (make) and release (free) functions
 */

/**Function********************************************************************
  Synopsis    [Build a Ddi_Expr_t from a given BDD.]
  Description [Build the Ddi_Expr_t structure (by means of DdiGenericAlloc)
               from BDD handle.]
  SideEffects []
  SeeAlso     [DdiGenericAlloc Ddi_ExprToBdd]
******************************************************************************/
Ddi_Expr_t *
Ddi_ExprMakeFromBdd (
  Ddi_Bdd_t *f
)
{
  Ddi_Mgr_t *mgr;
  Ddi_Expr_t *r;

  DdiConsistencyCheck(f,Ddi_Bdd_c);
  mgr = Ddi_ReadMgr(f);

  r = (Ddi_Expr_t *)DdiGenericAlloc(Ddi_Expr_c,mgr);
  r->common.code = Ddi_Expr_Bdd_c;
  r->data.bdd = (Ddi_Generic_t *)Ddi_BddDup(f);
  Ddi_Lock(r->data.bdd);

  return (r);
}


/**Function********************************************************************

  Synopsis    [Retrieve the Bdd associated to the expression]

  Description [Retrieve the Bdd associated to the expression.
    Result is NOT duplicated (in other words, the BDD field is read)]

  SideEffects []

  SeeAlso     [Ddi_ExprMakeFromBdd]

******************************************************************************/

Ddi_Expr_t *
Ddi_ExprSetBdd(
  Ddi_Expr_t *e
)
{
  DdiConsistencyCheck(e,Ddi_Expr_c);
  switch(e->common.code) {
    case Ddi_Expr_Bdd_c:
    break;
    case Ddi_Expr_Bool_c:
    case Ddi_Expr_Ctl_c:
    break;
    case Ddi_Expr_String_c:
    {
      Ddi_Bdd_t *l;
      Ddi_Var_t *v;
      v = Ddi_VarFromName(Ddi_ReadMgr(e),e->data.string);
      Pdtutil_Assert(v!=NULL,"missing variable while converting expr");
      l = Ddi_BddMakeLiteral(v,1);
      Pdtutil_Free(e->data.string);
      e->common.code = Ddi_Expr_Bdd_c;
      e->data.bdd = (Ddi_Generic_t *)l;
    }
    break;
    default:
        fprintf(stderr, "Error: Switch Exception.\n");
        exit (1);
        break;
  }

  return (e);
}

/**Function********************************************************************
  Synopsis    [Generate the Bdd encoding a Boolean expression]
  Description [Generate the Bdd encoding a Boolean expression.
    Result is generated]
  SideEffects []
  SeeAlso     [Ddi_ExprMakeFromBdd]
******************************************************************************/
Ddi_Bdd_t *
Ddi_ExprToBdd(
  Ddi_Expr_t *e
)
{
  DdiConsistencyCheck(e,Ddi_Expr_c);
  Pdtutil_Assert(e->common.code == Ddi_Expr_Bdd_c,
    "converting to BDD a non BDD expression.");
  return ((Ddi_Bdd_t *)(e->data.bdd));
}

/**Function********************************************************************
  Synopsis    [Build a Ddi_Expr_t from a given string.]
  Description [Build a Ddi_Expr_t  from a given string.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Expr_t *
Ddi_ExprMakeFromString (
  Ddi_Mgr_t *mgr,
  char *s
)
{
  Ddi_Expr_t *r;

  r = (Ddi_Expr_t *)DdiGenericAlloc(Ddi_Expr_c,mgr);
  r->common.code = Ddi_Expr_String_c;
  r->data.string = Pdtutil_StrDup(s);

  return (r);
}

/**Function********************************************************************
  Synopsis    [Retrieve the string associated to the expression]
  Description [Retrieve the string associated to the expression.
    Result is NOT duplicated (in other words, the BDD field is read)]
  SideEffects []
  SeeAlso     [Ddi_ExprMakeFromString]
******************************************************************************/
char *
Ddi_ExprToString(
  Ddi_Expr_t *e
)
{
  DdiConsistencyCheck(e,Ddi_Expr_c);
  Pdtutil_Assert(e->common.code == Ddi_Expr_String_c,
    "converting to string a non string expression.");
  return (e->data.string);
}

/**Function********************************************************************
  Synopsis    [Build a Ctl Ddi_Expr_t from given sub-expressions.]
  Description [Build a Ctl Ddi_Expr_t from given sub-expressions.]
  SideEffects []
  SeeAlso     [Ddi_ExprMakeFromBdd Ddi_ExprMakeFromString]
******************************************************************************/
Ddi_Expr_t *
Ddi_ExprCtlMake (
  Ddi_Mgr_t *mgr,
  int        opcode,
  Ddi_Expr_t *op1,
  Ddi_Expr_t *op2,
  Ddi_Expr_t *op3
)
{
  Ddi_Expr_t *r;

  r = (Ddi_Expr_t *)DdiGenericAlloc(Ddi_Expr_c,mgr);
  r->common.code = Ddi_Expr_Ctl_c;
  r->opcode = opcode;
  r->data.sub = DdiArrayAlloc(1);
  DdiArrayWrite(r->data.sub,0,(Ddi_Generic_t *)op1,Ddi_Dup_c);
  if (op2 != NULL) {
    DdiArrayWrite(r->data.sub,1,(Ddi_Generic_t *)op2,Ddi_Dup_c);
    if (op3 != NULL) {
      DdiArrayWrite(r->data.sub,2,(Ddi_Generic_t *)op3,Ddi_Dup_c);
    }
  }

  return (r);
}

/**Function********************************************************************
  Synopsis    [Build a Boolean Ddi_Expr_t from given sub-expressions.]
  Description [Build a Boolean Ddi_Expr_t from given sub-expressions.]
  SideEffects []
  SeeAlso     [Ddi_ExprMakeFromBdd Ddi_ExprMakeFromString]
******************************************************************************/
Ddi_Expr_t *
Ddi_ExprBoolMake (
  Ddi_Mgr_t *mgr,
  Ddi_Expr_t *op1,
  Ddi_Expr_t *op2
)
{
  Ddi_Expr_t *r;

  r = (Ddi_Expr_t *)DdiGenericAlloc(Ddi_Expr_c,mgr);
  r->common.code = Ddi_Expr_Bool_c;
  r->data.sub = DdiArrayAlloc(1);
  DdiArrayWrite(r->data.sub,0,(Ddi_Generic_t *)op1,Ddi_Dup_c);
  if (op2 != NULL) {
    DdiArrayWrite(r->data.sub,1,(Ddi_Generic_t *)op2,Ddi_Dup_c);
  }

  return (r);
}

/**Function********************************************************************
  Synopsis    [Write operand sub-expression to expression at given position.]
  Description [Write operand sub-expression to expression at given position.
    Sub-expression is added if not present, rewritten (by freeing the old one)
    if present at specified position]
  SideEffects []
  SeeAlso     [Ddi_ExprMakeFromBdd Ddi_ExprMakeFromString]
******************************************************************************/
Ddi_Expr_t *
Ddi_ExprWriteSub (
  Ddi_Expr_t *e,
  int pos,
  Ddi_Expr_t *op
)
{
  DdiConsistencyCheck(e,Ddi_Expr_c);
  Pdtutil_Assert(!Ddi_ExprIsTerminal(e),
    "Ctl or Bool expression required to read sub-expression!");
  DdiArrayWrite(e->data.sub,pos,(Ddi_Generic_t *)op,Ddi_Dup_c);

  return (e);
}


/**Function********************************************************************
  Synopsis    [Read the number of sub-expressions.]
  Description [Read the number of sub-expressions.
    In case of terminal expressioins (Bdd or String), 0 is returned.]
  SideEffects []
******************************************************************************/
int
Ddi_ExprSubNum (
  Ddi_Expr_t *e
)
{
  DdiConsistencyCheck(e,Ddi_Expr_c);
  switch (e->common.code) {
  case Ddi_Expr_Bdd_c:
  case Ddi_Expr_String_c:
    Pdtutil_Assert(0,"operation requires non terminal expression");
    break;
  case Ddi_Expr_Bool_c:
  case Ddi_Expr_Ctl_c:
    return (DdiArrayNum(e->data.sub));
    break;
  default:
    Pdtutil_Assert (0, "Wrong DDI node type");
  }
  return(-1);
}

/**Function********************************************************************
  Synopsis    [Rear sub-expression at given position]
  Description [Rear sub-expression at given position]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Expr_t *
Ddi_ExprReadSub(
  Ddi_Expr_t *e,
  int i
)
{
  DdiConsistencyCheck(e,Ddi_Expr_c);
  Pdtutil_Assert(!Ddi_ExprIsTerminal(e),
    "Ctl or Bool expression required to read sub-expression!");
  Pdtutil_Assert((i>=0)&&(i<Ddi_ExprSubNum(e)),
    "sub-expression index out of bounds");
  return ((Ddi_Expr_t *)DdiArrayRead(e->data.sub,i));
}

/**Function********************************************************************
  Synopsis    [Read expression opcode]
  Description [Read expression opcode]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_ExprReadOpcode(
  Ddi_Expr_t *e
)
{
  return (e->opcode);
}

/**Function********************************************************************
  Synopsis    [Return true (non 0) if expression is terminal (Bdd or string).]
  Description [Return true (non 0) if expression is terminal (Bdd or string).]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
int
Ddi_ExprIsTerminal (
  Ddi_Expr_t *e
)
{
  return ((Ddi_ReadCode(e)==Ddi_Expr_Bdd_c)||
          (Ddi_ReadCode(e)==Ddi_Expr_String_c));
}


/**Function********************************************************************
  Synopsis    [Load EXPRESSION from file]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/

Ddi_Expr_t *
Ddi_ExprLoad (
  Ddi_BddMgr *dd      /* dd manager */,
  char *filename     /* file name */,
  FILE *fp           /* file pointer */
  )
{
  return(Expr_ExprLoad(dd,filename,fp));
}


/**Function********************************************************************
  Synopsis     [Duplicate a Ddi_Expr_t]
  Description  [Duplicate a Ddi_Expr_t. Duplication is propagated
    recursively.]
  SideEffects  []
******************************************************************************/
Ddi_Expr_t *
Ddi_ExprDup (
  Ddi_Expr_t *f    /* expression to be duplicated */
)
{
  Ddi_Expr_t *r;

  DdiConsistencyCheck(f,Ddi_Expr_c);
  r = (Ddi_Expr_t *)DdiGenericDup((Ddi_Generic_t *)f);

  return (r);
}


/**Function********************************************************************
  Synopsis     [Print a Ddi_Expr_t]
  Description  [Print a Ddi_Expr_t]
  SideEffects  []
******************************************************************************/
void
Ddi_ExprPrint (
  Ddi_Expr_t *f,
  FILE *fp
)
{
  Expr_WriteTree(fp,f);
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/












/**CFile***********************************************************************

  FileName    [exprRead.c]

  PackageName [expr]

  Synopsis    [Input of PdTrav expressions]

  Description [Interface routine to expr parsing]

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

#include "exprInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

extern FILE *ExprYyin;
Ddi_Mgr_t *ExprYyDdmgr;
Ddi_Expr_t *ExprYySpecif;

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

static void listStringLeafsIntern(Ddi_Expr_t *e, char ***strP, int *numP, int *maxnP);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Load EXPRESSION from file]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/

Ddi_Expr_t *
Expr_ExprLoad (
  Ddi_BddMgr *dd      /* dd manager */,
  char *filename     /* file name */,
  FILE *fp           /* file pointer */
  )
{
  Ddi_Expr_t *f;
  int flagFile;

  ExprYyDdmgr = dd;

  f = NULL;

  fp = Pdtutil_FileOpen (fp, filename, "r", &flagFile);
  if (fp == NULL) {
    return (0);
  }

  ExprYyin = fp;

  ExprYyparse();
  f = ExprYySpecif;

  Expr_WriteTree(stdout,f);

  Pdtutil_FileClose (fp, &flagFile);

  return (f);
}

/**Function********************************************************************
  Synopsis    [Print EXPRESSION to file]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/


void
Expr_WriteTree(
  FILE *fout,
  Ddi_Expr_t *tnode
)
{
  int i,j;

  i = Ddi_GenericReadCode((Ddi_Generic_t *)tnode);
  if(i == Ddi_Expr_String_c){
    fprintf(fout,"%s",Ddi_ExprToString(tnode));
  }
  else if(i == Ddi_Expr_Ctl_c ){
    i = Ddi_ExprSubNum(tnode);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "(")
    );
    Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0));

    for(j=1;j<i;j++){
      int opcode = Ddi_ExprReadOpcode(tnode);
      switch (opcode) {

        case Expr_Ctl_SPEC_c:
        case Expr_Ctl_INVARSPEC_c:
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, " Spec ")
          );
        break;
        case Expr_Ctl_TERMINAL_c:
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, " Terminal ")
          );
        case Expr_Ctl_TRUE_c:
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, " True ")
          );
        case Expr_Ctl_FALSE_c:
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, " False ")
          );
        break;
        case Expr_Ctl_AND_c:
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, " AND ")
          );
        break;
        case Expr_Ctl_OR_c:
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, "OR ")
          );
        break;
        case Expr_Ctl_NOT_c:
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, " NOT ")
          );
        break;
        case Expr_Ctl_IFF_c:
        case Expr_Ctl_EQUAL_c:
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, " IFF ")
          );
        break;
        case Expr_Ctl_IMPLIES_c:
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, " IMPLIES ")
          );
        break;
        case Expr_Ctl_AG_c:
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, " AG ")
          );
        break;
        case Expr_Ctl_AF_c:
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, " AF ")
          );
        break;

        case Expr_Ctl_EX_c:
        case Expr_Ctl_AX_c:
        case Expr_Ctl_EF_c:
        case Expr_Ctl_EG_c:
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, " EX/AX/EF/EG ")
          );
        break;

        case Expr_Ctl_EU_c:
        case Expr_Ctl_AU_c:
        case Expr_Ctl_EBU_c:
        case Expr_Ctl_ABU_c:
        case Expr_Ctl_EBF_c:
        case Expr_Ctl_ABF_c:
        case Expr_Ctl_EBG_c:
        case Expr_Ctl_ABG_c:
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, " *U ")
          );
        break;

        case Expr_Ctl_TWODOTS_c:
        case Expr_Ctl_DOT_c:
        case Expr_Ctl_ARRAY_c:
        case Expr_Ctl_PLUS_c:
        case Expr_Ctl_MINUS_c:
        case Expr_Ctl_TIMES_c:
        case Expr_Ctl_DIVIDE_c:
        case Expr_Ctl_MOD_c:
        case Expr_Ctl_LT_c:
        case Expr_Ctl_GT_c:
        case Expr_Ctl_LE_c:
        case Expr_Ctl_GE_c:
        case Expr_Ctl_UNION_c:
        case Expr_Ctl_SETIN_c:
        case Expr_Ctl_SIGMA_c:
        case Expr_Ctl_TRUEEXP_c:
        case Expr_Ctl_CASE_c:
        case Expr_Ctl_COLON_c:

        case Expr_Ctl_Null_code_c:

        fprintf(stderr,"ERROR: Not yet supported EXPR opcode");
        break;
        default:
        fprintf(stderr,"ERROR: Unknown EXPR opcode");
        break;

      }

      Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,j));
    }
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, ")")
    );
  }

/*      i = Ddi_ExprReadOpcode(tnode); */
/*      switch(i){ */
/*      case SPEC:  */
/*        fprintf(fout,"SPEC\n\t"); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout,"\n"); */
/*        break; */
/*      case ATOM:   */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout,"%s",tnode->data.char); */
/*      case IMPLIES:   */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," -> "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        break; */
/*      case IFF:   */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," <-> "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        break; */
/*      case OR:   */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," | "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        break; */
/*      case AND:   */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," & "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        break; */
/*      case PLUS:   */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," + "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        break; */
/*      case MINUS:   */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," - "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        break; */
/*      case TIMES:   */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," * "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        break; */
/*      case DIVIDE:   */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," / "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        break; */
/*      case MOD:   */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," mod "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        break; */
/*      case EQUAL:   */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," = "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        break; */
/*      case LT:   */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," < "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        break; */
/*      case GT:   */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," > "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        break; */
/*      case LE:   */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," <= "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        break; */
/*      case GE:   */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," >= "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        break; */
/*      case UNION:   */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," union "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        break; */
/*      case SETIN:   */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," in "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        break; */
/*      case EX:   */
/*        fprintf(fout,"EX "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        break; */
/*      case AX:   */
/*        fprintf(fout,"AX "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        break; */
/*      case EF:   */
/*        fprintf(fout,"EF "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        break; */
/*      case AF:   */
/*        fprintf(fout,"AF "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        break; */
/*      case EG:   */
/*        fprintf(fout,"EG "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        break; */
/*      case AG:   */
/*        fprintf(fout,"AG "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        break; */
/*      case AU:   */
/*        fprintf(fout,"AA ( "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," U "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        fprintf(fout,")"); */
/*        break; */
/*      case EU:   */
/*        fprintf(fout,"EE ( "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,0)); */
/*        fprintf(fout," U "); */
/*        Expr_WriteTree(fout,Ddi_ExprReadSub(tnode,1)); */
/*        fprintf(fout,")"); */
/*        break; */
/*      }     */
  else
    fprintf(stderr, "DDI Code Unknown.\n");

  return;
}

/**Function********************************************************************
  Synopsis    [Generate string leafs as char * array]
  Description [Generate string leafs as char * array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
char **
Expr_ListStringLeafs(
  Ddi_Expr_t *e
)
{
  //Ddi_Expr_t *r = NULL;
  //Ddi_Expr_t *op0, *op1;
  //Ddi_Bdd_t *rBdd;
  //Ddi_Mgr_t *ddm = Ddi_ReadMgr(e);
  int num=0, maxn=10;
  char **strings = Pdtutil_Alloc(char *, maxn);

  listStringLeafsIntern(e,&strings,&num,&maxn);
  Pdtutil_Assert(num<maxn,"no space left in string array");
  strings[num] = NULL;
  return strings;
}

/**Function********************************************************************
  Synopsis    [Generate string leafs as char * array]
  Description [Generate string leafs as char * array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
listStringLeafsIntern(
  Ddi_Expr_t *e,
  char ***strP,
  int *numP,
  int *maxnP
)
{
  //Ddi_Expr_t *r = NULL;
  //Ddi_Expr_t *op0, *op1;
  //Ddi_Bdd_t *rBdd;
  //Ddi_Mgr_t *ddm = Ddi_ReadMgr(e);
  int opcode;

  switch(Ddi_GenericReadCode((Ddi_Generic_t *)e)) {
    case Ddi_Expr_Bdd_c:
    case Ddi_Expr_Bool_c:
    break;
    case Ddi_Expr_Ctl_c:

      opcode = Ddi_ExprReadOpcode(e);
      switch (opcode) {

        case Expr_Ctl_SPEC_c:
        case Expr_Ctl_INVARSPEC_c:
          listStringLeafsIntern(Ddi_ExprReadSub(e,0),strP,numP,maxnP);
        break;
        case Expr_Ctl_TERMINAL_c:
        case Expr_Ctl_TRUE_c:
        case Expr_Ctl_FALSE_c:
        break;

        case Expr_Ctl_NOT_c:
        case Expr_Ctl_AG_c:
        case Expr_Ctl_AF_c:
          listStringLeafsIntern(Ddi_ExprReadSub(e,0),strP,numP,maxnP);
        break;
        case Expr_Ctl_AND_c:
        case Expr_Ctl_OR_c:
        case Expr_Ctl_IFF_c:
        case Expr_Ctl_EQUAL_c:
        case Expr_Ctl_IMPLIES_c:
          listStringLeafsIntern(Ddi_ExprReadSub(e,0),strP,numP,maxnP);
          listStringLeafsIntern(Ddi_ExprReadSub(e,1),strP,numP,maxnP);
        break;

        case Expr_Ctl_EX_c:
        case Expr_Ctl_AX_c:
        case Expr_Ctl_EF_c:
        case Expr_Ctl_EG_c:
        break;

        case Expr_Ctl_EU_c:
        case Expr_Ctl_AU_c:
        case Expr_Ctl_EBU_c:
        case Expr_Ctl_ABU_c:
        case Expr_Ctl_EBF_c:
        case Expr_Ctl_ABF_c:
        case Expr_Ctl_EBG_c:
        case Expr_Ctl_ABG_c:

        case Expr_Ctl_TWODOTS_c:
        case Expr_Ctl_DOT_c:
        case Expr_Ctl_ARRAY_c:
        case Expr_Ctl_PLUS_c:
        case Expr_Ctl_MINUS_c:
        case Expr_Ctl_TIMES_c:
        case Expr_Ctl_DIVIDE_c:
        case Expr_Ctl_MOD_c:
        case Expr_Ctl_LT_c:
        case Expr_Ctl_GT_c:
        case Expr_Ctl_LE_c:
        case Expr_Ctl_GE_c:
        case Expr_Ctl_UNION_c:
        case Expr_Ctl_SETIN_c:
        case Expr_Ctl_SIGMA_c:
        case Expr_Ctl_TRUEEXP_c:
        case Expr_Ctl_CASE_c:
        case Expr_Ctl_COLON_c:

        case Expr_Ctl_Null_code_c:

        fprintf(stderr,"ERROR: Not yet supported EXPR opcode");
        break;
        default:
        fprintf(stderr,"ERROR: Unknown EXPR opcode");
        break;

      }

    break;

    case Ddi_Expr_String_c:
    {
      char *s = Pdtutil_Alloc(char, strlen(Ddi_ExprToString(e))+3);
      sprintf(s,"n %s", Ddi_ExprToString(e));
      if (*numP >= *maxnP-1) {
      *maxnP *= 2;
      *strP = Pdtutil_Realloc(char *, *strP, *maxnP);
      }
      (*strP)[*numP] = s;
      (*numP)++;
    }
    break;

    default:
    fprintf(stderr,"ERROR: Unknown EXPR opcode");
    break;
  }

}


/**Function********************************************************************
  Synopsis    [Print EXPRESSION to file]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/

Ddi_Expr_t *
Expr_EvalBddLeafs (
  Ddi_Expr_t *e
)
{
  Ddi_Expr_t *r = NULL;
  Ddi_Expr_t *op0, *op1;
  Ddi_Bdd_t *rBdd;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(e);
  int opcode;

  switch(Ddi_GenericReadCode((Ddi_Generic_t *)e)) {
    case Ddi_Expr_Bdd_c:
    r = Ddi_ExprDup(e);
    break;
    case Ddi_Expr_Bool_c:
    break;
    case Ddi_Expr_Ctl_c:

      opcode = Ddi_ExprReadOpcode(e);
      switch (opcode) {

        case Expr_Ctl_SPEC_c:
        case Expr_Ctl_INVARSPEC_c:
          op0 = Expr_EvalBddLeafs(Ddi_ExprReadSub(e,0));
          r = Ddi_ExprCtlMake(ddm,opcode,op0,NULL,NULL);
          Ddi_Free(op0);
        break;
        case Expr_Ctl_TERMINAL_c:
        case Expr_Ctl_TRUE_c:
        case Expr_Ctl_FALSE_c:
        break;

        case Expr_Ctl_AND_c:
          op0 = Expr_EvalBddLeafs(Ddi_ExprReadSub(e,0));
          op1 = Expr_EvalBddLeafs(Ddi_ExprReadSub(e,1));
          rBdd = Ddi_BddAnd(Ddi_ExprToBdd(op0),Ddi_ExprToBdd(op1));
          Ddi_Free(op0);
          Ddi_Free(op1);
          r = Ddi_ExprMakeFromBdd (rBdd);
          Ddi_Free(rBdd);
        break;
        case Expr_Ctl_OR_c:
          op0 = Expr_EvalBddLeafs(Ddi_ExprReadSub(e,0));
          op1 = Expr_EvalBddLeafs(Ddi_ExprReadSub(e,1));
          rBdd = Ddi_BddOr(Ddi_ExprToBdd(op0),Ddi_ExprToBdd(op1));
          Ddi_Free(op0);
          Ddi_Free(op1);
          r = Ddi_ExprMakeFromBdd (rBdd);
          Ddi_Free(rBdd);
        break;
        case Expr_Ctl_NOT_c:
          op0 = Expr_EvalBddLeafs(Ddi_ExprReadSub(e,0));
          rBdd = Ddi_BddNot(Ddi_ExprToBdd(op0));
          Ddi_Free(op0);
          r = Ddi_ExprMakeFromBdd (rBdd);
          Ddi_Free(rBdd);
        break;
        case Expr_Ctl_IFF_c:
        case Expr_Ctl_EQUAL_c:
          op0 = Expr_EvalBddLeafs(Ddi_ExprReadSub(e,0));
          op1 = Expr_EvalBddLeafs(Ddi_ExprReadSub(e,1));
          rBdd = Ddi_BddXnor(Ddi_ExprToBdd(op0),Ddi_ExprToBdd(op1));
          Ddi_Free(op0);
          Ddi_Free(op1);
          r = Ddi_ExprMakeFromBdd (rBdd);
          Ddi_Free(rBdd);
        break;
        case Expr_Ctl_IMPLIES_c:
          op0 = Expr_EvalBddLeafs(Ddi_ExprReadSub(e,0));
          op1 = Expr_EvalBddLeafs(Ddi_ExprReadSub(e,1));
          Ddi_BddNotAcc(Ddi_ExprToBdd(op0));
          rBdd = Ddi_BddOr(Ddi_ExprToBdd(op0),Ddi_ExprToBdd(op1));
          Ddi_Free(op0);
          Ddi_Free(op1);
          r = Ddi_ExprMakeFromBdd (rBdd);
          Ddi_Free(rBdd);
        break;

        case Expr_Ctl_AG_c:
          op0 = Expr_EvalBddLeafs(Ddi_ExprReadSub(e,0));
          r = Ddi_ExprCtlMake(ddm,opcode,op0,NULL,NULL);
          Ddi_Free(op0);
        break;

        case Expr_Ctl_AF_c:
          op0 = Expr_EvalBddLeafs(Ddi_ExprReadSub(e,0));
          r = Ddi_ExprCtlMake(ddm,opcode,op0,NULL,NULL);
          Ddi_Free(op0);
        break;

        case Expr_Ctl_EX_c:
        case Expr_Ctl_AX_c:
        case Expr_Ctl_EF_c:
        case Expr_Ctl_EG_c:
        break;

        case Expr_Ctl_EU_c:
        case Expr_Ctl_AU_c:
        case Expr_Ctl_EBU_c:
        case Expr_Ctl_ABU_c:
        case Expr_Ctl_EBF_c:
        case Expr_Ctl_ABF_c:
        case Expr_Ctl_EBG_c:
        case Expr_Ctl_ABG_c:

        case Expr_Ctl_TWODOTS_c:
        case Expr_Ctl_DOT_c:
        case Expr_Ctl_ARRAY_c:
        case Expr_Ctl_PLUS_c:
        case Expr_Ctl_MINUS_c:
        case Expr_Ctl_TIMES_c:
        case Expr_Ctl_DIVIDE_c:
        case Expr_Ctl_MOD_c:
        case Expr_Ctl_LT_c:
        case Expr_Ctl_GT_c:
        case Expr_Ctl_LE_c:
        case Expr_Ctl_GE_c:
        case Expr_Ctl_UNION_c:
        case Expr_Ctl_SETIN_c:
        case Expr_Ctl_SIGMA_c:
        case Expr_Ctl_TRUEEXP_c:
        case Expr_Ctl_CASE_c:
        case Expr_Ctl_COLON_c:

        case Expr_Ctl_Null_code_c:

        fprintf(stderr,"ERROR: Not yet supported EXPR opcode");
        break;
        default:
        fprintf(stderr,"ERROR: Unknown EXPR opcode");
        break;

      }

    break;

    case Ddi_Expr_String_c:
    {
      Ddi_Bdd_t *l;
      Ddi_Var_t *v;
      v = Ddi_VarFromName(Ddi_ReadMgr(e),Ddi_ExprToString(e));
      if (v==NULL) {
      Ddi_Bdd_t *node =
          (Ddi_Bdd_t *) Ddi_NodeFromName(ddm,Ddi_ExprToString(e));
        if (node!=NULL) {
        //          DdiConsistencyCheck(node,Ddi_Bdd_c);
          l = Ddi_BddDup(node);
      }
        else {
          Pdtutil_Warning(v==NULL,"missing variable while converting expr");
          v = Ddi_VarNew(ddm);
          Ddi_VarAttachName(v,Ddi_ExprToString(e));
          l = Ddi_BddMakeLiteral(v,1);
        }
      }
      else {
        l = Ddi_BddMakeLiteral(v,1);
      }
      r = Ddi_ExprMakeFromBdd(l);
      Ddi_Free(l);
    }
    break;

    default:
    fprintf(stderr,"ERROR: Unknown EXPR opcode");
    break;
  }

  return (r);

}


/**Function********************************************************************
  Synopsis    [Print EXPRESSION to file]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/

Ddi_Bdd_t *
Expr_InvarMakeFromAG (
  Ddi_Expr_t *e
)
{
  if (Ddi_GenericReadCode((Ddi_Generic_t *)e)==Ddi_Expr_Ctl_c) {
     if (Ddi_ExprReadOpcode(e) == Expr_Ctl_SPEC_c) {
       e = Ddi_ExprReadSub(e,0);
       if (Ddi_ExprReadOpcode(e) == Expr_Ctl_AG_c) {
         e = Ddi_ExprReadSub(e,0);
         if (Ddi_GenericReadCode((Ddi_Generic_t *)e)==Ddi_Expr_Bdd_c) {
         return (Ddi_BddDup(Ddi_ExprToBdd(e)));
       }
       }
     }
  }
  Pdtutil_Warning(1,"NO CTL AG expression converted");
  return (NULL);

}








/**CFile***********************************************************************

  FileName    [fsmUtil.c]

  PackageName [fsm]

  Synopsis    [Utility functions to manipulate a FSM]

  Description []

  SeeAlso     []

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

  Revision    []

******************************************************************************/

#include "fsmInt.h"

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
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Given a string it Returns an Integer.]

  Description [It receives a string; to facilitate the user that string
    can be an easy-to-remember predefined code or an integer number
    (interpreted as a string).
    It returns the integer DDDMP value.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Fsm_BddFormatString2Int(
  char *string                  /* String to Analyze */
)
{
  if (strcmp(string, "default") == 0 || strcmp(string, "0") == 0) {
    return (DDDMP_MODE_DEFAULT);
  }

  if (strcmp(string, "text") == 0 || strcmp(string, "1") == 0) {
    return (DDDMP_MODE_TEXT);
  }

  if (strcmp(string, "binary") == 0 || strcmp(string, "2") == 0) {
    return (DDDMP_MODE_BINARY);
  }

  Pdtutil_Warning(1, "Choice Not Allowed For BDD File Format.");
  return (DDDMP_MODE_DEFAULT);
}

/**Function********************************************************************

  Synopsis    [Given an Integer Returns a string]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

char *
Fsm_BddFormatInt2String(
  int intValue
)
{
  switch (intValue) {
    case DDDMP_MODE_DEFAULT:
      return ("default");
      break;
    case DDDMP_MODE_TEXT:
      return ("text");
      break;
    case DDDMP_MODE_BINARY:
      return ("binary");
      break;
    default:
      Pdtutil_Warning(1, "Choice Not Allowed.");
      return ("default");
      break;
  }
}

/**Function********************************************************************

 Synopsis    [Prints Statistics on the FSM Structure]

 SideEffects [none]

 SeeAlso     [Ddi_MgrPrintStats, Tr_MgrPrintStats, Trav_MgrPrintStats]

******************************************************************************/

void
Fsm_MgrPrintStats(
  Fsm_Mgr_t * fsmMgr
)
{
  double nStates;

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Finite State Machine Manager Name %s.\n",
      Fsm_MgrReadFsmName(fsmMgr));
    fprintf(stdout, "FSM File Name: %s.\n", Fsm_MgrReadFileName(fsmMgr));
    fprintf(stdout, "Number of PI, PO, FF, AuxVar: %d, %d, %d, %d.\n",
      Fsm_MgrReadNI(fsmMgr), Fsm_MgrReadNO(fsmMgr), Fsm_MgrReadNL(fsmMgr),
      Fsm_MgrReadNAuxVar(fsmMgr));
    );

  if (Fsm_MgrReadTrBDD(fsmMgr) != NULL) {
    Ddi_Bdd_t *tr;

    tr = Fsm_MgrReadTrBDD(fsmMgr);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "|TR|: %d.\n", Ddi_BddSize(tr))
      );
  }

  if (Fsm_MgrReadReachedBDD(fsmMgr) != NULL) {
    Ddi_Bdd_t *reached;
    Ddi_Vararray_t *psv;

    reached = Fsm_MgrReadReachedBDD(fsmMgr);
    psv = Fsm_MgrReadVarPS(fsmMgr);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "|Reached|: %d.\n", Ddi_BddSize(reached))
      );

    nStates = Ddi_BddCountMinterm(reached, Ddi_VararrayNum(psv));

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "#ReachedStates: %g.\n", nStates)
      );
  }



  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Total CPU Time: %s.\n", util_print_time(fsmMgr->fsmTime))
    );

  return;
}

/**Function********************************************************************

 Synopsis    [Prints the permutation on file "tmp.prm"]

 SideEffects [none]

 SeeAlso     []

******************************************************************************/

void
Fsm_MgrPrintPrm(
  Fsm_Mgr_t * fsmMgr
)
{
  int i, flagFile;
  FILE *fp;
  Ddi_Mgr_t *dd = Fsm_MgrReadDdManager(fsmMgr);

  fp = Pdtutil_FileOpen(NULL, "tmp.prm", "w", &flagFile);

  for (i = 0; i < Ddi_MgrReadSize(dd); i++) {
    fprintf(fp, "%d.\n", Ddi_MgrReadPerm(dd, i));
  }

  Pdtutil_FileClose(fp, &flagFile);

  return;
}

/**Function********************************************************************

 Synopsis    [Reads constrain]

 SideEffects [none]

 SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Pdtutil_ReadConstrain(
  Fsm_Mgr_t * fsmMgr,
  char *diagfile
)
{
#if 1
  fprintf(stderr, "Not Implemented Pdtutil_ReadConstrain.\n");
  return NULL;
#else
  FILE *fp;
  Ddi_Bddarray_t *Fa;
  Ddi_Bdd_t *tmp, *tr, *constrain, *diag, *X, *S, *Y, *Z, *SeY;
  Ddi_Bdd_t *one, *zero;
  int i, j, k, flagFile, var, fixval;
  char *fname;
  Ddi_Mgr_t *dd = Fsm_MgrReadDdManager(fsmMgr);
  char neg = 0;

  one = Ddi_MgrReadOne(Fsm_MgrReadDdManager(fsmMgr));

  /* TR constraining by imposing a partial S=Y diagonal */

  if (diagfile[0] == '!') {
    fname = &diagfile[1];
    neg = 1;
  } else
    fname = diagfile;

  if (fname[0] == '$') {
    Fa = Ddi_BddarrayLoad(dd, NULL, NULL, Fsm_MgrReadBDDFormat(fsmMgr),
      &fname[1], NULL);
    diag = Ddi_BddarrayRead(Fa, 0);
    Ddi_BddarrayWrite(Fa, 0, one);
    Ddi_Free(Fa);
  } else {
    diag = one;
    fp = Pdtutil_FileOpen(fp, fname, "r", &flagFile);
    if (fp != NULL) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "Reading %s.\n", fname)
        );

      while (fscanf(fp, "%d %d", &i, &fixval) > 0) {
        S = Ddi_VararrayRead(Fsm_MgrReadVarPS(fsmMgr), i);
        var = S->index - POLI_FSM_SX_OFFSET + POLI_FSM_Z_OFFSET;
        Y = Ddi_VararrayRead(Fsm_MgrReadVarNS(fsmMgr), i);

        switch (fixval) {
          case -2:
            SeY = Ddi_BddXor(Y, S);
            break;
          case -1:
            SeY = Ddi_BddXnor(Y, S);
            break;
          case 0:
            SeY = Ddi_BddNot(S);
            break;
          case 1:
            SeY = S;
/* tolto: da rifare     Ddi_Ref(SeY); */
            break;
          case 2:
            SeY = Ddi_BddNot(Y);
            break;
          case 3:
            SeY = Y;
/* tolto: da rifare     Ddi_Ref(SeY); */
            break;
          default:
            fprintf(stderr,
              "Error: Unknown Value in constrain File Selection.\n");
        }

        tmp = Ddi_BddAnd(SeY, diag);
        Ddi_Free(diag);
        Ddi_Free(SeY);
        diag = tmp;
      }                         /* end-while */
    }                           /* end-if */
  }                             /* end-else */

  if (neg)
    return Ddi_Not(diag);
  else
    return diag;

#endif

}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

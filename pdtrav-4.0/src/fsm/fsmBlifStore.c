/**CFile***********************************************************************

  FileName    [fsmBlifStore.c]

  PackageName [fsm]

  Synopsis    [Functions to write a description of the FSMs in a blif format.]

  Description [External procedures included in this module:
    <ul>
    <li> Fsm_BlifStore ()
    </ul>
    ]

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

  Synopsis     [Stores (on file or stdout) of FSM structure in a blif
    format.]

  Description  [If the filename is stdout print out the file on standard
    output.]

  SideEffects  []

  SeeAlso      [Ddi_BddarrayStore,Ddi_BddarrayRead,Ddi_BddarrayNum]

******************************************************************************/

int
Fsm_BlifStore(
  Fsm_Mgr_t * fsmMgr /* FSM structure */ ,
  int reduceDelta /* If 0 Store Original Delta, if 1 Store Dummy Delta */ ,
  char *fileName /* Output File Name */ ,
  FILE * fp                     /* File Pointer */
)
{
  int i, j, flag, flagFile, flagNameRepeated;
  char *word, *fsmName, *outName;
  Ddi_Mgr_t *dd;
  Ddi_Bddarray_t *tmpArray;
  Ddi_Bdd_t *tmpFunction;
  Ddi_Varset_t *supp;
  long fsmTime;

  fsmTime = util_cpu_time();

  /*------------------------- Check For Correctness -------------------------*/

  if (fsmMgr == NULL) {
    Pdtutil_Warning(1, "NULL FSM Manager.");
    return (1);
  }

  /*---------------------- Create Name and Open File ------------------------*/

  if (strcmp(fileName, "stdout") == 0) {
    fp = stdout;
  } else {
    fsmName = Pdtutil_FileName(fileName, "", "blif", 0);
    fp = Pdtutil_FileOpen(fp, fsmName, "w", &flagFile);
  }

  /*--------------------------- Take dd Manager -----------------------------*/

  dd = Fsm_MgrReadDdManager(fsmMgr);

  /*----------------------------- Store Header ------------------------------*/

  if (Fsm_MgrReadFileName(fsmMgr) != NULL) {
    fprintf(fp, ".model %s\n", Fsm_MgrReadFileName(fsmMgr));
  } else {
    fprintf(fp, ".model %s\n", "blif");
  }
  fflush(fp);

  /*----------------------------- Store Name -------------------------------*/

  /*
   *  .inputs
   */

  if (Fsm_MgrReadNI(fsmMgr) > 0) {
    fprintf(fp, ".inputs ");
    for (i = 0; i < Fsm_MgrReadNI(fsmMgr); i++) {
      Ddi_Var_t *vp = Ddi_VararrayRead(fsmMgr->var.i, i);
      fprintf(fp, "%s ", Ddi_VarName(vp));
    }
    fprintf(fp, "\n");
  }

  /*
   *  .o
   */

  if (Fsm_MgrReadNO(fsmMgr) > 0) {
    fprintf(fp, ".outputs ");
    for (i = 0; i < Fsm_MgrReadNO(fsmMgr); i++) {
      fprintf(fp, "out_%d ", i);
    }
    fprintf(fp, "\n");
  }

  /*
   *  Latches
   */

  fprintf(fp, ".wire_load_slope 0.00\n");
  if (Fsm_MgrReadNL(fsmMgr) > 0) {
    for (i = 0; i < Fsm_MgrReadNL(fsmMgr); i++) {
      Ddi_Var_t *vps = Ddi_VararrayRead(fsmMgr->var.ps, i);
      Ddi_Var_t *vns = Ddi_VararrayRead(fsmMgr->var.ns, i);
      fprintf(fp, ".latch %s", Ddi_VarName(vns));
      fprintf(fp, " %s 0\n", Ddi_VarName(vps));
    }
  }
  fflush(fp);

  /*---------------------------- Store Delta --------------------------------*/

  if (Fsm_MgrReadDeltaBDD(fsmMgr) != NULL) {

    for (i = 0; i < Fsm_MgrReadNL(fsmMgr); i++) {
      Ddi_Var_t *vp;

      vp = Ddi_VararrayRead(fsmMgr->var.ns, i);
      outName = Ddi_VarName(vp);
      tmpArray = Fsm_MgrReadDeltaBDD(fsmMgr);
      tmpFunction = Ddi_BddarrayRead(tmpArray, i);
      Ddi_BddSetMono(tmpFunction);
      supp = Ddi_BddSupp(tmpFunction);

      flagNameRepeated = 0;
      for (j = 0; j < i && flagNameRepeated == 0; j++) {
        vp = Ddi_VararrayRead(fsmMgr->var.ns, j);
        if (strcmp(outName, Ddi_VarName(vp)) == 0) {
          flagNameRepeated = 1;
        }
      }

      /*
       *  Take care of:
       *  .latch ff a 0
       *  .latch ff b 0
       *  .latch ff c 0
       *  to avoid multiple .names (one for each .latch, instead of just one for
       *  ff, e.g., n2782gat for s5378-179)
       */

      if (flagNameRepeated == 1) {
        continue;
      }

      /*
       *  Take care of:
       *  .inputs in
       *  .latch in s 0
       *  .name in in
       *  1 1
       *  to be avoided
       */

      flag = 0;
      for (j = 0; j < Fsm_MgrReadNI(fsmMgr) && flag == 0; j++) {
        vp = Ddi_VararrayRead(fsmMgr->var.i, j);
        if (strcmp(outName, Ddi_VarName(vp)) == 0) {
          flag = 1;
        }
      }
      if (flag == 1) {
        continue;
      }

      /*
       *  Print NULL PLA
       */

      if (Ddi_VarsetNum(supp) == 0) {
        fprintf(fp, ".names %s\n1\n", outName);
        continue;
      }

      /*
       *  Print PLA
       */

      fprintf(fp, ".names ");

      Ddi_VarsetPrint(supp, -1, NULL, fp);
      /* To Print-Out the Output name on the same line ...
         seek on the last caracter -1  */
      fseek(fp, ftell(fp) - 1, SEEK_SET);
      fprintf(fp, "%s\n", outName);

      if (reduceDelta == 1) {
        for (j = 0; j < Ddi_VarsetNum(supp); j++) {
          fprintf(fp, "1");
        }
        fprintf(fp, " 1\n");
      } else {
        Ddi_BddPrintCubes(tmpFunction, supp, -1, 1, NULL, fp);
      }

      Ddi_Free(supp);
      fflush(fp);
    }
  }

  /*--------------------------- Store Lambda --------------------------------*/

  if (Fsm_MgrReadLambdaBDD(fsmMgr) != NULL) {

    for (i = 0; i < Fsm_MgrReadNO(fsmMgr); i++) {
      char buf[PDTUTIL_MAX_STR_LEN];
      sprintf(buf, "out_%d", i);
      outName = buf;
      //outName = fsmMgr->name.o[i];
      tmpArray = Fsm_MgrReadLambdaBDD(fsmMgr);
      tmpFunction = Ddi_BddarrayRead(tmpArray, i);
      Ddi_BddSetMono(tmpFunction);
      supp = Ddi_BddSupp(tmpFunction);

      /*
       *  Take care of:
       *  .output ... name ...
       *  .latch name a 0
       *  to avoid multiple .names (one for .latch and one for .output,
       *  instead of just one for name, e.g., I818 for s6669)
       */

      flagNameRepeated = 0;
      for (j = 0; j < Fsm_MgrReadNL(fsmMgr) && flagNameRepeated == 0; j++) {
        Ddi_Var_t *vp = Ddi_VararrayRead(fsmMgr->var.ns, j);
        if (strcmp(outName, Ddi_VarName(vp)) == 0) {
          flagNameRepeated = 1;
        }
      }

      if (flagNameRepeated == 1) {
        continue;
      }

      /*
       *  Take care of:
       *  .outputs out
       *  .latch y out 0
       *  .name out out
       *  1 1
       *  to be avoided
       */

      flag = 0;
      for (j = 0; j < Fsm_MgrReadNL(fsmMgr) && flag == 0; j++) {
        Ddi_Var_t *vp = Ddi_VararrayRead(fsmMgr->var.ps, j);
        if (strcmp(outName, Ddi_VarName(vp)) == 0) {
          flag = 1;
        }
      }
      if (flag == 1) {
        continue;
      }

      /*
       *  Print NULL PLA
       */

      if (Ddi_VarsetNum(supp) == 0) {
        fprintf(fp, ".names %s\n1\n", outName);
        continue;
      }

      /*
       *  Print PLA
       */

      fprintf(fp, ".names ");

      Ddi_VarsetPrint(supp, -1, NULL, fp);
      /* To Print-Out the Output name on the same line ...
         seek on the last caracter -1  */
      fseek(fp, ftell(fp) - 1, SEEK_SET);
      fprintf(fp, "%s\n", outName);

      if (reduceDelta == 1) {
        for (j = 0; j < Ddi_VarsetNum(supp); j++) {
          fprintf(fp, "1");
        }
        fprintf(fp, " 1\n");
      } else {
        Ddi_BddPrintCubes(tmpFunction, supp, -1, 1, NULL, fp);
      }

      Ddi_Free(supp);
      fflush(fp);
    }
  }

  /*--------------------------- Close Blif File -----------------------------*/

  fprintf(fp, ".end\n");
  fflush(fp);
  Pdtutil_FileClose(fp, &flagFile);

  /*
   *  return 0 = All OK
   */

  fsmTime = util_cpu_time() - fsmTime;
  fsmMgr->fsmTime += fsmTime;

  return (0);
}

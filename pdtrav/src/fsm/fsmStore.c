/**CFile***********************************************************************

  FileName    [fsmStore.c]

  PackageName [fsm]

  Synopsis    [Functions to write description of FSMs]

  Description [External procedures included in this module:
    <ul>
    <li> Fsm_MgrStore ()
    </ul>
    ]

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

  Synopsis     [Stores (on file o stdout) of FSM structure.]

  Description  []

  SideEffects  []

  SeeAlso      [Ddi_BddarrayStore,Ddi_BddarrayRead,Ddi_BddarrayNum]

******************************************************************************/

int
Fsm_MgrStore(
  Fsm_Mgr_t * fsmMgr /* FSM Structure */ ,
  char *fileName /* Output File Name */ ,
  FILE * fp /* File Pointer */ ,
  int bddFlag /* Flag to Store or Not BDD on Files */ ,
  int bddFormat /* 0 = default, 1 = text, 2 = binary */ ,
  Pdtutil_VariableOrderFormat_e ordFileFormat
)
{
  int i, flagFile, v;
  char *word, *fsmName, *tmpName;
  Ddi_Mgr_t *dd;
  long fsmTime;
  char **varnames=Ddi_MgrReadVarnames(fsmMgr->dd);
  int *varauxids=Ddi_MgrReadVarauxids(fsmMgr->dd);

  fsmTime = util_cpu_time();

  /*------------------------- Check For Correctness -------------------------*/

  if (Fsm_MgrCheck(fsmMgr) == 1) {
    return (1);
  }

  /*---------------------- Create Name and Open File ------------------------*/

  fsmName = Pdtutil_FileName(fileName, "", "fsm", 0);
  fp = Pdtutil_FileOpen(fp, fsmName, "w", &flagFile);

  /*--------------------------- Take dd Manager -----------------------------*/

  dd = Fsm_MgrReadDdManager(fsmMgr);

  /*---------------------------- Set BDD Format -----------------------------*/

  if (bddFormat == DDDMP_MODE_DEFAULT) {
    bddFormat = Fsm_MgrReadBddFormat(fsmMgr);
  }

  /*----------------------------- Store Header ------------------------------*/

  if (Fsm_MgrReadFileName(fsmMgr) != NULL) {
    fprintf(fp, ".%s %s\n", FsmToken2String(KeyFSM_FSM),
      Fsm_MgrReadFileName(fsmMgr));
  } else {
    if (Fsm_MgrReadFsmName(fsmMgr) != NULL) {
      fprintf(fp, ".%s %s\n", FsmToken2String(KeyFSM_FSM),
        Fsm_MgrReadFsmName(fsmMgr));
    } else {
      fprintf(fp, ".%s %s\n", FsmToken2String(KeyFSM_FSM), "fsm");
    }
  }

  fprintf(fp, "\n");
  fflush(fp);

  /*------------------------------ Store Size -------------------------------*/

  /* Opening Section */
  fprintf(fp, ".%s\n", FsmToken2String(KeyFSM_SIZE));
  fflush(fp);

  /*
   *  .i
   */

  fprintf(fp, "   .%s %d\n", FsmToken2String(KeyFSM_I), Fsm_MgrReadNI(fsmMgr));

  /*
   *  .o
   */

  fprintf(fp, "   .%s %d\n", FsmToken2String(KeyFSM_O), Fsm_MgrReadNO(fsmMgr));

  /*
   *  .l
   */

  fprintf(fp, "   .%s %d\n", FsmToken2String(KeyFSM_L), Fsm_MgrReadNL(fsmMgr));

  /*
   *  .cutPoint
   */

  if (Fsm_MgrReadNAuxVar(fsmMgr) > 0) {
    fprintf(fp, "   .%s %d\n", FsmToken2String(KeyFSM_AUXVAR),
      Fsm_MgrReadNAuxVar(fsmMgr));
  }

  /* Closing Section */
  fprintf(fp, ".%s%s\n", FsmToken2String(KeyFSM_END),
    FsmToken2String(KeyFSM_SIZE));
  fprintf(fp, "\n");
  fflush(fp);

  /*------------------------------Store Order ------------------------------*/

  /* Opening Section */
  fprintf(fp, ".%s\n", FsmToken2String(KeyFSM_ORD));
  fflush(fp);

  fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_FILE_ORD));
  tmpName = Pdtutil_FileName(fileName, "FSM", "ord", 1);
  fprintf(fp, "%s\n", tmpName);

  /* Force Order with Auxiliary Index and Comments */
  Ddi_MgrOrdWrite(dd, tmpName, NULL, ordFileFormat);

  /* Closing Section */
  fprintf(fp, ".%s%s\n", FsmToken2String(KeyFSM_END),
    FsmToken2String(KeyFSM_ORD));
  fprintf(fp, "\n");
  fflush(fp);

  /*----------------------------- Store Name -------------------------------*/

  /* Opening Section */
  fprintf(fp, ".%s\n", FsmToken2String(KeyFSM_NAME));
  fflush(fp);

  /*
   *  .i
   */
  if (Fsm_MgrReadNI(fsmMgr) > 0) {
    fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_I));
    for (i = 0; i < Fsm_MgrReadNI(fsmMgr); i++) {
      Ddi_Var_t *vp = Ddi_VararrayRead(fsmMgr->var.i, i);
      fprintf(fp, "%s ", Ddi_VarName(vp));
    }
    fprintf(fp, "\n");
  }
  fflush(fp);

  /*
   *  .o
   */

  if (Fsm_MgrReadNO(fsmMgr) > 0) {
    fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_O));
    for (i = 0; i < Fsm_MgrReadNO(fsmMgr); i++) {
      fprintf(fp, "out_%d ", i);
    }
    fprintf(fp, "\n");
  }
  fflush(fp);

  /*
   *  .ps
   */

  if (Fsm_MgrReadNL(fsmMgr) > 0) {
    fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_PS));
    for (i = 0; i < Fsm_MgrReadNL(fsmMgr); i++) {
      Ddi_Var_t *vp = Ddi_VararrayRead(fsmMgr->var.ps, i);
      fprintf(fp, "%s ", Ddi_VarName(vp));
    }
    fprintf(fp, "\n");
  }
  fflush(fp);

  /*
   *  .ns
   */

  if (Fsm_MgrReadNL(fsmMgr) > 0) {
    fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_NS));
    for (i = 0; i < Fsm_MgrReadNL(fsmMgr); i++) {
      Ddi_Var_t *vp = Ddi_VararrayRead(fsmMgr->var.ns, i);
      fprintf(fp, "%s ", Ddi_VarName(vp));
    }
    fprintf(fp, "\n");
  }
  fflush(fp);

  /*
   *  .cutPoint
   */

  if (Fsm_MgrReadNAuxVar(fsmMgr) > 0) {
    fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_AUXVAR));
    for (i = 0; i < Fsm_MgrReadNAuxVar(fsmMgr); i++) {
      Ddi_Var_t *vp = Ddi_VararrayRead(fsmMgr->var.auxVar, i);
      fprintf(fp, "%s ", Ddi_VarName(vp));
    }
    fprintf(fp, "\n");
  }
  fflush(fp);

  /* Closing Section */
  fprintf(fp, ".%s%s\n", FsmToken2String(KeyFSM_END),
    FsmToken2String(KeyFSM_NAME));
  fprintf(fp, "\n");
  fflush(fp);

  /*------------------------------ Store Index ------------------------------*/

  /* Opening Section */
  fprintf(fp, ".%s\n", FsmToken2String(KeyFSM_INDEX));

  /*
   *  .i
   */

  if (Fsm_MgrReadNI(fsmMgr) > 0) {
    fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_I));
    for (i = 0; i < Fsm_MgrReadNI(fsmMgr); i++) {
      Ddi_Var_t *var = Ddi_VararrayRead(fsmMgr->var.i, i);
      fprintf(fp, "%d ", Ddi_VarAuxid(var));
    }
    fprintf(fp, "\n");
  }
  fflush(fp);

#if 0 
  // nxr!!!
  /*
   *  .o
   */
  if ((Fsm_MgrReadIndexO(fsmMgr)) != NULL) {
    fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_O));
    for (i = 0; i < Fsm_MgrReadNO(fsmMgr); i++) {
      v = Fsm_MgrReadIndexO(fsmMgr)[i];
      fprintf(fp, "%d ", v);
    }
    fprintf(fp, "\n");
  }
  fflush(fp);
#endif

  /*
   *  .ps
   */

  if (Fsm_MgrReadNL(fsmMgr) > 0) {
    fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_PS));
    for (i = 0; i < Fsm_MgrReadNL(fsmMgr); i++) {
      Ddi_Var_t *var = Ddi_VararrayRead(fsmMgr->var.ps, i);
      fprintf(fp, "%d ", Ddi_VarAuxid(var));
    }
    fprintf(fp, "\n");
  }
  fflush(fp);

  /*
   *  .ns
   */

  if (Fsm_MgrReadNL(fsmMgr) > 0) {
    fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_NS));
    for (i = 0; i < Fsm_MgrReadNL(fsmMgr); i++) {
      Ddi_Var_t *var = Ddi_VararrayRead(fsmMgr->var.ns, i);
      fprintf(fp, "%d ", Ddi_VarAuxid(var));
    }
    fprintf(fp, "\n");
  }
  fflush(fp);

  /*
   *  .cutPoint
   */

  if (Fsm_MgrReadNAuxVar(fsmMgr) > 0) {
    fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_AUXVAR));
    for (i = 0; i < Fsm_MgrReadNAuxVar(fsmMgr); i++) {
      Ddi_Var_t *var = Ddi_VararrayRead(fsmMgr->var.auxVar, i);
      fprintf(fp, "%d ", Ddi_VarAuxid(var));
    }
    fprintf(fp, "\n");
  }
  fflush(fp);

  /* Closing Section */
  fprintf(fp, ".%s%s\n", FsmToken2String(KeyFSM_END),
    FsmToken2String(KeyFSM_INDEX));
  fprintf(fp, "\n");
  fflush(fp);

  /*---------------------------- Store Delta --------------------------------*/

  /*
   *  If (BDDs on File YES and BDDs EXISTS) ... go on
   */

  if (Fsm_MgrReadNL(fsmMgr) > 0 &&
    bddFlag != 0 && Fsm_MgrReadDeltaBDD(fsmMgr) != NULL) {
    fprintf(fp, ".%s\n", FsmToken2String(KeyFSM_DELTA));

    /* BDD on a Separate File */
    if (bddFlag == 1) {
      fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_FILE_BDD));
      tmpName = Pdtutil_FileName(fileName, "delta", "bdd", 1);
      fprintf(fp, "%s\n", tmpName);

      Ddi_BddarrayStore(Fsm_MgrReadDeltaBDD(fsmMgr), tmpName, 
        varnames, NULL, varauxids, bddFormat, tmpName, NULL);

      Fsm_MgrSetDeltaName(fsmMgr, tmpName);
    }

    /* BDD on the Same File */
    if (bddFlag == 2) {
      fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_FILE_TEXT));

      Ddi_BddarrayStore(Fsm_MgrReadDeltaBDD(fsmMgr), NULL, 
        varnames, NULL, varauxids, bddFormat, NULL, fp);

      fprintf(fp, "   .%s%s\n", FsmToken2String(KeyFSM_END),
        FsmToken2String(KeyFSM_FILE_TEXT));

      Fsm_MgrSetDeltaName(fsmMgr, fsmName);
    }

    fprintf(fp, ".%s%s\n", FsmToken2String(KeyFSM_END),
      FsmToken2String(KeyFSM_DELTA));

    fprintf(fp, "\n");
    fflush(fp);
  }

  /*--------------------------- Store Lambda --------------------------------*/

  /*
   *  If (BDDs on File YES and BDDs EXIST) ... go on
   */

  if (bddFlag != 0 && Fsm_MgrReadLambdaBDD(fsmMgr) != NULL) {
    fprintf(fp, ".%s\n", FsmToken2String(KeyFSM_LAMBDA));

    /* BDD on a Separate File */
    if (bddFlag == 1) {
      fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_FILE_BDD));
      tmpName = Pdtutil_FileName(fileName, "lambda", "bdd", 1);
      fprintf(fp, "%s\n", tmpName);

      Ddi_BddarrayStore(Fsm_MgrReadLambdaBDD(fsmMgr), tmpName,
        varnames, NULL, varauxids, bddFormat, tmpName, NULL);

      Fsm_MgrSetLambdaName(fsmMgr, tmpName);
    }

    /* BDD on the Same File */
    if (bddFlag == 2) {
      fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_FILE_TEXT));

      Ddi_BddarrayStore(Fsm_MgrReadLambdaBDD(fsmMgr), NULL, 
        varnames, NULL, varauxids, bddFormat, NULL, fp);

      fprintf(fp, "   .%s%s\n", FsmToken2String(KeyFSM_END),
        FsmToken2String(KeyFSM_FILE_TEXT));

      Fsm_MgrSetLambdaName(fsmMgr, fsmName);
    }

    fprintf(fp, ".%s%s\n", FsmToken2String(KeyFSM_END),
      FsmToken2String(KeyFSM_LAMBDA));

    fprintf(fp, "\n");
    fflush(fp);
  }

  /*--------------------------- Store CutPoint ------------------------------*/

  /*
   *  If (BDDs on File YES and BDDs EXISTS) ... go on
   */

  if (Fsm_MgrReadNAuxVar(fsmMgr) > 0 &&
    bddFlag != 0 && Fsm_MgrReadAuxVarBDD(fsmMgr) != NULL) {
    fprintf(fp, ".%s\n", FsmToken2String(KeyFSM_AUXFUN));

    /* BDD on a Separate File */
    if (bddFlag == 1) {
      fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_FILE_BDD));
      tmpName = Pdtutil_FileName(fileName, "cutPoint", "bdd", 1);
      fprintf(fp, "%s\n", tmpName);

      /*??? */
      Ddi_BddarrayStore(Fsm_MgrReadAuxVarBDD(fsmMgr), tmpName, 
        varnames, NULL, varauxids, bddFormat, tmpName, NULL);

      Fsm_MgrSetAuxVarName(fsmMgr, tmpName);
    }

    /* BDD on the Same File */
    if (bddFlag == 2) {
      fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_FILE_TEXT));

      /*??? */
      Ddi_BddarrayStore(Fsm_MgrReadAuxVarBDD(fsmMgr), NULL, 
        varnames, NULL, varauxids, bddFormat, NULL, fp);

      fprintf(fp, "   .%s%s\n", FsmToken2String(KeyFSM_END),
        FsmToken2String(KeyFSM_FILE_TEXT));

      Fsm_MgrSetAuxVarName(fsmMgr, fsmName);
    }

    fprintf(fp, ".%s%s\n", FsmToken2String(KeyFSM_END),
      FsmToken2String(KeyFSM_AUXFUN));

    fprintf(fp, "\n");
    fflush(fp);
  }

  /*-------------------------- Store InitState ------------------------------*/

  /*
   *  If there is the string ... store it
   */

  if (0 && Fsm_MgrReadInitString(fsmMgr) != NULL) {
    fprintf(fp, ".%s\n", FsmToken2String(KeyFSM_INITSTATE));
    fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_STRING));
    fprintf(fp, "%s\n", Fsm_MgrReadInitString(fsmMgr));
    fprintf(fp, ".%s%s\n", FsmToken2String(KeyFSM_END),
      FsmToken2String(KeyFSM_INITSTATE));
    fprintf(fp, "\n");
    fflush(fp);
  }

  else
    /*
     *  If (BDDs on File YES and BDDs EXIST) ... go on
     */

  if (bddFlag != 0 && Fsm_MgrReadInitBDD(fsmMgr) != NULL) {
    fprintf(fp, ".%s\n", FsmToken2String(KeyFSM_INITSTATE));
    fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_FILE_BDD));

    /* BDD on a Separate File */
    if (bddFlag == 1) {
      Ddi_BddSetMono(Fsm_MgrReadInitBDD(fsmMgr));
      tmpName = Pdtutil_FileName(fileName, "s0", "bdd", 1);
      fprintf(fp, "%s\n", tmpName);

      Ddi_BddStore(Fsm_MgrReadInitBDD(fsmMgr), NULL, bddFormat, tmpName, NULL);

      Fsm_MgrSetInitName(fsmMgr, tmpName);
    }

    /* BDD on the Same File */
    if (bddFlag == 2) {
      fprintf(stderr, "fsmLoad.c (fsmLoad): Not Yet Implemented.\n");
      Fsm_MgrSetInitName(fsmMgr, fsmName);
    }

    fprintf(fp, ".%s%s\n", FsmToken2String(KeyFSM_END),
      FsmToken2String(KeyFSM_INITSTATE));
    fprintf(fp, "\n");
    fflush(fp);
  }

  /*------------------------ Store Transition Relation ----------------------*/

  /*
   *  If (BDD on File YES and BDD EXISTS) ... go on
   */

  if (bddFlag != 0 && Fsm_MgrReadTrBDD(fsmMgr) != NULL) {
    fprintf(fp, ".%s\n", FsmToken2String(KeyFSM_TRANS_REL));

    /* BDD on a Separate File */
    if (bddFlag == 1) {
      fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_FILE_BDD));
      tmpName = Pdtutil_FileName(fileName, "TR", "bdd", 1);
      fprintf(fp, "%s\n", tmpName);

      Ddi_BddStore(Fsm_MgrReadTrBDD(fsmMgr), tmpName, bddFormat,
        tmpName, NULL);

      Fsm_MgrSetTrName(fsmMgr, tmpName);
    }

    /* BDD on the Same File */
    if (bddFlag == 2) {
      fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_FILE_TEXT));

      Ddi_BddStore(Fsm_MgrReadTrBDD(fsmMgr), NULL, bddFormat, NULL, fp);

      fprintf(fp, "   .%s%s\n", FsmToken2String(KeyFSM_END),
        FsmToken2String(KeyFSM_FILE_TEXT));

      Fsm_MgrSetTrName(fsmMgr, fsmName);
    }

    fprintf(fp, ".%s%s\n", FsmToken2String(KeyFSM_END),
      FsmToken2String(KeyFSM_TRANS_REL));

    fprintf(fp, "\n");
    fflush(fp);
  }

  /*------------------------------ Store From Set ---------------------------*/

  /*
   *  If (BDD on File YES and BDD EXISTS) ... go on
   */

  if (bddFlag != 0 && Fsm_MgrReadFromBDD(fsmMgr) != NULL) {
    fprintf(fp, ".%s\n", FsmToken2String(KeyFSM_FROM));

    /* BDD on a Separate File */
    if (bddFlag == 1) {
      Ddi_BddSetMono(Fsm_MgrReadFromBDD(fsmMgr));
      fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_FILE_BDD));
      tmpName = Pdtutil_FileName(fileName, "From", "bdd", 1);
      fprintf(fp, "%s\n", tmpName);

      Ddi_BddStore(Fsm_MgrReadFromBDD(fsmMgr), tmpName, bddFormat,
        tmpName, NULL);

      Fsm_MgrSetFromName(fsmMgr, tmpName);
    }

    /* BDD on the Same File */
    if (bddFlag == 2) {
      fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_FILE_TEXT));

      Ddi_BddStore(Fsm_MgrReadFromBDD(fsmMgr), NULL, bddFormat, NULL, fp);

      fprintf(fp, "   .%s%s\n", FsmToken2String(KeyFSM_END),
        FsmToken2String(KeyFSM_FILE_TEXT));

      Fsm_MgrSetFromName(fsmMgr, fsmName);
    }

    fprintf(fp, ".%s%s\n", FsmToken2String(KeyFSM_END),
      FsmToken2String(KeyFSM_FROM));

    fprintf(fp, "\n");
    fflush(fp);
  }

  /*--------------------------- Store Reached Set ---------------------------*/

  /*
   *  If (BDD on File YES and BDD EXISTS) ... go on
   */

  if (bddFlag != 0 && Fsm_MgrReadReachedBDD(fsmMgr) != NULL) {
    fprintf(fp, ".%s\n", FsmToken2String(KeyFSM_REACHED));

    /* BDD on a Separate File */
    if (bddFlag == 1) {
      Ddi_BddSetMono(Fsm_MgrReadReachedBDD(fsmMgr));
      fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_FILE_BDD));
      tmpName = Pdtutil_FileName(fileName, "Reached", "bdd", 1);
      fprintf(fp, "%s\n", tmpName);

      Ddi_BddStore(Fsm_MgrReadReachedBDD(fsmMgr), tmpName,
        bddFormat, tmpName, NULL);

      Fsm_MgrSetReachedName(fsmMgr, tmpName);
    }

    /* BDD on the Same File */
    if (bddFlag == 2) {
      fprintf(fp, "   .%s ", FsmToken2String(KeyFSM_FILE_TEXT));

      Ddi_BddStore(Fsm_MgrReadReachedBDD(fsmMgr), NULL, bddFormat, NULL, fp);

      fprintf(fp, "   .%s%s\n", FsmToken2String(KeyFSM_END),
        FsmToken2String(KeyFSM_FILE_TEXT));

      Fsm_MgrSetReachedName(fsmMgr, fsmName);
    }

    fprintf(fp, ".%s%s\n", FsmToken2String(KeyFSM_END),
      FsmToken2String(KeyFSM_REACHED));

    fprintf(fp, "\n");
    fflush(fp);
  }

  /*----------------------------- Close FSM ---------------------------------*/

  fprintf(fp, ".%s%s\n", FsmToken2String(KeyFSM_END),
    FsmToken2String(KeyFSM_FSM));
  fflush(fp);
  Pdtutil_FileClose(fp, &flagFile);

  /*
   *  return 0 = All OK
   */

  fsmTime = util_cpu_time() - fsmTime;
  fsmMgr->fsmTime += fsmTime;

  return (0);
}

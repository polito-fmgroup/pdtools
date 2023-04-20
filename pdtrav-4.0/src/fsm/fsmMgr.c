/**CFile***********************************************************************

  FileName    [fsmMgr.c]

  PackageName [fsm]

  Synopsis    [Utility functions to create, free and duplicate a FSM]

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
    Send bug-reports and/or questions to: {cabodi,quer}@polito.it. ]

  Revision    []

******************************************************************************/

#include "fsmInt.h"
#include "ddiInt.h"

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

static Fsm_Mgr_t *MgrDupIntern(
  Fsm_Mgr_t * fsmMgr,
  int newDdMgr,
  int doAlign
);

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis     [Initializes the FSM structure.]

  Description  [Notice: Direct access to fileds is used, because macros
    check for NULL fields to discover memory leaks.]

  SideEffects  [none]

******************************************************************************/

Fsm_Mgr_t *
Fsm_MgrInit(
  char *fsmName /* Name of the FSM structure */ ,
  Ddi_Mgr_t * ddm
)
{
  Fsm_Mgr_t *fsmMgr;

  /*------------------------- FSM Structure Creation -----------------------*/

  fsmMgr = Pdtutil_Alloc(Fsm_Mgr_t, 1);
  if (fsmMgr == NULL) {
    fprintf(stderr, "fsmUtil.c (Fsm_MgrInit) Error: Out of memory.\n");
    return (NULL);
  }

  /*------------------- Set the Name of the FSM Structure ------------------*/

  if (fsmName == NULL) {
    fsmMgr->fsmName = "fsmMgr";
  }
  fsmMgr->fsmName = Pdtutil_StrDup(fsmName);

  /*------------------------------- Set Fields -----------------------------*/

  fsmMgr->fileName = NULL;
  fsmMgr->dd = ddm;

  fsmMgr->var.i = NULL;
  fsmMgr->var.o = NULL;
  fsmMgr->var.ps = NULL;
  fsmMgr->var.ns = NULL;
  fsmMgr->var.auxVar = NULL;

  Fsm_MgrSetBddFormat(fsmMgr, DDDMP_MODE_TEXT);

  fsmMgr->initStub.name = NULL;
  fsmMgr->initStub.bdd = NULL;

  fsmMgr->delta.name = NULL;
  fsmMgr->delta.bdd = NULL;
  fsmMgr->lambda.name = NULL;
  fsmMgr->lambda.bdd = NULL;
  fsmMgr->auxVar.name = NULL;
  fsmMgr->auxVar.bdd = NULL;

  fsmMgr->init.name = NULL;
  fsmMgr->init.string = NULL;
  fsmMgr->init.bdd = NULL;
  fsmMgr->tr.name = NULL;
  fsmMgr->tr.string = NULL;
  fsmMgr->tr.bdd = NULL;
  fsmMgr->from.name = NULL;
  fsmMgr->from.string = NULL;
  fsmMgr->from.bdd = NULL;
  fsmMgr->reached.name = NULL;
  fsmMgr->reached.string = NULL;
  fsmMgr->reached.bdd = NULL;
  fsmMgr->constraint.name = NULL;
  fsmMgr->constraint.string = NULL;
  fsmMgr->constraint.bdd = NULL;
  fsmMgr->justice.name = NULL;
  fsmMgr->justice.string = NULL;
  fsmMgr->justice.bdd = NULL;
  fsmMgr->fairness.name = NULL;
  fsmMgr->fairness.string = NULL;
  fsmMgr->fairness.bdd = NULL;
  fsmMgr->cex.name = NULL;
  fsmMgr->cex.string = NULL;
  fsmMgr->cex.bdd = NULL;
  fsmMgr->initStubConstraint.name = NULL;
  fsmMgr->initStubConstraint.string = NULL;
  fsmMgr->initStubConstraint.bdd = NULL;
  fsmMgr->care.name = NULL;
  fsmMgr->care.string = NULL;
  fsmMgr->care.bdd = NULL;
  fsmMgr->invarspec.name = NULL;
  fsmMgr->invarspec.string = NULL;
  fsmMgr->invarspec.bdd = NULL;
  fsmMgr->retimedPis = NULL;
  fsmMgr->initStubPiConstr = NULL;
  fsmMgr->pdtSpecVar = NULL;
  fsmMgr->pdtConstrVar = NULL;
  fsmMgr->fsmList = NULL;
  fsmMgr->pdtInitStubVar = NULL;
  fsmMgr->fsmTime = 0;

  fsmMgr->iFoldedProp = -1;
  fsmMgr->iFoldedConstr = -1;
  fsmMgr->iFoldedInit = -1;

  /*------------------------- Default Settings -----------------------------*/

  fsmMgr->settings.verbosity = Pdtutil_VerbLevelUsrMax_c;
  fsmMgr->settings.cutThresh = -1;
  fsmMgr->settings.cutAtAuxVar = -1;
  fsmMgr->settings.useAig = 0;
  fsmMgr->settings.cegarStrategy = 1;

  fsmMgr->stats.targetEnSteps = 0;
  fsmMgr->stats.initStubSteps = 0;
  fsmMgr->stats.initStubStepsAtPhaseAbstr = 0;
  fsmMgr->stats.retimedConstr = 0;
  fsmMgr->stats.removedPis = 0;
  fsmMgr->stats.phaseAbstr = 0;
  fsmMgr->stats.externalConstr = 0;
  fsmMgr->stats.xInitLatches = 0;

  /*------------------------- Printing Message -----------------------------*/

  Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "-- FSM Manager Init.\n")
    );

  /*-----------------  Create Internal State Variables  --------------------*/

  if (ddm != NULL) {
    Ddi_Var_t *psVar, *nsVar;

    psVar = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
    if (psVar == NULL) {
      psVar = Ddi_VarNewAtLevel(ddm, 0);
      Ddi_VarAttachName(psVar, "PDT_BDD_INVARSPEC_VAR$PS");
    }
    nsVar = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$NS");
    if (nsVar == NULL) {
      int xlevel = Ddi_VarCurrPos(psVar) + 1;

      nsVar = Ddi_VarNewAtLevel(ddm, xlevel);
      Ddi_VarAttachName(nsVar, "PDT_BDD_INVARSPEC_VAR$NS");
    }
    fsmMgr->pdtSpecVar = Ddi_VarCopy(ddm, psVar);
    //Fsm_MgrSetPdtSpecVar(fsmMgr, psVar);

    psVar = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
    if (psVar == NULL) {
      psVar = Ddi_VarNewAtLevel(ddm, 0);
      Ddi_VarAttachName(psVar, "PDT_BDD_INVAR_VAR$PS");
    }
    nsVar = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$NS");
    if (nsVar == NULL) {
      int xlevel = Ddi_VarCurrPos(psVar) + 1;

      nsVar = Ddi_VarNewAtLevel(ddm, xlevel);
      Ddi_VarAttachName(nsVar, "PDT_BDD_INVAR_VAR$NS");
    }
    fsmMgr->pdtConstrVar = Ddi_VarCopy(ddm, psVar);
    //Fsm_MgrSetPdtConstrVar(fsmMgr, psVar);

    psVar = Ddi_VarFromName(ddm, "PDT_BDD_INITSTUB_VAR$PS");
    if (psVar == NULL) {
      psVar = Ddi_VarNewAtLevel(ddm, 0);
      Ddi_VarAttachName(psVar, "PDT_BDD_INITSTUB_VAR$PS");
    }
    nsVar = Ddi_VarFromName(ddm, "PDT_BDD_INITSTUB_VAR$NS");
    if (nsVar == NULL) {
      int xlevel = Ddi_VarCurrPos(psVar) + 1;

      nsVar = Ddi_VarNewAtLevel(ddm, xlevel);
      Ddi_VarAttachName(nsVar, "PDT_BDD_INITSTUB_VAR$NS");
    }
    fsmMgr->pdtInitStubVar = Ddi_VarCopy(ddm, psVar);
    //Fsm_MgrSetPdtInitStubVar(fsmMgr, psVar);
  }

  return (fsmMgr);
}

/**Function********************************************************************

  Synopsis     [Check the Congruence of a FSM structure.]

  Description  [Check the Congruence of a FSM structure.
    Return 1 if the FSM is a NULL pointer or it is not correct.
    Return 0 otherwise.]

  SideEffects  [none]

******************************************************************************/

int
Fsm_MgrCheck(
  Fsm_Mgr_t * fsmMgr            /* FSM structure */
)
{
  int errorFlag;

  errorFlag = 0;

  if (fsmMgr == NULL) {
    errorFlag = 1;
  }

  if (fsmMgr->dd == NULL) {
    errorFlag = 1;
  }

  /*
     fsmMgr->var.i = NULL;
     fsmMgr->var.o = NULL;
     fsmMgr->var.ps = NULL;
     fsmMgr->var.ns = NULL;
     fsmMgr->var.auxVar = NULL;

     Fsm_MgrSetBddFormat (fsmMgr, DDDMP_MODE_TEXT);

     fsmMgr->delta.name = NULL;
     fsmMgr->delta.bdd = NULL;
     fsmMgr->lambda.name = NULL;
     fsmMgr->lambda.bdd = NULL;
     fsmMgr->auxVar.name = NULL;
     fsmMgr->auxVar.bdd = NULL;

     fsmMgr->init.name = NULL;
     fsmMgr->init.string = NULL;
     fsmMgr->init.bdd = NULL;
     fsmMgr->tr.name = NULL;
     fsmMgr->tr.string = NULL;
     fsmMgr->tr.bdd = NULL;
     fsmMgr->from.name = NULL;
     fsmMgr->from.string = NULL;
     fsmMgr->from.bdd = NULL;
     fsmMgr->reached.name = NULL;
     fsmMgr->reached.string = NULL;
     fsmMgr->reached.bdd = NULL;
   */

  if (errorFlag == 1) {
    Pdtutil_Warning(1, "FSM Structure NOT Congruent.");
    return (1);
  } else {
    return (0);
  }
}

/**Function********************************************************************

  Synopsis     [Frees unused FSM structure members ]

  Description  []

  SideEffects  [none]

  SeeAlso      [Ddi_MgrQuit, Tr_MgrQuit, Trav_MgrQuit]

******************************************************************************/

void
Fsm_MgrQuit(
  Fsm_Mgr_t * fsmMgr            /* FSM Manager */
)
{
  Pdtutil_VerbosityMgr(fsmMgr,
    Pdtutil_VerbLevelUsrMax_c, fprintf(stdout, "-- FSM Manager Quit.\n")
    );

  /*------------------------ Deletes the FSM Manager -------------------*/

  /* Free fsmName */
  Pdtutil_Free(fsmMgr->fsmName);

  /* Free fileName */
  Pdtutil_Free(fsmMgr->fileName);

  /* var */
  Ddi_Free(fsmMgr->var.i);
  Ddi_Free(fsmMgr->var.ps);
  Ddi_Free(fsmMgr->var.ns);
  Ddi_Free(fsmMgr->var.auxVar);

  /* InitStub */
  Pdtutil_Free(fsmMgr->initStub.name);
  Ddi_Free(fsmMgr->initStub.bdd);

  /* Delta */
  Pdtutil_Free(fsmMgr->delta.name);
  Ddi_Free(fsmMgr->delta.bdd);

  /* Lambda */
  Pdtutil_Free(fsmMgr->lambda.name);
  Ddi_Free(fsmMgr->lambda.bdd);

  /* Auxiliary Variable  */
  Pdtutil_Free(fsmMgr->auxVar.name);
  Ddi_Free(fsmMgr->auxVar.bdd);

  /* init */
  Pdtutil_Free(fsmMgr->init.name);
  Ddi_Free(fsmMgr->init.bdd);

  /* tr */
  Pdtutil_Free(fsmMgr->tr.name);
  Ddi_Free(fsmMgr->tr.bdd);

  /* from */
  Pdtutil_Free(fsmMgr->from.name);
  Ddi_Free(fsmMgr->from.bdd);

  /* reached */
  Pdtutil_Free(fsmMgr->reached.name);
  Ddi_Free(fsmMgr->reached.bdd);

  /* constraint */
  Pdtutil_Free(fsmMgr->constraint.name);
  Ddi_Free(fsmMgr->constraint.bdd);

  /* justice */
  Pdtutil_Free(fsmMgr->justice.name);
  Ddi_Free(fsmMgr->justice.bdd);

  /* fairness */
  Pdtutil_Free(fsmMgr->fairness.name);
  Ddi_Free(fsmMgr->fairness.bdd);

  /* cex */
  Pdtutil_Free(fsmMgr->cex.name);
  Ddi_Free(fsmMgr->cex.bdd);

  /* init stub constraint */
  Pdtutil_Free(fsmMgr->initStubConstraint.name);
  Ddi_Free(fsmMgr->initStubConstraint.bdd);

  /* care */
  Pdtutil_Free(fsmMgr->care.name);
  Ddi_Free(fsmMgr->care.bdd);

  /* invarspec */
  Pdtutil_Free(fsmMgr->invarspec.name);
  Ddi_Free(fsmMgr->invarspec.bdd);

  Ddi_Free(fsmMgr->retimedPis);
  Ddi_Free(fsmMgr->initStubPiConstr);
  /* vars */
  /* done outside
     Ddi_Unlock(fsmMgr->pdtSpecVar);
     Ddi_Unlock(fsmMgr->pdtConstrVar);
     Ddi_Unlock(fsmMgr->pdtInitStubVar);
     Ddi_Free (fsmMgr->pdtSpecVar);
     Ddi_Free (fsmMgr->pdtConstrVar);
     Ddi_Free (fsmMgr->pdtInitStubVar);
   */

  /* list of FSMs */
  Fsm_Fsm_t *tmp = fsmMgr->fsmList, *next = NULL;

  while (tmp != NULL) {
    next = tmp->next;
    Fsm_FsmFree(tmp);
    tmp = next;
  }

  /* structure */
  Pdtutil_Free(fsmMgr);

  return;
}

/**Function********************************************************************

  Synopsis    [Duplicates FSM structure.]

  Description [The new FSM is allocated and all the field are duplicated
    from the original FSM.]

  SideEffects [none]

  SeeAlso     [Fsm_MgrInit ()]

******************************************************************************/

Fsm_Mgr_t *
Fsm_MgrDup(
  Fsm_Mgr_t * fsmMgr            /* FSM Manager */
)
{
  return MgrDupIntern(fsmMgr, 0, 0);
}

/**Function********************************************************************

  Synopsis    [Duplicates FSM structure.]

  Description [The new FSM is allocated and all the field are duplicated
    from the original FSM.]

  SideEffects [none]

  SeeAlso     [Fsm_MgrInit ()]

******************************************************************************/

Fsm_Mgr_t *
Fsm_MgrDupWithNewDdiMgr(
  Fsm_Mgr_t * fsmMgr            /* FSM Manager */
)
{
  return MgrDupIntern(fsmMgr, 1, 0);
}

/**Function********************************************************************

  Synopsis    [Duplicates FSM structure.]

  Description [The new FSM is allocated and all the field are duplicated
    from the original FSM.]

  SideEffects [none]

  SeeAlso     [Fsm_MgrInit ()]

******************************************************************************/

Fsm_Mgr_t *
Fsm_MgrDupWithNewDdiMgrAligned(
  Fsm_Mgr_t * fsmMgr            /* FSM Manager */
)
{
  return MgrDupIntern(fsmMgr, 1, 1);
}

/**Function********************************************************************

  Synopsis    [Duplicates FSM structure.]

  Description [The new FSM is allocated and all the field are duplicated
    from the original FSM.]

  SideEffects [none]

  SeeAlso     [Fsm_MgrInit ()]

******************************************************************************/

static Fsm_Mgr_t *
MgrDupIntern(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  int newDdMgr,
  int doAlign
)
{
  Fsm_Mgr_t *fsmMgrNew;
  Ddi_Mgr_t *ddm0, *ddm;
  int siftResume = 0;

  /*------------------------- FSM structure creation -----------------------*/

  fsmMgrNew = Fsm_MgrInit(NULL, NULL);

  /*------------------------------- Copy Fields ----------------------------*/

  /*
   *  General Fields
   */

  ddm0 = ddm = Fsm_MgrReadDdManager(fsmMgr);
  if (newDdMgr) {
    ddm = Ddi_MgrDup(ddm0);
    if (doAlign) {
      Ddi_MgrAlign(ddm, ddm0);
    }
    else {
      Ddi_MgrSiftSuspend(ddm);
      siftResume = 1;
    }
  }

  Fsm_MgrSetDdManager(fsmMgrNew, ddm);

  if (Fsm_MgrReadFileName(fsmMgr) != NULL) {
    Fsm_MgrSetFileName(fsmMgrNew, Fsm_MgrReadFileName(fsmMgr));
  }

  Fsm_MgrSetBddFormat(fsmMgrNew, Fsm_MgrReadBddFormat(fsmMgr));

  /*
   *  Variables ...
   */

  if (Fsm_MgrReadVarI(fsmMgr) != NULL) {
    Fsm_MgrSetVarI(fsmMgrNew, Fsm_MgrReadVarI(fsmMgr));
  }
  if (Fsm_MgrReadVarPS(fsmMgr) != NULL) {
    Fsm_MgrSetVarPS(fsmMgrNew, Fsm_MgrReadVarPS(fsmMgr));
  }
  if (Fsm_MgrReadVarNS(fsmMgr) != NULL) {
    Fsm_MgrSetVarNS(fsmMgrNew, Fsm_MgrReadVarNS(fsmMgr));
  }
  if (Fsm_MgrReadVarAuxVar(fsmMgr) != NULL) {
    Fsm_MgrSetVarAuxVar(fsmMgrNew, Fsm_MgrReadVarAuxVar(fsmMgr));
  }
  if (Fsm_MgrReadVarO(fsmMgr) != NULL) {
    Fsm_MgrSetVarO(fsmMgrNew, Fsm_MgrReadVarO(fsmMgr));
  }

  /*
   *  BDDs
   */

  if (Fsm_MgrReadInitBDD(fsmMgr) != NULL) {
    Fsm_MgrSetInitBDD(fsmMgrNew, Fsm_MgrReadInitBDD(fsmMgr));
  }
  if (Fsm_MgrReadInitStubBDD(fsmMgr) != NULL) {
    Fsm_MgrSetInitStubBDD(fsmMgrNew, Fsm_MgrReadInitStubBDD(fsmMgr));
  }
  if (Fsm_MgrReadDeltaBDD(fsmMgr) != NULL) {
    Fsm_MgrSetDeltaBDD(fsmMgrNew, Fsm_MgrReadDeltaBDD(fsmMgr));
  }
  if (Fsm_MgrReadLambdaBDD(fsmMgr) != NULL) {
    Fsm_MgrSetLambdaBDD(fsmMgrNew, Fsm_MgrReadLambdaBDD(fsmMgr));
  }
  if (Fsm_MgrReadAuxVarBDD(fsmMgr) != NULL) {
    Fsm_MgrSetAuxVarBDD(fsmMgrNew, Fsm_MgrReadAuxVarBDD(fsmMgr));
  }
  if (Fsm_MgrReadTrBDD(fsmMgr) != NULL) {
    Fsm_MgrSetTrBDD(fsmMgrNew, Fsm_MgrReadTrBDD(fsmMgr));
  }
  if (Fsm_MgrReadReachedBDD(fsmMgr) != NULL) {
    Fsm_MgrSetReachedBDD(fsmMgrNew, Fsm_MgrReadReachedBDD(fsmMgr));
  }
  if (Fsm_MgrReadConstraintBDD(fsmMgr) != NULL) {
    Fsm_MgrSetConstraintBDD(fsmMgrNew, Fsm_MgrReadConstraintBDD(fsmMgr));
  }
  if (Fsm_MgrReadJusticeBDD(fsmMgr) != NULL) {
    Fsm_MgrSetJusticeBDD(fsmMgrNew, Fsm_MgrReadJusticeBDD(fsmMgr));
  }
  if (Fsm_MgrReadFairnessBDD(fsmMgr) != NULL) {
    Fsm_MgrSetFairnessBDD(fsmMgrNew, Fsm_MgrReadFairnessBDD(fsmMgr));
  }
  if (Fsm_MgrReadCexBDD(fsmMgr) != NULL) {
    Fsm_MgrSetCexBDD(fsmMgrNew, Fsm_MgrReadCexBDD(fsmMgr));
  }
  if (Fsm_MgrReadInitStubConstraintBDD(fsmMgr) != NULL) {
    Fsm_MgrSetInitStubConstraintBDD(fsmMgrNew,
      Fsm_MgrReadInitStubConstraintBDD(fsmMgr));
  }
  if (Fsm_MgrReadCareBDD(fsmMgr) != NULL) {
    Fsm_MgrSetCareBDD(fsmMgrNew, Fsm_MgrReadCareBDD(fsmMgr));
  }
  if (Fsm_MgrReadInvarspecBDD(fsmMgr) != NULL) {
    Fsm_MgrSetInvarspecBDD(fsmMgrNew, Fsm_MgrReadInvarspecBDD(fsmMgr));
  }

  if (Fsm_MgrReadRetimedPis(fsmMgr) != NULL) {
    Fsm_MgrSetRetimedPis(fsmMgrNew, Fsm_MgrReadRetimedPis(fsmMgr));
  }

  if (Fsm_MgrReadInitStubPiConstr(fsmMgr) != NULL) {
    Fsm_MgrSetInitStubPiConstr(fsmMgrNew, Fsm_MgrReadInitStubPiConstr(fsmMgr));
  }

  if (Fsm_MgrReadPdtSpecVar(fsmMgr) != NULL) {
    Fsm_MgrSetPdtSpecVar(fsmMgrNew, Fsm_MgrReadPdtSpecVar(fsmMgr));
  }

  if (Fsm_MgrReadPdtConstrVar(fsmMgr) != NULL) {
    Fsm_MgrSetPdtConstrVar(fsmMgrNew, Fsm_MgrReadPdtConstrVar(fsmMgr));
  }

  if (Fsm_MgrReadPdtInitStubVar(fsmMgr) != NULL) {
    Fsm_MgrSetPdtInitStubVar(fsmMgrNew, Fsm_MgrReadPdtInitStubVar(fsmMgr));
  }

  /*
   *  BDD Names
   */

  if (Fsm_MgrReadInitStubName(fsmMgr) != NULL) {
    Fsm_MgrSetInitStubName(fsmMgrNew, Fsm_MgrReadInitStubName(fsmMgr));
  }
  if (Fsm_MgrReadDeltaName(fsmMgr) != NULL) {
    Fsm_MgrSetDeltaName(fsmMgrNew, Fsm_MgrReadDeltaName(fsmMgr));
  }
  if (Fsm_MgrReadLambdaName(fsmMgr) != NULL) {
    Fsm_MgrSetLambdaName(fsmMgrNew, Fsm_MgrReadLambdaName(fsmMgr));
  }
  if (Fsm_MgrReadAuxVarName(fsmMgr) != NULL) {
    Fsm_MgrSetAuxVarName(fsmMgrNew, Fsm_MgrReadAuxVarName(fsmMgr));
  }
  if (Fsm_MgrReadTrName(fsmMgr) != NULL) {
    Fsm_MgrSetTrName(fsmMgrNew, Fsm_MgrReadTrName(fsmMgr));
  }
  if (Fsm_MgrReadInitString(fsmMgr) != NULL) {
    Fsm_MgrSetInitString(fsmMgrNew, Fsm_MgrReadInitString(fsmMgr));
  }
  if (Fsm_MgrReadInitName(fsmMgr) != NULL) {
    Fsm_MgrSetInitName(fsmMgrNew, Fsm_MgrReadInitName(fsmMgr));
  }

  fsmMgrNew->settings = fsmMgr->settings;
  fsmMgrNew->stats = fsmMgr->stats;

  fsmMgrNew->iFoldedProp = fsmMgr->iFoldedProp;
  fsmMgrNew->iFoldedConstr = fsmMgr->iFoldedConstr;
  fsmMgrNew->iFoldedInit = fsmMgr->iFoldedInit;

  fsmMgrNew->fsmList = NULL;

  if (siftResume) {
    Ddi_MgrSiftResume(ddm);
  }

  return (fsmMgrNew);
}

/**Function********************************************************************

  Synopsis    [Remove Auxiliary Variables from the FSM structure.]

  Description [The same FSM is returned in any case.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrAuxVarRemove(
  Fsm_Mgr_t * fsmMgr            /* FSM Manager */
)
{
  Ddi_Mgr_t *ddMgr;
  Ddi_Vararray_t *auxVararray;
  Ddi_Varset_t *auxVarset;
  Ddi_Bddarray_t *auxFunArray, *deltaArray;
  Ddi_Bdd_t *delta, *deltaNew, *auxRelation, *tmpRelation;
  long startTime, currTime;
  int i, psNumber, auxVarNumber;

  auxVarNumber = Fsm_MgrReadNAuxVar(fsmMgr);
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Auxiliary Variable Number: %d.\n", auxVarNumber)
    );

  if (auxVarNumber != 0) {

    /*------------------------- Set What is Needed --------------------------*/

    startTime = util_cpu_time();
    ddMgr = Fsm_MgrReadDdManager(fsmMgr);
    psNumber = Fsm_MgrReadNL(fsmMgr);
    deltaArray = Fsm_MgrReadDeltaBDD(fsmMgr);
    auxVararray = Fsm_MgrReadVarAuxVar(fsmMgr);
    auxFunArray = Fsm_MgrReadAuxVarBDD(fsmMgr);
    auxVarset = Ddi_VarsetMakeFromArray(auxVararray);

    if (auxVararray == NULL || auxFunArray == NULL) {
      Pdtutil_Warning(1, "NULL Arrays of Auxiliary Variables.");
      return;
    }

    /*------------- Compose Next State with Auxiliary Variables -------------*/

    auxRelation = Ddi_BddRelMakeFromArray(auxFunArray, auxVararray);

    for (i = 0; i < psNumber; i++) {
      tmpRelation = Ddi_BddDup(auxRelation);
      delta = Ddi_BddarrayExtract(deltaArray, i);

      Ddi_BddPartInsert(tmpRelation, 0, delta);
      deltaNew = Part_BddMultiwayLinearAndExist(tmpRelation, auxVarset, -1);

      Ddi_Free(tmpRelation);
      Ddi_Free(delta);
      Ddi_BddarrayWrite(deltaArray, i, deltaNew);
    }

    /*------------------------------- Free ... ------------------------------*/

    Ddi_Free(auxRelation);
    Ddi_Free(auxFunArray);
    Ddi_Free(auxVararray);
    Ddi_Free(auxVarset);

    /*------------------------ Modify the FSM Manager -----------------------*/

    Fsm_MgrSetDeltaBDD(fsmMgr, deltaArray);
    Fsm_MgrSetVarAuxVar(fsmMgr, NULL);
    Fsm_MgrSetAuxVarBDD(fsmMgr, NULL);

    /*---------------------------- Print-Out Time ---------------------------*/

    currTime = util_cpu_time();
    fsmMgr->fsmTime += currTime - startTime;
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Composition Time: %s.\n",
        util_print_time(currTime - startTime))
      );
  }

  return;
}

/**Function********************************************************************

  Synopsis    [Remove Auxiliary Variables from the FSM structure.]

  Description [The same FSM is returned in any case.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrCoiReduction(
  Fsm_Mgr_t * fsmMgr            /* FSM Manager */
)
{
  Ddi_Mgr_t *ddMgr;
  Ddi_Vararray_t *auxVararray;
  Ddi_Varset_t *auxVarset;
  Ddi_Bddarray_t *auxFunArray, *deltaArray;
  Ddi_Bdd_t *delta, *deltaNew, *auxRelation, *tmpRelation;
  long startTime, currTime;
  int i, psNumber, auxVarNumber;

  auxVarNumber = Fsm_MgrReadNAuxVar(fsmMgr);
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Auxiliary Variable Number: %d.\n", auxVarNumber)
    );

  if (auxVarNumber != 0) {

    /*------------------------- Set What is Needed --------------------------*/

    startTime = util_cpu_time();
    ddMgr = Fsm_MgrReadDdManager(fsmMgr);
    psNumber = Fsm_MgrReadNL(fsmMgr);
    deltaArray = Fsm_MgrReadDeltaBDD(fsmMgr);
    auxVararray = Fsm_MgrReadVarAuxVar(fsmMgr);
    auxFunArray = Fsm_MgrReadAuxVarBDD(fsmMgr);
    auxVarset = Ddi_VarsetMakeFromArray(auxVararray);

    if (auxVararray == NULL || auxFunArray == NULL) {
      Pdtutil_Warning(1, "NULL Arrays of Auxiliary Variables.");
      return;
    }

    /*------------- Compose Next State with Auxiliary Variables -------------*/

    auxRelation = Ddi_BddRelMakeFromArray(auxFunArray, auxVararray);

    for (i = 0; i < psNumber; i++) {
      tmpRelation = Ddi_BddDup(auxRelation);
      delta = Ddi_BddarrayExtract(deltaArray, i);

      Ddi_BddPartInsert(tmpRelation, 0, delta);
      deltaNew = Part_BddMultiwayLinearAndExist(tmpRelation, auxVarset, -1);

      Ddi_Free(tmpRelation);
      Ddi_Free(delta);
      Ddi_BddarrayWrite(deltaArray, i, deltaNew);
    }

    /*------------------------------- Free ... ------------------------------*/

    Ddi_Free(auxRelation);
    Ddi_Free(auxFunArray);
    Ddi_Free(auxVararray);
    Ddi_Free(auxVarset);

    /*------------------------ Modify the FSM Manager -----------------------*/

    Fsm_MgrSetDeltaBDD(fsmMgr, deltaArray);
    Fsm_MgrSetVarAuxVar(fsmMgr, NULL);
    Fsm_MgrSetAuxVarBDD(fsmMgr, NULL);

    /*---------------------------- Print-Out Time ---------------------------*/

    currTime = util_cpu_time();
    fsmMgr->fsmTime += currTime - startTime;
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Composition Time: %s.\n",
        util_print_time(currTime - startTime))
      );
  }

  return;
}

/**Function********************************************************************

  Synopsis    [Remove Auxiliary Variables from the FSM structure.]

  Description [The same FSM is returned in any case.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Fsm_MgrAigToBdd(
  Fsm_Mgr_t * fsmMgr            /* FSM Manager */
)
{
  Ddi_Mgr_t *ddm = fsmMgr->dd;
  int i, i0, j, th1, th = Fsm_MgrReadCutThresh(fsmMgr);
  Ddi_Bddarray_t *auxF = Ddi_BddarrayAlloc(ddm, 0);
  Ddi_Vararray_t *auxV = Ddi_VararrayAlloc(ddm, 0);
  int auxVarNumber;
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
  Ddi_Bdd_t *invarspec = Fsm_MgrReadInvarspecBDD(fsmMgr);
  Ddi_Bdd_t *invar = Fsm_MgrReadConstraintBDD(fsmMgr);
  Ddi_Bdd_t *initStubInvar = Fsm_MgrReadInitStubConstraintBDD(fsmMgr);
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);

  Ddi_Bddarray_t *totBdd, *totAig = Ddi_BddarrayDup(lambda);
  int precomputeCuts = 0;
  int sortDfs = 1;
  int nL = Ddi_VararrayNum(ps);
  int ret = 1;
  int cutAtAuxVar = fsmMgr->settings.cutAtAuxVar;

  /* sort variables by DFS order */

#if 0
  for (i = 0; i < Ddi_VararrayNum(ps); i++) {
    int ii = Ddi_VarCuddIndex(Ddi_VararrayRead(ps, i));
    int jj = Ddi_VarCuddIndex(Ddi_VararrayRead(ns, i));
    int ll = Ddi_MgrReadPerm(ddm, ii);
    int kk = Ddi_MgrReadPerm(ddm, jj);

    printf("[%3d] %s - ps[%d]: %d - ns[%d]: %d\n", i,
      Ddi_VarName(Ddi_VararrayRead(ps, i)), ii, ll, jj, kk);
  }

  for (i = 0; i < Ddi_MgrReadNumCuddVars(ddm); i++) {
    int ii = Ddi_MgrReadInvPerm(ddm, i);
    Ddi_Var_t *v = Ddi_VarFromCuddIndex(ddm, ii);

    printf("sorted [%3d] %s - id: %d\n", i, Ddi_VarName(v), ii);
  }
#endif

  if (sortDfs) {
    Ddi_Vararray_t *sortedVars;
    Ddi_Bddarray_t *fA = Ddi_BddarrayDup(delta);
    int *sortedIds, nIds, i;

    Ddi_BddarrayAppend(fA, lambda);

    for (i = 0; i < Ddi_VararrayNum(ps); i++) {
      Ddi_Var_t *v = Ddi_VararrayRead(ps, i);

      Pdtutil_Assert(Ddi_VarReadMark(v) == 0, "0 var mark required");
      Ddi_VarWriteMark(v, i + 1);
    }

    sortedVars = Ddi_AigarrayDFSOrdVars(fA);
    nIds = Ddi_VararrayNum(sortedVars);
    sortedIds = Pdtutil_Alloc(int,
      nIds + nL
    );                          // add next state vars

    //    printf("DFS sorted VARS:\n");
    for (i = j = 0; i < nIds; i++) {
      Ddi_Var_t *v = Ddi_VararrayRead(sortedVars, i);
      int mark = Ddi_VarReadMark(v);
      int id = Ddi_VarIndex(v);

      //      printf("[%d] id: %d - name: %s\n", i, id, Ddi_VarName(v));
      sortedIds[j++] = id;
      if (mark > 0) {
        // state var. Add next state
        Ddi_Var_t *vNs = Ddi_VararrayRead(ns, mark - 1);
        int idNs = Ddi_VarIndex(vNs);

        sortedIds[j++] = idNs;
      }
    }
    Pdtutil_Assert(j == nIds + nL, "not all state vars found in DFS");

    Ddi_VararrayWriteMark(ps, 0);

    Ddi_MgrShuffle(ddm, sortedIds, nIds + nL);

    Ddi_Free(fA);
    Ddi_Free(sortedVars);
  }

  Ddi_MgrCreateGroups2(ddm, ps, ns);

#if 0
  for (i = 0; i < Ddi_VararrayNum(ps); i++) {
    int ii = Ddi_VarCuddIndex(Ddi_VararrayRead(ps, i));
    int jj = Ddi_VarCuddIndex(Ddi_VararrayRead(ns, i));
    int ll = Ddi_MgrReadPerm(ddm, ii);
    int kk = Ddi_MgrReadPerm(ddm, jj);

    printf("[%3d] %s - ps: %d - ns: %d\n", i, Ddi_VarName(Ddi_VararrayRead(ps,
          i)), ll, kk);
  }
#endif

  Ddi_BddarrayAppend(totAig, delta);

  th1 = (!precomputeCuts || th >= 0) ? th : 100;
  totBdd = Ddi_AigarrayOptByBdd(totAig, auxF, auxV, th1, 1, -1, -1);

  if (precomputeCuts && totBdd != NULL && th < 0 && Ddi_VararrayNum(auxV) > 0) {
    int again;
    Ddi_Varset_t *aux = Ddi_VarsetMakeFromArray(auxV);

    do {
      Ddi_Varset_t *supp;

      Ddi_BddarrayComposeAcc(auxF, auxV, auxF);
      Ddi_BddarrayComposeAcc(totBdd, auxV, auxF);
      supp = Ddi_BddarraySupp(totBdd);
      Ddi_VarsetIntersectAcc(supp, aux);
      again = !Ddi_VarsetIsVoid(supp);
      Ddi_Free(supp);
    } while (again);
    Ddi_Free(aux);
    Ddi_Free(auxV);
    Ddi_Free(auxF);
  }

  if (totBdd == NULL) {
    ret = 0;
  } else {
    Ddi_BddSetMono(init);
    if (invarspec != NULL)
      Ddi_BddSetMono(invarspec);
    if (invar != NULL) {
#if 0
      Ddi_Bdd_t *invarPart = Ddi_AigPartitionTop(invar, 0);
      int i;
      for (i=0; i<Ddi_BddPartNum(invarPart); i++) {
        Ddi_Bdd_t *p_i = Ddi_BddPartRead(invarPart,i); 
        Ddi_BddSetMono(p_i);
      }
      Ddi_DataCopy(invar,invarPart);
      Ddi_Free(invar);
#else
      Ddi_BddSetMono(invar);
#endif
    }
    Pdtutil_Assert(totBdd != NULL, "error converting from aig to BDDs");

    if (initStub != NULL) {
      Ddi_Bddarray_t *auxF0 = Ddi_BddarrayAlloc(ddm, 0);
      Ddi_Vararray_t *auxV0 = Ddi_VararrayAlloc(ddm, 0);
      int auxVarNumber;

      Ddi_Bddarray_t *stubBdd = Ddi_AigarrayOptByBdd(initStub,
        auxF0, auxV0, th1, 1, -1, -1);

      if (stubBdd != NULL) {
        Ddi_Bdd_t *newInit = Ddi_BddRelMakeFromArray(stubBdd, ps);
        Ddi_Bdd_t *auxRel = Ddi_BddRelMakeFromArray(auxF0, auxV0);
        Ddi_Varset_t *psv = Ddi_VarsetMakeFromArray(ps);

        Ddi_BddPartInsertLast(newInit, auxRel);
        Ddi_BddSetFlattened(newInit);
        //    Ddi_BddSetClustered(newInit,1000);
        //    Ddi_BddExistProjectAcc(newInit,psv);
        Fsm_MgrSetInitBDD(fsmMgr, newInit);

        Ddi_Free(psv);
        Ddi_Free(auxRel);
        Ddi_Free(newInit);
      } else {
        Ddi_Bdd_t *newInit = Ddi_BddRelMakeFromArray(initStub, ps);

        Ddi_BddSetAig(newInit);
        Fsm_MgrSetInitBDD(fsmMgr, newInit);
        Ddi_Free(newInit);
      }
      Ddi_Free(auxV0);
      Ddi_Free(auxF0);
      Ddi_Free(stubBdd);
    }
    if (1) {
      Ddi_Var_t *iv = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");

      if (iv != NULL) {
        init = Fsm_MgrReadInitBDD(fsmMgr);
#if 1
        Ddi_BddCofactorAcc(init, iv, 1);
#else
        Ddi_Bdd_t *invarOut = Ddi_BddMakeLiteral(iv, 0);

        Ddi_BddOrAcc(init, invarOut);
        Ddi_Free(invarOut);
#endif
      }
    }

    auxVarNumber = auxV == NULL ? 0 : Ddi_VararrayNum(auxV);
    if (auxVarNumber > 0 && (cutAtAuxVar>0)) {
      Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
      Ddi_VararrayAppend(pi,auxV);
    }
    else if (auxVarNumber > 0) {
      char **auxNameArray;

      Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelDevMin_c) {
        fprintf(stdout, "Cut-point variables (%d) generated.\n", auxVarNumber);
      }
      Fsm_MgrSetVarAuxVar(fsmMgr, auxV);
      Fsm_MgrSetAuxVarBDD(fsmMgr, auxF);

      auxNameArray = Pdtutil_Alloc(char *,
        auxVarNumber
      );

      for (i = 0; i < auxVarNumber; i++) {
        Ddi_Var_t *aV = Ddi_VararrayRead(auxV, i);
        int index = Ddi_VarIndex(aV);

        auxNameArray[i] = Pdtutil_StrDup(Ddi_VarName(aV));
      }

      //Fsm_MgrSetNameAuxVar(fsmMgr, auxNameArray);
      Pdtutil_StrArrayFree(auxNameArray, auxVarNumber);
    }

    for (i = 0; i < Ddi_BddarrayNum(lambda); i++) {
      Ddi_BddarrayWrite(lambda, i, Ddi_BddarrayRead(totBdd, i));
    }
    i0 = Ddi_BddarrayNum(lambda);
    for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
      Ddi_BddarrayWrite(delta, i, Ddi_BddarrayRead(totBdd, i0 + i));
    }

  }

  Ddi_Free(totBdd);
  Ddi_Free(totAig);

  Ddi_Free(auxF);
  Ddi_Free(auxV);

  return ret;
}

/**Function********************************************************************

  Synopsis    [Remove Auxiliary Variables from the FSM structure.]

  Description [The same FSM is returned in any case.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Ddi_Var_t *
Fsm_MgrSetClkStateVar(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  char *varname,
  int toggle
)
{

  Ddi_Varset_t *pi;
  Ddi_Vararray_t *va;
  Ddi_Var_t *v = NULL, *vy = NULL, *vs = NULL, *vnew = NULL;
  Ddi_Bdd_t *clkDelta;
  char buf[PDTUTIL_MAX_STR_LEN];
  Ddi_Mgr_t *dd = Fsm_MgrReadDdManager(fsmMgr);

  if ((varname == NULL) || (strcmp(varname, "NULL") == 0)) {
    Pdtutil_Warning(1, "Null clock variable.");
    return (NULL);
  }

  /* Get Split Variable */
  v = Ddi_VarFromName(dd, varname);

  if (v == NULL) {
    fprintf(stderr, "Variable %s not found; No clock detected.\n", varname);
    return (NULL);
  }

  pi = Ddi_VarsetMakeFromArray(Fsm_MgrReadVarI(fsmMgr));

  if (Ddi_VarInVarset(pi, v)) {
    int i;
    Ddi_Vararray_t *piArray = Fsm_MgrReadVarI(fsmMgr);

    for (i = 0; i < Ddi_VararrayNum(piArray); i++) {
      if (v == Ddi_VararrayRead(piArray, i)) {
        Ddi_VararrayRemove(piArray, i);
        break;
      }
    }
    /* Move from pi to ps and generate ns and pi vars */
    vs = v;
    vnew = Ddi_VarNewAfterVar(v);
    sprintf(buf, "%s$PI", varname);
    Ddi_VarAttachName(vnew, buf);
    Ddi_VararrayInsert(piArray, i, vnew);
    if (Fsm_MgrReadUseAig(fsmMgr)) {
      Ddi_Bdd_t *dummy = Ddi_BddMakeLiteralAig(v, 1);

      Ddi_Free(dummy);
    }

    vy = Ddi_VarNewAfterVar(v);
    sprintf(buf, "%s$NS", varname);
    Ddi_VarAttachName(vy, buf);

    /* Group Present and Next State Variable for Clock */
    Ddi_VarMakeGroup(dd, v, 3);

    /* Put Present/Next For Clock into Present/Next Variables */
    va = Fsm_MgrReadVarPS(fsmMgr);
    Ddi_VararrayWrite(va, Ddi_VararrayNum(va), vs);
    va = Fsm_MgrReadVarNS(fsmMgr);
    Ddi_VararrayWrite(va, Ddi_VararrayNum(va), vy);

  } else {
    toggle = 0;
  }

  /* If Toggle Force Clock Generator */
  if (toggle) {
    Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);

    clkDelta = Ddi_BddMakeLiteral(v, 0);
    if (Ddi_BddIsAig(Ddi_BddarrayRead(delta, 0))) {
      Ddi_BddSetAig(clkDelta);
    }
    Ddi_BddarrayInsertLast(delta, clkDelta);
    /* initial clock value = 1 */
    Ddi_BddNotAcc(clkDelta);
    Ddi_BddAndAcc(Fsm_MgrReadInitBDD(fsmMgr), clkDelta);
    Ddi_Free(clkDelta);
  } else if (vnew != NULL) {
    Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);

    clkDelta = Ddi_BddMakeLiteral(vnew, 1);
    if (Ddi_BddIsAig(Ddi_BddarrayRead(delta, 0))) {
      Ddi_BddSetAig(clkDelta);
    }
    Ddi_BddarrayInsertLast(delta, clkDelta);
  }

  Ddi_Free(pi);
  return (v);

}


/**Function********************************************************************

  Synopsis    [Remove Auxiliary Variables from the FSM structure.]

  Description [The same FSM is returned in any case.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrComposeCutVars(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  int threshold
)
{
  Ddi_Vararray_t *auxV;
  Ddi_Bddarray_t *delta, *lambda, *auxF;    //, *init;

  //Ddi_Mgr_t *dd = Fsm_MgrReadDdManager (fsmMgr);
  int again, nAux, *doCompose, i, j;

  if ((auxF = Fsm_MgrReadAuxVarBDD(fsmMgr)) == NULL) {
    return;
  }

  nAux = Ddi_BddarrayNum(auxF);
  doCompose = Pdtutil_Alloc(int, nAux);
  for (i = 0; i < nAux; i++) {
    doCompose[i] = 1;
  }

  do {
    int minSize = 0, minI = -1;
    Ddi_Bdd_t *currAuxF;
    Ddi_Var_t *currAuxV;

    again = 0;
    auxF = Fsm_MgrReadAuxVarBDD(fsmMgr);
    auxV = Fsm_MgrReadVarAuxVar(fsmMgr);
    delta = Fsm_MgrReadDeltaBDD(fsmMgr);
    lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
    nAux = Ddi_BddarrayNum(auxF);

    for (i = 0; i < nAux; i++) {
      currAuxF = Ddi_BddarrayRead(auxF, i);
      currAuxV = Ddi_VararrayRead(auxV, i);
      if (doCompose[i] && (minI < 0 || Ddi_BddSize(currAuxF) < minSize)) {
        minI = i;
        minSize = Ddi_BddSize(currAuxF);
      }
    }

    if (minI >= 0) {
      int abort = 0;
      Ddi_Bdd_t *auxRel;
      Ddi_Varset_t *sm;
      Ddi_Bddarray_t *newArrays[3];

      currAuxF = Ddi_BddarrayExtract(auxF, minI);
      currAuxV = Ddi_VararrayExtract(auxV, minI);
      auxRel = Ddi_BddMakeLiteral(currAuxV, 1);
      sm = Ddi_VarsetMakeFromVar(currAuxV);

      again = 1;
      Ddi_BddXnorAcc(auxRel, currAuxF);

      newArrays[0] = Ddi_BddarrayDup(auxF);
      newArrays[1] = Ddi_BddarrayDup(delta);
      newArrays[2] = Ddi_BddarrayDup(lambda);

      for (j = 0; j < 3; j++) {
        Ddi_Bddarray_t *F = newArrays[j];

        for (i = 0; !abort && i < Ddi_BddarrayNum(F); i++) {
          Ddi_BddAndExistAcc(Ddi_BddarrayRead(F, i), auxRel, sm);
          if (Ddi_BddSize(Ddi_BddarrayRead(F, i)) > threshold) {
            abort = 1;
          }
        }
      }

      if (abort) {
        doCompose[minI] = 0;
        Ddi_Free(newArrays[0]);
        Ddi_Free(newArrays[1]);
        Ddi_Free(newArrays[2]);
        Ddi_BddarrayInsert(auxF, minI, currAuxF);
        Ddi_VararrayInsert(auxV, minI, currAuxV);
      } else {
        Ddi_Free(fsmMgr->delta.bdd);
        Ddi_Free(fsmMgr->lambda.bdd);
        Ddi_Free(fsmMgr->auxVar.bdd);
        fsmMgr->auxVar.bdd = newArrays[0];
        fsmMgr->delta.bdd = newArrays[1];
        fsmMgr->lambda.bdd = newArrays[2];
        for (; minI < nAux - 1; minI++) {
          doCompose[minI] = doCompose[minI + 1];
        }
      }
      Ddi_Free(sm);
      Ddi_Free(currAuxF);
    }
  } while (again);

  Pdtutil_Free(doCompose);

}


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     [CmdMgrOperation, CmdRegOperation, Trav_MgrOperation,
    Tr_MgrOperation]

******************************************************************************/

int
Fsm_MgrOperation(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  char *string /* String */ ,
  Pdtutil_MgrOp_t operationFlag /* Operation Flag */ ,
  void **voidPointer /* Generic Pointer */ ,
  Pdtutil_MgrRet_t * returnFlagP    /* Type of the Pointer Returned */
)
{
  char *stringFirst, *stringSecond;
  Ddi_Mgr_t *defaultDdMgr, *tmpDdMgr;
  Ddi_Bdd_t *bdd;

  /*------------ Check for the Correctness of the Command Sequence ----------*/

  if (fsmMgr == NULL) {
    Pdtutil_Warning(1, "Command Out of Sequence.");
    return (1);
  }

  defaultDdMgr = Fsm_MgrReadDdManager(fsmMgr);

  /*----------------------- Take Main and Secondary Name --------------------*/

  Pdtutil_ReadSubstring(string, &stringFirst, &stringSecond);

  /*------------------------ Operation on the FSM Manager -------------------*/

  if (stringFirst == NULL) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "FSM Manager Verbosity %s (%d).\n",
            Pdtutil_VerbosityEnum2String(Fsm_MgrReadVerbosity(fsmMgr)),
            Fsm_MgrReadVerbosity(fsmMgr))
          );
        return (0);
        break;
      case Pdtutil_MgrOpStats_c:
        Fsm_MgrPrintStats(fsmMgr);
        break;
      case Pdtutil_MgrOpMgrRead_c:
        *voidPointer = (void *)Fsm_MgrReadDdManager(fsmMgr);
        *returnFlagP = Pdtutil_MgrRetDdMgr_c;
        break;
      case Pdtutil_MgrOpMgrSet_c:
        tmpDdMgr = (Ddi_Mgr_t *) * voidPointer;
        Fsm_MgrSetDdManager(fsmMgr, tmpDdMgr);
        break;
      default:
        Pdtutil_Warning(1, "Operation Non Allowed on FSM Manager");
        break;
    }

    return (0);
  }

  /*----------------------------- Package is DDI ----------------------------*/

  if (strcmp(stringFirst, "ddm") == 0) {
    Ddi_MgrOperation(&fsmMgr->dd, stringSecond, operationFlag,
      voidPointer, returnFlagP);

    if (*returnFlagP == Pdtutil_MgrRetNone_c) {
      fsmMgr->dd = NULL;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*-------------------------------- Options -------------------------------*/

  if (strcmp(stringFirst, "verbosity") == 0) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionSet_c:
        Fsm_MgrSetOption(fsmMgr,
          Pdt_FsmVerbosity_c, inum,
          Pdtutil_VerbosityString2Enum(*(char **)voidPointer));
        break;
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Verbosity %s (%d).\n",
            Pdtutil_VerbosityEnum2String(Fsm_MgrReadVerbosity(fsmMgr)),
            Fsm_MgrReadVerbosity(fsmMgr))
          );
        break;
      default:
        Pdtutil_Warning(1, "Operation Non Allowed on FSM Manager");
        break;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*-------------------------------- Options -------------------------------*/

  if (strcmp(stringFirst, "cutThresh") == 0) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionSet_c:
        Fsm_MgrSetOption(fsmMgr, Pdt_FsmCutThresh_c, inum,
          atoi(*(char **)voidPointer));
        break;
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Cut Threshold: %d.\n", Fsm_MgrReadCutThresh(fsmMgr))
          );
        break;
      default:
        Pdtutil_Warning(1, "Operation Non Allowed on FSM Manager");
        break;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  if (strcmp(stringFirst, "useAig") == 0) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionSet_c:
        Fsm_MgrSetOption(fsmMgr, Pdt_FsmUseAig_c, inum,
          atoi(*(char **)voidPointer));
        break;
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Use Aig: %d.\n", Fsm_MgrReadUseAig(fsmMgr))
          );
        break;
      default:
        Pdtutil_Warning(1, "Operation Non Allowed on FSM Manager");
        break;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*-------------------------- Transition Relation --------------------------*/

  if (strcmp(stringFirst, "tr") == 0) {
    Ddi_BddOperation(defaultDdMgr, &(fsmMgr->tr.bdd),
      stringSecond, operationFlag, voidPointer, returnFlagP);

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }


  if (strcmp(stringFirst, "reached") == 0) {
    Ddi_BddOperation(defaultDdMgr, &(fsmMgr->reached.bdd),
      stringSecond, operationFlag, voidPointer, returnFlagP);

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*---------------------------- Initial States -----------------------------*/

  if (strcmp(stringFirst, "init") == 0) {
    Ddi_BddOperation(defaultDdMgr, &(fsmMgr->init.bdd),
      stringSecond, operationFlag, voidPointer, returnFlagP);

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*------------------------------- From Set --------------------------------*/

  if (strcmp(stringFirst, "from") == 0) {
    bdd = Fsm_MgrReadFromBDD(fsmMgr);
    Ddi_BddOperation(defaultDdMgr, &(fsmMgr->from.bdd),
      stringSecond, operationFlag, voidPointer, returnFlagP);

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*---------------------------- Lambda Functions ---------------------------*/

  if (strcmp(stringFirst, "lambda") == 0) {
    Ddi_BddarrayOperation(defaultDdMgr, &(fsmMgr->lambda.bdd),
      stringSecond, operationFlag, voidPointer, returnFlagP);

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*---------------------------- Delta Functions ----------------------------*/

  if (strcmp(stringFirst, "delta") == 0) {
    Ddi_BddarrayOperation(defaultDdMgr, &(fsmMgr->delta.bdd),
      stringSecond, operationFlag, voidPointer, returnFlagP);

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*---------------------------- Delta Functions ----------------------------*/

  if (strcmp(stringFirst, "auxVar") == 0) {
    Ddi_BddarrayOperation(defaultDdMgr, &(fsmMgr->auxVar.bdd),
      stringSecond, operationFlag, voidPointer, returnFlagP);

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*------------------------------- No Match --------------------------------*/

  Pdtutil_Warning(1, "Operation on FSM manager not allowed");
  return (1);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

char *
Fsm_MgrReadFileName(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->fileName);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

char *
Fsm_MgrReadFsmName(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->fsmName);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Mgr_t *
Fsm_MgrReadDdManager(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->dd);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Fsm_MgrReadReachedBDD(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->reached.bdd);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Fsm_MgrReadConstraintBDD(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->constraint.bdd);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Fsm_MgrReadJusticeBDD(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->justice.bdd);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Fsm_MgrReadFairnessBDD(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->fairness.bdd);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Fsm_MgrReadCexBDD(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->cex.bdd);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Fsm_MgrReadInitStubConstraintBDD(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->initStubConstraint.bdd);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Fsm_MgrReadCareBDD(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->care.bdd);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Fsm_MgrReadInvarspecBDD(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->invarspec.bdd);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Varsetarray_t *
Fsm_MgrReadRetimedPis(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->retimedPis);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

int
Fsm_MgrReadIFoldedProp(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->iFoldedProp);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Fsm_MgrReadInitStubPiConstr(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->initStubPiConstr);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Var_t *
Fsm_MgrReadPdtSpecVar(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->pdtSpecVar);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Var_t *
Fsm_MgrReadPdtConstrVar(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->pdtConstrVar);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Var_t *
Fsm_MgrReadPdtInitStubVar(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->pdtInitStubVar);
}



/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

int
Fsm_MgrReadBddFormat(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->bddFormat);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

int
Fsm_MgrReadNI(
  Fsm_Mgr_t * fsmMgr
)
{
  return fsmMgr->var.i==NULL ? 0 : Ddi_VararrayNum(fsmMgr->var.i);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

int
Fsm_MgrReadNO(
  Fsm_Mgr_t * fsmMgr
)
{
  return fsmMgr->lambda.bdd==NULL ? 0 : Ddi_BddarrayNum(fsmMgr->lambda.bdd);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

int
Fsm_MgrReadNL(
  Fsm_Mgr_t * fsmMgr
)
{
  return fsmMgr->delta.bdd==NULL ? 0 : Ddi_BddarrayNum(fsmMgr->delta.bdd);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

int
Fsm_MgrReadNAuxVar(
  Fsm_Mgr_t * fsmMgr
)
{
  return fsmMgr->var.auxVar==NULL ? 0 : Ddi_VararrayNum(fsmMgr->var.auxVar);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Vararray_t *
Fsm_MgrReadVarI(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->var.i);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Vararray_t *
Fsm_MgrReadVarO(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->var.o);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Vararray_t *
Fsm_MgrReadVarPS(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->var.ps);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Vararray_t *
Fsm_MgrReadVarNS(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->var.ns);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Vararray_t *
Fsm_MgrReadVarAuxVar(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->var.auxVar);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

char *
Fsm_MgrReadInitStubName(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->initStub.name);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

char *
Fsm_MgrReadDeltaName(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->delta.name);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Bddarray_t *
Fsm_MgrReadInitStubBDD(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->initStub.bdd);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Bddarray_t *
Fsm_MgrReadDeltaBDD(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->delta.bdd);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

char *
Fsm_MgrReadLambdaName(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->lambda.name);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Bddarray_t *
Fsm_MgrReadLambdaBDD(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->lambda.bdd);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

char *
Fsm_MgrReadAuxVarName(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->auxVar.name);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Bddarray_t *
Fsm_MgrReadAuxVarBDD(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->auxVar.bdd);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

char *
Fsm_MgrReadTrName(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->tr.name);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Fsm_MgrReadTrBDD(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->tr.bdd);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

char *
Fsm_MgrReadInitName(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->init.name);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

char *
Fsm_MgrReadInitString(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->init.string);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

char *
Fsm_MgrReadTrString(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->tr.string);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

char *
Fsm_MgrReadReachedString(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->reached.string);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

char *
Fsm_MgrReadFromString(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->init.string);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Fsm_MgrReadInitBDD(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->init.bdd);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

char *
Fsm_MgrReadFromName(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->from.name);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

char *
Fsm_MgrReadReachedName(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->reached.name);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Fsm_MgrReadFromBDD(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->from.bdd);
}

/**Function********************************************************************
  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

Pdtutil_VerbLevel_e
Fsm_MgrReadVerbosity(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->settings.verbosity);
}

/**Function********************************************************************
  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

int
Fsm_MgrReadCutThresh(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->settings.cutThresh);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

int
Fsm_MgrReadCegarStrategy(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->settings.cegarStrategy);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

int
Fsm_MgrReadPhaseAbstr(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->stats.phaseAbstr);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

int
Fsm_MgrReadXInitLatches(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->stats.xInitLatches);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

int
Fsm_MgrReadExternalConstr(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->stats.externalConstr);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

int
Fsm_MgrReadInitStubSteps(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->stats.initStubSteps);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

int
Fsm_MgrReadTargetEnSteps(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->stats.targetEnSteps);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

int
Fsm_MgrReadRemovedPis(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->stats.removedPis);
}


/**Function********************************************************************
  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

int
Fsm_MgrReadUseAig(
  Fsm_Mgr_t * fsmMgr
)
{
  return (fsmMgr->settings.useAig);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetFileName(
  Fsm_Mgr_t * fsmMgr,
  char *fileName
)
{
  if (fsmMgr->fileName != NULL) {
    Pdtutil_Free(fsmMgr->fileName);
  }

  if (fileName != NULL) {
    fsmMgr->fileName = Pdtutil_StrDup(fileName);
  } else {
    fsmMgr->fileName = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetFsmName(
  Fsm_Mgr_t * fsmMgr,
  char *fsmName
)
{
  if (fsmMgr->fsmName != NULL) {
    Pdtutil_Free(fsmMgr->fsmName);
  }

  if (fsmName != NULL) {
    fsmMgr->fsmName = Pdtutil_StrDup(fsmName);
  } else {
    fsmMgr->fsmName = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetDdManager(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Mgr_t * var
)
{
  fsmMgr->dd = var;

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetReachedBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * var
)
{
  if (fsmMgr->reached.bdd != NULL) {
    Ddi_Free(fsmMgr->reached.bdd);
  }
  if (var != NULL) {
    fsmMgr->reached.bdd = Ddi_BddCopy(fsmMgr->dd, var);
  } else {
    fsmMgr->reached.bdd = NULL;
  }

  return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Fsm_MgrSetConstraintBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * con
)
{
  if (fsmMgr->constraint.bdd != NULL) {
    Ddi_Free(fsmMgr->constraint.bdd);
  }
  if (con != NULL) {
    fsmMgr->constraint.bdd = Ddi_BddCopy(fsmMgr->dd, con);
  } else {
    fsmMgr->constraint.bdd = NULL;
  }

  return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Fsm_MgrSetJusticeBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * just
)
{
  if (fsmMgr->justice.bdd != NULL) {
    Ddi_Free(fsmMgr->justice.bdd);
  }
  if (just != NULL) {
    fsmMgr->justice.bdd = Ddi_BddCopy(fsmMgr->dd, just);
  } else {
    fsmMgr->justice.bdd = NULL;
  }

  return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Fsm_MgrSetFairnessBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * fair
)
{
  if (fsmMgr->fairness.bdd != NULL) {
    Ddi_Free(fsmMgr->fairness.bdd);
  }
  if (fair != NULL) {
    fsmMgr->fairness.bdd = Ddi_BddCopy(fsmMgr->dd, fair);
  } else {
    fsmMgr->fairness.bdd = NULL;
  }

  return;
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Fsm_MgrSetCexBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * cex
)
{
  if (fsmMgr->cex.bdd != NULL) {
    Ddi_Free(fsmMgr->cex.bdd);
  }
  if (cex != NULL) {
    fsmMgr->cex.bdd = Ddi_BddCopy(fsmMgr->dd, cex);
  } else {
    fsmMgr->cex.bdd = NULL;
  }

  return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Fsm_MgrSetInitStubConstraintBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * con
)
{
  if (fsmMgr->initStubConstraint.bdd != NULL) {
    Ddi_Free(fsmMgr->initStubConstraint.bdd);
  }
  if (con != NULL) {
    fsmMgr->initStubConstraint.bdd = Ddi_BddCopy(fsmMgr->dd, con);
  } else {
    fsmMgr->initStubConstraint.bdd = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetCareBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * care
)
{
  if (fsmMgr->care.bdd != NULL) {
    Ddi_Free(fsmMgr->care.bdd);
  }
  if (care != NULL) {
    fsmMgr->care.bdd = Ddi_BddCopy(fsmMgr->dd, care);
  } else {
    fsmMgr->care.bdd = NULL;
  }

  return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

void
Fsm_MgrSetInvarspecBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * inv
)
{
  if (fsmMgr->invarspec.bdd != NULL) {
    Ddi_Free(fsmMgr->invarspec.bdd);
  }
  if (inv != NULL) {
    fsmMgr->invarspec.bdd = Ddi_BddCopy(fsmMgr->dd, inv);
  } else {
    fsmMgr->invarspec.bdd = NULL;
  }

  return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

void
Fsm_MgrSetRetimedPis(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Varsetarray_t * retimedPis
)
{
  if (fsmMgr->retimedPis != NULL) {
    Ddi_Free(fsmMgr->retimedPis);
  }
  if (retimedPis != NULL) {
    fsmMgr->retimedPis = Ddi_VarsetarrayCopy(fsmMgr->dd, retimedPis);
  } else {
    fsmMgr->retimedPis = NULL;
  }

  return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

void
Fsm_MgrSetInitStubPiConstr(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * initStubPiConstr
)
{
  if (fsmMgr->initStubPiConstr != NULL) {
    Ddi_Free(fsmMgr->initStubPiConstr);
  }
  if (initStubPiConstr != NULL) {
    fsmMgr->initStubPiConstr = Ddi_BddCopy(fsmMgr->dd, initStubPiConstr);
  } else {
    fsmMgr->initStubPiConstr = NULL;
  }

  return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

void
Fsm_MgrSetPdtSpecVar(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Var_t * v
)
{
  if (v != NULL) {
    fsmMgr->pdtSpecVar = Ddi_VarCopy(fsmMgr->dd, v);
  } else {
    fsmMgr->pdtSpecVar = NULL;
  }

  return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

void
Fsm_MgrSetPdtConstrVar(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Var_t * v
)
{
  if (v != NULL) {
    fsmMgr->pdtConstrVar = Ddi_VarCopy(fsmMgr->dd, v);
  } else {
    fsmMgr->pdtConstrVar = NULL;
  }

  return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

void
Fsm_MgrSetPdtInitStubVar(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Var_t * v
)
{
  if (v != NULL) {
    fsmMgr->pdtInitStubVar = Ddi_VarCopy(fsmMgr->dd, v);
  } else {
    fsmMgr->pdtInitStubVar = NULL;
  }

  return;
}



/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetBddFormat(
  Fsm_Mgr_t * fsmMgr,
  int var
)
{
  fsmMgr->bddFormat = var;

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetVarI(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Vararray_t * var
)
{
  if (fsmMgr->var.i != NULL) {
    Ddi_Free(fsmMgr->var.i);
  }

  if (var != NULL) {
    fsmMgr->var.i = Ddi_VararrayCopy(fsmMgr->dd, var);
  } else {
    fsmMgr->var.i = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetVarO(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Vararray_t * var
)
{
  if (fsmMgr->var.o != NULL) {
    Ddi_Free(fsmMgr->var.o);
  }

  if (var != NULL) {
    fsmMgr->var.o = Ddi_VararrayCopy(fsmMgr->dd, var);
  } else {
    fsmMgr->var.o = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetVarPS(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Vararray_t * var
)
{
  if (fsmMgr->var.ps != NULL) {
    Ddi_Free(fsmMgr->var.ps);
  }

  if (var != NULL) {
    fsmMgr->var.ps = Ddi_VararrayCopy(fsmMgr->dd, var);
  } else {
    fsmMgr->var.ps = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetVarNS(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Vararray_t * var
)
{
  if (fsmMgr->var.ns != NULL) {
    Ddi_Free(fsmMgr->var.ns);
  }

  if (var != NULL) {
    fsmMgr->var.ns = Ddi_VararrayCopy(fsmMgr->dd, var);
  } else {
    fsmMgr->var.ns = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetVarAuxVar(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Vararray_t * var
)
{
  if (fsmMgr->var.auxVar != NULL) {
    Ddi_Free(fsmMgr->var.auxVar);
  }

  if (var != NULL) {
    fsmMgr->var.auxVar = Ddi_VararrayCopy(fsmMgr->dd, var);
  } else {
    fsmMgr->var.auxVar = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetInitStubName(
  Fsm_Mgr_t * fsmMgr,
  char *var
)
{
  if (fsmMgr->initStub.name != NULL) {
    Pdtutil_Free(fsmMgr->initStub.name);
  }

  if (var != NULL) {
    fsmMgr->initStub.name = Pdtutil_StrDup(var);
  } else {
    fsmMgr->initStub.name = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetDeltaName(
  Fsm_Mgr_t * fsmMgr,
  char *var
)
{
  if (fsmMgr->delta.name != NULL) {
    Pdtutil_Free(fsmMgr->delta.name);
  }

  if (var != NULL) {
    fsmMgr->delta.name = Pdtutil_StrDup(var);
  } else {
    fsmMgr->delta.name = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetInitStubBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * initStub
)
{
  if (fsmMgr->initStub.bdd != NULL) {
    Ddi_Free(fsmMgr->initStub.bdd);
  }

  if (initStub != NULL) {
    fsmMgr->initStub.bdd = Ddi_BddarrayCopy(fsmMgr->dd, initStub);
    if (fsmMgr->init.bdd != NULL) {
      Ddi_Free(fsmMgr->init.bdd);
      fsmMgr->init.bdd = Ddi_BddMakeConstAig(fsmMgr->dd, 0);
    }
  } else {
    fsmMgr->initStub.bdd = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetDeltaBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * delta
)
{
  if (fsmMgr->delta.bdd != NULL) {
    Ddi_Free(fsmMgr->delta.bdd);
  }

  if (delta != NULL) {
    fsmMgr->delta.bdd = Ddi_BddarrayCopy(fsmMgr->dd, delta);
  } else {
    fsmMgr->delta.bdd = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetLambdaName(
  Fsm_Mgr_t * fsmMgr,
  char *var
)
{
  if (fsmMgr->lambda.name != NULL) {
    Pdtutil_Free(fsmMgr->lambda.name);
  }

  if (var != NULL) {
    fsmMgr->lambda.name = Pdtutil_StrDup(var);
  } else {
    fsmMgr->lambda.name = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetLambdaBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * lambda
)
{
  if (fsmMgr->lambda.bdd != NULL) {
    Ddi_Free(fsmMgr->lambda.bdd);
  }

  if (lambda != NULL) {
    fsmMgr->lambda.bdd = Ddi_BddarrayCopy(fsmMgr->dd, lambda);
  } else {
    fsmMgr->lambda.bdd = NULL;
  }

  return;
}


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetAuxVarName(
  Fsm_Mgr_t * fsmMgr,
  char *var
)
{
  if (fsmMgr->auxVar.name != NULL) {
    Pdtutil_Free(fsmMgr->auxVar.name);
  }

  if (var != NULL) {
    fsmMgr->auxVar.name = Pdtutil_StrDup(var);
  } else {
    fsmMgr->auxVar.name = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetAuxVarBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * var
)
{
  if (fsmMgr->auxVar.bdd != NULL) {
    Ddi_Free(fsmMgr->auxVar.bdd);
  }

  if (var != NULL) {
    fsmMgr->auxVar.bdd = Ddi_BddarrayCopy(fsmMgr->dd, var);
  } else {
    fsmMgr->auxVar.bdd = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetTrName(
  Fsm_Mgr_t * fsmMgr,
  char *var
)
{
  if (fsmMgr->tr.name != NULL) {
    Pdtutil_Free(fsmMgr->tr.name);
  }

  if (var != NULL) {
    fsmMgr->tr.name = Pdtutil_StrDup(var);
  } else {
    fsmMgr->tr.name = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetTrBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * var
)
{
  if (fsmMgr->tr.bdd != NULL) {
    Ddi_Free(fsmMgr->tr.bdd);
  }

  if (var != NULL) {
    fsmMgr->tr.bdd = Ddi_BddCopy(fsmMgr->dd, var);
  } else {
    fsmMgr->tr.bdd = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetInitName(
  Fsm_Mgr_t * fsmMgr,
  char *var
)
{
  if (fsmMgr->init.name != NULL) {
    Pdtutil_Free(fsmMgr->init.name);
  }

  if (var != NULL) {
    fsmMgr->init.name = Pdtutil_StrDup(var);
  } else {
    fsmMgr->init.name = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetInitString(
  Fsm_Mgr_t * fsmMgr,
  char *var
)
{
  if (fsmMgr->init.string != NULL) {
    Pdtutil_Free(fsmMgr->init.string);
  }

  if (var != NULL) {
    fsmMgr->init.string = Pdtutil_StrDup(var);
  } else {
    fsmMgr->init.string = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetTrString(
  Fsm_Mgr_t * fsmMgr,
  char *var
)
{
  if (fsmMgr->tr.string != NULL) {
    Pdtutil_Free(fsmMgr->tr.string);
  }

  if (var != NULL) {
    fsmMgr->tr.string = Pdtutil_StrDup(var);
  } else {
    fsmMgr->tr.string = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetFromString(
  Fsm_Mgr_t * fsmMgr,
  char *var
)
{
  if (fsmMgr->from.string != NULL) {
    Pdtutil_Free(fsmMgr->from.string);
  }

  if (var != NULL) {
    fsmMgr->from.string = Pdtutil_StrDup(var);
  } else {
    fsmMgr->from.string = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetReachedString(
  Fsm_Mgr_t * fsmMgr,
  char *var
)
{
  if (fsmMgr->reached.string != NULL) {
    Pdtutil_Free(fsmMgr->reached.string);
    exit(1);
  }

  if (var != NULL) {
    fsmMgr->reached.string = Pdtutil_StrDup(var);
  } else {
    fsmMgr->reached.string = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetInitBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * var
)
{
  if (fsmMgr->init.bdd != NULL) {
    Ddi_Free(fsmMgr->init.bdd);
  }

  if (var != NULL) {
    fsmMgr->init.bdd = Ddi_BddCopy(fsmMgr->dd, var);
  } else {
    fsmMgr->init.bdd = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetFromName(
  Fsm_Mgr_t * fsmMgr,
  char *var
)
{
  if (fsmMgr->from.name != NULL) {
    Pdtutil_Free(fsmMgr->from.name);
  }

  if (var != NULL) {
    fsmMgr->from.name = Pdtutil_StrDup(var);
  } else {
    fsmMgr->from.name = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetReachedName(
  Fsm_Mgr_t * fsmMgr,
  char *var
)
{
  if (fsmMgr->reached.name != NULL) {
    Pdtutil_Free(fsmMgr->reached.name);
  }

  if (var != NULL) {
    fsmMgr->reached.name = Pdtutil_StrDup(var);
  } else {
    fsmMgr->reached.name = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetFromBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * var
)
{
  if (fsmMgr->from.bdd != NULL) {
    Ddi_Free(fsmMgr->from.bdd);
  }

  if (var != NULL) {
    fsmMgr->from.bdd = Ddi_BddCopy(fsmMgr->dd, var);
  } else {
    fsmMgr->from.bdd = NULL;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetVerbosity(
  Fsm_Mgr_t * fsmMgr,
  Pdtutil_VerbLevel_e var
)
{
  fsmMgr->settings.verbosity = var;

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetCutThresh(
  Fsm_Mgr_t * fsmMgr,
  int cutThresh
)
{
  fsmMgr->settings.cutThresh = cutThresh;

  return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

void
Fsm_MgrSetCegarStrategy(
  Fsm_Mgr_t * fsmMgr,
  int cegarStrategy
)
{
  fsmMgr->settings.cegarStrategy = cegarStrategy;

  return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

void
Fsm_MgrSetPhaseAbstr(
  Fsm_Mgr_t * fsmMgr,
  int phaseAbstr
)
{
  fsmMgr->stats.phaseAbstr = phaseAbstr;
  fsmMgr->stats.initStubStepsAtPhaseAbstr = fsmMgr->stats.initStubSteps;
  return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

void
Fsm_MgrSetRemovedPis(
  Fsm_Mgr_t * fsmMgr,
  int removedPis
)
{
  fsmMgr->stats.removedPis = removedPis;
  return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

void
Fsm_MgrSetTargetEnSteps(
  Fsm_Mgr_t * fsmMgr,
  int targetEnSteps
)
{
  fsmMgr->stats.targetEnSteps = targetEnSteps;
  return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

void
Fsm_MgrSetInitStubSteps(
  Fsm_Mgr_t * fsmMgr,
  int initStubSteps
)
{
  fsmMgr->stats.initStubSteps = initStubSteps;
  return;
}




/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

void
Fsm_MgrSetUseAig(
  Fsm_Mgr_t * fsmMgr,
  int useAig
)
{
  fsmMgr->settings.useAig = useAig;

  return;
}


/**Function********************************************************************

  Synopsis    [Transfer options from an option list to a Fsm Manager]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
void
Fsm_MgrSetOptionList(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  Pdtutil_OptList_t * pkgOpt    /* list of options */
)
{
  Pdtutil_OptItem_t optItem;

  while (!Pdtutil_OptListEmpty(pkgOpt)) {
    optItem = Pdtutil_OptListExtractHead(pkgOpt);
    Fsm_MgrSetOptionItem(fsmMgr, optItem);
  }
}


/**Function********************************************************************

  Synopsis    [Transfer options from an option list to a Fsm Manager]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
void
Fsm_MgrSetOptionItem(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  Pdtutil_OptItem_t optItem     /* option to set */
)
{
  switch (optItem.optTag.eFsmOpt) {
    case Pdt_FsmVerbosity_c:
      fsmMgr->settings.verbosity = Pdtutil_VerbLevel_e(optItem.optData.inum);
      break;
    case Pdt_FsmCutAtAuxVar_c:
      fsmMgr->settings.cutAtAuxVar = optItem.optData.inum;
      break;
    case Pdt_FsmCutThresh_c:
      fsmMgr->settings.cutThresh = optItem.optData.inum;
      break;
    case Pdt_FsmUseAig_c:
      fsmMgr->settings.useAig = optItem.optData.inum;
      break;
    case Pdt_FsmCegarStrategy_c:
      fsmMgr->settings.cegarStrategy = optItem.optData.inum;
      break;
    case Pdt_FsmTargetEnSteps_c:
      fsmMgr->stats.targetEnSteps = optItem.optData.inum;
      break;
    case Pdt_FsmInitStubSteps_c:
      fsmMgr->stats.initStubSteps = optItem.optData.inum;
      break;
    case Pdt_FsmPhaseAbstr_c:
      fsmMgr->stats.phaseAbstr = optItem.optData.inum;
      break;
    case Pdt_FsmRemovedPis_c:
      fsmMgr->stats.removedPis = optItem.optData.inum;
      break;
    case Pdt_FsmExternalConstr_c:
      fsmMgr->stats.externalConstr = optItem.optData.inum;
      break;
    case Pdt_FsmXInitLatches_c:
      fsmMgr->stats.xInitLatches = optItem.optData.inum;
      break;
    default:
      Pdtutil_Assert(0, "wrong fsm opt list item tag");
  }
}

/**Function********************************************************************
  Synopsis    [Return an option value from a Fsm Manager ]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Fsm_MgrReadOption(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  Pdt_OptTagFsm_e fsmOpt /* option code */ ,
  void *optRet                  /* returned option value */
)
{
  switch (fsmOpt) {
    case Pdt_FsmVerbosity_c:
      *(int *)optRet = (int)fsmMgr->settings.verbosity;
      break;
    case Pdt_FsmCutAtAuxVar_c:
      *(int *)optRet = (int)fsmMgr->settings.cutAtAuxVar;
      break;
    case Pdt_FsmCutThresh_c:
      *(int *)optRet = (int)fsmMgr->settings.cutThresh;
      break;
    case Pdt_FsmUseAig_c:
      *(int *)optRet = (int)fsmMgr->settings.useAig;
      break;
    case Pdt_FsmCegarStrategy_c:
      *(int *)optRet = (int)fsmMgr->settings.cegarStrategy;
      break;
    case Pdt_FsmTargetEnSteps_c:
      *(int *)optRet = (int)fsmMgr->stats.targetEnSteps;
      break;
    case Pdt_FsmInitStubSteps_c:
      *(int *)optRet = (int)fsmMgr->stats.initStubSteps;
      break;
    case Pdt_FsmPhaseAbstr_c:
      *(int *)optRet = (int)fsmMgr->stats.phaseAbstr;
      break;
    case Pdt_FsmRemovedPis_c:
      *(int *)optRet = (int)fsmMgr->stats.removedPis;
      break;
    case Pdt_FsmExternalConstr_c:
      *(int *)optRet = (int)fsmMgr->stats.externalConstr;
      break;
    case Pdt_FsmXInitLatches_c:
      *(int *)optRet = (int)fsmMgr->stats.xInitLatches;
      break;

    default:
      Pdtutil_Assert(0, "wrong fsm opt item tag");
  }
}


/**Function********************************************************************
  Synopsis    [Return an option value from a Fsm Manager ]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Fsm_MgrLogVerificationResult(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  char *filename,
  int result
)
{
  Pdtutil_WresNum(filename, NULL, result, 0, 0, -1, -1, 1);
}

/**Function********************************************************************
  Synopsis    [Return an option value from a Fsm Manager ]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Fsm_MgrLogUnsatBound(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  int bound,
  int unfolded
)
{
  int initStubSteps = fsmMgr->stats.initStubSteps;
  int targetEnSteps = fsmMgr->stats.targetEnSteps;
  int phaseAbstr = fsmMgr->stats.phaseAbstr;

  Pdtutil_WresNum(NULL, "u", bound, 1, 1, phaseAbstr,
		  initStubSteps + targetEnSteps, unfolded);
}

/**Function********************************************************************
  Synopsis    [Return an option value from a Fsm Manager ]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
int
Fsm_MgrReadUnsatBound(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */
)
{
  int initStubSteps = fsmMgr->stats.initStubSteps;
  int targetEnSteps = fsmMgr->stats.targetEnSteps;
  int phaseAbstr = fsmMgr->stats.phaseAbstr;

  return Pdtutil_WresReadNum(phaseAbstr,initStubSteps + targetEnSteps);
}

/**Function********************************************************************
  Synopsis    [Return an field value from a Fsm Manager ]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Fsm_MgrReadItem(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  Pdtutil_OptItem_t * optItemPtr
)
{
  switch (optItemPtr->optTag.eFsmOpt) {
    case Pdt_FsmInvarspec_c:
      optItemPtr->optData.pvoid = (void *)fsmMgr->invarspec.bdd;
      break;
  }
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

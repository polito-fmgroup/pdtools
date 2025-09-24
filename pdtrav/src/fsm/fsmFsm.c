/**CFile***********************************************************************

  FileName    [fsmFsm.c]

  PackageName [fsm]

  Synopsis    [Utility functions to create, free and duplicate a MINI FSM]

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

static Fsm_Fsm_t *FsmFsmDupIntern(
  Fsm_Fsm_t * fsmFsm
);

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Duplicates MINIFSM structure.]

  Description [The new FSM is allocated and all the field are duplicated
    from the original FSM.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Fsm_Fsm_t *
Fsm_FsmDup(
  Fsm_Fsm_t * fsmFsm            /* MINIFSM */
)
{
  return FsmFsmDupIntern(fsmFsm);
}

/**Function********************************************************************

  Synopsis     [Initializes the MINIFSM structure.]

  Description  [Notice: Direct access to fileds is used, because macros
    check for NULL fields to discover memory leaks.]

  SideEffects  [none]

******************************************************************************/

Fsm_Fsm_t *
Fsm_FsmInit(
)
{
  Fsm_Fsm_t *fsmFsm;

  /*------------------------- FSM Structure Creation -----------------------*/

  fsmFsm = Pdtutil_Alloc(Fsm_Fsm_t, 1);
  if (fsmFsm == NULL) {
    fprintf(stderr, "fsmUtil.c (Fsm_Fsm_Init) Error: Out of memory.\n");
    return (NULL);
  }

  /*------------------------------- Set Fields -----------------------------*/

  fsmFsm->fsmMgr = NULL;

  fsmFsm->size.i = 0;
  fsmFsm->size.o = 0;
  fsmFsm->size.l = 0;
  fsmFsm->size.auxVar = 0;


  fsmFsm->initStub = NULL;
  fsmFsm->delta = NULL;
  fsmFsm->lambda = NULL;

  fsmFsm->cex = NULL;
  fsmFsm->init = NULL;
  fsmFsm->constraint = NULL;
  fsmFsm->justice = NULL;
  fsmFsm->fairness = NULL;
  fsmFsm->initStubConstraint = NULL;
  fsmFsm->latchEqClasses = NULL;
  fsmFsm->constrInvar = NULL;

  fsmFsm->iFoldedProp = -1;
  fsmFsm->iFoldedConstr = -1;
  fsmFsm->iFoldedInit = -1;

  fsmFsm->invarspec = NULL;

  fsmFsm->pi = NULL;
  fsmFsm->ps = NULL;
  fsmFsm->ns = NULL;
  fsmFsm->is = NULL;


  /*
   *  Settings
   */


  fsmFsm->settings = NULL;

  /*
   *  Stats
   */


  fsmFsm->stats = NULL;

  /*
   *  Chain of FSMs
   */

  fsmFsm->prev = NULL;
  fsmFsm->next = NULL;


  return (fsmFsm);
}


/**Function********************************************************************

  Synopsis    [Duplicates MINIFSM structure.]

  Description [The new MINIFSM is allocated and all the field are duplicated
    from the original FSM.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

static Fsm_Fsm_t *
FsmFsmDupIntern(
  Fsm_Fsm_t * fsmFsm            /* MINIFSM */
)
{
  Fsm_Fsm_t *fsmFsmNew;

  /*------------------------- FSM structure creation -----------------------*/

  fsmFsmNew = Fsm_FsmInit();


  /*------------------------------- Copy Fields ----------------------------*/

  /*
   *  Manager
   */

  Fsm_FsmSetFsmMgr(fsmFsmNew, Fsm_FsmReadMgr(fsmFsm));

  /*
   *  Sizes
   */

  Fsm_FsmSetNI(fsmFsmNew, Fsm_FsmReadNI(fsmFsm));
  Fsm_FsmSetNO(fsmFsmNew, Fsm_FsmReadNO(fsmFsm));
  Fsm_FsmSetNL(fsmFsmNew, Fsm_FsmReadNL(fsmFsm));

  /*
   *  PI, LAMBDA, DELTA, PS, NS, 
   */

  if (Fsm_FsmReadPI(fsmFsm) != NULL) {
    Fsm_FsmWritePI(fsmFsmNew, Fsm_FsmReadPI(fsmFsm));
  }
  if (Fsm_FsmReadLambda(fsmFsm) != NULL) {
    Fsm_FsmWriteLambda(fsmFsmNew, Fsm_FsmReadLambda(fsmFsm));
  }

  if (Fsm_FsmReadPS(fsmFsm) != NULL) {
    Fsm_FsmWritePS(fsmFsmNew, Fsm_FsmReadPS(fsmFsm));
  }
  if (Fsm_FsmReadNS(fsmFsm) != NULL) {
    Fsm_FsmWriteNS(fsmFsmNew, Fsm_FsmReadNS(fsmFsm));
  }

  if (Fsm_FsmReadDelta(fsmFsm) != NULL) {
    Fsm_FsmWriteDelta(fsmFsmNew, Fsm_FsmReadDelta(fsmFsm));
  }

  /*************/

  if (Fsm_FsmReadInit(fsmFsm) != NULL) {
    Fsm_FsmWriteInit(fsmFsmNew, Fsm_FsmReadInit(fsmFsm));
  }

  if (Fsm_FsmReadInitStub(fsmFsm) != NULL) {
    Fsm_FsmWriteInitStub(fsmFsmNew, Fsm_FsmReadInitStub(fsmFsm));
  }

  if (Fsm_FsmReadConstraint(fsmFsm) != NULL) {
    Fsm_FsmWriteConstraint(fsmFsmNew, Fsm_FsmReadConstraint(fsmFsm));
  }

  if (Fsm_FsmReadJustice(fsmFsm) != NULL) {
    Fsm_FsmWriteJustice(fsmFsmNew, Fsm_FsmReadJustice(fsmFsm));
  }

  if (Fsm_FsmReadFairness(fsmFsm) != NULL) {
    Fsm_FsmWriteFairness(fsmFsmNew, Fsm_FsmReadFairness(fsmFsm));
  }

  if (Fsm_FsmReadInitStubConstraint(fsmFsm) != NULL) {
    Fsm_FsmWriteInitStubConstraint(fsmFsmNew,
      Fsm_FsmReadInitStubConstraint(fsmFsm));
  }

  if (Fsm_FsmReadLatchEqClasses(fsmFsm) != NULL) {
    Fsm_FsmWriteLatchEqClasses(fsmFsmNew,
      Fsm_FsmReadLatchEqClasses(fsmFsm));
  }
  
  if (Fsm_FsmReadConstrInvar(fsmFsm) != NULL) {
    Fsm_FsmWriteConstrInvar(fsmFsmNew,
      Fsm_FsmReadConstrInvar(fsmFsm));
  }
  
  if (Fsm_FsmReadCex(fsmFsm) != NULL) {
    Fsm_FsmWriteCex(fsmFsmNew, Fsm_FsmReadCex(fsmFsm));
  }

  if (Fsm_FsmReadInvarspec(fsmFsm) != NULL) {
    Fsm_FsmWriteInvarspec(fsmFsmNew, Fsm_FsmReadInvarspec(fsmFsm));
  }

  if (Fsm_FsmReadSettings(fsmFsm) != NULL) {
    Fsm_FsmWriteSettings(fsmFsmNew, Fsm_FsmReadSettings(fsmFsm));
  }

  if (Fsm_FsmReadStats(fsmFsm) != NULL) {
    Fsm_FsmWriteStats(fsmFsmNew, Fsm_FsmReadStats(fsmFsm));
  }

  fsmFsmNew->iFoldedProp = fsmFsm->iFoldedProp;
  fsmFsmNew->iFoldedConstr = fsmFsm->iFoldedConstr;
  fsmFsmNew->iFoldedInit = fsmFsm->iFoldedInit;


  return (fsmFsmNew);
}

/*
 * GETTERS
 */


Fsm_Mgr_t *
Fsm_FsmReadMgr(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->fsmMgr);
}


int
Fsm_FsmReadNI(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->size.i);
}

int
Fsm_FsmReadNO(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->size.o);
}

int
Fsm_FsmReadNL(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->size.l);
}

Ddi_Vararray_t *
Fsm_FsmReadPS(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->ps);
}


Ddi_Vararray_t *
Fsm_FsmReadNS(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->ns);
}



Ddi_Vararray_t *
Fsm_FsmReadPI(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->pi);
}


Ddi_Bddarray_t *
Fsm_FsmReadLambda(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->lambda);
}



Ddi_Bddarray_t *
Fsm_FsmReadDelta(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->delta);
}


Ddi_Bddarray_t *
Fsm_FsmReadInitStub(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->initStub);
}



Ddi_Bdd_t *
Fsm_FsmReadInit(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->init);
}


Ddi_Bdd_t *
Fsm_FsmReadInvarspec(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->invarspec);
}

Ddi_Bdd_t *
Fsm_FsmReadConstraint(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->constraint);
}

Ddi_Bdd_t *
Fsm_FsmReadJustice(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->justice);
}

Ddi_Bdd_t *
Fsm_FsmReadFairness(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->fairness);
}

Ddi_Bdd_t *
Fsm_FsmReadInitStubConstraint(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->initStubConstraint);
}

Ddi_Bdd_t *
Fsm_FsmReadLatchEqClasses(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->latchEqClasses);
}

Ddi_Bdd_t *
Fsm_FsmReadConstrInvar(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->constrInvar);
}

Ddi_Bdd_t *
Fsm_FsmReadCex(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->cex);
}

Pdtutil_List_t *
Fsm_FsmReadSettings(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->settings);
}

Pdtutil_List_t *
Fsm_FsmReadStats(
  Fsm_Fsm_t * fsmFsm
)
{
  return (fsmFsm->stats);
}


/*
 * SETTERS
 */

void
Fsm_FsmExchangeFsmMgr(
  Fsm_Fsm_t * fsmFsm,
  Fsm_Mgr_t * fsm_Mgr
)
{
  // Unlink current fsm from the current manager list
  if (fsmFsm->prev != NULL)
    fsmFsm->prev->next = fsmFsm->next;
  if (fsmFsm->next != NULL)
    fsmFsm->next->prev = fsmFsm->prev;

  Fsm_FsmSetFsmMgr(fsmFsm, fsm_Mgr);
}

void
Fsm_FsmSetFsmMgr(
  Fsm_Fsm_t * fsmFsm,
  Fsm_Mgr_t * fsm_Mgr
)
{
  fsmFsm->fsmMgr = fsm_Mgr;
  fsmFsm->prev = NULL;
  fsmFsm->next = NULL;

  // Update chain of FSMs
  if (fsm_Mgr->fsmList != NULL) {
    fsmFsm->next = fsm_Mgr->fsmList;
    fsm_Mgr->fsmList->prev = fsmFsm;
    fsm_Mgr->fsmList = fsmFsm;
  } else {
    fsm_Mgr->fsmList = fsmFsm;
  }
}

void
Fsm_FsmSetNI(
  Fsm_Fsm_t * fsmFsm,
  int ni
)
{
  fsmFsm->size.i = ni;
}

void
Fsm_FsmSetNO(
  Fsm_Fsm_t * fsmFsm,
  int no
)
{
  fsmFsm->size.o = no;
}


void
Fsm_FsmSetNL(
  Fsm_Fsm_t * fsmFsm,
  int nl
)
{
  fsmFsm->size.l = nl;
}

void
Fsm_FsmWriteDelta(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bddarray_t * delta
)
{
  Ddi_Unlock(fsmFsm->delta);
  Ddi_Free(fsmFsm->delta);
  fsmFsm->delta = Ddi_BddarrayDup(delta);
  Ddi_Lock(fsmFsm->delta);
}

void
Fsm_FsmWriteLambda(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bddarray_t * lambda
)
{
  Ddi_Unlock(fsmFsm->lambda);
  Ddi_Free(fsmFsm->lambda);
  fsmFsm->lambda = Ddi_BddarrayDup(lambda);
  Ddi_Lock(fsmFsm->lambda);
}

void
Fsm_FsmWriteInitStub(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bddarray_t * initStub
)
{
  Ddi_Unlock(fsmFsm->initStub);
  Ddi_Free(fsmFsm->initStub);
  fsmFsm->initStub = Ddi_BddarrayDup(initStub);
  Ddi_Lock(fsmFsm->initStub);
}

void
Fsm_FsmWritePI(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Vararray_t * pi
)
{
  Ddi_Unlock(fsmFsm->pi);
  Ddi_Free(fsmFsm->pi);
  fsmFsm->pi = Ddi_VararrayDup(pi);
  Ddi_Lock(fsmFsm->pi);
}

void
Fsm_FsmWritePS(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Vararray_t * ps
)
{
  Ddi_Unlock(fsmFsm->ps);
  Ddi_Free(fsmFsm->ps);
  fsmFsm->ps = Ddi_VararrayDup(ps);
  Ddi_Lock(fsmFsm->ps);
}

void
Fsm_FsmWriteNS(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Vararray_t * ns
)
{
  Ddi_Unlock(fsmFsm->ns);
  Ddi_Free(fsmFsm->ns);
  fsmFsm->ns = Ddi_VararrayDup(ns);
  Ddi_Lock(fsmFsm->ns);
}

void
Fsm_FsmWriteInit(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * init
)
{
  Ddi_Unlock(fsmFsm->init);
  Ddi_Free(fsmFsm->init);
  fsmFsm->init = Ddi_BddDup(init);
  Ddi_Lock(fsmFsm->init);
}

void
Fsm_FsmWriteInvarspec(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * invarspec
)
{
  Ddi_Unlock(fsmFsm->invarspec);
  Ddi_Free(fsmFsm->invarspec);
  fsmFsm->invarspec = Ddi_BddDup(invarspec);
  Ddi_Lock(fsmFsm->invarspec);
}

void
Fsm_FsmWriteConstraint(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * constraint
)
{
  Ddi_Unlock(fsmFsm->constraint);
  Ddi_Free(fsmFsm->constraint);
  fsmFsm->constraint = Ddi_BddDup(constraint);
  Ddi_Lock(fsmFsm->constraint);
}

void
Fsm_FsmWriteJustice(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * justice
)
{
  Ddi_Unlock(fsmFsm->justice);
  Ddi_Free(fsmFsm->justice);
  fsmFsm->justice = Ddi_BddDup(justice);
  Ddi_Lock(fsmFsm->justice);
}

void
Fsm_FsmWriteFairness(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * fairness
)
{
  Ddi_Unlock(fsmFsm->fairness);
  Ddi_Free(fsmFsm->fairness);
  fsmFsm->fairness = Ddi_BddDup(fairness);
  Ddi_Lock(fsmFsm->fairness);
}

void
Fsm_FsmWriteInitStubConstraint(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * constraint
)
{
  Ddi_Unlock(fsmFsm->initStubConstraint);
  Ddi_Free(fsmFsm->initStubConstraint);
  fsmFsm->initStubConstraint = Ddi_BddDup(constraint);
  Ddi_Lock(fsmFsm->initStubConstraint);
}

void
Fsm_FsmWriteLatchEqClasses(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * lEq
)
{
  Ddi_Unlock(fsmFsm->latchEqClasses);
  Ddi_Free(fsmFsm->latchEqClasses);
  fsmFsm->latchEqClasses = Ddi_BddDup(lEq);
  Ddi_Lock(fsmFsm->latchEqClasses);
}

void
Fsm_FsmWriteConstrInvar(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * cInv
)
{
  Ddi_Unlock(fsmFsm->constrInvar);
  Ddi_Free(fsmFsm->constrInvar);
  fsmFsm->constrInvar = Ddi_BddDup(cInv);
  Ddi_Lock(fsmFsm->constrInvar);
}

void
Fsm_FsmWriteCex(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * cex
)
{
  Ddi_Unlock(fsmFsm->cex);
  Ddi_Free(fsmFsm->cex);
  fsmFsm->cex = Ddi_BddDup(cex);
  Ddi_Lock(fsmFsm->cex);
}

void
Fsm_FsmWriteSettings(
  Fsm_Fsm_t * fsmFsm,
  Pdtutil_List_t * settings
)
{
  fsmFsm->settings = settings;
}

void
Fsm_FsmWriteStats(
  Fsm_Fsm_t * fsmFsm,
  Pdtutil_List_t * stats
)
{
  fsmFsm->stats = stats;
}



Fsm_Fsm_t *
Fsm_FsmMake(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * delta,
  Ddi_Bddarray_t * lambda,
  Ddi_Bddarray_t * initStub,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * constraint,
  Ddi_Bdd_t * initStubConstraint,
  Ddi_Bdd_t * cex,
  Ddi_Vararray_t * pi,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * ns,
  int *settings,
  int *stats
)
{
  Fsm_Fsm_t *fsmFsm;
  int addNewPis = 1;
  Ddi_Mgr_t *ddm = fsmMgr->dd;

  Pdtutil_Assert(fsmMgr != NULL, "Fsm Mgr must be !=NULL");

  fsmFsm = Fsm_FsmInit();

  Fsm_FsmSetFsmMgr(fsmFsm, fsmMgr);


  /*
   *  PI, LAMBDA, DELTA, PS, NS, 
   */

  if (pi == NULL) {

    if (Fsm_MgrReadVarI(fsmMgr) != NULL) {
      Fsm_FsmWritePI(fsmFsm, Fsm_MgrReadVarI(fsmMgr));
    }

  } else {
    Fsm_FsmWritePI(fsmFsm, pi);
  }

  if (lambda == NULL) {
    if (Fsm_MgrReadLambdaBDD(fsmMgr) != NULL) {
      Fsm_FsmWriteLambda(fsmFsm, Fsm_MgrReadLambdaBDD(fsmMgr));
    }

  } else {

    Fsm_FsmWriteLambda(fsmFsm, lambda);
  }

  if (ps == NULL) {
    if (Fsm_MgrReadVarPS(fsmMgr) != NULL) {
      Fsm_FsmWritePS(fsmFsm, Fsm_MgrReadVarPS(fsmMgr));
    }
  } else {
    Fsm_FsmWritePS(fsmFsm, ps);

  }

  if (ns == NULL) {
    if (Fsm_MgrReadVarNS(fsmMgr) != NULL) {
      Fsm_FsmWriteNS(fsmFsm, Fsm_MgrReadVarNS(fsmMgr));
    }
  } else {
    Fsm_FsmWriteNS(fsmFsm, ns);
  }

  if (delta == NULL) {
    if (Fsm_MgrReadDeltaBDD(fsmMgr) != NULL) {
      Fsm_FsmWriteDelta(fsmFsm, Fsm_MgrReadDeltaBDD(fsmMgr));
    }
  } else {
    Fsm_FsmWriteDelta(fsmFsm, delta);
  }

  /*************/

  if (init == NULL) {
    if (Fsm_MgrReadInitBDD(fsmMgr) != NULL) {
      Fsm_FsmWriteInit(fsmFsm, Fsm_MgrReadInitBDD(fsmMgr));
    }

  } else {
    Fsm_FsmWriteInit(fsmFsm, init);
  }

  if (initStub == NULL) {
    if (Fsm_MgrReadInitStubBDD(fsmMgr) != NULL) {
      Fsm_FsmWriteInitStub(fsmFsm, Fsm_MgrReadInitStubBDD(fsmMgr));
    }
  } else {
    Fsm_FsmWriteInitStub(fsmFsm, initStub);
  }

  if (constraint == NULL) {
    if (Fsm_MgrReadConstraintBDD(fsmMgr) != NULL) {
      Fsm_FsmWriteConstraint(fsmFsm, Fsm_MgrReadConstraintBDD(fsmMgr));
    }

  } else {
    Fsm_FsmWriteConstraint(fsmFsm, constraint);
  }

  if (Fsm_MgrReadJusticeBDD(fsmMgr) != NULL) {
    Fsm_FsmWriteJustice(fsmFsm, Fsm_MgrReadJusticeBDD(fsmMgr));
  }
  if (Fsm_MgrReadFairnessBDD(fsmMgr) != NULL) {
    Fsm_FsmWriteFairness(fsmFsm, Fsm_MgrReadFairnessBDD(fsmMgr));
  }

  if (Fsm_MgrReadLatchEqClassesBDD(fsmMgr) != NULL) {
    Fsm_FsmWriteLatchEqClasses(fsmFsm, Fsm_MgrReadLatchEqClassesBDD(fsmMgr));
  }

  if (Fsm_MgrReadConstrInvarBDD(fsmMgr) != NULL) {
    Fsm_FsmWriteConstrInvar(fsmFsm, Fsm_MgrReadConstrInvarBDD(fsmMgr));
  }

  if (initStubConstraint == NULL) {
    if (Fsm_MgrReadInitStubConstraintBDD(fsmMgr) != NULL) {
      Fsm_FsmWriteInitStubConstraint(fsmFsm,
        Fsm_MgrReadInitStubConstraintBDD(fsmMgr));
    }

  } else {
    Fsm_FsmWriteInitStubConstraint(fsmFsm, initStubConstraint);
  }

  if (cex == NULL) {
    if (Fsm_MgrReadCexBDD(fsmMgr) != NULL) {
      Fsm_FsmWriteCex(fsmFsm, Fsm_MgrReadCexBDD(fsmMgr));
    }

  } else {
    Fsm_FsmWriteCex(fsmFsm, cex);
  }

  if (invarspec == NULL) {
    if (Fsm_MgrReadInvarspecBDD(fsmMgr) != NULL) {
      Fsm_FsmWriteInvarspec(fsmFsm, Fsm_MgrReadInvarspecBDD(fsmMgr));
    }
  } else {
    Fsm_FsmWriteInvarspec(fsmFsm, invarspec);
  }


  Fsm_FsmSetNI(fsmFsm, Ddi_VararrayNum(fsmFsm->pi));
  Fsm_FsmSetNO(fsmFsm, Ddi_BddarrayNum(fsmFsm->lambda));
  Fsm_FsmSetNL(fsmFsm, Ddi_VararrayNum(fsmFsm->ps));

  //controllare i set se le variabili di ingresso/uscita/stato non sono NULL  
  if (addNewPis) {
    Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(fsmFsm->pi);
    Ddi_Varset_t *psv = Ddi_VarsetMakeFromArray(fsmFsm->ps);
    Ddi_Varset_t *fsmSupp = Ddi_BddarraySupp(fsmFsm->delta);
    Ddi_Varset_t *fsmSupp1 = Ddi_BddarraySupp(fsmFsm->lambda);

    if (Ddi_VarsetIsArray(fsmSupp)) {
      Ddi_VarsetSetArray(fsmSupp1);
    }

    Ddi_VarsetUnionAcc(fsmSupp, fsmSupp1);
    Ddi_Free(fsmSupp1);
    Ddi_VarsetSetArray(fsmSupp);
    Ddi_VarsetSetArray(piv);
    Ddi_VarsetSetArray(psv);
    Ddi_VarsetDiffAcc(fsmSupp, piv);
    Ddi_VarsetDiffAcc(fsmSupp, psv);
    Ddi_Free(piv);
    Ddi_Free(psv);
    if (Ddi_VarsetNum(fsmSupp) > 0) {
      Ddi_Vararray_t *newvA = Ddi_VararrayMakeFromVarset(fsmSupp, 1);

      Ddi_VararrayAppend(fsmFsm->pi, newvA);
      fsmFsm->size.i += Ddi_VararrayNum(newvA);
      Ddi_Free(newvA);
    }
    Ddi_Free(fsmSupp);
  }

/* if (Fsm_Fsm_ReadSettings (fsmFsm) != NULL) {
    Fsm_Fsm_WriteSettings(fsmFsmNew, Fsm_Fsm_ReadSettings (fsmFsm));
  }   

  if (Fsm_Fsm_ReadStats (fsmFsm) != NULL) {
    Fsm_Fsm_WriteStats(fsmFsmNew, Fsm_Fsm_ReadStats (fsmFsm));
    }*/

  if (fsmMgr->iFoldedProp>0) {
    int j;
    Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, 
			  "PDT_BDD_INVARSPEC_VAR$PS");
    for (j=Ddi_VararrayNum(fsmFsm->ps)-1; 
	 j>=0 && j>Ddi_VararrayNum(fsmFsm->ps)-5; j--) {
      if (Ddi_VararrayRead(fsmFsm->ps,j)==pvarPs) {
	fsmFsm->iFoldedProp = j;
      }
    }
  }
  if (fsmMgr->iFoldedConstr>0) {
    int j;
    Ddi_Var_t *cvarPs = Ddi_VarFromName(ddm, 
			  "PDT_BDD_INVAR_VAR$PS");
    for (j=Ddi_VararrayNum(fsmFsm->ps)-1; 
	 j>=0 && j>Ddi_VararrayNum(fsmFsm->ps)-5; j--) {
      if (Ddi_VararrayRead(fsmFsm->ps,j)==cvarPs) {
	fsmFsm->iFoldedConstr = j;
      }
    }
  }

  if (fsmMgr->iFoldedInit>0) {
    int j;
    Ddi_Var_t *ivarPs = Ddi_VarFromName(ddm, 
			  "PDT_BDD_INITSTUB_VAR$PS");
    for (j=Ddi_VararrayNum(fsmFsm->ps)-1; 
	 j>=0 && j>Ddi_VararrayNum(fsmFsm->ps)-5; j--) {
      if (Ddi_VararrayRead(fsmFsm->ps,j)==ivarPs) {
	fsmFsm->iFoldedInit = j;
      }
    }
  }

  return fsmFsm;
}


Fsm_Fsm_t *
Fsm_FsmMakeFromFsmMgr(
  Fsm_Mgr_t * fsmMgr
)
{

  return Fsm_FsmMake(fsmMgr, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

}

void
Fsm_FsmWriteToFsmMgr(
  Fsm_Mgr_t * fsmMgr,
  Fsm_Fsm_t * fsmFsm
)
{
  Ddi_Mgr_t *ddm = fsmMgr->dd;

  /*
   *  Variables ...
   */

  Fsm_MgrSetVarI(fsmMgr, Fsm_FsmReadPI(fsmFsm));
  Fsm_MgrSetVarPS(fsmMgr, Fsm_FsmReadPS(fsmFsm));
  Fsm_MgrSetVarNS(fsmMgr, Fsm_FsmReadNS(fsmFsm));

  /*
   *  BDDs
   */

  Fsm_MgrSetInitBDD(fsmMgr, Fsm_FsmReadInit(fsmFsm));
  Fsm_MgrSetInitStubBDD(fsmMgr, Fsm_FsmReadInitStub(fsmFsm));

  Fsm_MgrSetDeltaBDD(fsmMgr, Fsm_FsmReadDelta(fsmFsm));
  Fsm_MgrSetLambdaBDD(fsmMgr, Fsm_FsmReadLambda(fsmFsm));

  Fsm_MgrSetConstraintBDD(fsmMgr, Fsm_FsmReadConstraint(fsmFsm));
  Fsm_MgrSetJusticeBDD(fsmMgr, Fsm_FsmReadJustice(fsmFsm));
  Fsm_MgrSetFairnessBDD(fsmMgr, Fsm_FsmReadFairness(fsmFsm));
  Fsm_MgrSetInitStubConstraintBDD(fsmMgr,
    Fsm_FsmReadInitStubConstraint(fsmFsm));
  Fsm_MgrSetLatchEqClassesBDD(fsmMgr, Fsm_FsmReadLatchEqClasses(fsmFsm));
  Fsm_MgrSetConstrInvarBDD(fsmMgr, Fsm_FsmReadConstrInvar(fsmFsm));
  Fsm_MgrSetInvarspecBDD(fsmMgr, Fsm_FsmReadInvarspec(fsmFsm));

  if (fsmFsm->iFoldedProp>0) {
    int j;
    Ddi_Vararray_t *ps = fsmFsm->ps;
    Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, 
			  "PDT_BDD_INVARSPEC_VAR$PS");
    for (j=Ddi_VararrayNum(ps)-1; 
	 j>=0 && j>Ddi_VararrayNum(ps)-5; j--) {
      if (Ddi_VararrayRead(ps,j)==pvarPs) {
	fsmMgr->iFoldedProp = j;
      }
    }
  }
  if (fsmFsm->iFoldedConstr>0) {
    int j;
    Ddi_Vararray_t *ps = fsmFsm->ps;
    Ddi_Var_t *cvarPs = Ddi_VarFromName(ddm, 
			  "PDT_BDD_INVAR_VAR$PS");
    for (j=Ddi_VararrayNum(ps)-1; 
	 j>=0 && j>Ddi_VararrayNum(ps)-5; j--) {
      if (Ddi_VararrayRead(ps,j)==cvarPs) {
	fsmMgr->iFoldedConstr = j;
      }
    }
  }

  if (fsmFsm->iFoldedInit>0) {
    int j;
    Ddi_Vararray_t *ps = fsmFsm->ps;
    Ddi_Var_t *ivarPs = Ddi_VarFromName(ddm, 
			  "PDT_BDD_INITSTUB_VAR$PS");
    for (j=Ddi_VararrayNum(fsmFsm->ps)-1; 
	 j>=0 && j>Ddi_VararrayNum(fsmFsm->ps)-5; j--) {
      if (Ddi_VararrayRead(fsmFsm->ps,j)==ivarPs) {
	fsmMgr->iFoldedInit = j;
      }
    }
  }

}

int
Fsm_FsmCheckSizeConsistency(
  Fsm_Fsm_t * fsmFsm
)
{
  int alreadyConsistent = 1;

  int ffNum = Ddi_VararrayNum(fsmFsm->ps);
  int piNum = Ddi_VararrayNum(fsmFsm->pi);
  int poNum = Ddi_BddarrayNum(fsmFsm->lambda);

  if (fsmFsm->size.i != piNum) {
    alreadyConsistent = 0;
    Fsm_FsmSetNI(fsmFsm, piNum);
  }

  if (fsmFsm->size.o != poNum) {
    alreadyConsistent = 0;
    Fsm_FsmSetNO(fsmFsm, poNum);
  }

  if (fsmFsm->size.l != ffNum) {
    alreadyConsistent = 0;
    Fsm_FsmSetNL(fsmFsm, ffNum);
  }

  return alreadyConsistent;
}

void
Fsm_FsmNnfEncoding(
  Fsm_Fsm_t * fsmFsm
)
{
  if (fsmFsm->fsmMgr == NULL)
    return;

  Ddi_Mgr_t *ddm = fsmFsm->fsmMgr->dd;

  Ddi_Var_t *cvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
  Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
  Ddi_Var_t *cvarNs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$NS");
  Ddi_Var_t *pvarNs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$NS");
  Ddi_Bddarray_t *monotoneD, *newDelta;

  // The InitStub should be inserted before both INVAR and INVARSPEC
  int i, nl = Ddi_VararrayNum(fsmFsm->ps), isPosition = nl, nl0 = nl;

  Ddi_Var_t *var = Ddi_VararrayRead(fsmFsm->ps, nl - 1);

  Ddi_Vararray_t *rV = Ddi_VararrayAlloc(ddm, 0);
  Ddi_Vararray_t *aV = Ddi_VararrayAlloc(ddm, 0);
  Ddi_Vararray_t *aVNs, *psNew, *nsNew;
  Ddi_Bdd_t *constr, *prop;
  Ddi_Bddarray_t *myStub, *newStub;
  Ddi_Bddarray_t *fA = Ddi_BddarrayDup(fsmFsm->delta);

  if (fsmFsm->initStub != NULL) {
    myStub = Ddi_BddarrayDup(fsmFsm->initStub);
  } else {
    myStub = Ddi_BddarrayMakeLiteralsAig(fsmFsm->ps, 1);
    Ddi_AigarrayConstrainCubeAcc(myStub, fsmFsm->init);
  }

#if 0
  Pdtutil_Assert(var == pvarPs, "folded property needed");
  var = Ddi_VararrayRead(fsmFsm->ps, nl - 2);
  Pdtutil_Assert(var == cvarPs, "folded constr needed");
#endif

  Ddi_BddarrayInsertLast(fA, fsmFsm->invarspec);

  monotoneD = Ddi_AigarrayNnf(fA, fsmFsm->ps, NULL, NULL, rV, aV, NULL);

  Ddi_Free(fA);

  Ddi_DataCopy(fsmFsm->invarspec,
    Ddi_BddarrayRead(monotoneD, Ddi_BddarrayNum(monotoneD) - 2));
  Ddi_BddarrayWrite(fsmFsm->lambda, 0, fsmFsm->invarspec);
  Ddi_BddarrayWrite(fsmFsm->lambda, 1,
    Ddi_BddarrayRead(monotoneD, Ddi_BddarrayNum(monotoneD) - 1));
  Ddi_BddNotAcc(Ddi_BddarrayRead(fsmFsm->lambda, 1));

  Ddi_BddarrayRemove(monotoneD, Ddi_BddarrayNum(monotoneD) - 1);
  Ddi_BddarrayRemove(monotoneD, Ddi_BddarrayNum(monotoneD) - 1);

  Pdtutil_Assert(Ddi_VararrayNum(rV) == nl, "missing monotone var in array");

  Pdtutil_Assert(Ddi_AigarrayCheckMonotone(monotoneD, rV), "not monotone");
  Pdtutil_Assert(Ddi_AigarrayCheckMonotone(monotoneD, aV), "not monotone");



  if (0) {
    Ddi_Vararray_t *a = Ddi_VararrayAlloc(ddm, 2);
    Ddi_Bddarray_t *s = Ddi_BddarrayAlloc(ddm, 2);
    Ddi_Bdd_t *lit;

    Ddi_VararrayWrite(a, 0, Ddi_VararrayRead(aV, nl - 2));
    Ddi_VararrayWrite(a, 1, Ddi_VararrayRead(aV, nl - 1));
    lit = Ddi_BddMakeLiteralAig(Ddi_VararrayRead(rV, nl - 2), 0);
    Ddi_BddarrayWrite(s, 0, lit);
    Ddi_Free(lit);
    lit = Ddi_BddMakeLiteralAig(Ddi_VararrayRead(rV, nl - 1), 0);
    Ddi_BddarrayWrite(s, 1, lit);
    Ddi_Free(lit);
    Ddi_BddarrayComposeAcc(monotoneD, a, s);
    Ddi_Free(a);
    Ddi_Free(s);
  }

  nl = Ddi_BddarrayNum(monotoneD);
  newDelta = Ddi_BddarrayDup(monotoneD);

  // move prop/constr to last position
  prop = Ddi_BddarrayRead(monotoneD, nl - 2);
  constr = Ddi_BddarrayRead(monotoneD, nl - 4);
  Ddi_BddarrayWrite(newDelta, nl - 1, prop);
  Ddi_BddarrayWrite(newDelta, nl - 2, constr);
  // then negated prop/constr
  prop = Ddi_BddarrayRead(monotoneD, nl - 1);
  constr = Ddi_BddarrayRead(monotoneD, nl - 3);
  Ddi_BddarrayWrite(newDelta, nl - 3, prop);
  Ddi_BddarrayWrite(newDelta, nl - 4, constr);


  Ddi_DataCopy(fsmFsm->delta, newDelta);
  Ddi_Free(monotoneD);
  Ddi_Free(newDelta);

  aVNs = Ddi_VararrayMakeNewVars(fsmFsm->ps, "PDTRAV_NNF_RAIL_0", "$NS", 1);

  psNew = Ddi_VararrayAlloc(ddm, nl);
  nsNew = Ddi_VararrayAlloc(ddm, nl);

  for (i = 0; i < nl0 - 2; i++) {
    Ddi_Var_t *ps_i = Ddi_VararrayRead(fsmFsm->ps, i);
    Ddi_Var_t *ns_i = Ddi_VararrayRead(fsmFsm->ns, i);
    Ddi_Var_t *psAux_i = Ddi_VararrayRead(aV, i);
    Ddi_Var_t *nsAux_i = Ddi_VararrayRead(aVNs, i);

    Ddi_VararrayWrite(psNew, 2 * i, ps_i);
    Ddi_VararrayWrite(nsNew, 2 * i, ns_i);
    Ddi_VararrayWrite(psNew, 2 * i + 1, psAux_i);
    Ddi_VararrayWrite(nsNew, 2 * i + 1, nsAux_i);
  }
  // prop vars
  Ddi_VararrayWrite(psNew, nl - 1, Ddi_VararrayRead(fsmFsm->ps, nl0 - 1));
  Ddi_VararrayWrite(psNew, nl - 3, Ddi_VararrayRead(aV, nl0 - 1));
  Ddi_VararrayWrite(nsNew, nl - 1, Ddi_VararrayRead(fsmFsm->ns, nl0 - 1));
  Ddi_VararrayWrite(nsNew, nl - 3, Ddi_VararrayRead(aVNs, nl0 - 1));
  // constr vars
  Ddi_VararrayWrite(psNew, nl - 2, Ddi_VararrayRead(fsmFsm->ps, nl0 - 2));
  Ddi_VararrayWrite(psNew, nl - 4, Ddi_VararrayRead(aV, nl0 - 2));
  Ddi_VararrayWrite(nsNew, nl - 2, Ddi_VararrayRead(fsmFsm->ns, nl0 - 2));
  Ddi_VararrayWrite(nsNew, nl - 4, Ddi_VararrayRead(aVNs, nl0 - 2));

  Ddi_DataCopy(fsmFsm->ps, psNew);
  Ddi_DataCopy(fsmFsm->ns, nsNew);
  Ddi_Free(psNew);
  Ddi_Free(nsNew);

  Ddi_Free(aVNs);

  newStub = Ddi_BddarrayAlloc(ddm, nl);
  for (i = 0; i < nl0; i++) {
    Ddi_Bdd_t *s_i = Ddi_BddarrayRead(myStub, i);
    Ddi_Bdd_t *s_i_bar = Ddi_BddNot(Ddi_BddarrayRead(myStub, i));

    Ddi_BddarrayWrite(newStub, 2 * i, s_i);
    Ddi_BddarrayWrite(newStub, 2 * i + 1, s_i_bar);
    Ddi_Free(s_i_bar);
  }

  Pdtutil_Assert(Ddi_BddarrayNum(newStub) == nl, "wrong stub size");
  prop = Ddi_BddarrayRead(myStub, nl0 - 1);
  constr = Ddi_BddarrayRead(myStub, nl0 - 2);
  Ddi_BddarrayWrite(newStub, nl - 1, prop);
  Ddi_BddarrayWrite(newStub, nl - 2, constr);

  Ddi_BddNotAcc(prop);
  Ddi_BddNotAcc(constr);
  Ddi_BddarrayWrite(newStub, nl - 3, prop);
  Ddi_BddarrayWrite(newStub, nl - 4, constr);

  Ddi_Free(myStub);

  if (fsmFsm->initStub != NULL) {
    Ddi_DataCopy(fsmFsm->initStub, newStub);
  } else {
    Ddi_Bdd_t *newInit = Ddi_BddMakeConstAig(ddm, 1);

    Ddi_DataCopy(fsmFsm->init, newInit);
    Ddi_Free(newInit);
    for (i = 0; i < nl; i++) {
      Ddi_Bdd_t *s_i = Ddi_BddarrayRead(newStub, i);
      Ddi_Var_t *ps_i = Ddi_VararrayRead(fsmFsm->ps, i);
      Ddi_Bdd_t *i_i = Ddi_BddMakeLiteralAig(ps_i, Ddi_BddIsOne(s_i));

      Ddi_BddAndAcc(fsmFsm->init, i_i);
      Ddi_Free(i_i);
    }
  }

  Ddi_Free(newStub);

}

int
Fsm_FsmZeroInit(
  Fsm_Fsm_t * fsmFsm
)
{
  if (fsmFsm->fsmMgr == NULL)
    return 0;

  if (fsmFsm->initStub != NULL)
    return 0;

  Ddi_Mgr_t *ddm = fsmFsm->fsmMgr->dd;

  // The InitStub should be inserted before both INVAR and INVARSPEC
  int nl = Ddi_VararrayNum(fsmFsm->ps), isPosition = nl;
  int i = 0, j, ndelta = Ddi_BddarrayNum(fsmFsm->delta);
  int nlambda = Ddi_BddarrayNum(fsmFsm->lambda);
  Ddi_Bdd_t *initPart = Ddi_AigPartitionTop(fsmFsm->init, 0);
  int nFlip = 0;
  Ddi_Vararray_t *vA;
  Ddi_Bddarray_t *vSubst;

  for (i = 0; i < Ddi_BddPartNum(initPart); i++) {
    Ddi_Bdd_t *l_i = Ddi_BddPartRead(initPart, i);

    Pdtutil_Assert(Ddi_BddSize(l_i) == 1, "init is not a cube");
    if (!Ddi_BddIsComplement(l_i)) {
      Ddi_Var_t *v = Ddi_BddTopVar(l_i);

      Ddi_VarWriteMark(v, 1);
      Ddi_BddNotAcc(l_i);
      nFlip++;
    }
  }

  if (nFlip == 0) {
    Ddi_Free(initPart);
    return 0;
  }

  Ddi_BddSetAig(initPart);
  Ddi_DataCopy(fsmFsm->init, initPart);
  Ddi_Free(initPart);

  vA = Ddi_VararrayAlloc(ddm, nFlip);
  vSubst = Ddi_BddarrayAlloc(ddm, nFlip);

  for (i = j = 0; i < Ddi_VararrayNum(fsmFsm->ps); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(fsmFsm->ps, i);

    if (Ddi_VarReadMark(v) != 0) {
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(fsmFsm->delta, i);
      Ddi_Bdd_t *l = Ddi_BddMakeLiteralAig(v, 0);

      Ddi_VarWriteMark(v, 0);
      Ddi_BddNotAcc(d_i);       // complement delta
      Ddi_VararrayWrite(vA, j, v);
      Ddi_BddarrayWrite(vSubst, j, l);
      j++;
      Ddi_Free(l);
    }
  }

  Pdtutil_Assert(j == nFlip, "mismatch in num of flipped state vars");

  Ddi_BddarrayComposeAcc(fsmFsm->delta, vA, vSubst);
  Ddi_BddarrayComposeAcc(fsmFsm->lambda, vA, vSubst);
  Ddi_BddComposeAcc(fsmFsm->constraint, vA, vSubst);
  Ddi_BddComposeAcc(fsmFsm->invarspec, vA, vSubst);

  Ddi_Free(vA);
  Ddi_Free(vSubst);

  return nFlip;
}

void
Fsm_FsmFoldInit(
  Fsm_Fsm_t * fsmFsm
)
{
  if (fsmFsm->fsmMgr == NULL)
    return;
  Ddi_Mgr_t *ddiMgr = fsmFsm->fsmMgr->dd;

  Ddi_Var_t *initStubPs = NULL, *initStubNs = NULL;
  Ddi_Vararray_t *stubVarsA;
  int foldedConstr = -1;
  Ddi_Bdd_t *constraint = NULL;
  int chk = 0, doGroup = 0;
  
  if (fsmFsm->initStub == NULL) {
    int nv = Ddi_VararrayNum(fsmFsm->ps);
    Ddi_Vararray_t *ps = fsmFsm->ps;
    Ddi_Vararray_t *stubVars = Ddi_VararrayMakeNewAigVars(ps,
                                 "PDT_STUB_PS",0);
    fsmFsm->initStub = Ddi_BddarrayMakeLiteralsAig(stubVars, 1);
    Ddi_VararrayWriteMarkWithIndex (ps, 0);
    Ddi_Bdd_t *initPart = Ddi_AigPartitionTop(fsmFsm->init, 0);
    for (int i=0; i<Ddi_BddPartNum(initPart); i++) {
      Ddi_Bdd_t *l_i = Ddi_BddPartRead(initPart,i);
      Ddi_Var_t *v = Ddi_BddTopVar(l_i);
      Ddi_Bdd_t *val = Ddi_BddMakeConstAig(ddiMgr,
                         Ddi_BddIsComplement(l_i)?0:1);
      int j = Ddi_VarReadMark(v);
      Pdtutil_Assert(j>=0&&j<nv,"");
      Ddi_BddarrayWrite(fsmFsm->initStub,j,val);
      Ddi_Free(val);
    }
    Ddi_VararrayWriteMark (ps, 0);
#if 0
    // already merged to pis
    Ddi_Vararray_t *newPis = Ddi_BddarraySuppVararray(fsmFsm->initStub);
    Ddi_VararrayAppend(fsmFsm->pi,newPis);
    Ddi_Free(newPis);
#endif    
    Ddi_Free(initPart);
    Ddi_Free(stubVars);
    Ddi_Lock(fsmFsm->initStub);
  }

  
  Ddi_Var_t *cvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
  Ddi_Var_t *pvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");

  initStubPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INITSTUB_VAR$PS");

  if (initStubPs != NULL) {
    int is = Ddi_VararraySearchVarDown(fsmFsm->ps, initStubPs);

    if (is >= 0) {
      // already done
      return;
    }
  }
  if (initStubPs == NULL) {
    initStubPs = Ddi_VarNewAtLevel(ddiMgr, 0);
    Ddi_VarAttachName(initStubPs, "PDT_BDD_INITSTUB_VAR$PS");
  }

  if (initStubNs == NULL) {
    initStubNs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INITSTUB_VAR$NS");
    if (initStubNs == NULL) {
      initStubNs = Ddi_VarNewAtLevel(ddiMgr, 1);
      Ddi_VarAttachName(initStubNs, "PDT_BDD_INITSTUB_VAR$NS");
      doGroup = 1;
    }
  }

  if (doGroup)
    Ddi_VarMakeGroup(ddiMgr, initStubPs, 2);

  // The InitStub should be inserted before both INVAR and INVARSPEC
  int nl = Ddi_VararrayNum(fsmFsm->ps), isPosition = nl;

  if (nl > 0) {
    Ddi_Var_t *var = Ddi_VararrayRead(fsmFsm->ps, nl - 1);

    if (var == cvarPs) {
      isPosition = nl - 1;
      foldedConstr = isPosition+1;
    } else if (var == pvarPs) {
      isPosition = nl - 1;
      foldedConstr = isPosition+1;
      if (nl > 1) {
        var = Ddi_VararrayRead(fsmFsm->ps, nl - 2);
        if (var == cvarPs) {
          isPosition = nl - 2;
          foldedConstr = isPosition+1;
        }
      }
    }
  }

  Ddi_VararrayInsert(fsmFsm->ps, isPosition, initStubPs);
  Ddi_VararrayInsert(fsmFsm->ns, isPosition, initStubNs);

  fsmFsm->iFoldedInit = isPosition;

  fsmFsm->size.l++;
  nl++;

  Ddi_Bdd_t *newConstDelta = Ddi_BddMakeConstAig(ddiMgr, 1);
  Ddi_Bdd_t *newDeltaLit = Ddi_BddMakeLiteralAig(initStubPs, 1);

  Ddi_BddarrayInsert(fsmFsm->delta, isPosition, newConstDelta);
  Ddi_BddarrayInsert(fsmFsm->initStub, isPosition, newConstDelta);

  int i = 0, ndelta = Ddi_BddarrayNum(fsmFsm->delta);
  int nlambda = Ddi_BddarrayNum(fsmFsm->lambda);

  stubVarsA = Ddi_BddarraySuppVararray(fsmFsm->initStub);
  if (fsmFsm->initStubConstraint != NULL) {
    Ddi_Vararray_t *sA = Ddi_BddSuppVararray(fsmFsm->initStubConstraint);

    Ddi_VararrayUnionAcc(stubVarsA, sA);
    Ddi_Free(sA);
  }
  if (Ddi_VararrayNum(stubVarsA) > Ddi_VararrayNum(fsmFsm->pi)) {
    for (i = Ddi_VararrayNum(fsmFsm->pi); i < Ddi_VararrayNum(stubVarsA); i++) {
      Ddi_VararrayWrite(fsmFsm->pi, i, Ddi_VararrayRead(stubVarsA, i));
    }
  } else {
    for (i = Ddi_VararrayNum(stubVarsA); i < Ddi_VararrayNum(fsmFsm->pi); i++) {
      Ddi_VararrayWrite(stubVarsA, i, Ddi_VararrayRead(fsmFsm->pi, i));
    }
  }
  fsmFsm->is = Ddi_VararrayDup(stubVarsA);
  Ddi_Lock(fsmFsm->is);

  Ddi_BddarraySubstVarsAcc(fsmFsm->initStub, stubVarsA, fsmFsm->pi);
  if (fsmFsm->initStubConstraint != NULL) {
    Ddi_BddSubstVarsAcc(fsmFsm->initStubConstraint, stubVarsA, fsmFsm->pi);
  }

  fsmFsm->size.i = Ddi_VararrayNum(fsmFsm->pi);
  Ddi_Free(stubVarsA);

  if (foldedConstr >= 0) {
    constraint = Ddi_BddarrayRead(fsmFsm->delta, foldedConstr);
  } else if (fsmFsm->constraint != NULL) {
    constraint = fsmFsm->constraint;
  }

  if (constraint != NULL) {
    Ddi_Bdd_t *initStubConstr = fsmFsm->initStubConstraint;

    if (initStubConstr == NULL) {
      Ddi_BddNotAcc(newDeltaLit);
      Ddi_BddOrAcc(constraint, newDeltaLit);
      Ddi_BddNotAcc(newDeltaLit);
    } else {
      Ddi_Bdd_t *ddiNewITE = Ddi_BddIte(newDeltaLit,
        constraint, initStubConstr);

      Ddi_DataCopy(constraint, ddiNewITE);
      Ddi_Free(ddiNewITE);
    }
  }
  // Replace all deltas but the InitStub constant one
  for (i = 0; i < ndelta; i++) {
    if (i == isPosition)
      continue;
    Ddi_Bdd_t *d_i = Ddi_BddarrayRead(fsmFsm->delta, i);
    Ddi_Bdd_t *is_i = Ddi_BddarrayRead(fsmFsm->initStub, i);
    Ddi_Bdd_t *ddiNewITE = Ddi_BddIte(newDeltaLit, d_i, is_i);

    Ddi_BddarrayWrite(fsmFsm->delta, i, ddiNewITE);
    Ddi_Free(ddiNewITE);
  }

  Ddi_Bdd_t *initStubLit = Ddi_BddMakeLiteralAig(initStubPs, 1);

  // LAMBDAS are PROP here (NOT Target !!!)
  Ddi_BddNotAcc(initStubLit);
  for (i = 0; i < nlambda; i++) {
    Ddi_Bdd_t *l_i = Ddi_BddarrayRead(fsmFsm->lambda, i);

    Ddi_BddOrAcc(l_i, initStubLit);
  }
  if (fsmFsm->invarspec != NULL) {
    Ddi_BddOrAcc(fsmFsm->invarspec, initStubLit);
  }
  if (0 && fsmFsm->constraint != NULL) {
    // constr need to be active
    Ddi_BddOrAcc(fsmFsm->constraint, initStubLit);
  }
  Ddi_Free(initStubLit);

  // Initial state definition, for now all 0s
  Ddi_Unlock(fsmFsm->init);
  Ddi_Free(fsmFsm->init);
  fsmFsm->init = Ddi_BddMakeConstAig(ddiMgr, 1);

  for (i = 0; i < nl; i++) {
    Ddi_Var_t *var = Ddi_VararrayRead(fsmFsm->ps, i);
    Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(var, 0);

    if (var == pvarPs || var == cvarPs) {
      Ddi_BddNotAcc(lit);
    }
    Ddi_BddAndAcc(fsmFsm->init, lit);
    Ddi_Free(lit);
  }
  Ddi_Lock(fsmFsm->init);

  if (chk) {
    Ddi_Bddarray_t *delta2 = Ddi_BddarrayDup(fsmFsm->delta);

    Ddi_AigarrayConstrainCubeAcc(delta2, fsmFsm->init);
    for (i = 0; i < Ddi_BddarrayNum(delta2); i++) {
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta2, i);
      Ddi_Bdd_t *s_i = Ddi_BddarrayRead(fsmFsm->initStub, i);
      if (i==isPosition) continue;
      Pdtutil_Assert(Ddi_AigEqualSat(s_i, d_i), "error");
    }
    Ddi_Free(delta2);
  }
  // Destroy IS??? 
  Ddi_Unlock(fsmFsm->initStub);
  Ddi_Free(fsmFsm->initStub);

  Ddi_Free(newConstDelta);
  Ddi_Free(newDeltaLit);

}

void
Fsm_FsmFoldInitWithPi(
  Fsm_Fsm_t * fsmFsm
)
{
  if (fsmFsm->fsmMgr == NULL)
    return;

  Ddi_Mgr_t *ddiMgr = fsmFsm->fsmMgr->dd;
  Fsm_Mgr_t *fsmMgr = fsmFsm->fsmMgr;

  Ddi_Var_t *initStubPi = NULL;
  Ddi_Vararray_t *stubVarsA;
  int foldedConstr = -1;
  Ddi_Bdd_t *constraint = NULL;
  int chk = 1, doGroup = 0;

  Ddi_Var_t *cvarPs = Fsm_MgrReadPdtConstrVar(fsmMgr);
  Ddi_Var_t *pvarPs = Fsm_MgrReadPdtSpecVar(fsmMgr);

  initStubPi = Ddi_VarFromName(ddiMgr, "PDT_BDD_INITSTUB_VAR$PI");

  if (initStubPi != NULL) {
    int is = Ddi_VararraySearchVarDown(fsmFsm->pi, initStubPi);

    if (is >= 0) {
      // already done
      return;
    }
  }

  if (initStubPi == NULL) {
    initStubPi = Ddi_VarNewAtLevel(ddiMgr, 0);
    Ddi_VarAttachName(initStubPi, "PDT_BDD_INITSTUB_VAR$PI");
  }

  stubVarsA = Ddi_BddarraySuppVararray(fsmFsm->initStub);
  if (fsmFsm->initStubConstraint != NULL) {
    Ddi_Vararray_t *sA = Ddi_BddSuppVararray(fsmFsm->initStubConstraint);

    Ddi_VararrayUnionAcc(stubVarsA, sA);
    Ddi_Free(sA);
  }
  Ddi_VararrayAppend(fsmFsm->pi, stubVarsA);
  Ddi_VararrayInsertLast(fsmFsm->pi, initStubPi);

  fsmFsm->size.i = Ddi_VararrayNum(fsmFsm->pi);
  Ddi_Free(stubVarsA);

  if (foldedConstr >= 0) {
    constraint = Ddi_BddarrayRead(fsmFsm->delta, foldedConstr);
  } else if (fsmFsm->constraint != NULL) {
    constraint = fsmFsm->constraint;
  }

  Ddi_Bdd_t *newDeltaLit = Ddi_BddMakeLiteralAig(initStubPi, 1);
  Ddi_Bdd_t *initStubLit = Ddi_BddMakeLiteralAig(initStubPi, 0);

  if (constraint != NULL) {
    Ddi_Bdd_t *initStubConstr = fsmFsm->initStubConstraint;

    if (initStubConstr == NULL) {
      Ddi_BddOrAcc(constraint, newDeltaLit);
    } else {
      Ddi_Bdd_t *ddiNewITE = Ddi_BddIte(newDeltaLit,
        constraint, initStubConstr);

      Ddi_DataCopy(constraint, ddiNewITE);
      Ddi_Free(ddiNewITE);
    }
  }
  // Replace all deltas with muxes

  int i = 0, ndelta = Ddi_BddarrayNum(fsmFsm->delta);
  int nlambda = Ddi_BddarrayNum(fsmFsm->lambda);

  for (i = 0; i < ndelta; i++) {
    Ddi_Bdd_t *d_i = Ddi_BddarrayRead(fsmFsm->delta, i);
    Ddi_Bdd_t *is_i = Ddi_BddarrayRead(fsmFsm->initStub, i);
    Ddi_Bdd_t *ddiNewITE = Ddi_BddIte(newDeltaLit, d_i, is_i);

    Ddi_BddarrayWrite(fsmFsm->delta, i, ddiNewITE);
    Ddi_Free(ddiNewITE);
  }

  for (i = 0; i < nlambda; i++) {
    Ddi_Bdd_t *l_i = Ddi_BddarrayRead(fsmFsm->lambda, i);

    Ddi_BddOrAcc(l_i, initStubLit);
  }
  if (fsmFsm->invarspec != NULL) {
    Ddi_BddOrAcc(fsmFsm->invarspec, initStubLit);
  }
  Ddi_Free(initStubLit);

  // Initial state definition, for now all 0s
  Ddi_Unlock(fsmFsm->init);
  Ddi_Free(fsmFsm->init);
  fsmFsm->init = Ddi_BddMakeConstAig(ddiMgr, 1);

  for (i = 0; i < Ddi_VararrayNum(fsmFsm->ps); i++) {
    Ddi_Var_t *var = Ddi_VararrayRead(fsmFsm->ps, i);
    Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(var, 0);

    if (var == pvarPs || var == cvarPs) {
      Ddi_BddNotAcc(lit);
    }
    Ddi_BddAndAcc(fsmFsm->init, lit);
    Ddi_Free(lit);
  }
  Ddi_Lock(fsmFsm->init);

  if (chk) {
    Ddi_Bddarray_t *delta2 = Ddi_BddarrayDup(fsmFsm->delta);

    Ddi_BddarrayCofactorAcc(delta2, initStubPi, 0);
    //    Ddi_AigarrayConstrainCubeAcc(delta2,fsmFsm->init);
    for (i = 0; i < Ddi_BddarrayNum(delta2); i++) {
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta2, i);
      Ddi_Bdd_t *s_i = Ddi_BddarrayRead(fsmFsm->initStub, i);

      Pdtutil_Assert(Ddi_AigEqualSat(s_i, d_i), "error");
    }
    Ddi_Free(delta2);
  }

  Ddi_Unlock(fsmFsm->initStub);
  Ddi_Free(fsmFsm->initStub);

  Ddi_Free(newDeltaLit);

}

void
Fsm_FsmFoldProperty(
  Fsm_Fsm_t * fsmFsm,
  int compl_invarspec,
  int cntReachedOK,
  int foldLambda
)
{
  if (fsmFsm->invarspec == NULL)
    return;

  if (fsmFsm->fsmMgr == NULL)
    return;

  Ddi_Mgr_t *ddiMgr = fsmFsm->fsmMgr->dd;
  int nl = Fsm_MgrReadNL(fsmFsm->fsmMgr);

  Ddi_Bdd_t *newLambdaBdd, *property;

  Ddi_Var_t *lambdaPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
  Ddi_Var_t *lambdaNs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$NS");

  if (lambdaPs == NULL) {
    lambdaPs = Ddi_VarNewAtLevel(ddiMgr, 0);
    Ddi_VarAttachName(lambdaPs, "PDT_BDD_INVARSPEC_VAR$PS");
  }
  if (lambdaNs == NULL) {
    int xlevel = Ddi_VarCurrPos(lambdaPs) + 1;

    lambdaNs = Ddi_VarNewAtLevel(ddiMgr, xlevel);
    Ddi_VarAttachName(lambdaNs, "PDT_BDD_INVARSPEC_VAR$NS");
  }
  //  Ddi_VarMakeGroup (ddiMgr, lambdaPs, 2);

  Pdtutil_Assert(fsmFsm->iFoldedProp<0,"prop folding: already folded");
  fsmFsm->iFoldedProp = Ddi_VararrayNum(fsmFsm->ns);

  // The property is inserted always in the last position 
  Ddi_VararrayInsertLast(fsmFsm->ps, lambdaPs);
  Ddi_VararrayInsertLast(fsmFsm->ns, lambdaNs);

  if (!Ddi_BddIsMono(fsmFsm->invarspec)) {
    newLambdaBdd = Ddi_BddMakeLiteralAig(lambdaPs, 1);
  } else {
    newLambdaBdd = Ddi_BddMakeLiteral(lambdaPs, 1);
  }

  if (compl_invarspec) {
    Ddi_BddNotAcc(fsmFsm->invarspec);
    // opt->compl_invarspec = 0;
  }

  property = Ddi_BddDup(fsmFsm->invarspec);

  if (Ddi_BddIsPartConj(fsmFsm->invarspec)) {
    Ddi_BddSetAig(property);
  }
  Ddi_BddAndAcc(property, newLambdaBdd);
  Ddi_BddarrayInsertLast(fsmFsm->delta, property);
  if (foldLambda)
    Ddi_BddarrayWrite(fsmFsm->lambda, 0, newLambdaBdd);
  Ddi_DataCopy(fsmFsm->invarspec, newLambdaBdd);

  if (fsmFsm->initStub != NULL) {
    Ddi_Bdd_t *myOne = Ddi_BddMakeConstAig(ddiMgr, 1);

    Ddi_BddarrayInsertLast(fsmFsm->initStub, myOne);
    Ddi_Free(myOne);
  } else {
    Ddi_BddAndAcc(fsmFsm->init, newLambdaBdd);
  }

  Ddi_Free(property);

  Ddi_Unlock(fsmFsm->invarspec);
  Ddi_Free(fsmFsm->invarspec);
  fsmFsm->invarspec = newLambdaBdd;
  Ddi_Lock(fsmFsm->invarspec);
}

void
Fsm_FsmAddPropertyWitnesses(
  Fsm_Fsm_t * fsmFsm
)
{
  if (fsmFsm->invarspec == NULL)
    return;

  if (fsmFsm->fsmMgr == NULL)
    return;

  Ddi_Mgr_t *ddiMgr = fsmFsm->fsmMgr->dd;
  int nl = Fsm_MgrReadNL(fsmFsm->fsmMgr);

  Ddi_Bdd_t *newD0, *newD1, *property;
  Ddi_Var_t *newvPs0, *newvNs0;
  Ddi_Var_t *newvPs1, *newvNs1;
  char *name0p = "PDT_BDD_PROP_WITNESS_0_VAR$PS";
  char *name0n = "PDT_BDD_PROP_WITNESS_0_VAR$NS";
  char *name1p = "PDT_BDD_PROP_WITNESS_1_VAR$PS";
  char *name1n = "PDT_BDD_PROP_WITNESS_1_VAR$NS";
  
  Pdtutil_Assert(fsmFsm->iFoldedProp<0,"prop folding: already folded");

  int isBdd = Ddi_VarIsBdd(Ddi_VararrayRead(fsmFsm->ps,0));
  newvPs0 = Ddi_VarFindOrAdd(ddiMgr,name0p,isBdd);
  newvNs0 = Ddi_VarFindOrAdd(ddiMgr,name0n,isBdd);
  newvPs1 = Ddi_VarFindOrAdd(ddiMgr,name1p,isBdd);
  newvNs1 = Ddi_VarFindOrAdd(ddiMgr,name1n,isBdd);

  // The property is inserted always in the last position 

  property = Ddi_BddDup(fsmFsm->invarspec);

  Ddi_Bdd_t *cPart = Ddi_AigPartitionTop(property, 0);
  Ddi_BddPartSortBySizeAcc(cPart,0); // decreasing size
  Ddi_Bdd_t *p0c = Ddi_BddPartRead(cPart,0);
  Ddi_Bdd_t *dPart = Ddi_AigPartitionTop(p0c, 1);
  Ddi_BddPartSortBySizeAcc(dPart,0); // decreasing size
  int custom = 0;
  if (custom) {
    Ddi_BddPartRemove(dPart,2);
    Ddi_BddPartRemove(dPart,1);
    Ddi_BddPartRemove(dPart,0);
  }
  Ddi_Bdd_t *p0 = Ddi_BddDup(dPart);
  Ddi_Bdd_t *p1 = Ddi_BddPartExtract(p0,0);
  Ddi_Bdd_t *p1Part = Ddi_AigPartitionTop(p1, 0);
  Ddi_BddPartSortByTopVarAcc(p1Part,1);
  Ddi_BddSetAig(p1Part);
  Ddi_BddSetAig(p0);
  Ddi_BddSetAig(p1);
  Ddi_Bdd_t *newProp = Ddi_BddMakeLiteralAig(newvPs0, 1);
  Ddi_Bdd_t *newProp1 = Ddi_BddMakeLiteralAig(newvPs1, 1);
  Ddi_BddOrAcc(newProp,newProp1);
  //  Ddi_BddSetAig(dPart);
  Ddi_DataCopy(p0c,newProp);

  Ddi_BddSetAig(cPart);
  Ddi_DataCopy(fsmFsm->invarspec,cPart);
  Ddi_Free(newProp);
  Ddi_Free(newProp1);
  Ddi_Free(cPart);
  Ddi_Free(dPart);

  if (0) {
    Ddi_Vararray_t *vA, *vA1, *newPiVars;
      
    vA = Ddi_BddSuppVararray(p0);
    vA1 = Ddi_BddSuppVararray(p1);
    Ddi_VararrayUnionAcc(vA,vA1);
    Ddi_Free(vA1);
    Ddi_VararrayIntersectAcc(vA,fsmFsm->pi);
    newPiVars = Ddi_VararrayMakeNewVars(vA, "PDT_TE_STUB_PI", "0", 1);
    Ddi_BddSubstVarsAcc(p0,vA,newPiVars);
    Ddi_BddSubstVarsAcc(p1,vA,newPiVars);
    Ddi_VararrayAppend(fsmFsm->pi,newPiVars);
    Ddi_Free(newPiVars);
  }
#if 0  
  newD0 = Ddi_BddCompose(p0,fsmFsm->ps,fsmFsm->delta);
  newD1 = Ddi_BddCompose(p1,fsmFsm->ps,fsmFsm->delta);
#else
  newD0 = Ddi_BddDup(p0);
  newD1 = Ddi_BddDup(p1);
#endif

  Ddi_BddarrayInsertLast(fsmFsm->delta, newD0);
  Ddi_BddarrayInsertLast(fsmFsm->delta, newD1);

  if (fsmFsm->initStub != NULL) {
    Ddi_Bdd_t *newS0 = Ddi_BddCompose(p0,fsmFsm->ps,fsmFsm->initStub);
    Ddi_Bdd_t *newS1 = Ddi_BddCompose(p1,fsmFsm->ps,fsmFsm->initStub);
    Ddi_BddarrayInsertLast(fsmFsm->initStub, newS0);
    Ddi_BddarrayInsertLast(fsmFsm->initStub, newS1);
    Ddi_Free(newS0);
    Ddi_Free(newS1);
  } else {
    int isAig = !Ddi_BddIsMono(fsmFsm->init);
    Ddi_Bdd_t *newLit0 = Ddi_BddMakeLiteralAigOrBdd(newvPs0, 1, isAig);
    Ddi_Bdd_t *newLit1 = Ddi_BddMakeLiteralAigOrBdd(newvPs1, 1, isAig);
    Ddi_BddAndAcc(fsmFsm->init, newLit0);
    Ddi_BddAndAcc(fsmFsm->init, newLit1);
    Ddi_Free(newLit0);
    Ddi_Free(newLit1);
  }
  
  Ddi_VararrayInsertLast(fsmFsm->ps, newvPs0);
  Ddi_VararrayInsertLast(fsmFsm->ns, newvNs0);
  Ddi_VararrayInsertLast(fsmFsm->ps, newvPs1);
  Ddi_VararrayInsertLast(fsmFsm->ns, newvNs1);
                         
  Ddi_Free(p0);
  Ddi_Free(p1);

  Ddi_Free(property);
  Ddi_Free(newD0);
  Ddi_Free(newD1);
}

void
Fsm_FsmDeltaWithConstraint(
  Fsm_Fsm_t * fsmFsm
)
{
  Ddi_Mgr_t *ddiMgr = fsmFsm->fsmMgr->dd;
  Ddi_Var_t *cvarPs = Fsm_MgrReadPdtConstrVar(fsmFsm->fsmMgr);
  int j, constPosition = fsmFsm->iFoldedConstr;
  Ddi_Bdd_t *cLit;

  if (fsmFsm->constraint == NULL)
    return;
  if (fsmFsm->fsmMgr == NULL)
    return;
  if (cvarPs==NULL) return;
  if (constPosition<0) return;

  cLit = Ddi_BddMakeLiteralAig(cvarPs, 0);

  for (j=0;j<constPosition;j++) {
    Ddi_Bdd_t *d_j = Ddi_BddarrayRead(fsmFsm->delta,j); 
    Ddi_BddOrAcc(d_j, cLit);
  }
  Ddi_Free(cLit);
}

void
Fsm_FsmFoldConstraint(
  Fsm_Fsm_t * fsmFsm,
  int compl_invarspec
)
{

  if (fsmFsm->constraint == NULL)
    return;

  if (fsmFsm->fsmMgr == NULL)
    return;

  Ddi_Mgr_t *ddiMgr = fsmFsm->fsmMgr->dd;

  Ddi_Var_t *isvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INITSTUB_VAR$PS");
  Ddi_Var_t *pvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");

  int i, iProp = -1;

  Ddi_Bdd_t *newLambdaBdd, *myInvar, *property;
  Ddi_Var_t *lambdaPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
  Ddi_Var_t *lambdaNs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$NS");

  if (lambdaPs == NULL) {
    lambdaPs = Ddi_VarNewAtLevel(ddiMgr, 0);
    lambdaNs = Ddi_VarNewAtLevel(ddiMgr, 1);
    Ddi_VarAttachName(lambdaPs, "PDT_BDD_INVAR_VAR$PS");
    Ddi_VarAttachName(lambdaNs, "PDT_BDD_INVAR_VAR$NS");
  }
  //  Ddi_VarMakeGroup (ddiMgr, lambdaPs, 2);

  // The constraint should be inserted before INVARSPEC but after INITSTUB
  int nl = Ddi_VararrayNum(fsmFsm->ps), constPosition = nl;
  Ddi_Var_t *var = Ddi_VararrayRead(fsmFsm->ps, nl - 1);

  if (var == pvarPs) {
    constPosition = nl - 1;
    iProp = Ddi_BddarrayNum(fsmFsm->delta) - 1;
    Pdtutil_Assert(fsmFsm->iFoldedProp==iProp,"wrong ifoldedprop");;
    fsmFsm->iFoldedProp++;
  }

  if (var == isvarPs)
    constPosition = nl;

  Ddi_VararrayInsert(fsmFsm->ps, constPosition, lambdaPs);
  Ddi_VararrayInsert(fsmFsm->ns, constPosition, lambdaNs);

  fsmFsm->iFoldedConstr = constPosition;

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "|Constraint|=%d nodes.\n",
      Ddi_BddSize(fsmFsm->constraint)));

  {
    int nrem = 0, i;
    Ddi_Bdd_t *a = Ddi_AigPartitionTop(fsmFsm->constraint, 0);
    Ddi_Varset_t *psv = Ddi_VarsetMakeFromArray(fsmFsm->ps);
    Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(fsmFsm->pi);

    for (i = Ddi_BddPartNum(a) - 1; 1 && i >= 0; i--) {
      Ddi_Bdd_t *p_i = Ddi_BddPartRead(a, i);
      Ddi_Varset_t *si = Ddi_BddSupp(p_i);
      int nv = Ddi_VarsetNum(si);

      Ddi_VarsetDiffAcc(si, psv);
      Ddi_VarsetDiffAcc(si, piv);
      if (Ddi_VarsetNum(si) == nv) {
        /* remove component */
        Ddi_BddPartRemove(a, i);
        nrem++;
      }
      Ddi_Free(si);
    }
    Ddi_Free(psv);
    Ddi_Free(piv);

    if (nrem > 0) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "removed %d constraint components\n", nrem));
      Ddi_BddSetAig(a);
      Ddi_DataCopy(fsmFsm->constraint, a);
    }
    Ddi_Free(a);
  }

  if (!Ddi_BddIsMono(fsmFsm->constraint)) {
    newLambdaBdd = Ddi_BddMakeLiteralAig(lambdaPs, 1);
    myInvar = Ddi_BddDup(fsmFsm->constraint);
  } else {
    newLambdaBdd = Ddi_BddMakeLiteral(lambdaPs, 1);
    myInvar = Ddi_BddMakeAig(fsmFsm->constraint);
  }
  /* whenever constraint is violated, remember it */
  Ddi_BddAndAcc(myInvar, newLambdaBdd);
  Ddi_BddarrayInsert(fsmFsm->delta, constPosition, myInvar);
  if (fsmFsm->initStub != NULL) {
    Pdtutil_Assert(fsmFsm->initStubConstraint != NULL, "missing IS constr");
    Ddi_BddarrayInsert(fsmFsm->initStub, constPosition,
      fsmFsm->initStubConstraint);
    Ddi_Unlock(fsmFsm->initStubConstraint);
    Ddi_Free(fsmFsm->initStubConstraint);
  } else {
    Ddi_BddAndAcc(fsmFsm->init, newLambdaBdd);
  }

  if (fsmFsm->invarspec) {
    Ddi_Bdd_t *property = Ddi_BddarrayRead(fsmFsm->delta, 
					   fsmFsm->iFoldedProp);
    Ddi_BddNotAcc(newLambdaBdd);
    Ddi_BddOrAcc(fsmFsm->invarspec, newLambdaBdd);
    //    Ddi_BddOrAcc(property, newLambdaBdd);
    Ddi_BddNotAcc(newLambdaBdd);
  }

  for (i = 0; i < Ddi_BddarrayNum(fsmFsm->lambda); i++) {
    if (compl_invarspec) {
      Ddi_BddNotAcc(newLambdaBdd);
      Ddi_BddOrAcc(Ddi_BddarrayRead(fsmFsm->lambda, i), newLambdaBdd);
      Ddi_BddNotAcc(newLambdaBdd);
    }
    else 
      Ddi_BddAndAcc(Ddi_BddarrayRead(fsmFsm->lambda, i), newLambdaBdd);
  }

  Ddi_Free(newLambdaBdd);
  Ddi_Free(myInvar);
}

int
Fsm_FsmUnfoldConstraint(
  Fsm_Fsm_t * fsmFsm
)
{
  if (fsmFsm->constraint == NULL)
    return -1;

  if (fsmFsm->fsmMgr == NULL)
    return -1;

  Ddi_Mgr_t *ddiMgr = fsmFsm->fsmMgr->dd;

  Ddi_Bddarray_t *delta = fsmFsm->delta;
  Ddi_Bddarray_t *initStub = fsmFsm->initStub;
  Ddi_Vararray_t *ps = fsmFsm->ps;
  Ddi_Var_t *cvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

  int iConstr = -1, i = 0;
  int nstate = Ddi_BddarrayNum(delta);
  Ddi_Bdd_t *deltaConstr = NULL;
  Ddi_Bdd_t *initStubConstr = NULL;

  for (i = 0; i < nstate; i++) {
    Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);

    if (v_i == cvarPs) {
      iConstr = i;
      deltaConstr = Ddi_BddarrayRead(delta, i);
      // ???
      // Ddi_BddCofactorAcc(deltaConstr, cvarPs, 1);
      if (initStub != NULL) {
        initStubConstr = Ddi_BddarrayRead(initStub, i);
      }
    }
  }

  if (iConstr < 0)
    return 0;
  fsmFsm->iFoldedConstr = -1;

  // Clean vars and deltas...

  if (deltaConstr == NULL) {
    Ddi_BddComposeAcc(fsmFsm->constraint, ps, delta);
  } else {
    Ddi_Unlock(fsmFsm->constraint);
    Ddi_Free(fsmFsm->constraint);
    fsmFsm->constraint = Ddi_BddDup(deltaConstr);
    Ddi_Lock(fsmFsm->constraint);
  }

  if (initStubConstr != NULL) {
    Ddi_Unlock(fsmFsm->initStubConstraint);
    Ddi_Free(fsmFsm->initStubConstraint);
    fsmFsm->initStubConstraint = Ddi_BddDup(initStubConstr);
    Ddi_Lock(fsmFsm->initStubConstraint);
  }
  // Remove constant delta
  Ddi_BddarrayRemove(delta, iConstr);
  if (fsmFsm->initStub != NULL) {
    Ddi_BddarrayRemove(fsmFsm->initStub, iConstr);
  }
  // Remove variables
  Ddi_VararrayRemove(ps, iConstr);
  Ddi_VararrayRemove(fsmFsm->ns, iConstr);
  fsmFsm->size.l--;

  Ddi_BddarrayCofactorAcc(fsmFsm->delta, cvarPs, 1);
  Ddi_BddarrayCofactorAcc(fsmFsm->lambda, cvarPs, 1);
  Ddi_BddCofactorAcc(fsmFsm->init, cvarPs, 1);

  if (fsmFsm->constraint != NULL)
    Ddi_BddCofactorAcc(fsmFsm->constraint, cvarPs, 1);
  if (fsmFsm->invarspec != NULL)
    Ddi_BddCofactorAcc(fsmFsm->invarspec, cvarPs, 1);

  return 1;
}

int
Fsm_FsmUnfoldProperty(
  Fsm_Fsm_t * fsmFsm,
  int unfoldLambda
)
{
  //  if (fsmFsm->invarspec == NULL)
  //    return -1;

  if (fsmFsm->fsmMgr == NULL)
    return -1;

  Ddi_Mgr_t *ddiMgr = fsmFsm->fsmMgr->dd;

  Ddi_Bddarray_t *delta = fsmFsm->delta;
  Ddi_Vararray_t *ps = fsmFsm->ps;
  Ddi_Var_t *cvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
  Ddi_Var_t *pvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");

  int iConstr = -1, iProp = -1, i = 0;
  int nstate = Ddi_BddarrayNum(delta);
  Ddi_Bdd_t *deltaProp = NULL, *deltaPropRef;

  if (fsmFsm->iFoldedProp < 0) {
    /* not folded */
    return 0;
  }
  iProp = fsmFsm->iFoldedProp;
  fsmFsm->iFoldedProp = -1;

  if (unfoldLambda) {
    Ddi_BddarrayCofactorAcc(fsmFsm->lambda,cvarPs,1);
    Ddi_BddarrayComposeAcc(fsmFsm->lambda, ps, delta);
  }
  if (fsmFsm->invarspec != NULL)
    Ddi_BddComposeAcc(fsmFsm->invarspec, ps, delta);

  deltaPropRef = Ddi_BddDup(Ddi_BddarrayRead(delta, iProp));
  deltaProp = Ddi_BddCofactor(Ddi_BddarrayRead(delta, iProp), pvarPs, 1);
  if (fsmFsm->iFoldedConstr >= 0) {
    Ddi_BddCofactorAcc(deltaProp, cvarPs, 1);
  }
  // Remove constant delta
  Ddi_BddarrayRemove(delta, iProp);
  if (fsmFsm->initStub != NULL) {
    Ddi_BddarrayRemove(fsmFsm->initStub, iProp);
  }
  // Restore original lambda
  //  Fsm_FsmWriteLambda (fsmFsm, Fsm_MgrReadLambdaBDD(fsmFsm->fsmMgr));

  Ddi_BddarrayCofactorAcc(fsmFsm->delta, pvarPs, 1);
  if (unfoldLambda)
    Ddi_BddarrayCofactorAcc(fsmFsm->lambda, pvarPs, 1);
  Ddi_BddCofactorAcc(fsmFsm->init, pvarPs, 1);

  if (fsmFsm->constraint != NULL)
    Ddi_BddCofactorAcc(fsmFsm->constraint, pvarPs, 1);
  if (fsmFsm->invarspec != NULL)
    Ddi_BddCofactorAcc(fsmFsm->invarspec, pvarPs, 1);

  // Remove variables
  Ddi_VararrayRemove(ps, iProp);
  Ddi_VararrayRemove(fsmFsm->ns, iProp);
  fsmFsm->size.l--;

  Ddi_Free(deltaProp);
  Ddi_Free(deltaPropRef);

  return 1;
}

int
Fsm_FsmUnfoldInit(
  Fsm_Fsm_t * fsmFsm
)
{
  if (fsmFsm->fsmMgr == NULL)
    return -1;

  Ddi_Mgr_t *ddiMgr = fsmFsm->fsmMgr->dd;

  Ddi_Bddarray_t *delta = fsmFsm->delta;
  Ddi_Vararray_t *ps = fsmFsm->ps;
  Ddi_Var_t *isvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INITSTUB_VAR$PS");


  Pdtutil_Assert(0, "fix restore init stub");

  // It should be the last one, but whatever...
  int nstate = Ddi_BddarrayNum(delta), i;

  for (i = 0; i < nstate; i++) {
    Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);

    if (v_i == isvarPs) {
      break;
    }
  }

  Pdtutil_Assert(i < nstate, "PDT_BDD_INITSTUB_VAR$PS var not found");

  int isPos = i, ndelta = Ddi_BddarrayNum(fsmFsm->delta);

  i = 0;
  fsmFsm->iFoldedInit = -1;

  // Revert back all the deltas
  for (i = 0; i < ndelta - 1; i++) {
    Ddi_Bdd_t *iteDelta = Ddi_BddarrayRead(fsmFsm->delta, i);
    Ddi_Bdd_t *originalDelta = Ddi_BddCofactor(iteDelta, isvarPs, 1);

    // ???
    // Ddi_Bdd_t *originalInitstub = Ddi_BddCofactor(iteDelta, isvarPs, 0);
    Ddi_BddarrayWrite(fsmFsm->delta, i, originalDelta);
    Ddi_Free(originalDelta);
  }

  // Remove constant delta
  Ddi_BddarrayRemove(delta, isPos);

  // Remove variables
  Ddi_VararrayRemove(ps, isPos);
  Ddi_VararrayRemove(fsmFsm->ns, isPos);

}

int
Fsm_FsmUnfoldInitWithPi(
  Fsm_Fsm_t * fsmFsm
)
{
  if (fsmFsm->fsmMgr == NULL)
    return -1;

  Ddi_Mgr_t *ddiMgr = fsmFsm->fsmMgr->dd;
  Ddi_Bddarray_t *newDelta, *initStub;
  Ddi_Vararray_t *ps = fsmFsm->ps;
  Ddi_Var_t *isvarPi = Ddi_VarFromName(ddiMgr, "PDT_BDD_INITSTUB_VAR$PI");

  // Revert back all the deltas
  if (Ddi_BddarrayNum(fsmFsm->delta) > 0) {
    newDelta = Ddi_BddarrayCofactor(fsmFsm->delta, isvarPi, 1);
    initStub = Ddi_BddarrayCofactor(fsmFsm->delta, isvarPi, 0);
    Ddi_DataCopy(fsmFsm->delta, newDelta);
    Ddi_Free(newDelta);
    Ddi_AigarrayConstrainCubeAcc(initStub, fsmFsm->init);
  } else {
    initStub = Ddi_BddarrayDup(fsmFsm->delta);
  }
  Pdtutil_Assert(fsmFsm->initStub == NULL, "NULL init stub needed");
  fsmFsm->initStub = initStub;
  Ddi_Lock(fsmFsm->initStub);

  if (fsmFsm->lambda != NULL) {
    Ddi_BddarrayCofactorAcc(fsmFsm->lambda, isvarPi, 1);
  }

  if (fsmFsm->invarspec != NULL) {
    Ddi_BddCofactorAcc(fsmFsm->invarspec, isvarPi, 1);
  }
  if (fsmFsm->constraint != NULL) {
    Ddi_BddCofactorAcc(fsmFsm->constraint, isvarPi, 1);
  }
  // Remove variables
  Ddi_Vararray_t *stubVarsA = Ddi_BddarraySuppVararray(fsmFsm->initStub);

  Ddi_VararrayInsertLast(stubVarsA, isvarPi);
  Ddi_VararrayDiffAcc(fsmFsm->pi, stubVarsA);
  Ddi_Free(stubVarsA);
  fsmFsm->size.i = Ddi_VararrayNum(fsmFsm->pi);

  return 1;
}

void
Fsm_FsmFree(
  Fsm_Fsm_t * fsmFsm
)
{
  if (fsmFsm != NULL) {
    Ddi_Unlock(fsmFsm->initStub);
    Ddi_Free(fsmFsm->initStub);
    Ddi_Unlock(fsmFsm->delta);
    Ddi_Free(fsmFsm->delta);
    Ddi_Unlock(fsmFsm->lambda);
    Ddi_Free(fsmFsm->lambda);
    Ddi_Unlock(fsmFsm->pi);
    Ddi_Free(fsmFsm->pi);
    Ddi_Unlock(fsmFsm->ps);
    Ddi_Free(fsmFsm->ps);
    Ddi_Unlock(fsmFsm->ns);
    Ddi_Free(fsmFsm->ns);
    Ddi_Unlock(fsmFsm->is);
    Ddi_Free(fsmFsm->is);
    Ddi_Unlock(fsmFsm->init);
    Ddi_Free(fsmFsm->init);
    Ddi_Unlock(fsmFsm->constraint);
    Ddi_Free(fsmFsm->constraint);
    Ddi_Unlock(fsmFsm->invarspec);
    Ddi_Free(fsmFsm->invarspec);
    Ddi_Unlock(fsmFsm->justice);
    Ddi_Free(fsmFsm->justice);
    Ddi_Unlock(fsmFsm->fairness);
    Ddi_Free(fsmFsm->fairness);
    Ddi_Unlock(fsmFsm->initStubConstraint);
    Ddi_Free(fsmFsm->initStubConstraint);
    Ddi_Unlock(fsmFsm->latchEqClasses);
    Ddi_Free(fsmFsm->latchEqClasses);
    Ddi_Unlock(fsmFsm->constrInvar);
    Ddi_Free(fsmFsm->constrInvar);

    Pdtutil_ListFree(fsmFsm->settings);
    Pdtutil_ListFree(fsmFsm->stats);

    if (fsmFsm->prev != NULL) {
      fsmFsm->prev->next = fsmFsm->next;
    } else {
      fsmFsm->fsmMgr->fsmList = fsmFsm->next;
    }
    if (fsmFsm->next != NULL) {
      fsmFsm->next->prev = fsmFsm->prev;
    }

    Pdtutil_Free(fsmFsm);
  }
}


Fsm_Fsm_t *
Fsm_FsmAbcReduce(
  Fsm_Fsm_t * fsmFsm,
  float compactTh
)
{
  Fsm_Fsm_t *fsmOpt = Fsm_FsmDup(fsmFsm);

  int pUnfold = Fsm_FsmUnfoldProperty(fsmOpt, 1);
  int cUnfold = Fsm_FsmUnfoldConstraint(fsmOpt);
  int useConstr = 0;

  Pdtutil_Assert(Ddi_BddIsAig(fsmOpt->invarspec),
    "partitioned spec not supported in abc reduce");
  Ddi_Bdd_t *target = Ddi_BddNot(fsmOpt->invarspec);

  Ddi_BddarrayWrite(fsmOpt->lambda, 0, target);
  Ddi_Free(target);
  if (fsmOpt->constraint!=NULL && !Ddi_BddIsOne(fsmOpt->constraint)) {
    useConstr=1;
    Ddi_BddarrayInsert(fsmOpt->lambda,1,fsmOpt->constraint);
  }

  if (fsmFsm->initStub != NULL) {
    int i;

    Fsm_FsmFoldInitWithPi(fsmOpt);

#if 0

    Fsm_FsmUnfoldInitWithPi(fsmOpt);

    if (pUnfold)
      Fsm_FsmFoldProperty(fsmOpt, 0, 0);
    if (cUnfold)
      Fsm_FsmFoldConstraint(fsmOpt, 0);

    for (i = 0; i < Ddi_BddarrayNum(fsmOpt->delta); i++) {
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(fsmOpt->delta, i);
      Ddi_Bdd_t *d_i0 = Ddi_BddarrayRead(fsmFsm->delta, i);

      Pdtutil_Assert(Ddi_AigEqualSat(d_i0, d_i), "error");
      Ddi_Bdd_t *s_i = Ddi_BddarrayRead(fsmOpt->initStub, i);
      Ddi_Bdd_t *s_i0 = Ddi_BddarrayRead(fsmFsm->initStub, i);

      Pdtutil_Assert(Ddi_AigEqualSat(s_i0, s_i), "error");
    }
    Pdtutil_Assert(Ddi_AigEqualSat(fsmFsm->invarspec, fsmOpt->invarspec),
      "error");
    Pdtutil_Assert(Ddi_AigEqualSat(fsmFsm->constraint, fsmOpt->constraint),
      "error");

    Fsm_FsmFoldInitWithPi(fsmOpt);

#endif

  }

  Ddi_Bddarray_t *delta = fsmOpt->delta;
  Ddi_Bddarray_t *lambda = fsmOpt->lambda;
  Ddi_Vararray_t *pi = fsmOpt->pi;
  Ddi_Vararray_t *ps = fsmOpt->ps;
  Ddi_Vararray_t *ns = fsmOpt->ns;

  if (Ddi_AbcReduce(delta, lambda, pi, ps, ns, compactTh)) {
    Ddi_Bdd_t *invarspec = Ddi_BddNot(Ddi_BddarrayRead(lambda, 0));

    Fsm_FsmWriteInvarspec(fsmOpt, invarspec);
    Ddi_Free(invarspec);

    if (useConstr) {
      Ddi_Bdd_t *constr = Ddi_BddarrayExtract(lambda, 1);

      Fsm_FsmWriteConstraint(fsmOpt, constr);
      Ddi_Free(constr);
    }


    if (fsmFsm->initStub != NULL) {
      Fsm_FsmUnfoldInitWithPi(fsmOpt);
    } else {
      Ddi_Varset_t *psv = Ddi_VarsetMakeFromArray(ps);

      Ddi_AigCubeExistProjectAcc(fsmOpt->init, psv);
      Ddi_Free(psv);
    }

    if (!Ddi_BddIsOne(fsmOpt->invarspec)) {
      if (pUnfold)
        Fsm_FsmFoldProperty(fsmOpt, 0, 0, 1); 
      if (cUnfold)
        Fsm_FsmFoldConstraint(fsmOpt, 0);
   }

  } else {
    Fsm_FsmFree(fsmOpt);
    fsmOpt = NULL;
  }
  return fsmOpt;
}

Fsm_Fsm_t *
Fsm_FsmAbcScorr(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * latchEq,
  int indK
)
{
  Fsm_Fsm_t *fsmOpt = Fsm_FsmDup(fsmFsm);
  int nlambda = Ddi_BddarrayNum(fsmOpt->lambda);
  int pUnfold = Fsm_FsmUnfoldProperty(fsmOpt, 1);
  int cUnfold = Fsm_FsmUnfoldConstraint(fsmOpt);
  int useConstr = 0;
  Ddi_Bdd_t *myInvarspec = NULL;

  myInvarspec = (invarspec != NULL) ? invarspec : fsmOpt->invarspec;
  Ddi_Bdd_t *target = Ddi_BddNot(myInvarspec);

  if (Ddi_BddIsPartDisj(target)) {
    int j;

    for (j = 0; j < Ddi_BddPartNum(target); j++) {
      Ddi_BddarrayInsertLast(fsmOpt->lambda, Ddi_BddPartRead(target, j));
    }
  } else {
    Ddi_BddarrayInsertLast(fsmOpt->lambda, target);
  }

  if (fsmFsm->initStub != NULL) {
    int i;

    Fsm_FsmFoldInitWithPi(fsmOpt);
  }

  Abc_ScorrReduceFsm(fsmOpt, care, latchEq, indK);

  if (fsmFsm->initStub != NULL) {
    Fsm_FsmUnfoldInitWithPi(fsmOpt);
  } else {
    Ddi_Varset_t *psv = Ddi_VarsetMakeFromArray(fsmOpt->ps);

    Ddi_AigCubeExistProjectAcc(fsmOpt->init, psv);
    Ddi_Free(psv);
  }

  if (Ddi_BddIsPartDisj(target)) {
    int j;

    for (j = Ddi_BddPartNum(target) - 1; j >= 0; j--) {
      Ddi_BddPartWrite(target, j, Ddi_BddarrayRead(fsmOpt->lambda,
          nlambda + j));
      Ddi_BddarrayRemove(fsmOpt->lambda, nlambda + j);
    }
  } else {
    Ddi_Free(target);
    target = Ddi_BddDup(Ddi_BddarrayRead(fsmOpt->lambda, nlambda));
    Ddi_BddarrayRemove(fsmOpt->lambda, nlambda);
  }

  myInvarspec = (invarspec != NULL) ? invarspec : fsmOpt->invarspec;
  Ddi_DataCopy(myInvarspec, target);
  Ddi_BddNotAcc(myInvarspec);

  Ddi_Free(target);

  if (pUnfold)
    Fsm_FsmFoldProperty(fsmOpt, 0, 0, 0);
  if (cUnfold)
    Fsm_FsmFoldConstraint(fsmOpt, 0);

  return fsmOpt;
}

Fsm_Fsm_t *
Fsm_FsmAbcReduceMulti(
  Fsm_Fsm_t * fsmFsm,
  float compactTh
)
{
  Fsm_Fsm_t *fsmOpt = Fsm_FsmDup(fsmFsm);
  Ddi_Mgr_t *ddm = fsmFsm->fsmMgr->dd;
  int useConstr = 0, nc = 0, nprop = Ddi_BddarrayNum(fsmFsm->lambda);
  int pUnfold = Fsm_FsmUnfoldProperty(fsmOpt, 1);
  int cUnfold = Fsm_FsmUnfoldConstraint(fsmOpt);
  
  Pdtutil_Assert(fsmOpt->invarspec == NULL,
    "partitioned spec not supported in abc reduce");
  Pdtutil_Assert(1 || fsmOpt->initStub == NULL,
    "partitioned spec not supported in abc reduce");
  if (fsmOpt->constraint != NULL) {
    int j;
    Ddi_Bdd_t *myConstr = Ddi_AigPartitionTop(fsmOpt->constraint, 0);

    useConstr = 1;
    for (j = 0; j < Ddi_BddPartNum(myConstr); j++) {
      Ddi_Bdd_t *c_j = Ddi_BddPartRead(myConstr, j);

      Ddi_BddarrayInsertLast(fsmOpt->lambda, c_j);
    }
    Ddi_Free(myConstr);
  }

  if (fsmFsm->initStub != NULL) {
    int i;

    Fsm_FsmFoldInitWithPi(fsmOpt);
  }

  Ddi_Bddarray_t *delta = fsmOpt->delta;
  Ddi_Bddarray_t *lambda = fsmOpt->lambda;
  Ddi_Vararray_t *pi = fsmOpt->pi;
  Ddi_Vararray_t *ps = fsmOpt->ps;
  Ddi_Vararray_t *ns = fsmOpt->ns;

  if (Ddi_AbcReduce(delta, lambda, pi, ps, ns, compactTh)) {

    // Ddi_AigarrayAbcRpmAcc(delta,ps,-1,-1.0);

    if (useConstr) {
      int j, nz = 0;
      int il = Ddi_BddarrayNum(lambda) - 1;
      Ddi_Bdd_t *myConstr = Ddi_BddMakeConstAig(ddm, 1);

      for (j = Ddi_BddarrayNum(lambda) - 1; j >= nprop; j--) {
        Ddi_Bdd_t *constr = Ddi_BddarrayExtract(lambda, j);

        if (!Ddi_BddIsZero(constr)) {
          Ddi_BddAndAcc(myConstr, constr);
        } else {
          nz++;
        }
        Ddi_Free(constr);
      }
      Fsm_FsmWriteConstraint(fsmOpt, myConstr);
      Ddi_Free(myConstr);
    }


    if (fsmFsm->initStub != NULL) {
      Fsm_FsmUnfoldInitWithPi(fsmOpt);
    } else {
      Ddi_Varset_t *psv = Ddi_VarsetMakeFromArray(ps);

      Ddi_AigCubeExistProjectAcc(fsmOpt->init, psv);
      Ddi_Free(psv);
    }
    if (pUnfold)
        Fsm_FsmFoldProperty(fsmOpt, 0, 0, 1); 
    if (cUnfold)
        Fsm_FsmFoldConstraint(fsmOpt, 0);

  } else {
    Fsm_FsmFree(fsmOpt);
    fsmOpt = NULL;
  }
  return fsmOpt;
}



Ddi_Bdd_t *
Fsm_FsmSimRarity(
  Fsm_Fsm_t * fsmFsm
)
{
  Fsm_Fsm_t *fsm2 = Fsm_FsmDup(fsmFsm);

  int pUnfold = Fsm_FsmUnfoldProperty(fsm2, 1);
  int cUnfold = Fsm_FsmUnfoldConstraint(fsm2);
  int useConstr = 0;
  Ddi_Bdd_t *cex = NULL;

  Pdtutil_Assert(Ddi_BddIsAig(fsm2->invarspec),
    "partitioned spec not supported in abc reduce");
  Ddi_Bdd_t *target = Ddi_BddNot(fsm2->invarspec);

  Ddi_BddarrayWrite(fsm2->lambda, 0, target);
  Ddi_Free(target);

  if (fsmFsm->initStub != NULL) {
    int i;

    // target -> prop
    Ddi_BddNotAcc(Ddi_BddarrayRead(fsm2->lambda, 0));
    Fsm_FsmFoldInit(fsm2);
    // prop -> target
    Ddi_BddNotAcc(Ddi_BddarrayRead(fsm2->lambda, 0));
  }

  Fsm_FsmZeroInit(fsm2);

  Ddi_Bddarray_t *delta = fsm2->delta;
  Ddi_Bddarray_t *lambda = fsm2->lambda;
  Ddi_Vararray_t *pi = fsm2->pi;
  Ddi_Vararray_t *ps = fsm2->ps;
  Ddi_Vararray_t *ns = fsm2->ns;
  Ddi_Vararray_t *is = fsm2->is;

  if (Ddi_BddIsOne(fsmFsm->constraint)) {
    cex = Ddi_AbcRaritySim(delta, lambda, pi, ps, ns, is);
  }
  Fsm_FsmFree(fsm2);

  return cex;
}

Fsm_Fsm_t *
Fsm_FsmTargetEnAppr(
  Fsm_Fsm_t * fsmFsm,
  int depth
)
{
  Fsm_Fsm_t *fsm2 = Fsm_FsmDup(fsmFsm);
  Ddi_Mgr_t *ddm = fsmFsm->fsmMgr->dd;

  Ddi_Bddarray_t *delta = fsm2->delta;
  Ddi_Bddarray_t *lambda = fsm2->lambda;
  Ddi_Vararray_t *pi = fsm2->pi;
  Ddi_Vararray_t *ps = fsm2->ps;
  Ddi_Vararray_t *ns = fsm2->ns;
  Ddi_Vararray_t *is = fsm2->is;
  int i;
  Ddi_Bdd_t *prop, *target = Ddi_BddNot(Ddi_BddarrayRead(lambda,0));
  Ddi_Varset_t *psv = Ddi_VarsetMakeFromArray(fsmFsm->ps);

  Pdtutil_Assert(Ddi_BddarrayNum(lambda)==1,"too many lambdas");
  Pdtutil_Assert(fsmFsm->iFoldedInit<0,"unfolded init needed by approx te")

  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
    printf("APPROX TARGET ENLARGEMENT - T: %d - %d steps\n", 
	   Ddi_BddSize(target), depth);
  }

  for (i=0; i<depth; i++) {
    Ddi_AigExistProjectAcc (target,psv,NULL,0/*partial*/,1/*nosat*/,10.0);
    Ddi_AigExistProjectOverAcc(target, psv, NULL);
    Ddi_BddComposeAcc(target,ps,delta);
    Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
      printf("APPROX TARGET [%d]: %d\n", i, Ddi_BddSize(target));
    }
    prop = Ddi_BddNot(target);
    Ddi_BddarrayInsertLast(lambda, prop);
    Ddi_Free(prop);
  }

  Pdtutil_VerbosityMgrIf(ddm, Pdtutil_VerbLevelUsrMax_c) {
    printf("EXACT/APPROX TARGET TOTAL SIZE: %d\n", 
	   Ddi_BddarraySize(lambda));
  }

  Ddi_Free(psv);
  Ddi_Free(target);

  Fsm_FsmSetNO(fsm2, Ddi_BddarrayNum(lambda));

  return fsm2;
}

Fsm_Fsm_t *
Fsm_FsmReduceEnable(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bddarray_t *enArray
)
{
  Fsm_Fsm_t *fsm2 = Fsm_FsmDup(fsmFsm);
  Ddi_Mgr_t *ddm = fsmFsm->fsmMgr->dd;

  Ddi_Bddarray_t *delta = fsm2->delta;
  Ddi_Bddarray_t *lambda = fsm2->lambda;
  Ddi_Vararray_t *pi = fsm2->pi;
  Ddi_Vararray_t *ps = fsm2->ps;
  Ddi_Vararray_t *ns = fsm2->ns;
  Ddi_Vararray_t *is = fsm2->is;
  Ddi_Vararray_t *enA2=NULL, *enA2Ns=NULL;
  int i,j, freeEnA=0;
  Ddi_Vararray_t *vA;
  Ddi_Bddarray_t *vSubst;
  
  Ddi_Var_t *v=Ddi_VararrayRead(pi,1);
  Ddi_BddarrayCofactorAcc(delta,v,1);
  Ddi_BddarrayCofactorAcc(lambda,v,1);

  if (enArray==NULL) {
    enArray = Ddi_BddarrayAlloc(ddm, 0);
    freeEnA=1;
  }
  enA2 = Ddi_VararrayAlloc(ddm, 0);
  enA2Ns = Ddi_VararrayAlloc(ddm, 0);

  vA = Ddi_VararrayAlloc(ddm, 0);
  vSubst = Ddi_BddarrayAlloc(ddm, 0);
  int again = -1;

  for (i=0; i<Ddi_VararrayNum(pi) && again; i++) {
    int s0, s1, s01, nDep=0, nDep1=0, direct_j=-1, en_j=-1,
        directCompl=0;
    //    int initNodes = Ddi_MgrReadAigNodesNum(ddm);
    Ddi_Var_t *v = Ddi_VararrayRead(pi,i);
    Ddi_Bddarray_t *d_0 = Ddi_BddarrayCofactor(delta,v,0);
    Ddi_Bddarray_t *d_1 = Ddi_BddarrayCofactor(delta,v,1);
    for (j=0; j<Ddi_BddarrayNum(delta); j++) {
      Ddi_Bdd_t *d_0_j = Ddi_BddarrayRead(d_0,j);
      Ddi_Bdd_t *d_1_j = Ddi_BddarrayRead(d_1,j);
      if (!Ddi_BddEqual(d_0_j,d_1_j)) {
	nDep++;
	if (Ddi_BddIsConstant(d_0_j)&&Ddi_BddIsConstant(d_1_j)) {
	  direct_j = j;
	  directCompl = Ddi_BddIsZero(d_1_j);
	}
	else {
	  en_j = j;
	}
      }
    }
    if ((nDep==2) && direct_j>=0 && en_j>=0) {
      int k;
      Ddi_Var_t *vEn = NULL;
      Ddi_Var_t *vlDir = Ddi_VararrayRead(ps,direct_j);
      Ddi_Var_t *vl = Ddi_VararrayRead(ps,en_j);
      Ddi_Bdd_t *d_en = Ddi_BddarrayRead(delta,en_j);
      Ddi_Bdd_t *en = Ddi_BddCofactor(d_en,v,!directCompl);
      Ddi_Bdd_t *lit, *lit0, *mux;
      Ddi_BddCofactorAcc(en,vl,0);
      Ddi_BddNotAcc(en);
      for(k=0; k<Ddi_BddarrayNum(enArray); k++) {
	Ddi_Bdd_t *en_k = Ddi_BddarrayRead(enArray,k);
	if (Ddi_BddEqual(en,en_k)) {
	  vEn = Ddi_VararrayRead(enA2,k);
	  break;
	}
      }

      again--;
      char namei[100];
      sprintf(namei,"Pdtrav_Pi_Var_%s", Ddi_VarName(v));
      Ddi_Var_t *v2 = Ddi_VarNewBaig(ddm, namei);
      Pdtutil_Assert(v2!=NULL,"no en variable");
      Ddi_VararrayInsertLast(pi,v2);

      if (k>=Ddi_BddarrayNum(enArray)) {
	char name[100];
	sprintf(name,"Pdtrav_En_Var_%d$NS", k);
	vEn = Ddi_VarNewBaig(ddm, name);
	Pdtutil_Assert(vEn!=NULL,"no en variable");
	Ddi_VararrayInsertLast(enA2Ns,vEn);
	sprintf(name,"Pdtrav_En_Var_%d", k);
	vEn = Ddi_VarNewBaig(ddm, name);
	Pdtutil_Assert(vEn!=NULL,"no en variable");
	Ddi_VararrayInsertLast(enA2,vEn);
	Ddi_BddarrayInsertLast(enArray,en);
      }

      Pdtutil_Assert(vEn!=NULL,"no en variable");
      Ddi_VararrayInsertLast(vA,vlDir);
      mux = Ddi_BddMakeLiteralAig(vEn,1);
      lit = Ddi_BddMakeLiteralAig(v2,!directCompl);
      lit0 = Ddi_BddMakeLiteralAig(vl,1);
      Ddi_BddIteAcc(mux,lit,lit0);
      Ddi_Free(lit);
      Ddi_Free(lit0);
      Ddi_BddarrayInsertLast(vSubst,mux);
      Ddi_Free(en);
      Ddi_Free(mux);

      printf("%s) direct latch: %s - en latch: %s\n",
	     Ddi_VarName(v), 
	     Ddi_VarName(Ddi_VararrayRead(ps,direct_j)),
	     Ddi_VarName(Ddi_VararrayRead(ps,en_j)));
    }
    Ddi_Free(d_0);
    Ddi_Free(d_1);
  }


  if (fsmFsm->initStub != NULL) {
  }

  if (Ddi_BddarrayNum(enArray)>0) {
    int k;
    for(k=0; k<Ddi_BddarrayNum(enArray); k++) {
      Ddi_Bdd_t *en_k = Ddi_BddarrayRead(enArray,k);
      Ddi_Var_t *vEnPs = Ddi_VararrayRead(enA2,k);
      Ddi_Var_t *vEnNs = Ddi_VararrayRead(enA2Ns,k);
      Ddi_Bdd_t *initLit = Ddi_BddMakeLiteralAig(vEnPs, 0);
      Ddi_BddarrayInsert(delta,k,en_k);
      Ddi_VararrayInsert(ps,k,vEnPs);
      Ddi_VararrayInsert(ns,k,vEnNs);
      Ddi_BddAndAcc(fsm2->init,initLit);
      Ddi_Free(initLit);
    }
  }

  Ddi_BddarrayComposeAcc(delta,vA,vSubst);
  Ddi_BddarrayComposeAcc(lambda,vA,vSubst);

  if (freeEnA) {
    Ddi_Free(enArray);
  }
  Ddi_Free(enA2);
  Ddi_Free(enA2Ns);
  Ddi_Free(vA);
  Ddi_Free(vSubst);

  return fsm2;
}


/**CHeaderFile*****************************************************************

  FileName    [fsm.h]

  PackageName [fsm]

  Synopsis    [External header file for package Fsm]

  Description [This package provides functions to read in and write out
    descriptions of FSMs in the PdTrav format.<br>
    Support to read a blif file are also given. <p>

    External procedures included in this module are:
    <ul>
    <li> Fsm_MgrLoad ()
    <li> Fsm_MgrStore ()
    <li> Fsm_MgrInit ()
    <li> Fsm_MgrQuit ()
    <li> Fsm_MgrDup ()
    </ul>
    <p>

    The FSM (name myFsm) structure follows the following schema:<p>
    .Fsm myFsm<br>
    </P>
    .Size<br>
      .i 4<br>
      .o 1<br>
      .l 3<br>
    .EndSize<br>
    </P>
    .Ord<br>
      .ordFile myFsmFSM.ord<br>
    .EndOrd<br>
    </P>
    .Name<br>
      .i G0 G1 G2 G3<br>
      .ps G5 G6 G7<br>
      .ns G5$NS G6$NS G7$NS<br>
    .EndName<br>
    </P>
    .Index<br>
      .i 0 1 2 3<br>
      .ps 4 5 6<br>
      .ns 7 8 9<br>
    .EndIndex<br>
    </P>
    .Delta<br>
      .bddFile myFsmdelta.bdd<br>
    .EndDelta<br>
    </P>
    .Lambda<br>
      .bddFile myFsmlambda.bdd<br>
    .EndLambda<br>
    </P>
    .InitState<br>
      .bddFile myFsms0.bdd<br>
    .EndInitState<br>
    </P>
    .Tr<br>
      .bddFile myFsmTR.bdd<br>
    .EndTr<br>
    </P>
    .Reached<br>
       .bddFile myFsmReached.bdd<br>
    .EndReached<br>
    </P>
    .EndFsm<p>

    The functions to read a blif file are partially taken, almost verbatim,
    from the nanotrav directory of the Cudd-2.3.0 package.<br>
    The original names have been modified changing the prefix in the
    following way:<p>
    Port_ ---> Fsm_Port<br>
    Port ---> FsmPort<br>
    port ---> fsmPort<br>
    PORT_ ---> FSM_<p>
    The functions directly called by nanotrav are:
    <pre>
    (name in nanotrav)     (name in the fsmPort package)
    </P>
    Bnet_ReadNetwork       Fsm_PortBnetReadNetwork
    Bnet_FreeNetwork       Fsm_PortBnetFreeNetwork
    Ntr_buildDDs           Fsm_PortNtrBuildDDs 
    Ntr_initState          Fsm_PortNtrInitState
    </pre>
    <p>
    fsmPortBnet.c contains the parts taken from bnet.c
    fsmPortNtr.c contains the parts taken from ntr.c.
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

#ifndef _FSM
#define _FSM

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include "ddi.h"
#include "pdtutil.h"
#include "tr.h"
#include "part.h"
#include "aiger.h"



/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*
 *  FSM Manager Structure
 */

typedef struct Fsm_Mgr_s Fsm_Mgr_t;


/*
 *  MINI FSM Structure
 */

typedef struct Fsm_Fsm_s Fsm_Fsm_t;


/*
 *  Retime Structure
 */

struct Fsm_Retime_s {
  int retimeEqualFlag;
  int removeLatches;
  int *retimeArray;
  Ddi_Bddarray_t *dataArray;
  Ddi_Bddarray_t *enableArray;
  Ddi_Bddarray_t **retimeGraph;
  int *retimeVector;
  int *set;
  int *refEn;
  int *enCnt;
};

typedef struct Fsm_Retime_s Fsm_Retime_t;

/*
 * Typedef for the FsmPort (nanotrav) interface
 */

typedef struct FsmPortBnetTabline_s FsmPortBnetTabline_t;
typedef struct FsmPortBnetNode_s FsmPortBnetNode_t;
typedef struct FsmPortBnetNetwork_s FsmPortBnetNetwork_t;
typedef struct FsmPortNtrOptions_s FsmPortNtrOptions_t;

typedef struct Fsm_XsimMgr_t Fsm_XsimMgr_t;


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**Macro***********************************************************************
  Synopsis    [Wrapper for setting an option value]
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/

#define Fsm_MgrSetField(mgr,tag,dfield,data) { \
  Pdtutil_OptItem_t optItem;\
  optItem.optTag.eFsmOpt = tag;\
  optItem.optData.dfield = data;\
  Fsm_MgrSetFieldItem(mgr, optItem);\
}

#define Fsm_MgrReadField(mgr,tag,dfield,type,var) { \
  Pdtutil_OptItem_t optItem;\
  optItem.optTag.eFsmOpt = tag;\
  Fsm_MgrReadItem(mgr, &optItem);\
  var=(type)optItem.optData.dfield;\
}

#define Fsm_MgrSetOption(mgr,tag,dfield,data) { \
  Pdtutil_OptItem_t optItem;\
  optItem.optTag.eFsmOpt = tag;\
  optItem.optData.dfield = data;\
  Fsm_MgrSetOptionItem(mgr, optItem);\
}

#define Fsm_MgrIncrInitStubSteps(fsmMgr,incr) { \
  int is = Fsm_MgrReadInitStubSteps (fsmMgr);\
  Fsm_MgrSetInitStubSteps (fsmMgr, is+(incr));\
}

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

/* FSM MINI */

EXTERN Fsm_Fsm_t *Fsm_FsmDup(
  Fsm_Fsm_t * fsmFsm
);
EXTERN Fsm_Fsm_t *Fsm_FsmInit(
);
EXTERN Fsm_Mgr_t *Fsm_FsmReadMgr(
  Fsm_Fsm_t * fsmFsm
);
EXTERN int Fsm_FsmReadNI(
  Fsm_Fsm_t * fsmFsm
);
EXTERN int Fsm_FsmReadNO(
  Fsm_Fsm_t * fsmFsm
);
EXTERN int Fsm_FsmReadNL(
  Fsm_Fsm_t * fsmFsm
);
EXTERN Ddi_Vararray_t *Fsm_FsmReadPS(
  Fsm_Fsm_t * fsmFsm
);
EXTERN Ddi_Vararray_t *Fsm_FsmReadPI(
  Fsm_Fsm_t * fsmFsm
);
EXTERN Ddi_Vararray_t *Fsm_FsmReadNS(
  Fsm_Fsm_t * fsmFsm
);
EXTERN Ddi_Bddarray_t *Fsm_FsmReadLambda(
  Fsm_Fsm_t * fsmFsm
);
EXTERN Ddi_Bddarray_t *Fsm_FsmReadDelta(
  Fsm_Fsm_t * fsmFsm
);
EXTERN Ddi_Bddarray_t *Fsm_FsmReadInitStub(
  Fsm_Fsm_t * fsmFsm
);
EXTERN Ddi_Bdd_t *Fsm_FsmReadInit(
  Fsm_Fsm_t * fsmFsm
);
EXTERN Ddi_Bdd_t *Fsm_FsmReadInvarspec(
  Fsm_Fsm_t * fsmFsm
);
EXTERN Ddi_Bdd_t *Fsm_FsmReadConstraint(
  Fsm_Fsm_t * fsmFsm
);
EXTERN Ddi_Bdd_t *Fsm_FsmReadJustice(
  Fsm_Fsm_t * fsmFsm
);
EXTERN Ddi_Bdd_t *Fsm_FsmReadFairness(
  Fsm_Fsm_t * fsmFsm
);
EXTERN Ddi_Bdd_t *Fsm_FsmReadCex(
  Fsm_Fsm_t * fsmFsm
);
EXTERN Ddi_Bdd_t *Fsm_FsmReadInitStubConstraint(
  Fsm_Fsm_t * fsmFsm
);
EXTERN Pdtutil_List_t *Fsm_FsmReadSettings(
  Fsm_Fsm_t * fsmFsm
);
EXTERN Pdtutil_List_t *Fsm_FsmReadStats(
  Fsm_Fsm_t * fsmFsm
);

//EXTERN void Fsm_FsmWriteMgr(Fsm_Fsm_t *fsmFsm,Fsm_Mgr_t* fsmMgr);
EXTERN void Fsm_FsmSetNI(
  Fsm_Fsm_t * fsmFsm,
  int ni
);
EXTERN void Fsm_FsmSetNO(
  Fsm_Fsm_t * fsmFsm,
  int no
);
EXTERN void Fsm_FsmSetNL(
  Fsm_Fsm_t * fsmFsm,
  int nl
);
EXTERN void Fsm_FsmSetFsmMgr(
  Fsm_Fsm_t * fsmFsm,
  Fsm_Mgr_t * fsmMgr
);
EXTERN void Fsm_FsmNnfEncoding(
  Fsm_Fsm_t * fsmFsm
);
EXTERN void Fsm_FsmFoldInit(
  Fsm_Fsm_t * fsmFsm
);
EXTERN void Fsm_FsmFoldInitWithPi(
  Fsm_Fsm_t * fsmFsm
);
EXTERN void Fsm_FsmFoldConstraint(
  Fsm_Fsm_t * fsmFsm
);
EXTERN void Fsm_FsmDeltaWithConstraint(
  Fsm_Fsm_t * fsmFsm
);
EXTERN void Fsm_FsmFoldProperty(
  Fsm_Fsm_t * fsmFsm,
  int,
  int,
  int
);
EXTERN void
Fsm_FsmAddPropertyWitnesses(
  Fsm_Fsm_t * fsmFsm
);
EXTERN int Fsm_FsmUnfoldInit(
  Fsm_Fsm_t * fsmFsm
);
EXTERN int Fsm_FsmUnfoldConstraint(
  Fsm_Fsm_t * fsmFsm
);
EXTERN int Fsm_FsmUnfoldInitWithPi(
  Fsm_Fsm_t * fsmFsm
);
EXTERN int Fsm_FsmUnfoldProperty(
  Fsm_Fsm_t * fsmFsm,
  int unfoldLambda
);
EXTERN void Fsm_FsmExchangeFsmMgr(
  Fsm_Fsm_t * fsmFsm,
  Fsm_Mgr_t * fsmMgr
);
EXTERN void Fsm_FsmWritePS(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Vararray_t * ps
);
EXTERN void Fsm_FsmWritePI(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Vararray_t * pi
);
EXTERN void Fsm_FsmWriteNS(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Vararray_t * ns
);
EXTERN void Fsm_FsmWriteLambda(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bddarray_t * lambda
);
EXTERN void Fsm_FsmWriteDelta(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bddarray_t * delta
);
EXTERN void Fsm_FsmWriteInitStub(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bddarray_t * initStub
);
EXTERN void Fsm_FsmWriteInit(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * init
);
EXTERN void Fsm_FsmWriteInvarspec(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * invarspec
);
EXTERN void Fsm_FsmWriteConstraint(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * constraint
);
EXTERN void Fsm_FsmWriteJustice(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * constraint
);
EXTERN void Fsm_FsmWriteFairness(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * constraint
);
EXTERN void Fsm_FsmWriteInitStubConstraint(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * constraint
);
EXTERN void Fsm_FsmWriteCex(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * cex
);
EXTERN void Fsm_FsmWriteSettings(
  Fsm_Fsm_t * fsmFsm,
  Pdtutil_List_t * settings
);
EXTERN void Fsm_FsmWriteStats(
  Fsm_Fsm_t * fsmFsm,
  Pdtutil_List_t * stats
);
EXTERN void Fsm_FsmWriteToFsmMgr(
  Fsm_Mgr_t * fsmMgr,
  Fsm_Fsm_t * fsmFsm
);
EXTERN Fsm_Fsm_t *Fsm_FsmMakeFromFsmMgr(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Fsm_Fsm_t *Fsm_FsmMake(
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
);
EXTERN void Fsm_FsmFree(
  Fsm_Fsm_t * fsmFsm
);
EXTERN int Fsm_FsmWriteAiger(
  Fsm_Mgr_t * fsmMgr,
  char *fileAigName
);
EXTERN aiger *Fsm_FsmCopyToAiger(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_FsmMiniWriteAiger(
  Fsm_Fsm_t * fsmFsm,
  char *fileAigName
);
EXTERN aiger *Fsm_FsmMiniCopyToAiger(
  Fsm_Fsm_t * fsmMgr
);
EXTERN int Fsm_FsmCheckSizeConsistency(
  Fsm_Fsm_t * fsmFsm
);

 /**/ EXTERN int Fsm_BlifStore(
  Fsm_Mgr_t * fsmMgr,
  int reduceDelta,
  char *fileName,
  FILE * fp
);
EXTERN int Fsm_MgrLoad(
  Fsm_Mgr_t ** fsmMgrP,
  Ddi_Mgr_t * dd,
  char *fileFsmName,
  char *fileOrdName,
  int bddFlag,
  Pdtutil_VariableOrderFormat_e ordFileFormat
);
EXTERN int Fsm_MgrLoadAiger(
  Fsm_Mgr_t ** fsmMgrP,
  Ddi_Mgr_t * dd,
  char *fileFsmName,
  char *fileOrdName,
  Pdtutil_VariableOrderFormat_e ordFileFormat
);
EXTERN int Fsm_AigsimCex(
  char *aigerfile,
  char *cexfile
);
EXTERN Fsm_Mgr_t *Fsm_MgrInit(
  char *fsmName,
  Ddi_Mgr_t * ddm
);
EXTERN int Fsm_MgrCheck(
  Fsm_Mgr_t * fsmMgr
);
EXTERN void Fsm_MgrQuit(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Fsm_Mgr_t *Fsm_MgrDup(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Fsm_Mgr_t *Fsm_MgrDupWithNewDdiMgr(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Fsm_Mgr_t *Fsm_MgrDupWithNewDdiMgrAligned(
  Fsm_Mgr_t * fsmMgr
);
EXTERN void Fsm_MgrAuxVarRemove(
  Fsm_Mgr_t * fsmMgr
);
EXTERN void Fsm_MgrCoiReduction(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_MgrAigToBdd(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Var_t *Fsm_MgrSetClkStateVar(
  Fsm_Mgr_t * fsmMgr,
  char *varname,
  int toggle
);
EXTERN void Fsm_MgrComposeCutVars(
  Fsm_Mgr_t * fsmMgr,
  int threshold
);
EXTERN int Fsm_MgrOperation(
  Fsm_Mgr_t * fsmMgr,
  char *string,
  Pdtutil_MgrOp_t operationFlag,
  void **voidPointer,
  Pdtutil_MgrRet_t * returnFlagP
);
EXTERN char *Fsm_MgrReadFileName(
  Fsm_Mgr_t * fsmMgr
);
EXTERN char *Fsm_MgrReadFsmName(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Mgr_t *Fsm_MgrReadDdManager(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bdd_t *Fsm_MgrReadReachedBDD(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bdd_t *Fsm_MgrReadConstraintBDD(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bdd_t *Fsm_MgrReadJusticeBDD(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bdd_t *Fsm_MgrReadFairnessBDD(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bdd_t *Fsm_MgrReadCexBDD(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bdd_t *Fsm_MgrReadInitStubConstraintBDD(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bdd_t *Fsm_MgrReadCareBDD(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bdd_t *Fsm_MgrReadInvarspecBDD(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_MgrReadIFoldedProp(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Varsetarray_t *Fsm_MgrReadRetimedPis(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bdd_t *Fsm_MgrReadInitStubPiConstr(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Var_t *Fsm_MgrReadPdtSpecVar(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Var_t *Fsm_MgrReadPdtConstrVar(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Var_t *Fsm_MgrReadPdtInitStubVar(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_MgrReadBddFormat(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_MgrReadNI(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_MgrReadNO(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_MgrReadNL(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_MgrReadNAuxVar(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Vararray_t *Fsm_MgrReadVarI(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Vararray_t *Fsm_MgrReadVarO(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Vararray_t *Fsm_MgrReadVarPS(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Vararray_t *Fsm_MgrReadVarNS(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Vararray_t *Fsm_MgrReadVarAuxVar(
  Fsm_Mgr_t * fsmMgr
);
EXTERN char *Fsm_MgrReadInitStubName(
  Fsm_Mgr_t * fsmMgr
);
EXTERN char *Fsm_MgrReadDeltaName(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bddarray_t *Fsm_MgrReadInitStubBDD(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bddarray_t *Fsm_MgrReadDeltaBDD(
  Fsm_Mgr_t * fsmMgr
);
EXTERN char *Fsm_MgrReadLambdaName(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bddarray_t *Fsm_MgrReadLambdaBDD(
  Fsm_Mgr_t * fsmMgr
);
EXTERN char *Fsm_MgrReadAuxVarName(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bddarray_t *Fsm_MgrReadAuxVarBDD(
  Fsm_Mgr_t * fsmMgr
);
EXTERN char *Fsm_MgrReadTrName(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bdd_t *Fsm_MgrReadTrBDD(
  Fsm_Mgr_t * fsmMgr
);
EXTERN char *Fsm_MgrReadInitName(
  Fsm_Mgr_t * fsmMgr
);
EXTERN char *Fsm_MgrReadInitString(
  Fsm_Mgr_t * fsmMgr
);
EXTERN char *Fsm_MgrReadTrString(
  Fsm_Mgr_t * fsmMgr
);
EXTERN char *Fsm_MgrReadReachedString(
  Fsm_Mgr_t * fsmMgr
);
EXTERN char *Fsm_MgrReadFromString(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bdd_t *Fsm_MgrReadInitBDD(
  Fsm_Mgr_t * fsmMgr
);
EXTERN char *Fsm_MgrReadFromName(
  Fsm_Mgr_t * fsmMgr
);
EXTERN char *Fsm_MgrReadReachedName(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bdd_t *Fsm_MgrReadFromBDD(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Pdtutil_VerbLevel_e Fsm_MgrReadVerbosity(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_MgrReadCutThresh(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_MgrReadCegarStrategy(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_MgrReadPhaseAbstr(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_MgrReadExternalConstr(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_MgrReadXInitLatches(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_MgrReadTargetEnSteps(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_MgrReadInitStubSteps(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_MgrReadRemovedPis(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_MgrReadUseAig(
  Fsm_Mgr_t * fsmMgr
);
EXTERN void Fsm_MgrSetFileName(
  Fsm_Mgr_t * fsmMgr,
  char *fileName
);
EXTERN void Fsm_MgrSetFsmName(
  Fsm_Mgr_t * fsmMgr,
  char *fsmName
);
EXTERN void Fsm_MgrSetDdManager(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Mgr_t * var
);
EXTERN void Fsm_MgrSetReachedBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * var
);
EXTERN void Fsm_MgrSetCareBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * con
);
EXTERN void Fsm_MgrSetCexBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * con
);
EXTERN void Fsm_MgrSetInitStubConstraintBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * con
);
EXTERN void Fsm_MgrSetConstraintBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * con
);
EXTERN void Fsm_MgrSetJusticeBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * con
);
EXTERN void Fsm_MgrSetFairnessBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * con
);
EXTERN void Fsm_MgrSetInvarspecBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * inv
);
EXTERN void Fsm_MgrSetRetimedPis(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Varsetarray_t * retimedPis
);
EXTERN void Fsm_MgrSetInitStubPiConstr(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * initStubPiConstr
);
EXTERN void Fsm_MgrSetPdtSpecVar(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Var_t * v
);
EXTERN void Fsm_MgrSetPdtConstrVar(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Var_t * v
);
EXTERN void Fsm_MgrSetPdtInitStubVar(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Var_t * v
);
EXTERN void Fsm_MgrSetBddFormat(
  Fsm_Mgr_t * fsmMgr,
  int var
);
EXTERN void Fsm_MgrSetVarI(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Vararray_t * var
);
EXTERN void Fsm_MgrSetVarO(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Vararray_t * var
);
EXTERN void Fsm_MgrSetVarPS(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Vararray_t * var
);
EXTERN void Fsm_MgrSetVarNS(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Vararray_t * var
);
EXTERN void Fsm_MgrSetVarAuxVar(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Vararray_t * var
);
EXTERN void Fsm_MgrSetInitStubName(
  Fsm_Mgr_t * fsmMgr,
  char *var
);
EXTERN void Fsm_MgrSetDeltaName(
  Fsm_Mgr_t * fsmMgr,
  char *var
);
EXTERN void Fsm_MgrSetInitStubBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * initStub
);
EXTERN void Fsm_MgrSetDeltaBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * delta
);
EXTERN void Fsm_MgrSetLambdaName(
  Fsm_Mgr_t * fsmMgr,
  char *var
);
EXTERN void Fsm_MgrSetLambdaBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * var
);
EXTERN void Fsm_MgrSetAuxVarName(
  Fsm_Mgr_t * fsmMgr,
  char *var
);
EXTERN void Fsm_MgrSetAuxVarBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * var
);
EXTERN void Fsm_MgrSetTrName(
  Fsm_Mgr_t * fsmMgr,
  char *var
);
EXTERN void Fsm_MgrSetTrBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * var
);
EXTERN void Fsm_MgrSetInitName(
  Fsm_Mgr_t * fsmMgr,
  char *var
);
EXTERN void Fsm_MgrSetInitString(
  Fsm_Mgr_t * fsmMgr,
  char *var
);
EXTERN void Fsm_MgrSetTrString(
  Fsm_Mgr_t * fsmMgr,
  char *var
);
EXTERN void Fsm_MgrSetFromString(
  Fsm_Mgr_t * fsmMgr,
  char *var
);
EXTERN void Fsm_MgrSetReachedString(
  Fsm_Mgr_t * fsmMgr,
  char *var
);
EXTERN void Fsm_MgrSetInitBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * var
);
EXTERN void Fsm_MgrSetFromName(
  Fsm_Mgr_t * fsmMgr,
  char *var
);
EXTERN void Fsm_MgrSetReachedName(
  Fsm_Mgr_t * fsmMgr,
  char *var
);
EXTERN void Fsm_MgrSetFromBDD(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * var
);
EXTERN void Fsm_MgrSetVerbosity(
  Fsm_Mgr_t * fsmMgr,
  Pdtutil_VerbLevel_e var
);
EXTERN void Fsm_MgrSetCutThresh(
  Fsm_Mgr_t * fsmMgr,
  int cutThresh
);
EXTERN void Fsm_MgrSetUseAig(
  Fsm_Mgr_t * fsmMgr,
  int useAig
);
EXTERN void Fsm_MgrSetCegarStrategy(
  Fsm_Mgr_t * fsmMgr,
  int cegarStrategy
);
EXTERN void Fsm_MgrSetPhaseAbstr(
  Fsm_Mgr_t * fsmMgr,
  int phaseAbstr
);
EXTERN void Fsm_MgrSetRemovedPis(
  Fsm_Mgr_t * fsmMgr,
  int removedPis
);
EXTERN void Fsm_MgrSetInitStubSteps(
  Fsm_Mgr_t * fsmMgr,
  int initStubSteps
);
EXTERN void Fsm_MgrSetTargetEnSteps(
  Fsm_Mgr_t * fsmMgr,
  int targetEnSteps
);
EXTERN void Fsm_MgrSetOptionList(
  Fsm_Mgr_t * fsmMgr,
  Pdtutil_OptList_t * pkgOpt
);
EXTERN void Fsm_MgrSetOptionItem(
  Fsm_Mgr_t * fsmMgr,
  Pdtutil_OptItem_t optItem
);
EXTERN void Fsm_MgrReadOption(
  Fsm_Mgr_t * fsmMgr,
  Pdt_OptTagFsm_e fsmOpt,
  void *optRet
);
EXTERN int Fsm_MgrLoadFromBlif(
  Fsm_Mgr_t ** fsmMgrP,
  Ddi_Mgr_t * dd,
  char *fileFsmName,
  char *fileOrdName,
  int bddFlag,
  Pdtutil_VariableOrderFormat_e ordFileFormat
);
EXTERN int Fsm_MgrLoadFromBlifWithCoiReduction(
  Fsm_Mgr_t ** fsmMgrP,
  Ddi_Mgr_t * dd,
  char *fileFsmName,
  char *fileOrdName,
  int bddFlag,
  Pdtutil_VariableOrderFormat_e ordFileFormat,
  char **coiList
);
EXTERN FsmPortBnetNetwork_t *Fsm_PortBnetReadNetwork(
  FILE * fp,
  int pr
);
EXTERN int Fsm_PortBnetBuildNodeBDD(
  Ddi_Mgr_t * dd,
  FsmPortBnetNode_t * nd,
  st_table * hash,
  int params,
  int nodrop,
  int cutThresh,
  int useAig
);
EXTERN int Fsm_PortBnetDfsVariableOrder(
  Ddi_Mgr_t * dd,
  FsmPortBnetNetwork_t * net,
  int useAig
);
EXTERN int Fsm_PortBnetReadOrder(
  Ddi_Mgr_t * dd,
  char *ordFile,
  FsmPortBnetNetwork_t * net,
  int locGlob,
  int nodrop,
  Pdtutil_VariableOrderFormat_e ordFileFormat,
  int useAig
);
EXTERN void Fsm_PortBnetFreeNetwork(
  FsmPortBnetNetwork_t * net
);
EXTERN int Fsm_PortNtrBuildDDs(
  FsmPortBnetNetwork_t * net,
  Ddi_Mgr_t * dd,
  FsmPortNtrOptions_t * option,
  Pdtutil_VariableOrderFormat_e ordFileFormat
);
EXTERN Ddi_Bdd_t *Fsm_PortNtrInitState(
  Ddi_Mgr_t * dd,
  FsmPortBnetNetwork_t * net,
  FsmPortNtrOptions_t * option
);
EXTERN int Fsm_MgrPMBuild(
  Fsm_Mgr_t ** fsmMgrPMP,
  Fsm_Mgr_t * fsmMgr1,
  Fsm_Mgr_t * fsmMgr2
);
EXTERN Fsm_Mgr_t *
Fsm_RetimeGateCuts(
  Fsm_Mgr_t * fsmMgr,
  int mode
);
EXTERN Fsm_Mgr_t *Fsm_RetimeForRACompute(
  Fsm_Mgr_t * fsmMgr,
  Fsm_Retime_t * retimeStrPtr
);
EXTERN void Fsm_EnableDependencyCompute(
  Fsm_Mgr_t * fsmMgr,
  Fsm_Retime_t * retimeStrPtr
);
EXTERN void Fsm_CommonEnableCompute(
  Fsm_Mgr_t * fsmMgr,
  Fsm_Retime_t * retimeStrPtr
);
EXTERN void Fsm_RetimeCompute(
  Fsm_Mgr_t * fsmMgr,
  Fsm_Retime_t * retimeStrPtr
);
EXTERN void Fsm_OptimalRetimingCompute(
  Ddi_Mgr_t * dd,
  Pdtutil_VerbLevel_e verbosity,
  int retimeEqual,
  int *retimeDeltas,
  Ddi_Varset_t ** deltaSupp,
  Ddi_Bddarray_t * enableArray,
  Ddi_Bdd_t * en,
  Ddi_Vararray_t * sarray,
  int ns,
  Ddi_Varset_t * latches,
  Ddi_Varset_t * inputs,
  Ddi_Varset_t * notRetimed
);
EXTERN void Fsm_FindRemovableLatches(
  Ddi_Mgr_t * dd,
  int *removableLatches,
  Ddi_Bddarray_t * delta,
  Ddi_Vararray_t * sarray,
  Ddi_Varset_t * inputs,
  int ns
);
EXTERN Fsm_Mgr_t *Fsm_RetimeApply(
  Fsm_Mgr_t * fsmMgr,
  Fsm_Retime_t * retimeStrPtr
);
EXTERN int Fsm_RetimePeriferalLatches(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_RetimePeriferalLatchesForced(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bdd_t *Fsm_ReduceTerminalScc(
  Fsm_Mgr_t * fsmMgr,
  int *sccNumP,
  int *provedP,
  int constrLevel
);
EXTERN Ddi_Bdd_t *Fsm_ReduceImpliedConstr(
  Fsm_Mgr_t * fsmMgr,
  int decompId,
  Ddi_Bdd_t * outInvar,
  int *impliedConstrNumP
);
EXTERN Ddi_Bdd_t *Fsm_ReduceCustom(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bdd_t *Fsm_RetimeWithConstr(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bdd_t *Fsm_ReduceCegar(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * invarspec,
  int maxBound,
  int enPba
);
EXTERN int Fsm_RetimeMinreg(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * care,
  int strategy
);
EXTERN void Fsm_RetimeInitStub2Init(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_MgrStore(
  Fsm_Mgr_t * fsmMgr,
  char *fileName,
  FILE * fp,
  int bddFlag,
  int bddFormat,
  Pdtutil_VariableOrderFormat_e ordFileFormat
);
EXTERN int Fsm_BddFormatString2Int(
  char *string
);
EXTERN char *Fsm_BddFormatInt2String(
  int intValue
);
EXTERN void Fsm_MgrPrintStats(
  Fsm_Mgr_t * fsmMgr
);
EXTERN void Fsm_MgrPrintPrm(
  Fsm_Mgr_t * fsmMgr
);
EXTERN FsmPortBnetNetwork_t *Fsm_ReadAigFile(
  FILE * fp,
  char *fileFsmName
);
EXTERN Pdtutil_Array_t *Fsm_XsimSimulate(
  Fsm_XsimMgr_t * xMgr,
  int restart,
  Pdtutil_Array_t * psBinaryVals,
  int useTarget,
  int *splitDoneP
);
EXTERN Pdtutil_Array_t *Fsm_XsimSimulatePsBit(
  Fsm_XsimMgr_t * xMgr,
  int bitId,
  int bitVal,
  int useTarget,
  int *splitDoneP
);
EXTERN Pdtutil_Array_t *Fsm_XsimSymbolicSimulate(
  Fsm_XsimMgr_t * xMgr,
  int restart,
  Pdtutil_Array_t * psBinaryVals,
  int useTarget,
  int *splitDoneP
);
EXTERN void Fsm_XsimCheckEqClasses(
  Fsm_XsimMgr_t * xMgr
);
EXTERN Fsm_XsimMgr_t *Fsm_XsimInit(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * fA,
  Ddi_Bdd_t * invarspec,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * maxLevelVars,
  int strategy
);
EXTERN void Fsm_XsimQuit(
  Fsm_XsimMgr_t * xMgr
);
EXTERN void Fsm_XsimPrintEqClasses(
  Fsm_XsimMgr_t * xMgr
);
EXTERN int Fsm_XsimCountEquiv(
  Fsm_XsimMgr_t * xMgr
);
EXTERN Ddi_Bddarray_t *Fsm_XsimSymbolicMergeEq(
  Fsm_XsimMgr_t * xMgr,
  Ddi_Bdd_t * miters,
  Ddi_Bdd_t * invarspec
);
EXTERN void Fsm_XsimRandomSimulateSeq(
  Fsm_XsimMgr_t * xMgr,
  Ddi_Bddarray_t * initStub,
  Ddi_Bdd_t * init,
  int nTimeFrames
);
EXTERN Ddi_Bddarray_t *Fsm_XsimSymbolicSimulateAndRefineEqClasses(
  Fsm_XsimMgr_t * xMgr,
  Ddi_Bddarray_t * psSymbolicVals,
  Ddi_Bdd_t * assume,
  int maxLevel,
  int *splitDoneP
);
EXTERN Ddi_Bddarray_t *Fsm_XsimInductiveEq(
  Fsm_Mgr_t * fsmMgr,
  Fsm_XsimMgr_t * xMgr,
  int indDepth,
  int indMaxLevel,
  int assumeMiters,
  int *nEqP
);
EXTERN int Fsm_XsimSetGateFilter(
  Fsm_XsimMgr_t * xMgr,
  int maxLevel
);
EXTERN Ddi_Bdd_t *Fsm_XsimTraverse(
  Fsm_Mgr_t * fsmMgr,
  int enTernary,
  int strategy,
  int *pProved,
  int timeLimit
);
EXTERN int Fsm_FsmStructReduction(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Var_t * clkVar
);
EXTERN int Fsm_MgrAbcReduce(
  Fsm_Mgr_t * fsmMgr,
  float compactTh
);
EXTERN int Fsm_MgrAbcReduceMulti(
  Fsm_Mgr_t * fsmMgr,
  float compactTh
);
EXTERN int Fsm_MgrAbcScorr(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * pArray,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * latchEq,
  int indK
);
EXTERN Fsm_Fsm_t *Fsm_FsmAbcReduce(
  Fsm_Fsm_t * fsmFsm,
  float compactTh
);
EXTERN Fsm_Fsm_t *Fsm_FsmAbcReduceMulti(
  Fsm_Fsm_t * fsmFsm,
  float compactTh
);
EXTERN Fsm_Fsm_t *Fsm_FsmAbcScorr(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * latchEq,
  int indK
);
EXTERN Ddi_Bdd_t *Fsm_FsmSimRarity(
  Fsm_Fsm_t * fsmFsm
);
EXTERN void Fsm_MgrLogVerificationResult(
  Fsm_Mgr_t * fsmMgr,
  char *filename,
  int result
);
EXTERN void Fsm_MgrLogUnsatBound(
  Fsm_Mgr_t * fsmMgr,
  int bound,
  int unfolded
);
EXTERN int
Fsm_MgrReadUnsatBound(
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Fsm_CexNormalize(
  Fsm_Mgr_t * fsmMgr
);
EXTERN Ddi_Bdd_t *Fsm_AigCexToRefPis(
  Ddi_Bdd_t * tfCex,
  int jStub,
  int jTe,
  int phaseAbstr,
  int phase2
);
EXTERN int Fsm_MgrRetimePis(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * stubFuncs
);
EXTERN void Fsm_MgrReadItem(
  Fsm_Mgr_t * fsmMgr,
  Pdtutil_OptItem_t * optItemPtr
);
EXTERN void Abc_ScorrReduceFsm(
  Fsm_Fsm_t * fsm,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * latchEq,
  int indK
);
EXTERN Fsm_Fsm_t *
Fsm_FsmTargetEnAppr(
  Fsm_Fsm_t * fsmFsm,
  int depth
);
EXTERN Fsm_Fsm_t *
Fsm_FsmReduceEnable(
  Fsm_Fsm_t * fsmFsm,
  Ddi_Bddarray_t *enArray
);

// TEST PURPOSES :: June '13
EXTERN void myLink(
  Fsm_Fsm_t * fsm
);

/**AutomaticEnd***************************************************************/

#endif                          /* _FSM */

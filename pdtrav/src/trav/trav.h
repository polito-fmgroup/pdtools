/**CHeaderFile*****************************************************************

  FileName    [trav.h]

  PackageName [trav]

  Synopsis    [Main module for a simple traversal of finite state machine]

  Description [This package contains the main function to exploring the state
    space of a FSM.<br>
    There are three methods for image computation:
    <ol>
    <li> <b>Monolithic:</b> This is the most naive approach possible.<br>
       This technique is based on building <em>monolithic transition
       relation</em>. It is the conjunction of all latches transition
       relation. If we use <em>y</em> to denote the next state vector,
       <em>s</em> the present state vector, <em>x</em> the input
       vector and <em>delta()</em> the next state function, we define the
       trasition relation of <em>i</em>-th latch to be the function
       Ti (x,s,y) = yi <=> delta(i)(x,s).<br> Then, for a FSM of n latches,
       the monolhitic transition relation is:
       <p>
       T(x,s,y) = T1(x,s,y)*T2(x,s,y)* ... *Tn(x,s,y)
       <p>
       When the monolithic TR is built, the traversal algorithm is executed.
       This method is suitable for circuits with less than 20 latches

    <li> <b>IWLS95:</b> This technique is based on the early quantification
    heuristic.
       The initialization process consists of following steps:
       <ul>
       <li> Create the clustered TR from transition relation by clustering
            several function together. The size of clustering is controlled
            by a threshold value controlled by the user.
       <li> Order the clusters using the algorithm given in
            "Efficient BDD Algorithms for FSM Synthesis and
            Verification", by R. K. Ranjan et. al. in the proceedings of
            IWLS'95.
       <li> For each cluster, quantify out the quantify variables which
            are local to that particular cluster.
       </ul>
    <li> <b>Iterative squaring:</b> This technique is based on building the
       <em>transitive closure (TC)</em> of a monolithic TR. Afterwards, TC
       replace TR in the traversal algorithm.
    </ol>
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

#ifndef _TRAV
#define _TRAV

#include <pthread.h>
#include "pdtutil.h"
#include "ddi.h"
#include "part.h"
#include "fsm.h"

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define TRAV_BDD_VERIF     1

/* smoothed variables */
#define TRAV_SMOOTH_SX     0
#define TRAV_SMOOTH_S      1

#define TRAV_CLUST_THRESH 1000
#define TRAV_IMG_CLUST_THRESH 200

/* buffer size */
#define TRAV_FILENAME_BUFSIZE 80

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/**Enum************************************************************************

  Synopsis    [Type for From Selection]

  Description []

******************************************************************************/

typedef enum {
  Trav_FromSelectNew_c,
  Trav_FromSelectReached_c,
  Trav_FromSelectTo_c,
  Trav_FromSelectCofactor_c,
  Trav_FromSelectRestrict_c,
  Trav_FromSelectBest_c,
  Trav_FromSelectSame_c
} Trav_FromSelect_e;

/**Enum************************************************************************

  Synopsis    [Type for BMC bound chared handling]

  Description []

******************************************************************************/
typedef enum Trav_BmcBoundState_e {
  Trav_BmcBoundActive_c,
  Trav_BmcBoundProved_c,
  Trav_BmcBoundFailed_c,
  Trav_BmcBoundNone_c
} Trav_BmcBoundState_e;

/**Enum************************************************************************

  Synopsis    []

  Description []

******************************************************************************/

typedef struct Trav_Stats_s {
  Pdtutil_Array_t **arrayStats;
  int arrayStatsNum;
  int cpuTimeInit;
  int cpuTimeTot;
  int provedBound;
  int lastObserved;
  pthread_mutex_t mutex;
} Trav_Stats_t;

typedef struct Trav_Tune_s {
  Pdtutil_Array_t **arrayTune;
  int arrayTuneNum;
  int doAbort;
  int doSleep;
  pthread_mutex_t mutex;
} Trav_Tune_t;

typedef struct Trav_Shared_s {
  Ddi_ClauseArray_t *clauseShared;
  Pdtutil_Array_t *clauseSharedNum;
  Ddi_Bddarray_t *itpRchShared;
  Ddi_Mgr_t *ddmXchg;
  pthread_mutex_t mutex;
  struct Trav_Shared_s *sharedLink;
} Trav_Shared_t;

typedef struct Trav_Xchg_s {
  int enabled;
  Trav_Stats_t stats;
  Trav_Tune_t tune;
  Trav_Shared_t shared;
} Trav_Xchg_t;

typedef struct Trav_Mgr_s Trav_Mgr_t;
typedef struct Trav_Settings_s Trav_Settings_t;
typedef struct TravPdrMgr_s TravPdrMgr_t;
typedef struct TravTimeFrameInfo_s Trav_TimeFrameInfo_t;
typedef struct TravItpMgr_s Trav_ItpMgr_t; 
typedef struct TravItpTravMgr_s Trav_ItpTravMgr_t;



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

#define Trav_MgrSetOption(mgr,tag,dfield,data) {	\
  Pdtutil_OptItem_t optItem;\
  optItem.optTag.eTravOpt = tag;\
  optItem.optData.dfield = data;\
  Trav_MgrSetOptionItem(mgr, optItem);\
}

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EXTERN void Trav_Main(
  Trav_Mgr_t * travMgr
);
EXTERN Ddi_Bdd_t *Trav_ApproxMBM(
  Trav_Mgr_t * travMgr,
  Ddi_Bddarray_t * deltaAig
);
EXTERN Ddi_Bdd_t *Trav_ApproxLMBM(
  Trav_Mgr_t * travMgr,
  Ddi_Bddarray_t * deltaAig
);
EXTERN Ddi_Bdd_t *Trav_ApproxSatRefine(
  Ddi_Bdd_t * to,
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * trAig,
  Ddi_Bdd_t * prevRef,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * ns,
  Ddi_Bdd_t * init,
  int refineIter,
  int useInduction
);
EXTERN Trav_Mgr_t *Trav_MgrInit(
  char *travName,
  Ddi_Mgr_t * dd
);
EXTERN Trav_Mgr_t *Trav_MgrQuit(
  Trav_Mgr_t * travMgr
);
EXTERN void
Trav_PdrMgrQuit(
  TravPdrMgr_t * pdrMgr
);
EXTERN int Trav_MgrPrintStats(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrOperation(
  Trav_Mgr_t * travMgr,
  char *string,
  Pdtutil_MgrOp_t operationFlag,
  void **voidPointer,
  Pdtutil_MgrRet_t * returnFlagP
);
EXTERN Trav_Xchg_t *Trav_MgrXchg(
  Trav_Mgr_t * travMgr
);
EXTERN Tr_Tr_t *Trav_MgrReadTr(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetTr(
  Trav_Mgr_t * travMgr,
  Tr_Tr_t * tr
);
EXTERN int Trav_MgrReadLevel(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadProductPeak(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetProductPeak(
  Trav_Mgr_t * travMgr,
  int productPeak
);
EXTERN Ddi_Vararray_t *Trav_MgrReadI(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetTravOpt(
  Trav_Mgr_t * travMgr,
  Trav_Settings_t * opt
);
EXTERN void Trav_MgrSetI(
  Trav_Mgr_t * travMgr,
  Ddi_Vararray_t * i
);
EXTERN Ddi_Vararray_t *Trav_MgrReadPS(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetPS(
  Trav_Mgr_t * travMgr,
  Ddi_Vararray_t * ps
);
EXTERN Ddi_Vararray_t *Trav_MgrReadAux(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetAux(
  Trav_Mgr_t * travMgr,
  Ddi_Vararray_t * aux
);
EXTERN void Trav_MgrSetMgrAuxFlag(
  Trav_Mgr_t * travMgr,
  int flag
);
EXTERN Ddi_Vararray_t *Trav_MgrReadNS(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetNS(
  Trav_Mgr_t * travMgr,
  Ddi_Vararray_t * ns
);
EXTERN Ddi_Bdd_t *Trav_MgrReadReached(
  Trav_Mgr_t * travMgr
);
EXTERN Ddi_Bdd_t *Trav_MgrReadCare(
  Trav_Mgr_t * travMgr
);
EXTERN Ddi_Bdd_t *Trav_MgrReadAssume(
  Trav_Mgr_t * travMgr
);
EXTERN Ddi_Bdd_t *Trav_MgrReadProved(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetReached(
  Trav_Mgr_t * travMgr,
  Ddi_Bdd_t * reached
);
EXTERN void Trav_MgrSetCare(
  Trav_Mgr_t * travMgr,
  Ddi_Bdd_t * care
);
EXTERN void Trav_MgrSetAssume(
  Trav_Mgr_t * travMgr,
  Ddi_Bdd_t * assume
);
EXTERN void Trav_MgrSetProved(
  Trav_Mgr_t * travMgr,
  Ddi_Bdd_t * proved
);
EXTERN Ddi_Bdd_t *Trav_MgrReadAssert(
  Trav_Mgr_t * travMgr
);
EXTERN Ddi_Var_t *Trav_MgrReadPdtSpecVar(
  Trav_Mgr_t * travMgr
);
EXTERN Ddi_Var_t *Trav_MgrReadPdtConstrVar(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetAssert(
  Trav_Mgr_t * travMgr,
  Ddi_Bdd_t * assert
);
EXTERN void Trav_MgrSetPdtSpecVar(
  Trav_Mgr_t * travMgr,
  Ddi_Var_t * v
);
EXTERN void Trav_MgrSetPdtConstrVar(
  Trav_Mgr_t * travMgr,
  Ddi_Var_t * v
);
EXTERN int Trav_MgrReadAssertFlag(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetAssertFlag(
  Trav_Mgr_t * travMgr,
  int assertFlag
);
EXTERN Ddi_Bddarray_t *Trav_MgrReadNewi(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetNewi(
  Trav_Mgr_t * travMgr,
  Ddi_Bddarray_t * newi
);
EXTERN Ddi_Bddarray_t *Trav_MgrReadCexes(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetCexes(
  Trav_Mgr_t * travMgr,
  Ddi_Bddarray_t * cexes
);
EXTERN Ddi_Varsetarray_t *Trav_MgrReadAbstrRefRefinedVars(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetAbstrRefRefinedVars(
  Trav_Mgr_t * travMgr,
  Ddi_Varsetarray_t * abstrRefRefinedVars
);
EXTERN Ddi_Bdd_t *Trav_MgrReadFrom(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetFrom(
  Trav_Mgr_t * travMgr,
  Ddi_Bdd_t * from
);
EXTERN char *Trav_MgrReadName(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetName(
  Trav_Mgr_t * travMgr,
  char *travName
);
EXTERN int Trav_MgrReadSmoothVar(
  Trav_Mgr_t * travMgr
);
EXTERN double Trav_MgrReadW1(
  Trav_Mgr_t * travMgr
);
EXTERN double Trav_MgrReadW2(
  Trav_Mgr_t * travMgr
);
EXTERN double Trav_MgrReadW3(
  Trav_Mgr_t * travMgr
);
EXTERN double Trav_MgrReadW4(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadEnableDdR(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadTrProfileDynamicEnable(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetTrProfileDynamicEnable(
  Trav_Mgr_t * travMgr,
  int enable
);
EXTERN int Trav_MgrReadTrProfileThreshold(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetTrProfileThreshold(
  Trav_Mgr_t * travMgr,
  int threshold
);
EXTERN Cuplus_PruneHeuristic_e Trav_MgrReadTrProfileMethod(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetTrProfileMethod(
  Trav_Mgr_t * travMgr,
  Cuplus_PruneHeuristic_e method
);
EXTERN int Trav_MgrReadThreshold(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadDynAbstr(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadDynAbstrInitIter(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadImplAbstr(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadInputRegs(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpBdd(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpPart(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpAppr(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpExact(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpBoundkOpt(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpUseReached(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpTuneForDepth(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpConeOpt(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpInnerCones(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpReuseRings(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpInductiveTo(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpInitAbstr(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpEndAbstr(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpForceRun(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpCheckCompleteness(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpConstrLevel(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpGenMaxIter(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpRefineCex(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadTernaryAbstr(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadItpEnToPlusImg(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadIgrMaxIter(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadIgrMaxExact(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadIgrSide(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadIgrGrowCone(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadAbstrRef(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadAbstrRefGla(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadSquaring(
  Trav_Mgr_t * travMgr
);
EXTERN Pdtutil_VerbLevel_e Trav_MgrReadVerbosity(
  Trav_Mgr_t * travMgr
);
EXTERN FILE *Trav_MgrReadStdout(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetSorting(
  Trav_Mgr_t * travMgr,
  int sorting
);
EXTERN int Trav_MgrReadSorting(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadBackward(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadKeepNew(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadMaxIter(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadMismatchOptLevel(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadSelfTuningLevel(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadConstrLevel(
  Trav_Mgr_t * travMgr
);
EXTERN char *Trav_MgrReadClk(
  Trav_Mgr_t * travMgr
);
EXTERN char *Trav_MgrReadSatSolver(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadUnderApproxMaxVars(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadUnderApproxMaxSize(
  Trav_Mgr_t * travMgr
);
EXTERN float Trav_MgrReadUnderApproxTargetReduction(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadLogPeriod(
  Trav_Mgr_t * travMgr
);
EXTERN int Trav_MgrReadSavePeriod(
  Trav_Mgr_t * travMgr
);
EXTERN char *Trav_MgrReadSavePeriodName(
  Trav_Mgr_t * travMgr
);
EXTERN Trav_FromSelect_e Trav_MgrReadFromSelect(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetDdiMgrTr(
  Trav_Mgr_t * travMgr,
  Ddi_Mgr_t * mgrTr
);
EXTERN void Trav_MgrSetDdiMgrDefault(
  Trav_Mgr_t * travMgr,
  Ddi_Mgr_t * mgrTr
);
EXTERN void Trav_MgrSetDdiMgrR(
  Trav_Mgr_t * travMgr,
  Ddi_Mgr_t * mgrR
);
EXTERN Ddi_Mgr_t *Trav_MgrReadDdiMgrDefault(
  Trav_Mgr_t * travMgr
);
EXTERN Ddi_Mgr_t *Trav_MgrReadDdiMgrTr(
  Trav_Mgr_t * travMgr
);
EXTERN Ddi_Mgr_t *Trav_MgrReadDdiMgrR(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetVerbosity(
  Trav_Mgr_t * travMgr,
  Pdtutil_VerbLevel_e verbosity
);
EXTERN void Trav_MgrSetStdout(
  Trav_Mgr_t * travMgr,
  FILE * stdout
);
EXTERN void Trav_MgrSetBackward(
  Trav_Mgr_t * travMgr,
  int backward
);
EXTERN void Trav_MgrSetKeepNew(
  Trav_Mgr_t * travMgr,
  int keepNew
);
EXTERN void Trav_MgrSetMaxIter(
  Trav_Mgr_t * travMgr,
  int maxIter
);
EXTERN void Trav_MgrSetMismatchOptLevel(
  Trav_Mgr_t * travMgr,
  int mismatchOptLevel
);
EXTERN void Trav_MgrSetLazyTimeLimit(
  Trav_Mgr_t * travMgr,
  float tl
);
EXTERN void Trav_MgrSetSelfTuningLevel(
  Trav_Mgr_t * travMgr,
  int l
);
EXTERN void Trav_MgrSetConstrLevel(
  Trav_Mgr_t * travMgr,
  int l
);
EXTERN void Trav_MgrSetClk(
  Trav_Mgr_t * travMgr,
  char *clk
);
EXTERN void Trav_MgrSetSatSolver(
  Trav_Mgr_t * travMgr,
  char *satSolver
);
EXTERN void Trav_MgrSetUnderApproxMaxVars(
  Trav_Mgr_t * travMgr,
  int maxVars
);
EXTERN void Trav_MgrSetUnderApproxMaxSize(
  Trav_Mgr_t * travMgr,
  int maxSize
);
EXTERN void Trav_MgrSetUnderApproxTargetReduction(
  Trav_Mgr_t * travMgr,
  float targetReduction
);
EXTERN void Trav_MgrSetLogPeriod(
  Trav_Mgr_t * travMgr,
  int logPeriod
);
EXTERN void Trav_MgrSetSavePeriod(
  Trav_Mgr_t * travMgr,
  int savePeriod
);
EXTERN void Trav_MgrSetSavePeriodName(
  Trav_Mgr_t * travMgr,
  char *savePeriodName
);
EXTERN void Trav_MgrSetFromSelect(
  Trav_Mgr_t * travMgr,
  Trav_FromSelect_e fromSelect
);
EXTERN int Trav_MgrReadMgrAuxFlag(
  Trav_Mgr_t * travMgr
);
EXTERN void Trav_MgrSetDynAbstr(
  Trav_Mgr_t * travMgr,
  int dynAbstr
);
EXTERN void Trav_MgrSetDynAbstrInitIter(
  Trav_Mgr_t * travMgr,
  int dynAbstrInitIter
);
EXTERN void Trav_MgrSetImplAbstr(
  Trav_Mgr_t * travMgr,
  int implAbstr
);
EXTERN void Trav_MgrSetInputRegs(
  Trav_Mgr_t * travMgr,
  int inputRegs
);
EXTERN void Trav_MgrSetItpBdd(
  Trav_Mgr_t * travMgr,
  int itpBdd
);
EXTERN void Trav_MgrSetItpPart(
  Trav_Mgr_t * travMgr,
  int itpPart
);
EXTERN void Trav_MgrSetItpAppr(
  Trav_Mgr_t * travMgr,
  int itpAppr
);
EXTERN void Trav_MgrSetItpExact(
  Trav_Mgr_t * travMgr,
  int itpExact
);
EXTERN void Trav_MgrSetItpTuneForDepth(
  Trav_Mgr_t * travMgr,
  int itpTuneForDepth
);
EXTERN void Trav_MgrSetItpBoundkOpt(
  Trav_Mgr_t * travMgr,
  int itpBoundkOpt
);
EXTERN void Trav_MgrSetItpUseReached(
  Trav_Mgr_t * travMgr,
  int itpUseReached
);
EXTERN void Trav_MgrSetItpConeOpt(
  Trav_Mgr_t * travMgr,
  int itpConeOpt
);
EXTERN void Trav_MgrSetItpInnerCones(
  Trav_Mgr_t * travMgr,
  int itpInnerCones
);
EXTERN void Trav_MgrSetItpReuseRings(
  Trav_Mgr_t * travMgr,
  int itpReuseRings
);
EXTERN void Trav_MgrSetItpInductiveTo(
  Trav_Mgr_t * travMgr,
  int itpInductiveTo
);
EXTERN void Trav_MgrSetItpInitAbstr(
  Trav_Mgr_t * travMgr,
  int itpInitAbstr
);
EXTERN void Trav_MgrSetItpEndAbstr(
  Trav_Mgr_t * travMgr,
  int itpEndAbstr
);
EXTERN void Trav_MgrSetItpForceRun(
  Trav_Mgr_t * travMgr,
  int itpForceRun
);
EXTERN void Trav_MgrSetItpCheckCompleteness(
  Trav_Mgr_t * travMgr,
  int itpCheckCompleteness
);
EXTERN void Trav_MgrSetItpUsePdrReached(
  Trav_Mgr_t * travMgr,
  int itpUsePdrReached
);
EXTERN void Trav_MgrSetPdrShareReached(
  Trav_Mgr_t * travMgr,
  int pdrShareReached
);
EXTERN void Trav_MgrSetPdrUsePgEncoding(
  Trav_Mgr_t * travMgr,
  int pdrUsePgEncoding
);
EXTERN void Trav_MgrSetPdrBumpActTopologically(
  Trav_Mgr_t * travMgr,
  int pdrBumpActTopologically
);
EXTERN void Trav_MgrSetPdrSpecializedCallsMask(
  Trav_Mgr_t * travMgr,
  int pdrSpecializedCallsMask
);
EXTERN void Trav_MgrSetPdrRestartStrategy(
  Trav_Mgr_t * travMgr,
  int pdrRestartStrategy
);
EXTERN void Trav_MgrSetItpShareReached(
  Trav_Mgr_t * travMgr,
  int itpShareReached
);
EXTERN void Trav_MgrSetPdrFwdEq(
  Trav_Mgr_t * travMgr,
  int pdrFwdEq
);
EXTERN void Trav_MgrSetPdrInf(
  Trav_Mgr_t * travMgr,
  int pdrInf
);
EXTERN void Trav_MgrSetPdrCexJustify(
  Trav_Mgr_t * travMgr,
  int val
);
EXTERN void Trav_MgrSetPdrGenEffort(
  Trav_Mgr_t * travMgr,
  int pdrGenEffort
);
EXTERN void Trav_MgrSetPdrIncrementalTr(
  Trav_Mgr_t * travMgr,
  int pdrIncrementalTr
);
EXTERN void Trav_MgrSetItpConstrLevel(
  Trav_Mgr_t * travMgr,
  int itpConstrLevel
);
EXTERN void Trav_MgrSetItpGenMaxIter(
  Trav_Mgr_t * travMgr,
  int itpGenMaxIter
);
EXTERN void Trav_MgrSetItpRefineCex(
  Trav_Mgr_t * travMgr,
  int itpRefineCex
);
EXTERN void Trav_MgrSetTernaryAbstr(
  Trav_Mgr_t * travMgr,
  int ternaryAbstr
);
EXTERN void Trav_MgrSetItpEnToPlusImg(
  Trav_Mgr_t * travMgr,
  int val
);
EXTERN void Trav_MgrSetIgrMaxIter(
  Trav_Mgr_t * travMgr,
  int val
);
EXTERN void Trav_MgrSetIgrMaxExact(
  Trav_Mgr_t * travMgr,
  int val
);
EXTERN void Trav_MgrSetIgrSide(
  Trav_Mgr_t * travMgr,
  int val
);
EXTERN void Trav_MgrSetIgrGrowCone(
  Trav_Mgr_t * travMgr,
  int val
);
EXTERN void Trav_MgrSetAbstrRef(
  Trav_Mgr_t * travMgr,
  int abstrRef
);
EXTERN void Trav_MgrSetAbstrRefGla(
  Trav_Mgr_t * travMgr,
  int abstrRefGla
);
EXTERN void Trav_MgrSetOptionList(
  Trav_Mgr_t * travMgr,
  Pdtutil_OptList_t * pkgOpt
);
EXTERN void Trav_MgrSetOptionItem(
  Trav_Mgr_t * travMgr,
  Pdtutil_OptItem_t optItem
);
EXTERN void Trav_MgrReadOption(
  Trav_Mgr_t * travMgr,
  Pdt_OptTagTrav_e travOpt,
  void *optRet
);
EXTERN int Trav_TravBddExactForwVerify(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bddarray_t * delta
);
EXTERN int Trav_TravBddApproxForwExactBckFixPointVerify(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  Ddi_Bddarray_t * delta
);
EXTERN int Trav_TravBddApproxBackwExactForwVerify(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec
);
EXTERN int Trav_TravBddApproxForwExactBackwVerify(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  int doFwd
);
EXTERN int Trav_TravBddApproxForwApproxBckExactForwVerify(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  int doCoi,
  int doApproxBck
);
EXTERN Ddi_Bdd_t *Trav_TravSatBckVerif(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  int maxIter,
  int doApprox,
  int lemmasSteps
);
EXTERN int Trav_TravSatFwdVerif(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  int noCheck,
  int initBound,
  int lemmaSteps,
  int lemmaSimulDepth,
  int lemmaMaxLevel
);
EXTERN int Trav_TravSatFwdExactVerif(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  int noCheck,
  int initBound,
  int lemmaSteps,
  int lemmaSimulDepth,
  int lemmaMaxLevel
);
EXTERN int Trav_TravSatFwdInterpolantVerif(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  int noCheck,
  int initBound,
  int lemmasSteps
);
EXTERN int Trav_TravSatBwdInterpolantVerif(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  int noCheck,
  int initBound,
  int lemmasSteps
);
EXTERN Ddi_Bdd_t *Trav_TravSatFwdApprox(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar
);
EXTERN Ddi_Bdd_t *Trav_TravSatBckAllSolVerif(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int maxIter
);
EXTERN int Trav_TravSatBmcVerif(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int bound,
  int step,
  int first,
  int doApprox,
  int lemmaSteps,
  int interpolantSteps,
  int timeLimit
);
EXTERN Ddi_Bdd_t *Trav_TravSatBmcIncrVerif(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bddarray_t * prevCexes,
  int bound,
  int step,
  int learnStep,
  int first,
  int doReturnCex,
  int lemmaSteps,
  int interpolantSteps,
  int strategy,
  int teSteps,
  int timeLimit
);
EXTERN Ddi_Bdd_t *Trav_TravSatBmcIncrVerifFwdBwd(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int bound,
  int step,
  int learnStep,
  int first,
  int doReturnCex,
  int lemmaSteps,
  int interpolantSteps,
  int strategy,
  int teSteps,
  int timeLimit
);
EXTERN int Trav_TravSatBmcVerif2(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int bound,
  int step,
  int first,
  int doApprox,
  int lemmaSteps,
  int interpolantSteps,
  int timeLimit
);
EXTERN void Trav_TravSatItpSeqVerif(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int seqK,
  int seqGroup,
  float seqSerial,
  int cegar,
  int fwdBwdFp
);
EXTERN void Trav_TravSatBmcTrVerif(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int bound,
  int step
);
EXTERN void Trav_TravSatBmcTrVerif_ForwardInsert(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int bound,
  int step
);
EXTERN int Trav_TravSatInductiveTrVerif(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * externalCare,
  int step,
  int first,
  int bound,
  int timeLimit
);
EXTERN int Trav_TravSatDiameterVerif(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int step,
  int bound
);
EXTERN int Trav_TravSatFwdDiameterVerif(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int step,
  int bound
);
EXTERN int
Trav_TravSatCheckProof(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Fsm_Mgr_t * fsmMgr,
  Fsm_Mgr_t * fsmMgrRef,
  char *wFsm
);
EXTERN int Trav_TravSatCheckInvar(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t *invar,
  int *checkTargetSatP
);
EXTERN int Trav_TravTrAbstrItp(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Fsm_Mgr_t * fsmMgr,
  int nFrames,
  int firstFwdStep,
  int maxFwdStep,
  int bmcBound
);
EXTERN int Trav_TravSatItpGfp(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  int gfp,
  int countR
);
EXTERN int Trav_TravSatItpVerif(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * lemma,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  int step,
  int bound,
  float lazyRate,
  int firstFwdIter,
  int timeLimit
);
EXTERN int Trav_TravIgrPdrVerif(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Fsm_Mgr_t * fsmMgr,
  int timeLimit
);
EXTERN Ddi_Bdd_t *Trav_DeepestRingCex(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * myTarget,
  Ddi_Bdd_t * invarspec,
  Ddi_Bddarray_t * fromRings,
  int start_i,
  int genCubes,
  Ddi_Varset_t *hintVars,
  int *fullTarget,
  int doRunItp,
  int subsetByAntecedents
);
EXTERN int Trav_TravSatQbfVerif(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  int step,
  int bound,
  float lazyRate
);
EXTERN int Trav_TravLemmaReduction(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  int lemmasSteps,
  int doImpl,
  int execMode,
  char *fsmName
);
EXTERN Ddi_Vararray_t *Trav_RefPiVars(
  Trav_Mgr_t * travMgr,
  Ddi_Vararray_t * vars
);
Ddi_Bddarray_t *Trav_SimulateAndCheckProp(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * delta,
  Ddi_Bdd_t * deltaPropPart,
  Ddi_Bdd_t * init,
  Ddi_Bddarray_t * initStub,
  Ddi_Bdd_t * cex
);
EXTERN void Trav_SimulateMain(
  Fsm_Mgr_t * fsmMgr,
  int iterNumberMax,
  int deadEndNumberOf,
  int logPeriod,
  int simulationFlag,
  int depthBreadth,
  int random,
  char *init,
  char *pattern,
  char *result
);
EXTERN Ddi_Bdd_t *Trav_Traversal(
  Trav_Mgr_t * travMgr
);
EXTERN Ddi_Bddarray_t *Trav_MismatchPat(
  Trav_Mgr_t * travMgr,
  Tr_Tr_t * TR,
  Ddi_Bdd_t * firstC,
  Ddi_Bdd_t * lastC,
  Ddi_Bdd_t ** startp,
  Ddi_Bdd_t ** endp,
  Ddi_Bddarray_t * newi,
  Ddi_Vararray_t * psv,
  Ddi_Vararray_t * nsv,
  Ddi_Varset_t * pivars
);
EXTERN Ddi_Bddarray_t *Trav_UnivAlignPat0(
  Trav_Mgr_t * travMgr,
  Tr_Tr_t * TR,
  Ddi_Bdd_t * goal,
  Ddi_Bdd_t ** endp,
  Ddi_Bdd_t * alignSet,
  Ddi_Bddarray_t * rings,
  Ddi_Vararray_t * psv,
  Ddi_Vararray_t * nsv,
  Ddi_Varset_t * pivars,
  int maxDepth
);
EXTERN Ddi_Bddarray_t *Trav_UnivAlignPat(
  Trav_Mgr_t * travMgr,
  Tr_Tr_t * TR,
  Ddi_Bdd_t * goal,
  Ddi_Bdd_t ** endp,
  Ddi_Bdd_t * alignSet,
  Ddi_Bddarray_t * rings,
  Ddi_Vararray_t * psv,
  Ddi_Vararray_t * nsv,
  Ddi_Varset_t * pivars,
  int maxDepth
);
EXTERN Trav_FromSelect_e Trav_FromSelectString2Enum(
  char *string
);
EXTERN char *Trav_FromSelectEnum2String(
  Trav_FromSelect_e enumType
);
EXTERN Ddi_Bdd_t *Trav_TravPdrVerif(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  int maxIter,
  int timeLimit
);
EXTERN int Trav_TravSatIgrVerif(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  int step,
  int bound,
  float lazyRate,
  int firstFwdIter,
  int timeLimit
);
EXTERN int Trav_PersistentTarget(
  Fsm_Mgr_t * fsmMgr,
  Trav_Mgr_t * travMgr,
  Ddi_Bdd_t * target,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * ns,
  Ddi_Bddarray_t * delta
);
EXTERN Ddi_Bdd_t *Trav_TravSatBmcRefineCex(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * cex
);
EXTERN int Trav_SimulateRarity(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr
);
EXTERN int Trav_LogInit(
  Trav_Mgr_t * travMgr,
  int nArg,
  ...
);
EXTERN int Trav_LogStats(
  Trav_Mgr_t * travMgr,
  char *tag,
  int val,
  int logCpu
);
EXTERN int Trav_LogReadTotCpu(
  Trav_Stats_t * travStats
);
EXTERN int Trav_LogReadSize(
  Trav_Stats_t * travStats
);
EXTERN int Trav_LogReadStats(
  Trav_Stats_t * travStats,
  char *tag,
  int k
);
EXTERN Trav_ItpMgr_t *Trav_ItpMgrInit(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * invarspec,
  int newTrPis,
  int optLevel
);
EXTERN void Trav_ItpMgrQuit(
  Trav_ItpMgr_t * itpMgr
);
EXTERN void
Trav_ItpMgrSetConeBoundOK(
  Trav_ItpMgr_t * itpMgr,
  int ring_i,
  int cone_k
);
EXTERN int
Trav_ItpMgrResetConeBoundOK(
  Trav_ItpMgr_t * itpMgr
);
EXTERN int
Trav_ItpMgrReadConeBoundOK(
  Trav_ItpMgr_t * itpMgr,
  int ring_i
);
EXTERN int Trav_IgrTrav(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Trav_ItpMgr_t * itpMgr,
  Ddi_Bdd_t * itpCone,
  Ddi_Bdd_t * itpCare,
  Ddi_Bdd_t * rOut,
  Ddi_Bdd_t * careOut,
  int bwdK,
  int recLevel,
  int *activeInterpolantP,
  int *abortP,
  int *satP
);
EXTERN int
Trav_PdrToIgr (
  Trav_Mgr_t * travMgrPdr,
  Trav_Mgr_t * travMgrIgr
);
EXTERN int
Trav_IgrToPdr (
  Trav_Mgr_t * travMgrIgr,
  Trav_Mgr_t * travMgrPdr
);
EXTERN void
Trav_SimplifyRingsBwdFwd (
  Trav_ItpMgr_t *itpMgr,
  int start_i,
  int maxIter,
  int maxFail,
  int useRingConstr,
  int boundK,
  int doFwd
);
EXTERN void
Trav_TravBmcSharedInit(void);
EXTERN void
Trav_TravBmcSharedQuit(void);
EXTERN int
Trav_TravBmcSharedSet(
  int k,
  Trav_BmcBoundState_e state
);
EXTERN Trav_BmcBoundState_e 
Trav_TravBmcSharedRead(
  int k
);
EXTERN Trav_BmcBoundState_e 
Trav_TravBmcSharedReadAndSet(
  int k,
  Trav_BmcBoundState_e state
);
EXTERN int
Trav_TravBmcSharedMaxSafe(void);
EXTERN int
Trav_TravBmcSharedActive(void);
EXTERN int
Trav_TravBmcSharedRegisterThread(void);



/**AutomaticEnd***************************************************************/


#endif                          /* _TRAV */






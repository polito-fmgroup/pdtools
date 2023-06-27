/**CFile***********************************************************************

  FileName    [travMgr.c]

  PackageName [trav]

  Synopsis    [Functions to handle a Trav manager]

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

#include "travInt.h"
#include <stdarg.h>

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


/**Function********************************************************************
  Synopsis    [Create a Manager.]
  Description []
  SideEffects [none]
  SeeAlso     [Ddi_MgrQuit, Fsm_MgrQuit, Tr_MgrQuit]
******************************************************************************/
Trav_Mgr_t *
Trav_MgrInit(
  char *travName /* Name of the Trav structure */ ,
  Ddi_Mgr_t * dd                /* ddi manager */
)
{
  Trav_Mgr_t *travMgr;

  /*------------------------- Allocation -----------------------------*/

  travMgr = Pdtutil_Alloc(Trav_Mgr_t, 1);
  if (travMgr == NULL) {
    fprintf(stderr, "Error: Out of Memory (in travMgr.c).\n");
    return (NULL);
  }

  /*------------------------ Initializations -------------------------*/

  if (travName == NULL) {
    travName = "travMgr";
  }
  travMgr->travName = Pdtutil_StrDup(travName);

  travMgr->ddiMgrTr = dd;
  travMgr->ddiMgrR = dd;
  travMgr->ddiMgrAux = NULL;
  travMgr->tr = NULL;
  travMgr->i = NULL;
  travMgr->ns = NULL;
  travMgr->ps = NULL;
  travMgr->aux = NULL;
  travMgr->from = NULL;
  travMgr->reached = NULL;
  travMgr->care = NULL;
  travMgr->assume = NULL;
  travMgr->proved = NULL;
  travMgr->assert = NULL;
  travMgr->newi = NULL;
  travMgr->cexes = NULL;
  travMgr->pdtSpecVar = NULL;
  travMgr->pdtConstrVar = NULL;
  travMgr->level = 1;
  travMgr->productPeak = 0;
  travMgr->assertFlag = 0;
  travMgr->travTime = 0;
  travMgr->startTime = 0;

  /*---------------------- Default Settings --------------------------*/

  /* general */
  travMgr->settings.verbosity = Pdtutil_VerbLevelUsrMax_c;
  travMgr->settings.stdout = stdout;
  travMgr->settings.logPeriod = 1;
  travMgr->settings.clk = NULL;
  travMgr->settings.doCex = 0;

  /* unused */
  travMgr->settings.removeLLatches = 0;
  travMgr->settings.mgrAuxFlag = 0;
  travMgr->settings.enableDdR = 0;
  travMgr->settings.smoothVars = TRAV_SMOOTH_SX;
  travMgr->settings.backward = 0;
  travMgr->settings.constrLevel = 0;    // 1
  travMgr->settings.maxIter = -1;
  travMgr->settings.savePeriod = -1;
  travMgr->settings.savePeriodName = NULL;
  travMgr->settings.iwls95Variant = 0;

  /* tr */
  travMgr->settings.tr.trProfileMethod = Cuplus_None;
  travMgr->settings.tr.trProfileDynamicEnable = 0;
  travMgr->settings.tr.trProfileThreshold = -1;
  travMgr->settings.tr.trSorting = 0;
  travMgr->settings.tr.w1 = TRSORT_DEFAULT_W1;
  travMgr->settings.tr.w2 = TRSORT_DEFAULT_W2;
  travMgr->settings.tr.w3 = TRSORT_DEFAULT_W3;
  travMgr->settings.tr.w4 = TRSORT_DEFAULT_W4;
  travMgr->settings.tr.sortForBck = 1;
  travMgr->settings.tr.trPreimgSort = 0;
  travMgr->settings.tr.partDisjTrThresh = -1;
  travMgr->settings.tr.clustThreshold = TRAV_CLUST_THRESH;
  travMgr->settings.tr.apprClustTh = TRAV_CLUST_THRESH;
  travMgr->settings.tr.imgClustTh = TRAV_IMG_CLUST_THRESH;
  travMgr->settings.tr.squaring = 0;
  travMgr->settings.tr.trProj = 0;
  travMgr->settings.tr.trDpartTh = 0;
  travMgr->settings.tr.trDpartVar = NULL;
  travMgr->settings.tr.auxVarFile = NULL;
  travMgr->settings.tr.enableVar = NULL;
  travMgr->settings.tr.approxTrClustFile = NULL;
  travMgr->settings.tr.bwdTrClustFile = NULL;

  /* aig */
  travMgr->settings.aig.satSolver = Pdtutil_StrDup("minisat");
  travMgr->settings.aig.abcOptLevel = 3;
  travMgr->settings.aig.targetEn = 0;
  travMgr->settings.aig.dynAbstr = 0;
  travMgr->settings.aig.dynAbstrInitIter = 0;
  travMgr->settings.aig.implAbstr = 0;
  travMgr->settings.aig.ternaryAbstr = 0;
  travMgr->settings.aig.abstrRef = 0;
  travMgr->settings.aig.abstrRefGla = 0;
  travMgr->settings.aig.storeAbstrRefRefinedVars = NULL;
  travMgr->settings.aig.inputRegs = 0;
  travMgr->settings.aig.selfTuningLevel = 0;    // 1
  travMgr->settings.aig.lemmasTimeLimit = 60.0; // 120.0
  travMgr->settings.aig.lazyTimeLimit = 60.0;   // 120.0

  /* itp */
  travMgr->settings.aig.itpBdd = 0;
  travMgr->settings.aig.itpPart = 0;
  travMgr->settings.aig.itpPartCone = 0;
  travMgr->settings.aig.itpAppr = 0;
  travMgr->settings.aig.itpRefineCex = 0;
  travMgr->settings.aig.itpStructAbstr = 0;
  travMgr->settings.aig.itpNew = 0;
  travMgr->settings.aig.itpExact = 0;
  travMgr->settings.aig.itpInductiveTo = 0;
  travMgr->settings.aig.itpInnerCones = 0;
  travMgr->settings.aig.itpTrAbstr = 0;
  travMgr->settings.aig.itpInitAbstr = 1;
  travMgr->settings.aig.itpEndAbstr = 0;
  travMgr->settings.aig.itpReuseRings = 0;
  travMgr->settings.aig.itpTuneForDepth = 0;
  travMgr->settings.aig.itpBoundkOpt = 0;
  travMgr->settings.aig.itpExactBoundDouble = 0;
  travMgr->settings.aig.itpConeOpt = 0;
  travMgr->settings.aig.itpForceRun = -1;
  travMgr->settings.aig.itpMaxStepK = -1;
  travMgr->settings.aig.itpUseReached = 0;
  travMgr->settings.aig.itpCheckCompleteness = 0;
  travMgr->settings.aig.itpGfp = 0;
  travMgr->settings.aig.itpConstrLevel = 0; // 4
  travMgr->settings.aig.itpGenMaxIter = 0;
  travMgr->settings.aig.itpEnToPlusImg = 1;
  travMgr->settings.aig.itpShareReached = 0;
  travMgr->settings.aig.itpUsePdrReached = 0;
  travMgr->settings.aig.itpReImg = 0;
  travMgr->settings.aig.itpWeaken = 0;
  travMgr->settings.aig.itpStrengthen = 0;
  travMgr->settings.aig.itpFromNewLevel = 0;
  travMgr->settings.aig.itpRpm = 0;
  travMgr->settings.aig.itpTimeLimit = 0;
  travMgr->settings.aig.itpPeakAig = 0;

  /* igr */
  travMgr->settings.aig.igrSide = 1;
  travMgr->settings.aig.igrItpSeq = 0;
  travMgr->settings.aig.igrRestart = 0;
  travMgr->settings.aig.igrRewind = 1;
  travMgr->settings.aig.igrRewindMinK = -1;
  travMgr->settings.aig.igrBwdRefine = 0;
  travMgr->settings.aig.igrGrowCone = 3;
  travMgr->settings.aig.igrGrowConeMaxK = -1;
  travMgr->settings.aig.igrGrowConeMax = 0.5;
  travMgr->settings.aig.igrUseRings = 0;
  travMgr->settings.aig.igrUseRingsStep = 0;
  travMgr->settings.aig.igrUseBwdRings = 0;
  travMgr->settings.aig.igrAssumeSafeBound = 0;
  travMgr->settings.aig.igrMaxIter = 16;
  travMgr->settings.aig.igrMaxExact = 10;
  travMgr->settings.aig.igrFwdBwd = 0;
  travMgr->settings.aig.igrConeSubsetPiRatio = 0.5;
  travMgr->settings.aig.igrConeSplitRatio = 1.0;

  /* pdr */
  travMgr->settings.aig.pdrFwdEq = 0;
  travMgr->settings.aig.pdrInf = 0;
  travMgr->settings.aig.pdrUnfold = 1;
  travMgr->settings.aig.pdrIndAssumeLits = 0;
  travMgr->settings.aig.pdrPostordCnf = 0;
  travMgr->settings.aig.pdrCexJustify = 1;
  travMgr->settings.aig.pdrGenEffort = 3;
  travMgr->settings.aig.pdrIncrementalTr = 1;
  travMgr->settings.aig.pdrShareReached = 0;
  travMgr->settings.aig.pdrReuseRings = 0;
  travMgr->settings.aig.pdrMaxBlock = 0;
  travMgr->settings.aig.pdrUsePgEncoding = 0;
  travMgr->settings.aig.pdrBumpActTopologically = 0;
  travMgr->settings.aig.pdrSpecializedCallsMask = 0;
  travMgr->settings.aig.pdrRestartStrategy = 0;
  travMgr->settings.aig.pdrTimeLimit = 0;

  /* bmc */
  travMgr->settings.aig.bmcTimeLimit = 0;
  travMgr->settings.aig.bmcMemLimit = 0;

  /* bdd */
  travMgr->settings.bdd.fromSelect = Trav_FromSelectCofactor_c;
  travMgr->settings.bdd.underApproxMaxVars = -1;
  travMgr->settings.bdd.underApproxMaxSize = -1;
  travMgr->settings.bdd.underApproxTargetReduction = 1.0;
  travMgr->settings.bdd.noCheck = 0;
  travMgr->settings.bdd.noMismatch = 1;
  travMgr->settings.bdd.mismatchOptLevel = 1;
  travMgr->settings.bdd.groupBad = 0;
  travMgr->settings.bdd.keepNew = 0;
  travMgr->settings.bdd.prioritizedMc = 0;
  travMgr->settings.bdd.pmcPrioThresh = 5000;
  travMgr->settings.bdd.pmcMaxNestedFull = 100;
  travMgr->settings.bdd.pmcMaxNestedPart = 5;
  travMgr->settings.bdd.pmcNoSame = 0;
  travMgr->settings.bdd.countReached = 0;
  travMgr->settings.bdd.autoHint = 0;
  travMgr->settings.bdd.hintsStrategy = 0;
  travMgr->settings.bdd.hintsTh = 1000;
  travMgr->settings.bdd.hints = NULL;
  travMgr->settings.bdd.auxPsSet = NULL;
  travMgr->settings.bdd.gfpApproxRange = 1;
  travMgr->settings.bdd.useMeta = 0;
  travMgr->settings.bdd.metaMethod = Ddi_Meta_Size;
  travMgr->settings.bdd.metaMaxSmooth = 0;
  travMgr->settings.bdd.constrain = 0;
  travMgr->settings.bdd.constrainLevel = 1;
  travMgr->settings.bdd.checkFrozen = 0;
  travMgr->settings.bdd.apprOverlap = 0;
  travMgr->settings.bdd.apprNP = 0;
  travMgr->settings.bdd.apprMBM = 0;
  travMgr->settings.bdd.apprGrTh = 0;
  travMgr->settings.bdd.apprPreimgTh = 500000;
  travMgr->settings.bdd.maxFwdIter = -1;
  travMgr->settings.bdd.nExactIter = 0;
  travMgr->settings.bdd.siftMaxTravIter = -1;
  travMgr->settings.bdd.implications = 0;
  travMgr->settings.bdd.imgDpartTh = 0;
  travMgr->settings.bdd.imgDpartSat = 0;
  travMgr->settings.bdd.bound = -1;
  travMgr->settings.bdd.strictBound = 0;
  travMgr->settings.bdd.guidedTrav = 0;
  travMgr->settings.bdd.univQuantifyTh = -1;
  travMgr->settings.bdd.optImg = 0;
  travMgr->settings.bdd.optPreimg = 0;
  travMgr->settings.bdd.wP = NULL;
  travMgr->settings.bdd.wR = NULL;
  travMgr->settings.bdd.wBR = NULL;
  travMgr->settings.bdd.wU = NULL;
  travMgr->settings.bdd.wOrd = NULL;
  travMgr->settings.bdd.rPlus = NULL;
  travMgr->settings.bdd.bddTimeLimit = 0;
  travMgr->settings.bdd.bddNodeLimit = 0;
  travMgr->settings.bdd.invarFile = NULL;
  travMgr->settings.bdd.storeCNF = NULL;
  travMgr->settings.bdd.storeCNFTR = -1;
  travMgr->settings.bdd.storeCNFmode = 'B';
  travMgr->settings.bdd.storeCNFphase = 0;
  travMgr->settings.bdd.maxCNFLength = 20;

  /* internal fields */
  travMgr->settings.ints.saveClipLevel = 0;
  travMgr->settings.ints.auxFwdTr = 0;
  travMgr->settings.ints.itpMgr = NULL;
  travMgr->settings.ints.pdrMgr = NULL;
  travMgr->settings.ints.univQuantifyDone = 0;
  travMgr->settings.ints.univQuantifyVars = NULL;
  travMgr->settings.ints.abstrRefRefinedVars = NULL;
  travMgr->settings.ints.nPdrFrames = 0;
  travMgr->settings.ints.fixPointTimeFrame = 0;
  travMgr->settings.ints.doAbstrRefinedTo = 0;
  travMgr->settings.ints.suspendFrame = 0;
  travMgr->settings.ints.timeFrameClauses = NULL;
  travMgr->settings.ints.itpBmcBoundFirst = -1;
  travMgr->settings.ints.igrSingleRewind = -1;
  travMgr->settings.ints.igrRingFirst = -1;
  travMgr->settings.ints.igrRingLast = -1;
  travMgr->settings.ints.igrFpRing = -1;

  /*------------------------ Other Fields ----------------------------*/

  travMgr->xchg.enabled = 0;

  travMgr->xchg.tune.doAbort = 0;
  travMgr->xchg.tune.doSleep = 0;
  travMgr->xchg.stats.arrayStats = NULL;
  travMgr->xchg.stats.arrayStatsNum = 0;
  travMgr->xchg.stats.lastObserved = -1;
  travMgr->xchg.stats.provedBound = -1;

  travMgr->xchg.shared.clauseShared = NULL;
  travMgr->xchg.shared.clauseSharedNum = NULL;
  travMgr->xchg.shared.itpRchShared = NULL;
  travMgr->xchg.shared.ddmXchg = NULL;
  travMgr->xchg.shared.sharedLink = NULL;

  pthread_mutex_init(&travMgr->xchg.tune.mutex, NULL);
  pthread_mutex_init(&travMgr->xchg.stats.mutex, NULL);
  pthread_mutex_init(&travMgr->xchg.shared.mutex, NULL);

  /*---------------------- Printing Message --------------------------*/

  Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "-- TRAV Manager Init.\n")
    );

  return (travMgr);
}

/**Function********************************************************************
  Synopsis    [Closes a Manager.]
  Description []
  SideEffects [none]
  SeeAlso     [Ddi_BddiMgrInit]
******************************************************************************/
Trav_Mgr_t *
Trav_MgrQuit(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  Pdtutil_VerbosityMgr(travMgr,
    Pdtutil_VerbLevelUsrMax_c, fprintf(stdout, "-- TRAV Manager Quit.\n")
    );


  if (travMgr->settings.ints.pdrMgr != NULL) {
    //Quit PDR Manager
    Trav_PdrMgrQuit(travMgr->settings.ints.pdrMgr);
  }
  if (travMgr->settings.ints.itpMgr != NULL) {
    //Quit ITP Manager
    Trav_ItpMgrQuit(travMgr->settings.ints.itpMgr);
  }

  Pdtutil_Free(travMgr->travName);

  Ddi_Free(travMgr->i);
  Ddi_Free(travMgr->ps);
  Ddi_Free(travMgr->ns);

  Tr_TrFree(travMgr->tr);
  Ddi_Free(travMgr->from);
  Ddi_Free(travMgr->reached);
  Ddi_Free(travMgr->care);
  Ddi_Free(travMgr->assume);
  Ddi_Free(travMgr->proved);

  Ddi_Free(travMgr->assert);

  Ddi_Free(travMgr->newi);
  Ddi_Free(travMgr->cexes);

  Ddi_Free(travMgr->settings.ints.univQuantifyVars);
  Ddi_Free(travMgr->settings.ints.abstrRefRefinedVars);
  Pdtutil_Free(travMgr->settings.aig.storeAbstrRefRefinedVars);

  Pdtutil_Free(travMgr->settings.clk);
  Pdtutil_Free(travMgr->settings.savePeriodName);
  Pdtutil_Free(travMgr->settings.tr.trDpartVar);
  Pdtutil_Free(travMgr->settings.tr.auxVarFile);
  Pdtutil_Free(travMgr->settings.aig.satSolver);
  Ddi_Free(travMgr->settings.bdd.hints);
  Ddi_Free(travMgr->settings.bdd.auxPsSet);
  Pdtutil_Free(travMgr->settings.bdd.wP);
  Pdtutil_Free(travMgr->settings.bdd.wR);
  Pdtutil_Free(travMgr->settings.bdd.wBR);
  Pdtutil_Free(travMgr->settings.bdd.wU);
  Pdtutil_Free(travMgr->settings.bdd.wOrd);
  Pdtutil_Free(travMgr->settings.bdd.rPlus);

  if (travMgr->xchg.stats.arrayStats != NULL) {
    int i;

    for (i = 0; i < travMgr->xchg.stats.arrayStatsNum; i++) {
      Pdtutil_IntegerArrayFree(travMgr->xchg.stats.arrayStats[i]);
    }
  }

  if (travMgr->xchg.shared.clauseShared != NULL) {
    Ddi_ClauseArrayFree(travMgr->xchg.shared.clauseShared);
    Pdtutil_IntegerArrayFree(travMgr->xchg.shared.clauseSharedNum);
  }
  Ddi_Free(travMgr->xchg.shared.itpRchShared);
  if (travMgr->xchg.shared.ddmXchg != NULL) {
    Ddi_MgrQuit(travMgr->xchg.shared.ddmXchg);
  }

  pthread_mutex_destroy(&travMgr->xchg.stats.mutex);
  pthread_mutex_destroy(&travMgr->xchg.shared.mutex);

  Pdtutil_Free(travMgr);

  return NULL;
}

/**Function********************************************************************
  Synopsis           [Print Statistics on the Traversal Manager.]
  Description        []
  SideEffects        []
  SeeAlso            []
******************************************************************************/
int
Trav_MgrPrintStats(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Traversal Manager Name %s.\n",
      travMgr->travName);
    fprintf(stdout, "Total CPU Time: %s.\n",
      util_print_time(travMgr->travTime))
    );

  return (1);
}

/**Function********************************************************************
  Synopsis    [Perform an Operation on the Traversal Manager.]
  Description []
  SideEffects []
  SeeAlso     [CmdMgrOperation, CmdRegOperation, Fsm_MgrOperation,
    Tr_MgrOperation]
******************************************************************************/
int
Trav_MgrOperation(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  char *string /* String */ ,
  Pdtutil_MgrOp_t operationFlag /* Operation Flag */ ,
  void **voidPointer /* Generic Pointer */ ,
  Pdtutil_MgrRet_t * returnFlagP    /* Type of the Pointer Returned */
)
{
  char *stringFirst, *stringSecond;
  Ddi_Mgr_t *defaultDdMgr, *tmpDdMgr;

  /*------------ Check for the Correctness of the Command Sequence ----------*/

  if (travMgr == NULL) {
    Pdtutil_Warning(1, "Command Out of Sequence.");
    return (1);
  }

  defaultDdMgr = Trav_MgrReadDdiMgrDefault(travMgr);

  /*----------------------- Take Main and Secondary Name --------------------*/

  Pdtutil_ReadSubstring(string, &stringFirst, &stringSecond);

  /*----------------------- Operation on the TRAV Manager -------------------*/

  if (stringFirst == NULL) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionShow_c:
#if 0
        Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Traversal Manager Verbosity: %s (%d).\n",
            Pdtutil_VerbosityEnum2String(Trav_MgrReadVerbosity(travMgr)),
            Trav_MgrReadVerbosity(travMgr));
          fprintf(stdout, "Traversal Manager Log Period: %d.\n",
            Trav_MgrReadLogPeriod(travMgr));
          fprintf(stdout, "Traversal Manager Save Period: %d.\n",
            Trav_MgrReadSavePeriod(travMgr));
          fprintf(stdout, "Traversal Manager Save Period Name: %s.\n",
            Trav_MgrReadSavePeriodName(travMgr));
          fprintf(stdout, "Traversal Manager Backward: %d.\n",
            Trav_MgrReadBackward(travMgr));
          fprintf(stdout, "Traversal Manager keep New: %d.\n",
            Trav_MgrReadKeepNew(travMgr));
          fprintf(stdout, "Traversal Manager Maximum Iteration: %d.\n",
            Trav_MgrReadMaxIter(travMgr));
          fprintf(stdout, "Traversal Manager Mismatch Opt Level: %d.\n",
            Trav_MgrReadMismatchOptLevel(travMgr));
          fprintf(stdout,
            "Traversal Manager underApproximate Maximum Variables: %d.\n",
            Trav_MgrReadUnderApproxMaxVars(travMgr));
          fprintf(stdout, "TraversalManager underApproxMaxSize: %d.\n",
            Trav_MgrReadUnderApproxMaxSize(travMgr));
          fprintf(stdout,
            "Traversal Manager underApproximate Target Reduction: %f.\n",
            Trav_MgrReadUnderApproxTargetReduction(travMgr));
          fprintf(stdout, "Traversal Manager From Select: %s (%d).\n",
            Trav_FromSelectEnum2String(Trav_MgrReadFromSelect(travMgr)),
            Trav_MgrReadFromSelect(travMgr))
          );
#endif
        break;
      case Pdtutil_MgrOpStats_c:
        Trav_MgrPrintStats(travMgr);
        break;
      case Pdtutil_MgrOpMgrRead_c:
        *voidPointer = (void *)Trav_MgrReadDdiMgrDefault(travMgr);
        *returnFlagP = Pdtutil_MgrRetDdMgr_c;
        break;
      case Pdtutil_MgrOpMgrSet_c:
        tmpDdMgr = (Ddi_Mgr_t *) voidPointer;
        Trav_MgrSetDdiMgrDefault(travMgr, tmpDdMgr);
        break;
      default:
        Pdtutil_Warning(1, "Operation Non Allowed on TRAV Manager");
        break;
    }
    return (0);
  }

  /*----------------------------- Package is DDI ----------------------------*/

  if (strcmp(stringFirst, "ddmr") == 0) {
    Ddi_MgrOperation(&(travMgr->ddiMgrR), stringSecond, operationFlag,
      voidPointer, returnFlagP);

    if (*returnFlagP == Pdtutil_MgrRetNone_c) {
      travMgr->ddiMgrR = NULL;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  if (strcmp(stringFirst, "ddmtr") == 0) {
    Ddi_MgrOperation(&(travMgr->ddiMgrTr), stringSecond, operationFlag,
      voidPointer, returnFlagP);

    if (*returnFlagP == Pdtutil_MgrRetNone_c) {
      travMgr->ddiMgrTr = NULL;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*-------------------------------- Options --------------------------------*/

#if 0
  if (strcmp(stringFirst, "verbosity") == 0) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionSet_c:
        Trav_MgrSetOption(travMgr,
          Pdt_TravVerbosity_c, inum,
          Pdtutil_VerbosityString2Enum(*(char **)voidPointer));
        break;
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Verbosity Level: %s (%d).\n",
            Pdtutil_VerbosityEnum2String(Trav_MgrReadVerbosity(travMgr)),
            Trav_MgrReadVerbosity(travMgr))
          );
        break;
      default:
        Pdtutil_Warning(1, "Operation Non Allowed on TRAV Manager");
        break;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  if (strcmp(stringFirst, "currLevel") == 0) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionSet_c:
        travMgr->level = atoi(*(char **)voidPointer);
        break;
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Current Level: %d.\n", travMgr->level)
          );
        break;
      default:
        Pdtutil_Warning(1, "Operation Non Allowed on TRAV Manager");
        break;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  if (strcmp(stringFirst, "logPeriod") == 0) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionSet_c:
        Trav_MgrSetLogPeriod(travMgr, atoi(*(char **)voidPointer));
        break;
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Log Period: %d.\n", Trav_MgrReadLogPeriod(travMgr))
          );
        break;
      default:
        Pdtutil_Warning(1, "Operation Non Allowed on TRAV Manager");
        break;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  if (strcmp(stringFirst, "savePeriod") == 0) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionSet_c:
        Trav_MgrSetSavePeriod(travMgr, atoi(*(char **)voidPointer));
        break;
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Save Period: %d.\n",
            Trav_MgrReadSavePeriod(travMgr))
          );
        break;
      default:
        Pdtutil_Warning(1, "Operation Non Allowed on TRAV Manager");
        break;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  if (strcmp(stringFirst, "savePeriodName") == 0) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionSet_c:
        Trav_MgrSetSavePeriodName(travMgr, (char *)*voidPointer);
        break;
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Save Period: %s.\n",
            Trav_MgrReadSavePeriodName(travMgr))
          );
        break;
      default:
        Pdtutil_Warning(1, "Operation Non Allowed on TRAV Manager");
        break;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  if (strcmp(stringFirst, "maxIter") == 0) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionSet_c:
        Trav_MgrSetMaxIter(travMgr, atoi(*(char **)voidPointer));
        break;
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Maximum Iteration: %d.\n",
            Trav_MgrReadMaxIter(travMgr))
          );
        break;
      default:
        Pdtutil_Warning(1, "Operation Non Allowed on TRAV Manager");
        break;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  if (strcmp(stringFirst, "backward") == 0) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionSet_c:
        Trav_MgrSetBackward(travMgr, atoi(*(char **)voidPointer));
        break;
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Backward: %d.\n", Trav_MgrReadBackward(travMgr))
          );
        break;
      default:
        Pdtutil_Warning(1, "Operation Non Allowed on TRAV Manager");
        break;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  if (strcmp(stringFirst, "keepNew") == 0) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionSet_c:
        Trav_MgrSetKeepNew(travMgr, atoi(*(char **)voidPointer));
        break;
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Keep New: %d.\n", Trav_MgrReadKeepNew(travMgr))
          );
        break;
      default:
        Pdtutil_Warning(1, "Operation Non Allowed on TRAV Manager");
        break;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  if (strcmp(stringFirst, "fromSelect") == 0) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionSet_c:
        Trav_MgrSetFromSelect(travMgr,
          Trav_FromSelectString2Enum(*(char **)voidPointer));
        break;
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "From Selection: %s (%d).\n",
            Trav_FromSelectEnum2String(Trav_MgrReadFromSelect(travMgr)),
            Trav_MgrReadFromSelect(travMgr))
          );
        break;
      default:
        Pdtutil_Warning(1, "Operation Non Allowed on TRAV Manager");
        break;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  if (strcmp(stringFirst, "trProfileDynamicEnable") == 0) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionSet_c:
        Trav_MgrSetTrProfileDynamicEnable(travMgr,
          atoi(*(char **)voidPointer));
        break;
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Profile Enable: %d.\n",
            Trav_MgrReadTrProfileDynamicEnable(travMgr))
          );
        break;
      default:
        Pdtutil_Warning(1, "Operation Non Allowed on TRAV Manager");
        break;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  if (strcmp(stringFirst, "trProfileThreshold") == 0) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionSet_c:
        Trav_MgrSetTrProfileThreshold(travMgr, atoi(*(char **)voidPointer));
        break;
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Profile Threshold: %d.\n",
            Trav_MgrReadTrProfileThreshold(travMgr))
          );
        break;
      default:
        Pdtutil_Warning(1, "Operation Non Allowed on TRAV Manager");
        break;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  if (strcmp(stringFirst, "trProfileMethod") == 0) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionSet_c:
        Trav_MgrSetTrProfileMethod(travMgr,
          Ddi_ProfileHeuristicString2Enum(*(char **)voidPointer));
        break;
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Profile Method: %s (%d).\n",
            Ddi_ProfileHeuristicEnum2String(Trav_MgrReadTrProfileMethod
              (travMgr)), Trav_MgrReadTrProfileMethod(travMgr))
          );
        break;
      default:
        Pdtutil_Warning(1, "Operation Non Allowed on TRAV Manager");
        break;
    }

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }
#endif

  /*------------------------------ Reached Set ------------------------------*/

  if (strcmp(stringFirst, "reached") == 0) {
    Ddi_BddOperation(defaultDdMgr, &(travMgr->reached), stringSecond,
      operationFlag, voidPointer, returnFlagP);

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*------------------------------ Care Set ---------------------------------*/

  if (strcmp(stringFirst, "care") == 0) {
    Ddi_BddOperation(defaultDdMgr, &(travMgr->care), stringSecond,
      operationFlag, voidPointer, returnFlagP);

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*------------------------------ Assume Set -------------------------------*/

  if (strcmp(stringFirst, "assume") == 0) {
    Ddi_BddOperation(defaultDdMgr, &(travMgr->assume), stringSecond,
      operationFlag, voidPointer, returnFlagP);

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*------------------------------ Assume Set -------------------------------*/

  if (strcmp(stringFirst, "proved") == 0) {
    Ddi_BddOperation(defaultDdMgr, &(travMgr->proved), stringSecond,
      operationFlag, voidPointer, returnFlagP);

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*-------------------------------- From Set -------------------------------*/

  if (strcmp(stringFirst, "from") == 0) {
    Ddi_BddOperation(defaultDdMgr, &(travMgr->from), stringSecond,
      operationFlag, voidPointer, returnFlagP);

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*------------------------------- Assertion -------------------------------*/

  if (strcmp(stringFirst, "assert") == 0) {
    Ddi_BddOperation(defaultDdMgr, &(travMgr->assert), stringSecond,
      operationFlag, voidPointer, returnFlagP);

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*------------------------------- Frontier sets ---------------------------*/

  if (strcmp(stringFirst, "newi") == 0) {
    Ddi_BddarrayOperation(defaultDdMgr, &(travMgr->newi), stringSecond,
      operationFlag, voidPointer, returnFlagP);

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*------------------------------- Frontier sets ---------------------------*/

  if (strcmp(stringFirst, "cexes") == 0) {
    Ddi_BddarrayOperation(defaultDdMgr, &(travMgr->cexes), stringSecond,
      operationFlag, voidPointer, returnFlagP);

    Pdtutil_Free(stringFirst);
    Pdtutil_Free(stringSecond);
    return (0);
  }

  /*------------------------------- No Match --------------------------------*/

  Pdtutil_Warning(1, "Operation on TRAV manager not allowed");
  return (1);
}


/**Function********************************************************************
  Synopsis           [Print Statistics on the Traversal Manager.]
  Description        []
  SideEffects        []
  SeeAlso            []
******************************************************************************/
void
Trav_MgrSetTravOpt(
  Trav_Mgr_t * travMgr,
  Trav_Settings_t * opt
)
{
  /* copy static fields */
  travMgr->settings = *opt;

  /* copy dynamic fields */
  travMgr->settings.clk = Pdtutil_StrDup(opt->clk);
  travMgr->settings.savePeriodName = Pdtutil_StrDup(opt->savePeriodName);

  travMgr->settings.tr.trDpartVar = Pdtutil_StrDup(opt->tr.trDpartVar);
  travMgr->settings.tr.auxVarFile = Pdtutil_StrDup(opt->tr.auxVarFile);
  travMgr->settings.tr.enableVar = Pdtutil_StrDup(opt->tr.enableVar);
  travMgr->settings.tr.approxTrClustFile =
    Pdtutil_StrDup(opt->tr.approxTrClustFile);
  travMgr->settings.tr.bwdTrClustFile = Pdtutil_StrDup(opt->tr.bwdTrClustFile);

  travMgr->settings.aig.satSolver = Pdtutil_StrDup(opt->aig.satSolver);
  travMgr->settings.aig.storeAbstrRefRefinedVars = Pdtutil_StrDup(opt->aig.storeAbstrRefRefinedVars);

  travMgr->settings.bdd.hints = Ddi_BddarrayDup(opt->bdd.hints);
  travMgr->settings.bdd.auxPsSet = Ddi_VarsetDup(opt->bdd.auxPsSet);
  travMgr->settings.bdd.wP = Pdtutil_StrDup(opt->bdd.wP);
  travMgr->settings.bdd.wR = Pdtutil_StrDup(opt->bdd.wR);
  travMgr->settings.bdd.wBR = Pdtutil_StrDup(opt->bdd.wBR);
  travMgr->settings.bdd.wU = Pdtutil_StrDup(opt->bdd.wU);
  travMgr->settings.bdd.wOrd = Pdtutil_StrDup(opt->bdd.wOrd);
  travMgr->settings.bdd.rPlus = Pdtutil_StrDup(opt->bdd.rPlus);
  travMgr->settings.bdd.invarFile = Pdtutil_StrDup(opt->bdd.invarFile);
  travMgr->settings.bdd.storeCNF = Pdtutil_StrDup(opt->bdd.storeCNF);

  /* keep empty internals for the moment */
  travMgr->settings.ints.auxFwdTr = NULL;
  travMgr->settings.ints.univQuantifyVars = NULL;
  travMgr->settings.ints.abstrRefRefinedVars = NULL;
  if (opt->ints.abstrRefRefinedVars != NULL)
    travMgr->settings.ints.abstrRefRefinedVars = Ddi_VarsetarrayDup(opt->ints.abstrRefRefinedVars);
  travMgr->settings.ints.timeFrameClauses = NULL;
}

/**Function********************************************************************
  Synopsis    [Read transition relation]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Trav_Xchg_t *
Trav_MgrXchg(
  Trav_Mgr_t * travMgr          /* traversal manager */
)
{
  return (&(travMgr->xchg));
}

/**Function********************************************************************
  Synopsis    [Read transition relation]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Tr_Tr_t *
Trav_MgrReadTr(
  Trav_Mgr_t * travMgr          /* traversal manager */
)
{
  return (travMgr->tr);
}

/**Function********************************************************************
  Synopsis    [Read ]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Trav_MgrSetTr(
  Trav_Mgr_t * travMgr /* Traversal manager */ ,
  Tr_Tr_t * tr                  /* transition relation */
)
{
  if (travMgr->tr != NULL) {
    Tr_TrFree(travMgr->tr);
  }
  travMgr->tr = Tr_TrDup(tr);

  return;
}

/**Function********************************************************************

  Synopsis    [Read Number of Level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadLevel(
  Trav_Mgr_t * travMgr          /* traversal manager */
)
{
  return (travMgr->level);
}

/**Function********************************************************************

  Synopsis    [Read Product Peak Value]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadProductPeak(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  return (travMgr->productPeak);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetProductPeak(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int productPeak               /* Product Peak  */
)
{
  travMgr->productPeak = productPeak;

  return;
}

/**Function********************************************************************

  Synopsis    [Read PI array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Ddi_Vararray_t *
Trav_MgrReadI(
  Trav_Mgr_t * travMgr          /* tr manager */
)
{
  return (travMgr->i);
}

/**Function********************************************************************

  Synopsis    [Set the PI array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetI(
  Trav_Mgr_t * travMgr /* Traversal manager */ ,
  Ddi_Vararray_t * i            /* Array of variables */
)
{
  travMgr->i = Ddi_VararrayCopy(travMgr->ddiMgrTr, i);
}

/**Function********************************************************************

  Synopsis    [Read PS array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Ddi_Vararray_t *
Trav_MgrReadPS(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->ps);
}

/**Function********************************************************************

  Synopsis    [Set the PS array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetPS(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Ddi_Vararray_t * ps           /* Array of Variables */
)
{
  travMgr->ps = Ddi_VararrayCopy(travMgr->ddiMgrTr, ps);
}

/**Function********************************************************************

  Synopsis    [Read AUX array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Ddi_Vararray_t *
Trav_MgrReadAux(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->aux);
}

/**Function********************************************************************

  Synopsis    [Set the aux array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetAux(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Ddi_Vararray_t * aux          /* Array of Variables */
)
{
  travMgr->aux = Ddi_VararrayCopy(travMgr->ddiMgrTr, aux);

  return;
}

/**Function********************************************************************

  Synopsis    [Set the flag to indicate that during traversal there is an
    additional auziliary manager]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetMgrAuxFlag(
  Trav_Mgr_t * travMgr /* traversal manager */ ,
  int flag
)
{
  travMgr->settings.mgrAuxFlag = flag;

  return;
}

/**Function********************************************************************

  Synopsis    [Read NS array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Ddi_Vararray_t *
Trav_MgrReadNS(
  Trav_Mgr_t * travMgr          /* tr manager */
)
{
  return (travMgr->ns);
}

/**Function********************************************************************

  Synopsis    [Set the NS array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetNS(
  Trav_Mgr_t * travMgr /* Traversal manager */ ,
  Ddi_Vararray_t * ns           /* Array of variables */
)
{
  travMgr->ns = Ddi_VararrayCopy(travMgr->ddiMgrTr, ns);
}

/**Function********************************************************************

  Synopsis    [Read reached]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Trav_MgrReadReached(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  return (travMgr->reached);
}

/**Function********************************************************************

  Synopsis    [Read care set]
  Description []
  SideEffects [none]
  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Trav_MgrReadCare(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  return (travMgr->care);
}

/**Function********************************************************************

  Synopsis    [Read assume]
  Description []
  SideEffects [none]
  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Trav_MgrReadAssume(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  return (travMgr->assume);
}

/**Function********************************************************************

  Synopsis    [Read assume]
  Description []
  SideEffects [none]
  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Trav_MgrReadProved(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  return (travMgr->proved);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetReached(
  Trav_Mgr_t * travMgr /* Traversal manager */ ,
  Ddi_Bdd_t * reached           /* Reached set */
)
{
  if (travMgr->reached != NULL)
    Ddi_Free(travMgr->reached);
  travMgr->reached = Ddi_BddCopy(travMgr->ddiMgrR, reached);

  return;
}

/**Function********************************************************************

  Synopsis    [Set care]
  Description []
  SideEffects [none]
  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetCare(
  Trav_Mgr_t * travMgr /* Traversal manager */ ,
  Ddi_Bdd_t * care              /* Care set */
)
{
  if (travMgr->care != NULL)
    Ddi_Free(travMgr->care);
  travMgr->care = Ddi_BddCopy(travMgr->ddiMgrR, care);

  return;
}

/**Function********************************************************************

  Synopsis    [Set care]
  Description []
  SideEffects [none]
  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetAssume(
  Trav_Mgr_t * travMgr /* Traversal manager */ ,
  Ddi_Bdd_t * assume            /* Assume set */
)
{
  if (travMgr->assume != NULL)
    Ddi_Free(travMgr->assume);
  travMgr->assume = Ddi_BddCopy(travMgr->ddiMgrR, assume);

  return;
}

/**Function********************************************************************

  Synopsis    [Set care]
  Description []
  SideEffects [none]
  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetProved(
  Trav_Mgr_t * travMgr /* Traversal manager */ ,
  Ddi_Bdd_t * proved            /* Proved set */
)
{
  if (travMgr->proved != NULL)
    Ddi_Free(travMgr->proved);
  travMgr->proved = Ddi_BddCopy(travMgr->ddiMgrR, proved);

  return;
}

/**Function********************************************************************

  Synopsis    [Read Assert.]
  Description []
  SideEffects [none]
  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Trav_MgrReadAssert(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  return (travMgr->assert);
}

/**Function********************************************************************

  Synopsis    [Read Assert.]
  Description []
  SideEffects [none]
  SeeAlso     []

******************************************************************************/

Ddi_Var_t *
Trav_MgrReadPdtSpecVar(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  return (travMgr->pdtSpecVar);
}

/**Function********************************************************************

  Synopsis    [Read Assert.]
  Description []
  SideEffects [none]
  SeeAlso     []

******************************************************************************/

Ddi_Var_t *
Trav_MgrReadPdtConstrVar(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  return (travMgr->pdtConstrVar);
}

/**Function********************************************************************

  Synopsis    [Set Assert.]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetAssert(
  Trav_Mgr_t * travMgr /* Traversal manager */ ,
  Ddi_Bdd_t * assert            /* Assertion */
)
{
  if (travMgr->assert != NULL)
    Ddi_Free(travMgr->assert);
  travMgr->assert = Ddi_BddCopy(travMgr->ddiMgrR, assert);

  return;
}

/**Function********************************************************************

  Synopsis    [Set Assert.]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetPdtSpecVar(
  Trav_Mgr_t * travMgr /* Traversal manager */ ,
  Ddi_Var_t * v                 /* spec fold var */
)
{
  travMgr->pdtSpecVar = Ddi_VarCopy(travMgr->ddiMgrR, v);

  return;
}

/**Function********************************************************************

  Synopsis    [Set Assert.]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetPdtConstrVar(
  Trav_Mgr_t * travMgr /* Traversal manager */ ,
  Ddi_Var_t * v                 /* spec fold var */
)
{
  travMgr->pdtConstrVar = Ddi_VarCopy(travMgr->ddiMgrR, v);

  return;
}

/**Function********************************************************************

  Synopsis    [Read Assert Flag.]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadAssertFlag(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  return (travMgr->assertFlag);
}

/**Function********************************************************************

  Synopsis    [Set Assert Flag.]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetAssertFlag(
  Trav_Mgr_t * travMgr /* Traversal manager */ ,
  int assertFlag                /* Assertion */
)
{
  travMgr->assertFlag = assertFlag;

  return;
}

/**Function********************************************************************

  Synopsis    [Read newi]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Ddi_Bddarray_t *
Trav_MgrReadNewi(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  return (travMgr->newi);
}

/**Function********************************************************************

  Synopsis    [Read cexes]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Ddi_Bddarray_t *
Trav_MgrReadCexes(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  return (travMgr->cexes);
}

/**Function********************************************************************

  Synopsis    [Set Newi]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetNewi(
  Trav_Mgr_t * travMgr /* Traversal manager */ ,
  Ddi_Bddarray_t * newi         /* Frontier sets */
)
{
  if (travMgr->newi != NULL)
    Ddi_Free(travMgr->newi);
  travMgr->newi = Ddi_BddarrayCopy(travMgr->ddiMgrR, newi);

  return;
}

/**Function********************************************************************

  Synopsis    [Set Cexes]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetCexes(
  Trav_Mgr_t * travMgr /* Traversal manager */ ,
  Ddi_Bddarray_t * cexes        /* Array of cexes */
)
{
  if (travMgr->cexes != NULL)
    Ddi_Free(travMgr->cexes);
  travMgr->cexes = Ddi_BddarrayCopy(travMgr->ddiMgrR, cexes);

  return;
}

/**Function********************************************************************

  Synopsis    [Read abstrrefrefinedvars]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Ddi_Varsetarray_t *
Trav_MgrReadAbstrRefRefinedVars(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  return (travMgr->settings.ints.abstrRefRefinedVars);
}

/**Function********************************************************************

  Synopsis    [Set abstrrefrefinedvars]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetAbstrRefRefinedVars(
  Trav_Mgr_t * travMgr /* Traversal manager */ ,
  Ddi_Varsetarray_t * abstrRefRefinedVars
)
{
  if (travMgr->settings.ints.abstrRefRefinedVars != NULL)
    Ddi_Free(travMgr->settings.ints.abstrRefRefinedVars);
  travMgr->settings.ints.abstrRefRefinedVars =
    Ddi_VarsetarrayCopy(travMgr->ddiMgrR, abstrRefRefinedVars);

  return;
}

/**Function********************************************************************

  Synopsis    [Read from]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Trav_MgrReadFrom(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  return (travMgr->from);
}

/**Function********************************************************************

  Synopsis    [Read]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetFrom(
  Trav_Mgr_t * travMgr /* Traversal manager */ ,
  Ddi_Bdd_t * from              /* From set */
)
{
  if (travMgr->from != NULL) {
    Ddi_Free(travMgr->from);
  }
  travMgr->from = Ddi_BddCopy(travMgr->ddiMgrR, from);

  return;
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

char *
Trav_MgrReadName(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  return (travMgr->travName);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetName(
  Trav_Mgr_t * travMgr /* Traversal manager */ ,
  char *travName
)
{
  travMgr->travName = Pdtutil_StrDup(travName);

  return;
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadSmoothVar(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  return (travMgr->settings.smoothVars);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

double
Trav_MgrReadW1(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.tr.w1);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

double
Trav_MgrReadW2(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.tr.w2);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

double
Trav_MgrReadW3(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.tr.w3);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

double
Trav_MgrReadW4(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.tr.w4);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadEnableDdR(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.enableDdR);
}


/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadTrProfileDynamicEnable(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.tr.trProfileDynamicEnable);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetTrProfileDynamicEnable(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int enable
)
{
  travMgr->settings.tr.trProfileDynamicEnable = enable;

  return;
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadTrProfileThreshold(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.tr.trProfileThreshold);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetTrProfileThreshold(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int threshold
)
{
  travMgr->settings.tr.trProfileThreshold = threshold;

  return;
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Cuplus_PruneHeuristic_e
Trav_MgrReadTrProfileMethod(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.tr.trProfileMethod);
}

/**Function********************************************************************

  Synopsis    [Set]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetTrProfileMethod(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Cuplus_PruneHeuristic_e method
)
{
  travMgr->settings.tr.trProfileMethod = method;

  return;
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadThreshold(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.tr.clustThreshold);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadDynAbstr(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.dynAbstr);
}


/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadDynAbstrInitIter(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.dynAbstrInitIter);
}


/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadImplAbstr(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.implAbstr);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadInputRegs(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.inputRegs);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpPart(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpPart);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpBdd(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpBdd);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpAppr(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpAppr);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpExact(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpExact);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpRefineCex(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpRefineCex);
}

/**Function********************************************************************

  Synopsis    [Read ]
  Description []
  SideEffects [none]
  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpBoundkOpt(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpBoundkOpt);
}

/**Function********************************************************************

  Synopsis    [Read ]
  Description []
  SideEffects [none]
  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpUseReached(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpUseReached);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpTuneForDepth(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpTuneForDepth);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpConeOpt(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpConeOpt);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpInnerCones(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpInnerCones);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpReuseRings(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpReuseRings);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpInductiveTo(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpInductiveTo);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpInitAbstr(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpInitAbstr);
}

int
Trav_MgrReadItpEndAbstr(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpEndAbstr);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpForceRun(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpForceRun);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpCheckCompleteness(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpCheckCompleteness);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpConstrLevel(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpConstrLevel);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpGenMaxIter(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpGenMaxIter);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadTernaryAbstr(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.ternaryAbstr);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadItpEnToPlusImg(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.itpEnToPlusImg);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadIgrMaxIter(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.igrMaxIter);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadIgrSide(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.igrSide);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadIgrGrowCone(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.igrGrowCone);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadIgrMaxExact(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.igrMaxExact);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadAbstrRef(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.abstrRef);
}

/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadAbstrRefGla(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.abstrRefGla);
}


/**Function********************************************************************

  Synopsis    [Read ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadSquaring(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.tr.squaring);
}


/**Function********************************************************************

  Synopsis    [Read verbosity]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Pdtutil_VerbLevel_e
Trav_MgrReadVerbosity(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.verbosity);
}

/**Function********************************************************************

  Synopsis    [Read verbosity]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

FILE *
Trav_MgrReadStdout(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.stdout);
}

/**Function********************************************************************

  Synopsis    [Read verbosity]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetSorting(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int sorting
)
{
  travMgr->settings.tr.trSorting = sorting;

  return;
}


/**Function********************************************************************

  Synopsis    [Read verbosity]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadSorting(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.tr.trSorting);
}

/**Function********************************************************************

  Synopsis    [Read the backward traversal flag]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadBackward(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.backward);
}

/**Function********************************************************************

  Synopsis    [Read the keepNew  flag]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadKeepNew(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.bdd.keepNew);
}



/**Function********************************************************************

  Synopsis    [Read the maximum number of traversal iterations]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadMaxIter(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.maxIter);
}

/**Function********************************************************************

  Synopsis    [Read mismatch optimization level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadMismatchOptLevel(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.bdd.mismatchOptLevel);
}


/**Function********************************************************************

  Synopsis    [Read lazy time limit]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadSelfTuningLevel(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.selfTuningLevel);
}


/**Function********************************************************************

  Synopsis    [Read lazy time limit]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadConstrLevel(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.constrLevel);
}


/**Function********************************************************************

  Synopsis    [Read target enlargment iterations]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

char *
Trav_MgrReadClk(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.clk);
}

/**Function********************************************************************

  Synopsis    [Read target enlargment iterations]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

char *
Trav_MgrReadSatSolver(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.aig.satSolver);
}


/**Function********************************************************************

  Synopsis    [Read underApprox max vars]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadUnderApproxMaxVars(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.bdd.underApproxMaxVars);
}


/**Function********************************************************************

  Synopsis    [Read underApprox max size]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadUnderApproxMaxSize(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.bdd.underApproxMaxSize);
}

/**Function********************************************************************

  Synopsis    [Read underApprox target reduction]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

float
Trav_MgrReadUnderApproxTargetReduction(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.bdd.underApproxTargetReduction);
}



/**Function********************************************************************

  Synopsis    [Read the period for verbosity enabling]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadLogPeriod(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.logPeriod);
}



/**Function********************************************************************

  Synopsis    [Read the period for save BDDs enabling]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadSavePeriod(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.savePeriod);
}



/**Function********************************************************************

  Synopsis    [Read the period for save BDDs enabling]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

char *
Trav_MgrReadSavePeriodName(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.savePeriodName);
}



/**Function********************************************************************

  Synopsis    [Read from selection]

  SideEffects [none]

  Description []

  SeeAlso     []

******************************************************************************/

Trav_FromSelect_e
Trav_MgrReadFromSelect(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.bdd.fromSelect);
}


/**Function********************************************************************

  Synopsis    [Set DDi Mgr TR]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetDdiMgrTr(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Ddi_Mgr_t * mgrTr             /* dd Manager */
)
{
  travMgr->ddiMgrTr = mgrTr;

  return;
}

/**Function********************************************************************

  Synopsis    [Set default DDi Mgr]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetDdiMgrDefault(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Ddi_Mgr_t * mgrTr             /* dd Manager */
)
{
  travMgr->ddiMgrTr = mgrTr;

  return;
}

/**Function********************************************************************

  Synopsis    [Set DDi Mgr R]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetDdiMgrR(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Ddi_Mgr_t * mgrR              /* Decision Diagram Manager */
)
{
  travMgr->ddiMgrR = mgrR;

  return;
}

/**Function********************************************************************

  Synopsis    [Read default DDi Mgr]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Ddi_Mgr_t *
Trav_MgrReadDdiMgrDefault(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->ddiMgrTr);
}


/**Function********************************************************************

  Synopsis    [Read DDi Mgr TR]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Ddi_Mgr_t *
Trav_MgrReadDdiMgrTr(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->ddiMgrTr);
}

/**Function********************************************************************

  Synopsis    [Read DDi Mgr R]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Ddi_Mgr_t *
Trav_MgrReadDdiMgrR(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->ddiMgrR);
}

/**Function********************************************************************

  Synopsis    [Set verbosity]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetStdout(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  FILE * stdout                 /* stdout */
)
{
  travMgr->settings.stdout = stdout;

  return;
}

/**Function********************************************************************

  Synopsis    [Set verbosity]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetVerbosity(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Pdtutil_VerbLevel_e verbosity /* Verbosity Level */
)
{
  travMgr->settings.verbosity = verbosity;

  return;
}

/**Function********************************************************************

  Synopsis    [Set the backward traversal flag]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetBackward(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int backward                  /* Max iterations */
)
{
  travMgr->settings.backward = backward;
}

/**Function********************************************************************

  Synopsis    [Set the keepNew flag]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetKeepNew(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int keepNew                   /* Max iterations */
)
{
  travMgr->settings.bdd.keepNew = keepNew;
}


/**Function********************************************************************

  Synopsis    [Set the maximum number of traversal iterations]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetMaxIter(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int maxIter                   /* Max iterations */
)
{
  travMgr->settings.maxIter = maxIter;
}

/**Function********************************************************************

  Synopsis    [Set mismatch opt level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetMismatchOptLevel(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int mismatchOptLevel          /* opt level */
)
{
  travMgr->settings.bdd.mismatchOptLevel = mismatchOptLevel;
}

/**Function********************************************************************

  Synopsis    [Set lazy time limit]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetSelfTuningLevel(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int l                         /* self tuning level */
)
{
  travMgr->settings.aig.selfTuningLevel = l;
}

/**Function********************************************************************

  Synopsis    [Set lazy time limit]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetConstrLevel(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int l                         /* constr level */
)
{
  travMgr->settings.constrLevel = l;
}

/**Function********************************************************************

  Synopsis    [Set mismatch opt level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetClk(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  char *clk                     /* clock name */
)
{
  Pdtutil_Free(travMgr->settings.clk);
  travMgr->settings.clk = Pdtutil_StrDup(clk);
}

/**Function********************************************************************

  Synopsis    [Set mismatch opt level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetSatSolver(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  char *satSolver               /* target enlargment iterations */
)
{
  travMgr->settings.aig.satSolver = Pdtutil_StrDup(satSolver);
}


/**Function********************************************************************

  Synopsis    [Set underApprox max vars]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetUnderApproxMaxVars(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int maxVars                   /* max allowed vars */
)
{
  travMgr->settings.bdd.underApproxMaxVars = maxVars;
}

/**Function********************************************************************

  Synopsis    [Set underApprox max size]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetUnderApproxMaxSize(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int maxSize                   /* max allowed vars */
)
{
  travMgr->settings.bdd.underApproxMaxSize = maxSize;
}

/**Function********************************************************************

  Synopsis    [Set underApprox target reduction]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetUnderApproxTargetReduction(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  float targetReduction         /* expected reduction factor (>= 1.0) */
)
{
  travMgr->settings.bdd.underApproxTargetReduction = targetReduction;
}


/**Function********************************************************************

  Synopsis    [Set the period for verbosity enabling]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetLogPeriod(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int logPeriod                 /* Period */
)
{
  travMgr->settings.logPeriod = logPeriod;
}


/**Function********************************************************************

  Synopsis    [Set the period for verbosity enabling]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetSavePeriod(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int savePeriod                /* Period */
)
{
  travMgr->settings.savePeriod = savePeriod;

  return;
}



/**Function********************************************************************

  Synopsis    [Set the period for verbosity enabling]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetSavePeriodName(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  char *savePeriodName          /* Period File Name */
)
{
  travMgr->settings.savePeriodName = Pdtutil_StrDup(savePeriodName);

  return;
}


/**Function********************************************************************

  Synopsis    [Set the from selection]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetFromSelect(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Trav_FromSelect_e fromSelect  /* Selection */
)
{
  if (fromSelect != Trav_FromSelectSame_c) {
    travMgr->settings.bdd.fromSelect = fromSelect;
  }

  return;
}

/**Function********************************************************************

  Synopsis    [Read the flag to indicate that during traversal there is an
    additional auziliary manager]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Trav_MgrReadMgrAuxFlag(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  return (travMgr->settings.mgrAuxFlag);
}


/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetDynAbstr(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int dynAbstr                  /* dynamic abstraction level */
)
{
  travMgr->settings.aig.dynAbstr = dynAbstr;
  return;
}


/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetDynAbstrInitIter(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int dynAbstrInitIter          /* dynamic abstraction level */
)
{
  travMgr->settings.aig.dynAbstrInitIter = dynAbstrInitIter;
  return;
}


/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetImplAbstr(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int implAbstr                 /* implication abstraction level */
)
{
  travMgr->settings.aig.implAbstr = implAbstr;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetInputRegs(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int inputRegs                 /* input register detection */
)
{
  travMgr->settings.aig.inputRegs = inputRegs;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpPart(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpPart                   /* interpolant partitioning */
)
{
  travMgr->settings.aig.itpPart = itpPart;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpBdd(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpBdd                    /* interpolant Bdd use */
)
{
  travMgr->settings.aig.itpBdd = itpBdd;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpAppr(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpAppr                   /* interpolant approx */
)
{
  travMgr->settings.aig.itpAppr = itpAppr;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpExact(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpExact                  /* interpolant exact fwd */
)
{
  travMgr->settings.aig.itpExact = itpExact;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpRefineCex(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpRefineCex              /* interpolant refinecex */
)
{
  travMgr->settings.aig.itpRefineCex = itpRefineCex;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]
  Description []
  SideEffects [none]
  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpBoundkOpt(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpBoundkOpt              /* interpolant boundk opt */
)
{
  travMgr->settings.aig.itpBoundkOpt = itpBoundkOpt;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]
  Description []
  SideEffects [none]
  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpUseReached(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpUseReached             /* interpolant use reached */
)
{
  travMgr->settings.aig.itpUseReached = itpUseReached;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpTuneForDepth(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpTuneForDepth           /* interpolant tune for depth */
)
{
  travMgr->settings.aig.itpTuneForDepth = itpTuneForDepth;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpConeOpt(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpConeOpt                /* interpolant cone opt */
)
{
  travMgr->settings.aig.itpConeOpt = itpConeOpt;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpInnerCones(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpInnerCones             /* interpolant boundk opt */
)
{
  travMgr->settings.aig.itpInnerCones = itpInnerCones;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpReuseRings(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpReuseRings             /* interpolant reuse rings opt */
)
{
  travMgr->settings.aig.itpReuseRings = itpReuseRings;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpInductiveTo(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpInductiveTo            /* interpolant inductive to */
)
{
  travMgr->settings.aig.itpInductiveTo = itpInductiveTo;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpInitAbstr(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpInitAbstr              /* interpolant inti abstr level */
)
{
  travMgr->settings.aig.itpInitAbstr = itpInitAbstr;
  return;
}

void
Trav_MgrSetItpEndAbstr(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpEndAbstr              /* interpolant inti abstr level */
)
{
  travMgr->settings.aig.itpEndAbstr = itpEndAbstr;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpForceRun(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpForceRun               /* interpolant force run */
)
{
  travMgr->settings.aig.itpForceRun = itpForceRun;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpCheckCompleteness(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpCheckCompleteness      /* interpolant force run */
)
{
  travMgr->settings.aig.itpCheckCompleteness = itpCheckCompleteness;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpUsePdrReached(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpUsePdrReached
)
{
  travMgr->settings.aig.itpUsePdrReached = itpUsePdrReached;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetPdrShareReached(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int pdrShareReached
)
{
  travMgr->settings.aig.pdrShareReached = pdrShareReached;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetPdrUsePgEncoding(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int pdrUsePgEncoding
)
{
  travMgr->settings.aig.pdrUsePgEncoding = pdrUsePgEncoding;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetPdrBumpActTopologically(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int pdrBumpActTopologically
)
{
  travMgr->settings.aig.pdrBumpActTopologically = pdrBumpActTopologically;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetPdrSpecializedCallsMask(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int pdrSpecializedCallsMask
)
{
  travMgr->settings.aig.pdrSpecializedCallsMask = pdrSpecializedCallsMask;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetPdrRestartStrategy(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int pdrRestartStrategy
)
{
  travMgr->settings.aig.pdrRestartStrategy = pdrRestartStrategy;
  return;
}


/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpShareReached(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpShareReached
)
{
  travMgr->settings.aig.itpShareReached = itpShareReached;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetPdrFwdEq(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int pdrFwdEq
)
{
  travMgr->settings.aig.pdrFwdEq = pdrFwdEq;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetPdrInf(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int pdrInf
)
{
  travMgr->settings.aig.pdrInf = pdrInf;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetPdrCexJustify(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int val
)
{
  travMgr->settings.aig.pdrCexJustify = val;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetPdrGenEffort(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int pdrGenEffort
)
{
  travMgr->settings.aig.pdrGenEffort = pdrGenEffort;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetPdrIncrementalTr(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int pdrIncrementalTr
)
{
  travMgr->settings.aig.pdrIncrementalTr = pdrIncrementalTr;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpConstrLevel(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpConstrLevel            /* interpolant constr level */
)
{
  travMgr->settings.aig.itpConstrLevel = itpConstrLevel;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpGenMaxIter(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int itpGenMaxIter             /* generalized interpolant max iter */
)
{
  travMgr->settings.aig.itpGenMaxIter = itpGenMaxIter;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetTernaryAbstr(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int ternaryAbstr              /* ternary abstraction level */
)
{
  travMgr->settings.aig.ternaryAbstr = ternaryAbstr;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetItpEnToPlusImg(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int val                       /* itp en toplus img */
)
{
  travMgr->settings.aig.itpEnToPlusImg = val;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetIgrMaxIter(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int val                       /* igr max iter */
)
{
  travMgr->settings.aig.igrMaxIter = val;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetIgrSide(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int val                       /* igr max iter */
)
{
  travMgr->settings.aig.igrSide = val;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetIgrGrowCone(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int val                       /* igr max iter */
)
{
  travMgr->settings.aig.igrGrowCone = val;
  return;
}

/**Function********************************************************************

  Synopsis    [Set dynamic abstraction level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetIgrMaxExact(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int val                       /* igr max exact */
)
{
  travMgr->settings.aig.igrMaxExact = val;
  return;
}

/**Function********************************************************************

  Synopsis    [Set abstraction refinement level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetAbstrRef(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int abstrRef                  /* abstraction refinement level */
)
{
  travMgr->settings.aig.abstrRef = abstrRef;
  return;
}

/**Function********************************************************************

  Synopsis    [Set abstraction refinement level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Trav_MgrSetAbstrRefGla(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  int abstrRefGla                  /* abstraction refinement level */
)
{
  travMgr->settings.aig.abstrRefGla = abstrRefGla;
  return;
}


/**Function********************************************************************
  Synopsis    [Transfer options from an option list to a Trav Manager]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Trav_MgrSetOptionList(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Pdtutil_OptList_t * pkgOpt    /* list of options */
)
{
  Pdtutil_OptItem_t optItem;

  while (!Pdtutil_OptListEmpty(pkgOpt)) {
    optItem = Pdtutil_OptListExtractHead(pkgOpt);
    Trav_MgrSetOptionItem(travMgr, optItem);
  }
}


/**Function********************************************************************
  Synopsis    [Transfer options from an option list to a Trav Manager]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Trav_MgrSetOptionItem(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Pdtutil_OptItem_t optItem     /* option to set */
)
{
  switch (optItem.optTag.eTravOpt) {
      /* general */
    case Pdt_TravVerbosity_c:
      travMgr->settings.verbosity = (Pdtutil_VerbLevel_e) optItem.optData.inum;
      break;
    case Pdt_TravStdout_c:
      travMgr->settings.stdout = (FILE *) optItem.optData.pvoid;
      break;
    case Pdt_TravLogPeriod_c:
      travMgr->settings.logPeriod = optItem.optData.inum;
      break;
    case Pdt_TravClk_c:
      Pdtutil_Free(travMgr->settings.clk);
      travMgr->settings.clk = Pdtutil_StrDup(optItem.optData.pchar);
      break;
    case Pdt_TravDoCex_c:
      travMgr->settings.doCex = optItem.optData.inum;
      break;

      /* unused */
    case Pdt_TravConstrLevel_c:
      travMgr->settings.constrLevel = optItem.optData.inum;
      break;
    case Pdt_TravIwls95Variant_c:
      travMgr->settings.iwls95Variant = optItem.optData.inum;
      break;

      /* tr */
    case Pdt_TravTrProfileMethod_c:
      travMgr->settings.tr.trProfileMethod =
        (Cuplus_PruneHeuristic_e) optItem.optData.inum;
      break;
    case Pdt_TravTrProfileDynamicEnable_c:
      travMgr->settings.tr.trProfileDynamicEnable = optItem.optData.inum;
      break;
    case Pdt_TravTrProfileThreshold_c:
      travMgr->settings.tr.trProfileThreshold = optItem.optData.inum;
      break;
    case Pdt_TravTrSorting_c:
      travMgr->settings.tr.trSorting = optItem.optData.inum;
      break;
    case Pdt_TravTrSortW1_c:
      travMgr->settings.tr.w1 = optItem.optData.fnum;
      break;
    case Pdt_TravTrSortW2_c:
      travMgr->settings.tr.w2 = optItem.optData.fnum;
      break;
    case Pdt_TravTrSortW3_c:
      travMgr->settings.tr.w3 = optItem.optData.fnum;
      break;
    case Pdt_TravTrSortW4_c:
      travMgr->settings.tr.w4 = optItem.optData.fnum;
      break;
    case Pdt_TravSortForBck_c:
      travMgr->settings.tr.sortForBck = optItem.optData.inum;
      break;
    case Pdt_TravTrPreimgSort_c:
      travMgr->settings.tr.trPreimgSort = optItem.optData.inum;
      break;
    case Pdt_TravPartDisjTrThresh_c:
      travMgr->settings.tr.partDisjTrThresh = optItem.optData.inum;
      break;
    case Pdt_TravClustThreshold_c:
      travMgr->settings.tr.clustThreshold = optItem.optData.inum;
      break;
    case Pdt_TravApprClustTh_c:
      travMgr->settings.tr.apprClustTh = optItem.optData.inum;
      break;
    case Pdt_TravImgClustTh_c:
      travMgr->settings.tr.imgClustTh = optItem.optData.inum;
      break;
    case Pdt_TravSquaring_c:
      travMgr->settings.tr.squaring = optItem.optData.inum;
      break;
    case Pdt_TravTrProj_c:
      travMgr->settings.tr.trProj = optItem.optData.inum;
      break;
    case Pdt_TravTrDpartTh_c:
      travMgr->settings.tr.trDpartTh = optItem.optData.inum;
      break;
    case Pdt_TravTrDpartVar_c:
      Pdtutil_Free(travMgr->settings.tr.trDpartVar);
      travMgr->settings.tr.trDpartVar = Pdtutil_StrDup(optItem.optData.pchar);
      break;
    case Pdt_TravTrAuxVarFile_c:
      Pdtutil_Free(travMgr->settings.tr.auxVarFile);
      travMgr->settings.tr.auxVarFile = Pdtutil_StrDup(optItem.optData.pchar);
      break;
    case Pdt_TravTrEnableVar_c:
      Pdtutil_Free(travMgr->settings.tr.enableVar);
      travMgr->settings.tr.enableVar = Pdtutil_StrDup(optItem.optData.pchar);
      break;
    case Pdt_TravApproxTrClustFile_c:
      Pdtutil_Free(travMgr->settings.tr.approxTrClustFile);
      travMgr->settings.tr.approxTrClustFile =
        Pdtutil_StrDup(optItem.optData.pchar);
      break;
    case Pdt_TravBwdTrClustFile_c:
      Pdtutil_Free(travMgr->settings.tr.bwdTrClustFile);
      travMgr->settings.tr.bwdTrClustFile =
        Pdtutil_StrDup(optItem.optData.pchar);
      break;

      /* aig */
    case Pdt_TravSatSolver_c:
      Pdtutil_Free(travMgr->settings.aig.satSolver);
      travMgr->settings.aig.satSolver = Pdtutil_StrDup(optItem.optData.pchar);
      break;
    case Pdt_TravAbcOptLevel_c:
      travMgr->settings.aig.abcOptLevel = optItem.optData.inum;
      break;
    case Pdt_TravTargetEn_c:
      travMgr->settings.aig.targetEn = optItem.optData.inum;
      break;
    case Pdt_TravDynAbstr_c:
      travMgr->settings.aig.dynAbstr = optItem.optData.inum;
      break;
    case Pdt_TravDynAbstrInitIter_c:
      travMgr->settings.aig.dynAbstrInitIter = optItem.optData.inum;
      break;
    case Pdt_TravImplAbstr_c:
      travMgr->settings.aig.implAbstr = optItem.optData.inum;
      break;
    case Pdt_TravTernaryAbstr_c:
      travMgr->settings.aig.ternaryAbstr = optItem.optData.inum;
      break;
    case Pdt_TravStoreAbstrRefRefinedVars_c:
      travMgr->settings.aig.storeAbstrRefRefinedVars = Pdtutil_StrDup(optItem.optData.pchar);
      break;
    case Pdt_TravAbstrRef_c:
      travMgr->settings.aig.abstrRef = optItem.optData.inum;
      break;
    case Pdt_TravAbstrRefGla_c:
      travMgr->settings.aig.abstrRefGla = optItem.optData.inum;
      break;
    case Pdt_TravInputRegs_c:
      travMgr->settings.aig.inputRegs = optItem.optData.inum;
      break;
    case Pdt_TravSelfTuning_c:
      travMgr->settings.aig.selfTuningLevel = optItem.optData.inum;
      break;
    case Pdt_TravLemmasTimeLimit_c:
      travMgr->settings.aig.lemmasTimeLimit = optItem.optData.fnum;
      break;
    case Pdt_TravLazyTimeLimit_c:
      travMgr->settings.aig.lazyTimeLimit = optItem.optData.fnum;
      break;

      /* itp */
    case Pdt_TravItpBdd_c:
      travMgr->settings.aig.itpBdd = optItem.optData.inum;
      break;
    case Pdt_TravItpPart_c:
      travMgr->settings.aig.itpPart = optItem.optData.inum;
      break;
    case Pdt_TravItpPartCone_c:
      travMgr->settings.aig.itpPartCone = optItem.optData.inum;
      break;
    case Pdt_TravItpAppr_c:
      travMgr->settings.aig.itpAppr = optItem.optData.inum;
      break;
    case Pdt_TravItpRefineCex_c:
      travMgr->settings.aig.itpRefineCex = optItem.optData.inum;
      break;
    case Pdt_TravItpStructAbstr_c:
      travMgr->settings.aig.itpStructAbstr = optItem.optData.inum;
      break;
    case Pdt_TravItpNew_c:
      travMgr->settings.aig.itpNew = optItem.optData.inum;
      break;
    case Pdt_TravItpExact_c:
      travMgr->settings.aig.itpExact = optItem.optData.inum;
      break;
    case Pdt_TravItpInductiveTo_c:
      travMgr->settings.aig.itpInductiveTo = optItem.optData.inum;
      break;
    case Pdt_TravItpInnerCones_c:
      travMgr->settings.aig.itpInnerCones = optItem.optData.inum;
      break;
    case Pdt_TravItpInitAbstr_c:
      travMgr->settings.aig.itpInitAbstr = optItem.optData.inum;
      break;
    case Pdt_TravItpEndAbstr_c:
      travMgr->settings.aig.itpEndAbstr = optItem.optData.inum;
      break;
    case Pdt_TravItpTrAbstr_c:
      travMgr->settings.aig.itpTrAbstr = optItem.optData.inum;
      break;
    case Pdt_TravItpReuseRings_c:
      travMgr->settings.aig.itpReuseRings = optItem.optData.inum;
      break;
    case Pdt_TravItpTuneForDepth_c:
      travMgr->settings.aig.itpTuneForDepth = optItem.optData.inum;
      break;
    case Pdt_TravItpBoundkOpt_c:
      travMgr->settings.aig.itpBoundkOpt = optItem.optData.inum;
      break;
    case Pdt_TravItpExactBoundDouble_c:
      travMgr->settings.aig.itpExactBoundDouble = optItem.optData.inum;
      break;
    case Pdt_TravItpConeOpt_c:
      travMgr->settings.aig.itpConeOpt = optItem.optData.inum;
      break;
    case Pdt_TravItpMaxStepK_c:
      travMgr->settings.aig.itpMaxStepK = optItem.optData.inum;
      break;
    case Pdt_TravItpForceRun_c:
      travMgr->settings.aig.itpForceRun = optItem.optData.inum;
      break;
    case Pdt_TravItpUseReached_c:
      travMgr->settings.aig.itpUseReached = optItem.optData.inum;
      break;
    case Pdt_TravItpCheckCompleteness_c:
      travMgr->settings.aig.itpCheckCompleteness = optItem.optData.inum;
      break;
    case Pdt_TravItpGfp_c:
      travMgr->settings.aig.itpGfp = optItem.optData.inum;
      break;
    case Pdt_TravItpConstrLevel_c:
      travMgr->settings.aig.itpConstrLevel = optItem.optData.inum;
      break;
    case Pdt_TravItpGenMaxIter_c:
      travMgr->settings.aig.itpGenMaxIter = optItem.optData.inum;
      break;
    case Pdt_TravItpEnToPlusImg_c:
      travMgr->settings.aig.itpEnToPlusImg = optItem.optData.inum;
      break;
    case Pdt_TravItpShareReached_c:
      travMgr->settings.aig.itpShareReached = optItem.optData.inum;
      break;
    case Pdt_TravItpUsePdrReached_c:
      travMgr->settings.aig.itpUsePdrReached = optItem.optData.inum;
      break;
    case Pdt_TravItpWeaken_c:
      travMgr->settings.aig.itpWeaken = optItem.optData.inum;
      break;
    case Pdt_TravItpStrengthen_c:
      travMgr->settings.aig.itpStrengthen = optItem.optData.inum;
      break;
    case Pdt_TravItpReImg_c:
      travMgr->settings.aig.itpReImg = optItem.optData.inum;
      break;
    case Pdt_TravItpFromNewLevel_c:
      travMgr->settings.aig.itpFromNewLevel = optItem.optData.inum;
      break;
    case Pdt_TravItpRpm_c:
      travMgr->settings.aig.itpRpm = optItem.optData.inum;
      break;
    case Pdt_TravItpTimeLimit_c:
      travMgr->settings.aig.itpTimeLimit = optItem.optData.inum;
      break;
    case Pdt_TravItpPeakAig_c:
      travMgr->settings.aig.itpPeakAig = optItem.optData.inum;
      break;

      /* igr */
    case Pdt_TravIgrSide_c:
      travMgr->settings.aig.igrSide = optItem.optData.inum;
      break;
    case Pdt_TravIgrItpSeq_c:
      travMgr->settings.aig.igrItpSeq = optItem.optData.inum;
      break;
    case Pdt_TravIgrRestart_c:
      travMgr->settings.aig.igrRestart = optItem.optData.inum;
      break;
    case Pdt_TravIgrRewind_c:
      travMgr->settings.aig.igrRewind = optItem.optData.inum;
      break;
    case Pdt_TravIgrBwdRefine_c:
      travMgr->settings.aig.igrBwdRefine = optItem.optData.inum;
      break;
    case Pdt_TravIgrGrowCone_c:
      travMgr->settings.aig.igrGrowCone = optItem.optData.inum;
      break;
    case Pdt_TravIgrRewindMinK_c:
      travMgr->settings.aig.igrRewindMinK = optItem.optData.inum;
      break;
    case Pdt_TravIgrGrowConeMaxK_c:
      travMgr->settings.aig.igrGrowConeMaxK = optItem.optData.inum;
      break;
    case Pdt_TravIgrGrowConeMax_c:
      travMgr->settings.aig.igrGrowConeMax = optItem.optData.fnum;
      break;
    case Pdt_TravIgrUseRings_c:
      travMgr->settings.aig.igrUseRings = optItem.optData.inum;
      break;
    case Pdt_TravIgrUseRingsStep_c:
      travMgr->settings.aig.igrUseRingsStep = optItem.optData.inum;
      break;
    case Pdt_TravIgrUseBwdRings_c:
      travMgr->settings.aig.igrUseBwdRings = optItem.optData.inum;
      break;
    case Pdt_TravIgrAssumeSafeBound_c:
      travMgr->settings.aig.igrAssumeSafeBound = optItem.optData.inum;
      break;
    case Pdt_TravIgrConeSubsetBound_c:
      travMgr->settings.aig.igrConeSubsetBound = optItem.optData.inum;
      break;
    case Pdt_TravIgrConeSubsetSizeTh_c:
      travMgr->settings.aig.igrConeSubsetSizeTh = optItem.optData.inum;
      break;
    case Pdt_TravIgrConeSubsetPiRatio_c:
      travMgr->settings.aig.igrConeSubsetPiRatio = optItem.optData.fnum;
      break;
    case Pdt_TravIgrConeSplitRatio_c:
      travMgr->settings.aig.igrConeSplitRatio = optItem.optData.fnum;
      break;
    case Pdt_TravIgrMaxIter_c:
      travMgr->settings.aig.igrMaxIter = optItem.optData.inum;
      break;
    case Pdt_TravIgrMaxExact_c:
      travMgr->settings.aig.igrMaxExact = optItem.optData.inum;
      break;
    case Pdt_TravIgrFwdBwd_c:
      travMgr->settings.aig.igrFwdBwd = optItem.optData.inum;
      break;

      /* pdr */
    case Pdt_TravPdrFwdEq_c:
      travMgr->settings.aig.pdrFwdEq = optItem.optData.inum;
      break;
    case Pdt_TravPdrInf_c:
      travMgr->settings.aig.pdrInf = optItem.optData.inum;
      break;
    case Pdt_TravPdrUnfold_c:
      travMgr->settings.aig.pdrUnfold = optItem.optData.inum;
      break;
    case Pdt_TravPdrIndAssumeLits_c:
      travMgr->settings.aig.pdrIndAssumeLits = optItem.optData.inum;
      break;
    case Pdt_TravPdrPostordCnf_c:
      travMgr->settings.aig.pdrPostordCnf = optItem.optData.inum;
      break;
    case Pdt_TravPdrCexJustify_c:
      travMgr->settings.aig.pdrCexJustify = optItem.optData.inum;
      break;
    case Pdt_TravPdrGenEffort_c:
      travMgr->settings.aig.pdrGenEffort = optItem.optData.inum;
      break;
    case Pdt_TravPdrIncrementalTr_c:
      travMgr->settings.aig.pdrIncrementalTr = optItem.optData.inum;
      break;
    case Pdt_TravPdrShareReached_c:
      travMgr->settings.aig.pdrShareReached = optItem.optData.inum;
      break;
    case Pdt_TravPdrReuseRings_c:
      travMgr->settings.aig.pdrReuseRings = optItem.optData.inum;
      break;
    case Pdt_TravPdrMaxBlock_c:
      travMgr->settings.aig.pdrMaxBlock = optItem.optData.inum;
      break;
    case Pdt_TravPdrUsePgEncoding_c:
      travMgr->settings.aig.pdrUsePgEncoding = optItem.optData.inum;
      break;
    case Pdt_TravPdrBumpActTopologically_c:
      travMgr->settings.aig.pdrBumpActTopologically = optItem.optData.inum;
      break;
    case Pdt_TravPdrSpecializedCallsMask_c:
      travMgr->settings.aig.pdrSpecializedCallsMask = optItem.optData.inum;
      break;
    case Pdt_TravPdrRestartStrategy_c:
      travMgr->settings.aig.pdrRestartStrategy = optItem.optData.inum;
      break;
    case Pdt_TravPdrTimeLimit_c:
      travMgr->settings.aig.pdrTimeLimit = optItem.optData.inum;
      break;

      /* bmc */
    case Pdt_TravBmcTimeLimit_c:
      travMgr->settings.aig.bmcTimeLimit = optItem.optData.inum;
      break;
    case Pdt_TravBmcMemLimit_c:
      travMgr->settings.aig.bmcMemLimit = optItem.optData.inum;
      break;

      /* bdd */
    case Pdt_TravFromSelect_c:
      travMgr->settings.bdd.fromSelect =
        (Trav_FromSelect_e) optItem.optData.inum;
      break;
    case Pdt_TravUnderApproxMaxVars_c:
      travMgr->settings.bdd.underApproxMaxVars = optItem.optData.inum;
      break;
    case Pdt_TravUnderApproxMaxSize_c:
      travMgr->settings.bdd.underApproxMaxSize = optItem.optData.inum;
      break;
    case Pdt_TravUnderApproxTargetRed_c:
      travMgr->settings.bdd.underApproxTargetReduction = optItem.optData.fnum;
      break;
    case Pdt_TravNoCheck_c:
      travMgr->settings.bdd.noCheck = optItem.optData.inum;
      break;
    case Pdt_TravNoMismatch_c:
      travMgr->settings.bdd.noMismatch = optItem.optData.inum;
      break;
    case Pdt_TravMismOptLevel_c:
      travMgr->settings.bdd.mismatchOptLevel = optItem.optData.inum;
      break;
    case Pdt_TravGroupBad_c:
      travMgr->settings.bdd.groupBad = optItem.optData.inum;
      break;
    case Pdt_TravKeepNew_c:
      travMgr->settings.bdd.keepNew = optItem.optData.inum;
      break;
    case Pdt_TravPrioritizedMc_c:
      travMgr->settings.bdd.prioritizedMc = optItem.optData.inum;
      break;
    case Pdt_TravPmcPrioThresh_c:
      travMgr->settings.bdd.pmcPrioThresh = optItem.optData.inum;
      break;
    case Pdt_TravPmcMaxNestedFull_c:
      travMgr->settings.bdd.pmcMaxNestedFull = optItem.optData.inum;
      break;
    case Pdt_TravPmcMaxNestedPart_c:
      travMgr->settings.bdd.pmcMaxNestedPart = optItem.optData.inum;
      break;
    case Pdt_TravPmcNoSame_c:
      travMgr->settings.bdd.pmcNoSame = optItem.optData.inum;
      break;
    case Pdt_TravCountReached_c:
      travMgr->settings.bdd.countReached = optItem.optData.inum;
      break;
    case Pdt_TravAutoHint_c:
      travMgr->settings.bdd.autoHint = optItem.optData.inum;
      break;
    case Pdt_TravHintsStrategy_c:
      travMgr->settings.bdd.hintsStrategy = optItem.optData.inum;
      break;
    case Pdt_TravHintsTh_c:
      travMgr->settings.bdd.hintsTh = optItem.optData.inum;
      break;
    case Pdt_TravHints_c:
      // Pdtutil_Assert(optItem.optData.pvoid == NULL, "Non null hints array");
      Ddi_Free(travMgr->settings.bdd.hints);
      travMgr->settings.bdd.hints =
        Ddi_BddarrayDup((Ddi_Bddarray_t *) optItem.optData.pvoid);
      break;
    case Pdt_TravAuxPsSet_c:
      Ddi_Free(travMgr->settings.bdd.auxPsSet);
      travMgr->settings.bdd.auxPsSet =
        Ddi_VarsetDup((Ddi_Varset_t *) optItem.optData.pvoid);
      break;
    case Pdt_TravGfpApproxRange_c:
      travMgr->settings.bdd.gfpApproxRange = optItem.optData.inum;
      break;
    case Pdt_TravUseMeta_c:
      travMgr->settings.bdd.useMeta = optItem.optData.inum;
      break;
    case Pdt_TravMetaMethod_c:
      travMgr->settings.bdd.metaMethod =
        (Ddi_Meta_Method_e) optItem.optData.inum;
      break;
    case Pdt_TravMetaMaxSmooth_c:
      travMgr->settings.bdd.metaMaxSmooth = optItem.optData.inum;
      break;
    case Pdt_TravConstrain_c:
      travMgr->settings.bdd.constrain = optItem.optData.inum;
      break;
    case Pdt_TravConstrainLevel_c:
      travMgr->settings.bdd.constrainLevel = optItem.optData.inum;
      break;
    case Pdt_TravCheckFrozen_c:
      travMgr->settings.bdd.checkFrozen = optItem.optData.inum;
      break;
    case Pdt_TravApprOverlap_c:
      travMgr->settings.bdd.apprOverlap = optItem.optData.inum;
      break;
    case Pdt_TravApprNP_c:
      travMgr->settings.bdd.apprNP = optItem.optData.inum;
      break;
    case Pdt_TravApprMBM_c:
      travMgr->settings.bdd.apprMBM = optItem.optData.inum;
      break;
    case Pdt_TravApprGrTh_c:
      travMgr->settings.bdd.apprGrTh = optItem.optData.inum;
      break;
    case Pdt_TravApprPreimgTh_c:
      travMgr->settings.bdd.apprPreimgTh = optItem.optData.inum;
      break;
    case Pdt_TravMaxFwdIter_c:
      travMgr->settings.bdd.maxFwdIter = optItem.optData.inum;
      break;
    case Pdt_TravNExactIter_c:
      travMgr->settings.bdd.nExactIter = optItem.optData.inum;
      break;
    case Pdt_TravSiftMaxTravIter_c:
      travMgr->settings.bdd.siftMaxTravIter = optItem.optData.inum;
      break;
    case Pdt_TravImplications_c:
      travMgr->settings.bdd.implications = optItem.optData.inum;
      break;
    case Pdt_TravImgDpartTh_c:
      travMgr->settings.bdd.imgDpartTh = optItem.optData.inum;
      break;
    case Pdt_TravImgDpartSat_c:
      travMgr->settings.bdd.imgDpartSat = optItem.optData.inum;
      break;
    case Pdt_TravBound_c:
      travMgr->settings.bdd.bound = optItem.optData.inum;
      break;
    case Pdt_TravStrictBound_c:
      travMgr->settings.bdd.strictBound = optItem.optData.inum;
      break;
    case Pdt_TravGuidedTrav_c:
      travMgr->settings.bdd.guidedTrav = optItem.optData.inum;
      break;
    case Pdt_TravUnivQuantifyTh_c:
      travMgr->settings.bdd.univQuantifyTh = optItem.optData.inum;
      break;
    case Pdt_TravOptImg_c:
      travMgr->settings.bdd.optImg = optItem.optData.inum;
      break;
    case Pdt_TravOptPreimg_c:
      travMgr->settings.bdd.optPreimg = optItem.optData.inum;
      break;
    case Pdt_TravBddTimeLimit_c:
      travMgr->settings.bdd.bddTimeLimit = optItem.optData.inum;
      break;
    case Pdt_TravBddNodeLimit_c:
      travMgr->settings.bdd.bddNodeLimit = optItem.optData.inum;
      break;
    case Pdt_TravWP_c:
      Pdtutil_Free(travMgr->settings.bdd.wP);
      travMgr->settings.bdd.wP = Pdtutil_StrDup(optItem.optData.pchar);
      break;
    case Pdt_TravWR_c:
      Pdtutil_Free(travMgr->settings.bdd.wR);
      travMgr->settings.bdd.wR = Pdtutil_StrDup(optItem.optData.pchar);
      break;
    case Pdt_TravWBR_c:
      Pdtutil_Free(travMgr->settings.bdd.wBR);
      travMgr->settings.bdd.wBR = Pdtutil_StrDup(optItem.optData.pchar);
      break;
    case Pdt_TravWU_c:
      Pdtutil_Free(travMgr->settings.bdd.wU);
      travMgr->settings.bdd.wU = Pdtutil_StrDup(optItem.optData.pchar);
      break;
    case Pdt_TravWOrd_c:
      Pdtutil_Free(travMgr->settings.bdd.wOrd);
      travMgr->settings.bdd.wOrd = Pdtutil_StrDup(optItem.optData.pchar);
      break;
    case Pdt_TravRPlus_c:
      Pdtutil_Free(travMgr->settings.bdd.rPlus);
      travMgr->settings.bdd.rPlus = Pdtutil_StrDup(optItem.optData.pchar);
      break;
    case Pdt_TravInvarFile_c:
      Pdtutil_Free(travMgr->settings.bdd.invarFile);
      travMgr->settings.bdd.invarFile = Pdtutil_StrDup(optItem.optData.pchar);
      break;
    case Pdt_TravStoreCnf_c:
      Pdtutil_Free(travMgr->settings.bdd.storeCNF);
      travMgr->settings.bdd.storeCNF = Pdtutil_StrDup(optItem.optData.pchar);
      break;
    case Pdt_TravStoreCnfTr_c:
      travMgr->settings.bdd.storeCNFTR = optItem.optData.inum;
      break;
    case Pdt_TravStoreCnfMode_c:
      travMgr->settings.bdd.storeCNFmode = (char)optItem.optData.inum;
      break;
    case Pdt_TravStoreCnfPhase_c:
      travMgr->settings.bdd.storeCNFphase = optItem.optData.inum;
      break;
    case Pdt_TravMaxCnfLength_c:
      travMgr->settings.bdd.maxCNFLength = optItem.optData.inum;
      break;

    default:
      Pdtutil_Assert(0, "wrong trav opt list item tag");
  }
}


/**Function********************************************************************
  Synopsis    [Return an option value from a Trav Manager ]
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Trav_MgrReadOption(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Pdt_OptTagTrav_e travOpt /* option code */ ,
  void *optRet                  /* returned option value */
)
{
  switch (travOpt) {
      /* general */
    case Pdt_TravVerbosity_c:
      *(int *)optRet = (int)travMgr->settings.verbosity;
      break;
    case Pdt_TravStdout_c:
      *(FILE **) optRet = travMgr->settings.stdout;
      break;
    case Pdt_TravLogPeriod_c:
      *(int *)optRet = travMgr->settings.logPeriod;
      break;
    case Pdt_TravClk_c:
      *(char **)optRet = travMgr->settings.clk;
      break;
    case Pdt_TravDoCex_c:
      *(int *)optRet = travMgr->settings.doCex;
      break;

      /* unused */
    case Pdt_TravConstrLevel_c:
      *(int *)optRet = travMgr->settings.constrLevel;
      break;
    case Pdt_TravIwls95Variant_c:
      *(int *)optRet = travMgr->settings.iwls95Variant;
      break;

      /* tr */
    case Pdt_TravTrProfileMethod_c:
      *(int *)optRet = (int)travMgr->settings.tr.trProfileMethod;
      break;
    case Pdt_TravTrProfileDynamicEnable_c:
      *(int *)optRet = travMgr->settings.tr.trProfileDynamicEnable;
      break;
    case Pdt_TravTrProfileThreshold_c:
      *(int *)optRet = travMgr->settings.tr.trProfileThreshold;
      break;
    case Pdt_TravTrSorting_c:
      *(int *)optRet = travMgr->settings.tr.trSorting;
      break;
    case Pdt_TravTrSortW1_c:
      *(float *)optRet = travMgr->settings.tr.w1;
      break;
    case Pdt_TravTrSortW2_c:
      *(float *)optRet = travMgr->settings.tr.w2;
      break;
    case Pdt_TravTrSortW3_c:
      *(float *)optRet = travMgr->settings.tr.w3;
      break;
    case Pdt_TravTrSortW4_c:
      *(float *)optRet = travMgr->settings.tr.w4;
      break;
    case Pdt_TravSortForBck_c:
      *(int *)optRet = travMgr->settings.tr.sortForBck;
      break;
    case Pdt_TravTrPreimgSort_c:
      *(int *)optRet = travMgr->settings.tr.trPreimgSort;
      break;
    case Pdt_TravPartDisjTrThresh_c:
      *(int *)optRet = travMgr->settings.tr.partDisjTrThresh;
      break;
    case Pdt_TravClustThreshold_c:
      *(int *)optRet = travMgr->settings.tr.clustThreshold;
      break;
    case Pdt_TravApprClustTh_c:
      *(int *)optRet = travMgr->settings.tr.apprClustTh;
      break;
    case Pdt_TravImgClustTh_c:
      *(int *)optRet = travMgr->settings.tr.imgClustTh;
      break;
    case Pdt_TravSquaring_c:
      *(int *)optRet = travMgr->settings.tr.squaring;
      break;
    case Pdt_TravTrProj_c:
      *(int *)optRet = travMgr->settings.tr.trProj;
      break;
    case Pdt_TravTrDpartTh_c:
      *(int *)optRet = travMgr->settings.tr.trDpartTh;
      break;
    case Pdt_TravTrDpartVar_c:
      *(char **)optRet = travMgr->settings.tr.trDpartVar;
      break;
    case Pdt_TravTrAuxVarFile_c:
      *(char **)optRet = travMgr->settings.tr.auxVarFile;
      break;
    case Pdt_TravTrEnableVar_c:
      *(char **)optRet = travMgr->settings.tr.enableVar;
      break;
    case Pdt_TravApproxTrClustFile_c:
      *(char **)optRet = travMgr->settings.tr.approxTrClustFile;
      break;
    case Pdt_TravBwdTrClustFile_c:
      *(char **)optRet = travMgr->settings.tr.bwdTrClustFile;
      break;

      /* aig */
    case Pdt_TravSatSolver_c:
      *(char **)optRet = travMgr->settings.aig.satSolver;
      break;
    case Pdt_TravAbcOptLevel_c:
      *(int *)optRet = travMgr->settings.aig.abcOptLevel;
      break;
    case Pdt_TravTargetEn_c:
      *(int *)optRet = travMgr->settings.aig.targetEn;
      break;
    case Pdt_TravDynAbstr_c:
      *(int *)optRet = travMgr->settings.aig.dynAbstr;
      break;
    case Pdt_TravDynAbstrInitIter_c:
      *(int *)optRet = travMgr->settings.aig.dynAbstrInitIter;
      break;
    case Pdt_TravImplAbstr_c:
      *(int *)optRet = travMgr->settings.aig.implAbstr;
      break;
    case Pdt_TravTernaryAbstr_c:
      *(int *)optRet = travMgr->settings.aig.ternaryAbstr;
      break;
    case Pdt_TravStoreAbstrRefRefinedVars_c:
      *(char **)optRet = travMgr->settings.aig.storeAbstrRefRefinedVars;
      break;
    case Pdt_TravAbstrRef_c:
      *(int *)optRet = travMgr->settings.aig.abstrRef;
      break;
    case Pdt_TravAbstrRefGla_c:
      *(int *)optRet = travMgr->settings.aig.abstrRefGla;
      break;
    case Pdt_TravInputRegs_c:
      *(int *)optRet = travMgr->settings.aig.inputRegs;
      break;
    case Pdt_TravSelfTuning_c:
      *(int *)optRet = travMgr->settings.aig.selfTuningLevel;
      break;
    case Pdt_TravLemmasTimeLimit_c:
      *(float *)optRet = travMgr->settings.aig.lemmasTimeLimit;
      break;
    case Pdt_TravLazyTimeLimit_c:
      *(float *)optRet = travMgr->settings.aig.lazyTimeLimit;
      break;

      /* itp */
    case Pdt_TravItpBdd_c:
      *(int *)optRet = travMgr->settings.aig.itpBdd;
      break;
    case Pdt_TravItpPart_c:
      *(int *)optRet = travMgr->settings.aig.itpPart;
      break;
    case Pdt_TravItpPartCone_c:
      *(int *)optRet = travMgr->settings.aig.itpPartCone;
      break;
    case Pdt_TravItpAppr_c:
      *(int *)optRet = travMgr->settings.aig.itpAppr;
      break;
    case Pdt_TravItpRefineCex_c:
      *(int *)optRet = travMgr->settings.aig.itpRefineCex;
      break;
    case Pdt_TravItpStructAbstr_c:
      *(int *)optRet = travMgr->settings.aig.itpStructAbstr;
      break;
    case Pdt_TravItpNew_c:
      *(int *)optRet = travMgr->settings.aig.itpNew;
      break;
    case Pdt_TravItpExact_c:
      *(int *)optRet = travMgr->settings.aig.itpExact;
      break;
    case Pdt_TravItpInductiveTo_c:
      *(int *)optRet = travMgr->settings.aig.itpInductiveTo;
      break;
    case Pdt_TravItpInnerCones_c:
      *(int *)optRet = travMgr->settings.aig.itpInnerCones;
      break;
    case Pdt_TravItpInitAbstr_c:
      *(int *)optRet = travMgr->settings.aig.itpInitAbstr;
      break;
    case Pdt_TravItpEndAbstr_c:
      *(int *)optRet = travMgr->settings.aig.itpEndAbstr;
      break;
    case Pdt_TravItpTrAbstr_c:
      *(int *)optRet = travMgr->settings.aig.itpTrAbstr;
      break;
    case Pdt_TravItpReuseRings_c:
      *(int *)optRet = travMgr->settings.aig.itpReuseRings;
      break;
    case Pdt_TravItpTuneForDepth_c:
      *(int *)optRet = travMgr->settings.aig.itpTuneForDepth;
      break;
    case Pdt_TravItpBoundkOpt_c:
      *(int *)optRet = travMgr->settings.aig.itpBoundkOpt;
      break;
    case Pdt_TravItpExactBoundDouble_c:
      *(int *)optRet = travMgr->settings.aig.itpExactBoundDouble;
      break;
    case Pdt_TravItpConeOpt_c:
      *(int *)optRet = travMgr->settings.aig.itpConeOpt;
      break;
    case Pdt_TravItpForceRun_c:
      *(int *)optRet = travMgr->settings.aig.itpForceRun;
      break;
    case Pdt_TravItpMaxStepK_c:
      *(int *)optRet = travMgr->settings.aig.itpMaxStepK;
      break;
    case Pdt_TravItpUseReached_c:
      *(int *)optRet = travMgr->settings.aig.itpUseReached;
      break;
    case Pdt_TravItpCheckCompleteness_c:
      *(int *)optRet = travMgr->settings.aig.itpCheckCompleteness;
      break;
    case Pdt_TravItpConstrLevel_c:
      *(int *)optRet = travMgr->settings.aig.itpConstrLevel;
      break;
    case Pdt_TravItpGenMaxIter_c:
      *(int *)optRet = travMgr->settings.aig.itpGenMaxIter;
      break;
    case Pdt_TravItpEnToPlusImg_c:
      *(int *)optRet = travMgr->settings.aig.itpEnToPlusImg;
      break;
    case Pdt_TravItpShareReached_c:
      *(int *)optRet = travMgr->settings.aig.itpShareReached;
      break;
    case Pdt_TravItpUsePdrReached_c:
      *(int *)optRet = travMgr->settings.aig.itpUsePdrReached;
      break;
    case Pdt_TravItpWeaken_c:
      *(int *)optRet = travMgr->settings.aig.itpWeaken;
      break;
    case Pdt_TravItpStrengthen_c:
      *(int *)optRet = travMgr->settings.aig.itpStrengthen;
      break;
    case Pdt_TravItpReImg_c:
      *(int *)optRet = travMgr->settings.aig.itpReImg;
      break;
    case Pdt_TravItpFromNewLevel_c:
      *(int *)optRet = travMgr->settings.aig.itpFromNewLevel;
      break;
    case Pdt_TravItpRpm_c:
      *(int *)optRet = travMgr->settings.aig.itpRpm;
      break;
    case Pdt_TravItpTimeLimit_c:
      *(int *)optRet = travMgr->settings.aig.itpTimeLimit;
      break;
    case Pdt_TravItpPeakAig_c:
      *(int *)optRet = travMgr->settings.aig.itpPeakAig;
      break;

      /* igr */
    case Pdt_TravIgrSide_c:
      *(int *)optRet = travMgr->settings.aig.igrSide;
      break;
    case Pdt_TravIgrItpSeq_c:
      *(int *)optRet = travMgr->settings.aig.igrItpSeq;
      break;
    case Pdt_TravIgrRestart_c:
      *(int *)optRet = travMgr->settings.aig.igrRestart;
      break;
    case Pdt_TravIgrRewind_c:
      *(int *)optRet = travMgr->settings.aig.igrRewind;
      break;
    case Pdt_TravIgrBwdRefine_c:
      *(int *)optRet = travMgr->settings.aig.igrBwdRefine;
      break;
    case Pdt_TravIgrGrowCone_c:
      *(int *)optRet = travMgr->settings.aig.igrGrowCone;
      break;
    case Pdt_TravIgrRewindMinK_c:
      *(int *)optRet = travMgr->settings.aig.igrRewindMinK;
      break;
    case Pdt_TravIgrGrowConeMaxK_c:
      *(int *)optRet = travMgr->settings.aig.igrGrowConeMaxK;
      break;
    case Pdt_TravIgrGrowConeMax_c:
      *(float *)optRet = travMgr->settings.aig.igrGrowConeMax;
      break;
    case Pdt_TravIgrUseRings_c:
      *(int *)optRet = travMgr->settings.aig.igrUseRings;
      break;
    case Pdt_TravIgrUseRingsStep_c:
      *(int *)optRet = travMgr->settings.aig.igrUseRingsStep;
      break;
    case Pdt_TravIgrUseBwdRings_c:
      *(int *)optRet = travMgr->settings.aig.igrUseBwdRings;
      break;
    case Pdt_TravIgrAssumeSafeBound_c:
      *(int *)optRet = travMgr->settings.aig.igrAssumeSafeBound;
      break;
    case Pdt_TravIgrConeSubsetBound_c:
      *(int *)optRet = travMgr->settings.aig.igrConeSubsetBound;
      break;
    case Pdt_TravIgrConeSubsetSizeTh_c:
      *(int *)optRet = travMgr->settings.aig.igrConeSubsetSizeTh;
      break;
    case Pdt_TravIgrConeSubsetPiRatio_c:
      *(float *)optRet = travMgr->settings.aig.igrConeSubsetPiRatio;
      break;
    case Pdt_TravIgrConeSplitRatio_c:
      *(float *)optRet = travMgr->settings.aig.igrConeSplitRatio;
      break;
    case Pdt_TravIgrMaxIter_c:
      *(int *)optRet = travMgr->settings.aig.igrMaxIter;
      break;
    case Pdt_TravIgrMaxExact_c:
      *(int *)optRet = travMgr->settings.aig.igrMaxExact;
      break;
    case Pdt_TravIgrFwdBwd_c:
      *(int *)optRet = travMgr->settings.aig.igrFwdBwd;
      break;

      /* pdr */
    case Pdt_TravPdrFwdEq_c:
      *(int *)optRet = travMgr->settings.aig.pdrFwdEq;
      break;
    case Pdt_TravPdrInf_c:
      *(int *)optRet = travMgr->settings.aig.pdrInf;
      break;
    case Pdt_TravPdrUnfold_c:
      *(int *)optRet = travMgr->settings.aig.pdrUnfold;
      break;
    case Pdt_TravPdrIndAssumeLits_c:
      *(int *)optRet = travMgr->settings.aig.pdrIndAssumeLits;
      break;
    case Pdt_TravPdrPostordCnf_c:
      *(int *)optRet = travMgr->settings.aig.pdrPostordCnf;
      break;
    case Pdt_TravPdrCexJustify_c:
      *(int *)optRet = travMgr->settings.aig.pdrCexJustify;
      break;
    case Pdt_TravPdrGenEffort_c:
      *(int *)optRet = travMgr->settings.aig.pdrGenEffort;
      break;
    case Pdt_TravPdrIncrementalTr_c:
      *(int *)optRet = travMgr->settings.aig.pdrIncrementalTr;
      break;
    case Pdt_TravPdrShareReached_c:
      *(int *)optRet = travMgr->settings.aig.pdrShareReached;
      break;
    case Pdt_TravPdrMaxBlock_c:
      *(int *)optRet = travMgr->settings.aig.pdrMaxBlock;
      break;
    case Pdt_TravPdrUsePgEncoding_c:
      *(int *)optRet = travMgr->settings.aig.pdrUsePgEncoding;
      break;
    case Pdt_TravPdrBumpActTopologically_c:
      *(int *)optRet = travMgr->settings.aig.pdrBumpActTopologically;
      break;
    case Pdt_TravPdrSpecializedCallsMask_c:
      *(int *)optRet = travMgr->settings.aig.pdrSpecializedCallsMask;
      break;
    case Pdt_TravPdrRestartStrategy_c:
      *(int *)optRet = travMgr->settings.aig.pdrRestartStrategy;
      break;
    case Pdt_TravPdrTimeLimit_c:
      *(int *)optRet = travMgr->settings.aig.pdrTimeLimit;
      break;

      /* bmc */
    case Pdt_TravBmcTimeLimit_c:
      *(int *)optRet = travMgr->settings.aig.bmcTimeLimit;
      break;
    case Pdt_TravBmcMemLimit_c:
      *(int *)optRet = travMgr->settings.aig.bmcMemLimit;
      break;

      /* bdd */
    case Pdt_TravFromSelect_c:
      *(int *)optRet = (int)travMgr->settings.bdd.fromSelect;
      break;
    case Pdt_TravUnderApproxMaxVars_c:
      *(int *)optRet = travMgr->settings.bdd.underApproxMaxVars;
      break;
    case Pdt_TravUnderApproxMaxSize_c:
      *(int *)optRet = travMgr->settings.bdd.underApproxMaxSize;
      break;
    case Pdt_TravUnderApproxTargetRed_c:
      *(float *)optRet = travMgr->settings.bdd.underApproxTargetReduction;
      break;
    case Pdt_TravNoCheck_c:
      *(int *)optRet = travMgr->settings.bdd.noCheck;
      break;
    case Pdt_TravNoMismatch_c:
      *(int *)optRet = travMgr->settings.bdd.noMismatch;
      break;
    case Pdt_TravMismOptLevel_c:
      *(int *)optRet = travMgr->settings.bdd.mismatchOptLevel;
      break;
    case Pdt_TravGroupBad_c:
      *(int *)optRet = travMgr->settings.bdd.groupBad;
      break;
    case Pdt_TravKeepNew_c:
      *(int *)optRet = travMgr->settings.bdd.keepNew;
      break;
    case Pdt_TravPrioritizedMc_c:
      *(int *)optRet = travMgr->settings.bdd.prioritizedMc;
      break;
    case Pdt_TravPmcPrioThresh_c:
      *(int *)optRet = travMgr->settings.bdd.pmcPrioThresh;
      break;
    case Pdt_TravPmcMaxNestedFull_c:
      *(int *)optRet = travMgr->settings.bdd.pmcMaxNestedFull;
      break;
    case Pdt_TravPmcMaxNestedPart_c:
      *(int *)optRet = travMgr->settings.bdd.pmcMaxNestedPart;
      break;
    case Pdt_TravPmcNoSame_c:
      *(int *)optRet = travMgr->settings.bdd.pmcNoSame;
      break;
    case Pdt_TravCountReached_c:
      *(int *)optRet = travMgr->settings.bdd.countReached;
      break;
    case Pdt_TravAutoHint_c:
      *(int *)optRet = travMgr->settings.bdd.autoHint;
      break;
    case Pdt_TravHintsStrategy_c:
      *(int *)optRet = travMgr->settings.bdd.hintsStrategy;
      break;
    case Pdt_TravHintsTh_c:
      *(int *)optRet = travMgr->settings.bdd.hintsTh;
      break;
    case Pdt_TravHints_c:
      *(Ddi_Bddarray_t **) optRet = travMgr->settings.bdd.hints;
      break;
    case Pdt_TravAuxPsSet_c:
      *(Ddi_Varset_t **) optRet = travMgr->settings.bdd.auxPsSet;
      break;
    case Pdt_TravGfpApproxRange_c:
      *(int *)optRet = travMgr->settings.bdd.gfpApproxRange;
      break;
    case Pdt_TravUseMeta_c:
      *(int *)optRet = travMgr->settings.bdd.useMeta;
      break;
    case Pdt_TravMetaMethod_c:
      *(int *)optRet = (int)travMgr->settings.bdd.metaMethod;
      break;
    case Pdt_TravMetaMaxSmooth_c:
      *(int *)optRet = travMgr->settings.bdd.metaMaxSmooth;
      break;
    case Pdt_TravConstrain_c:
      *(int *)optRet = travMgr->settings.bdd.constrain;
      break;
    case Pdt_TravConstrainLevel_c:
      *(int *)optRet = travMgr->settings.bdd.constrainLevel;
      break;
    case Pdt_TravCheckFrozen_c:
      *(int *)optRet = travMgr->settings.bdd.checkFrozen;
      break;
    case Pdt_TravApprOverlap_c:
      *(int *)optRet = travMgr->settings.bdd.apprOverlap;
      break;
    case Pdt_TravApprNP_c:
      *(int *)optRet = travMgr->settings.bdd.apprNP;
      break;
    case Pdt_TravApprMBM_c:
      *(int *)optRet = travMgr->settings.bdd.apprMBM;
      break;
    case Pdt_TravApprGrTh_c:
      *(int *)optRet = travMgr->settings.bdd.apprGrTh;
      break;
    case Pdt_TravApprPreimgTh_c:
      *(int *)optRet = travMgr->settings.bdd.apprPreimgTh;
      break;
    case Pdt_TravMaxFwdIter_c:
      *(int *)optRet = travMgr->settings.bdd.maxFwdIter;
      break;
    case Pdt_TravNExactIter_c:
      *(int *)optRet = travMgr->settings.bdd.nExactIter;
      break;
    case Pdt_TravSiftMaxTravIter_c:
      *(int *)optRet = travMgr->settings.bdd.siftMaxTravIter;
      break;
    case Pdt_TravImplications_c:
      *(int *)optRet = travMgr->settings.bdd.implications;
      break;
    case Pdt_TravImgDpartTh_c:
      *(int *)optRet = travMgr->settings.bdd.imgDpartTh;
      break;
    case Pdt_TravImgDpartSat_c:
      *(int *)optRet = travMgr->settings.bdd.imgDpartSat;
      break;
    case Pdt_TravBound_c:
      *(int *)optRet = travMgr->settings.bdd.bound;
      break;
    case Pdt_TravStrictBound_c:
      *(int *)optRet = travMgr->settings.bdd.strictBound;
      break;
    case Pdt_TravGuidedTrav_c:
      *(int *)optRet = travMgr->settings.bdd.guidedTrav;
      break;
    case Pdt_TravUnivQuantifyTh_c:
      *(int *)optRet = travMgr->settings.bdd.univQuantifyTh;
      break;
    case Pdt_TravOptImg_c:
      *(int *)optRet = travMgr->settings.bdd.optImg;
      break;
    case Pdt_TravOptPreimg_c:
      *(int *)optRet = travMgr->settings.bdd.optPreimg;
      break;
    case Pdt_TravBddTimeLimit_c:
      *(int *)optRet = travMgr->settings.bdd.bddTimeLimit;
      break;
    case Pdt_TravBddNodeLimit_c:
      *(int *)optRet = travMgr->settings.bdd.bddNodeLimit;
      break;
    case Pdt_TravWP_c:
      *(char **)optRet = travMgr->settings.bdd.wP;
      break;
    case Pdt_TravWR_c:
      *(char **)optRet = travMgr->settings.bdd.wR;
      break;
    case Pdt_TravWBR_c:
      *(char **)optRet = travMgr->settings.bdd.wBR;
      break;
    case Pdt_TravWU_c:
      *(char **)optRet = travMgr->settings.bdd.wU;
      break;
    case Pdt_TravWOrd_c:
      *(char **)optRet = travMgr->settings.bdd.wOrd;
      break;
    case Pdt_TravRPlus_c:
      *(char **)optRet = travMgr->settings.bdd.rPlus;
      break;
    case Pdt_TravInvarFile_c:
      *(char **)optRet = travMgr->settings.bdd.invarFile;
      break;
    case Pdt_TravStoreCnf_c:
      *(char **)optRet = travMgr->settings.bdd.storeCNF;
      break;
    case Pdt_TravStoreCnfTr_c:
      *(int *)optRet = travMgr->settings.bdd.storeCNFTR;
      break;
    case Pdt_TravStoreCnfMode_c:
      *(char *)optRet = travMgr->settings.bdd.storeCNFmode;
      break;
    case Pdt_TravStoreCnfPhase_c:
      *(int *)optRet = travMgr->settings.bdd.storeCNFphase;
      break;
    case Pdt_TravMaxCnfLength_c:
      *(int *)optRet = travMgr->settings.bdd.maxCNFLength;
      break;

    default:
      Pdtutil_Assert(0, "wrong trav opt item tag");
  }

}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
int
Trav_LogStats(
  Trav_Mgr_t * travMgr,
  char *tag,
  int val,
  int logCpu
)
{
  int i, ret = 0;
  Trav_Stats_t *travStats = &(travMgr->xchg.stats);

  if (!travMgr->xchg.enabled)
    return 0;
  if (travStats == NULL)
    return 0;

  int currTime = (util_cpu_time() - travStats->cpuTimeInit);

  pthread_mutex_lock(&travStats->mutex);
  for (i = 0; i < travStats->arrayStatsNum; i++) {
    if (tag != NULL &&
      strcmp(Pdtutil_ArrayReadName(travStats->arrayStats[i]), tag) == 0) {
      Pdtutil_IntegerArrayInsertLast(travStats->arrayStats[i], val);
      ret = 1;
    }
    if (logCpu && strcmp(Pdtutil_ArrayReadName(travStats->arrayStats[i]),
        "CPUTIME") == 0) {
      Pdtutil_IntegerArrayInsertLast(travStats->arrayStats[i], currTime);
      ret = 1;
    }
  }
  travStats->cpuTimeTot = currTime;
  pthread_mutex_unlock(&travStats->mutex);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
int
Trav_LogInit(
  Trav_Mgr_t * travMgr,
  int nArg,
  ...
)
{
  int i, ret = 0;
  Trav_Stats_t *travStats = &(travMgr->xchg.stats);
  va_list ap;

  Pdtutil_Assert(!travMgr->xchg.enabled, "trav xchg double enable");
  if (travStats == NULL)
    return 0;

  pthread_mutex_lock(&travStats->mutex);

  travMgr->xchg.enabled = 1;

  travStats->cpuTimeInit = util_cpu_time();


  travStats->arrayStats = Pdtutil_Alloc(Pdtutil_Array_t *, nArg + 1);
  va_start(ap, travMgr);

  for (i = 0; i < nArg; i++) {
    char *str = va_arg(ap, char *
    );

    Pdtutil_Assert(str != NULL, "missing str argument");
    travStats->arrayStats[i] = Pdtutil_IntegerArrayAlloc(20);
    Pdtutil_ArrayWriteName(travStats->arrayStats[i], str);
    travStats->arrayStatsNum++;
    ret++;
  }
  printf("STATS NUM: %d\n", travStats->arrayStatsNum);
  va_end(ap);

  pthread_mutex_unlock(&travStats->mutex);

  return ret;
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
int
Trav_LogReadStats(
  Trav_Stats_t * travStats,
  char *tag,
  int k
)
{
  int i, ret;

  if (travStats->arrayStats == NULL) {
    return -1;
  }

  pthread_mutex_lock(&travStats->mutex);
  /* rescan after locking, fdue to possible modifications */
  if (travStats->arrayStats == NULL) {
    pthread_mutex_unlock(&travStats->mutex);
    return -1;
  }

  ret = -1;
  for (i = 0; i < travStats->arrayStatsNum; i++) {
    if (tag != NULL &&
      strcmp(Pdtutil_ArrayReadName(travStats->arrayStats[i]), tag) == 0) {
      if (k < 0 || k >= Pdtutil_IntegerArrayNum(travStats->arrayStats[i])) {
        k = Pdtutil_IntegerArrayNum(travStats->arrayStats[i]) - 1;
      }
      if (k>=0) {
	ret = Pdtutil_IntegerArrayRead(travStats->arrayStats[i], k);
	if (k > travStats->lastObserved) {
	  travStats->lastObserved = k;
	}
      }
    }
  }

  pthread_mutex_unlock(&travStats->mutex);

  return ret;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
int
Trav_LogReadSize(
  Trav_Stats_t * travStats
)
{
  int i, ret = 0;

  if (travStats->arrayStats == NULL)
    return -1;

  pthread_mutex_lock(&travStats->mutex);
  /* rescan after locking, fdue to possible modifications */
  if (travStats->arrayStats == NULL) {
    pthread_mutex_unlock(&travStats->mutex);
    return -1;
  }

  for (i = 0; i < travStats->arrayStatsNum; i++) {
    int s = Pdtutil_IntegerArrayNum(travStats->arrayStats[i]);

    if (s > ret)
      ret = s;
  }

  pthread_mutex_unlock(&travStats->mutex);

  return ret;
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
int
Trav_LogReadTotCpu(
  Trav_Stats_t * travStats
)
{
  int j, ret;

  pthread_mutex_lock(&travStats->mutex);
  /* rescan after locking, fdue to possible modifications */
  ret = travStats->cpuTimeTot;
  pthread_mutex_unlock(&travStats->mutex);

  return ret;
}

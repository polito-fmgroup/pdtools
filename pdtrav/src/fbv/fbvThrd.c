/**CFile***********************************************************************

  FileName    [fbvThrd.c]

  PackageName [fbv]

  Synopsis    [Multi-threaded model check handling]

  Description []

  SeeAlso   []

  Author    [Gianpiero Cabodi and Sergio Nocco]

  Copyright [This file was created at the Politecnico di Torino,
    Torino, Italy.
    The  Politecnico di Torino makes no warranty about the suitability of
    this software for any purpose.
    It is presented on an AS IS basis.
  ]

  Revision  []

******************************************************************************/

#include "fbvInt.h"
#include "ddiInt.h"
#include <pthread.h>
#include "fsm.h"
#include "fsmInt.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <signal.h>
#include "pdtutil.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define MAX_ENGINE_LIST_NUM 20
#define MAX_THREADS 50

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

typedef struct {
  int tId;
  pthread_t thread;
  pid_t childpid;
  int result;
  int completed;
  int winTh;
  int call;
  int forkProcess;
  Fsm_Mgr_t *fsmMgr;
  Fsm_Mgr_t *fsmMgrRef;
  Ddi_Mgr_t *ddiMgr;
  Trav_Mgr_t *travMgr;
  Tr_Mgr_t *trMgr;
  Mc_TaskSelection_e task;
  char *aigName;
  char *taskName;
  Fbv_Globals_t *opt;
  Pdtutil_OptList_t *optList;
  int pflWorker;
  int delayedStart;
  int blockedStart;
  struct {
    int outputPipe[2];
    int enOutputPipe;
    FILE *fpi;
    FILE *fpo;
  } io;
  union {
    struct {
      int bmcFirst;
      int bmcStep;
      int bmcSteps;
      int bmcTimeLimit;
    } bmc;
    struct {
    } bdd;
    struct {
      int timeLimit;
      int peakAig;
    } itp;
    struct {
    } pdr;
    struct {
      int doUndc;
    } syn;
    struct {
      int startId;
      int nw;
      int blockedId;
    } pfl;
  } specific;
  Trav_Xchg_t *xchg;
  pthread_mutex_t thrdInfoMutex;
  pthread_mutex_t blockedStartMutex;
} Fbv_ThreadWorker_t;

typedef struct {
  Fbv_ThreadWorker_t *workers;
  int nThreads;
  int nActiveThreads;
  int heuristic;
  int enTune;
  struct {
    int ng, nl, ni;
  } stats;
  pthread_attr_t attr;
  pthread_mutex_t shareMutex;
} Mc_McGlobals_t;


typedef struct {
  char *thrdEngines[MAX_ENGINE_LIST_NUM];
  int thrdNum;
} engineList_t;

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

Mc_McGlobals_t mcGlobals = { NULL, 0 };
Fbv_ThreadWorker_t synWorker;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static void mcGlobalsInit(
  engineList_t * elp
);
static int mcGlobalsNewThreads(
  engineList_t * elp,
  int *nump
);
static void mcGlobalsQuit(
);

static int callWorker(
  Fbv_ThreadWorker_t * worker,
  Fsm_Mgr_t * fsmMgr,
  Fsm_Mgr_t * fsmMgrRef,
  Mc_TaskSelection_e task,
  Fbv_Globals_t * opt
);
static void *doMcWork(
  void *voidptr
);
static void *doMcWork1(
  void *voidptr
);
static void tuneWorker(
  Fbv_ThreadWorker_t * masterWorker,
  Fbv_ThreadWorker_t * worker
);
static void *threadOutputPipes(
  void *voidptr
);
static void Kill_ChildProc(
  int signum
);
static int optSelectEngines(
  Fbv_Globals_t * opt,
  engineList_t * elp,
  int heuristic
);
static int runSchedThreads(
  Fbv_ThreadWorker_t *masterWorker,
  Fsm_Mgr_t * fsmMgr,
  Fsm_Mgr_t * fsmMgrRef,
  Fbv_Globals_t * opt,
  int level,
  int start,
  int num
);
static int logThrdCex(
  Fsm_Mgr_t * fsmMgr,
  Fsm_Mgr_t * fsmMgrRef,
  int tId,
  char *fName
);
static int
setCexWritten(
  void
);
static int optSelectEngine(
  Fbv_Globals_t * opt,
  engineList_t * elp,
  const char *engName
);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
FbvThrdVerif(
  char *fsm,
  char *ord,
  Fbv_Globals_t * opt
)
{
  Ddi_Mgr_t *ddiMgr;
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Varset_t *psvars, *nsvars;
  Tr_Mgr_t *trMgr = NULL;
  Trav_Mgr_t *travMgrAig = NULL;
  Fsm_Mgr_t *fsmMgr = NULL, *fsmMgr2 = NULL, *fsmMgr0 = NULL, *fsmMgrRef =
    NULL;
  Tr_Tr_t *tr = NULL;
  Ddi_Bdd_t *init = NULL, *invarspec, *invar = NULL, *care = NULL;
  Ddi_Bdd_t *invarspecBdd, *invarspecItp;
  Ddi_Bddarray_t *delta, *lambda;
  long elapsedTime, startVerifTime;
  int num_ff, heuristic, verificationResult = -1, itpBoundLimit = -1;
  int lastDdiNode, bddNodeLimit = -1;
  int deltaSize, deltaNum = 0,
    bmcTimeLimit = -1, bddTimeLimit = -1, bddMemoryLimit = -1;
  int doRunBdd = 0, doRunBmc = 0, doRunInd = 0, doRunLms = 0, doRunItp =
    0, doRunRed = 0;
  int doRunLemmas = 0, doRUnSim = 0, bmcSteps, indSteps, lemmasSteps =
    2, indTimeLimit = -1, itpTimeLimit = -1;
  int currThrd = 0;
  int thrdNum = 0, maxThrdNum = 10;
  int enOutputPipes = 0;
  engineList_t el;
  int winnerTh;
  int enPfl = opt->expt.enPfl;
  Fsm_Fsm_t *fsmFsm = NULL;
  int forceDeltaConstraint = 0;
  int doEnableSyn = 1;
  int fsmHasConstraint = 0;

  /**********************************************************************/
  /*                        Create DDI manager                          */
  /**********************************************************************/

  opt->expt.expertLevel = 2;

  ddiMgr = Ddi_MgrInit("DDI_manager", NULL, 0,
    DDI_UNIQUE_SLOTS, DDI_CACHE_SLOTS * 10, 0, -1, -1);

  FbvSetDdiMgrOpt(ddiMgr, opt, 0);

  /**********************************************************************/
  /*                        Create FSM manager                          */
  /**********************************************************************/

  opt->stats.startTime = util_cpu_time();

  fsmMgr = Fsm_MgrInit("fsm", ddiMgr);  //NULL);
  //Fsm_MgrSetDdManager (fsmMgr, ddiMgr);
  FbvSetFsmMgrOpt(fsmMgr, opt, 0);

  /**********************************************************************/
  /*                     Read BLIF/AIGER/FSM file                       */
  /**********************************************************************/

  fsmMgr = FbvFsmMgrLoad(fsmMgr, fsm, opt);
  if (Fsm_MgrReadConstraintBDD(fsmMgr)!=NULL && Ddi_BddSize(Fsm_MgrReadConstraintBDD(fsmMgr))>0)
    fsmHasConstraint = 1;
  if (!opt->mc.combinationalProblem) {
    fsmFsm = Fsm_FsmMakeFromFsmMgr(fsmMgr);
    Fsm_FsmFoldProperty(fsmFsm, opt->mc.compl_invarspec,
      opt->trav.cntReachedOK, 1);
    Fsm_FsmFoldConstraint(fsmFsm, opt->mc.compl_invarspec);
    //    Fsm_FsmFoldInit(fsmFsm);
    Fsm_FsmWriteToFsmMgr(fsmMgr, fsmFsm);
    Fsm_FsmFree(fsmFsm);
  }

  doEnableSyn = optSelectEngine(opt, NULL, "SYN");
  
  if (opt->expt.deepBound) {
    doEnableSyn = 0;
  }

  if (!doEnableSyn) {
    synWorker.result = -1;
  }
  else {
    synWorker.aigName = fsm;
    synWorker.task = Mc_TaskSyn_c;
    synWorker.result = -1;
    synWorker.taskName = Pdtutil_StrDup("SYN");
    synWorker.delayedStart = 0;
    synWorker.blockedStart = 0;
    synWorker.specific.syn.doUndc = Fsm_MgrReadXInitLatches(fsmMgr)>0;
      
    if (!callWorker(&synWorker, NULL, NULL, Mc_TaskSyn_c, opt)) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "\nThread %s activated\n",
          synWorker.taskName));
    } else {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout,
          "\nError creating Thread %s\n",
          synWorker.taskName));
    }
  }

  /* COMMON AIG-BDD PART */

  startVerifTime = util_cpu_time();

  if (opt->expt.expertLevel > 0) {
    if (deltaNum < 5500) {
      opt->mc.tuneForEqCheck = 1;
      opt->pre.findClk = 1;
      opt->pre.terminalScc = 1;
      opt->pre.impliedConstr = 1;
      opt->pre.specConj = 1;
    }
    opt->pre.peripheralLatches = 1;
    if (opt->expt.expertArgv != NULL) {
      FbvParseArgs(opt->expt.expertArgc, opt->expt.expertArgv, opt);
    }
  }

  invarspec = Fsm_MgrReadInvarspecBDD(fsmMgr);

  if (opt->mc.combinationalProblem || Ddi_BddIsConstant(invarspec)) {
    Ddi_Bdd_t *cex = NULL;
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "HEURISTIC strategy selection: COMB\n"));
    Ddi_BddNotAcc(invarspec);
    cex = Ddi_AigSatWithCex(invarspec);
    verificationResult = cex==NULL ? 0 : 1;
    fprintf(stdout, "Verification %s\n",
      verificationResult ? "FAILED" : "PASSED");
    fprintf(stdout, "\nInvariant verification completed by thread: COMB\n");
    if (verificationResult) {
      Ddi_BddSetPartConj(cex);
      Fsm_MgrSetCexBDD(fsmMgr, cex);
      Ddi_Free(cex);
      if (!logThrdCex(fsmMgr, fsmMgr, 0, opt->expName)) {
        verificationResult = -2;
      } else {
        verificationResult = 2;
        winnerTh = 0;
      }
    }
    Ddi_BddNotAcc(invarspec);
    goto quitMem;
  }

  heuristic = FbvSelectHeuristic(fsmMgr, opt, 0);
  // opt->trav.itpConstrLevel = 1;
  fsmMgrRef = Fsm_MgrDup(fsmMgr);

  if (heuristic != Fbv_HeuristicNone_c) {

    FbvSetHeuristicOpt(fsmMgr, opt, heuristic);
    opt->expt.doRunSim = 1;

  } else {

    //  opt->trav.itpConstrLevel = 0;
    //  opt->pre.ternarySim = 0;
    //  opt->pre.terminalScc = 0;
    //  opt->pre.impliedConstr = 0;
    //  opt->pre.reduceCustom = 0;

#if 1
    //problem on oski5..
    Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
    Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
    //    opt->pre.terminalScc = 0;
    //    opt->pre.peripheralLatches = 0;

    if (Ddi_VararrayNum(ps)==3491 && Ddi_VararrayNum(pi)==3175) {
      opt->pre.terminalScc = 0;
      //opt->pre.peripheralLatches = 0;
    }
#endif

    if (synWorker.result == 0) {
      verificationResult = 0; goto quitMem;
    }
    FbvFsmReductions(fsmMgr, opt, 0);

    if (synWorker.result == 0) {
      verificationResult = 0; goto quitMem;
    }
    /* phase 1 can verify */
    FbvFsmReductions(fsmMgr, opt, 1);
    if (opt->stats.verificationOK == 1) {
      //    Fsm_MgrQuit(fsmMgr);
      verificationResult = !opt->stats.verificationOK;
      printf("\nInvariant verification completed by thread: RED\n");
      goto quitMem;
    }

    if (synWorker.result == 0) {
      verificationResult = 0; goto quitMem;
    }
    FbvTestSec(fsmMgr, opt);
    FbvTestRelational(fsmMgr, opt);
    heuristic = FbvSelectHeuristic(fsmMgr, opt, 1);

    FbvSetHeuristicOpt(fsmMgr, opt, heuristic);

    doRunRed = 1 || (opt->pre.retime > 0);

    if (opt->expt.deepBound) {
      opt->pre.twoPhaseRed = 0;
    }

    /* phase 2: custom + two phase reduction */
    FbvFsmReductions(fsmMgr, opt, 2);

    FbvFsmMgrLoadRplus(fsmMgr, opt);

    /* SEQUENTIAL VERIFICATION STARTS HERE... */

    /********************************************************
     * Verification phase 0:
     * Ciruit reductions (coi, ternary sim, etc.)
     ********************************************************/

    elapsedTime = util_cpu_time();

    if (opt->trav.lemmasCare && !opt->pre.retime) {
      care = Ddi_BddMakeConstAig(ddiMgr, 1);
    }

    fsmMgr0 = Fsm_MgrDup(fsmMgr);

    if (doRunRed) {
      /* phase 3 can verify */
      opt->pre.retime = 0;
      FbvFsmReductions(fsmMgr, opt, 3);
      if (opt->stats.verificationOK == 1) {
        //      Fsm_MgrQuit(fsmMgr);
        verificationResult = !opt->stats.verificationOK;
        printf("\nInvariant verification completed by thread: RED\n");
        goto quitMem;
      }
    }



    opt->expt.doRunItp = 1;

      //    opt->expt.doRunBdd = 1; // forced for bddhwmcc strategy

    opt->expt.doRunIgr = 2;
    opt->expt.doRunSim = 1;

  }


  if (1&&(heuristic == Fbv_HeuristicCpu_c || Fbv_HeuristicIbm_c || heuristic == Fbv_HeuristicBmcBig_c ||
    heuristic == Fbv_HeuristicPdtSw_c ||
	    heuristic == Fbv_HeuristicSec_c)) {
    int reduced = 0;
    Ddi_Bddarray_t *optDelta = Fsm_MgrReadDeltaBDD(fsmMgr);
    Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
    Ddi_Bdd_t *myInvarspec = Ddi_BddDup(invarspec);
    if (!fsmHasConstraint)
      reduced = FbvFsmReductions(fsmMgr, opt, 4);
    opt->mc.combinationalProblem = opt->mc.combinationalProblem
      || Ddi_BddarrayNum(optDelta) == 0;
    if (!opt->mc.combinationalProblem && Ddi_BddarrayNum(optDelta) == 2) {
      opt->mc.combinationalProblem = 1;
      Ddi_BddComposeAcc(myInvarspec, ps, optDelta);
    }
    if (opt->mc.combinationalProblem || Ddi_BddIsConstant(myInvarspec)) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout,
          "HEURISTIC strategy selection: COMB\n"));
      Ddi_BddNotAcc(myInvarspec);
      verificationResult = Ddi_AigSat(myInvarspec);
      if (verificationResult == 0) {
        fprintf(stdout, "Verification %s\n",
          verificationResult ? "FAILED" : "PASSED");
        fprintf(stdout,
          "\nInvariant verification completed by thread: COMB\n");
        Ddi_Free(myInvarspec);
        goto quitMem;
      }
      Ddi_Free(myInvarspec);
      opt->expt.doRunBdd = 0;
      opt->expt.doRunBmc = 1;
      opt->expt.doRunSim = 0;
      opt->expt.doRunSyn = 0;
      opt->expt.doRunItp = 0;
      opt->expt.doRunPdr = 0;
      opt->expt.doRunPdr = 0;
      opt->expt.doRunPdr = 0;

    } else {
      if (reduced) {
        heuristic = FbvSelectHeuristic(fsmMgr, opt, 1);
        FbvSetHeuristicOpt(fsmMgr, opt, heuristic);
      }
    }

  }


  elapsedTime = util_cpu_time() - elapsedTime;
  Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
    fprintf(stdout, "Total run time for reductions: %.1f\n\n",
      elapsedTime / 1000.0);
    fprintf(stdout,
      "Circuit AIG stats: %d inp / %d state vars / %d AIG gates\n",
      Ddi_VararrayNum(Fsm_MgrReadVarI(fsmMgr)),
      Ddi_VararrayNum(Fsm_MgrReadVarPS(fsmMgr)),
      Ddi_BddarraySize(Fsm_MgrReadDeltaBDD(fsmMgr)));
  }

  FbvSetDdiMgrOpt(ddiMgr, opt, 1);

  forceDeltaConstraint = ((heuristic == Fbv_HeuristicDme_c) || 
			  Ddi_VararrayNum(Fsm_MgrReadVarPS(fsmMgr))<100);

  if (1 && forceDeltaConstraint)
  {
    fsmFsm = Fsm_FsmMakeFromFsmMgr(fsmMgr);
    Fsm_FsmDeltaWithConstraint(fsmFsm);
    Fsm_FsmWriteToFsmMgr(fsmMgr, fsmFsm);
    Fsm_FsmFree(fsmFsm);
  }


  /***************************/

  if (opt->mc.wFsm != NULL) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Writing FSM to file %s.\n", opt->mc.wFsm));

    Fsm_Fsm_t *fsmFsm = Fsm_FsmMakeFromFsmMgr(fsmMgr);

    if (!opt->mc.fold) {
      Fsm_FsmUnfoldProperty(fsmFsm, 1);
      Fsm_FsmUnfoldConstraint(fsmFsm);
    }

    if (Fsm_MgrReadInitStubBDD(fsmMgr) != NULL) {
      Fsm_FsmFoldInit(fsmFsm);
    }

    Fsm_FsmMiniWriteAiger(fsmFsm, opt->mc.wFsm);
    Fsm_FsmFree(fsmFsm);    
    exit(1);
  }


  /***************************/


  travMgrAig = Trav_MgrInit(NULL, ddiMgr);
  FbvSetTravMgrOpt(travMgrAig, opt);

  opt->expt.doRunBmc = 1;
  opt->expt.doRunPdr = 2;       // PDR & PDR2

  if (opt->expt.enItpPlus) {
    opt->expt.doRunItp++;
    opt->expt.doRunIgr++;
  }
  //  opt->expt.doRunBdd=1;
  //  opt->expt.doRunBmc=0;
  //  opt->expt.doRunPdr=0;
  //  opt->expt.doRunItp=0;

  //  opt->expt.doRunBmc = 0;

  if (opt->expt.deepBound) {
    opt->expt.doRunBmcPfl = 1;
    opt->expt.doRunBmc = 0;
    opt->expt.doRunSim = 0;
  }
  else if (enPfl) {
    if (opt->expt.doRunBdd && opt->expt.doRunBdd2) {
      opt->expt.doRunBddPfl = 1;
      opt->expt.doRunBdd = 0;
      opt->expt.doRunBdd2 = 0;
    }
    if (opt->expt.doRunItp && opt->expt.doRunIgr) {
      opt->expt.doRunItpPfl = 1;
      opt->expt.doRunItp = 0;
      opt->expt.doRunIgr = 0;
    }
    if (opt->expt.doRunBmc && opt->expt.doRunSim) {
      opt->expt.doRunBmcPfl = 1;
      opt->expt.doRunBmc = 0;
      opt->expt.doRunSim = 0;
    }
    if (opt->expt.doRunPdr) {
      opt->expt.doRunPdrPfl = 1;
      opt->expt.doRunPdr = 0;
    }
  }

  optSelectEngines(opt, &el, heuristic);

  /********************************************************
   * Verification phase 1:
   * Detect specific verification problems (currently only SEC)
   ********************************************************/

  //  lastDdiNode = Ddi_MgrReadCurrNodeId(ddiMgr);

  /********************************************************
   * Verification phase 2:
   * Try basic methods
   ********************************************************/


  mcGlobalsInit(&el);
  mcGlobals.heuristic = heuristic;
  mcGlobals.stats.ng = Ddi_BddarraySize(Fsm_MgrReadDeltaBDD(fsmMgr));
  mcGlobals.stats.nl = Ddi_VararrayNum(Fsm_MgrReadVarPS(fsmMgr));
  mcGlobals.stats.ni = Ddi_VararrayNum(Fsm_MgrReadVarI(fsmMgr));

  if (enPfl)
    mcGlobals.enTune = 1;

  winnerTh = runSchedThreads(NULL,fsmMgr, fsmMgrRef, opt, 0, -1, -1);


  if (winnerTh >= 0) {
    if (winnerTh >= mcGlobals.nThreads) {
      verificationResult = synWorker.result;
    }
    else {
      verificationResult = mcGlobals.workers[winnerTh].result;
    }
  }

quitMem:

  //  Kill_ChildProc();
  for (currThrd = 0; currThrd < mcGlobals.nThreads; currThrd++) {
    pid_t childpid = mcGlobals.workers[currThrd].childpid;

    if (childpid > 0) {
      // kill child processe
      kill(childpid, SIGKILL);
    }
  }

  if (opt->mc.wRes != NULL && verificationResult >= 0) {
    fprintf(stdout, "Verification result (%s) written to file %s\n",
      verificationResult ? "FAIL" : "PASS", opt->mc.wRes);

    if (verificationResult > 1) {
      // fail and cex
      char cmd[1000];

      Pdtutil_Assert(winnerTh >= 0, "winner thread required");

      if (pdtResOut != NULL) {
	sprintf(cmd, "cat /tmp/%s.%d.cex",
		Pdtutil_WresFileName(), winnerTh, Pdtutil_WresFileName());
      }
      else {
	sprintf(cmd, "cat /tmp/%s.%d.cex >> %s",
		Pdtutil_WresFileName(), winnerTh, Pdtutil_WresFileName());
      }
      fprintf(stdout, "CMD: %s\n", cmd);
      system(cmd);
      sprintf(cmd, "rm /tmp/%s.%d.cex", Pdtutil_WresFileName(), winnerTh);
      system(cmd);
      // fail - write cex
    } else {
      Fsm_MgrLogVerificationResult(fsmMgr, opt->mc.wRes,
        verificationResult ? 1 : 0);
    }
  }
  //  mcGlobalsQuit();

  Fsm_MgrQuit(fsmMgr);

  if (fsmMgr0 != NULL) {
    Fsm_MgrQuit(fsmMgr0);
  }

  if (fsmMgrRef != NULL) {
    Fsm_MgrQuit(fsmMgrRef);
  }

  if (travMgrAig != NULL) {
    Trav_MgrQuit(travMgrAig);
  }

  /* write result on file */

  if (0 && (Ddi_MgrCheckExtRef(ddiMgr, 0) == 0)) {
    Ddi_MgrPrintExtRef(ddiMgr, 0);
  }

  if (opt->mc.method != Mc_VerifPrintNtkStats_c) {
    printf("Total used memory: %.2f MBytes\n",
      ((float)Ddi_MgrReadMemoryInUse(ddiMgr)) / (2 << 20));

    printf("Total Verification Time: %.1f\n",
      (util_cpu_time() - startVerifTime) / 1000.0);
  }

  /**********************************************************************/
  /*                       Close DDI manager                            */
  /**********************************************************************/

  /*Ddi_MgrPrintStats(ddiMgr); */

  //   Ddi_MgrQuit(ddiMgr);

  /**********************************************************************/
  /*                             End test                               */
  /**********************************************************************/

  return verificationResult;
}

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
mcGlobalsInit(
  engineList_t * elp
)
{
  int i, nT = elp->thrdNum;
  size_t stacksize;
  int maxThrd = 8;
  int userlim=0;

  if (userlim) {
    struct rlimit rlim;
    getrlimit(RLIMIT_AS,&rlim);
    printf("A cur: %d - max:%d\n",rlim.rlim_cur, rlim.rlim_max);
    getrlimit(RLIMIT_DATA,&rlim);
    printf("D cur: %d - max:%d\n",rlim.rlim_cur, rlim.rlim_max);
  }


  mcGlobals.nThreads = 0;
  mcGlobals.nActiveThreads = 0;
  mcGlobals.enTune = 0;

  pthread_attr_init(&(mcGlobals.attr));
  pthread_attr_getstacksize(&(mcGlobals.attr), &stacksize);
  stacksize *= (maxThrd * 1.0);
  pthread_attr_setstacksize(&(mcGlobals.attr), stacksize);
  pthread_mutex_init(&(mcGlobals.shareMutex), NULL);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "INITIALIZING THREADED ENGINES (stacksize = %li)\n",
      stacksize));

  mcGlobals.workers = Pdtutil_Alloc(Fbv_ThreadWorker_t, MAX_THREADS);

  if (nT > 0) {
    mcGlobalsNewThreads(elp, NULL);
  }

}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
mcGlobalsNewThreads(
  engineList_t * elp,
  int *nump
)
{
  int i, j, nT0, nT = elp->thrdNum;
  int ret = nT;


  if (nT == 0)
    return -1;

  pthread_mutex_lock(&(mcGlobals.shareMutex));

  nT0 = mcGlobals.nThreads;
  if (nump != NULL)
    *nump = nT0;

  mcGlobals.nThreads += nT;

  Pdtutil_Assert(mcGlobals.nThreads <= MAX_THREADS, "max thrd num exceeded");

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "INITIALIZING %d ENGINES:", nT));

  for (i = 0, j = nT0; i < nT; i++, j++) {
    Pdtutil_Assert(elp->thrdEngines[i] != NULL, "thrd engine is null");
    mcGlobals.workers[j].childpid = -1;
    mcGlobals.workers[j].call = 0;
    mcGlobals.workers[j].tId = -1;
    mcGlobals.workers[j].result = -1;
    mcGlobals.workers[j].completed = 0;
    mcGlobals.workers[j].aigName = NULL;
    mcGlobals.workers[j].taskName = Pdtutil_StrDup(elp->thrdEngines[i]);
    mcGlobals.workers[j].fsmMgr = NULL;
    mcGlobals.workers[j].ddiMgr = NULL;
    mcGlobals.workers[j].travMgr = NULL;
    mcGlobals.workers[j].trMgr = NULL;
    mcGlobals.workers[j].pflWorker = 0;
    mcGlobals.workers[j].delayedStart = 0;
    mcGlobals.workers[j].blockedStart = 0;
    mcGlobals.workers[j].specific.pfl.startId = -1;
    mcGlobals.workers[j].specific.pfl.blockedId = -1;

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, " %s", elp->thrdEngines[i]));
    if (strcmp(elp->thrdEngines[i], "BMC") == 0) {
      mcGlobals.workers[j].task = Mc_TaskBmc_c;
    } else if (strcmp(elp->thrdEngines[i], "BMC2") == 0) {
      mcGlobals.workers[j].task = Mc_TaskBmc2_c;
    } else if (strcmp(elp->thrdEngines[i], "BMCB") == 0) {
      mcGlobals.workers[j].task = Mc_TaskBmcb_c;
    } else if (strcmp(elp->thrdEngines[i], "PDR") == 0) {
      mcGlobals.workers[j].task = Mc_TaskPdr_c;
    } else if (strcmp(elp->thrdEngines[i], "PDR2") == 0) {
      mcGlobals.workers[j].task = Mc_TaskPdr2_c;
    } else if (strcmp(elp->thrdEngines[i], "PDRS") == 0) {
      mcGlobals.workers[j].task = Mc_TaskPdrs_c;
    } else if (strcmp(elp->thrdEngines[i], "ITP") == 0) {
      mcGlobals.workers[j].task = Mc_TaskItp_c;
    } else if (strcmp(elp->thrdEngines[i], "ITP2") == 0) {
      mcGlobals.workers[j].task = Mc_TaskItp2_c;
    } else if (strcmp(elp->thrdEngines[i], "ITPL") == 0) {
      mcGlobals.workers[j].task = Mc_TaskItpl_c;
    } else if (strcmp(elp->thrdEngines[i], "BDD") == 0) {
      mcGlobals.workers[j].task = Mc_TaskBdd_c;
    } else if (strcmp(elp->thrdEngines[i], "BDD2") == 0) {
      mcGlobals.workers[j].task = Mc_TaskBdd2_c;
    } else if (strcmp(elp->thrdEngines[i], "IGR") == 0) {
      mcGlobals.workers[j].task = Mc_TaskIgr_c;
    } else if (strcmp(elp->thrdEngines[i], "IGR2") == 0) {
      mcGlobals.workers[j].task = Mc_TaskIgr2_c;
    } else if (strcmp(elp->thrdEngines[i], "IGRPDR") == 0) {
      mcGlobals.workers[j].task = Mc_TaskIgrPdr_c;
    } else if (strcmp(elp->thrdEngines[i], "LMS") == 0) {
      mcGlobals.workers[j].task = Mc_TaskLms_c;
    } else if (strcmp(elp->thrdEngines[i], "SIM") == 0) {
      mcGlobals.workers[j].task = Mc_TaskSim_c;
    } else if (strcmp(elp->thrdEngines[i], "SYN") == 0) {
      mcGlobals.workers[j].task = Mc_TaskSyn_c;
    } else if (strcmp(elp->thrdEngines[i], "BDDPFL") == 0) {
      mcGlobals.workers[j].task = Mc_TaskBddPfl_c;
      mcGlobals.workers[j].pflWorker = 1;
    } else if (strcmp(elp->thrdEngines[i], "BMCPFL") == 0) {
      mcGlobals.workers[j].task = Mc_TaskBmcPfl_c;
      mcGlobals.workers[j].pflWorker = 1;
    } else if (strcmp(elp->thrdEngines[i], "ITPPFL") == 0) {
      mcGlobals.workers[j].task = Mc_TaskItpPfl_c;
      mcGlobals.workers[j].pflWorker = 1;
    } else if (strcmp(elp->thrdEngines[i], "PDRPFL") == 0) {
      mcGlobals.workers[j].task = Mc_TaskPdrPfl_c;
      mcGlobals.workers[j].pflWorker = 1;
    }
    mcGlobals.workers[j].xchg = NULL;
    pthread_mutex_init(&(mcGlobals.workers[j].thrdInfoMutex), NULL);
  }

  pthread_mutex_unlock(&(mcGlobals.shareMutex));

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "\n"));

  return ret;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
mcGlobalsQuit(
)
{
  if (mcGlobals.workers != NULL) {
    int i;

    for (i = 0; i < mcGlobals.nThreads; i++) {
      Pdtutil_Free(mcGlobals.workers[i].taskName);
      if (mcGlobals.workers[i].io.enOutputPipe) {
        fclose(mcGlobals.workers[i].io.fpi);
        fclose(mcGlobals.workers[i].io.fpo);
        close(mcGlobals.workers[i].io.outputPipe[0]);
        close(mcGlobals.workers[i].io.outputPipe[1]);
      }
      pthread_mutex_destroy(&(mcGlobals.workers[i].thrdInfoMutex));
    }
    Pdtutil_Free(mcGlobals.workers);
  }
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
callWorker(
  Fbv_ThreadWorker_t * worker,
  Fsm_Mgr_t * fsmMgr,
  Fsm_Mgr_t * fsmMgrRef,
  Mc_TaskSelection_e task,
  Fbv_Globals_t * opt
)
{
  worker->opt = opt;
  worker->task = task;
  worker->fsmMgr = fsmMgr;
  worker->fsmMgrRef = fsmMgrRef;
  worker->result = -1;

  if (worker->call > 0) {
    // Ddi_Mgr_t *ddiMgr = Fsm_MgrReadDdManager (worker->fsmMgr);
    // worker->travMgr = Trav_MgrInit (NULL, ddiMgr);
    doMcWork1((void *)worker);
  }
  else if (worker->task == Mc_TaskSyn_c) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "CREATING SYN Thread (father %d - %x)\n",
        getpid(), pthread_self()));
    if (pthread_create(&(worker->thread),
        &(mcGlobals.attr), doMcWork1, (void *)worker)) {
      return 1;
    }
  } else {
    //    Ddi_Mgr_t *ddiMgr;
    if (opt->expt.ddiMgrAlign) {
      worker->fsmMgr = Fsm_MgrDupWithNewDdiMgrAligned(fsmMgr);
    } else {
      worker->fsmMgr = Fsm_MgrDupWithNewDdiMgr(fsmMgr);
    }
    worker->fsmMgrRef = fsmMgrRef;

    // ddiMgr = Fsm_MgrReadDdManager (worker->fsmMgr);
    // worker->travMgr = Trav_MgrInit (NULL, ddiMgr);
    // worker->xchg = Trav_MgrXchg(worker->travMgr);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "CREATING Thread (father %d - %x)\n",
        getpid(), pthread_self()));

    if (pthread_create(&(worker->thread),
        &(mcGlobals.attr), doMcWork1, (void *)worker)) {
      return 1;
    }
  }
  return 0;
}



/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void *
doMcWork(
  void *voidptr
)
{
  Fbv_ThreadWorker_t *worker = (Fbv_ThreadWorker_t *) voidptr;

  Ddi_Bdd_t *init = NULL, *invarspec, *invar = NULL, *care = NULL;
  Ddi_Bddarray_t *delta = NULL;
  Tr_Tr_t *tr = NULL;

  Ddi_Mgr_t *ddiMgr = Fsm_MgrReadDdManager(worker->fsmMgr);
  Trav_Stats_t *travStats;
  int runLemmas = worker->opt->mc.lemmas;

  pthread_mutex_lock(&(worker->thrdInfoMutex));

  worker->travMgr = Trav_MgrInit(NULL, ddiMgr);
  worker->xchg = Trav_MgrXchg(worker->travMgr);
  travStats = &(worker->xchg->stats);

  if (worker->io.enOutputPipe) {
    int flags;

    pipe(worker->io.outputPipe);
    flags = fcntl(worker->io.outputPipe[0], F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(worker->io.outputPipe[0], F_SETFL, flags);
    worker->io.fpi = fdopen(worker->io.outputPipe[0], "r");
    worker->io.fpo = fdopen(worker->io.outputPipe[1], "w");
    Trav_MgrSetStdout(worker->travMgr, worker->io.fpo);
    Ddi_MgrSetStdout(worker->ddiMgr, worker->io.fpo);
  }

  FbvSetTravMgrOpt(worker->travMgr, worker->opt);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "CREATED Thread (%d - %x)\n", getpid(), pthread_self()));

  worker->result = -1;

  pthread_mutex_unlock(&(worker->thrdInfoMutex));

  CATCH {

    switch (worker->task) {

      case Mc_TaskBmc_c:
      case Mc_TaskBmc2_c:
      case Mc_TaskBmcb_c:
        invarspec = Fsm_MgrReadInvarspecBDD(worker->fsmMgr);
        init = Fsm_MgrReadInitBDD(worker->fsmMgr);
        invar = Fsm_MgrReadConstraintBDD(worker->fsmMgr);

        if (0) {
          pthread_mutex_lock(&travStats->mutex);
          travStats->arrayStats = Pdtutil_Alloc(Pdtutil_Array_t *, 1);
          travStats->arrayStats[0] = Pdtutil_IntegerArrayAlloc(20);
          Pdtutil_ArrayWriteName(travStats->arrayStats[0], "BMCSTATS");
          travStats->arrayStatsNum++;
          pthread_mutex_unlock(&travStats->mutex);
        }
        //    break;
        if (worker->opt->trav.bmcStrategy >= 1) {
          //worker->opt->trav.bmcFirst =
          Trav_TravSatBmcIncrVerif(worker->travMgr, worker->fsmMgr,
            init, invarspec, invar, NULL,
            10000 /*worker->opt->expt.bmcSteps */ , 1 /*bmcStep */ ,
            worker->opt->trav.bmcLearnStep /*bmcLearnStep */ ,
            worker->opt->trav.bmcFirst,
            0 /*apprAig */ , 0, 0 /*interpolantBmcSteps */ ,
            worker->opt->trav.bmcStrategy,
            worker->opt->trav.bmcTe,
            5 /*worker->opt->expt.bmcTimeLimit */ );
          //          -1/*worker->opt->expt.bmcTimeLimit*/);
        } else {
          worker->opt->trav.bmcFirst =
            Trav_TravSatBmcVerif(worker->travMgr, worker->fsmMgr,
            init, invarspec, invar, 10000 /*worker->opt->expt.bmcSteps */ ,
            1 /*bmcStep */ , worker->opt->trav.bmcFirst,
            0 /*apprAig */ , 0, 0 /*interpolantBmcSteps */ ,
            5 /*worker->opt->expt.bmcTimeLimit */ );
          //          -1/*worker->opt->expt.bmcTimeLimit*/);
        }
        worker->result = Trav_MgrReadAssertFlag(worker->travMgr);
        worker->xchg = NULL;

        break;
      case Mc_TaskPdr_c:
      case Mc_TaskPdr2_c:
      case Mc_TaskPdrs_c:

        invarspec = Fsm_MgrReadInvarspecBDD(worker->fsmMgr);
        init = Fsm_MgrReadInitBDD(worker->fsmMgr);
        invar = Fsm_MgrReadConstraintBDD(worker->fsmMgr);

        Trav_TravPdrVerif(worker->travMgr, worker->fsmMgr,
          init, invarspec, invar, NULL, -1, -1);

        worker->result = Trav_MgrReadAssertFlag(worker->travMgr);

        break;
      case Mc_TaskBdd_c:

        worker->opt->expt.bddTimeLimit = 100000;

        Fsm_MgrSetOption(worker->fsmMgr, Pdt_FsmCutThresh_c, inum,
          worker->opt->fsm.cut);
        Fsm_MgrAigToBdd(worker->fsmMgr);

        invarspec = Fsm_MgrReadInvarspecBDD(worker->fsmMgr);
        init = Fsm_MgrReadInitBDD(worker->fsmMgr);
        invar = Fsm_MgrReadConstraintBDD(worker->fsmMgr);
        care = Fsm_MgrReadCareBDD(worker->fsmMgr);
        delta = Fsm_MgrReadDeltaBDD(worker->fsmMgr);

        worker->trMgr = Tr_MgrInit("TR_manager",
          Fsm_MgrReadDdManager(worker->fsmMgr));

        FbvSetTrMgrOpt(worker->trMgr, worker->fsmMgr, worker->opt, 0);

        //      FbvFsmCheckComposePi(fsmMgr2,tr);
        tr = Tr_TrMakePartConjFromFunsWithAuxVars(worker->trMgr,
          Fsm_MgrReadDeltaBDD(worker->fsmMgr),
          Fsm_MgrReadVarNS(worker->fsmMgr),
          Fsm_MgrReadAuxVarBDD(worker->fsmMgr),
          Fsm_MgrReadVarAuxVar(worker->fsmMgr));
        Tr_TrCoiReduction(tr, invarspec);

        FbvFsmCheckComposePi(worker->fsmMgr, tr);

        if (worker->opt->mc.method == Mc_VerifExactBck_c) {
          //      opt->trav.rPlus = "1";
          if (TRAV_BDD_VERIF) {
            Pdtutil_Assert(worker->travMgr != NULL, "NULL trav manager!");
            worker->opt->stats.verificationOK =
              Trav_TravBddApproxForwExactBckFixPointVerify(worker->travMgr,
              worker->fsmMgr, tr, init, invarspec, invar, care, delta);
          } else {
            worker->opt->stats.verificationOK =
              FbvApproxForwExactBckFixPointVerify(worker->fsmMgr, tr, init,
              invarspec, invar, care, delta, worker->opt);
          }
        } else if (worker->opt->mc.method == Mc_VerifApproxFwdExactBck_c) {
          if (TRAV_BDD_VERIF) {
            Pdtutil_Assert(worker->travMgr != NULL, "NULL trav manager!");
            Trav_TravBddApproxForwExactBckFixPointVerify(worker->travMgr,
              worker->fsmMgr, tr, init, invarspec, invar, care, delta);
          } else {
            worker->opt->stats.verificationOK =
              FbvApproxForwExactBckFixPointVerify(worker->fsmMgr, tr, init,
              invarspec, invar, care, delta, worker->opt);
          }
        } else {
          if (TRAV_BDD_VERIF) {
            Pdtutil_Assert(worker->travMgr != NULL, "NULL trav manager!");
            Trav_MgrSetOption(worker->travMgr, Pdt_TravBddTimeLimit_c, inum,
              worker->opt->expt.bddTimeLimit);
            Trav_MgrSetOption(worker->travMgr, Pdt_TravBddNodeLimit_c, inum,
              worker->opt->expt.bddNodeLimit);
            worker->opt->stats.verificationOK =
              Trav_TravBddExactForwVerify(worker->travMgr, worker->fsmMgr, tr,
              init, invarspec, invar, delta);
          } else {
            worker->opt->stats.verificationOK =
              FbvExactForwVerify(NULL, worker->fsmMgr, tr, init, invarspec,
              invar, delta, worker->opt->expt.bddTimeLimit,
              worker->opt->expt.bddNodeLimit, worker->opt);
          }
        }
        if (worker->opt->stats.verificationOK >= 0) {
          /* property proved or failed */
          worker->result = !worker->opt->stats.verificationOK;
        }

        Tr_TrFree(tr);
        Tr_MgrQuit(worker->trMgr);

        break;

      case Mc_TaskItpl_c:
        runLemmas = worker->opt->expt.lemmasSteps;
        worker->opt->mc.lemmas = worker->opt->expt.lemmasSteps;
        worker->opt->trav.itpAppr = 1;
        worker->opt->trav.implAbstr = 0;
        worker->opt->trav.itpInductiveTo = 1;
        worker->opt->trav.dynAbstr = 0;

      case Mc_TaskIgr_c:
        if (worker->task == Mc_TaskIgr_c) {
          worker->opt->trav.itpReuseRings = 2;
        }
      case Mc_TaskItp_c:
        invarspec = Fsm_MgrReadInvarspecBDD(worker->fsmMgr);
        init = Fsm_MgrReadInitBDD(worker->fsmMgr);
        invar = Fsm_MgrReadConstraintBDD(worker->fsmMgr);
        worker->opt->expt.bmcTimeLimit = -1 /*60 */ ;
        worker->opt->mc.lazy = 1;
        worker->opt->common.lazyRate = 1.0;
        if (worker->task == Mc_TaskItp_c) {
          // worker->opt->dynAbstr = 2;
          worker->opt->trav.itpAppr = 0;
          worker->opt->trav.implAbstr = 3;
        }

        FbvSetTravMgrOpt(worker->travMgr, worker->opt);

        Trav_TravSatItpVerif(worker->travMgr, worker->fsmMgr,
          init, invarspec, NULL, invar, NULL /*care */ , runLemmas,
          -1 /*itpBoundLimit */ ,
          worker->opt->common.lazyRate, -1, -1 /*itpTimeLimit */ );

        worker->result = Trav_MgrReadAssertFlag(worker->travMgr);

        break;
      case Mc_TaskNone_c:
      default:

        break;
    }

    //  sleep(1);

#if 0
    Trav_MgrQuit(worker->travMgr);
    if (worker->tId >= 0) {
      /* this is a worker thread */
      Fsm_MgrQuit(worker->fsmMgr);
    }
#endif

  }
  FAIL {

    sleep(1);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Invariant verification UNDECIDED after %s ...\n",
        worker->taskName));
    worker->result = -2;
  }

}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void *
doMcWork1(
  void *voidptr
)
{
  Fbv_ThreadWorker_t *worker = (Fbv_ThreadWorker_t *) voidptr;
  Fbv_ThreadWorker_t *worker2;
  Trav_Stats_t *travStats;
  Mc_Mgr_t *mcMgr;
  int doFork = worker->forkProcess = worker->opt->mc.useChildProc;
  int delayedStart = worker->delayedStart;
  int bl;

  if (worker->task == Mc_TaskSim_c) {
    doFork = 1; // use as child process as ABC not thread safe
  }

  pthread_mutex_lock(&(worker->thrdInfoMutex));

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "CREATED Thread (%d - %x)\n", getpid(), pthread_self()));

  worker->result = -1;

  pthread_mutex_unlock(&(worker->thrdInfoMutex));

  if (delayedStart > 0) {
    sleep(delayedStart);
  }

  /* sub-portfolio package */


  /* preprocessing for worker task */

  if (worker->task == Mc_TaskBddPfl_c ||
    worker->task == Mc_TaskBmcPfl_c ||
    worker->task == Mc_TaskItpPfl_c || worker->task == Mc_TaskPdrPfl_c) {

    engineList_t el;
    int start, num;

    el.thrdNum = 0;

    // portfolio selection
    switch (worker->task) {
      case Mc_TaskBddPfl_c:
        el.thrdEngines[el.thrdNum++] = "BDD";
	//        el.thrdEngines[el.thrdNum++] = "BDD2";
        break;
      case Mc_TaskBmcPfl_c:
        el.thrdEngines[el.thrdNum++] = "BMC";
	if (!worker->opt->expt.deepBound) {
	  el.thrdEngines[el.thrdNum++] = "SIM";
	}
	if (!worker->opt->expt.deepBound) {
	  worker->specific.pfl.blockedId = el.thrdNum;
	  el.thrdEngines[el.thrdNum++] = "BMC2";
	}
	else {
	  if (worker->opt->expt.pflMaxThreads < 0 || 
	      worker->opt->expt.pflMaxThreads > 3) {
	    el.thrdEngines[el.thrdNum++] = "BMC";
	  }
	  if (worker->opt->expt.pflMaxThreads < 0 || 
	      worker->opt->expt.pflMaxThreads > 2) {
	    el.thrdEngines[el.thrdNum++] = "BMC";
	  }
	  if (worker->opt->expt.pflMaxThreads < 0 || 
	      worker->opt->expt.pflMaxThreads > 1) {
	    el.thrdEngines[el.thrdNum++] = "BMC2";
	  }
	  //	  el.thrdEngines[el.thrdNum++] = "BMC";
	}
        break;
      case Mc_TaskItpPfl_c:
        el.thrdEngines[el.thrdNum++] = "ITP";
      	el.thrdEngines[el.thrdNum++] = "IGR";
      	el.thrdEngines[el.thrdNum++] = "IGR2";
        break;
      case Mc_TaskPdrPfl_c:
        el.thrdEngines[el.thrdNum++] = "PDR";
        el.thrdEngines[el.thrdNum++] = "PDR2";
	//        el.thrdEngines[el.thrdNum++] = "PDRS";
        break;
    }
    num = mcGlobalsNewThreads(&el, &start);

    worker->specific.pfl.startId = start;
    worker->specific.pfl.nw = num;

    worker->opt = FbvDupSettings(worker->opt);

    // post init adjustments
    switch (worker->task) {
      case Mc_TaskBmcPfl_c:
	if (worker->specific.pfl.blockedId>=0) {
	  bl = start + worker->specific.pfl.blockedId;
	  worker2 = &(mcGlobals.workers[bl]);
	  worker2->blockedStart = 1;
	  pthread_mutex_init(&(worker2->blockedStartMutex), NULL);
	  pthread_mutex_lock(&worker2->blockedStartMutex);
	}
	Trav_TravBmcSharedInit();
        break;
      case Mc_TaskBddPfl_c:
        break;
      case Mc_TaskItpPfl_c:
        break;
      case Mc_TaskPdrPfl_c:
        break;
    }


    int winnerTh = runSchedThreads(worker,worker->fsmMgr,
      worker->fsmMgrRef,
      worker->opt, 0, start, num);

    FbvFreeSettings(worker->opt);
    worker->opt = NULL;

    if (winnerTh >= 0) {
      if (winnerTh >= mcGlobals.nThreads) {
	worker->result = synWorker.result;
	worker->winTh = synWorker.winTh;
      }
      else {
	worker->result = mcGlobals.workers[winnerTh].result;
	worker->winTh = mcGlobals.workers[winnerTh].winTh;
      }
    } else {
      // aborted
      worker->result = -2;
    }
    return NULL;
  }

  if (worker->task == Mc_TaskSyn_c) {
    pid_t childpid;             /* variable to store the child's pid */

    childpid = fork();
    if (childpid >= 0) {        /* fork succeeded */
      if (childpid == 0) {
        // CHILD
        prctl(PR_SET_PDEATHSIG, SIGHUP);
        signal(SIGHUP, Pdtutil_CatchExit);

        CATCH {
          int status;
          int verbose = worker->opt->verbosity;
          int doUndc = worker->specific.syn.doUndc;
          int proved = Ddi_AbcSynVerif(worker->aigName, doUndc, 600.0, verbose)==1;
          if (proved) status = 0;
          else status = -1;
          printf("CHILD PROC DONE RES: %d (%s - %d)\n", 
                 status, worker->taskName, worker->task);
          printf("EXIT: %d (%s - %d)\n", status, worker->taskName, worker->task);
          _exit(status);  /* child exits with user-provided return code */
        }
        FAIL {
          fprintf(stderr, "Caught SIGHUP (missing parent of %s): process stopped.\n",
                  worker->taskName);
          exit(1);
        }

      } else {
        // PARENT
        pid_t wpid;
        int status;

        worker->childpid = childpid;
        wpid = waitpid(childpid, &status, 0);
        if (status < 0 || status > 2)
	  status = -2;
        if (wpid == -1) {
	  status = -2;
        } 
	else if WIFEXITED(status) {
	    status = WEXITSTATUS(status);
	  }

        worker->result = status;
        printf("RESULT: %d (%s - %d - %d)\n", worker->result,
          worker->taskName, worker->task, worker->tId);
	if (worker->result == 0) {
	  fprintf(stdout, "Verification PASSED\n");
	  fprintf(stdout,
		  "\nInvariant verification completed by thread: %s\n",
		  worker->taskName);
	}
      }
    } else {                    /* fork returns -1 on failure */
      //perror("fork"); /* display error message */
      //      printf("FORK error\n");
      worker->result = -2;
    }
    return NULL;

  }

  if (worker->task == Mc_TaskLms_c &&
    mcGlobals.heuristic == Fbv_HeuristicSec_c &&
    worker->opt->pre.nEqChecks > 1) {
    worker->opt->pre.retime = 1;
    FbvFsmReductions(worker->fsmMgr, worker->opt, 3);
    worker->opt->pre.retime = 0;
    if (worker->opt->stats.verificationOK == 1) {
      //      Fsm_MgrQuit(fsmMgr);
      worker->result = 0;
      printf("\nInvariant verification completed by thread: LMS(RET)\n");
      return NULL;
    }
  }


  Fsm_MgrSetOption(worker->fsmMgr, Pdt_FsmUseAig_c, inum, 1);

  if (worker->blockedStart) {
    printf("BLOCKED START (%s - %d - %d)\n",
	   worker->taskName, worker->task, worker->tId);
    pthread_mutex_lock(&worker->blockedStartMutex);
    printf("UNBLOCKED START (%s - %d - %d)\n",
	   worker->taskName, worker->task, worker->tId);
    pthread_mutex_unlock(&worker->blockedStartMutex);
    
    Pdtutil_Assert(worker->task == Mc_TaskBmc2_c ||
		   worker->task == Mc_TaskBmcb_c,"BMC2 or BMCB needed");
    //   mcMgr->settings.bmcFirst = worker->specific.bmc.bmcFirst;
    //    mcMgr->settings.bmcStep = worker->specific.bmc.bmcStep;
    if (worker->task == Mc_TaskBmc2_c) {
      worker->opt->trav.bmcFirst = worker->specific.bmc.bmcFirst;
      worker->opt->trav.bmcStep = worker->specific.bmc.bmcStep;
    }
    else if (worker->task == Mc_TaskBmcb_c) {
      worker->opt->trav.bmcStrategy = 4;
    }
  }

  mcMgr = Mc_MgrInit("mcThreadedMgr", worker->fsmMgr);
  mcMgr->optList = FbvOpt2OptList(worker->opt);
  mcMgr->auxPtr = (void *)(worker->opt);
  mcMgr->settings.task = worker->task;

  if (worker->opt->expt.deepBound) {
    if (worker->task == Mc_TaskBmc_c) {
      worker->opt->trav.bmcLearnStep = 4;
    }
    else if (worker->task == Mc_TaskBmc2_c) {
      worker->opt->trav.bmcLearnStep = 2;
    }
    if (worker->opt->expt.totMemoryLimit>1000) {
      /* more that 1 GB */
      if (worker->tId>1) {
	/* the observed memory is global (shared) */
	int myMemLimit = worker->opt->expt.totMemoryLimit;
	if (worker->tId>3) {
	  myMemLimit *= 0.4;
	}
	else if (worker->tId>2) {
	  myMemLimit *= 0.6;
	}
	else {
	  myMemLimit *= 0.8;
	}
	mcMgr->settings.bmcMemoryLimit = myMemLimit;
      }
    }
  }

  if (doFork) {
    pid_t childpid;             /* variable to store the child's pid */

    Ddi_AbcLock();
    // ensure ABC is not locked before forking
    childpid = fork();
    Ddi_AbcUnlock();

    if (childpid >= 0) {        /* fork succeeded */

      if (childpid == 0) {
        // CHILD
        prctl(PR_SET_PDEATHSIG, SIGHUP);
        if (Ddi_AbcLocked()) {
          printf("SIM THREAD: ABC IS LOCKED\n");
        }
        int result = Mc_McRun(mcMgr, NULL);

        printf("SIMULATION DONE RES: %d (%s - %d)\n", result, worker->taskName,
          worker->task);
        if (Fsm_MgrReadCexBDD(worker->fsmMgr) != NULL) {
          Pdtutil_Assert(result == 1, "wrong result for cex");
          if (!logThrdCex(worker->fsmMgr, worker->fsmMgrRef, worker->tId,
              worker->opt->expName)) {
            result = -1;
          } else {
            result = 2;
          }
        } else {
          if (result == 1) {
            // undecided as no cex provided
            worker->result = -1;
          }
        }
        // Mc_MgrQuit(mcMgr); 
        printf("EXIT: %d (%s - %d)\n", result, worker->taskName, worker->task);
        _exit(result);          /* child exits with user-provided return code */
      } else {
        // PARENT
        pid_t wpid;
        int status;

        worker->childpid = childpid;
        wpid = waitpid(childpid, &status, 0);
        printf
          ("CHILD EXIT: WE: %d - WES: %d - cp: %d - wi: %d - wn: %s - wpid: %d\n",
          (int)WIFEXITED(status), (int)WEXITSTATUS(status), childpid,
          worker->tId, worker->taskName, wpid);
        if (!WIFEXITED(status))
          status = -2;
        else if (wpid == -1) {
          status = -2;
        } else if WIFEXITED(status) {
            status = WEXITSTATUS(status);
            if (status > 2) status = -2; // user error
        }

        if (status==2) {
          if (!setCexWritten()) {
            /* already written - abort */
            status = -2;
          }
        }

        worker->result = status;
        printf("RESULT: %d (%s - %d - %d)\n", worker->result,
          worker->taskName, worker->task, worker->tId);

      }
    } else {                    /* fork returns -1 on failure */
      //perror("fork"); /* display error message */
      //      printf("FORK error\n");
      worker->result = -2;
    }
  } else {
    Trav_Xchg_t **xchgP = mcGlobals.enTune ? &(worker->xchg) : NULL;
    int result;

    result = Mc_McRun(mcMgr, xchgP);

    if (Fsm_MgrReadCexBDD(worker->fsmMgr) != NULL) {
      //      Pdtutil_Assert(result == 1, "wrong result for cex");
      if (!logThrdCex(worker->fsmMgr, worker->fsmMgrRef, worker->tId,
          worker->opt->expName)) {
        worker->result = -2;
      } else {
        worker->result = 2;
      }
    } else {
      worker->result = result;
      if (result == 1) {
        // undecided as no cex provided
        worker->result = -2;
      }
    }
  }
  // Mc_MgrQuit(mcMgr);
  return NULL;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
tuneWorker(
  Fbv_ThreadWorker_t * masterWorker,
  Fbv_ThreadWorker_t * worker
)
{
  if (!worker->pflWorker && (worker->xchg == NULL))
    return;
  if (worker->completed)
    return;
  Trav_Stats_t *travStats = NULL;
  Trav_Tune_t *travTune = NULL;
  int observeId;
  int lastTime0, lastTime1, totTime, n0, n1;
  float avg0, avg1, pred0, pred1;

  if (!worker->pflWorker) {
    travStats = &(worker->xchg->stats);
    travTune = &(worker->xchg->tune);

    if (travStats == NULL)
      return;
#if 0
    if (travStats->arrayStatsNum == 0)
      return;
    if (travStats->arrayStats == NULL)
      return;
    if (Trav_LogReadSize(travStats) <= 0)
      return;
#endif

    observeId =
      travStats->arrayStatsNum == 0 ? 0 :
      Pdtutil_IntegerArrayNum(travStats->arrayStats[0]) - 1;
  }

  switch (worker->task) {

    case Mc_TaskBmc2_c:
    case Mc_TaskBmcb_c:
      break;

    case Mc_TaskBmc_c:
      /* read data in locked mode */
      observeId = travStats->provedBound;
      /* check if no data w.r.t. previous tune */
      if (observeId < 4) // || observeId <= travStats->lastObserved)
        return;

      pthread_mutex_lock(&travStats->mutex);
      /* rescan after locking, fdue to possible modifications */
      observeId = travStats->lastObserved = travStats->provedBound;
      pthread_mutex_unlock(&travStats->mutex);
      if (observeId>200 && (Pdtutil_ProcRss()/1000 > 4000)) {
	pthread_mutex_lock(&travTune->mutex);
	if (travTune->doSleep==0) {
	  travTune->doSleep = 300;
	}
	pthread_mutex_unlock(&travTune->mutex);
      }
      else if (observeId>50 && (Pdtutil_ProcRss()/1000 > 6000) &&
	       mcGlobals.stats.ng > 300000) {
	pthread_mutex_lock(&travTune->mutex);
	if (travTune->doSleep==0) {
	  travTune->doSleep = 600;
	}
	pthread_mutex_unlock(&travTune->mutex);
      }
      if (masterWorker!=NULL) {
	// within a BMC portfolio
        int num = masterWorker->specific.pfl.nw,
          start = masterWorker->specific.pfl.startId;
	int bmc2Id = start + masterWorker->specific.pfl.blockedId;
	Fbv_ThreadWorker_t *workerB2 = &(mcGlobals.workers[bmc2Id]);
	double totCpu = Pdtutil_WallClockTime();

	if (totCpu>600.0 && workerB2->blockedStart) {
	  if (!worker->opt->expt.deepBound) {
	    if (observeId < 10) {
	      workerB2->specific.bmc.bmcFirst = (observeId+1)*2;
	      workerB2->specific.bmc.bmcStep = 1;
	      pthread_mutex_unlock(&workerB2->blockedStartMutex);
	      workerB2->blockedStart = 0;
	    }
	    else if (observeId < 20) {
	      workerB2->specific.bmc.bmcFirst = (observeId+1)*1.2;
	      workerB2->specific.bmc.bmcStep = 1;
	      pthread_mutex_unlock(&workerB2->blockedStartMutex);
	      workerB2->blockedStart = 0;
	    }
	    else {
	      workerB2->specific.bmc.bmcFirst = observeId*2.5;
	      workerB2->specific.bmc.bmcStep = observeId/8;
	      pthread_mutex_unlock(&workerB2->blockedStartMutex);
	      workerB2->blockedStart = 0;
	    }
	  }
	}
      }

#if 0
      observeId = travStats->lastObserved =
        Pdtutil_IntegerArrayNum(travStats->arrayStats[0]) - 1;
      Pdtutil_Assert(strcmp(Pdtutil_ArrayReadName(travStats->arrayStats[0]),
          "BMCSTATS") == 0, "wrong stats array name");
      lastTime1 =
        Pdtutil_IntegerArrayRead(travStats->arrayStats[0], observeId);
      lastTime0 =
        Pdtutil_IntegerArrayRead(travStats->arrayStats[0], observeId - 1);
      totTime = travStats->cpuTimeTot;
      pthread_mutex_unlock(&travStats->mutex);
      /* now process data */
      Pdtutil_Assert((observeId - 1) > 0, "not enough data for tuning");
      n0 = observeId - 1;
      n1 = n0 + 1;
      avg1 = ((float)(totTime - lastTime1)) / n1;
      avg0 = ((float)(totTime - lastTime1 - lastTime0)) / n0;
      pred1 = avg1;
      pred0 = avg0;
      if (pred1 * 2 < lastTime1 && pred0 * 2 < lastTime0) {
        int i;

        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
          Pdtutil_VerbLevelNone_c, fprintf(stdout,
            "re-tuning required for BMC - time sequence:\n"));
        pthread_mutex_lock(&travStats->mutex);
        for (i = 0; i < observeId; i++) {
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
            Pdtutil_VerbLevelNone_c, fprintf(stdout, " %3.1f",
              (float)Pdtutil_IntegerArrayRead(travStats->arrayStats[0],
                i) / 1000.0));
        }
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
          Pdtutil_VerbLevelNone_c, fprintf(stdout, "\n"));
        pthread_mutex_unlock(&travStats->mutex);
      }
#endif
      break;

    case Mc_TaskBmcPfl_c:

      break;

    case Mc_TaskPdr_c:
    case Mc_TaskPdr2_c:
    case Mc_TaskPdrs_c:

      break;

    case Mc_TaskPdrPfl_c:

      {
        static int size[4], totCpu[4], totcl[4], restart[4], totconfl[4],
          observed[4];
        int j, nObs, num = worker->specific.pfl.nw,
          start = worker->specific.pfl.startId;
	if (start<0) {
	  /* not yet enables */
	  break;
	}

        for (j = nObs = 0; j < num; j++) {
          int id = start + j;

          observed[j] = 0;
          Fbv_ThreadWorker_t *worker_j = &(mcGlobals.workers[id]);

          if (worker_j->xchg == NULL)
            continue;
          if (worker_j->completed)
            continue;
          Trav_Stats_t *travStats_j = &(worker_j->xchg->stats);
          Trav_Tune_t *travTune_j = &(worker_j->xchg->tune);

          if (travTune_j->doAbort)
            continue;

          size[j] = Trav_LogReadSize(travStats_j);
          if (size[j] <= 0)
            continue;
          observed[j] = 1;
          nObs++;
          totCpu[j] = Trav_LogReadTotCpu(travStats_j);
          totcl[j] = Trav_LogReadStats(travStats_j, "TOTCL", size[j] - 1);
          restart[j] = Trav_LogReadStats(travStats_j, "RESTART", size[j] - 1);
          totconfl[j] =
            Trav_LogReadStats(travStats_j, "TOTCONFL", size[j] - 1);
          if (0)
            printf("%s tune: TF: %d - CPU=%g - cl=%d, rst=%d, confl=%d\n",
              worker_j->taskName, size[j], (float)totCpu[j] / 1000.0,
              totcl[j], restart[j], totconfl[j]);
        }
        if (nObs > 1) {
          int maxF = 0, cpu = -1;

          for (j = 0; j < num; j++) {
            if (!observed[j])
              continue;
            if (size[j] > maxF)
              maxF = size[j];
            if (cpu < 0 || totCpu[j] < cpu)
              cpu = totCpu[j];
          }

          if (cpu > 100000) {
            for (j = 0; j < num; j++) {
              if (!observed[j])
                continue;
              if (maxF - size[j] > 3) {
                int id = start + j;
                Fbv_ThreadWorker_t *worker_j = &(mcGlobals.workers[id]);
                Trav_Tune_t *travTune_j = &(worker_j->xchg->tune);

                travTune_j->doAbort = 1;
              }
            }
          }
        }

      }

      break;
    case Mc_TaskBdd_c:


      break;
    case Mc_TaskItp_c:
    case Mc_TaskItpl_c:

      break;
    case Mc_TaskNone_c:
    default:

      break;
  }

}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void *
threadOutputPipes(
  void *voidptr
)
{
  int maxLines = 10;
  char buf[1000];

  while (mcGlobals.workers != NULL) {
    int i;

    for (i = 0; i < mcGlobals.nThreads; i++) {
      int j;

      for (j = 0; j < 10; j++) {
        if (fgets(buf, 999, mcGlobals.workers[i].io.fpi) == NULL) {
          break;
        }
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
          Pdtutil_VerbLevelNone_c, fprintf(stdout, "TH[%d]: %s", i, buf));
      }
    }
    sleep(2);
  }
}

static void
Kill_ChildProc(
  int signum
)
{
  int currThrd;

  printf("SIGTERM received - KILLING child processes\n");

  for (currThrd = 0; currThrd < mcGlobals.nThreads; currThrd++) {
    pid_t childpid = mcGlobals.workers[currThrd].childpid;

    if (childpid > 0) {
      // kill child processe
      kill(childpid, SIGKILL);
    }
  }
  exit(0);
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
runSchedThreads(
  Fbv_ThreadWorker_t *masterWorker,
  Fsm_Mgr_t * fsmMgr,
  Fsm_Mgr_t * fsmMgrRef,
  Fbv_Globals_t * opt,
  int level,
  int start,
  int num
)
{
  int currThrd, end;
  int enOutputPipes = 0;
  int winningThrd = -1, verificationResult, again = 1;
  int nCompleted = 0;

  if (start < 0) {
    start = mcGlobals.nActiveThreads;
  }
  if (num < 0) {
    end = mcGlobals.nThreads;
  } else {
    end = start + num;
  }

  if (synWorker.result == 0) {
    return 100;
  }

  for (currThrd = start; currThrd < end; currThrd++) {
    Fsm_Mgr_t *fsmMgr2 = NULL;

    if (mcGlobals.workers[currThrd].task == Mc_TaskPdrPfl_c) {
      if ((mcGlobals.heuristic == Fbv_HeuristicBmcBig_c ||
	   mcGlobals.heuristic == Fbv_HeuristicIbm_c) &&
	  mcGlobals.stats.ng > 300000) {
	mcGlobals.workers[currThrd].delayedStart = 300;
      }
    }

    if ((mcGlobals.workers[currThrd].task == Mc_TaskPdr_c
        || mcGlobals.workers[currThrd].task == Mc_TaskPdr2_c
        || mcGlobals.workers[currThrd].task == Mc_TaskPdrs_c
        || mcGlobals.workers[currThrd].task == Mc_TaskPdrPfl_c) &&
      opt->pre.retimed) {
      fsmMgr2 = Fsm_MgrDup(fsmMgrRef);
    } else {
      fsmMgr2 = Fsm_MgrDup(fsmMgr);
    }
    FbvSetDdiMgrOpt(Fsm_MgrReadDdManager(fsmMgr2), opt, 2);

    Pdtutil_Assert(Fsm_MgrReadCareBDD(fsmMgr2) == NULL, "NULL care required");

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "\nRunning %s ...\n",
        mcGlobals.workers[currThrd].taskName));
    if (opt->mc.useChildCall) {
      mcGlobals.workers[currThrd].call = 1;
    }
    mcGlobals.workers[currThrd].tId = currThrd;
    mcGlobals.workers[currThrd].winTh = currThrd;
    mcGlobals.workers[currThrd].result = -1;
    mcGlobals.workers[currThrd].io.enOutputPipe = enOutputPipes;

    if (!callWorker(&(mcGlobals.workers[currThrd]), fsmMgr2, fsmMgrRef,
        mcGlobals.workers[currThrd].task, opt)) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "\nThread %s activated\n",
          mcGlobals.workers[currThrd].taskName));
    } else {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout,
          "\nError creating Thread %s\n",
          mcGlobals.workers[currThrd].taskName));
    }

    Fsm_MgrQuit(fsmMgr2);
    //    travMgrAig = Trav_MgrQuit(travMgrAig);
    //    Ddi_MgrFreeExtRef(ddiMgr, lastDdiNode);

  }

  if (enOutputPipes) {
    pthread_t ioThread;

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "CREATING IO Thread\n"));
    if (pthread_create(&(ioThread), NULL, threadOutputPipes, NULL)) {
      return 1;
    }
  }

  if (opt->trav.pdrShareReached || opt->trav.itpShareReached) {
    Trav_Shared_t *travPdrShared = NULL, *travShared = NULL;

    for (currThrd = 0; currThrd < mcGlobals.nThreads; currThrd++) {

      pthread_mutex_lock(&(mcGlobals.workers[currThrd].thrdInfoMutex));

      if (mcGlobals.workers[currThrd].task == Mc_TaskPdr_c) {
        travPdrShared = &(mcGlobals.workers[currThrd].xchg->shared);
      } else if (mcGlobals.workers[currThrd].task == Mc_TaskItp_c) {
        travShared = &(mcGlobals.workers[currThrd].xchg->shared);
      }

      pthread_mutex_unlock(&(mcGlobals.workers[currThrd].thrdInfoMutex));

    }

    Pdtutil_Assert(travShared != NULL && travPdrShared != NULL,
      "NULL shared struct");

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "SHARED ptrs: %ux - %ux\n", travShared, travPdrShared));
    travShared->sharedLink = travPdrShared;
    travPdrShared->sharedLink = travShared;
  }

  opt->stats.setupTime = util_cpu_time();

  while (again) {

    if (synWorker.result == 0) {
      winningThrd = 100; 
      break;
    }

    for (currThrd = start; currThrd < end; currThrd++) {
      /* BMC just for falsification ! */
      int ok = mcGlobals.workers[currThrd].task == Mc_TaskBmc_c ? 1 : 0;

      if (mcGlobals.workers[currThrd].task == Mc_TaskSim_c)
        ok = 1;

      pthread_mutex_lock(&(mcGlobals.workers[currThrd].thrdInfoMutex));

      verificationResult = mcGlobals.workers[currThrd].result;

      if (verificationResult >= ok) {
        fprintf(stdout, "\nInvariant verification completed by engine: %s\n",
          mcGlobals.workers[currThrd].taskName);
        /* property failed */
        //      Fsm_MgrQuit(fsmMgr2);
        mcGlobals.enTune = 0;
        winningThrd = mcGlobals.workers[currThrd].winTh;
        mcGlobals.workers[currThrd].completed = 1;
        pthread_mutex_unlock(&(mcGlobals.workers[currThrd].thrdInfoMutex));
        nCompleted++;
        again = 0;
        break;
      }
      if (verificationResult < -1) {
        mcGlobals.workers[currThrd].result = -1;
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
          Pdtutil_VerbLevelNone_c, fprintf(stdout,
            "Invariant verification aborted by engine: %s\n",
            mcGlobals.workers[currThrd].taskName));
        nCompleted++;
        mcGlobals.workers[currThrd].completed = 1;
        if (nCompleted + start >= end) {
          pthread_mutex_unlock(&(mcGlobals.workers[currThrd].thrdInfoMutex));
          again = 0;
          break;
        }
      } else if (mcGlobals.enTune) {
        tuneWorker(masterWorker,&(mcGlobals.workers[currThrd]));
      }

      pthread_mutex_unlock(&(mcGlobals.workers[currThrd].thrdInfoMutex));

      //      printf("Invariant verification UNDECIDED after PDR ...\n");
    }

    if (again)
      sleep(1);
  }

  return winningThrd;

}


static int
optSelectEngine(
  Fbv_Globals_t * opt,
  engineList_t * elp,
  const char *engName
)
{
  int i, disabled = 0;

  if (opt->expt.thrdEnNum > 0) {
    disabled = 1;
    for (i = 0; i < opt->expt.thrdEnNum; i++) {
      if (strcmp(opt->expt.thrdEn[i], engName) == 0)
        disabled = 0;
    }
  } else {
    for (i = 0; i < opt->expt.thrdDisNum; i++) {
      if (strcmp(opt->expt.thrdDis[i], engName) == 0)
        disabled = 1;
    }
  }
  if (!disabled && elp!=NULL)
    elp->thrdEngines[elp->thrdNum++] = (char *)engName;

  return !disabled;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
optSelectEngines(
  Fbv_Globals_t * opt,
  engineList_t * elp,
  int heuristic
)
{
  elp->thrdNum = 0;

  if (opt->expt.doRunBmc > 1)
    optSelectEngine(opt, elp, "BMC");

  if (opt->expt.doRunPdr > 1) {
    optSelectEngine(opt, elp, "PDR");
    optSelectEngine(opt, elp, "PDR2");
  }
  //if (opt->expt.doRunPdr > 1) optSelectEngine(opt, elp, "PDRS");
  //  optSelectEngine(opt, elp, "IGRPDR");

  if (opt->expt.doRunLemmas > 1)
    optSelectEngine(opt, elp, "LMS");
  if (opt->expt.doRunSim > 1)
    optSelectEngine(opt, elp, "SIM");
  if (opt->expt.doRunSyn > 1)
    optSelectEngine(opt, elp, "SYN");
  if (opt->expt.doRunItp > 1) {
    optSelectEngine(opt, elp, "ITP");
    optSelectEngine(opt, elp, "ITP2");
  }
  if (opt->expt.doRunIgr > 1) {
    optSelectEngine(opt, elp, "IGR");
    optSelectEngine(opt, elp, "IGR2");
    //    optSelectEngine(opt, elp, "IGRPDR");
  }
  if (opt->expt.doRunBdd > 1)
    optSelectEngine(opt, elp, "BDD");
  if (opt->expt.doRunBdd2 > 1)
    optSelectEngine(opt, elp, "BDD2");


  if (opt->expt.doRunPdrPfl > 1)
    optSelectEngine(opt, elp, "PDRPFL");
  if (opt->expt.doRunItpPfl > 1)
    optSelectEngine(opt, elp, "ITPPFL");
  if (opt->expt.doRunBmcPfl > 1)
    optSelectEngine(opt, elp, "BMCPFL");
  if (opt->expt.doRunBddPfl > 1)
    optSelectEngine(opt, elp, "BDDPFL");

  if (opt->expt.doRunBmc == 1)
    optSelectEngine(opt, elp, "BMC");

  if (opt->expt.doRunPdr == 1)
    optSelectEngine(opt, elp, "PDR");
  if (opt->expt.doRunPdr == 1)
    optSelectEngine(opt, elp, "PDR2");
  //if (opt->expt.doRunPdr == 1) optSelectEngine(opt, elp, "PDRS");

  if (opt->expt.doRunLemmas == 1)
    optSelectEngine(opt, elp, "LMS");
  if (opt->expt.doRunSim == 1)
    optSelectEngine(opt, elp, "SIM");
  if (opt->expt.doRunSyn == 1)
    optSelectEngine(opt, elp, "SYN");
  if (opt->expt.doRunItp == 1)
    optSelectEngine(opt, elp, "ITP");
  if (opt->expt.doRunIgr == 1)
    optSelectEngine(opt, elp, "IGR");
  if (opt->expt.doRunBdd == 1)
    optSelectEngine(opt, elp, "BDD");
  if (opt->expt.doRunBdd2 == 1)
    optSelectEngine(opt, elp, "BDD2");

  if (opt->expt.doRunPdrPfl == 1)
    optSelectEngine(opt, elp, "PDRPFL");
  if (opt->expt.doRunItpPfl == 1)
    optSelectEngine(opt, elp, "ITPPFL");
  if (opt->expt.doRunBmcPfl == 1)
    optSelectEngine(opt, elp, "BMCPFL");
  if (opt->expt.doRunBddPfl == 1)
    optSelectEngine(opt, elp, "BDDPFL");


  return 1;
}



static int cexWritten = 0;

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
logThrdCex(
  Fsm_Mgr_t * fsmMgr,
  Fsm_Mgr_t * fsmMgrRef,
  int tId,
  char *aigerName
)
{
  Ddi_Bdd_t *cex = Fsm_MgrReadCexBDD(fsmMgr);
  int ret = 0, invalidCex = 0;
  char fname[500];
  char cmd[1000];
  int runAigsim = 1;

  if (cex == NULL)
    return 0;

  // used as fsmMgrRef can be shared (with threads)
  pthread_mutex_lock(&(mcGlobals.shareMutex));

  if (!cexWritten) {
    sprintf(fname, "/tmp/%s.%d", Pdtutil_WresFileName(), tId);

    //    FbvWriteCex(fname, fsmMgr, fsmMgrRef);
    if (!Fsm_CexNormalize(fsmMgr)) {
      if (Fsm_MgrReadRemovedPis(fsmMgr)) {
        Ddi_Bdd_t *cex = Fsm_MgrReadCexBDD(fsmMgr);
	int ret2;
        Fsm_MgrSetCexBDD(fsmMgrRef, cex);
        Trav_TravSatBmcRefineCex(fsmMgrRef,
          Fsm_MgrReadInitBDD(fsmMgrRef),
          Fsm_MgrReadInvarspecBDD(fsmMgrRef),
          Fsm_MgrReadConstraintBDD(fsmMgrRef), 
	  Fsm_MgrReadCexBDD(fsmMgrRef));
	ret2 = Fsm_CexNormalize(fsmMgrRef);
	Pdtutil_Assert(!ret2,"error extending cex");
        FbvWriteCexNormalized(fname, fsmMgrRef, fsmMgrRef, 0);
      } else {
        FbvWriteCexNormalized(fname, fsmMgr, fsmMgrRef, 0);
      }
      fprintf(stdout, "\ncex written to: %s.cex\n", fname);
    }

    if (runAigsim) {
      strcat(fname, ".cex");
      invalidCex = Fsm_AigsimCex(aigerName, fname);

      if (!invalidCex) {
        fprintf(stdout, "CEX validated by aigsim\n");
        cexWritten = 1;
        ret = 1;

      } else {
        fprintf(stdout, "CEX NOT validated by aigsim !!!\n");
        ret = 0;
      }
      //      Pdtutil_Assert(!invalidCex,"cex not validated");
    } else {
      cexWritten = 1;
      ret = 1;
    }

  }

  pthread_mutex_unlock(&(mcGlobals.shareMutex));
  return ret;
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
setCexWritten(
  void
)
{
  int ret = 0;
  // used as fsmMgrRef can be shared (with threads)
  pthread_mutex_lock(&(mcGlobals.shareMutex));

  if (!cexWritten) {
    cexWritten = 1;
    ret = 1;
  }
  
  pthread_mutex_unlock(&(mcGlobals.shareMutex));

  return ret;
}
 

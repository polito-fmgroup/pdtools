/**CFile***********************************************************************

  FileName    [travPdr.c]

  PackageName [trav]

  Synopsis    [IC3 (Proof Directed Reachebility) based verif/traversal routines]

  Description []

  SeeAlso   []

  Author    [Gianpiero Cabodi]

  Copyright [This file was created at the Politecnico di Torino,
    Torino, Italy.
    The  Politecnico di Torino makes no warranty about the suitability of
    this software for any purpose.
    It is presented on an AS IS basis.
  ]

  Revision  []

******************************************************************************/

#include "travInt.h"
#include "ddiInt.h"
#include "baigInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/* SAT calls identifiers */
#define PDR_TARGET_INTERSECTION_CALL 0
#define PDR_IS_BLOCKED_CALL 1
#define PDR_INDUCTIVE_PREDECESSOR_CALL 2
#define PDR_INDUCTIVE_GENERALIZATION_CALL 3
#define PDR_CLAUSE_PUSH_CALL 4
#define PDR_INDUCTION_BASE_CALL 5
#define PDR_INDUCTIVE_PUSH_CALL 6

/* Inductive predecessor modes */
#define PDR_BLOCKING_MODE 0
#define PDR_GENERALIZE_MODE 1
#define PDR_INDUCTIVE_PUSH_MODE 2
#define PDR_DEBUG_MODE 2

/* Restart strategies ids */
#define PDR_DEFAULT_RESTART 0
#define PDR_CUBE_DEPENDENT_RESTART 1
#define PDR_DOUBLE_THRESHOLD_RESTART 2
#define PDR_VAR_THRESHOLD_RESTART 3

/* TR loading status ids */
#define PDR_NOT_LOADED 0
#define PDR_LOADED_POS_POLARITY 1
#define PDR_LOADED_NEG_POLARITY 2
#define PDR_LOADED_BOTH_POLARITIES 3

/* TR counting status ids */
#define PDR_NOT_COUNTED 0
#define PDR_COUNTED 1

/* Restart constants */
#define USELESS_THRESHOLD 0.5

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

TravPdrMgr_t *gblPdrMgr = NULL;

long cexGenTime[4] = { 0, 0, 0, 0 };

long nContained = 0;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#define EN_PDR_DBG 0

#if EN_PDR_DBG
#define PDR_DBG1(...) {__VA_ARGS__ ;}
#if EN_PDR_DBG > 1
#define PDR_DBG(...) {__VA_ARGS__ ;}
#else
#define PDR_DBG(...) {}
#endif
#else
#define PDR_DBG(...) {}
#define PDR_DBG1(...) {}
#endif

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static void pdrTimeFrameAdd(
  TravPdrMgr_t * pdrMgr,
  int k,
  int reuseEnabled
);
static void pdrReuseTimeFrames(
  TravPdrMgr_t * pdrMgr
);
static void pdrTimeFrameAssertInvarspec(
  TravPdrMgr_t * pdrMgr,
  int k
);
static int pdrTimeFrameCheckTarget(
  TravPdrMgr_t * pdrMgr,
  int k,
  int *specCall
);
static int pdrTimeFrameCheckTargetTot(
  TravPdrMgr_t * pdrMgr,
  int k,
  int *specCall
);
static Ddi_Bdd_t *pdrBlockCube(
  TravPdrMgr_t * pdrMgr,
  Ddi_Clause_t * cube,
  int myK,
  int *pAbort,
  int *pForceSuspend
);
static int pdrTimeFramePushClauses(
  TravPdrMgr_t * pdrMgr,
  int maxFpK
);
static void pdrQueuePushCube(
  TravPdrMgr_t * pdrMgr,
  Ddi_Clause_t * cube,
  int k,
  int prio
);
static Ddi_Clause_t *pdrQueuePopCube(
  TravPdrMgr_t * pdrMgr,
  int *kp,
  int *prio
);
static Ddi_Clause_t *pdrInductivePredecessor(
  TravPdrMgr_t * pdrMgr,
  int k,
  Ddi_Clause_t * cube,
  Ddi_Clause_t ** pCore,
  int enTernary,
  int enReuseActVar,
  int useInduction,
  int mode
);
static Ddi_Clause_t *pdrInductivePredecessorNoActLit(
  TravPdrMgr_t * pdrMgr,
  int k,
  Ddi_Clause_t * cube,
  int enTernary,
  int mode
);
static Ddi_Clause_t *pdrPushClause(
  TravPdrMgr_t * pdrMgr,
  int k,
  Ddi_Clause_t * cl
);
static Ddi_Clause_t *pdrGeneralizeClause(
  TravPdrMgr_t * pdrMgr,
  int k,
  Ddi_Clause_t * cube,
  int generalizeLevel
);
static int pdrChkGenWithInitAndRefine(
  TravPdrMgr_t * pdrMgr,
  Ddi_Clause_t * clMin,
  Ddi_Clause_t * cube,
  int checkFull
);
static void pdrCheckSolvers(
  TravPdrMgr_t * pdrMgr
);
static int pdrTimeFrameShareClauses(
  Trav_Mgr_t * travMgr,
  TravPdrMgr_t * pdrMgr,
  int k
);
static int pdrClauseIsContained(
  TravPdrMgr_t * pdrMgr,
  Ddi_Clause_t * cl,
  int k0
);
static void pdrCheckSolverCustom(
  TravPdrMgr_t * pdrMgr,
  int k
);
static int pdrTimeFrameCheckClauses(
  TravPdrMgr_t * pdrMgr
);
static Ddi_Clause_t *pdrExtendCex(
  TravPdrMgr_t * pdrMgr,
  Ddi_SatSolver_t * solver,
  Ddi_Clause_t * cex,
  Ddi_Clause_t * cube,
  int enTernary
);

static Ddi_ClauseArray_t *pdrFindOrAddIncrementalClausesWithPg(
  TravPdrTimeFrame_t * tf,
  Ddi_PdrMgr_t * ddiPdr,
  Ddi_Clause_t * cl,
  int usePsClause,
  int useSolver2
);
static int pdrFindOrAddIncrementalClausesWithPgRecur(
  TravPdrTimeFrame_t * tf,
  Ddi_PdrMgr_t * ddiPdr,
  int isOdd,
  int gateId,
  Ddi_ClauseArray_t * out,
  int rootVar,
  int useSolver2,
  int level,
  int ldrGateId
);
static int pdrFindOrAddIncrementalClausesRecur(
  TravPdrTimeFrame_t * tf,
  Ddi_PdrMgr_t * ddiPdr,
  int gateId,
  Ddi_ClauseArray_t * clausesToLoad,
  int useSolver2,
  int level
);
static Ddi_ClauseArray_t *pdrFindOrAddIncrementalClauses(
  TravPdrTimeFrame_t * tf,
  Ddi_PdrMgr_t * ddiPdr,
  Ddi_Clause_t * cl,
  int usePsClause,
  int useSolver2
);
static int computeConeSize(
  TravPdrMgr_t * pdrMgr,
  Ddi_Clause_t * cl,
  int usePsClause,
  int overApproximate
);
static int computeConeSizeRecur(
  TravPdrMgr_t * pdrMgr,
  int gateId,
  int overApproximate
);

static void pdrTimeFrameLoadSolver(
  TravPdrMgr_t * pdrMgr,
  Ddi_SatSolver_t * solver,
  int k,
  int incrementalLoad,
  int useSolver2
);
static void timeFrameUpdateEqClasses(
  TravPdrMgr_t * pdrMgr,
  int k
);
static void timeFrameUpdateRplusClasses(
  TravPdrMgr_t * pdrMgr,
  int k
);
static int pdrSolverCheckRestart(
  TravPdrMgr_t * pdrMgr,
  int k,
  int useSolver2
);
static void bumpActTopologically(
  TravPdrTimeFrame_t * tf,
  TravPdrMgr_t * pdrMgr,
  int useSolver2
);
static void printClause(
  Ddi_Clause_t * cla
);
static int isSpecializedCall(
  TravPdrMgr_t * pdrMgr,
  int callId,
  int k
);
static void registerCall(
  TravPdrMgr_t * pdrMgr,
  int k,
  int useSolver2,
  float value
);

static int computeConeSizeWithPg(
  TravPdrMgr_t * pdrMgr,
  Ddi_Clause_t * cl,
  int usePsClause
);
static int computeConeSizeWithPgRecur(
  TravPdrMgr_t * pdrMgr,
  int gateId,
  int isOdd,
  int ldrGateId,
  int ldrVar
);

static int pdrEnumerateTargetCubes(
  TravPdrMgr_t * pdrMgr
);
static int pdrOutOfLimits(
  TravPdrMgr_t * pdrMgr
);
static int
pdrCheckFixPointSat(
  TravPdrMgr_t * pdrMgr,
  int k,
  int kMax
);
static int
checkRingInclusion(
  Trav_ItpMgr_t *itpMgr
);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

static int customRestart = 0;

int
chkCustom(
  TravPdrMgr_t * pdrMgr,
  int restart
)
{
  Ddi_Clause_t *cl = Ddi_ClauseAlloc(0, 5);
  TravPdrTimeFrame_t *tf = &(pdrMgr->timeFrames[2]);

  DdiClauseAddLiteral(cl, 191);
  DdiClauseAddLiteral(cl, -283);
  DdiClauseAddLiteral(cl, -297);
  DdiClauseAddLiteral(cl, 311);
  DdiClauseAddLiteral(cl, -315);

  if (restart) {
    customRestart = 1;
    pdrSolverCheckRestart(pdrMgr, 2, 0);
    customRestart = 0;
  }
  return Ddi_SatSolveCustomized(tf->solver, cl, 1, 1, -1);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
isSpecializedCall(
  TravPdrMgr_t * pdrMgr,
  int callId,
  int k
)
{

  TravPdrTimeFrame_t *tf = &(pdrMgr->timeFrames[k]);
  char specCallsMask = pdrMgr->settings.specCallsMask;

  //Check if time frame has auxiliary solver
  if (!tf->hasSolver2) {
    return 0;
  }

  char res = specCallsMask & (((char)1) << callId);

  return res > 0;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
registerCall(
  TravPdrMgr_t * pdrMgr,
  int k,
  int useSolver2,
  float value
)
{
  float *restartWindow, *wndSum;
  float prevValue;
  int *wndEnd;

  //Pdtutil_Assert(value >= 0, "");

  TravPdrTimeFrame_t *tf = &(pdrMgr->timeFrames[k]);

  Pdtutil_Assert(!useSolver2 || tf->hasSolver2, "Wrong call registration.");
  restartWindow = useSolver2 ? tf->restartWindow2 : tf->restartWindow;
  wndEnd = useSolver2 ? &(tf->wndEnd2) : &(tf->wndEnd);
  wndSum = useSolver2 ? &(tf->wndSum2) : &(tf->wndSum);
  prevValue = restartWindow[*wndEnd];

  restartWindow[(*wndEnd)] = value;
  (*wndSum) = (*wndSum) - prevValue + value;
  (*wndEnd) = ((*wndEnd) + 1) % TRAV_PDR_RESTART_WINDOW_SIZE;

}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
TravPdrMgr_t *
Trav_PdrMgrInit(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bddarray_t * initStub,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  int nTF
)
{
  int i;

  //Initialize ddi manager
  TravPdrMgr_t *pdrMgr;
  Ddi_Mgr_t *ddiMgr = Fsm_MgrReadDdManager(fsmMgr);
  Ddi_Var_t *cv = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
  Ddi_Var_t *pv = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
  Ddi_Bdd_t *myInvarspec = Ddi_BddDup(invarspec);
  Ddi_Bdd_t *myInvar = Ddi_BddMakeLiteralAig(cv, 1);
  Ddi_Bdd_t *myProp = Ddi_BddMakeLiteralAig(pv, 1);
  Ddi_Bddarray_t *myDelta = Ddi_BddarrayDup(Fsm_MgrReadDeltaBDD(fsmMgr));

//TOASK
//TODO: rimuovere queste compilazioni condizionali?
#if 0
  if (invar != NULL) {
    Ddi_BddAndAcc(myInvar, invar);
  }
  //  Ddi_BddAndAcc(myInvar, myProp);
#endif

#if 0
  //  Ddi_Free(myInvar);
  // myInvar = Ddi_BddDup(invar);
  if (care != NULL && !Ddi_BddIsOne(care)) {
    myInvar = Ddi_BddDup(care);
  }
#endif
  Ddi_Free(myProp);

  //Allocate and initialize trav pdr manager
  pdrMgr = Pdtutil_Alloc(TravPdrMgr_t, 1);
  pdrMgr->nTimeFrames = 0;
  pdrMgr->maxTimeFrames = 0;
  pdrMgr->nClauses = 0;
  pdrMgr->nRemoved = 0;
  pdrMgr->maxItpShare = 0;
  pdrMgr->fixPointTimeFrame = -1;
  pdrMgr->timeFrames = NULL;
  pdrMgr->infTimeFrame = NULL;
  pdrMgr->queueMaxPrio = TRAV_PDR_QUEUE_MAXPRIO;
  pdrMgr->fsmMgr = fsmMgr;
  pdrMgr->travMgr = travMgr;

  pdrMgr->stats.result = -1;

  //Initialize ddiPdr manager with CNF
  pdrMgr->settings.unfoldProp = travMgr->settings.aig.pdrUnfold;
  pdrMgr->ddiPdr = Ddi_PdrMake(myDelta,
    Fsm_MgrReadVarI(fsmMgr),
    Fsm_MgrReadVarPS(fsmMgr),
    Fsm_MgrReadVarNS(fsmMgr),
    init, initStub, myInvar, myInvarspec, pdrMgr->settings.unfoldProp);

  if (Ddi_BddIsConstant(myInvarspec)) {
    pdrMgr->stats.result = Ddi_BddIsZero(myInvarspec);
  }
  //Initialize ternary simulator
  pdrMgr->xMgr = Ddi_TernarySimInit(myDelta,
    myInvarspec, Fsm_MgrReadVarI(fsmMgr), Fsm_MgrReadVarPS(fsmMgr));

  //Initialize auxiliary SAT solver
  pdrMgr->auxSolver = Ddi_SatSolverAlloc();
  DdiSatSolverSetFreeVarMinId(pdrMgr->auxSolver, pdrMgr->ddiPdr->freeCnfVar);
  Ddi_SatSolverAddClauses(pdrMgr->auxSolver, pdrMgr->ddiPdr->trClauses);
  Ddi_SatSolverAddClauses(pdrMgr->auxSolver, pdrMgr->ddiPdr->targetClauses);

  //Initialize target subsetting data structures
  pdrMgr->settings.targetSubset = 0;
  pdrMgr->targetCube = NULL;
  pdrMgr->actTargetTot = NULL;
  pdrMgr->cubes = 0;
  if (pdrMgr->settings.targetSubset) {
    pdrMgr->targetEnumSolver = Ddi_SatSolverAlloc();
    DdiSatSolverSetFreeVarMinId(pdrMgr->targetEnumSolver,
      pdrMgr->ddiPdr->freeCnfVar);
    Ddi_SatSolverAddClauses(pdrMgr->targetEnumSolver,
      pdrMgr->ddiPdr->targetClauses);
    pdrMgr->actTargetTot = pdrMgr->ddiPdr->actTarget;
  }
  //Initialize mappings for cone size computing
  Ddi_NetlistClausesInfo_t *netCl = &(pdrMgr->ddiPdr->trNetlist);
  pdrMgr->tempAigMap = Pdtutil_Alloc(int,
    netCl->nGate
  );
  pdrMgr->conesSize = Pdtutil_Alloc(int,
    netCl->nRoot
  );

  for (i = 0; i < netCl->nRoot; i++) {
    pdrMgr->conesSize[i] = PDR_NOT_COUNTED;
  }
  for (i = 0; i < netCl->nGate; i++) {
    pdrMgr->tempAigMap[i] = PDR_NOT_COUNTED;
  }
  Ddi_Free(myInvarspec);
  Ddi_Free(myInvar);

  //Initialize global PDR statistics
  pdrMgr->stats.nSatSolverCalls = 0;
  pdrMgr->stats.nUnsat = 0;
  pdrMgr->stats.nQueuePop = 0;
  pdrMgr->stats.nGeneralize = 0;
  pdrMgr->stats.nClausePush = 0;
  pdrMgr->stats.nTernarySim = 0;
  pdrMgr->stats.nTernaryInit = 0;
  pdrMgr->stats.nBlocked = 0;
  pdrMgr->stats.nBlocks = 0;
  pdrMgr->stats.solverTime = 0.0;
  pdrMgr->stats.generalizeTime = 0.0;
  pdrMgr->stats.cexgenTime = 0.0;
  pdrMgr->stats.restartTime = 0.0;
  pdrMgr->stats.solverRestart = 0;
  pdrMgr->stats.maxBwdDepth = 0;
  pdrMgr->stats.nDec = 0;
  pdrMgr->stats.nConfl = 0;
  pdrMgr->stats.nProp = 0;
  pdrMgr->stats.avgTrLoadedClauses = 0.0;
  pdrMgr->stats.avgTrLoadedClausesOld = 0.0;
  for (i = 0; i < TRAV_PDR_MAX_SPLIT_STAT; i++)
    pdrMgr->stats.split[i] = 0;

  //Initialize global PDR settings
  pdrMgr->settings.indAssumeLits = travMgr->settings.aig.pdrIndAssumeLits;
  pdrMgr->ddiPdr->doPostOrderClauseLoad = travMgr->settings.aig.pdrPostordCnf;
  pdrMgr->settings.step = 0;
  pdrMgr->settings.ternarySimInit = 2;
  pdrMgr->settings.incrementalTrClauses =
    travMgr->settings.aig.pdrIncrementalTr;
  pdrMgr->settings.genEffort = travMgr->settings.aig.pdrGenEffort;
  pdrMgr->settings.solverRestartEnable = 1;
  pdrMgr->settings.enforceInductiveClauses = 0;
  pdrMgr->settings.fwdEquivalences = travMgr->settings.aig.pdrFwdEq;
  pdrMgr->settings.infTf = travMgr->settings.aig.pdrInf;
  pdrMgr->settings.cexJustify = travMgr->settings.aig.pdrCexJustify;
  pdrMgr->settings.shareReached = travMgr->settings.aig.pdrShareReached;
  pdrMgr->settings.shareItpReached = travMgr->settings.aig.itpShareReached;
  pdrMgr->settings.verbosity = travMgr->settings.verbosity;
  char restartStrategyOpt = (char)travMgr->settings.aig.pdrRestartStrategy;

  pdrMgr->settings.restartStrategy = (int)(restartStrategyOpt & 0x0F);
  pdrMgr->settings.restartStrategy2 = (int)((restartStrategyOpt >> 4) & 0x0F);
  pdrMgr->settings.usePgEnconding = travMgr->settings.aig.pdrUsePgEncoding;
  if (pdrMgr->settings.usePgEnconding > 1)
    pdrMgr->settings.usePgEnconding = 1;
  pdrMgr->settings.bumpActTopologically =
    travMgr->settings.aig.pdrBumpActTopologically;
  if (pdrMgr->settings.enforceInductiveClauses) {
    pdrMgr->settings.incrementalTrClauses = 0;
  }
  char specializedCallsMaskOpt =
    (char)travMgr->settings.aig.pdrSpecializedCallsMask;
  pdrMgr->settings.specCallsMask = specializedCallsMaskOpt;
  pdrMgr->settings.useSolver2 = (specializedCallsMaskOpt != 0x00);
  pdrMgr->settings.overApproximateConeSize = 0;
  pdrMgr->settings.restartThreshold = USELESS_THRESHOLD;
  pdrMgr->settings.restartThreshold2 = USELESS_THRESHOLD;

  //Initialize forward equivalence classes
  if (pdrMgr->settings.fwdEquivalences) {
    int i, nVars = Ddi_VararrayNum(Fsm_MgrReadVarPS(fsmMgr));

    pdrMgr->eqCl = Pdtutil_EqClassesAlloc(nVars + 1);
    /* all members unassigned and classes not active */
    Pdtutil_EqClassesReset(pdrMgr->eqCl);
    /* define constant class */
    Pdtutil_EqClassSetLdr(pdrMgr->eqCl, 0, 0);
    for (i = 0; i <= nVars; i++) {
      if (i == 0) {
        Pdtutil_EqClassCnfVarWrite(pdrMgr->eqCl, 0, -1);    /* dummy */
      } else {
        Pdtutil_EqClassCnfVarWrite(pdrMgr->eqCl, i,
          pdrMgr->ddiPdr->nsCnf[i - 1]);
      }
    }
  } else {
    pdrMgr->eqCl = NULL;
  }

  Ddi_Free(myDelta);

  //Set global reference to PDR manager
  gblPdrMgr = pdrMgr;

  return pdrMgr;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Trav_PdrMgrQuit(
  TravPdrMgr_t * pdrMgr
)
{
  int i, j;
  TravPdrTimeFrame_t *tf;
  int reuseFrames = pdrMgr->travMgr->settings.aig.pdrReuseRings ? 2 : 0;

  //Collect statistics from every time frame
  for (i = 0; i < pdrMgr->nTimeFrames; i++) {
    tf = &(pdrMgr->timeFrames[i]);
    int d = (int)(tf->solver->S->stats.decisions);
    int c = (int)(tf->solver->S->stats.conflicts);
    int p = (int)(tf->solver->S->stats.propagations);

    pdrMgr->stats.nDec += d;
    pdrMgr->stats.nProp += p;
    pdrMgr->stats.nConfl += c;
  }

  if (reuseFrames) {
    Ddi_Mgr_t *ddm = pdrMgr->ddiPdr->ddiMgr;
    Ddi_Bddarray_t *rings = NULL;
    Ddi_Bdd_t *tfAig, *tfAig1;
#if 1
    Ddi_Var_t *cv = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$NS");
    Ddi_Var_t *pv = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$NS");
#endif
    
    if (reuseFrames > 1) {
      Ddi_Bdd_t *myZero = Ddi_BddMakeConstAig(ddm, 0);

      rings = Ddi_BddarrayAlloc(ddm, pdrMgr->nTimeFrames);
      Ddi_BddarrayWrite(rings, 0, myZero);
      Ddi_Free(myZero);
    }

    pdrMgr->travMgr->settings.ints.nPdrFrames = pdrMgr->nTimeFrames;
    pdrMgr->travMgr->settings.ints.fixPointTimeFrame = pdrMgr->fixPointTimeFrame;
    pdrMgr->travMgr->settings.ints.timeFrameClauses =
      Pdtutil_Alloc(Ddi_ClauseArray_t *, pdrMgr->nTimeFrames);
    pdrMgr->travMgr->settings.ints.timeFrameClauses[0] = NULL;
    for (i = 1; i < pdrMgr->nTimeFrames; i++) {
      tf = &(pdrMgr->timeFrames[i]);
      // printf("TF[%d] -> GBL\n", i);
      // Ddi_PdrPrintClauseArray(pdrMgr->ddiPdr,tf->clauses);

      pdrMgr->travMgr->settings.ints.timeFrameClauses[i] =
        Ddi_ClauseArrayToGblVarIds(pdrMgr->ddiPdr, tf->clauses);

      if (reuseFrames > 1) {
        tfAig = Ddi_BddMakeFromClauseArrayWithVars(tf->clauses,
          pdrMgr->ddiPdr->ns);
        Ddi_BddSetAig(tfAig);
#if 1
	Ddi_BddCofactorAcc(tfAig,pv,1);
	Ddi_BddCofactorAcc(tfAig,cv,1);
#endif
        Ddi_BddarrayWrite(rings, i, tfAig);
        Ddi_Free(tfAig);
      }
    }

    if (reuseFrames > 1) {
      Ddi_Bdd_t *rOut = NULL;
      for (i = pdrMgr->nTimeFrames - 2; i > 0; i--) {
	tfAig1 = Ddi_BddarrayRead(rings, i + 1);
	tfAig = Ddi_BddarrayRead(rings, i);
	Ddi_BddAndAcc(tfAig, tfAig1);
      }
      if (pdrMgr->fixPointTimeFrame > 0) {
	int fp_i = pdrMgr->fixPointTimeFrame;
	rOut = Ddi_BddDup(Ddi_BddarrayRead(rings, fp_i));
      }
      else {
        int val = pdrMgr->nTimeFrames>0?0:1;
	rOut = Ddi_BddMakeConstAig(ddm, val);
	for (i = pdrMgr->nTimeFrames - 2; i > 0; i--) {
	  tfAig = Ddi_BddarrayRead(rings, i);
	  Ddi_BddOrAcc(rOut, tfAig);
	}
      }
      Ddi_BddSubstVarsAcc(rOut, pdrMgr->ddiPdr->ns, pdrMgr->ddiPdr->ps);
      Trav_MgrSetReached(pdrMgr->travMgr, rOut);
      Trav_MgrSetNewi(pdrMgr->travMgr, rings);
      Ddi_Free(rings);
      Ddi_Free(rOut);
    }

  }
  //Quit ddi manager
  Ddi_PdrMgrQuit(pdrMgr->ddiPdr);

  //TOASK
  //Quit global auxiliary solver
  //Ddi_SatSolverQuit(pdrMgr->auxSolver); TODO ATTIVARE
  //pdrMgr->stats.solverTime+=pdrMgr->auxSolver->cpuTime; TODO ATTIVARE

  //Clear target subsetting data structures
  if (pdrMgr->settings.targetSubset) {
    Ddi_SatSolverQuit(pdrMgr->targetEnumSolver);
    Ddi_ClauseFree(pdrMgr->actTargetTot);
    Ddi_ClauseFree(pdrMgr->targetCube);
  }
  //Clear mappings for cone size computing
  Pdtutil_Free(pdrMgr->tempAigMap);
  Pdtutil_Free(pdrMgr->conesSize);

  //Clear time frames
  for (i = -1; i < pdrMgr->nTimeFrames; i++) {
    if (i < 0) {
      if (pdrMgr->infTimeFrame == NULL)
        continue;
      tf = pdrMgr->infTimeFrame;
    } else {
      tf = &(pdrMgr->timeFrames[i]);

      //Clear blocking queues
      for (j = 0; j <= pdrMgr->queueMaxPrio; j++) {
        Ddi_ClauseArrayFree(tf->queue[j]);
      }
    }

    //Quit primary solver
    pdrMgr->stats.solverTime += tf->solver->cpuTime;
    tf->stats.solverTime += tf->solver->cpuTime;
    Ddi_SatSolverQuit(tf->solver);

    //Quit auxiliary solver
    if (tf->solver2 != NULL) {
      pdrMgr->stats.solverTime += tf->solver2->cpuTime;
      tf->stats.solverTime += tf->solver2->cpuTime;
      Ddi_SatSolverQuit(tf->solver2);
    }
    //Clear clauses sets
    Ddi_ClauseArrayFree(tf->clauses);
    Ddi_ClauseArrayFree(tf->itpClauses);

    //Free incremental TR loading helper structures
    Pdtutil_Free(tf->trClauseMap.aigClauseMap);
    Pdtutil_Free(tf->trClauseMap.latchLoaded);
    Pdtutil_Free(tf->trClauseMap.minDepth);
    if (tf->hasSolver2)
      Pdtutil_Free(tf->trClauseMap.latchLoaded2);
    if (tf->hasSolver2)
      Pdtutil_Free(tf->trClauseMap.aigClauseMap2);
  }

  //Print statistics
  if (pdrMgr->settings.verbosity > Pdtutil_VerbLevelNone_c) {
    printf("FINAL PDR STATE:\n");
    printf("satCalls: %d\n", pdrMgr->stats.nSatSolverCalls);
    printf("unsat/sat: %d\n", pdrMgr->stats.nUnsat,
      pdrMgr->stats.nSatSolverCalls - pdrMgr->stats.nUnsat);
    printf("queue pop: %d\n", pdrMgr->stats.nQueuePop);
    printf("max bwd depth: %d\n", pdrMgr->stats.maxBwdDepth);
    printf("generalize: %d\n", pdrMgr->stats.nGeneralize);
    printf("clause push: %d\n", pdrMgr->stats.nClausePush);
    printf("ternary sim: %d\n", pdrMgr->stats.nTernarySim);
    printf("ternary init: %d\n", pdrMgr->stats.nTernaryInit);
    printf("blocked: %d\n", pdrMgr->stats.nBlocked);
    printf("blocks: %d\n", pdrMgr->stats.nBlocks);
    printf("solver restarts: %d\n", pdrMgr->stats.solverRestart);
    printf("generalize time: %f\n", pdrMgr->stats.generalizeTime);
    printf("cexgen time: %f\n", pdrMgr->stats.cexgenTime);
    printf("cexgen time SPLIT: %f %f\n", (float)cexGenTime[0] / 1000.0,
      (float)cexGenTime[1] / 1000.0);
    printf("restart time: %f\n", pdrMgr->stats.restartTime);
    printf("solver dec/prop/confl: %lu/%lu/%lu\n",
      pdrMgr->stats.nDec, pdrMgr->stats.nProp, pdrMgr->stats.nConfl);
    printf("solver split decisions:");
    for (i = 0; i < TRAV_PDR_MAX_SPLIT_STAT; i++) {
      printf(" [%d:%d]", i, pdrMgr->stats.split[i]);
    }
    printf("\n");
    printf("solver time: %f\n", pdrMgr->stats.solverTime);
    printf("solver time by frames: %f\n", pdrMgr->stats.solverTime);
    for (i = 0; i < pdrMgr->nTimeFrames; i++) {
      printf(" %f", pdrMgr->timeFrames[i].stats.solverTime);
    }
    if (pdrMgr->infTimeFrame != NULL) {
      printf(" %f", pdrMgr->infTimeFrame->stats.solverTime);
    }
    printf("\n");
  }
  //Quit ternary simulation manager
  Ddi_TernarySimQuit(pdrMgr->xMgr);

  //Free memory
  Pdtutil_Free(pdrMgr->timeFrames);
  if (pdrMgr->infTimeFrame != NULL) {
    Pdtutil_Free(pdrMgr->infTimeFrame);
  }
  if (pdrMgr->eqCl != NULL) {
    Pdtutil_EqClassesFree(pdrMgr->eqCl);
  }
  Pdtutil_Free(pdrMgr);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Trav_TravPdrVerif(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  int maxIter,
  int timeLimit
)
{
  //Status variables
  int fail;
  int k, done;
  int targetSubset;
  int num_target_cubes = 0;
  int targetCubeChanged = 0;

  //Local settings variables
  // GpC: disabled as produced wrong CEX e.g. oski15a10b03s
  // only with target unfolded
  // target immediately hit on initstub
  int extendTargetCex = 0; 

  int useTfInf = 0;

  //Utility variables
  long initTime;
  unsigned long time_limit = ~0;
  int dec;
  int specCall = 0;
  int i, abort = 0;
  int forceSuspend = 0;
  int nSuspensions = 0;

  //Prepare system structures
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Bdd_t *bad = Ddi_BddNot(invarspec);
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bdd_t *returnedCex = NULL;
  TravPdrMgr_t *pdrMgr = NULL;

  //Initialize variables for IC3 suspension/resume
  //printf("Available frames: %d\n", travMgr->settings.ints.nPdrFrames);
  int reuseFrames = (travMgr->settings.ints.nPdrFrames > 0)
		     && (travMgr->settings.aig.pdrReuseRings>1);
  int suspendFrame = travMgr->settings.ints.suspendFrame;
  int suspended = 0, resumed = 0;

  //Prepare initial time and time limit
  initTime = util_cpu_time();
  if (timeLimit <= 0) {
    timeLimit = travMgr->settings.aig.pdrTimeLimit;
  }
  if (timeLimit > 0) {
    time_limit = initTime + timeLimit * 1000;
    travMgr->settings.aig.pdrTimeLimit = time_limit;
  }

  //Initialize or resume PDR search
  if (travMgr->settings.ints.pdrMgr==NULL) {

    //Check if initial states intersect bad states
    Pdtutil_LogMsgMgr(ddm, 3, "PDR verif\n");
    if (initStub != NULL) {
      ps = Fsm_MgrReadVarPS(fsmMgr);
      Ddi_BddComposeAcc(bad, ps, initStub);
      fail = Ddi_AigSat(bad);
    } else {
      fail = Ddi_AigSatAnd(init, bad, NULL);
    }
    if (fail)
      goto pdrEnd;

    //Initialize PDR manager for verification
    pdrMgr = travMgr->settings.ints.pdrMgr =
      Trav_PdrMgrInit(travMgr, fsmMgr, init, initStub,
		      invarspec, invar, care, 10);
    if (pdrMgr->stats.result >= 0) {
      // verification done
      fail = pdrMgr->stats.result;
      goto pdrEnd;
    }
  }
  else {
    resumed = 1;
    pdrMgr = travMgr->settings.ints.pdrMgr;
  }

  //Get search settings
  useTfInf = pdrMgr->settings.infTf;
  targetSubset = pdrMgr->settings.targetSubset;


  //Start IC3 from the first time frame
  if (!resumed) {

    //Acquire target subset (if enabled)
    if (targetSubset) {
      fprintf(stdout, "ENUMERATING CUBE\n");
      num_target_cubes++;
      int start = pdrEnumerateTargetCubes(pdrMgr);

      fprintf(stdout, "Enumerated cube (%d, %d)\n", pdrMgr->targetCube->nLits,
	      pdrMgr->ddiPdr->nState);
      Pdtutil_Assert(start, "Unsat target");
    }

    //Add initial time frame and optionally inifinite time frame
    pdrTimeFrameAdd(pdrMgr, 0, 0);
    if (useTfInf) {
      pdrTimeFrameAdd(pdrMgr, -1, 0);
    }

    //Reload previously computed frames
    pdrReuseTimeFrames(pdrMgr);

    //Initialize time frame cursor to the first time frame
    k = done = 0;
    if (pdrMgr->settings.verbosity > Pdtutil_VerbLevelNone_c) {
      fprintf(stdout, "\n");
    }
  }

  //Resume search
  else {

    //Initialize time frame cursor to the last time frame
    k = travMgr->settings.ints.nPdrFrames;
    done = 0;
    Pdtutil_Assert(k>0,"no frames in resumed PDR");

    Pdtutil_VerbosityMgrIf(pdrMgr->travMgr, Pdtutil_VerbLevelUsrMax_c) {
      fprintf(tMgrO(pdrMgr->travMgr), "resuming PDR at bound: %d\n\n", k);
    }

  }

  //IC3 verification loop
  while (!done) {
    int sat = 0;

    //Check if verfication limits have been reached
    if (pdrOutOfLimits(pdrMgr)) {
      fail = -1;
      goto pdrEnd;
    }

    //Check if the invariants are sufficient to prove the property. The infinite time frame is initialized just with the target and the invariants,
    //if the conjunction between invariants and target is unsat then the property is proved.
    if (useTfInf) {
      dec = pdrMgr->infTimeFrame->solver->S->stats.decisions;
      sat =
        Ddi_SatSolve(pdrMgr->infTimeFrame->solver, pdrMgr->ddiPdr->actTarget,
        -1);
      pdrMgr->stats.split[0] +=
        (pdrMgr->infTimeFrame->solver->S->stats.decisions - dec);
      pdrMgr->stats.nUnsat += !sat;
      if (!sat) {
        done = 1;
        fail = 0;
        Pdtutil_VerbosityMgr(pdrMgr, Pdtutil_VerbLevelNone_c, fprintf(stdout,
            "fix point on INF time frame\n"));
        goto pdrEnd;
      }
    }

    //If previously computed time frames are available, skip to the first that intersect the target
    if (reuseFrames && !resumed) {
      sat = pdrTimeFrameCheckTarget(pdrMgr, k, &specCall);
      while (!sat && k < pdrMgr->nTimeFrames - 1) {
        k++;
        sat = pdrTimeFrameCheckTarget(pdrMgr, k, &specCall);
      }
      if (!sat) {
         fail = 0;
         goto pdrEnd;
       }
      Pdtutil_Assert(sat, "");
      fprintf(stdout, "First target intersection on reused TF %d\n", k);
      reuseFrames = 0;
    }

    //If target subsetting is enabled and a new target subset has been selected
    else if (targetSubset && targetCubeChanged) {
      targetCubeChanged = 0;
      sat = pdrTimeFrameCheckTargetTot(pdrMgr, k, &specCall);
      // while(!sat && k < pdrMgr->nTimeFrames - 1){
      //   k++;
      //   sat = pdrTimeFrameCheckTarget(pdrMgr,k, &specCall);
      // }
      Pdtutil_Assert(sat, "");
      fprintf(stdout, "First target intersection on TF %d\n", k);
    }

    //Check if the current time frame instersect the target
    else {
      sat = pdrTimeFrameCheckTarget(pdrMgr, k, &specCall);
    }

    //Block bad cube
    if (sat) {

      //Get the bad cube (minterm) found
      Ddi_Clause_t *cex0;
      specCall = isSpecializedCall(pdrMgr, PDR_TARGET_INTERSECTION_CALL, k);
      if (specCall) {
        cex0 =
          Ddi_SatSolverGetCexCube(pdrMgr->timeFrames[k].solver2,
          pdrMgr->ddiPdr->targetSuppClause);
      } else {
        cex0 =
          Ddi_SatSolverGetCexCube(pdrMgr->timeFrames[k].solver,
          pdrMgr->ddiPdr->targetSuppClause);
      }
      if (0) {
        Ddi_Clause_t *cex1 = Ddi_ClauseDup(cex0);
        Ddi_ClauseJoin(cex1, pdrMgr->actTargetTot);
        Pdtutil_Assert(Ddi_SatSolve(pdrMgr->timeFrames[k].solver, cex1, -1),
          "wrong target cube");
        Ddi_ClauseFree(cex1);
      }

      //Extend bad cube (possibly) with ternary simulation
      Ddi_Clause_t *cex;
      if (extendTargetCex) {
        cex =
          pdrExtendCex(pdrMgr,
          specCall ? pdrMgr->timeFrames[k].solver2 : pdrMgr->
          timeFrames[k].solver, cex0, NULL, 2 /*enTernary */ );
      } else {
        cex = Ddi_ClauseDup(cex0);
        Ddi_ClauseFree(cex0);
      }
      PDR_DBG(printf("PROPERTY CEX\n"));
      PDR_DBG(Ddi_PdrPrintClause(pdrMgr->ddiPdr, cex));
      cex->actLit = 0;          //Set initial backward depth to 0

      //Block cube
      returnedCex = pdrBlockCube(pdrMgr, cex, k, &abort, &forceSuspend);
      if (returnedCex != NULL) {
	if (!pdrMgr->settings.unfoldProp) {
	  int np = Ddi_BddPartNum(returnedCex);
	  if (np>1) {
	    Ddi_BddPartRemove(returnedCex,np-1);
	  }
	}
        done = 1;
        fail = 1;
      } else if (abort) {
        done = 1;
        fail = -1;
        if (pdrMgr->travMgr->settings.ints.suspendFrame > 0) {
	  fail = -2;
	  suspended = 1;
	  if (k>=travMgr->settings.ints.nPdrFrames) {
	    travMgr->settings.ints.nPdrFrames = k;
	  }
	}
      } else if(forceSuspend){
        pdrMgr->travMgr->settings.ints.suspendFrame = k;
	pdrMgr->travMgr->settings.aig.pdrMaxBlock *= 1.5;
        nSuspensions++;
	pdrTimeFramePushClauses(pdrMgr, k);
        done = 1;
        fail = -2;
      	suspended = 1;
      	travMgr->settings.ints.nPdrFrames = k;
      }
    }

    //Time frame is safe
    else {

      //Outmost time frame doesn`t containt bad states: assert Fk --> P (timeframe is safe)
      int fixPoint;
      if (k > 0) {
        pdrTimeFrameAssertInvarspec(pdrMgr, k);
      }

      //Create new time frame
      k++;
      pdrTimeFrameAdd(pdrMgr, k, 1);

      //Output new time frame information
#if EN_PDR_DBG
      for (i = 0; i <= k; i++) {
        PDR_DBG(printf("level %d (before push)\n", i));
        PDR_DBG(Ddi_PdrPrintClauseArray(pdrMgr->ddiPdr,
            pdrMgr->timeFrames[i].clauses));
        printf("level %d (before push)\n", i);
        Ddi_PdrPrintClauseArray(pdrMgr->ddiPdr, pdrMgr->timeFrames[i].clauses);
      }
#endif
      Pdtutil_VerbosityMgr(pdrMgr, Pdtutil_VerbLevelUsrMin_c,
        fprintf(stdout, "-TF %2d: ", k);
        for (i = 0; i <= k; i++) {
        fprintf(stdout, " %d",
            Ddi_ClauseArrayNum(pdrMgr->timeFrames[i].clauses));}
        if (pdrMgr->infTimeFrame != NULL) {
        fprintf(stdout, " <%d>",
            Ddi_ClauseArrayNum(pdrMgr->infTimeFrame->clauses));}
        fprintf(stdout, " (tot: %d=%d-%d)\n",
          pdrMgr->nClauses - pdrMgr->nRemoved, pdrMgr->nClauses,
          pdrMgr->nRemoved);) ;

      //Push clauses forward
      fixPoint = pdrTimeFramePushClauses(pdrMgr, k);

      //Log time frame statistics
      Trav_LogStats(travMgr, "TOTCL", pdrMgr->nClauses - pdrMgr->nRemoved, 1);
      Trav_LogStats(travMgr, "RESTART", pdrMgr->stats.solverRestart, 0);
      Trav_LogStats(travMgr, "TOTCONFL", pdrMgr->stats.nConfl, 0);


      long currTime = util_cpu_time() - initTime;

      //Output time frame information after push
      Pdtutil_VerbosityMgrIf(pdrMgr->travMgr, Pdtutil_VerbLevelUsrMin_c) {
        fprintf(stdout, "+TF %2d: ", k);
        for (i = 0; i <= k; i++) {
	  fprintf(stdout, " %d",
            Ddi_ClauseArrayNum(pdrMgr->timeFrames[i].clauses));
	}
        if (pdrMgr->infTimeFrame != NULL) {
	  fprintf(stdout, " <%d>",
		  Ddi_ClauseArrayNum(pdrMgr->infTimeFrame->clauses));
	}
        fprintf(stdout, " (tot: %d=%d-%d) - ",
		pdrMgr->nClauses - pdrMgr->nRemoved, pdrMgr->nClauses,
		pdrMgr->nRemoved);
	fprintf(stdout,
		"time: %s\n", util_print_time(currTime));
      }

      //Fsm_MgrLogUnsatBound(fsmMgr, k, pdrMgr->settings.unfoldProp);
#if EN_PDR_DBG
      for (i = 0; i <= k; i++) {
        PDR_DBG(printf("level %d\n", i));
        PDR_DBG(Ddi_PdrPrintClauseArray(pdrMgr->ddiPdr,
            pdrMgr->timeFrames[i].clauses));
        if (pdrMgr->timeFrames[i].clauses->nClauses < 2) {
          printf("level %d\n", i);
          Ddi_PdrPrintClauseArray(pdrMgr->ddiPdr,
            pdrMgr->timeFrames[i].clauses);
        }
      }
#endif

      //Fixpoint not reached: share time frame clauses with external engines
      if (!fixPoint) {
        PDR_DBG(pdrTimeFrameCheckClauses(pdrMgr));
        pdrTimeFrameShareClauses(travMgr, pdrMgr, k - 1);
      }

      //Fixpoint reached: an inductive strenghtening for the property is found
      if (fixPoint) {

        //If target subsetting is enabled enumerate the next target cube
        if (targetSubset) {
          int start = pdrEnumerateTargetCubes(pdrMgr);

          //If whole target has been covered the property is proved
          if (!start) {
            done = 1;
            fail = 0;
          } else {
            k = start;
            targetCubeChanged = 1;
            num_target_cubes++;
            fprintf(stdout, "Enumerated cube (%d, %d)\n",
              pdrMgr->targetCube->nLits, pdrMgr->ddiPdr->nState);
          }
        } else {
          done = 1;
          fail = 0;
        }
      }

      //IC3 verification suspended
      else if (pdrMgr->travMgr->settings.ints.suspendFrame > 0 &&
        pdrMgr->travMgr->settings.ints.suspendFrame<=k) {
      	done = 1;
      	fail = -2;
      	suspended = 1;
      	travMgr->settings.ints.nPdrFrames = k;
      }
    }
  }

  //End of verification procedure
pdrEnd:

  //Log verification result
  if (fail >= 0) {
    Pdtutil_LogMsgMgr(ddm, 3, "PDR verif done - res: %d\n", fail);
  } else if (fail < -1) {
    // just stop
    Pdtutil_LogMsgMgr(ddm, 3, "PDR verif stopped\n");
  } else {
    Pdtutil_LogMsgMgr(ddm, 3, "PDR verif aborted due to reached time limit\n");
    pdrTimeFrameAdd(pdrMgr, pdrMgr->nTimeFrames, 1);
  }

  //Set verification result in TRAV manager
  Trav_MgrSetAssertFlag(travMgr, fail);

  //Free bad states BDD
  Ddi_Free(bad);

  //Clean manager after verification has been done
  if (!suspended) {

    //Quit PDR Manager
    if (pdrMgr != NULL) {
      Trav_PdrMgrQuit(pdrMgr);
      travMgr->settings.ints.pdrMgr = NULL;
    }

    printf("Number of forced suspensions: %d\n", nSuspensions);

    //Return verification result
    if (returnedCex != NULL) {
      if (fail==1) {
	Fsm_MgrSetCexBDD(fsmMgr, returnedCex);
	int res = 1;                // Fbv_CheckFsmCex (fsmMgr,travMgr);
	
	Pdtutil_Assert(res, "cex validation failure");
	Trav_MgrSetAssertFlag(travMgr, 1);
      }
      Ddi_Free(returnedCex);
    }
  }

  return Fsm_MgrReadCexBDD(fsmMgr);

}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************
  Synopsis    [ Adds a k-th time frame to PDR manager. ]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
pdrTimeFrameAdd(
  TravPdrMgr_t * pdrMgr,
  int k,
  int reuseEnabled
)
{
  int i;
  int nState = pdrMgr->ddiPdr->nState;
  TravPdrTimeFrame_t *tf;

  Pdtutil_Assert(reuseEnabled || k < 0 || pdrMgr->settings.targetSubset
    || pdrMgr->nTimeFrames == k, "wrong num of time frames in pdr mgr");

  //Skip if timeframe already present
  if ( /*pdrMgr->settings.targetSubset && */ k < pdrMgr->nTimeFrames) {

    //Clean  and reload solvers
    tf = &(pdrMgr->timeFrames[k]);

    Ddi_SatSolverQuit(tf->solver);
    Ddi_SatSolver_t *solver = Ddi_SatSolverAlloc();
    int incrementalLoad = pdrMgr->settings.incrementalTrClauses && k != 0;

    DdiSatSolverSetFreeVarMinId(solver, pdrMgr->ddiPdr->freeCnfVar);
    pdrTimeFrameLoadSolver(pdrMgr, solver, k, incrementalLoad, 0);
    tf->solver = solver;

    if (tf->hasSolver2) {
      Ddi_SatSolverQuit(tf->solver2);
      Ddi_SatSolver_t *solver2 = Ddi_SatSolverAlloc();

      incrementalLoad = pdrMgr->settings.incrementalTrClauses && k != 0;
      DdiSatSolverSetFreeVarMinId(solver2, pdrMgr->ddiPdr->freeCnfVar);
      pdrTimeFrameLoadSolver(pdrMgr, solver2, k, incrementalLoad, 1);
      tf->solver2 = solver2;
    }
    return;
  }
  //Allocate time frame
  if (k == 0) {
    /* First time frame */
    pdrMgr->maxTimeFrames = 4;
    pdrMgr->timeFrames =
      Pdtutil_Alloc(TravPdrTimeFrame_t, pdrMgr->maxTimeFrames);
  } else if (k < 0) {
    /* Infinite time frame */
    pdrMgr->infTimeFrame = Pdtutil_Alloc(TravPdrTimeFrame_t, 1);
  } else if (k >= pdrMgr->maxTimeFrames) {
    pdrMgr->maxTimeFrames *= 2;
    pdrMgr->timeFrames =
      Pdtutil_Realloc(TravPdrTimeFrame_t, pdrMgr->timeFrames,
      pdrMgr->maxTimeFrames);
  }
  //Get allocated time frame (and allocate priority queues if its not infinite)
  if (k < 0) {
    tf = pdrMgr->infTimeFrame;
  } else {
    pdrMgr->nTimeFrames++;
    tf = &(pdrMgr->timeFrames[k]);
    for (i = 0; i <= pdrMgr->queueMaxPrio; i++) {
      tf->queue[i] = Ddi_ClauseArrayAlloc(0);
    }
  }

  //Allocate primary solver
  tf->solver = Ddi_SatSolverAlloc();
  DdiSatSolverSetFreeVarMinId(tf->solver, pdrMgr->ddiPdr->freeCnfVar);
  tf->solver->S->pdt_opt_neg_first = false;

  //Allocate auxiliary solver (only for non-initial time frames)
  int useSolver2 = k > 0 && pdrMgr->settings.useSolver2;

  if (useSolver2) {
    tf->solver2 = Ddi_SatSolverAlloc();
    DdiSatSolverSetFreeVarMinId(tf->solver2, pdrMgr->ddiPdr->freeCnfVar);
    tf->solver2->S->pdt_opt_neg_first = false;
  } else
    tf->solver2 = NULL;
  tf->hasSolver2 = useSolver2;

  //Initialize solvers restart helper structures
  for (i = 0; i < TRAV_PDR_RESTART_WINDOW_SIZE; i++) {
    tf->restartWindow[i] = 0.0;
    tf->restartWindow2[i] = 0.0;
  }
  tf->wndStart = 0;
  tf->wndStart2 = 0;
  tf->wndEnd = 0;
  tf->wndEnd2 = 0;
  tf->wndSum = 0.0;
  tf->wndSum2 = 0.0;

  //Initialize clauses sets
  tf->clauses = Ddi_ClauseArrayAlloc(0);
  tf->itpClauses = NULL;

  //Initialize latch equivalences
  tf->latchEquiv = NULL;

  //Initialize TR loading helper structures
  tf->trClauseMap.aigClauseMap = NULL;
  tf->trClauseMap.latchLoaded = NULL;
  tf->trClauseMap.aigClauseMap2 = NULL;
  tf->trClauseMap.latchLoaded2 = NULL;
  tf->trClauseMap.minDepth = NULL;
  tf->trClauseMap.minLevel = -1;

  //Initialize timeframe statistics
  tf->stats.numInductionClauses = 0;
  tf->stats.numLoadedTrClauses = 0;
  tf->stats.numInductionClauses2 = 0;
  tf->stats.numLoadedTrClauses2 = 0;

  //Load initial state clauses for initial time frame (and compute literals implied by initial states)
  //To do so probe satisfiability assuming every literals. When the initial states are SAT only for
  //one polarity, the corresponding lit is implied by initial states
  if (k == 0) {
    Ddi_SatSolverAddClauses(tf->solver, pdrMgr->ddiPdr->initClauses);
    if (!pdrMgr->ddiPdr->useInitStub) {
      pdrMgr->ddiPdr->initLits = Pdtutil_Alloc(int,
        nState
      );

      for (i = 0; i < nState; i++) {
        int sat0, sat1, psLit_i = pdrMgr->ddiPdr->psCnf[i];
        Ddi_Clause_t *cl0 = Ddi_ClauseAlloc(0, 1);
        Ddi_Clause_t *cl1 = Ddi_ClauseAlloc(0, 1);

        DdiClauseAddLiteral(cl0, -psLit_i);
        DdiClauseAddLiteral(cl1, psLit_i);
        sat0 = Ddi_SatSolve(tf->solver, cl0, -1);
        pdrMgr->stats.nSatSolverCalls++;
        pdrMgr->stats.nUnsat += !sat0;
        sat1 = Ddi_SatSolve(tf->solver, cl1, -1);
        pdrMgr->stats.nUnsat += !sat1;
        Ddi_ClauseFree(cl0);
        Ddi_ClauseFree(cl1);
        Pdtutil_Assert(sat0 || sat1, "void init state");
        if (!sat0 && sat1) {
          /* 1 lit */
          pdrMgr->ddiPdr->initLits[i] = 1;
        } else if (sat0 && !sat1) {
          /* 0 lit */
          pdrMgr->ddiPdr->initLits[i] = 0;
        } else {
          /* - lit */
          pdrMgr->ddiPdr->initLits[i] = 2;
        }
      }
    }
  }
  //Load target & invariant clauses
  Ddi_SatSolverAddClauses(tf->solver, pdrMgr->ddiPdr->invarClauses);
  if (pdrMgr->targetCube != NULL) {

    //Ddi_SatSolverAddClauses(tf->solver,pdrMgr->target_subset);
    Ddi_SatSolverAddClauses(tf->solver, pdrMgr->ddiPdr->targetClauses);
  } else {
    Ddi_SatSolverAddClauses(tf->solver, pdrMgr->ddiPdr->targetClauses);
  }

  if (tf->hasSolver2) {
    Ddi_SatSolverAddClauses(tf->solver2, pdrMgr->ddiPdr->invarClauses);
    if (pdrMgr->targetCube != NULL) {

      //Ddi_SatSolverAddClauses(tf->solver2,pdrMgr->target_subset);
      Ddi_SatSolverAddClauses(tf->solver2, pdrMgr->ddiPdr->targetClauses);
    } else {
      Ddi_SatSolverAddClauses(tf->solver2, pdrMgr->ddiPdr->targetClauses);
    }
  }
  //Load transition relation
  if (pdrMgr->settings.incrementalTrClauses && k != 0) {

    //Incremental load: set up loading information structures
    Ddi_NetlistClausesInfo_t *netCl = &(pdrMgr->ddiPdr->trNetlist);

    Pdtutil_Assert(netCl->nRoot > 0, "wrong clause map info");
    Pdtutil_Assert(nState == netCl->nRoot, "error in clause map info");
    tf->trClauseMap.nGate = netCl->nGate;
    tf->trClauseMap.nLatch = netCl->nRoot;
    tf->trClauseMap.aigClauseMap = Pdtutil_Alloc(int,
      netCl->nGate
    );
    tf->trClauseMap.latchLoaded = Pdtutil_Alloc(int,
      netCl->nRoot
    );
    tf->trClauseMap.minDepth = Pdtutil_Alloc(int,
      netCl->nGate
    );

    if (tf->hasSolver2) {
      tf->trClauseMap.latchLoaded2 = Pdtutil_Alloc(int,
        netCl->nRoot
      );
      tf->trClauseMap.aigClauseMap2 = Pdtutil_Alloc(int,
        netCl->nGate
      );;
    }
    for (i = 0; i < nState; i++) {
      tf->trClauseMap.latchLoaded[i] = PDR_NOT_LOADED;
      pdrMgr->conesSize[i] = PDR_NOT_COUNTED;   //Clean cone size counters for the new time frame
      if (tf->hasSolver2)
        tf->trClauseMap.latchLoaded2[i] = PDR_NOT_LOADED;
    }
    for (i = 0; i < netCl->nGate; i++) {
      tf->trClauseMap.aigClauseMap[i] = PDR_NOT_LOADED;
      pdrMgr->tempAigMap[i] = PDR_NOT_COUNTED;  //Clean cone size counters for the new time frame
      tf->trClauseMap.minDepth[i] = -1;
      if (tf->hasSolver2)
        tf->trClauseMap.aigClauseMap2[i] = PDR_NOT_LOADED;
    }
  } else if (k == 0) {
    //Load TR cone of initial states variables in initial time frame
    Ddi_SatSolverAddClauses(tf->solver, pdrMgr->ddiPdr->trClausesInit);
  } else {
    //Non-incremental loading (load TR monolithically)
    Ddi_SatSolverAddClauses(tf->solver, pdrMgr->ddiPdr->trClauses);
    if (tf->hasSolver2)
      Ddi_SatSolverAddClauses(tf->solver2, pdrMgr->ddiPdr->trClauses);
  }

  //Generate equivalence classes
  if (pdrMgr->settings.fwdEquivalences && k > 0) {
    timeFrameUpdateEqClasses(pdrMgr, k);
  }

  timeFrameUpdateRplusClasses(pdrMgr, k);

  PDR_DBG(pdrCheckSolvers(pdrMgr));

}


/**Function*******************************************************************
  Synopsis    [ Adds a k-th time frame to PDR manager. ]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
pdrReuseTimeFrames(
  TravPdrMgr_t * pdrMgr
)
{
  int i;
  Trav_Mgr_t *travMgr = pdrMgr->travMgr;
  Ddi_ClauseArray_t **frameClauses = travMgr->settings.ints.timeFrameClauses;
  int nFrames = travMgr->settings.ints.nPdrFrames;
  TravPdrTimeFrame_t *tf;

  if (nFrames <= 0 || pdrMgr->travMgr->settings.aig.pdrReuseRings<2) {
    travMgr->settings.ints.nPdrFrames = 0;
    return;
  }

  Pdtutil_Assert(pdrMgr->nTimeFrames == 1,
    "wrong num of time frames in pdr mgr");

  Ddi_PdrRemapVarMark(pdrMgr->ddiPdr);
  for (i = 1; i < nFrames; i++) {
    pdrTimeFrameAdd(pdrMgr, i, 0);
    tf = &(pdrMgr->timeFrames[i]);
    tf->clauses =
      Ddi_ClauseArrayFromGblVarIds(pdrMgr->ddiPdr, frameClauses[i]);
    // printf("TF[%d] <- GBL\n", i);
    // Ddi_PdrPrintClauseArray(pdrMgr->ddiPdr,tf->clauses);
  }
  Ddi_PdrRemapVarUnmark(pdrMgr->ddiPdr);
  for (i = 1; i < nFrames; i++) {
    pdrTimeFrameAdd(pdrMgr, i, 1);
  }

}


/**Function*******************************************************************
  Synopsis    [Checks if the outmost time frame intersect the target]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
pdrTimeFrameCheckTarget(
  TravPdrMgr_t * pdrMgr,
  int k,
  int *specCall
)
{
  TravPdrTimeFrame_t *tf = &(pdrMgr->timeFrames[k]);
  int sat, dec;

  //Target intersection call on outmost time frame (Q1)
  *specCall = isSpecializedCall(pdrMgr, PDR_TARGET_INTERSECTION_CALL, k);
  if (*specCall) {
    dec = pdrMgr->timeFrames[k].solver2->S->stats.decisions;
    sat =
      Ddi_SatSolve(pdrMgr->timeFrames[k].solver2, pdrMgr->ddiPdr->actTarget,
      -1);
  } else {
    dec = pdrMgr->timeFrames[k].solver->S->stats.decisions;
    sat =
      Ddi_SatSolve(pdrMgr->timeFrames[k].solver, pdrMgr->ddiPdr->actTarget,
      -1);
  }
  pdrMgr->stats.split[1] +=
    (pdrMgr->timeFrames[k].solver->S->stats.decisions - dec);
  pdrMgr->stats.nUnsat += !sat;

  return sat;
}

/**Function*******************************************************************
  Synopsis    [Checks if the outmost time frame intersect the target]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
pdrTimeFrameCheckTargetTot(
  TravPdrMgr_t * pdrMgr,
  int k,
  int *specCall
)
{
  TravPdrTimeFrame_t *tf = &(pdrMgr->timeFrames[k]);
  int sat, dec;

  //Target intersection call on outmost time frame (Q1)
  *specCall = isSpecializedCall(pdrMgr, PDR_TARGET_INTERSECTION_CALL, k);
  if (*specCall) {
    dec = pdrMgr->timeFrames[k].solver2->S->stats.decisions;
    sat =
      Ddi_SatSolve(pdrMgr->timeFrames[k].solver2, pdrMgr->actTargetTot, -1);
  } else {
    dec = pdrMgr->timeFrames[k].solver->S->stats.decisions;
    sat = Ddi_SatSolve(pdrMgr->timeFrames[k].solver, pdrMgr->actTargetTot, -1);
  }
  pdrMgr->stats.split[1] +=
    (pdrMgr->timeFrames[k].solver->S->stats.decisions - dec);
  pdrMgr->stats.nUnsat += !sat;

  return sat;
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
pdrTimeFrameAssertInvarspec(
  TravPdrMgr_t * pdrMgr,
  int k
)
{
  TravPdrTimeFrame_t *tf = &(pdrMgr->timeFrames[k]);

  //Generate clause that assert Fk-->P
  Ddi_Clause_t *actInvarspec = Ddi_ClauseNegLits(pdrMgr->ddiPdr->actTarget);

  //Add asserting clause to solver (now Fk --> P)
  Ddi_SatSolverAddClause(tf->solver, actInvarspec);
  Ddi_SatSolverSimplify(tf->solver);
  if (tf->hasSolver2) {
    Ddi_SatSolverAddClause(tf->solver2, actInvarspec);
    Ddi_SatSolverSimplify(tf->solver2);
  }
  //Free asserting clause
  Ddi_ClauseFree(actInvarspec);
}

/**Function*******************************************************************
  Synopsis    [Clean the current proof-obligations queue.]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void pdrCleanCubeQueue(TravPdrMgr_t * pdrMgr){
  int k, prio;
  Ddi_Clause_t * cube;
  while ((cube = pdrQueuePopCube(pdrMgr, &k, &prio)) != NULL) {
    Ddi_ClauseDeref(cube);
  }
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
pdrBlockCube
(
  TravPdrMgr_t * pdrMgr,
  Ddi_Clause_t * cube,
  int myK,
  int *pAbort,
  int *pForceSuspend
)
{
  //Utility variables
  int i, k, prio, kMax = pdrMgr->nTimeFrames - 1;
  int dec;
  int specCall;
  int kMax0;
  Ddi_Clause_t *pred, *clMin;
  int blockCounter = 0;
  int blockLimit = pdrMgr->travMgr->settings.aig.pdrMaxBlock;

  //TOASK
  int prio0 = 0;                //pdrMgr->settings.unfoldProp ? TRAV_PDR_QUEUE_MAXPRIO : 0;

  //Setting variables
  int genEffort = pdrMgr->settings.genEffort;   //Effort in inductive generalization
  int doInductivePush = 1;      //Try to push found inductive lemmas forward in timeframes until induction is kept
  int checkCexReachability = 1; //Check counterexample reachability
  int logCex = 0;

  if (myK < 0)
    myK = kMax;
  //Push bad cube into blocking cube at latest priority
  pdrQueuePushCube(pdrMgr, cube, myK, prio0);
  pdrMgr->stats.nBlocked++;

  //Blocking loop: extract obligation proof from queue (cube + time frame index)
  while ((cube = pdrQueuePopCube(pdrMgr, &k, &prio)) != NULL) {
    TravPdrTimeFrame_t *tf = &(pdrMgr->timeFrames[k]);

    if((blockLimit>0) && (blockCounter >= blockLimit)){
      pdrCleanCubeQueue(pdrMgr);
      *pForceSuspend = 1;
      Pdtutil_VerbosityMgrIf(pdrMgr->travMgr, Pdtutil_VerbLevelUsrMax_c) {
	fprintf(tMgrO(pdrMgr->travMgr), 
	    "PDR suspended for for max block limit: %d\n", blockCounter);
      }
      return NULL;
    }

    if (pdrOutOfLimits(pdrMgr)) {
      if (pAbort != NULL) {
        *pAbort = 1;
        return NULL;
      }
    }
    //Updated blocking statistics
    if (cube->actLit > pdrMgr->stats.maxBwdDepth) {
      pdrMgr->stats.maxBwdDepth = cube->actLit;
    }
    pdrMgr->settings.step++;
    blockCounter++;
    PDR_DBG(pdrCheckSolvers(pdrMgr));
    PDR_DBG(printf("DEQUEUED CUBE (%d) c:%d\n", k, counter));
    PDR_DBG(Ddi_PdrPrintClause(pdrMgr->ddiPdr, cube));

    //Check time frame index
    if (k == 0) {

      //Bad cube must be blocked in initial time frame (counterexample found) --> Check CEX reachability
      //Chains of predecessors cube to be blocked, currenty in the blocking queue, are linked toghether
      //using the auxPtr field. To check if the counterexample is correct we traverse this chain of cubes
      //starting form the cube queued on the initial time frame, checking mutual reachability at every step.
      //It also computes the counterexample to return.
      Ddi_Mgr_t *ddm = pdrMgr->ddiPdr->ddiMgr;
      Ddi_Clause_t *cube0, *cube0Tot;
      Ddi_Bdd_t *returnedCex = Ddi_BddMakePartConjVoid(ddm);

      if (pdrMgr->ddiPdr->useInitStub) {
        Ddi_Bdd_t *cubeIsAig =
          Ddi_SatSolverGetCexCubeAig(pdrMgr->timeFrames[0].solver,
          pdrMgr->ddiPdr->isClause, pdrMgr->ddiPdr->isVars);

        Ddi_BddPartInsertLast(returnedCex, cubeIsAig);
	Ddi_Free(cubeIsAig);
      }

      Pdtutil_VerbosityMgr(pdrMgr, Pdtutil_VerbLevelNone_c, fprintf(stdout,
          "CEX of failure\n"));
      cube0Tot = Ddi_ClauseDup(cube);   //Minterm to follow for checking reachability
      for (i = 0, cube0 = cube; checkCexReachability && cube0 != NULL; i++) {

        //Get the successor bad cube
        Ddi_Clause_t *cube1 = Ddi_ClauseReadLink(cube0);

        if (logCex)
          Ddi_PdrPrintClause(pdrMgr->ddiPdr, cube0);

        //Check if end of successors chain is reached
        if (cube1 != NULL) {
          Ddi_Clause_t *cubeTot;
          Ddi_Bdd_t *cubePiAig = NULL;

          //If end of successors chain is not reached yet, remap the successor cube to next state
          Ddi_Clause_t *cubeNs = Ddi_ClauseRemapVars(cube1, 1);

          if (i == 0) {
            //Check reachability for the first step (using initial time frame solver that maintains the initial state)
            PDR_DBG(Pdtutil_Assert(Ddi_SatSolve(pdrMgr->timeFrames[0].solver,
                  cube, -1), "unreachable INITIAL CEX"));
            PDR_DBG(Pdtutil_Assert(Ddi_SatSolve(pdrMgr->timeFrames[0].solver,
                  cubeNs, -1), "unreachable first step CEX"));
            cubeTot =
              Ddi_SatSolverGetCexCube(pdrMgr->timeFrames[0].solver,
              pdrMgr->ddiPdr->nsClause);
            cubePiAig =
              Ddi_SatSolverGetCexCubeAig(pdrMgr->timeFrames[0].solver,
              pdrMgr->ddiPdr->piClause, pdrMgr->ddiPdr->pi);
          } else {
            int sat;

            //Check reachability for the i-th step (using the auxiliary solver that maintains the transition relation)
            //Check if the conjunction between the current cube and its successor is sat toghether with TR (successor reachable)
            Ddi_Clause_t *joined = Ddi_ClauseJoin(cube0Tot, cubeNs);

            PDR_DBG(Pdtutil_Assert(joined != NULL, "error merging clauses"));
            sat = Ddi_SatSolve(pdrMgr->auxSolver, joined, -1);
            Pdtutil_Assert(sat, "unreachable CEX");
            Ddi_ClauseFree(joined);
            cubeTot =
              Ddi_SatSolverGetCexCube(pdrMgr->auxSolver,
              pdrMgr->ddiPdr->nsClause);
            cubePiAig =
              Ddi_SatSolverGetCexCubeAig(pdrMgr->auxSolver,
              pdrMgr->ddiPdr->piClause, pdrMgr->ddiPdr->pi);
          }
          Ddi_ClauseFree(cube0Tot);
          cube0Tot = Ddi_ClauseRemapVars(cubeTot, -1);
          if (logCex)
            Ddi_PdrPrintClause(pdrMgr->ddiPdr, cubeTot);

          //Update counterexample to return (list of assignments to input variables)
          Ddi_BddPartInsertLast(returnedCex, cubePiAig);
          Ddi_Free(cubePiAig);
          Ddi_ClauseFree(cubeTot);
          Ddi_ClauseFree(cubeNs);
        } else {
          Ddi_Bdd_t *cubePiAig = NULL;
          Ddi_Clause_t *tc = pdrMgr->actTargetTot;

          if (tc == NULL)
            tc = pdrMgr->ddiPdr->actTarget;
          Ddi_Clause_t *joined = Ddi_ClauseJoin(cube0Tot, tc);

          PDR_DBG(Pdtutil_Assert(joined != NULL, "error merging clauses"));
          Pdtutil_Assert(Ddi_SatSolve(pdrMgr->auxSolver, joined, -1),
            "unreachable CEX");
          Ddi_ClauseFree(joined);
          cubePiAig = Ddi_SatSolverGetCexCubeAig(pdrMgr->auxSolver,
            pdrMgr->ddiPdr->piClause, pdrMgr->ddiPdr->pi);
          Ddi_BddPartInsertLast(returnedCex, cubePiAig);
          Ddi_Free(cubePiAig);
        }
        cube0 = cube1;
      }

      Ddi_ClauseFree(cube0Tot);
      Ddi_ClauseFree(cube);

      //Return counterexample
      return returnedCex;
    }
    //Check if cube is already blocked in current time frame by subsumption (SYNTACTIC INCLUSION)
    //(a clause that negates the cube of a subset of it is already present in TF)
    Ddi_Clause_t *notCube = Ddi_ClauseNegLits(cube);

    if (pdrClauseIsContained(pdrMgr, notCube, k)) {
      PDR_DBG(Pdtutil_Assert(Ddi_ClauseIsBlocked(tf->solver, cube),
          "wrong subsumption"));
      Ddi_ClauseDeref(cube);
      Ddi_ClauseFree(notCube);
      PDR_DBG(printf("CONT 0\n"));
      nContained++;
      continue;
    }
    Ddi_ClauseFree(notCube);
    PDR_DBG1(Pdtutil_Assert(Ddi_SatSolverOkay(pdrMgr->timeFrames[k -
            1].solver), "UNSAT solver before block"));
    PDR_DBG1(Pdtutil_Assert(Ddi_SatSolverOkay(tf->solver),
        "UNSAT solver before block"));

    //Check if cube is already blocked in current time frame by satisfiability (Q2) (SEMATIC INCLUSION)
    int isBlocked = 0;

    specCall = isSpecializedCall(pdrMgr, PDR_IS_BLOCKED_CALL, k);
    if (specCall) {
      dec = tf->solver2->S->stats.decisions;
      isBlocked = Ddi_ClauseIsBlocked(tf->solver2, cube);
    } else {
      dec = tf->solver->S->stats.decisions;
      isBlocked = Ddi_ClauseIsBlocked(tf->solver, cube);
    }
    if (isBlocked) {
      PDR_DBG(printf("BLOCK 1\n"));
      Ddi_ClauseDeref(cube);
      pdrMgr->stats.nBlocks++;
      continue;
    }
    //Debug
    PDR_DBG1(Pdtutil_Assert(Ddi_SatSolverOkay(tf->solver),
        "UNSAT solver after block"));
    PDR_DBG1(Pdtutil_Assert(Ddi_SatSolveCustomized(tf->solver, cube, 0, 0, -1),
        "not blocked cube"));
    PDR_DBG(Pdtutil_Assert(Ddi_SatSolveCustomized(tf->solver, cube, 0, 0, -1),
        "not blocked cube"));
    PDR_DBG(pdrCheckSolvers(pdrMgr));
    Pdtutil_Assert(Ddi_SatSolverOkay(pdrMgr->timeFrames[k - 1].solver),
      "UNSAT solver");
    pdrMgr->settings.step++;

    //Check if the clause obtained by cube negation (extended) is inductive relative to previous time frame
    pred =
      pdrInductivePredecessor(pdrMgr, k - 1, cube, NULL, 2 /* en ternary */ ,
      1 /*reuse act vars */ , 1 /*use induction */ , PDR_BLOCKING_MODE);
    if (pred != NULL) {

      //Predecessor to bad cube found in Fk-1
      Ddi_ClauseRef(pred);
      Ddi_ClauseSetLink(pred, cube);
      pred->actLit = cube->actLit + 1;  //Keeps track of backward depth

      //Enqueue (cube, k) and (pred, k-1) at the same priority
      pdrQueuePushCube(pdrMgr, cube, k, prio);
      pdrQueuePushCube(pdrMgr, pred, k - 1, prio);
    } else {

      //Clause !cube is inductive relative to Fk-1: generalize it while keeping induction
      PDR_DBG(printf("GENERALIZE c:%d\n", counter));
      clMin =
        pdrGeneralizeClause(pdrMgr, k - 1, cube, genEffort /* effort */ );

      Pdtutil_Assert(pdrChkGenWithInitAndRefine(pdrMgr, clMin, cube, 0) != 0,
        "cube intersects init state");

      PDR_DBG(pdrCheckSolvers(pdrMgr));
      Ddi_Clause_t *cubeMin = Ddi_ClauseNegLits(clMin);

      PDR_DBG(Pdtutil_Assert(Ddi_ClauseSubsumed(cube, cubeMin),
          "subsumption required"));
      PDR_DBG(printf("CONFIRMED cubeMin (%d) c:%d\n", k, counter));
      PDR_DBG(Ddi_PdrPrintClause(pdrMgr->ddiPdr, cubeMin));

      //Refine every time frame from 1 to k to block the cube, by adding the newly found inductive lemma to them
      for (i = 1; i <= k; i++) {
        if (pdrMgr->settings.enforceInductiveClauses) {
          //Add the inductive lemma also to the next state if enforce option is enabled
          Ddi_SatSolverAddClauseCustomized(pdrMgr->timeFrames[i].solver, clMin,
            0, 1, 0);
          if (pdrMgr->timeFrames[i].hasSolver2)
            Ddi_SatSolverAddClauseCustomized(pdrMgr->timeFrames[i].solver2,
              clMin, 0, 1, 0);
        }
        //Add inductive lemma to time frame
        Ddi_SatSolverAddClause(pdrMgr->timeFrames[i].solver, clMin);
	Pdtutil_Assert(Ddi_SatSolverOkay(pdrMgr->timeFrames[i].solver), "UNSAT solver");
        Ddi_SatSolverSimplify(pdrMgr->timeFrames[i].solver);
        if (pdrMgr->timeFrames[i].hasSolver2) {
          Ddi_SatSolverAddClause(pdrMgr->timeFrames[i].solver2, clMin);
          Ddi_SatSolverSimplify(pdrMgr->timeFrames[i].solver2);
        }
        PDR_DBG(pdrCheckSolvers(pdrMgr));
      }

      //Add clause to clause set of Fk
      Ddi_ClauseArrayPush(pdrMgr->timeFrames[k].clauses, clMin);

      //Inductive push
      if (doInductivePush) {

        //Tries to push the inductive clause forward while keeping induction: adds the inductive lemma to each forward time frame for which inducton still holds,
        //stops when it doesn't hold anymore.

        //TOASK
        //kMax0 = pdrMgr->settings.unfoldProp ? kMax : kMax-1; TODO: si deve mettere, cambiando il kMax nella riga sotto?
        while (k < kMax) {

          //Check if relative induction still hold with respect to the next time frame
          pred =
            pdrInductivePredecessor(pdrMgr, k, cubeMin, NULL, 0, 1, 1,
            PDR_INDUCTIVE_PUSH_MODE);

          //Induction does not hold anymore
          if (pred != NULL) {
            Ddi_ClauseFree(pred);
            break;
          }
          //Induction still holds: push clause on next time frame (remove clause from clause set of current TF then adds it to the solver and clause set of the next TF)
          Ddi_ClauseArrayRemove(pdrMgr->timeFrames[k].clauses,
            pdrMgr->timeFrames[k].clauses->nClauses - 1);
          k++;
          Ddi_ClauseArrayPush(pdrMgr->timeFrames[k].clauses, clMin);
          Ddi_SatSolverAddClause(pdrMgr->timeFrames[k].solver, clMin);
          Ddi_SatSolverSimplify(pdrMgr->timeFrames[k].solver);
          if (pdrMgr->timeFrames[k].hasSolver2) {
            Ddi_SatSolverAddClause(pdrMgr->timeFrames[k].solver2, clMin);
            Ddi_SatSolverSimplify(pdrMgr->timeFrames[k].solver2);
          }
        }
      }
      //Update variables activity and clause statistics
      Ddi_PdrClauseUpdateActivity(pdrMgr->ddiPdr, clMin);
      PDR_DBG(printf("added to time frame (%d) c:%d\n", k, counter));
      PDR_DBG(Ddi_PdrPrintClause(pdrMgr->ddiPdr, cubeMin));
      pdrMgr->nClauses++;
      PDR_DBG(pdrCheckSolvers(pdrMgr));

      //Enqueue bad cube on the outer ring: the cube found is bad or can reach a bad state, so we can schedule a proof obligation to block
      //such cube in the outer time frame. Infact, even if the cube was found to intersect an innermost time frame it can intersect outermost
      //time frames too. That proof obligation is enqueued at a lower priority.

      //TOASK
      //kMax0 = pdrMgr->settings.unfoldProp ? kMax : kMax-1; TODO: si deve mettere, cambiando il kMax nella riga sotto?
      if (k < kMax) {
        PDR_DBG(Pdtutil_Assert(Ddi_SatSolveCustomized(pdrMgr->
              timeFrames[k].solver, clMin, 1, 1, -1),
            "added clause(next) not reachable in curr solver"));
        if (prio == 0 || k < kMax) {
          pdrQueuePushCube(pdrMgr, cube, k + 1, prio + 1);
          PDR_DBG(printf("RESCHEDULED %d c:%d\n", k + 1, counter));
        }
      } else {
        Ddi_ClauseDeref(cube);
      }
      PDR_DBG(pdrCheckSolvers(pdrMgr));
    }
  }

  //Return no counterexample found
  return NULL;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
pdrCheckFixPointSat(
  TravPdrMgr_t * pdrMgr,
  int k,
  int kMax
)
{
  int k2, i;
  int fixpoint = 1;
  Ddi_SatSolver_t *solver = NULL;
  TravPdrTimeFrame_t *tf = &(pdrMgr->timeFrames[k]);

  if (pdrMgr->ddiPdr->trClausesTseitin != NULL) {
    Ddi_Clause_t *actInvarspec =
      Ddi_ClauseNegLits(pdrMgr->ddiPdr->actTarget);
    solver = Ddi_SatSolverAlloc();
    Ddi_SatSolverAddClauses(solver, pdrMgr->ddiPdr->invarClauses);
    Ddi_SatSolverAddClauses(solver,
      pdrMgr->ddiPdr->trClausesTseitin);

    if (pdrMgr->targetCube != NULL) {
      Ddi_SatSolverAddClauses(solver,
        pdrMgr->ddiPdr->targetClauses);
    } else {
      Ddi_SatSolverAddClauses(solver,
        pdrMgr->ddiPdr->targetClauses);
    }

    Ddi_SatSolverAddClause(solver, actInvarspec);
    Ddi_ClauseFree(actInvarspec);
    for (k2 = k + 1; k2 <= kMax; k2++) {
      TravPdrTimeFrame_t *tf2 = &(pdrMgr->timeFrames[k2]);

      Ddi_SatSolverAddClauses(solver, tf2->clauses);
    }
    if (pdrMgr->infTimeFrame != NULL) {
      TravPdrTimeFrame_t *tf2 = pdrMgr->infTimeFrame;

      Ddi_SatSolverAddClauses(solver, tf2->clauses);
    }
  }
  for (k2 = k + 1; k2 <= kMax && fixpoint; k2++) {
    TravPdrTimeFrame_t *tf2 = &(pdrMgr->timeFrames[k2]);

    for (i = 0; i < Ddi_ClauseArrayNum(tf2->clauses) && fixpoint; i++) {
      Ddi_Clause_t *cl = Ddi_ClauseArrayRead(tf2->clauses, i);

      if(solver == NULL && !Ddi_SatSolveCustomized(tf->solver, cl, 1, 1, -1)){
				fixpoint = 0;						
			}
			if(solver != NULL && !Ddi_SatSolveCustomized(solver, cl, 1, 1, -1)){
				fixpoint = 0;						
			}
    }
  }
  if (solver != NULL) {
    Ddi_SatSolverQuit(solver);
    Pdtutil_VerbosityMgr(pdrMgr, Pdtutil_VerbLevelDevMax_c,
      fprintf(stdout, "Aux solver with Tseitin clauses used\n"));
  }
  return fixpoint;
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
pdrTimeFramePushClauses(
  TravPdrMgr_t * pdrMgr,
  int maxFpK
)
{
  //Utility variables
  int i, k, kMax = pdrMgr->nTimeFrames - 1;
  int c1 = 0, c1Aux, c1Aux2, c1b = 0, cTot = 0, c2 = 0;
  int fixpoint = 0;

  if (0 && (maxFpK < kMax)) {
    kMax = maxFpK;
  }
  //Setting variables
  int checkFP = 1;
  static int cc1 = 0, cc1b = 0, cc2 = 0, ccTot = 0;

  //Push loop
  for (k = 1; k <= kMax; k++) {

    //Get current and next time frames
    TravPdrTimeFrame_t *tf = &(pdrMgr->timeFrames[k]);
    TravPdrTimeFrame_t *tf1 =
      (k == kMax) ? NULL : &(pdrMgr->timeFrames[k + 1]);

    //Sort and compact clause array
    Ddi_ClauseArraySort(tf->clauses);
    pdrMgr->nRemoved += (c1Aux = Ddi_ClauseArrayCompact(tf->clauses));
    c1 += c1Aux;

    //Try to push clauses forward if its not the last time frame
    if (tf1 != NULL) {
      int pushed;

      //TOASK
      //questo do while puo' essere tolto?
      do {
        pushed = 0;

        //Scan every clause of the time frame
        for (i = 0; i < Ddi_ClauseArrayNum(tf->clauses); i++) {
          Ddi_Clause_t *cl = Ddi_ClauseArrayRead(tf->clauses, i);

          //Try to push clause
          Ddi_Clause_t *clMin = pdrPushClause(pdrMgr, k, cl);

          cTot++;
          if (clMin != NULL) {

            //Clause can be pushed: a clause clMin that is an inductive generalization of it is derived
            pushed = 1;
            c2++;
            //If the clause has been reduced, add the reduced clause to every preceding time frame
            if (clMin->nLits < cl->nLits) {
              int j;

              for (j = 1; j <= k; j++) {
                if (j < k && pdrMgr->settings.enforceInductiveClauses) {
                  Ddi_SatSolverAddClauseCustomized(pdrMgr->
                    timeFrames[j].solver, clMin, 0, 1, 0);
                  if (pdrMgr->timeFrames[j].hasSolver2)
                    Ddi_SatSolverAddClauseCustomized(pdrMgr->
                      timeFrames[j].solver2, clMin, 0, 1, 0);
                }
                Ddi_SatSolverAddClause(pdrMgr->timeFrames[j].solver, clMin);
                Ddi_SatSolverSimplify(pdrMgr->timeFrames[j].solver);
                if (pdrMgr->timeFrames[j].hasSolver2) {
                  Ddi_SatSolverAddClause(pdrMgr->timeFrames[j].solver2, clMin);
                  Ddi_SatSolverSimplify(pdrMgr->timeFrames[j].solver2);
                }
              }
            }
            //Add clause to next time frame
            pdrMgr->nRemoved += (c1Aux2 =
              Ddi_ClauseArrayCompactWithClause(tf1->clauses, clMin));
            c1b += c1Aux2;
            Ddi_ClauseArrayPush(tf1->clauses, clMin);
            Ddi_SatSolverAddClause(tf1->solver, clMin);
            Ddi_SatSolverSimplify(tf1->solver);
            if (tf1->hasSolver2) {
              Ddi_SatSolverAddClause(tf1->solver2, clMin);
              Ddi_SatSolverSimplify(tf1->solver2);
            }
            //Remove clause from current time frame set
            Ddi_ClauseArrayRemove(tf->clauses, i);
            PDR_DBG(printf("push ca[%d] - %d->%d\n", i, k, k + 1));
            PDR_DBG(Ddi_PdrPrintClause(pdrMgr->ddiPdr, clMin));
            i--;
          }
        }

        if (k > 1 && (k < maxFpK)) {
	  if (pdrMgr->maxItpShare > 0 && (k<=pdrMgr->maxItpShare)) {
	    //Check fixpoint semantically
	    fixpoint = pdrCheckFixPointSat(pdrMgr, k, kMax);
	  }
	  else {
            fixpoint = Ddi_ClauseArrayNum(tf->clauses) == 0;
	  }
	}

        if (fixpoint) {
          TravPdrTimeFrame_t *tf0 = &(pdrMgr->timeFrames[k - 1]);

          if (pdrMgr->maxItpShare < k - 1) {

            //Explicitly check fixpoint soundness
            if (checkFP) {
              int k2, checks = 0;
              Ddi_SatSolver_t *solver = NULL;

              if (pdrMgr->ddiPdr->trClausesTseitin != NULL) {
                Ddi_Clause_t *actInvarspec =
                  Ddi_ClauseNegLits(pdrMgr->ddiPdr->actTarget);
                solver = Ddi_SatSolverAlloc();
                Ddi_SatSolverAddClauses(solver, pdrMgr->ddiPdr->invarClauses);
                Ddi_SatSolverAddClauses(solver,
                  pdrMgr->ddiPdr->trClausesTseitin);

                if (pdrMgr->targetCube != NULL) {

                  //Ddi_SatSolverAddClauses(solver,pdrMgr->ddiPdr->targetClauses);
                  Ddi_SatSolverAddClauses(solver,
                    pdrMgr->ddiPdr->targetClauses);
                } else {
                  Ddi_SatSolverAddClauses(solver,
                    pdrMgr->ddiPdr->targetClauses);
                }


                Ddi_SatSolverAddClause(solver, actInvarspec);
                Ddi_ClauseFree(actInvarspec);
                for (k2 = k + 1; k2 <= kMax; k2++) {
                  TravPdrTimeFrame_t *tf2 = &(pdrMgr->timeFrames[k2]);

                  Ddi_SatSolverAddClauses(solver, tf2->clauses);
                }
                if (pdrMgr->infTimeFrame != NULL) {
                  TravPdrTimeFrame_t *tf2 = pdrMgr->infTimeFrame;

                  Ddi_SatSolverAddClauses(solver, tf2->clauses);
                }
              }
              for (k2 = k + 1; k2 <= kMax; k2++) {
                TravPdrTimeFrame_t *tf2 = &(pdrMgr->timeFrames[k2]);

                for (i = 0; i < Ddi_ClauseArrayNum(tf2->clauses); i++) {
                  Ddi_Clause_t *cl = Ddi_ClauseArrayRead(tf2->clauses, i);

                  Pdtutil_Assert(solver != NULL
                    || !Ddi_SatSolveCustomized(tf->solver, cl, 1, 1, -1),
                    "UNSOUND fixed point in PDR");
                  Pdtutil_Assert(solver == NULL
                    || !Ddi_SatSolveCustomized(solver, cl, 1, 1, -1),
                    "UNSOUND fixed point in PDR (tseitin)");
                  checks++;
                }
              }
              Pdtutil_VerbosityMgr(pdrMgr, Pdtutil_VerbLevelNone_c,
                fprintf(stdout,
                  "fixed point at depth %d proved sound by %d clause checks\n",
                  k, checks));
	      pdrMgr->fixPointTimeFrame = k;

              if (solver != NULL) {
                Ddi_SatSolverQuit(solver);
                Pdtutil_VerbosityMgr(pdrMgr, Pdtutil_VerbLevelNone_c,
                  fprintf(stdout, "Aux solver with Tseitin clauses used\n"));
              }
            }
            //Return fixpoint reached
            return 1;
          }
        }
      } while (0 && pushed);
    }
    //If it's the last time frame but there's the infite one
    else if (k == kMax && pdrMgr->infTimeFrame != NULL) {
      tf1 = pdrMgr->infTimeFrame;

      //Scan every clause in the last time frame
      for (i = 0; i < Ddi_ClauseArrayNum(tf->clauses); i++) {
        Ddi_Clause_t *cl = Ddi_ClauseArrayRead(tf->clauses, i);
        Ddi_Clause_t *cube = Ddi_ClauseNegLits(cl);

        //Check if the clause is inductive relative to the infinite time frame
        Ddi_Clause_t *pred =
          pdrInductivePredecessor(pdrMgr, -1, cube, NULL, 0, 1, 1,
          PDR_INDUCTIVE_PUSH_MODE);

        Ddi_ClauseFree(cube);
        if (pred != NULL) {
          //If it's not clause can't be pushed
          Ddi_ClauseFree(pred);
        } else {
          //If it is, push clause forward on the infinite time frame (it's a global invariant)
          Ddi_Clause_t *clMin = Ddi_ClauseDup(cl);

          Ddi_ClauseArrayPush(tf1->clauses, clMin);
          Ddi_SatSolverAddClause(tf1->solver, clMin);
          Ddi_SatSolverSimplify(tf1->solver);
          if (tf1->hasSolver2) {
            Ddi_SatSolverAddClause(tf1->solver2, clMin);
            Ddi_SatSolverSimplify(tf1->solver2);
          }
          //Remove it from the last time frame
          Ddi_ClauseArrayRemove(tf->clauses, i);
          PDR_DBG(printf("push ca[%d] - %d->%d\n", i, k, k + 1));
          PDR_DBG(Ddi_PdrPrintClause(pdrMgr->ddiPdr, clMin));
          i--;
        }
      }
    }
  }

  cc1 += c1;
  cc1b += c1b;
  cc2 += c2;
  ccTot += cTot;
  Pdtutil_VerbosityMgrIf(pdrMgr, Pdtutil_VerbLevelDevMed_c) {
    printf("PUSH: %d %d %d <%d> (%d %d %d <%d>) - ", cTot, c2, c1, c1b, ccTot,
      cc2, cc1, cc1b);
    printf("cont: %d - deq: %d\n", nContained, pdrMgr->stats.nQueuePop);
  }
  //Compact infinite time frame
  if (pdrMgr->infTimeFrame != NULL) {
    Ddi_ClauseArraySort(pdrMgr->infTimeFrame->clauses);
    pdrMgr->nRemoved += Ddi_ClauseArrayCompact(pdrMgr->infTimeFrame->clauses);
  }
  //Return fixpoint not reached
  return 0;

}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
pdrTimeFrameCheckClauses(
  TravPdrMgr_t * pdrMgr
)
{
  int i, k, kMax = pdrMgr->nTimeFrames - 1;
  int chkSubs = 1;
  int minId = -1;

  for (k = 1; k <= kMax; k++) {
    TravPdrTimeFrame_t *tf = &(pdrMgr->timeFrames[k]);
    TravPdrTimeFrame_t *tf1 =
      (k == kMax) ? NULL : &(pdrMgr->timeFrames[k + 1]);
    if (chkSubs)
      Ddi_ClauseArrayCheckCompact(tf->clauses);

    //Checks conditions (1) {Fi --> Fi+1} (syntactically: clauses(Fi) includes clauses(Fi+1)) and (2) {Fi & TR --> Fi+1}
#if 1||DR_DBG
    if (tf1 != NULL) {
      for (i = 0; i < Ddi_ClauseArrayNum(tf1->clauses); i++) {
        Ddi_Clause_t *cl = Ddi_ClauseArrayRead(tf1->clauses, i);

        Pdtutil_Assert(!Ddi_SatSolveCustomized(tf->solver, cl, 1, 0, -1),
          "frontier clause not in prev solver");
        Pdtutil_Assert(!Ddi_SatSolveCustomized(tf1->solver, cl, 1, 0, -1),
          "frontier clause not in curr solver");
        if (k < kMax) {
          Pdtutil_Assert(Ddi_SatSolveCustomized(tf->solver, cl, 0, 1, -1),
            "frontier clause(next) not reachable in curr solver");
          if (Ddi_SatSolveCustomized(tf->solver, cl, 1, 1, -1)) {
            printf
              ("frontier [%d] clause(next) does not include img in curr solver - id: %d\n",
              k, cl->id);
            Ddi_PdrPrintClause(pdrMgr->ddiPdr, cl);
            if (minId < 0 || minId > cl->id)
              minId = cl->id;
          }
          Pdtutil_Assert(1
            || !Ddi_SatSolveCustomized(tf->solver, cl, 1, 1, -1),
            "frontier clause(next) does not include img in curr solver");
        }
      }
    }
#endif
  }

  if (minId >= 0) {
    printf("wrong frontier id: %d\n", minId);
  }

  return 0;

}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
pdrTimeFrameShareClauses(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  TravPdrMgr_t * pdrMgr,
  int k
)
{
  int i, k0, kMax = pdrMgr->nTimeFrames - 1;
  Trav_Shared_t *travShared = &(travMgr->xchg.shared);
  int fullShare = pdrMgr->settings.shareReached > 1;
  Trav_Shared_t *travItpShared = travShared->sharedLink;

  if (k == 0)
    return 0;
  if (!pdrMgr->settings.shareReached && !pdrMgr->settings.shareItpReached)
    return 0;

  if (pdrMgr->settings.shareReached) {
    pthread_mutex_lock(&travShared->mutex);
    if (travShared->clauseShared == NULL) {
      travShared->clauseShared = Ddi_ClauseArrayAlloc(40);
      travShared->clauseSharedNum = Pdtutil_IntegerArrayAlloc(10);
    } else if (fullShare) {
      Ddi_ClauseArrayReset(travShared->clauseShared);
      Pdtutil_IntegerArrayReset(travShared->clauseSharedNum);
    }

    for (k0 = (fullShare ? 1 : k); k0 <= k; k0++) {
      TravPdrTimeFrame_t *tf = &(pdrMgr->timeFrames[k0]);

      for (i = 0; i < Ddi_ClauseArrayNum(tf->clauses); i++) {
        Ddi_Clause_t *cl = Ddi_ClauseArrayRead(tf->clauses, i);
        Ddi_Clause_t *cl2 = Ddi_ClauseDup(cl);

        Ddi_ClauseArrayPush(travShared->clauseShared, cl2);
      }
      Pdtutil_IntegerArrayInsertLast(travShared->clauseSharedNum, i);
    }
    pthread_mutex_unlock(&travShared->mutex);
  }

  if (pdrMgr->settings.shareItpReached &&
    travItpShared != NULL && (travItpShared->itpRchShared != NULL)) {

    pthread_mutex_lock(&travItpShared->mutex);

    Pdtutil_Assert(travItpShared->ddmXchg != NULL, "null ddi xchg mgr");

    if (Ddi_BddarrayNum(travItpShared->itpRchShared) > k) {

      Ddi_Mgr_t *ddm = pdrMgr->ddiPdr->ddiMgr;
      bAig_Manager_t *bmgr = ddm->aig.mgr;
      Ddi_Bdd_t *rX = Ddi_BddarrayRead(travItpShared->itpRchShared, k);
      Ddi_Bdd_t *myR = Ddi_BddCopy(ddm, rX);
      int j, nVars = pdrMgr->ddiPdr->nState;
      Ddi_ClauseArray_t *rClauses;

      rClauses = Ddi_AigClauses(myR, 0, NULL);
      Pdtutil_VerbosityMgr(pdrMgr, Pdtutil_VerbLevelNone_c, fprintf(stdout,
          "PDR is loading ITP-R[%d] of size: %d\n", k, Ddi_BddSize(rX)));
      for (k0 = 1; k0 <= k; k0++) {
        TravPdrTimeFrame_t *tf = &(pdrMgr->timeFrames[k0]);

        Ddi_SatSolverAddClauses(tf->solver, rClauses);
        if (tf->hasSolver2)
          Ddi_SatSolverAddClauses(tf->solver2, rClauses);
      }
      Pdtutil_Assert(pdrMgr->timeFrames[k0].itpClauses == NULL,
        "itp clauses!=0");
      pdrMgr->timeFrames[k0].itpClauses = rClauses;

      pdrMgr->maxItpShare = k;

      Ddi_BddarrayInsertLast(pdrMgr->ddiPdr->bddSave, myR);

      pthread_mutex_unlock(&travItpShared->mutex);
    } else {
      pthread_mutex_unlock(&travItpShared->mutex);
    }

  }

  return i;
}


/**Function*******************************************************************
  Synopsis    [Checks if a clause (or a clause that subsumes it) is already
              present among the clauses of a time frame. ]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
pdrClauseIsContained(
  TravPdrMgr_t * pdrMgr,
  Ddi_Clause_t * cl,
  int k0
)
{
  int i, k, kMax = pdrMgr->nTimeFrames - 1;

  //Check if clause is contained in Fk0 clause set
  for (k = k0; k <= kMax; k++) {
    TravPdrTimeFrame_t *tf = &(pdrMgr->timeFrames[k]);

    if (Ddi_ClauseInClauseArray(tf->clauses, cl)) {
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
static void
pdrQueuePushCube(
  TravPdrMgr_t * pdrMgr,
  Ddi_Clause_t * cube,
  int k,
  int prio
)
{
  //Push cube to be blocked in appropriate priority queue
  if (prio > pdrMgr->queueMaxPrio) {
    prio = pdrMgr->queueMaxPrio;
  }
  Ddi_ClauseArrayPush(pdrMgr->timeFrames[k].queue[prio], cube);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Clause_t *
pdrQueuePopCube(
  TravPdrMgr_t * pdrMgr,
  int *kp,
  int *prio
)
{
  int k, j;

  Pdtutil_Assert(kp != NULL, "Null k pointer in pop cube");

  //Dequeue cube to be blocked from the queue with the highest priority of the lowest time frame
  for (k = 0; k < pdrMgr->nTimeFrames; k++) {
    for (j = 0; j <= pdrMgr->queueMaxPrio; j++) {
      if (Ddi_ClauseArrayNum(pdrMgr->timeFrames[k].queue[j]) > 0) {
        *kp = k;
        *prio = j;
        pdrMgr->stats.nQueuePop++;
        return Ddi_ClauseArrayPop(pdrMgr->timeFrames[k].queue[j]);
      }
    }
  }
  return NULL;
}

/**Function*******************************************************************
  Synopsis    [Checks if a solver must be cleaned up and reloaded.]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
pdrSolverCheckRestart(
  TravPdrMgr_t * pdrMgr,
  int k,
  int useSolver2
)
{
  //Utility variables
  TravPdrTimeFrame_t *tf =
    k < 0 ? pdrMgr->infTimeFrame : &(pdrMgr->timeFrames[k]);
  int restarted = 0;

  //Return if restart is not enabled
  if (!pdrMgr->settings.solverRestartEnable)
    return 0;

  //Get solver to restart
  if (useSolver2 && !tf->hasSolver2)
    return 0;
  Ddi_SatSolver_t *solver = (useSolver2) ? tf->solver2 : tf->solver;

  //Check if solver has to be restarted
  float avgUselessClauses = 0.0;
  float currentThreshold = 0.0;

  switch (useSolver2 ? pdrMgr->settings.restartStrategy2 : pdrMgr->
    settings.restartStrategy) {
    case PDR_DEFAULT_RESTART:
      if (((solver->nFreeTot > solver->settings.nFreeMax)
          || (solver->S->stats.starts > 1000)))
        restarted = 1;
      break;
    case PDR_CUBE_DEPENDENT_RESTART:
      avgUselessClauses =
        (useSolver2 ? tf->wndSum2 / TRAV_PDR_RESTART_WINDOW_SIZE : tf->wndSum /
        TRAV_PDR_RESTART_WINDOW_SIZE);
      currentThreshold =
        (useSolver2 ? pdrMgr->settings.restartThreshold2 *
        solver->S->nClauses() : pdrMgr->settings.restartThreshold) *
        solver->S->nClauses();
      if (avgUselessClauses > currentThreshold) {
        restarted = 1;
      }
      break;
    case PDR_DOUBLE_THRESHOLD_RESTART:
      avgUselessClauses =
        (useSolver2 ? tf->wndSum2 / TRAV_PDR_RESTART_WINDOW_SIZE : tf->wndSum /
        TRAV_PDR_RESTART_WINDOW_SIZE);
      currentThreshold =
        (useSolver2 ? pdrMgr->settings.restartThreshold2 *
        solver->S->nClauses() : pdrMgr->settings.restartThreshold) *
        solver->S->nClauses();
      if (solver->nFreeTot > (0.5 * solver->S->nVars())
        || avgUselessClauses > currentThreshold) {
        restarted = (solver->nFreeTot > (0.5 * solver->S->nVars()))? 1 : 2;
      }
      break;
    case PDR_VAR_THRESHOLD_RESTART:
      if (solver->nFreeTot > (0.5 * solver->S->nVars())) {
        restarted = 1;
      }
      break;
    default:
      restarted = 0;
      break;
  }

  restarted |= customRestart;

  //Do restart
  if (restarted) {

    //Collect statistics
    int d = (int)(solver->S->stats.decisions);
    int c = (int)(solver->S->stats.conflicts);
    int p = (int)(solver->S->stats.propagations);
    int cl = (int)(solver->S->stats.clauses_literals);
    int nv = (int)(solver->S->nVars());
    int nc = (int)(solver->S->nClauses());
    int ns = (int)(solver->S->stats.starts);
    long initTime, restartTime;

    ++pdrMgr->stats.solverRestart;
    pdrMgr->stats.nDec += d;
    pdrMgr->stats.nProp += p;
    pdrMgr->stats.nConfl += c;
    pdrMgr->stats.solverTime += solver->cpuTime;
    tf->stats.solverTime += solver->cpuTime;

    //Output restart line
    switch (useSolver2 ? pdrMgr->settings.restartStrategy2 : pdrMgr->
      settings.restartStrategy) {
      case PDR_DEFAULT_RESTART:
        Pdtutil_VerbosityMgrIf(pdrMgr, Pdtutil_VerbLevelDevMed_c) {
          fprintf(stdout, "restarting SAT solver%s #%d (%d) - [%s]\n",
            useSolver2 ? "2" : "", k, pdrMgr->stats.solverRestart,
            (solver->nFreeTot >
              solver->settings.nFreeMax) ? "by act literals" : "by restarts");
          fprintf(stdout,
            "solver stats: dec:%d, prop:%d, confl:%d, cllit:%d, v:%d, c:%d, st:%d\n",
            d, p, c, cl, nv, nc, ns);
        }
        break;
      case PDR_CUBE_DEPENDENT_RESTART:
        Pdtutil_VerbosityMgrIf(pdrMgr, Pdtutil_VerbLevelDevMed_c) {
          fprintf(stdout, "restarting SAT solver%s #%d (c:%f, t:%f)\n",
            useSolver2 ? "2" : "", k, pdrMgr->stats.solverRestart,
            avgUselessClauses, currentThreshold);
          fprintf(stdout,
            "solver stats: dec:%d, prop:%d, confl:%d, cllit:%d, v:%d, c:%d, st:%d\n",
            d, p, c, cl, nv, nc, ns);
        }
        break;
      case PDR_DOUBLE_THRESHOLD_RESTART:
        Pdtutil_VerbosityMgrIf(pdrMgr, Pdtutil_VerbLevelDevMed_c) {
          fprintf(stdout, "restarting SAT solver%s #%d by %s (c:%f, t:%f)\n",
            useSolver2 ? "2" : "", k,
            (restarted == 1) ? "useless vars" : "useless clauses",
            pdrMgr->stats.solverRestart,
            (restarted == 1) ? solver->nFreeTot : avgUselessClauses,
            (restarted == 1) ? 0.5 * solver->S->nVars() : currentThreshold);
          fprintf(stdout,
            "solver stats: dec:%d, prop:%d, confl:%d, cllit:%d, v:%d, c:%d, st:%d\n",
            d, p, c, cl, nv, nc, ns);
        }
        break;
      case PDR_VAR_THRESHOLD_RESTART:
        Pdtutil_VerbosityMgrIf(pdrMgr, Pdtutil_VerbLevelDevMed_c) {
          fprintf(stdout, "restarting SAT solver%s #%d (v:%f, t:%f)\n",
            useSolver2 ? "2" : "", k, solver->nFreeTot,
            0.5 * solver->S->nVars());
          fprintf(stdout,
            "solver stats: dec:%d, prop:%d, confl:%d, cllit:%d, v:%d, c:%d, st:%d\n",
            d, p, c, cl, nv, nc, ns);
        }
        break;
      default:
        break;
    }

    //Quit solver
    initTime = util_cpu_time();
    Ddi_SatSolverQuit(solver);

    //Allocate and load new solver
    solver = Ddi_SatSolverAlloc();
    int incrementalLoad = pdrMgr->settings.incrementalTrClauses && k != 0;

    if (customRestart)
      incrementalLoad = 0;
    pdrTimeFrameLoadSolver(pdrMgr, solver, k, incrementalLoad, useSolver2);
    if (useSolver2) {
      tf->solver2 = solver;
    } else {
      tf->solver = solver;
    }
    restartTime = util_cpu_time() - initTime;
    pdrMgr->stats.restartTime += (float)restartTime / 1000.0;
    cl = (int)(solver->S->stats.clauses_literals);
    nv = (int)(solver->S->nVars());
    nc = (int)(solver->S->nClauses());
    Pdtutil_VerbosityMgrIf(pdrMgr, Pdtutil_VerbLevelDevMed_c) {
      fprintf(stdout, "NEW solver stats: cllit:%d, v:%d, c:%d\n", cl, nv, nc);
    }
  }

  return restarted;
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Clause_t *
pdrInductivePredecessor(
  TravPdrMgr_t * pdrMgr,
  int k,
  Ddi_Clause_t * cube,
  Ddi_Clause_t ** pCore,
  int enTernary,                //Require extension of the predecessor (if found)
  int enReuseActVar,
  int useInduction,             //This function can be used to find an inductive predecessor or just a predecessor
  int mode                      //Different calls to inductive predecessor require different processing: if called during cube blocking, the inductive predecessor found must
  //be extended using ternary simulation, if called during inductive push or generalization only a check of induction is required with no
  //need to extend an inductive predecessor when found. Also SAT calls performed by this function can be specialized depending on the call cantext.
)
{
  //Utility variables
  int actLit = -1, sat, kMax = pdrMgr->nTimeFrames - 1;
  Ddi_Clause_t *assume, *assumeAct = NULL;
  TravPdrTimeFrame_t *tf =
    k < 0 ? pdrMgr->infTimeFrame : &(pdrMgr->timeFrames[k]);
  int restarted = 0;

  //Safety assertions
  Pdtutil_Assert(cube->nLits > 0, "void clause in pred eval");
  if (tf == NULL) {
    Pdtutil_Assert(k < 0, "null tf");
  }
  //Determine which solver to use
  int specCall;

  switch (mode) {
    case PDR_BLOCKING_MODE:
      specCall = isSpecializedCall(pdrMgr, PDR_INDUCTIVE_PREDECESSOR_CALL, k);
      break;
    case PDR_GENERALIZE_MODE:
      specCall =
        isSpecializedCall(pdrMgr, PDR_INDUCTIVE_GENERALIZATION_CALL, k);
      break;
    case PDR_INDUCTIVE_PUSH_MODE:
      specCall = isSpecializedCall(pdrMgr, PDR_INDUCTIVE_PUSH_CALL, k);
      break;
    default:
      specCall = isSpecializedCall(pdrMgr, PDR_INDUCTIVE_PREDECESSOR_CALL, k);
      break;
  }
  Ddi_SatSolver_t *solver = specCall ? tf->solver2 : tf->solver;

  //Manage loading of TR cone (only for non-initial time frames)
  int neededConeSize;
  int uselessLoadedConesSize;

  if (pdrMgr->settings.incrementalTrClauses && k != 0) {
    int *numLoadedTrClauses =
      specCall ? &(tf->stats.numLoadedTrClauses2) : &(tf->
      stats.numLoadedTrClauses);

    //Compute needed cone size
    if (!pdrMgr->settings.usePgEnconding) {
      neededConeSize = computeConeSize(pdrMgr, cube, 1 /* Cube in PS */ ,
        pdrMgr->settings.overApproximateConeSize);
    } else {
      int neededConeSizeOld =
        computeConeSize(pdrMgr, cube, 1 /* Cube in PS */ ,
        pdrMgr->settings.overApproximateConeSize);

      neededConeSize =
        computeConeSizeWithPg(pdrMgr, cube, 1 /* Cube in PS */ );
    }

    //Check restart
    if ((restarted = pdrSolverCheckRestart(pdrMgr, k, specCall))) {
      solver = (specCall) ? tf->solver2 : tf->solver;
    }
    //Get transitio relation clauses to load
    Ddi_ClauseArray_t *clausesToLoad;

    if (!pdrMgr->settings.usePgEnconding) {
      clausesToLoad =
        pdrFindOrAddIncrementalClauses(tf, pdrMgr->ddiPdr, cube,
        1 /* Cube in PS */ , specCall);
    } else {
      clausesToLoad =
        pdrFindOrAddIncrementalClausesWithPg(tf, pdrMgr->ddiPdr, cube,
        1 /* Cube in PS */ , specCall);
    }
    int numClausesToLoad = Ddi_ClauseArrayNum(clausesToLoad);

    //Compute number of useless clauses loaded
    //Pdtutil_Assert(!restarted || ((*numLoadedTrClauses) == 0 && numClausesToLoad == neededConeSize), "");
    uselessLoadedConesSize =
      (*numLoadedTrClauses) - (neededConeSize - numClausesToLoad);

    //Update restart window
    registerCall(pdrMgr, k, specCall, uselessLoadedConesSize);

    //Add TR cone clauses to the solver
    Ddi_SatSolverAddClauses(solver, clausesToLoad);
    (*numLoadedTrClauses) += numClausesToLoad;
    Ddi_ClauseArrayFree(clausesToLoad);
    Pdtutil_Assert((*numLoadedTrClauses) <=
      pdrMgr->ddiPdr->trClauses->nClauses,
      "Some TR clauses are loaded multiple times.");

    //Bump activity based on topological information
    if (pdrMgr->settings.bumpActTopologically) {
      bumpActTopologically(tf, pdrMgr, specCall);
    }
  }
  //Prepare and add to solver induction clause (only for non-initial timeframes, initial TF is exact)
  if (useInduction && k != 0) {

    //TOASK propUnfold
    //For small cubes check induction without adding !cube to the solver, but by enumerating and assuming the states it encodes
    if (!enTernary && (cube->nLits < pdrMgr->settings.indAssumeLits) && (1
        || !pdrMgr->settings.unfoldProp)) {
      return pdrInductivePredecessorNoActLit(pdrMgr, k, cube, enTernary, mode);
    }
    //Generate and load induction clause (!cube + act lit)
    actLit = DdiSatSolverGetFreeVar(solver);
    PDR_DBG1(Pdtutil_Assert(Ddi_SatSolverOkay(solver),
        "UNSAT before clause add"));
    Pdtutil_Assert(Ddi_SatSolverOkay(solver), "UNSAT before clause add");
    Ddi_SatSolverAddClauseCustomized(solver, cube, 1, 0, actLit);
    Pdtutil_Assert(Ddi_SatSolverOkay(solver), "UNSAT after clause add");
    assumeAct = Ddi_ClauseAlloc(0, 1);
    DdiClauseAddLiteral(assumeAct, actLit);
    PDR_DBG1(Pdtutil_Assert(Ddi_SatSolve(solver, assumeAct, -1),
        "unsat on ps clause"));
    specCall ? tf->stats.numInductionClauses2++ : tf->
      stats.numInductionClauses++;
  } else if (!useInduction) {
    //Add negated cube to solver (when not using induction)
    //      Ddi_SatSolverAddClauseCustomized(solver,cube,1,0,0);
  }
  //Generate next state assumption (cube + act lit)
  assume = Ddi_ClauseRemapVars(cube, 1);
  if (k != 0 && useInduction) {
    /* assume ps clause activation literal */
    DdiClauseAddLiteral(assume, actLit);
  }
  //Make SAT call
  int dec;

  if (mode == PDR_GENERALIZE_MODE) {
    //Check if relative induction is maintained for reduced clause in generalization (Q4)
    dec = solver->S->stats.decisions;
    sat = Ddi_SatSolve(solver, assume, -1);
    pdrMgr->stats.split[5] += (solver->S->stats.decisions - dec);
  } else if (mode == PDR_INDUCTIVE_PUSH_MODE) {
    //Check if relative induction hold on the next time frame (Q7)
    dec = solver->S->stats.decisions;
    sat = Ddi_SatSolve(solver, assume, -1);
    pdrMgr->stats.split[4] += (solver->S->stats.decisions - dec);
  } else if (mode == PDR_BLOCKING_MODE) {
    //Check if relative induction hold on the next time frame (Q3)
    dec = solver->S->stats.decisions;
    sat = Ddi_SatSolve(solver, assume, -1);
    pdrMgr->stats.split[6] += (solver->S->stats.decisions - dec);
  } else {
    //Check if clause !cube is inductive relative to Fk (DEBUG)
    dec = solver->S->stats.decisions;
    sat = Ddi_SatSolve(solver, assume, -1);
  }
  pdrMgr->stats.split[2] += (solver->S->stats.decisions - dec);
  pdrMgr->stats.nUnsat += !sat;

  //Free assume clause
  if (assumeAct != NULL)
    Ddi_ClauseFree(assumeAct);
  Ddi_ClauseFree(assume);

  //Check if (possibly) found predecessor must be extended
  if (sat && !enTernary) {
    return Ddi_ClauseDup(cube);
  } else if (sat) {

    //Extend predecessor by ternary simulation
    Ddi_Clause_t *coiSupp = Ddi_PdrClauseCoi(pdrMgr->ddiPdr, cube);
    Ddi_Clause_t *cex = Ddi_SatSolverGetCexCube(solver, coiSupp);
    Ddi_Clause_t *extendedCex;

    Ddi_ClauseFree(coiSupp);
    PDR_DBG(printf("PREDECESSOR\n"));
    PDR_DBG(Ddi_PdrPrintClause(pdrMgr->ddiPdr, cex));
    extendedCex =
      pdrExtendCex(pdrMgr,
      specCall ? pdrMgr->timeFrames[k].solver2 : pdrMgr->timeFrames[k].solver,
      cex, cube, enTernary);

    //Return extended predecessor
    return extendedCex;
  } else if (pCore != NULL) {
    // use unsat core
    Ddi_Clause_t *core =
      Ddi_SatSolverFinal(tf->solver, 2 * pdrMgr->ddiPdr->nState);
    if (core->nLits < cube->nLits) {
      *pCore = Ddi_ClauseRemapVars(core, -1);
    }
    Ddi_ClauseFree(core);
  }
  //Return null if relative induction holds
  return NULL;
}

#if 1
/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Clause_t *
pdrInductivePredecessorNoActLit(
  TravPdrMgr_t * pdrMgr,
  int k,
  Ddi_Clause_t * cube,
  int enTernary,
  int mode
)
{
  //Utility variables
  int sat, i, kMax = pdrMgr->nTimeFrames - 1;
  Ddi_Clause_t *assume;
  TravPdrTimeFrame_t *tf =
    k < 0 ? pdrMgr->infTimeFrame : &(pdrMgr->timeFrames[k]);
  int restarted = 0;

  //Safety assertions
  Pdtutil_Assert(cube->nLits > 0, "void clause in pred eval");
  Pdtutil_Assert(k != 0, "k!=0 required");
  if (tf == NULL) {
    Pdtutil_Assert(k < 0, "null tf");
  }
  //Determine which solver to use
  int specCall;

  switch (mode) {
    case PDR_BLOCKING_MODE:
      specCall = isSpecializedCall(pdrMgr, PDR_INDUCTIVE_PREDECESSOR_CALL, k);
      break;
    case PDR_GENERALIZE_MODE:
      specCall =
        isSpecializedCall(pdrMgr, PDR_INDUCTIVE_GENERALIZATION_CALL, k);
      break;
    case PDR_INDUCTIVE_PUSH_MODE:
      specCall = isSpecializedCall(pdrMgr, PDR_INDUCTIVE_PUSH_CALL, k);
      break;
    default:
      specCall = isSpecializedCall(pdrMgr, PDR_INDUCTIVE_PREDECESSOR_CALL, k);
      break;
  }
  Ddi_SatSolver_t *solver = specCall ? tf->solver2 : tf->solver;

  //Manage loading of TR cone
  int neededConeSize;
  int uselessLoadedConesSize;

  if (pdrMgr->settings.incrementalTrClauses && k != 0) {
    int *numLoadedTrClauses =
      specCall ? &(tf->stats.numLoadedTrClauses2) : &(tf->
      stats.numLoadedTrClauses);

    //Compute needed cone size
    if (!pdrMgr->settings.usePgEnconding) {
      neededConeSize = computeConeSize(pdrMgr, cube, 1 /* Cube in PS */ ,
        pdrMgr->settings.overApproximateConeSize);
    } else {
      int neededConeSizeOld =
        computeConeSize(pdrMgr, cube, 1 /* Cube in PS */ ,
        pdrMgr->settings.overApproximateConeSize);

      neededConeSize =
        computeConeSizeWithPg(pdrMgr, cube, 1 /* Cube in PS */ );
    }

    //Check restart
    if ((restarted = pdrSolverCheckRestart(pdrMgr, k, specCall))) {
      solver = (specCall) ? tf->solver2 : tf->solver;
    }
    //Get array of clauses to load
    Ddi_ClauseArray_t *clausesToLoad;

    if (!pdrMgr->settings.usePgEnconding) {
      clausesToLoad =
        pdrFindOrAddIncrementalClauses(tf, pdrMgr->ddiPdr, cube,
        1 /* Cube in PS */ , specCall);
    } else {
      clausesToLoad =
        pdrFindOrAddIncrementalClausesWithPg(tf, pdrMgr->ddiPdr, cube,
        1 /* Cube in PS */ , specCall);
    }
    int numClausesToLoad = Ddi_ClauseArrayNum(clausesToLoad);

    //Compute number of useless clauses loaded
    //Pdtutil_Assert(!restarted || ((*numLoadedTrClauses) == 0 && numClausesToLoad == neededConeSize), "");
    uselessLoadedConesSize =
      (*numLoadedTrClauses) - (neededConeSize - numClausesToLoad);

    //Update restart window
    registerCall(pdrMgr, k, specCall, uselessLoadedConesSize);

    //Add TR cone clauses to the solver
    Ddi_SatSolverAddClauses(solver, clausesToLoad);
    (*numLoadedTrClauses) += numClausesToLoad;
    Ddi_ClauseArrayFree(clausesToLoad);

    //Bump activity based on topological information
    if (pdrMgr->settings.bumpActTopologically == 1) {
      bumpActTopologically(tf, pdrMgr, specCall);
    }
  }
  //Generate next state assumption (cube)
  assume = Ddi_ClauseRemapVars(cube, 1);

  //Check induction without adding !cube to the solver
  int dec = solver->S->stats.decisions;

  sat = Ddi_SatSolve(solver, assume, -1);
  pdrMgr->stats.split[9] += (solver->S->stats.decisions - dec);
  pdrMgr->stats.nUnsat += !sat;
  Ddi_ClauseFree(assume);

  //If unsat cube is not reachable from Fk
  if (!sat) {
    return NULL;
  }
  //Otherwise check induction enumerating and assuming !cube states
  sat = 0;
  for (i = cube->nLits - 1; !sat && i >= 0; i--) {
    int j;

    assume = Ddi_ClauseRemapVars(cube, 1);
    DdiClauseAddLiteral(assume, -cube->lits[i]);
    for (j = i + 1; j < cube->nLits; j++) {
      DdiClauseAddLiteral(assume, cube->lits[j]);
    }
    int dec = solver->S->stats.decisions;

    sat = Ddi_SatSolve(solver, assume, -1);
    pdrMgr->stats.split[10] += (solver->S->stats.decisions - dec);
    pdrMgr->stats.nUnsat += !sat;
  }
  Ddi_ClauseFree(assume);

  //Check if (possibly) found predecessor must be extended
  if (sat && !enTernary) {
    return Ddi_ClauseDup(cube);
  } else if (sat) {

    //Extend predecessor by ternary simulation
    int i;
    Ddi_Clause_t *coiSupp = Ddi_PdrClauseCoi(pdrMgr->ddiPdr, cube);
    Ddi_Clause_t *cex = Ddi_SatSolverGetCexCube(solver, coiSupp);
    Ddi_Clause_t *extendedCex;

    Ddi_ClauseFree(coiSupp);
    PDR_DBG(printf("PREDECESSOR\n"));
    PDR_DBG(Ddi_PdrPrintClause(pdrMgr->ddiPdr, cex));
    extendedCex =
      pdrExtendCex(pdrMgr,
      specCall ? pdrMgr->timeFrames[k].solver2 : pdrMgr->timeFrames[k].solver,
      cex, cube, enTernary);

    //Return extended predecessor
    return extendedCex;
  }
  //Return null if relative induction holds
  return NULL;
}
#endif

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Clause_t *
pdrExtendCex(
  TravPdrMgr_t * pdrMgr,
  Ddi_SatSolver_t * solver,
  Ddi_Clause_t * cex,
  Ddi_Clause_t * cube,          //Destination of ternary simulation: can be target (NULL) or a cube
  int enTernary                 //If 0 do not perform ternary simulation, if 1 reduce by ternary simulation, if 2 also perform justification, if >2 also perform UNSAT core reduction
)
{

  //Utility variables
  int i;
  Ddi_ClauseArray_t *splitCube = NULL;
  int enSat = cube != NULL && (enTernary > 2);

  //TOASK
  //Mettere a posto il flag enSat: può essere attivo solo se enTernary e' attivo ma viene controllato comunque

  //Settings variables
  int chkRes = 0, enJustify = pdrMgr->settings.cexJustify && (enTernary > 1);

#if PDR_DBG1
  chkRes = 1;
#endif

  //Counter variables
  long initTime, initTime1, genTime;
  long cexGenTimeLocal[4] = { 0, 0, 0, 0 };

  //Safety check
  initTime = util_cpu_time();
  //Pdtutil_Assert(k>=0 || !enTernary,"wrong usage of inf tf");
  if (chkRes) {
    Ddi_Clause_t *cexDup = Ddi_ClauseDup(cex);

    splitCube = Ddi_ClauseArrayAlloc(1);
    Ddi_ClauseArrayPush(splitCube, cexDup);
  }
  //Make initial simulation and justify it
  if (enTernary) {
    int evDrivenJustify = 0;    // not yet implemented
    int useTarget = (cube == NULL);

    //Make initial simulation
    Ddi_Clause_t *cexPi =
      Ddi_SatSolverGetCexCube(solver, pdrMgr->ddiPdr->piClause);
    pdrMgr->settings.step++;
    enTernary =
      Ddi_TernaryPdrSimulate(pdrMgr->xMgr, pdrMgr->settings.ternarySimInit,
      cexPi, cex, cube, useTarget, 0);
    pdrMgr->stats.nTernaryInit++;
    Pdtutil_Assert(!evDrivenJustify,
      "ev driven justification not yet implemented");

    //Justify
    if (enTernary && enJustify) {
      Ddi_Clause_t *cex1 =
        Ddi_TernaryPdrJustify(pdrMgr->ddiPdr, pdrMgr->xMgr, cex,
        evDrivenJustify);

      if (cex1->nLits < cex->nLits) {
        Ddi_ClauseFree(cex);
        cex = cex1;
      } else {
        Ddi_ClauseFree(cex1);
      }
    }
    //Reduce CEX with UNSAT core
    if (enSat && cex->nLits > 1) {
      Ddi_Clause_t *cex1 = Ddi_SatSolverCexCore(solver, cex, cexPi, cube,
        2 * pdrMgr->ddiPdr->nState);

      if (cex1->nLits < cex->nLits) {
        Pdtutil_Assert(Ddi_ClauseSubsumed(cex, cex1), "subsumption required");
        Ddi_ClauseFree(cex);
        cex = cex1;
        Ddi_ClauseSort(cex);
      } else {
        Ddi_ClauseFree(cex1);
      }
    } else if (!evDrivenJustify) {
      //TOASK: questa non viene rifatta?
      enTernary =
        Ddi_TernaryPdrSimulate(pdrMgr->xMgr, pdrMgr->settings.ternarySimInit,
        cexPi, cex, cube, useTarget, 0);
      pdrMgr->stats.nTernaryInit++;
    }
    Ddi_ClauseFree(cexPi);
  } else if (enSat && cex->nLits > 1) {

    //Reduce CEX with unsat core if ternary simulation is not active
    Ddi_Clause_t *cex1, *cexPi =
      Ddi_SatSolverGetCexCube(solver, pdrMgr->ddiPdr->piClause);
    if (cex->nLits > 0) {
      cex1 =
        Ddi_SatSolverCexCore(solver, cex, cexPi, cube,
        2 * pdrMgr->ddiPdr->nState);
      if (cex1->nLits < cex->nLits) {
        Pdtutil_Assert(Ddi_ClauseSubsumed(cex, cex1), "subsumption required");
        Ddi_ClauseFree(cex);
        cex = cex1;
        Ddi_ClauseSort(cex);
      } else {
        Ddi_ClauseFree(cex1);
      }
    }
    Ddi_ClauseFree(cexPi);
  }
  //TOASK
  //If UNSAT core reduction is active disable ternary?
  if (enSat) {
    enTernary = 0;
  }

  initTime1 = util_cpu_time();
  cexGenTime[0] += initTime1 - initTime;

  //Try to extend CEX
  if (enTernary && cex->nLits > 1) {

    //TOASK:
    //Tutta la parte con noInitCnt?
    int noInitCnt;
    Ddi_Clause_t *reducedCex = Ddi_ClauseAlloc(0, 1);

    noInitCnt = 0 && Ddi_PdrCubeNoInitLitNum(pdrMgr->ddiPdr, cex, 0);

    //For each literal simulate to check if it must be kept
    for (i = 0; i < cex->nLits; i++) {
      int keepLit = 0;

      /* process vars without activity */
      if (Ddi_PdrVarHasActivity(pdrMgr->ddiPdr, cex->lits[i]))
        continue;
      if (0 && noInitCnt == 1
        && Ddi_PdrCubeLitIsNoInit(pdrMgr->ddiPdr, cex->lits[i])) {
        /* skip last no init literal alive */
        keepLit = 1;
      }
      pdrMgr->stats.nTernarySim += !keepLit;
      if (keepLit
        || !Ddi_TernaryPdrSimulate(pdrMgr->xMgr, 0, NULL, NULL, NULL, 0,
          cex->lits[i])) {
        DdiClauseAddLiteral(reducedCex, cex->lits[i]);
      } else {
        if (0 && noInitCnt > 1
          && Ddi_PdrCubeLitIsNoInit(pdrMgr->ddiPdr, cex->lits[i])) {
          noInitCnt--;
        }
        //TOASK
        //Questa split cube?
        if (splitCube != NULL) {
          int j, nc = Ddi_ClauseArrayNum(splitCube);

          for (j = 0; j < nc; j++) {
            Ddi_Clause_t *clDup = Ddi_ClauseArrayRead(splitCube, j);

            //Ddi_ClauseDup(Ddi_ClauseArrayRead(splitCube,j));
            clDup->lits[i] = -clDup->lits[i];
            //Ddi_ClauseArrayPush(splitCube,clDup);
          }
        }
      }
      Pdtutil_Assert(noInitCnt >= 0, "Negative no init cnt");
    }

    //TOASK
    //Perche' viene rifatta?
    for (i = 0; i < cex->nLits; i++) {
      int keepLit = 0;

      // process vars with activity
      if (!Ddi_PdrVarHasActivity(pdrMgr->ddiPdr, cex->lits[i]))
        continue;
      if (0 && noInitCnt == 1
        && Ddi_PdrCubeLitIsNoInit(pdrMgr->ddiPdr, cex->lits[i])) {
        //skip last no init literal alive
        keepLit = 1;
      }
      pdrMgr->stats.nTernarySim += !keepLit;
      if (keepLit
        || !Ddi_TernaryPdrSimulate(pdrMgr->xMgr, 0, NULL, NULL, NULL, 0,
          cex->lits[i])) {
        DdiClauseAddLiteral(reducedCex, cex->lits[i]);
      } else {
        if (0 && noInitCnt > 1
          && Ddi_PdrCubeLitIsNoInit(pdrMgr->ddiPdr, cex->lits[i])) {
          noInitCnt--;
        }
        //TOASK
        //Questa split cube?
        if (splitCube != NULL) {
          int j, nc = Ddi_ClauseArrayNum(splitCube);

          for (j = 0; j < Ddi_ClauseArrayNum(splitCube); j++) {
            Ddi_Clause_t *clDup = Ddi_ClauseArrayRead(splitCube, j);

            //Ddi_ClauseDup(Ddi_ClauseArrayRead(splitCube,j));
            clDup->lits[i] = -clDup->lits[i];
            //Ddi_ClauseArrayPush(splitCube,clDup);
          }
        }
      }
    }

    //If CEX is reduced free previous
    if (reducedCex->nLits < cex->nLits) {
      Ddi_ClauseSort(reducedCex);
      Ddi_ClauseFree(cex);
      cex = reducedCex;
      PDR_DBG(printf("X PREDECESSOR\n"));
      PDR_DBG(Ddi_PdrPrintClause(pdrMgr->ddiPdr, cex));
    } else {
      Ddi_Clause_t *freeCex = cex;

      cex = Ddi_ClauseDup(cex);
      Ddi_ClauseFree(reducedCex);
      Ddi_ClauseFree(freeCex);
    }
  } else {
    Ddi_Clause_t *freeCex = cex;

    cex = Ddi_ClauseDup(cex);
    Ddi_ClauseFree(freeCex);
  }

  cexGenTime[1] += util_cpu_time() - initTime1;

  //Debug
  // #if PDR_DBG1
  // if (splitCube != NULL) {
  //   int i;
  //   Ddi_Clause_t *cubeNs = Ddi_ClauseRemapVars(cube, 1);
  //   for (i=0; i<Ddi_ClauseArrayNum(splitCube); i++) {
  //     Ddi_Clause_t *cubePs = Ddi_ClauseArrayRead(splitCube,i);
  //     Ddi_Clause_t *joined = Ddi_ClauseJoin(cubePs,cubeNs);
  //     Pdtutil_Assert(joined!=NULL,"error merging clauses");
  //     PDR_DBG1(
  //       Pdtutil_Assert(Ddi_SatSolve(pdrMgr->timeFrames[k].solver,joined,-1),  "unreachable subcube after X simulation"));
  //     Ddi_ClauseFree(joined);
  //   }
  //   Ddi_ClauseFree(cubeNs);
  //   Ddi_ClauseArrayFree(splitCube);
  // }
  // if (k>0 && enTernary) {
  //   Pdtutil_Assert(!Ddi_SatSolve(pdrMgr->timeFrames[0].solver,cex,-1), "cube reaches init after X simulation");
  // }
  // #endif
  genTime = util_cpu_time() - initTime;
  pdrMgr->stats.cexgenTime += (float)genTime / 1000.0;

  //Return extended cex
  return cex;
}


/**Function*******************************************************************
  Synopsis    [Checks if a clause can be pushed from tf k to tf k+1]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Clause_t *
pdrPushClause(
  TravPdrMgr_t * pdrMgr,
  int k,
  Ddi_Clause_t * cl
)
{
  //Utility variables
  int sat, dec;
  int restarted;
  TravPdrTimeFrame_t *tf = &(pdrMgr->timeFrames[k]);
  int specCall = isSpecializedCall(pdrMgr, PDR_CLAUSE_PUSH_CALL, k);
  Ddi_SatSolver_t *solver = specCall ? tf->solver2 : tf->solver;
  Ddi_Clause_t *assume;
  Ddi_Clause_t *clMin;
  Ddi_Clause_t *cube;

  //Settings variables
  int genEffort = pdrMgr->settings.genEffort;
  int noGeneralize = genEffort <= 3;

  //Update push statistics
  pdrMgr->settings.step++;

  //Generate next state cube
  assume = Ddi_ClauseRemapVarsNegLits(cl, 1);

  //Load needed TR cone
  int neededConeSize;
  int uselessLoadedConesSize;

  if (pdrMgr->settings.incrementalTrClauses && k != 0) {
    int *numLoadedTrClauses =
      specCall ? &(tf->stats.numLoadedTrClauses2) : &(tf->
      stats.numLoadedTrClauses);

    //Compute size of TR cone needed
    if (pdrMgr->settings.usePgEnconding == 0) {
      neededConeSize = computeConeSize(pdrMgr, assume, 0 /* Cube in PS */ ,
        pdrMgr->settings.overApproximateConeSize);
    } else if (pdrMgr->settings.usePgEnconding == 1) {
      int neededConeSizeOld =
        computeConeSize(pdrMgr, assume, 0 /* Cube in PS */ ,
        pdrMgr->settings.overApproximateConeSize);

      neededConeSize =
        computeConeSizeWithPg(pdrMgr, assume, 0 /* Cube in PS */ );
      fprintf(stderr, "%d\t%d\n", neededConeSizeOld, neededConeSize);
    }
    //Check restart
    if (restarted = pdrSolverCheckRestart(pdrMgr, k, specCall)) {
      solver = specCall ? tf->solver2 : tf->solver;
    }
    //Get array of TR clauses to add
    Ddi_ClauseArray_t *clausesToLoad;

    if (pdrMgr->settings.usePgEnconding == 0) {
      clausesToLoad =
        pdrFindOrAddIncrementalClauses(tf, pdrMgr->ddiPdr, assume,
        /* Cube in PS */ 0, specCall);
    } else if (pdrMgr->settings.usePgEnconding == 1) {
      clausesToLoad =
        pdrFindOrAddIncrementalClausesWithPg(tf, pdrMgr->ddiPdr, assume,
        /* Cube in PS */ 0, specCall);
    }
    int numClausesToLoad = Ddi_ClauseArrayNum(clausesToLoad);

    //Compute useless clauses already loaded
    //Pdtutil_Assert(!restarted || ((*numLoadedTrClauses) == 0 && neededConeSize == numClausesToLoad), "");
    uselessLoadedConesSize =
      (*numLoadedTrClauses) - (neededConeSize - numClausesToLoad);

    //Update restart window
    registerCall(pdrMgr, k, specCall, uselessLoadedConesSize);

    //Add TR cone clauses to solver
    Ddi_SatSolverAddClauses(solver, clausesToLoad);
    (*numLoadedTrClauses) += numClausesToLoad;
    Ddi_ClauseArrayFree(clausesToLoad);

    //Bump activity based on topological information
    if (pdrMgr->settings.bumpActTopologically == 1) {
      bumpActTopologically(tf, pdrMgr, specCall);
    }
  }
  //Check if clause can be pushed to the next time frame (Q5)
  if (specCall) {
    dec = tf->solver2->S->stats.decisions;
    sat = Ddi_SatSolve(tf->solver2, assume, -1);
  } else {
    dec = tf->solver->S->stats.decisions;
    sat = Ddi_SatSolve(tf->solver, assume, -1);
  }
  pdrMgr->stats.split[3] += (solver->S->stats.decisions - dec);
  pdrMgr->stats.nUnsat += !sat;

  //If it can't be pushed return NULL (SAT(Fk & TR & !clause')==SAT means that SAT(Fk & clause & TR & !clause')==SAT and thus that clause is not inductive relative to Fk. So it can't be pushed forward.)
  if (sat) {
    Ddi_ClauseFree(assume);
    return NULL;
  }
  //If it can be pushed forward, and so it's inductive relative to Fk, tries to reduce the clause while keeping relative induction
  cube = Ddi_ClauseNegLits(cl);
  if (noGeneralize) {
    clMin = Ddi_ClauseDup(cl);
  } else {
    int myEffort = genEffort > 0 ? genEffort - 1 : 0;

    clMin = pdrGeneralizeClause(pdrMgr, k, cube, myEffort /* effort */ );

    Pdtutil_Assert(pdrChkGenWithInitAndRefine(pdrMgr, clMin, cube, 0) > 0,
      "cube intersects init state");

    //clMin = pdrGeneralizeClause(pdrMgr,k,cube,0/* no effort */);
    //clMin = pdrGeneralizeClause(pdrMgr,k,cube,1/* low effort */);
    //clMin = pdrGeneralizeClause(pdrMgr,k,cube,2/* high effort */);
    PDR_DBG1(Pdtutil_Assert(Ddi_ClauseSubsumed(cl, clMin),
        "subsumption required"));
  }
  if (clMin->nLits < cube->nLits) {
    PDR_DBG(printf("push generalization %d -> %d\n", cube->nLits,
        clMin->nLits));
  } else {
#if PDR_DBG
    /* init state extension can modify clause */
    int i;

    Ddi_ClauseSort(clMin);
    for (i = 0; i < cl->nLits; i++) {
      Pdtutil_Assert(cl->lits[i] == clMin->lits[i], "wrong generalization");
    }
#else
    Ddi_ClauseFree(clMin);
    clMin = Ddi_ClauseDup(cl);
#endif
  }

  //Free memory
  Ddi_ClauseFree(cube);
  Ddi_ClauseFree(assume);
  PDR_DBG1(Pdtutil_Assert(Ddi_ClauseSubsumed(cl, clMin),
      "subsumption required"));

  //Return minimized clause
  return clMin;
}

/**Function*******************************************************************
  Synopsis    [Update current target with a subset of the whole target]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
// static int
// pdrEnumerateTargetCubes(
//   TravPdrMgr_t *pdrMgr)
// {
//   int sat, i;

//   //Add blocking clause for current target cube
//   if(pdrMgr->targetCube != NULL){

//     Ddi_Clause_t *blockingClause = Ddi_ClauseNegLits(pdrMgr->targetCube);
//     Ddi_SatSolverAddClause(pdrMgr->targetEnumSolver, blockingClause);
//     Ddi_ClauseFree(pdrMgr->targetCube);
//     Ddi_ClauseFree(pdrMgr->ddiPdr->actTarget);
//     pdrMgr->targetCube = NULL;
//     pdrMgr->ddiPdr->actTarget = NULL;
//     pdrMgr->cubes++;
//     fprintf(stdout, "Blocked clauses: %d\n", pdrMgr->cubes);
//   }

//   //Extract new target cube
//   sat = Ddi_SatSolve(pdrMgr->targetEnumSolver, pdrMgr->actTargetTot,-1);
//   if(sat){

//     //Extend new target cube
//     Ddi_Clause_t *cube = Ddi_SatSolverGetCexCube(pdrMgr->targetEnumSolver, pdrMgr->ddiPdr->targetSuppClause);
//     Ddi_Clause_t *cubeMin = pdrExtendCex(pdrMgr,pdrMgr->targetEnumSolver, cube, NULL /*Target*/, 2 /*enTernary*/);
//     //Ddi_ClauseFree(cube);

//     // fprintf(stdout, "Enumerated cube: (");
//     // for (i=0; i < cubeMin->nLits; i++){
//     //   if(i == cubeMin->nLits -1) fprintf(stdout, "%d", cubeMin->lits[i]);
//     //   else fprintf(stdout, "%d, ", cubeMin->lits[i]);
//     // }
//     // fprintf(stdout, ")\n");

//     //Set new target cube
//     pdrMgr->targetCube = cubeMin;

//     //Set target cube as act target
//     pdrMgr->ddiPdr->actTarget = Ddi_ClauseDup(cubeMin);

//   } else {

//     //Target enumeration complete
//     return 1;
//   }

//   //Continue with verification
//   return 0;
// }

static int
pdrEnumerateTargetCubes(
  TravPdrMgr_t * pdrMgr
)
{
  int sat, i;

  //First?
  if (pdrMgr->targetCube == NULL) {

    //Get first target cube
    fprintf(stdout, "Proving a random cube from the target.\n");
    sat = Ddi_SatSolve(pdrMgr->targetEnumSolver, pdrMgr->actTargetTot, -1);
    Pdtutil_Assert(sat, "Empty target");
    Ddi_Clause_t *cube = Ddi_SatSolverGetCexCube(pdrMgr->targetEnumSolver,
      pdrMgr->ddiPdr->targetSuppClause);
    Ddi_Clause_t *cubeMin =
      pdrExtendCex(pdrMgr, pdrMgr->targetEnumSolver, cube, NULL /*Target */ ,
      2 /*enTernary */ );

    //Set new target cube
    pdrMgr->targetCube = cubeMin;

    //Set target cube as act target
    pdrMgr->ddiPdr->actTarget = Ddi_ClauseDup(cubeMin);

    //Continue with verification
    return 1;

  } else {

    //Free current target cube
    Ddi_ClauseFree(pdrMgr->targetCube);
    Ddi_ClauseFree(pdrMgr->ddiPdr->actTarget);
    pdrMgr->targetCube = NULL;
    pdrMgr->ddiPdr->actTarget = NULL;
    pdrMgr->cubes++;
  }

  //Extract new target cube
  for (i = 1; i < pdrMgr->nTimeFrames - 1; i++) {

    //Check if time frame intersect target
    //sat = Ddi_SatSolve(pdrMgr->timeFrames[pdrMgr->nTimeFrames - 1].solver, pdrMgr->actTargetTot,-1);
    sat = Ddi_SatSolve(pdrMgr->timeFrames[i].solver, pdrMgr->actTargetTot, -1);
    if (sat) {

      //Extend new target cube
      Ddi_Clause_t *cube =
        Ddi_SatSolverGetCexCube(pdrMgr->timeFrames[i].solver,
        pdrMgr->ddiPdr->targetSuppClause);
      Ddi_Clause_t *cubeMin =
        pdrExtendCex(pdrMgr, pdrMgr->timeFrames[i].solver, cube,
        NULL /*Target */ , 2 /*enTernary */ );

      //Set new target cube
      pdrMgr->targetCube = cubeMin;
      fprintf(stdout, "New cube extracted: %d on TF %d\n", cubeMin->nLits, i);

      //Check
      sat = Ddi_SatSolve(pdrMgr->timeFrames[i].solver, cubeMin, -1);
      Pdtutil_Assert(sat, "");

      //Set target cube as act target
      pdrMgr->ddiPdr->actTarget = Ddi_ClauseDup(cubeMin);

      //Continue with verification
      return i;
    }
  }

  //Target enumeration complete
  return 0;
}




/**Function*******************************************************************
  Synopsis    [check clause generalization against init cube]
  Description [check clause generalization against init cube. At least one
literal different fron init is required. Otherwise literals are picked by
cube (with complementation) for refinement. Num of different lits is returned.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
pdrChkGenWithInitAndRefine(
  TravPdrMgr_t * pdrMgr,
  Ddi_Clause_t * clMin,
  Ddi_Clause_t * cube,
  int checkFull
)
{
  int i;
  int clMinOK = 0;

  if (pdrMgr->ddiPdr->initLits == NULL)
    return -1;

  //Compute how many literals of the clause assure base of induction
  for (i = 0; i < Ddi_ClauseNumLits(clMin); i++) {
    int lit = clMin->lits[i];
    int pos = (lit > 0) ? 1 : 0;
    int var = abs(lit);

    Pdtutil_Assert(var % 2 == 1, "wrong cube var");
    var = var / 2;
    if (pdrMgr->ddiPdr->initLits[var] == pos) {
      /* if same phase as init, cube is out of init */
      /* found difference with init - clMin is OK */
      clMinOK++;
      if (!checkFull)
        break;
    }
  }

  //If no literal in the clause assure base of induction clause is augmented until there is at least one
  if (clMinOK == 0) {
    /* look for a literal making the difference */
    for (i = 0; i < Ddi_ClauseNumLits(cube); i++) {
      int lit = cube->lits[i];
      int pos = (lit > 0) ? 0 : 1;  // complemented to take into account - (val 2)
      int var = abs(lit);

      Pdtutil_Assert(var % 2 == 1, "wrong cube var");
      var = var / 2;
      if (pdrMgr->ddiPdr->initLits[var] == pos) {
        /* if phase in cube differs from init, cube is out of init */
        /* found difference with init - add literal */
        DdiClauseAddLiteral(clMin, -lit);
        clMinOK++;
        Ddi_ClauseSort(clMin);
        break;
      }
    }
  }
  //Pdtutil_Assert(clMinOK>0,"void clause");

  return clMinOK;
}

/**Function*******************************************************************
  Synopsis    [generalize clause using analyze final & mode aggressive checks]
  Description [generalize clause using analyze final & mode aggressive checks.
generalizeLevel is used to select effort -
<=0: just analyzeFinal; 1: sat based generalization attempted just if
analyzeFinal improved; >=2: always try sat based generalization]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Clause_t *
pdrGeneralizeClause(
  TravPdrMgr_t * pdrMgr,
  int k,
  Ddi_Clause_t * cube,
  int generalizeLevel
)
{
  //Utility variables
  int kMax = pdrMgr->nTimeFrames - 1;
  Ddi_Clause_t *clMin, *core;
  TravPdrTimeFrame_t *tf = &(pdrMgr->timeFrames[k]);
  int clMinOK = 0;
  int anFinalGain = 0;
  int enGenCore = 0;

  //Settings variables
  int enSatMin = 1;
  int useInduction = 1;

  //Counter variables
  long initTime, genTime;
  static int nc = 0;

  nc++;
  initTime = util_cpu_time();

  //Reduce inductive relative clause with unsatisfiability core
  core = Ddi_SatSolverFinal(tf->solver, 2 * pdrMgr->ddiPdr->nState);
  clMin = Ddi_ClauseRemapVars(core, -1);


  if (pdrChkGenWithInitAndRefine(pdrMgr, clMin, cube, 0) == 0) {
    printf("cube intersects init state\n");
  }
  Pdtutil_Assert(1 || pdrChkGenWithInitAndRefine(pdrMgr, clMin, cube, 0) != 0,
    "cube intersects init state");

  //If generalization level <=0 and no reduction obtained stop generalization
  anFinalGain = cube->nLits - clMin->nLits;

  if (generalizeLevel <= 1 && clMin->nLits == cube->nLits) {
    Ddi_ClauseSort(clMin);
    return clMin;
  }
  //Enable sat based generalization only if level > 1 or level is 1 and a reduction was obtained with analyze final
  enSatMin = (generalizeLevel > 1 || (generalizeLevel == 1
      && clMin->nLits < cube->nLits));

  //Manage generalize without induction
  useInduction = (k > 0) && (generalizeLevel > 2);
  if (0 && k > 0 && !useInduction) {
    Ddi_SatSolverAddClause(tf->solver, clMin);
    if (tf->hasSolver2)
      Ddi_SatSolverAddClause(tf->solver2, clMin);
  }
  //If init lits are available they are used to assure base of induction
  Pdtutil_Assert(clMin->nLits > 0, "void clause");
  clMinOK = pdrChkGenWithInitAndRefine(pdrMgr, clMin, cube, 1);

  //Try removing lits
  Pdtutil_Assert(clMin->nLits > 0, "void clause");
  if (enSatMin && clMin->nLits > 1) {
    int i, j, leftLits = clMin->nLits;
    Ddi_Clause_t *cubeMin = Ddi_ClauseNegLits(clMin);

    //Process clause literals in order of activity
    int *varOrder = Ddi_PdrClauseActivityOrder(pdrMgr->ddiPdr, cubeMin);

    for (i = j = 0; i < Ddi_ClauseNumLits(cubeMin); i++) {
      Ddi_Clause_t *pred = NULL, *cubeAux, *core = NULL;
      int ii = varOrder[i];
      int lit = cubeMin->lits[ii];
      int pos = (lit > 0);
      int var = abs(lit) / 2;
      int incrClminOK = 0;
      int initOK = 1;

      if (clMin->lits[ii] == 0) {
        continue;
      }

      if (leftLits == 1) {
        j++;
        continue;
      }
      //Skip last literal that protect base of induction
      if (clMinOK > 0 && pdrMgr->ddiPdr->initLits[var] != pos) {
        if (clMinOK == 1) {
          j++;
          continue;
        }
        incrClminOK = 1;
        clMinOK--;
      }
      pdrMgr->settings.step++;

      //If every literal but one has been removed skip

      //Remove literal
      cubeMin->lits[ii] = 0;
      cubeAux = Ddi_ClauseCompact(cubeMin);

      //If base of induction is not assured by init lits, check it explicitly (Q6)
      if (pdrMgr->ddiPdr->initLits == NULL) {
        int dec = pdrMgr->timeFrames[0].solver->S->stats.decisions;

        initOK = !Ddi_SatSolve(pdrMgr->timeFrames[0].solver, cubeAux, -1);
        pdrMgr->stats.split[7] +=
          (pdrMgr->timeFrames[0].solver->S->stats.decisions - dec);
      }
      //If base of induction holds check if inductive step holds for reduced clause
      if (initOK) {
        Pdtutil_Assert(Ddi_SatSolverOkay(pdrMgr->timeFrames[k].solver),
          "solver is not okay");
        pred =
          pdrInductivePredecessor(pdrMgr, k, cubeAux, enGenCore ? &core : NULL,
          0, 1, useInduction, PDR_GENERALIZE_MODE);
      }
      //If base of induction or inductive step doesn't hold restore literal and continue
      if (!initOK || pred != NULL) {
        cubeMin->lits[ii] = lit;
        j++;
        if (pred != NULL)
          Ddi_ClauseFree(pred);
        if (incrClminOK)
          clMinOK++;
        Pdtutil_Assert(core == NULL, "core clause not allowed here");
      } else {
        //Otherwise remove literal from clause and continue
        clMin->lits[ii] = 0;
        leftLits--;
        if (core != NULL) {
          int i1, ii1, jj;

          Pdtutil_Assert(core->nLits < cubeAux->nLits, "reduced core needed");
#if 1
          for (jj = 0; jj < core->nLits; jj++) {
            Ddi_PdrVarWriteMark(pdrMgr->ddiPdr, core->lits[jj], 1);
          }
          for (i1 = i + 1; i1 < Ddi_ClauseNumLits(cubeMin); i1++) {
            int ii1 = varOrder[i1];
            int lit1 = cubeMin->lits[ii1];
            int pos1 = (lit1 > 0);
            int var1 = abs(lit1) / 2;
            int myInitOK = 1;

            if (lit1 == 0)
              continue;
            if (Ddi_PdrVarReadMark(pdrMgr->ddiPdr, lit1) != 0)
              continue;
            // syntactic check with init state
            if (pdrMgr->ddiPdr->initLits == NULL) {
              int save = cubeMin->lits[ii1];

              cubeMin->lits[ii1] = 0;
              Ddi_Clause_t *cubeAux1 = Ddi_ClauseCompact(cubeMin);
              int dec = pdrMgr->timeFrames[0].solver->S->stats.decisions;

              myInitOK =
                !Ddi_SatSolve(pdrMgr->timeFrames[0].solver, cubeAux1, -1);
              pdrMgr->stats.split[7] +=
                (pdrMgr->timeFrames[0].solver->S->stats.decisions - dec);
              Ddi_ClauseFree(cubeAux1);
              cubeMin->lits[ii1] = save;
            } else {
              if (pdrMgr->ddiPdr->initLits[var1] != pos1) {
                if (clMinOK == 1) {
                  continue;
                }
                Pdtutil_Assert(clMinOK > 1, "problem with clMinOK");
                clMinOK--;
              }
            }
            if (myInitOK) {
              cubeMin->lits[ii1] = 0;
            }
          }

          for (jj = 0; jj < core->nLits; jj++) {
            Ddi_PdrVarWriteMark(pdrMgr->ddiPdr, core->lits[jj], 0);
          }
#endif
          Ddi_ClauseFree(core);
        }
      }

      Ddi_ClauseFree(cubeAux);
    }

    Pdtutil_Free(varOrder);

    //Compact clause and free temp clauses
    if (j < clMin->nLits) {
      Ddi_Clause_t *clAux = Ddi_ClauseCompact(clMin);

      PDR_DBG(printf("custom reduce: %d -> %d\n", clMin->nLits, j));
      Pdtutil_Assert(clAux->nLits == j, "error compacting clause");
      Ddi_ClauseFree(clMin);
      clMin = clAux;
    }
    Ddi_ClauseFree(cubeMin);
  }
  //Debug
#if PDR_DBG1
  if (0) {
    Ddi_Clause_t *cubeMin = Ddi_ClauseNegLits(clMin);

    Pdtutil_Assert(pdrInductivePredecessor(pdrMgr, k, cubeMin, NULL, 0, 0, 1,
        PDR_DEBUG_MODE) == NULL, "error in cube minimization");
    Ddi_ClauseFree(cubeMin);
  }
#endif

  //Check base of induction again
  //TOASK: why?
  clMinOK = 0;
  /* check init state */
  /* clMin is complemented w.r.t. cube !!! */

  //Assure base of induction with init lits
  if (pdrMgr->ddiPdr->initLits != NULL) {
    int i;

    for (i = 0; i < Ddi_ClauseNumLits(clMin); i++) {
      int lit = clMin->lits[i];
      int pos = (lit > 0);
      int var = abs(lit);

      Pdtutil_Assert(var % 2 == 1, "wrong cube var");
      var = var / 2;
      if (pdrMgr->ddiPdr->initLits[var] == pos) {
        /* if same phase as init, cube is out of init */
        /* found difference with init - clMin is OK */
        clMinOK = 1;
        break;
      }
    }
    if (!clMinOK) {
      int found = 0;

      for (i = 0; i < Ddi_ClauseNumLits(cube); i++) {
        int lit = cube->lits[i];
        int pos = (lit > 0);
        int var = abs(lit);

        Pdtutil_Assert(var % 2 == 1, "wrong cube var");
        var = var / 2;
        if (pdrMgr->ddiPdr->initLits[var] != pos) {
          /* found difference with init - update cubeMin */
          DdiClauseAddLiteral(clMin, -lit);
          found = 1;
          /* reset last free activation literals */
          break;
        }
      }
      Pdtutil_Assert(found, "error with init state");
    }
  } else {

    //Assure base of induction by SAT calls (reintegrate literals from initial clause until base of induction holds again) (Q6)
    int initOK, i;

    // GpC: bug fixed - 21/4/2015 - 
    // clMin is in next state -> clMin, 1, 1, ... (was 1, 0, ...) 
    initOK =
      !Ddi_SatSolveCustomized(pdrMgr->timeFrames[0].solver, clMin, 1, 0, -1);
    while (!initOK) {
      int progress = 0;

      for (i = 0; !initOK && i < Ddi_ClauseNumLits(cube); i++) {
        int lit = cube->lits[i];
        int var = abs(lit);

        Pdtutil_Assert(var % 2 == 1, "wrong cube var");
        if (!Ddi_SatCexIncludedInLit(pdrMgr->timeFrames[0].solver, lit)) {
          /* refine clMin filtering out cex */
          DdiClauseAddLiteral(clMin, -lit);
	  // GpC: bug fixed - 21/4/2015 - 
	  // clMin is in next state -> clMin, 1, 1, ... (was 1, 0, ...) 
          initOK =
            !Ddi_SatSolveCustomized(pdrMgr->timeFrames[0].solver, clMin, 1, 0,
            -1);
          /* reset last free activation literals */
          progress = 1;
          break;
        }
      }
      Pdtutil_Assert(progress, "problem refining cube w.r.t. init");
    }
  }

  Ddi_ClauseSort(clMin);

  //Debug
#if PDR_DBG1
  Ddi_Clause_t *cubeMin = Ddi_ClauseNegLits(clMin);

  Pdtutil_Assert(pdrInductivePredecessor(pdrMgr, k, cubeMin, NULL, 0, 0, 1,
      PDR_DEBUG_MODE) == NULL, "error in cube minimization");
  Ddi_ClauseFree(cubeMin);
#endif
  genTime = util_cpu_time() - initTime;
  pdrMgr->stats.generalizeTime += (float)genTime / 1000.0;

  return clMin;
}



/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
pdrCheckSolvers(
  TravPdrMgr_t * pdrMgr
)
{
  int j, kMax = pdrMgr->nTimeFrames;

  for (j = 0; j < kMax; j++) {
    Pdtutil_Assert(Ddi_SatSolverOkay(pdrMgr->timeFrames[j].solver),
      "SOLVER is not OK");
  }
  if (0 && (kMax > 1))
    pdrCheckSolverCustom(pdrMgr, 1);

}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
pdrCheckSolverCustom(
  TravPdrMgr_t * pdrMgr,
  int k
)
{
  int lit = 73;
  Ddi_Clause_t *assume = Ddi_ClauseAlloc(0, 1);

  DdiClauseAddLiteral(assume, lit);

  Pdtutil_Assert(Ddi_SatSolverOkay(pdrMgr->timeFrames[k].solver),
    "SOLVER is not OK");
  Pdtutil_Assert(Ddi_SatSolve(pdrMgr->timeFrames[k].solver, assume, -1),
    "SOLVER is not SAT");
}

/**Function*******************************************************************
  Synopsis    [ Compose a clause array with the clauses of the logic cone for the cube cl.
                Use Plaisted-Greenbaum encoding to reduce the size of the cone loaded. ]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_ClauseArray_t *
pdrFindOrAddIncrementalClausesWithPg(
  TravPdrTimeFrame_t * tf,
  Ddi_PdrMgr_t * ddiPdr,
  Ddi_Clause_t * cl,
  int usePsClause,
  int useSolver2
)
{
  //Utility variables
  int i, j;
  Ddi_Clause_t *clNew;
  Ddi_NetlistClausesInfo_t *netCl = &(ddiPdr->trNetlist);
  int *latchLoaded =
    useSolver2 ? tf->trClauseMap.latchLoaded2 : tf->trClauseMap.latchLoaded;
  Ddi_ClauseArray_t *outClauseArray = Ddi_ClauseArrayAlloc(0);

  //Load cone for every next state variable of the cube
  for (i = 0; i < Ddi_ClauseNumLits(cl); i++) {

    //Get latch
    int lit = cl->lits[i];
    int var = DdiPdrLit2Var(lit);
    int pos = lit > 0;          //Assumed polarity
    int id =
      usePsClause ? DdiPdrVarPsIndex(ddiPdr, var) : DdiPdrVarNsIndex(ddiPdr,
      var);

    Pdtutil_Assert(id >= 0
      && id < tf->trClauseMap.nLatch,
      "wrong var in incremental clause findOrAdd");

    //Check if latch already loaded
    if (latchLoaded[id] == PDR_LOADED_BOTH_POLARITIES) {
      continue;
    } else if (latchLoaded[id] == PDR_NOT_LOADED) {
      latchLoaded[id] =
        pos ? PDR_LOADED_POS_POLARITY : PDR_LOADED_NEG_POLARITY;
    } else if ((latchLoaded[id] == PDR_LOADED_POS_POLARITY && !pos)
      || (latchLoaded[id] == PDR_LOADED_NEG_POLARITY && pos)) {
      latchLoaded[id] = PDR_LOADED_BOTH_POLARITIES;
    } else {
      continue;
    }

    //If latch's input is a constant or a variable just load linking clauses
    if (netCl->roots[id] == 0) {
      int clId, rootId = netCl->nGate + id;

      Pdtutil_Assert(netCl->clauseMapNum[rootId] <= 2, "error in clause load");
      clId = netCl->clauseMapIndex[rootId];
      if (netCl->clauseMapNum[rootId] == 1) {

        //Constant input --> 1 link clause
        clNew = ddiPdr->trClauses->clauses[clId];
        Ddi_ClauseArrayPush(outClauseArray, clNew);
        latchLoaded[id] = PDR_LOADED_BOTH_POLARITIES;   //Constant input latch has just one polarity loadable
      } else if (netCl->clauseMapNum[rootId] == 2) {

        //Variable input --> load the link clause that is not satisfied by assumed latch polarity
        clNew = ddiPdr->trClauses->clauses[clId];
        for (j = 0; j < clNew->nLits; j++) {
          if (abs(clNew->lits[j]) == (usePsClause ? var + 1 : var)) {
            if ((clNew->lits[j] > 0) == pos) {
              clNew = ddiPdr->trClauses->clauses[clId + 1];
            }
            Ddi_ClauseArrayPush(outClauseArray, clNew);
            break;
          }
        }
      }
    } else {
      //If latch's input is a logic function load logic cone and then linking clauses
      int gateId, clId, rootId = netCl->nGate + id;

      //Get root gate of the logic function
      int root = netCl->roots[id];
      int isRootInv = root < 0;

      gateId = root < 0 ? (-root) - 1 : root - 1;

      //Determine gate initial polarity flag
      int isOdd = pos ? isRootInv : !isRootInv;

      //Load cone
      int rootVar = netCl->gateId2cnfId[gateId];

      pdrFindOrAddIncrementalClausesWithPgRecur(tf, ddiPdr, isOdd, gateId,
        outClauseArray, rootVar, useSolver2, 0, -1);

      //Load latch linking clauses
      if (netCl->clauseMapNum[rootId] > 0) {
        Pdtutil_Assert(netCl->clauseMapNum[rootId] == 2,
          "error in clause load");
        clId = netCl->clauseMapIndex[rootId];

        //Load the link clause that is not satisfied by assumed latch polarity
        clNew = ddiPdr->trClauses->clauses[clId];
        for (j = 0; j < clNew->nLits; j++) {
          if (abs(clNew->lits[j]) == (usePsClause ? var + 1 : var)) {
            if ((clNew->lits[j] > 0) == pos) {
              clNew = ddiPdr->trClauses->clauses[clId + 1];
            }
            Ddi_ClauseArrayPush(outClauseArray, clNew);
            break;
          }
        }
      }
    }
  }

  //Return logic cone clause array
  return outClauseArray;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int                      //M
pdrFindOrAddIncrementalClausesWithPgRecur(
  TravPdrTimeFrame_t * tf,
  Ddi_PdrMgr_t * ddiPdr,
  int isOdd,                    //Current path polarity flag
  int gateId,                   //Current gate id
  Ddi_ClauseArray_t * clausesToLoad,    //Output array
  int ldrVar,                   //Current cnf id var
  int useSolver2,               //Solver switch
  int level,                    //Recursion depth
  int ldrGateId                 //-1 if current gate is a leader gate, a gate id if current gate is an internal one
)
{
  //Utility variables
  int rightCounter = 0;
  int leftCounter = 0;
  int totAddedCounter;
  int i, j;
  Ddi_NetlistClausesInfo_t *netCl = &(ddiPdr->trNetlist);
  Ddi_Mgr_t *ddm = ddiPdr->ddiMgr;
  bAig_Manager_t *bmgr = Ddi_MgrReadMgrBaig(ddm);
  int *aigClauseMap =
    useSolver2 ? tf->trClauseMap.aigClauseMap2 : tf->trClauseMap.aigClauseMap;

  //Get current leader variable
  int isInternal = (netCl->gateId2cnfId[gateId] == -1) ? 1 : 0;
  int rootVar = ldrVar;

  if (!isInternal)
    rootVar = netCl->gateId2cnfId[gateId];

  //Compute min depths for touched leader gates
  if (netCl->clauseMapNum[gateId] > 0 && netCl->gateId2cnfId[gateId] != -1) {
    if (tf->trClauseMap.minDepth[gateId] == -1
      || tf->trClauseMap.minDepth[gateId] > level) {
      tf->trClauseMap.minDepth[gateId] = level;
      if (tf->trClauseMap.minLevel == -1 || tf->trClauseMap.minLevel > level) {
        tf->trClauseMap.minLevel = level;
      }
    }
  }
  //Check if gate is terminal
  if (netCl->fiIds[0][gateId] == 0) {
    Pdtutil_Assert(netCl->fiIds[1][gateId] == 0, "wrong netlist teminal");
    return 0;
  }
  //Internal node
  if (ldrGateId != -1) {

    //Check if it's a cut node
    int k;
    int isCutPoint = 0;

    if (netCl->leaders[gateId] != NULL) {
      for (k = 0; k < Pdtutil_IntegerArrayNum(netCl->leaders[gateId]); k++) {
        int ldr = Pdtutil_IntegerArrayRead(netCl->leaders[gateId], k);

        if (ldr == ldrGateId) {
          isCutPoint = 1;       //Found cut point
          break;
        }
      }
    }
    //If node is not a cut point recurr without loading clauses (just to propagate polarity paths internal to current leader gate)
    if (!isCutPoint) {
      int ir = netCl->fiIds[0][gateId];
      int il = netCl->fiIds[1][gateId];
      int rInv = ir < 0;
      int lInv = il < 0;
      int rGateId = ir < 0 ? (-ir) - 1 : ir - 1;
      int lGateId = il < 0 ? (-il) - 1 : il - 1;

      Pdtutil_Assert(rGateId >= 0
        && lGateId >= 0, "error recurring in clause map");
      int rOdd = rInv ? !isOdd : isOdd;
      int lOdd = lInv ? !isOdd : isOdd;
      int rRootVar = rRootVar = netCl->gateId2cnfId[rGateId];;
      int lRootVar = lRootVar = netCl->gateId2cnfId[lGateId];

      pdrFindOrAddIncrementalClausesWithPgRecur(tf, ddiPdr, rOdd, rGateId,
        clausesToLoad, rRootVar, useSolver2, level + 1, ldrGateId);
      pdrFindOrAddIncrementalClausesWithPgRecur(tf, ddiPdr, lOdd, lGateId,
        clausesToLoad, lRootVar, useSolver2, level + 1, ldrGateId);
      return 0;
    }
  }
  //If gate is a leader or a cut point current's path polarity must be loaded
  int polarityToLoad = !isOdd;

  //Check if polarity is loaded
  Pdtutil_Assert(gateId >= 0 && gateId < netCl->nGate, "wrong gate id");
  if (aigClauseMap[gateId] == PDR_LOADED_BOTH_POLARITIES) {
    return 0;
  } else if (aigClauseMap[gateId] == PDR_NOT_LOADED) {
    aigClauseMap[gateId] =
      polarityToLoad ? PDR_LOADED_POS_POLARITY : PDR_LOADED_NEG_POLARITY;
  } else if ((aigClauseMap[gateId] == PDR_LOADED_POS_POLARITY
      && polarityToLoad == 0)
    || (aigClauseMap[gateId] == PDR_LOADED_NEG_POLARITY
      && polarityToLoad == 1)) {
    aigClauseMap[gateId] = PDR_LOADED_BOTH_POLARITIES;
  } else {
    return 0;
  }

  //Load gate polarity

  //Se il gate è stato codificato come un ITE devo introdurre una negazione ulteriore sugli input
  if (0 && netCl->isRoot[gateId] == 4) {


    //Prendo i gate id dei figli di secondo livello
    int ir = netCl->fiIds[0][gateId];
    int il = netCl->fiIds[1][gateId];
    int rGateId = abs(ir) - 1;
    int lGateId = abs(il) - 1;
    int irr = netCl->fiIds[0][rGateId];
    int irl = netCl->fiIds[1][rGateId];
    int ilr = netCl->fiIds[0][lGateId];
    int ill = netCl->fiIds[1][lGateId];

    //Determino la selezione e gli operandi
    int sel, a, b;

    if (irr == -ilr) {
      sel = ilr;
      a = ill;
      b = irl;
    } else if (irr == -ill) {
      sel = ill;
      a = ilr;
      b = irl;
    } else if (irl == -ilr) {
      sel = ilr;
      a = ill;
      b = irr;
    } else if (irl == -ill) {
      sel = ill;
      a = ilr;
      b = irr;
    } else {
      Pdtutil_Assert(0, "Wrong ITE recognized");
    }

    //Determino le polarità
    int selInv = sel < 0;
    int aInv = a < 0;
    int bInv = b < 0;
    int selGateId = abs(sel) - 1;
    int aGateId = abs(a) - 1;
    int bGateId = abs(b) - 1;

    Pdtutil_Assert(netCl->gateId2cnfId[selGateId] != -1
      && netCl->gateId2cnfId[aGateId] != -1
      && netCl->gateId2cnfId[bGateId] != -1, "Wrong ITE recognized");

    //Determino i flag di parità/disparità
    int selOdd = selInv ? !isOdd : isOdd;
    int aOdd = aInv ? isOdd : !isOdd;   //Questi li devo invertire perchè l'ite presuppone al suo interno percorsi dispari per gli ingressi
    int bOdd = bInv ? isOdd : !isOdd;
    int selRootVar = netCl->gateId2cnfId[selGateId];
    int aRootVar = netCl->gateId2cnfId[aGateId];
    int bRootVar = netCl->gateId2cnfId[bGateId];

    //Per la selezione devo considerare tutte e due le fasi perchè una mi serve ad impedire inconsistenze quando viene scelto l'ingresso a, l'altra quando viene scelto l'ingresso b.
    int selCounter, aCounter, bCounter;

    if (ddiPdr->doPostOrderClauseLoad) {
      selCounter = pdrFindOrAddIncrementalClausesWithPgRecur(tf, ddiPdr, selOdd, selGateId, clausesToLoad, selRootVar, useSolver2, level + 1, -1) + pdrFindOrAddIncrementalClausesWithPgRecur(tf, ddiPdr, !selOdd, selGateId, clausesToLoad, selRootVar, useSolver2, level + 1, -1);    //M
      aCounter = pdrFindOrAddIncrementalClausesWithPgRecur(tf, ddiPdr, aOdd, aGateId, clausesToLoad, aRootVar, useSolver2, level + 1, -1);  //M
      bCounter = pdrFindOrAddIncrementalClausesWithPgRecur(tf, ddiPdr, bOdd, bGateId, clausesToLoad, bRootVar, useSolver2, level + 1, -1);  //M
    }
    //Se il gate corrente è un leader carico le clausole
    if (netCl->clauseMapNum[gateId] != 0) {

      //Prendo le clausole relative alla variabile
      int clStartId = netCl->clauseMapIndex[gateId];
      Ddi_ClauseArray_t *gateClauses =
        Ddi_ClauseArrayAlloc(netCl->clauseMapNum[gateId]);
      for (i = 0; i < netCl->clauseMapNum[gateId]; i++) {
        Ddi_Clause_t *clNew = ddiPdr->trClauses->clauses[clStartId + i];

        Ddi_ClauseArrayPush(gateClauses, clNew);
      }

      //Carico le clausole
      for (i = 0; i < Ddi_ClauseArrayNum(gateClauses); i++) {
        Ddi_Clause_t *clNew = gateClauses->clauses[i];

        //Aggiorno le su cui continuare il caricamento e determino la polarità della f sulla clausola corrente
        int satisfyingPolarity = -1;

        for (j = 0; j < Ddi_ClauseNumLits(clNew); j++) {
          int newVar = DdiPdrLit2Var(clNew->lits[j]);

          if (newVar == rootVar)
            satisfyingPolarity = (clNew->lits[j] < 0) ? 0 : 1;
        }
        Pdtutil_Assert(satisfyingPolarity != -1,
          "There's a clause that don't correlates inputs with outputs");

        //Aggiungo le clausole che non risultano soddisfatte dall'assunzione di polarità
        if (polarityToLoad != satisfyingPolarity) {
          //TODO: qui c'è spazio di miglioramento considerando le riduzioni
          Ddi_ClauseArrayPush(clausesToLoad, clNew);
          totAddedCounter++;
        }
      }

      Ddi_ClauseArrayFree(gateClauses);
    }

    if (!ddiPdr->doPostOrderClauseLoad) {
      selCounter = pdrFindOrAddIncrementalClausesWithPgRecur(tf, ddiPdr, selOdd, selGateId, clausesToLoad, selRootVar, useSolver2, level + 1, -1) + pdrFindOrAddIncrementalClausesWithPgRecur(tf, ddiPdr, !selOdd, selGateId, clausesToLoad, selRootVar, useSolver2, level + 1, -1);    //M
      aCounter = pdrFindOrAddIncrementalClausesWithPgRecur(tf, ddiPdr, aOdd, aGateId, clausesToLoad, aRootVar, useSolver2, level + 1, -1);  //M
      bCounter = pdrFindOrAddIncrementalClausesWithPgRecur(tf, ddiPdr, bOdd, bGateId, clausesToLoad, bRootVar, useSolver2, level + 1, -1);  //M
    }

    totAddedCounter =
      netCl->clauseMapNum[gateId] + selCounter + aCounter + bCounter;
  }                             /*if(netCl->isRoot[gateId] == 3){
                                   //Caricamento di uno XOR

                                   //Prendo i gate id dei figli di secondo livello
                                   int ir = netCl->fiIds[0][gateId];
                                   int il = netCl->fiIds[1][gateId];
                                   int rGateId = abs(ir) - 1;
                                   int lGateId = abs(il) - 1;
                                   int irr = netCl->fiIds[0][rGateId];
                                   int irl = netCl->fiIds[1][rGateId];
                                   int ilr = netCl->fiIds[0][lGateId];
                                   int ill = netCl->fiIds[1][lGateId];

                                   int a, b;
                                   if(irr == -ilr && irl == -ill){
                                   a = ilr;
                                   b = ill;
                                   } else{
                                   Pdtutil_Assert(0, "Wrong XOR recognized");
                                   }

                                   //Determino le polarità e i gate id
                                   int aInv = a < 0;
                                   int bInv = b < 0;
                                   int aGateId = abs(a) - 1;
                                   int bGateId = abs(b) -1;

                                   Pdtutil_Assert(netCl->gateId2cnfId[aGateId] != -1 && netCl->gateId2cnfId[bGateId] != -1, "Wrong XOR recognized");

                                   //Determino i flag di parità/disparità
                                   int aOdd = aInv ? !isOdd : isOdd;
                                   int bOdd = bInv ? !isOdd : isOdd;
                                   int aRootVar = netCl->gateId2cnfId[aGateId];
                                   int bRootVar = netCl->gateId2cnfId[bGateId];

                                   //Si caricano entrambe le fasi per le due variabili (all'interno dello XOR infatti esiste sempre sia un percorso con un numero pari sia un
                                   //percorso con un numero dispari di inverter).
                                   int aCounter  = pdrFindOrAddIncrementalClausesRecur(tf, ddiPdr, aOdd, aGateId, clausesToLoad, aRootVar, useSolver2) +
                                   pdrFindOrAddIncrementalClausesRecur(tf, ddiPdr, !aOdd, aGateId, clausesToLoad, aRootVar, useSolver2); //M
                                   int bCounter  = pdrFindOrAddIncrementalClausesRecur(tf, ddiPdr, bOdd, bGateId, clausesToLoad, bRootVar, useSolver2) +
                                   pdrFindOrAddIncrementalClausesRecur(tf, ddiPdr, !bOdd, bGateId, clausesToLoad, bRootVar, useSolver2); //M

                                   totAddedCounter = netCl->clauseMapNum[gateId] + aCounter + bCounter;
                                   } */
  else {

    //Compute recurring parameters
    Pdtutil_Assert(netCl->clauseMapNum[gateId] >= 0, "wrong map num");
    int ir = netCl->fiIds[0][gateId];
    int il = netCl->fiIds[1][gateId];
    int rInv = ir < 0;
    int lInv = il < 0;
    int rGateId = ir < 0 ? (-ir) - 1 : ir - 1;
    int lGateId = il < 0 ? (-il) - 1 : il - 1;

    Pdtutil_Assert(rGateId >= 0
      && lGateId >= 0, "error recurring in clause map");
    int rOdd = rInv ? !isOdd : isOdd;
    int lOdd = lInv ? !isOdd : isOdd;
    int rRootVar = netCl->gateId2cnfId[rGateId];
    int lRootVar = netCl->gateId2cnfId[lGateId];

    //If post order load childs' cones before gate's clauses
    if (ddiPdr->doPostOrderClauseLoad) {
      pdrFindOrAddIncrementalClausesWithPgRecur(tf, ddiPdr, rOdd, rGateId,
        clausesToLoad, rRootVar, useSolver2, level + 1, gateId);
      pdrFindOrAddIncrementalClausesWithPgRecur(tf, ddiPdr, lOdd, lGateId,
        clausesToLoad, lRootVar, useSolver2, level + 1, gateId);
    }
    //Load current clauses that are not satisfied by current's path polarity
    if (netCl->clauseMapNum[gateId] != 0) {
      int clStartId = netCl->clauseMapIndex[gateId];
      Ddi_ClauseArray_t *gateClauses =
        Ddi_ClauseArrayAlloc(netCl->clauseMapNum[gateId]);
      for (i = 0; i < netCl->clauseMapNum[gateId]; i++) {
        Ddi_Clause_t *clNew = ddiPdr->trClauses->clauses[clStartId + i];

        Ddi_ClauseArrayPush(gateClauses, clNew);
      }
      for (i = 0; i < Ddi_ClauseArrayNum(gateClauses); i++) {
        Ddi_Clause_t *clNew = gateClauses->clauses[i];
        int satisfyingPolarity = -1;

        for (j = 0; j < Ddi_ClauseNumLits(clNew); j++) {
          int newVar = DdiPdrLit2Var(clNew->lits[j]);

          if (newVar == rootVar) {
            satisfyingPolarity = (clNew->lits[j] < 0) ? 0 : 1;
            break;
          }
        }
        Pdtutil_Assert(satisfyingPolarity != -1, "Problem in encoding");
        if (polarityToLoad != satisfyingPolarity) {
          Ddi_ClauseArrayPush(clausesToLoad, clNew);
        }
      }
      Ddi_ClauseArrayFree(gateClauses);
    }
    //If pre order first load childs' cones after gate's clauses
    if (!ddiPdr->doPostOrderClauseLoad) {
      pdrFindOrAddIncrementalClausesWithPgRecur(tf, ddiPdr, rOdd, rGateId,
        clausesToLoad, rRootVar, useSolver2, level + 1, gateId);
      pdrFindOrAddIncrementalClausesWithPgRecur(tf, ddiPdr, lOdd, lGateId,
        clausesToLoad, lRootVar, useSolver2, level + 1, gateId);
    }
  }

  //Return number of clauses to load
  return Ddi_ClauseArrayNum(clausesToLoad);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_ClauseArray_t *      //M
pdrFindOrAddIncrementalClauses(
  TravPdrTimeFrame_t * tf,
  Ddi_PdrMgr_t * ddiPdr,
  Ddi_Clause_t * cl,
  int usePsClause,
  int useSolver2
)
{
  //Utility variables
  int i;
  Ddi_NetlistClausesInfo_t *netCl = &(ddiPdr->trNetlist);
  Ddi_ClauseArray_t *clausesToLoad = Ddi_ClauseArrayAlloc(0);
  int *latchLoaded =
    useSolver2 ? tf->trClauseMap.latchLoaded2 : tf->trClauseMap.latchLoaded;

  //Load cone for every next state variable of the cube
  for (i = 0; i < Ddi_ClauseNumLits(cl); i++) {

    //Get latch
    Ddi_Clause_t *clNew;
    int lit = cl->lits[i];
    int var = DdiPdrLit2Var(lit);
    int id =
      usePsClause ? DdiPdrVarPsIndex(ddiPdr, var) : DdiPdrVarNsIndex(ddiPdr,
      var);

    Pdtutil_Assert(id >= 0
      && id < tf->trClauseMap.nLatch,
      "wrong var in incremental clause findOrAdd");

    //Check if latch already loaded
    if (latchLoaded[id] == PDR_LOADED_BOTH_POLARITIES) {
      continue;
    }
    latchLoaded[id] = PDR_LOADED_BOTH_POLARITIES;

    //If latch's input is a constant or a variable just load linking clauses
    if (netCl->roots[id] == 0) {
      //Constant input --> 1 link clause
      int clId, j = netCl->nGate + id;

      Pdtutil_Assert(netCl->clauseMapNum[j] <= 2, "error in clause load");
      clId = netCl->clauseMapIndex[j];
      clNew = ddiPdr->trClauses->clauses[clId];
      Ddi_ClauseArrayPush(clausesToLoad, clNew);

      //Variable input --> 2 link clause
      if (netCl->clauseMapNum[j] == 2) {
        clNew = ddiPdr->trClauses->clauses[clId + 1];
        Ddi_ClauseArrayPush(clausesToLoad, clNew);
      }
    } else {
      //If latch's input is a logic function load logic cone and then linking clauses
      int gateId, clId, j = netCl->nGate + id;
      int root = netCl->roots[id];

      gateId = root < 0 ? (-root) - 1 : root - 1;

      //Load logic cone
      int recurCounter = pdrFindOrAddIncrementalClausesRecur(tf, ddiPdr, gateId, clausesToLoad, useSolver2, 0); //M

      //Load linkg clauses
      if (netCl->clauseMapNum[j] > 0) {
        Pdtutil_Assert(netCl->clauseMapNum[j] == 2, "error in clause load");
        clId = netCl->clauseMapIndex[j];
        clNew = ddiPdr->trClauses->clauses[clId];
        Ddi_ClauseArrayPush(clausesToLoad, clNew);
        clNew = ddiPdr->trClauses->clauses[clId + 1];
        Ddi_ClauseArrayPush(clausesToLoad, clNew);
      }
    }
  }

  //Return logic cone clause array
  return clausesToLoad;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int                      //M
pdrFindOrAddIncrementalClausesRecur(
  TravPdrTimeFrame_t * tf,
  Ddi_PdrMgr_t * ddiPdr,
  int gateId,
  Ddi_ClauseArray_t * clausesToLoad,
  int useSolver2,
  int level
)
{
  //Utility variables
  int rightCounter = 0;
  int leftCounter = 0;
  int i, ir, il, clStartId;
  Ddi_NetlistClausesInfo_t *netCl = &(ddiPdr->trNetlist);
  int *aigClauseMap =
    useSolver2 ? tf->trClauseMap.aigClauseMap2 : tf->trClauseMap.aigClauseMap;

  //Check if gate is loaded
  Pdtutil_Assert(gateId >= 0 && gateId < netCl->nGate, "wrong gate id");
  if (aigClauseMap[gateId] == PDR_LOADED_BOTH_POLARITIES) {
    return 0;
  }
  aigClauseMap[gateId] = PDR_LOADED_BOTH_POLARITIES;

  //Compute min depths for touched gates
  if (netCl->clauseMapNum[gateId] > 0 && netCl->gateId2cnfId[gateId] != -1) {
    if (tf->trClauseMap.minDepth[gateId] == -1
      || tf->trClauseMap.minDepth[gateId] > level) {
      tf->trClauseMap.minDepth[gateId] = level;
      if (tf->trClauseMap.minLevel == -1 || tf->trClauseMap.minLevel > level) {
        tf->trClauseMap.minLevel = level;
      }
    }
  }
  //Check if gate is terminal
  if (netCl->fiIds[0][gateId] == 0) {
    Pdtutil_Assert(netCl->fiIds[1][gateId] == 0, "wrong netlist teminal");
    return 0;
  }
  //Compute recurring parameters
  Pdtutil_Assert(netCl->clauseMapNum[gateId] >= 0, "wrong map num");
  ir = netCl->fiIds[0][gateId];
  il = netCl->fiIds[1][gateId];
  int rGateId = ir < 0 ? (-ir) - 1 : ir - 1;
  int lGateId = il < 0 ? (-il) - 1 : il - 1;

  Pdtutil_Assert(rGateId >= 0
    && lGateId >= 0, "error recurring in clause map");

  //If post order load childs' cones before gate's clauses
  if (ddiPdr->doPostOrderClauseLoad) {
    rightCounter =
      pdrFindOrAddIncrementalClausesRecur(tf, ddiPdr, rGateId, clausesToLoad,
      useSolver2, level + 1);
    leftCounter =
      pdrFindOrAddIncrementalClausesRecur(tf, ddiPdr, lGateId, clausesToLoad,
      useSolver2, level + 1);
  }
  //Load gate's clauses
  clStartId = netCl->clauseMapIndex[gateId];
  for (i = 0; i < netCl->clauseMapNum[gateId]; i++) {
    Ddi_Clause_t *clNew = ddiPdr->trClauses->clauses[clStartId + i];

    Ddi_ClauseArrayPush(clausesToLoad, clNew);
  }

  //If pre order load childs' cones after gate's clauses
  if (!ddiPdr->doPostOrderClauseLoad) {
    rightCounter =
      pdrFindOrAddIncrementalClausesRecur(tf, ddiPdr, rGateId, clausesToLoad,
      useSolver2, level + 1);
    leftCounter =
      pdrFindOrAddIncrementalClausesRecur(tf, ddiPdr, lGateId, clausesToLoad,
      useSolver2, level + 1);
  }
  //Return number of clauses to load
  return netCl->clauseMapNum[gateId] + rightCounter + leftCounter;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
computeConeSize(
  TravPdrMgr_t * pdrMgr,
  Ddi_Clause_t * cl,
  int usePsClause,
  int overApproximate
)
{
  //Utility variables
  Ddi_PdrMgr_t *ddiPdr = pdrMgr->ddiPdr;
  int i;
  int counter = 0;
  Ddi_NetlistClausesInfo_t *netCl = &(ddiPdr->trNetlist);

  //Over-approximated cone size
  if (overApproximate) {
    for (i = 0; i < Ddi_ClauseNumLits(cl); i++) {
      int lit = cl->lits[i];
      int var = DdiPdrLit2Var(lit);
      int id =
        usePsClause ? DdiPdrVarPsIndex(ddiPdr, var) : DdiPdrVarNsIndex(ddiPdr,
        var);

      Pdtutil_Assert(id >= 0
        && id < netCl->nRoot, "wrong var in incremental clause findOrAdd");

      if (pdrMgr->conesSize[id] > 0)
        counter += pdrMgr->conesSize[id];
      else {
        int j = netCl->nGate + id;

        counter += netCl->clauseMapNum[j];
        int coneSize;

        if (netCl->roots[id] != 0) {
          int root = netCl->roots[id];
          int gateId = root < 0 ? (-root) - 1 : root - 1;

          coneSize = computeConeSizeRecur(pdrMgr, gateId, overApproximate);
        }
        counter += coneSize;
        pdrMgr->conesSize[id] = counter;
      }
    }
  }
  //Non over-approximated cone size (takes into account sharing between cones)
  else {

    //Reset temporary loaded gates map
    for (i = 0; i < netCl->nGate; i++) {
      pdrMgr->tempAigMap[i] = PDR_NOT_COUNTED;
    }

    //Compute cones size
    for (i = 0; i < Ddi_ClauseNumLits(cl); i++) {
      int lit = cl->lits[i];
      int var = DdiPdrLit2Var(lit);
      int id =
        usePsClause ? DdiPdrVarPsIndex(ddiPdr, var) : DdiPdrVarNsIndex(ddiPdr,
        var);

      Pdtutil_Assert(id >= 0
        && id < netCl->nRoot, "wrong var in incremental clause findOrAdd");

      int j = netCl->nGate + id;

      counter += netCl->clauseMapNum[j];
      int coneSize;

      if (netCl->roots[id] != 0) {
        int j = netCl->nGate + id;
        int root = netCl->roots[id];
        int gateId = root < 0 ? (-root) - 1 : root - 1;

        coneSize = computeConeSizeRecur(pdrMgr, gateId, overApproximate);
      }
      counter += coneSize;
    }
  }

  //Return computed size
  return counter;
}

static int
computeConeSizeRecur(
  TravPdrMgr_t * pdrMgr,
  int gateId,
  int overApproximate
)
{
  //Utility variables
  Ddi_PdrMgr_t *ddiPdr = pdrMgr->ddiPdr;
  int rightCounter = 0;
  int leftCounter = 0;
  int i, ir, il, clStartId;
  Ddi_NetlistClausesInfo_t *netCl = &(ddiPdr->trNetlist);

  //Over-approximate: just recurr whitout accountign sharing
  Pdtutil_Assert(gateId >= 0 && gateId < netCl->nGate, "wrong gate id");
  if (overApproximate) {
    if (netCl->fiIds[0][gateId] == 0) {
      Pdtutil_Assert(netCl->fiIds[1][gateId] == 0, "wrong netlist teminal");
      return 0;
    }
    ir = netCl->fiIds[0][gateId];
    il = netCl->fiIds[1][gateId];
    int rGateId = ir < 0 ? (-ir) - 1 : ir - 1;
    int lGateId = il < 0 ? (-il) - 1 : il - 1;

    Pdtutil_Assert(rGateId >= 0
      && lGateId >= 0, "error recurring in clause map");
    rightCounter = computeConeSizeRecur(pdrMgr, rGateId, overApproximate);
    leftCounter = computeConeSizeRecur(pdrMgr, lGateId, overApproximate);
    return netCl->clauseMapNum[gateId] + rightCounter + leftCounter;
  }
  //Non over-approximated: take into account sharing between cones
  else {

    //Check if already counted
    if (pdrMgr->tempAigMap[gateId] == PDR_COUNTED) {
      return 0;
    }
    //Check if terminal node
    if (netCl->fiIds[0][gateId] == 0) {
      Pdtutil_Assert(netCl->fiIds[1][gateId] == 0, "wrong netlist teminal");
      return 0;
    }
    //Recurr on children
    Pdtutil_Assert(netCl->clauseMapNum[gateId] >= 0, "wrong map num");
    ir = netCl->fiIds[0][gateId];
    il = netCl->fiIds[1][gateId];
    int rGateId = ir < 0 ? (-ir) - 1 : ir - 1;
    int lGateId = il < 0 ? (-il) - 1 : il - 1;

    Pdtutil_Assert(rGateId >= 0
      && lGateId >= 0, "error recurring in clause map");
    rightCounter = computeConeSizeRecur(pdrMgr, rGateId, overApproximate);
    leftCounter = computeConeSizeRecur(pdrMgr, lGateId, overApproximate);

    //Set gate as counted
    pdrMgr->tempAigMap[gateId] = PDR_COUNTED;

    //Return size of the current cone
    return netCl->clauseMapNum[gateId] + rightCounter + leftCounter;
  }
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
computeConeSizeWithPg(
  TravPdrMgr_t * pdrMgr,
  Ddi_Clause_t * cl,
  int usePsClause
)
{

  //Utility variables
  int i, j;
  Ddi_PdrMgr_t *ddiPdr = pdrMgr->ddiPdr;
  int counter = 0;
  Ddi_Clause_t *clNew;
  Ddi_NetlistClausesInfo_t *netCl = &(ddiPdr->trNetlist);

  //Reset temporary loaded gates map
  for (i = 0; i < netCl->nGate; i++) {
    pdrMgr->tempAigMap[i] = PDR_NOT_LOADED;
  }

  //Load cone for every next state variable of the cube
  for (i = 0; i < Ddi_ClauseNumLits(cl); i++) {

    //Get latch
    int lit = cl->lits[i];
    int var = DdiPdrLit2Var(lit);
    int pos = lit > 0;          //Assumed polarity
    int id =
      usePsClause ? DdiPdrVarPsIndex(ddiPdr, var) : DdiPdrVarNsIndex(ddiPdr,
      var);

    Pdtutil_Assert(id >= 0
      && id < netCl->nRoot, "wrong var in incremental clause findOrAdd");

    int coneSize;

    counter++;                  //Load one linking clause if either gate is a variable, a gate or a constant
    if (netCl->roots[id] != 0) {
      int j = netCl->nGate + id;
      int root = netCl->roots[id];
      int isRootInv = root < 0;
      int gateId = root < 0 ? (-root) - 1 : root - 1;
      int isOdd = pos ? isRootInv : !isRootInv;
      int rootVar = netCl->gateId2cnfId[gateId];

      coneSize =
        computeConeSizeWithPgRecur(pdrMgr, gateId, isOdd, -1, rootVar);
    }
    counter += coneSize;
  }

  //Return computed size
  return counter;
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
computeConeSizeWithPgRecur(
  TravPdrMgr_t * pdrMgr,
  int gateId,
  int isOdd,
  int ldrGateId,
  int ldrVar
)
{

  //Utility variables
  Ddi_PdrMgr_t *ddiPdr = pdrMgr->ddiPdr;
  int rightCounter = 0;
  int leftCounter = 0;
  int counter = 0;
  int i, j, clStartId;
  Ddi_Mgr_t *ddm = ddiPdr->ddiMgr;
  bAig_Manager_t *bmgr = Ddi_MgrReadMgrBaig(ddm);
  Ddi_NetlistClausesInfo_t *netCl = &(ddiPdr->trNetlist);

  //Get current leader variable
  int isInternal = (netCl->gateId2cnfId[gateId] == -1) ? 1 : 0;
  int rootVar = ldrVar;

  if (!isInternal)
    rootVar = netCl->gateId2cnfId[gateId];

  //Check if gate is terminal
  if (netCl->fiIds[0][gateId] == 0) {
    Pdtutil_Assert(netCl->fiIds[1][gateId] == 0, "wrong netlist teminal");
    return 0;
  }
  //Internal node: check if it's a cut node
  if (ldrGateId != -1) {
    int k;
    int isCutPoint = 0;

    if (netCl->leaders[gateId] != NULL) {
      for (k = 0; k < Pdtutil_IntegerArrayNum(netCl->leaders[gateId]); k++) {
        int ldr = Pdtutil_IntegerArrayRead(netCl->leaders[gateId], k);

        if (ldr == ldrGateId) {
          isCutPoint = 1;       //Found cut point
          break;
        }
      }
    }
    //If node is not a point recurr without loading clauses (just to propagate polarity paths internal to current leader gate)
    if (!isCutPoint) {
      int ir = netCl->fiIds[0][gateId];
      int il = netCl->fiIds[1][gateId];
      int rInv = ir < 0;
      int lInv = il < 0;
      int rGateId = ir < 0 ? (-ir) - 1 : ir - 1;
      int lGateId = il < 0 ? (-il) - 1 : il - 1;

      Pdtutil_Assert(rGateId >= 0
        && lGateId >= 0, "error recurring in clause map");
      int rOdd = rInv ? !isOdd : isOdd;
      int lOdd = lInv ? !isOdd : isOdd;
      int rRootVar = rRootVar = netCl->gateId2cnfId[rGateId];;
      int lRootVar = lRootVar = netCl->gateId2cnfId[lGateId];

      rightCounter =
        computeConeSizeWithPgRecur(pdrMgr, rGateId, rOdd, ldrGateId, rRootVar);
      leftCounter =
        computeConeSizeWithPgRecur(pdrMgr, lGateId, lOdd, ldrGateId, lRootVar);
      return rightCounter + leftCounter;
    }
  }
  //If gate is a leader or a cut point current's path polarity must be loaded
  int polarityToLoad = !isOdd;

  //Check if polarity is loaded
  Pdtutil_Assert(gateId >= 0 && gateId < netCl->nGate, "wrong gate id");
  if (pdrMgr->tempAigMap[gateId] == PDR_LOADED_BOTH_POLARITIES) {
    return 0;
  } else if (pdrMgr->tempAigMap[gateId] == PDR_NOT_LOADED) {
    pdrMgr->tempAigMap[gateId] =
      polarityToLoad ? PDR_LOADED_POS_POLARITY : PDR_LOADED_NEG_POLARITY;
  } else if ((pdrMgr->tempAigMap[gateId] == PDR_LOADED_POS_POLARITY
      && polarityToLoad == 0)
    || (pdrMgr->tempAigMap[gateId] == PDR_LOADED_NEG_POLARITY
      && polarityToLoad == 1)) {
    pdrMgr->tempAigMap[gateId] = PDR_LOADED_BOTH_POLARITIES;
  } else {
    return 0;
  }

  //Compute recurring parameters
  Pdtutil_Assert(netCl->clauseMapNum[gateId] >= 0, "wrong map num");
  int ir = netCl->fiIds[0][gateId];
  int il = netCl->fiIds[1][gateId];
  int rInv = ir < 0;
  int lInv = il < 0;
  int rGateId = ir < 0 ? (-ir) - 1 : ir - 1;
  int lGateId = il < 0 ? (-il) - 1 : il - 1;

  Pdtutil_Assert(rGateId >= 0
    && lGateId >= 0, "error recurring in clause map");
  int rOdd = rInv ? !isOdd : isOdd;
  int lOdd = lInv ? !isOdd : isOdd;
  int rRootVar = netCl->gateId2cnfId[rGateId];
  int lRootVar = netCl->gateId2cnfId[lGateId];

  //Load current clauses that are not satisfied by current's path polarity
  if (netCl->clauseMapNum[gateId] != 0) {
    int clStartId = netCl->clauseMapIndex[gateId];
    Ddi_ClauseArray_t *gateClauses =
      Ddi_ClauseArrayAlloc(netCl->clauseMapNum[gateId]);
    for (i = 0; i < netCl->clauseMapNum[gateId]; i++) {
      Ddi_Clause_t *clNew = ddiPdr->trClauses->clauses[clStartId + i];

      Ddi_ClauseArrayPush(gateClauses, clNew);
    }
    for (i = 0; i < Ddi_ClauseArrayNum(gateClauses); i++) {
      Ddi_Clause_t *clNew = gateClauses->clauses[i];
      int satisfyingPolarity = -1;

      for (j = 0; j < Ddi_ClauseNumLits(clNew); j++) {
        int newVar = DdiPdrLit2Var(clNew->lits[j]);

        if (newVar == rootVar) {
          satisfyingPolarity = (clNew->lits[j] < 0) ? 0 : 1;
          break;
        }
      }
      Pdtutil_Assert(satisfyingPolarity != -1, "Problem in encoding");
      if (polarityToLoad != satisfyingPolarity) {
        counter++;
      }
    }
    Ddi_ClauseArrayFree(gateClauses);
  }
  //Recur
  rightCounter =
    computeConeSizeWithPgRecur(pdrMgr, rGateId, rOdd, gateId, rRootVar);
  leftCounter =
    computeConeSizeWithPgRecur(pdrMgr, lGateId, lOdd, gateId, lRootVar);

  //Return size of the current cone
  return counter + rightCounter + leftCounter;
}

/**Function*******************************************************************
  Synopsis    [generalize clause using analyze final & mode aggressive checks]
  Description [generalize clause using analyze final & mode aggressive checks.
generalizeLevel is used to select effort -
<=0: just analyzeFinal; 1: sat based generalization attempted just if
analyzeFinal improved; >=2: always try sat based generalization]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
pdrTimeFrameLoadSolver(
  TravPdrMgr_t * pdrMgr,
  Ddi_SatSolver_t * solver,
  int k,
  int incrementalLoad,
  int useSolver2
)
{
  int j, i, kMax = pdrMgr->nTimeFrames - 1;
  TravPdrTimeFrame_t *tf =
    k < 0 ? pdrMgr->infTimeFrame : &(pdrMgr->timeFrames[k]);

  //Initialize solvers restart helper structures
  if (useSolver2) {
    for (i = 0; i < TRAV_PDR_RESTART_WINDOW_SIZE; i++) {
      tf->restartWindow2[i] = 0.0;
    }
    tf->wndStart2 = 0;
    tf->wndEnd2 = 0;
    tf->wndSum2 = 0.0;
    tf->stats.numLoadedTrClauses2 = 0;
    tf->stats.numInductionClauses2 = 0;
  } else {
    for (i = 0; i < TRAV_PDR_RESTART_WINDOW_SIZE; i++) {
      tf->restartWindow[i] = 0.0;
    }
    tf->wndStart = 0;
    tf->wndSum = 0.0;
    tf->wndEnd = 0;
    tf->stats.numLoadedTrClauses = 0;
    tf->stats.numInductionClauses = 0;
  }

  //Add target clauses
  if (1 || k == kMax) {
    if (pdrMgr->targetCube != NULL) {

      //Ddi_SatSolverAddClauses(tf->solver,pdrMgr->target_subset);
    } else {
      Ddi_SatSolverAddClauses(solver, pdrMgr->ddiPdr->targetClauses);
      //      Ddi_SatSolverAddClauses(solver, pdrMgr->ddiPdr->targetClauses);
    }
  }
  //Reset structures for incremental TR loading
  if (incrementalLoad) {
    int i;

    Pdtutil_Assert(tf != NULL, "null tf");
    /* reset tr clause mapping */
    for (i = 0; i < tf->trClauseMap.nLatch; i++) {
      if (useSolver2)
        tf->trClauseMap.latchLoaded2[i] = PDR_NOT_LOADED;
      else {
        tf->trClauseMap.latchLoaded[i] = PDR_NOT_LOADED;
      }
    }
    for (i = 0; i < tf->trClauseMap.nGate; i++) {
      if (useSolver2)
        tf->trClauseMap.aigClauseMap2[i] = PDR_NOT_LOADED;
      else {
        tf->trClauseMap.aigClauseMap[i] = PDR_NOT_LOADED;
      }
      tf->trClauseMap.minDepth[i] = -1;
    }
    tf->trClauseMap.minLevel = -1;
  } else {
    //Add TR clauses monolithically
    Ddi_SatSolverAddClauses(solver, pdrMgr->ddiPdr->trClauses);
  }

  //Add invar clauses
  Ddi_SatSolverAddClauses(solver, pdrMgr->ddiPdr->invarClauses);

  //Add Fk clauses
  for (j = k; k >= 0 && j <= kMax; j++) {
    if (j > k && pdrMgr->settings.enforceInductiveClauses) {
      int jj;

      for (jj = 0; jj < pdrMgr->timeFrames[j].clauses->nClauses; jj++) {
        Ddi_SatSolverAddClauseCustomized(solver,
          pdrMgr->timeFrames[j].clauses->clauses[jj], 0, 1, 0);
      }
    }
    Ddi_SatSolverAddClauses(solver, pdrMgr->timeFrames[j].clauses);
    Ddi_SatSolverAddClauses(solver, pdrMgr->timeFrames[j].itpClauses);
    if (pdrMgr->infTimeFrame != NULL) {
      Ddi_SatSolverAddClauses(solver, pdrMgr->infTimeFrame->clauses);
    }
  }
  if (0 && k > 0 && k < kMax - 1) {
    pdrTimeFrameAssertInvarspec(pdrMgr, k);
  }
}


/**Function*******************************************************************
  Synopsis    [generalize clause using analyze final & mode aggressive checks]
  Description [generalize clause using analyze final & mode aggressive checks.
generalizeLevel is used to select effort -
<=0: just analyzeFinal; 1: sat based generalization attempted just if
analyzeFinal improved; >=2: always try sat based generalization]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
timeFrameUpdateEqClasses(
  TravPdrMgr_t * pdrMgr,
  int k
)
{
  int refDone, kMax = pdrMgr->nTimeFrames - 1;
  TravPdrTimeFrame_t *tf0, *tf = &(pdrMgr->timeFrames[k]);
  Ddi_SatSolver_t *solver;

  Pdtutil_Assert(k > 0, "k>0 required in eq class computation");
  Pdtutil_Assert(pdrMgr->eqCl != NULL, "eq classes required");

  tf0 = &(pdrMgr->timeFrames[k - 1]);

  solver = Ddi_SatSolverAlloc();
  DdiSatSolverSetFreeVarMinId(solver, pdrMgr->ddiPdr->freeCnfVar);

  pdrTimeFrameLoadSolver(pdrMgr, solver, k - 1, 0 /*no incremental */ , 0);

  refDone = Ddi_EqClassesRefine(pdrMgr->ddiPdr, pdrMgr->eqCl, solver,
    pdrMgr->ddiPdr->ns, pdrMgr->ddiPdr->nsClause);

  if (refDone) {
    int i;
    Ddi_ClauseArray_t *ca = Ddi_EqClassesAsClauses(pdrMgr->ddiPdr,
      pdrMgr->eqCl, pdrMgr->ddiPdr->psClause);

    if (ca != NULL) {
      int i;

      PDR_DBG(Ddi_PdrPrintClauseArray(pdrMgr->ddiPdr, ca));
      for (i = 0; i < Ddi_ClauseArrayNum(ca); i++) {
        Ddi_Clause_t *cl = Ddi_ClauseArrayRead(ca, i);

        Ddi_ClauseArrayPush(tf->clauses, cl);
      }
      Ddi_SatSolverAddClauses(tf->solver, ca);
      if (tf->solver2 != NULL)
        Ddi_SatSolverAddClauses(tf->solver2, ca);

      Ddi_ClauseArrayFree(ca);
    }
  }

  Ddi_SatSolverQuit(solver);

}

/**Function*******************************************************************
  Synopsis    [generalize clause using analyze final & mode aggressive checks]
  Description [generalize clause using analyze final & mode aggressive checks.
generalizeLevel is used to select effort -
<=0: just analyzeFinal; 1: sat based generalization attempted just if
analyzeFinal improved; >=2: always try sat based generalization]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
timeFrameUpdateRplusClasses(
  TravPdrMgr_t * pdrMgr,
  int k
)
{
  int refDone, kMax = pdrMgr->nTimeFrames - 1;
  TravPdrTimeFrame_t *tf0, *tf = &(pdrMgr->timeFrames[k]);
  Ddi_SatSolver_t *solver;
  Trav_Mgr_t *travMgr = pdrMgr->travMgr;
  Ddi_Bddarray_t *fwdRplus = Trav_MgrReadNewi(travMgr);
  Ddi_Bdd_t *r_k;


  if (k == 0 || fwdRplus == NULL || Ddi_BddarrayNum(fwdRplus) <= k)
    return;

  r_k = Ddi_BddarrayRead(fwdRplus, k);
  Ddi_ClauseArray_t *ca = Ddi_AigClauses(r_k, 0, NULL);

  if (tf->itpClauses == NULL) {
    tf->itpClauses = Ddi_ClauseArrayAlloc(0);
  }

  if (ca != NULL) {
    int i;

    for (i = 0; i < Ddi_ClauseArrayNum(ca); i++) {
      Ddi_Clause_t *cl = Ddi_ClauseArrayRead(ca, i);

      Ddi_ClauseArrayPush(tf->itpClauses, cl);
    }
    Ddi_SatSolverAddClauses(tf->solver, ca);
    if (tf->solver2 != NULL)
      Ddi_SatSolverAddClauses(tf->solver2, ca);

    Ddi_ClauseArrayFree(ca);
  }
}

static void
printClause(
  Ddi_Clause_t * cla
)
{
  int k;

  fprintf(stderr, "[");
  for (k = 0; k < cla->nLits; k++) {
    fprintf(stderr, " %d", cla->lits[k]);
  }
  fprintf(stderr, " ]\n");
}

static void
bumpActTopologically(
  TravPdrTimeFrame_t * tf,
  TravPdrMgr_t * pdrMgr,
  int useSolver2
)
{
  Ddi_PdrMgr_t *ddiPdr = pdrMgr->ddiPdr;
  Ddi_SatSolver_t *solver = tf->solver;
  Ddi_NetlistClausesInfo_t *netCl = &(ddiPdr->trNetlist);
  int i, j;

  for (i = 0; i < netCl->nGate; i++) {
    //Bump activities
    if (tf->trClauseMap.minDepth[i] >= 0) {
      Pdtutil_Assert(netCl->gateId2cnfId[i] != -1, "Wrong touched gate");
      Lit cnfId = DdiLit2Minisat(solver, netCl->gateId2cnfId[i]);
      int incr;

      if (pdrMgr->settings.bumpActTopologically == 1) {
        incr = tf->trClauseMap.minLevel - tf->trClauseMap.minDepth[i];  //Increment activity of variables close to next state
      } else if (pdrMgr->settings.bumpActTopologically == 2) {
        incr = tf->trClauseMap.minDepth[i]; //Increment activity of variables close to present state
      }
      solver->S->varBumpActivityExternal(cnfId, incr);
    }
    //Clear min depths
    tf->trClauseMap.minDepth[i] = -1;
  }

  tf->trClauseMap.minLevel = -1;
}




/**Function********************************************************************
  Synopsis    []
  Description [ Check if the current limits for PDR verification have been reached. ]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static int
pdrOutOfLimits(
  TravPdrMgr_t * pdrMgr
)
{
  Trav_Mgr_t *travMgr = pdrMgr->travMgr;

  //Check if an abort command is sent from outside PDR
  if (travMgr->xchg.enabled) {
    Trav_Tune_t *travTune = &(travMgr->xchg.tune);
    if (travTune != NULL && travTune->doAbort) {
      Pdtutil_VerbosityMgrIf(pdrMgr->travMgr, Pdtutil_VerbLevelUsrMed_c) {
        fprintf(tMgrO(pdrMgr->travMgr), "Tuning command: abort.\n");
      }
      return 1;
    }
  }

  //Check if time limit has been reached
  if (travMgr->settings.aig.pdrTimeLimit > 0 &&
    (util_cpu_time() > travMgr->settings.aig.pdrTimeLimit)) {
    Pdtutil_VerbosityMgrIf(pdrMgr->travMgr, Pdtutil_VerbLevelUsrMed_c) {
      fprintf(tMgrO(pdrMgr->travMgr), "Time limit reached.\n");
    }
    return 1;
  }

  return 0;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Trav_IgrToPdr (
  Trav_Mgr_t * travMgrIgr,
  Trav_Mgr_t * travMgrPdr
)
{
  TravPdrMgr_t *pdrMgr = travMgrPdr->settings.ints.pdrMgr;
  Trav_ItpMgr_t *itpMgr = travMgrIgr->settings.ints.itpMgr;
  Ddi_Mgr_t *ddmI = travMgrIgr->ddiMgrTr;
  Ddi_Mgr_t *ddm = travMgrPdr->ddiMgrTr;
  int i, k, kMax = pdrMgr->nTimeFrames - 1, 
    itpMax = Ddi_BddarrayNum(itpMgr->fromRings)-1;
  int kMin = travMgrIgr->settings.ints.igrRingFirst;
  if (kMin<1) kMin = 1; 

  if (travMgrIgr->settings.ints.igrRingLast>=0) {
    itpMax = travMgrIgr->settings.ints.igrRingLast;
  }

  while (kMax<itpMax) {
    kMax++;
    pdrTimeFrameAdd(pdrMgr, kMax, 1);
  }
  travMgrPdr->settings.ints.nPdrFrames = kMax;
  pdrMgr->maxItpShare = kMax;

  Pdtutil_VerbosityMgrIf(travMgrIgr, Pdtutil_VerbLevelUsrMax_c) {
    fprintf(tMgrO(travMgrIgr), "transferring rings IGR to PDR:\n\n");
  }

  //  checkRingInclusion(itpMgr);

  if (kMax<Ddi_BddarrayNum(itpMgr->pdrReachedRings)) {
    Ddi_Bdd_t *curR = Ddi_BddDup(Ddi_BddarrayRead(itpMgr->fromRings,kMax));
    Ddi_Bdd_t *pdrR = Ddi_BddarrayRead(itpMgr->pdrReachedRings,kMax);
    Ddi_Bdd_t *nonRedR = Ddi_AigInductiveImgPlus(NULL,
      curR, NULL, pdrR, 2 /*doInductiveToPlus*/);
    Ddi_Free(curR);
    if (nonRedR != NULL) {
      curR = Ddi_BddarrayRead(itpMgr->fromRings,kMax);
      Ddi_DataCopy(curR,nonRedR);
    }
  }
  Trav_SimplifyRingsBwdFwd(itpMgr,kMax,kMax-kMin,-1,1,0,0);

  for (k=kMin; k<Ddi_BddarrayNum(itpMgr->fromRings); k++) {
    Ddi_Bdd_t *curR = Ddi_BddarrayRead(itpMgr->fromRings,k);
    if (!Ddi_BddIsOne(curR)) {
      curR = Ddi_BddSubstVars(curR,itpMgr->ps,itpMgr->ns);
      Ddi_Bdd_t *myR = Ddi_BddCopy(ddm, curR);
      Ddi_ClauseArray_t *rClauses = Ddi_AigClauses(myR, 0, NULL);
      pdrMgr->timeFrames[k].itpClauses = rClauses;
      Ddi_Free(myR);
      Ddi_Free(curR);
    }

#if 0
    TravPdrTimeFrame_t *tf = &(pdrMgr->timeFrames[k]);
    Ddi_SatSolverQuit(tf->solver);
    Ddi_SatSolver_t *solver = Ddi_SatSolverAlloc();
    int incrementalLoad = pdrMgr->settings.incrementalTrClauses && k != 0;
    DdiSatSolverSetFreeVarMinId(solver, pdrMgr->ddiPdr->freeCnfVar);
    pdrTimeFrameLoadSolver(pdrMgr, solver, k, incrementalLoad, 0);
    tf->solver = solver;

    Pdtutil_Assert(Ddi_SatSolverOkay(solver),"UNSAT solver");
#endif
  }

  Pdtutil_VerbosityMgrIf(travMgrIgr, Pdtutil_VerbLevelUsrMax_c) {
    fprintf(tMgrO(travMgrIgr), "\n");
  }

  checkRingInclusion(itpMgr);

}


static int
checkRingInclusion(
  Trav_ItpMgr_t *itpMgr
)
{
  int k;

  for (k=2; k<Ddi_BddarrayNum(itpMgr->fromRings); k++) {
    Ddi_Bdd_t *curR = Ddi_BddDup(Ddi_BddarrayRead(itpMgr->fromRings,k));
    Ddi_Bdd_t *prevR = Ddi_BddDup(Ddi_BddarrayRead(itpMgr->fromRings,k-1));
    if (Ddi_BddarrayNum(itpMgr->pdrReachedRings)>k) {
      Ddi_Bdd_t *pdrR = Ddi_BddarrayRead(itpMgr->pdrReachedRings,k);
      Ddi_BddAndAcc(curR,pdrR);
    }
    if (Ddi_BddarrayNum(itpMgr->pdrReachedRings)>(k-1)) {
      Ddi_Bdd_t *pdrR = Ddi_BddarrayRead(itpMgr->pdrReachedRings,k-1);
      Ddi_BddAndAcc(prevR,pdrR);
    }

    if (Ddi_BddarrayNum(itpMgr->eqRings) > k) {
      Ddi_Bdd_t *eq = Ddi_BddMakeAig(Ddi_BddarrayRead(itpMgr->eqRings, k));
      Ddi_BddAndAcc(curR,eq);
    }
    if (Ddi_BddarrayNum(itpMgr->eqRings) >= k) {
      Ddi_Bdd_t *eq = Ddi_BddMakeAig(Ddi_BddarrayRead(itpMgr->eqRings, k-1));
      Ddi_BddAndAcc(prevR,eq);
    }

    Pdtutil_Assert(Ddi_BddIncluded(prevR,curR),"not included");
    Ddi_Free(curR);
    Ddi_Free(prevR);
  }

  if (itpMgr->initStub!=NULL) {
    Ddi_Bdd_t *curR = Ddi_BddDup(Ddi_BddarrayRead(itpMgr->fromRings,1));
    if (Ddi_BddarrayNum(itpMgr->pdrReachedRings)>0) {
      Ddi_Bdd_t *pdrR = Ddi_BddarrayRead(itpMgr->pdrReachedRings,0);
      Ddi_BddAndAcc(curR,pdrR);
    }
    if (Ddi_BddarrayNum(itpMgr->eqRings) > 1) {
      Ddi_Bdd_t *eq = Ddi_BddMakeAig(Ddi_BddarrayRead(itpMgr->eqRings, 1));
      Ddi_BddAndAcc(curR,eq);
    }
    Ddi_BddNotAcc(curR);
    Ddi_BddComposeAcc(curR,itpMgr->ns,itpMgr->initStub);
    Pdtutil_Assert(!Ddi_AigSat(curR),"not included");
    Ddi_Free(curR);
  }


  return 0;
}

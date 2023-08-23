/**CHeaderFile*****************************************************************

  FileName    [travInt.h]

  PackageName [trav]

  Synopsis    [Definition of the struct of FSM and struct for options]

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

#ifndef _TRAVINT
#define _TRAVINT

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include "dddmp.h"
#include "ddi.h"
#include "tr.h"
#include "part.h"
#include "trav.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define TRAV_PDR_QUEUE_MAXPRIO 3
#define TRAV_PDR_RESTART_WINDOW_SIZE 1000
#define TRAV_PDR_MAX_SPLIT_STAT 20


/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

struct Trav_Settings_s {
  Pdtutil_VerbLevel_e verbosity;
  FILE *stdout;
  int logPeriod;                /* period for verbosity activation */
  char *clk;
  int doCex;

  // the following seven options are currently NOT used...
  int removeLLatches;           /* if 1 do lambda latches removal */
  int mgrAuxFlag;               /* use the auxiliary manager during traversal */
  int enableDdR;                /* 1 if second dd manager for reached enabled */
  int smoothVars;               /* smoothing variables select for img/closure */
  int backward;                 /* backward traversal flag */
  int constrLevel;
  int maxIter;                  /* max allowed traversal iterations */
  int savePeriod;               /* Period for saving intermediate BDDs */
  char *savePeriodName;         /* Period Name (see savePeriod) */
  int iwls95Variant;            /* variant of IWLS95 image/traversal method */

  struct {
    /* dynamic profiling */
    Cuplus_PruneHeuristic_e trProfileMethod;
    int trProfileDynamicEnable; /* 1 if dynamic TR enabled */
    int trProfileThreshold;     /* -1 if TR pruning disabled */

    int trSorting;              /* method of sorting ( 0 = no sorting ) */
    float w1, w2, w3, w4;       /* weights for Weighted sorting method */
    int sortForBck;
    int trPreimgSort;

    int partDisjTrThresh;       /* 1 = disjoint TR partitioning threshold */
    int clustThreshold;         /* threshold (size of BDD) for building clusters */
    int apprClustTh;
    int imgClustTh;
    int squaring;               /* 1 enable squaring */
    int trProj;
    int trDpartTh;
    char *trDpartVar;
    char *auxVarFile;
    char *enableVar;
    char *approxTrClustFile;
    char *bwdTrClustFile;
  } tr;

  struct {
    char *satSolver;
    int abcOptLevel;
    int targetEn;
    int dynAbstr;
    int dynAbstrInitIter;
    int implAbstr;
    int ternaryAbstr;
    int abstrRef;
    int abstrRefGla;
    int abstrRefItp;
    int abstrRefItpMaxIter;
    int trAbstrItp;
    int trAbstrItpMaxFwdStep;
    char *storeAbstrRefRefinedVars;
    char *trAbstrItpLoad;
    char *trAbstrItpStore;
    int inputRegs;
    int selfTuningLevel;

    float lemmasTimeLimit;
    float lazyTimeLimit;

    int itpBdd;
    int itpPart;
    int itpPartCone;
    int itpAppr;
    int itpRefineCex;
    int itpStructAbstr;
    int itpNew;
    int itpExact;
    int itpInductiveTo;
    int itpInnerCones;
    int itpInitAbstr;
    int itpEndAbstr;
    int itpTrAbstr;
    int itpReuseRings;
    int itpTuneForDepth;
    int itpBoundkOpt;
    int itpExactBoundDouble;
    int itpConeOpt;
    int itpForceRun;
    int itpMaxStepK;
    int itpUseReached;
    int itpCheckCompleteness;
    int itpGfp;
    int itpConstrLevel;
    int itpGenMaxIter;
    int itpEnToPlusImg;
    int itpShareReached;
    int itpUsePdrReached;
    int itpReImg;
    int itpFromNewLevel;
    int itpWeaken;
    int itpStrengthen;
    int itpRpm;
    int itpTimeLimit;
    int itpPeakAig;
    char *itpStoreRings;

    int igrSide;
    int igrItpSeq;
    int igrRewind;
    int igrRewindMinK;
    int igrBwdRefine;
    int igrRestart;
    int igrGrowCone;
    int igrGrowConeMaxK;
    float igrGrowConeMax;
    int igrAssumeSafeBound;
    int igrUseRings;
    int igrUseRingsStep;
    int igrUseBwdRings;

    int igrConeSubsetBound;
    int igrConeSubsetSizeTh;
    float igrConeSubsetPiRatio;
    float igrConeSplitRatio;

    int igrMaxIter;
    int igrMaxExact;
    int igrFwdBwd;

    int pdrFwdEq;
    int pdrInf;
    int pdrUnfold;
    int pdrIndAssumeLits;
    int pdrPostordCnf;
    int pdrCexJustify;
    int pdrGenEffort;
    int pdrIncrementalTr;
    int pdrShareReached;
    int pdrReuseRings;
    int pdrMaxBlock;
    int pdrUsePgEncoding;
    int pdrBumpActTopologically;
    int pdrSpecializedCallsMask;
    int pdrRestartStrategy;
    int pdrTimeLimit;

    int bmcTimeLimit;
    int bmcMemLimit;
    int bmcTrAbstrPeriod;
    int bmcTrAbstrInit;
  } aig;

  struct {
    Trav_FromSelect_e fromSelect;
    int underApproxMaxVars;     // no corresponding opt field
    int underApproxMaxSize;     // no corresponding opt field
    float underApproxTargetReduction;   // no corresponding opt field
    int noCheck;
    int noMismatch;             // no corresponding opt field
    int mismatchOptLevel;
    int groupBad;
    int keepNew;                // no corresponding opt field
    int prioritizedMc;
    int pmcPrioThresh;
    int pmcMaxNestedFull;
    int pmcMaxNestedPart;
    int pmcNoSame;
    int countReached;
    int autoHint;
    int hintsStrategy;
    int hintsTh;
    Ddi_Bddarray_t *hints;
    Ddi_Varset_t *auxPsSet;
    int gfpApproxRange;
    int useMeta;
    Ddi_Meta_Method_e metaMethod;
    int metaMaxSmooth;
    int constrain;
    int constrainLevel;
    int checkFrozen;
    int apprOverlap;
    int apprNP;
    int apprMBM;
    int apprGrTh;
    int apprPreimgTh;
    int maxFwdIter;
    int nExactIter;
    int siftMaxTravIter;
    int implications;
    int imgDpartTh;
    int imgDpartSat;
    int bound;
    int strictBound;
    int guidedTrav;
    int univQuantifyTh;
    int optImg;
    int optPreimg;
    int bddTimeLimit;
    int bddNodeLimit;
    char *wP;
    char *wR;
    char *wBR;
    char *wU;
    char *wOrd;
    char *rPlus;
    char *invarFile;
    char *storeCNF;
    int storeCNFTR;
    char storeCNFmode;
    int storeCNFphase;
    int maxCNFLength;
  } bdd;

  struct {
    int saveClipLevel;
    Tr_Tr_t *auxFwdTr;
    Trav_ItpMgr_t *itpMgr;
    TravPdrMgr_t *pdrMgr;
    int univQuantifyDone;
    Ddi_Varset_t *univQuantifyVars;
    Ddi_Varsetarray_t *abstrRefRefinedVars;
    int nPdrFrames;
    int fixPointTimeFrame;
    int suspendFrame;
    int igrSingleRewind;
    int itpBmcBoundFirst;
    int igrRingFirst;
    int igrRingLast;
    int igrFpRing;
    Ddi_ClauseArray_t **timeFrameClauses;
    int doAbstrRefinedTo;
  } ints;

};


/**Struct*********************************************************************
 Synopsis    [Traversal Manager]
 Description [The Traversal manager structure]
******************************************************************************/

struct Trav_Mgr_s {
  char *travName;

  Ddi_Mgr_t *ddiMgrTr;          /* TR manager */
  Ddi_Mgr_t *ddiMgrR;           /* Reached states manager */
  Ddi_Mgr_t *ddiMgrAux;         /* auxiliary manager */
  Tr_Tr_t *tr;                  /* transition relation */
  Ddi_Vararray_t *i;            /* primary inputs */
  Ddi_Vararray_t *ps;           /* present state variables */
  Ddi_Vararray_t *ns;           /* next state variables */
  Ddi_Vararray_t *aux;          /* auxiliary state variables */
  Ddi_Bdd_t *from;              /* from set */
  Ddi_Bdd_t *reached;           /* reached set */
  Ddi_Bdd_t *care;              /* care set */
  Ddi_Bdd_t *proved;            /* set of proved properties */
  Ddi_Bdd_t *assume;            /* set of assumptions */
  Ddi_Bdd_t *assert;            /* assertion for reached */
  Ddi_Bddarray_t *newi;         /* frontier sets */
  Ddi_Bddarray_t *cexes;        /* array of counterexamples */
  Ddi_Var_t *pdtSpecVar;        /* pdt invarpec fold var */
  Ddi_Var_t *pdtConstrVar;      /* pdt invarpec fold var */
  int level;                    /* traversal level */
  int productPeak;              /* Product Peak value */
  int assertFlag;               /* 1 if Assertion Failed */
  long travTime;                /* Total Traversal Time */
  long startTime;               /* Traversal Start Time */

  Trav_Settings_t settings;
  Trav_Xchg_t xchg;
};

struct TravPdrTimeFrame_s {
  //Clauses sets
  Ddi_ClauseArray_t *clauses;
  Ddi_ClauseArray_t *itpClauses;

  //Blocking queues
  Ddi_ClauseArray_t *queue[TRAV_PDR_QUEUE_MAXPRIO + 1];

  //Latch equivalences array
  int *latchEquiv;
  int nLatch;

  //Solvers
  Ddi_SatSolver_t *solver;      //Primary solver
  Ddi_SatSolver_t *solver2;     //Auxiliary solver
  int hasSolver2;

  //Solvers restart helper structure
  float restartWindow[TRAV_PDR_RESTART_WINDOW_SIZE];
  float restartWindow2[TRAV_PDR_RESTART_WINDOW_SIZE];
  int wndStart;
  int wndStart2;
  int wndEnd;
  int wndEnd2;
  float wndSum;
  float wndSum2;

  //TR incremental loading informations
  struct {
    int nGate, nLatch;          //TR sizes

    //Mappings for primary solver TR loading (aigClauseMap states if clauses for a gate are already loaded,
    //latchLoaded states if clauses for a latch are already loaded)
    int *aigClauseMap;
    int *latchLoaded;

    //Mappings for auxiliary solver TR loading
    int *aigClauseMap2;
    int *latchLoaded2;

    //TR variables depth in AIG
    int *minDepth;
    int minLevel;
  } trClauseMap;

  //Time frame statistics
  struct {
    float solverTime;           //Cumulative TF solvng time
    int numInductionClauses;
    int numLoadedTrClauses;
    int numInductionClauses2;
    int numLoadedTrClauses2;
  } stats;
};
typedef struct TravPdrTimeFrame_s TravPdrTimeFrame_t;

struct TravPdrMgr_s {
  //Underlying managers
  Fsm_Mgr_t *fsmMgr;            //Fsm manager (original description of the system)
  Trav_Mgr_t *travMgr;          //Trav manager (original description of the system)
  Ddi_PdrMgr_t *ddiPdr;         //Ddi pdr manager (cnf encoding of the system)
  Ddi_TernarySimMgr_t *xMgr;    //Manager for ternary simulation

  //Auxiliary global solver
  Ddi_SatSolver_t *auxSolver;

  //Current target subset
  Ddi_SatSolver_t *targetEnumSolver;
  Ddi_Clause_t *targetCube;
  Ddi_Clause_t *actTargetTot;
  int cubes;

  //Time frames
  int nTimeFrames;
  int fixPointTimeFrame;
  int maxTimeFrames;
  TravPdrTimeFrame_t *timeFrames;
  TravPdrTimeFrame_t *infTimeFrame;

  //Counters
  int nClauses;
  int nRemoved;
  int maxItpShare;

  //Equivalence classes
  Pdtutil_EqClasses_t *eqCl;

  //Mappings for cone size computing (local to the current outmost timeframe?)
  int *tempAigMap;              //States if clauses for a gate are already counted for cone size computing
  int *conesSize;               //Maintains the cone size computed for each latch

  //Global statistics
  struct {
    int nSatSolverCalls;
    int nUnsat;
    int nQueuePop;
    int nGeneralize;
    int nClausePush;
    int nTernarySim;
    int nTernaryInit;
    int nBlocked;
    int nBlocks;
    int solverRestart;
    int maxBwdDepth;
    int result;
    long int nConfl;
    long int nDec;
    long int nProp;
    long int split[TRAV_PDR_MAX_SPLIT_STAT];
    float cexgenTime;
    float generalizeTime;
    float restartTime;
    float solverTime;
    float avgTrLoadedClauses;
    float avgTrLoadedClausesOld;
  } stats;

  //Global settings
  struct {
    int step;
    int ternarySimInit;
    int genEffort;
    int incrementalTrClauses;
    int solverRestartEnable;
    int enforceInductiveClauses;
    int usePgEnconding;
    int fwdEquivalences;
    int infTf;
    int cexJustify;
    int shareReached;
    int shareItpReached;
    int unfoldProp;
    int indAssumeLits;
    int useSolver2;
    int restartStrategy;
    int restartStrategy2;
    float restartThreshold;
    float restartThreshold2;
    int bumpActTopologically;
    int overApproximateConeSize;
    char specCallsMask;
    int targetSubset;
    Pdtutil_VerbLevel_e verbosity;
  } settings;
  int queueMaxPrio;
};

struct TravBmcMgr_s {
  //Underlying managers
  Fsm_Mgr_t *fsmMgr;            //Fsm manager (original description of the system)
  Trav_Mgr_t *travMgr;          //Trav manager (original description of the system)
  Ddi_TernarySimMgr_t *xMgr;    //Manager for ternary simulation

  Ddi_Bdd_t *bmcTarget;
  Ddi_Bdd_t *bmcAbstrTarget;
  Ddi_Bdd_t *bmcTargetCone;
  Ddi_Bdd_t *bmcConstrCone;
  Ddi_Bdd_t *bmcConstr;
  Ddi_Bdd_t *bmcInvar;
  Ddi_Bdd_t *bmcConstrBck;
  Ddi_Bddarray_t *unroll;
  Ddi_Bddarray_t *delta;
  Ddi_Bdd_t *target;
  Ddi_Bdd_t *invarspec;
  Ddi_Bdd_t *constraint;
  Ddi_Bdd_t *initStubConstr;
  Ddi_Bdd_t *init;
  Ddi_Bddarray_t *initStub;
  Ddi_Bddarray_t *itpRings;
  Ddi_Bdd_t *trAbstrItp;
  int trAbstrItpNumFrames;
  int trAbstrItpPeriod;
  int trAbstrItpInit;
  int trAbstrItpCheck;
  
  Ddi_Bddarray_t *dummyRefs;

  Ddi_Varsetarray_t *timeFrameVars;
  Ddi_Varsetarray_t *bwdTimeFrameVars;
  Pdtutil_Array_t *unrollArray;

  int nTimeFrames;
  int maxTimeFrames;

  //Equivalence classes
  Pdtutil_EqClasses_t *eqCl;

  Ddi_Vararray_t *inCoreScc;

  struct {
    Ddi_Vararray_t *psVars;
    Ddi_Vararray_t *cutVars;
    Ddi_Bdd_t *window;
  } part;

  struct {
    Ddi_Vararray_t *cutVarsTot;
    Ddi_Vararray_t *cutVars;
    Ddi_Vararray_t *cutVarsAllFrames;
    Ddi_Bddarray_t *rings;
    int lastFwd;
  } bwd;

  struct {
    Fsm_XsimMgr_t *xMgr;
    Pdtutil_Array_t *constLatchArray;
    int bound;
  } xsim;

  //Global statistics
  struct {
    int nSatSolverCalls;
    int nUnsat;
    int nTernarySim;
    int nTernaryInit;
    int nBlocked;
    int nBlocks;
    int solverRestart;
    long int nConfl;
    long int nDec;
    long int nProp;
  } stats;

  //Global settings
  struct {
    int step;
    int bwdStep;
    int unfolded;
    int strategy;
    int memLimit;
    int timeLimit;
    int stepFrames;
    int nFramesLimit;
    int incrementalBmc;
    int fwdBwdRatio;
    int enBwd;
    int shiftFwdFrames;
    int invarMaxBound;
    int useInvar;
    int targetByRefinement;
    Pdtutil_VerbLevel_e verbosity;
  } settings;

};
typedef struct TravBmcMgr_s TravBmcMgr_t;

struct TravTimeFrameInfo_s {
  long Size, Num;
  Ddi_Bddarray_t **Lits;
  Ddi_Vararray_t **vars;
  Ddi_Bddarray_t **PiLits;
  Ddi_Vararray_t **PiVars;
  Ddi_Varsetarray_t *Coi;
};

struct TravItpMgr_s {
  Trav_Mgr_t *travMgr;
  Fsm_Mgr_t *fsmMgr;
  Ddi_Mgr_t *ddiMgr;

  Ddi_Vararray_t *dynAbstrAux, *dynAbstrCut;
  Ddi_Vararray_t *abstrRefCtrl, *abstrRefInp, *abstrRefPsPiVars;
  Ddi_Bddarray_t *dynAbstrCutLits;
  Ddi_Bddarray_t *shiftPiLits;
  Ddi_Vararray_t *shiftPiVars, *saveNewPiVars;
  Ddi_Bddarray_t *abstrDoAbstr;
  Ddi_Bddarray_t *abstrRefFilter;
  Ddi_Bddarray_t *abstrDoRefine;
  Ddi_Bddarray_t *abstrPrioRefine;
  Ddi_Bddarray_t *abstrCurrAbstr;
  Ddi_Varsetarray_t *abstrRefRefinedVars;
  Ddi_Bdd_t *abstrRefTrConstr;
  unsigned char *enAbstr;
  Ddi_Bdd_t *tr, *trAux, *trRange;
  Ddi_Bdd_t *trAbstr, *itpTrAbstr, *trAuxAbstr;
  Ddi_Bdd_t *init;
  Ddi_Bdd_t *target;
  Ddi_Bdd_t *specSpace;
  Ddi_Bddarray_t *initStub;
  Ddi_Bdd_t *initStubState;
  Ddi_Bddarray_t *delta;
  Ddi_Bddarray_t *delta0;
  Ddi_Bddarray_t *deltaAbstr;
  Ddi_Bddarray_t *deltaStall;
  Ddi_Bddarray_t *trArray, *trArrayAbstr;
  Ddi_Bddarray_t *stubTrArray;
  Ddi_Var_t *stallCtrl;
  Ddi_Vararray_t *stalledVarsPs;
  Ddi_Vararray_t *stalledVarsNs;

  Ddi_Vararray_t *retimedCutPis;
  Ddi_Vararray_t *retimedCutRefPis;

  Tr_Tr_t *trBdd;
  Tr_Tr_t *trAig;

  Ddi_Bdd_t *splitCex;
  Ddi_Bdd_t *inductiveRplus;
  Ddi_Bdd_t *inductiveRplusLocal;
  Ddi_Bddarray_t *reachedRings;
  Ddi_Bddarray_t *bckReachedRings;
  Ddi_Bddarray_t *fromRings;
  Ddi_Bddarray_t *bckFromRings;
  Ddi_Bddarray_t *pdrReachedRings;
  Ddi_Bdd_t *provedProps;
  Ddi_Bdd_t *lemma;
  Ddi_Bddarray_t *antecedents;
  Ddi_Bddarray_t *enables;
  
  Pdtutil_Array_t *coneBoundOk;

  struct {
    Ddi_Vararray_t *refVars;
    Ddi_ClauseArray_t *clauseShared;
    Pdtutil_Array_t *clauseSharedNum;
  } pdrClauses;

  struct {
    int nFrames;
    Ddi_Bddarray_t *psNsConstr;
    int enCompute;
  } trItpAbstr;
  
  Ddi_Bddarray_t *eqRings;

  Ddi_Vararray_t *freeDeltaPi;
  Ddi_Bddarray_t *freeDeltaNsLit;

  Ddi_Vararray_t *pi, *ps, *ns, *auxVarPis;
  Ddi_Bddarray_t *psLit, *nsLit;
  Ddi_Varset_t *psvars, *nsvars, *pivars;
  Ddi_Varset_t *coreVars;
  int *freeDelta;
  int *coreVarsCnt;

  Ddi_Bdd_t *concurTr;
  Ddi_Bdd_t *concurStall;
  Ddi_Bdd_t *invarConstr;
  Ddi_Bdd_t *invarConstrForTr;

  Trav_TimeFrameInfo_t *timeFrames;

  Ddi_Vararray_t *boundkOptPis;

  struct {
    Ddi_Bddarray_t *delta;
    Ddi_Bddarray_t *deltaAbstr;
    Ddi_Vararray_t *pi2, *ps2, *ns2;
    Ddi_Vararray_t *pi0, *ps0, *ns0;
  } nnf;

  struct {
    int nScc;
    int *sccSize;
    int *latchSccMap;
    Ddi_Vararray_t *inCoreScc;
  } sccs;

  struct {
    int hintsEnabled;
    int hintsNum;
    int hintsFirst;
    int hintsMaxStep;
    int invar_i;
    int invar0_i;
    int strategy;
    Ddi_Vararray_t *hintsVars;
    Ddi_Bddarray_t *hintsArray;
    Ddi_Bdd_t *hintsConstr;
    Ddi_Bdd_t *saveConstr;
    int *hintsIds;
  } hints;

  int optLevel;
  int unsatGuaranteed;
  int dynAbstr;
  int abstrRef;
  int abstrRefNumRefinedLatches;
  int abstrRefNnf;
  int abstrRefScc;
  int abstrRefGla;
  int abstrRefItp;
  int abstrRefItpMaxIter;
  int trAbstrItp;
  int trAbstrItpMaxFwdStep;
  int abstrPrioRefineNum;
  int levelizeSccs;
  int redRem;
  int abcOpt;
  int itpOpt;
  int itpIncr;
  int useAbstrTr;
  int coneOpt;
  int nRings;
  int nPdrRings;
  int nFreeDelta;
  int lastRingId;
  int lastSafeCone;
  int trBddIsAig;
  int boundkOptNumVars;
  int useAigVars;
  long time_limit;
  long peakAig_limit;

  struct {
    int useRings;
    int useRingsStep;
    int useBwdRings;
    int sameConeFail;
    int coneSubsetBound;
    int coneSubsetSizeTh;
    float coneSubsetPiRatio;
  } igr;
  
  struct {
    int deltaSize;
    int igrRewindRing;
    int igrPiSubsetK;
    int igrPiSubsetRing;
    int igrFpRing;
    int itpOutOfLimits;
  } stats;

};

struct TravItpTravMgr_s {
  Trav_Mgr_t *travMgr;
  Trav_ItpMgr_t *itpMgr;
  Ddi_IncrSatMgr_t *incrSat;
  
  Ddi_Bdd_t *from;
  Ddi_Bdd_t *from0;
  Ddi_Bdd_t *fromBdd;
  Ddi_Bdd_t *restartFrom;
  Ddi_Bdd_t *prevStoredFrom;
  Ddi_Bdd_t *careForBwdCone;
  struct {
    Ddi_Bdd_t *ring;
    Ddi_Bdd_t *preImg;
    int ring_i;
  } splitConeInfo;
  Ddi_Bdd_t *conePiConstr;
  Ddi_Bdd_t *coneSubsetTop;

  Ddi_Bdd_t *trAux;
  Ddi_Bddarray_t *trArray;

  Ddi_Bdd_t *to;
  Ddi_Bddarray_t *toItpSeq;
  Ddi_Bdd_t *reached;
  Ddi_Bdd_t *cone;
  Ddi_Bdd_t *careFwd;
  Ddi_Bdd_t *prevTo;
  Ddi_Bdd_t *prevFrom;
  Ddi_Bdd_t *currFrom;
  Ddi_Bdd_t *careBwd;
  Ddi_Bddarray_t *fwdUnroll;
  Ddi_Bddarray_t *prevFwdUnroll;
  Ddi_Vararray_t *noDynAbstr;

  Ddi_Vararray_t *constrainVars;
  Ddi_Bddarray_t *constrainSubstLits;
  Ddi_Vararray_t *saveConstrainVars;
  Ddi_Bddarray_t *saveConstrainSubstLits;

  Ddi_Vararray_t *imgPartVars;
  Ddi_Bddarray_t *itpSatHints;
  Ddi_Bddarray_t *observedGates;

  struct {
    int computeRestartFrom;
    int innerWithReducedCone;
    int useInitStub;
    int itpExact;
    int enConcurTr;
    int storeRings;
    int newConstrain;
    int implAbstr;
    int useInvarFull;
    int abstrRef;
    int abstrRefGla;
    int useBwdReachAsCone;
    int bwdReach_k;
    int tryInnerRing;
    int enItpSeq;
    int checkCompleteness;
  } settings;
  struct {
    int fwdUnrollSize;
    int step;
    int coneHit;
    int conePiConstrTf_i;
    int conePiConstrStep;
  } stats;
};


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#define tMgrO(travMgr) ((travMgr)->settings.stdout)

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EXTERN Ddi_Bdd_t *Trav_ApproxPreimg(
  Tr_Tr_t * TR,
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * window,
  int doApprox,
  Trav_Settings_t * opt
);
EXTERN void simulateSimple(
  Fsm_Mgr_t * fsmMgr,
  int iterNumberMax,
  int logPeriod,
  char *pattern,
  char *result
);
EXTERN void simulateWave(
  Fsm_Mgr_t * fsmMgr,
  int iterNumberMax,
  int logPeriod,
  int simulationFlag,
  char *init,
  char *pattern,
  char *result
);
EXTERN void simulateWithDac99(
  Fsm_Mgr_t * fsmMgr,
  int iterNumberMax,
  int deadEndMaxNumberOf,
  int logPeriod,
  int depthBreadth,
  int random,
  char *pattern,
  char *result
);
EXTERN Ddi_Bdd_t *TravFromCompute(
  Ddi_Bdd_t * to,
  Ddi_Bdd_t * reached,
  int option
);
EXTERN TravBmcMgr_t *TravBmcMgrInit(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * delta,
  Ddi_Bdd_t * init,
  Ddi_Bddarray_t * initStub,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * cubeConstr
);
EXTERN void TravBmcMgrQuit(
  TravBmcMgr_t * bmcMgr
);
EXTERN int TravBmcMgrAddFrames(
  TravBmcMgr_t * bmcMgr,
  Ddi_IncrSatMgr_t *ddiS,
  int currBound,
  Ddi_Bdd_t * constrCube
);
EXTERN int TravBmcMgrAddFramesFwdBwd(
  TravBmcMgr_t * bmcMgr,
  Ddi_IncrSatMgr_t *ddiS,
  int currBound
);
EXTERN int
TravBmcMgrAddTargetCone(
  TravBmcMgr_t * bmcMgr,
  int nFrames
);
EXTERN int
TravBmcMgrTernarySimInit(
  TravBmcMgr_t * bmcMgr,
  int strategy
);
EXTERN int TravBddOutOfLimits(
  Trav_Mgr_t * travMgr
);
EXTERN Ddi_Bddarray_t *
TravTravTrAbstrLoad(
  Trav_Mgr_t * travMgr,
  int *nfp
);

/**AutomaticEnd***************************************************************/


#endif                          /* _TRAVINT */

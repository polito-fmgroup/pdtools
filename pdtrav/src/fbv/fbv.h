/**CHeaderFile*****************************************************************

  FileName    [fbv.h]

  PackageName [fbv]

  Synopsis    [Header file for fbv]

  Description []
              
  SeeAlso     []  

  Author    [Gianpiero Cabodi and Stefano Quer]

  Copyright [This file was created at the Politecnico di Torino,
    Torino, Italy. 
    The  Politecnico di Torino makes no warranty about the suitability of 
    this software for any purpose.  
    It is presented on an AS IS basis.]
  
  Revision    []

******************************************************************************/

#ifndef _FBV
#define _FBV

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include "mc.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define NMAX 100
#define MAXTHRD 10

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

typedef enum {
  Fbv_HeuristicBmcBig_c,
  Fbv_HeuristicIbm_c,
  Fbv_HeuristicBeem_c,
  Fbv_HeuristicIntel_c,
  Fbv_HeuristicRelational_c,
  Fbv_HeuristicDme_c,
  Fbv_HeuristicQueue_c,
  Fbv_HeuristicNeedham_c,
  Fbv_HeuristicSec_c,
  Fbv_HeuristicPdtSw_c,
  Fbv_HeuristicBddFull_c,
  Fbv_HeuristicBdd_c,
  Fbv_HeuristicCpu_c,
  Fbv_HeuristicOther_c,
  Fbv_HeuristicNone_c
} Fbv_HeuristicSelection_e;



typedef struct Fbv_DecompInfo_t Fbv_DecompInfo_t;

typedef struct {
  /* common */
  char *expName;
  int verbosity;

  struct {
    // shared between FSM and TRAV
    char *clk;                  // command line parameter
    // shared between MC and DDI
    float lazyRate;             // command line parameter
    // shared between TR and TRAV
    int checkFrozen;            // command line parameter
    // shared between TRAV and DDI
    int metaMaxSmooth;          // command line parameter
    // shared between TRAV and DDI
    char satSolver[NMAX];       // command line parameter
    // shared between TRAV and DDI
    int aigAbcOpt;              // command line parameter
  } common;

  struct {
    int nodeCnt[10000];         // INTERNAL FIELD
    int share[10000];           // INTERNAL FIELD
    int share2[10000];          // INTERNAL FIELD
    int zeroE[10000];           // INTERNAL FIELD
    int negE[10000];            // INTERNAL FIELD

    int siftTh;                 // command line parameter
    int siftMaxTh;              // command line parameter
    int siftMaxCalls;           // command line parameter
    int siftTravTh;             // INTERNAL FIELD
    int existClipTh;            // command line parameter
    int saveClipLevel;          // INTERNAL FIELD
    int imgClipTh;              // command line parameter
    int exist_opt_level;        // command line parameter
    int exist_opt_thresh;       // command line parameter
    int info_item_num;          // INTERNAL FIELD
    int dynAbstrStrategy;       // command line parameter
    int ternaryAbstrStrategy;   // command line parameter

    int itpActiveVars;          // command line parameter
    int itpAbc;                 // command line parameter
    int itpCore;                // command line parameter
    int itpMem;                 // command line parameter
    int itpOpt;                 // command line parameter
    char *itpStore;             // command line parameter
    int itpStoreTh;             // command line parameter
    int itpPartTh;              // command line parameter
    int itpAigCore;             // command line parameter
    int itpTwice;               // command line parameter
    int itpIteOptTh;            // command line parameter
    int itpStructOdcTh;         // command line parameter
    int itpMap;                 // command line parameter
    int itpLoad;                // command line parameter
    int itpDrup;                // command line parameter
    int itpCompute;             // command line parameter
    int nnfClustSimplify;       // command line parameter
    int itpCompact;                 // command line parameter
   int itpClust;             // INTERNAL FIELD
   int itpNorm;             // INTERNAL FIELD
   int itpSimp;             // INTERNAL FIELD

    int aigPartial;             // command line parameter -> used in fbv only!
    int aigBddOpt;              // command line parameter
    int aigRedRem;              // command line parameter
    int aigRedRemPeriod;        // command line parameter
    int aigCnfLevel;            // command line parameter
    int satIncremental;         // command line parameter
    int satTimeout;             // command line parameter
    int satTimeLimit;           // command line parameter
    int satVarActivity;         // command line parameter
    int satBinClauses;          // command line parameter -> NEVER used
  } ddi;

  struct {
    int nnf;                    // command line parameter -> used in fbv only!
    int cutAtAuxVar;            // command line parameter
    int insertCutLatches;       // command line parameter
    int cut;                    // command line parameter
    int useAig;                 // INTERNAL FIELD ???
    int cegarStrategy;          // command line parameter
    int auxVarsUsed;            // INTERNAL FIELD
    char *manualAbstr;          // command line parameter -> used in fbv only!
  } fsm;

  struct {
    int targetDecomp;           // command line parameter -> used in fbv only!
    int specEn;                 // command line parameter -> used in fbv only!
    int specConj;               // command line parameter -> used in fbv only!
    int specConjNum;            // INTERNAL FIELD
    int specDisjNum;            // INTERNAL FIELD
    int specPartMaxSize;        // INTERNAL FIELD
    int specPartMinSize;        // INTERNAL FIELD
    int specTotSize;            // INTERNAL FIELD
    int specDecompConstrTh;     // command line parameter -> used in fbv only!
    int specDecompMin;          // command line parameter -> used in fbv only!
    int specDecompMax;          // command line parameter -> used in fbv only!
    int specDecompTot;          // command line parameter -> used in fbv only!
    int specDecompForce;        // command line parameter -> used in fbv only!
    int specDecompEq;           // command line parameter -> used in fbv only!
    int specDecompSort;         // command line parameter -> used in fbv only!
    int specDecompSat;          // command line parameter -> used in fbv only!
    int specDecompCore;         // command line parameter -> used in fbv only!
    int specDecomp;             // command line parameter -> used in fbv only!
    int specDecompIndex;        // command line parameter -> used in fbv only!
    int speculateEqProp;        // command line parameter -> used in fbv only!
    int specSubsetByAntecedents;// command line parameter -> used in fbv only!
    
    int doCoi;                  // command line parameter -> used in fbv only!
    int sccCoi;                 // command line parameter -> used in fbv only!
    int doScc;                  // NEVER used!
    int coisort;                // command line parameter -> used in fbv only!
    Ddi_Varset_t **coiSets;     // INTERNAL FIELD
    int constrCoiLimit;         // command line parameter -> used in fbv only!
    int noConstr;               // NEVER used!
    int constrIsNeg;            // NEVER used!
    int clustThDiff;            // command line parameter -> used in fbv only!
    int clustThShare;           // command line parameter -> used in fbv only! 
    int thresholdCluster;       // command line parameter -> used in fbv only!
    int methodCluster;          // command line parameter -> used in fbv only! 
    int createClusterFile;      // command line parameter -> used in fbv only! 
    char *clusterFile;          // command line parameter -> used in fbv only! 
    char *wrCoi;                // command line parameter -> used in fbv only! 

    int performAbcOpt;          // command line parameter -> used in fbv only!
    int abcSeqOpt;              // INTERNAL FIELD
    int iChecker;               // command line parameter -> used in fbv only!
    int iCheckerBound;          // command line parameter -> used in fbv only!
    int lambdaLatches;          // command line parameter -> used in fbv only!
    int peripheralLatches;      // command line parameter -> used in fbv only!
    int peripheralLatchesNum;   // INTERNAL FIELD
    int periferalRetimeIterations;  // NEVER used!
    int forceInitStub;          // command line parameter -> used in fbv only!
    int retime;                 // command line parameter -> used in fbv only!
    int retimed;                // INTERNAL FIELD
    int terminalScc;            // command line parameter -> used in fbv only!
    int terminalSccNum;         // INTERNAL FIELD
    int findClk;                // command line parameter -> used in fbv only!
    Ddi_Var_t *clkVarFound;     // INTERNAL FIELD
    int clkPhaseSuggested;      // INTERNAL FIELD
    int twoPhaseRed;            // command line parameter -> used in fbv only!
    int twoPhaseForced;         // command line parameter -> used in fbv only!
    int twoPhaseRedPlus;        // command line parameter -> used in fbv only!
    int twoPhaseOther;          // command line parameter -> used in fbv only!
    int twoPhaseDone;           // INTERNAL FIELD
    int nEqChecks;              // INTERNAL FIELD
    int nTotChecks;             // INTERNAL FIELD
    int relationalNs;           // INTERNAL FIELD
    int impliedConstr;          // command line parameter -> used in fbv only!
    int impliedConstrNum;       // INTERNAL FIELD
    int reduceCustom;           // command line parameter -> used in fbv only!
    int ternarySim;             // command line parameter -> used in fbv only!
    int noTernary;              // command line parameter -> used in fbv only!
    int indEqDepth;             // command line parameter -> used in fbv only!
    int indEqMaxLevel;          // command line parameter -> used in fbv only!
    int indEqAssumeMiters;      // command line parameter -> used in fbv only!
    int initMinterm;            // command line parameter -> used in fbv only!
  } pre;

  struct {
    int coiLimit;               // command line parameter
    char trSort[NMAX];          // command line parameter
    int squaringMaxIter;        // command line parameter
    int invar_in_tr;            // command line parameter -> used in fbv only!
    int approxImgMin;           // command line parameter
    int approxImgMax;           // command line parameter
    int approxImgIter;          // command line parameter
    Tr_Tr_t *auxFwdTr;          // INTERNAL FIELD
    char *en;                   // command line parameter -> used in fbv only!
    char *manualTrDpartFile;    // command line parameter -> used in fbv only!
  } tr;

  struct {
    /* trav.general */
    int cex;                    // command line parameter

    /* trav.unused */
    int travConstrLevel;        // command line parameter

    /* these ones maybe are in a wrong place!!! */
    int apprAig;                // command line parameter -> used in fbv only!
    int noInit;                 // command line parameter -> used in fbv only!
    int cntReachedOK;           // INTERNAL FIELD --> replace with countReached?
    int checkProof;             // command line parameter
    
    /* trav.tr */
    int sort_for_bck;           // command line parameter
    int trPreimgSort;           // command line parameter
    int clustTh;                // command line parameter
    int apprClustTh;            // command line parameter
    int imgClustTh;             // command line parameter
    int squaring;               // command line parameter
    int trproj;                 // command line parameter
    int trDpartTh;              // command line parameter
    char *trDpartVar;           // command line parameter
    char *auxVarFile;           // command line parameter
    char *approxTrClustFile;    // command line parameter
    char *bwdTrClustFile;       // command line parameter

    /* trav.aig */
    int targetEn;               // command line parameter
    int targetEnAppr;           // command line parameter
    int dynAbstr;               // command line parameter
    int dynAbstrInitIter;       // command line parameter
    int implAbstr;              // command line parameter
    int ternaryAbstr;           // command line parameter
    int abstrRef;               // command line parameter
    int abstrRefGla;            // command line parameter
    int inputRegs;              // command line parameter
    int travSelfTuning;         // command line parameter
    float lazyTimeLimit;        // command line parameter

    /* trav.bmc */
    int bmcTr;                  // command line parameter -> used in fbv only!
    int bmcTe;                  // command line parameter -> used in fbv only!
    int bmcStep;                // command line parameter -> used in fbv only!
    int bmcFirst;               // command line parameter -> used in fbv only!
    int bmcStrategy;            // command line parameter -> used in fbv only!
    int interpolantBmcSteps;    // command line parameter -> used in fbv only!
    int bmcLearnStep;           // command line parameter

    /* trav.pdr */
    int pdrMaxBlock;          // command line parameter
    int pdrReuseRings;          // command line parameter
    int pdrFwdEq;               // command line parameter
    int pdrUnfold;              // command line parameter
    int pdrIndAssumeLits;       // command line parameter
    int pdrPostordCnf;          // command line parameter
    int pdrInf;                 // command line parameter
    int pdrCexJustify;          // command line parameter
    int pdrGenEffort;           // command line parameter
    int pdrIncrementalTr;       // command line parameter
    int pdrShareReached;        // command line parameter
    int pdrUsePgEncoding;       // command line parameter
    int pdrBumpActTopologically;    // command line parameter
    int pdrSpecializedCallsMask;    // command line parameter
    int pdrRestartStrategy;     // command line parameter

    /* trav.itp */
    int itpBdd;                 // command line parameter
    int itpPart;                // command line parameter
    int itpPartCone;            // command line parameter
    int itpAppr;                // command line parameter
    int itpExact;               // command line parameter
    int itpNew;                 // command line parameter
    int itpMaxStepK;            // command line parameter
    int itpForceRun;            // command line parameter
    int itpCheckCompleteness;   // command line parameter
    int itpGfp;                 // command line parameter
    int itpConstrLevel;         // command line parameter
    int itpShareReached;        // command line parameter
    int itpWeaken;              // command line parameter
    int itpStrengthen;          // command line parameter
    int itpReImg ;              // command line parameter
    int itpFromNewLevel;        // command line parameter
    int itpGenMaxIter;          // command line parameter
    int itpEnToPlusImg;         // command line parameter
    int itpStructAbstr;         // command line parameter
    int itpTuneForDepth;        // command line parameter
    int itpInitAbstr;           // command line parameter
    int itpEndAbstr;           // command line parameter
    int itpTrAbstr;             // command line parameter
    int itpBoundkOpt;           // command line parameter
    int itpExactBoundDouble;    // command line parameter
    int itpConeOpt;             // command line parameter
    int itpUseReached;          // command line parameter
    int itpReuseRings;          // command line parameter
    int itpInductiveTo;         // command line parameter
    int itpInnerCones;          // command line parameter
    int itpRefineCex;           // command line parameter
    int itpUsePdrReached;       // command line parameter
    int itpRpm;                 // command line parameter

    /* trav.igr */
    int igrSide;                // command line parameter
    int igrBwdRefine;           // command line parameter
    int igrRewind;              // command line parameter
    int igrRewindMinK;          // command line parameter
    int igrRestart;             // command line parameter
    int igrUseRings;            // command line parameter
    int igrUseRingsStep;        // command line parameter
    int igrUseBwdRings;         // command line parameter
    int igrAssumeSafeBound;     // command line parameter

    int igrConeSubsetBound;
    int igrConeSubsetSizeTh;
    float igrConeSubsetPiRatio;
    float igrConeSplitRatio;

    int igrGrowCone;            // command line parameter
    int igrGrowConeMaxK;        // command line parameter
    float igrGrowConeMax;       // command line parameter
    int igrItpSeq;              // command line parameter
    int igrMaxIter;             // command line parameter
    int igrMaxExact;            // command line parameter
    int igrFwdBwd;              // command line parameter

    /* trav.lemmas */
    int strongLemmas;           // INTERNAL FIELD
    int lemmasCare;             // command line parameter -> used in fbv only!
    int implLemmas;             // command line parameter -> used in fbv only!
    float lemmasTimeLimit;      // command line parameter

    /* trav.bdd */
    char fromSel;               // command line parameter
    int noCheck;                // command line parameter
    int mism_opt_level;         // command line parameter
    int groupbad;               // INTERNAL FIELD
    int prioritizedMc;          // command line parameter
    int pmcPrioThresh;          // command line parameter
    int pmcMaxNestedFull;       // command line parameter
    int pmcMaxNestedPart;       // command line parameter
    int pmcNoSame;              // command line parameter
    int countReached;           // command line parameter
    int autoHint;               // command line parameter
    int hintsStrategy;          // command line parameter
    int hintsTh;                // command line parameter
    char *hintsFile;            // command line parameter
    Ddi_Bddarray_t *hints;      // INTERNAL FIELD
    Ddi_Varset_t *auxPsSet;     // INTERNAL FIELD
    int gfpApproxRange;         // command line parameter
    int fbvMeta;                // command line parameter
    Ddi_Meta_Method_e metaMethod;   // command line parameter
    int constrain;              // command line parameter
    int constrain_level;        // command line parameter
    int apprOverlap;            // command line parameter
    int approxNP;               // command line parameter
    int approxMBM;              // command line parameter
    int apprGrTh;               // command line parameter
    int approxPreimgTh;         // command line parameter
    int maxFwdIter;             // command line parameter
    int nExactIter;             // command line parameter
    int siftMaxTravIter;        // INTERNAL FIELD
    int implications;           // command line parameter
    int imgDpartTh;             // command line parameter
    int imgDpartSat;            // command line parameter
    int bound;                  // command line parameter
    int strictBound;            // command line parameter
    int guidedTrav;             // INTERNAL FIELD
    int univQuantifyTh;         // command line parameter
    int opt_img;                // command line parameter
    int opt_preimg;             // command line parameter
    char *wP;                   // command line parameter
    char *wR;                   // command line parameter
    char *wC;                   // command line parameter
    char *wBR;                  // command line parameter
    char *wU;                   // command line parameter
    char *wOrd;                 // command line parameter
    char *rPlus;                // command line parameter
    char *rPlusRings;           // command line parameter
    char *invarFile;            // command line parameter
    char *storeCNF;             // command line parameter
    int storeCNFTR;             // command line parameter
    char storeCNFmode;          // command line parameter
    int storeCNFphase;          // command line parameter
    int maxCNFLength;           // command line parameter

    /* trav.internals */
    Ddi_Varset_t *univQuantifyVars; // INTERNAL FIELD
    int univQuantifyDone;       // INTERNAL FIELD
  } trav;

  struct {
    Mc_VerifMethod_e method;    // command line parameter
    Mc_VerifMethod_e saveMethod;    // INTERNAL FIELD

    int idelta;                 // command line parameter -> used in fbv only!
    int ilambda;                // command line parameter -> used in fbv only!
    int ilambdaCare;            // command line parameter -> used in fbv only!
    char *nlambda;              // command line parameter -> used in fbv only!
    char *ninvar;               // command line parameter -> used in fbv only!
    int range;                  // command line parameter -> used in fbv only!
    int all1;                   // command line parameter -> used in fbv only!
    int compl_invarspec;        // command line parameter -> used in fbv only!
    int saveComplInvarspec;     // INTERRNAL FIELD
    char *invSpec;              // command line parameter -> used in fbv only!
    char *ctlSpec;              // command line parameter -> used in fbv only!
    char *rInit;                // command line parameter -> used in fbv only!
    char *ord;                  // command line parameter -> used in fbv only!

    char *strategy;             // command line parameter -> used in fbv only!
    char *wRes;                 // command line parameter -> used in fbv only!
    char *wFsm;                 // command line parameter -> used in fbv only!
    char *checkInv;             // command line parameter -> used in fbv only!
    int fold;                   // command line parameter -> used in fbv only!
    int foldInit;               // command line parameter -> used in fbv only!
    int writeBound;             // INTERNAL FIELD
    int useChildProc;           // INTERNAL FIELD
    int useChildCall;           // INTERNAL FIELD

    int aig;                    // command line parameter
    int bmc;                    // command line parameter -> used in fbv only!
    int pdr;                    // command line parameter -> used in fbv only!
    int igrpdr;                    // command line parameter -> used in fbv only!
    int bdd;                    // INTERNAL FIELD
    int inductive;              // command line parameter -> used in fbv only!
    int interpolant;            // command line parameter -> used in fbv only!
    int lazy;                   // command line parameter
    int custom;                 // command line parameter -> used in fbv only!
    int lemmas;                 // command line parameter
    int qbf;                    // command line parameter -> used in fbv only!
    int gfp;                    // command line parameter -> used in fbv only!
    int diameter;               // command line parameter -> used in fbv only!
    int checkMult;              // command line parameter -> used in fbv only!
    int checkDead;              // command line parameter -> used in fbv only!
    int exactTrav;              // command line parameter -> used in fbv only!

    char *abcOptFilename;       // command line parameter -> used in fbv only!
    int nogroup;                // command line parameter -> used in fbv only!
    int combinationalProblem;   // INTERNAL FIELD
    int enFastRun;              // command line parameter
    int fwdBwdFP;               // command line parameter
    int tuneForEqCheck;         // command line parameter -> NEVER used!
    int initBound;              // command line parameter
    int itpSeq;                 // command line parameter
    int itpSeqGroup;            // command line parameter
    float itpSeqSerial;         // command line parameter
    float decompTimeLimit;      // command line parameter -> used in fbv only!

    int cegar;                  // command line parameter -> used in fbv only!
    int cegarPba;               // command line parameter -> used in fbv only!
  } mc;

  struct {
    int expertLevel;
    char **expertArgv;
    int expertArgc;

    int deepBound;
    int doRunBmc;
    int doRunItp;
    int doRunIgr;
    int doRunBdd;
    int doRunBdd2;
    int doRunInd;
    int doRunSim;
    int doRunSyn;
    int doRunLemmas;
    int doRunLms;
    int doRunPdr;
    int doRunBddPfl;
    int doRunItpPfl;
    int doRunPdrPfl;
    int doRunBmcPfl;
    int pflMaxThreads;
    int bmcSteps;
    int lemmasSteps;
    int bmcTimeLimit;
    int bmcMemoryLimit;
    int bddTimeLimit;
    int bddNodeLimit;
    int bddMemoryLimit;
    int totMemoryLimit;
    int totTimeLimit;
    int pdrTimeLimit;
    int itpTimeLimit;
    int itpPeakAig;
    int itpMemoryLimit;
    int itpBoundLimit;
    int indSteps;
    int indTimeLimit;
    char *thrdDis[MAXTHRD];
    int thrdDisNum;
    char *thrdEn[MAXTHRD];
    int thrdEnNum;
    int ddiMgrAlign;
    int enPfl;
    int enItpPlus;
  } expt;

  struct {
    long startTime;
    long setupTime;
    long fbvStartTime;
    long fbvStopTime;
    int verificationOK;
  } stats;


} Fbv_Globals_t;

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EXTERN int FbvApproxBackwExactForwVerify(
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Fbv_Globals_t * opt
);
EXTERN int FbvApproxForwApproxBckExactForwVerify(
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  int doApproxBck,
  Fbv_Globals_t * opt
);
EXTERN int Fbv_DagSize(
  DdNode * node,
  int nvar,
  Fbv_Globals_t * opt
);
EXTERN DdNode *Fbv_bddExistAbstractWeak(
  DdManager * manager,
  DdNode * f,
  DdNode * cube,
  st_table * table,
  int split,
  int splitPhase,
  int phase,
  Fbv_Globals_t * opt
);
EXTERN Ddi_Expr_t *Fbv_EvalCtlSpec(
  Tr_Tr_t * TR,
  Ddi_Expr_t * e,
  Fbv_Globals_t * opt
);
EXTERN int Fbv_RunBddMc(
  Mc_Mgr_t * mcMgr
);
EXTERN int Fbv_ResNum(
  char *prefix,
  int num
);
EXTERN int Fbv_CheckFsmCex(
  Fsm_Mgr_t * fsmMgr,
  Trav_Mgr_t * travMgr
);
EXTERN int Fbv_CheckFsmProp(
  Fsm_Mgr_t * fsmMgr
);

/**AutomaticEnd***************************************************************/

#endif                          /* _FBV */

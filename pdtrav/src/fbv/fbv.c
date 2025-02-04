/**CFile***********************************************************************

  FileName    [fbv.c]   

  PackageName [fbv]

  Synopsis    [Fbv package main]

  Description []

  SeeAlso     []

  Author      [Gianpiero Cabodi and Stefano Quer]

  Copyright   [
    This file was created at the Politecnico di Torino,
    Torino, Italy.
    The Politecnico di Torino makes no warranty about the suitablity of
    this software for any purpose.
    It is presented on an AS IS basis
  ]

  Revision  []

******************************************************************************/

#include <signal.h>
#include <time.h>
#include "fbvInt.h"
#include "fsmInt.h"
#include "travInt.h"
#include "ddiInt.h"
#include "baigInt.h"
#include "trInt.h"
#include "expr.h"
#include "st.h"
#include "pdtutil.h"

// StQ 2010.07.07
// #include "csatPort.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define FBV_IMG_CLIP_THRESH 20000000
#define FBV_SIFT_THRESH 500000
#define ENABLE_INVAR 0
#define FULL_COFACTOR 0
#define PARTITIONED_RINGS 0
#define TRAV_CONSTRAIN_LEVEL 1
#define DOUBLE_MGR 1
#define LINEMAX 10000

#define FILEPAT   "/tmp/pdtpat"
#define FILEINIT  "/tmp/pdtinit"

#define LOG_CONST(f,msg) \
if (Ddi_BddIsOne(f)) {\
  fprintf(stdout, "%s = 1\n", msg);            \
}\
else if (Ddi_BddIsZero(f)) {\
  fprintf(stdout, "%s = 0\n", msg);            \
}


/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

typedef struct node_info_item {
  DdNode *cu_bdd;
  int res, num0, num1, used, top, toptop, ph0, ph1;
  struct DdNode *merge;
} node_info_item_t;

typedef struct coi_scc_info {
  int size;
  char *visited;
  int *sc;
  int *stack;
  int stackIndex;
  int cnt0, cnt1;
  int *low;
  int *pre;
} coi_scc_info_t;

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

extern int fbvCustom;

static int hwmccOut = 0;

static int bmcSerialTime = 0;
static int bmcParallTime = 0;
static int umcSerialTime = 0;
static int umcParallTime = 0;

static char *help[] = {
  "-h [-]<opt>",
  "help for option",
  NULL,

  "-verbosity [0, 8]",
  "verbosity level; 0=none, 8=debug level", NULL,

  "-source <filename>",
  "read options from <file>", NULL,

  "-f",
  "Exact forward verification", NULL,
  "-fe",
  "Exact forward verification (no interpolant with aig)", NULL,
  "-b",
  "Exact backward verification", NULL,
  "-bf",
  "Approx backward exact forward verification", NULL,
  "-fb",
  "Approx forward exact backward verification (default)", NULL,
  "-fbf",
  "Approx forward approx backward exact forward verification", NULL,
  "-t",
  "Traversal: forward reachable state computation (no verif)", NULL,
  "-bt",
  "Backward Traversal: backw. state computation (no verif)", NULL,
  "-tg",
  "Guided Traversal: forward reachable state computation (no verif)", NULL,
  "-all1 <num>",
  "invarspec = !(state[0] & state[1] & ... & state[<num>-1])", NULL,
  "-specConj",
  "check conj decomp of properyy", NULL,
  "-specDecomp  <index>/<npart>",
  "try disj. decomp of property and take <index> partition as spec", NULL,
  "-specDecompConstrTh <th>",
  "Select spec decomp max constr size (default: 50000)", NULL,
  "-specDecompMin <n>",
  "Select target spec decomp min part num (0: off)", NULL,
  "-specDecompMax <n>",
  "Select target spec decomp max part num (0: off)", NULL,
  "-specDecompTot <h>",
  "Select spec decomp heuristic (0: off)", NULL,
  "-specDecompForce <v>",
  "Force spec decomposition with multiple given properties (0: off)", NULL,
  "-specDecompEq <v>",
  "try eq. decomp of property (0: off)", NULL,
  "-specDecompSat <n>",
  "try conj. decomp of property by SAT (n: #part)", NULL,
  "-specDecompCore <n>",
  "try core decomp of property by SAT (n: level)", NULL,
  "-specSubsetByAntecedents <n>",
  "try target decomp using antecedents subsetting", NULL,
  "-specDecompSort <v>",
  "en/dis sorting of decomposed property (0: disabled)", NULL,
  "-trPreimgSort",
  "Do sorting before Preimg Calls", NULL,
  "-trSort (iwls95|pdt|ord|size|minmax|none)",
  "Select tr sorting heuristic", NULL,
  "-ni",
  "Complement (NOT) invariant spec", NULL,
  "-implications",
  "propagate implications while partitioning TR", NULL,
  "-countR <v>",
  "count/estimate reached states.",
  "This may require generating MONOLITHIC BDDs!", NULL,
  "-aig",
  "use aig based representation", NULL,
  "-enFastRun <time>",
  "emable fast run for <time> seconds", NULL,
  "-sat <engi≤ne>",
  "choose SAT solver (zchaff|csat|berkmin|minisat), default=zchaff", NULL,
  "-bmc <k>",
  "SAT based Bounded Model Checking (bound k)", NULL,
  "-hwmccSetup",
  "initialize setup for HWMCC competition", NULL,
  "-task <arguments>",
  "select a task (not model checking) with command string", NULL,
  "-strategy <s>",
  "select a given strategy/engine among (bdd|ic3|itp(0..6)|igr(0..6)",
  "for engines itp and igr",
  "(Interpolation with Guided Refinement: unpublished, paper under preparation)",
  "a digit selects the substrategy (interpolation effort)",
  "default is 0, a further set of characters can be added",
  "c: phase abstr, p: push itp components to next iteration, ",
  "r: restart from reached, instead as to",
  "e.g.: itp4ic, igr4ic (most used) ",
  "multi: go for multiple properties (under construction) ",
  "thrd: threaded multiple engines",
  "expt: single threaded expert", NULL,

  "-engine <e>",
  "select a given strategy/engine among (bdd|ic3|itp(0..6)|igr(0..6)",
  "for engines itp and igr",
  "(Interpolation with Guided Refinement: unpublished, paper under preparation)",
  "a digit selects the substrategy (interpolation effort)",
  "default is 0, a further set of characters can be added",
  "c: phase abstr, p: push itp components to next iteration, ",
  "r: restart from reached, instead as to",
  "e.g.: itp4ic, igr4ic (most used) ",
  "multi: go for multiple properties (under construction) ",
  "thrd: threaded multiple engines",
  "expt: single threaded expert", NULL,

  "-bmcStrategy <s>",
  "select BDM strategy (default 0)", NULL,
  "-itpRefineCex <n>",
  "ITP CEX refinement optimization (n=0: disabled)", NULL,
  "-itpSeq <k>",
  "ITP SEQ based approach (k)", NULL,
  "-itpSeqGroup <n>",
  "ITP SEQ Group size (default: 0=disabled)", NULL,
  "-itpSeqSerial <r>",
  "ITP SEQ Serial rate", NULL,
  "-interpolantBmcSteps <k>",
  "depth of interpolant based init and target constrains", NULL,
  "-satTimeout <l>",
  "timeout for SAT calls (<l> is a level in the range 0..3, 0=no limit)",
  NULL,
  "-satTimeLimit <t>",
  "timeLimit for SAT calls (<t> is a level in seconds, default 30)",
  NULL,
  "-satNoIncr",
  "Disable Incremental SAT",
  NULL,
  "-lazy <gr. rate>",
  "SAT based lazy Model Checking", NULL,
  "-lazyRate <gr. rate>",
  "set lazy rate", NULL,
  "-checkInv",
  "Check reached plus by SAT  based GFP", NULL,
  "-gfp",
  "Strengthen reached plus by SAT  based GFP", NULL,
  "-certify <file>",
  "certify proof read from <file>", NULL,
  "-qbf",
  "QBF based Model Checking", NULL,
  "-diameter <s>",
  "SAT based Diameter Model Checking (step <s>)", NULL,
  "-inductive <s>",
  "SAT based Inductive Model Checking (step <s>)", NULL,
  "-interpolant",
  "SAT based Interpolant Model Checking", NULL,
  "-custom <method>",
  "Custom behavior for combinational circuits", NULL,
  "-indEqDepth <depth>",
  "Enable eq node check for induction depth <depth>", NULL,
  "-indEqMaxLevel <level>",
  "Max input level of SAT checks for induction depth <depth>", NULL,
  "-indEqAssumeMiters <val>",
  "Enable/disable assuming miters in inductive eq checks", NULL,
  "-lemmas <depth>",
  "Enable lemma generation for induction depth <depth>", NULL,
  "-lemmasCare <v>",
  "Enable using care set generated from inductive invariants", NULL,
  "-strongLemmas <v>",
  "Enable using strong lemmas", NULL,
  "-lemmasTimeLimit <time>",
  "Set lemmas generation time limit (seconds)", NULL,
  "-lazyTimeLimit <time>",
  "Set lazy generation time limit (seconds)", NULL,
  "-itpTimeLimit <time>",
  "Set itp/igr mc time limit (seconds)", NULL,
  "-totMemoryLimit <mem>",
  "Set tot mc mem limit (megabytes)", NULL,
  "-thrdEnItpPlus <val>",
  "enable enforced Interpolation portfolio", NULL,
  "-thrdEnPfl <val>",
  "enable portfolio engines (seconds)", NULL,
  "-itpPeakAig <num>",
  "Set itp/igr mc aig# limit (seconds)", NULL,
  "-insertCutLatches <val>",
  "Find cut points and insert redundant latches", NULL,
  "-decompTimeLimit <time>",
  "Set decomposed property time limit (seconds)", NULL,
  "-travSelfTuning <leve>",
  "Set trav self tuning level (0 = no self tuning)", NULL,
  "-travConstrLevel <leve>",
  "Set trav self tuning level (0 = no self tuning)", NULL,
  "-implLemmas",
  "Enable implication lemma generation", NULL,
  "-aigBddOpt <level>",
  "set aig Bdd opt level (default 0)", NULL,
  "-abc <filename>",
  "output file for abc fsm optimization", NULL,
  "-performAbcOpt <level>",
  "performs initial Abc optimization (default 3 0)", NULL,
  "-iChecker <bound>", NULL,
  "-aigCnfLevel <level>",
  "set aig to cnf level (default 1)", NULL,
  "-aigRedRem <level>",
  "set aig redundancy removal level (default 0)", NULL,
  "-aigRedRemPeriod <n>",
  "set aig redundancy removal period (default 1)", NULL,
  "-abstrRefLoad <filename>",
  "load abstraction refinement vars (default NULL = disabled)", NULL,
  "-abstrRefStore <filename>",
  "store abstraction refinement vars (default NULL = disabled)", NULL,
  "-abstrRef <level>",
  "set aig abstraction refinement level (default 0 = disabled)", NULL,
  "-abstrRefGla <level>",
  "set GLA abstraction refinement level (default 0 = disabled)", NULL,
  "-abstrRefItp <level>",
  "set ITP abstraction refinement level (0..100: default 0 = disabled)", NULL,
  "-abstrRefItpMaxIter <max>",
  "set ITP abstraction refinement max iterations (default 4)", NULL,
  "-trAbstrItp <nframes>",
  "set ITP TR abstraction frames (default 0 = disabled)", NULL,
  "-trAbstrItpOpt <level>",
  "set ITP TR abstraction optimization level (default 0 = disabled)", NULL,
  "-trAbstrItpFirstFwdStep <max>",
  "set ITP TR abstraction first Fwd step (default 0)", NULL,
  "-trAbstrItpMaxFwdStep <max>",
  "set ITP TR abstraction max Fwd step (default 0 = just first fwd step)", NULL,
  "-trAbstrItpLoad <filename>",
  "load ITP TR abstraction from file (default NULL = disabled)", NULL,
  "-trAbstrItpStore <filename>",
  "store ITP TR abstraction to file (prefix) (default NULL = disabled)", NULL,
  "-dynAbstr <level>",
  "set aig dynamic abstraction level (default 0 = disabled)", NULL,
  "-dynAbstrInitIter <iter>",
  "start aig dynamic abstraction at inner iteration <iter> (default 2)",
  NULL,
  "-ternaryAbstrStrategy <strat>",
  "set aig ternary abstraction strategy (default 1)", NULL,
  "-dynAbstrStrategy <strat>",
  "set aig dynamic abstraction strategy (default 0)", NULL,
  "-implAbstr <level>",
  "set aig implication abstraction level (default 0 = disabled)", NULL,
  "-inputRegs <n>",
  "set aig input register optimization handling (default 0 = disabled)",
  NULL,
  "-itpBdd <n>",
  "set interpolant Bdd use level (default 0 = disabled)", NULL,
  "-itpPart <l>",
  "set interpolant partitioning level (default 1 = standard)", NULL,
  "-itpPartTh <th>",
  "set interpolant partitioning th (default 100000)", NULL,
  "-itpIteOptTh <th>",
  "set interpolant ITE opt th (default 50000)", NULL,
  "-itpStructOdcTh <th>",
  "set Odc opt th (default 1000)", NULL,
  "-itpStoreTh <th>",
  "set interpolant store th (default 100000)", NULL,
  "-itpActiveVars <n>",
  "set interpolant use most active vars (default 0 = disabled)", NULL,
  "-itpAppr <th>",
  "set interpolant approx level (default 0 = disabled)", NULL,
  "-itpAbc <l>",
  "set interpolant computation using abc (default 0 = disabled)", NULL,
  "-itpCore <l>",
  "set interpolant core computation (default 0 = disabled)", NULL,
  "-itpTwice <l>",
  "set interpolant twice computation (default 0 = disabled)", NULL,
  "-itpRpm <l>",
  "set interpolant reparam threshold (default 0 = disabled)", NULL,
  "-itpAigCore <l>",
  "set interpolant aig core computation (default 0 = disabled)", NULL,
  "-itpMem <l>",
  "set interpolant file/mem opt (default 0 = disabled)", NULL,
  "-itpMap <l>",
  "set itpOpt and itpMem opt mapping (default 0 = disabled)", NULL,
  "-itpCompact <l>",
  "set interpolant compaction opt clust/norm/simp (default 0 = disabled)", NULL,
  "-itpOpt <l>",
  "set interpolant optimization level (default 0 = disabled)", NULL,
  "-itpStore <name>",
  "set name prefix for aiger files storing interpolants (default NULL = disabled)",
  NULL,
  "-bmcItpRingsPeriod <p>",
  "period for itp rings as a constraint (default 0 - disabled)", NULL,
  "-itpLoad <name>",
  "set file name for aiger file loading interpolant rings (default NULL = disabled)",
  NULL,
  "-itpStoreRings <fname>",
  "set interpolant rings store file name (default NULL)", NULL,
  "-itpDrup <v>",
  "enable DRUP-based interpolation (default 0 = disabled)", NULL,
  "-itpCompute <l>",
  "load A and B from file and compute a new interpolant (default 0 = disabled)",
  NULL,
  "-itpExact <val>",
  "set interpolant exact level (default 0 = disabled)", NULL,
  "-itpNew <val>",
  "set interpolant new (from) level (default 0 = disabled)", NULL,
  "-itpStructAbstr <val>",
  "set interpolant structural abstr level (for fwd tr) (default 2 = enaabled)",
  NULL,
  "-itpInnerCones <val>",
  "set itpInnerCones value (default 0 = disabled)", NULL,
  "-itpReuseRings <val>",
  "set itpReuseRings value (default 0 = disabled)", NULL,
  "-itpInductiveTo <val>",
  "set itpInductiveTo value (default 0 = disabled)", NULL,
  "-itpInitAbstr <val>",
  "set init state abstraction level (default 1 = medium)", NULL,
  "-itpEndAbstr <val>",
  "set init state abstraction max step (default 0 = disables)", NULL,
  "-itpTrAbstr <val>",
  "set interpolation-based TR abstraction level (default 0 = none)", NULL,
  "-itpTuneForDepth <val>",
  "set itpTuneForDepth value (default 0 = disabled)", NULL,
  "-itpConeOpt <val>",
  "set itpConeOpt value (default 1 = medium)", NULL,
  "-itpBoundkOpt <val>",
  "set itpBoundkOpt value (default 0 = disabled)", NULL,
  "-itpUseReached <val>",
  "set itpUseReached value (default 0 = disabled)", NULL,
  "-itpForceRun <step>",
  "set interpolant forceRun step (default -1 = disabled, -2 = all steps)",
  NULL,
  "-itpMaxStepK <step>",
  "set interpolant maximum coe k step (default -1 = disabled)",  NULL,
  "-itpCheckCompleteness <val>",
  "en/dis completeness check after ITP proof (default 0 = disabled)", NULL,
  "-itpGfp <val>",
  "en/dis gfp strengthening after ITP proof (default 0 = disabled)", NULL,
  "-itpConstrLevel <level>",
  "itp constr level (default 0 = disabled)", NULL,
  "-itpGenMaxIter <maxI>",
  "generalized itp max iterations (default 0 = disabled)", NULL,
  "-itpEnToPlusImg <val>",
  "enable/disable itp en toplus image (default enabled)", NULL,
  "-itpShareReached <val>",
  "enable/disable itp sharing of reached (default disabled)", NULL,
  "-itpWeaken <val>",
  "enable/disable itp weakening iteration (default disabled)", NULL,
  "-itpStrengthen <val>",
  "enable/disable itp strengthening iteration (default disabled)", NULL,
  "-nnfClustSimplify <val>",
  "enable/disable nnf cluster based simplification (default 1)", NULL,
  "-pdrShareReached <val>",
  "enable/disable pdr sharing of reached (default disabled)", NULL,
  "-pdrFwdEq <val>",
  "enable/disable pdr tightening using forward equiv (default disabled)",
  NULL,
  "-pdrGenEffort <val>",
  "set pdr generalization effort (default 2)", NULL,
  "-pdrIncrementalTr <val>",
  "en/dis incremental load of TR clauses (default 1)", NULL,
  "-pdrInf <val>",
  "en/dis use of infinite (inductive) time frame (default 0)", NULL,
  "-pdrUnfold <val>",
  "en/dis unfolding of property in IC3 (default 1)", NULL,
  "-pdrIndAssumeLits <val>",
  "en/dis iteratively assuming clause lits in generalization (default 0)",
  NULL,
  "-pdrPostordCnf <val>",
  "en/dis postorder cnf var assignment (default 0: preorder)", NULL,
  "-pdrCexJustify <val>",
  "en/dis use of pdr cex reduction by justification (default 1)", NULL,
  "-pdrReuseRings <val>",
  "set pdrReuseRings value (default 0 = disabled)", NULL,
  "-pdrMaxBlock <val>",
  "set pdrMaxBlock value (default 0 = disabled)", NULL,
  "-igrGrowConeMaxK <val>",
  "stop from cone at bound (default 0)", NULL,
  "-igrGrowConeMax <val>",
  "max relative cone bound increase (default 0.5)", NULL,
  "-igrGrowCone <val>",
  "cone initial grow for each refinement (default 1)", NULL,
  "-igrItpSeq <val>",
  "use itp seq in igr (default 0)", NULL,
  "-igrRestart <val>",
  "en/dis igr restart option (default 0)", NULL,
  "-igrRewind <val>",
  "en/dis igr rewind option (default 1)", NULL,
  "-igrRewindMinK <val>",
  "en igr rewind if K bound > <val> (default -1: disabled)", NULL,
  "-igrUseRings <val>",
  "level of constraining/anding of cone with fwd rings (default 0)", NULL,
  "-igrUseRingsStep <val>",
  "max steplevel of constraining/anging of cone with fwd rings (default 0)", NULL,
  "-igrUseBwdRings <val>",
  "level of constraining/anding of cone with bwd rings (default 0)", NULL,
  "-igrAssumeSafeBound <val>",
  "assume already proved bounds while using bwd cone for interpolation (default 0)", NULL,
  "-igrConeSubsetBound <val>",
  "cone bound at which emable Cone Pi-based subsetting (default: disables)", NULL,
  "-igrConeSubsetSizeTh <val>",
  "cone size threshold for  Cone Pi-based subsetting (default: 100000)", NULL,
  "-igrConeSubsetPiRatio <val>",
  "cratio of cone PIs used for subsetting (default: 0.5)", NULL,
  "-igrConeSplitRatio <val>",
  "cratio of cone split (default: 1.0 - disabled)", NULL,
  "-igrSide <val>",
  "max refinement iterations for same (side) ring (default 0)", NULL,
  "-igrBwdRefine <val>",
  "max refinement iterations for same (side) ring (default 0)", NULL,
  "-igrMaxIter <val>",
  "max refinement iterations for given k bwd bound (default 16)", NULL,
  "-igrMaxExact <val>",
  "max exact unrollings in igr refinement (default 2)", NULL,
  "-igrFwdBwd <val>",
  "0: fwd, 1: bwd (not yet implemented), 2: fwd-bwd", NULL,
  "-ternaryAbstr <level>",
  "set aig ternary abstraction level (default 0 = disabled)", NULL,
  "-te <n>",
  "target enlargment in SAT based verif. (k: # of trav iter.)", NULL,
  "-teAppr <n>",
  "overapprox target enlargment in SAT based verif. (k: # of trav iter.)", NULL,
  "-td <n>",
  "target decomposition. (k: # of trav iter.)", NULL,
  "-bmcStep <s>",
  "step of SAT based BMC", NULL,
  "-bmcTrAbstrPeriod <p>",
  "period for tr abstraction as a constraint (default 0 - use tr bound)", NULL,
  "-bmcTrAbstrInit <p>",
  "initial step for tr abstraction as a constraint (default 0)", NULL,
  "-bmcLearnStep <s>",
  "step of BMC pre-loaded during SAT based BMC", NULL,
  "-bmcFirst <s>",
  "initial bound of SAT based BMC", NULL,
  "-bmcTe <k>",
  "target enlargment in BMC", NULL,
  "-noInit",
  "no initial(reset) state used", NULL,
  "-noTernary",
  "no ternary simulation done", NULL,
  "-ternarySim <l>",
  "ternary simulation strategy (default 1, 0: disabled)", NULL,
  "-noCheck",
  "no property check done (but property used as init state in back t.", NULL, 
  "-mult",
  "check combinational multiplier", NULL,
  "-frozen",
  "check frozen (unchanging) state vars", NULL,
  "-pmc",
  "enable prioritized model check", NULL,
  "-pmcNoSame",
  "disable search on same ring in pmc check", NULL,
  "-pmcMF <n>",
  "pmc max nested full for prioritized (default 100)", NULL,
  "-pmcMP <n>",
  "pmc max nested part for prioritized (default 5)", NULL,
  "-pmcPT <th>",
  "pmc threshold to enable prioritized (|from|>th, def 5000)", NULL,

  "-strict",
  "not documented", NULL,
  "-range",
  "not documented", NULL,

  "-cl",
  "constrain level", NULL,

  "-meta [sch] <group size>",
  "enable meta BDDs", NULL,

  "-metaMaxSmooth <n>",
  "max num of smoothed vars (until partitioning) in meta and-exist", NULL,

  "-nogroup",
  "disable PS/NS variable groups (no group sifting)", NULL,

  "-fromsel (b|c|r)",
  "select from as best, new or reached (with -f or -t options)", NULL,

  "-invar_in_tr",
  "first component of TR is an invariant (see INVAR in SMV)", NULL,

  "-invarfile <file>",
  "read invariant (see INVAR in SMV) from BDD or AIG <file>", NULL,

  "-invarconstr <file>",
  "read invariant (see INVAR in SMV) from BDD or AIG <file>", NULL,

  "-opt_img <th>",
  "Enable two step optimized exact image (approx+exact)",
  "if |from| > <th>", NULL,

  "-auxVarFile <file>",
  "read list of state vars where to put aux vars from <file>", NULL,

  "-bwdTrClustFile <file>",
  "read partitions for bck Tr from <file>", NULL,

  "-approxTrClustFile <file>",
  "read partitions for approx Tr from <file>", NULL,

  "-manualTrDpartFile <file>",
  "read partitions for dpart Tr from <file>", NULL,

  "-manualAbstr <file>",
  "read abstraction vars from <file>", NULL,

  "-hintsFile <file>",
  "read hints vars from <file>", NULL,

  "-opt_preimg <th>",
  "Enable two step optimized exact image (approx+exact)",
  "if |from| > <th>", NULL,

  "-mism_opt_level <level>",
  "Optimization level for mismatch generation: 1..2",
  "1 (default) is min opt for memory and no overhead", NULL,

  "-exist_opt_level <level>",
  "Optimization level for exist operation: 0..3",
  "2 (default) is avg opt for memory and more overhead", NULL,

  "-exist_opt_thresh <size>",
  "Bdd size for inner exist optimizations: default 1000", NULL,

  "-existClipTh <size>",
  "Bdd size for inner exist clip: default -1 (no clip)", NULL,

  "-constrain",
  "compute IMG(TR,from) as range(TR|from) (with -f or -t)", NULL,

  "-coi",
  "enable functional cone-of-influence reduction (defaults to -cois)", NULL,
  "-sccCoi <v>",
  "enable/disable scc reduction for cone-of-influence (default 1)", NULL,

  "-cegar <k>",
  "enable cegar abstr-ref up to bound <k>", NULL,

  "-cegarPba <k>",
  "enable cegar+pba abstr-ref up to bound <k>", NULL,

  "-cegarStrategy <n>",
  "select cegar{+pba} strategy <n> (default: 1)", NULL,

  "-cois",
  "enable structural and functional cone-of-influence reduction", NULL,

  "-coisort",
  "sort latches and lambdas by increasing COI", NULL,

  "-dead",
  "check deadlock. verified spec is:",
  "AG (EX 1 | invarspec)", NULL,

  "-checkProof <val>",		/*  */
  "enable/disable final proof checking (default 0: disabled)", NULL,
  "-writeProof <fname>",
  "enable/disable final proof storing (default 0: disabled)", NULL,

  "-sort_for_bck <sel>",
  "select TR sorting for preimage, allowed values for <sel> are:",
  " 0: same sorting as for image",
  " 1 (default): sort by lowest (BDD ord) NS var - bottom vars first",
  " 2: sort by lowest (BDD ord) var - bottom vars first",
  " -1,-2: same as 1,2 with reverse order", NULL,

  "-bound <maxiter>",
  "bounded model check limit", NULL,

  "-initBound <k>",
  "initial bound in interpolant based verif", NULL,

  "-initMinterm <v>",
  "fill init state dont-cares with v (-1 to disable)", NULL,

  "-storeCNF <fname>",
  "store i-th R+ as cnf clauses to <fname>RPi.cnf",
  "-1: disabled (default)",
  "0: only TR (NO R+) stored",
  "1: TR (simplified using R+ as care set) and R+ stored", NULL,

  "-storeCNFTR <num>",
  "store also TR with i-th R+ as cnf clauses to <fname>RPi.cnf", NULL,

  "-storeCNFmode <mode>",
  "select mode ('M', 'N' or 'B' (default)) for cnf store ", NULL,

  "-maxCNFLength <num>",
  "sets the maximum length of CNF clauses (only with the 'B' mode) ", NULL,
  "-clk <name>",
  "name of clock variable", NULL,

  "-findClk",
  "try finding clock variable", NULL,

  "-en <name>",
  "name of enable variable", NULL,

  "-storeCNFphase <num>",
  "select when storing CNF clauses",
  "(0: after approx fwd (default), 1: after approx fwd/bwd", NULL,

  "-maxFwdIter <maxiter>",
  "max number of forward (approx) iterations", NULL,

  "-nExactIter <niter>",
  "number of initial exact images in forward approx trav", NULL,

  "-cut <th>",
  "enable cut-points with automatic aux var (pseudoinput) generation",
  "whwnever the BDD size of a node is larger than <th>", NULL,

  "-cutAtAuxVar <th>",
  "abstract (cut) at cut points", NULL,

  "-sift <th>",
  "set initial sift threshold (default 500000)", NULL,

  "-siftMaxTh <th>",
  "set max sift threshold (sift is disabled when > th, default: -1)", NULL,

  "-siftMaxCalls <n>",
  "set max num of sift calls in traversals (-1: disabled)", NULL,

  "-clust <th>",
  "set TR cluster threshold (default 1000)", NULL,

  "-clustThDiff <th>",
  "set COI difference cluster threshold (default 0)", NULL,

  "-clustThShare <th>",
  "set COI sharing cluster threshold (default 0 - disabled)", NULL,

  "-apprClust <th>",
  "set TR cluster threshold for approx trav (default 1000)", NULL,

  "-apprAig",
  "enable approx AIG trav", NULL,

  "-apprOverlap",
  "enable overlapping partitions in approx image", NULL,

  "-bwdClustFile <file>",
  "read TR clusters for bwd TR from <file>", NULL,

  "-imgClust <th>",
  "set image cluster threshold (default 5000)", NULL,

  "-imgClip <th>",
  "set image clip threshold (default 200000)", NULL,

  "-squaring",
  "emable iterative squaring", NULL,

  "-squaringMaxIter <d>",
  "set iterative squaring max depth (default -1)", NULL,

  "-imgDpart <th>",
  "set image disj. part threshold (default: 0 = no part)", NULL,

  "-trDpart <th>",
  "set tr disj. part threshold (default: 0 = no part)", NULL,

  "-appr <th>",
  "set group threshold for TR in approx img (default 1, i.e. disabled)",
  NULL,

  "-apprPrTh <th>",
  "set threshold for TR in approx preimg (default 1000)",
  NULL,

  "-approxNP <n>",
  "enable projection on co-domain space in approx image. Co-domain space",
  "is automatically split in <n> partitions set group following ord",
  "(default is disabled)", NULL,

  "-approxImgMin <th>",
  "lower threshold for approx image re-grouping",
  NULL,

  "-approxImgMax <th>",
  "upper threshold for approx image re-splitting",
  NULL,

  "-approxImgIter <n>",
  "maximum amount of re-grouping/re-splitting iterations in approx image",
  NULL,

  "-ilambda <i>",
  "select invarspec as i-th lambda (PO) function: AG (L[i])", NULL,

  "-ilambdaCare <i>",
  "select care as i-th lambda (PO) function", NULL,

  "-nlambda <name>",
  "select invarspec as lambda (PO by name) function: AG (L[name])", NULL,

  "-ninvar <name>",
  "select invar as lambda (PO by name) function: AG (L[name])", NULL,

  "-lambdaLatches <n>",
  "operate <n> lambda latches reductions", NULL,

  "-peripheralLatches",
  "operate (input) peripheral latch reduction", NULL,

  "-retime <s>",
  "operate (min-reg) retime (with strategy: 0 = no retime)", NULL,

  "-noPeripheralLatches",
  "disable (input) peripheral latch reduction", NULL,

  "-terminalScc",
  "operate terminal SCC reduction", NULL,

  "-twoPhase",
  "operate twoPhase reduction", NULL,

  "-twoPhaseForced",
  "force twoPhase reduction even with no clk", NULL,

  "-twoPhasePlus",
  "operate twoPhase reduction", NULL,

  "-twoPhaseOther",
  "operate twoPhase reduction on other phase (w.r.t. suggested)", NULL,

  "-impliedConstr",
  "operate implied constraint reduction", NULL,

  "-reduceCustom",
  "operate custom reduction", NULL,

  "-nnf <val>",
  "enable/disable nnf encoding of deltas (defaoult: 0)", NULL,

  "-printNtkStats",
  "just print number of inputs and latches", NULL,

  "-idelta <i>",
  "select invarspec as complement of i-th state var: AG (! PS[i])",
  NULL,

  "-invarspec <file>",
  "invariant specification read from <file>",
  "Required format: SPEC AG (<invariant>) or BDD (DDDMP) file", NULL,

  "-ctlspec <file>",
  "invariant specification read from <file>",
  "Supported CTL specs: SPEC AG (<invariant>)",
  "                     SPEC AG ([<req> ->] AF <ack>)",
  NULL,

  "-rp <file>",
  "read overapproximated reached from <file>",
  "used as care set with -coisort and in unbounded backward verif",
  NULL,

  "-wr <file>",
  "write overapproximated (forward) reached to <file>", NULL,

  "-wc <file>",
  "write (invar) constraint to <file>", NULL,

  "-wu <# unreached cubes>",
  "write unreached (forward) cubes", NULL,

  "-wp <file>",
  "write TR partitions to <file>", NULL,

  "-wbr <file>",
  "write backward reached to <file>", NULL,

  "-ord <file>",
  "read variable order from <file>", NULL,

  "-wo <file>",
  "write variable order to <file>", NULL,

  "-wres <file>",
  "write verification result to <file>", NULL,

  "-wfsm <file>",
  "write fsm to <file>", NULL,

  "-wfsmFolded <file>",
  "write fsm to <file> with folded property and constraint", NULL,

  NULL
};

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


#define printf Pdtutil_Printf

#define LOG_BDD(f,str) \
\
    {\
      Ddi_Varset_t *supp;\
\
      fprintf(stdout, "BDD-LOG: %s\n", str);      \
      if (f==NULL) \
        fprintf(stdout, "NULL!\n");\
      else if (Ddi_BddIsAig(f)) {\
        Ddi_Bdd_t *f1 = Ddi_BddMakeMono(f);\
        supp = Ddi_BddSupp(f1);\
        Ddi_VarsetPrint(supp, 0, NULL, stdout);\
        Ddi_BddPrintCubes(f1, supp, 100, 0, NULL, stdout);\
        Ddi_Free(supp);\
        Ddi_Free(f1);\
      }\
      else if (Ddi_BddIsMono(f)&&Ddi_BddIsZero(f)) \
        fprintf(stdout, "ZERO!\n");\
      else if (Ddi_BddIsMono(f)&&Ddi_BddIsOne(f)) \
        fprintf(stdout, "ONE!\n");\
      else {\
        /* print support variables and cubes*/\
        supp = Ddi_BddSupp(f);\
        Ddi_VarsetPrint(supp, 0, NULL, stdout);\
        Ddi_BddPrintCubes(f, supp, 100, 0, NULL, stdout);\
        Ddi_Free(supp);\
      }\
    }

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static char **readArgs(
  FILE * fp,
  int *argcP
);
static void freeArgs(
  int argc,
  char *args[]
);
static void printUsage(
  FILE * fp,
  char *prgrm
);
static void printHelp(
  FILE * fp,
  char *key
);
static int invarVerif(
  char *fsm,
  char *ord,
  Fbv_Globals_t * opt
);
static int invarMixedVerif(
  char *fsm,
  char *ord,
  Fbv_Globals_t * opt
);
static int invarDecompVerif(
  char *fsm,
  char *ord,
  Fbv_Globals_t * opt
);
static void WindowPart(
  Ddi_Bdd_t * f,
  Fbv_Globals_t * opt
);
static Ddi_Bdd_t *ExistPosEdge(
  Ddi_Bdd_t * f,
  Ddi_Var_t * v
);
static Ddi_Bdd_t *ExistFullPos(
  Ddi_Bdd_t * f,
  Ddi_Var_t * v,
  Ddi_Var_t * splitV,
  Fbv_Globals_t * opt
);
static int fbvDagInt(
  DdNode * n,
  Fbv_Globals_t * opt
);
static void fbvClearFlag(
  DdNode * f
);
static Ddi_Bdd_t *approxImgAux(
  Ddi_Bdd_t * trBdd,
  Ddi_Bdd_t * from,
  Ddi_Varset_t smoothV
);
static Ddi_Bdd_t *orProjectedSetAcc(
  Ddi_Bdd_t * f,
  Ddi_Bdd_t * g,
  Ddi_Var_t * partVar
);
static Ddi_Bdd_t *bddExistAppr(
  Ddi_Bdd_t * f,
  Ddi_Varset_t * sm
);
static Ddi_Varsetarray_t *computeCoiVars(
  Tr_Tr_t * tr,
  Ddi_Bdd_t * p,
  int maxIter
);
static Ddi_Varsetarray_t *computeFsmCoiVars(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * p,
  Ddi_Varset_t * extraVars,
  int maxIter,
  int verbosity
);
static Ddi_Varsetarray_t *computeFsmCoiRings(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * p,
  Ddi_Varset_t * extraVars,
  int maxIter,
  int verbosity
);
static void ManualAbstraction(
  Fsm_Mgr_t * fsm,
  char *file
);
static void ManualConstrain(
  Fsm_Mgr_t * fsm,
  char *file
);
static Ddi_Bddarray_t *
ManualHints(
  Ddi_Mgr_t *ddiMgr,
  char *file
);
static Tr_Tr_t *ManualTrDisjPartitions(
  Tr_Tr_t * tr,
  char *file,
  Ddi_Bdd_t * invar,
  Fbv_Globals_t * opt
);
static Ddi_Bdd_t *trExtractNochangeParts(
  Ddi_Bdd_t * trBdd,
  Ddi_Bdd_t * w,
  Ddi_Vararray_t * psv,
  Ddi_Vararray_t * nsv
);
static Tr_Tr_t *trCoiReduction(
  Tr_Tr_t * tr,
  Ddi_Varset_t * coiVars,
  int maxIter
);
static void fsmCoiReduction(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Varset_t * coiVars
);
static int fsmConstReduction(
  Fsm_Mgr_t * fsmMgr,
  int enTernary,
  int timeLimit,
  Fbv_Globals_t * opt,
  int latchOnly
);
static int fsmTsimReduction(
  Fsm_Mgr_t * fsmMgr,
  int enTernary,
  int timeLimit,
  Fbv_Globals_t * opt
);
static int fsmEqReduction(
  Fsm_Mgr_t * fsmMgr,
  int timeLimit,
  Fbv_Globals_t * opt,
  int depth,
  int latchOnly
);
static Ddi_Varset_t *FindClockInTr(
  Tr_Tr_t * tr
);
static node_info_item_t *hashSet(
  st_table * table,
  DdNode * f,
  Fbv_Globals_t * opt
);
static int nodeMerge(
  DdManager * manager,
  st_table * table,
  node_info_item_t * info0,
  node_info_item_t * info1,
  DdNode * auxV,
  int phase,
  DdNode ** resP
);
static void writeCoiShells(
  char *wrCoi,
  Ddi_Varsetarray_t * coi,
  Ddi_Vararray_t * vars
);
static Pdtutil_OptList_t *ddiOpt2OptList(
  Fbv_Globals_t * opt
);
static Pdtutil_OptList_t *fsmOpt2OptList(
  Fbv_Globals_t * opt
);
static Pdtutil_OptList_t *trOpt2OptList(
  Fbv_Globals_t * opt
);
static Pdtutil_OptList_t *travOpt2OptList(
  Fbv_Globals_t * opt
);
static Pdtutil_OptList_t *mcOpt2OptList(
  Fbv_Globals_t * opt
);
static Fbv_Globals_t *new_settings(
);
static void dispose_settings(
  Fbv_Globals_t * opt
);
static int checkEqGate(
  Ddi_Mgr_t * ddiMgr,
  Ddi_Varset_t * psv,
  bAigEdge_t baig,
  Ddi_Bddarray_t * delta,
  int chkDiff
);
static void saveDeltaCOIs(
  Fsm_Mgr_t * fsmMgr,
  int optCreateClusterFile,
  char *optClusterFile,
  int optMethodCluster,
  int thresholdCluster
);
static void savePropertiesCOIs(
  Fsm_Mgr_t * fsmMgr,
  int optCreateClusterFile,
  char *optClusterFile,
  int optMethodCluster,
  Ddi_Bddarray_t * pArray,
  int thresholdCluster
);
static Ddi_Bddarray_t *compactProperties(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Mgr_t * ddiMgr,
  Ddi_Varsetarray_t * coiArray,
  int optCreateClusterFile,
  char *optClusterFile,
  int optMethodCluster,
  Ddi_Bddarray_t * pArray,
  int thresholdCluster
);
static int checkFsmCex(
  Fsm_Mgr_t * fsmMgr,
  Trav_Mgr_t * travMgr
);
static void
InsertCutLatches(
  Fsm_Mgr_t * fsm,
  int th,
  int strategy
);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

int
FbvTwoPhaseReduction(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt,
  int do_find,
  int do_red
)
{
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Bdd_t *invar = Fsm_MgrReadConstraintBDD(fsmMgr);
  int teSteps = Fsm_MgrReadTargetEnSteps(fsmMgr);
  Ddi_Var_t *specV = Fsm_MgrReadPdtSpecVar(fsmMgr);
  Ddi_Var_t *constrV = Fsm_MgrReadPdtConstrVar(fsmMgr);
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);

  if (0 &&fsmMgr->stats.retimedConstr) {
    Pdtutil_Warning(1, 
      "phase abstraction disabled with retimed constraint\n");
    return 1;
  }

  if (teSteps>1) {
    Pdtutil_Warning(1, 
      "Multiple Target EN steps not supported for two phase reduction.");
    return 1;
  }

  if (do_find && opt->pre.findClk) {
    int i, nstate = Ddi_VararrayNum(ps);

    for (i = 0; i < nstate; i++) {
      Ddi_Bdd_t *dClk0, *dClk1;
      Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);

      dClk0 = Ddi_BddCofactor(d_i, v_i, 0);
      Ddi_BddNotAcc(dClk0);
      dClk1 = Ddi_BddCofactor(d_i, v_i, 1);
      if (!Ddi_AigSat(dClk1) && !Ddi_AigSat(dClk0)) { // GpC: try speedup
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Clock found; Name: %s\n", Ddi_VarName(v_i)));
        opt->pre.clkVarFound = v_i;
      } else {
        Ddi_Varset_t *s_i = Ddi_BddSupp(d_i);

        if (Ddi_VarsetNum(s_i) == 1) {
          int j;
          Ddi_Var_t *v_in = Ddi_VarsetTop(s_i);

          for (j = i + 1; j < nstate; j++) {
            if (Ddi_VararrayRead(ps, j) == v_in) {
              break;
            }
          }
          if (j < nstate) {
            Ddi_Bdd_t *d_j = Ddi_BddarrayRead(delta, j);
            Ddi_Varset_t *s_j = Ddi_BddSupp(d_j);

            if (Ddi_VarsetNum(s_j) == 1 && Ddi_VarInVarset(s_j, v_i)) {
              if (Ddi_BddIsComplement(d_i) && Ddi_BddIsComplement(d_j)) {
                Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
                Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
                int eqLatches = 1;
                int diffLatches = 1;

                Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
                  Pdtutil_VerbLevelNone_c,
                  fprintf(stdout, "Clock pair found; Names: %s - %s.\n",
                    Ddi_VarName(v_i), Ddi_VarName(v_in)));
                opt->pre.clkVarFound = v_i;
#if 1
                /* check init state !!! */
                if (initStub != NULL) {
                  Ddi_Bdd_t *st_i = Ddi_BddarrayRead(initStub, i);
                  Ddi_Bdd_t *st_j = Ddi_BddarrayRead(initStub, j);
                  Ddi_Bdd_t *st_jn = Ddi_BddNot(st_j);

                  eqLatches = Ddi_BddEqualSat(st_i, st_j);
                  diffLatches = Ddi_BddEqualSat(st_i, st_jn);
                  Ddi_Free(st_jn);
                } else {
                  Ddi_Bdd_t *dif = Ddi_BddMakeLiteralAig(v_i, 1);
                  Ddi_Bdd_t *aux = Ddi_BddMakeLiteralAig(v_in, 1);

                  Ddi_BddDiffAcc(dif, aux);
                  Ddi_Free(aux);
                  eqLatches = Ddi_AigSatAnd(init, dif, NULL);
                  Ddi_BddNotAcc(dif);
                  diffLatches = Ddi_AigSatAnd(init, dif, NULL);
                  Ddi_Free(dif);
                }
                if (eqLatches && !diffLatches) {
                  /* OK clock */
                  Ddi_DataCopy(d_i, d_j);
                } else {
                  /* undo clk */
                  opt->pre.clkVarFound = NULL;
                }
#endif
              }
            }
            Ddi_Free(s_j);
          }
        }
        Ddi_Free(s_i);
      }
      Ddi_Free(dClk0);
      Ddi_Free(dClk1);
    }
  }

  if (do_red) {
    if (1 && opt->pre.clkVarFound != NULL) {
      int i;
      int n0 = 0, n1 = 0, nd = Ddi_BddarrayNum(delta);

      for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
        Ddi_Bdd_t *d_i_0, *d_i_1, *d_i = Ddi_BddarrayRead(delta, i);

        d_i_0 = Ddi_BddCofactor(d_i, opt->pre.clkVarFound, 0);
        d_i_1 = Ddi_BddCofactor(d_i, opt->pre.clkVarFound, 1);
        if (Ddi_BddSize(d_i_0) == 1) {
          Ddi_Var_t *v = Ddi_BddTopVar(d_i_0);

          if (v != opt->pre.clkVarFound && v == Ddi_VararrayRead(ps, i)) {
            n0++;
          }
        }
        if (Ddi_BddSize(d_i_1) == 1) {
          Ddi_Var_t *v = Ddi_BddTopVar(d_i_1);

          if (v != opt->pre.clkVarFound && v == Ddi_VararrayRead(ps, i)) {
            n1++;
          }
        }
        Ddi_Free(d_i_0);
        Ddi_Free(d_i_1);
      }
      Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "clocked regs: n1=%d, n0=%d\n", n0, n1));
      if (n1 > nd / 10 || n0 > nd / 10) {
        if (n1 > n0 * 5) {
          opt->pre.clkPhaseSuggested = 1;
        } else if (n0 > n1 * 5) {
          opt->pre.clkPhaseSuggested = 0;
        }
      }
    }

    if (1 && ((opt->pre.twoPhaseRed && opt->pre.clkVarFound != NULL) ||
        opt->pre.twoPhaseForced)) {
      int nl = Ddi_VararrayNum(ps);
      int ni = Ddi_VararrayNum(pi);
      int ph = -1;
      int genAigVars = 0 && (Ddi_MgrReadNumCuddVars(fsmMgr->dd) > 16000 &&
        (nl > 3000 || ni > 3000));
      int size0 = Ddi_BddarraySize(delta);
      Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
      Ddi_Bdd_t *phase, *init = Fsm_MgrReadInitBDD(fsmMgr);
      Ddi_Vararray_t *twoPhasePis = genAigVars ?
        Ddi_VararrayMakeNewAigVars(pi, "PDT_ITP_TWOPHASE_PI", "0")
        : Ddi_VararrayMakeNewVars(pi, "PDT_ITP_TWOPHASE_PI", "0", 1);
      Ddi_Bddarray_t *twoPhasePiLits =
        Ddi_BddarrayMakeLiteralsAig(twoPhasePis, 1);
      Ddi_Bddarray_t *twoPhaseDelta = Ddi_BddarrayDup(delta);
      Ddi_Bddarray_t *twoPhaseDelta2 = Ddi_BddarrayDup(delta);

      if (1 && invar!=NULL && !Ddi_BddIsOne(invar)) {
	int iConstr = Ddi_BddarrayNum(delta)-2;
	int iProp = Ddi_BddarrayNum(delta)-1;
	if (Ddi_VararrayRead(ps,iConstr)==constrV &&
	    Ddi_VararrayRead(ps,iProp)==specV) {
	  Ddi_Bdd_t *dProp = Ddi_BddarrayRead(twoPhaseDelta,iProp);
	  Ddi_Bdd_t *dConstr = Ddi_BddarrayRead(twoPhaseDelta,iConstr);
	  Ddi_Bdd_t *dProp2 = Ddi_BddarrayRead(twoPhaseDelta2,iProp);
	  Ddi_Bdd_t *dConstr2 = Ddi_BddarrayRead(twoPhaseDelta2,iConstr);
	  Ddi_Bdd_t *specLit, *constrLit, *prevTarget;
	  Ddi_Bdd_t *prevOutOfConstr = Ddi_BddNot(invar);
	  specLit = Ddi_BddMakeLiteralAig(specV, 1);
	  constrLit = Ddi_BddMakeLiteralAig(constrV, 1);
	  prevTarget = Ddi_BddDiff(constrLit,specLit);
	  Ddi_BddAndAcc(dConstr,invar);
	  Ddi_BddOrAcc(dConstr,prevTarget);
	  Ddi_BddDiffAcc(dProp,prevTarget);
	  Ddi_BddOrAcc(dProp2,prevOutOfConstr);
	  Ddi_Free(prevOutOfConstr);
	  Ddi_Free(specLit);
	  Ddi_Free(constrLit);
	  Ddi_Free(prevTarget);
	}
      }
      
      if (teSteps>0) {
	// target en pis for 0 phase and standard pis op 1 frame cincide 
	Ddi_Vararray_t *tePis = genAigVars ?
	  Ddi_VararrayMakeNewAigVars(pi, "PDT_TARGET_EN_PI", "0")
	  : Ddi_VararrayMakeNewVars(pi, "PDT_TARGET_EN_PI", "0", 1);
	Ddi_BddarraySubstVarsAcc(twoPhaseDelta2, tePis, twoPhasePis);
	Ddi_Free(tePis);
      }

      Ddi_AigarrayComposeAcc(twoPhaseDelta, pi, twoPhasePiLits);
      Ddi_AigarrayComposeNoMultipleAcc(twoPhaseDelta, ps, twoPhaseDelta2);

      if (invar != NULL && !Ddi_BddIsOne(invar)) {
	Ddi_Bdd_t *prevSpec, *prevConstr;
	Ddi_Bdd_t *dupInvar = Ddi_BddDup(invar);
	Pdtutil_Assert(specV!=NULL && constrV!=NULL,
		       "missing pdt spec/constr vars");
	prevSpec = Ddi_BddMakeLiteralAig(specV, 1);
	prevConstr = Ddi_BddMakeLiteralAig(constrV, 1);
	Ddi_BddNotAcc(prevSpec);
	Ddi_BddAndAcc(invar,prevConstr);
	Ddi_BddOrAcc(invar,prevSpec);
	Ddi_Free(prevSpec);
	Ddi_Free(prevConstr);
	Ddi_BddComposeAcc(invar, pi, twoPhasePiLits);
	Ddi_BddComposeAcc(invar, ps, twoPhaseDelta2);
	Ddi_BddAndAcc(invar,dupInvar);
	Ddi_Free(dupInvar);
      }

      if (opt->pre.clkVarFound != NULL && opt->pre.twoPhaseRed) {
        phase = Ddi_BddMakeLiteralAig(opt->pre.clkVarFound, 1);
        if (initStub != NULL) {
          Ddi_BddComposeAcc(phase, ps, initStub);
        } else {
          Ddi_BddAndAcc(phase, init);
        }

        if (!Ddi_AigSat(phase)) {
          ph = 0;
        } else if (!Ddi_AigSat(Ddi_BddNotAcc(phase))) {
          ph = 1;
        }
        if (ph >= 0 && opt->pre.clkPhaseSuggested >= 0
          && ph != opt->pre.clkPhaseSuggested) {
          ph = opt->pre.clkPhaseSuggested;
          opt->pre.twoPhaseRedPlus = 1;
        }
        Ddi_Free(phase);
      }

      if (1 && opt->pre.twoPhaseRedPlus && initStub != NULL) {
        Ddi_Bddarray_t *initStubPlus = Ddi_BddarrayDup(delta);
        char suffix[4];
        Ddi_Vararray_t *newPiVars;
        Ddi_Bddarray_t *newPiLits;
	Ddi_Bdd_t *invar = Fsm_MgrReadConstraintBDD(fsmMgr);

	if (invar!=NULL && !Ddi_BddIsOne(invar)) {
	  int iConstr = Ddi_BddarrayNum(delta)-2;
	  int iProp = Ddi_BddarrayNum(delta)-1;
	  Ddi_Var_t *v1 = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
	  if (Ddi_VararrayRead(ps,iConstr)==v1) {
	    Ddi_Bdd_t *dConstr = Ddi_BddarrayRead(initStubPlus,iConstr);
	    Ddi_Bdd_t *dProp = Ddi_BddarrayRead(initStubPlus,iProp);
	    Ddi_Bdd_t *outOfConstr = Ddi_BddNot(dConstr);
	    //	    Ddi_BddAndAcc(dConstr,invar);
	    Ddi_BddOrAcc(dProp,outOfConstr);
	    Ddi_Free(outOfConstr);
	  }
	}
        sprintf(suffix, "%d", fsmMgr->stats.initStubSteps++);
        newPiVars = genAigVars ?
          Ddi_VararrayMakeNewAigVars(pi, "PDT_STUB_PI", suffix)
          : Ddi_VararrayMakeNewVars(pi, "PDT_STUB_PI", suffix, 1);
        newPiLits = Ddi_BddarrayMakeLiteralsAig(newPiVars, 1);
        Ddi_BddarrayComposeAcc(initStubPlus, pi, newPiLits);
        Ddi_BddarrayComposeAcc(initStubPlus, ps, initStub);
        Ddi_Free(newPiVars);
        Ddi_Free(newPiLits);
        Fsm_MgrSetInitStubBDD(fsmMgr, initStubPlus);
        initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
        Ddi_Free(initStubPlus);
      }

      Ddi_DataCopy(delta, twoPhaseDelta);
      opt->pre.twoPhaseDone = 1;
      Fsm_MgrSetPhaseAbstr(fsmMgr, 1);
      Pdtutil_WresSetTwoPhase();

      Ddi_VararrayAppend(pi, twoPhasePis);

      if (opt->pre.clkVarFound != NULL) {
        if (1 && (ph >= 0)) {
          int i;

          for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
            Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);

            Ddi_BddCofactorAcc(d_i, opt->pre.clkVarFound, ph);
          }
	  if (invar!=NULL && !Ddi_BddIsOne(invar)) {
	    Ddi_BddCofactorAcc(invar, opt->pre.clkVarFound, ph);
	  }
        }
      }

      int chkStub=0;
      if (chkStub) {
	Ddi_Var_t *vSpec = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
	Ddi_Var_t *vConstr = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
	Ddi_Bdd_t *target = Ddi_BddMakeLiteralAig(vSpec, 0);
	Ddi_Bdd_t *constr = Ddi_BddMakeLiteralAig(vConstr, 1);
	Ddi_Bdd_t *target0 = Ddi_BddMakeLiteralAig(vSpec, 0);
	Ddi_Bdd_t *constr0 = Ddi_BddMakeLiteralAig(vConstr, 1);
	Ddi_BddComposeAcc(target,ps,delta);
	Ddi_BddComposeAcc(target,ps,initStub);
	Ddi_BddComposeAcc(target0,ps,initStub);
	Ddi_BddComposeAcc(constr,ps,delta);
	Ddi_BddComposeAcc(constr,ps,initStub);
	Ddi_BddComposeAcc(constr0,ps,initStub);
	int sat = Ddi_AigSatAnd(target,constr,0);
	int sat1 = Ddi_AigSatAnd(target0,constr0,0);
	Ddi_Free(target);
	Ddi_Free(constr);
	Ddi_Free(target0);
	Ddi_Free(constr0);
      }

      Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "Two phase reduction: %d -> %d (ph: %d)\n",
          size0, Ddi_BddarraySize(delta), ph));

      Ddi_Free(twoPhasePis);
      Ddi_Free(twoPhasePiLits);
      Ddi_Free(twoPhaseDelta);
      Ddi_Free(twoPhaseDelta2);
    }
  }

  return 0;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
FbvApproxPreimg(
  Tr_Tr_t * TR,
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * window,
  int doApprox,
  Fbv_Globals_t * opt
)
{
  Ddi_Varset_t *ps, *ns, *smoothV, *pi, *abstract, *mySmooth;
  Tr_Mgr_t *trMgr = Tr_TrMgr(TR);
  Ddi_Bdd_t *trBdd, *to, *toPlus, *myFrom, *myCare, *myConflict = NULL;
  Ddi_Vararray_t *psv;
  Ddi_Vararray_t *nsv;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(from);
  char msg[NMAX];
  Ddi_Bdd_t *clk_part = NULL;

  psv = Tr_MgrReadPS(trMgr);
  nsv = Tr_MgrReadNS(trMgr);

  trBdd = Tr_TrBdd(TR);

  if (Ddi_BddIsPartDisj(trBdd)) {
    int i, np = Ddi_BddPartNum(trBdd);
    Ddi_Bdd_t *to_i;
    Ddi_Bdd_t *toDisj = Ddi_BddMakePartDisjVoid(ddiMgr);

    if (ddiMgr->exist.conflictProduct != NULL) {
      myConflict = Ddi_BddMakePartDisjVoid(ddiMgr);
    }

    for (i = 0; i < np; i++) {
      Tr_Tr_t *trP = Tr_TrDup(TR);

      TrTrRelWrite(trP, Ddi_BddPartRead(trBdd, i));
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "Dpart PreImg. %d/%d\n", i, np));

      to_i = FbvApproxPreimg(trP, from, care, window, doApprox, opt);
      if (!Ddi_BddIsZero(to_i)) {
        Ddi_BddPartInsertLast(toDisj, to_i);
      }
      Ddi_Free(to_i);
      Tr_TrFree(trP);
      if (opt->common.clk && (ddiMgr->exist.conflictProduct != NULL)) {
        Ddi_Bdd_t *clk_lit;

        Pdtutil_Assert(i <= 1, "wrong clocked partition");
        clk_lit =
          Ddi_BddMakeLiteral(Ddi_VarFromName(ddiMgr, opt->common.clk), 1 - i);
        if (Ddi_BddIsOne(ddiMgr->exist.conflictProduct)) {
          Ddi_BddNotAcc(ddiMgr->exist.conflictProduct);
        }
        Ddi_BddSetPartConj(ddiMgr->exist.conflictProduct);
        Ddi_BddPartInsert(ddiMgr->exist.conflictProduct, 0, clk_lit);
        Ddi_Free(clk_lit);
        Ddi_BddPartInsertLast(myConflict, ddiMgr->exist.conflictProduct);
        Ddi_Free(ddiMgr->exist.conflictProduct);
        ddiMgr->exist.conflictProduct = Ddi_BddMakeConst(ddiMgr, 1);
      }
    }
    if (ddiMgr->exist.conflictProduct != NULL) {
      Ddi_Free(ddiMgr->exist.conflictProduct);
      if (Ddi_BddPartNum(myConflict) == 2) {
        ddiMgr->exist.conflictProduct = myConflict;
      } else {
        ddiMgr->exist.conflictProduct = Ddi_BddMakeConst(ddiMgr, 1);
      }
    }
    if (Ddi_BddPartNum(toDisj) == 1) {
      to_i = Ddi_BddPartExtract(toDisj, 0);
      Ddi_Free(toDisj);
      toDisj = to_i;
    } else if (Ddi_BddPartNum(toDisj) == 0) {
      Ddi_Free(toDisj);
      toDisj = Ddi_BddMakeConst(ddiMgr, 0);
    }

    if (!Ddi_BddIsZero(toDisj)) {
      Ddi_Free(ddiMgr->exist.conflictProduct);
      ddiMgr->exist.conflictProduct = NULL;
    }

    return (toDisj);
  }

  if (1 && opt->trav.imgDpartTh > 0) {
    Ddi_MgrSetExistDisjPartTh(ddiMgr, opt->trav.imgDpartTh /* *5 */ );
  }

  if (opt->trav.imgDpartTh > 0 && opt->trav.imgDpartTh < Ddi_BddSize(from)) {
    int i;
    Ddi_Bdd_t *to_i;
    Ddi_Bdd_t *toDisj = Ddi_BddMakePartDisjVoid(ddiMgr);
    int saveImgDpartTh = opt->trav.imgDpartTh;

    myFrom = Part_PartitionDisjSet(from, NULL, NULL,
      Part_MethodEstimate_c, opt->trav.imgDpartTh, 3,
      Pdtutil_VerbLevelDevMed_c);

    Ddi_BddSetFlattened(myFrom);
    Ddi_BddSetPartDisj(myFrom);

    if (ddiMgr->exist.conflictProduct != NULL) {
      myConflict = Ddi_BddMakePartDisjVoid(ddiMgr);
    }
    opt->trav.imgDpartTh = -1;

    for (i = 0; i < Ddi_BddPartNum(myFrom); i++) {
      to_i = FbvApproxPreimg(TR, Ddi_BddPartRead(myFrom, i),
        care, window, doApprox, opt);
      if (!Ddi_BddIsZero(to_i)) {
        Ddi_BddPartInsertLast(toDisj, to_i);
      }
      Ddi_Free(to_i);
      if (opt->common.clk && (ddiMgr->exist.conflictProduct != NULL)) {
        Ddi_Bdd_t *clk_lit;

        Pdtutil_Assert(i <= 1, "wrong clocked partition");
        clk_lit =
          Ddi_BddMakeLiteral(Ddi_VarFromName(ddiMgr, opt->common.clk), 1 - i);
        if (Ddi_BddIsOne(ddiMgr->exist.conflictProduct)) {
          Ddi_BddNotAcc(ddiMgr->exist.conflictProduct);
        }
        Ddi_BddSetPartConj(ddiMgr->exist.conflictProduct);
        Ddi_BddPartInsert(ddiMgr->exist.conflictProduct, 0, clk_lit);
        Ddi_Free(clk_lit);
        Ddi_BddPartInsertLast(myConflict, ddiMgr->exist.conflictProduct);
        Ddi_Free(ddiMgr->exist.conflictProduct);
        ddiMgr->exist.conflictProduct = Ddi_BddMakeConst(ddiMgr, 1);
      }
    }
    opt->trav.imgDpartTh = saveImgDpartTh;

    if (ddiMgr->exist.conflictProduct != NULL) {
      Ddi_Free(ddiMgr->exist.conflictProduct);
      if (Ddi_BddPartNum(myConflict) == 2) {
        ddiMgr->exist.conflictProduct = myConflict;
      } else {
        ddiMgr->exist.conflictProduct = Ddi_BddMakeConst(ddiMgr, 1);
      }
    }
    if (Ddi_BddPartNum(toDisj) == 1) {
      to_i = Ddi_BddPartExtract(toDisj, 0);
      Ddi_Free(toDisj);
      toDisj = to_i;
    } else if (Ddi_BddPartNum(toDisj) == 0) {
      Ddi_Free(toDisj);
      toDisj = Ddi_BddMakeConst(ddiMgr, 0);
    }

    if (!Ddi_BddIsZero(toDisj)) {
      Ddi_Free(ddiMgr->exist.conflictProduct);
      ddiMgr->exist.conflictProduct = NULL;
    }
    Ddi_Free(myFrom);
    return (toDisj);
  }


  ps = Ddi_VarsetMakeFromArray(psv);
  ns = Ddi_VarsetMakeFromArray(nsv);
  pi = Ddi_VarsetMakeFromArray(Tr_MgrReadI(trMgr));

  toPlus = NULL;
  myCare = Ddi_BddDup(care);

  abstract = NULL;
  if (doApprox) {
    strcpy(msg, "APPROX");
    /* myFrom = Tr_TrProjectOverPartitions(from,TR,NULL,0,opt->trav.approxPreimgTh); */
    myFrom = Ddi_BddDup(from);
#if 0
    abstract = Ddi_BddSupp(myFrom);
    Ddi_VarsetRemoveAcc(abstract, Tr_TrClk(TR));
#endif
  } else {
    strcpy(msg, "Exact");
    myFrom = Ddi_BddDup(from);
    if ((opt->trav.opt_preimg > 0)
      && (Ddi_BddSize(from) > opt->trav.opt_preimg)) {
      toPlus = FbvApproxPreimg(TR, from, care, NULL, 1, opt);
      if (myCare != NULL) {
        Ddi_BddSetPartConj(myCare);
        Ddi_BddPartInsertLast(myCare, toPlus);
      }
    }
  }
#if 0
  if (Ddi_BddSize(myFrom) > 2000) {
    myFrom = Ddi_BddEvalFree(Part_PartitionDisjSet(myFrom, NULL, NULL,
        Part_MethodEstimateFast_c, 2000, Pdtutil_VerbLevelDevMed_c), myFrom);
  }
#endif

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMax_c,
    fprintf(stdout, "%s PreImg.\n", msg));

  Ddi_BddSwapVarsAcc(myFrom, psv, nsv);

  {
    int thresh;

    smoothV = Ddi_VarsetUnion(ns, pi);

    if (doApprox) {
      /*thresh = Ddi_MgrReadSiftThresh(ddiMgr); */
      /*thresh = 10*opt->trav.approxPreimgTh; */
      thresh = -1;
#if 0
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
        fprintf(stdout, "Abstracting %d vars\n", Ddi_VarsetNum(abstract)));
      Ddi_VarsetUnionAcc(smoothV, abstract);
      Ddi_Free(abstract);
#endif
    } else {
      thresh = -1;
    }

    if (1 && myCare != NULL /*&& !Ddi_BddIsOne(myCare) */ ) {
      Ddi_Bdd_t *overAppr = NULL;
      Ddi_Bdd_t *prod = Ddi_BddDup(trBdd);
      Ddi_Bdd_t *prod1 = Ddi_BddMakePartConjVoid(ddiMgr);
      Ddi_Varset_t *suppFrom = Ddi_BddSupp(myFrom);
      int i;

      if (window != NULL) {
        Ddi_BddSuppAttach(window);
        Ddi_VarsetUnionAcc(suppFrom, Ddi_BddSuppRead(window));
        Ddi_BddSuppDetach(window);
      }
      Ddi_BddSetPartConj(prod);

      if (opt->common.clk != 0) {
        clk_part = Ddi_BddPartExtract(prod, 0);
      }

      for (i = 0; i < Ddi_BddPartNum(prod); i++) {
        Ddi_Bdd_t *p = Ddi_BddPartRead(prod, i);
        Ddi_Varset_t *suppP = Ddi_BddSupp(p);
        Ddi_Varset_t *supp = Ddi_BddSupp(p);
        Ddi_Varset_t *suppProd;

        Ddi_VarsetIntersectAcc(supp, suppFrom);
        if (!opt->fsm.auxVarsUsed && Ddi_VarsetIsVoid(supp)) {
          mySmooth = Ddi_VarsetIntersect(smoothV, suppP);
          suppProd = Ddi_BddSupp(prod);
          Ddi_VarsetDiffAcc(mySmooth, suppProd);
          p = Ddi_BddPartExtract(prod, i);
          if (!Ddi_VarsetIsVoid(mySmooth)) {
            Ddi_Free(suppProd);
            suppProd = Ddi_BddSupp(prod1);
            Ddi_VarsetDiffAcc(mySmooth, suppProd);
            if (!Ddi_VarsetIsVoid(mySmooth)) {
              Ddi_BddExistAcc(p, mySmooth);
            }
          }
          if (!Ddi_BddIsOne(p)) {
            // Ddi_BddPartInsertLast(prod1,p);
          }
          Ddi_Free(p);
          i--;
          Ddi_Free(suppProd);
          Ddi_Free(mySmooth);
        }
        Ddi_Free(supp);
        Ddi_Free(suppP);
      }
      Ddi_Free(suppFrom);
      if (0 && opt->trav.fbvMeta) {
        Ddi_Bdd_t *fmeta = Ddi_BddDup(myFrom);

#if 0
        Ddi_MetaInit(ddiMgr, Ddi_Meta_Size,
          trBdd, 0, psv, nsv, opt->trav.fbvMeta);
        opt->trav.fbvMeta = 1;
#endif

        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
          fprintf(stdout, "\nPart=%d,", Ddi_BddSize(myFrom)));
        Ddi_BddSetMono(myFrom);
        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
          fprintf(stdout, "Mono=%d,", Ddi_BddSize(myFrom)));
        Ddi_BddSetMeta(myFrom);
        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
          fprintf(stdout, "Meta=%d\n", Ddi_BddSize(myFrom)));
        Ddi_Free(fmeta);
      }

      Ddi_BddRestrictAcc(prod, myCare);
      Ddi_BddRestrictAcc(prod1, myCare);
      Ddi_BddRestrictAcc(myFrom, myCare);

      if (opt->trav.trPreimgSort) {
        int verbosityLocal = Tr_MgrReadVerbosity(trMgr);
        double saveW5 = Tr_MgrReadSortW(trMgr, 5);

        Ddi_BddSetFlattened(prod);
        Ddi_BddSetPartConj(prod);
        Tr_MgrSetSortW(trMgr, 5, 100.0);

        Tr_MgrSetVerbosity(trMgr, Pdtutil_VerbLevelDevMin_c);
        Tr_TrBddSortIwls95(trMgr, prod, smoothV, ps);
        Tr_MgrSetVerbosity(trMgr, (Pdtutil_VerbLevel_e) verbosityLocal);
        Tr_MgrSetSortW(trMgr, 5, saveW5);
      }

      if (1 && opt->trav.fbvMeta) {
        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
          fprintf(stdout, "\nPart=%d,", Ddi_BddSize(myFrom)));
        Ddi_BddSetMeta(myFrom);
        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
          fprintf(stdout, "\nMeta=%d,", Ddi_BddSize(myFrom)));
      }

      Ddi_BddPartInsert(prod, 0, myFrom);

      if (1 && doApprox) {
#if 0
        myCare = Ddi_MgrReadOne(ddiMgr);
#endif
        overAppr = myFrom;
      }

      if (0 /*doApprox */ ) {
        Ddi_Varset_t *smoothPlus = Ddi_VarsetVoid(ddiMgr);
        Ddi_Bdd_t *lit;
        int j;

        for (i = j = 0, Ddi_VarsetWalkStart(ps);
          !Ddi_VarsetWalkEnd(ps); Ddi_VarsetWalkStep(ps), i++) {
          if (i == 166 /*Ddi_VarsetNum(ps)*10/11 */ ) {
            Ddi_VarsetAddAcc(smoothPlus, Ddi_VarsetWalkCurr(ps));
            if (j == 0) {
              lit = Ddi_BddMakeLiteral(Ddi_VarsetWalkCurr(ps), 0);
              Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
                fprintf(stdout, "\nS+[%d]:%s\n", i,
                  Ddi_VarName(Ddi_VarsetWalkCurr(ps))));
            }
            j++;
          }
        }
        for (i = 0; i < Ddi_BddPartNum(prod); i++) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(prod, i);

          Ddi_BddConstrainAcc(p, lit);
        }
        Ddi_Free(smoothPlus);
        Ddi_Free(lit);
      }


      ddiMgr->exist.clipLevel = opt->ddi.saveClipLevel;

      if (1 || ddiMgr->exist.clipLevel < 2) {
        if (window != NULL) {
          ddiMgr->exist.activeWindow = Ddi_BddDup(window);
          if (!Ddi_BddIsOne(window)) {
            Ddi_BddRestrictAcc(prod, window);
            Ddi_BddPartInsert(prod, 0, window);
          }
        } else {
          ddiMgr->exist.activeWindow = NULL;
        }
        if (clk_part != NULL) {
          Ddi_BddPartInsert(prod, 0, clk_part);
          Ddi_Free(clk_part);
        }
        if (Tr_MgrReadCheckFrozenVars(trMgr)) {
          to = Ddi_BddMultiwayRecursiveAndExistOverWithFrozenCheck(prod,
            smoothV, myFrom, psv, nsv, myCare, overAppr, -1, 0);
        } else {
#if 0
          to = Ddi_BddMultiwayRecursiveAndExistOver(prod, ns,
            myCare, overAppr, thresh);
          Ddi_BddExistAcc(to, smoothV);
#else
          to = Ddi_BddMultiwayRecursiveAndExistOver(prod, smoothV,
            myCare, overAppr, thresh);
#endif
        }
        Ddi_Free(ddiMgr->exist.activeWindow);
      } else {
        int again = 0;
        int myClipTh = ddiMgr->exist.clipThresh;

        if (clk_part != NULL) {
          Ddi_BddPartInsert(prod, 0, clk_part);
          Ddi_Free(clk_part);
        }
        do {
          to = Ddi_BddMultiwayRecursiveAndExistOver(prod, smoothV,
            myCare, overAppr, thresh);
          if (Ddi_BddIsZero(to) && ddiMgr->exist.clipDone) {
            ddiMgr->exist.clipDone = 0;
            ddiMgr->exist.clipThresh *= 1.5;
            Ddi_Free(ddiMgr->exist.clipWindow);
            again = 1;
            Ddi_Free(to);
          }
        } while (again);
        ddiMgr->exist.clipThresh = myClipTh;
      }

      if (1 && opt->trav.fbvMeta) {
        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
          fprintf(stdout, "\nMeta=%d,", Ddi_BddSize(to)));
        Ddi_BddSetPartConj(to);
        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
          fprintf(stdout, "Part=%d\n", Ddi_BddSize(to)));
      }

      Ddi_Free(prod);
      if (Ddi_BddIsPartConj(to)) {
        int i;

        for (i = 0; i < Ddi_BddPartNum(to); i++) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(to, i);

          Ddi_BddExistAcc(p, smoothV);
        }
      }
      Ddi_BddSetPartConj(to);
      for (i = 0; i < Ddi_BddPartNum(prod1); i++) {
        Ddi_Bdd_t *p = Ddi_BddPartRead(prod1, i);

        Ddi_BddPartInsertLast(to, p);
      }
      Ddi_Free(prod1);

      ddiMgr->exist.clipLevel = 0;
      Ddi_Free(clk_part);
    } else if (1) {
      Ddi_Bdd_t *prod = Ddi_BddDup(trBdd);

      Ddi_BddPartInsert(prod, 0, myFrom);
      to = Ddi_BddMultiwayRecursiveAndExistOver(prod, smoothV,
        myCare, NULL, -1);
      Ddi_Free(prod);
    } else {
      to = Ddi_BddAndExist(trBdd, myFrom, smoothV);
    }
    Ddi_Free(smoothV);
    Ddi_BddSetClustered(to, opt->trav.imgClustTh);
    Ddi_BddSetFlattened(to);
    Pdtutil_Assert(!(Ddi_BddIsPartConj(to) && (Ddi_BddPartNum(to) == 1)),
      "Invalid one partition part conj");
    Ddi_BddRestrictAcc(to, myCare);

    if (0 && doApprox) {
      to =
        Ddi_BddEvalFree(Tr_TrProjectOverPartitions(to, TR, myCare, 0,
          opt->trav.approxPreimgTh), to);
    }
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "\n%d\n", Ddi_BddSize(to)));
  }

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "%s PreImg Done.\n", msg));

  Ddi_Free(myFrom);
  Ddi_Free(ps);
  Ddi_Free(ns);
  Ddi_Free(pi);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelDevMin_c,
    LOG_CONST(to, "APPR-to")
    );

#if 0
  if ((doApprox > 1) && (Ddi_BddSize(to) < 100)) {
    Ddi_BddSetMono(to);
    if (Ddi_BddIsOne(to)) {
      Ddi_Free(to);
      to = FbvApproxPreimg(TR, from, myCare, NULL, 0, opt);
    }
  }
#endif

  Ddi_Free(myCare);

  if (toPlus != NULL) {
    Ddi_BddSetPartConj(to);
    Ddi_BddPartInsertLast(to, toPlus);
    Ddi_Free(toPlus);
  }

  if (opt->trav.univQuantifyTh > 0
    && Ddi_BddSize(to) > opt->trav.univQuantifyTh) {
    int i;

#if 0
    Ddi_Varset_t *supp = Ddi_BddSupp(to);
#else
    Ddi_Varset_t *supp = Ddi_VarsetDup(opt->trav.univQuantifyVars);
#endif
    Ddi_Vararray_t *q = Ddi_VararrayMakeFromVarset(supp, 1);
    int th = Ddi_BddSize(to) / 4;

    Ddi_Free(supp);
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "Forall %d=", Ddi_BddSize(to)));
    for (i = Ddi_VararrayNum(q) - 1; i >= 0; i--) {
      Ddi_Bdd_t *newTo;

      supp = Ddi_VarsetMakeFromVar(Ddi_VararrayRead(q, i));
      newTo = Ddi_BddForall(to, supp);
      Ddi_Free(supp);
      if (!Ddi_BddIsZero(newTo) && (Ddi_BddSize(newTo) < Ddi_BddSize(to))) {
        Ddi_Free(to);
        to = newTo;
        printf("%d,", Ddi_BddSize(to));
        fflush(stdout);
        opt->trav.univQuantifyDone = 1;
        if (Ddi_BddSize(to) < th) {
#if 0
          break;
#endif
        }
      } else {
        if (Ddi_BddIsZero(newTo)) {
          Ddi_Free(newTo);
          break;
        } else {
          Ddi_Free(newTo);
        }
      }
    }
    printf("\n");
    Ddi_Free(q);
  }

  return (to);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
FbvSetDdiMgrOpt(
  Ddi_Mgr_t * ddiMgr,
  Fbv_Globals_t * opt,
  int phase
)
{

  switch (phase) {
    case 0:
      if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
        ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
        Ddi_MgrSetVerbosity(ddiMgr, Pdtutil_VerbLevel_e(opt->verbosity));
      }

      Ddi_MgrSiftEnable(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"));
      Ddi_MgrSetSiftThresh(ddiMgr, opt->ddi.siftTh);
      Ddi_MgrSetExistOptLevel(ddiMgr, opt->ddi.exist_opt_level);
      Ddi_MgrSetExistOptClustThresh(ddiMgr, opt->ddi.exist_opt_thresh);

      Ddi_MgrSetAigDynAbstrStrategy(ddiMgr, opt->ddi.dynAbstrStrategy);
      Ddi_MgrSetAigTernaryAbstrStrategy(ddiMgr, opt->ddi.ternaryAbstrStrategy);
      Ddi_MgrSetAigItpAbc(ddiMgr, opt->ddi.itpAbc);
      Ddi_MgrSetAigItpCore(ddiMgr, opt->ddi.itpCore);
      Ddi_MgrSetAigItpMem(ddiMgr, opt->ddi.itpMem);
      Ddi_MgrSetAigItpOpt(ddiMgr, opt->ddi.itpOpt);
      Ddi_MgrSetAigItpStore(ddiMgr, opt->ddi.itpStore);
      Ddi_MgrSetAigItpPartTh(ddiMgr, opt->ddi.itpPartTh);
      Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpActiveVars_c, inum, opt->ddi.itpActiveVars);
      Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStoreTh_c, inum, opt->ddi.itpStoreTh);
      Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpAigCore_c, inum, opt->ddi.itpAigCore);
      Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpTwice_c, inum, opt->ddi.itpTwice);
      Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpIteOptTh_c, inum,
        opt->ddi.itpIteOptTh);
      Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStructOdcTh_c, inum,
        opt->ddi.itpStructOdcTh);
      Ddi_MgrSetOption(ddiMgr, Pdt_DdiNnfClustSimplify_c, inum,
        opt->ddi.nnfClustSimplify);
      Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMap_c, inum, opt->ddi.itpMap);
      Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompute_c, inum, opt->ddi.itpCompute);
      Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpLoad_c, pchar, opt->ddi.itpLoad);
      Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpDrup_c, inum, opt->ddi.itpDrup);
      Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompact_c, inum, opt->ddi.itpCompact);
      Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpClust_c, inum, opt->ddi.itpClust);
      Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpNorm_c, inum, opt->ddi.itpNorm);
      Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpSimp_c, inum, opt->ddi.itpSimp);

      break;

    case 1:

      Ddi_MgrSetAigBddOptLevel(ddiMgr, opt->ddi.aigBddOpt);
      Ddi_MgrSetAigAbcOptLevel(ddiMgr, opt->common.aigAbcOpt);
      Ddi_MgrSetAigCnfLevel(ddiMgr, opt->ddi.aigCnfLevel);
      Ddi_MgrSetAigRedRemLevel(ddiMgr, opt->ddi.aigRedRem);
      Ddi_MgrSetAigRedRemMaxCnt(ddiMgr, opt->ddi.aigRedRemPeriod);
      Ddi_MgrSetSatSolver(ddiMgr, opt->common.satSolver);
      Ddi_MgrSetAigSatTimeout(ddiMgr, opt->ddi.satTimeout);
      Ddi_MgrSetAigSatIncremental(ddiMgr, opt->ddi.satIncremental);
      Ddi_MgrSetAigSatVarActivity(ddiMgr, opt->ddi.satVarActivity);
      Ddi_MgrSetAigLazyRate(ddiMgr, opt->common.lazyRate);
      Ddi_MgrSetSiftMaxThresh(ddiMgr, opt->ddi.siftMaxTh);
      Ddi_MgrSetSiftMaxCalls(ddiMgr, opt->ddi.siftMaxCalls);
      if (opt->trav.fbvMeta && opt->trav.metaMethod == Ddi_Meta_Size) {
        Ddi_MetaInit(ddiMgr, opt->trav.metaMethod, NULL, 0, NULL, NULL,
          opt->trav.fbvMeta);
        Ddi_MgrSetMetaMaxSmooth(ddiMgr, opt->common.metaMaxSmooth);
        opt->trav.fbvMeta = 1;
      }

      break;

    case 2:

      Ddi_MgrSetTimeInitial(ddiMgr);
      Ddi_MgrSetMemoryLimit(ddiMgr, opt->expt.bddMemoryLimit);
      Ddi_MgrSetTimeLimit(ddiMgr, opt->expt.bddTimeLimit);
      Ddi_MgrSetSiftMaxThresh(ddiMgr, opt->ddi.siftMaxTh);
      Ddi_MgrSetSiftMaxCalls(ddiMgr, opt->ddi.siftMaxCalls);

      break;

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
void
FbvSetMgrOpt(
  Mc_Mgr_t * mcMgr,
  Fbv_Globals_t * opt,
  int phase
)
{

  Pdtutil_OptList_t *optList = FbvOpt2OptList(opt);

  /* transfer to MC manager */
  Mc_MgrSetMgrOptList(mcMgr, optList);

  /* free optList */

#if 0
  //  Fsm_MgrSetCegarStrategy (Mgr, opt->fsm.cegarStrategy);

  switch (phase) {
    case 0:
      if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
        ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
        Fsm_MgrSetVerbosity(fsmMgr, Pdtutil_VerbLevel_e(opt->verbosity));
      }

      Fsm_MgrSetUseAig(fsmMgr, 1);
      Fsm_MgrSetCutThresh(fsmMgr, opt->fsm.cut);
      Fsm_MgrSetOption(fsmMgr, Pdt_FsmCutAtAuxVar_c, inum,
                       opt->fsm.cutAtAuxVar);
      break;


    default:
      break;

  }

#endif
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
FbvSetFsmMgrOpt(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt,
  int phase
)
{

  Fsm_MgrSetCegarStrategy(fsmMgr, opt->fsm.cegarStrategy);

  switch (phase) {
    case 0:
      if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
        ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
        Fsm_MgrSetVerbosity(fsmMgr, Pdtutil_VerbLevel_e(opt->verbosity));
      }

      Fsm_MgrSetUseAig(fsmMgr, 1);
      Fsm_MgrSetCutThresh(fsmMgr, opt->fsm.cut);
      Fsm_MgrSetOption(fsmMgr, Pdt_FsmCutAtAuxVar_c, inum,
                       opt->fsm.cutAtAuxVar);
      break;


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
void
FbvSetTrMgrOpt(
  Tr_Mgr_t * trMgr,
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt,
  int phase
)
{
  Ddi_Mgr_t *ddiMgr;
  Ddi_Vararray_t *pi, *ps, *ns;

  if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
    ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
    Tr_MgrSetVerbosity(trMgr, Pdtutil_VerbLevel_e(opt->verbosity));
  }

  Tr_MgrSetSortMethod(trMgr, Tr_SortWeight_c);
  Tr_MgrSetClustSmoothPi(trMgr, 0);
  Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
  Tr_MgrSetImgClustThreshold(trMgr, opt->trav.imgClustTh);
  Tr_MgrSetImgApproxMinSize(trMgr, opt->tr.approxImgMin);
  Tr_MgrSetImgApproxMaxSize(trMgr, opt->tr.approxImgMax);
  Tr_MgrSetImgApproxMaxIter(trMgr, opt->tr.approxImgIter);
  Tr_MgrSetCoiLimit(trMgr, opt->tr.coiLimit);
  pi = Fsm_MgrReadVarI(fsmMgr);
  ps = Fsm_MgrReadVarPS(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);

  if (Fsm_MgrReadVarAuxVar(fsmMgr) != NULL) {
    Ddi_Vararray_t *auxvArray = Fsm_MgrReadVarAuxVar(fsmMgr);

    opt->fsm.auxVarsUsed = 1;
    Ddi_VararrayAppend(pi, auxvArray);
    Tr_MgrSetAuxVars(trMgr, Fsm_MgrReadVarAuxVar(fsmMgr));
    Tr_MgrSetAuxFuns(trMgr, Fsm_MgrReadAuxVarBDD(fsmMgr));
  }

  Tr_MgrSetI(trMgr, pi);
  Tr_MgrSetPS(trMgr, ps);
  Tr_MgrSetNS(trMgr, ns);
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
FbvSetHeuristicOpt(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt,
  int heuristic
)
{
  Ddi_Mgr_t *ddiMgr = Fsm_MgrReadDdManager(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  int num_ff = Ddi_BddarrayNum(delta);
  int num_pi = Ddi_VararrayNum(Fsm_MgrReadVarI(fsmMgr));
  int deltaSize = Ddi_BddarraySize(delta);

  opt->mc.method = Mc_VerifExactFwd_c;

  opt->trav.lemmasTimeLimit = 35 + 25 * (deltaSize > 3000);

  opt->expt.bmcSteps = 100;
  opt->expt.bmcTimeLimit = 200;
  opt->expt.indTimeLimit = 10;
  opt->expt.indSteps = 10;
  opt->expt.lemmasSteps = 2 * (deltaSize < 10000);
  opt->ddi.dynAbstrStrategy = 1;
  opt->ddi.siftMaxTh = 2000000;

  //  opt->trav.itpPart = 1;
  //  opt->trav.itpBoundkOpt = 2;
  //  opt->trav.igrGrowCone = 2;

  Ddi_MgrSetAigDynAbstrStrategy(ddiMgr, opt->ddi.dynAbstrStrategy);
  Ddi_MgrSetAigItpCore(ddiMgr, opt->ddi.itpCore);
  Ddi_MgrSetAigItpMem(ddiMgr, opt->ddi.itpMem);
  Ddi_MgrSetAigItpOpt(ddiMgr, opt->ddi.itpOpt);
  Ddi_MgrSetAigItpStore(ddiMgr, opt->ddi.itpStore);
  Ddi_MgrSetAigItpAbc(ddiMgr, opt->ddi.itpAbc);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpAigCore_c, inum, opt->ddi.itpAigCore);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpTwice_c, inum, opt->ddi.itpTwice);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpIteOptTh_c, inum, opt->ddi.itpIteOptTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStructOdcTh_c, inum,
    opt->ddi.itpStructOdcTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiNnfClustSimplify_c, inum,
    opt->ddi.nnfClustSimplify);

  opt->expt.doRunPdr = 1;

  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMap_c, inum, opt->ddi.itpMap);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompute_c, inum, opt->ddi.itpCompute);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpLoad_c, pchar, opt->ddi.itpLoad);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpDrup_c, inum, opt->ddi.itpDrup);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompact_c, inum, opt->ddi.itpCompact);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpClust_c, inum, opt->ddi.itpClust);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpNorm_c, inum, opt->ddi.itpNorm);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpSimp_c, inum, opt->ddi.itpSimp);


  switch (heuristic) {
    case Fbv_HeuristicBmcBig_c:    /* BMC */
      opt->pre.doCoi = 0;
      opt->expt.doRunBmc = 1;
      opt->expt.doRunIgr = 1;
      opt->expt.bmcSteps = -1;
#if 0
      if (deltaSize > 200000)
	opt->expt.pflMaxThreads = 2;
      if (deltaSize > 500000)
	opt->expt.pflMaxThreads = 1;
#endif
      opt->expt.bmcTimeLimit = -1;
      opt->pre.noTernary = 1;
      opt->pre.ternarySim = 0;
      break;
    case Fbv_HeuristicIntel_c: /* intel */
    case Fbv_HeuristicIbm_c:   /* ibm */
      if (deltaSize<1000000)
	opt->expt.doRunItp = 1;
      opt->ddi.aigBddOpt = 1;
      opt->expt.itpTimeLimit = -1;
      opt->expt.itpBoundLimit = -1;
      opt->expt.bmcSteps = -1;
      opt->expt.bmcTimeLimit = -1;

      if (heuristic == Fbv_HeuristicIntel_c) {
        opt->pre.reduceCustom = 1;
      }
      opt->pre.twoPhaseRed = 1;
      if (num_ff > 5000 || num_pi > 5000) {
        opt->pre.twoPhaseRed = 0;
      }
      /* itp4 setting */
      opt->mc.aig = 1;
      // opt->trav.itpPart = 1;
      opt->trav.lemmasTimeLimit = 120;
      opt->pre.peripheralLatches = 1;
      // opt->pre.performAbcOpt = 0;
      opt->pre.abcSeqOpt = 0;
      opt->ddi.aigRedRemPeriod = 100;
      opt->mc.lazy = 1;
      opt->common.lazyRate = 1.0;
      opt->trav.dynAbstr = 0;
      opt->trav.itpAppr = 0;
      opt->trav.implAbstr = 3;
      // opt->trav.ternaryAbstr = 1;

      if (1) {
        int doItp0 = 0;

        if (heuristic == Fbv_HeuristicIntel_c && !opt->mc.useChildProc) {
          doItp0 |= (num_ff < 190 || num_ff > 250);
        }
        doItp0 |= (num_ff > 2000);
        /* differenciate thread vs. proc */
        if (doItp0) {
          /* itp0 */
          opt->expt.lemmasSteps = 0;
          opt->trav.itpAppr = 0;
          opt->trav.implAbstr = 0;
          opt->trav.ternaryAbstr = 0;
          opt->common.lazyRate = 0.9;
          opt->trav.dynAbstr = 0;
          // opt->ddi.aigBddOpt = 0;
          opt->common.aigAbcOpt = 0;
          opt->trav.itpEnToPlusImg = 0;
        }
      }

      if (heuristic == Fbv_HeuristicIbm_c) {
        if (num_ff < 250 || num_pi < 200) {
          // opt->trav.itpBoundkOpt = 2; 
          opt->trav.implAbstr = 0;
          opt->expt.doRunBdd = 1;
          // opt->pre.twoPhaseRed = 0;
          opt->fsm.cut = 1000;
        }
      }
      break;
    case Fbv_HeuristicDme_c:   /* dme */
      opt->expt.doRunBdd = 2;
      opt->expt.ddiMgrAlign = 1;
      opt->ddi.siftTravTh = 4000;
      opt->trav.clustTh = 1000;
      // opt->ddi.siftMaxTh = 200000;
      opt->trav.siftMaxTravIter = 10;
      break;
    case Fbv_HeuristicQueue_c: /* queue */
      opt->pre.peripheralLatches = 0;
      opt->pre.ternarySim = 1;
      opt->expt.doRunPdr = 0;
      opt->expt.doRunBdd = 1;
      opt->fsm.cut = 10000;
      // opt->trav.imgDpartTh = 100000;
      break;
    case Fbv_HeuristicNeedham_c:   /* ns2-ns3 */
      // opt->expt.doRunInd = 1;
      // opt->expt.indSteps = 20;
      // opt->expt.indTimeLimit = 10;
      opt->expt.doRunBdd = 1;
      opt->trav.clustTh = 300;
      opt->trav.imgDpartTh = 500000;
      break;
    case Fbv_HeuristicSec_c:   /* sec */
      opt->expt.doRunLemmas = 1;
      opt->expt.doRunItp = 1;
      opt->trav.lemmasTimeLimit = -1;
      opt->expt.lemmasSteps = 10;
      opt->pre.retime = num_ff > 20;

      opt->pre.twoPhaseRed = 1;

      /* itp5 setting for interpolation (just in case ...) */
      opt->mc.aig = 1;
      opt->mc.lazy = 1;
      opt->common.lazyRate = 1.0;
      opt->trav.dynAbstr = 0;
      opt->trav.itpAppr = 1;
      opt->trav.implAbstr = 3;
      opt->trav.ternaryAbstr = 0;
      opt->trav.itpEnToPlusImg = 0;
      if ((num_ff < 150) && (deltaSize < 3000)) {
        opt->expt.doRunBdd = 1;
        opt->ddi.siftMaxTh = 500000;
      }
      break;
    case Fbv_HeuristicPdtSw_c: /* pdt-sw */
      opt->expt.doRunBmc = 1;
      opt->expt.doRunBdd = 1;
      opt->expt.ddiMgrAlign = 1;
      if (num_ff < 75 && num_ff > 65) {
        opt->mc.method = Mc_VerifApproxFwdExactBck_c;
        //opt->mc.method = Mc_VerifExactBck_c;
        opt->trav.apprOverlap = 0;
        opt->trav.apprGrTh = 1000;
        opt->trav.clustTh = 5000;
        opt->fsm.cut = 1000; 
       opt->trav.imgDpartTh = 300000;
      } else if (num_ff < 110) {
        // opt->fsm.cut = -1;
        opt->fsm.cut = 1000;
        opt->ddi.siftTravTh = 5000;
        opt->trav.clustTh = 200;
        opt->ddi.siftMaxTh = 1000000;
      } else {
        // opt->fsm.cut = -1;
        opt->fsm.cut = 1000;
        opt->ddi.siftTravTh = 5000;
        opt->ddi.siftMaxTh = 1000000;
        opt->trav.clustTh = 5000;
        // opt->ddi.siftMaxTh = 500000;
      }
      opt->expt.bmcSteps = 70;
      opt->expt.bmcTimeLimit = 120;
      opt->expt.bddTimeLimit = -1;

      break;
    case Fbv_HeuristicBddFull_c:   /* full-bdd */
      opt->expt.doRunBmc = 1;
      //opt->expt.doRunInd = 1;
      opt->expt.doRunLms = 1;   // i lemmi girano fuori dall'interpolante
      opt->expt.doRunItp = 1;
      opt->expt.doRunBdd = 1;
      opt->expt.doRunPdr = 0;

      opt->expt.bmcSteps = 35;
      opt->expt.bmcTimeLimit = 15;
      opt->expt.indSteps = 10;
      opt->expt.indTimeLimit = 10;
      opt->trav.lemmasTimeLimit = 15;
      opt->expt.itpBoundLimit = 100;
      opt->expt.itpTimeLimit = -1;
      opt->expt.bddTimeLimit = -1;  //30+15;

      /* itp5 setting */
      opt->mc.aig = 1;
      opt->expt.lemmasSteps = 0;
      opt->pre.peripheralLatches = 1;
      // opt->pre.performAbcOpt = 0;
      opt->ddi.itpIteOptTh = 300000;
      opt->pre.abcSeqOpt = 0;
      opt->ddi.aigRedRemPeriod = 100;
      opt->mc.lazy = 1;
      // opt->common.lazyRate = 2.3;
      // opt->trav.itpPart = 1;
      // opt->trav.itpAppr = 0;
      // opt->trav.abstrRef = 2;
      // opt->trav.dynAbstr = 2;
      // opt->trav.implAbstr = 0;
      opt->trav.itpBoundkOpt = 2;
      opt->trav.ternaryAbstr = 0;
      opt->trav.itpInductiveTo = 2;

      opt->trav.clustTh = 5000;
      opt->fsm.cut = -1;
      opt->trav.imgDpartTh = 100000;
      break;
    case Fbv_HeuristicBdd_c:   /* bdd */
      opt->expt.doRunBmc = 1;
      opt->expt.doRunInd = 1;
      opt->expt.doRunLms = 0;   //lemmi abilitati con l'interpolante
      opt->expt.doRunItp = 1;
      opt->expt.doRunBdd = 1;
      opt->expt.doRunBdd2 = 0;

      opt->expt.bmcSteps = 40;
      opt->expt.bmcTimeLimit = 30;
      opt->expt.indSteps = 10;
      opt->expt.indTimeLimit = 10;
      opt->expt.lemmasSteps = 2 * (deltaSize < 7000);   //lemmas enabled with ITPs
      opt->trav.lemmasTimeLimit = 40;
      opt->expt.itpBoundLimit = 100;
      opt->expt.itpTimeLimit = -1;

      /* itp4 setting */
      opt->mc.aig = 1;
      // opt->trav.itpPart = 1;
      opt->pre.peripheralLatches = 1;
      // opt->pre.performAbcOpt = 0;
      opt->pre.abcSeqOpt = 0;
      opt->ddi.aigRedRemPeriod = 100;
      opt->mc.lazy = 1;
      opt->common.lazyRate = 1.0;
      opt->trav.dynAbstr = 0;
      opt->trav.itpAppr = 0;
      opt->trav.implAbstr = 3;
      // opt->trav.ternaryAbstr = 1;

      opt->fsm.cut = 500;
      opt->trav.imgDpartTh = 100000;
      break;
    case Fbv_HeuristicRelational_c:    /* other */
      opt->pre.reduceCustom = 1;
      opt->expt.doRunBmc = 1;
      opt->expt.doRunInd = 1;
      opt->expt.doRunItp = 1;
      // opt->pre.retime = 1;

      opt->expt.bmcSteps = 200;
      opt->expt.bmcTimeLimit = -1;
      opt->expt.indSteps = 10;
      opt->expt.indTimeLimit = 20;
      opt->expt.lemmasSteps = 2 * (deltaSize < 10000);
      opt->trav.lemmasTimeLimit = 60;
      opt->expt.itpBoundLimit = -1;
      opt->expt.itpTimeLimit = -1;

      /* itp4 setting */
      opt->mc.aig = 1;
      // opt->trav.itpPart = 1;
      opt->pre.peripheralLatches = 1;
      // opt->pre.performAbcOpt = 0;
      opt->pre.abcSeqOpt = 0;
      opt->ddi.aigRedRemPeriod = 100;
      opt->mc.lazy = 1;
      opt->common.lazyRate = 1.0;
      // opt->trav.dynAbstr = 2;
      opt->trav.itpAppr = 0;
      opt->trav.implAbstr = 0;
      // opt->trav.ternaryAbstr = 1;

      break;

    case Fbv_HeuristicBeem_c:  /* beem */
      opt->pre.reduceCustom = 1;
      opt->expt.doRunBmc = 1;
      opt->expt.doRunInd = 0;
      opt->expt.doRunItp = 1;
      // opt->pre.retime = 1;

      opt->expt.bmcSteps = 50;
      opt->expt.bmcTimeLimit = 40;
      opt->expt.indSteps = 10;
      opt->expt.indTimeLimit = 20;
      opt->expt.lemmasSteps = 2 * (deltaSize < 10000);
      opt->trav.lemmasTimeLimit = 60;
      opt->expt.itpBoundLimit = -1;
      opt->expt.itpTimeLimit = -1;

      /* itp1 setting */
      opt->mc.aig = 1;
      // opt->trav.itpPart = 1;
      opt->pre.peripheralLatches = 1;
      // opt->pre.performAbcOpt = 0;
      opt->pre.abcSeqOpt = 0;
      opt->ddi.aigRedRemPeriod = 100;
      opt->mc.lazy = 1;
      opt->common.lazyRate = 1.0;
      // opt->trav.dynAbstr = 2;
      opt->trav.implAbstr = 0;
      opt->trav.ternaryAbstr = 0;
      break;
    case Fbv_HeuristicCpu_c: /* cpu */
    case Fbv_HeuristicOther_c: /* other */
      opt->expt.doRunBdd = num_ff < 600;
      opt->expt.doRunBmc = 1;
      opt->expt.doRunInd = 1;
      opt->expt.doRunItp = 1;
      // opt->pre.retime = 1;

      opt->expt.bmcSteps = 50;
      opt->expt.bmcTimeLimit = 40;
      opt->expt.indSteps = 10;
      opt->expt.indTimeLimit = 20;
      opt->expt.lemmasSteps = 2 * (deltaSize < 10000);
      opt->trav.lemmasTimeLimit = 60;
      opt->expt.itpBoundLimit = -1;
      opt->expt.itpTimeLimit = -1;

      /* itp4 setting */
      opt->mc.aig = 1;
      // opt->trav.itpPart = 1;
      opt->pre.peripheralLatches = 1;
      // opt->pre.performAbcOpt = 0;
      opt->pre.abcSeqOpt = 0;
      opt->ddi.aigRedRemPeriod = 100;
      opt->mc.lazy = 1;
      opt->common.lazyRate = 1.0;
      // opt->trav.dynAbstr = 2;
      opt->trav.itpAppr = 1;
      if (num_ff > 500 && num_ff < 2000) {
        opt->trav.implAbstr = 2;
      }
      // opt->trav.ternaryAbstr = 1;

      break;
    case Fbv_HeuristicNone_c:
      break;
    default:                   /* not possible */
      printf("Unknown verification strategy!");
      exit(1);
  }

  if (opt->expt.expertArgv != NULL) {
    FbvParseArgs(opt->expt.expertArgc, opt->expt.expertArgv, opt);
  }
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Fsm_Mgr_t *
FbvFsmMgrLoad(
  Fsm_Mgr_t * fsmMgr,
  char *fsm,
  Fbv_Globals_t * opt
)
{
  Ddi_Vararray_t *ps;
  Ddi_Bdd_t *init = NULL, *invar = NULL, *invarspec = NULL;
  Ddi_Mgr_t *ddiMgr = Fsm_MgrReadDdManager(fsmMgr);
  Ddi_Bddarray_t *lambda;

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "-- FSM Loading.\n"));

  if (strstr(fsm, ".fsm") != NULL) {
    if (strcmp(opt->mc.ord, "dfs") == 0) {
      Pdtutil_Free(opt->mc.ord);
      opt->mc.ord = NULL;
    }
    if (Fsm_MgrLoad(&fsmMgr, ddiMgr, fsm, opt->mc.ord, 1,
        Pdtutil_VariableOrderDefault_c) == 1) {
      fprintf(stderr, "-- FSM Loading Error.\n");
      exit(1);
    }
  } else if (strstr(fsm, ".aig") != NULL || strstr(fsm, ".aag") != NULL) {
    if (strcmp(opt->mc.ord, "dfs") == 0) {
      Pdtutil_Free(opt->mc.ord);
      opt->mc.ord = NULL;
    }
    if (Fsm_MgrLoadAiger(&fsmMgr, ddiMgr, fsm, opt->mc.ord, NULL,
        Pdtutil_VariableOrderDefault_c) == 1) {
      fprintf(stderr, "-- FSM Loading Error.\n");
      exit(1);
    }
  } else {
    char **ci = Pdtutil_Alloc(char *, 4
    );
    int ciNum = 0;
    int myCoiLimit = 0;

    ci[0] = NULL;

    if (0 && strcmp(opt->mc.ord, "dfs") == 0) {
      Pdtutil_Free(opt->mc.ord);
      opt->mc.ord = NULL;
    }
    if (opt->pre.doCoi > 1) {
      if (opt->mc.invSpec != NULL) {
        Ddi_Expr_t *e = Ddi_ExprLoad(ddiMgr, opt->mc.invSpec, NULL);

        ci = Expr_ListStringLeafs(e);
        ciNum = 1;
        Ddi_Free(e);
      } else if (opt->mc.ctlSpec != NULL) {
        Ddi_Expr_t *e = Ddi_ExprLoad(ddiMgr, opt->mc.ctlSpec, NULL);

        ci = Expr_ListStringLeafs(e);
        ciNum = 1;
        Ddi_Free(e);
      } else if (opt->mc.all1) {
        ci[ciNum] = Pdtutil_Alloc(char,
          30
        );

        sprintf(ci[ciNum], "l %d %d %d %d", 0, opt->mc.all1 - 1,
          myCoiLimit, opt->pre.constrCoiLimit);
        ci[++ciNum] = NULL;
      } else if (opt->mc.idelta >= 0) {
        ci[ciNum] = Pdtutil_Alloc(char,
          30
        );

        sprintf(ci[ciNum], "l %d %d %d %d", opt->mc.idelta, opt->mc.idelta,
          myCoiLimit, opt->pre.constrCoiLimit);
        ci[++ciNum] = NULL;
      } else if (opt->mc.nlambda != NULL) {
        ci[ciNum] = Pdtutil_Alloc(char,
          strlen(opt->mc.nlambda) + 4
        );

        sprintf(ci[ciNum], "n %s %d %d", opt->mc.nlambda,
          myCoiLimit, opt->pre.constrCoiLimit);
        ci[++ciNum] = NULL;
        opt->mc.ilambda = 0;
      } else {
        ci[ciNum] = Pdtutil_Alloc(char,
          30
        );

        sprintf(ci[ciNum], "o %d %d %d %d", opt->mc.ilambda, opt->mc.ilambda,
          myCoiLimit, opt->pre.constrCoiLimit);
        ci[++ciNum] = NULL;
        opt->mc.ilambda = 0;
      }
    }

    if (opt->mc.ninvar != NULL) {
      ci[ciNum] = Pdtutil_Alloc(char,
        strlen(opt->mc.ninvar) + 4
      );

      sprintf(ci[ciNum], "i %s %d %d", opt->mc.ninvar, myCoiLimit,
        opt->pre.constrCoiLimit);
      ci[++ciNum] = NULL;
    }
    if (Fsm_MgrLoadFromBlifWithCoiReduction(&fsmMgr, ddiMgr, fsm, opt->mc.ord,
        1, Pdtutil_VariableOrderDefault_c, ci) == 1) {
      fprintf(stderr, "BLIF File Loading Error.\n");
    }
    if (ci != NULL) {
      Pdtutil_StrArrayFree(ci, -1);
    }
  }

  if (Ddi_BddarrayNum(Fsm_MgrReadDeltaBDD(fsmMgr)) > 20000) {
    /* too big */
    opt->mc.nogroup = 1;
    Ddi_MgrSetSiftThresh(ddiMgr, 100000000);
  }

  if (0 && (!opt->mc.nogroup)) {
    Ddi_MgrCreateGroups2(Fsm_MgrReadDdManager(fsmMgr),
      Fsm_MgrReadVarPS(fsmMgr), Fsm_MgrReadVarNS(fsmMgr));
  }

  if (opt->pre.initMinterm >= 0) {
    Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
    Ddi_Varset_t *initSupp = Ddi_BddSupp(init);
    Ddi_Varset_t *forceVars =
      Ddi_VarsetMakeFromArray(Fsm_MgrReadVarPS(fsmMgr));
    Ddi_VarsetDiffAcc(forceVars, initSupp);
    Ddi_Free(initSupp);
    Pdtutil_Assert(opt->pre.initMinterm <= 1,
      "invalid initMinterm value (0/1 allowed)");
    if (!Ddi_VarsetIsVoid(forceVars)) {
      Ddi_Vararray_t *vA = Ddi_VararrayMakeFromVarset(forceVars, 1);
      int j;

      for (j = 0; j < Ddi_VararrayNum(vA); j++) {
        Ddi_Var_t *v = Ddi_VararrayRead(vA, j);
        Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(v, opt->pre.initMinterm);

        Ddi_BddAndAcc(init, lit);
        Ddi_Free(lit);
      }
      Ddi_Free(vA);
    }
    Ddi_Free(forceVars);
  }

  if (0 && Ddi_BddarraySize(Fsm_MgrReadDeltaBDD(fsmMgr)) > 20000) {
    int aol = Ddi_MgrReadAigAbcOptLevel(ddiMgr);

    Ddi_MgrSetAigAbcOptLevel(ddiMgr, 2);
    Ddi_AigarrayAbcOptAcc(Fsm_MgrReadDeltaBDD(fsmMgr), 60.0);
    Ddi_MgrSetAigAbcOptLevel(ddiMgr, aol);
  }

  if (Fsm_MgrReadVarPS(fsmMgr) == NULL) {
    opt->trav.noInit = 1;
    opt->mc.combinationalProblem = 1;
  }

  if (opt->trav.noInit) {
    init = Ddi_BddMakeConst(ddiMgr, 0);
  } else if (opt->mc.rInit != NULL) {
    Ddi_Var_t *v = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
    Ddi_Var_t *v1 = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

    if (1 && !v) {
      char buf[256];

      sprintf(buf, "PDT_BDD_INVARSPEC_VAR$PS");
      v = Ddi_VarNew(ddiMgr);
      Ddi_VarAttachName(v, buf);
      sprintf(buf, "PDT_BDD_INVAR_VAR$PS");
      v = Ddi_VarNew(ddiMgr);
      Ddi_VarAttachName(v, buf);
    }

    printf("\n**** Reading INIT set from %s ****\n", opt->mc.rInit);
    init = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
      DDDMP_MODE_DEFAULT, opt->mc.rInit, NULL);

    if (v != NULL) {
      Ddi_BddCofactorAcc(init, v, 1);
    }
    if (v1 != NULL) {
      Ddi_BddCofactorAcc(init, v1, 1);
    }

    Ddi_MgrReduceHeap(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"), 0);
    Ddi_BddSetAig(init);
    Ddi_MgrSetSiftThresh(ddiMgr, opt->ddi.siftTh);
  } else {
    init = Ddi_BddDup(Fsm_MgrReadInitBDD(fsmMgr));
  }

  Fsm_MgrSetInitBDD(fsmMgr, init);
  Ddi_Free(init);

  if (opt->common.clk != NULL) {
    Fsm_MgrSetClkStateVar(fsmMgr, opt->common.clk, 1);
  }

  if (opt->mc.ninvar != NULL) {
    int i;

    invar = NULL;
    for (i = 0; i < Ddi_BddarrayNum(lambda); i++) {
      char *name = Ddi_ReadName(Ddi_BddarrayRead(lambda, i));

      if (name != NULL && strcmp(name, opt->mc.ninvar) == 0) {
        invar = Ddi_BddDup(Ddi_BddarrayRead(lambda, i));
        break;
      }
    }
    Pdtutil_Assert(invar != NULL, "Lambda not found by name");
    Fsm_MgrSetConstraintBDD(fsmMgr, invar);
    Ddi_Free(invar);
  }

  lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  if (opt->mc.ilambda >= Ddi_BddarrayNum(lambda)) {
    opt->mc.ilambda = Ddi_BddarrayNum(lambda) - 1;
    printf("ilambda selection is too large: %d forced\n", opt->mc.ilambda);
  }

  invar = Ddi_BddDup(Fsm_MgrReadConstraintBDD(fsmMgr));
  if (opt->trav.noCheck) {
    invarspec = Ddi_BddMakeConstAig(ddiMgr, 1);

    if (opt->mc.method == Mc_VerifPrintNtkStats_c) {
      Ddi_Vararray_t *pi, *ps;
      Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);

      pi = Fsm_MgrReadVarI(fsmMgr);
      ps = Fsm_MgrReadVarPS(fsmMgr);
      printf("%d inputs / %d state variables / %d delta nodes \n",
        Ddi_VararrayNum(pi), Ddi_VararrayNum(ps), Ddi_BddarraySize(delta));
    }
  } else {
    invarspec = FbvGetSpec(fsmMgr, NULL, invar, NULL, opt);
  }

  if (!Ddi_BddIsAig(invarspec)) {
    Ddi_Bdd_t *newI = Ddi_AigMakeFromBdd(invarspec);

    Ddi_Free(invarspec);
    invarspec = newI;
  }
  Fsm_MgrSetInvarspecBDD(fsmMgr, invarspec);
  if (Ddi_BddIsConstant(invarspec)) {
    opt->mc.combinationalProblem = 1;
  }

  Ddi_Free(invarspec);
  Ddi_Free(invar);

  return fsmMgr;

}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
FbvFsmMgrLoadRplus(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt
)
{
  Ddi_Mgr_t *ddiMgr = Fsm_MgrReadDdManager(fsmMgr);

  if (opt->trav.rPlus != NULL) {
    char *pbench;
    Ddi_Bdd_t *care;
    Ddi_Var_t *v = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
    Ddi_Var_t *v1 = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "-- Reading REACHED set from %s.\n", opt->trav.rPlus));
    if ((pbench = strstr(opt->trav.rPlus, ".bench")) != NULL) {
      //      *pbench = '\0';
      care = Ddi_AigNetLoadBench(ddiMgr, opt->trav.rPlus, NULL);
      if (v != NULL) {
        Ddi_BddCofactorAcc(care, v, 1);
      }
      if (v1 != NULL) {
        Ddi_BddCofactorAcc(care, v1, 1);
      }
      //      care = Ddi_BddarrayExtract(cArray,0);
    } else {
      Ddi_Var_t *v = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
      Ddi_Var_t *v1 = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

      care = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
        DDDMP_MODE_DEFAULT, opt->trav.rPlus, NULL);
      if (v != NULL) {
        Ddi_BddCofactorAcc(care, v, 1);
      }
      if (v1 != NULL) {
        Ddi_BddCofactorAcc(care, v1, 1);
      }
      Ddi_MgrReduceHeap(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"), 0);
    }
    Ddi_BddSetAig(care);

    Fsm_MgrSetCareBDD(fsmMgr, care);

    Ddi_MgrSetSiftThresh(ddiMgr, opt->ddi.siftTh);
  }

}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
FbvFsmReductions(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt,
  int phase
)
{
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  int constrSize = Ddi_BddSize(Fsm_MgrReadConstraintBDD(fsmMgr));
  Ddi_Var_t *vc = Ddi_VarFromName(ddm,"AIGUNCONSTRAINT_INVALID_STATE");
  int hasConstraints = vc!=NULL || constrSize>1;
  
  int loopReduce, loopReduceCnt;
  int ret = 0;

  Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "\nRunning reductions...(phase %d)\n", phase));

  switch (phase) {

    case 0:

      if (opt->pre.peripheralLatches) {
        int nRed;
        int ngate = Ddi_BddarraySize(delta);
        int npi = Ddi_VararrayNum(pi);
        int nps = Ddi_VararrayNum(ps);

        if ((hasConstraints || npi < 30000) && nps < 100000 &&
          (ngate < 25000 || (ngate < 1000000 && nps < 100000))) {
          do {
            nRed = Fsm_RetimePeriferalLatches(fsmMgr);
            opt->pre.peripheralLatchesNum += nRed;
          } while (nRed > 0);
        }
      }

      while (opt->pre.forceInitStub) {
        Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
        Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
        Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
        char suffix[4];
        Ddi_Vararray_t *newPiVars;
        Ddi_Bddarray_t *newPiLits;
        Ddi_Bdd_t *initState = Fsm_MgrReadInitBDD(fsmMgr);
        Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
        Ddi_Bddarray_t *initStub = Ddi_BddarrayDup(delta);

        if (Fsm_MgrReadInitStubBDD(fsmMgr) != NULL) {
          Ddi_BddarrayComposeAcc(initStub, ps, Fsm_MgrReadInitStubBDD(fsmMgr));
        } else {
          Ddi_Bdd_t *myInit = Ddi_BddMakeMono(initState);

          Pdtutil_Assert(Ddi_BddIsCube(myInit),
            "Init not a cube - not supported");
          Ddi_AigarrayConstrainCubeAcc(initStub, initState);
          Ddi_Free(myInit);
        }

        sprintf(suffix, "%d", opt->pre.forceInitStub);
#if 0
        newPiVars =
          Ddi_VararrayMakeNewAigVars(pi, "PDT_FORCED_STUB_PI", suffix);
#else
        newPiVars =
          Ddi_VararrayMakeNewVars(pi, "PDT_FORCED_STUB_PI", suffix, 1);
#endif
        newPiLits = Ddi_BddarrayMakeLiteralsAig(newPiVars, 1);
        Ddi_BddarrayComposeAcc(initStub, pi, newPiLits);
        Ddi_Free(newPiVars);
        Ddi_Free(newPiLits);
        Fsm_MgrSetInitStubBDD(fsmMgr, initStub);
        Ddi_Free(initStub);
        opt->pre.forceInitStub--;
        Fsm_MgrIncrInitStubSteps(fsmMgr, 1);
        Pdtutil_WresIncrInitStubSteps(1);
      }

      // just find clock
      FbvTwoPhaseReduction(fsmMgr, opt, 1, 0);
      break;

    case 1:

      if (opt->pre.terminalScc) {
        opt->stats.verificationOK = 0;
        Ddi_Bdd_t *myInvar =
          Fsm_ReduceTerminalScc(fsmMgr, &opt->pre.terminalSccNum,
          &opt->stats.verificationOK, opt->trav.itpConstrLevel);

        if (opt->stats.verificationOK == 1) {
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
            Pdtutil_VerbLevelNone_c,
            printf("HEURISTIC strategy selection: TSCC\n");
            fprintf(stdout, "Verification Result: PASS.\n"));
          Ddi_Free(myInvar);
          return opt->stats.verificationOK;
        }
        if (myInvar != NULL) {
          FbvConstrainInitStub(fsmMgr, myInvar);
        }
        if (myInvar != NULL && opt->trav.itpConstrLevel > 0) {
          Ddi_Bdd_t *invar = Fsm_MgrReadConstraintBDD(fsmMgr);

          if (invar == NULL) {
            Fsm_MgrSetConstraintBDD(fsmMgr, myInvar);
          } else {
            Ddi_BddAndAcc(invar, myInvar);
          }
        }
        Ddi_Free(myInvar);
      }

      if (opt->pre.impliedConstr) {
        if (Ddi_BddarrayNum(delta) > 10000) {
          FbvTestRelational(fsmMgr, opt);
          if (opt->pre.relationalNs < Ddi_BddarrayNum(delta) * 0.8) {
            opt->pre.impliedConstrNum = 0;
            opt->pre.impliedConstr = 0;
          }
        }
      }
      if (opt->pre.impliedConstr) {
        Ddi_Bdd_t *invar = Fsm_MgrReadConstraintBDD(fsmMgr);

        if (Ddi_BddarraySize(delta) > 1000) {
          FbvTestRelational(fsmMgr, opt);
          if (opt->pre.relationalNs < Ddi_BddarrayNum(delta) * 0.8) {
            opt->pre.impliedConstrNum = 0;
          }
        }
        Ddi_Bdd_t *myInvar = Fsm_ReduceImpliedConstr(fsmMgr,
          opt->pre.specDecompIndex, invar, &opt->pre.impliedConstrNum);

        if (myInvar != NULL && opt->trav.itpConstrLevel > 0) {
          Ddi_Bdd_t *invar = Fsm_MgrReadConstraintBDD(fsmMgr);

          if (invar == NULL) {
            Fsm_MgrSetConstraintBDD(fsmMgr, myInvar);
          } else {
            Ddi_BddAndAcc(invar, myInvar);
          }
        }
        Ddi_Free(myInvar);
      }

      break;

    case 2:

      if (opt->pre.reduceCustom) {
        Ddi_Bdd_t *invarspec = Fsm_MgrReadInvarspecBDD(fsmMgr);

        if (invarspec != NULL) {
          Ddi_BddarrayWrite(lambda, 0, invarspec);
        }
        Ddi_Bdd_t *myInvar = Fsm_ReduceCustom(fsmMgr);

        Ddi_Free(myInvar);
        FbvTestSec(fsmMgr, opt);
        FbvTestRelational(fsmMgr, opt);
        if (invarspec != NULL) {
          Ddi_DataCopy(invarspec, Ddi_BddarrayRead(lambda, 0));
        }
      }

      FbvTwoPhaseReduction(fsmMgr, opt, 0, 1);
      break;

    case 3:

      loopReduce = 1;
      for (loopReduceCnt = 0; loopReduce; loopReduceCnt++) {
        int innerLoop, loopCnt;

        Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
        int ns0 = Ddi_BddarrayNum(delta);
        int size0 = Ddi_BddarraySize(delta);

        loopReduce = 0;

        if (opt->pre.doCoi >= 1) {

          Ddi_Varsetarray_t *coi;
          int nCoi;
          Ddi_Bdd_t *invarspec = Fsm_MgrReadInvarspecBDD(fsmMgr);

          coi =
            computeFsmCoiVars(fsmMgr, invarspec, NULL, opt->tr.coiLimit, 1);
          if (opt->pre.wrCoi != NULL) {
            writeCoiShells(opt->pre.wrCoi, coi, ps);
          }
          nCoi = Ddi_VarsetarrayNum(coi);
          fsmCoiReduction(fsmMgr, Ddi_VarsetarrayRead(coi, nCoi - 1));

          Ddi_Free(coi);
        }


        loopCnt = 0;

        do {

          if (opt->pre.ternarySim == 0) {
            innerLoop = 0;
          } else {
            innerLoop = fsmConstReduction(fsmMgr, loopCnt == 0, -1, opt, 0);
          }
          loopCnt++;

          if (innerLoop < 0) {
            /* property proved */
            printf("HEURISTIC strategy selection: STS\n");
            opt->stats.verificationOK = 1;
            return 1;
          }

          loopReduce |= (innerLoop > 2);

          if (opt->pre.doCoi >= 1) {

            Ddi_Varsetarray_t *coi;
            int nCoi;
            Ddi_Bdd_t *invarspec = Fsm_MgrReadInvarspecBDD(fsmMgr);

            coi =
              computeFsmCoiVars(fsmMgr, invarspec, NULL, opt->tr.coiLimit, 1);
            if (opt->pre.wrCoi != NULL) {
              writeCoiShells(opt->pre.wrCoi, coi, ps);
            }
            nCoi = Ddi_VarsetarrayNum(coi);
            fsmCoiReduction(fsmMgr, Ddi_VarsetarrayRead(coi, nCoi - 1));

            Ddi_Free(coi);
          }

        } while (innerLoop && loopCnt < 4);

        if (opt->pre.retime) {
          int nl2, nl = Ddi_BddarrayNum(Fsm_MgrReadDeltaBDD(fsmMgr));
          int size0 = Ddi_BddarraySize(Fsm_MgrReadDeltaBDD(fsmMgr));

          if (nl > 2) {
            loopReduce |= Fsm_RetimeMinreg(fsmMgr, NULL, opt->pre.retime);
            if (Ddi_BddarraySize(Fsm_MgrReadDeltaBDD(fsmMgr)) > size0 * 1.05) {
              opt->pre.retime = 0;
            }
            nl2 = Ddi_BddarrayNum(Fsm_MgrReadDeltaBDD(fsmMgr));
            if (nl2 < nl) {
              opt->pre.retimed = 1; // retimed !
            }
          }
        }

        delta = Fsm_MgrReadDeltaBDD(fsmMgr);
        if (Ddi_BddarrayNum(delta) > ns0 * 0.98 &&
          Ddi_BddarraySize(delta) > size0 * 0.95) {
          loopReduce = 0;
        }

      }


      break;

    case 4:

      ret = Fsm_MgrAbcReduce(fsmMgr, 0.9);

      break;

    default:
      break;
  }

  return ret;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Fbv_CheckFsmCex(
  Fsm_Mgr_t * fsmMgr,
  Trav_Mgr_t * travMgr
)
{
  return checkFsmCex(fsmMgr, travMgr);
}

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Main program of fbv package.]
  Description [Main program of fbv package.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
main(
  int argc,
  char *argv[]
)
{
  char buf[PDTUTIL_MAX_STR_LEN + 1];
  int numArgs;
  char *fsmFileName, *fsmFileNameNoDir, *namebis;
  char *fsmOptFileName;
  char pid_s[PDTUTIL_MAX_STR_LEN + 1];
  size_t len;
  int verifOK;
  Fbv_Globals_t *opt;
  int resource;
  struct rlimit rlim;
  time_t t = time(NULL);
  struct tm *tmp = localtime(&t); 

  int getrlimit(
    int resource,
    struct rlimit *rlim
  );
  int setrlimit(
    int resource,
    const struct rlimit *rlim
  );

  if (argc < 2) {
    printUsage(stderr, argv[0]);
    return (1);
  }

  getrlimit(RLIMIT_AS, &rlim);
  getrlimit(RLIMIT_STACK, &rlim);
  rlim.rlim_cur *= 8;
  setrlimit(RLIMIT_STACK, &rlim);
  getrlimit(RLIMIT_STACK, &rlim);
  getrlimit(RLIMIT_DATA, &rlim);

  /*
   *  Print Out Command and Version and etc.
   */

  //Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelNone_c,
  fprintf(stdout, "\nc ## Tool Version: %s\n", PDTRAV_VERSION);
  fprintf(stdout, "c ## DD Version: ");
  Ddi_PrintCuddVersion(stdout);
  fprintf(stdout, "c ## Compile Date: %s\n", COMPILE_DATE);
  //);

  /*
   *  Read Command Line
   */

  opt = FbvParseArgs(argc, argv, NULL);
  if (opt == NULL) {
    return (1);
  }

  if (hwmccOut) {
    Pdtutil_HwmccOutputSetup();
  }

  opt->stats.fbvStartTime = util_cpu_time();


  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "## Running Date: ");
    fflush(stdout);
    printf("%s",asctime(tmp)); 
    if (gethostname(buf, PDTUTIL_MAX_STR_LEN) == 0) {
    fprintf(stdout, "## Hostname: %s\n", buf);}
  ) ;

  Pdtutil_WallClockTimeReset();
  Pdtutil_InitCpuTimeScale(1000000);

#if 0
  // PAOLO 
  Mc_Mgr_t *mcMgr;

  mcMgr = Mc_MgrInit("mcThreadedMgr", NULL);
  FbvSetMgrOpt(mcMgr, opt, 0);

#endif

#if 0
  /* competition settings */
  opt->mc.strategy = Pdtutil_StrDup("mix");
  opt->mc.compl_invarspec = 1;
  opt->pre.doCoi = 2;
  opt->ddi.siftTh = 100000;
  opt->expt.expertLevel = 2;
#endif

  opt->expName = fsmFileName = fsmFileNameNoDir = argv[argc - 1];
  if (opt->mc.strategy != NULL) {
    FILE *fout;
    char *ext;

    /* competition common settings */
    if (opt->mc.wRes == NULL) {
      char *slash = fsmFileName+strlen(fsmFileName);
      while (slash!=fsmFileName && *slash!='/') slash--;
      if (slash!=fsmFileName) {
	fsmFileNameNoDir=slash+1;
      }
      opt->mc.wRes =
        (char *)malloc((strlen(fsmFileName) + 10 +
          strlen(opt->mc.strategy)) * sizeof(char));
      sprintf(opt->mc.wRes, "%s", fsmFileNameNoDir);
      ext = opt->mc.wRes + strlen(opt->mc.wRes) - 1;
      while (*ext != '.' && ext >= opt->mc.wRes) {
        ext--;
      }
      if (strcmp(ext, ".aig") != 0 && strcmp(ext, ".aag") != 0 && strcmp(ext, ".blif") != 0) {
        printf("Error: invalid circuit file extension.\n");
        exit(1);
      }
      //sprintf(ext, ".pdtrav.%s.out", opt->mc.strategy);
      sprintf(ext, ".pdtrav.out");
    }
    if (!hwmccOut) {
      fout = fopen(opt->mc.wRes, "w");
      fclose(fout);
    }
    if (opt->mc.writeBound == 1) {
      Pdtutil_WresSetup(opt->mc.wRes, 0);
    } else if (opt->mc.writeBound == 2) {
      Pdtutil_WresSetup(opt->mc.wRes, -1);
    }
    else {
      Pdtutil_WresSetup(opt->mc.wRes, -1);
    }
  }

  /*
   *  Run ABC Tool
   */

  if (opt->mc.method == Mc_VerifABCLink_c) {
    char fsmFileNameOut[PDTUTIL_MAX_STR_LEN];

    sprintf(fsmFileNameOut, "%s.blif", opt->mc.abcOptFilename);
    fbvAbcLink(fsmFileName, fsmFileNameOut, 0, 1, 0);
    Pdtutil_Free(opt);
    return (1);
  }

  /*
   *  "Usual" Flow
   */

  if (opt->mc.ord == NULL)
    opt->mc.ord = Pdtutil_StrDup("dfs");

  /*
   * Preliminary iChecker optimization
   */

  char *benchFileName;
  char *chkFileName;
  char *transFileName;
  char *newFsmFileName;
  char cmd[PDTUTIL_MAX_STR_LEN + 1];

  snprintf(pid_s, PDTUTIL_MAX_STR_LEN, "%d", getpid());

  if (opt->pre.iChecker) {
    FILE *old_stdout = stdout, *old_stderr = stderr;
    char **cmd_argv;
    char bound[PDTUTIL_MAX_STR_LEN + 1];

    /* blif2bench */
    len = strlen("blif2bench_") + strlen(pid_s) + strlen(".bench") + 1;
    benchFileName = Pdtutil_Alloc(char,
      len
    );

    snprintf(benchFileName, len, "%s%s.bench", "blif2bench_", pid_s);
    snprintf(cmd, PDTUTIL_MAX_STR_LEN, "blif2bench %s > %s",
      fsmFileName, benchFileName);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "# Executing command: %s\n", cmd));
    //system(cmd);
    stdout = fopen(benchFileName, "w+");
    cmd_argv = Pdtutil_Alloc(char *,
      3
    );

    cmd_argv[0] = strdup("blif2bench");
    cmd_argv[1] = strdup(fsmFileName);
    cmd_argv[2] = NULL;
    // StQ 2010.07.07
    fprintf(stderr, "Error: Ichecker Call!\n");
    exit(1);
    //blif2bench_main(2, cmd_argv);
    Pdtutil_Free(cmd_argv[1]);
    Pdtutil_Free(cmd_argv[0]);
    Pdtutil_Free(cmd_argv);
    fclose(stdout);
    stdout = old_stdout;
    /* ichecker */
    len = strlen("ichecker_") + strlen(pid_s) + strlen(".chk") + 1;
    chkFileName = Pdtutil_Alloc(char,
      len
    );

    snprintf(chkFileName, len, "%s%s.chk", "ichecker_", pid_s);
    snprintf(cmd, PDTUTIL_MAX_STR_LEN, "ichecker -B %d %s %s 2>/dev/null",
      opt->pre.iCheckerBound, benchFileName, chkFileName);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "# Executing command: %s\n", cmd));
    //system(cmd);
    cmd_argv = Pdtutil_Alloc(char *,
      6
    );

    cmd_argv[0] = strdup("ichecker");
    cmd_argv[1] = strdup("-B");
    snprintf(bound, PDTUTIL_MAX_STR_LEN, "%d", opt->pre.iCheckerBound);
    cmd_argv[2] = strdup(bound);
    cmd_argv[3] = strdup(benchFileName);
    cmd_argv[4] = strdup(chkFileName);
    cmd_argv[5] = NULL;
    // StQ 2010.07.07
    fprintf(stderr, "Error: Ichecker Call!\n");
    exit(1);
    //ichecker_main(5, cmd_argv);
    Pdtutil_Free(cmd_argv[4]);
    Pdtutil_Free(cmd_argv[3]);
    Pdtutil_Free(cmd_argv[2]);
    Pdtutil_Free(cmd_argv[1]);
    Pdtutil_Free(cmd_argv[0]);
    Pdtutil_Free(cmd_argv);
    /* chk2blif */
    len = strlen("chk2blif_") + strlen(pid_s) + strlen(".blif") + 1;
    newFsmFileName = Pdtutil_Alloc(char,
      len
    );

    snprintf(newFsmFileName, len, "%s%s.blif", "chk2blif_", pid_s);
    len = strlen("chk2blif_") + strlen(pid_s) + strlen(".trans") + 1;
    transFileName = Pdtutil_Alloc(char,
      len
    );

    snprintf(transFileName, len, "%s%s.trans", "chk2blif_", pid_s);
    snprintf(cmd, PDTUTIL_MAX_STR_LEN, "chk2blif %s %s 1>%s 2>%s",
      fsmFileName, chkFileName, newFsmFileName, transFileName);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "# Executing command: %s\n", cmd));
    //system(cmd);
    stdout = fopen(newFsmFileName, "w+");
    stderr = fopen(transFileName, "w+");
    cmd_argv = Pdtutil_Alloc(char *,
      4
    );

    cmd_argv[0] = strdup("chk2blif");
    cmd_argv[1] = strdup(fsmFileName);
    cmd_argv[2] = strdup(chkFileName);
    cmd_argv[3] = NULL;
    // StQ 2010.07.07
    fprintf(stderr, "Error: Ichecker Call!\n");
    exit(1);
    //chk2blif_main(3, cmd_argv);
    Pdtutil_Free(cmd_argv[2]);
    Pdtutil_Free(cmd_argv[1]);
    Pdtutil_Free(cmd_argv[0]);
    Pdtutil_Free(cmd_argv);
    fclose(stderr);
    stderr = old_stderr;
    fclose(stdout);
    stdout = old_stdout;
  }

  /*
   * ITP optimization mappings //DV
   */

  if (opt->ddi.itpMap > -1)     //-1 is the default value
  {
    switch (opt->ddi.itpMap) {
      case 0:                  //all disabled
        opt->ddi.itpOpt = 0;
        opt->ddi.itpMem = 0;
        break;
      case 1:                  //really base level
        opt->ddi.itpOpt = 0;
        opt->ddi.itpMem = 1;
        break;
      case 2:                  //start to be significant
        opt->ddi.itpOpt = 1;
        opt->ddi.itpMem = 3;
        break;
      case 3:                  //slightly more significant
        opt->ddi.itpOpt = 2;
        opt->ddi.itpMem = 3;
        break;
      case 4:                  //relevant
        opt->ddi.itpOpt = 3;
        opt->ddi.itpMem = 3;
        break;
      case 5:                  //more relevant
        opt->ddi.itpOpt = 3;
        opt->ddi.itpMem = 4;
        break;
      case 6:                  //maximum level of optimization
        opt->ddi.itpOpt = 5;
        opt->ddi.itpMem = 5;
        break;

      default:
        break;
    }
  }


if (opt->ddi.itpCompact > -1)     //-1 is the default value
  {
    switch (opt->ddi.itpCompact) {
      case 0:                  //all disabled
        opt->ddi.itpClust = 0;
	opt->ddi.itpNorm = 0;
	opt->ddi.itpSimp = 0;
        break;
      case 1:                  //non clust
        opt->ddi.itpClust = 0;
	opt->ddi.itpNorm = 1;
	opt->ddi.itpSimp = 1;
        break;
      case 2:                  //non norm
        opt->ddi.itpClust = 1;
	opt->ddi.itpNorm = 0;
	opt->ddi.itpSimp = 1;
         break;
      case 3:                  //non simp
        opt->ddi.itpClust = 1;
	opt->ddi.itpNorm = 1;
	opt->ddi.itpSimp = 0;
         break;
      case 4:                  //all enabled
        opt->ddi.itpClust = 1;
	opt->ddi.itpNorm = 1;
	opt->ddi.itpSimp = 1;
         break;   

    default:
        break;
    }
    opt->ddi.itpCompact = -1;
  }


  /*
   *  Custom behavior for combinational circuits
   */

  if (opt->mc.task != NULL) {
    FbvTask(opt);
  }
  else if (opt->mc.custom<0) {
    fbvCustom = -opt->mc.custom;
  }
  else if (opt->mc.custom) {
    Ddi_Mgr_t *ddiMgr;

    ddiMgr = Ddi_MgrInit("DDI_manager", NULL, 0, DDI_UNIQUE_SLOTS,
      DDI_CACHE_SLOTS * 10, 0, -1, -1);

    if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
      ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
      Ddi_MgrSetVerbosity(ddiMgr, Pdtutil_VerbLevel_e(opt->verbosity));
    }

    Ddi_AigInit(ddiMgr, 100);

    Ddi_MgrSiftEnable(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"));
    Ddi_MgrSetSiftThresh(ddiMgr, opt->ddi.siftTh);

    Ddi_MgrSetAigCnfLevel(ddiMgr, opt->ddi.aigCnfLevel);
    Ddi_MgrSetAigRedRemLevel(ddiMgr, opt->ddi.aigRedRem);
    Ddi_MgrSetAigRedRemMaxCnt(ddiMgr, opt->ddi.aigRedRemPeriod);
    Ddi_MgrSetAigAbcOptLevel(ddiMgr, opt->common.aigAbcOpt);

    Ddi_MgrSetAigItpAbc(ddiMgr, opt->ddi.itpAbc);
    Ddi_MgrSetAigItpCore(ddiMgr, opt->ddi.itpCore);
    Ddi_MgrSetAigItpMem(ddiMgr, opt->ddi.itpMem);
    Ddi_MgrSetAigItpOpt(ddiMgr, opt->ddi.itpOpt);
    Ddi_MgrSetAigItpStore(ddiMgr, opt->ddi.itpStore);
    Ddi_MgrSetAigItpPartTh(ddiMgr, opt->ddi.itpPartTh);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpActiveVars_c, inum, opt->ddi.itpActiveVars);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStoreTh_c, inum, opt->ddi.itpStoreTh);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpAigCore_c, inum, opt->ddi.itpAigCore);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpTwice_c, inum, opt->ddi.itpTwice);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpIteOptTh_c, inum, opt->ddi.itpIteOptTh);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStructOdcTh_c, inum,
      opt->ddi.itpStructOdcTh);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiNnfClustSimplify_c, inum,
      opt->ddi.nnfClustSimplify);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMap_c, inum, opt->ddi.itpMap);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompute_c, inum, opt->ddi.itpCompute);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpDrup_c, inum, opt->ddi.itpDrup);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompact_c, inum, opt->ddi.itpCompact);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpClust_c, inum, opt->ddi.itpClust);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpNorm_c, inum, opt->ddi.itpNorm);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpSimp_c, inum, opt->ddi.itpSimp);

    Ddi_AigCustomCombinationalCircuit(ddiMgr, fsmFileName, opt->mc.custom,
      opt->mc.compl_invarspec, opt->trav.cntReachedOK);
    Ddi_MgrQuit(ddiMgr);
    exit(8);
  }

  /*
   *  Combinational optimization
   */

  /*TODO add something*********************************/

  if (opt->ddi.itpCompact > -1) {
    Ddi_Mgr_t *ddiMgr;

    ddiMgr = Ddi_MgrInit("DDI_manager", NULL, 0, DDI_UNIQUE_SLOTS,
      DDI_CACHE_SLOTS * 10, 0, -1, -1);

    if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
      ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
      Ddi_MgrSetVerbosity(ddiMgr, Pdtutil_VerbLevel_e(opt->verbosity));
    }

    Ddi_AigInit(ddiMgr, 100);

    Ddi_MgrSetAigCnfLevel(ddiMgr, opt->ddi.aigCnfLevel);
    Ddi_MgrSetAigRedRemLevel(ddiMgr, opt->ddi.aigRedRem);
    Ddi_MgrSetAigRedRemMaxCnt(ddiMgr, opt->ddi.aigRedRemPeriod);
    Ddi_MgrSetAigAbcOptLevel(ddiMgr, opt->common.aigAbcOpt);

    Ddi_MgrSetAigItpAbc(ddiMgr, opt->ddi.itpAbc);
    Ddi_MgrSetAigItpCore(ddiMgr, opt->ddi.itpCore);
    Ddi_MgrSetAigItpMem(ddiMgr, opt->ddi.itpMem);
    Ddi_MgrSetAigItpOpt(ddiMgr, opt->ddi.itpOpt);
    Ddi_MgrSetAigItpStore(ddiMgr, opt->ddi.itpStore);
    Ddi_MgrSetAigItpPartTh(ddiMgr, opt->ddi.itpPartTh);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpActiveVars_c, inum, opt->ddi.itpActiveVars);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStoreTh_c, inum, opt->ddi.itpStoreTh);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpAigCore_c, inum, opt->ddi.itpAigCore);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpTwice_c, inum, opt->ddi.itpTwice);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpIteOptTh_c, inum, opt->ddi.itpIteOptTh);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStructOdcTh_c, inum,
      opt->ddi.itpStructOdcTh);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiNnfClustSimplify_c, inum,
      opt->ddi.nnfClustSimplify);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMap_c, inum, opt->ddi.itpMap);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompute_c, inum, opt->ddi.itpCompute);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpDrup_c, inum, opt->ddi.itpDrup);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompact_c, inum, opt->ddi.itpCompact);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpClust_c, inum, opt->ddi.itpClust);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpNorm_c, inum, opt->ddi.itpNorm);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpSimp_c, inum, opt->ddi.itpSimp);

    Ddi_AigSingleInterpolantLogicSynthesisCompaction(ddiMgr, fsmFileName);
    
    Ddi_MgrQuit(ddiMgr);
    exit(10);
  }

  /**********************************/
  

  /*
   *  Custom compacting ITP behaviour: load an itp or compute a new itp starting from A and B (previously stored) //DV
   */

  if (opt->ddi.itpCompute > 0) {

    int method = 0;
    Ddi_Mgr_t *ddiMgr;

    ddiMgr = Ddi_MgrInit("DDI_manager", NULL, 0, DDI_UNIQUE_SLOTS,
      DDI_CACHE_SLOTS * 10, 0, -1, -1);

    if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
      ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
      Ddi_MgrSetVerbosity(ddiMgr, Pdtutil_VerbLevel_e(opt->verbosity));
    }

    Ddi_AigInit(ddiMgr, 100);

    Ddi_MgrSetAigCnfLevel(ddiMgr, opt->ddi.aigCnfLevel);
    Ddi_MgrSetAigRedRemLevel(ddiMgr, opt->ddi.aigRedRem);
    Ddi_MgrSetAigRedRemMaxCnt(ddiMgr, opt->ddi.aigRedRemPeriod);
    Ddi_MgrSetAigAbcOptLevel(ddiMgr, opt->common.aigAbcOpt);

    Ddi_MgrSetAigItpAbc(ddiMgr, opt->ddi.itpAbc);
    Ddi_MgrSetAigItpCore(ddiMgr, opt->ddi.itpCore);
    Ddi_MgrSetAigItpMem(ddiMgr, opt->ddi.itpMem);
    Ddi_MgrSetAigItpOpt(ddiMgr, opt->ddi.itpOpt);
    Ddi_MgrSetAigItpStore(ddiMgr, opt->ddi.itpStore);
    Ddi_MgrSetAigItpPartTh(ddiMgr, opt->ddi.itpPartTh);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpActiveVars_c, inum, opt->ddi.itpActiveVars);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStoreTh_c, inum, opt->ddi.itpStoreTh);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpAigCore_c, inum, opt->ddi.itpAigCore);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpTwice_c, inum, opt->ddi.itpTwice);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpIteOptTh_c, inum, opt->ddi.itpIteOptTh);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStructOdcTh_c, inum,
      opt->ddi.itpStructOdcTh);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiNnfClustSimplify_c, inum,
      opt->ddi.nnfClustSimplify);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMap_c, inum, opt->ddi.itpMap);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompute_c, inum, opt->ddi.itpCompute);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpLoad_c, pchar, opt->ddi.itpLoad);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpDrup_c, inum, opt->ddi.itpDrup);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompact_c, inum, opt->ddi.itpCompact);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpClust_c, inum, opt->ddi.itpClust);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpNorm_c, inum, opt->ddi.itpNorm);
    Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpSimp_c, inum, opt->ddi.itpSimp);


    method = opt->ddi.itpCompute;

    Ddi_AigSingleInterpolantCompaction(ddiMgr, fsmFileName, opt->ddi.itpLoad,
      method);

    Ddi_MgrQuit(ddiMgr);
    exit(9);
  }


  /*
   *  Call verification
   */

  struct stat fsmOptStats;

  memset((void *)&fsmOptStats, 0, sizeof(struct stat));

  signal(SIGSEGV, Pdtutil_CatchExit);
  //  signal(SIGINT, Pdtutil_CatchExit);
  signal(SIGFPE, Pdtutil_CatchExit);
  signal(SIGABRT, Pdtutil_CatchExit);
  signal(SIGHUP, Pdtutil_CatchExit);
  signal(SIGKILL, Pdtutil_CatchExit);

  verifOK = -1;
  // opt->mc.strategy = Pdtutil_StrDup("mix");
  if (opt->mc.strategy != NULL &&
    (strcmp(opt->mc.strategy, "mix") == 0
      || strcmp(opt->mc.strategy, "expt") == 0)) {
    verifOK = invarMixedVerif(fsmFileName, opt->mc.ord, opt);
  } else if (opt->mc.strategy != NULL
    && strcmp(opt->mc.strategy, "decomp") == 0) {
    verifOK = invarDecompVerif(fsmFileName, opt->mc.ord, opt);
  } else if (opt->mc.strategy != NULL && strcmp(opt->mc.strategy, "thrd") == 0) {
    verifOK = FbvThrdVerif(fsmFileName, opt->mc.ord, opt);
  } else {

    CATCH {

      do {
        verifOK =
          invarVerif(opt->pre.iChecker ? newFsmFileName : fsmFileName,

                     opt->mc.ord, opt);
      } while (verifOK < 0);


    }
    FAIL {
      fprintf(stderr, "Caught SIGNAL: Verification aborted.\n");
      exit(1);
    }

  }

  if (verifOK < 0) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification did NOT complete.\n"));
  }

  /*
   * Temp files cleanup
   */

  if (opt->pre.iChecker) {
    /* tmp files cleanup */
    remove(newFsmFileName);
    Pdtutil_Free(newFsmFileName);
    remove(chkFileName);
    Pdtutil_Free(chkFileName);
    remove(transFileName);
    Pdtutil_Free(transFileName);
    remove(benchFileName);
    Pdtutil_Free(benchFileName);
  }

  opt->stats.fbvStopTime = util_cpu_time();

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Total Execution Time: %s\n",
      util_print_time(opt->stats.fbvStopTime - opt->stats.fbvStartTime))
    );

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "## PdTRAV End.\n")
    );

  if (opt->mc.strategy != NULL)
    Pdtutil_Free(opt->mc.strategy);
  Pdtutil_Free(opt);

  return (0);
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/

Fbv_Globals_t *
FbvParseArgs(
  int argc,
  char *argv[],
  Fbv_Globals_t * opt
)
{
  //Fbv_Globals_t *opt = new_settings();
  //int initialArgc = argc;
  int i;

  if (opt == NULL) {
    opt = new_settings();
  }
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "c ## Command: "); for (i = 0; i < argc; i++) {
    fprintf(stdout, "%s ", argv[i]);}
    fprintf(stdout, "\n")
    ) ;

  while (argc > 1 && argv[1][0] == '-') {
    if (strcmp(argv[1], "-h") == 0) {
      printHelp(stdout, argv[2]);
      dispose_settings(opt);
      return NULL;
    } else if (strcmp(argv[1], "-verbosity") == 0) {
      opt->verbosity = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-source") == 0) {
      FILE *fp = fopen(argv[2], "r");

      if (fp == NULL) {
        fprintf(stderr, "Error opening file %s as source\n", argv[2]);
      } else {
        char **localArgv;
        int localArgc, readArgc;

        localArgv = readArgs(fp, &localArgc);
        localArgv[0] = Pdtutil_Alloc(char,
          10 + strlen(argv[2])
        );

        sprintf(localArgv[0], "-source %s", argv[2]);
        //dispose_settings(opt);
        FbvParseArgs(localArgc, localArgv, opt);
        fclose(fp);
        if (0 && readArgc < localArgc) {
          fprintf(stderr, "Not all args read from %s. ", argv[2]);
          fprintf(stderr, "Reading stopped at \"%s\"\n", localArgv[readArgc]);
        }
      }
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-clk") == 0) {
      opt->common.clk = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-lazyRate") == 0) {
      opt->common.lazyRate = atof(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-frozen") == 0) {
      opt->common.checkFrozen = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-metaMaxSmooth") == 0) {
      opt->common.metaMaxSmooth = atoi(argv[2]);
      opt->trav.guidedTrav = 1;
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-sat") == 0) {
      strcpy(opt->common.satSolver, argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-aigAbcOpt") == 0) {
      opt->common.aigAbcOpt = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-sift") == 0) {
      opt->ddi.siftTh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-siftMaxTh") == 0) {
      opt->ddi.siftMaxTh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-siftMaxCalls") == 0) {
      opt->ddi.siftMaxCalls = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-existClipTh") == 0) {
      opt->ddi.existClipTh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-imgClip") == 0) {
      opt->ddi.imgClipTh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-exist_opt_level") == 0) {
      opt->ddi.exist_opt_level = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-exist_opt_thresh") == 0) {
      opt->ddi.exist_opt_thresh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-dynAbstrStrategy") == 0) {
      opt->ddi.dynAbstrStrategy = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-ternaryAbstrStrategy") == 0) {
      opt->ddi.ternaryAbstrStrategy = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpAbc") == 0) {
      opt->ddi.itpAbc = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpCore") == 0) {
      opt->ddi.itpCore = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpMem") == 0) {
      opt->ddi.itpMem = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpOpt") == 0) {
      opt->ddi.itpOpt = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpStore") == 0) {
      opt->ddi.itpStore = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpStoreTh") == 0) {
      opt->ddi.itpStoreTh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpPartTh") == 0) {
      opt->ddi.itpPartTh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpPartCone") == 0) {
      opt->trav.itpPartCone = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpAigCore") == 0) {
      opt->ddi.itpAigCore = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpTwice") == 0) {
      opt->ddi.itpTwice = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpIteOptTh") == 0) {
      opt->ddi.itpIteOptTh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpStructOdcTh") == 0) {
      opt->ddi.itpStructOdcTh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-nnfClustSimplify") == 0) {
      opt->ddi.nnfClustSimplify = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpMap") == 0) {
      opt->ddi.itpMap = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpCompact") == 0) {
      opt->ddi.itpCompact = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpLoad") == 0) {
      opt->ddi.itpLoad = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpDrup") == 0) {
      opt->ddi.itpDrup = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpCompute") == 0) {
      opt->ddi.itpCompute = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-aigBddOpt") == 0) {
      opt->ddi.aigBddOpt = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-aigRedRem") == 0) {
      opt->ddi.aigRedRem = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-aigRedRemPeriod") == 0) {
      opt->ddi.aigRedRemPeriod = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-aigCnfLevel") == 0) {
      opt->ddi.aigCnfLevel = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-satNoIncr") == 0) {
      opt->ddi.satIncremental = 0;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-satTimeout") == 0) {
      opt->ddi.satTimeout = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-satTimeLimit") == 0) {
      opt->ddi.satTimeLimit = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-satVarActivity") == 0) {
      opt->ddi.satVarActivity = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-bin_clauses_opt") == 0) {
      opt->ddi.satBinClauses = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-nnf") == 0) {
      opt->fsm.nnf = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-cut") == 0) {
      opt->fsm.cut = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-insertCutLatches") == 0) {
      opt->fsm.insertCutLatches = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-cutAtAuxVar") == 0) {
      opt->fsm.cutAtAuxVar = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-cegarStrategy") == 0) {
      opt->fsm.cegarStrategy = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-manualAbstr") == 0) {
      opt->fsm.manualAbstr = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-td") == 0) {
      opt->pre.targetDecomp = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-se") == 0) {
      opt->pre.specEn = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-specConj") == 0) {
      opt->pre.specConj = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-specDecomp") == 0) {
      sscanf(argv[2], "%d/%d", &opt->pre.specDecompIndex,
        &opt->pre.specDecomp);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-specDecompTot") == 0) {
      sscanf(argv[2], "%d", &opt->pre.specDecompTot);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-specDecompForce") == 0) {
      sscanf(argv[2], "%d", &opt->pre.specDecompForce);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-specDecompConstrTh") == 0) {
      sscanf(argv[2], "%d", &opt->pre.specDecompConstrTh);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-specDecompMin") == 0) {
      sscanf(argv[2], "%d", &opt->pre.specDecompMin);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-specDecompMax") == 0) {
      sscanf(argv[2], "%d", &opt->pre.specDecompMax);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-specDecompEq") == 0) {
      sscanf(argv[2], "%d", &opt->pre.specDecompEq);
      opt->pre.specDecompSort = 1;
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-specDecompSat") == 0) {
      sscanf(argv[2], "%d", &opt->pre.specDecompSat);
      opt->pre.specDecompSort = 1;
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-specDecompCore") == 0) {
      sscanf(argv[2], "%d", &opt->pre.specDecompCore);
      opt->pre.specDecompSort = 1;
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-specSubsetByAntecedents") == 0) {
      sscanf(argv[2], "%d", &opt->pre.specSubsetByAntecedents);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-specDecompSort") == 0) {
      sscanf(argv[2], "%d", &opt->pre.specDecompSort);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-speculateEqProp") == 0) {
      opt->pre.speculateEqProp = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-nocoi") == 0) {
      opt->pre.doCoi = 0;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-cois") == 0) {
      opt->pre.doCoi = 2;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-coi") == 0) {
      opt->pre.doCoi = 2;       // defaults to -cois
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-sccCoi") == 0) {
      opt->pre.sccCoi = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-coisort") == 0) {
      opt->pre.coisort = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-constrCoiLimit") == 0) {
      opt->pre.constrCoiLimit = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-noConstr") == 0) {
      opt->pre.constrCoiLimit = 0;
      opt->pre.noConstr = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-fullConstr") == 0) {
      opt->pre.constrCoiLimit = -1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-constrIsNeg") == 0) {
      opt->pre.constrIsNeg = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-clustThDiff") == 0) {
      opt->pre.clustThDiff = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-clustThShare") == 0) {
      opt->pre.clustThShare = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-thresholdCluster") == 0) {
      opt->pre.thresholdCluster = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-methodCluster") == 0) {
      opt->pre.methodCluster = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-createClusterFile") == 0) {
      opt->pre.createClusterFile = 1;
      opt->pre.clusterFile = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-wrCoi") == 0) {
      opt->pre.wrCoi = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-performAbcOpt") == 0) {
      opt->pre.performAbcOpt = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-iChecker") == 0) {
      opt->pre.iChecker = 1;
      opt->pre.iCheckerBound = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-lambdaLatches") == 0) {
      opt->pre.lambdaLatches = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-peripheralLatches") == 0) {
      opt->pre.peripheralLatches = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-noPeripheralLatches") == 0) {
      opt->pre.peripheralLatches = 0;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-forceInitStub") == 0) {
      opt->pre.forceInitStub = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-retime") == 0) {
      opt->pre.retime = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-terminalScc") == 0) {
      opt->pre.terminalScc = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-findClk") == 0) {
      opt->pre.findClk = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-twoPhase") == 0) {
      opt->pre.findClk = 1;
      opt->pre.twoPhaseRed = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-twoPhaseForced") == 0) {
      opt->pre.findClk = 1;
      opt->pre.twoPhaseForced = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-twoPhasePlus") == 0) {
      opt->pre.findClk = 1;
      opt->pre.twoPhaseRed = 1;
      opt->pre.twoPhaseRedPlus = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-twoPhaseOther") == 0) {
      opt->pre.twoPhaseOther = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-impliedConstr") == 0) {
      opt->pre.impliedConstr = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-reduceCustom") == 0) {
      opt->pre.reduceCustom = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-ternarySim") == 0) {
      opt->pre.ternarySim = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-noTernary") == 0) {
      opt->pre.noTernary = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-indEqDepth") == 0) {
      opt->pre.indEqDepth = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-indEqAssumeMiters") == 0) {
      opt->pre.indEqAssumeMiters = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-indEqMaxLevel") == 0) {
      opt->pre.indEqMaxLevel = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-initMinterm") == 0) {
      opt->pre.initMinterm = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-coiLimit") == 0) {
      opt->tr.coiLimit = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-trSort") == 0) {
      strcpy(opt->tr.trSort, argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-squaringMaxIter") == 0) {
      opt->tr.squaringMaxIter = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-invar_in_tr") == 0) {
      opt->tr.invar_in_tr = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-approxImgMin") == 0) {
      opt->tr.approxImgMin = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-approxImgMax") == 0) {
      opt->tr.approxImgMax = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-approxImgIter") == 0) {
      opt->tr.approxImgIter = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-en") == 0) {
      opt->tr.en = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-manualTrDpartFile") == 0) {
      opt->tr.manualTrDpartFile = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-cex") == 0) {
      opt->trav.cex = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-sort_for_bck") == 0) {
      opt->trav.sort_for_bck = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-checkProof") == 0) {
      opt->trav.checkProof = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-writeProof") == 0) {
      opt->trav.writeProof = Pdtutil_StrDup(argv[2]);
      opt->trav.pdrReuseRings = 1;
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-writeInvar") == 0) {
      opt->trav.writeInvar = Pdtutil_StrDup(argv[2]);
      opt->pre.peripheralLatches = 0;
      opt->trav.pdrReuseRings = 1;
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-trPreimgSort") == 0) {
      opt->trav.trPreimgSort = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-clust") == 0) {
      opt->trav.clustTh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-apprClust") == 0) {
      opt->trav.apprClustTh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-imgClust") == 0) {
      opt->trav.imgClustTh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-squaring") == 0) {
      opt->trav.squaring = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-trproj") == 0) {
      opt->trav.trproj = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-trDpart") == 0) {
      opt->trav.trDpartTh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-trDpartVar") == 0) {
      opt->trav.trDpartVar = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-auxVarFile") == 0) {
      opt->trav.auxVarFile = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-approxClustFile") == 0) {
      opt->trav.approxTrClustFile = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-bwdClustFile") == 0) {
      opt->trav.bwdTrClustFile = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-lazyTimeLimit") == 0) {
      opt->trav.lazyTimeLimit = atof(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-travSelfTuning") == 0) {
      opt->trav.travSelfTuning = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-travConstrLevel") == 0) {
      opt->trav.travConstrLevel = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-mism_opt_level") == 0) {
      opt->trav.mism_opt_level = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-te") == 0) {
      opt->trav.targetEn = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-teAppr") == 0) {
      opt->trav.targetEnAppr = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-noInit") == 0) {
      opt->trav.noInit = 1;
      opt->trav.maxFwdIter = 0;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-fromsel") == 0) {
      opt->trav.fromSel = argv[2][0];
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-opt_img") == 0) {
      opt->trav.opt_img = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-opt_preimg") == 0) {
      opt->trav.opt_preimg = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-noCheck") == 0) {
      opt->trav.noCheck = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pmc") == 0) {
      opt->trav.prioritizedMc = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pmcNoSame") == 0) {
      opt->trav.pmcNoSame = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pmcMF") == 0) {
      opt->trav.pmcMaxNestedFull = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pmcMP") == 0) {
      opt->trav.pmcMaxNestedPart = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pmcPT") == 0) {
      opt->trav.pmcPrioThresh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-countR") == 0) {
      opt->trav.countReached = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-autoHint") == 0) {
      opt->trav.autoHint = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-hintsTh") == 0) {
      opt->trav.hintsTh = atoi(argv[2]);
      opt->trav.igrMaxIter = 1000000;
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-hintsStrategy") == 0) {
      opt->trav.hintsStrategy = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-hintsFile") == 0) {
      opt->trav.hintsFile = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-checkInv") == 0) {
      opt->mc.checkInv = Pdtutil_StrDup(argv[2]);
      opt->pre.peripheralLatches = 0;
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-gfp") == 0) {
      opt->mc.gfp = atoi(argv[2]);
      opt->pre.peripheralLatches = 0;
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-certify") == 0) {
      opt->mc.checkInv = Pdtutil_StrDup(argv[2]);
      opt->pre.peripheralLatches = 0;
      opt->mc.exit_after_checkInv = 1;
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-meta") == 0) {
      if (strcmp(argv[2], "sch") == 0) {
        argv++;
        argc--;
        opt->trav.metaMethod = Ddi_Meta_EarlySched;
      }
      opt->trav.fbvMeta = atoi(argv[2]);
      opt->trav.guidedTrav = 1;
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-constrain") == 0) {
      opt->trav.constrain = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-cl") == 0) {
      opt->trav.constrain_level = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-approxNP") == 0) {
      opt->trav.approxNP = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-mbm") == 0) {
      opt->trav.approxMBM = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-lmbm") == 0) {
      opt->trav.approxMBM = 2;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-apprPrTh") == 0) {
      opt->trav.approxPreimgTh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-apprOverlap") == 0) {
      opt->trav.apprOverlap = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-appr") == 0) {
      opt->trav.apprGrTh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-maxFwdIter") == 0) {
      opt->trav.maxFwdIter = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-nExactIter") == 0) {
      opt->trav.nExactIter = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-implications") == 0) {
      opt->trav.implications = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-imgDpart") == 0) {
      opt->trav.imgDpartTh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-imgDpartSat") == 0) {
      opt->trav.imgDpartSat = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-bound") == 0) {
      opt->trav.bound = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-strict") == 0) {
      opt->trav.strictBound = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-uq") == 0) {
      opt->trav.univQuantifyTh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-rp") == 0) {
      opt->trav.rPlus = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-certify") == 0) {
      opt->trav.rPlus = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpStoreRings") == 0) {
      opt->trav.itpStoreRings = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-rpRings") == 0) {
      opt->trav.rPlusRings = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-wr") == 0) {
      opt->trav.wR = Pdtutil_StrDup(argv[2]);
      opt->trav.pdrReuseRings = 1;
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-wc") == 0) {
      opt->trav.wC = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-wu") == 0) {
      opt->trav.wU = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-wp") == 0) {
      opt->trav.wP = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-wbr") == 0) {
      opt->trav.wBR = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-wo") == 0) {
      opt->trav.wOrd = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-storeCNF") == 0) {
      opt->trav.storeCNF = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-storeCNFTR") == 0) {
      opt->trav.storeCNFTR = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-storeCNFmode") == 0) {
      opt->trav.storeCNFmode = argv[2][0];
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-storeCNFphase") == 0) {
      opt->trav.storeCNFphase = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-maxCNFLength") == 0) {
      opt->trav.maxCNFLength = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-apprAig") == 0) {
      opt->trav.apprAig = 1;
      opt->mc.aig = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-abstrRefLoad") == 0) {
      opt->trav.abstrRefLoad = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-abstrRefStore") == 0) {
      opt->trav.abstrRefStore = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-abstrRef") == 0) {
      opt->trav.abstrRef = atoi(argv[2]);
      if (opt->trav.abstrRefGla>0) {
	// enable using both
	if (opt->trav.abstrRefGla<100) {
	  opt->trav.abstrRefGla += 100;
	}
      }
      if (opt->trav.abstrRef>0 && opt->trav.itpConstrLevel > 1)
        opt->trav.itpConstrLevel = 1;
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-abstrRefGla") == 0) {
      opt->trav.abstrRefGla = atoi(argv[2]);
      if (opt->trav.abstrRef>0) {
	// enable using both
	opt->trav.abstrRefGla += 100;
      }
      else {
	opt->trav.abstrRef = 2;
      }
      if (opt->trav.itpConstrLevel > 1)
        opt->trav.itpConstrLevel = 1;
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-abstrRefItp") == 0) {
      opt->trav.abstrRefItp = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-abstrRefItpMaxIter") == 0) {
      opt->trav.abstrRefItpMaxIter = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-trAbstrItp") == 0) {
      opt->trav.trAbstrItp = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-trAbstrItpOpt") == 0) {
      opt->trav.trAbstrItpOpt = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-trAbstrItpFirstFwdStep") == 0) {
      opt->trav.trAbstrItpFirstFwdStep = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-trAbstrItpMaxFwdStep") == 0) {
      opt->trav.trAbstrItpMaxFwdStep = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-trAbstrItpLoad") == 0) {
      opt->trav.trAbstrItpLoad = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-trAbstrItpStore") == 0) {
      opt->trav.trAbstrItpStore = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-dynAbstr") == 0) {
      opt->trav.dynAbstr = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-dynAbstrInitIter") == 0) {
      opt->trav.dynAbstrInitIter = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-implAbstr") == 0) {
      opt->trav.implAbstr = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-ternaryAbstr") == 0) {
      opt->trav.ternaryAbstr = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-inputRegs") == 0) {
      opt->trav.inputRegs = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-bmcTr") == 0) {
      opt->mc.aig = 1;
      opt->trav.bmcTr = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-bmcStep") == 0) {
      opt->trav.bmcStep = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-bmcFirst") == 0) {
      opt->trav.bmcFirst = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-bmcTe") == 0) {
      opt->trav.bmcTe = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-bmcItpRingsPeriod") == 0) {
      opt->trav.bmcItpRingsPeriod = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-bmcTrAbstrPeriod") == 0) {
      opt->trav.bmcTrAbstrPeriod = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-bmcTrAbstrInit") == 0) {
      opt->trav.bmcTrAbstrInit = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-bmcLearnStep") == 0) {
      opt->trav.bmcLearnStep = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-bmcStrategy") == 0) {
      opt->trav.bmcStrategy = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-interpolantBmcSteps") == 0) {
      opt->trav.interpolantBmcSteps = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pdrReuseRings") == 0) {
      opt->trav.pdrReuseRings = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pdrMaxBlock") == 0) {
      opt->trav.pdrMaxBlock = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pdrShareReached") == 0) {
      opt->trav.pdrShareReached = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pdrUsePgEncoding") == 0) {
      opt->trav.pdrUsePgEncoding = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pdrBumpActTopologically") == 0) {
      opt->trav.pdrBumpActTopologically = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pdrSpecializedCallsMask") == 0) {
      opt->trav.pdrSpecializedCallsMask = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pdrRestartStrategy") == 0) {
      opt->trav.pdrRestartStrategy = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pdrFwdEq") == 0) {
      opt->trav.pdrFwdEq = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pdrGenEffort") == 0) {
      opt->trav.pdrGenEffort = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pdrIncrementalTr") == 0) {
      opt->trav.pdrIncrementalTr = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pdrInf") == 0) {
      opt->trav.pdrInf = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pdrUnfold") == 0) {
      opt->trav.pdrUnfold = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pdrIndAssumeLits") == 0) {
      opt->trav.pdrIndAssumeLits = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pdrPostordCnf") == 0) {
      opt->trav.pdrPostordCnf = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pdrCexJustify") == 0) {
      opt->trav.pdrCexJustify = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpBdd") == 0) {
      opt->trav.itpBdd = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpPart") == 0) {
      opt->trav.itpPart = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpActiveVars") == 0) {
      opt->ddi.itpActiveVars = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpAppr") == 0) {
      opt->trav.itpAppr = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpExact") == 0) {
      opt->trav.itpExact = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpNew") == 0) {
      opt->trav.itpNew = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpForceRun") == 0) {
      opt->trav.itpForceRun = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpMaxStepK") == 0) {
      opt->trav.itpMaxStepK = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpCheckCompleteness") == 0) {
      opt->trav.itpCheckCompleteness = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpGfp") == 0) {
      opt->trav.itpGfp = atoi(argv[2]);
      opt->trav.itpConstrLevel = 4;
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpConstrLevel") == 0) {
      opt->trav.itpConstrLevel = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpShareReached") == 0) {
      opt->trav.itpShareReached = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpWeaken") == 0) {
      opt->trav.itpWeaken = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpStrengthen") == 0) {
      opt->trav.itpStrengthen = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpReImg") == 0) {
      opt->trav.itpReImg = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpFromNewLevel") == 0) {
      opt->trav.itpFromNewLevel = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpGenMaxIter") == 0) {
      opt->trav.itpGenMaxIter = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpEnToPlusImg") == 0) {
      opt->trav.itpEnToPlusImg = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpTuneForDepth") == 0) {
      opt->trav.itpTuneForDepth = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpStructAbstr") == 0) {
      opt->trav.itpStructAbstr = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpInitAbstr") == 0) {
      opt->trav.itpInitAbstr = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpEndAbstr") == 0) {
      opt->trav.itpEndAbstr = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpTrAbstr") == 0) {
      opt->trav.itpTrAbstr = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpExactBoundDouble") == 0) {
      opt->trav.itpExactBoundDouble = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpBoundkOpt") == 0) {
      opt->trav.itpBoundkOpt = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpUseReached") == 0) {
      opt->trav.itpUseReached = atoi(argv[2]);
      opt->trav.fromSel = 'r';
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpConeOpt") == 0) {
      opt->trav.itpConeOpt = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpInnerCones") == 0) {
      opt->trav.itpInnerCones = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpReuseRings") == 0) {
      opt->trav.itpReuseRings = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpInductiveTo") == 0) {
      opt->trav.itpInductiveTo = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpRefineCex") == 0) {
      opt->trav.itpRefineCex = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpUsePdrReached") == 0) {
      opt->trav.itpUsePdrReached = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpRpm") == 0) {
      opt->trav.itpRpm = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrItpSeq") == 0) {
      opt->trav.igrItpSeq = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrSide") == 0) {
      opt->trav.igrSide = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrBwdRefine") == 0) {
      opt->trav.igrBwdRefine = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrRestart") == 0) {
      opt->trav.igrRestart = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrRewind") == 0) {
      opt->trav.igrRewind = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrRewindMinK") == 0) {
      opt->trav.igrRewindMinK = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrUseRings") == 0) {
      opt->trav.igrUseRings = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrUseRingsStep") == 0) {
      opt->trav.igrUseRingsStep = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrUseBwdRings") == 0) {
      opt->trav.igrUseBwdRings = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrAssumeSafeBound") == 0) {
      opt->trav.igrAssumeSafeBound = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrConeSubsetBound") == 0) {
      opt->trav.igrConeSubsetBound = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrConeSubsetSizeTh") == 0) {
      opt->trav.igrConeSubsetSizeTh = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrConeSubsetPiRatio") == 0) {
      opt->trav.igrConeSubsetPiRatio = atof(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrConeSplitRatio") == 0) {
      opt->trav.igrConeSplitRatio = atof(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrGrowCone") == 0) {
      opt->trav.igrGrowCone = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrGrowConeMaxK") == 0) {
      opt->trav.igrGrowConeMaxK = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrGrowConeMax") == 0) {
      opt->trav.igrGrowConeMax = atof(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrMaxIter") == 0) {
      opt->trav.igrMaxIter = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrMaxExact") == 0) {
      opt->trav.igrMaxExact = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-igrFwdBwd") == 0) {
      opt->trav.igrFwdBwd = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-strongLemmas") == 0) {
      opt->trav.strongLemmas = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-lemmasCare") == 0) {
      opt->trav.lemmasCare = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-implLemmas") == 0) {
      opt->trav.implLemmas = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-lemmasTimeLimit") == 0) {
      opt->trav.lemmasTimeLimit = atof(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-printNtkStats") == 0) {
      opt->mc.method = Mc_VerifPrintNtkStats_c;
      opt->trav.pmcNoSame = 1;
      opt->trav.noCheck = 1;
      opt->mc.aig = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-f") == 0) {
      opt->mc.fwdBwdFP = 1;
      opt->mc.method = Mc_VerifExactFwd_c;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-fe") == 0) {
      opt->mc.method = Mc_VerifExactFwd_c;
      opt->mc.exactTrav = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-fg") == 0) {
      opt->mc.method = Mc_VerifExactFwd_c;
      opt->trav.guidedTrav = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-b") == 0) {
      opt->mc.fwdBwdFP = -1;
      opt->mc.method = Mc_VerifExactBck_c;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-fb") == 0) {
      opt->mc.fwdBwdFP = 0;
      opt->mc.method = Mc_VerifApproxFwdExactBck_c;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-bf") == 0) {
      opt->mc.method = Mc_VerifApproxBckExactFwd_c;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-fbf") == 0) {
      opt->mc.method = Mc_VerifApproxFwdApproxBckExactFwd_c;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-t") == 0) {
      opt->mc.method = Mc_VerifTravFwd_c;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-bt") == 0) {
      opt->mc.method = Mc_VerifTravBwd_c;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-tg") == 0) {
      opt->mc.method = Mc_VerifTravFwd_c;
      opt->trav.guidedTrav = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-invarfile") == 0) {
      opt->trav.invarFile = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-invarconstr") == 0) {
      opt->trav.invarFile = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-abc") == 0) {
      opt->mc.method = Mc_VerifABCLink_c;
      opt->mc.abcOptFilename = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-aig") == 0) {
      opt->mc.aig = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-aig2") == 0) {
      opt->mc.aig = 2;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-aig0") == 0) {
      opt->ddi.aigPartial = 1;
      opt->mc.aig = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-bmc") == 0) {
      opt->mc.aig = 1;
      opt->mc.bmc = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-inductive") == 0) {
      opt->mc.aig = 1;
      opt->mc.inductive = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-interpolant") == 0) {
      opt->mc.aig = 1;
      opt->mc.interpolant = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-lazy") == 0) {
      opt->mc.aig = 1;
      opt->mc.lazy = 1;
      // opt->mc.tuneForEqCheck = 1;
      if (*argv[2] != '-') {
        opt->common.lazyRate = atof(argv[2]);
        argv++;
        argc--;
      }
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-custom") == 0) {
      opt->mc.aig = 1;
      opt->mc.custom = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-lemmas") == 0) {
      opt->mc.lemmas = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-qbf") == 0) {
      opt->mc.aig = 1;
      opt->mc.qbf = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-diameter") == 0) {
      opt->mc.aig = 1;
      opt->mc.diameter = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-nogroup") == 0) {
      opt->mc.nogroup = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-enFastRun") == 0) {
      opt->mc.enFastRun = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-tuneForEq") == 0) {
      opt->mc.tuneForEqCheck = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-initBound") == 0) {
      opt->mc.initBound = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpSeq") == 0) {
      opt->mc.itpSeq = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpSeqGroup") == 0) {
      opt->mc.itpSeqGroup = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpSeqSerial") == 0) {
      opt->mc.itpSeqSerial = atof(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-decompTimeLimit") == 0) {
      opt->mc.decompTimeLimit = atof(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-idelta") == 0) {
      opt->mc.idelta = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-ilambda") == 0) {
      opt->mc.ilambda = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-ilambdaCare") == 0) {
      opt->mc.ilambdaCare = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-nlambda") == 0) {
      opt->mc.nlambda = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-ninvar") == 0) {
      opt->mc.ninvar = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-range") == 0) {
      opt->mc.range = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-all1") == 0) {
      opt->mc.all1 = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-ni") == 0) {
      opt->mc.compl_invarspec = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-pi") == 0) {
      opt->mc.compl_invarspec = 0;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-invarspec") == 0) {
      opt->mc.invSpec = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-ctlspec") == 0) {
      opt->mc.ctlSpec = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-rInit") == 0) {
      opt->mc.rInit = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-ord") == 0) {
      opt->mc.ord = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-wres") == 0) {
      opt->mc.wRes = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-wfsm") == 0) {
      opt->mc.wFsm = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-wfsmFoldInit") == 0) {
      opt->mc.foldInit = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-wfsmFolded") == 0) {
      opt->mc.wFsm = Pdtutil_StrDup(argv[2]);
      opt->mc.fold = 1;
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-cegar") == 0) {
      opt->mc.cegar = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-cegarPba") == 0) {
      opt->mc.cegar = atoi(argv[2]);
      opt->mc.cegarPba = 1;
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-expertLevel") == 0) {
      opt->expt.expertLevel = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-expertForce") == 0) {
      argv++;
      argc--;
      opt->expt.expertArgv = argv;
      opt->expt.expertArgc = argc;
    } else if (strcmp(argv[1], "-itpTimeLimit") == 0) {
      opt->expt.itpTimeLimit = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-totMemoryLimit") == 0) {
      opt->expt.totMemoryLimit = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-thrdEnPfl") == 0) {
      opt->expt.enPfl = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-thrdEnItpPlus") == 0) {
      opt->expt.enItpPlus = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-itpPeakAig") == 0) {
      opt->expt.itpPeakAig = atoi(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-mult") == 0) {
      extern int doReadMult;

      doReadMult = 1;
      opt->mc.checkMult = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-dead") == 0) {
      opt->mc.checkDead = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-thrdDis") == 0) {
      if (opt->expt.thrdDisNum < MAXTHRD) {
        opt->expt.thrdDis[opt->expt.thrdDisNum++] = Pdtutil_StrDup(argv[2]);
      }
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-thrdEn") == 0) {
      if (opt->expt.thrdEnNum < MAXTHRD) {
        opt->expt.thrdEn[opt->expt.thrdEnNum++] = Pdtutil_StrDup(argv[2]);
      }
      argv++;
      argc--;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-hwmccSetup") == 0) {
      hwmccOut = 1;
      opt->ddi.itpDrup = 1;
      argv++;
      argc--;
    } else if (strcmp(argv[1], "-task") == 0) {
      opt->mc.task = argv+2;
      argc=0;
    } else if (strcmp(argv[1], "-strategy") == 0
      || strcmp(argv[1], "-engine") == 0) {
      int j;

      opt->mc.strategy = Pdtutil_StrDup(argv[2]);
      argv++;
      argc--;
      argv++;
      argc--;

      opt->mc.compl_invarspec = 1;
      opt->pre.doCoi = 2;
      // opt->pre.doCoi = 0;

      opt->ddi.siftTh = 100000;
      /* competition specific settings */
      if (strcmp(opt->mc.strategy, "call") == 0) {
        opt->mc.writeBound = 0;
        opt->mc.useChildCall = 1;
        opt->mc.strategy = Pdtutil_StrDup("thrd");
      } else if (strcmp(opt->mc.strategy, "igrhwmcc") == 0) {
        opt->mc.writeBound = 0;
        opt->mc.useChildCall = 1;
        opt->mc.strategy = Pdtutil_StrDup("thrd");
	if (opt->expt.thrdEnNum < MAXTHRD) {
	  opt->expt.thrdEn[opt->expt.thrdEnNum++] = Pdtutil_StrDup("IGR");
	}
      } else if (strcmp(opt->mc.strategy, "itphwmcc") == 0) {
        opt->mc.writeBound = 0;
        opt->mc.useChildCall = 1;
        opt->mc.strategy = Pdtutil_StrDup("thrd");
	if (opt->expt.thrdEnNum < MAXTHRD) {
	  opt->expt.thrdEn[opt->expt.thrdEnNum++] = Pdtutil_StrDup("ITP");
	}
      } else if (strcmp(opt->mc.strategy, "bddhwmcc") == 0) {
        opt->mc.writeBound = 0;
        opt->mc.useChildCall = 1;
        opt->mc.strategy = Pdtutil_StrDup("thrd");
	if (opt->expt.thrdEnNum < MAXTHRD) {
	  opt->expt.thrdEn[opt->expt.thrdEnNum++] = Pdtutil_StrDup("BDD");
	}
      } else if (strcmp(opt->mc.strategy, "deep") == 0) {
        opt->mc.strategy = Pdtutil_StrDup("thrd");
	opt->expt.deepBound = 1;
	opt->expt.totMemoryLimit = 90000;
        opt->mc.writeBound = 1;
        opt->mc.useChildCall = 0;
        opt->expt.enPfl = 1;
	if (opt->expt.thrdEnNum < MAXTHRD) {
	  opt->expt.thrdEn[opt->expt.thrdEnNum++] = Pdtutil_StrDup("BMCPFL");
	}
      } else if (strcmp(opt->mc.strategy, "thrd") == 0) {
        opt->mc.writeBound = 0;
        opt->mc.useChildCall = 0;
	opt->expt.enPfl = 1;
      } else if (strcmp(opt->mc.strategy, "thrdh") == 0) {
        opt->mc.strategy = Pdtutil_StrDup("thrd");
        opt->mc.writeBound = 0;
        opt->mc.useChildCall = 0;
        opt->expt.enPfl = 1;
      } else if (strcmp(opt->mc.strategy, "thrd0") == 0) {
        opt->mc.strategy = Pdtutil_StrDup("thrd");
        opt->mc.writeBound = 0;
        opt->mc.useChildCall = 0;
      } else if (strcmp(opt->mc.strategy, "decomp") == 0) {
        opt->expt.expertLevel = 2;
      } else if (strcmp(opt->mc.strategy, "proc") == 0) {
        opt->mc.writeBound = 0;
        opt->mc.useChildCall = 0;
        opt->mc.useChildProc = 1;
        opt->mc.strategy = Pdtutil_StrDup("thrd");
      } else if ((strcmp(opt->mc.strategy, "multi") == 0) ||
        (strcmp(opt->mc.strategy, "multijoin") == 0)) {
        opt->mc.ilambda = -1;
        opt->expt.expertLevel = 2;
        if (strcmp(opt->mc.strategy, "multi") == 0) {
          opt->pre.specDecompForce = 1;
        } else {
          opt->pre.specDecompForce = 2;
        }
        Pdtutil_Free(opt->mc.strategy);
        opt->mc.strategy = Pdtutil_StrDup("decomp");

        opt->mc.aig = 1;
        opt->mc.tuneForEqCheck = 0;
        // opt->trav.dynAbstr = 1;
        // opt->trav.implAbstr = 0;
        // opt->trav.itpPart = 1;
        // opt->mc.lemmas = 0; //2;
        // opt->trav.lemmasTimeLimit = 60;
        opt->pre.peripheralLatches = 1;
        // opt->pre.performAbcOpt = 0;
        opt->pre.abcSeqOpt = 0;
        opt->ddi.aigRedRemPeriod = 100;
        opt->mc.lazy = 1;
        opt->common.lazyRate = 1.0;
        // opt->trav.dynAbstr = 2;
        // opt->trav.itpAppr = 1;
        // opt->trav.implAbstr = 3;
        // opt->trav.ternaryAbstr = 0;
        // opt->trav.itpEnToPlusImg = 0;

        // if (opt->verbosity < 4) opt->verbosity = 4;
      } else if (strcmp(opt->mc.strategy, "pdr") == 0) {
        opt->mc.aig = 1;
        opt->mc.pdr = 1;
        //  if (opt->verbosity < 4) opt->verbosity = 4;
      } else if (strcmp(opt->mc.strategy, "ic3") == 0) {
        opt->mc.aig = 1;
        opt->mc.pdr = 1;
        opt->verbosity = 4;
	opt->pre.ternarySim = 0;
        opt->pre.peripheralLatches = 1;
      } else if (strcmp(opt->mc.strategy, "ic3d") == 0) {
        opt->mc.aig = 1;
        opt->mc.pdr = 1;
        opt->verbosity = 4;
        opt->mc.strategy = Pdtutil_StrDup("decomp");
	opt->pre.specDecompCore = 1;
	opt->mc.decompTimeLimit = 100.0;
      } else if (strcmp(opt->mc.strategy, "bdd") == 0) {
        opt->mc.bdd = 1;
        opt->trav.apprGrTh = 1000;
        opt->fsm.cut = 2000;
        opt->verbosity = 4;
      } else if (strcmp(opt->mc.strategy, "ind") == 0) {
        opt->mc.aig = 1;
        opt->mc.inductive = 1;
        opt->pre.performAbcOpt = 0;
      } else if (strcmp(opt->mc.strategy, "aigbwd") == 0) {
        opt->mc.aig = 1;
        opt->mc.fwdBwdFP = -1;
        opt->mc.method = Mc_VerifExactBck_c;
        opt->pre.peripheralLatches = 1;
        opt->pre.performAbcOpt = 0;
      } else if (strcmp(opt->mc.strategy, "lms") == 0) {
        opt->mc.aig = 1;
        opt->mc.inductive = 1;
        opt->mc.lemmas = 0;
        opt->pre.performAbcOpt = 0;
      } else if (strcmp(opt->mc.strategy, "cbq") == 0) {
        opt->mc.aig = 1;
        opt->mc.lazy = 1;
        opt->common.lazyRate = 2.3;
        opt->trav.dynAbstr = 2;
        opt->trav.implAbstr = 1;
        // opt->trav.itpPart = 1;
        opt->mc.lemmas = 3;
        opt->trav.lemmasTimeLimit = 60;
        opt->pre.peripheralLatches = 1;
        opt->pre.performAbcOpt = 0;
        opt->pre.abcSeqOpt = 1;
      } else if (strcmp(opt->mc.strategy, "bmc") == 0) {
        opt->mc.aig = 1;
        opt->mc.bmc = 100000;
        opt->pre.performAbcOpt = 0;
      } else if (strcmp(opt->mc.strategy, "itpb") == 0) {
        opt->mc.aig = 1;
        opt->mc.interpolant = 1;
      } else if ((strncmp(opt->mc.strategy, "itp", 3) == 0) ||
        (strncmp(opt->mc.strategy, "igr", 3) == 0)) {
        char nstra = '0';
        j = 3;

	if (strncmp(opt->mc.strategy, "igrpdr", 6) == 0) {
	  opt->mc.igrpdr = 1;
	  j=6;
	  if (opt->mc.strategy[6]=='a') {
	    opt->trav.igrRewind = 0;
	    j++;
	  }
	  else if (opt->mc.strategy[6]=='b') {
            opt->trav.igrGrowConeMax = -1.0;
            opt->trav.igrGrowCone = 0;
	    opt->trav.igrSide = 0;
	    opt->trav.igrMaxIter = 1000;
	    j++;
	  }
	}
        else if (strncmp(opt->mc.strategy, "igr", 3) == 0) {
          opt->trav.itpReuseRings = 2;
        }
        if (strncmp(opt->mc.strategy, "igrb", 4) == 0) {
          opt->trav.igrGrowConeMax = -1.0;
          opt->trav.igrGrowCone = 0;
	  opt->trav.igrSide = 0;
	  opt->trav.igrMaxIter = 1000;
	  j++;
        }
        else if (strncmp(opt->mc.strategy, "igra", 4) == 0) {
          opt->trav.igrRewind = 0;
	  j++;
        }
        else if (strncmp(opt->mc.strategy, "igri", 4) == 0) {
	  opt->trav.igrGrowConeMax = 0.01;
          opt->trav.igrRewind = 0;
	  j++;
        }
        opt->mc.aig = 1;
        opt->mc.tuneForEqCheck = 0;
        opt->trav.dynAbstr = 1;
        opt->trav.implAbstr = 0;
        // opt->trav.itpPart = 1;
        opt->mc.lemmas = 0;     //2;
        // opt->trav.lemmasTimeLimit = 60;
        opt->pre.peripheralLatches = 1;
        // opt->pre.performAbcOpt = 0;
        opt->pre.abcSeqOpt = 0;
        opt->ddi.aigRedRemPeriod = 100;
        if (j < strlen(opt->mc.strategy)) {
          nstra = opt->mc.strategy[j];
          if ((nstra < '0' || nstra > '9') && nstra != 'b')
            nstra = '0';
          else
            j++;
        }
        switch (nstra) {
          case 'b':
            opt->mc.interpolant = 1;
            opt->mc.lazy = -1;
            opt->common.lazyRate = 0.9;
            opt->trav.dynAbstr = 0;
            opt->mc.method = Mc_VerifExactBck_c;
	    opt->trav.igrMaxIter = 1000;
            break;
          case '0':
            opt->mc.lazy = 1;
            opt->common.lazyRate = 0.9;
            opt->trav.dynAbstr = 0;
            // opt->pre.peripheralLatches = 0;
            opt->ddi.aigBddOpt = 0;
            opt->common.aigAbcOpt = 0;
            opt->trav.itpEnToPlusImg = 0;
            opt->trav.itpPart = 0;
            opt->ddi.itpPartTh = 2000000;
            break;
          case '1':
            opt->mc.lazy = 1;
            opt->common.lazyRate = 0.9;
            opt->trav.dynAbstr = 0;
            opt->trav.itpEnToPlusImg = 0;
            break;
          case '2':
            opt->mc.lazy = 1;
            opt->common.lazyRate = 1.0;
            opt->trav.dynAbstr = 2;
            // opt->trav.implAbstr = 2;
            opt->trav.ternaryAbstr = 0;
            break;
          case '3':
            opt->mc.lazy = 1;
            opt->common.lazyRate = 1.0;
            opt->trav.dynAbstr = 2;
            // opt->trav.itpAppr = 1;
            opt->trav.implAbstr = 3;
            opt->trav.ternaryAbstr = 0;
            opt->trav.itpEnToPlusImg = 0;
            break;
          case '4':
            opt->mc.lazy = 1;
            opt->common.lazyRate = 1.0;
            opt->trav.dynAbstr = 0;
            opt->trav.itpAppr = 0;
            opt->trav.implAbstr = 3;
            // opt->trav.dynAbstrInitIter = 0;
            opt->trav.ternaryAbstr = 0;
            opt->trav.itpEnToPlusImg = 0;
            // opt->trav.itpInductiveTo = 2;
            break;
          case '5':
            opt->mc.lazy = 1;
            opt->common.lazyRate = 1.0;
            opt->trav.dynAbstr = 0;
            opt->trav.itpAppr = 1;
            opt->trav.implAbstr = 0;
            // opt->trav.dynAbstrInitIter = 0;
            opt->trav.ternaryAbstr = 0;
            opt->trav.itpEnToPlusImg = 0;
            // opt->trav.itpInductiveTo = 2;
            break;
          case '6':
            opt->mc.lazy = 1;
            opt->common.lazyRate = 1.0;
            opt->trav.dynAbstr = 0;
            opt->trav.itpAppr = 1;
            opt->trav.implAbstr = 3;
            // opt->trav.dynAbstrInitIter = 0;
            opt->trav.ternaryAbstr = 0;
            opt->trav.itpEnToPlusImg = 0;
            // opt->trav.itpInductiveTo = 2;
            break;
          case '7':
            opt->mc.lazy = 1;
            opt->common.lazyRate = 2.3;
            opt->trav.dynAbstr = 2;
            opt->trav.implAbstr = 2;
            opt->trav.ternaryAbstr = 0;
            break;
          default:
            opt->pre.peripheralLatches = 0;
            break;
        }
        for (; j < strlen(opt->mc.strategy); j++) {
          switch (opt->mc.strategy[j]) {
            case 'c':
              opt->pre.findClk = 1;
              opt->pre.twoPhaseRed = 1;
              break;
            case 'X':
              opt->ddi.ternaryAbstrStrategy = 0;
            case 'x':
              opt->trav.ternaryAbstr = 1;
              break;
            case 'l':
              opt->mc.lemmas = 2;
              break;
            case 'd':
              opt->ddi.dynAbstrStrategy = 3;
              break;
            case 'a':
              opt->trav.itpAppr = 1;
              break;
            case 'i':
            case 'p':
              opt->trav.itpInductiveTo = 2;
              break;
            case 'k':
              opt->trav.itpBoundkOpt = 1;
              break;
            case 'K':
              opt->trav.itpBoundkOpt = 2;
              break;
            case 'E':
              opt->trav.itpExactBoundDouble = 1;
            case 'e':
              opt->trav.itpBoundkOpt = 2;
              if (opt->trav.itpReuseRings == 2) {
		opt->trav.itpBoundkOpt = 0;
                /* igr */
		opt->trav.itpBoundkOpt = 0;
		if (opt->trav.igrGrowCone == 3) {
  		  opt->trav.igrGrowCone = 2;  /* exact bound */
		}
                if (0 && opt->trav.itpForceRun < 0) {
                  opt->trav.itpForceRun = 1;
                }
              }
              break;
            case 'I':
              opt->trav.itpInductiveTo = 3;
              break;
            case 's':
              opt->trav.itpStructAbstr = 1;
              break;
            case 'S':
              opt->trav.itpStructAbstr = 2;
              break;
            case 'n':
              opt->trav.itpNew = 1;
              break;
            case 'N':
              opt->trav.itpNew = 2;
              break;
            case 'A':
              opt->trav.itpAppr = 2;
              break;
            case 'r':
              opt->trav.igrRestart = -1;
              break;
            default:
              break;
          }
        }
      } else if (strcmp(opt->mc.strategy, "mix") != 0 &&
        strcmp(opt->mc.strategy, "decomp") != 0 &&
        strcmp(opt->mc.strategy, "expt") != 0) {
        fprintf(stderr, "Error: Invalid Verification Strategy.\n");
        exit(1);
      }

    } else {
#if 1
      fprintf(stderr, "Error: \"%s\" is an invalid option.\nAborting.\n",
        argv[1]);
      exit(1);
#else
      fprintf(stderr, "Invalid option (ignored): %s\n", argv[1]);
      argv++;
      argc--;
#endif
    }
  }

  return opt;                   //(1 + initialArgc - argc);
}


/**Function********************************************************************
  Synopsis    [saveDeltaCOIs]
  Description [save COI Bitmaps for Deltas]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static void
saveDeltaCOIs(
  Fsm_Mgr_t * fsmMgr,
  int optCreateClusterFile,
  char *optClusterFile,
  int optMethodCluster,
  int optThresholdCluster
)
{
  printf("\nBeginDelta\n");
  Ddi_Varsetarray_t *deltaCois;
  Ddi_Bddarray_t *myDeltas = Fsm_MgrReadDeltaBDD(fsmMgr);

  deltaCois =
    Ddi_AigEvalMultipleCoi(myDeltas, myDeltas, Fsm_MgrReadVarPS(fsmMgr),
    optCreateClusterFile, optClusterFile, optMethodCluster,
    optThresholdCluster);
  int nVars = Ddi_VararrayNum(Fsm_MgrReadVarPS(fsmMgr));
  int *myRankVars = Pdtutil_Alloc(int, nVars
  );

  memset(myRankVars, 0, sizeof(int) * nVars);
  DdiDeltaCoisVarsRank(deltaCois, myRankVars, Fsm_MgrReadVarPS(fsmMgr));
  float **cM =
    Pdtutil_Alloc(float *, Ddi_VarsetarrayNum(deltaCois /*myCois */ )
  );
  int i;

  for (i = 0; i < Ddi_VarsetarrayNum(deltaCois /*myCois */ ); i++)
    cM[i] = Pdtutil_Alloc(float,
      Ddi_VarsetarrayNum(deltaCois /*myCois */ )
    );

  DdiCoisStatistics(deltaCois /*myCois */ , cM,
    Ddi_CoiIntersectionOverUnion_c);
  printf("\nEndDelta\n");
  Ddi_Free(deltaCois);
  Pdtutil_Free(cM);
  Pdtutil_Free(myRankVars);
}


/**Function********************************************************************
  Synopsis    [saveDeltaCOIs]
  Description [save COI Bitmaps for Properties]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static void
savePropertiesCOIs(
  Fsm_Mgr_t * fsmMgr,
  int optCreateClusterFile,
  char *optClusterFile,
  int optMethodCluster,
  Ddi_Bddarray_t * pArray,
  int optThresholdCluster
)
{
  //@Signature COI Eval 
  if (pArray != NULL) {
    printf("\nBeginProperties\n");
    Ddi_Varsetarray_t *myCois;
    Ddi_Bddarray_t *myDelta = Fsm_MgrReadDeltaBDD(fsmMgr);

    //printf("\nSignature COI Eval\n");
    //Ddi_Varsetarray_t *myCois;
    //Ddi_Bddarray_t *myDelta = Fsm_MgrReadDeltaBDD (fsmMgr);
    long start = util_cpu_time();

    printf("Eval COI\n");
    myCois =
      Ddi_AigEvalMultipleCoi(myDelta, pArray, Fsm_MgrReadVarPS(fsmMgr),
      optCreateClusterFile, optClusterFile, optMethodCluster,
      optThresholdCluster);
    long end = util_cpu_time();

    printf("\nEndProperties\n");
    //printf("\nELAPSED NEW = %s\n", util_print_time(end-start));


    //printf("\nELAPSED NEW = %s\n", util_print_time(end-start));



    int nVars = Ddi_VararrayNum(Fsm_MgrReadVarPS(fsmMgr));
    int *myRankVars = Pdtutil_Alloc(int, nVars
    );

    memset(myRankVars, 0, sizeof(int) * nVars);
    DdiDeltaCoisVarsRank(myCois, myRankVars, Fsm_MgrReadVarPS(fsmMgr));

    //printf("\n\n\nPASSATO DI QUA\n\n\n");


    //printf("\n\n\nPASSATO DI QUA\n\n\n");


    float **cM = Pdtutil_Alloc(float *, Ddi_VarsetarrayNum(myCois)
    );
    int i;

    for (i = 0; i < Ddi_VarsetarrayNum(myCois); i++)
      cM[i] = Pdtutil_Alloc(float,
        Ddi_VarsetarrayNum(myCois)
      );

    DdiCoisStatistics(myCois, cM, Ddi_CoiIntersectionOverUnion_c);

    Ddi_Free(myCois);
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bddarray_t *
compactProperties(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Mgr_t * ddiMgr,
  Ddi_Varsetarray_t * coiArray,
  int optCreateClusterFile,
  char *optClusterFile,
  int optMethodCluster,
  Ddi_Bddarray_t * pArray,
  int thresholdCluster
)
{
  printf("CLUSTERING: PROPERTIES CLUSTERING NEW\n");
  printf("Num Properties %d\n", Ddi_BddarrayNum(pArray));
  printf("Num variables %d\n", Ddi_BddarrayNum(Fsm_MgrReadDeltaBDD(fsmMgr)));
  savePropertiesCOIs(fsmMgr, optCreateClusterFile, optClusterFile,
    optMethodCluster, pArray, thresholdCluster);
  Ddi_Bddarray_t *pArrayCL = Ddi_BddarrayAlloc(ddiMgr, 0);
  Ddi_Varset_t *partCoiCL = Ddi_VarsetVoid(ddiMgr);
  FILE *fp = fopen(optClusterFile, "r");
  int nProperties, nCluster, *position, i, pPartNumCL;

  fscanf(fp, "%d\n", &nProperties);
  fscanf(fp, "%d\n", &nCluster);
  //printf("nProperties: %d nCluster: %d\n",nProperties,nCluster);
  //printf("Numero proprietà %d\n",Ddi_BddarrayNum(pArray));
  position = Pdtutil_Alloc(int,
    nCluster * 2
  );

  for (i = 0; i < ((nCluster * 2) - 1); i++)
    position[i] = 0;
  while (!feof(fp)) {
    int prop, cluster, k, lastkCL = -1;

    fscanf(fp, "%d %d\n", &prop, &cluster);
    //printf("property: %d cluster: %d\n",prop,cluster);
    Ddi_Bdd_t *p_kCL = Ddi_BddarrayRead(pArray, prop - 1);
    Ddi_Varset_t *coi_kCL = Ddi_VarsetarrayRead(coiArray, prop - 1);

    Ddi_VarsetUnionAcc(partCoiCL, coi_kCL);
    k = (cluster - 1) * 2;
    //printf("Position %d\n",k); 
    if (position[k]) {
      Ddi_BddAndAcc(Ddi_BddarrayRead(pArrayCL, position[k + 1]), p_kCL);
    } else {
      position[k] = 1;
      lastkCL++;
      position[k + 1] = lastkCL;
      Ddi_BddarrayInsertLast(pArrayCL, p_kCL);
      Ddi_Free(partCoiCL);
      partCoiCL = Ddi_VarsetDup(coi_kCL);
    }
  }

  Ddi_Free(partCoiCL);
  pPartNumCL = Ddi_BddarrayNum(pArrayCL);
  fclose(fp);
  printf("Properties reduced to %d\n", pPartNumCL);
  /*
     printf("Grouped coi supps: ");
     for (i=0; i<pPartNumCL; i++) {
     coi = computeFsmCoiVars(fsmMgr, 
     Ddi_BddarrayRead(pArrayCL, i), NULL, 0, 0);
     nCoiShell = Ddi_VarsetarrayNum(coi);
     if (nCoiShell) {
     coivars = Ddi_VarsetDup(Ddi_VarsetarrayRead(coi, nCoiShell-1));
     nCoiVars = Ddi_VarsetNum(coivars);
     printf("%d ", nCoiVars);
     } else {
     coivars = Ddi_VarsetVoid(ddiMgr);
     nCoiVars = 0;
     printf("%d ", nCoiVars);
     }
     //coiPartSize[i] = coiArraySize[i] = nCoiVars;
     //Ddi_VarsetarrayWrite(coiPart, i, coivars);
     Ddi_VarsetarrayWrite(coiArray, i, coivars);
     }
     printf("\n");

     printf("Prop sizes:");

     for (i=0; i<pPartNumCL; i++) {
     printf(" %d", Ddi_BddSize(Ddi_BddarrayRead(pArrayCL, i)));
     }
     printf("\n");
     printf("PROPERTIES CLUSTERING NEW END\n");
     //decompInfo->pArray = pArrayCL;
     //decompInfo->partOrdId = partOrdId;
     //decompInfo->coiArray = coiArray;
   */
  return pArrayCL;
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static char **
readArgs(
  FILE * fp,
  int *argcP
)
{
  char buf[PDTUTIL_MAX_STR_LEN + 1];
  char **args;
  int size = 10, argc;

  argc = 0;
  args = Pdtutil_Alloc(char *,
    size
  );

  args[argc++] = NULL;

  while (fscanf(fp, "%s", buf) > 0) {
    if (argc >= size) {
      size *= 2;
      args = Pdtutil_Realloc(char *,
        args,
        size
      );
    }
    args[argc++] = Pdtutil_StrDup(buf);
  }

  *argcP = argc;
  return (args);

}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
freeArgs(
  int argc,
  char *args[]
)
{
  int i;

  for (i = 0; i < argc; i++) {
    if (args[i] != NULL) {
      Pdtutil_Free(args[i]);
    }
  }
  Pdtutil_Free(args);

}

/**Function*******************************************************************
  Synopsis    [Print PdTRAV Command Line]
  Description [It prints out the help array of string.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
printUsage(
  FILE * fp,
  char *prgrm
)
{
  int i, j;

  fprintf(fp, "USAGE: %s [<options>] <fsmfile>\n", prgrm);
  fprintf(fp, "\nAllowed options:\n");

  i = j = 0;
  do {
    /* individual option */
    fprintf(fp, " %-24s", help[i++]);
    if ((++j) % 3 == 0) {
      fprintf(fp, "\n");
    }
    /* skip full help */
    while (help[i] != NULL) {
      i++;
    }
    i++;
  } while (help[i] != NULL);

  fprintf(fp, "\n\n");
  fprintf(fp, "use -h [-]<opt>  for detailed help on given option\n\n");

  return;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
printHelp(
  FILE * fp,
  char *key
)
{
  int i;
  char mykey[100];

  fprintf(fp, "HELP: %s\n", key);
  if (key[0] == '-') {
    strcpy(mykey, key);
  } else {
    sprintf(mykey, "-%s", key);
  }
  if (mykey[strlen(mykey) - 1] == '\n') {
    mykey[strlen(mykey) - 1] = '\0';
  }

  i = 0;
  do {
    /* individual option */
    Pdtutil_Assert(help[i][0] == '-', "Wrong key in help array");
    if (strncmp(mykey, help[i], strlen(mykey)) == 0) {
      fprintf(stdout, ">>> %s\n", help[i]);
      i++;
      while (help[i] != NULL) {
        fprintf(stdout, "  %s\n", help[i]);
        i++;
      }
    } else {
      while (help[i] != NULL) {
        i++;
      }
    }
    /* skip full help */
    i++;
  } while (help[i] != NULL);

}

/**Function********************************************************************
  Synopsis    [Write to a file called designName.pdtrav.out.cex the counter example found in the AIGER format]
  Description [Write to a file called designName.pdtrav.out.cex the counter example found in the AIGER format]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
void
FbvWriteCex(
  char *fname,
  Fsm_Mgr_t * fsmMgr,
  Fsm_Mgr_t * fsmMgrOriginal
)
{
  char buf[1000] = "";
  int pi = Ddi_VararrayNum(Fsm_MgrReadVarI(fsmMgrOriginal));
  int ps = Ddi_VararrayNum(Fsm_MgrReadVarPS(fsmMgrOriginal));
  int i = 0;
  int useInitStub = 0;

  printf("pi: %d\n", pi);
  Ddi_Vararray_t *vpi = Fsm_MgrReadVarI(fsmMgrOriginal);
  Ddi_Vararray_t *vps = Fsm_MgrReadVarPS(fsmMgrOriginal);
  Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgrOriginal);
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgrOriginal);
  int nStubIter = fsmMgr->stats.initStubSteps;
  Ddi_Bdd_t *cex = Fsm_MgrReadCexBDD(fsmMgr);
  Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
  Ddi_Var_t *cvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");

  if (cex == NULL)
    return;

  Ddi_Bdd_t *myCex = Ddi_BddDup(cex);
  Pdtutil_Array_t *vpsInteger = Ddi_AigCubeMakeIntegerArrayFromBdd(init, vps);
  int part = Ddi_BddPartNum(cex);

  useInitStub = Fsm_MgrReadInitStubBDD(fsmMgr) != NULL;
  if (useInitStub) {
    Ddi_Bdd_t *tfCex = Ddi_BddPartRead(myCex, 0);

    Pdtutil_Assert(nStubIter > 0, "Missing stub iteration #");
    for (i = 1; i < nStubIter; i++) {
      Ddi_BddPartInsert(myCex, i, tfCex);
    }
  }

  sprintf(buf, "%s.cex", fname);
  FILE *cexOut = fopen(buf, "w");

  //printf("print header witness\n");
  fprintf(cexOut, "1\nb0\n");

  for (i = 0; i < ps; i++) {
    int psVal = Pdtutil_IntegerArrayRead(vpsInteger, i);
    Ddi_Var_t *v = Ddi_VararrayRead(vps, i);

    if (v == pvarPs || v == cvarPs)
      continue;
    //printf("print present state\n");
    fprintf(cexOut, "%d", psVal);
  }
  fprintf(cexOut, "\n");

  for (i = 0; i < part; i++) {

    Ddi_Bdd_t *tfCex = Ddi_BddPartRead(myCex, i);
    Ddi_Bdd_t *cexPart = Ddi_AigPartitionTop(tfCex, 0);
    int j, k, n = Ddi_BddPartNum(cexPart);

    //if (Ddi_BddIsOne(tfCex)) continue;
    for (j = 0; j < n; j++) {
      Ddi_Bdd_t *p_i = Ddi_BddPartRead(cexPart, j);
      Ddi_Var_t *v_i, *v_i_tf = Ddi_BddTopVar(p_i);
      char *buf2 = NULL;
      int val = Ddi_BddIsComplement(p_i) ? -1 : 1;

      strcpy(buf, Ddi_VarName(v_i_tf));

      if (i < nStubIter) {
        int istub;

        Pdtutil_Assert(strncmp(buf, "PDT_STUB_PI_", 12) == 0,
          "missing stub vars in cex");
        buf2 = Pdtutil_StrSkipPrefix(buf, "PDT_STUB_PI_");
        Pdtutil_Assert(buf2 != NULL, "missing stub vars in cex");
        istub = Pdtutil_StrRemoveNumSuffix(buf2, '_');
        Pdtutil_Assert(istub >= 0 && istub < nStubIter, "wrong stub index");
        if (istub != i)
          continue;
        v_i = Ddi_VarFromName(ddm, buf2);
      } else {
        buf2 = Pdtutil_StrSkipPrefix(buf, "PDT_TF_VAR_");
        if (buf2 != NULL) {
          Pdtutil_StrRemoveNumSuffix(buf2, '_');
        } else {
          buf2 = buf;
        }
        v_i = Ddi_VarFromName(ddm, buf2);
      }
      Pdtutil_Assert(v_i != NULL, "error looking for ref PI");
      Pdtutil_Assert(Ddi_BddSize(p_i) == 1, "not a cube");
      Pdtutil_Assert(Ddi_VarReadMark(v_i) == 0, "0 var mark required");
      Ddi_VarWriteMark(v_i, val);
    }
    for (j = 0; j < pi; j++) {
      Ddi_Var_t *v = Ddi_VararrayRead(vpi, j);

      //printf("Primary input\n");
      fprintf(cexOut, "%c", Ddi_VarReadMark(v) == 1 ? '1' :
        (Ddi_VarReadMark(v) == -1 ? '0' : 'x'));
      Ddi_VarWriteMark(v, 0);
    }
    fprintf(cexOut, "\n");
    Ddi_Free(cexPart);
  }
  Ddi_Free(myCex);
  fprintf(cexOut, ".\n");
  fclose(cexOut);
}

/**Function********************************************************************
  Synopsis    [Write to a file called designName.pdtrav.out.cex the counter example found in the AIGER format]
  Description [Write to a file called designName.pdtrav.out.cex the counter example found in the AIGER format]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
void
FbvWriteCexNormalized(
  char *fname,
  Fsm_Mgr_t * fsmMgr,
  Fsm_Mgr_t * fsmMgrOriginal,
  int badId
)
{
  static char buf[1000] = "";
  int pi = Ddi_VararrayNum(Fsm_MgrReadVarI(fsmMgrOriginal));
  int ps = Ddi_VararrayNum(Fsm_MgrReadVarPS(fsmMgrOriginal));
  int i = 0, j;

  // printf("pi: %d\n",pi);
  Ddi_Vararray_t *vpi = Fsm_MgrReadVarI(fsmMgrOriginal);
  Ddi_Vararray_t *vpiLocal=NULL;
  Ddi_Vararray_t *vps = Fsm_MgrReadVarPS(fsmMgrOriginal);
  Ddi_Vararray_t *vpsLocal = Fsm_MgrReadVarPS(fsmMgr);
  int phaseAbstr = fsmMgr->stats.phaseAbstr;
  int initStubSteps = fsmMgr->stats.initStubSteps;

  Ddi_Mgr_t *ddmO = Fsm_MgrReadDdManager(fsmMgrOriginal);
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);
  Ddi_Bdd_t *cex = Fsm_MgrReadCexBDD(fsmMgr);
  char *initStr = Fsm_MgrReadInitString(fsmMgrOriginal);
  char *cexStr = Pdtutil_Alloc(char, pi + 2
  );
  int cexStart = 0;

  if (cex == NULL)
    return;

  if (Ddi_BddPartNum(cex) > 0) {
    Ddi_Bdd_t *cex0 = Ddi_BddPartRead(cex, 0);
    char *name = Ddi_ReadName(cex0);

    if (name != NULL && strcmp(name, "PDT_CEX_IS") == 0) {
      Ddi_Bdd_t *initRefine = Ddi_AigPartitionTop(Ddi_BddPartRead(cex, 0), 0);
      int j;

      for (j = 0; j < Ddi_BddPartNum(initRefine); j++) {
        Ddi_Bdd_t *p_j = Ddi_BddPartRead(initRefine, j);
        Ddi_Var_t *v_j = Ddi_BddTopVar(p_j);
        int isCompl = Ddi_BddIsComplement(p_j);

        Ddi_VarWriteMark(v_j, isCompl ? -1 : 1);
      }
      cexStart = 1;
      Ddi_Free(initRefine);
    }
    if (initStubSteps > 0) {
      Ddi_Bdd_t *initRefine = Ddi_AigPartitionTop(Ddi_BddPartRead(cex, 1), 0);
      int j;

      for (j = 0; j < Ddi_BddPartNum(initRefine); j++) {
        Ddi_Bdd_t *p_j = Ddi_BddPartRead(initRefine, j);
        Ddi_Var_t *v_j = Ddi_BddTopVar(p_j);
        int isCompl = Ddi_BddIsComplement(p_j);
        char *name = Ddi_VarName(v_j);
        char *buf2 = Pdtutil_StrSkipPrefix(name, "PDT_STUB_PS_");
        if (buf2 != NULL) {
          Ddi_Var_t *v_j_s = Ddi_VarFromName(ddm, buf2);
          if (v_j_s != NULL) {
            Ddi_VarWriteMark(v_j_s, isCompl ? -1 : 1);
          }
        }
      }
      Ddi_Free(initRefine);
    }
  }

  Ddi_Bdd_t *myCex = Ddi_BddDup(cex);
  int part = Ddi_BddPartNum(cex);

  sprintf(buf, "%s.cex", fname);
  FILE *cexOut = fopen(buf, "w");

  //printf("print header witness\n");
  fprintf(cexOut, "1\nb%d\n", badId);

  for (i = 0; (i < ps) && (initStr[i] != '\0'); i++) {
    char c = initStr[i];

    if (c == 'x') {
      Ddi_Var_t *vRef = Ddi_VararrayRead(vps, i);
      Ddi_Var_t *v = Ddi_VarFromName(ddm, Ddi_VarName(vRef));
      int m = Ddi_VarReadMark(v);

      if (m == -1) {
        c = '0';
      } else if (m == 1) {
        c = '1';
      }
    }
    fprintf(cexOut, "%c", c);
  }
  fprintf(cexOut, "\n");
  Ddi_VararrayWriteMark(vpsLocal, 0);


  // vpiLocal = Ddi_VararrayDup(Fsm_MgrReadVarI(fsmMgr));
  /* done to take into account pis not listed explicitly 
     (e.g. from init stub or target en) */
  vpiLocal = Ddi_BddSuppVararray(cex);

  if (phaseAbstr) {
    char *buf2;
    Ddi_Vararray_t *vpiLocal2 = Ddi_VararrayAlloc(ddm, 0);

    for (i = 0; i < Ddi_VararrayNum(vpiLocal); i++) {
      Ddi_Var_t *v_i = Ddi_VararrayRead(vpiLocal, i);

      strcpy(buf, Ddi_VarName(v_i));
      buf2 = Pdtutil_StrSkipPrefix(buf, "PDT_ITP_TWOPHASE_PI_");
      if (buf2 != NULL) {
        int s = Pdtutil_StrRemoveNumSuffix(buf2, '_');

        Pdtutil_Assert(s >= 0, "wrong phase var suffix");
      } else {
        buf2 = buf;
      }
      v_i = Ddi_VarFromName(ddm, buf2);
      Pdtutil_Assert(v_i != NULL, "error looking for ref PI");
      if (Ddi_VarReadMark(v_i) == 1)
        continue;
      Ddi_VarWriteMark(v_i, 1);
      Ddi_VararrayInsertLast(vpiLocal2, v_i);
    }
    Ddi_Free(vpiLocal);
    vpiLocal = vpiLocal2;
  }
  Ddi_VararrayWriteMark(vpiLocal, 0);
  if (initStubSteps > 0) {
    Ddi_Varset_t *isPis = Ddi_VarsetVoid(ddm);
    Ddi_Vararray_t *isSupp =
      Ddi_BddarraySuppVararray(Fsm_MgrReadInitStubBDD(fsmMgr));
    char *buf2, *buf1;

    for (i = 0; i < Ddi_VararrayNum(isSupp); i++) {
      Ddi_Var_t *v_i = Ddi_VararrayRead(isSupp, i);

      strcpy(buf, Ddi_VarName(v_i));
      buf2 = Pdtutil_StrSkipPrefix(buf, "PDT_STUB_PI_");
      if (buf2 != NULL) {
        int s = Pdtutil_StrRemoveNumSuffix(buf2, '_');

        Pdtutil_Assert(s >= 0, "wrong stub var suffix");
      } else {
        buf2 = Pdtutil_StrSkipPrefix(buf, "PDT_STUB_PS_");
        if (buf2 != NULL) {
          continue; // just skip
        } else {
          buf2 = buf;
        }
      }
      if (phaseAbstr) {
        buf1 = buf2;
        buf2 = Pdtutil_StrSkipPrefix(buf1, "PDT_ITP_TWOPHASE_PI_");
        if (buf2 != NULL) {
          int s = Pdtutil_StrRemoveNumSuffix(buf2, '_');

          Pdtutil_Assert(s >= 0, "wrong phase var suffix");
        } else {
          buf2 = buf1;
        }
      }
      v_i = Ddi_VarFromName(ddm, buf2);
      Pdtutil_Assert(v_i != NULL, "error looking for ref PI");
      if (Ddi_VarReadMark(v_i) == 1)
        continue;
      Ddi_VarWriteMark(v_i, 1);
      Ddi_VarsetAddAcc(isPis, v_i);
    }
    Ddi_Vararray_t *isPisA = Ddi_VararrayMakeFromVarset(isPis, 1);

    Ddi_VararrayWriteMark(isPisA, 0);
    Ddi_VararrayUnionAcc(vpiLocal, isPisA);
    Ddi_Free(isSupp);
    Ddi_Free(isPis);
    Ddi_Free(isPisA);
  }

  Ddi_VararrayWriteMark(vpiLocal, 1);

  for (i = 0; i < pi; i++) {
    Ddi_Var_t *v_i_ref = Ddi_VararrayRead(vpi, i);
    Ddi_Var_t *v_i = Ddi_VarFromName(ddm, Ddi_VarName(v_i_ref));

    if (v_i == NULL)
      continue;
    if (Ddi_VarReadMark(v_i) == 0)
      continue;

    Ddi_VarWriteMark(v_i, i+2);
  }

  for (i = cexStart; i < part; i++) {

    Ddi_Bdd_t *tfCex = Ddi_BddPartRead(myCex, i);
    Ddi_Bdd_t *cexPart = Ddi_AigPartitionTop(tfCex, 0);
    int k, n = Ddi_BddPartNum(cexPart);

    for (j = 0; j < pi; j++) {
      cexStr[j] = 'x';
    }

    for (j = 0; j < n; j++) {
      Ddi_Bdd_t *p_i = Ddi_BddPartRead(cexPart, j);
      Ddi_Var_t *v_i = Ddi_BddTopVar(p_i);
      int iRef = Ddi_VarReadMark(v_i)-2;

      if ((iRef >= 0) && (iRef < pi)) {
        Pdtutil_Assert((iRef >= 0) && (iRef < pi), "wrong var ref");
        Pdtutil_Assert(v_i != NULL, "error looking for ref PI");
        Pdtutil_Assert(Ddi_BddSize(p_i) == 1, "not a cube");
        cexStr[iRef] = Ddi_BddIsComplement(p_i) ? '0' : '1';
      }
    }

    for (j = 0; j < pi; j++) {
      fprintf(cexOut, "%c", cexStr[j]);
    }
    fprintf(cexOut, "\n");
    Ddi_Free(cexPart);
  }

  Ddi_VararrayWriteMark(vpiLocal, 0);

  Pdtutil_Free(cexStr);

  Ddi_Free(vpiLocal);
  Ddi_Free(myCex);
  fprintf(cexOut, ".\n");
  fclose(cexOut);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
invarVerif(
  char *fsm,
  char *ord,
  Fbv_Globals_t * opt
)
{
  Ddi_Mgr_t *ddiMgr;
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Varset_t *psvars, *nsvars;
  Tr_Mgr_t *trMgr = NULL;
  Trav_Mgr_t *travMgrBdd = NULL, *travMgrAig = NULL;
  Fsm_Mgr_t *fsmMgr = NULL;
  Fsm_Mgr_t *fsmMgrOpt = NULL;
  Tr_Tr_t *tr = NULL;
  Fsm_Mgr_t *fsmMgrOriginal = NULL;
  Ddi_Bdd_t *init, *invarspec, *invar = NULL, *noDead, *care = NULL;
  Ddi_Bddarray_t *delta, *lambda, *pArray = NULL, *deltaAig = NULL;
  long elapsedTime, startVerifTime, currTime, travTime, setupTime;
  int loopReduce, loopReduceCnt;
  float firstRunTimeLimit = -1.0;
  float firstRunNodeLimit = -1;
  int doFastRun = 0;
  int twoPhaseVarIndex = 0;
  char phaseSuffix[2];
  int stallEnable = 0;
  int ibmConstr = 0 && opt->mc.wFsm != NULL;
  int forceDeltaConstraint = 1;
  int doXorDetection = 0;
  int doXorCuts = 0;

  static int nRun = 0;

  nRun++;

  if (opt->expt.expertLevel > 0) {
    opt->mc.tuneForEqCheck = 1;
    opt->pre.findClk = 1;
    opt->pre.terminalScc = 1;
    opt->pre.impliedConstr = 1;
    opt->pre.peripheralLatches = 1;
    opt->pre.specConj = 1;
    if (opt->expt.expertArgv != NULL) {
      FbvParseArgs(opt->expt.expertArgc, opt->expt.expertArgv, opt);
    }
  }

  /**********************************************************************/
  /*                        Create DDI manager                          */
  /**********************************************************************/

  ddiMgr = Ddi_MgrInit(opt->expName, NULL, 0,
    DDI_UNIQUE_SLOTS, DDI_CACHE_SLOTS * 10, 0, -1, -1);

  ddiMgr->exist.clipLevel = 0;
  ddiMgr->exist.clipThresh = opt->ddi.imgClipTh;
  if (opt->trav.guidedTrav) {
    ddiMgr->exist.clipLevel = 2;
  } else if (opt->trav.fbvMeta || opt->ddi.existClipTh > 0) {
    ddiMgr->exist.clipThresh = opt->ddi.existClipTh;
    ddiMgr->exist.clipLevel = 0;
  } else {
    ddiMgr->exist.clipLevel = 1;
  }

  if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
    ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
    Ddi_MgrSetVerbosity(ddiMgr, Pdtutil_VerbLevel_e(opt->verbosity));
  }

  Ddi_MgrSiftEnable(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"));
  Ddi_MgrSetSiftThresh(ddiMgr, opt->ddi.siftTh);
  Ddi_MgrSetExistOptLevel(ddiMgr, opt->ddi.exist_opt_level);
  Ddi_MgrSetExistOptClustThresh(ddiMgr, opt->ddi.exist_opt_thresh);
  Ddi_MgrSetAigTernaryAbstrStrategy(ddiMgr, opt->ddi.ternaryAbstrStrategy);
  Ddi_MgrSetAigDynAbstrStrategy(ddiMgr, opt->ddi.dynAbstrStrategy);
  Ddi_MgrSetAigItpAbc(ddiMgr, opt->ddi.itpAbc);
  Ddi_MgrSetAigItpCore(ddiMgr, opt->ddi.itpCore);
  Ddi_MgrSetAigItpMem(ddiMgr, opt->ddi.itpMem);
  Ddi_MgrSetAigItpOpt(ddiMgr, opt->ddi.itpOpt);
  Ddi_MgrSetAigItpStore(ddiMgr, opt->ddi.itpStore);
  Ddi_MgrSetAigItpPartTh(ddiMgr, opt->ddi.itpPartTh);
  Ddi_MgrSetAigCnfLevel(ddiMgr, opt->ddi.aigCnfLevel);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpActiveVars_c, inum, opt->ddi.itpActiveVars);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStoreTh_c, inum, opt->ddi.itpStoreTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpAigCore_c, inum, opt->ddi.itpAigCore);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpTwice_c, inum, opt->ddi.itpTwice);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpIteOptTh_c, inum, opt->ddi.itpIteOptTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStructOdcTh_c, inum,
    opt->ddi.itpStructOdcTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiNnfClustSimplify_c, inum,
    opt->ddi.nnfClustSimplify);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMap_c, inum, opt->ddi.itpMap);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompute_c, inum, opt->ddi.itpCompute);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpLoad_c, pchar, opt->ddi.itpLoad);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpDrup_c, inum, opt->ddi.itpDrup);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompact_c, inum, opt->ddi.itpCompact);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpClust_c, inum, opt->ddi.itpClust);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpNorm_c, inum, opt->ddi.itpNorm);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpSimp_c, inum, opt->ddi.itpSimp);


  /**********************************************************************/
  /*                        Create FSM manager                          */
  /**********************************************************************/

  opt->stats.startTime = util_cpu_time();

  fsmMgr = Fsm_MgrInit("fsm", NULL);
  Fsm_MgrSetDdManager(fsmMgr, ddiMgr);
  FbvSetFsmMgrOpt(fsmMgr, opt, 0);

  if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
    ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
    Fsm_MgrSetVerbosity(fsmMgr, Pdtutil_VerbLevel_e(opt->verbosity));
  }

  Fsm_MgrSetUseAig(fsmMgr, 1);
  Fsm_MgrSetCutThresh(fsmMgr, opt->fsm.cut);
  Fsm_MgrSetOption(fsmMgr, Pdt_FsmCutAtAuxVar_c, inum, opt->fsm.cutAtAuxVar);

  /**********************************************************************/
  /*                        Read BLIF file                              */
  /**********************************************************************/

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "-- FSM Loading.\n"));

  if (strstr(fsm, ".fsm") != NULL) {
    if (strcmp(opt->mc.ord, "dfs") == 0) {
      Pdtutil_Free(opt->mc.ord);
      opt->mc.ord = NULL;
    }
    if (Fsm_MgrLoad(&fsmMgr, ddiMgr, fsm, opt->mc.ord, 1,
        Pdtutil_VariableOrderDefault_c) == 1) {
      fprintf(stderr, "-- FSM Loading Error.\n");
      exit(1);
    }
  } else if (strstr(fsm, ".aig") != NULL || strstr(fsm, ".aag") != NULL) {
    if (strcmp(opt->mc.ord, "dfs") == 0) {
      Pdtutil_Free(opt->mc.ord);
      opt->mc.ord = NULL;
    }
    if (Fsm_MgrLoadAiger(&fsmMgr, ddiMgr, fsm, opt->mc.ord, NULL,
        Pdtutil_VariableOrderDefault_c) == 1) {
      fprintf(stderr, "-- FSM Loading Error.\n");
      exit(1);
    }
    if (opt->pre.performAbcOpt==1) {
      int ret = Fsm_MgrAbcReduceMulti(fsmMgr, 0.99);
    }
  } else {
    char **ci = Pdtutil_Alloc(char *, 4
    );
    int ciNum = 0;
    int myCoiLimit = opt->tr.coiLimit;

    ci[0] = NULL;

    if (0 && strcmp(opt->mc.ord, "dfs") == 0) {
      Pdtutil_Free(opt->mc.ord);
      opt->mc.ord = NULL;
    }
    if (opt->pre.doCoi > 1) {
      if (opt->mc.invSpec != NULL) {
        Ddi_Expr_t *e = Ddi_ExprLoad(ddiMgr, opt->mc.invSpec, NULL);

        ci = Expr_ListStringLeafs(e);
        ciNum = 1;
        Ddi_Free(e);
      } else if (opt->mc.ctlSpec != NULL) {
        Ddi_Expr_t *e = Ddi_ExprLoad(ddiMgr, opt->mc.ctlSpec, NULL);

        ci = Expr_ListStringLeafs(e);
        ciNum = 1;
        Ddi_Free(e);
      } else if (opt->mc.all1) {
        ci[ciNum] = Pdtutil_Alloc(char,
          30
        );

        sprintf(ci[ciNum], "l %d %d %d %d", 0, opt->mc.all1 - 1,
          myCoiLimit, opt->pre.constrCoiLimit);
        ci[++ciNum] = NULL;
      } else if (opt->mc.idelta >= 0) {
        ci[ciNum] = Pdtutil_Alloc(char,
          30
        );

        sprintf(ci[ciNum], "l %d %d %d %d", opt->mc.idelta, opt->mc.idelta,
          myCoiLimit, opt->pre.constrCoiLimit);
        ci[++ciNum] = NULL;
      } else if (opt->mc.nlambda != NULL) {
        ci[ciNum] = Pdtutil_Alloc(char,
          strlen(opt->mc.nlambda) + 4
        );

        sprintf(ci[ciNum], "n %s %d %d", opt->mc.nlambda,
          myCoiLimit, opt->pre.constrCoiLimit);
        ci[++ciNum] = NULL;
        opt->mc.ilambda = 0;
      } else {
        ci[ciNum] = Pdtutil_Alloc(char, 30);

        sprintf(ci[ciNum], "o %d %d %d %d", opt->mc.ilambda, opt->mc.ilambda,
          myCoiLimit, opt->pre.constrCoiLimit);
        ci[++ciNum] = NULL;
        opt->mc.ilambda = 0;
      }
    }
    if (opt->mc.ninvar != NULL) {
      ci[ciNum] = Pdtutil_Alloc(char,
        strlen(opt->mc.ninvar) + 4
      );

      sprintf(ci[ciNum], "i %s %d %d", opt->mc.ninvar, myCoiLimit,
        opt->pre.constrCoiLimit);
      ci[++ciNum] = NULL;
    }
    if (Fsm_MgrLoadFromBlifWithCoiReduction(&fsmMgr, ddiMgr, fsm, opt->mc.ord,
        1, Pdtutil_VariableOrderDefault_c, ci) == 1) {
      fprintf(stderr, "BLIF File Loading Error.\n");
    }
    if (ci != NULL) {
      Pdtutil_StrArrayFree(ci, -1);
    }
  }

  if (opt->pre.initMinterm >= 0) {
    Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
    Ddi_Varset_t *initSupp = Ddi_BddSupp(init);
    Ddi_Varset_t *forceVars =
      Ddi_VarsetMakeFromArray(Fsm_MgrReadVarPS(fsmMgr));
    Ddi_VarsetDiffAcc(forceVars, initSupp);
    Ddi_Free(initSupp);
    Pdtutil_Assert(opt->pre.initMinterm <= 1,
      "invalid initMinterm value (0/1 allowed)");
    if (!Ddi_VarsetIsVoid(forceVars)) {
      Ddi_Vararray_t *vA = Ddi_VararrayMakeFromVarset(forceVars, 1);
      int j;

      printf("filling INIT with %d %cs\n",
        Ddi_VararrayNum(vA), '0' + opt->pre.initMinterm);
      for (j = 0; j < Ddi_VararrayNum(vA); j++) {
        Ddi_Var_t *v = Ddi_VararrayRead(vA, j);
        Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(v, opt->pre.initMinterm);

        Ddi_BddAndAcc(init, lit);
        Ddi_Free(lit);
      }
      Ddi_Free(vA);
    }
    Ddi_Free(forceVars);
  }



  if (0) {
    Fsm_Mgr_t *fsmMgr2 = Fsm_MgrDupWithNewDdiMgr(fsmMgr);

    Fsm_MgrQuit(fsmMgr);
    Ddi_MgrQuit(ddiMgr);

    fsmMgr = fsmMgr2;
    ddiMgr = Fsm_MgrReadDdManager(fsmMgr);
  }

  //fsmMgrOriginal = Fsm_MgrDup(fsmMgr);
  //fsmMgrOpt = Fsm_MgrAbcReduce(fsmMgr)

  if (nRun > 1) {
    opt->mc.method = opt->mc.saveMethod;
    opt->mc.compl_invarspec = opt->mc.saveComplInvarspec;
  } else if (opt->mc.enFastRun > 0
    && Ddi_VararrayNum(Fsm_MgrReadVarPS(fsmMgr)) < 100) {
    /* try fwd BDDs */
    firstRunTimeLimit = (float)opt->mc.enFastRun;
    firstRunNodeLimit = 300000;
    opt->mc.saveMethod = opt->mc.method;
    opt->mc.saveComplInvarspec = opt->mc.compl_invarspec;
    opt->mc.method = Mc_VerifExactFwd_c;
    doFastRun = 1;
  }

  startVerifTime = util_cpu_time();

  if (0) {
    Ddi_Bdd_t *l = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
      DDDMP_MODE_DEFAULT, "test.bdd", NULL);

    WindowPart(l, opt);
    Ddi_Free(l);
    exit(1);
  }


  lambda = Fsm_MgrReadLambdaBDD(fsmMgr);

  if (opt->fsm.manualAbstr != NULL) {
    ManualAbstraction(fsmMgr, opt->fsm.manualAbstr);
  }

  pi = Fsm_MgrReadVarI(fsmMgr);
  ps = Fsm_MgrReadVarPS(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);

  if (Ddi_BddarrayNum(delta) > 20000) {
    /* too big */
    opt->mc.nogroup = 1;
    Ddi_MgrSetSiftThresh(ddiMgr, 100000000);
  }

  if (0 && !opt->mc.nogroup) {
    Ddi_MgrCreateGroups2(ddiMgr, ps, ns);
  }

  if (ps == NULL && opt->mc.checkMult) {
    FbvCombCheck(fsmMgr, Fbv_Mult_c);
    return 0;
  }

  if (ps != NULL) {
    psvars = Ddi_VarsetMakeFromArray(ps);
    nsvars = Ddi_VarsetMakeFromArray(ns);
  } else {
    opt->trav.noInit = 1;
    opt->mc.combinationalProblem = 1;
    psvars = nsvars = NULL;
  }

  if (opt->trav.noInit) {
    init = Ddi_BddMakeConst(ddiMgr, 0);
  } else {
    init = Ddi_BddDup(Fsm_MgrReadInitBDD(fsmMgr));
  }
  Fsm_MgrSetInitBDD(fsmMgr, init);

  Ddi_Free(init);

  opt->stats.setupTime = util_cpu_time();
  /* COMMON AIG-BDD PART */

  if (opt->common.clk != NULL) {
    Fsm_MgrSetClkStateVar(fsmMgr, opt->common.clk, 1);
  }

  if (opt->mc.ninvar != NULL) {
    int i;

    invar = NULL;
    for (i = 0; i < Ddi_BddarrayNum(lambda); i++) {
      char *name = Ddi_ReadName(Ddi_BddarrayRead(lambda, i));

      if (name != NULL && strcmp(name, opt->mc.ninvar) == 0) {
        invar = Ddi_BddDup(Ddi_BddarrayRead(lambda, i));
        break;
      }
    }
    Pdtutil_Assert(invar != NULL, "Lambda not found by name");
  } else if (Fsm_MgrReadConstraintBDD(fsmMgr) != NULL) {
    invar = Ddi_BddDup(Fsm_MgrReadConstraintBDD(fsmMgr));
  }

  if (0 && opt->trav.invarFile != NULL) {
    if (strstr(opt->trav.invarFile, ".aig") != NULL) {
      Ddi_Bddarray_t *rplus = Ddi_AigarrayNetLoadAiger(ddiMgr,
        NULL, opt->trav.invarFile);
      Ddi_Bdd_t *myInvar =
        Ddi_BddarrayExtract(rplus, Ddi_BddarrayNum(rplus) - 1);
      if (invar == NULL) {
        invar = Ddi_BddDup(myInvar);
      } else {
        Ddi_BddAndAcc(invar, myInvar);
      }
      Fsm_MgrSetConstraintBDD(fsmMgr, invar);
      Ddi_Free(myInvar);
      Ddi_Free(rplus);
      opt->trav.invarFile = NULL;
    }
  }


  if (opt->mc.ilambda >= Ddi_BddarrayNum(lambda)) {
    opt->mc.ilambda = Ddi_BddarrayNum(lambda) - 1;
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "ilambda selection too large: %d forced.\n",
        opt->mc.ilambda));
  }

  if (opt->trav.noCheck) {
    invarspec = Ddi_BddMakeConstAig(ddiMgr, 1);

    if (opt->mc.method == Mc_VerifPrintNtkStats_c) {
      Ddi_Vararray_t *pi, *ps;
      Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);

      pi = Fsm_MgrReadVarI(fsmMgr);
      ps = Fsm_MgrReadVarPS(fsmMgr);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout,
          "Design Size: %d inputs / %d state variables / %d delta nodes \n",
          Ddi_VararrayNum(pi), Ddi_VararrayNum(ps), Ddi_BddarraySize(delta))
        );
    }
  } else {
    invarspec = FbvGetSpec(fsmMgr, NULL, invar, NULL, opt);
    //      invarspec = getSpec (fsmMgr,NULL,NULL,opt);
    Fsm_MgrSetInvarspecBDD(fsmMgr, invarspec);
    Ddi_Free(invarspec);
  }

  // fsmMgrOriginal = Fsm_MgrDup(fsmMgr);

  if (0 && (Fsm_MgrReadConstraintBDD(fsmMgr) != NULL)) {
    Ddi_Bdd_t *constr = NULL;

    constr = Ddi_BddDup(Fsm_MgrReadConstraintBDD(fsmMgr));
    Fsm_MgrAbcScorr(fsmMgr, NULL, constr, NULL, 1);
    Ddi_Free(constr);
    Ddi_Free(invar);
    //    invar = Ddi_BddDup(Fsm_MgrReadConstraintBDD(fsmMgr));
  }


  if (opt->mc.rInit != NULL) {

    Ddi_Free(init);

    Ddi_MgrSetSiftThresh(ddiMgr, 1000000);
    printf("\n**** Reading INIT set from %s ****\n", opt->mc.rInit);
    init = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
      DDDMP_MODE_DEFAULT, opt->mc.rInit, NULL);

    //    Ddi_MgrReduceHeap (ddiMgr, Ddi_ReorderingMethodString2Enum("sift"), 0);
    Ddi_BddSetAig(init);
    Ddi_MgrSetSiftThresh(ddiMgr, opt->ddi.siftTh);

    Fsm_MgrSetInitBDD(fsmMgr, init);

  }

  {
    int addPropWitness = 0;
    Fsm_Fsm_t *fsmFsm = Fsm_FsmMakeFromFsmMgr(fsmMgr);
    if (addPropWitness)
      Fsm_FsmAddPropertyWitnesses(fsmFsm);

    Fsm_FsmFoldProperty(fsmFsm, opt->mc.compl_invarspec,
      opt->trav.cntReachedOK, 1);
    Fsm_FsmFoldConstraint(fsmFsm, 1 /*opt->mc.compl_invarspec*/);
    //    Fsm_FsmFoldInit(fsmFsm);
    Fsm_FsmWriteToFsmMgr(fsmMgr, fsmFsm);
    Fsm_FsmFree(fsmFsm);
    invarspec = Fsm_MgrReadInvarspecBDD(fsmMgr);
  }

  fsmMgrOriginal = Fsm_MgrDup(fsmMgr);

  if (opt->fsm.insertCutLatches > 0) {
    Fsm_Mgr_t *fsmMgrNew = Fsm_RetimeGateCuts(fsmMgr, opt->fsm.insertCutLatches);
    Fsm_MgrQuit(fsmMgr);
    fsmMgr = fsmMgrNew;
    invarspec = Fsm_MgrReadInvarspecBDD(fsmMgr);
  }
  
  invarspec = Ddi_BddDup(invarspec);
  pi = Fsm_MgrReadVarI(fsmMgr);
  ps = Fsm_MgrReadVarPS(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);

  if (opt->mc.combinationalProblem || invarspec!=NULL && (Ddi_BddIsConstant(invarspec))) {
    Ddi_BddNotAcc(invarspec);
    opt->stats.verificationOK = !Ddi_AigSat(invarspec);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: %s.\n",
        opt->stats.verificationOK ? "PASS" : "FAIL"));
    Ddi_BddNotAcc(invarspec);

    //    Fsm_MgrQuit(fsmMgr);
    goto freeMem1;
  }

  if (opt->pre.peripheralLatches) {
    int nRed;

    do {
      nRed = Fsm_RetimePeriferalLatches(fsmMgr);
      opt->pre.peripheralLatchesNum += nRed;
    } while (nRed > 0);
    if (care != NULL) {
      Ddi_Varset_t *psVars = Ddi_VarsetMakeFromArray(Fsm_MgrReadVarPS(fsmMgr));

      Ddi_BddExistProjectAcc(care, psVars);
      Ddi_Free(psVars);
    }

  }

  opt->stats.setupTime = util_cpu_time();

  while (opt->pre.forceInitStub) {
    Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
    Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
    Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
    char suffix[4];
    Ddi_Vararray_t *newPiVars;
    Ddi_Bddarray_t *newPiLits;
    Ddi_Bdd_t *initState = Fsm_MgrReadInitBDD(fsmMgr);
    Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
    Ddi_Bddarray_t *initStub = Ddi_BddarrayDup(delta);

    if (Fsm_MgrReadInitStubBDD(fsmMgr) != NULL) {
      Ddi_BddarrayComposeAcc(initStub, ps, Fsm_MgrReadInitStubBDD(fsmMgr));
    } else {
      Ddi_Bdd_t *myInit = Ddi_BddMakeMono(initState);

      Pdtutil_Assert(Ddi_BddIsCube(myInit), "Init not a cube - not supported");
      Ddi_AigarrayConstrainCubeAcc(initStub, initState);
      Ddi_Free(myInit);
    }

    sprintf(suffix, "%d", opt->pre.forceInitStub);
#if 0
    newPiVars = Ddi_VararrayMakeNewAigVars(pi, "PDT_FORCED_STUB_PI", suffix);
#else
    newPiVars = Ddi_VararrayMakeNewVars(pi, "PDT_FORCED_STUB_PI", suffix, 1);
#endif
    newPiLits = Ddi_BddarrayMakeLiteralsAig(newPiVars, 1);
    Ddi_BddarrayComposeAcc(initStub, pi, newPiLits);
    Ddi_Free(newPiVars);
    Ddi_Free(newPiLits);
    Fsm_MgrSetInitStubBDD(fsmMgr, initStub);
    Ddi_Free(initStub);
    opt->pre.forceInitStub--;
    Fsm_MgrIncrInitStubSteps(fsmMgr, 1);
    Pdtutil_WresIncrInitStubSteps(1);
  }

  if (opt->pre.findClk) {
    Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
    Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
    int i, nstate = Ddi_VararrayNum(ps);

    for (i = 0; i < nstate; i++) {
      Ddi_Bdd_t *dClk0, *dClk1;
      Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);

      /* skip too large functions */
      if (Ddi_BddSize(d_i) > 100)
        continue;
      dClk0 = Ddi_BddCofactor(d_i, v_i, 0);
      Ddi_BddNotAcc(dClk0);
      dClk1 = Ddi_BddCofactor(d_i, v_i, 1);
      if (!Ddi_AigSat(dClk1) && !Ddi_AigSat(dClk0)) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Clock found; Name: %s\n", Ddi_VarName(v_i)));
        opt->pre.clkVarFound = v_i;
      } else {
        Ddi_Varset_t *s_i = Ddi_BddSupp(d_i);

        if (Ddi_VarsetNum(s_i) == 1) {
          int j;
          Ddi_Var_t *v_in = Ddi_VarsetTop(s_i);

          for (j = i + 1; j < nstate; j++) {
            if (Ddi_VararrayRead(ps, j) == v_in) {
              break;
            }
          }
          if (j < nstate) {
            Ddi_Bdd_t *d_j = Ddi_BddarrayRead(delta, j);
            Ddi_Varset_t *s_j = Ddi_BddSupp(d_j);

            if (Ddi_VarsetNum(s_j) == 1 && Ddi_VarInVarset(s_j, v_i)) {
              if (Ddi_BddIsComplement(d_i) && Ddi_BddIsComplement(d_j)) {
                Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
                Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
                int eqLatches = 1;
                int diffLatches = 1;

                Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
                  Pdtutil_VerbLevelNone_c,
                  fprintf(stdout, "Clock pair found; Names: %s - %s.\n",
                    Ddi_VarName(v_i), Ddi_VarName(v_in)));
                opt->pre.clkVarFound = v_i;
#if 1
                /* check init state !!! */
                if (initStub != NULL) {
                  Ddi_Bdd_t *st_i = Ddi_BddarrayRead(initStub, i);
                  Ddi_Bdd_t *st_j = Ddi_BddarrayRead(initStub, j);
                  Ddi_Bdd_t *st_jn = Ddi_BddNot(st_j);

                  eqLatches = Ddi_BddEqualSat(st_i, st_j);
                  diffLatches = Ddi_BddEqualSat(st_i, st_jn);
                  Ddi_Free(st_jn);
                } else {
                  Ddi_Bdd_t *dif = Ddi_BddMakeLiteralAig(v_i, 1);
                  Ddi_Bdd_t *aux = Ddi_BddMakeLiteralAig(v_in, 1);

                  Ddi_BddDiffAcc(dif, aux);
                  Ddi_Free(aux);
                  eqLatches = Ddi_AigSatAnd(init, dif, NULL);
                  Ddi_BddNotAcc(dif);
                  diffLatches = Ddi_AigSatAnd(init, dif, NULL);
                  Ddi_Free(dif);
                }
                if (eqLatches && !diffLatches) {
                  /* OK clock */
                  Ddi_DataCopy(d_i, d_j);
                } else {
                  /* undo clk */
                  opt->pre.clkVarFound = NULL;
                }
#endif
              }
            }
            Ddi_Free(s_j);
          }
        }
        Ddi_Free(s_i);
      }
      Ddi_Free(dClk0);
      Ddi_Free(dClk1);
    }
  }
  //  done after tscc and impl constr
  //  FbvTestSec (fsmMgr, opt);

  if (0 && opt->pre.doCoi >= 1) {
    
    Ddi_Varsetarray_t *coi;
    int nCoi;
    
    coi =
      computeFsmCoiVars(fsmMgr, invarspec, NULL, opt->tr.coiLimit, 1);
    if (opt->pre.wrCoi != NULL) {
      writeCoiShells(opt->pre.wrCoi, coi, ps);
    }
    nCoi = Ddi_VarsetarrayNum(coi);
    fsmCoiReduction(fsmMgr, Ddi_VarsetarrayRead(coi, nCoi - 1));
    
    Ddi_Free(coi);
  }

  if (0 && opt->pre.impliedConstr) { // check order of implied and terminalScc
    Ddi_Bdd_t *myInvar = Fsm_ReduceImpliedConstr(fsmMgr,
      opt->pre.specDecompIndex, invar, &opt->pre.impliedConstrNum);

    if (myInvar != NULL) {
      if (invar == NULL) {
        invar = myInvar;
      } else {
        Ddi_BddAndAcc(invar, myInvar);
        Ddi_Free(myInvar);
      }
    }
  }
  if (opt->pre.terminalScc) {
    opt->stats.verificationOK = 0;
    Ddi_Bdd_t *myInvar =
      Fsm_ReduceTerminalScc(fsmMgr, &opt->pre.terminalSccNum,
      &opt->stats.verificationOK, opt->trav.itpConstrLevel);

    if (opt->stats.verificationOK == 1) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Verification Result: PASS.\n"));
      Ddi_Free(myInvar);

      //      Fsm_MgrQuit(fsmMgr);
      goto freeMem1;
    }
    if (myInvar != NULL && opt->trav.itpConstrLevel > 0) {
      FbvConstrainInitStub(fsmMgr, myInvar);

      if (invar == NULL) {
        invar = myInvar;
      } else {
        Ddi_BddAndAcc(invar, myInvar);
        Ddi_BddAndAcc(Fsm_MgrReadConstraintBDD(fsmMgr),myInvar);
        Ddi_Free(myInvar);
      }
    } else {
      Ddi_Free(myInvar);
    }

  }

  if (opt->pre.impliedConstr) {
    Ddi_Bdd_t *myInvar = Fsm_ReduceImpliedConstr(fsmMgr,
      opt->pre.specDecompIndex, invar, &opt->pre.impliedConstrNum);

    if (myInvar != NULL) {
      if (invar == NULL) {
        invar = myInvar;
      } else {
        Ddi_BddAndAcc(invar, myInvar);
        Ddi_Free(myInvar);
      }
    }
  }

  FbvTestSec(fsmMgr, opt);
  FbvTestRelational(fsmMgr, opt);
  FbvSelectHeuristic(fsmMgr, opt, 1);

  lambda = Fsm_MgrReadLambdaBDD(fsmMgr);

  pi = Fsm_MgrReadVarI(fsmMgr);
  ps = Fsm_MgrReadVarPS(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);


  opt->stats.setupTime = util_cpu_time();

  if (opt->pre.reduceCustom) {
    if (invarspec != NULL) {
      Ddi_BddarrayWrite(lambda, 0, invarspec);
    }
    if (1) {
      Ddi_Bdd_t *myInvar = Fsm_ReduceCustom(fsmMgr);

      Ddi_Free(myInvar);

      FbvTestSec(fsmMgr, opt);
      FbvTestRelational(fsmMgr, opt);
    } else {
      Ddi_Var_t *v0 = Ddi_VarFromName(ddiMgr, "i2");
      Ddi_Var_t *v1 = Ddi_VarFromName(ddiMgr, "i78");
      Ddi_Var_t *v2 = Ddi_VarFromName(ddiMgr, "i6");
      Ddi_Var_t *v3 = Ddi_VarFromName(ddiMgr, "l4992");
      Ddi_Var_t *v4 = Ddi_VarFromName(ddiMgr, "l4994");
      Ddi_Bdd_t *l3 = Ddi_BddMakeLiteralAig(v3, 1);
      Ddi_Bdd_t *l4 = Ddi_BddMakeLiteralAig(v4, 1);
      Ddi_Bdd_t *a = Ddi_BddAnd(l3, l4);
      Ddi_Bdd_t *one = Ddi_BddMakeConstAig(ddiMgr, 1);
      Ddi_Bddarray_t *subst = Ddi_BddarrayAlloc(ddiMgr, 0);
      Ddi_Vararray_t *vars = Ddi_VararrayAlloc(ddiMgr, 0);
      Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
      Ddi_Bddarray_t *d2 = NULL;

      //      Ddi_Bddarray_t *d2 = Ddi_BddarrayDup(delta);

      if (0) {
        Ddi_VararrayInsertLast(vars, v0);
        Ddi_BddarrayInsertLast(subst, one);

        Ddi_BddarrayComposeAcc(delta, vars, subst);

        Ddi_VararrayWrite(vars, 0, v1);
        Ddi_BddarrayWrite(subst, 0, one);

        Ddi_BddarrayComposeAcc(delta, vars, subst);
      }
      Ddi_VararrayInsertLast(vars, v0);
      Ddi_BddarrayInsertLast(subst, one);
      //      Ddi_VararrayInsertLast(vars,v1);
      // Ddi_BddarrayInsertLast(subst,one);

      //      Ddi_BddarrayComposeAcc(delta,vars,subst);

      Ddi_VararrayInsertLast(vars, v2);
      Ddi_BddarrayInsertLast(subst, a);

      Ddi_BddarrayComposeAcc(delta, vars, subst);
      if (d2 != NULL) {
        Ddi_BddarrayComposeAcc(d2, vars, subst);

        for (int k = 0; k < Ddi_BddarrayNum(delta); k++) {
          Ddi_Bdd_t *d0 = Ddi_BddarrayRead(delta, k);
          Ddi_Bdd_t *d1 = Ddi_BddarrayRead(d2, k);

          Pdtutil_Assert(Ddi_BddEqualSat(d0, d1), "NOT equal delta");
          //      Pdtutil_Assert(Ddi_BddToBaig(d0)==Ddi_BddToBaig(d1),"Error");
        }
        Ddi_DataCopy(delta, d2);
        Ddi_Free(d2);
      }

      Ddi_Free(one);
      Ddi_Free(l4);
      Ddi_Free(l3);
      Ddi_Free(subst);
      Ddi_Free(vars);
      Ddi_Free(a);
    }
    // Ddi_Free(init);
    // init = Ddi_BddDup(Fsm_MgrReadInitBDD (fsmMgr));
    lambda = Fsm_MgrReadLambdaBDD(fsmMgr);

    if (invarspec != NULL) {
      Ddi_DataCopy(invarspec, Ddi_BddarrayRead(lambda, 0));
    }
    if (0) {
      int i, n;
      Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(pi);

      delta = Fsm_MgrReadDeltaBDD(fsmMgr);

      n = Ddi_BddarrayNum(delta);
      for (i = 0; i < n; i++) {
        Ddi_Bdd_t *p_i = Ddi_BddarrayRead(delta, i);

        if (Ddi_BddSize(p_i) == 1) {
          Ddi_Var_t *v = Ddi_BddTopVar(p_i);

          logBdd(p_i);
          if (Ddi_VarInVarset(piv, v)) {
            Ddi_Varset_t *s_i;

            p_i = Ddi_BddarrayExtract(delta, i);
            s_i = Ddi_BddarraySupp(delta);
            Ddi_BddarrayInsert(delta, i, p_i);
            if (Ddi_VarsetIsVoid(s_i)) {
              Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
                Pdtutil_VerbLevelNone_c,
                fprintf(stdout, "Free latch found.\n"));
            } else {
              int j;

              for (j = 0; j < n; j++) {
                if (j != i) {
                  Ddi_Bdd_t *p_j = Ddi_BddarrayRead(delta, j);
                  Ddi_Varset_t *s_j = Ddi_BddSupp(p_j);

                  if (Ddi_VarInVarset(s_j, v)) {
                    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
                      Pdtutil_VerbLevelNone_c, fprintf(stdout, "Found.\n"));
                  }
                  Ddi_Free(s_j);
                }
              }
            }
            Ddi_Free(s_i);
            Ddi_Free(p_i);
          }
        }
      }

      Ddi_Free(piv);
    }
  }

  if (0) {
    int nc = 0, ni = 0, i;

    for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);

      if (Ddi_BddIsConstant(d_i)) {
        nc++;
      } else if (Ddi_BddSize(d_i) == 1) {
        Ddi_Var_t *v = Ddi_BddTopVar(d_i);

        if (v == Ddi_VararrayRead(ps, i) && !Ddi_BddIsComplement(d_i)) {
          int cph = 2;
          Ddi_Bdd_t *d_i_1 = Ddi_BddDup(d_i);
          Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);

          ni++;
          printf("IDLE reg: %s\n", Ddi_VarName(v));

          if (initStub != NULL) {
            Ddi_BddComposeAcc(d_i, ps, initStub);
          } else {
            Ddi_BddAndAcc(d_i_1, init);
          }

          if (!Ddi_AigSat(d_i_1)) {
            cph = 0;
          } else if (!Ddi_AigSat(Ddi_BddNotAcc(d_i_1))) {
            cph = 1;
          }

          if (cph == 2) {
            Ddi_Varset_t *s_i, *s;
            Ddi_Bdd_t *one = Ddi_BddMakeConstAig(ddiMgr, 1);
            Ddi_Bdd_t *save = Ddi_BddDup(Ddi_BddarrayRead(initStub, i));

            Ddi_BddarrayWrite(initStub, i, one);
            s_i = Ddi_BddSupp(save);
            s = Ddi_BddarraySupp(initStub);
            Ddi_VarsetIntersectAcc(s_i, s);
            if (Ddi_VarsetIsVoid(s_i)) {
              printf("set at init: %s\n", Ddi_VarName(v));
            } else {
              printf("idle related: %s\n", Ddi_VarName(v));
            }
            Ddi_Free(s_i);
            Ddi_Free(s);
            Ddi_Free(one);
            Ddi_BddarrayWrite(initStub, i, save);
            Ddi_Free(save);
          }

        }
      }
    }
    printf("constant regs: nc=%d - idle regs=%d\n", nc, ni);
  }

  Ddi_Free(invar);
  if (Fsm_MgrReadConstraintBDD(fsmMgr)!=NULL) 
    invar = Ddi_BddDup(Fsm_MgrReadConstraintBDD(fsmMgr));

  opt->stats.setupTime = util_cpu_time();

  if (1 && opt->pre.clkVarFound != NULL) {
    int i;
    int n0 = 0, n1 = 0, nd = Ddi_BddarrayNum(delta);

    for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
      Ddi_Bdd_t *d_i_0, *d_i_1, *d_i = Ddi_BddarrayRead(delta, i);

      d_i_0 = Ddi_BddCofactor(d_i, opt->pre.clkVarFound, 0);
      d_i_1 = Ddi_BddCofactor(d_i, opt->pre.clkVarFound, 1);
      if (Ddi_BddSize(d_i_0) == 1) {
        Ddi_Var_t *v = Ddi_BddTopVar(d_i_0);

        if (v != opt->pre.clkVarFound && v == Ddi_VararrayRead(ps, i)) {
          n0++;
        }
      }
      if (Ddi_BddSize(d_i_1) == 1) {
        Ddi_Var_t *v = Ddi_BddTopVar(d_i_1);

        if (v != opt->pre.clkVarFound && v == Ddi_VararrayRead(ps, i)) {
          n1++;
        }
      }
      Ddi_Free(d_i_0);
      Ddi_Free(d_i_1);
    }
    Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "clocked regs: n1=%d, n0=%d\n", n0, n1));
    if (n1 > nd / 10 || n0 > nd / 10) {
      if (n1 > n0 * 5) {
        opt->pre.clkPhaseSuggested = 1;
      } else if (n0 > n1 * 5) {
        opt->pre.clkPhaseSuggested = 0;
      }
    }
  }

  sprintf(phaseSuffix, "%d", twoPhaseVarIndex);
  if (((opt->pre.twoPhaseRed && opt->pre.clkVarFound != NULL) ||
      opt->pre.twoPhaseForced)) {
    int ph = -1;
    int nv = Ddi_VararrayNum(pi);
    int useBddVars = nv < 4000;
    int size0 = Ddi_BddarraySize(delta);
    Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
    Ddi_Bdd_t *phase, *init = Fsm_MgrReadInitBDD(fsmMgr);
    Ddi_Bddarray_t *twoPhaseDelta = Ddi_BddarrayDup(delta);

    Ddi_Vararray_t *twoPhasePis = Ddi_BddarrayNewVarsAcc(twoPhaseDelta, pi,
      "PDT_ITP_TWOPHASE_PI", phaseSuffix, useBddVars);
    Ddi_Bddarray_t *twoPhaseDelta2 = Ddi_BddarrayDup(delta);

    Ddi_BddarrayComposeAcc(twoPhaseDelta, ps, twoPhaseDelta2);

    if (opt->pre.clkVarFound != NULL && opt->pre.twoPhaseRed) {
      phase = Ddi_BddMakeLiteralAig(opt->pre.clkVarFound, 1);
      if (initStub != NULL) {
        Ddi_BddComposeAcc(phase, ps, initStub);
      } else {
        Ddi_BddAndAcc(phase, init);
      }

      if (!Ddi_AigSat(phase)) {
        ph = 0;
      } else if (!Ddi_AigSat(Ddi_BddNotAcc(phase))) {
        ph = 1;
      }
      
      if (Fsm_MgrReadConstraintBDD(fsmMgr)) {
	opt->pre.clkPhaseSuggested = ph;
      }      
					  
      if (ph >= 0 && opt->pre.clkPhaseSuggested >= 0
        && ph != opt->pre.clkPhaseSuggested) {
        ph = opt->pre.clkPhaseSuggested;
        opt->pre.twoPhaseRedPlus = 1;
      }
      Ddi_Free(phase);
      if (ph >= 0 && opt->pre.twoPhaseOther) {
        opt->pre.twoPhaseRedPlus = !opt->pre.twoPhaseRedPlus;
        ph = !ph;
      }
    }

    opt->stats.setupTime = util_cpu_time();

    if (opt->pre.twoPhaseRedPlus && initStub != NULL) {
      Ddi_Bddarray_t *initStubPlus = Ddi_BddarrayDup(delta);
      Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
      char suffix[4];
      Ddi_Vararray_t *newPiVars;
      Ddi_Bddarray_t *newPiLits;
      int nv = Ddi_VararrayNum(pi);
      int useBddVars = nv < 4000;

      sprintf(suffix, "%d", fsmMgr->stats.initStubSteps++);
      newPiVars = useBddVars ?
        Ddi_VararrayMakeNewVars(pi, "PDT_STUB_PI", suffix, 1) :
        Ddi_VararrayMakeNewAigVars(pi, "PDT_STUB_PI", suffix);
      newPiLits = Ddi_BddarrayMakeLiteralsAig(newPiVars, 1);
      Ddi_BddarrayComposeAcc(initStubPlus, pi, newPiLits);
      Ddi_BddarrayComposeAcc(initStubPlus, ps, initStub);
      Ddi_Free(newPiVars);
      Ddi_Free(newPiLits);
      Fsm_MgrSetInitStubBDD(fsmMgr, initStubPlus);
      initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
      Ddi_Free(initStubPlus);
    }

    Ddi_DataCopy(delta, twoPhaseDelta);
    opt->pre.twoPhaseDone = 1;

    Fsm_MgrSetPhaseAbstr(fsmMgr, 1);
    Pdtutil_WresSetTwoPhase();

    Ddi_VararrayAppend(pi, twoPhasePis);

    if (opt->pre.clkVarFound != NULL) {
      if (1 && (ph >= 0)) {
        //      Ddi_Var_t *vp = Ddi_VarFromName(ddiMgr, "signal38396");
        int i;

        for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
          Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);

          Ddi_BddCofactorAcc(d_i, opt->pre.clkVarFound, ph);
          //      if (vp != NULL) Ddi_BddCofactorAcc(d_i,vp,0);
        }
      }
    }

    Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "Two phase reduction: %d -> %d (ph: %d)\n",
        size0, Ddi_BddarraySize(delta), ph));

    Ddi_Free(twoPhasePis);
    Ddi_Free(twoPhaseDelta);
    Ddi_Free(twoPhaseDelta2);
  }

  if (opt->pre.performAbcOpt == 2) {
    int ret = Fsm_MgrAbcReduce(fsmMgr, 0.9);
  }

  if (0 && forceDeltaConstraint)
  {
    Fsm_Fsm_t *fsmFsm = Fsm_FsmMakeFromFsmMgr(fsmMgr);
    Fsm_FsmDeltaWithConstraint(fsmFsm);
    Fsm_FsmWriteToFsmMgr(fsmMgr, fsmFsm);
    Fsm_FsmFree(fsmFsm);
  }

  if (opt->trav.hintsFile != NULL) {
    opt->trav.hints = ManualHints(ddiMgr, opt->trav.hintsFile);
  }

  travMgrAig = Trav_MgrInit(opt->expName, ddiMgr);
  FbvSetTravMgrOpt(travMgrAig, opt);
  FbvSetTravMgrFsmData(travMgrAig, fsmMgr);

  opt->stats.setupTime = util_cpu_time();

  if (opt->trav.abstrRefLoad != NULL) {
    Ddi_Vararray_t *refinedVars = NULL;
    if (strcmp(opt->trav.abstrRefLoad,"cuts")==0) {
      refinedVars = Ddi_VararrayAlloc(ddiMgr, 0);
      for (int i=0; i<Ddi_VararrayNum(ps); i++) {
        Ddi_Var_t *v = Ddi_VararrayRead(ps,i);
        char *search = "retime_CUT";
        if (strncmp(Ddi_VarName(v),search,strlen(search))==0)
          Ddi_VararrayInsertLast(refinedVars,v);
        else {
          search = "PDT_BDD_INVAR";
          if (strncmp(Ddi_VarName(v),search,strlen(search))==0)
            Ddi_VararrayInsertLast(refinedVars,v);
        }
      }
    }
    else {
      refinedVars = Ddi_VararrayLoad (ddiMgr, opt->trav.abstrRefLoad, NULL);
    }
    if (refinedVars!=NULL) {
      Ddi_Varsetarray_t *abstrRefRefinedVars = Ddi_VarsetarrayAlloc(ddiMgr, 2);
      Ddi_Varset_t *rv = Ddi_VarsetMakeFromArray(refinedVars);
      Ddi_VarsetarrayWrite(abstrRefRefinedVars,0,rv);
      Ddi_VarsetarrayWrite(abstrRefRefinedVars,1,NULL);
      Pdtutil_VerbosityMgrIf(ddiMgr, Pdtutil_VerbLevelUsrMax_c) {
        fprintf(dMgrO(ddiMgr),
              "Abstr Ref: loaded %d refined vars\n", Ddi_VarsetNum(rv));
      }
      Trav_MgrSetAbstrRefRefinedVars(travMgrAig, abstrRefRefinedVars);
      Ddi_Free(abstrRefRefinedVars);
      Ddi_Free(rv);
      Ddi_Free(refinedVars);
    }
  }
  
  {
  char *constrFile = NULL;
  int useAsConstr = 0;
  if (opt->trav.rPlus != NULL) {
    constrFile = opt->trav.rPlus;
  }
  else if (opt->trav.invarFile != NULL) {
    constrFile = opt->trav.invarFile;
    useAsConstr = 1;
  }
  if (constrFile != NULL) {
    char *pbench;
    Ddi_Var_t *v = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
    Ddi_Var_t *v1 = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelNone_c,
                           fprintf(stdout, "-- Reading REACHED set from %s.\n", constrFile));
    if ((pbench = strstr(constrFile, ".bench")) != NULL) {
      //      *pbench = '\0';
      care = Ddi_AigNetLoadBench(ddiMgr, constrFile, NULL);
      if (v != NULL) {
        Ddi_BddCofactorAcc(care, v, 1);
      }
      if (v1 != NULL) {
        Ddi_BddCofactorAcc(care, v1, 1);
      }
      Ddi_BddSetAig(care);
      //      care = Ddi_BddarrayExtract(cArray,0);
    } else if ((pbench = strstr(constrFile, ".aig")) != NULL) {
      Ddi_Bddarray_t *rplus =
        Ddi_AigarrayNetLoadAiger(ddiMgr, NULL, constrFile);
      Ddi_Bddarray_t *nsLits = Ddi_BddarrayMakeLiteralsAig(ns, 1);
      Ddi_BddarraySubstVarsAcc(rplus,Fsm_MgrReadVarNS(fsmMgr),
                          Fsm_MgrReadVarPS(fsmMgr));
      if (ibmConstr) {
        Ddi_Var_t *v = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
        Ddi_Var_t *v1 = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
        Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
        Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
        int iConstr = -1, i = 0;
        int nstate = Ddi_BddarrayNum(delta);
        Ddi_Bdd_t *deltaConstr = NULL;
        Ddi_Bdd_t *myRP = Ddi_BddDup(Ddi_BddarrayRead(rplus,
            Ddi_BddarrayNum(rplus) - 1));
        Ddi_Bdd_t *myRPPart;

        Ddi_BddCofactorAcc(myRP, v1, 1);
        for (i = 0; i < nstate; i++) {
          Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);

          if (v_i == v1) {
            iConstr = i;
            deltaConstr = Ddi_BddarrayRead(delta, i);
          }
        }
        Pdtutil_Assert(deltaConstr != NULL, "missing deltaConstr");
        Ddi_BddSetPartConj(deltaConstr);
        myRPPart = Ddi_AigPartitionTop(myRP, 0);
        for (i = 0; i < Ddi_BddPartNum(myRPPart); i++) {
          Ddi_BddPartInsertLast(deltaConstr, Ddi_BddPartRead(myRPPart, i));
        }
        Ddi_BddSetAig(deltaConstr); //DV

        Ddi_Free(myRP);
        Ddi_Free(myRPPart);
      }
      //end ibmConstr
      Ddi_Bdd_t *myInvar = Fsm_MgrReadConstraintBDD(fsmMgr);
      Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
      Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
      int iConstr = Ddi_BddarrayNum(delta)-2;
      Ddi_Bdd_t *deltaConstr = Ddi_BddarrayRead(delta,iConstr);
      if (useAsConstr) {
	Ddi_BddAndAcc(myInvar, Ddi_BddarrayRead(rplus,0));
	Ddi_BddAndAcc(deltaConstr, Ddi_BddarrayRead(rplus,0));
	if (invar!=NULL)
	  Ddi_Free(invar);
	invar = Ddi_BddDup(Fsm_MgrReadConstraintBDD(fsmMgr));
      }
      else {
	Fsm_MgrSetReachedBDD(fsmMgr,Ddi_BddarrayRead(rplus,0));
	Trav_MgrSetReached(travMgrAig,Ddi_BddarrayRead(rplus,0));
      }
      Ddi_Free(rplus);
      Ddi_Free(nsLits);
    } else {
      //NO AIG
      Ddi_Var_t *v = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
      Ddi_Var_t *v1 = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

      care = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
        DDDMP_MODE_DEFAULT, constrFile, NULL);
      if (v != NULL) {
        Ddi_BddCofactorAcc(care, v, 1);
      }
      if (v1 != NULL) {
        Ddi_BddCofactorAcc(care, v1, 1);
      }
      Ddi_BddExistProjectAcc(care, psvars);
      Ddi_MgrReduceHeap(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"), 0);
      Ddi_BddSetAig(care);
    }

    Ddi_MgrSetSiftThresh(ddiMgr, opt->ddi.siftTh);
  }
  }
  
  if (opt->trav.rPlusRings != NULL) {
    char *pbench;
    Ddi_Var_t *v = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
    Ddi_Var_t *v1 = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "-- Reading REACHED rings from %s.\n", opt->trav.rPlusRings));
    if ((pbench = strstr(opt->trav.rPlusRings, ".aig")) != NULL) {
      Ddi_Bddarray_t *rplus =
        Ddi_AigarrayNetLoadAiger(ddiMgr, NULL, opt->trav.rPlusRings);
      Ddi_Bddarray_t *nsLits = Ddi_BddarrayMakeLiteralsAig(ns, 1);
      Ddi_BddarraySubstVarsAcc(rplus,Fsm_MgrReadVarNS(fsmMgr),
                          Fsm_MgrReadVarPS(fsmMgr));
      if (Ddi_BddarrayNum(rplus)>1) {
	Ddi_BddarrayWrite(rplus, 0, Fsm_MgrReadInitBDD(fsmMgr));
	Ddi_BddarraySubstVarsAcc(rplus, ps, ns);
	Trav_MgrSetNewi(travMgrAig, rplus);
      }
      else {
	Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
			       Pdtutil_VerbLevelNone_c,
			       fprintf(stdout,
			       "-- Just one ring present - ignored\n"));
      }
      Ddi_Free(rplus);
      Ddi_Free(nsLits);
    } else {
      //NO AIG
      Ddi_Var_t *v = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
      Ddi_Var_t *v1 = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

      care = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
        DDDMP_MODE_DEFAULT, opt->trav.rPlus, NULL);
      if (v != NULL) {
        Ddi_BddCofactorAcc(care, v, 1);
      }
      if (v1 != NULL) {
        Ddi_BddCofactorAcc(care, v1, 1);
      }
      Ddi_BddExistProjectAcc(care, psvars);
      Ddi_MgrReduceHeap(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"), 0);
      Ddi_BddSetAig(care);
    }

    Ddi_MgrSetSiftThresh(ddiMgr, opt->ddi.siftTh);
  }

  //NO rPlus
  elapsedTime = util_cpu_time();

  if (!doFastRun) {

    if (opt->trav.lemmasCare && !opt->pre.retime && care == NULL) {
      care = Ddi_BddMakeConstAig(ddiMgr, 1);
    }


    loopReduce = 1;
    for (loopReduceCnt = 0; loopReduce && loopReduceCnt <= 4; loopReduceCnt++) {
      int innerLoop, loopCnt, nl = Ddi_BddarrayNum(delta);;
      loopReduce = 0;

      Fsm_MgrSetInvarspecBDD(fsmMgr, invarspec);

      if (1 && opt->pre.doCoi >= 1) {

        Ddi_Varsetarray_t *coi;
        int nCoi;

        coi = computeFsmCoiVars(fsmMgr, invarspec, NULL, opt->tr.coiLimit, 1);
        if (opt->pre.wrCoi != NULL) {
          Ddi_Varsetarray_t *coiRings;
          coiRings = computeFsmCoiRings(fsmMgr, invarspec, NULL, opt->tr.coiLimit, 1);
          writeCoiShells(opt->pre.wrCoi, coiRings, ps);
          Ddi_Free(coiRings);
        }
        nCoi = Ddi_VarsetarrayNum(coi);
        fsmCoiReduction(fsmMgr, Ddi_VarsetarrayRead(coi, nCoi - 1));

        Ddi_Free(coi);
      }

      loopCnt = 0;
      do {

        if (opt->pre.ternarySim == 0) {
          innerLoop = 0;
        } else {
          innerLoop = fsmConstReduction(fsmMgr, loopCnt == 0 && nl < 7500,
            -1, opt, 0);
        }

        loopCnt++;

        if (innerLoop < 0) {
          /* property proved */
          opt->stats.verificationOK = 1;

          Fsm_MgrQuit(fsmMgr);
          goto freeMem1;
        }

        loopReduce |= (innerLoop > 2);

        if (1 && opt->pre.doCoi >= 1) {

          Ddi_Varsetarray_t *coi;
          int nCoi;

          coi =
            computeFsmCoiVars(fsmMgr, invarspec, NULL, opt->tr.coiLimit, 1);
          if (opt->pre.wrCoi != NULL) {
            writeCoiShells(opt->pre.wrCoi, coi, ps);
          }
          nCoi = Ddi_VarsetarrayNum(coi);
          fsmCoiReduction(fsmMgr, Ddi_VarsetarrayRead(coi, nCoi - 1));

          Ddi_Free(coi);
        }

      } while (innerLoop && loopCnt <= 1);


      init = Fsm_MgrReadInitBDD(fsmMgr);

      Trav_MgrSetOption(travMgrAig, Pdt_TravLemmasTimeLimit_c, fnum,
        opt->trav.lemmasTimeLimit / 4);

      if (opt->pre.retime) {

        int size0 = Ddi_BddarraySize(Fsm_MgrReadDeltaBDD(fsmMgr));

        loopReduce |= Fsm_RetimeMinreg(fsmMgr, care, opt->pre.retime);
        if (Ddi_BddarraySize(Fsm_MgrReadDeltaBDD(fsmMgr)) > size0 * 1.05) {
          opt->pre.retime = 0;
        }
        //    exit (1);
      }

      if (1 && opt->mc.cegar == 0 && (loopReduceCnt < 2) && opt->mc.lemmas > 1) {
        // GpC: must fix invar resulting from lemmas
         int result = Trav_TravLemmaReduction(travMgrAig,
          fsmMgr, init, invarspec, invar, care,
          opt->mc.lemmas > 1 ? opt->mc.lemmas / 2 : opt->mc.lemmas,
          opt->trav.implLemmas, opt->trav.strongLemmas, NULL);

        //      loopReduce = 1;
        if (result != 0) {
          Fsm_MgrQuit(fsmMgr);
          opt->stats.verificationOK = result > 0;
          goto freeMem1;
        }
      }

      Trav_MgrSetOption(travMgrAig, Pdt_TravLemmasTimeLimit_c, fnum,
        opt->trav.lemmasTimeLimit);
    }
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout,
        "Circuit AIG stats: %d inp / %d state vars / %d AIG gates.\n",
        Ddi_VararrayNum(Fsm_MgrReadVarI(fsmMgr)),
        Ddi_VararrayNum(Fsm_MgrReadVarPS(fsmMgr)),
        Ddi_BddarraySize(Fsm_MgrReadDeltaBDD(fsmMgr))));

    elapsedTime = util_cpu_time() - elapsedTime;

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Reduction run time: %.1f.\n", elapsedTime / 1000.0));


    if (opt->mc.cegar > 0 && opt->mc.itpSeq == 0) {
      Fsm_ReduceCegar(fsmMgr, invarspec, opt->mc.cegar, opt->mc.cegarPba);
    }

  }

  if (opt->trav.targetEn != 0) {
    Ddi_Bdd_t *r;
    int nIter = opt->trav.targetEn;
#if 0
    FbvTargetEn(fsmMgr,nIter);
#else
    int iProp = Ddi_BddarrayNum(Fsm_MgrReadDeltaBDD(fsmMgr)) - 1;

    opt->trav.targetEn = 0;
    Trav_MgrSetOption(travMgrAig, Pdt_TravTargetEn_c, inum,
      opt->trav.targetEn);
    r =
      Trav_TravSatBckVerif(travMgrAig, fsmMgr, init, invarspec, invar, NULL,
      nIter, 0, 0);
    Ddi_BddNotAcc(r);
    Ddi_BddarrayWrite(Fsm_MgrReadDeltaBDD(fsmMgr), iProp, r);
    Ddi_Free(r);
#endif
    //    stallEnable = 1;
  }

  opt->stats.setupTime = util_cpu_time();

  /* end common part */

  if (opt->pre.targetDecomp > 0) {
    Fsm_MgrSetInvarspecBDD(fsmMgr, NULL);
    Fbv_DecompInfo_t *decompInfo = FbvPropDecomp(fsmMgr, opt);
    Ddi_Bdd_t *pPart = Ddi_BddMakePartConjFromArray(decompInfo->pArray);
    Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
    int iprop = Ddi_BddarrayNum(delta)-1;
    Ddi_Bdd_t *p0 = Ddi_BddMakeAig(Ddi_BddPartRead(pPart,0));
    Fsm_MgrSetInvarspecBDD(fsmMgr, p0);
    Ddi_BddarrayWrite(delta,iprop, p0);
    Ddi_Free(decompInfo->pArray);
    Ddi_Free(decompInfo->coiArray);
    Ddi_Free(decompInfo->hintVars);
    Pdtutil_Free(decompInfo->partOrdId);
    Pdtutil_Free(decompInfo);
    Ddi_Free(pPart);
    Ddi_Free(p0);
  }
#if 0
  saveDeltaCOIs(fsmMgr, opt->pre.createClusterFile, opt->pre.clusterFile,
    opt->pre.methodCluster, opt->pre.thresholdCluster);
#endif

  if (0 && opt->mc.lemmas > 0) {
    int k;
    for (k=1; k<=opt->mc.lemmas; k *= 2) {
      Fsm_MgrAbcScorr(fsmMgr, NULL, NULL, NULL, k);
    }
    opt->mc.lemmas = 0;
  }

  
  if (doXorDetection) {
    Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
    Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
    Ddi_Bddarray_t *cuts = Ddi_AigarrayFindXors(delta,ps,0);
    Ddi_Free(cuts);
  }
  if (doXorCuts) {
    InsertCutLatches(fsmMgr,1,doXorCuts);
  }
  
  if (opt->mc.aig && !doFastRun) {

    Ddi_Bddarray_t *lambda = Fsm_MgrReadLambdaBDD(fsmMgr);

    //    Ddi_Bdd_t *care = NULL;

    Ddi_MgrSetAigBddOptLevel(ddiMgr, opt->ddi.aigBddOpt);
    Ddi_MgrSetAigAbcOptLevel(ddiMgr, opt->common.aigAbcOpt);
    Ddi_MgrSetAigRedRemLevel(ddiMgr, opt->ddi.aigRedRem);
    Ddi_MgrSetAigRedRemMaxCnt(ddiMgr, opt->ddi.aigRedRemPeriod);
    Ddi_MgrSetAigSatTimeout(ddiMgr, opt->ddi.satTimeout);
    Ddi_MgrSetAigSatTimeLimit(ddiMgr, opt->ddi.satTimeLimit);
    Ddi_MgrSetAigSatIncremental(ddiMgr, opt->ddi.satIncremental);
    Ddi_MgrSetAigSatVarActivity(ddiMgr, opt->ddi.satVarActivity);
    Ddi_MgrSetAigCnfLevel(ddiMgr, opt->ddi.aigCnfLevel);

    if (opt->trav.fbvMeta && opt->trav.metaMethod == Ddi_Meta_Size) {
      Ddi_MetaInit(ddiMgr, opt->trav.metaMethod, NULL, 0, NULL, NULL,
        opt->trav.fbvMeta);
      Ddi_MgrSetMetaMaxSmooth(ddiMgr, opt->common.metaMaxSmooth);
      opt->trav.fbvMeta = 1;
    }

    init = Ddi_BddDup(Fsm_MgrReadInitBDD(fsmMgr));

    Ddi_MgrSetAigLazyRate(ddiMgr, opt->common.lazyRate);

    if (Ddi_BddIsMono(invarspec)) {
      Ddi_Bdd_t *newI = Ddi_AigMakeFromBdd(invarspec);

      Ddi_Free(invarspec);
      invarspec = newI;
    }

    if (opt->mc.ilambdaCare >= 0) {
      care = Ddi_BddDup(Ddi_BddarrayRead(lambda, opt->mc.ilambdaCare));
      Ddi_BddExistProjectAcc(care, psvars);
    }

    if (opt->mc.gfp > 0) {
      Trav_TravSatItpGfp(travMgrAig,fsmMgr,opt->mc.gfp,
                         opt->trav.countReached);
    }
    
    if (opt->trav.trAbstrItp > 0 &&
        (opt->mc.bmc>0 || Trav_MgrReadNewi(travMgrAig) != NULL ||
         opt->trav.trAbstrItpLoad != NULL || opt->trav.trAbstrItpOpt != 0)) {
      Trav_TravTrAbstrItp(travMgrAig,fsmMgr,
                          opt->trav.trAbstrItp,
                          opt->trav.trAbstrItpOpt,
                          opt->trav.trAbstrItpFirstFwdStep,
                          opt->trav.trAbstrItpMaxFwdStep,
                          opt->mc.bmc
                          );
      exit(0);
    }
    
    if (opt->mc.checkInv != NULL) {
      Ddi_Bddarray_t *rplus = Ddi_AigarrayNetLoadAiger(ddiMgr,
                           NULL, opt->mc.checkInv);
      Pdtutil_Assert(Ddi_BddarrayNum(rplus)==1,"problem with invar");
      Ddi_Bdd_t *myInvar = Ddi_BddarrayExtract(rplus, 0);
      Ddi_Free(rplus);
      int chk, fp;
      fp = Trav_TravSatCheckInvar(travMgrAig,fsmMgr,
                                  myInvar,&chk,NULL);
      Ddi_Free(myInvar);
      if (fp && !chk) {
        Fsm_MgrSetConstraintBDD(fsmMgr, myInvar);
      }
      if (opt->mc.exit_after_checkInv) exit(0); 
    }
    
    if (opt->mc.wFsm != NULL) {

      //      Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
      //      Ddi_AigarraySift(delta);


      if (opt->trav.noCheck) {
        Ddi_Free(invarspec);
        invarspec = NULL;
      }

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Writing FSM to file %s.\n", opt->mc.wFsm));

      if (0 && (invarspec = Fsm_MgrReadInvarspecBDD(fsmMgr)) != NULL) {
        if (Ddi_BddIsPartConj(invarspec)) {
          Ddi_Bddarray_t *iArray = Ddi_BddarrayMakeFromBddPart(invarspec);

          Fsm_MgrSetLambdaBDD(fsmMgr, iArray);
          Ddi_Free(iArray);
          Fsm_MgrSetInvarspecBDD(fsmMgr, NULL);
        }
      }
      Fsm_Fsm_t *fsmFsm = Fsm_FsmMakeFromFsmMgr(fsmMgr);

      while (Ddi_BddarrayNum(fsmFsm->lambda) > 1) {
	Ddi_BddarrayRemove(fsmFsm->lambda,
			   Ddi_BddarrayNum(fsmFsm->lambda) - 1);
      }
      fsmFsm->size.o = 1;

      if (!opt->mc.fold) {
        Fsm_FsmUnfoldProperty(fsmFsm, 1);
        Fsm_FsmUnfoldConstraint(fsmFsm);
      }
      else {
        Fsm_FsmWriteConstraint(fsmFsm,NULL);
      }            

      if (opt->fsm.nnf) {
        Fsm_FsmNnfEncoding(fsmFsm);
      }

      if (opt->trav.targetEnAppr > 0) {
	Fsm_Fsm_t *fsm2 = Fsm_FsmTargetEnAppr(fsmFsm,opt->trav.targetEnAppr);
       	Fsm_FsmFree(fsmFsm);
        fsmFsm = fsm2;
      }

      if (Fsm_MgrReadInitStubBDD(fsmMgr) != NULL || opt->mc.foldInit) {
        Fsm_FsmFoldInit(fsmFsm);
      }
      //      Fsm_FsmFoldConstraint(fsmFsm);


      int doReduceEnable = 0;
      if (doReduceEnable && opt->mc.wFsm != NULL) {
	//	Fsm_Fsm_t *fsmFsm = Fsm_FsmMakeFromFsmMgr(fsmMgr);
	Fsm_Fsm_t *fsm2 = Fsm_FsmReduceEnable(fsmFsm,NULL);
       	Fsm_FsmFree(fsmFsm);
        fsmFsm = fsm2;
	//	Fsm_FsmMiniWriteAiger(fsmFsm, opt->mc.wFsm);
	// exit(1); 
      }


      Fsm_FsmMiniWriteAiger(fsmFsm, opt->mc.wFsm);

      exit(1);
    }


    if (0 && opt->trav.fbvMeta && opt->trav.metaMethod == Ddi_Meta_Size) {
      Ddi_MetaInit(ddiMgr, opt->trav.metaMethod, NULL, 0, NULL, NULL,
        opt->trav.fbvMeta);
      Ddi_MgrSetMetaMaxSmooth(ddiMgr, opt->common.metaMaxSmooth);
    }



    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Invarspec: %d.\n", Ddi_BddSize(invarspec));
      fprintf(stdout, "Init: %d.\n", Ddi_BddSize(init)));

    if (opt->mc.inductive >= 0) {
#if 1
      if (opt->mc.lemmas > 0) {
        opt->mc.inductive = opt->mc.lemmas;
      }
      if (opt->mc.lemmas == 0) {
        opt->mc.lemmas = 100000;
        Trav_MgrSetOption(travMgrAig, Pdt_TravLemmasTimeLimit_c, fnum, -1);
        int result = Trav_TravLemmaReduction(travMgrAig,
          fsmMgr, init, invarspec, invar, care, opt->mc.lemmas,
          opt->trav.implLemmas, 3, NULL);

        if (result != 0) {
          Fsm_MgrQuit(fsmMgr);
          opt->stats.verificationOK = result > 0;
          goto freeMem;
        }
      }
      Trav_TravSatInductiveTrVerif(travMgrAig, fsmMgr,
        init, invarspec, invar, care, opt->mc.inductive, -1, -1, -1);
#else
      FbvAigInductiveTrVerif(fsmMgr, init, invarspec, invar,
        opt->mc.inductive);
#endif
    } else if (opt->mc.itpSeq != 0) {
      Trav_TravSatItpSeqVerif(travMgrAig, fsmMgr,
        init, invarspec, invar, opt->mc.itpSeq, opt->mc.itpSeqGroup,
        opt->mc.itpSeqSerial, opt->mc.cegar, opt->mc.fwdBwdFP);
    } else if (opt->mc.pdr > 0) {
      printf("PDR\n");
      Trav_MgrSetOption(travMgrAig, Pdt_TravTargetEn_c, inum,
        opt->trav.targetEn);
      Trav_TravPdrVerif(travMgrAig, fsmMgr, init, invarspec, invar, care, -1,
        -1);
      if ((Fsm_MgrReadCexBDD(fsmMgr) != NULL)) {
        int res = checkFsmCex(fsmMgr, travMgrAig);

        Pdtutil_Assert(res, "cex validation failure");
        FbvWriteCex(opt->mc.wRes, fsmMgr, fsmMgrOriginal);
      }
    } else if (opt->mc.igrpdr > 0) {
      printf("IGRPDR\n");
      Trav_MgrSetOption(travMgrAig, Pdt_TravTargetEn_c, inum,
        opt->trav.targetEn);
      Trav_TravIgrPdrVerif(travMgrAig, fsmMgr, opt->mc.decompTimeLimit);
      if ((Fsm_MgrReadCexBDD(fsmMgr) != NULL)) {
        int res = checkFsmCex(fsmMgr, travMgrAig);

        Pdtutil_Assert(res, "cex validation failure");
        FbvWriteCex(opt->mc.wRes, fsmMgr, fsmMgrOriginal);
      }
    } else if (opt->mc.lazy >= 0) {
      Trav_MgrSetOption(travMgrAig, Pdt_TravTargetEn_c, inum,
        opt->trav.targetEn);
      Trav_TravSatItpVerif(travMgrAig, fsmMgr, init, invarspec, NULL, invar, care,
        opt->mc.lemmas , -1,
        opt->common.lazyRate, -1, -1);
      if ((Fsm_MgrReadCexBDD(fsmMgr) != NULL)) {
        int res = checkFsmCex(fsmMgr, travMgrAig);

        Pdtutil_Assert(res, "cex validation failure");
        FbvWriteCex(opt->mc.wRes, fsmMgr, fsmMgrOriginal);
      }
    } else if (opt->mc.qbf > 0) {
      Trav_TravSatQbfVerif(travMgrAig, fsmMgr,
        init, invarspec, invar, care, opt->mc.diameter, -1,
        opt->common.lazyRate);
    } else if (opt->mc.diameter >= 0) {
      Trav_MgrSetOption(travMgrAig, Pdt_TravTargetEn_c, inum,
        opt->trav.targetEn);
      Trav_TravSatDiameterVerif(travMgrAig, fsmMgr, init, invarspec, invar,
        opt->mc.lemmas, opt->mc.diameter);
    } else if (opt->trav.bmcTr >= 0) {
      Trav_TravSatBmcTrVerif(travMgrAig, fsmMgr, init, invarspec, invar,
        opt->trav.bmcTr, opt->trav.bmcStep);

      /* FbvAigBmcTrVerif(fsmMgr,init,invarspec,invar,opt->trav.bmcTr,opt->trav.bmcStep); */

    } else if (opt->trav.bmcTr < -1) {
      opt->trav.bmcTr = -(opt->trav.bmcTr);
      Trav_TravSatBmcTrVerif_ForwardInsert(travMgrAig, fsmMgr, init, invarspec,
        invar, opt->trav.bmcTr, opt->trav.bmcStep);

    }

    else if (opt->mc.bmc >= 0) {
#if 0
      FbvAigBmcVerif(fsmMgr, init, invarspec, invar, opt->mc.bmc,
        opt->trav.bmcStep);
#else
      Trav_MgrSetOption(travMgrAig, Pdt_TravTargetEn_c, inum,
        opt->trav.targetEn);
      Trav_MgrSetSatSolver(travMgrAig, opt->common.satSolver);
      Ddi_MgrSetSatSolver(ddiMgr, opt->common.satSolver);
      if (opt->trav.bmcStrategy >= 1) {
        int chkCex = 1;

        Trav_TravSatBmcIncrVerif(travMgrAig, fsmMgr,
          init, invarspec, invar, NULL, opt->mc.bmc, opt->trav.bmcStep,
          opt->trav.bmcLearnStep, opt->trav.bmcFirst, 1 /*doCex */ ,
          opt->mc.lemmas, opt->trav.interpolantBmcSteps,
				 opt->trav.bmcStrategy, opt->trav.bmcTe, -1);

        if (!Fsm_CexNormalize(fsmMgr)) {
          if (Fsm_MgrReadRemovedPis(fsmMgr)) {
            Ddi_Bdd_t *cex = Fsm_MgrReadCexBDD(fsmMgr);
	    int ret2;
            Fsm_MgrSetCexBDD(fsmMgrOriginal, cex);
            Trav_TravSatBmcRefineCex(fsmMgrOriginal,
              Fsm_MgrReadInitBDD(fsmMgrOriginal),
              Fsm_MgrReadInvarspecBDD(fsmMgrOriginal),
              Fsm_MgrReadConstraintBDD(fsmMgrOriginal),
              Fsm_MgrReadCexBDD(fsmMgrOriginal));
	    ret2 = Fsm_CexNormalize(fsmMgrOriginal);
	    Pdtutil_Assert(!ret2,"error extending cex");
            FbvWriteCexNormalized(opt->mc.wRes, fsmMgrOriginal,
                                  fsmMgrOriginal,opt->mc.ilambda);
          } else {
            FbvWriteCexNormalized(opt->mc.wRes, fsmMgr, fsmMgrOriginal,opt->mc.ilambda);
          }
          fprintf(stdout, "\ncex written to: %s.cex\n", opt->mc.wRes);
        }
#if 0
        if ((Fsm_MgrReadCexBDD(fsmMgr) != NULL)) {
          int res = checkFsmCex(fsmMgr, travMgrAig);

          Pdtutil_Assert(res, "cex validation failure");
	  //     FbvWriteCex(opt->mc.wRes, fsmMgr, fsmMgrOriginal);
        }
#else
        if ((Fsm_MgrReadCexBDD(fsmMgr) != NULL)) {
	  char fullCexName[1000];
	  sprintf(fullCexName,"%s.cex",opt->mc.wRes);
	  int invalidCex = Fsm_AigsimCex(opt->expName, fullCexName);

	  if (invalidCex) fprintf(stdout,"CEX NOT validated by aigsim !!!\n");
	  else 
	  fprintf(stdout, "CEX validated by aigsim\n");
	}
#endif
      } else {
        Trav_TravSatBmcVerif(travMgrAig, fsmMgr,
          init, invarspec, invar, opt->mc.bmc, opt->trav.bmcStep,
          opt->trav.bmcFirst, opt->trav.apprAig, opt->mc.lemmas,
          opt->trav.interpolantBmcSteps, -1);
      }
#endif
    } else if (opt->mc.method == Mc_VerifExactFwd_c) {
      Trav_MgrSetOption(travMgrAig, Pdt_TravTargetEn_c, inum,
        opt->trav.targetEn);
      if (opt->common.clk != NULL) {
        Trav_MgrSetClk(travMgrAig, opt->common.clk);
      }
      if (opt->trav.apprAig) {
        Ddi_Bdd_t *r =
          Trav_TravSatFwdApprox(travMgrAig, fsmMgr, init, invarspec, invar);
        Ddi_Free(r);
      } else {
        if (opt->mc.interpolant > 0) {
          Trav_TravSatFwdInterpolantVerif(travMgrAig, fsmMgr,
            init, invarspec, invar, care, opt->trav.noCheck, opt->mc.initBound,
            opt->mc.lemmas);
        } else if (opt->mc.exactTrav) {
          Trav_TravSatFwdExactVerif(travMgrAig, fsmMgr,
            init, invarspec, invar, care, opt->trav.noCheck, opt->mc.initBound,
            opt->mc.lemmas, 4, -1);
        } else {
          Trav_TravSatFwdVerif(travMgrAig, fsmMgr,
            init, invarspec, invar, care, opt->trav.noCheck, opt->mc.initBound,
            opt->mc.lemmas, 4, -1);
        }
      }
    } else if (opt->mc.method == Mc_VerifTravFwd_c) {
      invarspec = Ddi_BddMakeConstAig(ddiMgr, 1);
      FbvAigFwdVerif(fsmMgr, init, invarspec, invar, opt);
      Ddi_Free(invarspec);
    } else if (opt->mc.aig == 2) {
      Ddi_Bdd_t *r;

#if 1
      Trav_MgrSetOption(travMgrAig, Pdt_TravTargetEn_c, inum,
        opt->trav.targetEn);
      if (opt->common.clk != NULL) {
        Trav_MgrSetClk(travMgrAig, opt->common.clk);
      }
      r = Trav_TravSatBckAllSolVerif(travMgrAig, fsmMgr, init, invarspec,
        invar, -1);
#else
      r = FbvAigBckVerif2(fsmMgr, init, invarspec, invar);
#endif
      Ddi_Free(r);
    } else if (opt->mc.interpolant > 0) {
      Trav_MgrSetOption(travMgrAig, Pdt_TravTargetEn_c, inum,
        opt->trav.targetEn);
      if (opt->common.clk != NULL) {
        Trav_MgrSetClk(travMgrAig, opt->common.clk);
      }
      Trav_TravSatBwdInterpolantVerif(travMgrAig, fsmMgr,
        init, invarspec, invar, care, opt->trav.noCheck, opt->mc.initBound,
        opt->mc.lemmas);
    } else {
      Ddi_Bdd_t *r;

#if 1
      if (opt->common.clk != NULL) {
        Trav_MgrSetClk(travMgrAig, opt->common.clk);
      }
      r = Trav_TravSatBckVerif(travMgrAig,
        fsmMgr, init, invarspec, invar, care, -1, opt->trav.apprAig,
        opt->mc.lemmas);
#else
      r = FbvAigBckVerif(fsmMgr, init, invarspec, invar, opt->trav.targetEn);
#endif
      Ddi_Free(r);
    }

    if (opt->trav.checkProof) {
      char name[1000];
      strcpy(name,fsm);
      char *s = strstr(name,".aig");
      if (s!=NULL) {
        strcpy(s,"-with-proof-inv.aig");
      }
      else  {
        strcat(name,"-with-proof-inv.aig");
      }      
      int ok = Trav_TravSatCheckProof(travMgrAig, fsmMgr, fsmMgrOriginal, name);
    }
    
    if (opt->trav.writeProof!=NULL) {
      int ok = Trav_TravSatStoreProofAiger(travMgrAig, fsmMgr, fsmMgrOriginal, opt->trav.writeProof);
    }
    
    if (opt->trav.writeInvar != NULL) {
      Ddi_Bdd_t *r = Trav_MgrReadReached(travMgrAig);
      if (r!=NULL) {
        if (opt->verbosity >= Pdtutil_VerbLevelUsrMax_c) {
          printf("Writing invar to %s\n", opt->trav.writeInvar);
        }
        Ddi_Bdd_t *rUnfolded = Ddi_BddDup(r);
        Ddi_Var_t *pVar = Fsm_MgrReadPdtSpecVar(fsmMgr);
        Ddi_Var_t *cVar = Fsm_MgrReadPdtConstrVar(fsmMgr);
        if (pVar!=NULL) Ddi_BddCofactorAcc(rUnfolded,pVar,1);
        if (cVar!=NULL) Ddi_BddCofactorAcc(rUnfolded,cVar,1);
        Ddi_AigNetStoreAiger(rUnfolded,0,opt->trav.writeInvar);
        Ddi_Free(rUnfolded);
      }
    }

    if (opt->trav.wR != NULL) {
      Ddi_Bdd_t *r = Trav_MgrReadReached(travMgrAig);
      if (r!=NULL) {
        if (opt->verbosity >= Pdtutil_VerbLevelUsrMax_c) {
          printf("Writing itp reached to %s\n", opt->trav.wR);
        }
        Ddi_Bdd_t *rUnfolded = Ddi_BddDup(r);
        Ddi_Var_t *pVar = Fsm_MgrReadPdtSpecVar(fsmMgr);
        Ddi_Var_t *cVar = Fsm_MgrReadPdtConstrVar(fsmMgr);
        if (pVar!=NULL) Ddi_BddCofactorAcc(rUnfolded,pVar,1);
        if (cVar!=NULL) Ddi_BddCofactorAcc(rUnfolded,cVar,1);
        Ddi_AigNetStoreAiger(rUnfolded,0,opt->trav.wR);
        Ddi_Free(rUnfolded);
        if (Trav_MgrReadNewi(travMgrAig) != NULL) {
          Ddi_Bddarray_t *rings = Ddi_BddarrayDup(
                                    Trav_MgrReadNewi(travMgrAig));
          char ringsfile[1000];
          strcpy(ringsfile, opt->trav.wR);
          char *s = strstr(ringsfile,".aig");
          if (s==NULL) {
            strcat(ringsfile,"_rings");
          }
          else {
            sprintf(s,"%s", "_rings.aig");
          }
          if (opt->verbosity >= Pdtutil_VerbLevelUsrMax_c) {
            printf("Writing itp fromRings to %s\n",ringsfile);
          }
          int nRings = travMgrAig->settings.ints.igrRingLast+1;
          while (Ddi_BddarrayNum(rings)>nRings) {
            Ddi_BddarrayRemove(rings,Ddi_BddarrayNum(rings)-1);
          }
          Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
          Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
          Ddi_Var_t *pvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
          Ddi_Var_t *cvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
          Ddi_BddarrayWrite(rings, 0,
                              Fsm_MgrReadInitBDD(fsmMgr));
          Ddi_BddarraySubstVarsAcc(rings,ns,ps);
          Ddi_BddarrayCofactorAcc(rings, cvarPs, 1);
          Ddi_BddarrayCofactorAcc(rings, pvarPs, 1);
          for (int k = 0; k < Ddi_BddarrayNum(rings); k++) {
            Ddi_Bdd_t *rA = Ddi_BddarrayRead(rings, k);
            Ddi_BddSetAig(rA);
          }
          Ddi_AigarrayNetStoreAiger(rings, 0, ringsfile);
          Ddi_Free(rings);
        }

      }
    }

    if (opt->trav.wC != NULL) {
      Ddi_Bdd_t *c = Fsm_MgrReadConstraintBDD(fsmMgr);
      if (c!=NULL) {
        if (opt->verbosity >= Pdtutil_VerbLevelUsrMax_c) {
          printf("Writing constr to %s\n", opt->trav.wC);
        }
        Ddi_Bdd_t *cUnfolded = Ddi_BddDup(c);
        Ddi_Var_t *pVar = Fsm_MgrReadPdtSpecVar(fsmMgr);
        Ddi_Var_t *cVar = Fsm_MgrReadPdtConstrVar(fsmMgr);
        if (pVar!=NULL) Ddi_BddCofactorAcc(cUnfolded,pVar,1);
        if (cVar!=NULL) Ddi_BddCofactorAcc(cUnfolded,cVar,1);
        Ddi_AigNetStoreAiger(cUnfolded,0,opt->trav.wR);
        Ddi_Free(cUnfolded);
      }
    }
    
    opt->stats.verificationOK = !Trav_MgrReadAssertFlag(travMgrAig);

    Ddi_Free(care);

    Trav_MgrQuit(travMgrAig);

    goto freeMem;
    return opt->stats.verificationOK;
  } else {
    if (0 && opt->pre.peripheralLatches) {  /* BDDs */
      int nRed;

      do {
        nRed = Fsm_RetimePeriferalLatches(fsmMgr);
      } while (nRed > 0);
    }
  }

  if (stallEnable && Fsm_MgrReadInitStubBDD(fsmMgr) == NULL) {
    int i;
    Ddi_Var_t *v = Ddi_VarNew(ddiMgr);
    Ddi_Bdd_t *c_i;
    Ddi_Bddarray_t *pA;
    Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
    Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
    Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
    Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
    Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity(ddiMgr);

    pA = Ddi_BddarrayRangeMakeFromCube(init, ps);

    Pdtutil_VerbosityLocalIf(verbosity, Pdtutil_VerbLevelUsrMax_c) {
      fprintf(dMgrO(ddiMgr),
        "Modifying FSM to add init -> init tra\nsition.\n", i);
    }

    Ddi_VarAttachName(v, "PDT_STALL_DUMMY_CONTROL");
    Ddi_VararrayInsertLast(pi, v);
    c_i = Ddi_BddMakeLiteralAig(v, 1);
    for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
      Ddi_Bdd_t *d_i, *p_i;

      p_i = Ddi_BddarrayRead(pA, i);
      d_i = Ddi_BddIte(c_i, Ddi_BddarrayRead(delta, i), p_i);
      Ddi_BddarrayWrite(delta, i, d_i);
      Ddi_Free(d_i);
    }
    Ddi_Free(pA);
    Ddi_Free(c_i);
    // NO: because affects backward use
    //    Fsm_MgrSetDeltaBDD (fsmMgr,delta);
  }

  /* CONVERT from AIG to BDD */

  Fsm_MgrSetCutThresh(fsmMgr, opt->fsm.cut);
  Fsm_MgrSetOption(fsmMgr, Pdt_FsmCutAtAuxVar_c, inum, opt->fsm.cutAtAuxVar);


  Fsm_MgrSetInvarspecBDD(fsmMgr, invarspec);
  Ddi_Free(invarspec);

  if (0) {
    Fsm_Fsm_t *fsmFsm = Fsm_FsmMakeFromFsmMgr(fsmMgr);

    if (Fsm_MgrReadInitStubBDD(fsmMgr) != NULL) {
      Fsm_FsmFoldInit(fsmFsm);
    }
    Fsm_FsmWriteToFsmMgr(fsmMgr, fsmFsm);
    Fsm_FsmFree(fsmFsm);
  }


  deltaAig = Ddi_BddarrayDup(Fsm_MgrReadDeltaBDD(fsmMgr));

  Fsm_MgrAigToBdd(fsmMgr);

  invarspec = Ddi_BddDup(Fsm_MgrReadInvarspecBDD(fsmMgr));

  if (invar != NULL) {
    Ddi_BddSetMono(invar);
  }
  if (care != NULL) {
    Ddi_BddSetMono(care);
    Ddi_BddExistProjectAcc(care, psvars);
  }
  init = Ddi_BddDup(Fsm_MgrReadInitBDD(fsmMgr));


  if (0) {
    Ddi_Bdd_t *myOne = Ddi_BddMakeConst(ddiMgr, 1);
    int nd = Ddi_BddarrayNum(Fsm_MgrReadDeltaBDD(fsmMgr));

    Ddi_Free(invarspec);
    invarspec =
      Ddi_BddDup(Ddi_BddarrayRead(Fsm_MgrReadDeltaBDD(fsmMgr), nd - 1));
    Ddi_BddarrayWrite(Fsm_MgrReadDeltaBDD(fsmMgr), nd - 1, myOne);
    Ddi_Free(myOne);
  }

  /**********************************************************************/
  /*                       handle invariant                             */
  /**********************************************************************/

  //  invarspec = getSpec (fsmMgr,tr,invar);
  //  invarspec = getSpec (fsmMgr,tr,NULL);

  pi = Fsm_MgrReadVarI(fsmMgr);
  ps = Fsm_MgrReadVarPS(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);

  if (opt->trav.fbvMeta && opt->trav.metaMethod == Ddi_Meta_Size) {
    Ddi_MetaInit(ddiMgr, opt->trav.metaMethod, NULL, 0, NULL, NULL,
      opt->trav.fbvMeta);
    Ddi_MgrSetMetaMaxSmooth(ddiMgr, opt->common.metaMaxSmooth);
    opt->trav.fbvMeta = 1;
  }

  /**********************************************************************/
  /*                       generate TR manager                          */
  /**********************************************************************/

  trMgr = Tr_MgrInit("TR-manager", ddiMgr);

  if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
    ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
    Tr_MgrSetVerbosity(trMgr, Pdtutil_VerbLevel_e(opt->verbosity));
  }

  /* Iwls95 sorting method */
  if (strcmp(opt->tr.trSort, "iwls95") == 0) {
    Tr_MgrSetSortMethod(trMgr, Tr_SortWeight_c);
  } else if (strcmp(opt->tr.trSort, "pdt") == 0) {
    Tr_MgrSetSortMethod(trMgr, Tr_SortWeightPdt_c);
  } else if (strcmp(opt->tr.trSort, "ord") == 0) {
    Tr_MgrSetSortMethod(trMgr, Tr_SortOrd_c);
  } else if (strcmp(opt->tr.trSort, "size") == 0) {
    Tr_MgrSetSortMethod(trMgr, Tr_SortSize_c);
  } else if (strcmp(opt->tr.trSort, "minmax") == 0) {
    Tr_MgrSetSortMethod(trMgr, Tr_SortMinMax_c);
  } else if (strcmp(opt->tr.trSort, "none") == 0) {
    Tr_MgrSetSortMethod(trMgr, Tr_SortNone_c);
  } else {
    Tr_MgrSetSortMethod(trMgr, Tr_SortNone_c);
  }

#if 0
  Tr_MgrSetSortW(trMgr, 1, 2);
  Tr_MgrSetSortW(trMgr, 2, 1);
  Tr_MgrSetSortW(trMgr, 3, 1);
  Tr_MgrSetSortW(trMgr, 4, 2);
  Tr_MgrSetSortW(trMgr, 5, 2);
#endif
  /* enable smoothing PIs while clustering */
  Tr_MgrSetClustSmoothPi(trMgr, 0);
  Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
  Tr_MgrSetImgClustThreshold(trMgr, opt->trav.imgClustTh);
  Tr_MgrSetImgApproxMinSize(trMgr, opt->tr.approxImgMin);
  Tr_MgrSetImgApproxMaxSize(trMgr, opt->tr.approxImgMax);
  Tr_MgrSetImgApproxMaxIter(trMgr, opt->tr.approxImgIter);

  Tr_MgrSetI(trMgr, pi);
  Tr_MgrSetPS(trMgr, ps);
  Tr_MgrSetNS(trMgr, ns);

  if (Fsm_MgrReadTrBDD(fsmMgr) == NULL) {
    tr = Tr_TrMakePartConjFromFunsWithAuxVars(trMgr, delta, ns,
      Fsm_MgrReadAuxVarBDD(fsmMgr), Fsm_MgrReadVarAuxVar(fsmMgr));
  } else {
    tr = Tr_TrMakeFromRel(trMgr, Fsm_MgrReadTrBDD(fsmMgr));
  }


  if (opt->common.checkFrozen) {
    Tr_MgrSetCheckFrozenVars(trMgr);
  }

  if (opt->mc.checkDead) {
    Ddi_Varset_t *smooth = Ddi_VarsetMakeFromArray(pi);

    Ddi_VarsetUnionAcc(smooth, nsvars);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "-- Checking deadlock.\n"));
    noDead = Ddi_BddExist(Tr_TrBdd(tr), smooth);
    Ddi_Free(smooth);
  }
#if 0
  if (opt->mc.rInit != NULL) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "-- Reading INIT set from %s ****\n", opt->mc.rInit));
    Ddi_Free(init);
    init = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
      DDDMP_MODE_DEFAULT, opt->mc.rInit, NULL);
    Ddi_MgrReduceHeap(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"), 0);
    Ddi_MgrSetSiftThresh(ddiMgr, opt->ddi.siftTh);
  }
  if (!opt->trav.noInit) {
    Ddi_Free(init);
    init = Ddi_BddDup(Fsm_MgrReadInitBDD(fsmMgr));
  }
#endif

#if 0
  WindowPart(Ddi_BddarrayRead(delta, 0, opt));
#endif

  if (opt->mc.checkDead) {
    Ddi_BddOrAcc(invarspec, noDead);
    Ddi_Free(noDead);
  }

  if (opt->tr.invar_in_tr) {
    Ddi_Free(invar);
    invar = Ddi_BddarrayRead(delta, 0);
    Ddi_BddPartInsert(Tr_TrBdd(tr), 0, invar);
  } else {
    int i;

    if (opt->trav.invarFile != NULL) {
      Ddi_Free(invar);
      if (strstr(opt->trav.invarFile, ".bdd") != NULL) {
        invar = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
          DDDMP_MODE_DEFAULT, opt->trav.invarFile, NULL);
      } else if (strstr(opt->trav.invarFile, ".aig") != NULL) {
        Ddi_Bddarray_t *rplus = Ddi_AigarrayNetLoadAiger(ddiMgr,
          NULL, opt->trav.invarFile);

        invar = Ddi_BddarrayExtract(rplus, 0);
        Ddi_Free(rplus);
      } else {
        Ddi_Expr_t *e1, *e = Ddi_ExprLoad(ddiMgr, opt->trav.invarFile, NULL);

        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Done\n-- Reading invariant.\n"));
        e1 = Expr_EvalBddLeafs(e);
        invar = Expr_InvarMakeFromAG(e1);
        Ddi_Free(e1);
        Ddi_Free(e);
      }
    }

    for (i = 0; i < Ddi_BddarrayNum(lambda); i++) {
      char *name = Ddi_ReadName(Ddi_BddarrayRead(lambda, i));

      if (name != NULL && strcmp(name, "invar0") == 0) {
        Ddi_Free(invar);
        invar = Ddi_BddDup(Ddi_BddarrayRead(lambda, i));
        break;
      }
    }

  }

#if 1
  if (Fsm_MgrReadVarAuxVar(fsmMgr) != NULL) {
    Ddi_Vararray_t *auxvArray = Fsm_MgrReadVarAuxVar(fsmMgr);

    opt->fsm.auxVarsUsed = 1;
    Ddi_VararrayAppend(pi, auxvArray);
    Tr_MgrSetAuxVars(trMgr, Fsm_MgrReadVarAuxVar(fsmMgr));
    Tr_MgrSetAuxFuns(trMgr, Fsm_MgrReadAuxVarBDD(fsmMgr));
  }
#else
  if (Fsm_MgrReadVarAuxVar(fsmMgr) != NULL) {
    Ddi_Vararray_t *auxPs, *auxNs;
    Ddi_Var_t *var;
    Ddi_Bdd_t *trBdd = Tr_TrBdd(tr);
    int i;
    Ddi_Varset_t *piSet = Ddi_VarsetMakeFromArray(pi);

    auxPs = Fsm_MgrReadVarAuxVar(fsmMgr);
    opt->trav.auxPsSet = Ddi_VarsetMakeFromArray(auxPs);


    auxNs = Ddi_VararrayAlloc(ddiMgr, Ddi_VararrayNum(auxPs));

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "\nWORKING with %d couples of ps/ns cut-point vars\n",
        Ddi_VararrayNum(auxPs)));

    for (i = 0; i < Ddi_VararrayNum(auxPs); i++) {
      int xlevel = Ddi_VarCurrPos(Ddi_VararrayRead(auxPs, i)) + 1;

      var = Ddi_VarNewAtLevel(ddiMgr, xlevel);
      Ddi_VararrayWrite(auxNs, i, var);
    }

    if (!opt->mc.nogroup) {
      Ddi_MgrCreateGroups2(ddiMgr, auxPs, auxNs);
    }

    Ddi_VararrayAppend(ps, auxPs);
    Ddi_VararrayAppend(ns, auxNs);

    Ddi_BddSwapVarsAcc(trBdd, auxPs, auxNs);
    for (i = 0; i < Ddi_VararrayNum(auxPs); i += 2) {
      Ddi_Bdd_t *part = Ddi_BddDup(Ddi_BddPartRead(trBdd, i));

      Ddi_BddSwapVarsAcc(part, auxPs, auxNs);
      Ddi_BddPartInsert(trBdd, i, part);
      Ddi_BddAndAcc(init, part);
      Ddi_Free(part);
    }

    Ddi_BddExistAcc(init, piSet);

    Ddi_Free(auxNs);
    Ddi_Free(piSet);
  }
#endif

  Tr_MgrSetI(trMgr, pi);
  Tr_MgrSetPS(trMgr, ps);
  Tr_MgrSetNS(trMgr, ns);

#if 0
  if (opt->trav.fbvMeta) {
    Ddi_MetaInit(ddiMgr, opt->trav.metaMethod, NULL, 0, NULL, NULL, 10);
  }
#endif

  if (opt->pre.coisort) {
    Ddi_Varsetarray_t *coivars;
    int i, j, nl = Ddi_VararrayNum(ps);
    int *coisize = Pdtutil_Alloc(int, nl
    );
    int *latchIds = Pdtutil_Alloc(int, nl
    );
    int *lambdaIds;
    Ddi_Bdd_t *lit, *r;

    if (opt->trav.rPlus != NULL) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "-- Reading REACHED set from %s.\n", opt->trav.rPlus));
      r = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
        DDDMP_MODE_DEFAULT, opt->trav.rPlus, NULL);
      Ddi_BddSetMono(r);
    }

    opt->pre.coiSets = Pdtutil_Alloc(Ddi_Varset_t *, nl);
    for (i = 0; i < nl; i++) {
      opt->pre.coiSets[i] = NULL;
    }

    for (i = 0; i < Ddi_BddPartNum(Tr_TrBdd(tr)); i++) {
      Ddi_BddSuppAttach(Ddi_BddPartRead(Tr_TrBdd(tr), i));
    }

    for (i = 0; i < nl; i++) {
      lit = Ddi_BddMakeLiteral(Ddi_VararrayRead(ps, i), 0);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "%s - ", Ddi_VarName(Ddi_VararrayRead(ps, i))));
      if ((opt->trav.rPlus != NULL) && Ddi_BddIncluded(r, lit)) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
          Pdtutil_VerbLevelNone_c, fprintf(stdout, "P0 Droppoed.\n"));
      } else {
        int n;

        coivars = computeCoiVars(tr, lit, 0);
        n = Ddi_VarsetarrayNum(coivars);
        opt->pre.coiSets[i] =
          Ddi_VarsetDup(Ddi_VarsetarrayRead(coivars, n - 1));
        coisize[i] = Ddi_VarsetNum(Ddi_VarsetarrayRead(coivars, n - 1));
        Ddi_Free(coivars);
      }
      latchIds[i] = i;
      Ddi_Free(lit);
    }

    for (i = 0; i < nl - 1; i++) {
      for (j = i; j < nl; j++) {
        if (coisize[latchIds[i]] > coisize[latchIds[j]]) {
          int tmp = latchIds[i];

          latchIds[i] = latchIds[j];
          latchIds[j] = tmp;
        }
      }
    }

    for (i = 0; i < nl; i++) {
      j = latchIds[i];
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "L[%d] COI: %6d - name: %s\n", j, coisize[j],
          Ddi_VarName(Ddi_VararrayRead(ps, j))));
    }

    for (i = 0; i < nl; i++) {
      Ddi_Free(opt->pre.coiSets[i]);
    }
    Pdtutil_Free(opt->pre.coiSets);

    if (lambda == NULL)
      exit(1);

    nl = Ddi_BddarrayNum(lambda);
    coisize = Pdtutil_Alloc(int,
      nl
    );
    lambdaIds = Pdtutil_Alloc(int,
      nl
    );

    opt->pre.coiSets = Pdtutil_Alloc(Ddi_Varset_t *, nl);
    for (i = 0; i < nl; i++) {
      opt->pre.coiSets[i] = NULL;
    }


    for (i = 0; i < nl; i++) {
      int n;
      char *name;
      int dropped = 0;

      lambdaIds[i] = i;
      lit = Ddi_BddarrayRead(lambda, i);
      name = Ddi_ReadName(lit);
      if (name != NULL) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
          Pdtutil_VerbLevelNone_c, fprintf(stdout, "%s - ", name));
      }

      if ((opt->trav.rPlus != NULL) && Ddi_BddIncluded(r, lit)) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
          Pdtutil_VerbLevelNone_c, fprintf(stdout, "P1 Dropped.\n"));
        dropped++;
      }
      Ddi_BddNotAcc(lit);
      if ((opt->trav.rPlus != NULL) && Ddi_BddIncluded(r, lit)) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
          Pdtutil_VerbLevelNone_c, fprintf(stdout, "P0 Dropped.\n"));
        dropped++;
      }
      Ddi_BddNotAcc(lit);
      if (dropped >= 2)
        continue;

      coivars = computeCoiVars(tr, lit, 0);

      n = Ddi_VarsetarrayNum(coivars);
      opt->pre.coiSets[i] = Ddi_VarsetDup(Ddi_VarsetarrayRead(coivars, n - 1));
      coisize[i] = Ddi_VarsetNum(Ddi_VarsetarrayRead(coivars, n - 1));

      Ddi_Free(coivars);
    }

    for (i = 0; i < nl - 1; i++) {
      for (j = i; j < nl; j++) {
        if (coisize[lambdaIds[i]] > coisize[lambdaIds[j]]) {
          int tmp = lambdaIds[i];

          lambdaIds[i] = lambdaIds[j];
          lambdaIds[j] = tmp;
        }
      }
    }

    for (i = 0; i < nl; i++) {
      char *name;

      j = lambdaIds[i];
      lit = Ddi_BddarrayRead(lambda, j);
      name = Ddi_ReadName(lit);
      if (name != NULL) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Lambda[%d] COI: %6d - name: %s.\n",
            j, coisize[j], name));
      } else {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Lambda[%d] COI: %6d\n", j, coisize[j]));
      }

    }

    for (i = 0; i < nl; i++) {
      Ddi_Free(opt->pre.coiSets[i]);
    }


    for (i = 0; i < Ddi_BddPartNum(Tr_TrBdd(tr)); i++) {
      Ddi_BddSuppDetach(Ddi_BddPartRead(Tr_TrBdd(tr), i));
    }

    Ddi_Free(r);

    exit(1);
  }

  if (opt->pre.doCoi >= 1) {
    Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
    Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);

    if (opt->pre.wrCoi != NULL) {
      Ddi_Varsetarray_t *coi;

      coi = Tr_TrComputeCoiVars(tr, invarspec, opt->tr.coiLimit);
      writeCoiShells(opt->pre.wrCoi, coi, ps);
      Ddi_Free(coi);
    }
#if 1
    Tr_MgrSetCoiLimit(trMgr, opt->tr.coiLimit);
    Tr_TrCoiReduction(tr, invarspec);
    if (1 || opt->tr.coiLimit > 0) {

      Ddi_Varset_t *nsvars = Ddi_VarsetMakeFromArray(ns);

      Ddi_Varset_t *coiSupp = Ddi_BddSupp(Tr_TrBdd(tr));

      Ddi_VarsetIntersectAcc(coiSupp, nsvars);
      Ddi_VarsetSwapVarsAcc(coiSupp, ps, ns);

      Ddi_BddExistProjectAcc(init, coiSupp);
      Ddi_Free(nsvars);
      Ddi_Free(coiSupp);
    }
#else
    Ddi_Varsetarray_t *coi;
    Tr_Tr_t *reducedTR;
    int n;

    coi = computeCoiVars(tr, invarspec, opt->tr.coiLimit);
    n = Ddi_VarsetarrayNum(coi);
    if (opt->pre.wrCoi != NULL) {
      writeCoiShells(opt->pre.wrCoi, coi);
    }

    reducedTR = trCoiReduction(tr, Ddi_VarsetarrayRead(coi, n - 1), 0);

    Tr_TrFree(tr);
    tr = reducedTR;

    Ddi_BddExistProjectAcc(init, Ddi_VarsetarrayRead(coi, n - 1));

    Ddi_Free(coi);
#endif
  }
#if 1
  if (opt->trav.trproj) {
    Tr_Tr_t *newTr;

    newTr = FbvTrProjection(tr, invarspec);
    Tr_TrFree(tr);
    tr = newTr;
  }
#endif

  while (opt->pre.lambdaLatches) {
    Tr_RemoveLambdaLatches(tr);
    opt->pre.lambdaLatches--;
  }

  if (0 && opt->pre.findClk) {
    Ddi_Varset_t *clock_vars = FindClockInTr(tr);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "Set of clock vars: "));
    Ddi_VarsetPrint(clock_vars, 0, 0, stdout);
    Ddi_Free(clock_vars);
    exit(1);
  }

  if (opt->tr.manualTrDpartFile != NULL) {
    Tr_Tr_t *newTr =
      ManualTrDisjPartitions(tr, opt->tr.manualTrDpartFile, invar, opt);
    Tr_TrFree(tr);
    tr = newTr;
    //    Tr_TransClosure(tr);
  } else if (opt->trav.approxTrClustFile != NULL) {
    Tr_Tr_t *newTr = Tr_TrManualPartitions(tr, opt->trav.approxTrClustFile);

    Tr_TrFree(tr);
    tr = newTr;
  }

  if (opt->trav.autoHint > 0 && opt->trav.hintsStrategy == 0) {
    Ddi_Bdd_t *trBdd = Tr_TrBdd(tr);
    Ddi_Bdd_t *myHints;

    //    Ddi_Varset_t *psns = Ddi_VarsetUnion(psvars,nsvars);
    Ddi_Varset_t *vars = Ddi_VarsetMakeFromArray(pi);
    Ddi_Bdd_t *myOne = Ddi_BddMakeConst(ddiMgr, 1);

    Ddi_VarsetUnionAcc(vars, psvars);
    if (Tr_MgrReadAuxVars(trMgr) != NULL) {
      Ddi_Vararray_t *auxvArray = Tr_MgrReadAuxVars(trMgr);
      Ddi_Varset_t *auxSet = Ddi_VarsetMakeFromArray(auxvArray);

      Ddi_VarsetDiffAcc(vars, auxSet);
      Ddi_Free(auxSet);
    }
    myHints = Part_PartitionDisjSet(trBdd, init, vars,
      Part_MethodCofactorKernel_c,
      Ddi_BddSize(trBdd) / 100, opt->trav.autoHint, Pdtutil_VerbLevelDevMax_c);
    opt->trav.hints = Ddi_BddarrayMakeFromBddPart(myHints);
    Ddi_BddarrayInsertLast(opt->trav.hints, myOne);
    Ddi_Free(vars);
    Ddi_Free(myHints);
  }

  if (opt->mc.ilambdaCare >= 0) {
    care = Ddi_BddDup(Ddi_BddarrayRead(lambda, opt->mc.ilambdaCare));
    Ddi_BddExistProjectAcc(care, psvars);
  }

  FbvFsmCheckComposePi(fsmMgr, tr);

  Ddi_MgrSetSiftMaxThresh(ddiMgr, opt->ddi.siftMaxTh);
  Ddi_MgrSetSiftMaxCalls(ddiMgr, opt->ddi.siftMaxCalls);

  if (TRAV_BDD_VERIF) {
    travMgrBdd = Trav_MgrInit(opt->expName, ddiMgr);
    FbvSetTravMgrOpt(travMgrBdd, opt);
    //FbvSetTravMgrFsmData(travMgrBdd, fsmMgr);
    //Trav_MgrSetI(travMgrBdd, Tr_MgrReadI(trMgr));
    //Trav_MgrSetPS(travMgrBdd, Tr_MgrReadPS(trMgr));
    //Trav_MgrSetNS(travMgrBdd, Tr_MgrReadNS(trMgr));
    //Trav_MgrSetFrom(travMgrBdd, from);
    //Trav_MgrSetReached(travMgrBdd, from);
    //Trav_MgrSetTr(travMgrBdd, tr);
  }

  switch (opt->mc.method) {
    case Mc_VerifTravFwd_c:
      Ddi_Free(invarspec);
      invarspec = Ddi_BddMakeConst(ddiMgr, 1);
      if (travMgrBdd != NULL) {
        travMgrBdd->settings.bdd.bddTimeLimit = firstRunTimeLimit;
        travMgrBdd->settings.bdd.bddNodeLimit = firstRunNodeLimit;
        opt->stats.verificationOK =
          Trav_TravBddExactForwVerify(travMgrBdd, fsmMgr, tr, init, invarspec,
          invar, deltaAig);
      } else {
        opt->stats.verificationOK =
          FbvExactForwVerify(NULL, fsmMgr, tr, init, invarspec, invar,
          deltaAig, firstRunTimeLimit, firstRunNodeLimit, opt);
      }
      break;
    case Mc_VerifTravBwd_c:
      if (opt->trav.maxFwdIter < 0) {
        opt->trav.rPlus = "1";
      }
      if (travMgrBdd != NULL) {
        Trav_MgrSetOption(travMgrBdd, Pdt_TravRPlus_c, pchar, opt->trav.rPlus);
        opt->stats.verificationOK =
          Trav_TravBddApproxForwExactBckFixPointVerify(travMgrBdd, fsmMgr, tr,
          Ddi_MgrReadZero(ddiMgr), invarspec, invar, care, deltaAig);
      } else {
        opt->stats.verificationOK = FbvApproxForwExactBckFixPointVerify(fsmMgr,
          tr, Ddi_MgrReadZero(ddiMgr), invarspec, invar, care, deltaAig, opt);
      }
      break;
    case Mc_VerifExactFwd_c:
      if (travMgrBdd != NULL) {
        travMgrBdd->settings.bdd.bddTimeLimit = firstRunTimeLimit;
        travMgrBdd->settings.bdd.bddNodeLimit = firstRunNodeLimit;
        opt->stats.verificationOK =
          Trav_TravBddExactForwVerify(travMgrBdd, fsmMgr, tr, init, invarspec,
          invar, deltaAig);
      } else {
        opt->stats.verificationOK =
          FbvExactForwVerify(NULL, fsmMgr, tr, init, invarspec, invar,
          deltaAig, firstRunTimeLimit, firstRunNodeLimit, opt);
      }
      break;
    case Mc_VerifExactBck_c:
      if (opt->trav.maxFwdIter < 0) {
        opt->trav.rPlus = "1";
      }
      if (travMgrBdd != NULL) {
        Trav_MgrSetOption(travMgrBdd, Pdt_TravRPlus_c, pchar, opt->trav.rPlus);
        opt->stats.verificationOK =
          Trav_TravBddApproxForwExactBckFixPointVerify(travMgrBdd, fsmMgr, tr,
          init, invarspec, invar, care, deltaAig);
      } else {
        opt->stats.verificationOK = FbvApproxForwExactBckFixPointVerify(fsmMgr,
          tr, init, invarspec, invar, care, deltaAig, opt);
      }
      break;
    case Mc_VerifApproxBckExactFwd_c:
      if (travMgrBdd != NULL) {
        opt->stats.verificationOK =
          Trav_TravBddApproxBackwExactForwVerify(travMgrBdd, fsmMgr, tr,
          init, invarspec);
      } else {
        opt->stats.verificationOK =
          FbvApproxBackwExactForwVerify(tr, init, invarspec, opt);
      }
      break;
    case Mc_VerifApproxFwdExactBck_c:
      if (opt->trav.bound >= 0) {
#if 0
        if (travMgrBdd != NULL) {
          opt->stats.verificationOK =
            Trav_TravBddApproxForwExactBackwVerify(travMgrBdd, fsmMgr, tr,
            init, invarspec, deltaAig, 1);
        } else {
          opt->stats.verificationOK =
            FbvApproxForwExactBackwVerify(tr, init, invarspec, deltaAig, 1,
            opt);
        }
#else
        if (travMgrBdd != NULL) {
          opt->stats.verificationOK =
            Trav_TravBddApproxForwApproxBckExactForwVerify(travMgrBdd, fsmMgr,
            tr, init, invarspec, opt->pre.doCoi, 0);
        } else {
          opt->stats.verificationOK =
            FbvApproxForwApproxBckExactForwVerify(tr, init, invarspec, 0, opt);
        }
#endif
      } else {
        if (travMgrBdd != NULL) {
          opt->stats.verificationOK =
            Trav_TravBddApproxForwExactBckFixPointVerify(travMgrBdd, fsmMgr,
            tr, init, invarspec, invar, care, deltaAig);
        } else {
          opt->stats.verificationOK =
            FbvApproxForwExactBckFixPointVerify(fsmMgr, tr, init, invarspec,
            invar, care, deltaAig, opt);
        }
      }
      break;
    case Mc_VerifApproxFwdApproxBckExactFwd_c:
      if (travMgrBdd != NULL) {
        opt->stats.verificationOK =
          Trav_TravBddApproxForwApproxBckExactForwVerify(travMgrBdd, fsmMgr,
          tr, init, invarspec, opt->pre.doCoi, 1);
      } else {
        opt->stats.verificationOK =
          FbvApproxForwApproxBckExactForwVerify(tr, init, invarspec, 1, opt);
      }
      break;
    case Mc_VerifPrintNtkStats_c:
      break;
    case Mc_VerifMethodNone_c:
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c, fprintf(stderr, "Wrong Selection.\n"));
      exit(1);
      break;
  }

  if (travMgrBdd != NULL) {
    /* print stats */
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c, Pdtutil_VerbLevelNone_c,
      currTime = util_cpu_time();
      fprintf(stdout, "Live nodes (peak): %u(%u)\n",
        Ddi_MgrReadLiveNodes(ddiMgr), Ddi_MgrReadPeakLiveNodeCount(ddiMgr));
#if 0                           // NXR: fix time computations
      fprintf(stdout, "TotalTime: %s ", util_print_time(setupTime + travTime));
      fprintf(stdout, "(setup: %s - ", util_print_time(setupTime));
      fprintf(stdout, " trav: %s)\n", util_print_time(travTime));
#endif
      );
    Trav_MgrQuit(travMgrBdd);
  }

freeMem:

  Ddi_Free(init);

freeMem1:

  Ddi_Free(opt->trav.auxPsSet);

  Ddi_Free(deltaAig);
  Ddi_Free(care);
  Ddi_Free(invar);
  Ddi_Free(invarspec);
  Ddi_Free(psvars);
  Ddi_Free(nsvars);
  Ddi_Free(opt->trav.univQuantifyVars);
  Ddi_Free(opt->trav.hints);

  Tr_TrFree(tr);

  Tr_MgrQuit(trMgr);

  /* write ORD & FSM to file */

  if (opt->mc.wRes != NULL && opt->stats.verificationOK >= 0) {
    FILE *fp;

    fp = fopen(opt->mc.wRes, "w");
    if (fp != NULL) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Verification Result: %s.\n",
          opt->stats.verificationOK ? "PASS" : "FAIL");
        fprintf(stdout, "Result Written to File %s.\n", opt->mc.wRes)
        );
      fprintf(fp, "%d\n", opt->stats.verificationOK ? 0 : 1);
      fclose(fp);
    } else {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Error opening file %s.\n", opt->mc.wRes));
    }
  }

  if (opt->trav.wOrd != NULL) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Writing final variable order to file %s.\n",
        opt->trav.wOrd));
    Ddi_MgrOrdWrite(ddiMgr, opt->trav.wOrd, NULL,
      Pdtutil_VariableOrderDefault_c);
  }

  if (opt->mc.wFsm != NULL) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Writing FSM to file %s.\n", opt->mc.wFsm));

    Fsm_MgrStore(fsmMgr, opt->mc.wFsm, NULL, 1, DDDMP_MODE_DEFAULT,
      Pdtutil_VariableOrderDefault_c);

  }

  Fsm_MgrQuit(fsmMgr);
  if (fsmMgrOriginal != NULL)
    Fsm_MgrQuit(fsmMgrOriginal);

  if (Ddi_MgrCheckExtRef(ddiMgr, 0) == 0) {
    Ddi_MgrPrintExtRef(ddiMgr, 0);
  }

  if (opt->mc.method != Mc_VerifPrintNtkStats_c) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Total used memory: %.2f MBytes\n",
        ((float)Ddi_MgrReadMemoryInUse(ddiMgr)) / (2 << 20));
      fprintf(stdout, "Total Verification Time: %s\n",
        util_print_time(util_cpu_time() - startVerifTime))
      );
  }

  /**********************************************************************/
  /*                       Close DDI manager                            */
  /**********************************************************************/

  /*Ddi_MgrPrintStats(ddiMgr); */

  Ddi_MgrQuit(ddiMgr);

  /**********************************************************************/
  /*                             End test                               */
  /**********************************************************************/

  return opt->stats.verificationOK;
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
invarMixedVerif(
  char *fsm,
  char *ord,
  Fbv_Globals_t * opt
)
{
  Ddi_Mgr_t *ddiMgr;
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Varset_t *psvars, *nsvars;
  Tr_Mgr_t *trMgr = NULL;
  Trav_Mgr_t *travMgrBdd = NULL, *travMgrAig = NULL;
  Fsm_Mgr_t *fsmMgr = NULL, *fsmMgr2 = NULL;
  Tr_Tr_t *tr = NULL;
  Ddi_Bdd_t *init = NULL, *invarspec, *invar = NULL, *noDead, *care = NULL;
  Ddi_Bdd_t *invarspecBdd, *invarspecItp;
  Ddi_Bddarray_t *delta, *lambda;
  long elapsedTime, startVerifTime, currTime, travTime, setupTime;
  int num_ff, heuristic, verificationResult = -1, itpBoundLimit = -1;
  int loopReduce, loopReduceCnt, lastDdiNode, bddNodeLimit = -1;
  int deltaSize, deltaNum, bmcTimeLimit = -1, bddTimeLimit =
    -1, bddMemoryLimit = -1;
  int doRunBdd = 0, doRunBmc = 0, doRunInd = 0, doRunLms = 0, doRunItp =
    0, doRunRed = 0;
  int doRunLemmas = 0, bmcSteps, indSteps, lemmasSteps = 2, indTimeLimit = -1;
  int doRunPdr = 0, pdrTimeLimit = -1, itpTimeLimit = -1;


  /**********************************************************************/
  /*                        Create DDI manager                          */
  /**********************************************************************/

  opt->expt.expertLevel = 2;

  ddiMgr = Ddi_MgrInit("DDI_manager", NULL, 0,
    DDI_UNIQUE_SLOTS, DDI_CACHE_SLOTS * 10, 0, -1, -1);

  if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
    ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
    Ddi_MgrSetVerbosity(ddiMgr, Pdtutil_VerbLevel_e(opt->verbosity));
  }

  Ddi_MgrSiftEnable(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"));
  Ddi_MgrSetSiftThresh(ddiMgr, opt->ddi.siftTh);
  Ddi_MgrSetExistOptLevel(ddiMgr, opt->ddi.exist_opt_level);
  Ddi_MgrSetExistOptClustThresh(ddiMgr, opt->ddi.exist_opt_thresh);

  Ddi_MgrSetAigDynAbstrStrategy(ddiMgr, opt->ddi.dynAbstrStrategy);
  Ddi_MgrSetAigTernaryAbstrStrategy(ddiMgr, opt->ddi.ternaryAbstrStrategy);
  Ddi_MgrSetAigItpCore(ddiMgr, opt->ddi.itpCore);
  Ddi_MgrSetAigItpMem(ddiMgr, opt->ddi.itpMem);
  Ddi_MgrSetAigItpOpt(ddiMgr, opt->ddi.itpOpt);
  Ddi_MgrSetAigItpStore(ddiMgr, opt->ddi.itpStore);
  Ddi_MgrSetAigItpAbc(ddiMgr, opt->ddi.itpAbc);
  Ddi_MgrSetAigItpPartTh(ddiMgr, opt->ddi.itpPartTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpActiveVars_c, inum, opt->ddi.itpActiveVars);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStoreTh_c, inum, opt->ddi.itpStoreTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpAigCore_c, inum, opt->ddi.itpAigCore);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpTwice_c, inum, opt->ddi.itpTwice);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpIteOptTh_c, inum, opt->ddi.itpIteOptTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStructOdcTh_c, inum,
    opt->ddi.itpStructOdcTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiNnfClustSimplify_c, inum,
    opt->ddi.nnfClustSimplify);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMap_c, inum, opt->ddi.itpMap);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompute_c, inum, opt->ddi.itpCompute);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpLoad_c, pchar, opt->ddi.itpLoad);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpDrup_c, inum, opt->ddi.itpDrup);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompact_c, inum, opt->ddi.itpCompact);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpClust_c, inum, opt->ddi.itpClust);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpNorm_c, inum, opt->ddi.itpNorm);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpSimp_c, inum, opt->ddi.itpSimp);


  /**********************************************************************/
  /*                        Create FSM manager                          */
  /**********************************************************************/

  opt->stats.startTime = util_cpu_time();

  fsmMgr = Fsm_MgrInit("fsm", NULL);
  Fsm_MgrSetDdManager(fsmMgr, ddiMgr);
  FbvSetFsmMgrOpt(fsmMgr, opt, 0);

  if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
    ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
    Fsm_MgrSetVerbosity(fsmMgr, Pdtutil_VerbLevel_e(opt->verbosity));
  }

  Fsm_MgrSetUseAig(fsmMgr, 1);
  Fsm_MgrSetCutThresh(fsmMgr, opt->fsm.cut);
  Fsm_MgrSetOption(fsmMgr, Pdt_FsmCutAtAuxVar_c, inum, opt->fsm.cutAtAuxVar);

  /**********************************************************************/
  /*                        Read BLIF file                              */
  /**********************************************************************/

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "-- FSM Loading.\n"));

  if (strstr(fsm, ".fsm") != NULL) {
    if (strcmp(opt->mc.ord, "dfs") == 0) {
      Pdtutil_Free(opt->mc.ord);
      opt->mc.ord = NULL;
    }
    if (Fsm_MgrLoad(&fsmMgr, ddiMgr, fsm, opt->mc.ord, 1,
        Pdtutil_VariableOrderDefault_c) == 1) {
      fprintf(stderr, "-- FSM Loading Error.\n");
      exit(1);
    }
  } else if (strstr(fsm, ".aig") != NULL) {
    if (strcmp(opt->mc.ord, "dfs") == 0) {
      Pdtutil_Free(opt->mc.ord);
      opt->mc.ord = NULL;
    }
    if (Fsm_MgrLoadAiger(&fsmMgr, ddiMgr, fsm, opt->mc.ord, NULL,
        Pdtutil_VariableOrderDefault_c) == 1) {
      fprintf(stderr, "-- FSM Loading Error.\n");
      exit(1);
    }
  } else {
    char **ci = Pdtutil_Alloc(char *, 4
    );
    int ciNum = 0;
    int myCoiLimit = 0;

    ci[0] = NULL;

    if (0 && strcmp(opt->mc.ord, "dfs") == 0) {
      Pdtutil_Free(opt->mc.ord);
      opt->mc.ord = NULL;
    }
    if (opt->pre.doCoi > 1) {
      if (opt->mc.invSpec != NULL) {
        Ddi_Expr_t *e = Ddi_ExprLoad(ddiMgr, opt->mc.invSpec, NULL);

        ci = Expr_ListStringLeafs(e);
        ciNum = 1;
        Ddi_Free(e);
      } else if (opt->mc.ctlSpec != NULL) {
        Ddi_Expr_t *e = Ddi_ExprLoad(ddiMgr, opt->mc.ctlSpec, NULL);

        ci = Expr_ListStringLeafs(e);
        ciNum = 1;
        Ddi_Free(e);
      } else if (opt->mc.all1) {
        ci[ciNum] = Pdtutil_Alloc(char,
          20
        );

        sprintf(ci[ciNum], "l %d %d %d", 0, opt->mc.all1 - 1, myCoiLimit);
        ci[++ciNum] = NULL;
      } else if (opt->mc.idelta >= 0) {
        ci[ciNum] = Pdtutil_Alloc(char,
          20
        );

        sprintf(ci[ciNum], "l %d %d %d", opt->mc.idelta, opt->mc.idelta,
          myCoiLimit);
        ci[++ciNum] = NULL;
      } else if (opt->mc.nlambda != NULL) {
        ci[ciNum] = Pdtutil_Alloc(char,
          strlen(opt->mc.nlambda) + 4
        );

        sprintf(ci[ciNum], "n %s %d", opt->mc.nlambda, myCoiLimit);
        ci[++ciNum] = NULL;
        opt->mc.ilambda = 0;
      } else {
        ci[ciNum] = Pdtutil_Alloc(char,
          20
        );

        sprintf(ci[ciNum], "o %d %d %d", opt->mc.ilambda, opt->mc.ilambda,
          myCoiLimit);
        ci[++ciNum] = NULL;
        opt->mc.ilambda = 0;
      }
    }
    if (opt->mc.ninvar != NULL) {
      ci[ciNum] = Pdtutil_Alloc(char,
        strlen(opt->mc.ninvar) + 4
      );

      sprintf(ci[ciNum], "i %s %d", opt->mc.ninvar, myCoiLimit);
      ci[++ciNum] = NULL;
    }
    if (Fsm_MgrLoadFromBlifWithCoiReduction(&fsmMgr, ddiMgr, fsm, opt->mc.ord,
        1, Pdtutil_VariableOrderDefault_c, ci) == 1) {
      fprintf(stderr, "BLIF File Loading Error.\n");
    }
    if (ci != NULL) {
      Pdtutil_StrArrayFree(ci, -1);
    }
  }

  /* COMMON AIG-BDD PART */

  opt->stats.setupTime = util_cpu_time();

  if (0 && Ddi_BddarraySize(Fsm_MgrReadDeltaBDD(fsmMgr)) > 20000) {
    int aol = Ddi_MgrReadAigAbcOptLevel(ddiMgr);

    Ddi_MgrSetAigAbcOptLevel(ddiMgr, 2);
    Ddi_AigarrayAbcOptAcc(Fsm_MgrReadDeltaBDD(fsmMgr), 60.0);
    Ddi_MgrSetAigAbcOptLevel(ddiMgr, aol);
  }

  startVerifTime = util_cpu_time();
  pi = Fsm_MgrReadVarI(fsmMgr);
  ps = Fsm_MgrReadVarPS(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  deltaNum = Ddi_BddarrayNum(delta);
  lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
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

  if (!opt->mc.nogroup) {
    Ddi_MgrCreateGroups2(ddiMgr, ps, ns);
  }

  if (ps == NULL && opt->mc.checkMult) {
    FbvCombCheck(fsmMgr, Fbv_Mult_c);
    return 0;
  }

  if (ps != NULL) {
    psvars = Ddi_VarsetMakeFromArray(ps);
    nsvars = Ddi_VarsetMakeFromArray(ns);
  } else {
    opt->trav.noInit = 1;
    opt->mc.combinationalProblem = 1;
    psvars = nsvars = NULL;
  }

  if (opt->trav.noInit) {
    init = Ddi_BddMakeConst(ddiMgr, 0);
  } else if (opt->mc.rInit != NULL) {
    printf("\n**** Reading INIT set from %s ****\n", opt->mc.rInit);
    init = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
      DDDMP_MODE_DEFAULT, opt->mc.rInit, NULL);
    Ddi_MgrReduceHeap(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"), 0);
    Ddi_BddSetAig(init);
    Ddi_MgrSetSiftThresh(ddiMgr, opt->ddi.siftTh);
  } else {
    init = Ddi_BddDup(Fsm_MgrReadInitBDD(fsmMgr));
  }
  Fsm_MgrSetInitBDD(fsmMgr, init);
  Ddi_Free(init);

  if (opt->common.clk != NULL) {
    Fsm_MgrSetClkStateVar(fsmMgr, opt->common.clk, 1);
  }

  if (opt->mc.ninvar != NULL) {
    int i;

    invar = NULL;
    for (i = 0; i < Ddi_BddarrayNum(lambda); i++) {
      char *name = Ddi_ReadName(Ddi_BddarrayRead(lambda, i));

      if (name != NULL && strcmp(name, opt->mc.ninvar) == 0) {
        invar = Ddi_BddDup(Ddi_BddarrayRead(lambda, i));
        break;
      }
    }
    Pdtutil_Assert(invar != NULL, "Lambda not found by name");
  } else if (Fsm_MgrReadConstraintBDD(fsmMgr) != NULL) {
    invar = Ddi_BddDup(Fsm_MgrReadConstraintBDD(fsmMgr));
  }

  if (opt->mc.ilambda >= Ddi_BddarrayNum(lambda)) {
    opt->mc.ilambda = Ddi_BddarrayNum(lambda) - 1;
    printf("ilambda selection is too large: %d forced\n", opt->mc.ilambda);
  }

  if (opt->trav.noCheck) {
    invarspec = Ddi_BddMakeConstAig(ddiMgr, 1);

    if (opt->mc.method == Mc_VerifPrintNtkStats_c) {
      Ddi_Vararray_t *pi, *ps;
      Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);

      pi = Fsm_MgrReadVarI(fsmMgr);
      ps = Fsm_MgrReadVarPS(fsmMgr);
      printf("%d inputs / %d state variables / %d delta nodes \n",
        Ddi_VararrayNum(pi), Ddi_VararrayNum(ps), Ddi_BddarraySize(delta));
    }
  } else {
    invarspec = FbvGetSpec(fsmMgr, NULL, invar, NULL, opt);
    //      invarspec = getSpec (fsmMgr,NULL,NULL);
  }

  {
    Fsm_Fsm_t *fsmFsm = Fsm_FsmMakeFromFsmMgr(fsmMgr);

    Fsm_FsmFoldProperty(fsmFsm, opt->mc.compl_invarspec,
      opt->trav.cntReachedOK, 1);
    Fsm_FsmFoldConstraint(fsmFsm, opt->mc.compl_invarspec);
    //    Fsm_FsmFoldInit(fsmFsm);
    Fsm_FsmWriteToFsmMgr(fsmMgr, fsmFsm);
    Fsm_FsmFree(fsmFsm);
    invarspec = Fsm_MgrReadInvarspecBDD(fsmMgr);
  }

  if (opt->mc.combinationalProblem || Ddi_BddIsConstant(invarspec)) {
    printf("HEURISTIC strategy selection: COMB\n");
    Ddi_BddNotAcc(invarspec);
    verificationResult = Ddi_AigSat(invarspec);
    printf("Verification %s\n", verificationResult ? "FAILED" : "PASSED");
    Ddi_BddNotAcc(invarspec);
    goto quitMem;
  }
  //Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
  fprintf(stdout, "\nRunning reductions...\n"); //);
  if (0 && Ddi_BddarraySize(Fsm_MgrReadDeltaBDD(fsmMgr)) > 20000) {
    int aol = Ddi_MgrReadAigAbcOptLevel(ddiMgr);

    Ddi_MgrSetAigAbcOptLevel(ddiMgr, 2);
    Ddi_AigarrayAbcOptAcc(Fsm_MgrReadDeltaBDD(fsmMgr), 60.0);
    Ddi_MgrSetAigAbcOptLevel(ddiMgr, aol);
  }

  if (opt->pre.peripheralLatches) {
    int nRed;

    do {
      nRed = Fsm_RetimePeriferalLatches(fsmMgr);
      opt->pre.peripheralLatchesNum += nRed;
    } while (nRed > 0);
    if (care != NULL) {
      Ddi_Varset_t *psVars = Ddi_VarsetMakeFromArray(Fsm_MgrReadVarPS(fsmMgr));

      Ddi_BddExistProjectAcc(care, psVars);
      Ddi_Free(psVars);
    }
  }

  opt->stats.setupTime = util_cpu_time();

  if (opt->pre.findClk) {
    Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
    Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
    Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
    int i, nstate = Ddi_VararrayNum(ps);

    for (i = 0; i < nstate; i++) {
      Ddi_Bdd_t *dClk0, *dClk1;
      Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);

      dClk0 = Ddi_BddCofactor(d_i, v_i, 0);
      Ddi_BddNotAcc(dClk0);
      dClk1 = Ddi_BddCofactor(d_i, v_i, 1);
      if (!Ddi_AigSat(dClk1) && !Ddi_AigSat(dClk0)) {
        printf("clock found: %s\n", Ddi_VarName(v_i));
        opt->pre.clkVarFound = v_i;
      } else {
        Ddi_Varset_t *s_i = Ddi_BddSupp(d_i);

        if (Ddi_VarsetNum(s_i) == 1) {
          int j;
          Ddi_Var_t *v_in = Ddi_VarsetTop(s_i);

          for (j = 0; j < nstate; j++) {
            if (Ddi_VararrayRead(ps, j) == v_in) {
              break;
            }
          }
          if (j < nstate) {
            Ddi_Bdd_t *d_j = Ddi_BddarrayRead(delta, j);
            Ddi_Varset_t *s_j = Ddi_BddSupp(d_j);

            if (Ddi_VarsetNum(s_j) == 1 && Ddi_VarInVarset(s_j, v_i)) {
              if (Ddi_BddIsComplement(d_i) && Ddi_BddIsComplement(d_j)) {
                Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
                Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
                int eqLatches = 1;
                int diffLatches = 1;

                printf("clock pair found: %s - %s\n",
                  Ddi_VarName(v_i), Ddi_VarName(v_in));
                opt->pre.clkVarFound = v_i;
#if 1
                /* check init state !!! */
                if (initStub != NULL) {
                  Ddi_Bdd_t *st_i = Ddi_BddarrayRead(initStub, i);
                  Ddi_Bdd_t *st_j = Ddi_BddarrayRead(initStub, j);
                  Ddi_Bdd_t *st_jn = Ddi_BddNot(st_j);

                  eqLatches = Ddi_BddEqualSat(st_i, st_j);
                  diffLatches = Ddi_BddEqualSat(st_i, st_jn);
                  Ddi_Free(st_jn);
                } else {
                  Ddi_Bdd_t *dif = Ddi_BddMakeLiteralAig(v_i, 1);
                  Ddi_Bdd_t *aux = Ddi_BddMakeLiteralAig(v_in, 1);

                  Ddi_BddDiffAcc(dif, aux);
                  Ddi_Free(aux);
                  eqLatches = Ddi_AigSatAnd(init, dif, NULL);
                  Ddi_BddNotAcc(dif);
                  diffLatches = Ddi_AigSatAnd(init, dif, NULL);
                  Ddi_Free(dif);
                }
                if (eqLatches && !diffLatches) {
                  /* OK clock */
                  Ddi_DataCopy(d_i, d_j);
                } else {
                  /* undo clk */
                  opt->pre.clkVarFound = NULL;
                }
#endif
              }
            }
            Ddi_Free(s_j);
          }
        }
        Ddi_Free(s_i);
      }
      Ddi_Free(dClk0);
      Ddi_Free(dClk1);
    }
  }

  if (opt->pre.terminalScc) {
    opt->stats.verificationOK = 0;
    Ddi_Bdd_t *myInvar =
      Fsm_ReduceTerminalScc(fsmMgr, &opt->pre.terminalSccNum,
      &opt->stats.verificationOK, opt->trav.itpConstrLevel);

    if (opt->stats.verificationOK == 1) {
      printf("HEURISTIC strategy selection: TSCC\n");
      printf("\nVerification OK\n");
      verificationResult = 0;
      Ddi_Free(myInvar);
      init = NULL;
      goto quitMem;
    }
    if (myInvar != NULL) {
      if (invar == NULL) {
        invar = myInvar;
      } else {
        Ddi_BddAndAcc(invar, myInvar);
        Ddi_Free(myInvar);
      }
    }
  }

  if (opt->pre.impliedConstr) {
    Ddi_Bdd_t *myInvar = Fsm_ReduceImpliedConstr(fsmMgr,
      opt->pre.specDecompIndex, invar, &opt->pre.impliedConstrNum);

    if (myInvar != NULL) {
      if (invar == NULL) {
        invar = myInvar;
      } else {
        Ddi_BddAndAcc(invar, myInvar);
        Ddi_Free(myInvar);
      }
    }
  }

  FbvTestSec(fsmMgr, opt);
  FbvTestRelational(fsmMgr, opt);
  heuristic = FbvSelectHeuristic(fsmMgr, opt, 1);   // mixed

  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  deltaSize = Ddi_BddarraySize(delta);
  doRunRed = 1;
  bmcSteps = 35;
  bmcTimeLimit = 20;
  indSteps = 30;
  indTimeLimit = 10;
  lemmasSteps = 0;
  pdrTimeLimit = 120;
  opt->trav.lemmasTimeLimit = 35 + 25 * (deltaSize > 3000);
  opt->mc.method = Mc_VerifExactFwd_c;

  opt->ddi.dynAbstrStrategy = 1;
  Ddi_MgrSetAigDynAbstrStrategy(ddiMgr, opt->ddi.dynAbstrStrategy);
  Ddi_MgrSetAigItpCore(ddiMgr, opt->ddi.itpCore);
  Ddi_MgrSetAigItpMem(ddiMgr, opt->ddi.itpMem);
  Ddi_MgrSetAigItpOpt(ddiMgr, opt->ddi.itpOpt);
  Ddi_MgrSetAigItpStore(ddiMgr, opt->ddi.itpStore);
  Ddi_MgrSetAigItpAbc(ddiMgr, opt->ddi.itpAbc);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpAigCore_c, inum, opt->ddi.itpAigCore);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpTwice_c, inum, opt->ddi.itpTwice);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpIteOptTh_c, inum, opt->ddi.itpIteOptTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStructOdcTh_c, inum,
    opt->ddi.itpStructOdcTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiNnfClustSimplify_c, inum,
    opt->ddi.nnfClustSimplify);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMap_c, inum, opt->ddi.itpMap);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompute_c, inum, opt->ddi.itpCompute);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpLoad_c, pchar, opt->ddi.itpLoad);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpDrup_c, inum, opt->ddi.itpDrup);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompact_c, inum, opt->ddi.itpCompact);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpClust_c, inum, opt->ddi.itpClust);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpNorm_c, inum, opt->ddi.itpNorm);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpSimp_c, inum, opt->ddi.itpSimp);

  num_ff = Ddi_BddarrayNum(delta);

  switch (heuristic) {
    case Fbv_HeuristicBmcBig_c:    /* BMC */
      opt->pre.doCoi = 0;
      doRunBmc = 1;
      bmcSteps = -1;
      bmcTimeLimit = -1;
      opt->pre.noTernary = 1;
      opt->pre.ternarySim = 0;
      break;
    case Fbv_HeuristicIntel_c: /* intel */
      doRunPdr = 1;
      doRunItp = 1;
      itpTimeLimit = -1;
      itpBoundLimit = -1;

      opt->pre.reduceCustom = 1;
      opt->pre.twoPhaseRed = 1;

      /* itp4 setting */
      opt->mc.aig = 1;
      // opt->trav.itpPart = 1;
      lemmasSteps = 2 * (deltaSize < 10000);
      opt->trav.lemmasTimeLimit = 80;
      opt->pre.peripheralLatches = 1;
      // opt->pre.performAbcOpt = 0;
      opt->pre.abcSeqOpt = 0;
      opt->ddi.aigRedRemPeriod = 100;
      opt->mc.lazy = 1;
      opt->common.lazyRate = 1.0;
      opt->trav.dynAbstr = 0;
      opt->trav.itpAppr = 0;
      opt->trav.implAbstr = 3;

#if 1
      if (num_ff < 190 || num_ff > 250) {
        /* itp0 */
        lemmasSteps = 0;
        opt->trav.itpAppr = 0;
        opt->trav.implAbstr = 0;
        opt->trav.ternaryAbstr = 0;
        opt->common.lazyRate = 0.9;
        opt->trav.dynAbstr = 0;
        opt->ddi.aigBddOpt = 0;
        opt->common.aigAbcOpt = 0;
        opt->trav.itpEnToPlusImg = 0;
      }
#endif
      break;
    case Fbv_HeuristicDme_c:   /* dme */
      doRunBdd = 1;
      opt->fsm.cut = 200;
      break;
    case Fbv_HeuristicQueue_c: /* queue */
      opt->pre.peripheralLatches = 0;
      opt->pre.ternarySim = 1;
      doRunPdr = 0;
      doRunBdd = 1;
      opt->fsm.cut = 10000;
      // opt->trav.imgDpartTh = 100000;
      break;
    case Fbv_HeuristicNeedham_c:   /* ns2-ns3 */
      doRunPdr = 1;
      doRunInd = 1;
      doRunBdd = 1;
      indSteps = 20;
      indTimeLimit = 10;
      opt->trav.clustTh = 300;
      opt->trav.imgDpartTh = 500000;
      break;
    case Fbv_HeuristicSec_c:   /* sec */
      doRunLemmas = 1;
      opt->trav.lemmasTimeLimit = -1;
      opt->expt.lemmasSteps = 10;
      opt->pre.retime = 1;

      /* itp5 setting for interpolation (just in case ...) */
      opt->mc.aig = 1;
      opt->mc.lazy = 1;
      opt->common.lazyRate = 1.0;
      opt->trav.dynAbstr = 0;
      opt->trav.itpAppr = 1;
      opt->trav.implAbstr = 3;
      opt->trav.ternaryAbstr = 0;
      opt->trav.itpEnToPlusImg = 0;

      itpBoundLimit = -1;
      itpTimeLimit = -1;

      break;
    case Fbv_HeuristicPdtSw_c: /* pdt-sw */
      doRunBmc = 1;
      doRunBdd = 1;
      if (num_ff < 75 && num_ff > 65) {
        opt->mc.method = Mc_VerifApproxFwdExactBck_c;
        // opt->mc.method = Mc_VerifExactBck_c;
        opt->trav.apprOverlap = 0;
        opt->trav.apprGrTh = 1000;
        opt->trav.clustTh = 5000;
        opt->fsm.cut = 1000;
        opt->trav.imgDpartTh = 300000;
      } else {
        opt->fsm.cut = -1;
        opt->trav.clustTh = 500;
      }
      bmcSteps = 70;
      bmcTimeLimit = 120;
      bddTimeLimit = -1;

      break;
    case Fbv_HeuristicBddFull_c:   /* full-bdd */
      doRunBmc = 1;
      doRunInd = 1;
      doRunPdr = 1;
      doRunLms = 1;             // i lemmi girano fuori dall'interpolante
      doRunItp = 1;
      doRunBdd = 1;

      bmcSteps = 35;
      bmcTimeLimit = 15;
      indSteps = 10;
      indTimeLimit = 10;
      opt->trav.lemmasTimeLimit = 15;
      itpBoundLimit = 100;
      itpTimeLimit = 30;
      bddTimeLimit = -1;        //30+15;

      /* itp5 setting */
      opt->mc.aig = 1;
      lemmasSteps = 0;
      opt->pre.peripheralLatches = 1;
      // opt->pre.performAbcOpt = 0;
      opt->pre.abcSeqOpt = 0;
      opt->ddi.aigRedRemPeriod = 100;
      opt->mc.lazy = 1;
      opt->common.lazyRate = 2.3;
      // opt->trav.itpPart = 1;
      opt->trav.itpAppr = 1;
      // opt->trav.dynAbstr = 2;
      opt->trav.implAbstr = 2;
      opt->trav.ternaryAbstr = 0;

      opt->trav.clustTh = 5000;
      opt->fsm.cut = -1;
      opt->trav.imgDpartTh = 100000;
      break;
    case Fbv_HeuristicBdd_c:   /* bdd */
      doRunBmc = 1;
      doRunInd = 1;
      doRunPdr = 1;
      doRunLms = 0;             //lemmi abilitati con l'interpolante
      doRunItp = 1;
      doRunBdd = 1;

      bmcSteps = 40;
      bmcTimeLimit = 30;
      indSteps = 15;
      indTimeLimit = 15;
      lemmasSteps = opt->expt.lemmasSteps >= 0 ? opt->expt.lemmasSteps : 2 * (deltaSize < 7000);    //lemmi abilitati con l'interpolante
      opt->trav.lemmasTimeLimit = 40;
      itpBoundLimit = 100;
      itpTimeLimit = 60;

      /* itp4 setting */
      opt->mc.aig = 1;
      // opt->trav.itpPart = 1;
      opt->pre.peripheralLatches = 1;
      // opt->pre.performAbcOpt = 0;
      opt->pre.abcSeqOpt = 0;
      opt->ddi.aigRedRemPeriod = 100;
      opt->mc.lazy = 1;
      opt->common.lazyRate = 1.0;
      // opt->trav.dynAbstr = 2;
      opt->trav.itpAppr = 1;
      opt->trav.implAbstr = 2;
      opt->trav.ternaryAbstr = 1;

      opt->fsm.cut = 500;
      opt->trav.imgDpartTh = 100000;
      break;

    case Fbv_HeuristicBeem_c:  /* beem */
      opt->expt.doRunBmc = 1;
      opt->expt.doRunInd = 0;
      opt->expt.doRunItp = 1;
      // opt->pre.retime = 1;

      opt->expt.bmcSteps = 50;
      opt->expt.bmcTimeLimit = 40;
      opt->expt.indSteps = 10;
      opt->expt.indTimeLimit = 20;
      opt->expt.lemmasSteps = 2 * (deltaSize < 10000);
      opt->trav.lemmasTimeLimit = 60;
      opt->expt.itpBoundLimit = -1;
      opt->expt.itpTimeLimit = -1;

      /* itp1 setting */
      opt->mc.aig = 1;
      // opt->trav.itpPart = 1;
      opt->pre.peripheralLatches = 1;
      // opt->pre.performAbcOpt = 0;
      opt->pre.abcSeqOpt = 0;
      opt->ddi.aigRedRemPeriod = 100;
      opt->mc.lazy = 1;
      opt->common.lazyRate = 1.0;
      // opt->trav.dynAbstr = 2;
      opt->trav.implAbstr = 0;
      opt->trav.ternaryAbstr = 0;
      break;

    case Fbv_HeuristicRelational_c:    /* other */
      opt->pre.reduceCustom = 1;
    case Fbv_HeuristicOther_c: /* other */
      doRunBmc = 1;
      doRunInd = 1;
      doRunPdr = 1;
      doRunItp = 1;
      opt->pre.retime = 1;

      bmcSteps = 50;
      bmcTimeLimit = 40;
      indSteps = 40;
      indTimeLimit = 20;
      lemmasSteps = opt->expt.lemmasSteps >= 0 ? opt->expt.lemmasSteps : 2 * (deltaSize < 7000);    //lemmi abilitati con l'interpolante
      opt->trav.lemmasTimeLimit = 60;
      itpBoundLimit = -1;
      itpTimeLimit = -1;

      /* itp4 setting */
      opt->mc.aig = 1;
      // opt->trav.itpPart = 1;
      opt->pre.peripheralLatches = 1;
      // opt->pre.performAbcOpt = 0;
      opt->pre.abcSeqOpt = 0;
      opt->ddi.aigRedRemPeriod = 100;
      opt->mc.lazy = 1;
      opt->common.lazyRate = 1.0;
      // opt->trav.dynAbstr = 2;
      opt->trav.itpAppr = 1;
      opt->trav.implAbstr = 2;
      opt->trav.ternaryAbstr = 1;

      break;
    case Fbv_HeuristicNone_c:
    default:                   /* not possible */
      printf("Unknown verification strategy!");
      exit(1);
  }
  if (opt->expt.expertArgv != NULL) {
    FbvParseArgs(opt->expt.expertArgc, opt->expt.expertArgv, opt);
  }
  opt->stats.setupTime = util_cpu_time();

  if (opt->pre.reduceCustom) {
    if (invarspec != NULL) {
      Ddi_BddarrayWrite(lambda, 0, invarspec);
    }
    Ddi_Bdd_t *myInvar = Fsm_ReduceCustom(fsmMgr);

    Ddi_Free(myInvar);
    // Ddi_Free(init);
    // init = Ddi_BddDup(Fsm_MgrReadInitBDD (fsmMgr));
    if (invarspec != NULL) {
      Ddi_DataCopy(invarspec, Ddi_BddarrayRead(lambda, 0));
    }
    if (0) {
      int i, n;
      Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(pi);

      delta = Fsm_MgrReadDeltaBDD(fsmMgr);

      n = Ddi_BddarrayNum(delta);
      for (i = 0; i < n; i++) {
        Ddi_Bdd_t *p_i = Ddi_BddarrayRead(delta, i);

        if (Ddi_BddSize(p_i) == 1) {
          Ddi_Var_t *v = Ddi_BddTopVar(p_i);

          logBdd(p_i);
          if (Ddi_VarInVarset(piv, v)) {
            Ddi_Varset_t *s_i;

            p_i = Ddi_BddarrayExtract(delta, i);
            s_i = Ddi_BddarraySupp(delta);
            Ddi_BddarrayInsert(delta, i, p_i);
            if (Ddi_VarsetIsVoid(s_i)) {
              printf("free latch found\n");
            } else {
              int j;

              for (j = 0; j < n; j++) {
                if (j != i) {
                  Ddi_Bdd_t *p_j = Ddi_BddarrayRead(delta, j);
                  Ddi_Varset_t *s_j = Ddi_BddSupp(p_j);

                  if (Ddi_VarInVarset(s_j, v)) {
                    printf("found\n");
                  }
                  Ddi_Free(s_j);
                }
              }
            }
            Ddi_Free(s_i);
            Ddi_Free(p_i);
          }
        }
      }
      Ddi_Free(piv);
    }
  }

  if (opt->pre.twoPhaseRed && opt->pre.clkVarFound != NULL) {
    int ph = -1;
    int size0 = Ddi_BddarraySize(delta);
    Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
    Ddi_Bdd_t *phase, *init = Fsm_MgrReadInitBDD(fsmMgr);
    Ddi_Vararray_t *twoPhasePis =
      Ddi_VararrayMakeNewVars(pi, "PDT_ITP_TWOPHASE_PI", "0", 1);
    Ddi_Bddarray_t *twoPhasePiLits =
      Ddi_BddarrayMakeLiteralsAig(twoPhasePis, 1);
    Ddi_Bddarray_t *twoPhaseDelta = Ddi_BddarrayDup(delta);
    Ddi_Bddarray_t *twoPhaseDelta2 = Ddi_BddarrayDup(delta);

    Ddi_AigarrayComposeAcc(twoPhaseDelta, pi, twoPhasePiLits);
    Ddi_AigarrayComposeNoMultipleAcc(twoPhaseDelta, ps, twoPhaseDelta2);

    Ddi_DataCopy(delta, twoPhaseDelta);
    opt->pre.twoPhaseDone = 1;
    Fsm_MgrSetPhaseAbstr(fsmMgr, 1);
    Pdtutil_WresSetTwoPhase();

    Ddi_VararrayAppend(pi, twoPhasePis);

    phase = Ddi_BddMakeLiteralAig(opt->pre.clkVarFound, 1);
    if (initStub != NULL) {
      Ddi_BddComposeAcc(phase, ps, initStub);
    } else {
      Ddi_BddAndAcc(phase, init);
    }

    if (!Ddi_AigSat(phase)) {
      ph = 0;
    } else if (!Ddi_AigSat(Ddi_BddNotAcc(phase))) {
      ph = 1;
    }
    if (1 && (ph >= 0)) {
      int i;

      for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
        Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);

        Ddi_BddCofactorAcc(d_i, opt->pre.clkVarFound, ph);
      }
    }

    Ddi_Free(phase);

    printf("Two phase reduction: %d -> %d\n", size0, Ddi_BddarraySize(delta));

    Ddi_Free(twoPhasePis);
    Ddi_Free(twoPhasePiLits);
    Ddi_Free(twoPhaseDelta);
    Ddi_Free(twoPhaseDelta2);
  }

  if (0 && opt->trav.rPlus != NULL) {
    char *pbench;
    Ddi_Var_t *v = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
    Ddi_Var_t *v1 = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

    if (0 && !v) {
      char buf[256];

      sprintf(buf, "PDT_BDD_INVARSPEC_VAR$PS");
      v = Ddi_VarNew(ddiMgr);
      Ddi_VarAttachName(v, buf);
    }
    printf("\n**** Reading REACHED set from %s ****\n", opt->trav.rPlus);
    fflush(NULL);
    if ((pbench = strstr(opt->trav.rPlus, ".bench")) != NULL) {
      //      *pbench = '\0';
      care = Ddi_AigNetLoadBench(ddiMgr, opt->trav.rPlus, NULL);
      if (v != NULL) {
        Ddi_BddCofactorAcc(care, v, 1);
      }
      if (v1 != NULL) {
        Ddi_BddCofactorAcc(care, v1, 1);
      }
      //      care = Ddi_BddarrayExtract(cArray,0);
    } else {
      care = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
        DDDMP_MODE_DEFAULT, opt->trav.rPlus, NULL);
      Ddi_BddExistProjectAcc(care, psvars);
      Ddi_MgrReduceHeap(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"), 0);
    }
    Ddi_BddSetAig(care);
    Ddi_MgrSetSiftThresh(ddiMgr, opt->ddi.siftTh);
  }

  travMgrAig = Trav_MgrInit(NULL, ddiMgr);
  FbvSetTravMgrOpt(travMgrAig, opt);
  FbvSetTravMgrFsmData(travMgrAig, fsmMgr);

  opt->stats.setupTime = util_cpu_time();

  /* SEQUENTIAL VERIFICATION STARTS HERE... */

  /********************************************************
   * Verification phase 0:
   * Ciruit reductions (coi, ternary sim, etc.)
   ********************************************************/

  elapsedTime = util_cpu_time();

  if (opt->trav.lemmasCare && !opt->pre.retime) {
    care = Ddi_BddMakeConstAig(ddiMgr, 1);
  }

  if (doRunRed) {
    loopReduce = 1;
    for (loopReduceCnt = 0; loopReduce; loopReduceCnt++) {
      int innerLoop, loopCnt;

      loopReduce = 0;

      if (0) {
        int i;

        for (i = 0; i < Ddi_BddarrayNum(Fsm_MgrReadDeltaBDD(fsmMgr)); i++) {
          printf("D %d (%s) %d\n", i,
            Ddi_VarName(Ddi_VararrayRead(Fsm_MgrReadVarPS(fsmMgr), i)),
            Ddi_BddSize(Ddi_BddarrayRead(Fsm_MgrReadDeltaBDD(fsmMgr), i)));
        }
      }

      if (1 && opt->pre.doCoi >= 1) {

        Ddi_Varsetarray_t *coi;
        int nCoi;

        coi = computeFsmCoiVars(fsmMgr, invarspec, NULL, opt->tr.coiLimit, 1);
        if (opt->pre.wrCoi != NULL) {
          writeCoiShells(opt->pre.wrCoi, coi, ps);
        }
        nCoi = Ddi_VarsetarrayNum(coi);
        fsmCoiReduction(fsmMgr, Ddi_VarsetarrayRead(coi, nCoi - 1));

        Ddi_Free(coi);
      }

      loopCnt = 0;
      do {

        if (opt->pre.ternarySim == 0) {
          innerLoop = 0;
        } else {
          innerLoop = fsmConstReduction(fsmMgr, loopCnt == 0, -1, opt, 0);
        }
        loopCnt++;

        if (innerLoop < 0) {
          /* property proved */
          printf("HEURISTIC strategy selection: STS\n");
          verificationResult = 0;
          init = NULL;
          goto quitMem;
        }

        loopReduce |= (innerLoop > 2);

        if (1 && opt->pre.doCoi >= 1) {

          Ddi_Varsetarray_t *coi;
          int nCoi;

          coi =
            computeFsmCoiVars(fsmMgr, invarspec, NULL, opt->tr.coiLimit, 1);
          if (opt->pre.wrCoi != NULL) {
            writeCoiShells(opt->pre.wrCoi, coi, ps);
          }
          nCoi = Ddi_VarsetarrayNum(coi);
          fsmCoiReduction(fsmMgr, Ddi_VarsetarrayRead(coi, nCoi - 1));

          Ddi_Free(coi);
        }

      } while (innerLoop && loopCnt <= 4);

      init = Fsm_MgrReadInitBDD(fsmMgr);

      if (opt->pre.retime) {
        int size0 = Ddi_BddarraySize(Fsm_MgrReadDeltaBDD(fsmMgr));

        loopReduce |= Fsm_RetimeMinreg(fsmMgr, care, opt->pre.retime);
        if (Ddi_BddarraySize(Fsm_MgrReadDeltaBDD(fsmMgr)) > size0 * 1.05) {
          opt->pre.retime = 0;
        } else if (0 && size0 > 20000) {
          int aol = Ddi_MgrReadAigAbcOptLevel(ddiMgr);

          Ddi_MgrSetAigAbcOptLevel(ddiMgr, 2);
          Ddi_AigarrayAbcOptAcc(Fsm_MgrReadDeltaBDD(fsmMgr), 30.0);
          Ddi_MgrSetAigAbcOptLevel(ddiMgr, aol);
        }

        if (0 && loopReduceCnt < 2 && deltaSize < 6000) {
          Trav_MgrSetOption(travMgrAig, Pdt_TravLemmasTimeLimit_c, fnum,
            10 + 10 * (deltaSize > 3000));
          if (deltaSize > 20000) {
            Trav_MgrSetOption(travMgrAig, Pdt_TravLemmasTimeLimit_c, fnum,
              500);
          }
          int result = Trav_TravLemmaReduction(travMgrAig,
            fsmMgr, init, invarspec, invar, care, 1 /*opt->mc.lemmas */ ,
            opt->trav.implLemmas, opt->trav.strongLemmas, NULL);

          //      loopReduce = 1;
          if (result != 0) {
            Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
              fprintf(stdout, "HEURISTIC strategy selection: LMS\n"));
            verificationResult = result < 0;
            init = NULL;
            goto quitMem;
          }
        }
      }
    }
    elapsedTime = util_cpu_time() - elapsedTime;
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Total run time for reductions: %.1f\n\n",
        elapsedTime / 1000.0));
  }
  Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
    fprintf(stdout,
      "Circuit AIG stats: %d inp / %d state vars / %d AIG gates\n",
      Ddi_VararrayNum(Fsm_MgrReadVarI(fsmMgr)),
      Ddi_VararrayNum(Fsm_MgrReadVarPS(fsmMgr)),
      Ddi_BddarraySize(Fsm_MgrReadDeltaBDD(fsmMgr))));

  /********************************************************
   * Verification phase 1:
   * Detect specific verification problems (currently only SEC)
   ********************************************************/

  lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  deltaSize = Ddi_BddarraySize(delta);

  Ddi_MgrSetAigBddOptLevel(ddiMgr, opt->ddi.aigBddOpt);
  Ddi_MgrSetAigAbcOptLevel(ddiMgr, opt->common.aigAbcOpt);
  Ddi_MgrSetAigRedRemLevel(ddiMgr, opt->ddi.aigRedRem);
  Ddi_MgrSetAigRedRemMaxCnt(ddiMgr, opt->ddi.aigRedRemPeriod);
  Ddi_MgrSetSatSolver(ddiMgr, opt->common.satSolver);
  Ddi_MgrSetAigSatTimeout(ddiMgr, opt->ddi.satTimeout);
  Ddi_MgrSetAigSatTimeLimit(ddiMgr, opt->ddi.satTimeLimit);
  Ddi_MgrSetAigSatIncremental(ddiMgr, opt->ddi.satIncremental);
  Ddi_MgrSetAigSatVarActivity(ddiMgr, opt->ddi.satVarActivity);
  Ddi_MgrSetAigLazyRate(ddiMgr, opt->common.lazyRate);

  init = Ddi_BddDup(Fsm_MgrReadInitBDD(fsmMgr));
  if (!Ddi_BddIsAig(invarspec)) {
    Ddi_Bdd_t *newI = Ddi_AigMakeFromBdd(invarspec);

    Ddi_Free(invarspec);
    invarspec = newI;
  }

  Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
    printf("|invarspec|: %d\n", Ddi_BddSize(invarspec));
    printf("|init|: %d\n", Ddi_BddSize(init)););
  if (invar != NULL) {
    Fsm_MgrSetConstraintBDD(fsmMgr, invar);
  }
  Ddi_Free(init);
  Ddi_Free(invar);
  Fsm_MgrSetInvarspecBDD(fsmMgr, invarspec);
  lastDdiNode = Ddi_MgrReadCurrNodeId(ddiMgr);

  if (doRunLemmas) {            /* sec */
    //Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "\nRunning lemmas...\n");   //);

    init = Ddi_BddDup(Fsm_MgrReadInitBDD(fsmMgr));
    if (Fsm_MgrReadConstraintBDD(fsmMgr) != NULL) {
      invar = Ddi_BddDup(Fsm_MgrReadConstraintBDD(fsmMgr));
    }
    travMgrAig = Trav_MgrInit(NULL, ddiMgr);
    FbvSetTravMgrOpt(travMgrAig, opt);
    FbvSetTravMgrFsmData(travMgrAig, fsmMgr);

    Trav_MgrSetOption(travMgrAig, Pdt_TravLemmasTimeLimit_c, fnum, -1);
    Trav_TravSatItpVerif(travMgrAig, fsmMgr,
      init, invarspec, NULL, invar, care, opt->expt.lemmasSteps,
      itpBoundLimit, opt->common.lazyRate, -1, itpTimeLimit);
    verificationResult = Trav_MgrReadAssertFlag(travMgrAig);
    goto quitMem;
  }

  opt->stats.setupTime = util_cpu_time();

  /********************************************************
   * Verification phase 2:
   * Try basic methods
   ********************************************************/

  /* Verification phase 2a: BMC & induction */
  if (doRunBmc || doRunInd) {   /* ns2/3 or bdd or other */

    fsmMgr2 = Fsm_MgrDup(fsmMgr);
    assert(care == NULL);
    delta = Fsm_MgrReadDeltaBDD(fsmMgr2);
    lambda = Fsm_MgrReadLambdaBDD(fsmMgr2);
    init = Ddi_BddDup(Fsm_MgrReadInitBDD(fsmMgr2));
    if (Fsm_MgrReadConstraintBDD(fsmMgr2) != NULL) {
      invar = Ddi_BddDup(Fsm_MgrReadConstraintBDD(fsmMgr2));
    }

    if (doRunBmc) {
      //Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "\nRunning BMC...\n");    //);
      if (opt->trav.bmcStrategy >= 1) {
        Trav_TravSatBmcIncrVerif(travMgrAig, fsmMgr2,
          init, invarspec, invar, NULL, bmcSteps, 1 /*bmcStep */ ,
          opt->trav.bmcLearnStep, opt->trav.bmcFirst,
          1 /*doCex */ , 0, 0 /*interpolantBmcSteps */ , opt->trav.bmcStrategy,
          opt->trav.bmcTe, bmcTimeLimit);
        Ddi_Bdd_t *cex = Fsm_MgrReadCexBDD(fsmMgr);

        opt->trav.bmcFirst = 0;
        if (cex != NULL) {
          opt->trav.bmcFirst = Ddi_BddPartNum(cex);
        }
      } else {
        opt->trav.bmcFirst = Trav_TravSatBmcVerif(travMgrAig, fsmMgr2,
          init, invarspec, invar, bmcSteps, 1 /*bmcStep */ ,
          opt->trav.bmcFirst,
          0 /*apprAig */ , 0, 0 /*interpolantBmcSteps */ , bmcTimeLimit);
      }
      verificationResult = Trav_MgrReadAssertFlag(travMgrAig);
      if (verificationResult > 0) {
        /* property failed */
        Fsm_MgrQuit(fsmMgr2);
        goto quitMem;
      }
      printf("Invariant verification UNDECIDED after BMC ...\n");
    }

    if (doRunInd) {
      //Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "\nRunning simple induction...\n");   //);
      Trav_TravSatInductiveTrVerif(travMgrAig, fsmMgr2,
        init, invarspec, invar, care, 1 /*bmcStep */ ,
        opt->trav.bmcFirst, indSteps, indTimeLimit);
      verificationResult = Trav_MgrReadAssertFlag(travMgrAig);
      if (verificationResult >= 0) {
        /* property proved or failed */
        Fsm_MgrQuit(fsmMgr2);
        goto quitMem;
      }
      printf("Invariant verification UNDECIDED after simple induction ...\n");
    }

    Fsm_MgrQuit(fsmMgr2);
    Ddi_Free(init);
    Ddi_Free(invar);
    travMgrAig = Trav_MgrQuit(travMgrAig);
    Ddi_MgrFreeExtRef(ddiMgr, lastDdiNode);
  }

  /* Verification phase 2b: pdr */
  if (doRunPdr) {
    //Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "\nRunning PDR ...\n"); //);

    fsmMgr2 = Fsm_MgrDup(fsmMgr);
    delta = Fsm_MgrReadDeltaBDD(fsmMgr2);
    lambda = Fsm_MgrReadLambdaBDD(fsmMgr2);
    init = Ddi_BddDup(Fsm_MgrReadInitBDD(fsmMgr2));
    if (Fsm_MgrReadConstraintBDD(fsmMgr2) != NULL) {
      invar = Ddi_BddDup(Fsm_MgrReadConstraintBDD(fsmMgr2));
    }
    travMgrAig = Trav_MgrInit(NULL, ddiMgr);
    FbvSetTravMgrOpt(travMgrAig, opt);
    FbvSetTravMgrFsmData(travMgrAig, fsmMgr2);

    invarspecItp = Ddi_BddDup(invarspec);
    Trav_TravPdrVerif(travMgrAig, fsmMgr2, init, invarspecItp, invar, care, -1,
      pdrTimeLimit);
    verificationResult = Trav_MgrReadAssertFlag(travMgrAig);
    Fsm_MgrQuit(fsmMgr2);
    Ddi_Free(invarspecItp);
    if (verificationResult >= 0) {
      /* property proved or failed */
      goto quitMem;
    }
    travMgrAig = Trav_MgrQuit(travMgrAig);
    Ddi_Free(init);
    Ddi_Free(invar);
    printf("Invariant verification UNDECIDED after pdr ...\n");
    Ddi_MgrFreeExtRef(ddiMgr, lastDdiNode);
  }

  /* Verification phase 2c: lemmas */
  if (doRunLms && deltaSize < 8000) {
    //Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "\nRunning lemmas ...\n");  //);

    fsmMgr2 = Fsm_MgrDup(fsmMgr);
    delta = Fsm_MgrReadDeltaBDD(fsmMgr2);
    lambda = Fsm_MgrReadLambdaBDD(fsmMgr2);
    init = Ddi_BddDup(Fsm_MgrReadInitBDD(fsmMgr2));
    if (Fsm_MgrReadConstraintBDD(fsmMgr2) != NULL) {
      invar = Ddi_BddDup(Fsm_MgrReadConstraintBDD(fsmMgr2));
    }
    travMgrAig = Trav_MgrInit(NULL, ddiMgr);
    FbvSetTravMgrOpt(travMgrAig, opt);
    FbvSetTravMgrFsmData(travMgrAig, fsmMgr2);

    invarspecItp = Ddi_BddDup(invarspec);
    Trav_MgrSetOption(travMgrAig, Pdt_TravLemmasTimeLimit_c, fnum,
      opt->trav.lemmasTimeLimit);
    Trav_TravLemmaReduction(travMgrAig, fsmMgr2, init, invarspec, invar, care,
      2 /*lemmasSteps */ , 0, 3, NULL);
    verificationResult = Trav_MgrReadAssertFlag(travMgrAig);
    Fsm_MgrQuit(fsmMgr2);
    Ddi_Free(invarspecItp);
    if (verificationResult >= 0) {
      /* property proved or failed */
      goto quitMem;
    }
    travMgrAig = Trav_MgrQuit(travMgrAig);
    Ddi_Free(init);
    Ddi_Free(invar);
    printf("Invariant verification UNDECIDED after lemmas ...\n");
    Ddi_MgrFreeExtRef(ddiMgr, lastDdiNode);
  }

  /* Verification phase 2d: interpolants */
  if (doRunItp) {
    //Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "\nRunning interpolants %s...\n", lemmasSteps ? "with lemmas " : "");   //);

    Ddi_MgrSetAigBddOptLevel(ddiMgr, opt->ddi.aigBddOpt);
    Ddi_MgrSetAigAbcOptLevel(ddiMgr, opt->common.aigAbcOpt);
    Ddi_MgrSetAigRedRemLevel(ddiMgr, opt->ddi.aigRedRem);
    Ddi_MgrSetAigRedRemMaxCnt(ddiMgr, opt->ddi.aigRedRemPeriod);

    Ddi_MgrSetTimeInitial(ddiMgr);
    //Ddi_MgrSetTimeLimit(ddiMgr, bddTimeLimit);
    Ddi_MgrSetMemoryLimit(ddiMgr, bddMemoryLimit);
    Ddi_MgrSetSiftMaxThresh(ddiMgr, opt->ddi.siftMaxTh);
    Ddi_MgrSetSiftMaxCalls(ddiMgr, opt->ddi.siftMaxCalls);

    fsmMgr2 = Fsm_MgrDup(fsmMgr);
    delta = Fsm_MgrReadDeltaBDD(fsmMgr2);
    lambda = Fsm_MgrReadLambdaBDD(fsmMgr2);
    init = Ddi_BddDup(Fsm_MgrReadInitBDD(fsmMgr2));
    if (Fsm_MgrReadConstraintBDD(fsmMgr2) != NULL) {
      invar = Ddi_BddDup(Fsm_MgrReadConstraintBDD(fsmMgr2));
    }
    travMgrAig = Trav_MgrInit(NULL, ddiMgr);
    FbvSetTravMgrOpt(travMgrAig, opt);
    FbvSetTravMgrFsmData(travMgrAig, fsmMgr2);


    invarspecItp = Ddi_BddDup(invarspec);

    Trav_TravSatItpVerif(travMgrAig, fsmMgr2,
      init, invarspecItp, NULL, invar, care, lemmasSteps, itpBoundLimit,
      opt->common.lazyRate, -1, itpTimeLimit);
    verificationResult = Trav_MgrReadAssertFlag(travMgrAig);
    Ddi_Free(invarspecItp);
    Fsm_MgrQuit(fsmMgr2);
    if (verificationResult >= 0) {
      /* property proved or failed */
      goto quitMem;
    }
    travMgrAig = Trav_MgrQuit(travMgrAig);
    Ddi_Free(init);
    Ddi_Free(invar);
    printf("Invariant verification UNDECIDED after ITP ...\n");
    Ddi_MgrFreeExtRef(ddiMgr, lastDdiNode);
    bddTimeLimit = -1;
  }

  /* Verification phase 2e: forward BDDs */
  if (doRunBdd) {
    //int verificationOK;

    Ddi_MgrSetTimeInitial(ddiMgr);
    Ddi_MgrSetTimeLimit(ddiMgr, bddTimeLimit);
    Ddi_MgrSetMemoryLimit(ddiMgr, bddMemoryLimit);
    Ddi_MgrSetSiftMaxThresh(ddiMgr, opt->ddi.siftMaxTh);
    Ddi_MgrSetSiftMaxCalls(ddiMgr, opt->ddi.siftMaxCalls);

    //Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "\nRunning BDDs...\n"); //);

    CATCH {
      fsmMgr2 = Fsm_MgrDup(fsmMgr);
      delta = Fsm_MgrReadDeltaBDD(fsmMgr2);
      lambda = Fsm_MgrReadLambdaBDD(fsmMgr2);
      if (Fsm_MgrReadConstraintBDD(fsmMgr2) != NULL) {
        invar = Ddi_BddMakeMono(Fsm_MgrReadConstraintBDD(fsmMgr2));
      }
      invarspecBdd = Ddi_BddMakeMono(invarspec);
      if (care != NULL) {
        Ddi_BddSetMono(care);
        Ddi_BddExistProjectAcc(care, psvars);
      }

      Fsm_MgrSetCutThresh(fsmMgr2, opt->fsm.cut);
      Fsm_MgrSetOption(fsmMgr2, Pdt_FsmCutAtAuxVar_c, inum,
                       opt->fsm.cutAtAuxVar);
      Fsm_MgrAigToBdd(fsmMgr2);

      init = Ddi_BddMakeMono(Fsm_MgrReadInitBDD(fsmMgr2));

      trMgr = Tr_MgrInit("TR_manager", ddiMgr);

      if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
        ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
        Tr_MgrSetVerbosity(trMgr, Pdtutil_VerbLevel_e(opt->verbosity));
      }

      Tr_MgrSetSortMethod(trMgr, Tr_SortWeight_c);
      Tr_MgrSetClustSmoothPi(trMgr, 0);
      Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
      Tr_MgrSetImgClustThreshold(trMgr, opt->trav.imgClustTh);
      Tr_MgrSetImgApproxMinSize(trMgr, opt->tr.approxImgMin);
      Tr_MgrSetImgApproxMaxSize(trMgr, opt->tr.approxImgMax);
      Tr_MgrSetImgApproxMaxIter(trMgr, opt->tr.approxImgIter);

      pi = Fsm_MgrReadVarI(fsmMgr2);
      ps = Fsm_MgrReadVarPS(fsmMgr2);
      ns = Fsm_MgrReadVarNS(fsmMgr2);

      Tr_MgrSetI(trMgr, pi);
      Tr_MgrSetPS(trMgr, ps);
      Tr_MgrSetNS(trMgr, ns);

      if (Ddi_VararrayNum(pi) < 22) {
      }

      if (Fsm_MgrReadVarAuxVar(fsmMgr2) != NULL) {
        Ddi_Vararray_t *auxvArray = Fsm_MgrReadVarAuxVar(fsmMgr2);

        opt->fsm.auxVarsUsed = 1;
        Ddi_VararrayAppend(pi, auxvArray);
        Tr_MgrSetAuxVars(trMgr, Fsm_MgrReadVarAuxVar(fsmMgr2));
        Tr_MgrSetAuxFuns(trMgr, Fsm_MgrReadAuxVarBDD(fsmMgr2));
      }

      Tr_MgrSetI(trMgr, pi);
      Tr_MgrSetPS(trMgr, ps);
      Tr_MgrSetNS(trMgr, ns);

      if (Fsm_MgrReadTrBDD(fsmMgr2) == NULL) {
        tr = Tr_TrMakePartConjFromFunsWithAuxVars(trMgr, delta, ns,
          Fsm_MgrReadAuxVarBDD(fsmMgr2), Fsm_MgrReadVarAuxVar(fsmMgr2));
      } else {
        tr = Tr_TrMakeFromRel(trMgr, Fsm_MgrReadTrBDD(fsmMgr2));
      }
      Tr_MgrSetCoiLimit(trMgr, opt->tr.coiLimit);
      Tr_TrCoiReduction(tr, invarspecBdd);

      FbvFsmCheckComposePi(fsmMgr2, tr);
      Fsm_MgrQuit(fsmMgr2);

      if (TRAV_BDD_VERIF) {
        travMgrBdd = Trav_MgrInit(opt->expName, ddiMgr);
        FbvSetTravMgrOpt(travMgrBdd, opt);
        //FbvSetTravMgrFsmData(travMgrBdd, fsmMgr);
        //Trav_MgrSetI(travMgrBdd, Tr_MgrReadI(trMgr));
        //Trav_MgrSetPS(travMgrBdd, Tr_MgrReadPS(trMgr));
        //Trav_MgrSetNS(travMgrBdd, Tr_MgrReadNS(trMgr));
        //Trav_MgrSetFrom(travMgrBdd, from);
        //Trav_MgrSetReached(travMgrBdd, from);
        //Trav_MgrSetTr(travMgrBdd, tr);
      }

      if (opt->mc.method == Mc_VerifExactBck_c) {
        //opt->trav.rPlus = "1";
        if (travMgrBdd != NULL) {
          opt->stats.verificationOK =
            Trav_TravBddApproxForwExactBckFixPointVerify(travMgrBdd, fsmMgr,
            tr, init, invarspec, invar, care, delta);
        } else {
          opt->stats.verificationOK =
            FbvApproxForwExactBckFixPointVerify(fsmMgr, tr, init, invarspec,
            invar, care, delta, opt);
        }
      } else if (opt->mc.method == Mc_VerifApproxFwdExactBck_c) {
        if (travMgrBdd != NULL) {
          opt->stats.verificationOK =
            Trav_TravBddApproxForwExactBckFixPointVerify(travMgrBdd, fsmMgr,
            tr, init, invarspecBdd, invar, care, delta);
        } else {
          opt->stats.verificationOK =
            FbvApproxForwExactBckFixPointVerify(fsmMgr, tr, init, invarspecBdd,
            invar, care, delta, opt);
        }
      } else {
        if (travMgrBdd != NULL) {
          travMgrBdd->settings.bdd.bddTimeLimit = bddTimeLimit;
          travMgrBdd->settings.bdd.bddNodeLimit = bddNodeLimit;
          opt->stats.verificationOK =
            Trav_TravBddExactForwVerify(travMgrBdd, fsmMgr, tr, init,
            invarspecBdd, invar, delta);
        } else {
          opt->stats.verificationOK =
            FbvExactForwVerify(NULL, fsmMgr, tr, init, invarspecBdd, invar,
            delta, bddTimeLimit, bddNodeLimit, opt);
        }
      }

      if (travMgrBdd != NULL) {
        /* print stats */
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
          Pdtutil_VerbLevelNone_c, currTime = util_cpu_time();
          fprintf(stdout, "Live nodes (peak): %u(%u)\n",
            Ddi_MgrReadLiveNodes(ddiMgr),
            Ddi_MgrReadPeakLiveNodeCount(ddiMgr));
#if 0                           // NXR: fix time computations
          fprintf(stdout, "TotalTime: %s ",
            util_print_time(setupTime + travTime));
          fprintf(stdout, "(setup: %s - ", util_print_time(setupTime));
          fprintf(stdout, " trav: %s)\n", util_print_time(travTime));
#endif
          );
        Trav_MgrQuit(travMgrBdd);
      }

      if (opt->stats.verificationOK >= 0) {
        /* property proved or failed */
        verificationResult = !opt->stats.verificationOK;
        Tr_TrFree(tr);
        Ddi_Free(invarspecBdd);
        Tr_MgrQuit(trMgr);
        goto quitMem;
      }
    }
    FAIL {
      printf("Invariant verification UNDECIDED after BDDs ...\n");
    }

    Ddi_Free(init);
    Ddi_Free(invar);
    Ddi_Free(invarspecBdd);
    Tr_TrFree(tr);

    Tr_MgrQuit(trMgr);
    Ddi_MgrSetTimeLimit(ddiMgr, -1);
    Ddi_MgrSetMemoryLimit(ddiMgr, -1);
    Ddi_MgrFreeExtRef(ddiMgr, lastDdiNode);
  }

  /*
   *  Qui non dovrebbe mai arrivare: aggiungo un itp per sicurezza
   */
  opt->stats.setupTime = util_cpu_time();

  /* Verification phase 3a: aggressive interpolant */
  if (1) {
    //Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "\nRunning interpolants ...\n");    //);

    opt->common.aigAbcOpt = 3;
    opt->ddi.aigBddOpt = 0;
    opt->ddi.aigRedRem = 1;
    opt->ddi.aigRedRemPeriod = 100;
    Ddi_MgrSetAigBddOptLevel(ddiMgr, opt->ddi.aigBddOpt);
    Ddi_MgrSetAigAbcOptLevel(ddiMgr, opt->common.aigAbcOpt);
    Ddi_MgrSetAigRedRemLevel(ddiMgr, opt->ddi.aigRedRem);
    Ddi_MgrSetAigRedRemMaxCnt(ddiMgr, opt->ddi.aigRedRemPeriod);

    opt->mc.aig = 1;
    // opt->trav.itpPart = 1;
    opt->pre.peripheralLatches = 1;
    // opt->pre.performAbcOpt = 0;
    opt->pre.abcSeqOpt = 0;
    opt->ddi.aigRedRemPeriod = 100;
    opt->mc.lazy = 1;
    opt->common.lazyRate = 1.0;
    // opt->trav.dynAbstr = 2;
    opt->trav.itpAppr = 1;
    opt->trav.implAbstr = 2;
    opt->trav.ternaryAbstr = 1;
    // opt->trav.itpEnToPlusImg = 1;
    itpTimeLimit = -1;
    itpBoundLimit = -1;

    fsmMgr2 = Fsm_MgrDup(fsmMgr);
    delta = Fsm_MgrReadDeltaBDD(fsmMgr2);
    lambda = Fsm_MgrReadLambdaBDD(fsmMgr2);
    init = Ddi_BddDup(Fsm_MgrReadInitBDD(fsmMgr2));
    if (Fsm_MgrReadConstraintBDD(fsmMgr2) != NULL) {
      invar = Ddi_BddDup(Fsm_MgrReadConstraintBDD(fsmMgr2));
    }
    travMgrAig = Trav_MgrInit(NULL, ddiMgr);
    FbvSetTravMgrOpt(travMgrAig, opt);
    FbvSetTravMgrFsmData(travMgrAig, fsmMgr2);

    Trav_TravSatItpVerif(travMgrAig, fsmMgr2,
      init, invarspec, NULL, invar, care, 0 /*lemmasSteps */ , itpBoundLimit,
      opt->common.lazyRate, -1, itpTimeLimit);
    verificationResult = Trav_MgrReadAssertFlag(travMgrAig);
    Fsm_MgrQuit(fsmMgr2);
    if (verificationResult >= 0) {
      /* property proved or failed */
      goto quitMem;
    }
    travMgrAig = Trav_MgrQuit(travMgrAig);
    Ddi_Free(init);
    Ddi_Free(invar);
    printf("Invariant verification UNDECIDED after ITP ...\n");
    Ddi_MgrFreeExtRef(ddiMgr, lastDdiNode);
  }


quitMem:

  Ddi_Free(opt->trav.auxPsSet);
  Ddi_Free(init);
  Ddi_Free(care);
  Ddi_Free(invar);
  Ddi_Free(invarspec);
  Ddi_Free(psvars);
  Ddi_Free(nsvars);
  Ddi_Free(opt->trav.univQuantifyVars);
  Ddi_Free(opt->trav.hints);

  Fsm_MgrQuit(fsmMgr);

  if (travMgrAig != NULL) {
    Trav_MgrQuit(travMgrAig);
  }

  /* write result on file */

  if (opt->mc.wRes != NULL && verificationResult >= 0) {
    FILE *fp;

    fp = fopen(opt->mc.wRes, "w");
    if (fp != NULL) {
      fprintf(stdout, "Verification result (%s) written to file %s\n",
        verificationResult ? "FAIL" : "PASS", opt->mc.wRes);
      fprintf(fp, "%d\n", verificationResult);
      fclose(fp);
    } else {
      printf("Error opening file %s\n", opt->mc.wRes);
    }
  }

  if (Ddi_MgrCheckExtRef(ddiMgr, 0) == 0) {
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

  Ddi_MgrQuit(ddiMgr);

  /**********************************************************************/
  /*                             End test                               */
  /**********************************************************************/

  return verificationResult;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
invarDecompVerif(
  char *fsm,
  char *ord,
  Fbv_Globals_t * opt
)
{
  Ddi_Mgr_t *ddiMgr;
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Varset_t *psvars, *nsvars, *coivars = NULL, *pVars, *constrVars = NULL;
  Tr_Mgr_t *trMgr = NULL;
  Trav_Mgr_t *travMgrBdd = NULL, *travMgrAig = NULL;
  Fsm_Mgr_t *fsmMgr = NULL, *fsmMgr2 = NULL, *fsmMgr3 = NULL;
  Tr_Tr_t *tr = NULL;
  Ddi_Bdd_t *init = NULL, *invarspec, *invar = NULL, *rplus,
    *care = NULL, *assumeConstr = NULL;
  Ddi_Bddarray_t *rplusRings=NULL;
  Ddi_Bdd_t *invarspecBdd, *invarspecItp, *pPart, *pCore =
    NULL, *plitPs, *clitPs;
  Ddi_Bddarray_t *delta, *lambda, *pArray = NULL, *fromRings = NULL;
  Ddi_Varsetarray_t *abstrRefRefinedVars = NULL;
  long elapsedTime, startVerifTime, currTime, travTime, setupTime;
  int i, j, k, num_ff, heuristic, verificationResult = -1, itpBoundLimit = -1, prevAborted = 0, verificationEnded = 0;
  int loopReduce, loopReduceCnt, lastDdiNode, bddNodeLimit = -1;
  int deltaSize, deltaNum, bmcTimeLimit = -1, bddTimeLimit =
    -1, bddMemoryLimit = -1;
  int doRunBdd = 0, doRunBmc = 0, doRunBmcThreaded = 0, doRunInd =
    0, doRunLms = 0, doRunItp = 0, doRunRed = 0;
  int doRunLemmas = 0, bmcSteps, indSteps, lemmasSteps = opt->mc.lemmas;
  int doRunPdr = 0, pdrTimeLimit = -1, indTimeLimit = -1, itpTimeLimit = -1;
  int nCoiShell, nCoiVars, pPartNum, *coiPartSize = NULL,
    *propSize = NULL, *coiArraySize = NULL, *partOrdId = NULL, *partOrdId0 =
    NULL;
  Ddi_Varsetarray_t *coi, *coiPart, *coiArray = NULL;
  Ddi_Varset_t *hintVars = NULL;
  Ddi_Var_t *pvarPs, *pvarNs, *cvarPs, *cvarNs;
  int iProp = -1, iConstr = -1, nstate, size;
  Ddi_Bdd_t *deltaProp = NULL, *deltaConstr, *p, *pConj, *pDisj;
  int maxPart = opt->pre.specDecompMax;
  float coiRatio = 1.2;
  Ddi_Bdd_t *partInvarspec = NULL;
  int quitOnFailure = 0, done = 0;
  Fbv_DecompInfo_t *decompInfo;
  int multiProp = 0;
  int shareLemmas = 1;
  int prevNormCore = 0, normCore = abs(opt->pre.specDecompCore);
  int decrCore = normCore < 10 ? 1 : normCore / 10;
  Ddi_Bddarray_t *prevCexes = NULL;
  long bmcStepTime, umcStepTime;
  Fsm_Fsm_t *fsmFsmStore = NULL;
  int targetNvars = -1;
  int prevHit = -1;
  int useExpt = 0;
  int nPdrFrames = 0;
  int sameRingHit = 0;
  int totProved = 0, totFail = 0;
  Ddi_ClauseArray_t **pdrClauses = NULL;
  Ddi_Bdd_t *totProp = NULL;
  Ddi_Bdd_t *leftover = NULL;
  int doGenNew = 0, useTot = 0;
  float projVarsRate = 0.5;
  int replaceReached = 1; // do not and reached at each k - replace it
  int useFullPropAsConstr=0&&(opt->pre.specDecompCore>0);
  int igrFpRing = -1;
  int useRplusAsConstr = opt->trav.itpGfp>0;
  int useRplusAsCareWithItp = opt->trav.itpGfp>0;
  
  /**********************************************************************/
  /*                        Create DDI manager                          */
  /**********************************************************************/

  ddiMgr = Ddi_MgrInit("DDI_manager", NULL, 0,
    DDI_UNIQUE_SLOTS, DDI_CACHE_SLOTS * 10, 0, -1, -1);

  if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
    ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
    Ddi_MgrSetVerbosity(ddiMgr, Pdtutil_VerbLevel_e(opt->verbosity));
  }

  Ddi_MgrSiftEnable(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"));
  Ddi_MgrSetSiftThresh(ddiMgr, opt->ddi.siftTh);
  Ddi_MgrSetExistOptLevel(ddiMgr, opt->ddi.exist_opt_level);
  Ddi_MgrSetExistOptClustThresh(ddiMgr, opt->ddi.exist_opt_thresh);

  Ddi_MgrSetAigDynAbstrStrategy(ddiMgr, opt->ddi.dynAbstrStrategy);
  Ddi_MgrSetAigTernaryAbstrStrategy(ddiMgr, opt->ddi.ternaryAbstrStrategy);
  Ddi_MgrSetAigItpCore(ddiMgr, opt->ddi.itpCore);
  Ddi_MgrSetAigItpMem(ddiMgr, opt->ddi.itpMem);
  Ddi_MgrSetAigItpOpt(ddiMgr, opt->ddi.itpOpt);
  Ddi_MgrSetAigItpStore(ddiMgr, opt->ddi.itpStore);
  Ddi_MgrSetAigItpAbc(ddiMgr, opt->ddi.itpAbc);
  Ddi_MgrSetAigItpPartTh(ddiMgr, opt->ddi.itpPartTh);
  Ddi_MgrSetAigCnfLevel(ddiMgr, opt->ddi.aigCnfLevel);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpActiveVars_c, inum, opt->ddi.itpActiveVars);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStoreTh_c, inum, opt->ddi.itpStoreTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpAigCore_c, inum, opt->ddi.itpAigCore);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpTwice_c, inum, opt->ddi.itpTwice);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpIteOptTh_c, inum, opt->ddi.itpIteOptTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStructOdcTh_c, inum,
    opt->ddi.itpStructOdcTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiNnfClustSimplify_c, inum,
    opt->ddi.nnfClustSimplify);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMap_c, inum, opt->ddi.itpMap);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompute_c, inum, opt->ddi.itpCompute);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpDrup_c, inum, opt->ddi.itpDrup);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompact_c, inum, opt->ddi.itpCompact);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpClust_c, inum, opt->ddi.itpClust);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpNorm_c, inum, opt->ddi.itpNorm);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpSimp_c, inum, opt->ddi.itpSimp);

  /**********************************************************************/
  /*                        Create FSM manager                          */
  /**********************************************************************/

  opt->stats.startTime = util_cpu_time();

  fsmMgr = Fsm_MgrInit("fsm", NULL);
  Fsm_MgrSetDdManager(fsmMgr, ddiMgr);
  FbvSetFsmMgrOpt(fsmMgr, opt, 0);

  if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
    ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
    Fsm_MgrSetVerbosity(fsmMgr, Pdtutil_VerbLevel_e(opt->verbosity));
  }

  Fsm_MgrSetUseAig(fsmMgr, 1);
  Fsm_MgrSetCutThresh(fsmMgr, opt->fsm.cut);
  Fsm_MgrSetOption(fsmMgr, Pdt_FsmCutAtAuxVar_c, inum,
                   opt->fsm.cutAtAuxVar);

  /**********************************************************************/
  /*                        Read BLIF file                              */
  /**********************************************************************/

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "-- FSM Loading.\n"));
  if (strstr(fsm, ".fsm") != NULL) {
    if (strcmp(opt->mc.ord, "dfs") == 0) {
      Pdtutil_Free(opt->mc.ord);
      opt->mc.ord = NULL;
    }
    if (Fsm_MgrLoad(&fsmMgr, ddiMgr, fsm, opt->mc.ord, 1,
        Pdtutil_VariableOrderDefault_c) == 1) {
      fprintf(stderr, "-- FSM Loading Error.\n");
      exit(1);
    }
  } else if (strstr(fsm, ".aig") != NULL) {
    if (strcmp(opt->mc.ord, "dfs") == 0) {
      Pdtutil_Free(opt->mc.ord);
      opt->mc.ord = NULL;
    }
    if (Fsm_MgrLoadAiger(&fsmMgr, ddiMgr, fsm, opt->mc.ord, NULL,
        Pdtutil_VariableOrderDefault_c) == 1) {
      fprintf(stderr, "-- FSM Loading Error.\n");
      exit(1);
    }

    if (opt->pre.performAbcOpt == 1) {
      int ret = Fsm_MgrAbcReduceMulti(fsmMgr, 0.99);
    }
  } else {
    char **ci = Pdtutil_Alloc(char *, 4
    );
    int ciNum = 0;
    int myCoiLimit = 0;

    ci[0] = NULL;

    if (0 && strcmp(opt->mc.ord, "dfs") == 0) {
      Pdtutil_Free(opt->mc.ord);
      opt->mc.ord = NULL;
    }
    if (opt->pre.doCoi > 1) {
      if (opt->mc.invSpec != NULL) {
        Ddi_Expr_t *e = Ddi_ExprLoad(ddiMgr, opt->mc.invSpec, NULL);

        ci = Expr_ListStringLeafs(e);
        ciNum = 1;
        Ddi_Free(e);
      } else if (opt->mc.ctlSpec != NULL) {
        Ddi_Expr_t *e = Ddi_ExprLoad(ddiMgr, opt->mc.ctlSpec, NULL);

        ci = Expr_ListStringLeafs(e);
        ciNum = 1;
        Ddi_Free(e);
      } else if (opt->mc.all1) {
        ci[ciNum] = Pdtutil_Alloc(char,
          20
        );

        sprintf(ci[ciNum], "l %d %d %d", 0, opt->mc.all1 - 1,
          opt->pre.constrCoiLimit);
        ci[++ciNum] = NULL;
      } else if (opt->mc.idelta >= 0) {
        ci[ciNum] = Pdtutil_Alloc(char,
          20
        );

        sprintf(ci[ciNum], "l %d %d %d", opt->mc.idelta,
          opt->mc.idelta, opt->pre.constrCoiLimit);
        ci[++ciNum] = NULL;
      } else if (opt->mc.nlambda != NULL) {
        ci[ciNum] = Pdtutil_Alloc(char,
          strlen(opt->mc.nlambda) + 4
        );

        sprintf(ci[ciNum], "n %s %d", opt->mc.nlambda,
          opt->pre.constrCoiLimit);
        ci[++ciNum] = NULL;
        opt->mc.ilambda = 0;
      } else {
        ci[ciNum] = Pdtutil_Alloc(char,
          20
        );

        sprintf(ci[ciNum], "o %d %d %d %d", opt->mc.ilambda, opt->mc.ilambda,
          myCoiLimit, opt->pre.constrCoiLimit);
        ci[++ciNum] = NULL;
        if (opt->mc.ilambda > 0)
          opt->mc.ilambda = 0;
      }
    }
    if (opt->mc.ninvar != NULL) {
      ci[ciNum] = Pdtutil_Alloc(char,
        strlen(opt->mc.ninvar) + 4
      );

      sprintf(ci[ciNum], "i %s %d", opt->mc.ninvar, opt->pre.constrCoiLimit);
      ci[++ciNum] = NULL;
    }
    if (Fsm_MgrLoadFromBlifWithCoiReduction(&fsmMgr, ddiMgr, fsm, opt->mc.ord,
        1, Pdtutil_VariableOrderDefault_c, ci) == 1) {
      fprintf(stderr, "BLIF File Loading Error.\n");
    }
    if (ci != NULL) {
      Pdtutil_StrArrayFree(ci, -1);
    }
  }

  /* COMMON AIG-BDD PART */
  opt->stats.setupTime = util_cpu_time();

  if (0 && Ddi_BddarraySize(Fsm_MgrReadDeltaBDD(fsmMgr)) > 20000) {
    int aol = Ddi_MgrReadAigAbcOptLevel(ddiMgr);

    Ddi_MgrSetAigAbcOptLevel(ddiMgr, 2);
    Ddi_AigarrayAbcOptAcc(Fsm_MgrReadDeltaBDD(fsmMgr), 60.0);
    Ddi_MgrSetAigAbcOptLevel(ddiMgr, aol);
  }

  startVerifTime = util_cpu_time();
  pi = Fsm_MgrReadVarI(fsmMgr);
  ps = Fsm_MgrReadVarPS(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  deltaNum = delta == NULL ? 0 : Ddi_BddarrayNum(delta);
  lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  //  opt->expt.expertLevel = 2;
  if (opt->expt.expertLevel > 0) {
    if (0 && (1 || deltaNum < 5500)) {
      opt->mc.tuneForEqCheck = 1;
      opt->pre.findClk = 1;
      opt->pre.terminalScc = 1;
      opt->pre.impliedConstr = 1;
      opt->pre.specConj = 1;
    }
    // opt->pre.peripheralLatches = 1;
    if (opt->expt.expertArgv != NULL) {
      FbvParseArgs(opt->expt.expertArgc, opt->expt.expertArgv, opt);
    }
  }

  if (Ddi_BddarrayNum(delta) > 20000) {
    /* too big */
    opt->mc.nogroup = 1;
    Ddi_MgrSetSiftThresh(ddiMgr, 10000000);
  }

  if (!opt->mc.nogroup) {
    Ddi_MgrCreateGroups2(ddiMgr, ps, ns);
  }

  if (ps == NULL && opt->mc.checkMult) {
    FbvCombCheck(fsmMgr, Fbv_Mult_c);
    return 0;
  }

  if (ps != NULL) {
    psvars = Ddi_VarsetMakeFromArray(ps);
    nsvars = Ddi_VarsetMakeFromArray(ns);
  } else {
    opt->trav.noInit = 1;
    opt->mc.combinationalProblem = 1;
    psvars = nsvars = NULL;
  }

  if (opt->trav.noInit) {
    init = Ddi_BddMakeConst(ddiMgr, 0);
  } else if (opt->mc.rInit != NULL) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "-- INIT Set Loading From %s.\n", opt->mc.rInit));
    init = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
      DDDMP_MODE_DEFAULT, opt->mc.rInit, NULL);
    Ddi_MgrReduceHeap(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"), 0);
    Ddi_BddSetAig(init);
    Ddi_MgrSetSiftThresh(ddiMgr, opt->ddi.siftTh);
  } else {
    init = Ddi_BddDup(Fsm_MgrReadInitBDD(fsmMgr));
  }
  Fsm_MgrSetInitBDD(fsmMgr, init);
  Ddi_Free(init);

  if (opt->common.clk != NULL) {
    Fsm_MgrSetClkStateVar(fsmMgr, opt->common.clk, 1);
  }

  if (opt->mc.ninvar != NULL) {
    invar = NULL;
    for (i = 0; i < Ddi_BddarrayNum(lambda); i++) {
      char *name = Ddi_ReadName(Ddi_BddarrayRead(lambda, i));

      if (name != NULL && strcmp(name, opt->mc.ninvar) == 0) {
        invar = Ddi_BddDup(Ddi_BddarrayRead(lambda, i));
        break;
      }
    }
    Pdtutil_Assert(invar != NULL, "Lambda not found by name");
    Fsm_MgrSetConstraintBDD(fsmMgr,invar);
  } else if (Fsm_MgrReadConstraintBDD(fsmMgr) != NULL) {
    invar = Ddi_BddDup(Fsm_MgrReadConstraintBDD(fsmMgr));
  }

  if (opt->mc.ilambda >= Ddi_BddarrayNum(lambda)) {
    opt->mc.ilambda = Ddi_BddarrayNum(lambda) - 1;
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "ilambda selection is too large: %d forced\n",
        opt->mc.ilambda));
  }

  if (opt->trav.noCheck) {
    invarspec = Ddi_BddMakeConstAig(ddiMgr, 1);

    if (opt->mc.method == Mc_VerifPrintNtkStats_c) {
      Ddi_Vararray_t *pi, *ps;
      Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);

      pi = Fsm_MgrReadVarI(fsmMgr);
      ps = Fsm_MgrReadVarPS(fsmMgr);

      Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "%d inputs / %d state variables / %d delta nodes \n",
          Ddi_VararrayNum(pi), Ddi_VararrayNum(ps), Ddi_BddarraySize(delta)));
    }
  } else {
    if (opt->mc.ilambda < 0) {
      partInvarspec = Ddi_BddMakeConstAig(ddiMgr, 1);
    }
    invarspec = FbvGetSpec(fsmMgr, NULL, invar, partInvarspec, opt);
    if (partInvarspec != NULL) {
      Fsm_MgrSetInvarspecBDD(fsmMgr, partInvarspec);
    } else {
      Fsm_MgrSetInvarspecBDD(fsmMgr, invarspec);
    }
    multiProp = 1;
    Ddi_Free(invarspec);

  }

  {
    Fsm_Fsm_t *fsmFsm = Fsm_FsmMakeFromFsmMgr(fsmMgr);

    Fsm_FsmFoldProperty(fsmFsm, opt->mc.compl_invarspec,
      opt->trav.cntReachedOK, 1);
    Fsm_FsmFoldConstraint(fsmFsm, opt->mc.compl_invarspec);
    //    Fsm_FsmFoldInit(fsmFsm);
    Fsm_FsmWriteToFsmMgr(fsmMgr, fsmFsm);
    Fsm_FsmFree(fsmFsm);
    if (opt->fsm.insertCutLatches > 0) {
      Fsm_Mgr_t *fsmMgrNew = Fsm_RetimeGateCuts(fsmMgr, opt->fsm.insertCutLatches);
      Fsm_MgrQuit(fsmMgr);
      fsmMgr = fsmMgrNew;
    }
    invarspec = Ddi_BddDup(Fsm_MgrReadInvarspecBDD(fsmMgr));
    if (partInvarspec == NULL) {
      Fsm_MgrSetInvarspecBDD(fsmMgr, NULL);
    } else {
      Fsm_MgrSetInvarspecBDD(fsmMgr, partInvarspec);
    }
  }
  Ddi_Free(partInvarspec);

  pi = Fsm_MgrReadVarI(fsmMgr);
  ps = Fsm_MgrReadVarPS(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  deltaNum = delta == NULL ? 0 : Ddi_BddarrayNum(delta);
  lambda = Fsm_MgrReadLambdaBDD(fsmMgr);

  opt->stats.setupTime = util_cpu_time();

  if (!multiProp &&
    (opt->mc.combinationalProblem || Ddi_BddIsConstant(invarspec))) {
    Ddi_BddNotAcc(invarspec);
    verificationResult = Ddi_AigSat(invarspec);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: %s.\n",
        verificationResult ? "FAIL" : "PASS"));
    Ddi_BddNotAcc(invarspec);
    goto quitMem;
  } else if (Ddi_BddIsConstant(invarspec)) {
    Ddi_BddNotAcc(invarspec);
    verificationResult = Ddi_AigSat(invarspec);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: %s.\n",
        verificationResult ? "FAIL" : "PASS"));
    Ddi_BddNotAcc(invarspec);
    goto quitMem;
  }

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "Reductions running.\n"));
  if (0 && Ddi_BddarraySize(Fsm_MgrReadDeltaBDD(fsmMgr)) > 20000) {
    int aol = Ddi_MgrReadAigAbcOptLevel(ddiMgr);

    Ddi_MgrSetAigAbcOptLevel(ddiMgr, 2);
    Ddi_AigarrayAbcOptAcc(Fsm_MgrReadDeltaBDD(fsmMgr), 60.0);
    Ddi_MgrSetAigAbcOptLevel(ddiMgr, aol);
  }

  delta = Fsm_MgrReadDeltaBDD(fsmMgr);

  if (1 && opt->pre.peripheralLatches) {
    int nRed;
    do {
      nRed = Fsm_RetimePeriferalLatches(fsmMgr);
      opt->pre.peripheralLatchesNum += nRed;
    } while (nRed > 0);
    Ddi_Free(invar);
    invar = Ddi_BddDup(Fsm_MgrReadConstraintBDD(fsmMgr));
    if (care != NULL) {
      Ddi_Varset_t *psVars = Ddi_VarsetMakeFromArray(Fsm_MgrReadVarPS(fsmMgr));

      Ddi_BddExistProjectAcc(care, psVars);
      Ddi_Free(psVars);
    }
  }

  if (opt->pre.findClk) {
    Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
    Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
    int nstate = Ddi_VararrayNum(ps);

    for (i = 0; i < nstate; i++) {
      Ddi_Bdd_t *dClk0, *dClk1;
      Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);

      dClk0 = Ddi_BddCofactor(d_i, v_i, 0);
      Ddi_BddNotAcc(dClk0);
      dClk1 = Ddi_BddCofactor(d_i, v_i, 1);
      if (!Ddi_AigSat(dClk1) && !Ddi_AigSat(dClk0)) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "clock found: %s\n", Ddi_VarName(v_i)));
        opt->pre.clkVarFound = v_i;
      } else {
        Ddi_Varset_t *s_i = Ddi_BddSupp(d_i);

        if (Ddi_VarsetNum(s_i) == 1) {
          Ddi_Var_t *v_in = Ddi_VarsetTop(s_i);

          for (j = 0; j < nstate; j++) {
            if (Ddi_VararrayRead(ps, j) == v_in) {
              break;
            }
          }
          if (j < nstate) {
            Ddi_Bdd_t *d_j = Ddi_BddarrayRead(delta, j);
            Ddi_Varset_t *s_j = Ddi_BddSupp(d_j);

            if (Ddi_VarsetNum(s_j) == 1 && Ddi_VarInVarset(s_j, v_i)) {
              if (Ddi_BddIsComplement(d_i) && Ddi_BddIsComplement(d_j)) {
                Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
                Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
                int eqLatches = 1;
                int diffLatches = 1;

                Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
                  Pdtutil_VerbLevelNone_c,
                  fprintf(stdout, "clock pair found: %s - %s\n",
                    Ddi_VarName(v_i), Ddi_VarName(v_in)));

                opt->pre.clkVarFound = v_i;

                /* check init state !!! */
                if (initStub != NULL) {
                  Ddi_Bdd_t *st_i = Ddi_BddarrayRead(initStub, i);
                  Ddi_Bdd_t *st_j = Ddi_BddarrayRead(initStub, j);
                  Ddi_Bdd_t *st_jn = Ddi_BddNot(st_j);

                  eqLatches = Ddi_BddEqualSat(st_i, st_j);
                  diffLatches = Ddi_BddEqualSat(st_i, st_jn);
                  Ddi_Free(st_jn);
                } else {
                  Ddi_Bdd_t *dif = Ddi_BddMakeLiteralAig(v_i, 1);
                  Ddi_Bdd_t *aux = Ddi_BddMakeLiteralAig(v_in, 1);

                  Ddi_BddDiffAcc(dif, aux);
                  Ddi_Free(aux);
                  eqLatches = Ddi_AigSatAnd(init, dif, NULL);
                  Ddi_BddNotAcc(dif);
                  diffLatches = Ddi_AigSatAnd(init, dif, NULL);
                  Ddi_Free(dif);
                }
                if (eqLatches && !diffLatches) {
                  /* OK clock */
                  Ddi_DataCopy(d_i, d_j);
                } else {
                  /* undo clk */
                  opt->pre.clkVarFound = NULL;
                }
              }
            }
            Ddi_Free(s_j);
          }
        }
        Ddi_Free(s_i);
      }
      Ddi_Free(dClk0);
      Ddi_Free(dClk1);
    }
  }

  if (opt->pre.terminalScc) {
    opt->stats.verificationOK = 0;
    Ddi_Bdd_t *myInvar = Fsm_ReduceTerminalScc(fsmMgr,
      &opt->pre.terminalSccNum,
      &opt->stats.verificationOK, opt->trav.itpConstrLevel);

    if (opt->stats.verificationOK == 1) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Heuristic strategy selection: TSCC.\n");
        fprintf(stdout, "Verification Result: PASS.\n")
        );
      verificationResult = 0;
      Ddi_Free(myInvar);
      init = NULL;
      goto quitMem;
    }
    if (myInvar != NULL) {
      if (invar == NULL) {
        invar = myInvar;
      } else {
        Ddi_BddAndAcc(invar, myInvar);
        Ddi_Free(myInvar);
      }
    }
  }

  if (opt->pre.impliedConstr) {
    Ddi_Bdd_t *myInvar = Fsm_ReduceImpliedConstr(fsmMgr,
      opt->pre.specDecompIndex, invar, &opt->pre.impliedConstrNum);

    if (myInvar != NULL) {
      if (invar == NULL) {
        invar = myInvar;
      } else {
        Ddi_BddAndAcc(invar, myInvar);
        Ddi_Free(myInvar);
      }
    }
  }

  opt->stats.setupTime = util_cpu_time();

  // FbvTestSec (fsmMgr, opt);
  FbvTestRelational(fsmMgr, opt);

  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  deltaSize = Ddi_BddarraySize(delta);
  num_ff = Ddi_BddarrayNum(delta);

  if (useExpt) {

    heuristic = FbvSelectHeuristic(fsmMgr, opt, 1); // multi
    doRunRed = 1;
    bmcSteps = 100;
    bmcTimeLimit = 20;
    indSteps = 30;
    indTimeLimit = 10;
    //lemmasSteps = 0;
    pdrTimeLimit = 120;
    // opt->trav.lemmasTimeLimit = 35 + 25*(deltaSize>3000);
    // opt->mc.method = Mc_VerifExactFwd_c;

    opt->ddi.dynAbstrStrategy = 1;

    //  doRunItp = 1;

    switch (heuristic) {
      case Fbv_HeuristicBmcBig_c:  /* BMC */
        // opt->pre.doCoi = 0;
        // doRunBmc = 1;
        bmcSteps = -1;
        bmcTimeLimit = 100;
        opt->pre.noTernary = 1;
        opt->pre.ternarySim = 0;
        break;
      case Fbv_HeuristicIntel_c:   /* intel */
        opt->pre.reduceCustom = 1;
      case Fbv_HeuristicIbm_c: /* intel */
        doRunPdr = 1;
        doRunItp = 1;
        itpTimeLimit = 800;
        pdrTimeLimit = 100;

        opt->pre.twoPhaseRed = 1;

        /* itp4 setting */
        opt->mc.aig = 1;
        // opt->trav.itpPart = 1;
        lemmasSteps = 2 * (deltaSize < 10000);
        // opt->trav.lemmasTimeLimit = 80;
        // opt->pre.performAbcOpt = 0;
        opt->pre.abcSeqOpt = 0;
        opt->ddi.aigRedRemPeriod = 100;
        opt->mc.lazy = 1;
        opt->common.lazyRate = 1.0;
        // opt->trav.dynAbstr = 2;
        // opt->trav.itpAppr = 1;
        // opt->trav.implAbstr = 2;

#if 1
        if (num_ff < 190 || num_ff > 250) {
          /* itp0 */
          lemmasSteps = 0;
          // opt->trav.itpAppr = 0;
          // opt->trav.implAbstr = 0;
          opt->trav.ternaryAbstr = 0;
          // opt->common.lazyRate = 0.9;
          // opt->trav.dynAbstr = 0;
          // opt->ddi.aigBddOpt = 0;
          // opt->common.aigAbcOpt = 0;
          // opt->trav.itpEnToPlusImg = 0;
        }
#endif
        break;
      case Fbv_HeuristicDme_c: /* dme */
        doRunBdd = 1;
        opt->fsm.cut = 200;
        break;
      case Fbv_HeuristicQueue_c:   /* queue */
        opt->pre.peripheralLatches = 0;
        doRunPdr = 0;
        doRunBdd = 1;
        opt->fsm.cut = 10000;
        // opt->trav.imgDpartTh = 100000;
        break;
      case Fbv_HeuristicNeedham_c: /* ns2-ns3 */
        doRunPdr = 1;
        doRunInd = 1;
        doRunBdd = 1;
        indSteps = 20;
        indTimeLimit = 10;
        opt->trav.clustTh = 300;
        opt->trav.imgDpartTh = 500000;
        break;
      case Fbv_HeuristicSec_c: /* sec */
        doRunLemmas = 1;
        doRunItp = 1;
        // opt->trav.lemmasTimeLimit = -1;
        opt->expt.lemmasSteps = 10;
        opt->pre.retime = 1;

        /* itp5 setting for interpolation (just in case ...) */
        opt->mc.aig = 1;
        opt->pre.peripheralLatches = 1;
        // opt->pre.performAbcOpt = 0;
        opt->pre.abcSeqOpt = 0;
        opt->ddi.aigRedRemPeriod = 100;
        opt->mc.lazy = 1;
        // opt->common.lazyRate = 2.3;
        // opt->trav.itpPart = 1;
        // opt->trav.itpAppr = 1;
        // opt->trav.dynAbstr = 2;
        // opt->trav.implAbstr = 2;
        // opt->trav.ternaryAbstr = 0;
        itpBoundLimit = -1;
        itpTimeLimit = 800;

        break;
      case Fbv_HeuristicPdtSw_c:   /* pdt-sw */
        doRunBmc = 1;
        doRunBdd = 1;
        if (num_ff < 75 && num_ff > 65) {
          opt->mc.method = Mc_VerifApproxFwdExactBck_c;
          // opt->mc.method = Mc_VerifExactBck_c;
          opt->trav.apprOverlap = 0;
          opt->trav.apprGrTh = 1000;
          opt->trav.clustTh = 5000;
          opt->fsm.cut = 1000;
          opt->trav.imgDpartTh = 300000;
        } else {
          opt->fsm.cut = -1;
          opt->trav.clustTh = 500;
        }
        bmcSteps = 70;
        bmcTimeLimit = 100;
        bddTimeLimit = 800;

        break;
      case Fbv_HeuristicBddFull_c: /* full-bdd */
        doRunBmc = 1;
        doRunInd = 1;
        doRunPdr = 1;
        doRunItp = 1;
        doRunBdd = 1;

        bmcSteps = 35;
        bmcTimeLimit = 15;
        indSteps = 10;
        indTimeLimit = 10;
        opt->trav.lemmasTimeLimit = 15;
        itpBoundLimit = 100;
        itpTimeLimit = 30;
        pdrTimeLimit = 55;
        bddTimeLimit = 800;

        /* itp5 setting */
        opt->mc.aig = 1;
        // lemmasSteps = 0;
        opt->pre.peripheralLatches = 1;
        // opt->pre.performAbcOpt = 0;
        opt->pre.abcSeqOpt = 0;
        opt->ddi.aigRedRemPeriod = 100;
        opt->mc.lazy = 1;
        // opt->common.lazyRate = 1.0;
        // opt->trav.dynAbstr = 2;
        // opt->trav.itpAppr = 0;
        // opt->trav.itpInductiveTo = 1;
        // opt->trav.implAbstr = 3;
        // opt->trav.ternaryAbstr = 0;

        opt->trav.clustTh = 5000;
        opt->fsm.cut = -1;
        opt->trav.imgDpartTh = 100000;
        break;
      case Fbv_HeuristicBdd_c: /* bdd */
        doRunBmc = 1;
        doRunPdr = 1;
        doRunItp = 1;
        doRunBdd = 1;

        bmcSteps = 40;
        bmcTimeLimit = 30;
        // lemmasSteps = 2*(deltaSize<7000); //lemmi abilitati con l'interpolante
        // opt->trav.lemmasTimeLimit = 40;
        itpBoundLimit = 100;
        itpTimeLimit = 60;
        pdrTimeLimit = 120;
        bddTimeLimit = 700;

        /* itp4 setting */
        opt->mc.aig = 1;
        // opt->trav.itpPart = 1;
        opt->pre.peripheralLatches = 1;
        // opt->pre.performAbcOpt = 0;
        opt->pre.abcSeqOpt = 0;
        opt->ddi.aigRedRemPeriod = 100;
        opt->mc.lazy = 1;
        opt->common.lazyRate = 1.0;
        // opt->trav.dynAbstr = 2;
        // opt->trav.itpAppr = 0;
        // opt->trav.itpInductiveTo = 1;
        // opt->trav.implAbstr = 3;
        opt->trav.ternaryAbstr = 0;

        opt->fsm.cut = 500;
        opt->trav.imgDpartTh = 100000;
        break;

      case Fbv_HeuristicBeem_c:    /* beem */
        opt->expt.doRunBmc = 1;
        opt->expt.doRunInd = 0;
        opt->expt.doRunItp = 1;
        // opt->pre.retime = 1;

        opt->expt.bmcSteps = 50;
        opt->expt.bmcTimeLimit = 40;
        opt->expt.indSteps = 10;
        opt->expt.indTimeLimit = 20;
        opt->expt.lemmasSteps = 2 * (deltaSize < 10000);
        opt->trav.lemmasTimeLimit = 60;
        opt->expt.itpBoundLimit = -1;
        opt->expt.itpTimeLimit = -1;

        /* itp1 setting */
        opt->mc.aig = 1;
        // opt->trav.itpPart = 1;
        opt->pre.peripheralLatches = 1;
        // opt->pre.performAbcOpt = 0;
        opt->pre.abcSeqOpt = 0;
        opt->ddi.aigRedRemPeriod = 100;
        opt->mc.lazy = 1;
        opt->common.lazyRate = 1.0;
        // opt->trav.dynAbstr = 2;
        opt->trav.implAbstr = 0;
        opt->trav.ternaryAbstr = 0;
        break;

      case Fbv_HeuristicRelational_c:  /* other */
        opt->pre.reduceCustom = 1;
      case Fbv_HeuristicOther_c:   /* other */
        doRunBmc = 1;
        doRunPdr = 1;
        doRunItp = 1;
        opt->pre.retime = 1;

        bmcSteps = 500;
        bmcTimeLimit = 40;
        //      lemmasSteps = 2*(deltaSize<25000);
        //      opt->trav.lemmasTimeLimit = 1000;
        itpBoundLimit = -1;
        itpTimeLimit = 800;

        /* itp4 setting */
        opt->mc.aig = 1;
        // opt->trav.itpPart = 1;
        opt->pre.peripheralLatches = 1;
        // opt->pre.performAbcOpt = 0;
        opt->pre.abcSeqOpt = 0;
        opt->ddi.aigRedRemPeriod = 100;
        // opt->mc.lazy = 1;
        // opt->common.lazyRate = 1.0;
        // opt->trav.dynAbstr = 2;
        // opt->trav.itpAppr = 0;
        // opt->trav.implAbstr = 3;
        // opt->trav.ternaryAbstr = 0;

        break;
      case Fbv_HeuristicNone_c:
        break;
      default:                 /* not possible */
        printf("Unknown verification strategy!");
        exit(1);
    }
    if (opt->expt.expertArgv != NULL) {
      FbvParseArgs(opt->expt.expertArgc, opt->expt.expertArgv, opt);
    }
  }

  Ddi_MgrSetAigDynAbstrStrategy(ddiMgr, opt->ddi.dynAbstrStrategy);
  Ddi_MgrSetAigItpCore(ddiMgr, opt->ddi.itpCore);
  Ddi_MgrSetAigItpMem(ddiMgr, opt->ddi.itpMem);
  Ddi_MgrSetAigItpOpt(ddiMgr, opt->ddi.itpOpt);
  Ddi_MgrSetAigItpStore(ddiMgr, opt->ddi.itpStore);
  Ddi_MgrSetAigItpAbc(ddiMgr, opt->ddi.itpAbc);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpAigCore_c, inum, opt->ddi.itpAigCore);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpTwice_c, inum, opt->ddi.itpTwice);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpIteOptTh_c, inum, opt->ddi.itpIteOptTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStructOdcTh_c, inum,
    opt->ddi.itpStructOdcTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiNnfClustSimplify_c, inum,
    opt->ddi.nnfClustSimplify);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMap_c, inum, opt->ddi.itpMap);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompute_c, inum, opt->ddi.itpCompute);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpDrup_c, inum, opt->ddi.itpDrup);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompact_c, inum, opt->ddi.itpCompact);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpClust_c, inum, opt->ddi.itpClust);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpNorm_c, inum, opt->ddi.itpNorm);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpSimp_c, inum, opt->ddi.itpSimp);


  // opt->trav.itpAppr = 3;
  // lemmasSteps = 2;
  if (opt->mc.bdd) {
    doRunBdd = 1;
    doRunItp = 0;
  }

  opt->stats.setupTime = util_cpu_time();

  if (opt->pre.reduceCustom) {
    if (invarspec != NULL) {
      Ddi_BddarrayWrite(lambda, 0, invarspec);
    }
    Ddi_Bdd_t *myInvar = Fsm_ReduceCustom(fsmMgr);

    Ddi_Free(myInvar);
    // Ddi_Free(init);
    // init = Ddi_BddDup(Fsm_MgrReadInitBDD (fsmMgr));
    if (invarspec != NULL) {
      Ddi_DataCopy(invarspec, Ddi_BddarrayRead(lambda, 0));
    }
    if (0) {
      int i, n;
      Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(pi);

      delta = Fsm_MgrReadDeltaBDD(fsmMgr);

      n = Ddi_BddarrayNum(delta);
      for (i = 0; i < n; i++) {
        Ddi_Bdd_t *p_i = Ddi_BddarrayRead(delta, i);

        if (Ddi_BddSize(p_i) == 1) {
          Ddi_Var_t *v = Ddi_BddTopVar(p_i);

          logBdd(p_i);
          if (Ddi_VarInVarset(piv, v)) {
            Ddi_Varset_t *s_i;

            p_i = Ddi_BddarrayExtract(delta, i);
            s_i = Ddi_BddarraySupp(delta);
            Ddi_BddarrayInsert(delta, i, p_i);
            if (Ddi_VarsetIsVoid(s_i)) {
              Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
                Pdtutil_VerbLevelNone_c,
                fprintf(stdout, "Free latch found.\n"));
            } else {
              int j;

              for (j = 0; j < n; j++) {
                if (j != i) {
                  Ddi_Bdd_t *p_j = Ddi_BddarrayRead(delta, j);
                  Ddi_Varset_t *s_j = Ddi_BddSupp(p_j);

                  if (Ddi_VarInVarset(s_j, v)) {
                    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
                      Pdtutil_VerbLevelNone_c, fprintf(stdout, "Found.\n"));
                  }
                  Ddi_Free(s_j);
                }
              }
            }
            Ddi_Free(s_i);
            Ddi_Free(p_i);
          }
        }
      }

      Ddi_Free(piv);
    }
  }

  if (((opt->pre.twoPhaseRed && opt->pre.clkVarFound != NULL) ||
      opt->pre.twoPhaseForced)) {
    int ph = -1;
    int size0 = Ddi_BddarraySize(delta);
    Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
    Ddi_Bdd_t *phase, *init = Fsm_MgrReadInitBDD(fsmMgr);
    Ddi_Vararray_t *twoPhasePis =
      Ddi_VararrayMakeNewVars(pi, "PDT_ITP_TWOPHASE_PI", "0", 1);
    Ddi_Bddarray_t *twoPhasePiLits =
      Ddi_BddarrayMakeLiteralsAig(twoPhasePis, 1);
    Ddi_Bddarray_t *twoPhaseDelta = Ddi_BddarrayDup(delta);
    Ddi_Bddarray_t *twoPhaseDelta2 = Ddi_BddarrayDup(delta);

    Ddi_AigarrayComposeAcc(twoPhaseDelta, pi, twoPhasePiLits);
    Ddi_AigarrayComposeNoMultipleAcc(twoPhaseDelta, ps, twoPhaseDelta2);

    if (opt->pre.twoPhaseRedPlus && initStub != NULL) {
      Ddi_Bddarray_t *initStubPlus = Ddi_BddarrayDup(delta);
      Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
      char suffix[4];
      Ddi_Vararray_t *newPiVars;
      Ddi_Bddarray_t *newPiLits;

      sprintf(suffix, "%d", fsmMgr->stats.initStubSteps++);
      newPiVars = Ddi_VararrayMakeNewVars(pi, "PDT_STUB_PI", suffix, 1);
      newPiLits = Ddi_BddarrayMakeLiteralsAig(newPiVars, 1);
      Ddi_BddarrayComposeAcc(initStubPlus, pi, newPiLits);
      Ddi_BddarrayComposeAcc(initStubPlus, ps, initStub);
      Ddi_Free(newPiVars);
      Ddi_Free(newPiLits);
      Fsm_MgrSetInitStubBDD(fsmMgr, initStubPlus);
      initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
      Ddi_Free(initStubPlus);
    }

    Ddi_DataCopy(delta, twoPhaseDelta);
    opt->pre.twoPhaseDone = 1;
    Fsm_MgrSetPhaseAbstr(fsmMgr, 1);
    Pdtutil_WresSetTwoPhase();

    Ddi_VararrayAppend(pi, twoPhasePis);

    if (opt->pre.clkVarFound != NULL && opt->pre.twoPhaseRed) {
      phase = Ddi_BddMakeLiteralAig(opt->pre.clkVarFound, 1);
      if (initStub != NULL) {
        Ddi_BddComposeAcc(phase, ps, initStub);
      } else {
        Ddi_BddAndAcc(phase, init);
      }

      if (!Ddi_AigSat(phase)) {
        ph = 0;
      } else if (!Ddi_AigSat(Ddi_BddNotAcc(phase))) {
        ph = 1;
      }
      if (ph >= 0 && opt->pre.clkPhaseSuggested >= 0
        && ph != opt->pre.clkPhaseSuggested) {
        ph = opt->pre.clkPhaseSuggested;
        opt->pre.twoPhaseRedPlus = 1;
      }
      Ddi_Free(phase);
      if (ph >= 0 && opt->pre.twoPhaseOther) {
        opt->pre.twoPhaseRedPlus = !opt->pre.twoPhaseRedPlus;
        ph = !ph;
      }
    }

    printf("Two phase reduction: %d -> %d\n", size0, Ddi_BddarraySize(delta));

    Ddi_Free(twoPhasePis);
    Ddi_Free(twoPhasePiLits);
    Ddi_Free(twoPhaseDelta);
    Ddi_Free(twoPhaseDelta2);

  }

  //  lemmasSteps = 2;
  lemmasSteps = opt->mc.lemmas;

  if (invar != NULL) {
    Fsm_MgrSetConstraintBDD(fsmMgr, invar);
  }

  Ddi_Free(invar);

  if (opt->pre.performAbcOpt == 2) {
    //  int ret = Fsm_MgrAbcReduce(fsmMgr, 0.99);
    // folding/unfolding to be fixed in ...Multi
    int ret = Fsm_MgrAbcReduceMulti(fsmMgr, 0.99);
  }

  opt->stats.setupTime = util_cpu_time();

  if (useFullPropAsConstr) {
    Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
    Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
    int nl = Fsm_MgrReadNL(fsmMgr);
    char buf[256];
    sprintf(buf, "PDT_BDD_INVAR2_VAR$PS");
    Ddi_Var_t *invar2Ps = Ddi_VarNew(ddiMgr);
    Ddi_VarAttachName(invar2Ps, buf);
    sprintf(buf, "PDT_BDD_INVAR2_VAR$NS");
    Ddi_Var_t *invar2Ns = Ddi_VarNew(ddiMgr);
    Ddi_VarAttachName(invar2Ns, buf);
    int iConstr = nl-2;
    Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
    Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
    Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
    Ddi_Bdd_t *c2 = Ddi_BddDup(Ddi_BddarrayRead(delta,nl-1));
    Ddi_Bdd_t *cLit = Ddi_BddMakeLiteralAig(invar2Ps, 1);
    Ddi_Bdd_t *constr = Ddi_BddarrayRead(delta,nl-2);
    Ddi_BddAndAcc(constr,cLit);
    pvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
    Ddi_BddCofactorAcc(c2, pvarPs, 1);      
    Ddi_BddarrayInsert(delta,iConstr,c2);
    if (initStub==NULL) {
      Ddi_BddAndAcc(init,cLit);
    }
    else {
      Ddi_Bdd_t *initConstr = Ddi_BddMakeConstAig(ddiMgr, 1);
      Ddi_BddarrayInsert(initStub,iConstr,initConstr);
      Ddi_Free(initConstr);
    }
    Ddi_Free(c2);
    Ddi_Free(cLit);
    Ddi_VararrayInsert(ps,iConstr,invar2Ps);
    Ddi_VararrayInsert(ns,iConstr,invar2Ns);
  }

  /* PROPERTY DECOMPOSITION */
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  ps = Fsm_MgrReadVarPS(fsmMgr);
  pvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
  pvarNs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$NS");
  cvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
  cvarNs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$NS");
  iProp = iConstr = -1;
  nstate = Ddi_BddarrayNum(delta);
  size = Ddi_BddSize(invarspec);
  for (i = 0; i < nstate; i++) {
    Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);

    if (v_i == pvarPs) {
      iProp = i;
    } 
    if (v_i == cvarPs) {
      iConstr = i;
    }
  }

  if (opt->trav.countReached) {
    Ddi_Vararray_t *tSupp;
    Ddi_Bdd_t *myTarget = Ddi_BddNot(Ddi_BddarrayRead(delta, iProp));

    Ddi_BddCofactorAcc(myTarget, pvarPs, 1);
    Ddi_BddCofactorAcc(myTarget, cvarPs, 1);
    tSupp = Ddi_BddSuppVararray(myTarget);
    Ddi_VararrayIntersectAcc(tSupp, ps);
    targetNvars = Ddi_VararrayNum(tSupp);
    Ddi_Free(tSupp);
    if (opt->trav.countReached > 1) {
      Ddi_Bdd_t *tBdd = Ddi_BddMakeMono(myTarget);

      Ddi_BddExistProjectAcc(tBdd, psvars);
      fprintf(stdout, "#TOTAL TargetStates(exact)=%g (#ref vars: %d)\n",
        Ddi_BddCountMinterm(tBdd, targetNvars), targetNvars);
      Ddi_Free(tBdd);
    }
    fprintf(stdout, "#TOTAL TargetStates(estimated)=%g (#ref vars: %d)\n",
      Ddi_AigEstimateMintermCount(myTarget, targetNvars), targetNvars);
    Ddi_Free(myTarget);
  }


  Pdtutil_Assert(iProp < nstate, "PDT_BDD_INVARSPEC_VAR$PS var not found");

  decompInfo = FbvPropDecomp(fsmMgr, opt);

  pArray = decompInfo->pArray;
  partOrdId = decompInfo->partOrdId;
  coiArray = decompInfo->coiArray;
  hintVars = decompInfo->hintVars;
  if (hintVars!=NULL) {
    Ddi_VarsetSubstVarsAcc(hintVars,ps,ns);
  }
  // opt->common.lazyRate = 1.0;
  // opt->trav.dynAbstr = 2;
  // opt->trav.itpAppr = 1;
  // opt->trav.implAbstr = 2;
  // opt->trav.ternaryAbstr = 0;
  Ddi_MgrSetAigBddOptLevel(ddiMgr, opt->ddi.aigBddOpt);
  Ddi_MgrSetAigAbcOptLevel(ddiMgr, opt->common.aigAbcOpt);
  Ddi_MgrSetAigRedRemLevel(ddiMgr, opt->ddi.aigRedRem);
  Ddi_MgrSetAigRedRemMaxCnt(ddiMgr, opt->ddi.aigRedRemPeriod);
  Ddi_MgrSetSatSolver(ddiMgr, opt->common.satSolver);
  Ddi_MgrSetAigSatTimeout(ddiMgr, opt->ddi.satTimeout);
  Ddi_MgrSetAigSatTimeLimit(ddiMgr, opt->ddi.satTimeLimit);
  Ddi_MgrSetAigSatIncremental(ddiMgr, opt->ddi.satIncremental);
  Ddi_MgrSetAigSatVarActivity(ddiMgr, opt->ddi.satVarActivity);
  Ddi_MgrSetAigLazyRate(ddiMgr, opt->common.lazyRate);

  plitPs = Ddi_BddMakeLiteralAig(pvarPs, 1);
  if (cvarPs) {
    clitPs = Ddi_BddMakeLiteralAig(cvarPs, 0);
  } else {
    clitPs = Ddi_BddMakeConstAig(ddiMgr, 0);
  }

  opt->stats.setupTime = util_cpu_time();

  rplus = Ddi_BddMakeConstAig(ddiMgr, 1);
  if (opt->trav.rPlus != NULL) {
    char *pbench;
    Ddi_Var_t *v = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
    Ddi_Var_t *v1 = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "-- Reading REACHED set from %s.\n", opt->trav.rPlus));
    Ddi_Free(rplus);
    if ((pbench = strstr(opt->trav.rPlus, ".bench")) != NULL) {
      //      *pbench = '\0';
      rplus = Ddi_AigNetLoadBench(ddiMgr, opt->trav.rPlus, NULL);
    } else if ((pbench = strstr(opt->trav.rPlus, ".aig")) != NULL) {
      int useNs = 1;
      rplusRings =
        Ddi_AigarrayNetLoadAiger(ddiMgr, NULL, opt->trav.rPlus);
      Ddi_Bddarray_t *nsLits = Ddi_BddarrayMakeLiteralsAig(ns, 1);
      if (useNs)
	Ddi_BddarraySubstVarsAcc(rplusRings,Fsm_MgrReadVarPS(fsmMgr),
                          Fsm_MgrReadVarNS(fsmMgr));
      if (Ddi_BddarrayNum(rplusRings)==1) {
	rplus = Ddi_BddDup(Ddi_BddarrayRead(rplusRings,0));
	Ddi_Free(rplusRings);
      }
      else {
	Ddi_Bdd_t *initNs = Ddi_BddDup(Fsm_MgrReadInitBDD(fsmMgr));
	if (useNs)
	  Ddi_BddSubstVarsAcc(initNs,Fsm_MgrReadVarPS(fsmMgr),
                          Fsm_MgrReadVarNS(fsmMgr));
	Ddi_BddarrayWrite(rplusRings, 0, initNs);
	rplus = Ddi_BddMakeConstAig(ddiMgr, 1);
	Ddi_Free(initNs);
      }
      Ddi_Free(nsLits);
    } else {
      rplus = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
        DDDMP_MODE_DEFAULT, opt->trav.rPlus, NULL);
      Ddi_BddExistProjectAcc(rplus, psvars);
      Ddi_MgrReduceHeap(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"), 0);
      Ddi_MgrSetSiftThresh(ddiMgr, opt->ddi.siftTh);
    }
#if 0
    if (v != NULL) {
      Ddi_BddCofactorAcc(rplus, v, 1);
    }
    if (v1 != NULL) {
      Ddi_BddCofactorAcc(rplus, v1, 1);
    }
#endif
    Ddi_BddSetAig(rplus);
  }
  //doRunBmc = doRunPdr = 0 && !(opt->pre.specDecompSat || opt->pre.specDecompCore);
  doRunBmc = !(opt->pre.specDecompSat || opt->pre.specDecompCore);
  //  doRunBmcThreaded = 1;
  prevCexes = Ddi_BddarrayAlloc(ddiMgr, 0);
  if (doRunPdr) {
    doRunBmc = 0;
  }
  //  doRunPdr = 1;
  itpTimeLimit = opt->mc.decompTimeLimit;
  pdrTimeLimit = opt->mc.decompTimeLimit;
  bmcTimeLimit = opt->mc.decompTimeLimit;

  doRunBdd = !opt->mc.aig;
  doRunItp = opt->mc.aig && (opt->mc.bmc < 0);
  doRunBmc = opt->mc.bmc >= 0;
  doRunPdr = opt->mc.pdr;
  
  if (doRunPdr) {
    doRunItp = 0;
  }
#if 0
  for (k = Ddi_BddarrayNum(pArray) - 1; k > 0; k--) {
    Ddi_Bdd_t *p_k = Ddi_BddarrayRead(pArray, k);

    Ddi_BddAndAcc(Ddi_BddarrayRead(pArray, 0), p_k);
    Ddi_BddarrayRemove(pArray, k);
  }
#endif

  quitOnFailure = opt->pre.specDecompSat || opt->pre.specDecompCore;

  if (doRunBdd && opt->trav.fbvMeta && opt->trav.metaMethod == Ddi_Meta_Size) {
    Ddi_MetaInit(ddiMgr, opt->trav.metaMethod, NULL, 0, NULL, NULL,
      opt->trav.fbvMeta);
    Ddi_MgrSetMetaMaxSmooth(ddiMgr, opt->common.metaMaxSmooth);
    opt->trav.fbvMeta = 1;
  }

  if (opt->mc.wFsm != NULL) {
    Ddi_Bddarray_t *l = Ddi_BddarrayAlloc(ddiMgr, 1);
    Ddi_Bdd_t *propTot = Ddi_BddDup(Ddi_BddarrayRead(delta, iProp));
    int doDecomp = !opt->pre.specDecompCore;
    Ddi_BddCofactorAcc(propTot, pvarPs, 1);
    Ddi_BddCofactorAcc(propTot, cvarPs, 1);
    if (doDecomp) {
      int i, j;
      for (i=j=0; i<Ddi_BddarrayNum(pArray); i++) {
	Ddi_Bdd_t *p_i = Ddi_BddDup(Ddi_BddarrayRead(pArray,i));
	Ddi_BddSetAig(p_i);
	Ddi_BddCofactorAcc(p_i, pvarPs, 1);
	Ddi_BddCofactorAcc(p_i, cvarPs, 1);
	if (!Ddi_BddIsOne(p_i)) {
	  Ddi_BddarrayWrite(l, j++, p_i);
	}
	Ddi_Free(p_i);
      }
    }
    else {
      Ddi_BddarrayWrite(l, 0, propTot);
    }
    Ddi_Free(propTot);

    fsmFsmStore = Fsm_FsmMakeFromFsmMgr(fsmMgr);

    Fsm_FsmWriteInvarspec(fsmFsmStore, invarspec);
    if (!opt->mc.fold) {
      Fsm_FsmUnfoldProperty(fsmFsmStore, 1);
      Fsm_FsmUnfoldConstraint(fsmFsmStore);
    }
    if (Fsm_MgrReadInitStubBDD(fsmMgr) != NULL) {
      Fsm_FsmFoldInit(fsmFsmStore);
    }

    Fsm_FsmWriteLambda(fsmFsmStore, l);
    Fsm_FsmSetNO(fsmFsmStore, 1);
    Ddi_Free(l);
  }

  for (k = done = 0; !done &&
    (opt->pre.specDecompCore || k < Ddi_BddarrayNum(pArray)); k++) {
    char name[100];
    Ddi_Bdd_t *savedInvarspec = Ddi_BddDup(invarspec);
    Ddi_Bdd_t *d, *myInvar = NULL;
    int iConstr2 = -1, j;
    Ddi_Vararray_t *ps2, *ns2, *pi2;
    Ddi_Var_t *vi, *vj;
    int doCoiRed = 1;
    Ddi_Bdd_t *myWindow = NULL;
    Ddi_Bdd_t *deltaPropPart = NULL, *failureCex = NULL;
    Ddi_Varset_t *psVars = Ddi_VarsetMakeFromArray(ps);
    int currProp = partOrdId ? partOrdId[k] : k;
    int curHit = -1;
    int chkRing = 1;
    int clustSize = 1;
    int provedByCare = 0;
    
    if (0 && (k >= opt->trav.bmcFirst)) {
      doRunBmc = 1;
      bmcSteps = -1;
    }
    doGenNew |= (opt->pre.specDecompCore == 1) && (k > 100);

    fsmMgr2 = Fsm_MgrDup(fsmMgr);
    ps2 = Fsm_MgrReadVarPS(fsmMgr2);
    ns2 = Fsm_MgrReadVarNS(fsmMgr2);
    pi2 = Fsm_MgrReadVarI(fsmMgr2);

    Ddi_Free(pCore);
    int tryPart=0 && !opt->pre.specDecompCore;
    if (tryPart) {
      pCore = Ddi_BddMakePartDisjFromArray(pArray);
      for (int i=0; i<Ddi_BddPartNum(pCore); i++) {
	if (i!=k)
	  Ddi_BddNotAcc(Ddi_BddPartRead(pCore,i));
      }
    }
    else {
      pCore = Ddi_BddDup(Ddi_BddarrayRead(pArray, 0));
    }
    Ddi_BddSetAig(pCore);
    // opt->trav.dynAbstr = 0;
    // opt->trav.itpAppr = 1;

    travMgrAig = Trav_MgrInit(NULL, ddiMgr);
    FbvSetTravMgrOpt(travMgrAig, opt);
    FbvSetTravMgrFsmData(travMgrAig, fsmMgr2);
    delta = Fsm_MgrReadDeltaBDD(fsmMgr2);

    if (k==0 && (opt->trav.abstrRefLoad != NULL)) {
      Ddi_Vararray_t *refinedVars = NULL;
      if (strcmp(opt->trav.abstrRefLoad,"cuts")==0) {
        refinedVars = Ddi_VararrayAlloc(ddiMgr, 0);
        for (int i=0; i<Ddi_VararrayNum(ps); i++) {
          Ddi_Var_t *v = Ddi_VararrayRead(ps,i);
          char *search = "retime_CUT";
          if (strncmp(Ddi_VarName(v),search,strlen(search))==0)
            Ddi_VararrayInsertLast(refinedVars,v);
          else {
            search = "PDT_BDD_INVAR";
            if (strncmp(Ddi_VarName(v),search,strlen(search))==0)
              Ddi_VararrayInsertLast(refinedVars,v);
          }
        }
      }
      else {
        refinedVars = Ddi_VararrayLoad (ddiMgr, opt->trav.abstrRefLoad, NULL);
      }
      if (refinedVars!=NULL) {
        Ddi_Varsetarray_t *abstrRefRefinedVars = Ddi_VarsetarrayAlloc(ddiMgr, 2);
        Ddi_Varset_t *rv = Ddi_VarsetMakeFromArray(refinedVars);
        Ddi_VarsetarrayWrite(abstrRefRefinedVars,0,rv);
        Ddi_VarsetarrayWrite(abstrRefRefinedVars,1,NULL);
        Pdtutil_VerbosityMgrIf(ddiMgr, Pdtutil_VerbLevelUsrMax_c) {
          fprintf(dMgrO(ddiMgr),
                  "Abstr Ref: loaded %d refined vars\n", Ddi_VarsetNum(rv));
        }
        Trav_MgrSetAbstrRefRefinedVars(travMgrAig, abstrRefRefinedVars);
        Ddi_Free(abstrRefRefinedVars);
        Ddi_Free(rv);
        Ddi_Free(refinedVars);
      }
    }
  


    if (rplusRings!=NULL) {
      Trav_MgrSetNewi(travMgrAig, rplusRings);
    }
    int useRplusInTravMgr=1;
    if (useRplusInTravMgr && !Ddi_BddIsOne(rplus)) {
      Ddi_Bdd_t *myRp = Ddi_BddCofactor(rplus, pvarPs, 1);
      Trav_MgrSetReached(travMgrAig, myRp);
      Ddi_Free(myRp);
    }
    
    //    Ddi_BddSetMono(rplus);
    // care = Ddi_BddExistProject(rplus, pVars);
    //  Ddi_BddSetAig(care);
    myInvar = Ddi_BddCofactor(rplus, pvarPs, 1);
    //    myInvar = Ddi_BddDup(rplus);
    Ddi_BddSetAig(myInvar);

    if (0) {
      Ddi_Bdd_t *c0 = Ddi_BddCofactor(myInvar, cvarPs, 0);

      //    care = Ddi_BddDup(myInvar);
      care = Ddi_BddCofactor(myInvar, cvarPs, 1);
      Ddi_BddOrAcc(care, c0);
      Ddi_Free(c0);
      Ddi_Free(constrVars);
      constrVars = Ddi_BddSupp(care);
    } else {
      care = Ddi_BddDup(myInvar);
      Ddi_Free(constrVars);
      constrVars = Ddi_BddSupp(care);
      Ddi_BddCofactorAcc(myInvar, cvarPs, 1);
    }

    if (Trav_MgrReadNewi(travMgrAig) != NULL) {
      Ddi_Varset_t *prevVars = Ddi_BddarraySupp(Trav_MgrReadNewi(travMgrAig));

      Ddi_VarsetSwapVarsAcc(prevVars, ns2, ps2);
      if (constrVars != NULL) {
        Ddi_VarsetUnionAcc(constrVars, prevVars);
        Ddi_Free(prevVars);
      } else {
        constrVars = prevVars;
      }
    }

    if (assumeConstr != NULL) {
      Trav_MgrSetAssume(travMgrAig, assumeConstr);
      Ddi_Free(assumeConstr);
    }

    verificationResult = -1;

    opt->stats.setupTime = util_cpu_time();

    if (opt->pre.specDecompCore) {
      int fp, okCheck;
      Ddi_Bdd_t *target = Ddi_BddNot(savedInvarspec);
      Ddi_Bdd_t *notCare = Ddi_BddNot(care);
      Ddi_Bdd_t *chk, *cex;
      Ddi_Bdd_t *pLit = Ddi_BddMakeLiteralAig(pvarPs, 1);
      int doGen = opt->pre.specDecompCore < 0 ? 0 : opt->pre.specDecompCore;
      int doGenSpec = 0;
      int doGenSpec1 = 0;
      int lookBwd = 1;
      Ddi_Bdd_t *reachedRing = NULL, *eqConstr = NULL;

      if (doGen > 1)
        doGen--;
      opt->trav.pdrReuseRings = 1;

      lemmasSteps = opt->mc.lemmas;

      deltaProp = Ddi_BddDup(pCore);
      Ddi_BddAndAcc(deltaProp, plitPs);
      Ddi_BddOrAcc(deltaProp, clitPs);
      Ddi_BddarrayWrite(delta, iProp, deltaProp);
      Ddi_Free(deltaProp);

      Ddi_BddComposeAcc(target, ps2, delta);
      Ddi_BddComposeAcc(notCare, ps2, delta);

      if (Trav_MgrReadAssume(travMgrAig) != NULL) {
        Ddi_BddAndAcc(target, Trav_MgrReadAssume(travMgrAig));
        Ddi_BddAndAcc(notCare, Trav_MgrReadAssume(travMgrAig));
      }
      //      Pdtutil_Assert(fp,"wrong care set");
      if (invar) {
        Ddi_BddAndAcc(target, invar);
      }
      if (leftover!=NULL) 
        Ddi_BddAndAcc(target, leftover);
      //      chk = Ddi_BddAnd(care,pLit);
      Ddi_Free(pLit);
      if (fromRings != NULL) {
        int jj;
        Ddi_Bdd_t *myTarget = Ddi_BddDup(target);
        Ddi_Bdd_t *myCare = Ddi_BddDup(care);

        Ddi_BddAndAcc(myTarget, savedInvarspec);
        Ddi_BddCofactorAcc(myTarget, pvarPs, 1);
        Ddi_BddCofactorAcc(myCare, pvarPs, 1);
        Ddi_BddSwapVarsAcc(myTarget, ps2, ns2);
        Ddi_BddSubstVarsAcc(myCare, ps2, ns2);
        Ddi_Free(myWindow);
        for (jj = 1; jj < Ddi_BddarrayNum(fromRings); jj++) {
          Ddi_Bdd_t *f_jjTot = Ddi_BddarrayRead(fromRings, jj);
          Ddi_Bdd_t *f_jj = Ddi_BddarrayRead(fromRings, jj);
	  Ddi_Bdd_t *eq_jj = NULL;
          if (Ddi_BddIsPartConj(f_jj)) {
            eq_jj = Ddi_BddPartRead(f_jjTot, 1);
            f_jj = Ddi_BddPartRead(f_jj, 0);
          }
	  chk = Ddi_BddDup(f_jj);
	  if (eq_jj!=NULL) {
	    Ddi_Vararray_t *vars = Ddi_BddReadEqVars(eq_jj);
	    Ddi_Bddarray_t *subst = Ddi_BddReadEqSubst(eq_jj);
            Ddi_Bdd_t *eqConstr = Ddi_BddMakeEq(vars,subst);
	    Ddi_BddSetAig(eqConstr);
	    Ddi_BddAndAcc(chk,eqConstr);
	    Ddi_Free(eqConstr);
	  }
          Ddi_BddCofactorAcc(chk, pvarNs, 1);
          Ddi_BddCofactorAcc(chk, cvarNs, 1);
          Ddi_BddAndAcc(chk, myTarget);
          Ddi_BddAndAcc(chk, myCare);
          myWindow = Ddi_AigSatWithCex(chk);
          if (myWindow != NULL) {
            Ddi_Bdd_t *fPs = Ddi_BddSubstVars(f_jj, ns, ps);
            Ddi_Bdd_t *myConstr = Ddi_BddDiff(plitPs, clitPs);
            Ddi_Bdd_t *target2 = Ddi_BddNot(invarspec);
            Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);

            Ddi_BddComposeAcc(target2, ps, delta);
            if (Ddi_AigSatAnd(fPs, target, myConstr)) {
              printf("target hit confirmed\n");
            } else {
              printf("target hit NOT confirmed\n");
            }
            Ddi_Free(fPs);
            Ddi_Free(myConstr);
            Ddi_Free(target2);
          }
          if (myWindow != NULL) {
            Ddi_Bdd_t *myCube = Ddi_BddDup(f_jj);
            int genCubes = 50;

            if (0 && hintVars!=NULL) {
              lookBwd = 0;
              Ddi_BddExistProjectAcc(myWindow,hintVars);
            }
              
            if (lookBwd && jj >= 1) {
              int fullTarget = 0;
              if (!doRunItp && opt->mc.gfp > 0) {
                if (fromRings!=NULL) {
                  Trav_MgrSetNewi(travMgrAig,fromRings);
                }
                Trav_TravSatItpGfp(travMgrAig,fsmMgr,opt->mc.gfp,
                         opt->trav.countReached);
              }
              Ddi_Bdd_t *inWindow =
                Trav_DeepestRingCex(travMgrAig, fsmMgr2,
                  myTarget, invarspec, fromRings, jj, genCubes,
                                    hintVars,NULL /*&fullTarget*/,
                                    doRunItp,
                                    opt->pre.specSubsetByAntecedents
                                    );
              if (!doRunItp && opt->mc.gfp > 0) {
                Ddi_Free(fromRings);
                fromRings = Ddi_BddarrayDup(Trav_MgrReadNewi(travMgrAig));
              }
              opt->trav.abstrRef = Trav_MgrReadAbstrRef(travMgrAig);
              if (inWindow != NULL) {
                Ddi_Free(myWindow);
                if (Ddi_BddIsZero(inWindow)) {
                  myWindow = Ddi_BddDup(myTarget);
                  fullTarget = 1;
                }
                else {
                  myWindow = Ddi_BddDup(inWindow);
                }
                Ddi_Free(inWindow);
                genCubes = 0;
                if (k>2) {
                  Ddi_Free(myCube);
                  myCube = Ddi_BddDup(myWindow);
                  if (fullTarget) {
                    Trav_MgrSetIgrGrowCone(travMgrAig, 0);
                    Trav_MgrSetIgrSide(travMgrAig, 0);
                    Trav_MgrSetOption(travMgrAig,
                       Pdt_TravIgrGrowConeMax_c, fnum, -1.0);
                  }
                }
              }
	      else genCubes = 1;
            }
            else {
              genCubes = 1;
              if (jj<2) doGenSpec1=1;
            }
            if (doGen && !doGenSpec1 && genCubes) {
              int res;
              Ddi_Varset_t *nv = Ddi_VarsetMakeFromArray(ns2);
              Ddi_Varset_t *nvPi = Ddi_VarsetMakeFromArray(pi2);

              Ddi_Free(myWindow);
              myWindow = Ddi_AigExistProjectByGenClauses(myTarget,
                myCube, myCare, nv, 100 /**doGen*/, 0, &res);
              Ddi_Free(nv);
              Ddi_Free(nvPi);
              Pdtutil_Assert(myWindow != NULL, "null cube");
              Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelDevMax_c) {
                printf("\n", jj);
                logBdd(myWindow);
              }
#if 0
              Ddi_Bdd_t *careBdd = Ddi_BddDup(care);
              Ddi_BddSubstVarsAcc(careBdd, ps, ns);
              Ddi_BddSetMono(careBdd);
              Ddi_BddSetMono(myWindow);
              Ddi_BddConstrainAcc(myWindow,careBdd);
              Ddi_BddSetAig(myWindow);
              Ddi_Free(careBdd);
#endif
            }
            Ddi_Free(myCube);
            reachedRing = f_jj;
            if (Ddi_BddIsPartConj(f_jjTot) && Ddi_BddPartNum(f_jjTot) == 2) {
              eqConstr = Ddi_BddPartRead(f_jjTot, 1);
            }
            Ddi_BddSwapVarsAcc(myWindow, ps2, ns2);
            Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelNone_c) {
              printf("Target hitting ring %d\n", jj);
              curHit = jj;
              if (curHit == prevHit) {
                sameRingHit++;
                if (sameRingHit > 10) {
                  useTot = 1;
                  doGenSpec1=1;
                }
              }
              else
                sameRingHit = 0;
              prevHit = curHit;
            }
            break;
          }
          Ddi_Free(chk);
        }
	if ((myWindow == NULL) && (prevAborted)) {
          myWindow = Ddi_AigSatWithCex(myTarget);
	}
        Ddi_Free(myTarget);
        Ddi_Free(myCare);
      } else {
        Ddi_Bdd_t *space = NULL;
        Ddi_Vararray_t *tVars;
        Ddi_Bdd_t *myTarget = Ddi_BddCofactor(target, cvarPs, 1);

        Ddi_BddCofactorAcc(myTarget, pvarPs, 1);
        Ddi_BddCofactorAcc(myTarget, cvarPs, 1);

        chk = Ddi_BddCofactor(care, pvarPs, 1);
        Ddi_BddAndAcc(notCare, chk);
        Ddi_BddAndAcc(chk, savedInvarspec);
        space = Ddi_BddDup(chk);
        Ddi_BddAndAcc(chk, myTarget);
        Ddi_BddCofactorAcc(chk, pvarPs, 1);
        Ddi_BddCofactorAcc(chk, cvarPs, 1);
        Ddi_Free(myWindow);
        Ddi_Free(myTarget);
        myWindow = Ddi_AigSatWithCex(chk);
        if (k > 0 && !doGenSpec1 && doGen && myWindow != NULL) {
          int res;
          Ddi_Varset_t *pv = Ddi_VarsetMakeFromArray(ps2);

          Ddi_Free(myWindow);
          myWindow = Ddi_AigExistProjectByGenClauses(target, space,
                        NULL, pv, doGen, 0, &res);
          Ddi_Free(pv);
          Pdtutil_Assert(myWindow != NULL, "null cube");
        }
        tVars = Ddi_BddSuppVararray(myWindow);
        fprintf(stdout, "#Window vars: %d - ", Ddi_VararrayNum(tVars));
        Ddi_Free(space);
        Ddi_Free(tVars);
        if (1 && k == 0 && doGen) {
          lemmasSteps = 0;
        }
      }

      if (myWindow == NULL) {
	verificationEnded = 1;
        verificationResult = 0;
        deltaProp = NULL;
      } else if (doGenSpec1) {
        Ddi_Varset_t *proj=Ddi_VarsetVoid(ddiMgr);
        Ddi_Vararray_t *suppA = Ddi_BddSuppVararray(myWindow);
        int i; 
        int max = Ddi_VararrayNum(suppA)*projVarsRate;
        if (max<1) max=1;
        for (i=0; i<max; i++) {
          Ddi_Var_t *v = Ddi_VararrayRead(suppA,i);
          Ddi_VarsetAddAcc(proj,v);
        }
        projVarsRate *=0.5;
        Ddi_Free(suppA);
        Ddi_BddSetMono(myWindow);
        Ddi_BddExistProjectAcc(myWindow,proj);
        Ddi_BddSetAig(myWindow);
        printf("Subspace window of %d vars\n", i);
        //        DdiLogBdd(myWindow,0);
        deltaProp = Ddi_BddNot(pCore);
        Ddi_BddAndAcc(deltaProp, myWindow);
        Ddi_AigStructRedRemAcc(deltaProp, NULL);
        Ddi_BddNotAcc(deltaProp);
        Ddi_Free(myWindow);
      } else if (doGenSpec && fromRings != NULL) {
        Ddi_Bdd_t *myTarget = Ddi_BddDup(target);
        static int state = useTot;
        Ddi_Var_t *v;
        Ddi_Bdd_t *lit;

        Ddi_Free(myWindow);
        switch (state) {
          case 0:
            myWindow =
              Ddi_BddMakeLiteralAig(Ddi_VarFromName(ddiMgr, "l170"), 0);
            lit = Ddi_BddMakeLiteralAig(Ddi_VarFromName(ddiMgr, "l168"), 0);
            Ddi_BddAndAcc(myWindow, lit);
            Ddi_Free(lit);
            lit = Ddi_BddMakeLiteralAig(Ddi_VarFromName(ddiMgr, "l166"), 0);
            Ddi_BddAndAcc(myWindow, lit);
            Ddi_Free(lit);
            lit = Ddi_BddMakeLiteralAig(Ddi_VarFromName(ddiMgr, "l164"), 0);
            Ddi_BddAndAcc(myWindow, lit);
            Ddi_Free(lit);
            lit = Ddi_BddMakeLiteralAig(Ddi_VarFromName(ddiMgr, "l162"), 0);
            Ddi_BddAndAcc(myWindow, lit);
            Ddi_Free(lit);
            lit = Ddi_BddMakeLiteralAig(Ddi_VarFromName(ddiMgr, "l360"), 0);
            Ddi_BddAndAcc(myWindow, lit);
            Ddi_Free(lit);
            lit = Ddi_BddMakeLiteralAig(Ddi_VarFromName(ddiMgr, "l362"), 0);
            Ddi_BddAndAcc(myWindow, lit);
            Ddi_Free(lit);
            deltaProp = Ddi_BddNot(pCore);
            Ddi_AigAndCubeAcc(deltaProp, myWindow);
            Ddi_BddNotAcc(deltaProp);
            Ddi_Free(myWindow);
            state++;
            break;
          default:
            deltaProp = Ddi_BddNot(pCore);
            Ddi_BddAndAcc(deltaProp, reachedRing);
            Ddi_BddSubstVarsAcc(deltaProp, ns, ps);
            Ddi_BddNotAcc(deltaProp);
        }
      } else if (doGenNew && reachedRing != NULL) {
        Ddi_Bdd_t *myTarget = Ddi_BddNot(pCore);
        Ddi_Varset_t *pv = Ddi_VarsetMakeFromArray(ns2);
	int coreStrategy = 4; // nnf
        int thVars = doRunPdr ? 4 : 10; //Ddi_BddSize(myTarget) / 2;
        int thSize = (0 && doRunPdr) ? 1 : 5000;
        //        Ddi_BddAndAcc(myTarget,savedInvarspec);
	if (coreStrategy == 4) {
	  thSize = 150;
	  thVars = 50;
	}
        if (curHit == prevHit) {
          sameRingHit++;
          thVars -= sameRingHit;
          if (thVars < 0)
            thVars = 0;
        } else {
          sameRingHit = 0;
        }
        prevHit = curHit;
        Ddi_BddSwapVarsAcc(myTarget, ps2, ns2);
        Ddi_Free(myWindow);

        deltaProp = Ddi_AigSatCore(myTarget, reachedRing,
          eqConstr, pv, thVars, thSize, coreStrategy, 0);

        Ddi_BddSwapVarsAcc(deltaProp, ps2, ns2);

        Ddi_BddCofactorAcc(deltaProp, pvarPs, 1);
        Ddi_BddCofactorAcc(deltaProp, cvarPs, 1);
        Ddi_BddNotAcc(deltaProp);
        Ddi_Free(pv);
        Ddi_Free(myTarget);
      } else if (doGenNew && fromRings != NULL) {
        Ddi_Bdd_t *myTarget = Ddi_BddDup(target);
        Ddi_Varset_t *pv = Ddi_VarsetMakeFromArray(ps2);

        Ddi_BddAndAcc(myTarget, savedInvarspec);
        Ddi_AigConstrainCubeAcc(myTarget, myWindow);
        Ddi_Free(myWindow);
        myWindow = Ddi_AigSatWithCex(myTarget);
        deltaProp = Ddi_BddNot(pCore);
        //  Ddi_AigAndCubeAcc(deltaProp,myWindow);
        Ddi_AigConstrainCubeAcc(deltaProp, myWindow);
        Ddi_BddExistProjectAcc(myWindow, pv);
        Ddi_BddAndAcc(deltaProp, myWindow);
        Ddi_BddNotAcc(deltaProp);
        Ddi_Free(pv);
        Ddi_Free(myWindow);
        Ddi_Free(myTarget);
      } else if (0) {
        Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelNone_c) {
          printf("using fing full prop\n");
        }
        deltaProp = Ddi_BddNot(pCore);
        Ddi_Free(myWindow);
      } else if (doGen) {
        Ddi_Varset_t *s = Ddi_BddSupp(myWindow);

        Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelNone_c) {
          printf("Target size: %d (supp: %d) (orig: %d)\n",
            Ddi_BddSize(myWindow), Ddi_VarsetNum(s), Ddi_BddSize(target));
        }
        Ddi_Free(s);
        Ddi_Free(deltaProp);
        deltaProp = Ddi_BddNot(myWindow);
        Ddi_Free(myWindow);
      } else if (1) {
        Ddi_Varset_t *wSupp = Ddi_BddSupp(myWindow);
        Ddi_Varset_t *tSupp = Ddi_BddSupp(target);
        Ddi_Vararray_t *vA = Ddi_BddSuppVararray(myWindow);
        Ddi_Varset_t *smooth = Ddi_VarsetDiff(wSupp, tSupp);
        float r;
        int j;

        if (doGenNew)
          normCore = Ddi_VararrayNum(vA) / 2;

        r = (float)((normCore < 2) ? 2 : normCore) / Ddi_VararrayNum(vA);

        if (k == 0) {
          if (Ddi_VararrayNum(vA) < 8)
            r = 1.0;
          else {
            while (r * Ddi_VararrayNum(vA) < 8) {
              r *= 2;
            }
          }
          if (r > 1.0)
            r = 1.0;
        }

        prevNormCore = normCore;
        normCore -= decrCore;
        if (decrCore > 1 && decrCore > normCore / 3)
          decrCore--;

        Ddi_Free(wSupp);
        Ddi_Free(tSupp);
        if (r > 1) {
          r = 0.5;
        }
        for (j = Ddi_VararrayNum(vA) - 1; j >= 0; j--) {
          float r1 = (float)rand() / ((float)RAND_MAX);

          if (r1 < r) {
            Ddi_Var_t *v = Ddi_VararrayRead(vA, j);

            Ddi_VarsetAddAcc(smooth, v);
          }
        }
        Ddi_BddExistAcc(myWindow, smooth);
        Ddi_Free(vA);
        Ddi_Free(smooth);
        deltaProp = Ddi_BddNot(pCore);
        Ddi_AigAndCubeAcc(deltaProp, myWindow);
        //      Ddi_Free(deltaProp); deltaProp = Ddi_BddDup(myWindow);
        Ddi_BddNotAcc(deltaProp);
        //      Ddi_BddAndAcc(Ddi_BddarrayRead(pArray,0),deltaProp);
      } else {
        Ddi_BddExistAcc(myWindow, psVars);
        deltaProp = Ddi_BddNot(pCore);
        Ddi_AigAndCubeAcc(deltaProp, myWindow);
        Ddi_BddNotAcc(deltaProp);
        //      Ddi_BddAndAcc(Ddi_BddarrayRead(pArray,0),deltaProp);
      }

      //      pVars = Ddi_VarsetVoid(ddiMgr);
      pVars = Ddi_VarsetDup(psVars);

      //      printf("\nFP: %d \n", !Ddi_AigSat(notCare));
      Ddi_Free(notCare);
      Ddi_Free(chk);
      Ddi_Free(target);

      printf("\n***** Decomp. core verification: %d[%d] \n", k,
        prevNormCore);
    } else {
      printf("\n***** Decomp. verification: %d[%d]/%d \n", k, currProp,
        Ddi_BddarrayNum(pArray));
      deltaProp = Ddi_BddDup(Ddi_BddarrayRead(pArray, k));
      if (Ddi_BddIsPartConj(deltaProp)) {
        deltaPropPart = Ddi_BddarrayRead(pArray, k);
        Ddi_BddSetAig(deltaProp);
        clustSize = Ddi_BddPartNum(deltaPropPart);
      }
      pVars = Ddi_VarsetDup(Ddi_VarsetarrayRead(coiArray, k));
      if (Ddi_BddIsOne(deltaProp)) {
        verificationResult = 0;
        Ddi_Free(deltaProp);
      }
    }

    if (deltaProp==NULL) {
      deltaProp = Ddi_BddDup(Ddi_BddarrayRead(pArray, 0));
    }
    
    int computeTotProp = 0;
    if (computeTotProp) {
      if (totProp!=NULL) {
        if (care!=NULL) {
          if (Ddi_BddIncluded(care,totProp))
            Ddi_AigOptByMonotoneCoreAcc(totProp,care,NULL,1,-1.0);
        }
        Ddi_BddAndAcc(deltaProp,totProp);
        Ddi_Free(totProp);
      }
      totProp = Ddi_BddDup(deltaProp);
    }

    if (opt->mc.wFsm != NULL) {
      Ddi_Bddarray_t *lambda = Fsm_FsmReadLambda(fsmFsmStore);
      Ddi_Bdd_t *prop = Ddi_BddDup(deltaProp);
      int il = Ddi_BddarrayNum(lambda) - 1;

      char name[1000];
      strcpy(name,opt->mc.wFsm);
      int len = strlen(name);
      char *nump = name+len-6;
      if (*nump=='#') {
        sprintf(nump,"%02d.aig",k); 
      }
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Writing FSM to file %s.\n", name));

      if (prop!=NULL  && opt->pre.specDecompCore) {
	Ddi_BddCofactorAcc(prop, pvarPs, 1);
	Ddi_BddCofactorAcc(prop, cvarPs, 1);

	Pdtutil_Assert(il >= 0, "wrong part prop index");
	Ddi_BddarrayInsert(lambda, il, prop);
      }
      Ddi_Free(prop);
      Fsm_FsmSetNO(fsmFsmStore, Ddi_BddarrayNum(lambda));
      if (care!=NULL && !Ddi_BddIsOne(care)) {
        Ddi_Bdd_t *constr = Ddi_BddDup(care);
        Ddi_BddCofactorAcc(constr, pvarPs, 1);
	Ddi_BddCofactorAcc(constr, cvarPs, 1);
        Ddi_Bdd_t *init = Fsm_FsmReadInit(fsmFsmStore);
        if (!Ddi_BddIncluded(init,constr)) {
          Ddi_BddOrAcc(constr, init);
        }
        if (Fsm_FsmReadConstraint(fsmFsmStore) != NULL) {
          Ddi_Bdd_t *c0 = Fsm_FsmReadConstraint(fsmFsmStore);
          Ddi_BddAndAcc(constr,c0);
        }
        Fsm_FsmWriteConstraint(fsmFsmStore,constr);
        Ddi_Free(constr);
      }
      Fsm_FsmMiniWriteAiger(fsmFsmStore, name);

      if (0 && !opt->pre.specDecompCore) exit(1);
    }

    if (verificationResult < 0) {

      /* replace full property with a partition */
      delta = Fsm_MgrReadDeltaBDD(fsmMgr2);
      printf("Prop size: %d\n", Ddi_BddSize(deltaProp));

      if (opt->trav.countReached) {
        Ddi_Bdd_t *myTarget = Ddi_BddNot(deltaProp);

        Ddi_BddCofactorAcc(myTarget, pvarPs, 1);
        Ddi_BddCofactorAcc(myTarget, cvarPs, 1);
        if (opt->trav.countReached > 1) {
          Ddi_Bdd_t *tBdd = Ddi_BddMakeMono(myTarget);

          Ddi_BddExistProjectAcc(tBdd, psVars);
          fprintf(stdout, "#PARTIAL TargetStates(exact)=%g (#ref vars: %d)\n",
            Ddi_BddCountMinterm(tBdd, targetNvars), targetNvars);
          Ddi_Free(tBdd);
        }
        fprintf(stdout,
          "#PARTIAL TargetStates(estimated)=%g (#ref vars: %d)\n",
          Ddi_AigEstimateMintermCount(myTarget, targetNvars), targetNvars);
        Ddi_Free(myTarget);
      }

      Ddi_BddAndAcc(deltaProp, plitPs);
      Ddi_BddOrAcc(deltaProp, clitPs);
      Ddi_BddarrayWrite(delta, iProp, deltaProp);
      //      Ddi_Free(deltaProp);

      Ddi_VarsetAddAcc(pVars, pvarPs);
      Ddi_VarsetAddAcc(pVars, cvarPs);
      if (Ddi_VarsetIsArray(pVars)) {
        Ddi_VarsetSetArray(constrVars);
      }
      coivars = Ddi_VarsetUnion(pVars, constrVars);
      //      coivars = Ddi_VarsetDup(constrVars);
      //    coivars = Ddi_VarsetVoid(ddiMgr);

      if (doCoiRed) {
        if (1) {
          Ddi_Varsetarray_t *coi =
            computeFsmCoiVars(fsmMgr2, savedInvarspec, coivars, 0, 0);
          int nCoiShell = Ddi_VarsetarrayNum(coi);

          if (nCoiShell) {
            Ddi_Varset_t *coiTot = Ddi_VarsetarrayRead(coi, nCoiShell - 1);

            if (Ddi_VarsetIsArray(coiTot)) {
              Ddi_VarsetSetArray(coivars);
            }
            Ddi_VarsetUnionAcc(coivars, coiTot);
          }
          Ddi_Free(coi);
        }
        fsmCoiReduction(fsmMgr2, coivars);

      }
      printf("(%d vars in COI)\n", Ddi_VarsetNum(coivars));

      Ddi_Free(coivars);

      init = Ddi_BddDup(Fsm_MgrReadInitBDD(fsmMgr2));
      if (Fsm_MgrReadConstraintBDD(fsmMgr2) != NULL) {
        invar = Ddi_BddDup(Fsm_MgrReadConstraintBDD(fsmMgr2));
        Ddi_BddAndAcc(myInvar, invar);
      }

      for (j = Ddi_VararrayNum(ps2) - 1; j >= 0; j--) {
        Ddi_Var_t *v_j = Ddi_VararrayRead(ps2, j);

        if (v_j == cvarPs) {
          iConstr2 = j; break;
        }
      }
      Pdtutil_Assert(iConstr2 >= 0, "missing constr in delta");
      deltaConstr = Ddi_BddarrayRead(delta, iConstr2);
      if (useRplusAsConstr) {
        Ddi_BddAndAcc(deltaConstr, myInvar);
        Fsm_MgrSetConstraintBDD(fsmMgr2,myInvar);
      }
      //bmcTimeLimit = 0;

      int usePrevRings = 1;
      if (!usePrevRings) {
        Ddi_Free(care);
      }

      if (!opt->pre.specDecompCore && care != NULL && !Ddi_BddIsOne(care)) {
        int fp, okCheck;
        Ddi_Bdd_t *notReached = Ddi_BddDup(care);
        Ddi_Bdd_t *target = Ddi_BddNot(savedInvarspec);

        //      Ddi_BddAndAcc(notReached,plitPs);
        Pdtutil_Assert(verificationResult == -1, "wrong ver result init val");
        Ddi_BddNotAcc(notReached);
        Ddi_BddComposeAcc(notReached, ps2, delta);
        Ddi_BddComposeAcc(target, ps2, delta);
        if (Trav_MgrReadAssume(travMgrAig) != NULL) {
          Ddi_BddAndAcc(notReached, Trav_MgrReadAssume(travMgrAig));
          Ddi_BddAndAcc(target, Trav_MgrReadAssume(travMgrAig));
        }
        //      fp = !Ddi_AigSatAnd(notReached,care,invar);
        //      Pdtutil_Assert(fp,"wrong care set");
        if (invar) {
          Ddi_BddAndAcc(target, invar);
        }
        if (!Ddi_AigSatAnd(target, care, savedInvarspec)) {
          verificationResult = 0;
          provedByCare = 1;
        }
        Ddi_Free(notReached);
        Ddi_Free(target);
      }
    }

    Ddi_Free(pVars);
    Ddi_Free(psVars);

    if (verificationResult >= 0) {
      if (opt->pre.specDecompCore)
        done = 1;
      //      done = 1;
    } else if (0 && Ddi_BddarraySize(delta) > 1000) {
      verificationResult = -1;
    }
#if 0
    /* BMC */
    if (doRunBmcThreaded && verificationResult < 0) {
      Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "\nRunning BMC THREAD...\n"));
      verificationResult = FbvStartBmcThread(fsmMgr2, bmcPrevCexes);
      if (verificationResult > 0) {
        Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
          printf("Invariant verification UNDECIDED after BMC ...\n"));
        verificationResult = -1;
      } else {
        Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
          printf("Invariant verification UNDECIDED after BMC ...\n"));
        verificationResult = -1;
      }
    }
#endif

    bmcStepTime = util_cpu_time();
    if ((doRunBmc || doRunBmcThreaded) && verificationResult < 0) {
      bmcSteps = opt->mc.bmc;
      bmcTimeLimit += 100;
      if (Ddi_BddarrayNum(prevCexes) > 0) {
        Ddi_Bdd_t *cex;

        Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "\nRunning BMC on prev cexes...\n"));
        Trav_TravSatBmcIncrVerif(travMgrAig, fsmMgr2,
          init, savedInvarspec, invar, prevCexes, bmcSteps, 1 /*bmcStep */ ,
          opt->trav.bmcLearnStep,
          opt->trav.bmcFirst, 1 /*doCex */ , 0, 0 /*interpolantBmcSteps */ ,
          opt->trav.bmcStrategy, opt->trav.bmcTe, bmcTimeLimit);
        cex = Fsm_MgrReadCexBDD(fsmMgr);
        if (cex != NULL) {
          verificationResult = 1;   //Trav_MgrReadAssertFlag(travMgrAig);
        }
      }
      if (verificationResult < 0) {
        Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "\nRunning BMC...\n"));
        if (opt->trav.bmcStrategy >= 1) {
          Ddi_Bdd_t *cex;

          Trav_TravSatBmcIncrVerif(travMgrAig, fsmMgr2,
            init, savedInvarspec, invar, NULL, bmcSteps, 1 /*bmcStep */ ,
            opt->trav.bmcLearnStep,
            opt->trav.bmcFirst, 1 /*doCex */ , 0, 0 /*interpolantBmcSteps */ ,
            opt->trav.bmcStrategy, opt->trav.bmcTe, bmcTimeLimit);
          cex = Fsm_MgrReadCexBDD(fsmMgr2);
          if (cex != NULL) {
            int ii;

            for (ii = 0; ii < Ddi_BddarrayNum(prevCexes); ii++) {
              Ddi_Bdd_t *cex_ii = Ddi_BddarrayRead(prevCexes, ii);

              if (Ddi_BddPartNum(cex_ii) > Ddi_BddPartNum(cex)) {
                break;
              }
            }
            Ddi_BddarrayInsert(prevCexes, ii, cex);
            verificationResult = 1; //Trav_MgrReadAssertFlag(travMgrAig);
          }
        } else {
          Trav_TravSatBmcVerif(travMgrAig, fsmMgr2,
            init, savedInvarspec, invar, bmcSteps, 1 /*bmcStep */ ,
            opt->trav.bmcFirst,
            0 /*apprAig */ , 0, 0 /*interpolantBmcSteps */ , bmcTimeLimit);
        }
        verificationResult = Trav_MgrReadAssertFlag(travMgrAig);
      }
      if (verificationResult > 0) {
        /* property failed */
        if (!multiProp) {
          verificationResult = 1;
          Fsm_MgrQuit(fsmMgr2);
          goto quitMem;
        }
      } else {
        Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
          printf("Invariant verification UNDECIDED after BMC ...\n"));
        verificationResult = -1;
      }
    }

    bmcStepTime = util_cpu_time() - bmcStepTime;
    umcStepTime = util_cpu_time();

    /* PDR */
    if (doRunPdr && verificationResult < 0) {
      Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "\nRunning PDR...\n"));

      if (pdrClauses != NULL) {
        travMgrAig->settings.ints.timeFrameClauses = pdrClauses;
        travMgrAig->settings.ints.nPdrFrames = nPdrFrames;
        pdrClauses = NULL;
        nPdrFrames = 0;
        Ddi_Free(fromRings);
      }

      Trav_TravPdrVerif(travMgrAig, fsmMgr2,
        init, savedInvarspec, invar, care, -1, pdrTimeLimit);
      failureCex = Fsm_MgrReadCexBDD(fsmMgr2);

      verificationResult = Trav_MgrReadAssertFlag(travMgrAig);

      if (travMgrAig->settings.ints.nPdrFrames > 0) {
        pdrClauses = travMgrAig->settings.ints.timeFrameClauses;
        nPdrFrames = travMgrAig->settings.ints.nPdrFrames;
        travMgrAig->settings.ints.timeFrameClauses = NULL;
        travMgrAig->settings.ints.nPdrFrames = 0;

        if (Trav_MgrReadNewi(travMgrAig) != NULL) {
          fromRings = Ddi_BddarrayDup(Trav_MgrReadNewi(travMgrAig));
        }

      }

      if (verificationResult < 0) {
        Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
          printf("Invariant verification UNDECIDED after PDR ...\n"));
      }
    }

    /* ITP */
    if (doRunItp && verificationResult < 0) {
      int jj;
      Ddi_Bddarray_t *lambda2 = Fsm_MgrReadLambdaBDD(fsmMgr2);

      if (shareLemmas) {
        for (jj = 0; jj < Ddi_BddarrayNum(pArray); jj++) {
          Ddi_Bdd_t *p_jj = Ddi_BddarrayRead(pArray, jj);
	  Ddi_BddSetAig(p_jj);
          Ddi_BddarrayWrite(lambda2, jj + 1, p_jj);
        }
      }

      if (!useRplusAsCareWithItp)
        Ddi_Free(care);

      if (fromRings != NULL) {
        //        Ddi_Free(care);
        for (jj = 1; jj < Ddi_BddarrayNum(fromRings); jj++) {
          Ddi_Bdd_t *f_jj = Ddi_BddarrayRead(fromRings, jj);

          if (0 && Ddi_BddIsPartConj(f_jj)) {
            Pdtutil_Assert(Ddi_BddPartNum(f_jj) == 2, "wrong part format");
            Ddi_BddPartRemove(f_jj, 1);
            Ddi_BddSetAig(f_jj);
          }
          Ddi_BddCofactorAcc(f_jj, pvarNs, 1);
        }
        Trav_MgrSetNewi(travMgrAig, fromRings);
        Ddi_Free(fromRings);
	travMgrAig->settings.ints.igrFpRing = igrFpRing;
      }

      if (abstrRefRefinedVars != NULL) {
        //      Ddi_Free(care);
        Trav_MgrSetAbstrRefRefinedVars(travMgrAig, abstrRefRefinedVars);
        Ddi_Free(abstrRefRefinedVars);
      }

      Trav_MgrSetOption(travMgrAig, Pdt_TravItpTimeLimit_c, inum,
        itpTimeLimit);
      Trav_TravSatItpVerif(travMgrAig, fsmMgr2,
	init, savedInvarspec, NULL, invar, care, /*k<7?0: */ lemmasSteps,
        itpBoundLimit, opt->common.lazyRate, curHit, itpTimeLimit);

      failureCex = Fsm_MgrReadCexBDD(fsmMgr2);
      verificationResult = Trav_MgrReadAssertFlag(travMgrAig);

      if (Trav_MgrReadAbstrRefRefinedVars(travMgrAig) != NULL) {
        abstrRefRefinedVars =
          Ddi_VarsetarrayDup(Trav_MgrReadAbstrRefRefinedVars(travMgrAig));
      }

      if (Trav_MgrReadNewi(travMgrAig) != NULL) {
        fromRings = Ddi_BddarrayDup(Trav_MgrReadNewi(travMgrAig));
      }

      if (opt->pre.specDecompConstrTh > 0 && shareLemmas) {
        lambda2 = Fsm_MgrReadLambdaBDD(fsmMgr2);
        for (jj = 0; jj < Ddi_BddarrayNum(pArray); jj++) {
          Ddi_Bdd_t *p_jj = Ddi_BddarrayRead(lambda2, jj + 1);

          Ddi_BddarrayWrite(pArray, jj, p_jj);
        }
        if (deltaPropPart != NULL) {
          deltaPropPart = Ddi_BddarrayRead(pArray, k);
        }
      }
    }

    /* BDD */
    if (doRunBdd && verificationResult < 0) {
      //int verificationOK;
      //int bddTimeLimit=-1, bddMemoryLimit=-1;

      Ddi_MgrSetTimeInitial(ddiMgr);
      Ddi_MgrSetTimeLimit(ddiMgr, bddTimeLimit);
      Ddi_MgrSetMemoryLimit(ddiMgr, bddMemoryLimit);
      Ddi_MgrSetSiftMaxThresh(ddiMgr, opt->ddi.siftMaxTh);
      Ddi_MgrSetSiftMaxCalls(ddiMgr, opt->ddi.siftMaxCalls);

      Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "\nRunning BDDs...\n"));

      CATCH {
        Ddi_Bdd_t *careBdd = NULL;
        Ddi_Vararray_t *ps, *ns, *pi;

	ns = Fsm_MgrReadVarNS(fsmMgr2);
	ps = Fsm_MgrReadVarPS(fsmMgr2);
	pi = Fsm_MgrReadVarI(fsmMgr2);


        delta = Fsm_MgrReadDeltaBDD(fsmMgr2);
        lambda = Fsm_MgrReadLambdaBDD(fsmMgr2);
        if (Fsm_MgrReadConstraintBDD(fsmMgr2) != NULL) {
          Ddi_Free(invar);
          invar = Ddi_BddMakeMono(Fsm_MgrReadConstraintBDD(fsmMgr2));
        }
        invarspecBdd = Ddi_BddMakeMono(savedInvarspec);
        if (care != NULL) {
          careBdd = Ddi_BddMakeMono(care);
          Ddi_BddExistProjectAcc(careBdd, psvars);
        }

        Fsm_MgrSetInvarspecBDD(fsmMgr2, invarspecBdd);
        Fsm_MgrSetCutThresh(fsmMgr2, opt->fsm.cut);
        Fsm_MgrSetOption(fsmMgr2, Pdt_FsmCutAtAuxVar_c, inum,
                         opt->fsm.cutAtAuxVar);
        Fsm_MgrAigToBdd(fsmMgr2);

        Ddi_BddSetMono(init);

        trMgr = Tr_MgrInit("TR_manager", ddiMgr);

        if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c
          && ((Pdtutil_VerbLevel_e) opt->verbosity) <=
          Pdtutil_VerbLevelSame_c) {
          Tr_MgrSetVerbosity(trMgr, Pdtutil_VerbLevel_e(opt->verbosity));
        }

        Tr_MgrSetSortMethod(trMgr, Tr_SortWeight_c);
        Tr_MgrSetClustSmoothPi(trMgr, 0);
        Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
        Tr_MgrSetImgClustThreshold(trMgr, opt->trav.imgClustTh);
        Tr_MgrSetImgApproxMinSize(trMgr, opt->tr.approxImgMin);
        Tr_MgrSetImgApproxMaxSize(trMgr, opt->tr.approxImgMax);
        Tr_MgrSetImgApproxMaxIter(trMgr, opt->tr.approxImgIter);

        Tr_MgrSetI(trMgr, pi2);
        Tr_MgrSetPS(trMgr, ps2);
        Tr_MgrSetNS(trMgr, ns2);

        if (0 && Ddi_VararrayNum(pi) < 22) {
        }

        if (Fsm_MgrReadVarAuxVar(fsmMgr2) != NULL) {
          Ddi_Vararray_t *auxvArray = Fsm_MgrReadVarAuxVar(fsmMgr2);

          opt->fsm.auxVarsUsed = 1;
          Ddi_VararrayAppend(pi, auxvArray);
          Tr_MgrSetAuxVars(trMgr, Fsm_MgrReadVarAuxVar(fsmMgr2));
          Tr_MgrSetAuxFuns(trMgr, Fsm_MgrReadAuxVarBDD(fsmMgr2));
        }

        if (Fsm_MgrReadTrBDD(fsmMgr2) == NULL) {
          tr = Tr_TrMakePartConjFromFunsWithAuxVars(trMgr, delta, ns,
            Fsm_MgrReadAuxVarBDD(fsmMgr2), Fsm_MgrReadVarAuxVar(fsmMgr2));
        } else {
          tr = Tr_TrMakeFromRel(trMgr, Fsm_MgrReadTrBDD(fsmMgr2));
        }
        Tr_MgrSetCoiLimit(trMgr, opt->tr.coiLimit);
        Tr_TrCoiReduction(tr, invarspecBdd);

        FbvFsmCheckComposePi(fsmMgr2, tr);

        if (TRAV_BDD_VERIF) {
          travMgrBdd = Trav_MgrInit(opt->expName, ddiMgr);
          FbvSetTravMgrOpt(travMgrBdd, opt);
          //FbvSetTravMgrFsmData(travMgrBdd, fsmMgr);
          //Trav_MgrSetI(travMgrBdd, Tr_MgrReadI(trMgr));
          //Trav_MgrSetPS(travMgrBdd, Tr_MgrReadPS(trMgr));
          //Trav_MgrSetNS(travMgrBdd, Tr_MgrReadNS(trMgr));
          //Trav_MgrSetFrom(travMgrBdd, from);
          //Trav_MgrSetReached(travMgrBdd, from);
          //Trav_MgrSetTr(travMgrBdd, tr);
        }

        if (opt->mc.method == Mc_VerifExactBck_c) {
          opt->trav.rPlus = "1";
          if (travMgrBdd != NULL) {
            Trav_MgrSetOption(travMgrBdd, Pdt_TravRPlus_c, pchar,
              opt->trav.rPlus);
            verificationResult =
              Trav_TravBddApproxForwExactBckFixPointVerify(travMgrBdd, fsmMgr,
              tr, init, invarspecBdd, invar, careBdd, delta);
          } else {
            verificationResult =
              FbvApproxForwExactBckFixPointVerify(fsmMgr, tr, init,
              invarspecBdd, invar, careBdd, delta, opt);
          }
        } else if (opt->mc.method == Mc_VerifApproxFwdExactBck_c) {
          if (travMgrBdd != NULL) {
            verificationResult =
              Trav_TravBddApproxForwExactBckFixPointVerify(travMgrBdd, fsmMgr,
              tr, init, invarspecBdd, invar, careBdd, delta);
          } else {
            verificationResult = FbvApproxForwExactBckFixPointVerify(fsmMgr,
              tr, init, invarspecBdd, invar, careBdd, delta, opt);
          }
        } else {
          if (opt->mc.method == Mc_VerifTravFwd_c) {
            Ddi_Free(invarspecBdd);
            invarspecBdd = Ddi_BddMakeConst(ddiMgr, 1);
          }
          if (travMgrBdd != NULL) {
            travMgrBdd->settings.bdd.bddTimeLimit = bddTimeLimit;
            travMgrBdd->settings.bdd.bddNodeLimit = bddNodeLimit;
            verificationResult =
              Trav_TravBddExactForwVerify(travMgrBdd, fsmMgr, tr, init,
              invarspecBdd, invar, delta);
          } else {
            verificationResult =
              FbvExactForwVerify(travMgrAig, fsmMgr, tr, init, invarspecBdd,
              invar, delta, bddTimeLimit, bddNodeLimit, opt);
          }
        }

        if (travMgrBdd != NULL) {
          /* print stats */
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
            Pdtutil_VerbLevelNone_c, currTime = util_cpu_time();
            fprintf(stdout, "Live nodes (peak): %u(%u)\n",
              Ddi_MgrReadLiveNodes(ddiMgr),
              Ddi_MgrReadPeakLiveNodeCount(ddiMgr));
#if 0                           // NXR: fix time computations
            fprintf(stdout, "TotalTime: %s ",
              util_print_time(setupTime + travTime));
            fprintf(stdout, "(setup: %s - ", util_print_time(setupTime));
            fprintf(stdout, " trav: %s)\n", util_print_time(travTime));
#endif
            );
          Trav_MgrQuit(travMgrBdd);
        }

        if (verificationResult >= 0) {
          verificationResult = !verificationResult;
        }
        Ddi_Free(invarspecBdd);
        Ddi_Free(careBdd);
        Tr_MgrQuit(trMgr);
      }
      FAIL {
        Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
          printf("Invariant verification UNDECIDED after BDDs ...\n"));
      }
    }
    umcStepTime = util_cpu_time() - umcStepTime;

    bmcSerialTime += bmcStepTime;
    umcSerialTime += umcStepTime;
    printf("*** Step   bmc/umc time: %.1f/%.1f\n", bmcStepTime / 1000.0,
      umcStepTime / 1000.0);
    printf("*** Serial bmc/umc time: %.1f/%.1f\n", bmcSerialTime / 1000.0,
      umcSerialTime / 1000.0);
    if (verificationResult > 0) {
      /* fail: add bmc time to UMC as well */
      bmcParallTime += bmcStepTime;
      umcParallTime += bmcStepTime;
    }
    if (verificationResult == 0) {
      /* pass: add umc time to BMC as well */
      bmcParallTime += umcStepTime;
      umcParallTime += umcStepTime;
    }
    if (verificationResult < 0) {
      /* unknown: add time to both BMC and UMC */
      bmcParallTime += bmcStepTime + umcStepTime;
      umcParallTime += bmcStepTime + umcStepTime;
    }
    printf("*** Parallel bmc/umc time: %.1f/%.1f\n", bmcParallTime / 1000.0,
      umcParallTime / 1000.0);

    Ddi_Free(savedInvarspec);
    Ddi_Free(myWindow);
    Ddi_Free(myInvar);
    Ddi_Free(init);
    Ddi_Free(care);
    Ddi_Free(invar);

    prevAborted = verificationResult < 0;
    igrFpRing = travMgrAig->settings.ints.igrFpRing;

    if (verificationEnded) {
      printf("Partitioned prop verification ended!\n");
    }
    else if (verificationResult > 0) {
      /* property failed */
      printf("Property FAILED!\n");
      if (quitOnFailure) {
        Ddi_Free(fromRings);
        Ddi_Free(deltaProp);
        Ddi_Free(abstrRefRefinedVars);
        Fsm_MgrQuit(fsmMgr2);
        travMgrAig = Trav_MgrQuit(travMgrAig);
        done = 0;
        break;
      }
    } else if (verificationResult == 0) {
      /* property proved */
      printf("Property PASSED\n");
      totProved += clustSize;
      Ddi_Bdd_t *r = Trav_MgrReadReached(travMgrAig);

      if (Trav_MgrReadAssume(travMgrAig) != NULL) {
        assumeConstr = Ddi_BddDup(Trav_MgrReadAssume(travMgrAig));
      }
      if (opt->trav.itpConstrLevel >= 0 && r != NULL && !Ddi_BddIsOne(r)) {
        Ddi_BddSetAig(r);
        care = Ddi_BddDup(r);
      }
      if (care == NULL) {
        /* keep rplus */
      } else if (opt->pre.specDecompCore ||
        Ddi_BddSize(care) < opt->pre.specDecompConstrTh) {
        int tryScorr = 0;

        if (tryScorr) {
          // Ddi_BddSetMono(care);
          Ddi_Bddarray_t *lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
          int nlambda = Ddi_BddarrayNum(lambda);
          Ddi_Bdd_t *dummyProp = Ddi_BddMakeConstAig(ddiMgr, 1);
          Ddi_Bdd_t *myCare = Ddi_BddCofactor(care, pvarPs, 1);
          Ddi_Bdd_t *myL = Ddi_BddDup(Ddi_BddarrayRead(lambda, 0));
          Ddi_Bdd_t *myOne = Ddi_BddMakeConstAig(ddiMgr, 1);
          Ddi_Bdd_t *lCorr = NULL;  //Ddi_BddMakeConstAig(ddiMgr, 1);

          ns = Fsm_MgrReadVarNS(fsmMgr);
          ps = Fsm_MgrReadVarPS(fsmMgr);
          Ddi_BddarrayWrite(lambda, 0, myOne);
          Ddi_BddCofactorAcc(myCare, cvarPs, 1);
          if (savedInvarspec != NULL && !Ddi_BddIsOne(savedInvarspec)) {
            Ddi_BddarrayInsertLast(lambda, savedInvarspec);
          }
          for (i = 0; i < Ddi_BddarrayNum(fromRings); i++) {
            Ddi_Bdd_t *l_i = Ddi_BddarrayRead(fromRings, i);

            if (Ddi_BddIsPartConj(l_i)) {
              Ddi_BddCofactorAcc(Ddi_BddPartRead(l_i, 0), pvarNs, 1);
              Ddi_BddCofactorAcc(Ddi_BddPartRead(l_i, 0), cvarNs, 1);
              Ddi_BddarrayInsertLast(lambda, Ddi_BddPartRead(l_i, 0));
              // equiv info omitted
              // Ddi_BddarrayInsertLast(lambda,Ddi_BddPartRead(l_i,1));
            } else {
              Ddi_BddCofactorAcc(l_i, pvarNs, 1);
              Ddi_BddCofactorAcc(l_i, cvarNs, 1);
              Ddi_BddarrayInsertLast(lambda, l_i);
            }
          }
          Ddi_BddarraySubstVarsAcc(lambda, ns, ps);

          Fsm_MgrSetInvarspecBDD(fsmMgr, dummyProp);

          printf("INIT SIZE: %d\n", Ddi_BddSize(Fsm_MgrReadInitBDD(fsmMgr)));

          Ddi_Bddarray_t *dd = Fsm_MgrReadDeltaBDD(fsmMgr);
          int nn = Ddi_BddarrayNum(dd);

          printf("DPROP: %d\n", Ddi_BddSize(Ddi_BddarrayRead(dd, nn - 1)));
          printf("DD: %d\n", Ddi_BddSize(Ddi_BddarrayRead(pArray, 0)));

          Fsm_MgrAbcScorr(fsmMgr, pArray, myCare, lCorr, 1);

          dd = Fsm_MgrReadDeltaBDD(fsmMgr);
          nn = Ddi_BddarrayNum(dd);
          printf("DPROP2: %d\n", Ddi_BddSize(Ddi_BddarrayRead(dd, nn - 1)));
          printf("DD2: %d\n", Ddi_BddSize(Ddi_BddarrayRead(pArray, 0)));

          printf("INIT SIZE2: %d\n", Ddi_BddSize(Fsm_MgrReadInitBDD(fsmMgr)));

          Ddi_BddarrayWrite(lambda, 0, myL);
          Ddi_Free(myCare);
          Ddi_Free(myL);
          Ddi_Free(myOne);
          Ddi_Free(dummyProp);
          Fsm_MgrSetInvarspecBDD(fsmMgr, NULL);

          lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
          ns = Fsm_MgrReadVarNS(fsmMgr);
          ps = Fsm_MgrReadVarPS(fsmMgr);

          for (i = Ddi_BddarrayNum(fromRings) - 1; i >= 0; i--) {
            Ddi_Bdd_t *l_i = Ddi_BddarrayRead(fromRings, i);
            Ddi_Bdd_t *pvarLit = Ddi_BddMakeLiteralAig(pvarPs, 1);
            Ddi_Bdd_t *cvarLit = Ddi_BddMakeLiteralAig(cvarPs, 1);
            int n = Ddi_BddarrayNum(lambda);

            if (Ddi_BddIsPartConj(l_i)) {
              Ddi_Bdd_t *l_i_0 = Ddi_BddarrayExtract(lambda, --n);

              Ddi_BddAndAcc(l_i_0, pvarLit);
              //        Ddi_BddAndAcc(l_i_0,cvarLit);
              Ddi_BddSubstVarsAcc(l_i_0, ps, ns);
              Ddi_BddPartWrite(l_i, 0, l_i_0);
              Ddi_Free(l_i_0);

              // Ddi_Bdd_t *l_i_1 = Ddi_BddarrayExtract(lambda,--n);
              // Ddi_BddAndAcc(l_i_1,pvarLit);
              // Ddi_BddPartWrite(l_i,1,l_i_1);
              // Ddi_Free(l_i_1);

            } else {
              Ddi_Bdd_t *l_i_1 = Ddi_BddarrayExtract(lambda, --n);

              Ddi_BddAndAcc(l_i_1, pvarLit);
              //        Ddi_BddAndAcc(l_i_1,cvarLit);
              Ddi_DataCopy(l_i, l_i_1);
              Ddi_Free(l_i_1);
              Ddi_BddSubstVarsAcc(l_i, ps, ns);
            }

            Ddi_Free(pvarLit);
            Ddi_Free(cvarLit);
          }

          if (savedInvarspec != NULL && !Ddi_BddIsOne(savedInvarspec)) {
            int n = Ddi_BddarrayNum(lambda);
            Ddi_Bdd_t *is = Ddi_BddarrayExtract(lambda, --n);

            Ddi_DataCopy(savedInvarspec, is);
            Ddi_Free(is);
          }

          ps = Fsm_MgrReadVarPS(fsmMgr);
          ns = Fsm_MgrReadVarNS(fsmMgr);
          pi = Fsm_MgrReadVarI(fsmMgr);

          iProp = iConstr = -1;
          nstate = Ddi_VararrayNum(ps);
          for (i = 0; i < nstate; i++) {
            Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);

            if (v_i == pvarPs) {
              iProp = i;
            }
            if (v_i == cvarPs) {
              iConstr = i;
            }
          }
        }
        if (replaceReached) {
          Ddi_Free(rplus);
          rplus = Ddi_BddDup(care);
        }
        else {
          Ddi_BddAndAcc(rplus, care);
        }
        if (!provedByCare && (opt->trav.wR != NULL)) {
          if (strstr(opt->trav.wR, ".aig") != NULL) {
            char name[1000];
            int k1;
            Ddi_Bddarray_t *rings = Ddi_BddarrayDup(fromRings);
            int nRings = travMgrAig->settings.ints.igrRingLast+1;
	    if (nRings<=0 &&
		travMgrAig->settings.ints.fixPointTimeFrame > 0)
	      nRings = travMgrAig->settings.ints.fixPointTimeFrame+1;
            strcpy(name,opt->trav.wR);
            int len = strlen(name);
            char *nump = name+len-6;
            if (*nump=='#') {
              sprintf(nump,"%02d.aig",k); 
            }

	    if (opt->verbosity >= Pdtutil_VerbLevelUsrMax_c) {
	      printf("Writing reached to %s\n", name);
	    }
	    Ddi_Bdd_t *rUnfolded = Ddi_BddDup(care);
	    Ddi_Var_t *pVar = Fsm_MgrReadPdtSpecVar(fsmMgr);
	    Ddi_Var_t *cVar = Fsm_MgrReadPdtConstrVar(fsmMgr);
	    if (pVar!=NULL) Ddi_BddCofactorAcc(rUnfolded,pVar,1);
	    if (cVar!=NULL) Ddi_BddCofactorAcc(rUnfolded,cVar,1);
	    Ddi_AigNetStoreAiger(rUnfolded,0,name);
	    Ddi_Free(rUnfolded);

	    char *s = strstr(name,".aig");
	    if (s==NULL) {
	      strcat(name,"_rings");
	    }
	    else {
	      sprintf(s,"%s", "_rings.aig");
	    }
            while (Ddi_BddarrayNum(rings)>nRings) {
              Ddi_BddarrayRemove(rings,Ddi_BddarrayNum(rings)-1);
            }
            Ddi_BddarrayWrite(rings, 0,
                              Fsm_MgrReadInitBDD(fsmMgr));
            for (k1 = 0; k1 < Ddi_BddarrayNum(rings); k1++) {
              Ddi_Bdd_t *rA = Ddi_BddarrayRead(rings, k1);
              Ddi_BddSetAig(rA);
            }
            Ddi_BddarraySubstVarsAcc(rings,ns,ps);
            Ddi_BddarrayCofactorAcc(rings, cvarPs, 1);
            Ddi_BddarrayCofactorAcc(rings, pvarPs, 1);
            Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
                                   Pdtutil_VerbLevelNone_c,
                                   fprintf(stdout, "Writing reached rings to %s (AIGER file).\n",
                                           name));
            Ddi_AigarrayNetStoreAiger(rings, 0, name);
            Ddi_Free(rings);
          }
          else {
            Ddi_Bdd_t *rBdd = Ddi_BddMakeMono(rplus);
            
            if (opt->verbosity >= Pdtutil_VerbLevelUsrMax_c) {
              printf("Writing itp reached to %s\n", opt->trav.wR);
            }
            Ddi_BddStore(rBdd, "reached", DDDMP_MODE_TEXT, opt->trav.wR, NULL);
            // Ddi_AigNetStore (rplus, opt->trav.wR, NULL, Pdtutil_Aig2BenchName_c);
            Ddi_Free(rBdd);
          }
        }
        Ddi_Free(care);
        care = Ddi_BddDup(rplus);
      } else if (0) {
        Ddi_Free(rplus);
        rplus = Ddi_BddDup(care);
      }

      if (0 && care != NULL && !Ddi_BddIsOne(care)) {
        /* DONE ABOVE */
        int fp, okCheck;

        //      Ddi_Bdd_t *notReached = Ddi_BddNot(care);
        Ddi_Bdd_t *notReached = Ddi_BddCofactor(care, pvarPs, 1);

        Ddi_BddNotAcc(notReached);
        Ddi_BddComposeAcc(notReached, Fsm_MgrReadVarPS(fsmMgr2),
          Fsm_MgrReadDeltaBDD(fsmMgr2));
        if (Trav_MgrReadAssume(travMgrAig) != NULL) {
          Ddi_BddAndAcc(notReached, Trav_MgrReadAssume(travMgrAig));
        }
        fp = !Ddi_AigSatAnd(notReached, care, invar);
        Pdtutil_Assert(fp, "wrong care set");
        Ddi_Free(notReached);
      }


      Ddi_Free(care);
    } else {
      /* property unknown */
      printf("Property verification aborted...\n");
      itpTimeLimit *= 1.2;
      pdrTimeLimit *= 1.2;

      if (travMgrAig->settings.ints.igrFpRing > 0) {
	Ddi_Bdd_t *r = Trav_MgrReadReached(travMgrAig);
        Ddi_BddSetAig(r);
        care = Ddi_BddDup(r);
        Ddi_Free(rplus);
        rplus = Ddi_BddDup(care);
      }


    }
    travMgrAig = Trav_MgrQuit(travMgrAig);

    if (failureCex != NULL && deltaPropPart != NULL) {
      int i, np, nf = 0;
      Ddi_Bddarray_t *verifResOut =
        Trav_SimulateAndCheckProp(travMgrAig, fsmMgr2, delta, deltaPropPart,
        Fsm_MgrReadInitBDD(fsmMgr2),
        Fsm_MgrReadInitStubBDD(fsmMgr2), failureCex);

      np = Ddi_BddarrayNum(verifResOut);
      Pdtutil_Assert(np == Ddi_BddPartNum(deltaPropPart), "wrong prop num");
      for (i = np - 1; i >= 0; i--) {
        if (Ddi_BddIsZero(Ddi_BddarrayRead(verifResOut, i))) {
          Ddi_BddPartRemove(deltaPropPart, i);
          nf++;
        }
      }
      printf("%d/%d properties disproved\n", nf, np);
      totFail += nf;
      if (Ddi_BddPartNum(deltaPropPart) > 0) {
        // not all properties disproved - recheck
        printf("not all properties disproved - recheck\n");
        k--;
      }
      Ddi_Free(verifResOut);
    }

    if (!verificationEnded && multiProp && opt->mc.wRes != NULL) {
      FILE *fp;

      if (verificationResult < 0)
        verificationResult = 2;
      fp = fopen(opt->mc.wRes, "a");
      if (fp != NULL) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout,
            "Verification result (%s) written to file %s - (pass|fail): (%d|%d)\n",
            verificationResult == 1 ? "FAIL" : (verificationResult ==
              0 ? "PASS" : "UNDECIDED"), opt->mc.wRes, totProved, totFail));
        fprintf(fp, "%d\n", verificationResult);
        fprintf(fp, "b%d\n", currProp);
        fclose(fp);
      } else {
        fprintf(stderr, "Error opening file %s\n", opt->mc.wRes);
      }
      verificationResult = -1;
    }
    //    Ddi_Free(failureCex);
    if (chkRing && (curHit > 0)) {
      Ddi_Bdd_t *f_jj = Ddi_BddarrayRead(fromRings, curHit);

      if (Ddi_BddIsPartConj(f_jj)) {
        f_jj = Ddi_BddPartRead(f_jj, 0);
      }
      Ddi_Bdd_t *fPs = Ddi_BddSubstVars(f_jj, ns, ps);
      Ddi_Bdd_t *myConstr = Ddi_BddDiff(plitPs, clitPs);
      Ddi_Bdd_t *target = Ddi_BddNot(invarspec);
      Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);

      Ddi_BddComposeAcc(target, ps, delta);
      if (Ddi_AigSatAnd(fPs, target, myConstr)) {
        printf("target hit ring (%d) still hit\n", curHit);
      } else {
        printf("target hit ring (%d) fully refined\n", curHit);
      }
      Ddi_BddAndAcc(target, deltaProp); // deltaProp is negated target
      Ddi_BddAndAcc(target, fPs);
      Ddi_BddAndAcc(target, myConstr);
      printf("target %s fully covered by part target\n",
        Ddi_AigSat(target) ? "NOT" : "");

      Ddi_Free(target);
      target = Ddi_BddNot(deltaProp);
      if (Ddi_AigSatAnd(fPs, target, myConstr)) {
        printf("partial target hit ring (%d) still hit\n", curHit);
      } else {
        printf("partial target hit ring (%d) fully refined\n", curHit);
      }
      Ddi_Free(fPs);
      Ddi_Free(target);
      Ddi_Free(myConstr);
    }

    Ddi_Free(deltaProp);

    /* update deltas */
    if (opt->pre.specDecompConstrTh > 0 && lemmasSteps && !doRunBdd
      && shareLemmas) {
      int nSubst = 0;

      i = j = 0;
      for (; j < Ddi_VararrayNum(ps2); j++) {
        int stop = 0;

        vj = Ddi_VararrayRead(ps2, j);
        for (; i < Ddi_VararrayNum(ps) && !stop; i++) {
          vi = Ddi_VararrayRead(ps, i);
          if (vi == vj) {
            if (i != iProp && i != iConstr) {
              //printf("Replacing delta %d: ", i);
              d = Ddi_BddarrayRead(Fsm_MgrReadDeltaBDD(fsmMgr2), j);
              delta = Fsm_MgrReadDeltaBDD(fsmMgr);
              if (0
                && Ddi_BddSize(d) != Ddi_BddSize(Ddi_BddarrayRead(delta, i))) {
                printf("%d -> %d\n", Ddi_BddSize(Ddi_BddarrayRead(delta, i)),
                  Ddi_BddSize(d));

              }
              Ddi_BddarrayWrite(delta, i, d);
              nSubst++;
            }
            stop = 1;
          }
        }
      }
      Pdtutil_Assert(nSubst == Ddi_VararrayNum(ps2) - 2, "wrong num of subst");
    }
    //    fsmMgr3 = Fsm_MgrDup(fsmMgr2);
    Fsm_MgrQuit(fsmMgr2);
  }

  if (pdrClauses != NULL) {
    int i;

    for (i = 1; i < nPdrFrames; i++) {
      Ddi_ClauseArrayFree(pdrClauses[i]);
    }
  }

  Fsm_FsmFree(fsmFsmStore);

  Ddi_Free(leftover);
  Ddi_Free(prevCexes);
  Ddi_Free(assumeConstr);
  Ddi_Free(fromRings);
  Ddi_Free(abstrRefRefinedVars);
  Ddi_Free(rplus);
  Ddi_Free(rplusRings);
  Ddi_Free(plitPs);
  Ddi_Free(clitPs);
  Ddi_Free(pCore);
  Ddi_Free(constrVars);



#if 0
  if (pinv != NULL && Ddi_BddPartNum(pinv) == 1) {
    Ddi_Bdd_t *p0, *p1, *newPinv = NULL;

    Ddi_Free(pinv);
    pinv = Ddi_AigPartitionTop(invarspec, 0);
    p0 = Ddi_AigPartitionTop(Ddi_BddPartRead(pinv, 0), 1);
    for (i = 1; i < Ddi_BddPartNum(pinv); i++) {
      int j, k;

      Ddi_Free(newPinv);
      newPinv = Ddi_BddMakePartDisjVoid(ddiMgr);
      p1 = Ddi_AigPartitionTop(Ddi_BddPartRead(pinv, i), 1);
      for (j = 0; j < Ddi_BddPartNum(p0); j++) {
        for (k = 0; k < Ddi_BddPartNum(p1); k++) {
          Ddi_Bdd_t *p_jk = Ddi_BddAnd(Ddi_BddPartRead(p0, j),
            Ddi_BddPartRead(p1, k));

          Ddi_BddPartInsertLast(newPinv, p_jk);
          Ddi_Free(p_jk);
        }
      }
      Ddi_Free(p0);
      Ddi_Free(p1);
      p0 = Ddi_BddDup(newPinv);
    }
    Ddi_Free(p0);
    Ddi_Free(pinv);
    pinv = newPinv;
  }

  if (pinv != NULL) {
    int n;
    Ddi_Varset_t *pivars = Ddi_VarsetMakeFromArray(pi);

    Ddi_Free(invarspec);
    if (opt->pre.specDecompIndex >= Ddi_BddPartNum(pinv)) {
      opt->pre.specDecompIndex = Ddi_BddPartNum(pinv) - 1;
    }
    invarspec = Ddi_BddPartExtract(pinv, opt->pre.specDecompIndex);
    if (0 && invar != NULL) {
      Ddi_BddSetAig(pinv);
      Ddi_BddNotAcc(pinv);
      Ddi_BddAndAcc(invar, pinv);
    }
    Ddi_Free(pinv);
#if 0
    pinv = Ddi_AigPartitionTop(invarspec, 0);
    for (i = 0; i < Ddi_BddPartNum(pinv); i++) {
      Ddi_Bdd_t *pconstr;

      pconstr = Ddi_BddPartRead(pinv, i);
      //Ddi_BddNotAcc(pconstr);
      if (Ddi_AigSatAnd(Fsm_MgrReadInitBDD(fsmMgr), pconstr, NULL)) {
        Ddi_BddAndAcc(invar, pconstr);
        //Ddi_BddAndAcc(Fsm_MgrReadInitBDD (fsmMgr), pconstr);
        n++;
      }
    }
    Ddi_Free(pinv);
#endif
    Ddi_Free(pivars);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "spec decomp: %d -> %d\n", size,
        Ddi_BddSize(invarspec)));
  }
#endif

  //printf("\n");
  //opt->expt.expertLevel = 2;
  //selectHeuristic(fsmMgr, opt);

quitMem:

  if (decompInfo) {
    Ddi_Free(decompInfo->pArray);
    Ddi_Free(decompInfo->coiArray);
    Ddi_Free(decompInfo->hintVars);
    Pdtutil_Free(decompInfo->partOrdId);
    Pdtutil_Free(decompInfo);
  }
  Ddi_Free(totProp);
  Ddi_Free(opt->trav.auxPsSet);
  Ddi_Free(init);
  Ddi_Free(care);
  Ddi_Free(invar);
  Ddi_Free(invarspec);
  Ddi_Free(psvars);
  Ddi_Free(nsvars);
  Ddi_Free(opt->trav.univQuantifyVars);
  Ddi_Free(opt->trav.hints);

  Fsm_MgrQuit(fsmMgr);
  if (travMgrAig != NULL) {
    Trav_MgrQuit(travMgrAig);
  }

  /* write result on file */

  if (partInvarspec == NULL) {
    if (opt->mc.wRes != NULL && verificationResult >= 0) {
      FILE *fp;

      fp = fopen(opt->mc.wRes, "w");
      if (fp != NULL) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Verification result (%s) written to file %s\n",
            verificationResult ? "FAIL" : "PASS", opt->mc.wRes));
        fprintf(fp, "%d\n", verificationResult);
        fclose(fp);
      } else {
        fprintf(stderr, "Error opening file %s\n", opt->mc.wRes);
      }
    } else if (verificationResult < 0) {
      printf("Verification result is UNKNOWN!\n");
    }
  }

  if (Ddi_MgrCheckExtRef(ddiMgr, 0) == 0) {
    Ddi_MgrPrintExtRef(ddiMgr, 0);
  }

  if (opt->mc.method != Mc_VerifPrintNtkStats_c) {
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Total used memory: %.2f MBytes\n",
        ((float)Ddi_MgrReadMemoryInUse(ddiMgr)) / (2 << 20));
      fprintf(stdout, "Total Verification Time: %.1f\n",
        (util_cpu_time() - startVerifTime) / 1000.0)
      );
  }

  /**********************************************************************/
  /*                       Close DDI manager                            */
  /**********************************************************************/

  Ddi_MgrQuit(ddiMgr);

  /**********************************************************************/
  /*                             End test                               */
  /**********************************************************************/

  return verificationResult;
}


/**Function********************************************************************
  Synopsis    [Apply top-down reduction of ones to bottom layer]
  Description [Apply top-down reduction of ones to bottom layer. This reduces
    a Meta BDD to a conjunctive form]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
WindowPart(
  Ddi_Bdd_t * f,
  Fbv_Globals_t * opt
)
{
  int size, i, j, h, k, tot, totS2, min;
  Ddi_Var_t *v, *splitV;
  Ddi_Varset_t *supp, *smooth, *supp1;
  Ddi_Vararray_t *va;
  Ddi_Bdd_t *f1, *f0, *fa, *fb;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(f);
  Ddi_Bddarray_t *fA;

  Ddi_BddSetMono(f);
  /*Ddi_BddNotAcc(f); */

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "Init States Size: %d\n", Ddi_BddSize(f)));

  Fbv_DagSize(Ddi_BddToCU(f), Ddi_MgrReadNumCuddVars(ddiMgr), opt);

  i = 0;
  tot = totS2 = 0;
  min = Ddi_BddSize(f) / 2000;
  supp = Ddi_BddSupp(f);
  smooth = Ddi_VarsetDup(supp);

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "Support of f #: %d.\n", Ddi_VarsetNum(supp)));

  va = Ddi_VararrayMakeFromVarset(supp, 1);

  for (h = 0; h < Ddi_VararrayNum(va); h++) {
    splitV = Ddi_VararrayRead(va, h);

    for (k = h + 2; k < Ddi_VararrayNum(va); k++) {

      v = Ddi_VararrayRead(va, k);
      j = Ddi_VarCuddIndex(v);

      totS2 += opt->ddi.share2[j];

      if (totS2 < min)
        continue;

      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "SPLIT[%d->%d]\n", h, k);
        fprintf(stdout, "cnt[%d]{%s} = %d(%d)\n", j, Ddi_VarName(v),
          opt->ddi.nodeCnt[j], tot));

      Ddi_VarsetRemoveAcc(smooth, v);

      fA = Ddi_BddarrayAlloc(ddiMgr, 2);

      f1 = Ddi_BddExist(f, smooth);

      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "Exist: %d\n", Ddi_BddSize(f1)));

      Ddi_Free(f1);

      tot += opt->ddi.nodeCnt[j];

#if 1
      f1 = ExistFullPos(f, v, splitV, opt);
#else
      f1 = Ddi_BddMakePartConjVoid(ddiMgr);
#endif
      for (i = 0; i < Ddi_BddPartNum(f1); i++) {
        Ddi_Bdd_t *w = Ddi_BddPartRead(f1, i);

        fa = Ddi_BddConstrain(f, w);
        supp1 = Ddi_BddSupp(f1);

        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
          fprintf(stdout, "{s:%d}%d(%d)-",
            Ddi_VarsetNum(supp1), Ddi_BddSize(w), Ddi_BddSize(fa)));

        Ddi_Free(supp1);
        Ddi_BddarrayWrite(fA, 2 * i, w);
        Ddi_BddarrayWrite(fA, 2 * i + 1, fa);
        Ddi_Free(fa);
      }


      size = Ddi_BddarraySize(fA);

      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, " |%d|\n", size));

      fb = Ddi_BddMakeConst(ddiMgr, 0);
      f0 = Ddi_BddMakeConst(ddiMgr, 1);
      for (i = 0; i < Ddi_BddPartNum(f1); i++) {
        Ddi_Bdd_t *w = Ddi_BddarrayRead(fA, 2 * i);

        fa = Ddi_BddarrayRead(fA, 2 * i + 1);
#if 0
        Ddi_BddConstrainAcc(w, f0);
        Ddi_BddConstrainAcc(fa, f0);

        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
          fprintf(stdout, "%d(%d)-", Ddi_BddSize(w), Ddi_BddSize(fa)));

        Ddi_BddNotAcc(w);
        Ddi_BddSetPartConj(f0);
        Ddi_BddPartInsertLast(f0, w);
        Ddi_BddNotAcc(w);
#endif
        Ddi_BddAndAcc(fa, w);
        Ddi_BddOrAcc(fb, fa);
      }

      size = Ddi_BddarraySize(fA);

      Ddi_Free(f0);
      Ddi_Free(f1);
      Ddi_Free(fb);
      Ddi_Free(fA);
    }
  }

  Ddi_Free(va);
  Ddi_Free(supp);
  Ddi_Free(smooth);
}




/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
ExistPosEdge(
  Ddi_Bdd_t * f,
  Ddi_Var_t * v
)
{
  Ddi_Bdd_t *r;
  DdNode *fCU, *smCU;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(f);

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "EPE: %d", Ddi_BddSize(f)));

  fCU = Ddi_BddToCU(f);
  smCU = Ddi_VarToCU(v);
  r = Ddi_BddMakeFromCU(ddiMgr,
    Cuplus_bddExistAbstractWeak(Ddi_MgrReadMgrCU(ddiMgr), fCU, smCU));

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "->%d\n", Ddi_BddSize(r)));

  return (r);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
ExistFullPos(
  Ddi_Bdd_t * f,
  Ddi_Var_t * v,
  Ddi_Var_t * splitV,
  Fbv_Globals_t * opt
)
{
  Ddi_Bdd_t *r, *auxf, *r0;
  DdNode *fCU, *smCU, *vCU, *tmp;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(f);
  st_table *table = st_init_table(st_ptrcmp, st_ptrhash);
  st_generator *stgen;
  int i, j, n, nTop, left, right, left0, right0, rotate, rotate0;
  node_info_item_t *info, **infoArray;
  Ddi_Var_t *alpha;

  if (table == NULL) {
    Pdtutil_Warning(1, "Out-of-memory, couldn't alloc project table.");
    return (NULL);
  }

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "EFP: %d\n", Ddi_BddSize(f)));

  fCU = Ddi_BddToCU(f);

  opt->ddi.info_item_num = 0;

  alpha = Ddi_VarNewAfterVar(v);
  {
    char name[100];

    sprintf(name, "%s$AUX", Ddi_VarName(splitV));
    Ddi_VarAttachName(alpha, name);
  }
  vCU = Ddi_VarToCU(v);
  smCU = Ddi_VarToCU(alpha);

  tmp = Fbv_bddExistAbstractWeak(Ddi_MgrReadMgrCU(ddiMgr), fCU, smCU, table,
    Ddi_VarCuddIndex(splitV), 0, 0, opt);
  Cudd_Ref(tmp);
  Cudd_IterDerefBdd(Ddi_MgrReadMgrCU(ddiMgr), tmp);

#if 1
  tmp = Fbv_bddExistAbstractWeak(Ddi_MgrReadMgrCU(ddiMgr), fCU, smCU, table,
    Ddi_VarCuddIndex(splitV), 1, 0, opt);
  Cudd_Ref(tmp);
  Cudd_IterDerefBdd(Ddi_MgrReadMgrCU(ddiMgr), tmp);
#endif
  n = st_count(table);
  infoArray = Pdtutil_Alloc(node_info_item_t *, n);
  i = 0;
  st_foreach_item(table, stgen, (char **)&tmp, (char **)&info) {
    if (info->toptop) {
      infoArray[i++] = info;
    }
  }
  nTop = i;
  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "TOP: %d/ BOTTOM: %d\n", nTop, n - nTop));
  n = nTop;

#if 0
  opt->trav.apprGrTh = 2;
#endif

#if 0
  r = Ddi_BddMakePartDisjVoid(ddiMgr);
  for (j = 0; j < opt->trav.apprGrTh; j++) {
    Ddi_Bdd_t *p;

    for (i = 0; i < n; i++) {
#if 1
      if ((i >= (n * j) / opt->trav.apprGrTh)
        && (i < (n * (j + 1)) / opt->trav.apprGrTh)) {
        infoArray[i]->res = 1;
      } else {
        infoArray[i]->res = 0;
      }
#else
      infoArray[i]->res = ((infoArray[i]->num0 == infoArray[i]->num1) + j) % 2;
#endif
    }
#if 0
    for (i = (n * j) / opt->trav.apprGrTh;
      i < (n * (j + 1)) / opt->trav.apprGrTh; i++) {
      infoArray[i]->res = 1;
    }
#endif
    p = Ddi_BddMakeFromCU(ddiMgr,
      Fbv_bddExistAbstractWeak(Ddi_MgrReadMgrCU(ddiMgr), fCU, smCU, table, 1,
        opt));
    Ddi_BddPartInsertLast(r, p);
    Ddi_Free(p);
  }
#endif



  left = right = rotate = left0 = right0 = rotate0 = 0;
  for (i = 0; i < n; i++) {
    DdNode *cu_i = infoArray[i]->cu_bdd;

    for (j = 0; j < n; j++) {
      DdNode *cu_j = infoArray[j]->cu_bdd;
      int res;
      DdNode *resNode;

      if (i == j)
        continue;
      if (!(infoArray[i]->ph0 && !infoArray[i]->ph1))
        continue;
      if (!(!infoArray[j]->ph0 && infoArray[j]->ph1))
        continue;
#if 1

#if 1
      res = nodeMerge(Ddi_MgrReadMgrCU(ddiMgr),
        table, infoArray[i], infoArray[j], smCU, 0, &resNode);
      if (res >= 8) {
        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMed_c,
          fprintf(stdout, "merge\n"));

        nodeMerge(Ddi_MgrReadMgrCU(ddiMgr),
          table, infoArray[i], infoArray[j], smCU, 1, &resNode);
        if (resNode != NULL) {
          infoArray[j]->merge = infoArray[i]->merge = resNode;
          Cudd_Ref(infoArray[i]->merge);
          Cudd_Ref(infoArray[j]->merge);
          left0++;
          infoArray[i]->used = 1;
          infoArray[j]->used = 2;
          {
            Ddi_Bdd_t *a = Ddi_BddMakeFromCU(ddiMgr, cu_i);
            Ddi_Bdd_t *b = Ddi_BddMakeFromCU(ddiMgr, cu_j);
            Ddi_Bdd_t *m = Ddi_BddMakeFromCU(ddiMgr, resNode);
            Ddi_Bdd_t *tot = Ddi_BddMakePartConjVoid(ddiMgr);

            Ddi_BddPartInsertLast(tot, a);
            Ddi_BddPartInsertLast(tot, b);
            Ddi_Free(a);
            Ddi_Free(b);
            Ddi_Free(m);
            Ddi_Free(tot);
          }
        }
      }
#endif
#else
      if (cu_i->index == cu_j->index && cuddT(cu_i) == cuddT(cu_j)) {
        if (cuddT(cu_i) != DD_ONE(Ddi_MgrReadMgrCU(ddiMgr))) {
          Pdtutil_Assert(cuddE(cu_i) != cuddE(cu_j), "invalid merge cand.");
          left++;
          if (!infoArray[i]->used && !infoArray[j]->used) {
            DdNode *T = Cudd_T(cu_i);
            DdNode *E_i = Cudd_E(cu_i);
            DdNode *E_j = Cudd_E(cu_j);
            DdNode *tmpCU =
              Cudd_bddIte(Ddi_MgrReadMgrCU(ddiMgr), smCU, E_i, E_j);
            Cudd_Ref(tmpCU);
            infoArray[j]->merge = infoArray[i]->merge =
              Cudd_bddIte(Ddi_MgrReadMgrCU(ddiMgr), topV, T, tmpCU);
            Cudd_Ref(infoArray[i]->merge);
            Cudd_Ref(infoArray[j]->merge);
            Cudd_IterDerefBdd(Ddi_MgrReadMgrCU(ddiMgr), tmpCU);

            left0++;
            infoArray[i]->used = 1;
            infoArray[j]->used = 2;
          }
        }
      }
      if (cu_i->index == cu_j->index && cuddE(cu_i) == cuddE(cu_j)) {
        if (((cuddE(cu_i) != DD_ONE(Ddi_MgrReadMgrCU(ddiMgr))) &&
            (cuddE(cu_i) != Cudd_Not(DD_ONE(Ddi_MgrReadMgrCU(ddiMgr)))))) {
          Pdtutil_Assert(cuddT(cu_i) != cuddT(cu_j), "invalid merge cand.");
          right++;
          if (!infoArray[i]->used && !infoArray[j]->used) {

            DdNode *topV =
              Cudd_bddIthVar(Ddi_MgrReadMgrCU(ddiMgr), cu_i->index);
            DdNode *E = Cudd_E(cu_i);
            DdNode *T_i = Cudd_T(cu_i);
            DdNode *T_j = Cudd_T(cu_j);
            DdNode *tmpCU =
              Cudd_bddIte(Ddi_MgrReadMgrCU(ddiMgr), smCU, T_i, T_j);
            Cudd_Ref(tmpCU);
            infoArray[j]->merge = infoArray[i]->merge =
              Cudd_bddIte(Ddi_MgrReadMgrCU(ddiMgr), topV, tmpCU, E);
            Cudd_Ref(infoArray[i]->merge);
            Cudd_Ref(infoArray[j]->merge);
            Cudd_IterDerefBdd(Ddi_MgrReadMgrCU(ddiMgr), tmpCU);

            right0++;
            infoArray[i]->used = 1;
            infoArray[j]->used = 2;
          }
        }
      }
#endif
    }
  }

#if 1
  r = Ddi_BddMakeFromCU(ddiMgr,
    Fbv_bddExistAbstractWeak(Ddi_MgrReadMgrCU(ddiMgr), fCU, smCU, table, -1, 0,
      3, opt));
#if 0
  auxf = Ddi_BddMakeFromCU(ddiMgr,
    Fbv_bddExistAbstractWeak(Ddi_MgrReadMgrCU(ddiMgr), fCU, smCU, table, -1, 0,
      1, opt));
  r0 =
    Ddi_BddMakeFromCU(ddiMgr,
    Fbv_bddExistAbstractWeak(Ddi_MgrReadMgrCU(ddiMgr), fCU, smCU, table, -1, 0,
      2, opt));
#else
  auxf = Ddi_BddMakeLiteral(splitV, 1);
#endif

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "(l:%d(%d), r:%d(%d), rot:%d(%d)/%d\n",
      left, left0, right, right0, rotate, rotate0, n));

  st_foreach_item(table, stgen, (char **)&tmp, (char **)&info) {
    if (info->merge != NULL) {
      Cudd_IterDerefBdd(Ddi_MgrReadMgrCU(ddiMgr), info->merge);
    }
    Pdtutil_Free(info);
  }
  st_free_table(table);
  Pdtutil_Free(infoArray);

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "->%d+%d", Ddi_BddSize(r), Ddi_BddSize(auxf)));

  {
    Ddi_Vararray_t *va = Ddi_VararrayAlloc(ddiMgr, 1);
    Ddi_Bddarray_t *ba = Ddi_BddarrayAlloc(ddiMgr, 1);

    Ddi_VararrayWrite(va, 0, alpha);
    Ddi_BddarrayWrite(ba, 0, auxf);

#if 1
#if 1
    r0 = Ddi_BddCompose(r, va, ba);
    Pdtutil_Assert(Ddi_BddEqual(f, r0), "NOT EQUAL");
#endif
#else
    r0 = Ddi_BddCofactor(r, alpha, 0);
    r1 = Ddi_BddCofactor(r, alpha, 1);
    Ddi_BddOrAcc(r0, r1);
    Ddi_Free(r1);
    Pdtutil_Assert(Ddi_BddIncluded(f, r0), "NOT INCL");
#endif
    Ddi_Free(r0);
    Ddi_Free(va);
    Ddi_Free(ba);
  }
#else
  r = Ddi_BddMakeConst(ddiMgr, 1);
#endif

  Ddi_BddSetPartConj(r);
  return (r);
}



/**Function********************************************************************

  Synopsis    [Counts the number of nodes in a DD.]

  Description [Counts the number of nodes in a DD. Returns the number
  of nodes in the graph rooted at node.]

  SideEffects [None]

  SeeAlso     [Cudd_SharingSize Cudd_PrintDebug]

******************************************************************************/
int
Fbv_DagSize(
  DdNode * node,
  int nvar,
  Fbv_Globals_t * opt
)
{
  int i, j, tot, totN, totZ, totS, totS2;

  for (j = 0; j < nvar; j++) {
    opt->ddi.nodeCnt[j] = 0;
    opt->ddi.share[j] = 0;
    opt->ddi.share2[j] = 0;
    opt->ddi.negE[j] = 0;
    opt->ddi.zeroE[j] = 0;
  }

  i = fbvDagInt(Cudd_Regular(node), opt);
  fbvClearFlag(Cudd_Regular(node));

  tot = totN = totZ = totS = totS2 = 0;
  for (j = 0; j < nvar; j++) {
    tot += opt->ddi.nodeCnt[j];
    totS += opt->ddi.share[j];
    totS2 += opt->ddi.share2[j];
    totN += opt->ddi.negE[j];
    totZ += opt->ddi.zeroE[j];

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "cnt[%d] = %d/%d/%d/%d/%d(%d/%d/%d/%d/%d)\n", j,
        opt->ddi.nodeCnt[j], opt->ddi.negE[j], opt->ddi.zeroE[j],
        opt->ddi.share[j], opt->ddi.share2[j], tot, totN, totZ, totS, totS2));
  }

  return (i);

}                               /* end of Cudd_DagSize */



/**Function********************************************************************

  Synopsis    [Performs the recursive step of Cudd_DagSize.]

  Description [Performs the recursive step of Cudd_DagSize. Returns the
  number of nodes in the graph rooted at n.]

  SideEffects [None]

******************************************************************************/
static int
fbvDagInt(
  DdNode * n,
  Fbv_Globals_t * opt
)
{
  int tval, eval;
  DdNode *T, *E;

  if (Cudd_IsComplement(n->next)) {
    return (0);
  }
  n->next = Cudd_Not(n->next);
  if (cuddIsConstant(n)) {
    return (1);
  }
  T = cuddT(n);
  E = Cudd_Regular(cuddE(n));
  tval = fbvDagInt(T, opt);
  eval = fbvDagInt(E, opt);

  if ((!cuddIsConstant(T)) && (!cuddIsConstant(E))) {
    if ((n->ref < T->ref) && (n->ref < E->ref)) {
      opt->ddi.share[n->index]++;
    }
    if ((4 * n->ref < T->ref) && (4 * n->ref < E->ref)) {
      opt->ddi.share2[n->index]++;
    }
  }

  if (Cudd_IsComplement(cuddE(n))) {
    opt->ddi.negE[n->index]++;
    if (cuddIsConstant(Cudd_Regular(cuddE(n)))) {
      opt->ddi.zeroE[n->index]++;
    }
  }

  opt->ddi.nodeCnt[n->index]++;

  return (1 + tval + eval);

}                               /* end of ddDagInt */



/**Function********************************************************************

  Synopsis    [Performs a DFS from f, clearing the LSB of the next
  pointers.]

  Description []

  SideEffects [None]

  SeeAlso     [ddSupportStep ddDagInt]

******************************************************************************/
static void
fbvClearFlag(
  DdNode * f
)
{
  if (!Cudd_IsComplement(f->next)) {
    return;
  }
  /* Clear visited flag. */
  f->next = Cudd_Regular(f->next);
  if (cuddIsConstant(f)) {
    return;
  }
  fbvClearFlag(cuddT(f));
  fbvClearFlag(Cudd_Regular(cuddE(f)));
  return;

}                               /* end of ddClearFlag */



/**Function********************************************************************

  Synopsis [Existentially abstracts all the variables in cube from f.]

  Description [Existentially abstracts all the variables in cube from f.
  Returns the abstracted BDD if successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_bddUnivAbstract Cudd_addExistAbstract]

******************************************************************************/
DdNode *
Fbv_bddExistAbstractWeak(
  DdManager * manager,
  DdNode * f,
  DdNode * cube,
  st_table * table,
  int split,
  int splitPhase,
  int phase,
  Fbv_Globals_t * opt
)
{
  DdNode *res;

  do {
    manager->reordered = 0;
    res = fbvBddExistAbstractWeakRecur(manager, f, cube, table, split,
      splitPhase, phase, opt);
  } while (manager->reordered == 1);

  return (res);

}                               /* end of Cudd_bddExistAbstract */



/**Function********************************************************************

  Synopsis    [Performs the recursive steps of Cudd_bddExistAbstract.]

  Description [Performs the recursive steps of Cudd_bddExistAbstract.
  Returns the BDD obtained by abstracting the variables
  of cube from f if successful; NULL otherwise. It is also used by
  Cudd_bddUnivAbstract.]

  SideEffects [None]

  SeeAlso     [Cudd_bddExistAbstract Cudd_bddUnivAbstract]

******************************************************************************/
DdNode *
fbvBddExistAbstractWeakRecur(
  DdManager * manager,
  DdNode * f,
  DdNode * cube,
  st_table * table,
  int split,
  int splitPhase,
  int phase,
  Fbv_Globals_t * opt
)
{
  DdNode *F, *T, *E, *res, *res1, *res2, *one;
  node_info_item_t *info;

  statLine(manager);
  one = DD_ONE(manager);
  F = Cudd_Regular(f);

  /* Cube is guaranteed to be a cube at this point. */
  if (cube == one) {
    return (one);
  }
#if 0
  if (F == one) {
    return (one);
  }
#endif
  /* From now on, f and cube are non-constant. */

  /* Abstract a variable that does not appear in f. */
#if 0
  while (manager->perm[F->index] > manager->perm[cube->index]) {
    cube = cuddT(cube);
    if (cube == one)
      return (f);
  }
#endif


  info = hashSet(table, F, opt);
  if (split >= 0) {
    if (splitPhase) {
      info->ph1 = 1;
    } else {
      info->ph0 = 1;
    }
  }


  if (F != one) {
#if 0
    /* Check the cache. */
    if (F->ref != 1 && (res = cuddCacheLookup2(manager,
          fbvBddExistAbstractWeakRecur, f, cube, table, phase, opt)) != NULL) {
      return (res);
    }
#endif


    /* Compute the cofactors of f. */
    T = cuddT(F);
    E = cuddE(F);
    if (f != F) {
      T = Cudd_Not(T);
      E = Cudd_Not(E);
    }
    if ((manager->perm[Cudd_NodeReadIndex(F)] < manager->perm[cube->index])
#if 0
      && (Cudd_IsConstant(T) || Cudd_IsConstant(E)
        || (manager->perm[Cudd_NodeReadIndex(T)] > manager->perm[cube->index])
        || (manager->perm[Cudd_NodeReadIndex(E)] > manager->perm[cube->index]))
#endif
      ) {

      info->top = 1;

      if (phase == 1) {
        if (info->merge != NULL && (info->used % 2 == 1)) {
          return (one);
        }
      } else if (phase == 2) {
        if (info->merge != NULL) {
          return (one);
        }
      } else {
        if (info->merge != NULL) {
          if (f != F) {
            return (Cudd_Not(info->merge));
          } else {
            return (info->merge);
          }
        }
      }

    }

  }

  /* If the two indices are the same, so are their levels. */

  if (F == one || manager->perm[F->index] > manager->perm[cube->index]) {
    node_info_item_t *info;

    info = hashSet(table, F, opt);
    info->toptop = 0;


    if (phase == 3) {
      if (info->merge != NULL) {
        int doMerge = 1;

        if (splitPhase) {
          if (!info->ph1)
            doMerge = 0;
        } else {
          if (!info->ph0)
            doMerge = 0;
        }
        if (doMerge) {
          if (f != F) {
            return (Cudd_Not(info->merge));
          } else {
            return (info->merge);
          }
        }
      }
    }

    if (split >= 0) {
      if (splitPhase) {
        info->ph1 = 1;
      } else {
        info->ph0 = 1;
      }
    }
    if (phase == 0) {
      info->res = 0;
      return (one);
    } else if (phase == 1 || phase == 2) {
      return (Cudd_Not(one));
    }
    return (f);

  } else {                      /* if (cuddI(manager,F->index) < cuddI(manager,cube->index)) */
    if (split >= 0 && F->index == split && splitPhase == 0) {
      res1 = T;
    } else {
      res1 = fbvBddExistAbstractWeakRecur(manager, T, cube, table,
        split, splitPhase, phase, opt);
    }
    if (res1 == NULL)
      return (NULL);
    cuddRef(res1);
    if (split >= 0 && F->index == split && splitPhase == 1) {
      res2 = E;
    } else {
      res2 = fbvBddExistAbstractWeakRecur(manager, E, cube, table,
        split, splitPhase, phase, opt);
    }
    if (res2 == NULL) {
      Cudd_IterDerefBdd(manager, res1);
      return (NULL);
    }
    cuddRef(res2);
    /* ITE takes care of possible complementation of res1 */
    res = cuddBddIteRecur(manager, manager->vars[F->index], res1, res2);
    if (res == NULL) {
      Cudd_IterDerefBdd(manager, res1);
      Cudd_IterDerefBdd(manager, res2);
      return (NULL);
    }
    cuddRef(res);
    cuddDeref(res1);
    cuddDeref(res2);
#if 0
    if (F->ref != 1)
      cuddCacheInsert2(manager, (DdNode * (*)(DdManager *, DdNode *, DdNode *))
        fbvBddExistAbstractWeakRecur, f, cube, res);
#endif
    cuddDeref(res);
    return (res);
  }

}                               /* end of fbvBddExistAbstractWeakRecur */


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
approxImgAux(
  Ddi_Bdd_t * trBdd,
  Ddi_Bdd_t * from,
  Ddi_Varset_t smoothV
)
{
  Ddi_Bdd_t *to;

  if (Ddi_BddIsMono(from)) {
  }

  return (to);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
orProjectedSetAcc(
  Ddi_Bdd_t * f,
  Ddi_Bdd_t * g,
  Ddi_Var_t * partVar
)
{
  Ddi_Varset_t *smooth, *smoothi, *supp;
  Ddi_Bdd_t *p, *gProj;
  int i, j;

  if (Ddi_BddIsMono(f)) {
    return (Ddi_BddOrAcc(f, g));
  }

  if (Ddi_BddIsMeta(f)) {
    return (Ddi_BddOrAcc(f, g));
  }

  if (Ddi_BddIsPartDisj(f)) {
    Ddi_Bdd_t *p = NULL;

    j = -1;
    if (partVar != NULL) {
      Ddi_Bdd_t *lit = Ddi_BddMakeLiteral(partVar, 0);

      if (!Ddi_BddIncluded(g, lit)) {
        Ddi_BddNotAcc(lit);
        if (!Ddi_BddIncluded(g, lit)) {
          Ddi_Free(lit);
        }
      }
      if (lit != NULL) {
        for (i = 0; i < Ddi_BddPartNum(f); i++) {
          if (!Ddi_BddIsZero(Ddi_BddPartRead(f, i)) &&
            Ddi_BddIncluded(Ddi_BddPartRead(f, i), lit)) {
            j = i;
            break;
          }
        }
        if (j < 0) {
          j = 0;
          for (i = 0; i < Ddi_BddPartNum(f); i++) {
            if (Ddi_BddIsZero(Ddi_BddPartRead(f, i))) {
              j = i;
              break;
            }
          }
        }
        Ddi_Free(lit);
      } else {
        j = 0;
      }
    } else {
      j = 0;
    }
    p = Ddi_BddPartRead(f, j);
    if (Ddi_BddIsZero(p)) {
      Ddi_BddPartWrite(f, j, g);
    } else {
      orProjectedSetAcc(p, g, NULL);
    }
    return (f);
  }

  smooth = Ddi_BddSupp(g);

  for (i = 0; i < Ddi_BddPartNum(f); i++) {

    p = Ddi_BddPartRead(f, i);
#if 1
    supp = Ddi_BddSupp(p);
    smoothi = Ddi_VarsetDiff(smooth, supp);
    gProj = Ddi_BddExist(g, smoothi);
    Ddi_BddOrAcc(p, gProj);
    Ddi_Free(gProj);
    Ddi_Free(supp);
    Ddi_Free(smoothi);
#else
    Ddi_BddOrAcc(p, g);
#endif

  }
  Ddi_Free(smooth);

  return (f);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
bddExistAppr(
  Ddi_Bdd_t * f,
  Ddi_Varset_t * sm
)
{
  Ddi_Bdd_t *p, *r;
  int i;

  if (!Ddi_BddIsPartConj(f)) {
    return (Ddi_BddExist(f, sm));
  }

  r = Ddi_BddDup(f);

  for (i = 0; i < Ddi_BddPartNum(r); i++) {
    p = Ddi_BddPartRead(r, i);
    Ddi_BddExistAcc(p, sm);
  }

  return (r);

}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Varsetarray_t *
computeCoiVars(
  Tr_Tr_t * tr,
  Ddi_Bdd_t * p,
  int maxIter
)
{
  Ddi_Varsetarray_t *coirings;
  Ddi_Varset_t *ps, *ns, *supp, *cone, *New, *newnew;
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Ddi_Bdd_t *trBdd;
  Ddi_Vararray_t *psv;
  Ddi_Vararray_t *nsv;
  Ddi_Varsetarray_t *domain, *range;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(p);

  int i, j, np;

  psv = Tr_MgrReadPS(trMgr);
  nsv = Tr_MgrReadNS(trMgr);
  ps = Ddi_VarsetMakeFromArray(psv);
  ns = Ddi_VarsetMakeFromArray(nsv);

  coirings = Ddi_VarsetarrayAlloc(ddiMgr, 0);

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "COI Reduction: ")
    );

  trBdd = Ddi_BddDup(Tr_TrBdd(tr));
  if (!(Ddi_BddIsPartConj(trBdd) || Ddi_BddIsPartDisj(trBdd))) {
    Pdtutil_Warning(1, "Partitioned TR required for COI reduction\n");
    Ddi_BddSetPartConj(trBdd);
  }
  np = Ddi_BddPartNum(trBdd);
  domain = Ddi_VarsetarrayAlloc(ddiMgr, np);
  range = Ddi_VarsetarrayAlloc(ddiMgr, np);
  for (i = 0; i < np; i++) {
    supp = Ddi_BddSupp(Ddi_BddPartRead(trBdd, i));
    Ddi_VarsetIntersectAcc(supp, ps);
    Ddi_VarsetarrayWrite(domain, i, supp);
    Ddi_Free(supp);
    supp = Ddi_BddSupp(Ddi_BddPartRead(trBdd, i));
    Ddi_VarsetIntersectAcc(supp, ns);
    Ddi_VarsetarrayWrite(range, i, supp);
    Ddi_Free(supp);
  }

  cone = Ddi_BddSupp(p);
  Ddi_VarsetIntersectAcc(cone, ps); /* remove PI vars */

  New = Ddi_VarsetDup(cone);

  j = 0;
  while (((j++ < maxIter) || (maxIter <= 0)) && (!Ddi_VarsetIsVoid(New))) {
    Ddi_VarsetUnionAcc(cone, New);
    Ddi_VarsetarrayInsertLast(coirings, cone);
    Ddi_VarsetSwapVarsAcc(New, psv, nsv);
    newnew = Ddi_VarsetVoid(ddiMgr);
    for (i = 0; i < np; i++) {
      Ddi_Varset_t *common;

      supp = Ddi_VarsetarrayRead(range, i);
      common = Ddi_VarsetIntersect(supp, New);
      if (!Ddi_VarsetIsVoid(common)) {
#if 0
        if ((opt->pre.coiSets != NULL) && (opt->pre.coiSets[i] != NULL)) {
          Ddi_VarsetUnionAcc(cone, opt->pre.coiSets[i]);
        } else
#endif
        {
          Ddi_VarsetUnionAcc(newnew, Ddi_VarsetarrayRead(domain, i));
        }
      }
      Ddi_Free(common);
    }
    Ddi_Free(New);
    New = Ddi_VarsetDiff(newnew, cone);
    Ddi_Free(newnew);

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, ".(%d)", Ddi_VarsetNum(cone))
      );
  }
  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c, fprintf(stdout, "\n")
    );

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "COI Reduction Performed: Kept %d State Vars out of %d.\n",
      Ddi_VarsetNum(cone), Ddi_VarsetNum(ps)));

  Ddi_Free(New);
  Ddi_Free(domain);
  Ddi_Free(range);
  Ddi_Free(trBdd);
  Ddi_Free(ps);
  Ddi_Free(ns);

  Ddi_Free(cone);

  return (coirings);

}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Varsetarray_t *
computeFsmCoiVars(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * p,
  Ddi_Varset_t * extraVars,
  int maxIter,
  int verbosityLocal
)
{
  Ddi_Varsetarray_t *coirings;
  Ddi_Varset_t *ps, *ns, *supp, *cone, *New, *newnew;
  Ddi_Bddarray_t *delta;
  Ddi_Vararray_t *psv;
  Ddi_Vararray_t *nsv;
  Ddi_Varsetarray_t *domain, *range;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(p);
  int i, j, np;

  psv = Fsm_MgrReadVarPS(fsmMgr);
  nsv = Fsm_MgrReadVarNS(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);

  coirings = Ddi_VarsetarrayAlloc(ddiMgr, 0);

  Pdtutil_VerbosityLocal(verbosityLocal, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "COI Reduction: "));

  np = Ddi_BddarrayNum(delta);

  if (Ddi_BddIsAig(Ddi_BddarrayRead(delta, 0))) {
    int freeD = 0;
    Ddi_Varset_t *coi = NULL;

    if (Ddi_BddIsPartConj(Ddi_BddarrayRead(delta, np - 2))) {
      if (!freeD) {
        delta = Ddi_BddarrayDup(Fsm_MgrReadDeltaBDD(fsmMgr));
        freeD = 1;
      }
      Ddi_BddSetAig(Ddi_BddarrayRead(delta, np - 2));
    }
    coi = Ddi_AigCoi(p, psv, delta);
    Ddi_VarsetarrayInsertLast(coirings, coi);

    Pdtutil_VerbosityLocal(verbosityLocal, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout,
        "COI Reduction Performed: Kept %d State Vars out of %d.\n",
        Ddi_VarsetNum(coi), Ddi_VararrayNum(psv))
      );

    if (freeD) {
      Ddi_Free(delta);
    }

    Ddi_Free(coi);
    return coirings;
  }

  Ddi_MgrSiftSuspend(ddiMgr);

  ps = Ddi_VarsetMakeFromArray(psv);
  ns = Ddi_VarsetMakeFromArray(nsv);

  domain = Ddi_VarsetarrayAlloc(ddiMgr, np);
  range = Ddi_VarsetarrayAlloc(ddiMgr, np);
  for (i = 0; i < np; i++) {
    supp = Ddi_BddSupp(Ddi_BddarrayRead(delta, i));
    Ddi_VarsetIntersectAcc(supp, ps);   /* @ */
    Ddi_VarsetarrayWrite(domain, i, supp);
    Ddi_Free(supp);
    supp = Ddi_VarsetMakeFromVar(Ddi_VararrayRead(nsv, i));
    Ddi_VarsetarrayWrite(range, i, supp);
    Ddi_Free(supp);
  }

  cone = Ddi_BddSupp(p);
  if (extraVars != NULL) {
    Ddi_VarsetUnionAcc(cone, extraVars);
  }
  if (Ddi_VarsetIsArray(cone)) {
    Ddi_VarsetSetArray(ps);
  }
  Ddi_VarsetIntersectAcc(cone, ps); /* remove PI vars */
  if (Ddi_VarsetIsArray(cone)) {
    Ddi_VarsetSetBdd(cone);
  }

  New = Ddi_VarsetDup(cone);

  j = 0;
  while (((j++ < maxIter) || (maxIter <= 0)) && (!Ddi_VarsetIsVoid(New))) {
    if (Ddi_VarsetIsArray(New)) {
      Ddi_VarsetSetBdd(New);
    }
    Ddi_VarsetUnionAcc(cone, New);
    Ddi_VarsetarrayInsertLast(coirings, cone);
    Ddi_VarsetSwapVarsAcc(New, psv, nsv);
    newnew = Ddi_VarsetVoid(ddiMgr);

    //      fprintf(stdout, "NNN: %d\n", j);
    //  Ddi_VarsetPrint(New,0,0,stdout);

    for (i = 0; i < np; i++) {
      Ddi_Varset_t *common;

      supp = Ddi_VarsetarrayRead(range, i);
      if (Ddi_VarsetIsArray(New)) {
        Ddi_VarsetSetArray(supp);
      }
      common = Ddi_VarsetIntersect(supp, New);
      // fprintf(stdout, "SSS: %d\n", i);
      // Ddi_VarsetPrint(supp,0,0,stdout);
      if (!Ddi_VarsetIsVoid(common)) {
#if 0
        if ((opt->pre.coiSets != NULL) && (opt->pre.coiSets[i] != NULL)) {
          Ddi_VarsetUnionAcc(cone, opt->pre.coiSets[i]);
        } else
#endif
        {
          //        fprintf(stdout, "CCC: %d\n", i);
          if (Ddi_VarsetIsArray(Ddi_VarsetarrayRead(domain, i))) {
            Ddi_VarsetSetArray(newnew);
          }
          if (Ddi_VarsetIsArray(newnew)) {
            Ddi_VarsetSetArray(Ddi_VarsetarrayRead(domain, i));
          }
          Ddi_VarsetUnionAcc(newnew, Ddi_VarsetarrayRead(domain, i));
          // Ddi_VarsetPrint(Ddi_VarsetarrayRead(domain,i)),0,0,stdout);
        }
      }
      Ddi_Free(common);
    }
    Ddi_Free(New);
    New = Ddi_VarsetDiff(newnew, cone);
    Ddi_Free(newnew);

    Pdtutil_VerbosityLocal(verbosityLocal, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, ".(%d)", Ddi_VarsetNum(cone))
      );
  }

  Pdtutil_VerbosityLocal(verbosityLocal, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "\n")
    );

  Pdtutil_VerbosityLocal(verbosityLocal, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "COI Reduction Performed: Kept %d State Vars out of %d.\n",
      Ddi_VarsetNum(cone), Ddi_VarsetNum(ps))
    );

  Ddi_Free(New);
  Ddi_Free(delta);
  Ddi_Free(domain);
  Ddi_Free(range);
  Ddi_Free(ps);
  Ddi_Free(ns);

  Ddi_Free(cone);

  Ddi_MgrSiftResume(ddiMgr);


  return (coirings);

}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static coi_scc_info_t *
coiSccInfoInit(
  int n
)
{
  coi_scc_info_t *info = Pdtutil_Alloc(coi_scc_info_t, 1);

  info->size = n;
  info->visited = Pdtutil_Alloc(char,
    n
  );
  info->sc = Pdtutil_Alloc(int,
    n
  );
  info->stack = Pdtutil_Alloc(int,
    n
  );
  info->low = Pdtutil_Alloc(int,
    n
  );
  info->pre = Pdtutil_Alloc(int,
    n
  );

  info->stackIndex = info->cnt0 = info->cnt1 = 0;

  return info;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static void
coiSccInfoQuit(
  coi_scc_info_t * info
)
{
  Pdtutil_Free(info->visited);
  Pdtutil_Free(info->sc);
  Pdtutil_Free(info->stack);
  Pdtutil_Free(info->low);
  Pdtutil_Free(info->pre);

  Pdtutil_Free(info);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Varsetarray_t *
computeFsmCoiRings(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * p,
  Ddi_Varset_t * extraVars,
  int maxIter,
  int verbosityLocal
)
{
  Ddi_Varsetarray_t *coirings;
  Ddi_Varset_t *ps, *ns, *supp, *cone, *New, *newNew;
  Ddi_Bddarray_t *delta;
  Ddi_Vararray_t *psv;
  Ddi_Vararray_t *nsv;
  Ddi_Varsetarray_t *domain, *range;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(p);
  int i, j, np;

  psv = Fsm_MgrReadVarPS(fsmMgr);
  nsv = Fsm_MgrReadVarNS(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);

  Ddi_MgrSiftSuspend(ddiMgr);

  ps = Ddi_VarsetMakeFromArray(psv);
  ns = Ddi_VarsetMakeFromArray(nsv);

  coirings = Ddi_VarsetarrayAlloc(ddiMgr, 0);

  Pdtutil_VerbosityLocal(verbosityLocal, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "COI Reduction: "));

  np = Ddi_BddarrayNum(delta);
  domain = Ddi_VarsetarrayAlloc(ddiMgr, np);
  range = Ddi_VarsetarrayAlloc(ddiMgr, np);
  for (i = 0; i < np; i++) {
    supp = Ddi_BddSupp(Ddi_BddarrayRead(delta, i));
    if (Ddi_VarsetIsArray(ps)) {
      Ddi_VarsetSetArray(supp);
    }
    Ddi_VarsetIntersectAcc(supp, ps);   /* @ */
    Ddi_VarsetarrayWrite(domain, i, supp);
    Ddi_Free(supp);
    supp = Ddi_VarsetMakeFromVar(Ddi_VararrayRead(nsv, i));
    if (Ddi_VarsetIsArray(ps)) {
      Ddi_VarsetSetArray(supp);
    }
    Ddi_VarsetarrayWrite(range, i, supp);
    Ddi_Free(supp);
  }

  cone = Ddi_BddSupp(p);
  if (extraVars != NULL) {
    Ddi_VarsetUnionAcc(cone, extraVars);
  }
  if (Ddi_VarsetIsArray(cone)) {
    Ddi_VarsetSetArray(ps);
  }
  if (Ddi_VarsetIsArray(ps)) {
    Ddi_VarsetSetArray(cone);
  }
  Ddi_VarsetIntersectAcc(cone, ps); /* remove PI vars */

  New = Ddi_VarsetDup(cone);

  j = 0;

  newNew = Ddi_VarsetDup(New);
  
  while ((j++ < maxIter) || ((maxIter <= 0)) && (!Ddi_VarsetIsVoid(newNew))) {

    Ddi_Free(newNew);
    if (Ddi_VarsetIsArray(New)) {
      Ddi_VarsetSetArray(cone);
    }
    Ddi_VarsetUnionAcc(cone, New);
    Ddi_VarsetarrayInsertLast(coirings, New);
    Ddi_VarsetSwapVarsAcc(New, psv, nsv);

    //      fprintf(stdout, "NNN: %d\n", j);
    //  Ddi_VarsetPrint(New,0,0,stdout);

    Ddi_VarsetWriteMark (ps, 0);
    Ddi_VarsetWriteMark (ns, 0);
    Ddi_VarsetWriteMark (New, 1);
    Ddi_Free(New);
    New = Ddi_VarsetVoid(ddiMgr);
    
    for (i = 0; i < np; i++) {
      Ddi_Var_t *ns_i = Ddi_VararrayRead(nsv,i);
      if (Ddi_VarReadMark(ns_i) > 0) {
        supp = Ddi_VarsetarrayRead(domain, i);
        Ddi_VarsetWriteMark (supp, 1);
      }
    }
    Ddi_VarsetWriteMark (ns, 0);
    
    for (i = 0; i < np; i++) {
      Ddi_Var_t *ps_i = Ddi_VararrayRead(psv,i);
      if (Ddi_VarReadMark(ps_i) > 0) {
        Ddi_VarsetAddAcc(New,ps_i);
      }
    }

    newNew = Ddi_VarsetDiff(New,cone);
    
    Ddi_VarsetWriteMark (ps, 0);

    Pdtutil_VerbosityLocal(verbosityLocal, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, ".(%d)", Ddi_VarsetNum(cone))
      );
  }

  Pdtutil_VerbosityLocal(verbosityLocal, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "\n")
    );

  Pdtutil_VerbosityLocal(verbosityLocal, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "COI Reduction Performed: Kept %d State Vars out of %d.\n",
      Ddi_VarsetNum(cone), Ddi_VarsetNum(ps))
    );

  Ddi_Free(New);
  Ddi_Free(newNew);
  Ddi_Free(domain);
  Ddi_Free(range);
  Ddi_Free(ps);
  Ddi_Free(ns);
  Ddi_Free(cone);

  Ddi_MgrSiftResume(ddiMgr);
  return (coirings);

}

/**Function********************************************************************
  Synopsis    [Eval CTL expression]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
Ddi_Expr_t *
Fbv_EvalCtlSpec(
  Tr_Tr_t * TR,
  Ddi_Expr_t * e,
  Fbv_Globals_t * opt
)
{
  Ddi_Expr_t *r, *op0, *op1;
  Ddi_Bdd_t *rBdd, *opBdd;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(e);
  int opcode;

  switch (Ddi_GenericReadCode((Ddi_Generic_t *) e)) {
    case Ddi_Expr_Bdd_c:
      r = Ddi_ExprDup(e);
      break;
    case Ddi_Expr_Bool_c:
      break;
    case Ddi_Expr_Ctl_c:

      opcode = Ddi_ExprReadOpcode(e);
      switch (opcode) {

        case Expr_Ctl_SPEC_c:
        case Expr_Ctl_INVARSPEC_c:
          op0 = Fbv_EvalCtlSpec(TR, Ddi_ExprReadSub(e, 0), opt);
          r = Ddi_ExprCtlMake(ddiMgr, opcode, op0, NULL, NULL);
          Ddi_Free(op0);
          break;
        case Expr_Ctl_TERMINAL_c:
        case Expr_Ctl_TRUE_c:
        case Expr_Ctl_FALSE_c:
          break;

        case Expr_Ctl_AND_c:
          op0 = Fbv_EvalCtlSpec(TR, Ddi_ExprReadSub(e, 0), opt);
          op1 = Fbv_EvalCtlSpec(TR, Ddi_ExprReadSub(e, 1), opt);
          rBdd = Ddi_BddAnd(Ddi_ExprToBdd(op0), Ddi_ExprToBdd(op1));
          Ddi_Free(op0);
          Ddi_Free(op1);
          r = Ddi_ExprMakeFromBdd(rBdd);
          Ddi_Free(rBdd);
          break;
        case Expr_Ctl_OR_c:
          op0 = Fbv_EvalCtlSpec(TR, Ddi_ExprReadSub(e, 0), opt);
          op1 = Fbv_EvalCtlSpec(TR, Ddi_ExprReadSub(e, 1), opt);
          rBdd = Ddi_BddOr(Ddi_ExprToBdd(op0), Ddi_ExprToBdd(op1));
          Ddi_Free(op0);
          Ddi_Free(op1);
          r = Ddi_ExprMakeFromBdd(rBdd);
          Ddi_Free(rBdd);
          break;
        case Expr_Ctl_NOT_c:
          op0 = Fbv_EvalCtlSpec(TR, Ddi_ExprReadSub(e, 0), opt);
          rBdd = Ddi_BddNot(Ddi_ExprToBdd(op0));
          Ddi_Free(op0);
          r = Ddi_ExprMakeFromBdd(rBdd);
          Ddi_Free(rBdd);
          break;
        case Expr_Ctl_IFF_c:
        case Expr_Ctl_EQUAL_c:
          op0 = Fbv_EvalCtlSpec(TR, Ddi_ExprReadSub(e, 0), opt);
          op1 = Fbv_EvalCtlSpec(TR, Ddi_ExprReadSub(e, 1), opt);
          rBdd = Ddi_BddXnor(Ddi_ExprToBdd(op0), Ddi_ExprToBdd(op1));
          Ddi_Free(op0);
          Ddi_Free(op1);
          r = Ddi_ExprMakeFromBdd(rBdd);
          Ddi_Free(rBdd);
          break;
        case Expr_Ctl_IMPLIES_c:
          op0 = Fbv_EvalCtlSpec(TR, Ddi_ExprReadSub(e, 0), opt);
          op1 = Fbv_EvalCtlSpec(TR, Ddi_ExprReadSub(e, 1), opt);
          Ddi_BddNotAcc(Ddi_ExprToBdd(op0));
          rBdd = Ddi_BddOr(Ddi_ExprToBdd(op0), Ddi_ExprToBdd(op1));
          Ddi_Free(op0);
          Ddi_Free(op1);
          r = Ddi_ExprMakeFromBdd(rBdd);
          Ddi_Free(rBdd);
          break;

        case Expr_Ctl_AG_c:
          op0 = Fbv_EvalCtlSpec(TR, Ddi_ExprReadSub(e, 0), opt);
          r = Ddi_ExprCtlMake(ddiMgr, opcode, op0, NULL, NULL);
          Ddi_Free(op0);
          break;

        case Expr_Ctl_AF_c:
          op0 = Fbv_EvalCtlSpec(TR, Ddi_ExprReadSub(e, 0), opt);
          opBdd = Ddi_BddNot(Ddi_ExprToBdd(op0));
          rBdd = FbvEvalEG(TR, opBdd, opt);
          Ddi_Free(opBdd);
          Ddi_BddNotAcc(rBdd);
          /* LOG_BDD(rBdd,"EG"); */
          r = Ddi_ExprMakeFromBdd(rBdd);
          Ddi_Free(rBdd);
          Ddi_Free(op0);
          break;

        case Expr_Ctl_EX_c:
        case Expr_Ctl_AX_c:
        case Expr_Ctl_EF_c:
        case Expr_Ctl_EG_c:
          break;

        case Expr_Ctl_EU_c:
        case Expr_Ctl_AU_c:
        case Expr_Ctl_EBU_c:
        case Expr_Ctl_ABU_c:
        case Expr_Ctl_EBF_c:
        case Expr_Ctl_ABF_c:
        case Expr_Ctl_EBG_c:
        case Expr_Ctl_ABG_c:

        case Expr_Ctl_TWODOTS_c:
        case Expr_Ctl_DOT_c:
        case Expr_Ctl_ARRAY_c:
        case Expr_Ctl_PLUS_c:
        case Expr_Ctl_MINUS_c:
        case Expr_Ctl_TIMES_c:
        case Expr_Ctl_DIVIDE_c:
        case Expr_Ctl_MOD_c:
        case Expr_Ctl_LT_c:
        case Expr_Ctl_GT_c:
        case Expr_Ctl_LE_c:
        case Expr_Ctl_GE_c:
        case Expr_Ctl_UNION_c:
        case Expr_Ctl_SETIN_c:
        case Expr_Ctl_SIGMA_c:
        case Expr_Ctl_TRUEEXP_c:
        case Expr_Ctl_CASE_c:
        case Expr_Ctl_COLON_c:

        case Expr_Ctl_Null_code_c:
          fprintf(stderr, "ERROR: Not yet supported EXPR opcode.\n");
          break;
        default:
          fprintf(stderr, "ERROR: Unknown EXPR opcode.\n");
          break;
      }

      break;
    case Ddi_Expr_String_c:
      {
        Ddi_Bdd_t *l;
        Ddi_Var_t *v;

        v = Ddi_VarFromName(Ddi_ReadMgr(e), Ddi_ExprToString(e));
        if (v == NULL) {
          Ddi_Bdd_t *node =
            (Ddi_Bdd_t *) Ddi_NodeFromName(ddiMgr, Ddi_ExprToString(e));
          if (node != NULL) {
            DdiConsistencyCheck(node, Ddi_Bdd_c);
            l = Ddi_BddDup(node);
          } else {
            Pdtutil_Warning(v == NULL,
              "missing variable while converting expr");
            v = Ddi_VarNew(ddiMgr);
            Ddi_VarAttachName(v, Ddi_ExprToString(e));
            l = Ddi_BddMakeLiteral(v, 1);
          }
        } else {
          l = Ddi_BddMakeLiteral(v, 1);
        }
        r = Ddi_ExprMakeFromBdd(l);
        Ddi_Free(l);
      }
      break;
    default:
      Pdtutil_Warning(1, "Invalid CODE in Fbv_EvalCtlSpec");
      break;
  }

  return (r);

}


/**Function*******************************************************************
  Synopsis    [exact forward verification]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
FbvEvalEG(
  Tr_Tr_t * tr,
  Ddi_Bdd_t * opBdd,
  Fbv_Globals_t * opt
)
{
  int i, fixPoint;
  Ddi_Bdd_t *reached, *newReached, *to;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(opBdd);
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Tr_Tr_t *bckTr;
  long initialTime, currTime;

  initialTime = util_cpu_time();

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "EG |P| = %d ", Ddi_BddSize(opBdd));
    fprintf(stdout, "|Tr|    = %d\n", Ddi_BddSize(Tr_TrBdd(tr)))
    );

  bckTr = Tr_TrDup(tr);
  Tr_MgrSetClustSmoothPi(trMgr, 1);
  Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
  if (opt->trav.sort_for_bck != 0) {
    Tr_TrSortForBck(bckTr);
  } else {
    Tr_TrSortIwls95(bckTr);
  }
  Tr_TrSetClustered(bckTr);

  /* approx forward */

  reached = Ddi_BddDup(opBdd);
  Ddi_BddSetMono(reached);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "-- EG BACKWARD EVAL.\n");
    fprintf(stdout, "|TR|=%d.\n", Ddi_BddSize(Tr_TrBdd(bckTr)))
    );

  /* LOG_BDD(reached,"R"); */

  for (i = 0, fixPoint = 0; (!fixPoint); i++) {

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "-- EG iteration: %d.\n", i + 1));

    to = FbvApproxPreimg(bckTr, reached, reached, NULL, 0, opt);

    newReached = Ddi_BddAnd(reached, to);

    Ddi_BddSetMono(newReached);
    /* LOG_BDD(newReached,"nR"); */

    fixPoint = Ddi_BddEqual(reached, newReached);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "|To|=%d; ", Ddi_BddSize(to));
      fprintf(stdout, "|Reached|=%d.\n", Ddi_BddSize(newReached)));

    Ddi_Free(to);
    Ddi_Free(reached);
    reached = newReached;

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      currTime = util_cpu_time();
      fprintf(stdout, "Live nodes (peak): %u(%u) - Time: %s.\n",
        Ddi_MgrReadLiveNodes(ddiMgr),
        Ddi_MgrReadPeakLiveNodeCount(ddiMgr),
        util_print_time(currTime - initialTime))
      );
  }

  return (reached);
}


/**Function*******************************************************************
  Synopsis    [manual partitioning of TR based on partition file]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
ManualAbstraction(
  Fsm_Mgr_t * fsm,
  char *file
)
{
  Ddi_Mgr_t *ddiMgr = Fsm_MgrReadDdManager(fsm);

  FILE *fp;
  char buf[PDTUTIL_MAX_STR_LEN + 1];
  int i, n;
  Ddi_Bddarray_t *delta;
  Ddi_Vararray_t *pi, *ns, *ps;
  Ddi_Var_t *v;
  int useEnables = 1;

  pi = Fsm_MgrReadVarI(fsm);
  ps = Fsm_MgrReadVarPS(fsm);
  ns = Fsm_MgrReadVarNS(fsm);
  delta = Fsm_MgrReadDeltaBDD(fsm);
  n = Ddi_BddarrayNum(delta);

  if (useEnables) {
    int th = atoi(file);
    Ddi_Bddarray_t *enables = Ddi_FindIte(delta, ps, th);
    Ddi_Vararray_t *cutVars;
    int i, ne = Ddi_BddarrayNum(enables);
    char name[100];
    int chk=1;
    cutVars = Ddi_VararrayAlloc(ddiMgr, ne);
    for (i=0; i<ne; i++) {
      sprintf(name,"PDT_ENABLE_CUT_VAR_%d", i);
      Ddi_Var_t *v = Ddi_VarNewBaig(ddiMgr, name);
      Ddi_VararrayWrite(cutVars,i,v);
    }
    Ddi_Bddarray_t *newD = Ddi_AigarrayInsertCuts (delta, enables, cutVars);

    if (chk) {
      Ddi_Bddarray_t *cD = Ddi_BddarrayCompose(newD,cutVars,enables);
      for (i=0; i<n;i++) {
	Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta,i); 
	Ddi_Bdd_t *nd_i = Ddi_BddarrayRead(cD,i); 
	Pdtutil_Assert(Ddi_BddEqualSat(d_i,nd_i),"error in cuts");
      }
      Ddi_Free(cD);
    }

    Ddi_DataCopy(delta,newD);
    Ddi_VararrayAppend(pi,cutVars);
    Ddi_Free(newD);
    Ddi_Free(cutVars);
    Ddi_Free(enables);
  }

  return;

  fp = fopen(file, "r");
  if (fp == NULL) {
    fprintf(stderr, "Error Reading Abstraction File %s.\n", file);
    return;
  }

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Reading Abstraction File %s\n", file));

  while (fscanf(fp, "%s", buf) == 1) {
    int found = 0;

    if (strstr(buf, "$NS") == NULL) {
      strcat(buf, "$NS");
    }

    v = Ddi_VarFromName(ddiMgr, buf);
    if (v == NULL) {
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
        fprintf(stdout, "Variable not found: %s\n", buf));
    }
    for (i = 0; i < n; i++) {
      if (v == Ddi_VararrayRead(ns, i)) {
        found = 1;
        break;
      }
    }
    if (!found) {
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
        fprintf(stdout, "Variable not found: %s.\n", buf));
    } else {
      Ddi_Var_t *vPs = Ddi_VararrayRead(ps, i);
      Ddi_Bdd_t *d = Ddi_BddarrayExtract(delta, i);

      Ddi_Free(d);
      Ddi_VararrayInsertLast(pi, vPs);
      Ddi_VararrayExtract(ns, i);
      Ddi_VararrayExtract(ps, i);
      n--;
    }
  }

}

/**Function*******************************************************************
  Synopsis    [manual partitioning of TR based on partition file]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
InsertCutLatches(
  Fsm_Mgr_t * fsm,
  int th,
  int strategy
)
{
  Ddi_Mgr_t *ddiMgr = Fsm_MgrReadDdManager(fsm);

  char buf[PDTUTIL_MAX_STR_LEN + 1];
  int i, n;
  Ddi_Bddarray_t *delta;
  Ddi_Vararray_t *pi, *ns, *ps;
  Ddi_Var_t *v;
  int useEnables = strategy & 0x2;
  int useXors = strategy & 0x1;
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsm);
  Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsm);

  pi = Fsm_MgrReadVarI(fsm);
  ps = Fsm_MgrReadVarPS(fsm);
  ns = Fsm_MgrReadVarNS(fsm);
  delta = Fsm_MgrReadDeltaBDD(fsm);
  n = Ddi_BddarrayNum(delta);

  Ddi_Bddarray_t *cuts = Ddi_BddarrayAlloc(ddiMgr, 0);
  if (useEnables) {
    Ddi_Bddarray_t *enables = Ddi_FindIte(delta, ps, th);
    if (enables != NULL) {
      Ddi_BddarrayAppend(cuts,enables);
      Ddi_Free(enables);
    }
  }

  if (useXors) {
    Ddi_Bddarray_t *xors = Ddi_AigarrayFindXors(delta, ps, 0);
    if (xors != NULL) {
      Ddi_BddarrayAppend(cuts,xors);
      Ddi_Free(xors);
    }
  }
  if (Ddi_BddarrayNum (cuts)>0) {
    Ddi_Vararray_t *cutLatchPs, *cutLatchNs;
    int i, nx = Ddi_BddarrayNum(cuts);
    char name[100];
    int chk=1;
    cutLatchPs = Ddi_VararrayAlloc(ddiMgr, nx);
    cutLatchNs = Ddi_VararrayAlloc(ddiMgr, nx);
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
	 fprintf(stdout, "%d cut vars added\n", nx));
    for (i=0; i<nx; i++) {
      sprintf(name,"PDT_XOR_CUT_LATCH_%d$PS", i);
      Ddi_Var_t *v = Ddi_VarFindOrAdd(ddiMgr, name, 1);
      Ddi_VararrayWrite(cutLatchPs,i,v);
      sprintf(name,"PDT_XOR_CUT_LATCH_%d$NS", i);
      v = Ddi_VarFindOrAdd(ddiMgr, name, 1);
      Ddi_VararrayWrite(cutLatchNs,i,v);
    }
    Ddi_Bddarray_t *newD = Ddi_AigarrayInsertCuts (delta, cuts, cutLatchPs);
    if (chk) {
      Ddi_Bddarray_t *cD = Ddi_BddarrayCompose(newD,cutLatchPs,cuts);
      for (i=0; i<n;i++) {
	Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta,i); 
	Ddi_Bdd_t *nd_i = Ddi_BddarrayRead(cD,i); 
	Pdtutil_Assert(Ddi_BddEqualSat(d_i,nd_i),"error in cuts");
      }
      Ddi_Free(cD);
    }
    Ddi_Bddarray_t *extraD = Ddi_BddarrayCompose(cuts,ps,delta);

    Ddi_Bdd_t *prop = NULL;
    Ddi_Bdd_t *constr = NULL;
    Ddi_Bdd_t *stubConstr = NULL;
    Ddi_Bdd_t *stubProp = NULL;
    Ddi_Var_t *vPropPs = NULL;
    Ddi_Var_t *vPropNs = NULL;
    Ddi_Var_t *vConstrPs = NULL;
    Ddi_Var_t *vConstrNs = NULL;

#if 0
    // to be checked - probably not handled through reductions
    int iProp = Fsm_MgrReadIFoldedProp(fsm);
    int iConstr = Fsm_MgrReadIFoldedConstr(fsm);
#else
    int iProp = -1, iConstr = -1;
    if (Fsm_MgrReadIFoldedProp(fsm)) {
      iProp = Ddi_BddarrayNum(delta)-1;
      iConstr = iProp-1;
    }
#endif
    if (iProp >= 0) {
      Pdtutil_Assert(iProp==Ddi_BddarrayNum(newD)-1,"wrong prop folding");
      prop = Ddi_BddarrayExtract(newD,iProp);
      if (initStub!=NULL)
	stubProp = Ddi_BddarrayExtract(initStub,iProp);
      vPropPs = Ddi_VararrayExtract(ps,iProp);
      vPropNs = Ddi_VararrayExtract(ns,iProp);
    }
    if (iConstr >= 0) {
      Pdtutil_Assert(iConstr==Ddi_BddarrayNum(newD)-1,"wrong constr folding");
      constr = Ddi_BddarrayExtract(newD,iConstr);
      if (initStub!=NULL)
	stubConstr = Ddi_BddarrayExtract(initStub,iConstr);
      vConstrPs = Ddi_VararrayExtract(ps,iConstr);
      vConstrNs = Ddi_VararrayExtract(ns,iConstr);
    }
    
    Ddi_BddarrayAppend(newD,extraD);

    if (initStub!=NULL) {
      Ddi_Bddarray_t *extraStub = Ddi_BddarrayCompose(cuts,ps,initStub);
      Ddi_BddarrayAppend(initStub,extraStub);
      Ddi_Free(extraStub);
    }
    else {
      Ddi_Bdd_t *extraInit = Ddi_BddRelMakeFromArray(cuts,cutLatchPs);
      Ddi_BddSetAig(extraInit);
      Ddi_AigConstrainCubeAcc(extraInit, init);
      Ddi_BddAndAcc(init,extraInit);
      Ddi_Free(extraInit);
      //      Pdtutil_Assert(0,"init stub needed for xor cuts");
    }
            
    Ddi_Free(extraD);

    Ddi_DataCopy(delta,newD);
    Ddi_VararrayAppend(ps,cutLatchPs);
    Ddi_VararrayAppend(ns,cutLatchNs);

    if (iConstr >= 0) {
      Ddi_BddarrayInsertLast(delta,constr);
      if (initStub!=NULL)
	Ddi_BddarrayInsertLast(initStub,stubConstr);
      Ddi_VararrayInsertLast(ps,vConstrPs);
      Ddi_VararrayInsertLast(ns,vConstrNs);
    }
    if (iProp >= 0) {
      Ddi_BddarrayInsertLast(delta,prop);
      if (initStub!=NULL)
	Ddi_BddarrayInsertLast(initStub,stubProp);
      Ddi_VararrayInsertLast(ps,vPropPs);
      Ddi_VararrayInsertLast(ns,vPropNs);
    }
    Ddi_Free(prop);
    Ddi_Free(constr);
    Ddi_Free(newD);
    Ddi_Free(cutLatchPs);
    Ddi_Free(cutLatchNs);
  }
  Ddi_Free(cuts);

  return;


}

/**Function*******************************************************************
  Synopsis    [manual partitioning of TR based on partition file]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
ManualConstrain(
  Fsm_Mgr_t * fsm,
  char *file
)
{
  Ddi_Mgr_t *ddiMgr = Fsm_MgrReadDdManager(fsm);

  FILE *fp;
  char buf[PDTUTIL_MAX_STR_LEN + 1], *vname;
  int i, n;
  Ddi_Var_t *v;
  Ddi_Bdd_t *myConstr = Fsm_MgrReadConstraintBDD(fsm);

  fp = fopen(file, "r");
  if (fp == NULL) {
    fprintf(stderr, "Error Reading Abstraction File %s.\n", file);
    return;
  }

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Reading Abstraction File %s\n", file));

  while (fscanf(fp, "%s", buf) == 1) {
    int found = 0, isNeg = 0;
    Ddi_Bdd_t *lit;

    vname = buf;
    if (*vname == '!') {
      vname++;
      isNeg = 1;
    }
    v = Ddi_VarFromName(ddiMgr, vname);
    if (v == NULL) {
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
        fprintf(stdout, "Variable not found: %s\n", buf));
    }
    lit = Ddi_BddMakeLiteral(v, !isNeg);
    Ddi_BddAndAcc(myConstr, lit);
    Ddi_Free(lit);
  }

}

/**Function*******************************************************************
  Synopsis    [manual partitioning of TR based on partition file]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bddarray_t *
ManualHints(
  Ddi_Mgr_t *ddiMgr,
  char *file
)
{

  FILE *fp;
  char buf[PDTUTIL_MAX_STR_LEN + 1], *vname;
  int i, n;
  Ddi_Var_t *v;
  Ddi_Bddarray_t *myHint = Ddi_BddarrayAlloc(ddiMgr, 0);

  fp = fopen(file, "r");
  if (fp == NULL) {
    fprintf(stderr, "Error Reading Hints File %s.\n", file);
    Ddi_Free(myHint);
    return NULL;
  }

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Reading Hints File %s\n", file));

  while (fscanf(fp, "%s", buf) == 1) {
    int found = 0, isNeg = 0;
    Ddi_Bdd_t *lit;

    vname = buf;
    if (*vname == '!') {
      vname++;
      isNeg = 1;
    }
    v = Ddi_VarFromName(ddiMgr, vname);
    if (v == NULL) {
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
        fprintf(stdout, "Variable not found: %s\n", buf));
    }
    lit = Ddi_BddMakeLiteralAig(v, !isNeg);
    Ddi_BddarrayInsertLast(myHint, lit);
    Ddi_Free(lit);
  }

  return myHint;
}

/**Function*******************************************************************
  Synopsis    [manual partitioning of TR based on partition file]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Tr_Tr_t *
ManualTrDisjPartitions(
  Tr_Tr_t * tr,
  char *file,
  Ddi_Bdd_t * invar,
  Fbv_Globals_t * opt
)
{
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Ddi_Bdd_t *trBdd = Ddi_BddDup(Tr_TrBdd(tr)), *trBdd2;
  Ddi_Bdd_t *trBddNoPart = Ddi_BddDup(Tr_TrBdd(tr));
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(trBdd);
  Tr_Tr_t *tr2;
  FILE *fp;
  char buf[PDTUTIL_MAX_STR_LEN + 1];
  char vname[PDTUTIL_MAX_STR_LEN + 1];
  int i, nb, ib, nv, maxb = 100, again;
  typedef struct {
    int pcId, varId, primary;
    int nSb, maxSb;
    int *subBlocks;
    Ddi_Bdd_t *window;
    Ddi_Bdd_t *trBdd;
  } block_t;

  block_t *blocks;
  Ddi_Bdd_t *part2, *noChange;

  Ddi_Vararray_t *ps = Tr_MgrReadPS(Tr_TrMgr(tr));
  Ddi_Vararray_t *ns = Tr_MgrReadNS(Tr_TrMgr(tr));
  Ddi_Vararray_t *zv;

  if (!Ddi_BddIsPartConj(trBdd)) {
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "NO conj part TR for manual partitioning\n"));
    Ddi_Free(trBdd);
    return (Tr_TrDup(tr));
  }

  if (opt->trav.squaring > 0) {
    Tr_MgrSetMaxIter(trMgr, opt->tr.squaringMaxIter);
  }

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Reading partitions file %s\n", file));

  trBdd2 = Ddi_BddMakePartDisjVoid(ddiMgr);

  fp = fopen(file, "r");
  if (fp == NULL) {
    Ddi_Free(trBdd);
    Ddi_Free(trBdd2);
    return (Tr_TrDup(tr));
  }

  zv = Ddi_VararrayAlloc(ddiMgr, Ddi_VararrayNum(ps));

  for (i = 0; i < Ddi_VararrayNum(ps); i++) {
    int xlevel = Ddi_VarCurrPos(Ddi_VararrayRead(ps, i)) + 1;
    Ddi_Var_t *var = Ddi_VarNewAtLevel(ddiMgr, xlevel);

    Ddi_VararrayWrite(zv, i, var);
    sprintf(buf, "%s$CLUSURE$Z", Ddi_VarName(Ddi_VararrayRead(ps, i)));
    Ddi_VarAttachName(var, buf);
  }


  part2 = NULL;
  blocks = Pdtutil_Alloc(block_t, maxb);
  nb = 0;


  again = fscanf(fp, "%s", buf) == 1;
  if (strcmp(buf, ".idata") == 0) {
    int nbits, i, j;

    fscanf(fp, "%s %d %d", buf, &nb, &nbits);
    Pdtutil_Free(blocks);
    blocks = Pdtutil_Alloc(block_t, nb);
    Pdtutil_Assert(nbits <= 16, "too many bits in part");
    for (i = 0; i < nb; i++) {
      int k = i;

      blocks[i].primary = 1;
      blocks[i].pcId = 0;
      blocks[i].maxSb = 0;
      blocks[i].nSb = 0;
      blocks[i].subBlocks = NULL;
      blocks[i].varId = 0;
      blocks[i].trBdd = NULL;
      blocks[i].window = Ddi_BddMakeConst(ddiMgr, 1);
      for (j = 0; j < nbits; j++, k /= 2) {
        Ddi_Bdd_t *lit;

        sprintf(vname, "%s<%d>0", buf, j);
        lit = Ddi_BddMakeLiteral(Ddi_VarFromName(ddiMgr, vname), 0);
        if (k % 2) {
          /* bit is one */
          Ddi_BddNotAcc(lit);
        }
        Ddi_BddAndAcc(blocks[i].window, lit);
      }
    }
  } else
    while (again) {
      again = 0;
      if (buf[0] == '.') {
        sscanf(buf, ".block%d", &ib);
        fscanf(fp, "%s", buf);
        fscanf(fp, "%d", &nv);
        if (ib >= nb) {
          nb = ib + 1;
        }
        if (nb > maxb) {
          maxb *= 2;
          blocks = Pdtutil_Realloc(block_t, blocks, maxb);
        }
        blocks[ib].primary = 1;
        blocks[ib].pcId = nv;
        blocks[ib].maxSb = blocks[ib].nSb = 0;
        blocks[ib].subBlocks = NULL;
        blocks[ib].varId = nv;
        blocks[ib].window = NULL;
        blocks[ib].trBdd = NULL;
        if (strcmp(buf, "stmt") == 0) {
          fscanf(fp, "%s", buf);
        } else {
          int readAgain = 1, sb;

          blocks[ib].maxSb = 1;
          blocks[ib].subBlocks = Pdtutil_Alloc(int,
            blocks[ib].maxSb
          );

          do {
            readAgain = fscanf(fp, "%s", buf) == 1;
            readAgain = readAgain && (buf[0] != '.');
            sscanf(buf, "block%d", &sb);
            if (readAgain) {
              if (blocks[ib].nSb >= blocks[ib].maxSb) {
                blocks[ib].maxSb *= 2;
                blocks[ib].subBlocks = Pdtutil_Realloc(int,
                  blocks[ib].subBlocks,
                  blocks[ib].maxSb
                );
              }
              blocks[ib].subBlocks[blocks[ib].nSb] = sb;
              blocks[ib].nSb++;
            }
          } while (readAgain);
        }
        again = buf[0] == '.';
      }
    }

  do {
    again = 0;
    for (i = 0; i < nb; i++) {
      if (blocks[i].nSb == 0 && blocks[i].trBdd == NULL) {
        Ddi_Bdd_t *window;

        if (blocks[i].window != NULL) {
          window = Ddi_BddDup(blocks[i].window);
        } else {
          sprintf(vname, "pc<%d>0", blocks[i].varId);
          window = Ddi_BddMakeLiteral(Ddi_VarFromName(ddiMgr, vname), 1);
        }
        blocks[i].trBdd = Ddi_BddConstrain(trBdd, window);
        noChange = trExtractNochangeParts(blocks[i].trBdd, window,
          Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
        Ddi_BddAndAcc(blocks[i].trBdd, window);
        if (invar != NULL) {
          Ddi_BddConstrainAcc(blocks[i].trBdd, invar);
          //        Ddi_BddAndAcc(blocks[i].trBdd,invar);
        }
        if (opt->trav.squaring > 0) {
          Ddi_BddSetMono(blocks[i].trBdd);
          TrBuildTransClosure(trMgr, blocks[i].trBdd, ps, ns, zv);
        }
        Ddi_BddSetPartConj(blocks[i].trBdd);
        Ddi_BddPartInsertLast(blocks[i].trBdd, noChange);
        Ddi_Free(noChange);
        Ddi_BddNotAcc(window);
        Ddi_BddConstrainAcc(trBddNoPart, window);
        Ddi_BddAndAcc(trBddNoPart, window);
        Ddi_Free(window);
      } else if (blocks[i].nSb > 0) {
        int j, incomplete = 0, np = blocks[i].nSb;

        for (j = 0; j < np; j++) {
          int b = blocks[i].subBlocks[j];

          blocks[b].primary = 0;
          if (blocks[b].trBdd == NULL) {
            incomplete = 1;
          }
        }
        if (incomplete) {
          again = 1;
        } else if (opt->trav.squaring) {
          Ddi_Bdd_t *tr_i = blocks[i].trBdd = Ddi_BddMakePartConjVoid(ddiMgr);
          Ddi_Bdd_t *tChange;

          for (j = 0; j < np; j++) {
            int b = blocks[i].subBlocks[j];
            Ddi_Bdd_t *part = blocks[b].trBdd;

            if (j == 0) {
              Pdtutil_Assert(Ddi_BddPartNum(part) == 2,
                "2 partitions required");
              tChange = Ddi_BddDup(Ddi_BddPartRead(part, 0));
              noChange = Ddi_BddDup(Ddi_BddPartRead(part, 1));
            } else {
              Ddi_Bdd_t *c2 = Ddi_BddPartRead(part, 0);
              Ddi_Bdd_t *nc2 = Ddi_BddPartRead(part, 1);
              Ddi_Varset_t *vc1 = Ddi_BddSupp(tChange);
              Ddi_Varset_t *vc2 = Ddi_BddSupp(c2);
              Ddi_Varset_t *vnc1 = Ddi_BddSupp(noChange);
              Ddi_Varset_t *vnc2 = Ddi_BddSupp(nc2);
              Ddi_Varset_t *common = Ddi_VarsetUnion(vc1, vc2);
              Ddi_Bdd_t *nc1b = Ddi_BddExistProject(noChange, common);
              Ddi_Bdd_t *nc2b = Ddi_BddExistProject(nc2, common);

              Ddi_BddAndAcc(tChange, nc1b);
              Ddi_BddAndAcc(c2, nc2b);
              Ddi_BddAndAcc(noChange, nc2);
              Ddi_BddExistAcc(noChange, common);

              Ddi_Free(c2);
              Ddi_Free(nc2);
              Ddi_Free(vc1);
              Ddi_Free(vc2);
              Ddi_Free(vnc1);
              Ddi_Free(vnc2);
              Ddi_Free(common);
              Ddi_Free(nc1b);
              Ddi_Free(nc2b);

              TrBuildTransClosure(trMgr, tChange, ps, ns, zv);
            }
          }
          Ddi_BddPartInsertLast(tr_i, tChange);
          Ddi_BddPartInsertLast(tr_i, noChange);
          Ddi_Free(tChange);
          Ddi_Free(noChange);
        } else {
          Ddi_Bdd_t *tr_i = blocks[i].trBdd = Ddi_BddMakePartDisjVoid(ddiMgr);

          for (j = 0; j < np; j++) {
            int b = blocks[i].subBlocks[j];
            Ddi_Bdd_t *part = blocks[b].trBdd;

            Ddi_BddPartInsertLast(tr_i, part);
          }
        }
      }
    }
  } while (again);

  Ddi_BddPartInsertLast(trBdd2, trBddNoPart);

  for (i = 0; i < nb; i++) {
    if (blocks[i].primary) {
      Ddi_BddPartInsertLast(trBdd2, blocks[i].trBdd);
    }
    Ddi_Free(blocks[i].trBdd);
    Ddi_Free(blocks[i].window);
    Pdtutil_Free(blocks[i].subBlocks);
  }

  Pdtutil_Free(blocks);

  Ddi_Free(trBddNoPart);
  Ddi_Free(trBdd);
  Ddi_Free(zv);

  Ddi_BddSetFlattened(trBdd2);
  tr2 = Tr_TrMakeFromRel(trMgr, trBdd2);
  Ddi_Free(trBdd2);

  return (tr2);

}


/**Function*******************************************************************
  Synopsis    [manual partitioning of TR based on partition file]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
trExtractNochangeParts(
  Ddi_Bdd_t * trBdd,
  Ddi_Bdd_t * w,
  Ddi_Vararray_t * psv,
  Ddi_Vararray_t * nsv
)
{
  int i;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(trBdd);
  Ddi_Varset_t *nsVarset = Ddi_VarsetMakeFromArray(nsv);
  Ddi_Bdd_t *eqParts = Ddi_BddMakePartConjVoid(ddiMgr);

  Pdtutil_Assert(Ddi_BddIsPartConj(trBdd), "Wrong part TR");

  for (i = 0; i < Ddi_BddPartNum(trBdd); i++) {
    Ddi_BddSuppAttach(Ddi_BddPartRead(trBdd, i));
  }

  for (i = 0; i < Ddi_VararrayNum(psv); i++) {
    Ddi_Bdd_t *litp = Ddi_BddMakeLiteral(Ddi_VararrayRead(psv, i), 1);
    Ddi_Bdd_t *litn = Ddi_BddMakeLiteral(Ddi_VararrayRead(nsv, i), 1);
    Ddi_Bdd_t *diff = Ddi_BddXor(litp, litn);
    int j, eq = 0;

    Ddi_Free(litp);
    Ddi_Free(litn);
    for (j = Ddi_BddPartNum(trBdd) - 1; j >= 0; j--) {
      Ddi_Bdd_t *p_j = Ddi_BddPartRead(trBdd, j);

      if (Ddi_VarInVarset(Ddi_BddSuppRead(p_j), Ddi_VararrayRead(nsv, i))) {
        Ddi_Bdd_t *aux = Ddi_BddAnd(Ddi_BddPartRead(trBdd, j), diff);

        if (Ddi_BddIsZero(aux)) {
          Ddi_Varset_t *y_j =
            Ddi_VarsetIntersect(Ddi_BddSuppRead(p_j), nsVarset);
          if (Ddi_VarsetNum(y_j) == 1) {
            Ddi_BddPartInsertLast(eqParts, p_j);
            Ddi_BddPartRemove(trBdd, j);
          }
          Ddi_Free(y_j);
          Ddi_Free(aux);
          eq = 1;
          break;
        }
        Ddi_Free(aux);
      }
    }

    Ddi_Free(diff);
  }
  Ddi_Free(nsVarset);
  if (Ddi_BddPartNum(eqParts) == 0) {
    Ddi_Free(eqParts);
    eqParts = Ddi_BddMakeConst(ddiMgr, 1);
  }

  return (eqParts);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Tr_Tr_t *
trCoiReduction(
  Tr_Tr_t * tr,
  Ddi_Varset_t * coiVars,
  int maxIter
)
{
  Ddi_Varset_t *nocoi, *nocoiNS;
  Ddi_Bdd_t *reducedTRBdd;
  Tr_Tr_t *reducedTr;
  Ddi_Vararray_t *ps = Tr_MgrReadPS(Tr_TrMgr(tr));
  Ddi_Vararray_t *ns = Tr_MgrReadNS(Tr_TrMgr(tr));

  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(coiVars);

  nocoi = Ddi_VarsetMakeFromArray(ps);
  Ddi_VarsetDiffAcc(nocoi, coiVars);
  nocoiNS = Ddi_VarsetSwapVars(nocoi, ps, ns);
#if 1
  Ddi_VarsetUnionAcc(nocoi, nocoiNS);
#else
  Ddi_Free(nocoi);
  nocoi = Ddi_VarsetDup(nocoiNS);

  Ddi_Free(nocoi);
  nocoi = Ddi_VarsetVoid(ddiMgr);
#endif
  Ddi_Free(nocoiNS);

  reducedTRBdd = Part_BddMultiwayLinearAndExist(Tr_TrBdd(tr), nocoi, 1);

  Ddi_Free(nocoi);

#if 0
  nocoi = Ddi_BddSupp(reducedTRBdd);
  opt->trav.univQuantifyVars = Ddi_VarsetMakeFromArray(ps);
  Ddi_VarsetIntersectAcc(opt->trav.univQuantifyVars, nocoi);
  Ddi_Free(nocoi);
  Ddi_VarsetDiffAcc(opt->trav.univQuantifyVars, coiVars);
#endif

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Original TR: Size=%d; #Partitions=%d.\n",
      Ddi_BddSize(Tr_TrBdd(tr)), Ddi_BddPartNum(Tr_TrBdd(tr)))
    );

  reducedTr = Tr_TrMakeFromRel(Tr_TrMgr(tr), reducedTRBdd);

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Reduced TR: Size=%d; #Partitions=%d\n",
      Ddi_BddSize(reducedTRBdd), Ddi_BddPartNum(reducedTRBdd)));

  Ddi_Free(reducedTRBdd);

  return (reducedTr);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static void
fsmCoiReduction(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Varset_t * coiVars
)
{
  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Varset_t *dsupp, *stubsupp;
  Ddi_Vararray_t *psRemoved, *dSuppArray, *varsNew;
  int i, n;
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(delta);
  Ddi_Vararray_t *coiVarsA;

  n = Ddi_BddarrayNum(delta);

  Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Original Delta: Size=%d; #Partitions=%d.\n",
      Ddi_BddarraySize(delta), n)
    );

  psRemoved = Ddi_VararrayAlloc(ddiMgr, 0);
  coiVarsA = Ddi_VararrayMakeFromVarset(coiVars, 1);

  Ddi_VararrayWriteMark(coiVarsA, 1);

  for (i = n - 1; i >= 0; i--) {
    Ddi_Var_t *pv_i = Ddi_VararrayRead(ps, i);
    int isConstr = Ddi_VarName(pv_i) != NULL &&
      (strcmp(Ddi_VarName(pv_i), "PDT_BDD_INVAR_VAR$PS") == 0);
    if (Ddi_VarReadMark(pv_i) == 0 && !isConstr) {
      Ddi_VararrayWrite(ps, i, NULL);
      Ddi_VararrayWrite(ns, i, NULL);
      Ddi_BddarrayWrite(delta, i, NULL);
      Ddi_VararrayInsertLast(psRemoved, pv_i);
      if (initStub != NULL) {
        Ddi_BddarrayWrite(initStub, i, NULL);
      }
    }
  }
  Ddi_VararrayRemoveNull(ps);
  Ddi_VararrayRemoveNull(ns);
  Ddi_BddarrayRemoveNull(delta);
  if (initStub != NULL) {
    Ddi_BddarrayRemoveNull(initStub);
  }

  Ddi_VararrayWriteMark(coiVarsA, 0);
  Ddi_Free(coiVarsA);

  n = Ddi_BddarrayNum(delta);

  if (initStub == NULL) {
    Ddi_Varset_t *proj = Ddi_VarsetMakeFromArray(ps);

    if (Ddi_BddIsMono(init)) {
      Ddi_BddExistProjectAcc(init, proj);
    } else {
      Ddi_AigCubeExistProjectAcc(init, proj);
    }
    Ddi_Free(proj);
  }

  Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Reduced Delta: Size=%d; #Partitions=%d.\n",
      Ddi_BddarraySize(delta), n));

  dsupp = Ddi_BddarraySupp(delta);
  if (initStub != NULL) {
    Ddi_VarsetSetArray(dsupp);
    stubsupp = Ddi_BddarraySupp(initStub);
    Ddi_VarsetSetArray(stubsupp);
    Ddi_VarsetUnionAcc(dsupp, stubsupp);
    Ddi_Free(stubsupp);
  }

  dSuppArray = Ddi_VararrayMakeFromVarset(dsupp, 1);
  Ddi_VararrayWriteMark(dSuppArray, 1);

  for (i = 0; i < Ddi_VararrayNum(pi); i++) {
    Ddi_Var_t *pi_i = Ddi_VararrayRead(pi, i);

    if (Ddi_VarReadMark(pi_i) == 0) {
      /* not in dsupp */
      Ddi_VararrayWrite(pi, i, NULL);
    }
  }

  Ddi_VararrayWriteMark(dSuppArray, 0);
  Ddi_VararrayRemoveNull(pi);

  Ddi_VararrayWriteMark(psRemoved, 1);

  for (i = 0; i < Ddi_VararrayNum(ps); i++) {
    Ddi_Var_t *ps_i = Ddi_VararrayRead(ps, i);

    if (Ddi_VarReadMark(ps_i) == 1) {
      /* in removed */
      Ddi_VararrayWrite(ps, i, NULL);
      Ddi_VararrayInsertLast(pi, ps_i);
    }
  }

  Ddi_VararrayWriteMark(psRemoved, 0);
  Ddi_VararrayRemoveNull(ps);

  Ddi_Free(dSuppArray);
  Ddi_Free(dsupp);
  Ddi_Free(psRemoved);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static int
fsmConstReduction(
  Fsm_Mgr_t * fsmMgr,
  int enTernary,
  int timeLimit,
  Fbv_Globals_t * opt,
  int latchOnly
)
{
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Bddarray_t *delta, *lambda, *initStub;
  Ddi_Bdd_t *init;
  Ddi_Mgr_t *ddiMgr;
  Ddi_Varset_t *dsupp, *stubsupp;
  int i, n, nconst = 0, neq = 0;
  int *sizeA, *enEq, *suppSize;
  Ddi_Bddarray_t *substF;
  Ddi_Vararray_t *substV;
  Ddi_Bdd_t *reached;
  int proved = 0;
  unsigned long int time_limit = ~0;
  long initTime, endTime;
  int addStub;
  int strongRun = 0;
  Ddi_Varset_t **suppDelta = NULL;

  Fsm_FsmStructReduction(fsmMgr, opt->pre.clkVarFound);
  if (opt->pre.ternarySim > 1) {
    int ret = fsmTsimReduction(fsmMgr, enTernary, timeLimit, opt);
    int k;

    if (opt->pre.ternarySim > 3) {
      ret += fsmTsimReduction(fsmMgr, opt->pre.ternarySim, timeLimit, opt);
    }

    for (k = 0; k <= opt->pre.indEqDepth; k++) {
      ret += fsmEqReduction(fsmMgr, timeLimit, opt, k, latchOnly);
    }

    return ret;
  }

  pi = Fsm_MgrReadVarI(fsmMgr);
  ps = Fsm_MgrReadVarPS(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  init = Fsm_MgrReadInitBDD(fsmMgr);
  ddiMgr = Ddi_ReadMgr(lambda);

  if (timeLimit > 0) {
    /* actually NOT used */
    time_limit = util_cpu_time() + timeLimit * 1000;
  }

  n = Ddi_BddarrayNum(delta);

  Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Original Delta: Size=%d; #Partitions=%d.\n",
      Ddi_BddarraySize(delta), n));

  /* ternary fix overappr simulation */

  if (enTernary) {
    int j, fixPoint;
    int nState = Ddi_VararrayNum(ns);

    //    Ddi_Bdd_t *myTr = Ddi_BddRelMakeFromArray(delta,ns);
    Ddi_Bdd_t *from;
    Ddi_Varset_t *ternaryVars;
    Ddi_Varset_t *nsv = Ddi_VarsetMakeFromArray(ns);
    Ddi_Varset_t *deltaSupp = Ddi_BddarraySupp(delta);
    Ddi_Vararray_t *deltaVars = Ddi_VararrayMakeFromVarset(deltaSupp, 1);

    Ddi_Free(deltaSupp);

    //    Ddi_BddSetAig(myTr);
    if (initStub != NULL) {
      from = Ddi_BddRelMakeFromArray(initStub, ns);
      Ddi_BddSetAig(from);
      ternaryVars = Ddi_BddSupp(from);
      Ddi_VarsetDiffAcc(ternaryVars, nsv);
      DdiAigExistOverAcc(from, ternaryVars, NULL);
      Ddi_Free(ternaryVars);
      Ddi_BddSwapVarsAcc(from, ps, ns);
    } else {
      from = Ddi_BddDup(init);
    }
    reached = Ddi_BddDup(from);
    Ddi_BddSetMono(reached);
    fixPoint = 0;
    initTime = util_cpu_time();

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "Ternary Simulation: "));

    for (j = 0; !fixPoint; j++) {
      Ddi_Bdd_t *to;

      Ddi_BddSetAig(from);
      to = Ddi_AigTernaryArrayImg(from, delta, ps, deltaVars);
      Ddi_BddSetMono(to);
      // logBdd(to);
      fixPoint = Ddi_BddIncluded(to, reached);
      Ddi_Free(from);
      from = Ddi_BddDup(to);
      Ddi_BddSetMono(to);

      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
        fprintf(stdout, "(%d)", Ddi_BddSize(to)));

      Ddi_BddOrAcc(reached, to);
      Ddi_Free(to);
    }
    Ddi_Free(from);
    Ddi_Free(nsv);
    //    Ddi_Free(myTr);
    Ddi_Free(deltaVars);
    endTime = util_cpu_time() - initTime;

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "\n")
      );

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "Ternary Simulation Performed: ");
      fprintf(stdout, "%d iterations in %.2f s; |R|=%d.\n",
        j, endTime / 1000.0, Ddi_BddSize(reached))
      );

    for (i = n - 1; i >= 0; i--) {
      Ddi_Var_t *pv_i = Ddi_VararrayRead(ps, i);

      if (strcmp(Ddi_VarName(pv_i), "PDT_BDD_INVARSPEC_VAR$PS") == 0) {
        Ddi_Bdd_t *chk = Ddi_BddCofactor(reached, pv_i, 0);

        if (Ddi_BddIsZero(chk)) {
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
            fprintf(stdout,
              "Verification Result: PASS (by ternary simulation).\n"));
          proved = 1;
        }
        Ddi_Free(chk);
        break;
      }
    }

  } else {
    reached = Ddi_BddMakeConstAig(ddiMgr, 1);
  }

  if (proved) {
    Ddi_Free(reached);
    return -1;
  }

  if (0 && (Ddi_BddSize(reached) > 2000)) {
    Ddi_Free(reached);
    reached = Ddi_BddMakeConstAig(ddiMgr, 1);
  }
  Ddi_BddSetAig(reached);

  do {
    addStub = 0;
    n = Ddi_BddarrayNum(delta);
    for (i = n - 1; i >= 0; i--) {
      Ddi_Var_t *pv_i = Ddi_VararrayRead(ps, i);
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);
      Ddi_Bdd_t *d_i_bar = Ddi_BddNot(d_i);
      int isOne = !Ddi_AigSatAnd(d_i_bar, reached, NULL);   /* @ */
      int isZero = !isOne && !Ddi_AigSatAnd(d_i, reached, NULL);
      int isConst = isOne || isZero;

      Ddi_Free(d_i_bar);
      if (!Ddi_BddIsConstant(d_i) && isConst && pv_i != opt->pre.clkVarFound &&
        strncmp(Ddi_VarName(pv_i), "PDT_BDD_", 8) != 0) {
        int removeVar = 0;
        Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(pv_i, isOne);

        if ((initStub == NULL) && Ddi_AigSatAnd(init, lit, NULL)) {
          removeVar = 1;
        } else if (initStub != NULL) {
          Ddi_Bdd_t *stub_i = Ddi_BddDup(Ddi_BddarrayRead(initStub, i));

          if (isOne)
            Ddi_BddNotAcc(stub_i);
          if (!Ddi_AigSat(stub_i)) {
            /* init stub confirms constant value: reduce */
            removeVar = 1;
            Pdtutil_VerbosityMgrIf(ddiMgr, Pdtutil_VerbLevelDevMed_c) {
              fprintf(stdout, "Remove var %s\n", Ddi_VarName(pv_i));
            }
            Ddi_BddarrayRemove(initStub, i);
          } else if (1) {
            addStub = 1;
          }
          Ddi_Free(stub_i);
        }
        if (removeVar) {
#if 1
          if (initStub == NULL) {
            Ddi_BddCofactorAcc(init, pv_i, isOne);
          }
          Ddi_BddarrayRemove(delta, i);
          Ddi_VararrayRemove(ps, i);
          Ddi_VararrayRemove(ns, i);
          Ddi_AigarrayConstrainCubeAcc(delta, lit);
          Ddi_AigarrayConstrainCubeAcc(lambda, lit);
#else
          Ddi_BddCofactorAcc(d_i, pv_i, isOne);
#endif
          nconst++;
        }
        Ddi_Free(lit);
      }
    }
    if (addStub) {
      char suffix[4];
      Ddi_Vararray_t *newPiVars;
      Ddi_Bddarray_t *newPiLits;
      Ddi_Bddarray_t *myInitStub = Ddi_BddarrayDup(delta);

      sprintf(suffix, "%d", fsmMgr->stats.initStubSteps++);
      newPiVars = Ddi_VararrayMakeNewVars(pi, "PDT_STUB_PI", suffix, 1);
      newPiLits = Ddi_BddarrayMakeLiteralsAig(newPiVars, 1);
      Ddi_BddarrayComposeAcc(myInitStub, pi, newPiLits);
      if (Fsm_MgrReadInitStubBDD(fsmMgr) != NULL) {
        Ddi_BddarrayComposeAcc(myInitStub, ps, Fsm_MgrReadInitStubBDD(fsmMgr));
      }
      Ddi_Free(newPiVars);
      Ddi_Free(newPiLits);
      Fsm_MgrSetInitStubBDD(fsmMgr, myInitStub);
      Ddi_Free(myInitStub);
      initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
      //      Fsm_MgrIncrInitStubSteps (fsmMgr,1);
      Pdtutil_WresIncrInitStubSteps(1);
    }
  } while (addStub);

  Ddi_Free(reached);

  n = Ddi_BddarrayNum(delta);

  if (nconst > 0) {
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "Found %d Constants; ", nconst);
      fprintf(stdout, "Reduced Delta: Size=%d; #Partitions=%d.\n",
        Ddi_BddarraySize(delta), n));
  }

  sizeA = Pdtutil_Alloc(int,
    n
  );
  enEq = Pdtutil_Alloc(int,
    n
  );

  for (i = 0; i < n; i++) {
    Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);

    sizeA[i] = Ddi_BddSize(d_i);
    enEq[i] = 1;

    if (0 && sizeA[i] == 1) {
      Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);
      Ddi_Var_t *v = Ddi_BddTopVar(d_i);

      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
        fprintf(stdout, "Literal delta[%d]: %s <- %s%s.\n", i,
          Ddi_VarName(v_i), Ddi_BddIsComplement(d_i) ? "!" : "",
          Ddi_VarName(v)));
    }
  }

  substV = Ddi_VararrayAlloc(ddiMgr, 0);
  substF = Ddi_BddarrayAlloc(ddiMgr, 0);

  if (strongRun) {
    suppDelta = Ddi_BddarraySuppArray(delta);
    suppSize = Pdtutil_Alloc(int,
      n
    );

    for (i = 0; i < n; i++) {
      suppSize[i] = Ddi_VarsetNum(suppDelta[i]);
    }
  }

  for (i = 0; i < n; i++) {
    if (sizeA[i] <= 100 && enEq[i]) {
      int j;
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);
      Ddi_Bdd_t *d_i_bar = NULL;
      Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);
      Ddi_Varset_t *s_i = strongRun ? suppDelta[i] : NULL;

      if (strncmp(Ddi_VarName(v_i), "PDT_BDD_", 8) == 0)
        continue;
      d_i_bar = Ddi_BddNot(d_i);
      for (j = i + 1; j < n; j++) {
        Ddi_Varset_t *s_j = strongRun ? suppDelta[i] : NULL;

        if ((!strongRun || suppSize[j] == suppSize[i]) &&
          (strongRun || (sizeA[j] == sizeA[i])) && enEq[j]) {
          int equal = 0;
          Ddi_Bdd_t *d_j = Ddi_BddarrayRead(delta, j);
          Ddi_Var_t *v_j = Ddi_VararrayRead(ps, j);

          if (strncmp(Ddi_VarName(v_j), "PDT_BDD_", 8) == 0)
            continue;
          if (strongRun ? Ddi_BddEqualSat(d_i, d_j) : Ddi_BddEqual(d_i, d_j)) {
            /* equal deltas found ! Check init state */
            if (initStub != NULL) {
              equal = strongRun ?
                Ddi_BddEqualSat(Ddi_BddarrayRead(initStub, i),
                Ddi_BddarrayRead(initStub,
                  j)) : Ddi_BddEqual(Ddi_BddarrayRead(initStub, i),
                Ddi_BddarrayRead(initStub, j));
            } else {
              Ddi_Bdd_t *eq = Ddi_BddMakeLiteralAig(v_i, 1);
              Ddi_Bdd_t *l_j = Ddi_BddMakeLiteralAig(v_j, 1);

              Ddi_BddXnorAcc(eq, l_j);
              Ddi_Free(l_j);
              equal = Ddi_BddIncluded(init, eq);
              Ddi_Free(eq);
            }
            if (equal) {
              Ddi_Bdd_t *l_i = Ddi_BddMakeLiteralAig(v_i, 1);

              Ddi_BddarrayInsertLast(substF, l_i);
              Ddi_VararrayInsertLast(substV, v_j);
              Ddi_Free(l_i);
              enEq[j] = 0;
              neq++;

              Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
                fprintf(stdout, "SEQ equiv found(j:%d,i:%d): %s -> %s.\n", j,
                  i, Ddi_VarName(v_j), Ddi_VarName(v_i)));
            }
          } else if (strongRun ? Ddi_BddEqualSat(d_i_bar, d_j) :
            Ddi_BddEqual(d_i_bar, d_j)) {
            /* equal deltas found ! Check init state */
            if (initStub != NULL) {
              Ddi_Bdd_t *s_j = Ddi_BddNot(Ddi_BddarrayRead(initStub, j));

              equal = strongRun ?
                Ddi_BddEqualSat(Ddi_BddarrayRead(initStub, i), s_j) :
                Ddi_BddEqual(Ddi_BddarrayRead(initStub, i), s_j);
              Ddi_Free(s_j);
            } else {
              Ddi_Bdd_t *eq = Ddi_BddMakeLiteralAig(v_i, 0);
              Ddi_Bdd_t *l_j = Ddi_BddMakeLiteralAig(v_j, 1);

              Ddi_BddXnorAcc(eq, l_j);
              Ddi_Free(l_j);
              equal = Ddi_BddIncluded(init, eq);
              Ddi_Free(eq);
            }
            if (equal) {
              Ddi_Bdd_t *l_i = Ddi_BddMakeLiteralAig(v_i, 0);

              Ddi_BddarrayInsertLast(substF, l_i);
              Ddi_VararrayInsertLast(substV, v_j);
              Ddi_Free(l_i);
              enEq[j] = 0;
              neq++;

              Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
                fprintf(stdout, "SEQ equiv found: %s -> !%s\n",
                  Ddi_VarName(v_j), Ddi_VarName(v_i)));
            }
          }
        }
      }
      Ddi_Free(d_i_bar);
    }
    if (strongRun) {
      Ddi_Free(suppDelta[i]);
    }
  }
  if (strongRun) {
    Pdtutil_Free(suppDelta);
    Pdtutil_Free(suppSize);
  }

  if (neq > 0) {
    for (i = n - 1; i >= 0; i--) {
      if (!enEq[i]) {
        if (initStub != NULL) {
          Ddi_BddarrayRemove(initStub, i);
        } else {
          Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);
          Ddi_Bdd_t *l_i = Ddi_BddMakeLiteralAig(v_i, 1);
          int pos = Ddi_BddIncluded(init, l_i);

          Ddi_BddCofactorAcc(init, v_i, pos);
          Ddi_Free(l_i);
        }
        Ddi_VararrayRemove(ps, i);
        Ddi_VararrayRemove(ns, i);
        Ddi_BddarrayRemove(delta, i);
      }
    }
    Ddi_AigarrayComposeNoMultipleAcc(delta, substV, substF);
    Ddi_AigarrayComposeNoMultipleAcc(lambda, substV, substF);
  }

  if (neq > 0) {
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout,
        "Found %d seq EQ - Reduced DELTA - size: %d - components: %d\n",
        neq, Ddi_BddarraySize(delta), n));
  }
  Ddi_Free(substV);
  Ddi_Free(substF);

  Pdtutil_Free(sizeA);
  Pdtutil_Free(enEq);
  return (neq + nconst);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static int
fsmTsimReduction(
  Fsm_Mgr_t * fsmMgr,
  int enTernary,
  int timeLimit,
  Fbv_Globals_t * opt
)
{
  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(lambda);
  Ddi_Varset_t *dsupp, *stubsupp;
  int i, n, nconst = 0, neq = 0;
  int *sizeA, *enEq, *suppSize;
  Ddi_Bddarray_t *substF;
  Ddi_Vararray_t *substV;
  Ddi_Bdd_t *reached;
  int proved = 0;
  int addStub;
  int strongRun = 0;
  Ddi_Varset_t **suppDelta = NULL;
  Ddi_Bddarray_t *initArray=NULL;

  n = Ddi_BddarrayNum(delta);

  Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Original Delta: Size=%d; #Partitions=%d.\n",
      Ddi_BddarraySize(delta), n));

  /* ternary fix overappr simulation */

  if (enTernary) {
    reached = Fsm_XsimTraverse(fsmMgr, enTernary, opt->pre.ternarySim,
      &proved, timeLimit);
  } else {
    reached = Ddi_BddMakeConstAig(ddiMgr, 1);
  }

  if (proved) {
    Ddi_Free(reached);
    return -1;
  }

  if (Ddi_BddSize(reached) > 2000) {
    Ddi_Free(reached);
    reached = Ddi_BddMakeConstAig(ddiMgr, 1);
  }
  Ddi_BddSetAig(reached);

  // read again as could be modified in tsim reduction
  initStub = Fsm_MgrReadInitStubBDD(fsmMgr);

  do {
    addStub = 0;
    n = Ddi_BddarrayNum(delta);
    if (0)
      for (i = n - 1; i >= 0; i--) {
        Ddi_Var_t *pv_i = Ddi_VararrayRead(ps, i);
        Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);
        Ddi_Bdd_t *d_i_bar = Ddi_BddNot(d_i);
        int isOne = !Ddi_AigSatAnd(d_i_bar, reached, NULL); /* @ */
        int isZero = !isOne && !Ddi_AigSatAnd(d_i, reached, NULL);
        int isConst = isOne || isZero;

        Ddi_Free(d_i_bar);
        if (!Ddi_BddIsConstant(d_i) && isConst && pv_i != opt->pre.clkVarFound
          && strncmp(Ddi_VarName(pv_i), "PDT_BDD_", 8) != 0) {
          int removeVar = 0;
          Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(pv_i, isOne);

          if ((initStub == NULL) && Ddi_AigSatAnd(init, lit, NULL)) {
            removeVar = 1;
          } else if (initStub != NULL) {
            Ddi_Bdd_t *stub_i = Ddi_BddDup(Ddi_BddarrayRead(initStub, i));

            if (isOne)
              Ddi_BddNotAcc(stub_i);
            if (!Ddi_AigSat(stub_i)) {
              /* init stub confirms constant value: reduce */
              removeVar = 1;
              Pdtutil_VerbosityMgrIf(ddiMgr, Pdtutil_VerbLevelDevMed_c) {
                fprintf(stdout, "Remove var %s\n", Ddi_VarName(pv_i));
              }
              Ddi_BddarrayRemove(initStub, i);
            } else if (1) {
              addStub = 1;
            }
            Ddi_Free(stub_i);
          }
          if (removeVar) {
#if 1
            Ddi_BddCofactorAcc(init, pv_i, isOne);
            Ddi_BddarrayRemove(delta, i);
            Ddi_VararrayRemove(ps, i);
            Ddi_VararrayRemove(ns, i);
            Ddi_AigarrayConstrainCubeAcc(delta, lit);
            Ddi_AigarrayConstrainCubeAcc(lambda, lit);
#else
            Ddi_BddCofactorAcc(d_i, pv_i, isOne);
#endif
            nconst++;
          }
          Ddi_Free(lit);
        }
      }
    if (addStub) {
      char suffix[4];
      Ddi_Vararray_t *newPiVars;
      Ddi_Bddarray_t *newPiLits;
      Ddi_Bddarray_t *myInitStub = Ddi_BddarrayDup(delta);

      sprintf(suffix, "%d", fsmMgr->stats.initStubSteps++);
      newPiVars = Ddi_VararrayMakeNewVars(pi, "PDT_STUB_PI", suffix, 1);
      newPiLits = Ddi_BddarrayMakeLiteralsAig(newPiVars, 1);
      Ddi_BddarrayComposeAcc(myInitStub, pi, newPiLits);
      if (Fsm_MgrReadInitStubBDD(fsmMgr) != NULL) {
        Ddi_BddarrayComposeAcc(myInitStub, ps, Fsm_MgrReadInitStubBDD(fsmMgr));
      }
      Ddi_Free(newPiVars);
      Ddi_Free(newPiLits);
      Fsm_MgrSetInitStubBDD(fsmMgr, myInitStub);
      Ddi_Free(myInitStub);
      initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
      //      Fsm_MgrIncrInitStubSteps (fsmMgr,1);
      Pdtutil_WresIncrInitStubSteps(1);
    }
  } while (addStub);

  Ddi_Free(reached);

  n = Ddi_BddarrayNum(delta);

  if (nconst > 0) {
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "Found %d Constants; ", nconst);
      fprintf(stdout, "Reduced Delta: Size=%d; #Partitions=%d.\n",
        Ddi_BddarraySize(delta), n));
  }

  sizeA = Pdtutil_Alloc(int,
    n
  );
  enEq = Pdtutil_Alloc(int,
    n
  );

  for (i = 0; i < n; i++) {
    Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);

    sizeA[i] = Ddi_BddSize(d_i);
    enEq[i] = 1;

    if (0 && sizeA[i] == 1) {
      Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);
      Ddi_Var_t *v = Ddi_BddTopVar(d_i);

      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
        fprintf(stdout, "Literal delta[%d]: %s <- %s%s.\n", i,
          Ddi_VarName(v_i), Ddi_BddIsComplement(d_i) ? "!" : "",
          Ddi_VarName(v)));
    }
  }

  substV = Ddi_VararrayAlloc(ddiMgr, 0);
  substF = Ddi_BddarrayAlloc(ddiMgr, 0);

  if (strongRun) {
    suppDelta = Ddi_BddarraySuppArray(delta);
    suppSize = Pdtutil_Alloc(int,
      n
    );

    for (i = 0; i < n; i++) {
      suppSize[i] = Ddi_VarsetNum(suppDelta[i]);
    }
  }

  if (initStub == NULL) {
    initArray = Ddi_BddarrayMakeLiteralsAig(ps, 1);
    Ddi_AigarrayConstrainCubeAcc(initArray, init);
  }

  for (i = 0; i < n; i++) {
    if (sizeA[i] <= 100 && enEq[i]) {
      int j;
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);
      Ddi_Bdd_t *d_i_bar = NULL;
      Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);
      Ddi_Varset_t *s_i = strongRun ? suppDelta[i] : NULL;

      if (strncmp(Ddi_VarName(v_i), "PDT_BDD_", 8) == 0) {
        if (strongRun) {
          Ddi_Free(suppDelta[i]);
        }
        continue;
      }
      d_i_bar = Ddi_BddNot(d_i);
      for (j = i + 1; j < n; j++) {
        Ddi_Varset_t *s_j = strongRun ? suppDelta[i] : NULL;

        if ((!strongRun || suppSize[j] == suppSize[i]) &&
          (strongRun || (sizeA[j] == sizeA[i])) && enEq[j]) {
          int equal = 0;
          Ddi_Bdd_t *d_j = Ddi_BddarrayRead(delta, j);
          Ddi_Var_t *v_j = Ddi_VararrayRead(ps, j);

          if (strncmp(Ddi_VarName(v_j), "PDT_BDD_", 8) == 0) {
            if (strongRun) {
              Ddi_Free(suppDelta[i]);
            }
            continue;
          }
          if (strongRun ? Ddi_BddEqualSat(d_i, d_j) : Ddi_BddEqual(d_i, d_j)) {
            /* equal deltas found ! Check init state */
            if (initStub != NULL) {
              equal = strongRun ?
                Ddi_BddEqualSat(Ddi_BddarrayRead(initStub, i),
                Ddi_BddarrayRead(initStub,
                  j)) : Ddi_BddEqual(Ddi_BddarrayRead(initStub, i),
                Ddi_BddarrayRead(initStub, j));
            } else {
	      Ddi_Bdd_t *init_i = Ddi_BddarrayRead(initArray,i);
	      Ddi_Bdd_t *init_j = Ddi_BddarrayRead(initArray,j);
	      equal = Ddi_BddEqual(init_i, init_j);
            }
            if (equal) {
              Ddi_Bdd_t *l_i = Ddi_BddMakeLiteralAig(v_i, 1);

              Ddi_BddarrayInsertLast(substF, l_i);
              Ddi_VararrayInsertLast(substV, v_j);
              Ddi_Free(l_i);
              enEq[j] = 0;
              neq++;

              Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
                fprintf(stdout, "SEQ equiv found(j:%d,i:%d): %s -> %s.\n", j,
                  i, Ddi_VarName(v_j), Ddi_VarName(v_i)));
            }
          } else if (strongRun ? Ddi_BddEqualSat(d_i_bar, d_j) :
            Ddi_BddEqual(d_i_bar, d_j)) {
            /* equal deltas found ! Check init state */
            if (initStub != NULL) {
              Ddi_Bdd_t *s_j = Ddi_BddNot(Ddi_BddarrayRead(initStub, j));

              equal = strongRun ?
                Ddi_BddEqualSat(Ddi_BddarrayRead(initStub, i), s_j) :
                Ddi_BddEqual(Ddi_BddarrayRead(initStub, i), s_j);
              Ddi_Free(s_j);
            } else {
              Ddi_Bdd_t *eq = Ddi_BddMakeLiteralAig(v_i, 0);
              Ddi_Bdd_t *l_j = Ddi_BddMakeLiteralAig(v_j, 1);

              Ddi_BddXnorAcc(eq, l_j);
              Ddi_Free(l_j);
              equal = Ddi_BddIncluded(init, eq);
              Ddi_Free(eq);
            }
            if (equal) {
              Ddi_Bdd_t *l_i = Ddi_BddMakeLiteralAig(v_i, 0);

              Ddi_BddarrayInsertLast(substF, l_i);
              Ddi_VararrayInsertLast(substV, v_j);
              Ddi_Free(l_i);
              enEq[j] = 0;
              neq++;

              Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
                fprintf(stdout, "SEQ equiv found: %s -> !%s\n",
                  Ddi_VarName(v_j), Ddi_VarName(v_i)));
            }
          }
        }
      }
      Ddi_Free(d_i_bar);
    }
    if (strongRun) {
      Ddi_Free(suppDelta[i]);
    }
  }
  if (strongRun) {
    Pdtutil_Free(suppDelta);
    Pdtutil_Free(suppSize);
  }

  if (neq > 0) {
    for (i = n - 1; i >= 0; i--) {
      if (!enEq[i]) {
        if (initStub != NULL) {
          Ddi_BddarrayRemove(initStub, i);
        }
        Ddi_VararrayRemove(ps, i);
        Ddi_VararrayRemove(ns, i);
        Ddi_BddarrayRemove(delta, i);
      }
    }
    if (initStub != NULL) {
      Ddi_BddComposeAcc(init, substV, substF);
    }
    Ddi_AigarrayComposeNoMultipleAcc(delta, substV, substF);
    Ddi_AigarrayComposeNoMultipleAcc(lambda, substV, substF);
  }
  Ddi_Free(initArray);

  if (neq > 0) {
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout,
        "Found %d seq EQ - Reduced DELTA - size: %d - components: %d\n",
        neq, Ddi_BddarraySize(delta), n));
  }
  Ddi_Free(substV);
  Ddi_Free(substF);

  Pdtutil_Free(sizeA);
  Pdtutil_Free(enEq);
  return (neq + nconst);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static int
fsmEqReduction(
  Fsm_Mgr_t * fsmMgr,
  int timeLimit,
  Fbv_Globals_t * opt,
  int depth,
  int latchOnly
)
{
  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
  Ddi_Bdd_t *invarspec = Fsm_MgrReadInvarspecBDD(fsmMgr);
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(lambda);
  Ddi_Varset_t *dsupp, *stubsupp;
  int i, n, nconst = 0, neq = 0;
  int *sizeA, *enEq, *suppSize;
  Ddi_Bddarray_t *substF;
  Ddi_Vararray_t *substV;
  Ddi_Bdd_t *reached;
  int proved = 0;
  unsigned long int time_limit = ~0;
  long initTime, endTime;
  int addStub;
  int strongRun = 0;
  Ddi_Varset_t **suppDelta = NULL;
  Fsm_XsimMgr_t *xMgr;
  int initStubInReached = 0;
  int enSimulation = 1;
  int nEq;
  int singlePattern = 0;

  if (timeLimit > 0) {
    /* actually NOT used */
    time_limit = util_cpu_time() + timeLimit * 1000;
  }

  n = Ddi_BddarrayNum(delta);

  Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Original Delta: Size=%d; #Partitions=%d.\n",
      Ddi_BddarraySize(delta), n));

  /* ternary fix overappr simulation */

  if (enSimulation) {
    int nState = Ddi_VararrayNum(ns);

    //    Ddi_Bdd_t *myTr = Ddi_BddRelMakeFromArray(delta,ns);
    int nTimeFrames = depth == 0 ? 4 : depth * 8;
    int maxDepth = opt->pre.indEqMaxLevel;

    if (latchOnly) {
      maxDepth = depth == 0 ? -2 : -1;
    }

    xMgr =
      Fsm_XsimInit(fsmMgr, delta, invarspec, ps, NULL,
      5 /*opt->pre.ternarySim */ );

    if (singlePattern) {
      Pdtutil_Array_t *toArray, *fromArray;

      Pdtutil_Assert(initStub == NULL, "init stub not supported");
      fromArray = Ddi_AigCubeMakeIntegerArrayFromBdd(init, ps);
      toArray = Fsm_XsimSimulate(xMgr, 2, fromArray, 0, NULL);
      Pdtutil_IntegerArrayFree(fromArray);
      fromArray = toArray;
      toArray = Fsm_XsimSimulate(xMgr, 2, fromArray, 0, NULL);
      Pdtutil_IntegerArrayFree(toArray);
      Pdtutil_IntegerArrayFree(fromArray);
    } else {
      Fsm_XsimRandomSimulateSeq(xMgr, initStub, init, nTimeFrames);
    }

    Fsm_XsimInductiveEq(fsmMgr, xMgr, depth,
      maxDepth, opt->pre.indEqAssumeMiters, &nEq);

    if (nEq > 0) {
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "SAT FOUND %d EQUIVALENCES\n", nEq));
      // Fsm_XsimPrintEqClasses(xMgr);
      if (1) {
        Ddi_Bddarray_t *newD = Fsm_XsimSymbolicMergeEq(xMgr, NULL, invarspec);

        if (newD != NULL) {
          Ddi_Bdd_t *invar =
            Ddi_BddDup(Ddi_BddarrayRead(delta, Ddi_BddarrayNum(delta) - 2));
          Ddi_Bdd_t *propD =
            Ddi_BddarrayRead(newD, Ddi_BddarrayNum(delta) - 1);
          Ddi_BddarrayWrite(newD, Ddi_BddarrayNum(delta) - 2, invar);
          Ddi_BddNotAcc(invar);
          Ddi_BddOrAcc(propD, invar);
          Ddi_Free(invar);
          if (Ddi_BddarraySize(newD) == Ddi_BddarraySize(delta) && nEq <= 5) {
            nEq = 0;
          }
          Ddi_DataCopy(delta, newD);
          Fsm_FsmStructReduction(fsmMgr, opt->pre.clkVarFound);
          Ddi_Free(newD);
        }
      }
    }

    Fsm_XsimQuit(xMgr);
  }
  return nEq;
}

/**Function*******************************************************************
  Synopsis    [manual partitioning of TR based on partition file]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Varset_t *
FindClockInTr(
  Tr_Tr_t * tr
)
{
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Ddi_Bdd_t *trBdd = Ddi_BddDup(Tr_TrBdd(tr));
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(trBdd);
  int i, j;
  Ddi_Var_t *vPS, *vNS;
  Ddi_Bdd_t *litPS, *litNS, *neq, *part;
  Ddi_Varset_t *supp;
  Ddi_Vararray_t *psv = Tr_MgrReadPS(trMgr);
  Ddi_Vararray_t *nsv = Tr_MgrReadNS(trMgr);

  Ddi_Varset_t *clock_vars = Ddi_VarsetVoid(ddiMgr);


  if (!Ddi_BddIsPartConj(trBdd)) {
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "NO conj part TR for clock search\n"));
    return (clock_vars);
  }

  for (i = 0; i < Ddi_BddPartNum(trBdd); i++) {
    Ddi_BddSuppAttach(Ddi_BddPartRead(trBdd, i));
  }

  for (j = 0; j < Ddi_VararrayNum(psv); j++) {
    vPS = Ddi_VararrayRead(psv, j);
    vNS = Ddi_VararrayRead(nsv, j);
    litPS = Ddi_BddMakeLiteral(vPS, 1);
    litNS = Ddi_BddMakeLiteral(vNS, 1);
    neq = Ddi_BddXor(litPS, litNS);
    Ddi_Free(litPS);
    Ddi_Free(litNS);
    for (i = 0; i < Ddi_BddPartNum(trBdd); i++) {
      supp = Ddi_BddSuppRead(Ddi_BddPartRead(trBdd, i));
      if (Ddi_VarInVarset(supp, vNS)) {
        part = Ddi_BddPartRead(trBdd, i);
        if (Ddi_BddIncluded(part, neq)) {
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
            fprintf(stdout, "Clock var found: %s\n", Ddi_VarName(vPS)));
          Ddi_VarsetAddAcc(clock_vars, vPS);
        }
      }
    }
    Ddi_Free(neq);
  }

  for (i = 0; i < Ddi_BddPartNum(trBdd); i++) {
    Ddi_BddSuppDetach(Ddi_BddPartRead(trBdd, i));
  }

  return (clock_vars);

}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static node_info_item_t *
hashSet(
  st_table * table,
  DdNode * f,
  Fbv_Globals_t * opt
)
{
  struct node_info_item *info_item_ptr;

  if (!st_lookup(table, (char *)f, (char **)&info_item_ptr)) {
    info_item_ptr = ALLOC(struct node_info_item,
      1
    );

    if (info_item_ptr == NULL) {
      return (NULL);
    }
    if (st_add_direct(table, (char *)f, (char *)info_item_ptr)
      == ST_OUT_OF_MEM) {
      FREE(info_item_ptr);
      return (NULL);
    }
    info_item_ptr->cu_bdd = f;
    info_item_ptr->merge = NULL;
    info_item_ptr->used = 0;
    info_item_ptr->top = 0;
    info_item_ptr->toptop = 1;
    info_item_ptr->ph0 = 0;
    info_item_ptr->ph1 = 0;
    info_item_ptr->num0 = info_item_ptr->num1 = opt->ddi.info_item_num++;
  } else {
    info_item_ptr->num1 = opt->ddi.info_item_num;
  }
  return (info_item_ptr);
}



/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static int
nodeMerge(
  DdManager * manager,
  st_table * table,
  node_info_item_t * info0,
  node_info_item_t * info1,
  DdNode * auxV,
  int phase,
  DdNode ** resP
)
{
  DdNode *cu0 = info0->cu_bdd;
  DdNode *cu1 = info1->cu_bdd;
  DdNode *myRes0, *myRes1;
  int id0 = Cudd_NodeReadIndex(cu0);
  int id1 = Cudd_NodeReadIndex(cu1);
  int ret;

  *resP = NULL;

  if (cu0 == cu1) {
    return 1;
  }
  if (id0 != id1) {
    return 0;
  }

  if (info0->used || info1->used) {
    return 0;
  }
  if (info0->toptop) {
    DdNode *T0 = Cudd_T(cu0);
    DdNode *T1 = Cudd_T(cu1);
    DdNode *E0 = Cudd_E(cu0);
    DdNode *E1 = Cudd_E(cu1);
    node_info_item_t *info_T0, *info_T1, *info_E0, *info_E1;
    int ret0, ret1;
    DdNode *topV = Cudd_bddIthVar(manager, id0);

    Pdtutil_Assert(info1->toptop, "wrong top flag");

    if ((Cudd_Regular(E0) == E0) != (Cudd_Regular(E1) == E1)) {
      return (0);
    }

    st_lookup(table, (char *)Cudd_Regular(T0), (char **)&info_T0);
    st_lookup(table, (char *)Cudd_Regular(T1), (char **)&info_T1);
    st_lookup(table, (char *)Cudd_Regular(E0), (char **)&info_E0);
    st_lookup(table, (char *)Cudd_Regular(E1), (char **)&info_E1);

    ret0 = nodeMerge(manager, table, info_T0, info_T1, auxV, phase, &myRes0);
    Pdtutil_Assert(Cudd_Regular(T0) == T0, "Complemented then edge");

    ret1 = nodeMerge(manager, table, info_E0, info_E1, auxV, phase, &myRes1);
    if (myRes1 != NULL && Cudd_Regular(E0) != E0) {
      myRes1 = Cudd_Not(myRes1);
    }

    if (ret0 == 0 || ret1 == 0) {
      return (0);
    } else if ((ret0 >= 2) && (ret1 >= 2)) {
      /* merge right-left */
      if (phase == 1) {
        Pdtutil_Assert(myRes0 != NULL, "NULL merged BDD found");
        Pdtutil_Assert(myRes1 != NULL, "NULL merged BDD found");
        *resP = Cudd_bddIte(manager, topV, myRes0, myRes1);
        Cudd_Ref(*resP);
        Cudd_IterDerefBdd(manager, myRes0);
        Cudd_IterDerefBdd(manager, myRes1);
      }
      return (2 * (ret1 + ret0));
    } else if ((ret0 >= 1 && ret1 == -1) || (ret1 >= 2)) {
      /* merge left */
      Pdtutil_Assert(myRes0 == NULL, "Non NULL merged BDD found");
      if (ret1 < 0)
        ret1 = ret0;
      if (phase == 1) {
        Pdtutil_Assert(myRes1 != NULL, "NULL merged BDD found");
        *resP = Cudd_bddIte(manager, topV, T0, myRes1);
        Cudd_Ref(*resP);
        Cudd_IterDerefBdd(manager, myRes1);
      }
      return (2 * ret1);
    } else if ((ret0 == -1 && ret1 >= 1) || (ret0 >= 2)) {
      /* merge right */
      Pdtutil_Assert(myRes1 == NULL, "Non NULL merged BDD found");
      if (ret0 < 0)
        ret0 = ret1;
      if (phase == 1) {
        Pdtutil_Assert(myRes0 != NULL, "NULL merged BDD found");
        *resP = Cudd_bddIte(manager, topV, myRes0, E0);
        Cudd_Ref(*resP);
        Cudd_IterDerefBdd(manager, myRes0);
      }
      return (2 * ret0);
    } else if (ret0 <= 0 && ret1 <= 0) {
      Pdtutil_Assert(myRes0 == NULL, "Non NULL merged BDD found");
      Pdtutil_Assert(myRes1 == NULL, "Non NULL merged BDD found");
      return (0);
    } else {
      return (1);
    }

  } else {
    if (info0->ph0 && !info1->ph1)
      return 0;
    if (!info0->ph0 && info1->ph1)
      return 0;
    if (phase == 1) {
      if (cu0 != cu1) {
        /* do merge */
        DdNode *tmpCU = Cudd_bddIte(manager, auxV, cu1, cu0);

        Cudd_Ref(tmpCU);
        *resP = tmpCU;
        return -1;
      }
      return 1;
    } else {
      ret = cu0 == cu1;
      if (ret == 0) {
        ret = -1;
        /* can merge */ ;
      }
      return (ret);
    }
  }

}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
FbvTargetEn(
  Fsm_Mgr_t * fsmMgr,
  int nIter
)
{
  Ddi_Mgr_t *ddiMgr = Fsm_MgrReadDdManager(fsmMgr);

  Fsm_Fsm_t *fsmFsm = Fsm_FsmMakeFromFsmMgr(fsmMgr);
  Fsm_FsmUnfoldProperty(fsmFsm, 1);
  Fsm_FsmUnfoldConstraint(fsmFsm);

  Ddi_Vararray_t *newPis = Ddi_VararrayAlloc(ddiMgr,0);
  Ddi_Vararray_t *ps = fsmFsm->ps;
  Ddi_Vararray_t *ns = fsmFsm->ns;
  Ddi_Vararray_t *pi = fsmFsm->pi;
  Ddi_Bddarray_t *delta = fsmFsm->delta;
  Ddi_Varset_t *psVars = Ddi_VarsetMakeFromArray(ps);
  Ddi_Bdd_t *invarspec=fsmFsm->invarspec;
  int size0 = Ddi_BddSize(invarspec);
  int doExist = nIter < 0;
  int printInfo=0;
  Ddi_Bdd_t *ring=NULL;
  int inRing = 1, margin = 10, nItpRuns = 4;
  int initSteps = 20;

  if (printInfo) {
    int i;
    for (i=0; i<Ddi_BddarrayNum(delta); i++) {
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta,i);
      Ddi_Varset_t *s = Ddi_BddSupp(d_i);
      printf("D[%3d] s:%4d - size: %6d\n", 
	     i, Ddi_VarsetNum(s), Ddi_BddSize(d_i));
      if (Ddi_VarsetNum(s)<3) {
	printf("latch: %s\n", Ddi_VarName(Ddi_VararrayRead(ps,i)));
	DdiLogBdd(d_i,0);
      }
      Ddi_Free(s);
    }
  }


  if (0 && (nIter<0)) {
    int repart=0;
    Ddi_Bdd_t *pAnd = Ddi_AigPartitionTop(invarspec, 0);
    int np = Ddi_BddPartNum(pAnd);
    nIter=-nIter;
    if (nIter > 100) {
      nIter-=100;
      repart = 1;
    }
    if (np>1 && nIter<=np) {
      Ddi_DataCopy(invarspec,Ddi_BddPartRead(pAnd,nIter-1));
    }
    Ddi_Free(pAnd);
    if (repart) {
      int i, j;
      Ddi_Bdd_t *newI = Ddi_BddMakeConstAig(ddiMgr, 0);
      Ddi_Bdd_t *pOr = Ddi_AigPartitionTop(invarspec, 1);
      for (i=0; i<Ddi_BddPartNum(pOr); i++) {
	Ddi_Bdd_t *p_i = Ddi_BddPartRead(pOr,i);
	Ddi_Bdd_t *pAnd = Ddi_AigPartitionTop(p_i, 0);
	int npA = Ddi_BddPartNum(pAnd);
	if (npA>1) {
	  Ddi_BddPartSortBySizeAcc(pAnd, 0);   // 0: decreasing size
	  for (j=npA-1; j>1; j--) {
	    Ddi_BddPartRemove(pAnd,j);
	  }
	}
	Ddi_BddSetAig(pAnd);
	Ddi_BddOrAcc(newI,pAnd);
	Ddi_Free(pAnd);
      }
      Ddi_Free(pOr);
      Ddi_DataCopy(invarspec,newI);
      Ddi_Free(newI);
    }
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      printf("TARGET SUBSETTING: %d -> %d\n", 
	     size0, Ddi_BddSize(invarspec));
    }
  }
  else {
  Ddi_Bdd_t *saveD = Ddi_BddNot(invarspec);
  while (nIter != 0) {
    Ddi_Bdd_t *deltaTarget0 = Ddi_BddNot(invarspec);
    Ddi_Bdd_t *deltaTarget1 = NULL;
    Ddi_Bdd_t *deltaTarget2 = NULL;
    
    if (doExist && nIter < 2) {
      float lazyRate = Ddi_MgrReadAigLazyRate(ddiMgr);
      Ddi_Bdd_t *saveD2 = Ddi_BddDup(saveD);
      
      Ddi_MgrSetAigLazyRate(ddiMgr, 1.05);
      Ddi_AigExistProjectItpAcc(deltaTarget0, psVars, NULL,1);
      //      Ddi_AigExistProjectSkolemAcc(deltaTarget0, psVars, NULL);
      //      Ddi_AigExistProjectOverAcc(deltaTarget0, psVars, NULL);
      //      Ddi_AigExistProjectAcc(deltaTarget0, psVars, NULL, 0, 1, 10);
      Ddi_MgrSetAigLazyRate(ddiMgr, lazyRate);
      if (0&&Ddi_AigOptByInterpolantAcc(saveD2,deltaTarget0,NULL)) {
        Ddi_DataCopy(deltaTarget0,saveD2);
        Ddi_AigOptByMonotoneCoreAcc (deltaTarget0,saveD,NULL,1,-1.0);
      }
      Ddi_Free(saveD2);
    }
    if (1) {
      Ddi_Var_t *pvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
      char suffix[4];
      Ddi_Vararray_t *vA, *newPiVars;
      Ddi_Bddarray_t *newPiLits;
      
      vA = Ddi_BddSuppVararray(deltaTarget0);
      if (saveD!=NULL) {
        Ddi_Vararray_t *vA2 = Ddi_BddSuppVararray(saveD);
        Ddi_VararrayUnionAcc(vA,vA2);
      }
      sprintf(suffix, "%d", nIter);
      //	Ddi_VararrayDiffAcc(vA, ps);
      Ddi_VararrayIntersectAcc(vA, pi);
      newPiVars = Ddi_VararrayMakeNewVars(vA, "PDT_TE_STUB_PI", suffix, 1);
      newPiLits = Ddi_BddarrayMakeLiteralsAig(newPiVars, 1);
      deltaTarget1 = Ddi_BddCompose(deltaTarget0, vA, newPiLits);
      if (saveD!=NULL) {
        Ddi_BddComposeAcc(saveD, vA, newPiLits);
      }
      Ddi_VararrayAppend(newPis, newPiVars);
      
      deltaTarget2 = Ddi_BddDup(deltaTarget1);
      Ddi_BddCofactorAcc(deltaTarget2,pvarPs,1);


      if (0) {
	int i, n0;
	bAig_Manager_t *bmgr = ddiMgr->aig.mgr;
	bAig_array_t *visitedNodes = bAigArrayAlloc();

	for (i=0; i<Ddi_BddarrayNum(delta)-2; i++) {
	  Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta,i);
	  Ddi_PostOrderBddAigVisitIntern(d_i, visitedNodes, -1);
	}
	n0 = visitedNodes->num;
#if 0
	for (i=0; i<n0;i++) {
	  bAigEdge_t baig = visitedNodes->nodes[i];
	  bmgr->visited[bAigNodeID(baig)] = 8;
	}
#endif
	Ddi_PostOrderBddAigVisitIntern(deltaTarget0, visitedNodes, -1);

	Ddi_PostOrderAigClearVisitedIntern(bmgr, visitedNodes);
	bAigArrayFree(visitedNodes);

      }

      Ddi_BddComposeAcc(deltaTarget2, ps, delta);
      if (saveD!=NULL) {
        Ddi_BddComposeAcc(saveD, ps, delta);
      }
      
      Ddi_Free(newPiLits);
      Ddi_Free(newPiVars);
      Ddi_Free(vA);
    }
    // Ddi_BddOrAcc(deltaTarget1,deltaTarget2);

    if (0) {
      int i;
      Ddi_Vararray_t *vA, *vA2;
      char name[1000];
      vA = Ddi_BddSuppVararray(deltaTarget0);
      Ddi_VararrayDiffAcc(vA, ps);
      vA2 = Ddi_VararrayDup(vA);
      for (i=0; i<Ddi_VararrayNum(vA2); i++) {
	Ddi_Var_t *v2,*v = Ddi_VararrayRead(vA2,i);
	if (strncmp(Ddi_VarName(v),"PDT_TE_STUB_PI",14)==0) {
	  char *s; int t;
	  strcpy(name,Ddi_VarName(v));
	  for (s=name+strlen(name);*s!='_';s--);
	  s++;
	  Pdtutil_Assert(s>name,"problem processing var name");
	  t = atoi(s);
	  sprintf(s,"%d",t-1);
	  v2 = Ddi_VarFromName(ddiMgr, name);
	  if (v2==NULL) {
	    v2 = Ddi_VarFindOrAdd(ddiMgr,name,0);
	    Ddi_VararrayInsertLast(newPis,v2);
	  }
	  Ddi_VararrayWrite(vA2,i,v2);
	}
      }
      Ddi_BddSubstVarsAcc(deltaTarget0, vA, vA2);
      Ddi_BddDiffAcc(deltaTarget2, deltaTarget0);
      Ddi_Free(vA);
      Ddi_Free(vA2);
    }

    Ddi_DataCopy(deltaTarget1, deltaTarget2);
    if (abs(nIter) == margin) {
      Ddi_MgrSetAigAbcOptLevel(ddiMgr, 1);
      ddiAbcOptAcc(deltaTarget1, -1.0);

      //      Ddi_NnfClustSimplifyAcc(deltaTarget1,0);

      Ddi_Varset_t *supp = Ddi_BddSupp(deltaTarget1);
      Ddi_VarsetIntersectAcc(supp, psVars);
      int i, period=4;
      int sat;
      Ddi_Bdd_t *q, *q0;
      Ddi_Bddarray_t *d = Ddi_BddarrayDup(delta);

      Ddi_Bdd_t *itp;
      Ddi_Vararray_t *vA, *vA2;
      char suffix[4];


      vA = Ddi_BddarraySuppVararray(delta);
      Ddi_VararrayDiffAcc(vA, ps);

      for (i=0; i<initSteps; i++) {
	Ddi_Bddarray_t *d2 = Ddi_BddarrayDup(delta);
	sprintf(suffix, "%d", nIter);
	vA2 = Ddi_VararrayMakeNewVars(vA, "PDT_TE_INIT_STUB_PI", suffix, 1);
	Ddi_BddarraySubstVarsAcc(d2, vA, vA2);
	Ddi_Free(vA2);
	Ddi_BddarrayComposeAcc(d,ps,d2);
	Ddi_Free(d2);
      }
      Ddi_Free(vA);

      Ddi_AigarrayConstrainCubeAcc(d, fsmFsm->init);
      int again = 1;

      q = Ddi_BddRelMakeFromArray(d,ps);
      q0 = Ddi_BddDup(q);
      Ddi_Free(d);

      for (i=0; i<nItpRuns && again; i++) {

	printf("\nITP ITERATION # %d\n\n", i);
	if (i>0) {
	  int maxIter = 10, res;
	  Ddi_Bdd_t *itpGen =
	    Ddi_AigInterpolantByGenClauses(q, deltaTarget1, NULL, NULL, ns,
					   ps, NULL, ps, NULL, NULL, NULL,-maxIter, 0, &res);
	  Ddi_BddOrAcc(q,itpGen);
	  Ddi_Free(itpGen);
	}
	itp = Ddi_AigSatAndWithInterpolant(q,deltaTarget1,
				supp,NULL,NULL,NULL,
			        NULL,NULL,&sat,0,0,-1.0);
	Ddi_AigOptByMonotoneCoreAcc (itp,deltaTarget1,NULL,0,-1.0);

	Ddi_BddSetAig(q);
	Ddi_BddNotAcc(q);
	Ddi_BddAndAcc(q,itp);
	again = Ddi_AigSat(q);
	Ddi_Free(q);
	q = itp;
      }

      ring = Ddi_BddDup(q);
      //      Ddi_BddNotAcc(q);
      //      Ddi_BddAndAcc(deltaTarget1,q);
      Ddi_Free(supp);
      Ddi_Free(q);
      Ddi_Free(q0);

      // DdiAigRedRemovalOdcAcc (deltaTarget1,NULL,-1,-1.0);
      // DdiAigRedRemovalOdcByEqClasses (deltaTarget1, NULL, -1, 0, -1.0);
      ddiAbcOptAcc(deltaTarget1, -1.0);

    }
    else if (abs(nIter) == 1 && ring!=NULL) {
      //      Ddi_BddNotAcc(q);
      if (inRing)  
	Ddi_BddAndAcc(deltaTarget1,ring);
      else 
	Ddi_BddDiffAcc(deltaTarget1,ring);
    }

    Ddi_BddNotAcc(deltaTarget1);

    Ddi_DataCopy(invarspec, deltaTarget1);

    Ddi_Free(deltaTarget0);
    Ddi_Free(deltaTarget1);
    Ddi_Free(deltaTarget2);

    if (doExist) {
      nIter++;
    } else {
      nIter--;
    }
  }
  Ddi_Free(saveD);
  }
  
  Ddi_VararrayAppend(pi,newPis);
  Ddi_Free(newPis);
  Ddi_Free(psVars);

  Fsm_FsmFoldProperty(fsmFsm, 0, 0, 1);
  Fsm_FsmFoldConstraint(fsmFsm, 0);
  Fsm_FsmWriteToFsmMgr(fsmMgr, fsmFsm);
  Fsm_FsmFree(fsmFsm);

  return 1;
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
FbvGetSpec(
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * partInvarspec,
  Fbv_Globals_t * opt
)
{
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Bddarray_t *delta, *lambda;
  Ddi_Mgr_t *ddiMgr;
  Ddi_Bdd_t *invarspec = NULL;
  int customConstraint = 1;
  int specEnIter = 0;

  pi = Fsm_MgrReadVarI(fsmMgr);
  ps = Fsm_MgrReadVarPS(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  ddiMgr = Ddi_ReadMgr(lambda);

  /**********************************************************************/
  /*                       handle invariant                             */
  /**********************************************************************/

  if (opt->mc.invSpec != NULL && strstr(opt->mc.invSpec, ".aig") != NULL) {

    Ddi_Bddarray_t *rplus =
      Ddi_AigarrayNetLoadAiger(ddiMgr, NULL, opt->mc.invSpec);
    Ddi_BddarraySubstVarsAcc(rplus,Fsm_MgrReadVarNS(fsmMgr),
			     Fsm_MgrReadVarPS(fsmMgr));
      //end ibmConstr
    Pdtutil_Assert (Ddi_BddarrayNum(rplus)==1,"wrong rplus format");
    Ddi_BddNotAcc(Ddi_BddarrayRead(rplus,0));
    Ddi_BddarrayWrite(lambda,0,Ddi_BddarrayRead(rplus,0));
    Ddi_Free(rplus);
    opt->mc.invSpec = NULL;
  } 
  if (opt->mc.invSpec != NULL) {
    if (strcmp(opt->mc.invSpec, "1") == 0) {
      invarspec = Ddi_BddMakeConst(ddiMgr, 1);
    } else if (strstr(opt->mc.invSpec, ".bdd") != NULL) {
      invarspec = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
        DDDMP_MODE_DEFAULT, opt->mc.invSpec, NULL);
    } else {
      Ddi_Expr_t *e1, *e = Ddi_ExprLoad(ddiMgr, opt->mc.invSpec, NULL);

      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
        fprintf(stdout, "Done\n-- Reading Invariant Specification:\n"));

      Ddi_ExprPrint(e, stdout);

      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
        fprintf(stdout, "\n"));

      e1 = Expr_EvalBddLeafs(e);
      invarspec = Expr_InvarMakeFromAG(e1);
      Ddi_Free(e1);
      Ddi_Free(e);
    }
  } else if (opt->mc.ctlSpec != NULL) {
    if (tr == NULL) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "NULL tr. Ignoring ctl specification.\n"));
    } else {
      Ddi_Expr_t *e1, *e = Ddi_ExprLoad(ddiMgr, opt->mc.ctlSpec, NULL);

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Done\n-- Reading ctl specification.\n"));
      Ddi_ExprPrint(e, stdout);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "\n"));
      e1 = Fbv_EvalCtlSpec(tr, e, opt);
      invarspec = Expr_InvarMakeFromAG(e1);
      Ddi_Free(e1);
      Ddi_Free(e);
    }
  } else if (opt->mc.all1) {
    Ddi_Bdd_t *lit;
    int i;

    if (opt->mc.all1 > Ddi_VararrayNum(ps)) {
      opt->mc.all1 = Ddi_VararrayNum(ps);
    }
    invarspec = Ddi_BddMakeConst(ddiMgr, 0);
    for (i = opt->mc.all1 - 1; i >= 0; i--) {
      lit = Ddi_BddMakeLiteral(Ddi_VararrayRead(ps, i), 0);
      Ddi_BddOrAcc(invarspec, lit);
      Ddi_Free(lit);
    }
  } else if (opt->mc.idelta >= 0) {
    if (opt->mc.idelta >= Ddi_VararrayNum(ps)) {
      opt->mc.idelta = Ddi_VararrayNum(ps) - 1;

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "idelta selection is too large: %d forced\n",
          opt->mc.idelta));
    }
    invarspec = Ddi_BddMakeLiteral(Ddi_VararrayRead(ps, opt->mc.idelta), 0);
  } else if (lambda != NULL) {
    int addLatch;
    int i;

    if (opt->mc.ilambda >= Ddi_BddarrayNum(lambda)) {
      opt->mc.ilambda = Ddi_BddarrayNum(lambda) - 1;

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "ilambda selection is too large: %d forced\n",
          opt->mc.ilambda));
    } else if (opt->mc.nlambda != NULL) {
      invarspec = NULL;
      for (i = 0; i < Ddi_BddarrayNum(lambda); i++) {
        char *name = Ddi_ReadName(Ddi_BddarrayRead(lambda, i));

        if (name != NULL && strcmp(name, opt->mc.nlambda) == 0) {
          invarspec = Ddi_BddDup(Ddi_BddarrayRead(lambda, i));
          break;
        }
      }
      Pdtutil_Assert(invarspec != NULL, "Lambda not found by name");
    } else if (opt->mc.ilambda >= 0) {
      invarspec = Ddi_BddDup(Ddi_BddarrayRead(lambda, opt->mc.ilambda));
    } else if (opt->mc.ilambda < 0) {
      if (opt->mc.compl_invarspec)
        invarspec = Ddi_BddMakePartDisjFromArray(lambda);
      else
        invarspec = Ddi_BddMakePartConjFromArray(lambda);
      if (partInvarspec != NULL) {
        Ddi_DataCopy(partInvarspec, invarspec);
        if (opt->mc.compl_invarspec) {
          Ddi_BddNotAcc(partInvarspec);
        }
      }
      while (Ddi_BddarrayNum(lambda)>1) {
        Ddi_BddarrayRemove(lambda,1);
      }
      if (!opt->fsm.nnf) {
        Ddi_BddSetAig(invarspec);
      }
    }
    Pdtutil_Assert(opt->mc.nlambda != NULL
      || opt->mc.ilambda >= -1, "ilambda or nlambda required");

    int printdeltaSizes=0;
    if (printdeltaSizes) {
      printf("\nDelta:\n");
      for (int i=0; i<Ddi_BddarrayNum(delta); i++) {
	printf("%d\n", Ddi_BddSize(Ddi_BddarrayRead(delta,i)));
      }
    }
    
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Property: %s%s.\n",
        opt->mc.compl_invarspec ? "!" : "",
        Ddi_ReadName(invarspec) ? Ddi_ReadName(invarspec) : "o0");
      fprintf(stdout,
        "Design Statistics: %d Inputs / %d State Vars / %d delta AIG Gates / %d lambda AIG Gates\n",
              Ddi_VararrayNum(pi), Ddi_VararrayNum(ps), Ddi_BddarraySize(delta), Ddi_BddarraySize(lambda))
      );
    if (opt->mc.range) {
      while (--opt->mc.ilambda >= 0) {
        Ddi_BddAndAcc(invarspec, Ddi_BddarrayRead(lambda, opt->mc.ilambda));
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, " * %s%s", opt->mc.compl_invarspec ? "!" : "",
            Ddi_ReadName(Ddi_BddarrayRead(lambda, opt->mc.ilambda))));
      }
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "\n"));
    }

    /*-------------------------- Add property state var --------------------*/

    addLatch = 0 || (partInvarspec != NULL) ||
      (Ddi_BddSize(invarspec) > 0 && !opt->mc.combinationalProblem);
    if (addLatch)
      addLatch = 2;
    //    addLatch = 0;

    if (!opt->mc.compl_invarspec) {
      Ddi_BddNotAcc(invarspec);
    }

    if (0) {
      Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(pi);
      Ddi_Varset_t *isupp = Ddi_BddSupp(invarspec);

      Ddi_VarsetIntersectAcc(isupp, piv);
      if (Ddi_VarsetIsVoid(isupp)) {
        Ddi_BddComposeAcc(invarspec, ps, delta);
      }
      Ddi_Free(piv);
      Ddi_Free(isupp);
    }

    int addSpecAsPsConstr = 0;
    if (addSpecAsPsConstr) {
      int nl = Fsm_MgrReadNL(fsmMgr);

      Ddi_Bdd_t *newLambdaBdd, *property;
      Ddi_Var_t *lambdaPs =
        Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_CONSTR_VAR$PS");
      Ddi_Var_t *lambdaNs =
        Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_CONSTR_VAR$NS");
      if (lambdaPs == NULL) {
        lambdaPs = Ddi_VarNewAtLevel(ddiMgr, 0);
        Ddi_VarAttachName(lambdaPs, "PDT_BDD_INVARSPEC_CONSTR_VAR$PS");
      }
      if (lambdaNs == NULL) {
        int xlevel = Ddi_VarCurrPos(lambdaPs) + 1;
        lambdaNs = Ddi_VarNewAtLevel(ddiMgr, xlevel);
        Ddi_VarAttachName(lambdaNs, "PDT_BDD_INVARSPEC_CONSTR_VAR$NS");
      }

      Ddi_VararrayInsertLast(Fsm_MgrReadVarPS(fsmMgr), lambdaPs);
      Ddi_VararrayInsertLast(Fsm_MgrReadVarNS(fsmMgr), lambdaNs);

      if (Fsm_MgrReadInitStubBDD(fsmMgr) != NULL) {
	newLambdaBdd = Ddi_BddMakeConstAig(ddiMgr, 0);
	Ddi_BddarrayInsertLast(Fsm_MgrReadInitStubBDD(fsmMgr),newLambdaBdd);
	Ddi_Free(newLambdaBdd);
      }
      else {
	newLambdaBdd = Ddi_BddMakeLiteralAig(lambdaPs, 0);
	Ddi_BddAndAcc(Fsm_MgrReadInitBDD(fsmMgr), newLambdaBdd);
	Ddi_Free(newLambdaBdd);
      }
      newLambdaBdd = Ddi_BddNot(invarspec);
      if (opt->mc.compl_invarspec) {
	Ddi_BddNotAcc(newLambdaBdd);
      }
      Ddi_BddarrayInsertLast(Fsm_MgrReadDeltaBDD(fsmMgr),newLambdaBdd);
      Ddi_Free(newLambdaBdd);

      newLambdaBdd = Ddi_BddMakeLiteralAig(lambdaPs, 0);
      Ddi_BddAndAcc(invar, newLambdaBdd);
      Ddi_Free(newLambdaBdd);
      Fsm_MgrSetConstraintBDD(fsmMgr,invar);
      
    }

    if (1 && opt->pre.specDecomp >= 0) {
      int size = Ddi_BddSize(invarspec);
      Ddi_Bdd_t *pinv = Ddi_AigPartitionTop(invarspec, 1);
      int custom = 1;
      if (pinv != NULL && Ddi_BddPartNum(pinv) == 1) {
        Ddi_Bdd_t *p0, *p1, *newPinv = NULL;
        int maxNewPart = -1, repartFactor = 4, enPart = 1;
        Ddi_Free(pinv);
        pinv = Ddi_AigPartitionTop(invarspec, 0);
	Ddi_BddPartSortBySizeAcc(pinv, 0);    // decreasing size

        if (1 && custom) {
          Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(pi);
          Ddi_Bdd_t *pA = Ddi_BddPartRead(pinv,3);
          if (0) {
            Ddi_Bdd_t *p0 = Ddi_AigPartitionTop(Ddi_BddPartRead(pinv, 3), 1);
            Ddi_BddPartSortByTopVarAcc(p0,1);
            for (i=Ddi_BddPartNum(p0)-1;i>=0; i--) {
              if (i!=20 && i!=21)
                Ddi_BddPartRemove(p0,i);
            }

            Ddi_BddSetAig(p0);
            Ddi_DataCopy(pA,p0);
            Ddi_Free(p0);
          }
          else {
            Ddi_Vararray_t *suppA = Ddi_BddSuppVararray(pA);
            int n = Ddi_VararrayNum(suppA);
            Ddi_Varset_t *supp = Ddi_VarsetVoid(ddiMgr);
            for (i=n-1;i>=130; i--) {
              Ddi_Var_t *v = Ddi_VararrayRead(suppA,i);
              Ddi_VarsetAddAcc(supp,v);
            }
            Ddi_BddForallAcc(pA,supp);
            Ddi_Free(suppA);
            Ddi_Free(supp);
          }
#if 0
          pA = Ddi_BddPartRead(pinv,0);
          Ddi_BddForallAcc(pA,piv);
          pA = Ddi_BddPartRead(pinv,1);
          Ddi_BddForallAcc(pA,piv);
          pA = Ddi_BddPartRead(pinv,2);
          Ddi_BddForallAcc(pA,piv);
          Ddi_Free(piv);
#endif          
          //Ddi_BddPartRemove(pinv,4);
          //Ddi_BddPartRemove(pinv,3);
          Ddi_BddPartRemove(pinv,2);
          Ddi_BddPartRemove(pinv,1);
          Ddi_BddPartRemove(pinv,0);
          Ddi_Free(piv);
        }
        else {
        maxNewPart = Ddi_BddPartNum(pinv) * repartFactor;
        p0 = Ddi_AigPartitionTop(Ddi_BddPartRead(pinv, 0), 1);
        Ddi_AigStructRedRemAcc(p0,NULL);

        if (0) {
          Ddi_Bdd_t *pPrem = Ddi_BddDup(pinv);
          Ddi_Bdd_t *p3 = Ddi_BddPartExtract(pPrem,3);
          Ddi_Bdd_t *next = Ddi_BddDup(p3);
          Ddi_BddSetAig(pPrem);
          Ddi_BddDiffAcc(next,pPrem);
          Ddi_BddComposeAcc(next,ps,delta);
          Ddi_BddNotAcc(p3);
          if (!Ddi_AigSatAnd(next,p3,NULL)) {
            printf("closed component found\n");
          }
          else {
            Ddi_BddAndAcc(next,p3);
            Ddi_Bdd_t *myCex = Ddi_AigSatWithCex(next);
            Ddi_Free(myCex);
          }
          Ddi_Free(next);
          Ddi_BddNotAcc(pPrem);
          next = Ddi_BddCompose(pPrem,ps,delta);
          Ddi_BddNotAcc(pPrem);
          if (!Ddi_AigSatAnd(next,pPrem,p3)) {
            printf("closed component found\n");
          }
          Ddi_Free(next);

          next = Ddi_BddDup(p3);
          Ddi_BddComposeAcc(p3,ps,delta);
          Ddi_BddNotAcc(pPrem);
          Ddi_BddNotAcc(p3);
          if (!Ddi_AigSatAnd(next,pPrem,p3)) {
            printf("closed component found\n");
          }

          Ddi_Free(pPrem);
          Ddi_Free(next);
          Ddi_Free(p3);
        }

        for (i = Ddi_BddPartNum(p0)-1; i>0; i--) {
          Ddi_Bdd_t *p0_i = Ddi_BddPartRead(p0,i);
          Ddi_Bdd_t *p0_prev = Ddi_BddPartRead(p0,i-1);
          if (Ddi_BddSize(p0_prev)==1) {
            Ddi_BddAndAcc(p0_prev,p0_i);
            Ddi_BddPartRemove(p0,i);
          }
          else
            break;
        }
        for (i = 1, enPart=1; i < Ddi_BddPartNum(pinv); i++) {
          int j, k;
          Ddi_Free(newPinv);
          newPinv = Ddi_BddMakePartDisjVoid(ddiMgr);
          p1 = Ddi_AigPartitionTop(Ddi_BddPartRead(pinv, i), 1);
          if (!enPart) Ddi_BddSetAig(p1);
          for (j = 0; j < Ddi_BddPartNum(p0); j++) {
            for (k = 0; k < Ddi_BddPartNum(p1); k++) {
              Ddi_Bdd_t *p_jk = Ddi_BddAnd(Ddi_BddPartRead(p0, j),
                Ddi_BddPartRead(p1, k));

              Ddi_BddPartInsertLast(newPinv, p_jk);
              Ddi_Free(p_jk);
            }
          }
          Ddi_Free(p0);
          Ddi_Free(p1);
          p0 = Ddi_BddDup(newPinv);
          if (Ddi_BddPartNum(newPinv)>maxNewPart) enPart=0;
        }
        Ddi_Free(p0);
        Ddi_Free(pinv);
        pinv = newPinv;
        }
      }
      else if (pinv != NULL && Ddi_BddPartNum(pinv) == 2) {
	Ddi_Bddarray_t *myDelta = Ddi_BddarrayDup(delta);
	Ddi_Bdd_t *p0 = Ddi_BddPartRead(pinv,0);
	Ddi_Bdd_t *p1 = Ddi_BddPartRead(pinv,1);
	int np = Ddi_BddarrayNum(delta);
	//	Ddi_BddarrayRemove(myDelta,np-1);
	//        Ddi_AigStructRedRemAcc(p0, NULL);
	//        Ddi_AigStructRedRemAcc(p1, NULL);
	Ddi_Bdd_t *p1c = Ddi_AigPartitionTop(p1, 0);
	Ddi_Bdd_t *p0c = Ddi_AigPartitionTop(p0, 0);
	Ddi_BddPartSortBySizeAcc(p0c, 0);    // decreasing size	
	Ddi_BddPartSortBySizeAcc(p1c, 0);    // decreasing size	

	Ddi_Bdd_t *p10 = Ddi_BddPartRead(p1c,0);
	Ddi_Bdd_t *p00 = Ddi_BddPartRead(p0c,0);
	Ddi_Bdd_t *p01 = Ddi_BddPartRead(p0c,1);
	Ddi_Bdd_t *p1c0d = Ddi_AigPartitionTop(p10, 1);
	Ddi_Bdd_t *p0c0d = Ddi_AigPartitionTop(p00, 1);
	Ddi_Bdd_t *p0c1d = Ddi_AigPartitionTop(p01, 1);
	Ddi_BddPartSortBySizeAcc(p1c0d, 1);    // increasing size	
	Ddi_BddPartSortBySizeAcc(p0c0d, 1);    // increasing size	
	Ddi_BddPartSortBySizeAcc(p0c1d, 1);    // increasing size	
     	Ddi_BddPartRemove(p1c0d,6);
     	Ddi_BddPartRemove(p0c0d,6);
     	Ddi_BddPartRemove(p0c1d,1);
	Ddi_BddSetAig(p1c0d);
	Ddi_BddSetAig(p0c0d);
	Ddi_BddSetAig(p0c1d);
	Ddi_DataCopy(p10,p1c0d);
	Ddi_DataCopy(p00,p0c0d);
	Ddi_DataCopy(p01,p0c1d);
	//	Ddi_DataCopy(p10,Ddi_BddPartRead(p1c0d,5));
	//	Ddi_BddPartSortBySizeAcc(p1c, 0);    // decreasing size	
	//	Ddi_BddPartRemove(p0c,0);
     	Ddi_Free(p1c0d);
     	Ddi_Free(p0c0d);
     	Ddi_Free(p0c1d);
     	Ddi_BddSetAig(p0c);
     	Ddi_BddSetAig(p1c);
	//	Ddi_BddOrAcc(p1,p0c);
       	Ddi_DataCopy(p0,p0c);
       	Ddi_DataCopy(p1,p1c);
      	Ddi_BddSetAig(pinv);
      	Ddi_DataCopy(invarspec,pinv);
      	Ddi_Free(pinv);
	Ddi_Free(p0c);
	Ddi_Free(p1c);
	Ddi_Free(myDelta);
      }

      if (custom && pinv != NULL) {
        Ddi_Free(invarspec);
        invarspec = Ddi_BddMakeAig(pinv);
        Ddi_Free(pinv);
      }
      else if (pinv != NULL) {

        int n;
        Ddi_Varset_t *pivars = Ddi_VarsetMakeFromArray(pi);

        Ddi_Free(invarspec);
        if (opt->pre.specDecompIndex >= Ddi_BddPartNum(pinv)) {
          opt->pre.specDecompIndex = Ddi_BddPartNum(pinv) - 1;
        }
        invarspec = Ddi_BddPartExtract(pinv, opt->pre.specDecompIndex);
        if (0 && invar != NULL) {
          Ddi_BddSetAig(pinv);
          Ddi_BddNotAcc(pinv);
          Ddi_BddAndAcc(invar, pinv);
        }
        Ddi_Free(pinv);
#if 0
        pinv = Ddi_AigPartitionTop(invarspec, 0);
        for (i = 0; i < Ddi_BddPartNum(pinv); i++) {
          Ddi_Bdd_t *pconstr;

          pconstr = Ddi_BddPartRead(pinv, i);
          //Ddi_BddNotAcc(pconstr);
          if (Ddi_AigSatAnd(Fsm_MgrReadInitBDD(fsmMgr), pconstr, NULL)) {
            Ddi_BddAndAcc(invar, pconstr);
            //Ddi_BddAndAcc(Fsm_MgrReadInitBDD (fsmMgr), pconstr);
            n++;
          }
        }
        Ddi_Free(pinv);
#endif
        Ddi_Free(pivars);

        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "spec decomp: %d -> %d\n",
            size, Ddi_BddSize(invarspec))
          );
      }
      // opt->pre.specDecomp = -1;
    }

    if (!opt->mc.compl_invarspec) {
      Ddi_BddNotAcc(invarspec);
    }

    if (addLatch > 1) {
      int nl = Fsm_MgrReadNL(fsmMgr);
      Ddi_Var_t *lambdaPs =
        Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
      if (lambdaPs != NULL) {
        if (nl>0 && (lambdaPs == Ddi_VararrayRead(ps, nl - 1))) {
          Ddi_Var_t *lambdaNs =
            Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS$NS");
          if (lambdaNs == Ddi_VararrayRead(ns, nl - 1)) {
            addLatch = 1;
            Ddi_VarAttachName(lambdaNs, "PDT_BDD_INVARSPEC_VAR$NS");
            Fsm_MgrSetPdtSpecVar(fsmMgr, lambdaPs);
          }
        }
      }
    }

    if (addLatch > 1) {
      int nl = Fsm_MgrReadNL(fsmMgr);

      Ddi_Bdd_t *newLambdaBdd, *property;
      Ddi_Var_t *lambdaPs =
        Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
      Ddi_Var_t *lambdaNs =
        Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$NS");
      if (lambdaPs == NULL) {
        lambdaPs = Ddi_VarNewAtLevel(ddiMgr, 0);
        Ddi_VarAttachName(lambdaPs, "PDT_BDD_INVARSPEC_VAR$PS");
      }
      if (lambdaNs == NULL) {
        int xlevel = Ddi_VarCurrPos(lambdaPs) + 1;

        lambdaNs = Ddi_VarNewAtLevel(ddiMgr, xlevel);
        Ddi_VarAttachName(lambdaNs, "PDT_BDD_INVARSPEC_VAR$NS");
      }
      //Ddi_Var_t *lambdaPs = Ddi_VarNew(ddiMgr);
      //Ddi_Var_t *lambdaNs = Ddi_VarNew(ddiMgr);
      Fsm_MgrSetPdtSpecVar(fsmMgr, lambdaPs);
      //      Ddi_VarMakeGroup (ddiMgr, lambdaPs, 2);
#if 0
      Ddi_VararrayInsertLast(Fsm_MgrReadVarPS(fsmMgr), lambdaPs);
      Ddi_VararrayInsertLast(Fsm_MgrReadVarNS(fsmMgr), lambdaNs);

      if (!Ddi_BddIsMono(invarspec)) {
        newLambdaBdd = Ddi_BddMakeLiteralAig(lambdaPs, 1);
      } else {
        newLambdaBdd = Ddi_BddMakeLiteral(lambdaPs, 1);
      }
      Ddi_BddAndAcc(Fsm_MgrReadInitBDD(fsmMgr), newLambdaBdd);

      /* whenever property is violated, it keeps its state */
      if (opt->mc.compl_invarspec) {
        Ddi_BddNotAcc(invarspec);
        opt->mc.compl_invarspec = 0;
      }
      if (opt->trav.cntReachedOK) {
        property = Ddi_BddDup(invarspec);
      } else {
        if (Ddi_BddIsPartConj(invarspec)) {
          property = Ddi_BddDup(invarspec);
          Ddi_BddPartInsertLast(invarspec, newLambdaBdd);
        } else {
          property = Ddi_BddAnd(invarspec, newLambdaBdd);
          //          property = Ddi_BddDup(invarspec);
        }
      }
      Ddi_BddarrayInsertLast(Fsm_MgrReadDeltaBDD(fsmMgr), property);

      Ddi_BddarrayWrite(Fsm_MgrReadLambdaBDD(fsmMgr), 0, newLambdaBdd);

      if (tr != NULL) {
        Ddi_Bdd_t *trNew, *nsLit;
        Tr_Mgr_t *trMgr = Tr_TrMgr(tr);

        Ddi_VararrayInsertLast(Tr_MgrReadPS(trMgr), lambdaPs);
        Ddi_VararrayInsertLast(Tr_MgrReadNS(trMgr), lambdaNs);
        if (Ddi_BddIsAig(invarspec)) {
          nsLit = Ddi_BddMakeLiteralAig(lambdaNs, 1);
        } else {
          nsLit = Ddi_BddMakeLiteral(lambdaNs, 1);
        }
        trNew = Ddi_BddXnor(property, nsLit);
        Ddi_BddPartInsert(Tr_TrBdd(tr), 0, trNew);
        Ddi_Free(nsLit);
        Ddi_Free(trNew);
      }
      Ddi_Free(property);
      Ddi_Free(invarspec);
      invarspec = newLambdaBdd;
#endif
    }

    if (0) {
      /* disable constraint */
      Ddi_Bdd_t *myOne = Ddi_BddMakeConstAig(ddiMgr, 1);

      Ddi_DataCopy(invar, myOne);
      Ddi_Free(myOne);
    }

    if (invar != NULL /* && !Ddi_BddIsOne(invar) */  && addLatch) {
      int nl = Fsm_MgrReadNL(fsmMgr);
      Ddi_Var_t *lambdaPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

      if (lambdaPs != NULL) {
        if (nl>1 && (lambdaPs == Ddi_VararrayRead(ps, nl - 2))) {
          Ddi_Var_t *lambdaNs =
            Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS$NS");
          if (lambdaNs == Ddi_VararrayRead(ns, nl - 2)) {
            addLatch = 0;
            Ddi_VarAttachName(lambdaNs, "PDT_BDD_INVAR_VAR$NS");
            Fsm_MgrSetPdtConstrVar(fsmMgr, lambdaPs);
          }
        }
      }
    }

    if (invar != NULL /* && !Ddi_BddIsOne(invar) */  && addLatch) {
      int nl = Ddi_VararrayNum(ps);
      int iProp = Ddi_BddarrayNum(Fsm_MgrReadDeltaBDD(fsmMgr)) - 1;

      Ddi_Bdd_t *newLambdaBdd, *myInvar, *property;
      Ddi_Var_t *lambdaPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
      Ddi_Var_t *lambdaNs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$NS");

      if (lambdaPs == NULL) {
        lambdaPs = Ddi_VarNewAtLevel(ddiMgr, 0);
        Ddi_VarAttachName(lambdaPs, "PDT_BDD_INVAR_VAR$PS");
      }
      if (lambdaNs == NULL) {
        int xlevel = Ddi_VarCurrPos(lambdaPs) + 1;

        lambdaNs = Ddi_VarNewAtLevel(ddiMgr, xlevel);
        Ddi_VarAttachName(lambdaNs, "PDT_BDD_INVAR_VAR$NS");
      }
      Fsm_MgrSetPdtConstrVar(fsmMgr, lambdaPs);
      // Ddi_VarMakeGroup (ddiMgr, lambdaPs, 2);
#if 0
      Ddi_VararrayInsert(Fsm_MgrReadVarPS(fsmMgr), nl - 1, lambdaPs);
      Ddi_VararrayInsert(Fsm_MgrReadVarNS(fsmMgr), nl - 1, lambdaNs);
#endif
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "|Constraint|=%d nodes.\n", Ddi_BddSize(invar)));
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "|Property|=%d nodes.\n", Ddi_BddSize(invarspec)));


      if (1) {
        int nrem = 0;
        Ddi_Bdd_t *a = Ddi_AigPartitionTop(invar, 0);
        Ddi_Varset_t *psv = Ddi_VarsetMakeFromArray(ps);
        Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(pi);

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
          Ddi_DataCopy(invar, a);
        }
        Ddi_Free(a);
      }
#if 0
      if (!Ddi_BddIsMono(invar)) {
        newLambdaBdd = Ddi_BddMakeLiteralAig(lambdaPs, 1);
        myInvar = Ddi_BddDup(invar);
      } else {
        newLambdaBdd = Ddi_BddMakeLiteral(lambdaPs, 1);
        myInvar = Ddi_BddMakeAig(invar);
      }
      Ddi_BddAndAcc(Fsm_MgrReadInitBDD(fsmMgr), newLambdaBdd);

      /* whenever constraint is violated, remember it */
      Ddi_BddAndAcc(myInvar, newLambdaBdd);
      Ddi_BddarrayInsert(Fsm_MgrReadDeltaBDD(fsmMgr), nl - 1, myInvar);

      iProp++;
      property = Ddi_BddarrayRead(Fsm_MgrReadDeltaBDD(fsmMgr), iProp);
      Ddi_BddNotAcc(newLambdaBdd);

      /* saturate reachable states with out-of-invar-states */
      //      Ddi_BddOrAcc(Fsm_MgrReadInitBDD (fsmMgr), newLambdaBdd);

      if (customConstraint) {
        // BEWARE OF FAILURE WHEN GOING OUT OF CONSTRAINT
        //      extern Ddi_Var_t *constraintGuard;

        if (opt->mc.compl_invarspec) {
          Ddi_BddDiffAcc(invarspec, newLambdaBdd);
          Ddi_BddDiffAcc(property, newLambdaBdd);
        } else {
          Ddi_BddOrAcc(invarspec, newLambdaBdd);
          Ddi_BddOrAcc(property, newLambdaBdd);
        }
        //      constraintGuard = lambdaPs;
      } else {
        Ddi_BddOrAcc(property, newLambdaBdd);
      }
      Ddi_Free(newLambdaBdd);

      if (tr != NULL) {
        Ddi_Bdd_t *trNew, *nsLit;
        Tr_Mgr_t *trMgr = Tr_TrMgr(tr);

        Ddi_VararrayInsertLast(Tr_MgrReadPS(trMgr), lambdaPs);
        Ddi_VararrayInsertLast(Tr_MgrReadNS(trMgr), lambdaNs);
        if (Ddi_BddIsAig(invarspec)) {
          nsLit = Ddi_BddMakeLiteralAig(lambdaNs, 1);
        } else {
          nsLit = Ddi_BddMakeLiteral(lambdaNs, 1);
        }
        trNew = Ddi_BddXnor(myInvar, nsLit);
        Ddi_BddPartInsert(Tr_TrBdd(tr), 0, trNew);
        Ddi_Free(nsLit);
        Ddi_Free(trNew);
      }
      Ddi_Free(myInvar);
      if (0) {
        /* ignore constraint */
        Ddi_Bdd_t *iOne = Ddi_BddMakeConstAig(ddiMgr, 1);

        Ddi_DataCopy(invar, iOne);
        Ddi_Free(iOne);
      }
#endif
    }
  }


  if (opt->mc.compl_invarspec) {
    Ddi_BddNotAcc(invarspec);
    opt->mc.compl_invarspec = 0;
  }

  if (opt->pre.specEn != 0) {
    Ddi_Vararray_t *newPis = Ddi_VararrayAlloc(ddiMgr,0);
    Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
    Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
    Ddi_Varset_t *psVars = Ddi_VarsetMakeFromArray(ps);

    while (opt->pre.specEn != 0) {
      Ddi_Bdd_t *deltaTarget0 = Ddi_BddNot(invarspec);
      Ddi_Bdd_t *deltaTarget1 = NULL;
      Ddi_Bdd_t *deltaTarget2 = NULL;
      int doExist = opt->pre.specEn < 0;

      if (opt->pre.specEn <= -100) {
	Ddi_Var_t *v0 = Ddi_VarFromName(ddiMgr, "l148");
	Ddi_Var_t *v1 = Ddi_VarFromName(ddiMgr, "l150");
	Ddi_Var_t *v2 = Ddi_VarFromName(ddiMgr, "l152");
	Ddi_Var_t *v3 = Ddi_VarFromName(ddiMgr, "l156");
	Ddi_Var_t *v4 = Ddi_VarFromName(ddiMgr, "l168");
	Ddi_Var_t *v5 = Ddi_VarFromName(ddiMgr, "l302");
	Ddi_Bdd_t *l0 = Ddi_BddMakeLiteralAig(v0,1);
	Ddi_Bdd_t *l1 = Ddi_BddMakeLiteralAig(v1,1);
	Ddi_Bdd_t *l2 = Ddi_BddMakeLiteralAig(v2,1);
	Ddi_Bdd_t *l3 = Ddi_BddMakeLiteralAig(v3,1);
	Ddi_Bdd_t *l4 = Ddi_BddMakeLiteralAig(v4,1);
	Ddi_Bdd_t *l5 = Ddi_BddMakeLiteralAig(v5,1);
	deltaTarget1 = Ddi_BddDup(deltaTarget0);
	Ddi_BddAndAcc(deltaTarget1,l0);
	//     	Ddi_BddAndAcc(deltaTarget1,l1);
     	Ddi_BddDiffAcc(deltaTarget1,l1);
	Ddi_BddDiffAcc(deltaTarget1,l2);
	Ddi_BddDiffAcc(deltaTarget1,l3);
	Ddi_BddDiffAcc(deltaTarget1,l4);
	//	Ddi_BddAndAcc(deltaTarget1,l5);
	Ddi_Free(l0);
	Ddi_Free(l1);
	Ddi_Free(l2);
	Ddi_Free(l3);
	Ddi_Free(l4);
	Ddi_Free(l5);
        Ddi_AigStructRedRemAcc(deltaTarget1, NULL);
	opt->pre.specEn = -1;
      }
      else if (opt->pre.specEn <= -10) {
	Ddi_Vararray_t *vA = Ddi_BddSuppVararray(deltaTarget0);
	int i, nc, maxc=10;
	for (i=nc=0; i<Ddi_VararrayNum(vA) && nc<maxc; i++) {
	  Ddi_Var_t *v_i = Ddi_VararrayRead(vA,i);
	  Ddi_Bdd_t *c0 = Ddi_BddMakeLiteralAig(v_i,0);
	  Ddi_Bdd_t *c1 = Ddi_BddMakeLiteralAig(v_i,1);
	  deltaTarget1 = Ddi_BddDup(deltaTarget0);
	  deltaTarget2 = Ddi_BddDup(deltaTarget0);
	  Ddi_AigAndCubeAcc(deltaTarget1, c0);
	  Ddi_AigAndCubeAcc(deltaTarget2, c1);
	  Ddi_Free(c0);
	  Ddi_Free(c1);
	  if (!Ddi_BddIsZero(deltaTarget1) && 
	      (Ddi_BddSize(deltaTarget1) < 0.8*Ddi_BddSize(deltaTarget0))) {
	    Ddi_DataCopy(deltaTarget0,deltaTarget2); nc++;
	  }
	  if (!Ddi_BddIsZero(deltaTarget2) && 
	      (Ddi_BddSize(deltaTarget2) < 0.8*Ddi_BddSize(deltaTarget0))) {
	    Ddi_DataCopy(deltaTarget0,deltaTarget1); nc++;
	  }
	  Ddi_Free(deltaTarget1);
	  Ddi_Free(deltaTarget2);
	}
	deltaTarget1 = Ddi_BddDup(deltaTarget0);
	Ddi_Free(vA);
	opt->pre.specEn = -1;
      }
      else if (doExist && specEnIter++ < 2) {
	float lazyRate = Ddi_MgrReadAigLazyRate(ddiMgr);
	Ddi_Bdd_t *ref = Ddi_BddDup(deltaTarget0);

	Ddi_MgrSetAigLazyRate(ddiMgr, 1.05);
	Ddi_AigExistProjectAcc(deltaTarget0, psVars, NULL, 3, 0, 10);
	Ddi_MgrSetAigLazyRate(ddiMgr, lazyRate);
#if 0
	Ddi_BddNotAcc(deltaTarget0);
	Ddi_AigOptByMonotoneCoreAcc (ref,deltaTarget0,NULL,0,-1.0);
	Ddi_BddNotAcc(deltaTarget0);
	if (Ddi_BddSize(ref) < Ddi_BddSize(deltaTarget0)) {
	  Ddi_DataCopy(deltaTarget0,ref);
	}     
	Ddi_Free(ref);
#endif
      }
      if (1) {
	Ddi_Var_t *pvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
	char suffix[4];
	Ddi_Vararray_t *vA, *newPiVars;
	Ddi_Bddarray_t *newPiLits;

	vA = Ddi_BddSuppVararray(deltaTarget0);
	sprintf(suffix, "%d", opt->pre.specEn);
	//	Ddi_VararrayDiffAcc(vA, ps);
	Ddi_VararrayIntersectAcc(vA, pi);
	newPiVars = Ddi_VararrayMakeNewVars(vA, "PDT_TE_STUB_PI", suffix, 1);
	newPiLits = Ddi_BddarrayMakeLiteralsAig(newPiVars, 1);
	deltaTarget1 = Ddi_BddCompose(deltaTarget0, vA, newPiLits);
	Ddi_VararrayAppend(newPis, newPiVars);

	deltaTarget2 = Ddi_BddDup(deltaTarget1);
	Ddi_BddCofactorAcc(deltaTarget2,pvarPs,1);
	Ddi_BddComposeAcc(deltaTarget2, ps, delta);

	Ddi_Free(newPiLits);
	Ddi_Free(newPiVars);
	Ddi_Free(vA);
      }
      if (0) {
	if (Trav_PersistentTarget(fsmMgr, NULL, deltaTarget0, ps, ns, delta)) {
	  printf("target bwd reachable from target\n");
	} else if (Trav_PersistentTarget(NULL, NULL, deltaTarget1, ps, ns,
					 delta)) {
	  Ddi_BddDiffAcc(deltaTarget2, deltaTarget1);
	  printf("target bwd reachable from target\n");
	}
      }
      // Ddi_BddOrAcc(deltaTarget1,deltaTarget2);

      if (1) {
	int i;
	Ddi_Vararray_t *vA, *vA2;
	char name[1000];
	vA = Ddi_BddSuppVararray(deltaTarget0);
	Ddi_VararrayDiffAcc(vA, ps);
	vA2 = Ddi_VararrayDup(vA);
	for (i=0; i<Ddi_VararrayNum(vA2); i++) {
	  Ddi_Var_t *v2,*v = Ddi_VararrayRead(vA2,i);
	  if (strncmp(Ddi_VarName(v),"PDT_TE_STUB_PI",14)==0) {
	    char *s; int t;
	    strcpy(name,Ddi_VarName(v));
	    for (s=name+strlen(name);*s!='_';s--);
	    s++;
	    Pdtutil_Assert(s>name,"problem processing var name");
	    t = atoi(s);
	    sprintf(s,"%d",t-1);
	    v2 = Ddi_VarFromName(ddiMgr, name);
	    if (v2==NULL) {
	      v2 = Ddi_VarFindOrAdd(ddiMgr,name,0);
	      Ddi_VararrayInsertLast(newPis,v2);
	    }
	    Ddi_VararrayWrite(vA2,i,v2);
	  }
	}
	Ddi_BddSubstVarsAcc(deltaTarget0, vA, vA2);
	Ddi_BddDiffAcc(deltaTarget2, deltaTarget0);
	Ddi_Free(vA);
	Ddi_Free(vA2);
      }

      Ddi_DataCopy(deltaTarget1, deltaTarget2);
      if (abs(opt->pre.specEn) == 1) {
	Ddi_MgrSetAigAbcOptLevel(ddiMgr, opt->common.aigAbcOpt);
	ddiAbcOptAcc(deltaTarget1, -1.0);
	//	DdiAigRedRemovalOdcAcc (deltaTarget1,NULL,-1,-1.0);
	// DdiAigRedRemovalOdcByEqClasses (deltaTarget1, NULL, -1, 0, -1.0);
	ddiAbcOptAcc(deltaTarget1, -1.0);
      }
      Ddi_BddNotAcc(deltaTarget1);

      Ddi_DataCopy(invarspec, deltaTarget1);

      Ddi_Free(deltaTarget0);
      Ddi_Free(deltaTarget1);
      Ddi_Free(deltaTarget2);

      if (doExist) {
	opt->pre.specEn++;
      } else {
	opt->pre.specEn--;
      }
    }
    Ddi_VararrayAppend(pi,newPis);
    Ddi_Free(newPis);
    Ddi_Free(psVars);
  }

  if (0) {
    int i;

    for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
      Ddi_Varset_t *s = Ddi_BddSupp(Ddi_BddarrayRead(delta, i));

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "D[%d] = %d\n", i, Ddi_VarsetNum(s)));

      Ddi_Free(s);
    }
  }

  return (invarspec);

}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
FbvSelectHeuristic(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt,
  int phase
)
{
  int heuristic = 0;
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  int nstate = Ddi_VararrayNum(ps);
  int ninput = Ddi_VararrayNum(pi);
  int isSec = 0;
  int doPrint = 1;
  int ngate = Ddi_BddarraySize(delta);
  int constrSize = Ddi_BddSize(Fsm_MgrReadConstraintBDD(fsmMgr));
  Ddi_Var_t *vc = Ddi_VarFromName(ddm,"AIGUNCONSTRAINT_INVALID_STATE");
  int hasConstraints = vc!=NULL || constrSize>1;
  
  if (opt->expt.expertLevel <= 0) {
    /* disabled - just return */
    return Fbv_HeuristicNone_c;
  }

  heuristic = Fbv_HeuristicNone_c;

  Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
    if (doPrint)
      printf
        ("HEURISTIC selection [%d] - FSM stats: L: %d - I: %d - Nodes: %d\n",
        phase, Ddi_VararrayNum(ps), Ddi_VararrayNum(pi), ngate);
  }

  if (!hasConstraints &&
      (nstate > 12000 || ninput > 8000 || ngate > 150000)) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      fprintf(stdout, "HEURISTIC strategy selection: BMC-BIG\n\n");
    }
    heuristic = Fbv_HeuristicBmcBig_c;
    opt->pre.doCoi = 0;
  }

  if (phase == 0) {
    return heuristic;
  }

  if (opt->pre.nEqChecks > 0
    && ((float)opt->pre.nEqChecks) / opt->pre.nTotChecks > 0.2) {
    isSec = 1;
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      if (doPrint)
        printf("HEURISTIC selection [%d] - SEC problem: %d/%d eq checks\n",
          phase, opt->pre.nEqChecks, opt->pre.nTotChecks);
    }
  }

  if (opt->pre.clkVarFound != NULL) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      if (doPrint)
        printf
          ("HEURISTIC selection [%d] - clock found: %s (two phase red possible)\n",
          phase, Ddi_VarName(opt->pre.clkVarFound));
    }
  }

  if (opt->pre.terminalSccNum != 0) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      if (doPrint)
        printf("HEURISTIC selection [%d] - %d terminal Scc found\n",
          phase, abs(opt->pre.terminalSccNum));
    }
  }

  if (opt->pre.peripheralLatchesNum > 0) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      if (doPrint)
        printf("HEURISTIC selection [%d] - %d peripheral latches found\n",
          phase, opt->pre.peripheralLatchesNum);
    }
  }

  if (opt->pre.impliedConstrNum > 0) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      if (doPrint)
        printf("HEURISTIC selection [%d] - %d implied constr found\n", phase,
          opt->pre.impliedConstrNum);
    }
  }

  if (opt->pre.relationalNs > 0) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      if (doPrint)
        printf
          ("HEURISTIC selection [%d] - %d relational latch candidates found\n",
          phase, opt->pre.relationalNs);
    }
  }

  if (opt->pre.specConjNum > 1) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      if (doPrint)
        printf("HEURISTIC selection [%d] - %d conj components in property\n",
          phase, opt->pre.specConjNum);
    }
  }

  if (opt->pre.specDisjNum > 1) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      if (doPrint)
        printf("HEURISTIC selection [%d] - %d disj components in property\n",
          phase, opt->pre.specDisjNum);
    }
  }

  if (opt->trav.hints != NULL) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      if (doPrint)
        printf("HEURISTIC selection [%d] - auto hints size %d\n",
          phase, Ddi_BddSize(Ddi_BddarrayRead(opt->trav.hints, 0)));
    }
  }

  if ((opt->pre.clkVarFound != NULL ||
      (nstate > 100 && opt->pre.relationalNs > nstate * 0.8)) &&
    opt->pre.terminalSccNum == 1 && opt->pre.impliedConstr == 1) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      fprintf(stdout, "HEURISTIC strategy selection: INTEL\n\n");
    }
    heuristic = Fbv_HeuristicIntel_c;
  } else if (opt->pre.terminalSccNum == -1 && opt->pre.impliedConstrNum == 0
    && isSec && opt->pre.specDisjNum == 1 && opt->pre.specConjNum < 5
    && opt->pre.specPartMaxSize < 4 && opt->pre.specPartMinSize > 2) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      fprintf(stdout, "HEURISTIC strategy selection: QUEUE\n\n");
    }
    heuristic = Fbv_HeuristicQueue_c;
  } else if ((opt->pre.relationalNs > nstate * 0.8) &&
    opt->pre.terminalSccNum == -1 && opt->pre.impliedConstr == 1) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      fprintf(stdout, "HEURISTIC strategy selection: BEEM\n\n");
    }
    heuristic = Fbv_HeuristicBeem_c;
  } else if (opt->pre.terminalSccNum == 1 && opt->pre.impliedConstrNum == 0
    && !isSec && (opt->pre.specDisjNum == 2 || opt->pre.specConjNum > 100)) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      fprintf(stdout, "HEURISTIC strategy selection: DME\n\n");
    }
    heuristic = Fbv_HeuristicDme_c;
  } else if (opt->pre.terminalSccNum == 0 && opt->pre.impliedConstrNum == 0
    && !isSec && opt->pre.specDisjNum > 5 && nstate > 100 && nstate < 150
    && opt->pre.specPartMaxSize > 10 && opt->pre.specPartMinSize == 1) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      fprintf(stdout, "HEURISTIC strategy selection: NEEDHAM\n\n");
    }
    heuristic = Fbv_HeuristicNeedham_c;
  } else if (opt->pre.nEqChecks > 0
    && ((float)opt->pre.nEqChecks) / opt->pre.nTotChecks > 0.2
    && (opt->pre.peripheralLatchesNum == 0 || ngate <= 10000)) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      fprintf(stdout, "HEURISTIC strategy selection: SEC\n\n");
    }
    heuristic = Fbv_HeuristicSec_c;
    if (nstate > 500) {
      opt->pre.ternarySim = 3;
      opt->pre.indEqDepth = nstate <= 900 ? 0 : (nstate > 2500 ? -1 : 0);   //nstate>900;
      opt->pre.twoPhaseRed = 1;
    }
  } else if (opt->pre.clkVarFound != NULL || ngate > 30000) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      fprintf(stdout, "HEURISTIC strategy selection: IBM\n\n");
    }
    heuristic = Fbv_HeuristicIbm_c;
  } else if (nstate <= 150 && opt->pre.impliedConstrNum == 1
    && opt->pre.specDisjNum >= 10) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      fprintf(stdout, "HEURISTIC strategy selection: PDT_SW\n\n");
    }
    heuristic = Fbv_HeuristicPdtSw_c;
  } else if (nstate <= 75) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      fprintf(stdout, "HEURISTIC strategy selection: FULL-BDD\n\n");
    }
    heuristic = Fbv_HeuristicBddFull_c;
  } else if (nstate < 150 || nstate + ninput < 250) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      fprintf(stdout, "HEURISTIC strategy selection: BDD\n\n");
    }
    heuristic = Fbv_HeuristicBdd_c;
  } else if (opt->pre.terminalSccNum == 1 && opt->pre.impliedConstrNum == 0
    && !isSec && (nstate > 100 && opt->pre.relationalNs > nstate * 0.5)) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      fprintf(stdout, "HEURISTIC strategy selection: RELATIONAL\n\n");
    }
    heuristic = Fbv_HeuristicRelational_c;
  } else if (!isSec && (hasConstraints ||
                        (nstate > 1000 && ninput < 100 && ngate>1000))) {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      fprintf(stdout, "HEURISTIC strategy selection: CPU\n\n");
    }
    heuristic = Fbv_HeuristicRelational_c;
  } else {
    Pdtutil_VerbosityMgrIf(fsmMgr, Pdtutil_VerbLevelUsrMin_c) {
      fprintf(stdout, "HEURISTIC strategy selection: OTHER\n\n");
    }
    heuristic = Fbv_HeuristicOther_c;
  }

  if (opt->expt.expertLevel == 1) {
    exit(1);
  }

  return heuristic;
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
writeCoiShells(
  char *wrCoi,
  Ddi_Varsetarray_t * coi,
  Ddi_Vararray_t * vars
)
{
  Ddi_Varset_t *curr, *old, *New, *varSet;
  FILE *fp = fopen(wrCoi, "w");
  int i;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(coi);

  varSet = Ddi_VarsetMakeFromArray(vars);
  old = NULL;
  for (i = 0; i < Ddi_VarsetarrayNum(coi); i++) {
    curr = Ddi_VarsetarrayRead(coi, i);
    if (old == NULL) {
      New = Ddi_VarsetDup(curr);
    } else {
      New = Ddi_VarsetDiff(curr, old);
    }
    Ddi_VarsetIntersectAcc(New, varSet);
    old = curr;
    Ddi_VarsetPrint(New, 0, NULL, fp);
    if (1 || i < Ddi_VarsetarrayNum(coi) - 1) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c, fprintf(fp, "#\n"));
    }

    Ddi_Free(New);
  }

  fclose(fp);
  Ddi_Free(varSet);
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
FbvSetTravMgrOpt(
  Trav_Mgr_t * travMgr,
  Fbv_Globals_t * opt
)
{
  Trav_FromSelect_e fromSelect = Trav_FromSelectBest_c;

  if (opt->trav.fromSel == 'n') {
    fromSelect = Trav_FromSelectNew_c;
  } else if (opt->trav.fromSel == 'r') {
    fromSelect = Trav_FromSelectReached_c;
  } else if (opt->trav.fromSel == 't') {
    fromSelect = Trav_FromSelectTo_c;
  }

  /* general */
  if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
    ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
    Trav_MgrSetVerbosity(travMgr, Pdtutil_VerbLevel_e(opt->verbosity));
  }
  Trav_MgrSetOption(travMgr, Pdt_TravClk_c, pchar, opt->common.clk);
  Trav_MgrSetOption(travMgr, Pdt_TravDoCex_c, inum, opt->trav.cex);
  Trav_MgrSetAssertFlag(travMgr, -1);

  /* unused */
  Trav_MgrSetConstrLevel(travMgr, opt->trav.travConstrLevel);
  //Trav_MgrSetOption(travMgr, Pdt__c, inum, opt->trav.);

  /* aig */
  Trav_MgrSetOption(travMgr, Pdt_TravSatSolver_c, pchar,
    opt->common.satSolver);
  Trav_MgrSetOption(travMgr, Pdt_TravAbcOptLevel_c, inum,
    opt->common.aigAbcOpt);
  Trav_MgrSetOption(travMgr, Pdt_TravTargetEn_c, inum, opt->trav.targetEn);
  Trav_MgrSetDynAbstr(travMgr, opt->trav.dynAbstr);
  Trav_MgrSetDynAbstrInitIter(travMgr, opt->trav.dynAbstrInitIter);
  Trav_MgrSetImplAbstr(travMgr, opt->trav.implAbstr);
  Trav_MgrSetTernaryAbstr(travMgr, opt->trav.ternaryAbstr);
  Trav_MgrSetAbstrRef(travMgr, opt->trav.abstrRef);
  Trav_MgrSetAbstrRefGla(travMgr, opt->trav.abstrRefGla);
  Trav_MgrSetOption(travMgr, Pdt_TravAbstrRefItp_c, inum,
    opt->trav.abstrRefItp);
  Trav_MgrSetOption(travMgr, Pdt_TravAbstrRefItpMaxIter_c, inum,
    opt->trav.abstrRefItpMaxIter);
  Trav_MgrSetOption(travMgr, Pdt_TravTrAbstrItp_c, inum,
    opt->trav.trAbstrItp);
  Trav_MgrSetOption(travMgr, Pdt_TravTrAbstrItpMaxFwdStep_c, inum,
    opt->trav.trAbstrItpMaxFwdStep);
  Trav_MgrSetOption(travMgr, Pdt_TravTrAbstrItpLoad_c, pchar,
    opt->trav.trAbstrItpLoad);
  Trav_MgrSetOption(travMgr, Pdt_TravTrAbstrItpStore_c, pchar,
    opt->trav.trAbstrItpStore);
  Trav_MgrSetOption(travMgr, Pdt_TravStoreAbstrRefRefinedVars_c, pchar,
    opt->trav.abstrRefStore);
  Trav_MgrSetInputRegs(travMgr, opt->trav.inputRegs);
  Trav_MgrSetSelfTuningLevel(travMgr, opt->trav.travSelfTuning);

  Trav_MgrSetOption(travMgr, Pdt_TravLazyTimeLimit_c, fnum,
    opt->trav.lazyTimeLimit);
  Trav_MgrSetOption(travMgr, Pdt_TravLemmasTimeLimit_c, fnum,
    opt->trav.lemmasTimeLimit);

  /* itp */
  Trav_MgrSetItpBdd(travMgr, opt->trav.itpBdd);
  Trav_MgrSetItpPart(travMgr, opt->trav.itpPart);
  Trav_MgrSetItpAppr(travMgr, opt->trav.itpAppr);
  Trav_MgrSetItpRefineCex(travMgr, opt->trav.itpRefineCex);
  Trav_MgrSetOption(travMgr, Pdt_TravItpPartCone_c, inum, opt->trav.itpPartCone);
  Trav_MgrSetOption(travMgr, Pdt_TravItpStructAbstr_c, inum,
    opt->trav.itpStructAbstr);
  Trav_MgrSetOption(travMgr, Pdt_TravItpNew_c, inum, opt->trav.itpNew);
  Trav_MgrSetOption(travMgr, Pdt_TravItpExactBoundDouble_c, inum, opt->trav.itpExactBoundDouble);
  Trav_MgrSetItpExact(travMgr, opt->trav.itpExact);
  Trav_MgrSetItpInductiveTo(travMgr, opt->trav.itpInductiveTo);
  Trav_MgrSetItpInnerCones(travMgr, opt->trav.itpInnerCones);
  Trav_MgrSetItpInitAbstr(travMgr, opt->trav.itpInitAbstr);
  Trav_MgrSetItpEndAbstr(travMgr, opt->trav.itpEndAbstr);
  Trav_MgrSetItpReuseRings(travMgr, opt->trav.itpReuseRings);
  Trav_MgrSetItpTuneForDepth(travMgr, opt->trav.itpTuneForDepth);
  Trav_MgrSetItpBoundkOpt(travMgr, opt->trav.itpBoundkOpt);
  Trav_MgrSetItpConeOpt(travMgr, opt->trav.itpConeOpt);
  Trav_MgrSetItpForceRun(travMgr, opt->trav.itpForceRun);
  Trav_MgrSetItpUseReached(travMgr, opt->trav.itpUseReached);
  Trav_MgrSetItpCheckCompleteness(travMgr, opt->trav.itpCheckCompleteness);
  Trav_MgrSetItpConstrLevel(travMgr, opt->trav.itpConstrLevel);
  Trav_MgrSetItpGenMaxIter(travMgr, opt->trav.itpGenMaxIter);
  Trav_MgrSetItpEnToPlusImg(travMgr, opt->trav.itpEnToPlusImg);
  Trav_MgrSetItpShareReached(travMgr, opt->trav.itpShareReached);
  Trav_MgrSetOption(travMgr, Pdt_TravItpTrAbstr_c, inum,
    opt->trav.itpTrAbstr);
  Trav_MgrSetOption(travMgr, Pdt_TravItpMaxStepK_c, inum,
    opt->trav.itpMaxStepK);
  Trav_MgrSetOption(travMgr, Pdt_TravItpUsePdrReached_c, inum,
    opt->trav.itpUsePdrReached);
  Trav_MgrSetOption(travMgr, Pdt_TravItpGfp_c, inum, opt->trav.itpGfp);
  Trav_MgrSetOption(travMgr, Pdt_TravItpWeaken_c, inum, opt->trav.itpWeaken);
  Trav_MgrSetOption(travMgr, Pdt_TravItpStrengthen_c, inum, opt->trav.itpStrengthen);
  Trav_MgrSetOption(travMgr, Pdt_TravItpReImg_c, inum, opt->trav.itpReImg);
  Trav_MgrSetOption(travMgr, Pdt_TravItpFromNewLevel_c, inum, opt->trav.itpFromNewLevel);
  Trav_MgrSetOption(travMgr, Pdt_TravItpRpm_c, inum, opt->trav.itpRpm);
  Trav_MgrSetOption(travMgr, Pdt_TravItpTimeLimit_c, inum,
    opt->expt.itpTimeLimit);
  Trav_MgrSetOption(travMgr, Pdt_TravItpPeakAig_c, inum, opt->expt.itpPeakAig);

  /* igr */
  Trav_MgrSetIgrSide(travMgr, opt->trav.igrSide);
  Trav_MgrSetOption(travMgr, Pdt_TravIgrItpSeq_c, inum, opt->trav.igrItpSeq);
  Trav_MgrSetOption(travMgr, Pdt_TravIgrRestart_c, inum, opt->trav.igrRestart);
  Trav_MgrSetOption(travMgr, Pdt_TravIgrRewind_c, inum, opt->trav.igrRewind);
  Trav_MgrSetOption(travMgr, Pdt_TravIgrRewindMinK_c, inum, opt->trav.igrRewindMinK);
  Trav_MgrSetOption(travMgr, Pdt_TravIgrBwdRefine_c, inum, opt->trav.igrBwdRefine);
  Trav_MgrSetIgrGrowCone(travMgr, opt->trav.igrGrowCone);
  Trav_MgrSetOption(travMgr, Pdt_TravIgrUseRings_c, inum,
    opt->trav.igrUseRings);
  Trav_MgrSetOption(travMgr, Pdt_TravIgrUseRingsStep_c, inum,
    opt->trav.igrUseRingsStep);
  Trav_MgrSetOption(travMgr, Pdt_TravIgrUseBwdRings_c, inum,
    opt->trav.igrUseBwdRings);
  Trav_MgrSetOption(travMgr, Pdt_TravIgrAssumeSafeBound_c, inum,
    opt->trav.igrAssumeSafeBound);
  Trav_MgrSetOption(travMgr, Pdt_TravIgrConeSubsetBound_c, inum,
    opt->trav.igrConeSubsetBound);
  Trav_MgrSetOption(travMgr, Pdt_TravIgrConeSubsetSizeTh_c, inum,
    opt->trav.igrConeSubsetSizeTh);
  Trav_MgrSetOption(travMgr, Pdt_TravIgrConeSubsetPiRatio_c, fnum,
    opt->trav.igrConeSubsetPiRatio);
  Trav_MgrSetOption(travMgr, Pdt_TravIgrConeSplitRatio_c, fnum,
    opt->trav.igrConeSplitRatio);
  Trav_MgrSetOption(travMgr, Pdt_TravIgrGrowConeMaxK_c, inum,
    opt->trav.igrGrowConeMaxK);
  Trav_MgrSetOption(travMgr, Pdt_TravIgrGrowConeMax_c, fnum,
    opt->trav.igrGrowConeMax);
  Trav_MgrSetOption(travMgr, Pdt_TravIgrFwdBwd_c, inum,
    opt->trav.igrFwdBwd);
  Trav_MgrSetIgrMaxIter(travMgr, opt->trav.igrMaxIter);
  Trav_MgrSetIgrMaxExact(travMgr, opt->trav.igrMaxExact);

  /* pdr */
  Trav_MgrSetPdrFwdEq(travMgr, opt->trav.pdrFwdEq);
  Trav_MgrSetPdrInf(travMgr, opt->trav.pdrInf);
  Trav_MgrSetOption(travMgr, Pdt_TravPdrUnfold_c, inum, opt->trav.pdrUnfold);
  Trav_MgrSetOption(travMgr, Pdt_TravPdrIndAssumeLits_c, inum,
    opt->trav.pdrIndAssumeLits);
  Trav_MgrSetOption(travMgr, Pdt_TravPdrPostordCnf_c, inum,
    opt->trav.pdrPostordCnf);
  Trav_MgrSetPdrCexJustify(travMgr, opt->trav.pdrCexJustify);
  Trav_MgrSetPdrGenEffort(travMgr, opt->trav.pdrGenEffort);
  Trav_MgrSetPdrIncrementalTr(travMgr, opt->trav.pdrIncrementalTr);
  Trav_MgrSetPdrShareReached(travMgr, opt->trav.pdrShareReached);
  Trav_MgrSetOption(travMgr, Pdt_TravPdrReuseRings_c, inum,
    opt->trav.pdrReuseRings);
  Trav_MgrSetOption(travMgr, Pdt_TravPdrMaxBlock_c, inum,
    opt->trav.pdrMaxBlock);
  Trav_MgrSetPdrUsePgEncoding(travMgr, opt->trav.pdrUsePgEncoding);
  Trav_MgrSetPdrBumpActTopologically(travMgr,
    opt->trav.pdrBumpActTopologically);
  Trav_MgrSetPdrSpecializedCallsMask(travMgr,
    opt->trav.pdrSpecializedCallsMask);
  Trav_MgrSetPdrRestartStrategy(travMgr, opt->trav.pdrRestartStrategy);
  Trav_MgrSetOption(travMgr, Pdt_TravPdrTimeLimit_c, inum,
    opt->expt.pdrTimeLimit);

  /* bmc */
  Trav_MgrSetOption(travMgr, Pdt_TravBmcItpRingsPeriod_c, inum,
    opt->trav.bmcItpRingsPeriod);
  Trav_MgrSetOption(travMgr, Pdt_TravBmcTrAbstrPeriod_c, inum,
    opt->trav.bmcTrAbstrPeriod);
  Trav_MgrSetOption(travMgr, Pdt_TravBmcTrAbstrInit_c, inum,
    opt->trav.bmcTrAbstrInit);
  
  /* tr */
  //Trav_MgrSetOption(travMgr, Pdt_TravTrProfileMethod_c, inum, opt->trav.???);
  //Trav_MgrSetOption(travMgr, Pdt_TravTrProfileDynamicEnable_c, inum, opt->trav.???);
  //Trav_MgrSetOption(travMgr, Pdt_TravTrProfileThreshold_c, inum, opt->trav.???);
  //Trav_MgrSetOption(travMgr, Pdt_TravTrSorting_c, inum, opt->trav.???);
  //Trav_MgrSetOption(travMgr, Pdt_TravTrSortW1_c, inum, opt->trav.???);
  //Trav_MgrSetOption(travMgr, Pdt_TravTrSortW2_c, inum, opt->trav.???);
  //Trav_MgrSetOption(travMgr, Pdt_TravTrSortW3_c, inum, opt->trav.???);
  //Trav_MgrSetOption(travMgr, Pdt_TravTrSortW4_c, inum, opt->trav.???);
  Trav_MgrSetOption(travMgr, Pdt_TravSortForBck_c, inum,
    opt->trav.sort_for_bck);
  Trav_MgrSetOption(travMgr, Pdt_TravTrPreimgSort_c, inum,
    opt->trav.trPreimgSort);
  //Trav_MgrSetOption(travMgr, Pdt_TravPartDisjTrThresh_c, inum, opt->trav.???);
  Trav_MgrSetOption(travMgr, Pdt_TravClustThreshold_c, inum,
    opt->trav.clustTh);
  Trav_MgrSetOption(travMgr, Pdt_TravApprClustTh_c, inum,
    opt->trav.apprClustTh);
  Trav_MgrSetOption(travMgr, Pdt_TravImgClustTh_c, inum, opt->trav.imgClustTh);
  Trav_MgrSetOption(travMgr, Pdt_TravSquaring_c, inum, opt->trav.squaring);
  Trav_MgrSetOption(travMgr, Pdt_TravTrProj_c, inum, opt->trav.trproj);
  Trav_MgrSetOption(travMgr, Pdt_TravTrDpartTh_c, inum, opt->trav.trDpartTh);
  Trav_MgrSetOption(travMgr, Pdt_TravTrDpartVar_c, pchar,
    opt->trav.trDpartVar);
  Trav_MgrSetOption(travMgr, Pdt_TravTrAuxVarFile_c, pchar,
    opt->trav.auxVarFile);
  //Trav_MgrSetOption(travMgr, Pdt_TravTrEnableVar_c, inum, opt->trav.???);
  Trav_MgrSetOption(travMgr, Pdt_TravApproxTrClustFile_c, pchar,
    opt->trav.approxTrClustFile);
  Trav_MgrSetOption(travMgr, Pdt_TravBwdTrClustFile_c, pchar,
    opt->trav.bwdTrClustFile);

  /* bdd */
  Trav_MgrSetFromSelect(travMgr, fromSelect);
  //Trav_MgrSetOption(travMgr, Pdt_TravUnderApproxMaxVars_c, inum, opt->trav.???);
  //Trav_MgrSetOption(travMgr, Pdt_TravUnderApproxMaxSize_c, inum, opt->trav.???);
  //Trav_MgrSetOption(travMgr, Pdt_TravUnderApproxTargetRed_c, inum, opt->trav.???);
  Trav_MgrSetOption(travMgr, Pdt_TravNoCheck_c, inum, opt->trav.noCheck);
  //Trav_MgrSetOption(travMgr, Pdt_TravNoMismatch_c, inum, opt->trav.???);
  Trav_MgrSetOption(travMgr, Pdt_TravMismOptLevel_c, inum,
    opt->trav.mism_opt_level);
  Trav_MgrSetOption(travMgr, Pdt_TravGroupBad_c, inum, opt->trav.groupbad);
  //Trav_MgrSetOption(travMgr, Pdt_TravKeepNew_c, inum, opt->trav.???);
  Trav_MgrSetOption(travMgr, Pdt_TravPrioritizedMc_c, inum,
    opt->trav.prioritizedMc);
  Trav_MgrSetOption(travMgr, Pdt_TravPmcPrioThresh_c, inum,
    opt->trav.pmcPrioThresh);
  Trav_MgrSetOption(travMgr, Pdt_TravPmcMaxNestedFull_c, inum,
    opt->trav.pmcMaxNestedFull);
  Trav_MgrSetOption(travMgr, Pdt_TravPmcMaxNestedPart_c, inum,
    opt->trav.pmcMaxNestedPart);
  Trav_MgrSetOption(travMgr, Pdt_TravPmcNoSame_c, inum, opt->trav.pmcNoSame);
  Trav_MgrSetOption(travMgr, Pdt_TravCountReached_c, inum,
    opt->trav.countReached);
  Trav_MgrSetOption(travMgr, Pdt_TravAutoHint_c, inum, opt->trav.autoHint);
  Trav_MgrSetOption(travMgr, Pdt_TravHintsStrategy_c, inum,
    opt->trav.hintsStrategy);
  Trav_MgrSetOption(travMgr, Pdt_TravHintsTh_c, inum, opt->trav.hintsTh);
  //Pdtutil_Assert(opt->trav.hints == NULL, "non NULL hints array!");
  Trav_MgrSetOption(travMgr, Pdt_TravHints_c, pvoid, (void *)opt->trav.hints);
  Pdtutil_Assert(opt->trav.auxPsSet == NULL, "non NULL auxPsSet!");
  Trav_MgrSetOption(travMgr, Pdt_TravAuxPsSet_c, pvoid,
    (void *)opt->trav.auxPsSet);
  Trav_MgrSetOption(travMgr, Pdt_TravGfpApproxRange_c, inum,
    opt->trav.gfpApproxRange);
  Trav_MgrSetOption(travMgr, Pdt_TravUseMeta_c, inum, opt->trav.fbvMeta);
  Trav_MgrSetOption(travMgr, Pdt_TravMetaMethod_c, inum,
    (int)opt->trav.metaMethod);
  Trav_MgrSetOption(travMgr, Pdt_TravMetaMaxSmooth_c, inum,
    opt->common.metaMaxSmooth);
  Trav_MgrSetOption(travMgr, Pdt_TravConstrain_c, inum, opt->trav.constrain);
  Trav_MgrSetOption(travMgr, Pdt_TravConstrainLevel_c, inum,
    opt->trav.constrain_level);
  Trav_MgrSetOption(travMgr, Pdt_TravCheckFrozen_c, inum,
    opt->common.checkFrozen);
  Trav_MgrSetOption(travMgr, Pdt_TravApprOverlap_c, inum,
    opt->trav.apprOverlap);
  Trav_MgrSetOption(travMgr, Pdt_TravApprNP_c, inum, opt->trav.approxNP);
  Trav_MgrSetOption(travMgr, Pdt_TravApprMBM_c, inum, opt->trav.approxMBM);
  Trav_MgrSetOption(travMgr, Pdt_TravApprGrTh_c, inum, opt->trav.apprGrTh);
  Trav_MgrSetOption(travMgr, Pdt_TravApprPreimgTh_c, inum,
    opt->trav.approxPreimgTh);
  Trav_MgrSetOption(travMgr, Pdt_TravMaxFwdIter_c, inum, opt->trav.maxFwdIter);
  Trav_MgrSetOption(travMgr, Pdt_TravNExactIter_c, inum, opt->trav.nExactIter);
  Trav_MgrSetOption(travMgr, Pdt_TravSiftMaxTravIter_c, inum,
    opt->trav.siftMaxTravIter);
  Trav_MgrSetOption(travMgr, Pdt_TravImplications_c, inum,
    opt->trav.implications);
  Trav_MgrSetOption(travMgr, Pdt_TravImgDpartTh_c, inum, opt->trav.imgDpartTh);
  Trav_MgrSetOption(travMgr, Pdt_TravImgDpartSat_c, inum,
    opt->trav.imgDpartSat);
  Trav_MgrSetOption(travMgr, Pdt_TravBound_c, inum, opt->trav.bound);
  Trav_MgrSetOption(travMgr, Pdt_TravStrictBound_c, inum,
    opt->trav.strictBound);
  Trav_MgrSetOption(travMgr, Pdt_TravGuidedTrav_c, inum, opt->trav.guidedTrav);
  Trav_MgrSetOption(travMgr, Pdt_TravUnivQuantifyTh_c, inum,
    opt->trav.univQuantifyTh);
  Trav_MgrSetOption(travMgr, Pdt_TravOptImg_c, inum, opt->trav.opt_img);
  Trav_MgrSetOption(travMgr, Pdt_TravOptPreimg_c, inum, opt->trav.opt_preimg);
  Trav_MgrSetOption(travMgr, Pdt_TravBddTimeLimit_c, inum,
    opt->expt.bddTimeLimit);
  Trav_MgrSetOption(travMgr, Pdt_TravBddNodeLimit_c, inum,
    opt->expt.bddNodeLimit);
  Trav_MgrSetOption(travMgr, Pdt_TravWP_c, pchar, opt->trav.wP);
  Trav_MgrSetOption(travMgr, Pdt_TravWR_c, pchar, opt->trav.wR);
  Trav_MgrSetOption(travMgr, Pdt_TravWBR_c, pchar, opt->trav.wBR);
  Trav_MgrSetOption(travMgr, Pdt_TravWU_c, pchar, opt->trav.wU);
  Trav_MgrSetOption(travMgr, Pdt_TravWOrd_c, pchar, opt->trav.wOrd);
  Trav_MgrSetOption(travMgr, Pdt_TravRPlus_c, pchar, opt->trav.rPlus);
  Trav_MgrSetOption(travMgr, Pdt_TravInvarFile_c, pchar, opt->trav.invarFile);
  Trav_MgrSetOption(travMgr, Pdt_TravStoreCnf_c, pchar, opt->trav.storeCNF);
  Trav_MgrSetOption(travMgr, Pdt_TravStoreCnfTr_c, inum, opt->trav.storeCNFTR);
  Trav_MgrSetOption(travMgr, Pdt_TravItpStoreRings_c, pchar, opt->trav.itpStoreRings);
  Trav_MgrSetOption(travMgr, Pdt_TravStoreCnfMode_c, inum,
    (int)opt->trav.storeCNFmode);
  Trav_MgrSetOption(travMgr, Pdt_TravStoreCnfPhase_c, inum,
    opt->trav.storeCNFphase);
  Trav_MgrSetOption(travMgr, Pdt_TravMaxCnfLength_c, inum,
    opt->trav.maxCNFLength);

}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
FbvSetTravMgrFsmData(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr
)
{
  Trav_MgrSetPdtSpecVar(travMgr, Fsm_MgrReadPdtSpecVar(fsmMgr));
  Trav_MgrSetPdtConstrVar(travMgr, Fsm_MgrReadPdtConstrVar(fsmMgr));
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
FbvFsmCheckComposePi(
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr
)
{
  Ddi_Mgr_t *ddiMgr = Fsm_MgrReadDdManager(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  int i, n = Ddi_VararrayNum(ps);
  Ddi_Vararray_t *substVar = Ddi_VararrayAlloc(ddiMgr, 0);
  Ddi_Bddarray_t *substFunc = Ddi_BddarrayAlloc(ddiMgr, 0);
  Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(pi);

  for (i = 0; i < n; i++) {
    Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);

    if (Ddi_BddSize(d_i) == 2) {
      Ddi_Var_t *v = Ddi_BddTopVar(d_i);

      if (Ddi_VarInVarset(piv, v)) {
        Ddi_Var_t *ns_i = Ddi_VararrayRead(ns, i);
        int isComplement = Ddi_BddIsComplement(d_i);
        Ddi_Bdd_t *substF = Ddi_BddMakeLiteral(ns_i, !isComplement);

        Ddi_VararrayInsertLast(substVar, v);
        Ddi_BddarrayInsertLast(substFunc, substF);
        Ddi_Free(substF);
      }
    }
  }

  if (Ddi_VararrayNum(substVar) > 0) {
    Ddi_Bdd_t *trBdd = Tr_TrBdd(tr);

    Ddi_BddComposeAcc(trBdd, substVar, substFunc);
  }

  Ddi_Free(piv);
  Ddi_Free(substVar);
  Ddi_Free(substFunc);
  return 0;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
FbvTestSec(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt
)
{
  Ddi_Mgr_t *ddiMgr = Fsm_MgrReadDdManager(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *deltaDup = NULL;
  bAig_Manager_t *bmgr = ddiMgr->aig.mgr;

  Ddi_Var_t *pvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
  Ddi_Var_t *cvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
  int i, j, size, iProp = -1, iConstr = -1, nstate = Ddi_VararrayNum(ps);
  Ddi_Bdd_t *deltaProp = NULL, *deltaPropRef;
  int doRetimeEq = 0;
  int nLatchEq = 0;
  int nStructEq = 0;
  int npart = 0, neq = 0;

  if (nstate == 0)
    return 0;
  for (i = 0; i < nstate; i++) {
    Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);

    if (v_i == pvarPs) {
      iProp = i;
    } else if (v_i == cvarPs) {
      iConstr = i;
    }
  }

  neq = Ddi_DeltaFindEqSpecs(delta, ps, pvarPs, cvarPs,
    Fsm_MgrReadInvarspecBDD(fsmMgr), &npart, 1, opt->pre.speculateEqProp);
  if (neq > 0) {
    opt->pre.nEqChecks = neq;
    opt->pre.nTotChecks = npart;
    if (neq > 0 && neq > npart / 5) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout,
          "Checking for Equivalence Property: OK (%d/%d Equivalences Found).\n",
          neq, npart)
        );
    } else {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout,
          "Checking for Equivalence Property: NO (%d/%d Equivalences Found).\n",
          neq, npart)
        );
    }
  }

  Ddi_Free(deltaDup);

  if (deltaProp == NULL) {
    deltaProp = Ddi_BddCofactor(Ddi_BddarrayRead(delta, iProp), pvarPs, 1);
    if (iConstr >= 0) {
      Ddi_BddCofactorAcc(deltaProp, cvarPs, 1);
    }
  }

  if (1 && opt->pre.specConj != 0) {
    int i, size = Ddi_BddSize(deltaProp);
    int conj = 1;

    Ddi_Bdd_t *pinv = Ddi_AigPartitionTop(deltaProp, 0);

    if (pinv != NULL && Ddi_BddPartNum(pinv) == 1) {
      Ddi_Free(pinv);
      pinv = Ddi_AigPartitionTop(deltaProp, 1);
      if (Ddi_BddPartNum(pinv) > 1) {
        opt->pre.specDisjNum = Ddi_BddPartNum(pinv);
        conj = 0;
      }
    } else if (pinv != NULL && Ddi_BddPartNum(pinv) > 1) {
      opt->pre.specConjNum = Ddi_BddPartNum(pinv);
    }

    opt->pre.specTotSize = size;

    for (i = 0; i < Ddi_BddPartNum(pinv); i++) {
      Ddi_Bdd_t *p_i = Ddi_BddPartRead(pinv, i);
      int size_i = Ddi_BddSize(p_i);

      if (i == 0 || size_i > opt->pre.specPartMaxSize)
        opt->pre.specPartMaxSize = size_i;
      if (i == 0 || size_i < opt->pre.specPartMinSize)
        opt->pre.specPartMinSize = size_i;
      if (opt->trav.autoHint > 0 && opt->pre.specDisjNum > 0
        && size_i <= opt->trav.autoHint) {
        /* check init state */
        int coversInit = 0;
        Ddi_Bdd_t *chk = Ddi_BddNot(p_i);

        if (Fsm_MgrReadInitStubBDD(fsmMgr) != NULL) {
          Ddi_BddComposeAcc(chk, ps, Fsm_MgrReadInitStubBDD(fsmMgr));
        } else {
          Ddi_BddAndAcc(chk, Fsm_MgrReadInitBDD(fsmMgr));
        }
        coversInit = Ddi_AigSat(chk);
        Ddi_Free(chk);
        if (coversInit) {
          if (opt->trav.hints == NULL) {
            opt->trav.hints = Ddi_BddarrayAlloc(ddiMgr, 0);
            Ddi_BddarrayInsertLast(opt->trav.hints, p_i);
          } else {
            Ddi_BddAndAcc(Ddi_BddarrayRead(opt->trav.hints, 0), p_i);
          }
        }
      }
    }
    Ddi_Free(pinv);
  }

  Ddi_Free(deltaProp);

  return opt->pre.nEqChecks;

}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
FbvTestRelational(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt
)
{
  Ddi_Mgr_t *ddiMgr = Fsm_MgrReadDdManager(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bdd_t *deltaConstr = NULL;
  Ddi_Var_t *pvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
  Ddi_Var_t *cvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
  int i, j, iProp = -1, iConstr = -1, nstate = Ddi_VararrayNum(ps);

  opt->pre.relationalNs = 0;
  if (nstate == 0)
    return 0;

  for (i = 0; i < nstate; i++) {
    Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);

    if (v_i == pvarPs) {
      iProp = i;
    } else if (v_i == cvarPs) {
      iConstr = i;
      deltaConstr = Ddi_BddarrayRead(delta, i);
    }
  }
  if (deltaConstr != NULL) {
    Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(pi);
    Ddi_Varset_t *constrSupp = Ddi_BddSupp(deltaConstr);
    Ddi_Vararray_t *piConstr = NULL;

    Ddi_VarsetSetArray(constrSupp);
    Ddi_VarsetSetArray(piv);

    Ddi_VarsetIntersectAcc(constrSupp, piv);
    Ddi_Free(piv);
    piConstr = Ddi_VararrayMakeFromVarset(constrSupp, 1);
    Ddi_Free(constrSupp);

    for (i = 0; i < Ddi_VararrayNum(piConstr); i++) {
      Ddi_Var_t *v = Ddi_VararrayRead(piConstr, i);

      Pdtutil_Assert(Ddi_VarReadMark(v) == 0, "0 var mark required");
      Ddi_VarWriteMark(v, 1);
    }

    for (i = 0; i < nstate; i++) {
      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta, i);
      Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);

      if (i == iProp || i == iConstr)
        continue;
      if (Ddi_BddSize(d_i) == 1) {
        Ddi_Var_t *v = Ddi_BddTopVar(d_i);

        if (Ddi_VarReadMark(v) != 0) {
          /* constrained pi var */
          opt->pre.relationalNs++;
        }
      }
    }

    for (i = 0; i < Ddi_VararrayNum(piConstr); i++) {
      Ddi_Var_t *v = Ddi_VararrayRead(piConstr, i);

      Ddi_VarWriteMark(v, 0);
    }

    Ddi_Free(piConstr);

  }

  return opt->pre.relationalNs;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
FbvConstrainInitStub(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * invar
)
{
  Ddi_Mgr_t *ddiMgr = Fsm_MgrReadDdManager(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  int i, j, ret = 0, nv, np;
  Ddi_Bdd_t *myConstr = NULL, *myConstrPart = NULL;
  Ddi_Bdd_t *myOne, *myZero;
  Ddi_Varset_t *supp;
  Ddi_Vararray_t *suppA;
  Ddi_Bddarray_t *substF = NULL;
  Ddi_Vararray_t *substV = NULL;
  Ddi_Bdd_t *impliedTerm = NULL;

  if (Ddi_BddarrayNum(delta)>1000 || initStub == NULL) {
    return 0;
  }

  myConstr = Ddi_BddCompose(invar, ps, initStub);

  myOne = Ddi_BddMakeConstAig(ddiMgr, 1);
  myZero = Ddi_BddMakeConstAig(ddiMgr, 0);
  supp = Ddi_BddarraySupp(initStub);
  suppA = Ddi_VararrayMakeFromVarset(supp, 1);
  nv = Ddi_VararrayNum(suppA);

  substV = Ddi_VararrayAlloc(ddiMgr, 0);
  substF = Ddi_BddarrayAlloc(ddiMgr, 0);
  Ddi_Free(supp);

  impliedTerm = DdiAigImpliedVarsAcc(myConstr, NULL, NULL);
  if (impliedTerm == NULL) {
    ret = 0;
  } else {
    myConstrPart = Ddi_AigPartitionTop(impliedTerm, 0);

    np = Ddi_BddPartNum(myConstrPart);

    for (i = 0; i < np; i++) {
      Ddi_Bdd_t *p_i = Ddi_BddPartRead(myConstrPart, i);

      Pdtutil_Assert(Ddi_BddSize(p_i) == 1,
        "init stub constrain not yet supported");
      Ddi_Var_t *v_i = Ddi_BddTopVar(p_i);
      int isCompl = Ddi_BddIsComplement(p_i);

      Ddi_VararrayInsertLast(substV, v_i);
      ret++;
      if (isCompl) {
        /* zero value */
        Ddi_BddarrayInsertLast(substF, myZero);
      } else {
        /* one value */
        Ddi_BddarrayInsertLast(substF, myOne);
      }
    }
    Fsm_MgrSetInitStubPiConstr(fsmMgr, impliedTerm);
  }
  if (ret > 0) {
    Ddi_BddarrayComposeAcc(initStub, substV, substF);
  }
  Ddi_Free(substF);
  Ddi_Free(substV);
  Ddi_Free(suppA);

  Ddi_Free(myOne);
  Ddi_Free(myZero);
  Ddi_Free(myConstr);
  Ddi_Free(myConstrPart);
  Ddi_Free(impliedTerm);

  return ret;

}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Fbv_DecompInfo_t *
FbvPropDecomp(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt
)
{
  Ddi_Mgr_t *ddiMgr = Fsm_MgrReadDdManager(fsmMgr);
  int i, j, k;
  Ddi_Bddarray_t *delta, *lambda, *pArray = NULL, *fromRings = NULL;
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Var_t *pvarPs, *cvarPs;
  int iProp = -1, iConstr = -1, nstate;
  Ddi_Bdd_t *deltaProp, *deltaConstr, *p, *pConj, *pDisj;
  Ddi_Varsetarray_t *coi, *coiPart = NULL, *coiArray = NULL;
  int nCoiShell, nCoiVars, pPartNum, *coiPartSize = NULL,
    *propSize = NULL, *coiArraySize = NULL, *partOrdId = NULL, *partOrdId0 =
    NULL;
  Fbv_DecompInfo_t *decompInfo;
  Ddi_Varset_t *psvars, *nsvars, *coivars = NULL, *pVars, *constrVars = NULL;
  Ddi_Bdd_t *pPart, *pCore = NULL, *plitPs, *clitPs;
  float coiRatio = 1.2;
  Ddi_Bdd_t *partInvarspec = NULL;
  int maxPart = opt->pre.specDecompMax;
  int forcePart = opt->pre.specDecompForce;
  Ddi_Varsetarray_t *multipleCois = NULL;

#if 0
  Tr_Mgr_t *trMgr = NULL;
  Trav_Mgr_t *travMgrAig = NULL;
  Fsm_Mgr_t *fsmMgr = NULL, *fsmMgr2 = NULL, *fsmMgr3 = NULL;
  Tr_Tr_t *tr = NULL;
  Ddi_Bdd_t *init = NULL, *invarspec, *invar = NULL, *rplus,
    *care = NULL, *assumeConstr = NULL;
  int i, j, k, num_ff, heuristic, verificationResult = -1, itpBoundLimit = -1;
  Ddi_Bdd_t *invarspecBdd, *invarspecItp, *pPart, *pCore =
    NULL, *plitPs, *clitPs;
  long elapsedTime, startVerifTime;
  int loopReduce, loopReduceCnt, lastDdiNode, bddNodeLimit = -1;
  int deltaSize, deltaNum, bmcTimeLimit = -1, bddTimeLimit =
    -1, bddMemoryLimit = -1;
  int doRunBdd = 0, doRunBmc = 0, doRunInd = 0, doRunLms = 0, doRunItp =
    0, doRunRed = 0;
  int doRunLemmas = 0, bmcSteps, indSteps, lemmasSteps =
    opt->mc.lemmas, indTimeLimit = -1, itpTimeLimit = -1;
  int useRplus = 0;
  int quitOnFailure = 0, done = 0;
#endif

  decompInfo = Pdtutil_Alloc(Fbv_DecompInfo_t, 1);

  /* PROPERTY DECOMPOSITION */
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  ps = Fsm_MgrReadVarPS(fsmMgr);
  pvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
  cvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
  iProp = iConstr = -1;
  nstate = Ddi_BddarrayNum(delta);
  for (i = 0; i < nstate; i++) {
    Ddi_Var_t *v_i = Ddi_VararrayRead(ps, i);

    if (v_i == pvarPs) {
      iProp = i;
    }
    if (v_i == cvarPs) {
      iConstr = i;
    }
  }


  Pdtutil_Assert(iProp < nstate, "PDT_BDD_INVARSPEC_VAR$PS var not found");

  deltaProp = Ddi_BddCofactor(Ddi_BddarrayRead(delta, iProp), pvarPs, 1);

  printf("TOT PROP SIZE: %d\n", Ddi_BddSize(deltaProp));

  printf("Total coi variables: ");

  coi = computeFsmCoiVars(fsmMgr, deltaProp, NULL, 0, 0);
  nCoiShell = Ddi_VarsetarrayNum(coi);
  nCoiVars = nCoiShell ?
    Ddi_VarsetNum(Ddi_VarsetarrayRead(coi, nCoiShell - 1)) : 0;
  printf("%d ", nCoiVars);
  Ddi_Free(coi);
  if (iConstr >= 0) {
    deltaConstr = Ddi_BddarrayRead(delta, iConstr);
    Ddi_BddCofactorAcc(deltaProp, cvarPs, 1);
  }
  coi = computeFsmCoiVars(fsmMgr, deltaProp, NULL, 0, 0);
  nCoiShell = Ddi_VarsetarrayNum(coi);
  nCoiVars = nCoiShell ?
    Ddi_VarsetNum(Ddi_VarsetarrayRead(coi, nCoiShell - 1)) : 0;
  printf("(%d", nCoiVars);
  Ddi_Free(coi);

  if (iConstr >= 0) {
    coi = computeFsmCoiVars(fsmMgr, deltaConstr, NULL, 0, 0);
    nCoiShell = Ddi_VarsetarrayNum(coi);
    if (nCoiShell) {
      constrVars = Ddi_VarsetDup(Ddi_VarsetarrayRead(coi, nCoiShell - 1));
      nCoiVars = Ddi_VarsetNum(constrVars);
    } else {
      constrVars = Ddi_VarsetVoid(ddiMgr);
      nCoiVars = 0;
    }
    printf("+%d", nCoiVars);
    Ddi_Free(coi);
  } else {
    constrVars = Ddi_VarsetVoid(ddiMgr);
  }
  printf(")\n");

  if (opt->pre.specDecompTot == 2) {
    Ddi_BddSetAig(deltaProp);
  }

  /* conj/disj partitioning */
  int tryHints = 0;
  Ddi_Bdd_t *hintConstr = NULL;
  Ddi_Varset_t *hintVars = NULL;
  pPart = NULL;
  if (!Ddi_BddIsPartConj(deltaProp)) {
    Ddi_AigStructRedRemAcc(deltaProp, NULL);
    pConj = Ddi_AigPartitionTop(deltaProp, 0);
    pDisj = Ddi_AigPartitionTop(deltaProp, 1);

    if (tryHints && Ddi_BddPartNum(pDisj)>1) {
      int j;
      Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
      //Ddi_BddPartSortBySizeAcc(pDisj, 0);
      Ddi_BddPartSortBySizeAcc(pDisj, 1); // now increrasing size
      Ddi_VararrayWriteMark(pi, 1);
      hintConstr = Ddi_BddMakePartConjVoid(ddiMgr);
      Ddi_Free(hintVars);
      hintVars = Ddi_VarsetVoid(ddiMgr);
      for (j = Ddi_BddPartNum(pDisj)-1; j>=0; j--) {
        Ddi_Bdd_t *p_j = Ddi_BddPartRead(pDisj, j);
        if (Ddi_BddSize(p_j)<10) {
          Ddi_Bdd_t *impl = Ddi_AigPartitionTop(p_j, 0);
          Ddi_Varset_t *supp = Ddi_BddSupp(impl);
          Ddi_VarsetUnionAcc(hintVars,supp);
          Ddi_Free(supp);
          for (int i=0; i<Ddi_BddPartNum(impl); i++) {
            Ddi_Bdd_t *lit = Ddi_BddPartRead(impl,i);
            Ddi_Var_t *v = Ddi_BddTopVar(lit);
#if 0
            printf("%s%s%s", i>0?"-":"",
                   Ddi_BddIsComplement(lit) ?"!":"",
                   Ddi_VarName(v));
#endif
            if (Ddi_VarReadMark(v) == 1 && Ddi_BddSize(lit)==1) {
              Ddi_VarWriteMark(v, 0);
            }
          }
          //          printf("\n");
          Ddi_BddSetAig(impl);
                  Ddi_BddNotAcc(impl);
          if (Ddi_AigSatAnd(hintConstr,impl,NULL)) 
            Ddi_BddPartInsertLast(hintConstr,impl);
          Ddi_Free(impl);
        }
        else {
          Ddi_Bdd_t *impl = Ddi_AigPartitionTop(p_j, 0);
          Ddi_BddPartSortBySizeAcc(impl, 1);
          int removed=1;
          for (int i=Ddi_BddPartNum(impl)-1; i>0; i--) {
            if (Ddi_BddSize(impl) < 4) break;
            Ddi_BddPartRemove(impl,i);
            removed=1;
          }
          Ddi_BddSetAig(impl);
          if (removed)
            Ddi_BddNotAcc(impl);
          if (Ddi_AigSatAnd(hintConstr,impl,NULL)) 
            Ddi_BddPartInsertLast(hintConstr,impl);
          Ddi_Free(impl);
        }
      }
      if (Ddi_BddPartNum(hintConstr)==0) {
        Ddi_Free(hintConstr);
      }
      //      Ddi_Free(hintConstr);
      Ddi_Free(hintVars);
      Ddi_VararrayWriteMark(pi, 0);
    }
    if (0) {
      int j;
      Ddi_Varset_t *psVars = Ddi_VarsetMakeFromArray(ps);

      for (j = 0; j < Ddi_BddPartNum(pConj); j++) {
        Ddi_Bdd_t *p_j = Ddi_BddPartRead(pConj, j);

        Ddi_BddNotAcc(p_j);
        Ddi_BddExistProjectAcc(p_j, psVars);
        Ddi_BddNotAcc(p_j);
      }
      Ddi_Free(psVars);
    }
  }

  int tryExistByCircQuant = 0;
  if (tryExistByCircQuant) {
    Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
    for (int i=0; i<Ddi_BddPartNum(pConj); i++) {
      Ddi_Bdd_t *p_i = Ddi_BddPartRead(pConj,i);
      Ddi_BddNotAcc(p_i);
      Ddi_AigExistByCircQuant (p_i,pi);
      Ddi_BddNotAcc(p_i);
    }
  }
  int tryExistByXor = 0;
  if (tryExistByXor) {
    Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
    for (int i=0; i<Ddi_BddPartNum(pConj); i++) {
      Ddi_Bdd_t *p_i = Ddi_BddPartRead(pConj,i);
      Ddi_BddNotAcc(p_i);
      Ddi_AigExistByXorAcc (p_i,pi);
      Ddi_BddNotAcc(p_i);
    }
  }

  if ((opt->pre.specDecompTot < 2) &&
    ((partInvarspec = Fsm_MgrReadInvarspecBDD(fsmMgr)) != NULL)) {
    pPart = Ddi_BddDup(partInvarspec);
  }
  if (pPart != NULL) {
    pPartNum = opt->pre.specConjNum = Ddi_BddPartNum(pPart);
    Ddi_Free(pConj);
    Ddi_Free(pDisj);
    if (forcePart) {
      pConj = pPart;
      pPart = NULL;
      opt->mc.ilambda = 0;
      Ddi_BddSetAig(deltaProp);
    }
  }
  if (!forcePart && pPart != NULL) {
  } else if (hintConstr!=NULL) {
    pPart = Ddi_BddMakePartConjVoid(ddiMgr);
    for (int i=Ddi_BddPartNum(hintConstr)-1; i>0; i--) {
      Ddi_Bdd_t *constr = Ddi_BddMakeAig(hintConstr);
      Ddi_Bdd_t *target_i = Ddi_BddNot(deltaProp);
      Ddi_BddAndAcc(target_i,constr);
      Ddi_Free(constr);
      Ddi_AigStructRedRemAcc(target_i, NULL);
      Ddi_BddNotAcc(target_i);
      if (Ddi_AigSat(target_i))
        Ddi_BddPartInsertLast(pPart,target_i);
      Ddi_Free(target_i);
      Ddi_BddPartRemove(hintConstr,Ddi_BddPartNum(hintConstr)-1);
    }
    Ddi_BddPartInsertLast(pPart, deltaProp);
    pPartNum = opt->pre.specConjNum = Ddi_BddPartNum(pPart);
    Ddi_Free(pConj);
    Ddi_Free(pDisj);
  } else if (opt->pre.specDecompEq == 3) {
    int j, k;
    Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);

    pPart = Ddi_BddMakePartConjVoid(ddiMgr);
    Ddi_BddPartInsertLast(pPart, deltaProp);
    Ddi_BddPartInsertLast(pPart, deltaProp);
    pPartNum = opt->pre.specConjNum = Ddi_BddPartNum(pPart);
    Ddi_Free(pConj);
    Ddi_Free(pDisj);

  } else if (opt->pre.specDecompEq == 2) {
    int j, k;
    Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
    Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
    Ddi_Varset_t **suppArray;

    suppArray = Ddi_BddarraySuppArray(delta);

    for (j = 0; j < Ddi_BddarrayNum(delta); j++) {
      Ddi_Bdd_t *d_j = Ddi_BddarrayRead(delta, j);
      Ddi_Varset_t *s = suppArray[j];
      Ddi_Var_t *v = Ddi_VararrayRead(ps, j);
      int cntSupp = 0, last = -1;

      for (k = 0; k < Ddi_BddarrayNum(delta); k++) {
        if (k != j) {
          Ddi_Varset_t *s_k = suppArray[k];

          if (Ddi_VarInVarset(s_k, v)) {
            last = k;
            cntSupp++;
          }
        }
      }
      //                if (Ddi_VarsetNum(s)==1)
      printf("%3d) s: %4d, v: %3d (ns: %3d) - %s ", j,
        Ddi_BddSize(d_j), Ddi_VarsetNum(s), cntSupp, Ddi_VarName(v));
      if (Ddi_VarsetNum(s) == 1) {
        printf(" <- %s%s",
          Ddi_BddIsComplement(d_j) ? "!" : "", Ddi_VarName(Ddi_VarsetTop(s)));
      }
      if (cntSupp == 1) {
        Ddi_Var_t *v_k = Ddi_VararrayRead(ps, last);

        printf(" |  -> %s", Ddi_VarName(v_k));
      }
      //        if (Ddi_VarsetNum(s)==1)
      printf("\n");
      if (Ddi_VarsetNum(s) > 1 && Ddi_VarsetNum(s) <= 5) {
        printf("SUPP: ");
        Ddi_VarsetPrint(s, 0, 0, stdout);
      }
      if (cntSupp <= 5) {
        printf("FO:");
        for (k = 0; k < Ddi_BddarrayNum(delta); k++) {
          if (k != j) {
            Ddi_Varset_t *s_k = suppArray[k];

            if (Ddi_VarInVarset(s_k, v)) {
              Ddi_Var_t *v_k = Ddi_VararrayRead(ps, k);

              printf(" %s", Ddi_VarName(v_k));
            }
          }
        }
        printf("\n");
      }

      if (strcmp(Ddi_VarName(v), "signal00000374") == 0) {
        Ddi_BddCofactorAcc(d_j, Ddi_VarFromName(ddiMgr,
            "signal00000375_normalized"), 0);
        printf("modified %s\n", Ddi_VarName(v));
      } else if (strcmp(Ddi_VarName(v), "signal0000036C_normalized") == 0) {
        Ddi_Var_t *v2 = Ddi_VarFromName(ddiMgr, "signal00000370_normalized");
        Ddi_Bdd_t *a0 = Ddi_BddMakeLiteralAig(v, 1);
        Ddi_Bdd_t *a2 = Ddi_BddMakeLiteralAig(v2, 1);

        Ddi_BddAndAcc(a2, a0);
        Ddi_BddOrAcc(d_j, a2);
        Ddi_Free(a2);
        Ddi_Free(a0);
        printf("modified %s\n", Ddi_VarName(v));
      }

    }

    pPart = Ddi_BddMakePartConjVoid(ddiMgr);
    Ddi_BddPartInsertLast(pPart, deltaProp);
    Ddi_BddPartInsertLast(pPart, deltaProp);
    pPartNum = opt->pre.specConjNum = Ddi_BddPartNum(pPart);
    Ddi_Free(pConj);
    Ddi_Free(pDisj);

  }

  else if (opt->pre.specDecompEq) {

    if (Ddi_BddPartNum(pConj) == 2) {
      Ddi_Bdd_t *a = Ddi_BddPartRead(pConj, 0);
      Ddi_Bdd_t *b = Ddi_BddPartRead(pConj, 1);
      Ddi_Bdd_t *p_a = Ddi_AigPartitionTop(a, 1);
      Ddi_Bdd_t *p_b = Ddi_AigPartitionTop(b, 1);
      Ddi_Bdd_t *eq_0 = NULL, *eq_1 = NULL;

      if (Ddi_BddPartNum(p_a) == 2 && Ddi_BddPartNum(p_b) == 2) {
        Ddi_Bdd_t *p_a_0 = Ddi_BddPartRead(p_a, 0);
        Ddi_Bdd_t *p_a_1 = Ddi_BddPartRead(p_a, 1);
        Ddi_Bdd_t *p_b_0n = Ddi_BddNot(Ddi_BddPartRead(p_b, 0));
        Ddi_Bdd_t *p_b_1n = Ddi_BddNot(Ddi_BddPartRead(p_b, 1));

        if (Ddi_BddEqual(p_a_0, p_b_0n) && Ddi_BddEqual(p_a_1, p_b_1n)) {
          printf("EQ found\n");
          eq_0 = Ddi_BddDup(p_a_0);
          eq_1 = Ddi_BddDup(p_b_1n);
        } else if (Ddi_BddEqual(p_a_0, p_b_1n) && Ddi_BddEqual(p_a_1, p_b_0n)) {
          printf("EQ found\n");
          eq_0 = Ddi_BddDup(p_a_0);
          eq_1 = Ddi_BddDup(p_b_0n);
        }
        Ddi_Free(p_b_0n);
        Ddi_Free(p_b_1n);
        if (eq_0 != NULL) {
          pPart = Ddi_BddMakePartConjVoid(ddiMgr);
          Ddi_BddPartInsertLast(pPart, eq_0);
          Ddi_BddPartInsertLast(pPart, eq_1);
          pPartNum = opt->pre.specConjNum = Ddi_BddPartNum(pPart);
          Ddi_Free(pConj);
          Ddi_Free(pDisj);
        }
      }
      Ddi_Free(p_a);
      Ddi_Free(p_b);
    }
  } else if (opt->pre.specDecompSat) {
    float cexSuppRatio = 1 /*0.95 */  / abs(opt->pre.specDecompSat);
    float cexSuppStep = cexSuppRatio;
    Ddi_Bdd_t *target = Ddi_BddNot(deltaProp);
    Ddi_Bdd_t *targetCare = Ddi_BddDup(target);
    int ii, kk;
    int restrictTarget = opt->pre.specDecompSat < 0;

    opt->pre.specDecompSat = abs(opt->pre.specDecompSat);

    pPart = Ddi_BddMakePartConjVoid(ddiMgr);

    for (kk = 0; kk < opt->pre.specDecompSat; kk++) {
      Ddi_Bdd_t *pPart_kk = Ddi_BddDup(target);
      Ddi_Bdd_t *myCex = Ddi_AigSatWithCex(targetCare);

      if (myCex == NULL) {
        opt->pre.specDecompSat = kk;
        break;
      }
      Ddi_Varset_t *mySm = Ddi_BddSupp(myCex);
      Ddi_Vararray_t *vA = NULL;
      Ddi_Varset_t *psVars = Ddi_VarsetMakeFromArray(ps);
      int nv;

      //      Ddi_VarsetIntersectAcc(mySm,psVars);
      vA = Ddi_VararrayMakeFromVarset(mySm, 1);
      nv = Ddi_VararrayNum(vA);
      Ddi_BddSetAig(myCex);
      for (ii = nv - 1; ii >= cexSuppRatio * nv; ii--) {
        // int iii = nv-ii-1;
        int iii = ii;
        Ddi_Var_t *v = Ddi_VararrayRead(vA, iii);

        Ddi_VarsetRemoveAcc(mySm, v);
      }
      //      Ddi_Free(mySm);
      // mySm = Ddi_VarsetDup(psVars);
      Ddi_Free(psVars);
      Ddi_BddExistAcc(myCex, mySm);

      Ddi_AigAndCubeAcc(pPart_kk, myCex);
      Ddi_BddNotAcc(pPart_kk);
      Ddi_BddAndAcc(targetCare, pPart_kk);
      if (restrictTarget)
        Ddi_BddAndAcc(target, pPart_kk);    /* ??? */
      Ddi_BddPartInsertLast(pPart, pPart_kk);
      Ddi_Free(pPart_kk);
      Ddi_Free(myCex);
      Ddi_Free(mySm);
      cexSuppRatio += cexSuppStep;
      // cexSuppRatio = 1.0;
      Ddi_Free(vA);
    }

    Ddi_BddPartInsertLast(pPart, deltaProp);
    pPartNum = opt->pre.specConjNum = Ddi_BddPartNum(pPart);
    Ddi_Free(pConj);
    Ddi_Free(pDisj);
    Ddi_Free(target);
    Ddi_Free(targetCare);
  } else if (!opt->pre.specDecompCore && pConj != NULL
    && opt->pre.specDecompMin <= 0
    && Ddi_BddPartNum(pConj) == 1) {
    Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
    Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);

    Ddi_BddPartSortBySizeAcc(pDisj, 1);   // increasing size
    int nc=0;
    Ddi_Bdd_t *partT = Ddi_BddNot(pConj);
    Ddi_BddSetPartDisj(partT);
    for (int ii = 0; ii < Ddi_BddPartNum(pDisj); ii++) {
      Ddi_Bdd_t *p_ii = Ddi_BddPartRead(pDisj, ii);

      Ddi_AigStructRedRemAcc(p_ii, NULL);

      Ddi_Vararray_t *s_ii = Ddi_BddSuppVararray(p_ii);
      Ddi_VararrayDiffAcc(s_ii,pi);
      int nSupp = Ddi_VararrayNum(s_ii);
      if (nSupp >=2 && nSupp <= 5) {
        for (int k=0; k<Ddi_VararrayNum(s_ii); k++) {
          Ddi_Var_t *v = Ddi_VararrayRead(s_ii,k);
          Ddi_Bdd_t *l0 = Ddi_BddMakeLiteralAig(v, 0);
          Ddi_Bdd_t *l1 = Ddi_BddMakeLiteralAig(v, 1);
          nc++;
          int np = Ddi_BddPartNum(partT);
          for (int j=0; j<np; j++) {
            Ddi_Bdd_t *t_j = Ddi_BddPartRead(partT,j);
            Ddi_Bdd_t *newPart = Ddi_BddDup(t_j);
            Ddi_AigAndCubeAcc(newPart, l1);
            Ddi_AigAndCubeAcc(t_j, l0);
            Ddi_BddPartInsertLast(partT,newPart);
            Ddi_Free(newPart);
          }
          Ddi_Free(l0);
          Ddi_Free(l1);
          Ddi_BddSetFlattened(partT);
          Ddi_BddSetPartDisj(partT);
        }
      }
      Ddi_Free(s_ii);
    }
    Ddi_BddNotAcc(partT);
    Ddi_DataCopy(pConj,partT);
    Ddi_Free(partT);
    pPart = Ddi_BddDup(pConj);
    pPartNum = opt->pre.specConjNum = Ddi_BddPartNum(pConj);
    printf("Conj part: %d constr var used - #part: %d.\n", nc,
           Ddi_BddPartNum(pConj));
  } else if (!opt->pre.specDecompCore && pConj != NULL
    && opt->pre.specDecompMin > 0
    && Ddi_BddPartNum(pConj) < opt->pre.specDecompMin) {

    int tryDec = 1, tryCompact = 0;

    while (tryDec && (Ddi_BddPartNum(pConj) < opt->pre.specDecompMin)) {

      int ii, np0 = Ddi_BddPartNum(pConj);

      tryDec = 0;
      Ddi_BddPartSortBySizeAcc(pConj, 0);   // decreasing size

      for (ii = 0; ii < Ddi_BddPartNum(pConj); ii++) {
        Ddi_Bdd_t *p_ii = Ddi_BddPartRead(pConj, ii);

        Ddi_AigStructRedRemAcc(p_ii, NULL);
        Ddi_Bdd_t *p_ii_Part = Ddi_AigPartitionTop(p_ii, 1);
        int np_ii = Ddi_BddPartNum(p_ii_Part);
        int j, j_max = -1, max = 1;
        Ddi_Bdd_t *pMaxPart = NULL, *pMax = NULL;
        int supset = 0;


        Ddi_BddPartSortBySizeAcc(p_ii_Part, 0); // decreasing size
        pPart = Ddi_BddDup(p_ii_Part);
        Ddi_Free(p_ii_Part);

        printf("Disj partitioning: %d partitions obtained.\n",
          opt->pre.specDisjNum);
        for (j = 0; j < np_ii; j++) {
          Ddi_Bdd_t *p_j = Ddi_BddPartRead(pPart, j);
          Ddi_Bdd_t *p_j_Part = Ddi_AigPartitionTopWithXor(p_j, 0, 1);

          int size = Ddi_BddSize(p_j);
          int np = Ddi_BddPartNum(p_j_Part);

          Ddi_Free(p_j_Part);
          //          if ( /*j==0 || */ np > max) {
          if (j==0 || np > max) {
            j_max = j;
            max = np;
          }
        }

        if (supset) {
          Ddi_BddSetAig(pPart);
          Ddi_DataCopy(p_ii, pPart);
          continue;
        }

        if (j_max < 0) {
          Ddi_Free(pPart);
          continue;
          //      break;
        }

        tryDec = 1;

        pMax = Ddi_BddPartExtract(pPart, j_max);
        //Ddi_AigStructRedRemAcc(pMax, NULL);
        pMaxPart = Ddi_AigPartitionTopWithXor(pMax, 0, 1);
        Ddi_Free(pMax);
        Ddi_BddSetAig(pPart);
        if (tryCompact) {
          int j;
          int size = Ddi_BddSize(pMaxPart);

          Ddi_BddPartSortBySizeAcc(pMaxPart, 0);    // decreasing size
          for (j = Ddi_BddPartNum(pMaxPart) - 2; j >= 1; j--) {
            Ddi_Bdd_t *p_j = Ddi_BddPartRead(pMaxPart, j);

            if (Ddi_BddSize(p_j) > size / 2)
              break;
            Ddi_Bdd_t *p_j1 = Ddi_BddPartExtract(pMaxPart, j + 1);

            Ddi_BddAndAcc(p_j, p_j1);
            Ddi_Free(p_j1);
          }
        }
        for (j = 0; j < Ddi_BddPartNum(pMaxPart); j++) {
          Ddi_Bdd_t *p_j = Ddi_BddPartRead(pMaxPart, j);

          Ddi_BddOrAcc(p_j, pPart);
        }
        Ddi_Free(pPart);
        pPart = pMaxPart;
        Ddi_DataCopy(p_ii, pPart);
        Ddi_Free(pPart);

      }
      //    useRplus = 1;
      Ddi_BddSetFlattened(pConj);
      Ddi_BddSetPartConj(pConj);
      //    if (Ddi_BddPartNum(pConj)>=np0) tryDec=0;
    }
    
    pPart = Ddi_BddDup(pConj);
    pPartNum = opt->pre.specConjNum = Ddi_BddPartNum(pConj);
    printf("Conj partitioning: %d partitions obtained.\n",
      opt->pre.specConjNum);
  } else if (!opt->pre.specDecompCore && pConj != NULL
    && Ddi_BddPartNum(pConj) >= 1) {
    int tryThis = 0; // pDisj!=NULL && Ddi_BddPartNum(pDisj)>1;
    if (tryThis && 0 && (Ddi_BddPartNum(pConj) == 2)) { /* just a try */
      Ddi_BddPartRemove(pConj,1);
      Ddi_Free(pDisj);
      pDisj = Ddi_AigPartitionTop(Ddi_BddPartRead(pConj,0), 1);
    }
    if (tryThis && (Ddi_BddPartNum(pConj) <= 2)) { /* just a try */
      int i, refMax=5; 
      for (i=Ddi_BddPartNum(pDisj)-1; i>0; i--) {
	Ddi_Bdd_t *p_i = Ddi_BddPartRead(pDisj,i);
	if (Ddi_BddSize(p_i) > 3 && (--refMax>0)) {
	  Ddi_BddPartRemove(pDisj,i);
	}
      }
      for (i=Ddi_BddPartNum(pDisj)-1; i>=0; i--) {
	Ddi_Bdd_t *p_i = Ddi_BddPartRead(pDisj,i);
	if (Ddi_BddSize(p_i) >= 3 && (--refMax>0)) {
	  Ddi_BddPartRemove(pDisj,i);
	}
      }
      Ddi_BddPartSortBySizeAcc(pDisj, 0);
      Ddi_AigStructRedRemAcc(pDisj, NULL);

      if (Ddi_BddSize(Ddi_BddPartRead(pDisj,0))>10) {
	int j;
	Ddi_Bdd_t *p_0 = Ddi_BddPartExtract(pDisj,0);
	Ddi_Bdd_t *p_i_part = Ddi_AigPartitionTop(p_0, 0);
	Ddi_BddPartSortBySizeAcc(p_i_part, 1);    // increasing size
	Ddi_BddSetAig(pDisj);
	for (j=0; j<Ddi_BddPartNum(p_i_part); j++) {
	  Ddi_Bdd_t *p_j = Ddi_BddPartRead(p_i_part,j);
	  Ddi_BddOrAcc(p_j,pDisj);
	  Ddi_AigStructRedRemAcc(p_j, NULL);
	}
	Ddi_Free(pDisj);
	pDisj = p_i_part;
	Ddi_DataCopy(pConj,pDisj);
      }
      else {
	Ddi_BddSetAig(pDisj);
	Ddi_BddPartWrite(pConj,0,pDisj);
      }
    }
    pPart = Ddi_BddDup(pConj);
    pPartNum = opt->pre.specConjNum = Ddi_BddPartNum(pConj);
    printf("Conj partitioning: %d partitions obtained.\n",
      opt->pre.specConjNum);
  } else if (opt->pre.specDecompCore != 0) {
    pCore = Ddi_BddMakeAig(pConj);
    pArray = Ddi_BddarrayAlloc(ddiMgr, 1);
    Ddi_BddarrayWrite(pArray, 0, pCore);
    Ddi_Free(pCore);
    pPartNum = 0;
    pPart = NULL;
  } else {
    printf("No conj partitioning found.\n");
  }

  if (0 && !opt->pre.specDecompCore && pDisj != NULL
    && Ddi_BddPartNum(pDisj) > 1) {
    int j, j_max = -1, max = 0;
    Ddi_Bdd_t *pMaxPart = NULL, *pMax = NULL;

    pPart = Ddi_BddDup(pDisj);
    pPartNum = opt->pre.specDisjNum = Ddi_BddPartNum(pDisj);
    printf("Disj partitioning: %d partitions obtained.\n",
      opt->pre.specDisjNum);
    for (j = 0; j < pPartNum; j++) {
      Ddi_Bdd_t *p_j = Ddi_BddPartRead(pPart, j);
      Ddi_Bdd_t *p_j_Part = Ddi_AigPartitionTop(p_j, 0);

      int size = Ddi_BddSize(p_j);
      int np = Ddi_BddPartNum(p_j_Part);

      Ddi_Free(p_j_Part);
      if (j == 0 || np > max) {
        j_max = j;
        max = np;
      }
    }
    pMax = Ddi_BddPartExtract(pPart, j_max);
    pMaxPart = Ddi_AigPartitionTop(pMax, 0);
    Ddi_Free(pMax);
    Ddi_BddSetAig(pPart);
    for (j = 0; j < Ddi_BddPartNum(pMaxPart); j++) {
      Ddi_Bdd_t *p_j = Ddi_BddPartRead(pMaxPart, j);

      Ddi_BddOrAcc(p_j, pPart);
    }
    Ddi_Free(pPart);
    pPart = pMaxPart;
    pPartNum = opt->pre.specConjNum = Ddi_BddPartNum(pPart);
    //    useRplus = 1;

  } else {
    printf("No disj partitioning found.\n");
  }
  Ddi_Free(pConj);
  Ddi_Free(pDisj);
  Ddi_Free(deltaProp);


  if (!opt->pre.specDecompCore) {

    if (pPart == NULL) {
      printf("No decomposition is possible! Aborting...\n");
      goto quitMem;
    }
    /* this is to avoid useless work for the moment */
    if (opt->pre.specConjNum < 1) {
      printf("Only conj verification enabled!\n");
      Ddi_Free(pPart);
      Ddi_Free(constrVars);
      goto quitMem;
    }

    {
      Ddi_Bddarray_t *myDelta = Fsm_MgrReadDeltaBDD(fsmMgr);
      Ddi_Bddarray_t *pA = Ddi_BddarrayMakeFromBddPart(pPart);
      long start = util_cpu_time();

      if (opt->pre.sccCoi) {
        multipleCois = Ddi_AigEvalMultipleCoiWithScc(myDelta, pA,
          Fsm_MgrReadVarPS(fsmMgr),
          opt->pre.createClusterFile, NULL,
          opt->pre.methodCluster, maxPart,
          opt->pre.clustThDiff, opt->pre.clustThShare,
          opt->pre.specDecompSort);
        Ddi_Free(pPart);
        pPart = Ddi_BddMakePartConjFromArray(pA);
        pPartNum = Ddi_BddPartNum(pPart);
      } else {
        multipleCois = Ddi_AigEvalMultipleCoi(myDelta, pA,
          Fsm_MgrReadVarPS(fsmMgr), opt->pre.createClusterFile, NULL, 0, 0);
      }
      Ddi_Free(pA);
      //printf("\n\ndecomp\n\n");
      /** processing **/
      //printf("\n\n\nPASSATO DI QUA\n\n\n");

      long end = util_cpu_time();

      printf("\nELAPSED BITMAP COI = %s\n", util_print_time(end - start));

    }

    /* compute partition supports */
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "COI Support Size: ")
      );
    coiPartSize = Pdtutil_Alloc(int,
      pPartNum
    );
    propSize = Pdtutil_Alloc(int,
      pPartNum
    );

    coiPart = Ddi_VarsetarrayAlloc(ddiMgr, pPartNum);
    long start = util_cpu_time();

    for (i = 0; i < pPartNum; i++) {
      if (multipleCois != NULL) {
        coivars = Ddi_VarsetDup(Ddi_VarsetarrayRead(multipleCois, i));
        nCoiVars = Ddi_VarsetNum(coivars);
      } else {
        coi = computeFsmCoiVars(fsmMgr, Ddi_BddPartRead(pPart, i), NULL, 0, 0);
        nCoiShell = Ddi_VarsetarrayNum(coi);
        if (nCoiShell) {
          coivars = Ddi_VarsetDup(Ddi_VarsetarrayRead(coi, nCoiShell - 1));
          nCoiVars = Ddi_VarsetNum(coivars);
        } else {
          coivars = Ddi_VarsetVoid(ddiMgr);
          nCoiVars = 0;
        }
      }
#if 0
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "%d ", nCoiVars)
        );
#endif
      coiPartSize[i] = nCoiVars;
      propSize[i] = Ddi_BddSize(Ddi_BddPartRead(pPart, i));
      Ddi_VarsetarrayWrite(coiPart, i, coivars);
      Ddi_Free(coi);
      Ddi_Free(coivars);
    }
    long end = util_cpu_time();

    Ddi_Free(multipleCois);

    printf("\nELAPSED STANDARD = %s\n", util_print_time(end - start));
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "\n")
      );

    /* sort partition supports */
    pArray = Ddi_BddarrayAlloc(ddiMgr, 0);
    coiArraySize = Pdtutil_Alloc(int,
      pPartNum
    );

    coiArray = Ddi_VarsetarrayAlloc(ddiMgr, pPartNum);
    k = 0;
    partOrdId0 = Pdtutil_Alloc(int,
      pPartNum
    );
    partOrdId = Pdtutil_Alloc(int,
      pPartNum
    );

    for (i = 0; i < pPartNum; i++) {
      partOrdId[i] = i;
      partOrdId0[i] = i;
    }

    if (!opt->pre.specDecompSort) {
      for (i = 0; i < pPartNum; i++) {
        coiArraySize[i] = coiPartSize[i];
        p = Ddi_BddPartRead(pPart, i);
        Ddi_BddarrayWrite(pArray, i, p);
        coivars = Ddi_VarsetarrayRead(coiPart, i);
        Ddi_VarsetarrayWrite(coiArray, i, coivars);
      }
    } else {
      int maxSize = 0;

      for (i = 0; i < pPartNum; i++) {
        if (coiPartSize[i] > maxSize) {
          maxSize = coiPartSize[i];
        }
      }
      while (pPartNum > 0) {
        j = 0;
        for (i = 1; i < pPartNum; i++) {
          if (1 || (coiPartSize[i] < 0.9 * maxSize)) {
            if ((abs(coiPartSize[i] - coiPartSize[j]) < 3) &&
              (propSize[i] < 0.8 * propSize[j])) {
              j = i;
            } else if (coiPartSize[i] < coiPartSize[j]) {
              j = i;
            } else if ((coiPartSize[i] == coiPartSize[j]) &&
              (propSize[i] < propSize[j])) {
              j = i;
            }
          } else {
            if (propSize[i] < propSize[j]) {
              j = i;
            }
          }
        }
        p = Ddi_BddPartQuickExtract(pPart, j);
        Ddi_BddarrayInsertLast(pArray, p);
        Ddi_Free(p);
        coivars = Ddi_VarsetarrayQuickExtract(coiPart, j);
        Ddi_VarsetarrayWrite(coiArray, k, coivars);
        Ddi_Free(coivars);
        coiArraySize[k] = coiPartSize[j];
        coiPartSize[j] = coiPartSize[--pPartNum];
        propSize[j] = propSize[pPartNum];
        partOrdId[k] = partOrdId0[j];
        partOrdId0[j] = partOrdId0[pPartNum];
        k++;
      }
      pPartNum = k;
    }

    Ddi_Free(pPart);
    Ddi_Free(coiPart);
    Pdtutil_Free(propSize);
    Pdtutil_Free(coiPartSize);

    printf("Sorted parts: ");
    for (i = 0; i < pPartNum; i++) {
      printf("%d ", partOrdId[i]);
    }
    printf("\n");
    printf("Sorted coi supps: ");
    for (i = 0; i < pPartNum; i++) {
      //coi = computeFsmCoiVars(fsmMgr, Ddi_BddarrayRead(pArray, i), 0, 0);
      //nCoiShell = Ddi_VarsetarrayNum(coi);
      //nCoiVars = nCoiShell ? 
      //   Ddi_VarsetNum(Ddi_VarsetarrayRead(coi, nCoiShell-1)) : 0;
      //Ddi_Free(coi);
      nCoiVars = Ddi_VarsetNum(Ddi_VarsetarrayRead(coiArray, i));
      assert(nCoiVars == coiArraySize[i]);
      printf("%d ", nCoiVars);
    }
    printf("\n");

#if 0
    printf("CLUSTERING: PROPERTIES CLUSTERING NEW\n");
    printf("Num Properties %d\n", Ddi_BddarrayNum(pArray));
    printf("Num variables %d\n", Ddi_BddarrayNum(Fsm_MgrReadDeltaBDD(fsmMgr)));
    savePropertiesCOIs(fsmMgr, opt->pre.createClusterFile,
      opt->pre.clusterFile, opt->pre.methodCluster, pArray,
      opt->pre.thresholdCluster);
    Ddi_Bddarray_t *pArrayCL = Ddi_BddarrayAlloc(ddiMgr, 0);
    Ddi_Varset_t *partCoiCL = Ddi_VarsetVoid(ddiMgr);
    FILE *fp = fopen(opt->pre.clusterFile, "r");
    int nProperties, nCluster, *position, i, pPartNumCL;

    fscanf(fp, "%d\n", &nProperties);
    fscanf(fp, "%d\n", &nCluster);
    //printf("nProperties: %d nCluster: %d\n",nProperties,nCluster);
    //printf("Numero proprietà %d\n",Ddi_BddarrayNum(pArray));
    position = Pdtutil_Alloc(int,
      nCluster * 2
    );

    for (i = 0; i < ((nCluster * 2) - 1); i++)
      position[i] = 0;
    while (!feof(fp)) {
      int prop, cluster, k, lastkCL = -1;

      fscanf(fp, "%d %d\n", &prop, &cluster);
      //printf("property: %d cluster: %d\n",prop,cluster);
      Ddi_Bdd_t *p_kCL = Ddi_BddarrayRead(pArray, prop - 1);
      Ddi_Varset_t *coi_kCL = Ddi_VarsetarrayRead(coiArray, prop - 1);

      Ddi_VarsetUnionAcc(partCoiCL, coi_kCL);
      k = (cluster - 1) * 2;
      //printf("Position %d\n",k); 
      if (position[k]) {
        Ddi_BddAndAcc(Ddi_BddarrayRead(pArrayCL, position[k + 1]), p_kCL);
      } else {
        position[k] = 1;
        lastkCL++;
        position[k + 1] = lastkCL;
        Ddi_BddarrayInsertLast(pArrayCL, p_kCL);
        Ddi_Free(partCoiCL);
        partCoiCL = Ddi_VarsetDup(coi_kCL);
      }
    }
    Ddi_Free(partCoiCL);
    pPartNumCL = Ddi_BddarrayNum(pArrayCL);
    fclose(fp);
    printf("Properties reduced to %d\n", pPartNumCL);

    printf("Grouped coi supps: ");
    for (i = 0; i < pPartNumCL; i++) {
      coi = computeFsmCoiVars(fsmMgr,
        Ddi_BddarrayRead(pArrayCL, i), NULL, 0, 0);
      nCoiShell = Ddi_VarsetarrayNum(coi);
      if (nCoiShell) {
        coivars = Ddi_VarsetDup(Ddi_VarsetarrayRead(coi, nCoiShell - 1));
        nCoiVars = Ddi_VarsetNum(coivars);
        printf("%d ", nCoiVars);
      } else {
        coivars = Ddi_VarsetVoid(ddiMgr);
        nCoiVars = 0;
        printf("%d ", nCoiVars);
      }
      //coiPartSize[i] = coiArraySize[i] = nCoiVars;
      //Ddi_VarsetarrayWrite(coiPart, i, coivars);
      Ddi_VarsetarrayWrite(coiArray, i, coivars);
    }
    printf("\n");

    printf("Prop sizes:");

    for (i = 0; i < pPartNumCL; i++) {
      printf(" %d", Ddi_BddSize(Ddi_BddarrayRead(pArrayCL, i)));
    }
    printf("\n");
    printf("PROPERTIES CLUSTERING NEW END\n");
    decompInfo->pArray = pArrayCL;
    //decompInfo->pArray = compactProperties(fsmMgr,ddiMgr,coiArray,opt->pre.createClusterFile,opt->pre.clusterFile,opt->pre.methodCluster,pArray,opt->pre.thresholdCluster);
    decompInfo->partOrdId = partOrdId;
    decompInfo->coiArray = coiArray;
    decompInfo->hintVars = hintVars;
    return decompInfo;
#endif
    printf("\n");

    printf
      ("forcePart %d\nilambda %d\nspecDecompTot %d\nmaxPart %d\nnProperties %d\n",
      forcePart, opt->mc.ilambda, opt->pre.specDecompTot, maxPart,
      Ddi_BddarrayNum(pArray));
    if ((!(forcePart == 1) && (opt->mc.ilambda >= 0)
        || opt->pre.specDecompTot > 1)
      && maxPart > 0 && Ddi_BddarrayNum(pArray) > maxPart) {
      int maxCoi = Ddi_VarsetNum(Ddi_VarsetarrayRead(coiArray, pPartNum - 1));
      int minCoi = Ddi_VarsetNum(Ddi_VarsetarrayRead(coiArray, 0));
      int k, np = Ddi_BddarrayNum(pArray);
      int factor = np / maxPart + 1;
      int tryCompact = 0;;
      if (!opt->pre.createClusterFile) {
        tryCompact = 1;
        printf("metodo originale");
      } else {
        tryCompact = 0;
        printf("nuovo metodo");
      }
      printf("\n");
      if (!tryCompact) {
        decompInfo->pArray =
          compactProperties(fsmMgr, ddiMgr, coiArray,
          opt->pre.createClusterFile, opt->pre.clusterFile,
          opt->pre.methodCluster, pArray, opt->pre.thresholdCluster);
        pArray = decompInfo->pArray;
        pPartNum = Ddi_BddarrayNum(pArray);
      }

      while (tryCompact) {
        Ddi_Bddarray_t *pArray1 = Ddi_BddarrayAlloc(ddiMgr, 0);
        Ddi_Varset_t *partCoi = Ddi_VarsetVoid(ddiMgr);

        for (k = 0; k < np; k++) {
          Ddi_Bdd_t *p_k = Ddi_BddarrayRead(pArray, k);
          Ddi_Varset_t *coi_k = Ddi_VarsetarrayRead(coiArray, k);
          int nCoi = coiArraySize[k];

          if (nCoi < maxCoi / 2)
            nCoi = maxCoi / 2;
          Ddi_VarsetUnionAcc(partCoi, coi_k);
          if ((k % factor) == 0) {
            //    if (k==0 || Ddi_VarsetNum(partCoi) > nCoi*coiRatio) {
            Ddi_BddarrayInsertLast(pArray1, p_k);
            Ddi_Free(partCoi);
            partCoi = Ddi_VarsetDup(coi_k);
          } else {
            Ddi_BddAndAcc(Ddi_BddarrayRead(pArray1,
                Ddi_BddarrayNum(pArray1) - 1), p_k);
          }
        }
        Ddi_Free(partCoi);
        pPartNum = Ddi_BddarrayNum(pArray1);
        if (Ddi_BddarrayNum(pArray1) > maxPart) {
          Ddi_Free(pArray1);
          tryCompact = 1;
        } else {
          Ddi_Free(pArray);
          pArray = pArray1;
          tryCompact = 0;
        }
        coiRatio *= 1.1;
      }

      Ddi_Free(coiArray);
      Ddi_Free(coiPart);
      Pdtutil_Free(coiArraySize);

      coiPartSize = Pdtutil_Alloc(int,
        pPartNum
      );

      coiPart = Ddi_VarsetarrayAlloc(ddiMgr, pPartNum);
      coiArray = Ddi_VarsetarrayAlloc(ddiMgr, pPartNum);
      coiArraySize = Pdtutil_Alloc(int,
        pPartNum
      );

      for (i = 0; i < pPartNum; i++) {
        coi = computeFsmCoiVars(fsmMgr,
          Ddi_BddarrayRead(pArray, i), NULL, 0, 0);
        nCoiShell = Ddi_VarsetarrayNum(coi);
        if (nCoiShell) {
          coivars = Ddi_VarsetDup(Ddi_VarsetarrayRead(coi, nCoiShell - 1));
          nCoiVars = Ddi_VarsetNum(coivars);
        } else {
          coivars = Ddi_VarsetVoid(ddiMgr);
          nCoiVars = 0;
        }
        coiPartSize[i] = coiArraySize[i] = nCoiVars;
        Ddi_VarsetarrayWrite(coiPart, i, coivars);
        Ddi_VarsetarrayWrite(coiArray, i, coivars);
        Ddi_Free(coi);
        Ddi_Free(coivars);
      }

      printf("partition reduced from %d to %d - max coi: %d\n", np, pPartNum,
        coiPartSize[pPartNum - 1]);

      printf("Grouped coi supps: ");
      for (i = 0; i < pPartNum; i++) {
        nCoiVars = coiArraySize[i];
        printf("%d ", nCoiVars);
      }
      printf("\n");

      Pdtutil_Free(coiPartSize);
    }

    /* perform decomposed verification */
    if (opt->pre.specConjNum < 1) {
      printf("Only conj verification enabled!\n");
      Ddi_Free(pArray);
      Ddi_Free(coiPart);
      Ddi_Free(coiArray);
      Ddi_Free(constrVars);
      Pdtutil_Free(coiArraySize);
      goto quitMem;
    }

    printf("Prop sizes:");

    for (i = 0; i < pPartNum; i++) {
      printf(" %d", Ddi_BddSize(Ddi_BddarrayRead(pArray, i)));
    }
    printf("\n");

  }

  Ddi_Free(coiPart);
  Pdtutil_Free(coiArraySize);

quitMem:

  Pdtutil_Free(partOrdId0);
  Ddi_Free(constrVars);
  Ddi_Free(hintConstr);

  decompInfo->pArray = pArray;
  decompInfo->partOrdId = partOrdId;
  decompInfo->coiArray = coiArray;
  decompInfo->hintVars = hintVars;


  // exit(0);
  return decompInfo;

}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
FbvTask(
  Fbv_Globals_t * opt
)
{
  Ddi_Mgr_t *ddiMgr;

  ddiMgr = Ddi_MgrInit("DDI_manager", NULL, 0, DDI_UNIQUE_SLOTS,
                       DDI_CACHE_SLOTS * 10, 0, -1, -1);

  if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
      ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
    Ddi_MgrSetVerbosity(ddiMgr, Pdtutil_VerbLevel_e(opt->verbosity));
  }

  Ddi_AigInit(ddiMgr, 100);

  Ddi_MgrSiftEnable(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"));
  Ddi_MgrSetSiftThresh(ddiMgr, opt->ddi.siftTh);

  Ddi_MgrSetAigCnfLevel(ddiMgr, opt->ddi.aigCnfLevel);
  Ddi_MgrSetAigRedRemLevel(ddiMgr, opt->ddi.aigRedRem);
  Ddi_MgrSetAigRedRemMaxCnt(ddiMgr, opt->ddi.aigRedRemPeriod);
  Ddi_MgrSetAigAbcOptLevel(ddiMgr, opt->common.aigAbcOpt);

  Ddi_MgrSetAigItpAbc(ddiMgr, opt->ddi.itpAbc);
  Ddi_MgrSetAigItpCore(ddiMgr, opt->ddi.itpCore);
  Ddi_MgrSetAigItpMem(ddiMgr, opt->ddi.itpMem);
  Ddi_MgrSetAigItpOpt(ddiMgr, opt->ddi.itpOpt);
  Ddi_MgrSetAigItpStore(ddiMgr, opt->ddi.itpStore);
  Ddi_MgrSetAigItpPartTh(ddiMgr, opt->ddi.itpPartTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpActiveVars_c, inum, opt->ddi.itpActiveVars);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStoreTh_c, inum, opt->ddi.itpStoreTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpAigCore_c, inum, opt->ddi.itpAigCore);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpTwice_c, inum, opt->ddi.itpTwice);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpIteOptTh_c, inum, opt->ddi.itpIteOptTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStructOdcTh_c, inum,
                   opt->ddi.itpStructOdcTh);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiNnfClustSimplify_c, inum,
                   opt->ddi.nnfClustSimplify);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMap_c, inum, opt->ddi.itpMap);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompute_c, inum, opt->ddi.itpCompute);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpDrup_c, inum, opt->ddi.itpDrup);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCompact_c, inum, opt->ddi.itpCompact);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpClust_c, inum, opt->ddi.itpClust);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpNorm_c, inum, opt->ddi.itpNorm);
  Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpSimp_c, inum, opt->ddi.itpSimp);


  char **argv = opt->mc.task;
  char *task = argv[0]; argv++;
  if (strcmp(task,"op")==0) {
    char *op = argv[0]; argv++;
    Ddi_Bdd_t *op0, *op1, *res;
    op0 = Ddi_AigNetLoadAiger(ddiMgr, NULL, argv[0]);
    op1 = Ddi_AigNetLoadAiger(ddiMgr, NULL, argv[1]);
    if (strcmp(op,"and")==0) {
      res = Ddi_BddAnd(op0,op1);
    }
    else if (strcmp(op,"or")==0) {
      res = Ddi_BddOr(op0,op1);
    }
    Pdtutil_VerbosityMgrIf(ddiMgr, Pdtutil_VerbLevelUsrMax_c) {
      fprintf(dMgrO(ddiMgr),
              "op done: |%d| %s |%d| -> |%d|\n",
              Ddi_BddSize(op0), op, Ddi_BddSize(op1), Ddi_BddSize(res));
    }
    Ddi_AigNetStoreAiger(res, 0, argv[2]);
    Ddi_Free(op0);
    Ddi_Free(op1);
    Ddi_Free(res);
  }

  Ddi_MgrQuit(ddiMgr);
  exit(8);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Pdtutil_OptList_t *
FbvOpt2OptList(
  Fbv_Globals_t * opt
)
{
  Pdtutil_OptList_t *totOpt;
  Pdtutil_OptList_t *pkgOpt;

  totOpt = Pdtutil_OptListCreate(Pdt_OptPdt_c);

  /* Ddi options */
  pkgOpt = ddiOpt2OptList(opt);
  Pdtutil_OptListIns(totOpt, eListOpt, Pdt_ListNone_c, pvoid, pkgOpt);

  /* Fsm options */
  pkgOpt = fsmOpt2OptList(opt);
  Pdtutil_OptListIns(totOpt, eListOpt, Pdt_ListNone_c, pvoid, pkgOpt);

  /* Tr options */
  pkgOpt = trOpt2OptList(opt);
  Pdtutil_OptListIns(totOpt, eListOpt, Pdt_ListNone_c, pvoid, pkgOpt);

  /* Trav options */
  pkgOpt = travOpt2OptList(opt);
  Pdtutil_OptListIns(totOpt, eListOpt, Pdt_ListNone_c, pvoid, pkgOpt);

  /* Mc options */
  pkgOpt = mcOpt2OptList(opt);
  Pdtutil_OptListIns(totOpt, eListOpt, Pdt_ListNone_c, pvoid, pkgOpt);

  /* MORE ... */

#if 0
  opt->expName = NULL;
  opt->mc.strategy = NULL;

  /* ddi */

  //opt->ddi.satTimeout = 1;
  //opt->ddi.satTimeLimit = 30;
  //opt->ddi.satIncremental = 1;
  //opt->ddi.satVarActivity = 0;
  //opt->ddi.aigBddOpt = 0;
  //opt->common.aigAbcOpt = 3;
  //opt->ddi.aigCnfLevel = 2;
  //opt->ddi.aigRedRem = 1;
  //opt->ddi.aigRedRemPeriod = 1;

  //opt->ddi.itpCore = 0;
  //opt->ddi.itpMem = 0;
  //opt->ddi.itpMap = -1;
  //opt->ddi.itpOpt = 0;
  //opt->ddi.itpStore = NULL;
  //opt->ddi.itpLoad = 0;
  //opt->ddi.itpCompute = 0;
  //opt->ddi.itpAbc = 0;
  //opt->ddi.itpCompact = -1;
  //opt->ddi.itpClust = 1;
  //opt->ddi.itpNorm = 1;
  //opt->ddi.itpSimp = 1;

  //opt->trav.ternaryAbstr = 0;

  //opt->ddi.exist_opt_level = 2;
  //opt->ddi.exist_opt_thresh = 1000;
  opt->ddi.existClipTh = -1;
  opt->mc.nogroup = 0;
  opt->trav.metaMethod = Ddi_Meta_Size;
  //opt->common.metaMaxSmooth = 0;
  //opt->ddi.siftTh = FBV_SIFT_THRESH;
  //opt->ddi.siftMaxTh = -1;
  //opt->ddi.siftMaxCalls = -1;
  //opt->ddi.dynAbstrStrategy = 1;
  //opt->ddi.ternaryAbstrStrategy = 1;
  //opt->ddi.itpPartTh = 100000;

  /* trav */
  //opt->trav.itpRefineCex = 0;
  //opt->trav.lazyTimeLimit = 120.0;
  //opt->trav.travSelfTuning = 1;
  //opt->trav.travConstrLevel = 1;
  //opt->trav.mism_opt_level = 1;

  //opt->trav.itpPart = 0;
  //opt->trav.itpBdd = 0;
  //opt->trav.itpBoundkOpt = 0;
  //opt->trav.itpUseReached = 0;
  //opt->trav.itpTuneForDepth = 0;
  //opt->trav.itpConeOpt = 0;
  //opt->trav.itpInnerCones = 0;
  //opt->trav.itpReuseRings = 0;
  //opt->trav.itpInductiveTo = 0;
  //opt->trav.itpInitAbstr = 1;
  //opt->trav.itpAppr = 0;
  //opt->trav.itpExact = 0;
  //opt->trav.itpForceRun = -1;
  //opt->trav.itpCheckCompleteness = 0;
  //opt->trav.itpGenMaxIter = 0;
  //opt->trav.itpConstrLevel = 4;
  //opt->trav.itpEnToPlusImg = 1;

  //opt->trav.abstrRef = 0;
  //opt->trav.implAbstr = 0;
  //opt->trav.inputRegs = 0;
  //opt->trav.pdrShareReached = 0;
  //opt->trav.itpShareReached = 0;
  //opt->trav.igrMaxIter = 16;
  //opt->trav.igrMaxExact = 3;
  //opt->trav.igrGrowCone = 1;
  //opt->trav.igrSide = 0;
  //opt->trav.pdrFwdEq = 0;
  //opt->trav.pdrInf = 0;
  //opt->trav.pdrCexJustify = 1;
  //opt->trav.pdrGenEffort = 3;
  //opt->trav.pdrIncrementalTr = 1;
  //opt->trav.dynAbstr = 0;
  //opt->trav.dynAbstrInitIter = 0;

  //opt->trav.targetEn = 0;

  /* mc */
  //opt->mc.method = Mc_VerifApproxFwdExactBck_c;
  //opt->mc.pdr = 0;
  //opt->mc.aig = 0;
  //opt->mc.inductive = -1;
  //opt->mc.interpolant = -1;
  //opt->mc.custom = 0;
  //opt->mc.lemmas = -1;
  //opt->mc.lazy = -1;
  //opt->common.lazyRate = 1.0;
  //opt->mc.qbf = 0;
  //opt->mc.diameter = -1;
  //opt->mc.checkMult = 0;
  //opt->mc.exactTrav = 0;

  //opt->mc.bmc = -1;
  //opt->trav.bmcStrategy = 1;
  //opt->trav.interpolantBmcSteps = 0;
  //opt->trav.bmcTr = -1;
  //opt->trav.bmcStep = 1;
  //opt->trav.bmcFirst = 0;

  //opt->mc.itpSeq = 0;
  //opt->mc.itpSeqGroup = 0;
  //opt->mc.itpSeqSerial = 1.0;
  //opt->mc.initBound = -1;
  //opt->mc.enFastRun = -1;
  //opt->mc.fwdBwdFP = 1;

  /* mc expt/dynamic settings: decision taken by expt (?) */
  opt->expt.bddNodeLimit = -1;
  opt->expt.bddMemoryLimit = -1;
  opt->expt.bddTimeLimit = -1;
  opt->expt.itpBoundLimit = -1;
  opt->expt.itpTimeLimit = -1;
  opt->expt.itpMemoryLimit = -1;
  opt->expt.bmcTimeLimit = -1;
  opt->expt.indTimeLimit = -1;

  opt->expt.doRunBmc = 0;
  opt->expt.doRunItp = 0;
  opt->expt.doRunBdd = 0;
  opt->expt.doRunInd = 0;
  opt->expt.doRunLms = 0;
  opt->expt.doRunLemmas = 0;
  opt->expt.lemmasSteps = 0;
  opt->expt.bmcSteps = -1;
  opt->expt.indSteps = -1;
  opt->expt.thrdDisNum = 0;
  opt->expt.thrdEnNum = 0;

  /* mc spec decomp */
  opt->pre.specConj = 0;
  opt->pre.specConjNum = 1;
  opt->pre.specDisjNum = 1;
  opt->pre.specPartMaxSize = 1;
  opt->pre.specPartMinSize = 1;
  opt->pre.specTotSize = 1;
  opt->pre.specDecompConstrTh = 50000;
  opt->pre.specDecompMin = 2;
  opt->pre.specDecompMax = 30;
  opt->pre.specDecompTot = 0;
  opt->pre.specDecompForce = 0;
  opt->pre.specDecompEq = 0;
  opt->pre.specDecompSort = 1;
  opt->pre.specDecompSat = 0;
  opt->pre.specDecompCore = 0;
  opt->pre.specDecomp = -1;
  opt->pre.specDecompIndex = -1;


  /* tr */
  opt->common.checkFrozen = 0;
  // opt->trav.clustTh = TRAV_CLUST_THRESH;
  // opt->tr.squaringMaxIter=-1;
  opt->trav.trproj = 0;
  strcpy(opt->tr.trSort, "iwls95");
  opt->trav.sort_for_bck = 1;
  opt->tr.invar_in_tr = 0;
  // opt->trav.apprClustTh = TRAV_CLUST_THRESH;
  // opt->tr.approxImgMin = -1;
  // opt->tr.approxImgMax = -1;
  // opt->tr.approxImgIter = 1;
  // opt->tr.coiLimit = 0;

  /* dynamic for expt system */
  opt->mc.saveMethod = Mc_VerifMethodNone_c;
  opt->mc.saveComplInvarspec = 0;
  opt->trav.cntReachedOK = 0;

  /* pdt */
  opt->expt.expertLevel = 0;
  opt->trav.autoHint = 0;
  opt->mc.decompTimeLimit = 2000.0;
  opt->mc.abcOptFilename = NULL;
  opt->pre.speculateEqProp = 1;

  /* fsm opt on FILE */

  opt->pre.performAbcOpt = 0;
  opt->pre.abcSeqOpt = 0;
  opt->pre.iChecker = 0;
  opt->pre.iCheckerBound = 0;

  /* fsm load reductions/transformations */
  opt->pre.indEqDepth = -1;
  opt->pre.indEqMaxLevel = -1;
  opt->pre.indEqAssumeMiters = 0;
  opt->trav.strongLemmas = 0;
  opt->trav.implLemmas = 0;
  opt->trav.lemmasCare = 0;
  opt->trav.lemmasTimeLimit = 120.0;
  opt->pre.specEn = 0;
  opt->trav.noInit = 0;
  opt->pre.noTernary = 0;
  opt->pre.ternarySim = 3;
  opt->mc.combinationalProblem = 0;
  opt->trav.noCheck = 0;
  opt->pre.doCoi = 0;
  opt->pre.coisort = 0;
  opt->pre.constrCoiLimit = 1;
  opt->pre.constrIsNeg = 0;
  opt->pre.noConstr = 0;
  opt->mc.cegar = 0;
  opt->mc.cegarPba = 0;
  opt->fsm.cegarStrategy = 1;
  opt->fsm.cut = -1;
  opt->fsm.cutAtAuxVar = -1;
  opt->pre.lambdaLatches = 0;
  opt->pre.peripheralLatches = 0;
  opt->pre.peripheralLatchesNum = 0;
  opt->pre.forceInitStub = 0;
  opt->pre.initMinterm = -1;
  opt->pre.retime = 0;
  opt->pre.terminalScc = 0;
  opt->pre.impliedConstr = 0;
  opt->pre.reduceCustom = 0;
  opt->pre.findClk = 0;
  opt->pre.twoPhaseRed = 0;
  opt->pre.twoPhaseForced = 0;
  opt->pre.twoPhaseRedPlus = 0;
  opt->pre.twoPhaseOther = 0;
  opt->pre.clkPhaseSuggested = -1;
  opt->common.clk = NULL;
  opt->pre.clkVarFound = NULL;

  /* fsm get spec */
  opt->mc.range = 0;
  opt->mc.ilambda = 0;
  opt->mc.ilambdaCare = -1;
  opt->mc.nlambda = NULL;
  opt->mc.ninvar = NULL;
  opt->mc.compl_invarspec = 0;
  opt->mc.all1 = 0;


  /* unused */
  opt->mc.tuneForEqCheck = 0;

  /* stats */
  opt->pre.nEqChecks = 0;
  opt->pre.nTotChecks = 0;
  opt->pre.relationalNs = 0;
  opt->pre.terminalSccNum = 0;
  opt->pre.impliedConstrNum = 0;
  opt->stats.startTime = 0;
  opt->stats.setupTime = 0;
  opt->stats.fbvStartTime = 0;
  opt->stats.fbvStopTime = 0;

  /* other data */
  opt->tr.auxFwdTr = NULL;
  opt->trav.univQuantifyVars = NULL;    /* unused */

  opt->stats.verificationOK = 0;

  /* fbv */
  opt->ddi.aigPartial = 0;
  opt->pre.coiSets = NULL;

  /* fbv bdd routines -> to trav... */
  opt->trav.univQuantifyTh = -1;
  opt->trav.univQuantifyDone = 0;
  opt->ddi.saveClipLevel = 0;
  opt->trav.cex = 0;
  opt->trav.countReached = 0;
  opt->trav.gfpApproxRange = 1;
  opt->trav.implications = 0;
  opt->trav.approxPreimgTh = 500000;
  opt->trav.trPreimgSort = 0;
  opt->trav.guidedTrav = 0;
  opt->trav.prioritizedMc = 0;
  opt->trav.strictBound = 0;
  opt->trav.approxMBM = 0;
  opt->trav.approxNP = 0;
  opt->trav.apprAig = 0;
  opt->trav.apprOverlap = 0;
  opt->mc.checkDead = 0;
  opt->trav.opt_img = 0;
  opt->trav.opt_preimg = 0;
  opt->trav.fromSel = 'b';
  opt->trav.groupbad = 0;
  opt->trav.fbvMeta = 0;
  opt->trav.bound = -1;
  opt->trav.nExactIter = 0;
  opt->trav.maxFwdIter = -1;
  opt->trav.imgClustTh = TRAV_IMG_CLUST_THRESH;
  opt->ddi.imgClipTh = FBV_IMG_CLIP_THRESH;
  opt->trav.imgDpartTh = 0;
  opt->trav.imgDpartSat = 0;
  opt->trav.trDpartTh = 0;
  opt->trav.trDpartVar = NULL;
  opt->trav.squaring = 0;
  opt->trav.apprGrTh = 0;       // expt
  opt->trav.constrain = 0;
  opt->trav.constrain_level = 1;
  opt->tr.en = NULL;
  opt->trav.pmcPrioThresh = 5000;
  opt->trav.pmcMaxNestedFull = 100;
  opt->trav.pmcMaxNestedPart = 5;
  opt->trav.pmcNoSame = 0;
  opt->trav.auxPsSet = NULL;
  opt->trav.hints = NULL;
  opt->fsm.auxVarsUsed = 0;
  opt->ddi.info_item_num = 0;


  /* file/name management */

  opt->trav.auxVarFile = NULL;
  opt->trav.bwdTrClustFile = NULL;
  opt->trav.approxTrClustFile = NULL;
  opt->tr.manualTrDpartFile = NULL;
  opt->fsm.manualAbstr = NULL;
  opt->trav.hintsFile = NULL;
  opt->trav.invarFile = NULL;
  opt->trav.writeInvar = NULL;
  opt->trav.writeProof = NULL;
  opt->mc.invSpec = NULL;
  opt->mc.ctlSpec = NULL;
  opt->trav.rPlus = NULL;
  opt->mc.rInit = NULL;
  opt->trav.wP = NULL;
  opt->trav.wR = NULL;
  opt->trav.wBR = NULL;
  opt->trav.wU = NULL;
  opt->mc.ord = NULL;
  opt->trav.wOrd = NULL;
  opt->mc.checkInv = NULL;
  opt->mc.wFsm = NULL;
  opt->mc.wRes = NULL;

  opt->trav.storeCNF = NULL;
  opt->trav.storeCNFmode = 'B';
  opt->trav.storeCNFphase = 0;
  opt->trav.storeCNFTR = -1;
  opt->trav.maxCNFLength = 20;

  opt->pre.wrCoi = NULL;
  opt->trav.
#endif
    return totOpt;
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Pdtutil_OptList_t *
ddiOpt2OptList(
  Fbv_Globals_t * opt
)
{
  Pdtutil_OptList_t *pkgOpt = Pdtutil_OptListCreate(Pdt_OptDdi_c);

  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiVerbosity_c, inum,
    opt->verbosity);
  // Pdtutil_OptListIns(pkgOpt,eDdiOpt,Pdt_DdiExistClipTh_c,inum,opt->ddi.existClipTh);
  // Pdtutil_OptListIns(pkgOpt,eDdiOpt,Pdt_DdiSiftTh_c,inum,opt->ddi.siftTh);
  // Pdtutil_OptListIns(pkgOpt,eDdiOpt,Pdt_DdiSiftMaxTh_c,inum,opt->ddi.siftMaxTh);
  // Pdtutil_OptListIns(pkgOpt,eDdiOpt,Pdt_DdiSiftMaxCalls_c,inum,opt->ddi.siftMaxCalls);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiExistOptThresh_c, inum,
    opt->ddi.exist_opt_thresh);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiExistOptLevel_c, inum,
    opt->ddi.exist_opt_level);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiSatSolver_c, pchar,
    opt->common.satSolver);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiLazyRate_c, fnum,
    opt->common.lazyRate);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiAigAbcOpt_c, inum,
    opt->common.aigAbcOpt);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiAigBddOpt_c, inum,
    opt->ddi.aigBddOpt);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiAigRedRem_c, inum,
    opt->ddi.aigRedRem);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiAigRedRemPeriod_c, inum,
    opt->ddi.aigRedRemPeriod);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpPartTh_c, inum,
    opt->ddi.itpPartTh);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpIteOptTh_c, inum,
    opt->ddi.itpIteOptTh);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiDynAbstrStrategy_c, inum,
    opt->ddi.dynAbstrStrategy);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiTernaryAbstrStrategy_c, inum,
    opt->ddi.ternaryAbstrStrategy);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiSatTimeout_c, inum,
    opt->ddi.satTimeout);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiSatIncremental_c, inum,
    opt->ddi.satIncremental);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiSatVarActivity_c, inum,
    opt->ddi.satVarActivity);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiSatTimeLimit_c, inum,
    opt->ddi.satTimeLimit);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpOpt_c, inum, opt->ddi.itpOpt);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpAbc_c, inum, opt->ddi.itpAbc);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpCore_c, inum,
    opt->ddi.itpCore);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpAigCore_c, inum,
    opt->ddi.itpAigCore);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpTwice_c, inum,
    opt->ddi.itpTwice);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpMem_c, inum, opt->ddi.itpMem);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpMap_c, inum, opt->ddi.itpMap);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpStore_c, pchar,
    opt->ddi.itpStore);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpLoad_c, pchar,
    opt->ddi.itpLoad);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpDrup_c, inum,
    opt->ddi.itpDrup);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpCompute_c, inum,
    opt->ddi.itpCompute);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiAigCnfLevel_c, inum,
    opt->ddi.aigCnfLevel);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpCompact_c, inum, 
    opt->ddi.itpCompact);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpClust_c, inum, 
    opt->ddi.itpClust);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpNorm_c, inum, 
    opt->ddi.itpNorm);
  Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpSimp_c, inum,  
    opt->ddi.itpSimp);

  return pkgOpt;
}

/**Function*******************************************************************
  Synopsis    [] 
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Pdtutil_OptList_t *
fsmOpt2OptList(
  Fbv_Globals_t * opt
)
{
  Pdtutil_OptList_t *pkgOpt = Pdtutil_OptListCreate(Pdt_OptFsm_c);

  Pdtutil_OptListIns(pkgOpt, eFsmOpt, Pdt_FsmVerbosity_c, inum,
    opt->verbosity);
  Pdtutil_OptListIns(pkgOpt, eFsmOpt, Pdt_FsmCutThresh_c, inum, opt->fsm.cut);
  Pdtutil_OptListIns(pkgOpt,  eFsmOpt, Pdt_FsmCutAtAuxVar_c, inum, opt->fsm.cutAtAuxVar);
  Pdtutil_OptListIns(pkgOpt, eFsmOpt, Pdt_FsmUseAig_c, inum, opt->fsm.useAig);
  //  Pdtutil_OptListIns(pkgOpt,eFsmOpt,Pdt_FsmPeriferalRetimeIterations_c,inum,opt->pre.periferalRetimeIterations);
  Pdtutil_OptListIns(pkgOpt, eFsmOpt, Pdt_FsmCegarStrategy_c, inum,
    opt->fsm.cegarStrategy);

  return pkgOpt;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Pdtutil_OptList_t *
trOpt2OptList(
  Fbv_Globals_t * opt
)
{
  Pdtutil_OptList_t *pkgOpt = Pdtutil_OptListCreate(Pdt_OptTr_c);

  Pdtutil_OptListIns(pkgOpt, eTrOpt, Pdt_TrVerbosity_c, inum, opt->verbosity);
  //Pdtutil_OptListIns(pkgOpt,eTrOpt,Pdt_TrSort_c,pchar,opt->tr.trSort);
  Pdtutil_OptListIns(pkgOpt, eTrOpt, Pdt_TrClustTh_c, inum, opt->trav.clustTh);
  Pdtutil_OptListIns(pkgOpt, eTrOpt, Pdt_TrSquaringMaxIter_c, inum,
    opt->tr.squaringMaxIter);
  Pdtutil_OptListIns(pkgOpt, eTrOpt, Pdt_TrImgClustTh_c, inum,
    opt->trav.imgClustTh);
  Pdtutil_OptListIns(pkgOpt, eTrOpt, Pdt_TrApproxMinSize_c, inum,
    opt->tr.approxImgMin);
  Pdtutil_OptListIns(pkgOpt, eTrOpt, Pdt_TrApproxMaxSize_c, inum,
    opt->tr.approxImgMax);
  Pdtutil_OptListIns(pkgOpt, eTrOpt, Pdt_TrApproxMaxIter_c, inum,
    opt->tr.approxImgIter);
  Pdtutil_OptListIns(pkgOpt, eTrOpt, Pdt_TrCheckFrozen_c, inum,
    opt->common.checkFrozen);
  Pdtutil_OptListIns(pkgOpt, eTrOpt, Pdt_TrCoiLimit_c, inum, opt->tr.coiLimit);

  return pkgOpt;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Pdtutil_OptList_t *
travOpt2OptList(
  Fbv_Globals_t * opt
)
{
  Pdtutil_OptList_t *pkgOpt = Pdtutil_OptListCreate(Pdt_OptTrav_c);
  Trav_FromSelect_e fromSelect = Trav_FromSelectBest_c;

  if (opt->trav.fromSel == 'n') {
    fromSelect = Trav_FromSelectNew_c;
  } else if (opt->trav.fromSel == 'r') {
    fromSelect = Trav_FromSelectReached_c;
  } else if (opt->trav.fromSel == 't') {
    fromSelect = Trav_FromSelectTo_c;
  }

  /* general */
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravVerbosity_c, inum,
    opt->verbosity);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravClk_c, pchar, opt->common.clk);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravDoCex_c, inum, opt->trav.cex);

  /* unused */
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravConstrLevel_c, inum,
    opt->trav.travConstrLevel);

  /* aig */
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravSatSolver_c, pchar,
    opt->common.satSolver);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravAbcOptLevel_c, inum,
    opt->common.aigAbcOpt);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravTargetEn_c, inum,
    opt->trav.targetEn);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravDynAbstr_c, inum,
    opt->trav.dynAbstr);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravDynAbstrInitIter_c, inum,
    opt->trav.dynAbstrInitIter);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravImplAbstr_c, inum,
    opt->trav.implAbstr);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravTernaryAbstr_c, inum,
    opt->trav.ternaryAbstr);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravAbstrRef_c, inum,
    opt->trav.abstrRef);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravAbstrRefGla_c, inum,
    opt->trav.abstrRefGla);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravAbstrRefItp_c, inum,
    opt->trav.abstrRefItp);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravAbstrRefItpMaxIter_c, inum,
    opt->trav.abstrRefItpMaxIter);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravTrAbstrItp_c, inum,
    opt->trav.trAbstrItp);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravTrAbstrItpMaxFwdStep_c, inum,
    opt->trav.trAbstrItpMaxFwdStep);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravTrAbstrItpLoad_c, pchar,
    opt->trav.trAbstrItpLoad);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravTrAbstrItpStore_c, pchar,
    opt->trav.trAbstrItpStore);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravInputRegs_c, inum,
    opt->trav.inputRegs);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravSelfTuning_c, inum,
    opt->trav.travSelfTuning);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravLazyTimeLimit_c, fnum,
    opt->trav.lazyTimeLimit);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravLemmasTimeLimit_c, fnum,
    opt->trav.lemmasTimeLimit);


  /* itp */
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpBdd_c, inum,
    opt->trav.itpBdd);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpPart_c, inum,
    opt->trav.itpPart);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpAppr_c, inum,
    opt->trav.itpAppr);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpRefineCex_c, inum,
    opt->trav.itpRefineCex);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpStructAbstr_c, inum,
    opt->trav.itpStructAbstr);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpNew_c, inum,
    opt->trav.itpNew);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpExact_c, inum,
    opt->trav.itpExact);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpInductiveTo_c, inum,
    opt->trav.itpInductiveTo);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpInnerCones_c, inum,
    opt->trav.itpInnerCones);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpInitAbstr_c, inum,
    opt->trav.itpInitAbstr);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpEndAbstr_c, inum,
    opt->trav.itpEndAbstr);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpTrAbstr_c, inum,
    opt->trav.itpTrAbstr);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpReuseRings_c, inum,
    opt->trav.itpReuseRings);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpTuneForDepth_c, inum,
    opt->trav.itpTuneForDepth);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpBoundkOpt_c, inum,
    opt->trav.itpBoundkOpt);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpExactBoundDouble_c, inum,
    opt->trav.itpExactBoundDouble);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpConeOpt_c, inum,
    opt->trav.itpConeOpt);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpForceRun_c, inum,
    opt->trav.itpForceRun);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpMaxStepK_c, inum,
    opt->trav.itpMaxStepK);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpUseReached_c, inum,
    opt->trav.itpUseReached);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpCheckCompleteness_c, inum,
    opt->trav.itpCheckCompleteness);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpGfp_c, inum,
    opt->trav.itpGfp);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpConstrLevel_c, inum,
    opt->trav.itpConstrLevel);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpGenMaxIter_c, inum,
    opt->trav.itpGenMaxIter);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpEnToPlusImg_c, inum,
    opt->trav.itpEnToPlusImg);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpShareReached_c, inum,
    opt->trav.itpShareReached);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpUsePdrReached_c, inum,
    opt->trav.itpUsePdrReached);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpReImg_c, inum,
    opt->trav.itpReImg);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpFromNewLevel_c, inum,
    opt->trav.itpFromNewLevel);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpWeaken_c, inum,
    opt->trav.itpWeaken);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpStrengthen_c, inum,
    opt->trav.itpStrengthen);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpRpm_c, inum,
    opt->trav.itpRpm);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpStoreRings_c, pchar,
    opt->trav.itpStoreRings);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpTimeLimit_c, inum,
    opt->expt.itpTimeLimit);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravItpPeakAig_c, inum,
    opt->expt.itpPeakAig);

  /* igr */
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrSide_c, inum,
    opt->trav.igrSide);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrItpSeq_c, inum,
    opt->trav.igrItpSeq);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrRestart_c, inum,
    opt->trav.igrRestart);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrRewind_c, inum,
    opt->trav.igrRewind);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrRewindMinK_c, inum,
    opt->trav.igrRewindMinK);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrBwdRefine_c, inum,
    opt->trav.igrBwdRefine);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrGrowCone_c, inum,
    opt->trav.igrGrowCone);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrGrowConeMaxK_c, inum,
    opt->trav.igrGrowConeMaxK);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrGrowConeMax_c, fnum,
    opt->trav.igrGrowConeMax);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrUseRings_c, inum,
    opt->trav.igrUseRings);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrUseRingsStep_c, inum,
    opt->trav.igrUseRingsStep);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrUseBwdRings_c, inum,
    opt->trav.igrUseBwdRings);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrAssumeSafeBound_c, inum,
    opt->trav.igrAssumeSafeBound);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrConeSubsetBound_c, inum,
    opt->trav.igrConeSubsetBound);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrConeSubsetSizeTh_c, inum,
    opt->trav.igrConeSubsetSizeTh);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrConeSubsetPiRatio_c, fnum,
    opt->trav.igrConeSubsetPiRatio);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrConeSplitRatio_c, fnum,
    opt->trav.igrConeSplitRatio);

  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrMaxIter_c, inum,
    opt->trav.igrMaxIter);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrMaxExact_c, inum,
    opt->trav.igrMaxExact);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravIgrFwdBwd_c, inum,
    opt->trav.igrFwdBwd);

  /* pdr */
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPdrFwdEq_c, inum,
    opt->trav.pdrFwdEq);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPdrInf_c, inum,
    opt->trav.pdrInf);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPdrUnfold_c, inum,
    opt->trav.pdrUnfold);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPdrIndAssumeLits_c, inum,
    opt->trav.pdrIndAssumeLits);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPdrPostordCnf_c, inum,
    opt->trav.pdrPostordCnf);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPdrCexJustify_c, inum,
    opt->trav.pdrCexJustify);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPdrGenEffort_c, inum,
    opt->trav.pdrGenEffort);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPdrIncrementalTr_c, inum,
    opt->trav.pdrIncrementalTr);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPdrShareReached_c, inum,
    opt->trav.pdrShareReached);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPdrReuseRings_c, inum,
    opt->trav.pdrReuseRings);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPdrMaxBlock_c, inum,
    opt->trav.pdrMaxBlock);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPdrUsePgEncoding_c, inum,
    opt->trav.pdrUsePgEncoding);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPdrBumpActTopologically_c, inum,
    opt->trav.pdrBumpActTopologically);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPdrSpecializedCallsMask_c, inum,
    opt->trav.pdrSpecializedCallsMask);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPdrRestartStrategy_c, inum,
    opt->trav.pdrRestartStrategy);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPdrTimeLimit_c, inum,
    opt->expt.pdrTimeLimit);

  /* tr */
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravSortForBck_c, inum,
    opt->trav.sort_for_bck);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravTrPreimgSort_c, inum,
    opt->trav.trPreimgSort);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravClustThreshold_c, inum,
    opt->trav.clustTh);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravApprClustTh_c, inum,
    opt->trav.apprClustTh);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravImgClustTh_c, inum,
    opt->trav.imgClustTh);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravSquaring_c, inum,
    opt->trav.squaring);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravTrProj_c, inum,
    opt->trav.trproj);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravTrDpartTh_c, inum,
    opt->trav.trDpartTh);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravTrDpartVar_c, pchar,
    opt->trav.trDpartVar);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravTrAuxVarFile_c, pchar,
    opt->trav.auxVarFile);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravApproxTrClustFile_c, pchar,
    opt->trav.approxTrClustFile);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravBwdTrClustFile_c, pchar,
    opt->trav.bwdTrClustFile);

  /* bdd */
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravFromSelect_c, inum,
    (int)fromSelect);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravNoCheck_c, inum,
    opt->trav.noCheck);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravMismOptLevel_c, inum,
    opt->trav.mism_opt_level);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravGroupBad_c, inum,
    opt->trav.groupbad);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPrioritizedMc_c, inum,
    opt->trav.prioritizedMc);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPmcPrioThresh_c, inum,
    opt->trav.pmcPrioThresh);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPmcMaxNestedFull_c, inum,
    opt->trav.pmcMaxNestedFull);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPmcMaxNestedPart_c, inum,
    opt->trav.pmcMaxNestedPart);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravPmcNoSame_c, inum,
    opt->trav.pmcNoSame);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravCountReached_c, inum,
    opt->trav.countReached);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravAutoHint_c, inum,
    opt->trav.autoHint);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravHintsStrategy_c, inum,
    opt->trav.hintsStrategy);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravHintsTh_c, inum,
    opt->trav.hintsTh);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravHints_c, pvoid,
    opt->trav.hints);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravAuxPsSet_c, pvoid,
    opt->trav.auxPsSet);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravGfpApproxRange_c, inum,
    opt->trav.gfpApproxRange);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravUseMeta_c, inum,
    opt->trav.fbvMeta);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravMetaMethod_c, inum,
    (int)opt->trav.metaMethod);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravMetaMaxSmooth_c, inum,
    opt->common.metaMaxSmooth);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravConstrain_c, inum,
    opt->trav.constrain);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravConstrainLevel_c, inum,
    opt->trav.constrain_level);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravCheckFrozen_c, inum,
    opt->common.checkFrozen);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravApprOverlap_c, inum,
    opt->trav.apprOverlap);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravApprNP_c, inum,
    opt->trav.approxNP);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravApprMBM_c, inum,
    opt->trav.approxMBM);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravApprGrTh_c, inum,
    opt->trav.apprGrTh);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravApprPreimgTh_c, inum,
    opt->trav.approxPreimgTh);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravMaxFwdIter_c, inum,
    opt->trav.maxFwdIter);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravNExactIter_c, inum,
    opt->trav.nExactIter);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravSiftMaxTravIter_c, inum,
    opt->trav.siftMaxTravIter);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravImplications_c, inum,
    opt->trav.implications);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravImgDpartTh_c, inum,
    opt->trav.imgDpartTh);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravImgDpartSat_c, inum,
    opt->trav.imgDpartSat);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravBound_c, inum, opt->trav.bound);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravStrictBound_c, inum,
    opt->trav.strictBound);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravGuidedTrav_c, inum,
    opt->trav.guidedTrav);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravUnivQuantifyTh_c, inum,
    opt->trav.univQuantifyTh);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravOptImg_c, inum,
    opt->trav.opt_img);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravOptPreimg_c, inum,
    opt->trav.opt_preimg);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravBddTimeLimit_c, inum,
    opt->expt.bddTimeLimit);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravBddNodeLimit_c, inum,
    opt->expt.bddNodeLimit);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravWP_c, pchar, opt->trav.wP);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravWR_c, pchar, opt->trav.wR);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravWBR_c, pchar, opt->trav.wBR);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravWU_c, pchar, opt->trav.wU);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravWOrd_c, pchar, opt->trav.wOrd);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravRPlus_c, pchar,
    opt->trav.rPlus);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravInvarFile_c, pchar,
    opt->trav.invarFile);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravStoreCnf_c, pchar,
    opt->trav.storeCNF);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravStoreCnfTr_c, inum,
    opt->trav.storeCNFTR);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravStoreCnfMode_c, inum,
    (int)opt->trav.storeCNFmode);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravStoreCnfPhase_c, inum,
    opt->trav.storeCNFphase);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravMaxCnfLength_c, inum,
    opt->trav.maxCNFLength);

  /* bmc */
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravBmcItpRingsPeriod_c, inum,
    opt->trav.bmcItpRingsPeriod);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravBmcTrAbstrPeriod_c, inum,
    opt->trav.bmcTrAbstrPeriod);
  Pdtutil_OptListIns(pkgOpt, eTravOpt, Pdt_TravBmcTrAbstrInit_c, inum,
    opt->trav.bmcTrAbstrInit);

  
  return pkgOpt;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Pdtutil_OptList_t *
mcOpt2OptList(
  Fbv_Globals_t * opt
)
{
  Pdtutil_OptList_t *pkgOpt = Pdtutil_OptListCreate(Pdt_OptMc_c);

  // nxr: eliminare campi non utilizzati!

  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McVerbosity_c, inum, opt->verbosity);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McMethod_c, inum, opt->mc.method);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McAig_c, inum, opt->mc.aig);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McBmc_c, inum, opt->mc.bmc);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McPdr_c, inum, opt->mc.pdr);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McInductive_c, inum,
    opt->mc.inductive);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McInterpolant_c, inum,
    opt->mc.interpolant);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McLazy_c, inum, opt->mc.lazy);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McCustom_c, inum, opt->mc.custom);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McLemmas_c, inum, opt->mc.lemmas);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McLazyRate_c, fnum,
    opt->common.lazyRate);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McQbf_c, inum, opt->mc.qbf);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McDiameter_c, inum, opt->mc.diameter);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McCheckMult_c, inum,
    opt->mc.checkMult);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McExactTrav_c, inum,
    opt->mc.exactTrav);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McBmcStrategy_c, inum,
    opt->trav.bmcStrategy);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McInterpolantBmcSteps_c, inum,
    opt->trav.interpolantBmcSteps);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McBmcTr_c, inum, opt->trav.bmcTr);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McBmcStep_c, inum, opt->trav.bmcStep);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McBmcLearnStep_c, inum,
    opt->trav.bmcLearnStep);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McBmcFirst_c, inum,
    opt->trav.bmcFirst);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McBmcTe_c, inum,
    opt->trav.bmcTe);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McItpSeq_c, inum, opt->mc.itpSeq);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McItpSeqGroup_c, inum,
    opt->mc.itpSeqGroup);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McItpSeqSerial_c, fnum,
    opt->mc.itpSeqSerial);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McInitBound_c, inum,
    opt->mc.initBound);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McEnFastRun_c, inum,
    opt->mc.enFastRun);
  Pdtutil_OptListIns(pkgOpt, eMcOpt, Pdt_McFwdBwdFP_c, inum, opt->mc.fwdBwdFP);

  return pkgOpt;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Fbv_Globals_t *
new_settings(
)
{
  Fbv_Globals_t *opt = Pdtutil_Alloc(Fbv_Globals_t, 1);

  /*
   *  common options
   */

  opt->verbosity = 0;
  opt->expName = NULL;
  opt->mc.task = NULL;
  opt->mc.strategy = NULL;
  opt->expt.expertArgv = NULL;
  opt->expt.expertArgc = 0;

  /*
   *  ddi options
   */

  // queste non sono nei settings ...
  opt->ddi.siftTh = FBV_SIFT_THRESH;    /* in mgr */
  opt->ddi.siftTravTh = -1;
  opt->ddi.siftMaxTh = -1;      /* in mgr */
  opt->ddi.siftMaxCalls = -1;   /* in mgr */
  opt->trav.siftMaxTravIter = -1;   /* in mgr */
  opt->ddi.existClipTh = -1;    /* in mgr.exist */
  /* ddi.part */
  opt->ddi.exist_opt_thresh = 1000;
  opt->ddi.exist_opt_level = 2;
  /* ddi.aig */
  strcpy(opt->common.satSolver, "minisat"); // conflict with trav
  opt->common.lazyRate = 1.0;   // conflict with mc
  opt->common.aigAbcOpt = 3;
  opt->ddi.aigBddOpt = 0;
  opt->ddi.aigRedRem = 1;
  opt->ddi.aigRedRemPeriod = 1;
  opt->ddi.dynAbstrStrategy = 1;
  opt->ddi.ternaryAbstrStrategy = 1;
  opt->ddi.satTimeout = 1;
  opt->ddi.satIncremental = 1;
  opt->ddi.satVarActivity = 0;
  opt->ddi.satTimeLimit = 30;
  opt->ddi.itpOpt = 3;
  opt->ddi.itpActiveVars = 0;
  opt->ddi.itpAbc = 0;
  opt->ddi.itpTwice = 0;
  opt->ddi.itpCore = 0;
  opt->ddi.itpAigCore = 0;
  opt->ddi.itpMem = 4;
  opt->ddi.itpMap = -1;
  opt->ddi.itpStore = NULL;
  opt->ddi.itpLoad = NULL;
  opt->ddi.itpDrup = 0;
  opt->ddi.itpCompute = 0;
  opt->ddi.itpIteOptTh = 50000;
  opt->ddi.itpStructOdcTh = 1000;
  opt->ddi.itpPartTh = 100000;
  opt->ddi.itpStoreTh = 30000;
  opt->ddi.aigCnfLevel = 6;
  opt->ddi.nnfClustSimplify = 1;
  opt->ddi.itpCompact = 2;
  opt->ddi.itpClust = 1;
  opt->ddi.itpNorm = 1;
  opt->ddi.itpSimp = 1;


  /*
   *  fsm options
   */

  opt->fsm.insertCutLatches = -1;
  opt->fsm.cutAtAuxVar = -1;
  opt->fsm.cut = -1;
  opt->fsm.useAig = 0;
  opt->pre.periferalRetimeIterations = 0;
  opt->fsm.cegarStrategy = 1;

  //<marco> 06/04/2013
  /*
   *  sat options
   */
  opt->ddi.satBinClauses = 0;
  //<marco>

  /*
   *  tr options
   */

  /* tr.sort */
  strcpy(opt->tr.trSort, "iwls95");
  /* tr.cluster */
  opt->trav.clustTh = TRAV_CLUST_THRESH;
  /* tr.closure */
  opt->tr.squaringMaxIter = -1;
  /* tr.image */
  opt->trav.imgClustTh = TRAV_IMG_CLUST_THRESH;
  opt->tr.approxImgMin = -1;
  opt->tr.approxImgMax = -1;
  opt->tr.approxImgIter = 1;
  opt->common.checkFrozen = 0;  // --> conflict with trav.bdd
  /* tr.coi */
  opt->tr.coiLimit = 0;


  /*
   *  trav options
   */

  /* trav general (???) */
  strcpy(opt->common.satSolver, "minisat"); // conflict with ddi.aig
  opt->trav.cex = 0;
  opt->common.clk = NULL;
  opt->trav.lemmasTimeLimit = 120.0;
  opt->trav.lazyTimeLimit = 10000.0;
  opt->trav.travSelfTuning = 1;
  opt->trav.travConstrLevel = 1;
  opt->trav.mism_opt_level = 1;
  opt->trav.targetEn = 0;
  opt->trav.targetEnAppr = 0;
  opt->pre.targetDecomp = 0;

  /* trav.aig */
  opt->trav.dynAbstr = 0;
  opt->trav.dynAbstrInitIter = 0;
  opt->trav.implAbstr = 0;
  opt->trav.ternaryAbstr = 0;
  opt->trav.abstrRef = 0;
  opt->trav.abstrRefGla = 0;
  opt->trav.abstrRefItp = 0;
  opt->trav.abstrRefItpMaxIter = 4;
  opt->trav.trAbstrItp = 0;
  opt->trav.trAbstrItpOpt = 0;
  opt->trav.trAbstrItpFirstFwdStep = 0;
  opt->trav.trAbstrItpMaxFwdStep = 0;
  opt->trav.trAbstrItpLoad = NULL;
  opt->trav.trAbstrItpStore = NULL;
  opt->trav.inputRegs = 0;

  opt->trav.itpBdd = 0;
  opt->trav.itpPart = 0;
  opt->trav.itpAppr = 0;
  opt->trav.itpRefineCex = 0;
  opt->trav.itpExact = 0;
  opt->trav.itpNew = 0;
  opt->trav.itpStructAbstr = 0;
  opt->trav.itpInductiveTo = 0;
  opt->trav.itpInnerCones = 0;
  opt->trav.itpTrAbstr = 0;
  opt->trav.itpInitAbstr = 1;
  opt->trav.itpEndAbstr = 0;
  opt->trav.itpReuseRings = 0;
  opt->trav.itpTuneForDepth = 0;
  opt->trav.itpBoundkOpt = 0;
  opt->trav.itpExactBoundDouble = 0;
  opt->trav.itpConeOpt = 0;
  opt->trav.itpForceRun = -1;
  opt->trav.itpMaxStepK = -1;
  opt->trav.itpUseReached = 0;
  opt->trav.itpCheckCompleteness = 0;
  opt->trav.itpGfp = 0;
  opt->trav.itpConstrLevel = 4;
  opt->trav.itpGenMaxIter = 100;
  opt->trav.itpEnToPlusImg = 1;
  opt->trav.itpShareReached = 0;
  opt->trav.itpUsePdrReached = 0;
  opt->trav.itpReImg = 0;
  opt->trav.itpFromNewLevel = 0;
  opt->trav.itpWeaken = 0;
  opt->trav.itpStrengthen = 0;
  opt->trav.itpRpm = 0;

  opt->trav.igrUseRings = 0;
  opt->trav.igrUseRingsStep = 0;
  opt->trav.igrUseBwdRings = 0;
  opt->trav.igrAssumeSafeBound = 0;
  opt->trav.igrConeSubsetBound = -1;
  opt->trav.igrConeSubsetSizeTh = 100000;
  opt->trav.igrConeSubsetPiRatio = 0.5;
  opt->trav.igrConeSplitRatio = 1.0;


  opt->trav.igrSide = 3;
  opt->trav.igrRewind = 1;
  opt->trav.igrRewindMinK = -1;
  opt->trav.igrBwdRefine = 0;
  opt->trav.igrRestart = 0;
  opt->trav.igrGrowCone = 3;
  opt->trav.igrGrowConeMaxK = -1;
  opt->trav.igrGrowConeMax = 0;
  opt->trav.igrItpSeq = 0;
  opt->trav.igrMaxIter = 16;
  opt->trav.igrMaxExact = -1;
  opt->trav.igrFwdBwd = 0;

  opt->trav.pdrReuseRings = 0;
  opt->trav.pdrMaxBlock = 0;
  opt->trav.pdrFwdEq = 0;
  opt->trav.pdrInf = 0;
  opt->trav.pdrUnfold = 1;
  opt->trav.pdrIndAssumeLits = 0;
  opt->trav.pdrPostordCnf = 0;
  opt->trav.pdrCexJustify = 1;
  opt->trav.pdrGenEffort = 3;
  opt->trav.pdrIncrementalTr = 1;
  opt->trav.pdrShareReached = 0;
  opt->trav.pdrUsePgEncoding = 0;
  opt->trav.pdrBumpActTopologically = 0;
  opt->trav.pdrSpecializedCallsMask = 0;
  opt->trav.pdrRestartStrategy = 0;

  /* trav.bdd */
  opt->trav.noCheck = 0;
  opt->trav.prioritizedMc = 0;
  opt->trav.countReached = 0;
  opt->trav.autoHint = 0;
  opt->trav.hintsTh = 0;
  opt->trav.hintsStrategy = 1;
  opt->trav.hints = NULL;
  opt->trav.auxPsSet = NULL;
  opt->trav.gfpApproxRange = 1;
  opt->trav.fbvMeta = 0;
  opt->trav.metaMethod = Ddi_Meta_Size;
  opt->common.metaMaxSmooth = 0;
  opt->trav.trproj = 0;
  opt->trav.constrain = 0;
  opt->common.checkFrozen = 0;  // --> conflict with tr.image
  opt->trav.apprOverlap = 0;
  opt->trav.approxNP = 0;
  opt->trav.approxMBM = 0;
  opt->trav.apprGrTh = 0;
  opt->trav.apprClustTh = TRAV_CLUST_THRESH;
  opt->trav.approxPreimgTh = 500000;
  opt->trav.maxFwdIter = -1;
  opt->trav.nExactIter = 0;
  opt->trav.implications = 0;
  opt->trav.trDpartTh = 0;
  opt->trav.imgDpartTh = 0;
  opt->trav.imgDpartSat = 0;
  opt->trav.bound = -1;
  opt->trav.strictBound = 0;
  opt->trav.guidedTrav = 0;
  opt->trav.sort_for_bck = 1;
  opt->trav.univQuantifyTh = -1;
  opt->trav.trDpartVar = NULL;
  opt->trav.checkProof = 0;
  opt->trav.wP = NULL;
  opt->trav.wR = NULL;
  opt->trav.wC = NULL;
  opt->trav.wBR = NULL;
  opt->trav.wU = NULL;
  opt->trav.wOrd = NULL;
  opt->trav.rPlus = NULL;
  opt->trav.itpStoreRings = NULL;

  /*
   *  other options: TO BE MANAGED
   */


  /* expt/dynamic settings: decision taken by expt */
  opt->expt.bddNodeLimit = -1;
  opt->expt.bddMemoryLimit = -1;
  opt->expt.bddTimeLimit = -1;
  opt->expt.deepBound = 0;
  opt->expt.doRunBmc = 0;
  opt->expt.doRunItp = 0;
  opt->expt.doRunBddPfl = 0;
  opt->expt.doRunBmcPfl = 0;
  opt->expt.doRunItpPfl = 0;
  opt->expt.doRunPdrPfl = 0;
  opt->expt.doRunBdd = 0;
  opt->expt.doRunInd = 0;
  opt->expt.doRunLms = 0;
  opt->expt.doRunSim = 0;
  opt->expt.doRunSyn = 0;
  opt->expt.doRunLemmas = 0;
  opt->expt.bmcSteps = -1;
  opt->expt.itpBoundLimit = -1;
  opt->expt.lemmasSteps = 0;
  opt->expt.bmcSteps = -1;
  opt->expt.pflMaxThreads = -1;
  opt->expt.bmcTimeLimit = -1;
  opt->expt.bmcMemoryLimit = -1;
  opt->expt.bddTimeLimit = -1;
  opt->expt.bddMemoryLimit = -1;
  opt->expt.bddTimeLimit = -1;
  opt->expt.pdrTimeLimit = -1;
  opt->expt.itpTimeLimit = -1;
  opt->expt.itpPeakAig = -1;
  opt->expt.itpMemoryLimit = -1;
  opt->expt.totTimeLimit = -1;
  opt->expt.totMemoryLimit = -1;
  opt->expt.indSteps = -1;
  opt->expt.indTimeLimit = -1;
  opt->expt.thrdDisNum = 0;
  opt->expt.thrdEnNum = 0;
  opt->expt.ddiMgrAlign = 0;
  opt->expt.enItpPlus = 0;
  opt->expt.enPfl = 0;

  /* mc */
  opt->mc.method = Mc_VerifApproxFwdExactBck_c;
  opt->mc.aig = 0;
  opt->mc.bmc = -1;
  opt->mc.pdr = 0;
  opt->mc.bdd = 0;
  opt->mc.inductive = -1;
  opt->mc.interpolant = -1;
  opt->mc.custom = 0;
  opt->mc.lemmas = -1;
  opt->mc.lazy = -1;
  opt->mc.gfp = 0;
  opt->mc.exit_after_checkInv = 0;
  opt->mc.qbf = 0;
  opt->mc.diameter = -1;
  opt->mc.checkMult = 0;
  opt->mc.exactTrav = 0;

  opt->trav.bmcTr = -1;
  opt->trav.bmcStep = 1;
  opt->trav.bmcTe = 0;
  opt->trav.bmcFirst = 0;
  opt->trav.bmcStrategy = 1;
  opt->trav.interpolantBmcSteps = 0;
  opt->trav.bmcLearnStep = 4;
  opt->trav.bmcItpRingsPeriod = 0;
  opt->trav.bmcTrAbstrPeriod = 0;
  opt->trav.bmcTrAbstrInit = 0;

  opt->mc.itpSeq = 0;
  opt->mc.itpSeqGroup = 0;
  opt->mc.itpSeqSerial = 1.0;
  opt->mc.initBound = -1;
  opt->mc.enFastRun = -1;
  opt->mc.fwdBwdFP = 1;

  /* mc spec decomp */
  opt->pre.specConj = 0;
  opt->pre.specConjNum = 1;
  opt->pre.specDisjNum = 1;
  opt->pre.specPartMaxSize = 1;
  opt->pre.specPartMinSize = 1;
  opt->pre.specTotSize = 1;
  opt->pre.specDecompConstrTh = 50000;
  opt->pre.specDecompMin = 1;
  opt->pre.specDecompMax = 30;
  opt->pre.specDecompTot = 0;
  opt->pre.specDecompForce = 0;
  opt->pre.specDecompEq = 0;
  opt->pre.specDecompSort = 1;
  opt->pre.specDecompSat = 0;
  opt->pre.specDecompCore = 0;
  opt->pre.specSubsetByAntecedents = 0;
  opt->pre.specDecomp = -1;
  opt->pre.specDecompIndex = -1;


  /* dynamic for expt system */
  opt->mc.saveMethod = Mc_VerifMethodNone_c;
  opt->mc.saveComplInvarspec = 0;
  opt->trav.cntReachedOK = 0;

  /* pdt */
  opt->expt.expertLevel = 0;
  opt->mc.decompTimeLimit = 5000.0;
  opt->mc.abcOptFilename = NULL;
  opt->pre.speculateEqProp = 1;

  /* fsm opt on FILE */

  opt->fsm.nnf = 0;
  opt->pre.performAbcOpt = 0;
  opt->pre.abcSeqOpt = 0;
  opt->pre.iChecker = 0;
  opt->pre.iCheckerBound = 0;

  /* fsm load reductions/transformations */
  opt->pre.indEqDepth = -1;
  opt->pre.indEqMaxLevel = -1;
  opt->pre.indEqAssumeMiters = 0;
  opt->trav.strongLemmas = 0;
  opt->trav.implLemmas = 0;
  opt->trav.lemmasCare = 0;
  opt->pre.specEn = 0;
  opt->trav.noInit = 0;
  opt->pre.noTernary = 0;
  opt->pre.ternarySim = 3;
  opt->mc.combinationalProblem = 0;
  opt->pre.doCoi = 0;
  opt->pre.sccCoi = 1;
  opt->pre.coisort = 0;
  opt->pre.constrCoiLimit = 1;
  opt->pre.constrIsNeg = 0;
  opt->pre.noConstr = 0;
  opt->mc.cegar = 0;
  opt->mc.cegarPba = 0;
  opt->pre.lambdaLatches = 0;
  opt->pre.peripheralLatches = 0;
  opt->pre.peripheralLatchesNum = 0;
  opt->pre.forceInitStub = 0;
  opt->pre.initMinterm = -1;
  opt->pre.retime = 0;
  opt->pre.terminalScc = 0;
  opt->pre.impliedConstr = 0;
  opt->pre.reduceCustom = 0;
  opt->pre.findClk = 0;
  opt->pre.twoPhaseRed = 0;
  opt->pre.twoPhaseForced = 0;
  opt->pre.twoPhaseRedPlus = 0;
  opt->pre.twoPhaseOther = 0;
  opt->pre.twoPhaseDone = 0;
  opt->pre.clkPhaseSuggested = -1;
  opt->pre.clkVarFound = NULL;
  opt->mc.nogroup = 0;

  /* fsm get spec */
  opt->mc.idelta = -1;
  opt->mc.ilambda = 0;
  opt->mc.ilambdaCare = -1;
  opt->mc.nlambda = NULL;
  opt->mc.ninvar = NULL;
  opt->mc.compl_invarspec = 0;
  opt->mc.range = 0;
  opt->mc.all1 = 0;

  /* unused */
  opt->mc.tuneForEqCheck = 0;

  /* stats */
  opt->pre.nEqChecks = 0;
  opt->pre.nTotChecks = 0;
  opt->pre.relationalNs = 0;
  opt->pre.terminalSccNum = 0;
  opt->pre.impliedConstrNum = 0;
  opt->stats.startTime = 0;
  opt->stats.setupTime = 0;
  opt->stats.fbvStartTime = 0;
  opt->stats.fbvStopTime = 0;

  /* other data */
  opt->tr.auxFwdTr = NULL;
  opt->trav.univQuantifyVars = NULL;    /* unused */

  opt->stats.verificationOK = 0;

  /* fbv */
  opt->mc.writeBound = 0;
  opt->mc.useChildProc = 0;
  opt->mc.useChildCall = 0;
  opt->ddi.aigPartial = 0;
  opt->pre.coiSets = NULL;

  /* fbv bdd routines -> to trav... */
  opt->trav.univQuantifyDone = 0;
  opt->ddi.saveClipLevel = 0;
  opt->trav.trPreimgSort = 0;
  opt->trav.apprAig = 0;
  opt->mc.checkDead = 0;
  opt->trav.opt_img = 0;
  opt->trav.opt_preimg = 0;
  opt->trav.fromSel = 'b';
  opt->trav.groupbad = 0;
  opt->ddi.imgClipTh = FBV_IMG_CLIP_THRESH;
  opt->trav.squaring = 0;
  opt->trav.constrain_level = 1;
  opt->tr.en = NULL;
  opt->trav.pmcPrioThresh = 5000;
  opt->trav.pmcMaxNestedFull = 100;
  opt->trav.pmcMaxNestedPart = 5;
  opt->trav.pmcNoSame = 0;
  opt->fsm.auxVarsUsed = 0;
  opt->ddi.info_item_num = 0;
  opt->mc.fold = 0;
  opt->mc.foldInit = 0;

  /* tr preproc for bdds */
  opt->tr.invar_in_tr = 0;


  /* file/name management */

  opt->trav.auxVarFile = NULL;
  opt->trav.bwdTrClustFile = NULL;
  opt->trav.approxTrClustFile = NULL;
  opt->pre.createClusterFile = 0;
  opt->pre.clusterFile = NULL;
  opt->pre.methodCluster = 0;
  opt->pre.clustThDiff = 0;
  opt->pre.clustThShare = 0;
  opt->pre.thresholdCluster = 1;
  opt->tr.manualTrDpartFile = NULL;
  opt->fsm.manualAbstr = NULL;
  opt->trav.hintsFile = NULL;
  opt->trav.invarFile = NULL;
  opt->trav.writeInvar = NULL;
  opt->trav.writeProof = NULL;
  opt->mc.invSpec = NULL;
  opt->mc.ctlSpec = NULL;
  opt->mc.rInit = NULL;
  opt->mc.ord = NULL;
  opt->mc.checkInv = NULL;
  opt->mc.wFsm = NULL;
  opt->mc.wRes = NULL;
  opt->trav.rPlus = NULL;
  opt->trav.rPlusRings = NULL;

  opt->trav.storeCNF = NULL;
  opt->trav.storeCNFmode = 'B';
  opt->trav.storeCNFphase = 0;
  opt->trav.storeCNFTR = -1;
  opt->trav.maxCNFLength = 20;

  opt->pre.wrCoi = NULL;

  return opt;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Fbv_Globals_t *
FbvDupSettings(
  Fbv_Globals_t * opt0
)
{
  Fbv_Globals_t *opt = Pdtutil_Alloc(Fbv_Globals_t, 1);

  /*
   *  common options
   */

  *opt = *opt0;

  Pdtutil_Assert(opt0->tr.auxFwdTr == NULL, "tr is not null");
  Pdtutil_Assert(opt0->trav.univQuantifyVars == NULL, "uq is not null");
  Pdtutil_Assert(opt0->trav.hints == NULL, "hints opt field is not null");
  Pdtutil_Assert(opt0->trav.auxPsSet == NULL, "auxps opt field is not null");

  opt->mc.abcOptFilename = Pdtutil_StrDup(opt0->mc.abcOptFilename);
  opt->trav.trDpartVar = Pdtutil_StrDup(opt0->trav.trDpartVar);
  opt->mc.nlambda = Pdtutil_StrDup(opt0->mc.nlambda);
  opt->mc.ninvar = Pdtutil_StrDup(opt0->mc.ninvar);
  opt->ddi.itpStore = Pdtutil_StrDup(opt0->ddi.itpStore);
  opt->ddi.itpLoad = Pdtutil_StrDup(opt0->ddi.itpLoad);
  opt->trav.auxVarFile = Pdtutil_StrDup(opt0->trav.auxVarFile);
  opt->trav.bwdTrClustFile = Pdtutil_StrDup(opt0->trav.bwdTrClustFile);
  opt->trav.approxTrClustFile = Pdtutil_StrDup(opt0->trav.approxTrClustFile);
  opt->tr.manualTrDpartFile = Pdtutil_StrDup(opt0->tr.manualTrDpartFile);
  opt->fsm.manualAbstr = Pdtutil_StrDup(opt0->fsm.manualAbstr);
  opt->trav.hintsFile = Pdtutil_StrDup(opt0->trav.hintsFile);
  opt->trav.invarFile = Pdtutil_StrDup(opt0->trav.invarFile);
  opt->trav.writeInvar = Pdtutil_StrDup(opt0->trav.writeInvar);
  opt->trav.writeProof = Pdtutil_StrDup(opt0->trav.writeProof);
  opt->mc.invSpec = Pdtutil_StrDup(opt0->mc.invSpec);
  opt->mc.ctlSpec = Pdtutil_StrDup(opt0->mc.ctlSpec);
  opt->trav.rPlus = Pdtutil_StrDup(opt0->trav.rPlus);
  opt->trav.rPlusRings = Pdtutil_StrDup(opt0->trav.rPlusRings);
  opt->trav.itpStoreRings = Pdtutil_StrDup(opt0->trav.itpStoreRings);
  opt->mc.rInit = Pdtutil_StrDup(opt0->mc.rInit);
  opt->trav.wP = Pdtutil_StrDup(opt0->trav.wP);
  opt->trav.wR = Pdtutil_StrDup(opt0->trav.wR);
  opt->trav.wC = Pdtutil_StrDup(opt0->trav.wC);
  opt->trav.wBR = Pdtutil_StrDup(opt0->trav.wBR);
  opt->trav.wU = Pdtutil_StrDup(opt0->trav.wU);
  opt->mc.ord = Pdtutil_StrDup(opt0->mc.ord);
  opt->trav.wOrd = Pdtutil_StrDup(opt0->trav.wOrd);
  opt->mc.checkInv = Pdtutil_StrDup(opt0->mc.checkInv);
  opt->mc.wFsm = Pdtutil_StrDup(opt0->mc.wFsm);
  opt->mc.wRes = Pdtutil_StrDup(opt0->mc.wRes);
  opt->trav.storeCNF = Pdtutil_StrDup(opt0->trav.storeCNF);
  opt->tr.en = Pdtutil_StrDup(opt0->tr.en);
  opt->pre.wrCoi = Pdtutil_StrDup(opt0->pre.wrCoi);
  opt->mc.task = opt0->mc.task;
  opt->mc.strategy = Pdtutil_StrDup(opt0->mc.strategy);

  //opt->common.clk = NULL;
  //opt->pre.clkVarFound = NULL; //variabile DDI
  //opt->pre.coiSets = NULL;

  return opt;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
dispose_settings(
  Fbv_Globals_t * opt
)
{
  /* fare le free opportune */

  Tr_TrFree(opt->tr.auxFwdTr);
  Ddi_Free(opt->trav.univQuantifyVars);
  Pdtutil_Free(opt->mc.abcOptFilename);
  Pdtutil_Free(opt->trav.trDpartVar);
  Pdtutil_Free(opt->mc.nlambda);
  Pdtutil_Free(opt->mc.ninvar);
  Pdtutil_Free(opt->ddi.itpStore);
  Pdtutil_Free(opt->trav.auxVarFile);
  Pdtutil_Free(opt->trav.bwdTrClustFile);
  Pdtutil_Free(opt->trav.approxTrClustFile);
  Pdtutil_Free(opt->tr.manualTrDpartFile);
  Pdtutil_Free(opt->fsm.manualAbstr);
  Pdtutil_Free(opt->trav.hintsFile);
  Pdtutil_Free(opt->trav.invarFile);
  Pdtutil_Free(opt->mc.invSpec);
  Pdtutil_Free(opt->mc.ctlSpec);
  Pdtutil_Free(opt->trav.rPlus);
  Pdtutil_Free(opt->trav.rPlusRings);
  Pdtutil_Free(opt->trav.itpStoreRings);
  Pdtutil_Free(opt->trav.writeInvar);
  Pdtutil_Free(opt->trav.writeProof);
  Pdtutil_Free(opt->mc.rInit);
  Pdtutil_Free(opt->trav.wP);
  Pdtutil_Free(opt->trav.wR);
  Pdtutil_Free(opt->trav.wBR);
  Pdtutil_Free(opt->trav.wC);
  Pdtutil_Free(opt->trav.wU);
  Pdtutil_Free(opt->mc.ord);
  Pdtutil_Free(opt->trav.wOrd);
  Pdtutil_Free(opt->mc.wFsm);
  Pdtutil_Free(opt->mc.wRes);
  Pdtutil_Free(opt->mc.checkInv);
  Pdtutil_Free(opt->trav.storeCNF);
  //opt->common.clk = NULL;
  //opt->pre.clkVarFound = NULL; //variabile DDI
  Pdtutil_Free(opt->tr.en);
  Pdtutil_Free(opt->pre.wrCoi);
  Pdtutil_Free(opt->mc.strategy);
  Ddi_Free(opt->trav.hints);
  Ddi_Free(opt->trav.auxPsSet);
  //opt->pre.coiSets = NULL;
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
FbvFreeSettings(
  Fbv_Globals_t * opt
)
{
  dispose_settings(opt);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
checkEqGate(
  Ddi_Mgr_t * ddiMgr,
  Ddi_Varset_t * psv,
  bAigEdge_t baig,
  Ddi_Bddarray_t * delta,
  int chkDiff
)
{
  return 0;
}


int
Fbv_CheckFsmProp(
  Fsm_Mgr_t * fsmMgr
)
{
  Ddi_Var_t *pVar = Fsm_MgrReadPdtSpecVar(fsmMgr);
  Ddi_Var_t *cVar = Fsm_MgrReadPdtConstrVar(fsmMgr);
  Ddi_Bdd_t *prop = Ddi_BddDup(Fsm_MgrReadInvarspecBDD(fsmMgr));
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Bdd_t *cex = Fsm_MgrReadCexBDD(fsmMgr);
  int ret = -1;


  Ddi_BddComposeAcc(prop, ps, Fsm_MgrReadDeltaBDD(fsmMgr));
  Ddi_BddCofactorAcc(prop, pVar, 1);
  Ddi_BddCofactorAcc(prop, cVar, 1);
  if (Ddi_BddIsOne(prop)) {
    ret = 1;
  } else if (Ddi_BddIsZero(prop)) {
    ret = 0;
  }

  Ddi_Free(prop);
  return ret;
}

static int
checkFsmCex(
  Fsm_Mgr_t * fsmMgr,
  Trav_Mgr_t * travMgr
)
{
  Ddi_Var_t *pVar = Fsm_MgrReadPdtSpecVar(fsmMgr);
  Ddi_Var_t *cVar = Fsm_MgrReadPdtConstrVar(fsmMgr);
  Ddi_Bdd_t *prop = Ddi_BddDup(Fsm_MgrReadInvarspecBDD(fsmMgr));
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Bdd_t *cex = Fsm_MgrReadCexBDD(fsmMgr);
  int ret;


  Ddi_BddComposeAcc(prop, ps, Fsm_MgrReadDeltaBDD(fsmMgr));
  Ddi_BddCofactorAcc(prop, pVar, 1);
  Ddi_BddCofactorAcc(prop, cVar, 1);
  Ddi_BddSetPartConj(prop);
  Ddi_Bddarray_t *verifResOut =
    Trav_SimulateAndCheckProp(travMgr, fsmMgr, Fsm_MgrReadDeltaBDD(fsmMgr),
    prop, Fsm_MgrReadInitBDD(fsmMgr),
    Fsm_MgrReadInitStubBDD(fsmMgr), cex);
  int np = Ddi_BddarrayNum(verifResOut);

  Pdtutil_Assert(np == 1, "wrong prop num");
  ret = Ddi_BddIsZero(Ddi_BddarrayRead(verifResOut, 0));

  Ddi_Free(verifResOut);
  Ddi_Free(prop);

  return ret;
}

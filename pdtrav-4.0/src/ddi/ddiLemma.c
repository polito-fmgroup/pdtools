/**CFile***********************************************************************

  FileName    [ddiVar.c]

  PackageName [ddi]

  Synopsis    [Functions to manipulate BDD variables]

  Description []

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

#include "ddiInt.h"
#include "baigInt.h"
#include "Solver.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

typedef struct lemmaMgr_s lemmaMgr_t;

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

struct lemmaMgr_s {
  Ddi_Vararray_t **timeFrameStates;
  Ddi_Vararray_t **timeFrameInputs;
  Ddi_Bddarray_t **timeFrameNodes;
  Ddi_Bddarray_t *lemmaNodes;
  Ddi_Bddarray_t *lemmaClasses;
  Ddi_Bddarray_t *provedClasses;
  int *provedNodes;
  int *lemmaLits;
  short int ***lemmaImp;
  char **combImp;
  short int ***provedImp;
  short int **redImp;
  Ddi_Bddarray_t *delta;
  Ddi_Bdd_t *init;
  Ddi_Bdd_t *spec;
  Ddi_Bdd_t *care;
  Ddi_Bdd_t *invar;
  int nNodes;
  int nLemmas;
  int nEquiv;
  int nConst;
  int windowSize;
  int windowCount;
  int windowStart;
  Ddi_Bddarray_t *windowClasses;
  short int ***windowImp;
  int nProvedImp;
  int verbosity;
  unsigned long timeLimit;
  int maxFalse;
  Ddi_Bddarray_t *falseLemmas;
  int nVars;
  int doStrong;
};

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#define dMgrO(ddiMgr) ((ddiMgr)->settings.stdout)

#define aigIndexRead(aigMgr, f) bAig_AuxInt1(aigMgr, Ddi_BddToBaig(f))

#define aigIndexWrite(aigMgr, f, n) {bAig_AuxInt1(aigMgr, Ddi_BddToBaig(f)) = n;}

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static lemmaMgr_t * lemmaMgrInit(Ddi_Vararray_t *pi, Ddi_Vararray_t *ps, Ddi_Bddarray_t *delta, Ddi_Bdd_t *spec, Ddi_Bdd_t *care, Ddi_Bdd_t *invar, int bound, int verbosity, int do_imp, int do_strong);
static void lemmaMgrQuit(lemmaMgr_t *lemmaMgr, int depth);
static Ddi_Bddarray_t * Ddi_AigComputeInitialLemmaClasses(Ddi_Bddarray_t *lemmaNodes, Ddi_Bddarray_t *delta, Ddi_Vararray_t *psVars, Ddi_Bdd_t *initState, Ddi_Bddarray_t *initStub, Ddi_Bdd_t *spec, Ddi_Bdd_t *careBdd, short int ***lemmaImp, int sequential, int constrained, int do_skip, unsigned long simTimeLimit, unsigned long totalTimeLimit);
static void lemma_insert_hash(st_table *sig_hash, Ddi_Bdd_t *node, Ddi_AigSignatureArray_t *nodeSigs, Ddi_AigSignature_t *careSig);
static void lemmaSortClassAcc(Ddi_Bdd_t *eqClass);
static int expandLemmaClasses(Ddi_Bddarray_t *lemmaClasses, Ddi_Bddarray_t *represents, Ddi_Bddarray_t *equalities);
static int AigProveLemmaClassesMinisat(lemmaMgr_t *lemmaMgr, Ddi_Bdd_t *init, int idepth, int do_inductive, int do_bmc, int *result);
static int AigProveLemmaClassesWindow(lemmaMgr_t *lemmaMgr, Ddi_Bddarray_t *lemmaClasses, short int ***lemmaImp, int *lemmaEqLits, int ***lemmaImpLits, int idepth, Ddi_Bdd_t *init, Ddi_Bddarray_t *falseLemmas, float *satTime, float *optTime, float *disTime);
static int computeLemmaNum(Ddi_Bddarray_t *lemmaClasses, short int ***lemmaImp, int *nImpPtr, int nNodes);
static Ddi_Bdd_t *updateProvedLemmas(lemmaMgr_t *lemmaMgr, Ddi_Bddarray_t *lemmaClasses, short int ***lemmaImp, int *result);
static short int *** lemmaImpCopy(short int ***lemmaImp, int nNodes);
static int lemmaImpBasicImp(lemmaMgr_t *lemmaMgr, Ddi_Bddarray_t *lemmaArray, int nNodes);
static int lemmaImpSimplify(Ddi_Bddarray_t *lemmaClasses, short int ***lemmaImp, char **combImp, int nNodes, int onlyUnits);
static int lemmaStructImp(Ddi_Bddarray_t *lemmaClasses, short int ***lemmaImp, lemmaMgr_t *lemmaMgr, Solver& S, int *nTotImp, float *satTime);
static int lemmaImpClosure(short int ***lemmaImp, lemmaMgr_t *lemmaMgr, char **combImp, short int **redImp);
static char ** lemmaImpAlloc1d(int nNodes);
static short int ** lemmaImpAlloc2d(int nNodes);
static short int *** lemmaImpAlloc3d(int nNodes);
static int *** lemmaImpAllocNd(int nNodes, int depth);
static void lemmaImpFree3d(short int ***lemmaImp, int nNodes);
static void lemmaImpFree1d(char **lemmaImp, int nNodes);
static void lemmaImpFree2d(short int **lemmaImp, int nNodes);
static void lemmaImpFreeNd(int ***lemmaImp, int nNodes, int depth);
static int lemmaLoopFreeClauses(Ddi_Vararray_t **timeFrameStateVars, int idepth, int full, Solver& S);
static int enableFalseLemmaClause(lemmaMgr_t *lemmaMgr, Ddi_Bddarray_t *lemmaClasses, int *lemmaEqLits, short int ***lemmaImp, int ***lemmaImpLits, int idepth, int nNodes, Solver& S);
static int checkFalseLemmaClause(lemmaMgr_t *lemmaMgr, Ddi_Bddarray_t *lemmaClasses, int *lemmaEqLits, short int ***lemmaImp, int ***lemmaImpLits, int idepth, int nNodes, Solver& S);
static void lemmaAssumptionClauses(lemmaMgr_t *lemmaMgr, Ddi_Bddarray_t *lemmaClasses, int *lemmaEqLits, int idepth, Solver& S);
static void potentialLemmaClauses(lemmaMgr_t *lemmaMgr, Ddi_Bddarray_t *lemmaClasses, short int ***lemmaImp, int **lemmaEqLits, int ***lemmaImpLits, int idepth, Solver& S);
static void potentialLemmaClassClauses(Ddi_Bdd_t *lemmaClass, Ddi_Bddarray_t **timeFrameNodes, int **lemmaEqLits, int idepth, Solver& S, int constant);
static void provedLemmaClauses(lemmaMgr_t *lemmaMgr, Ddi_Bddarray_t *provedClasses, int **lemmaEqLits, short int ***provedImp, int idepth, Solver& S);
static void provedLemmaClassClauses(Ddi_Bdd_t *lemmaClass, Ddi_Bddarray_t **timeFrameNodes, int **lemmaEqLits, int assumeFrames, Solver& S, int constant);
static int lemmaImpConsistencyCheck(Ddi_Bddarray_t *lemmaClasses, short int ***lemmaImp, short int **redImp, int nNodes);
static int lemmaImpWrite(Ddi_Bddarray_t *lemmaClasses, short int ***lemmaImp, int nNodes);
static int lemmaImpTransitiveCheck(Ddi_Bddarray_t *lemmaClasses, short int ***lemmaImp, int **countImp, int nNodes);
static int lemmaImpReduce(lemmaMgr_t *lemmaMgr, Ddi_Bddarray_t *lemmaClasses, short int ***lemmaImp, int nNodes);
static int disprovedLemmaClauses(lemmaMgr_t *lemmaMgr, Ddi_Bddarray_t *lemmaClasses, short int ***lemmaImp, int *lemmaEqLits, int ***lemmaImpLits, int idepth, Solver& S, vec<Lit>& cex, Ddi_Bddarray_t *falseLemmas);
static void lemmaImpAddEdge(short int ***lemmaImp, short int **redImp, int pClassId, int pSign, int rClassId, int rSign, int nNodes, int do_red);
static void lemmaImpRemoveEdge(short int ***lemmaImp, short int **redImp, int pClassId, int pSign, int rClassId, int rSign, int classesNum, int nNodes);
static int timeFrameLemmaClauses(Ddi_Bddarray_t **timeFrameNodes, int **lemmaEqLits, int ***lemmaImpLits, int idepth, Solver& S, int pos0, int inv0, int pos1, int inv1, int implication, int action);
static int enableLemmaClauses(Ddi_Mgr_t *ddm, Solver& S, int lit0, int lit1, int equality, int complement);
static void printLemmaClasses(Ddi_Bddarray_t *lemmaClasses, char *msg);
static int AigOneDistanceSimulation(Ddi_Bddarray_t **timeFrameNodes, Ddi_Bddarray_t *delta, Ddi_Vararray_t **psVars, Ddi_Vararray_t **piVars, Ddi_Bdd_t *initState, Ddi_Bdd_t *careBdd, Ddi_Bddarray_t *lemmaClasses, int **lemmaEqLits, short int ***lemmaImp, char **combImp, int ***lemmaImpLits, Solver& S, int assumeFrames, int trace);
static Ddi_Vararray_t * newTimeFrameVars(Ddi_Vararray_t *baseVars, char *nameSuffix);


/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Ddi_LemmaGenerateClasses (
  Ddi_Vararray_t *pi,
  Ddi_Vararray_t *ps,
  Ddi_Vararray_t *ns,
  Ddi_Bddarray_t *delta,
  Ddi_Bdd_t *initBdd,
  Ddi_Bddarray_t *initStub,
  Ddi_Bdd_t *invarAig,
  Ddi_Bdd_t *spec,
  Ddi_Bdd_t *careAig,
  Ddi_Bddarray_t *constants,
  int bound,
  int max_false,
  int do_compaction,
  int do_implication,
  int do_strong,
  int verbosity,
  int *timeLimitPtr,
  Ddi_Bddarray_t *reprs, 
  Ddi_Bddarray_t *equals,
  Ddi_Bddarray_t *false_lemmas,
  int *result,
  int *done_bound,
  int *again_ptr
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(delta);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  Ddi_Bddarray_t *lemmaArray=NULL, *piLits, *psLits, *nsLits;
  Ddi_Bdd_t *lemmaClass, *lemma, *node, *one, *tr_i, *init_i;
  int do_imp=do_implication, numRef=Ddi_MgrReadExtRef(ddm), again=1;
  Ddi_Bdd_t *care, *invar, *init = initBdd ? Ddi_BddMakeMono(initBdd) : NULL;
  int i, j, k, pos, sign0, sign1, nNodes, num, nEquiv, depth=bound;
  Ddi_Vararray_t *pi_i, *ps_i, *ns_i;
  long currTime, startTime=util_cpu_time();
  unsigned long simTimeLimit=~0;
  lemmaMgr_t *lemmaMgr;
  bAigEdge_t baig;

  assert(!do_imp);
  lemmaMgr = lemmaMgrInit(pi, ps, delta, spec, careAig, invarAig, bound, verbosity, do_imp, do_strong);
  lemmaMgr->maxFalse = max_false;
  lemmaMgr->falseLemmas = false_lemmas;
  if (*timeLimitPtr > 0) {
    lemmaMgr->timeLimit = util_cpu_time() + *timeLimitPtr;
#if 1
    if (*timeLimitPtr > 20*1000) {
      simTimeLimit = util_cpu_time() + 10*1000; /* max 10 s for simulation */
    } else {
      simTimeLimit = util_cpu_time() + (*timeLimitPtr)/2;
    }
#else
    simTimeLimit = util_cpu_time() + 10*1000; /* max 10 s for simulation */
    if (simTimeLimit > lemmaMgr->timeLimit) {
      simTimeLimit = lemmaMgr->timeLimit;
    }
#endif
  }
  nNodes = Ddi_BddarrayNum(lemmaMgr->lemmaNodes);
  care = lemmaMgr->care;
  invar = lemmaMgr->invar;

  /* combinational lemmas */
  Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelNone_c,
    fprintf(dMgrO(ddm),"*** Gen. lemmas iteration 0 ***\n"));
  if (!bound) {
    ddm->settings.aig.lemma_done_bound = -1;
    currTime = util_cpu_time();
    lemmaMgr->lemmaClasses = Ddi_AigComputeInitialLemmaClasses(lemmaMgr->lemmaNodes, 
            delta, ps, NULL/*init*/, NULL/*initStub*/, lemmaMgr->spec, invar/*care*/, 
            lemmaMgr->lemmaImp, 0/*sequential*/, invar?1:0/*constrained*/, 1, 
            simTimeLimit, lemmaMgr->timeLimit);
    currTime = util_cpu_time() - currTime;
    Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelNone_c,
      fprintf(dMgrO(ddm),"Simulation time: %s\n", util_print_time(currTime)));
    if (util_cpu_time() > lemmaMgr->timeLimit) {
      fprintf(dMgrO(ddm),"Abort due to time limit\n");
      *timeLimitPtr = 0;
      goto lemmaEnd;
    }
    for (i=Ddi_BddarrayNum(lemmaMgr->lemmaClasses)-1; i>0; i--) {
      lemmaClass = Ddi_BddarrayRead(lemmaMgr->lemmaClasses, i);
      if (Ddi_BddPartNum(lemmaClass) == 1) {
	Ddi_BddarrayQuickRemove(lemmaMgr->lemmaClasses, i);
      } else {
	lemmaSortClassAcc(lemmaClass);
      }
    }

    AigProveLemmaClassesMinisat(lemmaMgr, NULL, 0, 1, 0, result);
    if (*result) {
      fprintf(dMgrO(ddm),"*** Property combinationally PROVED\n");
    } else if (util_cpu_time() > lemmaMgr->timeLimit) {
      fprintf(dMgrO(ddm),"Abort due to time limit\n");
      *timeLimitPtr = 0;
    }
    goto lemmaEnd;
  }

  /* sequential lemmas */
  currTime = util_cpu_time();
  lemmaMgr->lemmaClasses = 
    Ddi_AigComputeInitialLemmaClasses(lemmaMgr->lemmaNodes, delta,
    ps, init, initStub, lemmaMgr->spec, invar/*care*/, lemmaMgr->lemmaImp, 
			       1, 1, 1, simTimeLimit, lemmaMgr->timeLimit);
  currTime = util_cpu_time() - currTime;
  Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelNone_c,
    fprintf(dMgrO(ddm),"Simulation time: %s\n", util_print_time(currTime)));
  if (lemmaMgr->lemmaClasses == NULL) {
    fprintf(dMgrO(ddm),"*** Property DISPROVED by random simulation\n");
    *result = -1;
    goto lemmaEnd;
  }
  if (util_cpu_time() > lemmaMgr->timeLimit) {
    fprintf(dMgrO(ddm),"Abort due to time limit\n");
    *timeLimitPtr = 0;
    goto lemmaEnd;
  }
  for (i=Ddi_BddarrayNum(lemmaMgr->lemmaClasses)-1; i>0; i--) {
    lemmaClass = Ddi_BddarrayRead(lemmaMgr->lemmaClasses, i);
    if (Ddi_BddPartNum(lemmaClass) == 1) {
      Ddi_BddarrayQuickRemove(lemmaMgr->lemmaClasses, i);
    } else {
      lemmaSortClassAcc(lemmaClass);
    }
  }

  if (initStub) {
    assert(!initBdd);
    init = Ddi_BddMakeConstAig(ddm, 1);
    psLits = Ddi_BddarrayMakeLiteralsAig(ps, 1);
    for (i=0; i<Ddi_BddarrayNum(initStub); i++) {
      tr_i = Ddi_BddXnor(Ddi_BddarrayRead(psLits, i), Ddi_BddarrayRead(initStub, i));
      Ddi_BddAndAcc(init, tr_i);
      Ddi_Free(tr_i);
    }
    Ddi_Free(psLits);
  }
  Ddi_BddSetAig(init);

  /* do bmc analysis (0-depth) */
  AigProveLemmaClassesMinisat(lemmaMgr, init, 0, 0, 1, result);
  if (*result < 0) {
    fprintf(dMgrO(ddm),"*** Property DISPROVED by BMC\n");
    goto lemmaEnd;
  }
  if (util_cpu_time() > lemmaMgr->timeLimit) {
    fprintf(dMgrO(ddm),"Abort due to time limit\n");
    *timeLimitPtr = 0;
    goto lemmaEnd;
  }

  /* assume and prove (k-inductive) steps */
  nEquiv = again = i = 0; 
  while (*timeLimitPtr && (i<bound) && !*result) {
    char suffix[10];
    sprintf(suffix, "%d", ++i);
    if (i > depth) {
      lemmaMgr->timeFrameStates = Pdtutil_Realloc(Ddi_Vararray_t *, lemmaMgr->timeFrameStates, 2*depth+1);
      lemmaMgr->timeFrameInputs = Pdtutil_Realloc(Ddi_Vararray_t *, lemmaMgr->timeFrameInputs, 2*depth+1);
      lemmaMgr->timeFrameNodes = Pdtutil_Realloc(Ddi_Bddarray_t *, lemmaMgr->timeFrameNodes, 2*depth+1);
      for (j=depth+1; j<=2*depth; j++) {
	lemmaMgr->timeFrameInputs[j] = NULL;
	lemmaMgr->timeFrameStates[j] = NULL;
	lemmaMgr->timeFrameNodes[j] = NULL;
      }
      depth = 2*depth;
    }
    lemmaMgr->timeFrameInputs[i] = newTimeFrameVars(pi, suffix);
    lemmaMgr->timeFrameStates[i] = Ddi_VararrayDup(ps);
    Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelNone_c,
      fprintf(dMgrO(ddm),"*** Gen. lemmas iteration %d ***\n", i));
    lemmaMgr->nLemmas = computeLemmaNum(lemmaMgr->lemmaClasses, NULL, NULL, nNodes);
    lemmaMgr->nLemmas += Ddi_BddPartNum(Ddi_BddarrayRead(lemmaMgr->lemmaClasses, 0)); 
    again = AigProveLemmaClassesMinisat(lemmaMgr, init, i, 
                                        (i==1 || i%2==0)/*lemma_done_bound*/, i<bound, result);
    nEquiv += again;
    if (*result == 1) {
      fprintf(dMgrO(ddm),"*** Property inductively PROVED\n");
    } else if (*result == -1) {
      fprintf(dMgrO(ddm),"*** Property DISPROVED by BMC\n");
    } else if (util_cpu_time() > lemmaMgr->timeLimit) {
      fprintf(dMgrO(ddm),"Abort due to time limit\n");
      *timeLimitPtr = 0; 
      break;
    }
  }

 lemmaEnd: 
#if 0
  if (bound) {
    /* (partially) verify lemma correctness */
    Ddi_Bddarray_t *lemmasBaseUnroll;
    int nTotLemmas, nFalseLemmas=0, nTotFalseLemmas=0;
    int *enableLemma;
    Ddi_Bdd_t *lemmaSpec;
    char suffix[10];

    for (i=1; i<=depth; i++) {
      sprintf(suffix, "%d", i);
      if (lemmaMgr->timeFrameInputs[i] == NULL) {
	lemmaMgr->timeFrameInputs[i] = newTimeFrameVars(pi, suffix);
      }
      lemmaMgr->timeFrameStates[i] = newTimeFrameVars(ps, suffix);
    }

    lemmaArray = Ddi_BddarrayAlloc(ddm, 0);
    lemmaClass = Ddi_BddarrayRead(lemmaMgr->provedClasses, 0);
    for (j=0; j<Ddi_BddPartNum(lemmaClass); j++) {
      node = Ddi_BddPartRead(lemmaClass, j);
      Ddi_BddarrayInsertLast(lemmaArray, node);
    }
    for (i=1; i<Ddi_BddarrayNum(lemmaMgr->provedClasses); i++) {
      lemmaClass = Ddi_BddarrayRead(lemmaMgr->provedClasses, i);
      lemmaSpec = Ddi_BddPartRead(lemmaClass, 0);
      for (j=1; j<Ddi_BddPartNum(lemmaClass); j++) {
        node = Ddi_BddDup(Ddi_BddPartRead(lemmaClass, j));
        Ddi_BddXnorAcc(node, lemmaSpec);
        Ddi_BddarrayInsertLast(lemmaArray, node);
        Ddi_Free(node);
      }
    }
    nTotLemmas = Ddi_BddarrayNum(lemmaArray);
    enableLemma = Pdtutil_Alloc(int, nTotLemmas);
    for (i=0; i<nTotLemmas; i++) {
      enableLemma[i] = 1;
    }

    lemmasBaseUnroll = Ddi_BddarrayDup(lemmaArray);
    for (i=0; i<=depth; i++) {
      int *resultArray;

      init_i = Ddi_BddDup(init);
      if (i > 0) {
	Ddi_Vararray_t *pi_i = lemmaMgr->timeFrameInputs[i];
	Ddi_Bddarray_t *piLits = Ddi_BddarrayMakeLiteralsAig(pi_i,1);
	Ddi_Vararray_t *ps_i = lemmaMgr->timeFrameStates[i];
	Ddi_Bddarray_t *psLits = Ddi_BddarrayMakeLiteralsAig(ps_i,1);
	Ddi_Vararray_t *ps_j = lemmaMgr->timeFrameStates[i-1];

	Ddi_Bddarray_t *newdelta = Ddi_BddarrayDup(delta);
	Ddi_AigarrayComposeAcc(newdelta,pi,piLits);
	Ddi_AigarrayComposeAcc(newdelta,ps,psLits);
	Ddi_AigarrayComposeAcc(lemmasBaseUnroll,ps_j,newdelta);
	Ddi_BddComposeAcc(init_i, ps, psLits);
	Ddi_Free(newdelta);
	Ddi_Free(psLits);
	Ddi_Free(piLits);
      }

      resultArray = Ddi_AigProveLemmasMinisat(init_i,lemmasBaseUnroll,NULL);

      Ddi_Free(init_i);
      nFalseLemmas = 0;
      for (j=0; j<nTotLemmas; j++) {
	if (enableLemma[j]) {
	  enableLemma[j] = resultArray[j];
	  if (resultArray[j] == 0) {
	    nFalseLemmas++;
	  }
	}
      }
      fprintf(dMgrO(ddm),"Lemma BMC (depth %d): %d/%d lemmas disproved\n", i, nFalseLemmas, nTotLemmas);
      nTotFalseLemmas += nFalseLemmas;
      Pdtutil_Free(resultArray);
    }
    Ddi_Free(lemmasBaseUnroll);
    Pdtutil_Free(enableLemma);
    Ddi_Free(lemmaArray);
    fprintf(dMgrO(ddm),"Lemma BMC: %d/%d total lemmas disproved\n", nTotFalseLemmas, nTotLemmas);
  }
#endif

  /* DA FINIRE DI METTERE A POSTO */
  //exit(8);
  *again_ptr = again;
  *done_bound = ddm->settings.aig.lemma_done_bound;

  /* build resulting lemma array */
  nEquiv = computeLemmaNum(lemmaMgr->provedClasses, NULL, NULL, nNodes);
  nEquiv += Ddi_BddPartNum(Ddi_BddarrayRead(lemmaMgr->provedClasses, 0));
  //printf("Number of proved equivalences: %d\n", nEquiv);
  //if (Ddi_BddarrayNum(lemmaMgr->provedClasses)+Ddi_BddPartNum(Ddi_BddarrayRead(lemmaMgr->provedClasses, 0)) > 1) {
  if (do_compaction) {
    expandLemmaClasses(lemmaMgr->provedClasses, reprs, equals);
  } 
  if (bound) {
    lemmaArray = Ddi_BddarrayAlloc(ddm, 0);
    if (nEquiv && *result >= 0) {
      /* save equivalences */
      lemmaClass = Ddi_BddarrayRead(lemmaMgr->provedClasses, 0);
      for (j=0; j<Ddi_BddPartNum(lemmaClass); j++) {
	lemma = Ddi_BddPartRead(lemmaClass,j);
	Ddi_BddarrayInsertLast(lemmaArray,lemma);
      }
      for (i=1; i<Ddi_BddarrayNum(lemmaMgr->provedClasses); i++) {
	lemmaClass = Ddi_BddarrayRead(lemmaMgr->provedClasses, i);
	Ddi_Bdd_t *f_i = Ddi_BddPartRead(lemmaClass, 0);
	for (j=1; j<Ddi_BddPartNum(lemmaClass); j++) {
	  Ddi_Bdd_t *f_j = Ddi_BddPartRead(lemmaClass, j);
	  lemma = Ddi_BddXnor(f_i, f_j);
	  Ddi_BddarrayInsertLast(lemmaArray, lemma);
	  Ddi_Free(lemma);
	}
      }
    }
  }
  if (constants && !*result && lemmaMgr->lemmaClasses) {
    lemmaClass = Ddi_BddarrayRead(lemmaMgr->lemmaClasses, 0);
    for (i=0; i<Ddi_BddPartNum(lemmaClass); i++) {
      lemma = Ddi_BddPartRead(lemmaClass, i);
      Ddi_BddarrayInsertLast(constants, lemma);
    }
  }

  Ddi_Free(init);
  lemmaMgrQuit(lemmaMgr, depth);
  //assert(Ddi_MgrCheckExtRef(ddm, numRef+(lemmaArray!=NULL)));

  if (1&&verbosity) {
    fprintf(dMgrO(ddm),"Lemma generation time: %s\n", 
            util_print_time((util_cpu_time()-startTime)));
  }
  if (*timeLimitPtr) {
    *timeLimitPtr -= util_cpu_time()-startTime;
  }

  return (lemmaArray);
}


/**Function********************************************************************
  Synopsis    [Aig to BDD conversion]
  Description [Aig to BDD conversion]
  SideEffects []
  SeeAlso     []
******************************************************************************/
char **
Ddi_AigCombinationalImplications(
  Ddi_Bddarray_t *lemmasBase,
  int complement
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmasBase);
  bAig_Manager_t  *bmgr = ddm->aig.mgr;
  int i, j, n=Ddi_BddarrayNum(lemmasBase);
  int nImpl;
  Solver    S;
  vec<Lit> lits;
  vec<Lit> assumps;
  int nVars = Ddi_MgrReadNumVars(ddm);
  bAig_array_t *visitedNodes = bAigArrayAlloc();
  Ddi_Bddarray_t *nodesAig;
  Ddi_AigSignatureArray_t *varSigs, *nodeSigs, *nodeSigsCompl;
  Ddi_AigSignature_t oneSig, zeroSig, careSig;
  char **impl = Pdtutil_Alloc(char *, n);

  for (i=0; i<n; i++) {
    impl[i]=Pdtutil_Alloc(char, n);
    for (j=0; j<n; j++) {
      impl[i][j]=-1;
    }
  }


  varSigs = DdiAigSignatureArrayAlloc(nVars);
  DdiSetSignaturesRandom(varSigs,nVars);
  DdiSetSignatureConstant(&oneSig,1);
  DdiSetSignatureConstant(&zeroSig,0);
  DdiSetSignatureConstant(&careSig,1);

  nodesAig = Ddi_AigarrayNodes(lemmasBase,-1);
  for (i=0; i<Ddi_BddarrayNum(nodesAig); i++) {
    bAigEdge_t baig = Ddi_BddToBaig(Ddi_BddarrayRead(nodesAig,i));
    bAigArrayWriteLast(visitedNodes,baig);
  }
  Ddi_Free(nodesAig);

  nodeSigs = DdiAigEvalSignature(ddm,visitedNodes,bAig_NULL,0,varSigs);
  nodeSigsCompl = DdiAigSignatureArrayAlloc(visitedNodes->num);

  for (i=0; i<visitedNodes->num; i++) {
    bAigEdge_t baig = visitedNodes->nodes[i];
    Pdtutil_Assert(bAig_AuxInt(bmgr,baig) == -1, "wrong auxId field");
    bAig_AuxInt(bmgr,baig) = i;
    nodeSigsCompl->sArray[i] = nodeSigs->sArray[i];
    DdiComplementSignature(&(nodeSigsCompl->sArray[i]));
  }


  for (i=nImpl=0; i<Ddi_BddarrayNum(lemmasBase); i++) {
    bAigEdge_t base_i = Ddi_BddToBaig(Ddi_BddarrayRead(lemmasBase,i));
    int i_sig = bAig_AuxInt(bmgr,base_i);
    Ddi_AigSignature_t iSig = nodeSigs->sArray[i_sig];
    if (bAig_NodeIsInverted(base_i)) {
      iSig = nodeSigsCompl->sArray[i_sig];
    }
    for (j=0; j<i; j++) {
      bAigEdge_t base_j = Ddi_BddToBaig(Ddi_BddarrayRead(lemmasBase,j));
      int j_sig = bAig_AuxInt(bmgr,base_j);
      Ddi_AigSignature_t jSig = nodeSigs->sArray[j_sig];
      Ddi_AigSignature_t jSigCompl = nodeSigsCompl->sArray[j_sig];
      if (bAig_NodeIsInverted(base_j)^complement) {
        jSig = nodeSigsCompl->sArray[j_sig];
        jSigCompl = nodeSigs->sArray[j_sig];
      }
      if (!DdiImplySignatures(&iSig,&jSig,&careSig)) {
        impl[i][j]=0;nImpl++;
      }
    }
  }

  fprintf(dMgrO(ddm),"disabled %d/%d impl checks by simulation\n", nImpl, n*(n-1));

  DdiAig2CnfIdInit(ddm);
  assumps.clear();

  for (i=0; i<n; i++) {
    Ddi_Bdd_t *f = Ddi_BddarrayRead(lemmasBase,i);
    MinisatClauses(S,f,NULL,NULL,1);
  }

  assumps.push();
  assumps.push();

  for (i=nImpl=0; i<n; i++) {
    int sat;
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(lemmasBase,i);
    int id_i = aig2CnfId(bmgr,Ddi_BddToBaig(f_i));
    assumps[0] = MinisatLit(id_i);
    for (j=0; j<n; j++) {
      if (j!=i) {
        Ddi_Bdd_t *f_j = Ddi_BddarrayRead(lemmasBase,j);
        int id_j = aig2CnfId(bmgr,Ddi_BddToBaig(f_j));
        assumps[1] = MinisatLit((complement?id_j:-id_j));
	if (impl[i][j]==0) {
	  continue;
	}
	impl[i][j]=0;
        if ((sat = S.okay())) {
          sat = S.solve(assumps);
        }
        if (sat==0) {
 	  impl[i][j]=1; 
   	  nImpl++;
        }
	else {
	  int k;
          for (k=j+1; k<n; k++) {
	    if (impl[i][k]<0) {
              Ddi_Bdd_t *f_k = Ddi_BddarrayRead(lemmasBase,k);
              int lit = aigCnfLit(S, f_k);
              int var = abs(lit) - 1;
              Pdtutil_Assert(S.model[var] != l_Undef, "undefined var in cex");
              if (S.model[var] == (complement?l_True:l_False)) {
  	        impl[i][k]=0; 
	      }
	    }
	  }
	}
      }
    }
  }

  DdiAig2CnfIdClose(ddm);

  fprintf(dMgrO(ddm),"found %d/%d combinational implications\n", nImpl, n*(n-1));

  bAigArrayFree(visitedNodes);

  return (impl);
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
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
static lemmaMgr_t *
lemmaMgrInit (
  Ddi_Vararray_t *pi,
  Ddi_Vararray_t *ps,
  Ddi_Bddarray_t *delta,
  Ddi_Bdd_t *spec,
  Ddi_Bdd_t *care,
  Ddi_Bdd_t *invar,
  int bound,
  int verbosity,
  int do_imp,
  int do_strong
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(delta);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  Ddi_Vararray_t **timeFrameStates = NULL, **timeFrameInputs = NULL;
  Ddi_Bddarray_t **timeFrameNodes = NULL, *lemmasBase, *deltaPlus;
  Ddi_Bdd_t *lemmaClass, *lemmaSpec=NULL, *node, *one;
  Ddi_Bddarray_t *lemmaClasses, *provedClasses, *sortedNodes;
  short int ***lemmaImp=NULL, **redImp=NULL, ***provedImp=NULL;
  char **combImp=NULL;
  lemmaMgr_t *lemmaMgr=NULL;
  int i, nNodes, nVars=0;
  Ddi_Var_t *v;
  bAigEdge_t baig;

  /* generate initial node array */
  deltaPlus = Ddi_BddarrayDup(delta);
  if (0) {
    for (i=0; i<Ddi_VararrayNum(ps); i++) {
      v = Ddi_VararrayRead(ps, i);
      if (strcmp(Ddi_VarName(v), "PDT_BDD_INVAR_VAR$PS") == 0) {
	Ddi_BddarrayQuickRemove(deltaPlus, i);
	break;
      }
    }
  }
  if (spec) {
    baig = bAig_NonInvertedEdge(Ddi_BddToBaig(spec));
    if (0&&bAig_isVarNode(aigMgr, baig)) {
      /* it should be a secodary input */
      for (i=0; i<Ddi_VararrayNum(ps); i++) {
	node = Ddi_BddMakeLiteralAig(Ddi_VararrayRead(ps, i), 1);
	if (bAig_NonInvertedEdge(Ddi_BddToBaig(node)) == baig) {
	  break;
	}
	Ddi_Free(node);
      }
      Ddi_Free(node);
      assert(i < Ddi_VararrayNum(ps));
      lemmaSpec = Ddi_BddDup(Ddi_BddarrayRead(delta, i));
      if (aigIsInv(spec)) {
	Ddi_BddNotAcc(lemmaSpec);
      }
    } else {
      lemmaSpec = Ddi_BddDup(spec);
      Ddi_BddarrayInsertLast(deltaPlus, lemmaSpec);
    }
  }
  lemmasBase = Ddi_AigarrayNodes(deltaPlus, -1);
  Ddi_Free(deltaPlus);

  one = Ddi_BddMakeConstAig(ddm, 1);
  Ddi_BddarrayInsert(lemmasBase, 0, one);
  baig = Ddi_BddToBaig(one);
  assert(bAig_AuxInt(aigMgr, baig) == -1);
  bAig_AuxInt(aigMgr, baig) = 0;
  
  /* remove possible node repetitions */
  for (i=Ddi_BddarrayNum(lemmasBase)-1; i>0; i--) {
    node = Ddi_BddarrayRead(lemmasBase, i);
    if (bAig_AuxInt(aigMgr, Ddi_BddToBaig(node)) == -1) {
      bAig_AuxInt(aigMgr, Ddi_BddToBaig(node)) = 0;
      if (aigIsInv(node)) {
	Ddi_BddNotAcc(node);
      }
    } else {
      assert(bAig_AuxInt(aigMgr, Ddi_BddToBaig(node)) == 0);
      Ddi_BddarrayQuickRemove(lemmasBase, i); 
    }
  }
  sortedNodes = Ddi_AigarraySortByLevel(lemmasBase);
  Ddi_Free(lemmasBase);
  lemmasBase = sortedNodes; 
  nNodes = Ddi_BddarrayNum(lemmasBase);

  /* restore auxInt field, prepare the index field */
  for (i=0; i<Ddi_BddarrayNum(lemmasBase); i++) {
    node = Ddi_BddarrayRead(lemmasBase, i);
    assert(i ? !aigIsInv(node) : aigIsInv(node));
    baig = Ddi_BddToBaig(node);
    bAig_AuxInt(aigMgr, baig) = -1;
    aigIndexWrite(aigMgr, node, i);
    if (0&&bAig_isVarNode(aigMgr, baig)) 
      printf("Baig %d - aig1: %d\n", baig, i);
    nVars += (i==0) || bAig_isVarNode(aigMgr, baig);
  }
  Ddi_Free(one);
  if (0&&verbosity) {
    fprintf(dMgrO(ddm),"Total nodes for invariant candidates: %d\n", Ddi_BddarrayNum(lemmasBase));
  }

  timeFrameStates = Pdtutil_Alloc(Ddi_Vararray_t *, bound+1);
  timeFrameInputs = Pdtutil_Alloc(Ddi_Vararray_t *, bound+1);
  timeFrameNodes = Pdtutil_Alloc(Ddi_Bddarray_t *, bound+1);
  timeFrameStates[0] = Ddi_VararrayDup(ps);
  timeFrameInputs[0] = Ddi_VararrayDup(pi);
  timeFrameNodes[0] = NULL;
  for (i=1; i<=bound; i++) {
    timeFrameStates[i] = NULL;
    timeFrameInputs[i] = NULL;
    timeFrameNodes[i] = NULL;
  }

  provedClasses = Ddi_BddarrayAlloc(ddm, 0); 
  lemmaClass = Ddi_BddMakePartConjVoid(ddm);
  Ddi_BddarrayInsertLast(provedClasses, lemmaClass);
  Ddi_Free(lemmaClass);

  lemmaMgr = (lemmaMgr_t *)malloc(sizeof(lemmaMgr_t));
  lemmaMgr->timeFrameStates = timeFrameStates;
  lemmaMgr->timeFrameInputs = timeFrameInputs;
  lemmaMgr->timeFrameNodes = timeFrameNodes;
  lemmaMgr->lemmaNodes = lemmasBase;
  lemmaMgr->lemmaClasses = lemmaClasses;
  lemmaMgr->provedClasses = provedClasses;
  lemmaMgr->provedNodes = (int *)malloc(nNodes*sizeof(int));
  memset(lemmaMgr->provedNodes, -1, nNodes*sizeof(int));
  lemmaMgr->lemmaLits = (int *)malloc(nNodes*sizeof(int));
  lemmaMgr->lemmaImp = lemmaImp;
  lemmaMgr->redImp = redImp;
  lemmaMgr->combImp = combImp;
  lemmaMgr->provedImp = provedImp;
  lemmaMgr->delta = Ddi_BddarrayDup(delta);
  lemmaMgr->spec = lemmaSpec;
  lemmaMgr->nNodes = Ddi_BddarrayNum(lemmasBase);
  lemmaMgr->nLemmas = lemmaMgr->nEquiv = lemmaMgr->nConst = 0;
  lemmaMgr->nProvedImp = 0;
  lemmaMgr->windowSize = 0;
  lemmaMgr->windowCount = 0;
  lemmaMgr->windowStart = 0;
  lemmaMgr->windowClasses = NULL;
  lemmaMgr->windowImp = NULL;
  lemmaMgr->verbosity = 1 && verbosity;
  lemmaMgr->timeLimit = ~0;
  lemmaMgr->nVars = nVars;
  lemmaMgr->doStrong = do_strong;

  if (invar) {
    lemmaMgr->invar = Ddi_BddDup(invar);
  } else {
    lemmaMgr->invar = Ddi_BddMakeConstAig(ddm, 1);    
  }
  if (care) {
    lemmaMgr->care = Ddi_BddDup(care);
  } else {
    lemmaMgr->care = Ddi_BddMakeConstAig(ddm, 1);    
  }
  Ddi_BddAndAcc(lemmaMgr->care, lemmaMgr->invar);

  return lemmaMgr;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
lemmaMgrQuit (
  lemmaMgr_t *lemmaMgr,
  int depth
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaMgr->delta);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  Ddi_Bdd_t *node, *lemmaClass;
  int i, nNodes;

  nNodes = Ddi_BddarrayNum(lemmaMgr->lemmaNodes);
  for (i=0; i<nNodes; i++) {
     node = Ddi_BddarrayRead(lemmaMgr->lemmaNodes, i);
     //     aigIndexWrite(aigMgr, node, -1);
  }
  for (i=0; i<=depth; i++) {
    Ddi_Free(lemmaMgr->timeFrameStates[i]);
    Ddi_Free(lemmaMgr->timeFrameInputs[i]);
  }
  Pdtutil_Free(lemmaMgr->timeFrameStates);
  Pdtutil_Free(lemmaMgr->timeFrameInputs);
  Pdtutil_Free(lemmaMgr->timeFrameNodes);
  Pdtutil_Free(lemmaMgr->provedNodes);
  Pdtutil_Free(lemmaMgr->lemmaLits);
  Ddi_Free(lemmaMgr->spec);
  Ddi_Free(lemmaMgr->care);
  Ddi_Free(lemmaMgr->invar);
  Ddi_Free(lemmaMgr->delta);
  Ddi_Free(lemmaMgr->lemmaNodes);
  Ddi_Free(lemmaMgr->provedClasses);
  Ddi_Free(lemmaMgr->lemmaClasses);

  Pdtutil_Free(lemmaMgr);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
expandLemmaClasses (
  Ddi_Bddarray_t *lemmaClasses,
  Ddi_Bddarray_t *represents,
  Ddi_Bddarray_t *equalities
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaClasses);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  Ddi_Bdd_t *lemmaClass, *node, *leader;
  int i, j, pos0, pos1;

  lemmaClass = Ddi_BddarrayRead(lemmaClasses, 0);
  leader = Ddi_MgrReadOne(ddm);
  for (j=0; j<Ddi_BddPartNum(lemmaClass); j++) {
    node = Ddi_BddPartRead(lemmaClass, j);
    Ddi_BddarrayInsertLast(represents, leader);
    Ddi_BddarrayInsertLast(equalities, node);
  }

  for (i=1; i<Ddi_BddarrayNum(lemmaClasses); i++) {
    lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
    leader = Ddi_BddPartRead(lemmaClass, 0);
    pos0 = aigIndexRead(aigMgr, leader);
    for (j=1; j<Ddi_BddPartNum(lemmaClass); j++) {
      node = Ddi_BddPartRead(lemmaClass, j);
      pos1 = aigIndexRead(aigMgr, node);
      assert(pos0 < pos1);
      Ddi_BddarrayInsertLast(represents, leader);
      Ddi_BddarrayInsertLast(equalities, node);
    }
  }

  return 1;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
AigProveLemmaClassesMinisat (
  lemmaMgr_t *lemmaMgr,
  Ddi_Bdd_t *init,
  int idepth,
  int do_inductive,
  int do_bmc,
  int *result
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaMgr->lemmaNodes);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  int i, j, end, verbosity, size, nNodes, nConst, nEquiv, nLemmas, again=1;
  int *lemmaEqLits=NULL, ***lemmaImpLits=NULL, sizeFactor=2;
  int nrun, tot_run, iter, numFalse, maxFalse=idepth ? lemmaMgr->maxFalse : 0;
  short int **redImp=lemmaMgr->redImp, ***windowImp=NULL;
  Ddi_Bddarray_t *lemmaNodes, *indClasses=NULL, *falseLemmas=NULL;
  Ddi_Bdd_t *lemmaClass, *node;
  Ddi_Bddarray_t *lemmaClasses=lemmaMgr->lemmaClasses;
  Ddi_Bddarray_t *provedClasses=lemmaMgr->provedClasses;
  long startTime, endTime;
  float satTime, optTime, disTime, cpuTime;
  bAigEdge_t baig;

  lemmaNodes = lemmaMgr->lemmaNodes;
  nNodes = Ddi_BddarrayNum(lemmaNodes);
  verbosity = lemmaMgr->verbosity;
  lemmaEqLits = lemmaMgr->lemmaLits;
  memset(lemmaEqLits, 0, nNodes*sizeof(int));

  if (do_inductive) {
    ddm->settings.aig.lemma_done_bound++;
    satTime = optTime = disTime = tot_run = iter = 0;
    startTime = util_cpu_time();
    lemmaClasses = Ddi_BddarrayDup(lemmaMgr->lemmaClasses);
    lemmaMgr->nConst = nConst = Ddi_BddPartNum(Ddi_BddarrayRead(lemmaClasses, 0));
    lemmaMgr->nEquiv = nEquiv = computeLemmaNum(lemmaClasses, NULL, NULL, nNodes);
    Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelNone_c,
      fprintf(dMgrO(ddm),"Potential eq.: %d+%d\n", nConst, nEquiv));

    do {
      iter++;
      if (verbosity) {
	nConst = lemmaMgr->nConst;
	nEquiv = lemmaMgr->nEquiv;
	printf("[%2d] %d+%d: ", iter, nConst, nEquiv);
	fflush(NULL);
      }
      if (maxFalse > 0) {
	indClasses = Ddi_BddarrayDup(lemmaClasses);
	falseLemmas = Ddi_BddarrayAlloc(ddm, 0);
      }
      nrun = AigProveLemmaClassesWindow(lemmaMgr, lemmaClasses, NULL, lemmaEqLits, 
					lemmaImpLits, idepth, NULL, falseLemmas, 
					&satTime, &optTime, &disTime);
      tot_run += nrun;
      numFalse = 0;
      if (maxFalse > 0) {
	numFalse = Ddi_BddarrayNum(falseLemmas);
      }
      if (nrun>0 && numFalse>0 && numFalse<=maxFalse) {
	Ddi_Free(lemmaClasses);
	lemmaClasses = indClasses;
	Ddi_BddarrayAppend(lemmaMgr->falseLemmas, falseLemmas);
	nrun = 1;
	if (verbosity) {
	  fprintf(dMgrO(ddm),"@ (%d+%d)\n", lemmaMgr->nConst, lemmaMgr->nEquiv);
	  fflush(NULL);
	}
	lemmaMgr->nConst = nConst;
	lemmaMgr->nEquiv = nEquiv;
      } else {
	Ddi_Free(indClasses);
	if (verbosity) {
	  fprintf(dMgrO(ddm)," %d+%d\n", lemmaMgr->nConst, lemmaMgr->nEquiv);
	  fflush(NULL);
	}
      }
      Ddi_Free(falseLemmas);
    } while (nrun > 1);

    if (nrun == 1) {
      /* update classes */
      lemmaMgr->spec = updateProvedLemmas(lemmaMgr, lemmaClasses, NULL, result);
      endTime = util_cpu_time();
      cpuTime = (endTime-startTime)/1000.0;
      Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelNone_c,
        fprintf(dMgrO(ddm),"%s equiv.: %d+%d in ",
          numFalse ? "Specul." : "Proved", lemmaMgr->nConst, lemmaMgr->nEquiv);
        fprintf(dMgrO(ddm),"%.2f s (%d iter, %d total run)\n",
         cpuTime, iter, tot_run));
      //printf("(%d iter, %d total run, %.2f s, %.2f s, %.2f)\n", iter, tot_run, satTime, optTime, disTime);
      again = (lemmaMgr->nConst+lemmaMgr->nEquiv) && idepth;
    }
    Ddi_Free(lemmaClasses);
  }

  if ((do_bmc) && !*result && util_cpu_time()<lemmaMgr->timeLimit) {
    satTime = tot_run = iter = 0;
    startTime = util_cpu_time();
    lemmaClasses = lemmaMgr->lemmaClasses;
    lemmaMgr->nConst = nConst = Ddi_BddPartNum(Ddi_BddarrayRead(lemmaClasses, 0));
    lemmaMgr->nEquiv = nEquiv = computeLemmaNum(lemmaClasses, NULL, NULL, nNodes);
    nLemmas = nConst+nEquiv;
    Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelNone_c,
      fprintf(dMgrO(ddm),"Potential eq.: %d+%d\n", nConst, nEquiv));
    
    do {
      iter++;
      if (verbosity) {
	printf("[%2d] %d+%d: ", iter, lemmaMgr->nConst, lemmaMgr->nEquiv);
      }
      nrun = AigProveLemmaClassesWindow(lemmaMgr, lemmaClasses, NULL, lemmaEqLits, 
					lemmaImpLits, idepth, init, NULL,
					&satTime, &optTime, &disTime);
      if (verbosity) {
	printf(" %d+%d\n", lemmaMgr->nConst, lemmaMgr->nEquiv);
      }
      tot_run += nrun;
    } while (nrun > 1);

    if (nrun == 1) {
      endTime = util_cpu_time();
      cpuTime = (endTime-startTime)/1000.0;
      lemmaMgr->nConst = nConst = Ddi_BddPartNum(Ddi_BddarrayRead(lemmaClasses, 0));
      lemmaMgr->nEquiv = nEquiv = computeLemmaNum(lemmaClasses, NULL, NULL, nNodes);
      Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelNone_c,
        fprintf(dMgrO(ddm),"Remaining eq.: %d+%d in ", nConst, nEquiv);
        fprintf(dMgrO(ddm),"%.2f s (%d iter, %d total run, %.2f s)\n",
          cpuTime, iter, nrun, satTime));

      if (0 && idepth==1) {
	int pos, pos0;
	lemmaClass = Ddi_BddarrayRead(lemmaMgr->lemmaClasses, 0);
	for (j=0; j<Ddi_BddPartNum(lemmaClass); j++) {
	  node = Ddi_BddPartRead(lemmaClass, j);
	  pos = aigIndexRead(aigMgr, node);
	  fprintf(dMgrO(ddm),"0 - %d\n", aigIsInv(node) ? pos : -pos);
	}
	for (i=1; i<Ddi_BddarrayNum(lemmaMgr->lemmaClasses); i++) {
	  lemmaClass = Ddi_BddarrayRead(lemmaMgr->lemmaClasses, i);
	  node = Ddi_BddPartRead(lemmaClass, 0);
	  pos = aigIndexRead(aigMgr, node);
	  pos0 = aigIsInv(node) ? -pos : pos;
	  for (j=1; j<Ddi_BddPartNum(lemmaClass); j++) {
	    node = Ddi_BddPartRead(lemmaClass, j);
	    pos = aigIndexRead(aigMgr, node);
	    fprintf(dMgrO(ddm),"%d - %d\n", pos0, aigIsInv(node) ? -pos : pos);
	  }
	}
      }


    }
    if (lemmaMgr->spec) {
      /* check whether the property has been disproved */
      lemmaClass = Ddi_BddarrayRead(lemmaClasses, 0);
      baig = bAig_NonInvertedEdge(Ddi_BddToBaig(lemmaMgr->spec));
      for (j=0; j<Ddi_BddPartNum(lemmaClass); j++) {
	node = Ddi_BddPartRead(lemmaClass, j);
	if (baig == bAig_NonInvertedEdge(Ddi_BddToBaig(node))) {
	  break;
	}
      }
      *result = -(j==Ddi_BddPartNum(lemmaClass));
    }
  }

  return again;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
AigProveLemmaClassesWindow (
  lemmaMgr_t *lemmaMgr,
  Ddi_Bddarray_t *lemmaClasses,
  short int ***lemmaImp,
  int *lemmaEqLits,
  int ***lemmaImpLits,
  int idepth,
  Ddi_Bdd_t *init,
  Ddi_Bddarray_t *falseLemmas,
  float *satTime,
  float *optTime,
  float *disTime
)
{
  int i, j, k, verbose, nrun, nNodes, sat, timeOut, falsified, numFalse;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaClasses);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  Ddi_Bdd_t *lemmaClass, *lemma, *node;
  Ddi_Bdd_t *care, *care_i, *invar, *invar_i;
  long elapsedTime, startTime, endTime;
  float currTime=0, timeLimit, localTimeLimit;
  Ddi_Bddarray_t *reprs, *equals, *delta, *delta_i, *deltaTot, *piLits;
  Ddi_Vararray_t *pi, *pi_i, *ps, *ps_i;
  int pos, cexUsed, fake, numFake, numFailInSequence, maxFake=8, 
    numRef=Ddi_MgrReadExtRef(ddm);
  vec<Lit> assumps, lits, cex;
  Solver S;
  Ddi_Bdd_t *constr = NULL;
  Ddi_Bdd_t *constrTot = NULL;
  int *checkEq = NULL;
  int checkRes=0;

  elapsedTime = util_cpu_time();
  if (elapsedTime > lemmaMgr->timeLimit) {
    return 0;
  }
  nNodes = Ddi_BddarrayNum(lemmaMgr->lemmaNodes);
  verbose = lemmaMgr->verbosity;

  /* generate the optimized "unroll" (by composition) */
  startTime = util_cpu_time();
  reprs = Ddi_BddarrayAlloc(ddm, 0);
  equals = Ddi_BddarrayAlloc(ddm, 0);
  expandLemmaClasses(lemmaMgr->provedClasses, reprs, equals);
  expandLemmaClasses(lemmaClasses, reprs, equals);
  //checkEq = Pdtutil_Alloc(int,Ddi_BddarrayNum(reprs));
  lemmaMgr->timeFrameNodes[0] = Ddi_BddarrayDup(lemmaMgr->lemmaNodes);
  Ddi_AigarrayOptByEquivFwd(lemmaMgr->timeFrameNodes[0], reprs, equals, 
			    checkEq, 0);
  //Pdtutil_Free(checkEq);
  delta = Ddi_BddarrayAlloc(ddm, 0);
  for (i=0; i<Ddi_BddarrayNum(lemmaMgr->delta); i++) {
    node = Ddi_BddarrayRead(lemmaMgr->delta, i);
    pos = aigIndexRead(aigMgr, node);
    if (pos) {
      node = Ddi_BddDup(Ddi_BddarrayRead(lemmaMgr->timeFrameNodes[0], pos));
      if (aigIsInv(Ddi_BddarrayRead(lemmaMgr->delta, i))) {
	Ddi_BddNotAcc(node);
      }
      Ddi_BddarrayInsertLast(delta, node);
      Ddi_Free(node);
    } else {
      Ddi_BddarrayInsertLast(delta, node);
    }
  }
  if (0 && verbose) {
    fprintf(dMgrO(ddm),"%d -> %d ", Ddi_BddarraySize(lemmaMgr->delta), Ddi_BddarraySize(delta));
    if (!idepth) {
      fprintf(dMgrO(ddm),"(%d) ", Ddi_BddarraySize(lemmaMgr->timeFrameNodes[0]));
    }
    //printf("%d -> %d ", Ddi_BddarraySize(lemmaMgr->lemmaNodes), Ddi_BddarraySize(lemmaMgr->timeFrameNodes[0]));
    fflush(NULL);
  }

  //Ddi_Free(delta);
  //delta = Ddi_BddarrayDup(lemmaMgr->delta);
  //Ddi_AigarrayOptByEquivFwd(delta, reprs, equals, NULL);

  //care = Ddi_BddMakeConstAig(ddm, 1);
  //invar = Ddi_BddMakeConstAig(ddm, 1);
  ps_i = lemmaMgr->timeFrameStates[idepth];
  deltaTot = Ddi_BddarrayMakeLiteralsAig(ps_i, 1);
  pi = lemmaMgr->timeFrameInputs[0];
  ps = lemmaMgr->timeFrameStates[0];

  if (checkRes)
  {
    constr = Ddi_BddMakeConstAig(ddm, 1);
    constrTot = Ddi_BddMakeConstAig(ddm, 1);
    int ii;
    for (ii=0;ii<Ddi_BddarrayNum(reprs); ii++) {
      Ddi_Bdd_t *r_i = Ddi_BddarrayRead(reprs,ii);
      Ddi_Bdd_t *e_i = Ddi_BddarrayRead(equals,ii);
      Ddi_Bdd_t *eq = Ddi_BddXnor(r_i,e_i);
      Ddi_BddAndAcc(constr,eq);
      Ddi_Free(eq);
    }
  }

  for (i=idepth; i>0; i--) {
    delta_i = Ddi_BddarrayDup(delta);
    pi_i = lemmaMgr->timeFrameInputs[i];
    piLits = Ddi_BddarrayMakeLiteralsAig(pi_i, 1);
    Ddi_BddarrayComposeAcc(delta_i, pi, piLits);
    Ddi_BddarrayComposeAcc(delta_i, ps, deltaTot);

    if (checkRes) {
      Ddi_Bdd_t *constr_i = Ddi_BddDup(constr);
      Ddi_BddComposeAcc(constr_i, pi, piLits);
      Ddi_BddComposeAcc(constr_i, ps, deltaTot);
      Ddi_BddAndAcc(constrTot,constr_i);
      Ddi_Free(constr_i);
    }

    lemmaMgr->timeFrameNodes[i] = Ddi_BddarrayDup(lemmaMgr->timeFrameNodes[0]);
    Ddi_BddarrayComposeAcc(lemmaMgr->timeFrameNodes[i], pi, piLits);
    Ddi_BddarrayComposeAcc(lemmaMgr->timeFrameNodes[i], ps, deltaTot);
    Ddi_Free(piLits);
    Ddi_Free(deltaTot);
    deltaTot = delta_i;
  }

  if (checkRes && reprs!=NULL && Ddi_BddarrayNum(reprs)>0) {
    Ddi_Bddarray_t *r0 = Ddi_BddarrayDup(reprs);
    Ddi_Bddarray_t *e0 = Ddi_BddarrayDup(equals);
    int ii, diffCnt = 0, diffCnt1 = 0, nr = Ddi_BddarrayNum(reprs);
    for (ii=0;ii<Ddi_BddarrayNum(r0); ii++) {
      Ddi_Bdd_t *r_i = Ddi_BddarrayRead(r0,ii);
      Ddi_BddSetAig(r_i);
    }
    Ddi_BddarrayComposeAcc(r0, ps, deltaTot);
    Ddi_BddarrayComposeAcc(e0, ps, deltaTot);
    for (ii=0;ii<Ddi_BddarrayNum(r0); ii++) {
      Ddi_Bdd_t *r0_i = Ddi_BddarrayRead(r0,ii);
      Ddi_Bdd_t *e0_i = Ddi_BddarrayRead(e0,ii);
      Ddi_Bdd_t *diff = Ddi_BddXor(r0_i,e0_i);
      if (Ddi_AigSatAnd(diff,init,NULL)) {
	diffCnt1++;
      }
      else if (Ddi_AigSatAnd(diff,init,constrTot)) {
	diffCnt++;
        diffCnt1++;
      }
      Ddi_Free(diff);
    }
    fprintf(dMgrO(ddm),"\n(%s) DIFF FOUND %d(%d)/%d\n", init?"BMC":"IND", diffCnt,diffCnt1,nr);
    Ddi_Free(r0);
    Ddi_Free(e0);
  }

  if (checkRes) {
    Ddi_Free(constr);
    Ddi_Free(constrTot);
  }

  if (0 && idepth && verbose) {
    fprintf(dMgrO(ddm),"(%d+%d", Ddi_BddarraySize(lemmaMgr->timeFrameNodes[0]),Ddi_BddarraySize(deltaTot));
    fflush(NULL);
  }

  if (checkRes && reprs!=NULL && Ddi_BddarrayNum(reprs)>0) {
    int cntDiff;
    static int myCntCalls = 0;

    Ddi_Bddarray_t *r0 = Ddi_BddarrayDup(reprs);
    Ddi_Bddarray_t *e0 = Ddi_BddarrayDup(equals);
    Ddi_BddarrayComposeAcc(r0, ps, deltaTot);
    Ddi_BddarrayComposeAcc(e0, ps, deltaTot);

    if (1) {
      cntDiff = 0;
      for (i=0; i<Ddi_BddarrayNum(reprs); i++) {
        Ddi_Bdd_t *r0_i = Ddi_BddarrayRead(r0, i);
        Ddi_Bdd_t *e0_i = Ddi_BddarrayRead(e0, i);
        Ddi_Bdd_t *diff = Ddi_BddXor(r0_i, e0_i);
	if (init) {
	  if (Ddi_AigSatAnd(diff,init,NULL)) cntDiff++;
	} else {
	  if (Ddi_AigSat(diff)) cntDiff++;
	}
        Ddi_Free(diff);
      }
      fprintf(dMgrO(ddm)," [%d / %d] ", cntDiff, myCntCalls++);
    }

    Ddi_Free(r0);
    Ddi_Free(e0);
  } 

  Ddi_BddarrayComposeAcc(lemmaMgr->timeFrameNodes[0], ps, deltaTot);
  if (0 && idepth && verbose) {
    fprintf(dMgrO(ddm),"=%d) ", Ddi_BddarraySize(lemmaMgr->timeFrameNodes[0]));
    fflush(NULL);
  }
  Ddi_Free(delta);
  Ddi_Free(deltaTot);
  Ddi_Free(reprs);
  Ddi_Free(equals);

  /* translate into cnf */
  DdiAig2CnfIdInit(ddm);

  /* add clauses for all time frame nodes */
  if (1) {
    Ddi_Bddarray_t *allNodes = Ddi_BddarrayDup(lemmaMgr->timeFrameNodes[0]);
    Ddi_Bdd_t *auxAigPart;
    int lit;
    for (k=1; k<=idepth; k++) {
      // temporary until protection from multiple load implemented
      Ddi_BddarrayAppend(allNodes, lemmaMgr->timeFrameNodes[k]);
    }
    auxAigPart = Ddi_BddMakePartConjFromArray(allNodes);
    MinisatClauses(S, auxAigPart, NULL, NULL, 1);
    Ddi_Free(auxAigPart);
    Ddi_Free(allNodes);
    /* force constant 1 to be true */
    auxAigPart = Ddi_BddarrayRead(lemmaMgr->timeFrameNodes[0], 0);
    lit = aigCnfLit(S, auxAigPart);
    MinisatClause1(S, lits, lit);
  }
  /* add clauses for variable nodes (needed by simulation) */
  //for (i=0; i<lemmaMgr->nVars; i++) {
  //  node = Ddi_BddarrayRead(lemmaMgr->timeFrameNodes[0], i);
  //  MinisatClauses(S, node, NULL, NULL, i>0);
  //}
  /* add clauses for init states if necessary (bmc step only) */
  if (init) {
    MinisatClauses(S, init, NULL, NULL, 0);
  } 
  /* add clauses for care/invar constraints if necessary */
  if (lemmaMgr->invar) {
    MinisatClauses(S, lemmaMgr->invar, NULL, NULL, 0);
  }
  if (idepth && lemmaMgr->care) {
    MinisatClauses(S, lemmaMgr->care, NULL, NULL, 0);
  }
  /* add clauses for already proved lemmas */
  if (idepth) {
    provedLemmaClauses(lemmaMgr, lemmaMgr->provedClasses, NULL, lemmaMgr->provedImp, idepth, S);
  }
  /* add clauses for the loopFree expression if necessary */
  if (0 && idepth>1 && !init) {
    lemmaLoopFreeClauses(lemmaMgr->timeFrameStates, idepth, 1, S);
  }
  /* add clauses for assumptions (fixed) */
  lemmaAssumptionClauses(lemmaMgr, lemmaClasses, lemmaEqLits, idepth, S);

  endTime = util_cpu_time();
  *optTime += (endTime-startTime)/1000.0;

  /* perform SAT loop */
  cexUsed = fake = numFake = numFalse = numFailInSequence = nrun = timeOut = 0;
  //  S.minisat20_opt = true;
  S.minisat20_opt = false;

  do {
     assert(nrun++ < nNodes);
     /* add a clause forcing at least one false lemma */
     enableFalseLemmaClause(lemmaMgr, lemmaClasses, lemmaEqLits, NULL, lemmaImpLits, idepth, nNodes, S);
     if (!S.okay()) {
       if (verbose) {printf("*"); fflush(NULL);}
       //assert(nrun>1 || lemmaMgr->nConst+lemmaMgr->nEquiv==0);
       break;
     }
     startTime = util_cpu_time();
     localTimeLimit = timeLimit = (lemmaMgr->timeLimit - startTime) / 1000.0;

#if 0
     if (S.minisat20_opt == false) S.minisat20_opt = true;
     else   S.minisat20_opt = false;
#endif
     // S.pdt_opt_neg_first = true;

     sat = S.solve(assumps); //, localTimeLimit);
     endTime = util_cpu_time();
     currTime += (endTime-startTime)/1000.0;
     if (endTime>lemmaMgr->timeLimit) {
       if (verbose) {printf("A"); fflush(NULL);}
       timeOut = 1;
       nrun = 0;
     } else if (S.undefined()) {
       if (verbose) {printf("?"); fflush(NULL);}
       if (!cexUsed) {
	 timeOut = 1;
	 nrun = 0;
       } else {
	 cexUsed = 0;
       }
     } else if (sat) {
       if (1 || idepth && !init) localTimeLimit = 1.0; //S.dump("bug.cnf", 0);
       //if (idepth) fprintf(dMgrO(ddm),"%4d: %d+%d --> ", nrun, lemmaMgr->nConst, lemmaMgr->nEquiv);
       /* remove unproven/disproved lemmas */
       //cexUsed = 1;
       startTime = util_cpu_time();
       falsified = disprovedLemmaClauses(lemmaMgr, lemmaClasses, lemmaImp, lemmaEqLits,
					 lemmaImpLits, idepth, S, cex, falseLemmas);
       endTime = util_cpu_time();
       *disTime += (endTime-startTime)/1000.0;
       numFalse += falsified;
       if (falsified) {
	 if (verbose) {printf("."); fflush(NULL);}
	 fake = 0;
         numFailInSequence++;
         if (numFailInSequence == 4 && numFake>0) {
           numFake--;
         }
       } else {
	 /* no false lemmas in the original model, according to this cex */
	 //assert(nrun > 1);
	 if (verbose) {printf("!"); fflush(NULL);}
         numFailInSequence=0;
	 if (++numFake>=maxFake || ++fake>=maxFake/2) {
	   if (numFalse==0) {
             lemmaMgr->timeLimit = 0;
           }
           break;
	 }
       }
       //if (idepth) fprintf(dMgrO(ddm),"%d+%d\n", lemmaMgr->nConst, lemmaMgr->nEquiv);
     } else {
       if (verbose) {printf("*"); fflush(NULL);}
     }
  } while ((sat || cexUsed) && !timeOut && (nrun<1000000 || !idepth || init));

  //if (numFalse==0) {
  //  lemmaMgr->timeLimit = 0;
  //  nrun=0;
  //}

  //if (verbose) {printf("\n"); fflush(NULL);}
  *satTime += currTime;
  DdiAig2CnfIdClose(ddm);
  elapsedTime = util_cpu_time() - elapsedTime;
  if (0 && idepth && !init) {
    fprintf(dMgrO(ddm),"    --> %d runs, sat=%s, total=%s\n", nrun, 
            util_print_time(currTime), util_print_time(elapsedTime));
    fflush(NULL);
  }
  for (i=0; i<=idepth; i++) {
    Ddi_Free(lemmaMgr->timeFrameNodes[i]);
  }
  //assert(Ddi_MgrCheckExtRef(ddm, numRef));
  return nrun;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
computeLemmaNum (
  Ddi_Bddarray_t *lemmaClasses,
  short int ***lemmaImp,
  int *nImpPtr,
  int nNodes
)
{
  Ddi_Bdd_t *lemmaClass;
  int i, sign, nEquiv, nImp;

  nEquiv = nImp = 0;
  if (lemmaClasses) {
    for (i=1; i<Ddi_BddarrayNum(lemmaClasses); i++) {
      lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
      nEquiv += Ddi_BddPartNum(lemmaClass) - 1;
    }
  }
  if (lemmaImp) {
    for (sign=0; sign<2; sign++) {
      for (i=1; i<nNodes; i++) {
	nImp += lemmaImp[sign][0][i];
      }
    }
    *nImpPtr = nImp;
  }
  return nEquiv;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
updateProvedLemmas (
  lemmaMgr_t *lemmaMgr,
  Ddi_Bddarray_t *lemmaClasses,
  short int ***lemmaImp,
  int *result
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaClasses);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  int *provedNodes = lemmaMgr->provedNodes;
  Ddi_Bddarray_t *lemmaNodes = lemmaMgr->lemmaNodes;
  Ddi_Bddarray_t *provedClasses = lemmaMgr->provedClasses;
  int i, j, pos, nNodes = Ddi_BddarrayNum(lemmaNodes);
  Ddi_Bdd_t *lemmaClass, *newClass, *node;
  bAigEdge_t baig;

  /* constants */
  lemmaClass = Ddi_BddarrayRead(lemmaClasses, 0);
  newClass = Ddi_BddarrayRead(provedClasses, 0);

  //  printf("provedConst: ");
  for (j=0; j<Ddi_BddPartNum(lemmaClass); j++) {
    node = Ddi_BddPartRead(lemmaClass, j);

    //  printf("%d ", Ddi_BddToBaig(node));

    provedNodes[aigIndexRead(aigMgr, node)] = 0;
    Ddi_BddPartInsertLast(newClass, node);
  }
  // printf("\n");
  /* equivalences */
  for (i=1; i<Ddi_BddarrayNum(lemmaClasses); i++) {
    lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
    if (Ddi_BddPartNum(lemmaClass) > 1) {
      //    printf("EQ CL: %d", Ddi_BddToBaig(Ddi_BddPartRead(lemmaClass, 0)));
      for (j=1; j<Ddi_BddPartNum(lemmaClass); j++) {
	node = Ddi_BddPartRead(lemmaClass, j);
	provedNodes[aigIndexRead(aigMgr, node)] = Ddi_BddarrayNum(provedClasses);
	//printf(" %d", Ddi_BddToBaig(node));
      }
      //      printf("\n");

      Ddi_BddarrayInsertLast(provedClasses, lemmaClass);
    }
  }

  /* remove represented nodes from the original lemma list */
  for (i=Ddi_BddarrayNum(lemmaMgr->lemmaClasses)-1; i>=0; i--) {
    lemmaClass = Ddi_BddarrayRead(lemmaMgr->lemmaClasses, i);
    for (j=Ddi_BddPartNum(lemmaClass)-1; j>=0; j--) {
      node = Ddi_BddPartRead(lemmaClass, j);
      if (provedNodes[aigIndexRead(aigMgr, node)] >= 0) {
	assert(i==0 || j>0);
	Ddi_BddPartQuickRemove(lemmaClass, j);
      }
    }
    assert(i==0 || Ddi_BddPartNum(lemmaClass)>0);
    if (i>0 && Ddi_BddPartNum(lemmaClass)==1) {
      Ddi_BddarrayQuickRemove(lemmaMgr->lemmaClasses, i);
    }
  }

  /* check property */
  if (lemmaMgr->spec != NULL) {
    node = lemmaMgr->spec;
    pos = provedNodes[aigIndexRead(aigMgr, node)];
    while (pos > 0) {
      lemmaClass = Ddi_BddarrayRead(provedClasses, pos);
      node = Ddi_BddPartRead(lemmaClass, 0);
      pos = provedNodes[aigIndexRead(aigMgr, node)];
    }
    *result = (pos==0);
    if (Ddi_BddToBaig(lemmaMgr->spec) != Ddi_BddToBaig(node)) {
      Ddi_Free(lemmaMgr->spec);
      lemmaMgr->spec = Ddi_BddDup(node);
    }
  }

  return lemmaMgr->spec; 
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static short int ***
lemmaImpCopy(
  short int ***lemmaImp,
  int nNodes
)
{
  int i, j, k, num;
  short int ***lemmaDup;

  if (!lemmaImp) 
     return NULL;

  lemmaDup = Pdtutil_Alloc(short int **, 2);
  for (k=0; k<2; k++) {
     lemmaDup[k] = Pdtutil_Alloc(short int *, nNodes);
     for (i=0; i<nNodes; i++) {
	lemmaDup[k][i] = Pdtutil_Alloc(short int, nNodes);
	memcpy(lemmaDup[k][i], lemmaImp[k][i], nNodes*sizeof(short int));
     }
  }

  return lemmaDup;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
lemmaImpBasicImp (
  lemmaMgr_t *lemmaMgr,
  Ddi_Bddarray_t *lemmaArray,
  int nNodes
)
{
  Ddi_Bddarray_t *lemmaNodes = lemmaMgr->lemmaNodes;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaNodes);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  int i, j, k, id0, id1, sign, sign0, sign1, num0, num1, nImp=0, nTotImp=0, again;
  short int ***provedImp = lemmaMgr->provedImp;
  short int ***lemmaImp = lemmaMgr->lemmaImp;
  short int **redImp = lemmaMgr->redImp;
  char **combImp = lemmaMgr->combImp;
  Ddi_Bdd_t *node, *lemma;
  
  fprintf(dMgrO(ddm),"Removing redundant implications: ");
  fflush(NULL);

  /* move implications from comb matrix to lemmaImp structure */
  for (i=1; i<nNodes; i++) {
     memset(redImp[i], -1, nNodes*sizeof(short int));
     lemmaImp[0][0][i] = lemmaImp[1][0][i] = 0;
     lemmaImp[0][i][nNodes-i] = lemmaImp[1][i][nNodes-i] = nNodes;
  }

  for (i=1; i<nNodes; i++) {
     for (j=i+1; j<nNodes; j++) {
	if (combImp[i][j]) {
	   sign0 = (combImp[i][j] & 0x4) >> 2;
	   sign1 = (combImp[i][j] & 0x2) >> 1;
	   lemmaImp[sign0][i][lemmaImp[sign0][0][i]++] = sign1 ? -j : j;
	   lemmaImp[sign1][j][--lemmaImp[sign1][j][nNodes-j]] = sign0 ? -i : i;
	   redImp[i][j] = redImp[j][i] = 0;
	}
     }
  }

  do {
     again = 0;
     /* compute redundant implications */
     for (i=1; i<nNodes; i++) {
	/* inter-classes implications */
	for (sign0=0; sign0<=1; sign0++) {
	   num0 = lemmaImp[sign0][0][i];
	   assert(num0 < nNodes-i);
	   for (j=0; j<num0; j++) {
	      sign1 = lemmaImp[sign0][i][j] < 0;
	      id0 = abs(lemmaImp[sign0][i][j]);
	      num1 = lemmaImp[!sign1][0][id0];
	      for (k=0; k<num1; k++) {
		 id1 = abs(lemmaImp[!sign1][id0][k]);
		 sign = lemmaImp[!sign1][id0][k] < 0;
		 redImp[i][id1] = redImp[id1][i] = 1;
		 if (1&&!combImp[i][id1]) {
		    again = 1; combImp[i][id1] = combImp[id1][i] = 1;
		    lemmaImp[sign0][i][lemmaImp[sign0][0][i]++] = lemmaImp[!sign1][id0][k];
		    lemmaImp[sign][id1][--lemmaImp[sign][id1][nNodes-id1]] = sign0 ? -i : i;
		 }
	      }
	   }
	}
	/* intra-class implications (outgoing edges) */
	num0 = lemmaImp[0][0][i];
	num1 = lemmaImp[1][0][i];
	for (j=0; j<num0; j++) {
	   id0 = abs(lemmaImp[0][i][j]);
	   sign0 = lemmaImp[0][i][j] < 0;
	   for (k=0; k<num1; k++) {
	      id1 = abs(lemmaImp[1][i][k]);
	      sign1 = lemmaImp[1][i][k] < 0;
	      redImp[id0][id1] = redImp[id1][id0] = 1;
	      if (1&&!combImp[id0][id1]) {
		 again = 1; combImp[id0][id1] = combImp[id1][id0] = 1;
		 if (id0 < id1) {
		    lemmaImp[sign0][id0][lemmaImp[sign0][0][id0]++] = sign1 ? -id1 : id1;
		    lemmaImp[sign1][id1][--lemmaImp[sign1][id1][nNodes-id1]] = sign0 ? -id0 : id0;
		 } else {
		    lemmaImp[sign1][id1][lemmaImp[sign1][0][id1]++] = sign0 ? -id0 : id0;
		    lemmaImp[sign0][id0][--lemmaImp[sign0][id0][nNodes-id0]] = sign1 ? -id1 : id1;
		 }
	      }
	   }
	}
	/* intra-class implications (incoming edges) */
	num0 = lemmaImp[0][i][nNodes-i];
	num1 = lemmaImp[1][i][nNodes-i];
	for (j=num0; j<nNodes; j++) {
	   id0 = abs(lemmaImp[0][i][j]);
	   sign0 = lemmaImp[0][i][j] < 0;
	   for (k=num1; k<nNodes; k++) {
	      id1 = abs(lemmaImp[1][i][k]);
	      sign1 = lemmaImp[1][i][k] < 0;
	      redImp[id0][id1] = redImp[id1][id0] = 1;
	      if (1&&!combImp[id0][id1]) {
		 again = 1; combImp[id0][id1] = combImp[id1][id0] = 1;
		 if (id0 < id1) {
		    lemmaImp[sign0][id0][lemmaImp[sign0][0][id0]++] = sign1 ? -id1 : id1;
		    lemmaImp[sign1][id1][--lemmaImp[sign1][id1][nNodes-id1]] = sign0 ? -id0 : id0;
		 } else {
		    lemmaImp[sign1][id1][lemmaImp[sign1][0][id1]++] = sign0 ? -id0 : id0;
		    lemmaImp[sign0][id0][--lemmaImp[sign0][id0][nNodes-id0]] = sign1 ? -id1 : id1;
		 }
	      }
	   }
	}
     }
  } while (again);

  /* remove redundant implications */
  for (i=1; i<nNodes; i++) {
     node = Ddi_BddDup(Ddi_BddarrayRead(lemmaNodes, i));
     for (sign0=0; sign0<=1; sign0++) {
	if (sign0) Ddi_BddNotAcc(node);
	num0 = provedImp[sign0][0][i];
	for (j=0; j<num0; j++) {
	   sign1 = provedImp[sign0][i][j] < 0;
	   id1 = abs(provedImp[sign0][i][j]);
	   assert(redImp[i][id1] >= 0);
	   assert((combImp[i][id1] & 0x4) == (sign0<<2));
	   assert((combImp[i][id1] & 0x2) == (sign1<<1));
	   if (!redImp[i][id1]) {
	      lemma = Ddi_BddDup(Ddi_BddarrayRead(lemmaNodes, id1));
	      if (sign1) Ddi_BddNotAcc(lemma);
	      Ddi_BddOrAcc(lemma, node);
	      Ddi_BddarrayInsertLast(lemmaArray, lemma);
	      Ddi_Free(lemma);
	   }
	   nImp++;
	}
     }
     Ddi_Free(node);
  }
  computeLemmaNum(NULL, lemmaImp, &nTotImp, nNodes);
  fprintf(dMgrO(ddm),"%d -> %d (total=%d)\n", nImp, Ddi_BddarrayNum(lemmaArray), nTotImp);
  return nImp; /* number of total imp */
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
lemmaImpSimplify (
  Ddi_Bddarray_t *lemmaClasses,
  short int ***lemmaImp,
  char **combImp,
  int nNodes,
  int onlyUnits
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaClasses);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  int i, j, sign, num, pos0, pos1, nImp, nTotImp;
  Ddi_Bdd_t *lemmaClass, *node;

  nImp = nTotImp = 0;
  for (i=1; i<Ddi_BddarrayNum(lemmaClasses); i++) {
     lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
     nTotImp += lemmaImp[0][0][i] + lemmaImp[1][0][i];
     if (onlyUnits && Ddi_BddPartNum(lemmaClass)>1)
	continue;

     node = Ddi_BddPartRead(lemmaClass, 0);
     pos0 = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(node));
     for (sign=0; sign<2; sign++) {
	num = lemmaImp[sign][0][i];
	for (j=num-1; j>=0; j--) {
	   pos1 = lemmaImp[sign][i][j];
	   lemmaClass = Ddi_BddarrayRead(lemmaClasses, abs(pos1));
	   if (onlyUnits && Ddi_BddPartNum(lemmaClass)>1)
	      continue;

	   node = Ddi_BddPartRead(lemmaClass, 0);
	   pos1 = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(node));
	   if (combImp[pos0][pos1]) {
	      /* remove implication */
	      lemmaImp[sign][i][j] = lemmaImp[sign][i][--num];
	      nImp++;
	   }
	}
	lemmaImp[sign][0][i] = num;
     }
  }
  fprintf(dMgrO(ddm),"Simplification: %d/%d imp. removed\n", nImp, nTotImp);
  if (0&&!onlyUnits) {
     for (i=1; i<Ddi_BddarrayNum(lemmaClasses); i++) {
	lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
	node = Ddi_BddPartRead(lemmaClass, 0);
	pos0 = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(node));
	for (sign=0; sign<2; sign++) {
	   num = lemmaImp[sign][0][i];
	   for (j=num-1; j>0; j--) {
	      pos1 = lemmaImp[sign][i][j];
	      lemmaClass = Ddi_BddarrayRead(lemmaClasses, abs(pos1));
	      node = Ddi_BddPartRead(lemmaClass, 0);
	      pos1 = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(node));
	      assert(!combImp[pos0][pos1]);
	   }
	}
     }
  }
  return nImp; /* number of REMOVED imp */
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
lemmaStructImp(
  Ddi_Bddarray_t *lemmaClasses,
  short int ***lemmaImp,
  lemmaMgr_t *lemmaMgr,
  Solver& S,
  int *nTotImp,
  float *satTime
)
{
  int i, j, k, l, sign, num, cnt, sat, nImp=0, do_close=1, nrun=0;
  int pPos, pLit, qLit, qPos, pSign, qSign, sign0, sign1, rPos, tPos;
  int pos, lit0, lit1, lit0v, lit1v, var0, var1, val, qClass;
  short int ***provedImp=lemmaMgr->provedImp;
  short int **redImp=lemmaMgr->redImp;
  char **combImp=lemmaMgr->combImp;
  long startTime, timeLimit=lemmaMgr->timeLimit;
  Ddi_Bddarray_t *lemmaNodes = lemmaMgr->lemmaNodes;
  int again, nNodes = Ddi_BddarrayNum(lemmaNodes);
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaClasses);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  Ddi_Bdd_t *p, *q, *node, *lemmaClass; 
  bAigEdge_t baig, child;
  vec<Lit> assumps;

  *satTime = 0;
#if 0
  for (i=1; i<nNodes; i++) {
     baig = Ddi_BddToBaig(Ddi_BddarrayRead(lemmaNodes, i));
     assert(!bAig_NodeIsInverted(baig));
     if (!bAig_isVarNode(aigMgr, baig)) {
	child = bAig_NodeReadIndexOfLeftChild(aigMgr, baig);
	pos = bAig_AuxAig0(aigMgr, child);
	sign = bAig_NodeIsInverted(child);
	combImp[i][pos] = 5 | (sign << 1);
	combImp[pos][i] = 3 | (sign << 2);

	child = bAig_NodeReadIndexOfRightChild(aigMgr, baig);
	pos = bAig_AuxAig0(aigMgr, child);
	sign = bAig_NodeIsInverted(child);
	combImp[i][pos] = 5 | (sign << 1);
	combImp[pos][i] = 3 | (sign << 2);
     }
  }
  /* check only non-redundant implications */
  lemmaImpReduce(lemmaMgr, lemmaClasses, lemmaImp, nNodes);
  do {
     again = 0;
     if (util_cpu_time() > timeLimit) {
	return 0;
     }
     for (i=1; i<Ddi_BddarrayNum(lemmaClasses); i++) {
	lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
	p = Ddi_BddPartRead(lemmaClass, 0);
	pPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(p));
	pLit = aigCnfLit(S, p); 
	pSign = aigIsInv(p);
	for (sign=0; sign<=1; sign++) {
	   assumps.clear();
	   assumps.push(MinisatLit(sign ? pLit : -pLit));
	   pSign ^= sign;
	   cnt = lemmaImp[sign][0][i];
	   for (j=0; j<cnt; j++) {
	      qClass = lemmaImp[sign][i][j];
	      qSign = qClass < 0;
	      lemmaClass = Ddi_BddarrayRead(lemmaClasses, abs(qClass));
	      q = Ddi_BddPartRead(lemmaClass, 0);
	      qPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(q));
	      if (combImp[pPos][qPos] || redImp[i][abs(qClass)])
		 continue;
	      qLit = qSign ? -aigCnfLit(S, q) : aigCnfLit(S, q);
	      qSign ^= aigIsInv(q);
	      assumps.push(MinisatLit(-qLit));
	      assert(S.okay());
	      sat = S.solve(assumps);
	      nrun++;
	      if (!sat) {
		 combImp[pPos][qPos] = 1 | (pSign << 2) | (qSign << 1);
		 combImp[qPos][pPos] = 1 | (qSign << 2) | (pSign << 1);
	      } else {
		 /* use cex to prune the canditate set */
		 for (k=1; k<Ddi_BddarrayNum(lemmaClasses); k++) {
		    lemmaClass = Ddi_BddarrayRead(lemmaClasses, k);
		    node = Ddi_BddPartRead(lemmaClass, 0);
		    rPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(node));
		    lit0 = aigCnfLit(S, node); //cnf literal
		    sign0 = lit0 < 0;          //literal sign
		    var0 = abs(lit0) - 1;      //minisat variable index
		    Pdtutil_Assert(S.model[var0] != l_Undef, "undefined var in cex");
		    lit0v = (S.model[var0] == l_True) ^ sign0;//literal value
		    for (l=lemmaImp[lit0v][0][k]-1; l>=0; l--) {
		       pos = lemmaImp[lit0v][k][l];
		       assert(redImp[k][abs(pos)] >= 0);
		       sign1 = pos < 0;
		       lemmaClass = Ddi_BddarrayRead(lemmaClasses, abs(pos));
		       node = Ddi_BddPartRead(lemmaClass, 0);
		       tPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(node));
		       lit1 = aigCnfLit(S, node); //cnf literal
		       sign1 ^= aigIsInv(node);
		       var1 = abs(lit1) - 1;      //minisat variable index
		       Pdtutil_Assert(S.model[var1] != l_Undef, "undefined var in cex");
		       lit1v = (S.model[var1] == l_True) ^ sign1;//literal value
		       if (!lit1v) {
			  assert(!combImp[rPos][tPos]);
			  lemmaImp[lit0v][k][l] = lemmaImp[lit0v][k][--lemmaImp[lit0v][0][k]];
			  redImp[k][abs(pos)] = redImp[abs(pos)][k] = -1;
			  lemmaImpRemoveEdge(lemmaImp, redImp, k, lit0v, abs(pos), pos<0, 
					     Ddi_BddarrayNum(lemmaClasses), nNodes);
		       }
		    }
		 }
		 /* force loops to stop and restart */
		 j = cnt;
		 sign = 2;
		 i = Ddi_BddarrayNum(lemmaClasses);
		 again = 1;
	      }
	      assumps.pop();
	   }
	}
     }
  } while (again);
  for (i=1; i<Ddi_BddarrayNum(lemmaClasses); i++) {
     lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
     p = Ddi_BddPartRead(lemmaClass, 0);
     pPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(p));
     pSign = aigIsInv(p);
     for (sign=0; sign<=1; sign++) {
	pSign ^= sign; 
	for (j=0; j<lemmaImp[sign][0][i]; j++) {
	   qClass = lemmaImp[sign][i][j];
	   lemmaClass = Ddi_BddarrayRead(lemmaClasses, abs(qClass));
	   q = Ddi_BddPartRead(lemmaClass, 0);
	   qSign = aigIsInv(q) ^ (qClass < 0);
	   qPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(q));
	   combImp[pPos][qPos] = 1 | (pSign << 2) | (qSign << 1);
	   combImp[qPos][pPos] = 1 | (qSign << 2) | (pSign << 1);
	}
     }
  }
#else
#if 1
  for (i=1; i<nNodes; i++) {
     provedImp[0][0][i] = provedImp[1][0][i] = 0;
     provedImp[0][i][nNodes-i] = provedImp[1][i][nNodes-i] = nNodes;
  }
  for (i=1; i<nNodes; i++) {
     baig = Ddi_BddToBaig(Ddi_BddarrayRead(lemmaNodes, i));
     assert(!bAig_NodeIsInverted(baig));
     assert(bAig_AuxAig0(aigMgr, baig) == i);
     if (!bAig_isVarNode(aigMgr, baig)) {
	child = bAig_NodeReadIndexOfLeftChild(aigMgr, baig);
	pos = bAig_AuxAig0(aigMgr, child);
	sign = bAig_NodeIsInverted(child);
	combImp[i][pos] = 5 | (sign << 1);
	combImp[pos][i] = 3 | (sign << 2);
	assert(pos < i);
	num = provedImp[sign][0][pos]++;
	provedImp[sign][pos][num] = -i;
	num = --provedImp[1][i][nNodes-i];
	provedImp[1][i][num] = sign ? -pos : pos;

	child = bAig_NodeReadIndexOfRightChild(aigMgr, baig);
	pos = bAig_AuxAig0(aigMgr, child);
	sign = bAig_NodeIsInverted(child);
	combImp[i][pos] = 5 | (sign << 1);
	combImp[pos][i] = 3 | (sign << 2);
	assert(pos < i);
	num = provedImp[sign][0][pos]++;
	provedImp[sign][pos][num] = -i;
	num = --provedImp[1][i][nNodes-i];
	provedImp[1][i][num] = sign ? -pos : pos;
     }
  }

  /* check only non-redundant implications */
  do {
     if (do_close) {
	/* remove already proved implications */
	do_close = nImp = 0;
	lemmaImpClosure(provedImp, lemmaMgr, combImp, NULL);
#if 1
	for (i=1; i<Ddi_BddarrayNum(lemmaClasses); i++) {
	   lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
	   p = Ddi_BddPartRead(lemmaClass, 0);
	   pPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(p));
	   for (sign=0; sign<=1; sign++) {
	      for (j=lemmaImp[sign][0][i]-1; j>=0; j--) {
		 qClass = lemmaImp[sign][i][j];
		 lemmaClass = Ddi_BddarrayRead(lemmaClasses, abs(qClass));
		 q = Ddi_BddPartRead(lemmaClass, 0);
		 qPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(q));
		 if (combImp[pPos][qPos]) {
		    lemmaImp[sign][i][j] = lemmaImp[sign][i][--lemmaImp[sign][0][i]];
		 }
	      }
	   }
	}
#endif
     }
     lemmaImpReduce(lemmaMgr, lemmaClasses, lemmaImp, nNodes);
     again = 0;
     for (i=1; i<Ddi_BddarrayNum(lemmaClasses); i++) {
	lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
	p = Ddi_BddPartRead(lemmaClass, 0);
	pPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(p));
	pLit = aigCnfLit(S, p); 
	pSign = aigIsInv(p);
	for (sign=0; sign<=1; sign++) {
	   assumps.clear();
	   assumps.push(MinisatLit(sign ? pLit : -pLit));
	   pSign ^= sign;
	   for (j=0; j<lemmaImp[sign][0][i]; j++) {
	      if (util_cpu_time() > timeLimit) {
		 //nImp = lemmaImpClosure(provedImp, lemmaMgr, combImp, NULL);
		 for (k=1; k<nNodes; k++) {
		    provedImp[0][0][k] = provedImp[1][0][k] = 0;
		    provedImp[0][k][nNodes-k] = provedImp[1][k][nNodes-k] = nNodes;
		 }
		 return nImp;
	      }
	      qClass = lemmaImp[sign][i][j];
	      qSign = qClass < 0;
	      lemmaClass = Ddi_BddarrayRead(lemmaClasses, abs(qClass));
	      q = Ddi_BddPartRead(lemmaClass, 0);
	      qPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(q));
	      if (combImp[pPos][qPos] || redImp[i][abs(qClass)]) {
		 continue;
	      }
	      qLit = qSign ? -aigCnfLit(S, q) : aigCnfLit(S, q);
	      qSign ^= aigIsInv(q);
	      assumps.push(MinisatLit(-qLit));
	      assert(S.okay());
	      startTime = util_cpu_time();
	      sat = S.solve(assumps);
	      *satTime += (util_cpu_time()-startTime)/1000.0;
	      nrun++;
	      if (!sat) {
		 do_close = ++nImp > 0;
		 if (pPos < qPos) {
		    num = provedImp[pSign][0][pPos]++;
		    provedImp[pSign][pPos][num] = qSign ? -qPos : qPos;
		    num = --provedImp[qSign][qPos][nNodes-qPos];
		    provedImp[qSign][qPos][num] = pSign ? -pPos : pPos;
		 } else {
		    num = provedImp[qSign][0][qPos]++;
		    provedImp[qSign][qPos][num] = pSign ? -pPos : pPos;
		    num = --provedImp[pSign][pPos][nNodes-pPos];
		    provedImp[pSign][pPos][num] = qSign ? -qPos : qPos;
		 }
		 /* skip implication during next iterations */
		 combImp[pPos][qPos] = 1 | (pSign << 2) | (qSign << 1);
		 combImp[qPos][pPos] = 1 | (qSign << 2) | (pSign << 1);
	      } else {
		 /* use cex to prune the canditate set */
		 for (k=1; k<Ddi_BddarrayNum(lemmaClasses); k++) {
		    lemmaClass = Ddi_BddarrayRead(lemmaClasses, k);
		    node = Ddi_BddPartRead(lemmaClass, 0);
		    rPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(node));
		    lit0 = aigCnfLit(S, node); //cnf literal
		    sign0 = lit0 < 0;          //literal sign
		    var0 = abs(lit0) - 1;      //minisat variable index
		    Pdtutil_Assert(S.model[var0] != l_Undef, "undefined var in cex");
		    lit0v = (S.model[var0] == l_True) ^ sign0;//literal value
		    for (l=lemmaImp[lit0v][0][k]-1; l>=0; l--) {
		       pos = lemmaImp[lit0v][k][l];
		       assert(redImp[k][abs(pos)] >= 0);
		       sign1 = pos < 0;
		       lemmaClass = Ddi_BddarrayRead(lemmaClasses, abs(pos));
		       node = Ddi_BddPartRead(lemmaClass, 0);
		       tPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(node));
		       lit1 = aigCnfLit(S, node); //cnf literal
		       sign1 ^= aigIsInv(node);
		       var1 = abs(lit1) - 1;      //minisat variable index
		       Pdtutil_Assert(S.model[var1] != l_Undef, "undefined var in cex");
		       lit1v = (S.model[var1] == l_True) ^ sign1;//literal value
		       if (!lit1v) {
			  assert(!combImp[rPos][tPos]);
			  lemmaImp[lit0v][k][l] = lemmaImp[lit0v][k][--lemmaImp[lit0v][0][k]];
			  redImp[k][abs(pos)] = redImp[abs(pos)][k] = -1;
		       }
		    }
		 }
		 j = 0;
		 again = 1;
	      }
	      assumps.pop();
	   }
	}
     }
  } while (again);
  nImp = lemmaImpClosure(provedImp, lemmaMgr, combImp, NULL);

  if (0) {
     int nTotImp=0, cImp=0;
     computeLemmaNum(lemmaClasses, lemmaImp, &nTotImp, nNodes);
     for (i=1; i<nNodes; i++) {
	for (j=1; j<nNodes; j++) {
	   cImp += (combImp[i][j] != 0);
	}
     }
     assert(cImp % 2 == 0);
     cImp /= 2;
     assert(cImp == nTotImp);
     assert(cImp == nImp);
     assert(nImp == nTotImp);
  }
  for (i=1; i<nNodes; i++) {
     provedImp[0][0][i] = provedImp[1][0][i] = 0;
     provedImp[0][i][nNodes-i] = provedImp[1][i][nNodes-i] = nNodes;
  }
#else
  for (i=1; i<Ddi_BddarrayNum(lemmaClasses); i++) {
     lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
     p = Ddi_BddPartRead(lemmaClass, 0);
     //assert(!aigIsInv(p));
     pPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(p));
     pLit = aigCnfLit(S, p);
     for (sign=0; sign<=1; sign++) {
	assumps.clear();
	assumps.push(MinisatLit(sign ? pLit : -pLit));
	num = lemmaImp[sign][0][i];
	for (j=0; j<num; j++) {
	   qClass = lemmaImp[sign][i][j];
	   qSign = qClass < 0;
	   lemmaClass = Ddi_BddarrayRead(lemmaClasses, abs(qClass));
	   q = Ddi_BddPartRead(lemmaClass, 0);
	   qPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(q));
	   qLit = qSign ? -aigCnfLit(S, q) : aigCnfLit(S, q);
	   assumps.push(MinisatLit(-qLit));
	   assert(S.okay());
	   sat = S.solve(assumps);
	   nrun++;
	   if (!sat) {
	      combImp[pPos][qPos] = combImp[qPos][pPos] = 1;
	   } else {
	      /* use cex to prune the canditate set */
	      for (k=i+1; k<Ddi_BddarrayNum(lemmaClasses); k++) {
		 lemmaClass = Ddi_BddarrayRead(lemmaClasses, k);
		 node = Ddi_BddPartRead(lemmaClass, 0);
		 lit0 = aigCnfLit(S, node); //cnf literal
		 sign0 = lit0 < 0;          //literal sign
		 var0 = abs(lit0) - 1;      //minisat variable index
		 Pdtutil_Assert(S.model[var0] != l_Undef, "undefined var in cex");
		 lit0v = (S.model[var0] == l_True) ^ sign0;//literal value
		 cnt = lemmaImp[lit0v][0][k];
		 for (l=cnt-1; l>=0; l--) {
		    pos = lemmaImp[lit0v][k][l];
		    sign1 = pos < 0;
		    lemmaClass = Ddi_BddarrayRead(lemmaClasses, abs(pos));
		    node = Ddi_BddPartRead(lemmaClass, 0);
		    lit1 = aigCnfLit(S, node); //cnf literal
		    sign1 ^= aigIsInv(node);
		    var1 = abs(lit1) - 1;      //minisat variable index
		    Pdtutil_Assert(S.model[var1] != l_Undef, "undefined var in cex");
		    lit1v = (S.model[var1] == l_True) ^ sign1;//literal value
		    if (!lit1v) {
		       lemmaImp[lit0v][k][l] = lemmaImp[lit0v][k][--lemmaImp[lit0v][0][k]];
		    }
		 }
	      }
	      for (k=num-1; k>j; k--) {
		 qPos = lemmaImp[sign][i][k];
		 qSign = qPos < 0;
		 lemmaClass = Ddi_BddarrayRead(lemmaClasses, abs(qPos));
		 q = Ddi_BddPartRead(lemmaClass, 0);
		 qLit = aigCnfLit(S, q); //cnf literal
		 qSign ^= aigIsInv(q);
		 var1 = abs(qLit) - 1;   //minisat variable index
		 Pdtutil_Assert(S.model[var1] != l_Undef, "undefined var in cex");
		 lit1v = (S.model[var1] == l_True) ^ qSign;//literal value
		 if (!lit1v) {
		    lemmaImp[sign][i][k] = lemmaImp[sign][i][--lemmaImp[sign][0][i]];
		 }
	      }
	      lemmaImp[sign][i][j--] = lemmaImp[sign][i][--lemmaImp[sign][0][i]];
	      num = lemmaImp[sign][0][i];
	   }
	   assumps.pop();
	}
     }
  }
#endif
#endif

  *nTotImp = nImp;
  return nrun;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [MODIFIES THE COMBIMP MATRIX.]
  SeeAlso     []
******************************************************************************/
static int
lemmaImpClosure(
  short int ***lemmaImp,
  lemmaMgr_t *lemmaMgr,
  char **combImp,
  short int **redImp
)
{
  Ddi_Bddarray_t *lemmaNodes = lemmaMgr->lemmaNodes;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaNodes);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  int id, id0, id1, sign, sign0, sign1, num, num0, num1;
  int i, j, k, l, again, nImp=0, cImp=0;
  int nNodes = Ddi_BddarrayNum(lemmaNodes);

  assert(combImp);
  /* compute the implication transitive closure */
  do {
     again = 0;
     for (i=1; i<nNodes; i++) {
	/* inter-classes implications */
	for (sign0=0; sign0<=1; sign0++) {
	   id0 = i;
	   num0 = lemmaImp[sign0][0][i];
	   assert(num0 < nNodes-i);
	   for (j=0; j<num0; j++) {
	      id = abs(lemmaImp[sign0][i][j]);
	      sign = lemmaImp[sign0][i][j] < 0;
	      num = lemmaImp[!sign][0][id];
	      for (k=0; k<num; k++) {
		 id1 = abs(lemmaImp[!sign][id][k]);
		 sign1 = lemmaImp[!sign][id][k] < 0;
		 if (!combImp[id0][id1]) {
		    again = 1; 
		    combImp[id0][id1] = 1 | (sign0 << 2) | (sign1 << 1);
		    combImp[id1][id0] = 1 | (sign1 << 2) | (sign0 << 1);
		    lemmaImp[sign0][id0][lemmaImp[sign0][0][id0]++] = sign1 ? -id1 : id1;
		    lemmaImp[sign1][id1][--lemmaImp[sign1][id1][nNodes-id1]] = sign0 ? -id0 : id0;
		 }
	      }
	   }
	}
	/* intra-class implications (outgoing edges) */
	num0 = lemmaImp[0][0][i];
	num1 = lemmaImp[1][0][i];
	for (j=0; j<num0; j++) {
	   id0 = abs(lemmaImp[0][i][j]);
	   sign0 = lemmaImp[0][i][j] < 0;
	   for (k=0; k<num1; k++) {
	      id1 = abs(lemmaImp[1][i][k]);
	      sign1 = lemmaImp[1][i][k] < 0;
	      if (!combImp[id0][id1]) {
		 again = 1; 
		 combImp[id0][id1] = 1 | (sign0 << 2) | (sign1 << 1);
		 combImp[id1][id0] = 1 | (sign1 << 2) | (sign0 << 1);
		 if (id0 < id1) {
		    lemmaImp[sign0][id0][lemmaImp[sign0][0][id0]++] = sign1 ? -id1 : id1;
		    lemmaImp[sign1][id1][--lemmaImp[sign1][id1][nNodes-id1]] = sign0 ? -id0 : id0;
		 } else {
		    lemmaImp[sign1][id1][lemmaImp[sign1][0][id1]++] = sign0 ? -id0 : id0;
		    lemmaImp[sign0][id0][--lemmaImp[sign0][id0][nNodes-id0]] = sign1 ? -id1 : id1;
		 }
	      }
	   }
	}
	/* intra-class implications (incoming edges) */
	num0 = lemmaImp[0][i][nNodes-i];
	num1 = lemmaImp[1][i][nNodes-i];
	for (j=num0; j<nNodes; j++) {
	   id0 = abs(lemmaImp[0][i][j]);
	   sign0 = lemmaImp[0][i][j] < 0;
	   for (k=num1; k<nNodes; k++) {
	      id1 = abs(lemmaImp[1][i][k]);
	      sign1 = lemmaImp[1][i][k] < 0;
	      if (!combImp[id0][id1]) {
		 again = 1; 
		 combImp[id0][id1] = 1 | (sign0 << 2) | (sign1 << 1);
		 combImp[id1][id0] = 1 | (sign1 << 2) | (sign0 << 1);
		 if (id0 < id1) {
		    lemmaImp[sign0][id0][lemmaImp[sign0][0][id0]++] = sign1 ? -id1 : id1;
		    lemmaImp[sign1][id1][--lemmaImp[sign1][id1][nNodes-id1]] = sign0 ? -id0 : id0;
		 } else {
		    lemmaImp[sign1][id1][lemmaImp[sign1][0][id1]++] = sign0 ? -id0 : id0;
		    lemmaImp[sign0][id0][--lemmaImp[sign0][id0][nNodes-id0]] = sign1 ? -id1 : id1;
		 }
	      }
	   }
	}
     }
  } while (again);

#if 0
  for (i=1; i<nNodes; i++) {
     for (sign0=0; sign0<=1; sign0++) {
	for (j=0; j<lemmaImp[sign0][0][i]; j++) {
	   id1 = abs(lemmaImp[sign0][i][j]);
	   assert(combImp[i][id1]);
	   cImp++;
	}
     }
  }
  for (i=1; i<nNodes; i++) {
     nImp += lemmaImp[0][0][i] + lemmaImp[1][0][i];
  }
  assert(cImp == nImp);
  for (i=1; i<nNodes; i++) {
     for (j=1; j<nNodes; j++) {
	if (combImp[i][j]) {
	   cImp--;
	}
     }
  }
  assert(cImp == -nImp);
#else
  for (i=1; i<nNodes; i++) {
     nImp += lemmaImp[0][0][i] + lemmaImp[1][0][i];
  }
#endif
  return nImp;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static char **
lemmaImpAlloc1d (
  int nNodes
)
{
  char **lemmaImp=NULL;
  int i, k;

  lemmaImp = Pdtutil_Alloc(char *, nNodes);
  Pdtutil_Assert(lemmaImp, "memory allocation failure\n");
  for (i=0; i<nNodes; i++) {
     lemmaImp[i] = (char *)calloc(nNodes, sizeof(char));
     Pdtutil_Assert(lemmaImp[i], "memory allocation failure\n");
  }

  return lemmaImp;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static short int **
lemmaImpAlloc2d (
  int nNodes
)
{
  short int **lemmaImp=NULL;
  int i, k;

  lemmaImp = Pdtutil_Alloc(short int *, nNodes);
  Pdtutil_Assert(lemmaImp, "memory allocation failure\n");
  for (i=0; i<nNodes; i++) {
     lemmaImp[i] = (short int *)calloc(nNodes, sizeof(short int));
     Pdtutil_Assert(lemmaImp[i], "memory allocation failure\n");
  }

  return lemmaImp;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static short int ***
lemmaImpAlloc3d (
  int nNodes
)
{
  short int ***lemmaImp=NULL;
  int i, k;

  lemmaImp = Pdtutil_Alloc(short int **, 2);
  Pdtutil_Assert(lemmaImp, "memory allocation failure\n");
  for (k=0; k<2; k++) {
     lemmaImp[k] = Pdtutil_Alloc(short int *, nNodes);
     Pdtutil_Assert(lemmaImp[k], "memory allocation failure\n");
     for (i=0; i<nNodes; i++) {
	lemmaImp[k][i] = (short int *)calloc(nNodes, sizeof(short int));
	Pdtutil_Assert(lemmaImp[k][i], "memory allocation failure\n");
     }
  }

  return lemmaImp;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int ***
lemmaImpAllocNd (
  int nNodes,
  int depth
)
{
  int ***lemmaImp=NULL;
  int i, k;

  if (depth) {
     lemmaImp = Pdtutil_Alloc(int **, depth);
     Pdtutil_Assert(lemmaImp, "memory allocation failure\n");
     for (k=0; k<depth; k++) {
        lemmaImp[k] = Pdtutil_Alloc(int *, nNodes);
        Pdtutil_Assert(lemmaImp[k], "memory allocation failure\n");
        for (i=0; i<nNodes; i++) {
           lemmaImp[k][i] = (int *)calloc(nNodes, sizeof(int));
           Pdtutil_Assert(lemmaImp[k][i], "memory allocation failure\n");
        }
     }
  }

  return lemmaImp;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
lemmaImpFree3d (
  short int ***lemmaImp,
  int nNodes
)
{
  int i, j;

  if (lemmaImp) {
     for (i=0; i<2; i++) {
	for (j=0; j<nNodes; j++) {
	   Pdtutil_Free(lemmaImp[i][j]);
	}
	Pdtutil_Free(lemmaImp[i]);
     }
     Pdtutil_Free(lemmaImp);
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
lemmaImpFree1d (
  char **lemmaImp,
  int nNodes
)
{
  int i;

  if (lemmaImp) {
     for (i=0; i<nNodes; i++) {
	Pdtutil_Free(lemmaImp[i]);
     }
     Pdtutil_Free(lemmaImp);
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
lemmaImpFree2d (
  short int **lemmaImp,
  int nNodes
)
{
  int i;

  if (lemmaImp) {
     for (i=0; i<nNodes; i++) {
	Pdtutil_Free(lemmaImp[i]);
     }
     Pdtutil_Free(lemmaImp);
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
lemmaImpFreeNd (
  int ***lemmaImp,
  int nNodes,
  int depth
)
{
  int i, j;

  if (lemmaImp) {
     for (i=0; i<depth; i++) {
	for (j=0; j<nNodes; j++) {
	   Pdtutil_Free(lemmaImp[i][j]);
	}
	Pdtutil_Free(lemmaImp[i]);
     }
     Pdtutil_Free(lemmaImp);
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
lemmaLoopFreeClauses(
  Ddi_Vararray_t **timeFrameStateVars,
  int idepth,
  int full,
  Solver& S
)
{
  Ddi_Mgr_t *ddm=Ddi_ReadMgr(timeFrameStateVars);
  Ddi_Bddarray_t **timeFrameStates;
  Ddi_Bddarray_t *psLits, *nsLits;
  int i, j, k, nStates, lit, litp, litn;
  Ddi_Bdd_t *stateBit;
  vec<Lit> lits;

  if (idepth <= 1)
     return 0;

  timeFrameStates = Pdtutil_Alloc(Ddi_Bddarray_t *, idepth);
  for (i=0; i<idepth; i++) {
   timeFrameStates[i] = Ddi_BddarrayMakeLiteralsAig(timeFrameStateVars[i+1],1);
  }
  nStates = Ddi_BddarrayNum(timeFrameStates[0]);

  if (!full) {
     j = idepth-1;
     for (i=0; i<idepth-1; i++) {
	psLits = timeFrameStates[i];
	nsLits = timeFrameStates[j];
	lits.clear();
	for (k=0; k<nStates; k++) {
	   stateBit = Ddi_BddarrayRead(psLits, k);
	   litp = aigCnfLit(S, stateBit);
	   stateBit = Ddi_BddarrayRead(nsLits, k);
	   litn = aigCnfLit(S, stateBit);
	   lit = enableLemmaClauses(ddm, S, litp, litn, 1, 1);
	   lits.push(MinisatLit(lit));
	}
	S.addClause(lits);
     }
  } else {
     for (i=0; i<idepth-1; i++) {
	psLits = timeFrameStates[i];
	for (j=i+1; j<idepth; j++) {
	   nsLits = timeFrameStates[j];
	   lits.clear();
	   for (k=0; k<nStates; k++) {
	      stateBit = Ddi_BddarrayRead(psLits, k);
	      litp = aigCnfLit(S, stateBit);
	      stateBit = Ddi_BddarrayRead(nsLits, k);
	      litn = aigCnfLit(S, stateBit);
	      lit = enableLemmaClauses(ddm, S, litp, litn, 1, 1);
	      lits.push(MinisatLit(lit));
	   }
	   S.addClause(lits);
	}
     }
  }
  for (i=0; i<idepth; i++) {
     Ddi_Free(timeFrameStates[i]);
  }
  Pdtutil_Free(timeFrameStates);
  return 0;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
enableFalseLemmaClause (
  lemmaMgr_t *lemmaMgr,
  Ddi_Bddarray_t *lemmaClasses,
  int *lemmaEqLits,
  short int ***lemmaImp,
  int ***lemmaImpLits,
  int idepth,
  int nNodes,
  Solver& S
)
{
  int i, j, k, pos, pPos, qPos, sign, num;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaClasses);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  Ddi_Bdd_t *lemmaClass, *lemmaNode;
  short int pClassId, qClassId;
  short int **redImp = lemmaMgr->redImp;
  char **combImp = lemmaMgr->combImp;
  vec<Lit> lits;

  lits.clear(); 

  /* constants and equivalences */
  if (lemmaMgr->nConst + lemmaMgr->nEquiv) {
    for (i=0; i<Ddi_BddarrayNum(lemmaClasses); i++) {
      lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
      for (j=(i==0 ? 0 : 1); j<Ddi_BddPartNum(lemmaClass); j++) {
	lemmaNode = Ddi_BddPartRead(lemmaClass, j);
	pos = aigIndexRead(aigMgr, lemmaNode);
	//assert(lemmaEqLits[pos] != 0);
	if (lemmaEqLits[pos] != 0) {
	  lits.push(MinisatLit(lemmaEqLits[pos]));
	}
      }
    }
  }

  S.addClause(lits);
  return lits.size();
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
checkFalseLemmaClause (
  lemmaMgr_t *lemmaMgr,
  Ddi_Bddarray_t *lemmaClasses,
  int *lemmaEqLits,
  short int ***lemmaImp,
  int ***lemmaImpLits,
  int idepth,
  int nNodes,
  Solver& S
)
{
  int i, j, k, pos, pPos, qPos, sign, num;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaClasses);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  Ddi_Bdd_t *lemmaClass, *lemmaNode;
  short int pClassId, qClassId;
  short int **redImp = lemmaMgr->redImp;
  char **combImp = lemmaMgr->combImp;
  int ret = 0;

  /* constants and equivalences */
  if (lemmaMgr->nConst + lemmaMgr->nEquiv) {
    for (i=0; i<Ddi_BddarrayNum(lemmaClasses); i++) {
      lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
      for (j=(i==0 ? 0 : 1); j<Ddi_BddPartNum(lemmaClass); j++) {
	lemmaNode = Ddi_BddPartRead(lemmaClass, j);
	pos = aigIndexRead(aigMgr, lemmaNode);
	//assert(lemmaEqLits[pos] != 0);
	if (lemmaEqLits[pos] != 0) {
	  int var = abs(lemmaEqLits[pos])-1;
          int isNeg = lemmaEqLits[pos]<0;
          int active = S.model[var] == (isNeg ? false : true);
          if (active) {
            ret = 1;
            printf ("Active lemma: %d - %d\n", i, j);
          }
	}
      }
    }
  }

  return ret;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
lemmaAssumptionClauses (
  lemmaMgr_t *lemmaMgr,
  Ddi_Bddarray_t *lemmaClasses,
  int *lemmaEqLits,
  int idepth,
  Solver& S
)
{
  int i, j, k, lit, lit0, lit1, pos, pos0, pos1, sign, sign0, sign1;
  Ddi_Bddarray_t **timeFrameNodes=lemmaMgr->timeFrameNodes;
  Ddi_Bdd_t *lemmaClass, *node, *node0, *node1;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaClasses);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  vec<Lit> lits;

  /* constants */
  lemmaClass = Ddi_BddarrayRead(lemmaClasses, 0);
  for (k=0; k<=(lemmaMgr->doStrong ? 0 : idepth); k++) {
    // temporary until protection from multiple load implemented
    //Ddi_Bdd_t *auxAigPart = Ddi_BddMakePartConjFromArray(timeFrameNodes[k]);
    //MinisatClauses(S, auxAigPart, NULL, NULL, 1);
    //Ddi_Free(auxAigPart);
    //
    for (j=0; j<Ddi_BddPartNum(lemmaClass); j++) {
      node = Ddi_BddPartRead(lemmaClass, j);
      pos = aigIndexRead(aigMgr, node);
      sign = aigIsInv(node);
      node = Ddi_BddarrayRead(timeFrameNodes[k], pos);
      //sign = sign ^ aigIsInv(node); // nxr: NOT SURE
      /* node may be now complemented w.r.t. the previous one: check sign! */
      lit = (k>0 ^ sign) ? aigCnfLit(S, node) : -aigCnfLit(S, node);
      if (k) {
	/* assumption */
	/* GpC */ MinisatClause1(S, lits, lit);
      } else {
	lemmaEqLits[pos] = lit;
      }
    }
  }

  /* equivalences */
  for (k=0; k<=(lemmaMgr->doStrong ? 0 : idepth); k++) {
    for (i=1; i<Ddi_BddarrayNum(lemmaClasses); i++) {
      lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);

      node0 = Ddi_BddPartRead(lemmaClass, 0);
      pos0 = aigIndexRead(aigMgr, node0);
      sign0 = aigIsInv(node0);
      node0 = Ddi_BddarrayRead(timeFrameNodes[k], pos0);
      lit0 = sign0 ? -aigCnfLit(S, node0) : aigCnfLit(S, node0);

      for (j=1; j<Ddi_BddPartNum(lemmaClass); j++) {
	node1 = Ddi_BddPartRead(lemmaClass, j);
	pos1 = aigIndexRead(aigMgr, node1);
	sign1 = aigIsInv(node1);
	node1 = Ddi_BddarrayRead(timeFrameNodes[k], pos1);
	lit1 = sign1 ? -aigCnfLit(S, node1) : aigCnfLit(S, node1);

	if (k) {
	  /* assumption */
	  MinisatClause2(S, lits, lit0, -lit1);
	  MinisatClause2(S, lits, -lit0, lit1);
	} else {
	  lit = aig2CnfNewIdIntern(ddm);
	  while (abs(lit) > S.nVars()) S.newVar();

	  MinisatClause3(S, lits, -lit, -lit0, -lit1);
	  MinisatClause3(S, lits, -lit, lit0, lit1);
	  lemmaEqLits[pos1] = lit;
	}
      }
    }
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
potentialLemmaClauses(
  lemmaMgr_t *lemmaMgr,
  Ddi_Bddarray_t *lemmaClasses,
  short int ***lemmaImp,
  int **lemmaEqLits,
  int ***lemmaImpLits,
  int idepth,
  Solver& S
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaClasses);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  int i, j, id, num, pPos, qPos, sign, pSign, qSign, redundant;
  Ddi_Bddarray_t **timeFrameNodes=lemmaMgr->timeFrameNodes;
  short int **redImp=lemmaMgr->redImp;
  char **combImp=lemmaMgr->combImp;
  Ddi_Bdd_t *p, *q, *lemmaClass;

  if (!lemmaImp) {
     /* equivalences */
     if (lemmaMgr->nConst) {
	lemmaClass = Ddi_BddarrayRead(lemmaClasses, 0);
	potentialLemmaClassClauses(lemmaClass, timeFrameNodes, 
				   lemmaEqLits, idepth, S, 1);
     }
     if (lemmaMgr->nEquiv) {
	for (i=1; i<Ddi_BddarrayNum(lemmaClasses); i++) {
	   lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
	   potentialLemmaClassClauses(lemmaClass, timeFrameNodes, 
				      lemmaEqLits, idepth, S, 0);
	}
     }
  } else {
     /* implications */
     for (i=1; i<Ddi_BddarrayNum(lemmaClasses)-1; i++) {
	lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
	p = Ddi_BddPartRead(lemmaClass, 0);
	pPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(p));
	pSign = aigIsInv(p);
	for (sign=0; sign<=1; sign++) {
	   num = lemmaImp[sign][0][i];
	   for (j=0; j<num; j++) {
	      id = lemmaImp[sign][i][j];
	      qSign = id < 0;
	      lemmaClass = Ddi_BddarrayRead(lemmaClasses, abs(id));
	      q = Ddi_BddPartRead(lemmaClass, 0);
	      qPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(q));
	      qSign ^= aigIsInv(q);
	      assert(redImp[i][abs(id)] >= 0);
	      redundant = redImp[i][abs(id)] || combImp[pPos][qPos];
	      //assert(!redImp[i][abs(id)] || !lemmaImpLits[0][pPos][qPos]);
	      if (!redundant && !lemmaImpLits[0][pPos][qPos]) {
		 timeFrameLemmaClauses(timeFrameNodes, NULL, lemmaImpLits, 
				       idepth, S, pPos, sign ^ pSign, 
				       qPos, qSign, 1, 1);
	      }
	   }
	}
     }
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
potentialLemmaClassClauses(
  Ddi_Bdd_t *lemmaClass,
  Ddi_Bddarray_t **timeFrameNodes,
  int **lemmaEqLits,
  int idepth,
  Solver& S,
  int constant
)
{
   Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaClass);
   bAig_Manager_t *manager = ddm->aig.mgr;
   int j, k, pos, pos0, sign, sign0;
   Ddi_Bdd_t *node;

   if (constant) {
      for (j=0; j<Ddi_BddPartNum(lemmaClass); j++) {
	 node = Ddi_BddPartRead(lemmaClass, j);
	 sign = aigIsInv(node);
	 pos = bAig_AuxAig0(manager, Ddi_BddToBaig(node));
	 assert(lemmaEqLits[pos][0] == 0);
	 for (k=0; k<=idepth; k++) {
	    node = Ddi_BddarrayRead(timeFrameNodes[k], pos);
	    lemmaEqLits[pos][k] = (k>0 ^ sign) ? aigCnfLit(S, node) :
	                                        -aigCnfLit(S, node);
	 }
      }
      return;
   }

   if (Ddi_BddPartNum(lemmaClass) > 1) {
      /* generate equivalences */
      node = Ddi_BddPartRead(lemmaClass, 0);
      pos0 = bAig_AuxAig0(manager, Ddi_BddToBaig(node));
      sign0 = aigIsInv(node);
      for (j=1; j<Ddi_BddPartNum(lemmaClass); j++) {
	 node = Ddi_BddPartRead(lemmaClass, j);
	 sign = aigIsInv(node);
	 pos = bAig_AuxAig0(manager, Ddi_BddToBaig(node));
	 timeFrameLemmaClauses(timeFrameNodes, lemmaEqLits, NULL, 
	                  idepth, S, pos0, sign0, pos, sign, 0, 1);
      }
   }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
provedLemmaClauses (
  lemmaMgr_t *lemmaMgr,
  Ddi_Bddarray_t *provedClasses,
  int **lemmaEqLits,
  short int ***provedImp,
  int idepth,
  Solver& S
)
{
  Ddi_Bddarray_t *lemmaNodes = lemmaMgr->lemmaNodes;
  Ddi_Bddarray_t **timeFrameNodes = lemmaMgr->timeFrameNodes;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaNodes);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  Ddi_Bdd_t *lemmaClass, *node;
  int i, j, k, pos, sign, lit, pos0, sign0, lit0;
  vec<Lit> lits;

  /* constants */
  lemmaClass = Ddi_BddarrayRead(provedClasses, 0);
  for (j=0; j<Ddi_BddPartNum(lemmaClass); j++) {
    node = Ddi_BddPartRead(lemmaClass, j);
    sign = aigIsInv(node);
    pos = aigIndexRead(aigMgr, node);
    for (k=1; k<=idepth; k++) {
      node = Ddi_BddarrayRead(timeFrameNodes[k], pos);
      lit = sign ? -aigCnfLit(S, node) : aigCnfLit(S, node);
      MinisatClause1(S, lits, lit);
    }
  }

  /* equivalences */
  for (i=1; i<Ddi_BddarrayNum(provedClasses); i++) {
    lemmaClass = Ddi_BddarrayRead(provedClasses, i);
    node = Ddi_BddPartRead(lemmaClass, 0);
    sign0 = aigIsInv(node);
    pos0 = aigIndexRead(aigMgr, node);
    for (j=1; j<Ddi_BddPartNum(lemmaClass); j++) {
      node = Ddi_BddPartRead(lemmaClass, j);
      sign = aigIsInv(node);
      pos = aigIndexRead(aigMgr, node);
      for (k=1; k<=idepth; k++) {
	node = Ddi_BddarrayRead(timeFrameNodes[k], pos0);
	lit0 = sign0 ? -aigCnfLit(S, node) : aigCnfLit(S, node);
	node = Ddi_BddarrayRead(timeFrameNodes[k], pos);
	lit = sign ? -aigCnfLit(S, node) : aigCnfLit(S, node);
	MinisatClause2(S, lits, -lit0, lit);
	MinisatClause2(S, lits, lit0, -lit);
      }
    }
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
provedLemmaClassClauses(
  Ddi_Bdd_t *lemmaClass,
  Ddi_Bddarray_t **timeFrameNodes,
  int **lemmaEqLits,
  int assumeFrames,
  Solver& S,
  int constant
)
{
   Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaClass);
   bAig_Manager_t *manager = ddm->aig.mgr;
   int j, k, pos, pos0, lit, sign, sign0;
   Ddi_Bdd_t *node;
   vec<Lit> lits;

   lits.clear();
   if (constant) {
      for (j=0; j<Ddi_BddPartNum(lemmaClass); j++) {
	 node = Ddi_BddPartRead(lemmaClass, j);
	 sign = aigIsInv(node);
	 pos = bAig_AuxAig0(manager, Ddi_BddToBaig(node));
	 for (k=0; k<=assumeFrames; k++) {
	    node = Ddi_BddarrayRead(timeFrameNodes[k], pos);
	    lit = sign ? -aigCnfLit(S, node) : aigCnfLit(S, node);
	    lits.push(MinisatLit(lit));
	    S.addClause(lits);
	    lits.clear();
	 }
      }
      return;
   }

   node = Ddi_BddPartRead(lemmaClass, 0);
   pos0 = bAig_AuxAig0(manager, Ddi_BddToBaig(node));
   sign0 = aigIsInv(node);
   for (j=1; j<Ddi_BddPartNum(lemmaClass); j++) {
      node = Ddi_BddPartRead(lemmaClass, j);
      sign = aigIsInv(node);
      pos = bAig_AuxAig0(manager, Ddi_BddToBaig(node));
      timeFrameLemmaClauses(timeFrameNodes, lemmaEqLits, NULL,
			    assumeFrames, S, pos0, sign0, pos, sign, 
			    0, lemmaEqLits ? 3 : 0);
   }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
lemmaImpConsistencyCheck (
  Ddi_Bddarray_t *lemmaClasses,
  short int ***lemmaImp,
  short int **redImp,
  int nNodes
)
{
   int i, j, k, sign, classesNum, impNum;

   classesNum = Ddi_BddarrayNum(lemmaClasses);
   for (i=1; i<classesNum; i++) {
      for (sign=0; sign<=1; sign++) {
	 impNum = lemmaImp[sign][0][i];
	 for (j=0; j<impNum; j++) {
	    k = abs(lemmaImp[sign][i][j]);
	    assert(k > i);
	    assert(redImp[i][k] >= 0);
	    assert(redImp[k][i] >= 0);
	 }

	 impNum = lemmaImp[sign][i][nNodes-i];
	 for (j=impNum; j<nNodes; j++) {
	    k = abs(lemmaImp[sign][i][j]);
	    assert(k < i);
	    assert(redImp[i][k] >= 0);
	    assert(redImp[k][i] >= 0);
	 }
      }
   }

   return 1;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
lemmaImpWrite (
  Ddi_Bddarray_t *lemmaClasses,
  short int ***lemmaImp,
  int nNodes
)
{

   Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaClasses);
   int pos, num, sign, pSign, qSign;
   int i, j, k, qClassId, rClassId;

   if (lemmaClasses) {
      num = Ddi_BddarrayNum(lemmaClasses);
   } else {
      num = nNodes;
   }
   for (sign=0; sign<=1; sign++) {
      lemmaImp[sign][0][0] = num;
      fprintf(dMgrO(ddm),"***** SIGN = %d\n", sign);
      for (i=0; i<num; i++) {
	 fprintf(dMgrO(ddm),"%2d) ", i);
	 for (j=0; j<lemmaImp[sign][0][i]; j++) {
	    qClassId = lemmaImp[sign][i][j];
	    fprintf(dMgrO(ddm),"%3d ", qClassId);
	 }
	 fprintf(dMgrO(ddm),"\n");
      }
      lemmaImp[sign][0][0] = 0;
      fprintf(dMgrO(ddm),"\n");
   }

   return 0;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
lemmaImpTransitiveCheck (
  Ddi_Bddarray_t *lemmaClasses,
  short int ***lemmaImp,
  int **countImp,
  int nNodes
)
{
   Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaClasses);
   bAig_Manager_t *aigMgr = ddm->aig.mgr;
   int pos, pPos, qPos, rPos, pNum, qNum, sign, pSign, qSign;
   int i, j, k, l=0, pClassId, qClassId, rClassId;
   short int **redImp;
   Ddi_Bdd_t *p, *q, *r, *pClass, *qClass, *rClass;

   redImp = lemmaImpAlloc2d(nNodes);
   if (countImp) {
      for (i=Ddi_BddarrayNum(lemmaClasses)-1; i>0; i--) {
	 pClass = Ddi_BddarrayRead(lemmaClasses, i);
	 p = Ddi_BddPartRead(pClass, 0);
	 pPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(p));
	 /* inter-classes implications */
	 for (sign=0; sign<=1; sign++) {
	    pNum = lemmaImp[sign][0][i];
	    assert(pNum < nNodes-i);
	    for (j=0; j<pNum; j++) {
	       qSign = lemmaImp[sign][i][j] < 0;
	       qClassId = abs(lemmaImp[sign][i][j]);
	       qClass = Ddi_BddarrayRead(lemmaClasses, qClassId);
	       q = Ddi_BddPartRead(qClass, 0);
	       qPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(q));
	       qNum = lemmaImp[!qSign][0][qClassId];
	       redImp[pPos][qPos] = ++redImp[qPos][pPos];

	       for (k=0; k<qNum; k++) {
		  rClassId = lemmaImp[!qSign][qClassId][k];
		  rClass = Ddi_BddarrayRead(lemmaClasses, abs(rClassId));
		  r = Ddi_BddPartRead(rClass, 0);
		  rPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(r));
		  assert(redImp[qPos][rPos] >= 0);
		  redImp[pPos][rPos] = ++redImp[rPos][pPos];
	       }
	    }
	 }
	 /* intra-class implications */
	 pNum = lemmaImp[0][0][i];
	 qNum = lemmaImp[1][0][i];
	 for (j=0; j<pNum; j++) {
	    pos = lemmaImp[0][i][j];
	    qClass = Ddi_BddarrayRead(lemmaClasses, abs(pos));
	    q = Ddi_BddPartRead(qClass, 0);
	    qPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(q));
	    qSign = pos < 0;
	    for (k=0; k<qNum; k++) {
	       pos = lemmaImp[1][i][k];
	       rClass = Ddi_BddarrayRead(lemmaClasses, abs(pos));
	       r = Ddi_BddPartRead(rClass, 0);
	       rPos = bAig_AuxAig0(aigMgr, Ddi_BddToBaig(r));
	       assert(redImp[qPos][rPos] >= 0);
	       redImp[qPos][rPos] = ++redImp[rPos][qPos];
	    }
	 }
      }
      /* verify equality between the two databases */
      for (i=1; i<nNodes; i++) {
	 for (j=1; j<nNodes; j++) {
	    assert(countImp[i][j] == redImp[i][j]-1);
	 }
      }
   } else {
      /* build DAG */
      for (i=Ddi_BddarrayNum(lemmaClasses)-1; i>0; i--) {
	 pClassId = i;
	 for (sign=0; sign<=1; sign++) {
	    pNum = lemmaImp[sign][0][pClassId];
	    assert(pNum < nNodes-pClassId);
	    for (j=0; j<pNum; j++) {
	       qClassId = abs(lemmaImp[sign][i][j]);
	       assert(!redImp[qClassId][pClassId]);
	       redImp[pClassId][qClassId] = ++redImp[qClassId][pClassId];
	    }
	 }
      }

      /* check transitivity */
      for (i=Ddi_BddarrayNum(lemmaClasses)-1; i>0; i--) {
	 pClassId = i;
	 /* inter-classes implications */
	 for (sign=0; sign<=1; sign++) {
	    pNum = lemmaImp[sign][0][pClassId];
	    assert(pNum < nNodes-pClassId);
	    for (j=0; j<pNum; j++) {
	       qSign = lemmaImp[sign][i][j] < 0;
	       qClassId = abs(lemmaImp[sign][i][j]);
	       assert(redImp[pClassId][qClassId] > 0);
	       qNum = lemmaImp[!qSign][0][qClassId];
	       for (k=0; k<qNum; k++) {
		  rClassId = abs(lemmaImp[!qSign][qClassId][k]);
		  assert(redImp[qClassId][rClassId] > 0);
		  assert(redImp[pClassId][rClassId] > 0);
	       }
	    }
	 }
	 /* intra-class implications */
	 pNum = lemmaImp[0][0][i];
	 qNum = lemmaImp[1][0][i];
	 for (j=0; j<pNum; j++) {
	    qClassId = abs(lemmaImp[0][i][j]);
	    for (k=0; k<qNum; k++) {
	       rClassId = abs(lemmaImp[1][i][k]);
	       assert(redImp[qClassId][rClassId] > 0);
	    }
	 }
      }
   }

   for (i=0; i<nNodes; i++) {
      Pdtutil_Free(redImp[i]);
   }
   Pdtutil_Free(redImp);
   return 1;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
lemmaImpReduce (
  lemmaMgr_t *lemmaMgr,
  Ddi_Bddarray_t *lemmaClasses,
  short int ***lemmaImp,
  int nNodes
)
{
   Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaClasses);
   bAig_Manager_t *aigMgr = ddm->aig.mgr;
   int pos, pPos, qPos, rPos, pNum, qNum, sign, pSign, qSign;
   int i, j, k, qClassId, rClassId;
   Ddi_Bdd_t *lemmaClass, *node;
   short int **redImp = lemmaMgr->redImp;

   for (i=0; i<nNodes; i++) {
      memset(redImp[i], -1, nNodes*sizeof(short int));
   }
   for (i=Ddi_BddarrayNum(lemmaClasses)-1; i>0; i--) {
      /* inter-classes implications */
      for (sign=0; sign<=1; sign++) {
	 pNum = lemmaImp[sign][0][i];
	 assert(pNum < nNodes-i);
	 lemmaImp[sign][i][nNodes-i] = nNodes;
	 for (j=0; j<pNum; j++) {
	    qSign = lemmaImp[sign][i][j] < 0;
	    qClassId = abs(lemmaImp[sign][i][j]);
	    redImp[i][qClassId] = ++redImp[qClassId][i];
	    pos = --lemmaImp[qSign][qClassId][nNodes-qClassId];
	    lemmaImp[qSign][qClassId][pos] = sign ? -i : i;
	    qNum = lemmaImp[!qSign][0][qClassId];
	    for (k=0; k<qNum; k++) {
	       rClassId = abs(lemmaImp[!qSign][qClassId][k]);
	       //assert(redImp[qClassId][rClassId] >= 0);
	       redImp[i][rClassId] = ++redImp[rClassId][i];
	    }
	 }
      }
      /* intra-class implications (outgoing edges) */
      pNum = lemmaImp[0][0][i];
      qNum = lemmaImp[1][0][i];
      for (j=0; j<pNum; j++) {
	 qClassId = abs(lemmaImp[0][i][j]);
	 for (k=0; k<qNum; k++) {
	    rClassId = abs(lemmaImp[1][i][k]);
	    //assert(redImp[qClassId][rClassId] >= 0);
	    redImp[qClassId][rClassId] = ++redImp[rClassId][qClassId];
	 }
      }
   }

   /* intra-class implications (incoming edges) */
   for (i=Ddi_BddarrayNum(lemmaClasses)-1; i>0; i--) {
      pNum = lemmaImp[0][i][nNodes-i];
      qNum = lemmaImp[1][i][nNodes-i];
      for (j=pNum; j<nNodes; j++) {
	 qClassId = abs(lemmaImp[0][i][j]);
	 for (k=qNum; k<nNodes; k++) {
	    rClassId = abs(lemmaImp[1][i][k]);
	    //assert(redImp[qClassId][rClassId] >= 0);
	    redImp[qClassId][rClassId] = ++redImp[rClassId][qClassId];
	 }
      }
   }

   return 1;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
disprovedLemmaClauses (
  lemmaMgr_t *lemmaMgr,
  Ddi_Bddarray_t *lemmaClasses,
  short int ***lemmaImp,
  int *lemmaEqLits,
  int ***lemmaImpLits,
  int idepth,
  Solver& S,
  vec<Lit>& cex,
  Ddi_Bddarray_t *falseLemmas
)
{
   Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaClasses);
   bAig_Manager_t *aigMgr = ddm->aig.mgr;
   int i, j, k, l=-1, id, lit0, lit1, litv, lit0v, lit1v, var0, var1, *lemmaSplit=NULL;
   int pos, pPos, qPos, rPos, tPos, lit0s, lit1s, posMin, idxMin, count=0;
   int sign, pSign, qSign, newNum, nNodes, nFalse=0;
   short int *currImp, ***oldImp;
   Ddi_Bdd_t *node, *lemmaClass, *newClass, *node0, *node1;
   Ddi_Bddarray_t **timeFrameNodes=lemmaMgr->timeFrameNodes;
   short int **redImp=lemmaMgr->redImp;
   char **combImp=lemmaMgr->combImp;
   bAigEdge_t baig, left, right;
   vec<Lit> lits;
   int checkSim=0;

   if (checkSim) {
     checkFalseLemmaClause(lemmaMgr, lemmaClasses, lemmaEqLits, NULL, lemmaImpLits, idepth, nNodes, S);
   }

   /* simulate original circuit */
   lits.clear();
   nNodes = Ddi_BddarrayNum(lemmaMgr->lemmaNodes);
   for (i=1; i<nNodes; i++) {
     node = Ddi_BddarrayRead(lemmaMgr->lemmaNodes, i);
     baig = Ddi_BddToBaig(node);
     if (bAig_isVarNode(aigMgr, baig)) {
       node = Ddi_BddarrayRead(timeFrameNodes[0], i);
       assert(bAig_CnfId(aigMgr, Ddi_BddToBaig(node)) >= 0);
       lit0 = aigCnfLit(S, node);
       pSign = (lit0 < 0);
       var0 = abs(lit0) - 1;
       assert(S.model[var0] != l_Undef);
       nodeAuxChar(aigMgr, baig) = (S.model[var0]==l_True) ^ pSign;
       lits.push(nodeAuxChar(aigMgr, baig) ? MinisatLit(-lit0) : MinisatLit(lit0));
     } else {
       left = bAig_NodeReadIndexOfLeftChild(aigMgr, baig);
       right = bAig_NodeReadIndexOfRightChild(aigMgr, baig);
       assert(nodeAuxChar(aigMgr, left) != 2);
       assert(nodeAuxChar(aigMgr, right) != 2);
       nodeAuxChar(aigMgr, baig) = (nodeAuxChar(aigMgr, left)^bAig_NodeIsInverted(left)) &&
	                           (nodeAuxChar(aigMgr, right)^bAig_NodeIsInverted(right));
       if (checkSim) {
         lit0 = aig2CnfIdSigned(aigMgr, baig);
         pSign = (lit0 < 0);
         var0 = abs(lit0) - 1;
         //assert(S.model[var0] != l_Undef);
         if (S.model[var0] == true) {
           if (!nodeAuxChar(aigMgr, baig)) {
             printf("simulation mismatch\n");
           }
         }
         else if (S.model[var0] == false) {
           if (nodeAuxChar(aigMgr, baig)) {
             printf("simulation mismatch\n");
           }
         }
       }
     }
   }
   S.addClause(lits);

   /* false equivalences */
   for (i=Ddi_BddarrayNum(lemmaClasses)-1; i>0; i--) {
     lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
     node = Ddi_BddPartRead(lemmaClass, 0);
     pPos = aigIndexRead(aigMgr, node);
     pSign = aigIsInv(node);
     posMin = lemmaMgr->nNodes;

     /* value by simulation */
     node = Ddi_BddarrayRead(lemmaMgr->lemmaNodes, pPos);
     assert(nodeAuxChar(aigMgr, Ddi_BddToBaig(node)) != 2);
     lit0s = (nodeAuxChar(aigMgr, Ddi_BddToBaig(node))) ^ pSign; //literal value

     assert(Ddi_BddPartNum(lemmaClass) > 1);
     if (Ddi_BddPartNum(lemmaClass) > 1) {
       newClass = Ddi_BddMakePartConjVoid(ddm);
       for (j=Ddi_BddPartNum(lemmaClass)-1; j>0; j--) {
	 node = Ddi_BddPartRead(lemmaClass, j);
	 qPos = aigIndexRead(aigMgr, node);
	 qSign = aigIsInv(node);
	 node = Ddi_BddarrayRead(lemmaMgr->lemmaNodes, qPos);
	 assert(nodeAuxChar(aigMgr, Ddi_BddToBaig(node)) != 2);
	 lit1s = (nodeAuxChar(aigMgr, Ddi_BddToBaig(node))) ^ qSign; //literal value

	 //assert(lit0v^lit1v == lit0s^lit1s);
	 if (lit1s != lit0s) {
	   /* "false" lemma */
	   nFalse++;
	   if (qPos < posMin) {
	     posMin = qPos;
	     idxMin = Ddi_BddPartNum(newClass);
	   }
	   node = Ddi_BddPartQuickExtract(lemmaClass, j);
	   Ddi_BddPartInsertLast(newClass, node);
	   Ddi_Free(node);
	   /* invalidate previous lemma constraints */
	   //assert(lemmaEqLits[qPos][0]);
	   if (lemmaEqLits[qPos]) {
	     MinisatClause1(S, lits, -lemmaEqLits[qPos]);
	   }
	   lemmaEqLits[qPos] = 0;
	 }
       }

       /* add clauses for new potential lemmas */
       newNum = Ddi_BddPartNum(newClass);
       if (newNum) {
	 lemmaMgr->nEquiv--;
	 //lemmaMgr->nEquiv -= Ddi_BddPartNum(newClass);
	 if (newNum > 1) {
	   Ddi_BddPartSwap(newClass, 0, idxMin);
	   //lemmaSortClassAcc(newClass);
	   assert(Ddi_BddPartNum(newClass) > 1);
	   Ddi_BddarrayInsertLast(lemmaClasses, newClass);
	 }
	 if (falseLemmas) {
	   node0 = Ddi_BddPartRead(lemmaClass, 0);
	   node1 = Ddi_BddPartRead(newClass, 0);
	   node = Ddi_BddXnor(node0, node1);
	   Ddi_BddarrayInsertLast(falseLemmas, node);
	   Ddi_Free(node);
	 }
       }
       if (Ddi_BddPartNum(lemmaClass) == 1) {
	 Ddi_BddarrayQuickRemove(lemmaClasses, i);
       }
       Ddi_Free(newClass);
     }

   } /* end for i in classes */

   /* false constant lemmas */
   lemmaClass = Ddi_BddarrayRead(lemmaClasses, 0);
   newClass = Ddi_BddMakePartConjVoid(ddm);
   posMin = lemmaMgr->nNodes;
   for (j=Ddi_BddPartNum(lemmaClass)-1; j>=0; j--) {
     node = Ddi_BddPartRead(lemmaClass, j);
     //printf("Node: %d ", Ddi_BddToBaig(node));
     pos = aigIndexRead(aigMgr, node);
     pSign = aigIsInv(node);
     node = Ddi_BddarrayRead(lemmaMgr->lemmaNodes, pos);
     litv = !(nodeAuxChar(aigMgr, Ddi_BddToBaig(node)) ^ pSign);
     if (litv) {
       /* "false" lemma */
       nFalse++;
       if (pos < posMin) {
	 posMin = pos;
	 idxMin = Ddi_BddPartNum(newClass);
       }
       node = Ddi_BddPartQuickExtract(lemmaClass, j);
       Ddi_BddPartInsertLast(newClass, node);
       Ddi_Free(node);
       lemmaEqLits[pos] = 0;
     }
   }
   lemmaMgr->nConst = Ddi_BddPartNum(lemmaClass);

   if (Ddi_BddPartNum(newClass)) {
     if (Ddi_BddPartNum(newClass) > 1) {
       Ddi_BddPartSwap(newClass, 0, idxMin);
       //lemmaSortClassAcc(newClass);
       lemmaMgr->nEquiv += Ddi_BddPartNum(newClass)-1;
       Ddi_BddarrayInsertLast(lemmaClasses, newClass);
     }
     if (falseLemmas) {
       node = Ddi_BddPartRead(newClass, 0);
       Ddi_BddarrayInsertLast(falseLemmas, node);
     }
   }
   Ddi_Free(newClass);
   //assert(nFalse);
   return nFalse;
} /* disprovedLemmaClauses */

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void 
lemmaImpAddEdge (
   short int ***lemmaImp,
   short int **redImp,
   int pClassId,
   int pSign,
   int rClassId,
   int rSign,
   int nNodes,
   int do_red
)
{
   short int *impArray, pId, rId, xClassId;
   int k, impNum;

   pId = pSign ? -pClassId : pClassId;
   rId = rSign ? -rClassId : rClassId;

   /* insert direct edge */
   impNum = lemmaImp[pSign][0][pClassId]++;
   lemmaImp[pSign][pClassId][impNum] = rId;
   if (!do_red) return;
   impNum = --lemmaImp[rSign][rClassId][nNodes-rClassId];
   lemmaImp[rSign][rClassId][impNum] = pId;
   redImp[pClassId][rClassId] = ++redImp[rClassId][pClassId];

   /* update reduction: p intra (outgoing) edges */
   impNum = lemmaImp[!pSign][0][pClassId];
   impArray = lemmaImp[!pSign][pClassId];
   for (k=0; k<impNum; k++) {
      xClassId = abs(impArray[k]);
      redImp[xClassId][rClassId] = ++redImp[rClassId][xClassId];
   }

   /* update reduction: r intra (incoming) edges */
   impNum = lemmaImp[!rSign][rClassId][nNodes-rClassId];
   impArray = lemmaImp[!rSign][rClassId];
   for (k=impNum; k<nNodes; k++) {
      xClassId = abs(impArray[k]);
      redImp[xClassId][pClassId] = ++redImp[pClassId][xClassId];
   }

   /* update reduction: p extra (incoming) edges */
   impNum = lemmaImp[!pSign][pClassId][nNodes-pClassId];
   impArray = lemmaImp[!pSign][pClassId];
   for (k=impNum; k<nNodes; k++) {
      xClassId = abs(impArray[k]);
      redImp[xClassId][rClassId] = ++redImp[rClassId][xClassId];
   }

   /* update reduction: r extra (outgoing) edges */
   impNum = lemmaImp[!rSign][0][rClassId];
   impArray = lemmaImp[!rSign][rClassId];
   for (k=0; k<impNum; k++) {
      xClassId = abs(impArray[k]);
      redImp[xClassId][pClassId] = ++redImp[pClassId][xClassId];
   }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void 
lemmaImpRemoveEdge (
   short int ***lemmaImp,
   short int **redImp,
   int pClassId,
   int pSign,
   int rClassId,
   int rSign,
   int classesNum,
   int nNodes
)
{
   short int *impArray, xClassId, pId;//, rId;
   int k, impNum;

   //assert(redImp[pClassId][rClassId] >= 0);
   //redImp[pClassId][rClassId] = redImp[rClassId][pClassId] = -1;
   pId = pSign ? -pClassId : pClassId;
   //rId = rSign ? -rClassId : rClassId;

   impNum = lemmaImp[rSign][rClassId][nNodes-rClassId];
   impArray = lemmaImp[rSign][rClassId];
   for (k=impNum; k<nNodes; k++) {
      if (impArray[k] == pId) {
	 impArray[k] = impArray[impNum];
	 lemmaImp[rSign][rClassId][nNodes-rClassId]++;
	 break;
      }
   }
   assert(k < nNodes);
   
   /* p-intra (outgoing) edges */
   impNum = lemmaImp[!pSign][0][pClassId];
   impArray = lemmaImp[!pSign][pClassId];
   for (k=0; k<impNum; k++) {
      assert((xClassId=abs(impArray[k])) > pClassId);
      //assert((redImp[xClassId][rClassId] = --redImp[rClassId][xClassId]) >= 0);
      redImp[xClassId][rClassId] = --redImp[rClassId][xClassId];
   }

   /* r-intra (incoming) edges */
   impNum = lemmaImp[!rSign][rClassId][nNodes-rClassId];
   impArray = lemmaImp[!rSign][rClassId];
   for (k=impNum; k<nNodes; k++) {
      assert((xClassId=abs(impArray[k])) < rClassId);
      //assert((redImp[xClassId][pClassId] = --redImp[pClassId][xClassId]) >= 0);
      redImp[xClassId][pClassId] = --redImp[pClassId][xClassId];
   }

   /* p-incoming edges */
   impNum = lemmaImp[!pSign][pClassId][nNodes-pClassId];
   impArray = lemmaImp[!pSign][pClassId];
   for (k=impNum; k<nNodes; k++) {
      assert((xClassId=abs(impArray[k])) < pClassId);
      //assert(redImp[xClassId][rClassId] > 0);
      //assert((redImp[xClassId][rClassId] = --redImp[rClassId][xClassId]) >= 0);
      redImp[xClassId][rClassId] = --redImp[rClassId][xClassId];
   }

   /* r-outgoing edges */
   impNum = lemmaImp[!rSign][0][rClassId];
   impArray = lemmaImp[!rSign][rClassId];
   for (k=0; k<impNum; k++) {
      assert((xClassId=abs(impArray[k])) > rClassId);
      //assert((redImp[xClassId][pClassId] = --redImp[pClassId][xClassId]) >= 0);
      redImp[xClassId][pClassId] = --redImp[pClassId][xClassId];
   }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
timeFrameLemmaClauses(
  Ddi_Bddarray_t **timeFrameNodes,
  int **lemmaEqLits,
  int ***lemmaImpLits,
  int idepth,
  Solver& S,
  int pos0,
  int inv0,
  int pos1,
  int inv1,
  int implication, 
  int action
)
{
  Ddi_Mgr_t *ddm=Ddi_ReadMgr(timeFrameNodes);
  Ddi_Bdd_t *p, *q;
  int k, lit0, lit1, lemmaLit;
  vec<Lit> lits;

   assert(!action || (implication ? lemmaImpLits!=NULL : lemmaEqLits!=NULL));
   if (action==0 || action==1) {
      assert(!action || lemmaEqLits || implication);
      /* generate lemma clauses */
      for (k=0; k<=idepth; k++) {
	 p = Ddi_BddarrayRead(timeFrameNodes[k], pos0);
	 lit0 = inv0 ? -aigCnfLit(S, p) : aigCnfLit(S, p);
	 q = Ddi_BddarrayRead(timeFrameNodes[k], pos1);
	 lit1 = inv1 ? -aigCnfLit(S, q) : aigCnfLit(S, q);
	 lemmaLit = enableLemmaClauses(ddm, S, lit0, lit1, !implication, 
				       action && k==0);
	 if (action) {
	    /* save enable lit for assumptions */
	    if (implication) {
	       lemmaImpLits[k][pos0][pos1] = lemmaLit;
	    } else {
	       lemmaEqLits[pos1][k] = lemmaLit;
	    }
	 } else {
	    /* assert the relation to be true */
	    MinisatClause1(S, lits, lemmaLit);
	 }
      }
   } else if (action == 2) {
      /* disproved lemma: remove constraints */
      if (implication) {
	 assert(lemmaImpLits[0][pos0][pos1]);
	 for (k=0; k<=idepth; k++) {
	    MinisatClause1(S, lits, -lemmaImpLits[k][pos0][pos1]);
	    lemmaImpLits[k][pos0][pos1] = 0;
	 }
      } else {
	 assert(lemmaEqLits[pos1][0]);
	 for (k=0; k<=idepth; k++) {
	    MinisatClause1(S, lits, -lemmaEqLits[pos1][k]);
	    lemmaEqLits[pos1][k] = 0;
	 }
      }
   } else if (action == 3) {
      /* proved lemma: force constraints */
      if (implication) {
	 assert(lemmaImpLits[0][pos0][pos1]);
	 for (k=1; k<=idepth; k++) {
	    MinisatClause1(S, lits, lemmaImpLits[k][pos0][pos1]);
	    lemmaImpLits[k][pos0][pos1] = 0;
	 }
	 MinisatClause1(S, lits, -lemmaImpLits[0][pos0][pos1]);
	 lemmaImpLits[0][pos0][pos1] = 0;
	 p = Ddi_BddarrayRead(timeFrameNodes[0], pos0);
	 lit0 = inv0 ? -aigCnfLit(S, p) : aigCnfLit(S, p);
	 q = Ddi_BddarrayRead(timeFrameNodes[0], pos1);
	 lit1 = inv1 ? -aigCnfLit(S, q) : aigCnfLit(S, q);
	 lemmaLit = enableLemmaClauses(ddm, S, lit0, lit1, 0, 0);
	 MinisatClause1(S, lits, lemmaLit);
      } else {
	 assert(lemmaEqLits[pos1][0]);
	 for (k=1; k<=idepth; k++) {
	    MinisatClause1(S, lits, lemmaEqLits[pos1][k]);
	 }
	 MinisatClause1(S, lits, -lemmaEqLits[pos1][0]);
	 p = Ddi_BddarrayRead(timeFrameNodes[0], pos0);
	 lit0 = inv0 ? -aigCnfLit(S, p) : aigCnfLit(S, p);
	 q = Ddi_BddarrayRead(timeFrameNodes[0], pos1);
	 lit1 = inv1 ? -aigCnfLit(S, q) : aigCnfLit(S, q);
	 lemmaLit = enableLemmaClauses(ddm, S, lit0, lit1, 1, 0);
	 MinisatClause1(S, lits, lemmaLit);
      }
   } else if (action == 4) {
      /* clean data structure */
      if (implication) {
	 assert(lemmaImpLits[0][pos0][pos1]);
	 for (k=0; k<=idepth; k++) {
	    lemmaImpLits[k][pos0][pos1] = 0;
	 }
      } else {
	 assert(lemmaEqLits[pos1][0]);
	 for (k=0; k<=idepth; k++) {
	    lemmaEqLits[pos1][k] = 0;
	 }
      }
   } else {
      assert(0);
   }

   return 0;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
enableLemmaClauses(
  Ddi_Mgr_t *ddm,
  Solver& S,
  int lit0,
  int lit1,
  int equality,
  int complement
)
{
   int enableLit = aig2CnfNewIdIntern(ddm);
   vec<Lit> lits;

   //printf("Enable var: %d/%d\n", enableLit, S.nVars());
   while (abs(enableLit) > S.nVars()) S.newVar();

   if (equality) {
      if (complement) lit1 = -lit1;
      MinisatClause3(S,lits,-enableLit,lit0,-lit1);
      MinisatClause3(S,lits,-enableLit,-lit0,lit1);
   } else {
      if (complement) {
	 MinisatClause2(S,lits,-enableLit,-lit0);
	 MinisatClause2(S,lits,-enableLit,-lit1);
      } else {
	 MinisatClause3(S,lits,-enableLit,lit0,lit1);
      }
   }

   return enableLit;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
printLemmaClasses(
  Ddi_Bddarray_t *lemmaClasses,
  char *msg
)
{
   Ddi_Mgr_t *ddm = Ddi_ReadMgr(lemmaClasses);
   int i, nTotLemmas=0;
   Ddi_Bdd_t *lemmaClass;

   for (i=0; i<Ddi_BddarrayNum(lemmaClasses); i++) {
      lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
      nTotLemmas += Ddi_BddPartNum(lemmaClass);
   }
   fprintf(dMgrO(ddm),"%s (%d/%d):", msg, Ddi_BddarrayNum(lemmaClasses), nTotLemmas);
   for (i=0; i<Ddi_BddarrayNum(lemmaClasses); i++) {
      lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
      fprintf(dMgrO(ddm)," %d", Ddi_BddPartNum(lemmaClass));
   }
   fprintf(dMgrO(ddm),"\n"); 
   fflush(NULL);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
AigOneDistanceSimulation (
  Ddi_Bddarray_t **timeFrameNodes,
  Ddi_Bddarray_t *delta,
  Ddi_Vararray_t **psVars,
  Ddi_Vararray_t **piVars,
  Ddi_Bdd_t *initState,
  Ddi_Bdd_t *careBdd,
  Ddi_Bddarray_t *lemmaClasses,
  int **lemmaEqLits,
  short int ***lemmaImp,
  char **combImp,
  int ***lemmaImpLits,
  Solver& S,
  int assumeFrames,
  int trace
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(delta);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  Ddi_Bddarray_t *lemmaNodes=Ddi_BddarrayDup(timeFrameNodes[assumeFrames]);
  Ddi_AigSignatureArray_t *varSigs, *nextSigs, *satSigs, *nodeSigs, *nodeSigsCompl;
  Ddi_AigSignature_t careSig, *tmpSig, *lSig, *lSigCompl, *careSigPtr=&careSig;
  Ddi_AigSignature_t *iSig, *iSigCompl, *jSig, *jSigCompl, *kSig, *kSigCompl;
  Ddi_AigSignature_t zeroSig, oneSig;
  Ddi_Bdd_t *one, *node, *lemma, *lemmaNode, *lemmaClass, *class_i, *class_k; 
  int *splitStart, *splitEnd, impNum[2], result, pos, sign, end, nNodes;
  int i, j, k, l, step, classesNum, initClassesNum, initNum;
  int nVars, nStateVars, nInputVars, i_sig, j_sig, k_sig, l_sig;
  bAigEdge_t baig, base_i, base_j, base_k, base_l;
  bAig_array_t *visitedNodes=bAigArrayAlloc();
  short int *currImp[2], *tmpArray;

  nVars = Ddi_MgrReadNumVars(ddm);
  nStateVars = Ddi_VararrayNum(psVars[0]);
  nInputVars = Ddi_VararrayNum(piVars[0])*(assumeFrames+1);
  one = Ddi_BddMakeConstAig(ddm, 1);
  lemmaClass = Ddi_BddarrayRead(lemmaClasses, 0);
#if 0
  if (Ddi_BddPartNum(lemmaClass)) {
     lemmaNode = Ddi_BddPartRead(lemmaClass, 0);
     Ddi_BddPartInsertLast(lemmaClass, lemmaNode);
     Ddi_BddPartWrite(lemmaClass, 0, one);
  } else {
     Ddi_BddPartInsertLast(lemmaClass, one);
  }
#else
  Ddi_BddPartInsert(lemmaClass, 0, one);
#endif
  Ddi_Free(one);
  initNum = Ddi_BddarrayNum(lemmaClasses);
  nNodes = Ddi_BddarrayNum(lemmaNodes);
  for (i=0; i<nNodes; i++) {
     node = Ddi_BddarrayRead(lemmaNodes, i);
     baig = Ddi_BddToBaig(node);
     assert(i ? !aigIsInv(node) : aigIsInv(node));
     bAigArrayWriteLast(visitedNodes, baig);
  }
  DdiSetSignatureConstant(&careSig, 1);
  DdiSetSignatureConstant(&oneSig, 1);
  DdiSetSignatureConstant(&zeroSig, 0);
  varSigs = DdiAigSignatureArrayAlloc(nVars);
  satSigs = DdiAigSignatureArrayAlloc(nVars);
  nextSigs = DdiAigSignatureArrayAlloc(nStateVars);
  nodeSigsCompl = DdiAigSignatureArrayAlloc(visitedNodes->num);
  //if (careBdd) {
  //   Ddi_BddarrayInsertLast(lemmasBase, careBdd);
  //}
  if (lemmaImp) {
     currImp[0] = Pdtutil_Alloc(short int, nNodes);
     currImp[1] = Pdtutil_Alloc(short int, nNodes);
     splitStart = Pdtutil_Alloc(int, nNodes);
     splitEnd = Pdtutil_Alloc(int, nNodes);
  }
  DdiSetSignaturesRandom(varSigs,nVars);

  step = 0;
  do { 
     /* generate patterns */
     end = DdiUpdateInputVarSignatureAllBits(satSigs, psVars, piVars, 
                                  S, assumeFrames, step++, !initState);

     for (j=0; j<Ddi_VararrayNum(psVars[0]); j++) {
	Ddi_Var_t *v = Ddi_VararrayRead(psVars[0], j); 
	nextSigs->sArray[j] = satSigs->sArray[Ddi_VarIndex(v)];
     }

     /* sequential simulation */
     for (i=0; i<=assumeFrames; i++) {
	for (j=0; j<Ddi_VararrayNum(psVars[assumeFrames]); j++) {
	   Ddi_Var_t *v = Ddi_VararrayRead(psVars[assumeFrames], j); 
	   varSigs->sArray[Ddi_VarIndex(v)] = nextSigs->sArray[j];
	}
	for (j=0; j<Ddi_VararrayNum(piVars[i]); j++) {
	   Ddi_Var_t *vsim = Ddi_VararrayRead(piVars[assumeFrames], j); 
	   Ddi_Var_t *vsat = Ddi_VararrayRead(piVars[i], j); 
	   varSigs->sArray[Ddi_VarIndex(vsim)] = satSigs->sArray[Ddi_VarIndex(vsat)];
	}
	nodeSigs = DdiAigEvalSignature(ddm,visitedNodes,bAig_NULL,0,varSigs);

	for (j=0; j<Ddi_VararrayNum(psVars[assumeFrames]); j++) {
	   baig = Ddi_BddToBaig(Ddi_BddarrayRead(delta, j));
	   pos = bAig_AuxAig0(bmgr, baig);
	   //Pdtutil_Assert(pos != -1, "wrong auxId field");
	   nextSigs->sArray[j] = nodeSigs->sArray[pos];
	   if (bAig_NodeIsInverted(baig)) {
	      DdiComplementSignature(&(nextSigs->sArray[j]));
	   }
	}
	if (i < assumeFrames) {
	   DdiAigSignatureArrayFree(nodeSigs);
	}
     }
     for (i=0; i<visitedNodes->num; i++) {
	baig = visitedNodes->nodes[i];
	nodeSigsCompl->sArray[i] = nodeSigs->sArray[i];
	DdiComplementSignature(&(nodeSigsCompl->sArray[i]));
     }
     //if (careBdd) { /* AUXINT -> AUXAIG0 !!! */
     //	baig = Ddi_BddToBaig(careBdd);
     //	Pdtutil_Assert(bAig_AuxInt(bmgr,baig) == nNodes-1, "wrong auxId field");
     //	if (aigIsInv(careBdd)) {
     //	   careSigPtr = &nodeSigsCompl->sArray[bAig_AuxInt(bmgr,baig)];
     //	} else {
     //	   careSigPtr = &nodeSigs->sArray[bAig_AuxInt(bmgr,baig)];
     //	}
     //}
     if (lemmaImp) {
	for (i=0; i<nNodes; i++) {
	   splitStart[i] = splitEnd[i] = 0;
	}
     }

     /* refine invariants */
     classesNum = initClassesNum = Ddi_BddarrayNum(lemmaClasses);
     for (i=initClassesNum-1; i>=0; i--) {
	result = Ddi_BddarrayNum(lemmaClasses);
	class_i = Ddi_BddarrayRead(lemmaClasses, i);
	base_i = Ddi_BddToBaig(Ddi_BddPartRead(class_i, 0));
	i_sig = bAig_AuxAig0(bmgr,base_i);
	iSig = &nodeSigs->sArray[i_sig];
	iSigCompl = &nodeSigsCompl->sArray[i_sig];
	if (bAig_NodeIsInverted(base_i)) {
	   iSig = &nodeSigsCompl->sArray[i_sig];
	   iSigCompl = &nodeSigs->sArray[i_sig];
	}

	/* equivalences */
	for (j=Ddi_BddPartNum(class_i)-1; j>0; j--) {
	   base_j = Ddi_BddToBaig(Ddi_BddPartRead(class_i,j));
	   j_sig = bAig_AuxAig0(bmgr,base_j);
	   jSig = &nodeSigs->sArray[j_sig];
	   jSigCompl = &nodeSigsCompl->sArray[j_sig];
	   if (bAig_NodeIsInverted(base_j)) {
	      jSig = &nodeSigsCompl->sArray[j_sig];
	      jSigCompl = &nodeSigs->sArray[j_sig];
	   }

	   if (!DdiEqSignatures(iSig,jSig,careSigPtr)) {
	      lemma = Ddi_BddPartQuickExtract(class_i, j);
#if 0
	      if (i>0 && i<initNum) {
	         pos = bAig_AuxAig0(bmgr, Ddi_BddToBaig(lemma));
	         timeFrameLemmaClauses(timeFrameNodes, lemmaEqLits, NULL, 
				       assumeFrames, S, 0, 0, pos, 0, 0, 2);
	      }
#endif
	      for (k=classesNum; k<Ddi_BddarrayNum(lemmaClasses); k++) {
		 class_k = Ddi_BddarrayRead(lemmaClasses, k);
		 base_k = Ddi_BddToBaig(Ddi_BddPartRead(class_k, 0));
		 k_sig = bAig_AuxAig0(bmgr, base_k);
		 kSig = &nodeSigs->sArray[k_sig];
		 if (bAig_NodeIsInverted(base_k)) {
		    kSig = &nodeSigsCompl->sArray[k_sig];
		 }
		 if (DdiEqSignatures(jSig,kSig,careSigPtr)) {
		    Ddi_BddPartInsertLast(class_k, lemma);
		    break;
		 }
	      }
	      if (k == Ddi_BddarrayNum(lemmaClasses)) {
		 /* no equivalences: add new class */ 
		 Ddi_Bdd_t *newClass = Ddi_BddMakePartConjVoid(ddm);
		 Ddi_BddPartInsertLast(newClass, lemma);
		 Ddi_BddarrayInsertLast(lemmaClasses, newClass);
		 Ddi_Free(newClass);
	      }
	      Ddi_Free(lemma);
	   }
	   if (lemmaImp && classesNum < Ddi_BddarrayNum(lemmaClasses)) {
	      splitStart[i] = classesNum;
	      splitEnd[i] = Ddi_BddarrayNum(lemmaClasses);
	   }
	}

	/* implications */
	if (lemmaImp) {
	   if (i == 0) {
	      /* generate only new "inter" implications */
	      for (k=splitStart[i]; k<splitEnd[i]; k++) {
		 lemmaClass = Ddi_BddarrayRead(lemmaClasses, k);
		 lemmaNode = Ddi_BddPartRead(lemmaClass, 0);
		 base_k = Ddi_BddToBaig(lemmaNode);
		 k_sig = bAig_AuxAig0(bmgr, base_k);
		 kSig = &nodeSigs->sArray[k_sig];
		 if (bAig_NodeIsInverted(base_k)) {
		    kSig = &nodeSigsCompl->sArray[k_sig];
		 }
		 for (l=1; l<splitStart[i]; l++) {
		    lemmaClass = Ddi_BddarrayRead(lemmaClasses, l);
		    lemmaNode = Ddi_BddPartRead(lemmaClass, 0);
		    base_l = Ddi_BddToBaig(lemmaNode);
		    l_sig = bAig_AuxAig0(bmgr, base_l);
		    lSig = &nodeSigs->sArray[l_sig];
		    lSigCompl = &nodeSigsCompl->sArray[l_sig];
		    if (bAig_NodeIsInverted(base_l)) {
		       lSig = &nodeSigsCompl->sArray[l_sig];
		       lSigCompl = &nodeSigs->sArray[l_sig];
		    }
		    /* TWO exclusive implications */
		    if (DdiImplySignatures(lSigCompl, kSig, careSigPtr)) { 
		       /* !l => k, that is, l + k */
		       pos = lemmaImp[0][0][l]++;
		       lemmaImp[0][l][pos] = k;
		    } else if (DdiImplySignatures(lSig, kSig, careSigPtr)) { 
		       /* l => k, that is, !l + k */
		       pos = lemmaImp[1][0][l]++;
		       lemmaImp[1][l][pos] = k;
		    }
		 }
	      }
	      continue;
	   }

	   /* manage "inter" implications */
	   for (sign=0; sign<2; sign++) {
	      impNum[sign] = lemmaImp[sign][0][i];
	      lemmaImp[sign][0][i] = 0;
	      tmpArray = lemmaImp[sign][i];
	      lemmaImp[sign][i] = currImp[sign];
	      currImp[sign] = tmpArray;
	   }

	   for (sign=0; sign<2; sign++) {
	      if (sign) {
		 tmpSig = iSig;
		 iSig = iSigCompl;
		 iSigCompl = tmpSig;
	      }
	      for (j=0; j<impNum[sign]; j++) {
		 lemmaClass = Ddi_BddarrayRead(lemmaClasses, abs(currImp[sign][j]));
		 lemmaNode = Ddi_BddPartRead(lemmaClass, 0);
		 base_j = Ddi_BddToBaig(lemmaNode);
		 j_sig = bAig_AuxAig0(bmgr, base_j);
		 jSig = &nodeSigs->sArray[j_sig];
		 jSigCompl = &nodeSigsCompl->sArray[j_sig];
		 if ((currImp[sign][j]<0) ^ bAig_NodeIsInverted(base_j)) {
		    jSig = &nodeSigsCompl->sArray[j_sig];
		    jSigCompl = &nodeSigs->sArray[j_sig];
		 }
		 /* check original implication */
		 if (DdiImplySignatures(iSigCompl, jSig, careSigPtr)) { 
		    /* !i => j, that is, i + j */
		    pos = lemmaImp[sign][0][i]++;
		    lemmaImp[sign][i][pos] = currImp[sign][j];
		 } else {
#if 0
		    if (!trace && !combImp[i_sig][j_sig]) {
		       timeFrameLemmaClauses(timeFrameNodes, NULL, lemmaImpLits, 
				     assumeFrames, S, i_sig, 0, j_sig, 0, 1, 2);
		    }
#endif
		 }
		 /* implications with previously obtained classes */
		 for (k=splitStart[abs(currImp[sign][j])]; k<splitEnd[abs(currImp[sign][j])]; k++) {
		    lemmaClass = Ddi_BddarrayRead(lemmaClasses, k);
		    lemmaNode = Ddi_BddPartRead(lemmaClass, 0);
		    base_k = Ddi_BddToBaig(lemmaNode);
		    k_sig = bAig_AuxAig0(bmgr, base_k);
		    kSig = &nodeSigs->sArray[k_sig];
		    kSigCompl = &nodeSigsCompl->sArray[k_sig];
		    if ((currImp[sign][j]<0) ^ bAig_NodeIsInverted(base_k)) {
		       kSig = &nodeSigsCompl->sArray[k_sig];
		       kSigCompl = &nodeSigs->sArray[k_sig];
		    }
		    if (DdiImplySignatures(iSigCompl, kSig, careSigPtr)) { 
		       /* !i => k, that is, i + k */
		       pos = lemmaImp[sign][0][i]++;
		       lemmaImp[sign][i][pos] = currImp[sign][j]<0 ? -k : k;
		    }
		 }
		 /* implications for the newly obtained classes */
		 for (k=splitStart[i]; k<splitEnd[i]; k++) {
		    lemmaClass = Ddi_BddarrayRead(lemmaClasses, k);
		    lemmaNode = Ddi_BddPartRead(lemmaClass, 0);
		    base_k = Ddi_BddToBaig(lemmaNode);
		    k_sig = bAig_AuxAig0(bmgr, base_k);
		    kSig = &nodeSigs->sArray[k_sig];
		    kSigCompl = &nodeSigsCompl->sArray[k_sig];
		    if (sign ^ bAig_NodeIsInverted(base_k)) {
		       kSig = &nodeSigsCompl->sArray[k_sig];
		       kSigCompl = &nodeSigs->sArray[k_sig];
		    }
		    /* original implication */
		    if (DdiImplySignatures(jSigCompl, kSig, careSigPtr)) { 
		       /* !j => k, that is, j + k */
		       pos = lemmaImp[currImp[sign][j]<0][0][abs(currImp[sign][j])]++;
		       lemmaImp[currImp[sign][j]<0][abs(currImp[sign][j])][pos] = sign ? -k : k;
		    }
		    for (l=splitStart[abs(currImp[sign][j])]; l<splitEnd[abs(currImp[sign][j])]; l++) {
		       lemmaClass = Ddi_BddarrayRead(lemmaClasses, l);
		       lemmaNode = Ddi_BddPartRead(lemmaClass, 0);
		       base_l = Ddi_BddToBaig(lemmaNode);
		       l_sig = bAig_AuxAig0(bmgr, base_l);
		       lSig = &nodeSigs->sArray[l_sig];
		       lSigCompl = &nodeSigsCompl->sArray[l_sig];
		       if ((currImp[sign][j]<0) ^ bAig_NodeIsInverted(base_l)) {
			  lSig = &nodeSigsCompl->sArray[l_sig];
			  lSigCompl = &nodeSigs->sArray[l_sig];
		       }
		       if (DdiImplySignatures(lSigCompl, kSig, careSigPtr)) { 
			  /* !l => k, that is, l + k */
			  pos = lemmaImp[currImp[sign][j]<0][0][l]++;
			  lemmaImp[currImp[sign][j]<0][l][pos] = sign ? -k : k;
		       }
		    }	    
		 }
	      }
	   }

	   /* possibly add some "intra" implications */
	   if (1) {
	      iSig = &nodeSigs->sArray[i_sig];
	      iSigCompl = &nodeSigsCompl->sArray[i_sig];
	      if (bAig_NodeIsInverted(base_i)) {
		 iSig = &nodeSigsCompl->sArray[i_sig];
		 iSigCompl = &nodeSigs->sArray[i_sig];
	      }
	      for (k=splitStart[i]; k<splitEnd[i]; k++) {
		 lemmaClass = Ddi_BddarrayRead(lemmaClasses, k);
		 lemmaNode = Ddi_BddPartRead(lemmaClass, 0);
		 base_k = Ddi_BddToBaig(lemmaNode);
		 k_sig = bAig_AuxAig0(bmgr, base_k);
		 kSig = &nodeSigs->sArray[k_sig];
		 kSigCompl = &nodeSigsCompl->sArray[k_sig];
		 if (bAig_NodeIsInverted(base_k)) {
		    kSig = &nodeSigsCompl->sArray[k_sig];
		    kSigCompl = &nodeSigs->sArray[k_sig];
		 }
		 /* TWO possible (exclusive) implications */
		 if (DdiImplySignatures(iSig, kSig, careSigPtr)) { 
		    /* i => k, that is, !i + k */
		    pos = lemmaImp[1][0][i]++;
		    lemmaImp[1][i][pos] = k;
		 } else if (DdiImplySignatures(kSig, iSig, careSigPtr)) {
		    /* k => i, that is, i + !k */
		    pos = lemmaImp[0][0][i]++;
		    lemmaImp[0][i][pos] = -k;
		 } 
	      }
	   }
	}
	classesNum = Ddi_BddarrayNum(lemmaClasses);
     }
  } while (!end);
#if 0
  for (i=initNum; i<Ddi_BddarrayNum(lemmaClasses); i++) {
     lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
     if (Ddi_BddPartNum(lemmaClass) > 1) {
  	potentialLemmaClassClauses(lemmaClass, timeFrameNodes, 
  			      lemmaEqLits, assumeFrames, S, 0);
     }
  }
#endif
  if (lemmaImp) {
     Pdtutil_Free(splitStart);
     Pdtutil_Free(splitEnd);
     Pdtutil_Free(currImp[0]);
     Pdtutil_Free(currImp[1]);
  }
  DdiAigSignatureArrayFree(varSigs);
  DdiAigSignatureArrayFree(satSigs);
  DdiAigSignatureArrayFree(nextSigs);
  DdiAigSignatureArrayFree(nodeSigs);
  DdiAigSignatureArrayFree(nodeSigsCompl);
  bAigArrayFree(visitedNodes);
  Ddi_Free(lemmaNodes);
  /* remove constant node from classes */
  //Ddi_BddPartQuickRemove(Ddi_BddarrayRead(lemmaClasses, 0), 0);
  Ddi_BddPartRemove(Ddi_BddarrayRead(lemmaClasses, 0), 0);

  return result;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
lemmaSortClassAcc (
  Ddi_Bdd_t *eqClass
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(eqClass);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  int i, j=0, pos, min;
  Ddi_Bdd_t *node, *tmp;

  assert(Ddi_BddIsPartConj(eqClass));
  node = Ddi_BddPartRead(eqClass, 0);
  min = aigIndexRead(aigMgr, node);

  for (i=1; i<Ddi_BddPartNum(eqClass); i++) {
    node = Ddi_BddPartRead(eqClass, i);
    pos = aigIndexRead(aigMgr, node);
    if (pos < min) {
      min = pos;
      j = i;
    }
  }

  Ddi_BddPartSwap(eqClass, 0, j);
#if 0
  if (j != 0) {
    node = Ddi_BddDup(Ddi_BddPartRead(eqClass, j));
    tmp = Ddi_BddPartRead(eqClass, 0);
    Ddi_BddPartWrite(eqClass, j, tmp);
    Ddi_BddPartWrite(eqClass, 0, node);
    Ddi_Free(node);
  }
#endif
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bddarray_t *
Ddi_AigComputeInitialLemmaClasses (
  Ddi_Bddarray_t *lemmaNodes,
  Ddi_Bddarray_t *delta,
  Ddi_Vararray_t *psVars,
  Ddi_Bdd_t *initState,
  Ddi_Bddarray_t *initStub,
  Ddi_Bdd_t *spec,
  Ddi_Bdd_t *careBdd,
  short int ***lemmaImp,
  int sequential,
  int constrained,
  int do_skip,
  unsigned long simTimeLimit,
  unsigned long totalTimeLimit
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(delta);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  Ddi_Bddarray_t *careArray, *initBddNodes;
  Ddi_Bddarray_t *lemmasBase, *lemmaClasses, *careBddNodes;
  Ddi_AigSignatureArray_t *nodeSigs, *nodeSigsCompl, *varSigs, *nextSigs;
  Ddi_AigSignatureArray_t *initSigs, *initSigsCompl, *careSigs;
  int failed=0, simulDepth, maxSimulDepth=128, nVars = Ddi_MgrReadNumVars(ddm);
  bAig_array_t *visitedNodes=bAigArrayAlloc(), *careNodes=NULL, *initNodes=NULL;
  int noSplitDepth, maxNoSplitDepth=4, didSplit, nNodes, sign, impNum[2];
  int i, j, k, l, classesNum, initClassesNum, pos, careNum;
  short int *currImp[2], *tmpArray, *splitStart, *splitEnd;
  Ddi_Bdd_t *lemmaClass, *lemma, *node, *lemmaNode, *notSpec;
  Ddi_AigSignature_t careSig, *tmpSig, *lSig, *lSigCompl, *careSigPtr=NULL/*&careSig*/;
  Ddi_AigSignature_t *iSig, *iSigCompl, *jSig, *jSigCompl, *kSig, *kSigCompl;
  bAigEdge_t baig, base_i, base_j, base_k, base_l, base_n;
  Ddi_Bdd_t *class_i, *class_j, *class_k;
  int i_sig, j_sig, k_sig, l_sig, n_sig, hash_sig;
  st_table *sig_hash;
  st_generator *gen;

  lemmasBase = Ddi_BddarrayDup(lemmaNodes);
  nNodes = Ddi_BddarrayNum(lemmasBase);
  for (i=0; i<nNodes; i++) {
    node = Ddi_BddarrayRead(lemmasBase, i);
    assert(i ? !aigIsInv(node) : aigIsInv(node));
    bAigArrayWriteLast(visitedNodes, Ddi_BddToBaig(node));
  }
  if (lemmaImp) {
    memset(lemmaImp[0][0], 0, nNodes*sizeof(short int));	
    memset(lemmaImp[1][0], 0, nNodes*sizeof(short int));	
  }

  DdiSetSignatureConstant(&careSig, 1);
  if (careBdd) {
    careArray = Ddi_BddarrayAlloc(ddm, 1);
    Ddi_BddarrayWrite(careArray, 0, careBdd);
    careBddNodes = Ddi_AigarrayNodes(careArray, -1);
    Ddi_Free(careArray);
    careNum = Ddi_BddarrayNum(careBddNodes);
    assert(Ddi_BddEqual(Ddi_BddarrayRead(careBddNodes, careNum-1), careBdd));
    careNodes = bAigArrayAlloc();
    for (i=0; i<Ddi_BddarrayNum(careBddNodes); i++) {
      node = Ddi_BddarrayRead(careBddNodes, i);
      bAigArrayWriteLast(careNodes, Ddi_BddToBaig(node));
    }
    Ddi_Free(careBddNodes);
  }

  if (lemmaImp) {
    currImp[0] = Pdtutil_Alloc(short int, nNodes);
    currImp[1] = Pdtutil_Alloc(short int, nNodes);
    splitStart = Pdtutil_Alloc(short int, nNodes);
    splitEnd = Pdtutil_Alloc(short int, nNodes);
  }

  if (initStub != NULL) {
    initBddNodes = Ddi_AigarrayNodes(initStub, -1);
    initNodes = bAigArrayAlloc();
    for (i=0; i<Ddi_BddarrayNum(initBddNodes); i++) {
      node = Ddi_BddarrayRead(initBddNodes, i);
      bAigArrayWriteLast(initNodes, bAig_NonInvertedEdge(Ddi_BddToBaig(node)));
    }
    Ddi_Free(initBddNodes);
  }

  varSigs = DdiAigSignatureArrayAlloc(nVars);
  nextSigs = DdiAigSignatureArrayAlloc(Ddi_VararrayNum(psVars));
  DdiSetSignaturesRandom(varSigs, nVars);
  if (careBdd && constrained) {
    /* constrain signature in care */
    Ddi_Bdd_t *careStub;
    int num;
    if (initStub != NULL) {
      careStub = Ddi_BddCompose(careBdd, psVars, initStub);
    } else {
      careStub = Ddi_BddDup(careBdd);
    }
    num = DdiAigConstrainSignatures(NULL, careStub, varSigs, 5, NULL,
				    50*(1+Ddi_BddarrayNum(lemmaNodes)/1000));
    if (!num) {
      fprintf(dMgrO(ddm),"Signature constraining failed.\n");
    }
    Ddi_Free(careStub);
  }

  if (initState != NULL) {
    DdiUpdateVarSignatureAllBits(varSigs, initState);
  }
  if (initStub != NULL) {
    initSigs = DdiAigEvalSignature(ddm, initNodes, bAig_NULL, 0, varSigs);
    initSigsCompl = DdiAigSignatureArrayAlloc(initNodes->num);
    for (i=0; i<initNodes->num; i++) {
      baig = initNodes->nodes[i];
      Pdtutil_Assert(bAig_AuxInt(bmgr, baig) == -1, "wrong auxInt field");
      bAig_AuxInt(bmgr, baig) = i;
      initSigsCompl->sArray[i] = initSigs->sArray[i];
      DdiComplementSignature(&(initSigsCompl->sArray[i]));
    }

    for (i=0; i<Ddi_VararrayNum(psVars); i++) {
      base_n = Ddi_BddToBaig(Ddi_BddarrayRead(initStub, i));
      n_sig = bAig_AuxInt(bmgr, base_n);
      Pdtutil_Assert(n_sig != -1, "wrong auxInt field");
      if (bAig_NodeIsInverted(base_n)) {
	nextSigs->sArray[i] = initSigsCompl->sArray[n_sig]; 
      } else {
	nextSigs->sArray[i] = initSigs->sArray[n_sig];
      }
    }

    for (i=0; i<Ddi_VararrayNum(psVars); i++) {
      Ddi_Var_t *v = Ddi_VararrayRead(psVars, i); 
      int varId = Ddi_VarIndex(v);
      varSigs->sArray[varId] = nextSigs->sArray[i];
    }
    for (i=0; i<initNodes->num; i++) {
      baig = initNodes->nodes[i];
      bAig_AuxInt(bmgr, baig) = -1;
    }
    DdiAigSignatureArrayFree(initSigs);
    DdiAigSignatureArrayFree(initSigsCompl);
  }

  /* first simulation step: determine initial lemma classes */
  nodeSigs = DdiAigEvalSignature(ddm,visitedNodes,bAig_NULL,0, varSigs);
  nodeSigsCompl = DdiAigSignatureArrayAlloc(visitedNodes->num);
  if (careNodes) {
    careSigs = DdiAigEvalSignature(ddm,careNodes,bAig_NULL,0, varSigs);
    careSigPtr = &careSigs->sArray[careNum-1];
    if (aigIsInv(careBdd)) DdiComplementSignature(careSigPtr);
    /* debug code */
    if (1) {
      Ddi_AigSignature_t zero;
      DdiSetSignatureConstant(&zero, 0);
      if (DdiEqSignatures(careSigPtr, &zero, NULL)) {
	printf("ZERO care signature!\n");
      }
    }
  }
  for (i=0; i<visitedNodes->num; i++) {
    nodeSigsCompl->sArray[i] = nodeSigs->sArray[i];
    DdiComplementSignature(&(nodeSigsCompl->sArray[i]));
  }

  lemmaClasses = Ddi_BddarrayAlloc(ddm, 0);
#if 1
  lemmaClass = Ddi_BddMakePartConjVoid(ddm);
  Ddi_BddarrayInsertLast(lemmaClasses, lemmaClass);
  Ddi_Free(lemmaClass);
  sig_hash = st_init_table(st_numcmp, st_numhash);
  for (i=0; i<nNodes; i++) {
    lemmaNode = Ddi_BddarrayRead(lemmasBase,i);
    lemma_insert_hash(sig_hash, lemmaNode, nodeSigs, careSigPtr);
  }
  //printf("Initial lemma classes: %d\n", st_count(sig_hash));
  st_foreach_item(sig_hash, gen, &hash_sig, &class_i) {
    base_i = Ddi_BddToBaig(Ddi_BddPartRead(class_i, 0));
    i_sig = aigIndexRead(bmgr,Ddi_BddPartRead(class_i, 0));
    iSig = &nodeSigs->sArray[i_sig];
    if (bAig_NodeIsInverted(base_i)) {
      iSig = &nodeSigsCompl->sArray[i_sig];
    }
    classesNum = Ddi_BddarrayNum(lemmaClasses);
    for (j=Ddi_BddPartNum(class_i)-1; j>0; j--) {
      base_j = Ddi_BddToBaig(Ddi_BddPartRead(class_i,j));
      j_sig = aigIndexRead(bmgr,Ddi_BddPartRead(class_i,j));
      jSig = &nodeSigs->sArray[j_sig];
      if (bAig_NodeIsInverted(base_j)) {
	jSig = &nodeSigsCompl->sArray[j_sig];
      }
      if (!DdiEqSignatures(iSig,jSig,careSigPtr)) {
	lemma = Ddi_BddPartQuickExtract(class_i, j);
	for (k=classesNum; k<Ddi_BddarrayNum(lemmaClasses); k++) {
	  class_k = Ddi_BddarrayRead(lemmaClasses, k);
	  base_k = Ddi_BddToBaig(Ddi_BddPartRead(class_k, 0));
	  k_sig = aigIndexRead(bmgr, Ddi_BddPartRead(class_k, 0));
	  kSig = &nodeSigs->sArray[k_sig];
	  if (bAig_NodeIsInverted(base_k)) {
	    kSig = &nodeSigsCompl->sArray[k_sig];
	  }
	  if (DdiEqSignatures(jSig, kSig, careSigPtr)) {
	    Ddi_BddPartInsertLast(class_k, lemma);
	    break;
	  }
	}
	if (k == Ddi_BddarrayNum(lemmaClasses)) {
	  /* no equivalences: add new class */ 
	  Ddi_Bdd_t *newClass = Ddi_BddMakePartConjVoid(ddm);
	  Ddi_BddPartInsertLast(newClass, lemma);
	  Ddi_BddarrayInsertLast(lemmaClasses, newClass);
	  Ddi_Free(newClass);
	  didSplit = 1;
	}
	Ddi_Free(lemma);
      }
    }
    if (i_sig == 0) {
      Ddi_BddarrayWrite(lemmaClasses, 0, class_i);
    } else {
      Ddi_BddarrayInsertLast(lemmaClasses, class_i);
    }
    Ddi_Free(class_i);
  }
#if 0
  for (i=0; i<Ddi_BddarrayNum(lemmaClasses); i++) {
    lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
    for (j=0; j<Ddi_BddPartNum(lemmaClass); j++) {
      lemmaNode = Ddi_BddPartRead(lemmaClass, j);
      assert(bAig_AuxInt(bmgr, Ddi_BddToBaig(lemmaNode)) == -1);
      bAig_AuxInt(bmgr, Ddi_BddToBaig(lemmaNode)) = 1;
    }
  }
  for (i=0; i<nNodes; i++) {
    lemmaNode = Ddi_BddarrayRead(lemmasBase, i);
    assert(bAig_AuxInt(bmgr, Ddi_BddToBaig(lemmaNode)) == 1);
    bAig_AuxInt(bmgr, Ddi_BddToBaig(lemmaNode)) = -1;
  }
#endif
  st_free_table(sig_hash);
#else
  for (i=0; i<nNodes; i++) {
    if (util_cpu_time() > totalTimeLimit) {
      break;
    }
    lemmaNode = Ddi_BddarrayRead(lemmasBase,i);
    if (0 && do_skip && i && (aigIndexRead(bmgr, lemmaNode)!=nNodes)) {
      continue; /* skip already proved nodes */
    }
    i_sig = aigIndexRead(bmgr, lemmaNode);
    iSig = &nodeSigs->sArray[i_sig];
    iSigCompl = &nodeSigsCompl->sArray[i_sig];
    base_i = Ddi_BddToBaig(lemmaNode);
    assert(i ? !bAig_NodeIsInverted(base_i) : bAig_NodeIsInverted(base_i));

    for (j=0; j<Ddi_BddarrayNum(lemmaClasses); j++) {
      class_j = Ddi_BddarrayRead(lemmaClasses, j);
      base_j = Ddi_BddToBaig(Ddi_BddPartRead(class_j, 0));
      j_sig = aigIndexRead(bmgr,Ddi_BddPartRead(class_j, 0));
      jSig = &nodeSigs->sArray[j_sig];
      if (bAig_NodeIsInverted(base_j)) {
	jSig = &nodeSigsCompl->sArray[j_sig];
      }
      if (DdiEqSignatures(iSig,jSig,careSigPtr)) {
	lemma = Ddi_BddarrayRead(lemmasBase,i);
	Ddi_BddPartInsertLast(class_j, lemma);
	break;
      }
      if (DdiEqSignatures(iSigCompl,jSig,careSigPtr)) {
	lemma = Ddi_BddNot(Ddi_BddarrayRead(lemmasBase,i));
	Ddi_BddPartInsertLast(class_j, lemma);
	Ddi_Free(lemma);
	break;
      }
    }
    if (j == Ddi_BddarrayNum(lemmaClasses)) {
      /* no equivalences: add new class */ 
      lemmaClass = Ddi_BddMakePartConjVoid(ddm);
      Ddi_BddPartInsertLast(lemmaClass, lemmaNode);
      Ddi_BddarrayInsertLast(lemmaClasses, lemmaClass);
      Ddi_Free(lemmaClass);
      /* manage implications involving the new class */
      if (lemmaImp) {
	for (k=1; k<j; k++) {
	  lemmaClass = Ddi_BddarrayRead(lemmaClasses, k);
	  lemmaNode = Ddi_BddPartRead(lemmaClass, 0);
	  base_k = Ddi_BddToBaig(lemmaNode);
	  k_sig = aigIndexRead(bmgr, lemmaNode);
	  kSig = &nodeSigs->sArray[k_sig];
	  kSigCompl = &nodeSigsCompl->sArray[k_sig];
	  if (bAig_NodeIsInverted(base_k)) {
	    kSig = &nodeSigsCompl->sArray[k_sig];
	    kSigCompl = &nodeSigs->sArray[k_sig];
	  }
	  /* four possible (exclusive) implications */
	  if (implySignatures(kSigCompl,iSig,careSigPtr)) { 
	    /* !k => i, that is, k + i */
	    pos = lemmaImp[0][0][k]++;
	    lemmaImp[0][k][pos] = j;
	  } else if (implySignatures(kSig,iSig,careSigPtr)) {
	    /* k => i, that is, !k + i */
	    pos = lemmaImp[1][0][k]++;
	    lemmaImp[1][k][pos] = j;
	  } else if (implySignatures(kSigCompl,iSigCompl,careSigPtr)) {
	    /* !k => !i, that is, k + !i */
	    pos = lemmaImp[0][0][k]++;
	    lemmaImp[0][k][pos] = -j;
	  } else if (implySignatures(kSig,iSigCompl,careSigPtr)) {
	    /* k => !i, that is, !k + !i */
	    pos = lemmaImp[1][0][k]++;
	    lemmaImp[1][k][pos] = -j;
	  }
	}
      }
    }
  }
#endif
  if (spec != NULL && (util_cpu_time() < totalTimeLimit) && (initState!=NULL || initStub!=NULL)) {
    lemmaClass = Ddi_BddarrayRead(lemmaClasses, 0);
    for (i=0; i<Ddi_BddPartNum(lemmaClass); i++) {
      lemma = Ddi_BddPartRead(lemmaClass, i);
      if (Ddi_BddEqual(lemma, spec))
	break;
    }
    if (i == Ddi_BddPartNum(lemmaClass)) {
      failed = 1;
    }
  }

#if 0
  /* output classes (debugging) */
  fprintf(dMgrO(ddm),"Classes (0: %d/%d): ", Ddi_BddarrayNum(lemmaClasses), nNodes);
  for (i=0; i<Ddi_BddarrayNum(lemmaClasses); i++) {
    lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
    fprintf(dMgrO(ddm),"%d ", Ddi_BddPartNum(lemmaClass));
  }
  fprintf(dMgrO(ddm),"\n");
#endif

  /* further random simulation: refine classes */
  simulDepth = noSplitDepth = 0; 
  while (noSplitDepth<maxNoSplitDepth && simulDepth<maxSimulDepth && !failed) {
    if (util_cpu_time() > simTimeLimit) {
      break;
    }
    simulDepth++; 
    didSplit = 0;
    if (lemmaImp) {
      memset(splitStart, 0, nNodes*sizeof(short int));	
      memset(splitEnd, 0, nNodes*sizeof(short int));
    }
    /* generate new random sigs, take care of present states */
    if (sequential) { //(1 || initState != NULL) {
#if 1
      /* remove zeros from care signature... */
      for (k=0; k<DDI_AIG_SIGNATURE_BITS && 
	     DdiGetSignatureBit(careSigPtr, k)==0; k++) ; 
      if (k == DDI_AIG_SIGNATURE_BITS) {
	/* care is a constant zero: abort simulation (should not happen...) */
	break;
      }
      for (i=0; i<DDI_AIG_SIGNATURE_BITS; i++) {
	if (DdiGetSignatureBit(careSigPtr, i)==0) {
	  for (j=0; j<Ddi_VararrayNum(psVars); j++) {
	    base_n = Ddi_BddToBaig(Ddi_BddarrayRead(delta,j));
	    n_sig = aigIndexRead(bmgr, Ddi_BddarrayRead(delta,j));
	    DdiSetSignatureBit(&nodeSigs->sArray[n_sig], i, 
		      DdiGetSignatureBit(&nodeSigs->sArray[n_sig], k));
	    DdiSetSignatureBit(&nodeSigsCompl->sArray[n_sig], i, 
		      DdiGetSignatureBit(&nodeSigsCompl->sArray[n_sig], k));
	  }
	}
      }
#endif
      for (i=0; i<Ddi_VararrayNum(psVars); i++) {
	base_n = Ddi_BddToBaig(Ddi_BddarrayRead(delta,i));
	n_sig = aigIndexRead(bmgr, Ddi_BddarrayRead(delta,i));
#if 0
        printf("Baig [%d] -> %d - aig1: %d\n", i, 
               Ddi_BddToBaig(Ddi_BddarrayRead(delta,i)), n_sig);
#endif
      	Pdtutil_Assert(n_sig != -1, "wrong auxId field");
	if (n_sig==-1) {
	  int varId = Ddi_VarIndex(Ddi_VararrayRead(psVars,i));
	  nextSigs->sArray[i] = varSigs->sArray[varId];
	}
	else if (bAig_NodeIsInverted(base_n)) {
	  nextSigs->sArray[i] = nodeSigsCompl->sArray[n_sig];
	} else {
	  nextSigs->sArray[i] = nodeSigs->sArray[n_sig];
	}
      }
    }
    DdiSetSignaturesRandom(varSigs, nVars);
    if (sequential) { //(1 || initState != NULL) {
      for (i=0; i<Ddi_VararrayNum(psVars); i++) {
	Ddi_Var_t *v = Ddi_VararrayRead(psVars,i); 
	int varId = Ddi_VarIndex(v);
	varSigs->sArray[varId] = nextSigs->sArray[i];
      }
    } else {
      if (initState) {
	DdiUpdateVarSignatureAllBits(varSigs, initState);
      }
      if (initStub) {
	initSigs = DdiAigEvalSignature(ddm, initNodes, bAig_NULL, 0, varSigs);
	initSigsCompl = DdiAigSignatureArrayAlloc(initNodes->num);
	for (i=0; i<initNodes->num; i++) {
	  baig = initNodes->nodes[i];
	  Pdtutil_Assert(bAig_AuxInt(bmgr, baig) == -1, "wrong auxInt field");
	  bAig_AuxInt(bmgr, baig) = i;
	  initSigsCompl->sArray[i] = initSigs->sArray[i];
	  DdiComplementSignature(&(initSigsCompl->sArray[i]));
	}

	for (i=0; i<Ddi_VararrayNum(psVars); i++) {
	  base_n = Ddi_BddToBaig(Ddi_BddarrayRead(initStub, i));
	  n_sig = bAig_AuxInt(bmgr, base_n);
	  Pdtutil_Assert(n_sig != -1, "wrong auxInt field");
	  if (bAig_NodeIsInverted(base_n)) {
	    nextSigs->sArray[i] = initSigsCompl->sArray[n_sig]; 
	  } else {
	    nextSigs->sArray[i] = initSigs->sArray[n_sig];
	  }
	}

	for (i=0; i<Ddi_VararrayNum(psVars); i++) {
	  Ddi_Var_t *v = Ddi_VararrayRead(psVars, i); 
	  int varId = Ddi_VarIndex(v);
	  varSigs->sArray[varId] = nextSigs->sArray[i];
	}
	for (i=0; i<initNodes->num; i++) {
	  baig = initNodes->nodes[i];
	  bAig_AuxInt(bmgr, baig) = -1;
	}
	DdiAigSignatureArrayFree(initSigs);
	DdiAigSignatureArrayFree(initSigsCompl);
      }
    }

    /* simulate */
    DdiAigSignatureArrayFree(nodeSigs);
    nodeSigs = DdiAigEvalSignature(ddm,visitedNodes,bAig_NULL,0,varSigs);
    if (careNodes) {
      DdiAigSignatureArrayFree(careSigs);
      careSigs = DdiAigEvalSignature(ddm,careNodes,bAig_NULL,0,varSigs);
      careSigPtr = &careSigs->sArray[careNum-1];
      if (aigIsInv(careBdd)) DdiComplementSignature(careSigPtr);
    }
    for (i=0; i<visitedNodes->num; i++) {
      nodeSigsCompl->sArray[i] = nodeSigs->sArray[i];
      DdiComplementSignature(&(nodeSigsCompl->sArray[i]));
    }

    /* refine classes */
    classesNum = initClassesNum = Ddi_BddarrayNum(lemmaClasses);
    for (i=initClassesNum-1; i>=0; i--) {
      class_i = Ddi_BddarrayRead(lemmaClasses, i);
      base_i = Ddi_BddToBaig(Ddi_BddPartRead(class_i, 0));
      i_sig = aigIndexRead(bmgr,Ddi_BddPartRead(class_i, 0));
      iSig = &nodeSigs->sArray[i_sig];
      iSigCompl = &nodeSigsCompl->sArray[i_sig];
      if (bAig_NodeIsInverted(base_i)) {
	iSig = &nodeSigsCompl->sArray[i_sig];
	iSigCompl = &nodeSigs->sArray[i_sig];
      }
      /* equivalences */
      for (j=Ddi_BddPartNum(class_i)-1; j>0; j--) {
	base_j = Ddi_BddToBaig(Ddi_BddPartRead(class_i,j));
	j_sig = aigIndexRead(bmgr,Ddi_BddPartRead(class_i,j));
	jSig = &nodeSigs->sArray[j_sig];
	jSigCompl = &nodeSigsCompl->sArray[j_sig];
	if (bAig_NodeIsInverted(base_j)) {
	  jSig = &nodeSigsCompl->sArray[j_sig];
	  jSigCompl = &nodeSigs->sArray[j_sig];
	}

	if (!DdiEqSignatures(iSig,jSig,careSigPtr)) {
	  lemma = Ddi_BddPartQuickExtract(class_i, j);
	  if ((initState || initStub) && spec && Ddi_BddEqual(lemma, spec)) {
	    failed = 1;
	    Ddi_Free(lemma);
	    break;
	  }
	  for (k=classesNum; k<Ddi_BddarrayNum(lemmaClasses); k++) {
	    class_k = Ddi_BddarrayRead(lemmaClasses, k);
	    base_k = Ddi_BddToBaig(Ddi_BddPartRead(class_k, 0));
	    k_sig = aigIndexRead(bmgr, Ddi_BddPartRead(class_k, 0));
	    kSig = &nodeSigs->sArray[k_sig];
	    if (bAig_NodeIsInverted(base_k)) {
	      kSig = &nodeSigsCompl->sArray[k_sig];
	    }
	    if (DdiEqSignatures(jSig, kSig, careSigPtr)) {
	      Ddi_BddPartInsertLast(class_k, lemma);
	      break;
	    }
	  }
	  if (k == Ddi_BddarrayNum(lemmaClasses)) {
	    /* no equivalences: add new class */ 
	    Ddi_Bdd_t *newClass = Ddi_BddMakePartConjVoid(ddm);
	    Ddi_BddPartInsertLast(newClass, lemma);
	    Ddi_BddarrayInsertLast(lemmaClasses, newClass);
	    Ddi_Free(newClass);
	    didSplit = 1;
	  }
	  Ddi_Free(lemma);
	}
      }
      if (failed) break;
      if (lemmaImp && classesNum < Ddi_BddarrayNum(lemmaClasses)) {
	splitStart[i] = classesNum;
	splitEnd[i] = Ddi_BddarrayNum(lemmaClasses);
      }

      /* implications */
      if (lemmaImp) {
	if (i == 0) {
	  /* generate only new "inter" implications */
	  for (k=splitStart[i]; k<splitEnd[i]; k++) {
	    lemmaClass = Ddi_BddarrayRead(lemmaClasses, k);
	    lemmaNode = Ddi_BddPartRead(lemmaClass, 0);
	    base_k = Ddi_BddToBaig(lemmaNode);
	    k_sig = bAig_AuxAig0(bmgr, base_k);
	    kSig = &nodeSigs->sArray[k_sig];
	    kSigCompl = &nodeSigsCompl->sArray[k_sig];
	    if (bAig_NodeIsInverted(base_k)) {
	      kSig = &nodeSigsCompl->sArray[k_sig];
	      kSigCompl = &nodeSigs->sArray[k_sig];
	    }
	    for (l=1; l<k; l++) {
	      lemmaClass = Ddi_BddarrayRead(lemmaClasses, l);
	      lemmaNode = Ddi_BddPartRead(lemmaClass, 0);
	      base_l = Ddi_BddToBaig(lemmaNode);
	      l_sig = bAig_AuxAig0(bmgr, base_l);
	      lSig = &nodeSigs->sArray[l_sig];
	      lSigCompl = &nodeSigsCompl->sArray[l_sig];
	      if (bAig_NodeIsInverted(base_l)) {
		lSig = &nodeSigsCompl->sArray[l_sig];
		lSigCompl = &nodeSigs->sArray[l_sig];
	      }
	      /* THREE exclusive implications */
	      if (DdiImplySignatures(lSigCompl, kSig, careSigPtr)) { 
		/* !l => k, that is, l + k */
		pos = lemmaImp[0][0][l]++;
		lemmaImp[0][l][pos] = k;
	      } else if (DdiImplySignatures(lSig, kSig, careSigPtr)) { 
		/* l => k, that is, !l + k */
		pos = lemmaImp[1][0][l]++;
		lemmaImp[1][l][pos] = k;
	      } else if (l >= splitStart[0]) {
		if (DdiImplySignatures(lSigCompl, kSigCompl, careSigPtr)) { 
		  /* !l => !k, that is, l + !k */
		  pos = lemmaImp[0][0][l]++;
		  lemmaImp[0][l][pos] = -k;
		} else if (0&&DdiImplySignatures(lSig, kSigCompl, careSigPtr)) { 
		  /* l => !k, that is, !l + !k */
		  pos = lemmaImp[1][0][l]++;
		  lemmaImp[1][l][pos] = -k;
		}
	      }
	    }
	  }
	  continue;
	}

	/* manage "inter" implications */
	for (sign=0; sign<2; sign++) {
	  impNum[sign] = lemmaImp[sign][0][i];
	  lemmaImp[sign][0][i] = 0;
	  tmpArray = lemmaImp[sign][i];
	  lemmaImp[sign][i] = currImp[sign];
	  currImp[sign] = tmpArray;
	}
	for (sign=0; sign<2; sign++) {
	  if (sign) {
	    tmpSig = iSig;
	    iSig = iSigCompl;
	    iSigCompl = tmpSig;
	  }
	  for (j=0; j<impNum[sign]; j++) {
	    lemmaClass = Ddi_BddarrayRead(lemmaClasses, abs(currImp[sign][j]));
	    lemmaNode = Ddi_BddPartRead(lemmaClass, 0);
	    base_j = Ddi_BddToBaig(lemmaNode);
	    j_sig = bAig_AuxAig0(bmgr, base_j);
	    jSig = &nodeSigs->sArray[j_sig];
	    jSigCompl = &nodeSigsCompl->sArray[j_sig];
	    if ((currImp[sign][j]<0) ^ bAig_NodeIsInverted(base_j)) {
	      jSig = &nodeSigsCompl->sArray[j_sig];
	      jSigCompl = &nodeSigs->sArray[j_sig];
	    }
	    /* check original implication */
	    if (DdiImplySignatures(iSigCompl, jSig, careSigPtr)) { 
	      /* !i => j, that is, i + j */
	      pos = lemmaImp[sign][0][i]++;
	      lemmaImp[sign][i][pos] = currImp[sign][j];
	    } else {
	      didSplit = 1;
	    }
	    /* implications with previously obtained classes */
	    for (k=splitStart[abs(currImp[sign][j])]; k<splitEnd[abs(currImp[sign][j])]; k++) {
	      lemmaClass = Ddi_BddarrayRead(lemmaClasses, k);
	      lemmaNode = Ddi_BddPartRead(lemmaClass, 0);
	      base_k = Ddi_BddToBaig(lemmaNode);
	      k_sig = bAig_AuxAig0(bmgr, base_k);
	      kSig = &nodeSigs->sArray[k_sig];
	      kSigCompl = &nodeSigsCompl->sArray[k_sig];
	      if ((currImp[sign][j]<0) ^ bAig_NodeIsInverted(base_k)) {
		kSig = &nodeSigsCompl->sArray[k_sig];
		kSigCompl = &nodeSigs->sArray[k_sig];
	      }
	      if (DdiImplySignatures(iSigCompl, kSig, careSigPtr)) { 
		/* !i => k, that is, i + k */
		pos = lemmaImp[sign][0][i]++;
		lemmaImp[sign][i][pos] = currImp[sign][j]<0 ? -k : k;
	      }
	    }
	    /* implications for the newly obtained classes */
	    for (k=splitStart[i]; k<splitEnd[i]; k++) {
	      lemmaClass = Ddi_BddarrayRead(lemmaClasses, k);
	      lemmaNode = Ddi_BddPartRead(lemmaClass, 0);
	      base_k = Ddi_BddToBaig(lemmaNode);
	      //k_sig = bAig_AuxInt(bmgr, base_k);
	      k_sig = bAig_AuxAig0(bmgr, base_k);
	      kSig = &nodeSigs->sArray[k_sig];
	      kSigCompl = &nodeSigsCompl->sArray[k_sig];
	      if (sign ^ bAig_NodeIsInverted(base_k)) {
		kSig = &nodeSigsCompl->sArray[k_sig];
		kSigCompl = &nodeSigs->sArray[k_sig];
	      }
	      /* original implication */
	      if (DdiImplySignatures(jSigCompl, kSig, careSigPtr)) { 
		/* !j => k, that is, j + k */
		pos = lemmaImp[currImp[sign][j]<0][0][abs(currImp[sign][j])]++;
		lemmaImp[currImp[sign][j]<0][abs(currImp[sign][j])][pos] = sign ? -k : k;
	      }
	      for (l=splitStart[abs(currImp[sign][j])]; l<splitEnd[abs(currImp[sign][j])]; l++) {
		lemmaClass = Ddi_BddarrayRead(lemmaClasses, l);
		lemmaNode = Ddi_BddPartRead(lemmaClass, 0);
		base_l = Ddi_BddToBaig(lemmaNode);
		l_sig = bAig_AuxAig0(bmgr, base_l);
		lSig = &nodeSigs->sArray[l_sig];
		lSigCompl = &nodeSigsCompl->sArray[l_sig];
		if ((currImp[sign][j]<0) ^ bAig_NodeIsInverted(base_l)) {
		  lSig = &nodeSigsCompl->sArray[l_sig];
		  lSigCompl = &nodeSigs->sArray[l_sig];
		}
		if (DdiImplySignatures(lSigCompl, kSig, careSigPtr)) { 
		  /* !l => k, that is, l + k */
		  pos = lemmaImp[currImp[sign][j]<0][0][l]++;
		  lemmaImp[currImp[sign][j]<0][l][pos] = sign ? -k : k;
		}
	      }	    
	    }
	  }
	} /* end for sign */
	/* possibly add some "intra" implications */
	if (1) {
	  iSig = &nodeSigs->sArray[i_sig];
	  iSigCompl = &nodeSigsCompl->sArray[i_sig];
	  if (bAig_NodeIsInverted(base_i)) {
	    iSig = &nodeSigsCompl->sArray[i_sig];
	    iSigCompl = &nodeSigs->sArray[i_sig];
	  }
	  for (k=splitStart[i]; k<splitEnd[i]; k++) {
	    lemmaClass = Ddi_BddarrayRead(lemmaClasses, k);
	    lemmaNode = Ddi_BddPartRead(lemmaClass, 0);
	    base_k = Ddi_BddToBaig(lemmaNode);
	    k_sig = bAig_AuxAig0(bmgr, base_k);
	    kSig = &nodeSigs->sArray[k_sig];
	    kSigCompl = &nodeSigsCompl->sArray[k_sig];
	    if (bAig_NodeIsInverted(base_k)) {
	      kSig = &nodeSigsCompl->sArray[k_sig];
	      kSigCompl = &nodeSigs->sArray[k_sig];
	    }
	    /* implications with the original class */
	    /* TWO possible (exclusive) implications */
	    if (DdiImplySignatures(iSig, kSig, careSigPtr)) { 
	      /* i => k, that is, !i + k */
	      pos = lemmaImp[1][0][i]++;
	      lemmaImp[1][i][pos] = k;
	    } else if (DdiImplySignatures(kSig, iSig, careSigPtr)) {
	      /* k => i, that is, i + !k */
	      pos = lemmaImp[0][0][i]++;
	      lemmaImp[0][i][pos] = -k;
	    }
	    /* implications among the newly obtained classes */
	    for (l=splitStart[i]; l<k; l++) {
	      lemmaClass = Ddi_BddarrayRead(lemmaClasses, l);
	      lemmaNode = Ddi_BddPartRead(lemmaClass, 0);
	      base_l = Ddi_BddToBaig(lemmaNode);
	      l_sig = bAig_AuxAig0(bmgr, base_l);
	      lSig = &nodeSigs->sArray[l_sig];
	      lSigCompl = &nodeSigsCompl->sArray[l_sig];
	      if (bAig_NodeIsInverted(base_l)) {
		lSig = &nodeSigsCompl->sArray[l_sig];
		lSigCompl = &nodeSigs->sArray[l_sig];
	      }
	      /* TWO possible (exclusive) implications */
	      if (DdiImplySignatures(lSig, kSig, careSigPtr)) { 
		/* l => k, that is, !l + k */
		pos = lemmaImp[1][0][l]++;
		lemmaImp[1][l][pos] = k;
	      } else if (DdiImplySignatures(kSig, lSig, careSigPtr)) {
		/* k => l, that is, l + !k */
		pos = lemmaImp[0][0][l]++;
		lemmaImp[0][l][pos] = -k;
	      }
	    }
	  }
	}
      } /* end if lemmaImp */
      classesNum = Ddi_BddarrayNum(lemmaClasses);
      if (failed) {
	break;
      }
    } /* end for i in classes */

    if (failed) {
      break;
    }
    if (didSplit == 0) {
      noSplitDepth++;
    } else {
      noSplitDepth = 0;
    }
#if 0
    /* output classes (debugging) */
    fprintf(dMgrO(ddm),"Classes (%d: %d/%d): ", simulDepth, Ddi_BddarrayNum(lemmaClasses), nNodes);
    for (i=0; i<Ddi_BddarrayNum(lemmaClasses); i++) {
      lemmaClass = Ddi_BddarrayRead(lemmaClasses, i);
      fprintf(dMgrO(ddm),"%d ", Ddi_BddPartNum(lemmaClass));
    }
    fprintf(dMgrO(ddm),"\n");
#endif
  }

 simulEnd:
  if (lemmaImp) {
    Pdtutil_Free(splitStart);
    Pdtutil_Free(splitEnd);
    Pdtutil_Free(currImp[0]);
    Pdtutil_Free(currImp[1]);
  }
  DdiAigSignatureArrayFree(varSigs);
  DdiAigSignatureArrayFree(nextSigs);
  DdiAigSignatureArrayFree(nodeSigs);
  DdiAigSignatureArrayFree(nodeSigsCompl);
  bAigArrayFree(visitedNodes);
  Ddi_Free(lemmasBase);
  if (careNodes) {
    DdiAigSignatureArrayFree(careSigs);
    bAigArrayFree(careNodes);
  }
  if (initNodes) {
    bAigArrayFree(initNodes);
  }

  if (failed) {
    Ddi_Free(lemmaClasses);
    return NULL;
  }
  /* remove constant node from classes */
  lemmaNode = Ddi_BddPartRead(Ddi_BddarrayRead(lemmaClasses, 0), 0);
  assert(Ddi_BddIsOne(lemmaNode));
  Ddi_BddPartQuickRemove(Ddi_BddarrayRead(lemmaClasses, 0), 0);

  return lemmaClasses;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
lemma_insert_hash (
  st_table *sig_hash,
  Ddi_Bdd_t *node,
  Ddi_AigSignatureArray_t *nodeSigs,
  Ddi_AigSignature_t *careSig
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(node);
  bAig_Manager_t *aigMgr = ddm->aig.mgr;
  Ddi_AigSignature_t *nodeSig;
  Ddi_Bdd_t *lemmaClass, *lemma;
  unsigned hashSig;
  int i, node_sig;
  //bAigEdge_t baig;

  //baig = Ddi_BddToBaig(node);
  node_sig = aigIndexRead(aigMgr, node);
  nodeSig = &nodeSigs->sArray[node_sig];

  hashSig = (~nodeSig->s[0]) & careSig->s[0];
  if (st_lookup(sig_hash, (void *)hashSig, &lemmaClass)) {
    lemma = Ddi_BddNot(node);
    Ddi_BddPartInsertLast(lemmaClass, lemma);
    Ddi_Free(lemma);
    //for (i=0; i<Ddi_BddPartNum(lemmaClass); i++) {
    //  lemma = Ddi_BddPartRead(lemmaClass, i);
    //  assert(lemma->common.status == Ddi_Locked_c);
    //}
    //if (baig == 35948) {
    //  globalClass = lemmaClass;
    //  globalNode = Ddi_BddPartRead(lemmaClass, Ddi_BddPartNum(lemmaClass)-1);
    //}
    return;
  }

  hashSig = nodeSig->s[0] & careSig->s[0];
  if (st_lookup(sig_hash, (void *)hashSig, &lemmaClass)) {
    Ddi_BddPartInsertLast(lemmaClass, node);
    //for (i=0; i<Ddi_BddPartNum(lemmaClass); i++) {
    //  lemma = Ddi_BddPartRead(lemmaClass, i);
    //  assert(lemma->common.status == Ddi_Locked_c);
    //}
    //if (baig == 35948) {
    //  globalClass = lemmaClass;
    //  globalNode = Ddi_BddPartRead(lemmaClass, Ddi_BddPartNum(lemmaClass)-1);
    //}
    return;
  }

  if (bAig_NodeIsInverted(Ddi_BddToBaig(node))) {
    hashSig = (~nodeSig->s[0]) & careSig->s[0];
  }
  lemmaClass = Ddi_BddMakePartConjVoid(ddm);
  Ddi_BddPartInsertLast(lemmaClass, node);
  //for (i=0; i<Ddi_BddPartNum(lemmaClass); i++) {
  //  lemma = Ddi_BddPartRead(lemmaClass, i);
  //  assert(lemma->common.status == Ddi_Locked_c);
  //}
  st_insert(sig_hash, (void *)hashSig, (void *)lemmaClass);
  //if (baig == 35948) {
  //  globalClass = lemmaClass;
  //  globalNode = Ddi_BddPartRead(lemmaClass, Ddi_BddPartNum(lemmaClass)-1);
  //}
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Vararray_t *
newTimeFrameVars (
  Ddi_Vararray_t *baseVars, 
  char *nameSuffix
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(baseVars);
  int nVars = Ddi_VararrayNum(baseVars);
  Ddi_Vararray_t *newVars = Ddi_VararrayAlloc(ddm,nVars);
  int useAigVars=1, j;

  for (j=0; j<Ddi_VararrayNum(baseVars); j++) {
    char name[1000];
    Ddi_Var_t *newv, *v = Ddi_VararrayRead(baseVars,j);
    sprintf(name,"PDT_LEMMAS_%s_%s", Ddi_VarName(v),nameSuffix);
    newv = Ddi_VarFromName(ddm, name);
    if (newv == NULL) {
      if (useAigVars) {
	newv = Ddi_VarNewBaig(ddm, name);
      } else {
	newv = Ddi_VarNewBeforeVar(v);
	Ddi_VarAttachName(newv, name);
      } 
    }
    Ddi_VararrayWrite(newVars, j, newv);
  }
  return (newVars);
}




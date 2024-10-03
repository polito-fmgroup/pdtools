/**CFile***********************************************************************

  FileName    [travBmc.c]

  PackageName [trav]

  Synopsis    [Aig/SAT based verif/traversal routines]

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
#include "fbv.h"
#include <pthread.h>

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
/*--------------------------------------------------------------------------s-*/

//int TravAigPartial = 0;

//Ddi_Var_t *constraintGuard=NULL;
//Ddi_Var_t *constraintGuard2=NULL;

#define USE_RWLOCK 1

struct bmcSharedStats {
#if USE_RWLOCK
  pthread_rwlock_t lock;
#else
  pthread_mutex_t lock;
#endif
  int active;
  int nActive;
  int nBounds;
  int boundsSize;
  Trav_BmcBoundState_e *bounds;
  int maxSafe;
} bmcShared;


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#define tMgrO(travMgr) ((travMgr)->settings.stdout)
#define dMgrO(ddiMgr) ((ddiMgr)->settings.stdout)

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static Ddi_Bdd_t *bmcCoiFilter(
  Ddi_Bdd_t * f,
  Ddi_Bdd_t * filter,
  Ddi_Bdd_t * care,
  Ddi_Vararray_t * cutVars
);
static int bmcSatAndWithAbort(
  Ddi_Bdd_t * a,
  Ddi_Bdd_t * b,
  Ddi_Bddarray_t * pre,
  Ddi_Bdd_t * care,
  Ddi_Vararray_t * cutVars,
  Ddi_Varset_t * cexVars,
  int satPart,
  int timeLimit
);
static Ddi_Vararray_t *composeWithNewTimeFrameVars(
  Ddi_Bdd_t * f,
  Ddi_Vararray_t * ps,
  Ddi_Bddarray_t * delta,
  Ddi_Vararray_t * pi,
  char *varNamePrefix,
  int frameSuffix
);
static Ddi_Bddarray_t *
unrollWithNewTimeFrameVars(
  Ddi_Bddarray_t * delta,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * pi,
  char *varNamePrefix,
  Ddi_Varsetarray_t *piSets,
  int nFrames
);
static Ddi_Bdd_t *bmcTfSplitCex(
  TravBmcMgr_t * bmcMgr,
  Ddi_Bdd_t * cex,
  int maxTf
);
static int
bmcOutOfLimits(
  TravBmcMgr_t  *bmcMgr
);
static int
travBmcSharedSetLocked(
  int k,
  Trav_BmcBoundState_e state
);
static Ddi_Bdd_t *
bmcCheckTargetByRefinement(
  TravBmcMgr_t *bmcMgr,
  Ddi_IncrSatMgr_t *ddiS,
  int bmcStep,
  Ddi_Bdd_t *checkPart,
  Ddi_Vararray_t *cutVars,
  Ddi_Vararray_t *cutAuxVars,
  float satTimeLimit,
  int *pabort
);
static void
timeFrameShiftKAcc(
  Ddi_Bdd_t * f,
  Ddi_Varsetarray_t *timeFrameVars,
  int k
);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Trav_TravSatBmcVerif(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
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
)
{
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Varset_t *totPiVars;
  Ddi_Bddarray_t *delta;
  Ddi_Bdd_t *reached, *target, *myInit, *myInvarspec;   //, *to;
  Ddi_Bdd_t *cubeInit;
  Ddi_Bddarray_t *unroll;
  int sat = 0, i, initIsCube, nsat;
  long startTime, bmcTime = 0, bmcStartTime, bmcStepTime;
  Ddi_Bdd_t *constrain = NULL;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);
  int provedBound = 0, inverse = 0;
  int doOptThreshold = 5000;
  int optLevel = 0;
  Ddi_Varset_t *psVars = NULL;
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  unsigned long time_limit = ~0;
  Trav_Stats_t *travStats = &(travMgr->xchg.stats);

  if (first < 0) {
    return Trav_TravSatBmcVerif2(travMgr, fsmMgr, init, invarspec, invar,
      bound, step, abs(first + 1), doApprox, lemmaSteps,
      interpolantSteps, timeLimit);
  }

  startTime = util_cpu_time();
  if (timeLimit > 0) {
    time_limit = util_cpu_time() + timeLimit * 1000;
  }
  if (bound < 0) {
    bound = 1000000;
  }

  travStats->cpuTimeTot = 0;

  ps = Fsm_MgrReadVarPS(fsmMgr);
  pi = Fsm_MgrReadVarI(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);

  optLevel = Ddi_MgrReadAigBddOptLevel(ddm);
#if 0
  cubeInit = Ddi_BddMakeMono(init);
  initIsCube = Ddi_BddIsCube(cubeInit);
  Ddi_Free(cubeInit);
#else
  initIsCube = 0;               //Ddi_BddarraySize(delta) < 8000;
#endif
  myInvarspec = Ddi_BddDup(invarspec);
  myInit = Ddi_BddDup(init);
  psVars = Ddi_VarsetMakeFromArray(ps);
  target = Ddi_BddNot(myInvarspec);

  if (1 && interpolantSteps != 0) {
    Ddi_Vararray_t *newStateVars;
    Ddi_Bddarray_t *newStateLits;
    Ddi_Bdd_t *newInit = NULL, *newTarget = NULL, *newInit2 = NULL,
      *overInit = NULL, *overTarget = NULL;

    Ddi_Bddarray_t *psLits = Ddi_BddarrayMakeLiteralsAig(ps, 1);
    Ddi_Bddarray_t *nsLits = Ddi_BddarrayMakeLiteralsAig(ns, 1);

    /* build tr */
    Ddi_Bdd_t *tr = Ddi_BddMakeConstAig(ddm, 1);

    if (interpolantSteps < 0) {
      interpolantSteps *= -1;
      inverse = 1;
    }

    for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
      Ddi_Bdd_t *nsLit_i = Ddi_BddarrayRead(nsLits, i);
      Ddi_Bdd_t *tr_i = Ddi_BddXnor(nsLit_i, Ddi_BddarrayRead(delta, i));

      Ddi_BddAndAcc(tr, tr_i);
      Ddi_Free(tr_i);
    }

    newTarget = Ddi_BddDup(target);
    int tryDecomp = 0;
    if (tryDecomp) {
      Ddi_Bdd_t *prop1 = Ddi_BddNot(newTarget);
      Ddi_Var_t *cvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
      Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
      Ddi_BddCofactorAcc(prop1,cvarPs,1);

      Ddi_Vararray_t *newVars = composeWithNewTimeFrameVars(prop1,
        ps, delta, pi, "PDT_BMC_TARGET_PI", i);

      Ddi_Free(newVars);

      Ddi_BddCofactorAcc(prop1,pvarPs,1);
      Ddi_BddarrayCofactorAcc(delta,pvarPs,1);

      Ddi_Bdd_t *pPart = Ddi_AigConjDecomp (prop1,8,0);
      Ddi_Free(prop1);

      int i;
      Ddi_BddNotAcc(pPart);
      for (i=0; i<Ddi_BddPartNum(pPart); i++) {
	Ddi_Bdd_t *p_i = Ddi_BddPartRead(pPart,i);
	Ddi_Vararray_t *vA = Ddi_BddSuppVararray(p_i);
	printf("Part[%d] supp: %d - size: %d - ", 
	       i, Ddi_VararrayNum(vA), Ddi_BddSize(p_i));
	Ddi_BddSetMono(p_i);
	Ddi_BddExistProjectAcc(p_i,psVars);
	Ddi_Free(vA);
	vA = Ddi_BddSuppVararray(p_i);
	printf("BDD[%d] supp: %d - size: %d\n", 
	       i, Ddi_VararrayNum(vA), Ddi_BddSize(p_i));
	Ddi_Free(vA);
      }
      for (i=0; i<Ddi_BddPartNum(pPart); i++) {
	Ddi_Bdd_t *p_i = Ddi_BddPartRead(pPart,i);
	Ddi_Bdd_t *cof = Ddi_BddNot(p_i);
	int j;
	for (j=i+1; j<Ddi_BddPartNum(pPart); j++) {
	  Ddi_Bdd_t *p_j = Ddi_BddPartRead(pPart,j);
	  int size0 = Ddi_BddSize(p_j);
	  Ddi_BddRestrictAcc(p_j,cof);
	  printf("BDD[%d] cof[%d]: %d -> %d\n", 
		 j, i, size0, Ddi_BddSize(p_j));
	}
	Ddi_Free(cof);
	Ddi_BddSetAig(p_i);
      }

      Ddi_DataCopy(newTarget,pPart);
      Ddi_Free(pPart);
    }


    for (i = 1; i < interpolantSteps; i++) {
      Ddi_Vararray_t *newVars = composeWithNewTimeFrameVars(newTarget,
        ps, delta, pi, "PDT_BMC_TARGET_PI", i);

      Ddi_Free(newVars);
    }
    newInit = Ddi_BddDup(tr);
    int nSteps = (interpolantSteps*2)/3;
    nSteps = (interpolantSteps)/2;
    for (i = 0; i < nSteps; i++) {
      Ddi_Vararray_t *newVars = composeWithNewTimeFrameVars(newInit,
        ps, delta, pi, "PDT_BMC_INIT_PI", i);
      Ddi_Free(newVars);
    }
    newInit2 = Ddi_BddDup(newInit);
    for (i = nSteps; i < interpolantSteps+nSteps; i++) {
      Ddi_Vararray_t *newVars = composeWithNewTimeFrameVars(newInit2,
        ps, delta, pi, "PDT_BMC_INIT_PI", i);
      Ddi_Free(newVars);
    }


    newStateVars = Ddi_VararrayMakeNewVars(ps, "PDT_BMC_INIT_PS", "0", 1);
    newStateLits = Ddi_BddarrayMakeLiteralsAig(newStateVars, 1);

    if (initStub != NULL) {
      Ddi_BddComposeAcc(newInit, ps, initStub);
      Ddi_BddComposeAcc(newInit2, ps, initStub);
    }
    else if (initIsCube) {
      Ddi_AigAndCubeAcc(newInit, myInit);
      Ddi_AigAndCubeAcc(newInit2, myInit);
    } else {
      Ddi_BddAndAcc(newInit, myInit);
      Ddi_BddAndAcc(newInit2, myInit);
    }

    Ddi_BddComposeAcc(newInit, ps, newStateLits);
    Ddi_BddComposeAcc(newInit, ns, psLits);
    Ddi_BddComposeAcc(newInit2, ps, newStateLits);
    Ddi_BddComposeAcc(newInit2, ns, psLits);
    Ddi_Free(newStateVars);
    Ddi_Free(newStateLits);
    Ddi_Free(psLits);
    Ddi_Free(nsLits);
    Ddi_Free(tr);

    bmcStartTime = util_cpu_time();

    /* check interpolant */
    Ddi_Bdd_t *partTarget = Ddi_BddDup(newTarget);
    Ddi_BddSetAig(newTarget);
    overTarget = Ddi_AigSatAndWithInterpolant(newTarget, newInit,
      psVars, NULL, NULL, NULL, NULL, NULL, &sat, 0, 0, -1.0);
    long startTime, stepTime;

    if (Ddi_BddIsPartDisj(partTarget)) {
      int i;
      startTime = util_cpu_time();
      Ddi_BddIncluded(newTarget,overTarget);
      stepTime = util_cpu_time() - startTime;
      fprintf(tMgrO(travMgr), "REF time: %s.\n",
	      util_print_time(stepTime));

      for (i=0; i<Ddi_BddPartNum(partTarget); i++) {
	Ddi_Bdd_t *p_i = Ddi_BddPartRead(partTarget,i);
	Ddi_Vararray_t *vA = Ddi_BddSuppVararray(p_i);
	printf("Part[%d] supp: %d - size: %d\n", 
	       i, Ddi_VararrayNum(vA), Ddi_BddSize(p_i));
	Ddi_Free(vA);
	startTime = util_cpu_time();
	Ddi_BddIncluded(p_i,overTarget);
	stepTime = util_cpu_time() - startTime;
	fprintf(tMgrO(travMgr), "part time: %s.\n",
		util_print_time(stepTime));

      }
    }
    Ddi_Free(partTarget);

    if (1) {
      Ddi_Bdd_t *B = Ddi_BddDup(overTarget);
      Ddi_Bdd_t *itp0 = Ddi_AigSatAndWithInterpolant(newInit, B,
      psVars, NULL, NULL, NULL, NULL, NULL, &sat, 0, 0, -1.0);
      int i, nRefine = 0; 

      Ddi_BddNotAcc(itp0);
      for (i=0; i<nRefine; i++) {
	Ddi_Bdd_t *itp0new = Ddi_AigSatAndWithInterpolant(itp0, newInit,
	  psVars, NULL, NULL, NULL, NULL, NULL, &sat, 0, 0, -1.0);
	Ddi_AigOptByMonotoneCoreAcc(itp0new, newInit, NULL, 0, -1.0);
	Ddi_Free(itp0);
	itp0 = itp0new;
      }
      Ddi_BddNotAcc(itp0);

      Ddi_BddNotAcc(B);
      for (i=0; i<nRefine; i++) {
	Ddi_Bdd_t *Bnew = Ddi_AigSatAndWithInterpolant(B, newTarget,
	  psVars, NULL, NULL, NULL, NULL, NULL, &sat, 0, 0, -1.0);
	Ddi_AigOptByMonotoneCoreAcc(Bnew, newTarget, NULL, 0, -1.0);
	Ddi_Free(B);
	B = Bnew;
      }
      Ddi_BddNotAcc(B);

      long startTime, stepTime;
      startTime = util_cpu_time();
      Ddi_AigSatAnd(newInit2,newTarget,0);
      stepTime = util_cpu_time() - startTime;
      fprintf(tMgrO(travMgr), "BMC 1: %s.\n",
	      util_print_time(stepTime));
      startTime = util_cpu_time();

      Ddi_AigSatAnd(newInit2,newTarget,B);

      stepTime = util_cpu_time() - startTime;
      fprintf(tMgrO(travMgr), "BMC 1 + itp: %s.\n",
	      util_print_time(stepTime));

      Ddi_BddComposeAcc(B,ps,delta);
      int isSat = Ddi_AigSatAnd(B,itp0,NULL);

      Ddi_Free(itp0);
      Ddi_Free(B);
    }

    if (!sat) {
      int size;

      DdiAigRedRemovalAcc(overTarget, NULL, -1, 30.0);
      size = Ddi_BddSize(overTarget);

      overInit = Ddi_BddNot(overTarget);

#if 0
      Ddi_Free(overTarget);
      overTarget = Ddi_AigSatAndWithInterpolant(newTarget, overInit,
        psVars, NULL, &sat, 0, 0, -1.0);
      DdiAigRedRemovalAcc(overTarget, NULL, -1, 30.0);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(tMgrO(travMgr), "recomputed overTarget: %d -> %d.\n",
          size, Ddi_BddSize(overTarget))
        );
#endif

      //      constrain = Ddi_BddDup(overTarget);
      DdiAigRedRemovalAcc(newTarget, overTarget, -1, 30.0);
      Ddi_BddAndAcc(newTarget, overTarget);
      Ddi_Free(target);
#if 1
      target = Ddi_BddDup(newTarget);
#else
      target = Ddi_BddDup(overTarget);
#endif

#if 1
      //      Ddi_BddAndAcc(newInit,overInit);
      Ddi_Free(myInit);
      myInit = Ddi_BddDup(newInit);
      initIsCube = 0;
#endif

    } else {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(tMgrO(travMgr),
          "BMC failure at bound %d (interpolant check).\n", i)
        );
    }

    bmcTime += (bmcStepTime = (util_cpu_time() - bmcStartTime));

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(tMgrO(travMgr), "INTERPOLANT overhead time: %s.\n",
        util_print_time(bmcTime))
      );

    Ddi_Free(newInit);
    Ddi_Free(newTarget);
    Ddi_Free(overInit);
    Ddi_Free(overTarget);

  }

  reached = Ddi_BddMakeConstAig(ddm, 0);
  totPiVars = Ddi_VarsetMakeFromArray(pi);
  unroll = Ddi_BddarrayDup(delta);

  for (i = nsat = 0; i <= bound && !sat; i++) {
    if (util_cpu_time() > time_limit) {
      //printf("Invariant verification UNDECIDED after BMC ...\n");
      sat = -1;
      break;
    }

    if (i == interpolantSteps - 7 && constrain != NULL) {
      if (inverse) {
        Ddi_BddDiffAcc(target, constrain);
      } else {
        Ddi_BddAndAcc(target, constrain);
      }
    }

    if (i >= first && ((i - first) % step) == 0) {
      Ddi_Bdd_t *check;
      int sat1, enCheck = 1;

      if (0 && nsat++ == 0) {
        DdiAigRedRemovalAcc(target, NULL, -1, 30.0);
        DdiAigRedRemovalAcc(myInit, NULL, -1, 30.0);
      }

      if (initStub != NULL) {
        check = Ddi_BddCompose(target, ps, initStub);
      } else if (initIsCube) {
        check = Ddi_BddDup(target);
        Ddi_AigAndCubeAcc(check, myInit);
      } else {
        check = Ddi_BddAnd(target, myInit);
      }

      if (0) {
        Ddi_Bddarray_t *benchArray;
        char filename[100];
        Ddi_Bdd_t *care;

        sprintf(filename, "bmc%02d.bench", i);
        benchArray = Ddi_BddarrayAlloc(ddm, 0);
        Ddi_BddarrayInsertLast(benchArray, target);
        care = Ddi_BddMakeConstAig(ddm, 1);
        Ddi_BddarrayInsertLast(benchArray, care);
        Ddi_Free(care);
        Ddi_AigarrayNetStore(benchArray, filename, NULL,
          Pdtutil_Aig2BenchLocalId_c);
        Ddi_Free(benchArray);
      }
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(tMgrO(travMgr), "BMC check at bound %2d - ", i)
        );
      bmcStartTime = util_cpu_time();

      if (doApprox) {
        int size = Ddi_BddSize(check);

        //        Ddi_Bdd_t *subs = Ddi_BddNot(check);
        Ddi_Bdd_t *subs = Ddi_BddDup(check);

        DdiAigExistOverPartialAcc(subs, totPiVars, NULL, size / 2);
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(tMgrO(travMgr), "SUBS: %d/%d.\n", Ddi_BddSize(subs),
            Ddi_BddSize(check))
          );
        if (1) {
          Ddi_Bdd_t *a = Ddi_BddDiff(check, subs);

          Pdtutil_Assert(Ddi_AigSatBerkmin(a) != 1, "invalid overappr");
          Ddi_Free(a);
        }
        //        DdiAigRedRemovalAcc (check,subs,-1);
        //        Ddi_BddNotAcc(subs);
        if (Ddi_AigSatBerkmin(subs) != 1) {
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(tMgrO(travMgr), "UNSAT with overappr.\n")
            );
          sat1 = 0;
          enCheck = 0;
        } else {
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(tMgrO(travMgr), "SAT with overappr.\n")
            );
        }
        Ddi_Free(subs);
        Ddi_MgrConsistencyCheck(ddm);

      }
      if (!enCheck) {
        sat1 = 0;
      } else if (strcmp(travMgr->settings.aig.satSolver, "csat") == 0) {
        sat1 = Ddi_AigSatCircuit(check);
      } else if (0 && strcmp(travMgr->settings.aig.satSolver, "minisat") == 0) {
        sat1 = Ddi_AigSatMinisat(check);
      } else {
        if (util_cpu_time() >= time_limit) {
          sat1 = -1;
        } else {
          sat1 =
            Ddi_AigSatWithAbort(check,
            (time_limit - util_cpu_time()) / 1000.0);
        }
        //sat1 = Ddi_AigSat(check);
      }
      if (0) {
        //Ddi_Bdd_t *subs = Ddi_AigSubset (check,(int)(Ddi_BddSize(check)*0.99),0);
        Ddi_Bdd_t *subs = Ddi_AigSupersetNode(check, Ddi_VararrayNum(ps), 0);

        if (subs)
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(tMgrO(travMgr), "SUPER: %d/%d = %d.\n",
              Ddi_BddSize(subs), Ddi_BddSize(check),
              (int)Ddi_AigSatMinisat(subs))
            );
        //Ddi_AigSatBerkmin(subs);
        Ddi_Free(subs);
      }
      Ddi_Free(check);

      bmcTime += (bmcStepTime = (util_cpu_time() - bmcStartTime));
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(tMgrO(travMgr), "BMC step time: %s.\n",
          util_print_time(bmcStepTime))
        );

      pthread_mutex_lock(&travStats->mutex);
      if (travStats->arrayStatsNum > 0 &&
        strcmp(Pdtutil_ArrayReadName(travStats->arrayStats[0]),
          "BMCSTATS") == 0) {
        Pdtutil_IntegerArrayInsertLast(travStats->arrayStats[0], bmcStepTime);
      }
      travStats->cpuTimeTot += bmcStepTime;
      travStats->provedBound = i;
      pthread_mutex_unlock(&travStats->mutex);

      /*Pdtutil_Assert(sat==sat2,"wrong result of circuit sat"); */
      if (sat1 == 1) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(tMgrO(travMgr), "BMC failure at bound %d.\n", i)
          );
        provedBound = -i;
        sat = 1;
      }
      if (sat1 == -1) {
        //printf("Invariant verification UNDECIDED after BMC ...\n");
        sat = -1;
      }
      if (sat1 == 0) {
        provedBound = i;
      }
    }
    if (!sat) {
      int j;
      Ddi_Bddarray_t *delta_i;
      Ddi_Bdd_t *invarspec_i;
      Ddi_Vararray_t *newPi = Ddi_VararrayAlloc(ddm, Ddi_VararrayNum(pi));
      Ddi_Bddarray_t *newPiLit = Ddi_BddarrayAlloc(ddm, Ddi_VararrayNum(pi));

      for (j = 0; j < Ddi_VararrayNum(pi); j++) {
        char name[1000];
        Ddi_Var_t *v = Ddi_VararrayRead(pi, j);
        Ddi_Var_t *newv = NULL;
        Ddi_Bdd_t *newvLit;

        sprintf(name, "%s_%d", Ddi_VarName(v), i);
#if 0
        newv = Ddi_VarNewBeforeVar(v);
        Ddi_VarAttachName(newv, name);
#else
        newv = Ddi_VarFromName(ddm, name);
        if (newv == NULL) {
          newv = Ddi_VarNewBaig(ddm, name);
        }
#endif
        Ddi_VararrayWrite(newPi, j, newv);
        //      Ddi_VarsetAddAcc(totPiVars,newv);
        newvLit = Ddi_BddMakeLiteralAig(newv, 1);
        Ddi_BddarrayWrite(newPiLit, j, newvLit);
        Ddi_Free(newvLit);
      }
      delta_i = Ddi_BddarrayDup(delta);
      Ddi_BddarrayComposeAcc(delta_i, pi, newPiLit);
      Ddi_BddComposeAcc(target, ps, delta_i);
      invarspec_i = Ddi_BddCompose(myInvarspec, pi, newPiLit);
      if (i < step - 1) {
        Ddi_BddNotAcc(invarspec_i);
        Ddi_BddOrAcc(target, invarspec_i);
      } else {
        Ddi_BddAndAcc(target, invarspec_i);
      }
#if 0
      if (0) {
        Ddi_Bdd_t *careAig = Ddi_BddNot(reached);

        Ddi_AigExistAcc(target, totPiVars, careAig, 1 /*partial */ , 3, -1.0);
        to = Ddi_BddDup(target);
        Ddi_AigExistAcc(to, totPiVars, careAig, 0, 0, -1.0);
        Ddi_BddrOoptleAcc(reached, to);
        Ddi_Free(to);
        Ddi_Free(careAig);
      }
#endif
      if (0 && optLevel > 0) {
        int size = Ddi_BddSize(target);

        if (size > doOptThreshold) {
          int n;

          if (optLevel == 1) {
            DdiAigRedRemovalAcc(target, NULL, 1000, 60.0);
          } else {
            for (n = 1; n < optLevel; n++)
              DdiAigOptByBddRelation(target, 500, 500, psVars, 200.0);
          }
          size = Ddi_BddSize(target);
          if (size * 1.5 > doOptThreshold)
            doOptThreshold = (int)(size * 1.5);
          // Ddi_AigExistProjectAcc(newCheck,psVars,NULL,3,0,20.0);
        }
      }

      Fsm_MgrLogUnsatBound(fsmMgr, i, 0);

      Ddi_Free(invarspec_i);
      Ddi_Free(delta_i);
      Ddi_Free(newPi);
      Ddi_Free(newPiLit);
    }
  }

  if (!sat) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(tMgrO(travMgr), "BMC pass at bound %d.\n", provedBound)
      );
  }

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(tMgrO(travMgr), "Total SAT time: %s, total BMC time: %s.\n",
      util_print_time(bmcTime), util_print_time(util_cpu_time() - startTime))
    );

  Trav_MgrSetAssertFlag(travMgr, sat);

  Ddi_Free(psVars);
  Ddi_Free(constrain);
  Ddi_Free(myInvarspec);
  Ddi_Free(myInit);
  Ddi_Free(unroll);
  Ddi_Free(target);
  Ddi_Free(reached);
  Ddi_Free(totPiVars);
  return provedBound + 1;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
bmcCheckTargetByRefinement(
  TravBmcMgr_t *bmcMgr,
  Ddi_IncrSatMgr_t *ddiS,
  int bmcStep,
  Ddi_Bdd_t *checkPart,
  Ddi_Vararray_t *cutVars,
  Ddi_Vararray_t *cutAuxVars,
  float satTimeLimit,
  int *pabort
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(checkPart);
  Ddi_Bdd_t *cex;
  int refined = 0;
  Ddi_Bdd_t *targetCone = bmcMgr->bmcTargetCone;
  Ddi_Bdd_t *itpTarget = NULL;
  Ddi_Vararray_t *smVarsA=NULL, *addVars=NULL;
  Ddi_Varset_t *smVars=NULL;
  int nAdd = 0, stepAdd = 0;
  Ddi_Bdd_t *myTarget = Ddi_BddDup(checkPart);
  Ddi_Bddarray_t *unroll_k = (Ddi_Bddarray_t *)
    Pdtutil_PointerArrayRead(bmcMgr->unrollArray,bmcStep);
  Ddi_Bdd_t *myA0 = Ddi_BddRelMakeFromArray(unroll_k, cutVars);
  Ddi_Bdd_t *myA0Aux = Ddi_BddRelMakeFromArray(unroll_k, cutAuxVars);
  Ddi_Varset_t *globalVars = Ddi_VarsetMakeFromArray(cutVars);
  int checkWithNewSolver = 0;
  int useEnables = 1, enFoMin = 10;
  Ddi_Bddarray_t *en = NULL;

  Ddi_VarsetSetArray(globalVars);


  if (Ddi_BddIsOne(myTarget)) {
    Ddi_Free(myTarget);
    myTarget = Ddi_BddDup(myA0Aux);
    Ddi_BddSetAig(myTarget);
    checkWithNewSolver = 1;
  }
  else if (bmcStep == 89 || bmcStep == 70 || bmcStep>4 && 
	   bmcStep % bmcMgr->settings.targetByRefinement == 0) {
    checkWithNewSolver = 1;
  }

  int iter=0, maxIter = 4;
  int assumeNotTarget = 1;
  do {
    refined = 0;
    if (Ddi_BddIsZero(myTarget)) {
      cex = NULL;
    }
    else if (iter>0 || checkWithNewSolver) {
      Ddi_IncrSatMgrSuspend(ddiS);
      cex = Ddi_AigSatWithCexAndAbort(myTarget, NULL,
                  satTimeLimit, pabort);
      Ddi_IncrSatMgrResume(ddiS);
    }
    else if (iter>=maxIter) {
      Ddi_BddarrayInsertLast(bmcMgr->dummyRefs,myA0);
      cex = Ddi_AigSatMinisat22WithCexAndAbortIncremental(ddiS,
			       myA0, NULL, 0,
			       satTimeLimit, pabort);
    }
    else {
      Ddi_BddarrayInsertLast(bmcMgr->dummyRefs,myTarget);
      cex = Ddi_AigSatMinisat22WithCexAndAbortIncremental(ddiS,
			       myTarget, NULL, 0,
			       satTimeLimit, pabort);
    }
    if (cex==NULL) {
      if (itpTarget!=NULL) {
	int bwdK = Ddi_BddReadMark(bmcMgr->bmcTargetCone);
	Pdtutil_Assert (bmcMgr->bwd.rings!=NULL,"No bwd rings"); 
	Ddi_BddarrayWrite(bmcMgr->bwd.rings,bwdK,itpTarget);
	// assume !target
	if (assumeNotTarget) {
	  Ddi_BddNotAcc(myTarget);
	  Ddi_BddarrayInsertLast(bmcMgr->dummyRefs,myTarget);
	  Ddi_AigSatMinisatLoadClausesIncrementalAsserted(ddiS,
							  myTarget);
	}
	Ddi_AigSatMinisatLoadClausesIncrementalAsserted(ddiS,
	  itpTarget);
      }
    }    
    else {
      if (1) {
	Ddi_Bdd_t *cex2;
	Ddi_Bdd_t *check = Ddi_BddDup(myA0Aux);
	Ddi_BddSetAig(check);
	Ddi_AigAndCubeAcc(check, cex);
	Ddi_Free(cex);
	Ddi_IncrSatMgrSuspend(ddiS);
        cex = Ddi_AigSatWithCexAndAbort(check, NULL,
                   satTimeLimit, pabort);
	Ddi_Free(check);
	Pdtutil_Assert(cex!=NULL,"cex mismatch");
	Ddi_IncrSatMgrResume(ddiS);
      }

      Ddi_Bdd_t *cex1, *myTarget1 = Ddi_BddDup(targetCone);
      Ddi_Varset_t *removeVars = Ddi_BddSupp(targetCone);
      Ddi_AigCubeExistAcc (cex,removeVars);
      Ddi_Free(removeVars);
      removeVars = Ddi_VarsetMakeFromArray(cutVars);
      Ddi_AigCubeExistAcc (cex,removeVars);
      Ddi_Free(removeVars);
      Ddi_BddSubstVarsAcc(cex,cutAuxVars,cutVars);
      Ddi_AigAndCubeAcc(myTarget1, cex);
#if 1
      Ddi_IncrSatMgrSuspend(ddiS);
      cex1 = Ddi_AigSatWithCexAndAbort(myTarget1, NULL,
                      satTimeLimit, pabort);
#else
      cex1 = Ddi_AigSatMinisat22WithCexAndAbortIncremental(ddiS,
			       cex, NULL, 0,
			       satTimeLimit, pabort);
      Ddi_IncrSatMgrSuspend(ddiS);
#endif
      Ddi_Free(myTarget1);
      if (cex1!=NULL) {
	Ddi_Free(cex);
	cex = cex1; cex1 = NULL;
      }
      else {
	// refine
	int j, sat, nEn=0;;
	Ddi_Bdd_t *itp, *myA = Ddi_BddDup(myA0);
	Ddi_Bdd_t *clearCheck = Ddi_BddMakeConstAig(ddm, 0);
	Ddi_DataCopy(checkPart,clearCheck);
	Ddi_Free(clearCheck);
	if (smVars==NULL) {
	  // first time
	  Ddi_Vararray_t *suppCone = Ddi_BddSuppVararray(targetCone);
	  smVarsA = Ddi_BddSuppVararray(myA0);
	  Ddi_VararrayUnionAcc(smVarsA,cutVars);
	  addVars = Ddi_VararrayDiff(smVarsA,cutVars); 
	  Ddi_VararrayIntersectAcc(smVarsA,cutVars);
	  Ddi_VararrayUnionAcc(smVarsA,suppCone);
	  Ddi_Free(suppCone);
	  smVars = Ddi_VarsetMakeFromArray(smVarsA);
	  Ddi_VarsetSetArray(smVars);
	  nAdd = Ddi_VararrayNum(addVars);
	  stepAdd = nAdd/4;
	}
	if (useEnables) {
	  Ddi_BddSetAig(myA);
	  if (en==NULL) {
	    en = Ddi_FindAigIte(myA, enFoMin);
	  }
	  else {
	    int nEn = Ddi_BddarrayNum(en);
	    if (nEn>20) {
	      enFoMin++;
	      Ddi_Free(en);
	      en = Ddi_FindAigIte(myA, enFoMin);
	    }
	    else {
	      Ddi_BddarrayRemove(en,nEn-1);
	    }
	  }
	}
	else {
	  for (j=0; j<stepAdd && 
		 Ddi_VararrayNum(addVars)>10; j++) {
	    int iTop = Ddi_VararrayNum(addVars)-1;
	    Ddi_Var_t *vSm = Ddi_VararrayExtract(addVars,iTop);
	    Ddi_VarsetAddAcc(smVars,vSm);
	  }
          if (stepAdd > 100)
            stepAdd /= 4;
	  Ddi_AigCubeExistAcc (cex,smVars);
	}

	if (itpTarget==NULL) {
	  int bwdK = Ddi_BddReadMark(bmcMgr->bmcTargetCone);
	  if (bmcMgr->bwd.rings!=NULL) {
	    Ddi_Bdd_t *bRing = Ddi_BddarrayRead(bmcMgr->bwd.rings,bwdK);
	    if (bRing != NULL)
	      itpTarget = Ddi_BddDup(bRing);
	  }
	}

	if (useEnables) {
	  Ddi_Bdd_t *a2 = Ddi_AigSubsetWithCexOnControl (myA,cex,en);
	  nEn = Ddi_BddarrayNum(en);
	  Ddi_Free(myA);
	  myA = a2;
	}
	else 
	  if (itpTarget!=NULL && !Ddi_BddIsOne(itpTarget)){
	    // disable on first run
	    Ddi_AigConstrainCubeAcc(myA, cex);
	    assumeNotTarget = 0;
	  }
	// reverse interpolant - NO: now direct
	//	Ddi_BddSetAig(myA);

	Pdtutil_VerbosityMgrIf(bmcMgr->travMgr, Pdtutil_VerbLevelUsrMin_c) {
	  fprintf(tMgrO(bmcMgr->travMgr), 
		  "TARGET ITP REFINEMENT iteration %d (cex vars: %d) - #enables: %d\n", 
		  iter, Ddi_BddSize(cex)/2, nEn);
	}

	itp = Ddi_AigSat22AndWithInterpolant(NULL,myA,targetCone,
				     NULL,globalVars,NULL,NULL,0,
				     NULL,NULL,
				     &sat, 0, 1, 0, -1.0);

	if (itp==NULL) {
	  // sat
	  Ddi_Free(cex);
	  Ddi_BddAndAcc(myA,targetCone);
	  cex = Ddi_AigSatWithCexAndAbort(myA, NULL,
					  satTimeLimit, pabort);
	}
	else {
	  refined = 1;
	  Ddi_AigOptByMonotoneCoreAcc(itp,targetCone,NULL,0,-1.0);
	  if (itpTarget==NULL) {
	    itpTarget = Ddi_BddMakeConstAig(ddm, 1);
	  }
	  Ddi_BddDiffAcc(itpTarget,itp);
	  Ddi_Free(itp);
	  Ddi_Free(myTarget);
          if (1) {
            Ddi_BddNotAcc(itpTarget);
            Ddi_Bdd_t *itp1 = Ddi_AigSat22AndWithInterpolant(NULL,
                                     itpTarget,targetCone,
				     NULL,globalVars,NULL,NULL,0,
				     NULL,NULL,
				     &sat, 0, 1, 0, -1.0);
            Ddi_AigOptByMonotoneCoreAcc(itp1,targetCone,NULL,0,-1.0);
            if (Ddi_BddSize(itp1)<Ddi_BddSize(itpTarget)) {
              Pdtutil_VerbosityMgrIf(bmcMgr->travMgr, Pdtutil_VerbLevelUsrMin_c) {
                fprintf(tMgrO(bmcMgr->travMgr), 
		  "Compacting itp: %d -> %d\n", 
                        Ddi_BddSize(itpTarget), Ddi_BddSize(itp1));
                Ddi_Free(itpTarget);
                itpTarget = Ddi_BddDup(itp1);
              }
            }
            Ddi_BddNotAcc(itpTarget);
            Ddi_Free(itp1);
          }
          myTarget = Ddi_BddCompose(itpTarget,cutVars,unroll_k);
	}
	Ddi_Free(cex);
	Ddi_Free(myA);
      }
      Ddi_IncrSatMgrResume(ddiS);
    }
    iter++;
  } while (refined);

  Ddi_Free(globalVars);
  Ddi_Free(myA0);
  Ddi_Free(en);
  Ddi_Free(myA0Aux);
  Ddi_Free(itpTarget);
  Ddi_Free(myTarget);
  Ddi_Free(smVars);
  Ddi_Free(smVarsA);
  Ddi_Free(addVars);

  return cex;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
bmcCheckTargetByRefinementOnCone(
  TravBmcMgr_t *bmcMgr,
  Ddi_IncrSatMgr_t *ddiS,
  int bmcStep,
  Ddi_Bdd_t *checkPart,
  Ddi_Vararray_t *cutVars,
  Ddi_Vararray_t *cutAuxVars,
  float satTimeLimit,
  int *pabort
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(checkPart);
  Ddi_Bdd_t *cex;
  int refined = 0;
  Ddi_Bdd_t *targetCone = bmcMgr->bmcTargetCone;
  Ddi_Bdd_t *itpTarget = NULL;
  Ddi_Vararray_t *smVarsA=NULL, *addVars=NULL;
  Ddi_Varset_t *smVars=NULL;
  int nAdd = 0, stepAdd = 0;
  Ddi_Bdd_t *myTarget = Ddi_BddDup(checkPart);
  Ddi_Bddarray_t *unroll_k = (Ddi_Bddarray_t *)
    Pdtutil_PointerArrayRead(bmcMgr->unrollArray,bmcStep);
  Ddi_Bdd_t *myA0 = Ddi_BddRelMakeFromArray(unroll_k, cutVars);
  Ddi_Bdd_t *myA0Aux = Ddi_BddRelMakeFromArray(unroll_k, cutAuxVars);
  Ddi_Varset_t *globalVars = Ddi_VarsetMakeFromArray(cutVars);
  int checkWithNewSolver = 0;
  Ddi_Bddarray_t *en = NULL;

  Ddi_VarsetSetArray(globalVars);


  if (1 && Ddi_BddIsOne(myTarget)) {
    Ddi_Free(myTarget);
    myTarget = Ddi_BddDup(targetCone);
    Ddi_BddSetAig(myTarget);
    //    checkWithNewSolver = 1;
  }

  int iter=0, maxIter = 200, enFoMin = 20;
  int assumeNotTarget = 1;
  do {
    refined = 0;
    if (Ddi_BddIsZero(myTarget)) {
      cex = NULL;
    }
    else if (checkWithNewSolver) {
      Ddi_IncrSatMgrSuspend(ddiS);
      cex = Ddi_AigSatWithCexAndAbort(myTarget, NULL,
                  satTimeLimit, pabort);
      Ddi_IncrSatMgrResume(ddiS);
    }
    else if (iter>=maxIter) {
      Ddi_BddarrayInsertLast(bmcMgr->dummyRefs,myA0);
      cex = Ddi_AigSatMinisat22WithCexAndAbortIncremental(ddiS,
			       myA0, NULL, 0,
			       satTimeLimit, pabort);
    }
    else {
      Ddi_BddarrayInsertLast(bmcMgr->dummyRefs,myTarget);
      cex = Ddi_AigSatMinisat22WithCexAndAbortIncremental(ddiS,
			       myTarget, NULL, 0,
			       satTimeLimit, pabort);
    }
    if (cex==NULL) {
      if (itpTarget!=NULL) {
	int bwdK = Ddi_BddReadMark(bmcMgr->bmcTargetCone);
	Pdtutil_Assert (bmcMgr->bwd.rings!=NULL,"No bwd rings"); 
	Ddi_BddarrayWrite(bmcMgr->bwd.rings,bwdK,itpTarget);
	// assume !target
	if (assumeNotTarget) {
	  Ddi_BddNotAcc(myTarget);
	  Ddi_BddarrayInsertLast(bmcMgr->dummyRefs,myTarget);
	  Ddi_AigSatMinisatLoadClausesIncrementalAsserted(ddiS,
							  myTarget);
	}
	//	Ddi_AigSatMinisatLoadClausesIncrementalAsserted(ddiS,
	//  itpTarget);
      }
    }    
    else {

      // check feasibility on A
      Ddi_Bdd_t *cex1, *cex0 = Ddi_BddDup(cex);
      Ddi_Vararray_t *removeVarsA = Ddi_BddSuppVararray(cex);
      Ddi_Varset_t *removeVars = NULL;
      Ddi_VararrayDiffAcc(removeVarsA,cutVars);
      removeVars = Ddi_VarsetMakeFromArray(removeVarsA);
      Ddi_Free(removeVarsA);
      Ddi_AigCubeExistAcc (cex0,removeVars);
      Ddi_Free(removeVars);
      Ddi_BddComposeAcc(cex0,cutVars,unroll_k);
      Ddi_IncrSatMgrSuspend(ddiS);
      cex1 = Ddi_AigSatWithCexAndAbort(cex0, NULL,
                       satTimeLimit, pabort);
      //Ddi_IncrSatMgrResume(ddiS);
      Ddi_Free(cex0);
      if (cex1!=NULL) {
	Ddi_Free(cex);
	cex = cex1; cex1 = NULL;
      }
      else {
	// refine
	int j, sat;
	Ddi_Bdd_t *itp, *myA = Ddi_BddDup(myA0);
	Ddi_Bdd_t *clearCheck = Ddi_BddMakeConstAig(ddm, 0);
	Ddi_DataCopy(checkPart,clearCheck);
	Ddi_Free(clearCheck);
	if (en==NULL) {
	  en = Ddi_FindAigIte(targetCone, enFoMin);
	}
	else {
	  int nEn = Ddi_BddarrayNum(en);
	  if (nEn>20) {
	    enFoMin++;
	    Ddi_Free(en);
	    en = Ddi_FindAigIte(targetCone, enFoMin);
	  }
	  else {
	    Ddi_BddarrayRemove(en,nEn-1);
	  }
	}
	Ddi_Bdd_t *b2 = Ddi_AigSubsetWithCexOnControl (targetCone,cex,en);

	Pdtutil_VerbosityMgrIf(bmcMgr->travMgr, Pdtutil_VerbLevelUsrMin_c) {
	  fprintf(tMgrO(bmcMgr->travMgr), 
		  "TARGET ITP REFINEMENT iteration %d (cex vars: %d) - enables: %d\n", 
		  iter, Ddi_BddSize(cex)/2, Ddi_BddarrayNum(en));
	}

	itp = Ddi_AigSat22AndWithInterpolant(NULL,myA,b2,
				     NULL,globalVars,NULL,NULL,0,
				     NULL,NULL,
				     &sat, 0, 1, 0, -1.0);

	if (itp==NULL) {
	  // sat
	  Ddi_Free(cex);
	  Ddi_BddAndAcc(myA,b2);
	  cex = Ddi_AigSatWithCexAndAbort(myA, NULL,
					  satTimeLimit, pabort);
	}
	else {
	  Ddi_Bdd_t *fwdConstr;
	  refined = 1;
	  Ddi_AigOptByMonotoneCoreAcc(itp,myA,NULL,1,-1.0);
	  if (itpTarget==NULL) {
	    itpTarget = Ddi_BddMakeConstAig(ddm, 1);
	  }
	  Ddi_BddAndAcc(itpTarget,itp);
	  Ddi_BddSetAig(myA0);
	  Pdtutil_Assert(Ddi_BddIncluded(myA0,itpTarget),"unsound itp");
	  Ddi_BddAndAcc(myTarget,itp);
	  fwdConstr = Ddi_BddCompose(itp,cutVars,unroll_k);
	  Ddi_BddAndAcc(myTarget,fwdConstr);
	  Ddi_Free(itp);
	  Ddi_Free(fwdConstr);
	}
	Ddi_Free(cex);
	Ddi_Free(myA);
      }
      Ddi_IncrSatMgrResume(ddiS);
    }
    iter++;
  } while (refined);

  Ddi_Free(globalVars);
  Ddi_Free(en);
  Ddi_Free(myA0);
  Ddi_Free(myA0Aux);
  Ddi_Free(itpTarget);
  Ddi_Free(myTarget);
  Ddi_Free(smVars);
  Ddi_Free(smVarsA);
  Ddi_Free(addVars);

  return cex;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Trav_TravSatBmcIncrVerif(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bddarray_t * prevCexes,
  int bound,
  int step,
  int stepFrames,
  int first,
  int doReturnCex,
  int lemmaSteps,
  int interpolantSteps,
  int strategy,
  int teSteps,
  int timeLimit
)
{
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Varset_t *totPiVars;
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bdd_t *target, *myInit, *myInvarspec; //, *to;
  Ddi_Bdd_t *cubeInit;
  Ddi_Bddarray_t *unroll;
  int sat = 0, i, nsat;
  long startTime, bmcTime = 0, bmcStartTime, bmcStepStartTime, bmcStepTime;
  Ddi_Bdd_t *constrain = NULL;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);
  int provedBound = 0, inverse = 0;
  int doOptThreshold = 5000;
  int optLevel = 0;
  Ddi_Varset_t *psVars = NULL;
  Ddi_Varset_t *nsVars = NULL;
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  unsigned long time_limit = ~0;
  float stepTimeLimit = -1.0;
  float incrSolverTimeLimit = 5.0;
  Trav_Stats_t *travStats = &(travMgr->xchg.stats);
  Trav_Tune_t *travTune = &(travMgr->xchg.tune);
  Ddi_Bdd_t *returnedCex = NULL;
  Ddi_Varset_t *tfSupp = NULL;
  Ddi_Bddarray_t *myCexes = NULL;
  int currCexId = -1, currCexLen = -1;
  int safe = -1;
  int doMonotone = 0;
  //  Solver S;
  Ddi_IncrSatMgr_t *ddiS = NULL;
  int doImpl = Trav_MgrReadImplAbstr(travMgr);
  Ddi_Bdd_t *trAux = NULL;
  TravBmcMgr_t *bmcMgr = NULL;
  int assumeInvarspec = (strategy % 2 == 0);
  int useMinisat22 = 1;
  int unfolded = 0, initSteps = 0;
  int enLogBound=0;
  int targetConeFrames = teSteps;
  int doTernarySim =1;
  int shiftFwdFrames = 1;

  int stepIncrFactor = 4, stepIncrIter = 4 * 4 * 8;
  Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
  Ddi_Var_t *cvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
  int nnf = 0;

  int tryFwdBwd = 0;
  int useShared = Trav_TravBmcSharedActive();

  int first0 = -1;
  int myThrdId = -1;
  char myThrdIdStr[20] = "";
  int incrSolver = 0;
  int doBmcInvar = 0;
  // int itpTargetByRefinement = 0; //teSteps>0 ? teSteps : 0;
  int itpTargetByRefinement = teSteps>0 ? teSteps : 0
;
  if (useShared) {
    myThrdId = Trav_TravBmcSharedRegisterThread();
    sprintf(myThrdIdStr,"[Thrd: %d] ", myThrdId);
  }

  if (first > 20) first0 = first/3;

  if (first>0) {
    stepTimeLimit = Ddi_MgrReadAigSatTimeLimit(ddm);
    //    if (stepRimeLimit > 0) incrSolver = 1;
  }

  if (tryFwdBwd) {
    return Trav_TravSatBmcIncrVerifFwdBwd(
	      travMgr /* Traversal Manager */ ,
	      fsmMgr,
	      init,
	      invarspec,
	      invar,
	      bound,
	      step,
	      stepFrames,
	      first,
	      doReturnCex,
	      lemmaSteps,
	      interpolantSteps,
	      strategy,
	      teSteps,
	      timeLimit
    );
  }

  if (strategy>100) {
    strategy -= 100;
    useMinisat22 = -1; // LGL
  }

  if (strategy==4) {
    strategy = 2;
    tryFwdBwd = 1;
  }

  startTime = util_cpu_time();
  if (timeLimit > 0) {
    time_limit = util_cpu_time() + timeLimit * 1000;
  }
  if (stepFrames <= 0) {
    stepFrames = 1;             //default is 4!
  }
  if (bound < 0) {
    bound = 1000000;
  }

  if (strategy >= 100) {
    strategy -= 100;
    nnf = 1;
  }

  if (0) {
    Ddi_Bdd_t *e0 = Ddi_BddMakeFromBaig(ddm,1192*4);
    Ddi_Bdd_t *e1 = Ddi_BddMakeFromBaig(ddm,1196*4);
    Ddi_Bddarray_t *fA=delta;

    Ddi_BddAndAcc(invar,e0);
    //    Ddi_BddAndAcc(invar,e1);

#if 1
    Ddi_Bddarray_t *fA_00 = Ddi_AigarrayConstrain(fA,e0,0);
    Ddi_Bddarray_t *fA_01 = Ddi_AigarrayConstrain(fA,e0,1);
    Ddi_Bddarray_t *fA_10 = Ddi_AigarrayConstrain(fA,e1,0);
    Ddi_Bddarray_t *fA_11 = Ddi_AigarrayConstrain(fA,e1,1);

    printf("enable constr en0: %d, %d\n", 
	   Ddi_BddarraySize(fA_00),Ddi_BddarraySize(fA_01));

    printf("enable constr en1: %d, %d\n", 
	   Ddi_BddarraySize(fA_10),Ddi_BddarraySize(fA_11));

    Ddi_Free(fA_00);
    Ddi_Free(fA_01);
    Ddi_Free(fA_10);
    Ddi_Free(fA_11);
#endif

    Ddi_Free(e0);
    Ddi_Free(e1);
  }

  if (prevCexes != NULL) {
    int ii, prevNum = 0;

    myCexes = Ddi_BddarrayDup(prevCexes);
    for (ii = 0; ii < Ddi_BddarrayNum(myCexes); ii++) {
      Ddi_Bdd_t *cex_ii = Ddi_BddarrayRead(myCexes, ii);

      Pdtutil_Assert(Ddi_BddPartNum(cex_ii) >= prevNum,
        "cex array not sorted");
      prevNum = Ddi_BddPartNum(cex_ii);
    }
    currCexId = 0;
    bound = prevNum - 1;
    currCexLen = Ddi_BddPartNum(Ddi_BddarrayRead(myCexes, currCexId));
    Pdtutil_Assert(currCexLen >= 2, "wrong cex len");
    first = currCexLen - 2;
    Pdtutil_Assert(Ddi_BddarrayNum(myCexes) > 0, "no prev cexes");
  }

  travStats->cpuTimeTot = 0;


  bmcMgr = TravBmcMgrInit(travMgr, fsmMgr, delta, init, initStub, invarspec,
    invar, NULL, NULL);
  bmcMgr->settings.shiftFwdFrames = shiftFwdFrames;
  bmcMgr->settings.stepFrames = stepFrames; 

  bmcMgr->settings.targetByRefinement = itpTargetByRefinement;

  if (targetConeFrames>0) {
    int teSteps = Fsm_MgrReadTargetEnSteps(fsmMgr);
    Fsm_MgrSetTargetEnSteps(fsmMgr,targetConeFrames);
    TravBmcMgrAddTargetCone(bmcMgr,teSteps+targetConeFrames);
  }
  if (doTernarySim) {
    TravBmcMgrTernarySimInit(bmcMgr,3);
  }

  optLevel = Ddi_MgrReadAigBddOptLevel(ddm);

  if (strategy <= 2) {
    DdiAig2CnfIdInit(ddm);
    ddiS = Ddi_IncrSatMgrAlloc(ddm,useMinisat22, 0, 0);
    if (bmcMgr->bmcTargetCone!=NULL/* && !itpTargetByRefinement*/) {
      Ddi_Vararray_t *coneSupp = Ddi_BddSuppVararray(bmcMgr->bmcTargetCone);
      Ddi_VararrayIntersectAcc(coneSupp,Fsm_MgrReadVarPS(fsmMgr));
      Ddi_AigSatMinisatLoadClausesIncrementalAsserted(ddiS,
        bmcMgr->bmcTargetCone);
      Ddi_IncrSolverFreezeVars (ddm,ddiS,coneSupp);
      Ddi_Free(coneSupp);
      if (bmcMgr->bmcConstrCone != NULL)
	Ddi_AigSatMinisatLoadClausesIncrementalAsserted(ddiS,
	  bmcMgr->bmcConstrCone);
    }
  }

  bmcStartTime = util_cpu_time();

  initSteps = Ddi_VarsetarrayNum(bmcMgr->timeFrameVars);

  for (i = nsat = 0; i <= bound && (sat<=0); i++) {

    if (i == stepIncrIter) {
      bmcMgr->settings.stepFrames *= stepIncrFactor;
    }

    TravBmcMgrAddFrames(bmcMgr, ddiS, i, NULL);

    if (1 && Ddi_BddPartNum(bmcMgr->bmcConstr) >= i && i>0) {
      // i-1 as constrain is 1 time frame ahead of target
      Ddi_Bdd_t *constr_i = Ddi_BddPartRead(bmcMgr->bmcConstr, i-1);

      if (ddiS != NULL && constr_i!=NULL && !Ddi_BddIsOne(constr_i)) {
        Ddi_AigSatMinisatLoadClausesIncrementalAsserted(ddiS,
          constr_i);
      }
    }

    //    printf("\nBOUND %d\n", i);
    //    Ddi_IncrSatPrintStats(ddm,ddiS,0);    

    if (myCexes != NULL && currCexId >= Ddi_BddarrayNum(myCexes)) {
      sat = -1;
      break;
    }

    //  if (util_cpu_time() > time_limit) {
    if (bmcOutOfLimits(bmcMgr)) {
      //printf("Invariant verification UNDECIDED after BMC ...\n");
      sat = -1;
      break;
    }

    if (travTune!=NULL) {
      int doSleep;
      pthread_mutex_lock(&travTune->mutex);
      doSleep = travTune->doSleep;
      pthread_mutex_unlock(&travTune->mutex);
      if (doSleep>0) {
	sleep(doSleep);
	pthread_mutex_lock(&travTune->mutex);
	travTune->doSleep = -1;
	pthread_mutex_unlock(&travTune->mutex);
      }
    }

    safe = Fsm_MgrReadUnsatBound(fsmMgr);
    enLogBound = 0;

    if ((i==first0) || ((i >= first && ((i - first) % step) == 0))) {
      int sat1, enCheck = 1;

      if (teSteps>0) {
	Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMin_c,
	  fprintf(tMgrO(travMgr), "%sBMC check at bound %2d (%2d+%2d) - ", 
		  myThrdIdStr, teSteps+i, teSteps, i)
	);
      }
      else {
	Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMin_c,
	  fprintf(tMgrO(travMgr), "%sBMC check at bound %2d - ", 
		  myThrdIdStr, i)
	);
      }

      if (1 && safe > 0 && (i-safe > 10)) {
	Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMin_c) {
	  fprintf(tMgrO(travMgr), "%s - RELEASING CPU (Yield) at BOUND %d\n", 
		  myThrdIdStr, i);
	}
	pthread_yield();
      }
      if (safe > 0 && safe >= i) {
	// skip as already proved
	enCheck=0;
	Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMin_c,
	  fprintf(tMgrO(travMgr), "(already SAFE) - ", 
		  teSteps+i)
	);
      }
      else if (useShared) {
	Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMax_c) {
	  fprintf(tMgrO(travMgr), "%s - READING BOUND %d\n", 
		  myThrdIdStr, i);
	}
	Trav_BmcBoundState_e state = 
	  Trav_TravBmcSharedReadAndSet(i,Trav_BmcBoundActive_c);
	Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMax_c) {
	  fprintf(tMgrO(travMgr), "%s - BOUND %d READ\n", 
		  myThrdIdStr, i);
	}
	if (state == Trav_BmcBoundProved_c) {
	  enCheck=0;
	  Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMin_c,
	     fprintf(tMgrO(travMgr), "(already SAFE) - ", 
	     teSteps+i)
	  );
	}
	if (state == Trav_BmcBoundActive_c) {
	  enCheck=0;
	  Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMin_c,
	     fprintf(tMgrO(travMgr), "(already ACTIVE) - ", 
	     teSteps+i)
	  );
	}
	if (state == Trav_BmcBoundFailed_c) {
	  enCheck=0;
	  Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMin_c,
	     fprintf(tMgrO(travMgr), "(already FAILED) - ", 
	     teSteps+i)
	  );
	  sat = -1;
	  break;
	}
      }
      if (1 && !enCheck) {
     	if (ddiS != NULL) {
	  Ddi_Bdd_t *constr = 
	    Ddi_BddNot(Ddi_BddPartRead(bmcMgr->bmcTarget, i));
	  Ddi_BddSetAig(constr);
	  Ddi_AigSatMinisatLoadClausesIncrementalAsserted(ddiS,
	    constr);
	  Ddi_BddarrayInsertLast(bmcMgr->dummyRefs,constr);
	  Ddi_Free(constr);
	}
      }

      bmcStepStartTime = util_cpu_time();
      Pdtutil_Assert(Ddi_BddPartNum(bmcMgr->bmcTarget) > i,
        "missing bmc target");
      if (!enCheck) {
        sat1 = 0;
      } else {
        Ddi_Bdd_t *target = Ddi_BddPartRead(bmcMgr->bmcTarget, i);
        Ddi_Bdd_t *abstrTarget = Ddi_BddPartRead(bmcMgr->bmcAbstrTarget, i);
	Ddi_Bdd_t *abstrCheckPart = NULL;
#if 0
        Ddi_Bdd_t *checkPart = Ddi_BddDup(target);
#else
        Ddi_Bdd_t *checkPart = Ddi_BddDup(bmcMgr->bmcConstr);
        //	if (1 && Ddi_BddIsOne(bmcMgr->bmcConstr)) { // can be part
	if (Ddi_BddSize(bmcMgr->bmcConstr)==0) {
	  Ddi_Free(checkPart);
	  checkPart = Ddi_AigPartitionTop(target, 1);
	}
	else {
	  Ddi_BddPartInsertLast(checkPart, target);
	}
#endif
	if (abstrTarget != NULL) {
	  abstrCheckPart = Ddi_BddDup(abstrTarget);
	}
        if (0 && bmcMgr->bmcConstrBck!=NULL) {
          if (Ddi_BddPartNum(bmcMgr->bmcConstrBck)>i) {
            Ddi_Bdd_t *bckConstr = Ddi_BddPartRead(bmcMgr->bmcConstrBck,i);
            if (!Ddi_BddIsOne(bckConstr)) {
              Ddi_BddPartInsertLast(checkPart, bckConstr);
            }
          }
        }
	enLogBound = (safe>=first || (first==0));

        if (util_cpu_time() >= time_limit) {
          sat1 = -1;
        } else {
          int abort = 0;
          Ddi_Bdd_t *cex = NULL;
	  float satTimeLimit = -1.0;
	  if (timeLimit > 0) {
	    satTimeLimit = (time_limit - bmcStepStartTime) / 1000.0;
	  }
	  else {
	    satTimeLimit = stepTimeLimit;
	  }
	  if (stepTimeLimit > 0) {
	    if (stepTimeLimit < satTimeLimit) {
	      satTimeLimit = stepTimeLimit;
	    }
	  }

          if (myCexes != NULL) {
            while (!abort && cex == NULL
              && currCexId < Ddi_BddarrayNum(myCexes)
              && Ddi_BddPartNum(Ddi_BddarrayRead(myCexes,
                  currCexId)) == (i + 2)) {
              int jj;
              Ddi_Bdd_t *prev = Ddi_BddarrayRead(myCexes, currCexId);
              Ddi_Bdd_t *myCheck = Ddi_BddDup(checkPart);

              Ddi_BddSetAig(prev);
              currCexId++;
              for (jj = 0; jj < Ddi_BddPartNum(myCheck); jj++) {
                Ddi_Bdd_t *p = Ddi_BddPartRead(myCheck, jj);

                Ddi_AigAndCubeAcc(p, prev);
              }

              if (strategy > 2) {
                /* not incremental */
                cex = Ddi_AigSatWithCexAndAbort(myCheck, NULL,
                  satTimeLimit, &abort);
              } else {
                /* incremental SAT */
                cex = Ddi_AigSatMinisatWithCexAndAbortIncremental(ddiS,
                  myCheck, NULL,
                  satTimeLimit, &abort);
              }

              Ddi_Free(myCheck);
            }
          } else {
            if (strategy > 2) {
              char fileOut[100], *cnfFile = NULL;

              /* not incremental */
              if (strategy > 6) {
                /* store aiger on file */
                Ddi_Bddarray_t *fA = Ddi_BddarrayAlloc(ddm, 1);
                Ddi_Bdd_t *chkAig = Ddi_BddMakeAig(checkPart);

                Ddi_BddarrayWrite(fA, 0, chkAig);
                Ddi_Free(chkAig);
                sprintf(fileOut, "PDT_BMC_%d.aig", i);
                Ddi_AigarrayNetStoreAiger(fA, 0, fileOut);
                printf("BMC instance od size %d written in %s\n",
                  Ddi_BddSize(checkPart), fileOut);
                Ddi_Free(fA);
              }
              if (strategy > 4) {
                /* store cnf on file */
                sprintf(fileOut, "PDT_BMC_%d.cnf", i);
                cnfFile = fileOut;
                printf("CNF instance written in %s\n", fileOut);
              }
              if (nnf && Ddi_BddSize(checkPart) > 1) {
                Ddi_Bdd_t *constr = Ddi_BddMakeConstAig(ddm, 1);
                Ddi_Bdd_t *chkNnf = Ddi_AigNnf(checkPart, NULL, constr,
                  NULL, NULL, NULL);

                Ddi_BddPartInsertLast(constr, chkNnf);
                cex = Ddi_AigSatMinisatWithCexAndAbort(constr, NULL,
                  satTimeLimit, cnfFile, &abort);
                Ddi_Free(constr);
                Ddi_Free(chkNnf);
              } else {
                cex = Ddi_AigSatMinisatWithCexAndAbort(checkPart, NULL,
                  satTimeLimit, cnfFile, &abort);
              }
            } else {
              /* incremental */
              if (useMinisat22) {
		if (Ddi_BddIsPartDisj(checkPart)) {
		  int j;
		  //		  for (j=Ddi_BddPartNum(checkPart)-1; j>=0; j--) {
		  for (j=0; j<Ddi_BddPartNum(checkPart); j++) {
		    Ddi_Bdd_t *p_j = Ddi_BddPartRead(checkPart,j);
		    if (0 && (j==0)) {
		      Ddi_Bdd_t *cex1, *chk = Ddi_BddMakePartConjVoid(ddm);
		      Ddi_Bdd_t *pPart = Ddi_AigPartitionTop(p_j, 1);
		      int jj;
		      for (jj=Ddi_BddPartNum(pPart)-1; jj>0; jj--) {
			Ddi_Bdd_t *cex1, *p_j_j = Ddi_BddPartRead(p_j,jj);
			Ddi_BddPartInsertLast(chk,p_j_j);	
			cex1 = Ddi_AigSatMinisat22WithCexAndAbortIncremental(
			       ddiS,chk, NULL, 0,
			       satTimeLimit, &abort);
			Ddi_Free(cex1);
		      }
		      Ddi_Free(chk);
		    }
		    cex = Ddi_AigSatMinisat22WithCexAndAbortIncremental(
			       ddiS,p_j, NULL, 0,
			       satTimeLimit, &abort);
		    printf(" P:%d ", j); fflush(stdout);
		    if (cex!=NULL) break;
		  }
		}
		else if (bmcMgr->settings.targetByRefinement) {
		  cex = bmcCheckTargetByRefinement(bmcMgr, ddiS, i,
			   checkPart, Fsm_MgrReadVarPS(bmcMgr->fsmMgr),
			   Fsm_MgrReadVarNS(bmcMgr->fsmMgr),
			   satTimeLimit, &abort);
		}
		else {
		  if (incrSolver) {
		    if (incrSolverTimeLimit<5.0) incrSolverTimeLimit = 5.0;
 		    cex = Ddi_AigSatMinisat22WithCexRefinedIncremental(ddiS,
			       checkPart, NULL, 0,
			       incrSolverTimeLimit, &abort);
		  }
		  else {
		    int doFullCheck = 1;
		    if (abstrCheckPart != NULL) {
		      cex = Ddi_AigSatMinisat22WithCexAndAbortIncremental(ddiS,
			       abstrCheckPart, NULL, 0,
			       satTimeLimit, &abort);
		      if (cex!=NULL) {
			Ddi_Free(cex);
		      }
		      else {
			doFullCheck = 0;
			Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMax_c) {
			  fprintf(tMgrO(travMgr), "UNSAT on ABSTR CHECK\n", 
				  myThrdIdStr, i);
			}
		      }
		    }
		    if (doFullCheck)
		      cex = Ddi_AigSatMinisat22WithCexAndAbortIncremental(ddiS,
			       checkPart, NULL, 0,
			       satTimeLimit, &abort);
		  }
		}
              } else {
		cex = Ddi_AigSatMinisatWithCexAndAbortIncremental(ddiS,
			  checkPart, NULL, satTimeLimit, &abort);
              }
            }

          }
          sat1 = 0;
          if (abort) {
            Pdtutil_Assert(!sat1, "SAT abort with cex");
	    stepTimeLimit *= 1.2;
            sat1 = -1;
          } else if (cex != NULL) {

	    if (useShared) {
	      Trav_TravBmcSharedSet(i,Trav_BmcBoundFailed_c);
	    }
            if (doReturnCex) {
              returnedCex = bmcTfSplitCex(bmcMgr, cex, initSteps + i);
            }
            Ddi_Free(cex);
            sat1 = 1;
          }
	  else if (useShared) {
	    Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMax_c) {
	      fprintf(tMgrO(travMgr), "%s - SETTING PROVED %d\n", 
		      myThrdIdStr, i);
	    }
	    Trav_TravBmcSharedSet(i,Trav_BmcBoundProved_c);
	    Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMax_c) {
	      fprintf(tMgrO(travMgr), "%s - PROVED SET %d DONE\n", 
		      myThrdIdStr, i);
	    }
	  }
        }
        Ddi_Free(checkPart);
        Ddi_Free(abstrCheckPart);
      }

      bmcStepTime = (util_cpu_time() - bmcStepStartTime);

      incrSolverTimeLimit = bmcStepTime/4000.0;

      bmcTime = (util_cpu_time() - bmcStartTime);
      Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMin_c) {
        fprintf(tMgrO(travMgr), "%sBMC step %d %s(tot) time: %s ", 
		myThrdIdStr, i,
	   (sat1==-1)? "TIMED_OUT ":"",
           util_print_time(bmcStepTime));
        fprintf(tMgrO(travMgr), "(%s).\n", util_print_time(bmcTime));
	Ddi_IncrSatPrintStats(ddm,ddiS,0);    
      }
#if 1
      Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMin_c) {
        fprintf(tMgrO(travMgr), "Thrd peak AIG : %ld - ", 
		Ddi_MgrReadPeakAigNodesNum(ddm));
        fprintf(tMgrO(travMgr), "Proc RSS : %ld\n", Pdtutil_ProcRss());
      }
#endif

      pthread_mutex_lock(&travStats->mutex);
      if (travStats->arrayStatsNum > 0 &&
        strcmp(Pdtutil_ArrayReadName(travStats->arrayStats[0]),
          "BMCSTATS") == 0) {
        Pdtutil_IntegerArrayInsertLast(travStats->arrayStats[0], bmcStepTime);
      }
      travStats->cpuTimeTot += bmcStepTime;
      travStats->provedBound = i;
      pthread_mutex_unlock(&travStats->mutex);

      if (sat1 == 1) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(tMgrO(travMgr), "BMC failure at bound %d.\n", i)
          );
        provedBound = -i;
        sat = 1;
      }
      if (sat1 == -1) {
        //printf("Invariant verification UNDECIDED after BMC ...\n");
        sat = -1;
      }
      if (sat1 == 0) {
        provedBound = i;
      }
    }

    if (!sat) {
      if (enLogBound) {
	int safeBound = i;
	if (useShared) {
	  safeBound = Trav_TravBmcSharedMaxSafe();
	}
	Fsm_MgrLogUnsatBound(fsmMgr, safeBound,0);
	//			     bmcMgr->settings.unfolded);
      }
    }
  }

  if (strategy <= 2) {
    Ddi_IncrSatMgrQuitKeepDdi(ddiS);
    DdiAig2CnfIdCloseNoCheck(ddm);
  }

  if (!sat) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(tMgrO(travMgr), "BMC pass at bound %d.\n", provedBound)
      );
  }

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(tMgrO(travMgr), "Total SAT time: %s, total BMC time: %s.\n",
      util_print_time(bmcTime), util_print_time(util_cpu_time() - startTime))
    );

  if (sat < 0) {
    sat = -2;
  }

  TravBmcMgrQuit(bmcMgr);

  Trav_MgrSetAssertFlag(travMgr, sat);

  Ddi_Free(myCexes);

  if (returnedCex != NULL) {
    Fsm_MgrSetCexBDD(fsmMgr, returnedCex);
    Ddi_Free(returnedCex);
  }

  return Fsm_MgrReadCexBDD(fsmMgr);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Trav_TravSatBmcIncrVerifFwdBwd(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int bound,
  int step,
  int stepFrames,
  int first,
  int doReturnCex,
  int lemmaSteps,
  int interpolantSteps,
  int strategy,
  int teSteps,
  int timeLimit
)
{
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Varset_t *totPiVars;
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bdd_t *target, *myInit, *myInvarspec; //, *to;
  Ddi_Bdd_t *cubeInit;
  Ddi_Bddarray_t *unroll;
  int sat = 0, i, nsat;
  long startTime, bmcTime = 0, bmcStartTime, bmcStepStartTime, bmcStepTime;
  Ddi_Bdd_t *constrain = NULL;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);
  int provedBound = 0, inverse = 0;
  int doOptThreshold = 5000;
  int optLevel = 0;
  Ddi_Varset_t *psVars = NULL;
  Ddi_Varset_t *nsVars = NULL;
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  unsigned long time_limit = ~0;
  Trav_Stats_t *travStats = &(travMgr->xchg.stats);
  Trav_Tune_t *travTune = &(travMgr->xchg.tune);
  Ddi_Bdd_t *returnedCex = NULL;
  Ddi_Varset_t *tfSupp = NULL;
  Ddi_Bddarray_t *myCexes = NULL;
  int currCexId = -1, currCexLen = -1;
  int safe = -1;
  int doMonotone = 0;
  Solver S;
  Ddi_IncrSatMgr_t *ddiS = NULL;
  int doImpl = Trav_MgrReadImplAbstr(travMgr);
  Ddi_Bdd_t *trAux = NULL;
  TravBmcMgr_t *bmcMgr = NULL;
  int assumeInvarspec = (strategy % 2 == 0);
  int useMinisat22 = 1;
  int unfolded = 0, initSteps = 0;
  int enLogBound=0;
  int targetConeFrames = teSteps;

  //int stepFrames = 4; 
  int stepIncrFactor = 4, stepIncrIter = 4 * 4 * 8;
  Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
  Ddi_Var_t *cvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
  int nnf = 0;

  int first0 = -1;

  if (first > 20) first0 = first/3;


  //  stepFrames = 1;

  startTime = util_cpu_time();
  if (timeLimit > 0) {
    time_limit = util_cpu_time() + timeLimit * 1000;
  }
  if (stepFrames <= 0) {
    stepFrames = 1;             //default is 4!
  }
  if (bound < 0) {
    bound = 1000000;
  }


#if 0
  if (strategy >= 100) {
    strategy -= 100;
    nnf = 1;
  }
#endif
  if (strategy>100) {
    strategy -= 100;
    useMinisat22 = -1; // LGL
  }


  travStats->cpuTimeTot = 0;

  bmcMgr = TravBmcMgrInit(travMgr, fsmMgr, delta, init, initStub, invarspec,
    invar, NULL, NULL);
  bmcMgr->settings.fwdBwdRatio = 60;
  bmcMgr->settings.stepFrames = stepFrames; 

  if (targetConeFrames>0) {
    TravBmcMgrAddTargetCone(bmcMgr,targetConeFrames);
  }

  optLevel = Ddi_MgrReadAigBddOptLevel(ddm);

  if (strategy <= 2) {
    DdiAig2CnfIdInit(ddm);
    ddiS = Ddi_IncrSatMgrAlloc(NULL,useMinisat22, 0, 0);
  }

  bmcStartTime = util_cpu_time();

  initSteps = Ddi_VarsetarrayNum(bmcMgr->timeFrameVars);

  for (i = nsat = 0; i <= bound && !sat; i++) {

    if (i == stepIncrIter) {
      bmcMgr->settings.stepFrames *= stepIncrFactor;
    }

    TravBmcMgrAddFramesFwdBwd(bmcMgr, ddiS, i);

    if (Ddi_BddPartNum(bmcMgr->bmcConstr) > i) {
      Ddi_Bdd_t *constr_i = Ddi_BddPartRead(bmcMgr->bmcConstr, i);

      if (ddiS != NULL) {
        Ddi_AigSatMinisatLoadClausesIncrementalAsserted(ddiS,
          bmcMgr->bmcConstr);
      }
    }

    if (myCexes != NULL && currCexId >= Ddi_BddarrayNum(myCexes)) {
      sat = -1;
      break;
    }

    if (util_cpu_time() > time_limit) {
      //printf("Invariant verification UNDECIDED after BMC ...\n");
      sat = -1;
      break;
    }

    if (travTune!=NULL) {
      int doSleep;
      pthread_mutex_lock(&travTune->mutex);
      doSleep = travTune->doSleep;
      pthread_mutex_unlock(&travTune->mutex);
      if (doSleep>0) {
	sleep(doSleep);
	pthread_mutex_lock(&travTune->mutex);
	travTune->doSleep = -1;
	pthread_mutex_unlock(&travTune->mutex);
      }
    }

    safe = Fsm_MgrReadUnsatBound(fsmMgr);
    enLogBound = 0;

    if ((i==first0) || ((i >= first && ((i - first) % step) == 0))) {
      int sat1, enCheck = 1;

      if (teSteps>0) {
	Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMin_c,
	  fprintf(tMgrO(travMgr), "FB BMC check at bound %2d (%2d+%2d) - ", 
		  teSteps+i, teSteps, i)
	);
      }
      else {
	Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMin_c,
	  fprintf(tMgrO(travMgr), "FB BMC check at bound %2d - ", i)
	);
      }

      if (safe > 0 && safe >= i) {
	// skip as already proved
	enCheck=0;
	Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMin_c,
	  fprintf(tMgrO(travMgr), "(already SAFE) - ", 
		  teSteps+i)
	);
      }

      bmcStepStartTime = util_cpu_time();
      Pdtutil_Assert(Ddi_BddPartNum(bmcMgr->bmcTarget) > i,
        "missing bmc target");
      if (!enCheck) {
        sat1 = 0;
      } else {
        Ddi_Bdd_t *target = Ddi_BddPartRead(bmcMgr->bmcTarget, i);

        Ddi_Bdd_t *checkPart = Ddi_BddDup(bmcMgr->bmcConstr);
        Ddi_BddPartInsertLast(checkPart, target);

	enLogBound = (safe>=first || (first==0));

        if (util_cpu_time() >= time_limit) {
          sat1 = -1;
        } else {
          int abort = 0;
          Ddi_Bdd_t *cex = NULL;

	  if (strategy > 2) {
	    char fileOut[100], *cnfFile = NULL;

	    /* not incremental */
	    if (strategy > 6) {
	      /* store aiger on file */
	      Ddi_Bddarray_t *fA = Ddi_BddarrayAlloc(ddm, 1);
	      Ddi_Bdd_t *chkAig = Ddi_BddMakeAig(checkPart);
	      
	      Ddi_BddarrayWrite(fA, 0, chkAig);
	      Ddi_Free(chkAig);
	      sprintf(fileOut, "PDT_BMC_%d.aig", i);
	      Ddi_AigarrayNetStoreAiger(fA, 0, fileOut);
	      printf("BMC instance od size %d written in %s\n",
		     Ddi_BddSize(checkPart), fileOut);
	      Ddi_Free(fA);
              }
	    if (strategy > 4) {
	      /* store cnf on file */
	      sprintf(fileOut, "PDT_BMC_%d.cnf", i);
	      cnfFile = fileOut;
	      printf("CNF instance written in %s\n", fileOut);
	    }
	    if (nnf && Ddi_BddSize(checkPart) > 1) {
	      Ddi_Bdd_t *constr = Ddi_BddMakeConstAig(ddm, 1);
	      Ddi_Bdd_t *chkNnf = Ddi_AigNnf(checkPart, NULL, constr,
					     NULL, NULL, NULL);

	      Ddi_BddPartInsertLast(constr, chkNnf);
	      cex = Ddi_AigSatMinisatWithCexAndAbort(constr, NULL,
			(time_limit - util_cpu_time()) / 1000.0, cnfFile, &abort);
	      Ddi_Free(constr);
	      Ddi_Free(chkNnf);
	    } else {
	      cex = Ddi_AigSatMinisatWithCexAndAbort(checkPart, NULL,
		        (time_limit - util_cpu_time()) / 1000.0, cnfFile, &abort);
	    }
	  } else {
	    /* incremental */
	    if (useMinisat22) {
	      cex = Ddi_AigSatMinisat22WithCexAndAbortIncremental(ddiS,
			checkPart, NULL, 0,
			(time_limit - util_cpu_time()) / 1000.0, &abort);
	    } else {
	      cex = Ddi_AigSatMinisatWithCexAndAbortIncremental(ddiS,
			checkPart, NULL,
                        (time_limit - util_cpu_time()) / 1000.0, &abort);
	    }
	  }

          sat1 = 0;
          if (abort) {
            Pdtutil_Assert(!sat1, "SAT abort with cex");
            sat1 = -1;
            DdiAig2CnfIdCloseNoCheck(ddm);
          } else if (cex != NULL) {
            DdiAig2CnfIdCloseNoCheck(ddm);
            if (doReturnCex) {
	      if (bmcMgr->settings.bwdStep>0) {
		int j;
                Ddi_Varset_t *smooth = NULL;
		Ddi_Varsetarray_t *btf = bmcMgr->bwdTimeFrameVars;
		Ddi_Varsetarray_t *ftf = bmcMgr->timeFrameVars;
		Pdtutil_Assert(Ddi_VarsetarrayNum(btf)==bmcMgr->settings.bwdStep,
			       "wrong bwd tf num");
                if (bmcMgr->bwd.lastFwd==1) {
                  Ddi_VarsetarrayRemove(btf,Ddi_VarsetarrayNum(btf)-1);
                }
                else if (bmcMgr->bwd.lastFwd==0) {
                  Ddi_VarsetarrayRemove(ftf,Ddi_VarsetarrayNum(ftf)-1);
                }
		for (j=Ddi_VarsetarrayNum(btf)-1;j>=0;j--) {
		  Ddi_Varset_t *vs_j = Ddi_VarsetarrayRead(btf,j);
		  Ddi_VarsetarrayInsertLast(bmcMgr->timeFrameVars, vs_j);
		}
                if (0) {
		  Ddi_VarsetarrayInsertLast(bmcMgr->timeFrameVars, NULL);
                }
                smooth = Ddi_VarsetMakeFromArray(bmcMgr->bwd.cutVarsAllFrames);
                //                Ddi_BddExistAcc(cex,smooth);
                Ddi_Free(smooth);
	      }
              returnedCex = bmcTfSplitCex(bmcMgr, cex, initSteps + i);
            }
            Ddi_Free(cex);
            sat1 = 1;
          }
        }
        Ddi_Free(checkPart);
      }

      bmcStepTime = (util_cpu_time() - bmcStepStartTime);
      bmcTime = (util_cpu_time() - bmcStartTime);
      Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMin_c) {
        fprintf(tMgrO(travMgr), "BMC step (tot) time: %s ",
          util_print_time(bmcStepTime));
        fprintf(tMgrO(travMgr), "(%s).\n", util_print_time(bmcTime));
	Ddi_IncrSatPrintStats(ddm,ddiS,0);    
      }
#if 0
      Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMin_c) {
        fprintf(tMgrO(travMgr), "Thrd RSS : %ld - ", Pdtutil_ThrdRss());
        fprintf(tMgrO(travMgr), "Proc RSS : %ld\n", Pdtutil_ProcRss());
      }
#endif

      pthread_mutex_lock(&travStats->mutex);
      if (travStats->arrayStatsNum > 0 &&
        strcmp(Pdtutil_ArrayReadName(travStats->arrayStats[0]),
          "BMCSTATS") == 0) {
        Pdtutil_IntegerArrayInsertLast(travStats->arrayStats[0], bmcStepTime);
      }
      travStats->cpuTimeTot += bmcStepTime;
      travStats->provedBound = i;
      pthread_mutex_unlock(&travStats->mutex);

      if (sat1 == 1) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(tMgrO(travMgr), "BMC failure at bound %d.\n", i)
          );
        provedBound = -i;
        sat = 1;
      }
      if (sat1 == -1) {
        //printf("Invariant verification UNDECIDED after BMC ...\n");
        sat = -1;
      }
      if (sat1 == 0) {
        provedBound = i;
      }
    }

    if (!sat) {
      int safeBound = i;
      if (enLogBound) {
	Fsm_MgrLogUnsatBound(fsmMgr, safeBound,0);
	//			     bmcMgr->settings.unfolded);
      }
    }
  }

  if (strategy <= 2) {
    Ddi_IncrSatMgrQuit(ddiS);
    DdiAig2CnfIdCloseNoCheck(ddm);
  }

  if (!sat) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(tMgrO(travMgr), "BMC pass at bound %d.\n", provedBound)
      );
  }

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(tMgrO(travMgr), "Total SAT time: %s, total BMC time: %s.\n",
      util_print_time(bmcTime), util_print_time(util_cpu_time() - startTime))
    );

  if (sat < 0) {
    sat = -2;
  }

  TravBmcMgrQuit(bmcMgr);

  Trav_MgrSetAssertFlag(travMgr, sat);

  Ddi_Free(myCexes);

  if (returnedCex != NULL) {
    Fsm_MgrSetCexBDD(fsmMgr, returnedCex);
    Ddi_Free(returnedCex);
  }

  return Fsm_MgrReadCexBDD(fsmMgr);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Trav_TravSatBmcRefineCex(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * cex
)
{
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Varset_t *totPiVars;
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bdd_t *target, *myInit, *myInvarspec; //, *to;
  Ddi_Bdd_t *cubeInit;
  Ddi_Bddarray_t *unroll;
  int sat = 0, i, nsat;
  long startTime, bmcTime = 0, bmcStartTime, bmcStepStartTime, bmcStepTime;
  Ddi_Bdd_t *constrain = NULL;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);
  int provedBound = 0, inverse = 0;
  int doOptThreshold = 5000;
  int optLevel = 0;
  Ddi_Varset_t *psVars = NULL;
  Ddi_Varset_t *nsVars = NULL;
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  unsigned long time_limit = ~0;
  Ddi_Bdd_t *partConstr = Ddi_BddMakePartConjVoid(ddm);
  Ddi_Bdd_t *returnedCex = NULL;
  Ddi_Varset_t *tfSupp = NULL;
  Ddi_Bddarray_t *myCexes = NULL;
  int currCexId = -1, currCexLen = -1;
  int safe = 0;
  int doMonotone = 0;
  Solver S;
  Ddi_IncrSatMgr_t *ddiS = NULL;
  Ddi_Bdd_t *trAux = NULL;
  Ddi_Vararray_t *constrainVarsPs = NULL;
  Ddi_Bddarray_t *constrainSubstLitsPs = NULL;
  TravBmcMgr_t *bmcMgr = NULL;
  int useMinisat22 = 1;
  int unfold = Fsm_MgrReadIFoldedProp(fsmMgr)>=0;
  int stepFrames = -1;
  Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
  Ddi_Var_t *cvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");

  int iCex = 0;

  startTime = util_cpu_time();


  bmcMgr = TravBmcMgrInit(NULL, fsmMgr, delta, init, initStub, invarspec,
    invar, NULL, cex);


  ps = Fsm_MgrReadVarPS(fsmMgr);
  pi = Fsm_MgrReadVarI(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);

  myInvarspec = Ddi_BddDup(invarspec);
  myInit = Ddi_BddDup(init);
  //  target = Ddi_BddNot(myInvarspec);

  // check if reference fsm is folded and unfold - INTEL SAT circuits.
  if (unfold) {
    Ddi_BddComposeAcc(bmcMgr->target,ps,delta);
  }

  if (initStub != NULL) {
    Ddi_Bdd_t *cex_i = Ddi_BddPartRead(cex, iCex++);

    unroll = Ddi_BddarrayDup(initStub);
    Ddi_AigarrayConstrainCubeAcc(unroll, cex_i);
    tfSupp = Ddi_BddarraySupp(initStub);
  } else {
    Ddi_Bdd_t *initLits = Ddi_AigPartitionTop(myInit, 0);
    int j, nv = Ddi_BddPartNum(initLits);
    Ddi_Vararray_t *vA = Ddi_VararrayAlloc(ddm, 0);
    Ddi_Bddarray_t *subst = Ddi_BddarrayAlloc(ddm, 0);
    Ddi_Bdd_t *myOne = Ddi_BddMakeConstAig(ddm, 1);
    Ddi_Bdd_t *myZero = Ddi_BddMakeConstAig(ddm, 0);

    // removed because added extra TF in cex
    //    tfSupp = Ddi_VarsetVoid(ddm); 
    for (j = 0; j < nv; j++) {
      Ddi_Bdd_t *l_j = Ddi_BddPartRead(initLits, j);

      if (Ddi_BddSize(l_j) == 1) {
        /* literal */
        Ddi_Var_t *v = Ddi_BddTopVar(l_j);

        Ddi_VararrayWrite(vA, j, v);
        if (Ddi_BddIsComplement(l_j)) {
          Ddi_BddarrayWrite(subst, j, myZero);
        } else {
          Ddi_BddarrayWrite(subst, j, myOne);
        }
      } else {
        /* not a literal */
        Ddi_BddPartInsertLast(partConstr, l_j);
      }
    }
    Ddi_Free(myZero);
    Ddi_Free(myOne);
    unroll = Ddi_BddarrayMakeLiteralsAig(ps, 1);

    if (Ddi_VararrayNum(vA) > 0) {
      Ddi_BddarrayComposeAcc(unroll, vA, subst);
    }

    Ddi_Free(initLits);
    Ddi_Free(vA);
    Ddi_Free(subst);
  }


  int end_k, start_k = bmcMgr->nTimeFrames;

  stepFrames = Ddi_BddPartNum(cex);
  Pdtutil_Assert(stepFrames>=0,"missing cex");

  TravBmcMgrAddFrames(bmcMgr, ddiS, 0, cex);

  Pdtutil_Assert(Ddi_BddPartNum(bmcMgr->bmcTarget) == 1,
    "single target needed in cex refine");
  target = Ddi_BddPartRead(bmcMgr->bmcTarget, 0);
  Ddi_Bdd_t *check = Ddi_BddDup(target);
  Ddi_Bdd_t *checkPart = Ddi_BddDup(bmcMgr->bmcConstr);

  Ddi_BddPartInsertLast(checkPart, check);
  Ddi_Free(check);

  Ddi_Bdd_t *cex2 = Ddi_AigSatWithCexAndAbort(checkPart, NULL, -1, NULL);
  Ddi_Free(checkPart);

  Ddi_Bdd_t *splitCex = bmcTfSplitCex(bmcMgr, cex2, stepFrames);
  Ddi_Free(cex2);

  Pdtutil_Assert(Ddi_BddPartNum(splitCex) == Ddi_BddPartNum(cex),
    "misaligned refined cex");

  for (i = 0; i < Ddi_BddPartNum(cex); i++) {
    Ddi_Bdd_t *cex_i = Ddi_BddPartRead(cex, i);
    Ddi_Bdd_t *split_i = Ddi_BddPartRead(splitCex, i);

    Ddi_BddAndAcc(cex_i, split_i);
  }

  TravBmcMgrQuit(bmcMgr);

  Ddi_Free(splitCex);
  Ddi_Free(partConstr);

  Ddi_Free(constrainVarsPs);
  Ddi_Free(constrainSubstLitsPs);

  Ddi_Free(constrain);
  Ddi_Free(myInvarspec);
  Ddi_Free(myInit);
  Ddi_Free(unroll);
  //  Ddi_Free(target);


  return Fsm_MgrReadCexBDD(fsmMgr);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
TravBmcMgr_t *
TravBmcMgrInit(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * delta,
  Ddi_Bdd_t * init,
  Ddi_Bddarray_t * initStub,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * cubeConstr
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(invarspec);
  TravBmcMgr_t *bmcMgr;
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
  Ddi_Var_t *cvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
  int useBmcInvar = 0;
  int useSccs = 0;

  //Allocate and initialize bmc manager
  bmcMgr = Pdtutil_Alloc(TravBmcMgr_t, 1);

  bmcMgr->fsmMgr = fsmMgr;
  bmcMgr->travMgr = travMgr;

  bmcMgr->xMgr = NULL;

  bmcMgr->bmcTargetCone = NULL;
  bmcMgr->bmcConstrCone = NULL;
  bmcMgr->bmcTarget = NULL;
  bmcMgr->bmcAbstrTarget = NULL;
  bmcMgr->bmcConstr = NULL;
  bmcMgr->bmcInvar = NULL;
  bmcMgr->bmcConstrBck = NULL;
  bmcMgr->unroll = NULL;
  bmcMgr->delta = NULL;
  bmcMgr->target = NULL;
  bmcMgr->invarspec = NULL;
  bmcMgr->constraint = NULL;
  bmcMgr->initStubConstr = NULL;
  bmcMgr->init = NULL;
  bmcMgr->initStub = NULL;
  bmcMgr->itpRings = NULL;
  bmcMgr->trAbstrItp = NULL;
  bmcMgr->dummyRefs = NULL;
  bmcMgr->unrollArray = NULL;

  bmcMgr->itpRingsPeriod = 0;
  bmcMgr->trAbstrItpNumFrames = 0;
  bmcMgr->trAbstrItpPeriod = 0;
  bmcMgr->trAbstrItpCheck = 0;

  bmcMgr->inCoreScc = NULL;

  bmcMgr->xsim.xMgr = NULL;
  bmcMgr->xsim.constLatchArray = NULL;
  bmcMgr->xsim.bound = -1;

  bmcMgr->nTimeFrames = 0;
  bmcMgr->maxTimeFrames = 0;

  bmcMgr->eqCl = NULL;
  bmcMgr->part.psVars = NULL;
  bmcMgr->part.cutVars = NULL;
  bmcMgr->part.window = NULL;

  bmcMgr->bwd.cutVars = NULL;
  bmcMgr->bwd.cutVarsTot = NULL;
  bmcMgr->bwd.cutVarsAllFrames = NULL;
  bmcMgr->bwd.lastFwd = -1;
  bmcMgr->bwd.rings = NULL;

  bmcMgr->stats.nSatSolverCalls = 0;
  bmcMgr->stats.nUnsat = 0;
  bmcMgr->stats.nTernarySim = 0;
  bmcMgr->stats.nTernaryInit = 0;
  bmcMgr->stats.nBlocked = 0;
  bmcMgr->stats.nBlocks = 0;
  bmcMgr->stats.solverRestart = 0;
  bmcMgr->stats.nConfl = 0;
  bmcMgr->stats.nDec = 0;
  bmcMgr->stats.nProp = 0;

  bmcMgr->settings.strategy = 2;
  bmcMgr->settings.step = 0;
  bmcMgr->settings.bwdStep = 0;
  // zeroed to fix intel045.
  //  bmcMgr->settings.unfolded = (invar==NULL || Ddi_BddIsOne(invar)) && !Fsm_MgrReadRemovedPis(fsmMgr);  
  bmcMgr->settings.unfolded = 1;
  bmcMgr->settings.memLimit = -1;
  bmcMgr->settings.timeLimit = -1;
  bmcMgr->settings.nFramesLimit = -1;
  bmcMgr->settings.stepFrames = 4;
  bmcMgr->settings.incrementalBmc = 1;
  bmcMgr->settings.fwdBwdRatio = 0;
  bmcMgr->settings.shiftFwdFrames = 0;
  bmcMgr->settings.enBwd = 0;
  bmcMgr->settings.invarMaxBound = -1;
  bmcMgr->settings.useInvar = 0;
  bmcMgr->settings.verbosity = Pdtutil_VerbLevelNone_c;
  bmcMgr->settings.targetByRefinement = 0;

  bmcMgr->bmcTarget = Ddi_BddMakePartDisjVoid(ddm);
  bmcMgr->bmcAbstrTarget = Ddi_BddMakePartDisjVoid(ddm);
  bmcMgr->bmcConstr = Ddi_BddMakePartConjVoid(ddm);
  bmcMgr->bmcConstrBck = Ddi_BddMakePartConjVoid(ddm);
  bmcMgr->delta = Ddi_BddarrayDup(delta);
  bmcMgr->target = Ddi_BddNot(invarspec);
  bmcMgr->invarspec = Ddi_BddDup(invarspec);
  bmcMgr->dummyRefs = Ddi_BddarrayAlloc(ddm, 0);

#if 0
  // disabled as backward retimed by one step
  // constraint is already included in invarspec
  // see testConstr.aig
  if (invar != NULL && !Ddi_BddIsOne(invar))
    bmcMgr->constraint = Ddi_BddDup(invar);
#endif
  bmcMgr->init = NULL;
  bmcMgr->initStub = NULL;
  bmcMgr->timeFrameVars = Ddi_VarsetarrayAlloc(ddm, 0);
  bmcMgr->bwdTimeFrameVars = Ddi_VarsetarrayAlloc(ddm, 0);

  bmcMgr->unrollArray = Pdtutil_PointerArrayAlloc(1);

  if (travMgr!=NULL && travMgr->settings.bdd.hints!=NULL) {
    Ddi_Bddarray_t *h = travMgr->settings.bdd.hints;
    int i, j, nv = Ddi_BddarrayNum(h);
    bmcMgr->part.psVars = Ddi_VararrayAlloc(ddm, nv);
    bmcMgr->part.cutVars = Ddi_VararrayAlloc(ddm, nv);

    bmcMgr->part.window = Ddi_BddMakeConstAig(ddm, 1);
    Ddi_BddSetPartDisj(bmcMgr->part.window);
    Pdtutil_Assert(Ddi_BddPartNum(bmcMgr->part.window)==1,
                   "wrong dpart num");
    printf("hints FOUND: ");
    for (i=0; i<nv; i++) {
      Ddi_Bdd_t *lit = Ddi_BddarrayRead(h,i);
      Ddi_Var_t *v = Ddi_BddTopVar(lit);
      printf("%s%s ", Ddi_BddIsComplement(lit)?"!":"", 
             Ddi_VarName(v));
      Ddi_VararrayWrite(bmcMgr->part.psVars,i,v);
    }
    printf("\n");
    bmcMgr->part.cutVars = Ddi_VararrayMakeNewVars(
      bmcMgr->part.psVars, "PDT_BMC_PART_PS", "0", 1);
    for (i=0; i<nv; i++) {
      Ddi_Bdd_t *lit = Ddi_BddarrayRead(h,i);
      Ddi_Bdd_t *litNeg = Ddi_BddNot(lit);
      Ddi_Bdd_t *wDup = Ddi_BddDup(bmcMgr->part.window);
      for (j=0; j<Ddi_BddPartNum(wDup); j++) {
        Ddi_Bdd_t *p_j = Ddi_BddPartRead(bmcMgr->part.window,j);
        Ddi_Bdd_t *pN_j = Ddi_BddPartRead(wDup,j);
        Ddi_BddAndAcc(p_j,lit);
        Ddi_BddAndAcc(pN_j,litNeg);
      }
      Ddi_Free(litNeg);
      for (j=0; j<Ddi_BddPartNum(wDup); j++) {
        Ddi_Bdd_t *pN_j = Ddi_BddPartRead(wDup,j);
        Ddi_BddPartInsertLast(bmcMgr->part.window,pN_j);
      }
    }
    Ddi_BddSubstVarsAcc(bmcMgr->part.window,
                        bmcMgr->part.psVars,
                        bmcMgr->part.cutVars);
  }

  if (bmcMgr->settings.unfolded) {
    int iProp = Ddi_BddarrayNum(bmcMgr->delta) - 1;
    Ddi_Bdd_t *deltaProp = Ddi_BddarrayRead(bmcMgr->delta, iProp);

    if (1 && invar != NULL && !Ddi_BddIsOne(invar)) {
      bmcMgr->constraint = Ddi_BddDup(invar);
    }
    else {
      Ddi_BddCofactorAcc(bmcMgr->target, cvarPs, 1);
      Ddi_BddCofactorAcc(bmcMgr->invarspec, cvarPs, 1);
    }
    // constrain on initStub...
    if (1 && bmcMgr->constraint != NULL)
      Ddi_BddCofactorAcc(bmcMgr->constraint, cvarPs, 1);
    Ddi_BddCofactorAcc(deltaProp, pvarPs, 1);
    Ddi_BddCofactorAcc(deltaProp, cvarPs, 1);
  }
  else {
    int iProp = Ddi_BddarrayNum(bmcMgr->delta) - 1;
    Ddi_Bdd_t *deltaProp = Ddi_BddarrayRead(bmcMgr->delta, iProp);
    Ddi_BddCofactorAcc(deltaProp, pvarPs, 1);
  }

  if (initStub != NULL) {
    Ddi_Varset_t *tfSupp;

    bmcMgr->initStub = Ddi_BddarrayDup(initStub);
    bmcMgr->unroll = Ddi_BddarrayDup(initStub);

    if (cubeConstr != NULL) {
      Ddi_Bdd_t *cex_i = Ddi_BddPartRead(cubeConstr, 0);

      Ddi_AigarrayConstrainCubeAcc(bmcMgr->unroll, cex_i);
    }

    tfSupp = Ddi_BddarraySupp(bmcMgr->unroll);
    Ddi_VarsetarrayWrite(bmcMgr->timeFrameVars, 0, tfSupp);
    Ddi_Free(tfSupp);
    if (1) {
      Ddi_Var_t *cVar = Fsm_MgrReadPdtConstrVar(fsmMgr);
      bmcMgr->initStubConstr = Ddi_BddMakeLiteralAig(cVar, 1);
      Ddi_BddComposeAcc(bmcMgr->initStubConstr,ps,initStub);
      if (Ddi_BddIsOne(bmcMgr->initStubConstr)) {
	Ddi_Free(bmcMgr->initStubConstr);
      }
    }
  } else {
    Ddi_Bdd_t *initLits = Ddi_AigPartitionTop(init, 0);
    int j, nv = Ddi_BddPartNum(initLits);
    Ddi_Vararray_t *vA = Ddi_VararrayAlloc(ddm, 0);
    Ddi_Bddarray_t *subst = Ddi_BddarrayAlloc(ddm, 0);
    Ddi_Bdd_t *myOne = Ddi_BddMakeConstAig(ddm, 1);
    Ddi_Bdd_t *myZero = Ddi_BddMakeConstAig(ddm, 0);
    Ddi_Bdd_t *initConstr = Ddi_BddMakeConstAig(ddm, 1);

    bmcMgr->init = Ddi_BddDup(init);

    for (j = 0; j < nv; j++) {
      Ddi_Bdd_t *l_j = Ddi_BddPartRead(initLits, j);

      if (Ddi_BddSize(l_j) == 1) {
        /* literal */
        Ddi_Var_t *v = Ddi_BddTopVar(l_j);

        Ddi_VararrayInsertLast(vA, v);
        if (Ddi_BddIsComplement(l_j)) {
          Ddi_BddarrayInsertLast(subst, myZero);
        } else {
          Ddi_BddarrayInsertLast(subst, myOne);
        }
      } else {
        /* not a literal */
        Ddi_BddAndAcc(initConstr, l_j);
      }
    }
    if (!Ddi_BddIsOne(initConstr)) {
      Pdtutil_Assert(Ddi_BddPartNum(bmcMgr->bmcConstr) == 0,
        "error setting init constr");
      Ddi_BddPartInsertLast(bmcMgr->bmcConstr, initConstr);
    }
    Ddi_Free(initConstr);
    Ddi_Free(myZero);
    Ddi_Free(myOne);
    bmcMgr->unroll = Ddi_BddarrayMakeLiteralsAig(ps, 1);

    if (Ddi_VararrayNum(vA) > 0) {
      Ddi_BddarrayComposeAcc(bmcMgr->unroll, vA, subst);
    }

    Ddi_Free(initLits);
    Ddi_Free(vA);
    Ddi_Free(subst);
  }


  char *trConstrLoad = travMgr->settings.aig.trAbstrItpLoad;
  int trConstrNumFrames = 4;
  if (trConstrLoad!=NULL) {
    Ddi_Bddarray_t *trConstrA;
    trConstrA = TravTravTrAbstrLoad(travMgr,&trConstrNumFrames);
    Pdtutil_Assert(trConstrA != NULL  ,"Unexpected NULL pointer");

    Ddi_Free(bmcMgr->trAbstrItp);
    int nc = Ddi_BddarrayNum(trConstrA);
    bmcMgr->trAbstrItp = Ddi_BddMakePartConjFromArray(trConstrA);

    Ddi_BddSetAig(bmcMgr->trAbstrItp);

    bmcMgr->trAbstrItpNumFrames = trConstrNumFrames;

    bmcMgr->trAbstrItpPeriod = travMgr->settings.aig.bmcTrAbstrPeriod;
    bmcMgr->trAbstrItpInit = travMgr->settings.aig.bmcTrAbstrInit;
    if (bmcMgr->trAbstrItpInit<trConstrNumFrames)
      bmcMgr->trAbstrItpInit += trConstrNumFrames;
    bmcMgr->trAbstrItpCheck = 0;
    if (bmcMgr->trAbstrItpPeriod==0)
      bmcMgr->trAbstrItpPeriod = trConstrNumFrames;
    else if (bmcMgr->trAbstrItpPeriod<0) {
      bmcMgr->trAbstrItpCheck = 1;
      bmcMgr->trAbstrItpPeriod *= -1;
    }
    Pdtutil_VerbosityMgrIf(bmcMgr->travMgr, Pdtutil_VerbLevelUsrMed_c) {
      fprintf(dMgrO(ddm), "Loaded %d constraints - size: %d - frames: %d - period: %d\n",
              Ddi_BddarrayNum(trConstrA), Ddi_BddarraySize(trConstrA),
              trConstrNumFrames, bmcMgr->trAbstrItpPeriod);
      Ddi_Vararray_t *supp = Ddi_BddarraySuppVararray(trConstrA);
      int n = Ddi_VararrayNum(supp);
      Ddi_Vararray_t *psupp = Ddi_VararrayIntersect(supp,ps);
      Ddi_VararrayIntersectAcc(supp,ns);
      fprintf(dMgrO(ddm), "TR constraint has %d->%d (%d) ps->ns (pi) vars\n",
              Ddi_VararrayNum(psupp), Ddi_VararrayNum(supp),
              n-(Ddi_VararrayNum(psupp)+Ddi_VararrayNum(supp)));
      Ddi_Free(supp);      
      Ddi_Free(psupp);      
    }
    Ddi_Free(trConstrA);
    
      
  }
  
  if (ddm->settings.aig.itpLoad != NULL) {
    Ddi_Bddarray_t *itpRings;
    char filename[100];
    int nRings;

    itpRings = Ddi_AigarrayNetLoadAiger(ddm,NULL,ddm->settings.aig.itpLoad);
    Pdtutil_Assert( itpRings != NULL  ,"Unexpected NULL pointer");

    bmcMgr->itpRings = itpRings;
    bmcMgr->itpRingsPeriod = travMgr->settings.aig.bmcItpRingsPeriod;
  }

  if (0) {
    Ddi_Bdd_t *c = Ddi_BddMakeFromBaig(ddm, 55421);
    Ddi_BddNotAcc(c);
    if (bmcMgr->constraint==NULL) {
      bmcMgr->constraint = Ddi_BddDup(c);
    }
    else {
      Ddi_BddAndAcc(bmcMgr->constraint,c);
    }
    Ddi_Free(c);
    //    Ddi_FindIte(bmcMgr->delta, ps);
  }

  if (useBmcInvar) {
    Ddi_Bdd_t *myInvar = Ddi_BddNot(bmcMgr->target);
    Ddi_BddComposeAcc(myInvar,ps,bmcMgr->delta);
    Ddi_Bdd_t *pPart = Ddi_AigConjDecomp (myInvar,useBmcInvar,0);
    Ddi_Free(myInvar);

    bmcMgr->settings.invarMaxBound = 10000;
    bmcMgr->settings.useInvar = 1;

    bmcMgr->bmcInvar = Ddi_BddMakeAig(Ddi_BddPartRead(pPart,7));
    Ddi_Free(pPart);
  }

  if (useSccs) {

    bAig_Manager_t *bmgr = ddm->aig.mgr;
    Ddi_SccMgr_t *sccMgr = Ddi_FsmSccTarjan(bmcMgr->delta, NULL, ps);
    int maxScc=-1, maxSccBit=-1;
    int iScc, bit, n;

    int level, maxl = -1;
    int i, j, ni = Ddi_VararrayNum(pi);
    char *singleLatchScc = Pdtutil_Alloc(char,sccMgr->nScc);
    int singleLatchFilter = 2;
    for (i = 0; i < sccMgr->nScc; i++) {
      if (sccMgr->sccLatchCnt[i]==0) continue;
      int bit = sccMgr->mapSccBit[i];
      Pdtutil_Assert(bit>=0 && bit<sccMgr->nScc,"wrong scc bit");
      if (singleLatchFilter>1)
	singleLatchScc[bit] = (char)(sccMgr->sccLatchCnt[i] <= 1);
      else {
	singleLatchScc[bit] = (char)(sccMgr->sccLatchCnt[i] <= 1);
	singleLatchScc[bit] &= (char)(sccMgr->sccGateCnt[i] <= 1);
      }
    }

    bmcMgr->inCoreScc = Ddi_VararrayAlloc(ddm,0);

    for (j = 0; j < sccMgr->nLatches; j++) {
      Ddi_Var_t *psV = Ddi_VararrayRead(ps, j);
      int bit = sccMgr->mapLatches[j];
      if (strncmp("PDT_BDD_",Ddi_VarName(psV),8)==0) {
	Ddi_VararrayInsertLast(bmcMgr->inCoreScc,psV);
      }
      else {
	Pdtutil_Assert(bit>=0 && bit<sccMgr->nScc,"wrong scc bit");
	if (!singleLatchScc[bit]) 
	  Ddi_VararrayInsertLast(bmcMgr->inCoreScc,psV);
      }
    }
      
    Pdtutil_Free(singleLatchScc);
    Ddi_SccMgrFree(sccMgr);
  }

  return bmcMgr;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
TravBmcMgrQuit(
  TravBmcMgr_t * bmcMgr
)
{
  Ddi_Free(bmcMgr->bmcTarget);
  Ddi_Free(bmcMgr->bmcAbstrTarget);
  Ddi_Free(bmcMgr->bmcTargetCone);
  Ddi_Free(bmcMgr->bmcConstrCone);
  Ddi_Free(bmcMgr->bmcConstr);
  Ddi_Free(bmcMgr->bmcInvar);
  Ddi_Free(bmcMgr->bmcConstrBck);
  Ddi_Free(bmcMgr->unroll);
  Ddi_Free(bmcMgr->delta);
  Ddi_Free(bmcMgr->target);
  Ddi_Free(bmcMgr->invarspec);
  Ddi_Free(bmcMgr->constraint);
  Ddi_Free(bmcMgr->initStubConstr);
  Ddi_Free(bmcMgr->init);
  Ddi_Free(bmcMgr->initStub);
  Ddi_Free(bmcMgr->itpRings);
  Ddi_Free(bmcMgr->trAbstrItp);
  Ddi_Free(bmcMgr->dummyRefs);
  Ddi_Free(bmcMgr->timeFrameVars);
  Ddi_Free(bmcMgr->bwdTimeFrameVars);
  Ddi_Free(bmcMgr->inCoreScc);

  if (bmcMgr->unrollArray!=NULL) {
    int i, n=Pdtutil_PointerArrayNum(bmcMgr->unrollArray);
    for (i=0; i<n; i++) {
      Ddi_Bddarray_t *u_i = 
	(Ddi_Bddarray_t *)Pdtutil_PointerArrayRead(bmcMgr->unrollArray,i);
      Ddi_Free(u_i);
    }
    Pdtutil_PointerArrayFree(bmcMgr->unrollArray);
  }

  if (bmcMgr->xsim.xMgr!=NULL) 
    Fsm_XsimQuit(bmcMgr->xsim.xMgr);
  Pdtutil_IntegerArrayFree(bmcMgr->xsim.constLatchArray);

  Ddi_Free(bmcMgr->part.psVars);
  Ddi_Free(bmcMgr->part.cutVars);
  Ddi_Free(bmcMgr->part.window);

  Ddi_Free(bmcMgr->bwd.cutVars);
  Ddi_Free(bmcMgr->bwd.cutVarsTot);
  Ddi_Free(bmcMgr->bwd.cutVarsAllFrames);
  Ddi_Free(bmcMgr->bwd.rings);

  Pdtutil_Free(bmcMgr);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
TravBmcMgrAddFrames(
  TravBmcMgr_t * bmcMgr,
  Ddi_IncrSatMgr_t *ddiS,
  int currBound,
  Ddi_Bdd_t * cubeConstr
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(bmcMgr->unroll);
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Bddarray_t *delta;
  int k, start_k, end_k;
  int multipleTarget = 1;
  int iCex = 0;
  int nFrames = bmcMgr->settings.stepFrames;
  int loadAbstrTarget = 0;
  int targetDecomp = 0; 

  static int enforceNewStates = 0;

  if (!bmcMgr->settings.shiftFwdFrames) {
    if (currBound%nFrames != 0) return 0; 
  }

  if (cubeConstr != NULL) {
    nFrames = Ddi_BddPartNum(cubeConstr);
    multipleTarget = 0;
    if (bmcMgr->initStub != NULL) {
      iCex++;
    }
  }

  ps = Fsm_MgrReadVarPS(bmcMgr->fsmMgr);
  pi = Fsm_MgrReadVarI(bmcMgr->fsmMgr);
  ns = Fsm_MgrReadVarNS(bmcMgr->fsmMgr);
  delta = bmcMgr->delta;

  start_k = bmcMgr->nTimeFrames;

  if (bmcMgr->settings.shiftFwdFrames) {
    end_k = nFrames + currBound - 1;
  }
  else {
    end_k = start_k + nFrames - 1;
  }

  for (k = start_k; k <= end_k; k++) {
    Ddi_Bdd_t *target_k, *prevTarget=NULL, *abstrTarget_k=NULL;
    Ddi_Bdd_t *constr_k=NULL;
    Ddi_Bddarray_t *delta_k;
    Ddi_Vararray_t *newPi;
    Ddi_Varset_t *tfSupp;
    char stepString[100];

    if (0 && k>0)
      prevTarget = Ddi_BddPartRead(bmcMgr->bmcTarget, k-1);

    if (multipleTarget || k == end_k) {
      if (bmcMgr->settings.targetByRefinement) {
      // nothing to do
	int bwdK;
	Ddi_Bdd_t *bwdRing=NULL;
	Pdtutil_Assert (bmcMgr->bmcTargetCone!=NULL,"Missing target cone");
	bwdK = Ddi_BddReadMark(bmcMgr->bmcTargetCone);
	if (bmcMgr->bwd.rings==NULL) {
	  Ddi_Bdd_t *one = Ddi_BddMakeConstAig(ddm, 1);
	  bmcMgr->bwd.rings = Ddi_BddarrayAlloc(ddm, bwdK+1);
	  Ddi_BddarrayWrite(bmcMgr->bwd.rings,bwdK,one);
	  Ddi_Free(one);
	}
	bwdRing = Ddi_BddarrayRead(bmcMgr->bwd.rings,bwdK);
	target_k = Ddi_BddDup(bwdRing);
	Ddi_BddComposeAcc(target_k, ps, bmcMgr->unroll);
	if (k<4 || k % bmcMgr->settings.targetByRefinement != 0) {
	  Ddi_Bdd_t *rel_k = Ddi_BddRelMakeFromArray(bmcMgr->unroll, ps);
	  //	  Ddi_BddSetAig(rel_k);
	  Pdtutil_Assert (bmcMgr->bmcTargetCone!=NULL,"Missing target cone");
	  if (Ddi_BddIsPartConj(rel_k)) {
	    Ddi_BddPartInsertLast(rel_k,target_k);
	    Ddi_DataCopy(target_k,rel_k);
	  }
	  else 
	    Ddi_BddAndAcc(target_k,rel_k);
	  Ddi_Free(rel_k);
	}
	else {
	  //	  bmcMgr->settings.targetByRefinement *= 4;
	}
      }
      else if (bmcMgr->bmcTargetCone!=NULL) {
	Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
	target_k = Ddi_BddRelMakeFromArray(bmcMgr->unroll, ps);
	//	Ddi_BddCofactorAcc(target_k,pvarPs,1);
	Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(pvarPs,1);
	if (Ddi_BddIsPartConj(target_k)) {
	  int np = Ddi_BddPartNum(target_k);
	  Ddi_BddPartWrite(target_k,np-1,lit);
	}
	else {
	  Ddi_BddAndAcc(target_k,lit);
	}
	Ddi_Free(lit);
	//	Ddi_BddSetAig(target_k);
	int addConstraint = 0;
	if (addConstraint) {
	  Ddi_Bdd_t *constr_1 = 
	    Ddi_BddCompose(bmcMgr->target, ps, bmcMgr->unroll);
	  Ddi_BddNotAcc(constr_1);
	  Ddi_BddPartInsertLast(bmcMgr->bmcConstr,constr_1);
	  Ddi_Free(constr_1);
	}
	if (bmcMgr->inCoreScc!=NULL) {
	  abstrTarget_k = Ddi_BddRelMakeFromArrayFiltered (bmcMgr->unroll, 
			   ps, bmcMgr->inCoreScc);
       	  loadAbstrTarget = 1;
	}
      }
      else {
	target_k = Ddi_BddCompose(bmcMgr->target, ps, bmcMgr->unroll);
      }
      if (targetDecomp > 1 && Ddi_BddSize(target_k) > 1000) {
	Ddi_Bdd_t *prop = Ddi_BddNot(target_k);
	Ddi_Bdd_t *pPart = Ddi_AigConjDecomp (prop,targetDecomp,0);
	Ddi_Free(target_k);
	target_k = Ddi_BddNot(pPart);
	Ddi_BddSetFlattened(target_k);
	if (Ddi_BddPartNum(target_k)==1) {
	  Ddi_BddSetAig(target_k);
	}
	Ddi_Free(prop);
      }
      Pdtutil_Assert(Ddi_BddPartNum(bmcMgr->bmcTarget) ==
        (multipleTarget ? k : 0), "wrong target num");
      Ddi_BddPartInsertLast(bmcMgr->bmcTarget, target_k);
      //      Ddi_BddSetAig(abstrTarget_k);
      Ddi_BddPartInsertLast(bmcMgr->bmcAbstrTarget, abstrTarget_k);

      Ddi_Free(target_k);
      Ddi_Free(abstrTarget_k);
    }
    // prepare for next iteration
    sprintf(stepString, "%d", k);
    newPi = Ddi_VararrayMakeNewAigVars(pi, "PDT_TF_VAR", stepString);
    tfSupp = Ddi_VarsetMakeFromArray(newPi);
    Ddi_VarsetarrayInsertLast(bmcMgr->timeFrameVars, tfSupp);
    Ddi_Free(tfSupp);

    if (prevTarget!=NULL && !Ddi_BddIsZero(prevTarget)) {
      // add previous target as constraint
      Ddi_Bdd_t *constr = Ddi_BddNot(prevTarget);
      if (0 && Ddi_BddPartNum(bmcMgr->bmcConstr) >= k) {
	Ddi_Bdd_t *prevConstr = Ddi_BddPartRead(bmcMgr->bmcConstr,k-1);
	if (prevConstr!=NULL) {
	  Ddi_BddAndAcc(constr,prevConstr);
	}
      }
      Ddi_BddSetAig(constr);
      timeFrameShiftKAcc(constr,bmcMgr->timeFrameVars,1);
      Ddi_BddPartWrite(bmcMgr->bmcConstr, k, constr);
      Ddi_Free(constr);
    }

    if (bmcMgr->bmcInvar != NULL && (k<=bmcMgr->settings.invarMaxBound)) {
      Ddi_Bdd_t *invar_k = 
	Ddi_BddCompose(bmcMgr->bmcInvar, ps, bmcMgr->unroll);
      Ddi_AigSatMinisatLoadClausesIncrementalAsserted(ddiS,invar_k);
      Pdtutil_Assert(bmcMgr->dummyRefs!=NULL,"uninitialized refs");
      Ddi_BddarrayInsertLast(bmcMgr->dummyRefs,invar_k);
      Ddi_Free(invar_k);
    }

    if (multipleTarget && (bmcMgr->constraint != NULL ||
                           k==0 && bmcMgr->initStubConstr!=NULL)) {
      if (bmcMgr->constraint != NULL) {
        constr_k = Ddi_BddCompose(bmcMgr->constraint, ps, bmcMgr->unroll);
        Ddi_BddSubstVarsAcc(constr_k, pi, newPi);
      }
      else {
        constr_k = Ddi_BddMakeConstAig(ddm, 1);
      }    
      if (k==0 && bmcMgr->initStubConstr!=NULL) {
	Ddi_BddAndAcc(constr_k,bmcMgr->initStubConstr);
      }
      if (constr_k != NULL) {
        if (Ddi_BddPartNum(bmcMgr->bmcConstr) == (k + 1)) {
          Pdtutil_Assert(k == 0, "wrong constr num");
          Ddi_BddAndAcc(Ddi_BddPartRead(bmcMgr->bmcConstr, 0), constr_k);
        } else {
          if (Ddi_BddPartNum(bmcMgr->bmcConstr) != k) {
            printf("k = %d, constr: %d\n", k, Ddi_BddPartNum(bmcMgr->bmcConstr));
          }
          Pdtutil_Assert(Ddi_BddPartNum(bmcMgr->bmcConstr) == k,
                         "wrong target num");
          Ddi_BddPartInsertLast(bmcMgr->bmcConstr, constr_k);
        }
        Ddi_Free(constr_k);
      }
    }

    if (bmcMgr->itpRings != NULL) {
      int nRings = Ddi_BddarrayNum(bmcMgr->itpRings);
      int period = bmcMgr->itpRingsPeriod;
      int iMid = nRings/2;
      //      if (k==(iMid+3) || 1&&(k==((iMid*3)/2+2))) {
      if (k>0 && k%period == 0 && k<nRings) {
        Ddi_Bdd_t *constr_k = Ddi_BddDup(Ddi_BddarrayRead(bmcMgr->itpRings,k));
        Ddi_Bdd_t *myOne = Ddi_BddMakeConstAig(ddm, 1);
        int sz = Ddi_BddSize(constr_k);
        Ddi_BddComposeAcc(constr_k, ns, bmcMgr->unroll);
        while (Ddi_BddPartNum(bmcMgr->bmcConstr) <= k) {
          Ddi_BddPartInsertLast(bmcMgr->bmcConstr,myOne);
        }
        Ddi_BddAndAcc(Ddi_BddPartRead(bmcMgr->bmcConstr, k), constr_k);
        Pdtutil_VerbosityMgrIf(bmcMgr->travMgr, Pdtutil_VerbLevelUsrMed_c) {
          fprintf(tMgrO(bmcMgr->travMgr), "++ ITP Constr (size: %d)) added at frame: %d.\n", sz, k);
        }
        Ddi_Free(constr_k);
        Ddi_Free(myOne);
      }
      if (0 && (k>=(iMid) && (k%2==0))) {
        int bckConeDepth = nRings-(iMid);
        int kConstr = k + bckConeDepth;
        Ddi_Bdd_t *constr_k = Ddi_BddDup(Ddi_BddarrayRead(bmcMgr->itpRings,iMid));
        Ddi_Bdd_t *myOne = Ddi_BddMakeConstAig(ddm, 1);
        Ddi_BddComposeAcc(constr_k, ns, bmcMgr->unroll);
        while (Ddi_BddPartNum(bmcMgr->bmcConstrBck) <= kConstr) {
          Ddi_BddPartInsertLast(bmcMgr->bmcConstrBck,myOne);
        }
        // bck constr is complement of ring
        Ddi_BddDiffAcc(Ddi_BddPartRead(bmcMgr->bmcConstrBck, k), constr_k);
        Ddi_Free(constr_k);
        Ddi_Free(myOne);
      }
    }

#if 0
    if (bmcMgr->part.window!=NULL) {
      Ddi_Vararray_t *cutVars = bmcMgr->part.cutVars;
      Ddi_Vararray_t *cutPi = Ddi_VararrayMakeNewAigVars(pi, "PDT_TF_VAR", stepString);
      tfSupp = Ddi_VarsetMakeFromArray(newPi);
      Ddi_VarsetarrayInsertLast(bmcMgr->timeFrameVars, tfSupp);
      Ddi_Free(tfSupp);
    }
#endif

    delta_k = Ddi_BddarrayDup(bmcMgr->delta);
    Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
    Ddi_BddarrayCofactorAcc(delta_k,pvarPs,1);

    Ddi_Bddarray_t *deltaAux=NULL;
    static int tryEquiv = 1, trySat = 0;
    if (1 && (bmcMgr->xsim.bound==k)) {
      Pdtutil_Array_t *toArray;
      Ddi_Bdd_t *fromConstr = Ddi_AigCubeMakeBddFromIntegerArray(
		   bmcMgr->xsim.constLatchArray, ps);
      if (Ddi_BddSize(fromConstr)>1) {
	int eqSplit, simplifyOut=1;
	deltaAux = Ddi_BddarrayDup(delta_k);

	toArray = Fsm_XsimSimulate(bmcMgr->xsim.xMgr, 1, 
		    bmcMgr->xsim.constLatchArray, 0, &eqSplit);
	if (trySat) {
	  int i, cnt=0;
	  Ddi_Bddarray_t *deltaConstr = Ddi_BddarrayDup(delta_k);
	  Ddi_IncrSatMgrSuspend(ddiS);
	  Ddi_AigarrayConstrainCubeAcc(deltaConstr, fromConstr);
	  for (i=0; i<Ddi_BddarrayNum(delta_k); i++) {
	    int oldV = Pdtutil_IntegerArrayRead(bmcMgr->xsim.constLatchArray, i);
	    int newV = Pdtutil_IntegerArrayRead(toArray, i);
	    if (oldV == 0 && newV == 2) {
	      Ddi_Bdd_t *d_i = Ddi_BddarrayRead(deltaConstr,i);
	      if (!Ddi_AigSat(d_i)) {
		cnt++;
		Pdtutil_IntegerArrayWrite(toArray, i, 0);
	      }
	    }
	  }
	  Ddi_Free(deltaConstr);
	  Ddi_IncrSatMgrResume(ddiS);
	  if (cnt>0)
	    printf("found %d constants using SAT\n", cnt);
	}
	Pdtutil_IntegerArrayFree(bmcMgr->xsim.constLatchArray);

	if (tryEquiv) {
	  Ddi_Free(delta_k);
	  delta_k = Fsm_XsimSymbolicMergeEq(bmcMgr->xsim.xMgr, NULL, NULL);
	}
	Ddi_AigarrayConstrainCubeAcc(delta_k, fromConstr);
	Ddi_Free(fromConstr);
	bmcMgr->xsim.constLatchArray = toArray;
	if (simplifyOut) {
	  int i;
	  Ddi_Bdd_t *zero = Ddi_BddMakeConstAig(ddm, 0);
	  Ddi_Bdd_t *one = Ddi_BddMakeConstAig(ddm, 1);
	  for (i=0; i<Ddi_VararrayNum(ps); i++) {
	    int val = Pdtutil_IntegerArrayRead(toArray,i);
	    Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta_k,i);
	    if (val != 2 && !Ddi_BddIsConstant(d_i)) {
	      /* skip don't care */
	      Ddi_BddarrayWrite(delta_k,i,val==1?one:zero);
	    }
	  }
	  Ddi_Free(zero);
	  Ddi_Free(one);
	}
	bmcMgr->xsim.bound++;
      }
      Ddi_Free(fromConstr);
    }

    if (cubeConstr != NULL) {
      Ddi_Bdd_t *cexFrame = Ddi_BddPartRead(cubeConstr, iCex++);

      Ddi_AigarrayConstrainCubeAcc(delta_k, cexFrame);
    }

    Ddi_BddarraySubstVarsAcc(delta_k, pi, newPi);
    Ddi_BddarrayComposeAcc(delta_k, ps, bmcMgr->unroll);
    if (deltaAux!=NULL) {
      Ddi_BddarraySubstVarsAcc(deltaAux, pi, newPi);
      Ddi_BddarrayComposeAcc(deltaAux, ps, bmcMgr->unroll);
      if (Ddi_BddarraySize(deltaAux)>Ddi_BddarraySize(delta_k)) {
        Pdtutil_VerbosityMgrIf(bmcMgr->travMgr, Pdtutil_VerbLevelUsrMed_c) {
          printf("ternary compaction: %d -> %d\n",
	       Ddi_BddarraySize(deltaAux),
	       Ddi_BddarraySize(delta_k));
        }
      }
      Ddi_Free(deltaAux);
    }
    if (bmcMgr->trAbstrItp != NULL && k>= bmcMgr->trAbstrItpInit &&
        ((k-bmcMgr->trAbstrItpInit)%bmcMgr->trAbstrItpPeriod==0) ) {
      //if (bmcMgr->trAbstrItp != NULL && k>bmcMgr->trAbstrItpNumFrames ) {
      Ddi_Bdd_t *myOne = Ddi_BddMakeConstAig(ddm, 1);
      Ddi_Bdd_t *unrolledTrConstr = Ddi_BddDup(bmcMgr->trAbstrItp);
      Ddi_Bddarray_t *currUnroll = bmcMgr->unroll;
      int nFrames = bmcMgr->trAbstrItpNumFrames;
      Ddi_Bddarray_t *prevUnroll =
        (Ddi_Bddarray_t *)Pdtutil_PointerArrayRead(bmcMgr->unrollArray,k-nFrames);
      Ddi_BddComposeAcc(unrolledTrConstr,ps,prevUnroll);
      static int chkIncl = bmcMgr->trAbstrItpCheck;
      Pdtutil_VerbosityMgrIf(bmcMgr->travMgr, Pdtutil_VerbLevelUsrMed_c) {
        fprintf(tMgrO(bmcMgr->travMgr), "++ TR Abstr Constr added at frame: %d.\n", k);
      }
      if (chkIncl) {
        Ddi_Bdd_t *tr = Ddi_BddRelMakeFromArray(delta, ns);
        Ddi_Bdd_t *trUnrolled = Ddi_BddRelMakeFromArray(currUnroll, ns);
        Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
        Ddi_Var_t *cvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$PS");
        Ddi_Var_t *cvarNs = Ddi_VarFromName(ddm, "PDT_BDD_INVAR_VAR$NS");
        Ddi_BddSetAig(tr);
        Ddi_BddSetAig(trUnrolled);
        Ddi_BddCofactorAcc(trUnrolled, cvarNs, 1);
        Ddi_IncrSatMgrSuspend(ddiS);
        if (nFrames==1 && k<=nFrames) {
          int isInclTr = Ddi_BddIncluded(tr,bmcMgr->trAbstrItp);
          printf("tr constr inclusion TR 1 check: %s\n", isInclTr?"OK":"KO");
          Pdtutil_Assert(isInclTr,"bug in tr appr");
        }
        else if (k<=nFrames) {
          for (int i = 1; i < nFrames; i++) {
            Ddi_Vararray_t *newVars = composeWithNewTimeFrameVars(tr,
                                        ps, delta, pi, "PDT_BMC_TARGET_PI", i);
            Ddi_Free(newVars);
          }
          //          Ddi_BddCofactorAcc(tr, pvarPs, 1);
          Ddi_BddCofactorAcc(tr, cvarPs, 1);
          Ddi_BddCofactorAcc(tr, cvarNs, 1);
          int isInclTr = Ddi_BddIncluded(tr,bmcMgr->trAbstrItp);
          printf("tr constr inclusion TR %d check: %s\n", nFrames, isInclTr?"OK":"KO");
          if (!isInclTr) {
            Ddi_Bdd_t *myCheck = Ddi_BddDiff(tr,bmcMgr->trAbstrItp);
            Ddi_Bdd_t *cex = Ddi_AigSatWithCexAndAbort(myCheck, NULL, -1.0, NULL);
            Ddi_Free(cex);
            Ddi_Free(myCheck);
          }                         
          Pdtutil_Assert(isInclTr,"bug in tr appr");
        }
        int isIncl = Ddi_BddIncluded(trUnrolled,unrolledTrConstr);
        printf("tr constr inclusion check: %s\n", isIncl?"OK":"KO");
        Pdtutil_Assert(isIncl,"bug in tr appr");
        Ddi_IncrSatMgrResume(ddiS);
        Ddi_Free(trUnrolled);
        Ddi_Free(tr);
      }
      Ddi_BddComposeAcc(unrolledTrConstr,ns,currUnroll);
      while (Ddi_BddPartNum(bmcMgr->bmcConstr) <= k) {
        Ddi_BddPartInsertLast(bmcMgr->bmcConstr,myOne);
      }
      Ddi_BddAndAcc(Ddi_BddPartRead(bmcMgr->bmcConstr, k), unrolledTrConstr);
      Ddi_Free(unrolledTrConstr);
      Ddi_Free(myOne);
    }

    if (enforceNewStates && ddiS != NULL) {
      int i;
      Ddi_Bdd_t *diffConstr = Ddi_BddMakeConstAig(ddm, 0);
      for (i=0; i<Ddi_BddarrayNum(bmcMgr->unroll); i++) {
	Ddi_Bdd_t *u_i = Ddi_BddarrayRead(bmcMgr->unroll,i);
	Ddi_Bdd_t *d_i = Ddi_BddarrayRead(delta_k,i);
	Ddi_Bdd_t *diff = Ddi_BddXor(u_i,d_i);
	Ddi_BddOrAcc(diffConstr,diff);
	Ddi_Free(diff);
      }
      Ddi_BddarrayInsertLast(bmcMgr->dummyRefs,diffConstr);
      Ddi_AigSatMinisatLoadClausesIncremental(ddiS, diffConstr,NULL);
      Ddi_Free(diffConstr);
    }

    Pdtutil_Assert (bmcMgr->unrollArray!=NULL,"Missing unroll array");
    void *u_i = Ddi_BddarrayDup(bmcMgr->unroll);
    Pdtutil_PointerArrayWrite(bmcMgr->unrollArray,k,u_i);

    Ddi_DataCopy(bmcMgr->unroll, delta_k);
    Ddi_Free(delta_k);
    Ddi_Free(newPi);
    //    DdiAigArrayRedRemovalControlAcc (bmcMgr->unroll,NULL,-1,200.0);
  }

  bmcMgr->nTimeFrames = end_k+1;

  if (ddiS != NULL) {
    if (loadAbstrTarget) {
      Ddi_AigSatMinisatLoadClausesIncremental(ddiS, bmcMgr->bmcAbstrTarget,NULL);
    }
    else {
      int np = Ddi_BddPartNum(bmcMgr->bmcTarget);
      Ddi_Bdd_t *last = Ddi_BddPartRead(bmcMgr->bmcTarget,np-1);
      Ddi_AigSatMinisatLoadClausesIncremental(ddiS, last,NULL);
      Ddi_AigLockTopClauses(ddiS,last);
      Ddi_AigSatMinisatLoadBddarrayClausesIncremental(ddiS, bmcMgr->unroll);
      Ddi_AigarrayLockTopClauses(ddiS,bmcMgr->unroll);
    }
    //    Ddi_BddarrayAppend(bmcMgr->dummyRefs,bmcMgr->unroll);
  }

}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
TravBmcMgrAddFramesFwdBwd(
  TravBmcMgr_t * bmcMgr,
  Ddi_IncrSatMgr_t *ddiS,
  int currBound
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(bmcMgr->unroll);
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Bddarray_t *delta;
  int k, start_k, end_k;
  int multipleTarget = 1;
  int iCex = 0;
  int sizeDelta = Ddi_BddarraySize(bmcMgr->delta);
  int nFrames = bmcMgr->settings.stepFrames;

  if (!bmcMgr->settings.shiftFwdFrames) {
    if (currBound%nFrames != 0) return 0; 
  }

  ps = Fsm_MgrReadVarPS(bmcMgr->fsmMgr);
  pi = Fsm_MgrReadVarI(bmcMgr->fsmMgr);
  ns = Fsm_MgrReadVarNS(bmcMgr->fsmMgr);
  delta = bmcMgr->delta;

  start_k = bmcMgr->nTimeFrames;
  end_k = start_k + nFrames - 1;

  for (k = start_k; k <= end_k; k++) {
    Ddi_Bdd_t *target_k;
    Ddi_Bdd_t *constr_k;
    Ddi_Bddarray_t *delta_k;
    Ddi_Bdd_t *tr_k;
    Ddi_Vararray_t *newPi;
    Ddi_Varset_t *tfSupp;
    char stepString[10];
    int fwdIter = 0;

    if (bmcMgr->settings.enBwd==0 || (bmcMgr->settings.enBwd > 0) && 
        ((k+1)%(bmcMgr->settings.fwdBwdRatio+1)==0)) fwdIter = 1;

    if (k==0) {
      /* load initial target frames */
      Ddi_Bdd_t *target_0;

      if (bmcMgr->settings.targetByRefinement) {
	// nothing to do
	Pdtutil_Assert (bmcMgr->bmcTargetCone!=NULL,"Missing target cone");
        target_0 = Ddi_BddDup(bmcMgr->bmcTargetCone);
      }
      else if (bmcMgr->bmcTargetCone!=NULL) {
        target_0 = Ddi_BddDup(bmcMgr->bmcTargetCone);
      }
      else {
	target_0 = Ddi_BddDup(bmcMgr->target);
      }
      Ddi_BddSubstVarsAcc(target_0, ps, ns);

      bmcMgr->bwd.cutVarsTot = Ddi_VararrayDup(ns);
      bmcMgr->bwd.cutVars = Ddi_BddSuppVararray(target_0);
      Ddi_VararrayIntersectAcc(bmcMgr->bwd.cutVars,ns);
      bmcMgr->bwd.cutVarsAllFrames = Ddi_VararrayDup(bmcMgr->bwd.cutVars);

      Ddi_AigSatMinisatLoadClausesIncrementalAsserted(ddiS,target_0);		
      Pdtutil_Assert(bmcMgr->dummyRefs!=NULL,"uninitialized refs");
      Ddi_BddarrayInsertLast(bmcMgr->dummyRefs,target_0);
      Ddi_Free(target_0);
    }

    if (multipleTarget || k == end_k) {
      Ddi_Vararray_t *currCutVars = bmcMgr->bwd.cutVars;
      Ddi_Vararray_t *totCutVars = bmcMgr->bwd.cutVarsTot;
      target_k = Ddi_BddRelMakeFromArrayFiltered (bmcMgr->unroll, totCutVars, currCutVars);
      Ddi_BddSetAig(target_k);
      Ddi_BddPartInsertLast(bmcMgr->bmcTarget, target_k);
      Ddi_Free(target_k);
    }

    // prepare for next iteration
    if (fwdIter) {
      /* forward frame */
      int kFwd = bmcMgr->settings.step++;
      sprintf(stepString, "%d", kFwd);
      newPi = Ddi_VararrayMakeNewAigVars(pi, "PDT_TF_VAR", stepString);
      tfSupp = Ddi_VarsetMakeFromArray(newPi);
      Ddi_VarsetarrayInsertLast(bmcMgr->timeFrameVars, tfSupp);
      Ddi_Free(tfSupp);

      delta_k = Ddi_BddarrayDup(bmcMgr->delta);

      Ddi_BddarraySubstVarsAcc(delta_k, pi, newPi);
      Ddi_BddarrayComposeAcc(delta_k, ps, bmcMgr->unroll);

      if (bmcMgr->settings.enBwd==0) {
        int unrollSize0 = Ddi_BddarraySize(bmcMgr->unroll);
        int unrollSize = Ddi_BddarraySize(delta_k);
        if (1 || 1&&((unrollSize-unrollSize0) > 1*sizeDelta/3)) {
          bmcMgr->settings.enBwd = 1;
          Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelUsrMin_c,
            fprintf(dMgrO(ddm), "ENABLING BWD UNROLL\n")
          );
	  //	  bmcMgr->settings.fwdBwdRatio = 0;
        }
      }
      Ddi_DataCopy(bmcMgr->unroll, delta_k);
      Ddi_Free(delta_k);
      Ddi_Free(newPi);
      bmcMgr->bwd.lastFwd = 1;
    }
    else {
      /* backward frame */
      int kBwd = bmcMgr->settings.bwdStep++;
      Ddi_Vararray_t *newPs, *newPsSupp;
      sprintf(stepString, "%d", kBwd);
      newPi = Ddi_VararrayMakeNewAigVars(pi, "PDT_BWD_TF_VAR", stepString);
      newPs = Ddi_VararrayMakeNewAigVars(ps, "PDT_BWD_TF_PS_VAR", stepString);
      if (0 && (kBwd>10)) {
        bmcMgr->settings.enBwd = -1;
        Pdtutil_VerbosityMgr(ddm, Pdtutil_VerbLevelUsrMin_c,
            fprintf(dMgrO(ddm), "DISABLING BWD UNROLL\n")
        );
      }

      tfSupp = Ddi_VarsetMakeFromArray(newPi);
      Ddi_VarsetarrayInsertLast(bmcMgr->bwdTimeFrameVars, tfSupp);
      Ddi_Free(tfSupp);

      delta_k = Ddi_BddarrayDup(bmcMgr->delta);

      Ddi_BddarraySubstVarsAcc(delta_k, pi, newPi);
      Ddi_BddarraySubstVarsAcc(delta_k, ps, newPs);
      tr_k = Ddi_BddRelMakeFromArrayFiltered (delta_k, 
	       bmcMgr->bwd.cutVarsTot, bmcMgr->bwd.cutVars);

      newPsSupp = Ddi_BddSuppVararray(tr_k);
      Ddi_VararrayIntersectAcc(newPsSupp,newPs);
      //      Ddi_BddSetAig(tr_k);

      Ddi_Free(bmcMgr->bwd.cutVars);
      Ddi_Free(bmcMgr->bwd.cutVarsTot);
      bmcMgr->bwd.cutVarsTot = Ddi_VararrayDup(newPs);
      bmcMgr->bwd.cutVars = Ddi_VararrayDup(newPsSupp);
      Ddi_VararrayAppend(bmcMgr->bwd.cutVarsAllFrames,bmcMgr->bwd.cutVars);

      Ddi_AigSatMinisatLoadClausesIncrementalAsserted(ddiS,tr_k);							
      Ddi_BddarrayInsertLast(bmcMgr->dummyRefs,tr_k);
      Ddi_Free(delta_k);
      Ddi_Free(tr_k);
      Ddi_Free(newPi);
      Ddi_Free(newPsSupp);
      Ddi_Free(newPs);
      bmcMgr->bwd.lastFwd = 0;
    }
  }

  if (ddiS != NULL) {
    Ddi_AigSatMinisatLoadClausesIncremental(ddiS, bmcMgr->bmcTarget,NULL);
  }

  bmcMgr->nTimeFrames += nFrames;
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
TravBmcMgrAddTargetCone(
  TravBmcMgr_t * bmcMgr,
  int nFrames
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(bmcMgr->unroll);
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Bddarray_t *delta, *unroll, *unrollConstr=NULL;
  Ddi_Vararray_t *newPis;
  Ddi_Varset_t *newPisV;
  int i, k;

  Pdtutil_Assert(bmcMgr->target!=NULL,"missing bmc target");

  ps = Fsm_MgrReadVarPS(bmcMgr->fsmMgr);
  pi = Fsm_MgrReadVarI(bmcMgr->fsmMgr);
  ns = Fsm_MgrReadVarNS(bmcMgr->fsmMgr);
  delta = bmcMgr->delta;

  Ddi_Bddarray_t *delta1 = Ddi_BddarrayDup(delta);
  Ddi_Var_t *pvarPs = Ddi_VarFromName(ddm, "PDT_BDD_INVARSPEC_VAR$PS");
  Ddi_BddarrayCofactorAcc(delta1,pvarPs,1);
  unroll = unrollWithNewTimeFrameVars(delta1, ps, pi, 
	     "PDT_BMC_TARGET_CONE_PI", bmcMgr->bwdTimeFrameVars, nFrames);

  bmcMgr->bmcTargetCone = Ddi_BddCompose(bmcMgr->target,ps,unroll);
  Ddi_BddWriteMark(bmcMgr->bmcTargetCone,nFrames);

  int exactBound = 1;
  if (exactBound) {
    bmcMgr->bmcConstrCone = Ddi_BddMakeConstAig(ddm, 1);
    for (k=nFrames-1; k>0 && k>nFrames-3 ; k--) {
      Ddi_Bdd_t *myConstr;
      myConstr = Ddi_BddDup(bmcMgr->target);
      if (k>0) {
	unrollConstr = unrollWithNewTimeFrameVars(delta, ps, pi, 
			 "PDT_BMC_TARGET_CONE_PI", NULL, k);
	myConstr = Ddi_BddComposeAcc(myConstr,ps,unrollConstr);
	Ddi_Free(unrollConstr);
      }
      Ddi_BddDiffAcc(bmcMgr->bmcConstrCone,myConstr);
      Ddi_Free(myConstr);
    }
  }
  Ddi_Free(delta1);

  Ddi_Free(bmcMgr->constraint);
  Ddi_Free(unroll);
  Ddi_Free(unrollConstr);

  return 1;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
TravBmcMgrTernarySimInit(
  TravBmcMgr_t * bmcMgr,
  int strategy
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(bmcMgr->unroll);
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Bddarray_t *delta, *unroll;
  Ddi_Bddarray_t *initStub = bmcMgr->initStub;
  Ddi_Bdd_t *init = bmcMgr->init;
  int j, nState;

  Pdtutil_Assert(bmcMgr->target!=NULL,"missing bmc target");

  ps = Fsm_MgrReadVarPS(bmcMgr->fsmMgr);
  pi = Fsm_MgrReadVarI(bmcMgr->fsmMgr);
  ns = Fsm_MgrReadVarNS(bmcMgr->fsmMgr);
  delta = bmcMgr->delta;
  nState = Ddi_VararrayNum(ps);

  bmcMgr->xsim.xMgr = Fsm_XsimInit(bmcMgr->fsmMgr, 
				   delta, NULL, ps, NULL, strategy);

  if (initStub != NULL) {
    bmcMgr->xsim.constLatchArray = Pdtutil_IntegerArrayAlloc(nState);
    for (j = 0; j < nState; j++) {
      Ddi_Bdd_t *f_j = Ddi_BddarrayRead(initStub, j);
      int val = 2;
      if (Ddi_BddIsConstant(f_j)) {
        val = Ddi_BddIsOne(f_j) ? 1 : 0;
      }
      Pdtutil_IntegerArrayInsertLast(bmcMgr->xsim.constLatchArray, val);
    }
  } else {
    bmcMgr->xsim.constLatchArray = 
      Ddi_AigCubeMakeIntegerArrayFromBdd(init, ps);
  }

  bmcMgr->xsim.bound = 0;

  return 1;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Trav_TravSatBmcIncrVerif0(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bddarray_t * prevCexes,
  int bound,
  int step,
  int first,
  int doReturnCex,
  int lemmaSteps,
  int interpolantSteps,
  int timeLimit
)
{
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Varset_t *totPiVars;
  Ddi_Bddarray_t *delta;
  Ddi_Bdd_t *target, *myInit, *myInvarspec; //, *to;
  Ddi_Bdd_t *cubeInit;
  Ddi_Bddarray_t *unroll;
  int sat = 0, i, nsat;
  long startTime, bmcTime = 0, bmcStartTime, bmcStepTime;
  Ddi_Bdd_t *constrain = NULL;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);
  int provedBound = 0, inverse = 0;
  int doOptThreshold = 5000;
  int optLevel = 0;
  Ddi_Varset_t *psVars = NULL;
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  unsigned long time_limit = ~0;
  Trav_Stats_t *travStats = &(travMgr->xchg.stats);
  Ddi_Bdd_t *partConstr = Ddi_BddMakePartConjVoid(ddm);
  Ddi_Bdd_t *returnedCex = NULL;
  Ddi_Varsetarray_t *timeFrameVars = Ddi_VarsetarrayAlloc(ddm, 1);
  Ddi_Varsetarray_t *bwdTimeFrameVars = Ddi_VarsetarrayAlloc(ddm, 1);
  Ddi_Varset_t *tfSupp = NULL;
  Ddi_Bddarray_t *myCexes = NULL;
  int currCexId = -1, currCexLen = -1;
  int safe = 0;

  startTime = util_cpu_time();
  if (timeLimit > 0) {
    time_limit = util_cpu_time() + timeLimit * 1000;
  }
  if (bound < 0) {
    bound = 1000000;
  }

  if (prevCexes != NULL) {
    int ii, prevNum = 0;

    myCexes = Ddi_BddarrayDup(prevCexes);
    for (ii = 0; ii < Ddi_BddarrayNum(myCexes); ii++) {
      Ddi_Bdd_t *cex_ii = Ddi_BddarrayRead(myCexes, ii);

      Pdtutil_Assert(Ddi_BddPartNum(cex_ii) >= prevNum,
        "cex array not sorted");
      prevNum = Ddi_BddPartNum(cex_ii);
    }
    currCexId = 0;
    bound = prevNum - 1;
    currCexLen = Ddi_BddPartNum(Ddi_BddarrayRead(myCexes, currCexId));
    Pdtutil_Assert(currCexLen >= 2, "wrong cex len");
    first = currCexLen - 2;
    Pdtutil_Assert(Ddi_BddarrayNum(myCexes) > 0, "no prev cexes");
  }

  travStats->cpuTimeTot = 0;

  ps = Fsm_MgrReadVarPS(fsmMgr);
  pi = Fsm_MgrReadVarI(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);

  optLevel = Ddi_MgrReadAigBddOptLevel(ddm);

  myInvarspec = Ddi_BddDup(invarspec);
  myInit = Ddi_BddDup(init);
  psVars = Ddi_VarsetMakeFromArray(ps);
  target = Ddi_BddNot(myInvarspec);

  totPiVars = Ddi_VarsetMakeFromArray(pi);

  if (initStub != NULL) {
    unroll = Ddi_BddarrayDup(initStub);
    tfSupp = Ddi_BddarraySupp(initStub);
  } else {
    Ddi_Bdd_t *initLits = Ddi_AigPartitionTop(myInit, 0);
    int j, nv = Ddi_BddPartNum(initLits);
    Ddi_Vararray_t *vA = Ddi_VararrayAlloc(ddm, 0);
    Ddi_Bddarray_t *subst = Ddi_BddarrayAlloc(ddm, 0);
    Ddi_Bdd_t *myOne = Ddi_BddMakeConstAig(ddm, 1);
    Ddi_Bdd_t *myZero = Ddi_BddMakeConstAig(ddm, 0);

    tfSupp = Ddi_VarsetVoid(ddm);
    for (j = 0; j < nv; j++) {
      Ddi_Bdd_t *l_j = Ddi_BddPartRead(initLits, j);

      if (Ddi_BddSize(l_j) == 1) {
        /* literal */
        Ddi_Var_t *v = Ddi_BddTopVar(l_j);

        Ddi_VararrayWrite(vA, j, v);
        if (Ddi_BddIsComplement(l_j)) {
          Ddi_BddarrayWrite(subst, j, myZero);
        } else {
          Ddi_BddarrayWrite(subst, j, myOne);
        }
      } else {
        /* not a literal */
        Ddi_BddPartInsertLast(partConstr, l_j);
      }
    }
    Ddi_Free(myZero);
    Ddi_Free(myOne);
    unroll = Ddi_BddarrayMakeLiteralsAig(ps, 1);

    if (Ddi_VararrayNum(vA) > 0) {
      Ddi_BddarrayComposeAcc(unroll, vA, subst);
    }

    Ddi_Free(initLits);
    Ddi_Free(vA);
    Ddi_Free(subst);
  }

  Ddi_VarsetarrayWrite(timeFrameVars, 0, tfSupp);
  Ddi_Free(tfSupp);

  for (i = nsat = 0; i <= bound && !sat; i++) {

    if (myCexes != NULL && currCexId >= Ddi_BddarrayNum(myCexes)) {
      sat = -1;
      break;
    }

    if (util_cpu_time() > time_limit) {
      //printf("Invariant verification UNDECIDED after BMC ...\n");
      sat = -1;
      break;
    }

    safe = Fsm_MgrReadUnsatBound(fsmMgr);
    if (safe > 0 && safe > first) {
      first = safe;
    }

    if (i >= first && ((i - first) % step) == 0) {
      int sat1, enCheck = 1;

      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMin_c,
        fprintf(tMgrO(travMgr), "BMC check at bound %2d - ", i)
        );
      bmcStartTime = util_cpu_time();

      if (!enCheck) {
        sat1 = 0;
      } else {

        Ddi_Bdd_t *check = Ddi_BddCompose(target, ps, unroll);
        Ddi_Bdd_t *checkPart = Ddi_BddDup(partConstr);

        Ddi_BddPartInsertLast(checkPart, check);

        if (util_cpu_time() >= time_limit) {
          sat1 = -1;
        } else {
          int abort = 0;
          Ddi_Bdd_t *cex = NULL;

          if (myCexes != NULL) {
            while (!abort && cex == NULL
              && currCexId < Ddi_BddarrayNum(myCexes)
              && Ddi_BddPartNum(Ddi_BddarrayRead(myCexes,
                  currCexId)) == (i + 2)) {
              int jj;
              Ddi_Bdd_t *prev = Ddi_BddarrayRead(myCexes, currCexId);
              Ddi_Bdd_t *myCheck = Ddi_BddDup(checkPart);

              Ddi_BddSetAig(prev);
              currCexId++;
              for (jj = 0; jj < Ddi_BddPartNum(myCheck); jj++) {
                Ddi_Bdd_t *p = Ddi_BddPartRead(myCheck, jj);

                Ddi_AigAndCubeAcc(p, prev);
              }
              cex = Ddi_AigSatWithCexAndAbort(myCheck, NULL,
                (time_limit - util_cpu_time()) / 1000.0, &abort);
              Ddi_Free(myCheck);
            }
          } else {
            cex = Ddi_AigSatWithCexAndAbort(checkPart, NULL,
              (time_limit - util_cpu_time()) / 1000.0, &abort);
          }
          sat1 = 0;
          if (abort) {
            Pdtutil_Assert(!sat1, "SAT abort with cex");
            sat1 = -1;
          } else if (cex != NULL) {
            if (doReturnCex) {
              int jj;

              returnedCex = Ddi_BddMakePartConjVoid(ddm);
              for (jj = 0; jj < Ddi_VarsetarrayNum(timeFrameVars); jj++) {
                Ddi_Bdd_t *tfCex;

                tfSupp = Ddi_VarsetarrayRead(timeFrameVars, jj);
                tfCex = Ddi_BddCubeExistProject(cex, tfSupp);
                Ddi_BddCubeExistAcc(cex, tfSupp);
                Ddi_BddPartInsertLast(returnedCex, tfCex);
                Ddi_Free(tfCex);
              }
              Ddi_BddPartInsertLast(returnedCex, cex);
            }
            Ddi_Free(cex);
            sat1 = 1;
          }
        }
        /* save !check as future constraint */
        Ddi_BddNotAcc(check);
        //  Ddi_BddPartInsertLast(partConstr,check);
        Ddi_Free(check);
        Ddi_Free(checkPart);
      }

      bmcTime += (bmcStepTime = (util_cpu_time() - bmcStartTime));
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMin_c,
        fprintf(tMgrO(travMgr),
          "BMC step (tot) time: %s (%s).\n",
          util_print_time(bmcStepTime), util_print_time(bmcTime))
        );

      pthread_mutex_lock(&travStats->mutex);
      if (travStats->arrayStatsNum > 0 &&
        strcmp(Pdtutil_ArrayReadName(travStats->arrayStats[0]),
          "BMCSTATS") == 0) {
        Pdtutil_IntegerArrayInsertLast(travStats->arrayStats[0], bmcStepTime);
      }
      travStats->cpuTimeTot += bmcStepTime;
      travStats->provedBound = i;
      pthread_mutex_unlock(&travStats->mutex);

      if (sat1 == 1) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(tMgrO(travMgr), "BMC failure at bound %d.\n", i)
          );
        provedBound = -i;
        sat = 1;
      }
      if (sat1 == -1) {
        //printf("Invariant verification UNDECIDED after BMC ...\n");
        sat = -1;
      }
      if (sat1 == 0) {
        provedBound = i;
      }
    }

    if (!sat) {
      int j;
      Ddi_Bddarray_t *delta_i;
      Ddi_Vararray_t *newPi = Ddi_VararrayAlloc(ddm, Ddi_VararrayNum(pi));
      Ddi_Bddarray_t *newPiLit = Ddi_BddarrayAlloc(ddm, Ddi_VararrayNum(pi));

      for (j = 0; j < Ddi_VararrayNum(pi); j++) {
        char name[1000];
        Ddi_Var_t *v = Ddi_VararrayRead(pi, j);
        Ddi_Var_t *newv = NULL;
        Ddi_Bdd_t *newvLit;

        sprintf(name, "%s_%d", Ddi_VarName(v), i);
#if 0
        newv = Ddi_VarNewBeforeVar(v);
        Ddi_VarAttachName(newv, name);
#else
        newv = Ddi_VarFromName(ddm, name);
        if (newv == NULL) {
          newv = Ddi_VarNewBaig(ddm, name);
        }
#endif
        Ddi_VararrayWrite(newPi, j, newv);
        //      Ddi_VarsetAddAcc(totPiVars,newv);
        newvLit = Ddi_BddMakeLiteralAig(newv, 1);
        Ddi_BddarrayWrite(newPiLit, j, newvLit);
        Ddi_Free(newvLit);
      }
      delta_i = Ddi_BddarrayDup(delta);
      Ddi_BddarrayComposeAcc(delta_i, pi, newPiLit);
      Ddi_BddarrayComposeAcc(delta_i, ps, unroll);
      Ddi_Free(unroll);
      unroll = delta_i;

      tfSupp = Ddi_VarsetMakeFromArray(newPi);
      Ddi_VarsetarrayInsertLast(timeFrameVars, tfSupp);
      Ddi_Free(tfSupp);

      Ddi_Free(newPi);
      Ddi_Free(newPiLit);

      Fsm_MgrLogUnsatBound(fsmMgr, i, 0);
    }
  }

  if (!sat) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(tMgrO(travMgr), "BMC pass at bound %d.\n", provedBound)
      );
  }

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(tMgrO(travMgr), "Total SAT time: %s, total BMC time: %s.\n",
      util_print_time(bmcTime), util_print_time(util_cpu_time() - startTime))
    );

  if (sat < 0) {
    sat = -2;
  }
  Trav_MgrSetAssertFlag(travMgr, sat);

  Ddi_Free(partConstr);

  Ddi_Free(timeFrameVars);
  Ddi_Free(bwdTimeFrameVars);
  Ddi_Free(myCexes);

  Ddi_Free(psVars);
  Ddi_Free(constrain);
  Ddi_Free(myInvarspec);
  Ddi_Free(myInit);
  Ddi_Free(unroll);
  Ddi_Free(target);
  Ddi_Free(totPiVars);
  return returnedCex;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Trav_TravSatBmcVerif2(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
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
)
{
  Ddi_Vararray_t *pi, *ps, *ns, *cutVars;
  Ddi_Varset_t *totPiVars;
  Ddi_Bddarray_t *delta, *deltaCut, *cutLits, *psLits, *deltaPre;
  Ddi_Bdd_t *reached, *target, *preTarget, *myInit, *myInvarspec;   //, *to;
  Ddi_Bdd_t *cubeInit;
  int sat = 0, i, initIsCube, nsat;
  long startTime, bmcTime = 0, bmcStartTime, bmcStepTime;
  Ddi_Bdd_t *constrain = NULL;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);
  int provedBound = 0, inverse = 0;
  int doOptThreshold = 5000;
  int optLevel = 0;
  Ddi_Varset_t *psVars = NULL;
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  unsigned long time_limit = ~0;
  Trav_Stats_t *travStats = &(travMgr->xchg.stats);

  startTime = util_cpu_time();
  if (timeLimit > 0) {
    time_limit = util_cpu_time() + timeLimit * 1000;
  }
  if (bound < 0) {
    bound = 1000000;
  }

  travStats->cpuTimeTot = 0;

  ps = Fsm_MgrReadVarPS(fsmMgr);
  pi = Fsm_MgrReadVarI(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);

  optLevel = Ddi_MgrReadAigBddOptLevel(ddm);

  initIsCube = 0;               //Ddi_BddarraySize(delta) < 8000;

  cutVars = Ddi_VararrayMakeNewVars(ps, "PDT_BMC_CUT_PS", "0", 1);
  cutLits = Ddi_BddarrayMakeLiteralsAig(cutVars, 1);
  psLits = Ddi_BddarrayMakeLiteralsAig(ps, 1);

  myInvarspec = Ddi_BddDup(invarspec);
  myInit = Ddi_BddDup(init);
  psVars = Ddi_VarsetMakeFromArray(ps);
  target = Ddi_BddNot(myInvarspec);
  Ddi_BddComposeAcc(target, ps, cutLits);
  Ddi_BddComposeAcc(myInvarspec, ps, cutLits);
  /* part conj BDD */
  preTarget = Ddi_BddRelMakeFromArray(psLits, cutVars);
  deltaCut = Ddi_BddarrayDup(delta);
  deltaPre = Ddi_BddarrayDup(delta);
  Ddi_BddarrayComposeAcc(deltaCut, ps, cutLits);

  totPiVars = Ddi_VarsetMakeFromArray(pi);
  Ddi_VarsetSetArray(totPiVars);

  for (i = nsat = 0; i <= bound && !sat; i++) {
    if (util_cpu_time() > time_limit) {
      //printf("Invariant verification UNDECIDED after BMC ...\n");
      sat = -1;
      break;
    }

    if (i >= first && ((i - first) % step) == 0) {
      Ddi_Bdd_t *check;
      int sat1, enCheck = 1;

      check = bmcCoiFilter(preTarget, target, NULL, cutVars);
      if (initStub != NULL) {
        Ddi_BddComposeAcc(check, ps, initStub);
      } else if (initIsCube) {
        Ddi_AigAndCubeAcc(check, myInit);
      } else {
        Ddi_BddAndAcc(check, myInit);
      }

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(tMgrO(travMgr), "BMC check at bound %2d - ", i)
        );
      bmcStartTime = util_cpu_time();


      if (!enCheck) {
        sat1 = 0;
      } else {
        if (util_cpu_time() >= time_limit) {
          sat1 = -1;
        } else {
          int partSatIter = 0;

          if (i > 2) {
            partSatIter = interpolantSteps;
          }
          sat1 = bmcSatAndWithAbort(check, target, deltaPre,
            NULL, cutVars, totPiVars, partSatIter,
            (time_limit - util_cpu_time()) / 1000.0);
        }
      }

      Ddi_Free(check);

      bmcTime += (bmcStepTime = (util_cpu_time() - bmcStartTime));
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(tMgrO(travMgr), "BMC step time: %s. (res: %d)\n",
          util_print_time(bmcStepTime), sat1)
        );

      pthread_mutex_lock(&travStats->mutex);
      if (travStats->arrayStatsNum > 0 &&
        strcmp(Pdtutil_ArrayReadName(travStats->arrayStats[0]),
          "BMCSTATS") == 0) {
        Pdtutil_IntegerArrayInsertLast(travStats->arrayStats[0], bmcStepTime);
      }
      travStats->cpuTimeTot += bmcStepTime;
      travStats->provedBound = i;
      pthread_mutex_unlock(&travStats->mutex);

      /*Pdtutil_Assert(sat==sat2,"wrong result of circuit sat"); */
      if (sat1 == 1) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(tMgrO(travMgr), "BMC failure at bound %d.\n", i)
          );
        provedBound = -i;
        sat = 1;
      }
      if (sat1 == -1) {
        //printf("Invariant verification UNDECIDED after BMC ...\n");
        sat = -1;
      }
      if (sat1 == 0) {
        provedBound = i;
      }
    }
    if (!sat) {
      int j;
      Ddi_Bddarray_t *delta_i;
      Ddi_Bdd_t *invarspec_i;
      Ddi_Vararray_t *newPi = Ddi_VararrayAlloc(ddm, Ddi_VararrayNum(pi));
      Ddi_Bddarray_t *newPiLit = Ddi_BddarrayAlloc(ddm, Ddi_VararrayNum(pi));
      Ddi_Varset_t *newVars;

      for (j = 0; j < Ddi_VararrayNum(pi); j++) {
        char name[1000];
        Ddi_Var_t *v = Ddi_VararrayRead(pi, j);
        Ddi_Var_t *newv = NULL;
        Ddi_Bdd_t *newvLit;

        sprintf(name, "%s_%d", Ddi_VarName(v), i);
        newv = Ddi_VarFromName(ddm, name);
        if (newv == NULL) {
          newv = Ddi_VarNewBaig(ddm, name);
        }
        Ddi_VararrayWrite(newPi, j, newv);
        newvLit = Ddi_BddMakeLiteralAig(newv, 1);
        Ddi_BddarrayWrite(newPiLit, j, newvLit);
        Ddi_Free(newvLit);
      }
      newVars = Ddi_VarsetMakeFromArray(newPi);
      Ddi_VarsetUnionAcc(totPiVars, newVars);
      Ddi_Free(newVars);

      if (i < step - 1 || (i % 2)) {
        delta_i = Ddi_BddarrayDup(deltaCut);
        Ddi_BddarrayComposeAcc(delta_i, pi, newPiLit);
        Ddi_BddComposeAcc(target, cutVars, delta_i);
        invarspec_i = Ddi_BddCompose(myInvarspec, pi, newPiLit);
        if (i < step - 1) {
          Ddi_BddNotAcc(invarspec_i);
          Ddi_BddOrAcc(target, invarspec_i);
        } else {
          Ddi_BddAndAcc(target, invarspec_i);
        }
      } else {
        delta_i = Ddi_BddarrayDup(delta);
        Ddi_BddarrayComposeAcc(delta_i, pi, newPiLit);
        Ddi_BddComposeAcc(preTarget, ps, delta_i);
        Ddi_BddarrayComposeAcc(deltaPre, ps, delta_i);
        invarspec_i = Ddi_BddCompose(invarspec, pi, newPiLit);
        Ddi_BddAndAcc(preTarget, invarspec_i);
      }
      Ddi_Free(invarspec_i);
      Ddi_Free(delta_i);
      Ddi_Free(newPi);
      Ddi_Free(newPiLit);

      Fsm_MgrLogUnsatBound(fsmMgr, i, 0);
    }
  }

  if (!sat) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(tMgrO(travMgr), "BMC pass at bound %d.\n", provedBound)
      );
  }

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(tMgrO(travMgr), "Total SAT time: %s, total BMC time: %s.\n",
      util_print_time(bmcTime), util_print_time((util_cpu_time() - startTime)))
    );

  Trav_MgrSetAssertFlag(travMgr, sat);

  Ddi_Free(preTarget);
  Ddi_Free(deltaCut);
  Ddi_Free(deltaPre);
  Ddi_Free(cutVars);
  Ddi_Free(cutLits);
  Ddi_Free(psLits);

  Ddi_Free(psVars);
  Ddi_Free(constrain);
  Ddi_Free(myInvarspec);
  Ddi_Free(myInit);
  Ddi_Free(target);
  Ddi_Free(totPiVars);
  return provedBound + 1;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************
void
Trav_TravSatBmcTrVerif(
  Trav_Mgr_t *travMgr  // Traversal Manager ,
  Fsm_Mgr_t *fsmMgr,
  Ddi_Bdd_t *init,
  Ddi_Bdd_t *invarspec,
  Ddi_Bdd_t *invar,
  int bound,
  int step
)
{
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Varset_t *pivars, *smooth;
  Ddi_Bddarray_t *delta, *psLit, *nsLit;
  Ddi_Bdd_t *unroll, *tr, *tr_i; //, *reached;
  int sat=0, i, nState;
  long startTime;//, currTime;
  char stepString[10];

  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);
  ps = Fsm_MgrReadVarPS(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  pi = Fsm_MgrReadVarI(fsmMgr);

  delta = Fsm_MgrReadDeltaBDD (fsmMgr);
  nState = Ddi_BddarrayNum(delta);
  psLit = Ddi_BddarrayAlloc(ddm,nState);
  nsLit = Ddi_BddarrayAlloc(ddm,nState);

  // build tr 
  tr = Ddi_BddRelMakeFromArray(delta,ns);
  // to be removed!!!!
  Ddi_BddSetAig(tr);

  unroll = Ddi_BddNot(invarspec);

  startTime = util_cpu_time ();

  for (i=0; i<bound && !sat; i++) {

    if (((i)%step) == 0) {
      Ddi_Bdd_t *check;

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(tMgrO(travMgr), "BMC check at bound %d (AIG size: %d).\n",
          i, Ddi_BddSize(unroll))
      );

      check = Ddi_BddAnd(unroll,init);
      if (Ddi_AigSat(check)==1) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(tMgrO(travMgr), "BMC failure at bound %d.\n", i)
        );
        sat=1;
      }
      Ddi_Free(check);
    }

    sprintf(stepString,"%d",i);

    if (!sat) {
      int j;
      //Ddi_Bddarray_t *delta_i;
      Ddi_Bdd_t *invarspec_i;
      Ddi_Vararray_t *newPi =
	Ddi_VararrayMakeNewAigVars (pi, "PDT_TF_VAR", stepString);
      Ddi_Bddarray_t *newPiLit = Ddi_BddarrayMakeLiteralsAig (newPi,1);
      Ddi_Vararray_t *newNs =
        Ddi_VararrayMakeNewAigVars (ns, "PDT_TF_VAR", stepString);
      Ddi_Bddarray_t *newNsLit = Ddi_BddarrayMakeLiteralsAig (newNs,1);

      tr_i = Ddi_BddCompose(tr,pi,newPiLit);
      Ddi_BddComposeAcc(tr_i,ns,newNsLit);
      Ddi_BddComposeAcc(unroll,ps,newNsLit);

      invarspec_i = Ddi_BddCompose(invarspec,pi,newPiLit);
      Ddi_BddAndAcc(unroll,tr_i);
      if (i<step-1) {
	Ddi_BddNotAcc(invarspec_i);
        Ddi_BddOrAcc(unroll,invarspec_i);
      }
      else {
        Ddi_BddAndAcc(unroll,invarspec_i);
      }
      Ddi_Free(invarspec_i);
      Ddi_Free(tr_i);
      Ddi_Free(newPi);
      Ddi_Free(newPiLit);
      Ddi_Free(newNs);
      Ddi_Free(newNsLit);
    }
  }

  if (!sat) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(tMgrO(travMgr), "BMC pass at bound %d.\n", bound)
    );
  }
  Trav_MgrSetAssertFlag (travMgr, sat);

  Ddi_Free(unroll);
  Ddi_Free(psLit);
  Ddi_Free(nsLit);
  Ddi_Free(tr);
}
*/

void
Trav_TravSatBmcTrVerif(
  Trav_Mgr_t * travMgr,         // Traversal Manager
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int bound,
  int step
)
{
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Varset_t *pivars, *smooth;
  Ddi_Bddarray_t *delta, *psLit, *nsLit;
  Ddi_Bdd_t *unroll, *tr, *tr_i;    //, *reached;
  int sat = 0, i, nState;

  // long startTime;//, currTime;
  char stepString[10];

  //NEW
  long startTime, currTime, currStepInit, totTime, satTime = 0;


  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);

  ps = Fsm_MgrReadVarPS(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  pi = Fsm_MgrReadVarI(fsmMgr);

  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  nState = Ddi_BddarrayNum(delta);
  psLit = Ddi_BddarrayAlloc(ddm, nState);
  nsLit = Ddi_BddarrayAlloc(ddm, nState);

  /* build tr */
  tr = Ddi_BddRelMakeFromArray(delta, ns);
  // to be removed!!!!
  Ddi_BddSetAig(tr);

  unroll = Ddi_BddNot(invarspec);

  startTime = util_cpu_time();

  for (i = 0; i < bound && !sat; i++) {

    if (((i) % step) == 0) {
      Ddi_Bdd_t *check;

      if (i == 0) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(tMgrO(travMgr), "BMC check at bound %d (AIG size: %d).\n",
            i, Ddi_BddSize(unroll)));
      } else {

        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c, fprintf(tMgrO(travMgr),
            "BMC check at bound %d (AIG size: %d). time (sat, tot, s/t):\t%s\t%s\t(%g)\n",
            i, Ddi_BddSize(unroll), util_print_time(satTime),
            util_print_time(totTime), (float)satTime / totTime));
      }

      check = Ddi_BddAnd(unroll, init);
      //      check = Ddi_BddAnd(init,unroll);

      currStepInit = util_cpu_time();

      if (Ddi_AigSat(check) == 1) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(tMgrO(travMgr), "BMC failure at bound %d.\n", i)
          );
        sat = 1;
      }

      currTime = util_cpu_time();
      Ddi_Free(check);

      satTime += currTime - currStepInit;
      totTime = currTime - startTime;
    }

    sprintf(stepString, "%d", i);

    if (!sat) {
      int j;

      //Ddi_Bddarray_t *delta_i;
      Ddi_Bdd_t *invarspec_i;
      Ddi_Vararray_t *newPi =
        Ddi_VararrayMakeNewAigVars(pi, "PDT_TF_VAR", stepString);
      Ddi_Bddarray_t *newPiLit = Ddi_BddarrayMakeLiteralsAig(newPi, 1);
      Ddi_Vararray_t *newNs =
        Ddi_VararrayMakeNewAigVars(ns, "PDT_TF_VAR", stepString);
      Ddi_Bddarray_t *newNsLit = Ddi_BddarrayMakeLiteralsAig(newNs, 1);

      tr_i = Ddi_BddCompose(tr, pi, newPiLit);
      Ddi_BddComposeAcc(tr_i, ns, newNsLit);
      Ddi_BddComposeAcc(unroll, ps, newNsLit);

      invarspec_i = Ddi_BddCompose(invarspec, pi, newPiLit);
      Ddi_BddAndAcc(unroll, tr_i);


      if (i < step - 1) {
        //ignore, it's not working
        Pdtutil_Assert(1 == 2, "shouldn't be here");
        Ddi_BddNotAcc(invarspec_i);
        Ddi_BddOrAcc(unroll, invarspec_i);
      } else {
        //it works
        Ddi_BddAndAcc(unroll, invarspec_i);
      }

      Ddi_Free(invarspec_i);
      Ddi_Free(tr_i);
      Ddi_Free(newPi);
      Ddi_Free(newPiLit);
      Ddi_Free(newNs);
      Ddi_Free(newNsLit);
    }
  }

  if (!sat) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(tMgrO(travMgr), "BMC pass at bound %d.\n", bound)
      );
  }
  Trav_MgrSetAssertFlag(travMgr, sat);

  Ddi_Free(unroll);
  Ddi_Free(psLit);
  Ddi_Free(nsLit);
  Ddi_Free(tr);
}

/* FORWARD INSERT VERSION */
void
Trav_TravSatBmcTrVerif_ForwardInsert(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int bound,
  int step
)
{
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Varset_t *pivars, *smooth;
  Ddi_Bddarray_t *delta, *psLit, *nsLit;
  Ddi_Bdd_t *unroll, *tr, *tr_i;
  int sat = 0, i, nState;
  char stepString[10];

  //NEW
  long startTime, currTime, currStepInit, totTime, satTime = 0;
  Ddi_Bdd_t *target;
  Ddi_Bdd_t *target_i;
  Ddi_Bddarray_t *lastNsLit;

  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);

  ps = Fsm_MgrReadVarPS(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  pi = Fsm_MgrReadVarI(fsmMgr);

  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  nState = Ddi_BddarrayNum(delta);
  psLit = Ddi_BddarrayAlloc(ddm, nState);
  nsLit = Ddi_BddarrayAlloc(ddm, nState);

  printf("<<< TravSatBmcTrVerif_ForwardInsert >>>\n");

  /* build tr */
  tr = Ddi_BddRelMakeFromArray(delta, ns);
  Ddi_BddSetAig(tr);

  target = Ddi_BddNot(invarspec);
  target_i = Ddi_BddDup(target);

  startTime = util_cpu_time();

  for (i = 0; i < bound && !sat; i++) {

    if (((i) % step) == 0) {
      Ddi_Bdd_t *check;

      if (i == 0) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c, fprintf(tMgrO(travMgr),
            "BMC check at bound %d (AIG size: %d).\n", i,
            Ddi_BddSize(target_i)));
        check = Ddi_BddAnd(init, target_i);

      } else {

        //  check = Ddi_BddAnd(unroll,target_i);
        check = Ddi_BddDup(unroll);
        Ddi_BddPartInsertLast(check, target_i);

        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c, fprintf(tMgrO(travMgr),
            "BMC check at bound %d (AIG size: %d). time (sat, tot, s/t):\t%s\t%s\t(%g)\n",
            i, Ddi_BddSize(check), util_print_time(satTime),
            util_print_time(totTime), (float)satTime / totTime));

        //  Ddi_BddAndAcc(check,init);
      }

      currStepInit = util_cpu_time();

      // AigSat_withChecks
      if (Ddi_AigSat(check) == 1) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c, fprintf(tMgrO(travMgr),
            "BMC failure at bound %d.\n", i));
        sat = 1;
      }

      currTime = util_cpu_time();
      Ddi_Free(check);

      satTime += currTime - currStepInit;
      totTime = currTime - startTime;
    }


    sprintf(stepString, "%d", i);

    Ddi_Free(target_i);

    if (i == 0) {

      //first round
      if (!sat) {
        int j;

        //Ddi_Bdd_t *invarspec_i;
        Ddi_Vararray_t *newPi =
          Ddi_VararrayMakeNewAigVars(pi, "PDT_TF_VAR", stepString);
        Ddi_Bddarray_t *newPiLit = Ddi_BddarrayMakeLiteralsAig(newPi, 1);
        Ddi_Vararray_t *newNs =
          Ddi_VararrayMakeNewAigVars(ns, "PDT_TF_VAR", stepString);
        Ddi_Bddarray_t *newNsLit = Ddi_BddarrayMakeLiteralsAig(newNs, 1);

        unroll = Ddi_BddCompose(tr, pi, newPiLit);

        Ddi_BddSetPartConj(unroll);
        Ddi_BddPartInsert(unroll, 0, init);
        Ddi_BddComposeAcc(unroll, ns, newNsLit);

        target_i = Ddi_BddCompose(target, ps, newNsLit);

        //invarspec_i = Ddi_BddCompose(invarspec,pi,newPiLit);

        lastNsLit = Ddi_BddarrayDup(newNsLit);
        //lastNsLit = newNsLit;

        /* if (i<step-1) {
           Pdtutil_Assert(1,"wtf");    //ignore, it's not working
           Ddi_BddNotAcc(invarspec_i);
           Ddi_BddOrAcc(unroll,invarspec_i);
           }
           else { */
        //Ddi_BddAndAcc(unroll,invarspec_i);   //it works
        /*   } */

        //Ddi_Free(invarspec_i);
        Ddi_Free(newPi);
        Ddi_Free(newPiLit);
        Ddi_Free(newNs);
        Ddi_Free(newNsLit);
      }
      //END first round

    } else {

      //further round
      if (!sat) {
        int j;

        //Ddi_Bdd_t *invarspec_i;
        Ddi_Vararray_t *newPi =
          Ddi_VararrayMakeNewAigVars(pi, "PDT_TF_VAR", stepString);
        Ddi_Bddarray_t *newPiLit = Ddi_BddarrayMakeLiteralsAig(newPi, 1);
        Ddi_Vararray_t *newNs =
          Ddi_VararrayMakeNewAigVars(ns, "PDT_TF_VAR", stepString);
        Ddi_Bddarray_t *newNsLit = Ddi_BddarrayMakeLiteralsAig(newNs, 1);

        tr_i = Ddi_BddCompose(tr, pi, newPiLit);
        Ddi_BddComposeAcc(tr_i, ps, lastNsLit);
        Ddi_BddComposeAcc(tr_i, ns, newNsLit);

        //Ddi_Free(target_i); //useless, i already free it before the first if  
        target_i = Ddi_BddCompose(target, ps, newNsLit);

        //invarspec_i = Ddi_BddCompose(invarspec,pi,newPiLit);

        Ddi_BddPartInsertLast(unroll, tr_i);
        //  Ddi_BddAndAcc(unroll,tr_i);

        /* if (i<step-1) {
           Pdtutil_Assert(1,"wtf");    //ignore, it's not working
           Ddi_BddNotAcc(invarspec_i);
           Ddi_BddOrAcc(unroll,invarspec_i);
           }
           else { */
        //Ddi_BddAndAcc(unroll,invarspec_i);   //it works
        /*   } */

        Ddi_Free(lastNsLit);
        lastNsLit = Ddi_BddarrayDup(newNsLit);
        //lastNsLit = newNsLit;

        Ddi_Free(tr_i);
        //Ddi_Free(invarspec_i);
        Ddi_Free(newPi);
        Ddi_Free(newPiLit);
        Ddi_Free(newNs);
        Ddi_Free(newNsLit);
      }
      //END further round
    }

  }

  if (!sat) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(tMgrO(travMgr), "BMC pass at bound %d.\n", bound));
  }
  Trav_MgrSetAssertFlag(travMgr, sat);

  Ddi_Free(target_i);
  Ddi_Free(target);
  Ddi_Free(lastNsLit);
  Ddi_Free(unroll);
  Ddi_Free(psLit);
  Ddi_Free(nsLit);
  Ddi_Free(tr);
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Trav_TravBmcSharedInit(void)
{

  bmcShared.active = 1;
#if !USE_RWLOCK
  pthread_mutex_init(&(bmcShared.lock), NULL);
  pthread_mutex_lock(&(bmcShared.lock));
#else
  pthread_rwlock_init(&(bmcShared.lock), NULL);
  pthread_rwlock_wrlock(&(bmcShared.lock));
#endif

  bmcShared.nActive = 0;
  bmcShared.nBounds = 0;
  bmcShared.boundsSize = 100;
  bmcShared.bounds = Pdtutil_Alloc(Trav_BmcBoundState_e, bmcShared.boundsSize);
  bmcShared.maxSafe = -1;

#if !USE_RWLOCK
  pthread_mutex_unlock(&(bmcShared.lock));
#else
  pthread_rwlock_unlock(&(bmcShared.lock));
#endif  
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Trav_TravBmcSharedQuit(void)
{
#if !USE_RWLOCK
  pthread_mutex_lock(&(bmcShared.lock));
#else
  pthread_rwlock_wrlock(&(bmcShared.lock));
#endif
  bmcShared.active = 0;
  bmcShared.nBounds = 0;
  bmcShared.boundsSize = 0;
  Pdtutil_Free(bmcShared.bounds);
  bmcShared.maxSafe = -1;

#if !USE_RWLOCK
  pthread_mutex_unlock(&(bmcShared.lock)); 
#else
  pthread_rwlock_unlock(&(bmcShared.lock)); 
#endif
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Trav_TravBmcSharedSet(
  int k,
  Trav_BmcBoundState_e state
)
{
  int error = 0;
  if (!bmcShared.active) return -1;

#if !USE_RWLOCK
  pthread_mutex_lock(&(bmcShared.lock));
#else 
  pthread_rwlock_wrlock(&(bmcShared.lock));
#endif

  error = travBmcSharedSetLocked(k,state);

#if !USE_RWLOCK
  pthread_mutex_unlock(&(bmcShared.lock));
#else
  pthread_rwlock_unlock(&(bmcShared.lock));
#endif

  if (error) {
    Pdtutil_Assert(0,"problem in BMC bound state");
  }
  return 1;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
travBmcSharedSetLocked(
  int k,
  Trav_BmcBoundState_e state
)
{
  int error = 0;

  if (k>=bmcShared.nBounds) {
    int oldB = bmcShared.nBounds;
    bmcShared.nBounds = k+1;
    while (bmcShared.nBounds>bmcShared.boundsSize) {
      bmcShared.boundsSize *= 2;
      bmcShared.bounds = Pdtutil_Realloc(Trav_BmcBoundState_e, 
			   bmcShared.bounds, bmcShared.boundsSize);
    }
    while (oldB<=k) {
      bmcShared.bounds[oldB++] = Trav_BmcBoundNone_c;
    }
  }

  switch (bmcShared.bounds[k]) {
  case Trav_BmcBoundActive_c:
    bmcShared.bounds[k] = state;
    if (state == Trav_BmcBoundProved_c) {
      while (bmcShared.maxSafe < k) {
	if (bmcShared.bounds[bmcShared.maxSafe+1] != Trav_BmcBoundProved_c)
	  break;
	else bmcShared.maxSafe++;
      }
    }
    break;
  case Trav_BmcBoundProved_c:
  case Trav_BmcBoundFailed_c:
    error = state!=Trav_BmcBoundActive_c && bmcShared.bounds[k]!=state;
    break;
  case Trav_BmcBoundNone_c:
    error = state!=Trav_BmcBoundActive_c;
    bmcShared.bounds[k] = state;
    break;
  default:
    error = 1;
  }

  return error;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Trav_BmcBoundState_e 
Trav_TravBmcSharedRead(
  int k
)
{
  Trav_BmcBoundState_e state;
  int error = 0;
  if (!bmcShared.active) return Trav_BmcBoundNone_c;

#if !USE_RWLOCK
  pthread_mutex_lock(&(bmcShared.lock));
#else
  pthread_rwlock_rdlock(&(bmcShared.lock));
#endif

  if (k>=bmcShared.nBounds) {
    state = Trav_BmcBoundNone_c;
  }
  else {
    state = bmcShared.bounds[k];
  }

#if !USE_RWLOCK
  pthread_mutex_unlock(&(bmcShared.lock)); 
#else
  pthread_rwlock_unlock(&(bmcShared.lock)); 
#endif

  return state;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Trav_BmcBoundState_e 
Trav_TravBmcSharedReadAndSet(
  int k,
  Trav_BmcBoundState_e stateNew
)
{
  Trav_BmcBoundState_e state;
  int error = 0;
  if (!bmcShared.active) return Trav_BmcBoundNone_c;

  if (stateNew == Trav_BmcBoundActive_c) {
    // just read in case already active
    state = Trav_TravBmcSharedRead(k);
    if (state == stateNew) return state;
  }

#if !USE_RWLOCK
  pthread_mutex_lock(&(bmcShared.lock));
#else
  pthread_rwlock_wrlock(&(bmcShared.lock));
#endif

  if (k>=bmcShared.nBounds) {
    state = Trav_BmcBoundNone_c;
  }
  else {
    state = bmcShared.bounds[k];
  }

  error = travBmcSharedSetLocked(k,stateNew);

#if !USE_RWLOCK
  pthread_mutex_unlock(&(bmcShared.lock)); 
#else
  pthread_rwlock_unlock(&(bmcShared.lock)); 
#endif

  if (error) {
    Pdtutil_Assert(0,"problem in BMC bound state");
  }

  return state;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Trav_TravBmcSharedMaxSafe(void)
{
  int ret;

  if (!bmcShared.active) return -1;

#if !USE_RWLOCK
  pthread_mutex_lock(&(bmcShared.lock));
#else 
  pthread_rwlock_rdlock(&(bmcShared.lock));
#endif

  ret = bmcShared.maxSafe;

#if !USE_RWLOCK
  pthread_mutex_unlock(&(bmcShared.lock)); 
#else
  pthread_rwlock_unlock(&(bmcShared.lock)); 
#endif

  return ret;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Trav_TravBmcSharedRegisterThread(void)
{
  int ret;

  if (!bmcShared.active) return -1;

#if !USE_RWLOCK
  pthread_mutex_lock(&(bmcShared.lock));
#else 
  pthread_rwlock_wrlock(&(bmcShared.lock));
#endif

  ret = bmcShared.nActive++;

#if !USE_RWLOCK
  pthread_mutex_unlock(&(bmcShared.lock)); 
#else
  pthread_rwlock_unlock(&(bmcShared.lock)); 
#endif

  return ret;
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Trav_TravBmcSharedActive(void)
{
  return bmcShared.active;
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
static Ddi_Vararray_t *
composeWithNewTimeFrameVars(
  Ddi_Bdd_t * f,
  Ddi_Vararray_t * ps,
  Ddi_Bddarray_t * delta,
  Ddi_Vararray_t * pi,
  char *varNamePrefix,
  int frameSuffix
)
{
  int j;
  Ddi_Bddarray_t *delta_i;
  Ddi_Bdd_t *invarspec_i;
  Ddi_Vararray_t *newPi;
  Ddi_Bddarray_t *newPiLits;
  char suffix[20];

  sprintf(suffix, "%d", frameSuffix);
  newPi = Ddi_VararrayMakeNewVars(pi, varNamePrefix, suffix, 1);
  newPiLits = Ddi_BddarrayMakeLiteralsAig(newPi, 1);

  delta_i = Ddi_BddarrayDup(delta);
  Ddi_BddarrayComposeAcc(delta_i, pi, newPiLits);

  Ddi_BddComposeAcc(f, ps, delta_i);
  Ddi_Free(delta_i);
  Ddi_Free(newPiLits);

  return (newPi);

}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bddarray_t *
unrollWithNewTimeFrameVars(
  Ddi_Bddarray_t * delta,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * pi,
  char *varNamePrefix,
  Ddi_Varsetarray_t *piSets,
  int nFrames
)
{
  int j;
  Ddi_Bddarray_t *delta_i;
  Ddi_Bddarray_t *unroll;
  Ddi_Bdd_t *invarspec_i;
  Ddi_Vararray_t *newPi;
  Ddi_Bddarray_t *newPiLits;
  char suffix[20];

  unroll = Ddi_BddarrayMakeLiteralsAig(ps, 1);

  for (j=0; j<nFrames; j++) {
    sprintf(suffix, "%d", j);
    newPi = Ddi_VararrayMakeNewVars(pi, varNamePrefix, suffix, 0);
    newPiLits = Ddi_BddarrayMakeLiteralsAig(newPi, 1);
    
    delta_i = Ddi_BddarrayDup(delta);
    Ddi_BddarrayComposeAcc(delta_i, pi, newPiLits);
    
    Ddi_BddarrayComposeAcc(delta_i, ps, unroll);
    Ddi_Free(unroll);
    unroll = delta_i;
    Ddi_Free(newPiLits);
    if (piSets!=NULL) {
      Ddi_Varset_t *newPiSet = Ddi_VarsetMakeFromArray(newPi);
      Ddi_VarsetarrayInsertLast(piSets, newPiSet);
      Ddi_Free(newPiSet);
    } 
    Ddi_Free(newPi);
  }

  return (unroll);

}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
bmcCoiFilter(
  Ddi_Bdd_t * f,
  Ddi_Bdd_t * filter,
  Ddi_Bdd_t * care,
  Ddi_Vararray_t * cutVars
)
{
  int sat, i;
  Ddi_Bdd_t *fRed;
  Ddi_Varset_t *supp;
  Ddi_Vararray_t *vars;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  bAig_Manager_t *bmgr = ddm->aig.mgr;

  Pdtutil_Assert(Ddi_BddIsPartConj(f), "conj part required");
  Pdtutil_Assert(Ddi_BddPartNum(f) == Ddi_VararrayNum(cutVars),
    "conj part an var num do not match");
  supp = Ddi_BddSupp(filter);
  vars = Ddi_VararrayMakeFromVarset(supp, 1);
  Ddi_Free(supp);
  for (i = 0; i < Ddi_VararrayNum(vars); i++) {
    Ddi_Var_t *v_i = Ddi_VararrayRead(vars, i);
    bAigEdge_t varIndex = Ddi_VarToBaig(v_i);

    Pdtutil_Assert(bAig_AuxInt(bmgr, varIndex) == -1,
      "aux int field not clean");
    bAig_AuxInt(bmgr, varIndex) = 1;
  }
  fRed = Ddi_BddMakePartConjVoid(ddm);
  for (i = 0; i < Ddi_VararrayNum(cutVars); i++) {
    Ddi_Var_t *v_i = Ddi_VararrayRead(cutVars, i);
    bAigEdge_t varIndex = Ddi_VarToBaig(v_i);

    if (bAig_AuxInt(bmgr, varIndex) > 0) {
      /* common var, take a component */
      Ddi_Bdd_t *f_i = Ddi_BddPartRead(f, i);

      Ddi_BddPartInsertLast(fRed, f_i);
    }
  }
  for (i = 0; i < Ddi_VararrayNum(vars); i++) {
    Ddi_Var_t *v_i = Ddi_VararrayRead(vars, i);
    bAigEdge_t varIndex = Ddi_VarToBaig(v_i);

    bAig_AuxInt(bmgr, varIndex) = -1;
  }
  Ddi_Free(vars);

  return fRed;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static int
bmcSatAndWithAbort(
  Ddi_Bdd_t * a,
  Ddi_Bdd_t * b,
  Ddi_Bddarray_t * pre,
  Ddi_Bdd_t * care,
  Ddi_Vararray_t * cutVars,
  Ddi_Varset_t * cexVars,
  int satPart,
  int timeLimit
)
{
  int sat = 0, i;
  Ddi_Bdd_t *itp, *a2;
  Ddi_Varset_t *suppB = NULL, *suppBTot, *suppAB = NULL;
  Ddi_Vararray_t *bVars;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(a);
  bAig_Manager_t *bmgr = ddm->aig.mgr;

  Pdtutil_Assert(Ddi_BddIsPartConj(a), "conj part required");
  Pdtutil_Assert(Ddi_BddPartNum(a) <= Ddi_VararrayNum(cutVars),
    "conj part an var num do not match");

#if 0
  a2 = Ddi_BddMakeAig(a);
  suppAB = Ddi_BddSupp(a2);
  Ddi_Free(a2);
  suppB = Ddi_BddSupp(b);
  Ddi_VarsetSetArray(suppB);
  Ddi_VarsetSetArray(suppAB);

  Ddi_VarsetIntersectAcc(suppAB, suppB);
  Ddi_VarsetDiffAcc(suppB, suppAB);
#else
  suppB = Ddi_VarsetDup(cexVars);
#endif

  if (satPart <= 0) {
    Ddi_Free(suppB);
    Ddi_Free(suppAB);
    return Ddi_AigSatAndWithAbort(a, b, care, timeLimit);
  } else {
    Ddi_Bdd_t *myCare = Ddi_BddMakeConstAig(ddm, 1);
    int endPart = 0;
    int sizeB = Ddi_BddSize(b);

    suppBTot = Ddi_VarsetDup(suppB);
    for (i = 0; i < satPart && !endPart; i++) {
      Ddi_Bdd_t *cubeWindow = Ddi_BddMakeConstAig(ddm, 0);
      Ddi_Varset_t *sm = Ddi_VarsetDup(suppB);
      Ddi_Bdd_t *bb = Ddi_BddDup(b);

      Ddi_VarsetSetArray(sm);
      if (i < satPart - 1) {
        Ddi_AigExistAllSolutionAcc(bb, sm, myCare, cubeWindow, 1);
      }
      Ddi_Free(sm);

      if (Ddi_BddIsZero(bb)) {
        endPart = 1;
        fprintf(dMgrO(ddm), "partitioned SAT ended\n");
      } else {
        int cubeSize = Ddi_BddSize(cubeWindow);

        fprintf(dMgrO(ddm), "partition %d - size: %d/%d\n", i,
          Ddi_BddSize(bb), sizeB);
        //  itp = Ddi_AigSatAndWithInterpolant (a,bb,suppAB,NULL,cubeWindow,
        //                      NULL,&sat,0,0,-1.0);
        if (0) {
          itp = Ddi_AigSatAndWithInterpolant(a, b,
            suppAB, NULL, NULL, NULL, cubeWindow, NULL, &sat, 0, 0, -1.0);
        } else {
          Ddi_Bdd_t *chk = Ddi_BddAnd(a, b);

          //      Ddi_Bdd_t *chk = Ddi_BddCompose(b,cutVars,pre); 
          //      Ddi_BddAndAcc(chk,myCare);
          sat = Ddi_AigSatMinisatWithAbortAndFinal(chk, cubeWindow, -1, 0);
          Ddi_Free(chk);
          if (sat) {
            itp = NULL;
          } else {
            itp = Ddi_BddMakeConstAig(ddm, 1);
          }
        }
        if (itp == NULL) {
          endPart = 1;
        } else {
          Ddi_Free(suppB);
          if (1 || (Ddi_BddSize(cubeWindow) < cubeSize)) {
            suppB = Ddi_BddSupp(cubeWindow);
            if (0) {
              Ddi_Vararray_t *va = Ddi_VararrayMakeFromVarset(suppB, 1);
              int nv = Ddi_VararrayNum(va);

              if (nv > 1) {
                Ddi_Var_t *v = Ddi_VararrayRead(va, nv - 1);

                Ddi_VarsetRemoveAcc(suppB, v);
              }
              Ddi_Free(va);
            }
          } else {
            suppB = Ddi_VarsetDup(suppBTot);
          }
          fprintf(dMgrO(ddm), "size: %d - supp: %d\n\n",
            Ddi_BddSize(itp), Ddi_VarsetNum(suppB));
          Ddi_BddAndAcc(myCare, itp);
          Ddi_BddDiffAcc(myCare, cubeWindow);
          Ddi_BddDiffAcc(myCare, bb);
          Ddi_Free(itp);
        }
      }
      Ddi_Free(cubeWindow);
      Ddi_Free(bb);
      Ddi_Free(sm);

    }
    Ddi_Free(suppBTot);
    Ddi_Free(itp);
    Ddi_Free(myCare);
  }
  Ddi_Free(suppAB);
  Ddi_Free(suppB);

  return sat;
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
bmcTfSplitCex(
  TravBmcMgr_t * bmcMgr,
  Ddi_Bdd_t * cex,
  int nTf
)
{
  int jj, ii, maxTf=nTf-1;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(cex);
  Ddi_Bdd_t *returnedCex;
  Ddi_Bdd_t *cexInitState = NULL;

  // mark cex vars
  Ddi_Bdd_t *cexPart = Ddi_AigPartitionTop(cex, 0);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(bmcMgr->fsmMgr);
  int nxl = Fsm_MgrReadXInitLatches(bmcMgr->fsmMgr);
  int initStubSteps = Fsm_MgrReadInitStubSteps(bmcMgr->fsmMgr);
  
  if (bmcMgr->bwdTimeFrameVars != NULL) { 
    int nbTf = Ddi_VarsetarrayNum(bmcMgr->bwdTimeFrameVars);
    if (nbTf > 0) {
      Ddi_Varset_t *voidTf = Ddi_VarsetVoid(ddm); 
      while (Ddi_VarsetarrayNum(bmcMgr->timeFrameVars)>nTf) {
	int last = Ddi_VarsetarrayNum(bmcMgr->timeFrameVars)-1;
	Ddi_VarsetarrayRemove(bmcMgr->timeFrameVars,last);
      }
      Ddi_VarsetarrayAppend(bmcMgr->timeFrameVars,bmcMgr->bwdTimeFrameVars);
      Ddi_VarsetarrayInsertLast(bmcMgr->timeFrameVars,voidTf);
      nTf += nbTf+1;
      maxTf += nbTf+1;
      Ddi_Free(voidTf);
    }
  }

  if (maxTf < 0)
    maxTf = Ddi_VarsetarrayNum(bmcMgr->timeFrameVars) - 1;
  Pdtutil_Assert(maxTf<Ddi_VarsetarrayNum(bmcMgr->timeFrameVars),
		 "missing time frame vars");

  returnedCex = Ddi_BddMakePartConjVoid(ddm);
  for (jj = 0; jj < Ddi_BddPartNum(cexPart); jj++) {
    Ddi_Bdd_t *p = Ddi_BddPartRead(cexPart, jj);
    int isCompl = Ddi_BddIsComplement(p);
    Ddi_Var_t *v_j = Ddi_BddTopVar(p);

    Pdtutil_Assert(Ddi_BddSize(p) == 1, "Wrong partition cube");
    Ddi_VarWriteMark(v_j, isCompl ? -1 : 1);
  }
  for (jj = 0; jj < Ddi_VararrayNum(ps); jj++) {
    Ddi_Var_t *v_j = Ddi_VararrayRead(ps, jj);
    int m = Ddi_VarReadMark(v_j);

    if (m==0 && nxl>=0 && initStubSteps>0) {
      char *refName = Ddi_VarName(v_j);
      char stubName[1000];
      sprintf(stubName,"PDT_STUB_PS_%s", refName);
      Ddi_Var_t *v_j_s = Ddi_VarFromName(ddm, stubName);
      if (v_j_s!=NULL) {
        m = Ddi_VarReadMark(v_j_s);
        // reset mark to disable var as init stub PI
        Ddi_VarWriteMark(v_j_s, 0); 
      }
    }
    if (m != 0) {
      // ps in cex. Refine initstate
      Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(v_j, m == -1 ? 0 : 1);

      if (cexInitState == NULL) {
        cexInitState = lit;
      } else {
        Ddi_BddAndAcc(cexInitState, lit);
        Ddi_Free(lit);
      }
      Ddi_VarWriteMark(v_j, 0);
    }
  }
  if (cexInitState != NULL) {
    Ddi_SetName(cexInitState, "PDT_CEX_IS");
    Ddi_BddPartInsertLast(returnedCex, cexInitState);
    Ddi_Free(cexInitState);
  }

  Ddi_VararrayCheckMark(ps, 0);
  for (jj = 0; jj <= maxTf; jj++) {
    Ddi_Bdd_t *tfCex = Ddi_BddMakeConstAig(ddm, 1);
    Ddi_Vararray_t *tfSuppA =
      Ddi_VararrayMakeFromVarset(Ddi_VarsetarrayRead(bmcMgr->timeFrameVars,
        jj), 1);

    for (ii = 0; ii < Ddi_VararrayNum(tfSuppA); ii++) {
      Ddi_Var_t *v_ii = Ddi_VararrayRead(tfSuppA, ii);

      if (Ddi_VarReadMark(v_ii) != 0) {
        Ddi_Bdd_t *lit =
          Ddi_BddMakeLiteralAig(v_ii, Ddi_VarReadMark(v_ii) == -1 ? 0 : 1);
        Ddi_BddAndAcc(tfCex, lit);
        Ddi_Free(lit);
      }
    }
    Ddi_Free(tfSuppA);
    Ddi_BddPartInsertLast(returnedCex, tfCex);
    Ddi_Free(tfCex);
  }
  for (jj = 0; jj < Ddi_BddPartNum(cexPart); jj++) {
    Ddi_Bdd_t *p = Ddi_BddPartRead(cexPart, jj);

    Pdtutil_Assert(Ddi_BddSize(p) == 1, "Wrong partition cube");
    Ddi_VarWriteMark(Ddi_BddTopVar(p), 0);
  }
  Ddi_Free(cexPart);
  //  Ddi_BddPartInsertLast(returnedCex,cex);

  return returnedCex;
}



/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static int
bmcOutOfLimits(
  TravBmcMgr_t  *bmcMgr
)
{
  int memLimit = 1000 * bmcMgr->travMgr->settings.aig.bmcMemLimit;
  unsigned long time_limit = 1000*
    (unsigned long)bmcMgr->travMgr->settings.aig.bmcTimeLimit;

  if (time_limit > 0 && (util_cpu_time() > time_limit)) {
    Pdtutil_VerbosityMgrIf(bmcMgr->travMgr, Pdtutil_VerbLevelUsrMed_c) {
      fprintf(tMgrO(bmcMgr->travMgr), "Time limit reached.\n");
    }
    return 1;
  }
  if (memLimit > 0) {
    if (Pdtutil_ProcRss() > memLimit) {
      Pdtutil_VerbosityMgrIf(bmcMgr->travMgr, Pdtutil_VerbLevelUsrMed_c) {
	fprintf(tMgrO(bmcMgr->travMgr), "BMC Mem limit reached.\n");
      }
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
timeFrameShiftKAcc(
  Ddi_Bdd_t * f,
  Ddi_Varsetarray_t *timeFrameVars,
  int k
)
{
  int i;
  int nTimeFrames = Ddi_VarsetarrayNum(timeFrameVars);
  Ddi_Vararray_t *vA = NULL;
  Ddi_Vararray_t *vA0 = NULL;

  if (k<=0) return;
  Pdtutil_Assert(k >= 1, "wrong k in time frame shift");

  for (i = nTimeFrames - 1; i >= k; i--) {
    Ddi_Varset_t *vs_i = Ddi_VarsetarrayRead(timeFrameVars,i);
    Ddi_Varset_t *vs_i0 = Ddi_VarsetarrayRead(timeFrameVars,i-k);
    Ddi_Vararray_t *va_i = Ddi_VararrayMakeFromVarset(vs_i,1);
    Ddi_Vararray_t *va_i0 = Ddi_VararrayMakeFromVarset(vs_i0,1);
    if (vA == NULL) {
      vA = Ddi_VararrayDup(va_i);
      vA0 = Ddi_VararrayDup(va_i0);
    } else {
      Ddi_VararrayAppend(vA, va_i);
      Ddi_VararrayAppend(vA0, va_i0);
    }
    Ddi_Free(va_i);
    Ddi_Free(va_i0);
  }
  Pdtutil_Assert(vA != NULL, "NULL var array in time frame shift");
  Ddi_BddSubstVarsAcc(f, vA0, vA);

  Ddi_Free(vA);
  Ddi_Free(vA0);
}

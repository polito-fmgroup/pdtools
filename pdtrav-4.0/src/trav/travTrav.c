/**CFile***********************************************************************

  FileName    [travTrav.c]

  PackageName [trav]

  Synopsis    [Functions to traverse a FSM]

  Description [External procedures included in this file are:
    <ul>
    <li>Trav_Traversal()
    </ul>
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

#include "travInt.h"
#include "trInt.h"
#include "ddiInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define PRUNING_NUMBER_MAX 20
#define THRESHOLD_FACTOR 2
#define TIME_COST_FACTOR 0.5
#define MEM_COST_FACTOR 0.25

#define ENABLE_CONFLICT 0
#define ENABLE_INVAR 0
#define PARTITIONED_RINGS 0
#define DOUBLE_MGR 1

#define FILEPAT   "/tmp/pdtpat"
#define FILEINIT  "/tmp/pdtinit"

#define LOG_CONST(f,msg) \
if (Ddi_BddIsOne(f)) {\
  fprintf(stdout, "%s = 1\n", msg);            \
}\
else if (Ddi_BddIsZero(f)) {\
  fprintf(stdout, "%s = 0\n", msg);            \
}

#define LOG_BDD_CUBE_PLA(f,str) \
\
    {\
      Ddi_Varset_t *supp;\
\
      fprintf(stdout, ".names ");            \
      if (f==NULL) \
        fprintf(stdout, "NULL!\n");\
      else if (Ddi_BddIsAig(f)) {\
        Ddi_Bdd_t *f1 = Ddi_BddMakeMono(f);\
        supp = Ddi_BddSupp(f1);\
        Ddi_VarsetPrint(supp, 10000, NULL, stdout);\
        fprintf(stdout, " %s", str);                        \
        Ddi_BddPrintCubes(f1, supp, 100, 1, NULL, stdout);\
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
        Ddi_VarsetPrint(supp, 10000, NULL, stdout);\
        fprintf(stdout, " %s", str);                        \
        Ddi_BddPrintCubes(f, supp, 100, 1, NULL, stdout);\
        Ddi_Free(supp);\
      }\
    }

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

/* if 0 allows don't cares within input patterns */
#define NO_PI_DONT_CARES 1
/* if 0 allows don't cares within counterexample states */
#define NO_PS_DONT_CARES 1

/*
 *  If 0 allows don't cares within initial state
 *  At present no don't cares allowed.
 *  So don't change next definition for now: not yet supported
 */
#define NO_IS_DONT_CARES 1

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static Ddi_Bdd_t *FindSubsetOfItersectionMono(
  Ddi_Bdd_t * set,
  Ddi_Bdd_t * ring,
  Ddi_Varset_t * smoothV,
  int optLevel
);
static Ddi_Bdd_t *orProjectedSetAcc(
  Ddi_Bdd_t * f,
  Ddi_Bdd_t * g,
  Ddi_Var_t * partVar
);
static Ddi_Bdd_t *diffProjectedSetAcc(
  Ddi_Bdd_t * f,
  Ddi_Bdd_t * g,
  Ddi_Var_t * partVar
);
static int
 travOverRingsWithFixPoint(
  Ddi_Bdd_t * from0,
  Ddi_Bdd_t * reached0,
  Ddi_Bdd_t * target,
  Tr_Tr_t * TR,
  Ddi_Bdd_t * inRings,
  Ddi_Bdd_t * outRings,
  Ddi_Bdd_t * overApprox,
  int maxIter,
  char *iterMsg,
  long initialTime,
  int reverse,
  int approx,
  Trav_Settings_t * opt
);
static Ddi_Bdd_t *dynAbstract(
  Ddi_Bdd_t * f,
  int maxSize,
  int maxAbstract
);
static Ddi_Bdd_t *findNonVacuumHint(
  Ddi_Bdd_t * from,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * pi,
  Ddi_Vararray_t * aux,
  Ddi_Bdd_t * reached,
  Ddi_Bddarray_t * delta,
  Ddi_Bdd_t * trBdd,
  Ddi_Bdd_t * hConstr,
  Ddi_Bdd_t * prevHint,
  int nVars,
  int hintsStrategy
);
static int
 travOverRings(
  Ddi_Bdd_t * from0,
  Ddi_Bdd_t * reached0,
  Ddi_Bdd_t * target,
  Tr_Tr_t * TR,
  Ddi_Bdd_t * inRings,
  Ddi_Bdd_t * outRings,
  Ddi_Bdd_t * overApprox,
  int maxIter,
  char *iterMsg,
  long initialTime,
  int reverse,
  int approx,
  Trav_Settings_t * opt
);
static int
 checkSetIntersect(
  Ddi_Bdd_t * set,
  Ddi_Bdd_t * target,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * care2,
  int doAnd,
  int imgClustTh
);
static Ddi_Bdd_t *bckInvarCheckRecur(
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * target,
  Tr_Tr_t * TR,
  Ddi_Bdd_t * rings,
  Trav_Settings_t * opt
);
static Ddi_Bdd_t *bckInvarCheckRecurStep(
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * target,
  Ddi_Bdd_t * rings,
  Ddi_Bdd_t * Rplus,
  Ddi_Bdd_t * careRings,
  Tr_Tr_t * TR,
  int level,
  int nestedFull,
  int nestedPart,
  int innerFail,
  int nestedCalls,
  int noPartition,
  Trav_Settings_t * opt
);
static void
 analyzeConflict(
  Tr_Tr_t * TR,
  Ddi_Bdd_t * inner,
  Ddi_Bdd_t * same,
  Ddi_Bdd_t * outer,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * from,
  Trav_Settings_t * opt
);
static int
 travBddCexToFsm(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * cexArray,
  Ddi_Bdd_t * initState,
  int reverse
);
static void
 storeCNFRings(
  Ddi_Bdd_t * fwdRings,
  Ddi_Bdd_t * bckRings,
  Ddi_Bdd_t * TrBdd,
  Ddi_Bdd_t * prop,
  Trav_Settings_t * opt
);
static void
 storeRingsCNF(
  Ddi_Bdd_t * rings,
  Trav_Settings_t * opt
);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of external functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [FSM traversal given a transition relation]

  Description [The pseudo-code of traversal algorithm is:<br>
    <code>
    traverse ( delta , S0 )<br>
      {<br>
      reached = from = S0;<br>
      do<br>
        {<br>
        to = Img  ( delta , from );<br>
        new = to - reached;<br>
        reached = reached + new;<br>
        from = new;<br>
        }<br>
      while ( new!= 0 )<br>
      <p>
      return reached ;<br>
    }<br>
    </code>
    We use the following notations:
    <ul>
    <li>from = initial set of states
    <li>to  = set of reached state in one step from current states
    <li>new  = new reached states
    <li>reached  = set of reached states from 0 up to current
    iteration
    </ul>]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Trav_Traversal(
  Trav_Mgr_t * travMgr          /* Traversal Manager */
)
{
  Ddi_Mgr_t *dd, *ddR;
  Ddi_Vararray_t *psv, *nsv;
  Ddi_Varset_t *quantify, *inputs;
  Ddi_Varset_t *part_vars = NULL;
  Ddi_Bdd_t *newr, *from, *to, *reached, *careNS;
  Tr_Tr_t *trActive;
  Pdtutil_VerbLevel_e verbosity, verbositySave;
  double nStates;
  long initialTime, imgTime, currTime1, currTime2;
  long currMem1, currMem2;
  int step, iterNumberMax, travContinue, toIncluded,
    logPeriod, savePeriod, toSize, l;
  char *periodNameSave, *nameTmp;
  int extRef, nodeId;

#if 0
  {
    extern int Cuplus_EnableAndAbs;

    Cuplus_EnableAndAbs = 1;
  }
#endif
  /*
   *  Check for Correctness and Get Basic Objects
   */

  Pdtutil_Assert(travMgr != NULL, "NULL traversal manager");

  dd = travMgr->ddiMgrTr;
  ddR = travMgr->ddiMgrR;

  extRef = Ddi_MgrReadExtRef(ddR);

  trActive = Tr_TrDup(Trav_MgrReadTr(travMgr));

  psv = Trav_MgrReadPS(travMgr);
  nsv = Trav_MgrReadNS(travMgr);

  quantify = Ddi_VarsetMakeFromArray(psv);
  if ((Trav_MgrReadSmoothVar(travMgr) == TRAV_SMOOTH_SX)
    && (Trav_MgrReadI(travMgr) != NULL)) {
    inputs = Ddi_VarsetMakeFromArray(Trav_MgrReadI(travMgr));
    Ddi_VarsetUnionAcc(quantify, inputs);
    Ddi_Free(inputs);
  }

  verbosity = Trav_MgrReadVerbosity(travMgr);
  iterNumberMax = Trav_MgrReadMaxIter(travMgr);
  logPeriod = Trav_MgrReadLogPeriod(travMgr);

  savePeriod = Trav_MgrReadSavePeriod(travMgr);
  if (Trav_MgrReadSavePeriodName(travMgr) != NULL) {
    periodNameSave = Pdtutil_StrDup(Trav_MgrReadSavePeriodName(travMgr));
  } else {
    periodNameSave = Pdtutil_StrDup("");
  }

  Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c, if (ddR != dd) {
    fprintf(stdout, "Two dd Managers Used for Traversal.\n");}
  ) ;

  reached = Ddi_BddCopy(ddR, travMgr->reached);
  from = Ddi_BddCopy(ddR, travMgr->from);
  Ddi_Free(travMgr->from);
  Ddi_Free(travMgr->reached);
  travMgr->from = from;
  travMgr->reached = reached;

  /*
   *  Initialize frontier set array if option enabled
   */

  if ((travMgr->settings.bdd.keepNew) && (travMgr->newi == NULL)) {
    travMgr->newi = Ddi_BddarrayAlloc(ddR, 1);
    Pdtutil_Assert(travMgr->newi != NULL, "NULL newi array in trav manager");

    Ddi_BddarrayWrite(travMgr->newi, 0, travMgr->reached);
  }

  /*
   *  Final Settings Before Cycle
   */

  initialTime = util_cpu_time();
  verbositySave = verbosity;
  step = 1;
  travContinue = (iterNumberMax < 0) || (step <= iterNumberMax);

  if (Ddi_BddIsPartConj(travMgr->reached)) {
    Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Partitioned Reached: %d (#Component=%d).\n",
        Ddi_BddSize(travMgr->reached), Ddi_BddPartNum(travMgr->reached))
      );
    Ddi_BddSetClustered(travMgr->reached, 5000);
    Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Clustered Reached: %d (#Component=%d).\n",
        Ddi_BddSize(travMgr->reached), Ddi_BddPartNum(travMgr->reached))
      );
    Ddi_Free(travMgr->from);

    travMgr->from = Ddi_BddDup(travMgr->reached);
  }
#if 1
  if ((!Ddi_BddIsMeta(travMgr->reached))) {
    if (Ddi_MetaActive(dd)) {
      if (Ddi_BddEqual(travMgr->reached, travMgr->from)) {
        if (Ddi_BddIsPartConj(travMgr->reached)) {
          Ddi_Varset_t *novars = Ddi_VarsetVoid(dd);

          Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
            fprintf(stdout, "Init Meta Reached: %d (#Component=%d).\n",
              Ddi_BddSize(travMgr->reached), Ddi_BddPartNum(travMgr->reached))
            );
          Ddi_BddSetMeta(travMgr->reached);
          Ddi_BddExistAcc(travMgr->reached, novars);
          Ddi_Free(novars);
          Pdtutil_VerbosityLocal(verbositySave, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "Final Meta Reached: %d.\n",
              Ddi_BddSize(travMgr->reached))
            );
        } else {
          Ddi_BddSetMeta(travMgr->reached);
        }
        /*Ddi_BddSetMono(travMgr->reached); */
        Ddi_Free(travMgr->from);
        travMgr->from = Ddi_BddDup(travMgr->reached);
      } else {
        Ddi_BddSetMeta(travMgr->reached);
        Ddi_BddSetMeta(travMgr->from);
      }
      if (Trav_MgrReadFromSelect(travMgr) != Trav_FromSelectReached_c)
        Trav_MgrSetFromSelect(travMgr, Trav_FromSelectNew_c);
    } else {
      Ddi_BddSetMono(travMgr->reached);
      Pdtutil_VerbosityLocal(verbositySave, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "Final Reached: %d.\n", Ddi_BddSize(travMgr->reached));
#ifndef __alpha__
        nStates = Ddi_BddCountMinterm(travMgr->reached, Ddi_VararrayNum(psv));
        fprintf(stdout, "[#ReachedStates: %g].\n", nStates)
#endif
        );
    }
  }
#endif

  if (0 && travMgr->assume != NULL) {
    inputs = Ddi_VarsetMakeFromArray(Trav_MgrReadI(travMgr));
    travMgr->care = Ddi_BddExistAcc(travMgr->assume, inputs);
    Ddi_Free(inputs);
  }


  /*---------------------- Traversal Cycle ... Start ------------------------*/

  nodeId = Ddi_MgrReadCurrNodeId(ddR);

  while (travContinue) {

    /*
     *  Cope with logPeriod through verbosity level
     */

    if ((step % logPeriod) == 0) {
      verbosity = travMgr->settings.verbosity = verbositySave;
    } else {
      verbosity = travMgr->settings.verbosity = Pdtutil_VerbLevelNone_c;
    }

    /*
     *  Check Assertion
     */

    if (Trav_MgrReadAssert(travMgr) != NULL) {
      int fail;
      Ddi_Bdd_t *auxR = Ddi_BddDup(travMgr->reached);

      if (travMgr->care != NULL) {
        Ddi_BddSetPartConj(auxR);
        Ddi_BddPartInsertLast(auxR, travMgr->care);
      }
      fail = !Ddi_BddIncluded(auxR, Trav_MgrReadAssert(travMgr));
      Ddi_Free(auxR);
      if (fail) {
        Pdtutil_VerbosityLocal(verbositySave, Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "Traversal Level %d: Assertion failed.\n",
            travMgr->level)
          );
        Trav_MgrSetAssertFlag(travMgr, 1);
        break;
      }
    }

    /*
     *  Print Statistics for this Iteration - Part 1
     */

    Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "Traversal Level = %d; ", travMgr->level);
      fprintf(stdout, "|Tr|=%d; ", Ddi_BddSize(Tr_TrBdd(trActive)));
      fprintf(stdout, "|From|=%d\n", Ddi_BddSize(travMgr->from))
      );

    /*
     *  Compute image of next state function: to = Img (TR, from)
     */

    currMem1 = Ddi_MgrReadMemoryInUse(dd) / 1024;
    currTime1 = util_cpu_time();
#if 0
    Tr_TrSortIwls95(trActive);
#endif


    if (0 && travMgr->assume != NULL) {

#if 1
      if (Ddi_BddIsMeta(travMgr->from)) {
        Ddi_BddAndAcc(travMgr->from, travMgr->assume);
      } else if (Ddi_BddIsPartDisj(travMgr->from)) {
        int j;

        for (j = 0; j < Ddi_BddPartNum(travMgr->from); j++) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(travMgr->from, j);

          Ddi_BddSetPartConj(p);
          Ddi_BddPartInsertLast(p, travMgr->assume);
        }
      } else {
        Ddi_BddSetPartConj(travMgr->from);
        Ddi_BddPartInsertLast(travMgr->from, travMgr->assume);
      }
#endif
    }

    if (travMgr->care != NULL) {

#if 0
      careNS = Ddi_BddSwapVars(travMgr->care, psv, nsv);
      inputs = Ddi_VarsetMakeFromArray(Trav_MgrReadI(travMgr));
      Ddi_BddExistAcc(careNS, inputs);
      Ddi_Free(inputs);
#else
      careNS = Ddi_BddMakeConst(ddR, 1);
#endif
    } else {
#if 0
      careNS = Ddi_BddSwapVars(travMgr->reached, psv, nsv);
      Ddi_BddNotAcc(careNS);
#else
      careNS = Ddi_BddMakeConst(ddR, 1);
#endif
    }

    //    Tr_MgrSetImgSmoothPi (Tr_TrMgr(trActive), 0);

    if (travMgr->assume != NULL) {
      Ddi_Bdd_t *newTo;
      Ddi_Varset_t *psVars = Ddi_VarsetMakeFromArray(psv);

      Tr_MgrSetImgSmoothPi(Tr_TrMgr(trActive), 0);
      to = Tr_ImgWithConstrain(trActive, travMgr->from, careNS);
      Tr_MgrSetImgSmoothPi(Tr_TrMgr(trActive), 1);
      newTo = Ddi_BddRestrict(travMgr->assume, to);
      Ddi_BddSetPartConj(newTo);
      Ddi_BddPartInsert(newTo, 0, to);
      Ddi_BddExistProjectAcc(newTo, psVars);
      Ddi_Free(psVars);
      Ddi_Free(to);
      to = newTo;
    } else {
      to = Tr_ImgWithConstrain(trActive, travMgr->from, careNS);
    }

    //    inputs = Ddi_VarsetMakeFromArray (Trav_MgrReadI (travMgr));
    //    Ddi_BddExistAcc(to,inputs);
    //    Ddi_Free(inputs);

    if (travMgr->care != NULL) {
      Ddi_BddAndAcc(to, travMgr->care);
    }

    Ddi_Free(careNS);

    Ddi_AndAbsFreeClipWindow(dd);

    currTime2 = util_cpu_time();
    imgTime = currTime2 - currTime1;
    currMem2 = Ddi_MgrReadMemoryInUse(dd) / 1024;
    toSize = Ddi_BddSize(to);

    if (Tr_MgrReadImgMethod(Tr_TrMgr(trActive)) == Tr_ImgMethodApprox_c) {
      Ddi_BddSetMono(to);
    }

    /*
     *  under-approx dynamic abstraction
     */

    if (travMgr->settings.bdd.underApproxMaxVars > 0 ||
      travMgr->settings.bdd.underApproxMaxSize > 0) {
      Ddi_Varset_t *supp = Ddi_BddSupp(to);
      int ns = Ddi_VarsetNum(supp);
      int size = Ddi_BddSize(to);
      int targetAbstrVars = -1, targetSize = -1;

      Ddi_Free(supp);

      if (travMgr->settings.bdd.underApproxMaxVars > 0 &&
        travMgr->settings.bdd.underApproxMaxVars < ns) {
        targetAbstrVars =
          ns - (int)(ns / travMgr->settings.bdd.underApproxTargetReduction);
      }
      if (travMgr->settings.bdd.underApproxMaxSize > 0 &&
        travMgr->settings.bdd.underApproxMaxSize < size) {
        targetSize =
          (int)(size / travMgr->settings.bdd.underApproxTargetReduction);
      }

      if (targetSize > 0 || targetAbstrVars > 0) {
        Ddi_BddDynamicAbstractAcc(to,
          targetSize, targetAbstrVars, 0 /*underAppr */ );

        Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "Dynamic Abstraction: |from|=%d->%d.\n",
            size, Ddi_BddSize(to))
          );
      }
    }

    /*
     *  Check Fixt Point
     */

    if (Ddi_BddIncluded(to, travMgr->reached)) {
      toIncluded = 1;
    } else {
      toIncluded = 0;
    }
    if (travMgr->assume != NULL && !toIncluded) {
      Ddi_Varset_t *suppCare = Ddi_BddSupp(travMgr->assume);
      Ddi_Varset_t *smooth;
      Ddi_Bdd_t *newR;
      Ddi_Bdd_t *rBar = Ddi_BddNot(travMgr->reached);

      Ddi_BddSetPartConj(rBar);
      Ddi_BddPartInsertLast(rBar, to);
      smooth = Ddi_BddSupp(rBar);
      Ddi_VarsetDiffAcc(smooth, suppCare);
      newR = Ddi_BddMultiwayRecursiveAndExistOver(rBar, smooth,
        NULL, travMgr->assume, -1);
      toIncluded = Ddi_BddIsZero(newR);
      Ddi_Free(newR);
      Ddi_Free(rBar);
      Ddi_Free(smooth);
      Ddi_Free(suppCare);
    }

    /*
     *  Prepare Sets for the New Iteration
     */

    /* Ddi_MgrSiftSuspend(dd); */

    /* Compute new from according to the selection option
       and transfer to main manager */

    Ddi_Free(travMgr->from);
    travMgr->from = TravFromCompute(to, travMgr->reached,
      Trav_MgrReadFromSelect(travMgr));

    if (Ddi_MetaActive(dd)) {
      Ddi_Bdd_t *newR = Ddi_BddDiff(travMgr->from, travMgr->reached);

      toIncluded = Ddi_BddIsZero(newR);
      Ddi_Free(newR);
    }
#if 1
    if (Ddi_MetaActive(dd)) {
      Ddi_Varset_t *novars = Ddi_VarsetVoid(dd);

      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "|R(meta)|=%d; ", Ddi_BddSize(travMgr->reached))
        );
      Ddi_BddExistAcc(travMgr->reached, novars);
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "|R(meta2)|=%d; ", Ddi_BddSize(travMgr->reached))
        );
      Ddi_Free(novars);
    }
#endif

    /* Keep frontier set if option enabled */
    if (toIncluded == 0 && travMgr->settings.bdd.keepNew) {
      Pdtutil_Assert(travMgr->newi != NULL, "NULL newi array in trav manager");
      newr = TravFromCompute(to, travMgr->reached, Trav_FromSelectNew_c);
      Ddi_BddSetMono(newr);
      Ddi_BddarrayWrite(travMgr->newi, travMgr->level, newr);
      Ddi_Free(newr);
    }
#if 0
    Ddi_BddSetMono(to);
#endif


    if (travMgr->assume != NULL) {
      Ddi_BddRestrictAcc(to, travMgr->assume);
    }

    /* Update the Set of Reached States */
    Ddi_BddOrAcc(travMgr->reached, to);

    if (0) {
      Ddi_Bdd_t *notReached = Ddi_BddNot(travMgr->reached);
      Ddi_Bdd_t *implications = Ddi_BddImplications(notReached,
        NULL, NULL, NULL, NULL);

      Ddi_Free(notReached);
      Ddi_Free(implications);
    }

    if (travMgr->assume != NULL) {
      Ddi_BddRestrictAcc(travMgr->reached, travMgr->assume);
    }
#if 1
    if (Ddi_MetaActive(dd)) {
      Ddi_Varset_t *novars = Ddi_VarsetVoid(dd);

      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "|F,R(meta)|=%d,%d; ",
          Ddi_BddSize(travMgr->from), Ddi_BddSize(travMgr->reached))
        );
#if 1
      Ddi_BddExistAcc(travMgr->reached, novars);
      Ddi_BddExistAcc(travMgr->from, novars);
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "|F,R(meta2)|=%d,%d; ",
          Ddi_BddSize(travMgr->from), Ddi_BddSize(travMgr->reached))
        );
      if (Ddi_BddSize(travMgr->reached) < Ddi_BddSize(travMgr->from)) {
        Ddi_Free(travMgr->from);
        travMgr->from = Ddi_BddDup(travMgr->reached);
      }
#endif
#if 0
      Ddi_BddSetMono(travMgr->reached);
      Ddi_BddSetMono(travMgr->from);
      nStates = Ddi_BddCountMinterm(travMgr->reached, Ddi_VararrayNum(psv));
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "|F,R(mono)|=%d,%d; ",
          Ddi_BddSize(travMgr->from), Ddi_BddSize(travMgr->reached))
        );
      Ddi_BddSetMeta(travMgr->reached);
      Ddi_BddSetMeta(travMgr->from);
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "|F,R(meta)|=%d,%d; ",
          Ddi_BddSize(travMgr->from), Ddi_BddSize(travMgr->reached));
        fprintf(stdout, "#ReachedStates=%g.\n", nStates)
        );
#endif
      Ddi_Free(novars);
    }
#endif

    /*Ddi_MgrSiftResume(dd); */

    /*
     *  Print Statistics for this Iteration - Part 2
     */

    Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "|To|=%d; ", Ddi_BddSize(to));
      fprintf(stdout, "|Reached|=%d.\n", Ddi_BddSize(travMgr->reached));
#ifndef __alpha__
      nStates = Ddi_BddCountMinterm(travMgr->reached, Ddi_VararrayNum(psv));
      fprintf(stdout, "#ReachedStates=%g; ", nStates);
#endif
      fprintf(stdout, "ImgTime=%s; ", util_print_time(imgTime));
      fprintf(stdout, "TotalTime=%s.\n",
        util_print_time(currTime2 - initialTime));
      fprintf(stdout,
        "{ImgMemory: %ld Kbytes} {TotalMemory: %ld Kbytes} {DDI-DD Num: %d}.\n",
        currMem2 - currMem1, currMem2, Ddi_MgrReadExtRef(dd));
      fprintf(stdout,
        "{Peak live: %u nodes} {Live: %u nodes} {Cache: %u slots}.\n",
        Ddi_MgrReadPeakLiveNodeCount(dd), Ddi_MgrReadLiveNodes(dd),
        Ddi_MgrReadCacheSlots(dd))
      );

    /*
     *  Free the to set
     */

    Ddi_Free(to);

    /*
     * Save BDDs and Variable Orders every "savePeriod" cycles
     */

    if (savePeriod != (-1) && (step % savePeriod) == 0) {
      l = strlen(periodNameSave) + 20;
      nameTmp = Pdtutil_Alloc(char,
        l
      );

      /* Save Ord */
      sprintf(nameTmp, "%sord-l%d.ord", periodNameSave, travMgr->level);
      Ddi_MgrOrdWrite(dd, nameTmp, NULL, Pdtutil_VariableOrderComment_c);

      /* Save From */
      sprintf(nameTmp, "%sfrom-l%d.bdd", periodNameSave, travMgr->level);
      Ddi_BddStore(travMgr->from, nameTmp,
        DDDMP_MODE_BINARY /*DDDMP_MODE_TEXT */ , nameTmp, NULL);

      /* Save Reached */
      sprintf(nameTmp, "%sreached-l%d.bdd", periodNameSave, travMgr->level);
      Ddi_BddStore(travMgr->reached, nameTmp,
        DDDMP_MODE_BINARY /*DDDMP_MODE_TEXT */ , nameTmp, NULL);

      Pdtutil_Free(nameTmp);
    }

    /*
     *  Check End Of Cycle: Fix Point for Closure, etc.
     */

    travContinue = (iterNumberMax < 0) || (step < iterNumberMax);

    if (toIncluded == 1) {
      travContinue = 0;
    }

    step++;
    travMgr->level++;

  }

  /*----------------------- Traversal Cycle ... End -------------------------*/

  verbosity = travMgr->settings.verbosity = verbositySave;

  Pdtutil_Free(periodNameSave);

  if (part_vars != NULL) {
    Ddi_Free(part_vars);
  }

  if (travMgr->ddiMgrAux != NULL) {
    Ddi_MgrQuit(travMgr->ddiMgrAux);
    travMgr->ddiMgrAux = NULL;
  }

  Ddi_Free(quantify);
  Tr_TrFree(trActive);

  if (travMgr->newi != NULL) {
    Ddi_MgrCheckExtRef(ddR, extRef + 1);
  } else {
    Ddi_MgrCheckExtRef(ddR, extRef);
#if 0
    Ddi_MgrPrintExtRef(ddR, nodeId);
#endif
  }


#if 0
  {
    int i, nv;
    extern int absWithNodeIncr[];
    extern int absWithNodeDecr[];
    extern int newvarSizeIncr;
    extern int newvarSizeRepl;
    extern int keepvarSizeIncr;
    extern int newvarSizeNum;
    extern int keepvarSizeNum;

    nv = Ddi_MgrReadNumVars(ddR);

#if 0
    for (i = 0; i < nv; i++) {
      if ((absWithNodeIncr[i] > 0) || (absWithNodeDecr[i] > 0)) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "%30s | I: %5d - D: %5d.\n",
            Ddi_VarName(Ddi_VarFromIndex(ddR, i)),
            absWithNodeIncr[i], absWithNodeDecr[i])
          );
      }
    }
#endif
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "New Variable Incr: %5d->%5d (%d).\n", newvarSizeIncr,
        newvarSizeRepl, newvarSizeNum);
      fprintf(stdout, "Keep Variable Incr: %5d (%d).\n", keepvarSizeIncr,
        keepvarSizeNum)
      );

  }
#endif


  if ((iterNumberMax < 0) && Ddi_MetaActive(dd)) {
    Ddi_BddFromMeta(travMgr->reached);
  }

  return (travMgr->reached);
}

/**Function********************************************************************

  Synopsis    [Generation of a mismatch input sequence]

  Description [
              ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

Ddi_Bddarray_t *
Trav_MismatchPat(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Tr_Tr_t * TR /* Transition relation */ ,
  Ddi_Bdd_t * firstC /* constrain for start set */ ,
  Ddi_Bdd_t * lastC /* constrain for last set */ ,
  Ddi_Bdd_t ** startp /* Pointer to start set */ ,
  Ddi_Bdd_t ** endp /* Pointer to end set */ ,
  Ddi_Bddarray_t * newi /* Frontier sets */ ,
  Ddi_Vararray_t * psv /* Array of present state variables */ ,
  Ddi_Vararray_t * nsv /* Array of next state variables */ ,
  Ddi_Varset_t * pivars         /* Set of pattern (input) variables */
)
{
  int i, depth, pimintermSelected, optLevel;
  Ddi_Bdd_t *from, *Bset, *Bcube, *currpat, *newr;
  Ddi_Bddarray_t *totpat;
  Ddi_Varset_t *psvars, *auxvars;
  long startTime;
  Ddi_Mgr_t *dd;                /* TR DD manager */
  Ddi_Mgr_t *ddR;               /* reached DD manager */

  Pdtutil_Assert(travMgr != NULL, "NULL traversal manager");
  Pdtutil_Assert(newi != NULL, "NULL newi array in trav manager");

  dd = travMgr->ddiMgrTr;
  ddR = travMgr->ddiMgrR;

  /* keep PIs while computing image */
  Tr_MgrSetImgSmoothPi(Tr_TrMgr(TR), 0);
  Tr_MgrSetImgMethod(Tr_TrMgr(TR), Tr_ImgMethodCofactor_c);

  if (Tr_MgrReadAuxVars(Tr_TrMgr(TR)) == NULL) {
    auxvars = Ddi_VarsetVoid(ddR);
  } else {
    auxvars = Ddi_VarsetMakeFromArray(Tr_MgrReadAuxVars(Tr_TrMgr(TR)));
  }

  depth = Ddi_BddarrayNum(newi);

  Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelDevMin_c) {
    fprintf(stdout, "Generating a mismatch sequence of length %d.\n", depth);
    fprintf(stdout, "From last to first frontier set.\n");
    if (ddR != dd) {
      fprintf(stdout, "Two dd managers used.\n");
    }
  }

  startTime = util_cpu_time();

  /*
   * Image keeps input values to tie next states and inputs
   * so only present state vars will be quantified out by image
   */

  psvars = Ddi_VarsetMakeFromArray(psv);

  if (endp != NULL) {
    /*
     *  Save end set if required
     */

    *endp = Ddi_BddDup(Ddi_MgrReadZero(ddR));
  }
  if (startp != NULL) {
    /*
     *  Save start set if required
     */

    *startp = Ddi_BddDup(Ddi_MgrReadZero(ddR));
  }


  /*----------------------- Generate START set  -----------------------*/

  if (firstC != NULL) {
    Bset = Ddi_BddCopy(ddR, firstC);
  } else {
    Bset = Ddi_BddDup(Ddi_MgrReadOne(ddR));
  }

  /*----------------------------   START   ----------------------------*/


  totpat = Ddi_BddarrayAlloc(ddR, 0);
  pimintermSelected = 0;

  for (i = depth - 1; i >= 0; i--) {

    Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelDevMin_c) {
      fprintf(stdout, "Level %d; ", i);
      fprintf(stdout, "|Bset|=%d.\n", Ddi_BddSize(Bset));
    }

    /*
     * get current frontier set
     */

    newr = Ddi_BddDup(Ddi_BddarrayRead(newi, i));

    optLevel = 0;
    if (!pimintermSelected && travMgr->settings.bdd.mismatchOptLevel > 1) {
#if 0
      if (Ddi_BddIsPartConj(Bset) && Ddi_BddIsPartConj(newr)) {
        if ((Ddi_BddPartNum(Bset) > 1) && (Ddi_BddPartNum(newr) > 1)) {
#else
      if (1) {
        if (1) {
#endif
          optLevel = 1;
        }
      }
    }

    /*
     * AND backward reached set with current frontier
     */

    Bset =
      Ddi_BddEvalFree(FindSubsetOfItersectionMono(Bset, newr, NULL, optLevel),
      Bset);

    Ddi_Free(newr);
    if (Ddi_BddIsZero(Bset)) {
      fprintf(stderr,
        "Error: Void Set Found; No Mismatch Sequence Possible.\n");
      fflush(stderr);
      Ddi_Free(Bset);
      Ddi_Free(psvars);
      Ddi_Free(auxvars);
      Ddi_Free(totpat);
      return (NULL);
    }

    Pdtutil_Assert(!Ddi_BddIsZero(Bset), "VOID set found computing mismatch");

    /*
     *  Pick up one cube randomly
     */


#if NO_PS_DONT_CARES
    {
      Ddi_Varset_t *pipsvars = Ddi_VarsetUnion(pivars, psvars);

      Ddi_VarsetDiffAcc(pipsvars, auxvars);
      Bcube = Ddi_BddPickOneMinterm(Bset, pipsvars);
      Ddi_Free(pipsvars);
    }
#else
    Bcube = Ddi_BddPickOneCube(Bset);
#endif

    Ddi_Free(Bset);


    Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelDevMin_c) {
      fprintf(stdout, "Current State.\n");
      Ddi_BddPrintCubes(Bcube, psvars, 1, 0, NULL, stdout);
    }

    /*
     *  Remove non input vars for current pattern
     *  and save current pattern in result array
     *  don't do if first iteration and firstC not specified
     */

    if ((firstC != NULL) || (i < (depth - 1))) {
      currpat = Ddi_BddExist(Bcube, psvars);
#if NO_PI_DONT_CARES
      {
        Ddi_Varset_t *myPivars = Ddi_VarsetDup(pivars);

        Ddi_VarsetDiffAcc(myPivars, auxvars);
        Ddi_BddPickOneMintermAcc(currpat, myPivars);
        Ddi_Free(myPivars);
      }
#endif
      Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelDevMin_c) {
        fprintf(stdout, "Input Pattern.\n");
        Ddi_BddPrintCubes(currpat, pivars, 1, 0, NULL, stdout);
      }
      Ddi_BddarrayInsertLast(totpat, currpat);
      Ddi_Free(currpat);
    }

    if ((startp != NULL) && (i == depth - 1)) {
      /*
       * Save start set if required
       */
      Ddi_Free(*startp);        /* free dup of ZERO */
      *startp = Ddi_BddExist(Bcube, pivars);
#if NO_IS_DONT_CARES
      *startp =
        Ddi_BddEvalFree(Ddi_BddPickOneMinterm(*startp, psvars), *startp);
      Ddi_BddAndAcc(Bcube, *startp);
      Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelDevMin_c) {
        fprintf(stdout, "Start Point.\n");
        Ddi_BddPrintCubes(*startp, psvars, 1, 0, NULL, stdout);
      }
#endif
      /* remove clock */
      if (Tr_TrClk(TR) != NULL) {
        Ddi_Varset_t *sm = Ddi_VarsetMakeFromVar(Tr_TrClk(TR));

        Ddi_BddExistAcc(*startp, sm);
        Ddi_VarsetSwapVarsAcc(sm, psv, nsv);
        Ddi_BddExistAcc(*startp, sm);
        Ddi_Free(sm);
      }
    }

    /*
     * Compute new Bset for next iteration
     */

    Bset = Ddi_BddExist(Bcube, pivars);
    Ddi_Free(Bcube);

    pimintermSelected = 0;

    if (i > 0) {

      /* and with inner ring to restrict image space */
      newr = Ddi_BddSwapVars(Ddi_BddarrayRead(newi, i - 1), psv, nsv);

      /*@@@ */
      if (travMgr->settings.bdd.mismatchOptLevel > 1) {

#if 1
        {
          Ddi_Bdd_t *to0;

          Tr_Tr_t *tr2 = Tr_TrDup(TR);
          int again = 0;
          int myClipTh = dd->exist.clipThresh;
          int myClipLevel = dd->exist.clipLevel;
          Ddi_Varset_t *nsvars = Ddi_VarsetMakeFromArray(nsv);

          dd->exist.clipDone = 0;
          dd->exist.clipLevel = 2;

          do {
            Tr_MgrSetImgSmoothPi(Tr_TrMgr(TR), 1);
            Tr_MgrSetImgMethod(Tr_TrMgr(TR), Tr_ImgMethodIwls95_c);

            Ddi_BddRestrictAcc(Tr_TrBdd(tr2), Bset);
            to0 = Tr_ImgWithConstrain(tr2, Bset, newr);

            Tr_MgrSetImgSmoothPi(Tr_TrMgr(TR), 0);
            Tr_MgrSetImgMethod(Tr_TrMgr(TR), Tr_ImgMethodCofactor_c);

            if (Ddi_BddIsZero(to0) && dd->exist.clipDone) {
              Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
                Pdtutil_VerbLevelNone_c,
                fprintf(stdout, "Clip within Mismatch Computation.\n")
                );
              dd->exist.clipDone = 0;
              dd->exist.clipThresh = (int)(dd->exist.clipThresh * 1.1);
              if (Ddi_BddIsPartConj(dd->exist.clipWindow)) {
                Ddi_Bdd_t *tmp = Ddi_BddPartExtract(dd->exist.clipWindow, 0);

                Ddi_Free(dd->exist.clipWindow);
                dd->exist.clipWindow = tmp;
              }
              Ddi_BddNotAcc(dd->exist.clipWindow);
              if (Ddi_BddIsPartConj(Bset)) {
                Ddi_BddAndAcc(Ddi_BddPartRead(Bset, 0), dd->exist.clipWindow);
              } else {
                Ddi_BddAndAcc(Bset, dd->exist.clipWindow);
              }
              Ddi_Free(dd->exist.clipWindow);
              again = 1;
              Ddi_Free(to0);
            } else {
              Ddi_BddSetMono(to0);
            }

          } while (again);

          Tr_TrFree(tr2);
          dd->exist.clipThresh = myClipTh;
          dd->exist.clipLevel = myClipLevel;

          Ddi_BddSwapVarsAcc(to0, psv, nsv);
          to0 =
            Ddi_BddEvalFree(FindSubsetOfItersectionMono(to0, newr, NULL,
              optLevel), to0);

          Ddi_Free(newr);

          newr = Ddi_BddPickOneMinterm(to0, nsvars);
          Ddi_Free(to0);
          Ddi_Free(nsvars);

        }
#else

        if (travMgr->settings.bdd.mismatchOptLevel > 2) {

          Ddi_Bdd_t *to0, *newr1;
          Ddi_Bdd_t *trBdd = Ddi_BddDup(Tr_TrBdd(TR));
          Ddi_Varset_t *nsvars = Ddi_VarsetMakeFromArray(nsv);
          Ddi_Varset_t *pipsvars = Ddi_VarsetUnion(pivars, psvars);

          Ddi_BddRestrictAcc(trBdd, Bset);
          to0 = Ddi_BddMultiwayRecursiveAndExistOver(trBdd, pipsvars,
            newr, Ddi_MgrReadOne(ddR), 1000);
          Ddi_BddExistOverApproxAcc(to0, pipsvars);

          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, "|ring|=%d -> ", Ddi_BddSize(newr))
            );

          newr1 = Ddi_BddRestrict(newr, to0);

          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, "(%d,%d).\n", Ddi_BddSize(to0), Ddi_BddSize(newr1))
            );

#if 0
          Ddi_BddSetPartConj(newr1);
          Ddi_BddPartInsert(newr1, 0, to0);
#endif
          Ddi_Free(to0);
          if (Ddi_BddSize(newr1) < Ddi_BddSize(newr)) {
            Ddi_Free(newr);
            newr = newr1;
#if 0
            to0 = Ddi_BddMultiwayRecursiveAndExistOver(trBdd, pipsvars,
              newr, Ddi_MgrReadOne(ddR), -1);

            Ddi_BddSetPartConj(newr);
            Ddi_BddPartInsert(newr, 0, to0);
#endif
            Ddi_Free(to0);
          } else {
            Ddi_Free(newr1);
          }


          Ddi_Free(pipsvars);
          Ddi_Free(trBdd);

        }

        if (Ddi_VarsetNum(pivars) > 0) {
          Ddi_Bdd_t *piset, *piset0;
          Ddi_Varset_t *sm = Ddi_VarsetMakeFromArray(nsv);
          Ddi_Varset_t *sm0, *keep0;
          Ddi_Varset_t *piv0;
          int npi, l;

          pimintermSelected = 1;

          Ddi_VarsetUnionAcc(sm, psvars);

          piset = Ddi_BddDup(Tr_TrBdd(TR));
          Ddi_BddSetPartConj(piset);

          Ddi_BddPartInsert(piset, 0, newr);

          Ddi_MgrSiftSuspend(ddR);
          Ddi_BddConstrainAcc(piset, Bset);
          Ddi_MgrSiftResume(ddR);

          npi = Ddi_VarsetNum(pivars);
          if (npi > 100) {
            sm0 = Ddi_VarsetDup(sm);
            for (l = 0, Ddi_VarsetWalkStart(pivars);
              !Ddi_VarsetWalkEnd(pivars); Ddi_VarsetWalkStep(pivars)) {
              if (l++ < npi / 2) {
                Ddi_VarsetAddAcc(sm0, Ddi_VarsetWalkCurr(pivars));
              }
            }
            piv0 = Ddi_VarsetDiff(pivars, sm0);

            {
              int verbosity = Tr_MgrReadVerbosity(trMgr);
              Ddi_Bdd_t *setOver;

              Ddi_BddSetFlattened(piset);
              Ddi_BddSetPartConj(piset);
              keep0 = Ddi_VarsetDiff(pivars, sm0);
              Tr_MgrSetOption(trMgr, Pdt_TrVerbosity_c, inum,
                Pdtutil_VerbLevelDevMin_c);
              Tr_TrBddSortIwls95(trMgr, piset, sm0, keep0);
              Tr_MgrSetOption(trMgr, Pdt_TrVerbosity_c, inum, verbosity);

              setOver = Ddi_BddMultiwayRecursiveAndExistOver(piset,
                sm0, NULL, newr, 1000);
              Ddi_BddExistOverApproxAcc(setOver, sm0);
              piset0 = Ddi_BddMultiwayRecursiveAndExist(piset,
                sm0, setOver, -1);
              Ddi_BddAndAcc(piset0, setOver);
              Ddi_Free(setOver);
              Ddi_Free(keep0);
            }
            Ddi_BddSetMono(piset0);
            Ddi_BddPickOneMintermAcc(piset0, piv0);
            Ddi_Free(piv0);
            Ddi_MgrSiftSuspend(ddR);
            Ddi_BddConstrainAcc(piset, piset0);
            Ddi_MgrSiftResume(ddR);

            {
              int verbosity = Tr_MgrReadVerbosity(trMgr);

              Ddi_BddSetFlattened(piset);
              Ddi_BddSetPartConj(piset);
              keep0 = Ddi_VarsetDiff(pivars, sm);
              Tr_MgrSetOption(trMgr, Pdt_TrVerbosity_c, inum,
                Pdtutil_VerbLevelDevMin_c);
              Tr_TrBddSortIwls95(trMgr, piset, sm, keep0);
              Tr_MgrSetOption(trMgr, Pdt_TrVerbosity_c, inum, verbosity);

              piset =
                Ddi_BddEvalFree(Ddi_BddMultiwayRecursiveAndExist(piset, sm,
                  Ddi_MgrReadOne(ddR), -1), piset);

              Ddi_Free(keep0);
            }

            Ddi_BddSetMono(piset);
            piv0 = Ddi_VarsetIntersect(pivars, sm0);
            Ddi_BddPickOneMintermAcc(piset, piv0);
            Ddi_BddAndAcc(Bset, piset0);
            Ddi_Free(piset0);
            Ddi_Free(piv0);
            Ddi_Free(sm0);
          } else {
#if 0
            piset =
              Ddi_BddEvalFree(Ddi_BddMultiwayRecursiveAndExist(piset, sm,
                Ddi_MgrReadOne(ddR), -1), piset);
            Ddi_BddSetMono(piset);
#else
            piset =
              Ddi_BddEvalFree(FindSubsetOfItersectionMono(piset,
                Ddi_MgrReadOne(ddR), sm, optLevel), piset);
#endif
            Ddi_BddPickOneMintermAcc(piset, pivars);
          }
          Ddi_Free(sm);
          Ddi_BddAndAcc(Bset, piset);
          Ddi_Free(piset);
        }
#endif

      }

      from = Bset;

      if (Ddi_BddIsMeta(newr)) {
        Bset = Tr_Img(TR, from);
      } else {
        if (pimintermSelected) {
          Ddi_MgrSiftSuspend(ddR);
          Bset = Ddi_BddConstrain(Tr_TrBdd(TR), from);
          Ddi_BddSwapVarsAcc(Bset, psv, nsv);
          Ddi_MgrSiftResume(ddR);
        } else {
          Bset = Tr_ImgWithConstrain(TR, from, newr);
        }
      }

      Ddi_Free(newr);
      /*Ddi_BddSetMono(Bset); */
      Ddi_Free(from);
    }

  }

  if (lastC != NULL) {
    /*
     * a constrain is specified for the last step
     * select proper cube and generate pattern
     */
    Bcube = Ddi_BddAnd(Bset, lastC);
    Bcube = Ddi_BddEvalFree(Ddi_BddPickOneCube(Bcube), Bcube);
    currpat = Ddi_BddExist(Bcube, psvars);
#if NO_PI_DONT_CARES
    currpat = Ddi_BddEvalFree(Ddi_BddPickOneMinterm(currpat, pivars), currpat);
#endif
    Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "Input for Last Constrain.\n");
      Ddi_BddPrintCubes(currpat, pivars, 1, 0, NULL, stdout)
      );
    Ddi_BddarrayInsertLast(totpat, currpat);
    Ddi_Free(currpat);
    Ddi_Free(Bset);
    Bset = Ddi_BddExist(Bcube, pivars);
    Ddi_Free(Bcube);
  }
  if (endp != NULL) {
    /*
     * Save start set if required
     */
    Ddi_Free(*endp);            /* free dup of ZERO */
    *endp = Ddi_BddExist(Bset, pivars);
#if NO_IS_DONT_CARES
    *endp = Ddi_BddEvalFree(Ddi_BddPickOneMinterm(*endp, psvars), *endp);
    Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelDevMin_c) {
      fprintf(stdout, "endpoint.\n");
      Ddi_BddPrintCubes(*endp, psvars, 1, 0, NULL, stdout);
    }
#endif
  }

  Ddi_Free(psvars);
  Ddi_Free(Bset);
  Ddi_Free(auxvars);

  /*-------------------- Mismatch generation ... End ------------------------*/

  return (totpat);
}

/**Function********************************************************************

  Synopsis    [Find a universal alignment sequence]

  Description [Compute a universal alignment sequence using Pixley's
               algorithm: ICCAD'91, DAC'92.
               The algorithm works knowing the set of rings or frontier
               sets.
               The goal is the innermost ring. The outermost one must be 0.
              ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

Ddi_Bddarray_t *
Trav_UnivAlignPat0(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Tr_Tr_t * TR /* Transition relation */ ,
  Ddi_Bdd_t * goal /* Destination set */ ,
  Ddi_Bdd_t ** endp /* Pointer to end set */ ,
  Ddi_Bdd_t * alignSet /* set of states to be aligned */ ,
  Ddi_Bddarray_t * rings /* Frontier or ring sets */ ,
  Ddi_Vararray_t * psv /* Array of present state variables */ ,
  Ddi_Vararray_t * nsv /* Array of next state variables */ ,
  Ddi_Varset_t * pivars /* Set of pattern (input) variables */ ,
  int maxDepth                  /* maximum depth allowed for the sequence */
)
{
  int i, minDepth, currDepth;
  Ddi_Bdd_t *ring, *from, *sum, *Bset, *Bcube,
    *currpat, *prevpat, *inner, *advance, *regress, *covered, *leftover,
    *goalBar;
  Ddi_Bdd_t *to = NULL;
  Tr_Tr_t *TrAux;
  Ddi_Bddarray_t *totpat, *ringsX;
  Ddi_Varset_t *aux, *quantify, *psvars;
  Pdtutil_VerbLevel_e verbosity;
  long startTime;
  Ddi_Mgr_t *dd;                /* TR DD manager */
  Ddi_Mgr_t *ddR;               /* reached DD manager */

  Pdtutil_Assert(travMgr != NULL, "NULL traversal manager");
  Pdtutil_Assert(rings != NULL, "NULL rings array in trav manager");

  dd = travMgr->ddiMgrTr;
  ddR = travMgr->ddiMgrR;

  minDepth = Ddi_BddarrayNum(rings);
  Pdtutil_Assert(minDepth > 0, "# of rings is <= 0!");

  verbosity = Trav_MgrReadVerbosity(travMgr);

  Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "Generating an alignment sequence (minimum length %d).\n",
      minDepth); fprintf(stdout, "From outer to inner frontier sets.\n");
    if (ddR != dd) {
    fprintf(stdout, "Two dd managers used.\n");}
  ) ;

  startTime = util_cpu_time();

  /*
   * Image keeps input values to tie next states and inputs
   * so only present state vars will be quantified out by image
   */
  quantify = Ddi_VarsetMakeFromArray(psv);
  Ddi_VarsetUnionAcc(quantify, pivars);

  /*----------------------------   START   ----------------------------*/

  sum = Ddi_BddDup(Ddi_MgrReadZero(ddR));
  for (i = 0; i < minDepth; i++)
    sum = Ddi_BddEvalFree(Ddi_BddOr(sum, Ddi_BddarrayRead(rings, i)), sum);
  if (!Ddi_BddIsOne(sum)) {
    Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "There is NO universal alignment sequence.\n")
      );
#if 1
    return (NULL);
#endif
  }

  totpat = Ddi_BddarrayAlloc(ddR, 0);
  leftover = Ddi_BddDup(sum);
  Ddi_Free(sum);

  psvars = Ddi_VarsetMakeFromArray(psv);
  TrAux = Tr_TrReverse(TR);

  /* keep PIs while computing image */
  Tr_MgrSetImgSmoothPi(Tr_TrMgr(TR), 0);

  inner = Ddi_BddDup(Ddi_BddarrayRead(rings, 0));
  advance = Ddi_BddDup(goal);

  ringsX = Ddi_BddarrayAlloc(ddR, minDepth);
  Ddi_BddarrayWrite(ringsX, 0, goal);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "Force Reordering (twice).\n")
    );
  Ddi_MgrReduceHeap(ddR, CUDD_REORDER_SAME, 0);
  Ddi_MgrReduceHeap(ddR, CUDD_REORDER_SAME, 0);
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "Done.\n")
    );

  for (i = 1; i < minDepth; i++) {
    ring = Ddi_BddarrayRead(rings, i);
    /* do preimage of inner ring, keeping input vars */

    Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "Universal Alignment: Computing Full Ring %d.\n", i)
      );

#if 0

    /* DO Partition TR */
    {
      int j;
      Tr_Tr_t *tr_i;
      Tr_Mgr_t *trMgr;
      Ddi_Bdd_t *to_i, *trBdd, *trBdd_i;
      Ddi_Varset_t *pi_i;

      /* now partitioned image done using library img func */
      to = Ddi_BddMakePartDisjVoid(ddR);
      trMgr = Tr_TrMgr(TrAux);
      trBdd = Tr_TrBdd(TrAux);

      for (j = 0; j < Ddi_BddPartNum(trBdd); j++) {
        Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMax_c,
          fprintf(stdout, "Transition Relation Partition %d.\n", j)
          );
        trBdd_i = Ddi_BddPartRead(trBdd, j);
        /* keep inactive PIs unchanged: remove from inner only active PIs */
#if 0
        pi_i = Ddi_BddSupp(trBdd_i);
        Ddi_VarsetIntersectAcc(pivars, pi_i);
        from = Ddi_BddExist(inner, pi_i);
#else
        pi_i = Ddi_VarsetDup(pivars);
        from = Ddi_BddExist(inner, pi_i);
#endif
        tr_i = Tr_TrMakeFromRel(trMgr, trBdd_i);

        to_i = Tr_Img(tr_i, from);

        Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout,
            "PartImg [%d/out of %d] - |From| %d - |To| %d.\n",
            j + 1, Ddi_BddPartNum(trBdd), Ddi_BddSize(from), Ddi_BddSize(to_i))
          );

        Ddi_Free(from);
        Ddi_Free(pi_i);

        Ddi_BddPartInsertLast(to, to_i);
        Ddi_Free(to_i);
      }
    }

#else

    /* Do NOT Partition TR */
    from = Ddi_BddExist(inner, pivars);
    to = Tr_Img(TrAux, from);

    Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "|From[%d]|: %d - |To[%d]|: %d.\n",
        i, Ddi_BddSize(from), i, Ddi_BddSize(to))
      );

    Ddi_Free(from);
#endif

    Ddi_Free(inner);
    /* set new inner */
    inner = Ddi_BddDup(to);

    /* AND with ring */
    ring = Ddi_BddAnd(to, ring);
    Ddi_BddSetMono(ring);
    Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "- |Ring[%d]|: %d.\n", i, Ddi_BddSize(ring))
      );

    /* add to advance and ringsX */
    Ddi_BddarrayWrite(ringsX, i, ring);
    Ddi_BddOrAcc(advance, ring);
    Ddi_Free(ring);
    Ddi_Free(to);
  }                             /* for END */

  Tr_TrFree(TrAux);
  regress = Ddi_BddNot(advance);

  for (currDepth = 0; (!Ddi_BddIsZero(leftover)); currDepth++) {

    if ((maxDepth > 0) && (currDepth > maxDepth)) {
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMax_c,
        fprintf(stdout, "Max requested depth reached: %d.\n", maxDepth);
        fprintf(stdout, "Process interrupted and solution non complete.\n")
        );
      break;
    }
#if 0
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "Leftover: ")
      );
    aux = Ddi_BddSupp(leftover);
    Ddi_VarsetPrint(aux, -1, NULL, stdout);
    Ddi_Free(aux);
    Ddi_BddPrintCubes(leftover, NULL, 100, 0, NULL, stdout);
#endif

#if 0
    if (i == 0) {
      /* only check goal on inner ring */
      Ddi_BddAndAcc(Bset, goal);
    } else {
      /*
       * get inner frontier set in the next state space
       */
      Ddi_Free(inner);
      inner = Ddi_BddSwapVars(Ddi_BddarrayRead(rings, i - 1), psv, nsv);
      /*
       * add input variables
       */
      TrBdd = Ddi_BddDup(Tr_TrBdd(TR));
      Ddi_BddSetPartConj(TrBdd);
      Ddi_BddPartInsert(TrBdd, 0, inner);
      Ddi_BddPartInsert(TrBdd, 0, Bset);
      aux = Ddi_VarsetMakeFromArray(nsv);
#if 0
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "Tr-aux: ")
        );
      aux = Ddi_BddSupp(TrBdd);
      Ddi_VarsetPrint(aux, -1, NULL, stdout);
      Ddi_Free(aux);
#endif
      Ddi_Free(Bset);
      Bset = Ddi_BddExist(TrBdd, aux);

      Ddi_Free(inner);
      Ddi_Free(TrBdd);
      Ddi_Free(aux);
    }
#endif

    /* check for an input making progress for all states in leftover */
    Bset = Ddi_BddAndExist(leftover, regress, psvars);
    Ddi_BddNotAcc(Bset);

    /* try to advance part of leftover */
    if (Ddi_BddIsZero(Bset)) {

      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMax_c,
        fprintf(stdout, "Partial.\n")
        );

      /*
       *  Find Inner non null intersection btw leftover and rings
       */

      Ddi_Free(Bset);
      Bset = Ddi_BddDup(Ddi_MgrReadZero(ddR));
      for (i = 0; i < minDepth; i++) {
        Ddi_Free(Bset);
        Bset = Ddi_BddAnd(leftover, Ddi_BddarrayRead(ringsX, i));
        if (!Ddi_BddIsZero(Bset))
          break;
      }

      Pdtutil_Assert(!Ddi_BddIsZero(Bset), "ZERO Bset found in alignment");

      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMax_c,
        fprintf(stdout, "iteration %d - progress for ring %d - ",
          currDepth, i);
        fprintf(stdout, "[|set|: %d].\n", Ddi_BddSize(Bset))
        );

    } else {
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "Iteration %d; Progress for All States; ", currDepth);
        fprintf(stdout, "|Set|=%d.\n", Ddi_BddSize(Bset))
        );
    }

    /*
     * pick up one cube trying to maximize improvement
     */

    Bcube = Ddi_BddPickOneCube(Bset);

    /*
     *  remove non input vars for current pattern
     *  pick one minterm
     *  and save current pattern in result array
     */

    aux = Ddi_BddSupp(Bcube);

    if (currDepth == 0) {
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMax_c,
        fprintf(stdout, "NO previous PATTERN.\n")
        );
      prevpat = Ddi_MgrReadOne(ddR);
    } else {
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "Previous PATTERN exists.\n")
        );
      prevpat = Ddi_BddarrayRead(totpat, currDepth - 1);
    }

    /* Minimize input transitions
       Keep previous pattern vars free in Bcube */
    prevpat = Ddi_BddExist(prevpat, aux);
    /* Set free vars in Bcube according to prevpat */
    Ddi_BddAndAcc(Bcube, prevpat);
    Ddi_Free(prevpat);

    Ddi_VarsetDiffAcc(aux, pivars);
    currpat = Ddi_BddExist(Bcube, aux);
    Ddi_Free(Bcube);
    Ddi_Free(aux);

    currpat = Ddi_BddEvalFree(Ddi_BddPickOneMinterm(currpat, pivars), currpat);
    Ddi_BddarrayInsertLast(totpat, currpat);

    Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "Pattern[%3d]: ", currDepth);
      Ddi_BddPrintCubes(currpat, pivars, 1, 0, NULL, stdout)
      );

    /*
     * Compute states where goal is satisfied
     * remove them from leftover
     */

    covered = Ddi_BddAndExist(goal, currpat, pivars);
    if (!Ddi_BddIsZero(covered)) {
      goalBar = Ddi_BddNot(covered);
      Ddi_BddAndAcc(leftover, goalBar);
      Ddi_Free(goalBar);
    }

    Ddi_Free(covered);
    Ddi_Free(Bset);

    /* Avoid Doing Image */
    if (Ddi_BddIsZero(leftover)) {
      Ddi_Free(currpat);
      continue;
    }

    /*
     * Compute new leftover for next iteration
     */

    from = Ddi_BddAnd(leftover, currpat);
    Ddi_Free(currpat);

    /* smooth PIs while computing forward image */
    Tr_MgrSetImgSmoothPi(Tr_TrMgr(TR), 1);
    Ddi_Free(to);
    to = Tr_Img(TR, from);
    Ddi_BddSetMono(to);

    Ddi_Free(from);
    Ddi_Free(leftover);
    leftover = Ddi_BddDup(to);
    Ddi_Free(to);
  }                             /* End for */

  Ddi_Free(inner);
  Ddi_Free(psvars);
  Ddi_Free(advance);
  Ddi_Free(leftover);
  Ddi_Free(quantify);
  Ddi_Free(regress);
  Ddi_Free(ringsX);

  /*----------------- Univ Alignment generation ... End ---------------------*/

  return (totpat);
}

/**Function********************************************************************

  Synopsis    [Find a universal alignment sequence]

  Description [Compute a universal alignment sequence using Pixley's
               algorithm: ICCAD'91, DAC'92.
               The algorithm works knowing the set of rings or frontier
               sets.
               The goal is the innermost ring. The outermost one must be 0.
              ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

Ddi_Bddarray_t *
Trav_UnivAlignPat(
  Trav_Mgr_t * travMgr /* Traversal Manager */ ,
  Tr_Tr_t * TR /* Transition relation */ ,
  Ddi_Bdd_t * goal /* Destination set */ ,
  Ddi_Bdd_t ** endp /* Pointer to end set */ ,
  Ddi_Bdd_t * alignSet /* set of states to be aliged */ ,
  Ddi_Bddarray_t * rings /* Frontier or ring sets */ ,
  Ddi_Vararray_t * psv /* Array of present state variables */ ,
  Ddi_Vararray_t * nsv /* Array of next state variables */ ,
  Ddi_Varset_t * pivars /* Set of pattern (input) variables */ ,
  int maxDepth                  /* maximum depth allowed for the sequence */
)
{
  int i, j, minDepth, currDepth;
  Ddi_Bdd_t *from, *sum, *Bset, *Bcube,
    *currpat, *prevpat, *regress, *advance, *covered, *leftover, *goalBar;
  Ddi_Bdd_t *to = NULL;
  Ddi_Bddarray_t *totpat;
  Ddi_Varset_t *aux, *quantify, *psvars, *psnsvars;
  Pdtutil_VerbLevel_e verbosity;
  long startTime;
  Ddi_Mgr_t *dd;                /* TR DD manager */
  Ddi_Mgr_t *ddR;               /* reached DD manager */
  double totStates;

  Pdtutil_Assert(travMgr != NULL, "NULL traversal manager");
  Pdtutil_Assert(rings != NULL, "NULL rings array in trav manager");

  dd = travMgr->ddiMgrTr;
  ddR = travMgr->ddiMgrR;

  minDepth = Ddi_BddarrayNum(rings);
  Pdtutil_Assert(minDepth > 0, "# of rings is <= 0!");

  totStates = Ddi_BddCountMinterm(Ddi_MgrReadOne(dd), Ddi_VararrayNum(psv));
  verbosity = Trav_MgrReadVerbosity(travMgr);

  Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "Generating an alignment sequence (minimum length %d).\n",
      minDepth); fprintf(stdout, "From outer to inner frontier sets.\n");
    if (ddR != dd) {
    fprintf(stdout, "Two dd managers used.\n");}
  ) ;

  startTime = util_cpu_time();

  /*
   * Image keeps input values to tie next states and inputs
   * so only present state vars will be quantified out by image
   */
  quantify = Ddi_VarsetMakeFromArray(psv);
  psnsvars = Ddi_VarsetMakeFromArray(psv);
  aux = Ddi_VarsetMakeFromArray(nsv);
  Ddi_VarsetUnionAcc(psnsvars, aux);
  Ddi_Free(aux);
  Ddi_VarsetUnionAcc(quantify, pivars);

  /*----------------------------   START   ----------------------------*/

  if (alignSet != NULL) {
    leftover = Ddi_BddDup(alignSet);
  } else {
    sum = Ddi_BddDup(Ddi_MgrReadZero(ddR));
    for (i = 0; i < minDepth; i++)
      sum = Ddi_BddEvalFree(Ddi_BddOr(sum, Ddi_BddarrayRead(rings, i)), sum);
    if (!Ddi_BddIsOne(sum)) {
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "There is NO universal alignment sequence.\n")
        );
      return (NULL);
    }
    leftover = Ddi_BddDup(sum);
    Ddi_Free(sum);
  }

  totpat = Ddi_BddarrayAlloc(ddR, 0);

  psvars = Ddi_VarsetMakeFromArray(psv);

  /* keep PIs while computing image */
  Tr_MgrSetImgSmoothPi(Tr_TrMgr(TR), 0);

  for (currDepth = 0; (!Ddi_BddIsZero(leftover)); currDepth++) {

    int someAdvance = 0;
    Ddi_Bdd_t *partialAdvance = NULL;

    if ((maxDepth > 0) && (currDepth > maxDepth)) {
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "Max requested depth reached: %d.\n", maxDepth);
        fprintf(stdout, "Process interrupted and solution non complete.\n")
        );
      break;
    }

    /* looking for inputs making progress for all states in leftover */
    /* regress collects inputs NOT doing progress */
    advance = Ddi_BddMakeConst(ddR, 1);
#if 1
    for (i = 0; i < minDepth && !Ddi_BddIsZero(advance); i++) {
#else
    for (i = minDepth - 1; i >= 0 && !Ddi_BddIsZero(advance); i--) {
#endif
      /* find inputs producing Img(ring_i) outside ring_{i-1} */
      Ddi_Bdd_t *prod = Ddi_BddDup(Tr_TrBdd(TR));
      Ddi_Bdd_t *partitionAdvance = Ddi_BddMakeConst(ddR, 0);
      int advanceOrRegressDone = 0;

      if (someAdvance) {
        Ddi_Free(partialAdvance);
        partialAdvance = Ddi_BddDup(advance);
      }
      Ddi_BddSetPartDisj(prod);
      for (j = 0; j < Ddi_BddPartNum(prod); j++) {
        Ddi_Bdd_t *tmp;
        Ddi_Bdd_t *prod_j = Ddi_BddPartRead(prod, j);

        Ddi_BddSetPartConj(prod_j);
        tmp = Ddi_BddAnd(Ddi_BddarrayRead(rings, i), leftover);
        if (!Ddi_BddIsZero(tmp)) {
          advanceOrRegressDone = 1;
          Ddi_BddPartInsert(prod_j, 0, tmp);
          Ddi_Free(tmp);
          if (i > 0) {
            tmp = Ddi_BddSwapVars(Ddi_BddarrayRead(rings, i - 1), psv, nsv);
          } else {
            tmp = Ddi_BddDup(goal);
          }
          Ddi_BddNotAcc(tmp);
          Ddi_BddPartInsert(prod_j, 0, tmp);
          Ddi_Free(tmp);

          /* Eval inputs producing some regress */
          regress =
            Ddi_BddMultiwayRecursiveAndExist(prod_j, psnsvars, advance, -1);
          someAdvance = 1;

          /* Eval inputs producing some advance */
          Ddi_BddNotAcc(Ddi_BddPartRead(prod_j, 0));
          Bset =
            Ddi_BddMultiwayRecursiveAndExist(prod_j, psnsvars, advance, -1);
          Ddi_BddDiffAcc(Bset, regress);
          Ddi_Free(regress);
          Ddi_BddOrAcc(partitionAdvance, Bset);
          Ddi_Free(Bset);
        } else {
          Ddi_Free(tmp);
        }
      }
      Ddi_Free(prod);
      if (advanceOrRegressDone) {
        Ddi_BddAndAcc(advance, partitionAdvance);
      }
      Ddi_Free(partitionAdvance);
    }

    Bset = Ddi_BddDup(advance);
    Ddi_Free(advance);
    if (0 && (i > 1 && partialAdvance != NULL && Ddi_BddIsZero(Bset))) {
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "--> %d (%d).\n", i, Ddi_BddSize(partialAdvance))
        );
      Ddi_Free(Bset);
      Bset = Ddi_BddDup(partialAdvance);
    }
    Ddi_Free(partialAdvance);

    /* try to advance part of leftover */
    if (Ddi_BddIsZero(Bset)) {

      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMax_c,
        fprintf(stdout, "PARTIAL!.\n")
        );

      /*
       *  Find Inner non null intersection btw leftover and rings
       */

      Ddi_Free(Bset);
      Bset = NULL;
      for (i = 0; i < minDepth; i++) {
        /* find inputs making some progress */
        Ddi_Bdd_t *prod = Ddi_BddDup(Tr_TrBdd(TR));
        Ddi_Bdd_t *newR = Ddi_BddMakeConst(ddR, 0);

        Ddi_BddSetPartDisj(prod);
        for (j = 0; j < Ddi_BddPartNum(prod); j++) {
          Ddi_Bdd_t *new_j;
          Ddi_Bdd_t *prod_j = Ddi_BddPartRead(prod, j);

          Ddi_BddSetPartConj(prod_j);
          if (i > 0) {
            Ddi_Bdd_t *tmp =
              Ddi_BddSwapVars(Ddi_BddarrayRead(rings, i - 1), psv, nsv);
            Ddi_BddPartInsert(prod_j, 0, tmp);
            Ddi_Free(tmp);
            Ddi_BddPartInsert(prod_j, 0, Ddi_BddarrayRead(rings, i));
          } else {
            Ddi_BddPartInsert(prod_j, 0, goal);
          }
          Ddi_BddPartInsert(prod_j, 0, leftover);

          new_j = Ddi_BddMultiwayRecursiveAndExist(prod_j, psnsvars, Bset, -1);
          Ddi_BddOrAcc(newR, new_j);
          Ddi_Free(new_j);
          if (!Ddi_BddIsZero(newR)) {
            break;
          }
        }
        Ddi_Free(prod);
        if (Bset != NULL) {
          Ddi_BddAndAcc(newR, Bset);
        }
        if (Ddi_BddIsZero(newR)) {
          Ddi_Free(newR);
        } else {
          Ddi_Free(Bset);
          Bset = newR;
        }
        if (Bset != NULL && !Ddi_BddIsZero(Bset)) {
          break;
        }
      }

      Pdtutil_Assert(Bset != NULL && !Ddi_BddIsZero(Bset),
        "Wrong Bset found in alignment");

      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMax_c,
        fprintf(stdout, "Iteration %d; Progress for Ring %d; ", currDepth, i);
        fprintf(stdout, "|Set|=%d.\n", Ddi_BddSize(Bset))
        );

    } else {
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMax_c,
        fprintf(stdout, "Iteration %d; Progress for All States; ", currDepth);
        fprintf(stdout, "|Set|=%d.\n", Ddi_BddSize(Bset))
        );
    }

    if (verbosity >= Pdtutil_VerbLevelUsrMax_c) {
      double nStates = Ddi_BddCountMinterm(leftover, Ddi_VararrayNum(psv));

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "States Still to Align: %g out of %g.\n", nStates,
          totStates)
        );
    }

    /*
     * pick up one cube trying to maximize improvement
     */

    Bcube = Ddi_BddPickOneCube(Bset);
    Ddi_Free(Bset);

    /*
     *  pick one minterm
     *  and save current pattern in result array
     */

    aux = Ddi_BddSupp(Bcube);

    if (currDepth == 0) {
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "NO previous PATTERN.\n")
        );
      prevpat = Ddi_MgrReadOne(ddR);
    } else {
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "Previous PATTERN exists.\n")
        );
      prevpat = Ddi_BddarrayRead(totpat, currDepth - 1);
    }

    /* Minimize input transitions
       Keep previous pattern vars free in Bcube */
    prevpat = Ddi_BddExist(prevpat, aux);
    Ddi_Free(aux);
    /* Set free vars in Bcube according to prevpat */
    Ddi_BddAndAcc(Bcube, prevpat);
    Ddi_Free(prevpat);

    currpat = Ddi_BddPickOneMinterm(Bcube, pivars);
    Ddi_Free(Bcube);
    Ddi_BddarrayInsertLast(totpat, currpat);

    Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "Pattern[%3d]: ", currDepth);
      Ddi_BddPrintCubes(currpat, pivars, 1, 0, NULL, stdout)
      );

    /*
     * Compute states where goal is satisfied
     * remove them from leftover
     */

    covered = Ddi_BddAndExist(goal, currpat, pivars);
    if (!Ddi_BddIsZero(covered)) {
      goalBar = Ddi_BddNot(covered);
      Ddi_BddAndAcc(leftover, goalBar);
      Ddi_Free(goalBar);
    }

    Ddi_Free(covered);

    /* Avoid Doing Image */
    if (Ddi_BddIsZero(leftover)) {
      Ddi_Free(currpat);
      continue;
    }

    /*
     * Compute new leftover for next iteration
     */

    from = Ddi_BddAnd(leftover, currpat);
    Ddi_Free(currpat);

    /* smooth PIs while computing forward image */
    Tr_MgrSetImgSmoothPi(Tr_TrMgr(TR), 1);
    Ddi_Free(to);
    to = Tr_Img(TR, from);
    Ddi_BddSetMono(to);

    Ddi_Free(from);
    Ddi_Free(leftover);
    leftover = Ddi_BddDup(to);
    Ddi_Free(to);
  }                             /* End for */

  Ddi_Free(psvars);
  Ddi_Free(leftover);
  Ddi_Free(quantify);
  Ddi_Free(psnsvars);

  /*----------------- Univ Alignment generation ... End ---------------------*/

  return (totpat);
}

/**Function*******************************************************************
  Synopsis    [exact forward verification]
  Description []
  SideEffects []
  SeeAlso     [FbvExactForwVerify]
******************************************************************************/
int
Trav_ExactForwVerify(
  Trav_Mgr_t * travMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  float timeLimit,
  int bddNodeLimit
)
{
  int i, verifOK, travAgain;
  Ddi_Bdd_t *from, *fromCube = NULL, *reached, *to;
  Ddi_Vararray_t *ps, *ns, *aux;
  Ddi_Varset_t *nsv, *auxv = NULL, *nsauxv = NULL;
  long initialTime, currTime, refTime, imgSiftTime, maxTime = -1;
  Tr_Tr_t *tr1, *tr2;
  Ddi_Bddarray_t *frontier;
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  char *travClk = travMgr->settings.clk;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(init);
  Ddi_Var_t *clock_var = NULL;
  int metaFixPoint = 0, optFixPoint = 0;
  int initAig = 1;
  int currHintId = 0;
  int hintsAgain = 0, useHints = (travMgr->settings.bdd.hints != NULL &&
    travMgr->settings.bdd.autoHint > 0);

  int abcOptLevel = Ddi_MgrReadAigAbcOptLevel(ddiMgr);
  Pdtutil_VerbLevel_e verbosityDd = Ddi_MgrReadVerbosity(ddiMgr);
  Pdtutil_VerbLevel_e verbosityTr = Tr_MgrReadVerbosity(Tr_TrMgr(tr));
  int clustTh = travMgr->settings.tr.clustThreshold;
  int apprOverlap = travMgr->settings.bdd.apprOverlap;

  if (timeLimit >= 0) {
    maxTime = timeLimit * 1000;
  }
  initialTime = util_cpu_time();
  ps = Tr_MgrReadPS(Tr_TrMgr(tr));
  ns = Tr_MgrReadNS(Tr_TrMgr(tr));
  aux = Tr_MgrReadAuxVars(Tr_TrMgr(tr));

  nsv = Ddi_VarsetMakeFromArray(ns);
  if (aux != NULL) {
    auxv = Ddi_VarsetMakeFromArray(aux);
    nsauxv = Ddi_VarsetUnion(nsv, auxv);
  } else {
    nsauxv = Ddi_VarsetDup(nsv);
  }

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "-- Exact FORWARD.\n")
    );

#if 0
  invarspec = Ddi_MgrReadOne(Ddi_ReadMgr(init));    /*@@@ */
#endif

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "|Invar|=%d; ", Ddi_BddSize(invar));
    fprintf(stdout, "|Tr|=%d.\n", Ddi_BddSize(Tr_TrBdd(tr)))
    );

  if (travClk != NULL) {

    clock_var = Tr_TrSetClkStateVar(tr, travClk, 1);
    if (travClk != Ddi_VarName(clock_var)) {
      Ddi_Bdd_t *lit = Ddi_BddMakeLiteral(clock_var, 0);

      travClk = travMgr->settings.clk = Pdtutil_StrDup(Ddi_VarName(clock_var));
      Ddi_BddAndAcc(init, lit);
      Ddi_Free(lit);
    }

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c, if (clock_var != NULL) {
      fprintf(stdout, "Clock Variable (%s) Detected.\n", travClk);}
    ) ;

#if 0
    {
      Ddi_Varset_t *reducedVars = Ddi_VarsetMakeFromArray(ps);
      Ddi_Bddarray_t *l = Ddi_BddarrayAlloc(ddiMgr, 0);

      Ddi_BddarrayInsert(l, 0, invarspec);
      Tr_TrReduceMasterSlave(tr, reducedVars, l, clock_var);
      Ddi_Free(reducedVars);
      DdiGenericDataCopy((Ddi_Generic_t *) invarspec,
        (Ddi_Generic_t *) Ddi_BddarrayRead(l, 0));
      Ddi_Free(l);
    }
#endif
  }
#if 0
  WindowPart(Ddi_BddPartRead(Tr_TrBdd(tr), 0, opt));
#endif

  tr1 = Tr_TrDup(tr);
  if (1
    || Ddi_BddIsPartConj(Tr_TrBdd(tr)) /*!travMgr->settings.bdd.useMeta */ ) {
    Tr_MgrSetOption(trMgr, Pdt_TrClustTh_c, inum, 1);
    Tr_TrSetClustered(tr);
    Tr_TrSortIwls95(tr);
    Tr_MgrSetOption(trMgr, Pdt_TrClustTh_c, inum, clustTh);
    Tr_TrSetClustered(tr);
    // Tr_TrSortIwls95(tr);
  }

  Tr_MgrSetClustSmoothPi(trMgr, 1);
  Tr_MgrSetOption(trMgr, Pdt_TrClustTh_c, inum, 1);

  if (!Ddi_BddIsPartDisj(Tr_TrBdd(tr))) {
    Tr_TrSetClustered(tr1);
    Tr_TrSortIwls95(tr1);
  }
#if 0
  if (travMgr->settings.bdd.useMeta) {
    for (i = 0; i < Ddi_BddPartNum(Tr_TrBdd(tr1)); i++) {
      Ddi_BddSetMeta(Ddi_BddPartRead(Tr_TrBdd(tr1), i));
    }
  }
#endif
  Tr_MgrSetOption(trMgr, Pdt_TrClustTh_c, inum, clustTh);
  Tr_TrSetClustered(tr1);

  Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "|Tr| = %d ", Ddi_BddSize(Tr_TrBdd(tr1)));
    fprintf(stdout, "#Part = %d\n", Ddi_BddPartNum(Tr_TrBdd(tr1))));

#if 1
  if (travMgr->settings.tr.trProj) {
    Tr_TrRepartition(tr1, clustTh);
    Tr_TrSetClustered(tr1);
    /*return(0); */
  }
#endif

  if (Ddi_BddIsPartConj(init)) {
    Ddi_Varset_t *sm = Ddi_BddSupp(init);
    Ddi_Varset_t *psvars = Ddi_VarsetMakeFromArray(Tr_MgrReadPS(trMgr));

    Ddi_VarsetDiffAcc(sm, psvars);
    Tr_TrBddSortIwls95(trMgr, init, sm, psvars);
    Ddi_BddSetClustered(init, clustTh);
    Ddi_BddExistProjectAcc(init, psvars);
    Ddi_Free(sm);
    Ddi_Free(psvars);
  }
  Ddi_BddSetMono(init);
  from = Ddi_BddDup(init);

  //  Ddi_BddAndAcc(from,invar);

  reached = Ddi_BddDup(from);
  if (1) {
    Ddi_Var_t *iv = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

    if (iv != NULL) {
      Ddi_Bdd_t *invarOut = Ddi_BddMakeLiteral(iv, 0);

      Ddi_BddOrAcc(reached, invarOut);
      Ddi_Free(invarOut);
    }
  }

  verifOK = 1;

  if (travMgr->settings.bdd.useMeta) {
#if 0
    Ddi_BddSetMeta(Tr_TrBdd(tr1));
    Tr_MgrSetOption(trMgr, Pdt_TrClustTh_c, inum, 3 * clustTh);
    Tr_TrSetClustered(tr1);
#endif
    Tr_MgrSetOption(trMgr, Pdt_TrClustTh_c, inum, clustTh);

    Ddi_BddSetMeta(from);
    Ddi_BddSetMeta(reached);

  }

  if (travMgr->settings.bdd.constrain) {
    Tr_MgrSetImgMethod(Tr_TrMgr(tr), Tr_ImgMethodCofactor_c);
  }

  frontier = Ddi_BddarrayAlloc(Ddi_ReadMgr(init), 1);
  Ddi_BddarrayWrite(frontier, 0, from);

  if (travMgr->settings.bdd.checkFrozen) {
    Tr_MgrSetCheckFrozenVars(Tr_TrMgr(tr));
  }

  if (travMgr->settings.bdd.wP != NULL) {
    char name[200];

    sprintf(name, "%s", travMgr->settings.bdd.wP);
    Tr_TrWritePartitions(tr, name);
  }
  if (travMgr->settings.bdd.wOrd != NULL) {
    char name[200];

    sprintf(name, "%s.0", travMgr->settings.bdd.wOrd);
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Writing intermediate variable order to file %s.\n",
        name));
    Ddi_MgrOrdWrite(ddiMgr, name, NULL, Pdtutil_VariableOrderDefault_c);
  }

  do {

    ddiMgr->exist.clipDone = travAgain = 0;

    if (0) {
      /* 24.08.2010 StQ Parte da Rivedere
         Occorre verificare che cosa calcolare a seconda del livello di verbosity */
      tr2 = Tr_TrPartition(tr1, "CLK", 0);
      if (Ddi_BddIsPartDisj(Tr_TrBdd(tr2))) {
        Ddi_Varset_t *psvars = Ddi_VarsetMakeFromArray(Tr_MgrReadPS(trMgr));
        Ddi_Varset_t *nsvars = Ddi_VarsetMakeFromArray(Tr_MgrReadNS(trMgr));
        Ddi_Varset_t *supp3;
        Ddi_Varset_t *supp = Ddi_BddSupp(Tr_TrBdd(tr2));
        Ddi_Varset_t *supp2 = Ddi_VarsetDup(supp);
        int totv = Ddi_VarsetNum(supp);
        int totp, totn;

        Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMed_c,
          Ddi_VarsetIntersectAcc(supp, psvars);
          Ddi_VarsetIntersectAcc(supp2, nsvars);
          totp = Ddi_VarsetNum(supp);
          totn = Ddi_VarsetNum(supp2);
          fprintf(stdout, "TR Variables: Tot: %d - PI: %d, PS: %d, NS: %d\n",
            totv, totv - (totp + totn), totp, totn);
          Ddi_Free(supp);
          Ddi_Free(supp2);
          supp = Ddi_BddSupp(Ddi_BddPartRead(Tr_TrBdd(tr2), 0));
          supp2 = Ddi_VarsetDup(supp);
          totv = Ddi_VarsetNum(supp);
          Ddi_VarsetIntersectAcc(supp, psvars);
          Ddi_VarsetIntersectAcc(supp2, nsvars);
          totp = Ddi_VarsetNum(supp);
          totn = Ddi_VarsetNum(supp2);
          fprintf(stdout, "TR0 Variables: Tot: %d - PI: %d, PS: %d, NS: %d\n",
            totv, totv - (totp + totn), totp, totn);
          supp3 = Ddi_VarsetDup(supp2);
          Ddi_Free(supp);
          Ddi_Free(supp2);
          supp = Ddi_BddSupp(Ddi_BddPartRead(Tr_TrBdd(tr2), 1));
          supp2 = Ddi_VarsetDup(supp);
          totv = Ddi_VarsetNum(supp);
          Ddi_VarsetIntersectAcc(supp, psvars);
          Ddi_VarsetIntersectAcc(supp2, nsvars);
          totp = Ddi_VarsetNum(supp);
          totn = Ddi_VarsetNum(supp2);
          fprintf(stdout, "TR1 Variables: Tot: %d - PI: %d, PS: %d, NS: %d\n",
            totv, totv - (totp + totn), totp, totn);
          );

        Ddi_VarsetDiffAcc(supp2, supp3);
        Ddi_VarsetPrint(supp2, 0, NULL, stdout);

        Ddi_Free(supp);
        Ddi_Free(supp2);
        Ddi_Free(supp3);

        Ddi_Free(psvars);
        Ddi_Free(nsvars);
      }

      Ddi_BddSetClustered(Tr_TrBdd(tr2), clustTh);
    } else {
      tr2 = Tr_TrDup(tr1);
    }

    if (travMgr->settings.tr.squaring > 0) {
      Ddi_Vararray_t *zv = Ddi_VararrayAlloc(ddiMgr, Ddi_VararrayNum(ps));
      char buf[1000];

      for (i = 0; i < Ddi_VararrayNum(ps); i++) {
        int xlevel = Ddi_VarCurrPos(Ddi_VararrayRead(ps, i)) + 1;
        Ddi_Var_t *var = Ddi_VarNewAtLevel(ddiMgr, xlevel);

        Ddi_VararrayWrite(zv, i, var);
        sprintf(buf, "%s$CLOSURE$Z", Ddi_VarName(Ddi_VararrayRead(ps, i)));
        Ddi_VarAttachName(var, buf);
      }

      Ddi_BddSetMono(Tr_TrBdd(tr2));
      TrBuildTransClosure(trMgr, Tr_TrBdd(tr2), ps, ns, zv);
      Ddi_Free(zv);
    }
#if 1
    {
      int j;

      for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(tr1));) {
        Ddi_Bdd_t *group, *p;

        group = Ddi_BddPartRead(Tr_TrBdd(tr1), j);
        Ddi_BddSetPartConj(group);
        p = Ddi_BddPartRead(Tr_TrBdd(tr1), i);
        Ddi_BddPartInsertLast(group, p);
        if (Ddi_BddSize(group) > travMgr->settings.bdd.apprGrTh) {
          /* resume from group insert */
          p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
          Ddi_Free(p);
          i++;
          j++;
        } else {
          p = Ddi_BddPartExtract(Tr_TrBdd(tr1), i);
          Ddi_Free(p);
        }
      }
    }
#endif

    if (travClk != 0) {
      Tr_Tr_t *newTr = Tr_TrPartitionWithClock(tr2, clock_var,
        travMgr->settings.bdd.implications);

      if (newTr != NULL) {
        Ddi_Bdd_t *p0, *p1, *f, *fpart;

        Tr_TrFree(tr2);
        tr2 = newTr;
#if 1
        if (travMgr->settings.tr.trDpartTh > 0) {
          p0 = Ddi_BddPartRead(Tr_TrBdd(tr2), 0);
          p1 = Ddi_BddPartRead(Tr_TrBdd(tr2), 1);
          f = Ddi_BddPartRead(p0, 1);
          fpart = Part_PartitionDisjSet(f, NULL, NULL,
            /*Part_MethodCofactor_c, */ Part_MethodCofactor_c,
            travMgr->settings.tr.trDpartTh, 3, Pdtutil_VerbLevelDevMax_c);
          Ddi_BddPartWrite(p0, 1, fpart);
          Ddi_Free(fpart);
          f = Ddi_BddPartRead(p1, 1);
          fpart = Part_PartitionDisjSet(f, NULL, NULL,
            /*Part_MethodCofactor_c, */ Part_MethodCofactor_c,
            2 * Ddi_BddSize(f) / 3, 3, Pdtutil_VerbLevelDevMax_c);
          Ddi_BddPartWrite(p1, 1, fpart);
          Ddi_Free(fpart);
        }
#endif
      }
    } else if (travMgr->settings.tr.trDpartVar != NULL) {
      Ddi_Bdd_t *newTrBdd, *f;
      Ddi_Var_t *v2, *v =
        Ddi_VarFromName(ddiMgr, travMgr->settings.tr.trDpartVar);
      Ddi_Bdd_t *p0, *p1;
      char name2[100];

      sprintf(name2, "%s$NS", travMgr->settings.tr.trDpartVar);
      v2 = Ddi_VarFromName(ddiMgr, name2);
      f = Tr_TrBdd(tr2);
      p0 = Ddi_BddCofactor(f, v2, 0);
      p1 = Ddi_BddCofactor(f, v2, 1);
      newTrBdd = Ddi_BddMakePartDisjVoid(ddiMgr);
      Ddi_BddPartInsertLast(newTrBdd, p0);
      Ddi_BddPartInsertLast(newTrBdd, p1);
      TrTrRelWrite(tr2, newTrBdd);
      Ddi_Free(p0);
      Ddi_Free(p1);
      Ddi_Free(newTrBdd);
      //    Tr_TrSetClustered(tr2);
    } else if (travMgr->settings.tr.trDpartTh > 0) {
      Ddi_Bdd_t *newTrBdd, *f;

      f = Tr_TrBdd(tr2);
      newTrBdd = Part_PartitionDisjSet(f, NULL, NULL, Part_MethodCofactor_c,
        travMgr->settings.tr.trDpartTh, 3, Pdtutil_VerbLevelDevMax_c);
      TrTrRelWrite(tr2, newTrBdd);
      Ddi_Free(newTrBdd);
      //    Tr_TrSetClustered(tr2);
    }

    fromCube = Ddi_BddDup(from);
    if (Ddi_BddIsMeta(from)) {
      Ddi_BddSetMono(fromCube);
    }

    if (1) {
      Ddi_Free(fromCube);
    } else if (!Ddi_BddIsCube(fromCube)) {
      //    printf("NO FROMCUBE\n");
      Ddi_Free(fromCube);
      fromCube = Ddi_BddMakeConst(ddiMgr, 1);
    } else {
      //    from = Ddi_BddMakeConst(ddiMgr,1);
      printf("FROMCUBE\n");
      travMgr->settings.bdd.fromSelect = Trav_FromSelectTo_c;
    }

    i = 0;
    verbosityDd = Ddi_MgrReadVerbosity(ddiMgr);
    verbosityTr = Tr_MgrReadVerbosity(Tr_TrMgr(tr));

    do {

      hintsAgain = 0;

      for (; !optFixPoint && !metaFixPoint &&
        (!Ddi_BddIsZero(from)) && ((travMgr->settings.bdd.bound < 0)
          || (i < travMgr->settings.bdd.bound)); i++) {

        int enableLog = i < 1000 || (i % 100 == 0);
        int prevLive = Ddi_MgrReadLiveNodes(ddiMgr);

        refTime = currTime = util_cpu_time();
        imgSiftTime = Ddi_MgrReadSiftTotTime(ddiMgr);
        if ((maxTime >= 0 && (currTime - initialTime) > maxTime) ||
          (bddNodeLimit > 0 && Ddi_MgrReadLiveNodes(ddiMgr) > bddNodeLimit)) {
          /* abort */
          verifOK = -1;
          break;
        }

        if (1 || enableLog) {
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
            fprintf(stdout,
              "\n-- Exact Forward Verification; #Iteration=%d.\n", i + 1));
        }

        if (0 && travMgr->settings.bdd.useMeta) {
          int l;

          for (l = 0; l < Ddi_BddPartNum(Tr_TrBdd(tr2)); l++) {
            DdiBddMetaAlign(Ddi_BddPartRead(Tr_TrBdd(tr2), l));
          }
        }

        if ((!Ddi_BddIsMeta(from)) && (!Ddi_BddIncluded(from, invarspec))) {
          verifOK = 0;
          break;
        } else if (Ddi_BddIsMeta(from)) {
          Ddi_Bdd_t *bad = Ddi_BddDiff(from, invarspec);

          verifOK = Ddi_BddIsZero(bad);
          if (!verifOK) {
            Ddi_BddSetMono(bad);
            verifOK = Ddi_BddIsZero(bad);
            Ddi_Free(bad);
            if (!verifOK) {
              break;
            }
          } else
            Ddi_Free(bad);
        }
        //    Ddi_MgrSetOption(ddiMgr,Pdt_DdiAigAbcOpt_c,inum,aigAbcOpt);
        if (initAig) {
          initAig = 0;
          Ddi_AigInit(ddiMgr, 100);
        }

        if (1 && travMgr->settings.bdd.imgDpartTh > 0) {
          Ddi_MgrSetExistDisjPartTh(ddiMgr, travMgr->settings.bdd.imgDpartTh);
        }


        if (useHints
          && currHintId < Ddi_BddarrayNum(travMgr->settings.bdd.hints)) {
          Ddi_Bdd_t *h_i =
            Ddi_BddarrayRead(travMgr->settings.bdd.hints, currHintId);
          if (Ddi_BddIsMeta(from)) {
            Ddi_BddSetMeta(h_i);
          } else {
            Ddi_BddSetMono(h_i);
          }
          Ddi_BddAndAcc(from, h_i);
        }

        if (fromCube != NULL) {
          Ddi_Bdd_t *constrain = Ddi_BddMakeConst(ddiMgr, 1);
          Tr_Tr_t *tr3 = Tr_TrDup(tr2);
          Ddi_Bdd_t *t = Tr_TrBdd(tr3);
          Ddi_Bdd_t *fromAig = Ddi_BddMakeAig(from);
          int nAux = 0;

          if (0 && travClk != 0) {  /* now handled by TR cluster */
            Ddi_Bdd_t *p0, *p1;

            t = Ddi_BddPartRead(Tr_TrBdd(tr3), 0);
            p0 = Ddi_BddPartExtract(t, 0);
            Ddi_BddRestrictAcc(t, from);
            t = Ddi_BddPartRead(Tr_TrBdd(tr3), 1);
            p1 = Ddi_BddPartExtract(t, 0);
            Ddi_BddRestrictAcc(t, from);
            t = Ddi_BddPartRead(Tr_TrBdd(tr3), 0);
            Ddi_BddPartInsert(t, 0, p0);
            t = Ddi_BddPartRead(Tr_TrBdd(tr3), 1);
            Ddi_BddPartInsert(t, 0, p1);
            Ddi_Free(p0);
            Ddi_Free(p1);
            Tr_TrSetClustered(tr3);
          } else if (1 && fromCube != NULL) {
            int l;
            Ddi_Bdd_t *auxCube = NULL;
            int again = 1, run = 0;

            Ddi_BddConstrainAcc(t, fromCube);
            Ddi_BddConstrainAcc(from, fromCube);
            Ddi_Free(fromCube);
            fromCube = Ddi_BddMakeConst(ddiMgr, 1);

            do {

              again = 0;
              auxCube = Ddi_BddMakeConst(ddiMgr, 1);
              run++;

              for (l = 0; l < Ddi_BddPartNum(Tr_TrBdd(tr3)); l++) {
                int j;
                Ddi_Bdd_t *p_l = Ddi_BddPartRead(t, l);
                Ddi_Bdd_t *proj_l = Ddi_BddExistProject(p_l, nsauxv);
                Ddi_Bdd_t *pAig = Ddi_BddMakeAig(p_l);
                Ddi_Varset_t *supp = Ddi_BddSupp(proj_l);
                Ddi_Vararray_t *vA = Ddi_VararrayMakeFromVarset(supp, 1);

                for (j = 0; j < Ddi_VararrayNum(vA); j++) {
                  Ddi_Var_t *ns_j = Ddi_VararrayRead(vA, j);
                  Ddi_Bdd_t *lit = Ddi_BddMakeLiteral(ns_j, 0);
                  Ddi_Bdd_t *litAig0 = Ddi_BddMakeLiteralAig(ns_j, 0);
                  Ddi_Bdd_t *litAig1 = Ddi_BddMakeLiteralAig(ns_j, 1);

                  if (Ddi_BddIncluded(proj_l, lit) ||
                    (run > 1 && !Ddi_AigSatAnd(fromAig, pAig, litAig1))) {
                    if (Ddi_VarInVarset(nsv, ns_j)) {
                      Ddi_BddAndAcc(fromCube, lit);
                    } else {
                      Ddi_BddAndAcc(auxCube, lit);
                      nAux++;
                    }
                  } else if (Ddi_BddIncluded(proj_l, Ddi_BddNotAcc(lit)) ||
                    (run > 1 && !Ddi_AigSatAnd(fromAig, pAig, litAig0))) {
                    if (Ddi_VarInVarset(nsv, ns_j)) {
                      Ddi_BddAndAcc(fromCube, lit);
                    } else {
                      Ddi_BddAndAcc(auxCube, lit);
                      nAux++;
                    }
                  } else {
                    Ddi_VarsetRemoveAcc(supp, ns_j);
                  }
                  Ddi_Free(lit);
                  Ddi_Free(litAig0);
                  Ddi_Free(litAig1);
                }
                Ddi_BddExistAcc(p_l, supp);
                Ddi_Free(supp);
                Ddi_Free(vA);
                Ddi_Free(pAig);
                Ddi_Free(proj_l);
              }
#if 0
              Tr_TrSortIwls95(tr3);
              Tr_MgrSetOption(trMgr, Pdt_TrClustTh_c, inum, clustTh);
              Tr_TrSetClustered(tr3);
#endif
              if (!Ddi_BddIsOne(auxCube)) {
                Ddi_BddConstrainAcc(t, auxCube);
                again = 1;
              }
              Ddi_Free(auxCube);
            } while (again || run <= 1);
          }
          if (travMgr->settings.bdd.useMeta && Ddi_BddSize(from) > 5
            && !Ddi_BddIsMeta(from)) {
            Ddi_BddSetMeta(from);
          }
          to = Tr_ImgWithConstrain(tr3, from, constrain);
          // logBdd(to);
          Tr_TrFree(tr3);
          Ddi_Free(constrain);
          Ddi_Free(fromAig);
          if (fromCube != NULL && Ddi_BddSize(fromCube) > 0) {
            Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
              fprintf(stdout, "IMG CUBE: %d (aux: %d).\n",
                Ddi_BddSize(fromCube), nAux));
            Ddi_BddSwapVarsAcc(fromCube, Tr_MgrReadPS(trMgr),
              Tr_MgrReadNS(trMgr));
          }

        } else {
          Ddi_Bdd_t *constrain = Ddi_BddMakeConst(ddiMgr, 1);

#if 0
          Ddi_Var_t *v = Ddi_VarFromName(ddiMgr, "i62");
          Ddi_Var_t *v1 = Ddi_VarFromName(ddiMgr, "l132");
          Ddi_Bdd_t *constrain = Ddi_BddMakeLiteral(v, 0);
          Ddi_Bdd_t *c2 = Ddi_BddMakeLiteral(v1, 1);

          if (i > 0)
            Ddi_BddAndAcc(from, c2);
#endif
          if (travMgr->settings.bdd.imgDpartSat > 0) {
            Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodPartSat_c);
            Ddi_Free(constrain);
            constrain = Ddi_BddSwapVars(reached, ps, ns);
            Ddi_BddNotAcc(constrain);
          }
          to = Tr_ImgWithConstrain(tr2, from, constrain);

          // logBdd(reached);
          Ddi_Free(constrain);
        }

        if (0) {
          Ddi_Varset_t *s = Ddi_BddSupp(to);
          Ddi_Vararray_t *vA = Ddi_VararrayMakeFromVarset(s, 1);
          int i, j, neq = 0;
          Ddi_Vararray_t *a = Ddi_VararrayAlloc(ddiMgr, 1);
          Ddi_Vararray_t *b = Ddi_VararrayAlloc(ddiMgr, 1);

          for (i = 0; i < Ddi_VararrayNum(vA); i++) {
            Ddi_Var_t *v_i = Ddi_VararrayRead(vA, i);

            Ddi_VararrayWrite(a, 0, v_i);
            Ddi_Bdd_t *to_i_0 = Ddi_BddCofactor(to, v_i, 0);
            Ddi_Bdd_t *to_i_1 = Ddi_BddCofactor(to, v_i, 1);

            for (j = i + 1; j < Ddi_VararrayNum(vA); j++) {
              Ddi_Bdd_t *to2;
              Ddi_Var_t *v_j = Ddi_VararrayRead(vA, j);

              Ddi_VararrayWrite(b, 0, v_j);
              to2 = Ddi_BddSwapVars(to, a, b);
              if (Ddi_BddEqual(to2, to)) {
                Ddi_Bdd_t *to_i_0_j_1 = Ddi_BddCofactor(to_i_0, v_j, 1);
                Ddi_Bdd_t *to_i_1_j_0 = Ddi_BddCofactor(to_i_1, v_j, 0);

                neq++;
                Ddi_Free(to2);
                printf("%d - %d SYMMETRY FOUND (%s-%s) - ", i, j,
                  Ddi_VarName(v_i), Ddi_VarName(v_j));
                if (Ddi_BddIsZero(to_i_0_j_1)) {
                  Pdtutil_Assert(Ddi_BddIsZero(to_i_1_j_0), "wrong symmetry");
                  printf("EQ");
                }
                printf("\n");
                Ddi_Free(to_i_0_j_1);
                Ddi_Free(to_i_1_j_0);
                break;
              }
              Ddi_Free(to2);
            }
            Ddi_Free(to_i_0);
            Ddi_Free(to_i_1);
          }
          printf("%d SYMMETRIES FOUND\n", neq);
          Ddi_Free(a);
          Ddi_Free(vA);
          Ddi_Free(s);
        }

        if (ddiMgr->exist.clipDone) {
          Ddi_Free(ddiMgr->exist.clipWindow);
          ddiMgr->exist.clipWindow = NULL;
          travAgain = 1;
        }

        if (!Ddi_BddIsMeta(to) && !Ddi_BddIsOne(invar)) {
          if (0 && Ddi_BddIsMeta(to)) {
            Ddi_BddSetMeta(invar);
          }
          Ddi_BddAndAcc(to, invar);
        }
        Ddi_Free(from);

        if (Ddi_BddIsMeta(to) || Ddi_BddIsMeta(reached)) {
          Ddi_Varset_t *novars = Ddi_VarsetVoid(Ddi_ReadMgr(reached));

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "[|R(meta)|: %d]", Ddi_BddSize(reached)));
          Ddi_BddExistAcc(reached, novars);
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "[|R(meta2)|: %d]", Ddi_BddSize(reached)));
          Ddi_Free(novars);
        }

        if (fromCube != NULL) {
          Ddi_BddAndAcc(to, fromCube);
        }

        optFixPoint = 0;
        if (Ddi_BddIsMeta(to) &&
          (Ddi_BddIsAig(reached) || Ddi_BddIsPartDisj(reached))) {
          Ddi_Bdd_t *chk = Ddi_BddDup(to);

          if (initAig) {
            initAig = 0;
            Ddi_AigInit(ddiMgr, 100);
          }
          if (fromCube != NULL) {
            Ddi_BddAndAcc(chk, fromCube);
          }
          if (Ddi_BddIsAig(reached)) {
            Ddi_BddSetAig(chk);
          }
          Ddi_BddDiffAcc(chk, reached);
          from = Ddi_BddDup(to);
          Ddi_BddSetAig(chk);
          metaFixPoint = !Ddi_AigSat(chk);
          Ddi_Free(chk);
        } else {
          from = Ddi_BddDiff(to, reached);
          optFixPoint = Ddi_BddIsZero(from);
        }

        if (!Ddi_BddIsZero(from) &&
          (travMgr->settings.bdd.fromSelect == Trav_FromSelectTo_c)) {
          Ddi_Free(from);
          from = Ddi_BddDup(to);
        } else if (!Ddi_BddIsZero(from) && (travMgr->settings.bdd.fromSelect ==
            Trav_FromSelectCofactor_c)) {
          Ddi_BddNotAcc(reached);
          Ddi_Free(from);
          from = Ddi_BddRestrict(to, reached);
          Ddi_BddNotAcc(reached);
        }


        if (Ddi_BddIsAig(reached)) {
          Ddi_Bdd_t *toAig = Ddi_BddDup(to);

          Ddi_BddSetAig(toAig);
          if (abcOptLevel > 2) {
            ddiAbcOptAcc(toAig, -1.0);
          }
          Ddi_BddOrAcc(reached, toAig);
          if (abcOptLevel > 2) {
            ddiAbcOptAcc(reached, -1.0);
          }
          Ddi_Free(toAig);
        } else if (Ddi_BddIsMeta(from) &&
          (Ddi_BddSize(to) > 500000 || Ddi_BddSize(reached) > 1000000)) {
          Ddi_BddSetPartDisj(reached);
          Ddi_BddPartInsertLast(reached, to);
          if (Ddi_BddSize(reached) > 50000) {
            if (initAig) {
              initAig = 0;
              Ddi_AigInit(ddiMgr, 100);
              //      Ddi_MgrSetOption(ddiMgr,Pdt_DdiAigAbcOpt_c,inum,abcOptLevel);
            }
            Ddi_BddSetAig(reached);
          }
        } else {
          if (Ddi_BddIsMeta(from)) {
            //  Ddi_MgrSiftSuspend(ddiMgr);
            Ddi_BddSetMeta(reached);
          }
          Ddi_BddOrAcc(reached, to);
          if (Ddi_BddIsMeta(from)) {
            //  Ddi_MgrSiftResume(ddiMgr);
          }
        }

        if (!Ddi_BddIsZero(from)
          && ((travMgr->settings.bdd.fromSelect == Trav_FromSelectReached_c)
            || ((travMgr->settings.bdd.fromSelect == Trav_FromSelectBest_c)
              && ((Ddi_BddSize(reached) < Ddi_BddSize(from)))))) {
          if (!(Ddi_BddIsMeta(from) && Ddi_BddIsPartDisj(reached))) {
            Ddi_Free(from);
            from = Ddi_BddDup(reached);
          }
        }

        if (travMgr->settings.bdd.noMismatch == 0) {
          if (travMgr->settings.bdd.guidedTrav) {
            Ddi_BddarrayInsertLast(frontier, reached);
          } else {
            Ddi_BddarrayWrite(frontier, i + 1, from);
          }
        }

        if (enableLog) {
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "|From|=%d; ", Ddi_BddSize(from));
            fprintf(stdout, "|To|=%d; |Reached|=%d; ",
              Ddi_BddSize(to), Ddi_BddSize(reached));
            fprintf(stdout, "#ReachedStates=%g\n",
              Ddi_BddCountMinterm(reached, Ddi_VararrayNum(ps)));
#if 0
            fprintf(stdout, "#ReachedStates(estimated)=%g\n",
              Ddi_AigEstimateMintermCount(reached, Ddi_VararrayNum(ps)))
#endif
            );
          if (travMgr->settings.bdd.auxPsSet != NULL) {
            Ddi_Bdd_t *r =
              Ddi_BddExist(reached, travMgr->settings.bdd.auxPsSet);
            Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
              printf("# NO AUX Reached states = %g (size:%d)\n",
                Ddi_BddCountMinterm(r,
                  Ddi_VararrayNum(ps) -
                  Ddi_VarsetNum(travMgr->settings.bdd.auxPsSet)),
                Ddi_BddSize(r)));
            Ddi_Free(r);
          }
        }
        Ddi_Free(to);

        if (travMgr->settings.bdd.useMeta) {
          Ddi_Varset_t *novars = Ddi_VarsetVoid(Ddi_ReadMgr(from));

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "[|F,R(meta)|: %d,%d]",
              Ddi_BddSize(from), Ddi_BddSize(reached)));

          Ddi_BddExistAcc(reached, novars);
          Ddi_BddExistAcc(from, novars);

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "[|F,R(meta2)|: %d,%d]",
              Ddi_BddSize(from), Ddi_BddSize(reached)));

          Ddi_Free(novars);
        }

        currTime = util_cpu_time();
        imgSiftTime = Ddi_MgrReadSiftTotTime(ddiMgr) - imgSiftTime;
        if (1 || enableLog) {
          int currLive = Ddi_MgrReadLiveNodes(ddiMgr);

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
            fprintf(stdout,
              " Live Nodes (Peak/Incr) = %u (%u/%u\%)\n",
              currLive,
              Ddi_MgrReadPeakLiveNodeCount(Ddi_ReadMgr(reached)),
              ((currLive - prevLive) * 100 / prevLive));
            fprintf(stdout,
              " CPU Time (img/img-no-sift) = %f (%f/%f)\n",
              ((currTime - initialTime) / 1000.0),
              (currTime - refTime) / 1000.0,
              (currTime - refTime - imgSiftTime) / 1000.0);
            );
        }
      }

      if (verifOK && useHints
        && currHintId < Ddi_BddarrayNum(travMgr->settings.bdd.hints)) {
        Ddi_Bdd_t *h_i =
          Ddi_BddarrayRead(travMgr->settings.bdd.hints, currHintId);
        if (Ddi_BddIsMeta(from)) {
          Ddi_BddSetMeta(h_i);
        } else {
          Ddi_BddSetMono(h_i);
        }
        currHintId++;
        hintsAgain = 1;
        Ddi_Free(from);
        from = Ddi_BddDup(reached);
        Ddi_BddDiffAcc(from, h_i);
        optFixPoint = metaFixPoint = 0;
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
          fprintf(stdout, "Traversal Resumes out of previously used Hint.\n"));
      }

    } while (hintsAgain);

    Ddi_Free(from);
    from = Ddi_BddDup(reached);
    ddiMgr->exist.clipThresh *= 1.5;

    Tr_TrFree(tr2);

  } while ((travAgain || ddiMgr->exist.clipDone) && verifOK);

  //  if (Ddi_BddIsMeta(reached)&&(opt->mc.method == Mc_VerifTravFwd_c)) {
  if (1) {
    Ddi_BddSetMono(reached);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|Reached(mono)|=%d; ", Ddi_BddSize(reached));
      fprintf(stdout, "#ReachedStates=%g.\n",
        Ddi_BddCountMinterm(reached, Ddi_VararrayNum(ps)));
      );
  }

  if (travMgr->settings.bdd.wR != NULL) {
    Ddi_MgrReduceHeap(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"), 0);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "Writing outer reached ring to %s.\n",
        travMgr->settings.bdd.wR));

    Ddi_BddStore(reached, "reached", DDDMP_MODE_TEXT, travMgr->settings.bdd.wR,
      NULL);
  }


  Ddi_Free(from);
  Ddi_Free(fromCube);

  if (travMgr != NULL) {
    Trav_MgrSetReached(travMgr, reached);
  }

  Ddi_Free(reached);

  Tr_TrFree(tr1);

  ddiMgr->exist.clipLevel = 0;

  if (verifOK < 0) {
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Invariant Verification: UNDECIDED.\n"));
  } else if (verifOK) {
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Invariant Verification: PASS.\n"));
  } else {
    FILE *fp;
    Trav_Mgr_t *travMgr1;
    Ddi_Bddarray_t *patternMisMat;
    Ddi_Varset_t *piv;
    Ddi_Bdd_t *invspec_bar, *end;

    Pdtutil_VerbosityMgr(travMgr1, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Invariant Verification: FAIL.\n"));

    if (travMgr1->settings.bdd.noMismatch == 0) {

      Tr_MgrSetClustSmoothPi(trMgr, 0);
      travMgr1 = Trav_MgrInit(NULL, ddiMgr);
      Trav_MgrSetOption(travMgr1, Pdt_TravMismOptLevel_c, inum,
        travMgr->settings.bdd.mismatchOptLevel);

#if 1
      piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));
#if 0
      /* keep PIs for counterexample */
      invspec_bar = Ddi_BddExist(invarspec, piv);
#else
      invspec_bar = Ddi_BddNot(invarspec);
#endif

      Tr_TrReverseAcc(tr);
      patternMisMat = Trav_MismatchPat(travMgr1, tr, invspec_bar,
        NULL, NULL, &end, frontier,
        Tr_MgrReadPS(Tr_TrMgr(tr)), Tr_MgrReadNS(Tr_TrMgr(tr)), piv);

      Tr_TrReverseAcc(tr);

      /*---------------------------- Write Patterns --------------------------*/

      if (patternMisMat != NULL) {
        char filename[100];

        sprintf(filename, "%s%d.txt", FILEPAT, getpid());
        fp = fopen(filename, "w");
        //fp = fopen ("pdtpat.txt", "w");
        if (Ddi_BddarrayPrintSupportAndCubes(patternMisMat, 1, -1,
            0, 1, NULL, fp) == 0) {
          Pdtutil_Warning(1, "Store cube Failed.");
        }
        fclose(fp);
      }

      /*----------------------- Write Initial State Set ----------------------*/

      if (end != NULL && !Ddi_BddIsZero(end)) {
        char filename[100];

        sprintf(filename, "%s%d.txt", FILEINIT, getpid());
        fp = fopen(filename, "w");
        //fp = fopen ("pdtinit.txt", "w");
        if (Ddi_BddPrintSupportAndCubes(end, 1, -1, 0, NULL, fp) == 0) {
          Pdtutil_Warning(1, "Store cube Failed.");
        }
        fclose(fp);
        Ddi_Free(end);
      }

      Ddi_Free(piv);
      Ddi_Free(invspec_bar);
      Ddi_Free(patternMisMat);

#endif
    }

    Trav_MgrQuit(travMgr1);

  }

  Ddi_Free(frontier);
  Ddi_Free(nsv);
  Ddi_Free(auxv);
  Ddi_Free(nsauxv);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c, if (verifOK >= 0) {
    currTime = util_cpu_time();
      fprintf(stdout, "Live Nodes (Peak): %u(%u) ",
        Ddi_MgrReadLiveNodes(ddiMgr), Ddi_MgrReadPeakLiveNodeCount(ddiMgr));
      fprintf(stdout, "TotalTime: %s ",
        util_print_time(currTime - travMgr->startTime));
      fprintf(stdout, "(setup: %s - ",
        util_print_time(initialTime - travMgr->startTime));
      fprintf(stdout, "trav: %s)\n", util_print_time(currTime - initialTime));}
  ) ;

  return (verifOK);
}

/**Function*******************************************************************
  Synopsis    [exact forward verification]
  Description []
  SideEffects []
  SeeAlso     [FbvExactForwVerify]
******************************************************************************/
int
Trav_TravBddExactForwVerify(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bddarray_t * delta
)
{
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(init);
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Pdtutil_VerbLevel_e verbosityDd = Ddi_MgrReadVerbosity(ddiMgr);
  Pdtutil_VerbLevel_e verbosityTr = Tr_MgrReadVerbosity(Tr_TrMgr(tr));
  int i, verifOK, travAgain;
  Ddi_Bdd_t *from, *fromCube = NULL, *reached, *to;
  Ddi_Vararray_t *ps, *ns, *pi, *aux;
  Ddi_Varset_t *nsv, *auxv = NULL, *nsauxv = NULL;
  long initialTime, currTime, refTime, imgSiftTime;
  unsigned long maxTime = ~0;
  Tr_Tr_t *tr1, *tr2;
  Ddi_Bddarray_t *frontier;
  Ddi_Var_t *clock_var = NULL;
  int metaFixPoint = 0, optFixPoint = 0;
  int initAig = 1;
  Ddi_Bdd_t *hConstr = NULL;
  Ddi_Bddarray_t *myDeltaAig = NULL;
  int satFpChk = 0;
  Trav_Settings_t *opt = &travMgr->settings;
  int clustTh = opt->tr.clustThreshold;
  char *travClk = opt->clk;
  int useHints = opt->bdd.autoHint > 0;
  int enHints = 0, hintsAgain = 0;
  int currHintId = 0, maxHintIter = -1;
  int useHintsAsConstraints = 0;
  Ddi_Bddarray_t *hints = NULL;
  int bddTimeLimit = -1;
  int keepFrontier = opt->bdd.wR != NULL;
    
  //float timeLimit = opt->bdd.bddTimeLimit;

  initialTime = util_cpu_time();
  if (bddTimeLimit > 0) {
    maxTime = initialTime + bddTimeLimit * 1000;
  }
#if 0                           // NXR out!
  if (travMgr != NULL) {
    if (bddNodeLimit <= 0) {
      bddNodeLimit = travMgr->settings.bdd.bddNodeLimit;
    }
    if (bddNodeLimit >= 0) {
      travMgr->settings.bdd.bddNodeLimit = bddNodeLimit;
    }

    if (timeLimit <= 0) {
      timeLimit = travMgr->settings.bdd.bddTimeLimit;
    }
    if (timeLimit >= 0) {
      travMgr->settings.bdd.bddTimeLimit = initialTime + timeLimit * 1000;
    }
  }

  if (timeLimit > 0) {
    maxTime = timeLimit * 1000;
  }
#endif

  pi = Tr_MgrReadI(Tr_TrMgr(tr));
  ps = Tr_MgrReadPS(Tr_TrMgr(tr));
  ns = Tr_MgrReadNS(Tr_TrMgr(tr));
  aux = Tr_MgrReadAuxVars(Tr_TrMgr(tr));

  nsv = Ddi_VarsetMakeFromArray(ns);
  if (aux != NULL) {
    auxv = Ddi_VarsetMakeFromArray(aux);
    nsauxv = Ddi_VarsetUnion(nsv, auxv);
  } else {
    nsauxv = Ddi_VarsetDup(nsv);
  }

  Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMed_c) {
    fprintf(stdout, "-- Exact FORWARD.\n");
  }

#if 0
  invarspec = Ddi_MgrReadOne(ddiMgr);   /*@@@ */
#endif

  Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMed_c) {
    fprintf(stdout, "|Invar|=%d; ", Ddi_BddSize(invar));
    fprintf(stdout, "|Tr|=%d.\n", Ddi_BddSize(Tr_TrBdd(tr)));
  }

  if (travClk != NULL) {
    clock_var = Tr_TrSetClkStateVar(tr, travClk, 1);
    if (travClk != Ddi_VarName(clock_var)) {
      Ddi_Bdd_t *lit = Ddi_BddMakeLiteral(clock_var, 0);

      Pdtutil_Free(travMgr->settings.clk);
      travClk = travMgr->settings.clk = Pdtutil_StrDup(Ddi_VarName(clock_var));
      Ddi_BddAndAcc(init, lit);
      Ddi_Free(lit);
    }

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c, if (clock_var != NULL) {
      fprintf(stdout, "Clock Variable (%s) Detected.\n", travClk);}
    ) ;

#if 0
    {
      Ddi_Varset_t *reducedVars = Ddi_VarsetMakeFromArray(ps);
      Ddi_Bddarray_t *l = Ddi_BddarrayAlloc(ddiMgr, 0);

      Ddi_BddarrayInsert(l, 0, invarspec);
      Tr_TrReduceMasterSlave(tr, reducedVars, l, clock_var);
      Ddi_Free(reducedVars);
      DdiGenericDataCopy((Ddi_Generic_t *) invarspec,
        (Ddi_Generic_t *) Ddi_BddarrayRead(l, 0));
      Ddi_Free(l);
    }
#endif
  }
#if 0
  WindowPart(Ddi_BddPartRead(Tr_TrBdd(tr), 0, opt));
#endif

  if (0 /*travMgr->settings.bdd.useMeta */ ) {
    Ddi_Bdd_t *trAux, *trBdd;
    Ddi_Bddarray_t *dc;
    int i, n, j;
    Tr_Mgr_t *trMgr = Tr_TrMgr(tr);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "Transition Relation Monolitic (%d nodes).\n",
        Ddi_BddSize(Tr_TrBdd(tr))));

    Tr_TrSortIwls95(tr);
    Tr_MgrSetClustThreshold(trMgr, clustTh);
    Tr_TrSetClustered(tr);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "Transiotion Relation Clustered (%d nodes).\n",
        Ddi_BddSize(Tr_TrBdd(tr))));

    Ddi_BddSetMeta(Tr_TrBdd(tr));
#if 1
    Tr_MgrSetClustThreshold(trMgr, clustTh * 10);
    Tr_TrSetClustered(tr);
    Tr_MgrSetClustThreshold(trMgr, clustTh);
#endif

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Transition Relation Meta (%d nodes).\n",
        Ddi_BddSize(Tr_TrBdd(tr))));

    /*exit (1); */
    trAux = Ddi_BddMakePartConjVoid(Ddi_ReadMgr(Tr_TrBdd(tr)));
    trBdd = Tr_TrBdd(tr);
    for (j = 0; j < Ddi_BddPartNum(trBdd); j++) {
      Ddi_BddMetaBfvSetConj(Ddi_BddPartRead(Tr_TrBdd(tr), j));
    }
#if 0
    Ddi_BddSetMono(Ddi_BddPartRead(trBdd, 0));
    Ddi_BddSetMono(trBdd);
    tr1 = Tr_TrDup(tr);
#else
    dc = Ddi_BddPartRead(trBdd, 0)->data.meta->dc;
    n = Ddi_BddarrayNum(dc);
    for (i = 0; i < n - 1; i++) {
      for (j = 0; j < Ddi_BddPartNum(trBdd); j++) {
        dc = Ddi_BddPartRead(trBdd, j)->data.meta->dc;
        Ddi_BddPartInsert(trAux, 0, Ddi_BddarrayRead(dc, i));
      }
    }
    for (j = 0; j < Ddi_BddPartNum(trBdd); j++) {
      dc = Ddi_BddPartRead(trBdd, j)->data.meta->one;
      Ddi_BddPartInsert(trAux, 0, Ddi_BddarrayRead(dc, i));
    }
    tr1 = Tr_TrMakeFromRel(trMgr, trAux);
    Tr_TrSetClustered(tr1);
#endif

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Transition Relation (%d nodes - #part %d).\n",
        Ddi_BddSize(trAux), Ddi_BddPartNum(trAux)));

    Ddi_Free(trAux);
    travMgr->settings.bdd.useMeta = 1;
  } else {
    tr1 = Tr_TrDup(tr);
  }


  if (1
    || Ddi_BddIsPartConj(Tr_TrBdd(tr)) /*!travMgr->settings.bdd.useMeta */ ) {
    Tr_MgrSetClustThreshold(trMgr, 1);
    Tr_TrSetClustered(tr);
    Tr_TrSortIwls95(tr);
    Tr_MgrSetClustThreshold(trMgr, clustTh);
    Ddi_BddSetFlattened(Tr_TrBdd(tr));
    Tr_TrSetClustered(tr);
    // Tr_TrSortIwls95(tr);
  }

  Tr_MgrSetClustSmoothPi(trMgr, 1);
  Tr_MgrSetClustThreshold(trMgr, 1);
  if (0 /*travMgr->settings.bdd.useMeta */ ) {
    int j;
    Ddi_Bdd_t *trBdd;

    Tr_MgrSetClustThreshold(trMgr, clustTh);
    Tr_TrSetClustered(tr1);
    trBdd = Tr_TrBdd(tr1);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|TR_0| = %d ", Ddi_BddSize(Tr_TrBdd(tr1)));
      fprintf(stdout, "#Part_0 = %d\n", Ddi_BddPartNum(Tr_TrBdd(tr1)))
      );

    for (i = Ddi_BddPartNum(trBdd) - 1; i > 0; i--) {
      for (j = i - 1; j >= 0; j--) {
        Ddi_BddRestrictAcc(Ddi_BddPartRead(trBdd, j), Ddi_BddPartRead(trBdd,
            i));
      }
    }
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|Tr_1| = %d ", Ddi_BddSize(Tr_TrBdd(tr1)));
      fprintf(stdout, "#Part_1 = %d\n", Ddi_BddPartNum(Tr_TrBdd(tr1))));
    travMgr->settings.bdd.useMeta = 1;
  } else if (!Ddi_BddIsPartDisj(Tr_TrBdd(tr))) {
    Tr_TrSetClustered(tr1);
    Tr_TrSortIwls95(tr1);
  }
#if 0
  if (travMgr->settings.bdd.useMeta) {
    for (i = 0; i < Ddi_BddPartNum(Tr_TrBdd(tr1)); i++) {
      Ddi_BddSetMeta(Ddi_BddPartRead(Tr_TrBdd(tr1), i));
    }
  }
#endif
  Tr_MgrSetClustThreshold(trMgr, clustTh);
  Ddi_BddSetFlattened(Tr_TrBdd(tr1));
  Tr_TrSetClustered(tr1);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "|Tr| = %d ", Ddi_BddSize(Tr_TrBdd(tr1)));
    fprintf(stdout, "#Part = %d\n", Ddi_BddPartNum(Tr_TrBdd(tr1))));

#if 1
  if (travMgr->settings.tr.trProj) {
    Tr_TrRepartition(tr1, clustTh);
    Tr_TrSetClustered(tr1);
    /*return(0); */
  }
#endif

  if (Ddi_BddIsPartConj(init)) {
    Ddi_Varset_t *sm = Ddi_BddSupp(init);
    Ddi_Varset_t *psvars = Ddi_VarsetMakeFromArray(Tr_MgrReadPS(trMgr));

    Ddi_VarsetDiffAcc(sm, psvars);
    Tr_TrBddSortIwls95(trMgr, init, sm, psvars);
    Ddi_BddSetClustered(init, clustTh);
    Ddi_BddExistProjectAcc(init, psvars);
    Ddi_Free(sm);
    Ddi_Free(psvars);
  }
  Ddi_BddSetMono(init);
  from = Ddi_BddDup(init);

  //  Ddi_BddAndAcc(from,invar);

  reached = Ddi_BddDup(from);
  if (1) {
    Ddi_Var_t *iv = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

    if (iv != NULL) {
      Ddi_Bdd_t *invarOut = Ddi_BddMakeLiteral(iv, 0);

      Ddi_BddOrAcc(reached, invarOut);
      Ddi_Free(invarOut);
    }
  }

  verifOK = 1;

  if (travMgr->settings.bdd.useMeta) {
#if 0
    Ddi_BddSetMeta(Tr_TrBdd(tr1));
    Tr_MgrSetClustThreshold(trMgr, 3 * clustTh);
    Tr_TrSetClustered(tr1);
#endif
    Tr_MgrSetClustThreshold(trMgr, clustTh);

    Ddi_BddSetMeta(from);
    Ddi_BddSetMeta(reached);
  }

  if (travMgr->settings.bdd.constrain) {
    Tr_MgrSetImgMethod(Tr_TrMgr(tr), Tr_ImgMethodCofactor_c);
  }

  frontier = Ddi_BddarrayAlloc(Ddi_ReadMgr(init), 1);
  Ddi_BddarrayWrite(frontier, 0, from);

  if (travMgr->settings.bdd.checkFrozen) {
    Tr_MgrSetCheckFrozenVars(Tr_TrMgr(tr));
  }

  if (opt->bdd.wP != NULL) {
    char name[200];

    sprintf(name, "%s", opt->bdd.wP);
    Tr_TrWritePartitions(tr, name);
  }
  if (opt->bdd.wOrd != NULL) {
    char name[200];

    sprintf(name, "%s.0", opt->bdd.wOrd);
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Writing intermediate variable order to file %s.\n",
        name));
    Ddi_MgrOrdWrite(ddiMgr, name, NULL, Pdtutil_VariableOrderDefault_c);
  }

  do {

    ddiMgr->exist.clipDone = travAgain = 0;

    if (0) {
      /* 24.08.2010 StQ Parte da Rivedere
         Occorre verificare che cosa calcolare a seconda del livello di verbosity */
      tr2 = Tr_TrPartition(tr1, "CLK", 0);
      if (Ddi_BddIsPartDisj(Tr_TrBdd(tr2))) {
        Ddi_Varset_t *psvars = Ddi_VarsetMakeFromArray(Tr_MgrReadPS(trMgr));
        Ddi_Varset_t *nsvars = Ddi_VarsetMakeFromArray(Tr_MgrReadNS(trMgr));
        Ddi_Varset_t *supp3;
        Ddi_Varset_t *supp = Ddi_BddSupp(Tr_TrBdd(tr2));
        Ddi_Varset_t *supp2 = Ddi_VarsetDup(supp);
        int totv = Ddi_VarsetNum(supp);
        int totp, totn;

        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
          Ddi_VarsetIntersectAcc(supp, psvars);
          Ddi_VarsetIntersectAcc(supp2, nsvars);
          totp = Ddi_VarsetNum(supp);
          totn = Ddi_VarsetNum(supp2);
          fprintf(stdout, "TR Variables: Tot: %d - PI: %d, PS: %d, NS: %d\n",
            totv, totv - (totp + totn), totp, totn);
          Ddi_Free(supp);
          Ddi_Free(supp2);
          supp = Ddi_BddSupp(Ddi_BddPartRead(Tr_TrBdd(tr2), 0));
          supp2 = Ddi_VarsetDup(supp);
          totv = Ddi_VarsetNum(supp);
          Ddi_VarsetIntersectAcc(supp, psvars);
          Ddi_VarsetIntersectAcc(supp2, nsvars);
          totp = Ddi_VarsetNum(supp);
          totn = Ddi_VarsetNum(supp2);
          fprintf(stdout, "TR0 Variables: Tot: %d - PI: %d, PS: %d, NS: %d\n",
            totv, totv - (totp + totn), totp, totn);
          supp3 = Ddi_VarsetDup(supp2);
          Ddi_Free(supp);
          Ddi_Free(supp2);
          supp = Ddi_BddSupp(Ddi_BddPartRead(Tr_TrBdd(tr2), 1));
          supp2 = Ddi_VarsetDup(supp);
          totv = Ddi_VarsetNum(supp);
          Ddi_VarsetIntersectAcc(supp, psvars);
          Ddi_VarsetIntersectAcc(supp2, nsvars);
          totp = Ddi_VarsetNum(supp);
          totn = Ddi_VarsetNum(supp2);
          fprintf(stdout, "TR1 Variables: Tot: %d - PI: %d, PS: %d, NS: %d\n",
            totv, totv - (totp + totn), totp, totn);
          );

        Ddi_VarsetDiffAcc(supp2, supp3);
        Ddi_VarsetPrint(supp2, 0, NULL, stdout);

        Ddi_Free(supp);
        Ddi_Free(supp2);
        Ddi_Free(supp3);

        Ddi_Free(psvars);
        Ddi_Free(nsvars);
      }

      Ddi_BddSetClustered(Tr_TrBdd(tr2), clustTh);
    } else {
      tr2 = Tr_TrDup(tr1);
    }

    if (travMgr->settings.tr.squaring > 0) {
      Ddi_Vararray_t *zv = Ddi_VararrayAlloc(ddiMgr, Ddi_VararrayNum(ps));
      char buf[1000];
      Ddi_Bdd_t *tmp;

      for (i = 0; i < Ddi_VararrayNum(ps); i++) {
        int xlevel = Ddi_VarCurrPos(Ddi_VararrayRead(ps, i)) + 1;
        Ddi_Var_t *var = Ddi_VarNewAtLevel(ddiMgr, xlevel);

        Ddi_VararrayWrite(zv, i, var);
        sprintf(buf, "%s$CLOSURE$Z", Ddi_VarName(Ddi_VararrayRead(ps, i)));
        Ddi_VarAttachName(var, buf);
      }
      if (travMgr->settings.tr.squaring > 1) {
	Tr_MgrSetSquaringMethod(trMgr,TR_SQUARING_POWER);
      }
      Ddi_BddSetMono(Tr_TrBdd(tr2));
      tmp = TrBuildTransClosure(trMgr, Tr_TrBdd(tr2), ps, ns, zv);
      Ddi_Free(tmp);
      Ddi_Free(zv);
    }
#if 1
    {
      int j;

      for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(tr1));) {
        Ddi_Bdd_t *group, *p;

        group = Ddi_BddPartRead(Tr_TrBdd(tr1), j);
        Ddi_BddSetPartConj(group);
        p = Ddi_BddPartRead(Tr_TrBdd(tr1), i);
        Ddi_BddPartInsertLast(group, p);
        if (Ddi_BddSize(group) > travMgr->settings.bdd.apprGrTh) {
          /* resume from group insert */
          p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
          Ddi_Free(p);
          i++;
          j++;
        } else {
          p = Ddi_BddPartExtract(Tr_TrBdd(tr1), i);
          Ddi_Free(p);
        }
      }
    }
#endif

    if (opt->tr.auxVarFile != NULL) {
      Tr_TrAuxVars(tr2, opt->tr.auxVarFile);
    }
    if (travClk != 0) {
      Tr_Tr_t *newTr = Tr_TrPartitionWithClock(tr2, clock_var,
        travMgr->settings.bdd.implications);

      if (newTr != NULL) {
        Ddi_Bdd_t *p0, *p1, *f, *fpart;

        Tr_TrFree(tr2);
        tr2 = newTr;
#if 1
        if (opt->tr.trDpartTh > 0) {
          p0 = Ddi_BddPartRead(Tr_TrBdd(tr2), 0);
          p1 = Ddi_BddPartRead(Tr_TrBdd(tr2), 1);
          f = Ddi_BddPartRead(p0, 1);
          fpart = Part_PartitionDisjSet(f, NULL, NULL,
            /*Part_MethodCofactor_c, */ Part_MethodCofactor_c,
            opt->tr.trDpartTh, 3, Pdtutil_VerbLevelDevMax_c);
          Ddi_BddPartWrite(p0, 1, fpart);
          Ddi_Free(fpart);
          f = Ddi_BddPartRead(p1, 1);
          fpart = Part_PartitionDisjSet(f, NULL, NULL,
            /*Part_MethodCofactor_c, */ Part_MethodCofactor_c,
            2 * Ddi_BddSize(f) / 3, 3, Pdtutil_VerbLevelDevMax_c);
          Ddi_BddPartWrite(p1, 1, fpart);
          Ddi_Free(fpart);
        }
#endif
      }
    } else if (opt->tr.trDpartVar != NULL) {
      Ddi_Bdd_t *newTrBdd, *f;
      Ddi_Var_t *v2, *v = Ddi_VarFromName(ddiMgr, opt->tr.trDpartVar);
      Ddi_Bdd_t *p0, *p1;
      char name2[100];

      sprintf(name2, "%s$NS", opt->tr.trDpartVar);
      v2 = Ddi_VarFromName(ddiMgr, name2);
      f = Tr_TrBdd(tr2);
      p0 = Ddi_BddCofactor(f, v2, 0);
      p1 = Ddi_BddCofactor(f, v2, 1);
      newTrBdd = Ddi_BddMakePartDisjVoid(ddiMgr);
      Ddi_BddPartInsertLast(newTrBdd, p0);
      Ddi_BddPartInsertLast(newTrBdd, p1);
      TrTrRelWrite(tr2, newTrBdd);
      Ddi_Free(p0);
      Ddi_Free(p1);
      Ddi_Free(newTrBdd);
      //    Tr_TrSetClustered(tr2);
    } else if (opt->tr.trDpartTh > 0) {
      Ddi_Bdd_t *newTrBdd, *f;

      f = Tr_TrBdd(tr2);
      newTrBdd = Part_PartitionDisjSet(f, NULL, NULL, Part_MethodCofactor_c,
        opt->tr.trDpartTh, 3, Pdtutil_VerbLevelDevMax_c);
      TrTrRelWrite(tr2, newTrBdd);
      Ddi_Free(newTrBdd);
      //    Tr_TrSetClustered(tr2);
    }

    fromCube = Ddi_BddDup(from);
    if (Ddi_BddIsMeta(from)) {
      Ddi_BddSetMono(fromCube);
    }

    if (1) {
      Ddi_Free(fromCube);
    } else if (!Ddi_BddIsCube(fromCube)) {
      //    printf("NO FROMCUBE\n");
      Ddi_Free(fromCube);
      fromCube = Ddi_BddMakeConst(ddiMgr, 1);
    } else {
      //    from = Ddi_BddMakeConst(ddiMgr,1);
      printf("FROMCUBE\n");
      opt->bdd.fromSelect = Trav_FromSelectTo_c;
    }

    i = 0;
    verbosityDd = Ddi_MgrReadVerbosity(ddiMgr);
    verbosityTr = Tr_MgrReadVerbosity(Tr_TrMgr(tr));

    do {

      hintsAgain = 0;

      for (; !optFixPoint && !metaFixPoint &&
        (!Ddi_BddIsZero(from)) && ((opt->bdd.bound < 0)
          || (i < opt->bdd.bound)); i++) {

        int enableLog = (i < 1000 || (i % 100 == 0)) &&
          (verbosityTr > Pdtutil_VerbLevelNone_c);
        int prevLive = Ddi_MgrReadLiveNodes(ddiMgr);

        refTime = currTime = util_cpu_time();
        imgSiftTime = Ddi_MgrReadSiftTotTime(ddiMgr);
        if (travMgr != NULL && TravBddOutOfLimits(travMgr)) {
          /* abort */
          verifOK = -1;
          break;
        }

        if (enableLog) {
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
            fprintf(stdout,
              "\n-- Exact Forward Verification; #Iteration=%d.\n", i + 1));
        }
	//        Fsm_MgrLogUnsatBound(fsmMgr, i - 1);

        if (opt->bdd.siftMaxTravIter > 0 && i >= opt->bdd.siftMaxTravIter) {
          if (enableLog) {
            Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
              fprintf(stdout, "\nSIFTING disabled.\n"));
          }
          Ddi_MgrSetSiftThresh(ddiMgr, 10000000);
        }

        if (0 && travMgr->settings.bdd.useMeta) {
          int l;

          for (l = 0; l < Ddi_BddPartNum(Tr_TrBdd(tr2)); l++) {
            DdiBddMetaAlign(Ddi_BddPartRead(Tr_TrBdd(tr2), l));
          }
        }

        if ((!Ddi_BddIsMeta(from)) && (!Ddi_BddIncluded(from, invarspec))) {
          verifOK = 0;
          break;
        } else if (Ddi_BddIsMeta(from)) {
          Ddi_Bdd_t *bad = Ddi_BddDiff(from, invarspec);

          verifOK = Ddi_BddIsZero(bad);
          if (!verifOK) {
            Ddi_BddSetMono(bad);
            verifOK = Ddi_BddIsZero(bad);
            Ddi_Free(bad);
            if (!verifOK) {
              break;
            }
          } else
            Ddi_Free(bad);
        }

	/* CHECK CAREFULLY see beemprdcell2f1 */
	/* done here to avoid overcosntraining */
	if (0) Ddi_BddAndAcc(from, invar); 

        Ddi_MgrSetAigAbcOptLevel(ddiMgr, opt->aig.abcOptLevel);
        if (initAig) {
          initAig = 0;
          Ddi_AigInit(ddiMgr, 100);
        }

        if (1 && opt->bdd.imgDpartTh > 0) {
          Ddi_MgrSetExistDisjPartTh(ddiMgr, opt->bdd.imgDpartTh);
        }

        if ((useHints) && !enHints) {
          if (Ddi_BddSize(from) > opt->bdd.hintsTh) {
            enHints = 1;
            if (opt->bdd.hintsStrategy > 0) {
              Ddi_Bdd_t *h = Ddi_BddMakeConst(ddiMgr, 1);
              Ddi_Bdd_t *cex = NULL;

              Pdtutil_Assert(hints == NULL, "no hints needed at this point");
              hints = Ddi_BddarrayAlloc(ddiMgr, 2);
              Ddi_BddarrayWrite(hints, 1, h);
              Ddi_Free(h);
              h = findNonVacuumHint(from, ps, pi, aux, reached, delta,
                Tr_TrBdd(tr2), hConstr, NULL, opt->bdd.autoHint,
                opt->bdd.hintsStrategy);
              Ddi_BddarrayWrite(hints, 0, h);
              Ddi_Free(h);
              currHintId = 0;
              fprintf(stdout, "\nTraversal uses Hint [0].\n");
              maxHintIter = opt->bdd.autoHint;
              hConstr = Ddi_BddMakeConstAig(ddiMgr, 1);
            }
            //      Ddi_Free(from);
            //      from = Ddi_BddDup(reached);
          }
        }

        if ((useHints) && enHints &&    /* !useHintsAsConstraints && */
          currHintId < Ddi_BddarrayNum(hints)) {
          int disHint = 0;
          Ddi_Bdd_t *h_i = Ddi_BddarrayRead(hints, currHintId);

          if (opt->bdd.nExactIter > 0 && (i < opt->bdd.nExactIter)) {
            if (currHintId == 0)
              disHint = 1;
          }
          if (!disHint) {
            if (Ddi_BddIsMeta(from)) {
              Ddi_BddSetMeta(h_i);
            } else {
              Ddi_BddSetMono(h_i);
            }
            Ddi_BddAndAcc(from, h_i);
          }
        }

        if ((opt->bdd.optImg > 0) && (Ddi_BddSize(from) > opt->bdd.optImg)) {
          to = Tr_ImgOpt(tr2, from, NULL);
        } else if (fromCube != NULL) {
          Ddi_Bdd_t *constrain = Ddi_BddMakeConst(ddiMgr, 1);
          Tr_Tr_t *tr3 = Tr_TrDup(tr2);
          Ddi_Bdd_t *t = Tr_TrBdd(tr3);
          Ddi_Bdd_t *fromAig = Ddi_BddMakeAig(from);
          int nAux = 0;

          if (useHints && enHints && useHintsAsConstraints &&
            currHintId < Ddi_BddarrayNum(hints)) {
            Ddi_Bdd_t *h_i = Ddi_BddarrayRead(hints, currHintId);

            Ddi_BddAndAcc(constrain, h_i);
          }
          if (0 && travClk != 0) {  /* now handled by TR cluster */
            Ddi_Bdd_t *p0, *p1;

            t = Ddi_BddPartRead(Tr_TrBdd(tr3), 0);
            p0 = Ddi_BddPartExtract(t, 0);
            Ddi_BddRestrictAcc(t, from);
            t = Ddi_BddPartRead(Tr_TrBdd(tr3), 1);
            p1 = Ddi_BddPartExtract(t, 0);
            Ddi_BddRestrictAcc(t, from);
            t = Ddi_BddPartRead(Tr_TrBdd(tr3), 0);
            Ddi_BddPartInsert(t, 0, p0);
            t = Ddi_BddPartRead(Tr_TrBdd(tr3), 1);
            Ddi_BddPartInsert(t, 0, p1);
            Ddi_Free(p0);
            Ddi_Free(p1);
            Tr_TrSetClustered(tr3);
          } else if (1 && fromCube != NULL) {
            int l;
            Ddi_Bdd_t *auxCube = NULL;
            int again = 1, run = 0;

            Ddi_BddConstrainAcc(t, fromCube);
            Ddi_BddConstrainAcc(from, fromCube);
            Ddi_Free(fromCube);
            fromCube = Ddi_BddMakeConst(ddiMgr, 1);

            do {

              again = 0;
              auxCube = Ddi_BddMakeConst(ddiMgr, 1);
              run++;

              for (l = 0; l < Ddi_BddPartNum(Tr_TrBdd(tr3)); l++) {
                int j;
                Ddi_Bdd_t *p_l = Ddi_BddPartRead(t, l);
                Ddi_Bdd_t *proj_l = Ddi_BddExistProject(p_l, nsauxv);
                Ddi_Bdd_t *pAig = Ddi_BddMakeAig(p_l);
                Ddi_Varset_t *supp = Ddi_BddSupp(proj_l);
                Ddi_Vararray_t *vA = Ddi_VararrayMakeFromVarset(supp, 1);

                for (j = 0; j < Ddi_VararrayNum(vA); j++) {
                  Ddi_Var_t *ns_j = Ddi_VararrayRead(vA, j);
                  Ddi_Bdd_t *lit = Ddi_BddMakeLiteral(ns_j, 0);
                  Ddi_Bdd_t *litAig0 = Ddi_BddMakeLiteralAig(ns_j, 0);
                  Ddi_Bdd_t *litAig1 = Ddi_BddMakeLiteralAig(ns_j, 1);

                  if (Ddi_BddIncluded(proj_l, lit) ||
                    (run > 1 && !Ddi_AigSatAnd(fromAig, pAig, litAig1))) {
                    if (Ddi_VarInVarset(nsv, ns_j)) {
                      Ddi_BddAndAcc(fromCube, lit);
                    } else {
                      Ddi_BddAndAcc(auxCube, lit);
                      nAux++;
                    }
                  } else if (Ddi_BddIncluded(proj_l, Ddi_BddNotAcc(lit)) ||
                    (run > 1 && !Ddi_AigSatAnd(fromAig, pAig, litAig0))) {
                    if (Ddi_VarInVarset(nsv, ns_j)) {
                      Ddi_BddAndAcc(fromCube, lit);
                    } else {
                      Ddi_BddAndAcc(auxCube, lit);
                      nAux++;
                    }
                  } else {
                    Ddi_VarsetRemoveAcc(supp, ns_j);
                  }
                  Ddi_Free(lit);
                  Ddi_Free(litAig0);
                  Ddi_Free(litAig1);
                }
                Ddi_BddExistAcc(p_l, supp);
                Ddi_Free(supp);
                Ddi_Free(vA);
                Ddi_Free(pAig);
                Ddi_Free(proj_l);
              }
#if 0
              Tr_TrSortIwls95(tr3);
              Tr_MgrSetClustThreshold(trMgr, clustTh);
              Tr_TrSetClustered(tr3);
#endif
              if (!Ddi_BddIsOne(auxCube)) {
                Ddi_BddConstrainAcc(t, auxCube);
                again = 1;
              }
              Ddi_Free(auxCube);
            } while (again || run <= 1);
          }
          if (travMgr->settings.bdd.useMeta && Ddi_BddSize(from) > 5
            && !Ddi_BddIsMeta(from)) {
            Ddi_BddSetMeta(from);
          }
          to = Tr_ImgWithConstrain(tr3, from, constrain);
          // logBdd(to);
          Tr_TrFree(tr3);
          Ddi_Free(constrain);
          Ddi_Free(fromAig);
          if (fromCube != NULL && Ddi_BddSize(fromCube) > 0) {
            Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
              fprintf(stdout, "IMG CUBE: %d (aux: %d).\n",
                Ddi_BddSize(fromCube), nAux));
            Ddi_BddSwapVarsAcc(fromCube, Tr_MgrReadPS(trMgr),
              Tr_MgrReadNS(trMgr));
          }

        } else {
          Ddi_Bdd_t *constrain = Ddi_BddMakeConst(ddiMgr, 1);

#if 0
          Ddi_Var_t *v = Ddi_VarFromName(ddiMgr, "i62");
          Ddi_Var_t *v1 = Ddi_VarFromName(ddiMgr, "l132");
          Ddi_Bdd_t *constrain = Ddi_BddMakeLiteral(v, 0);
          Ddi_Bdd_t *c2 = Ddi_BddMakeLiteral(v1, 1);

          if (i > 0)
            Ddi_BddAndAcc(from, c2);
#endif
          if (0 || travMgr->settings.bdd.imgDpartSat > 0) {
            // Tr_MgrSetImgMethod(trMgr,Tr_ImgMethodPartSat_c);
            Ddi_Free(constrain);
            constrain = Ddi_BddSwapVars(reached, ps, ns);
            Ddi_BddNotAcc(constrain);
          }
          if (!enableLog) {
            Ddi_MgrSetVerbosity(ddiMgr, (Pdtutil_VerbLevel_e) ((int)0));
            Tr_MgrSetVerbosity(trMgr, (Pdtutil_VerbLevel_e) ((int)0));
          }
          if ((useHints) && enHints && useHintsAsConstraints &&
            currHintId < Ddi_BddarrayNum(hints)) {
            Ddi_Bdd_t *h_i = Ddi_BddarrayRead(hints, currHintId);

            Ddi_BddAndAcc(constrain, h_i);
          }
          to = Tr_ImgWithConstrain(tr2, from, constrain);
          if (enableLog) {
            Ddi_MgrSetVerbosity(ddiMgr,
              (Pdtutil_VerbLevel_e) ((int)verbosityDd));
            Tr_MgrSetVerbosity(trMgr,
              (Pdtutil_VerbLevel_e) ((int)verbosityTr));
          }
	  //          DdiLogBdd(to,0);
	  if (0) {
	    Ddi_Var_t *cvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
	    Ddi_Bdd_t *toc = Ddi_BddCofactor(to,cvarPs,1);
	    DdiLogBdd(toc,0);
	    Ddi_Free(toc);
	  }
          Ddi_Free(constrain);
        }

        if (0) {
          Ddi_Varset_t *s = Ddi_BddSupp(to);
          Ddi_Vararray_t *vA = Ddi_VararrayMakeFromVarset(s, 1);
          int i, j, neq = 0;
          Ddi_Vararray_t *a = Ddi_VararrayAlloc(ddiMgr, 1);
          Ddi_Vararray_t *b = Ddi_VararrayAlloc(ddiMgr, 1);

          for (i = 0; i < Ddi_VararrayNum(vA); i++) {
            Ddi_Var_t *v_i = Ddi_VararrayRead(vA, i);

            Ddi_VararrayWrite(a, 0, v_i);
            Ddi_Bdd_t *to_i_0 = Ddi_BddCofactor(to, v_i, 0);
            Ddi_Bdd_t *to_i_1 = Ddi_BddCofactor(to, v_i, 1);

            for (j = i + 1; j < Ddi_VararrayNum(vA); j++) {
              Ddi_Bdd_t *to2;
              Ddi_Var_t *v_j = Ddi_VararrayRead(vA, j);

              Ddi_VararrayWrite(b, 0, v_j);
              to2 = Ddi_BddSwapVars(to, a, b);
              if (Ddi_BddEqual(to2, to)) {
                Ddi_Bdd_t *to_i_0_j_1 = Ddi_BddCofactor(to_i_0, v_j, 1);
                Ddi_Bdd_t *to_i_1_j_0 = Ddi_BddCofactor(to_i_1, v_j, 0);

                neq++;
                Ddi_Free(to2);
                printf("%d - %d SYMMETRY FOUND (%s-%s) - ", i, j,
                  Ddi_VarName(v_i), Ddi_VarName(v_j));
                if (Ddi_BddIsZero(to_i_0_j_1)) {
                  Pdtutil_Assert(Ddi_BddIsZero(to_i_1_j_0), "wrong symmetry");
                  printf("EQ");
                }
                printf("\n");
                Ddi_Free(to_i_0_j_1);
                Ddi_Free(to_i_1_j_0);
                break;
              }
              Ddi_Free(to2);
            }
            Ddi_Free(to_i_0);
            Ddi_Free(to_i_1);
          }
          printf("%d SYMMETRIES FOUND\n", neq);
          Ddi_Free(a);
          Ddi_Free(vA);
          Ddi_Free(s);
        }

        if (ddiMgr->exist.clipDone) {
          Ddi_Free(ddiMgr->exist.clipWindow);
          ddiMgr->exist.clipWindow = NULL;
          travAgain = 1;
        }

        if ((!Ddi_BddIsMeta(to)) && (!Ddi_BddIncluded(to, invarspec))) {
          verifOK = 0;
        } else if (Ddi_BddIsMeta(from)) {
          Ddi_Bdd_t *bad = Ddi_BddDiff(to, invarspec);

          verifOK = Ddi_BddIsZero(bad);
          if (!verifOK) {
            Ddi_BddSetMono(bad);
            verifOK = Ddi_BddIsZero(bad);
            Ddi_Free(bad);
          } else
            Ddi_Free(bad);
        }
	if (!verifOK) {
	  if (opt->doCex >= 1) {
	    if (travMgr->settings.bdd.guidedTrav) {
	      Ddi_BddOrAcc(reached, to);
	      Ddi_BddarrayInsertLast(frontier, reached);
	    } else {
	      Ddi_BddarrayWrite(frontier, i + 1, to);
	    }
	  }
          Ddi_Free(to);
	  i++;
	  break;
	}
	
        if (!Ddi_BddIsMeta(to) && !Ddi_BddIsOne(invar)) {
          if (0 && Ddi_BddIsMeta(to)) {
            Ddi_BddSetMeta(invar);
          }
	  /* CHECK CAREFULLY see beemprdcell2f1 */
          if (1) Ddi_BddAndAcc(to, invar); 
          else {
            Ddi_Var_t *iv = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$NS");
            if (iv != NULL) {
              Ddi_Bdd_t *invarIn = Ddi_BddMakeLiteral(iv, 1);
              Ddi_BddAndAcc(to, invarIn);
              Ddi_Free(invarIn);
            }
          }
        }
        Ddi_Free(from);

        if (Ddi_BddIsMeta(to) || Ddi_BddIsMeta(reached)) {
          Ddi_Varset_t *novars = Ddi_VarsetVoid(Ddi_ReadMgr(reached));

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "[|R(meta)|: %d]", Ddi_BddSize(reached)));
          Ddi_BddExistAcc(reached, novars);
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "[|R(meta2)|: %d]", Ddi_BddSize(reached)));
          Ddi_Free(novars);
        }

        if (fromCube != NULL) {
          Ddi_BddAndAcc(to, fromCube);
        }

        optFixPoint = 0;
        if (Ddi_BddIsMeta(to) &&
          (Ddi_BddIsAig(reached) || Ddi_BddIsPartDisj(reached))) {
          Ddi_Bdd_t *chk = Ddi_BddDup(to);

          if (initAig) {
            initAig = 0;
            Ddi_AigInit(ddiMgr, 100);
          }
          if (fromCube != NULL) {
            Ddi_BddAndAcc(chk, fromCube);
          }
          if (Ddi_BddIsAig(reached)) {
            Ddi_BddSetAig(chk);
          }
          Ddi_BddDiffAcc(chk, reached);
          from = Ddi_BddDup(to);
          Ddi_BddSetAig(chk);
          metaFixPoint = !Ddi_AigSat(chk);
          Ddi_Free(chk);
        } else if (satFpChk) {
          Ddi_Bdd_t *chk = Ddi_BddDup(to);
          Ddi_Bdd_t *rAig = Ddi_BddDup(reached);

          if (initAig) {
            initAig = 0;
            Ddi_AigInit(ddiMgr, 100);
          }
          if (fromCube != NULL) {
            Ddi_BddAndAcc(chk, fromCube);
          }
          Ddi_BddSetAig(chk);
          Ddi_BddSetAig(rAig);

          Ddi_BddDiffAcc(chk, rAig);
          from = Ddi_BddDup(to);
          optFixPoint = !Ddi_AigSat(chk);
          Ddi_Free(chk);
          Ddi_Free(rAig);
        } else {
          from = Ddi_BddDiff(to, reached);
          optFixPoint = Ddi_BddIsZero(from);
        }

        if (!Ddi_BddIsZero(from)
          && (opt->bdd.fromSelect == Trav_FromSelectTo_c)) {
          Ddi_Free(from);
          from = Ddi_BddDup(to);
        } else if (!Ddi_BddIsZero(from)
          && (opt->bdd.fromSelect == Trav_FromSelectCofactor_c)) {
          Ddi_BddNotAcc(reached);
          Ddi_Free(from);
          from = Ddi_BddRestrict(to, reached);
          Ddi_BddNotAcc(reached);
        }

        if (Ddi_BddIsAig(reached)) {
          Ddi_Bdd_t *toAig = Ddi_BddDup(to);

          Ddi_BddSetAig(toAig);
          if (opt->aig.abcOptLevel > 2) {
            ddiAbcOptAcc(toAig, -1.0);
          }
          Ddi_BddOrAcc(reached, toAig);
          if (opt->aig.abcOptLevel > 2) {
            ddiAbcOptAcc(reached, -1.0);
          }
          Ddi_Free(toAig);
        } else if (Ddi_BddIsMeta(from) &&
          (Ddi_BddSize(to) > 500000 || Ddi_BddSize(reached) > 1000000)) {
          Ddi_BddSetPartDisj(reached);
          Ddi_BddPartInsertLast(reached, to);
          if (Ddi_BddSize(reached) > 50000) {
            if (initAig) {
              initAig = 0;
              Ddi_AigInit(ddiMgr, 100);
              Ddi_MgrSetAigAbcOptLevel(ddiMgr, opt->aig.abcOptLevel);
            }
            Ddi_BddSetAig(reached);
          }
        } else {
          if (Ddi_BddIsMeta(from)) {
            //      Ddi_MgrSiftSuspend(ddiMgr);
            Ddi_BddSetMeta(reached);
          }
          Ddi_BddOrAcc(reached, to);
          if (Ddi_BddIsMeta(from)) {
            //      Ddi_MgrSiftResume(ddiMgr);
          }
        }

        if (!Ddi_BddIsZero(from)
          && ((opt->bdd.fromSelect == Trav_FromSelectReached_c)
            || ((opt->bdd.fromSelect == Trav_FromSelectBest_c)
              && ((Ddi_BddSize(reached) < Ddi_BddSize(from)))))) {
          if (!(Ddi_BddIsMeta(from) && Ddi_BddIsPartDisj(reached))) {
            Ddi_Free(from);
            from = Ddi_BddDup(reached);
          }
        }

        if (opt->doCex >= 1 || keepFrontier) {
          if (travMgr->settings.bdd.guidedTrav || keepFrontier) {
            Ddi_BddarrayInsertLast(frontier, reached);
          } else {
            Ddi_BddarrayWrite(frontier, i + 1, from);
          }
        }

        if (enableLog) {
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "|From|=%d; ", Ddi_BddSize(from));
            fprintf(stdout, "|To|=%d; |Reached|=%d; ",
              Ddi_BddSize(to), Ddi_BddSize(reached));
            fprintf(stdout, "#ReachedStates=%g\n",
              Ddi_BddCountMinterm(reached, Ddi_VararrayNum(ps)));
#if 0
            fprintf(stdout, "#ReachedStates(estimated)=%g\n",
              Ddi_AigEstimateMintermCount(reached, Ddi_VararrayNum(ps)))
#endif
            );
          if (opt->bdd.auxPsSet != NULL) {
            Ddi_Bdd_t *r = Ddi_BddExist(reached, opt->bdd.auxPsSet);

            Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
              printf("# NO AUX Reached states = %g (size:%d)\n",
                Ddi_BddCountMinterm(r,
                  Ddi_VararrayNum(ps) - Ddi_VarsetNum(opt->bdd.auxPsSet)),
                Ddi_BddSize(r)));
            Ddi_Free(r);
          }
        }
        Ddi_Free(to);

        if (travMgr->settings.bdd.useMeta) {
          Ddi_Varset_t *novars = Ddi_VarsetVoid(Ddi_ReadMgr(from));

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "[|F,R(meta)|: %d,%d]",
              Ddi_BddSize(from), Ddi_BddSize(reached)));

          Ddi_BddExistAcc(reached, novars);
          Ddi_BddExistAcc(from, novars);

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "[|F,R(meta2)|: %d,%d]",
              Ddi_BddSize(from), Ddi_BddSize(reached)));

          Ddi_Free(novars);
        }

        currTime = util_cpu_time();
        imgSiftTime = Ddi_MgrReadSiftTotTime(ddiMgr) - imgSiftTime;
        if (0 || enableLog) {
          int currLive = Ddi_MgrReadLiveNodes(ddiMgr);

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
            fprintf(stdout,
              " Live Nodes (Peak/Incr) = %u (%u/%u\%)\n",
              currLive,
              Ddi_MgrReadPeakLiveNodeCount(Ddi_ReadMgr(reached)),
              ((currLive - prevLive) * 100 / prevLive));
            fprintf(stdout,
              " CPU Time (img/img-no-sift) = %f (%f/%f)\n",
              ((currTime - initialTime) / 1000.0),
              (currTime - refTime) / 1000.0,
              (currTime - refTime - imgSiftTime) / 1000.0);
            );
        }
      }

      if (verifOK && (useHints) && enHints &&
        currHintId < Ddi_BddarrayNum(hints) - 1) {

        int useMeta = 0;
        Ddi_Bdd_t *h_i, *prev_h;
        int vacuum = 0;

        if (Ddi_BddIsMeta(from)) {
          useMeta = 1;
        }
        hintsAgain = 1;
        Ddi_Free(from);
        from = Ddi_BddDup(reached);
        prev_h = Ddi_BddDup(Ddi_BddarrayRead(hints, currHintId));
        Ddi_BddNotAcc(prev_h);
        Ddi_BddConstrainAcc(from, prev_h);
        Ddi_BddNotAcc(prev_h);
        Ddi_BddDiffAcc(from, prev_h);
        currHintId++;
        i--;

        do {
          if (useHints && opt->bdd.hintsStrategy > 0 && maxHintIter-- > 0) {
            int myHintsStrategy = opt->bdd.hintsStrategy;

            if (0 && opt->bdd.hintsStrategy > 1
              && opt->bdd.autoHint - maxHintIter < 3) {
              //myHintsStrategy = 1;
            }
            h_i = findNonVacuumHint(from, ps, pi, aux, reached, delta,
              Tr_TrBdd(tr2), hConstr, prev_h, maxHintIter / 2,
              myHintsStrategy);
            Ddi_BddarrayWrite(hints, 0, h_i);
            Ddi_Free(h_i);
            h_i = Ddi_BddarrayRead(hints, 0);
            currHintId = 0;
          } else {
            Ddi_Bdd_t *trAndNotReached = Ddi_BddNot(reached);
            Ddi_Bdd_t *myFrom = Ddi_BddDup(from);
            Ddi_Var_t *iv = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

            if (myDeltaAig == NULL) {
              int j;

              myDeltaAig = Ddi_BddarrayDup(delta);
            }
            Ddi_BddSetAig(trAndNotReached);
            if (iv != NULL) {
              Ddi_Bdd_t *invar = Ddi_BddMakeLiteralAig(iv, 1);

              Ddi_AigAndCubeAcc(trAndNotReached, invar);
              Ddi_BddSetMono(invar);
              Ddi_BddConstrainAcc(myFrom, invar);
              Ddi_BddAndAcc(myFrom, invar);
              Ddi_Free(invar);
            }

            Ddi_BddComposeAcc(trAndNotReached, ps, myDeltaAig);
            if (iv != NULL) {
              Ddi_Bdd_t *invar = Ddi_BddMakeLiteralAig(iv, 1);

              Ddi_BddAndAcc(trAndNotReached, invar);
              Ddi_Free(invar);
            }

            h_i = Ddi_BddarrayRead(hints, currHintId);
            Ddi_BddConstrainAcc(myFrom, h_i);
            Ddi_BddSetAig(myFrom);
            Ddi_BddSetAig(h_i);
            Ddi_BddAndAcc(myFrom, h_i);
            vacuum = !Ddi_AigSat(myFrom);
            vacuum |=
              Ddi_AigSatAndWithAbort(myFrom, trAndNotReached, NULL, -1.0) == 0;

            if (0 && !vacuum) {
              Ddi_Bdd_t *myTo, *cex = Ddi_AigSatAndWithCexAndAbort(myFrom,
                trAndNotReached, NULL, NULL, -1, NULL);

              Pdtutil_Assert(cex != NULL, "NULL cex");
              Ddi_BddSetMono(cex);
              myTo = Tr_ImgWithConstrain(tr2, cex, NULL);
              Pdtutil_Assert(!Ddi_BddIncluded(myTo, reached),
                "wrong vacuum verdict");
              Ddi_Free(myTo);
              Ddi_Free(cex);
            }
            Ddi_Free(trAndNotReached);
            Ddi_Free(myFrom);

          }
          if (useMeta) {
            Ddi_BddSetMeta(h_i);
          } else {
            Ddi_BddSetMono(h_i);
          }
          if (vacuum) {
            Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
              fprintf(stdout, "\nVacuum hint [%d].\n", currHintId));
            currHintId++;
          }

        } while (vacuum && currHintId < Ddi_BddarrayNum(hints));

        Ddi_Free(prev_h);

        if (vacuum) {
          hintsAgain = 0;
          optFixPoint = metaFixPoint = 1;
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "\nTraversal Stops on vacuum hint [%d].\n",
              currHintId));
        } else {
          Ddi_BddAndAcc(from, h_i);
          optFixPoint = metaFixPoint = 0;
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "\nTraversal Resumes on Hint [%d/%d].\n",
              opt->bdd.hintsStrategy >
              0 ? opt->bdd.autoHint - maxHintIter : currHintId,
              opt->bdd.autoHint));
        }

      }
    } while (hintsAgain);

    Ddi_Free(from);
    from = Ddi_BddDup(reached);
    ddiMgr->exist.clipThresh *= 1.5;

    Tr_TrFree(tr2);

  } while ((travAgain || ddiMgr->exist.clipDone) && verifOK);

  if (Ddi_BddIsMeta(reached)) { // && (opt->mc.method == Mc_VerifTravFwd_c)) {
    Ddi_BddSetMono(reached);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|Reached(mono)|=%d; ", Ddi_BddSize(reached));
      fprintf(stdout, "#ReachedStates=%g.\n",
        Ddi_BddCountMinterm(reached, Ddi_VararrayNum(ps)));
      );
  }

  if (opt->bdd.wR != NULL) {
    Ddi_MgrReduceHeap(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"), 0);
    if (strstr(opt->bdd.wR, ".aig") != NULL) {
      int k;
      Ddi_Bddarray_t *rings = Ddi_BddarrayDup(frontier);
      for (k = 0; k < Ddi_BddarrayNum(rings); k++) {
        Ddi_Bdd_t *rA = Ddi_BddarrayRead(rings, k);

        Ddi_BddSetAig(rA);
      }
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Writing reached rings to %s (AIGER file).\n",
          opt->bdd.wR));
      Ddi_AigarrayNetStoreAiger(rings, 0, opt->bdd.wR);
      Ddi_Free(rings);
    } else {
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
                           fprintf(stdout, "Writing outer reached ring to %s.\n", opt->bdd.wR));
      
      Ddi_BddStore(reached, "reached", DDDMP_MODE_TEXT, opt->bdd.wR, NULL);
    }
  }

  if (opt->bdd.wU != NULL) {
    int i;
    int n = atoi(opt->bdd.wU);
    Ddi_Bdd_t *unr = Ddi_BddMakeConst(ddiMgr, 0);
    Ddi_Var_t *vp = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
    Ddi_Var_t *vc = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
    Ddi_Varset_t *sm = Ddi_VarsetVoid(ddiMgr);

    Ddi_VarsetAddAcc(sm, vp);
    Ddi_VarsetAddAcc(sm, vc);
    Ddi_BddExistAcc(reached, sm);
    Ddi_BddNotAcc(reached);

    if (n == 0) {
      Ddi_Bdd_t *c;
      Ddi_Varset_t *psvars = Ddi_VarsetMakeFromArray(Tr_MgrReadPS(trMgr));

      Ddi_VarsetRemoveAcc(psvars, vp);
      Ddi_VarsetRemoveAcc(psvars, vc);

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "Init state.\n"));

      c = Ddi_BddPickOneMinterm(reached, psvars);

      logBdd(c);
      Ddi_Free(psvars);
      Ddi_Free(c);
    } else {
      if (n < 0) {
        int iF = Ddi_BddarrayNum(frontier) - 2;

        n *= -1;
        Ddi_Free(reached);
        reached = Ddi_BddDup(Ddi_BddarrayRead(frontier, iF));
        Ddi_BddExistAcc(reached, sm);
        if (opt->bdd.rPlus != NULL) {
          Ddi_Bdd_t *r2 = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
            DDDMP_MODE_DEFAULT, opt->bdd.rPlus, NULL);

          Ddi_BddExistAcc(r2, sm);
          Ddi_BddDiffAcc(reached, r2);
          while (iF > 1 && Ddi_BddIsZero(reached)) {
            iF--;
            Ddi_Free(reached);
            reached = Ddi_BddDup(Ddi_BddarrayRead(frontier, iF));
            Ddi_BddExistAcc(reached, sm);
            Ddi_BddDiffAcc(reached, r2);
          }
          Ddi_Free(r2);
        }
      }

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, ".outputs p0\n\n"));

      for (i = 0; i < n; i++) {
        Ddi_Bdd_t *c;

        c = Ddi_BddPickOneCube(reached);
        Ddi_BddDiffAcc(reached, c);
        Ddi_BddOrAcc(unr, c);
        Ddi_Free(c);
      }
      LOG_BDD_CUBE_PLA(unr, " p0\n");
    }
    Ddi_Free(unr);
    Ddi_Free(sm);
  }

  Ddi_Free(from);
  Ddi_Free(fromCube);

  if (travMgr != NULL) {
    Trav_MgrSetReached(travMgr, reached);
  }

  Ddi_Free(reached);
  Ddi_Free(hints);

  Tr_TrFree(tr1);

  ddiMgr->exist.clipLevel = 0;

  if (verifOK==0) {
    if (init!=NULL && !Ddi_BddIsCube(init)) {
      verifOK = -1;
    }
  }

  if (verifOK < 0) {
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Invariant Verification: UNDECIDED.\n"));
  } else if (verifOK) {
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Invariant Verification: PASS.\n"));
  } else {
    FILE *fp;
    Trav_Mgr_t *travMgrFail;
    Ddi_Bddarray_t *patternMisMat;
    Ddi_Varset_t *piv;
    Ddi_Bdd_t *invspec_bar, *end;

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Invariant Verification: FAIL.\n"));

    if (0 ||opt->doCex >= 1) {

      Tr_MgrSetClustSmoothPi(trMgr, 0);
      travMgrFail = Trav_MgrInit(NULL, ddiMgr);
      travMgrFail->settings.bdd.mismatchOptLevel = opt->bdd.mismatchOptLevel;

#if 1
      piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));
#if 0
      /* keep PIs for counterexample */
      invspec_bar = Ddi_BddExist(invarspec, piv);
#else
      invspec_bar = Ddi_BddNot(invarspec);
#endif

      Tr_TrReverseAcc(tr);

      patternMisMat = Trav_MismatchPat(travMgrFail, tr, invspec_bar,
        NULL, NULL, &end, frontier,
        Tr_MgrReadPS(Tr_TrMgr(tr)), Tr_MgrReadNS(Tr_TrMgr(tr)), piv);

      Tr_TrReverseAcc(tr);

      /*---------------------------- Write Patterns --------------------------*/

      if (patternMisMat != NULL) {
        if (opt->doCex == 1) {
          travBddCexToFsm(fsmMgr, patternMisMat, end, 1 /* reverse */ );
        } else {
          char filename[100];

          sprintf(filename, "%s%d.txt", FILEPAT, getpid());
          fp = fopen(filename, "w");
          //fp = fopen ("pdtpat.txt", "w");
          if (Ddi_BddarrayPrintSupportAndCubes(patternMisMat, 1, -1,
              0, 1, NULL, fp) == 0) {
            Pdtutil_Warning(1, "Store cube Failed.");
          }
          fclose(fp);
        }
      }

      /*----------------------- Write Initial State Set ----------------------*/

      if (end != NULL && !Ddi_BddIsZero(end)) {
        char filename[100];

        sprintf(filename, "%s%d.txt", FILEINIT, getpid());
        fp = fopen(filename, "w");
        //fp = fopen ("pdtinit.txt", "w");
        if (Ddi_BddPrintSupportAndCubes(end, 1, -1, 0, NULL, fp) == 0) {
          Pdtutil_Warning(1, "Store cube Failed.");
        }
        fclose(fp);
        Ddi_Free(end);
      }

      Trav_MgrQuit(travMgrFail);
      Ddi_Free(piv);
      Ddi_Free(invspec_bar);
      Ddi_Free(patternMisMat);

#endif
    }

  }

  Ddi_Free(hConstr);
  Ddi_Free(myDeltaAig);
  Ddi_Free(frontier);
  Ddi_Free(nsv);
  Ddi_Free(auxv);
  Ddi_Free(nsauxv);

  currTime = util_cpu_time();
  travMgr->travTime = currTime - initialTime;

  return (verifOK);
}


/**Function*******************************************************************
  Synopsis    [exact forward verification]
  Description []
  SideEffects []
  SeeAlso     [FbvApproxForwExactBckFixPointVerify]
******************************************************************************/
int
Trav_ApproxForwExactBckFixPointVerify(
  Trav_Mgr_t * travMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care
)
{
  int i, j, verifOK, fixPoint, fwdRings, saveClipTh, doApproxRange;
  Ddi_Bdd_t *tmp, *from, *fwdReached, *bckReached, *to, *notInit, *badStates;
  Ddi_Vararray_t *ps, *ns;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(init);
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Tr_Tr_t *bckTr, *fwdTr;
  long initialTime, currTime;
  Ddi_Bddarray_t *frontier;
  char *travClk = travMgr->settings.clk;
  int apprGrTh = travMgr->settings.bdd.apprGrTh;
  int apprClustTh = travMgr->settings.tr.apprClustTh;
  int clustTh = travMgr->settings.tr.clustThreshold;

  Ddi_Bdd_t *clock_tr = NULL;
  Ddi_Var_t *clock_var = NULL;
  Ddi_Var_t *clock_var_ns = NULL;
  Tr_Tr_t *auxFwdTr = NULL;

  ps = Tr_MgrReadPS(trMgr);
  ns = Tr_MgrReadNS(trMgr);

  if (travClk != NULL) {
    clock_var = Tr_TrSetClkStateVar(tr, travClk, 1);
    if (travClk != Ddi_VarName(clock_var)) {
      travClk = Ddi_VarName(clock_var);
    }

    if (clock_var != NULL) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Clock Variable (%s) Found.\n", travClk));

      for (i = 0; i < Ddi_VararrayNum(ps); i++) {
        if (Ddi_VararrayRead(ps, i) == clock_var) {
          clock_var_ns = Ddi_VararrayRead(ns, i);
          break;
        }
      }

      if (clock_var_ns == NULL) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Clock is not a state variable.\n"));
      } else {
        for (i = 0; i < Ddi_BddPartNum(Tr_TrBdd(tr)); i++) {
          Ddi_Varset_t *supp = Ddi_BddSupp(Ddi_BddPartRead(Tr_TrBdd(tr), i));

          /*Ddi_VarsetPrint(supp, 0, NULL, stdout); */
          if (Ddi_VarInVarset(supp, clock_var_ns)) {
            clock_tr = Ddi_BddDup(Ddi_BddPartRead(Tr_TrBdd(tr), i));
            Ddi_Free(supp);
            Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
              fprintf(stdout, "Found clock TR.\n"));
            break;
          } else {
            Ddi_Free(supp);
          }
        }

      }
#if 0
      clk_lit = Ddi_BddMakeLiteral(clock_var, 0);
      Ddi_BddOrAcc(invarspec, clk_lit);
      Ddi_Free(clk_lit);
#endif
    }
  }

  badStates = Ddi_BddNot(invarspec);

  //  saveClipLevel = ddiMgr->exist.clipLevel;
  ddiMgr->exist.clipLevel = 0;

  initialTime = util_cpu_time();

  {
    Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));

    if (clock_tr != NULL) {
      Ddi_Varset_t *nsv = Ddi_VarsetMakeFromArray(ns);

      Ddi_VarsetUnionAcc(piv, nsv);
      Ddi_BddAndExistAcc(badStates, clock_tr, piv);
      Ddi_Free(nsv);
    } else {
      Ddi_BddExistAcc(badStates, piv);
    }
    Ddi_Free(piv);
  }

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "|Invar|=%d; ", Ddi_BddSize(invar));
    fprintf(stdout, "|Tr|=%d.\n", Ddi_BddSize(Tr_TrBdd(tr)))
    );

  if (invar != NULL) {
    invar = Ddi_BddDup(invar);
  } else {
#if ENABLE_INVAR
    invar = Ddi_BddPartExtract(Tr_TrBdd(tr), 0);
#else
    invar = Ddi_BddMakeConst(ddiMgr, 1);
#endif
  }

  fwdTr = Tr_TrDup(tr);

  bckTr = Tr_TrDup(tr);

  Tr_MgrSetClustSmoothPi(trMgr, 1);
  Tr_MgrSetOption(trMgr, Pdt_TrClustTh_c, inum, 1);
  Tr_TrSetClustered(fwdTr);
  Tr_TrSortIwls95(fwdTr);
  Tr_MgrSetOption(trMgr, Pdt_TrClustTh_c, inum, apprClustTh);
  Tr_TrSetClustered(fwdTr);

  Tr_MgrSetOption(trMgr, Pdt_TrClustTh_c, inum, 1);
  Tr_TrSetClustered(tr);
  Tr_TrSortIwls95(tr);
  Tr_MgrSetOption(trMgr, Pdt_TrClustTh_c, inum, clustTh);
  Tr_TrSetClustered(tr);

#if 1
  {
    Ddi_Bdd_t *group, *p;
    Ddi_Varset_t *suppGroup;
    int nsupp, nv = Ddi_VararrayNum(ps) + Ddi_VararrayNum(Tr_MgrReadI(trMgr));

    for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(fwdTr));) {
      group = Ddi_BddPartRead(Tr_TrBdd(fwdTr), j);
      Ddi_BddSetPartConj(group);
      p = Ddi_BddPartRead(Tr_TrBdd(fwdTr), i);
      Ddi_BddPartInsertLast(group, p);
      suppGroup = Ddi_BddSupp(group);
      nsupp = Ddi_VarsetNum(suppGroup);
      Ddi_Free(suppGroup);
      if ((Ddi_BddSize(group) * nsupp) / nv > apprGrTh) {
        /* resume from group insert */
        p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
        Ddi_Free(p);
#if 0
        if (clock_tr != NULL) {
          Ddi_BddPartInsert(group, 0, clock_tr);
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "#"));
        }
#endif
        i++;
        j++;
      } else {
        p = Ddi_BddPartExtract(Tr_TrBdd(fwdTr), i);
        Ddi_Free(p);
      }
    }
#if 0
    if (clock_tr != NULL) {
      Ddi_BddPartInsert(group, 0, clock_tr);
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "#"));
    }
#endif
  }

  {
    if (travMgr->settings.bdd.apprOverlap) {
      Ddi_Bdd_t *group0, *group1, *overlap, *p;
      int np;

      for (i = Ddi_BddPartNum(Tr_TrBdd(fwdTr)) - 1; i > 0; i--) {
        group0 = Ddi_BddPartRead(Tr_TrBdd(fwdTr), i - 1);
        group1 = Ddi_BddPartRead(Tr_TrBdd(fwdTr), i);
        overlap = Ddi_BddMakePartConjVoid(ddiMgr);
        if (Ddi_BddIsPartConj(group0)) {
          np = Ddi_BddPartNum(group0);
          for (j = np / 2; j < np; j++) {
            p = Ddi_BddPartRead(group0, j);
            Ddi_BddPartInsertLast(overlap, p);
          }
        } else {
          Ddi_BddPartInsertLast(overlap, group0);
        }
        if (Ddi_BddIsPartConj(group1)) {
          np = Ddi_BddPartNum(group1);
          for (j = 0; j < (np + 1) / 2; j++) {
            p = Ddi_BddPartRead(group1, j);
            Ddi_BddPartInsertLast(overlap, p);
          }
        } else {
          Ddi_BddPartInsertLast(overlap, group1);
        }
        Ddi_BddPartInsert(Tr_TrBdd(fwdTr), i, overlap);
        Ddi_Free(overlap);
      }
    }

  }
#if 0
  for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(bckTr));) {
    Ddi_Bdd_t *group, *p;

    group = Ddi_BddPartRead(Tr_TrBdd(bckTr), j);
    Ddi_BddSetPartConj(group);
    p = Ddi_BddPartRead(Tr_TrBdd(bckTr), i);
    Ddi_BddPartInsertLast(group, p);
    if (Ddi_BddSize(group) > 4 * apprGrTh) {
      /* resume from group insert */
      p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
      Ddi_Free(p);
      i++;
      j++;
    } else {
      p = Ddi_BddPartExtract(Tr_TrBdd(bckTr), i);
      Ddi_Free(p);
    }
  }
#endif
#endif


  /* approx forward */

  from = Ddi_BddDup(init);

  /* disjunctively partitioned reached */
  fwdReached = Ddi_BddMakePartDisjVoid(ddiMgr);
  Ddi_BddPartInsert(fwdReached, 0, from);

  if (travClk != NULL) {
    Tr_Tr_t *newTr = Tr_TrPartitionWithClock(fwdTr, clock_var,
      travMgr->settings.bdd.implications);

    if (newTr != NULL) {
      Tr_TrFree(fwdTr);
      fwdTr = newTr;
    }
  }

  if (travMgr->settings.bdd.rPlus != NULL) {
    Ddi_Bdd_t *r;

    if (care != NULL) {
      r = Ddi_BddDup(care);
    } else if (strcmp(travMgr->settings.bdd.rPlus, "1") == 0) {
      r = Ddi_BddMakeConst(ddiMgr, 1);
    } else {
      Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "-- Reading CARE set from %s.\n",
          travMgr->settings.bdd.rPlus));
      r = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
        DDDMP_MODE_DEFAULT, travMgr->settings.bdd.rPlus, NULL);
    }
#if 1
    Ddi_BddPartInsertLast(fwdReached, r);
#else
    care = Ddi_BddDup(r);
#endif
    Ddi_Free(r);
    Ddi_Free(from);
  } else if (travMgr->settings.bdd.apprMBM) {
    Ddi_Bdd_t *r;
    Trav_Mgr_t *travMgr1 = Trav_MgrInit(NULL, ddiMgr);

    Trav_MgrSetFromSelect(travMgr1, Trav_FromSelectRestrict_c);

    Ddi_BddAndAcc(from, invar);

    Trav_MgrSetI(travMgr1, Tr_MgrReadI(trMgr));
    Trav_MgrSetPS(travMgr1, Tr_MgrReadPS(trMgr));
    Trav_MgrSetNS(travMgr1, Tr_MgrReadNS(trMgr));
    Trav_MgrSetFrom(travMgr1, from);
    Trav_MgrSetReached(travMgr1, from);
    Trav_MgrSetTr(travMgr1, fwdTr);
    if ((Trav_MgrReadVerbosity(travMgr) >= Pdtutil_VerbLevelNone_c &&
        (Trav_MgrReadVerbosity(travMgr) <= Pdtutil_VerbLevelSame_c))) {
      Trav_MgrSetOption(travMgr1, Pdt_TravVerbosity_c, inum,
        Trav_MgrReadVerbosity(travMgr));
    }


    r = Trav_ApproxMBM(travMgr, NULL);
    Ddi_BddPartInsertLast(fwdReached, r);
    Ddi_Free(from);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "-- TRAV Manager Free.\n"));

    Trav_MgrQuit(travMgr);
  } else {

    Ddi_BddAndAcc(from, invar);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "Approximate Forward.\n"));

    if (travMgr->settings.bdd.constrain) {
      Tr_MgrSetImgMethod(Tr_TrMgr(fwdTr), Tr_ImgMethodCofactor_c);
    }

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "|TR|=%d\n", Ddi_BddSize(Tr_TrBdd(fwdTr))));

    saveClipTh = ddiMgr->exist.clipThresh;
    ddiMgr->exist.clipThresh = 100000000;

    doApproxRange = (travMgr->settings.bdd.maxFwdIter == 0);
    for (i = 0, fixPoint = doApproxRange; (!fixPoint); i++) {

      if (i >= travMgr->settings.bdd.nExactIter) {
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
          fprintf(stdout,
            "-- Approximate Forward Verification; #Iteration=%d.\n", i + 1));

        to = Tr_ImgApprox(fwdTr, from, care);
      } else {
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "-- Exact Forward Verification; #Iteration=%d.\n",
            i + 1));

        if (travMgr->settings.bdd.useMeta) {
          int initSize = Ddi_BddSize(from);

          Ddi_BddSetMeta(from);
          if (i > 0) {
            Ddi_Bdd_t *r = Ddi_BddMakeMeta(Ddi_BddPartRead(fwdReached, i - 1));

            Ddi_BddDiffAcc(from, r);
            Ddi_Free(r);
          }
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "From: part:%d -> meta:%d.\n",
              initSize, Ddi_BddSize(from)));
        }
        to = Tr_Img(fwdTr, from);
        if (travMgr->settings.bdd.useMeta) {
          Ddi_Bdd_t *r = Ddi_BddMakeMeta(Ddi_BddPartRead(fwdReached, i));

          Ddi_BddOrAcc(to, r);
          Ddi_Free(r);
          Ddi_BddSetPartConj(to);
        }
      }

      /*      Ddi_BddAndAcc(from,invarspec); */

      if (travMgr->settings.bdd.apprNP > 0) {
        to = Ddi_BddEvalFree(Tr_TrProjectOverPartitions(to, NULL, NULL,
            travMgr->settings.bdd.apprNP,
            travMgr->settings.bdd.apprPreimgTh), to);
      }
      /*Ddi_BddSetMono(to); */

      if (travMgr->settings.bdd.fromSelect != Trav_FromSelectNew_c) {
        tmp = Ddi_BddAnd(init, to);
        Ddi_BddSetMono(tmp);
        if (!Ddi_BddIsZero(tmp)) {
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "Init States is Reachable from INIT.\n"));
        } else {
          if (!travMgr->settings.bdd.strictBound) {
            if (1 || (clock_var_ns == NULL) || (i % 2 == 0)) {
              orProjectedSetAcc(to, init, clock_var);
            }
          }
        }
        Ddi_Free(tmp);
      }

      Ddi_Free(from);

      from = Ddi_BddDup(to);
      if (Ddi_BddIsMeta(to)) {
        Ddi_BddSetPartConj(to);
      }

      if (travMgr->settings.bdd.fromSelect == Trav_FromSelectNew_c) {
        Ddi_Bdd_t *r;

        r = Ddi_BddDup(Ddi_BddPartRead(fwdReached, i));
        if (0) {
          diffProjectedSetAcc(from, r, clock_var);
          orProjectedSetAcc(r, to, clock_var);
          Ddi_Free(to);
          to = r;
        } else {
          orProjectedSetAcc(to, r, clock_var);
          Ddi_Free(r);
        }
      }
      Ddi_BddPartInsertLast(fwdReached, to);

      if ((travMgr->settings.bdd.maxFwdIter >= 0) &&
        (i >= travMgr->settings.bdd.maxFwdIter - 1)) {
        doApproxRange = 1;
        fixPoint = 1;
      } else {
        int k;

        for (k = i; k >= 0; k--) {
          fixPoint = Ddi_BddEqual(to, Ddi_BddPartRead(fwdReached, k));
          if (fixPoint) {
            break;
          }
        }
      }

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "|To|=%d; ", Ddi_BddSize(to));
        fprintf(stdout, "|Reached|=%d", Ddi_BddSize(fwdReached));
        if (Ddi_BddIsMono(to)) {
          fprintf(stdout, "; #ToStates=%g",
          Ddi_BddCountMinterm(to, Ddi_VararrayNum(ps)));
        }
        fprintf(stdout, ".\n");
      );
      Ddi_Free(to);
    }

    ddiMgr->exist.clipThresh = saveClipTh;

    Ddi_Free(from);

    if (doApproxRange) {
      int k;

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "Forward Verisification; Iteration Limit Reached.\n");
        fprintf(stdout, "Approximate Range used for Outer Rplus.\n"););

      from = Ddi_BddMakeConst(ddiMgr, 1);
      for (k = 0; k < travMgr->settings.bdd.gfpApproxRange; k++) {
        to = Tr_ImgApprox(fwdTr, from, NULL);
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "|GFP range[%d]|=%d.\n", k, Ddi_BddSize(to)););
        Ddi_Free(from);
        from = to;
      }
      orProjectedSetAcc(to, init, NULL);
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "|Rplus(range)|=%d\n", Ddi_BddSize(to)););
      Ddi_BddPartInsertLast(fwdReached, to);
      Ddi_Free(to);
    }

  }

  Ddi_Free(clock_tr);

  if (travMgr->settings.bdd.wR != NULL) {
    Ddi_Bdd_t *r =
      Ddi_BddDup(Ddi_BddPartRead(fwdReached, Ddi_BddPartNum(fwdReached) - 1));
    Ddi_SetName(r, "rPlus");
    if (Ddi_BddIsPartConj(r)) {
      int i;

      if (care != NULL) {
        Ddi_BddPartInsertLast(r, care);
      }
      for (i = 0; i < Ddi_BddPartNum(r); i++) {
        char name[100];

        sprintf(name, "rPlus_part_%d", i);
        Ddi_SetName(Ddi_BddPartRead(r, i), name);
      }
    } else if (care != NULL) {
      Ddi_BddAndAcc(r, care);
    }

    if (strstr(travMgr->settings.bdd.wR, ".blif") != NULL) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Writing outer reached ring to %s (BLIF file).\n",
          travMgr->settings.bdd.wR););
      Ddi_BddStoreBlif(r, NULL, NULL, "reachedPlus", travMgr->settings.bdd.wR,
        NULL);
    } else {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Writing outer reached ring to %s\n",
          travMgr->settings.bdd.wR););
      Ddi_BddStore(r, "reachedPlus", DDDMP_MODE_TEXT, travMgr->settings.bdd.wR,
        NULL);
    }

    if (0 && travMgr->settings.bdd.wP != NULL) {
      Tr_TrWritePartitions(fwdTr, travMgr->settings.bdd.wP);
    }

    if (travMgr->settings.bdd.countReached) {
      int nv = Ddi_VararrayNum(Tr_MgrReadPS(trMgr));

      if (0 || (Ddi_BddSize(r) < 100)) {
        Ddi_BddSetMono(r);
      }

      if (nv < 1000) {
        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "|Reached(mono)|=%d; ", Ddi_BddSize(r));
          fprintf(stdout, "#ReachedStates=%g.\n",
            Ddi_BddCountMinterm(r, nv));
          fprintf(stdout, "#R States(estimated)=%g\n",
            Ddi_AigEstimateMintermCount(r, Ddi_VararrayNum(ps)));
        );
      } else {
        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "|Reached(mono)|=%d; ", Ddi_BddSize(r));
          double res = Ddi_BddCountMinterm(r, nv - 1000);
          double fact = pow((pow(2, 100) / pow(10, 30)), 10);
          res = res * fact;
          fprintf(stdout, "#Reached states = %g * 10^300\n", res);
        );
      }
    }

    Ddi_Free(r);

    exit(1);
  } else if (travMgr->settings.bdd.countReached) {
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c, {
        Ddi_Bdd_t * ra = Ddi_BddMakeAig(Ddi_BddPartRead(fwdReached,
            Ddi_BddPartNum(fwdReached) - 1));
        fprintf(stdout, "#R States(estimated)=%g\n",
          Ddi_AigEstimateMintermCount(ra, Ddi_VararrayNum(ps)));
        Ddi_Bdd_t * r = Ddi_BddMakeMono(Ddi_BddPartRead(fwdReached,
            Ddi_BddPartNum(fwdReached) - 1));
        fprintf(stdout, "|Reached(mono)|=%d; ", Ddi_BddSize(r));
        fprintf(stdout, "#ReachedStates=%g.\n",
          Ddi_BddCountMinterm(r, Ddi_VararrayNum(Tr_MgrReadPS(trMgr))));
        Ddi_Free(ra);
        Ddi_Free(r);
      }
    );
  }

  /* Ddi_BddSetMono(fwdReached); */

  /* exact backward */

#if 1
  if (travMgr->settings.bdd.useMeta &&
    travMgr->settings.bdd.metaMethod == Ddi_Meta_EarlySched) {
    Ddi_MetaInit(Ddi_ReadMgr(init), Ddi_Meta_EarlySched, Tr_TrBdd(tr), NULL,
      Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr), travMgr->settings.bdd.useMeta);
    Ddi_MgrSetMetaMaxSmooth(ddiMgr, travMgr->settings.bdd.metaMaxSmooth);
    /*Ddi_BddSetMeta(fwdReached); */
  }
#endif

  /* clipLevel = saveClipLevel; */
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "-- Exact Backward; Using Fix Point R+.\n"));

#if 1
  auxFwdTr = fwdTr;
  Tr_MgrSetOption(trMgr, Pdt_TrApproxMaxIter_c, inum, 1);
#else
  Tr_TrFree(fwdTr);
#endif

  if (travMgr->settings.tr.sortForBck != 0) {
    Tr_TrSortForBck(bckTr);
    Tr_MgrSetOption(trMgr, Pdt_TrClustTh_c, inum, clustTh);
    Tr_TrSetClustered(bckTr);
  } else {
    Tr_TrFree(bckTr);
    bckTr = Tr_TrDup(fwdTr);
  }

  verifOK = 1;

  Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodIwls95_c);
#if 0
  Tr_MgrSetOption(trMgr, Pdt_TrClustTh_c, inum, 100000);
#endif

#if 0
#if 1
  Ddi_BddConstrainAcc(Tr_TrBdd(bckTr), fwdReached);
#endif
#endif

#if 0
  Tr_TrSortIwls95(bckTr);
  Tr_TrSetClustered(bckTr);
#endif

  notInit = Ddi_BddNot(init);

  from = Ddi_BddMakeConst(ddiMgr, 0);
  for (i = Ddi_BddPartNum(fwdReached) - 1; i >= 0; i--) {
    Ddi_Free(from);
    from = Ddi_BddAnd(Ddi_BddPartRead(fwdReached, i), badStates);
    if (!Ddi_BddIsZero(from))
      break;
  }
  Ddi_Free(from);
  from = Ddi_BddDup(badStates);

  Ddi_BddRestrictAcc(Tr_TrBdd(bckTr),
    Ddi_BddPartRead(fwdReached, Ddi_BddPartNum(fwdReached) - 1));

  if (travClk != NULL) {
    Tr_Tr_t *newTr = Tr_TrPartition(bckTr, travClk, 1);

    if (newTr != NULL) {
      Ddi_Bdd_t *ck_p0, *ck_p1;

      Tr_TrFree(bckTr);
      bckTr = newTr;
      ck_p0 = Ddi_BddPartExtract(Ddi_BddPartRead(Tr_TrBdd(bckTr), 0), 0);
      ck_p1 = Ddi_BddPartExtract(Ddi_BddPartRead(Tr_TrBdd(bckTr), 1), 0);
      Ddi_BddSetClustered(Tr_TrBdd(bckTr), clustTh);
      Ddi_BddSetPartConj(Ddi_BddPartRead(Tr_TrBdd(bckTr), 0));
      Ddi_BddSetPartConj(Ddi_BddPartRead(Tr_TrBdd(bckTr), 1));
      Ddi_BddPartInsert(Ddi_BddPartRead(Tr_TrBdd(bckTr), 0), 0, ck_p0);
      Ddi_BddPartInsert(Ddi_BddPartRead(Tr_TrBdd(bckTr), 1), 0, ck_p1);
      Ddi_Free(ck_p0);
      Ddi_Free(ck_p1);
    } else {
      Ddi_BddSetClustered(Tr_TrBdd(bckTr), clustTh);
    }

    {
      Ddi_Bdd_t *p0, *p1, *f, *fpart;

      if (travMgr->settings.tr.trDpartTh > 0) {
        p0 = Ddi_BddPartRead(Tr_TrBdd(bckTr), 0);
        p1 = Ddi_BddPartRead(Tr_TrBdd(bckTr), 1);
        f = Ddi_BddPartRead(p0, 1);
        fpart = Part_PartitionDisjSet(f, NULL, NULL,
          /*Part_MethodCofactor_c, */ Part_MethodCofactorSupport_c,
          travMgr->settings.tr.trDpartTh, 3, Pdtutil_VerbLevelDevMax_c);
        Ddi_BddPartWrite(p0, 1, fpart);
        Ddi_Free(fpart);
        f = Ddi_BddPartRead(p1, 1);
        fpart = Part_PartitionDisjSet(f, NULL, NULL,
          /*Part_MethodCofactor_c, */ Part_MethodCofactorSupport_c,
          2 * Ddi_BddSize(f) / 3, 3, Pdtutil_VerbLevelDevMax_c);
        Ddi_BddPartWrite(p1, 1, fpart);
        Ddi_Free(fpart);
      }

    }

  } else {
    Ddi_BddSetClustered(Tr_TrBdd(bckTr), clustTh);

    if (travMgr->settings.tr.trDpartTh > 0) {
      Ddi_Bdd_t *newTrBdd, *f;

      f = Tr_TrBdd(bckTr);
      newTrBdd = Part_PartitionDisjSet(f, NULL, NULL, Part_MethodCofactor_c,
        travMgr->settings.tr.trDpartTh, 3, Pdtutil_VerbLevelDevMax_c);
      TrTrRelWrite(bckTr, newTrBdd);
      Ddi_Free(newTrBdd);
    }
  }

  if (travMgr->settings.bdd.noCheck) {
    Ddi_BddAndAcc(init, Ddi_MgrReadZero(ddiMgr));
  }

  if (travMgr->settings.bdd.wP != NULL) {
    char name[200];

    sprintf(name, "fwd-appr-%s", travMgr->settings.bdd.wP);
    Tr_TrWritePartitions(fwdTr, name);
    sprintf(name, "%s", travMgr->settings.bdd.wP);
    Tr_TrWritePartitions(bckTr, name);
  }
  if (travMgr->settings.bdd.wOrd != NULL) {
    char name[200];

    sprintf(name, "%s.0", travMgr->settings.bdd.wOrd);
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "Writing intermediate variable order to file %s\n",
        name));
    Ddi_MgrOrdWrite(ddiMgr, name, NULL, Pdtutil_VariableOrderDefault_c);
  }

  if (Ddi_BddIsAig(init)) {
    if (invar != NULL && !Ddi_BddIsOne(invar)) {
      Ddi_Bdd_t *i1 = Ddi_BddMakeAig(invar);

      Ddi_BddAndAcc(init, i1);
      Ddi_Free(i1);
    }
  }

  do {
    travMgr->settings.ints.univQuantifyDone = 0;
    if (!travMgr->settings.bdd.prioritizedMc) {
      bckReached = Ddi_BddMakePartDisjVoid(ddiMgr);

      /*Tr_TrReverseAcc(bckTr); */
      verifOK = !travOverRingsWithFixPoint(from, NULL, init, bckTr, fwdReached,
        bckReached, NULL, Ddi_BddPartNum(fwdReached),
        "Exact Backward Verification", initialTime, 1 /*reverse */ ,
        0 /*approx */ , &travMgr->settings);
    } else {
      //      bckReached = travBckInvarCheckRecur(travMgr,from,init,bckTr,fwdReached);
      verifOK = (bckReached == NULL);
    }

    if (1 && travMgr->settings.ints.univQuantifyDone) {
      int nq;

      Ddi_Free(from);
      from = bckReached;
      nq = Ddi_VarsetNum(travMgr->settings.ints.univQuantifyVars);
      if (nq < 10) {
        Ddi_Free(travMgr->settings.ints.univQuantifyVars);
        travMgr->settings.bdd.univQuantifyTh = -1;
      } else {
        for (i = 0; i < nq / 2; i++) {
          Ddi_Var_t *v =
            Ddi_VarsetTop(travMgr->settings.ints.univQuantifyVars);
          Ddi_VarsetRemoveAcc(travMgr->settings.ints.univQuantifyVars, v);
        }
      }
    }

  } while (1 && travMgr->settings.ints.univQuantifyDone);

  frontier = NULL;

  if (travMgr->settings.bdd.wBR != NULL) {
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "Writing backward reached to %s\n",
        travMgr->settings.bdd.wBR));
    Ddi_BddStore(bckReached, "backReached", DDDMP_MODE_TEXT,
      travMgr->settings.bdd.wBR, NULL);
  }

  Tr_TrFree(auxFwdTr);

  if (!verifOK) {
    frontier = Ddi_BddarrayMakeFromBddPart(bckReached);
    fwdRings = 0;
  }

  Tr_TrFree(bckTr);
  Ddi_Free(bckReached);
  Ddi_Free(from);
  Ddi_Free(invar);
  Ddi_Free(notInit);
  Ddi_Free(fwdReached);
  //  Ddi_Free(care);

  if (verifOK) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: PASS.\n"));
  } else {
    FILE *fp;
    Trav_Mgr_t *travMgr1;
    Ddi_Bddarray_t *patternMisMat;
    Ddi_Varset_t *piv;
    Ddi_Bdd_t *invspec_bar, *end;

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: FAIL.\n"));

    Tr_MgrSetClustSmoothPi(trMgr, 0);
    travMgr1 = Trav_MgrInit(NULL, ddiMgr);
    Trav_MgrSetOption(travMgr1, Pdt_TravMismOptLevel_c, inum,
      travMgr->settings.bdd.mismatchOptLevel);

#if 0
    ddiMgr->exist.clipThresh = 100000000;
#else
    ddiMgr->exist.clipLevel = 0;
#endif

#if 1
    piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));
#if 0
    /* keep PIs for counterexample */
    invspec_bar = Ddi_BddExist(invarspec, piv);
#else
    invspec_bar = Ddi_BddNot(invarspec);
#endif

#if 0
    Tr_MgrSetImgMethod(Tr_TrMgr(tr), Tr_ImgMethodCofactor_c);
#endif

    if (fwdRings) {

      Tr_TrReverseAcc(tr);
      patternMisMat = Trav_MismatchPat(travMgr1, tr, invspec_bar,
        NULL, NULL, &end, frontier,
        Tr_MgrReadPS(Tr_TrMgr(tr)), Tr_MgrReadNS(Tr_TrMgr(tr)), piv);
      Tr_TrReverseAcc(tr);
    } else {
      patternMisMat = Trav_MismatchPat(travMgr1, tr, NULL,
        invspec_bar, &end, NULL, frontier,
        Tr_MgrReadPS(Tr_TrMgr(tr)), Tr_MgrReadNS(Tr_TrMgr(tr)), piv);
    }

    /*---------------------------- Write Patterns --------------------------*/

    if (patternMisMat != NULL) {
      char filename[100];

      sprintf(filename, "%s%d.txt", FILEPAT, getpid());
      fp = fopen(filename, "w");
      //fp = fopen ("pdtpat.txt", "w");
      if (Ddi_BddarrayPrintSupportAndCubes(patternMisMat, 1, -1,
          0, 0, NULL, fp) == 0) {
        Pdtutil_Warning(1, "Store cube Failed.");
      }
      fclose(fp);
    }

    /*----------------------- Write Initial State Set ----------------------*/

    if (end != NULL && !Ddi_BddIsZero(end)) {
      char filename[100];

      sprintf(filename, "%s%d.txt", FILEINIT, getpid());
      fp = fopen(filename, "w");
      //fp = fopen ("pdtinit.txt", "w");
      if (Ddi_BddPrintSupportAndCubes(end, 1, -1, 0, NULL, fp) == 0) {
        Pdtutil_Warning(1, "Store cube Failed.");
      }
      fclose(fp);
      Ddi_Free(end);
    }

    Trav_MgrQuit(travMgr1);
    Ddi_Free(piv);
    Ddi_Free(invspec_bar);
    Ddi_Free(patternMisMat);

#endif

  }

  Ddi_Free(frontier);
  Ddi_Free(badStates);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c,
    currTime = util_cpu_time();
    fprintf(stdout, "Live nodes (peak): %u(%u)\n",
      Ddi_MgrReadLiveNodes(ddiMgr), Ddi_MgrReadPeakLiveNodeCount(ddiMgr));
    fprintf(stdout, "TotalTime: %s ",
      util_print_time(currTime - travMgr->startTime));
    fprintf(stdout, "(setup: %s -",
      util_print_time(initialTime - travMgr->startTime));
    fprintf(stdout, " trav: %s)\n", util_print_time(currTime - initialTime))
    );

  return (verifOK);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
int
TravBddOutOfLimits(
  Trav_Mgr_t * travMgr
)
{
  Ddi_Mgr_t *ddiMgr = travMgr->ddiMgrR;

  return 0;

  if (travMgr->settings.bdd.bddTimeLimit > 0 &&
    (util_cpu_time() > travMgr->settings.bdd.bddTimeLimit)) {
    Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMed_c) {
      fprintf(tMgrO(travMgr), "Time limit reached.\n");
    }
    return 1;
  }

  if (travMgr->settings.bdd.bddNodeLimit > 0 &&
    (Ddi_MgrReadLiveNodes(ddiMgr) > travMgr->settings.bdd.bddNodeLimit)) {
    Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMed_c) {
      fprintf(tMgrO(travMgr), "Peak node limit reached.\n");
    }
    return 1;
  }

  return 0;
}


/**Function*******************************************************************
  Synopsis    [exact forward verification]
  Description []
  SideEffects []
  SeeAlso     [FbvApproxForwExactBckFixPointVerify]
******************************************************************************/
int
Trav_TravBddApproxForwExactBckFixPointVerify(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  Ddi_Bddarray_t * delta
)
{
  int i, j, verifOK, fixPoint, fwdRings, saveClipTh, doApproxRange;
  Ddi_Bdd_t *tmp, *from, *fwdReached, *bckReached, *to, *notInit, *badStates;
  Ddi_Vararray_t *ps, *ns;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(init);
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Tr_Tr_t *bckTr, *fwdTr;
  Trav_Settings_t *opt = &travMgr->settings;
  Trav_FromSelect_e fromSelect = opt->bdd.fromSelect;
  int itpAppr = opt->aig.itpAppr;
  int satRefine = itpAppr > 0 ? itpAppr : 0;
  int satRefineInductive = itpAppr < 0 ? -itpAppr : 0;
  char *travClk = opt->clk;
  int clustTh = opt->tr.clustThreshold;
  int useMeta = opt->bdd.useMeta;
  long initialTime, currTime;
  Ddi_Bddarray_t *frontier;
  Ddi_Bdd_t *trAig = NULL;
  Ddi_Bdd_t *prevRef = NULL;
  Ddi_Bdd_t *clock_tr = NULL;
  Ddi_Var_t *clock_var = NULL;
  Ddi_Var_t *clock_var_ns = NULL;

  ps = Tr_MgrReadPS(trMgr);
  ns = Tr_MgrReadNS(trMgr);

  if (satRefine || satRefineInductive) {
    trAig = Ddi_BddRelMakeFromArray(delta, ns);
    Ddi_BddSetAig(trAig);

    if (satRefine && travMgr->settings.aig.itpInductiveTo) {
      prevRef = Ddi_BddMakeConstAig(ddiMgr, 1);
    }
  }

  if (opt->tr.enableVar != NULL) {
    Ddi_Var_t *enVar = Ddi_VarFromName(ddiMgr, opt->tr.enableVar);
    Ddi_Var_t *enAuxPs, *enAuxNs;
    char buf[1000];
    Ddi_Bdd_t *trEn, *lit;

    enAuxPs = Ddi_VarNewAfterVar(enVar);
    sprintf(buf, "%s$ENABLE", opt->tr.enableVar);
    Ddi_VarAttachName(enAuxPs, buf);
    enAuxNs = Ddi_VarNewAfterVar(enVar);
    sprintf(buf, "%s$ENABLE$NS", opt->tr.enableVar);
    Ddi_VarAttachName(enAuxNs, buf);
    Ddi_VarMakeGroup(ddiMgr, enAuxNs, 2);
    trEn = Ddi_BddMakeLiteral(enAuxNs, 1);
    lit = Ddi_BddMakeLiteral(enVar, 1);
    Ddi_BddXnorAcc(trEn, lit);
    Ddi_Free(lit);
    Ddi_BddPartInsert(Tr_TrBdd(tr), 1, trEn);
    Ddi_Free(trEn);
    Ddi_VararrayInsert(ps, 1, enAuxPs);
    Ddi_VararrayInsert(ns, 1, enAuxNs);
  }
  if (opt->tr.auxVarFile != NULL) {
    Tr_TrAuxVars(tr, opt->tr.auxVarFile);
  }

  if (travClk != NULL) {
#if 0
    {
      Ddi_Varset_t *reducedVars = Ddi_VarsetMakeFromArray(ps);
      Ddi_Bddarray_t *l = Ddi_BddarrayAlloc(ddiMgr, 0);

      Ddi_BddarrayInsert(l, 0, invarspec);
      Tr_TrReduceMasterSlave(tr, reducedVars,
        l, Ddi_VarFromName(ddiMgr, travClk));
      Ddi_Free(reducedVars);
      DdiGenericDataCopy((Ddi_Generic_t *) invarspec,
        (Ddi_Generic_t *) Ddi_BddarrayRead(l, 0));
      Ddi_Free(l);
    }
#endif

#if 0
    clock_var = Ddi_VarFromName(ddiMgr, travClk);
#else
    clock_var = Tr_TrSetClkStateVar(tr, travClk, 1);
    if (travClk != Ddi_VarName(clock_var)) {
      travClk = Ddi_VarName(clock_var);
    }
#endif

    if (clock_var != NULL) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Clock Variable (%s) Found.\n", travClk));

      for (i = 0; i < Ddi_VararrayNum(ps); i++) {
        if (Ddi_VararrayRead(ps, i) == clock_var) {
          clock_var_ns = Ddi_VararrayRead(ns, i);
          break;
        }
      }

      if (clock_var_ns == NULL) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Clock is not a state variable.\n"));
      } else {
        for (i = 0; i < Ddi_BddPartNum(Tr_TrBdd(tr)); i++) {
          Ddi_Varset_t *supp = Ddi_BddSupp(Ddi_BddPartRead(Tr_TrBdd(tr), i));

          /*Ddi_VarsetPrint(supp, 0, NULL, stdout); */
          if (Ddi_VarInVarset(supp, clock_var_ns)) {
            clock_tr = Ddi_BddDup(Ddi_BddPartRead(Tr_TrBdd(tr), i));
            Ddi_Free(supp);
            Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
              fprintf(stdout, "Found clock TR.\n"));
            break;
          } else {
            Ddi_Free(supp);
          }
        }
      }
#if 0
      clk_lit = Ddi_BddMakeLiteral(clock_var, 0);
      Ddi_BddOrAcc(invarspec, clk_lit);
      Ddi_Free(clk_lit);
#endif
    }
  }

  badStates = Ddi_BddNot(invarspec);

  opt->ints.saveClipLevel = ddiMgr->exist.clipLevel;
  ddiMgr->exist.clipLevel = 0;

  initialTime = util_cpu_time();

  {
    Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));

    if (clock_tr != NULL) {
      Ddi_Varset_t *nsv = Ddi_VarsetMakeFromArray(ns);

      Ddi_VarsetUnionAcc(piv, nsv);
      Ddi_BddAndExistAcc(badStates, clock_tr, piv);
      Ddi_Free(nsv);
    } else {
      Ddi_BddExistAcc(badStates, piv);
    }
    Ddi_Free(piv);
  }

  Pdtutil_VerbosityMgrIf(trMgr, Pdtutil_VerbLevelUsrMed_c) {
    fprintf(stdout, "|Invar|=%d; ", Ddi_BddSize(invar));
    fprintf(stdout, "|Tr|=%d.\n", Ddi_BddSize(Tr_TrBdd(tr)));
  }

  if (opt->bdd.invarFile != NULL) {
    Ddi_BddPartInsert(Tr_TrBdd(tr), 1, invar);
  } else if (invar != NULL) {
    invar = Ddi_BddDup(invar);
  } else {
#if ENABLE_INVAR
    invar = Ddi_BddPartExtract(Tr_TrBdd(tr), 0);
#else
    invar = Ddi_BddMakeConst(ddiMgr, 1);
#endif
  }

  fwdTr = Tr_TrDup(tr);

  if (0 && opt->tr.approxTrClustFile != NULL) {
    Tr_Tr_t *newTr = Tr_TrManualPartitions(fwdTr, opt->tr.approxTrClustFile);

    Tr_TrFree(fwdTr);
    fwdTr = newTr;
    // opt->tr.apprClustTh = 0;
  }
  if (0 && opt->tr.approxTrClustFile != NULL) {
    opt->tr.apprClustTh = 0;
  }

  bckTr = Tr_TrDup(tr);

  Tr_MgrSetClustSmoothPi(trMgr, 1);
  Tr_MgrSetClustThreshold(trMgr, 1);
  if (opt->tr.approxTrClustFile != NULL) {
    Tr_TrSetClusteredApprGroups(fwdTr);
    Tr_TrSortIwls95(fwdTr);
    Tr_MgrSetClustThreshold(trMgr, opt->tr.apprClustTh);
    Tr_TrSetClusteredApprGroups(fwdTr);
  } else {
    Tr_TrSetClustered(fwdTr);
    Tr_TrSortIwls95(fwdTr);
    Tr_MgrSetClustThreshold(trMgr, opt->tr.apprClustTh);
    Tr_TrSetClustered(fwdTr);
  }

  Tr_MgrSetClustThreshold(trMgr, 1);
  Tr_TrSetClustered(tr);
  Tr_TrSortIwls95(tr);
  Tr_MgrSetClustThreshold(trMgr, clustTh);
  Tr_TrSetClustered(tr);

#if 1
  {
    Ddi_Bdd_t *group, *p;
    Ddi_Varset_t *suppGroup;
    int nsupp, nv = Ddi_VararrayNum(ps) + Ddi_VararrayNum(Tr_MgrReadI(trMgr));

    for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(fwdTr));) {
      group = Ddi_BddPartRead(Tr_TrBdd(fwdTr), j);
      Ddi_BddSetPartConj(group);
      p = Ddi_BddPartRead(Tr_TrBdd(fwdTr), i);
      Ddi_BddPartInsertLast(group, p);
      suppGroup = Ddi_BddSupp(group);
      nsupp = Ddi_VarsetNum(suppGroup);
      Ddi_Free(suppGroup);
      if ((Ddi_BddSize(group) * nsupp) / nv > opt->bdd.apprGrTh) {
        /* resume from group insert */
        p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
        Ddi_Free(p);
#if 0
        if (clock_tr != NULL) {
          Ddi_BddPartInsert(group, 0, clock_tr);
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "#"));
        }
#endif
        i++;
        j++;
      } else {
        p = Ddi_BddPartExtract(Tr_TrBdd(fwdTr), i);
        Ddi_Free(p);
      }
    }
#if 0
    if (clock_tr != NULL) {
      Ddi_BddPartInsert(group, 0, clock_tr);
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "#"));
    }
#endif
  }

  {
    if (travMgr->settings.bdd.apprOverlap) {
      Ddi_Bdd_t *group0, *group1, *overlap, *p;
      int np;

      for (i = Ddi_BddPartNum(Tr_TrBdd(fwdTr)) - 1; i > 0; i--) {
        group0 = Ddi_BddPartRead(Tr_TrBdd(fwdTr), i - 1);
        group1 = Ddi_BddPartRead(Tr_TrBdd(fwdTr), i);
        overlap = Ddi_BddMakePartConjVoid(ddiMgr);
        if (Ddi_BddIsPartConj(group0)) {
          np = Ddi_BddPartNum(group0);
          for (j = np / 2; j < np; j++) {
            p = Ddi_BddPartRead(group0, j);
            Ddi_BddPartInsertLast(overlap, p);
          }
        } else {
          Ddi_BddPartInsertLast(overlap, group0);
        }
        if (Ddi_BddIsPartConj(group1)) {
          np = Ddi_BddPartNum(group1);
          for (j = 0; j < (np + 1) / 2; j++) {
            p = Ddi_BddPartRead(group1, j);
            Ddi_BddPartInsertLast(overlap, p);
          }
        } else {
          Ddi_BddPartInsertLast(overlap, group1);
        }
        Ddi_BddPartInsert(Tr_TrBdd(fwdTr), i, overlap);
        Ddi_Free(overlap);
      }
    }
  }

#if 0
  for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(bckTr));) {
    Ddi_Bdd_t *group, *p;

    group = Ddi_BddPartRead(Tr_TrBdd(bckTr), j);
    Ddi_BddSetPartConj(group);
    p = Ddi_BddPartRead(Tr_TrBdd(bckTr), i);
    Ddi_BddPartInsertLast(group, p);
    if (Ddi_BddSize(group) > 4 * opt->bdd.apprGrTh) {
      /* resume from group insert */
      p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
      Ddi_Free(p);
      i++;
      j++;
    } else {
      p = Ddi_BddPartExtract(Tr_TrBdd(bckTr), i);
      Ddi_Free(p);
    }
  }
#endif
#endif


  /* approx forward */

  from = Ddi_BddDup(init);

#if 0
  if (travMgr->settings.bdd.noInit) {
    Ddi_BddAndAcc(init, Ddi_MgrReadZero(ddiMgr));
  }
#endif

  /* disjunctively partitioned reached */
  fwdReached = Ddi_BddMakePartDisjVoid(ddiMgr);
  Ddi_BddPartInsert(fwdReached, 0, from);

  if (opt->tr.auxVarFile != NULL) {
    Tr_TrAuxVars(bckTr, opt->tr.auxVarFile);
  }
  if (travClk != 0) {
    Tr_Tr_t *newTr = Tr_TrPartitionWithClock(fwdTr, clock_var,
      travMgr->settings.bdd.implications);

    if (newTr != NULL) {
      Tr_TrFree(fwdTr);
      fwdTr = newTr;
    }
  }

  if (opt->bdd.rPlus != NULL) {
    Ddi_Bdd_t *r;

    if (care != NULL) {
      Ddi_Varset_t *s = Ddi_BddSupp(init);

      r = Ddi_BddDup(care);
      //Ddi_BddExistProjectAcc(r,s);
      Ddi_Free(s);
    } else if (strcmp(opt->bdd.rPlus, "1") == 0) {
      r = Ddi_BddMakeConst(ddiMgr, 1);
    } else {
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "-- Reading CARE set from %s.\n", opt->bdd.rPlus));
      r = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
        DDDMP_MODE_DEFAULT, opt->bdd.rPlus, NULL);
    }
#if 1
    Ddi_BddPartInsertLast(fwdReached, r);
#else
    care = Ddi_BddDup(r);
#endif
    Ddi_Free(r);
    Ddi_Free(from);
  } else if (opt->bdd.apprMBM) {
    Ddi_Bdd_t *r;
    Trav_Mgr_t *travMgrMBM = Trav_MgrInit(NULL, ddiMgr);

    Trav_MgrSetTravOpt(travMgrMBM, opt);
    Trav_MgrSetFromSelect(travMgrMBM, Trav_FromSelectRestrict_c);

    Ddi_BddAndAcc(from, invar);

    Trav_MgrSetI(travMgrMBM, Tr_MgrReadI(trMgr));
    Trav_MgrSetPS(travMgrMBM, Tr_MgrReadPS(trMgr));
    Trav_MgrSetNS(travMgrMBM, Tr_MgrReadNS(trMgr));
    Trav_MgrSetFrom(travMgrMBM, from);
    Trav_MgrSetReached(travMgrMBM, from);
    Trav_MgrSetTr(travMgrMBM, fwdTr);
#if 0
    if (opt->verbosity >= Pdtutil_VerbLevelNone_c &&
      opt->verbosity <= Pdtutil_VerbLevelSame_c) {
      Trav_MgrSetVerbosity(travMgrMBM, opt->verbosity);
    }
#endif
    if (opt->bdd.apprMBM > 1) {
      r = Trav_ApproxLMBM(travMgrMBM, delta);
    } else {
      r = Trav_ApproxMBM(travMgrMBM, delta);
    }

    Ddi_BddPartInsertLast(fwdReached, r);
    Ddi_Free(from);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "-- TRAV Manager Free.\n"));

    Trav_MgrQuit(travMgrMBM);
  } else {
    Ddi_BddAndAcc(from, invar);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "Approximate Forward.\n"));

    if (opt->bdd.constrain) {
      Tr_MgrSetImgMethod(Tr_TrMgr(fwdTr), Tr_ImgMethodCofactor_c);
    }

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "|TR|=%d\n", Ddi_BddSize(Tr_TrBdd(fwdTr))));

    saveClipTh = ddiMgr->exist.clipThresh;
    ddiMgr->exist.clipThresh = 100000000;

    doApproxRange = (opt->bdd.maxFwdIter == 0);
    for (i = 0, fixPoint = doApproxRange; (!fixPoint); i++) {

      if (i >= travMgr->settings.bdd.nExactIter) {
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
          fprintf(stdout,
            "-- Approximate Forward Verification; #Iteration=%d.\n", i + 1));

#if 0
        if (clock_tr != NULL) {
          Ddi_BddSetPartConj(from);
          Ddi_BddPartInsert(from, 0, clock_tr);
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "#"));
        }
#endif
        if (0 && useMeta) {
          int initSize = Ddi_BddSize(from);

          Ddi_BddSetMeta(from);
          if (i > 0) {
            Ddi_Bdd_t *r = Ddi_BddMakeMeta(Ddi_BddPartRead(fwdReached, i - 1));

            Ddi_BddDiffAcc(from, r);
            Ddi_Free(r);
          }

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "From: part:%d -> meta:%d.\n",
              initSize, Ddi_BddSize(from)));
        }
        to = Tr_ImgApprox(fwdTr, from, care);

        if (satRefine) {
          Ddi_Bdd_t *ref = Trav_ApproxSatRefine(to, from, trAig, prevRef,
            ps, ns, init, satRefine, 0);

          Ddi_Free(ref);
        }

        if (0 && useMeta) {
          Ddi_Bdd_t *r = Ddi_BddMakeMeta(Ddi_BddPartRead(fwdReached, i));

          Ddi_BddOrAcc(to, r);
          Ddi_Free(r);
          Ddi_BddSetPartConj(to);
        }
      } else {
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "-- Exact Forward Verification; #Iteration=%d.\n",
            i + 1));

        if (useMeta) {
          int initSize = Ddi_BddSize(from);

          Ddi_BddSetMeta(from);
          if (i > 0) {
            Ddi_Bdd_t *r = Ddi_BddMakeMeta(Ddi_BddPartRead(fwdReached, i - 1));

            Ddi_BddDiffAcc(from, r);
            Ddi_Free(r);
          }
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "From: part:%d -> meta:%d.\n",
              initSize, Ddi_BddSize(from)));
        }
        to = Tr_Img(fwdTr, from);
        if (useMeta) {
          Ddi_Bdd_t *r = Ddi_BddMakeMeta(Ddi_BddPartRead(fwdReached, i));

          Ddi_BddOrAcc(to, r);
          Ddi_Free(r);
          Ddi_BddSetPartConj(to);
        }
      }

      /*      Ddi_BddAndAcc(from,invarspec); */

      if (opt->bdd.apprNP > 0) {
        to =
          Ddi_BddEvalFree(Tr_TrProjectOverPartitions(to, NULL, NULL,
            opt->bdd.apprNP, opt->bdd.apprPreimgTh), to);
      }

      /*Ddi_BddSetMono(to); */

      if (opt->bdd.fromSelect != Trav_FromSelectNew_c) {
        tmp = Ddi_BddAnd(init, to);
        Ddi_BddSetMono(tmp);
        if (!Ddi_BddIsZero(tmp)) {
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "Init States is Reachable from INIT.\n"));
        } else {
          if (!travMgr->settings.bdd.strictBound) {
            if (1 || (clock_var_ns == NULL) || (i % 2 == 0)) {
              orProjectedSetAcc(to, init, clock_var);
            }
          }
        }
        Ddi_Free(tmp);
      }

      Ddi_Free(from);

      from = Ddi_BddDup(to);
      if (Ddi_BddIsMeta(to)) {
        Ddi_BddSetPartConj(to);
      }

      if (opt->bdd.fromSelect == Trav_FromSelectNew_c) {
        Ddi_Bdd_t *r;

        r = Ddi_BddDup(Ddi_BddPartRead(fwdReached, i));
        if (0) {
          diffProjectedSetAcc(from, r, clock_var);
          orProjectedSetAcc(r, to, clock_var);
          Ddi_Free(to);
          to = r;
        } else {
          orProjectedSetAcc(to, r, clock_var);
          Ddi_Free(r);
        }
      }

      if (0 && satRefine && opt->bdd.maxFwdIter < 0) {
        Ddi_Bdd_t *prev = Ddi_BddPartRead(fwdReached,
          Ddi_BddPartNum(fwdReached) - 1);

        if (Ddi_BddIsPartConj(prev)) {
          Ddi_Bdd_t *satPart = Ddi_BddPartRead(to, Ddi_BddPartNum(to) - 1);
          Ddi_Bdd_t *satPart0 =
            Ddi_BddPartRead(prev, Ddi_BddPartNum(prev) - 1);
          Ddi_BddOrAcc(satPart, satPart0);
        }
      }
      Ddi_BddPartInsertLast(fwdReached, to);

      if ((opt->bdd.maxFwdIter >= 0) && (i >= opt->bdd.maxFwdIter - 1)) {
        doApproxRange = 1;
        fixPoint = 1;
      } else if (satRefine) {
        int k;

        for (k = i; k >= 0 && k >= i - 2; k--) {
          fixPoint = Ddi_BddIncluded(to, Ddi_BddPartRead(fwdReached, k));
          if (fixPoint) {
            break;
          }
        }
      } else {
        int k;

        for (k = i; k >= 0; k--) {
          fixPoint = Ddi_BddEqual(to, Ddi_BddPartRead(fwdReached, k));
          if (fixPoint) {
            break;
          }
        }
      }

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "|To|=%d; ", Ddi_BddSize(to));
        fprintf(stdout, "|Reached|=%d", Ddi_BddSize(fwdReached));
        if (Ddi_BddIsMono(to)) {
        fprintf(stdout, "; #ToStates=%g",
            Ddi_BddCountMinterm(to, Ddi_VararrayNum(ps)));}
        fprintf(stdout, ".\n");) ;
      Ddi_Free(to);
    }

    ddiMgr->exist.clipThresh = saveClipTh;

    Ddi_Free(from);

    if (doApproxRange) {
      int k;

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "Forward Verisification; Iteration Limit Reached.\n");
        fprintf(stdout, "Approximate Range used for Outer Rplus.\n"));

      from = Ddi_BddMakeConst(ddiMgr, 1);
      for (k = 0; k < travMgr->settings.bdd.gfpApproxRange; k++) {
        to = Tr_ImgApprox(fwdTr, from, NULL);
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "|GFP range[%d]|=%d.\n", k, Ddi_BddSize(to)));
        Ddi_Free(from);
        from = to;
      }
      orProjectedSetAcc(to, init, NULL);
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "|Rplus(range)|=%d\n", Ddi_BddSize(to)));
      Ddi_BddPartInsertLast(fwdReached, to);
      Ddi_Free(to);
    }

  }

  Ddi_Free(clock_tr);

  if (satRefineInductive) {
    int nv = Ddi_VararrayNum(Tr_MgrReadPS(trMgr));
    double cnt0, cnt1;
    Ddi_Bdd_t *ref;
    Ddi_Bdd_t *r = Ddi_BddPartRead(fwdReached, Ddi_BddPartNum(fwdReached) - 1);

    cnt0 = Ddi_BddCountMinterm(r, nv);
    ref =
      Trav_ApproxSatRefine(r, r, trAig, NULL, ps, ns, init, satRefineInductive,
      1);
    cnt1 = Ddi_BddCountMinterm(r, nv);
    Pdtutil_VerbosityMgrIf(trMgr, Pdtutil_VerbLevelUsrMax_c) {
      fprintf(stdout, "#R states: %g -> %g (sat tightening: %g)\n",
        cnt0, cnt1, cnt0 / cnt1);
    }
    Ddi_Free(ref);
  }

  if (opt->bdd.wR != NULL) {
    Ddi_Bdd_t *r =
      Ddi_BddDup(Ddi_BddPartRead(fwdReached, Ddi_BddPartNum(fwdReached) - 1));
    Ddi_SetName(r, "rPlus");
    if (Ddi_BddIsPartConj(r)) {
      int i;

      if (care != NULL) {
        Ddi_BddPartInsertLast(r, care);
      }
      for (i = 0; i < Ddi_BddPartNum(r); i++) {
        char name[100];

        sprintf(name, "rPlus_part_%d", i);
        Ddi_SetName(Ddi_BddPartRead(r, i), name);
      }
    } else if (care != NULL) {
      Ddi_BddAndAcc(r, care);
    }

    if (strstr(opt->bdd.wR, ".blif") != NULL) {
      //Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      //  Pdtutil_VerbLevelNone_c,
      //  fprintf(stdout, "Writing outer reached ring to %s (BLIF file).\n",
      //    opt->bdd.wR));
      Ddi_BddStoreBlif(r, NULL, NULL, "reachedPlus", opt->bdd.wR, NULL);
    } else if (strstr(opt->bdd.wR, ".aig") != NULL) {
      int k;
      Ddi_Bddarray_t *rings = Ddi_BddarrayMakeFromBddPart(fwdReached);

      if (satRefineInductive) {
        Ddi_Free(rings);
        rings = Ddi_BddarrayAlloc(ddiMgr, 1);
        Ddi_BddarrayWrite(rings, 0, Ddi_BddPartRead(fwdReached,
            Ddi_BddPartNum(fwdReached) - 1));
      }

      Ddi_MgrReduceHeap(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"), 0);
      for (k = 0; k < Ddi_BddarrayNum(rings); k++) {
        Ddi_Bdd_t *rA = Ddi_BddarrayRead(rings, k);

        Ddi_BddSetAig(rA);
      }
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Writing reached rings to %s (AIGER file).\n",
          opt->bdd.wR));
      Ddi_AigarrayNetStoreAiger(rings, 0, opt->bdd.wR);
      Ddi_Free(rings);
    } else {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Writing outer reached ring to %s\n", opt->bdd.wR));
      Ddi_BddStore(r, "reachedPlus", DDDMP_MODE_TEXT, opt->bdd.wR, NULL);
    }

    if (0 && opt->bdd.wP != NULL) {
      Tr_TrWritePartitions(fwdTr, opt->bdd.wP);
    }

    if (opt->bdd.countReached) {
      int nv = Ddi_VararrayNum(Tr_MgrReadPS(trMgr));
      Ddi_Var_t *cvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

      Ddi_Bdd_t *myR = Ddi_BddMakeMono(r);

      if (0 && cvarPs != NULL) {
        Ddi_BddCofactorAcc(myR, cvarPs, 1);
        nv--;
      }

      if (0 || (Ddi_BddSize(r) < 100)) {
        Ddi_BddSetMono(r);
      }
      // nxr: indent fa casino se si lascia la macro
      //Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
      if (ddiMgr->settings.verbosity >= Pdtutil_VerbLevelUsrMax_c) {
        fprintf(stdout, "|Reached(mono)|=%d; NV: %d\n", Ddi_BddSize(r), nv);
        if (nv < 10000) {
          double ns = Ddi_BddCountMinterm(myR, nv);
          double space = pow(2, nv);

          fprintf(stdout, "#ReachedStates=%g. Space=%g\n", ns, space);
          fprintf(stdout, "space density=%g.\n", ns / space);
#if 0
          fprintf(stdout, "#R States(estimated)=%g\n",
            Ddi_AigEstimateMintermCount(myR, Ddi_VararrayNum(ps)));
#endif
        } else {
          double res = Ddi_BddCountMinterm(myR, nv - 1000);
          double fact = pow((pow(2, 100) / pow(10, 30)), 10);

          res = res * fact;
          fprintf(stdout, "#Reached states = %g * 10^300\n", res);
        }
      }
      //);
      Ddi_Free(myR);
    }

    Ddi_Free(r);

    exit(1);
  } else if (opt->bdd.countReached) {
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c, {
        Ddi_Bdd_t * ra = Ddi_BddMakeAig(Ddi_BddPartRead(fwdReached,
            Ddi_BddPartNum(fwdReached) - 1));
        fprintf(stdout, "#R States(estimated)=%g\n",
          Ddi_AigEstimateMintermCount(ra, Ddi_VararrayNum(ps)));
        Ddi_Bdd_t * r = Ddi_BddMakeMono(Ddi_BddPartRead(fwdReached,
            Ddi_BddPartNum(fwdReached) - 1));
        fprintf(stdout, "|Reached(mono)|=%d; ", Ddi_BddSize(r));
        fprintf(stdout, "#ReachedStates=%g.\n",
          Ddi_BddCountMinterm(r, Ddi_VararrayNum(Tr_MgrReadPS(trMgr))));
        Ddi_Free(ra);
        Ddi_Free(r);
      }
    );
  }

  /* Ddi_BddSetMono(fwdReached); */

  /* exact backward */

#if 1
  if (useMeta && travMgr->settings.bdd.metaMethod == Ddi_Meta_EarlySched) {
    Ddi_MetaInit(Ddi_ReadMgr(init), Ddi_Meta_EarlySched, Tr_TrBdd(tr), NULL,
      Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr), useMeta);
    Ddi_MgrSetMetaMaxSmooth(ddiMgr, travMgr->settings.bdd.metaMaxSmooth);
    /*Ddi_BddSetMeta(fwdReached); */
  }
#endif

  /* clipLevel = opt->ints.saveClipLevel; */
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "-- Exact Backward; Using Fix Point R+.\n"));

#if 1
  opt->ints.auxFwdTr = fwdTr;
  Tr_MgrSetImgApproxMaxIter(trMgr, 1);
#else
  Tr_TrFree(fwdTr);
#endif

  if (opt->tr.bwdTrClustFile != NULL) {
    Tr_Tr_t *newTr = Tr_TrManualPartitions(bckTr, opt->tr.bwdTrClustFile);

    Tr_TrFree(bckTr);
    bckTr = newTr;
  } else if (travMgr->settings.tr.sortForBck != 0) {
    Tr_TrSortForBck(bckTr);
    Tr_MgrSetClustThreshold(trMgr, clustTh);
    Tr_TrSetClustered(bckTr);
  } else {
    Tr_TrFree(bckTr);
    bckTr = Tr_TrDup(fwdTr);
  }

  verifOK = 1;

  Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodIwls95_c);
#if 0
  Tr_MgrSetClustThreshold(trMgr, 100000);
#endif

#if 0
#if 1
  Ddi_BddConstrainAcc(Tr_TrBdd(bckTr), fwdReached);
#endif
#endif

#if 0
  Tr_TrSortIwls95(bckTr);
  Tr_TrSetClustered(bckTr);
#endif

  notInit = Ddi_BddNot(init);

  from = Ddi_BddMakeConst(ddiMgr, 0);
  for (i = Ddi_BddPartNum(fwdReached) - 1; i >= 0; i--) {
    Ddi_Free(from);
    from = Ddi_BddAnd(Ddi_BddPartRead(fwdReached, i), badStates);
    if (!Ddi_BddIsZero(from))
      break;
  }
  Ddi_Free(from);
  from = Ddi_BddDup(badStates);

  Ddi_BddRestrictAcc(Tr_TrBdd(bckTr),
    Ddi_BddPartRead(fwdReached, Ddi_BddPartNum(fwdReached) - 1));

  if (travClk != NULL) {
    Tr_Tr_t *newTr = Tr_TrPartition(bckTr, travClk, 1);

    if (newTr != NULL) {
      Ddi_Bdd_t *ck_p0, *ck_p1;

      Tr_TrFree(bckTr);
      bckTr = newTr;
      ck_p0 = Ddi_BddPartExtract(Ddi_BddPartRead(Tr_TrBdd(bckTr), 0), 0);
      ck_p1 = Ddi_BddPartExtract(Ddi_BddPartRead(Tr_TrBdd(bckTr), 1), 0);
      Ddi_BddSetClustered(Tr_TrBdd(bckTr), clustTh);
      Ddi_BddSetPartConj(Ddi_BddPartRead(Tr_TrBdd(bckTr), 0));
      Ddi_BddSetPartConj(Ddi_BddPartRead(Tr_TrBdd(bckTr), 1));
      Ddi_BddPartInsert(Ddi_BddPartRead(Tr_TrBdd(bckTr), 0), 0, ck_p0);
      Ddi_BddPartInsert(Ddi_BddPartRead(Tr_TrBdd(bckTr), 1), 0, ck_p1);
      Ddi_Free(ck_p0);
      Ddi_Free(ck_p1);
    } else {
      Ddi_BddSetClustered(Tr_TrBdd(bckTr), clustTh);
    }

    {
      Ddi_Bdd_t *p0, *p1, *f, *fpart;

      if (opt->tr.trDpartTh > 0) {
        p0 = Ddi_BddPartRead(Tr_TrBdd(bckTr), 0);
        p1 = Ddi_BddPartRead(Tr_TrBdd(bckTr), 1);
        f = Ddi_BddPartRead(p0, 1);
        fpart = Part_PartitionDisjSet(f, NULL, NULL,
          /*Part_MethodCofactor_c, */ Part_MethodCofactorSupport_c,
          opt->tr.trDpartTh, 3, Pdtutil_VerbLevelDevMax_c);
        Ddi_BddPartWrite(p0, 1, fpart);
        Ddi_Free(fpart);
        f = Ddi_BddPartRead(p1, 1);
        fpart = Part_PartitionDisjSet(f, NULL, NULL,
          /*Part_MethodCofactor_c, */ Part_MethodCofactorSupport_c,
          2 * Ddi_BddSize(f) / 3, 3, Pdtutil_VerbLevelDevMax_c);
        Ddi_BddPartWrite(p1, 1, fpart);
        Ddi_Free(fpart);
      }

    }

  } else {
    Ddi_BddSetClustered(Tr_TrBdd(bckTr), clustTh);

    if (opt->tr.trDpartTh > 0) {
      Ddi_Bdd_t *newTrBdd, *f;

      f = Tr_TrBdd(bckTr);
      newTrBdd = Part_PartitionDisjSet(f, NULL, NULL, Part_MethodCofactor_c,
        opt->tr.trDpartTh, 3, Pdtutil_VerbLevelDevMax_c);
      TrTrRelWrite(bckTr, newTrBdd);
      Ddi_Free(newTrBdd);
    }
  }

  if (opt->bdd.noCheck) {
    Ddi_BddAndAcc(init, Ddi_MgrReadZero(ddiMgr));
  }


  if (opt->bdd.wP != NULL) {
    char name[200];

    sprintf(name, "fwd-appr-%s", opt->bdd.wP);
    Tr_TrWritePartitions(fwdTr, name);
    sprintf(name, "%s", opt->bdd.wP);
    Tr_TrWritePartitions(bckTr, name);
  }
  if (opt->bdd.wOrd != NULL) {
    char name[200];

    sprintf(name, "%s.0", opt->bdd.wOrd);
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "Writing intermediate variable order to file %s\n",
        name));
    Ddi_MgrOrdWrite(ddiMgr, name, NULL, Pdtutil_VariableOrderDefault_c);
  }

  if (Ddi_BddIsAig(init)) {
    if (invar != NULL && !Ddi_BddIsOne(invar)) {
      Ddi_Bdd_t *i1 = Ddi_BddMakeAig(invar);

      Ddi_BddAndAcc(init, i1);
      Ddi_Free(i1);
    }
  }

  do {
    opt->ints.univQuantifyDone = 0;
    if (!opt->bdd.prioritizedMc) {
      bckReached = Ddi_BddMakePartDisjVoid(ddiMgr);

      /*Tr_TrReverseAcc(bckTr); */
      verifOK = !travOverRingsWithFixPoint(from, NULL, init, bckTr, fwdReached,
        bckReached, NULL, Ddi_BddPartNum(fwdReached),
        "Exact Backward Verification", initialTime, 1 /*reverse */ ,
        0 /*approx */ , opt);
    } else {
      bckReached = bckInvarCheckRecur(from, init, bckTr, fwdReached, opt);
      verifOK = (bckReached == NULL);
    }

    if (1 && opt->ints.univQuantifyDone) {
      int nq;

      Ddi_Free(from);
      from = bckReached;
      nq = Ddi_VarsetNum(opt->ints.univQuantifyVars);
      if (nq < 10) {
        Ddi_Free(opt->ints.univQuantifyVars);
        opt->bdd.univQuantifyTh = -1;
      } else {
        for (i = 0; i < nq / 2; i++) {
          Ddi_Var_t *v = Ddi_VarsetTop(opt->ints.univQuantifyVars);

          Ddi_VarsetRemoveAcc(opt->ints.univQuantifyVars, v);
        }
      }
    }

  } while (1 && opt->ints.univQuantifyDone);

  frontier = NULL;

  if (opt->bdd.wBR != NULL) {
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "Writing backward reached to %s\n", opt->bdd.wBR));
    Ddi_BddStore(bckReached, "backReached", DDDMP_MODE_TEXT, opt->bdd.wBR,
      NULL);
  }

  Tr_TrFree(opt->ints.auxFwdTr);

  if (!verifOK) {
    fwdRings = 0;

    Ddi_Bdd_t *overApprox = Ddi_BddDup(fwdReached);

    Ddi_BddPartWrite(overApprox, 0, init);
    /* opt->trav.constrain_level = 0; */
    verifOK = !travOverRings(init, NULL, badStates, tr, bckReached,
      fwdReached, overApprox, Ddi_BddPartNum(bckReached),
      "Exact fwd iteration", initialTime, 0 /*reverse */ , 0 /*approx */ ,
      opt);

    Ddi_Free(overApprox);
    frontier = Ddi_BddarrayMakeFromBddPart(fwdReached);
    fwdRings = 1;
  }

  Ddi_Free(trAig);
  Ddi_Free(prevRef);
  Tr_TrFree(bckTr);
  Ddi_Free(bckReached);
  Ddi_Free(from);
  Ddi_Free(invar);
  Ddi_Free(notInit);
  Ddi_Free(fwdReached);
  //  Ddi_Free(care);

  if (verifOK) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: PASS.\n"));
  } else {
    FILE *fp;
    Trav_Mgr_t *travMgrFail;
    Ddi_Bddarray_t *patternMisMat;
    Ddi_Varset_t *piv;
    Ddi_Bdd_t *invspec_bar, *end;

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: FAIL.\n"));

    Tr_MgrSetClustSmoothPi(trMgr, 0);
    travMgrFail = Trav_MgrInit(NULL, ddiMgr);
    travMgrFail->settings.bdd.mismatchOptLevel = opt->bdd.mismatchOptLevel;

#if 0
    ddiMgr->exist.clipThresh = 100000000;
#else
    ddiMgr->exist.clipLevel = 0;
#endif

#if 1
    piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));
#if 0
    /* keep PIs for counterexample */
    invspec_bar = Ddi_BddExist(invarspec, piv);
#else
    invspec_bar = Ddi_BddNot(invarspec);
#endif

#if 0
    Tr_MgrSetImgMethod(Tr_TrMgr(tr), Tr_ImgMethodCofactor_c);
#endif

    if (0 && Ddi_MgrReadExistOptLevel(ddiMgr) > 2) {
      Ddi_MgrSetExistOptLevel(ddiMgr, 2);
    }

    if (fwdRings) {
#if 0
      if (travClk != 0) {
        Tr_Tr_t *newTr = Tr_TrPartitionWithClock(tr, clock_var,
          travMgr->settings.bdd.implications);

        if (newTr != NULL) {
          Tr_TrFree(tr);
          tr = newTr;
        }
      }
#endif

      Tr_TrReverseAcc(tr);
      patternMisMat = Trav_MismatchPat(travMgrFail, tr, invspec_bar,
        NULL, NULL, &end, frontier,
        Tr_MgrReadPS(Tr_TrMgr(tr)), Tr_MgrReadNS(Tr_TrMgr(tr)), piv);
      Tr_TrReverseAcc(tr);
    } else {
      patternMisMat = Trav_MismatchPat(travMgrFail, tr, NULL,
        invspec_bar, &end, NULL, frontier,
        Tr_MgrReadPS(Tr_TrMgr(tr)), Tr_MgrReadNS(Tr_TrMgr(tr)), piv);
    }

    /*---------------------------- Write Patterns --------------------------*/

    if (patternMisMat != NULL) {
      if (opt->doCex == 1) {
        travBddCexToFsm(fsmMgr, patternMisMat, end, 1 /* reverse */ );
      } else {
        char filename[100];

        sprintf(filename, "%s%d.txt", FILEPAT, getpid());
        fp = fopen(filename, "w");
        //fp = fopen ("pdtpat.txt", "w");
        if (Ddi_BddarrayPrintSupportAndCubes(patternMisMat, 1, -1,
            0, 0, NULL, fp) == 0) {
          Pdtutil_Warning(1, "Store cube Failed.");
        }
        fclose(fp);
      }
    }

    /*----------------------- Write Initial State Set ----------------------*/

    if (end != NULL && !Ddi_BddIsZero(end)) {
      char filename[100];

      sprintf(filename, "%s%d.txt", FILEINIT, getpid());
      fp = fopen(filename, "w");
      //fp = fopen ("pdtinit.txt", "w");
      if (Ddi_BddPrintSupportAndCubes(end, 1, -1, 0, NULL, fp) == 0) {
        Pdtutil_Warning(1, "Store cube Failed.");
      }
      fclose(fp);
      Ddi_Free(end);
    }

    Trav_MgrQuit(travMgrFail);
    Ddi_Free(piv);
    Ddi_Free(end);
    Ddi_Free(invspec_bar);
    Ddi_Free(patternMisMat);

#endif

  }

  Ddi_Free(frontier);
  Ddi_Free(badStates);

  currTime = util_cpu_time();
  travMgr->travTime = currTime - initialTime;

  return (verifOK);
}

/**Function*******************************************************************
  Synopsis    [exact forward verification]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Trav_TravBddApproxBackwExactForwVerify(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec
)
{
  int i, j, backi, verifOK, fixPoint, approx;
  Ddi_Bdd_t *tmp, *from, *bckReached, *to, *care, *invar, *ring, *badStates;
  Ddi_Vararray_t *ps;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(init);
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Tr_Tr_t *bckTr, *fwdTr0, *fwdTr;
  long initialTime, currTime;
  Ddi_Bddarray_t *frontier;
  Trav_Settings_t *opt = &travMgr->settings;
  int clustTh = opt->tr.clustThreshold;

  ps = Tr_MgrReadPS(trMgr);

  badStates = Ddi_BddNot(invarspec);

  initialTime = util_cpu_time();

  {
    Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));

    Ddi_BddExistAcc(badStates, piv);
    Ddi_Free(piv);
  }

#if ENABLE_INVAR
  invar = Ddi_BddPartExtract(Tr_TrBdd(tr), 0);
#else
  invar = Ddi_BddMakeConst(ddiMgr, 1);
#endif

  Pdtutil_VerbosityMgrIf(trMgr, Pdtutil_VerbLevelUsrMed_c) {
    fprintf(stdout, "|Invar|=%d; ", Ddi_BddSize(invar));
    fprintf(stdout, "|Tr|=%d.\n", Ddi_BddSize(Tr_TrBdd(tr)));
  }

  fwdTr = Tr_TrDup(tr);

  Tr_MgrSetClustThreshold(trMgr, 1);
  Tr_TrSetClustered(tr);
  Tr_TrSortIwls95(tr);
  Tr_MgrSetClustThreshold(trMgr, clustTh);
  Tr_TrSetClustered(tr);

  Tr_MgrSetClustSmoothPi(trMgr, 1);
  Tr_MgrSetClustThreshold(trMgr, 1);
  Tr_TrSetClustered(fwdTr);
  Tr_TrSortIwls95(fwdTr);
  Tr_MgrSetClustThreshold(trMgr, clustTh);
  Tr_TrSetClustered(fwdTr);

  bckTr = Tr_TrDup(fwdTr);
  /*Tr_TrReverseAcc(bckTr); */

#if 1
  for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(bckTr));) {
    Ddi_Bdd_t *group, *p;

    group = Ddi_BddPartRead(Tr_TrBdd(bckTr), j);
    Ddi_BddSetPartConj(group);
    p = Ddi_BddPartRead(Tr_TrBdd(bckTr), i);
    Ddi_BddPartInsertLast(group, p);
    if (Ddi_BddSize(group) > opt->bdd.apprGrTh) {
      /* resume from group insert */
      p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
      Ddi_Free(p);
      i++;
      j++;
    } else {
      p = Ddi_BddPartExtract(Tr_TrBdd(bckTr), i);
      Ddi_Free(p);
    }
  }
#endif

  /* approx forward */

  from = Ddi_BddDup(badStates);
  Ddi_BddAndAcc(from, invar);

  /* disjunctively partitioned reached */
  bckReached = Ddi_BddMakePartDisjVoid(ddiMgr);
  Ddi_BddPartInsert(bckReached, 0, from);

  Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodApprox_c);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "-- Backward Approximate.\n"));

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
    fprintf(stdout, "|TR|=%d\n", Ddi_BddSize(Tr_TrBdd(bckTr))));

  approx = 1;
  for (i = 0, fixPoint = 0;
    (!fixPoint) && ((opt->bdd.bound < 0) || (i < opt->bdd.bound)); i++) {

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "-- Approximate Backward Verification Iteration: %d.\n",
        i + 1));

    to = Trav_ApproxPreimg(bckTr, from, Ddi_MgrReadOne(ddiMgr), NULL, 1, opt);
    Ddi_BddSetMono(to);

#if 0
    if (Ddi_BddIsOne(to)) {
      Ddi_Free(to);
      Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodIwls95_c);
      to = Tr_Img(bckTr, from);
      Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodApprox_c);
    }
#endif

    Ddi_Free(from);
    Ddi_BddPartInsertLast(bckReached, to);

    from = Ddi_BddDup(to);
#if 0
    fixPoint = Ddi_BddIsZero(tmp);
#endif

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|To|=%d; ", Ddi_BddSize(to));
      fprintf(stdout, "|Reached|=%d.\n", Ddi_BddSize(bckReached))
      );

#if 1
    if (Ddi_BddIsMono(to)) {
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "#To States = %g.\n",
          Ddi_BddCountMinterm(to, Ddi_VararrayNum(ps))));
    }
#endif
    Ddi_Free(to);
  }

  Ddi_Free(from);

  Tr_TrFree(bckTr);

  /* Ddi_BddSetMono(fwdReached); */

  /* exact backward */

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "-- Exact Backward.\n"));

  verifOK = 1;

  Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodIwls95_c);

  from = Ddi_BddDup(init);

  fwdTr0 = Tr_TrDup(fwdTr);

  frontier = Ddi_BddarrayAlloc(ddiMgr, 1);
  Ddi_BddarrayWrite(frontier, 0, from);

  for (backi = 0; /*!Ddi_BddIsZero(from) */ i >= 0; i--, backi++) {

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "-- Exact Forward Verification; #Iteration=%d.\n",
        i + 1));

    Ddi_BddConstrainAcc(from, Ddi_BddPartRead(bckReached, i));

    Ddi_BddSetMono(from);

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|From|=%d\n", Ddi_BddSize(from)));

    Ddi_BddarrayWrite(frontier, backi, from);
    ring = Ddi_BddarrayRead(frontier, backi);

    if (Ddi_BddIsZero(from))
      continue;

    tmp = Ddi_BddAnd(Ddi_BddPartRead(bckReached, i), badStates);
    Ddi_BddSetMono(tmp);
    if (!Ddi_BddIsZero(tmp)) {
      Ddi_BddAndAcc(tmp, from);
      Ddi_BddSetMono(tmp);
      if (!Ddi_BddIsZero(tmp)) {
        verifOK = 0;
        Ddi_Free(tmp);
        Ddi_BddAndAcc(ring, badStates);
        break;
      }
    }
    Ddi_Free(tmp);

    if (i == 0)
      continue;

    Pdtutil_Assert(i > 0, "Iteration 0 reached");

    care = Ddi_BddPartRead(bckReached, i - 1);

    if (opt->bdd.constrainLevel > 1) {
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      Ddi_BddConstrainAcc(Tr_TrBdd(fwdTr), care);
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
    }

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|TR|=%d\n", Ddi_BddSize(Tr_TrBdd(fwdTr))));

    to = Tr_ImgWithConstrain(fwdTr, from, NULL);

#if FULL_COFACTOR
    Tr_TrFree(fwdTr);
    fwdTr = Tr_TrDup(fwdTr0);
#endif

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelDevMin_c,
      LOG_CONST(to, "to")
      );

    Ddi_BddConstrainAcc(to, care);

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|To|=%d\n", Ddi_BddSize(to)));

    Ddi_Free(from);

    from = Ddi_BddDup(to);

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|To|=%d ", Ddi_BddSize(to));
      currTime = util_cpu_time();
      fprintf(stdout, "Live nodes (peak): %u(%u) - Time: %s.\n",
        Ddi_MgrReadLiveNodes(ddiMgr),
        Ddi_MgrReadPeakLiveNodeCount(ddiMgr),
        util_print_time(currTime - initialTime));
      );

    Ddi_Free(to);
  }

  Tr_TrFree(fwdTr);
  Tr_TrFree(fwdTr0);
  Ddi_Free(from);
  Ddi_Free(invar);
  Ddi_Free(bckReached);

  if (verifOK) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: PASS.\n"));
  } else {
#if 0
    FILE *fp;
    Trav_Mgr_t *travMgrFail;
    Ddi_Bddarray_t *patternMisMat;
    Ddi_Varset_t *piv;
    Ddi_Bdd_t *invspec_bar, *end;
#endif

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: FAIL.\n"));

#if 0

    Tr_MgrSetClustSmoothPi(trMgr, 0);
    travMgrFail = Trav_MgrInit(NULL, ddiMgr);
    travMgrFail->settings.bdd.mismatchOptLevel = opt->bdd.mismatchOptLevel;

#if 1
    piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));
#if 0
    /* keep PIs for counterexample */
    invspec_bar = Ddi_BddExist(invarspec, piv);
#else
    invspec_bar = Ddi_BddNot(invarspec);
#endif

#if 0
    Tr_MgrSetImgMethod(Tr_TrMgr(tr), Tr_ImgMethodCofactor_c);
#endif
    patternMisMat = Trav_MismatchPat(travMgrFail, tr, NULL,
      invspec_bar, &end, NULL, frontier,
      Tr_MgrReadPS(Tr_TrMgr(tr)), Tr_MgrReadNS(Tr_TrMgr(tr)), piv);

    /*---------------------------- Write Patterns --------------------------*/

    if (patternMisMat != NULL) {
      char filename[100];

      sprintf(filename, "%s%d.txt", FILEPAT, getpid());
      fp = fopen(filename, "w");
      //fp = fopen ("pdtpat.txt", "w");
      if (Ddi_BddarrayPrintSupportAndCubes(patternMisMat, 1, -1,
          0, 0, NULL, fp) == 0) {
        Pdtutil_Warning(1, "Store cube Failed.");
      }
      fclose(fp);
    }

    /*----------------------- Write Initial State Set ----------------------*/

    if (end != NULL && !Ddi_BddIsZero(end)) {
      char filename[100];

      sprintf(filename, "%s%d.txt", FILEINIT, getpid());
      fp = fopen(filename, "w");
      //fp = fopen ("pdtinit.txt", "w");
      if (Ddi_BddPrintSupportAndCubes(end, 1, -1, 0, NULL, fp) == 0) {
        Pdtutil_Warning(1, "Store cube Failed.");
      }
      fclose(fp);
      Ddi_Free(end);
    }

    Trav_MgrQuit(travMgrFail);
    Ddi_Free(piv);
    Ddi_Free(invspec_bar);
    Ddi_Free(patternMisMat);

#endif
#endif

  }

  Ddi_Free(frontier);
  Ddi_Free(badStates);

  currTime = util_cpu_time();
  travMgr->travTime = currTime - initialTime;

  return (verifOK);
}

/**Function*******************************************************************
  Synopsis    [exact forward verification]
  Description []
  SideEffects []
  SeeAlso     [FbvApproxForwExactBackwVerify]
******************************************************************************/
int
Trav_TravBddApproxForwExactBackwVerify(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  int doFwd
)
{
  int i, j, backi, verifOK, fixPoint, initReachable;
  Ddi_Bdd_t *tmp, *from, *fwdReached, *bckReached, *to,
    *notInit, *care, *invar, *badi, *ring, *badStates;
  Ddi_Vararray_t *ps;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(init);
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Tr_Tr_t *bckTr, *bckTr0, *fwdTr;
  long initialTime, currTime;
  Ddi_Bddarray_t *frontier;
  Trav_Settings_t *opt = &travMgr->settings;
  int clustTh = opt->tr.clustThreshold;

  ps = Tr_MgrReadPS(trMgr);

  badStates = Ddi_BddNot(invarspec);

  initialTime = util_cpu_time();

  {
    Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));

    Ddi_BddExistAcc(badStates, piv);
    Ddi_Free(piv);
  }

#if ENABLE_INVAR
  invar = Ddi_BddPartExtract(Tr_TrBdd(tr), 0);
#else
  invar = Ddi_BddMakeConst(ddiMgr, 1);
#endif

  Pdtutil_VerbosityMgrIf(ddiMgr, Pdtutil_VerbLevelUsrMed_c) {
    fprintf(stdout, "|Invar|=%d; ", Ddi_BddSize(invar));
    fprintf(stdout, "|Tr|=%d.\n", Ddi_BddSize(Tr_TrBdd(tr)));
  }

  fwdTr = Tr_TrDup(tr);
  bckTr = Tr_TrDup(tr);

  Tr_MgrSetClustThreshold(trMgr, 1);
  Tr_TrSetClustered(tr);
  Tr_TrSortIwls95(tr);
  Tr_MgrSetClustThreshold(trMgr, clustTh);
  Tr_TrSetClustered(tr);

  Tr_MgrSetClustSmoothPi(trMgr, 1);
  Tr_MgrSetClustThreshold(trMgr, 1);
  Tr_TrSetClustered(fwdTr);
  Tr_TrSortIwls95(fwdTr);
  Tr_MgrSetClustThreshold(trMgr, clustTh);
  Tr_TrSetClustered(fwdTr);

  if (opt->tr.sortForBck != 0) {
    Tr_TrSortForBck(bckTr);
    Tr_TrSetClustered(bckTr);
  } else {
    Tr_TrFree(bckTr);
    bckTr = Tr_TrDup(fwdTr);
  }


#if 1
  for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(fwdTr));) {
    Ddi_Bdd_t *group, *p;

    group = Ddi_BddPartRead(Tr_TrBdd(fwdTr), j);
    Ddi_BddSetPartConj(group);
    p = Ddi_BddPartRead(Tr_TrBdd(fwdTr), i);
    Ddi_BddPartInsertLast(group, p);
    if (Ddi_BddSize(group) > opt->bdd.apprGrTh) {
      /* resume from group insert */
      p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
      Ddi_Free(p);
      i++;
      j++;
    } else {
      p = Ddi_BddPartExtract(Tr_TrBdd(fwdTr), i);
      Ddi_Free(p);
    }
  }
#endif

  if (doFwd) {

    /* approx forward */

    from = Ddi_BddDup(init);
    Ddi_BddAndAcc(from, invar);

    /* disjunctively partitioned reached */
    fwdReached = Ddi_BddMakePartDisjVoid(ddiMgr);
    Ddi_BddPartInsert(fwdReached, 0, from);


    Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodApprox_c);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "-- Approximate Forward.\n"));

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|TR|=%d\n", Ddi_BddSize(Tr_TrBdd(fwdTr))));

    initReachable = 0;

    for (i = 0, fixPoint = 0;
      (!fixPoint) && ((opt->bdd.bound < 0) || (i < opt->bdd.bound)); i++) {

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout,
          "-- Approximate Forward Verification; #Iteration=%d.\n", i + 1));

      to = Tr_Img(fwdTr, from);
      if (opt->bdd.apprNP > 0) {
        to =
          Ddi_BddEvalFree(Tr_TrProjectOverPartitions(to, NULL, NULL,
            opt->bdd.apprNP, opt->bdd.apprPreimgTh), to);
      }

      /*Ddi_BddSetMono(to); */
      /*if (i==0) */  {
        tmp = Ddi_BddAnd(init, to);
        Ddi_BddSetMono(tmp);
        if (!Ddi_BddIsZero(tmp)) {
          initReachable = i + 1;

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "Init States are Reachable from Init.\n"));
        } else {
          if (!opt->bdd.strictBound)
            orProjectedSetAcc(to, init, NULL);
        }
        Ddi_Free(tmp);
      }
      Ddi_Free(from);
      Ddi_BddPartInsertLast(fwdReached, to);

      from = Ddi_BddDup(to);
#if 0
      fixPoint = Ddi_BddIsZero(tmp);
#endif

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "|To|=%d; ", Ddi_BddSize(to));
        fprintf(stdout, "|Reached|=%d.\n", Ddi_BddSize(fwdReached)));

#if 1
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        if (Ddi_BddIsMono(to)) {
        fprintf(stdout, "#To States = %g\n",
            Ddi_BddCountMinterm(to, Ddi_VararrayNum(ps)));}
      ) ;
#endif
      Ddi_Free(to);
    }

    Ddi_Free(from);

  } else {
    fwdReached = Ddi_BddMakeConst(ddiMgr, 1);
    Ddi_BddAndAcc(fwdReached, invar);
  }

  Tr_TrFree(fwdTr);

  /* Ddi_BddSetMono(fwdReached); */

  /* exact backward */

#if 0
  if (opt->bdd.useMeta) {
    Ddi_MetaInit(Ddi_ReadMgr(init), opt->bdd.metaMethod, Tr_TrBdd(tr), NULL,
      Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr), 100);
  }
  if (opt->bdd.useMeta) {
    Ddi_BddSetMeta(fwdReached);
  }
#endif

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
    fprintf(stdout, "-- Exact Backward.\n"));

  verifOK = 1;
#if 0
  bckReached = Ddi_BddAnd(fwdReached, badStates);
#else
  bckReached = Ddi_BddDup(badStates);
#endif
  /*Ddi_BddAndAcc(bckReached,invar); */

  Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodIwls95_c);
#if 0
  Tr_MgrSetClustThreshold(trMgr, 100000);
#endif

#if 0
#if 1
  Ddi_BddConstrainAcc(Tr_TrBdd(bckTr), fwdReached);
  /*  Ddi_BddConstrainAcc(bckReached,fwdReached); */
#else
  Ddi_BddAndAcc(bckReached, fwdReached);
#endif
#endif

  Tr_TrReverseAcc(bckTr);

#if 0
  Tr_TrSortIwls95(bckTr);
  Tr_TrSetClustered(bckTr);
#endif

  notInit = Ddi_BddNot(init);

  from = Ddi_BddMakeConst(ddiMgr, 0);
  for (i = Ddi_BddPartNum(fwdReached) - 1; i >= 0; i--) {
    Ddi_Free(from);
    from = Ddi_BddAnd(Ddi_BddPartRead(fwdReached, i), badStates);
    if (!Ddi_BddIsZero(from))
      break;
  }
  Ddi_Free(from);
  from = Ddi_BddDup(badStates);

  bckTr0 = Tr_TrDup(bckTr);

  frontier = Ddi_BddarrayAlloc(ddiMgr, 1);
  Ddi_BddarrayWrite(frontier, 0, from);

  for (backi = 0; /*!Ddi_BddIsZero(from) */ i >= 0; i--, backi++) {

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "-- Exact Backward Verification; #Iteration=%d.\n",
        i + 1));

#if 0
    care = Ddi_BddNot(bckReached);
    Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
    Ddi_BddRestrictAcc(Tr_TrBdd(bckTr), care);
    Ddi_Free(care);
#endif

    if (opt->bdd.constrainLevel > 1)
      care = Ddi_BddPartRead(fwdReached, i);
    else
      care = Ddi_MgrReadOne(ddiMgr);

#if 0
    Ddi_BddConstrainAcc(Tr_TrBdd(bckTr), care);
    Ddi_BddConstrainAcc(from, care);
#else
    Ddi_BddRestrictAcc(from, Ddi_BddPartRead(fwdReached, i));
#endif

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|from0|=%d ", Ddi_BddSize(from));
      fprintf(stdout, "|from|=%d\n", Ddi_BddSize(from)));

    if (opt->bdd.groupBad) {
      badi = Ddi_BddDup(badStates);
      Ddi_BddConstrainAcc(badi, care);
      Ddi_BddSetMono(badi);
      if (!Ddi_BddIsZero(badi)) {
        Ddi_BddOrAcc(from, badi);
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
          fprintf(stdout, "|from+i|=%d\n", Ddi_BddSize(from)));
      }
      Ddi_Free(badi);
    }

    Ddi_BddarrayWrite(frontier, backi, care);
    ring = Ddi_BddarrayRead(frontier, backi);

#if PARTITIONED_RINGS
    Ddi_BddSetPartConj(ring);
    Ddi_BddPartInsertLast(ring, from);
#else
    Ddi_BddAndAcc(ring, from);
#endif


    if (Ddi_BddIsZero(from))
      continue;

    tmp = Ddi_BddAnd(Ddi_BddPartRead(fwdReached, i), init);
    Ddi_BddSetMono(tmp);
    if (!Ddi_BddIsZero(tmp)) {
      Ddi_BddAndAcc(tmp, from);
      Ddi_BddSetMono(tmp);
      if (!Ddi_BddIsZero(tmp)) {
        verifOK = 0;
        Ddi_Free(tmp);
#if PARTITIONED_RINGS
        Ddi_BddPartInsert(ring, 0, init);
#else
        Ddi_BddAndAcc(ring, init);
#endif
        break;
      }
    }
    Ddi_Free(tmp);

    if (i == 0)
      continue;

    Pdtutil_Assert(i > 0, "Iteration 0 reached");

    if (opt->bdd.constrainLevel > 1)
      from = Ddi_BddEvalFree(Ddi_BddAnd(care, from), from);

    care = Ddi_BddPartRead(fwdReached, i - 1);

    if (opt->bdd.constrainLevel > 1) {
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      Ddi_BddConstrainAcc(Tr_TrBdd(bckTr), care);
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
    }

    if (opt->bdd.useMeta) {
      Ddi_BddSetMono(from);
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "FROM: mono: %d -> ", Ddi_BddSize(from)));
      Ddi_BddSetMeta(from);
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "meta: %d\n", Ddi_BddSize(from)));
    }

    {
#if 0
      Ddi_Bdd_t *myCare = Ddi_BddNot(bckReached);

      Ddi_BddSetPartConj(myCare);
      Ddi_BddPartInsertLast(myCare, care);
#else
      Ddi_Bdd_t *myCare = Ddi_BddDup(care);
#endif
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "|TR|=%d\n", Ddi_BddSize(Tr_TrBdd(bckTr))));
      Ddi_BddSwapVarsAcc(myCare, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      to = Tr_ImgWithConstrain(bckTr, from, myCare);
      Ddi_BddSwapVarsAcc(myCare, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));

      if (opt->bdd.useMeta) {
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
          fprintf(stdout, "\nTO: meta: %d -> ", Ddi_BddSize(to)));
        Ddi_BddSetMono(to);
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
          fprintf(stdout, "mono: %d\n", Ddi_BddSize(to)));
      }
#if 1
      Tr_TrFree(bckTr);
      bckTr = Tr_TrDup(bckTr0);
#endif

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelDevMin_c, LOG_CONST(to, "to")
        );

      if (opt->bdd.constrainLevel <= 1) {
        Ddi_BddRestrictAcc(to, myCare);
      }

      Ddi_Free(myCare);
    }

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|To|=%d\n", Ddi_BddSize(to)));

    Ddi_Free(from);

#if 0
    care = Ddi_BddNot(bckReached);
    from = Ddi_BddConstrain(to, care);
    Ddi_Free(care);
#else

    if (opt->bdd.constrainLevel > 0) {
      from = Ddi_BddDup(to);
    } else {
      from = Ddi_BddAnd(care, to);
    }
#endif

#if 0
    if (Ddi_BddSize(from) > Ddi_BddSize(to)) {
#else
    if (0) {
#endif
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "|To|=%d\n", Ddi_BddSize(to)));

      Ddi_Free(from);
      from = Ddi_BddDup(to);
    } else {
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "|To*BRi|=%d\n", Ddi_BddSize(from)));
    }

#if 0
    Ddi_BddConstrainAcc(from, care);
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|To!BRi|=%d\n", Ddi_BddSize(from)));
#endif

#if 0
    Ddi_BddOrAcc(bckReached, to);

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|To|=%d; |Reached|=%d; ", Ddi_BddSize(to),
        Ddi_BddSize(bckReached));
      fprintf(stdout, "#ReachedStates=%g.\n",
        Ddi_BddCountMinterm(bckReached, Ddi_VararrayNum(ps)))
      );
#endif

    currTime = util_cpu_time();

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "Live Nodes (Peak): %u(%u) - Time: %s\n",
        Ddi_MgrReadLiveNodes(ddiMgr),
        Ddi_MgrReadPeakLiveNodeCount(ddiMgr),
        util_print_time(currTime - initialTime))
      );

    Ddi_Free(to);
  }

  Tr_TrFree(bckTr);
  Tr_TrFree(bckTr0);
  Ddi_Free(bckReached);
  Ddi_Free(from);
  Ddi_Free(invar);
  Ddi_Free(notInit);
  Ddi_Free(fwdReached);

  if (verifOK) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: PASS.\n"));
  } else {
    FILE *fp;
    Trav_Mgr_t *travMgrFail;
    Ddi_Bddarray_t *patternMisMat;
    Ddi_Varset_t *piv;
    Ddi_Bdd_t *invspec_bar, *end;

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: FAIL.\n"));

    Tr_MgrSetClustSmoothPi(trMgr, 0);
    travMgrFail = Trav_MgrInit(NULL, ddiMgr);
    travMgrFail->settings.bdd.mismatchOptLevel = opt->bdd.mismatchOptLevel;

#if 1
    piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));
#if 0
    /* keep PIs for counterexample */
    invspec_bar = Ddi_BddExist(invarspec, piv);
#else
    invspec_bar = Ddi_BddNot(invarspec);
#endif

#if 0
    Tr_MgrSetImgMethod(Tr_TrMgr(tr), Tr_ImgMethodCofactor_c);
#endif
    patternMisMat = Trav_MismatchPat(travMgrFail, tr, NULL,
      invspec_bar, &end, NULL, frontier,
      Tr_MgrReadPS(Tr_TrMgr(tr)), Tr_MgrReadNS(Tr_TrMgr(tr)), piv);

    /*---------------------------- Write Patterns --------------------------*/

    if (patternMisMat != NULL) {
      if (opt->doCex == 1) {
        travBddCexToFsm(fsmMgr, patternMisMat, end, 1 /* reverse */ );
      } else {
        char filename[100];

        sprintf(filename, "%s%d.txt", FILEPAT, getpid());
        fp = fopen(filename, "w");
        //fp = fopen ("pdtpat.txt", "w");
        if (Ddi_BddarrayPrintSupportAndCubes(patternMisMat, 1, -1,
            0, 0, NULL, fp) == 0) {
          Pdtutil_Warning(1, "Store cube Failed.");
        }
      }
      fclose(fp);
    }

    /*----------------------- Write Initial State Set ----------------------*/

    if (end != NULL && !Ddi_BddIsZero(end)) {
      char filename[100];

      sprintf(filename, "%s%d.txt", FILEINIT, getpid());
      fp = fopen(filename, "w");
      //fp = fopen ("pdtinit.txt", "w");
      if (Ddi_BddPrintSupportAndCubes(end, 1, -1, 0, NULL, fp) == 0) {
        Pdtutil_Warning(1, "Store cube Failed.");
      }
      fclose(fp);
      Ddi_Free(end);
    }

    Trav_MgrQuit(travMgrFail);
    Ddi_Free(piv);
    Ddi_Free(invspec_bar);
    Ddi_Free(patternMisMat);

#endif

  }

  Ddi_Free(frontier);
  Ddi_Free(badStates);

#if 0
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c,
    currTime = util_cpu_time();
    fprintf(stdout, "Live nodes (peak): %u(%u)\n",
      Ddi_MgrReadLiveNodes(ddiMgr), Ddi_MgrReadPeakLiveNodeCount(ddiMgr));
    fprintf(stdout, "TotalTime: %s ",
      util_print_time(currTime - opt->stats.startTime));
    fprintf(stdout, "(setup: %s -",
      util_print_time(opt->stats.setupTime - opt->stats.startTime));
//fprintf(stdout, "(setup: %s -", util_print_time (initialTime-opt->stats.startTime));
    fprintf(stdout, " trav: %s)\n", util_print_time(currTime - initialTime))
    );
#endif

  return (verifOK);
}

/**Function*******************************************************************
  Synopsis    [exact forward verification]
  Description []
  SideEffects []
  SeeAlso     [FbvApproxForwApproxBckExactForwVerify]
******************************************************************************/
int
Trav_TravBddApproxForwApproxBckExactForwVerify(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  int doCoi,
  int doApproxBck
)
{
  int i, j, verifOK, fixPoint, fwdRings;
  Ddi_Bdd_t *tmp, *from, *fwdReached, *bckReached, *to,
    *notInit, *invar, *badStates;
  Ddi_Vararray_t *ps;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(init);
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Tr_Tr_t *bckTr, *fwdTr;
  long initialTime, currTime;
  Ddi_Bddarray_t *frontier;
  Ddi_Bdd_t *clock_tr = NULL;
  Ddi_Var_t *clock_var = NULL;
  Trav_Settings_t *opt = &travMgr->settings;
  int clustTh = opt->tr.clustThreshold;
  char *travClk = opt->clk;
  int coiLimit = Tr_MgrReadCoiLimit(trMgr);

  if (travClk != NULL) {
#if 0
    clock_var = Ddi_VarFromName(ddiMgr, travClk);
#else
    clock_var = Tr_TrSetClkStateVar(tr, travClk, 1);
    if (travClk != Ddi_VarName(clock_var)) {
      Ddi_Bdd_t *lit = Ddi_BddMakeLiteral(clock_var, 0);

      travClk = Ddi_VarName(clock_var);
      Ddi_BddAndAcc(init, lit);
      Ddi_Free(lit);
    }
#endif

    if (clock_var != NULL) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Clock Variable (%s) Detected.\n", travClk));

      for (i = 0; i < Ddi_BddPartNum(Tr_TrBdd(tr)); i++) {
        Ddi_Varset_t *supp = Ddi_BddSupp(Ddi_BddPartRead(Tr_TrBdd(tr), i));

        /*Ddi_VarsetPrint(supp, 0, NULL, stdout); */
        if (Ddi_VarInVarset(supp, clock_var)) {
          clock_tr = Ddi_BddDup(Ddi_BddPartRead(Tr_TrBdd(tr), i));
          Ddi_Free(supp);

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
            fprintf(stdout, "Found Clock TR.\n"));

          break;
        } else {
          Ddi_Free(supp);
        }
      }
    }
  }

  ps = Tr_MgrReadPS(trMgr);

  badStates = Ddi_BddNot(invarspec);

  {
    Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));

    Ddi_BddExistAcc(badStates, piv);
    Ddi_Free(piv);
  }

#if ENABLE_INVAR
  invar = Ddi_BddPartExtract(Tr_TrBdd(tr), 0);
#else
  invar = Ddi_BddMakeConst(ddiMgr, 1);
#endif

  Pdtutil_VerbosityMgrIf(trMgr, Pdtutil_VerbLevelUsrMed_c) {
    fprintf(stdout, "|Invar|=%d; ", Ddi_BddSize(invar));
    fprintf(stdout, "|Tr|=%d.\n", Ddi_BddSize(Tr_TrBdd(tr)));
  }

  fwdTr = Tr_TrDup(tr);
  bckTr = Tr_TrDup(tr);

  Tr_MgrSetClustThreshold(trMgr, 1);
  Tr_TrSetClustered(tr);
  Tr_TrSortIwls95(tr);
  Tr_MgrSetClustThreshold(trMgr, clustTh);


  if (opt->tr.approxTrClustFile != NULL) {
    Tr_Tr_t *newTr = Tr_TrManualPartitions(fwdTr, opt->tr.approxTrClustFile);

    Tr_TrFree(fwdTr);
    fwdTr = newTr;
  }

  if (doCoi == 1 && coiLimit > 0) {
    Ddi_Varsetarray_t *coi;
    Tr_Tr_t *reducedTR;
    int n;

    coi = Tr_TrComputeCoiVars(fwdTr, invarspec, coiLimit);
    n = Ddi_VarsetarrayNum(coi);

    reducedTR =
      Tr_TrCoiReductionWithVars(fwdTr, Ddi_VarsetarrayRead(coi, n - 1),
      coiLimit);

    Tr_TrFree(fwdTr);
    fwdTr = reducedTR;

    Ddi_Free(coi);
  }


  Tr_MgrSetClustSmoothPi(trMgr, 1);
  Tr_MgrSetClustThreshold(trMgr, 1);
  Tr_TrSetClustered(fwdTr);
  Tr_TrSortIwls95(fwdTr);
  Tr_MgrSetClustThreshold(trMgr, clustTh);
  Tr_TrSetClustered(fwdTr);

  if (opt->tr.sortForBck != 0) {
    Tr_TrSortForBck(bckTr);
    Tr_TrSetClustered(bckTr);
  } else {
    Tr_TrFree(bckTr);
    bckTr = Tr_TrDup(fwdTr);
  }


#if 1
  {
    Ddi_Bdd_t *group, *p;

    for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(fwdTr));) {
      group = Ddi_BddPartRead(Tr_TrBdd(fwdTr), j);
      Ddi_BddSetPartConj(group);
      p = Ddi_BddPartRead(Tr_TrBdd(fwdTr), i);
      Ddi_BddPartInsertLast(group, p);
      if (Ddi_BddSize(group) > opt->bdd.apprGrTh) {
        /* resume from group insert */
        p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
        Ddi_Free(p);
        if (clock_tr != NULL) {
          Ddi_BddPartInsert(group, 0, clock_tr);
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "#"));
        }
        i++;
        j++;
      } else {
        p = Ddi_BddPartExtract(Tr_TrBdd(fwdTr), i);
        Ddi_Free(p);
      }
    }
    if (clock_tr != NULL) {
      Ddi_BddPartInsert(group, 0, clock_tr);
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "#"));
    }
  }
#if 0
  for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(bckTr));) {
    Ddi_Bdd_t *group, *p;

    group = Ddi_BddPartRead(Tr_TrBdd(bckTr), j);
    Ddi_BddSetPartConj(group);
    p = Ddi_BddPartRead(Tr_TrBdd(bckTr), i);
    Ddi_BddPartInsertLast(group, p);
    if (Ddi_BddSize(group) > 4 * opt->bdd.apprGrTh) {
      /* resume from group insert */
      p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
      Ddi_Free(p);
      i++;
      j++;
    } else {
      p = Ddi_BddPartExtract(Tr_TrBdd(bckTr), i);
      Ddi_Free(p);
    }
  }
#endif
#endif

  Ddi_Free(clock_tr);

  initialTime = util_cpu_time();


  /* approx forward */

  from = Ddi_BddDup(init);
  Ddi_BddAndAcc(from, invar);

  /* disjunctively partitioned reached */
  fwdReached = Ddi_BddMakePartDisjVoid(ddiMgr);
  Ddi_BddPartInsert(fwdReached, 0, from);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "-- Approximate Forward.\n");
    fprintf(stdout, "|TR|=%d.\n", Ddi_BddSize(Tr_TrBdd(fwdTr)))
    );

  for (i = 0, fixPoint = 0;
    (!fixPoint) && ((opt->bdd.bound < 0) || (i < opt->bdd.bound)); i++) {

    if (i >= opt->bdd.nExactIter) {
      Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodApprox_c);
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
        fprintf(stdout,
          "-- Approximate Forward Verification; #Iteration=%d.\n", i + 1));
    } else {
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "-- Exact Forward Verification; #Iteration=%d.\n",
          i + 1));
    }

    /*      Ddi_BddAndAcc(from,invarspec); */

    to = Tr_Img(fwdTr, from);

    if (opt->bdd.apprNP > 0) {
      to =
        Ddi_BddEvalFree(Tr_TrProjectOverPartitions(to, NULL, NULL,
          opt->bdd.apprNP, opt->bdd.apprPreimgTh), to);
    }
    /*Ddi_BddSetMono(to); */

    tmp = Ddi_BddAnd(init, to);
    Ddi_BddSetMono(tmp);
    if (!Ddi_BddIsZero(tmp)) {
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Init States are Reachable from Init.\n"));
    } else {
      if (!opt->bdd.strictBound /*&& opt->mc.storeCNF == NULL */ )
        orProjectedSetAcc(to, init, NULL);
    }
    Ddi_Free(tmp);

    Ddi_Free(from);
    Ddi_BddPartInsertLast(fwdReached, to);

    from = Ddi_BddDup(to);
#if 0
    fixPoint = Ddi_BddIsZero(tmp);
#endif
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|To|=%d; ", Ddi_BddSize(to));
      fprintf(stdout, "|Reached|=%d; ", Ddi_BddSize(fwdReached));
      if (Ddi_BddIsMono(to)) {
      fprintf(stdout, "#ToStates=%g.\n",
          Ddi_BddCountMinterm(to, Ddi_VararrayNum(ps)));}
    ) ;

    Ddi_Free(to);
  }

  Ddi_Free(from);


  if (opt->bdd.wR != NULL) {
    Ddi_MgrReduceHeap(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"), 0);

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "Writing outer reached ring to %s\n", opt->bdd.wR));

    Ddi_BddStore(Ddi_BddPartRead(fwdReached, Ddi_BddPartNum(fwdReached) - 1),
      "reachedPlus", DDDMP_MODE_TEXT, opt->bdd.wR, NULL);
  }

  /* Ddi_BddSetMono(fwdReached); */

  if ((opt->bdd.storeCNF != NULL) && (opt->bdd.storeCNFphase == 0)) {
    storeCNFRings(fwdReached, NULL, Tr_TrBdd(bckTr), badStates, opt);
    currTime = util_cpu_time();
    travMgr->travTime = currTime - initialTime;

    Ddi_Free(invar);
    Ddi_Free(badStates);
    Ddi_Free(fwdReached);
    Tr_TrFree(fwdTr);
    Tr_TrFree(bckTr);
    return (1);
  }

  if (0 && doCoi) {
    Ddi_Varsetarray_t *coi;
    int i, j, k, n;

    coi = Tr_TrComputeCoiVars(fwdTr, invarspec, coiLimit);
    n = Ddi_VarsetarrayNum(coi);

    for (i = Ddi_BddPartNum(fwdReached) - 1, j = n - 1; j >= 0; i--, j--) {
      Ddi_Bdd_t *p = Ddi_BddPartRead(fwdReached, i);
      Ddi_Varset_t *vs = Ddi_VarsetarrayRead(coi, j);

      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "Projecting r+[%d] to %d vars\n", i,
          Ddi_VarsetNum(vs)));
      if (Ddi_BddIsMono(p)) {
        Ddi_BddExistProjectAcc(p, vs);
      } else {
        for (k = 0; k < Ddi_BddPartNum(p); k++) {
          Ddi_Bdd_t *pk = Ddi_BddPartRead(p, k);

          Ddi_BddExistProjectAcc(pk, vs);
        }
      }
    }

    Ddi_Free(coi);
  }


  /* exact backward */

#if 0
  if (opt->bdd.useMeta) {
    Ddi_MetaInit(Ddi_ReadMgr(init), Ddi_Meta_EarlySched, Tr_TrBdd(tr), NULL,
      Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr), 100);
  }
  if (opt->bdd.useMeta) {
    Ddi_BddSetMeta(fwdReached);
  }
#endif

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "Backward Approximate.\n"));

  verifOK = 1;


  Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodIwls95_c);
#if 0
  Tr_MgrSetClustThreshold(trMgr, 100000);
#endif

#if 0
#if 1
  Ddi_BddConstrainAcc(Tr_TrBdd(bckTr), fwdReached);
#endif
#endif

#if 0
  Tr_TrSortIwls95(bckTr);
  Tr_TrSetClustered(bckTr);
#endif

  notInit = Ddi_BddNot(init);

  if (!doApproxBck && (!Ddi_BddIsOne(invarspec))) {
    for (i = Ddi_BddPartNum(fwdReached) - 2; i >= 0; i--) {
      Ddi_Bdd_t *p = Ddi_BddPartRead(fwdReached, i);

      Ddi_BddSetPartConj(p);
      Ddi_BddPartInsert(p, 0, invarspec);
    }
  }

  from = Ddi_BddDup(badStates);

  bckReached = Ddi_BddMakePartDisjVoid(ddiMgr);

  if (0) {
    Tr_Tr_t *newTr = Tr_TrPartition(bckTr, "CLK", 0);

    Ddi_BddSetClustered(Tr_TrBdd(bckTr), clustTh);
    Tr_TrFree(bckTr);
    bckTr = newTr;
  }



  /*Tr_TrReverseAcc(bckTr); done in approx preimg */
  verifOK = !travOverRings(from, NULL, init, bckTr, fwdReached,
    bckReached, NULL, Ddi_BddPartNum(fwdReached),
    "Approx backwd iteration", initialTime, 1 /*reverse */ ,
    doApproxBck, opt);
  //doApproxBck || (opt->mc.storeCNF != NULL), opt);

  frontier = NULL;

  if ((opt->bdd.storeCNF != NULL) && (opt->bdd.storeCNFphase == 1)) {
    storeCNFRings(fwdReached, bckReached, Tr_TrBdd(bckTr), badStates, opt);
    currTime = util_cpu_time();
    travMgr->travTime = currTime - initialTime;

    Tr_TrFree(fwdTr);
    Tr_TrFree(bckTr);
    Ddi_Free(bckReached);
    Ddi_Free(from);
    Ddi_Free(invar);
    Ddi_Free(badStates);
    Ddi_Free(notInit);
    Ddi_Free(fwdReached);
    return (1);
  }

  if (verifOK) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: PASS.\n"));
  } else {

    if ((opt->bdd.storeCNF != NULL) && (opt->bdd.storeCNFphase == 1)) {
#if 0
      storeCNFRings(fwdReached, bckReached, Tr_TrBdd(bckTr), badStates, opt);
      currTime = util_cpu_time();
      travMgr->travTime = currTime - initialTime;

      Tr_TrFree(fwdTr);
      Tr_TrFree(bckTr);
      Ddi_Free(bckReached);
      Ddi_Free(from);
      Ddi_Free(invar);
      Ddi_Free(badStates);
      Ddi_Free(notInit);
      Ddi_Free(fwdReached);
      return (1);
#endif
    } else if (!doApproxBck) {
      frontier = Ddi_BddarrayMakeFromBddPart(bckReached);
      fwdRings = 0;
    } else {
      Ddi_Bdd_t *overApprox = Ddi_BddDup(fwdReached);

      Ddi_BddPartWrite(overApprox, 0, init);
      /* opt->bdd.constrainLevel = 0; */
      verifOK = !travOverRings(init, NULL, badStates, fwdTr, bckReached,
        fwdReached, overApprox, Ddi_BddPartNum(bckReached),
        "Exact fwd iteration", initialTime, 0 /*reverse */ , 0 /*approx */ ,
        opt);

      Ddi_Free(overApprox);
      frontier = Ddi_BddarrayMakeFromBddPart(fwdReached);
      fwdRings = 1;
    }
  }

  Tr_TrFree(fwdTr);
  Tr_TrFree(bckTr);
  Ddi_Free(bckReached);
  Ddi_Free(from);
  Ddi_Free(invar);
  Ddi_Free(notInit);
  Ddi_Free(fwdReached);

  if (verifOK) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: PASS.\n"));
  } else {
    FILE *fp;
    Trav_Mgr_t *travMgrFail;
    Ddi_Bddarray_t *patternMisMat;
    Ddi_Varset_t *piv;
    Ddi_Bdd_t *invspec_bar, *end;

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: FAIL.\n"));

    Tr_MgrSetClustThreshold(trMgr, clustTh);
    Tr_TrSetClustered(tr);

    Tr_MgrSetClustSmoothPi(trMgr, 0);
    travMgrFail = Trav_MgrInit(NULL, ddiMgr);
    travMgrFail->settings.bdd.mismatchOptLevel = opt->bdd.mismatchOptLevel;

#if 1
    piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));
#if 0
    /* keep PIs for counterexample */
    invspec_bar = Ddi_BddExist(invarspec, piv);
#else
    invspec_bar = Ddi_BddNot(invarspec);
#endif

#if 0
    Tr_MgrSetImgMethod(Tr_TrMgr(tr), Tr_ImgMethodCofactor_c);
#endif

    if (fwdRings) {
      Tr_TrReverseAcc(tr);
      patternMisMat = Trav_MismatchPat(travMgr, tr, invspec_bar,
        NULL, NULL, &end, frontier,
        Tr_MgrReadPS(Tr_TrMgr(tr)), Tr_MgrReadNS(Tr_TrMgr(tr)), piv);
      Tr_TrReverseAcc(tr);
    } else {
      patternMisMat = Trav_MismatchPat(travMgr, tr, NULL,
        invspec_bar, &end, NULL, frontier,
        Tr_MgrReadPS(Tr_TrMgr(tr)), Tr_MgrReadNS(Tr_TrMgr(tr)), piv);
    }

    /*---------------------------- Write Patterns --------------------------*/

    if (patternMisMat != NULL) {
      char filename[100];

      sprintf(filename, "%s%d.txt", FILEPAT, getpid());
      fp = fopen(filename, "w");
      //fp = fopen ("pdtpat.txt", "w");
      if (Ddi_BddarrayPrintSupportAndCubes(patternMisMat, 1, -1,
          0, 0, NULL, fp) == 0) {
        Pdtutil_Warning(1, "Store cube Failed.");
      }
      fclose(fp);
    }

    /*----------------------- Write Initial State Set ----------------------*/

    if (end != NULL && !Ddi_BddIsZero(end)) {
      char filename[100];

      sprintf(filename, "%s%d.txt", FILEINIT, getpid());
      fp = fopen(filename, "w");
      //fp = fopen ("pdtinit.txt", "w");
      if (Ddi_BddPrintSupportAndCubes(end, 1, -1, 0, NULL, fp) == 0) {
        Pdtutil_Warning(1, "Store cube Failed.");
      }
      fclose(fp);
      Ddi_Free(end);
    }

    Trav_MgrQuit(travMgrFail);
    Ddi_Free(piv);
    Ddi_Free(invspec_bar);
    Ddi_Free(patternMisMat);

#endif

  }

  Ddi_Free(frontier);
  Ddi_Free(badStates);

  currTime = util_cpu_time();
  travMgr->travTime = currTime - initialTime;

  return (verifOK);
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [And possibly conj. part. sets. Underapprox result monolitic]
  Description [And possibly conj. part. sets. Underapprox result monolitic.
               Underapprox based on abort mechanism.
              ]
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
FindSubsetOfItersectionMono(
  Ddi_Bdd_t * set,
  Ddi_Bdd_t * ring,
  Ddi_Varset_t * smoothV,
  int optLevel
)
{

  Ddi_Mgr_t *ddm = Ddi_ReadMgr(set);
  int again = 0;
  int myClipTh = ddm->exist.clipThresh;
  int myClipLevel = ddm->exist.clipLevel;
  Ddi_Bdd_t *res;
  Ddi_Varset_t *sm;

  ddm->exist.clipDone = 0;
  ddm->exist.clipLevel = 2;

  if (smoothV == NULL) {
    sm = Ddi_VarsetVoid(ddm);
  } else {
    sm = Ddi_VarsetDup(smoothV);
  }

  do {

    if (0 && (optLevel > 0)) {
      Ddi_Bdd_t *tmp;
      int l, n;
      Ddi_Varset_t *s1;
      Ddi_Varset_t *s2;

      Ddi_BddRestrictAcc(ring, set);

      s1 = Ddi_BddSupp(set);
      s2 = Ddi_BddSupp(ring);

      Ddi_VarsetUnionAcc(s1, s2);
      Ddi_Free(s2);
      s2 = Ddi_VarsetVoid(ddm);
      n = Ddi_VarsetNum(s1);
      for (l = 0, Ddi_VarsetWalkStart(s1); !Ddi_VarsetWalkEnd(s1);
        Ddi_VarsetWalkStep(s1)) {
        if (l++ > n / 3) {
          Ddi_VarsetAddAcc(s2, Ddi_VarsetWalkCurr(s1));
        }
      }

      tmp = Ddi_BddAndExist(set, ring, s2);

      Ddi_VarsetDiffAcc(s1, s2);
      Ddi_BddSetMono(tmp);
      Ddi_BddPickOneMintermAcc(tmp, s1);
      Ddi_Free(s1);
      Ddi_Free(s2);
      Ddi_BddAndAcc(set, tmp);
      /* Ddi_BddAndAcc(ring,tmp); */
      Ddi_BddRestrictAcc(ring, tmp);
      Ddi_BddRestrictAcc(ring, set);
      Ddi_Free(tmp);
    }


    res = Ddi_BddRestrict(set, ring);
    Ddi_BddAndExistAcc(res, ring, sm);

    if (Ddi_BddIsZero(res) && ddm->exist.clipDone) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Clip within Mismatch Computation.\n")
        );
      ddm->exist.clipDone = 0;
      ddm->exist.clipThresh = (int)(ddm->exist.clipThresh * 1.5);
      Ddi_BddNotAcc(ddm->exist.clipWindow);
      if (Ddi_BddIsPartConj(set)) {
        Ddi_BddAndAcc(Ddi_BddPartRead(set, 0), ddm->exist.clipWindow);
      } else {
        Ddi_BddAndAcc(set, ddm->exist.clipWindow);
      }
      Ddi_Free(ddm->exist.clipWindow);
      again = 1;
      Ddi_Free(res);
    } else {
      Ddi_BddSetMono(res);
    }

  } while (again);

  ddm->exist.clipThresh = myClipTh;
  ddm->exist.clipLevel = myClipLevel;

  Ddi_Free(sm);

  return (res);

}


#if 0
static void
TraceFrozenLatches(
  Trav_Mgr_t * travMgr /* traversal manager */ ,
  Ddi_Mgr_t * dd /* dd manager */ ,
  Ddi_Vararray_t * s /* present state vars */ ,
  Ddi_Vararray_t * y /* next state vars */ ,
  Ddi_Varset_t * quantify /* set of quantifying variables */ ,
  Ddi_Bdd_t * TR /* Transition Relation */ ,
  Ddi_Bdd_t * from /* result of image computation */ ,
  Ddi_Bdd_t * to /* result of image computation */ ,
  Ddi_Bdd_t * reached /* old reached */ ,
  int *frozen_latches /* array of frozen latches */ ,
  Pdtutil_VerbLevel_e verbosity
)
{
  int i, ns;
  Ddi_Var_t *vs, *vy;
  Ddi_Bdd_t *constr_TR, *tmp, *eq, *newY, *reachedY;

  /*  assert(Ddi_BddIsMono(TR)); */

  ns = Ddi_VararrayNum(s);

  reachedY = Ddi_BddSwapVars(reached, s, y);
  newY = Ddi_BddAnd(to, (tmp = Ddi_BddNot(reachedY)));

  Ddi_Free(tmp);
  Ddi_Free(reachedY);

  for (i = 0; i < ns; i++) {
    if (frozen_latches[i] == 1) {
      vs = Ddi_VararrayRead(s, i);
      vy = Ddi_VararrayRead(y, i);
      eq = Ddi_BddXnor(Ddi_BddMakeLiteral(vs, 1), Ddi_BddMakeLiteral(vy, 1));

      constr_TR = Ddi_BddAnd(TR, eq);

      to = Trav_ImgConjPartTr(travMgr, constr_TR, y, y, from, quantify);

      if (!(Ddi_BddIncluded(newY, to)))
        frozen_latches[i] = 0;

      Ddi_Free(eq);
      Ddi_Free(constr_TR);
      Ddi_Free(to);
    }
  }

  return;
}
#endif

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
diffProjectedSetAcc(
  Ddi_Bdd_t * f,
  Ddi_Bdd_t * g,
  Ddi_Var_t * partVar
)
{
  Ddi_Bdd_t *p;
  int i, j;

  if (Ddi_BddIsMono(f)) {
    return (Ddi_BddDiffAcc(f, g));
  }

  if (Ddi_BddIsMeta(f)) {
    return (Ddi_BddDiffAcc(f, g));
  }

  if (Ddi_BddIsPartDisj(f)) {
    Ddi_Bdd_t *p = NULL;

    Pdtutil_Assert(0, "Not yet supported diffProjectedSet with dpart");
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

  for (i = 0; i < Ddi_BddPartNum(f); i++) {

    p = Ddi_BddPartRead(f, i);
    Ddi_BddDiffAcc(p, g);

  }

  return (f);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
dynAbstract(
  Ddi_Bdd_t * f,
  int maxSize,
  int maxAbstract
)
{
  Ddi_Varset_t *supp;
  Ddi_Bdd_t *newF;
  int i, j, nv, bestI, initSize;
  Ddi_Vararray_t *va;
  int over_under_approx;
  double refCnt, w, bestW;

  supp = Ddi_BddSupp(f);
  nv = Ddi_VarsetNum(supp);
  refCnt = Ddi_BddCountMinterm(f, nv);

  newF = Ddi_BddDup(f);
  va = Ddi_VararrayMakeFromVarset(supp, 1);
  Ddi_Free(supp);

  over_under_approx = 1;        /* 0: under, 1: over */
  j = 0;
  initSize = Ddi_BddSize(f);

  do {
    Ddi_Var_t *v;
    Ddi_Varset_t *vs;
    Ddi_Bdd_t *f0;
    Ddi_Bdd_t *f1;
    Ddi_Bdd_t *f01;

    bestI = -1;
    bestW = over_under_approx ? -1.0 : 0.0;
    for (i = 0; i < Ddi_VararrayNum(va); i++) {
      v = Ddi_VararrayRead(va, i);
      f0 = Ddi_BddCofactor(newF, v, 0);
      f1 = Ddi_BddCofactor(newF, v, 1);
      if (over_under_approx) {
        f01 = Ddi_BddOr(f0, f1);
      } else {
        f01 = Ddi_BddAnd(f0, f1);
      }
      Ddi_Free(f0);
      Ddi_Free(f1);
      w = Ddi_BddCountMinterm(f01, nv) / refCnt;
      if (bestI < 0 || ((Ddi_BddSize(f01) < Ddi_BddSize(newF)) &&
          (over_under_approx ? w < bestW : w > bestW))) {
        bestI = i;
        bestW = w;
      }
      Ddi_Free(f01);
    }
    Pdtutil_Assert(bestI >= 0, "bestI not found");
    v = Ddi_VararrayRead(va, bestI);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "best abstract var found: %s - cost: %f\n",
        Ddi_VarName(v), bestW));

    vs = Ddi_VarsetMakeFromVar(v);
    if (over_under_approx) {
      Ddi_BddExistAcc(newF, vs);
    } else {
      Ddi_BddForallAcc(newF, vs);
    }
    Ddi_Free(vs);
    Ddi_VararrayRemove(va, bestI);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "new size: %d\n", Ddi_BddSize(newF)));

    j++;

  } while (Ddi_BddSize(newF) > maxSize && (maxAbstract < 0
      || j < maxAbstract));

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
    Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "Dynamic Abstract (%sapprox): %d(nv:%d) -> %d(nv:%d)\n",
      over_under_approx ? "over" : "under",
      initSize, nv, Ddi_BddSize(newF), nv - j)
    );

  w = Ddi_BddCountMinterm(newF, nv);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
    Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "Dynamic Abstract minterms: %g -> %g (%f)\n",
      refCnt, w, w / refCnt));

  Ddi_Free(va);

  return (newF);

}

/**Function*******************************************************************
  Synopsis    [find dynamic non vacuum hint]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
findNonVacuumHint(
  Ddi_Bdd_t * from,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * pi,
  Ddi_Vararray_t * aux,
  Ddi_Bdd_t * reached,
  Ddi_Bddarray_t * delta,
  Ddi_Bdd_t * trBdd,
  Ddi_Bdd_t * hConstr,
  Ddi_Bdd_t * prevHint,
  int nVars,
  int hintsStrategy
)
{
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(from);
  Ddi_Bdd_t *trAndNotReached = Ddi_BddNot(reached);
  Ddi_Bdd_t *myFromAig, *myFrom = Ddi_BddDup(from);
  Ddi_Var_t *iv = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
  Ddi_Bdd_t *h, *hFull=NULL, *cex0 = NULL, *cex = NULL;
  Ddi_Bdd_t *partBdd = trBdd != NULL ? trBdd : from;
  int abo, maxIter = nVars;
  Ddi_Varset_t *vars = Ddi_VarsetMakeFromArray(ps);
  Ddi_Varset_t *vars1 = Ddi_VarsetMakeFromArray(pi);
  int timeLimit = -1;

  if (nVars == 0) {
    Ddi_Free(vars);
    Ddi_Free(vars1);
    Ddi_Free(myFrom);
    Ddi_Free(trAndNotReached);
    return Ddi_BddMakeConst(ddiMgr, 1);
  }

  Ddi_VarsetUnionAcc(vars, vars1);
  Ddi_Free(vars1);
  if (aux != NULL) {
    Ddi_Varset_t *vars1 = Ddi_VarsetMakeFromArray(aux);

    Ddi_VarsetDiffAcc(vars, vars1);
    Ddi_Free(vars1);
  }

  Ddi_BddSetAig(trAndNotReached);
  if (iv != NULL) {
    Ddi_Bdd_t *invar = Ddi_BddMakeLiteralAig(iv, 1);

    Ddi_AigAndCubeAcc(trAndNotReached, invar);
    Ddi_BddSetMono(invar);
    Ddi_BddConstrainAcc(myFrom, invar);
    Ddi_BddAndAcc(myFrom, invar);
    Ddi_Free(invar);
  }

  Ddi_BddComposeAcc(trAndNotReached, ps, delta);
  if (iv != NULL) {
    Ddi_Bdd_t *invar = Ddi_BddMakeLiteralAig(iv, 1);

    Ddi_BddAndAcc(trAndNotReached, invar);
    Ddi_Free(invar);
  }

  myFromAig = Ddi_BddMakeAig(myFrom);
  if (hConstr != NULL && !Ddi_BddIsOne(hConstr)) {
    Ddi_BddAndAcc(myFromAig, hConstr);
  }

  h = NULL;

  if (hintsStrategy == 1 && prevHint != NULL) {
    int sat;
    Ddi_Bdd_t *myBlock = Ddi_BddMakeAig(prevHint);
    Ddi_Bdd_t *fAig = Ddi_BddAnd(myFromAig, trAndNotReached);
    Ddi_Varset_t *sm;

    sat = Ddi_AigSatMinisatWithAbortAndFinal(fAig, myBlock, 10, 0);
    Pdtutil_Assert(sat <= 0, "unsat or undef required");
    Ddi_Free(fAig);
    sm = Ddi_VarsetMakeFromVar(Ddi_BddTopVar(myBlock));
    Ddi_Free(myBlock);
    h = Ddi_BddExist(prevHint, sm);
    Ddi_Free(sm);
  }

  if (h == NULL) {
    cex0 = Ddi_AigSatWithCex(myFromAig);
    cex =
      Ddi_AigSatAndWithCexAndAbort(cex0, trAndNotReached, NULL, NULL, 5, &abo);
    if (cex == NULL || abo) {
      cex = Ddi_AigSatAndWithCexAndAbort(myFromAig, trAndNotReached,
        NULL, NULL, 10, &abo);
      if (cex == NULL || abo) {
        int k, sat;

        h = Ddi_BddMakeConst(ddiMgr, 1);
        for (k = sat = 0; k < maxIter && !sat; k++) {
          Ddi_Bdd_t *myCex = Ddi_BddDup(cex0);
          Ddi_Bdd_t *fAig = Ddi_BddDup(trAndNotReached);

          sat = Ddi_AigSatMinisatWithAbortAndFinal(fAig, myCex, timeLimit, 4);
          if (!sat) {
            Ddi_Free(cex0);
            Ddi_BddDiffAcc(myFromAig, myCex);
            if (hConstr != NULL)
              Ddi_BddDiffAcc(hConstr, myCex);
            cex0 = Ddi_AigSatWithCex(myFromAig);
            Ddi_BddSetMono(myCex);
            Ddi_BddDiffAcc(h, myCex);
          } else {
            Ddi_Free(h);
            cex = Ddi_BddDup(myCex);
          }
          Ddi_Free(myCex);
          Ddi_Free(fAig);
        }
      }
    }
  }

  if (h == NULL) {
    hFull = Part_PartitionDisjSet(partBdd, cex, vars,
      Part_MethodCofactorKernel_c,
      Ddi_BddSize(partBdd) / 100, nVars, Pdtutil_VerbLevelDevMax_c);
    h = Ddi_BddPartExtract(hFull, 0);
  }

  Ddi_Free(cex0);
  Ddi_Free(cex);

  Ddi_Free(trAndNotReached);
  Ddi_Free(myFromAig);
  Ddi_Free(myFrom);
  Ddi_Free(trAndNotReached);

  Ddi_Free(hFull);
  Ddi_Free(vars);

  return h;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static int
travBddCexToFsm(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * cexArray,
  Ddi_Bdd_t * initState,
  int reverse
)
{
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(cexArray);

  // write cex as aig into fsm
  Ddi_Bdd_t *cexAig = Ddi_BddMakePartConjVoid(ddiMgr);
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  int i;

  if (initStub != NULL) {
    Pdtutil_Assert(initState != NULL && !Ddi_BddIsZero(initState),
      "missing init state in cex");
    Ddi_Bdd_t *chk = Ddi_BddRelMakeFromArray(initStub, ps);
    Ddi_Bdd_t *initAig = Ddi_BddMakeAig(initState);

    Ddi_BddSetAig(chk);
    Ddi_AigConstrainCubeAcc(chk, initAig);
    Ddi_Free(initAig);
    Ddi_Bdd_t *cex0 = Ddi_AigSatWithCex(chk);

    Ddi_BddPartInsertLast(cexAig, cex0);
    Ddi_Free(chk);
    Ddi_Free(cex0);
  }

  if (reverse) {
    for (i = Ddi_BddarrayNum(cexArray) - 1; i >= 0; i--) {
      Ddi_Bdd_t *c_i = Ddi_BddDup(Ddi_BddarrayRead(cexArray, i));

      Ddi_BddSetAig(c_i);
      Ddi_BddPartInsertLast(cexAig, c_i);
      Ddi_Free(c_i);
    }
  } else {
    for (i = 0; i < Ddi_BddarrayNum(cexArray); i++) {
      Ddi_Bdd_t *c_i = Ddi_BddDup(Ddi_BddarrayRead(cexArray, i));

      Ddi_BddSetAig(c_i);
      Ddi_BddPartInsertLast(cexAig, c_i);
      Ddi_Free(c_i);
    }
  }
  Fsm_MgrSetCexBDD(fsmMgr, cexAig);

  return 1;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static int
travOverRingsWithFixPoint(
  Ddi_Bdd_t * from0,
  Ddi_Bdd_t * reached0,
  Ddi_Bdd_t * target,
  Tr_Tr_t * TR,
  Ddi_Bdd_t * inRings,
  Ddi_Bdd_t * outRings,
  Ddi_Bdd_t * overApprox,
  int maxIter,
  char *iterMsg,
  long initialTime,
  int reverse,
  int approx,
  Trav_Settings_t * opt
)
{
  int targetReached, i, fixPoint;
  Ddi_Bdd_t *care, *careAig = NULL,
    *careNS, *ring, *tmp, *from, *reached, *to, *oldReached;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(from0);
  Tr_Mgr_t *trMgr = Tr_TrMgr(TR);
  Tr_Tr_t *myTr = Tr_TrDup(TR);
  long currTime;
  Ddi_Bdd_t *toPlus;
  int useAigReached = opt->bdd.useMeta != 0;
  int existOptLevel = Ddi_MgrReadExistOptLevel(ddiMgr);
  int coiLimit = Tr_MgrReadCoiLimit(trMgr);

  Pdtutil_Assert(Ddi_BddIsPartDisj(outRings), "Disj PART out rings required");

  /* exact backward */

  targetReached = 0;
  from = Ddi_BddDup(from0);

  if (opt->bdd.countReached) {
    reached = Ddi_BddMakeConst(ddiMgr, 0);
  } else {
    reached = Ddi_BddDup(from);
  }
  /* Ddi_BddSetMono(reached); */

  Ddi_BddPartWrite(outRings, 0, from);
  /*@@@@@ */
#if 0
  tmp = Ddi_BddMakeConst(ddiMgr, 0);
  for (i = Ddi_BddPartNum(inRings) - 1; i >= 0; i--) {
    Ddi_Free(tmp);
    tmp = Ddi_BddAnd(Ddi_BddPartRead(inRings, i), target);
    if (!Ddi_BddIsZero(tmp))
      break;
  }
  Ddi_Free(tmp);
#endif

  care = Ddi_BddPartRead(inRings, Ddi_BddPartNum(inRings) - 1);
  oldReached = NULL;

  for (i = 0, fixPoint = 0; (!fixPoint); i++) {

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "-- %s; Iteration=%d.\n", iterMsg, i + 1));

    //    Ddi_BddRestrictAcc(from,care);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|From|=%d; ", Ddi_BddSize(from)));

    Ddi_BddPartWrite(outRings, i, care);
    ring = Ddi_BddPartRead(outRings, i);

    Ddi_BddAndAcc(ring, from);

    if (Ddi_BddIsZero(from)) {
      fixPoint = 1;
      break;
    }

    if (Ddi_BddIsAig(target)) {
      tmp = Ddi_BddMakeAig(care);
      Ddi_BddAndAcc(tmp, target);
    } else {
      tmp = Ddi_BddAnd(care, target);
      Ddi_BddSetMono(tmp);
    }
    if (!Ddi_BddIsZero(tmp)) {
      if (Ddi_BddIsAig(tmp)) {
        Ddi_Bdd_t *f = Ddi_BddMakeAig(from);

        Ddi_BddAndAcc(tmp, f);
        Ddi_Free(f);
        targetReached = Ddi_AigSat(tmp);
      } else {
        Ddi_BddAndAcc(tmp, from);
        Ddi_BddSetMono(tmp);
        targetReached = !Ddi_BddIsZero(tmp);
      }
      if (targetReached) {
        Ddi_Free(tmp);
        if (!Ddi_BddIsAig(target)) {
          Ddi_BddAndAcc(ring, target);
          while (Ddi_BddPartNum(outRings) > (i + 1)) {
            Ddi_Bdd_t *p = Ddi_BddPartExtract(outRings,
              Ddi_BddPartNum(outRings) - 1);

            Ddi_Free(p);
          }
        }
        break;
      }
    }

    Ddi_Free(tmp);

    if (overApprox != NULL) {
      Ddi_BddAndAcc(from, Ddi_BddPartRead(overApprox, i));

      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "|from*overAppr|=%d\n", Ddi_BddSize(from)));
    }

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|TR|=%d.\n", Ddi_BddSize(Tr_TrBdd(myTr))));

    if (!Ddi_BddIsAig(reached) &&
      ((existOptLevel > 0) || Ddi_BddSize(reached) < 10000)) {
#if 0
      careNS = Ddi_BddNot(reached);
      Ddi_BddSetPartConj(careNS);
#if 0
      Ddi_BddSwapVarsAcc(careNS, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
#endif
      Ddi_BddPartInsertLast(careNS, care);
#else
      careNS = Ddi_BddDup(care);
      if (oldReached != NULL) {
        Ddi_BddNotAcc(oldReached);
        Ddi_BddSwapVarsAcc(oldReached, Tr_MgrReadPS(trMgr),
          Tr_MgrReadNS(trMgr));
        Ddi_BddSetPartConj(careNS);
        Ddi_BddPartInsertLast(careNS, oldReached);
        Ddi_Free(oldReached);
      }
#endif
    } else {
      careNS = Ddi_BddMakeConst(ddiMgr, 1);
    }

    if (opt->bdd.useMeta
      && /* Ddi_BddSize(from) > 1000 && */ !Ddi_BddIsMeta(from)) {
      Ddi_BddSetMeta(from);
    }

    if (reverse) {
      to = Trav_ApproxPreimg(myTr, from, careNS, NULL, approx, opt);
    } else {
      Ddi_BddSwapVarsAcc(careNS, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      to = Tr_ImgWithConstrain(myTr, from, careNS);
    }
    Ddi_BddSetMono(to);

    Ddi_Free(careNS);

    toPlus = Ddi_BddDup(to);
    if (0 && !approx && i > 1 && Ddi_BddSize(to) > opt->bdd.apprPreimgTh) {
      Ddi_Varset_t *suppF = Ddi_BddSupp(reached);
      Ddi_Varset_t *suppT = Ddi_BddSupp(to);
      Ddi_Varset_t *suppFT = Ddi_VarsetUnion(suppT, suppF);
      Ddi_Varset_t *keep = NULL;
      Ddi_Bdd_t *out;
      int nv = Ddi_VarsetNum(suppT);

      if (Tr_TrClk(TR) != NULL) {
        keep = Ddi_VarsetMakeFromVar(Tr_TrClk(TR));
      }

      approx = 1;               /* enable approx preimg on next iterations */
      Ddi_VarsetDiffAcc(suppT, suppF);
      if (!Ddi_VarsetIsVoid(suppT)) {
        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
          fprintf(stdout, "Dynamic Abstraction: in=%d -> out=%d vars.\n",
            Ddi_VarsetNum(suppF),
            Ddi_VarsetNum(suppFT) - Ddi_VarsetNum(suppF)));
        if (keep != NULL)
          Ddi_VarsetDiffAcc(suppF, keep);
#if 0
        Ddi_Free(suppT);
        suppT = Ddi_BddSupp(to);
        Ddi_VarsetIntersectAcc(suppF, suppT);
        {
          int k;
          Ddi_Vararray_t *v = Ddi_VararrayMakeFromVarset(suppF, 1);

          for (k = Ddi_VararrayNum(v) / 2; k < Ddi_VararrayNum(v); k++) {
            Ddi_VarsetRemoveAcc(suppF, Ddi_VararrayRead(v, k));
          }
          Ddi_Free(v);
        }
#endif
#if 0
        out = Ddi_BddNot(reached);
        Ddi_BddAndExistAcc(out, to, suppF);
        if (!Ddi_BddIsZero(out)) {
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "Reducing state set (%d-%d)\n",
              Ddi_BddSize(to), Ddi_BddSize(out)));
          Ddi_Free(toPlus);
          toPlus = Ddi_BddDup(out);
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "After smoothing -> %d\n", Ddi_BddSize(toPlus)));
          Ddi_Free(out);
          out = Ddi_BddNot(reached);
          Ddi_BddAndExistAcc(out, to, suppT);
          Ddi_BddAndAcc(toPlus, out);
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "After smoothing 2 -> *%d=%d\n",
              Ddi_BddSize(out), Ddi_BddSize(toPlus)));
        }
#else
        out = dynAbstract(to, (Ddi_BddSize(to) * 3) / 4, 3 * nv / 4);
        Ddi_Free(toPlus);
        toPlus = Ddi_BddDup(out);
#endif
        Ddi_Free(out);
      }
      Ddi_Free(suppF);
      Ddi_Free(suppFT);
      Ddi_Free(suppT);
      Ddi_Free(keep);
    }
#if 1
    if (!Ddi_BddIsAig(reached) && existOptLevel > 0) {
      Ddi_BddNotAcc(reached);
      Ddi_BddRestrictAcc(to, reached);
      Ddi_BddRestrictAcc(toPlus, reached);
      Ddi_BddNotAcc(reached);
    }

    Ddi_BddRestrictAcc(toPlus, care);
    Ddi_BddRestrictAcc(to, care);
#endif

    Ddi_BddSetMono(to);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelDevMin_c,
      LOG_CONST(to, "to")
      );

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "|To|=%d; ", Ddi_BddSize(to)));

    Ddi_Free(from);

    fixPoint = Ddi_BddIsZero(to);
    if (fixPoint) {
      Ddi_Free(to);
      Ddi_Free(toPlus);
      break;
    }
    from = Ddi_BddDup(toPlus);
    Ddi_Free(toPlus);

    if (reached != NULL && useAigReached && Ddi_BddSize(reached) > 30000) {
      if (careAig == NULL && care != NULL) {
        careAig = Ddi_BddMakeAig(care);
      }
      Ddi_BddSetAig(reached);
      Ddi_BddConstrainAcc(to, careAig);
      Ddi_BddSetAig(to);
      Ddi_BddNotAcc(reached);
      fixPoint = !Ddi_AigSatAnd(to, reached, careAig);
      Ddi_BddNotAcc(reached);
      Ddi_BddOrAcc(reached, to);
      Ddi_MgrSetAigAbcOptLevel(ddiMgr, opt->aig.abcOptLevel);
      ddiAbcOptAcc(reached, -1.0);
    } else if (reached != NULL) {
      Ddi_Bdd_t *newReached = Ddi_BddOr(reached, to);

      if (!fixPoint)
        fixPoint = Ddi_BddEqual(newReached, reached);
      Ddi_BddConstrainAcc(newReached, care);
      //      Ddi_BddRestrictAcc(newReached,care);
      if (!fixPoint)
        fixPoint = Ddi_BddEqual(newReached, reached);
#if 0
      oldReached = Ddi_BddDup(reached);
#endif
      if (opt->bdd.fromSelect == Trav_FromSelectCofactor_c) {
        Ddi_BddNotAcc(reached);
        Ddi_BddConstrainAcc(from, reached);
        Ddi_BddConstrainAcc(from, care);
      }

      Ddi_Free(reached);
      reached = newReached;

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "|Reached|=%d; ", Ddi_BddSize(reached));
        fprintf(stdout, "#ReachedStates=%g.\n",
          Ddi_BddCountMinterm(reached, Ddi_VararrayNum(Tr_MgrReadPS(trMgr))))
        );
    }

    if (!useAigReached) {
      if (opt->bdd.fromSelect == Trav_FromSelectReached_c) {
        Ddi_Free(from);
        from = Ddi_BddDup(reached);
      } else if (opt->bdd.fromSelect == Trav_FromSelectBest_c) {
        if (Ddi_BddSize(from) > Ddi_BddSize(reached)) {
          Ddi_Free(from);
          from = Ddi_BddDup(reached);
        }
      }
    }

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      currTime = util_cpu_time();
      fprintf(stdout, "Live Nodes (Peak) = %u (%u); CPU Time=%s\n",
        Ddi_MgrReadLiveNodes(ddiMgr),
        Ddi_MgrReadPeakLiveNodeCount(ddiMgr),
        util_print_time(currTime - initialTime))
      );

    if (0 && coiLimit > 0) {
      Ddi_Varset_t *ps = Ddi_VarsetMakeFromArray(Tr_MgrReadPS(trMgr));
      Ddi_Varset_t *ns = Ddi_VarsetMakeFromArray(Tr_MgrReadNS(trMgr));
      Ddi_Varset_t *supp = Ddi_BddSupp(Tr_TrBdd(TR));
      Ddi_Varset_t *suppPs = Ddi_VarsetIntersect(supp, ps);
      Ddi_Varset_t *suppNs = Ddi_VarsetIntersect(supp, ns);

      Ddi_VarsetSwapVarsAcc(suppNs, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      Ddi_VarsetDiffAcc(suppPs, suppNs);
      Ddi_Free(ps);
      Ddi_Free(ns);
      Ddi_Free(supp);
      Ddi_Free(suppNs);
      Ddi_BddNotAcc(from);
      Ddi_BddExistAcc(from, suppPs);
      Ddi_BddNotAcc(from);
      Ddi_Free(suppPs);
    }

    Ddi_Free(to);
  }

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
    fprintf(stdout, "|Reached|=%d; ", Ddi_BddSize(reached));
    fprintf(stdout, "#ReachedStates=%.g.\n",
      Ddi_BddCountMinterm(reached, Ddi_VararrayNum(Tr_MgrReadPS(trMgr))))
    );

  if (0) {
    Ddi_Var_t *iv = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
    Ddi_Var_t *ivp = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");

    if (iv != NULL) {
      Ddi_Bdd_t *invarIn = Ddi_BddMakeLiteral(iv, 1);

      if (useAigReached) {
        Ddi_BddSetAig(invarIn);
      }
      Ddi_BddAndAcc(reached, invarIn);
      Ddi_Free(invarIn);
      if (0 && (ivp != NULL)) {
        Ddi_Bdd_t *invarInP = Ddi_BddMakeLiteral(ivp, 1);

        Ddi_BddAndAcc(reached, invarInP);
        Ddi_Free(invarInP);
      }
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "#Reached States in Constraints = %g.\n",
          Ddi_BddCountMinterm(reached, Ddi_VararrayNum(Tr_MgrReadPS(trMgr)))));
    }
  }

  Tr_TrFree(myTr);
  Ddi_Free(oldReached);
  Ddi_Free(careAig);
  Ddi_Free(reached);
  Ddi_Free(from);

  return (targetReached);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static int
travOverRings(
  Ddi_Bdd_t * from0,
  Ddi_Bdd_t * reached0,
  Ddi_Bdd_t * target,
  Tr_Tr_t * TR,
  Ddi_Bdd_t * inRings,
  Ddi_Bdd_t * outRings,
  Ddi_Bdd_t * overApprox,
  int maxIter,
  char *iterMsg,
  long initialTime,
  int reverse,
  int approx,
  Trav_Settings_t * opt
)
{
  int targetReached, i, in_i, iter;
  Ddi_Bdd_t *care, *notReached, *ring, *tmp, *from, *reached, *to;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(from0);
  Tr_Mgr_t *trMgr = Tr_TrMgr(TR);
  Tr_Tr_t *myTr = Tr_TrDup(TR);
  long currTime;

  Pdtutil_Assert(Ddi_BddIsPartDisj(outRings), "Disj PART out rings required");

  /* exact backward */

  targetReached = 0;
  from = Ddi_BddDup(from0);
  if (reached0 != NULL)
    reached = Ddi_BddDup(reached0);
  else
    reached = NULL;

  i = (reverse ? 0 : maxIter - 1);
  Ddi_BddPartWrite(outRings, i, from);
  /*@@@@@ */
#if 0
  tmp = Ddi_BddMakeConst(ddiMgr, 0);
  for (i = Ddi_BddPartNum(inRings) - 1; i >= 0; i--) {
    Ddi_Free(tmp);
    tmp = Ddi_BddAnd(D < di_BddPartRead(inRings, i), target);
    if (!Ddi_BddIsZero(tmp))
      break;
  }
  Ddi_Free(tmp);
#endif

  if ((!approx) && (Ddi_BddIsMono(from) ||
      (Ddi_BddIsPartConj(from) && (Ddi_BddPartNum(from) == 1)))) {
    notReached = Ddi_BddNot(from);
    Ddi_BddSetMono(notReached);
    Ddi_BddSetPartConj(notReached);
  } else {
    notReached = Ddi_BddMakePartConjVoid(ddiMgr);
  }


  for (i = 0; (!Ddi_BddIsZero(from) && (i < maxIter)); i++) {

    iter = (reverse ? maxIter - i - 1 : i);
    in_i = maxIter - i - 1;

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "-- %s iteration: %d.\n", iterMsg, iter));

    care = Ddi_MgrReadOne(ddiMgr);

    if (opt->bdd.constrainLevel >= 1)
      if (inRings != NULL) {
        if (Ddi_BddIsMono(inRings)) {
          care = inRings;
        } else {
          care = Ddi_BddPartRead(inRings, in_i);
        }
        Ddi_BddRestrictAcc(from, care);
      }

    /*Ddi_BddSetMono(from); */

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "|From|=%d\n", Ddi_BddSize(from)));

    if ((!approx) && (opt->bdd.constrainLevel == 1)) {
      Ddi_BddPartWrite(outRings, i, care);
    } else {
      Ddi_BddPartWrite(outRings, i, Ddi_MgrReadOne(ddiMgr));
    }
    ring = Ddi_BddPartRead(outRings, i);

    /*#if PARTITIONED_RINGS */
#if 1
    Ddi_BddSetPartConj(ring);
    Ddi_BddPartInsertLast(ring, from);
#else
    /* can be replaced by from|ring ??? */
    Ddi_BddAndAcc(ring, from);
#endif

    if (overApprox != NULL) {
      Ddi_BddPartInsertLast(ring, Ddi_BddPartRead(overApprox, i));
    }

    if (Ddi_BddIsZero(from))
      continue;

    /* check for intersection with target */
    if (Ddi_BddIsMono(inRings)) {
      tmp = Ddi_BddAnd(inRings, target);
    } else {
      tmp = Ddi_BddAnd(Ddi_BddPartRead(inRings, in_i), target);
    }
    Ddi_BddSetMono(tmp);

    if (!Ddi_BddIsZero(tmp)) {
      /*Ddi_BddSetMono(from); */
      Ddi_BddAndAcc(tmp, from);
      Ddi_BddSetMono(tmp);
      if (overApprox != NULL) {
        Ddi_BddAndAcc(tmp, Ddi_BddPartRead(overApprox, i));
        Ddi_BddSetMono(tmp);
      }
      if (!Ddi_BddIsZero(tmp)) {
        targetReached = 1;
        Ddi_Free(tmp);
        if (!approx) {
          /*#if PARTITIONED_RINGS */
#if 0
          Ddi_BddPartInsert(ring, 0, target);
          Ddi_BddSetMono(ring);
#else
          Ddi_BddAndAcc(ring, target);
#endif
          while (Ddi_BddPartNum(outRings) > (i + 1)) {
            Ddi_Bdd_t *p = Ddi_BddPartExtract(outRings,
              Ddi_BddPartNum(outRings) - 1);

            Ddi_Free(p);
          }
          if (0) {
            int j;

            for (j = 0; j < Ddi_BddPartNum(outRings); j++) {
              Ddi_Bdd_t *p = Ddi_BddPartRead(outRings, j);

              Ddi_BddSetMono(p);
            }
          }
          break;
        }
      }
    }
    Ddi_Free(tmp);

    if (i == maxIter - 1)
      continue;

    Pdtutil_Assert(i < maxIter - 1, "Max Iteration exceeded");

    if (opt->bdd.constrainLevel == 0) {
      tmp = Ddi_BddAnd(care, from);
      if (Ddi_BddSize(tmp) < Ddi_BddSize(from)) {
        Ddi_Free(from);
        from = tmp;
      } else
        Ddi_Free(tmp);
    }

    care = Ddi_BddDup(notReached);
    Ddi_BddPartInsertLast(care, Ddi_BddPartRead(inRings, in_i - 1));

    if (opt->bdd.constrainLevel > 1) {
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      Ddi_BddConstrainAcc(Tr_TrBdd(myTr), care);
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
    }
#if 0
    if (overApprox != NULL) {
#if 1
#if 0
      if (i < Ddi_BddPartNum(overApprox) - 1) {
        Ddi_BddPartInsertLast(care, Ddi_BddPartRead(overApprox, i + 1));
      }
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      toPlus = Tr_ImgWithConstrain(myTr, from, care);
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));

      Ddi_BddPartInsertLast(care, toPlus);
#endif
#if 0
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      tmp = Tr_ImgWithConstrain(myTr, Ddi_BddPartRead(overApprox, i), care);
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));

      Ddi_BddPartInsertLast(care, tmp);

      Ddi_BddSetPartConj(toPlus);
      Ddi_BddPartInsertLast(toPlus, tmp);
      Ddi_Free(tmp);
#endif

#if 0
      Ddi_BddRestrictAcc(Tr_TrBdd(myTr), Ddi_BddPartRead(overApprox, i));
#endif
      if (Ddi_BddIsPartConj(from)) {
        Ddi_BddPartInsertLast(from, Ddi_BddPartRead(overApprox, i));
      } else if (Ddi_BddIsPartConj(Ddi_BddPartRead(overApprox, i))) {
        Ddi_Bdd_t *t = Ddi_BddDup(Ddi_BddPartRead(overApprox, i));

        Ddi_BddPartInsert(t, 0, from);
        Ddi_Free(from);
        from = t;
      } else {
        Ddi_BddAndAcc(from, Ddi_BddPartRead(overApprox, i));
      }

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "|From*OverAppr|=%d\n", Ddi_BddSize(from)));
#else
      Ddi_BddConstrainAcc(Tr_TrBdd(myTr), Ddi_BddPartRead(overApprox, i));
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "\n|TR|overAppr|=%d\n", Ddi_BddSize(Tr_TrBdd(myTr))));
#endif
#if 1
      Ddi_BddRestrictAcc(from, Ddi_BddPartRead(inRings, in_i - 1));
#endif
    }
#endif
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "|TR|=%d\n", Ddi_BddSize(Tr_TrBdd(myTr))));
    if (reverse) {
      to = Trav_ApproxPreimg(myTr, from, care, NULL, 2 * approx, opt);
    } else {
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      to = Tr_ImgWithConstrain(myTr, from, care);
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
    }
    /*Ddi_BddSetMono(to); */

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelDevMin_c,
      LOG_CONST(to, "to")
      );

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMax_c,
      Ddi_Bdd_t * tom = Ddi_BddMakeMono(to);
      Ddi_BddSetMeta(tom); fprintf(stdout, "TO meta: %d\n", Ddi_BddSize(tom));
      Ddi_Free(tom)
      );

    if (0 && approx) {
      /* add initial set so that paths shorter than maxIter
         are taken into account */
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "|To-0|=%d\n", Ddi_BddSize(to)));
      orProjectedSetAcc(to, from0, NULL);
    }

    if (opt->bdd.constrainLevel <= 1) {
      Tr_TrFree(myTr);
      myTr = Tr_TrDup(TR);
    }

    Ddi_Free(from);

    if (opt->bdd.constrainLevel >= 1) {
      Ddi_BddRestrictAcc(to, care);
    }

    if (opt->bdd.constrainLevel > 0) {
      from = Ddi_BddDup(to);
    } else {
      from = Ddi_BddAnd(care, to);
    }

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "|To|=%d.\n", Ddi_BddSize(to)));

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMax_c,
      tmp = Ddi_BddDup(to);
      Ddi_BddSetMono(tmp); fprintf(stdout, "|ToMono|=%d.\n", Ddi_BddSize(tmp));
      Ddi_Free(tmp)
      );

#if 0
    if (overApprox != NULL) {
#if 1
      if (i < Ddi_BddPartNum(overApprox) - 1) {
        Ddi_BddSetPartConj(Ddi_BddPartRead(overApprox, i + 1));
        Ddi_BddPartInsertLast(Ddi_BddPartRead(overApprox, i + 1), toPlus);
      } else
#endif
      {
        Ddi_BddSetPartConj(from);
        Ddi_BddPartInsertLast(from, toPlus);
      }
      Ddi_Free(toPlus);
    }
#endif

#if 0
    if (Ddi_BddSize(from) > Ddi_BddSize(to)) {
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "|To|=%d\n", Ddi_BddSize(to)));
      Ddi_Free(from);
      from = Ddi_BddDup(to);
    } else
#endif
    {
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "|To*BRi|=%d\n", Ddi_BddSize(from)));
    }

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "|To|=%d", Ddi_BddSize(to)));

    if (reached != NULL) {
      Ddi_BddOrAcc(reached, to);

      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "|Reached|=%d; ", Ddi_BddSize(reached));
        fprintf(stdout, "#ReachedStates=%g.\n",
          Ddi_BddCountMinterm(reached, Ddi_VararrayNum(Tr_MgrReadPS(trMgr))))
        );
    }
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "\n"));

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
      currTime = util_cpu_time();
      fprintf(stdout, "Live nodes (peak): %u(%u) - Time: %s.\n",
        Ddi_MgrReadLiveNodes(ddiMgr),
        Ddi_MgrReadPeakLiveNodeCount(ddiMgr),
        util_print_time(currTime - initialTime))
      );

#if 0
    if ((!approx) && (Ddi_BddIsMono(to) ||
        (Ddi_BddIsPartConj(to) && (Ddi_BddPartNum(to) == 1)))) {
      Ddi_BddAndAcc(to, care);
      Ddi_BddNotAcc(to);
      Ddi_BddSetMono(to);
      Ddi_BddPartInsertLast(notReached, to);
    }
#endif

    Ddi_Free(care);
    Ddi_Free(to);
  }

  Tr_TrFree(myTr);
  Ddi_Free(notReached);
  Ddi_Free(reached);
  Ddi_Free(from);

  return (targetReached);
}

/**Function*******************************************************************
  Synopsis    [Check for set intersection.]
  Description [Check for set intersection. Partitioned Bdds and input care
               are supported.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
checkSetIntersect(
  Ddi_Bdd_t * set,
  Ddi_Bdd_t * target,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * care2,
  int doAnd,
  int imgClustTh
)
{
  int intersect = 0, again, myClipTh, myClipLevel;
  Ddi_Bdd_t *targetAux = Ddi_BddDup(target);
  Ddi_Bdd_t *setAux = Ddi_BddDup(set), *myClipWindow;
  Ddi_Varset_t *s0;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(set);
  Pdtutil_VerbLevel_e verbosity = Ddi_MgrReadVerbosity(ddm);

  Ddi_MgrSetOption(ddm, Pdt_DdiVerbosity_c, inum, Pdtutil_VerbLevelUsrMin_c);

  if (doAnd) {
    Ddi_BddAndAcc(targetAux, set);
    if (care != NULL) {
      Ddi_BddAndAcc(targetAux, care);
    }
    if (care2 != NULL) {
      Ddi_BddAndAcc(targetAux, care2);
    }
    Ddi_BddSetMono(targetAux);
  } else {
    Ddi_BddRestrictAcc(setAux, targetAux);
    Ddi_BddRestrictAcc(targetAux, setAux);
    Ddi_BddSetPartConj(targetAux);
    if (Ddi_BddIsPartConj(setAux)) {
      int i, j;

      for (i = 0; i < Ddi_BddPartNum(setAux); i++) {
        j = i * 2;
        if (j >= Ddi_BddPartNum(targetAux)) {
          j = Ddi_BddPartNum(targetAux);
        }
        Ddi_BddPartInsert(targetAux, j, Ddi_BddPartRead(set, i));
      }
    } else {
      Ddi_BddPartInsert(targetAux, 0, setAux);
    }

    if (care != NULL && !Ddi_BddIsOne(care)) {
      Ddi_Bdd_t *careAux = Ddi_BddDup(care);

      Ddi_BddRestrictAcc(targetAux, careAux);
      Ddi_BddRestrictAcc(careAux, targetAux);
      if (Ddi_BddIsZero(careAux)) {
        Ddi_Free(targetAux);
        targetAux = Ddi_BddMakeConst(ddm, 0);
      } else {
        Ddi_BddPartInsert(targetAux, 0, careAux);
      }
      Ddi_Free(careAux);
    }

    if (!Ddi_BddIsZero(targetAux)) {

      if (care2 != NULL && !Ddi_BddIsOne(care2)) {
        Ddi_BddRestrictAcc(targetAux, care2);
        Ddi_BddPartInsert(targetAux, 0, care2);
      }

      s0 = Ddi_BddSupp(targetAux);

      Ddi_BddSetFlattened(targetAux);
      Ddi_BddSetClustered(targetAux, imgClustTh);

      Pdtutil_Assert(ddm->exist.clipDone == 0, "Wrong clipDone setting");
      Pdtutil_Assert(ddm->exist.clipWindow == NULL,
        "Wrong clipWindow setting");

      myClipWindow = Ddi_BddMakeConst(ddm, 1);
      myClipTh = ddm->exist.clipThresh;
      myClipLevel = ddm->exist.clipLevel;

      ddm->exist.clipLevel = 2;

      do {
        Ddi_Bdd_t *newTargetAux;

        ddm->exist.clipDone = again = 0;

        Ddi_BddSetPartConj(targetAux);
        newTargetAux =
          Ddi_BddMultiwayRecursiveAndExist(targetAux, s0, myClipWindow, -1);
        Ddi_BddSetMono(newTargetAux);

        again = ddm->exist.clipDone &&
          (ddm->exist.clipWindow != NULL) && Ddi_BddIsZero(newTargetAux);

        if (again) {
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
              Pdtutil_VerbLevelNone_c, fprintf(stdout, ",")
            ););
          Ddi_BddSetMono(ddm->exist.clipWindow);

          Ddi_BddNotAcc(ddm->exist.clipWindow);
          Ddi_BddAndAcc(myClipWindow, ddm->exist.clipWindow);
          Ddi_Free(newTargetAux);
        } else {
          Ddi_Free(targetAux);
          targetAux = newTargetAux;
        }

        Ddi_Free(ddm->exist.clipWindow);
        ddm->exist.clipWindow = NULL;
        ddm->exist.clipThresh *= 1.5;

      } while (again);

      ddm->exist.clipLevel = myClipLevel;
      ddm->exist.clipThresh = myClipTh;
      Ddi_Free(myClipWindow);

      Ddi_Free(s0);

    }

  }

  intersect = !Ddi_BddIsZero(targetAux);

  Ddi_Free(setAux);
  Ddi_Free(targetAux);

  Ddi_MgrSetOption(ddm, Pdt_DdiVerbosity_c, inum, verbosity);

  return (intersect);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
bckInvarCheckRecur(
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * target,
  Tr_Tr_t * TR,
  Ddi_Bdd_t * rings,
  Trav_Settings_t * opt
)
{
  Ddi_Bdd_t *trace, *tmp, *Rplus;
  Ddi_Bdd_t *careRings;
  Ddi_Bdd_t *care;
  int level = 0;

  opt->ints.saveClipLevel = 2;

  careRings = Ddi_BddDup(rings);
  care = Ddi_BddPartRead(careRings, Ddi_BddPartNum(careRings) - 1);
  Rplus = Ddi_BddDup(Ddi_BddPartRead(rings, Ddi_BddPartNum(rings) - 1));
  Ddi_BddRestrictAcc(Rplus, care);

  while ((level < Ddi_BddPartNum(rings)) && (!checkSetIntersect(from,
        Ddi_BddPartRead(rings, level), Rplus, NULL, 0, opt->tr.imgClustTh))) {
    level++;
  }
  if (level >= Ddi_BddPartNum(rings)) {
    trace = NULL;
  } else {
    if (level < Ddi_BddPartNum(rings) - 1)
      level++;
    trace = bckInvarCheckRecurStep(from, target, rings, Rplus, careRings, TR,
      level, 0 /*nested full */ , 0 /*nested part */ , 0 /*innerFail */ ,
      0 /*nestedCalls */ , 0 /*OK part */ , opt);
  }

  Ddi_Free(careRings);

  if (trace != NULL) {
    /* revert order */
    tmp = trace;
    trace = Ddi_BddPartExtract(tmp, Ddi_BddPartNum(tmp) - 1);
    Ddi_BddSetPartDisj(trace);
    while (Ddi_BddPartNum(tmp) > 0) {
      Ddi_Bdd_t *p = Ddi_BddPartExtract(tmp, Ddi_BddPartNum(tmp) - 1);

      Ddi_BddPartInsertLast(trace, p);
      Ddi_Free(p);
    }
    Ddi_Free(tmp);
  }

  Ddi_Free(Rplus);

  return (trace);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
bckInvarCheckRecurStep(
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * target,
  Ddi_Bdd_t * rings,
  Ddi_Bdd_t * Rplus,
  Ddi_Bdd_t * careRings,
  Tr_Tr_t * TR,
  int level,
  int nestedFull,
  int nestedPart,
  int innerFail,
  int nestedCalls,
  int noPartition,
  Trav_Settings_t * opt
)
{
  Ddi_Bdd_t *trace, *to, *myFrom, *myFrom2, *myFrom3,
    *outer, *inner, *myOuter, *care, *careTot, *outerCare, *tmp, *myClipWindow;
  int nRings = Ddi_BddPartNum(rings);
  int localPart, sameLevel;
  int travAgain, countClip, myClipTh;
  Tr_Mgr_t *trMgr = Tr_TrMgr(TR);
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(from);

  static int callId = 0;
  int id = callId++;

  if (noPartition) {
    care = Ddi_BddPartRead(careRings, level);
  } else {
    care = Ddi_BddPartRead(careRings, Ddi_BddPartNum(careRings) - 1);
  }
  care = Ddi_BddPartRead(careRings, Ddi_BddPartNum(careRings) - 1);
  careTot = Ddi_BddPartRead(careRings, Ddi_BddPartNum(careRings) - 1);


  /* check termination */
  if (checkSetIntersect(from, target, Rplus, care, 1, opt->tr.imgClustTh)) {
    trace = Ddi_BddDup(target);
    Ddi_BddSetPartDisj(trace);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "TARGET REACHED.\n")
      );
    return (trace);
  }
  if (level <= 0) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "DEAD END.\n")
      );
    return (NULL);
  }

  Pdtutil_Assert(level > 0,
    "Innermost level reached in recursive Invar Check");

  Ddi_BddSetClustered(from, opt->tr.imgClustTh);
  Ddi_BddRestrictAcc(from, care);

  fprintf(stdout,
    "rec step %d (|f|=%d,|R+|=%d) (id:%d/if:%d/nc:%d/np:%d/nf:%d).\n", level,
    Ddi_BddSize(from), Ddi_BddSize(Rplus), id, innerFail, nestedCalls,
    nestedPart, nestedFull);
  if (level < opt->bdd.nExactIter || (level == opt->bdd.nExactIter
      && opt->bdd.nExactIter < nRings - 1)) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "EXACT region reached.\n")
      );
  }

  /* generate local from as from intersected with level ring */
  if (level < Ddi_BddPartNum(careRings) - 1) {
    sameLevel = level;
  } else {
    sameLevel = level;
  }

  outer = Ddi_BddPartRead(rings, sameLevel);
  myOuter = Ddi_BddDup(Ddi_BddPartRead(rings, sameLevel));
  outerCare = Ddi_BddPartRead(careRings, sameLevel);
  inner = Ddi_BddDup(Ddi_BddPartRead(rings, level - 1));


  /* exclude myFrom from Rplus */
#if 1
  /* @ $$ @ */
  Ddi_BddSetPartConj(Rplus);
  tmp = Ddi_BddDup(from);
  Ddi_BddSetClustered(tmp, opt->tr.imgClustTh);
  Ddi_BddNotAcc(tmp);
  Ddi_BddRestrictAcc(tmp, care);
  Ddi_BddRestrictAcc(Rplus, tmp);
  Ddi_BddPartInsertLast(Rplus, tmp);
  Ddi_BddSetFlattened(Rplus);
  Ddi_BddSetClustered(Rplus, opt->tr.imgClustTh);
  Ddi_BddRestrictAcc(Rplus, care);
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "|R+|=%d.\n", Ddi_BddSize(Rplus))
    );
  /*Ddi_BddPartInsertLast(inner,tmp); */
  Ddi_Free(tmp);
#endif


#if 1
  if ((level < opt->bdd.nExactIter)
    || ((Ddi_BddSize(from) > opt->bdd.pmcPrioThresh)
      && (nestedFull < opt->bdd.pmcMaxNestedFull)
      && (nestedPart < opt->bdd.pmcMaxNestedPart))) {

    myFrom = Ddi_BddDup(from);
    Ddi_BddRestrictAcc(myFrom, care);
    Ddi_BddRestrictAcc(myFrom, outerCare);


#if 0
    Ddi_BddSetPartConj(inner);
    tmp = Ddi_BddRestrict(Rplus, inner);
    Ddi_BddPartInsertLast(inner, tmp);
    Ddi_BddSetFlattened(inner);
    Ddi_BddSetClustered(inner, opt->tr.imgClustTh);
    Ddi_Free(tmp);
#endif

#if 0
    Ddi_BddSetPartConj(outer);
    tmp = Ddi_BddDup(from);
    Ddi_BddSetClustered(tmp, opt->tr.imgClustTh);
    Ddi_BddNotAcc(tmp);

    Ddi_BddPartInsertLast(outer, tmp);
    Ddi_Free(tmp);
    Ddi_BddSetClustered(outer, opt->tr.imgClustTh);
    Ddi_BddRestrictAcc(outer, care);
#endif

    fprintf(stdout, "L: %d (id: %d) -> inner (|from| = %d).\n", level, id,
      Ddi_BddSize(myFrom));
    fprintf(stdout, "Live nodes (peak): %u(%u).\n",
      Ddi_MgrReadLiveNodes(Ddi_ReadMgr(from)),
      Ddi_MgrReadPeakLiveNodeCount(Ddi_ReadMgr(from)));

    myClipWindow = Ddi_BddMakeConst(Ddi_ReadMgr(from), 1);
    myClipTh = ddm->exist.clipThresh;

    countClip = 0;
    do {
      Ddi_Bdd_t *localFrom;

      ddm->exist.clipDone = travAgain = 0;
#if ENABLE_CONFLICT
      conflictProduct = Ddi_BddMakeConst(ddm, 1);
#endif
      /* try preimg to inner ring */
      to = Trav_ApproxPreimg(TR, myFrom, inner, myClipWindow, 0, opt);

      travAgain = ddm->exist.clipDone && (ddm->exist.clipWindow != NULL);

      localFrom = NULL;
      if (travAgain) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c, fprintf(stdout, "Clip INNER.\n")
          );
        Ddi_BddSetMono(ddm->exist.clipWindow);

        Ddi_BddNotAcc(ddm->exist.clipWindow);
        Ddi_BddAndAcc(myClipWindow, ddm->exist.clipWindow);
        Ddi_Free(ddm->exist.clipWindow);
        ddm->exist.clipWindow = NULL;
        ddm->exist.clipDone = 0;
        countClip++;
      } else if (Ddi_BddIsZero(to)) {
        localFrom = Ddi_BddDup(myFrom);
      }
#if ENABLE_CONFLICT
      if (sameLevel < Ddi_BddPartNum(careRings) - 1) {
        analyzeConflict(opt->ints.auxFwdTr, inner, outer,
          Ddi_BddPartRead(rings, sameLevel + 1), care, localFrom, opt);
      } else {
        analyzeConflict(opt->ints.auxFwdTr, inner, outer, care, care,
          localFrom, opt);
      }
#endif
      Ddi_Free(localFrom);


      if (!Ddi_BddIsZero(to)) {
        Ddi_BddSetPartConj(to);
        ddm->exist.clipThresh = myClipTh;

#if 0
        {
          int i;
          Ddi_Bdd_t *innerPlus;
          Ddi_Varset_t *smooth = Ddi_BddSupp(inner);
          Ddi_Varset_t *supp = Ddi_BddSupp(to);

          Ddi_VarsetDiffAcc(smooth, supp);
          innerPlus = Ddi_BddDup(inner);
          if (Ddi_BddIsPartDisj(innerPlus)) {
            for (i = 0; i < Ddi_BddPartNum(innerPlus); i++) {
              Ddi_Bdd_t *p = Ddi_BddPartRead(innerPlus, i);

              if (Ddi_BddIsPartConj(p)) {
                int j;

                for (j = 0; j < Ddi_BddPartNum(p); j++) {
                  Ddi_BddExistAcc(Ddi_BddPartRead(p, j), smooth);
                }
              } else {
                Ddi_BddExistAcc(p, smooth);
              }
            }
          } else if (Ddi_BddIsPartConj(innerPlus)) {
            for (i = 0; i < Ddi_BddPartNum(innerPlus); i++) {
              Ddi_BddExistAcc(Ddi_BddPartRead(innerPlus, i), smooth);
            }
          } else {
            Ddi_BddExistAcc(innerPlus, smooth);
          }
          fprintf(stdout, "[%d]|%d|=>|%d|.\n", Ddi_VarsetNum(smooth),
            Ddi_BddSize(inner), Ddi_BddSize(innerPlus));
          Ddi_Free(supp);
          Ddi_Free(smooth);

#if 0
          Ddi_BddRestrictAcc(to, innerPlus);
          Ddi_BddRestrictAcc(innerPlus, to);
          Ddi_BddPartInsertLast(to, innerPlus);
          Ddi_BddSetFlattened(to);
#endif
          Ddi_Free(innerPlus);
        }
#else
        {
          Ddi_Bdd_t *innerPlus = Ddi_BddDup(inner);

          Ddi_BddRestrictAcc(to, innerPlus);
          Ddi_BddRestrictAcc(innerPlus, to);
          Ddi_BddPartInsertLast(to, innerPlus);
          Ddi_BddSetFlattened(to);
          Ddi_Free(innerPlus);
        }
#endif

        trace = bckInvarCheckRecurStep(to, target, rings, Rplus, careRings, TR,
          level - 1, nestedFull, nestedPart + 1, innerFail,
          nestedCalls + 1, 0 /*no part */ , opt);
        Ddi_Free(to);
        if (trace != NULL) {
          Ddi_BddPartInsertLast(trace, myFrom);
          Ddi_Free(myFrom);
          Ddi_Free(myOuter);
          Ddi_Free(myClipWindow);
          Ddi_Free(inner);
          return (trace);
        }
      }
      Ddi_Free(to);
      ddm->exist.clipThresh *= 1.5;

    } while (travAgain && countClip < 3);

    ddm->exist.clipThresh = myClipTh;
    Ddi_Free(myClipWindow);
    Ddi_Free(myFrom);

    if (noPartition) {
      Ddi_Free(myOuter);
      Ddi_Free(inner);
      return (NULL);
    }

    innerFail++;

#endif

  }

  Ddi_Free(inner);


  myFrom = Ddi_BddDup(from);
  myFrom2 = Ddi_BddDup(from);


#if 0
  if ((Ddi_BddSize(from) > opt->bdd.pmcPrioThresh) &&
    (nestedPart < opt->bdd.pmcMaxNestedPart)) {
    /* take inner partition of myFrom */
    Ddi_BddSetPartConj(myFrom);
#if 1
    {
      int i;
      Ddi_Bdd_t *outerPlus;
      Ddi_Varset_t *smooth = Ddi_BddSupp(myOuter);
      Ddi_Varset_t *supp = Ddi_BddSupp(myFrom);

      Ddi_VarsetDiffAcc(smooth, supp);
      outerPlus = Ddi_BddDup(myOuter);
      if (Ddi_BddIsPartConj(outerPlus)) {
        for (i = 0; i < Ddi_BddPartNum(outerPlus); i++) {
          Ddi_BddExistAcc(Ddi_BddPartRead(outerPlus, i), smooth);
        }
      }
      fprintf(stdout, "(%d)|%d|=>|%d|.\n", Ddi_VarsetNum(smooth),
        Ddi_BddSize(myOuter), Ddi_BddSize(outerPlus));
      Ddi_Free(supp);
      Ddi_Free(smooth);
      Ddi_BddPartInsertLast(myFrom, outerPlus);
      Ddi_Free(outerPlus);
    }
#else
    Ddi_BddPartInsertLast(myFrom, myOuter);
#endif
    localPart = 1;

    tmp = Ddi_BddDup(myOuter);
    Ddi_BddSetClustered(tmp, opt->tr.imgClustTh);
    Ddi_BddNotAcc(tmp);
    Ddi_BddSetPartConj(myFrom2);
    Ddi_BddPartInsertLast(myFrom2, tmp);
    Ddi_Free(tmp);
    Ddi_BddSetClustered(myOuter, opt->tr.imgClustTh);
    Ddi_BddRestrictAcc(myOuter, care);

  } else {
    localPart = 0;
  }
#else
  localPart = 0;
#endif

  Ddi_Free(myOuter);


  Ddi_BddRestrictAcc(myFrom, care);
  Ddi_BddRestrictAcc(myFrom2, care);

  if ((!opt->bdd.pmcNoSame) && (nestedFull < opt->bdd.pmcMaxNestedFull) &&
    (Ddi_BddSize(from) > opt->bdd.pmcPrioThresh) &&
    (level < Ddi_BddPartNum(careRings) - 1)) {

    fprintf(stdout, "L: %d (id: %d) -> same(%d) (|from| = %d).\n", level, id,
      localPart, Ddi_BddSize(myFrom));

    /* try same ring */

    myClipWindow = Ddi_BddMakeConst(Ddi_ReadMgr(from), 1);
    if (sameLevel < Ddi_BddPartNum(careRings) - 1) {
      sameLevel++;
    }
    myClipTh = ddm->exist.clipThresh;

    countClip = 0;
    do {

      ddm->exist.clipDone = travAgain = 0;

      myFrom3 = Ddi_BddDup(myFrom);
      myOuter = Ddi_BddDup(outer);
      Ddi_BddSetPartConj(myOuter);
      Ddi_BddPartInsertLast(myOuter, care);
#if 0
      Ddi_BddNotAcc(myFrom);
      Ddi_BddPartInsertLast(myOuter, myFrom);
      Ddi_BddNotAcc(myFrom);
#else
      /* @ $$ @ */
      Ddi_BddPartInsertLast(myOuter, Rplus);
      Ddi_BddSetFlattened(myOuter);
#endif

      if (sameLevel < Ddi_BddPartNum(careRings) - 1) {
        outerCare = Ddi_BddPartRead(careRings, sameLevel + 1);
        Ddi_BddRestrictAcc(myFrom3, care);
        Ddi_BddRestrictAcc(myFrom3, outerCare);
      }


      if (countClip > 0) {
        opt->bdd.optPreimg = 1000;
      }
      to = Trav_ApproxPreimg(TR, myFrom3, myOuter, myClipWindow, 0, opt);
      if (countClip > 0) {
        opt->bdd.optPreimg = 0;
      }
      Ddi_BddSetFlattened(to);

      travAgain = ddm->exist.clipDone && (ddm->exist.clipWindow != NULL);

      if (travAgain) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c, fprintf(stdout, "Clip SAME.\n")
          );
        Ddi_BddSetMono(ddm->exist.clipWindow);

        Ddi_BddNotAcc(ddm->exist.clipWindow);
        Ddi_BddAndAcc(myClipWindow, ddm->exist.clipWindow);
        Ddi_Free(ddm->exist.clipWindow);
        ddm->exist.clipWindow = NULL;
        ddm->exist.clipDone = 0;
        countClip++;
      }

      Ddi_Free(myOuter);
      Ddi_Free(myFrom3);

      if (!Ddi_BddIsZero(to)) {
        Ddi_BddSetPartConj(to);
        tmp = Ddi_BddRestrict(outer, to);
        Ddi_BddRestrictAcc(to, tmp);
        Ddi_BddPartInsertLast(to, outer);
        Ddi_BddSetFlattened(to);
        Ddi_Free(tmp);
        ddm->exist.clipThresh = myClipTh;

        trace = bckInvarCheckRecurStep(to, target, rings, Rplus, careRings, TR,
          sameLevel, nestedFull, nestedPart + localPart, innerFail,
          nestedCalls + 1, 0 /*no part */ , opt);
        Ddi_Free(to);
        if (trace != NULL) {
          Ddi_BddPartInsertLast(trace, myFrom);
          Ddi_Free(myFrom);
          Ddi_Free(myFrom2);
          Ddi_Free(myClipWindow);
          return (trace);
        }
      }

      Ddi_Free(to);
      ddm->exist.clipThresh *= 1.5;

    } while (travAgain && countClip < 2);

    ddm->exist.clipThresh = myClipTh;

    Ddi_Free(myClipWindow);

  }

  fprintf(stdout, "L: %d (id: %d) -> full(%d) (|from| = %d).\n", level, id,
    localPart, Ddi_BddSize(myFrom));
  fprintf(stdout, "Live nodes (peak): %u(%u).\n",
    Ddi_MgrReadLiveNodes(Ddi_ReadMgr(from)),
    Ddi_MgrReadPeakLiveNodeCount(Ddi_ReadMgr(from)));

  /* try preimg to full Rplus */

  myOuter = Ddi_BddDup(Rplus);
  Ddi_BddSetPartConj(myOuter);
  Ddi_BddPartInsertLast(myOuter, care);
  myClipWindow = Ddi_BddMakeConst(Ddi_ReadMgr(from), 1);
  myClipTh = ddm->exist.clipThresh;

  countClip = 0;
  do {
    Ddi_Bdd_t *localFrom;

    ddm->exist.clipDone = travAgain = 0;

    Pdtutil_Assert(ddm->exist.conflictProduct == NULL,
      "non NULL conflict product !");
#if ENABLE_CONFLICT
    ddm->exist.conflictProduct = Ddi_BddMakeConst(ddm, 1);
#endif

#if 1
    if (countClip > 0) {
      opt->bdd.optPreimg = 1000;
    }
    {
      Ddi_Varset_t *supp = Ddi_BddSupp(myFrom);
      int doApprox = (Ddi_BddSize(myFrom) > 1000 && Ddi_VarsetNum(supp) > 200);

      Ddi_Free(supp);
      to = Trav_ApproxPreimg(TR, myFrom, care, myClipWindow, doApprox, opt);
    }
    if (countClip > 0) {
      opt->bdd.optPreimg = 0;
    }
    Ddi_BddRestrictAcc(to, myOuter);
#else
    to = Trav_ApproxPreimg(TR, myFrom, myOuter, myClipWindow, 0, opt);
#endif


    travAgain = ddm->exist.clipDone && (ddm->exist.clipWindow != NULL);

    localFrom = NULL;
    if (travAgain) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "Clip FULL.\n")
        );
      Ddi_BddSetMono(ddm->exist.clipWindow);

      if (0 && Ddi_BddIsZero(to)) {
        Ddi_Bdd_t *auxTo;
        int auxClipLevel = opt->ints.saveClipLevel;

        ddm->exist.clipLevel = opt->ints.saveClipLevel = 0;
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c, fprintf(stdout, "Trying clipWindow.\n")
          );
        auxTo = Trav_ApproxPreimg(TR, Ddi_MgrReadOne(Ddi_ReadMgr(from)),
          myOuter, ddm->exist.clipWindow, 0, opt);
        if (Ddi_BddIsZero(auxTo)) {
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c, fprintf(stdout, "Found Overapprox.\n")
            );
        }
        Ddi_Free(auxTo);
        opt->ints.saveClipLevel = auxClipLevel;
      }

      Ddi_BddNotAcc(ddm->exist.clipWindow);
      Ddi_BddAndAcc(myClipWindow, ddm->exist.clipWindow);
      Ddi_Free(ddm->exist.clipWindow);
      ddm->exist.clipWindow = NULL;
      ddm->exist.clipDone = 0;
      countClip++;
    } else if (Ddi_BddIsZero(to)) {
      localFrom = Ddi_BddDup(myFrom);
    }
#if ENABLE_CONFLICT
    analyzeConflict(opt->ints.auxFwdTr, NULL, care, care, care, localFrom,
      opt);
#endif
    Ddi_Free(localFrom);

    if (!Ddi_BddIsZero(to)) {
      int myLevel;

      ddm->exist.clipThresh = myClipTh;

      myLevel = (level >= 2) ? level - 2 : 0;
      while (myLevel < nRings) {
        if (Ddi_BddIsOne(Ddi_BddPartRead(rings, myLevel)) ||
          checkSetIntersect(to, Ddi_BddPartRead(rings, myLevel),
            Rplus, care, 0, opt->tr.imgClustTh)) {
          break;
        }
        myLevel++;
      }
      if (myLevel >= nRings) {
        trace = NULL;
      } else {
        if (myLevel < nRings - 1)
          /*myLevel++ */ ;
#if 1
        myLevel += innerFail;
        if (myLevel >= nRings) {
          myLevel = nRings - 1;
        }
#endif
#if 0
        Ddi_BddSetPartConj(to);
        Ddi_BddPartInsertLast(to, Rplus);
        Ddi_BddRestrictAcc(to, care);
#endif

        trace = bckInvarCheckRecurStep(to, target, rings, Rplus, careRings, TR,
          myLevel, nestedFull + 1, nestedPart + localPart, innerFail,
          nestedCalls + 1, 0 /*part OK */ , opt);
      }
      if (trace != NULL) {
        Ddi_BddPartInsertLast(trace, myFrom);
        Ddi_Free(myFrom);
        Ddi_Free(myFrom2);
        Ddi_Free(myOuter);
        Ddi_Free(myClipWindow);
        Ddi_Free(to);
        return (trace);
      }
    }

    Ddi_Free(to);
    ddm->exist.clipThresh *= 1.5;

  } while (travAgain /* && countClip < 2 */ );

  if (travAgain) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "FULL trav ABORTED.\n")
      );
  }

  ddm->exist.clipThresh = myClipTh;

  Ddi_Free(myClipWindow);
  Ddi_Free(myFrom);
  Ddi_Free(myOuter);


#if 1
  if (localPart == 1) {

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "L: %d (id: %d) -> outer.\n", level, id)
      );

    /* try from outer ring */
    if (!noPartition) {
      if (level < nRings - 1) {
        int outerLevel = level + /* 1 */ (nRings - level - 1) / 2;

        if (outerLevel == level)
          outerLevel++;
        trace =
          bckInvarCheckRecurStep(myFrom2, target, rings, Rplus, careRings, TR,
          outerLevel, nestedFull, nestedPart, innerFail, nestedCalls + 1,
          0 /*part OK */ , opt);
        Ddi_Free(myFrom2);
        return (trace);
      }
    }
  }
#endif
  Ddi_Free(myFrom2);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "L: %d (id: %d) -> NULL.\n", level, id)
    );

  return (NULL);

}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
analyzeConflict(
  Tr_Tr_t * TR,
  Ddi_Bdd_t * inner,
  Ddi_Bdd_t * same,
  Ddi_Bdd_t * outer,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * from,
  Trav_Settings_t * opt
)
{
  int i, full;
  Ddi_Varset_t *ps, *pi, *smooth, *supp, *supp_i;
  Tr_Mgr_t *trMgr = Tr_TrMgr(TR);
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(Tr_TrBdd(TR));
  int clk_phase = 0;

  Ddi_Bdd_t *auxTo, *prod, *myConflict;
  int auxClipLevel = opt->ints.saveClipLevel, propagateConflict = 0;
  int saveClipTh = ddm->exist.clipThresh;

  if ((ddm->exist.conflictProduct != NULL) &&
    !Ddi_BddIsOne(ddm->exist.conflictProduct)) {

    myConflict = Ddi_BddDup(ddm->exist.conflictProduct);

    if (!Ddi_BddIsConstant(myConflict) && (outer != NULL)) {

      ddm->exist.clipLevel = opt->ints.saveClipLevel = 0;

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Trying conflictProduct (%d).\n",
          Ddi_BddSize(myConflict))
        );

      /* take complement of conflict product */

      if (Ddi_BddIsPartDisj(ddm->exist.conflictProduct)) {
        /* partitioned NOT */
        Pdtutil_Assert(Ddi_BddPartNum(myConflict) == 2,
          "unsupported disj part in conflict product");
        for (i = 0; i < Ddi_BddPartNum(myConflict); i++) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(myConflict, i);
          Ddi_Bdd_t *clk_lit = Ddi_BddPartExtract(p, 0);

          Ddi_BddNotAcc(p);
          if (!Ddi_BddIsOne(p)) {
            propagateConflict = 1;
          }
          Ddi_BddSetPartConj(p);
          Ddi_BddPartInsert(p, 0, clk_lit);
          Ddi_Free(clk_lit);
        }

      } else {
        Ddi_BddNotAcc(myConflict);
      }

      if (propagateConflict) {
        ddm->exist.clipThresh = 1000000;
        auxTo = Tr_ImgApprox(TR, myConflict, NULL);

        ddm->exist.clipThresh = saveClipTh;

        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Conflict set: %d.\n", Ddi_BddSize(auxTo))
          );

        Ddi_BddRestrictAcc(auxTo, care);
        Ddi_BddSetPartConj(same);
        Ddi_BddRestrictAcc(auxTo, same);
        Ddi_BddRestrictAcc(same, auxTo);
        Ddi_BddPartInsert(same, 0, auxTo);
        Ddi_BddSetFlattened(same);
        Ddi_BddSetClustered(same, opt->tr.imgClustTh);
        Ddi_Free(auxTo);
      }
    }

    Ddi_Free(myConflict);

  }


  if ((outer != NULL) && (from != NULL) && !Ddi_BddIsZero(from)) {
    ddm->exist.clipThresh = 1000000;
    Ddi_BddNotAcc(from);
    auxTo = Tr_ImgApprox(TR, from, NULL);
    Ddi_BddSetPartConj(auxTo);
    if (Ddi_BddIsPartDisj(from) && (Ddi_BddPartNum(from) == 1)) {
      Ddi_BddPartInsertLast(auxTo, Ddi_BddPartRead(from, 0));
    } else if (!Ddi_BddIsPartDisj(from)) {
      Ddi_BddPartInsertLast(auxTo, from);
    }
    Ddi_BddNotAcc(from);

    ddm->exist.clipThresh = saveClipTh;

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Conflict outer set: %d ", Ddi_BddSize(auxTo))
      );

    Ddi_BddRestrictAcc(auxTo, care);
    Ddi_BddSetPartConj(outer);
    Ddi_BddRestrictAcc(auxTo, outer);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "-> %d.\n", Ddi_BddSize(auxTo))
      );
    Ddi_BddRestrictAcc(outer, auxTo);
    Ddi_BddPartInsert(outer, 0, auxTo);
    Ddi_BddSetFlattened(outer);
    Ddi_BddSetClustered(outer, opt->tr.imgClustTh);
    Ddi_Free(auxTo);
  }

  opt->ints.saveClipLevel = ddm->exist.clipLevel = auxClipLevel;
  Ddi_Free(ddm->exist.conflictProduct);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static void
storeCNFRings(
  Ddi_Bdd_t * fwdRings,
  Ddi_Bdd_t * bckRings,
  Ddi_Bdd_t * TrBdd,
  Ddi_Bdd_t * prop,
  Trav_Settings_t * opt
)
{
  int i, j, k;
  Ddi_Bdd_t *fp, *bp, *localTrBdd, *p;
  int numpart;

  if (prop != NULL) {
    Ddi_BddSetMono(prop);
  } else {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "***Prop = NULL!!\n"));
  }

  if (opt->bdd.storeCNFphase == 0) {
    switch (opt->bdd.storeCNFTR) {
      case -1:
        break;
      case 0:
        numpart = Ddi_BddPartNum(fwdRings);
        for (i = numpart - 1; i >= 0; i--) {
          fp = Ddi_BddPartRead(fwdRings, i);
          Ddi_BddSetPartConj(fp);

          if (i > 0) {
            while (Ddi_BddPartNum(fp) > 0) {
              p = Ddi_BddPartExtract(fp, 0);
              Ddi_Free(p);
            }
          }
          if (i == numpart - 1) {
            Ddi_BddPartInsertLast(fp, prop);
          }

          if (i < numpart - 1) {
            for (k = 0; k < Ddi_BddPartNum(TrBdd); k++) {
              Ddi_BddPartInsertLast(fp, Ddi_BddPartRead(TrBdd, k));
            }
          }

        }
        break;
      case 1:
        numpart = Ddi_BddPartNum(fwdRings);
        for (i = numpart - 1; i >= 0; i--) {
          fp = Ddi_BddPartRead(fwdRings, i);
          Ddi_BddSetPartConj(fp);
          if (i == numpart - 1) {
            Ddi_BddPartInsertLast(fp, prop);
          }
          if (i < numpart - 1) {

            localTrBdd = Ddi_BddRestrict(TrBdd, fp);
            for (k = 0; k < Ddi_BddPartNum(localTrBdd); k++) {
              Ddi_BddPartInsertLast(fp, Ddi_BddPartRead(localTrBdd, k));
            }
            Ddi_Free(localTrBdd);
          }
        }
        break;
    }
    storeRingsCNF(fwdRings, opt);
    return;
  }

  /* NOTICE: the following code assumes that the   */
  /*         property is stored in bckRings[0].    */
  /*         this is NOT the case when a property  */
  /*         passes just with a BDD-analisys       */
  if (opt->bdd.storeCNFphase == 1) {
    switch (opt->bdd.storeCNFTR) {
      case -1:
        numpart = Ddi_BddPartNum(fwdRings);
        for (i = numpart - 1, j = 0; i >= 0; j++, i--) {
          fp = Ddi_BddPartRead(fwdRings, i);
          bp = Ddi_BddPartRead(bckRings, j);
          Ddi_BddSetPartConj(fp);
          Ddi_BddSetMono(bp);
          Ddi_BddPartInsertLast(fp, bp);
        }
        break;
      case 0:
        numpart = Ddi_BddPartNum(fwdRings);
        for (i = numpart - 1, j = 0; i >= 0; j++, i--) {
          fp = Ddi_BddPartRead(fwdRings, i);
          Ddi_BddSetPartConj(fp);
          if (i > 0) {
            while (Ddi_BddPartNum(fp) > 0) {
              p = Ddi_BddPartExtract(fp, 0);
              Ddi_Free(p);
            }
          }

          if (i == numpart - 1) {
            Ddi_BddPartInsertLast(fp, prop);
          }

          if (i < numpart - 1) {
            localTrBdd = TrBdd;
            for (k = 0; k < Ddi_BddPartNum(localTrBdd); k++) {
              Ddi_BddPartInsertLast(fp, Ddi_BddPartRead(localTrBdd, k));
            }
          }

        }
        break;
      case 1:
        for (i = Ddi_BddPartNum(fwdRings) - 1, j = 0; i >= 0; j++, i--) {
          fp = Ddi_BddPartRead(fwdRings, i);
          bp = Ddi_BddPartRead(bckRings, j);
          Ddi_BddSetPartConj(fp);
          Ddi_BddSetMono(bp);
          Ddi_BddPartInsertLast(fp, bp);
          if (i < Ddi_BddPartNum(fwdRings) - 1) {

            localTrBdd = Ddi_BddRestrict(TrBdd, fp);

            for (k = 0; k < Ddi_BddPartNum(localTrBdd); k++) {
              Ddi_BddPartInsertLast(fp, Ddi_BddPartRead(localTrBdd, k));
            }
            Ddi_Free(localTrBdd);
          }
        }
        break;
    }
    storeRingsCNF(fwdRings, opt);
    return;
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static void
storeRingsCNF(
  Ddi_Bdd_t * rings,
  Trav_Settings_t * opt
)
{
  int i, *vauxIds;
  char fname[100], **vnames;
  Ddi_Bdd_t *myRings;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(rings);
  int addedClauses, addedVars;

#if DOUBLE_MGR
  Ddi_Mgr_t *ddiMgr2 = Ddi_MgrDup(ddiMgr);

  Ddi_MgrSiftEnable(ddiMgr2, Ddi_ReorderingMethodString2Enum("sift"));
  Ddi_MgrSetSiftThresh(ddiMgr2, Ddi_MgrReadLiveNodes(ddiMgr));

  myRings = Ddi_BddCopy(ddiMgr2, rings);
  Ddi_MgrReduceHeap(ddiMgr2, Ddi_ReorderingMethodString2Enum("sift"), 0);

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "|rp| (original)  : %5d\n", Ddi_BddSize(rings));
    fprintf(stdout, "|rp| (sifted)  : %5d\n\n", Ddi_BddSize(myRings))
    );

  vauxIds = Ddi_MgrReadVarauxids(ddiMgr2);
  vnames = Ddi_MgrReadVarnames(ddiMgr2);
#else
  myRings = Ddi_BddDup(rings);
  vauxIds = Ddi_MgrReadVarauxids(ddiMgr);
  vnames = Ddi_MgrReadVarnames(ddiMgr);
#endif

  for (i = 0; i < Ddi_BddPartNum(myRings); i++) {
    Ddi_Bdd_t *p = Ddi_BddPartRead(myRings, i);

    sprintf(fname, "%sRP%d.cnf", opt->bdd.storeCNF, i);
    switch (opt->bdd.storeCNFmode) {
      case 'N':
        Ddi_BddStoreCnf(p, DDDMP_CNF_MODE_NODE, 0, vnames, NULL,
          vauxIds, NULL, -1, -1, -1, fname, NULL, &addedClauses, &addedVars);
        break;
      case 'M':
        Ddi_BddStoreCnf(p, DDDMP_CNF_MODE_MAXTERM, 0, vnames, NULL,
          vauxIds, NULL, -1, -1, -1, fname, NULL, &addedClauses, &addedVars);
        break;
      default:
        Ddi_BddStoreCnf(p, DDDMP_CNF_MODE_BEST, 0, vnames, NULL,
          vauxIds, NULL, -1, 1, opt->bdd.maxCNFLength, fname, NULL,
          &addedClauses, &addedVars);
    }
#if 0
    sprintf(fname, "%sRP%d.bdd", opt->bdd.storeCNF, i);
    Ddi_BddStore(p, NULL, DDDMP_MODE_TEXT, fname, NULL);
#endif
  }

  Ddi_Free(myRings);

#if DOUBLE_MGR
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "-- DDI Manager Free.\n"));
  Ddi_MgrQuit(ddiMgr2);
#endif
}

/**CFile***********************************************************************

  FileName    [travSim.c]

  PackageName [trav]

  Synopsis    [Main module for the symboic simulation]

  Description [External procedure included in this file is:
    <ul>
    <li>()
    </ul>
    ]

  SeeAlso     []

  Author      [Valeria Bertacco and Maurizio Damiani and Stefano Quer]

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
#include "fbv.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define NETLIST_DEBUG_1 0
#define NETLIST_DEBUG_2 0
#define USE_TRAV_OR_DIRECTMETHOD 0

// do not change the value for UNSET (memset counts on it)
#define UNSET   -1

#define SIMPLE  -2
#define COMPLEX -3
#define BOUND   -4
#define SHARED  -5

/* Limits for the simulationWaveArray */
#define N_SIGNAL 1000
#define N_CLOCK_TICK 100

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

struct table_e {
  Ddi_Bdd_t *f;
  int n;
};

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

static void printWave(
  FILE * fp,
  char presentValue,
  char nextValue
);
static int reduceNextStateFunctions(
  Ddi_Mgr_t * ddMgr,
  Pdtutil_VerbLevel_e verbosity,
  int depthBreadth,
  int random,
  Ddi_Bddarray_t * delta,
  Ddi_Bdd_t * from,
  int piNumber,
  int psNumber,
  Ddi_Varset_t * psVarset,
  Ddi_Vararray_t * psVararray,
  Ddi_Vararray_t * nsVararray,
  Ddi_Bdd_t ** relationPsPiPs
);
static void constrainWith1(
  Ddi_Mgr_t * ddMgr,
  Ddi_Bddarray_t * delta,
  Ddi_Varset_t * suppIn,
  Ddi_Bdd_t * constrainIn,
  Ddi_Bdd_t ** constrainOut1,
  Ddi_Bdd_t ** constrainOut2
);
static void constrainWith2(
  Ddi_Mgr_t * ddMgr,
  Ddi_Bddarray_t * delta,
  Ddi_Varset_t * suppIn,
  Ddi_Bdd_t ** constrainOut
);
static void constrainWithAllRandomValue(
  Ddi_Mgr_t * ddMgr,
  Ddi_Bddarray_t * delta,
  Ddi_Varset_t * supp,
  Ddi_Bdd_t ** constrainOut
);
static void constrainWithPartialRandomValue(
  Ddi_Mgr_t * ddMgr,
  Ddi_Bddarray_t * delta,
  Ddi_Varset_t * supp,
  Ddi_Bdd_t ** constrain
);
static Ddi_Bdd_t *SimFromCompute(
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * to,
  Ddi_Bdd_t * relation,
  int depthBreadth
);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Temporary main program to be used as interface for cmd.]

  Description [If simplifyDelta is equal to 0 performs a standard
    simulation otherwise applies the Bertacco, Damiani, Quer DAC'99
    strategy.
  ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

void
Trav_SimulateMain(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  int iterNumberMax /* Maximum Number of Iterations */ ,
  int deadEndNumberOf /* Maximum Number of Dead End */ ,
  int logPeriod /* Period to Print Out Informations */ ,
  int simulationFlag /* Enable Simulation with Waves or DAC'99 */ ,
  int depthBreadth /* In DAC'99 Method Use Breadth(1) or Depth(0) */ ,
  int random /* In DAC'99 Use Random Values */ ,
  char *init /* Specify Where to Get Initial State */ ,
  char *pattern /* Specify Where to Get Input Patterns */ ,
  char *result                  /* Specify Where to Put Output Results */
)
{
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "-- Simulation Begin.\n")
    );

  switch (simulationFlag) {
    case 0:
      simulateSimple(fsmMgr, iterNumberMax, logPeriod, pattern, result);
      break;
    case 1:
    case 2:
      simulateWave(fsmMgr, iterNumberMax, logPeriod, simulationFlag,
        init, pattern, result);
      break;
    case 3:
      simulateWithDac99(fsmMgr, iterNumberMax, deadEndNumberOf, logPeriod,
        depthBreadth, random, pattern, result);
      break;
    default:
      Pdtutil_Warning(1, "Wrong Choice for Simulate Routine.");
      break;
  }

  return;
}

/**Function********************************************************************

  Synopsis    [Temporary main program to be used as interface for cmd.]

  Description [If simplifyDelta is equal to 0 performs a standard
    simulation otherwise applies the Bertacco, Damiani, Quer DAC'99
    strategy.
  ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int
Trav_SimulateRarity(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr            /* FSM Manager */
)
{
  int ret = 0;
  Fsm_Fsm_t *fsmFsm = Fsm_FsmMakeFromFsmMgr(fsmMgr);

  Ddi_Bdd_t *cex = Fsm_FsmSimRarity(fsmFsm);

  if (cex != NULL) {
    Fsm_MgrSetCexBDD(fsmMgr, cex);
    //    int res = Fbv_CheckFsmCex (fsmMgr,travMgr);
    int res = 1;                // SAT check disabled as slow 

    if (res == 0) {
      printf("RARITY: cex validation failure\n");
      Fsm_MgrSetCexBDD(fsmMgr, NULL);
    } else {
      Trav_MgrSetAssertFlag(travMgr, 1);
      ret = 1;
    }
  }

  Fsm_FsmFree(fsmFsm);

  return ret;
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Simulation Engine for PdTrav]

  Description [
    If pattern is "random" the initial state set is used and input
    patterns are randomly generated. If pattern is not "random" it specifies
    the input file.
    The format of this file is the following:
    <ol>
    <li> initial state variables names
    <li> initial state (one row of 0, 1),
    <li> input variables names
    <li> input patterns (a certain number of row of 0, 1).
    </ol>
    # identifies comments.
    If results is "stdout" output is printed on standad output. Otherwise
    it specifies the name of a file.
    The format is:
    <ol>
    <li> present state variable names, input variables names, output
         variable names, next state variable names
    <li> one row for each transition in a 'kiss' format
         preset state, input, output, next states.
    </ol>
    Again # specifies comments.
    ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

void
simulateSimple(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  int iterNumberMax /* Maximum Number of Iterations */ ,
  int logPeriod /* Period to Print Out Informations */ ,
  char *pattern /* Specify Where to Get Input Patterns */ ,
  char *result                  /* Specify Where to Put Output Results */
)
{
  FILE *fpPattern, *fpResult;
  char row[PDTUTIL_MAX_STR_LEN];
  int i, j, flagFilePattern, flagFileResult,
    step, piNumber, psNumber, poNumber, suppSize;
  long startTime, currTime;
  Pdtutil_VerbLevel_e verbosity;
  Ddi_Mgr_t *ddMgr;
  Ddi_Bdd_t *f, *bddResult;
  Ddi_Bddarray_t *delta, *lambda;
  Ddi_Vararray_t *psVararray, *nsVararray;
  int *auxidArray, *piIndex, *stateIndex, *presentCycleArray,
    *nextCycleArray, *tmpArray, *outputArray, simContinue;
  char **nameArray;
  Ddi_Bdd_t *one;

  /*------------------------------ Init Process -----------------------------*/

  startTime = util_cpu_time();

  verbosity = Fsm_MgrReadVerbosity(fsmMgr);
  ddMgr = Fsm_MgrReadDdManager(fsmMgr);

  piNumber = Fsm_MgrReadNI(fsmMgr);
  psNumber = Fsm_MgrReadNL(fsmMgr);
  poNumber = Fsm_MgrReadNO(fsmMgr);

  psVararray = Fsm_MgrReadVarPS(fsmMgr);
  nsVararray = Fsm_MgrReadVarNS(fsmMgr);
  auxidArray = Ddi_MgrReadVarauxids(ddMgr);
  nameArray = Ddi_MgrReadVarnames(ddMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  lambda = Fsm_MgrReadLambdaBDD(fsmMgr);

  suppSize = Ddi_MgrReadNumCuddVars(ddMgr);
  one = Ddi_MgrReadOne(ddMgr);

  presentCycleArray = Pdtutil_Alloc(int,
    suppSize
  );
  nextCycleArray = Pdtutil_Alloc(int,
    suppSize
  );
  outputArray = Pdtutil_Alloc(int,
    poNumber
  );
  piIndex = Pdtutil_Alloc(int,
    piNumber
  );
  stateIndex = Pdtutil_Alloc(int,
    psNumber
  );

  /*------------------------------  Input File ------------------------------*/

  /*
     if (strcmp (pattern, "random") == 0) {
     fpPattern = NULL;
     } else {
   */
  fpPattern = Pdtutil_FileOpen(NULL, pattern, "r", &flagFilePattern);
  if (fpPattern == NULL) {
    Pdtutil_Warning(1, "Input Pattern File Not Present.");
    return;
  }

  /*--------------- Read Present State and Primary Input Variables ----------*/

  /*
   * PROBLEMA
   * vedere gestione name  anche in simulate
   */

  i = 0;
  while (i < psNumber) {
    fscanf(fpPattern, "%s", row);
    for (j = 0; j < suppSize; j++) {
      if (strcmp(nameArray[j], row) == 0) {
        stateIndex[i] = j;
        break;
      }
    }
    i++;
  }

  /* lettura del pattern = s0 */
  fscanf(fpPattern, "%s", row);

  for (i = 0; i < psNumber; i++) {
    if (row[i] == '0') {
      presentCycleArray[stateIndex[i]] = 0;
    } else {
      presentCycleArray[stateIndex[i]] = 1;
    }
  }

  i = 0;
  while (i < piNumber) {
    fscanf(fpPattern, "%s", row);
    for (j = 0; j < suppSize; j++) {
      if (strcmp(nameArray[j], row) == 0) {
        piIndex[i] = j;
        break;
      }
    }
    i++;
  }

  /*----------------------------- Output File -------------------------------*/

  fpResult = Pdtutil_FileOpen(NULL, result, "w", &flagFileResult);
  if (fpResult == NULL) {
    Pdtutil_Warning(1, "Impossible to Open Output File.");
    return;
  }

  fprintf(fpResult, "#StateNames\n");
  for (i = 0; i < psNumber; i++) {
    fprintf(fpResult, "%s ", nameArray[stateIndex[i]]);
  }
  fprintf(fpResult, "\n");

  fprintf(fpResult, "#InputNames\n");
  for (i = 0; i < piNumber; i++) {
    fprintf(fpResult, "%s ", nameArray[piIndex[i]]);
  }
  fprintf(fpResult, "\n");

  fprintf(fpResult,
    "#PatternNumber PresenStates PrimaryInputs PrimaryOutputs NextStates\n");

  /*---------------------- Simulation Cycle ... Start -----------------------*/

  step = 0;
  simContinue = (iterNumberMax < 0) || (step <= iterNumberMax);
  while ((fscanf(fpPattern, "%s", row) != EOF) && simContinue) {
    /*
     *  Check for Comments
     */

    if (row[0] == '#') {
      continue;
    }

    /*
     *  Deal with PI patterns
     */

    for (i = 0; i < piNumber; i++) {
      if (row[i] == '0') {
        presentCycleArray[piIndex[i]] = 0;
      } else {
        presentCycleArray[piIndex[i]] = 1;
      }
    }

    /*
     *  Compute Next State Values
     */

    for (i = 0; i < psNumber; i++) {
      f = Ddi_BddarrayRead(delta, i);

      bddResult = Ddi_BddMakeFromCU(ddMgr, Ddi_BddEval(ddMgr, f,
          presentCycleArray));
      if (Ddi_BddIsOne(bddResult)) {
        nextCycleArray[stateIndex[i]] = 1;
      } else {
        nextCycleArray[stateIndex[i]] = 0;
      }
      Ddi_Free(bddResult);
    }

    /*
     *  Compute Output Values
     */

    /* Attenzione: occorre specificare l'ordine degli output */
    for (i = 0; i < poNumber; i++) {
      f = Ddi_BddarrayRead(lambda, i);

      bddResult = Ddi_BddMakeFromCU(ddMgr, Ddi_BddEval(ddMgr, f,
          presentCycleArray));
      if (Ddi_BddIsOne(bddResult)) {
        outputArray[i] = 1;
      } else {
        outputArray[i] = 0;
      }
      Ddi_Free(bddResult);

    }

    /*
     *  Print Out Information
     */

    /* Cycle Number */
    fprintf(fpResult, "%d ", step);
    fflush(fpResult);

    /* Present State */
    for (i = 0; i < psNumber; i++) {
      fprintf(fpResult, "%d", presentCycleArray[stateIndex[i]]);
      fflush(fpResult);
    }

    /* Primary Inputs */
    fprintf(fpResult, " ");
    for (i = 0; i < piNumber; i++) {
      fprintf(fpResult, "%d", presentCycleArray[piIndex[i]]);
      fflush(fpResult);
    }

    /* Primary Outputs */
    fprintf(fpResult, " ");
    for (i = 0; i < poNumber; i++) {
      fprintf(fpResult, "%d", outputArray[i]);
      fflush(fpResult);
    }

    /* Next State */
    fprintf(fpResult, " ");
    for (i = 0; i < psNumber; i++) {
      fprintf(fpResult, "%d", nextCycleArray[stateIndex[i]]);
      fflush(fpResult);
    }
    fprintf(fpResult, "\n");
    fflush(fpResult);

    /*
     *  Prepare (i.e., swap) Arrays for the Next Iteratin
     */

    tmpArray = presentCycleArray;
    presentCycleArray = nextCycleArray;
    nextCycleArray = tmpArray;

    step++;

    simContinue = (iterNumberMax < 0) || (step <= iterNumberMax);
  }

  /*-------------------------- Traversal ... End ----------------------------*/

  currTime = util_cpu_time() - startTime;
  Pdtutil_FileClose(fpPattern, &flagFilePattern);
  Pdtutil_FileClose(fpResult, &flagFileResult);

  /*-------------------------------- Exit -----------------------------------*/

  return;
}

/**Function********************************************************************

  Synopsis    [Simulation Engine for PdTrav]

  Description [
    If pattern is "random" the initial state set is used and input
    patterns are randomly generated. If pattern is not "random" it specifies
    the input file.
    The format of this file is the following:
    <ol>
    <li> initial state variables names
    <li> initial state (one row of 0, 1),
    <li> input variables names
    <li> input patterns (a certain number of row of 0, 1).
    </ol>
    # identifies comments.
    If results is "stdout" output is printed on standad output. Otherwise
    it specifies the name of a file.
    The format is:
    <ol>
    <li> present state variable names, input variables names, output
         variable names, next state variable names
    <li> one row for each transition in a 'kiss' format
         preset state, input, output, next states.
    </ol>
    Again # specifies comments.
    ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

void
simulateWave(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  int iterNumberMax /* Maximum Number of Iterations */ ,
  int logPeriod /* Period to Print Out Informations */ ,
  int simulationFlag /* If 1 Print 0/1 Values, if 2 Print Waves */ ,
  char *init /* Specify Where to Get Initial State */ ,
  char *pattern /* Specify Where to Get Input Patterns */ ,
  char *result                  /* Specify Where to Put Output Results */
)
{
  FILE *fpPattern, *fpInit, *fpResult;
  char row[PDTUTIL_MAX_STR_LEN], myName[PDTUTIL_MAX_STR_LEN];
  int i, j, n, flagFileInit, flagFilePattern, flagFileResult,
    step, piNumber, psNumber, poNumber, suppSize;
  long startTime, currTime;
  Pdtutil_VerbLevel_e verbosity;
  Ddi_Mgr_t *ddMgr;
  Ddi_Bdd_t *f, *bddResult;
  Ddi_Bddarray_t *delta, *lambda;
  Ddi_Var_t *v;
  Ddi_Vararray_t *psVararray, *nsVararray, *piVararray;
  int *auxidArray, *piIndex, *piInputIndex, *stateIndex, *presentCycleArray,
    *nextCycleArray, *tmpArray, *outputArray, simContinue;
  char **nameArray;
  Ddi_Bdd_t *one;
  char simulationWaveArray[N_SIGNAL][N_CLOCK_TICK];

  /*------------------------------ Init Process -----------------------------*/

  startTime = util_cpu_time();

  verbosity = Fsm_MgrReadVerbosity(fsmMgr);
  ddMgr = Fsm_MgrReadDdManager(fsmMgr);

  piNumber = Fsm_MgrReadNI(fsmMgr);
  psNumber = Fsm_MgrReadNL(fsmMgr);
  poNumber = Fsm_MgrReadNO(fsmMgr);

  psVararray = Fsm_MgrReadVarPS(fsmMgr);
  nsVararray = Fsm_MgrReadVarNS(fsmMgr);
  piVararray = Fsm_MgrReadVarI(fsmMgr);
  auxidArray = Ddi_MgrReadVarauxids(ddMgr);
  nameArray = Ddi_MgrReadVarnames(ddMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  lambda = Fsm_MgrReadLambdaBDD(fsmMgr);

  suppSize = Ddi_MgrReadNumCuddVars(ddMgr);
  one = Ddi_MgrReadOne(ddMgr);

  presentCycleArray = Pdtutil_Alloc(int,
    suppSize
  );
  nextCycleArray = Pdtutil_Alloc(int,
    suppSize
  );
  outputArray = Pdtutil_Alloc(int,
    poNumber
  );
  piInputIndex = Pdtutil_Alloc(int,
    piNumber
  );
  piIndex = Pdtutil_Alloc(int,
    piNumber
  );
  stateIndex = Pdtutil_Alloc(int,
    psNumber
  );

  /*------------------------------  Input File ------------------------------*/

  if (init != NULL) {
    fpInit = Pdtutil_FileOpen(NULL, init, "r", &flagFileInit);
    if (fpInit == NULL) {
      Pdtutil_Warning(1, "Init state File Not Present.");
      return;
    }
  } else {
    fpInit = NULL;
  }

  fpPattern = Pdtutil_FileOpen(NULL, pattern, "r", &flagFilePattern);
  if (fpPattern == NULL) {
    Pdtutil_Warning(1, "Input Pattern File Not Present.");
    return;
  }

  /*----------------------------- Output File -------------------------------*/

  fpResult = Pdtutil_FileOpen(NULL, result, "w", &flagFileResult);
  if (fpResult == NULL) {
    Pdtutil_Warning(1, "Impossible to Open Output File.");
    return;
  }

  /*--------------- Read Present State and Primary Input Variables ----------*/

  i = 0;
  do {
    if (fpInit != NULL) {
      fscanf(fpInit, "%s", row);
    } else {
      fscanf(fpPattern, "%s", row);
    }
    if (row[0] < '0' || row[0] > '9') {
      j = Ddi_VarCuddIndex(Ddi_VarFromName(ddMgr, row));
      Pdtutil_Assert(j >= 0, "Name not found for init state var");
      stateIndex[i] = j;
      i++;
    }
  } while (row[0] < '0' || row[0] > '9');
  n = i;

  /* set s0 */
  /* default is 0 */
  for (i = 0; i < suppSize; i++) {
    presentCycleArray[i] = 0;
    nextCycleArray[i] = 0;
  }

  /* change don't cares to zeroes */
  for (i = 0; i < n; i++) {
    if (row[i] == '-')
      row[i] = '0';
  }
  for (i = 0; i < n; i++) {
    presentCycleArray[stateIndex[i]] = row[i] - '0';
  }

  i = 0;
  do {
    fscanf(fpPattern, "%s", row);
    if (row[0] < '0' || row[0] > '9') {
      j = Ddi_VarCuddIndex(Ddi_VarFromName(ddMgr, row));
      Pdtutil_Assert(j >= 0, "name not found for init state var");
      piInputIndex[i] = j;
      i++;
    }
  } while (row[0] < '0' || row[0] > '9');
  n = i;

  /* Set Full Array of PI Indexes */
  for (i = 0; i < piNumber; i++) {
    v = Ddi_VararrayRead(piVararray, i);
    j = Ddi_VarCuddIndex(v);
    piIndex[i] = j;
    fprintf(fpResult, "PI[%3d]: %s\n", i, Ddi_VarName(v));
  }
  fprintf(fpResult, "\n");

  /* Set Full Array of NS Indexes */
  for (i = 0; i < psNumber; i++) {
    v = Ddi_VararrayRead(psVararray, i);
    j = Ddi_VarCuddIndex(v);
    stateIndex[i] = j;
    fprintf(fpResult, "PS[%3d]: %s\n", i, Ddi_VarName(v));
  }
  fprintf(fpResult, "\n");

  /*---------------------- Simulation Cycle ... Start -----------------------*/

  if (fpInit != NULL) {
    fscanf(fpPattern, "%s", row);
  }
  step = 0;
  simContinue = (iterNumberMax < 0) || (step <= iterNumberMax);
  while (simContinue) {

    /*
     *  Check for Comments
     */

    if (row[0] == '#') {
      /* NON FUNZIONA con fscanf perche' cerca il commento sulla
         seconda parola della riga */
      continue;
    }

    /*
     *  Deal with PI patterns
     */

    /* change don't cares to zeroes */
    for (i = 0; i < n; i++) {
      if (row[i] == '-')
        row[i] = '1';
    }

    for (i = 0; i < n; i++) {
      presentCycleArray[piInputIndex[i]] = row[i] - '0';
    }

    /*
     *  Compute Next State Values
     */

    for (i = 0; i < psNumber; i++) {
      f = Ddi_BddarrayRead(delta, i);

      bddResult = Ddi_BddMakeFromCU(ddMgr, Ddi_BddEval(ddMgr, f,
          presentCycleArray));
      if (Ddi_BddIsOne(bddResult)) {
        nextCycleArray[stateIndex[i]] = 1;
      } else {
        nextCycleArray[stateIndex[i]] = 0;
      }
      Ddi_Free(bddResult);

    }

    /*
     *  Compute Output Values
     */

    /* Attenzione: occorre specificare l'ordine degli output */
    for (i = 0; i < poNumber; i++) {
      f = Ddi_BddarrayRead(lambda, i);

      bddResult = Ddi_BddMakeFromCU(ddMgr, Ddi_BddEval(ddMgr, f,
          presentCycleArray));
      if (Ddi_BddIsOne(bddResult)) {
        outputArray[i] = 1;
      } else {
        outputArray[i] = 0;
      }
      Ddi_Free(bddResult);

    }

    /*
     *  Print Out Information
     */

    /* Cycle Number */
    fprintf(fpResult, "[%d] ", step);
    fflush(fpResult);

    /* Primary Inputs */
    fprintf(fpResult, "\nPI\n");
    for (i = 0; i < piNumber; i++) {
      fprintf(fpResult, "%d", presentCycleArray[piIndex[i]]);
      fflush(fpResult);
    }

    /* Primary Outputs */
    fprintf(fpResult, "\nPO\n");
    for (i = 0; i < poNumber; i++) {
      fprintf(fpResult, "%d", outputArray[i]);
      fflush(fpResult);
    }

    /* Present State */
    fprintf(fpResult, "\nPS\n");
    for (i = 0; i < psNumber; i++) {
      fprintf(fpResult, "%d", presentCycleArray[stateIndex[i]]);
      fflush(fpResult);
    }

    /* Next State */
    fprintf(fpResult, "\nNS\n");
    for (i = 0; i < psNumber; i++) {
      fprintf(fpResult, "%d", nextCycleArray[stateIndex[i]]);
      fflush(fpResult);
    }
    fprintf(fpResult, "\n");
    fflush(fpResult);

    /* Toggling bits */
    fprintf(fpResult, "State Bits Transitions:\n");

    for (i = 0; i < psNumber; i++) {
      if (presentCycleArray[stateIndex[i]] != nextCycleArray[stateIndex[i]]) {
        fprintf(fpResult, "[%3d] %60s -> %d\n", i,
          Ddi_VarName(Ddi_VarFromIndex(ddMgr, stateIndex[i])),
          nextCycleArray[stateIndex[i]]);
        fflush(fpResult);
      }
    }
    fprintf(fpResult, "\n\n");
    fflush(fpResult);

    /*
     *  Set simulationWaveArray
     */

    /* Primary Inputs */
    for (i = 0; i < piNumber; i++) {
      simulationWaveArray[i][step] = '0' + presentCycleArray[piIndex[i]];
    }

    /* Next State */
    for (i = 0; i < psNumber; i++) {
      simulationWaveArray[i + piNumber][step] = '0' +
        presentCycleArray[stateIndex[i]];
    }

    /* Primary Outputs */
    for (i = 0; i < poNumber; i++) {
      simulationWaveArray[i + piNumber + psNumber][step] =
        '0' + outputArray[i];
    }

    /*
     *  Prepare (i.e., swap) Arrays for the Next Iteratin
     */

    tmpArray = presentCycleArray;
    presentCycleArray = nextCycleArray;
    nextCycleArray = tmpArray;

    step++;

    simContinue = (iterNumberMax < 0) || (step <= iterNumberMax);

    /* Skip a Test Pattern .. Why??? */
    if (fpInit != NULL) {
      fscanf(fpPattern, "%*s");
    }

    /* Read Test Pattern */
    if (fscanf(fpPattern, "%s", row) == EOF)
      break;
  }

  /*--------------------- Printing Out Simulation Table ---------------------*/

  fprintf(fpResult, "\n\n");
  fprintf(fpResult, "Simulation table: %d Signal * %d Clock Tick\n\n",
    piNumber + psNumber + poNumber, step);

  /*
   *  Print Primary Inputs
   */

  for (i = 0; i < piNumber; i++) {
    fprintf(fpResult, "%-20s  ", Ddi_VarName(Ddi_VarFromIndex(ddMgr,
          piIndex[i])));

    for (j = 0; j < step; j++) {
      if (simulationFlag == 1) {
        fprintf(fpResult, "%c", simulationWaveArray[i][j]);
      } else {
        if (j == (step - 1)) {
          printWave(fpResult, simulationWaveArray[i][j],
            simulationWaveArray[i][j]);
        } else {
          printWave(fpResult, simulationWaveArray[i][j],
            simulationWaveArray[i][j + 1]);
        }
      }
    }

    fprintf(fpResult, "\n");
  }

  /*
   *  Print States
   */

  for (i = 0; i < psNumber; i++) {
    fprintf(fpResult, "%-20s  ",
      Ddi_VarName(Ddi_VarFromIndex(ddMgr, stateIndex[i])));

    for (j = 0; j < step; j++) {
      if (simulationFlag == 1) {
        fprintf(fpResult, "%c", simulationWaveArray[i + piNumber][j]);
      } else {
        if (j == (step - 1)) {
          printWave(fpResult, simulationWaveArray[i + piNumber][j],
            ('0' + presentCycleArray[stateIndex[i]]));
        } else {
          printWave(fpResult, simulationWaveArray[i + piNumber][j],
            simulationWaveArray[i + piNumber][j + 1]);
        }
      }
    }

    /* Next and Last State */
    if (simulationFlag == 1) {
      fprintf(fpResult, "%c", ('0' + presentCycleArray[stateIndex[i]]));
    } else {
      printWave(fpResult, ('0' + presentCycleArray[stateIndex[i]]),
        ('0' + presentCycleArray[stateIndex[i]]));
    }

    fprintf(fpResult, "\n");
  }

  /*
   *  Print Outputs
   */

  for (i = 0; i < poNumber; i++) {
    if (Ddi_ReadName(Ddi_BddarrayRead(lambda, i)) != NULL) {
      sprintf(myName, "%s (PO#%3d)",
        Ddi_ReadName(Ddi_BddarrayRead(lambda, i)), i);
    } else {
      sprintf(myName, "PO#%3d", i);
    }

    fprintf(fpResult, "%-20s  ", myName);

#if 0
    /* First Output Value ... THIS IS WRONG */
    if (simulationFlag == 1) {
      fprintf(fpResult, "x");
    } else {
      printWave(fpResult, 'x', 'x');
    }
#endif

    for (j = 0; j < step; j++) {
      if (simulationFlag == 1) {
        fprintf(fpResult, "%c",
          simulationWaveArray[i + piNumber + psNumber][j]);
      } else {
        if (j == (step - 1)) {
          printWave(fpResult, simulationWaveArray[i + piNumber + psNumber][j],
            simulationWaveArray[i + piNumber + psNumber][j]);
        } else {
          printWave(fpResult, simulationWaveArray[i + piNumber + psNumber][j],
            simulationWaveArray[i + piNumber + psNumber][j + 1]);
        }
      }
    }

    fprintf(fpResult, "\n");
  }

  /*-------------------------- Traversal ... End ----------------------------*/

  currTime = util_cpu_time() - startTime;
  Pdtutil_FileClose(fpInit, &flagFileInit);
  Pdtutil_FileClose(fpPattern, &flagFilePattern);
  Pdtutil_FileClose(fpResult, &flagFileResult);

  /*-------------------------------- Exit -----------------------------------*/

  return;
}

/**Function********************************************************************

  Synopsis    [Follow Bertacco, Damiani, Quer, DAC'99 paper to
    peeform simbolic simulation.
    ]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

void
simulateWithDac99(
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  int iterNumberMax /* Maximum Number of Iterations */ ,
  int deadEndMaxNumberOf /* Maximum Number of Dead Ends */ ,
  int logPeriod /* Period to Print Out Informations */ ,
  int depthBreadth /* In Dac'99 Method Use Random Choices */ ,
  int random /* In Dac'99 Use Random Values */ ,
  char *pattern /* Specify Where to Get Input Patterns */ ,
  char *result                  /* Specify Where to Put Output Results */
)
{
  Ddi_Mgr_t *ddMgr;
  Ddi_Bddarray_t *delta, *deltaActive;
  Ddi_Vararray_t *psVararray, *piVararray, *nsVararray;
  Ddi_Varset_t *psPiVarset, *psVarset, *piVarset, *nsVarset;
  Ddi_Var_t *var;
  Ddi_Bdd_t *one, *from, *reached, *to, *relationPsPiPs, *tmp, *lit;
  Pdtutil_VerbLevel_e verbosity;
  long startTime, currTime;
  double stateNumber;
  int i, step, travContinue, piNumber, psNumber, toIncluded, deadEndNumberOf;
  int varN, varFreeAvgN = 0, varAssAvgN = 0;

#if USE_TRAV_OR_DIRECTMETHOD
  Trav_Mgr_t *travMgr;
  Tr_Mgr_t *trMgr;
#endif

  /*-----------------------------   Traversal   -----------------------------*/

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    Pdtutil_ChrPrint(stdout, '*', 50);
    fprintf(stdout, "\nDAC1999 Simulation.\n");
    fprintf(stdout, "Parameters:.\n");
    fprintf(stdout, "Max Number of Iterations = %d.\n", iterNumberMax);
    fprintf(stdout, "Max Number of Dead-Ends = %d.\n", deadEndMaxNumberOf);
    fprintf(stdout, "Log Period = %d.\n", logPeriod);
    fprintf(stdout, "Depth (0) - Breadth (1) = %d.\n", depthBreadth);
    fprintf(stdout, "Exaustive (0) - Random (1) = %d.\n", random);
    Pdtutil_ChrPrint(stdout, '*', 50);
    fprintf(stdout, "\n")
    );

  startTime = util_cpu_time();

  ddMgr = Fsm_MgrReadDdManager(fsmMgr);
  verbosity = Fsm_MgrReadVerbosity(fsmMgr);
  one = Ddi_MgrReadOne(ddMgr);

#if USE_TRAV_OR_DIRECTMETHOD
  trMgr = Tr_MgrInit(NULL, ddMgr);
  Tr_MgrSetOption(trMgr, Pdt_TrSort_c, inum, Tr_SortWeight_c);
  travMgr = Trav_MgrInit(NULL, ddMgr);
#endif

  piNumber = Fsm_MgrReadNI(fsmMgr);
  psNumber = Fsm_MgrReadNL(fsmMgr);

  psVararray = Fsm_MgrReadVarPS(fsmMgr);
  piVararray = Fsm_MgrReadVarI(fsmMgr);
  nsVararray = Fsm_MgrReadVarNS(fsmMgr);

  psVarset = Ddi_VarsetMakeFromArray(psVararray);
  piVarset = Ddi_VarsetMakeFromArray(piVararray);
  nsVarset = Ddi_VarsetMakeFromArray(nsVararray);
  psPiVarset = Ddi_VarsetUnion(psVarset, piVarset);

#if 0
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "Inputs:\n")
    );
  Ddi_VarsetPrint(piVarset, -1, NULL, stdout);
  Ddi_PrintVararray(piVararray);
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "Ps:\n")
    );
  Ddi_VarsetPrint(psVarset, -1, NULL, stdout);
  Ddi_PrintVararray(psVararray);
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "Ns:\n")
    );
  Ddi_VarsetPrint(nsVarset, -1, NULL, stdout);
  Ddi_PrintVararray(nsVararray);
#endif

  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  from = Ddi_BddDup(Fsm_MgrReadInitBDD(fsmMgr));
  reached = Ddi_BddDup(from);
  relationPsPiPs = Ddi_BddDup(Ddi_BddNot(one));

  step = 0;
  deadEndNumberOf = 0;
  travContinue = (iterNumberMax < 0) || (step <= iterNumberMax);

  /*---------------------- Traversal Cycle ... Start ------------------------*/

  while (travContinue) {
    if ((step % logPeriod) == 0) {
      verbosity = Fsm_MgrReadVerbosity(fsmMgr);
    } else {
      verbosity = Pdtutil_VerbLevelNone_c;
    }

    // Get Original Delta, i.e., circuit representation
    deltaActive = Ddi_BddarrayDup(delta);

    /*
     *  Simplify Next PsNumber Functions Before Image Computation
     *  DAC'99 VaB, MaD, StQ
     */

    varN = reduceNextStateFunctions(ddMgr, verbosity, depthBreadth, random,
      deltaActive, from, piNumber, psNumber, psVarset, psVararray, nsVararray,
      &relationPsPiPs);
    varAssAvgN += varN & 0xfff;
    varFreeAvgN += (varN >> 12) & 0xfff;

#if 0
    tmpVarset = Ddi_BddSupp(relationPsPiPs);
    Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Relation Ps-Pi-Ps (Ns):\n")
      );
    Ddi_VarsetPrint(tmpVarset, -1, NULL, stdout);
    Ddi_Free(tmpVarset);
#endif

    /*
     *  Compute Image
     */

#if USE_TRAV_OR_DIRECTMETHOD
    for (i = 0; i < psNumber; i++) {
      f = Ddi_BddarrayRead(deltaActive, i);
      tmp = Ddi_BddSwapVars(f, nsVararray, psVararray);
      Ddi_Free(f);
      Ddi_BddarrayWrite(deltaActive, i, tmp);
    }
    trPart = Tr_BuildPartTR(deltaActive, nsVararray);
    trCluster = trPart;
    trMono = Tr_BuildMonoTR(trMgr, trCluster, psPiVarset, piVarset, nsVarset);
    to =
      Trav_ImgMonoTr(travMgr, trMono, psVararray, nsVararray, one, psPiVarset);
#else
    to = Ddi_BddDup(one);
    for (i = 0; i < psNumber; i++) {
      var = Ddi_VararrayRead(psVararray, i);
      lit = Ddi_BddMakeLiteral(var, 1);
      tmp = Ddi_BddarrayRead(deltaActive, i);
      tmp = Ddi_BddXnor(tmp, lit);
      Ddi_BddAndAcc(to, tmp);
      Ddi_Free(lit);
      Ddi_Free(tmp);
    }
    Ddi_BddExistAcc(to, nsVarset);
#endif

#if 0
    Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "To:\n")
      );
    Ddi_BddPrint(to);
#endif

    /*
     *  Print Statistics for this Iteration - Part 1
     */

    Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "TravLevel %d: ", step);
      fprintf(stdout, "[|From|: %d]", Ddi_BddSize(from));
      fprintf(stdout, "[|To|: %d]", Ddi_BddSize(to));
      currTime = util_cpu_time();
      fprintf(stdout, "(Time: %s).\n", util_print_time(currTime - startTime))
      );

    /*
     *  Check Fix Point
     */

    if (Ddi_BddIncluded(to, reached)) {
      deadEndNumberOf++;
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Number of Dead-End Reached = %d.\n", deadEndNumberOf)
        );
    }

    if ((deadEndMaxNumberOf > 0) && (deadEndNumberOf > deadEndMaxNumberOf)) {
      toIncluded = 1;
    } else {
      toIncluded = 0;
    }

    /*
     *  Prepare Sets for the New Iteration
     */

    Ddi_Free(deltaActive);
    Ddi_BddOrAcc(reached, to);
    tmp = SimFromCompute(from, to, reached, depthBreadth);
    Ddi_Free(from);
    from = tmp;
    Ddi_Free(to);

    /*
     *  Print Statistics for this Iteration - Part 2
     */

    Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "[|Reached|: %d][|RelationsPsPiPs|: %d]",
        Ddi_BddSize(reached), Ddi_BddSize(relationPsPiPs));
      stateNumber = Ddi_BddCountMinterm(reached, Ddi_MgrReadSize(ddMgr)) /
      pow(2.0, (double)(Ddi_MgrReadSize(ddMgr) - Ddi_VararrayNum(psVararray)));
      fprintf(stdout, "[#Reached: %g].\n", stateNumber)
      );

    Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout,
        "Memory: %ld Kbytes; Unique Table: %u nodes; DDI-DD Num: %d;.\n",
        Ddi_MgrReadMemoryInUse(ddMgr) / 1024, Ddi_MgrReadKeys(ddMgr),
        Ddi_MgrReadExtBddRef(ddMgr));
      fprintf(stdout, "Cache: %u slots.\n", Ddi_MgrReadCacheSlots(ddMgr))
      );

    travContinue = (iterNumberMax < 0) || (step <= iterNumberMax);

    if (toIncluded == 1) {
      travContinue = 0;
    }

    if (travContinue == 1) {
      step++;
    }
  }

  /*------------------------- Print Final Statistics ------------------------*/

  currTime = util_cpu_time() - startTime;

  if (reached == NULL) {
    Pdtutil_Warning(1, "Traversal is failed.");
    return;
  }

  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelNone_c,
    Pdtutil_ChrPrint(stdout, '*', 50);
    fprintf(stdout, "\n");
    fprintf(stdout, "Simbolic Simulation Results Summary:.\n");
    fprintf(stdout, "Traversal Depth: %d.\n", step);
    fprintf(stdout, "#Reached Size: %d.\n", Ddi_BddSize(reached))
    );

  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMed_c,
    /* Total number of reached psNumbers */
    stateNumber = Ddi_BddCountMinterm(reached, Ddi_MgrReadSize(ddMgr)) /
    pow(2.0, (double)(Ddi_MgrReadSize(ddMgr) - Ddi_VararrayNum(psVararray)));
    fprintf(stdout, "#Reached States: %g.\n", stateNumber);
    fprintf(stdout, "CPU Time: %s.\n", util_print_time(currTime))
    );

  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelNone_c,
    fprintf(stdout,
      "Average intermediate variables %.3f - assigned %.3f.\n",
      ((double)varFreeAvgN) / ((double)step),
      ((double)varAssAvgN) / ((double)step));
    Pdtutil_ChrPrint(stdout, '*', 50);
    fprintf(stdout, "\n")
    );

  /*-------------------- Traversal ... End ... Free ALL ---------------------*/

  Ddi_Free(from);
  Ddi_Free(reached);
  Ddi_Free(relationPsPiPs);

  Ddi_Free(psVarset);
  Ddi_Free(piVarset);
  Ddi_Free(nsVarset);
  Ddi_Free(psPiVarset);

  Ddi_Free(psVararray);
  Ddi_Free(piVararray);
  Ddi_Free(nsVararray);

  /*-------------------------------- Exit -----------------------------------*/

  return;
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Print Out A Wave Depending on Present and Next Value]

  Description [Sequence of Print Statements.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static void
printWave(
  FILE * fp /* File Pointer to Print on */ ,
  char presentValue /* Specify Present Value */ ,
  char nextValue                /* Specify Next Value */
)
{
  presentValue = tolower(presentValue);
  nextValue = tolower(nextValue);

  /* Print Out Current Value */
  if (presentValue == '0') {
    fprintf(fp, "__");
  }

  if (presentValue == '1') {
    fprintf(fp, "~~");
  }

  if (presentValue == 'x') {
    fprintf(fp, "xx");
  }

  /* Print Out Time Transition */
  if (presentValue == '0' && nextValue == '0') {
    fprintf(fp, "_");
  }

  if (presentValue == '0' && nextValue == '1') {
    fprintf(fp, "/");
  }

  if (presentValue == '1' && nextValue == '0') {
    fprintf(fp, "\\");
  }

  if (presentValue == '1' && nextValue == '1') {
    fprintf(fp, "~");
  }


  if (presentValue == 'x' || nextValue == 'x') {
    fprintf(fp, "|");
  }

  fflush(fp);

  return;
}

/**Function********************************************************************

  Synopsis    [Reduce Next PsNumber Functions].

  Description [Follow DAC'99 by VaB, MaD, StQ. In particular the ReducePsNumber
    function from VaB.
    ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static int
reduceNextStateFunctions(
  Ddi_Mgr_t * ddMgr,
  Pdtutil_VerbLevel_e verbosity,
  int depthBreadth,
  int random,
  Ddi_Bddarray_t * delta,
  Ddi_Bdd_t * from,
  int piNumber,
  int psNumber,
  Ddi_Varset_t * psVarset,
  Ddi_Vararray_t * psVararray,
  Ddi_Vararray_t * nsVararray,
  Ddi_Bdd_t ** relationPsPiPs
)
{
  Ddi_Bdd_t *one, *f, *tmp, *tmp1, *tmp2, *constrainIn = NULL, *constrainOut1,
    *constrainOut2;
  Ddi_Varset_t *supp, *suppTmp, *aux;
  Ddi_Varset_t *simple, *complex, *shared, *bound;
  Ddi_Var_t *var;
  Ddi_Vararray_t *SwapSrc, *SwapDst;
  st_table *table;
  int i, hashValue;
  int totalSupport, simpleSize, initialSupport;
  int index, position, remappingIndex = 0, shcm, intermediate = 0, funcs = 0;
  int *pStatePositions, *equivalence;

  /*
   *  Variable order must be stable during parametric transform
   */

  Ddi_MgrSiftSuspend(ddMgr);

  /*
   *  Get What is Needed
   */

  one = Ddi_MgrReadOne(ddMgr);

  simple = Ddi_VarsetVoid(ddMgr);
  bound = Ddi_VarsetVoid(ddMgr);
  shared = Ddi_VarsetVoid(ddMgr);
  complex = Ddi_VarsetVoid(ddMgr);

  // Restrict the delta array with the from set
  Ddi_BddarrayOp2(Ddi_BddConstrain_c, delta, from);

  // Get what is needed
  totalSupport = Ddi_MgrReadNumCuddVars(ddMgr);
  pStatePositions = Pdtutil_Alloc(int,
    totalSupport
  );
  equivalence = Pdtutil_Alloc(int,
    totalSupport
  );

  aux = Ddi_BddarraySupp(delta);
  initialSupport = Ddi_VarsetNum(aux);
  Ddi_Free(aux);
  memset(equivalence, 0, sizeof(int) * totalSupport);

  // find the positions of the parameters
  memset(pStatePositions, -1, sizeof(int) * totalSupport);
  for (i = 0; i < Ddi_VararrayNum(nsVararray); i++) {
    index = Ddi_VarCuddIndex(Ddi_VararrayRead(nsVararray, i));
    position = Ddi_MgrReadPerm(ddMgr, index);
    pStatePositions[position] = index;
  }

  // Now stateNumber is an array storing all the valid indexes for psNumber
  // variables, at the proper position in the current ordering

  // print the total set
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDebug_c,
    fprintf(stdout, "inputParam #%d.\n", piNumber + psNumber);
    fprintf(stdout, "totalSupport set #%d.\n", totalSupport);
    fprintf(stdout, "pStatePositions ");
    for (i = 0; i < totalSupport; i++) {
    fprintf(stdout, "[%d]", pStatePositions[i]);}
    fprintf(stdout, "\n")
    ) ;

  /*
   *  Step One: Create the simple set
   */

  for (i = 0; i < psNumber; i++) {
    /* Get a Dd from an Array */
    f = Ddi_BddarrayRead(delta, i);

    /* don't care of constants for now */
    if (Ddi_BddIsConstant(f)) {
      continue;
    }

    supp = Ddi_BddSupp(f);

    Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDebug_c,
      fprintf(stdout, "Support of f%d.\n", i);
      Ddi_VarsetPrint(supp, -1, NULL, stdout)
      );

    if (Ddi_VarsetNum(supp) == 1) {
      /* Variable BDD node */
      /* add to the set of simple variables */
      Ddi_VarsetUnionAcc(simple, supp);

    }
    Ddi_Free(supp);
  }

  simpleSize = Ddi_VarsetNum(simple);

  // print the simple set
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDebug_c,
    fprintf(stdout, "Simple set (%d):", simpleSize);
    if (!(Ddi_VarsetIsVoid(simple))) {
    Ddi_VarsetPrint(simple, -1, NULL, stdout);}
    fprintf(stdout, "\n")
    ) ;

  /*
   *  Step Two: Discover complex variable set and cofactor them out
   */

  for (i = 0; i < psNumber; i++) {
    /* Get a Dd from an Array */
    f = Ddi_BddarrayRead(delta, i);
    supp = Ddi_BddSupp(f);
    suppTmp = Ddi_VarsetIntersect(supp, simple);
    if (!Ddi_VarsetIsVoid(suppTmp)) {
      Ddi_VarsetDiffAcc(supp, simple);
      if (!Ddi_VarsetIsVoid(supp)) {
        Ddi_VarsetUnionAcc(complex, supp);
      }
    }
    Ddi_Free(suppTmp);
    Ddi_Free(supp);
  }

  /*
   *  Constraint with complex var;
   *  1) random values
   *  2) use the relationPsPiPs as:
   *     constrainIn = \exist ps (relationPsPiPs * from)
   *     i.e., set of pi - ps (coded as ns) used in the past to restrict those ps
   */

  if (random == 1) {
    constrainWithAllRandomValue(ddMgr, delta, complex, &constrainOut1);
  } else {
    constrainIn = Ddi_BddAnd(*relationPsPiPs, from);
    Ddi_BddExistAcc(constrainIn, psVarset);
    Ddi_BddSwapVarsAcc(constrainIn, nsVararray, psVararray);
    constrainWith1(ddMgr, delta, complex, constrainIn, &constrainOut1,
      &constrainOut2);
  }

  /*
   *  At this point the array of psNumber functions
   *  does not contain any complex variable.
   */

  /*
   *  Step Three: Discover the equivalence classes
   */

  // create temporary hash table to count occurrences of each delta function
  table = st_init_table(st_ptrcmp, st_ptrhash);
  hashValue = 0;
  for (i = 0; i < psNumber; i++) {
    f = Ddi_BddarrayRead(delta, i);
    if (st_insert(table, (char *)f, (char *)hashValue) == ST_OUT_OF_MEM) {
      Pdtutil_Warning(1, "Hast Table Full ... Abort.");
      exit(1);
    }
  }

  // mark unbound functions
  // count how many times each unbound variable appears
  for (i = 0; i < psNumber; i++) {
    f = Ddi_BddarrayRead(delta, i);

    if (Ddi_BddIsConstant(f)) {
      continue;
    }

    supp = Ddi_BddSupp(f);
    suppTmp = Ddi_VarsetIntersect(simple, supp);
    if (Ddi_VarsetIsVoid(suppTmp)) {
      // this is an unbound function
      // count how many members in the equiv class

      if (st_delete(table, (char **)&f, (char **)&hashValue) != 1) {
        Pdtutil_Warning(1, "Hash Table has lost a function ... Abort.");
        exit(1);
      }

      hashValue++;
      if (st_insert(table, (char *)f, (char *)hashValue) == ST_OUT_OF_MEM) {
        Pdtutil_Warning(1, "Hast Table Full ... Abort.");
        exit(1);
      }

      while (!Ddi_VarsetIsVoid(supp)) {
        var = Ddi_VarsetTop(supp);

        // equivalence array counts in how many functions each variable
        // appears (mantained by variable index)
        index = Ddi_VarCuddIndex(var);
        equivalence[index]++;

        /* Remove the Var from the Var Set */
        Ddi_VarsetNextAcc(supp);
      }
    } else {
      // It must be a function of simple variables: check
      Ddi_Free(suppTmp);
      suppTmp = Ddi_VarsetDiff(supp, simple);
      if (!Ddi_VarsetIsVoid(suppTmp)) {
        Pdtutil_Warning(1,
          "Error: Unbound function intersecting properly the simple set");
        exit(1);
      }
    }
    Ddi_Free(suppTmp);
    Ddi_Free(supp);
  }

  /*
   *  Step Four: Split each variable in shared or bound
   */

  for (i = 0; i < psNumber; i++) {
    /* Get a Dd from an Array */
    f = Ddi_BddarrayRead(delta, i);

    if (st_lookup(table, (char *)f, (char **)&hashValue) != 1) {
      Pdtutil_Warning(1, "Hash Table has lost a function ... Abort.");
      exit(1);
    }

    if (hashValue == 0) {
      continue;
    }

    supp = Ddi_BddSupp(f);
    while (!Ddi_VarsetIsVoid(supp)) {
      var = Ddi_VarsetTop(supp);
      index = Ddi_VarCuddIndex(var);
      assert(equivalence[index] >= hashValue);

      if (equivalence[index] == hashValue) {
        Ddi_VarsetAddAcc(bound, var);
      } else {
        Ddi_VarsetAddAcc(shared, var);
      }

      /* Remove the Var from the Var Set */
      Ddi_VarsetNextAcc(supp);
    }

    if (st_delete(table, (char **)&f, (char **)&hashValue) != 1) {
      Pdtutil_Warning(1, "Hash Table has lost a function ... Abort.");
      exit(1);
    }

    hashValue = 0;
    if (st_insert(table, (char *)f, (char *)hashValue) == ST_OUT_OF_MEM) {
      Pdtutil_Warning(1, "Hast Table Full ... Abort.");
      exit(1);
    }
  }

  /*
   *  Step Five: Cofactor shared variables
   */

  if (random == 1) {
    constrainWithAllRandomValue(ddMgr, delta, shared, &constrainOut2);
  } else {
    constrainWith2(ddMgr, delta, shared, &constrainOut2);

    /*
     *  Compute
     *  relationPsPiPs = relationPsPiPs +
     *     from(s) * constrainOut1(s,x)|s->y * constrainOut2(s,x)|s->y
     */

    tmp1 = Ddi_BddSwapVars(constrainOut1, psVararray, nsVararray);
    tmp2 = Ddi_BddSwapVars(constrainOut2, psVararray, nsVararray);
    tmp = Ddi_BddAnd(from, tmp1);
    Ddi_BddAndAcc(tmp, tmp2);
    *relationPsPiPs = Ddi_BddEvalFree(Ddi_BddOr(*relationPsPiPs, tmp),
      *relationPsPiPs);
    Ddi_Free(tmp1);
    Ddi_Free(tmp2);
    Ddi_Free(tmp);
    Ddi_Free(constrainIn);
  }

  Ddi_Free(constrainOut1);
  Ddi_Free(constrainOut2);

  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDebug_c,
    for (i = 0; i < psNumber; i++) {
      /* Get a Dd from an Array */
      f = Ddi_BddarrayRead(delta, i); if (Ddi_BddIsConstant(f)) {
      continue;}
    fprintf(stdout, "Support of f%d.\n", i);
      supp = Ddi_BddSupp(f);
      Ddi_VarsetPrint(supp, -1, NULL, stdout);
      Ddi_Free(supp); fprintf(stdout, "\n");}
  ) ;

  /*
   *  Step Six: Remap bound functions and simple variables
   */

  // construct the remapping for simple variables
  remappingIndex = 0;
  supp = Ddi_VarsetDup(simple);
  SwapSrc = Ddi_VararrayAlloc(ddMgr, simpleSize);
  SwapDst = Ddi_VararrayAlloc(ddMgr, simpleSize);

  for (i = 0; i < simpleSize; i++) {
    var = Ddi_VarsetTop(supp);
    Ddi_VararrayWrite(SwapSrc, i, var);
    Ddi_VarsetNextAcc(supp);

    while (pStatePositions[remappingIndex] == UNSET) {
      remappingIndex++;
      assert(remappingIndex < totalSupport);
    }
    intermediate++;
    var = Ddi_VarFromIndex(ddMgr, pStatePositions[remappingIndex++]);
    Ddi_VararrayWrite(SwapDst, i, var);
  }
  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelDebug_c,
    for (i = 0; i < simpleSize; i++) {
    var = Ddi_VararrayRead(SwapSrc, i);
      fprintf(stdout, "%s->", Ddi_VarName(var));
      var = Ddi_VararrayRead(SwapDst, i);
      fprintf(stdout, "%s ", Ddi_VarName(var));}
    fprintf(stdout, "\n")
    ) ;

  // prepare the new delta:
  // - constant functions are unchanged
  // - bound functions are mapped to a simple parameter
  // - simple function are translated according to the Swap mapping

  for (i = 0; i < psNumber; i++) {
    /* Get a Dd from an Array */
    f = Ddi_BddarrayRead(delta, i);

    if (Ddi_BddIsConstant(f)) {
      // The new delta entry is already ok
      continue;
    }

    funcs++;
    supp = Ddi_BddSupp(f);
    suppTmp = Ddi_VarsetIntersect(bound, supp);
    if (!Ddi_VarsetIsVoid(suppTmp)) {
      // Must be a bound function. Check that it's actually a subset
      if (!Ddi_VarsetEqual(supp, suppTmp)) {
        Pdtutil_Warning(1, "Bound function contains non-bound variable.");
        exit(1);
      }
      // ok. the function it's fine, now substitute with an
      // available parameter
      while (pStatePositions[remappingIndex] == UNSET) {
        remappingIndex++;
        assert(remappingIndex < totalSupport);
      }
      intermediate++;
      var = Ddi_VarFromIndex(ddMgr, pStatePositions[remappingIndex++]);
      Ddi_Free(f);
      f = Ddi_BddMakeLiteral(var, 1);
      Ddi_BddarrayWrite(delta, i, f);
    } else {
      // function of simple variables
      tmp = Ddi_BddSwapVars(f, SwapSrc, SwapDst);
      Ddi_Free(f);
      Ddi_BddarrayWrite(delta, i, tmp);
    }                           /* if */

    Ddi_Free(suppTmp);
    Ddi_Free(supp);
  }                             /* for */

  /*
   *  Parameters are the next psNumber variables ...
   */

#if NETLIST_DEBUG_1
  for (i = 0; i < psNumber; i++) {
    /* Get a Dd from an Array */
    f = Ddi_BddarrayRead(delta, i);
    if (Ddi_BddIsConstant(f))
      continue;
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "Final support of f%d.\n", i)
      );
    supp = Ddi_BddSupp(f);
    Ddi_VarsetPrint(supp, -1, NULL, stdout);
    Ddi_Free(supp);
  }
#endif

  /*
   *  Final checks and clean up
   */

  // intermediate are less or equal number of psNumbers
  assert(remappingIndex < totalSupport);

  assert(simpleSize + Ddi_VarsetNum(complex)
    + Ddi_VarsetNum(shared) + Ddi_VarsetNum(bound) == initialSupport);

  // intermediate
  shcm = Ddi_VarsetNum(shared) + Ddi_VarsetNum(complex);

  Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMed_c,
    fprintf(stdout,
      "ReduceState: |Simple|=%d |Bound|=%d Ass=%d Inter=%d NonConstFuncs=%d.\n",
      simpleSize, Ddi_VarsetNum(bound), shcm, intermediate, funcs)
    );

  assert(shcm >= 0 && (shcm < (1 << 12)));
  assert(intermediate >= 0 && (intermediate < (1 << 12)));

  // Free
  Pdtutil_Free(pStatePositions);
  Pdtutil_Free(equivalence);

  Ddi_Free(one);
  Ddi_Free(SwapSrc);
  Ddi_Free(SwapDst);
  Ddi_Free(simple);
  Ddi_Free(bound);
  Ddi_Free(shared);
  Ddi_Free(complex);

  // variable order can be free again
  Ddi_MgrSiftResume(ddMgr);

  st_free_table(table);

  return ((intermediate & 0xfff) << 12) | (shcm & 0xfff);
}

/**Function********************************************************************

  Synopsis    [Constraint delta (Ddi_Bddarray_t) with suppLocal (Ddi_Varset_t).
    ]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static void
constrainWith1(
  Ddi_Mgr_t * ddMgr,
  Ddi_Bddarray_t * delta,
  Ddi_Varset_t * suppIn,
  Ddi_Bdd_t * constrainIn,
  Ddi_Bdd_t ** constrainOut1,
  Ddi_Bdd_t ** constrainOut2
)
{
  Ddi_Bdd_t *constrainInNot;
  Ddi_Varset_t *suppLocal1, *suppLocal2;

  constrainInNot = Ddi_BddNot(constrainIn);
  *constrainOut2 = Ddi_BddPickOneCube(constrainInNot);

  suppLocal1 = Ddi_BddSupp(*constrainOut2);
  suppLocal2 = Ddi_VarsetDiff(suppLocal1, suppIn);

  *constrainOut1 = Ddi_BddExist(*constrainOut2, suppLocal2);

  constrainWithPartialRandomValue(ddMgr, delta, suppIn, constrainOut1);

  Ddi_Free(suppLocal1);
  Ddi_Free(suppLocal2);

  return;
}

/**Function********************************************************************

  Synopsis    [Constraint delta (Ddi_Bddarray_t) with suppLocal (Ddi_Varset_t).
    ]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static void
constrainWith2(
  Ddi_Mgr_t * ddMgr,
  Ddi_Bddarray_t * delta,
  Ddi_Varset_t * suppIn,
  Ddi_Bdd_t ** constrainOut
)
{
  Ddi_Varset_t *suppLocal1, *suppLocal2;

  suppLocal1 = Ddi_BddSupp(*constrainOut);
  suppLocal2 = Ddi_VarsetDiff(suppLocal1, suppIn);

  Ddi_BddExistAcc(*constrainOut, suppLocal2);

  constrainWithPartialRandomValue(ddMgr, delta, suppIn, constrainOut);

  Ddi_Free(suppLocal1);
  Ddi_Free(suppLocal2);

  return;
}

/**Function********************************************************************

  Synopsis    [Constraint delta (Ddi_Bddarray_t) with suppLocal (Ddi_Varset_t).
    Use random values for each variable in suppLocal.
    constrainIn is not used.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static void
constrainWithAllRandomValue(
  Ddi_Mgr_t * ddMgr,
  Ddi_Bddarray_t * delta,
  Ddi_Varset_t * supp,
  Ddi_Bdd_t ** constrainOut
)
{
  Ddi_Var_t *var;
  Ddi_Varset_t *suppLocal;
  Ddi_Bdd_t *one, *lit, *constrainLocal;
  int i;

  one = Ddi_MgrReadOne(ddMgr);
  constrainLocal = Ddi_BddDup(one);
  suppLocal = Ddi_VarsetDup(supp);

  while (!Ddi_VarsetIsVoid(suppLocal)) {
    var = Ddi_VarsetTop(suppLocal);
    i = rand() % 2;
    if (i) {
      lit = Ddi_BddMakeLiteral(var, 1);
    } else {
      lit = Ddi_BddMakeLiteral(var, 0);
    }

    Ddi_BddAndAcc(constrainLocal, lit);
    Ddi_Free(lit);
    Ddi_VarsetNextAcc(suppLocal);
  }

  // cofactor out all the shared vars
  Ddi_BddarrayOp2(Ddi_BddConstrain_c, delta, constrainLocal);
  *constrainOut = Ddi_BddDup(constrainLocal);

  // Set ConstraintOut
  Ddi_Free(constrainLocal);
  Ddi_Free(suppLocal);

  return;
}


/**Function********************************************************************

  Synopsis    [Constraint delta (Ddi_Bddarray_t) with suppLocal (Ddi_Varset_t).
    Use random values for each variable in suppLocal that does not appear in
    constrain. All the new values are and-ed with constrain that is returned.
    ]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static void
constrainWithPartialRandomValue(
  Ddi_Mgr_t * ddMgr,
  Ddi_Bddarray_t * delta,
  Ddi_Varset_t * supp,
  Ddi_Bdd_t ** constrain
)
{
  Ddi_Var_t *var;
  Ddi_Varset_t *suppLocal, *suppIn;
  Ddi_Bdd_t *one, *lit;
  int i;

  one = Ddi_MgrReadOne(ddMgr);

  suppLocal = Ddi_VarsetDup(supp);
  suppIn = Ddi_BddSupp(*constrain);

  while (!Ddi_VarsetIsVoid(suppLocal)) {
    var = Ddi_VarsetTop(suppLocal);

    if (Ddi_VarInVarset(suppIn, var) == 0) {
      i = rand() % 2;
      if (i) {
        lit = Ddi_BddMakeLiteral(var, 1);
      } else {
        lit = Ddi_BddMakeLiteral(var, 0);
      }

      Ddi_BddAndAcc(*constrain, lit);

      Ddi_Free(lit);
    }

    Ddi_VarsetNextAcc(suppLocal);
  }

  // cofactor out all the shared vars
  Ddi_BddarrayOp2(Ddi_BddConstrain_c, delta, *constrain);

  // Set ConstraintOut
  Ddi_Free(suppLocal);
  Ddi_Free(suppIn);

  return;
}

/**Function********************************************************************

  Synopsis    [Compute New From for Next Iteration.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static Ddi_Bdd_t *
SimFromCompute(
  Ddi_Bdd_t * from /* from set for previous image computation */ ,
  Ddi_Bdd_t * to /* result of image computation */ ,
  Ddi_Bdd_t * relation /* old reached */ ,
  int depthBreadth              /* selection option */
)
{
  Ddi_Bdd_t *tmp, *fromNew;

  switch (depthBreadth) {
    case 0:
      /* Depth First */
      fromNew = Ddi_BddDup(to);
      break;
    case 1:
      /* Breadth First: Get From Different From Last One in relationPsPiPs */
      /*fromNew = Ddi_BddDup (from); */
      tmp = Ddi_BddAnd(relation, Ddi_BddNot(from));
      fromNew = Ddi_BddPickOneCube(tmp);
      Ddi_Free(tmp);
      break;
    default:
      fromNew = Ddi_BddDup(to);
      Pdtutil_Warning(1, "Wrong SimFromCompute Option.");
      break;
  }

  return (fromNew);
}

/*
 * Garbage Tip
 */

#if 0
  /* How to Use Symbol Table */
table = st_init_table(st_ptrcmp, st_ptrhash);
for (i = 0, scritto = 0; i < psNumber; i++, scritto += 13) {
  f = Ddi_BddarrayRead(delta, i);
  if (st_insert(table, (char *)f, (char *)scritto) == ST_OUT_OF_MEM) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "Error.\n")
      );
  } else {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Number=%d  Pointer=%d Value=%d.\n", i, f, scritto)
      );
  }
}

for (i = 0;; i++) {
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "Input Number: ")
    );
  scanf("%d", &j);
  f = Ddi_BddarrayRead(delta, j);
  /*
     if (st_lookup (table, (char *) f, (char **) &letto) == 1) {
   */
  if (st_delete(table, (char **)&f, (char **)&letto) == 1) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "---> %d %d.\n", f, letto)
      );
  } else {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "---> NO.\n")
      );
  }
}

st_free_table(table);
#endif

#if 0
  /* Esempio di come Settare Nuovi BDD Nell'Array Delle delta */
for (i = 0; i < psNumber; i++) {
  f = Ddi_BddarrayRead(delta, i);
  Ddi_Free(f);
  Ddi_BddarrayWrite(delta, i, one);
}
#endif

#if 0
  /*
   *  Step Zero: Build Total Var Set
   *  (probably avoidable)
   */
for (i = 0; i < psNumber; i++) {
  f = Ddi_BddarrayRead(delta, i);


  supp = Ddi_BddSupp(f);
  Ddi_VarsetUnionAcc(total, supp);
  Ddi_Free(supp);
}

totalSize = Ddi_Varset_n(total);
#endif

/**CFile***********************************************************************

  FileName    [fsmPort.c]

  PackageName [fsm]

  Synopsis    [Functions to guarantee portability]

  Description [In this file are declarated function to deal with
    BLIF format (as package Nanotrav in CUDD).
    ]

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

#include "fsmInt.h"
#include "aiger.h"

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

static FsmPortNtrOptions_t *mainInit(
);
static int veri_int;

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of external functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Reads description of a fsmMgr (BLIF format) from file]

  Description [The function gives a compatible DDI:
    <ul>
    <li>number of primary input
    <li>number of latches
    <li>array of primary input variables
    <li>array of present state variables
    <li>array of next state variables
    <li>array of next state functions
    <li>array of partitioned transition relation
    </ul>]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int
Fsm_MgrLoadFromBlif(
  Fsm_Mgr_t ** fsmMgrP /* FSM Pointer */ ,
  Ddi_Mgr_t * dd /* BDD manager */ ,
  char *fileFsmName /* FSM File Name */ ,
  char *fileOrdName /* ORD File Name */ ,
  int bddFlag /* Not Used For Now */ ,
  Pdtutil_VariableOrderFormat_e ordFileFormat   /* Order File Format */
)
{
  return (Fsm_MgrLoadFromBlifWithCoiReduction(fsmMgrP, dd, fileFsmName,
      fileOrdName, bddFlag, ordFileFormat, NULL));
}



/**Function********************************************************************

  Synopsis    [Reads description of a fsmMgr (BLIF format) from file]

  Description [see Fsm_MgrLoadFromBlif. Coi reduction is added.]
  SideEffects [None]

  SeeAlso     [Fsm_MgrLoadFromBlif]

******************************************************************************/

int
Fsm_MgrLoadFromBlifWithCoiReduction(
  Fsm_Mgr_t ** fsmMgrP /* FSM Pointer */ ,
  Ddi_Mgr_t * dd /* BDD manager */ ,
  char *fileFsmName /* FSM File Name */ ,
  char *fileOrdName /* ORD File Name */ ,
  int bddFlag /* Not Used For Now */ ,
  Pdtutil_VariableOrderFormat_e ordFileFormat /* Order File Format */ ,
  char **coiList                /* NULL terminated string array */
)
{
  Fsm_Mgr_t *fsmMgr;
  FsmPortBnetNetwork_t *net;    /* network */
  FsmPortNtrOptions_t *option;  /* options */
  FILE *fp = NULL;
  Ddi_Var_t *var;
  int i, j, flagFile, result, nTrueO;
  int xlevel;
  FsmPortBnetNode_t *node;
  char *name, *nameext;
  int index = 0, value, pointPos = -1;
  long fsmTime;
  Ddi_Bddarray_t *auxFunArray;
  Ddi_Vararray_t *auxVarArray;
  int auxVarNumber;
  char *fsmFileName;
  char varname[FSM_MAX_STR_LEN];
  size_t a;
  static FsmPortBnetNetwork_t *mynet;

  fsmTime = util_cpu_time();

  /*-------------------------- Check For FSM File  --------------------------*/

  nameext = Pdtutil_FileName(fileFsmName, "", "", 0);
  fp = Pdtutil_FileOpen(fp, nameext, "r", &flagFile);
  if (fp == NULL) {
    Pdtutil_Warning(1, "NULL Finite State Machine Pointer.");
    return (1);
  }
  //  Pdtutil_Free(nameext);n

  /*------------------------- FSM Manager Allocation ------------------------*/

  fsmMgr = *fsmMgrP;
  if (fsmMgr == NULL) {
    Pdtutil_Warning(1, "Allocate a new FSM structure");
    fsmMgr = Fsm_MgrInit(NULL, NULL);
  }

  Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "(Loading File %s).", fileFsmName)
    );

  /*------------------------------ Init Options -----------------------------*/

  option = mainInit();

  if (fileOrdName != NULL) {
    if (strcmp(fileOrdName, "file") == 0) {
      /* Enable Using Order From Blif File (default) */
      option->ordering = PI_PS_FROM_FILE;
    } else {
      if (strcmp(fileOrdName, "dfs") == 0) {
        /* Enable Using Depth First Search */
        option->ordering = PI_PS_DFS;
      } else {
        /* Enable Reading Order From File */
        option->ordering = PI_PS_GIVEN;
        option->orderPiPs = Pdtutil_StrDup(fileOrdName);
      }
    }
  }

  option->cutThresh = Fsm_MgrReadCutThresh(fsmMgr);
  option->useAig = Fsm_MgrReadUseAig(fsmMgr);
  if (option->useAig) {
    Ddi_AigInit(dd, 100);
    option->cutThresh = -1;     /* disable cut thresh */
  }

  /*------------------ Load description of the FSM Manager ------------------*/

  a = strlen(nameext);
  for (i = a; i > 0; i--) {
    if (nameext[i] == '.') {
      pointPos = i;
      break;
    }
  }
  if (nameext[pointPos + 1] == 'b')
    net = Fsm_PortBnetReadNetwork(fp, option->verb);
  else if (nameext[pointPos + 1] == 'a')
    net = Fsm_ReadAigFile(fp, fileFsmName);
  else {
    fprintf(stderr, "Error: Wrong Input File Format.\n");
  }
  Pdtutil_Warning(net == NULL, "Error : Description of fsmMgr not loaded.\n");
  if (net == NULL) {
    return (1);
  }

  Pdtutil_FileClose(fp, &flagFile);

  /*----------------------------- Coi Reduction -----------------------------*/

  if (coiList != NULL && coiList[0] != NULL) {
    option->coiReduction = TRUE;
    FsmCoiReduction(net, coiList);
  }

  /*-------------- Build the BDDs for the Nodes of the Network --------------*/

  mynet = net;

  //  option->progress = 1;

  result = Fsm_PortNtrBuildDDs(net, dd, option, ordFileFormat);
  Pdtutil_Warning(result == 0, "Impossible to build the BDDs for the network");
  if (result == 0) {
    return (1);
  }
#if 0
  /* since Fsm_PortNtrBuildDDs has operated on CUDD manager only,
     DDI mgr update is required */
  Ddi_MgrUpdate(dd);
#endif

  Ddi_MgrConsistencyCheck(dd);
  i = Ddi_MgrCheckZeroRef(dd);
  if (i != 0) {
    fprintf(stderr, "Reference counts: %d Non-zero DDs after building DDs.\n",
      i);
  }

  /*------- Translate CUDD format in our internal FSM Manager Format --------*/

  Fsm_MgrSetDdManager(fsmMgr, dd);

  /* Number of Primary Outputs */
  /* filter out constraints */
  for (i = nTrueO = 0; i < net->npos; i++) {
    if (!net->outputIsConstraint[i]) {
      nTrueO++;
    }
  }

  /* BDD of Initial & Reached State Set */
  if (net->nlatches > 0) {
    fsmMgr->init.bdd = Fsm_PortNtrInitState(dd, net, option);
    fsmMgr->reached.bdd = Ddi_BddDup(fsmMgr->init.bdd);
  } else {
    fsmMgr->init.bdd = NULL;
    fsmMgr->reached.bdd = NULL;
  }

  /*
   *  Init Temporary Set and Array
   */

  Ddi_MgrConsistencyCheck(dd);

  if (net->nlatches > 0) {
    fsmMgr->var.ps = Ddi_VararrayAlloc(dd, net->nlatches);
    fsmMgr->var.ns = Ddi_VararrayAlloc(dd, net->nlatches); 
    fsmMgr->delta.bdd = Ddi_BddarrayAlloc(dd, net->nlatches);
    fsmMgr->init.string = Pdtutil_Alloc(char,
      net->nlatches + 2
    );
  } else {
    fsmMgr->var.ps = NULL;
    fsmMgr->var.ns = NULL;
    fsmMgr->delta.bdd = NULL;
  }

  fsmMgr->lambda.bdd = Ddi_BddarrayAlloc(dd, nTrueO);

  if (Fsm_MgrReadUseAig(fsmMgr)) {
    fsmMgr->constraint.bdd = Ddi_BddMakeConstAig(dd, 1);
  } else {
    fsmMgr->constraint.bdd = Ddi_BddMakeConst(dd, 1);
  }

  Ddi_MgrConsistencyCheck(dd);

  /*------------------ Translate Array of Input Variables -------------------*/

  /* GpC: force void input array when no input found */
  if (1) {
    /*  if (net->npis > 0) { */
    fsmMgr->var.i = Ddi_VararrayAlloc(dd, net->npis);
    for (i = 0; i < net->npis; i++) {
      if (!st_lookup(net->hash, net->inputs[i], (char **)&node)) {
        return (1);
      }

      var = node->var;
      Ddi_VararrayWrite(fsmMgr->var.i, i, var);
      Ddi_VarAttachAuxid(var, i);
      // this variable already has a name!
      //Ddi_VarAttachName(var, net->inputs[i]);

      //printf("PI%02d: %s ", i, Ddi_VarName(var));
      //printf("(%d) %d\n", Ddi_VarIndex(var), Ddi_VarAuxid(var));
    }
  } else {
    fsmMgr->var.i = NULL;
  }

  Ddi_MgrConsistencyCheck(dd);

  /*-------- Translate Array of Present State Variables and Delta -----------*/

  for (i = 0; i < net->nlatches; i++) {
    if (!st_lookup(net->hash, net->latches[i][1], (char **)&node)) {
      return (1);
    }

    char val = net->latches[i][2][0];

    if (val < '2') {
      fsmMgr->init.string[i] = val;
    } else {
      fsmMgr->init.string[i] = 'x';
    }

    var = node->var;
    Ddi_VararrayWrite(fsmMgr->var.ps, i, var);
    Ddi_VarAttachAuxid(var, i+net->npis);
    // this variable already has a name!
    //Ddi_VarAttachName(var, net->latches[i][1]);

    //printf("PS%02d: %s ", i, Ddi_VarName(var));
    //printf("(%d) %d\n", Ddi_VarIndex(var), Ddi_VarAuxid(var));

    /* Create array of next state var */
    xlevel = Ddi_VarCurrPos(var) + 1;
#if 1
    var = Ddi_VarNewAtLevel(dd, xlevel);
#else
    var = Ddi_VarNew(dd);
#endif
    Ddi_VararrayWrite(fsmMgr->var.ns, i, var);
    Ddi_VarAttachAuxid(var, i+net->npis+net->nlatches);
    sprintf(varname, "%s$NS", net->latches[i][1]);
    Ddi_VarAttachName(var, varname);

    //printf("NS%02d: %s ", i, Ddi_VarName(var));
    //printf("(%d) %d\n", Ddi_VarIndex(var), Ddi_VarAuxid(var));

    if (node->coiFlag != 2) {

      /* Translate array of next state function (delta) */

      if (!st_lookup(net->hash, net->latches[i][0], (char **)&node)) {
        return (1);
      }

    }

    Ddi_BddarrayWrite(fsmMgr->delta.bdd, i, node->dd);
  }
  fsmMgr->init.string[net->nlatches] = '\0';

  Ddi_MgrConsistencyCheck(dd);

  /*---------------- Translate Array of Output Function ---------------------*/

  for (i = j = 0; i < net->npos; i++) {
    if (!st_lookup(net->hash, net->outputs[i], (char **)&node)) {
      return (1);
    }

    if (net->outputIsConstraint[i]) {
      Ddi_BddAndAcc(fsmMgr->constraint.bdd, node->dd);
      continue;
    }
    Ddi_BddarrayWrite(fsmMgr->lambda.bdd, j, node->dd);
    j++;
  }

  auxVarNumber = 0;
  auxFunArray = Ddi_BddarrayAlloc(dd, 0);
  auxVarArray = Ddi_VararrayAlloc(dd, 0);

  /*------------------ Manage Manual Auxiliary Variables --------------------*/

  if (0) {
    extern Ddi_Bdd_t *manualAuxTot;
    extern Ddi_Bddarray_t *manualAux;
    extern Ddi_Vararray_t *manualAuxVars;
    int i;

    if (manualAuxTot != NULL) {
      for (i = 0; i < Ddi_VararrayNum(manualAuxVars); i++) {
        Ddi_Var_t *v = Ddi_VararrayRead(manualAuxVars, i);

        if (Ddi_VarName(v) == NULL) {
          sprintf(varname, "AUXVAR$%d", auxVarNumber);
          Ddi_VarAttachName(v, varname);
        }
        index = Ddi_VarIndex(v);
        Ddi_VarAttachAuxid(v, index);
        Ddi_VararrayWrite(auxVarArray, auxVarNumber, v);
        Ddi_BddarrayInsertLast(auxFunArray, Ddi_BddarrayRead(manualAux, i));
        auxVarNumber++;
      }

      Ddi_Free(manualAuxTot);
      Ddi_Free(manualAux);
      Ddi_Free(manualAuxVars);
    }
  }

  /*----------------- Manage Automatic Auxiliary Variables ------------------*/

  for (node = net->nodes; node != NULL; node = node->next) {
    if (node->cutpoint) {
      index = Ddi_VarIndex(node->var);
      Ddi_VarAttachAuxid(node->var, index);
      if (Ddi_VarName(node->var) == NULL) {
        Pdtutil_Warning(1, "NULL Name on Aux Variable.");
        sprintf(varname, "AUXVAR$%d$%s", auxVarNumber, node->name);
        Ddi_VarAttachName(node->var, varname);
      }
      //index++;
      Ddi_VararrayWrite(auxVarArray, auxVarNumber, node->var);
      Ddi_BddarrayInsertLast(auxFunArray, node->cutdd);
      auxVarNumber++;

      Ddi_Free(node->cutdd);
    }
    Ddi_Free(node->dd);
  }

  if (auxVarNumber > 0) {
    Fsm_MgrSetVarAuxVar(fsmMgr, auxVarArray);
    Fsm_MgrSetAuxVarBDD(fsmMgr, auxFunArray);
  }

  Ddi_Free(auxFunArray);
  Ddi_Free(auxVarArray);

  /*-------------------- Set Names of Variables -----------------------------*/

  //if (Fsm_MgrReadVarnames(fsmMgr) != NULL) {
  if (1) {
#if 1
    if (nameext[pointPos + 1] == 'b')
      Fsm_PortBnetFreeNetwork(net);

    Pdtutil_Free(nameext);
#endif
    Ddi_MgrConsistencyCheck(dd);
  }

  /*---------------------- Safe Test and Return -----------------------------*/

  if (net->nlatches == 0) {
    /* always enables */
    Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Combinational Network.\n")
      );
  }

  Pdtutil_Free(option);

  /*
   *  return 0 = All OK
   */

  fsmTime = util_cpu_time() - fsmTime;
  fsmMgr->fsmTime += fsmTime;

  *fsmMgrP = fsmMgr;
  return (0);
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Allocates the option structure for BLIF format and initializes
    it.]

  Description [This function was taken from main.c in <b>nanotrav</b> package]

  SideEffects [none]

  SeeAlso     [Fsm_MgrLoadFromBlif]

******************************************************************************/

static FsmPortNtrOptions_t *
mainInit(
)
{
  FsmPortNtrOptions_t *option;

  /* Initialize option structure. */
  option = Pdtutil_Alloc(FsmPortNtrOptions_t, 1);

  option->initialTime = util_cpu_time();
  option->verify = FALSE;
  option->second = FALSE;
  option->file1 = NULL;
  option->file2 = NULL;
  option->traverse = FALSE;
  option->depend = FALSE;
  option->image = PORT_NTRIMAGE_MONO;
  option->imageClip = 1.0;
  option->approx = PORT_NTRUNDER_APPROX;
  option->threshold = -1;
  option->from = PORT_NTRFROM_NEW;
  option->groupnsps = PORT_NTRGROUP_NONE;
  option->closure = FALSE;
  option->closureClip = 1.0;
  option->envelope = FALSE;
  option->scc = FALSE;
  option->maxflow = FALSE;
  option->zddtest = FALSE;
  option->sinkfile = NULL;
  option->partition = FALSE;
  option->char2vect = FALSE;
  option->density = FALSE;
  option->quality = 1.0;
  option->decomp = FALSE;
  option->cofest = FALSE;
  option->clip = -1.0;
  option->noBuild = FALSE;
  option->stateOnly = FALSE;
  option->node = NULL;
  option->locGlob = PORT_BNETGLOBAL_DD;
  option->progress = FALSE;
  option->cacheSize = DDI_CACHE_SLOTS;
  option->maxMemory = 0;        /* set automatically */
  option->slots = CUDD_UNIQUE_SLOTS;

  /*
   * Create Variable Order while Building the BDDs
   */

  option->ordering = PI_PS_FROM_FILE;
  option->orderPiPs = NULL;
  option->reordering = CUDD_REORDER_NONE;
  option->autoMethod = CUDD_REORDER_SIFT;
  option->autoDyn = 0;
  option->treefile = NULL;
  option->firstReorder = DD_FIRST_REORDER;
  option->countDead = FALSE;
  option->maxGrowth = 20;
  option->groupcheck = CUDD_GROUP_CHECK7;
  option->arcviolation = 10;
  option->symmviolation = 10;
  option->recomb = DD_DEFAULT_RECOMB;
  option->nodrop = FALSE;
  option->signatures = FALSE;
  option->verb = 0;
  option->gaOnOff = 0;
  option->populationSize = 0;   /* use default */
  option->numberXovers = 0;     /* use default */
  option->bdddump = FALSE;
  option->dumpFmt = 0;          /* dot */
  option->dumpfile = NULL;
  option->store = -1;           /* do not store */
  option->storefile = NULL;
  option->load = FALSE;
  option->loadfile = NULL;
  option->cutThresh = -1;
  option->coiReduction = FALSE;
  option->useAig = FALSE;

  return (option);
}

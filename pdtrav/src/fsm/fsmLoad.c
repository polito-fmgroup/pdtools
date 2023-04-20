/**CFile***********************************************************************

  FileName    [fsmLoad.c]

  PackageName [fsm]

  Synopsis    [Functions to read BDD based description of FSMs]

  Description [External procedures included in this module:
    <ul>
    <li> Fsm_MgrLoad ()
    </ul>
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

#include "aiger.h"
#include "fsm.h"
#include "fsmInt.h"

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

static int readFsm(
  FILE * fp,
  Fsm_Mgr_t * fsmMgr,
  char *fileOrdName,
  int bddFlag,
  Pdtutil_VariableOrderFormat_e ordFileFormat
);
static void readFsmSize(
  FILE * fp,
  FsmSize_t *size,
  char *word
);
static void readFsmOrd(
  FILE * fp,
  Fsm_Mgr_t * fsmMgr,
  char *fileOrdName,
  char *word,
  Pdtutil_VariableOrderFormat_e ordFileFormat
);
static void readFsmName(
  FILE * fp,
  FsmSize_t size,
  FsmNodeName_t *name,
  char *word
);
static void readFsmIndex(
  FILE * fp,
  FsmSize_t size,
  FsmNodeId_t *index,
  char *word
);
static void readFsmDelta(
  FILE * fp,
  Fsm_Mgr_t * fsmMgr,
  char *word,
  int bddFlag
);
static void readFsmLambda(
  FILE * fp,
  Fsm_Mgr_t * fsmMgr,
  char *word,
  int bddFlag
);
static void readFsmAuxFun(
  FILE * fp,
  Fsm_Mgr_t * fsmMgr,
  char *word,
  int bddFlag
);
static void readFsmTransRel(
  FILE * fp,
  Fsm_Mgr_t * fsmMgr,
  char *word,
  int bddFlag
);
static void readFsmFrom(
  FILE * fp,
  Fsm_Mgr_t * fsmMgr,
  char *word,
  int bddFlag
);
static void readFsmReached(
  FILE * fp,
  Fsm_Mgr_t * fsmMgr,
  char *word,
  int bddFlag
);
static void readFsmInitState(
  FILE * fp,
  Fsm_Mgr_t * fsmMgr,
  char *word,
  int bddFlag
);
static int readOrdFile(
  FILE * fp,
  char ***varnames_ptr,
  int **varauxids_ptr,
  Pdtutil_VariableOrderFormat_e ordFileFormat
);
static void readBddFile(
  FILE * fp,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Mgr_t * dd,
  char *word,
  char **pName,
  Ddi_Bdd_t ** pBdd,
  Ddi_Bddarray_t ** pBddarray,
  int bddFlag
);
static Fsm_TokenType get_token(
  FILE * fp,
  char *word
);
static int get_number(
  FILE * fp,
  char *word
);
static char **get_string_array(
  FILE * fp,
  char *word,
  int size
);
static int *get_integer_array(
  FILE * fp,
  char *word,
  int size
);

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis           []

  Description        []

  SideEffects        []

  SeeAlso            [get_name_file,readFsm]

******************************************************************************/

int
Fsm_MgrLoad(
  Fsm_Mgr_t ** fsmMgrP /* FSM Pointer */ ,
  Ddi_Mgr_t * dd /* Main DD manager */ ,
  char *fileFsmName /* Input file name */ ,
  char *fileOrdName /* ORD File Name */ ,
  int bddFlag /* 0=Do non load BDD (default), 1=Load BDD */ ,
  Pdtutil_VariableOrderFormat_e ordFileFormat
)
{
  Fsm_Mgr_t *fsmMgr;
  FILE *fp = NULL;
  int i, flagFile;
  int flagError;
  char *nameext;
  long fsmTime;

  fsmTime = util_cpu_time();

  /*-------------------------- Check For FSM File  --------------------------*/

  nameext = Pdtutil_FileName(fileFsmName, (char *)"", (char *)"fsm", 0);
  fp = Pdtutil_FileOpen(fp, nameext, "r", &flagFile);
  Pdtutil_Free(nameext);
  if (fp == NULL) {
    Pdtutil_Warning(1, "NULL Finite State Machine Pointer.");
    return (1);
  }

  /*------------------------ Check For FSM Structure ------------------------*/

  fsmMgr = *fsmMgrP;
  if (fsmMgr == NULL) {
    Pdtutil_Warning(1, "Allocate a new FSM structure");
    fsmMgr = Fsm_MgrInit(NULL, NULL);
  }

  Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "Loading File (%s).\n", fileFsmName);
    );

  Fsm_MgrSetDdManager(fsmMgr, dd);

  /*------------------------- Read the FSM Structure ------------------------*/

  flagError = readFsm(fp, fsmMgr, fileOrdName, bddFlag, ordFileFormat);

  /*--------------- Check for Errors and Perform Final Copies ---------------*/

  /*
   *  return 1 = An Error Occurred
   */

  if (flagError == 1) {
    Fsm_MgrQuit(fsmMgr);
    return (1);
  }

  Pdtutil_FileClose(fp, &flagFile);

  /*
   *  Set Output Names
   */

  for (i = 0; i < Fsm_MgrReadNO(fsmMgr); i++) {
    char nameOut[PDTUTIL_MAX_STR_LEN];
    sprintf(nameOut, "out_%d", i);
    Ddi_SetName(Ddi_BddarrayRead(fsmMgr->lambda.bdd, i), nameOut);
  }

  if (Fsm_MgrReadUseAig(fsmMgr)) {
    int i;
    Ddi_Bddarray_t *d = Fsm_MgrReadDeltaBDD(fsmMgr);
    Ddi_Bddarray_t *l = Fsm_MgrReadLambdaBDD(fsmMgr);
    Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
    Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);

    Ddi_AigInit(dd, 100);
    Ddi_BddSetAig(Fsm_MgrReadInitBDD(fsmMgr));
    for (i = 0; i < Ddi_VararrayNum(ps); i++) {
      Ddi_Bdd_t *aux = Ddi_BddMakeLiteralAig(Ddi_VararrayRead(ps, i), 1);

      Ddi_Free(aux);
    }
    for (i = 0; i < Ddi_VararrayNum(pi); i++) {
      Ddi_Bdd_t *aux = Ddi_BddMakeLiteralAig(Ddi_VararrayRead(pi, i), 1);

      Ddi_Free(aux);
    }
    for (i = 0; i < Ddi_BddarrayNum(d); i++) {
      Ddi_BddSetAig(Ddi_BddarrayRead(d, i));
    }
    for (i = 0; i < Ddi_BddarrayNum(l); i++) {
      Ddi_BddSetAig(Ddi_BddarrayRead(l, i));
    }
    if (Fsm_MgrReadNAuxVar(fsmMgr) > 0) {
      Ddi_Vararray_t *aV = Fsm_MgrReadVarAuxVar(fsmMgr);
      Ddi_Bddarray_t *a = Fsm_MgrReadAuxVarBDD(fsmMgr);

      for (i = 0; i < Ddi_BddarrayNum(a); i++) {
        Ddi_Bdd_t *aux = Ddi_BddMakeLiteralAig(Ddi_VararrayRead(aV, i), 1);

        Ddi_Free(aux);
        Ddi_BddSetAig(Ddi_BddarrayRead(a, i));
      }
      d = Ddi_AigarrayCompose(Fsm_MgrReadDeltaBDD(fsmMgr), aV, a);
      l = Ddi_AigarrayCompose(Fsm_MgrReadLambdaBDD(fsmMgr), aV, a);
      Fsm_MgrSetDeltaBDD(fsmMgr, d);
      Fsm_MgrSetLambdaBDD(fsmMgr, l);
      Ddi_Free(d);
      Ddi_Free(l);
    }
  }

  /*
   *  return 0 = All OK
   */

  fsmTime = util_cpu_time() - fsmTime;
  fsmMgr->fsmTime += fsmTime;

  *fsmMgrP = fsmMgr;
  return (0);
}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Reads current section of the <em>.fsm</em> file.]

  SideEffects []

  SeeAlso     [readFsmSize,readFsmName,readFsmIndex,
    readFsmDelta,readFsmLambda,readFsmTransRel,readFsmInitState]

******************************************************************************/

static int
readFsm(
  FILE * fp /* File */ ,
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  char *fileOrdName /* File Name for the Order File */ ,
  int bddFlag,
  Pdtutil_VariableOrderFormat_e ordFileFormat
)
{
  char word[FSM_MAX_STR_LEN];
  Fsm_TokenType token;
  FsmSize_t size;
  FsmNodeName_t name;
  FsmNodeId_t index;
  Ddi_Mgr_t *dd = fsmMgr->dd;
  Ddi_Vararray_t *vars;
  Ddi_Var_t *v;
  char **names;
  int *auxids, nvars, i;

  size.i = 0;
  size.o = 0;
  size.l = 0;
  size.auxVar = 0;
  name.i = NULL;
  name.o = NULL;
  name.ps = NULL;
  name.ns = NULL;
  name.nsTrueName = NULL;
  name.auxVar = NULL;
  index.i = NULL;
  index.o = NULL;
  index.ps = NULL;
  index.ns = NULL;
  index.auxVar = NULL;

  switch (get_token(fp, word)) {
    case KeyFSM_FSM:
      if (get_token(fp, word) == KeyFSM_STRING) {
        Fsm_MgrSetFileName(fsmMgr, word);
      }
      break;
    default:
      fprintf(stderr, "Error: Wrong Keyword at the Beginning of the File.\n");
      return (1);
  }

  while ((token = get_token(fp, word)) != KeyFSM_END) {
    switch (token) {
      case KeyFSM_SIZE:
        readFsmSize(fp, &size, word);
        break;
      case KeyFSM_ORD:
        readFsmOrd(fp, fsmMgr, fileOrdName, word, ordFileFormat);
        nvars = Ddi_MgrReadNumVars(dd);
        if (nvars != size.i+size.auxVar+2*size.l) {
          Pdtutil_Warning(1, "Wrong number of variables in ord file.");
        }
        break;
      case KeyFSM_NAME:
        readFsmName(fp, size, &name, word);
        break;
      case KeyFSM_INDEX:
        readFsmIndex(fp, size, &index, word);
        break;
      case KeyFSM_DELTA:
        readFsmDelta(fp, fsmMgr, word, bddFlag);
        break;
      case KeyFSM_LAMBDA:
        readFsmLambda(fp, fsmMgr, word, bddFlag);
        break;
      case KeyFSM_AUXFUN:
        readFsmAuxFun(fp, fsmMgr, word, bddFlag);
        break;
      case KeyFSM_INITSTATE:
        readFsmInitState(fp, fsmMgr, word, bddFlag);
        break;
      case KeyFSM_TRANS_REL:
        readFsmTransRel(fp, fsmMgr, word, bddFlag);
        break;
      case KeyFSM_FROM:
        readFsmFrom(fp, fsmMgr, word, bddFlag);
        break;
      case KeyFSM_REACHED:
        readFsmReached(fp, fsmMgr, word, bddFlag);
        break;
      default:
        fprintf(stderr, "Error: Wrong Section (%s) in FSM file.\n", word);
        return (1);
    }
  }

  /*
   *  Set the field Variable - Input of the FSM
   */

  /* GpC: force void input array when no input found */
  if (1) {
    /*  if (size.i > 0) {  */
    vars = Ddi_VararrayAlloc(dd, 0);
    auxids = index.i;
    names = name.i;

    for (i = 0; i < size.i; i++) {
      if (names != NULL) {
        v = Ddi_VarFromName(dd, names[i]);
      } else {
        v = Ddi_VarFromAuxid(dd, auxids[i]);
      }
      Ddi_VararrayWrite(vars, i, v);
    }

    Fsm_MgrSetVarI(fsmMgr, vars);
    Ddi_Free(vars);
  } else {
    Fsm_MgrSetVarI(fsmMgr, NULL);
  }

  /*
   *  Set the fields Variable - PS and the NS of the FSM
   */

  if (size.l > 0) {

    /* Set PS */
    vars = Ddi_VararrayAlloc(dd, 0);
    auxids = index.ps;
    names = name.ps;


    for (i = 0; i < size.l; i++) {
      if (names != NULL) {
        v = Ddi_VarFromName(dd, names[i]);
      } else {
        v = Ddi_VarFromAuxid(dd, auxids[i]);
      }
      Ddi_VararrayWrite(vars, i, v);
    }

    Fsm_MgrSetVarPS(fsmMgr, vars);
    Ddi_Free(vars);

    /* Set NS */
    vars = Ddi_VararrayAlloc(dd, 0);
    auxids = index.ns;
    names = name.ns;

    for (i = 0; i < size.l; i++) {
      if (names != NULL) {
        v = Ddi_VarFromName(dd, names[i]);
      } else {
        v = Ddi_VarFromAuxid(dd, auxids[i]);
      }
      Ddi_VararrayWrite(vars, i, v);
    }

    Fsm_MgrSetVarNS(fsmMgr, vars);
    Ddi_Free(vars);

  } else {
    Fsm_MgrSetVarPS(fsmMgr, NULL);
    Fsm_MgrSetVarNS(fsmMgr, NULL);
  }

  /*
   *  Set the field Variable - AuxVar of the FSM
   */

  if (size.auxVar > 0) {
    vars = Ddi_VararrayAlloc(dd, 0);
    auxids = index.auxVar;
    names = name.auxVar;

    for (i = 0; i < size.auxVar; i++) {
      if (names != NULL) {
        v = Ddi_VarFromName(dd, names[i]);
      } else {
        v = Ddi_VarFromAuxid(dd, auxids[i]);
      }
      Ddi_VararrayWrite(vars, i, v);
    }

    Fsm_MgrSetVarAuxVar(fsmMgr, vars);
    Ddi_Free(vars);
  } else {
    Fsm_MgrSetVarAuxVar(fsmMgr, NULL);
  }

  Pdtutil_StrArrayFree(name.i, size.i);
  Pdtutil_StrArrayFree(name.o, size.o);
  Pdtutil_StrArrayFree(name.ps, size.l);
  Pdtutil_StrArrayFree(name.ns, size.l);
  Pdtutil_StrArrayFree(name.nsTrueName, size.l);
  Pdtutil_StrArrayFree(name.auxVar, size.auxVar);
  Pdtutil_Free(index.i);
  Pdtutil_Free(index.o);
  Pdtutil_Free(index.ps);
  Pdtutil_Free(index.ns);
  Pdtutil_Free(index.auxVar);

  return 0;
}


/**Function********************************************************************

  Synopsis    [Reads <em>.size</em> section of the <em>.fsm</em> file.]

  Description [Looks up ".size" section of the new ".fsm" file format
    for input, output and latch number to be loaded in the FSM structure.
    ]

  SideEffects []

  SeeAlso     [number]

******************************************************************************/

static void
readFsmSize(
  FILE *fp,
  FsmSize_t *size,
  char *word
)
{
  Fsm_TokenType token;

  token = get_token(fp, word);

  while (token != KeyFSM_END) {
    switch (token) {
      case KeyFSM_I:
        size->i = get_number(fp, word);
        break;
      case KeyFSM_O:
        size->o = get_number(fp, word);
        break;
      case KeyFSM_L:
        size->l = get_number(fp, word);
        break;
      case KeyFSM_AUXVAR:
        size->auxVar = get_number(fp, word);
        break;
      default:
        fprintf(stderr, "Error: Wrong Keyword in .size Segment (%s).\n", word);
        break;
    }

    /* Get Next Token */
    token = get_token(fp, word);
  }
}


/**Function********************************************************************

  Synopsis           [Reads <em>.ord</em> section of the <em>.fsm</em> file.]

  Description        []

  SideEffects        []

  SeeAlso            [get_string_array]

******************************************************************************/

static void
readFsmOrd(
  FILE * fp /* File Pointer */ ,
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  char *fileOrdName,            /* File Name for the Order File */
  char *word /* Current Word */ ,
  Pdtutil_VariableOrderFormat_e ordFileFormat
)
{
  FILE *filep = NULL;
  char *fileName;
  int i, flagFile;
  char **varnames=NULL;
  int *varauxids=NULL;
  Fsm_TokenType token;
  int nvars;
  Ddi_Mgr_t *dd = Fsm_MgrReadDdManager(fsmMgr);

  token = get_token(fp, word);

  while (token != KeyFSM_END) {
    switch (token) {
      case KeyFSM_FILE_ORD:
        get_token(fp, word);
        if (fileOrdName == NULL) {
          fileName = word;
        } else {
          /* OverWrite Internal (of FSM structure) File Name */
          fileName = fileOrdName;
        }
        filep = Pdtutil_FileOpen(filep, fileName, "r", &flagFile);
        Pdtutil_Warning(filep == NULL, "Cannot open FSM file.");
        nvars = readOrdFile(filep, &varnames, &varauxids, ordFileFormat);
        Pdtutil_FileClose(filep, &flagFile);
        break;
      default:
        fprintf(stderr,
          "Error: Wrong keyword (%s); Keyword .ord expected.\n", word);
        break;
    }
    token = get_token(fp, word);
  }

  if (varauxids == NULL) {
    fprintf(stderr, "Order withoud IDs not yet Supported.\n");
    exit(1);
  }
  while (nvars > Ddi_MgrReadNumVars(dd)) {
    Ddi_VarNew(dd);
  }
  for (i = 0; i < nvars; i++) {
    Ddi_VarAttachName(Ddi_VarFromIndex(dd, i), varnames[i]);
    Ddi_VarAttachAuxid(Ddi_VarFromIndex(dd, i), varauxids[i]);
  }
  Pdtutil_StrArrayFree(varnames, nvars);
  Pdtutil_Free(varauxids);
}


/**Function********************************************************************

  Synopsis           [Reads <em>.name</em> section of the <em>.fsm</em> file.]

  Description        [Reads ".name" section of the new ".fsm" file format
    and then loads the FSM structure by means of "get_string_array"
    with input (".i"), output (".o"), latch input (".ps") and latch
    output (".ns") names in the circuit.]

  SideEffects        []

  SeeAlso            [get_string_array]

******************************************************************************/

static void
readFsmName(
  FILE * fp,
  FsmSize_t size,
  FsmNodeName_t *name,
  char *word
)
{
  Fsm_TokenType token;
  char **tmpArray;

  while ((token = get_token(fp, word)) != KeyFSM_END) {
    switch (token) {
      case KeyFSM_I:
        tmpArray = get_string_array(fp, word, size.i);
        name->i = tmpArray;
        break;
      case KeyFSM_O:
        tmpArray = get_string_array(fp, word, size.o);
        name->o = tmpArray;
        break;
      case KeyFSM_PS:
        tmpArray = get_string_array(fp, word, size.l);
        name->ps = tmpArray;
        break;
      case KeyFSM_NS:
        tmpArray = get_string_array(fp, word, size.l);
        name->ns = tmpArray;
        break;
      case KeyFSM_AUXVAR:
        tmpArray = get_string_array(fp, word, size.auxVar);
        name->auxVar = tmpArray;
        break;
      default:
        fprintf(stderr, "Error: String (%s) in Section .name NOT recognized.\n",
          word);
        exit(1);
    }
  }
}


/**Function********************************************************************

  Synopsis           [Reads <em>.index</em> section of the <em>.fsm</em> file.]

  Description        [Reads ".name" section of the new ".fsm" file format
    and then loads the FSM structure by means of "get_integer_array"
    with input (".i"), output (".o"), latch input (".ps") and latch output
    (".ns") indexes in the circuit.]

  SideEffects        []

  SeeAlso            [get_integer_array]

******************************************************************************/

static void
readFsmIndex(
  FILE * fp,
  FsmSize_t size,
  FsmNodeId_t *index,
  char *word
)
{
  Fsm_TokenType token;
  int *tmpArray;
  // nxr!!!

  while ((token = get_token(fp, word)) != KeyFSM_END) {
    switch (token) {
      case KeyFSM_I:
        tmpArray = get_integer_array(fp, word, size.i);
        index->i = tmpArray;
        break;
      case KeyFSM_O:
        tmpArray = get_integer_array(fp, word, size.o);
        index->o = tmpArray;
        break;
      case KeyFSM_PS:
        tmpArray = get_integer_array(fp, word, size.l);
        index->ps = tmpArray;
        break;
      case KeyFSM_NS:
        tmpArray = get_integer_array(fp, word, size.l);
        index->ns = tmpArray;
        break;
      case KeyFSM_AUXVAR:
        tmpArray = get_integer_array(fp, word, size.auxVar);
        index->auxVar = tmpArray;
        break;
      default:
        fprintf(stderr,
          "Error: Option (%s) in Section .index Not Supported.\n", word);
        exit(1);
    }
  }
}


/**Function********************************************************************

  Synopsis           [Reads <em>.delta</em> section of the <em>.fsm</em> file.]

  Description        [Reads ".delta" section of the new ".fsm" file format
    and then loads the FSM structure with future state function BDDs.
    In order to do that, uses "Ddi_BddarrayLoad" procedure,
    preceded by "Pdtutil_FileOpen" and followed by "Pdtutil_FileClose" in case
    BDD is specified with a fileName.]

  SideEffects        []

  SeeAlso            [Poli_bddRead_new]

******************************************************************************/

static void
readFsmDelta(
  FILE * fp /* File */ ,
  Fsm_Mgr_t * fsmMgr /* FSM Manager */ ,
  char *word /* Current Word */ ,
  int bddFlag
)
{
  char *name;                   /* delta file name */
  Ddi_Bddarray_t *delta;        /* array of bdds */

  readBddFile(fp, fsmMgr, Fsm_MgrReadDdManager(fsmMgr), word,
    &name, NULL, &delta, bddFlag);

  Fsm_MgrSetDeltaName(fsmMgr, name);
  Pdtutil_Free(name);
  if (bddFlag == 1) {
    Fsm_MgrSetDeltaBDD(fsmMgr, delta);
    Ddi_Free(delta);
  } else {
    Fsm_MgrSetDeltaBDD(fsmMgr, NULL);
  }
}

/**Function********************************************************************

  Synopsis           [Reads <em>.lambda</em> section of the
    <em>.fsm</em> file.
    ]

  Description        [Reads ".lambda" section of the new ".fsm" file format
    and then loads the FSM structure with output function BDD.
    In order to do that, it uses "Ddi_BddarrayLoad" procedure, preceded by
    "Pdtutil_FileOpen" and followed by "Pdtutil_FileClose" in case
    BDD is specified with a fileName.
    ]

  SideEffects        []

  SeeAlso            [Poli_bddRead_new]

******************************************************************************/

static void
readFsmLambda(
  FILE * fp /* file */ ,
  Fsm_Mgr_t * fsmMgr /* struttura FSM */ ,
  char *word /* parola corrente */ ,
  int bddFlag
)
{
  char *name;                   /* lambda file name */
  Ddi_Bddarray_t *lambda;       /* array of bdds */

  readBddFile(fp, fsmMgr, Fsm_MgrReadDdManager(fsmMgr), word,
    &name, NULL, &lambda, bddFlag);

  Fsm_MgrSetLambdaName(fsmMgr, name);
  Pdtutil_Free(name);

  if (bddFlag == 1) {
    Fsm_MgrSetLambdaBDD(fsmMgr, lambda);
    Ddi_Free(lambda);
  } else {
    Fsm_MgrSetLambdaBDD(fsmMgr, NULL);
  }

  return;
}

/**Function********************************************************************

  Synopsis           [Reads <em>.cut</em> section of the
    <em>.fsm</em> file.
    ]

  Description        [Reads ".cut" section of the new ".fsm" file format
    and then loads the FSM structure with the cut point function BDDs.
    In order to do that, it uses "Ddi_BddarrayLoad" procedure, preceded by
    "Pdtutil_FileOpen" and followed by "Pdtutil_FileClose" in case
    BDD is specified with a fileName.
    ]

  SideEffects        []

  SeeAlso            [Poli_bddRead_new]

******************************************************************************/

static void
readFsmAuxFun(
  FILE * fp /* file */ ,
  Fsm_Mgr_t * fsmMgr /* struttura FSM */ ,
  char *word /* parola corrente */ ,
  int bddFlag
)
{
  char *name;                   /* lambda file name */
  Ddi_Bddarray_t *auxFun;       /* array of bdds */

  readBddFile(fp, fsmMgr, Fsm_MgrReadDdManager(fsmMgr), word,
    &name, NULL, &auxFun, bddFlag);

  Fsm_MgrSetAuxVarName(fsmMgr, name);
  Pdtutil_Free(name);

  if (bddFlag == 1) {
    Fsm_MgrSetAuxVarBDD(fsmMgr, auxFun);
    Ddi_Free(auxFun);
  } else {
    Fsm_MgrSetAuxVarBDD(fsmMgr, NULL);
  }

  return;
}


/**Function********************************************************************

  Synopsis           [Reads <em>.tr</em> section of the <em>.fsm</em> file.]

  Description        [Reads ".tr" section of the new ".fsm" file format
    and then loads the FSM structure with transition relation BDD.
    In order to do that, it uses "Ddi_BddarrayLoad" procedure, preceded by
    "Pdtutil_FileOpen" and followed by "Pdtutil_FileClose" in case
    BDD is specified with a fileName.
    ]

  SideEffects        []

  SeeAlso            [Poli_bddRead_new]

******************************************************************************/

static void
readFsmTransRel(
  FILE * fp /* file */ ,
  Fsm_Mgr_t * fsmMgr /* struttura FSM */ ,
  char *word /* parola corrente */ ,
  int bddFlag
)
{
  char *name;                   /* tr file name */
  Ddi_Bdd_t *tr;                /* bdd */

  readBddFile(fp, fsmMgr, Fsm_MgrReadDdManager(fsmMgr), word, &name,
    &tr, NULL, bddFlag);

  Fsm_MgrSetTrName(fsmMgr, name);
  Pdtutil_Free(name);
  if (bddFlag == 1) {
    Fsm_MgrSetTrBDD(fsmMgr, tr);
    Ddi_Free(tr);
  } else {
    Fsm_MgrSetTrBDD(fsmMgr, NULL);
  }
}

/**Function********************************************************************

  Synopsis           [Reads <em>.tr</em> section of the <em>.fsm</em> file.]

  Description        [Reads ".from" section.]

  SideEffects        []

  SeeAlso            [Poli_bddRead_new]

******************************************************************************/

static void
readFsmFrom(
  FILE * fp /* file */ ,
  Fsm_Mgr_t * fsmMgr /* struttura FSM */ ,
  char *word /* parola corrente */ ,
  int bddFlag
)
{
  char *name;                   /* tr file name */
  Ddi_Bdd_t *from;              /* bdd */

  readBddFile(fp, fsmMgr, Fsm_MgrReadDdManager(fsmMgr), word, &name,
    &from, NULL, bddFlag);

  Fsm_MgrSetFromName(fsmMgr, name);
  Pdtutil_Free(name);
  if (bddFlag == 1) {
    Fsm_MgrSetFromBDD(fsmMgr, from);
    Ddi_Free(from);
  } else {
    Fsm_MgrSetFromBDD(fsmMgr, NULL);
  }

  return;
}

/**Function********************************************************************

  Synopsis           [Reads <em>.tr</em> section of the <em>.fsm</em> file.]

  Description        [Reads ".reached" section.]

  SideEffects        []

  SeeAlso            [Poli_bddRead_new]

******************************************************************************/

static void
readFsmReached(
  FILE * fp /* file */ ,
  Fsm_Mgr_t * fsmMgr /* struttura FSM */ ,
  char *word /* parola corrente */ ,
  int bddFlag
)
{
  char *name;                   /* tr file name */
  Ddi_Bdd_t *reached;           /* bdd */

  readBddFile(fp, fsmMgr, Fsm_MgrReadDdManager(fsmMgr), word, &name,
    &reached, NULL, bddFlag);

  Fsm_MgrSetReachedName(fsmMgr, name);
  Pdtutil_Free(name);
  if (bddFlag == 1) {
    Fsm_MgrSetReachedBDD(fsmMgr, reached);
    Ddi_Free(reached);
  } else {
    Fsm_MgrSetReachedBDD(fsmMgr, NULL);
  }

  return;
}


/**Function********************************************************************

  Synopsis     [Reads <em>.init</em> section of the <em>.fsm</em> file.]

  Description  [Reads ".init" section of the new ".fsm" file format
    and then loads the FSM structure with initial state BDD.
    In order to do that, it uses "Ddi_BddarrayLoad" procedure, preceded by
    "Pdtutil_FileOpen" and followed by "Pdtutil_FileClose" in case
    BDD is specified with a fileName.
    ]

  SideEffects []

  SeeAlso     [Poli_bddRead_new,readInitFile]

******************************************************************************/

static void
readFsmInitState(
  FILE * fp /* file */ ,
  Fsm_Mgr_t * fsmMgr /* struttura FSM */ ,
  char *word /* parola corrente */ ,
  int bddFlag
)
{
  char *name;                   /* s0 file name */
  Ddi_Bdd_t *s0;                /* bdd */

  readBddFile(fp, fsmMgr, Fsm_MgrReadDdManager(fsmMgr), word, &name,
    &s0, NULL, bddFlag);

  Fsm_MgrSetInitName(fsmMgr, name);
  Pdtutil_Free(name);
  if (bddFlag == 1) {
    Fsm_MgrSetInitBDD(fsmMgr, s0);
    Ddi_Free(s0);
  } else {
    Fsm_MgrSetInitBDD(fsmMgr, NULL);
  }

  return;
}


/**Function********************************************************************

  Synopsis           [Reads the order in the <em>.ord</em> file.]

  Description        []

  SideEffects        []

******************************************************************************/

static int
readOrdFile(
  FILE * fp /* File Pointer */ ,
  char ***varnames_ptr,
  int **varauxids_ptr,
  Pdtutil_VariableOrderFormat_e ordFileFormat
)
{
  int nvars;

  nvars = Pdtutil_OrdRead(varnames_ptr, varauxids_ptr, NULL, fp, ordFileFormat);

  if (nvars < 0) {
    Pdtutil_Warning(1, "Error reading variable ordering.");
  }

  return nvars;
}

/**Function********************************************************************

  Synopsis           [Read a bdd file]

  Description        [Read a BDD dump file. Returns an array of BDD roots]

  SideEffects        []

  SeeAlso            []

******************************************************************************/

static void
readBddFile(
  FILE * fp /* file */ ,
  Fsm_Mgr_t * fsmMgr /* struttura FSM */ ,
  Ddi_Mgr_t * dd /* DD manager */ ,
  char *word /* parola corrente */ ,
  char **pName /* return file name by reference */ ,
  Ddi_Bdd_t ** pBdd /* return bdd by reference */ ,
  Ddi_Bddarray_t ** pBddarray /* return bdd array by reference */ ,
  int bddFlag                   /* Load - NotLoad BDD from files */
)
{
  Fsm_TokenType token;
  FILE *bddfp;
  char **varnames=Ddi_MgrReadVarnames(fsmMgr->dd);
  int *varauxids=Ddi_MgrReadVarauxids(fsmMgr->dd);

  *pName = NULL;
  bddfp = fp;

  token = get_token(fp, word);
  while (token != KeyFSM_END) {
    switch (token) {
      case KeyFSM_FILE_BDD:
        get_token(fp, word);
        *pName = Pdtutil_StrDup(word);
        bddfp = NULL;
      case KeyFSM_FILE_TEXT:
        if (bddFlag == 1) {
          if (pBdd != NULL) {
            *pBdd = Ddi_BddLoad(dd, DDDMP_VAR_MATCHNAMES,
              DDDMP_MODE_DEFAULT, *pName, bddfp);
          } else {
            *pBddarray = Ddi_BddarrayLoad(dd, varnames, varauxids,
              DDDMP_MODE_DEFAULT, *pName, bddfp);
          }
        }
        break;
      default:
        fprintf(stderr, "Error: Option (%s) Not Supported.\n", word);
        exit(1);
        break;
    }

    token = get_token(fp, word);
  }

  return;
}

/**Function********************************************************************

  Synopsis           [Read a String from the File.]

  Description        [Read a String from the File. It returns the
    string and a code that indicates the type of the string.
    ]

  SideEffects        []

  SeeAlso            [get_line]

******************************************************************************/

static Fsm_TokenType
get_token(
  FILE * fp,
  char *word                    /* Current Word */
)
{
  Fsm_TokenType val;

  fscanf(fp, "%s", word);

  val = FsmString2Token(word);
  return (val);
}


/**Function********************************************************************

  Synopsis           [Returns the current word as integer.]

  SideEffects        []

******************************************************************************/

static int
get_number(
  FILE * fp,
  char *word
)
{
  int num;

  if (get_token(fp, word) != KeyFSM_STRING) {
    fprintf(stderr, "Error: String %s Expected.\n", word);
  }
  if (sscanf(word, "%d", &num) < 1) {
    fprintf(stderr, "Error: String %s Must be a Number.\n", word);
  }
  return num;
}


/**Function********************************************************************

  Synopsis           [Inse le parole in cima all'array "datum_str".]

  SideEffects        []

  SeeAlso            [array_insert]

******************************************************************************/

static char **
get_string_array(
  FILE * fp,
  char *word,
  int size                      /* number of expected items */
)
{
  char *str;
  int i;
  char **array;

  array = Pdtutil_Alloc(char *,
    size
  );

  if (array == NULL) {
    fprintf(stderr, "Error: Out of Memory Allocating String Array.\n");
    return (NULL);
  }
  for (i = 0; i < size; i++) {
    if (get_token(fp, word) != KeyFSM_STRING) {
      fprintf(stderr, "Error: String Expected for Array.\n");
    }
    str = Pdtutil_StrDup(word);
    array[i] = str;
  }

  return array;
}


/**Function********************************************************************

  Synopsis           [Inserisce i numeri in cima all'array "datum_nmb".]

  Description        [Trasforma le parole estratte da "get_token" in interi
  per mezzo della procedura "number". Tramite la procedura "Ddi_BddarrayWrite"
  inserisce i numeri in cima all'array "datum_nmb".]

  SideEffects        []

  SeeAlso            [number,Ddi_BddarrayWrite]

******************************************************************************/

static int *
get_integer_array(
  FILE * fp,
  char *word,
  int size                      /* number of expected items */
)
{
  int i;
  int *array;

  array = Pdtutil_Alloc(int,
    size
  );

  if (array == NULL) {
    fprintf(stderr, "Error: Out of Memory Allocating Integer Array.\n");
    return (NULL);
  }

  for (i = 0; i < size; i++) {
    array[i] = get_number(fp, word);
  }

  return (array);
}

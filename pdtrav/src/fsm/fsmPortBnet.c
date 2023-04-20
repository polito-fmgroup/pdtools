/**CFile***********************************************************************

  FileName    [fsmPortBnet.c]

  PackageName [fsm]
  Synopsis    [Nanotrav parts used by pdtrav]

  Description [This package contains functions, types and constants taken from
    nanotrav to be used in pdtrav. The original names have been modified adding
    the "Fsm_Port" prefix.<p>
    FsmPortBnet.c contains the parts taken from bnet.c.]

  SeeAlso     [fsmPortNtr.c pdtrav]

  Author      [Gianpiero Cabodi, Stefano Quer]

  Copyright [  Copyright (c) 2001 by Politecnico di Torino.
    All Rights Reserved. This software is for educational purposes only.
    Permission is given to academic institutions to use, copy, and modify
    this software and its documentation provided that this introductory
    message is not removed, that this software and its documentation is
    used for the institutions' internal research and educational purposes,
    and that no monies are exchanged. No guarantee is expressed or implied
    by the distribution of this code.
    Send bug-reports and/or questions to: {cabodi,quer}@polito.it. ]

******************************************************************************/

#include "fsmInt.h"

#define MAXLENGTH 131072

static char BuffLine[MAXLENGTH];
static char *CurPos;

int doReadMult = 0;

Ddi_Bdd_t *manualAuxTot = NULL;
Ddi_Bddarray_t *manualAux = NULL;
Ddi_Vararray_t *manualAuxVars = NULL;
static int manualAuxChecked = 0;
static int nManualAux = 0;
int maxNodeSize = 0;
int nDropped = 0;
int nDone = 0;

int nAuxSlices = 0;
Ddi_Vararray_t *AuxSliceVars[1000];
Ddi_Bddarray_t *AuxSliceFuns[1000];

int myDbgCnt = 0;

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static char *readString(
  FILE * fp
);
static char **readList(
  FILE * fp,
  int *n
);
static void printList(
  char **list,
  int n
);
static int buildExorBDD(
  Ddi_Mgr_t * dd,
  FsmPortBnetNode_t * nd,
  st_table * hash,
  int params,
  int nodrop,
  int cutThresh,
  int useAig
);
static int buildMuxBDD(
  Ddi_Mgr_t * dd,
  FsmPortBnetNode_t * nd,
  st_table * hash,
  int params,
  int nodrop,
  int cutThresh,
  int useAig
);
static FsmPortBnetNode_t **fsmPortBnetOrderRoots(
  FsmPortBnetNetwork_t * net,
  int *nroots
);
static int fsmPortBnetDfsOrder(
  Ddi_Mgr_t * dd,
  FsmPortBnetNetwork_t * net,
  FsmPortBnetNode_t * node,
  int useAig
);
static int fsmPortBnetLevelCompare(
  FsmPortBnetNode_t ** x,
  FsmPortBnetNode_t ** y
);
static int readMultAux(
  Ddi_Mgr_t * ddm
);
static int printCombLoop(
  FILE * fp,
  st_table * hash,
  FsmPortBnetNode_t * nd,
  FsmPortBnetNode_t * startnd
);
static int setupCombLoop(
  st_table * hash,
  FsmPortBnetNode_t * nd,
  FsmPortBnetNode_t * startnd,
  int phase
);
static int propagateCOI(
  st_table * hash,
  FsmPortBnetNode_t * nd,
  int stopAtLatch
);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exfsmPorted functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Reads boolean network from blif file.]

  Description [Reads a boolean network from a blif file. A very restricted
  subset of blif is supfsmPorted. Specifically:
  <ul>
  <li> The only directives recognized are:
    <ul>
    <li> .model
    <li> .inputs
    <li> .outputs
    <li> .latch
    <li> .names
    <li> .exdc
    <li> .wire_load_slope
    <li> .end
    </ul>
  <li> Latches must have an initial values and no other parameters
       specified.
  <li> Lines must not exceed MAXLENGTH-1 characters, and individual names must
       not exceed 1023 characters.
  </ul>
  Caveat emptor: There may be other limitations as well. One should
  check the syntax of the blif file with some other tool before relying
  on this parser. Fsm_PortBnetReadNetwork returns a pointer to the network if
  successful; NULL otherwise.
  TODO: check memory leaks in case of COI reduction.
  ]

  SideEffects [None]

  SeeAlso     [Fsm_PortBnetPrintNetwork Fsm_PortBnetFreeNetwork]

******************************************************************************/
FsmPortBnetNetwork_t *
Fsm_PortBnetReadNetwork(
  FILE * fp /* pointer to the blif file */ ,
  int pr                        /* verbosity level */
)
{
  char *savestring;
  char **list;
  int i, j, n;
  FsmPortBnetNetwork_t *net;
  FsmPortBnetNode_t *newnode;
  FsmPortBnetNode_t *lastnode = NULL;
  FsmPortBnetTabline_t *newline;
  FsmPortBnetTabline_t *lastline;
  char ***latches = NULL;
  int maxlatches = 0;
  int exdc = 0;
  FsmPortBnetNode_t *node;
  int count;

  /* Allocate network object and initialize symbol table. */
  net = Pdtutil_Alloc(FsmPortBnetNetwork_t, 1);
  if (net == NULL)
    goto failure;
  memset((char *)net, 0, sizeof(FsmPortBnetNetwork_t));
  net->hash = st_init_table(strcmp, st_strhash);
  if (net->hash == NULL)
    goto failure;

  savestring = readString(fp);
  if (savestring == NULL)
    goto failure;
  net->nlatches = 0;
  while (strcmp(savestring, ".model") == 0 ||
    strcmp(savestring, ".inputs") == 0 ||
    strcmp(savestring, ".outputs") == 0 ||
    strcmp(savestring, ".constraint") == 0 ||
    strcmp(savestring, ".latch") == 0 ||
    strcmp(savestring, ".wire_load_slope") == 0 ||
    strcmp(savestring, ".exdc") == 0 ||
    strcmp(savestring, ".names") == 0 || strcmp(savestring, ".end") == 0) {
    if (strcmp(savestring, ".model") == 0) {
      /* Read .model directive. */
      Pdtutil_Free(savestring);
      /* Read network name. */
      savestring = readString(fp);
      if (savestring == NULL)
        goto failure;
      net->name = savestring;
    } else if (strcmp(savestring, ".inputs") == 0) {
      /* Read .inputs directive. */
      Pdtutil_Free(savestring);
      /* Read input names. */
      list = readList(fp, &n);
      if (list == NULL)
        goto failure;
      if (pr > 2)
        printList(list, n);
      /* Expect at least one input. */
      if (n < 1) {
        fprintf(stderr, "Error: Empty .input List.\n");
        goto failure;
      }
      if (exdc) {
        for (i = 0; i < n; i++)
          Pdtutil_Free(list[i]);
        Pdtutil_Free(list);
        savestring = readString(fp);
        if (savestring == NULL)
          goto failure;
        continue;
      }
      if (net->ninputs) {
        net->inputs = Pdtutil_Realloc(char *,
          net->inputs,
           (net->ninputs + n)
        );

        for (i = 0; i < n; i++)
          net->inputs[net->ninputs + i] = list[i];
      } else
        net->inputs = list;
      /* Create a node for each primary input. */
      for (i = 0; i < n; i++) {
        newnode = Pdtutil_Alloc(FsmPortBnetNode_t, 1);
        memset((char *)newnode, 0, sizeof(FsmPortBnetNode_t));
        if (newnode == NULL)
          goto failure;
        newnode->name = list[i];
        newnode->inputs = NULL;
        newnode->type = PORT_BNETINPUT_NODE;
        newnode->active = FALSE;
        newnode->nfo = 0;
        newnode->ninp = 0;
        newnode->f = NULL;
        newnode->polarity = 0;
        newnode->dd = NULL;
        newnode->next = NULL;
        newnode->cutpoint = FALSE;
        newnode->manualCutpoint = FALSE;
        newnode->cutdd = NULL;
        newnode->evaluating = FALSE;
        newnode->combLoop = FALSE;
        newnode->coiFlag = FALSE;
        if (lastnode == NULL) {
          net->nodes = newnode;
        } else {
          lastnode->next = newnode;
        }
        lastnode = newnode;
      }
      net->npis += n;
      net->ninputs += n;
    } else if (strcmp(savestring, ".outputs") == 0 ||
      strcmp(savestring, ".constraint") == 0) {
      /* Read .outputs directive. We do not create nodes for the primary
       ** outputs, because the nodes will be created when the same names
       ** appear as outputs of some gates.
       */
      int isConstr = strcmp(savestring, ".constraint") == 0;

      Pdtutil_Free(savestring);
      /* Read output names. */
      list = readList(fp, &n);
      if (list == NULL)
        goto failure;
      if (pr > 2)
        printList(list, n);
      if (n < 1) {
        fprintf(stderr, "Error: Empty .output List.\n");
        goto failure;
      }
      if (exdc) {
        for (i = 0; i < n; i++)
          Pdtutil_Free(list[i]);
        Pdtutil_Free(list);
        savestring = readString(fp);
        if (savestring == NULL)
          goto failure;
        continue;
      }
      if (net->noutputs) {
        net->outputs = Pdtutil_Realloc(char *,
          net->outputs,
           (net->noutputs + n)
        );
        net->outputIsConstraint = Pdtutil_Realloc(int,
          net->outputIsConstraint,
           (net->noutputs + n)
        );

        for (i = 0; i < n; i++)
          net->outputs[net->noutputs + i] = list[i];
        for (i = 0; i < n; i++)
          net->outputIsConstraint[net->noutputs + i] = isConstr;
      } else {
        net->outputs = list;
        net->outputIsConstraint = Pdtutil_Alloc(int,
          n
        );

        for (i = 0; i < n; i++)
          net->outputIsConstraint[i] = isConstr;
      }
      net->npos += n;
      net->noutputs += n;
    } else if (strcmp(savestring, ".wire_load_slope") == 0) {
      Pdtutil_Free(savestring);
      savestring = readString(fp);
      net->slope = savestring;
    } else if (strcmp(savestring, ".latch") == 0) {
      Pdtutil_Free(savestring);
      newnode = Pdtutil_Alloc(FsmPortBnetNode_t, 1);
      if (newnode == NULL)
        goto failure;
      memset((char *)newnode, 0, sizeof(FsmPortBnetNode_t));
      newnode->type = PORT_BNETPRESENT_STATE_NODE;
      list = readList(fp, &n);
      if (list == NULL)
        goto failure;
      if (pr > 2)
        printList(list, n);
      /* Expect three names. */
      if (n != 3) {
        (void)fprintf(stdout, ".latch not followed by three tokens.\n");
        goto failure;
      }
      newnode->name = list[1];
      newnode->inputs = list;
      newnode->ninp = 1;
      newnode->f = NULL;
      newnode->active = FALSE;
      newnode->nfo = 0;
      newnode->polarity = 0;
      newnode->dd = NULL;
      newnode->next = NULL;
      newnode->cutpoint = FALSE;
      newnode->manualCutpoint = FALSE;
      newnode->cutdd = NULL;
      newnode->evaluating = FALSE;
      newnode->combLoop = FALSE;
      newnode->coiFlag = FALSE;
      if (lastnode == NULL) {
        net->nodes = newnode;
      } else {
        lastnode->next = newnode;
      }
      lastnode = newnode;
      /* Add next state variable to list. */
      if (maxlatches == 0) {
        maxlatches = 20;
        latches = Pdtutil_Alloc(char **,
          maxlatches
        );
      } else if (maxlatches <= net->nlatches) {
        maxlatches += 20;
        latches = Pdtutil_Realloc(char **,
          latches,
          maxlatches
        );
      }
      latches[net->nlatches] = list;
      net->nlatches++;
      savestring = readString(fp);
      if (savestring == NULL)
        goto failure;
    } else if (strcmp(savestring, ".names") == 0) {
      Pdtutil_Free(savestring);
      newnode = Pdtutil_Alloc(FsmPortBnetNode_t, 1);
      memset((char *)newnode, 0, sizeof(FsmPortBnetNode_t));
      if (newnode == NULL)
        goto failure;
      list = readList(fp, &n);
      if (list == NULL)
        goto failure;
      if (pr > 2)
        printList(list, n);
      /* Expect at least one name (the node output). */
      if (n < 1) {
        fprintf(stderr, "Error: Missing Output Name.\n");
        goto failure;
      }
      newnode->name = list[n - 1];
      newnode->inputs = list;
      newnode->ninp = n - 1;
      newnode->active = FALSE;
      newnode->nfo = 0;
      newnode->polarity = 0;
      newnode->cutpoint = FALSE;
      newnode->manualCutpoint = FALSE;
      newnode->cutdd = NULL;
      newnode->evaluating = FALSE;
      newnode->combLoop = FALSE;
      newnode->coiFlag = FALSE;
      if (newnode->ninp > 0) {
        newnode->type = PORT_BNETINTERNAL_NODE;
        for (i = 0; i < net->noutputs; i++) {
          if (strcmp(net->outputs[i], newnode->name) == 0) {
            newnode->type = PORT_BNETOUTPUT_NODE;
            break;
          }
        }
      } else {
        newnode->type = PORT_BNETCONSTANT_NODE;
      }
      newnode->dd = NULL;
      newnode->next = NULL;
      if (lastnode == NULL) {
        net->nodes = newnode;
      } else {
        lastnode->next = newnode;
      }
      lastnode = newnode;
      /* Read node function. */
      newnode->f = NULL;
      if (exdc) {
        newnode->exdc_flag = 1;
        node = net->nodes;
        while (node) {
          if (node->type == PORT_BNETOUTPUT_NODE &&
            strcmp(node->name, newnode->name) == 0) {
            node->exdc = newnode;
            break;
          }
          node = node->next;
        }
      }
      savestring = readString(fp);
      if (savestring == NULL)
        goto failure;
      lastline = NULL;
      while (savestring[0] != '.') {
        /* Reading a table line. */
        newline = Pdtutil_Alloc(FsmPortBnetTabline_t, 1);
        if (newline == NULL)
          goto failure;
        newline->next = NULL;
        if (lastline == NULL) {
          newnode->f = newline;
        } else {
          lastline->next = newline;
        }
        lastline = newline;
        if (newnode->type == PORT_BNETINTERNAL_NODE ||
          newnode->type == PORT_BNETOUTPUT_NODE) {
          newline->values = savestring;
          /* Read output 1 or 0. */
          savestring = readString(fp);
          if (savestring == NULL)
            goto failure;
        } else {
          newline->values = NULL;
        }
        if (savestring[0] == '0')
          newnode->polarity = 1;
        Pdtutil_Free(savestring);
        savestring = readString(fp);
        if (savestring == NULL)
          goto failure;
      }
    } else if (strcmp(savestring, ".exdc") == 0) {
      Pdtutil_Free(savestring);
      exdc = 1;
    } else if (strcmp(savestring, ".end") == 0) {
      Pdtutil_Free(savestring);
      break;
    }
    if ((!savestring) || savestring[0] != '.')
      savestring = readString(fp);
    if (savestring == NULL)
      goto failure;
  }
  if (latches) {
    net->latches = latches;

    count = 0;
    net->outputs = Pdtutil_Realloc(char *,
      net->outputs,
       (net->noutputs + net->nlatches) * sizeof(char *)
    );

    for (i = 0; i < net->nlatches; i++) {
      for (j = 0; j < net->noutputs; j++) {
        if (strcmp(latches[i][0], net->outputs[j]) == 0)
          break;
      }
      if (j < net->noutputs)
        continue;
      savestring = Pdtutil_Alloc(char,
        strlen(latches[i][0]) + 1
      );

      strcpy(savestring, latches[i][0]);
      net->outputs[net->noutputs + count] = savestring;
      count++;
      node = net->nodes;
      while (node) {
        if (node->type == PORT_BNETINTERNAL_NODE &&
          strcmp(node->name, savestring) == 0) {
          node->type = PORT_BNETOUTPUT_NODE;
          break;
        }
        node = node->next;
      }
    }
    net->noutputs += count;

    net->inputs = Pdtutil_Realloc(char *,
      net->inputs,
       (net->ninputs + net->nlatches) * sizeof(char *)
    );

    for (i = 0; i < net->nlatches; i++) {
      savestring = Pdtutil_Alloc(char,
        strlen(latches[i][1]) + 1
      );

      strcpy(savestring, latches[i][1]);
      net->inputs[net->ninputs + i] = savestring;
    }
    net->ninputs += net->nlatches;
  }

  /* Put nodes in symbol table. */
  newnode = net->nodes;
  while (newnode != NULL) {
    Pdtutil_VerbosityLocal(pr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "Inserting %s.\n", newnode->name)
      );
    if (st_insert(net->hash, newnode->name, (char *)newnode) == ST_OUT_OF_MEM) {
      goto failure;
    }
    newnode = newnode->next;
  }

  /* Compute fanout counts. For each node in the linked list, fetch
   ** all its fanins using the symbol table, and increment the fanout of
   ** each fanin.
   */
  newnode = net->nodes;
  while (newnode != NULL) {
    FsmPortBnetNode_t *auxnd;

    for (i = 0; i < newnode->ninp; i++) {
      if (!st_lookup(net->hash, newnode->inputs[i], (char **)&auxnd)) {
        fprintf(stderr, "Node %s Not Driven.\n", newnode->inputs[i]);
        goto failure;
      }
      auxnd->nfo++;
    }
    newnode = newnode->next;
  }

  if (!fsmPortBnetSetLevel(net))
    goto failure;

  return (net);

failure:
  /* Here we should clean up the mess. */
  fprintf(stdout, "Error: Reading Network from File.\n");
  return (NULL);

}                               /* end of Fsm_PortBnetReadNetwork */

/**Function********************************************************************

  Synopsis    [Builds the BDD for the function of a node.]

  Description [Builds the BDD for the function of a node and stores a
  pointer to it in the dd field of the node itself. The reference count
  of the BDD is incremented. If params is PORT_BNETLOCAL_DD, then the BDD is
  built in terms of the local inputs to the node; otherwise, if params
  is PORT_BNETGLOBAL_DD, the BDD is built in terms of the network primary
  inputs. To build the global BDD of a node, the BDDs for its local
  inputs must exist. If that is not the case, Fsm_PortBnetBuildNodeBDD
  recursively builds them. Likewise, to create the local BDD for a node,
  the local inputs must have variables assigned to them. If that is not
  the case, Fsm_PortBnetBuildNodeBDD recursively assigns variables to nodes.
  Auxiliary variable for cutpoint is generated with PORT_BNETGLOBAL_DD, if
  BDD of node is larger than cutThresh (-1 means no threshold).
  Fsm_PortBnetBuildNodeBDD returns 1 in case of success; 0 otherwise.]

  SideEffects [Sets the dd field of the node.]

  SeeAlso     []

******************************************************************************/
int
Fsm_PortBnetBuildNodeBDD(
  Ddi_Mgr_t * dd /* DD manager */ ,
  FsmPortBnetNode_t * nd /* node of the boolean network */ ,
  st_table * hash /* symbol table of the boolean network */ ,
  int params /* type of DD to be built */ ,
  int nodrop /* retain the intermediate node DDs until the end */ ,
  int cutThresh /* threshold for cutpoint generation */ ,
  int useAig
)
{
  FsmPortBnetNode_t *auxnd;
  Ddi_Bdd_t *prod, *var;
  FsmPortBnetTabline_t *line;
  char buf[1000];
  int i;
  static int auxVarNumber = 0;

  if (nd->dd != NULL)
    return (1);

  nd->evaluating = TRUE;

  if (nd->type == PORT_BNETCONSTANT_NODE) {
    if (useAig) {
      if (nd->f == NULL) {      /* constant 0 */
        nd->dd = Ddi_BddMakeConstAig(dd, 0);
      } else {                  /* either constant depending on the polarity */
        nd->dd = Ddi_BddMakeConstAig(dd, 1);
      }
    } else {
      if (nd->f == NULL) {      /* constant 0 */
        nd->dd = Ddi_BddMakeConst(dd, 0);
      } else {                  /* either constant depending on the polarity */
        nd->dd = Ddi_BddMakeConst(dd, 1);
      }
    }
  } else if (nd->type == PORT_BNETINPUT_NODE ||
    nd->type == PORT_BNETPRESENT_STATE_NODE) {
    if (nd->active != TRUE) {
      /* a variable is NOT already associated: use it */
      nd->var = Ddi_VarNew(dd);
      nd->active = TRUE;
      Ddi_VarAttachName(nd->var, nd->name);
    }
    if (useAig) {
      nd->dd = Ddi_BddMakeLiteralAig(nd->var, 1);
    } else {
      nd->dd = Ddi_BddMakeLiteral(nd->var, 1);
    }
  } else if (buildExorBDD(dd, nd, hash, params, nodrop, cutThresh, useAig)) {
    /* Do nothing */
  } else if (buildMuxBDD(dd, nd, hash, params, nodrop, cutThresh, useAig)) {
    /* Do nothing */
  } else {                      /* type == PORT_BNETINTERNAL_NODE or PORT_BNETOUTPUT_NODE */
    /* Initialize the sum to logical 0. */
    if (useAig) {
      nd->dd = Ddi_BddMakeConstAig(dd, 0);
    } else {
      nd->dd = Ddi_BddMakeConst(dd, 0);
    }
    /* Build a term for each line of the table and add it to the
     ** accumulator (func).
     */
    line = nd->f;
    while (line != NULL) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelDebug_c, fprintf(stdout, "line = %s.\n", line->values)
        );
      /* Initialize the product to logical 1. */
      if (useAig) {
        prod = Ddi_BddMakeConstAig(dd, 1);
      } else {
        prod = Ddi_BddMakeConst(dd, 1);
      }
      /* Scan the table line. */
      for (i = 0; i < nd->ninp; i++) {
        if (line->values[i] == '-')
          continue;
        if (!st_lookup(hash, (char *)nd->inputs[i], (char **)&auxnd)) {
          goto failure;
        }
        if (auxnd->evaluating == TRUE) {
          fprintf(stderr, "Combinational Loop Found on Node %s.\n",
            auxnd->name);
          /* Temporarily break the loop by auxiliary variable */
          fprintf(stderr,
            "Loop Cutting; Inserting Auxiliary Variable on Node: %s.\n",
            auxnd->name);
          auxnd->var = Ddi_VarNew(dd);
          auxnd->cutpoint = TRUE;
          sprintf(buf, "LOOP$AUXVAR$%s", auxnd->name);
          Ddi_VarAttachName(auxnd->var, buf);
          auxnd->cutdd = NULL;
          if (useAig) {
            auxnd->dd = Ddi_BddMakeLiteralAig(auxnd->var, 1);
          } else {
            auxnd->dd = Ddi_BddMakeLiteral(auxnd->var, 1);
          }
          setupCombLoop(hash, auxnd, auxnd, 0);
#if 0
          /* this is temporarily disabled */
          printCombLoop(stderr, hash, auxnd, auxnd);
          goto failure;
#endif
        }
        if (params == PORT_BNETLOCAL_DD) {
          if (auxnd->active == FALSE) {
            if (!Fsm_PortBnetBuildNodeBDD(dd,
                auxnd, hash, params, nodrop, -1, useAig)) {
              goto failure;
            }
          }
          Pdtutil_Assert(useAig == FALSE, "Wrong setting for AIG usage");
          var = Ddi_BddMakeLiteral(auxnd->var, 1);
          if (line->values[i] == '0') {
            Ddi_BddNotAcc(var);
          }
        } else {                /* params == PORT_BNETGLOBAL_DD */
          if (auxnd->dd == NULL) {
            if (!Fsm_PortBnetBuildNodeBDD(dd,
                auxnd, hash, params, nodrop, cutThresh, useAig)) {
              goto failure;
            }
          }
          if (line->values[i] == '1') {
            var = Ddi_BddDup(auxnd->dd);
          } else {              /* line->values[i] == '0' */
            var = Ddi_BddNot(auxnd->dd);
          }
        }
        Ddi_BddAndAcc(prod, var);
        Ddi_Free(var);
      }
      Ddi_BddOrAcc(nd->dd, prod);
      Ddi_Free(prod);
      line = line->next;
    }
    /* Associate a variable to this node if local BDDs are being
     ** built. This is done at the end, so that the primary inputs tend
     ** to get lower indices.
     */
    if (params == PORT_BNETLOCAL_DD && nd->active == FALSE) {
      nd->var = Ddi_VarNew(dd);
      nd->active = TRUE;
      Ddi_VarAttachName(nd->var, nd->name);
    }
  }
  if (nd->polarity == 1) {
    Ddi_BddNotAcc(nd->dd);
  }

  if (doReadMult) {
    if (!manualAuxChecked) {
      manualAuxChecked = 1;
      readMultAux(dd);
    }
    if (manualAuxTot != NULL) {
      if (Ddi_BddSize(nd->dd) > 2 && Ddi_BddSize(nd->dd) < 4) {
        int k;

#if 0
        Ddi_Varset_t *suppNd = Ddi_BddSupp(nd->dd);
        Ddi_Bdd_t *myAux = Ddi_BddMakeConst(dd, 1);

        for (k = 0; k < Ddi_BddPartNum(manualAuxTot); k++) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(manualAuxTot, k);
          Ddi_Varset_t *supp = Ddi_BddSupp(Ddi_BddarrayRead(manualAux, k));
          int nv = Ddi_VarsetNum(supp);
          Ddi_Varset_t *suppCovered = Ddi_VarsetVoid(dd);

          Ddi_VarsetIntersectAcc(supp, suppNd);
          if (Ddi_VarsetNum(supp) == nv) {
#if 0
            Ddi_Bdd_t *tmp = Ddi_BddConstrain(nd->dd, p);

            if (Ddi_BddSize(tmp) < Ddi_BddSize(nd->dd)) {
              Ddi_Free(nd->dd);
              nd->dd = tmp;
              Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
                Pdtutil_VerbLevelNone_c,
                fprintf(stdout, "Manual Auxiliary Variable %d on Node %s.\n",
                  ++nManualAux, nd->name)
                );
              Ddi_Free(suppNd);
              suppNd = Ddi_BddSupp(nd->dd);
              nd->manualCutpoint = TRUE;
            } else {
              Ddi_Free(tmp);
            }
#else
            Ddi_BddAndAcc(myAux, p);
            Ddi_VarsetUnionAcc(suppCovered, supp);
            if (Ddi_VarsetEqual(suppCovered, suppNd)) {
              /*break; */
            }
#endif
          }
          Ddi_Free(suppCovered);
          Ddi_Free(supp);
        }
        Ddi_Free(suppNd);
        if (!Ddi_BddIsOne(myAux)) {
          Ddi_Bdd_t *tmp = Ddi_BddConstrain(nd->dd, myAux);

          if (Ddi_BddSize(tmp) < Ddi_BddSize(nd->dd)) {
            Ddi_Free(nd->dd);
            nd->dd = tmp;
            Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
              Pdtutil_VerbLevelNone_c,
              fprintf(stdout, "Manual Auxiliary Variable %d on Node %s.\n",
                ++nManualAux, nd->name)
              );
            nd->manualCutpoint = TRUE;
          } else {
            Ddi_Free(tmp);
          }
        }
        Ddi_Free(myAux);
#else
#if 1
        for (k = 0; k < Ddi_BddPartNum(manualAuxTot); k++) {
          Ddi_Bdd_t *p = Ddi_BddPartRead(manualAuxTot, k);
          Ddi_Bdd_t *tmp = Ddi_BddConstrain(nd->dd, p);

          if (Ddi_BddSize(tmp) < Ddi_BddSize(nd->dd)) {
            Ddi_Free(nd->dd);
            nd->dd = tmp;
            Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
              Pdtutil_VerbLevelNone_c,
              fprintf(stdout, "Manual Auxiliary Variable %d on Node %s.\n",
                ++nManualAux, nd->name)
              );
            nd->manualCutpoint = TRUE;
          } else {
            Ddi_Free(tmp);
          }
        }
#else
        for (k = 0; k < Ddi_BddarrayNum(manualAux); k++) {
          Ddi_Bdd_t *p = Ddi_BddarrayRead(manualAux, k);

          Pdtutil_Assert(useAig == FALSE, "Wrong setting for AIG usage");
          if (Ddi_BddEqual(p, nd->dd)) {
            Ddi_Free(nd->dd);
            nd->dd = Ddi_BddMakeLiteral(Ddi_VararrayRead(manualAuxVars, k), 1);
            Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
              Pdtutil_VerbLevelNone_c,
              fprintf(stdout, "Manual Auxiliary Variable %d on Node %s.\n",
                ++nManualAux, nd->name)
              );
            nd->manualCutpoint = TRUE;
            break;
          } else {
            Ddi_BddNotAcc(nd->dd);
            if (Ddi_BddEqual(p, nd->dd)) {
              Ddi_Free(nd->dd);
              nd->dd =
                Ddi_BddMakeLiteral(Ddi_VararrayRead(manualAuxVars, k), 0);
              Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
                Pdtutil_VerbLevelNone_c,
                fprintf(stdout, "Manual Auxiliary Variable %d on Node %s.\n",
                  ++nManualAux, nd->name)
                );
              nd->manualCutpoint = TRUE;
              break;
            } else {
              Ddi_BddNotAcc(nd->dd);
            }
          }
        }
#endif
#endif
      }
    }
  }

  if ((nd->cutpoint == TRUE) && (nd->cutdd == NULL)) {
    Pdtutil_Assert(nd->combLoop == TRUE, "Wrong setting of comb loop cut");
    nd->cutdd = Ddi_BddDup(nd->dd);
    setupCombLoop(hash, nd, nd, 1);
    Ddi_Free(nd->cutdd);
    nd->var = NULL;
    nd->cutpoint = FALSE;
  }

  if (params == PORT_BNETGLOBAL_DD && (cutThresh >= 0)) {
    if ((nd->cutpoint == FALSE) && (Ddi_BddSize(nd->dd) > cutThresh)) {
      char name[PDTUTIL_MAX_STR_LEN];

      Pdtutil_Assert(useAig == FALSE, "Wrong setting for AIG usage");
      sprintf(name, "AUXVAR$%d$%s", auxVarNumber, nd->name);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout,
          "Automatic Auxiliart Variable %d on Node %s (Size: %d).\n",
          auxVarNumber, nd->name, Ddi_BddSize(nd->dd))
        );
      auxVarNumber++;
      nd->var = Ddi_VarNew(dd);
      Ddi_VarAttachName(nd->var, name);
      nd->cutpoint = TRUE;
      nd->cutdd = nd->dd;
      nd->dd = Ddi_BddMakeLiteral(nd->var, 1);

    }
  }

  if (Ddi_BddSize(nd->dd) > maxNodeSize) {
    maxNodeSize = Ddi_BddSize(nd->dd);
  }
#if 0
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, if (Ddi_BddSize(nd->dd) > 10000) {
    fprintf(stdout, "node: %s - sixe: %d - fo: %d.\n",
        nd->name, Ddi_BddSize(nd->dd), nd->count);}
  ) ;
#endif
  nDone++;
  if (params == PORT_BNETGLOBAL_DD && nodrop == FALSE) {
    /* Decrease counters for all faninis.
     ** When count reaches 0, the DD is freed.
     */
    for (i = 0; i < nd->ninp; i++) {
      if (!st_lookup(hash, (char *)nd->inputs[i], (char **)&auxnd)) {
        goto failure;
      }
      auxnd->count--;
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, if (0 && (Ddi_BddSize(nd->dd) > 100000)) {
        fprintf(stdout, " input node: %s - size: %d - fo: %d.\n",
            auxnd->name, Ddi_BddSize(auxnd->dd), auxnd->count);}
      ) ;
      if (auxnd->count == 0 || (0 && (Ddi_BddSize(auxnd->dd) > 10000))) {

        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c, if (auxnd->count > 0) {
          fprintf(stdout, "Dropping node %s (Size = %d - fo: %d).\n",
              auxnd->name, Ddi_BddSize(auxnd->dd), auxnd->count);}
        ) ;
        /* don't remove cut-point nodes */
        if (!auxnd->cutpoint) {
          Ddi_Free(auxnd->dd);
          nDropped++;
        }
      }
    }
  }

  nd->evaluating = FALSE;

  return (1);

failure:
  /* Here we should clean up the mess. */
  return (0);

}                               /* end of Fsm_PortBnetBuildNodeBDD */


/**Function********************************************************************

  Synopsis    [Orders the BDD variables by DFS.]

  Description [Orders the BDD variables by DFS.  Returns 1 in case of
  success; 0 otherwise.]

  SideEffects [Uses the visited flags of the nodes.]

  SeeAlso     []

******************************************************************************/
int
Fsm_PortBnetDfsVariableOrder(
  Ddi_Mgr_t * dd,
  FsmPortBnetNetwork_t * net,
  int useAig
)
{
  FsmPortBnetNode_t **roots;
  FsmPortBnetNode_t *node;
  int nroots;
  int i;
  st_generator *gen = st_init_gen(net->hash);
  char *key;
  FsmPortBnetNode_t *value;

  roots = fsmPortBnetOrderRoots(net, &nroots);
  if (roots == NULL)
    return (0);
  for (i = 0; i < nroots; i++) {
    if (!fsmPortBnetDfsOrder(dd, net, roots[i], useAig)) {
      Pdtutil_Free(roots);
      return (0);
    }
  }
  /* Clear visited flags. */
  node = net->nodes;
  while (node != NULL) {
    node->visited = 0;
    node = node->next;
  }
  st_free_gen(gen);
  Pdtutil_Free(roots);
  return (1);

}                               /* end of Fsm_PortBnetDfsVariableOrder */

/**Function********************************************************************

  Synopsis    [Reads the variable order from a file.]

  Description [Reads the variable order from a file.
    Returns 1 if successful; 0 otherwise.]

  SideEffects [The BDDs for the primary inputs and present state variables
    are built.
    ]

  SeeAlso     []

*****************************************************************************/

int
Fsm_PortBnetReadOrder(
  Ddi_Mgr_t * dd,
  char *ordFile,
  FsmPortBnetNetwork_t * net,
  int locGlob,
  int nodrop,
  Pdtutil_VariableOrderFormat_e ordFileFormat,
  int useAig
)
{
  st_table *dict;
  int i, result, *varauxids, nvars;
  FsmPortBnetNode_t *node;
  char **varnames;

  dict = st_init_table(strcmp, st_strhash);
  if (dict == NULL) {
    return (0);
  }

  /*
   *  StQ 01.04.1999
   *  Patch to deal with nanotrav+vis+pdtrav variable order files
   *  Call Pdtutil_OrdRead to read the file
   */

  nvars = Pdtutil_OrdRead(&varnames, &varauxids, ordFile, NULL, ordFileFormat);

  if (nvars < 0) {
    Pdtutil_Warning(1, "Error reading variable ordering.");
    st_free_table(dict);
    return (0);
  }

  for (i = 0; i < nvars; i++) {
    if (strstr(varnames[i], "$NS") != NULL) {
      continue;
    }

    if (strlen(varnames[i]) > MAXLENGTH) {
      st_free_table(dict);
      return (0);
    }

    /* There should be a node named "name" in the network. */
    if (!st_lookup(net->hash, varnames[i], (char **)&node)) {
      fprintf(stderr, "Unknown Name in Order File (%s).\n", varnames[i]);
      st_free_table(dict);
      return (0);
    }

    /* A name should not appear more than once in the order. */
    if (st_is_member(dict, varnames[i])) {
      fprintf(stderr, "Duplicate Name in Order File (%s).\n", varnames[i]);
      st_free_table(dict);
      return (0);
    }

    /* The name should correspond to a primary input or present state. */
    if (node->type != PORT_BNETINPUT_NODE &&
      node->type != PORT_BNETPRESENT_STATE_NODE) {
      fprintf(stderr, "Node %s has the Wrong Type (%d).\n", varnames[i],
        node->type);
      st_free_table(dict);
      return (0);
    }

    /* Insert in table. Use node->name rather than name, because the
     ** latter gets overwritten.
     */
    if (st_insert(dict, node->name, NULL) == ST_OUT_OF_MEM) {
      fprintf(stderr, "Out of Memory in Fsm_PortBnetReadOrder.\n");
      st_free_table(dict);
      return (0);
    }

    result = Fsm_PortBnetBuildNodeBDD(dd,
      node, net->hash, locGlob, nodrop, -1, useAig);
    if (result == 0) {
      fprintf(stderr, "Construction of BDD Failed.\n");
      st_free_table(dict);
      return (0);
    }
  }

  /* The number of names in the order file should match exactly the
   ** number of primary inputs and present states.
   */
  if (st_count(dict) != net->ninputs) {
    fprintf(stderr, "Order Incomplete: %d Names instead of %d.\n",
      st_count(dict), net->ninputs);
    st_free_table(dict);
    return (0);
  }

  st_free_table(dict);
  return (1);

}                               /* end of Fsm_PortBnetReadOrder */

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************
  Synopsis    [Allocate and initialize a new DDI block]
  Description [Allocation and initialization of generic DDI block.
               owner DDI manager always required.]
  SideEffects [none]
  SeeAlso     [DdiGenericNew DdiGenericFree]
******************************************************************************/


void
FsmPortBnetGenSliceVars(
  Ddi_Mgr_t * dd /* DD manager */ ,
  FsmPortBnetNetwork_t * net    /* network for which DDs are to be built */
)
{
  FsmPortBnetNode_t *node;
  static int nSliceVars = 0;
  Ddi_Bddarray_t *SliceFuns = Ddi_BddarrayAlloc(dd, 0);
  Ddi_Vararray_t *SliceVars = Ddi_VararrayAlloc(dd, 0);


  AuxSliceVars[nAuxSlices] = SliceVars;
  AuxSliceFuns[nAuxSlices] = SliceFuns;
  nAuxSlices++;

  for (node = net->nodes; node != NULL; node = node->next) {
    if (node->dd != NULL && !node->cutpoint && node->count != -1 &&
      (strcmp(node->name, "VSS") != 0) &&
      !node->manualCutpoint &&
      (node->count > 0) &&
      (Ddi_BddSize(node->dd) > 2) &&
      (node->type != PORT_BNETINPUT_NODE) &&
      (node->type != PORT_BNETOUTPUT_NODE)) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout,
          "Automatic Slice Variable %d on Node %s (Size = %d).\n",
          nSliceVars++, node->name, Ddi_BddSize(node->dd))
        );
      if (node->var != NULL)
        node->var = Ddi_VarNewBeforeVar(node->var);
      else
        node->var = Ddi_VarNew(dd);
      node->cutpoint = TRUE;
      node->cutdd = node->dd;
      node->dd = Ddi_BddMakeLiteral(node->var, 1);
      Ddi_VararrayInsertLast(SliceVars, node->var);
      Ddi_BddarrayInsertLast(SliceFuns, node->cutdd);
    }
  }
}

/**Function********************************************************************

  Synopsis    [Sets the level of each node.]

  Description [Sets the level of each node. Returns 1 if successful; 0
  otherwise.]

  SideEffects [Changes the level and visited fields of the nodes it
  visits.]

  SeeAlso     [fsmPortBnetLevelDFS]

******************************************************************************/
int
fsmPortBnetSetLevel(
  FsmPortBnetNetwork_t * net
)
{
  FsmPortBnetNode_t *node;

  /* Recursively visit nodes. This is pretty inefficient, because we
   ** visit all nodes in this loop, and most of them in the recursive
   ** calls to fsmPortBnetLevelDFS. However, this approach guarantees that
   ** all nodes will be reached ven if there are dangling outputs. */
  node = net->nodes;
  while (node != NULL) {
    if (!fsmPortBnetLevelDFS(net, node))
      return (0);
    node = node->next;
  }

  /* Clear visited flags. */
  node = net->nodes;
  while (node != NULL) {
    node->visited = 0;
    node = node->next;
  }
  return (1);

}                               /* end of fsmPortBnetSetLevel */

/**Function********************************************************************

  Synopsis    [Does a DFS from a node setting the level field.]

  Description [Does a DFS from a node setting the level field. Returns
  1 if successful; 0 otherwise.]

  SideEffects [Changes the level and visited fields of the nodes it
  visits.]

  SeeAlso     [fsmPortBnetSetLevel]

******************************************************************************/
int
fsmPortBnetLevelDFS(
  FsmPortBnetNetwork_t * net,
  FsmPortBnetNode_t * node
)
{
  int i;
  FsmPortBnetNode_t *auxnd;

  if (node->visited == 1) {
    return (1);
  }

  node->visited = 1;

  /* Graphical sources have level 0.  This is the final value if the
   ** node has no fan-ins. Otherwise the successive loop will
   ** increase the level. */
  node->level = 0;
  for (i = 0; i < node->ninp; i++) {
    if (!st_lookup(net->hash, (char *)node->inputs[i], (char **)&auxnd)) {
      return (0);
    }
    if (!fsmPortBnetLevelDFS(net, auxnd)) {
      return (0);
    }
    if (auxnd->level >= node->level)
      node->level = 1 + auxnd->level;
  }
  return (1);

}                               /* end of fsmPortBnetLevelDFS */


/**Function********************************************************************

  Synopsis    [Frees a boolean network created by Fsm_PortBnetReadNetwork.]

  Description []

  SideEffects [None]

  SeeAlso     [Fsm_PortBnetReadNetwork]

******************************************************************************/
void
Fsm_PortBnetFreeNetwork(
  FsmPortBnetNetwork_t * net
)
{
  FsmPortBnetNode_t *node, *nextnode;
  FsmPortBnetTabline_t *line, *nextline;
  int i;

  Pdtutil_Free(net->name);
  /* The input name strings are already pointed by the input nodes.
   ** Here we only need to free the latch names and the array that
   ** points to them.
   */
#if 1
  for (i = 0; i < net->nlatches; i++) {
    Pdtutil_Free(net->inputs[net->npis + i]);
  }
#endif
  Pdtutil_Free(net->inputs);
  /* Free the output name strings and then the array pointing to them.  */
#if 1
  for (i = 0; i < net->noutputs; i++) {
    Pdtutil_Free(net->outputs[i]);
  }
#endif
  Pdtutil_Free(net->outputs);

  for (i = 0; i < net->nlatches; i++) {
    Pdtutil_Free(net->latches[i][0]);
    Pdtutil_Free(net->latches[i][1]);
    Pdtutil_Free(net->latches[i][2]);
    Pdtutil_Free(net->latches[i]);
  }
  if (net->nlatches)
    Pdtutil_Free(net->latches);
  node = net->nodes;
  while (node != NULL) {
    nextnode = node->next;
    if (node->type != PORT_BNETPRESENT_STATE_NODE) {
      Pdtutil_Free(node->name);
      for (i = 0; i < node->ninp; i++) {
        Pdtutil_Free(node->inputs[i]);
      }
      if (node->inputs != NULL) {
        Pdtutil_Free(node->inputs);
      }
    }
    /* Free the function table. */
    line = node->f;
    while (line != NULL) {
      nextline = line->next;
      Pdtutil_Free(line->values);
      Pdtutil_Free(line);
      line = nextline;
    }
    Pdtutil_Free(node);
    node = nextnode;
  }
  st_free_table(net->hash);
  if (net->slope != NULL)
    Pdtutil_Free(net->slope);
  Pdtutil_Free(net);

}                               /* end of Fsm_PortBnetFreeNetwork */


/**Function********************************************************************

  Synopsis    [Structural COI reduction]

  Description [Structural COI reduction. Transitive fanin of marked nodes
               (coiFlag field) is first generated. Input, output and latch
               arrays are then collapsed. TODO: check free routines for
               memory leaks]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int
FsmCoiReduction(
  FsmPortBnetNetwork_t * net,
  char **coiList
)
{
  FsmPortBnetNode_t *nd;
  int i, n, npis0, npos0;
  int l, j, j0, j1, coiLimit, constrCoiLimit = 1;
  char *name;

  for (l = 0; coiList[l] != NULL; l++) {
    switch (coiList[l][0]) {
      case 'o':
        sscanf(coiList[l], "%*c%d%d%d%d", &j0, &j1, &coiLimit,
          &constrCoiLimit);
        if (j0 < 0)
          j0 = 0;
        if (j1 < 0)
          j1 = net->npos - 1;
        for (j = j0; j <= j1; j++) {
          if (st_lookup(net->hash, net->outputs[j], (char **)&nd)) {
            nd->coiFlag = TRUE;
            propagateCOI(net->hash, nd, coiLimit);
          }
        }
        break;
      case 'l':
        sscanf(coiList[l], "%*c%d%d&d&d", &j0, &j1, &coiLimit,
          &constrCoiLimit);
        for (j = j0; j <= j1; j++) {
          if (st_lookup(net->hash, net->latches[j][1], (char **)&nd)) {
            nd->coiFlag = TRUE;
            propagateCOI(net->hash, nd, coiLimit);
          }
        }
        break;
      case 'i':
        name = Pdtutil_Alloc(char,
          strlen(coiList[l]) + 1
        );

        sscanf(coiList[l], "%*c%s%d%d", name, &coiLimit, &constrCoiLimit);
        if (st_lookup(net->hash, name, (char **)&nd)) {
          nd->coiFlag = TRUE;
          /* stop at latch */
          propagateCOI(net->hash, nd, 1);
        }
        /* @@@@ Add as output if internal node */
#if 1
        Pdtutil_Assert(net->noutputs, "network has no outputs: not supported");
        {
          int i;

          for (i = 0; i < net->noutputs; i++) {
            if (strcmp(net->outputs[i], name) == 0) {
              break;
            }
          }
          if (i == net->noutputs) {
            net->outputs = Pdtutil_Realloc(char *,
              net->outputs,
               (net->noutputs + 1) * sizeof(char *)
            );

            net->outputs[i] = name;
            net->npos += 1;
            net->noutputs += 1;
          } else {
            Pdtutil_Free(name);
          }
        }
#endif
        break;
      case 'n':
        name = Pdtutil_Alloc(char,
          strlen(coiList[l]) + 1
        );

        sscanf(coiList[l], "%*c%s%d%d", name, &coiLimit, &constrCoiLimit);
        if (st_lookup(net->hash, name, (char **)&nd)) {
          nd->coiFlag = TRUE;
          propagateCOI(net->hash, nd, coiLimit);
        }
        /* @@@@ Add as output if internal node */
#if 1
        Pdtutil_Assert(net->noutputs, "network has no outputs: not supported");
        {
          int i;

          for (i = 0; i < net->noutputs; i++) {
            if (strcmp(net->outputs[i], name) == 0) {
              break;
            }
          }
          if (i == net->noutputs) {
            net->outputs = Pdtutil_Realloc(char *,
              net->outputs,
               (net->noutputs + 1) * sizeof(char *)
            );

            net->outputs[i] = name;
            net->npos += 1;
            net->noutputs += 1;
          } else {
            Pdtutil_Free(name);
          }
        }
#endif
        break;
      default:
        Pdtutil_Warning(1, "Wrong COI list item");
    }
  }

  if (!st_lookup(net->hash, net->outputs[0], (char **)&nd)) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Missing Output for COI Reduction.\n")
      );
    return (0);
  }

  for (i = 0; constrCoiLimit != 0 && i < net->npos; i++) {
    if (!net->outputIsConstraint[i]) {
      continue;
    }
    if (!st_lookup(net->hash, net->outputs[i], (char **)&nd)) {
      continue;
    }
    nd->coiFlag = TRUE;
    propagateCOI(net->hash, nd, constrCoiLimit);
  }

  for (i = 0, n = 0; i < net->npos; i++) {
    if (!st_lookup(net->hash, net->outputs[i], (char **)&nd)) {
      continue;
    }
    if (nd->coiFlag == TRUE) {
      net->outputIsConstraint[n] = net->outputIsConstraint[i];
      net->outputs[n++] = net->outputs[i];
    }
  }
  npos0 = net->npos;
  net->npos = n;

  for (i = 0, n = 0; i < net->npis; i++) {
    if (strcmp(net->name, "v13890") == 0) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "OK.\n")
        );
    }
    if (!st_lookup(net->hash, net->inputs[i], (char **)&nd)) {
      continue;
    }
    if (nd->coiFlag == TRUE) {
      net->inputs[n++] = net->inputs[i];
    }
  }
  npis0 = net->npis;
  net->npis = n;

  for (i = 0, n = 0; i < net->nlatches; i++) {
    if (0 && strcmp(net->name, "v13890") == 0) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "OK.\n")
        );
    }
    if (!st_lookup(net->hash, net->latches[i][0], (char **)&nd)) {
      continue;
    }
    if (nd->coiFlag == TRUE) {
      net->outputs[net->npos + n] = net->outputs[npos0 + i];
      net->inputs[net->npis + n] = net->inputs[npis0 + i];
      net->latches[n++] = net->latches[i];
    } else if (nd->coiFlag == 2) {
      net->outputs[net->npos + n] = net->outputs[npos0 + i];
      net->inputs[net->npis + n] = net->inputs[npis0 + i];
      net->latches[n++] = net->latches[i];
    }
  }
  net->noutputs = net->npos + n;
  net->ninputs = net->npis + n;
  net->nlatches = n;

  return (1);
}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Reads a string from a file.]

  Description [Reads a string from a file. The string can be MAXLENGTH-1
  characters at most. readString allocates memory to hold the string and
  returns a pointer to the result if successful. It returns NULL
  otherwise.]

  SideEffects [None]

  SeeAlso     [readList]

******************************************************************************/
static char *
readString(
  FILE * fp                     /* pointer to the file from which the string is read */
)
{
  char *savestring;
  int length;

  while (!CurPos) {
    if (!fgets(BuffLine, MAXLENGTH, fp))
      return (NULL);
    BuffLine[strlen(BuffLine) - 1] = '\0';
    CurPos = strtok(BuffLine, " \t");
    if (CurPos && CurPos[0] == '#')
      CurPos = (char *)NULL;
  }
  length = strlen(CurPos);
  savestring = Pdtutil_Alloc(char,
    length + 1
  );

  if (savestring == NULL)
    return (NULL);
  strcpy(savestring, CurPos);
  CurPos = strtok(NULL, " \t");
  return (savestring);

}                               /* end of readString */


/**Function********************************************************************

  Synopsis    [Reads a list of strings from a file.]

  Description [Reads a list of strings from a line of a file.
  The strings are sequences of characters separated by spaces or tabs.
  The total length of the list, white space included, must not exceed
  MAXLENGTH-1 characters.  readList allocates memory for the strings and
  creates an array of pointers to the individual lists. Only two pieces
  of memory are allocated by readList: One to hold all the strings,
  and one to hold the pointers to them. Therefore, when freeing the
  memory allocated by readList, only the pointer to the list of
  pointers, and the pointer to the beginning of the first string should
  be freed. readList returns the pointer to the list of pointers if
  successful; NULL otherwise.]

  SideEffects [n is set to the number of strings in the list.]

  SeeAlso     [readString printList]

******************************************************************************/
static char **
readList(
  FILE * fp /* pointer to the file from which the list is read */ ,
  int *n                        /* on return, number of strings in the list */
)
{
  char *savestring;
  int length;
  char **list;
  int i, count = 0, size = 1;

  list = Pdtutil_Alloc(char *,
    size
  );

  while (CurPos) {
    if (strcmp(CurPos, "\\") == 0) {
#if 1
      /* GpC: patch to avoid including blank lines after \\ */
      if (!fgets(BuffLine, MAXLENGTH, fp))
        return (NULL);
      BuffLine[strlen(BuffLine) - 1] = '\0';
      CurPos = strtok(BuffLine, " \t");
      if (!CurPos) {
        break;
      }
#else
      CurPos = (char *)NULL;
      while (!CurPos) {
        if (!fgets(BuffLine, MAXLENGTH, fp))
          return (NULL);
        BuffLine[strlen(BuffLine) - 1] = '\0';
        CurPos = strtok(BuffLine, " \t");
      }
#endif
    }
    length = strlen(CurPos);
    savestring = Pdtutil_Alloc(char, length + 1);
    if (savestring == NULL)
      return (NULL);
    strcpy(savestring, CurPos);
    if (count == size) {
      size *= 2;
      list = Pdtutil_Realloc(char *,
        list,
        size
      );
    }
    list[count] = savestring;
    count++;
    CurPos = strtok(NULL, " \t");
  }
  *n = count;
  return (list);

}                               /* end of readList */


/**Function********************************************************************

  Synopsis    [Prints a list of strings to the standard output.]

  Description [Prints a list of strings to the standard output. The list
  is in the format created by readList.]

  SideEffects [None]

  SeeAlso     [readList Fsm_PortBnetPrintNetwork]

******************************************************************************/
static void
printList(
  char **list /* list of pointers to strings */ ,
  int n                         /* length of the list */
)
{
  int i;

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, for (i = 0; i < n; i++) {
    fprintf(stdout, "%s", list[i]);}
    fprintf(stdout, "\n")
    ) ;

}                               /* end of printList */



/**Function********************************************************************

  Synopsis    [Builds BDD for a XOR function.]

  Description [Checks whether a function is a XOR with 2 or 3 inputs. If so,
  it builds the BDD. Returns 1 if the BDD has been built; 0 otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static int
buildExorBDD(
  Ddi_Mgr_t * dd,
  FsmPortBnetNode_t * nd,
  st_table * hash,
  int params,
  int nodrop,
  int cutThresh,
  int useAig
)
{
  int check[8];
  int i;
  int nlines;
  FsmPortBnetTabline_t *line;
  Ddi_Bdd_t *var;
  FsmPortBnetNode_t *auxnd;

  if (nd->ninp < 2 || nd->ninp > 3)
    return (0);

  nd->evaluating = TRUE;

  nlines = 1 << (nd->ninp - 1);
  for (i = 0; i < 8; i++)
    check[i] = 0;
  line = nd->f;
  while (line != NULL) {
    int num = 0;
    int count = 0;

    nlines--;
    for (i = 0; i < nd->ninp; i++) {
      num <<= 1;
      if (line->values[i] == '-') {
        return (0);
      } else if (line->values[i] == '1') {
        count++;
        num++;
      }
    }
    if ((count & 1) == 0)
      return (0);
    if (check[num])
      return (0);
    line = line->next;
  }
  if (nlines != 0)
    return (0);

  /* Initialize the exclusive sum to logical 0. */

  if (useAig) {
    nd->dd = Ddi_BddMakeConstAig(dd, 0);
  } else {
    nd->dd = Ddi_BddMakeConst(dd, 0);
  }

  /* Scan the inputs. */
  for (i = 0; i < nd->ninp; i++) {
    if (!st_lookup(hash, (char *)nd->inputs[i], (char **)&auxnd)) {
      goto failure;
    }
    if (auxnd->evaluating == TRUE) {
      fprintf(stderr, "Combinational Loop Found on Node %s.\n", auxnd->name);
      goto failure;
    }
    if (params == PORT_BNETLOCAL_DD) {
      Pdtutil_Assert(useAig == FALSE, "Wrong setting for AIG usage");
      if (auxnd->active == FALSE) {
        if (!Fsm_PortBnetBuildNodeBDD(dd,
            auxnd, hash, params, nodrop, -1, useAig)) {
          goto failure;
        }
      }
      var = Ddi_BddMakeLiteral(auxnd->var, 1);
    } else {                    /* params == PORT_BNETGLOBAL_DD */
      if (auxnd->dd == NULL) {
        if (!Fsm_PortBnetBuildNodeBDD(dd,
            auxnd, hash, params, nodrop, cutThresh, useAig)) {
          goto failure;
        }
      }
      var = auxnd->dd;
    }
    Ddi_BddXorAcc(nd->dd, var);
    if (params == PORT_BNETLOCAL_DD) {
      Ddi_Free(var);
    }
  }

  /* Associate a variable to this node if local BDDs are being
   ** built. This is done at the end, so that the primary inputs tend
   ** to get lower indices.
   */
  if (params == PORT_BNETLOCAL_DD && nd->active == FALSE) {
    nd->var = Ddi_VarNew(dd);
    nd->active = TRUE;
    Ddi_VarAttachName(nd->var, nd->name);
  }

  nd->evaluating = FALSE;

  return (1);
failure:
  return (0);

}                               /* end of buildExorBDD */


/**Function********************************************************************

  Synopsis    [Builds BDD for a multiplexer.]

  Description [Checks whether a function is a 2-to-1 multiplexer. If so,
  it builds the BDD. Returns 1 if the BDD has been built; 0 otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static int
buildMuxBDD(
  Ddi_Mgr_t * dd,
  FsmPortBnetNode_t * nd,
  st_table * hash,
  int params,
  int nodrop,
  int cutThresh,
  int useAig
)
{
  FsmPortBnetTabline_t *line;
  char *values[2];
  int mux[2];
  int phase[2];
  int j;
  int nlines = 0;
  int controlC = -1;
  int controlR = -1;
  Ddi_Bdd_t *f, *g, *h;
  FsmPortBnetNode_t *auxnd;

  if (nd->ninp != 3)
    return (0);

  nd->evaluating = TRUE;

  for (line = nd->f; line != NULL; line = line->next) {
    int dc = 0;

    if (nlines > 1)
      return (0);
    values[nlines] = line->values;
    for (j = 0; j < 3; j++) {
      if (values[nlines][j] == '-') {
        if (dc)
          return (0);
        dc = 1;
      }
    }
    if (!dc)
      return (0);
    nlines++;
  }
  if (nlines == 1)
    return (0);
  /* At this point we know we have:
   **   3 inputs
   **   2 lines
   **   1 dash in each line
   ** If the two dashes are not in the same column, then there is
   ** exaclty one column without dashes: the control column.
   */
  for (j = 0; j < 3; j++) {
    if (values[0][j] == '-' && values[1][j] == '-')
      return (0);
    if (values[0][j] != '-' && values[1][j] != '-') {
      if (values[0][j] == values[1][j])
        return (0);
      controlC = j;
      controlR = values[0][j] == '0';
    }
  }
  assert(controlC != -1 && controlR != -1);
  /* At this point we know that there is indeed no column with two
   ** dashes. The control column has been identified, and we know that
   ** its two elelments are different. */
  for (j = 0; j < 3; j++) {
    if (j == controlC)
      continue;
    if (values[controlR][j] == '1') {
      mux[0] = j;
      phase[0] = 0;
    } else if (values[controlR][j] == '0') {
      mux[0] = j;
      phase[0] = 1;
    } else if (values[1 - controlR][j] == '1') {
      mux[1] = j;
      phase[1] = 0;
    } else if (values[1 - controlR][j] == '0') {
      mux[1] = j;
      phase[1] = 1;
    }
  }

  /* Get the inputs. */
  if (!st_lookup(hash, (char *)nd->inputs[controlC], (char **)&auxnd)) {
    goto failure;
  }
  if (auxnd->evaluating == TRUE) {
    fprintf(stderr, "Combinational Loop Found on Node %s.\n", auxnd->name);
    goto failure;
  }
  if (params == PORT_BNETLOCAL_DD) {
    Pdtutil_Assert(useAig == FALSE, "Wrong setting for AIG usage");
    if (auxnd->active == FALSE) {
      if (!Fsm_PortBnetBuildNodeBDD(dd,
          auxnd, hash, params, nodrop, -1, useAig)) {
        goto failure;
      }
    }
    f = Ddi_BddMakeLiteral(auxnd->var, 1);
  } else {                      /* params == PORT_BNETGLOBAL_DD */
    if (auxnd->dd == NULL) {
      if (!Fsm_PortBnetBuildNodeBDD(dd,
          auxnd, hash, params, nodrop, cutThresh, useAig)) {
        goto failure;
      }
    }
    f = Ddi_BddDup(auxnd->dd);
  }
  if (!st_lookup(hash, (char *)nd->inputs[mux[0]], (char **)&auxnd)) {
    goto failure;
  }
  if (auxnd->evaluating == TRUE) {
    fprintf(stderr, "Combinational Loop Found on Node %s.\n", auxnd->name);
    goto failure;
  }
  if (params == PORT_BNETLOCAL_DD) {
    Pdtutil_Assert(useAig == FALSE, "Wrong setting for AIG usage");
    if (auxnd->active == FALSE) {
      if (!Fsm_PortBnetBuildNodeBDD(dd,
          auxnd, hash, params, nodrop, -1, useAig)) {
        goto failure;
      }
    }
    g = Ddi_BddMakeLiteral(auxnd->var, 1);
  } else {                      /* params == PORT_BNETGLOBAL_DD */
    if (auxnd->dd == NULL) {
      myDbgCnt++;
      if (!Fsm_PortBnetBuildNodeBDD(dd,
          auxnd, hash, params, nodrop, cutThresh, useAig)) {
        goto failure;
      }
    }
    g = Ddi_BddDup(auxnd->dd);
  }
  if (phase[0]) {
    Ddi_BddNotAcc(g);
  }
  if (!st_lookup(hash, (char *)nd->inputs[mux[1]], (char **)&auxnd)) {
    goto failure;
  }
  if (auxnd->evaluating == TRUE) {
    fprintf(stderr, "Combinational Loop Found on Node %s.\n", auxnd->name);
    goto failure;
  }
  if (params == PORT_BNETLOCAL_DD) {
    Pdtutil_Assert(useAig == FALSE, "Wrong setting for AIG usage");
    if (auxnd->active == FALSE) {
      if (!Fsm_PortBnetBuildNodeBDD(dd,
          auxnd, hash, params, nodrop, -1, useAig)) {
        goto failure;
      }
    }
    h = Ddi_BddMakeLiteral(auxnd->var, 1);
  } else {                      /* params == PORT_BNETGLOBAL_DD */
    if (auxnd->dd == NULL) {
      if (!Fsm_PortBnetBuildNodeBDD(dd,
          auxnd, hash, params, nodrop, cutThresh, useAig)) {
        goto failure;
      }
    }
    h = Ddi_BddDup(auxnd->dd);
  }
  if (phase[1]) {
    Ddi_BddNotAcc(h);
  }
  nd->dd = Ddi_BddIte(f, g, h);

  Ddi_Free(f);
  Ddi_Free(g);
  Ddi_Free(h);

  /* Associate a variable to this node if local BDDs are being
   ** built. This is done at the end, so that the primary inputs tend
   ** to get lower indices.
   */
  if (params == PORT_BNETLOCAL_DD && nd->active == FALSE) {
    nd->var = Ddi_VarNew(dd);
    nd->active = TRUE;
    Ddi_VarAttachName(nd->var, nd->name);
  }

  nd->evaluating = FALSE;

  return (1);
failure:
  return (0);

}                               /* end of buildExorBDD */

/**Function********************************************************************

  Synopsis    [Orders network roots for variable ordering.]

  Description [Orders network roots for variable ordering. Returns
  an array with the ordered outputs and next state variables if
  successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static FsmPortBnetNode_t **
fsmPortBnetOrderRoots(
  FsmPortBnetNetwork_t * net,
  int *nroots
)
{
  int i, noutputs;
  FsmPortBnetNode_t *node;
  FsmPortBnetNode_t **nodes = NULL;

  /* Initialize data structures. */
  noutputs = net->noutputs;
  nodes = Pdtutil_Alloc(FsmPortBnetNode_t *, noutputs);
  if (nodes == NULL)
    goto endgame;

  /* Find output names and levels. */
  for (i = 0; i < net->nlatches; i++) {
    if (!st_lookup(net->hash, net->latches[i][0], (char **)&node)) {
      goto endgame;
    }
    nodes[i] = node;
  }
  for (i = 0; i < net->npos; i++) {
    if (!st_lookup(net->hash, net->outputs[i], (char **)&node)) {
      goto endgame;
    }
    nodes[i + net->nlatches] = node;
  }

  qsort((void *)nodes, noutputs, sizeof(FsmPortBnetNode_t *),
    (int (*)(const void *, const void *))fsmPortBnetLevelCompare);
  *nroots = noutputs;
  return (nodes);

endgame:
  if (nodes != NULL)
    Pdtutil_Free(nodes);
  return (NULL);

}                               /* end of fsmPortBnetOrderRoots */

/**Function********************************************************************

  Synopsis    [Does a DFS from a node ordering the inputs.]

  Description [Does a DFS from a node ordering the inputs. Returns
  1 if successful; 0 otherwise.]

  SideEffects [Changes visited fields of the nodes it visits.]

  SeeAlso     [Fsm_PortBnetDfsVariableOrder]

******************************************************************************/
static int
fsmPortBnetDfsOrder(
  Ddi_Mgr_t * dd,
  FsmPortBnetNetwork_t * net,
  FsmPortBnetNode_t * node,
  int useAig
)
{
  int i;
  FsmPortBnetNode_t *auxnd;
  FsmPortBnetNode_t **fanins;

  if (node->visited == 1) {
    return (1);
  }

  node->visited = 1;
  if (node->type == PORT_BNETINPUT_NODE ||
    node->type == PORT_BNETPRESENT_STATE_NODE) {
    node->var = Ddi_VarNew(dd);
    node->active = TRUE;
    Ddi_VarAttachName(node->var, node->name);
    if (useAig) {
      node->dd = Ddi_BddMakeLiteralAig(node->var, 1);
    } else {
      node->dd = Ddi_BddMakeLiteral(node->var, 1);
    }
    return (1);
  }

  fanins = Pdtutil_Alloc(FsmPortBnetNode_t *, node->ninp);
  if (fanins == NULL)
    return (0);

  for (i = 0; i < node->ninp; i++) {
    if (!st_lookup(net->hash, (char *)node->inputs[i], (char **)&auxnd)) {
      Pdtutil_Free(fanins);
      return (0);
    }
    fanins[i] = auxnd;
  }

  qsort((void *)fanins, node->ninp, sizeof(FsmPortBnetNode_t *),
    (int (*)(const void *, const void *))fsmPortBnetLevelCompare);
  for (i = 0; i < node->ninp; i++) {
    /* for (i = node->ninp - 1; i >= 0; i--) { */
    int res = fsmPortBnetDfsOrder(dd, net, fanins[i], useAig);

    if (res == 0) {
      Pdtutil_Free(fanins);
      return (0);
    }
  }
  Pdtutil_Free(fanins);
  return (1);

}                               /* end of fsmPortBnetLevelDFS */


/**Function********************************************************************

  Synopsis    [Comparison function used by qsort.]

  Description [Comparison function used by qsort to order the
  variables according to the number of keys in the subtables.
  Returns the difference in number of keys between the two
  variables being compared.]

  SideEffects [None]

******************************************************************************/
static int
fsmPortBnetLevelCompare(
  FsmPortBnetNode_t ** x,
  FsmPortBnetNode_t ** y
)
{
  return ((*y)->level - (*x)->level);

}                               /* end of fsmPortBnetLevelCompare */


/**Function********************************************************************

  Synopsis    [Read manual aux vars.]

  Description [Read multiplier auxiliary variables from file.]

  SideEffects [None]

******************************************************************************/
static int
readMultAux(
  Ddi_Mgr_t * ddm
)
{
  int ni, i, j, k;
  char buf[1000];
  Ddi_Vararray_t *v1, *v2;
  Ddi_Bdd_t *auxf, *lit1, *lit2;
  Ddi_Var_t *v;

  FILE *fp = fopen("manualaux.txt", "r");

  if (fp == NULL)
    return (0);

  fscanf(fp, "%d", &ni);

  v1 = Ddi_VararrayAlloc(ddm, 0);
  v2 = Ddi_VararrayAlloc(ddm, 0);
  manualAux = Ddi_BddarrayAlloc(ddm, 0);
  manualAuxVars = Ddi_VararrayAlloc(ddm, 0);
  manualAuxTot = Ddi_BddMakePartConjVoid(ddm);

  for (i = 0; i < ni; i++) {
    fscanf(fp, "%s", buf);
    v = Ddi_VarFromName(ddm, buf);
    if (v == NULL) {
      Pdtutil_Warning(v == NULL, "variable not found");
    } else {
      Ddi_VararrayInsertLast(v1, v);
    }
  }
  for (i = 0; i < ni; i++) {
    fscanf(fp, "%s", buf);
    v = Ddi_VarFromName(ddm, buf);
    if (v == NULL) {
      Pdtutil_Warning(v == NULL, "variable not found");
    } else {
      Ddi_VararrayInsertLast(v2, v);
    }
  }

  fclose(fp);

  for (k = 2 * (ni - 1); k >= 0; k--) {
    for (i = Ddi_VararrayNum(v1) - 1; i >= 0; i--) {
      lit1 = Ddi_BddMakeLiteral(Ddi_VararrayRead(v1, i), 1);
      /*for (j=0; j<Ddi_VararrayNum(v2); j++) { */
      j = k - i;
      if (j >= 0 && j < Ddi_VararrayNum(v2)) {
        lit2 = Ddi_BddMakeLiteral(Ddi_VararrayRead(v2, j), 1);
        v = Ddi_VarNewAtLevel(ddm, 0);

        sprintf(buf, "MAN$AUXVAR$%s$%s", Ddi_VarName(Ddi_VararrayRead(v1, i)),
          Ddi_VarName(Ddi_VararrayRead(v2, j)));

        Ddi_VarAttachName(v, buf);

        Ddi_VararrayInsertLast(manualAuxVars, v);
        auxf = Ddi_BddMakeLiteral(v, 1);
        Ddi_BddAndAcc(lit2, lit1);
        Ddi_BddarrayInsertLast(manualAux, lit2);

        Ddi_BddXnorAcc(auxf, lit2);

        Ddi_BddPartInsertLast(manualAuxTot, auxf);

        Ddi_Free(lit2);
        Ddi_Free(auxf);

#if 0
        lit2 = Ddi_BddMakeLiteral(Ddi_VararrayRead(v2, j), 0);
        v = Ddi_VarNewAtLevel(ddm, 0);

        sprintf(buf, "MAN$AUXVAR$%s$%s", Ddi_VarName(Ddi_VararrayRead(v1, i)),
          Ddi_VarName(Ddi_VararrayRead(v2, j)));
        Ddi_VarAttachName(v, buf);

        Ddi_VararrayInsertLast(manualAuxVars, v);
        auxf = Ddi_BddMakeLiteral(v, 1);
        Ddi_BddAndAcc(lit2, lit1);
        Ddi_BddarrayInsertLast(manualAux, lit2);

        Ddi_BddXnorAcc(auxf, lit2);

        Ddi_BddPartInsertLast(manualAuxTot, auxf);

        Ddi_Free(lit2);
        Ddi_Free(auxf);

        Ddi_BddNotAcc(lit1);
        lit2 = Ddi_BddMakeLiteral(Ddi_VararrayRead(v2, j), 1);
        v = Ddi_VarNewAtLevel(ddm, 0);

        sprintf(buf, "MAN$AUXVAR$%s$%s", Ddi_VarName(Ddi_VararrayRead(v1, i)),
          Ddi_VarName(Ddi_VararrayRead(v2, j)));
        Ddi_VarAttachName(v, buf);

        Ddi_VararrayInsertLast(manualAuxVars, v);
        auxf = Ddi_BddMakeLiteral(v, 1);
        Ddi_BddAndAcc(lit2, lit1);
        Ddi_BddarrayInsertLast(manualAux, lit2);

        Ddi_BddXnorAcc(auxf, lit2);

        Ddi_BddPartInsertLast(manualAuxTot, auxf);

        Ddi_Free(lit2);
        Ddi_Free(auxf);

        lit2 = Ddi_BddMakeLiteral(Ddi_VararrayRead(v2, j), 0);
        v = Ddi_VarNewAtLevel(ddm, 0);

        sprintf(buf, "MAN$AUXVAR$%s$%s", Ddi_VarName(Ddi_VararrayRead(v1, i)),
          Ddi_VarName(Ddi_VararrayRead(v2, j)));
        Ddi_VarAttachName(v, buf);

        Ddi_VararrayInsertLast(manualAuxVars, v);
        auxf = Ddi_BddMakeLiteral(v, 1);
        Ddi_BddAndAcc(lit2, lit1);
        Ddi_BddarrayInsertLast(manualAux, lit2);

        Ddi_BddXnorAcc(auxf, lit2);

        Ddi_BddPartInsertLast(manualAuxTot, auxf);

        Ddi_Free(lit2);
        Ddi_Free(auxf);

        Ddi_BddNotAcc(lit1);
#endif
      }
      Ddi_Free(lit1);
    }

#if 0
    for (i = Ddi_VararrayNum(v1) - 1; i >= 0; i--) {
      lit1 = Ddi_BddMakeLiteral(Ddi_VararrayRead(v1, i), 1);
      /*for (j=0; j<Ddi_VararrayNum(v2); j++) { */
      j = 2 * ni - 1 - k - i;
      if (j >= 0 && j < Ddi_VararrayNum(v2)) {
        lit2 = Ddi_BddMakeLiteral(Ddi_VararrayRead(v2, j), 1);
        v = Ddi_VarNewAtLevel(ddm, 0);

        sprintf(buf, "MAN$AUXVAR$%s$%s", Ddi_VarName(Ddi_VararrayRead(v1, i)),
          Ddi_VarName(Ddi_VararrayRead(v2, j)));

        Ddi_VarAttachName(v, buf);

        Ddi_VararrayInsertLast(manualAuxVars, v);
        auxf = Ddi_BddMakeLiteral(v, 1);
        Ddi_BddAndAcc(lit2, lit1);
        Ddi_BddarrayInsertLast(manualAux, lit2);

        Ddi_BddXnorAcc(auxf, lit2);

        Ddi_BddPartInsertLast(manualAuxTot, auxf);

        Ddi_Free(lit2);
        Ddi_Free(auxf);
      }
      Ddi_Free(lit1);
    }
#endif
  }

  /* Ddi_BddSetClustered(manualAuxTot,100); */
  Ddi_Free(v1);
  Ddi_Free(v2);

  return (1);
}




/**Function********************************************************************

  Synopsis    [print combinational loop.]

  Description [print combinational loop by stepping through gates. evaluating
               flag used.]

  SideEffects [None]

******************************************************************************/
static int
printCombLoop(
  FILE * fp,
  st_table * hash /* symbol table of the boolean network */ ,
  FsmPortBnetNode_t * nd,
  FsmPortBnetNode_t * startnd
)
{
  int i;
  FsmPortBnetNode_t *auxnd;

  fprintf(stderr, "Error on Node %s.\n", nd->name);

  for (i = 0; i < nd->ninp; i++) {
    if (!st_lookup(hash, (char *)nd->inputs[i], (char **)&auxnd)) {
      goto failure;
    }
    if (auxnd->evaluating == TRUE) {
      if (auxnd == startnd) {
        return (1);
      } else {
        printCombLoop(fp, hash, auxnd, startnd);
      }
      break;
    }
  }

  return (1);
failure:return (0);
}




/**Function********************************************************************

  Synopsis    [print combinational loop.]

  Description [print combinational loop by stepping through gates. evaluating
               flag used.]

  SideEffects [None]

******************************************************************************/
static int
setupCombLoop(
  st_table * hash /* symbol table of the boolean network */ ,
  FsmPortBnetNode_t * nd,
  FsmPortBnetNode_t * startnd,
  int phase
)
{
  int i;
  FsmPortBnetNode_t *auxnd;
  Ddi_Bdd_t *cutf, *cutrel;
  Ddi_Var_t *cutv;
  Ddi_Varset_t *smoothv;

  switch (phase) {

    case 0:

      nd->combLoop = TRUE;
      for (i = 0; i < nd->ninp; i++) {
        if (!st_lookup(hash, (char *)nd->inputs[i], (char **)&auxnd)) {
          goto failure;
        }
        if (auxnd->evaluating == TRUE) {
          if (auxnd == startnd) {
            return (1);
          } else {
            setupCombLoop(hash, auxnd, startnd, phase);
          }
          break;
        }
      }

      break;

    case 1:
    default:

      if (nd->dd != NULL) {
        cutf = startnd->cutdd;
        cutv = startnd->var;
        Ddi_BddSuppAttach(cutf);
        if (Ddi_VarInVarset(Ddi_BddSuppRead(cutf), cutv)) {
          fprintf(stderr,
            "Combinational Loop with Cyclic Dependency Found.\n");
          return (0);
        }
        Ddi_BddSuppDetach(cutf);
        cutrel = Ddi_BddMakeLiteral(cutv, 1);
        Ddi_BddXnorAcc(cutrel, cutf);
        smoothv = Ddi_VarsetMakeFromVar(cutv);

        Ddi_BddAndExistAcc(nd->dd, cutrel, smoothv);

        Ddi_Free(cutrel);
        Ddi_Free(smoothv);
      }

      for (i = 0; i < nd->ninp; i++) {
        if (!st_lookup(hash, (char *)nd->inputs[i], (char **)&auxnd)) {
          goto failure;
        }
        if (auxnd->combLoop == TRUE) {
          if (auxnd == startnd) {
            return (1);
          } else {
            setupCombLoop(hash, auxnd, startnd, phase);
          }
          auxnd->combLoop = FALSE;
          break;
        }
      }

  }

  return (1);
failure:return (0);
}


/**Function********************************************************************

  Synopsis    [compute structural COI.]
  Description [compute structural COI.]
  SideEffects [None]

******************************************************************************/
static int
propagateCOI(
  st_table * hash /* symbol table of the boolean network */ ,
  FsmPortBnetNode_t * nd,
  int stopAtLatch
)
{
  int i;
  FsmPortBnetNode_t *auxnd;

  if (stopAtLatch == 1 && nd->type == PORT_BNETPRESENT_STATE_NODE) {
    if (nd->coiFlag == FALSE) {
      nd->coiFlag = 2;
    }
    return (1);
  } else if (stopAtLatch > 1 && nd->type == PORT_BNETPRESENT_STATE_NODE) {
    stopAtLatch--;
  }

  nd->coiFlag = TRUE;

  for (i = 0; i < nd->ninp; i++) {
    if (!st_lookup(hash, (char *)nd->inputs[i], (char **)&auxnd)) {
      goto failure;
    }
    if (auxnd->coiFlag == FALSE) {
      propagateCOI(hash, auxnd, stopAtLatch);
    }
  }

  return (1);
failure:return (0);
}

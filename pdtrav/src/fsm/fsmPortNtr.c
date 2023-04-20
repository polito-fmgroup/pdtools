/**CFile***********************************************************************

  FileName    [fsmPortNtr.c]

  PackageName [fsm]

  Synopsis    [Nanotrav parts used by pdtrav]

  Description [This package contains functions, types and constants taken from
    nanotrav to be used in pdtrav. The original names have been modified adding
    the "FsmPort" prefix.<p>
    FsmPortNtr.c contains the parts taken from ntr.c.]

  SeeAlso     [fsmPortBnet.c pdtrav]

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

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static void fsmPortNtrInitializeCount(
  FsmPortBnetNetwork_t * net,
  FsmPortNtrOptions_t * option
);
static void fsmPortNtrCountDFS(
  FsmPortBnetNetwork_t * net,
  FsmPortBnetNode_t * node
);

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exfsmPorted functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Builds DDs for a network outputs and next state
  functions.]

  Description [Builds DDs for a network outputs and next state
  functions. The method is really brain-dead, but it is very simple.
  Returns 1 in case of success; 0 otherwise. Some inputs to the network
  may be shared with another network whose DDs have already been built.
  In this case we want to share the DDs as well.]

  SideEffects [the dd fields of the network nodes are modified. Uses the
  count fields of the nodes.]

  SeeAlso     []

******************************************************************************/

int
Fsm_PortNtrBuildDDs(
  FsmPortBnetNetwork_t * net /* network for which DDs are to be built */ ,
  Ddi_Mgr_t * dd /* DD manager */ ,
  FsmPortNtrOptions_t * option /* option structure */ ,
  Pdtutil_VariableOrderFormat_e ordFileFormat
)
{
  int verbosity = option->verb;
  int result;
  int i;
  FsmPortBnetNode_t *node;

  /* First assign variables to inputs if the order is provided.
   ** (Either in the .blif file or in an order file.)
   */
  if (option->ordering == PI_PS_FROM_FILE) {
    /* Follow order given in input file. First primary inputs
     ** and then present state variables.
     */
    for (i = 0; i < net->npis; i++) {
      if (!st_lookup(net->hash, net->inputs[i], (char **)&node)) {
        return (0);
      }
      result = Fsm_PortBnetBuildNodeBDD(dd, node, net->hash,
        option->locGlob, option->nodrop, -1, option->useAig);
      if (result == 0)
        return (0);
    }
    for (i = 0; i < net->nlatches; i++) {
      if (!st_lookup(net->hash, net->latches[i][1], (char **)&node)) {
        return (0);
      }
      result = Fsm_PortBnetBuildNodeBDD(dd, node, net->hash,
        option->locGlob, option->nodrop, -1, option->useAig);
      if (result == 0)
        return (0);
    }
  } else if (option->ordering == PI_PS_GIVEN) {
    result = Fsm_PortBnetReadOrder(dd, option->orderPiPs, net,
      option->locGlob, option->nodrop, ordFileFormat, option->useAig);
    if (result == 0)
      return (0);
  } else {
    result = Fsm_PortBnetDfsVariableOrder(dd, net, option->useAig);
    if (result == 0)
      return (0);
  }
  /* At this point the BDDs of all primary inputs and present state
   ** variables have been built. */

  /* Currently noBuild doesn't do much. */
  if (option->noBuild == TRUE)
    return (1);

  if (option->locGlob == PORT_BNETLOCAL_DD) {
    node = net->nodes;
    while (node != NULL) {
      result = Fsm_PortBnetBuildNodeBDD(dd,
        node, net->hash, PORT_BNETLOCAL_DD, TRUE, -1, option->useAig);
      if (result == 0) {
        return (0);
      }
      Pdtutil_VerbosityLocal(verbosity, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "%s", node->name)
        //Cudd_PrintDebug(dd,node->dd,Cudd_ReadSize(dd),verbosity);
        );
      node = node->next;
    }
  } else {                      /* option->locGlob == PORT_BNETGLOBAL_DD */
    /* Create BDDs with DFS from the primary outputs and the next
     ** state functions. If the inputs had not been ordered yet,
     ** this would result in a DFS order for the variables.
     */

    fsmPortNtrInitializeCount(net, option);

    if (option->node != NULL) {
      if (!st_lookup(net->hash, option->node, (char **)&node)) {
        return (0);
      }
      result = Fsm_PortBnetBuildNodeBDD(dd, node, net->hash,
        PORT_BNETGLOBAL_DD, option->nodrop, option->cutThresh, option->useAig);
      if (result == 0)
        return (0);
    } else {
      if (option->stateOnly == FALSE) {
        for (i = 0; i < net->npos; i++) {
          if (!st_lookup(net->hash, net->outputs[i], (char **)&node)) {
            continue;
          }
          if (option->coiReduction == TRUE && node->coiFlag == FALSE) {
            node->dd = Ddi_BddMakeConst(dd, 1);
            continue;
          }
          result = Fsm_PortBnetBuildNodeBDD(dd, node, net->hash,
            PORT_BNETGLOBAL_DD, option->nodrop, option->cutThresh,
            option->useAig);
          if (result == 0)
            return (0);
          {
            extern int maxNodeSize;
            extern int nDropped;
            extern int nDone;
            extern int doReadMult;

            if (doReadMult) {
              Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
                Pdtutil_VerbLevelNone_c,
                fprintf(stdout,
                  "Live nodes (peak): %u(%u) out: %s (%d) - max node: %d.\n",
                  Ddi_MgrReadLiveNodes(dd),
                  Ddi_MgrReadPeakLiveNodeCount(dd), node->name,
                  Ddi_BddSize(node->dd), maxNodeSize);
                fprintf(stdout,
                  "  Done / dropped / active: %d / %d / %d\n.\n",
                  nDone, nDropped, nDone - nDropped)
                );
              if ((i + 1) % 1 == 0) {
                FsmPortBnetGenSliceVars(dd, net);
              }
            }
          }
          if (option->progress) {
            Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
              Pdtutil_VerbLevelNone_c, fprintf(stdout, "%s.\n", node->name)
              );
          }
#if 0
          Cudd_PrintDebug(dd, node->dd, net->ninputs, option->verb);
#endif
        }
      }
      for (i = 0; i < net->nlatches; i++) {
        if (!st_lookup(net->hash, net->latches[i][0], (char **)&node)) {
          continue;
        }
        if (option->coiReduction == TRUE && node->coiFlag == FALSE) {
          node->dd = Ddi_BddMakeConst(dd, 1);
          continue;
        }
        result = Fsm_PortBnetBuildNodeBDD(dd, node, net->hash,
          PORT_BNETGLOBAL_DD, option->nodrop, option->cutThresh,
          option->useAig);
        if (result == 0)
          return (0);
        if (option->progress) {
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c, fprintf(stdout, "%s.\n", node->name)
            );
        }
#if 0
        Cudd_PrintDebug(dd, node->dd, net->ninputs, option->verb);
#endif
      }
    }
    /* Make sure all inputs have a DD and dereference the DDs of
     ** the nodes that are not reachable from the outputs.
     */
    for (i = 0; i < net->npis; i++) {
      if (!st_lookup(net->hash, net->inputs[i], (char **)&node)) {
        return (0);
      }
      result = Fsm_PortBnetBuildNodeBDD(dd, node, net->hash,
        PORT_BNETGLOBAL_DD, option->nodrop, option->cutThresh, option->useAig);
      if (result == 0)
        return (0);
      if (node->count == -1)
        Ddi_Free(node->dd);
    }
    for (i = 0; i < net->nlatches; i++) {
      if (!st_lookup(net->hash, net->latches[i][1], (char **)&node)) {
        return (0);
      }
      result = Fsm_PortBnetBuildNodeBDD(dd, node, net->hash,
        PORT_BNETGLOBAL_DD, option->nodrop, option->cutThresh, option->useAig);
      if (result == 0)
        return (0);
      /* disabled to preserve initial state evaluation
         if (node->count == -1) Ddi_Free(node->dd);
       */
    }

    /* Dispose of the BDDs of the internal nodes if they have not
     ** been dropped already.
     */
    if (option->nodrop == TRUE) {
      for (node = net->nodes; node != NULL; node = node->next) {
        if (node->dd != NULL && !node->cutpoint && node->count != -1 && (node->type == PORT_BNETINTERNAL_NODE   /*||
                                                                                                                   node->type == PORT_BNETINPUT_NODE ||
                                                                                                                   node->type == PORT_BNETPRESENT_STATE_NODE */ )) {
          Ddi_Free(node->dd);
          if (node->type == PORT_BNETINTERNAL_NODE)
            node->dd = NULL;
        }
      }
    }
  }

  return (1);

}                               /* end of buildDD */

/**Function********************************************************************

  Synopsis    [Builds the BDD of the initial state(s).]

  Description [Builds the BDD of the initial state(s).  Returns a BDD
  if successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
Ddi_Bdd_t *
Fsm_PortNtrInitState(
  Ddi_Mgr_t * dd,
  FsmPortBnetNetwork_t * net,
  FsmPortNtrOptions_t * option
)
{
  Ddi_Bdd_t *res, *x;
  FsmPortBnetNode_t *node;
  int i;

  if (option->load) {
    res = Ddi_BddLoad(dd, DDDMP_VAR_MATCHIDS,
      DDDMP_MODE_DEFAULT, option->loadfile, NULL);
  } else {
    if (option->useAig) {
      res = Ddi_BddMakeConstAig(dd, 1);
    } else {
      res = Ddi_BddMakeConst(dd, 1);
    }
    if (net->nlatches == 0)
      return (res);

    for (i = 0; i < net->nlatches; i++) {
      if (!st_lookup(net->hash, net->latches[i][1], (char **)&node)) {
        goto endgame;
      }
      x = Ddi_BddDup(node->dd);
      switch (net->latches[i][2][0]) {
        case '0':
          Ddi_BddAndAcc(res, Ddi_BddNotAcc(x));
          break;
        case '1':
          Ddi_BddAndAcc(res, x);
          break;
        default:               /* don't care */
          break;
      }
      Ddi_Free(x);

    }
  }
  return (res);

endgame:

  return (NULL);

}                               /* end of Fsm_PortNtrinitState */

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Initializes the count fields used to drop DDs.]

  Description [Initializes the count fields used to drop DDs.
  Before actually building the BDDs, we perform a DFS from the outputs
  to initialize the count fields of the nodes.  The initial value of the
  count field will normally coincide with the fanout of the node.
  However, if there are nodes with no path to any primary output or next
  state variable, then the initial value of count for some nodes will be
  less than the fanout. For primary outputs and next state functions we
  add 1, so that we will never try to free their DDs. The count fields
  of the nodes that are not reachable from the outputs are set to -1.]

  SideEffects [Changes the count fields of the network nodes. Uses the
  visited fields.]

  SeeAlso     []

******************************************************************************/
static void
fsmPortNtrInitializeCount(
  FsmPortBnetNetwork_t * net,
  FsmPortNtrOptions_t * option
)
{
  FsmPortBnetNode_t *node;
  int i;

  if (option->node != NULL) {
    if (!st_lookup(net->hash, option->node, (char **)&node)) {
      fprintf(stderr, "Warning: node %s not found!.\n", option->node);
    } else {
      fsmPortNtrCountDFS(net, node);
      node->count++;
    }
  } else {
    if (option->stateOnly == FALSE) {
      for (i = 0; i < net->npos; i++) {
        if (!st_lookup(net->hash, net->outputs[i], (char **)&node)) {
          fprintf(stdout, "Warning: output %s is not driven!.\n",
            net->outputs[i]);
          continue;
        }
        fsmPortNtrCountDFS(net, node);
        node->count++;
      }
    }
    for (i = 0; i < net->nlatches; i++) {
      if (!st_lookup(net->hash, net->latches[i][0], (char **)&node)) {
        fprintf(stdout,
          "Warning: latch input %s is not driven!.\n", net->outputs[i]);
        continue;
      }
      fsmPortNtrCountDFS(net, node);
      node->count++;
    }
  }

  /* Clear visited flags. */
  node = net->nodes;
  while (node != NULL) {
    if (node->visited == 0) {
      node->count = -1;
    } else {
      node->visited = 0;
    }
    node = node->next;
  }

}                               /* end of fsmPortNtrInitializeCount */

/**Function********************************************************************

  Synopsis    [Does a DFS from a node setting the count field.]

  Description []

  SideEffects [Changes the count and visited fields of the nodes it
  visits.]

  SeeAlso     [fsmPortNtrLevelDFS]

******************************************************************************/
static void
fsmPortNtrCountDFS(
  FsmPortBnetNetwork_t * net,
  FsmPortBnetNode_t * node
)
{
  int i;
  FsmPortBnetNode_t *auxnd;

  node->count++;

  if (node->visited == 1) {
    return;
  }

  node->visited = 1;

  for (i = 0; i < node->ninp; i++) {
    if (!st_lookup(net->hash, (char *)node->inputs[i], (char **)&auxnd)) {
      exit(2);
    }
    fsmPortNtrCountDFS(net, auxnd);
  }

}                               /* end of fsmPortNtrCountDFS */

/***************************************************************************
Copyright (c) 2006-2007, Armin Biere, Johannes Kepler University.
Copyright (c) 2006, Marc Herbstritt, University of Freiburg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
***************************************************************************/

/**CFile***********************************************************************

  FileName    [aigtoblif.c]

  PackageName [fsm]
  Synopsis    [aiger used by pdtrav]

  Description [This package contains functions, types and constants taken from
    aiger to be used in pdtrav.]

  SeeAlso     [aiger.c pdtrav]

  Author      [Luz Garcia]

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
#include "aiger.h"
#include "fsm.h"
#include "fsmInt.h"
#include "baigInt.h"
#include "pdtutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

static FILE *file;
static aiger *mgr;
static int count;
static int verbose;
static char buffer[20];

//static void Bnet_LuzPrintNetwork(BnetNetwork *);

/**Function********************************************************************

  Synopsis           []

  Description        []

  SideEffects        []

  SeeAlso            [get_name_file,readFsm]

******************************************************************************/

int
Fsm_MgrLoadAiger(
  Fsm_Mgr_t ** fsmMgrP /* FSM Pointer */ ,
  Ddi_Mgr_t * dd /* Main DD manager */ ,
  char *fileFsmName /* Input file name */ ,
  char *fileOrdName /* ORD File Name */ ,
  Ddi_Vararray_t *mapVars,
  Pdtutil_VariableOrderFormat_e ordFileFormat
)
{
  bAig_Manager_t *bmgr;
  Fsm_Mgr_t *fsmMgr;
  int i, index = 0;
  int m, ni, nl, no, na, nb, nc, nj, nf, ntotv, nxl;
  Ddi_Vararray_t *vars;
  Ddi_Var_t *v;
  int *auxids, flagError;
  char **names;
  char *nameext;
  long fsmTime;

  aiger *mgr;
  const char *error;
  Ddi_Bdd_t *litBdd;
  static char buf[100000];
  int sortDfs = 1;
  Ddi_Bdd_t *bddZero = NULL;
  aiger_symbol *targets;
  int useBddVars = 1;
  bAigEdge_t *nodesArray;

  fsmTime = util_cpu_time();

  /*------------------------ Check For FSM Structure ------------------------*/

  fsmMgr = *fsmMgrP;
  if (fsmMgr == NULL) {
    Pdtutil_Warning(1, "Allocate a new FSM structure");
    fsmMgr = Fsm_MgrInit(NULL, NULL);
  }

  Pdtutil_VerbosityMgr(fsmMgr, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "(Loading File %s).\n", fileFsmName)
    );

  Fsm_MgrSetDdManager(fsmMgr, dd);

  if (Ddi_MgrReadMgrBaig(dd) == NULL) {
    Ddi_AigInit(dd, 100);
  }
  bmgr = Ddi_MgrReadMgrBaig(dd);

  /*-------------------------- Read AIGER File  --------------------------*/

  mgr = aiger_init();
  error = aiger_open_and_read_from_file(mgr, fileFsmName);

  /*--------------- Check for Errors and Perform Final Copies ---------------*/

  if (error != NULL) {
    fprintf(stderr, "Error Loading FSM %s.\n", error);
    Fsm_MgrQuit(fsmMgr);
    aiger_reset(mgr);
    return 1;
  }


  /*--------------- Setup data structures   ---------------*/

  m = mgr->maxvar;
  ni = mgr->num_inputs;
  nl = mgr->num_latches;
  no = mgr->num_outputs;
  nb = mgr->num_bad;
  na = mgr->num_ands;
  nc = mgr->num_constraints;
  nj = mgr->num_justice;
  nf = mgr->num_fairness;

  //  printf("# CONSTRAINTS: %d\n", nc);

  nodesArray = Pdtutil_Alloc(bAigEdge_t, m + 1);
  nodesArray[0] = bAig_Zero;

  targets = mgr->outputs;
  if (nb > 0) {
    /* ignore outputs - read targets from bad array */
    no = nb;
    targets = mgr->bad;
  }

  /*--------------- Setup input and latch variables  ---------------*/

  if (nl > 0) {
    fsmMgr->var.ps = Ddi_VararrayAlloc(dd, nl);
    fsmMgr->var.ns = Ddi_VararrayAlloc(dd, nl);
    fsmMgr->delta.bdd = Ddi_BddarrayAlloc(dd, nl);
  } else {
    fsmMgr->var.ps = NULL;
    fsmMgr->var.ns = NULL;
    fsmMgr->delta.bdd = NULL;
  }

  if (nl > 550000) {
    useBddVars = 0;
  }

  ntotv = ni + 2 * nl;

  fsmMgr->lambda.bdd = Ddi_BddarrayAlloc(dd, 0);

  Pdtutil_Assert(Fsm_MgrReadUseAig(fsmMgr),
    "AIG support required for AIGER load");
  fsmMgr->constraint.bdd = Ddi_BddMakeConstAig(dd, 1);

  Ddi_MgrConsistencyCheck(dd);


  /*
   *  Set the field Variable - Input of the FSM
   */


  /*------------------ Translate Array of Input Variables -------------------*/

  /* GpC: force void input array when no input found */

  fsmMgr->var.i = Ddi_VararrayAlloc(dd, ni);
  for (i = 0; i < ni; i++) {
    unsigned lit = mgr->inputs[i].lit;
    int i1 = i + 1;
    Ddi_Var_t *var;
    char *s, *name = aiger_get_symbol(mgr, lit);

    Pdtutil_Assert(lit == 2 * i1, "Wrong variable id in AIGER manager");

    if (mapVars == NULL || name!=NULL) {
      if (name == NULL) {
	Pdtutil_Assert(lit%2==0 && lit/2>0,"wrong aiger variable lit");
	sprintf(buf, "i%d", lit/2-1);
	name = buf;
      }
      
      for (s=name; *s!='\0'; s++) {
	if (*s=='#' || *s==':' || *s=='(' || *s==')') *s='_';
      }
      var = Ddi_VarFindOrAdd(dd, name, useBddVars);
      Ddi_VarAttachAuxid(var, i);
    }
    else {
      Pdtutil_Assert(Ddi_VararrayNum(mapVars)==ni,
                     "wrong number of vars");
      var = Ddi_VararrayRead(mapVars,i);
    }
    
    //Ddi_VarAttachName(var, name);
    Ddi_VararrayWrite(fsmMgr->var.i, i, var);

    nodesArray[i + 1] = Ddi_VarToBaig(var);
    bAig_Ref(bmgr, nodesArray[i + 1]);
    index++;
  }

  Ddi_MgrConsistencyCheck(dd);

  /*-------- Translate Array of Present State Variables and Delta -----------*/

  if (nl > 0) {
    fsmMgr->init.bdd = Ddi_BddMakeConstAig(dd, 1);
    fsmMgr->init.string = Pdtutil_Alloc(char,
      nl + 2
    );
  } else {
    fsmMgr->init.bdd = NULL;
    fsmMgr->reached.bdd = NULL;
  }

  for (i = 0; i < nl; i++) {
    unsigned lit = mgr->latches[i].lit;
    int i1 = i + 1;
    Ddi_Var_t *var;
    char *s, *name = aiger_get_symbol(mgr, lit);
    Ddi_Var_t *varNs;

    Pdtutil_Assert(lit == 2 * (i1 + ni), "Wrong variable id in AIGER manager");

    if (name == NULL) {
      sprintf(buf, "l%d", lit);
    } else {
      sprintf(buf, "%s", name);
    }
    name = buf;
    for (s = name; *s != '\0'; s++) {
      if (*s == '#' || *s == ':' || *s == '(' || *s == ')')
        *s = '_';
    }

    var = Ddi_VarFindOrAdd(dd, name, useBddVars);
    Ddi_VarAttachAuxid(var, i+ni);
    //Ddi_VarAttachName(var, name);
    Ddi_VararrayWrite(fsmMgr->var.ps, i, var);
    index++;

    nodesArray[ni + i + 1] = Ddi_VarToBaig(var);
    bAig_Ref(bmgr, nodesArray[ni + i + 1]);

    /* next state vars */

    sprintf(buf, "%s$NS", name);

    /* Create array of next state var */
    //    varNs = Ddi_VarFindOrAddAfterVar(dd,buf,useBddVars,var);
    varNs = Ddi_VarFindOrAdd(dd, buf, useBddVars);
    Ddi_VarAttachAuxid(varNs, i + ni + nl);
    //Ddi_VarAttachName(var, buf);
    Ddi_VararrayWrite(fsmMgr->var.ns, i, varNs);
    index++;
  }

  if (nl > 0) {

    bAigEdge_t init0Baig, initBaig = bAig_One;

    nxl=0;
    bAig_Ref(bmgr, initBaig);
    for (i = nl - 1; i >= 0; i--) {

      Ddi_Var_t *var = Ddi_VararrayRead(fsmMgr->var.ps, i);
      bAigEdge_t litBaig = Ddi_VarToBaig(var);
      int val = mgr->latches[i].reset;

      /* initial state is all 0's */
      if (val < 2) {
        fsmMgr->init.string[i] = (char)val + '0';
      } else {
        fsmMgr->init.string[i] = 'x';
        nxl++;
      }
      if (mgr->latches[i].reset < 2) {
        if (mgr->latches[i].reset == 0) {
          litBaig = bAig_Not(litBaig);
        }
        init0Baig = initBaig;
        initBaig = bAig_And(bmgr, initBaig, litBaig);
        bAig_Ref(bmgr, initBaig);
        bAig_RecursiveDeref(bmgr, init0Baig);
      }
    }
    fsmMgr->init.string[nl] = '\0';

    Ddi_Free(fsmMgr->init.bdd);
    fsmMgr->init.bdd = Ddi_BddMakeFromBaig(dd, initBaig);
    bAig_RecursiveDeref(bmgr, initBaig);
    fsmMgr->reached.bdd = Ddi_BddDup(fsmMgr->init.bdd);
    if (nxl>0) {
      fsmMgr->stats.xInitLatches = nxl;
    }
  }
  //  Ddi_MgrConsistencyCheck(dd);

  /*---------------- Translate Array of And Gates  ---------------------*/

  for (i = 0; i < mgr->num_ands; i++) {
    aiger_and *n = &(mgr->ands[i]);
    unsigned rhs0 = n->rhs0;
    unsigned rhs1 = n->rhs1;
    unsigned lit = n->lhs;

    bAigEdge_t r0, r1, r;

    Pdtutil_Assert(lit == 2 * (i + ni + nl + 1),
      "Wrong node id in AIGER manager");
    Pdtutil_Assert(rhs0 < lit
      && rhs1 < lit, "Wrong node order in AIGER manager");

    r0 = nodesArray[rhs0 / 2];
    r1 = nodesArray[rhs1 / 2];
    if (aiger_sign(rhs0))
      r0 = bAig_Not(r0);
    if (aiger_sign(rhs1))
      r1 = bAig_Not(r1);

    r = bAig_And(bmgr, r0, r1);
    nodesArray[ni + nl + i + 1] = r;
    bAig_Ref(bmgr, r);
  }

  for (i = 0; i < nl; i++) {
    Ddi_Bddarray_t *delta = fsmMgr->delta.bdd;
    unsigned d = mgr->latches[i].next;
    Ddi_Bdd_t *delta_i;

    delta_i = Ddi_BddMakeFromBaig(dd, nodesArray[d / 2]);
    if (aiger_sign(d))
      Ddi_BddNotAcc(delta_i);

    Ddi_BddarrayWrite(delta, i, delta_i);
    Ddi_Free(delta_i);
  }

  /*---------------- Translate Array of Output Function ---------------------*/

  for (i = 0; i < no; i++) {
    unsigned lit = targets[i].lit;
    int i1 = i + 1;
    char *name = targets[i].name;
    Ddi_Bdd_t *oBdd;

    oBdd = Ddi_BddMakeFromBaig(dd, nodesArray[lit / 2]);
    if (aiger_sign(lit))
      Ddi_BddNotAcc(oBdd);

    if (name == NULL) {
      sprintf(buf, "o%d", 2 * i1);
      name = buf;
    }

    if (name != NULL
      && strncmp(name, "constraint:", strlen("constraint:")) == 0) {
      Ddi_BddSetPartConj(fsmMgr->constraint.bdd);
      Ddi_BddPartInsertLast(fsmMgr->constraint.bdd, oBdd);
    } else if (name != NULL
      && strncmp(name, "invariant:", strlen("invariant")) == 0) {
      Ddi_BddSetPartConj(fsmMgr->constraint.bdd);
      Ddi_BddPartInsertLast(fsmMgr->constraint.bdd, oBdd);
    } else {
      Ddi_BddarrayInsertLast(fsmMgr->lambda.bdd, oBdd);
    }
#if 0
    if (net->outputIsConstraint[i]) {
      Ddi_BddAndAcc(fsmMgr->constraint.bdd, node->dd);
    }
#endif
    Ddi_Free(oBdd);
  }

  no = Ddi_BddarrayNum(fsmMgr->lambda.bdd);

  if (nc > 0) {
    Ddi_BddSetPartConj(fsmMgr->constraint.bdd);
  }
  for (i = 0; i < nc; i++) {
    unsigned lit = mgr->constraints[i].lit;
    int i1 = i + 1;
    char *name = NULL;
    Ddi_Bdd_t *cBdd;

    cBdd = Ddi_BddMakeFromBaig(dd, nodesArray[lit / 2]);
    if (aiger_sign(lit))
      Ddi_BddNotAcc(cBdd);

    //    printf("CONSTR[%d]\n",i);
    //if (Ddi_BddSize(cBdd)>20) {
    //  logBddSupp(cBdd);
    //}
    //else {
    //  DdiLogBdd(cBdd,0);
    //}
    if (name == NULL) {
      sprintf(buf, "c%d", 2 * i1);
    }

    Ddi_BddPartInsertLast(fsmMgr->constraint.bdd, cBdd);
    //    fsmMgr->name.c[i] = Pdtutil_StrDup (name);
    Ddi_Free(cBdd);
  }
  int chkConstr = 1;
  if (chkConstr) {
    Ddi_Bdd_t *myChk = Ddi_BddMakeConstAig(dd, 1);
    for (int i = 0; i<Ddi_BddPartNum(fsmMgr->constraint.bdd); i++) {
      Ddi_Bdd_t *c_i = Ddi_BddPartRead(fsmMgr->constraint.bdd, i);
      if (!Ddi_AigSatAnd(myChk,c_i,NULL)) {
        printf("unconsistency found: %d\n", i);
        //Ddi_BddPartRemove(fsmMgr->constraint.bdd,i); i--; continue;
        for (int j = 0; j<i; j++) {
          Ddi_Bdd_t *c_j = Ddi_BddPartRead(fsmMgr->constraint.bdd, j);
          if (!Ddi_AigSatAnd(c_i,c_j,NULL)) {
            printf("couple found: %d %d\n", j, i);
            break;
          }
        }
        break;
      }
      Ddi_BddAndAcc(myChk,c_i);
    }
    Ddi_Free(myChk);
  }

  Ddi_BddSetAig(fsmMgr->constraint.bdd);
  fsmMgr->stats.externalConstr = nc;
  
  if (nf > 0) {
    fsmMgr->fairness.bdd = Ddi_BddMakeConstAig(dd, 1);
    Ddi_BddSetPartConj(fsmMgr->fairness.bdd);
  }
  for (i = 0; i < nf; i++) {
    unsigned lit = mgr->fairness[i].lit;
    int i1 = i + 1;
    char *name = NULL;
    Ddi_Bdd_t *cBdd;

    cBdd = Ddi_BddMakeFromBaig(dd, nodesArray[lit / 2]);
    if (aiger_sign(lit))
      Ddi_BddNotAcc(cBdd);

    //    printf("CONSTR[%d]\n",i);
    //if (Ddi_BddSize(cBdd)>20) {
    //  logBddSupp(cBdd);
    //}
    //else {
    //  DdiLogBdd(cBdd,0);
    //}
    if (name == NULL) {
      sprintf(buf, "c%d", 2 * i1);
    }

    Ddi_BddPartInsertLast(fsmMgr->fairness.bdd, cBdd);
    //    fsmMgr->name.c[i] = Pdtutil_StrDup (name);
    Ddi_Free(cBdd);
  }

  //  Ddi_BddSetAig(fsmMgr->fairness.bdd);

  if (nj > 0) {
    fsmMgr->justice.bdd = Ddi_BddMakeConstAig(dd, 1);
    Ddi_BddSetPartConj(fsmMgr->justice.bdd);
  }
  for (i = 0; i < nj; i++) {
    unsigned lit = mgr->justice[i].lit;
    int i1 = i + 1;
    char *name = NULL;
    Ddi_Bdd_t *cBdd;

    cBdd = Ddi_BddMakeFromBaig(dd, nodesArray[lit / 2]);
    if (aiger_sign(lit))
      Ddi_BddNotAcc(cBdd);

    //    printf("CONSTR[%d]\n",i);
    //if (Ddi_BddSize(cBdd)>20) {
    //  logBddSupp(cBdd);
    //}
    //else {
    //  DdiLogBdd(cBdd,0);
    //}
    if (name == NULL) {
      sprintf(buf, "c%d", 2 * i1);
    }

    Ddi_BddPartInsertLast(fsmMgr->justice.bdd, cBdd);
    //    fsmMgr->name.c[i] = Pdtutil_StrDup (name);
    Ddi_Free(cBdd);
  }

  //  Ddi_BddSetAig(fsmMgr->justice.bdd);

  Pdtutil_Assert(m == (na + nl + ni), "error in aiger nums");
  for (i = 0; i <= m; i++) {
    bAig_RecursiveDeref(bmgr, nodesArray[i]);
  }
  Pdtutil_Free(nodesArray);

  /*
   *  return 0 = All OK
   */

#if 0
  if (Ddi_BddarraySize(Fsm_MgrReadDeltaBDD(fsmMgr)) > 20000) {
    int aol = Ddi_MgrReadAigAbcOptLevel(dd);

    Ddi_MgrSetOption(dd, Pdt_DdiAigAbcOpt_c, inum, 2);

    Ddi_AigarrayAbcOptAcc(Fsm_MgrReadDeltaBDD(fsmMgr), 60.0);

    Ddi_MgrSetOption(dd, Pdt_DdiAigAbcOpt_c, inum, aol);
  }
#endif

  aiger_reset(mgr);

  fsmTime = util_cpu_time() - fsmTime;
  fsmMgr->fsmTime += fsmTime;

  fsmMgrP = fsmMgr;

  return (0);
}


/**Function********************************************************************

  Synopsis           []

  Description        []

  SideEffects        []

  SeeAlso            [get_name_file,readFsm]

******************************************************************************/

int
Fsm_AigsimCex(
  char *aigerfile,
  char *cexfile
)
{
  int ret;
  int argc = 7;
  char *argv[10];

  argv[0] = "aigsimPdt";
  argv[1] = "-c";
  argv[2] = "-m";
  argv[3] = "-w";
  argv[4] = "-2";
  argv[5] = aigerfile;
  argv[6] = cexfile;
  argv[7] = NULL;

  ret = aigsim(argc, argv);

  return ret;

}


static const char *
on(
  unsigned i
)
{
  assert(mgr && i < mgr->num_outputs);
  if (mgr->outputs[i].name)
    return mgr->outputs[i].name;

  sprintf(buffer, "o%u", i);

  return buffer;
}

static void
ps(
  const char *str
)
{
  fputs(str, file);
}

static const char *
pl(
  unsigned lit
)
{
  const char *name;

  if (lit == 0)
    putc('0', file);
  else if (lit == 1)
    putc('1', file);
  else if ((lit & 1))
    //putc ('!', file), pl (lit - 1);
    ;
  else if ((name = aiger_get_symbol(mgr, lit))) {
    fputs(name, file);
  } else {
    if (aiger_is_input(mgr, lit)) {
      sprintf(buffer, "I%d\0", lit);
    } else if (aiger_is_latch(mgr, lit))
      sprintf(buffer, "L%d\0", lit);
    else {
      assert(aiger_is_and(mgr, lit));
      sprintf(buffer, "A%d\0", lit);
    }
  }
  return buffer;
}

static int
count_ch_prefix(
  const char *str,
  char ch
)
{
  const char *p;

  assert(ch);
  for (p = str; *p == ch; p++) ;

  if (*p && !isdigit(*p))
    return 0;

  return p - str;
}

static void
setupcount(
  void
)
{
  const char *symbol;
  unsigned i;
  int tmp;

  count = 0;
  for (i = 1; i <= mgr->maxvar; i++) {
    symbol = aiger_get_symbol(mgr, 2 * i);
    if (!symbol)
      continue;

    if ((tmp = count_ch_prefix(symbol, 'i')) > count)
      count = tmp;

    if ((tmp = count_ch_prefix(symbol, 'l')) > count)
      count = tmp;

    if ((tmp = count_ch_prefix(symbol, 'o')) > count)
      count = tmp;

    if ((tmp = count_ch_prefix(symbol, 'a')) > count)
      count = tmp;
  }
}

LoadInputs(FsmPortBnetNetwork_t * net, FsmPortBnetNode_t ** plastnode)
{
  char letter[20];
  char **listA;
  int i;
  FsmPortBnetNode_t *newnode;
  FsmPortBnetNode_t *lastnode = NULL;


  net->npis = mgr->num_inputs;
  listA = Pdtutil_Alloc(char *,
    mgr->num_inputs
  );

  memset((char *)listA, 0, sizeof(mgr->num_inputs));
  for (i = 0; i < mgr->num_inputs; i++) {
    strcpy(letter, pl(mgr->inputs[i].lit));
    listA[i] = strdup(letter);
    //listA[i] = (char *)malloc((strlen(input)+1)*sizeof(char));
    //strcpy(listA[i], input);
  }
  if (listA == NULL)
    goto failure;
  if (net->ninputs) {
    net->inputs = Pdtutil_Realloc(char *,
      net->inputs,
       (net->ninputs + mgr->num_inputs) * sizeof(char *)
    );

    for (i = 0; i < mgr->num_inputs; i++)
      net->inputs[net->ninputs + i] = listA[i];
  } else
    net->inputs = listA;
  /* Create a node for each primary input. */
  for (i = 0; i < mgr->num_inputs; i++) {
    newnode = Pdtutil_Alloc(FsmPortBnetNode_t, 1);
    memset((char *)newnode, 0, sizeof(FsmPortBnetNode_t));
    if (newnode == NULL)
      goto failure;
    newnode->name = listA[i];
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
  net->ninputs = mgr->num_inputs;
  *plastnode = lastnode;
  return 1;
failure:
  return NULL;
}

int
LoadLatches(
  FsmPortBnetNetwork_t * net,
  int *const0,
  int *const1,
  unsigned *count_latch,
  int **platch_helper,
  FsmPortBnetNode_t ** plastnode
)
{
  char letter[20];
  char **listA;
  int *latch_helper;
  FsmPortBnetNode_t *newnode;
  FsmPortBnetNode_t *lastnode;
  int require_const0 = 0;
  int require_const1 = 0;
  char let[5];
  char ***latches = NULL;
  int maxlatches = 0;
  unsigned i, j, latch_helper_cnt;
  char *savestring;

  latch_helper_cnt = 0;

  lastnode = *plastnode;
  net->nlatches = mgr->num_latches;
  latch_helper = calloc(mgr->num_latches, sizeof(latch_helper[0]));
  for (i = 0; i < mgr->num_latches; i++) {
    newnode = Pdtutil_Alloc(FsmPortBnetNode_t, 1);
    if (newnode == NULL)
      goto failure;
    memset((char *)newnode, 0, sizeof(FsmPortBnetNode_t));
    newnode->type = PORT_BNETPRESENT_STATE_NODE;
    listA = Pdtutil_Alloc(char *,
      3
    );

    memset((char *)listA, 0, 3);
    latch_helper[i] = 0;
    if (mgr->latches[i].next == aiger_false)
      *const0 = 1;
    if (mgr->latches[i].next == aiger_true)
      *const1 = 1;

/*  this case normally makes no sense, but you never know ... */
    if (mgr->latches[i].next == aiger_false ||
      mgr->latches[i].next == aiger_true) {
      if (mgr->latches[i].next == aiger_false) {
        sprintf(letter, "%s", "C0");
        listA[0] = strdup(letter);
      } else {
        sprintf(letter, "%s", "C1");
        listA[0] = strdup(letter);
      }
      strcpy(letter, pl(mgr->latches[i].lit));
      listA[1] = strdup(letter);
      listA[2] = "0";
    }

/* this should be the general case! */
    else {
      if (!aiger_sign(mgr->latches[i].next)) {
        strcpy(letter, pl(mgr->latches[i].next));
        listA[0] = strdup(letter);
        strcpy(letter, pl(mgr->latches[i].lit));
        listA[1] = strdup(letter);
        listA[2] = "0";
        if (listA == NULL)
          goto failure;
      } else {
        /* add prefix 'n' to inverted AIG nodes.
         * corresponding inverters are inserted below!
         */
        sprintf(letter, "%c%s", 'N', pl(aiger_strip(mgr->latches[i].next)));
        listA[0] = strdup(letter);
        strcpy(letter, pl(mgr->latches[i].lit));
        listA[1] = strdup(letter);
        listA[2] = "0";
        if (listA == NULL)
          goto failure;
        int already_done = 0;

        for (j = 0; j < latch_helper_cnt && !already_done; j++) {
          if (mgr->latches[latch_helper[j]].next == mgr->latches[i].next)
            already_done = 1;

        }
        if (!already_done) {
          latch_helper[latch_helper_cnt] = i;
          latch_helper_cnt++;
        }
      }
    }
    newnode->name = listA[1];
    newnode->inputs = listA;
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
    } else if (maxlatches <= i) {
      maxlatches += 20;
      latches = Pdtutil_Realloc(char **,
        latches,
        maxlatches
      );
    }
    latches[i] = listA;
  }
  net->latches = latches;
  *count_latch = latch_helper_cnt;
  *platch_helper = latch_helper;
  *plastnode = lastnode;
  return 1;
failure:
  return 0;
}

int
LoadNamesAnd(
  FsmPortBnetNetwork_t * net,
  FsmPortBnetNode_t ** plastnode
)
{
  FsmPortBnetNode_t *newnode;
  FsmPortBnetTabline_t *newline;
  FsmPortBnetTabline_t *lastline;
  FsmPortBnetNode_t *lastnode;
  char letter[10];
  char **listA;
  int i, j, k;

  lastnode = *plastnode;
  for (i = 0; i < mgr->num_ands; i++)
    //newnode->polarity = 0;
  {
    aiger_and *n = mgr->ands + i;
    unsigned rhs0 = n->rhs0;
    unsigned rhs1 = n->rhs1;

    newnode = Pdtutil_Alloc(FsmPortBnetNode_t, 1);
    memset((char *)newnode, 0, sizeof(FsmPortBnetNode_t));
    listA = Pdtutil_Alloc(char *,
      3
    );

    memset((char *)listA, 0, 3);
    if (newnode == NULL)
      goto failure;
    strcpy(letter, pl(aiger_strip(rhs0)));
    listA[0] = strdup(letter);
    strcpy(letter, pl(aiger_strip(rhs1)));
    listA[1] = strdup(letter);
    strcpy(letter, pl(aiger_strip(n->lhs)));
    listA[2] = strdup(letter);
    if (listA == NULL)
      goto failure;

    newnode->name = listA[2];
    newnode->inputs = listA;
    newnode->ninp = 2;
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
      for (j = 0; j < net->noutputs; j++) {
        if (strcmp(net->outputs[j], newnode->name) == 0) {
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

    lastline = NULL;
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
      if (aiger_sign(rhs0) && aiger_sign(rhs1))
        newline->values = "00";
      if (aiger_sign(rhs0) && !aiger_sign(rhs1))
        newline->values = "01";
      if (!aiger_sign(rhs0) && aiger_sign(rhs1))
        newline->values = "10";
      if (!aiger_sign(rhs0) && !aiger_sign(rhs1))
        newline->values = "11";
    } else
      newline->values = NULL;

  }
  *plastnode = lastnode;
  return 1;
failure:
  return 0;
}

int
LoadNamesOut(
  FsmPortBnetNetwork_t * net,
  int *const0,
  int *const1,
  FsmPortBnetNode_t ** plastnode
)
{
  FsmPortBnetNode_t *newnode;
  FsmPortBnetTabline_t *newline;
  FsmPortBnetTabline_t *lastline;
  FsmPortBnetNode_t *lastnode;
  char buffer[10];
  char **listA;
  int i, j;
  int require_const0;
  int require_const1;

  require_const0 = 0;
  require_const1 = 0;

  lastnode = *plastnode;
  for (i = 0; i < mgr->num_outputs; i++) {
    newnode = Pdtutil_Alloc(FsmPortBnetNode_t, 1);
    memset((char *)newnode, 0, sizeof(FsmPortBnetNode_t));
    listA = Pdtutil_Alloc(char *,
      3
    );

    memset((char *)listA, 0, 3);
    if (newnode == NULL)
      goto failure;
    if (mgr->outputs[i].lit == aiger_false ||
      mgr->outputs[i].lit == aiger_true) {
      if (mgr->outputs[i].lit == aiger_false) {
        sprintf(buffer, "%s", "C0");
        listA[0] = strdup(buffer);
      } else {
        sprintf(buffer, "%s", "C1");
        listA[0] = strdup(buffer);
      }
      if (mgr->outputs[i].name)
        listA[1] = mgr->outputs[i].name;
      else {
        sprintf(buffer, "O%d", i);
        listA[1] = strdup(buffer);
      }
      (mgr->outputs[i].lit == aiger_false) ? (require_const0 =
        1) : (require_const1 = 1);
      *const0 = require_const0;
      *const1 = require_const1;
    } else {
      strcpy(buffer, pl(aiger_strip(mgr->outputs[i].lit)));
      listA[0] = strdup(buffer);
      sprintf(buffer, "O%d", i);
      listA[1] = strdup(buffer);
    }

    if (listA == NULL)
      goto failure;
    newnode->name = listA[1];
    newnode->inputs = listA;
    newnode->ninp = 1;
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
      for (j = 0; j < net->noutputs; j++) {
        if (strcmp(net->outputs[j], newnode->name) == 0) {
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

    lastline = NULL;
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
    if (mgr->outputs[i].lit == aiger_false)
      newline->values = "1";
    else if (aiger_sign(mgr->outputs[i].lit))
      newline->values = "0";
    else
      newline->values = "1";
    /*   else */
/*        newline->values = NULL; */

  }
  *plastnode = lastnode;
  return 1;
failure:
  return 0;
}

int
RequireConsts(
  FsmPortBnetNetwork_t * net,
  int const0,
  int const1,
  FsmPortBnetNode_t ** plastnode
)
{
  FsmPortBnetNode_t *newnode;
  FsmPortBnetTabline_t *newline;
  FsmPortBnetTabline_t *lastline;
  FsmPortBnetNode_t *lastnode;
  char buffer[10];
  char **listA;
  int i, j;

  lastnode = *plastnode;
  newnode = Pdtutil_Alloc(FsmPortBnetNode_t, 1);
  memset((char *)newnode, 0, sizeof(FsmPortBnetNode_t));
  listA = Pdtutil_Alloc(char *,
    2
  );

  memset((char *)listA, 0, 2);
  if (const0 == 1) {
    sprintf(buffer, "%s", "C0");
    listA[0] = strdup(buffer);
  }
  if (const1 == 1) {
    sprintf(buffer, "%s", "C1");
    listA[0] = strdup(buffer);
  }

  if (const0 == 0 && const1 == 0) {
    free(listA);
    free(newnode);
    return 1;
  }
  if (newnode == NULL)
    goto failure;
  if (listA == NULL)
    goto failure;
  newnode->name = listA[0];
  newnode->inputs = listA;
  newnode->ninp = 0;
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
    for (j = 0; j < net->noutputs; j++) {
      if (strcmp(net->outputs[j], newnode->name) == 0) {
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

  lastline = NULL;
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
  newline->values = NULL;
  if (const0 == 1)
    newnode->polarity = 1;
  *plastnode = lastnode;
  return 1;
failure:
  return 0;
}

int
LoadNamesLatches(
  FsmPortBnetNetwork_t * net,
  unsigned latch_helper_cnt_or,
  int *latch_helper,
  FsmPortBnetNode_t ** plastnode
)
{
  FsmPortBnetNode_t *newnode;
  FsmPortBnetTabline_t *newline;
  FsmPortBnetTabline_t *lastline;
  FsmPortBnetNode_t *lastnode;
  char buffer[10];
  char **listA;
  int i, j, k;

  lastnode = *plastnode;
  for (i = 0; i < latch_helper_cnt_or; i++) {
    newnode = Pdtutil_Alloc(FsmPortBnetNode_t, 1);
    memset((char *)newnode, 0, sizeof(FsmPortBnetNode_t));
    listA = Pdtutil_Alloc(char *,
      2
    );

    memset((char *)listA, 0, 2);
    if (newnode == NULL)
      goto failure;
    unsigned l = latch_helper[i];

    if (mgr->latches[l].next != aiger_false &&
      mgr->latches[l].next != aiger_true) {
      assert(aiger_sign(mgr->latches[l].next));
      strcpy(buffer, pl(aiger_strip(mgr->latches[l].next)));
      listA[0] = strdup(buffer);
      sprintf(buffer, "%c%s", 'N', pl(aiger_strip(mgr->latches[l].next)));
      listA[1] = strdup(buffer);
    }
    if (listA == NULL)
      goto failure;
    newnode->name = listA[1];
    newnode->inputs = listA;
    newnode->ninp = 1;
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
      for (j = 0; j < net->noutputs; j++) {
        if (strcmp(net->outputs[j], newnode->name) == 0) {
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

    lastline = NULL;
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
    if (mgr->latches[l].next != aiger_false &&
      mgr->latches[l].next != aiger_true) {
      newline->values = "0";
    } else
      newline->values = NULL;

  }
  *plastnode = lastnode;
  free(latch_helper);
  return 1;
failure:
  return 0;
}

static void
print_LuzList(
  char **list /* list of pointers to strings */ ,
  int n                         /* length of the list */
)
{
  int i;

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelDevMed_c, for (i = 0; i < n; i++) {
    fprintf(stdout, "%s", list[i]);}
    fprintf(stdout, "\n");) ;

  return;
}                               /* end of printList */

static void
Bnet_LuzPrintNetwork(
  FsmPortBnetNetwork_t * net    /* boolean network */
)
{                               /*  */
  FsmPortBnetNode_t *nd;
  FsmPortBnetTabline_t *tl;
  int i;

  if (net == NULL)
    return;

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, ".model %s.\n", net->name);
    fprintf(stdout, ".inputs");
    print_LuzList(net->inputs, net->npis);
    fprintf(stdout, ".outputs");
    print_LuzList(net->outputs, net->npos);
    for (i = 0; i < net->nlatches; i++) {
    fprintf(stdout, ".latch"); print_LuzList(net->latches[i], 3);}
  ) ;
  nd = net->nodes;
  while (nd != NULL) {
    if (nd->type != PORT_BNETINPUT_NODE
      && nd->type != PORT_BNETPRESENT_STATE_NODE) {
      //Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, ".names");
      for (i = 0; i < nd->ninp; i++) {
        fprintf(stdout, "%s", nd->inputs[i]);
      }
      fprintf(stdout, "%s\n", nd->name);
        //) ;
        tl = nd->f;
      while (tl != NULL) {
        //Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelNone_c, 
        if (tl->values != NULL) {
          fprintf(stdout, "%s %d\n", tl->values, 1 - nd->polarity);
        } else {
          fprintf(stdout, "%d\n", 1 - nd->polarity);
        }
        //) ;
        tl = tl->next;
      }
    }
    nd = nd->next;
  }

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, ".end\n")
    );
}                               /* end of Bnet_PrintNetwork */


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
FsmPortBnetNetwork_t *
Fsm_ReadAigFile(
  FILE * fp /* Pointer to the AIG file */ ,
  char *fileFsmName             /* Pointer to the name file */
)
{
  unsigned i, j, latch_helper_cnt;
  const char *src, *dst, *error;
  char **listA;
  int *platch_helper = 0;
  int res, strip, ag, count = 0;
  int require_const0;
  int require_const1;

  require_const0 = 0;
  require_const1 = 0;
  latch_helper_cnt = 0;
  src = dst = 0;
  strip = 0;
  res = 0;
  ag = 0;
  FsmPortBnetNetwork_t *net;
  FsmPortBnetNode_t *newnode;
  FsmPortBnetNode_t *plastnode;
  FsmPortBnetTabline_t *newline;
  FsmPortBnetTabline_t *lastline;
  char input[10];
  char let[5];
  int maxlatches = 0, const0 = 0, const1 = 0;
  char ***latches = NULL;
  int exdc = 0;
  unsigned cnt, count_latch;
  int level;


  /* Allocate network object and initialize symbol table. *\ */
  net = Pdtutil_Alloc(FsmPortBnetNetwork_t, 1);
  if (net == NULL)
    goto failure;
  memset((char *)net, 0, sizeof(FsmPortBnetNetwork_t));
  net->hash = st_init_table(strcmp, st_strhash);
  if (net->hash == NULL)
    goto failure;
  /* The manager is initializated */
  mgr = aiger_init();
  error = aiger_open_and_read_from_file(mgr, fileFsmName);
  setupcount();
  /* name */
  net->name = Pdtutil_Alloc(char,
    strlen(fileFsmName) + 1
  );

  strcpy(net->name, fileFsmName);
  /* inputs */
  LoadInputs(net, &plastnode);
  /* outputs */
  net->npos = mgr->num_outputs;
  listA = Pdtutil_Alloc(char *,
    mgr->num_outputs
  );

  memset((char *)listA, 0, mgr->num_outputs);
  for (i = 0; i < mgr->num_outputs; ++i) {
    if (mgr->outputs[i].name) {
      listA[i] = strdup(mgr->outputs[i].name);
    } else {
      sprintf(input, "O%d", i);
      listA[i] = strdup(input);
      if (listA == NULL)
        goto failure;
    }
  }
  net->outputs = listA;
  net->noutputs = mgr->num_outputs;

  /*Wire_load_slope */
  net->slope = NULL;
/* Latches */
  LoadLatches(net, &const0, &const1, &count_latch, &platch_helper, &plastnode);

/*  AND   */
  LoadNamesAnd(net, &plastnode);
  LoadNamesOut(net, &const0, &const1, &plastnode);
  require_const0 = const0;
  require_const1 = const1;
  cnt = count_latch;
  LoadNamesLatches(net, cnt, platch_helper, &plastnode);
  RequireConsts(net, require_const0, require_const1, &plastnode);

/* Put nodes in symbol table. */

  newnode = net->nodes;
  while (newnode != NULL) {
    if (st_insert(net->hash, newnode->name, (void *)newnode) == ST_OUT_OF_MEM) {
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
      if (!st_lookup(net->hash, newnode->inputs[i], (void *)&auxnd)) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "%s not driven.\n", newnode->inputs[i])
          );
        goto failure;
      }
      auxnd->nfo++;
    }
    newnode = newnode->next;
  }

  net->noutputs = net->noutputs + net->nlatches;
  net->ninputs = net->ninputs + net->nlatches;
  if (!fsmPortBnetSetLevel(net))
    goto failure;
  level = fsmPortBnetSetLevel(net);
  aiger_reset(mgr);
  return (net);
failure:
  return (NULL);
}



int
Fsm_FsmWriteAiger(
  Fsm_Mgr_t * fsmMgr,
  char *fileAigName
)
{
  aiger *mgr = Fsm_FsmCopyToAiger(fsmMgr);

  if (!aiger_open_and_write_to_file(mgr, fileAigName))
    return (1);

  aiger_reset(mgr);

  return (0);
}

aiger *
Fsm_FsmCopyToAiger(
  Fsm_Mgr_t * fsmMgr
)
{
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);
  bAig_Manager_t *bmgr = Ddi_MgrReadMgrBaig(ddm);
  int i;
  int m, ni, nl, no, na, nb, nc, ntotv;
  Ddi_Vararray_t *vars;
  Ddi_Var_t *v;
  int *auxids, flagError;
  char **names;
  char *nameext;
  long fsmTime;

  aiger *mgr;
  const char *error;
  Ddi_Bddarray_t *nodesArray;
  Ddi_Bdd_t *litBdd;
  char buf[100];
  int sortDfs = 1;
  Ddi_Bdd_t *bddZero = NULL;
  bAig_array_t *visitedNodes;
  Ddi_Bddarray_t *fA;
  Ddi_Bdd_t *f;

  fsmTime = util_cpu_time();
  if (fsmMgr == NULL) {
    fprintf(stderr, "Error FSM==NULL %s.\n", error);
    return 1;
  }
  mgr = aiger_init();
  //mgr->maxvar=;
  ni = Fsm_MgrReadNI(fsmMgr);
  nl = Fsm_MgrReadNL(fsmMgr);
  no = Fsm_MgrReadNO(fsmMgr);
  //mgr->num_bad=;
  //mgr->num_ands=;
  //mgr->num_constraints=;


  // build aig nodes array with proper node numbering
  visitedNodes = bAigArrayAlloc();

  fA = Fsm_MgrReadDeltaBDD(fsmMgr);
  for (i = 0; i < Ddi_BddarrayNum(fA); i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA, i);

    Ddi_PostOrderAigVisitIntern(bmgr, Ddi_BddToBaig(f_i), visitedNodes, -1);
  }
  fA = Fsm_MgrReadLambdaBDD(fsmMgr);
  for (i = 0; i < Ddi_BddarrayNum(fA); i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA, i);

    Ddi_PostOrderAigVisitIntern(bmgr, Ddi_BddToBaig(f_i), visitedNodes, -1);
  }
  f = Fsm_MgrReadConstraintBDD(fsmMgr);
  if (f != NULL) {
    Ddi_PostOrderAigVisitIntern(bmgr, Ddi_BddToBaig(f), visitedNodes, -1);
  }
  f = Fsm_MgrReadInvarspecBDD(fsmMgr);
  if (f != NULL) {
    Ddi_PostOrderAigVisitIntern(bmgr, Ddi_BddToBaig(f), visitedNodes, -1);
  }

  Ddi_PostOrderAigClearVisitedIntern(bmgr, visitedNodes);


  for (i = 0; i < ni; i++) {
    int i1 = i + 1;
    Ddi_Var_t *var;
    char *s, *name;
    bAigEdge_t varIndex;
    unsigned lit = 2 * i1;

    var = Ddi_VararrayRead(fsmMgr->var.i, i);
    name = Ddi_VarName(var);
    if (name == NULL) {
      sprintf(buf, "i%d", lit);
      name = buf;
    }
    aiger_add_input(mgr, lit, name);
    varIndex = Ddi_VarToBaig(var);
    bAig_AuxInt(bmgr, varIndex) = i1;

  }

  for (i = 0; i < nl; i++) {
    int i1 = i + 1;
    unsigned lit = 2 * (i1 + ni);
    Ddi_Var_t *var;
    char *s, *name;
    bAigEdge_t varIndex;

    var = Ddi_VararrayRead(fsmMgr->var.ps, i);
    name = Ddi_VarName(var);
    if (name == NULL) {
      sprintf(buf, "l%d", lit);
    } else {
      sprintf(buf, "%s", name);
    }
    name = buf;

    for (s = name; *s != '\0'; s++) {
      if (*s == '#' || *s == ':' || *s == '(' || *s == ')')
        *s = '_';
    }

    //add latches after visiting all nodes
    varIndex = Ddi_VarToBaig(var);
    bAig_AuxInt(bmgr, varIndex) = i1 + ni;

  }

  unsigned iAnd = ni + nl + 1;

  for (i = 0; i < visitedNodes->num; i++) {
    bAigEdge_t baig, right, left;
    unsigned rhs0, rhs1, lhs;

    baig = visitedNodes->nodes[i];
    if (!bAig_NodeIsConstant(baig) && !bAig_isVarNode(bmgr, baig)) {
      bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr, baig);
      bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(bmgr, baig);
      unsigned ir = bAig_AuxInt(bmgr, right);
      unsigned il = bAig_AuxInt(bmgr, left);

      bAig_AuxInt(bmgr, baig) = iAnd;
      lhs = iAnd * 2;
      rhs0 = bAig_NodeIsInverted(right) ? 2 * ir + 1 : 2 * ir;
      rhs1 = bAig_NodeIsInverted(left) ? 2 * il + 1 : 2 * il;
      aiger_add_and(mgr, lhs, rhs0, rhs1);
      iAnd++;
    }
  }

  f = Fsm_MgrReadConstraintBDD(fsmMgr);
  if (f != NULL) {
    Ddi_PostOrderAigVisitIntern(bmgr, Ddi_BddToBaig(f), visitedNodes, -1);
  }
  f = Fsm_MgrReadInvarspecBDD(fsmMgr);
  if (f != NULL) {
    Ddi_PostOrderAigVisitIntern(bmgr, Ddi_BddToBaig(f), visitedNodes, -1);
  }

  unsigned reverse = 1;         //1 or 0 ????

  if (reverse) {
    fA = Fsm_MgrReadLambdaBDD(fsmMgr);
    for (i = 0; i < Ddi_BddarrayNum(fA); i++) {
      Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA, i);
      bAigEdge_t baig = Ddi_BddToBaig(f_i);
      unsigned lit = bAig_AuxInt(bmgr, baig);

      lit *= 2;
      if (bAig_NodeIsInverted(baig))
        lit++;
      aiger_add_bad(mgr, lit, Ddi_ReadName(f_i));
      //      aiger_add_output (mgr, lit, Ddi_ReadName (f_i));
    }
  } else {
    fA = Fsm_MgrReadLambdaBDD(fsmMgr);
    for (i = 0; i < Ddi_BddarrayNum(fA); i++) {
      Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA, i);
      bAigEdge_t baig = Ddi_BddToBaig(f_i);
      unsigned lit = bAig_AuxInt(bmgr, baig);

      lit *= 2;
      if (bAig_NodeIsInverted(baig))
        lit++;
      aiger_add_output(mgr, lit, Ddi_ReadName(f_i));
    }

    // if (src->num_bad && src->num_constraints) {

    fA = Fsm_MgrReadDeltaBDD(fsmMgr);
    for (i = 0; i < Ddi_BddarrayNum(fA); i++) {
      Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA, i);
      int isConstant = Ddi_BddIsConstant(f_i);
      bAigEdge_t baig = Ddi_BddToBaig(f_i);
      Ddi_Var_t *var = Ddi_VararrayRead(fsmMgr->var.ps, i);
      bAigEdge_t varIndex = Ddi_VarToBaig(var);
      int litPres = bAig_AuxInt(bmgr, varIndex);
      int litNext = bAig_AuxInt(bmgr, baig);
      int reset = 0;
      char *name;

      if (isConstant) {
        litNext = 0;
      }
      litPres *= 2;
      litNext *= 2;
      if (bAig_NodeIsInverted(baig))
        litNext++;

      name = Ddi_VarName(var);
#if 0
      if (strncmp(name, "PDT_", 4) == 0) {
        int iter = 0;

        sprintf(buf, "PDTVAR_%s", name);
        name = buf;
        while (Ddi_VarFromName(ddm, name) != NULL) {
          sprintf(buf, "PDTVAR%d_%s", iter, Ddi_VarName(var));
          iter++;
        }
      }
#endif
      aiger_add_latch(mgr, litPres, litNext, name);
      aiger_add_reset(mgr, litPres, reset);
    }
  }

  Ddi_PostOrderAigClearVisitedIntern(bmgr, visitedNodes);
  //free visited nodes
  bAigArrayFree(visitedNodes);

  return mgr;
}

int
Fsm_FsmMiniWriteAiger(
  Fsm_Fsm_t * fsmFsm,
  char *fileAigName
)
{
  aiger *mgr;

  mgr = Fsm_FsmMiniCopyToAiger(fsmFsm);

  if (!aiger_open_and_write_to_file(mgr, fileAigName))
    return (1);

  aiger_reset(mgr);

  return (0);
}

aiger *
Fsm_FsmMiniCopyToAiger(
  Fsm_Fsm_t * fsmFsm
)
{

  Fsm_Mgr_t *fsmMgr = Fsm_FsmReadMgr(fsmFsm);
  Ddi_Mgr_t *ddm = Fsm_MgrReadDdManager(fsmMgr);
  bAig_Manager_t *bmgr = Ddi_MgrReadMgrBaig(ddm);
  int i;
  int m, ni, nl, no, na, nb, nc, ntotv;
  Ddi_Vararray_t *vars;
  Ddi_Var_t *v;
  int *auxids, flagError;
  char **names;
  char *nameext;
  long fsmTime;

  aiger *mgr;
  const char *error;
  Ddi_Bddarray_t *nodesArray;
  Ddi_Bdd_t *litBdd;
  char buf[500];
  int sortDfs = 1;
  Ddi_Bdd_t *bddZero = NULL;
  bAig_array_t *visitedNodes;
  Ddi_Bddarray_t *fA, *myInitStub;
  Ddi_Bdd_t *f;
  int complOutput = 1;

  fsmTime = util_cpu_time();
  if (fsmFsm == NULL) {
    fprintf(stderr, "Error FSM==NULL %s.\n", error);
    return 1;
  }
  mgr = aiger_init();
  //mgr->maxvar=;

  ni = Ddi_VararrayNum(Fsm_FsmReadPI(fsmFsm));
  nl = Ddi_VararrayNum(Fsm_FsmReadPS(fsmFsm));
  no = Fsm_FsmReadNO(fsmFsm);

  // build aig nodes array with proper node numbering
  visitedNodes = bAigArrayAlloc();

  fA = Fsm_FsmReadDelta(fsmFsm);
  for (i = 0; i < Ddi_BddarrayNum(fA); i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA, i);

    Ddi_PostOrderAigVisitIntern(bmgr, Ddi_BddToBaig(f_i), visitedNodes, -1);
  }
  fA = Fsm_FsmReadLambda(fsmFsm);
  for (i = 0; i < Ddi_BddarrayNum(fA); i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA, i);

    Ddi_PostOrderAigVisitIntern(bmgr, Ddi_BddToBaig(f_i), visitedNodes, -1);
  }
  f = Fsm_FsmReadConstraint(fsmFsm);
  if (f != NULL) {
    Ddi_PostOrderBddAigVisitIntern(f, visitedNodes, -1);
  }
  f = Fsm_FsmReadInvarspec(fsmFsm);
  if (f != NULL) {
    Ddi_PostOrderAigVisitIntern(bmgr, Ddi_BddToBaig(f), visitedNodes, -1);
  }

  Ddi_PostOrderAigClearVisitedIntern(bmgr, visitedNodes);


  for (i = 0; i < ni; i++) {
    int index, i1 = i + 1;
    Ddi_Var_t *var;
    char *s, *name;
    bAigEdge_t varIndex;
    unsigned lit = 2 * i1;

    var = Ddi_VararrayRead(Fsm_FsmReadPI(fsmFsm), i);
    index = Ddi_VarIndex(var);
    name = Ddi_VarName(var);
    if (name == NULL) {
      sprintf(buf, "i%d", lit);
      name = buf;
    }
#if 0
    if (strncmp(name, "PDT_", 4) == 0) {
      int iter = 0;

      sprintf(buf, "PDTVAR_%s", name);
      name = buf;
      while (Ddi_VarFromName(ddm, name) != NULL) {
        sprintf(buf, "PDTVAR%d_%s", iter, Ddi_VarName(var));
        iter++;
      }
    }
    if (strncmp(name, "PDTRAV_MONOTONE_", 16) == 0) {
      int iter = 0;

      sprintf(buf, "PDTVAR_MON_%s", name + 16);
      name = buf;
      while (Ddi_VarFromName(ddm, name) != NULL) {
        sprintf(buf, "PDTVAR%d_%s", iter, Ddi_VarName(var));
        iter++;
      }
    }
#endif
    aiger_add_input(mgr, lit, name);
    varIndex = Ddi_VarToBaig(var);
    bAig_AuxInt(bmgr, varIndex) = i1;

  }

  for (i = 0; i < nl; i++) {
    int index, i1 = i + 1;
    unsigned lit = 2 * (i1 + ni);
    Ddi_Var_t *var;
    char *s, *name;
    bAigEdge_t varIndex;

    var = Ddi_VararrayRead(Fsm_FsmReadPS(fsmFsm), i);
    name = Ddi_VarName(var);
    if (name == NULL) {
      sprintf(buf, "l%d", lit);
    } else {
      sprintf(buf, "%s", name);
    }
    name = buf;

    for (s = name; *s != '\0'; s++) {
      if (*s == '#' || *s == ':' || *s == '(' || *s == ')')
        *s = '_';
    }

    //add latches after visiting all nodes
    varIndex = Ddi_VarToBaig(var);
    bAig_AuxInt(bmgr, varIndex) = i1 + ni;
  }

  unsigned iAnd = ni + nl + 1;

  for (i = 0; i < visitedNodes->num; i++) {
    bAigEdge_t baig, right, left;
    unsigned rhs0, rhs1, lhs;

    baig = visitedNodes->nodes[i];
    if (!bAig_NodeIsConstant(baig) && !bAig_isVarNode(bmgr, baig)) {
      bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr, baig);
      bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(bmgr, baig);
      unsigned ir = bAig_AuxInt(bmgr, right);
      unsigned il = bAig_AuxInt(bmgr, left);

      Pdtutil_Assert(ir < 2 * iAnd, "wrong ir");
      Pdtutil_Assert(il < 2 * iAnd, "wrong il");
      bAig_AuxInt(bmgr, baig) = iAnd;
      lhs = iAnd * 2;
      rhs0 = bAig_NodeIsInverted(right) ? 2 * ir + 1 : 2 * ir;
      rhs1 = bAig_NodeIsInverted(left) ? 2 * il + 1 : 2 * il;
      aiger_add_and(mgr, lhs, rhs0, rhs1);
      iAnd++;
    }
  }

#if 0
  f = Fsm_FsmReadConstraint(fsmFsm);
  if (f != NULL) {
    Ddi_PostOrderAigVisitIntern(bmgr, Ddi_BddToBaig(f), visitedNodes, -1);
  }
  f = Fsm_FsmReadInvarspec(fsmFsm);
  if (f != NULL) {
    Ddi_PostOrderAigVisitIntern(bmgr, Ddi_BddToBaig(f), visitedNodes, -1);
  }
#endif

  unsigned oldFormat = 0;       //1 or 0 ????

  if (!oldFormat) {
    fA = Fsm_FsmReadLambda(fsmFsm);
    for (i = 0; i < Ddi_BddarrayNum(fA); i++) {
      Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA, i);
      bAigEdge_t baig = Ddi_BddToBaig(f_i);
      unsigned lit = bAig_AuxInt(bmgr, baig);

      if (Ddi_BddIsConstant(f_i)) {
	lit = Ddi_BddIsOne(f_i) ? 1 : 0;
      }
      else {
	lit *= 2;
	if (complOutput) {
	  if (!bAig_NodeIsInverted(baig))
	    lit++;
	} else {
	  if (bAig_NodeIsInverted(baig))
	    lit++;
	}
      }
      aiger_add_bad(mgr, lit, Ddi_ReadName(f_i));
    }
  } else {
    fA = Fsm_FsmReadLambda(fsmFsm);
    for (i = 0; i < Ddi_BddarrayNum(fA); i++) {
      Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA, i);
      bAigEdge_t baig = Ddi_BddToBaig(f_i);
      unsigned lit = bAig_AuxInt(bmgr, baig);

      if (Ddi_BddIsConstant(f_i)) {
	lit = Ddi_BddIsOne(f_i) ? 1 : 0;
      }
      else {
	lit *= 2;
	if (complOutput) {
	  if (!bAig_NodeIsInverted(baig))
	    lit++;
	} else {
	  if (bAig_NodeIsInverted(baig))
	    lit++;
	}
      }
      aiger_add_output(mgr, lit, Ddi_ReadName(f_i));
    }
  }
  // if (src->num_bad && src->num_constraints) {

  fA = Fsm_FsmReadDelta(fsmFsm);
  Pdtutil_Assert(fsmFsm->initStub == NULL, "init stub not supported in aiger");
  myInitStub = Ddi_BddarrayMakeLiteralsAig(fsmFsm->ps, 1);
  Ddi_AigarrayConstrainCubeAcc(myInitStub, fsmFsm->init);

  for (i = 0; i < Ddi_BddarrayNum(fA); i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA, i);
    int isConstant = Ddi_BddIsConstant(f_i);
    bAigEdge_t baig = Ddi_BddToBaig(f_i);
    Ddi_Var_t *var = Ddi_VararrayRead(Fsm_FsmReadPS(fsmFsm), i);
    bAigEdge_t varIndex = Ddi_VarToBaig(var);
    int litPres = bAig_AuxInt(bmgr, varIndex);
    int litNext = bAig_AuxInt(bmgr, baig);
    char *name;

    if (isConstant) {
      litNext = 0;
    }
    litPres *= 2;
    litNext *= 2;
    if (bAig_NodeIsInverted(baig))
      litNext++;

    name = Ddi_VarName(var);
#if 0
    if (strncmp(name, "PDT_", 4) == 0) {
      int iter = 0;

      sprintf(buf, "PDTVAR_%s", name);
      name = buf;
      while (Ddi_VarFromName(ddm, name) != NULL) {
        sprintf(buf, "PDTVAR%d_%s", iter, Ddi_VarName(var));
        iter++;
      }
    }
#endif
    aiger_add_latch(mgr, litPres, litNext, name);
    if (Ddi_BddIsOne(Ddi_BddarrayRead(myInitStub, i))) {
      aiger_add_reset(mgr, litPres, 1);
    } else if (Ddi_BddIsZero(Ddi_BddarrayRead(myInitStub, i))) {
      aiger_add_reset(mgr, litPres, 0);
    } else {
      Pdtutil_Warning(1, "latch with free initial value - check aiger reset");
    }
  }
  Ddi_Free(myInitStub);
  // Ddi_PostOrderAigClearVisitedIntern(bmgr,visitedNodes); 

  if (fsmFsm->constraint != NULL) {
    Ddi_Bdd_t *myC = Ddi_BddDup(fsmFsm->constraint);

    Ddi_BddSetPartConj(myC);
    for (i = 0; i < Ddi_BddPartNum(myC); i++) {
      Ddi_Bdd_t *constr_i = Ddi_BddPartRead(myC, i);

      if (Ddi_BddIsOne(constr_i))
        continue;
      bAigEdge_t baig = Ddi_BddToBaig(constr_i);
      unsigned lit = bAig_AuxInt(bmgr, baig);

      lit *= 2;
      if (bAig_NodeIsInverted(baig))
        lit++;
      if (oldFormat) {
        aiger_add_output(mgr, lit, Ddi_ReadName(fsmFsm->constraint));
      } else {
        aiger_add_constraint(mgr, lit, Ddi_ReadName(fsmFsm->constraint));
      }
    }
    Ddi_Free(myC);
  }
  //free visited nodes
  bAigArrayFree(visitedNodes);

  return mgr;
}

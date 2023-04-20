/**CFile***********************************************************************

  FileName    [ddiInterpGraph.c]

  PackageName [ddi]

  Synopsis    [Functions to generate the interpolant from SAT
    proofs.
    ]

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

#include "Proof.h"
#include "File.h"
#include "Sort.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/


#define DDI_INTERP_DEBUG 0
/* IFF SUB_SUPER_SET == 0 apply McMillan Subset Reduction
   IFF SUB_SUPER_SET == 1 apply the opposite, i.e., SuperSet Reductions */
#define SUB_SUPER_SET    0
/* IFF PERFORM_BCP == 1 perform BCP
                   == 0 do not perform BCP */
#define PERFORM_BCP 1

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

static int DdiInterpOpLComplex(ddiInterpGraphMgr_t *interpMgr, ddiInterpGraph_t *interpGraph, int n, int rv, int l, int r, int rvl, int ll, int lr, int rvr, int rl, int rr);
static int DdiInterpOpLLSimple(ddiInterpGraph_t *interpGraph, int n, int rv, int l, int r, int rvl, int ll, int lr, int rvr, int rl, int rr);
static int DdiInterpOpLRSimple(ddiInterpGraph_t *interpGraph, int n, int rv, int l, int r, int rvl, int ll, int lr, int rvr, int rl, int rr);
static int DdiInterpOpRComplex(ddiInterpGraphMgr_t *interpMgr, ddiInterpGraph_t *interpGraph, int n, int rv, int l, int r, int rvl, int ll, int lr, int rvr, int rl, int rr);
static int DdiInterpOpRLSimple(ddiInterpGraph_t *interpGraph, int n, int rv, int l, int r, int rvl, int ll, int lr, int rvr, int rl, int rr);
static int DdiInterpOpRRSimple(ddiInterpGraph_t *interpGraph, int n, int rv, int l, int r, int rvl, int ll, int lr, int rvr, int rl, int rr);
static int DdiInterpOpLComplexInv(ddiInterpGraphMgr_t *interpMgr, ddiInterpGraph_t *interpGraph, int n, int rv, int l, int r, int rvl, int ll, int lr, int rvr, int rl, int rr);
static int DdiInterpOpLLSimpleInv(ddiInterpGraph_t *interpGraph, int n, int rv, int l, int r, int rvl, int ll, int lr, int rvr, int rl, int rr);
static int DdiInterpOpLRSimpleInv(ddiInterpGraph_t *interpGraph, int n, int rv, int l, int r, int rvl, int ll, int lr, int rvr, int rl, int rr);
static int DdiInterpOpRComplexInv(ddiInterpGraphMgr_t *interpMgr, ddiInterpGraph_t *interpGraph, int n, int rv, int l, int r, int rvl, int ll, int lr, int rvr, int rl, int rr);
static int DdiInterpOpRLSimpleInv(ddiInterpGraph_t *interpGraph, int n, int rv, int l, int r, int rvl, int ll, int lr, int rvr, int rl, int rr);
static int DdiInterpOpRRSimpleInv(ddiInterpGraph_t *interpGraph, int n, int rv, int l, int r, int rvl, int ll, int lr, int rvr, int rl, int rr);
static bAigEdge_t DdiInterpToBaigRecur(ddiInterpGraphMgr_t *interpMgr, bAig_Manager_t *bMgr, int index);
static int DdiInterpCreateNode(ddiInterpGraphMgr_t *interpMgr);
static void DdiInterpPrintNode(ddiInterpGraph_t *interpGraph, int index);
static int DdiInterpResVarInClause(int index, ddiInterpGraph_t node);
static void DdiInterpResVarExchange(int indexOld, int indexNew, ddiInterpGraph_t node);
static void DdiInterpCreateClauseArray(ddiInterpGraph_t *interpGraph, int index);
static void DdiInterpCOISet(ddiInterpGraphMgr_t *interpMgr, int index, int *delNP);
static void DdiInterpCOIStats(ddiInterpGraphMgr_t *interpMgr, int index, int *constNP, int *varNP, int *clauseNP, int *andNP, int *orNP);
static int DdiInterpCOISetRecur(ddiInterpGraphMgr_t *interpMgr, int index, int *delNP, int *indexRemap);
static char * DdiInt2Str(int i);

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

ddiInterpGraphMgr_t *
DdiInterpMgrInit (
  int maxSize
  )
{
  ddiInterpGraphMgr_t *mgr = Pdtutil_Alloc (ddiInterpGraphMgr_t, 1);

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpMgrInit#");
  fflush (stdout);
#endif

  if (mgr == NIL (ddiInterpGraphMgr_t)){
    return (mgr);
  }
  if (maxSize >  interpArrayMaxSize ) {
    maxSize =  interpArrayMaxSize;
  }
  mgr->interpGraphSize = maxSize;
  mgr->interpGraph = Pdtutil_Alloc (ddiInterpGraph_t, maxSize);

  mgr->interpGraphRoot = (-1);

  /* Start From 2:
     0 unused
     0 reserved for the Zero Constant Node */
  mgr->interpGraphIndex = 1;

  /* Unused */
  mgr->interpGraph[0].op = Ddi_InterpGraph_Null;
  mgr->interpGraph[0].flag = DDI_INTERP_NULL_NODE_INDEX;
  mgr->interpGraph[0].fanout = DDI_INTERP_NULL_NODE_INDEX;
  mgr->interpGraph[0].name = Pdtutil_Alloc (char, 7 + 1);
  strcpy (mgr->interpGraph[0].name, "Nd_Null");
  mgr->interpGraph[0].resolvedVar = DDI_INTERP_NULL_NODE_INDEX;
  mgr->interpGraph[0].clauseArray = NULL;
  mgr->interpGraph[0].clauseArrayN = DDI_INTERP_NULL_NODE_INDEX;
  mgr->interpGraph[0].indexBaig = DDI_INTERP_NULL_NODE_INDEX;
  mgr->interpGraph[0].left = DDI_INTERP_NULL_NODE_INDEX;
  mgr->interpGraph[0].right = DDI_INTERP_NULL_NODE_INDEX;

  /* Zero Constant */
  mgr->interpGraph[1].op = Ddi_InterpGraph_Zero;
  mgr->interpGraph[1].flag = DDI_INTERP_NULL_NODE_INDEX;
  mgr->interpGraph[1].fanout = DDI_INTERP_NULL_NODE_INDEX;
  mgr->interpGraph[1].name = Pdtutil_Alloc (char, 7 + 1);
  strcpy (mgr->interpGraph[1].name, "Nd_Zero");
  mgr->interpGraph[1].resolvedVar = DDI_INTERP_NULL_NODE_INDEX;
  mgr->interpGraph[1].clauseArray = NULL;
  mgr->interpGraph[1].clauseArrayN = DDI_INTERP_NULL_NODE_INDEX;
  mgr->interpGraph[1].indexBaig = bAig_Zero; 
  mgr->interpGraph[1].left = DDI_INTERP_NULL_NODE_INDEX;
  mgr->interpGraph[1].right = DDI_INTERP_NULL_NODE_INDEX;

  return (mgr);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

void
DdiInterpMgrFree (
  ddiInterpGraphMgr_t *mgr
  )
{
  int i;

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpMgrFree#");
  fflush (stdout);
#endif

  for (i=2; i<=mgr->interpGraphIndex; i++) {
    Pdtutil_Free (mgr->interpGraph[i].name);
    Pdtutil_Free (mgr->interpGraph[i].clauseArray);
  }

  Pdtutil_Free (mgr->interpGraph[1].name);
  Pdtutil_Free (mgr->interpGraph[0].name);
  
  Pdtutil_Free (mgr->interpGraph);
  Pdtutil_Free (mgr);

  return;
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

void
DdiInterpMgrPrint (
  ddiInterpGraphMgr_t *interpMgr
  )
{
  int i, j;
  char str[10];

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpMgrPrint#");
  fflush (stdout);
#endif

  fprintf (stdout, "Alloc Size: %d\n", interpMgr->interpGraphSize);
  fprintf (stdout, "Root Index: %d\n", interpMgr->interpGraphRoot);
  fprintf (stdout, "Final Size: %d\n", interpMgr->interpGraphIndex);
  fprintf (stdout, "%s%s\n",
      "# Op Name Fanout indexBaig Left Right ",
      "ResolvedVar ClauseArraySize Vars");
  for (i=0; i<=interpMgr->interpGraphIndex; i++) {
    DdiInterpPrintNode (interpMgr->interpGraph, i);
  }

  return;
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

void
DdiInterpAllToBaig (
  ddiInterpGraphMgr_t *interpMgr,
  bAig_Manager_t *bMgr
  )
{
  int i;
  bAigEdge_t retBaig;

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpAllToBaig#");
  fflush (stdout);
#endif

  for (i=interpMgr->interpGraphIndex; i>DDI_INTERP_ZERO_NODE_INDEX; i--) {
    retBaig = DdiInterpToBaigRecur (interpMgr, bMgr, i);
  }

  return;
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

bAigEdge_t
DdiInterpToBaig (
  ddiInterpGraphMgr_t *interpMgr,
  bAig_Manager_t *bMgr,
  int index
  )
{
  bAigEdge_t retBaig;

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpToBaig#");
  fflush (stdout);
#endif

  retBaig = DdiInterpToBaigRecur (interpMgr, bMgr, index);

  return (retBaig);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

void
DdiInterpOpt (
  ddiInterpGraphMgr_t *interpMgr
  )
{
  int rootNew;
  int constN, varN, clauseN, andN, orN, delN;

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpOpt#");
  fflush (stdout);
#endif

  /* Get Statistics Before COI and BCP */
#if 0
  fprintf (stdout, "InterpGraph Before COI and BCP:\n");
  DdiInterpMgrPrint (interpMgr);
  fprintf (stdout, "InterpGraph Before END.\n");
#endif
  constN = varN = clauseN = andN = orN = delN = 0;
  DdiInterpCOIStats (interpMgr, DdiInterpGetRoot (interpMgr),
    &constN, &varN, &clauseN, &andN, &orN);
  DdiInterpMarkNodeCOI (interpMgr, DdiInterpGetRoot (interpMgr), 0);
  fprintf (stdout, "\n");
  fprintf (stdout,
    "COI Stats Before Reduction: #const(%d) #var(%d) #clause(%d) #and(%d) #or(%d)\n",
    constN, varN, clauseN, andN, orN);

  /* COI and BCP */
  DdiInterpCOISet (interpMgr, DdiInterpGetRoot (interpMgr), &delN);
  DdiInterpCOIGet (interpMgr);

  /* Get Statistics After COI and BCP */
  constN = varN = clauseN = andN = orN = 0;
  DdiInterpCOIStats (interpMgr, DdiInterpGetRoot (interpMgr),
    &constN, &varN, &clauseN, &andN, &orN);
  DdiInterpMarkNodeCOI (interpMgr, DdiInterpGetRoot (interpMgr), 0);
  fprintf (stdout, "(deleted Nodes %d)\n", delN);
  fprintf (stdout,
    "COI Stats After  Reduction: #const(%d) #var(%d) #clause(%d) #and(%d) #or(%d)\n",
    constN, varN, clauseN, andN, orN);
#if 0
  fprintf (stdout, "InterpGraph After COI and BCP:\n");
  DdiInterpMgrPrint (interpMgr);
  fprintf (stdout, "InterpGraph After END.\n");
#endif

  /* McMillan Sub OR Sup Set */
  if (SUB_SUPER_SET == 0) {
    DdiInterpSub (interpMgr, 1);
  } else {
    DdiInterpSuper (interpMgr, 5);
  }

#if 0
  fprintf (stdout, "InterpGraph After McMillan:\n");
  DdiInterpMgrPrint (interpMgr);
  fprintf (stdout, "InterpGraph After McMillan END.\n");
#endif

  return;
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

void
DdiInterpMarkNodeCOI (
  ddiInterpGraphMgr_t *interpMgr,
  int index,
  int flag
  )
{
  int leftInterp, rightInterp;

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpMarkNodeCOI#");
  fflush (stdout);
#endif

  if (index == DDI_INTERP_NULL_NODE_INDEX) {
    return;
  }

  if (interpMgr->interpGraph[index].flag == flag) {
    return;
  }

  interpMgr->interpGraph[index].flag = flag;

  leftInterp = DdiInterpRegular (interpMgr->interpGraph[index].left);
  DdiInterpMarkNodeCOI (interpMgr, leftInterp, flag);

  rightInterp = DdiInterpRegular (interpMgr->interpGraph[index].right);
  DdiInterpMarkNodeCOI (interpMgr, rightInterp, flag);

  return;
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

void
DdiInterpCOIGet (
  ddiInterpGraphMgr_t *interpMgr
  )
{
  int *indexRemap;
  int i, j, indexLeft, indexRight;

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpGetCOI#");
  fflush (stdout);
#endif

  indexRemap = Pdtutil_Alloc (int, (interpMgr->interpGraphIndex+1));
  if (indexRemap == NULL){
    Pdtutil_Warning (1, "Memory Allocation Error to Create the Remap Array.");
    exit (1);
  }

  /* Set Rimap Index */
  indexRemap[0] = 0;
  indexRemap[1] = 1;
  for (i=j=2; i<=interpMgr->interpGraphIndex; i++) {
    if (interpMgr->interpGraph[i].fanout > 0) {
      /* Map Position */
      indexRemap[i] = j;
      j++;
    } else {
      indexRemap[i] = (-1);
    }
  }

  /* Skip NULL and Zero Nodes */
  for (i=j=2; i<=interpMgr->interpGraphIndex; i++) {
    if (interpMgr->interpGraph[i].fanout > 0) {
      /* Map Position */
      interpMgr->interpGraph[j] = interpMgr->interpGraph[i];

      indexLeft = indexRemap[DdiInterpRegular (interpMgr->interpGraph[j].left)];
      indexRight = indexRemap[DdiInterpRegular (interpMgr->interpGraph[j].right)];
      Pdtutil_Assert (indexLeft!=(-1), "Wrong Remap Index.");
      Pdtutil_Assert (indexRight!=(-1), "Wrong Remap Index.");

      if (DdiInterpIsInverted (interpMgr->interpGraph[j].left)) {
        interpMgr->interpGraph[j].left = -indexLeft;
      } else {
        interpMgr->interpGraph[j].left = indexLeft;
      }

      if (DdiInterpIsInverted (interpMgr->interpGraph[j].right)) {
        interpMgr->interpGraph[j].right = -indexRight;
      } else {
        interpMgr->interpGraph[j].right = indexRight;
      }

      j++;
    } else {
      //printf ("#%d#", i);
      Pdtutil_Free (interpMgr->interpGraph[i].name);
      Pdtutil_Free (interpMgr->interpGraph[i].clauseArray);
    }
  }

  fprintf (stdout, "InterpReduction: From %d Nodes To %d Nodes ",
    interpMgr->interpGraphIndex, j-1);

  if (interpMgr->interpGraphRoot != (-1)) {
    interpMgr->interpGraphRoot = indexRemap[interpMgr->interpGraphRoot];
  }
  interpMgr->interpGraphIndex = j-1;

#if 0
  printf ("Index Remap:\n");
  for (i=0; i<=interpMgr->interpGraphIndex; i++) {
    printf ("{[%d-%d]}", i, indexRemap[i]);
  }
  printf ("Index Remap END.\n");
#endif

  Pdtutil_Free (indexRemap);

  return;
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

int
DdiInterpZero (
  ddiInterpGraphMgr_t *interpMgr
  )
{
#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpZero#");
  fflush (stdout);
#endif

  return (ddi_InterpGraph_Zero);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

int
DdiInterpOne (
  ddiInterpGraphMgr_t *interpMgr
  )
{
#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpOne#");
  fflush (stdout);
#endif

  return (Ddi_InterpGraph_One);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

int
DdiInterpVar (
  ddiInterpGraphMgr_t *interpMgr,
  int indexBaig
  )
{
  int index;

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpVar#");
  fflush (stdout);
#endif

  index = DdiInterpCreateNode (interpMgr);

  interpMgr->interpGraph[index].op = Ddi_InterpGraph_Var;
  interpMgr->interpGraph[index].name =
    Pdtutil_Alloc (char, 3 + 1);
  sprintf (interpMgr->interpGraph[index].name, "Var");
  interpMgr->interpGraph[index].indexBaig = indexBaig;

  return (index);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

int
DdiInterpClause (
  ddiInterpGraphMgr_t *interpMgr,
  int indexBaig,
  const vec<Lit>& c
  )
{
  int i, index;

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpClause#");
  fflush (stdout);
#endif

  index = DdiInterpCreateNode (interpMgr);

  interpMgr->interpGraph[index].op = Ddi_InterpGraph_Clause;
  interpMgr->interpGraph[index].name =
    Pdtutil_Alloc (char, 6 + 1);
  sprintf (interpMgr->interpGraph[index].name, "Clause");
  interpMgr->interpGraph[index].indexBaig = indexBaig;

  interpMgr->interpGraph[index].clauseArrayN = c.size();
  interpMgr->interpGraph[index].clauseArray =
    Pdtutil_Alloc (int, c.size());
  for (i=0; i<c.size(); i++) {
    interpMgr->interpGraph[index].clauseArray[i] = var(c[i]); 
    if (sign(c[i])) {
      interpMgr->interpGraph[index].clauseArray[i] =
	-(interpMgr->interpGraph[index].clauseArray[i]);
    }
  }

  return (index);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

void
DdiInterpRef (
  ddiInterpGraphMgr_t *interpMgr,
  int index
  )
{
#if DDI_INTERP_DEBUG
  fprintf (stdout, "#Ddi_InterpRef#");
  fflush (stdout);
#endif

  interpMgr->interpGraph[index].fanout++;

  return;
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

void
DdiInterpDeref (
  ddiInterpGraphMgr_t *interpMgr,
  int index
  )
{
#if DDI_INTERP_DEBUG
  fprintf (stdout, "#Ddi_InterpDeref#");
  fflush (stdout);
#endif

  if (interpMgr->interpGraph[index].fanout > 0) {
    DdiInterpDeref (interpMgr, interpMgr->interpGraph[index].left);
    DdiInterpDeref (interpMgr, interpMgr->interpGraph[index].right);

    interpMgr->interpGraph[index].fanout--;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

int
DdiInterpNot (
  int index
  )
{
#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpNot#");
  fflush (stdout);
#endif

  return (-index);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

int
DdiInterpIsInverted (
  int index
  )
{
#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpIsInverted#");
  fflush (stdout);
#endif

  if (index > 0 ) {
    return (0);
  } else {
    return (1);
  }
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

int
DdiInterpRegular (
  int index
  )
{
#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpRegular#");
  fflush (stdout);
#endif

  return (abs(index));
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

int
DdiInterpOr (
  ddiInterpGraphMgr_t *interpMgr,
  int index1,
  int index2,
  int resolvedVar,
  const vec<Lit>& c
  )
{
  int i, index, tmpI;
  char *name, *tmpS, *name1, *name2, *str;

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpOr#");
  fflush (stdout);
#endif

  index = DdiInterpCreateNode (interpMgr);

  interpMgr->interpGraph[index].op = Ddi_InterpGraph_Or;
  interpMgr->interpGraph[index].indexBaig = DDI_INTERP_NULL_NODE_INDEX; 
  interpMgr->interpGraph[index].left = index1; 
  interpMgr->interpGraph[index].right = index2;

  if (DdiInterpIsInverted (index1)) {
    tmpI = DdiInterpNot (index1);
    tmpS = DdiInt2Str (tmpI);
    name1 = Pdtutil_Alloc (char, strlen (tmpS) + 5 + 1);
    sprintf (name1, "not(%s)", tmpS);
    Pdtutil_Free (tmpS);
  } else {
    name1 = DdiInt2Str (index1);
  }

  if (DdiInterpIsInverted (index2)) {
    tmpI = DdiInterpNot (index2);
    tmpS = DdiInt2Str (tmpI);
    name2 = Pdtutil_Alloc (char, strlen (tmpS) + 5 + 1);
    sprintf (name2, "not(%s)", tmpS);
    Pdtutil_Free (tmpS);
  } else {
    name2 = DdiInt2Str (index2);
  }

  interpMgr->interpGraph[index].name =
    Pdtutil_Alloc (char, strlen(name1) + strlen(name2) + 4 + 1);
  sprintf (interpMgr->interpGraph[index].name, "Nd_%s_%s", name1, name2);
  Pdtutil_Free(name1);
  Pdtutil_Free(name2);

  interpMgr->interpGraph[index].resolvedVar = resolvedVar;
  interpMgr->interpGraph[index].clauseArrayN = c.size();
  interpMgr->interpGraph[index].clauseArray =
    Pdtutil_Alloc (int, c.size());
  for (i=0; i<c.size(); i++) {
    interpMgr->interpGraph[index].clauseArray[i] = var(c[i]); 
    if (sign(c[i])) {
      interpMgr->interpGraph[index].clauseArray[i] =
	-(interpMgr->interpGraph[index].clauseArray[i]);
    }
  }

  return (index);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

int
DdiInterpAnd (
  ddiInterpGraphMgr_t *interpMgr,
  int index1,
  int index2,
  int resolvedVar,
  const vec<Lit>& c
  )
{
  int i, index;
  char *name, *tmp, *name1, *name2, *str;

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpAnd#");
  fflush (stdout);
#endif

  index = DdiInterpCreateNode (interpMgr);

  interpMgr->interpGraph[index].op = Ddi_InterpGraph_And;
  interpMgr->interpGraph[index].indexBaig = DDI_INTERP_NULL_NODE_INDEX; 
  interpMgr->interpGraph[index].left = index1;
  interpMgr->interpGraph[index].right = index2;

  if (DdiInterpIsInverted (index1)) {
    tmp = DdiInt2Str (DdiInterpNot (index1));
    name1 = Pdtutil_Alloc (char, strlen (tmp) + 5 + 1);
    sprintf (name1, "not(%s)", tmp);
    Pdtutil_Free (tmp);
  } else {
    name1 = DdiInt2Str (index1);
  }

  if (DdiInterpIsInverted (index2)) {
    tmp = DdiInt2Str (DdiInterpNot (index2));
    name2 = Pdtutil_Alloc (char, strlen (tmp) + 5 + 1);
    sprintf (name2, "not(%s)", tmp);
    Pdtutil_Free (tmp);
  } else {
    name2 = DdiInt2Str (index2);
  }

  interpMgr->interpGraph[index].name =
    Pdtutil_Alloc (char, strlen(name1) + strlen(name2) + 4 + 1);
  sprintf (interpMgr->interpGraph[index].name, "Nd_%s_%s", name1, name2);
  Pdtutil_Free(name1);
  Pdtutil_Free(name2);

  interpMgr->interpGraph[index].resolvedVar = resolvedVar;
  interpMgr->interpGraph[index].clauseArrayN = c.size();
  interpMgr->interpGraph[index].clauseArray =
    Pdtutil_Alloc (int, c.size());
  for (i=0; i<c.size(); i++) {
    interpMgr->interpGraph[index].clauseArray[i] = var(c[i]); 
    if (sign(c[i])) {
      interpMgr->interpGraph[index].clauseArray[i] =
	-(interpMgr->interpGraph[index].clauseArray[i]);
    }
  }

  return (index);
}

/**Function********************************************************************
  Synopsis    []
  Description [Converts an integer into a string]
  SideEffects []
  SeeAlso     []
******************************************************************************/

int
DdiInterpGetRoot (
  ddiInterpGraphMgr_t *interpMgr
  )
{
  if (interpMgr->interpGraphRoot != (-1)) {
    return (interpMgr->interpGraphRoot);
  } else {
    return (interpMgr->interpGraphIndex);
  }
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

void
DdiInterpSub (
  ddiInterpGraphMgr_t *interpMgr,
  int stepNMax
  )
{
  int i, sizeInitial;
  int n, l, ll, lr, r, rl, rr;
  int rv, rvl, rvr;
  int goOn, retValue;
  int llsN, lrsN, lcN, rlsN, rrsN, rcN;
  int llsNT, lrsNT, lcNT, rlsNT, rrsNT, rcNT;
  int debugLimit = (-1);

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpSub#");
  fflush (stdout);
#endif

  sizeInitial = interpMgr->interpGraphIndex;
  goOn = 1;
  llsNT = lrsNT = lcNT = rlsNT = rrsNT = rcNT = 0;
  i = 0;
  while (goOn==1 && (stepNMax==(-1) || i<stepNMax)) {
    llsN = lrsN = lcN = rlsN = rrsN = rcN = 0;
    goOn = 0;

    /* Linear Visit */
    for (n=2; n<=interpMgr->interpGraphIndex; n++) {
      if (interpMgr->interpGraph[n].op == Ddi_InterpGraph_Or) {

        rv = interpMgr->interpGraph[n].resolvedVar;
        l = DdiInterpRegular (interpMgr->interpGraph[n].left);
        r = DdiInterpRegular (interpMgr->interpGraph[n].right);
        rvl = interpMgr->interpGraph[l].resolvedVar;
        ll = DdiInterpRegular (interpMgr->interpGraph[l].left);
        lr = DdiInterpRegular (interpMgr->interpGraph[l].right);
        rvr = interpMgr->interpGraph[r].resolvedVar;
        rl = DdiInterpRegular (interpMgr->interpGraph[r].left);
        rr = DdiInterpRegular (interpMgr->interpGraph[r].right);

        if (interpMgr->interpGraph[l].fanout == 1) {
#if 1
          retValue = DdiInterpOpLComplex (interpMgr, interpMgr->interpGraph,
            n, rv, l, r, rvl, ll, lr, rvr, rl, rr);
	  debugLimit -= retValue; if (debugLimit==0) break;
          if (retValue == 1) {
            lcN++;
            goOn = 1;
            continue;
          }
#endif
#if 0
          retValue = DdiInterpOpLLSimple (interpMgr->interpGraph,
            n, rv, l, r, rvl, ll, lr, rvr, rl, rr);
	  debugLimit -= retValue; if (debugLimit==0) break;
          if (retValue == 1) {
            llsN++;
            goOn = 1;
            continue;
          }
#endif
#if 1
          retValue = DdiInterpOpLRSimple (interpMgr->interpGraph,
            n, rv, l, r, rvl, ll, lr, rvr, rl, rr);
	  debugLimit -= retValue; if (debugLimit==0) break;
          if (retValue == 1) {
            lrsN++;
            goOn = 1;
            continue;
          }
#endif
	}

        if (interpMgr->interpGraph[r].fanout == 1) {
#if 1
          retValue = DdiInterpOpRComplex (interpMgr, interpMgr->interpGraph,
            n, rv, l, r, rvl, ll, lr, rvr, rl, rr);
	  debugLimit -= retValue; if (debugLimit==0) break;
          if (retValue == 1) {
            rcN++;
            goOn = 1;
            continue;
          }
#endif
#if 1
          retValue = DdiInterpOpRLSimple (interpMgr->interpGraph,
            n, rv, l, r, rvl, ll, lr, rvr, rl, rr);
	  debugLimit -= retValue; if (debugLimit==0) break;
          if (retValue == 1) {
            rlsN++;
            goOn = 1;
            continue;
          }
#endif
#if 1
          retValue = DdiInterpOpRRSimple (interpMgr->interpGraph,
            n, rv, l, r, rvl, ll, lr, rvr, rl, rr);
	  //retValue = DdiInterpOpRRSimpleInv (interpMgr->interpGraph,
          //  n, rvr, rr, r, 0, 0, 0, rv, rl, l);
	  debugLimit -= retValue; if (debugLimit==0) break;
          if (retValue == 1) {
            rrsN++;
            goOn = 1;
            continue;
          }
#endif
        }
      }
    }

    fprintf (stdout,
      "RedStep %3d/%3d: lc=%4d lls=%4d lrs=%4d rc=%4d rls=%4d rrs=%4d TOT=%4d\n",
      i+1, stepNMax, lcN, llsN, lrsN, rcN, rlsN, rrsN,
      lcN+llsN+lrsN+rcN+rlsN+rrsN);

    lcNT += lcN;
    llsNT += llsN;
    lrsNT += lrsN;
    rcNT += rcN;
    rlsNT += rlsN;
    rrsNT += rrsN;

    i++;
  } /* Go On To Next Linear Visit? */

  if (sizeInitial != interpMgr->interpGraphIndex) {
    interpMgr->interpGraphRoot = sizeInitial;
  }

  fprintf (stdout,
    "RedTotal       : lc=%4d lls=%4d lrs=%4d rc=%4d rls=%4d rrs=%4d TOT=%4d\n",
    lcNT, llsNT, lrsNT, rcNT, rlsNT, rrsNT,
    lcNT+llsNT+lrsNT+rcNT+rlsNT+rrsNT);
  fprintf (stdout, "Size           : initial=%8d final  =%8d\n",
    sizeInitial, interpMgr->interpGraphIndex);

  return;
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

void
DdiInterpSuper (
  ddiInterpGraphMgr_t *interpMgr,
  int stepNMax
  )
{
  int i, sizeInitial;
  int n, l, ll, lr, r, rl, rr;
  int rv, rvl, rvr;
  int goOn, retValue;
  int llsN, lrsN, lcN, rlsN, rrsN, rcN;
  int llsNT, lrsNT, lcNT, rlsNT, rrsNT, rcNT;
  int debugLimit = (-1);

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpSuper#");
  fflush (stdout);
#endif

  sizeInitial = interpMgr->interpGraphIndex;
  goOn = 1;
  llsNT = lrsNT = lcNT = rlsNT = rrsNT = rcNT = 0;
  i = 0;
  while (goOn==1 && (stepNMax==(-1) || i<stepNMax)) {
    llsN = lrsN = lcN = rlsN = rrsN = rcN = 0;
    goOn = 0;

    /* Linear Visit */
    for (n=2; n<=interpMgr->interpGraphIndex; n++) {
      if (interpMgr->interpGraph[n].op == Ddi_InterpGraph_And) {

        rv = interpMgr->interpGraph[n].resolvedVar;
        l = DdiInterpRegular (interpMgr->interpGraph[n].left);
        r = DdiInterpRegular (interpMgr->interpGraph[n].right);
        rvl = interpMgr->interpGraph[l].resolvedVar;
        ll = DdiInterpRegular (interpMgr->interpGraph[l].left);
        lr = DdiInterpRegular (interpMgr->interpGraph[l].right);
        rvr = interpMgr->interpGraph[r].resolvedVar;
        rl = DdiInterpRegular (interpMgr->interpGraph[r].left);
        rr = DdiInterpRegular (interpMgr->interpGraph[r].right);

        if (interpMgr->interpGraph[l].fanout == 1) {
#if 1
          retValue =
            DdiInterpOpLComplexInv (interpMgr, interpMgr->interpGraph,
              n, rv, l, r, rvl, ll, lr, rvr, rl, rr);
	  debugLimit -= retValue; if (debugLimit==0) break;
          if (retValue == 1) {
            lcN++;
            goOn = 1;
            continue;
          }
#endif
#if 1
          retValue =
            DdiInterpOpLLSimpleInv (interpMgr->interpGraph,
              n, rv, l, r, rvl, ll, lr, rvr, rl, rr);
	  debugLimit -= retValue; if (debugLimit==0) break;
          if (retValue == 1) {
            llsN++;
            goOn = 1;
            continue;
          }
#endif
#if 1
          retValue =
            DdiInterpOpLRSimpleInv (interpMgr->interpGraph,
              n, rv, l, r, rvl, ll, lr, rvr, rl, rr);
	  debugLimit -= retValue; if (debugLimit==0) break;
          if (retValue == 1) {
            lrsN++;
            goOn = 1;
            continue;
          }
#endif
        }

        if (interpMgr->interpGraph[r].fanout == 1) {
#if 1
          retValue =
            DdiInterpOpRComplexInv (interpMgr, interpMgr->interpGraph,
              n, rv, l, r, rvl, ll, lr, rvr, rl, rr);
	  debugLimit -= retValue; if (debugLimit==0) break;
          if (retValue == 1) {
            rcN++;
            goOn = 1;
            continue;
          }
#endif
#if 1
          retValue =
            DdiInterpOpRLSimpleInv (interpMgr->interpGraph,
              n, rv, l, r, rvl, ll, lr, rvr, rl, rr);
          debugLimit -= retValue; if (debugLimit==0) break;
          if (retValue == 1) {
            rlsN++;
            goOn = 1;
            continue;
          }
#endif
#if 1
          retValue =
            DdiInterpOpRRSimpleInv (interpMgr->interpGraph,
              n, rv, l, r, rvl, ll, lr, rvr, rl, rr);
	  debugLimit -= retValue; if (debugLimit==0) break;
          if (retValue == 1) {
            rrsN++;
            goOn = 1;
            continue;
          }
#endif
        }
      }
    }

    fprintf (stdout,
      "InvRedStep %3d/%3d: lc=%4d lls=%4d lrs=%4d rc=%4d rls=%4d rrs=%4d TOT=%4d\n",
      i+1, stepNMax, lcN, llsN, lrsN, rcN, rlsN, rrsN,
      lcN+llsN+lrsN+rcN+rlsN+rrsN);

    lcNT += lcN;
    llsNT += llsN;
    lrsNT += lrsN;
    rcNT += rcN;
    rlsNT += rlsN;
    rrsNT += rrsN;

    i++;
  } /* Go On To Next Linear Visit? */

  if (sizeInitial != interpMgr->interpGraphIndex) {
    interpMgr->interpGraphRoot = sizeInitial;
  }

  fprintf (stdout,
    "InvRedTotal       : lc=%4d lls=%4d lrs=%4d rc=%4d rls=%4d rrs=%4d TOT=%4d\n",
    lcNT, llsNT, lrsNT, rcNT, rlsNT, rrsNT,
    lcNT+llsNT+lrsNT+rcNT+rlsNT+rrsNT);
  fprintf (stdout, "Size           : initial=%8d final  =%8d\n",
    sizeInitial, interpMgr->interpGraphIndex);

  return;
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static int
DdiInterpOpLComplex (
  ddiInterpGraphMgr_t *interpMgr, 
  ddiInterpGraph_t *interpGraph,
  int n,
  int rv,
  int l,
  int r,
  int rvl,
  int ll,
  int lr,
  int rvr,
  int rl,
  int rr
  )
{
  int tmpIndex, index;
  ddiInterpGraph_t tmpNode;

  if (interpGraph[l].op != Ddi_InterpGraph_And) {
    return (0);
  }

  if (
    DdiInterpResVarInClause (rvl, interpGraph[ll])==0 ||
    DdiInterpResVarInClause (rvl, interpGraph[lr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[ll])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[lr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[r])==0
  ) {
    return (0);
  }


  /* Reduce */
#if 0
  fprintf (stdout, "LC Reduction:\n");
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r);
  fprintf (stdout, "ll -->");
  DdiInterpPrintNode (interpGraph, ll);
  fprintf (stdout, "lr -->");
  DdiInterpPrintNode (interpGraph, lr);
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  index = DdiInterpCreateNode (interpMgr);
  interpGraph[index].op = Ddi_InterpGraph_Or;
  interpGraph[index].flag = DDI_INTERP_NULL_NODE_INDEX;
  interpGraph[index].name = Pdtutil_Alloc (char, 10 + 1);
  strcpy (interpGraph[index].name, "Nd_LeftOpt");
  interpGraph[index].resolvedVar = rv;
  interpGraph[index].indexBaig = DDI_INTERP_NULL_NODE_INDEX;
  interpGraph[index].left = interpGraph[l].right;
  interpGraph[index].right = interpGraph[n].right;

  interpGraph[n].op = Ddi_InterpGraph_And;
  interpGraph[l].op = Ddi_InterpGraph_Or;
  
  interpGraph[n].resolvedVar = rvl;
  interpGraph[l].resolvedVar = rv;

  interpGraph[l].right = interpGraph[n].right;  
  interpGraph[n].right = index;

  DdiInterpCreateClauseArray (interpGraph, l);
  DdiInterpCreateClauseArray (interpGraph, index);

#if 0
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l); 
  fprintf (stdout, "index -->");
  DdiInterpPrintNode (interpGraph, index); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  return (1);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static int
DdiInterpOpLLSimple (
  ddiInterpGraph_t *interpGraph,
  int n,
  int rv,
  int l,
  int r,
  int rvl,
  int ll,
  int lr,
  int rvr,
  int rl,
  int rr
  )
{
  int tmpIndex;
  ddiInterpGraph_t tmpNode;

  if (interpGraph[l].op != Ddi_InterpGraph_And) {
    return (0);
  }

  if (DdiInterpResVarInClause (rvl, interpGraph[ll])==0 ||
    DdiInterpResVarInClause (rvl, interpGraph[lr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[ll])==1 ||
    DdiInterpResVarInClause (rv, interpGraph[lr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[r])==0
  ) {
    return (0);
  }

#if 0
  fprintf (stdout, "LLS Reduction:\n");
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r);
  fprintf (stdout, "ll -->");
  DdiInterpPrintNode (interpGraph, ll);
  fprintf (stdout, "lr -->");
  DdiInterpPrintNode (interpGraph, lr);
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  /* Reduce */
  interpGraph[n].op = Ddi_InterpGraph_And;
  interpGraph[l].op = Ddi_InterpGraph_Or;
  
  interpGraph[n].resolvedVar = rvl;
  interpGraph[l].resolvedVar = rv;
  
  tmpIndex = interpGraph[n].right;
  interpGraph[n].right = interpGraph[l].left;
  interpGraph[l].left = tmpIndex;
  
  DdiInterpResVarExchange (rv, rvl, interpGraph[l]);

#if 0
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
  fprintf (stdout, "LLS Reduction END.\n");
#endif

  return (1);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static int
DdiInterpOpLRSimple (
  ddiInterpGraph_t *interpGraph,
  int n,
  int rv,
  int l,
  int r,
  int rvl,
  int ll,
  int lr,
  int rvr,
  int rl,
  int rr
  )
{
  int tmpIndex;
  ddiInterpGraph_t tmpNode;

  if (interpGraph[l].op != Ddi_InterpGraph_And) {
    return (0);
  }

  if (DdiInterpResVarInClause (rvl, interpGraph[ll])==0 ||
    DdiInterpResVarInClause (rvl, interpGraph[lr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[ll])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[lr])==1 ||
    DdiInterpResVarInClause (rv, interpGraph[r])==0
  ) {
    return (0);
  }

#if 0
  fprintf (stdout, "LRS Reduction:\n");
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r);
  fprintf (stdout, "ll -->");
  DdiInterpPrintNode (interpGraph, ll);
  fprintf (stdout, "lr -->");
  DdiInterpPrintNode (interpGraph, lr);
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  /* Reduce */
  interpGraph[n].op = Ddi_InterpGraph_And;
  interpGraph[l].op = Ddi_InterpGraph_Or;
  
  interpGraph[n].resolvedVar = rvl;
  interpGraph[l].resolvedVar = rv;
  
  tmpIndex = interpGraph[n].right;
  interpGraph[n].right = interpGraph[l].right;
  interpGraph[l].right = tmpIndex;
  
  DdiInterpResVarExchange (rv, rvl, interpGraph[l]);

#if 0
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  return (1);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static int
DdiInterpOpRComplex (
  ddiInterpGraphMgr_t *interpMgr, 
  ddiInterpGraph_t *interpGraph,
  int n,
  int rv,
  int l,
  int r,
  int rvl,
  int ll,
  int lr,
  int rvr,
  int rl,
  int rr
  )
{
  int tmpIndex, index;
  ddiInterpGraph_t tmpNode;

  if (interpGraph[r].op != Ddi_InterpGraph_And) {
    return (0);
  }

  if (DdiInterpResVarInClause (rvr, interpGraph[rl])==0 ||
    DdiInterpResVarInClause (rvr, interpGraph[rr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[rl])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[rr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[l])==0
  ) {
    return (0);
  }

  /* Reduce */
#if 0
  fprintf (stdout, "RC Reduction:\n");
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l);
  fprintf (stdout, "rl -->");
  DdiInterpPrintNode (interpGraph, rl);
  fprintf (stdout, "rr -->");
  DdiInterpPrintNode (interpGraph, rr);
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  index = DdiInterpCreateNode (interpMgr);
  interpGraph[index].op = Ddi_InterpGraph_Or;
  interpGraph[index].flag = DDI_INTERP_NULL_NODE_INDEX;
  interpGraph[index].name = Pdtutil_Alloc (char, 11 + 1);
  strcpy (interpGraph[index].name, "Nd_RightOpt");
  interpGraph[index].resolvedVar = rv;
  interpGraph[index].indexBaig = DDI_INTERP_NULL_NODE_INDEX;
  interpGraph[index].left = interpGraph[r].left;
  interpGraph[index].right = interpGraph[n].left;

  interpGraph[n].op = Ddi_InterpGraph_And;
  interpGraph[r].op = Ddi_InterpGraph_Or;
  
  interpGraph[n].resolvedVar = rvr;
  interpGraph[r].resolvedVar = rv;

  interpGraph[r].left = interpGraph[n].left;  
  interpGraph[n].left = index;

  DdiInterpCreateClauseArray (interpGraph, r);
  DdiInterpCreateClauseArray (interpGraph, index);

#if 0
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r); 
  fprintf (stdout, "index -->");
  DdiInterpPrintNode (interpGraph, index); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  return (1);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static int
DdiInterpOpRLSimple (
  ddiInterpGraph_t *interpGraph,
  int n,
  int rv,
  int l,
  int r,
  int rvl,
  int ll,
  int lr,
  int rvr,
  int rl,
  int rr
  )
{
  int tmpIndex;
  ddiInterpGraph_t tmpNode;

  if (interpGraph[r].op != Ddi_InterpGraph_And) {
    return (0);
  }

  if (DdiInterpResVarInClause (rvr, interpGraph[rl])==0 ||
    DdiInterpResVarInClause (rvr, interpGraph[rr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[rl])==1 ||
    DdiInterpResVarInClause (rv, interpGraph[rr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[l])==0
  ) {
    return (0);
  }

#if 0
  fprintf (stdout, "RLS Reduction:\n");
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l); 
  fprintf (stdout, "rl -->");
  DdiInterpPrintNode (interpGraph, rl);
  fprintf (stdout, "rr -->");
  DdiInterpPrintNode (interpGraph, rr);
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r);
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  /* Reduce */
  interpGraph[n].op = Ddi_InterpGraph_And;
  interpGraph[r].op = Ddi_InterpGraph_Or;
  
  interpGraph[n].resolvedVar = rvr;
  interpGraph[r].resolvedVar = rv;
  
  tmpIndex = interpGraph[n].left;
  interpGraph[n].left = interpGraph[r].left;
  interpGraph[r].left = tmpIndex;
  
  DdiInterpResVarExchange (rv, rvr, interpGraph[r]);

#if 0
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  return (1);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static int
DdiInterpOpRRSimple (
  ddiInterpGraph_t *interpGraph,
  int n,
  int rv,
  int l,
  int r,
  int rvl,
  int ll,
  int lr,
  int rvr,
  int rl,
  int rr
  )
{
  int tmpIndex;
  ddiInterpGraph_t tmpNode;

  if (interpGraph[r].op != Ddi_InterpGraph_And) {
    return (0);
  }

  if (DdiInterpResVarInClause (rvr, interpGraph[rl])==0 ||
    DdiInterpResVarInClause (rvr, interpGraph[rr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[rl])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[rr])==1 ||
    DdiInterpResVarInClause (rv, interpGraph[l])==0
  ) {
    return (0);
  }

#if 1
  fprintf (stdout, "RRS Reduction:\n");
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l); 
  fprintf (stdout, "rl -->");
  DdiInterpPrintNode (interpGraph, rl);
  fprintf (stdout, "rr -->");
  DdiInterpPrintNode (interpGraph, rr);
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r);
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  /* Reduce */
  interpGraph[n].op = Ddi_InterpGraph_And;
  interpGraph[r].op = Ddi_InterpGraph_Or;
  
  interpGraph[n].resolvedVar = rvr;
  interpGraph[r].resolvedVar = rv;
  
  tmpIndex = interpGraph[n].left;
  interpGraph[n].left = interpGraph[r].right;
  interpGraph[r].right = tmpIndex;
  
  DdiInterpResVarExchange (rv, rvr, interpGraph[r]);

#if 1
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
  fprintf (stdout, "RRS Reduction END.\n");
#endif

  return (1);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static int
DdiInterpOpLComplexInv (
  ddiInterpGraphMgr_t *interpMgr, 
  ddiInterpGraph_t *interpGraph,
  int n,
  int rv,
  int l,
  int r,
  int rvl,
  int ll,
  int lr,
  int rvr,
  int rl,
  int rr
  )
{
  if (interpGraph[l].op != Ddi_InterpGraph_Or ||
      interpGraph[r].op != Ddi_InterpGraph_Or) {
    return (0);
  }

  if (
    DdiInterpResVarInClause (rvl, interpGraph[ll])==0 ||
    DdiInterpResVarInClause (rvl, interpGraph[lr])==0 ||
    DdiInterpResVarInClause (rvl, interpGraph[rl])==0 ||
    DdiInterpResVarInClause (rvl, interpGraph[rr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[l])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[r])==0
  ) {
    return (0);
  }


  /* Reduce */
#if 1
  fprintf (stdout, "LC Reduction:\n");
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r);
  fprintf (stdout, "ll -->");
  DdiInterpPrintNode (interpGraph, ll);
  fprintf (stdout, "lr -->");
  DdiInterpPrintNode (interpGraph, lr);
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  interpGraph[n].op = Ddi_InterpGraph_Or;
  interpGraph[l].op = Ddi_InterpGraph_And;

  interpGraph[n].resolvedVar = rvl;
  interpGraph[l].resolvedVar = rv;

  interpGraph[l].right = interpGraph[r].right;  
  interpGraph[n].right = interpGraph[r].left;  

  DdiInterpCreateClauseArray (interpGraph, l);

  interpGraph[r].fanout--;
  if (interpGraph[r].fanout == 0) {
    interpGraph[r].op = Ddi_InterpGraph_Null;
    interpGraph[r].flag = DDI_INTERP_NULL_NODE_INDEX;
    Pdtutil_Free (interpGraph[r].name);
    interpGraph[r].name = NULL;
    interpGraph[r].resolvedVar = DDI_INTERP_NULL_NODE_INDEX;
    Pdtutil_Free (interpGraph[r].clauseArray);
    interpGraph[r].clauseArrayN = 0;
    interpGraph[r].indexBaig = DDI_INTERP_NULL_NODE_INDEX;
    interpGraph[r].left = DDI_INTERP_NULL_NODE_INDEX;
    interpGraph[r].right = DDI_INTERP_NULL_NODE_INDEX;
  }

#if 1
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  return (1);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static int
DdiInterpOpLLSimpleInv (
  ddiInterpGraph_t *interpGraph,
  int n,
  int rv,
  int l,
  int r,
  int rvl,
  int ll,
  int lr,
  int rvr,
  int rl,
  int rr
  )
{
  int tmpIndex;
  ddiInterpGraph_t tmpNode;

  if (interpGraph[l].op != Ddi_InterpGraph_Or) {
    return (0);
  }

  if (DdiInterpResVarInClause (rvl, interpGraph[ll])==0 ||
    DdiInterpResVarInClause (rvl, interpGraph[lr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[ll])==1 ||
    DdiInterpResVarInClause (rv, interpGraph[lr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[r])==0
  ) {
    return (0);
  }

#if 0
  fprintf (stdout, "LLS Reduction:\n");
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r);
  fprintf (stdout, "ll -->");
  DdiInterpPrintNode (interpGraph, ll);
  fprintf (stdout, "lr -->");
  DdiInterpPrintNode (interpGraph, lr);
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  /* Reduce */
  interpGraph[n].op = Ddi_InterpGraph_Or;
  interpGraph[l].op = Ddi_InterpGraph_And;
  
  interpGraph[n].resolvedVar = rvl;
  interpGraph[l].resolvedVar = rv;
  
  tmpIndex = interpGraph[n].right;
  interpGraph[n].right = interpGraph[l].left;
  interpGraph[l].left = tmpIndex;
  
  DdiInterpResVarExchange (rv, rvl, interpGraph[l]);

#if 0
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  return (1);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static int
DdiInterpOpLRSimpleInv (
  ddiInterpGraph_t *interpGraph,
  int n,
  int rv,
  int l,
  int r,
  int rvl,
  int ll,
  int lr,
  int rvr,
  int rl,
  int rr
  )
{
  int tmpIndex;
  ddiInterpGraph_t tmpNode;

  if (interpGraph[l].op != Ddi_InterpGraph_Or) {
    return (0);
  }

  if (DdiInterpResVarInClause (rvl, interpGraph[ll])==0 ||
    DdiInterpResVarInClause (rvl, interpGraph[lr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[ll])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[lr])==1 ||
    DdiInterpResVarInClause (rv, interpGraph[r])==0
  ) {
    return (0);
  }

#if 0
  fprintf (stdout, "LRS Reduction:\n");
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r);
  fprintf (stdout, "ll -->");
  DdiInterpPrintNode (interpGraph, ll);
  fprintf (stdout, "lr -->");
  DdiInterpPrintNode (interpGraph, lr);
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  /* Reduce */
  interpGraph[n].op = Ddi_InterpGraph_Or;
  interpGraph[l].op = Ddi_InterpGraph_And;
  
  interpGraph[n].resolvedVar = rvl;
  interpGraph[l].resolvedVar = rv;
  
  tmpIndex = interpGraph[n].right;
  interpGraph[n].right = interpGraph[l].right;
  interpGraph[l].right = tmpIndex;
  
  DdiInterpResVarExchange (rv, rvl, interpGraph[l]);

#if 0
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  return (1);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static int
DdiInterpOpRComplexInv (
  ddiInterpGraphMgr_t *interpMgr, 
  ddiInterpGraph_t *interpGraph,
  int n,
  int rv,
  int l,
  int r,
  int rvl,
  int ll,
  int lr,
  int rvr,
  int rl,
  int rr
  )
{
  if (interpGraph[l].op != Ddi_InterpGraph_Or ||
      interpGraph[r].op != Ddi_InterpGraph_Or) {
    return (0);
  }

  if (DdiInterpResVarInClause (rvr, interpGraph[ll])==0 ||
    DdiInterpResVarInClause (rvr, interpGraph[lr])==0 ||
    DdiInterpResVarInClause (rvr, interpGraph[rl])==0 ||
    DdiInterpResVarInClause (rvr, interpGraph[rr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[l])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[r])==0
  ) {
    return (0);
  }

  /* Reduce */
#if 0
  fprintf (stdout, "RC Reduction:\n");
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l);
  fprintf (stdout, "rl -->");
  DdiInterpPrintNode (interpGraph, rl);
  fprintf (stdout, "rr -->");
  DdiInterpPrintNode (interpGraph, rr);
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  interpGraph[n].op = Ddi_InterpGraph_Or;
  interpGraph[r].op = Ddi_InterpGraph_And;
  
  interpGraph[n].resolvedVar = rvr;
  interpGraph[r].resolvedVar = rv;

  interpGraph[r].left = interpGraph[l].left;  
  interpGraph[n].left = interpGraph[l].right;  

  DdiInterpCreateClauseArray (interpGraph, r);

  interpGraph[l].fanout--;
  if (interpGraph[l].fanout == 0) {
    interpGraph[l].op = Ddi_InterpGraph_Null;
    interpGraph[l].flag = DDI_INTERP_NULL_NODE_INDEX;
    Pdtutil_Free (interpGraph[l].name);
    interpGraph[l].name = NULL;
    interpGraph[l].resolvedVar = DDI_INTERP_NULL_NODE_INDEX;
    Pdtutil_Free (interpGraph[l].clauseArray);
    interpGraph[l].clauseArrayN = 0;
    interpGraph[l].indexBaig = DDI_INTERP_NULL_NODE_INDEX;
    interpGraph[l].left = DDI_INTERP_NULL_NODE_INDEX;
    interpGraph[l].right = DDI_INTERP_NULL_NODE_INDEX;
  }

#if 0
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  return (1);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static int
DdiInterpOpRLSimpleInv (
  ddiInterpGraph_t *interpGraph,
  int n,
  int rv,
  int l,
  int r,
  int rvl,
  int ll,
  int lr,
  int rvr,
  int rl,
  int rr
  )
{
  int tmpIndex;
  ddiInterpGraph_t tmpNode;

  if (interpGraph[r].op != Ddi_InterpGraph_Or) {
    return (0);
  }

  if (DdiInterpResVarInClause (rvr, interpGraph[rl])==0 ||
    DdiInterpResVarInClause (rvr, interpGraph[rr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[rl])==1 ||
    DdiInterpResVarInClause (rv, interpGraph[rr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[l])==0
  ) {
    return (0);
  }

#if 0
  fprintf (stdout, "RLS Reduction:\n");
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l); 
  fprintf (stdout, "rl -->");
  DdiInterpPrintNode (interpGraph, rl);
  fprintf (stdout, "rr -->");
  DdiInterpPrintNode (interpGraph, rr);
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r);
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  /* Reduce */
  interpGraph[n].op = Ddi_InterpGraph_Or;
  interpGraph[r].op = Ddi_InterpGraph_And;
  
  interpGraph[n].resolvedVar = rvr;
  interpGraph[r].resolvedVar = rv;
  
  tmpIndex = interpGraph[n].left;
  interpGraph[n].left = interpGraph[r].left;
  interpGraph[r].left = tmpIndex;
  
  DdiInterpResVarExchange (rv, rvr, interpGraph[r]);

#if 0
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  return (1);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static int
DdiInterpOpRRSimpleInv (
  ddiInterpGraph_t *interpGraph,
  int n,
  int rv,
  int l,
  int r,
  int rvl,
  int ll,
  int lr,
  int rvr,
  int rl,
  int rr
  )
{
  int tmpIndex;
  ddiInterpGraph_t tmpNode;

  if (interpGraph[r].op != Ddi_InterpGraph_Or) {
    return (0);
  }

  if (DdiInterpResVarInClause (rvr, interpGraph[rl])==0 ||
    DdiInterpResVarInClause (rvr, interpGraph[rr])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[rl])==0 ||
    DdiInterpResVarInClause (rv, interpGraph[rr])==1 ||
    DdiInterpResVarInClause (rv, interpGraph[l])==0
  ) {
    return (0);
  }

#if 1
  fprintf (stdout, "RRS Inv Reduction:\n");
  fprintf (stdout, "l -->");
  DdiInterpPrintNode (interpGraph, l); 
  fprintf (stdout, "rl -->");
  DdiInterpPrintNode (interpGraph, rl);
  fprintf (stdout, "rr -->");
  DdiInterpPrintNode (interpGraph, rr);
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r);
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
#endif

  /* Reduce */
  interpGraph[n].op = Ddi_InterpGraph_Or;
  interpGraph[r].op = Ddi_InterpGraph_And;
  
  interpGraph[n].resolvedVar = rvr;
  interpGraph[r].resolvedVar = rv;
  
  tmpIndex = interpGraph[n].left;
  interpGraph[n].left = interpGraph[r].right;
  interpGraph[r].right = tmpIndex;
  
  DdiInterpResVarExchange (rv, rvr, interpGraph[r]);

#if 1
  fprintf (stdout, "r -->");
  DdiInterpPrintNode (interpGraph, r); 
  fprintf (stdout, "n -->");
  DdiInterpPrintNode (interpGraph, n);
  fprintf (stdout, "RRS Inv Reduction END.\n");
#endif

  return (1);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static bAigEdge_t
DdiInterpToBaigRecur (
  ddiInterpGraphMgr_t *interpMgr,
  bAig_Manager_t *bMgr,
  int index
  )
{
  int n, leftInterp, rightInterp;
  int isInverted;
  bAigEdge_t leftBaig, rightBaig, resBaig;

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpToBaigRecur#");
  fflush (stdout);
#endif

  if (index == DDI_INTERP_NULL_NODE_INDEX) {
    fprintf (stdout, "Null Interpolation Node reached.");
    exit (1);
      //Pdtutil_Assert (interpMgr->interpGraph[n].op!=Ddi_InterpGraph_Zero,
      //"Zero Interpolation Node with No Baig Associated.");
  }

  if (DdiInterpIsInverted (index)) {
    isInverted = 1;
    n = DdiInterpRegular (index);
  } else {
    isInverted = 0;
    n = index;
  }

  /* Zero Involved? */
  if (n == ddi_InterpGraph_Zero ||
    interpMgr->interpGraph[n].indexBaig != DDI_INTERP_NULL_NODE_INDEX) {
    if (isInverted == 1) {
      return (bAig_Not(interpMgr->interpGraph[n].indexBaig));
    } else {
      return (interpMgr->interpGraph[n].indexBaig);
    }
  }

  Pdtutil_Assert (interpMgr->interpGraph[n].op!=Ddi_InterpGraph_Zero,
    "Zero Interpolation Node with No Baig Associated.");
  Pdtutil_Assert (interpMgr->interpGraph[n].op!=Ddi_InterpGraph_One,
    "One Interpolation Node with No Baig Associated.");
  Pdtutil_Assert (interpMgr->interpGraph[n].op!=Ddi_InterpGraph_Var,
    "Variable Interpolation Node with No Baig Associated.");

  leftInterp = interpMgr->interpGraph[n].left;
  leftBaig = DdiInterpToBaigRecur (interpMgr, bMgr, leftInterp);

  rightInterp = interpMgr->interpGraph[n].right;
  rightBaig = DdiInterpToBaigRecur (interpMgr, bMgr, rightInterp);

  switch (interpMgr->interpGraph[n].op) {
    case Ddi_InterpGraph_Null:
    case Ddi_InterpGraph_Zero:
    case Ddi_InterpGraph_One:
    case Ddi_InterpGraph_Var:
      fprintf (stdout, "Error Wrong Interpolation Node.\n");
      break;
    case Ddi_InterpGraph_Or:
      resBaig = bAig_Or (bMgr, leftBaig, rightBaig);
      break;
    case Ddi_InterpGraph_And:
      resBaig = bAig_And (bMgr, leftBaig, rightBaig);
      break;
    default:
      fprintf (stderr, "Format Error.\n");
      break;
  }

  bAig_Ref (bMgr, resBaig);
  interpMgr->interpGraph[n].indexBaig = resBaig;

  return (resBaig);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static int
DdiInterpCreateNode (
  ddiInterpGraphMgr_t *interpMgr
  )
{
  int index;

#if DDI_INTERP_DEBUG
#if 0
  fprintf (stdout, "#DdiInterpCreateNode#");
#elseif
  fprintf (stdout, "#DdiInterpCreateNode");
  fprintf (stdout, "(%d-%d)", interpMgr->interpGraphIndex,
    interpMgr->interpGraphSize);
  fprintf (stdout, "#");
  fflush (stdout);
#endif
#endif

  if (interpMgr->interpGraphIndex >= (interpMgr->interpGraphSize-1)) {
    /* Check fo Overflow on Size */
    if (interpMgr->interpGraphSize >= (interpArrayMaxSize-1)) {
      fprintf (stdout, "Overflow.\n");
      exit (1);
    }

    /* Re-Size */
    interpMgr->interpGraphSize = 2 * interpMgr->interpGraphSize;

    if (interpMgr->interpGraphSize > interpArrayMaxSize ) {
      interpMgr->interpGraphSize = interpArrayMaxSize;
    }

    interpMgr->interpGraph = Pdtutil_Realloc (ddiInterpGraph_t,
      interpMgr->interpGraph, interpMgr->interpGraphSize);
  }

  /* Return New Node */
  interpMgr->interpGraphIndex = interpMgr->interpGraphIndex + 1;
  index = interpMgr->interpGraphIndex;

  /* Init Fields */
  interpMgr->interpGraph[index].op = Ddi_InterpGraph_Null;
  interpMgr->interpGraph[index].flag = DDI_INTERP_NULL_NODE_INDEX;
  interpMgr->interpGraph[index].fanout = DDI_INTERP_NULL_NODE_INDEX;
  interpMgr->interpGraph[index].name = NULL;
  interpMgr->interpGraph[index].resolvedVar = DDI_INTERP_NULL_NODE_INDEX;
  interpMgr->interpGraph[index].clauseArray = NULL;
  interpMgr->interpGraph[index].clauseArrayN = DDI_INTERP_NULL_NODE_INDEX;
  interpMgr->interpGraph[index].indexBaig = DDI_INTERP_NULL_NODE_INDEX;
  interpMgr->interpGraph[index].left = DDI_INTERP_NULL_NODE_INDEX;
  interpMgr->interpGraph[index].right = DDI_INTERP_NULL_NODE_INDEX;

  return (index);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static void
DdiInterpPrintNode (
  ddiInterpGraph_t *interpGraph,
  int index
  )
{
  int i;
  char str[10];

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpPrintNode#");
  fflush (stdout);
#endif

  switch (interpGraph[index].op) {
    case Ddi_InterpGraph_Null:
      strcpy (str, "Null");
      break;
    case Ddi_InterpGraph_Zero:
      strcpy (str, "Zero");
      break;
    case Ddi_InterpGraph_One:
      strcpy (str, "One");
      break;
    case Ddi_InterpGraph_Var:
      strcpy (str, "Var");
      break;
    case Ddi_InterpGraph_Clause:
      strcpy (str, "Clause");
      break;
    case Ddi_InterpGraph_Or:
      strcpy (str, "Or");
      break;
    case Ddi_InterpGraph_And:
      strcpy (str, "And");
      break;
    default:
      fprintf (stderr, "Format Error.\n");
      break;
  }

  fprintf (stdout,
    "id(%d) %s flag(%d) %s fanout(%d) index(%d) Left(%d) Right(%d) ResVar(%d) NVars(%d)",
    index, str, interpGraph[index].flag, interpGraph[index].name,
    interpGraph[index].fanout, interpGraph[index].indexBaig,
    interpGraph[index].left, interpGraph[index].right,
    interpGraph[index].resolvedVar, interpGraph[index].clauseArrayN);
  fprintf (stdout, " Vars(");
  for (i=0; i<interpGraph[index].clauseArrayN; i++) {
    fprintf (stdout, "%d ", interpGraph[index].clauseArray[i]);
  }
  fprintf (stdout, ")\n");

  return;
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static int
DdiInterpResVarInClause (
  int index,
  ddiInterpGraph_t node
  )
{
  int i;

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpResVarInClause#");
  fflush (stdout);
#endif

  for (i=0; i<node.clauseArrayN; i++) {
    if (DdiInterpRegular (index) ==
      DdiInterpRegular (node.clauseArray[i])) {
      return (1);
    }
  }

  return (0);
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static void
DdiInterpResVarExchange (
  int indexOld,
  int indexNew,
  ddiInterpGraph_t node
  )
{
  int i;

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpResVarExchange#");
  fflush (stdout);
#endif

  for (i=0; i<node.clauseArrayN; i++) {
    if (DdiInterpRegular (node.clauseArray[i]) ==
      DdiInterpRegular (indexOld)) {
	node.clauseArray[i] = indexNew;
        return;
    }
  }

  return;
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static void
DdiInterpCreateClauseArray (
  ddiInterpGraph_t *interpGraph,
  int index
  )
{
  int i, j, k1, k2, l, r, varN;
  int flagSkipVar;

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpCreateClauseArray#");
  fflush (stdout);
#endif

  l = interpGraph[index].left;
  r = interpGraph[index].right;

  Pdtutil_Free (interpGraph[index].clauseArray);
  varN = interpGraph[l].clauseArrayN + interpGraph[r].clauseArrayN - 2;

  interpGraph[index].clauseArray = Pdtutil_Alloc (int, varN);

  for (i=j=0; i<interpGraph[l].clauseArrayN; i++) {
    if (DdiInterpRegular (interpGraph[l].clauseArray[i]) == 
      DdiInterpRegular (interpGraph[index].resolvedVar)) {
      continue;
    }
    interpGraph[index].clauseArray[j] = interpGraph[l].clauseArray[i];
    j++;
  }

  k1 = j;
  for (i=0; i<interpGraph[r].clauseArrayN; i++) {
    flagSkipVar = 0;
    if (DdiInterpRegular (interpGraph[r].clauseArray[i]) == 
      DdiInterpRegular (interpGraph[index].resolvedVar)) {
      flagSkipVar = 1;
    }
    /* Eliminate Duplicate Variables */
    for (k2=0; k2<k1 && flagSkipVar==0; k2++) {
      if (DdiInterpRegular (interpGraph[r].clauseArray[i]) == 
        DdiInterpRegular (interpGraph[index].clauseArray[k2])) {
        varN--;
        flagSkipVar = 1;
      }
    }
    if (flagSkipVar==0) {
      interpGraph[index].clauseArray[j] = interpGraph[r].clauseArray[i];
      j++;
    }
  }

  interpGraph[index].clauseArrayN = varN;

  return;
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static void
DdiInterpCOISet (
  ddiInterpGraphMgr_t *interpMgr,
  int index,
  int *delNP
  )
{
  int i, j, is, id, rootNew;
  int *indexRemap;

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpCOISet#");
  fflush (stdout);
#endif

  indexRemap = Pdtutil_Alloc (int, (interpMgr->interpGraphIndex+1));
  if (indexRemap == NULL){
    Pdtutil_Warning (1, "Memory Allocation Error to Create the Remap Array.");
    exit (1);
  }
  for (i=0; i<=interpMgr->interpGraphIndex; i++) {
    indexRemap[i] = DDI_INTERP_NULL_NODE_INDEX;
  }

  rootNew = DdiInterpCOISetRecur (interpMgr, DdiInterpGetRoot (interpMgr),
    delNP, indexRemap);
  if (rootNew != DdiInterpGetRoot (interpMgr)) {
    interpMgr->interpGraphRoot = rootNew;
  }

  /* Conver Support and Pivot Variables */
  /* @@@ */
  for (i=0; i<=interpMgr->interpGraphIndex; i++) {
    if (indexRemap[i] != DDI_INTERP_NULL_NODE_INDEX &&
        DdiInterpRegular (indexRemap[i]) != DDI_INTERP_ZERO_NODE_INDEX) {
      fprintf (stdout, "Remapped Node: %d -> %d\n", i, indexRemap[i]);
      id = i;
      is = DdiInterpRegular (indexRemap[i]);
      // in i copia pivot e arrayN e arrays di indexRemap[i]
      interpMgr->interpGraph[id].resolvedVar = interpMgr->interpGraph[is].resolvedVar;
      Pdtutil_Free (interpMgr->interpGraph[id].clauseArray);
      interpMgr->interpGraph[id].clauseArray = Pdtutil_Alloc (int,
        interpMgr->interpGraph[is].clauseArrayN);
      for (j=0; j<interpMgr->interpGraph[is].clauseArrayN; j++) {
        interpMgr->interpGraph[id].clauseArray[j] = interpMgr->interpGraph[is].clauseArray[j];
      }

      interpMgr->interpGraph[id].clauseArrayN = interpMgr->interpGraph[is].clauseArrayN;
    }
  }

  Pdtutil_Free (indexRemap);

  return;
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static void
DdiInterpCOIStats (
  ddiInterpGraphMgr_t *interpMgr,
  int index,
  int *constNP,
  int *varNP,
  int *clauseNP,
  int *andNP,
  int *orNP
  )
{
  int leftReg, rightReg;

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpCOIStats#");
  fflush (stdout);
#endif

  if (interpMgr->interpGraph[index].flag == 1) {
    return;
  }

  switch (interpMgr->interpGraph[index].op) {
    case Ddi_InterpGraph_Zero:
    case Ddi_InterpGraph_One:
      *constNP = *constNP + 1;
      break;
    case Ddi_InterpGraph_Var:
      *varNP = *varNP + 1;
      break;
    case Ddi_InterpGraph_Clause:
      *clauseNP = *clauseNP + 1;
      break;
    case Ddi_InterpGraph_And:
      *andNP = *andNP + 1;
      break;
    case Ddi_InterpGraph_Or:
      *orNP = *orNP + 1;
      break;
    default:
      Pdtutil_Assert (0, "Wrong Node Type.");
      break;
  }

  interpMgr->interpGraph[index].flag = 1;

  /* Terminal Node? - Return it */
  if (interpMgr->interpGraph[index].op == Ddi_InterpGraph_Zero ||
      interpMgr->interpGraph[index].op == Ddi_InterpGraph_One ||
      interpMgr->interpGraph[index].op == Ddi_InterpGraph_Var ||
      interpMgr->interpGraph[index].op == Ddi_InterpGraph_Clause
  ) {
    return;
  }

  leftReg = DdiInterpRegular (interpMgr->interpGraph[index].left);
  rightReg = DdiInterpRegular (interpMgr->interpGraph[index].right);

  DdiInterpCOIStats (interpMgr, leftReg, constNP, varNP, clauseNP,
    andNP, orNP);
  DdiInterpCOIStats (interpMgr, rightReg, constNP, varNP, clauseNP,
    andNP, orNP);

  return;
}

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []

******************************************************************************/

static int
DdiInterpCOISetRecur (
  ddiInterpGraphMgr_t *interpMgr,
  int index,
  int *delNP,
  int *indexRemap
  )
{
  int leftSign, leftReg, rightSign, rightReg;
  int leftNew, leftNewSign, leftNewReg, rightNew, rightNewSign, rightNewReg;

#if DDI_INTERP_DEBUG
  fprintf (stdout, "#DdiInterpCOISetRecur#");
  fflush (stdout);
#endif

  Pdtutil_Assert (interpMgr->interpGraph[index].op!=Ddi_InterpGraph_Null,
    "Null Node Reach in Fanout COI Set Recur.");

  /* Alredy Remapped Node? - Return it */
  if (indexRemap[index] != DDI_INTERP_NULL_NODE_INDEX) {
    interpMgr->interpGraph[DdiInterpRegular (indexRemap[index])].fanout++;
    return (indexRemap[index]);
  }

  /* Alredy Reached Node? - Return it */
  if (interpMgr->interpGraph[index].fanout > 0) {
    interpMgr->interpGraph[index].fanout++;
    return (index);
  }

  /* Terminal Node? - Return it */
  if (interpMgr->interpGraph[index].op == Ddi_InterpGraph_Zero ||
      interpMgr->interpGraph[index].op == Ddi_InterpGraph_One ||
      interpMgr->interpGraph[index].op == Ddi_InterpGraph_Var ||
      interpMgr->interpGraph[index].op == Ddi_InterpGraph_Clause
  ) {
    interpMgr->interpGraph[index].fanout++;
    return (index);
  }

  /* Not Visited / Not Mapped / Not Terminal Node */
  Pdtutil_Assert (
    (interpMgr->interpGraph[index].op==Ddi_InterpGraph_And ||
     interpMgr->interpGraph[index].op==Ddi_InterpGraph_Or),
    "Wrong Node Type.");

  leftSign = DdiInterpIsInverted (interpMgr->interpGraph[index].left);
  leftReg = DdiInterpRegular (interpMgr->interpGraph[index].left);
  leftNew = DdiInterpCOISetRecur (interpMgr, leftReg, delNP, indexRemap);
  leftNewSign = DdiInterpIsInverted (leftNew);
  leftNewReg = DdiInterpRegular (leftNew);

  rightSign = DdiInterpIsInverted (interpMgr->interpGraph[index].right);
  rightReg = DdiInterpRegular (interpMgr->interpGraph[index].right);
  rightNew = DdiInterpCOISetRecur (interpMgr, rightReg, delNP, indexRemap);
  rightNewSign = DdiInterpIsInverted (rightNew);
  rightNewReg = DdiInterpRegular (rightNew);

  /*
   *  Perform BCP
   */

#if PERFORM_BCP
  if (leftNewReg == DDI_INTERP_ZERO_NODE_INDEX) {
    *delNP = *delNP + 1;
    if ((leftSign==1 && leftNewSign==0) || (leftSign==0 && leftNewSign==1)) {
      /* 1 as left child */
      if (interpMgr->interpGraph[index].op==Ddi_InterpGraph_And) {
        /* 1 and x => x */
        indexRemap[index] = rightSign ? (-rightNew) : (rightNew);
        DdiInterpDeref (interpMgr, leftNewReg);

        //printf ("[#1*x=x#(%d)  -  gate(%d) - FROM %d and %d TO %d and %d = %d)]\n",
	//  *delNP, index,
        //  leftSign?(-leftReg):(leftReg), rightSign?(-rightReg):(rightReg),
        //  leftSign?(-leftNew):(leftNew), rightSign?(-rightNew):(rightNew),
        //  indexRemap[index]);
      } else {
        /* 1 or x => 1 */
        indexRemap[index] = (-DDI_INTERP_ZERO_NODE_INDEX);
        DdiInterpDeref (interpMgr, rightNewReg);

        // printf ("[#1+x=x#(%d)  -  gate(%d) - FROM %d or  %d TO %d or  %d = %d)]\n",
        //  *delNP, index,
        //  leftSign?(-leftReg):(leftReg), rightSign?(-rightReg):(rightReg),
        //  leftSign?(-leftNew):(leftNew), rightSign?(-rightNew):(rightNew),
        //  indexRemap[index]);
      }
    } else {
      /* 0 as left child */
      if (interpMgr->interpGraph[index].op==Ddi_InterpGraph_And) {
        /* 0 and x => 0 */
        indexRemap[index] = DDI_INTERP_ZERO_NODE_INDEX;
        DdiInterpDeref (interpMgr, rightNewReg);

        // printf ("[#0*x=0#(%d)  -  gate(%d) - FROM %d and %d TO %d and %d = %d)]\n",
        //  *delNP, index,
        //  leftSign?(-leftReg):(leftReg), rightSign?(-rightReg):(rightReg),
        //  leftSign?(-leftNew):(leftNew), rightSign?(-rightNew):(rightNew),
        //  indexRemap[index]);
      } else {
        /* 0 or x => x */
        indexRemap[index] = rightSign ? (-rightNew) : (rightNew);
        DdiInterpDeref (interpMgr, leftNewReg);

        // printf ("[#0+x=x#(%d)  -  gate(%d) - FROM %d or  %d TO %d or  %d = %d)]\n",
        //  *delNP, index,
        //  leftSign?(-leftReg):(leftReg), rightSign?(-rightReg):(rightReg),
        //  leftSign?(-leftNew):(leftNew), rightSign?(-rightNew):(rightNew),
        //  indexRemap[index]);
      }
    }

    return (indexRemap[index]);
  }

  if (rightNewReg == DDI_INTERP_ZERO_NODE_INDEX) {
    *delNP = *delNP + 1;
    if ((rightSign==1 && rightNewSign==0) || (rightSign==0 && rightNewSign==1)) {
      /* 1 as right child */
      if (interpMgr->interpGraph[index].op==Ddi_InterpGraph_And) {
        /* x and 1 => x */
        indexRemap[index] = leftSign ? (-leftNew) : (leftNew);
        DdiInterpDeref (interpMgr, rightNewReg);

        // printf ("[#x*1=x#(%d)  -  gate(%d) - FROM %d and %d TO %d and %d = %d)]\n",
        //  *delNP, index,
        //  leftSign?(-leftReg):(leftReg), rightSign?(-rightReg):(rightReg),
        //  leftSign?(-leftNew):(leftNew), rightSign?(-rightNew):(rightNew),
        //  indexRemap[index]);
      } else {
        /* x or 1 => 1 */
        indexRemap[index] = (-DDI_INTERP_ZERO_NODE_INDEX);
        DdiInterpDeref (interpMgr, leftNewReg);

        // printf ("[#x+1=1#(%d)  -  gate(%d) - FROM %d or  %d TO %d or  %d = %d)]\n",
        //  *delNP, index,
        //  leftSign?(-leftReg):(leftReg), rightSign?(-rightReg):(rightReg),
        //  leftSign?(-leftNew):(leftNew), rightSign?(-rightNew):(rightNew),
        //  indexRemap[index]);
      }
    } else {
      /* 0 as right child */
      if (interpMgr->interpGraph[index].op==Ddi_InterpGraph_And) {
        /* x and 0 => 0 */
        indexRemap[index] = DDI_INTERP_ZERO_NODE_INDEX;
        DdiInterpDeref (interpMgr, leftNewReg);

        // printf ("[#x*0=0#(%d)  -  gate(%d) - FROM %d and %d TO %d and %d = %d)]\n",
        //  *delNP, index,
        //  leftSign?(-leftReg):(leftReg), rightSign?(-rightReg):(rightReg),
        //  leftSign?(-leftNew):(leftNew), rightSign?(-rightNew):(rightNew),
        //  indexRemap[index]);
      } else {
        /* x or 0 => x */
        indexRemap[index] = leftSign ? (-leftNew) : (leftNew);
        DdiInterpDeref (interpMgr, rightNewReg);

        // printf ("[#x+0=x#(%d)  -  gate(%d) - FROM %d or  %d TO %d or  %d = %d)]\n",
        //  *delNP, index,
        //  leftSign?(-leftReg):(leftReg), rightSign?(-rightReg):(rightReg),
        //  leftSign?(-leftNew):(leftNew), rightSign?(-rightNew):(rightNew),
        //  indexRemap[index]);
      }
    }

    return (indexRemap[index]);
  }
#endif

  /* Rejoin Children on Nodes Not Deleted */
  interpMgr->interpGraph[index].fanout++;

  if (leftSign) {
    interpMgr->interpGraph[index].left = (-leftNew);
  } else {
    interpMgr->interpGraph[index].left = leftNew;
  }
  if (rightSign) {
    interpMgr->interpGraph[index].right = (-rightNew);
  } else {
    interpMgr->interpGraph[index].right = rightNew;
  }

  return (index);
}

/**Function********************************************************************
  Synopsis    []
  Description [Converts an integer into a string]
  SideEffects []
  SeeAlso     []
******************************************************************************/

static char *
DdiInt2Str ( 
  int i
  )
{
  unsigned int mod, len;
  char *s;

  if (i == 0)
    len = 1;
  else {
    if (i < 0) {
      len = 1;
      mod = -i;
    }
    else {
      len = 0;
      mod = i;
    }


    len += (unsigned) floor (log10(mod)) + 1;
  }

  s = Pdtutil_Alloc (char, len + 1);
  sprintf (s, "%d", i);

  return s;
}








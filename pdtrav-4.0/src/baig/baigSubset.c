/**CFile***********************************************************************

   FileName    [baigSubset.c]

   PackageName [baig]

   Synopsis    [Routines to compute a subset of a bAig.]

   Author      [Sergio Nocco]

   Copyright [ This file was created at the University of Colorado at
   Boulder.  The University of Colorado at Boulder makes no warranty
   about the suitability of this software for any purpose.  It is
   presented on an AS IS basis.]


*******************************************************************************/

#include "baigInt.h"
#include <assert.h>

//static char rcsid[] = "$Id: baigSubset.c,v 1.1.1.1 2021/09/11 09:57:46 cabodi Exp $";

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

typedef struct info_s info_t;
typedef struct queue_s queue_t;
typedef struct array_s array_t;

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

/**Struct**********************************************************************
  Synopsis    [Structure for information about a bAig.]
  Description []
******************************************************************************/
struct info_s {
   array_t    *postOrderNodes;
   array_t   **fanout;
   int        *maxHeight;
   int        *minHeight;
   int        *maxDepth;
   int        *minDepth;
   long       *regPaths;
   long       *invPaths;
   int        *cc0;
   int        *cc1;
   double     *p1;
   array_t    *uniPhase;
   array_t    *dualPhase;
   bAigEdge_t *regSet;
   bAigEdge_t *invSet;
   array_t    *pivars;

   queue_t    *head;
   queue_t    *tail;
   bAigEdge_t *heap;
   int         heapSize;
   int         heapNum;

   int aigSize;
   int sumsh;
   int maxsh;
   int sumlh;
   int maxlh;
   int sumsd;
   int maxsd;
   int sumld;
   int maxld;
   int sumf;
   int maxf;
   int sum0cc;
   int max0cc;
   int sum1cc;
   int max1cc;
   double sum1p;
   double max1p;
   float sump;
   float maxp;
   int sumpl;
   int maxpl;

   int *cutNodes;
   int cutMethod;
   int cutLenght;
   double cutError;
   double *errorVal;
   int *lenghtNum;
   int lenghtSize;

   int *fanCnt;
   int fanSize;
};

/**Struct**********************************************************************
  Synopsis    [Structure used for BFS visit.]
  Description []
******************************************************************************/
struct queue_s {
   bAigEdge_t node;
   queue_t *next;
};

/**Struct**********************************************************************
  Synopsis    [Dynamic array.]
  Description []
******************************************************************************/
struct array_s {
   bAigEdge_t *array;
   int num;
   int size;
};


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**Macro***********************************************************************
  Synopsis     [Returns the max height value.]
  Description  [This macro returns the max height value of the given node.]
  SideEffects  []
  SeeAlso      []
******************************************************************************/

#define maxHeight(manager, info, edge) ( \
info->maxHeight[bAig_AuxInt(manager, edge)]\
)

/**Macro***********************************************************************
  Synopsis     [Returns the min height value.]
  Description  [This macro returns the min height value of the given node.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

#define minHeight(manager, info, edge) ( \
info->minHeight[bAig_AuxInt(manager, edge)]\
)

/**Macro***********************************************************************
  Synopsis     [Returns the max depth value.]
  Description  [This macro returns the max depth value of the given node.]
  SideEffects  []
  SeeAlso      []
******************************************************************************/

#define maxDepth(manager, info, edge) ( \
info->maxDepth[bAig_AuxInt(manager, edge)]\
)

/**Macro***********************************************************************
  Synopsis     [Returns the min depth value.]
  Description  [This macro returns the min depth value of the given node.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

#define minDepth(manager, info, edge) ( \
info->minDepth[bAig_AuxInt(manager, edge)]\
)

/**Macro***********************************************************************
  Synopsis     [Returns the regular paths value.]
  Description  [This macro returns the regular paths value of the given node.]
  SideEffects  []
  SeeAlso      []
******************************************************************************/

#define regPaths(manager, info, edge) ( \
info->regPaths[bAig_AuxInt(manager, edge)]\
)

/**Macro***********************************************************************
  Synopsis     [Returns the inverted paths value.]
  Description  [This macro returns the inverted paths value of the given node.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

#define invPaths(manager, info, edge) ( \
info->invPaths[bAig_AuxInt(manager, edge)]\
)

/**Macro***********************************************************************
  Synopsis     [Returns the regular subset.]
  Description  [This macro returns the regular subset of the given node.]
  SideEffects  []
  SeeAlso      []
******************************************************************************/

#define regSet(manager, info, edge) ( \
bAig_AuxAig0(manager, edge)\
)
/* info->regSet[bAig_AuxInt(manager, edge)]\ */

/**Macro***********************************************************************
  Synopsis     [Returns the regular superset.]
  Description  [This macro returns the regular superset of the given node.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

#define invSet(manager, info, edge) ( \
bAig_AuxAig1(manager, edge)\
)
/* info->invSet[bAig_AuxInt(manager, edge)]\ */

/**Macro***********************************************************************
  Synopsis     [Returns the fanout array.]
  Description  [This macro returns the fanout array of the given node.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

#define fanout(manager, info, edge) ( \
info->fanout[bAig_AuxInt(manager, edge)]\
)

/**Macro***********************************************************************
  Synopsis     [Returns the regular subset.]
  Description  [This macro returns the regular subset of the given node.]
  SideEffects  []
  SeeAlso      []
******************************************************************************/

#define cc0(manager, info, edge) ( \
info->cc0[bAig_AuxInt(manager, edge)]\
)

/**Macro***********************************************************************
  Synopsis     [Returns the regular superset.]
  Description  [This macro returns the regular superset of the given node.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

#define cc1(manager, info, edge) ( \
info->cc1[bAig_AuxInt(manager, edge)]\
)

/**Macro***********************************************************************
  Synopsis     [Returns the fanout array.]
  Description  [This macro returns the fanout array of the given node.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

#define p1(manager, info, edge) ( \
info->p1[bAig_AuxInt(manager, edge)]\
)

/**Macro***********************************************************************
  Synopsis     [Returns the uni-phase value.]
  Description  [This macro returns the uni-phase value of the given node.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

#define uniPhaseNode(manager, info, edge) ( \
(regPaths(manager, info, edge) == 0) || (invPaths(manager, info, edge) == 0) \
)

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static info_t *infoInit();
static void infoQuit(bAig_Manager_t *manager, info_t *info);
static void postOrderVisit(bAig_Manager_t *manager, bAigEdge_t nodeIndex, info_t *info);
static void heightCount(bAig_Manager_t *manager, info_t *info);
static void minDepthCount(bAig_Manager_t *manager, bAigEdge_t nodeIndex, info_t *info);
static void maxDepthCount(bAig_Manager_t *manager, bAigEdge_t nodeIndex, info_t *info);
static void pathCount(bAig_Manager_t *manager, bAigEdge_t nodeIndex, info_t *info);
static void controllabilityCount(bAig_Manager_t *manager, bAigEdge_t nodeIndex, info_t *info);
static void fanoutCount(bAig_Manager_t *manager, bAigEdge_t nodeIndex, info_t *info);
static void phaseCount(bAig_Manager_t *manager, bAigEdge_t nodeIndex, info_t *info);
static void infoFinalize(info_t *info, int threshold);
static void infoPrint(info_t *info, int threshold);
static bAigEdge_t buildSubset(bAig_Manager_t *manager, bAigEdge_t nodeIndex, info_t *info);

static array_t *arrayInsert(array_t *array, bAigEdge_t nodeIndex);
static array_t *arrayFree(array_t *array);
static void queueInsert(info_t *info, bAigEdge_t nodeIndex);
static bAigEdge_t queueExtract(info_t *info);
static void heapInsert(bAig_Manager_t *manager, info_t *info, bAigEdge_t nodeIndex);
static bAigEdge_t heapExtract(bAig_Manager_t *manager, info_t *info);
static void heapify(bAig_Manager_t *manager, info_t *info, int i);
static void heapPrint(bAig_Manager_t *manager, info_t *info);
static void quickSort(double *array, int left, int right);
static void swap(double *array, int i, int j);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Print some statistics about an aig.]
  Description [Print some statistics about an aig.
               Return 0 if constant, 1 otherwise.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
bAig_PrintStats (
    bAig_Manager_t *manager,
    bAigEdge_t nodeIndex
)
{
   info_t *info = NULL;

   if (nodeIndex == bAig_Zero || nodeIndex == bAig_One) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Constant function.\n")
      );
      return 0;
   }

   info = infoInit();
   postOrderVisit(manager, nodeIndex, info);  /* set */
   info->aigSize = info->postOrderNodes->num;
   heightCount(manager, info);
   minDepthCount(manager, nodeIndex, info);   /* reset */
   maxDepthCount(manager, nodeIndex, info);   /* set */
   pathCount(manager, nodeIndex, info);       /* reset */
   controllabilityCount(manager, nodeIndex, info);
   fanoutCount(manager, nodeIndex, info);
   phaseCount(manager, nodeIndex, info);      /* set */

   infoFinalize(info, 0);
   infoPrint(info, 0);
   infoQuit(manager, info);                   /* reset */

   return 1;
}


/**Function********************************************************************
  Synopsis    [Print some statistics about an aig.]
  Description [Print some statistics about an aig.
               Return 0 if constant, 1 otherwise.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
bAig_PrintNodeStats (
    bAig_Manager_t *manager,
    bAigEdge_t nodeIndex
)
{
   info_t *info = NULL;
   int i;
   double pe;

   if (nodeIndex == bAig_Zero || nodeIndex == bAig_One) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Constant function!\n")
      );
      return 0;
   }

   info = infoInit();
   postOrderVisit(manager, nodeIndex, info);  /* set */
   info->aigSize = info->postOrderNodes->num;
   heightCount(manager, info);
   minDepthCount(manager, nodeIndex, info);   /* reset */
   maxDepthCount(manager, nodeIndex, info);   /* set */
   pathCount(manager, nodeIndex, info);       /* reset */
   controllabilityCount(manager, nodeIndex, info);
   fanoutCount(manager, nodeIndex, info);
   phaseCount(manager, nodeIndex, info);      /* set */

   infoFinalize(info, 0);
   infoPrint(info, 0);
   Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
     Pdtutil_VerbLevelNone_c,
     fprintf(stdout, "-----------------------------------------.\n");
     fprintf(stdout, "     Id    H      D         P         C      P1   FO    VM   PE.\n")
   );
   for (i=0; i<info->aigSize; i++) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Node %3d: ", i)
      );
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "%2d/%-2d  ", info->minHeight[i], info->maxHeight[i])
      );
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "%2d/%-2d  ", info->minDepth[i], info->maxDepth[i])
      );
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "%5ld/%-5ld ", info->regPaths[i], info->invPaths[i])
      );
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "%3d/%-3d  ", info->cc1[i], info->cc0[i])
      );
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "%.2f  %2d    ", info->p1[i], info->fanout[i]->num)
      );
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "%c", bAig_isVarNode(manager, info->postOrderNodes->array[i]) ? 'x' : ' ')
      );
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "%c", uniPhaseNode(manager, info, info->postOrderNodes->array[i]) ? 'x' : ' ')
      );
      pe = info->regPaths[i]*info->p1[i] + info->invPaths[i]*(1-info->p1[i]);
      pe /= (info->maxDepth[i]);
      pe /= ((double)info->regPaths[i]+info->invPaths[i]);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, " %.5f\n", pe)
      );
   }

   infoQuit(manager, info);                   /* reset */

   return 1;
}


/**Function********************************************************************
  Synopsis    [Return a bAig representing a subset of the given one.]
  Description [Return a bAig representing a subset of the given one.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
bAigEdge_t
bAig_Subset (
    bAig_Manager_t *manager,
    bAigEdge_t nodeIndex,
    int threshold,
    int verbose
)
{
   info_t *info = NULL;
   bAigEdge_t subset = nodeIndex;

   if (nodeIndex == bAig_Zero || nodeIndex == bAig_One) {
      if (verbose)
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, "Constant function.\n")
       );
      return bAig_Zero;
   }
   if (threshold <= 0) {
      threshold = -1;
      if (!verbose)
       return bAig_Zero;
   }

   info = infoInit();
   info->cutMethod = 1;
   postOrderVisit(manager, nodeIndex, info);  /* set */
   info->aigSize = info->postOrderNodes->num;
   if ((info->aigSize <= threshold) && (!verbose)) {
      infoQuit(manager, info);                /* reset */
      return nodeIndex;
   }

   heightCount(manager, info);
   minDepthCount(manager, nodeIndex, info);   /* reset */
   if (1||verbose) {
      maxDepthCount(manager, nodeIndex, info);/* set */
      pathCount(manager, nodeIndex, info);    /* reset */
      controllabilityCount(manager, nodeIndex, info);
      fanoutCount(manager, nodeIndex, info);
      phaseCount(manager, nodeIndex, info);   /* set */
   }
   infoFinalize(info, threshold);
   if (verbose)
      infoPrint(info, threshold);
   if (info->aigSize > threshold)
      subset = buildSubset(manager, nodeIndex, info);
   bAig_Ref(manager, subset);
   infoQuit(manager, info);                   /* reset */
   bAig_Deref(manager, subset);

   return subset;
}

/**Function********************************************************************
  Synopsis    [Return a bAig representing a subset of the given one.]
  Description [Return a bAig representing a subset of the given one.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
bAigEdge_t
bAig_SubsetNode (
    bAig_Manager_t *manager,
    bAigEdge_t node,
    int subIndex,
    int verbose
)
{
   info_t *info = NULL;
   bAigEdge_t subset = node;

   if (node == bAig_Zero || node == bAig_One) {
      if (verbose)
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, "Constant function.\n")
       );
      return bAig_Zero;
   }

   info = infoInit();
   postOrderVisit(manager, node, info);  /* set */
   info->aigSize = info->postOrderNodes->num;

   info->cutNodes = (int *)calloc(info->postOrderNodes->num, sizeof(int));
   info->cutNodes[subIndex] = 1;
   subset = buildSubset(manager, node, info);
   bAig_Ref(manager, subset);
   infoQuit(manager, info);              /* reset */
   bAig_Deref(manager, subset);

   return subset;
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Initializes the info structure.]
  Description [Initializes the info structure.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static info_t *
infoInit(
)
{
   info_t *info = NULL;
   int i;

   info = (info_t *)malloc(sizeof(info_t));
   if (info == NULL) return NULL;

   info->postOrderNodes = info->pivars = NULL;
   info->maxHeight = info->minHeight = NULL;
   info->maxDepth = info->minDepth = NULL;
   info->regPaths = info->invPaths = NULL;
   info->uniPhase = info->dualPhase = NULL;
   info->regSet = info->invSet = NULL;
   info->cc0 = info->cc1 = NULL;
   info->fanout = NULL; info->p1 = NULL;
   info->errorVal = NULL; info->cutNodes = NULL;

   info->head = info->tail = NULL;
   info->heapSize = 128; info->heapNum = 0;
   info->heap = (bAigEdge_t *)malloc(info->heapSize*sizeof(bAigEdge_t));
   for (i=0; i<info->heapSize; i++) info->heap[i] = bAig_NULL;

   info->sumsh = info->maxsh = info->sumlh = info->maxlh = 0;
   info->sumsd = info->maxsd = info->sumld = info->maxld = 0;
   info->sumf = info->maxf = info->aigSize = 0;
   info->sumpl = info->maxpl = (int) (info->sump = info->maxp = 0);
   info->sum0cc = info->max0cc = info->sum1cc = info->max1cc = 0;
   info->sum1p = info->max1p = info->cutError = 0;
   info->cutLenght = info->cutMethod = 0;

   info->fanSize = info->lenghtSize = 128;
   info->fanCnt = (int *)malloc(info->fanSize*sizeof(int));
   info->lenghtNum = (int *)malloc(info->lenghtSize*sizeof(int));
   for (i=0; i<info->fanSize; i++) info->lenghtNum[i] = info->fanCnt[i] = 0;

   return info;
}

/**Function********************************************************************
  Synopsis    [Quit the info structure.]
  Description [Quit the info structure.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
infoQuit(
   bAig_Manager_t *manager,
   info_t *info
)
{
   bAigEdge_t baig;
   int i;

   if (!info || !info->postOrderNodes) return;
   for (i=0; i<info->postOrderNodes->num; i++) {
      baig = info->postOrderNodes->array[i];
      bAig_AuxInt(manager, baig) = -1;
      nodeClearVisited(manager, baig);

      if (regSet(manager, info, baig) != bAig_NULL)
       bAig_RecursiveDeref(manager, regSet(manager, info, baig));
      regSet(manager, info, baig) = bAig_NULL;

      if (invSet(manager, info, baig) != bAig_NULL)
       bAig_RecursiveDeref(manager, invSet(manager, info, baig));
      invSet(manager, info, baig) = bAig_NULL;
   }
   arrayFree(info->postOrderNodes);

   if (info->fanout) {
      for (i=0; i<info->aigSize; i++)
       arrayFree(info->fanout[i]);
      Pdtutil_Free(info->fanout);
   }

   Pdtutil_Free(info->maxHeight);
   Pdtutil_Free(info->minHeight);
   Pdtutil_Free(info->maxDepth);
   Pdtutil_Free(info->minDepth);
   Pdtutil_Free(info->regPaths);
   Pdtutil_Free(info->invPaths);
   Pdtutil_Free(info->regSet);
   Pdtutil_Free(info->invSet);
   Pdtutil_Free(info->cc0);
   Pdtutil_Free(info->cc1);
   Pdtutil_Free(info->p1);

   arrayFree(info->uniPhase);
   arrayFree(info->dualPhase);
   arrayFree(info->pivars);

   Pdtutil_Free(info->heap);
   Pdtutil_Free(info->fanCnt);
   Pdtutil_Free(info->lenghtNum);
   Pdtutil_Free(info->errorVal);
   Pdtutil_Free(info->cutNodes);
   Pdtutil_Free(info);
}

/**Function********************************************************************
  Synopsis    [Visit a bAig in post-order.]
  Description [Visit a bAig in post-order: nodes are saved into the "postOrder"
               array; variables are saved into the "pivars" array.]
  SideEffects [Set the visited flag]
  SeeAlso     []
******************************************************************************/
static void
postOrderVisit(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex,
   info_t *info
)
{
   if (nodeIndex == bAig_Zero || nodeIndex == bAig_One) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Constant Value Reached.\n")
      );
      return;
   }

   if (nodeVisited(manager, nodeIndex)) {
      return;
   }
   nodeSetVisited(manager, nodeIndex);

   if (bAig_isVarNode(manager, nodeIndex)) {
      info->postOrderNodes = arrayInsert(info->postOrderNodes, nodeIndex);
      bAig_AuxInt(manager, nodeIndex) = info->postOrderNodes->num-1;

      info->pivars = arrayInsert(info->pivars, nodeIndex);
      return;
   }

   postOrderVisit(manager, rightChild(manager,nodeIndex), info);
   postOrderVisit(manager, leftChild(manager,nodeIndex), info);

   info->postOrderNodes = arrayInsert(info->postOrderNodes, nodeIndex);
   bAig_AuxInt(manager, nodeIndex) = info->postOrderNodes->num-1;

   assert(bAig_AuxAig0(manager, nodeIndex) == bAig_NULL);
   assert(bAig_AuxAig1(manager, nodeIndex) == bAig_NULL);
}

/**Function********************************************************************
  Synopsis    [Count the height of the nodes belonging to a bAig.]
  Description [Count the min/max height of each node belonging to a bAig.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
heightCount(
    bAig_Manager_t *manager,
    info_t *info)
{
  bAigEdge_t node;
  int i, lh, rh;

  info->maxHeight = (int *)malloc(info->postOrderNodes->num*sizeof(int));
  info->minHeight = (int *)malloc(info->postOrderNodes->num*sizeof(int));
  for (i=0; i<info->postOrderNodes->num; i++) {
     info->maxHeight[i] = info->minHeight[i] = 0;
  }

  for (i=0; i<info->postOrderNodes->num; i++) {
     node = info->postOrderNodes->array[i];

     if (bAig_isVarNode(manager, node)) {
      maxHeight(manager, info, node) = 1;
      minHeight(manager, info, node) = 1;
      continue;
     }

     lh = maxHeight(manager, info, leftChild(manager, node));
     rh = maxHeight(manager, info, rightChild(manager, node));
     maxHeight(manager, info, node) = ((rh > lh) ? rh : lh) + 1;

     lh = minHeight(manager, info, leftChild(manager, node));
     rh = minHeight(manager, info, rightChild(manager, node));
     minHeight(manager, info, node) = ((rh < lh) ? rh : lh) + 1;
  }
}

/**Function********************************************************************
  Synopsis    [Compute the min depth for each node.]
  Description [Compute the min depth for each node by a BFS visit.]
  SideEffects [Reset the visited flag]
  SeeAlso     []
******************************************************************************/
static void
minDepthCount(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex,
   info_t *info)
{
   bAigEdge_t node, lc, rc;
   int i;

   info->minDepth = (int *)malloc(info->postOrderNodes->num*sizeof(int));
   for (i=0; i<info->postOrderNodes->num; i++) {
      info->minDepth[i] = 0;
   }

   nodeClearVisited(manager, nodeIndex);
   minDepth(manager, info, nodeIndex) = 1;
   if (bAig_isVarNode(manager, nodeIndex))
      return;

   queueInsert(info, nodeIndex);
   while ((node=queueExtract(info)) != bAig_NULL) {

      lc = leftChild(manager, node);
      if (nodeVisited(manager, lc)) {
       nodeClearVisited(manager, lc);
       minDepth(manager, info, lc) = minDepth(manager, info, node)+1;
       if (!bAig_isVarNode(manager, lc))
          queueInsert(info, lc);
      }

      rc = rightChild(manager, node);
      if (nodeVisited(manager, rc)) {
       nodeClearVisited(manager, rc);
       minDepth(manager, info, rc) = minDepth(manager, info, node)+1;
       if (!bAig_isVarNode(manager, rc))
          queueInsert(info, rc);
      }
   }

}

/**Function********************************************************************
  Synopsis    [Compute the max depth for each node.]
  Description [Compute the max depth for each node by a prioritized visit.]
  SideEffects [Set the visited flag]
  SeeAlso     []
******************************************************************************/
static void
maxDepthCount(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex,
   info_t *info)
{
   int lc, rc, node, i;

   info->maxDepth = (int *)malloc(info->postOrderNodes->num*sizeof(int));
   for (i=0; i<info->postOrderNodes->num; i++) {
      info->maxDepth[i] = 0;
   }

   nodeSetVisited(manager, nodeIndex);
   maxDepth(manager, info, nodeIndex) = 1;
   if (bAig_isVarNode(manager, nodeIndex))
      return;

   heapInsert(manager, info, nodeIndex);
   while ((node=heapExtract(manager, info)) != bAig_NULL) {

      lc = leftChild(manager, node);
      if (maxDepth(manager, info, lc) <= maxDepth(manager, info, node)) {
       maxDepth(manager, info, lc) = maxDepth(manager, info, node) + 1;
      }
      if (!nodeVisited(manager, lc)) {
       nodeSetVisited(manager, lc);
       if (!bAig_isVarNode(manager, lc))
          heapInsert(manager, info, lc);
      }

      rc = rightChild(manager, node);
      if (maxDepth(manager, info, rc) <= maxDepth(manager, info, node)) {
       maxDepth(manager, info, rc) = maxDepth(manager, info, node) + 1;
      }
      if (!nodeVisited(manager, rc)) {
       nodeSetVisited(manager, rc);
       if (!bAig_isVarNode(manager, rc))
          heapInsert(manager, info, rc);
      }
   }

}

/**Function********************************************************************
  Synopsis    [Count the number of paths to the root for each node of a bAig.]
  Description [Count the number of paths to the root for each node of a bAig.]
  SideEffects [Reset the visited flag]
  SeeAlso     []
******************************************************************************/
static void
pathCount(
    bAig_Manager_t *manager,
    bAigEdge_t nodeIndex,
    info_t *info)
{
   int lc, rc, node, i;

   info->regPaths = (long *)malloc(info->postOrderNodes->num*sizeof(long));
   info->invPaths = (long *)malloc(info->postOrderNodes->num*sizeof(long));
   for (i=0; i<info->postOrderNodes->num; i++) {
      info->regPaths[i] = info->invPaths[i] = 0;
   }

   nodeClearVisited(manager, nodeIndex);
   if (bAig_IsInverted(nodeIndex))
      invPaths(manager, info, nodeIndex) = 1;
   else
      regPaths(manager, info, nodeIndex) = 1;

   if (bAig_isVarNode(manager, nodeIndex))
      return;

   heapInsert(manager, info, nodeIndex);
   while ((node=heapExtract(manager, info)) != bAig_NULL) {
      lc = leftChild(manager, node);
      if (bAig_IsInverted(lc)) {
       invPaths(manager, info, lc) += regPaths(manager, info, node);
       regPaths(manager, info, lc) += invPaths(manager, info, node);
      } else {
       regPaths(manager, info, lc) += regPaths(manager, info, node);
       invPaths(manager, info, lc) += invPaths(manager, info, node);
      }
      if (nodeVisited(manager, lc)) {
       nodeClearVisited(manager, lc);
       if (!bAig_isVarNode(manager, lc))
          heapInsert(manager, info, lc);
      }

      rc = rightChild(manager, node);
      if (bAig_IsInverted(rc)) {
       invPaths(manager, info, rc) += regPaths(manager, info, node);
       regPaths(manager, info, rc) += invPaths(manager, info, node);
      } else {
       regPaths(manager, info, rc) += regPaths(manager, info, node);
       invPaths(manager, info, rc) += invPaths(manager, info, node);
      }
      if (nodeVisited(manager, rc)) {
       nodeClearVisited(manager, rc);
       if (!bAig_isVarNode(manager, rc))
          heapInsert(manager, info, rc);
      }
   }

}

/**Function********************************************************************
  Synopsis    [Count the fanout of the nodes belonging to a bAig.]
  Description [Count the fanout of the nodes belonging to a bAig.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
controllabilityCount(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex,
   info_t *info
)
{
   bAigEdge_t node, lc, rc;
   int i, cl, cr;
   double pl, pr;

   info->cc0 = (int *)malloc(info->postOrderNodes->num*sizeof(int));
   info->cc1 = (int *)malloc(info->postOrderNodes->num*sizeof(int));
   info->p1 = (double *)malloc(info->postOrderNodes->num*sizeof(double));

   for (i=0; i<info->postOrderNodes->num; i++) {
      node = info->postOrderNodes->array[i];

      if (bAig_isVarNode(manager, node)) {
       cc0(manager, info, node) = 1;
       cc1(manager, info, node) = 1;
       p1(manager, info, node) = 0.5;
       continue;
      }

      lc = leftChild(manager, node);
      rc = rightChild(manager, node);

      /* regular 0-controllability */
      cl = bAig_IsInverted(lc) ? cc1(manager, info, lc) : cc0(manager, info, lc);
      cr = bAig_IsInverted(rc) ? cc1(manager, info, rc) : cc0(manager, info, rc);
      cc0(manager, info, node) = (cl < cr ? cl : cr) + 1;

      /* regular 1-controllability */
      cl = bAig_IsInverted(lc) ? cc0(manager, info, lc) : cc1(manager, info, lc);
      cr = bAig_IsInverted(rc) ? cc0(manager, info, rc) : cc1(manager, info, rc);
      cc1(manager, info, node) = cl + cr + 1;

      /* regular 1-probability */
      pl = bAig_IsInverted(lc) ? (1-p1(manager, info, lc)) : p1(manager, info, lc);
      pr = bAig_IsInverted(rc) ? (1-p1(manager, info, rc)) : p1(manager, info, rc);
      p1(manager, info, node) = pl * pr;
      if (p1(manager, info, node) <= 0)
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, " AAAARRRGGHHHH.\n")
       );
   }

}

/**Function********************************************************************
  Synopsis    [Count the fanout of the nodes belonging to a bAig.]
  Description [Count the fanout of the nodes belonging to a bAig.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
fanoutCount(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex,
   info_t *info
)
{
   bAigEdge_t node, lc, rc;
   int i;

   info->fanout = (array_t **)malloc(info->postOrderNodes->num*sizeof(array_t *));
   for (i=0; i<info->postOrderNodes->num; i++) {
      info->fanout[i] = NULL;
   }

   for (i=0; i<info->postOrderNodes->num; i++) {
      node = info->postOrderNodes->array[i];

      if (bAig_isVarNode(manager, node)) {
       continue;
      }

      lc = leftChild(manager, node);
      fanout(manager, info, lc) = arrayInsert(fanout(manager, info, lc), node);

      rc = rightChild(manager, node);
      fanout(manager, info, rc) = arrayInsert(fanout(manager, info, rc), node);
   }

   /* root fanout */
   fanout(manager, info, nodeIndex) = arrayInsert(fanout(manager, info, nodeIndex), nodeIndex);
}

/**Function********************************************************************
  Synopsis    [Count the mono-phased and dual-phased nodes of a bAig.]
  Description [Count the mono-phased and dual-phased nodes of a bAig.]
  SideEffects [Set the visited flag.]
  SeeAlso     []
******************************************************************************/
static void
phaseCount(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex,
   info_t *info)
{
  int lc, rc, node;
  array_t *fanout;
  int i, uniPhase;

  nodeSetVisited(manager,nodeIndex);
  heapInsert(manager, info, nodeIndex);
  while ((node=heapExtract(manager, info)) != bAig_NULL) {

     if (uniPhaseNode(manager, info, node)) {
      info->uniPhase = arrayInsert(info->uniPhase, node);
     } else {
      uniPhase = 1;
      fanout = fanout(manager, info, node);
      for (i=0; uniPhase && i<fanout->num; i++)
         uniPhase &= uniPhaseNode(manager, info, fanout->array[i]);
      if (uniPhase)
         info->dualPhase = arrayInsert(info->dualPhase, node);
     }

     if (bAig_isVarNode(manager, node))
      continue;

     lc = leftChild(manager, node);
     if (!nodeVisited(manager, lc)) {
      nodeSetVisited(manager, lc);
      heapInsert(manager, info, lc);
     }

     rc = rightChild(manager, node);
     if (!nodeVisited(manager, rc)) {
      nodeSetVisited(manager, rc);
      heapInsert(manager, info, rc);
     }
  }

}

/**Function********************************************************************
  Synopsis    [Finalize the information for a bAig.]
  Description [Finalize the information for a bAig: generate printable info;
               generate cut-off lenght for subsetting.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
infoFinalize(
   info_t *info,
   int threshold
)
{
  int i, size, minLen;
  double val;

  for (i=0; i<info->postOrderNodes->num; i++) {

     if (info->cc0) {
      info->sum0cc += info->cc0[i];
      if (info->cc0[i] > info->max0cc)
         info->max0cc = info->cc0[i];
     }
     if (info->cc1) {
      info->sum1cc += info->cc1[i];
      if (info->cc1[i] > info->max1cc)
         info->max1cc = info->cc1[i];
     }
     if (info->p1) {
      info->sum1p += info->p1[i];
      if (info->p1[i] > info->max1p)
         info->max1p = info->p1[i];
     }

     if (info->maxHeight) {
      info->sumlh += info->maxHeight[i];
      if (info->maxHeight[i] > info->maxlh)
         info->maxlh = info->maxHeight[i];
     }
     if (info->minHeight) {
      info->sumsh += info->minHeight[i];
      if (info->minHeight[i] > info->maxsh)
         info->maxsh = info->minHeight[i];
     }

     if (info->maxDepth) {
      info->sumld += info->maxDepth[i];
      if (info->maxDepth[i] > info->maxld)
         info->maxld = info->maxDepth[i];
     }
     if (info->minDepth) {
      info->sumsd += info->minDepth[i];
      if (info->minDepth[i] > info->maxsd)
         info->maxsd = info->minDepth[i];
     }

     if (info->regPaths && info->invPaths) {
      info->sump += info->regPaths[i] + info->invPaths[i];
      if (info->maxp < info->regPaths[i] + info->invPaths[i])
         info->maxp = info->regPaths[i] + info->invPaths[i];
     }

     if (info->fanout) {
      info->sumf += (info->fanout[i] ? info->fanout[i]->num : 0);
      if (info->maxf < (info->fanout[i] ? info->fanout[i]->num : 0))
         info->maxf = (info->fanout[i] ? info->fanout[i]->num : 0);
      /*
      while (info->maxf >= info->fanSize)) {
         info->fanSize *= 2;
         info->fanCnt = (int *)realloc(info->fanCnt, info->fanSize*sizeof(int));
         for (i=info->fanSize/2; i<info->fanSize; i++)
            info->fanCnt[i] = 0;
      }
      info->fanCnt[info->fanout[i] ? info->fanout[i]->num : 0]++;
      */
     }

     if (info->minDepth && info->minHeight) {
      minLen = info->minDepth[i] + info->minHeight[i];
      info->sumpl += minLen;
      if (info->maxpl < minLen)
         info->maxpl = minLen;
      while (minLen >= info->lenghtSize) {
         info->lenghtSize *= 2;
         info->lenghtNum = (int *)realloc(info->lenghtNum, info->lenghtSize*sizeof(int));
         for (i=info->lenghtSize/2; i<info->lenghtSize; i++)
            info->lenghtNum[i] = 0;
      }
      info->lenghtNum[minLen]++;
     }

  }

  if (threshold) {
     info->cutNodes = (int *)malloc(info->postOrderNodes->num*sizeof(int));
     size = info->aigSize;
     switch (info->cutMethod) {
      case 1:  /* probabilistic approach */
               info->errorVal = (double *)malloc(info->postOrderNodes->num*sizeof(double));
             for (i=0; i<info->postOrderNodes->num; i++) {
                val = info->regPaths[i]*info->p1[i] + info->invPaths[i]*(1-info->p1[i]);
                val /= (info->maxDepth[i]);// *info->minDepth[i]);
                val /= ((double)info->regPaths[i]+info->invPaths[i]);
                info->p1[i] = info->errorVal[i] = val;
             }
             quickSort(info->errorVal, 0, info->postOrderNodes->num-1);
             if (threshold < 0)
                info->cutError = 1;
             else if (threshold >= size)
                info->cutError = -1;
             else
                info->cutError = info->errorVal[threshold];
             for (i=0; i<info->postOrderNodes->num; i++) {
                if (info->p1[i] < info->cutError)
                   info->cutNodes[i] = 1;
                else
                   info->cutNodes[i] = 0;
             }
             break;
      default: /* shortest paths */
               info->cutLenght = info->maxpl;
             while (size-info->lenghtNum[info->cutLenght] > threshold)
                size -= info->lenghtNum[info->cutLenght--];
             for (i=0; i<info->postOrderNodes->num; i++) {
                minLen = info->minDepth[i] + info->minHeight[i];
                if (minLen > info->cutLenght)
                   info->cutNodes[i] = 1;
                else
                   info->cutNodes[i] = 0;
             }
     }
  }
}

/**Function********************************************************************
  Synopsis    [Display bAig info.]
  Description [Display bAig info.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
infoPrint(
   info_t *info,
   int threshold
)
{
   int size=info->aigSize;
   //bAigEdge_t node;

   //printf("Size = %d nodes.\n", size);
   Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
     Pdtutil_VerbLevelNone_c,
     fprintf(stdout, "Average Min Height = %.1f (max = %d).\n",
       ((float)info->sumsh)/size, info->maxsh);
     fprintf(stdout, "Average Max Height = %.1f (max = %d).\n",
       ((float)info->sumlh)/size, info->maxlh);
     fprintf(stdout, "Average Min Depth = %.1f (max = %d).\n",
       ((float)info->sumsd)/size, info->maxsd);
     fprintf(stdout, "Average Max Depth = %.1f (max = %d).\n",
       ((float)info->sumld)/size, info->maxld);
     fprintf(stdout, "Average Fan-out = %.1f (max = %d).\n",
       ((float)info->sumf)/size, info->maxf);
     fprintf(stdout, "Average Paths Number = %.1f (max = %.0f).\n",
       info->sump/size, info->maxp);
     fprintf(stdout, "Average Min Path Len = %.1f (max = %d).\n",
       ((float)info->sumpl)/size, info->maxpl)
   );
   if (info->pivars)
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Variable nodes  : %d (%.1f%%).\n", info->pivars->num, (100*(float)info->pivars->num)/size)
      );
   if (info->uniPhase)
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Mono-phase nodes: %d (%.1f%%).\n",
          info->uniPhase->num, (100*(float)info->uniPhase->num)/size)
      );
   if (info->dualPhase)
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Dual-phase nodes: %d (%.1f%%).\n",
          info->dualPhase->num, (100*(float)info->dualPhase->num)/size)
      );
   if (info->p1)
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Average 1-Probability = %.4f (max = %.3f).\n",
          info->sum1p/size, info->max1p)
      );
   if (info->cc0)
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Average 0-Controllability = %.1f (max = %d).\n",
          ((float)info->sum0cc)/size, info->max0cc)
      );
   if (info->cc1)
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Average 1-Controllability = %.1f (max = %d).\n",
          ((float)info->sum1cc)/size, info->max1cc)
      );
   if (threshold) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        switch (info->cutMethod) {
          case 1: fprintf(stdout, "Cut-off Error: %g.\n", info->cutError); break;
          default: fprintf(stdout, "Cut-off Lenght: %d.\n", info->cutLenght+1);
        }
        fprintf(stdout, "Size threshold: %d.\n", threshold)
      );
   }

#if 0
   if (info->cc0) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Levelized 0-Controllability:\n")
      );
      level = 0;
      while (level++ < info->maxsh) {
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, "Level %2d:", level) ncol = 9;
       );
       for (i=0; i<info->postOrderNodes->num; i++)
          if (info->minHeight[i] == level) {
             if (ncol > 80) {printf("         "); ncol = 9;}
             Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
               Pdtutil_VerbLevelNone_c,
               fprintf(stdout, "%2d [%4.1f]", info->cc0[i], ((float)info->cc0[i])/level)
             );
             ncol += 10;
          }
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, "\n")
       );
      }
   }
   if (info->cc1) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Levelized 1-Controllability:\n")
      );
      level = 0;
      while (level++ < info->maxsh) {
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, "Level %2d:", level) ncol = 9;
       );
       for (i=0; i<info->postOrderNodes->num; i++)
          if (info->minHeight[i] == level) {
             if (ncol > 80) {printf("         "); ncol = 9;}
             Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
               Pdtutil_VerbLevelNone_c,
               fprintf(stdout, "%2d [%4.1f]", info->cc1[i], ((float)info->cc1[i])/level)
             );
             ncol += 10;
          }
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, "\n")
       );
      }
   }
   if (info->p1) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Levelized 1-Probability:\n")
      );
      level = 0;
      while (level++ < info->maxsd) {
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, "Level %2d:", level) ncol = 9;
       );
       for (i=0; i<info->postOrderNodes->num; i++)
          if (info->minDepth[i] == level) {
             if (ncol > 80) {printf("         "); ncol = 9;}
             Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
               Pdtutil_VerbLevelNone_c,
               fprintf(stdout, "%.3f [%.3f]", info->p1[i], info->p1[i]/level)
             );
             ncol += 14;
          }
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, "\n")
       );
      }
   }
#endif

   Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
     Pdtutil_VerbLevelDevMax_c,
     int i;
     fprintf(stdout, "Nodes with fan-out=1  : %d (%.1f \%).\n", info->fanCnt[1],
       (100*((float)info->fanCnt[1]))/size);
     fprintf(stdout, "Nodes with fan-out=2  : %d (%.1f \%).\n", info->fanCnt[2],
       (100*((float)info->fanCnt[2]))/size);
     for (i=1; i<100; i++) info->fanCnt[i] += info->fanCnt[i-1];
     fprintf(stdout, "Nodes with fan-out<=4 : %d (%.1f \%).\n", info->fanCnt[4],
       (100*((float)info->fanCnt[4]))/size);
     if (info->fanCnt[4] < size)
       fprintf(stdout, "Nodes with fan-out<=8 : %d (%.1f \%).\n", info->fanCnt[8],
         (100*((float)info->fanCnt[8]))/size);
     if (info->fanCnt[8] < size)
      fprintf(stdout, "Nodes with fan-out<=16: %d (%.1f \%).\n", info->fanCnt[16],
        (100*((float)info->fanCnt[16]))/size);
     if (info->fanCnt[16] < size)
       fprintf(stdout, "Nodes with fan-out<=32: %d (%.1f \%).\n", info->fanCnt[32],
         (100*((float)info->fanCnt[32]))/size);
     if (info->fanCnt[32] < size)
       fprintf(stdout, "Nodes with fan-out<=64: %d (%.1f \%).\n", info->fanCnt[64],
         (100*((float)info->fanCnt[64]))/size)
     );
}

/**Function********************************************************************
  Synopsis    [Build a subset of a bAig.]
  Description [Build a subset of a bAig.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static bAigEdge_t
buildSubset(
    bAig_Manager_t *manager,
    bAigEdge_t nodeIndex,
    info_t *info
)
{
   int i;
   bAigEdge_t node, lc, rc, ls, rs;

#if 0
   info->regSet = (bAigEdge_t *)malloc(info->postOrderNodes->num*sizeof(bAigEdge_t));
   info->invSet = (bAigEdge_t *)malloc(info->postOrderNodes->num*sizeof(bAigEdge_t));
   for (i=0; i<info->postOrderNodes->num; i++) {
      info->regSet[i] = info->invSet[i] = bAig_NULL;
   }
#endif

   for (i=0; i<info->postOrderNodes->num; i++) {
      node = info->postOrderNodes->array[i];

      if (info->cutNodes[i]) {
       regSet(manager, info, node) = bAig_Zero;
       invSet(manager, info, node) = bAig_One;
       bAig_Ref(manager, regSet(manager, info, node));
       bAig_Ref(manager, invSet(manager, info, node));
       continue;
      }

      if (bAig_isVarNode(manager, node)) {
       regSet(manager, info, node) = bAig_NonInvertedEdge(node);
       invSet(manager, info, node) = bAig_NonInvertedEdge(node);
       bAig_Ref(manager, regSet(manager, info, node));
       bAig_Ref(manager, invSet(manager, info, node));
       continue;
      }

      lc = leftChild(manager, node);
      rc = rightChild(manager, node);

      /* regular subset */
      ls = bAig_IsInverted(lc) ? bAig_Not(invSet(manager, info, lc)) :
                                 regSet(manager, info, lc);
      rs = bAig_IsInverted(rc) ? bAig_Not(invSet(manager, info, rc)) :
                                 regSet(manager, info, rc);
      regSet(manager, info, node) = bAig_And(manager, ls, rs);
      bAig_Ref(manager, regSet(manager, info, node));

      /* regular superset */
      ls = bAig_IsInverted(lc) ? bAig_Not(regSet(manager, info, lc)) :
                                 invSet(manager, info, lc);
      rs = bAig_IsInverted(rc) ? bAig_Not(regSet(manager, info, rc)) :
                                 invSet(manager, info, rc);
      invSet(manager, info, node) = bAig_And(manager, ls, rs);
      bAig_Ref(manager, invSet(manager, info, node));
   }

   return (bAig_IsInverted(nodeIndex) ? bAig_Not(invSet(manager, info, nodeIndex)) :
                                      regSet(manager, info, nodeIndex));
}

/**Function********************************************************************
  Synopsis           [Add a bAig node to a list of nodes.]
  Description        [Add a bAig node to a list of nodes.]
  SideEffects        []
  SeeAlso            []
******************************************************************************/
static array_t *
arrayInsert (
  array_t *array,
  bAigEdge_t nodeIndex
)
{
   if (array == NULL) { /* create */
      array = (array_t *)malloc(sizeof(array_t));
      array->size = 2;
      array->array = (bAigEdge_t *)malloc(array->size*sizeof(bAigEdge_t));
      array->num = 1;
      array->array[0] = nodeIndex;
      return array;
   }

   if (array->num >= array->size) { /* resize */
      array->size *= 2;
      array->array = (bAigEdge_t *)realloc(array->array, array->size*sizeof(int));
   }
   array->array[array->num++] = nodeIndex;
   return array;
}

/**Function********************************************************************
  Synopsis           [Free the dynamically allocated memory for an array.]
  Description        [Free the dynamically allocated memory for an array.]
  SideEffects        []
  SeeAlso            []
******************************************************************************/
static array_t *
arrayFree(
  array_t *array
)
{
   if (array != NULL) {
      Pdtutil_Free(array->array);
      Pdtutil_Free(array);
   }
   return NULL;
}

/**Function********************************************************************
  Synopsis    [Insert a node in queue.]
  Description [Insert a node in queue.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
queueInsert(
   info_t *info,
   bAigEdge_t nodeIndex)
{
  queue_t *elem = (queue_t *)malloc(sizeof(queue_t));

  elem->node = nodeIndex;
  elem->next = NULL;

  if (info->head == NULL) {
     info->head = info->tail = elem;
     return;
  }

  info->tail->next = elem;
  info->tail = elem;
}

/**Function********************************************************************
  Synopsis    [Extract a node from queue.]
  Description [Extract a node from queue.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static bAigEdge_t
queueExtract(
   info_t *info
)
{
  queue_t *elem = info->head;
  bAigEdge_t node;

  if (elem == NULL) {
     return bAig_NULL;
  }

  node = elem->node;
  info->head = elem->next;
  if (info->head == NULL)
     info->tail = NULL;

  Pdtutil_Free(elem);
  return node;
}

/**Function********************************************************************
  Synopsis    [Insert a node in heap.]
  Description [Insert a node in heap.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
heapInsert(
   bAig_Manager_t *manager,
   info_t *info,
   bAigEdge_t nodeIndex
)
{
   int i, p;

   if (info->heapNum >= info->heapSize) {
      info->heapSize *= 2;
      info->heap = REALLOC(bAigEdge_t, info->heap, info->heapSize);
   }

   i = info->heapNum++; p = (i-1)/2;
   while ((i>0) && (maxHeight(manager,info,info->heap[p]) < maxHeight(manager,info,nodeIndex))) {
      info->heap[i] = info->heap[p];
      i = p; p = (i-1)/2;
   }
   info->heap[i] = nodeIndex;
}

/**Function********************************************************************
  Synopsis    [Insert a node in heap.]
  Description [Insert a node in heap.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static bAigEdge_t
heapExtract(
   bAig_Manager_t *manager,
   info_t *info
)
{
   bAigEdge_t node;

   if (info->heapNum == 0)
      return bAig_NULL;

   node = info->heap[0];
   if ((--info->heapNum) == 0)
      return node;

   info->heap[0] = info->heap[info->heapNum];
   heapify(manager, info, 0);

   return node;
}

/**Function********************************************************************
  Synopsis    [Heap management routine.]
  Description [Heap management routine.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
heapify(
   bAig_Manager_t *manager,
   info_t *info,
   int i
)
{
   int l=2*i+1, r=2*i+2, key=i;
   bAigEdge_t temp;

   if ((l<info->heapNum) && (maxHeight(manager,info,info->heap[l]) > maxHeight(manager,info,info->heap[key])))
      key = l;
   if ((r<info->heapNum) && (maxHeight(manager,info,info->heap[r]) > maxHeight(manager,info,info->heap[key])))
      key = r;
   if (key != i) {
      temp = info->heap[i];
      info->heap[i] = info->heap[key];
      info->heap[key] = temp;
      heapify(manager, info, key);
   }
}

/**Function********************************************************************
  Synopsis    [Heap management routine.]
  Description [Heap management routine.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
heapPrint (
   bAig_Manager_t *manager,
   info_t *info
)
{
   bAigEdge_t node;
   int i;

   Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
     Pdtutil_VerbLevelNone_c,
     fprintf(stdout, "Data in heap %d.\n", info->heapNum)
   );
   for (i=0; i<info->heapNum; i++) {
      node = info->heap[i];
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "%3d. %d\t", i, (int)node);
        fprintf(stdout, "%ld\t%ld.\n", regPaths(manager, info, node),
          invPaths(manager, info, node))
      );
   }
   Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
     Pdtutil_VerbLevelNone_c,
     fprintf(stdout, "\n")
   );
}

/**Function********************************************************************
  Synopsis    [Quick Sort.]
  Description [Quick Sort.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
quickSort (
   double *array,
   int left,
   int right
)
{
   int i, j;
   double x;

   if (left >= right) return;

   x = array[left];
   i = left-1; j = right+1;

   while (i < j) {
      while (array[--j] < x);
      while (array[++i] > x);

      if (i < j) swap(array, i, j);
   }
   quickSort(array, left, j);
   quickSort(array, j+1, right);
}



/**Function********************************************************************
  Synopsis    [Swap two elements.]
  Description [Swap two elements.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
swap (
   double *array,
   int i,
   int j
)
{
   double tmp = array[i];
   array[i] = array[j];
   array[j] = tmp;
}


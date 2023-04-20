/**CHeaderFile*****************************************************************

  FileName    [baig.h]

  PackageName [baig]

  Synopsis    [Binary AND/INVERTER graph]

  Description [External functions and data strucures of the binary AND/INVERTER graph package]

  SeeAlso     [baigInt.h]

  Author      [Mohammad Awedh]

  Copyright [This file was created at the University of Colorado at
  Boulder.  The University of Colorado at Boulder makes no warranty
  about the suitability of this software for any purpose.  It is
  presented on an AS IS basis.]

  Revision    [$Id: baig.h,v 1.2 2022/01/21 09:11:06 cabodi Exp $]


******************************************************************************/

#ifndef _BAIG
#define _BAIG

#include "cudd.h"
/*#include "bdd.h"*/

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define bAigInvertBit      0x1                        /* This bit is set to indicate inverted edge*/

#define bAigArrayMaxSize   (1 << 27)  /* Maximum size of the node array */

#define ZERO_NODE_INDEX    0                          /* Reserve this index for constant Zero node*/
#define bAig_Zero          (ZERO_NODE_INDEX)          /* Constatnt Zero node */
#define bAig_One           (bAig_Not(bAig_Zero))      /* Constatnt One node  */

#define bAig_NULL          2

#define bAigNodeSize       4                          /* Array is used to implement the bAig graph.
                                                         Each node occupies 4 array elements. */
#define bAigFirstNodeIndex 4                          /* The first node index. */

#define bAig_HashTableSize 517731

/* MaMu */
#define	bAigReduceStruct	0x01	
#define bAigReduceThreshold 0x02

#define MAX_LINESIZE		65535
#define BLOCK_SIZE			65536
#define MAX_NAMESIZE		255

typedef enum _readBlifState {INIT, 
							 HEADER, 
							 INPUTS, 
							 OUTPUTS, 
							 NAMES, 
							 END} 
readBlifState;
typedef enum _recurseState  {START, 
							 OR_LEVEL, 
							 AND_LEVEL,
							 ITE_REACHED,
							 XOR_REACHED} 
recurseState;

#define D_LEFT				0
#define D_RIGHT 			1
#define D_START				2
/* MaMu */			

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

typedef char			 		 nameType_t;
typedef int                      bAigEdge_t;
typedef struct bAigManagerStruct bAig_Manager_t;

/**Struct*********************************************************************
 Synopsis    [bAig array]
 Description [array of bAig indexes]
******************************************************************************/

typedef struct bAig_array_t {
  bAigEdge_t *nodes;
  int num;
  int size;
} bAig_array_t;


/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#ifndef EXTERN
#   ifdef __cplusplus
#	define EXTERN extern "C"
#   else
#	define EXTERN extern
#   endif
#endif

/**Macro***********************************************************************

  Synopsis     [Performs the 'Not' Logical operator]

  Description  [This macro inverts the edge going to a node which corresponds 
                to NOT operator]

  SideEffects [none]

  SeeAlso      []

******************************************************************************/

#define bAig_Not(node) (                         \
        (bAigEdge_t) ((node) ^ (bAigInvertBit))  \
)


#define bAigArrayFree(aigArray) {\
  bAigArrayFreeIntern(aigArray);\
  aigArray = NULL;\
}


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EXTERN int bAig_PrintNodeStats(bAig_Manager_t *manager, bAigEdge_t nodeIndex);
EXTERN bAigEdge_t bAig_Subset(bAig_Manager_t *manager, bAigEdge_t nodeIndex, int threshold, int verbose);
EXTERN bAigEdge_t bAig_SubsetNode(bAig_Manager_t *manager, bAigEdge_t node, int subIndex, int verbose);

EXTERN bAig_Manager_t * bAig_initAig(int maxSize);
EXTERN void bAig_quit(bAig_Manager_t *manager);
EXTERN void bAig_NodePrint(bAig_Manager_t *manager, bAigEdge_t node);
EXTERN void bAig_Ref(bAig_Manager_t *manager, bAigEdge_t node);
EXTERN void bAig_Deref(bAig_Manager_t *manager, bAigEdge_t node);
EXTERN nameType_t * bAig_NodeReadName(bAig_Manager_t *manager, bAigEdge_t node);
EXTERN void bAig_NodeSetName(bAig_Manager_t *manager, bAigEdge_t node, nameType_t *name);
EXTERN bAigEdge_t bAig_NodeReadIndexOfRightChild(bAig_Manager_t *manager, bAigEdge_t nodeIndex);
EXTERN int bAig_NodeIsInverted(bAigEdge_t nodeIndex);
EXTERN int bAig_NodeIsConstant(bAigEdge_t nodeIndex);
EXTERN bAigEdge_t bAig_NodeReadIndexOfLeftChild(bAig_Manager_t *manager, bAigEdge_t nodeIndex);
EXTERN bAigEdge_t bAig_And(bAig_Manager_t *manager, bAigEdge_t nodeIndex1, bAigEdge_t nodeIndex2);
EXTERN int bAig_OrDecompose(bAig_Manager_t *manager, bAigEdge_t nodeIndex, bAigEdge_t *term0, bAigEdge_t *term1);
EXTERN int bAig_FanoutNodeCount(bAig_Manager_t *manager, bAigEdge_t nodeIndex, bAigEdge_t refIndex);
EXTERN int bAig_NodeCount(bAig_Manager_t *manager, bAigEdge_t nodeIndex);
EXTERN int bAig_NodeCountWithBound(bAig_Manager_t *manager, bAigEdge_t nodeIndex, int max);
EXTERN int bAig_PrintStats(bAig_Manager_t *manager, bAigEdge_t nodeIndex);
EXTERN bAigEdge_t bAig_Subset(bAig_Manager_t *manager, bAigEdge_t nodeIndex, int threshold, int verbose);
EXTERN bAigEdge_t bAig_Or(bAig_Manager_t *manager, bAigEdge_t nodeIndex1, bAigEdge_t nodeIndex2);
EXTERN bAigEdge_t bAig_Xor(bAig_Manager_t *manager, bAigEdge_t nodeIndex1, bAigEdge_t nodeIndex2);
EXTERN bAigEdge_t bAig_Eq(bAig_Manager_t *manager, bAigEdge_t nodeIndex1, bAigEdge_t nodeIndex2);
EXTERN bAigEdge_t bAig_Then(bAig_Manager_t *manager, bAigEdge_t nodeIndex1, bAigEdge_t nodeIndex2);
EXTERN bAigEdge_t bAig_CreateNode(bAig_Manager_t *manager, bAigEdge_t nodeIndex1, bAigEdge_t nodeIndex2);
EXTERN bAigEdge_t bAig_CreateVarNode(bAig_Manager_t *manager, nameType_t *name);
EXTERN bAigEdge_t bAig_VarNodeFromName(bAig_Manager_t *manager, nameType_t *name);
EXTERN int bAig_isVarNode(bAig_Manager_t *manager, bAigEdge_t node);
EXTERN int bAig_hashInsert(bAig_Manager_t *manager, bAigEdge_t parent, bAigEdge_t node1, bAigEdge_t node2);
EXTERN bAigEdge_t bAig_hashLookup(bAig_Manager_t *manager, bAigEdge_t node1, bAigEdge_t node2);
EXTERN bAigEdge_t bAig_bddtobAig(bAig_Manager_t *bAigManager, DdManager * bddManager, DdNode *fn, char **varnames, int *cuToIds);
EXTERN int bAig_PrintDot(FILE *fp, bAig_Manager_t *manager);
EXTERN void bAig_RecursiveDeref(bAig_Manager_t *manager, bAigEdge_t node);

/* MaMu */
EXTERN int bAig_tryReduceAig(bAig_Manager_t *manager,
							 unsigned char flags,
							 unsigned long threshold,
							 unsigned long maxDepth);

EXTERN bAig_array_t *bAigArraySortByValue(bAig_array_t *aigNodes, int *values, int maxValue, int reverse);
EXTERN void bAigArrayWriteLast(bAig_array_t *aigArray, bAigEdge_t nodeIndex);
EXTERN void bAigArrayPush(bAig_array_t *aigArray, bAigEdge_t nodeIndex);
EXTERN bAigEdge_t bAigArrayPop(bAig_array_t *aigArray);
EXTERN bAig_array_t * bAigArrayAlloc();
EXTERN void bAigArrayFreeIntern(bAig_array_t *aigArray);
EXTERN void bAigArrayClearFreeAuxAig(bAig_Manager_t *bmgr, bAig_array_t *aigArray);
EXTERN bAig_array_t *bAigArrayDup(bAig_array_t *aigArray);
EXTERN void bAigArrayAppend(bAig_array_t *aigArray1, bAig_array_t *aigArray2);
EXTERN void bAigArrayAuxIntSetId(bAig_Manager_t  *manager, bAig_array_t *aigArray);
EXTERN void bAigArrayAuxIntClear(bAig_Manager_t  *manager, bAig_array_t *aigArray);
EXTERN void bAigArrayAuxInt1Clear(bAig_Manager_t  *manager, bAig_array_t *aigArray);
EXTERN void bAigArrayClear(bAig_array_t *aigArray);
EXTERN void bAigArrayClearDeref(bAig_Manager_t  *bmgr, bAig_array_t *aigArray);

/* MaMu */


/**AutomaticEnd***************************************************************/

#endif /* _BAIG */

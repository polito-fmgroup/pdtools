/**CHeaderFile*****************************************************************

  FileName    [baigInt.h]

  PackageName [baig]

  Synopsis    []

  Description [Internal data structures of the bAig package.]

  SeeAlso     []

  Author      [Mohammad Awedh]

  Copyright [ This file was created at the University of Colorado at
  Boulder.  The University of Colorado at Boulder makes no warranty
  about the suitability of this software for any purpose.  It is
  presented on an AS IS basis.]

  Revision    [$Id: baigInt.h,v 1.1.1.1 2021/09/11 09:57:46 cabodi Exp $]

******************************************************************************/

#ifndef _bAIGINT
#define _bAIGINT

#include "util.h"
#include "pdtutil.h"
#include "st.h"
#include "cuddInt.h"
/*#include "array.h"*/
/*#include "ntk.h"*/
#include "baig.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/* Each node occupies 4 location.  The following describes these locations */

#define  bAigLeft     0   /* Index of the left child */ 
#define  bAigRight    1   /* Index of the right child */ 
#define  bAigNext     2   /* Index of the next node in the Hash Table. */  
#define  bAigRefCount 3   /* Index of reference count for this node.*/

#define FREE_LIST_SORT_TH    1000000

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

typedef union {
  int number;
  void *pointer;
} bAigPtrOrInt_t;

/**Struct**********************************************************************

  Synopsis    [And/Inverter graph manager.]

  Description []

******************************************************************************/

struct bAigManagerStruct {
  bAigEdge_t *NodesArray;       /* contains pointers to nodes of type
				   bAig_Node_t */
  bAigEdge_t *freeList;         /* free Aig list */
  int        nodesArraySize;    /* contains the current number of nodes */
  int        maxNodesArraySize; /* Nodes Array maximum size */

  bAigEdge_t *HashTable;        /* Hash table */
  st_table   *SymbolTable;      /* Symbol table to store variables and their
				   indices */
  bAigPtrOrInt_t *auxPtr;         /* This is an auxiliary pointer internally 
                                   used by recursive procedures */
  void       **auxPtr0;           /* This is an auxiliary pointer internally 
                                   used by recursive procedures */
  void       **auxPtr1;           /* This is an auxiliary pointer internally 
                                   used by recursive procedures */
  void       **varPtr;           /* This is an auxiliary pointer internally 
                                   used by recursive procedures */
  int        *auxInt;           /* This is an auxiliary integer internally 
                                   used by recursive procedures */
  int        *auxInt1;           /* This is an auxiliary integer internally 
                                   used by recursive procedures */
  float      *auxFloat;           /* This is an auxiliary float internally 
                                   used by recursive procedures */
  int        *cnfId;            /* This is an auxiliary integer internally 
                                   used for CNF IDS */
  int        *auxRef;           /* This is an auxiliary integer internally 
                                   used by recursive procedures */
  unsigned char *auxChar;       /* auxiliary char */
  unsigned char *visited;       /* visited flag */
  bAigEdge_t *auxAig0;          /* auxiliary Aig */
  bAigEdge_t *auxAig1;          /* auxiliary Aig */
  bAigEdge_t *cacheAig;         /* cache Aig */
  char       **nameList;        /* This table contains the name of each node.
				   nodeID is used to access the node's name. */
  void       *mVarList;         /* List of mVarId of each variable */ 
  void       *bVarList;         /* List of bVardId (bAig_t) of the binary value
				   encoding for multi-valued variables. */
  void *owner;                  /* used to store pointer to owner
				   manager */
  int freeNum;
  int freeCount;
  int freeSort;
};


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**Macro***********************************************************************

  Synopsis     [Returns the non inverted edge.]

  Description  [This macro returns the non inverted edge.  It clears the least 
  significant bit of the edge if it is set.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

#define bAig_NonInvertedEdge(edge) ( \
	(edge) & ~(bAigInvertBit)    \
)

/**Macro***********************************************************************

  Synopsis     [Returns true if the edge is inverted.]

  Description  []

  SideEffects [none] 

  SeeAlso      []

******************************************************************************/

#define bAig_IsInverted(edge) (   \
	(edge) & (bAigInvertBit)  \
)

/**Macro***********************************************************************

  Synopsis    []

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

#define rightChild(manager,node)(\
manager->NodesArray[bAig_NonInvertedEdge(node)+bAigRight]\
)

/**Macro***********************************************************************

  Synopsis    []

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

#define leftChild(manager,node)(\
manager->NodesArray[bAig_NonInvertedEdge(node)+bAigLeft]\
)

/**Macro***********************************************************************

  Synopsis    []

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

#define refCount(manager,node)(\
manager->NodesArray[(bAig_NonInvertedEdge(node))+bAigRefCount]\
)

/**Macro***********************************************************************

  Synopsis    []

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

#define nodeVisited(manager,node)(\
manager->visited[bAigNodeID(node)]\
)
/**Macro***********************************************************************

  Synopsis    []

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

#define nodeAuxChar(manager,node)(\
manager->auxChar[bAigNodeID(node)]\
)

/**Macro***********************************************************************

  Synopsis    []

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

#define nodeSetVisited(manager,node) {\
manager->visited[bAigNodeID(node)] = 1;\
}

/**Macro***********************************************************************

  Synopsis    []

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

#define nodeClearVisited(manager,node) {\
manager->visited[bAigNodeID(node)] = 0;\
}


/**Macro***********************************************************************

  Synopsis    []

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

#define bnext(manager,node)(\
manager->NodesArray[bAig_NonInvertedEdge(node)+bAigNext]\
)

/**Macro***********************************************************************

  Synopsis    [The Aig node count]
  Description [The Aig node count]
  SideEffects [none]
  SeeAlso     []

******************************************************************************/
#define bAigNodeCount(manager)(\
manager->maxNodesArraySize/bAigNodeSize\
)

/**Macro***********************************************************************

  Synopsis    [The Aig node count]
  Description [The Aig node count]
  SideEffects [none]
  SeeAlso     []

******************************************************************************/
#define bAigMaxNodeCount(manager)(\
manager->maxNodesArraySize/bAigNodeSize\
)

/**Macro***********************************************************************

  Synopsis    [The Aig node count]
  Description [The Aig node count]
  SideEffects [none]
  SeeAlso     []

******************************************************************************/
#define bAigFreeNodeCount(manager)(\
(manager->nodesArraySize/bAigNodeSize-manager->freeNum)\
)

/**Macro***********************************************************************

  Synopsis    [The node ID]

  Description [Node ID is a unique number for each node and can be found by 
              dividing the edge going to this node by bAigNodeSize.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

#define bAigNodeID(edge)(\
bAig_NonInvertedEdge(edge)>>2 \
)

/**Macro***********************************************************************

  Synopsis    [Hash function for the hash table.]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

#define HashTableFunction(left, right) \
( (bAigEdge_t)\
((((unsigned) (( (bAigEdge_t) (left))) << (3+11)) ^ \
((((unsigned)(bAigEdge_t) (right)) << 3))) % bAig_HashTableSize) \
)


/**Macro***********************************************************************

  Synopsis     [Returns the auxPtr field.]

  Description  [This macro returns the auxPtr field.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

#define bAig_AuxPtr(manager,edge) ( \
manager->auxPtr[bAigNodeID(edge)].pointer\
)
/**Macro***********************************************************************

  Synopsis     [Returns the auxPtr field.]

  Description  [This macro returns the auxPtr field.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

#define bAig_AuxPtrNum(manager,edge) ( \
manager->auxPtr[bAigNodeID(edge)].number\
)

/**Macro***********************************************************************

  Synopsis     [Returns the auxPtr field.]

  Description  [This macro returns the auxPtr field.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

#define bAig_AuxPtr0(manager,edge) ( \
manager->auxPtr0[bAigNodeID(edge)]\
)

/**Macro***********************************************************************

  Synopsis     [Returns the auxPtr field.]

  Description  [This macro returns the auxPtr field.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

#define bAig_AuxPtr1(manager,edge) ( \
manager->auxPtr1[bAigNodeID(edge)]\
)

/**Macro***********************************************************************

  Synopsis     [Returns the auxPtr field.]

  Description  [This macro returns the auxPtr field.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

#define bAig_VarPtr(manager,edge) ( \
manager->varPtr[bAigNodeID(edge)]\
)

/**Macro***********************************************************************

  Synopsis     [Returns the auxPtr field.]

  Description  [This macro returns the auxPtr field.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

#define bAig_AuxInt(manager,edge) ( \
manager->auxInt[bAigNodeID(edge)]\
)

/**Macro***********************************************************************

  Synopsis     [Returns the auxPtr field.]

  Description  [This macro returns the auxPtr field.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

#define bAig_AuxInt1(manager,edge) ( \
manager->auxInt1[bAigNodeID(edge)]\
)

/**Macro***********************************************************************

  Synopsis     [Returns the auxPtr field.]

  Description  [This macro returns the auxPtr field.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

#define bAig_AuxFloat(manager,edge) ( \
manager->auxFloat[bAigNodeID(edge)]\
)
/**Macro***********************************************************************

  Synopsis     [Returns the auxPtr field.]

  Description  [This macro returns the auxPtr field.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

#define bAig_CnfId(manager,edge) ( \
manager->cnfId[bAigNodeID(edge)]\
)

/**Macro***********************************************************************

  Synopsis     [Returns the auxPtr field.]

  Description  [This macro returns the auxPtr field.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

#define bAig_AuxRef(manager,edge) ( \
manager->auxRef[bAigNodeID(edge)]\
)

/**Macro***********************************************************************
  Synopsis     [Returns the auxAig0 field.]
  Description  [This macro returns the auxAig0 field.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

#define bAig_AuxAig0(manager,edge) ( \
manager->auxAig0[bAigNodeID(edge)]\
)

/**Macro***********************************************************************
  Synopsis     [Returns the auxAig1 field.]
  Description  [This macro returns the auxAig1 field.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

#define bAig_AuxAig1(manager,edge) ( \
manager->auxAig1[bAigNodeID(edge)]\
)

/**Macro***********************************************************************
  Synopsis     [Returns the cacheAig field.]
  Description  [This macro returns the cacheAig field.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

#define bAig_CacheAig(manager,edge) ( \
manager->cacheAig[bAigNodeID(edge)]\
)

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EXTERN bAigEdge_t bAigCreateAndNode(bAig_Manager_t *manager, bAigEdge_t node1, bAigEdge_t node2);
EXTERN void bAigDeleteAndNode(bAig_Manager_t *manager, bAigEdge_t node);
EXTERN int nodeCountIntern(bAig_Manager_t *manager, bAigEdge_t nodeIndex);
EXTERN int nodeCountInternWithBound(bAig_Manager_t *manager, bAigEdge_t nodeIndex, int max);
EXTERN int nodeFanoutCountIntern(bAig_Manager_t *manager, bAigEdge_t nodeIndex, bAigEdge_t refIndex);
EXTERN void nodeClearVisitedIntern(bAig_Manager_t *manager, bAigEdge_t nodeIndex);
EXTERN void bAigManagerSortFreeList(bAig_Manager_t *manager);


/**AutomaticEnd***************************************************************/
#endif /* _bAIGINT */

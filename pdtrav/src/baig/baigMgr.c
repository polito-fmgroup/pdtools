/**CFile***********************************************************************

  FileName    [baigMgr.c]

  PackageName [baig]

  Synopsis    [Functions to initialize and shut down the bAig manager.]

  Author      [Mohammad Awedh]

  Copyright [ This file was created at the University of Colorado at
  Boulder.  The University of Colorado at Boulder makes no warranty
  about the suitability of this software for any purpose.  It is
  presented on an AS IS basis.]

******************************************************************************/

#include "baigInt.h"


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


/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Creates a new bAig manager.]

  Description [Creates a new bAig manager, initializes the NodeArray
  and noOfOutputFunctions. Returns a pointer to the manager if successful;
  NULL otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
bAig_Manager_t *
bAig_initAig(
  int maxSize
)
{
  int i;

    bAig_Manager_t *manager = Pdtutil_Alloc(bAig_Manager_t,1);


    if (manager == NIL( bAig_Manager_t)){
      return manager;
    }
    if (maxSize >  bAigArrayMaxSize ) {
      maxSize =  bAigArrayMaxSize;
    }
    manager->freeNum = 0;
    manager->freeCount = 0;
    manager->freeSort = 0;
    manager->NodesArray = Pdtutil_Alloc(bAigEdge_t, maxSize);
    memset(manager->NodesArray, 0 , sizeof(bAigEdge_t) * maxSize);
    manager->maxNodesArraySize = maxSize;
    manager->nodesArraySize = bAigFirstNodeIndex;
    manager->freeList = Pdtutil_Alloc(bAigEdge_t,
      manager->maxNodesArraySize/bAigNodeSize);

    manager->SymbolTable = st_init_table(strcmp,st_strhash);
    manager->nameList = Pdtutil_Alloc(char *, manager->maxNodesArraySize/bAigNodeSize);
    manager->auxPtr = Pdtutil_Alloc(bAigPtrOrInt_t, manager->maxNodesArraySize/bAigNodeSize);
    manager->auxPtr0 = Pdtutil_Alloc(void *, manager->maxNodesArraySize/bAigNodeSize);
    manager->auxPtr1 = Pdtutil_Alloc(void *, manager->maxNodesArraySize/bAigNodeSize);
    manager->varPtr = Pdtutil_Alloc(void *, manager->maxNodesArraySize/bAigNodeSize);
    manager->auxInt = Pdtutil_Alloc(int, manager->maxNodesArraySize/bAigNodeSize);
    manager->auxInt1 = Pdtutil_Alloc(int, manager->maxNodesArraySize/bAigNodeSize);
    manager->auxFloat = Pdtutil_Alloc(float, manager->maxNodesArraySize/bAigNodeSize);
    manager->cnfId = Pdtutil_Alloc(int, manager->maxNodesArraySize/bAigNodeSize);
    manager->auxRef = Pdtutil_Alloc(int, manager->maxNodesArraySize/bAigNodeSize);
    manager->auxChar = Pdtutil_Alloc(unsigned char,
      manager->maxNodesArraySize/bAigNodeSize);
    manager->visited = Pdtutil_Alloc(unsigned char,
      manager->maxNodesArraySize/bAigNodeSize);
    manager->auxAig0 = Pdtutil_Alloc(bAigEdge_t,
      manager->maxNodesArraySize/bAigNodeSize);
    manager->auxAig1 = Pdtutil_Alloc(bAigEdge_t,
      manager->maxNodesArraySize/bAigNodeSize);
    manager->cacheAig = Pdtutil_Alloc(bAigEdge_t,
      manager->maxNodesArraySize/bAigNodeSize);
    for (i=0; i<manager->maxNodesArraySize/bAigNodeSize; i++) {
      manager->auxChar[i] = 0;
      manager->visited[i] = 0;
      manager->auxPtr[i].pointer = NULL;
      manager->auxPtr0[i] = NULL;
      manager->auxPtr1[i] = NULL;
      manager->varPtr[i] = NULL;
      manager->auxInt[i] = -1;
      manager->auxInt1[i] = -1;
      manager->cnfId[i] = 0;
      manager->auxRef[i] = 0;
      manager->auxAig0[i] = bAig_NULL;
      manager->auxAig1[i] = bAig_NULL;
      manager->cacheAig[i] = bAig_NULL;
    }

    manager->HashTable =  Pdtutil_Alloc(bAigEdge_t, bAig_HashTableSize);
    for (i=0; i<bAig_HashTableSize; i++)
      manager->HashTable[i]= bAig_NULL;

    refCount(manager,bAig_Zero) = 0;
    bnext(manager,bAig_Zero) = bAig_NULL;
    leftChild(manager,bAig_Zero) = bAig_NULL;
    rightChild(manager,bAig_Zero) = bAig_NULL;

    return(manager);

} /* end of bAig_Init */

/**Function********************************************************************

  Synopsis [Quit bAig manager]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void
bAig_quit(
  bAig_Manager_t *manager)
{
  char          *name;
  st_generator  *stGen;
  bAigEdge_t    varIndex;
  int i, nRef=0, totRef=0;

  Pdtutil_Free(manager->HashTable);
  st_foreach_item_int(manager->SymbolTable, stGen, &name, &varIndex) {
    Pdtutil_Free(name);
  }
  st_free_table(manager->SymbolTable);

  /* i is too small to represent 80000 */
  for (i=0; i< manager->nodesArraySize; i += bAigNodeSize){
    //    Pdtutil_Free(manager->nameList[i]);
    if (refCount(manager,i) > 0) {
      nRef++;
      totRef += refCount(manager,i);
    }
  }

  if (nRef>0) {
    printf("WARNING: %d referenced aig nodes - %d tot refs\n", nRef, totRef);

    for (i=0; i< manager->nodesArraySize; i += bAigNodeSize){
      //    Pdtutil_Free(manager->nameList[i]);
      if (refCount(manager,i) > 0) {
	if (!bAig_NodeIsConstant(i) && !bAig_isVarNode(manager,i)) {
	  refCount(manager,leftChild(manager,i))--;
	  refCount(manager,rightChild(manager,i))--;
	}
      }
    }

    for (i=nRef=totRef=0; i< manager->nodesArraySize; i += bAigNodeSize){
      //    Pdtutil_Free(manager->nameList[i]);
      if (refCount(manager,i) > 0) {
	nRef++;
	totRef += refCount(manager,i);
      }
    }
    printf("WARNING: %d referenced root aigs - %d tot refs\n", nRef, totRef);
  }

  Pdtutil_Free(manager->nameList);
  Pdtutil_Free(manager->NodesArray);
  Pdtutil_Free(manager->freeList);

  Pdtutil_Free(manager->auxPtr);
  Pdtutil_Free(manager->auxPtr0);
  Pdtutil_Free(manager->auxPtr1);
  Pdtutil_Free(manager->varPtr);
  Pdtutil_Free(manager->auxInt);
  Pdtutil_Free(manager->auxFloat);
  Pdtutil_Free(manager->auxInt1);
  Pdtutil_Free(manager->cnfId);
  Pdtutil_Free(manager->auxRef);
  Pdtutil_Free(manager->auxChar);
  Pdtutil_Free(manager->visited);
  Pdtutil_Free(manager->auxAig0);
  Pdtutil_Free(manager->auxAig1);
  Pdtutil_Free(manager->cacheAig);

  Pdtutil_Free(manager);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
void
bAig_NodePrint(
   bAig_Manager_t  *manager,
   bAigEdge_t node)
{

  if (node == bAig_Zero){
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "0")
    );
      return;
  }
  if (node == bAig_One){
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "1")
    );
      return;
  }

 if (bAig_IsInverted(node)){
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, "NOT(")
       );
 }


 if ( rightChild(manager,node) == bAig_NULL){
   Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
     Pdtutil_VerbLevelNone_c,
     fprintf(stdout, "Var Node")
   );
     if (bAig_IsInverted(node)){
            Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
              Pdtutil_VerbLevelNone_c,
              fprintf(stdout, ")")
            );
       }
       return;
 }
 Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
   Pdtutil_VerbLevelNone_c,
   fprintf(stdout, "(")
 );
 bAig_NodePrint(manager, leftChild(manager,node));
 Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
   Pdtutil_VerbLevelNone_c,
   fprintf(stdout, "AND")
 );
 bAig_NodePrint(manager, rightChild(manager,node));
 Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
   Pdtutil_VerbLevelNone_c,
   fprintf(stdout, ")")
 );
 if (bAig_IsInverted(node)){
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, ")")
       );
 }

}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


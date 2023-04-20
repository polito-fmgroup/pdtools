/**CFile***********************************************************************

   FileName    [baigArray.c]

   PackageName [baig]

   Synopsis    [Routines to access Arrays of baig nodes.]

   Author      [Gianpiero Cabodi]

  Copyright [This file was created at the Politecnico di Torino,
    Torino, Italy. 
    The  Politecnico di Torino makes no warranty about the suitability of 
    this software for any purpose.  
    It is presented on an AS IS basis.
  ]
  

 ******************************************************************************/

 #include "baigInt.h"

 /*---------------------------------------------------------------------------*/
 /* Constant declarations                                                     */
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
  Synopsis    [Constant redundancy removal]
  Description [Constant redundancy removal]
  SideEffects []
  SeeAlso     []
******************************************************************************/
bAig_array_t *
bAigArraySortByValue(
  bAig_array_t *aigNodes,
  int *values,
  int maxValue,
  int reverse
)
{
  bAig_array_t *sortedNodes;
  bAigEdge_t baig;
  int *count, i;

  if (maxValue <= 0) {
    for (i=0; i<aigNodes->num; i++) {
      if (values[i] > maxValue) {
	maxValue = values[i];
      }
    }
  }

  /* counting sort */
  count = (int *)calloc(maxValue+1, sizeof(int));
  for (i=0; i<aigNodes->num; i++) {
    count[values[i]]++;
  }

  for (i=1; i<=maxValue; i++) {
    count[i] += count[i-1];
  }

  sortedNodes = bAigArrayAlloc();
  sortedNodes->nodes = Pdtutil_Alloc(bAigEdge_t, aigNodes->num);
  sortedNodes->num = sortedNodes->size = aigNodes->num;
  for (i=aigNodes->num-1; i>=0; i--) {
    if (reverse) {
      sortedNodes->nodes[aigNodes->num - count[values[i]]--] = 
	aigNodes->nodes[i];
    } else {
      sortedNodes->nodes[--count[values[i]]] = aigNodes->nodes[i];
    }
  }

  free(count);
  return sortedNodes;
}

/**Function********************************************************************
  Synopsis    [Push new entry in AIG array]
  Description [Push new entry in AIG array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
bAigArrayPush(
  bAig_array_t *aigArray,
  bAigEdge_t nodeIndex
)
{
  bAigArrayWriteLast(aigArray,nodeIndex);
}

/**Function********************************************************************
  Synopsis    [Push new entry in AIG array]
  Description [Push new entry in AIG array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
bAigEdge_t
bAigArrayPop(
  bAig_array_t *aigArray
)
{
  Pdtutil_Assert(aigArray->num>0,"pop from empty baig array");
  return aigArray->nodes[--aigArray->num];
}

/**Function********************************************************************
  Synopsis    [Push new entry in AIG array]
  Description [Push new entry in AIG array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
bAigArrayClear(
  bAig_array_t *aigArray
)
{
  Pdtutil_Assert(aigArray->num>=0,"pop from empty baig array");
  aigArray->num = 0;
}

/**Function********************************************************************
  Synopsis    [Add new entry in AIG array]
  Description [Add new entry in AIG array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
bAigArrayWriteLast(
  bAig_array_t *aigArray,
  bAigEdge_t nodeIndex
)
{
  if (aigArray->num >= aigArray->size) {
    if (aigArray->size==0) {
      aigArray->size = 100;
      aigArray->nodes = Pdtutil_Alloc(bAigEdge_t,aigArray->size);
    }
    else {
      aigArray->size *= 2;
      aigArray->nodes = 
        Pdtutil_Realloc(bAigEdge_t,aigArray->nodes,aigArray->size);
    }
  }
  aigArray->nodes[aigArray->num++] = nodeIndex;
}

/**Function********************************************************************
  Synopsis    [Alloc Aig Array]
  Description [Alloc Aig Array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
bAig_array_t *
bAigArrayAlloc(
)
{
  bAig_array_t *aigArray = Pdtutil_Alloc(bAig_array_t,1);
  aigArray->num = aigArray->size = 0;
  aigArray->nodes = NULL;
  return aigArray;
}

/**Function********************************************************************
  Synopsis    [Alloc Aig Array]
  Description [Alloc Aig Array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
bAig_array_t *
bAigArrayDup(
  bAig_array_t *aigArray
)
{
  bAig_array_t *aigArray2 = Pdtutil_Alloc(bAig_array_t,1);
  int i;
  aigArray2->num = aigArray2->size = aigArray->num;
  aigArray2->nodes = Pdtutil_Alloc(bAigEdge_t,aigArray->size);
  for (i=0; i<aigArray2->num; i++) {
    aigArray2->nodes[i] = aigArray->nodes[i];
  }
  return aigArray2;
}

/**Function********************************************************************
  Synopsis    [Alloc Aig Array]
  Description [Alloc Aig Array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
bAigArrayAppend(
  bAig_array_t *aigArray1,
  bAig_array_t *aigArray2
)
{
  int i;
  for (i=0; i<aigArray2->num; i++) {
    bAigArrayPush(aigArray1,aigArray2->nodes[i]);
  }
}

/**Function********************************************************************
  Synopsis    [Alloc Aig Array]
  Description [Alloc Aig Array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
bAigArrayAuxIntSetId(
  bAig_Manager_t  *manager,
  bAig_array_t *aigArray
)
{
  int i;
  for (i=0; i<aigArray->num; i++) {
    bAigEdge_t baig = aigArray->nodes[i];
    Pdtutil_Assert (bAig_AuxInt(manager,baig) == -1, "wrong auxInt field");
    bAig_AuxInt(manager,baig) = i;
  }
}

/**Function********************************************************************
  Synopsis    [Alloc Aig Array]
  Description [Alloc Aig Array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
bAigArrayAuxIntClear(
  bAig_Manager_t  *manager,
  bAig_array_t *aigArray
)
{
  int i;
  for (i=0; i<aigArray->num; i++) {
    bAigEdge_t baig = aigArray->nodes[i];
    bAig_AuxInt(manager,baig) = -1;
  }
}

/**Function********************************************************************
  Synopsis    [Alloc Aig Array]
  Description [Alloc Aig Array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
bAigArrayAuxInt1Clear(
  bAig_Manager_t  *manager,
  bAig_array_t *aigArray
)
{
  int i;
  for (i=0; i<aigArray->num; i++) {
    bAigEdge_t baig = aigArray->nodes[i];
    bAig_AuxInt1(manager,baig) = -1;
  }
}

/**Function********************************************************************
  Synopsis    [Free Aig Array]
  Description [Free Aig Array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
bAigArrayFreeIntern(
  bAig_array_t *aigArray
)
{
  if (aigArray!=NULL) {
    Pdtutil_Free(aigArray->nodes);
    Pdtutil_Free(aigArray);
  }
}

/**Function********************************************************************
  Synopsis    [Free Aig Array and clear auxaig field]
  Description [Free Aig Array and clear auxaig field]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
bAigArrayClearDeref(
  bAig_Manager_t  *bmgr,
  bAig_array_t *aigArray
)
{
  int j;
  for (j=0; j<aigArray->num; j++) {
    bAigEdge_t baig = aigArray->nodes[j];
    bAig_RecursiveDeref(bmgr,baig);
    aigArray->nodes[j]=bAig_NULL;
  }
}

/**Function********************************************************************
  Synopsis    [Free Aig Array and clear auxaig field]
  Description [Free Aig Array and clear auxaig field]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
bAigArrayClearFreeAuxAig(
  bAig_Manager_t  *bmgr,
  bAig_array_t *aigArray
)
{
  int j;
  for (j=0; j<aigArray->num; j++) {
    bAigEdge_t baig = aigArray->nodes[j];
    bAig_RecursiveDeref(bmgr,bAig_AuxAig0(bmgr,baig));
    bAig_RecursiveDeref(bmgr,bAig_AuxAig1(bmgr,baig));
    bAig_AuxAig0(bmgr,baig) = bAig_NULL;
    bAig_AuxAig1(bmgr,baig) = bAig_NULL;
  }
}



 /*---------------------------------------------------------------------------*/
 /* Definition of internal functions                                          */
 /*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

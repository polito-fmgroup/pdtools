/**CFile***********************************************************************

   FileName    [baigNode.c]

   PackageName [baig]

   Synopsis    [Routines to access node data structure of the And/Inverter
            graph.]

   Author      [Mohammad Awedh]

   Copyright [ This file was created at the University of Colorado at
   Boulder.  The University of Colorado at Boulder makes no warranty
   about the suitability of this software for any purpose.  It is
   presented on an AS IS basis.]


 ******************************************************************************/

 #include "baigInt.h"

// static char rcsid[] = "$Id: baigNode.c,v 1.2 2022/01/21 09:11:06 cabodi Exp $";

 /*---------------------------------------------------------------------------*/
 /* Constant declarations                                                     */
 /*---------------------------------------------------------------------------*/

 /**AutomaticStart*************************************************************/

 /*---------------------------------------------------------------------------*/
 /* Static function prototypes                                                */
 /*---------------------------------------------------------------------------*/

 static void incRefCount(bAig_Manager_t *manager, bAigEdge_t nodeIndex);
 static void decRefCount(bAig_Manager_t *manager, bAigEdge_t nodeIndex);
 static bAigEdge_t HashTableLookup(bAig_Manager_t *manager, bAigEdge_t node1, bAigEdge_t node2);
 static void HashTableDelete(bAig_Manager_t *manager, bAigEdge_t node);
 static int HashTableAdd(bAig_Manager_t *manager, bAigEdge_t nodeIndexParent, bAigEdge_t nodeIndex1, bAigEdge_t nodeIndex2);
 static char * util_inttostr(int i);
 static char * util_strcat3(char * str1, char * str2, char * str3);
 static char * util_strcat4(char * str1, char * str2, char * str3, char * str4);

 /**AutomaticEnd***************************************************************/


 /*---------------------------------------------------------------------------*/
 /* Definition of exported functions                                          */
 /*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Increment the reference count of a bAig node.]

  Description [.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void
bAig_Ref(
  bAig_Manager_t *manager,
  bAigEdge_t  node
)
{
  incRefCount(manager,node);
}

/**Function********************************************************************

  Synopsis    [Decrement the reference count of a bAig node.]

  Description [.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void
bAig_Deref(
  bAig_Manager_t *manager,
  bAigEdge_t  node
)
{
  decRefCount(manager,node);
}


/**Function********************************************************************

  Synopsis    [Read Node's name.]

  Description [Read the name of a node given its index.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
nameType_t *
bAig_NodeReadName(
   bAig_Manager_t *manager,
   bAigEdge_t     node
)
{
  return manager->nameList[bAigNodeID(node)];
}

/**Function********************************************************************

  Synopsis    [Set Node's name.]

  Description [Set the name of node in Symbol table and name List]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void
bAig_NodeSetName(
   bAig_Manager_t *manager,
   bAigEdge_t     node,
   nameType_t     *name
)
{
  char *myName = util_strsav(name);
  if (manager->nameList[bAigNodeID(node)] != NULL) {
     st_delete(manager->SymbolTable,
       &(manager->nameList[bAigNodeID(node)]), NULL);
     //     Pdtutil_Free(manager->nameList[bAigNodeID(node)]);
   }
   st_insert(manager->SymbolTable, myName, (char*) (long) node);
   manager->nameList[bAigNodeID(node)] = myName;
}

/**Function********************************************************************

  Synopsis    [Returns the index of the right node.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
bAigEdge_t
bAig_NodeReadIndexOfRightChild(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex
)
{
  return rightChild(manager,nodeIndex);
}

/**Function********************************************************************

  Synopsis    [Returns the index of the right node.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int
bAig_NodeIsInverted(
   bAigEdge_t nodeIndex
)
{
  return bAig_IsInverted(nodeIndex);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int
bAig_NodeIsConstant(
   bAigEdge_t nodeIndex
)
{
  return (nodeIndex == bAig_One || nodeIndex == bAig_Zero);
}

/**Function********************************************************************

  Synopsis    [Returns the index of the left node.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
bAigEdge_t
bAig_NodeReadIndexOfLeftChild(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex
)
{
  return leftChild(manager,nodeIndex);
}


/**Function********************************************************************

  Synopsis    [Sort free baig list.]

  Description [Sort free baig list with simplified counting sort.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void bAigManagerSortFreeList(
   bAig_Manager_t *manager
)
{
  int i, j, n, m;
  bAigEdge_t b;

  if (manager->freeNum <= 0) return;

  manager->freeSort++;

  n = manager->freeNum;
  m = manager->nodesArraySize;

  for (i=0; i<n; i++) {
    b = manager->freeList[i];
    //Pdtutil_Assert(refCount(manager,b)>=-1,"negative ref count");
    refCount(manager,b) = -2;
  }
  for (b=0,i=n; b<m; b+=4) {
    //  for (b=0,i=0; b<m; b+=4) {
    if (refCount(manager,b)<0) {
      refCount(manager,b) = 0;
      manager->freeList[--i] = b;
      //      manager->freeList[i++] = b;
    }
  }
  Pdtutil_Assert(i==0,"wrong num of free baigs");

}


#define NEW_AND 1
#if NEW_AND


/**Function********************************************************************

  Synopsis    [Performs the Logical AND of two nodes.]

  Description [This function performs the Logical AND of two nodes.  The inputs
            are the indices of the two nodes.  This function returns the index
            of the result node.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
bAigEdge_t bAig_And(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex1,
   bAigEdge_t nodeIndex2
)
{

       bAigEdge_t newNodeIndex;

       /* MaMu */
       bAigEdge_t r1, l1, r2, l2, tmp;
       char isVarNode1, isVarNode2;
       char isInvertedNode1, isInvertedNode2, isInvertedL1, isInvertedR1, isInvertedL2, isInvertedR2;
       /* MaMu */

       /* VIS 2.1 code, moved from the bottom of f(x) to the top */
       newNodeIndex = nodeIndex1;     /* The left node has the smallest index */
       if (nodeIndex1 > nodeIndex2)
       {
             nodeIndex1 = nodeIndex2;
             nodeIndex2 = newNodeIndex;
       }

       /* Trivial one level minimization */
       if ( nodeIndex2 == bAig_Zero )
       {
             /* MaMu */
             //incRefCount(manager, bAig_Zero);
             /* MaMu */
             return bAig_Zero;
       }
       if ( nodeIndex1 == bAig_Zero )
       {
             /* MaMu */
             //incRefCount(manager, bAig_Zero);
             /* MaMu */
             return bAig_Zero;
       }
       if ( nodeIndex2 == bAig_One )
       {
             /* MaMu */
             //incRefCount(manager, nodeIndex1);
             /* MaMu */
             return nodeIndex1;
       }
       if ( nodeIndex1 == bAig_One )
       {
             /* MaMu */
             //incRefCount(manager, nodeIndex2);
             /* MaMu */
             return nodeIndex2;
       }
       if ( nodeIndex1 == nodeIndex2 )
       {
             /* MaMu */
             //incRefCount(manager, nodeIndex1);
             /* MaMu */
             return nodeIndex1;
       }
       if ( nodeIndex1 == bAig_Not(nodeIndex2) )
       {
             /* MaMu */
             //incRefCount(manager, bAig_Zero);
             /* MaMu */
             return bAig_Zero;
       }

       /* MaMu */
       /* 2 level logic backward minimization */
         /* Remeber: permutation of nodes is useless in expression evaluation: e.g.: (xy)!x only and not (xy)!x + (yx)!x because
       of static variable ordering thru pointer address */

       r1=l1=r2=l2 = bAig_NULL;
       isInvertedL1 = isInvertedL2 = isInvertedR1 = isInvertedR2 = 0;

       /* begin: non-varNode vertexes minimization for nodeIndex1 and nodeIndex2 */
       isVarNode1 = bAig_isVarNode(manager,nodeIndex1);
       isVarNode2 = bAig_isVarNode(manager,nodeIndex2);
       isInvertedNode1 = bAig_IsInverted(nodeIndex1);
       isInvertedNode2 = bAig_IsInverted(nodeIndex2);

       /* preliminary checks */
       if (!isVarNode1)
       {
             r1=rightChild(manager, nodeIndex1);
             l1=leftChild(manager, nodeIndex1);
             isInvertedL1 = bAig_IsInverted(l1);
             isInvertedR1 = bAig_IsInverted(r1);
       }

       if (!isVarNode2)
       {
             r2=rightChild(manager, nodeIndex2);
             l2=leftChild(manager, nodeIndex2);
             isInvertedL2 = bAig_IsInverted(l2);
             isInvertedR2 = bAig_IsInverted(r2);
       }

       /* Nodi figli duplicati */
        /* x(xy) || y(xy) = xy */
        if (!isInvertedNode2 && r2 != bAig_NULL && l2 != bAig_NULL)
        {
              if (nodeIndex1 == l2 || nodeIndex1 == r2)
              {
                    //incRefCount(manager, nodeIndex2);
                    return nodeIndex2;
              }
        }


       /* (xy)x || (xy)y = xy */
       if (!isInvertedNode1 && r1 != bAig_NULL && l1 != bAig_NULL)
       {
             if (nodeIndex2 == l1 || nodeIndex2 == r1)
             {
                   //incRefCount(manager, nodeIndex1);
                   return nodeIndex1;
             }
       }

       /* !x(xy) || !y(xy) = 0 */
       if (!isInvertedNode2 && r2 != bAig_NULL && l2 != bAig_NULL)
       {
             if (nodeIndex1 == bAig_Not(l2) || nodeIndex1 == bAig_Not(r2))
             {
                   /* MaMu */
                   //incRefCount(manager, bAig_Zero);
                   /* MaMu */
                   return bAig_Zero;
             }
       }

       /* (xy)!x || (xy)!y = 0 */
       if (!isInvertedNode1 && r1 != bAig_NULL && l1 != bAig_NULL)
       {
             if (nodeIndex2 == bAig_Not(l1) || nodeIndex2 == bAig_Not(r1))
             {
                   /* MaMu */
                   //incRefCount(manager, bAig_Zero);
                   /* MaMu */
                   return bAig_Zero;
             }
       }

       /* x!(xy) || y!(xy) = x!y || y!x */
       if (isInvertedNode2 && r2 != bAig_NULL && l2 != bAig_NULL)
       {
             if (nodeIndex1 == l2) /* x!y */
             {
                   return(bAig_And(manager,nodeIndex1,bAig_Not(r2)));
             }
             if (nodeIndex1 == r2) /* y!x */
             {
                   return(bAig_And(manager,nodeIndex1,bAig_Not(l2)));
             }
       }

       /* !(xy)x || !(xy)y = !yx || !xy */
       if (isInvertedNode1 && r1 != bAig_NULL && l1 != bAig_NULL)
       {
             if (nodeIndex2 == l1) /* !yx */
             {
                   return(bAig_And(manager,nodeIndex2,bAig_Not(r1)));
             }
             if (nodeIndex2 == r1) /* !xy */
             {
                   return(bAig_And(manager,nodeIndex2,bAig_Not(l1)));
             }
       }

//        /* !x!(xy) || !y!(xy) = !x || !y */
        if (isInvertedNode2 && r2 != bAig_NULL && l2 != bAig_NULL)
        {
              if (nodeIndex1 == bAig_Not(l2) || nodeIndex1 == bAig_Not(r2))
              {
                 //incRefCount(manager, nodeIndex1);
                 return nodeIndex1;
              }

        }

       /* !(xy)!x || !(xy)!y = !x || !y */
       if (isInvertedNode1 && r1 != bAig_NULL && l1 != bAig_NULL)
       {
             if (nodeIndex2 == bAig_Not(l1) || nodeIndex2 == bAig_Not(r1))
             {
	       // incRefCount(manager, nodeIndex2);
               return nodeIndex2;
             }
       }

       /* Nessun nodo figlio duplicato */
       if (!isVarNode1 && !isVarNode2)
       {
             if (!isInvertedNode1 && !isInvertedNode2)
             {
                   /* (xy)(zy) || (xy)(zx) = (xy)z */
                   if (r1 == r2 || l1 == r2)
                   {
                         return(bAig_And(manager,nodeIndex1,l2));
                   }

                   /* (xy)(yz) || (xy)(xz) = (xy)z */
                   if (r1 == l2 || l1 == l2)
                   {
                         return(bAig_And(manager,nodeIndex1,r2));
                   }

                   /* (!xy)(zx) || (!xy)(xz) || (x!y)(zy) || (x!y)(yz) = 0 */
                   if (l1 == bAig_Not(r2) || l1 == bAig_Not(l2) || r1 == bAig_Not(r2) || r1 == bAig_Not(l2))
                   {
                         /* MaMu */
                         //incRefCount(manager, bAig_Zero);
                         /* MaMu */
                         return bAig_Zero;
                   }
             }

             if (!isInvertedNode1 && isInvertedNode2)
             {
                   /* (x!y)!(zy) || (x!y)!(yz) || (!xy)!(zx) || (!xy)!(xz) = (x!y) || (x!y) || (!xy) || (!xy) */
                   if (r1 == bAig_Not(r2) || r1 == bAig_Not(l2) || l1 == bAig_Not(r2) || l1 == bAig_Not(l2))
                   {
                         //incRefCount(manager, nodeIndex1);
                         return nodeIndex1;
                   }

                   /* (xy)!(yz) || (xy)!(xz) = (xy)!z */
                   if (r1 == l2 || l1 == l2)
                   {
                         return(bAig_And(manager,nodeIndex1,bAig_Not(r2)));
                   }

                   /* (xy)!(zy) || (xy)!(zx) = (xy)!z */
                   if (r1 == r2 || l1 == r2)
                   {
                         return(bAig_And(manager,nodeIndex1,bAig_Not(l2)));
                   }
             }

             if (isInvertedNode1 && !isInvertedNode2)
             {
                   /* !(xy)(z!y) || !(xy)(z!x) || !(xy)(!yz) || !(xy)(!xz) = (z!y) || (z!x) || (!yz) || (!xz) */
                   if (r2 == bAig_Not(r1) || r2 == bAig_Not(l1) || l2 == bAig_Not(r1) || l2 == bAig_Not(l1))
                   {
                         //incRefCount(manager, nodeIndex2);
                         return nodeIndex2;
                   }

                   /* !(xy)(zx) || !(xy)(xz) = (zx)!y || (xz)!y */
                   if (r2 == l1 || l2 == l1)
                   {
                         return(bAig_And(manager,nodeIndex2,bAig_Not(r1)));
                   }

                   /* !(xy)(zy) || !(xy)(yz) = (zy)!x || (yz)!x*/
                   if (r2 == r1 || l2 == r1)
                   {
                         return(bAig_And(manager,nodeIndex2,bAig_Not(l1)));
                   }

             }

             if (isInvertedNode1 && isInvertedNode2)
             {
                    /* !(xy)!(zy) = !(y(z+x)) */
                    if (r1 == r2)
                    {
                          tmp=bAig_Or(manager,l1,l2);
                    }
                    else if (r1 == l2)  /* !(xy)!(yz) = !(y(x+z)) */
                    {
                          tmp=bAig_Or(manager,r2,l1);
                    }

                    /* shared code */
                    if (r1 == r2 || r1 == l2)
                    {
                          incRefCount(manager, tmp);
                          newNodeIndex = bAig_Not(bAig_And(manager,r1,tmp));
                          incRefCount(manager, newNodeIndex);
                          bAig_RecursiveDeref(manager,tmp);
                          decRefCount(manager, newNodeIndex);
                          return(newNodeIndex);
                    }

                    /* !(xy)!(zx) = !(x(y+z)) */
                    if (l1 == r2)
                    {
                          tmp=bAig_Or(manager,r1,l2);
                    }

                    /* !(xy)!(xz) = !(x(y+z)) */
                    if (l1 == l2)
                    {
                          tmp=bAig_Or(manager,r1,r2);
                    }

                    /* shared code */
                    if (l1 == r2 || l1 == l2)
                    {
                          incRefCount(manager, tmp);
                          newNodeIndex = bAig_Not(bAig_And(manager,l1,tmp));
                          incRefCount(manager, newNodeIndex);
                          bAig_RecursiveDeref(manager,tmp);
                          decRefCount(manager, newNodeIndex);
                          return(newNodeIndex);
                    }

             }
       }
       /* MaMu */

       /* Look for the new node in the Hash table */
       newNodeIndex = HashTableLookup(manager, nodeIndex1, nodeIndex2);

       if (newNodeIndex == bAig_NULL)
       {
#if 0
         bAigEdge_t newAux1;
         if (!isInvertedNode1 && r1 != bAig_NULL && l1 != bAig_NULL) {
           newAix1 = HashTableLookup(manager, nodeIndex1, nodeIndex2);

         }
#endif
         

         /* MaMu */
         /* In the original source this piece of code was located here (*) */
         incRefCount(manager, nodeIndex1); /*Increment no of fanouts of this node */
         incRefCount(manager, nodeIndex2);
         /* In the original source this piece of code was located here (*) */
         /* MaMu */

         newNodeIndex = bAigCreateAndNode(manager, nodeIndex1, nodeIndex2) ;
         HashTableAdd(manager, newNodeIndex, nodeIndex1, nodeIndex2);
       }

       /* (*) */

       return newNodeIndex;

}

#else

/**Function********************************************************************

  Synopsis    [Performs the Logical AND of two nodes.]

  Description [This function performs the Logical AND of two nodes.  The inputs
            are the indices of the two nodes.  This function returns the index
            of the result node.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
bAigEdge_t
bAig_And(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex1,
   bAigEdge_t nodeIndex2
)
{
   bAigEdge_t newNodeIndex, r1, l1, r2, l2, tmp;

   if ( nodeIndex2 == bAig_Zero ) {
     return bAig_Zero;
   }
   if ( nodeIndex1 == bAig_Zero ) {
     return bAig_Zero;
   }
   if ( nodeIndex2 == bAig_One ) {
     return nodeIndex1;
   }
   if ( nodeIndex1 == bAig_One ) {
     return nodeIndex2;
   }
   if ( nodeIndex1 == nodeIndex2 ) {
     return nodeIndex1;
   }
   if ( nodeIndex1 == bAig_Not(nodeIndex2) ) {
     return bAig_Zero;
   }

   /* GpC: two level optimizations */

   r1=l1=r2=l2 = bAig_NULL;

   if (!bAig_isVarNode(manager,nodeIndex1)) {
     r1=rightChild(manager,nodeIndex1);
     l1=leftChild(manager,nodeIndex1);
   }
   if (!bAig_isVarNode(manager,nodeIndex2)) {
     r2=rightChild(manager,nodeIndex2);
     l2=leftChild(manager,nodeIndex2);
   }

   /* a(a*b) = a*b */
   if (!bAig_IsInverted(nodeIndex2) && r2 != bAig_NULL && l2 != bAig_NULL) {
     if (nodeIndex1 == r2) {
       return(bAig_And(manager,nodeIndex1,l2));
     }
     if (nodeIndex1 == l2) {
       return(bAig_And(manager,nodeIndex1,r2));
     }
   }
   if (!bAig_IsInverted(nodeIndex1) && r1 != bAig_NULL && l1 != bAig_NULL) {
     if (nodeIndex2 == r1) {
       return(bAig_And(manager,nodeIndex2,l1));
     }
     if (nodeIndex2 == l1) {
       return(bAig_And(manager,nodeIndex2,r1));
     }
   }
   /* a(!a*b) = 0 */
   if (!bAig_IsInverted(nodeIndex2) && r2 != bAig_NULL && l2 != bAig_NULL) {
     if (nodeIndex1 == bAig_Not(r2) || nodeIndex1 == bAig_Not(l2)) {
       return bAig_Zero;
     }
   }
   if (!bAig_IsInverted(nodeIndex1) && r1 != bAig_NULL && l1 != bAig_NULL) {
     if (nodeIndex2 == bAig_Not(r1) || nodeIndex2 == bAig_Not(l1)) {
       return bAig_Zero;
     }
   }
   /* a(!(a*b)) = !a*b */
   if (bAig_IsInverted(nodeIndex2) && r2 != bAig_NULL && l2 != bAig_NULL) {
     if (nodeIndex1 == r2) {
       return(bAig_And(manager,nodeIndex1,bAig_Not(l2)));
     }
     if (nodeIndex1 == l2) {
       return(bAig_And(manager,nodeIndex1,bAig_Not(r2)));
     }
   }
   if (bAig_IsInverted(nodeIndex1) && r1 != bAig_NULL && l1 != bAig_NULL) {
     if (nodeIndex2 == r1) {
       return(bAig_And(manager,nodeIndex2,bAig_Not(l1)));
     }
     if (nodeIndex2 == l1) {
       return(bAig_And(manager,nodeIndex2,bAig_Not(r1)));
     }
   }
   /* a(!(!a*b)) = a */
   if (bAig_IsInverted(nodeIndex2) && r2 != bAig_NULL && l2 != bAig_NULL) {
     if (nodeIndex1 == bAig_Not(r2)) {
       return nodeIndex1;
     }
     if (nodeIndex1 == bAig_Not(l2)) {
       return nodeIndex1;
     }
   }
   if (bAig_IsInverted(nodeIndex1) && r1 != bAig_NULL && l1 != bAig_NULL) {
     if (nodeIndex2 == bAig_Not(r1)) {
       return nodeIndex2;
     }
     if (nodeIndex2 == bAig_Not(l1)) {
       return nodeIndex2;
     }
   }

   if (!bAig_isVarNode(manager,nodeIndex1) &&
       !bAig_isVarNode(manager,nodeIndex2)) {

     if (!bAig_IsInverted(nodeIndex1) && !bAig_IsInverted(nodeIndex2)) {
       /* (a*b)(a*c) = (a*b)*c */
       if (r1 == r2 || l1 == r2) {
       return(bAig_And(manager,nodeIndex1,l2));
       }
       if (r1 == l2 || l1 == l2) {
       return(bAig_And(manager,nodeIndex1,r2));
       }
       /* (a*b)(!a*c) = 0 */
       if (r1 == bAig_Not(r2) || r1 == bAig_Not(l2)) {
       return bAig_Zero;
       }
       if (l1 == bAig_Not(r2) || l1 == bAig_Not(l2)) {
       return bAig_Zero;
       }
     }
     if (!bAig_IsInverted(nodeIndex1) && bAig_IsInverted(nodeIndex2)) {
       /* (a*b)*(!(!a*c)) = a*b */
       if (r1 == bAig_Not(r2) || r1 == bAig_Not(l2) ||
         l1 == bAig_Not(r2) || l1 == bAig_Not(l2)) {
       return nodeIndex1;
       }
       /* (a*b)*(!(a*c)) = (a*b)*!c */
       if (r1 == l2 || l1 == l2) {
       return(bAig_And(manager,nodeIndex1,bAig_Not(r2)));
       }
       if (r1 == r2 || l1 == r2) {
       return(bAig_And(manager,nodeIndex1,bAig_Not(l2)));
       }
     }
     if (bAig_IsInverted(nodeIndex1) && !bAig_IsInverted(nodeIndex2)) {
       /* (a*b)*(!(!a*c)) = a*b */
       if (r2 == bAig_Not(r1) || r2 == bAig_Not(l1) ||
         l2 == bAig_Not(r1) || l2 == bAig_Not(l1)) {
       return nodeIndex2;
       }
       /* (a*b)(!(a*c)) = (a*b)*!c */
       if (r2 == l1 || l2 == l1) {
       return(bAig_And(manager,nodeIndex2,bAig_Not(r1)));
       }
       if (r2 == r1 || l2 == r1) {
       return(bAig_And(manager,nodeIndex2,bAig_Not(l1)));
       }
     }
     if (bAig_IsInverted(nodeIndex1) && bAig_IsInverted(nodeIndex2)) {
       /* (!a+!b)*(!a+!c) = !a + (!b*!c) = !(a*(b+c)) */
       if (r1 == r2) {
         tmp=bAig_Or(manager,l1,l2);
         incRefCount(manager, tmp);
       newNodeIndex =
           bAig_Not(bAig_And(manager,r1,tmp));
         incRefCount(manager, newNodeIndex);
         bAig_RecursiveDeref(manager,tmp);
         decRefCount(manager, newNodeIndex);
       return(newNodeIndex);
       }
       if (r1 == l2) {
         tmp=bAig_Or(manager,r2,l1);
         incRefCount(manager, tmp);
       newNodeIndex =
         bAig_Not(bAig_And(manager,r1,tmp));
         incRefCount(manager, newNodeIndex);
         bAig_RecursiveDeref(manager,tmp);
         decRefCount(manager, newNodeIndex);
       return(newNodeIndex);
       }
       if (l1 == r2) {
         tmp=bAig_Or(manager,r1,l2);
         incRefCount(manager, tmp);
       newNodeIndex =
         bAig_Not(bAig_And(manager,l1,tmp));
         incRefCount(manager, newNodeIndex);
         bAig_RecursiveDeref(manager,tmp);
         decRefCount(manager, newNodeIndex);
       return(newNodeIndex);
       }
       if (l1 == l2) {
         tmp=bAig_Or(manager,r1,r2);
         incRefCount(manager, tmp);
       newNodeIndex =
         bAig_Not(bAig_And(manager,l1,tmp));
         incRefCount(manager, newNodeIndex);
         bAig_RecursiveDeref(manager,tmp);
         decRefCount(manager, newNodeIndex);
       return(newNodeIndex);
       }
     }

   }

   /********************************/

   newNodeIndex = nodeIndex1;     /* The left node has the smallest index */
   if (nodeIndex1 > nodeIndex2){
     nodeIndex1 = nodeIndex2;
     nodeIndex2 = newNodeIndex;
   }

   /* Look for the new node in the Hash table */
   newNodeIndex = HashTableLookup(manager, nodeIndex1, nodeIndex2);

   if (newNodeIndex == bAig_NULL){
     /*Increment no of fanouts of this node */
     incRefCount(manager, nodeIndex1);
     incRefCount(manager, nodeIndex2);
     newNodeIndex = bAigCreateAndNode(manager, nodeIndex1, nodeIndex2) ;
     HashTableAdd(manager, newNodeIndex, nodeIndex1, nodeIndex2);

   }

   return newNodeIndex;

}

#endif

/**Function*******************************************************************
   Synopsis    [Performs the Logical AND of two nodes.]
   Description [This function performs the Logical AND of two nodes.  The input
            are the indices of the two nodes.  This function returns the
                index of the result node.]
   SideEffects []
   SeeAlso     []
*****************************************************************************/
int
bAig_OrDecompose(
  bAig_Manager_t *manager,
  bAigEdge_t nodeIndex,
  bAigEdge_t *term0,
  bAigEdge_t *term1
)
{
  bAigEdge_t newNodeIndex, r, l;

   if ( nodeIndex == bAig_Zero || nodeIndex == bAig_One ) {
     return 0;
   }
   if (bAig_isVarNode(manager,nodeIndex)) {
     return 0;
   }

   r=rightChild(manager,nodeIndex);
   l=leftChild(manager,nodeIndex);

   if (bAig_IsInverted(nodeIndex)) {
     /* OR found */
     *term0 = bAig_Not(r);
     *term1 = bAig_Not(l);
     return 2;
   }
   if (bAig_IsInverted(r)) {
     bAigEdge_t tmp = r;
     r = l; l = tmp;
   }
   if (!bAig_IsInverted(r) && bAig_IsInverted(l)) {
     bAigEdge_t lr = rightChild(manager,l);
     bAigEdge_t ll = leftChild(manager,l);
     *term0 = bAig_And(manager,r,bAig_Not(lr));
     incRefCount(manager, *term0);
     *term1 = bAig_And(manager,r,bAig_Not(ll));
     incRefCount(manager, *term1);
     decRefCount(manager, *term0);
     decRefCount(manager, *term1);
     return 2;
   }
   if (bAig_IsInverted(l)) {
     bAigEdge_t lr = rightChild(manager,l);
     bAigEdge_t ll = leftChild(manager,l);
     *term0 = bAig_And(manager,r,bAig_Not(lr));
     incRefCount(manager, *term0);
     *term1 = bAig_And(manager,r,bAig_Not(ll));
     incRefCount(manager, *term1);
     decRefCount(manager, *term0);
     decRefCount(manager, *term1);
     return 2;
   }
   return 0;
}

/**Function********************************************************************

  Synopsis    [Count nodes of a bAig.]
  Description [Count nodes of a bAig.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
bAig_NodeCount(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex
)
{
   int count;

   count = nodeCountIntern(manager, nodeIndex);
   nodeClearVisitedIntern(manager, nodeIndex);

   return count;

}

/**Function********************************************************************

  Synopsis    [Count nodes of a bAig.]
  Description [Count nodes of a bAig.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
bAig_NodeCountWithBound(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex,
   int max
)
{
   int count;

   count = nodeCountInternWithBound(manager, nodeIndex, max);
   nodeClearVisitedIntern(manager, nodeIndex);

   return count;

}

/**Function********************************************************************

  Synopsis    [Count nodes of a bAig.]
  Description [Count nodes of a bAig.]
  SideEffects []
  SeeAlso     []

******************************************************************************/
int
bAig_FanoutNodeCount(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex,
   bAigEdge_t refIndex
)
{
   int count;

   count = nodeFanoutCountIntern(manager, nodeIndex, refIndex);
   nodeClearVisitedIntern(manager, nodeIndex);

   return count;

}

/**Function********************************************************************

  Synopsis    [Performs the Logical OR of two nodes.]

  Description [This function performs the Logical OR of two nodes.  The inputs
            are the indices of the two nodes.  This function returns the index
            of the result node.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
bAigEdge_t
bAig_Or(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex1,
   bAigEdge_t nodeIndex2
)
{
   return ( bAig_Not(bAig_And(manager,  bAig_Not(nodeIndex1),  bAig_Not(nodeIndex2))));
}

/**Function********************************************************************

  Synopsis    [Performs the Logical XOR of two nodes.]

  Description [This function performs the Logical XOR of two nodes.  The inputs
            are the indices of the two nodes.  This function returns the index
            of the result node.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
bAigEdge_t
bAig_Xor(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex1,
   bAigEdge_t nodeIndex2
)
{
  return bAig_Or(manager,
           bAig_And(manager, nodeIndex1, bAig_Not(nodeIndex2)),
           bAig_And(manager, bAig_Not(nodeIndex1),nodeIndex2)
       );
}

/**Function********************************************************************

  Synopsis    [Performs the Logical Equal ( <--> XNOR) of two nodes.]

  Description [This function performs the Logical XNOR of two nodes.  The inputs
            are the indices of the two nodes.  This function returns the index
            of the result node.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
bAigEdge_t
bAig_Eq(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex1,
   bAigEdge_t nodeIndex2
)
{
  return bAig_Not(
           bAig_Xor(manager, nodeIndex1, nodeIndex2)
           );
}

/**Function********************************************************************

  Synopsis    [Performs the Logical Then (--> Implies) of two nodes.]

  Description [This function performs the Logical Implies of two nodes.  The inputs
            are the indices of the two nodes.  This function returns the index
            of the result node.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
bAigEdge_t
bAig_Then(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex1,
   bAigEdge_t nodeIndex2
)
{
  return bAig_Or(manager,
             bAig_Not(nodeIndex1),
             nodeIndex2);
}

/**Function********************************************************************

  Synopsis    [create new node]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
bAigEdge_t
bAig_CreateNode(
   bAig_Manager_t  *manager,
   bAigEdge_t nodeIndex1,
   bAigEdge_t nodeIndex2
)
{

   bAigEdge_t newNode;
   assert(nodeIndex1!=bAig_NULL || nodeIndex2 == bAig_NULL);
   if (manager->freeNum > 0) {
     int i;
     if (manager->nodesArraySize > FREE_LIST_SORT_TH && 
         manager->freeCount > manager->nodesArraySize * 2 && 
         manager->freeNum > manager->nodesArraySize/8) { 
       /* free counts have scale factor 4 w.r.t. nodesArraySize */
       bAigManagerSortFreeList(manager);
       manager->freeCount = 0;
     }
     newNode = manager->freeList[--(manager->freeNum)];
     i = bAig_NonInvertedEdge(newNode)/bAigNodeSize;
     manager->auxChar[i] = 0;
     manager->visited[i] = 0;
     manager->auxInt[i] = -1;
     manager->auxInt1[i] = -1;
     manager->auxFloat[i] = 0;
     manager->cnfId[i] = 0;
     manager->auxRef[i] = 0;
     manager->auxPtr[i].pointer = NULL;
     manager->auxPtr0[i] = NULL;
     manager->auxPtr1[i] = NULL;
     manager->varPtr[i] = NULL;
     manager->auxAig0[i] = bAig_NULL;
     manager->auxAig1[i] = bAig_NULL;
     manager->cacheAig[i] = bAig_NULL;
   }
   else {
     newNode = manager->nodesArraySize;

     if (manager->nodesArraySize >= manager->maxNodesArraySize ){
       int i;
       manager->maxNodesArraySize = 2 * manager->maxNodesArraySize;
       manager->NodesArray =
       Pdtutil_Realloc(bAigEdge_t, manager->NodesArray, manager->maxNodesArraySize);
       manager->freeList =
       Pdtutil_Realloc(bAigEdge_t, manager->freeList,
             manager->maxNodesArraySize/bAigNodeSize);
       manager->nameList =
       Pdtutil_Realloc(char *, manager->nameList,
             manager->maxNodesArraySize/bAigNodeSize);
       manager->auxPtr =
       Pdtutil_Realloc(bAigPtrOrInt_t, manager->auxPtr,
             manager->maxNodesArraySize/bAigNodeSize);
       manager->auxPtr0 =
       Pdtutil_Realloc(void *, manager->auxPtr0,
             manager->maxNodesArraySize/bAigNodeSize);
       manager->auxPtr1 =
       Pdtutil_Realloc(void *, manager->auxPtr1,
             manager->maxNodesArraySize/bAigNodeSize);
       manager->varPtr =
       Pdtutil_Realloc(void *, manager->varPtr,
             manager->maxNodesArraySize/bAigNodeSize);
       manager->auxInt =
       Pdtutil_Realloc(int, manager->auxInt,
             manager->maxNodesArraySize/bAigNodeSize);
       manager->auxInt1 =
       Pdtutil_Realloc(int, manager->auxInt1,
             manager->maxNodesArraySize/bAigNodeSize);
       manager->auxFloat =
       Pdtutil_Realloc(float, manager->auxFloat,
             manager->maxNodesArraySize/bAigNodeSize);
       manager->cnfId =
       Pdtutil_Realloc(int, manager->cnfId,
             manager->maxNodesArraySize/bAigNodeSize);
       manager->auxRef =
       Pdtutil_Realloc(int, manager->auxRef,
             manager->maxNodesArraySize/bAigNodeSize);
       manager->auxAig0 =
       Pdtutil_Realloc(bAigEdge_t, manager->auxAig0,
             manager->maxNodesArraySize/bAigNodeSize);
       manager->auxAig1 =
       Pdtutil_Realloc(bAigEdge_t, manager->auxAig1,
             manager->maxNodesArraySize/bAigNodeSize);
       manager->cacheAig =
       Pdtutil_Realloc(bAigEdge_t, manager->cacheAig,
             manager->maxNodesArraySize/bAigNodeSize);
       manager->visited =
       Pdtutil_Realloc(unsigned char, manager->visited,
             manager->maxNodesArraySize/bAigNodeSize);
       manager->auxChar =
       Pdtutil_Realloc(unsigned char, manager->auxChar,
             manager->maxNodesArraySize/bAigNodeSize);
       for (i=(manager->maxNodesArraySize/2)/bAigNodeSize;
            i<manager->maxNodesArraySize/bAigNodeSize; i++) {
       manager->auxChar[i] = 0;
       manager->visited[i] = 0;
       manager->auxInt[i] = -1;
       manager->auxInt1[i] = -1;
       manager->auxFloat[i] = 0;
       manager->cnfId[i] = 0;
       manager->auxRef[i] = 0;
       manager->auxPtr[i].pointer = NULL;
       manager->auxPtr0[i] = NULL;
       manager->auxPtr1[i] = NULL;
       manager->varPtr[i] = NULL;
       manager->auxAig0[i] = 
       manager->auxAig1[i] = bAig_NULL;
       manager->cacheAig[i] = bAig_NULL;
       }
     }
     manager->nodesArraySize +=bAigNodeSize;
   }

   manager->NodesArray[bAig_NonInvertedEdge(newNode)+bAigRight] = nodeIndex2;
   manager->NodesArray[bAig_NonInvertedEdge(newNode)+bAigLeft]  = nodeIndex1;
   bnext(manager,newNode)     = bAig_NULL;
   refCount(manager,newNode) = 0;

   return(newNode);
}

/**Function********************************************************************

  Synopsis    [create var node ]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
bAigEdge_t
bAig_CreateVarNode(
   bAig_Manager_t  *manager,
   nameType_t         *name
)
{

   bAigEdge_t varIndex;
   char *myName;

   if (!st_lookup_int(manager->SymbolTable, name, (int*) &varIndex)){
     varIndex = bAig_CreateNode(manager, bAig_NULL, bAig_NULL);
     if (varIndex == bAig_NULL){
       return (varIndex);
     }
     /* Insert the varaible in the Symbol Table */
     myName = manager->nameList[bAigNodeID(varIndex)] = util_strsav(name);
     st_insert(manager->SymbolTable, myName, (char*) (long) varIndex);
     return(varIndex);
   }
   else {
     return (varIndex);
   }
}

/**Function********************************************************************

  Synopsis    [read var node ]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
bAigEdge_t
bAig_VarNodeFromName(
   bAig_Manager_t  *manager,
   nameType_t         *name
)
{

   bAigEdge_t varIndex;


   if (!st_lookup_int(manager->SymbolTable, name, (int*) &varIndex)){
     return (bAig_NULL);
   }
   else {
     return (varIndex);
   }
}

/**Function********************************************************************

  Synopsis [Return True if this node is Variable node]

  SideEffects []

******************************************************************************/
int
bAig_isVarNode(
   bAig_Manager_t *manager,
   bAigEdge_t      node
)
{
   //   if (bAig_NodeIsConstant(node)) return 0;
   if((rightChild(manager,node) == bAig_NULL) &&
      (leftChild(manager,node) == bAig_NULL)) {
#if 0
     if (bAig_NodeIsConstant(node)) {
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, "!!!!!!!!!!!.\n")
       );
     }
#endif
     return 1;
   }
   return 0;
}

/**Function********************************************************************

  Synopsis [Build the binary Aig for a given bdd]

  SideEffects [Build the binary Aig for a given bdd.
            We assume that the return bdd nodes from the foreach_bdd_node are
            in the order from childeren to parent. i.e all the childeren
            nodes are returned before the parent node.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
bAigEdge_t
bAig_bddtobAig(
   bAig_Manager_t *bAigManager,
   DdManager * bddManager,
   DdNode         *fn,
   char **varnames,
   int *cuToIds
)
{
   DdGen     *gen;
   DdNode      *node, *thenNode, *elseNode, *funcNode;
   DdNode      *one        = Cudd_ReadOne (bddManager);
   DdNode      *zero       = Cudd_Not (Cudd_ReadOne (bddManager));
   //int         is_complemented;

   bAigEdge_t  var, left, right, result, tmp1, tmp0;

   //char     *name;
   st_table *bddTObaigTable;
   st_generator *stgen;

   bddTObaigTable = st_init_table(st_numcmp, st_numhash);

   if (fn == NULL){
     return bAig_NULL;
   }

   st_insert(bddTObaigTable, (char *) one,
     (char *) bAig_One);

   funcNode = Cudd_Regular(fn);

   if (Cudd_IsConstant(fn)){
     if (fn == one) return bAig_One;
     else {
       assert (fn == zero);
       return bAig_Zero;
     }
   }

   Pdtutil_Assert(cuToIds!=NULL, "wrong cudd id conversion array");

   Cudd_ForeachNode(bddManager, funcNode, gen, node){
     if (!Cudd_IsConstant(node)) {
       int id, cuddId = Cudd_NodeReadIndex (node);

       Pdtutil_Assert(cuToIds[cuddId]>=0, "wrong cudd id");
       id = cuToIds[cuddId];

       if ((varnames != NULL) && (varnames[id] != NULL)) {
         var  = bAig_CreateVarNode(bAigManager, varnames[id]);
       }
       else {
         char name[100];
         sprintf(name,"BDD_%d", id);
         var  = bAig_CreateVarNode(bAigManager, name);
       }

       thenNode  = Cudd_T(node);
       st_lookup_int(bddTObaigTable, (char *) thenNode, (int*)&left);

       elseNode  = Cudd_Regular(Cudd_E(node));
       st_lookup_int(bddTObaigTable, (char *) elseNode, (int*) &right);
       if (Cudd_IsComplement(Cudd_E(node))) {
       right = bAig_Not(right);
       }
       tmp0 = bAig_And(bAigManager, var, left);
       incRefCount(bAigManager, tmp0);
       tmp1 = bAig_And(bAigManager, bAig_Not(var), right);
       incRefCount(bAigManager, tmp1);

       result = bAig_Or(bAigManager, tmp0, tmp1);
       incRefCount(bAigManager, result);
       bAig_RecursiveDeref(bAigManager,tmp0);
       bAig_RecursiveDeref(bAigManager,tmp1);
       st_insert(bddTObaigTable, (char *) node, (char *) (long) result);
     }
   }
   st_lookup_int(bddTObaigTable, (char *) funcNode, (int*) &result);
   incRefCount(bAigManager, result);
   st_foreach_item_int(bddTObaigTable, stgen, (char**)&tmp0, (int*)&tmp1){
     bAig_RecursiveDeref(bAigManager,tmp1);
   }
   st_free_table(bddTObaigTable);
   decRefCount(bAigManager, result);
   if (Cudd_IsComplement(fn)) {
     return bAig_Not(result);
   } else {
     return result;
   }
} /* end of bAig_bddtobAig() */

/**Function********************************************************************

  Synopsis [.]

  SideEffects []

******************************************************************************/
int
bAig_PrintDot(
  FILE *fp,
  bAig_Manager_t *manager
)
{
   int i;

   /*
    * Write out the header for the output file.
    */
   fprintf(fp, "digraph \"AndInv\" {\n  rotate=90;.\n");
   fprintf(fp, "  margin=0.5;\n  label=\"AndInv\";.\n");
   fprintf(fp, "  size=\"10,7.5\";\n  ratio=\"fill\";.\n");


   for (i=bAigFirstNodeIndex ; i< manager->nodesArraySize ; i+=bAigNodeSize){
     fprintf(fp,"Node%d  [label=\"%s \"];.\n",i,bAig_NodeReadName(manager, i));
   }
   for (i=bAigFirstNodeIndex ; i< manager->nodesArraySize ; i+=bAigNodeSize){
     if (rightChild(manager,i) != bAig_NULL){
       if (bAig_IsInverted(rightChild(manager,i))){
       fprintf(fp,"Node%d -> Node%d [color = red];\n",
           bAig_NonInvertedEdge(rightChild(manager,i)), i);
       }
       else{
       fprintf(fp,"Node%d -> Node%d;\n",
           bAig_NonInvertedEdge(rightChild(manager,i)), i);
       }
       if (bAig_IsInverted(leftChild(manager,i))){
       fprintf(fp,"Node%d -> Node%d [color = red];\n",
           bAig_NonInvertedEdge(leftChild(manager,i)), i);
       }
       else{
       fprintf(fp,"Node%d -> Node%d;\n",
         bAig_NonInvertedEdge(leftChild(manager,i)), i);
       }
     }/* if */
   }/*for */

   fprintf(fp, "}\n");

   return 1;
}

 /*---------------------------------------------------------------------------*/
 /* Definition of internal functions                                          */
 /*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Create AND node and assign name to it ]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
bAigEdge_t
bAigCreateAndNodeWithName(
   bAig_Manager_t  *manager,
   bAigEdge_t node1,
   bAigEdge_t node2
)
{

   bAigEdge_t varIndex;
   char       *name = NIL(char), *myName;
   char       *node1Str = util_inttostr(node1);
   char       *node2Str = util_inttostr(node2);

   name = util_strcat4("Nd", node1Str,"_", node2Str);
   while (st_lookup_int(manager->SymbolTable, name, &varIndex)){
     char *oldname = name;
     Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
       Pdtutil_VerbLevelNone_c,
       fprintf(stdout, "Find redundant node at %d %d.\n", node1, node2)
     );
     name = util_strcat3(name, node1Str, node2Str);
     Pdtutil_Free(oldname);
   }
   Pdtutil_Free(node1Str);
   Pdtutil_Free(node2Str);
   varIndex = bAig_CreateNode(manager, node1, node2);
   if (varIndex == bAig_NULL){
     Pdtutil_Free(name);
     return (varIndex);
   }
   /* Insert the variable in the Symbol Table */
   myName = manager->nameList[bAigNodeID(varIndex)] = util_strsav(name);
   Pdtutil_Free(name);

   return(varIndex);

}

/**Function********************************************************************

  Synopsis    [Create AND node and assign name to it ]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
bAigEdge_t
bAigCreateAndNode(
   bAig_Manager_t  *manager,
   bAigEdge_t node1,
   bAigEdge_t node2
)
{

   bAigEdge_t varIndex;

   varIndex = bAig_CreateNode(manager, node1, node2);
   if (varIndex == bAig_NULL){
     return (varIndex);
   }

   return(varIndex);

}


/**Function********************************************************************

  Synopsis    [Decrement the reference count of a bAig node.]

  Description [.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void
bAig_RecursiveDeref(
  bAig_Manager_t *manager,
  bAigEdge_t  node
)
{
   if (node == bAig_NULL) {
     return;
   }
   decRefCount(manager,node);
   if (refCount(manager,node) == 0) {
     if (!bAig_NodeIsConstant(node) && !bAig_isVarNode(manager,node)) {
       bAig_RecursiveDeref(manager,rightChild(manager,node));
       bAig_RecursiveDeref(manager,leftChild(manager,node));
       bAigDeleteAndNode(manager,node);
       manager->freeList[(manager->freeNum)++] = bAig_NonInvertedEdge(node);
       manager->freeCount++;
       HashTableDelete(manager,node);
       rightChild(manager,node) = bAig_NULL;
       leftChild(manager,node) = bAig_NULL;
       bnext(manager,node) = bAig_NULL;
       refCount(manager,node) = -1;
     }
   }
}

/**Function********************************************************************

  Synopsis    [Delete AND node and assign name to it ]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void
bAigDeleteAndNodeWithName(
   bAig_Manager_t  *manager,
   bAigEdge_t node
)
{
   char       *name;

  name = manager->nameList[bAigNodeID(node)];
  st_delete(manager->SymbolTable, &name, NULL);
  assert(name == manager->nameList[bAigNodeID(node)]);
  Pdtutil_Free(name);
  manager->nameList[bAigNodeID(node)] = NULL;
}

/**Function********************************************************************

  Synopsis    [Delete AND node and assign name to it ]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void
bAigDeleteAndNode(
   bAig_Manager_t  *manager,
   bAigEdge_t node
)
{
   char       *name;

  manager->nameList[bAigNodeID(node)] = NULL;
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Increment the reference count of a bAig node.]

  Description [.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
static void
incRefCount(
  bAig_Manager_t *manager,
  bAigEdge_t  nodeIndex
)
{
  if (!bAig_NodeIsConstant(nodeIndex) &&
        !bAig_isVarNode(manager,nodeIndex)) {
    assert (refCount(manager,nodeIndex) >= 0);
    refCount(manager,nodeIndex)++;
#if 0
    if (nodeIndex/4 == 9034) {
      printf("found+: %d\n", refCount(manager,nodeIndex));
    }
#endif
  }
}

/**Function********************************************************************

  Synopsis    [Decrement the reference count of a bAig node.]

  Description [.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
static void
decRefCount(
  bAig_Manager_t *manager,
  bAigEdge_t  nodeIndex
)
{
    if (!bAig_NodeIsConstant(nodeIndex) &&
        !bAig_isVarNode(manager,nodeIndex)) {
      int r;
      r = refCount(manager,nodeIndex);
      refCount(manager,nodeIndex)--;
      assert (refCount(manager,bAig_NonInvertedEdge(nodeIndex))>=0);
      assert(refCount(manager,nodeIndex)==r-1);
#if 0
      if (nodeIndex/4 == 9034) {
	printf("found-: %d\n", refCount(manager,nodeIndex));
      }
#endif
    }
}

/**Function********************************************************************

  Synopsis [Return result of hash lookup. If and(node1,node2) node exists, 
  returned]

  SideEffects []

******************************************************************************/
int
bAig_hashInsert(
   bAig_Manager_t *manager,
   bAigEdge_t      parent,
   bAigEdge_t      node1,
   bAigEdge_t      node2
)
{
  int ret;
  if ( 1 && (node1 > node2)) {
    bAigEdge_t  tmp = node1;
    node1 = node2;
    node2 = tmp;
  }
  ret = HashTableAdd(manager, parent, node1, node2);
  return ret;
}

/**Function********************************************************************

  Synopsis [Return result of hash lookup. If and(node1,node2) node exists, 
  returned]

  SideEffects []

******************************************************************************/
bAigEdge_t
bAig_hashLookup(
   bAig_Manager_t *manager,
   bAigEdge_t      node1,
   bAigEdge_t      node2
)
{
  bAigEdge_t  ret;
  if (1 && (node1 > node2)) {
    bAigEdge_t  tmp = node1;
    node1 = node2;
    node2 = tmp;
  }
  ret = HashTableLookup(manager, node1, node2);

  return ret;
}

/**Function********************************************************************

  Synopsis    [Look for the node in the Hash Table.]

  Description [.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
static bAigEdge_t
HashTableLookup(
  bAig_Manager_t *manager,
  bAigEdge_t  node1,
  bAigEdge_t  node2
)
{
  bAigEdge_t key = HashTableFunction(node1, node2);
  bAigEdge_t node;

  node = manager->HashTable[key];
  if  (node == bAig_NULL) {
    return bAig_NULL;
  }
  else{
    while ( (rightChild(manager,bAig_NonInvertedEdge(node)) != node2) ||
          (leftChild(manager,bAig_NonInvertedEdge(node))  != node1)) {

      if (bnext(manager,bAig_NonInvertedEdge(node)) == bAig_NULL){
      return(bAig_NULL);
      }
      node = bnext(manager,bAig_NonInvertedEdge(node)); /* Get the next Node */
    } /* While loop */
    return(node);

  } /* If Then Else */

} /* End of HashTableLookup() */

/**Function********************************************************************

  Synopsis    [Look for the node in the Hash Table.]

  Description [.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
static void
HashTableDelete(
  bAig_Manager_t *manager,
  bAigEdge_t  node
)
{
  bAigEdge_t key;
  bAigEdge_t listNode, right, left;
  assert (!bAig_NodeIsConstant(node) && !bAig_isVarNode(manager,node));

  node = bAig_NonInvertedEdge(node);

  right = rightChild(manager,node);
  left = leftChild(manager,node);
  if (left<right) {
    listNode = left;
    left = right;
    right = listNode;
  }
  key = HashTableFunction(right,left);

  listNode = manager->HashTable[key];
  assert(listNode != bAig_NULL);
  if (listNode == node) {
    manager->HashTable[key] = bnext(manager,node);
  }
  else {
    bAigEdge_t nextNode = bnext(manager,bAig_NonInvertedEdge(listNode));
    while (nextNode != node) {
      listNode = nextNode;
      nextNode = bnext(manager,bAig_NonInvertedEdge(listNode));
      assert(nextNode!=bAig_NULL);
    }
    bnext(manager,bAig_NonInvertedEdge(listNode)) =
      bnext(manager,node);
  }
} /* End of HashTableLookup() */

/**Function********************************************************************

  Synopsis    [Add a node in the Hash Table.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
#define HEAD_INSERT 1
static int
HashTableAdd(
  bAig_Manager_t   *manager,
  bAigEdge_t  nodeIndexParent,
  bAigEdge_t  nodeIndex1,
  bAigEdge_t  nodeIndex2
)
{
  bAigEdge_t key = HashTableFunction(nodeIndex1, nodeIndex2);
  bAigEdge_t nodeIndex;
  bAigEdge_t node;

  nodeIndex = manager->HashTable[key];
#if HEAD_INSERT
  bnext(manager,bAig_NonInvertedEdge(nodeIndexParent)) = nodeIndex;
  manager->HashTable[key] = nodeIndexParent;
  return TRUE;
#else
  if  (nodeIndex == bAig_NULL) {
    manager->HashTable[key] = nodeIndexParent;
    return TRUE;
  }
  else{
    node = nodeIndex;
    /* Get the Node */
    nodeIndex = bnext(manager,bAig_NonInvertedEdge(nodeIndex));
    while (nodeIndex != bAig_NULL) {
      node = nodeIndex;
      nodeIndex = bnext(manager,bAig_NonInvertedEdge(nodeIndex));
    }
    bnext(manager,bAig_NonInvertedEdge(node)) = nodeIndexParent;
    return TRUE;
  }
#endif

} /* End of HashTableAdd() */




/**Function********************************************************************

  Synopsis    [Count nodes of a bAig. Internal recursion]
  Description [Count nodes of a bAig. Internal recursion]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
nodeCountIntern(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex
)
{
  int count;

  if ( nodeIndex == bAig_Zero || nodeIndex == bAig_One ) {
    return 0;
  }

  if (nodeVisited(manager,nodeIndex)) {
    return 0;
  }

  nodeSetVisited(manager,nodeIndex);

  if (bAig_isVarNode(manager,nodeIndex)) {
    return 1;
  }

  count = 1;
  count += nodeCountIntern(manager,rightChild(manager,nodeIndex));
  count += nodeCountIntern(manager,leftChild(manager,nodeIndex));

  return (count);
}

/**Function********************************************************************

  Synopsis    [Count nodes of a bAig. Internal recursion]
  Description [Count nodes of a bAig. Internal recursion]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
nodeCountInternWithBound(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex,
   int max
)
{
  int count;

  if (max<=0) return 0;

  if ( nodeIndex == bAig_Zero || nodeIndex == bAig_One ) {
    return 0;
  }

  if (nodeVisited(manager,nodeIndex)) {
    return 0;
  }

  nodeSetVisited(manager,nodeIndex);

  if (bAig_isVarNode(manager,nodeIndex)) {
    return 1;
  }

  count = 1;
  count += nodeCountInternWithBound(manager,rightChild(manager,nodeIndex),
				    max-count);
  count += nodeCountInternWithBound(manager,leftChild(manager,nodeIndex),
				    max-count);

  return (count);
}


/**Function********************************************************************

  Synopsis    [Count nodes of a bAig. Internal recursion]
  Description [Count nodes of a bAig. Internal recursion]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
nodeFanoutCountIntern(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex,
   bAigEdge_t refIndex
)
{
  int count;

  if ( nodeIndex == bAig_Zero || nodeIndex == bAig_One ) {
    return 0;
  }

  if (nodeVisited(manager,nodeIndex)) {
    return 0;
  }

  nodeSetVisited(manager,nodeIndex);

  if (bAig_isVarNode(manager,nodeIndex)) {
    if (bAig_NonInvertedEdge(nodeIndex) == bAig_NonInvertedEdge(refIndex)) {
      return 1;
    }
    else {
      return 0;
    }
  }

  count = nodeFanoutCountIntern(manager,
            rightChild(manager,nodeIndex),refIndex);
  count += nodeFanoutCountIntern(manager,
             leftChild(manager,nodeIndex),refIndex);
  if (count > 0) count++;

  return (count);
}


/**Function********************************************************************

  Synopsis    [Clear Visited flag]
  Description [Clear Visited flag]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
nodeClearVisitedIntern(
   bAig_Manager_t *manager,
   bAigEdge_t nodeIndex
)
{
  //int count;

  if ( nodeIndex == bAig_Zero || nodeIndex == bAig_One ) {
    return;
  }

  if (!nodeVisited(manager,nodeIndex)) {
    return;
  }

  nodeClearVisited(manager,nodeIndex);

  if (bAig_isVarNode(manager,nodeIndex)) {
    return;
  }

  nodeClearVisitedIntern(manager,rightChild(manager,nodeIndex));
  nodeClearVisitedIntern(manager,leftChild(manager,nodeIndex));

}





/**Function********************************************************************
  Synopsis    []
  Description [util_inttostr -- converts an integer into a string]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static char *
util_inttostr(
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
    len += (unsigned) floor(log10(mod)) + 1;
  }

  s = Pdtutil_Alloc(char, len + 1);
  sprintf(s, "%d", i);

  return s;
}

/**Function********************************************************************
  Synopsis    []
  Description [util_strcat3 -- Creates a new string which is the
    concatenation of 3 strings.
    It is the responsibility of the caller to free this string using Pdtutil_Free.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static char *
util_strcat3(
  char * str1,
  char * str2,
  char * str3)
{
  char *str = Pdtutil_Alloc(char, strlen(str1) + strlen(str2) + strlen(str3) + 1);

  (void) strcpy(str, str1);
  (void) strcat(str, str2);
  (void) strcat(str, str3);

  return (str);
}

/**Function********************************************************************
  Synopsis    []
  Description [util_strcat3 -- Creates a new string which is the
    concatenation of 4 strings.
    It is the responsibility of the caller to free this string using Pdtutil_Free.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static char *
util_strcat4(
  char * str1,
  char * str2,
  char * str3,
  char * str4
)
{
  char *str = Pdtutil_Alloc(char, strlen(str1) + strlen(str2) + strlen(str3) +
                    strlen(str4) + 1);

  (void) strcpy(str, str1);
  (void) strcat(str, str2);
  (void) strcat(str, str3);
  (void) strcat(str, str4);

  return (str);
}




/* MaMu */

/**Function********************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

******************************************************************************/
static int recursiveFind(bAig_Manager_t *manager,
                             bAigEdge_t *candidates,
                             unsigned long *candidatesSize,
                             bAigEdge_t node,
                             unsigned char flags,
                          unsigned long currDepth,
                             unsigned long maxDepth,
                             recurseState state,
                             bAigEdge_t parent,
                             unsigned char direction)
{
    unsigned long nodeRefCount = 0;
    int lRes, rRes;
    unsigned long i;

    // already explored or not valid node (constant, primary input...)
    if (nodeVisited(manager, node) ||
        bAig_NodeIsConstant(node) ||
        bAig_isVarNode(manager, node) ||
        refCount(manager, node) == -1 ||
        currDepth > maxDepth)
        return (-1);

    // not explored, recurse in order
    for (i=0;i<(candidatesSize[0]);i++)
    {
        // new node is referenced into the current set
        unsigned char l_ref = (bAig_NonInvertedEdge(leftChild(manager, candidates[i])) ==
                               bAig_NonInvertedEdge(node));
        unsigned char r_ref = (bAig_NonInvertedEdge(rightChild(manager, candidates[i])) ==
                               bAig_NonInvertedEdge(node));

        // number of refs
        nodeRefCount += (unsigned long) (l_ref || r_ref);
    }

    // new node referenced only in the current set
    if ((refCount(manager, node) - nodeRefCount) == 0 || candidatesSize[0] == 0)
    {
        candidates[candidatesSize[0]] = bAig_NonInvertedEdge(node);
        candidatesSize[0]++;
        nodeSetVisited(manager, node);

        bAigEdge_t l = leftChild(manager, node);
        bAigEdge_t r = rightChild(manager, node);

        // Structural reduction
        if ((flags & bAigReduceStruct) == bAigReduceStruct)
        {
            recurseState state_l, state_r;

            // recursive state machine: advances with recursion if target
            // is reached through the net
            switch (state)
            {
                case START:
                case ITE_REACHED:
                case XOR_REACHED:
                    state_l = bAig_IsInverted(l) ? OR_LEVEL : START;
                    state_r = bAig_IsInverted(r) ? OR_LEVEL : START;

                    if (state_l == state_r && state_l == START)
                    {
                        candidatesSize[0]--;
                        nodeClearVisited(manager, node);

                        return (-1);
                    }
                    break;
                case OR_LEVEL:
                    state_l = state_r = bAig_IsInverted(l) && bAig_IsInverted(r) ? AND_LEVEL : START;

                    if (state_l == START)
                    {
                        candidatesSize[0]--;
                        nodeClearVisited(manager, node);

                        return (-1);
                    }
                    break;
                case AND_LEVEL:
                    if (direction == D_LEFT)
                    {
                        // XOR
                        if (l == bAig_Not(leftChild(manager, rightChild(manager, parent))) &&
                            r == bAig_Not(rightChild(manager, rightChild(manager, parent))))
                            state_l = state_r = XOR_REACHED;
                        // ITE
                        else if ((l == bAig_Not(leftChild(manager, rightChild(manager, parent))) &&
                                  r == rightChild(manager, rightChild(manager, parent))) ||
                                 (r == bAig_Not(rightChild(manager, rightChild(manager, parent))) &&
                                  l == leftChild(manager, rightChild(manager, parent))))
                            state_l = state_r = ITE_REACHED;
                    }
                    else if (direction == D_RIGHT)
                    {
                        // XOR
                        if (l == bAig_Not(leftChild(manager, leftChild(manager, parent))) &&
                            r == bAig_Not(rightChild(manager, leftChild(manager, parent))))
                            state_l = state_r = XOR_REACHED;
                        // ITE
                        else if ((l == bAig_Not(leftChild(manager, leftChild(manager, parent))) &&
                                  r == rightChild(manager, leftChild(manager, parent))) ||
                                 (r == bAig_Not(rightChild(manager, leftChild(manager, parent))) &&
                                  l == leftChild(manager, leftChild(manager, parent))))
                            state_l = state_r = ITE_REACHED;
                    }
                    else
                    {
                        state_l = state_r = START;
                    }

                    if (state_l == START)
                    {
                        candidatesSize[0]--;
                        nodeClearVisited(manager, node);

                        return (-1);
                    }
                    break;
                default:
                    state_l = state_r = START;
            }

            lRes = recursiveFind(manager,
                                 candidates,
                                 candidatesSize,
                                 l,
                                 flags,
                                 currDepth + 1,
                                 maxDepth,
                                 state_l,
                                 node,
                                 D_LEFT);

            rRes = recursiveFind(manager,
                                 candidates,
                                 candidatesSize,
                                 r,
                                 flags,
                                 currDepth + 1,
                                 maxDepth,
                                 state_r,
                                 node,
                                 D_RIGHT);
        }
        else
        {
            lRes = recursiveFind(manager,
                            candidates,
                            candidatesSize,
                            l,
                            flags,
                            currDepth + 1,
                            maxDepth,
                            START,
                            node,
                            D_LEFT);

            rRes = recursiveFind(manager,
                            candidates,
                            candidatesSize,
                            r,
                            flags,
                            currDepth + 1,
                            maxDepth,
                            START,
                            node,
                            D_RIGHT);
        }
    }

    return (0);
}

/* returns TRUE if the element node is contained into the relative cluster */
static unsigned char isNodeInCluster(bAigEdge_t *candidates,
                                     unsigned long candidatesSize,
                                       bAigEdge_t node)
{
    unsigned long i;

    if (candidatesSize == 0)
        return (FALSE);

    for (i = 0; i < candidatesSize; i++)
    {
        // success, match
        if (candidates[i] == bAig_NonInvertedEdge(node))
            return (TRUE);
    }

    // failure, no match
    return (FALSE);
}

/* returns TRUE if node is in the "free variables list" (Primary Input of a cluster) */
static unsigned char isNodeInFreeVarsList(bAigEdge_t *freeVarsList,
                                   unsigned long freeVarsListSize,
                                   bAigEdge_t node)
{
    unsigned long i;

    if (freeVarsListSize == 0)
        return FALSE;

    for(i=0; i< freeVarsListSize; i++)
    {
        if (freeVarsList[i] == bAig_NonInvertedEdge(node))
            return TRUE;
    }

    return FALSE;
}

/* builds a blif net from an aig subset, returns result in mmapped obj */
static void baig2blif(FILE* fp,
               bAig_Manager_t *manager,
               bAigEdge_t *candidates,
               unsigned long candidatesSize)
{
    unsigned long freeVarsListSize = 0;
    unsigned long inputStrLen = MAX_LINESIZE + 1;
    bAigEdge_t *freeVarsList = Pdtutil_Alloc(bAigEdge_t, 2*candidatesSize);
    unsigned long i;
    bAigEdge_t l, r;
    unsigned char isInvertedL, isInvertedR;
    char **body_lines = Pdtutil_Alloc(char *, 2*candidatesSize);
    char **header_lines = Pdtutil_Alloc(char *, 4);
    char *tmpstr1 = Pdtutil_Alloc(char, MAX_LINESIZE + 1);
    char *tmpstr2 = Pdtutil_Alloc(char, MAX_LINESIZE + 1);
    char *end_line = Pdtutil_Alloc(char, MAX_LINESIZE + 1);

    // default output
    if (fp == NULL)
        fp = stdout;

    for (i=0;i<4;i++)
        header_lines[i] = Pdtutil_Alloc(char, MAX_LINESIZE + 1);

    for (i=0; i < 2*candidatesSize; i++)
        body_lines[i] = Pdtutil_Alloc(char , MAX_LINESIZE + 1);

    /* builds nodes output functions */
    for (i = 0; i < candidatesSize; i++)
    {
        // Node depending on other ones
        l = leftChild(manager,candidates[i]);
        r = rightChild(manager,candidates[i]);
        isInvertedL = bAig_IsInverted(l);
        isInvertedR = bAig_IsInverted(r);

        sprintf(body_lines[2*i], "%s %s%d %s%d %s%d\r\n", ".names",
                "_n", bAig_NonInvertedEdge(l),
                "_n", bAig_NonInvertedEdge(r),
                "_n", candidates[i]);

        if (!isInvertedL && !isInvertedR)
            sprintf(body_lines[2*i+1], "%d%d %d\r\n", 1,1,1);

        if (!isInvertedL && isInvertedR)
            sprintf(body_lines[2*i+1], "%d%d %d\r\n", 1,0,1);

        if (isInvertedL && !isInvertedR)
            sprintf(body_lines[2*i+1], "%d%d %d\r\n", 0,1,1);

        if (isInvertedL && isInvertedR)
            sprintf(body_lines[2*i+1], "%d%d %d\r\n", 0,0,1);

        // Adding children to free variable list
        if (!isNodeInCluster(candidates, candidatesSize, bAig_NonInvertedEdge(l)))
        {
            if (!isNodeInFreeVarsList(freeVarsList, freeVarsListSize, bAig_NonInvertedEdge(l)))
            {
                freeVarsList[freeVarsListSize++] = bAig_NonInvertedEdge(l);
            }
        }

        if (!isNodeInCluster(candidates, candidatesSize, bAig_NonInvertedEdge(r)))
        {
            if (!isNodeInFreeVarsList(freeVarsList, freeVarsListSize, bAig_NonInvertedEdge(r)))
            {
                freeVarsList[freeVarsListSize++] = bAig_NonInvertedEdge(r);
            }
        }
    }

    if (freeVarsListSize > 0)
    {
        /* stores blif headers */
        sprintf(header_lines[0], "%s", "# Automatic generated BLIF file - FBV framework\r\n");
        sprintf(header_lines[1], "%s %s%d\r\n", ".model", "node", candidates[0/*clusterMarker*/]);
        /* inputs */
        sprintf(header_lines[2], "%s ", ".inputs");
        for (i=0; i< freeVarsListSize; i++)
        {
            sprintf(tmpstr1, "%s%d ", "_n", bAig_NonInvertedEdge(freeVarsList[i]));
            sprintf(tmpstr2, "%s", header_lines[2]);

            if ((strlen(tmpstr2) + strlen(tmpstr1) + 2 + strlen("\\\n")) > inputStrLen)
            {
                Pdtutil_Realloc(char *, header_lines[2], 2*inputStrLen);
                inputStrLen *= 2;
            }

            if (i%10 == 0 && i!= 0)
                sprintf(header_lines[2], "%s%s \\\n", tmpstr2, tmpstr1);
            else
                sprintf(header_lines[2], "%s%s", tmpstr2, tmpstr1);

        }

        sprintf(tmpstr1, "%s", "\r\n");
        sprintf(tmpstr2, "%s", header_lines[2]);
        if ((strlen(tmpstr2) + strlen(tmpstr1) + 2) > inputStrLen)
        {
            Pdtutil_Realloc(char *, header_lines[2], 2*inputStrLen);
            inputStrLen *= 2;
        }
        sprintf(header_lines[2], "%s%s", tmpstr2, tmpstr1);

        /* outputs */
        sprintf(header_lines[3], "%s %s%d\r\n", ".outputs", "_n", candidates[0]);

        /* end of model */
        sprintf(end_line, "%s", ".end\r\n\r\n");

        /* prints blif net on file */
        for (i=0;i<4;i++)
            fwrite(header_lines[i], strlen(header_lines[i]), 1, fp);

        // reversed file, from cuts to output
        for (i = 0; i < candidatesSize; i++)
        {
            fwrite(body_lines[2*i], strlen(body_lines[2*i]),1, fp);
            fwrite(body_lines[2*i+1], strlen(body_lines[2*i+1]), 1, fp);
        }

        fwrite(end_line, strlen(end_line), 1, fp);
        fflush(fp);
    }
    else
    {
        fprintf(stderr, "Warning - No BLIF net written to file. Empty net...");
    }

    /* frees memory */
    for (i=0;i<4;i++)
    {
        Pdtutil_Free(header_lines[i]); header_lines[i] = NULL;
    }

    for (i=0; i < 2*candidatesSize; i++)
    {
        Pdtutil_Free(body_lines[i]); body_lines[i] = NULL;
    }
    Pdtutil_Free(body_lines); body_lines = NULL;

    Pdtutil_Free(tmpstr1); tmpstr1 = NULL;
    Pdtutil_Free(tmpstr2); tmpstr2 = NULL;

    Pdtutil_Free(end_line); end_line = NULL;

    Pdtutil_Free(freeVarsList); freeVarsList = NULL;
}

void deleteNodeNoDeref(bAig_Manager_t *manager, bAigEdge_t node)
{
    bAigDeleteAndNode(manager,node);
    manager->freeList[(manager->freeNum)++] = node;
    manager->freeCount++;
    HashTableDelete(manager,node);
    rightChild(manager,node) = bAig_NULL;
    leftChild(manager,node) = bAig_NULL;
    bnext(manager,node) = bAig_NULL;
    refCount(manager,node) = -1;
}

void emergeNode(bAig_Manager_t *manager, bAigEdge_t node)
{
    unsigned long i, j;

    for (i=0;i<manager->freeNum;i++)
    {
        // emerges node and shifts the others
        if (manager->freeList[i] ==  node)
        {
            for (j=i;j<(manager->freeNum-1);j++)
            {
                manager->freeList[j] = manager->freeList[j+1];
            }
            manager->freeList[manager->freeNum-1] = node;
        }
    }
}

/* restructures the net */
void doRestruct(bAig_Manager_t *manager, bAigEdge_t **restruct, unsigned long restructSize)
{
    unsigned char finished = FALSE;
    unsigned long i,k;

    // TODO: Modify this block!!!!
    // Searches for each node into the freeList,
    // swapping the others on the left. Iterative cost
    // is high, for a large cluster
    while (!finished)
    {
        finished = TRUE;
        for (i=0;i<restructSize;i++)
        {
            // reversed visit to nodes grants consistency
            // into optimized net
            unsigned long j = restructSize - i - 1;
            if (restruct[0][4*j+3] == FALSE)
            {
                emergeNode(manager, restruct[0][4*j]);

                // new node
                bAigEdge_t tmp = bAig_And(manager, restruct[0][4*j+1], restruct[0][4*j+2]);
                if (tmp != bAig_NULL)
                {
                            // save restructured net info
                    restruct[0][4*j+3] = TRUE;

                            // updates related nodes into the reconstruction net
                    for (k=0;k<restructSize;k++)
                    {
                        unsigned char inv;

                        // left child
                        if (j!=k &&
                            restruct[0][4*j] == bAig_NonInvertedEdge(restruct[0][4*k+1]) &&
                            restruct[0][4*k+3] == FALSE)
                        {
                            inv = bAig_IsInverted(restruct[0][4*k+1]);
                            restruct[0][4*k+1] = inv ? bAig_Not(tmp) : tmp;
                        }

                        // right child
                        if (j!=k &&
                            restruct[0][4*j] == bAig_NonInvertedEdge(restruct[0][4*k+2]) &&
                            restruct[0][4*k+3] == FALSE)
                        {
                            inv = bAig_IsInverted(restruct[0][4*k+2]);
                            restruct[0][4*k+2] = inv ? bAig_Not(tmp) : tmp;
                        }
                    }
                    restruct[0][4*j] = tmp;
                }
            }
            finished = (finished && restruct[0][4*j+3]);
        }
    }
}

void buildRestruct(FILE *fp, char *buff, bAigEdge_t **restruct, unsigned long *restructSize)
{
    bAigEdge_t l, r, v, tmp;
    unsigned char inv_l, inv_r;//, inv_v;
    char *p;
    char *dummy = Pdtutil_Alloc(char, MAX_NAMESIZE + 1);
    char *name_l = Pdtutil_Alloc(char, MAX_NAMESIZE + 1);
    char *name_r = Pdtutil_Alloc(char, MAX_NAMESIZE + 1);
    char *name_v = Pdtutil_Alloc(char, MAX_NAMESIZE + 1);

    int res = sscanf(buff, "%s %s %s %s", dummy, name_l, name_r, name_v);

    if (res == 4) // existing node updated
    {
        l = (bAigEdge_t) atol(&(name_l[2]));

        r = (bAigEdge_t) atol(&(name_r[2]));

        v = (bAigEdge_t) atol(&(name_v[2]));

        if (l>r)
        {
            tmp = l; l = r; r = tmp;
        }
    }
    else if (res == 3) // redundant node
    {
        l = bAig_One;

        r = (bAigEdge_t) atol(&(name_l[2]));

        v = (bAigEdge_t) atol(&(name_r[2]));
    }
    else // nodo eliminato
    {
        restruct[0][4*restructSize[0]+3] = (bAigEdge_t) TRUE;        // not to be processed
        restructSize[0]++;
        return;
    }

            // reads boolean mapping
    fgets(buff, MAX_LINESIZE, fp);

    // exception, suppressed node
    if ((p = strstr(buff, ".names")) != NULL)
    {
                // TODO: Any exceptional behaviour?
                // back to strlen(...) characters into stream
        fseek(fp, (long) (-1 * strlen(buff)), SEEK_CUR);
        restruct[0][4*restructSize[0]+3] = (bAigEdge_t) TRUE;        // not to be processed
        restructSize[0]++;
        return;
    }

    // AND node
    if (res == 4)
    {
        inv_l = ((buff[0] - '0') == 0);
        inv_r = ((buff[1] - '0') == 0);
    }
    else if (res == 3) // sets a node in AND with 1
    {
        inv_l = 0;
        inv_r = ((buff[0] - '0') == 0);
    }

    // Adds into restruct vector
    if (res == 3 || res == 4)
    {
        restruct[0][4*restructSize[0]] = v;                          // node
        restruct[0][4*restructSize[0]+1] = inv_l ? bAig_Not(l) : l; // left child
        restruct[0][4*restructSize[0]+2] = inv_r ? bAig_Not(r) : r; // right child
        restruct[0][4*restructSize[0]+3] = (bAigEdge_t) FALSE;
        restructSize[0]++;
    }

    Pdtutil_Free(dummy); dummy = NULL;
    Pdtutil_Free(name_l); name_l = NULL;
    Pdtutil_Free(name_r); name_r = NULL;
    Pdtutil_Free(name_v); name_v = NULL;
}

// parses blif optimized net to retrieve changes into the target net
void blif2baig(FILE *fp,
               bAig_Manager_t *manager,
               bAigEdge_t *candidates,
               unsigned long candidatesSize)
{
    unsigned long i;
    char *p;
    unsigned long restructSize = 0;
    readBlifState state = INIT;
    char *buff = Pdtutil_Alloc(char, MAX_LINESIZE + 1);
    bAigEdge_t *restruct = Pdtutil_Alloc(bAigEdge_t, (unsigned long) 4*candidatesSize);

    while((fgets(buff, MAX_LINESIZE, fp))!=NULL)
    {

        if (strstr(buff, ".model") != NULL && state == INIT)
        {
            state = HEADER;
            continue;
        }

        if ((p = strstr(buff, ".outputs")) != NULL && state == INPUTS)
        {
            state = OUTPUTS;
            continue;
        }

        if (((p = strstr(buff, ".inputs")) != NULL &&
            state == HEADER)
            || state == INPUTS)
        {
            state = INPUTS;
            continue;
        }

        if ((p = strstr(buff, ".end")) != NULL && state == NAMES)
        {
            state = END;

            // Deletes all candidates node from the bAig Manager Array
            for (i=0;i<candidatesSize; i++)
            {
                deleteNodeNoDeref(manager, candidates[i]);
            }
            // restructures tree iteratively
            doRestruct(manager, &restruct, restructSize);

            break;
        }

        if (((p = strstr(buff, ".names")) != NULL && state == OUTPUTS) || state == NAMES)
        {
            state = NAMES;
            buildRestruct(fp, buff, &restruct, &restructSize);
            continue;
        }
    }

    Pdtutil_Free(restruct); restruct = NULL;
    Pdtutil_Free(buff); buff = NULL;
}

 /**Function********************************************************************

   Synopsis    [Post processes the entire AIG tree in
                order to reduce the overall size]
   Description [Given a heuristic, this function tries to shrink the size of
                   the previously built AIG tree by finding well-known
                   structural patterns]
   SideEffects []
   SeeAlso     []
 ******************************************************************************/
int bAig_tryReduceAig(bAig_Manager_t *manager, /* the AIG manager */
                      unsigned char flags,        /* visit heuristics */
                      unsigned long threshold, /* throw away threshold */
                      unsigned long maxDepth)
{
    unsigned long i, tmp;
    int res;
    FILE* sis_pipe, *fp_blif_opt, *fp_blif;
    unsigned long candidatesSize = 0;

    /* Task 2: recursively finds backward paths where internal reference count
       is == 1 */
    bAigEdge_t *candidates = Pdtutil_Alloc(bAigEdge_t,
                                   2*manager->nodesArraySize/bAigNodeSize);

    if (candidates == NIL(bAigEdge_t))
    {
        fprintf(stderr, "Error - Cannot allocate memory for candidates' list.\r\n");
        return (-1);
    }

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "%s", "Search for a single-referenced cluster in the AIG.\r\n");
      fprintf(stdout, "%s %ld - %s %ld - %s %d %s",
        "Max allowed depth:", maxDepth,
        "Minimum cluster threshold:", threshold,
        "Search flags:", flags, "\r\n")
     );

    // ultimo nodo, è sicuramente una foglia prima dell'uscita
    bAigEdge_t curr = (bAigEdge_t) (manager->nodesArraySize - bAigNodeSize);

    /* recurses on dependencies tree to retrieve children */
    /* avoids recursing on non-instantated nodes, which are contained
       into freeList */
    while(curr > bAigFirstNodeIndex)
    {
        // only last created/single-referenced nodes (output nodes)
        if (refCount(manager,curr) == 1)
        {
            // save last result
            tmp = candidatesSize;

            res = recursiveFind(manager,
                        &(candidates[0]),
                        &candidatesSize,
                        curr,
                        flags,
                        0,
                        maxDepth,
                        START,
                        curr,
                        D_START);

            // check threshold
            if (((flags & bAigReduceThreshold) == bAigReduceThreshold &&
                (candidatesSize - tmp) >= threshold) ||
                ((flags & bAigReduceThreshold) == 0))
            {
                break;
            }
            else
            {
                if ((candidatesSize - tmp) > 0)
                {
                    // clears visited flag on previous nodes
                    for (i=tmp; i < candidatesSize; i++)
                        nodeClearVisited(manager, candidates[i]);
                }
                candidatesSize -= (candidatesSize - tmp); // backtrack
            }
        }
        curr -= bAigNodeSize;
    }

    if (candidatesSize > 0)
    {
        // Stats
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "%ld %s", candidatesSize,
            "total clusterized node(s) into the net.\r\n")
        );

        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "%s", "Processing cluster.\r.\n")
        );
        /* Task 3: exploits 2-Level minimization whith resource sharing of longest paths*/

        /* multilevel */
        char *filename_in = Pdtutil_Alloc(char, MAX_LINESIZE + 1);
        char *filename_out = Pdtutil_Alloc(char, MAX_LINESIZE + 1);

        sprintf(filename_in, "%s.%s", "tmpNode", "blif");
        sprintf(filename_out, "%s.%s", "tmpNode_opt", "blif");

        fp_blif = fopen(filename_in, "w+");

        if (fp_blif != NULL)
        {
            /* translates array of linked nodes into BLIF network, writing into a file */
            baig2blif(fp_blif, manager, candidates, candidatesSize/*, i*/); // prova
            fclose(fp_blif);

            /* comunicazione con SIS */
            sis_pipe = popen("/usr/local/sis/bin/sis", "w"); // 1>/dev/null 2>/dev/null

            if (sis_pipe != NULL)
            {
                char *str1 = Pdtutil_Alloc(char, MAX_LINESIZE + 1);
                sprintf(str1, "%s %s %s", "read_blif", filename_in, "\r\n");
                char *str2 = Pdtutil_Alloc(char, MAX_LINESIZE + 1);
                sprintf(str2, "%s", "full_simplify -o nocomp\r\n");
                char *str3 = Pdtutil_Alloc(char, MAX_LINESIZE + 1);
                sprintf(str3, "%s %s %s" , "write_blif", filename_out, "\r\n");
                char *str4 = Pdtutil_Alloc(char, MAX_LINESIZE + 1);
                sprintf(str4, "%s", "quit\r\n");

                fwrite(str1, strlen(str1), 1, sis_pipe);
                fwrite(str2, strlen(str2), 1, sis_pipe);
                fwrite(str3, strlen(str3), 1, sis_pipe);
                fwrite(str4, strlen(str4), 1, sis_pipe);
                fflush(sis_pipe);
                pclose(sis_pipe);

                Pdtutil_Free(str1); str1 = NULL;
                Pdtutil_Free(str2); str2 = NULL;
                Pdtutil_Free(str3); str3 = NULL;
                Pdtutil_Free(str4); str4 = NULL;
                /* fine comunicazioni, recupero risultato */

                fp_blif_opt = fopen(filename_out, "r");

                if (fp_blif_opt != NULL)
                {
                    fprintf(stdout, "%s %ld %s", "Before processing:",
                      manager->nodesArraySize/bAigNodeSize - manager->freeNum,
                            "node(s) totally referenced...\r\n");

                    // returns restructured array, use to rebuild the AIG
                    // reference count statistics
                    // and to update root node fan-outs
                    blif2baig(fp_blif_opt, manager, candidates, candidatesSize);
                    incRefCount(manager, candidates[0]);
                    fclose(fp_blif_opt);

                    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
                      Pdtutil_VerbLevelNone_c,
                      fprintf(stdout, "%s %ld %s", "After processing:",
                        manager->nodesArraySize/bAigNodeSize - manager->freeNum,
                        "node(s) totally referenced...\r\n");
                      fprintf(stdout, "%s", "Done.\r\n")
                    );
                }
                else
                    fprintf(stderr, "%s %s %s",
                      "Error - Cannot open", filename_out,
                       "for reading optimized BLIF net.\r\n");
            }
            else
                fprintf(stderr, "%s",
                  "Error - Cannot open pipe with SIS for processing BLIF net.\r\n");
        }
        else
        {
            fprintf(stderr, "%s %s %s", "Error - Cannot open ", filename_in,
              "for writing BLIF net...\r\n");
        }

        Pdtutil_Free(filename_in); filename_in = NULL;
        Pdtutil_Free(filename_out); filename_out = NULL;
    }
    else
    {
        fprintf(stderr, "%s", "Warning - No optimization feasible...\r\n");
    }

    /* Task 4: frees structures and resets flags */
    for (i=0;i < candidatesSize; i++)
    {
        nodeClearVisited(manager, candidates[i]);
    }

    Pdtutil_Free(candidates);

    /* Success */
    return (0);
}

int bAig_reduceTest()
{
    char *buff = Pdtutil_Alloc(char, MAX_LINESIZE + 1);
     unsigned long i;
    bAigEdge_t tmp, l, r;

    // creates manager
    bAig_Manager_t *manager = bAig_initAig(100);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "%s", "AIG Manager created.\r\n")
    );
    for (i= 0; i<20; i++)
    {
        sprintf(buff, "%s_%ld", "var", i);
        tmp = bAig_CreateVarNode(manager, buff);
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "%s %ld %s",
            "bAig_reduceTest() - New variable node created:", tmp, "\r\n")
        );
    }

    tmp = bAig_And(manager, (bAigEdge_t) 5, (bAigEdge_t) 8); // 84
    tmp = bAig_And(manager, (bAigEdge_t) 4, (bAigEdge_t) 9); // 88
    tmp = bAig_And(manager, (bAigEdge_t) 85, (bAigEdge_t) 89); // 92

    tmp = bAig_And(manager, (bAigEdge_t) 13, (bAigEdge_t) 16); // 96
    tmp = bAig_And(manager, (bAigEdge_t) 12, (bAigEdge_t) 17); // 100
    tmp = bAig_And(manager, (bAigEdge_t) 97, (bAigEdge_t) 101); // 104

    tmp = bAig_And(manager, (bAigEdge_t) 93, (bAigEdge_t) 105); // 108

    tmp = bAig_And(manager, (bAigEdge_t) 108, (bAigEdge_t) 16); // 112

    tmp = bAig_And(manager, (bAigEdge_t) 20, (bAigEdge_t) 25); // 116
    tmp = bAig_And(manager, (bAigEdge_t) 21, (bAigEdge_t) 24); // 120
    tmp = bAig_And(manager, (bAigEdge_t) 117, (bAigEdge_t) 121); // 124

    tmp = bAig_And(manager, (bAigEdge_t) 29, (bAigEdge_t) 32); // 128
    tmp = bAig_And(manager, (bAigEdge_t) 28, (bAigEdge_t) 33); // 132
    tmp = bAig_And(manager, (bAigEdge_t) 129, (bAigEdge_t) 133); // 136

    tmp = bAig_And(manager, (bAigEdge_t) 125, (bAigEdge_t) 137); // 140

    tmp = bAig_And(manager, (bAigEdge_t) 140, (bAigEdge_t) 32); // 144

    tmp = bAig_And(manager, (bAigEdge_t) 144, (bAigEdge_t) 112); // 148

    incRefCount(manager, tmp);

    for (i=84;i<manager->nodesArraySize;i+=bAigNodeSize)
    {
        l = leftChild(manager, i);
        r = rightChild(manager, i);
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "(%ld)%ld <- %ld(%ld) -> %ld(%ld)\r.\n",
            refCount(manager, l), l, i, refCount(manager, i), r,
            refCount(manager,r))
        );
    }

    bAig_tryReduceAig(manager, 0x02, 1, 1000);

    for (i=84;i<manager->nodesArraySize;i+=bAigNodeSize)
    {
        l = leftChild(manager, i);
        r = rightChild(manager, i);
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "(%ld)%ld <- %ld(%ld) -> %ld(%ld)\r.\n",
            refCount(manager, l), l, i, refCount(manager, i), r,
            refCount(manager,r))
        );
    }

    // deletes manager
    bAig_quit(manager);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "%s", "AIG Manager deleted.\r\n")
    );

    Pdtutil_Free(buff); buff = NULL;
    return (0);
}




/* MaMu */

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static void incRefCount(bAig_Manager_t *manager, bAigEdge_t nodeIndex);
static void decRefCount(bAig_Manager_t *manager, bAigEdge_t nodeIndex);
static bAigEdge_t HashTableLookup(bAig_Manager_t *manager, bAigEdge_t node1, bAigEdge_t node2);
static void HashTableDelete(bAig_Manager_t *manager, bAigEdge_t node);
static int HashTableAdd(bAig_Manager_t *manager, bAigEdge_t nodeIndexParent, bAigEdge_t nodeIndex1, bAigEdge_t nodeIndex2);
static char * util_inttostr(int i);
static char * util_strcat3(char * str1, char * str2, char * str3);
static char * util_strcat4(char * str1, char * str2, char * str3, char * str4);
static int recursiveFind(bAig_Manager_t *manager, bAigEdge_t *candidates, unsigned long *candidatesSize, bAigEdge_t node, unsigned char flags, unsigned long currDepth, unsigned long maxDepth, recurseState state, bAigEdge_t parent, unsigned char direction);

/**AutomaticEnd***************************************************************/




















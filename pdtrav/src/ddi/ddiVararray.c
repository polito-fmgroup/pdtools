/**CFile***********************************************************************

  FileName    [ddiVararray.c]

  PackageName [ddi]

  Synopsis    [Functions to manage <em>arrays of variables</em>]

  Description [Array of variables is implemented as an array of BDD nodes
               (called "projection funtions" in CUDD package).<BR>
               The basic manipulation allowed for variable arrays are:<BR>
               <OL>
               <LI><B>Alloc</B> and <B>free</B> an array
               <LI><B>Insert</B> and <B>fetch</B> a variable
               <LI><B>Duplicate</B> an array
               <LI><B>Join</B> and <B>append</B> a new array
               <LI>Convert into array of integer
               <LI>Convert into a set of variables
               </OL>]

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

static int VarOrdCompare(const void *v0, const void *v1);
static int VarIdCompare(const void *v0, const void *v1);

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************
  Synopsis    [Generate a variable array from CU vars (BDD nodes)]
  Description [No variable dup is done (as all DDI operations working with
               variables)]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayToCU]
*****************************************************************************/
Ddi_Vararray_t *
Ddi_VararrayMakeFromCU (
  Ddi_Mgr_t *mgr,
  DdNode **array,
  int n
)
{
  Ddi_Vararray_t *data;
  int i;

  data = Ddi_VararrayAlloc(mgr,n);
  for (i=0; i<n; i++)
    Ddi_VararrayWrite(data,i,Ddi_VarFromCU(mgr,array[i]));

  return data;
}

/**Function*******************************************************************
  Synopsis    [Generate an array of pointers to CUDD variables]
  Description [Generate a dynamic allocated array of pointers to CUDD BDDs
              representing variables in input array.]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayMakeFromCU]
*****************************************************************************/
DdNode **
Ddi_VararrayToCU (
  Ddi_Vararray_t *array )
{
  DdNode **vett;
  int i;

  if (array==NULL)
    return NULL;

  vett = Pdtutil_Alloc(DdNode *,Ddi_VararrayNum(array));

  for(i=0;i<Ddi_VararrayNum(array);i++) {
    vett[i] = Ddi_VarToCU(Ddi_VararrayRead(array,i));
  }

  return vett;
}

/**Function*******************************************************************
  Synopsis    [Generate a variable array from array of integer indexes]
  Description [Integer indexes are used as CUDD indexes.
               No variable dup is done (as all DDI operations working with
               variables)]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayToCU]
*****************************************************************************/
Ddi_Vararray_t *
Ddi_VararrayMakeFromInt (
  Ddi_Mgr_t *mgr,
  int *array,
  int n
)
{
  Ddi_Vararray_t *data;
  int i;

  data = Ddi_VararrayAlloc(mgr,n);
  for (i=0; i<n; i++)
    Ddi_VararrayWrite(data,i,Ddi_VarFromIndex(mgr,array[i]));

  return data;
}

/**Function*******************************************************************
  Synopsis    [Generate a variable array from an existing var array]
  Description [Generate a variable array from an existing var array. New vars
               are created without a given order constrain (relativeOrder=0) or
               before/after the corresponding reference variable
               (relativeOrder=-1/1). Prefix and suffix for varnames are
               optionally accepted (NULL pointers to avoid prefix or suffix).]
  SideEffects [none]
  SeeAlso     [none]
*****************************************************************************/
Ddi_Vararray_t *
Ddi_VararrayMakeNewVars (
  Ddi_Vararray_t *refArray,
  char *namePrefix,
  char *nameSuffix,
  int relativeOrder
)
{
  Ddi_Vararray_t *data;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(refArray);
  int i, n=Ddi_VararrayNum(refArray);
  int usePrefix = namePrefix!=NULL && strlen(namePrefix)>0;
  int useSuffix = nameSuffix!=NULL && strlen(nameSuffix)>0;

  Pdtutil_Assert(usePrefix||useSuffix,"name prefix or suffix needed");

  data = Ddi_VararrayAlloc(ddm,n);
  for (i=0; i<n; i++) {
    char name[1000];
    Ddi_Var_t *v = Ddi_VararrayRead(refArray,i);
    Ddi_Var_t *newv;
    Ddi_Bdd_t *newvLit;
    strcpy(name,"");
    if (usePrefix) {
      strcat(name,namePrefix); strcat(name,"_");
    }
    strcat(name,Ddi_VarName(v));
    if (useSuffix) {
      strcat(name,"_"); strcat(name,nameSuffix); 
    }

    newv = Ddi_VarFromName(ddm,name);
    if (newv == NULL) {
      int isAig = Ddi_VarIsAig(v);
      if (isAig) {
        newv = Ddi_VarNewBaig(ddm,name);
      }
      else if (relativeOrder == -1) {
        newv = Ddi_VarNewBeforeVar(v);
      Ddi_VarAttachName (newv, name);
      }
      else if (relativeOrder == 1) {
        newv = Ddi_VarNewAfterVar(v);
      Ddi_VarAttachName (newv, name);
      }
      else if (relativeOrder == 2) {
        newv = Ddi_VarNewBaig(ddm,name);
      }
      else {
        newv = Ddi_VarNew(ddm);
        Ddi_VarAttachName (newv, name);
      }
    }
    Ddi_VararrayWrite(data,i,newv);
  }

  return data;
}



/**Function*******************************************************************
  Synopsis    [Generate a variable array from an existing var array]
  Description [Generate a variable array from an existing var array. New vars
               are created without a given order constrain (relativeOrder=0) or
               before/after the corresponding reference variable
               (relativeOrder=-1/1). Prefix and suffix for varnames are
               optionally accepted (NULL pointers to avoid prefix or suffix).]
  SideEffects [none]
  SeeAlso     [none]
*****************************************************************************/
Ddi_Vararray_t *
Ddi_VararrayMakeNewAigVars (
  Ddi_Vararray_t *refArray,
  char *namePrefix,
  char *nameSuffix
)
{
  Ddi_Vararray_t *data;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(refArray);
  int i, n=Ddi_VararrayNum(refArray);
  static char name[5000];

  data = Ddi_VararrayAlloc(ddm,n);
  for (i=0; i<n; i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(refArray,i);
    Ddi_Var_t *newv;
    Ddi_Bdd_t *newvLit;
    sprintf(name,"%s%s%s%s%s", namePrefix?namePrefix:"",namePrefix?"_":"",
            Ddi_VarName(v),nameSuffix?"_":"",nameSuffix?nameSuffix:"");
    newv = Ddi_VarFromName(ddm,name);
    if (newv == NULL) {
      newv = Ddi_VarNewBaig(ddm,name);
    }
    else {
      //     Pdtutil_Assert(Ddi_VarIsAig(newv),"aig var required");
    }
    Ddi_VararrayWrite(data,i,newv);
  }

  return data;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects [none]
  SeeAlso     [none]
*****************************************************************************/
Ddi_Vararray_t *
Ddi_VararrayFindVarsFromPrefixSuffix (
  Ddi_Vararray_t *vars,
  char *namePrefix,
  char *nameSuffix
)
{
  Ddi_Vararray_t *varArray;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(vars);
  int i, n=Ddi_VararrayNum(vars);
  int prefixLen = namePrefix==NULL ? 0 : strlen(namePrefix);
  int suffixLen = nameSuffix==NULL ? 0 : strlen(nameSuffix);
  Pdtutil_Assert(prefixLen>0||suffixLen,"name prefix or suffix needed");

  varArray = Ddi_VararrayAlloc(ddm,0);
  for (i=0; i<n; i++) {
    Ddi_Var_t *v_i = Ddi_VararrayRead(vars,i);
    char *name_i = Ddi_VarName(v_i);
    int l = strlen(name_i);
    if (prefixLen>0 && strncmp(name_i,namePrefix,prefixLen)!=0)
      continue;
    if (suffixLen>0 && strcmp(name_i+l-suffixLen,nameSuffix)!=0)
      continue;
    Ddi_VararrayInsertLast(varArray,v_i);
  }
  if (Ddi_VararrayNum(varArray)==0)
    Ddi_Free(varArray);
  
  return(varArray);
}

/**Function*******************************************************************
  Synopsis    []
  Description []

  SideEffects [none]
  SeeAlso     [none]
*****************************************************************************/
Ddi_Vararray_t *
Ddi_VararrayFindRefVars (
  Ddi_Vararray_t *vars,
  char *namePrefix,
  char *nameSuffix
)
{
  Ddi_Vararray_t *refArray;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(vars);
  int i, n=Ddi_VararrayNum(vars);
  int prefixLen = namePrefix==NULL ? 0 : strlen(namePrefix);
  int suffixLen = nameSuffix==NULL ? 0 : strlen(nameSuffix);
  char nameBuf[1000];
  Pdtutil_Assert(prefixLen>0||suffixLen,"name prefix or suffix needed");

  refArray = Ddi_VararrayAlloc(ddm,0);
  for (i=0; i<n; i++) {
    Ddi_Var_t *v_i = Ddi_VararrayRead(vars,i);
    char *name_i = Ddi_VarName(v_i);
    int l = strlen(name_i);
    if (prefixLen>0 && strncmp(name_i,namePrefix,prefixLen)!=0)
      continue;
    if (suffixLen>0 && strcmp(name_i+l-suffixLen,nameSuffix)!=0)
      continue;
    strncpy(nameBuf,name_i+(prefixLen>0?prefixLen+1:0),l-(suffixLen>0?suffixLen+1:0));
    Ddi_Var_t *ref_i = Ddi_VarFromName(ddm,nameBuf);
    if (ref_i != NULL)
      Ddi_VararrayInsertLast(refArray,ref_i);
  }
  if (Ddi_VararrayNum(refArray)<n)
    Ddi_Free(refArray);
  
  return(refArray);
}

/**Function*******************************************************************
  Synopsis    [Generate a variable array from varset]
  Description [Generate a variable array from varset. Variables are inserted
               in array according to their index or position in variable
               ordering (depending on parameter
               variables)]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayToCU]
*****************************************************************************/
Ddi_Vararray_t *
Ddi_VararrayMakeFromVarset (
  Ddi_Varset_t *vs           /* input varset */,
  int sortByOrderOrIndex     /* 1: sort by order, 0: sort by index */
)
{
  Ddi_Vararray_t *va;
  int i;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(vs);

  if (vs->common.code == Ddi_Varset_Array_c) {
    va = Ddi_VararrayDup(vs->data.array.va);
    return va;
  }

  va = Ddi_VararrayAlloc(ddm,Ddi_VarsetNum(vs));

  if (sortByOrderOrIndex) {
    /* sort by order */
    for (i=0,Ddi_VarsetWalkStart(vs);
      !Ddi_VarsetWalkEnd(vs);
       i++,Ddi_VarsetWalkStep(vs)) {
      Ddi_VararrayWrite(va,i,Ddi_VarsetWalkCurr(vs));
    }
  }
  else {
    /* sort by index */
    int k, nMgrVars = Ddi_MgrReadNumCuddVars(ddm);
    int *ids = Pdtutil_Alloc(int,nMgrVars);
    for(i=0;i<nMgrVars;i++) {
      ids[i] = 0;
    }
    for (i=0,Ddi_VarsetWalkStart(vs);
      !Ddi_VarsetWalkEnd(vs);
       i++,Ddi_VarsetWalkStep(vs)) {
      ids[Ddi_VarCuddIndex(Ddi_VarsetWalkCurr(vs))] = 1;
    }
    for(i=0,k=0;i<nMgrVars;i++) {
      if (ids[i]) {
        Ddi_VararrayWrite(va,k++,Ddi_VarFromCuddIndex(ddm,i));
      }
    }
    Pdtutil_Free(ids);
  }

  return va;
}

/**Function********************************************************************
  Synopsis    [Return support of Aig]
  Description [Return support of Aig]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_VararraySort (
  Ddi_Vararray_t *va         /* input varset */,
  int sortByOrderOrIndex     /* 1: sort by order, 0: sort by index */
)
{
  int nv = Ddi_VararrayNum(va);
  Ddi_Generic_t **array = va->array->data;
  

  if (sortByOrderOrIndex) {
    qsort((void **)array,nv,sizeof(Ddi_Generic_t *),VarOrdCompare);
  }
  else {
    qsort((void **)array,nv,sizeof(Ddi_Generic_t *),VarIdCompare);
  }
}

/**Function*******************************************************************
  Synopsis    [Generate an array of integer variable indexes]
  Description [Generate a dynamically allocated array of integer variable
    indexes. Integer indexes are taken from CUDD indexes.]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayToCU]
*****************************************************************************/
int *                    /* array of integer */
Ddi_VararrayToInt (
  Ddi_Vararray_t *array  /* array of variables */ )
{
  int i;
  int *vett;

  vett = Pdtutil_Alloc(int,Ddi_VararrayNum(array));

  for (i=0; i<Ddi_VararrayNum(array); i++) {
    vett[i] = Ddi_VarCuddIndex(Ddi_VararrayRead(array,i));
  }

 return vett;
}

/**Function*******************************************************************
  Synopsis    [Allocate a new array of variables of given length]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Vararray_t *
Ddi_VararrayAlloc (
  Ddi_Mgr_t *mgr     /* DDI manager */,
  int size           /* array length */
)
{
  Ddi_Vararray_t *array;

  array = (Ddi_Vararray_t *)DdiGenericAlloc(Ddi_Vararray_c,mgr);
  array->array = DdiArrayAlloc(size);

  return (array);
}

/**Function*******************************************************************
  Synopsis    [Return the number of variables (entries) in array]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
int
Ddi_VararrayNum(
  Ddi_Vararray_t *array
)
{
  if (array==NULL) return 0;
  DdiConsistencyCheck(array,Ddi_Vararray_c);
  return (DdiArrayNum(array->array));
}


/**Function*******************************************************************
  Synopsis    [Write a variable in array at given position]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayRead]
******************************************************************************/
void
Ddi_VararrayWrite(
  Ddi_Vararray_t *array    /* array of variables */,
  int pos                  /* position of new element */,
  Ddi_Var_t *var           /* variable to be inserted */ )
{
  DdiConsistencyCheck(array,Ddi_Vararray_c);
  DdiArrayWrite(array->array,pos,(Ddi_Generic_t *)var,Ddi_Mov_c);
}

/**Function*******************************************************************
  Synopsis    [Return the variable at i-th position in array]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayInsert]
******************************************************************************/
Ddi_Var_t *
Ddi_VararrayRead (
  Ddi_Vararray_t *array    /* variable array */,
  int i                    /* position */ )
{
  DdiConsistencyCheck(array,Ddi_Vararray_c);
  return ((Ddi_Var_t *)DdiArrayRead(array->array,i));
}


/**Function*******************************************************************
  Synopsis    [clear array at given position (variable is replaced by NULL)]
  Description [clear array at given position (variable is replaced by NULL)]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayRead]
******************************************************************************/
void
Ddi_VararrayClear(
  Ddi_Vararray_t *array    /* array of variables */,
  int pos                  /* position of element to be cleared */
)
{
  DdiConsistencyCheck(array,Ddi_Vararray_c);
  DdiArrayWrite(array->array,pos,NULL,Ddi_Mov_c);
}

/**Function*******************************************************************
  Synopsis    [Insert a variable in array at given position]
  Description [Insert a variable in array at given position.
    Following entries are shifted down.]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayExtract]
******************************************************************************/
void
Ddi_VararrayInsert(
  Ddi_Vararray_t *array    /* array of variables */,
  int pos                  /* position of new element */,
  Ddi_Var_t *v             /* variable to be written */ )
{
  DdiConsistencyCheck(array,Ddi_Vararray_c);
  DdiArrayInsert(array->array,pos,(Ddi_Generic_t *)v,Ddi_Dup_c);
}

/**Function*******************************************************************
  Synopsis    [Insert a variable in array at last (new) position]
  Description [Insert a variable in array at last (new) position]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayExtract]
******************************************************************************/
void
Ddi_VararrayInsertLast(
  Ddi_Vararray_t *array    /* array of variables */,
  Ddi_Var_t *v             /* variable to be written */ )
{
  DdiConsistencyCheck(array,Ddi_Vararray_c);
  Ddi_VararrayInsert(array,Ddi_VararrayNum(array),v);
}

/**Function*******************************************************************
  Synopsis    [Extract the variable at i-th position in array]
  Description [Extract the variable at i-th position in array.
    The extracted variable is removed from the array and the following entries
    are shifted up.]
  SideEffects []
  SeeAlso     [Ddi_VararrayInsert]
******************************************************************************/
Ddi_Var_t *
Ddi_VararrayExtract (
  Ddi_Vararray_t *array    /* variable array */,
  int i                    /* position */ )
{
  DdiConsistencyCheck(array,Ddi_Vararray_c);
  return ((Ddi_Var_t *)DdiArrayExtract(array->array,i));
}


/**Function*******************************************************************
  Synopsis    [Remove array entry at given position]
  Description [Remove array entry at given position.
    This operation is equivalent to extract (but return type is void).]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayExtract Ddi_VararrayClear]
******************************************************************************/
void
Ddi_VararrayRemove(
  Ddi_Vararray_t *array    /* array of variables */,
  int pos                  /* position of element to be cleared */
)
{
  Ddi_Var_t *tmp;

  DdiConsistencyCheck(array,Ddi_Vararray_c);
  tmp = Ddi_VararrayExtract(array,pos);
}

/**Function*******************************************************************
  Synopsis    [Remove NULL array entries]
  Description [Remove NULL array entries]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Ddi_VararrayRemoveNull(
  Ddi_Vararray_t *array    /* array of variables */
)
{
  DdiConsistencyCheck(array,Ddi_Vararray_c);
  DdiArrayRemoveNull(array->array);
}


/**Function********************************************************************
  Synopsis    [Return union of two var-arrays. Result generated]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayUnionAcc,Ddi_VararrayIntersect,Ddi_VararrayDiff]
******************************************************************************/
Ddi_Vararray_t *
Ddi_VararrayUnion (
  Ddi_Vararray_t *v1    /* first var-array */ ,
  Ddi_Vararray_t *v2    /* second var-array */
  )
{
  return((Ddi_Vararray_t *)DdiGenericOp2(Ddi_VararrayUnion_c,
    Ddi_Generate_c,v1,v2));
}

/**Function********************************************************************
  Synopsis    [Return union of two var-arrays. Result accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayUnion]
******************************************************************************/
Ddi_Vararray_t *
Ddi_VararrayUnionAcc (
  Ddi_Vararray_t *v1    /* first var-array */ ,
  Ddi_Vararray_t *v2    /* second var-array */
  )
{
  return((Ddi_Vararray_t *)DdiGenericOp2(Ddi_VararrayUnion_c,
    Ddi_Accumulate_c,v1,v2));
}

/**Function********************************************************************
  Synopsis    [Return intersection of two var-arrays. Result generated]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayUnion,Ddi_VararrayIntersectAcc,Ddi_VararrayDiff]
******************************************************************************/
Ddi_Vararray_t *
Ddi_VararrayIntersect (
  Ddi_Vararray_t *v1    /* first var-array */ ,
  Ddi_Vararray_t *v2    /* second var-array */
  )
{
  return((Ddi_Vararray_t *)DdiGenericOp2(Ddi_VararrayIntersect_c,
    Ddi_Generate_c,v1,v2));
}

/**Function********************************************************************
  Synopsis    [Return intersection of two var-arrays. Result accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayUnion]
******************************************************************************/
Ddi_Vararray_t *
Ddi_VararrayIntersectAcc (
  Ddi_Vararray_t *v1    /* first var-array */ ,
  Ddi_Vararray_t *v2    /* second var-array */
  )
{
  return((Ddi_Vararray_t *)DdiGenericOp2(Ddi_VararrayIntersect_c,
    Ddi_Accumulate_c,v1,v2));
}

/**Function********************************************************************
  Synopsis    [Return difference of two var-arrays. Result generated]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayUnionAcc,Ddi_VararrayIntersect,Ddi_VararrayDiff]
******************************************************************************/
Ddi_Vararray_t *
Ddi_VararrayDiff (
  Ddi_Vararray_t *v1    /* first var-array */ ,
  Ddi_Vararray_t *v2    /* second var-array */
  )
{
  return((Ddi_Vararray_t *)DdiGenericOp2(Ddi_VararrayDiff_c,
    Ddi_Generate_c,v1,v2));
}

/**Function********************************************************************
  Synopsis    [Return difference of two var-arrays. Result accumulated]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayUnion]
******************************************************************************/
Ddi_Vararray_t *
Ddi_VararrayDiffAcc (
  Ddi_Vararray_t *v1    /* first var-array */ ,
  Ddi_Vararray_t *v2    /* second var-array */
  )
{
  return((Ddi_Vararray_t *)DdiGenericOp2(Ddi_VararrayDiff_c,
    Ddi_Accumulate_c,v1,v2));
}

/**Function*******************************************************************
  Synopsis    [Duplicate an array of variables]
  Description [Only the "array" part is duplicated. Variables are never
               duplicated nor freed, except when closing the owner manager]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayAlloc]
******************************************************************************/
Ddi_Vararray_t *
Ddi_VararrayDup(
  Ddi_Vararray_t *old  /* array to be duplicated */
)
{
  Ddi_Vararray_t *newa;

  DdiConsistencyCheck(old,Ddi_Vararray_c);
  newa = (Ddi_Vararray_t *)DdiGenericDup((Ddi_Generic_t *)old);

  return (newa);
}

/**Function*******************************************************************
  Synopsis    [Copy an array of variables to a destination maneger]
  Description [Variable correspondence is established "by index", i.e.
               variables with same index in different manager correspond]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayDup]
******************************************************************************/
Ddi_Vararray_t *
Ddi_VararrayCopy (
  Ddi_BddMgr *ddm          /* dd Manager */,
  Ddi_Vararray_t  *old    /* array of variables */
)
{
  Ddi_Vararray_t *newa;

  DdiConsistencyCheck(old,Ddi_Vararray_c);
  newa = (Ddi_Vararray_t *)DdiGenericCopy(ddm,(Ddi_Generic_t *)old,NULL,NULL);

  return (newa);
}

/**Function*******************************************************************
  Synopsis    [Append the elements of array2 at the end of array1]
  SideEffects [none]
  SeeAlso     [Ddi_VararrayJoin]
******************************************************************************/
void
Ddi_VararrayAppend (
  Ddi_Vararray_t *array1   /* first array */,
  Ddi_Vararray_t *array2   /* array to be appended */
)
{
  DdiConsistencyCheck(array1,Ddi_Vararray_c);
  DdiConsistencyCheck(array2,Ddi_Vararray_c);
  DdiArrayAppend(array1->array,array2->array);
}

/**Function*******************************************************************
  Synopsis    [search variable in array. return index (<0 = not found)]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
int
Ddi_VararraySearchVar (
  Ddi_Vararray_t *array   /* array */,
  Ddi_Var_t *v            /* variable */
)
{
  int i,n;
  DdiConsistencyCheck(array,Ddi_Vararray_c);
  DdiConsistencyCheck(v,Ddi_Var_c);
  n = Ddi_VararrayNum(array);
  for (i=0; i<n; i++) {
    if (Ddi_VararrayRead(array,i)==v) return i;
  }
  return -1;
}

/**Function*******************************************************************
  Synopsis    [search variable in array. return index (<0 = not found)]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
int
Ddi_VararraySearchVarDown (
  Ddi_Vararray_t *array   /* array */,
  Ddi_Var_t *v            /* variable */
)
{
  int i,n;
  DdiConsistencyCheck(array,Ddi_Vararray_c);
  DdiConsistencyCheck(v,Ddi_Var_c);
  n = Ddi_VararrayNum(array);
  for (i=n-1; i>=0; i--) {
    if (Ddi_VararrayRead(array,i)==v) return i;
  }
  return -1;
}


/**Function********************************************************************
  Synopsis    [Swap two sets of variables in vararray. Result generated]
  Description [Return a new varset with the variables swapped
               (x replaces y and viceversa).]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Vararray_t *
Ddi_VararraySwapVars(
  Ddi_Vararray_t   *va    /* input vararray */,
  Ddi_Vararray_t *x    /* 1-st array of variables */,
  Ddi_Vararray_t *y    /* 2-nd array of variables */
  )
{
  return ((Ddi_Vararray_t *)DdiGenericOp3(Ddi_VararraySwapVars_c,
    Ddi_Generate_c,va,x,y));
}

/**Function********************************************************************
  Synopsis    [Swap two sets of variables in varset. Result accumulated]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Vararray_t *
Ddi_VararraySwapVarsAcc (
  Ddi_Vararray_t   *va    /* input vararray */,
  Ddi_Vararray_t *x    /* 1-st array of variables */,
  Ddi_Vararray_t *y    /* 2-nd array of variables */
  )
{
  return ((Ddi_Vararray_t *)DdiGenericOp3(Ddi_VararraySwapVars_c,
    Ddi_Accumulate_c,va,x,y));
}

/**Function********************************************************************
  Synopsis    [Variable substitution in vararray. Result generated]
  Description [Return a new varset with variable substitution x <- y.]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Vararray_t *
Ddi_VararraySubstVars(
  Ddi_Vararray_t   *va    /* input vararray */,
  Ddi_Vararray_t *x    /* 1-st array of variables */,
  Ddi_Vararray_t *y    /* 2-nd array of variables */
  )
{
  return ((Ddi_Vararray_t *)DdiGenericOp3(Ddi_VararraySubstVars_c,
    Ddi_Generate_c,va,x,y));
}

/**Function********************************************************************
  Synopsis    [Variable substitution in vararray. Result accumulated]
  Description [Return a new varset with variable substitution x <- y.]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Vararray_t *
Ddi_VararraySubstVarsAcc (
  Ddi_Vararray_t   *va    /* input vararray */,
  Ddi_Vararray_t *x    /* 1-st array of variables */,
  Ddi_Vararray_t *y    /* 2-nd array of variables */
  )
{
  return ((Ddi_Vararray_t *)DdiGenericOp3(Ddi_VararraySubstVars_c,
    Ddi_Accumulate_c,va,x,y));
}

/**Function*******************************************************************
  Synopsis    [Print variable in array                               ]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Ddi_VararrayPrint (
  Ddi_Vararray_t *array
)
{
  int i,n;
  DdiConsistencyCheck(array,Ddi_Vararray_c);
  n = Ddi_VararrayNum(array);
  for (i=0; i<n; i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(array,i);
    printf ("%s ", Ddi_VarName(v));
  }
  printf ("\n ");
}


/**Function*******************************************************************
  Synopsis    [Print variable in array                               ]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Ddi_VararrayStore (
  Ddi_Vararray_t *array,
  char *filename,
  FILE *fp
)
{
  int i,n;
  int doClose = 0;
  if (fp == NULL) {
    fp = fopen(filename,"w");
    doClose = 1;
    if (fp==NULL) return;
  }
  DdiConsistencyCheck(array,Ddi_Vararray_c);
  n = Ddi_VararrayNum(array);
  for (i=0; i<n; i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(array,i);
    fprintf (fp, "%s\n", Ddi_VarName(v));
  }
  if (doClose)
    fclose(fp);
}

/**Function*******************************************************************
  Synopsis    [Print variable in array                               ]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Vararray_t *
Ddi_VararrayLoad (
  Ddi_BddMgr *ddm          /* dd Manager */,
  char *filename,
  FILE *fp
)
{
  int i,n;
  int doClose = 0;
  Ddi_Vararray_t *array=NULL;
  if (fp == NULL) {
    fp = fopen(filename,"r");
    doClose = 1;
    if (fp==NULL) return NULL;
  }
  array = Ddi_VararrayAlloc(ddm,0);
  char name [1000];
  while (fscanf(fp,"%s",name)==1) {
    Ddi_Var_t *v = Ddi_VarFromName(ddm,name);
    Pdtutil_Assert (v != NULL, "missing var in vararray load");
    Ddi_VararrayInsertLast(array,v);
  }
  if (doClose)
    fclose(fp);
  return array;
}

/**Function*******************************************************************
  Synopsis    [Mark variables in array]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Ddi_VararrayWriteMark (
  Ddi_Vararray_t *array,
  int val
)
{
  int j;

  for(j=0;j<Ddi_VararrayNum(array);j++) {
    Ddi_Var_t *v = Ddi_VararrayRead(array, j);
    Ddi_VarWriteMark(v,val);
  } 
}

/**Function*******************************************************************
  Synopsis    [Mark variables in array]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Ddi_VararrayWriteMarkWithIndex (
  Ddi_Vararray_t *array,
  int offset
)
{
  int j;

  for(j=0;j<Ddi_VararrayNum(array);j++) {
    Ddi_Var_t *v = Ddi_VararrayRead(array, j);
    Ddi_VarWriteMark(v,offset+j);
  } 
}

/**Function*******************************************************************
  Synopsis    [Mark variables in array]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Ddi_VararrayCheckMark (
  Ddi_Vararray_t *array,
  int val
)
{
  int j;

  for(j=0;j<Ddi_VararrayNum(array);j++) {
    Ddi_Var_t *v = Ddi_VararrayRead(array, j);
    Pdtutil_Assert(Ddi_VarReadMark(v)==val,"wrong var mark");
  } 
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Sort by var ord]
  Description [Sort by var ord. Counting sort is used.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
AigVararraySortByOrd(
  Ddi_Vararray_t *vA
)
{
  Pdtutil_Assert(0,"not yet implemented");

  return 0;
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
VarOrdCompare(
  const void *v0p,
  const void *v1p
)
{
  return (DdiVarCompare(*((Ddi_Var_t **)v0p),*((Ddi_Var_t **)v1p)));
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
VarIdCompare(
  const void *v0p,
  const void *v1p
)
{
  return (Ddi_VarIndex(*((Ddi_Var_t **)v0p)) - 
	  Ddi_VarIndex(*((Ddi_Var_t **)v1p)));
}

/**CFile***********************************************************************

  FileName    [ddiBddarray.c]

  PackageName [ddi]

  Synopsis    [Functions to manage <em>arrays of BDDs</em>]

  Description [The basic manipulation allowed for BDD arrays are:<BR> 
    <OL>
    <LI>alloc, make, free arrays
    <LI>write, read, clear, insert, extract, remove array entries
    <LI>duplicate, copy, append
    <LI>store and load to/from file
    </OL>
    For each element of array can be apply any boolean operator
    as AND,OR,XOR,XNOR,Restrict and Constrain.
    ]

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


/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of internal function                                           */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Free meta struct and pointed arrays: one, zero and dc.]
  Description [Free meta struct and pointed arrays: one, zero and dc.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiLogBddarraySizes (
  Ddi_Bddarray_t *a
)
{
  int i; 
  printf("ARRAY SIZES: \n");
  for (i = 0; i < Ddi_BddarrayNum(a); i++) {
    Ddi_Bdd_t *a_i = Ddi_BddarrayRead(a,i);
    printf("[%d] %d\n", i, Ddi_BddSize(a_i));
  }
  printf("\n");
}

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/*
 *  Conversion, creation (make) and release (free) functions
 */

/**Function*******************************************************************
  Synopsis    [Allocate a new array of BDDs]
  Description [Allocate a new array of BDDs. The array slots are initialized 
    with NULL pointers, so further Write operations are required.]
  SideEffects [none]
  SeeAlso     [Ddi_BddarrayWrite]
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayAlloc (
  Ddi_Mgr_t *mgr     /* DDI manager */,
  int length  /* array length */
)
{
  Ddi_Bddarray_t *array;

  array = (Ddi_Bddarray_t *)DdiGenericAlloc(Ddi_Bddarray_c,mgr);
  array->array = DdiArrayAlloc(length);

  return (array);
}

/**Function*******************************************************************
  Synopsis    [Generate a BDD array from CUDD BDDs]
  Description [Generate a BDD array from CUDD BDDs.
    The function allocates a Ddi_Bddarray_t structure, then write 
    monolithic components to proper array slots.]
  SideEffects [none]
  SeeAlso     [Ddi_BddarrayToCU]
*****************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayMakeFromCU (
  Ddi_Mgr_t *mgr,
  DdNode **array,
  int n
)
{
  Ddi_Bddarray_t *data;
  Ddi_Bdd_t *tmp;
  int i;

  Pdtutil_Assert((mgr!=NULL)&&(array!=NULL),
    "NULL manager or BDD when generating DDI node");
  Pdtutil_Assert((n>0),
    "generating array of size 0");

  data = Ddi_BddarrayAlloc(mgr,n);
  for (i=0; i<n; i++) {
    tmp = Ddi_BddMakeFromCU(mgr,array[i]); 
    Ddi_BddarrayWrite(data,i,tmp);
    Ddi_Free(tmp); 
  }

  return(data);
}

/**Function*******************************************************************
  Synopsis    [Generate an array of pointers to CUDD nodes] 
  Description [Generate a dynamically allocated array of pointers to CUDD 
    BDDs, one for each entry in the DDI array. Array entries are required
    to be monolithic.
    The number of array entries is equal to Ddi_BddarrayNum(array), but the 
    array is overdimensioned (by one NULL slot) to make it NULL-terminated.
    The array of pointers is allocated (so explicit free is required), whereas
    the CUDD nodes are NOT referenced.]  
  SideEffects [none]
  SeeAlso     [Ddi_BddarrayMakeFromCU]
*****************************************************************************/
DdNode **
Ddi_BddarrayToCU (
  Ddi_Bddarray_t *array 
)
{
  DdiConsistencyCheck(array,Ddi_Bddarray_c);
  return (DdiArrayToCU(array->array));
}

/**Function*******************************************************************
  Synopsis    [Generate a BDD array from partitions of partitioned BDD]
  Description [Generate a BDD array from partitions of partitioned
  BDD. If no partitioned BDD provided, array of size 1 is generated.]
  SideEffects [none]
  SeeAlso     [Ddi_BddarrayToCU]
*****************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayMakeFromBddPart (
  Ddi_Bdd_t *part
)
{
  Ddi_Mgr_t *mgr = Ddi_ReadMgr(part);
  Ddi_Bddarray_t *array;
  int i, n;

  DdiConsistencyCheck(part,Ddi_Bdd_c);
  if (
    (Ddi_ReadCode(part) == Ddi_Bdd_Part_Conj_c) ||
    (Ddi_ReadCode(part) == Ddi_Bdd_Part_Disj_c)) {

    n = Ddi_BddPartNum(part);
    array = Ddi_BddarrayAlloc(mgr,n);
    
    for (i=0; i<n; i++) {
      Ddi_BddarrayWrite(array,i,Ddi_BddPartRead(part,i)); 
    }
  }
  else {
    /* if not partitioned, make an array of size 1 */
    array = Ddi_BddarrayAlloc(mgr,1);
    Ddi_BddarrayWrite(array,0,part); 
  }
 
  return(array);
}

/**Function*******************************************************************
  Synopsis    [Generate a BDD array from roots of partitioned BDD.]
  Description [Generate a BDD array from roots of partitioned BDD. 
               Nested and multilevel partitions are supported for input BDD.
               The resulting Bddarray simply gathers all BDD roots, 
               regardless of their original depth.]
  SideEffects [none]
  SeeAlso     []
*****************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayMakeFromBddRoots (
  Ddi_Bdd_t *part
)
{
  Ddi_Mgr_t *mgr;
  Ddi_Bddarray_t *array;

  DdiConsistencyCheck(part,Ddi_Bdd_c);
  mgr = Ddi_ReadMgr(part);

  array = (Ddi_Bddarray_t *)DdiGenericAlloc(Ddi_Bddarray_c,mgr);
  array->array = DdiGenBddRoots((Ddi_Generic_t  *)part);

  return(array);
}

/**Function*******************************************************************
  Synopsis    [Return the number of BDDs (entries) in array]
  Description [Return the number of BDDs (entries) in array]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
int
Ddi_BddarrayNum(
  Ddi_Bddarray_t *array
)
{
  if (array==NULL) return 0;
  DdiConsistencyCheck(array,Ddi_Bddarray_c);
  return (DdiArrayNum(array->array));
}

/**Function*******************************************************************
  Synopsis    [Write a BDD in array at given position]
  Description [Write a BDD in array at given position. Previous non NULL entry
    is freed. The written BDD (f) is duplicated]
  SideEffects [none]
  SeeAlso     [Ddi_BddarrayRead]
******************************************************************************/
void
Ddi_BddarrayWrite(
  Ddi_Bddarray_t *array    /* array of BDDs */,
  int pos                  /* position of new element */,
  Ddi_Bdd_t *f           /* BDD to be written */ )
{
  DdiConsistencyCheck(array,Ddi_Bddarray_c);
  DdiArrayWrite(array->array,pos,(Ddi_Generic_t *)f,Ddi_Dup_c);
}

/**Function*******************************************************************
  Synopsis    [Read the BDD at i-th position in array]
  Description [Read the BDD at i-th position in array. As all read operations
    no data duplication is done, so the returned BDD should be duplicated
    if further manipulations are required on it.]
  SideEffects [none]
  SeeAlso     [Ddi_BddarrayWrite]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddarrayRead (
  Ddi_Bddarray_t *array    /* BDD array */,
  int i                    /* position */ )
{
  DdiConsistencyCheck(array,Ddi_Bddarray_c);
  return ((Ddi_Bdd_t *)DdiArrayRead(array->array,i));
}


/**Function*******************************************************************
  Synopsis    [clear array at given position (BDD freed and replaced by NULL)]
  Description [clear array at given position (BDD freed and replaced by NULL)]
  SideEffects [none]
  SeeAlso     [Ddi_BddarrayRead]
******************************************************************************/
void
Ddi_BddarrayClear(
  Ddi_Bddarray_t *array    /* array of BDDs */,
  int pos                  /* position of element to be cleared */
)
{
  DdiConsistencyCheck(array,Ddi_Bddarray_c);
  DdiArrayWrite(array->array,pos,NULL,Ddi_Mov_c);
}

/**Function*******************************************************************
  Synopsis    [Insert a BDD in array at given position]
  Description [Insert a BDD in array at given position. 
    Following entries are shifted down.
    The written BDD (f) is duplicated]
  SideEffects [none]
  SeeAlso     [Ddi_BddarrayExtract]
******************************************************************************/
void
Ddi_BddarrayInsert(
  Ddi_Bddarray_t *array    /* array of BDDs */,
  int pos                  /* position of new element */,
  Ddi_Bdd_t *f           /* BDD to be written */ )
{
  DdiConsistencyCheck(array,Ddi_Bddarray_c);
  DdiArrayInsert(array->array,pos,(Ddi_Generic_t *)f,Ddi_Dup_c);
}

/**Function*******************************************************************
  Synopsis    [Insert a BDD in array at last (new) position]
  Description [In<sert a BDD in array at last (new) position] 
  SideEffects [none]
  SeeAlso     [Ddi_BddarrayExtract]
******************************************************************************/
void
Ddi_BddarrayInsertLast(
  Ddi_Bddarray_t *array    /* array of BDDs */,
  Ddi_Bdd_t *f           /* BDD to be written */ )
{
  DdiConsistencyCheck(array,Ddi_Bddarray_c);
  Ddi_BddarrayInsert(array,Ddi_BddarrayNum(array),f);
}

/**Function*******************************************************************
  Synopsis    [Extract the BDD at i-th position in array]
  Description [Extract the BDD at i-th position in array. 
    The extracted BDD is removed from the array and the following entries
    are shifted up.]
  SideEffects []
  SeeAlso     [Ddi_BddarrayInsert]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddarrayExtract (
  Ddi_Bddarray_t *array    /* BDD array */,
  int i                    /* position */ )
{
  DdiConsistencyCheck(array,Ddi_Bddarray_c);
  return ((Ddi_Bdd_t *)DdiArrayExtract(array->array,i));
}


/**Function*******************************************************************
  Synopsis    [Extract the BDD at i-th position in array]
  Description [Extract the BDD at i-th position in array. 
               The extracted BDD is removed from the array and replaced by 
	       the last array entry.]
  SideEffects []
  SeeAlso     [Ddi_BddarrayInsert]
******************************************************************************/
Ddi_Bdd_t *
Ddi_BddarrayQuickExtract (
  Ddi_Bddarray_t *array    /* BDD array */,
  int i                    /* position */ )
{
  Ddi_Bdd_t *last, *result;

  DdiConsistencyCheck(array,Ddi_Bddarray_c);
  last = Ddi_BddarrayExtract(array, Ddi_BddarrayNum(array)-1);
  if (i == Ddi_BddarrayNum(array))
     return last;

  result = Ddi_BddDup(Ddi_BddarrayRead(array, i));
  Ddi_BddarrayWrite(array, i, last);
  Ddi_Free(last);

  return result;
}


/**Function*******************************************************************
  Synopsis    [Remove array entry at given position]
  Description [Remove array entry at given position.
    This operation is equivalent to extract + free of extracted BDD.]
  SideEffects [none]
  SeeAlso     [Ddi_BddarrayExtract Ddi_BddarrayClear]
******************************************************************************/
void
Ddi_BddarrayRemove(
  Ddi_Bddarray_t *array    /* array of BDDs */,
  int pos                  /* position of element to be cleared */
)
{
  Ddi_Bdd_t *tmp;

  DdiConsistencyCheck(array,Ddi_Bddarray_c);
  tmp = Ddi_BddarrayExtract(array,pos);
  Ddi_Free(tmp);
}

/**Function*******************************************************************
  Synopsis    [Remove array entry at given position]
  Description [Remove array entry at given position.
    This operation is equivalent to extract + free of extracted BDD.]
  SideEffects [none]
  SeeAlso     [Ddi_BddarrayExtract Ddi_BddarrayClear]
******************************************************************************/
void
Ddi_BddarrayQuickRemove(
  Ddi_Bddarray_t *array    /* array of BDDs */,
  int pos                  /* position of element to be cleared */
)
{
  Ddi_Bdd_t *last;

  DdiConsistencyCheck(array,Ddi_Bddarray_c);
  last = Ddi_BddarrayExtract(array, Ddi_BddarrayNum(array)-1);
  if (pos == Ddi_BddarrayNum(array)) {
     Ddi_Free(last);
     return;
  }
  Ddi_BddarrayWrite(array, pos, last);
  Ddi_Free(last);
}


/**Function*******************************************************************
  Synopsis    [Remove NULL array entries]
  Description [Remove NULL array entries]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
Ddi_BddarrayRemoveNull(
  Ddi_Bddarray_t *array    /* array of bdds */
)
{
  DdiConsistencyCheck(array,Ddi_Bddarray_c);
  DdiArrayRemoveNull(array->array);
}


/**Function*******************************************************************
  Synopsis    [Duplicate an array of BDDs]
  Description [Duplicate an array of BDDs]
  SideEffects [none]
  SeeAlso     [Ddi_BddarrayAlloc]
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayDup(
  Ddi_Bddarray_t *old  /* array to be duplicated */ 
)
{
  Ddi_Bddarray_t *newa = NULL;

  if (old != NULL) {
    DdiConsistencyCheck(old,Ddi_Bddarray_c);
    newa = (Ddi_Bddarray_t *)DdiGenericDup((Ddi_Generic_t *)old);
  }
  return (newa);
}


/**Function*******************************************************************
  Synopsis    [Copy an array of BDDs to a destination manager]
  Description [Copy an array of BDDs to a destination manager.
    Variable correspondence is established "by index", i.e. 
    variables with same index in different manager correspond]
  SideEffects [none]
  SeeAlso     [Ddi_BddarrayDup]
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayCopy (
  Ddi_BddMgr *ddm          /* dd Manager */,
  Ddi_Bddarray_t  *old    /* array of BDDs */
)
{
  Ddi_Bddarray_t *newa;

  DdiConsistencyCheck(old,Ddi_Bddarray_c);
  newa = (Ddi_Bddarray_t *)DdiGenericCopy(ddm,(Ddi_Generic_t *)old,NULL,NULL);

  return (newa);
}

/**Function*******************************************************************
  Synopsis    [Append the elements of array2 at the end of array1]
  Description [Append the elements of array2 at the end of array1. As all
    array write/insert operations, new entries are duplicated.]
  SideEffects [none]
  SeeAlso     [Ddi_BddarrayWrite Ddi_BddarrayInsert]
******************************************************************************/

void
Ddi_BddarrayAppend (
  Ddi_Bddarray_t *array1   /* first array */,
  Ddi_Bddarray_t *array2   /* array to be appended */
)
{
  DdiConsistencyCheck(array1,Ddi_Bddarray_c);
  DdiConsistencyCheck(array2,Ddi_Bddarray_c);
  DdiArrayAppend(array1->array,array2->array);
}


/**Function*******************************************************************
  Synopsis    [Return the number of BDD nodes in a BDD array]
  Description [Count the  numbers of BDD nodes in a BDD array. Shared
    nodes are counted once.]
  SideEffects [none]
  SeeAlso     [Ddi_BddSize]
*****************************************************************************/

int
Ddi_BddarraySize (
  Ddi_Bddarray_t *array
)
{
  if (array == NULL) {
    return(0);
  }
  DdiConsistencyCheck(array,Ddi_Bddarray_c);
  return (DdiGenericBddSize((Ddi_Generic_t *)array,-1));
}

/**Function*******************************************************************

  Synopsis    [Writes array of BDDs in a dump file]

  Description [This function stores a BDD array using the
               DDDMP format. The parameter "mode" 
               can be DDDMP_MODE_TEXT, DDDMP_MODE_COMPRESSED or
               DDDMP_MODE_AUTOMATIC.<br>
               The function returns 1 if succefully stored, 0 otherwise.]

  SideEffects [none]

  SeeAlso     [Dddmp_cuddBddStore Ddi_BddStore]

*****************************************************************************/

int
Ddi_BddarrayStore (
  Ddi_Bddarray_t *array    /* array to be stored */,
  char *ddname            /* dd name (or NULL) */,
  char **vnames           /* array of variable names (or NULL) */,
  char **rnames           /* array of root names (or NULL) */,
  int *vauxids             /* array of aux var ids (or NULL) */,
  int mode                /* storing mode selector */,
  char *fname             /* file name */,
  FILE *fp                /* pointer to the store file */ )
{
  return (DdiArrayStore (array->array,ddname,vnames,
    rnames,vauxids,mode,fname,fp));
}

/**Function*******************************************************************

  Synopsis    [Writes array of BDDs in a dump file]

  Description [This function stores a BDD array using the
               DDDMP format. The parameter "mode" 
               can be DDDMP_MODE_TEXT, DDDMP_MODE_COMPRESSED or
               DDDMP_MODE_AUTOMATIC.<br>
               The function returns 1 if succefully stored, 0 otherwise.]

  SideEffects [none]

  SeeAlso     [Dddmp_cuddBddStore Ddi_BddStore]

*****************************************************************************/

int
Ddi_BddarrayStoreCnf (
  Ddi_Bddarray_t *array         /* IN: array to be stored */,
  Dddmp_DecompCnfStoreType mode /* IN: storing mode selector */,
  int noHeader                  /* IN: storing mode selector */,
  char **vnames                 /* IN: array of variable names (or NULL) */,
  int *vbddids                  /* IN: array of bdd var ids (or NULL) */,
  int *vauxids                  /* IN: array of aux var ids (or NULL) */,
  int *vcnfids                  /* IN: array of cnf var ids (or NULL) */,
  int idInitial                 /* IN: first aux var cnf id */,
  int edgeInMaxTh               /* IN: Max # Incoming Edges */,
  int pathLengthMaxTh           /* IN: Max Path Length */,
  char *fname                   /* IN: file name */,
  FILE *fp                      /* IN: pointer to the store file */,
  int *clauseNPtr               /* OUT: number of added clauses */,
  int *varNewNPtr               /* OUT: number of added variables */
)
{
  return (DdiArrayStoreCnf (array->array, mode, noHeader, vnames, vbddids, 
    vauxids, vcnfids, idInitial, edgeInMaxTh, pathLengthMaxTh, 
    fname, fp, clauseNPtr, varNewNPtr));
}

/**Function*******************************************************************

  Synopsis    [Writes array of BDDs in a dump file in Blif format.]

  Description [This function stores a BDD array using the function
    implemented inside the CUDD package.
    It receives the arrays of input variables (BDD nodes variables) and
    output variables (BDD root nodes names).
    The function returns 1 if the array is succefully stored.
    It returns 0 otherwise.
  ]

  SideEffects [none]

  SeeAlso     [Dddmp_cuddBddStore Ddi_BddStore]

*****************************************************************************/

int
Ddi_BddarrayStoreBlif (
  Ddi_Bddarray_t *array   /* IN: Array to be stored */,
  char **inputNames       /* IN: Array of variable names (or NULL) */,
  char **outputNames      /* IN: Array of root names (or NULL) */,
  char *modelName         /* IN: Model Name */,
  char *fname             /* IN: File name */,
  FILE *fp                /* IN: Pointer to the store file */
  )
{
  int retValue;

  retValue = DdiArrayStoreBlif (array->array, inputNames,
    outputNames, modelName, fname, fp);

  return (retValue);
}

/**Function*******************************************************************

  Synopsis    [Writes array of BDDs in a dump file in prefix notation.]

  Description [This function stores a BDD array using the function
    implemented inside the DDDMP package.
    It receives the arrays of input variables (BDD nodes variables) and
    output variables (BDD root nodes names).
    The function returns 1 if the array is succefully stored.
    It returns 0 otherwise.
  ]

  SideEffects [none]

  SeeAlso     [Dddmp_cuddBddStore Ddi_BddStore]

*****************************************************************************/

int
Ddi_BddarrayStorePrefix (
  Ddi_Bddarray_t *array   /* IN: Array to be stored */,
  char **inputNames       /* IN: Array of variable names (or NULL) */,
  char **outputNames      /* IN: Array of root names (or NULL) */,
  char *modelName         /* IN: Model Name */,
  char *fname             /* IN: File name */,
  FILE *fp                /* IN: Pointer to the store file */
  )
{
  int retValue;

  retValue = DdiArrayStorePrefix (array->array, inputNames,
    outputNames, modelName, fname, fp);

  return (retValue);}

/**Function*******************************************************************

  Synopsis    [Writes array of BDDs in a dump file in SMV format.]

  Description [This function stores a BDD array using the function
    implemented inside the CUDD package.
    It receives the arrays of input variables (BDD nodes variables) and
    output variables (BDD root nodes names).
    The function returns 1 if the array is succefully stored.
    It returns 0 otherwise.
  ]

  SideEffects [none]

  SeeAlso     [Dddmp_cuddBddStore Ddi_BddStore]

*****************************************************************************/

int
Ddi_BddarrayStoreSmv (
  Ddi_Bddarray_t *array   /* IN: Array to be stored */,
  char **inputNames       /* IN: Array of variable names (or NULL) */,
  char **outputNames      /* IN: Array of root names (or NULL) */,
  char *modelName         /* IN: Model Name */,
  char *fname             /* IN: File name */,
  FILE *fp                /* IN: Pointer to the store file */
  )
{
  int retValue;

  retValue = DdiArrayStoreSmv (array->array, inputNames,
    outputNames, modelName, fname, fp);

  return (retValue);
}

/**Function*******************************************************************

  Synopsis    [Reads array of BDDs from a dump file]

  Description [This function loads a BDDs'array.<br>
               The BDD on file must be in the DDDMP format. The parameter 
               "mode" can be DDDMP_MODE_TEXT, DDDMP_MODE_COMPRESSED or
               DDDMP_MODE_AUTOMATIC.<br>
               The function returns the pointer of array if succefully
               loaded, NULL otherwise.]

  SideEffects [none]

  SeeAlso     [Dddmp_cuddBddLoad,Ddi_BddLoad]

*****************************************************************************/

Ddi_Bddarray_t *
Ddi_BddarrayLoad (
  Ddi_BddMgr *dd           /* dd manager */,
  char **vnames           /* variable names */,
  int *vauxids            /* variable auxids */,
  int mode                /* storing mode selector */,
  char *file              /* name file */,
  FILE *fp                /* file pointer */ )             
{
  DdNode   **roots;        /* array of BDD roots to be loaded */
  Ddi_Bddarray_t *array;    /* descriptor of array */
  int nroots,              /* number of BDD roots */
      i;
  char **rnames = Pdtutil_Alloc(char *,DDI_MAX_BDDARRAY_LEN);
  for (i=0;i<DDI_MAX_BDDARRAY_LEN;i++) {
    rnames[i]=NULL;
  }

  nroots = Dddmp_cuddBddArrayLoad(dd->mgrCU,DDDMP_ROOT_MATCHLIST,rnames,
             DDDMP_VAR_MATCHNAMES,vnames,vauxids,NULL,
             mode,file,fp,&roots);

  Pdtutil_Assert(nroots<DDI_MAX_BDDARRAY_LEN,
    "DDI_MAX_BDDARRAY_LEN exceeded by BDD array len. Please increase it");

  if (nroots<=0) {
    Pdtutil_Free(rnames);   
    return (NULL);
  }

  array = Ddi_BddarrayMakeFromCU(dd,roots,nroots);

  for (i=0;i<nroots;i++) {
    if (rnames[i] != NULL) {
       Ddi_SetName(Ddi_BddarrayRead(array,i),rnames[i]);
    }
    /* Deref is required since dddmp references loaded BDDs */
    Cudd_RecursiveDeref(dd->mgrCU,roots[i]);
  }

  Pdtutil_Free(rnames);   
  Pdtutil_Free(roots);   

  return (array); 
}


/**Function*******************************************************************
  Synopsis    [Return the support of a BDD array]
  Description [Returns a var-set representing the global support of the array]
  SideEffects []
  SeeAlso     []
*****************************************************************************/
Ddi_Varset_t *
Ddi_BddarraySupp (
  Ddi_Bddarray_t *array   /* BDDs'array */ )
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(array);
  if (Ddi_BddarrayNum(array) == 0) {
    return (Ddi_VarsetVoid(ddm));
  }
  if (Ddi_BddIsAig(Ddi_BddarrayRead(array,0))) {
    return DdiAigArraySupp(array);
  }


  return((Ddi_Varset_t *)DdiGenericOp1(Ddi_BddSupp_c,Ddi_Generate_c,array));
} 

/**Function*******************************************************************
  Synopsis    [Return the support of a BDD array]
  Description [Returns a var-set representing the global support of the array]
  SideEffects []
  SeeAlso     []
*****************************************************************************/
Ddi_Vararray_t *
Ddi_BddarraySuppVararray (
  Ddi_Bddarray_t *array   /* BDDs'array */ )
{
  Ddi_Varset_t *supp = Ddi_BddarraySupp(array);
  Ddi_Vararray_t *vA = Ddi_VararrayMakeFromVarset(supp,1);
  Ddi_Free(supp);
  return vA;
} 

/**Function*******************************************************************
  Synopsis    [OLD(pdt-1). Return a vector of supports of BDD array elements]
  Description [OLD(pdt-1). Return a vector of supports of BDD array elements.
    Should be replaced by Varsetarray usage]
  SideEffects [none]
  SeeAlso     []
*****************************************************************************/

Ddi_Varset_t **
Ddi_BddarraySuppArray (
  Ddi_Bddarray_t *fArray   /* array of function */ )
{
  Ddi_Varset_t **fArraySupp;  /* array of function supports */
  Ddi_Bdd_t *F;              /* function */
  Ddi_Varset_t *S;        /* support of function */
  int i,n;                    

  n = Ddi_BddarrayNum(fArray);    /* length of array */

  fArraySupp = Pdtutil_Alloc(Ddi_Varset_t *,n);

  for (i=0;i<n;i++) {
    F = Ddi_BddarrayRead(fArray,i);
    S = Ddi_BddSupp(F);
    fArraySupp[i]=S;
  }

  return (fArraySupp); 
}

/**Function********************************************************************
  Synopsis    [Generate an array of literals from variables]
  Description [Generate an array of literals from variables.
    Literals are all either affirmed or complemented.]
  SideEffects [none]
  SeeAlso     [Ddi_Bdd]
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayMakeLiteralsAigOrBdd (
  Ddi_Vararray_t *vA,
  int polarity    /* non 0: affirmed (v), 0: complemented literals (!v) */,
  int doAig
)
{
  if (doAig) return Ddi_BddarrayMakeLiteralsAig (vA,polarity);
  else return Ddi_BddarrayMakeLiterals (vA,polarity);
}

/**Function********************************************************************
  Synopsis    [Generate an array of literals from variables]
  Description [Generate an array of literals from variables.
    Literals are all either affirmed or complemented.]
  SideEffects [none]
  SeeAlso     [Ddi_Bdd]
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayMakeLiterals (
  Ddi_Vararray_t *vA,
  int polarity    /* non 0: affirmed (v), 0: complemented literals (!v) */
)
{
  int i;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(vA);
  Ddi_Bddarray_t *litArray = Ddi_BddarrayAlloc(ddm,Ddi_VararrayNum(vA));
  for (i=0; i<Ddi_VararrayNum(vA); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(vA,i);
    Ddi_Bdd_t *lit = Ddi_BddMakeLiteral(v,polarity);
    Ddi_BddarrayWrite(litArray,i,lit);
    Ddi_Free(lit);
  }
  return(litArray);
}

/**Function********************************************************************
  Synopsis    [Generate an array of constants]
  Description [Generate an array of constants]
  SideEffects [none]
  SeeAlso     [Ddi_Bdd]
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayMakeConst (
  Ddi_Mgr_t *ddm,
  int n,
  int constVal    
)
{
  int i;
  Ddi_Bddarray_t *constArray = Ddi_BddarrayAlloc(ddm,n);
  for (i=0; i<n; i++) {
    Ddi_Bdd_t *fc = Ddi_BddMakeConst(ddm,constVal);
    Ddi_BddarrayWrite(constArray,i,fc);
    Ddi_Free(fc);
  }
  return(constArray);
}

/**Function********************************************************************
  Synopsis    [Generate an array of constants]
  Description [Generate an array of constants]
  SideEffects [none]
  SeeAlso     [Ddi_Bdd]
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayMakeConstAig (
  Ddi_Mgr_t *ddm,
  int n,
  int constVal
)
{
  int i;
  Ddi_Bddarray_t *constArray = Ddi_BddarrayAlloc(ddm,n);
  for (i=0; i<n; i++) {
    Ddi_Bdd_t *fc = Ddi_BddMakeConstAig(ddm,constVal);
    Ddi_BddarrayWrite(constArray,i,fc);
    Ddi_Free(fc);
  }
  return(constArray);
}

/**Function********************************************************************
  Synopsis    [Generate an array of AIG literals from variables]
  Description [Generate an array of AIG literals from variables.
    Literals are all either affirmed or complemented.]
  SideEffects [none]
  SeeAlso     [Ddi_Bdd]
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayMakeLiteralsAig (
  Ddi_Vararray_t *vA,
  int polarity    /* non 0: affirmed (v), 0: complemented literals (!v) */
)
{
  int i;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(vA);
  Ddi_Bddarray_t *litArray = Ddi_BddarrayAlloc(ddm,Ddi_VararrayNum(vA));
  for (i=0; i<Ddi_VararrayNum(vA); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(vA,i);
    Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(v,polarity);
    Ddi_BddarrayWrite(litArray,i,lit);
    Ddi_Free(lit);
  }
  return(litArray);
}

/**Function********************************************************************
  Synopsis    [Function composition x <- g in f. New result is generated]
  Description [Function composition x <- g in f. New result is generated.  
               Vector composition algorithm is used.]
  SideEffects [none]
  SeeAlso     [Ddi_BddSwapVars Ddi_BddSubstVars]
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayCompose (
  Ddi_Bddarray_t  *f,
  Ddi_Vararray_t *x  /* array of variables */,
  Ddi_Bddarray_t *g  /* array of functions */
)
{
  Ddi_Bddarray_t *fDup;

  if (x==NULL || Ddi_VararrayNum(x)==0 || Ddi_BddarrayNum(f)==0) {
    return Ddi_BddarrayDup(f);
  }
  fDup = Ddi_BddarrayDup(f);
  Ddi_BddarrayComposeAcc(fDup,x,g);
  return (fDup);
}

/**Function********************************************************************
  Synopsis    [Function composition x <- g in f. New result is accumulated]
  Description [Function composition x <- g in f. New result is accumulated.  
               Vector composition algorithm is used.]
  SideEffects [none]
  SeeAlso     [Ddi_BddSwapVars Ddi_BddSubstVars]
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayComposeAcc (
  Ddi_Bddarray_t  *f,
  Ddi_Vararray_t *x  /* array of variables */,
  Ddi_Bddarray_t *g  /* array of functions */
)
{
  int i;
  if (x==NULL || Ddi_VararrayNum(x)==0 || Ddi_BddarrayNum(f)==0) {
    return (f);
  }
  if (Ddi_BddIsAig(Ddi_BddarrayRead(f,0))) {
    return Ddi_AigarrayComposeNoMultipleAcc(f,x,g);
  }

  for (i=0;i<Ddi_BddarrayNum(f);i++) {
    Ddi_BddComposeAcc(Ddi_BddarrayRead(f,i),x,g);
  }

  return (f);
}

/**Function********************************************************************
  Synopsis    [Variable substitution x <- y in f. New result is generated]
  Description [Variable substitution x <- y in f. New result is generated.
               Variable correspondence is established by position in x, y.
               Substitution is done by compose. This differs from variable
               swapping since some y vars may be present in x as well as in
               the support of f.]
  SideEffects [none]
  SeeAlso     [Ddi_BddSwapVars Ddi_BddSubstVarsAcc]
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarraySubstVars (
  Ddi_Bddarray_t  *fA,
  Ddi_Vararray_t *x  /* 1-st array of variables */,
  Ddi_Vararray_t *y  /* 2-nd array of variables */
)
{
  Ddi_Bddarray_t *fAsubst = Ddi_BddarrayDup(fA);
  return (Ddi_BddarraySubstVarsAcc(fAsubst,x,y));
}

/**Function********************************************************************
  Synopsis    [Variable substitution x <-> y in f. New result is generated]
  Description [Variable substitution x <-> y in f. New result is generated.
               Variable correspondence is established by position in x, y.
               Substitution is done by compose.]
  SideEffects [none]
  SeeAlso     [Ddi_BddSwapVars Ddi_BddSubstVarsAcc]
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarraySwapVars (
  Ddi_Bddarray_t  *fA,
  Ddi_Vararray_t *x  /* 1-st array of variables */,
  Ddi_Vararray_t *y  /* 2-nd array of variables */
)
{
  Ddi_Bddarray_t *fAsubst = Ddi_BddarrayDup(fA);
  return (Ddi_BddarraySwapVarsAcc(fAsubst,x,y));
}

/**Function********************************************************************
  Synopsis    [Variable substitution x <- y in f. New result is accumulated]
  Description [Variable substitution x <- y in f. New result is accumulated.
               Variable correspondence is established by position in x, y.
               Substitution is done by compose. This differs from variable
               swapping since some y vars may be present in x as well as in
               the support of f.]
  SideEffects [none]
  SeeAlso     [Ddi_BddSwapVars Ddi_BddSubstVars]
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarraySubstVarsAcc (
  Ddi_Bddarray_t  *fA,
  Ddi_Vararray_t *x  /* 1-st array of variables */,
  Ddi_Vararray_t *y  /* 2-nd array of variables */
)
{
  Ddi_Bddarray_t *lits = Ddi_BddarrayMakeLiteralsAig(y, 1);
  Ddi_BddarrayComposeAcc(fA,x,lits);
  Ddi_Free(lits);
  return (fA);
}


/**Function********************************************************************
  Synopsis    [Variable substitution x <-> y in f. New result is accumulated]
  Description [Variable substitution x <-> y in f. 
  New result is accumulated. 
               Variable correspondence is established by position in x, y.
               Substitution is done by compose. ]
  SideEffects [none]
  SeeAlso     [Ddi_BddSwapVars Ddi_BddSubstVars]
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarraySwapVarsAcc (
  Ddi_Bddarray_t  *fA,
  Ddi_Vararray_t *x  /* 1-st array of variables */,
  Ddi_Vararray_t *y  /* 2-nd array of variables */
)
{
  Ddi_Bddarray_t *lits = Ddi_BddarrayMakeLiteralsAig(y, 1);
  Ddi_Bddarray_t *litsX = Ddi_BddarrayMakeLiteralsAig(x, 1);
  Ddi_Vararray_t *vTot = Ddi_VararrayDup(x);

  Ddi_VararrayAppend(vTot,y);
  Ddi_BddarrayAppend(lits,litsX);

  Ddi_BddarrayComposeAcc(fA,vTot,lits);
  Ddi_Free(litsX);
  Ddi_Free(vTot);
  Ddi_Free(lits);
  return (fA);
}


/**Function********************************************************************
  Synopsis    [Function composition x <- newv in f. New result is accumulated]
  Description [Function composition x <- newv in f. New result is accumulated.
               New fresh vars are generated.]
  SideEffects [none]
  SeeAlso     [Ddi_BddSwapVars Ddi_BddSubstVars]
******************************************************************************/
Ddi_Vararray_t *
Ddi_BddarrayNewVarsAcc(
  Ddi_Bddarray_t *fA,
  Ddi_Vararray_t *x  /* array of OLD variables */,
  char *prefix,
  char *suffix,
  int useBddVars
)
{
  Ddi_Bddarray_t *newVarsLits;
  Ddi_Vararray_t *newVars, *oldVars = Ddi_BddarraySuppVararray (fA);

  Ddi_Varset_t *oldS = Ddi_VarsetMakeFromArray(oldVars);
  Ddi_Varset_t *xS = Ddi_VarsetMakeFromArray(x);

  Ddi_VarsetIntersectAcc(oldS,xS);
  Ddi_Free(xS);

  Ddi_VararrayIntersectAcc(oldVars,x);

  xS = Ddi_VarsetMakeFromArray(oldVars);
  Pdtutil_Assert(Ddi_VarsetEqual(xS,oldS),"wrong vararray op");
  Ddi_Free(xS);
  Ddi_Free(oldS);

  newVars = useBddVars ? 
    Ddi_VararrayMakeNewVars (oldVars, prefix, suffix, 1) :
    Ddi_VararrayMakeNewAigVars (oldVars, prefix, suffix);
  newVarsLits = Ddi_BddarrayMakeLiteralsAig (newVars,1);
  Ddi_AigarrayComposeAcc(fA,oldVars,newVarsLits);
  Ddi_Free(newVarsLits);
  Ddi_Free(oldVars);

  return newVars;
}

/**Function********************************************************************
  Synopsis    [Cofactor with variable. New result is generated]
  Description [Cofactor with variable. New result is generated]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarraySortBySize (
  Ddi_Bddarray_t  *fA,
  int increasing
)
{
  Ddi_Bddarray_t *sorted;
  Ddi_Bdd_t *fpart = Ddi_BddMakePartConjFromArray(fA);
  Ddi_BddPartSortBySizeAcc(fpart, increasing);
  sorted = Ddi_BddarrayMakeFromBddPart(fpart);
  Ddi_Free(fpart);
  return sorted;
}

/**Function********************************************************************
  Synopsis    [Cofactor with variable. New result is generated]
  Description [Cofactor with variable. New result is generated]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarraySortBySizeAcc (
  Ddi_Bddarray_t  *fA,
  int increasing
)
{
  Ddi_Bddarray_t *sorted = Ddi_BddarraySortBySize(fA,increasing);
  Ddi_DataCopy(fA,sorted);
  Ddi_Free(sorted);
  return fA;
}

/**Function********************************************************************
  Synopsis    [Cofactor with variable. New result is generated]
  Description [Cofactor with variable. New result is generated]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayCofactor (
  Ddi_Bddarray_t  *fA,
  Ddi_Var_t  *v,
  int phase
)
{
  Ddi_Bddarray_t *fANew = Ddi_BddarrayDup(fA);
  return Ddi_BddarrayCofactorAcc(fANew,v,phase);
}

/**Function********************************************************************
  Synopsis    [Cofactor with variable. New result is accumulated]
  Description [Cofactor with variable. New result is accumulated]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayCofactorAcc (
  Ddi_Bddarray_t  *fA,
  Ddi_Var_t  *v,
  int phase
)
{
  int i;

  DdiConsistencyCheck(fA,Ddi_Bddarray_c);
  DdiConsistencyCheck(v,Ddi_Var_c);

  if (Ddi_BddarrayNum(fA)>0) {
    if (Ddi_BddIsAig(Ddi_BddarrayRead(fA,0))) {
      DdiAigArrayCofactorAcc (fA,v,phase);
    }
    else {
      for (i=0; i<Ddi_BddarrayNum(fA); i++) {
        DdiAigCofactorAcc (Ddi_BddarrayRead(fA,i),v,phase);
      }
    }
  }
  return(fA);
}

/**Function********************************************************************
  Synopsis    [Cofactor with variable array. New result is accumulated]
  Description [Cofactor with variable array. New result is accumulated]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayVararrayCofactorAcc (
  Ddi_Bddarray_t  *fA,
  Ddi_Vararray_t  *vA,
  int phase
)
{
  Ddi_BddMgr *ddm = Ddi_ReadMgr(fA);
  int i, n = Ddi_VararrayNum(vA);

  Ddi_Bddarray_t *subst = Ddi_BddarrayMakeConstAig(ddm,n,phase);

  Ddi_BddarrayComposeAcc(fA,vA,subst);

  Ddi_Free(subst);

  return (fA);
}

/**Function********************************************************************
  Synopsis    [Cofactor with variable array. New result is generated]
  Description [Cofactor with variable array. New result is generated]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayVararrayCofactor (
  Ddi_Bddarray_t  *fA,
  Ddi_Vararray_t  *vA,
  int phase
)
{
  Ddi_Bddarray_t *newFA = Ddi_BddarrayDup(fA);

  Ddi_BddarrayVararrayCofactorAcc (newFA,vA,phase);

  return (newFA);
}




/**Function********************************************************************
  Synopsis    [Generate a Ddi_Bdd_t relation from array of functions]
  Description [Generate a Ddi_Bddarray_t with range corresponding to 
    cube.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Ddi_BddarrayRangeMakeFromCube (
  Ddi_Bdd_t *cube           /* cube */,
  Ddi_Vararray_t *vars        /* array of range variables */
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(cube);
  Ddi_Bdd_t *rel, *r_i, *lit;
  int i, n;
  int isAig = Ddi_BddIsAig(cube);
  Ddi_Bdd_t *cubeAig = Ddi_BddDup(cube), *cubePart;
  Ddi_Vararray_t *vA = Ddi_VararrayAlloc(ddm,0);
  Ddi_Bddarray_t *subst = Ddi_BddarrayAlloc(ddm,0);
  Ddi_Bdd_t *myOne = Ddi_BddMakeConstAig(ddm,1);
  Ddi_Bdd_t *myZero = Ddi_BddMakeConstAig(ddm,0);
  Ddi_Bddarray_t *cubeA = Ddi_BddarrayMakeLiteralsAig(vars,1);

  DdiConsistencyCheck(cube,Ddi_Bdd_c);
  DdiConsistencyCheck(vars,Ddi_Vararray_c);

  if (!isAig) {
    Ddi_BddSetAig(cubeAig);
  }
  cubePart = Ddi_AigPartitionTop(cubeAig,0);
  Ddi_Free(cubeAig);

  n = Ddi_BddPartNum(cubePart);

  for (i=0; i<n; i++) {
    Ddi_Bdd_t *l_i = Ddi_BddPartRead(cubePart,i);
    if (Ddi_BddSize(l_i)==1) {
      /* literal */
      Ddi_Var_t *v = Ddi_BddTopVar(l_i);
      Ddi_VararrayWrite(vA,i,v);
      if (Ddi_BddIsComplement(l_i)) {
	Ddi_BddarrayWrite(subst,i,myZero);
      }
      else {
	Ddi_BddarrayWrite(subst,i,myOne);
      }
    }
    else {
      Pdtutil_Assert(0,"wrong cube encoding");
      /* not a literal */
    }
  }

  Ddi_BddarrayComposeAcc(cubeA,vA,subst);

  Ddi_Free(myZero);
  Ddi_Free(myOne);

  Ddi_Free(subst);
  Ddi_Free(vA);
  Ddi_Free(cubePart);

  return cubeA;

}



/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

void searchSingleVars (Ddi_Bddarray_t *fA) {
  Ddi_Bddarray_t *fDup = Ddi_BddarrayDup(fA);
  for (int i=0; i<Ddi_BddarrayNum(fA); i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayExtract(fDup,i);
    Ddi_Vararray_t *tot = Ddi_BddarraySuppVararray(fDup);
    Ddi_Vararray_t *s_i = Ddi_BddSuppVararray(f_i);
    Ddi_VararrayDiffAcc(s_i,tot);
    if (Ddi_VararrayNum(s_i)>0) {
      printf("%d single vars for f[%d]: ", Ddi_VararrayNum(s_i), i);
      Ddi_VararrayPrint(s_i);
    }
    Ddi_BddarrayInsert(fDup,i,f_i);
    Ddi_Free(f_i);
    Ddi_Free(s_i);
    Ddi_Free(tot);
  }
  Ddi_Free(fDup);
}


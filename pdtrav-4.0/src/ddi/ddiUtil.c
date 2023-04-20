/**CFile***********************************************************************

  FileName    [ddiUtil.c]

  PackageName [ddi]

  Synopsis    [utility functions]

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
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Given a string it Returns an Enumerated type]

  Description [It receives a string; to facilitate the user that string
    can be an easy-to-remember predefined code or an integer number
    (interpreted as a string).
    It returns the dense method type.]

  SideEffects [none]

  SeeAlso     []
******************************************************************************/

Cuplus_PruneHeuristic_e
Ddi_ProfileHeuristicString2Enum (
  char *string     /* String to Analyze */
)
{
  if (strcmp (string, "rec")==0 || strcmp (string, "0")==0) {
    return (Cuplus_Recursions);
  }

  if (strcmp (string, "recShr")==0 || strcmp (string, "1")==0) {
    return (Cuplus_RecursionsWithSharing);
  }

  if (strcmp (string, "normSize")==0 || strcmp (string, "2")==0) {
    return (Cuplus_NormSizePrune);
  }

  if (strcmp (string, "size")==0 || strcmp (string, "3")==0) {
    return (Cuplus_SizePrune);
  }

  if (strcmp (string, "normSizeLight")==0 || strcmp (string, "4")==0) {
    return (Cuplus_NormSizePruneLight);
  }

  if (strcmp (string, "normSizeHeavy")==0 || strcmp (string, "5")==0) {
    return (Cuplus_NormSizePruneHeavy);
  }

  if (strcmp (string, "sizeLight")==0 || strcmp (string, "6")==0) {
    return (Cuplus_SizePruneLight);
  }

  if (strcmp (string, "sizeHeavy")==0 || strcmp (string, "7")==0) {
    return (Cuplus_SizePruneHeavy);
  }

  if (strcmp (string, "sizeLightNoRecur")==0 || strcmp (string, "8")==0) {
    return (Cuplus_SizePruneLight);
  }

  if (strcmp (string, "sizeHeavyNoRecur")==0 || strcmp (string, "9")==0) {
    return (Cuplus_SizePruneHeavy);
  }

  if (strcmp (string, "none")==0 || strcmp (string, "10")==0) {
    return (Cuplus_None);
  }

  Pdtutil_Warning (1, "Choice Not Allowed For Profile Method.");
  return (Cuplus_None);
}

/**Function********************************************************************

 Synopsis    [Given an Enumerated type Returns a string]

 Description []

 SideEffects [none]

 SeeAlso     []

******************************************************************************/

char *
Ddi_ProfileHeuristicEnum2String (
  Cuplus_PruneHeuristic_e enumType
)
{
  switch (enumType) {

    case Cuplus_Recursions:
      return ("rec");
      break;
    case Cuplus_RecursionsWithSharing:
      return ("recShr");
      break;
    case Cuplus_NormSizePrune:
      return ("normSize");
      break;
    case Cuplus_SizePrune:
      return ("size");
      break;
    case Cuplus_NormSizePruneLight:
      return ("normSizeLight");
      break;
    case Cuplus_NormSizePruneHeavy:
      return ("normSizeHeavy");
      break;
    case Cuplus_SizePruneLight:
      return ("sizeLight");
      break;
    case Cuplus_SizePruneHeavy:
      return ("sizeHeavy");
      break;
    case Cuplus_SizePruneLightNoRecur:
      return ("sizeLightNoRecur");
      break;
    case Cuplus_SizePruneHeavyNoRecur:
      return ("sizeHeavyNoRecur");
      break;
    case Cuplus_None:
      return ("none");
      break;
    default:
      Pdtutil_Warning (1, "Choice Not Allowed.");
      return ("none");
      break;

  }
}



/**Function********************************************************************

  Synopsis    [Given a string it Returns an Enumerated type]

  Description [It receives a string; to facilitate the user that string
    can be an easy-to-remember predefined code or an integer number
    (interpreted as a string).
    It returns the dense method type.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Cudd_ReorderingType
Ddi_ReorderingMethodString2Enum (
  char *string     /* String to Analyze */
  )
{
  if (strcmp (string, "same")==0 || strcmp (string, "0")==0) {
    return (CUDD_REORDER_SAME);
  }

  if (strcmp (string, "none")==0 || strcmp (string, "1")==0) {
    return (CUDD_REORDER_NONE);
  }

  if (strcmp (string, "random")==0 || strcmp (string, "2")==0) {
    return (CUDD_REORDER_RANDOM);
  }

  if (strcmp (string, "pivot")==0 || strcmp (string, "3")==0) {
    return (CUDD_REORDER_RANDOM_PIVOT);
  }

  if (strcmp (string, "sift")==0 || strcmp (string, "4")==0) {
    return (CUDD_REORDER_SIFT);
  }

  if (strcmp (string, "siftCon")==0 || strcmp (string, "5")==0) {
    return (CUDD_REORDER_SIFT_CONVERGE);
  }

  if (strcmp (string, "siftSym")==0 || strcmp (string, "6")==0) {
    return (CUDD_REORDER_SYMM_SIFT);
  }

  if (strcmp (string, "siftSymCon")==0 || strcmp (string, "7")==0) {
    return (CUDD_REORDER_SYMM_SIFT_CONV);
  }

  if (strcmp (string, "win2")==0 || strcmp (string, "8")==0) {
    return (CUDD_REORDER_WINDOW2);
  }

  if (strcmp (string, "win3")==0 || strcmp (string, "9")==0) {
    return (CUDD_REORDER_WINDOW3);
  }

  if (strcmp (string, "win4")==0 || strcmp (string, "10")==0) {
    return (CUDD_REORDER_WINDOW4);
  }

  if (strcmp (string, "win2Con")==0 || strcmp (string, "11")==0) {
    return (CUDD_REORDER_WINDOW2_CONV);
  }

  if (strcmp (string, "win3Con")==0 || strcmp (string, "12")==0) {
    return (CUDD_REORDER_WINDOW3_CONV);
  }

  if (strcmp (string, "win4Con")==0 || strcmp (string, "13")==0) {
    return (CUDD_REORDER_WINDOW4_CONV);
  }

  if (strcmp (string, "siftGroup")==0 || strcmp (string, "14")==0) {
    return (CUDD_REORDER_GROUP_SIFT);
  }

  if (strcmp (string, "siftGroupCon")==0 || strcmp (string, "15")==0) {
    return (CUDD_REORDER_GROUP_SIFT_CONV);
  }

  if (strcmp (string, "annealing")==0 || strcmp (string, "16")==0) {
    return (CUDD_REORDER_ANNEALING);
  }

  if (strcmp (string, "genetic")==0 || strcmp (string, "17")==0) {
    return (CUDD_REORDER_GENETIC);
  }

  if (strcmp (string, "linear")==0 || strcmp (string, "18")==0) {
    return (CUDD_REORDER_LINEAR);
  }

  if (strcmp (string, "linearCon")==0 || strcmp (string, "19")==0) {
    return (CUDD_REORDER_LINEAR_CONVERGE);
  }

  if (strcmp (string, "exact")==0 || strcmp (string, "20")==0) {
    return (CUDD_REORDER_EXACT);
  }

  Pdtutil_Warning (1, "Choice Not Allowed For Density Method.");
  return (CUDD_REORDER_SAME);
}

/**Function********************************************************************

  Synopsis    [Given an Enumerated type Returns a string]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

char *
Ddi_ReorderingMethodEnum2String (
  Cudd_ReorderingType enumType
  )
{
  switch (enumType) {
    case CUDD_REORDER_SAME:
      return ("same");
      break;
    case CUDD_REORDER_NONE:
      return ("none");
      break;
    case CUDD_REORDER_RANDOM:
      return ("random");
      break;
    case CUDD_REORDER_RANDOM_PIVOT:
      return ("pivot");
      break;
    case CUDD_REORDER_SIFT:
      return ("sift");
      break;
    case CUDD_REORDER_SIFT_CONVERGE:
      return ("siftCon");
      break;
    case CUDD_REORDER_SYMM_SIFT:
      return ("siftSym");
      break;
    case CUDD_REORDER_SYMM_SIFT_CONV:
      return ("siftSymCon");
      break;
    case CUDD_REORDER_WINDOW2:
      return ("window2");
      break;
    case CUDD_REORDER_WINDOW3:
      return ("window3");
      break;
    case CUDD_REORDER_WINDOW4:
      return ("window4");
      break;
    case CUDD_REORDER_WINDOW2_CONV:
      return ("window2Con");
      break;
    case CUDD_REORDER_WINDOW3_CONV:
      return ("window3Con");
      break;
    case CUDD_REORDER_WINDOW4_CONV:
      return ("window4Con");
      break;
    case CUDD_REORDER_GROUP_SIFT:
      return ("sift");
      break;
    case CUDD_REORDER_GROUP_SIFT_CONV:
      return ("siftCon");
      break;
    case CUDD_REORDER_ANNEALING:
      return ("annealing");
      break;
    case CUDD_REORDER_GENETIC:
      return ("genetic");
      break;
    case CUDD_REORDER_LINEAR:
      return ("linear");
      break;
    case CUDD_REORDER_LINEAR_CONVERGE:
      return ("linearCon");
      break;
    case CUDD_REORDER_EXACT:
      return ("exact");
      break;
    default:
      Pdtutil_Warning (1, "Choice Not Allowed.");
      return ("same");
      break;
  }
}

/**Function********************************************************************

  Synopsis    [Given a string it Returns an Enumerated type]

  Description [It receives a string; to facilitate the user that string
    can be an easy-to-remember predefined code or an integer number
    (interpreted as a string).
    It returns the dense method type.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Ddi_DenseMethod_e
Ddi_DenseMethodString2Enum (
  char *string     /* String to Analyze */
  )
{
  if (strcmp (string, "none")==0 || strcmp (string, "0")==0) {
    return (Ddi_DenseNone_c);
  }

  if (strcmp (string, "SubHeavyBranch")==0 || strcmp (string, "1")==0) {
    return (Ddi_DenseSubHeavyBranch_c);
  }

  if (strcmp (string, "SupHeavyBranch")==0 || strcmp (string, "2")==0) {
    return (Ddi_DenseSupHeavyBranch_c);
  }

  if (strcmp (string, "SubShortPath")==0 || strcmp (string, "3")==0) {
    return (Ddi_DenseSubShortPath_c);
  }

  if (strcmp (string, "SupShortPath")==0 || strcmp (string, "4")==0) {
    return (Ddi_DenseSupShortPath);
  }

  if (strcmp (string, "UnderApprox")==0 || strcmp (string, "5")==0) {
    return (Ddi_DenseUnderApprox_c);
  }

  if (strcmp (string, "OverApprox")==0 || strcmp (string, "6")==0) {
    return (Ddi_DenseOverApprox_c);
  }

  if (strcmp (string, "RemapUnder")==0 || strcmp (string, "7")==0) {
    return (Ddi_DenseRemapUnder_c);
  }

  if (strcmp (string, "RemapOver")==0 || strcmp (string, "8")==0) {
    return (Ddi_DenseRemapOver_c);
  }

  if (strcmp (string, "SubCompress")==0 || strcmp (string, "9")==0) {
    return (Ddi_DenseSubCompress_c);
  }

  if (strcmp (string, "SupCompress")==0 || strcmp (string, "10")==0) {
    return (Ddi_DenseSupCompress_c);
  }

  Pdtutil_Warning (1, "Choice Not Allowed For Density Method.");
  return (Ddi_DenseNone_c);
}

/**Function********************************************************************

  Synopsis    [Given an Enumerated type Returns a string]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

char *
Ddi_DenseMethodEnum2String (
  Ddi_DenseMethod_e enumType
  )
{
  switch (enumType) {
    case Ddi_DenseNone_c:
      return ("none");
      break;
    case Ddi_DenseSubHeavyBranch_c:
      return ("SubHeavyBranch");
      break;
    case Ddi_DenseSupHeavyBranch_c:
      return ("SupHeavyBranch");
      break;
    case Ddi_DenseSubShortPath_c:
      return ("SubShortPath");
      break;
    case Ddi_DenseSupShortPath:
      return ("SupShortPath");
      break;
    case Ddi_DenseUnderApprox_c:
      return ("UnderApprox");
      break;
    case Ddi_DenseOverApprox_c:
      return ("OverApprox");
      break;
    case Ddi_DenseRemapUnder_c:
      return ("RemapUnder");
      break;
    case Ddi_DenseRemapOver_c:
      return ("RemapOver");
      break;
    case Ddi_DenseSubCompress_c:
      return ("SubCompress");
      break;
    case Ddi_DenseSupCompress_c:
      return ("SupCompress");
      break;
    default:
      Pdtutil_Warning (1, "Choice Not Allowed.");
      return ("none");
      break;
  }
}



/**Function********************************************************************

  Synopsis    [Returns the version of CUDD package]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Ddi_PrintCuddVersion (
  FILE *fp
  )
{
   fprintf(fp, "Cudd-");

   Cudd_PrintVersion(fp);

   return;
}

/**Function********************************************************************

  Synopsis    []

  Description [Use a negative value to indicate no bound in the number of
    printed cubes.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Ddi_BddPrintSupportAndCubes (
  Ddi_Bdd_t *f        /* BDD */,
  int numberPerRow   /* Number of Names Printed on a Single Row */,
  int cubeNumberMax  /* Maximum number of cubes printed */,
  int formatPla      /* Prints a 1 at the end of the cube (PLA format) */,
  char *filename     /* File Name */,
  FILE *fp           /* Pointer to the store file */
  )
{
  Ddi_Varset_t *supp;
  int flag;

  if (Ddi_BddIsZero (f)) {
    Pdtutil_Warning (1, "BDD is ZERO.");
    flag = 0;
  } else {
    /* Print support variables */
    supp = Ddi_BddSupp (f);
    Ddi_VarsetPrint (supp, numberPerRow, filename, fp);

    flag = Ddi_BddPrintCubes (f, supp, cubeNumberMax, formatPla, filename, fp);

    Ddi_Free(supp);
  }

  return (flag);
}

/**Function********************************************************************

  Synopsis    []

  Description [Use a negative value to indicate no bound in the number of
    printed cubes.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Ddi_BddarrayPrintSupportAndCubes (
  Ddi_Bddarray_t *fArray  /* BDD Array */,
  int numberPerRow       /* Number of Names Printed on a Single Row */,
  int cubeNumberMax      /* Maximum number of cubes printed */,
  int formatPla          /* Prints a 1 at the end of the cube (PLA format) */,
  int reverse            /* Reverse Order if 1 */,
  char *filename         /* File Name */,
  FILE *fp               /* Pointer to the store file */
  )
{
  Ddi_Varset_t *supp;
  Ddi_Bdd_t *f;
  int i, n, nv, flag;

  flag = 0;
  if (Ddi_BddarrayNum (fArray)==0) {
    Pdtutil_Warning (1, "BDD array is void.");
  } else {
    /* print support variables */
    supp = Ddi_BddarraySupp (fArray);
    nv = Ddi_VarsetNum(supp);
    n = Ddi_BddarrayNum (fArray);

    fprintf(fp, "%d %d\n", nv, n);
    Ddi_VarsetPrint (supp, numberPerRow, filename, fp);

    for (i=0; i<n; i++) {
      if (reverse) {
        f = Ddi_BddarrayRead (fArray, n-1-i);
      } else {
        f = Ddi_BddarrayRead (fArray, i);
      }

      fprintf(fp, "%d ", i);

      flag = Ddi_BddPrintCubes (f, supp, cubeNumberMax, formatPla,
        filename, fp);

      if (flag == 0) {
        break;
      }
    }

  Ddi_Free(supp);
  }

  return (flag);
}












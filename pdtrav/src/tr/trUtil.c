/**CFile***********************************************************************

  FileName    [trUtil.c]

  PackageName [tr]

  Synopsis    [Utility Functions]

  Description []

  SeeAlso     []  

  Author      [Gianpiero Cabodi and Stefano Quer]

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

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include "trInt.h"

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

  Synopsis    [Given a string it Returns an Enumerated type]

  Description [It receives a string; to facilitate the user that string
    can be an easy-to-remember predefined code or an integer number
    (interpreted as a string).
    It returns the verbosity enumerated type.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Tr_Sort_e
Tr_TrSortString2Enum(
  char *string                  /* String to Analyze */
)
{
  if (strcmp(string, "none") == 0 || strcmp(string, "0") == 0) {
    return (Tr_SortNone_c);
  }

  if (strcmp(string, "default") == 0 || strcmp(string, "1") == 0) {
    return (Tr_SortDefault_c);
  }

  if (strcmp(string, "minMax") == 0 || strcmp(string, "2") == 0) {
    return (Tr_SortMinMax_c);
  }

  if (strcmp(string, "ord") == 0 || strcmp(string, "3") == 0) {
    return (Tr_SortOrd_c);
  }

  if (strcmp(string, "size") == 0 || strcmp(string, "4") == 0) {
    return (Tr_SortSize_c);
  }

  if (strcmp(string, "weight") == 0 || strcmp(string, "5") == 0) {
    return (Tr_SortWeight_c);
  }

  if (strcmp(string, "weightDac99") == 0 || strcmp(string, "6") == 0) {
    return (Tr_SortWeightDac99_c);
  }

  Pdtutil_Warning(1, "Choice Not Allowed For Verbosity.");
  return (Tr_SortDefault_c);
}

/**Function********************************************************************

 Synopsis    [Given an Enumerated type Returns a string]

 Description []

 SideEffects [none]

 SeeAlso     []

******************************************************************************/

char *
Tr_TrSortEnum2String(
  Tr_Sort_e enumType
)
{
  switch (enumType) {
    case Tr_SortNone_c:
      return ("none");
      break;
    case Tr_SortDefault_c:
      return ("default");
      break;
    case Tr_SortMinMax_c:
      return ("minMax");
      break;
    case Tr_SortOrd_c:
      return ("ord");
      break;
    case Tr_SortSize_c:
      return ("size");
      break;
    case Tr_SortWeight_c:
      return ("weight");
      break;
    case Tr_SortWeightDac99_c:
      return ("weightDac99");
      break;
    default:
      Pdtutil_Warning(1, "Choice Not Allowed.");
      return ("default");
      break;
  }
}

/**Function********************************************************************

  Synopsis    [Given a string it Returns an Enumerated type]

  Description [It receives a string; to facilitate the user that string
    can be an easy-to-remember predefined code or an integer number
    (interpreted as a string).
    It returns the enumerated type.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Tr_ImgMethod_e
Tr_ImgMethodString2Enum(
  char *string                  /* String to Analyze */
)
{
  if (strcmp(string, "monolithic") == 0 || strcmp(string, "0") == 0) {
    return (Tr_ImgMethodMonolithic_c);
  }

  if (strcmp(string, "iwls95") == 0 || strcmp(string, "1") == 0) {
    return (Tr_ImgMethodIwls95_c);
  }

  if (strcmp(string, "approx") == 0 || strcmp(string, "2") == 0) {
    return (Tr_ImgMethodApprox_c);
  }

  if (strcmp(string, "cofactor") == 0 || strcmp(string, "3") == 0) {
    return (Tr_ImgMethodCofactor_c);
  }
  Pdtutil_Warning(1, "Choice Not Allowed For Img method.");
  return (Tr_ImgMethodIwls95_c);
}

/**Function********************************************************************

 Synopsis    [Given an Enumerated type Returns a string]

 Description []

 SideEffects [none]

 SeeAlso     []

******************************************************************************/

char *
Tr_ImgMethodEnum2String(
  Tr_ImgMethod_e enumType
)
{
  switch (enumType) {
    case Tr_ImgMethodMonolithic_c:
      return ("monolithic");
      break;
    case Tr_ImgMethodIwls95_c:
      return ("iwls95");
      break;
    case Tr_ImgMethodApprox_c:
      return ("approx");
      break;
    case Tr_ImgMethodCofactor_c:
      return ("cofactor");
      break;
    default:
      Pdtutil_Warning(1,
        "Choice Not Allowed (monolithic, iwls95, approx, cofactor allowed).");
      return ("iwls95");
      break;
  }
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

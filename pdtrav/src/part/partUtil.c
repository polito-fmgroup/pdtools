/**CFile***********************************************************************

  FileName    [partUtil.c]

  PackageName [part]

  Synopsis    [Utility Functions for the Partitioning Package]

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
                                         
#include "partInt.h"                                          

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

  SeeAlso     [Part_MethodEnum2String]

******************************************************************************/

Part_Method_e
Part_MethodString2Enum (
  char *string     /* String to Analyze */
  )
{
  if (strcmp (string, "none")==0 || strcmp (string, "0")==0) {
     return (Part_MethodNone_c);
  }

  if (strcmp (string, "manual")==0 || strcmp (string, "1")==0) {
     return (Part_MethodManual_c);
  }

  if (strcmp (string, "cofactor")==0 || strcmp (string, "2")==0) {
     return (Part_MethodCofactor_c);
  }

  if (strcmp (string, "support")==0 || strcmp (string, "3")==0) {
     return (Part_MethodCofactor_c);
  }

  if (strcmp (string, "estimate")==0 || strcmp (string, "4")==0) {
     return (Part_MethodEstimate_c);
  }

  if (strcmp (string, "complex")==0 || strcmp (string, "5")==0) {
     return (Part_MethodEstimateComplex_c);
  }

  if (strcmp (string, "fast")==0 || strcmp (string, "6")==0) {
     return (Part_MethodEstimateFast_c);
  }

  if (strcmp (string, "free")==0 || strcmp (string, "7")==0) {
     return (Part_MethodEstimateFreeOrder_c);
  }

  if (strcmp (string, "comparison")==0 || strcmp (string, "8")==0) {
     return (Part_MethodComparison_c);
  }

  if (strcmp (string, "cuddAppCon")==0 || strcmp (string, "9")==0) {
     return (Part_MethodAppCon_c);
  }

  if (strcmp (string, "cuddAppDis")==0 || strcmp (string, "10")==0) {
     return (Part_MethodAppDis_c);
  }

  if (strcmp (string, "cuddGenCon")==0 || strcmp (string, "11")==0) {
     return (Part_MethodGenCon_c);
  }

  if (strcmp (string, "cuddGenDis")==0 || strcmp (string, "11")==0) {
     return (Part_MethodGenDis_c);
  }

  if (strcmp (string, "cuddIteCon")==0 || strcmp (string, "13")==0) {
     return (Part_MethodIteCon_c);
  }

  if (strcmp (string, "cuddIteDis")==0 || strcmp (string, "14")==0) {
     return (Part_MethodIteDis_c);
  }

  if (strcmp (string, "cuddVarCon")==0 || strcmp (string, "15")==0) {
     return (Part_MethodVarCon_c);
  }

  if (strcmp (string, "cuddVarDis")==0 || strcmp (string, "16")==0) {
     return (Part_MethodVarDis_c);
  }

  Pdtutil_Warning (1, "Choice Not Allowed For Paritition Method.");

  return (Part_MethodNone_c);
}

/**Function********************************************************************

  Synopsis           [Given an Enumerated type Returns a string.]

  Description        [Given an Enumerated type Returns a string.]

  SideEffects        [none]

  SeeAlso            [Part_MethodString2Enum]

******************************************************************************/

char *
Part_MethodEnum2String (
  Part_Method_e enumType
  )
{
  switch (enumType) {
    case Part_MethodNone_c:
      return ("none");
      break;
    case Part_MethodManual_c:
      return ("manual");
      break;
    case Part_MethodCofactor_c:
      return ("cofactor");
      break;
    case Part_MethodCofactorSupport_c:
      return ("support");
      break;
    case Part_MethodEstimate_c:
      return ("estimate");
      break;
    case Part_MethodEstimateComplex_c:
      return ("complex");
      break;
    case Part_MethodEstimateFast_c:
      return ("fast");
      break;
    case Part_MethodEstimateFreeOrder_c:
      return ("free");
      break;
    case Part_MethodComparison_c:
      return ("comparison");
      break;
    case Part_MethodAppCon_c:
      return ("cuddAppCon");
      break;
    case Part_MethodAppDis_c:
      return ("cuddAppDis");
      break;
    case Part_MethodGenCon_c:
      return ("cuddGenCon");
      break;
    case Part_MethodGenDis_c:
      return ("cuddGenDis");
      break;
    case Part_MethodIteCon_c:
      return ("cuddIteCon");
      break;
    case Part_MethodIteDis_c:
      return ("cuddIteDis");
      break;
    case Part_MethodVarCon_c:
      return ("cuddVarCon");
      break;
    case Part_MethodVarDis_c:
      return ("cuddVarDis");
      break;
    default:
      Pdtutil_Warning (1, "Choice Not Allowed.");
      return ("none");
      break;
  }
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

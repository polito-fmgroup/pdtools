/**CFile***********************************************************************

  FileName    [travUtil.c]

  PackageName [trav]

  Synopsis    [Utility Functions for the Traverse Package]

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

#include "travInt.h"

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
    It returns the enumerated type.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Trav_FromSelect_e
Trav_FromSelectString2Enum(
  char *string                  /* String to Analyze */
)
{
  if (strcmp(string, "new") == 0 || strcmp(string, "0") == 0) {
    return (Trav_FromSelectNew_c);
  }

  if (strcmp(string, "reached") == 0 || strcmp(string, "1") == 0) {
    return (Trav_FromSelectReached_c);
  }

  if (strcmp(string, "to") == 0 || strcmp(string, "2") == 0) {
    return (Trav_FromSelectTo_c);
  }

  if (strcmp(string, "cofactor") == 0 || strcmp(string, "3") == 0) {
    return (Trav_FromSelectCofactor_c);
  }

  if (strcmp(string, "restrict") == 0 || strcmp(string, "4") == 0) {
    return (Trav_FromSelectRestrict_c);
  }

  if (strcmp(string, "best") == 0 || strcmp(string, "5") == 0) {
    return (Trav_FromSelectBest_c);
  }

  if (strcmp(string, "same") == 0 || strcmp(string, "6") == 0) {
    return (Trav_FromSelectSame_c);
  }

  Pdtutil_Warning(1, "Choice Not Allowed For From Selection.");
  return (Trav_FromSelectBest_c);
}

/**Function********************************************************************

 Synopsis    [Given an Enumerated type Returns a string]

 Description []

 SideEffects [none]

 SeeAlso     []

******************************************************************************/

char *
Trav_FromSelectEnum2String(
  Trav_FromSelect_e enumType
)
{
  switch (enumType) {
    case Trav_FromSelectNew_c:
      return ("new");
      break;
    case Trav_FromSelectReached_c:
      return ("reached");
      break;
    case Trav_FromSelectTo_c:
      return ("to");
      break;
    case Trav_FromSelectCofactor_c:
      return ("cofactor");
      break;
    case Trav_FromSelectRestrict_c:
      return ("restrict");
      break;
    case Trav_FromSelectBest_c:
      return ("best");
      break;
    case Trav_FromSelectSame_c:
      return ("same");
      break;
    default:
      Pdtutil_Warning(1, "Choice Not Allowed.");
      return ("none");
      break;
  }
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Compute from given to and reached]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
TravFromCompute(
  Ddi_Bdd_t * to /* result of image computation */ ,
  Ddi_Bdd_t * reached /* old reached */ ,
  int option                    /* selection option */
)
{
  Ddi_Bdd_t *from, *best, *tmp;
  int size, bestsize;

  best = NULL;
  bestsize = 0;

  switch (option) {
    case Trav_FromSelectBest_c:
      /* compute all following cases */
    case Trav_FromSelectNew_c:
      /* compute from as newly reached states */
      from = Ddi_BddAnd(to, (tmp = Ddi_BddNot(reached)));
      Ddi_Free(tmp);
      if (option != Trav_FromSelectBest_c) {
        return (from);
      }
      best = from;
      bestsize = Ddi_BddSize(best);
    case Trav_FromSelectReached_c:
      from = Ddi_BddOr(to, reached);
      if (option != Trav_FromSelectBest_c)
        return (from);
      size = Ddi_BddSize(from);
      if (size < bestsize) {
        Ddi_Free(best);
        best = from;
        bestsize = size;
      } else
        Ddi_Free(from);
    case Trav_FromSelectTo_c:
      from = Ddi_BddDup(to);
      if (option != Trav_FromSelectBest_c)
        return (from);
      size = Ddi_BddSize(from);
      if (size < bestsize) {
        Ddi_Free(best);
        best = from;
        bestsize = size;
      } else
        Ddi_Free(from);
    case Trav_FromSelectCofactor_c:
      from = Ddi_BddConstrain(to, (tmp = Ddi_BddNot(reached)));
      Ddi_Free(tmp);
      if (option != Trav_FromSelectBest_c)
        return (from);
      size = Ddi_BddSize(from);
      if (size < bestsize) {
        Ddi_Free(best);
        best = from;
        bestsize = size;
      } else
        Ddi_Free(from);
    case Trav_FromSelectRestrict_c:
      from = Ddi_BddRestrict(to, (tmp = Ddi_BddNot(reached)));
      Ddi_Free(tmp);
      if (option != Trav_FromSelectBest_c)
        return (from);
      size = Ddi_BddSize(from);
      if (size < bestsize) {
        Ddi_Free(best);
        best = from;
        bestsize = size;
      } else
        Ddi_Free(from);
  }

  return (best);
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

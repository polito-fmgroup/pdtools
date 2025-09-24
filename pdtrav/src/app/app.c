/**CFile***********************************************************************

  FileName    [app.c]

  PackageName [tr]

  Synopsis    [Functions to handle pdtrav apps as single programs]

  Description []

  SeeAlso     []

  Author      [Gianpiero Cabodi]

  Copyright [  Copyright (c) 2001 by Politecnico di Torino.
    All Rights Reserved. This software is for educational purposes only.
    Permission is given to academic institutions to use, copy, and modify
    this software and its documentation provided that this introductory
    message is not removed, that this software and its documentation is
    used for the institutions' internal research and educational purposes,
    and that no monies are exchanged. No guarantee is expressed or implied
    by the distribution of this code.
    Send bug-reports and/or questions to: gianpiero.cabodi@polito.it. ]

  Revision    []

******************************************************************************/

#include "appInt.h"
#include "pdtutil.h"

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

static struct {
  char *appName;
  AppMainFunc_t appFunc;
} appTable[] =
  {
   {"aiger", App_Aiger}, 
   {"certify", App_Certify},
   {"gfp", App_Gfp}, 
   {"iso", App_Iso}, 
   {NULL,NULL}
  };

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
  Synopsis    [Main program of pdtrav apps.]
  Description [Main program of pdtrav apps.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
main(
  int argc,
  char *argv[]
)
{
  int res = EXIT_SUCCESS;
  if (argc<2) {
    printf("missing app name\n");
  }
  else {
    for (int i=0; appTable[i].appName!=NULL; i++) {
      if (strcmp(argv[1],appTable[i].appName)==0) {
        res = !appTable[i].appFunc(argc-2,argv+2);
        return res;
      }
    }
    printf("wrong app name\n");
  }

  printf("Choose among:");
  for (int i=0; appTable[i].appName!=NULL; i++) {
    printf(" %s", appTable[i].appName);
  }
  printf("\n");
  return EXIT_FAILURE;
}



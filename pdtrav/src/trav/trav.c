/**CFile***********************************************************************

  FileName    [trav.c]

  PackageName [trav]

  Synopsis    [Main module for a simple traversal of finite state machine]

  Description [External procedure included in this file is:
    <ul>
    <li>TravReadOptions()
    </ul>
    ]

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

#define MAX_ROW_LEN 120
#define MAX_OPT_LEN 15

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

  Synopsis    [Temporary main program to be used as interface for cmd.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

void
Trav_Main(
  Trav_Mgr_t * travMgr          /* Traversal manager */
)
{
  Ddi_Bdd_t *reached;
  double nstates;
  Ddi_Mgr_t *dd;
  long travTime;

  dd = travMgr->ddiMgrTr;

  /*-------------------------------- Traversal ------------------------------*/

  travTime = util_cpu_time();

  reached = Trav_Traversal(travMgr);

  travTime = util_cpu_time() - travTime;
  travMgr->travTime += travTime;

  if (reached == NULL) {
    Pdtutil_Warning(1, "Traversal is failed.");
    return;
  }

  /*------------------------- Print Final Statistics -----------------------*/

  Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
    Pdtutil_ChrPrint(stdout, '*', 50);
    fprintf(stdout, "\n");
    fprintf(stdout, "Reachability Analysis Results Summary:\n");
    fprintf(stdout, "  Traversal depth: %d.\n",
      Trav_MgrReadLevel(travMgr) - 1);
    fprintf(stdout, "#Reached size: %d.\n", Ddi_BddSize(reached))
    );

#if 0
  /* StQ PATCH */
  {
    int i;
    Ddi_Bdd_t *reachedN;
    Ddi_Bddarray_t *tmpArray;
    char **inputNames, **outputNames, *outputOneName;

    outputNames = Pdtutil_Alloc(char *,
      2
    );
    outputNames[0] = Pdtutil_Alloc(char,
      20
    );
    outputNames[1] = Pdtutil_Alloc(char,
      20
    );

    strcpy(outputNames[0], "outArray1");
    strcpy(outputNames[1], "outArray2");

    outputOneName = Pdtutil_Alloc(char,
      20
    );

    strcpy(outputOneName, "outSingleBdd");

    reachedN = Ddi_BddNot(reached);
    tmpArray = Ddi_BddarrayAlloc(dd, 0);
    Ddi_BddarrayInsertLast(tmpArray, reached);
    Ddi_BddarrayInsertLast(tmpArray, reachedN);

    Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "Patch: Write Blif.\n")
      );
    inputNames = Ddi_MgrReadVarnames(dd);

      /*-------------------------- Store Blif -------------------------------*/

    i = Ddi_BddStoreBlif(reached,
#if 0
      NULL, NULL,
#else
      inputNames, outputOneName,
#endif
      "mioModelloExlif", "bddStore.exlif", NULL);

    i = Ddi_BddarrayStoreBlif(tmpArray,
#if 0
      NULL, NULL,
#else
      inputNames, outputNames,
#endif
      "mioModelloExlifForest", "bddArrayStore.exlif", NULL);

      /*-------------------------- Store Prefix------------------------------*/

    i = Ddi_BddStorePrefix(reached,
#if 0
      NULL, NULL,
#else
      inputNames, outputOneName,
#endif
      "mioModelloGmf", "bddStore.gmf", NULL);

    i = Ddi_BddarrayStorePrefix(tmpArray,
#if 0
      NULL, NULL,
#else
      inputNames, outputNames,
#endif
      "mioModelloGmfForest", "bddArrayStore.gmf", NULL);

      /*---------------------------- Store Smv ------------------------------*/

    i = Ddi_BddStoreSmv(reached,
#if 0
      NULL, NULL,
#else
      inputNames, outputOneName,
#endif
      "mioModelloSmv", "bddStore.smv", NULL);

    i = Ddi_BddarrayStoreSmv(tmpArray,
#if 0
      NULL, NULL,
#else
      inputNames, outputNames,
#endif
      "mioModelloSmvForest", "bddArrayStore.smv", NULL);

    exit(1);
  }
  /* StQ FINE PATCH */
#endif

  Pdtutil_VerbosityMgr(travMgr, Pdtutil_VerbLevelDevMed_c,
#ifndef __alpha__
    nstates = Ddi_BddCountMinterm(reached,
      Ddi_VararrayNum(Trav_MgrReadPS(travMgr)));
    fprintf(stdout, "#Reached States: %g.\n", nstates);
#endif
    fprintf(stdout, "Maximum Internal Product: %d.\n",
      Trav_MgrReadProductPeak(travMgr));
    fprintf(stdout, "CPU Time: %s.\n", util_print_time(travTime));
    Pdtutil_ChrPrint(stdout, '*', 50); fprintf(stdout, "\n")
    );

  /*--------------------- Partitioning Reached ----------------------------*/

#if 0
  if (Trav_MgrReadVerbosity(travMgr) > 2) {
    Part_BddDisjSuppPart(reached, Trav_MgrReadTr(travMgr),
      Trav_MgrReadPS(travMgr), Trav_MgrReadNS(travMgr),
      Trav_MgrReadVerbosity(travMgr));
  }
#endif

  /*------------------------------ Exit -----------------------------------*/
}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

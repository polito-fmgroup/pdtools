/**CFile***********************************************************************

  FileName    [partCudd.c]

  PackageName [part]

  Synopsis    [Functions for Disjunctive and Conjunctive Partitioning
    as implemented in the Cudd package.]

  Description [This file contains interface function to linked with the
    standard cudd partitionin routines.]

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

static void partPartitionCudd(Ddi_Bdd_t *function, Part_Method_e partitionMethod, int threshold, Ddi_Bddarray_t *partition, Pdtutil_VerbLevel_e verbosity);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Partition a state set]

  Description [This function derives from the Part_PartitionDisjSet
    function. It is rewritten here for two reasons:
    <ul>
    <li> this file includes the direct calls to the routine implemented
    in the Cudd package,
    <li> this file includes both disjunctive and conjuctive decompositions.
    </ul>
    ]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Part_PartitionSetCudd (
  Ddi_Bdd_t *f                      /* Input BDD */,
  Part_Method_e partitionMethod    /* Partition Method */,
  int threshold                    /* Threshold Size */,
  Pdtutil_VerbLevel_e verbosity    /* Verbosity Level */
  )
{
  int i, bddSize=0;
  long startTime=0, currTime;
  Ddi_Bddarray_t *ddVet;
  Ddi_Bdd_t *fdup;
  Ddi_Bdd_t *result, *dpart;
  Ddi_Mgr_t *dd = Ddi_ReadMgr(f);

  ddVet = Ddi_BddarrayAlloc (dd,0);
  
  /* f is freed by partPartitionDisjSet */ 
  fdup = Ddi_BddDup (f);
  
  if (verbosity >= Pdtutil_VerbLevelUsrMax_c) {
    startTime = util_cpu_time();
    bddSize = Ddi_BddSize (fdup);
  }

  /*----------------------------- Partitioning ------------------------------*/

  switch (Ddi_ReadCode(fdup)) {
    case Ddi_Bdd_Mono_c:
      partPartitionCudd (fdup, partitionMethod, threshold, ddVet, verbosity);
      break;
    case Ddi_Bdd_Part_Conj_c:
      Pdtutil_Warning (Ddi_BddPartNum(f)>1, "One partition expected");
      partPartitionCudd (Ddi_BddPartRead (fdup,0), 
        partitionMethod, threshold, ddVet, verbosity);
      break;
    default:
      Pdtutil_Assert(0,"Wrong DDI node code");
      break;
  }
  
  Ddi_Free(fdup);

  /*--------------------- From BDD-Array to BDD-Disjunctive -----------------*/

  if (verbosity >= Pdtutil_VerbLevelUsrMax_c) {
    fprintf (stdout, "PARTITIONING (%d) ==> [", bddSize);
  }

  result = Ddi_BddMakePartDisjVoid(dd);

  for (i=0; i<Ddi_BddarrayNum (ddVet); i++) {
    dpart = Ddi_BddarrayRead (ddVet, i);
    bddSize = Ddi_BddSize (dpart);

    if (verbosity >= Pdtutil_VerbLevelUsrMax_c) {
      fprintf (stdout, "%d+", bddSize);
      fflush(stdout);
    }

    /* Add partition to disjoined result */
    Ddi_BddPartInsertLast(result,dpart);
  }

  /*------------------------------ Final Statistics -------------------------*/

  if (verbosity >= Pdtutil_VerbLevelUsrMax_c) {
    bddSize = Ddi_BddarrayNum (ddVet);
    currTime = util_cpu_time();

    fprintf (stdout, "=%d]", bddSize);
    fprintf (stdout, "(time:%s)\n", util_print_time (currTime - startTime));
    fflush(stdout);
  }

  Ddi_Free (ddVet);
  
  return (result);
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Direct call to cudd partitioning functions.]

  Description [Direct call to cudd partitioning functions. It transforms
    ddi objects to cudd ones before the call and reverse the process after
    it.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

static void
partPartitionCudd (
  Ddi_Bdd_t *function              /* Function to Partition */,
  Part_Method_e partitionMethod    /* Partition Method */,
  int threshold                    /* Threshold Size */,
  Ddi_Bddarray_t *partition        /* Partition Array */,
  Pdtutil_VerbLevel_e verbosity    /* Verbosity Level */
)
{
  Ddi_Bdd_t *thenFunction, *elseFunction;
  Ddi_Mgr_t *dd;
  DdManager *ddCudd;
  DdNode *functionCudd;
  DdNode **partitionCudd;
  long startTime, currTime;
  int bddSize, thenSize, elseSize, numPart=0;

  bddSize = Ddi_BddSize (function);
  startTime = util_cpu_time();

  /*--------------------- Check NULL method and BDD size --------------------*/

  if (bddSize<threshold) {
    Ddi_BddarrayWrite (partition, Ddi_BddarrayNum (partition), function);
    return;
  }

  /*--------------------------------- Split ---------------------------------*/

  /*
   *  From ddi to Cudd
   */

  functionCudd = Ddi_BddToCU (function);
  dd = Ddi_ReadMgr (function);
  ddCudd = Ddi_MgrReadMgrCU (dd);   

  switch (partitionMethod) {
    case Part_MethodAppCon_c:
      numPart = Cudd_bddApproxConjDecomp (ddCudd, functionCudd,
        &partitionCudd);
      break;
    case Part_MethodAppDis_c:
      numPart = Cudd_bddApproxDisjDecomp (ddCudd, functionCudd,
        &partitionCudd);
      break;
    case Part_MethodGenCon_c:
      numPart = Cudd_bddGenConjDecomp (ddCudd, functionCudd, &partitionCudd);
      break;
    case Part_MethodGenDis_c:
      numPart = Cudd_bddGenDisjDecomp (ddCudd, functionCudd, &partitionCudd);
      break;
    case Part_MethodIteCon_c:
      numPart = Cudd_bddIterConjDecomp (ddCudd, functionCudd, &partitionCudd);
      break;
    case Part_MethodIteDis_c:
      numPart = Cudd_bddIterDisjDecomp (ddCudd, functionCudd, &partitionCudd);
      break;
    case Part_MethodVarCon_c:
      numPart = Cudd_bddVarConjDecomp (ddCudd, functionCudd, &partitionCudd);
      break;
    case Part_MethodVarDis_c:
      numPart = Cudd_bddVarDisjDecomp (ddCudd, functionCudd, &partitionCudd);
      break;
    default:
      break;
  }
 
  if (numPart<2) {
    Ddi_BddarrayWrite (partition, Ddi_BddarrayNum (partition),
      function);

    return;
  }

  /*
   *  From Cudd to ddi
   */

  thenFunction = Ddi_BddMakeFromCU (dd, partitionCudd[0]);
  elseFunction = Ddi_BddMakeFromCU (dd, partitionCudd[1]);

  /*------------------------------ Statistics -------------------------------*/

  thenSize = Ddi_BddSize (thenFunction);
  elseSize = Ddi_BddSize (elseFunction);

  currTime = util_cpu_time();

  if (verbosity >= Pdtutil_VerbLevelNone_c) {
    fprintf (stdout, "[%d--->%d,%d(time:%s)]\n",
      bddSize, thenSize, elseSize,
      util_print_time(currTime - startTime));
  }

  /*--------------------------------- Recur ---------------------------------*/

  partPartitionCudd (thenFunction, partitionMethod, threshold,
    partition, verbosity);
  Ddi_Free (thenFunction);
  partPartitionCudd (elseFunction, partitionMethod, threshold,
    partition, verbosity);
  Ddi_Free (elseFunction);

  return;
}




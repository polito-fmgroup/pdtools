/**CHeaderFile*****************************************************************

  FileName    [part.h]

  PackageName [part]

  Synopsis    [Partitioning Module]

  Description [This <b>part</b> package provides functions to partition BDDs.
    in a discjunctive or conjunctive form. "Internal" partitioning routines
    are provided (see Cabodi et. al ICCAD'96, and Cabodi et. al TCAD - to
    Appear). Moreover a link to Cudd partitioning routines is provided
    too.<br>
    The following method are actually provided:
    <ul>
    <li>PdTrav Methods
      <ul>
      <li>Part_MethodManual_c, partitioning variable choose manually
      <li>Part_MethodCofactor_c, use standard cofactor to evaluate the
      best splitting variable
      <li>Part_MethodCofactorKernel_c, use standard cofactor to evaluate the
      best (for unbalanced cofactors) splitting variable 
      <li>Part_MethodEstimate_c, use original ICCAD'96 method to estimate
      the codactors of a function
      <li>Part_MethodEstimateComplex_c, modification to the previous one
      (Somenzi, Private Communication, 1996)
      <li>Part_MethodEstimateFast_c, fast "linear" heuristic TCAD'99
      <li>Part_MethodEstimateFreeOrder_c, heuristic to move from BDD
      to FBDD (TCAD'99)
      <li>Part_MethodComparison_c, comparison routine (debugging purpose
      mainly).
      </ul>
    To notice that up to now (June 01, 1999) not all the funcions
    are tested, as they have been taken, sometimes too verbatim, from
    old code versions.
    <li>Cudd Based Methods
      <ul>
      <li>Part_MethodAppCon_c, routine Cudd_bddApproxConjDecomp
      <li>Part_MethodAppDis_c, routine Cudd_bddApproxDisjDecomp
      <li>Part_MethodGenCon_c, routine Cudd_bddGenConjDecomp
      <li>Part_MethodGenDis_c, routine Cudd_bddGenDisjDecomp
      <li>Part_MethodIteCon_c, routine Cudd_bddIterConjDecomp
      <li>Part_MethodIteDis_c, routine Cudd_bddIterDisjDecomp
      <li>Part_MethodVarCon_c, routine Cudd_bddVarConjDecomp
      <li>Part_MethodVarDis_c, routine Cudd_bddVarDisjDecomp
      </ul>
   see Cudd for further details.
   </ul>
   These methods are called with the command option:<br>
   none, manual, cofactor, estimate, complex, fast, free,
   comparison, cuddAppCon, cuddAppDis, cuddGenCon, cuddGenDis,
   cuddIteCon, cuddIteDis, cuddVarCon, cuddVarDis.<br>
   Notice that this module contains both a direct link both to the ddi
   and to the cudd package.<br>
   This structure should be revised. 
  ]

  SeeAlso   []  

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
  
  Revision  []

******************************************************************************/

#ifndef _PART
#define _PART

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/**Enum************************************************************************

  Synopsis    [Type for Partitioning Method.]

  Description []

******************************************************************************/

typedef enum
  {
  /*
   *  PdTrav Based Methods
   */
  Part_MethodNone_c,
  Part_MethodManual_c,
  Part_MethodCofactor_c,
  Part_MethodCofactorKernel_c,
  Part_MethodCofactorSupport_c,
  Part_MethodEstimate_c,
  Part_MethodEstimateComplex_c,
  Part_MethodEstimateFast_c,
  Part_MethodEstimateFreeOrder_c,
  Part_MethodComparison_c,
  /*
   *  Cudd Based Methods
   */
  Part_MethodAppCon_c,
  Part_MethodAppDis_c,
  Part_MethodGenCon_c,
  Part_MethodGenDis_c,
  Part_MethodIteCon_c,
  Part_MethodIteDis_c,
  Part_MethodVarCon_c,
  Part_MethodVarDis_c
  }
Part_Method_e;

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EXTERN Ddi_Bdd_t * Part_BddMultiwayLinearAndExist(Ddi_Bdd_t *FPart, Ddi_Varset_t *smoothV, int threshold);
EXTERN Ddi_Bdd_t * Part_BddMultiwayRecursiveAndExist(Ddi_Bdd_t *FPart, Ddi_Varset_t *smoothV, Ddi_Bdd_t *constrain, int threshold);
EXTERN Ddi_Bdd_t * Part_BddMultiwayRecursiveAndExistOver(Ddi_Bdd_t *FPart, Ddi_Varset_t *smoothV, Ddi_Bdd_t *constrain, Ddi_Bdd_t *overApprTerm, int threshold);
EXTERN Ddi_Bdd_t * Part_BddMultiwayRecursiveTreeAndExist(Ddi_Bdd_t *FPart, Ddi_Varset_t *smoothV, Ddi_Varset_t *rvars, int threshold);
EXTERN Ddi_Bdd_t * Part_BddDisjSuppPart(Ddi_Bdd_t *f, Ddi_Bdd_t *TR, Ddi_Vararray_t *psv, Ddi_Vararray_t *nsv, int verbosity);
EXTERN Ddi_Bdd_t * Part_PartDomainCC(Ddi_Bdd_t *f, Ddi_Vararray_t *piv, Ddi_Vararray_t *psv, Ddi_Vararray_t *nsv, int clustth, int maxcommon, int minshare);
EXTERN Ddi_Bdd_t * Part_PartManualGroups(Ddi_Bdd_t *f, char *fileName);
EXTERN void Part_PartPrintGroups(Ddi_Bdd_t *f, Ddi_Varset_t *rvars, char *fileName);
EXTERN Ddi_Bdd_t * Part_PartPrintGroupsStats(Ddi_Bdd_t *f, Ddi_Varset_t *rvars, char *fileName);
EXTERN int Part_EstimateCofactor(Ddi_Bdd_t *f, Ddi_Bdd_t *topVar);
EXTERN int Part_EstimateCofactorComplex(Ddi_Bdd_t *f, Ddi_Bdd_t *topVar);
EXTERN int Part_EstimateCofactorFast(Ddi_Bdd_t *f, int LeftRight, Ddi_Varset_t *supp, int suppSize, int *totalArray, int *thenArray, int *elseArray);
EXTERN int Part_EstimateCofactorFreeOrder(Ddi_Bdd_t *f, Ddi_Bdd_t *topVar, Ddi_Varset_t *supp, int *vet);
EXTERN Ddi_Bdd_t * Part_PartitionSetCudd(Ddi_Bdd_t *f, Part_Method_e partitionMethod, int threshold, Pdtutil_VerbLevel_e verbosity);
EXTERN Ddi_Bdd_t * Part_PartitionSetInterface(Ddi_Bdd_t *f, Ddi_Varset_t *careVars, Part_Method_e partitionMethod, int threshold, int maxDepth, Pdtutil_VerbLevel_e verbosity);
EXTERN Ddi_Bdd_t * Part_PartitionDisjSet(Ddi_Bdd_t *f, Ddi_Bdd_t *pivotCube, Ddi_Varset_t *careVars, Part_Method_e partitionMethod, int threshold, int maxDepth, Pdtutil_VerbLevel_e verbosity);
EXTERN Ddi_Bdd_t * Part_PartitionDisjSetWindow(Ddi_Bdd_t *f, Ddi_Bdd_t *pivotCube, Ddi_Varset_t *careVars, Part_Method_e partitionMethod, int threshold, int maxDepth, Pdtutil_VerbLevel_e verbosity);
EXTERN Part_Method_e Part_MethodString2Enum(char *string);
EXTERN char * Part_MethodEnum2String(Part_Method_e enumType);

/**AutomaticEnd***************************************************************/

#endif /* _PART */






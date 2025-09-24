/**CHeaderFile*****************************************************************

  FileName    [ddi.h]

  PackageName [ddi]

  Synopsis    [Portability interface with the Decision Diagram package.]

  Description [The <b>DDI</b> package provides functions to manipulate the 
    following objects:
    <ul>
    <li>boolean functions (Ddi_Bdd_t)
    <li>arrays of boolean functions (Ddi_Bddarray_t)
    <li>variables (Ddi_Var_t)
    <li>arrays of variables (Ddi_Vararray_t)
    <li>sets of variables (Ddi_Varset_t).
    </ul>
    It can be viewed both as a portability interface, concentrating
    all dependancies from the BDD package, and as an upper level,
    manipulating the previously listed BDD based data types.
    A particular feature, distinguishing this interface from other ones,
    is the support for partitioned (conjunctive as well as disjunctive)
    functions.
    This implementation is written on top of the CUDD package.
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

#ifndef _DDI
#define _DDI

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include "dddmp.h"
#include "pdtutil.h"
#include "cuplus.h"
#include "baig.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*
 *  Codes for generalized Boolean functions.
 */

#define DDI_MONO_BOOLF                    0
#define DDI_CPART_BOOLF                   1
#define DDI_DPART_BOOLF                   2

/*
 *  Default settings.
 */

#define DDI_NO_LIMIT                      ((1<<30)-2)
#define DDI_DFLT_ITE_ON                   TRUE
#define DDI_DFLT_ITE_RESIZE_AT            75
#define DDI_DFLT_ITE_MAX_SIZE             1000000
#define DDI_DFLT_ITE_CONST_ON             TRUE
#define DDI_DFLT_ITE_CONST_RESIZE_AT      75
#define DDI_DFLT_ITE_CONST_MAX_SIZE       1000000
#define DDI_DFLT_ADHOC_ON                 TRUE
#define DDI_DFLT_ADHOC_RESIZE_AT          0
#define DDI_DFLT_ADHOC_MAX_SIZE           10000000
#define DDI_DFLT_GARB_COLLECT_ON          TRUE
#define DDI_DFLT_DAEMON                   NIL(void)
#define DDI_DFLT_MEMORY_LIMIT             BDD_NO_LIMIT
#define DDI_DFLT_NODE_RATIO               2.0
#define DDI_DFLT_INIT_BLOCKS              10

#define DDI_UNIQUE_SLOTS CUDD_UNIQUE_SLOTS
#define DDI_CACHE_SLOTS CUDD_CACHE_SLOTS
#define DDI_MAX_CACHE_SIZE 0

/*
 *  Dynamic reordering.
 */

#define DDI_REORDER_SAME 0
#define DDI_REORDER_NONE 1
#define DDI_REORDER_RANDOM 2
#define DDI_REORDER_RANDOM_PIVOT 3
#define DDI_REORDER_SIFT 4
#define DDI_REORDER_SIFT_CONVERGE 5
#define DDI_REORDER_SYMM_SIFT 6
#define DDI_REORDER_SYMM_SIFT_CONV 7
#define DDI_REORDER_WINDOW2 8
#define DDI_REORDER_WINDOW3 9
#define DDI_REORDER_WINDOW4 10
#define DDI_REORDER_WINDOW2_CONV 11
#define DDI_REORDER_WINDOW3_CONV 12
#define DDI_REORDER_WINDOW4_CONV 13
#define DDI_REORDER_GROUP_SIFT 14
#define DDI_REORDER_GROUP_SIFT_CONV 15
#define DDI_REORDER_ANNEALING 16
#define DDI_REORDER_GENETIC 17
#define DDI_REORDER_LINEAR 18
#define DDI_REORDER_LINEAR_CONVERGE 19
#define DDI_REORDER_EXACT 20

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/**Enum************************************************************************
  Synopsis    [DDI block codes.]
  Description [DDI block coses. Used as subtypes for selections within objects
               of same type.]
******************************************************************************/

typedef enum {
  Ddi_Bdd_Mono_c,
  Ddi_Bdd_Part_Conj_c,
  Ddi_Bdd_Part_Disj_c,
  Ddi_Bdd_Meta_c,
  Ddi_Bdd_Aig_c,
  Ddi_Var_Bdd_c,
  Ddi_Var_Aig_c,
  Ddi_Varset_Bdd_c,
  Ddi_Varset_Array_c,
  Ddi_Expr_Bool_c,
  Ddi_Expr_Ctl_c,
  Ddi_Expr_String_c,
  Ddi_Expr_Bdd_c,
  Ddi_Null_code_c
}
Ddi_Code_e;

/**Enum************************************************************************
  Synopsis    [DDI block codes.]
  Description [DDI block coses. Used as subtypes for selections within objects
               of same type.]
******************************************************************************/

typedef enum {
  Ddi_Info_Var_c,
  Ddi_Info_Mark_c,
  Ddi_Info_Eq_c,
  Ddi_Info_Compose_c,
  Ddi_Null_Null_c
}
Ddi_Info_e;

/**Enum************************************************************************
  Synopsis    [Type for operation codes.]
  Description [Type for operation codes. Used for selection of DDI
               Boolean operation.]
******************************************************************************/
typedef enum {
  Ddi_BddNot_c,
  Ddi_BddSwapVars_c,
  Ddi_BddSubstVars_c,
  Ddi_BddCompose_c,
  Ddi_BddMakeMono_c,
  Ddi_BddAnd_c,
  Ddi_BddDiff_c,
  Ddi_BddNand_c,
  Ddi_BddOr_c,
  Ddi_BddNor_c,
  Ddi_BddXor_c,
  Ddi_BddXnor_c,
  Ddi_BddExist_c,
  Ddi_BddForall_c,
  Ddi_BddConstrain_c,
  Ddi_BddRestrict_c,
  Ddi_BddCproject_c,
  Ddi_BddAndExist_c,
  Ddi_BddIte_c,
  Ddi_BddSupp_c,
  Ddi_BddSuppAttach_c,
  Ddi_BddSuppDetach_c,
  Ddi_VararrayDiff_c,
  Ddi_VararrayUnion_c,
  Ddi_VararrayIntersect_c,
  Ddi_VararraySwapVars_c,
  Ddi_VararraySubstVars_c,
  Ddi_VarsetNext_c,
  Ddi_VarsetAdd_c,
  Ddi_VarsetRemove_c,
  Ddi_VarsetDiff_c,
  Ddi_VarsetUnion_c,
  Ddi_VarsetIntersect_c,
  Ddi_VarsetSwapVars_c,
  Ddi_VarsetSubstVars_c
} Ddi_OpCode_e;

/**Enum************************************************************************
  Synopsis    [Type for Dense Subsetting.]
  Description []
******************************************************************************/
typedef enum
  {
  Ddi_DenseNone_c,
  Ddi_DenseSubHeavyBranch_c,
  Ddi_DenseSupHeavyBranch_c,
  Ddi_DenseSubShortPath_c,
  Ddi_DenseSupShortPath,
  Ddi_DenseUnderApprox_c,
  Ddi_DenseOverApprox_c,
  Ddi_DenseRemapUnder_c,
  Ddi_DenseRemapOver_c,
  Ddi_DenseSubCompress_c,
  Ddi_DenseSupCompress_c
  }
Ddi_DenseMethod_e;

/**Enum************************************************************************
  Synopsis    [Selector for Meta BDD method.]
******************************************************************************/
typedef enum
{
  Ddi_Meta_Size,
  Ddi_Meta_Size_Window,
  Ddi_Meta_EarlySched,
  Ddi_Meta_McM,
  Ddi_Meta_FilterVars
}
Ddi_Meta_Method_e;

/* marco */
typedef enum {
  Ddi_MapType_None_c,
  Ddi_MapType_BddSat_c,
  Ddi_MapType_BddSmt_c,
  Ddi_MapType_AigSat_c,
  Ddi_MapType_AigSmt_c,
} Ddi_MapType_e;

typedef struct Ddi_Signature_t Ddi_Signature_t;
typedef struct Ddi_Signaturearray_t Ddi_Signaturearray_t;
typedef struct Ddi_Map_t Ddi_Map_t;
typedef struct Ddi_SccMgr_t Ddi_SccMgr_t;

/* Stucture for DRUP logging */
/*
typedef struct Ddi_ResolutionNode_t Ddi_ResolutionNode_t;

typedef enum {
  Ddi_ResolutionNodeType_None_c,
  Ddi_ResolutionNodeType_Unit_c,
  Ddi_ResolutionNodeType_Chain_c,
  Ddi_ResolutionNodeType_Original_c
} Ddi_ResolutionNodeType_e;
*/
/* marco */

typedef struct Ddi_Mgr_t Ddi_BddMgr;
typedef struct Ddi_Mgr_t Ddi_Mgr_t;

typedef struct DdNode Ddi_BddNode;
typedef struct DdNode Ddi_CuddNode;

typedef union  Ddi_Info_t Ddi_Info_t;
typedef union  Ddi_Generic_t Ddi_Generic_t;

typedef struct Ddi_GenericArray_t Ddi_GenericArray_t;

typedef struct Ddi_Bdd_t Ddi_Bdd_t;
typedef struct Ddi_Var_t Ddi_Var_t;
typedef struct Ddi_Varset_t Ddi_Varset_t;
typedef struct Ddi_Expr_t Ddi_Expr_t;

typedef struct Ddi_Bddarray_t Ddi_Bddarray_t;
typedef struct Ddi_Vararray_t Ddi_Vararray_t;
typedef struct Ddi_Varsetarray_t Ddi_Varsetarray_t;
typedef struct Ddi_Exprarray_t Ddi_Exprarray_t;

typedef Ddi_Generic_t * (*NPFN)();

/* portability to 2.0 */
typedef struct Ddi_Bdd_t Ddi_Dd_t;
typedef struct Ddi_Bddarray_t Ddi_DdArray_t;

typedef struct Ddi_Clause_t Ddi_Clause_t;
typedef struct Ddi_ClauseArray_t Ddi_ClauseArray_t;

typedef struct Ddi_PdrMgr_t Ddi_PdrMgr_t;
typedef struct Ddi_SatSolver_t Ddi_SatSolver_t;
typedef struct Ddi_TernarySimMgr_t Ddi_TernarySimMgr_t;

typedef struct Ddi_NetlistClausesInfo_t Ddi_NetlistClausesInfo_t;

typedef struct Ddi_AigSignature_t Ddi_AigSignature_t;

typedef struct Ddi_AigSignatureArray_t Ddi_AigSignatureArray_t;

//@US
typedef struct Ddi_AigDynSignature_t Ddi_AigDynSignature_t;
typedef struct Ddi_AigDynSignatureArray_t Ddi_AigDynSignatureArray_t;

typedef struct Ddi_AigAbcInfo_s Ddi_AigAbcInfo_t;
typedef struct Ddi_IncrSatMgr_s Ddi_IncrSatMgr_t;



/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**Macro***********************************************************************
  Synopsis    [Wrapper for setting an option value]
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/

#define Ddi_MgrSetOption(mgr,tag,dfield,data) {	\
  Pdtutil_OptItem_t optItem;\
  optItem.optTag.eDdiOpt = tag;\
  optItem.optData.dfield = data;\
  Ddi_MgrSetOptionItem(mgr, optItem);\
}


/*
 *  Read fields and components
 */

/**Macro***********************************************************************
  Synopsis    [Read handle code.]
  Description [Read handle code. The code field of DDI nodes is a sort of 
    subtype. In case of BDDs, it selects among monolithic, partitioned,
    meta and aig formats. In other cases it is not used.
    Allowed return values are:
    <ul> 
    <li> Ddi_Bdd_Mono_c (monolithic BDD)
    <li> Ddi_Bdd_Part_Conj_c (conjunctively partitioned BDD)
    <li> Ddi_Bdd_Part_Disj_c (disjunctively partitioned BDD)
    <li> Ddi_Bdd_Meta_c (meta BDD)
    <li> Ddi_Bdd_Aig_c (Aig)
    <li> Ddi_Expr_Op_c (operation node in expressions)
    <li> Ddi_Expr_String_c (string identifier terminal node of expressions)
    <li> Ddi_Expr_Bdd_c (BDD terminal of expressions)
    <li> Ddi_Null_code_c (no BDD pointed, possibly wrong DDI node)
    </ul>
  ]
  SideEffects []
******************************************************************************/
#define /* Ddi_Code_e */ Ddi_ReadCode(f /* any DDI node pointer */) \
  Ddi_GenericReadCode((Ddi_Generic_t *)(f))

/**Macro***********************************************************************

  Synopsis    [Read the DDI Manager field.]

  Description [Read the DDI Manager field. This is a pointer to the owner 
    DDI manager.
    ]

  SideEffects []

******************************************************************************/

#define /* Ddi_Mgr_t * */ Ddi_ReadMgr(f /* any DDI node pointer */) \
  Ddi_GenericReadMgr((Ddi_Generic_t *)(f))

/**Macro***********************************************************************
  Synopsis    [Read the name field.]

  Description [Read the name field. Mainly thought for debug/report purposes.]

  SideEffects []

******************************************************************************/

#define /* char * */ Ddi_ReadName(f /* any DDI node pointer */) \
  Ddi_GenericReadName((Ddi_Generic_t *)(f))


/**Macro***********************************************************************

  Synopsis    [Compute Operation generating a BDD (Ddi_Bdd_t).]

  Description [Compute unary, binary or tenary operation returning a BDD.
    Result is generated.
    Requires an operation code and three operand parameters.
    It is also available in the ...Opi version, where i = 1, 2, or 3, is
    the number of operands.
    The same functions are also available in the accumulated version.
    The complete list of related functions follows:
    <ul>
    <li>   Ddi_BddOp(op,f,g,h)
    <li>   Ddi_BddOp1(op,f)    
    <li>   Ddi_BddOp2(op,f,g)  
    <li>   Ddi_BddOp3(op,f,g,h)
    <li>   Ddi_BddOpAcc(op,f,g,h)
    <li>   Ddi_BddOpAcc1(op,f)    
    <li>   Ddi_BddOpAcc2(op,f,g)  
    <li>   Ddi_BddOpAcc3(op,f,g,h)
    </ul>
    whereas the list of available operation codes is:
    <ul>
    <li>   Ddi_BddNot_c,
    <li>   Ddi_BddSwapVars_c,
    <li>   Ddi_BddMakeMono_c,
    <li>   Ddi_BddAnd_c,
    <li>   Ddi_BddDiff_c,
    <li>   Ddi_BddNand_c,
    <li>   Ddi_BddOr_c,
    <li>   Ddi_BddNor_c,
    <li>   Ddi_BddXor_c,
    <li>   Ddi_BddXnor_c,
    <li>   Ddi_BddExist_c,
    <li>   Ddi_BddForall_c,
    <li>   Ddi_BddConstrain_c,
    <li>   Ddi_BddRestrict_c,
    <li>   Ddi_BddCproject_c,
    <li>   Ddi_BddAndExist_c,
    <li>   Ddi_BddIte_c,
    <li>   Ddi_BddSupp_c,
    <li>   Ddi_BddSuppAttach_c,
    <li>   Ddi_BddSuppDetach_c,
    </ul>
    ]

  SideEffects []

  SeeAlso     [Ddi_BddarrayOp]

*****************************************************************************/

#define Ddi_BddOp(op,f,g,h) \
  ((Ddi_Bdd_t*)Ddi_GenericOp(op,\
  (Ddi_Generic_t*)(f),(Ddi_Generic_t*)(g),(Ddi_Generic_t*)(h)))
#define Ddi_BddOpAcc(op,f,g,h) \
  ((Ddi_Bdd_t*)Ddi_GenericOpAcc(op,\
  (Ddi_Generic_t*)(f),(Ddi_Generic_t*)(g),(Ddi_Generic_t*)(h)))

#define Ddi_BddOp1(op,f)             Ddi_BddOp(op,f,NULL,NULL)
#define Ddi_BddOp2(op,f,g)           Ddi_BddOp(op,f,g,NULL)
#define Ddi_BddOp3(op,f,g,h)         Ddi_BddOp(op,f,g,h)
#define Ddi_BddOpAcc1(op,f)          Ddi_BddOpAcc(op,f,NULL,NULL)
#define Ddi_BddOpAcc2(op,f,g)        Ddi_BddOpAcc(op,f,g,NULL)
#define Ddi_BddOpAcc3(op,f,g,h)      Ddi_BddOpAcc(op,f,g,h)

/**Macro***********************************************************************
  Synopsis    [Compute Operation on all entries of an Array of BDDs.]
  Description [The function, for all elements of the array, computes
    unary, binary or tenary operation involving the <em>i-th</em> element 
    of the array. Result is generated.
    The function requires an operation code and three operand parameters.
    It is also available in the ...Op<i> version, where <i> = 1, 2, or 3, is
    the number of operands.
    All functions operate in accumulated mode, i.e. they modify the first 
    parameter instead of generating a new array.
    The complete list of related functions follows:
    </ul>
    <li>   Ddi_BddarrayOp(op,f,g,h)
    <li>   Ddi_BddarrayOp1(op,f)    
    <li>   Ddi_BddarrayOp2(op,f,g)  
    <li>   Ddi_BddarrayOp3(op,f,g,h)
    </ul>
    ]
  SideEffects []
  SeeAlso     [Ddi_BddOp]
*****************************************************************************/

#define Ddi_BddarrayOp(op,f,g,h) \
  ((Ddi_Bddarray_t*)Ddi_GenericOpAcc(op,\
  (Ddi_Generic_t*)(f),(Ddi_Generic_t*)(g),(Ddi_Generic_t*)(h)))

#define Ddi_BddarrayOp1(op,f)        Ddi_BddarrayOp(op,f,NULL,NULL)
#define Ddi_BddarrayOp2(op,f,g)      Ddi_BddarrayOp(op,f,g,NULL)
#define Ddi_BddarrayOp3(op,f,g,h)    Ddi_BddarrayOp(op,f,g,h)


/**Macro***********************************************************************
  Synopsis    [Free DDI node.]
  Description [Free DDI node (compatible with all DDI handles).]
  SideEffects []
******************************************************************************/
#define Ddi_Free(f/* any DDI node pointer */) \
  (((f)!=NULL)?(Ddi_GenericFree((Ddi_Generic_t *)(f)),(f)=NULL):NULL)

/**Macro***********************************************************************
  Synopsis    [Copy Data between DDI nodes.]
  Description [Free DDI node (compatible with all DDI handles).]
  SideEffects []
******************************************************************************/
#define Ddi_DataCopy(f/* dest node pointer */,g/* src node */)\
  DdiGenericDataCopy((Ddi_Generic_t *)(f),(Ddi_Generic_t *)(g))

/**Macro***********************************************************************
  Synopsis    [Lock DDI node.]
  Description [Lock DDI node so that cannot be freed unless unlocked.
    Used as a protection mechanism for internal objects (array entries,
    partitions, ...]
  SideEffects [Ddi_Unlock]
******************************************************************************/
#define  Ddi_Lock(f/* any DDI node pointer */) \
  Ddi_GenericLock((Ddi_Generic_t *)(f))

/**Macro***********************************************************************
  Synopsis    [Unlock DDI node.]
  Description [Unlock DDI node so that can be freed again.]
  SideEffects [Ddi_Lock]
******************************************************************************/
#define  Ddi_Unlock(f/* any DDI node pointer */) \
  Ddi_GenericUnlock((Ddi_Generic_t *)(f));

/**Macro***********************************************************************
  Synopsis     [Set name field of DDI node]
  Description  [Set name field of DDI node]
  SideEffects []
******************************************************************************/
#define Ddi_SetName(f/* any DDI node pointer */, /* char * */ name) \
  Ddi_GenericSetName((Ddi_Generic_t *)(f),name)

/**Macro***********************************************************************
  Synopsis     [check if partitioned]
  Description  [check if partitioned]
  SideEffects []
******************************************************************************/
#define Ddi_BddIsPart(f) \
  (Ddi_BddIsPartConj((f))||Ddi_BddIsPartDisj((f)))


/**Macro***********************************************************************
  Synopsis     [Read name field of DDI node]
  Description  [Read name field of DDI node]
  SideEffects []
******************************************************************************/
#define Ddi_ReadName(f/* any DDI node pointer */) \
  Ddi_GenericReadName((Ddi_Generic_t *)(f))



#define DDI_MAX(a,b)         ((a>b)? a:b)

/* hide dependancy on the CUDD package */

#define ddiP(F) cuddP((Ddi_ReadMgr(F))->mgrCU,Ddi_BddToCU(F))
#define Ddi_MgrReduceHeap(dd,m,n) Cudd_ReduceHeap(Ddi_MgrReadMgrCU(dd),m,n)
#define Ddi_MgrMakeTreeNode(dd,i,j,mode) Cudd_MakeTreeNode(Ddi_MgrReadMgrCU(dd),i,j,mode)
#define Ddi_MgrCheckZeroRef(dd) Cudd_CheckZeroRef(Ddi_MgrReadMgrCU(dd))
#define Ddi_MgrReadReorderingStatus(dd,pmethod) \
                       Cudd_ReorderingStatus(Ddi_MgrReadMgrCU(dd),\
                        ((Cudd_ReorderingType *)pmethod))
#define Ddi_MgrReadPerm(dd,i) Cudd_ReadPerm(Ddi_MgrReadMgrCU(dd),i)
#define Ddi_MgrReadInvPerm(dd,i) Cudd_ReadInvPerm(Ddi_MgrReadMgrCU(dd),i)
#define Ddi_MgrReadSize(dd) Cudd_ReadSize(Ddi_MgrReadMgrCU(dd))
#define Ddi_MgrReadNumCuddVars(dd) (Cudd_ReadSize(Ddi_MgrReadMgrCU(dd)))
#define Ddi_MgrReadNumAigVars(dd) (Ddi_VararrayNum((dd)->baigVariables))
#define Ddi_MgrReadSlots(dd) Cudd_ReadSlots(Ddi_MgrReadMgrCU(dd))
#define Ddi_MgrReadKeys(dd) Cudd_ReadKeys(Ddi_MgrReadMgrCU(dd))
#define Ddi_MgrReadLiveNodes(dd) \
  (Cudd_ReadKeys(Ddi_MgrReadMgrCU(dd))-Cudd_ReadDead(Ddi_MgrReadMgrCU(dd)))
#define Ddi_MgrReadPeakNodeCount(dd) \
  (Cudd_ReadPeakNodeCount(Ddi_MgrReadMgrCU(dd)))
#define Ddi_MgrReadPeakLiveNodeCount(dd) \
  (Cudd_ReadPeakLiveNodeCount(Ddi_MgrReadMgrCU(dd)))
#define Ddi_MgrReadMinDead(dd) Cudd_ReadMinDead(Ddi_MgrReadMgrCU(dd))
#define Ddi_MgrReadMemoryInUse(dd) Cudd_ReadMemoryInUse(Ddi_MgrReadMgrCU(dd))
#define Ddi_MgrReadReorderings(dd) Cudd_ReadReorderings(Ddi_MgrReadMgrCU(dd))
#define Ddi_MgrReadReorderingTime(dd) Cudd_ReadReorderingTime(Ddi_MgrReadMgrCU(dd))
#define Ddi_MgrReadGarbageCollections(dd) Cudd_ReadGarbageCollections(Ddi_MgrReadMgrCU(dd))
#define Ddi_MgrReadGarbageCollectionTime(dd) Cudd_ReadGarbageCollectionTime(Ddi_MgrReadMgrCU(dd))

#define Ddi_BddEval(dd,f,inputs) Cudd_Eval(Ddi_MgrReadMgrCU(dd),Ddi_BddToCU(f),inputs)
#define Ddi_BddIsConstant(F) \
 (Ddi_BddIsOne(F)||Ddi_BddIsZero(F))
#define Ddi_BddIsComplement(F) \
(Ddi_BddIsAig((F))?Ddi_AigIsInverted((F)):Cudd_IsComplement(Ddi_BddToCU(F)))
#define Ddi_MgrSetMaxCacheHard(dd,n) \
 Cudd_SetMaxCacheHard(Ddi_MgrReadMgrCU(dd),n)
#define Ddi_MgrSetMaxMemory(dd,n) Cudd_SetMaxMemory(Ddi_MgrReadMgrCU(dd),n)
#define Ddi_MgrSetNextReordering(dd,n) \
  Cudd_SetNextReordering(Ddi_MgrReadMgrCU(dd),n)

/* old pdtrav supported function */
#define Ddi_CuddNodeIsVisited(fcu) \
  Cudd_IsComplement(Cudd_Regular(fcu)->next)
#define Ddi_CuddNodeSetVisited(fcu) \
{\
  Cudd_Regular(fcu)->next = Cudd_Complement(Cudd_Regular(fcu)->next);\
}
#define Ddi_CuddNodeClearVisited(fcu) \
{\
  if (Ddi_CuddNodeIsVisited(fcu)) {\
    Cudd_Regular(fcu)->next = Cudd_Regular(Cudd_Regular(fcu)->next);\
  }\
}

#define Ddi_CuddNodeElse(fcu) \
  (Cudd_IsComplement(fcu) ? Cudd_Not(Cudd_E(fcu)) : Cudd_E(fcu))

#define Ddi_CuddNodeThen(fcu) \
  (Cudd_IsComplement(fcu) ? Cudd_Not(Cudd_T(fcu)) : Cudd_T(fcu))
 
#define Ddi_CuddNodeIsConstant(F) Cudd_IsConstant(F)
#define Ddi_CuddNodeReadIndex(F) Cudd_NodeReadIndex(F)


#define Ddi_BddPrintDbg(msg,f,fp) \
{\
  Ddi_Varset_t *supp;\
  fprintf(fp,"%s",msg);\
  if (Ddi_BddIsZero(f))\
    printf("ZERO!\n");\
  else if (Ddi_BddIsOne(f)) \
    printf("ONE!\n");\
  else {\
    supp = Ddi_BddSupp(f);\
    Ddi_VarsetPrint(supp,100,NULL,fp);\
    Ddi_BddPrintCubes(f,NULL,100,0,NULL,fp);\
    Ddi_Free(supp);\
  }\
}

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EXTERN int Ddi_AigPrintStats(Ddi_Bdd_t *fAig);
EXTERN int Ddi_AigPrintNodeStats(Ddi_Bdd_t *fAig, int npsVars);
EXTERN void Ddi_AigOptByAbc(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_AigOrDpartDecompose(Ddi_Bdd_t *fAig, int nPart, int maxRedFactor);
EXTERN Ddi_Bdd_t * Ddi_AigOrDecompose(Ddi_Bdd_t *fAig);
EXTERN Ddi_Bdd_t * Ddi_AigDisjDecompWithVars(Ddi_Bdd_t *fAig, Ddi_Vararray_t *partitionVars);
EXTERN Ddi_Bdd_t * Ddi_AigSubsetNode(Ddi_Bdd_t *fAig, int npsVars, int verbose);
EXTERN Ddi_Bdd_t * Ddi_AigSubset(Ddi_Bdd_t *fAig, int threshold, int verbose);
EXTERN Ddi_Bdd_t *Ddi_AigSubsetWithCube (Ddi_Bdd_t *f, Ddi_Bdd_t *cube, float ratio);
EXTERN Ddi_Bdd_t *Ddi_AigSubsetWithCubeAcc (Ddi_Bdd_t *f, Ddi_Bdd_t *cube, float ratio);
EXTERN Ddi_Bdd_t *Ddi_AigSubsetWithCexOnControl (Ddi_Bdd_t *f, Ddi_Bdd_t *cex, Ddi_Bddarray_t *enables);
EXTERN Ddi_Bdd_t *Ddi_AigSubsetWithCexConstr (Ddi_Bdd_t *cex, Ddi_Bddarray_t *enables);
EXTERN Ddi_Bdd_t * Ddi_AigSuperset(Ddi_Bdd_t *fAig, int threshold, int verbose);
EXTERN Ddi_Bdd_t * Ddi_AigSupersetNode(Ddi_Bdd_t *fAig, int npsVars, int verbose);
EXTERN Ddi_Bdd_t *
Ddi_AigSatNnfAbstrPba (
  Ddi_Bdd_t *f,
  Ddi_Bdd_t *initAbstr,
  Ddi_Bdd_t *g,
  Ddi_Vararray_t *ps,
  Ddi_Vararray_t *ns
);
EXTERN Ddi_Bdd_t *Ddi_AigSatNnfSubset (Ddi_Bdd_t *f, Ddi_Bdd_t *g, Ddi_Bdd_t *care);
EXTERN Ddi_Bdd_t *
Ddi_AigNnfStats (
  Ddi_Bdd_t *f,
  Ddi_Vararray_t *filterVars,
  int doExist
);
EXTERN Ddi_Bdd_t * Ddi_AigSatCore(Ddi_Bdd_t *fAig, Ddi_Bdd_t *care, Ddi_Bdd_t *eq, Ddi_Varset_t *projVars, int thresholdVars, int threshold, int strategy, int verbose);
EXTERN int Ddi_AigEqualSat (Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN int Ddi_AigEqualSatWithCare (Ddi_Bdd_t *f, Ddi_Bdd_t *g, Ddi_Bdd_t *care);
EXTERN int Ddi_AigCheckFixPoint(Ddi_Bdd_t *oldCheck, Ddi_Bdd_t *check, Ddi_Bdd_t *care, Ddi_Varset_t *psVars, int maxIter);
EXTERN Ddi_Varset_t *Ddi_AigRefineRelationalPartWithCex (Ddi_Bdd_t *checkPart, Ddi_Bdd_t *cex, Ddi_Varset_t *psIn, char *prefix, int cegar);
EXTERN Ddi_Bdd_t *Ddi_AigInductiveImgPlus (Ddi_Bdd_t *from, Ddi_Bdd_t *inductiveTo, Ddi_Bdd_t *care, Ddi_Bdd_t *toPlus, int inductiveLevel);
EXTERN Ddi_Bdd_t *Ddi_AigImgSimplify (Ddi_Bdd_t *img, Ddi_Bdd_t *care, Ddi_Bdd_t *cone, int inductiveLevel);
EXTERN Ddi_Bdd_t *Ddi_AigSatSubset (Ddi_Bdd_t *f, Ddi_Bdd_t *care, int inductiveLevel);
EXTERN Ddi_Bdd_t *Ddi_AigGeneralizeItpAcc (Ddi_Bdd_t *f, Ddi_Bdd_t *fPlus, Ddi_Bdd_t *care, int level);
EXTERN Ddi_Bdd_t *Ddi_AigRefineImgPlus (Ddi_Bdd_t *from, Ddi_Bdd_t *cone, Ddi_Bdd_t *pConj, Ddi_Bdd_t *toPlus, Ddi_Bdd_t *care, int refineLevel);
EXTERN Ddi_Bdd_t *Ddi_AigTightenOrLoosen (Ddi_Bdd_t *f, Ddi_Bdd_t *constr, Ddi_Bdd_t *care, int tighten);
EXTERN Pdtutil_Array_t *Ddi_AigCubeMakeIntegerArrayFromBdd (Ddi_Bdd_t *cubeAig, Ddi_Vararray_t *vars);
EXTERN Ddi_Bdd_t *Ddi_AigCubeMakeBddFromIntegerArray (Pdtutil_Array_t *cubeArray, Ddi_Vararray_t *vars);
EXTERN Ddi_Bdd_t *Ddi_AigCubeExist (Ddi_Bdd_t *cubeAig, Ddi_Varset_t *smooth);
EXTERN Ddi_Bdd_t *Ddi_AigCubeExistAcc (Ddi_Bdd_t *cubeAig, Ddi_Varset_t *smooth);
EXTERN Ddi_Bdd_t *Ddi_AigCubeExistProject (Ddi_Bdd_t *cubeAig, Ddi_Varset_t *smooth);
EXTERN Ddi_Bdd_t *Ddi_AigCubeExistProjectAcc (Ddi_Bdd_t *cubeAig, Ddi_Varset_t *smooth);
EXTERN Ddi_Bdd_t *Ddi_AigNnfSubsetWithCubeAcc(Ddi_Bdd_t *fAig, Ddi_Bdd_t *cubeAig);
EXTERN Ddi_Bdd_t * Ddi_AigAndCubeAcc(Ddi_Bdd_t *fAig, Ddi_Bdd_t *cubeAig);
EXTERN Ddi_Bdd_t * Ddi_AigConstrainCubeAcc(Ddi_Bdd_t *f, Ddi_Bdd_t *cubeAig);
EXTERN Ddi_Bddarray_t * Ddi_AigarrayConstrainCubeAcc(Ddi_Bddarray_t *fA, Ddi_Bdd_t *cubeAig);
EXTERN Ddi_Bdd_t * Ddi_AigConstrainImplAcc(Ddi_Bdd_t *f, Ddi_Bddarray_t *implArray);
EXTERN Ddi_Bdd_t * Ddi_AigConstrainOptAcc(Ddi_Bdd_t *f, Ddi_Bdd_t *c, Ddi_Varset_t *filterVars, Ddi_Varset_t *implyVars, Ddi_Bdd_t *pivotCube, int optVal);
EXTERN Ddi_Bdd_t *Ddi_AigTernaryArrayImg (Ddi_Bdd_t *from, Ddi_Bddarray_t *delta, Ddi_Vararray_t *ps, Ddi_Vararray_t *inVars);
EXTERN int Ddi_AigOptByFoCnt(Ddi_Bdd_t *f, int minFo, int maxFo, int maxRecur, int maxRecurCut, int maxSize, int doKernelExtr, int enBddOpt, int strategy, int balance, int *auxvCntP);
EXTERN int Ddi_AigOptByFoCntWithCare(Ddi_Bdd_t *f, Ddi_Bdd_t *care, int minFo, int maxFo, int maxRecur, int maxRecurCut, int maxSize, int doKernelExtr, int enBddOpt, int strategy, int balance, int *auxvCntP);
EXTERN int Ddi_AigFilterStructMonotone(Ddi_Bdd_t *f, Ddi_Varset_t *keepVars);
EXTERN Ddi_Bdd_t *Ddi_AigSplitMonotoneGenAcc (Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Bdd_t *care);
EXTERN Ddi_Bdd_t *Ddi_AigSplitMonotoneGen (Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Bdd_t *care);
EXTERN int Ddi_AigOptByFoCntTop(Ddi_Bdd_t *f, Ddi_Bdd_t *care, int enBddOpt);
EXTERN int Ddi_AigOptByBddSweepTop(Ddi_Bdd_t *f, Ddi_Bdd_t *care, int maxMerge);
EXTERN int Ddi_AigOptByMinCut(Ddi_Bdd_t *f, Ddi_Bdd_t *care, Ddi_Varset_t *projectVars, int optLevel, int topDecomp, float timeLimit);
EXTERN int Ddi_AigOptByPart(Ddi_Bdd_t *f, Ddi_Bdd_t *care, int optLevel, float timeLimit);
EXTERN int Ddi_AigOptByImpl(Ddi_Bdd_t *f, Ddi_Bddarray_t *implied, int optLevel, float timeLimit);
EXTERN int Ddi_AigOptByCut(Ddi_Bdd_t *f, Ddi_Bdd_t *care, int cutNum, int optLevel, float timeLimit);
EXTERN Ddi_Bdd_t *Ddi_AigSplitByCut(Ddi_Bdd_t *f, Ddi_Bdd_t *care, Ddi_Vararray_t *initVars, Ddi_Bddarray_t *cutFSubst, Ddi_Vararray_t *cutFVars, float cutRatio);
EXTERN Ddi_Bdd_t * Ddi_AigExistProjectAcc(Ddi_Bdd_t *fAig, Ddi_Varset_t *projectVars, Ddi_Bdd_t *care, int partial, int nosat, float timeLimit);
EXTERN Ddi_Bdd_t * Ddi_AigExistProjectOverAcc(Ddi_Bdd_t *fAig, Ddi_Varset_t *projectVars, Ddi_Bdd_t *care);
EXTERN Ddi_Bdd_t * Ddi_AigExistProjectSkolemAcc(Ddi_Bdd_t *fAig, Ddi_Varset_t *projectVars, Ddi_Bdd_t *care);
EXTERN Ddi_Bdd_t * Ddi_AigExistProjectItpAcc(Ddi_Bdd_t *fAig, Ddi_Varset_t *projectVars, Ddi_Bdd_t *care, int maxIter);
EXTERN Ddi_Bdd_t * Ddi_AigExistProjectAllSolutionAcc(Ddi_Bdd_t *fAig, Ddi_Varset_t *projectVars, Ddi_Bdd_t *care, int maxIter);
EXTERN Ddi_Bdd_t * Ddi_AigExistAcc(Ddi_Bdd_t *fAig, Ddi_Varset_t *smooth, Ddi_Bdd_t *care, int partial, int nosat, float timeLimit);
EXTERN Ddi_Bdd_t * Ddi_AigComposeExistAcc(Ddi_Bdd_t *fAig, Ddi_Vararray_t *vars, Ddi_Bddarray_t *funcs, Ddi_Varset_t *smooth, int minGroup, Ddi_Bdd_t *care, int partial, int nosat, float timeLimit);
EXTERN Ddi_Bdd_t * Ddi_AigExistSubsetAcc(Ddi_Bdd_t *fAig, Ddi_Varset_t *smooth, Ddi_Bdd_t *care, Ddi_Bdd_t *pivotCube, int subsetLevel, float timeLimit);
EXTERN Ddi_Bdd_t * Ddi_AigExistAllSolutionAcc(Ddi_Bdd_t *fAig, Ddi_Varset_t *smooth, Ddi_Bdd_t *care, Ddi_Bdd_t *window, int maxIter);
EXTERN Ddi_Bdd_t * Ddi_AigProjectAllSolutionImgAcc(Ddi_Bdd_t *fAig, Ddi_Varset_t *projVars, Ddi_Bdd_t *care, int maxIter, int apprLevel);
EXTERN Ddi_Bdd_t * Ddi_AigProjectRefineOutImgAcc(Ddi_Bdd_t *fAig, Ddi_Varset_t *projVars, Ddi_Bdd_t *care, int maxIter, int apprLevel);
EXTERN Ddi_Bdd_t * Ddi_AigProjectSatTernaryImgAcc(Ddi_Bdd_t *fAig, Ddi_Varset_t *projVars, Ddi_Varset_t *ternaryVars, Ddi_Bdd_t *care, int maxIter, int apprLevel);
EXTERN void Ddi_AigArrayExistPartialAcc(Ddi_Bddarray_t *fAigArray, Ddi_Varset_t *smooth, Ddi_Bdd_t *care);
EXTERN int Ddi_AigIsInverted(Ddi_Bdd_t *fAig);
EXTERN float Ddi_GenConstrRandSimulSignatures(Ddi_Vararray_t *alpha_va, Ddi_Vararray_t *beta_va, int beta_va_var, Ddi_Bdd_t *constr, int minCubesNum, int maxCubesNum, long timeLimit, Ddi_Bddarray_t *cubes, Pdtutil_Array_t *betas);
EXTERN Ddi_Bddarray_t * Ddi_AigarrayComposeAcc(Ddi_Bddarray_t *fArrayAig, Ddi_Vararray_t *vA, Ddi_Bddarray_t *gA);
EXTERN Ddi_Bddarray_t * Ddi_AigarrayCompose(Ddi_Bddarray_t *fArrayAig, Ddi_Vararray_t *vA, Ddi_Bddarray_t *gA);
EXTERN Ddi_Varset_t *Ddi_AigCoi (Ddi_Bdd_t *fAig, Ddi_Vararray_t *vA, Ddi_Bddarray_t *gA);
EXTERN Ddi_Varset_t *Ddi_AigarrayCoi (Ddi_Bddarray_t *fArrayAig, Ddi_Vararray_t *vA, Ddi_Bddarray_t *gA);
EXTERN Ddi_Bddarray_t * Ddi_AigarrayComposeNoMultipleAcc(Ddi_Bddarray_t *fArrayAig, Ddi_Vararray_t *vA, Ddi_Bddarray_t *gA);
EXTERN Ddi_Bddarray_t * Ddi_AigarrayComposeNoMultiple(Ddi_Bddarray_t *fArrayAig, Ddi_Vararray_t *vA, Ddi_Bddarray_t *gA);
EXTERN Ddi_Bddarray_t *Ddi_AigarrayComposeFuncAcc (Ddi_Bddarray_t *fArrayAig, Ddi_Bddarray_t *oldA, Ddi_Bddarray_t *newA);
EXTERN Ddi_Bddarray_t *Ddi_AigarrayComposeFunc (Ddi_Bddarray_t *fArrayAig, Ddi_Bddarray_t *oldA, Ddi_Bddarray_t *newA);
EXTERN Ddi_Bdd_t *Ddi_AigComposeFunc (Ddi_Bdd_t *fAig, Ddi_Bddarray_t *oldA, Ddi_Bddarray_t *newA);
EXTERN int Ddi_AigActive(Ddi_Mgr_t *ddm);
EXTERN void Ddi_AigInit(Ddi_Mgr_t *ddm, int maxSize);
EXTERN void Ddi_AigQuit(Ddi_Mgr_t *ddm);
EXTERN Ddi_Bdd_t * Ddi_AigMakePartDisj(Ddi_Bdd_t *f);
EXTERN void Ddi_AigSetPartDisj(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddMakeFromAig(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddMakeFromAigWithAbort(Ddi_Bdd_t *f, int th);
EXTERN int Ddi_AigCnfStore(Ddi_Bdd_t *f, char *filename, FILE *fp);
EXTERN int Ddi_AigTrCnfStore(Ddi_Bdd_t *tr, Ddi_Vararray_t *psVars, Ddi_Vararray_t *nsVars, Ddi_Vararray_t *piVars, char *filename);
EXTERN int Ddi_AigQbfAndSolve(Ddi_Bdd_t *f, Ddi_Bdd_t *g, Ddi_Vararray_t *forallVars);
EXTERN void Ddi_AigFsmStore(Ddi_Vararray_t *pi, Ddi_Vararray_t *ps, Ddi_Vararray_t *ns, Ddi_Bdd_t *s0, Ddi_Bddarray_t *delta, Ddi_Bddarray_t *lambda, char *filename, FILE *fp, Pdtutil_AigDump_e format);
EXTERN Ddi_Bdd_t * Ddi_AigNetStore(Ddi_Bdd_t *f, char *filename, FILE *fp, Pdtutil_AigDump_e format);
EXTERN Ddi_Bdd_t * Ddi_AigarrayNetStore(Ddi_Bddarray_t *fA, char *filename, FILE *fp, Pdtutil_AigDump_e format);
EXTERN Ddi_Bdd_t * Ddi_AigNetLoadBench(Ddi_Mgr_t *ddm, char *filename, FILE *fp);
EXTERN Ddi_Bddarray_t * Ddi_AigarrayNetLoadBench(Ddi_Mgr_t *ddm, char *filename, FILE *fp);
EXTERN Ddi_Bdd_t * Ddi_AigMakeFromBdd(Ddi_Bdd_t *f);
EXTERN int * Ddi_AigSatMultiple(Ddi_Bddarray_t *fA, Ddi_Bdd_t *care);
EXTERN int * Ddi_AigSatMultipleMinisat(Ddi_Bddarray_t *fA, Ddi_Bdd_t *care);
EXTERN int Ddi_AigSat(Ddi_Bdd_t *f);
EXTERN int Ddi_AigSatAnd(Ddi_Bdd_t *f, Ddi_Bdd_t *g, Ddi_Bdd_t *care);
EXTERN int Ddi_AigSatAndWithAbort(Ddi_Bdd_t *f, Ddi_Bdd_t *g, Ddi_Bdd_t *care, int timeLimit);
EXTERN Ddi_Bdd_t *Ddi_AigSatAndWithCexAndAbort(Ddi_Bdd_t *f, Ddi_Bdd_t *g, Ddi_Bdd_t *care, Ddi_Vararray_t *filterVars, int timeLimit, int *pAbort);
EXTERN int Ddi_AigSatConstrain(Ddi_Bdd_t *f, Ddi_Bdd_t *constr, int timeLimit, int *aboP);
EXTERN Ddi_Bdd_t * Ddi_AigSatWithCex(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_AigSatWithCexAndAbort(Ddi_Bdd_t *f, Ddi_Vararray_t *filterVars, float timeLimit, int *pAbort);
EXTERN int Ddi_AigSatBerkmin(Ddi_Bdd_t *f);
EXTERN int Ddi_AigSatMinisat(Ddi_Bdd_t *f);
EXTERN int Ddi_AigSatMinisatWithAbort(Ddi_Bdd_t *f, float timeLimit);
EXTERN int Ddi_AigSatMinisat22WithAbortAndFinal (Ddi_Bdd_t *f, Ddi_Bdd_t *constrCube, int doGen, float timeLimit);
EXTERN Ddi_Bdd_t *
Ddi_AigSatMinisat22RemoveCoreFinal
(
  Ddi_Bdd_t *f,
  Ddi_Bdd_t *constr,
  int doGen,
  float timeLimit,
  int *pAbort
);
EXTERN int Ddi_AigSatMinisatWithAbortAndFinal (Ddi_Bdd_t *f, Ddi_Bdd_t *constrCube, float timeLimit, int split);
EXTERN Ddi_Bdd_t *Ddi_AigSatWindow(Ddi_Bdd_t *f, Ddi_Bdd_t *care, Ddi_Bdd_t *pivotCube, Ddi_Varset_t *vars, float timeLimit);
EXTERN Ddi_Bdd_t *Ddi_AigSatMinisatWithCexAndAbort(Ddi_Bdd_t *f, Ddi_Vararray_t *filterVars, float timeLimit, char *dumpFilename, int *pAbort);
EXTERN Ddi_Bdd_t *Ddi_AigSatMinisatWithCexAndAbortIncremental(Ddi_IncrSatMgr_t *incrSat, Ddi_Bdd_t *f, Ddi_Vararray_t *filterVars, float timeLimit, int *pAbort);
EXTERN void Ddi_IncrSatPrintStats(Ddi_Mgr_t *ddm,Ddi_IncrSatMgr_t *incrSat, int doProof);
EXTERN int Ddi_AigSatMinisat22Solve(Ddi_Mgr_t *ddm, Ddi_IncrSatMgr_t *incrSat);
EXTERN void Ddi_IncrSolverFreezeVars (Ddi_Mgr_t *ddm, Ddi_IncrSatMgr_t *incrSat, Ddi_Vararray_t *vars);
EXTERN int Ddi_AigSatMinisat22WithAbortIncremental(Ddi_IncrSatMgr_t *incrSat, Ddi_Bdd_t *f, Ddi_Vararray_t *filterVars, float timeLimit, int *pAbort);
EXTERN void
Ddi_AigSatInitLearningIncremental
(
  Ddi_IncrSatMgr_t *incrSat,
  int restart
);
EXTERN Ddi_Bddarray_t *
Ddi_AigSatImpliedLearningIncremental (
  Ddi_IncrSatMgr_t *incrSat,
  Ddi_Bdd_t *assume
);
EXTERN void
Ddi_AigSatStopLearningIncremental
(
  Ddi_IncrSatMgr_t *incrSat
);
EXTERN Ddi_Bdd_t *Ddi_AigSatMinisat22WithCexAndAbortIncremental(Ddi_IncrSatMgr_t *incrSat, Ddi_Bdd_t *f, Ddi_Vararray_t *filterVars, int asserted, float timeLimit, int *pAbort);
Ddi_Bdd_t *
Ddi_AigSatMinisat22WithCexAigAndAbortIncremental
(
  Ddi_IncrSatMgr_t *incrSat,
  Ddi_Bdd_t *f,
  int nSatVars,
  int asserted,
  float timeLimit,
  int *pAbort
);
EXTERN Ddi_Bdd_t *
Ddi_AigSatMinisat22SubsetWithCexIncremental
(
  Ddi_IncrSatMgr_t *incrSat,
  Ddi_Bdd_t *f,
  Ddi_Bdd_t *forSubs,
  float subsetRatio,
  int asserted,
  float timeLimit,
  int *pAbort,
  Ddi_Mgr_t *ddmAux
 );
EXTERN Ddi_Bdd_t *Ddi_AigSatMinisat22WithCexRefinedIncremental(Ddi_IncrSatMgr_t *incrSat, Ddi_Bdd_t *f, Ddi_Vararray_t *filterVars, int asserted, float timeLimit, int *pAbort);
EXTERN void Ddi_AigarrayLockTopClauses (Ddi_IncrSatMgr_t *incrSat, Ddi_Bddarray_t *fA);
EXTERN void Ddi_AigLockTopClauses (Ddi_IncrSatMgr_t *incrSat, Ddi_Bdd_t *f);
EXTERN int Ddi_AigSatMinisatLoadBddarrayClausesIncremental(Ddi_IncrSatMgr_t *incrSat, Ddi_Bddarray_t *fA);
EXTERN int Ddi_AigSatMinisatLoadClausesIncremental(Ddi_IncrSatMgr_t *incrSat, Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN int Ddi_AigSatMinisatLoadClausesIncrementalAsserted(Ddi_IncrSatMgr_t *incrSat, Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_AigSatMinisatWithCex(Ddi_Bdd_t *f);
EXTERN int Ddi_AigSatCircuit(Ddi_Bdd_t *f);
EXTERN int Ddi_AigSatWithAbort(Ddi_Bdd_t *f, float timeLimit);
EXTERN int Ddi_AigSatCircuitWithAbort(Ddi_Bdd_t *f, int timeLimit);
EXTERN int Ddi_AigSatBerkminWithAbort(Ddi_Bdd_t *f, int timeLimit);
EXTERN Ddi_Bdd_t * Ddi_AigDiffOptByBdd(Ddi_Bdd_t *f, Ddi_Var_t *v);
EXTERN void Ddi_AigDomainStats(Ddi_Bdd_t *f);
EXTERN int Ddi_AigOptByInterpolantAcc(Ddi_Bdd_t *f, Ddi_Bdd_t *fPlus, Ddi_Bdd_t *care);
EXTERN int Ddi_AigOptByBdd(Ddi_Bdd_t *f, float timeLimit, int sizeLimit);
EXTERN int Ddi_AigOptByBddWithCare(Ddi_Bdd_t *f, Ddi_Bdd_t *care, float timeLimit, int sizeLimit);
EXTERN Ddi_Bddarray_t * Ddi_AigarrayOptByBdd(Ddi_Bddarray_t *fA, Ddi_Bddarray_t *auxF, Ddi_Vararray_t *auxV, int th, int makeBddVars, float timeLimit, int sizeLimit);
EXTERN void Ddi_AigOptByBddPartial(Ddi_Bdd_t *f, Ddi_Bdd_t *care, float timeLimit);
EXTERN void Ddi_AigSift(Ddi_Bdd_t *f, Ddi_Bdd_t *care);
EXTERN void Ddi_AigarraySift(Ddi_Bddarray_t *fA);
EXTERN void Ddi_AigOptByMeta(Ddi_Bdd_t *f, Ddi_Bdd_t *care);
EXTERN Ddi_Bdd_t *Ddi_AigOptByConstrain(Ddi_Bdd_t *f, Ddi_Bdd_t *care, Ddi_Varset_t *smooth, int level);
EXTERN void Ddi_AigOptTopOr(Ddi_Bdd_t *f, int neg);
EXTERN void Ddi_AigOptTop(Ddi_Bdd_t *f, int maxDepth, int sizeMax);
EXTERN Ddi_Bddarray_t *Ddi_AigGates(Ddi_Bdd_t *f,int includeVars);
EXTERN Ddi_Bddarray_t *Ddi_FindPropCut(Ddi_Bdd_t *prop, Ddi_Vararray_t *ps, Ddi_Bddarray_t *delta, int levels);
EXTERN Ddi_Bdd_t *Ddi_AigPartitionTop(Ddi_Bdd_t *f, int topOr);
EXTERN Ddi_Bdd_t *Ddi_AigRePartitionAcc(Ddi_Bdd_t *f, int min);
EXTERN Ddi_Bdd_t *Ddi_AigPartitionTopWithXor(Ddi_Bdd_t *f, int topOr, int keepXor);
EXTERN Ddi_Bddarray_t *Ddi_AigPartitionTopIte(Ddi_Bdd_t *f, int sizeTh);
EXTERN Ddi_Bddarray_t *Ddi_AigPartitionTopXorOrXnor(Ddi_Bdd_t *f, int doXor);
EXTERN int Ddi_AigIsXorOrXnor(Ddi_Bdd_t *f);
EXTERN int * Ddi_AigProveLemmas(Ddi_Bdd_t *f, Ddi_Bddarray_t *lemmas, Ddi_Bddarray_t *assumptions);
EXTERN int * Ddi_AigProveLemmasMinisat(Ddi_Bdd_t *f, Ddi_Bddarray_t *lemmas, Ddi_Bddarray_t *assumptions);
EXTERN Ddi_Bddarray_t * Ddi_AigComputeInitialLemmas(Ddi_Bddarray_t *lemmasSimul, Ddi_Bddarray_t *lemmasBase, Ddi_Bdd_t *initState, int maxLevel);
EXTERN char ** Ddi_AigCombinationalImplications(Ddi_Bddarray_t *lemmasBase, int complement);
EXTERN Ddi_Bddarray_t *Ddi_BddarrayFindMinCut (Ddi_Bddarray_t *fA, Ddi_Bdd_t *care, Ddi_Vararray_t *initVars,   Ddi_Vararray_t *lockedVars, Ddi_Bddarray_t *substF, Ddi_Vararray_t *substV, int disablePiFlow, int fwdCut);
EXTERN Ddi_Bdd_t *Ddi_BddSplitMinCut (Ddi_Bdd_t *f, Ddi_Bdd_t *care, Ddi_Vararray_t *initVars, Ddi_Bddarray_t *substF, Ddi_Vararray_t *substV, int disablePiFlow, int fwdCut, float cutRatio);
EXTERN Ddi_Bdd_t *Ddi_BddSplitShortEdges (Ddi_Bdd_t *f, Ddi_Bdd_t *care, Ddi_Vararray_t *initVars, Ddi_Bddarray_t *substF, Ddi_Vararray_t *substV, float cutRatio);
EXTERN Ddi_Bddarray_t *Ddi_BddarraySplitShortEdges (Ddi_Bddarray_t *fA, Ddi_Bdd_t *care, Ddi_Vararray_t *initVars, Ddi_Bddarray_t *substF, Ddi_Vararray_t *substV, float cutRatio);
EXTERN Ddi_Bddarray_t * Ddi_LemmaGenerateClasses(Ddi_Vararray_t *pi, Ddi_Vararray_t *ps, Ddi_Vararray_t *ns, Ddi_Bddarray_t *delta, Ddi_Bdd_t *init, Ddi_Bddarray_t *initStub, Ddi_Bdd_t *invar, Ddi_Bdd_t *spec, Ddi_Bdd_t *careBdd, Ddi_Bddarray_t *constants, int bound, int max_false, int do_compaction, int do_implication, int do_strong, int verbosity, int *timeLimitPtr, Ddi_Bddarray_t *reprs, Ddi_Bddarray_t *equals, Ddi_Bddarray_t *false_lemmas, int *result, int *done_bound, int *again_ptr);
EXTERN int Ddi_AigCustomCombinationalCircuit(Ddi_Mgr_t *ddm, char *filename, int method, int compl_inv, int cnt_reached);
EXTERN int Ddi_AigSingleInterpolantCompaction(Ddi_Mgr_t *ddm, char *filename, char *outFilename, int method);
EXTERN int Ddi_AigSingleInterpolantLogicSynthesisCompaction(Ddi_Mgr_t *ddm, char *filename);
EXTERN Ddi_Bdd_t * Ddi_AigSatAndWithInterpolant(Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Varset_t *globalVars, Ddi_Varset_t *domainVars, Ddi_Bdd_t *optCare, Ddi_Bdd_t *prevItp, Ddi_Bdd_t *constrCube, Ddi_Bddarray_t *implArray, int *psat, int itpPart, int itpOdc, float timeLimit);
EXTERN Ddi_Bdd_t * Ddi_AigSatAndWithInterpolantIncr(Ddi_IncrSatMgr_t *incrSat, Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Varset_t *globalVars, Ddi_Varset_t *domainVars, Ddi_Bdd_t *optCare, Ddi_Bdd_t *prevItp, Ddi_Bdd_t *constrCube, Ddi_Bddarray_t *implArray, int *psat, int itpPart, int itpOdc, float timeLimit);
EXTERN Ddi_Bdd_t * Ddi_AigSat22AndWithInterpolant(Ddi_IncrSatMgr_t *incrSat, Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Bdd_t *bSplit, Ddi_Varset_t *globalVars, Ddi_Varset_t *domainVars, Ddi_Vararray_t **TfPiVars, int tfPiNum, Ddi_Bdd_t *optCare, Ddi_Bdd_t *prevItp, int *psat, int itpPart, int itpOdc, int tryRevItp, float timeLimit);
EXTERN Ddi_Bdd_t * Ddi_AigSat22AndWithInterpolantPart(Ddi_IncrSatMgr_t *incrSat, Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Varset_t *globalVars, Ddi_Varset_t *domainVars, Ddi_Bdd_t *optCare, Ddi_Bdd_t *prevItp, int *psat, int itpPart, int itpOdc, float timeLimit);
EXTERN Ddi_Bdd_t *Ddi_AigSat22AndWithInterpolantPartNnfA (Ddi_IncrSatMgr_t *incrSat, Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Bddarray_t *bwdRings, Ddi_Bdd_t *prevA, Ddi_Varset_t *globalVars, Ddi_Varset_t *domainVars, Ddi_Vararray_t *globalVarsA, Ddi_Vararray_t **tfPiVars, int tfPiNum, Ddi_Bdd_t *optCare, Ddi_Bdd_t *itpPlus, int *psat, int itpPart, int itpOdc, float timeLimit);
EXTERN Ddi_Bdd_t *Ddi_AigSatAndWithAigCore (Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Bdd_t *optCare, int useMonotone, int *psat, float timeLimit);
EXTERN Ddi_Bdd_t *Ddi_AigSatAndWithAigCoreByRefinement (Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Bdd_t *optCare, int useMonotone, int *psat, float timeLimit);
EXTERN Ddi_Bdd_t *Ddi_AigNnfOptBySatSimplify (Ddi_Bdd_t *itp, Ddi_Bdd_t *b, Ddi_Bdd_t *optCare, int complement);
EXTERN Ddi_Bdd_t *Ddi_AigNnfOptBySatSimplifyAcc (Ddi_Bdd_t *itp, Ddi_Bdd_t *b, Ddi_Bdd_t *optCare, int complement);
EXTERN Ddi_Bdd_t *Ddi_AigOptByMonotoneCore (Ddi_Bdd_t *itp, Ddi_Bdd_t *b, Ddi_Bdd_t *optCare, int complement, float timeLimit);
EXTERN Ddi_Bdd_t *Ddi_AigOptByMonotoneCoreAcc (Ddi_Bdd_t *itp, Ddi_Bdd_t *b, Ddi_Bdd_t *optCare, int complement, float timeLimit);
EXTERN Ddi_Bdd_t *Ddi_AigOptByMonotoneCoreDualAcc (Ddi_Bdd_t *itp, Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Bdd_t *optCare, float timeLimit);
EXTERN Ddi_Bdd_t *Ddi_AigOptByMonotoneCoreWithForallAcc (Ddi_Bdd_t *itp, Ddi_Bdd_t *b, Ddi_Bdd_t *optCare, int complement, float timeLimit);
EXTERN Ddi_Bdd_t *Ddi_AigOptByMonotoneCoreByRefinementAcc (Ddi_Bdd_t *itp, Ddi_Bdd_t *b, Ddi_Bdd_t *optCare, int complement, float timeLimit);
EXTERN Ddi_Bdd_t *Ddi_AigOptByMonotoneCorePartAcc (Ddi_Bdd_t *itp, Ddi_Bdd_t *b, Ddi_Bdd_t *optCare, int complement, float timeLimit);
EXTERN Ddi_Bdd_t *Ddi_AigOptByMonotoneCoreDecompAcc (Ddi_Bdd_t *itp, Ddi_Bdd_t *b, Ddi_Bdd_t *optCare, Ddi_Bddarray_t *partitionLits,   void *coreBClauses, void *cnfMappedVars, int complement, float timeLimit);
EXTERN Ddi_Bdd_t *Ddi_AigOptByMonotoneCorePartAcc0 (Ddi_Bdd_t *itp, Ddi_Bdd_t *b, Ddi_Bdd_t *optCare, Ddi_Bdd_t *fromAndTr, Ddi_Bdd_t *prevItp, Ddi_Varset_t *vars, int maxPart, int complement, float timeLimit);
EXTERN Ddi_Bddarray_t *Ddi_AigSatAndWithInterpolantSequence (Ddi_Bdd_t *a, Ddi_Bdd_t *b, int *psat, int nCuts, float timeLimit);
EXTERN Ddi_Bddarray_t *Ddi_AigItpSeqEq(Ddi_Bdd_t *a, Ddi_Vararray_t *eqVars, Ddi_Bddarray_t *eqSubst, int bound);
EXTERN Ddi_Bdd_t * Ddi_AigAbstrVarsForInterpolantByRefinement(Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Vararray_t *aVars, Ddi_Vararray_t *bVars, Ddi_Vararray_t *auxVars, char *wfileName, unsigned char *enAbstr);
EXTERN Ddi_Bdd_t * Ddi_AigAbstrVarsForInterpolant(Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Vararray_t *aVars, Ddi_Vararray_t *bVars, Ddi_Vararray_t *auxVars, char *wfileName, unsigned char *enAbstr, int *pAbort);
EXTERN Ddi_Bdd_t * Ddi_AigAbstrVarsForInterpolant22(Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Vararray_t *aVars, Ddi_Vararray_t *bVars, Ddi_Vararray_t *auxVars, char *wfileName, unsigned char *enAbstr, int *pAbort);
EXTERN Ddi_Varset_t * Ddi_AigAbstrVarsIncremental(Ddi_Bdd_t *f, Ddi_Vararray_t *vars);
EXTERN Ddi_Bdd_t * Ddi_AigOptByEquiv(Ddi_Bdd_t *fAig, Ddi_Bddarray_t *representatives, Ddi_Bddarray_t *equalNodes);
EXTERN Ddi_Bdd_t * Ddi_AigOptByEquivFwd(Ddi_Bdd_t *fAig, Ddi_Bddarray_t *representatives, Ddi_Bddarray_t *equalNodes);
EXTERN Ddi_Bddarray_t * Ddi_AigarrayOptByEquiv(Ddi_Bddarray_t *fA, Ddi_Bddarray_t *representatives, Ddi_Bddarray_t *equalNodes);
EXTERN Ddi_Bddarray_t * Ddi_AigarrayOptByEquivFwd(Ddi_Bddarray_t *fA, Ddi_Bddarray_t *representatives, Ddi_Bddarray_t *equalNodes, int *checkEq, int checkStartId);
EXTERN Ddi_Bddarray_t * Ddi_AigarrayNodes(Ddi_Bddarray_t *fAigArray, int maxLevel);
EXTERN void Ddi_AigPartPropImplications(Ddi_Bdd_t *fAigPart, Ddi_Bdd_t *constr, int propUpOrDown);
EXTERN Ddi_Bddarray_t * Ddi_AigarraySortByLevel(Ddi_Bddarray_t *fAigArray);
EXTERN Ddi_Vararray_t * Ddi_AigDFSOrdVars(Ddi_Bdd_t *fAig);
EXTERN Ddi_Vararray_t * Ddi_AigarrayDFSOrdVars(Ddi_Bddarray_t *fAigArray);
EXTERN Ddi_Bdd_t * Ddi_BddMakeFromCU(Ddi_Mgr_t *mgr, DdNode *bdd);
EXTERN Ddi_BddNode * Ddi_BddToCU(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddMakeLiteral(Ddi_Var_t *v, int polarity);
EXTERN Ddi_Bdd_t * Ddi_BddMakeFromBaig(Ddi_Mgr_t *mgr, bAigEdge_t bAigNode);
EXTERN bAigEdge_t Ddi_BddToBaig(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddMakeLiteralAig(Ddi_Var_t *v, int polarity);
EXTERN Ddi_Bdd_t * Ddi_BddMakeLiteralAigOrBdd(Ddi_Var_t *v, int polarity, int doAig);
EXTERN Ddi_Bdd_t * Ddi_BddMakeConstAig(Ddi_Mgr_t *mgr, int value);
EXTERN Ddi_Bdd_t * Ddi_BddMakeConst(Ddi_Mgr_t *mgr, int value);
EXTERN Ddi_Bdd_t * Ddi_BddMakePartConjFromMono(Ddi_Bdd_t *mono);
EXTERN Ddi_Bdd_t * Ddi_BddMakePartConjVoid(Ddi_Mgr_t *mgr);
EXTERN Ddi_Bdd_t * Ddi_BddMakePartDisjVoid(Ddi_Mgr_t *mgr);
EXTERN Ddi_Bdd_t * Ddi_BddMakePartDisjFromMono(Ddi_Bdd_t *mono);
EXTERN Ddi_Bdd_t * Ddi_BddMakePartConjFromArray(Ddi_Bddarray_t *array);
EXTERN Ddi_Bdd_t * Ddi_BddMakePartDisjFromArray(Ddi_Bddarray_t *array);
EXTERN Ddi_Bdd_t * Ddi_BddSetPartConj(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddSetPartDisj(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddRelMakeFromArray(Ddi_Bddarray_t *Fa, Ddi_Vararray_t *Va);
EXTERN Ddi_Bdd_t * Ddi_BddRelMakeFromArrayFiltered (Ddi_Bddarray_t *Fa, Ddi_Vararray_t *Va, Ddi_Vararray_t *filter);
EXTERN Ddi_Bdd_t * Ddi_BddMiterMakeFromArray(Ddi_Bddarray_t *Fa, Ddi_Bddarray_t *Fb);
EXTERN Ddi_Bdd_t *Ddi_BddMakeEq (Ddi_Vararray_t *vars, Ddi_Bddarray_t *subst);
EXTERN Ddi_Bdd_t *Ddi_BddMakeCompose (Ddi_Bdd_t *f, Ddi_Vararray_t *refVars, Ddi_Vararray_t *vars, Ddi_Bddarray_t *subst, Ddi_Bdd_t *care, Ddi_Bdd_t *constr, Ddi_Bdd_t *cone);
EXTERN Ddi_Bdd_t * Ddi_BddMakePartConjFromCube(Ddi_Bdd_t *cube);
EXTERN Ddi_Bdd_t * Ddi_BddDup(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddCopy(Ddi_Mgr_t *ddm, Ddi_Bdd_t *old);
EXTERN Ddi_Bdd_t * Ddi_BddCopyRemapVars(Ddi_Mgr_t *ddm, Ddi_Bdd_t *old, Ddi_Vararray_t *varsOld, Ddi_Vararray_t *varsNew);
EXTERN int Ddi_BddSize(Ddi_Bdd_t *f);
EXTERN int Ddi_BddSizeWithBound(Ddi_Bdd_t *f, int bound);
EXTERN Ddi_Var_t * Ddi_BddTopVar(Ddi_Bdd_t *f);
Ddi_Var_t *Ddi_BddLitVar (Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddEvalFree(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN int Ddi_BddEqual(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN int Ddi_BddEqualSat(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN int Ddi_BddIsZero(Ddi_Bdd_t *f);
EXTERN int Ddi_BddIsOne(Ddi_Bdd_t *f);
EXTERN double Ddi_BddCountMinterm(Ddi_Bdd_t *f, int nvar);
EXTERN int Ddi_BddIncluded(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddNot(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddNotAcc(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddAnd(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddAndAcc(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddDiff(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddDiffAcc(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddNand(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddNandAcc(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddOr(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddOrAcc(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddNor(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddNorAcc(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddXor(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddXorAcc(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddXnor(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddXnorAcc(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddIte(Ddi_Bdd_t *f, Ddi_Bdd_t *g, Ddi_Bdd_t *h);
EXTERN Ddi_Bdd_t * Ddi_BddIteAcc(Ddi_Bdd_t *f, Ddi_Bdd_t *g, Ddi_Bdd_t *h);
EXTERN Ddi_Bdd_t * Ddi_BddExist(Ddi_Bdd_t *f, Ddi_Varset_t *vars);
EXTERN Ddi_Bdd_t * Ddi_BddExistAcc(Ddi_Bdd_t *f, Ddi_Varset_t *vars);
EXTERN Ddi_Bdd_t * Ddi_BddCubeExist(Ddi_Bdd_t *f, Ddi_Varset_t *vars);
EXTERN Ddi_Bdd_t * Ddi_BddCubeExistAcc(Ddi_Bdd_t *f, Ddi_Varset_t *vars);
EXTERN Ddi_Bdd_t * Ddi_BddExistProject(Ddi_Bdd_t *f, Ddi_Varset_t *vars);
EXTERN Ddi_Bdd_t * Ddi_BddCubeExistProject(Ddi_Bdd_t *f, Ddi_Varset_t *vars);
EXTERN Ddi_Bdd_t * Ddi_BddCubeExistProjectAcc(Ddi_Bdd_t *f, Ddi_Varset_t *vars);
EXTERN Ddi_Bdd_t * Ddi_BddExistProjectAcc(Ddi_Bdd_t *f, Ddi_Varset_t *vars);
EXTERN Ddi_Bdd_t * Ddi_BddForall(Ddi_Bdd_t *f, Ddi_Varset_t *vars);
EXTERN Ddi_Bdd_t * Ddi_BddForallAcc(Ddi_Bdd_t *f, Ddi_Varset_t *vars);
EXTERN Ddi_Bdd_t * Ddi_BddAndExist(Ddi_Bdd_t *f, Ddi_Bdd_t *g, Ddi_Varset_t *vars);
EXTERN Ddi_Bdd_t * Ddi_BddAndExistAcc(Ddi_Bdd_t *f, Ddi_Bdd_t *g, Ddi_Varset_t *vars);
EXTERN Ddi_Bdd_t * Ddi_BddExistOverApprox(Ddi_Bdd_t *f, Ddi_Varset_t *sm);
EXTERN Ddi_Bdd_t * Ddi_BddExistOverApproxAcc(Ddi_Bdd_t *f, Ddi_Varset_t *sm);
EXTERN Ddi_Bdd_t * Ddi_BddCofactor(Ddi_Bdd_t *f, Ddi_Var_t *v, int phase);
EXTERN Ddi_Bdd_t * Ddi_BddCofactorAcc(Ddi_Bdd_t *f, Ddi_Var_t *v, int phase);
EXTERN Ddi_Bdd_t *Ddi_BddVararrayCofactor(Ddi_Bdd_t  *f, Ddi_Vararray_t  *vA, int phase);
EXTERN Ddi_Bdd_t *Ddi_BddVararrayCofactorAcc(Ddi_Bdd_t  *f, Ddi_Vararray_t  *vA, int phase);
EXTERN Ddi_Bdd_t * Ddi_BddConstrain(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddConstrainAcc(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddRestrict(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddRestrictAcc(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddCofexist(Ddi_Bdd_t *f, Ddi_Bdd_t *g, Ddi_Varset_t *smooth);
EXTERN Ddi_Bdd_t * Ddi_BddCproject(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddCprojectAcc(Ddi_Bdd_t *f, Ddi_Bdd_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddSwapVars(Ddi_Bdd_t *f, Ddi_Vararray_t *x, Ddi_Vararray_t *y);
EXTERN Ddi_Bdd_t * Ddi_BddSwapVarsAcc(Ddi_Bdd_t *f, Ddi_Vararray_t *x, Ddi_Vararray_t *y);
EXTERN Ddi_Bdd_t * Ddi_BddSubstVars(Ddi_Bdd_t *f, Ddi_Vararray_t *x, Ddi_Vararray_t *y);
EXTERN Ddi_Bdd_t * Ddi_BddSubstVarsAcc(Ddi_Bdd_t *f, Ddi_Vararray_t *x, Ddi_Vararray_t *y);
EXTERN Ddi_Bdd_t * Ddi_BddCompose(Ddi_Bdd_t *f, Ddi_Vararray_t *x, Ddi_Bddarray_t *g);
EXTERN Ddi_Bdd_t * Ddi_BddComposeAcc(Ddi_Bdd_t *f, Ddi_Vararray_t *x, Ddi_Bddarray_t *g);
EXTERN Ddi_Vararray_t * Ddi_BddNewVarsAcc(Ddi_Bdd_t *f, Ddi_Vararray_t *x, char *prefix, char *suffix, int useBddVars);
EXTERN int Ddi_BddIsCube(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddPickOneCube(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddPickOneCubeAcc(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddPickOneMinterm(Ddi_Bdd_t *f, Ddi_Varset_t *vars);
EXTERN Ddi_Bdd_t * Ddi_BddPickOneMintermAcc(Ddi_Bdd_t *f, Ddi_Varset_t *vars);
EXTERN Ddi_Bdd_t * Ddi_BddCover (Ddi_Bdd_t  *onset, Ddi_Bdd_t  *offset, int *np);
EXTERN Ddi_Bdd_t * Ddi_BddImpliedCube(Ddi_Bdd_t *f, Ddi_Varset_t *vars);
EXTERN Ddi_Varset_t * Ddi_BddSupp(Ddi_Bdd_t *f);
EXTERN Ddi_Vararray_t * Ddi_BddSuppVararray(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddSuppAttach(Ddi_Bdd_t *f);
EXTERN Ddi_Varset_t * Ddi_BddSuppRead(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddSuppDetach(Ddi_Bdd_t *f);
EXTERN int Ddi_BddIsMono(Ddi_Bdd_t *f);
EXTERN int Ddi_BddIsMeta(Ddi_Bdd_t *f);
EXTERN int Ddi_BddIsAig(Ddi_Bdd_t *f);
EXTERN int Ddi_BddIsLiteralAig(Ddi_Bdd_t *f);
EXTERN int Ddi_BddIsPartConj(Ddi_Bdd_t *f);
EXTERN int Ddi_BddIsPartDisj(Ddi_Bdd_t *f);
EXTERN int Ddi_BddPartNum(Ddi_Bdd_t * f);
EXTERN Ddi_Bdd_t * Ddi_BddPartRead(Ddi_Bdd_t * f, int i);
EXTERN Ddi_Bdd_t * Ddi_BddPartWrite(Ddi_Bdd_t *f, int i, Ddi_Bdd_t *newp);
EXTERN Ddi_Bdd_t * Ddi_BddPartExtract(Ddi_Bdd_t * f, int i);
EXTERN Ddi_Bdd_t * Ddi_BddPartQuickExtract(Ddi_Bdd_t * f, int i);
EXTERN Ddi_Bdd_t * Ddi_BddPartSwap(Ddi_Bdd_t * f, int i, int j);
EXTERN Ddi_Bdd_t * Ddi_BddPartSortBySizeAcc(Ddi_Bdd_t * f, int increasing);
EXTERN Ddi_Bdd_t * Ddi_BddPartSortByTopVarAcc(Ddi_Bdd_t * f, int increasing);
EXTERN Ddi_Bdd_t * Ddi_BddPartInsert(Ddi_Bdd_t *f, int i, Ddi_Bdd_t *newp);
EXTERN Ddi_Bdd_t * Ddi_BddPartInsertLast(Ddi_Bdd_t *f, Ddi_Bdd_t *newp);
EXTERN Ddi_Bdd_t * Ddi_BddPartInsertLastNoDup(Ddi_Bdd_t *f, Ddi_Bdd_t *newp);
EXTERN void Ddi_BddPartRemove(Ddi_Bdd_t * f, int i);
EXTERN void Ddi_BddPartQuickRemove(Ddi_Bdd_t * f, int i);
EXTERN Ddi_Bdd_t * Ddi_BddMakeMono(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddSetMono(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddMakeAig(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddSetAig(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddMakeFlattened(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddSetFlattened(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddMakeClustered(Ddi_Bdd_t *f, int threshold);
EXTERN Ddi_Bdd_t * Ddi_BddSetClustered(Ddi_Bdd_t *f, int threshold);
EXTERN void Ddi_BddWriteMark (Ddi_Bdd_t *f, int val);
EXTERN int Ddi_BddReadMark (Ddi_Bdd_t *f);
EXTERN Ddi_Vararray_t *Ddi_BddReadEqVars (Ddi_Bdd_t *f);
EXTERN Ddi_Bddarray_t *Ddi_BddReadEqSubst (Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t *Ddi_BddReadComposeF (Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t *Ddi_BddReadComposeCare (Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t *Ddi_BddReadComposeConstr (Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t *Ddi_BddReadComposeCone (Ddi_Bdd_t *f);
EXTERN Ddi_Vararray_t *Ddi_BddReadComposeRefVars (Ddi_Bdd_t *f);
EXTERN Ddi_Vararray_t *Ddi_BddReadComposeVars (Ddi_Bdd_t *f);
EXTERN Ddi_Bddarray_t *Ddi_BddReadComposeSubst (Ddi_Bdd_t *f);
EXTERN void Ddi_BddPrint(Ddi_Bdd_t *f, FILE *fp);
EXTERN void Ddi_BddPrintStats(Ddi_Bdd_t *f, FILE *fp);
EXTERN int Ddi_BddPrintCubeToString(Ddi_Bdd_t *f, Ddi_Varset_t *vars, char *string);
EXTERN int Ddi_BddPrintCubes(Ddi_Bdd_t *f, Ddi_Varset_t *vars, int cubeNumberMax, int formatPla, char *filename, FILE *fp);
EXTERN int Ddi_BddStore(Ddi_Bdd_t *f, char *ddname, char mode, char *filename, FILE *fp);
EXTERN Ddi_Bdd_t * Ddi_BddLoad(Ddi_BddMgr *dd, int varmatchmode, char mode, char *filename, FILE *fp);
EXTERN int Ddi_BddStoreCnf(Ddi_Bdd_t *f, Dddmp_DecompCnfStoreType mode, int noHeader, char **vnames, int *vbddIds, int *vauxIds, int *vcnfIds, int idInitial, int edgeInMaxTh, int pathLengthMaxTh, char *filename, FILE *fp, int *clauseNPtr, int *varNewNPtr);
EXTERN int Ddi_BddStoreBlif(Ddi_Bdd_t *f, char **inputNames, char *outputName, char *modelName, char *fname, FILE *fp);
EXTERN int Ddi_BddStorePrefix(Ddi_Bdd_t *f, char **inputNames, char *outputName, char *modelName, char *fname, FILE *fp);
EXTERN int Ddi_BddStoreSmv(Ddi_Bdd_t *f, char **inputNames, char *outputName, char *modelName, char *fname, FILE *fp);
EXTERN Ddi_Bdd_t * Ddi_BddDenseSet(Ddi_DenseMethod_e method, Ddi_Bdd_t *f, int threshold, int safe, int quality, double hardlimit);
EXTERN Ddi_Bdd_t * Ddi_BddClippingAndAbstract(Ddi_Bdd_t *f, Ddi_Bdd_t *g, Ddi_Varset_t *smoothV, int recurThreshold, int direction);
EXTERN Ddi_Bdd_t * Ddi_BddImplications(Ddi_Bdd_t *f, Ddi_Bdd_t *care, char *eqFileName, char *implFileName, char *clkStateVarName);
EXTERN Ddi_Bdd_t * Ddi_BddOverapprConj(Ddi_Bdd_t *f, Ddi_Bdd_t *care, int nPart, int th);
EXTERN Ddi_Varsetarray_t * Ddi_BddTightSupports(Ddi_Bdd_t *f);
EXTERN Ddi_Bddarray_t * Ddi_BddarrayAlloc(Ddi_Mgr_t *mgr, int length);
EXTERN Ddi_Bddarray_t * Ddi_BddarrayMakeFromCU(Ddi_Mgr_t *mgr, DdNode **array, int n);
EXTERN DdNode ** Ddi_BddarrayToCU(Ddi_Bddarray_t *array);
EXTERN Ddi_Bddarray_t * Ddi_BddarrayMakeFromBddPart(Ddi_Bdd_t *part);
EXTERN Ddi_Bddarray_t * Ddi_BddarrayMakeFromBddRoots(Ddi_Bdd_t *part);
EXTERN int Ddi_BddarrayNum(Ddi_Bddarray_t *array);
EXTERN void Ddi_BddarrayWrite(Ddi_Bddarray_t *array, int pos, Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddarrayRead(Ddi_Bddarray_t *array, int i);
EXTERN void Ddi_BddarrayClear(Ddi_Bddarray_t *array, int pos);
EXTERN void Ddi_BddarrayInsert(Ddi_Bddarray_t *array, int pos, Ddi_Bdd_t *f);
EXTERN void Ddi_BddarrayInsertLast(Ddi_Bddarray_t *array, Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddarrayExtract(Ddi_Bddarray_t *array, int i);
EXTERN Ddi_Bdd_t * Ddi_BddarrayQuickExtract(Ddi_Bddarray_t *array, int i);
EXTERN void Ddi_BddarrayRemove(Ddi_Bddarray_t *array, int pos);
EXTERN void Ddi_BddarrayRemoveNull(Ddi_Bddarray_t *array);
EXTERN void Ddi_BddarrayQuickRemove(Ddi_Bddarray_t *array, int pos);
EXTERN Ddi_Bddarray_t * Ddi_BddarrayDup(Ddi_Bddarray_t *old);
EXTERN Ddi_Bddarray_t * Ddi_BddarrayCopy(Ddi_BddMgr *ddm, Ddi_Bddarray_t *old);
EXTERN void Ddi_BddarrayAppend(Ddi_Bddarray_t *array1, Ddi_Bddarray_t *array2);
EXTERN int Ddi_BddarraySize(Ddi_Bddarray_t *array);
EXTERN int Ddi_BddarrayStore(Ddi_Bddarray_t *array, char *ddname, char **vnames, char **rnames, int *vauxids, int mode, char *fname, FILE *fp);
EXTERN int Ddi_BddarrayStoreCnf(Ddi_Bddarray_t *array, Dddmp_DecompCnfStoreType mode, int noHeader, char **vnames, int *vbddids, int *vauxids, int *vcnfids, int idInitial, int edgeInMaxTh, int pathLengthMaxTh, char *fname, FILE *fp, int *clauseNPtr, int *varNewNPtr);
EXTERN int Ddi_BddarrayStoreBlif(Ddi_Bddarray_t *array, char **inputNames, char **outputNames, char *modelName, char *fname, FILE *fp);
EXTERN int Ddi_BddarrayStorePrefix(Ddi_Bddarray_t *array, char **inputNames, char **outputNames, char *modelName, char *fname, FILE *fp);
EXTERN int Ddi_BddarrayStoreSmv(Ddi_Bddarray_t *array, char **inputNames, char **outputNames, char *modelName, char *fname, FILE *fp);
EXTERN Ddi_Bddarray_t * Ddi_BddarrayLoad(Ddi_BddMgr *dd, char **vnames, int *vauxids, int mode, char *file, FILE *fp);
EXTERN Ddi_Varset_t * Ddi_BddarraySupp(Ddi_Bddarray_t *array);
EXTERN Ddi_Vararray_t * Ddi_BddarraySuppVararray(Ddi_Bddarray_t *array);
EXTERN Ddi_Varset_t ** Ddi_BddarraySuppArray(Ddi_Bddarray_t *fArray);
EXTERN Ddi_Bddarray_t * Ddi_BddarrayMakeLiterals(Ddi_Vararray_t *vA, int polarity);
EXTERN Ddi_Bddarray_t * Ddi_BddarrayMakeLiteralsAig(Ddi_Vararray_t *vA, int polarity);
EXTERN Ddi_Bddarray_t * Ddi_BddarrayMakeLiteralsAigOrBdd(Ddi_Vararray_t *vA, int polarity, int doAig);
EXTERN Ddi_Bddarray_t *Ddi_BddarrayMakeConst(Ddi_Mgr_t *ddm, int n, int constVal);
EXTERN Ddi_Bddarray_t *Ddi_BddarrayMakeConstAig(Ddi_Mgr_t *ddm, int n, int constVal);
EXTERN Ddi_Bddarray_t * Ddi_BddarrayCompose(Ddi_Bddarray_t *f, Ddi_Vararray_t *x, Ddi_Bddarray_t *g);
EXTERN Ddi_Bddarray_t * Ddi_BddarrayComposeAcc(Ddi_Bddarray_t *f, Ddi_Vararray_t *x, Ddi_Bddarray_t *g);
EXTERN Ddi_Bddarray_t *Ddi_BddarraySubstVars(Ddi_Bddarray_t  *fA, Ddi_Vararray_t *x,  Ddi_Vararray_t *y);
EXTERN Ddi_Bddarray_t *Ddi_BddarraySubstVarsAcc(Ddi_Bddarray_t  *fA, Ddi_Vararray_t *x,  Ddi_Vararray_t *y);
EXTERN Ddi_Bddarray_t *Ddi_BddarraySwapVars(Ddi_Bddarray_t  *fA, Ddi_Vararray_t *x,  Ddi_Vararray_t *y);
EXTERN Ddi_Bddarray_t *Ddi_BddarraySwapVarsAcc(Ddi_Bddarray_t  *fA, Ddi_Vararray_t *x,  Ddi_Vararray_t *y);
EXTERN Ddi_Vararray_t * Ddi_BddarrayNewVarsAcc(Ddi_Bddarray_t *fA, Ddi_Vararray_t *x, char *prefix, char *suffix, int useBddVars);
EXTERN Ddi_Bddarray_t *Ddi_BddarrayRangeMakeFromCube (Ddi_Bdd_t *cube, Ddi_Vararray_t *Va);
EXTERN Ddi_Bddarray_t *Ddi_BddarraySortBySize (Ddi_Bddarray_t  *fA, int increasing);
EXTERN Ddi_Bddarray_t *Ddi_BddarraySortBySizeAcc (Ddi_Bddarray_t  *fA, int increasing);
EXTERN Ddi_Bddarray_t *Ddi_BddarrayCofactor (Ddi_Bddarray_t  *fA, Ddi_Var_t  *v, int phase);
EXTERN Ddi_Bddarray_t *Ddi_BddarrayCofactorAcc (Ddi_Bddarray_t  *fA, Ddi_Var_t  *v, int phase);
EXTERN Ddi_Bddarray_t *Ddi_BddarrayVararrayCofactor(Ddi_Bddarray_t  *fA, Ddi_Vararray_t  *vA, int phase);
EXTERN Ddi_Bddarray_t *Ddi_BddarrayVararrayCofactorAcc(Ddi_Bddarray_t  *fA, Ddi_Vararray_t  *vA, int phase);
EXTERN Ddi_Bdd_t * Ddi_BddMultiwayRecursiveAndExist(Ddi_Bdd_t *FPart, Ddi_Varset_t *smoothV, Ddi_Bdd_t *constrain, int threshold);
EXTERN Ddi_Bdd_t * Ddi_BddMultiwayRecursiveAndExistOver(Ddi_Bdd_t *FPart, Ddi_Varset_t *smoothV, Ddi_Bdd_t *constrain, Ddi_Bdd_t *overApprTerm, int threshold);
EXTERN Ddi_Bdd_t * Ddi_BddMultiwayRecursiveAndExistOverWithFrozenCheck(Ddi_Bdd_t *FPart, Ddi_Varset_t *smoothV, Ddi_Bdd_t *from, Ddi_Vararray_t *psv, Ddi_Vararray_t *nsv, Ddi_Bdd_t *constrain, Ddi_Bdd_t *overApprTerm, int threshold, int fwd);
EXTERN Ddi_Bdd_t * Ddi_BddDynamicAbstract(Ddi_Bdd_t *f, int maxSize, int maxAbstract, int exist_or_forall);
EXTERN Ddi_Bdd_t * Ddi_BddDynamicAbstractAcc(Ddi_Bdd_t *f, int maxSize, int maxAbstract, int over_under_approx);
EXTERN Ddi_Expr_t * Ddi_ExprMakeFromBdd(Ddi_Bdd_t *f);
EXTERN Ddi_Expr_t * Ddi_ExprSetBdd(Ddi_Expr_t *e);
EXTERN Ddi_Bdd_t * Ddi_ExprToBdd(Ddi_Expr_t *e);
EXTERN Ddi_Expr_t * Ddi_ExprMakeFromString(Ddi_Mgr_t *mgr, char *s);
EXTERN char * Ddi_ExprToString(Ddi_Expr_t *e);
EXTERN Ddi_Expr_t * Ddi_ExprCtlMake(Ddi_Mgr_t *mgr, int opcode, Ddi_Expr_t *op1, Ddi_Expr_t *op2, Ddi_Expr_t *op3);
EXTERN Ddi_Expr_t * Ddi_ExprBoolMake(Ddi_Mgr_t *mgr, Ddi_Expr_t *op1, Ddi_Expr_t *op2);
EXTERN Ddi_Expr_t * Ddi_ExprWriteSub(Ddi_Expr_t *e, int pos, Ddi_Expr_t *op);
EXTERN int Ddi_ExprSubNum(Ddi_Expr_t *e);
EXTERN Ddi_Expr_t * Ddi_ExprReadSub(Ddi_Expr_t *e, int i);
EXTERN int Ddi_ExprReadOpcode(Ddi_Expr_t *e);
EXTERN int Ddi_ExprIsTerminal(Ddi_Expr_t *e);
EXTERN Ddi_Expr_t * Ddi_ExprLoad(Ddi_BddMgr *dd, char *filename, FILE *fp);
EXTERN Ddi_Expr_t * Ddi_ExprDup(Ddi_Expr_t *f);
EXTERN void Ddi_ExprPrint(Ddi_Expr_t *f, FILE *fp);
EXTERN Ddi_Generic_t * Ddi_GenericOp(Ddi_OpCode_e opcode, Ddi_Generic_t *f, Ddi_Generic_t *g, Ddi_Generic_t *h);
EXTERN Ddi_Generic_t * Ddi_GenericOpAcc(Ddi_OpCode_e opcode, Ddi_Generic_t *f, Ddi_Generic_t *g, Ddi_Generic_t *h);
EXTERN Ddi_Generic_t * Ddi_GenericDup(Ddi_Generic_t *f);
EXTERN void Ddi_GenericLock(Ddi_Generic_t *f);
EXTERN void Ddi_GenericUnlock(Ddi_Generic_t *f);
EXTERN void Ddi_GenericFree(Ddi_Generic_t *f);
EXTERN void Ddi_GenericSetName(Ddi_Generic_t *f, char *name);
EXTERN Ddi_Code_e Ddi_GenericReadCode(Ddi_Generic_t *f);
EXTERN Ddi_Mgr_t * Ddi_GenericReadMgr(Ddi_Generic_t * f);
EXTERN char * Ddi_GenericReadName(Ddi_Generic_t * f);
EXTERN int Ddi_MetaActive(Ddi_Mgr_t *ddm);
EXTERN void Ddi_MetaInit(Ddi_Mgr_t *ddm, Ddi_Meta_Method_e method, Ddi_Bdd_t *ref, Ddi_Varset_t *firstGroup, Ddi_Vararray_t *ps, Ddi_Vararray_t *ns, int sizeMin);
EXTERN void Ddi_MetaQuit(Ddi_Mgr_t *ddm);
EXTERN Ddi_Bdd_t * Ddi_BddMakeMeta(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddSetMeta(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddMetaBfvSetConj(Ddi_Bdd_t *f);
EXTERN Ddi_Bddarray_t * Ddi_BddarrayMakeMeta(Ddi_Bddarray_t *f);
EXTERN Ddi_Bddarray_t * Ddi_BddArraySetMeta(Ddi_Bddarray_t *f);
EXTERN Ddi_Bdd_t * Ddi_AigMakeFromMeta(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_AigFromMeta(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddMakeFromMeta(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddFromMeta(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddSetPartConjFromMeta(Ddi_Bdd_t *f);
EXTERN Ddi_Bdd_t * Ddi_BddMakePartConjFromMeta(Ddi_Bdd_t *f);
EXTERN Ddi_Mgr_t * Ddi_MgrInit(char *ddiName, DdManager *CUMgr, unsigned int nvar, unsigned int numSlots, unsigned int cacheSize, unsigned long cuddMemoryLimit, long int memoryLimit, long int timeLimit);
EXTERN int Ddi_MgrConsistencyCheck(Ddi_Mgr_t *ddm);
EXTERN int Ddi_MgrCheckExtRef(Ddi_Mgr_t *ddm, int n);
EXTERN void Ddi_MgrPrintExtRef(Ddi_Mgr_t *ddm, int minNodeId);
EXTERN void Ddi_MgrFreeExtRef(Ddi_Mgr_t *ddm, int minNodeId);
EXTERN void Ddi_MgrPrintBySize(Ddi_Mgr_t *ddm, int minSize, int minNodeId);
EXTERN void Ddi_MgrUpdate(Ddi_Mgr_t *ddm);
EXTERN void Ddi_MgrQuit(Ddi_Mgr_t *dd);
EXTERN Ddi_Mgr_t * Ddi_MgrDup(Ddi_Mgr_t *dd);
EXTERN void Ddi_MgrShuffle(Ddi_Mgr_t *dd, int *sortedIds, int nids);
EXTERN void Ddi_MgrAlign(Ddi_Mgr_t *dd, Ddi_Mgr_t *ddRef);
EXTERN void Ddi_MgrCreateGroups2(Ddi_Mgr_t *dd, Ddi_Vararray_t *vfix, Ddi_Vararray_t *vmov);
EXTERN int Ddi_MgrOrdWrite(Ddi_Mgr_t *dd, char *filename, FILE *fp, Pdtutil_VariableOrderFormat_e ordFileFormat);
EXTERN int Ddi_MgrReadOrdNamesAuxids(Ddi_Mgr_t *dd, char *filename, FILE *fp, Pdtutil_VariableOrderFormat_e ordFileFormat);
EXTERN void Ddi_MgrSiftEnable(Ddi_Mgr_t *dd, int method);
EXTERN void Ddi_MgrSiftDisable(Ddi_Mgr_t *dd);
EXTERN void Ddi_MgrSiftSuspend(Ddi_Mgr_t *dd);
EXTERN void Ddi_MgrAbortOnSiftSuspend(Ddi_Mgr_t *dd);
EXTERN void Ddi_MgrSiftResume(Ddi_Mgr_t *dd);
EXTERN void Ddi_MgrAbortOnSiftResume(Ddi_Mgr_t *dd);
EXTERN void Ddi_MgrAbortOnSiftEnable(Ddi_Mgr_t *dd);
EXTERN void Ddi_MgrAbortOnSiftEnableWithThresh(Ddi_Mgr_t *dd, int deltaNodes, int allowSift);
EXTERN void Ddi_MgrAbortOnSiftDisable(Ddi_Mgr_t *dd);
EXTERN int Ddi_MgrOperation(Ddi_Mgr_t **ddMgrP, char *string, Pdtutil_MgrOp_t operationFlag, void **voidPointer, Pdtutil_MgrRet_t *returnFlagP);
EXTERN void Ddi_MgrPrintStats(Ddi_Mgr_t *dd);
EXTERN int Ddi_MgrReadNumVars(Ddi_Mgr_t *dd);
EXTERN DdManager * Ddi_MgrReadMgrCU(Ddi_Mgr_t *dd);
EXTERN bAig_Manager_t *Ddi_MgrReadMgrBaig(Ddi_Mgr_t *dd);
EXTERN Ddi_Bdd_t * Ddi_MgrReadOne(Ddi_Mgr_t *dd);
EXTERN Ddi_Bdd_t * Ddi_MgrReadZero(Ddi_Mgr_t *dd);
EXTERN int Ddi_MgrReadCurrNodeId(Ddi_Mgr_t *dd);
EXTERN void Ddi_MgrSetTracedId(Ddi_Mgr_t *dd, int id);
EXTERN void Ddi_MgrSetExistClustThresh(Ddi_Mgr_t *dd, int th);
EXTERN int Ddi_MgrReadExistClustThresh(Ddi_Mgr_t *dd);
EXTERN void Ddi_MgrSetExistOptClustThresh(Ddi_Mgr_t *dd, int th);
EXTERN int Ddi_MgrReadExistOptClustThresh(Ddi_Mgr_t *dd);
EXTERN void Ddi_MgrSetExistOptLevel(Ddi_Mgr_t *dd, int lev);
EXTERN void Ddi_MgrSetExistDisjPartTh(Ddi_Mgr_t *dd, int th);
EXTERN int Ddi_MgrReadExistOptLevel(Ddi_Mgr_t *dd);
EXTERN int Ddi_MgrReadExistDisjPartTh(Ddi_Mgr_t *dd);
EXTERN char** Ddi_MgrReadVarnames(Ddi_Mgr_t *dd);
EXTERN int * Ddi_MgrReadVarauxids(Ddi_Mgr_t *dd);
EXTERN bAigEdge_t * Ddi_MgrReadVarBaigIds(Ddi_Mgr_t *dd);
EXTERN int * Ddi_MgrReadVarIdsFromCudd(Ddi_Mgr_t *dd);
EXTERN int Ddi_MgrReadExtRef(Ddi_Mgr_t *dd);
EXTERN int Ddi_MgrReadExtBddRef(Ddi_Mgr_t *dd);
EXTERN int Ddi_MgrReadExtBddarrayRef(Ddi_Mgr_t *dd);
EXTERN int Ddi_MgrReadExtVarsetRef(Ddi_Mgr_t *dd);
EXTERN int Ddi_MgrReadSiftThresh(Ddi_Mgr_t *dd);
EXTERN int Ddi_MgrReadSiftMaxThresh(Ddi_Mgr_t *dd);
EXTERN int Ddi_MgrReadSiftMaxCalls(Ddi_Mgr_t *dd);
EXTERN long Ddi_MgrReadSiftLastTime(Ddi_Mgr_t *dd);
EXTERN long Ddi_MgrReadSiftTotTime(Ddi_Mgr_t *dd);
EXTERN char * Ddi_MgrReadName(Ddi_Mgr_t *dd);
EXTERN void Ddi_MgrSetName(Ddi_Mgr_t *dd, char *ddiName);
EXTERN Pdtutil_VerbLevel_e Ddi_MgrReadVerbosity(Ddi_Mgr_t *ddiMgr);
EXTERN void Ddi_MgrSetVerbosity(Ddi_Mgr_t *ddiMgr, Pdtutil_VerbLevel_e verbosity);
EXTERN void Ddi_MgrSetOptionList(Ddi_Mgr_t *ddiMgr, Pdtutil_OptList_t *pkgOpt);
EXTERN void Ddi_MgrSetOptionItem(Ddi_Mgr_t *ddiMgr, Pdtutil_OptItem_t optItem);
EXTERN void Ddi_MgrReadOption(Ddi_Mgr_t *ddiMgr, Pdt_OptTagDdi_e ddiOpt, void *optRet);
EXTERN FILE *Ddi_MgrReadStdout(Ddi_Mgr_t *ddiMgr);
EXTERN void Ddi_MgrSetStdout(Ddi_Mgr_t *ddiMgr, FILE *stdout);
EXTERN int Ddi_MgrReadPeakProdLocal(Ddi_Mgr_t *ddiMgr);
EXTERN int Ddi_MgrReadSiftRunNum(Ddi_Mgr_t *ddiMgr);
EXTERN int Ddi_MgrReadPeakProdGlobal(Ddi_Mgr_t *ddiMgr);
EXTERN void Ddi_MgrPeakProdLocalReset(Ddi_Mgr_t *ddiMgr);
EXTERN void Ddi_MgrPeakProdUpdate(Ddi_Mgr_t *ddiMgr, int size);
EXTERN void Ddi_MgrSetMgrCU(Ddi_Mgr_t *dd, DdManager *m);
EXTERN void Ddi_MgrSetOne(Ddi_Mgr_t *dd, Ddi_Bdd_t *one);
EXTERN void Ddi_MgrSetZero(Ddi_Mgr_t *dd, Ddi_Bdd_t *zero);
EXTERN void Ddi_MgrSetVarnames(Ddi_Mgr_t *dd, char **vn);
EXTERN void Ddi_MgrSetVarauxids(Ddi_Mgr_t *dd, int *va);
EXTERN void Ddi_MgrSetVarBaigIds(Ddi_Mgr_t *dd, bAigEdge_t *va);
EXTERN void Ddi_MgrSetVarIdsFromCudd(Ddi_Mgr_t *dd, int *va);
EXTERN void Ddi_MgrSetSiftThresh(Ddi_Mgr_t *dd, int th);
EXTERN void Ddi_MgrSetSiftMaxThresh(Ddi_Mgr_t *dd, int th);
EXTERN void Ddi_MgrSetSiftMaxCalls(Ddi_Mgr_t *dd, int n);
EXTERN unsigned int Ddi_MgrReadCacheSlots(Ddi_Mgr_t *dd);
EXTERN double Ddi_MgrReadCacheLookUps(Ddi_Mgr_t *dd);
EXTERN double Ddi_MgrReadCacheHits(Ddi_Mgr_t *dd);
EXTERN unsigned int Ddi_MgrReadMinHit(Ddi_Mgr_t *dd);
EXTERN unsigned int Ddi_MgrReadMaxCacheHard(Ddi_Mgr_t *dd);
EXTERN unsigned int Ddi_MgrReadMaxCache(Ddi_Mgr_t* dd);
EXTERN long int Ddi_MgrReadMetaMaxSmooth(Ddi_Mgr_t *ddMgr);
EXTERN void Ddi_MgrSetMetaMaxSmooth(Ddi_Mgr_t *ddMgr, long int maxSmooth);
EXTERN float Ddi_MgrReadAigLazyRate(Ddi_Mgr_t *ddMgr);
EXTERN int Ddi_MgrReadAigSatTimeout(Ddi_Mgr_t *ddMgr);
EXTERN int Ddi_MgrReadAigSatTimeLimit(Ddi_Mgr_t *ddMgr);
EXTERN int Ddi_MgrReadAigSatIncremental(Ddi_Mgr_t *ddMgr);
EXTERN int Ddi_MgrReadAigSatVarActivity(Ddi_Mgr_t *ddMgr);
EXTERN int Ddi_MgrReadAigSatIncrByRefinement(Ddi_Mgr_t *ddMgr);
EXTERN int Ddi_MgrReadAigMaxAllSolutionIter(Ddi_Mgr_t *ddMgr);
EXTERN void Ddi_MgrSetAigLazyRate(Ddi_Mgr_t *ddMgr, float lazyRate);
EXTERN void Ddi_MgrSetAigSatTimeout(Ddi_Mgr_t *ddMgr, int satTimeout);
EXTERN void Ddi_MgrSetAigSatTimeLimit(Ddi_Mgr_t *ddMgr, int satTimeLimit);
EXTERN void Ddi_MgrSetAigSatIncremental(Ddi_Mgr_t *ddMgr, int satIncremental);
EXTERN void Ddi_MgrSetAigSatVarActivity(Ddi_Mgr_t *ddMgr, int satVarActivity);
EXTERN void Ddi_MgrSetAigSatIncrByRefinement(Ddi_Mgr_t *ddMgr, int satIncrByRefinement);
EXTERN void Ddi_MgrSetAigMaxAllSolutionIter(Ddi_Mgr_t *ddMgr, int maxAllSolutionIter);
EXTERN int Ddi_MgrReadAigAbcOptLevel(Ddi_Mgr_t *ddMgr);
EXTERN void Ddi_MgrSetAigAbcOptLevel(Ddi_Mgr_t *ddMgr, int abcOptLevel);
EXTERN int Ddi_MgrReadAigCnfLevel(Ddi_Mgr_t *ddMgr);
EXTERN void Ddi_MgrSetAigCnfLevel(Ddi_Mgr_t *ddMgr, int aigCnfLevel);
EXTERN int Ddi_MgrReadAigBddOptLevel(Ddi_Mgr_t *ddMgr);
EXTERN void Ddi_MgrSetAigBddOptLevel(Ddi_Mgr_t *ddMgr, int bddOptLevel);
EXTERN int Ddi_MgrReadAigRedRemLevel(Ddi_Mgr_t *ddMgr);
EXTERN int Ddi_MgrReadAigRedRemCnt(Ddi_Mgr_t *ddMgr);
EXTERN int Ddi_MgrReadAigItpPartTh(Ddi_Mgr_t *ddMgr);
EXTERN int Ddi_MgrReadAigDynAbstrStrategy(Ddi_Mgr_t *ddMgr);
EXTERN int Ddi_MgrReadAigItpAbc(Ddi_Mgr_t *ddMgr);
EXTERN int Ddi_MgrReadAigItpCore(Ddi_Mgr_t *ddMgr);
EXTERN int Ddi_MgrReadAigItpMem(Ddi_Mgr_t *ddMgr);
EXTERN int Ddi_MgrReadAigItpOpt(Ddi_Mgr_t *ddMgr);
EXTERN char *Ddi_MgrReadAigItpStore(Ddi_Mgr_t *ddMgr);
EXTERN int Ddi_MgrReadAigTernaryAbstrStrategy(Ddi_Mgr_t *ddMgr);
EXTERN int Ddi_MgrReadAigRedRemMaxCnt(Ddi_Mgr_t *ddMgr);
EXTERN void Ddi_MgrSetAigRedRemLevel(Ddi_Mgr_t *ddMgr, int redRemLevel);
EXTERN void Ddi_MgrSetAigDynAbstrStrategy(Ddi_Mgr_t *ddMgr, int dynAbstrStrategy);
EXTERN void Ddi_MgrSetAigItpAbc(Ddi_Mgr_t *ddMgr, int itpAbc);
EXTERN void Ddi_MgrSetAigItpCore(Ddi_Mgr_t *ddMgr, int itpCore);
EXTERN void Ddi_MgrSetAigItpMem(Ddi_Mgr_t *ddMgr, int itpMem);
EXTERN void Ddi_MgrSetAigItpMap(Ddi_Mgr_t *ddMgr, int itpMap);
EXTERN void Ddi_MgrSetAigItpOpt(Ddi_Mgr_t *ddMgr, int itpOpt);
EXTERN void Ddi_MgrSetAigItpStore(Ddi_Mgr_t *ddMgr, char *itpStore);
EXTERN void Ddi_MgrSetAigItpCompute(Ddi_Mgr_t *ddMgr, int itpCompute);
EXTERN void Ddi_MgrSetAigItpLoad(Ddi_Mgr_t *ddMgr, int itpLoad);
EXTERN void Ddi_MgrSetAigTernaryAbstrStrategy(Ddi_Mgr_t *ddMgr, int ternaryAbstrStrategy);
EXTERN void Ddi_MgrSetAigRedRemCnt(Ddi_Mgr_t *ddMgr, int redRemCnt);
EXTERN void Ddi_MgrSetAigItpPartTh(Ddi_Mgr_t *ddMgr, int itpPartTh);
EXTERN void Ddi_MgrSetAigRedRemMaxCnt(Ddi_Mgr_t *ddMgr, int redRemMaxCnt);
EXTERN void Ddi_MgrPrintAllocStats(Ddi_Mgr_t *ddm, FILE *fp);
EXTERN Ddi_Generic_t * Ddi_NodeFromName(Ddi_Mgr_t *ddm, char *name);
EXTERN long int Ddi_MgrReadMemoryLimit(Ddi_Mgr_t *ddMgr);
EXTERN void Ddi_MgrSetMemoryLimit(Ddi_Mgr_t *ddMgr, long int memoryLimit);
EXTERN long int Ddi_MgrReadTimeLimit(Ddi_Mgr_t *ddMgr);
EXTERN void Ddi_MgrSetTimeLimit(Ddi_Mgr_t *ddMgr, long int timeLimit);
EXTERN long int Ddi_MgrReadTimeInitial(Ddi_Mgr_t *ddMgr);
EXTERN void Ddi_MgrSetTimeInitial(Ddi_Mgr_t *ddMgr);
EXTERN int Ddi_MgrReadAigNodesNum(Ddi_Mgr_t *dd);
EXTERN int Ddi_MgrReadPeakAigNodesNum(Ddi_Mgr_t *dd);
EXTERN char * Ddi_MgrReadSatSolver(Ddi_Mgr_t *ddm);
EXTERN void Ddi_MgrSetSatSolver(Ddi_Mgr_t *dd, char *satSolver);
EXTERN void Ddi_AndAbsQuit (Ddi_Mgr_t *ddm);
EXTERN void Ddi_AndAbsFreeClipWindow (Ddi_Mgr_t *ddm);
EXTERN int Ddi_BddOperation(Ddi_Mgr_t *defaultDdMgr, Ddi_Bdd_t **bddP, char *string, Pdtutil_MgrOp_t operationFlag, void **voidPointer, Pdtutil_MgrRet_t *returnFlag);
EXTERN int Ddi_BddarrayOperation(Ddi_Mgr_t *defaultDdMgr, Ddi_Bddarray_t **bddArrayP, char *string, Pdtutil_MgrOp_t operationFlag, void **voidPointer, Pdtutil_MgrRet_t *returnFlag);
EXTERN Ddi_Bdd_t * Ddi_BddLoadCube(Ddi_Mgr_t *dd, FILE *fp, int idOrName);
EXTERN Ddi_Varset_t * Ddi_VarsetLoad(Ddi_Mgr_t *dd, FILE *fp, int idOrName);
EXTERN Ddi_Bdd_t * Ddi_VarSubst(Ddi_Bdd_t *f, Ddi_Vararray_t *x, Ddi_Vararray_t *y);
EXTERN void Ddi_PrintVararray(Ddi_Vararray_t *array);
EXTERN void Ddi_BddarrayPrint(Ddi_Bddarray_t *array);
EXTERN Cuplus_PruneHeuristic_e Ddi_ProfileHeuristicString2Enum(char *string);
EXTERN char * Ddi_ProfileHeuristicEnum2String(Cuplus_PruneHeuristic_e enumType);
EXTERN Cudd_ReorderingType Ddi_ReorderingMethodString2Enum(char *string);
EXTERN char * Ddi_ReorderingMethodEnum2String(Cudd_ReorderingType enumType);
EXTERN Ddi_DenseMethod_e Ddi_DenseMethodString2Enum(char *string);
EXTERN char * Ddi_DenseMethodEnum2String(Ddi_DenseMethod_e enumType);
EXTERN void Ddi_PrintCuddVersion(FILE *fp);
EXTERN int Ddi_BddPrintSupportAndCubes(Ddi_Bdd_t *f, int numberPerRow, int cubeNumberMax, int formatPla, char *filename, FILE *fp);
EXTERN int Ddi_BddarrayPrintSupportAndCubes(Ddi_Bddarray_t *fArray, int numberPerRow, int cubeNumberMax, int formatPla, int reverse, char *filename, FILE *fp);
EXTERN int Ddi_VarIndex(Ddi_Var_t *var);
EXTERN int Ddi_VarCuddIndex(Ddi_Var_t *var);
EXTERN Ddi_Var_t * Ddi_VarFromIndex(Ddi_BddMgr *ddm, int index);
EXTERN Ddi_Var_t * Ddi_VarFromCuddIndex(Ddi_BddMgr *ddm, int index);
EXTERN Ddi_Var_t * Ddi_VarAtLevel(Ddi_BddMgr *ddm, int lev);
EXTERN Ddi_Var_t * Ddi_VarNewBeforeVar(Ddi_Var_t *var);
EXTERN Ddi_Var_t * Ddi_VarNewAfterVar(Ddi_Var_t *var);
EXTERN Ddi_Var_t * Ddi_VarNewBaig(Ddi_BddMgr *ddm, char *name);
EXTERN Ddi_Var_t * Ddi_VarNew(Ddi_BddMgr *ddm);
EXTERN Ddi_Var_t * Ddi_VarNewAtLevel(Ddi_BddMgr *ddm, int lev);
EXTERN Ddi_Var_t *Ddi_VarFindOrAdd(Ddi_Mgr_t *ddm, char *name, int useBddVars);
EXTERN Ddi_Var_t *Ddi_VarFindOrAddAfterVar(Ddi_Mgr_t *ddm, char *name, int useBddVars, Ddi_Var_t *refVar);
EXTERN Ddi_Var_t *Ddi_VarFindOrAddBeforeVar(Ddi_Mgr_t *ddm, char *name, int useBddVars, Ddi_Var_t *refVar);
EXTERN int Ddi_VarCurrPos(Ddi_Var_t *var);
EXTERN char * Ddi_VarName(Ddi_Var_t *var);
EXTERN void Ddi_VarAttachName(Ddi_Var_t *var, char *name);
EXTERN void Ddi_VarDetachName(Ddi_Var_t *var);
EXTERN int Ddi_VarReadMark (Ddi_Var_t *v);
EXTERN void Ddi_VarWriteMark (Ddi_Var_t *v, int val);
EXTERN void Ddi_VarIncrMark (Ddi_Var_t *v, int val);
EXTERN int Ddi_VarAuxid(Ddi_Var_t *var);
EXTERN void Ddi_VarAttachAuxid(Ddi_Var_t *var, int auxid);
EXTERN bAigEdge_t Ddi_VarBaigId(Ddi_Var_t *var);
EXTERN void Ddi_VarAttachBaigId(Ddi_Var_t *var, int baigId);
EXTERN Ddi_Var_t * Ddi_VarFromName(Ddi_BddMgr *ddm, char *name);
EXTERN Ddi_Var_t * Ddi_VarFromAuxid(Ddi_BddMgr *ddm, int auxid);
EXTERN DdNode * Ddi_VarToCU(Ddi_Var_t * v);
EXTERN bAigEdge_t Ddi_VarToBaig(Ddi_Var_t * v);
EXTERN Ddi_Var_t * Ddi_VarFromCU(Ddi_BddMgr *ddm, DdNode * v);
EXTERN Ddi_Var_t * Ddi_VarFromBaig(Ddi_BddMgr *ddm, bAigEdge_t v);
EXTERN Ddi_Var_t * Ddi_VarCopy(Ddi_BddMgr *dd2, Ddi_Var_t *v);
EXTERN void Ddi_VarMakeGroup(Ddi_BddMgr *dd, Ddi_Var_t *v, int grpSize);
EXTERN void Ddi_VarMakeGroupFixed(Ddi_BddMgr *dd, Ddi_Var_t *v, int grpSize);
EXTERN Ddi_Varset_t * Ddi_VarReadGroup(Ddi_Var_t *v);
EXTERN int Ddi_VarIsGrouped(Ddi_Var_t *v);
EXTERN int Ddi_VarIsBdd(Ddi_Var_t *v);
EXTERN int Ddi_VarIsAig(Ddi_Var_t *v);
EXTERN Ddi_Var_t *Ddi_VarFindRefVar (Ddi_Var_t *v, char *prefix, char tailSeparator);
EXTERN Ddi_Vararray_t * Ddi_VararrayMakeFromCU(Ddi_Mgr_t *mgr, DdNode **array, int n);
EXTERN DdNode ** Ddi_VararrayToCU(Ddi_Vararray_t *array);
EXTERN Ddi_Vararray_t * Ddi_VararrayMakeFromInt(Ddi_Mgr_t *mgr, int *array, int n);
EXTERN Ddi_Vararray_t * Ddi_VararrayMakeNewVars(Ddi_Vararray_t *refArray, char *namePrefix, char *nameSuffix, int relativeOrder);
EXTERN Ddi_Vararray_t * Ddi_VararrayMakeNewAigVars(Ddi_Vararray_t *refArray, char *namePrefix, char *nameSuffix);
EXTERN Ddi_Vararray_t * Ddi_VararrayFindVarsFromPrefixSuffix (Ddi_Vararray_t *vars, char *namePrefix, char *nameSuffix);
EXTERN Ddi_Vararray_t * Ddi_VararrayFindRefVars(Ddi_Vararray_t *vars, char *namePrefix, char *nameSuffix);
EXTERN Ddi_Vararray_t * Ddi_VararrayMakeFromVarset(Ddi_Varset_t *vs, int sortByOrderOrIndex);
EXTERN void Ddi_VararraySort (Ddi_Vararray_t *va, int sortByOrderOrIndex);
EXTERN int * Ddi_VararrayToInt(Ddi_Vararray_t *array);
EXTERN Ddi_Vararray_t * Ddi_VararrayAlloc(Ddi_Mgr_t *mgr, int size);
EXTERN int Ddi_VararrayNum(Ddi_Vararray_t *array);
EXTERN void Ddi_VararrayWrite(Ddi_Vararray_t *array, int pos, Ddi_Var_t *var);
EXTERN Ddi_Var_t * Ddi_VararrayRead(Ddi_Vararray_t *array, int i);
EXTERN void Ddi_VararrayClear(Ddi_Vararray_t *array, int pos);
EXTERN void Ddi_VararrayInsert(Ddi_Vararray_t *array, int pos, Ddi_Var_t *v);
EXTERN void Ddi_VararrayInsertLast(Ddi_Vararray_t *array, Ddi_Var_t *v);
EXTERN Ddi_Var_t * Ddi_VararrayExtract(Ddi_Vararray_t *array, int i);
EXTERN void Ddi_VararrayRemove(Ddi_Vararray_t *array, int pos);
EXTERN void Ddi_VararrayRemoveNull(Ddi_Vararray_t *array);
EXTERN Ddi_Vararray_t *Ddi_VararrayUnion (Ddi_Vararray_t *v1, Ddi_Vararray_t *v2);
EXTERN Ddi_Vararray_t *Ddi_VararrayUnionAcc(Ddi_Vararray_t *v1, Ddi_Vararray_t *v2);
EXTERN Ddi_Vararray_t *Ddi_VararrayIntersect(Ddi_Vararray_t *v1, Ddi_Vararray_t *v2);
EXTERN Ddi_Vararray_t *Ddi_VararrayIntersectAcc(Ddi_Vararray_t *v1, Ddi_Vararray_t *v2);
EXTERN Ddi_Vararray_t * Ddi_VararrayDiff(Ddi_Vararray_t *v1, Ddi_Vararray_t *v2);
EXTERN Ddi_Vararray_t * Ddi_VararrayDiffAcc(Ddi_Vararray_t *v1, Ddi_Vararray_t *v2);
EXTERN Ddi_Vararray_t * Ddi_VararrayDup(Ddi_Vararray_t *old);
EXTERN Ddi_Vararray_t * Ddi_VararrayCopy(Ddi_BddMgr *ddm, Ddi_Vararray_t *old);
EXTERN void Ddi_VararrayAppend(Ddi_Vararray_t *array1, Ddi_Vararray_t *array2);
EXTERN int Ddi_VararraySearchVar(Ddi_Vararray_t *array, Ddi_Var_t *v);
EXTERN int Ddi_VararraySearchVarDown(Ddi_Vararray_t *array, Ddi_Var_t *v);
EXTERN void Ddi_VararrayPrint (Ddi_Vararray_t *array);
EXTERN Ddi_Vararray_t * Ddi_VararrayLoad (Ddi_BddMgr *ddm, char *filename, FILE *fp);
EXTERN void Ddi_VararrayStore (Ddi_Vararray_t *array, char *filename, FILE *fp);
EXTERN void Ddi_VararrayWriteMark (Ddi_Vararray_t *array, int val);
EXTERN void Ddi_VararrayWriteMarkWithIndex (Ddi_Vararray_t *array, int offset);
EXTERN void Ddi_VararrayCheckMark (Ddi_Vararray_t *array, int val);
EXTERN Ddi_Vararray_t * Ddi_VararraySwapVars(Ddi_Vararray_t *va, Ddi_Vararray_t *x, Ddi_Vararray_t *y);
EXTERN Ddi_Vararray_t * Ddi_VararraySwapVarsAcc(Ddi_Vararray_t *va, Ddi_Vararray_t *x, Ddi_Vararray_t *y);
EXTERN Ddi_Vararray_t * Ddi_VararraySubstVars(Ddi_Vararray_t *va, Ddi_Vararray_t *x, Ddi_Vararray_t *y);
EXTERN Ddi_Vararray_t * Ddi_VararraySubstVarsAcc(Ddi_Vararray_t *va, Ddi_Vararray_t *x, Ddi_Vararray_t *y);
EXTERN Ddi_Varset_t * Ddi_VarsetVoid(Ddi_BddMgr *ddm);
EXTERN int Ddi_VarsetIsVoid(Ddi_Varset_t *varset);
EXTERN int Ddi_VarsetIsArray (Ddi_Varset_t *varset);
EXTERN Ddi_Varset_t * Ddi_VarsetNext(Ddi_Varset_t *vs);
EXTERN Ddi_Varset_t * Ddi_VarsetNextAcc(Ddi_Varset_t *vs);
EXTERN Ddi_Varset_t * Ddi_VarsetAdd(Ddi_Varset_t *vs, Ddi_Var_t *v);
EXTERN Ddi_Varset_t * Ddi_VarsetAddAcc(Ddi_Varset_t *vs, Ddi_Var_t *v);
EXTERN void Ddi_VarsetBddAddAccFast(Ddi_Varset_t *vs, Ddi_Var_t *v);
EXTERN Ddi_Varset_t * Ddi_VarsetRemove(Ddi_Varset_t *vs, Ddi_Var_t *v);
EXTERN Ddi_Varset_t * Ddi_VarsetRemoveAcc(Ddi_Varset_t *vs, Ddi_Var_t *v);
EXTERN Ddi_Varset_t * Ddi_VarsetUnion(Ddi_Varset_t *v1, Ddi_Varset_t *v2);
EXTERN Ddi_Varset_t * Ddi_VarsetUnionAcc(Ddi_Varset_t *v1, Ddi_Varset_t *v2);
EXTERN Ddi_Varset_t * Ddi_VarsetIntersect(Ddi_Varset_t *v1, Ddi_Varset_t *v2);
EXTERN Ddi_Varset_t * Ddi_VarsetIntersectAcc(Ddi_Varset_t *v1, Ddi_Varset_t *v2);
EXTERN Ddi_Varset_t * Ddi_VarsetDiff(Ddi_Varset_t *v1, Ddi_Varset_t *v2);
EXTERN Ddi_Varset_t * Ddi_VarsetDiffAcc(Ddi_Varset_t *v1, Ddi_Varset_t *v2);
EXTERN Ddi_Varset_t * Ddi_VarsetDup(Ddi_Varset_t *src);
EXTERN Ddi_Varset_t * Ddi_VarsetCopy(Ddi_BddMgr *dd2, Ddi_Varset_t *src);
EXTERN Ddi_Varset_t * Ddi_VarsetEvalFree(Ddi_Varset_t *f, Ddi_Varset_t *g);
EXTERN int Ddi_VarsetNum(Ddi_Varset_t *vars);
EXTERN void Ddi_VarsetPrint(Ddi_Varset_t *vars, int numberPerRow, char *filename, FILE *fp);
EXTERN int Ddi_VarInVarset(Ddi_Varset_t *varset, Ddi_Var_t *var);
EXTERN int Ddi_VarsetEqual(Ddi_Varset_t *varset1, Ddi_Varset_t *varset2);
EXTERN Ddi_Var_t * Ddi_VarsetTop(Ddi_Varset_t *varset);
EXTERN Ddi_Var_t * Ddi_VarsetBottom(Ddi_Varset_t *varset);
EXTERN Ddi_BddNode * Ddi_VarsetToCU(Ddi_Varset_t *vs);
EXTERN Ddi_Varset_t * Ddi_VarsetMakeFromCU(Ddi_Mgr_t *mgr, DdNode *bdd);
EXTERN Ddi_Varset_t * Ddi_VarsetMakeArrayVoid(Ddi_Mgr_t *mgr);
EXTERN Ddi_Varset_t * Ddi_VarsetMakeFromVar(Ddi_Var_t *v);
EXTERN Ddi_Varset_t * Ddi_VarsetMakeFromArray(Ddi_Vararray_t *va);
EXTERN Ddi_Varset_t * Ddi_VarsetMakeArrayFromArray(Ddi_Vararray_t *va);
EXTERN Ddi_Varset_t * Ddi_VarsetMakeArray(Ddi_Varset_t *vs);
EXTERN Ddi_Varset_t * Ddi_VarsetMakeBdd(Ddi_Varset_t *vs);
EXTERN Ddi_Varset_t * Ddi_VarsetSetArray(Ddi_Varset_t *vs);
EXTERN Ddi_Varset_t * Ddi_VarsetSetBdd(Ddi_Varset_t *vs);
EXTERN Ddi_Varset_t * Ddi_VarsetSwapVars(Ddi_Varset_t *vs, Ddi_Vararray_t *x, Ddi_Vararray_t *y);
EXTERN Ddi_Varset_t * Ddi_VarsetSwapVarsAcc(Ddi_Varset_t *vs, Ddi_Vararray_t *x, Ddi_Vararray_t *y);
EXTERN Ddi_Varset_t * Ddi_VarsetSubstVars(Ddi_Varset_t *vs, Ddi_Vararray_t *x, Ddi_Vararray_t *y);
EXTERN Ddi_Varset_t * Ddi_VarsetSubstVarsAcc(Ddi_Varset_t *vs, Ddi_Vararray_t *x, Ddi_Vararray_t *y);
EXTERN void Ddi_VarsetWalkStart(Ddi_Varset_t *vs);
EXTERN void Ddi_VarsetWalkStep(Ddi_Varset_t *vs);
EXTERN int Ddi_VarsetWalkEnd(Ddi_Varset_t *vs);
EXTERN Ddi_Var_t * Ddi_VarsetWalkCurr(Ddi_Varset_t *vs);
EXTERN void Ddi_VarsetCheckMark (Ddi_Varset_t *vars, int val);
EXTERN void Ddi_VarsetWriteMark (Ddi_Varset_t *vars, int val);
EXTERN void Ddi_VarsetIncrMark (Ddi_Varset_t *vars, int val);
EXTERN Ddi_Varsetarray_t * Ddi_VarsetarrayAlloc(Ddi_Mgr_t *mgr, int length);
EXTERN int Ddi_VarsetarrayNum(Ddi_Varsetarray_t *array);
EXTERN void Ddi_VarsetarrayWrite(Ddi_Varsetarray_t *array, int pos, Ddi_Varset_t *vs);
EXTERN void Ddi_VarsetarrayInsert(Ddi_Varsetarray_t *array, int pos, Ddi_Varset_t *vs);
EXTERN void Ddi_VarsetarrayInsertLast(Ddi_Varsetarray_t *array, Ddi_Varset_t *vs);
EXTERN Ddi_Varset_t * Ddi_VarsetarrayRead(Ddi_Varsetarray_t *array, int i);
EXTERN void Ddi_VarsetarrayClear(Ddi_Varsetarray_t *array, int pos);
EXTERN Ddi_Varsetarray_t * Ddi_VarsetarrayDup(Ddi_Varsetarray_t *old);
EXTERN Ddi_Varsetarray_t * Ddi_VarsetarrayCopy(Ddi_BddMgr *ddm, Ddi_Varsetarray_t *old);
EXTERN Ddi_Varset_t * Ddi_VarsetarrayExtract(Ddi_Varsetarray_t *array, int i);
EXTERN Ddi_Varset_t * Ddi_VarsetarrayQuickExtract(Ddi_Varsetarray_t *array, int i);
EXTERN void Ddi_VarsetarrayRemove(Ddi_Varsetarray_t *array, int pos);
EXTERN void Ddi_VarsetarrayQuickRemove(Ddi_Varsetarray_t *array, int pos);
EXTERN void Ddi_VarsetarrayAppend (Ddi_Varsetarray_t *array1, Ddi_Varsetarray_t *array2);
EXTERN int Ddi_AigarrayAbcOptAcc (Ddi_Bddarray_t *aigA, float timeLimit);
EXTERN int Ddi_AigarrayAbcRpmAcc (Ddi_Bddarray_t *aigA, Ddi_Vararray_t *ps, int nCutMax, float timeLimit);
EXTERN int Ddi_AigAbcRpmAcc (Ddi_Bdd_t *aig, Ddi_Vararray_t *ps, int nCutMax, float timeLimit);
EXTERN Ddi_Bdd_t *Ddi_ImgConjPartTr (Ddi_Bdd_t *TR, Ddi_Bdd_t *from, Ddi_Bdd_t *constrain, Ddi_Varset_t *smoothV);
EXTERN Ddi_Bdd_t *Ddi_ImgDisjSat (Ddi_Bdd_t *TR, Ddi_Bdd_t *from, Ddi_Bdd_t *constrain, Ddi_Vararray_t *psv, Ddi_Vararray_t *nsv, Ddi_Varset_t *smoothV);

EXTERN Ddi_Clause_t * Ddi_ClauseAlloc(int nLits, int initSize);
EXTERN Ddi_Clause_t *Ddi_ClauseDup(Ddi_Clause_t *cl);
EXTERN Ddi_ClauseArray_t *Ddi_ClauseArrayDup(Ddi_ClauseArray_t *ca);
EXTERN Ddi_Clause_t *Ddi_ClauseCompact(Ddi_Clause_t *cl);
EXTERN Ddi_Clause_t *Ddi_ClauseNegLits(Ddi_Clause_t *cl);
EXTERN Ddi_Clause_t *Ddi_ClauseNegLitsAcc(Ddi_Clause_t *cl);
EXTERN Ddi_Clause_t *Ddi_ClauseRandLits(Ddi_Clause_t *vars);
EXTERN void Ddi_ClauseAddLiteral(Ddi_Clause_t *cl, int lit);
EXTERN int Ddi_ClauseRef(Ddi_Clause_t *cl);
EXTERN int Ddi_ClauseDeref(Ddi_Clause_t *cl);
EXTERN void Ddi_ClauseFree(Ddi_Clause_t *cl);
EXTERN int Ddi_ClauseSetLink(Ddi_Clause_t *cl, Ddi_Clause_t *cl2);
EXTERN int Ddi_ClauseClearLink(Ddi_Clause_t *cl);
EXTERN Ddi_Clause_t *Ddi_ClauseReadLink(Ddi_Clause_t *cl);
EXTERN Ddi_ClauseArray_t *Ddi_ClauseArrayAlloc(int initSize);
EXTERN void Ddi_ClauseArrayFree(Ddi_ClauseArray_t *ca);
EXTERN void Ddi_ClauseArrayWriteLast(Ddi_ClauseArray_t *ca, Ddi_Clause_t *cl);
EXTERN Ddi_Clause_t *Ddi_ClauseArrayRead(Ddi_ClauseArray_t *ca, int i);
EXTERN void Ddi_ClauseArrayRemove(Ddi_ClauseArray_t *ca, int i);
EXTERN void Ddi_ClauseArrayReset(Ddi_ClauseArray_t *ca);
EXTERN Ddi_PdrMgr_t *Ddi_PdrMake(Ddi_Bddarray_t *delta, Ddi_Vararray_t *pi, Ddi_Vararray_t *ps, Ddi_Vararray_t *ns, Ddi_Bdd_t *init, Ddi_Bddarray_t *initStub, Ddi_Bdd_t *invar, Ddi_Bdd_t *invarspec, int enUnfold);
EXTERN Ddi_ClauseArray_t *Ddi_ClausesMakeRel(Ddi_Bddarray_t *fA, Ddi_Vararray_t *vA, Ddi_NetlistClausesInfo_t *netCl);
EXTERN Ddi_ClauseArray_t *Ddi_ClausesMakeRelAbc(Ddi_Bddarray_t *fA, Ddi_Vararray_t *vA, Ddi_NetlistClausesInfo_t *netCl);
EXTERN Ddi_ClauseArray_t *Ddi_AigClauses(Ddi_Bdd_t *f, int makeRel, Ddi_Clause_t *assumeCl);
EXTERN Ddi_ClauseArray_t *Ddi_AigClausesWithJustify(Ddi_Bdd_t *f, int makeRel, Ddi_Clause_t *assumeCl, Ddi_ClauseArray_t *justify);
EXTERN Ddi_ClauseArray_t *Ddi_bAigArrayClauses(Ddi_Mgr_t *ddm, bAig_array_t *visitedNodes, int i0);
EXTERN Ddi_PdrMgr_t *Ddi_PdrMgrAlloc(void);
EXTERN void Ddi_PdrMgrQuit(Ddi_PdrMgr_t *pdrMgr);
EXTERN Ddi_SatSolver_t *Ddi_SatSolverAlloc(void);
EXTERN void Ddi_SatSolverQuit(Ddi_SatSolver_t *solver);
EXTERN void Ddi_SatSolverAddClauses(Ddi_SatSolver_t *solver, Ddi_ClauseArray_t *ca);
EXTERN void Ddi_SatSolverAddClause(Ddi_SatSolver_t *solver, Ddi_Clause_t *cl);

EXTERN int Ddi_SatSolve(Ddi_SatSolver_t *solver, Ddi_Clause_t *assume, int timeLimit);
EXTERN int Ddi_SatSolveCustomized(Ddi_SatSolver_t *solver, Ddi_Clause_t *assume, int neg, int offset, int timeLimit);
EXTERN void Ddi_PdrPrintClause(Ddi_PdrMgr_t *pdrMgr, Ddi_Clause_t *cl);
EXTERN void Ddi_PdrPrintClauseArray(Ddi_PdrMgr_t *pdrMgr, Ddi_ClauseArray_t *ca);
EXTERN Ddi_Clause_t *Ddi_PdrReadStateClause(Ddi_PdrMgr_t *pdrMgr, char *buf, int ps);
EXTERN int Ddi_SatCexIncludedInLit(Ddi_SatSolver_t *solver, int lit);
EXTERN Ddi_Clause_t *Ddi_SatSolverGetCexCube(Ddi_SatSolver_t *solver, Ddi_Clause_t *vars);
EXTERN Ddi_Bdd_t *Ddi_SatSolverGetCexCubeAig(Ddi_SatSolver_t *solver, Ddi_Clause_t *vars, Ddi_Vararray_t *varsAig);
EXTERN void Ddi_ClauseArrayPush(Ddi_ClauseArray_t *ca, Ddi_Clause_t *cl);
EXTERN Ddi_Clause_t *Ddi_ClauseArrayPop(Ddi_ClauseArray_t *ca);
EXTERN void Ddi_SatSolverAddClauseCustomized(Ddi_SatSolver_t *solver, Ddi_Clause_t *cl, int neg, int offset, int actLit);
EXTERN Ddi_Clause_t *Ddi_ClauseRemapVars(Ddi_Clause_t *cl, int offset);
EXTERN Ddi_Clause_t *Ddi_ClauseRemapVarsNegLits(Ddi_Clause_t *cl, int offset);
EXTERN int Ddi_PdrMgrGetFreeCnfVar(Ddi_PdrMgr_t *pdrMgr);
EXTERN Ddi_Clause_t *Ddi_SatSolverFinal(Ddi_SatSolver_t *solver, int maxVar);
EXTERN Ddi_Clause_t *Ddi_SatSolverCexCore (Ddi_SatSolver_t *solver, Ddi_Clause_t *cexPs, Ddi_Clause_t *cexPi, Ddi_Clause_t *cubePs, int maxVar);
EXTERN int Ddi_ClauseIsBlocked(Ddi_SatSolver_t *solver, Ddi_Clause_t *cube);
EXTERN int Ddi_ClauseInClauseArray(Ddi_ClauseArray_t *ca, Ddi_Clause_t *cl);
EXTERN int Ddi_ClauseArrayNum(Ddi_ClauseArray_t *ca);
EXTERN int Ddi_SatSolverOkay(Ddi_SatSolver_t *solver);
EXTERN int Ddi_SatSolverSimplify(Ddi_SatSolver_t *solver);
EXTERN int Ddi_ClauseNumLits(Ddi_Clause_t *cl);
EXTERN int Ddi_ClauseSubsumed(Ddi_Clause_t *c0, Ddi_Clause_t *cl);
EXTERN Ddi_Clause_t *Ddi_ClauseJoin(Ddi_Clause_t *c0, Ddi_Clause_t *c1);
EXTERN int Ddi_ClauseSort(Ddi_Clause_t *cl);
EXTERN int Ddi_ClauseArraySort(Ddi_ClauseArray_t *ca);
EXTERN int Ddi_ClauseArrayCompact(Ddi_ClauseArray_t *ca);
EXTERN int Ddi_ClauseArrayCompactWithClause(Ddi_ClauseArray_t *ca, Ddi_Clause_t *c);
EXTERN void Ddi_ClauseArrayCheckCompact(Ddi_ClauseArray_t *ca);

EXTERN Ddi_Clause_t *Ddi_ClauseToGblVarIds(Ddi_PdrMgr_t *pdrMgr, Ddi_Clause_t *cl);
EXTERN Ddi_Clause_t *Ddi_ClauseFromGblVarIds(Ddi_PdrMgr_t *pdrMgr, Ddi_Clause_t *cl);
EXTERN int Ddi_PdrRemapVarMark(Ddi_PdrMgr_t *pdrMgr);
EXTERN int Ddi_PdrRemapVarUnmark(Ddi_PdrMgr_t *pdrMgr);
EXTERN Ddi_ClauseArray_t *Ddi_ClauseArrayToGblVarIds(Ddi_PdrMgr_t *pdrMgr, Ddi_ClauseArray_t *ca);
EXTERN Ddi_ClauseArray_t *Ddi_ClauseArrayFromGblVarIds(Ddi_PdrMgr_t *pdrMgr, Ddi_ClauseArray_t *ca);

EXTERN Ddi_Clause_t *Ddi_PdrClauseCoi(Ddi_PdrMgr_t *pdrMgr, Ddi_Clause_t *cl);
EXTERN Ddi_Clause_t *Ddi_PdrClauseArraySupp(Ddi_PdrMgr_t *pdrMgr, Ddi_ClauseArray_t *ca);
EXTERN Ddi_Bdd_t *Ddi_BddMakeFromClause(Ddi_PdrMgr_t *pdrMgr, Ddi_Clause_t *cl);
EXTERN Ddi_Bdd_t *Ddi_BddMakeFromClauseArrayWithVars(Ddi_ClauseArray_t *ca, Ddi_Vararray_t *vars);
EXTERN Ddi_Bdd_t *Ddi_BddMakeFromClauseWithVars(Ddi_Clause_t *cl, Ddi_Vararray_t *vars);
EXTERN void Ddi_TernarySimQuit(Ddi_TernarySimMgr_t *xMgr);
EXTERN int Ddi_EqClassesRefine(  Ddi_PdrMgr_t *pdrMgr, Pdtutil_EqClasses_t *eqCl, Ddi_SatSolver_t *solver, Ddi_Vararray_t *vars, Ddi_Clause_t *clauseVars);
EXTERN Ddi_ClauseArray_t *Ddi_EqClassesAsClauses(Ddi_PdrMgr_t *pdrMgr, Pdtutil_EqClasses_t *eqCl, Ddi_Clause_t *clauseVars);
EXTERN Ddi_TernarySimMgr_t *Ddi_TernarySimInit(Ddi_Bddarray_t *fA, Ddi_Bdd_t *invarspec, Ddi_Vararray_t *pi, Ddi_Vararray_t *ps);
EXTERN Ddi_Clause_t *Ddi_SatSolverCubeConstrain (Ddi_SatSolver_t *solver, Ddi_Clause_t *cube);
EXTERN int Ddi_TernaryPdrSimulate(Ddi_TernarySimMgr_t *xMgr, int restart, Ddi_Clause_t *piBinaryVals, Ddi_Clause_t *psBinaryVals, Ddi_Clause_t *poBinaryVals, int useTarget, int xId);
EXTERN Ddi_Clause_t *Ddi_TernaryPdrJustify (Ddi_PdrMgr_t *pdrMgr, Ddi_TernarySimMgr_t *xMgr, Ddi_Clause_t *psBinaryVals, int evDriven);
EXTERN int *Ddi_PdrClauseActivityOrder(Ddi_PdrMgr_t *pdrMgr, Ddi_Clause_t *cl);
EXTERN void Ddi_PdrClauseUpdateActivity(Ddi_PdrMgr_t *pdrMgr, Ddi_Clause_t *cl);
EXTERN int Ddi_PdrVarHasActivity(Ddi_PdrMgr_t *pdrMgr, int lit);
EXTERN void Ddi_PdrVarWriteMark(Ddi_PdrMgr_t *pdrMgr, int lit, int mark);
EXTERN int Ddi_PdrVarReadMark(Ddi_PdrMgr_t *pdrMgr, int lit);
EXTERN Ddi_Clause_t *Ddi_ClauseJoin(Ddi_Clause_t *c0, Ddi_Clause_t *c1);
EXTERN int Ddi_PdrCubeNoInitLitNum(Ddi_PdrMgr_t *pdrMgr, Ddi_Clause_t *cl, int neg);
EXTERN int Ddi_PdrCubeLitIsNoInit(Ddi_PdrMgr_t *pdrMgr, int lit);
EXTERN Ddi_Clause_t *Ddi_GeneralizeClause(Ddi_SatSolver_t *solver, Ddi_Clause_t *cl, unsigned char *enAbstr, int nAbstr, int *initLits, int useInduction);
EXTERN int Ddi_UnderEstimateUnsatCoreByCexGen(Ddi_Bdd_t *f, Ddi_Bdd_t *cone, Ddi_Vararray_t *vars, unsigned char *enAbstr, int maxIter, int *result);
EXTERN Ddi_Bdd_t *Ddi_AigInterpolantByGenClauses(Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Bdd_t *care, Ddi_Bdd_t *careA, Ddi_Vararray_t *ps, Ddi_Vararray_t *ns, Ddi_Bdd_t *init, Ddi_Vararray_t *globVars,   Ddi_Vararray_t *dynAbstrCut, Ddi_Vararray_t *dynAbstrAux, Ddi_Bddarray_t *dynAbstrCutLits, int maxIter, int useInduction, int *result);
EXTERN Ddi_Bddarray_t *Ddi_AigAbstrRefineCegarPba(Ddi_Bdd_t *f, Ddi_Vararray_t *abstrVars, Ddi_Bddarray_t *noAbstr, Ddi_Bddarray_t *prevAbstr, int *result, float timeLimit);
EXTERN Ddi_Bddarray_t *Ddi_AigAbstrRefinePba(Ddi_Bdd_t *f, Ddi_Vararray_t *abstrVars, Ddi_Bddarray_t *noAbstr, Ddi_Bddarray_t *prevAbstr, int *result, float timeLimit);
EXTERN Ddi_Bdd_t *Ddi_AigExistProjectByGenClauses(Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Bdd_t *care, Ddi_Varset_t *projVars, int maxIter, int doConstr, int *result);
EXTERN Ddi_Bdd_t *Ddi_AigExistProjectBySubset(Ddi_Bdd_t *f, Ddi_Bdd_t *gHit, Ddi_Varset_t *projVars, int minCube, int maxIter, int *result);
EXTERN int Ddi_AigSignatureArrayNum(Ddi_AigSignatureArray_t *sig);
EXTERN Ddi_AigSignature_t *Ddi_AigSignatureArrayRead(Ddi_AigSignatureArray_t *sig, int i);
EXTERN long double Ddi_AigEstimateMintermCount (Ddi_Bdd_t *f, int numv);
EXTERN Ddi_AigDynSignatureArray_t *Ddi_AigEvalVarSignatures (Ddi_Mgr_t *ddm, bAig_array_t *aigNodes, Ddi_AigSignatureArray_t *varSig, int *mapPs, int *mapNs, int nMap);
EXTERN Ddi_Vararray_t *
Ddi_GetSccLatches (
  Ddi_SccMgr_t *sccMgr,
  Ddi_Vararray_t *ps,
  int sccBit
);
EXTERN int
Ddi_GetSccBitWithMaxLatches (
  Ddi_SccMgr_t *sccMgr
);
EXTERN Ddi_AigDynSignatureArray_t *Ddi_AigEvalVarSignaturesWithScc (Ddi_SccMgr_t *sccMgr, Ddi_Mgr_t *ddm, bAig_array_t *aigNodes, Ddi_Bddarray_t *lambda, Ddi_AigSignatureArray_t *varSig, int *mapPs, int *mapNs, int nMap);
EXTERN int Ddi_DeltaFindEqSpecs(Ddi_Bddarray_t *delta, Ddi_Vararray_t *ps, Ddi_Var_t *pvarPs, Ddi_Var_t *cvarPs, Ddi_Bdd_t *partSpec, int *nPartP, int doConstr, int speculate);
EXTERN Ddi_Bddarray_t *
Ddi_AigFindXors(
  Ddi_Bdd_t *f,
  Ddi_Vararray_t *mapVars,
  int topEqOnly
);
EXTERN Ddi_Bddarray_t *Ddi_AigarrayFindXors(Ddi_Bddarray_t *fA, Ddi_Vararray_t *mapVars, int topEqOnly);
EXTERN int Ddi_AigCheckEqGate (Ddi_Mgr_t *ddiMgr, bAigEdge_t baig, bAigEdge_t right, bAigEdge_t left, bAigEdge_t *baigEq0P, bAigEdge_t *baigEq1P, int chkDiff);
EXTERN int Ddi_AigCheckImplGate (Ddi_Mgr_t *ddiMgr, bAigEdge_t baig, bAigEdge_t right, bAigEdge_t left, bAigEdge_t *baigEq0P, bAigEdge_t *baigEq1P);
EXTERN Ddi_Bdd_t *Ddi_AigSatAndFlowAbstraction (Ddi_Bdd_t *a, Ddi_Bdd_t *b);
EXTERN void Ddi_PostOrderAigVisitIntern(bAig_Manager_t *manager, bAigEdge_t nodeIndex, bAig_array_t *visitedNodes, int maxDepth);
EXTERN void Ddi_PostOrderBddAigVisitIntern(Ddi_Bdd_t *fAig, bAig_array_t *visitedNodes, int maxDepth);
EXTERN void Ddi_PostOrderAigClearVisitedIntern(bAig_Manager_t *bmgr, bAig_array_t *visitedNodes);
EXTERN Ddi_Bdd_t *Ddi_AbcInterpolant (Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Varset_t *globalVars, Ddi_Varset_t *domainVars, int *psat, float timeLimit);
EXTERN Ddi_Bddarray_t *Ddi_AigarrayNetLoadAiger(Ddi_Mgr_t *dd, Ddi_Vararray_t *vars, char *filename);
EXTERN Ddi_Bddarray_t *
Ddi_AigarrayNetLoadAigerMapVars(
  Ddi_Mgr_t *dd,
  Ddi_Vararray_t *vars,
  Ddi_Vararray_t *mapVars,
  char *filename
) ;
EXTERN Ddi_Bdd_t *Ddi_AigNetLoadAiger(Ddi_Mgr_t *dd, Ddi_Vararray_t *vars, char *filename);
EXTERN int Ddi_AigarrayNetStoreAiger(Ddi_Bddarray_t *fA, int badOrOutput, char *filename);
EXTERN int Ddi_AigNetStoreAiger(Ddi_Bdd_t *f, int badOrOutput, char *filename);
EXTERN Ddi_Bddarray_t *Ddi_AigarrayNnf (Ddi_Bddarray_t *fA, Ddi_Vararray_t *monotoneVars, int assumeRed, Ddi_Bdd_t *splitVarConstrain, Ddi_Vararray_t *refVars, Ddi_Vararray_t *auxVars0, Ddi_Vararray_t *auxVars1);
EXTERN Ddi_Bdd_t *Ddi_AigNnf (Ddi_Bdd_t *f, Ddi_Vararray_t *monotoneVars, Ddi_Bdd_t *splitVarConstrain, Ddi_Vararray_t *refVars, Ddi_Vararray_t *auxVars0, Ddi_Vararray_t *auxVars1);
EXTERN Ddi_Vararray_t *Ddi_NnfDualRailVars(Ddi_Vararray_t *refVars, int twoRails);
EXTERN Ddi_Bdd_t *Ddi_NnfDualRailEq(Ddi_Bdd_t *eqConstr0, Ddi_Vararray_t *refV);
EXTERN Ddi_Vararray_t *Ddi_NnfDualRailFilterRefVars(Ddi_Vararray_t *refVars);
EXTERN int Ddi_AigarrayNnfCheck (Ddi_Bddarray_t *fA, Ddi_Bddarray_t *fRef);
EXTERN Ddi_Bddarray_t *Ddi_BddarrayAigFromNnf(Ddi_Bddarray_t *fA, Ddi_Vararray_t *refV, char *varPrefix, int joinOutputs);
EXTERN Ddi_Bddarray_t *Ddi_BddarrayAigFromNnfAcc(Ddi_Bddarray_t *fA, Ddi_Vararray_t *refV, char *varPrefix, int joinOutputs);
EXTERN Ddi_Bddarray_t *Ddi_BddarrayNnfOutputSplit(Ddi_Bddarray_t *fA);
EXTERN Ddi_Bddarray_t *Ddi_BddarrayNnfOutputSplitAcc(Ddi_Bddarray_t *fA);
EXTERN Ddi_Bdd_t *Ddi_BddAigFromNnf(Ddi_Bdd_t *f, Ddi_Vararray_t *refV);
EXTERN Ddi_Bdd_t *Ddi_BddAigFromNnfAcc(Ddi_Bdd_t *f, Ddi_Vararray_t *refV);
EXTERN Ddi_Bdd_t *Ddi_BddAigFromNnfTwoRailsAcc(Ddi_Bdd_t *f, Ddi_Vararray_t *refV);
EXTERN int Ddi_AigCheckMonotone (Ddi_Bdd_t *f, Ddi_Vararray_t *monotoneVars);
EXTERN int Ddi_AigarrayCheckMonotone (Ddi_Bddarray_t *fA, Ddi_Vararray_t *monotoneVars);
EXTERN Ddi_Bdd_t *Ddi_AigStructRedRem (Ddi_Bdd_t *f, Ddi_Bdd_t *share);
EXTERN Ddi_Bdd_t *Ddi_AigStructRedRemAcc (Ddi_Bdd_t *f, Ddi_Bdd_t *share);
EXTERN Ddi_Bdd_t *Ddi_AigExistByXorAcc (
  Ddi_Bdd_t *f,
  Ddi_Vararray_t *smooth
);
EXTERN Ddi_Bdd_t *Ddi_AigExistByCircQuant (
  Ddi_Bdd_t *f,
  Ddi_Vararray_t *smooth
);
EXTERN Ddi_Bdd_t *
Ddi_AigStructRedRemByOdcChain (
  Ddi_Bdd_t *f,
  int byLevel
);
EXTERN Ddi_Bdd_t *
Ddi_AigStructRedRemByOdcChainAcc (
  Ddi_Bdd_t *f,
  int byLevel
);
EXTERN Ddi_Bdd_t *Ddi_AigConstrainByStructRedRemAcc (Ddi_Bdd_t *f, Ddi_Bddarray_t *shareA, Ddi_Bdd_t *constr);
EXTERN Ddi_Bdd_t *Ddi_AigConstrainByStructRedRem (Ddi_Bdd_t *f, Ddi_Bddarray_t *shareA, Ddi_Bdd_t *constr);
EXTERN Ddi_Bddarray_t *Ddi_AigarrayStructRedRemWithConstrAcc (Ddi_Bddarray_t *fA, Ddi_Bddarray_t *shareA, Ddi_Bdd_t *constr);
EXTERN Ddi_Bddarray_t *Ddi_AigarrayStructRedRem (Ddi_Bddarray_t *fA, Ddi_Bddarray_t *shareA, int assumeRed);
EXTERN Ddi_Bddarray_t *Ddi_AigarrayStructRedRemOdcWithAssume (Ddi_Bddarray_t *fA, Ddi_Bddarray_t *assume, void *nnfCoreMgrVoid);
EXTERN Ddi_Bddarray_t *Ddi_AigarrayInsertCuts (Ddi_Bddarray_t *fA, Ddi_Bddarray_t *cuts, Ddi_Vararray_t *vars);
EXTERN Ddi_Bdd_t *Ddi_NnfClustSimplifyAcc (Ddi_Bdd_t *f, int fast);
EXTERN Ddi_Bdd_t *Ddi_NnfClustSimplify (Ddi_Bdd_t *f, int fast);
EXTERN Ddi_Bdd_t *Ddi_NnfOdcSimplify (Ddi_Bdd_t *f,Ddi_Bdd_t *care,int level);
EXTERN Ddi_Bdd_t *Ddi_NnfOdcSimplifyAcc (Ddi_Bdd_t *f,Ddi_Bdd_t *care,int level);
EXTERN Ddi_Bdd_t *Ddi_NnfIteSimplifyAcc (Ddi_Bdd_t *f, Ddi_Bdd_t *aForItp, Ddi_Bdd_t *bForItp, Ddi_Bdd_t *care);
EXTERN Ddi_Bdd_t *Ddi_NnfIteSimplify (Ddi_Bdd_t *f, Ddi_Bdd_t *aForItp, Ddi_Bdd_t *bForItp,
Ddi_Bdd_t *care);
Ddi_Bdd_t *Ddi_AigConstrainAcc (Ddi_Bdd_t *f, Ddi_Bdd_t *constrain, int phase);
Ddi_Bdd_t *Ddi_AigConstrain (Ddi_Bdd_t *f, Ddi_Bdd_t *constrain, int phase);
Ddi_Bddarray_t *Ddi_AigarrayConstrainAcc (Ddi_Bddarray_t *fA, Ddi_Bdd_t *constrain, int phase);
Ddi_Bddarray_t *Ddi_AigarrayConstrain (Ddi_Bddarray_t *fA, Ddi_Bdd_t *constrain, int phase);
EXTERN Ddi_Bdd_t *Ddi_AigStructEquivAcc (Ddi_Bdd_t *f, Ddi_Varset_t *project);
EXTERN Ddi_Bdd_t *Ddi_AigStructEquiv (Ddi_Bdd_t *f, Ddi_Varset_t *project);
EXTERN Ddi_AigAbcInfo_t *Ddi_BddarrayToAbcCnfInfo (Ddi_Bddarray_t *fA, Ddi_Vararray_t *vars, bAig_array_t *visitedNodes);
EXTERN bAig_array_t *Ddi_AbcCnfInfoReadVisitedNodes (Ddi_AigAbcInfo_t *aigAbcInfo);
EXTERN void Ddi_AigAbcInfoFree (Ddi_AigAbcInfo_t *aigAbcInfo);
EXTERN int Ddi_AigAbcCnfMaxId (Ddi_AigAbcInfo_t *aigAbcInfo);
EXTERN int Ddi_AigAbcCnfId (Ddi_AigAbcInfo_t *aigAbcInfo, int i);
EXTERN int Ddi_AigAbcCnfSetAbc2PdtMap(Ddi_AigAbcInfo_t *aigAbcInfo, int abcId, int pdtId);
EXTERN Ddi_ClauseArray_t *Ddi_AigClausesFromAbcCnf (Ddi_AigAbcInfo_t *aigAbcInfo, int i);
EXTERN Pdtutil_Array_t* getFaninsGateIds(Ddi_AigAbcInfo_t *aigAbcInfo, int j, int rootCnfId);
EXTERN Ddi_IncrSatMgr_t *Ddi_IncrSatMgrAlloc(Ddi_Mgr_t *ddm, int Minisat22, int enableProofLogging, int enableSimplify);
EXTERN void Ddi_IncrSatMgrLockAig(Ddi_IncrSatMgr_t *mgr,Ddi_Bdd_t *f);
EXTERN void Ddi_IncrSatMgrRestart(Ddi_IncrSatMgr_t *mgr);
EXTERN void Ddi_IncrSatMgrQuit(Ddi_IncrSatMgr_t *mgr);
EXTERN void Ddi_IncrSatMgrQuitKeepDdi(Ddi_IncrSatMgr_t *mgr);
EXTERN void
Ddi_IncrSatMgrQuitIntern(
  Ddi_IncrSatMgr_t *mgr,
  int freeDdi
);
EXTERN void Ddi_IncrSatMgrSuspend(Ddi_IncrSatMgr_t *mgr);
EXTERN void Ddi_IncrSatMgrResume(Ddi_IncrSatMgr_t *mgr);
EXTERN void Ddi_AbcUnlock (void);
EXTERN void Ddi_AbcLock (void);
EXTERN int Ddi_AbcLocked (void);
EXTERN void Ddi_AbcStop (void);
EXTERN int Ddi_AbcSynVerif (char *fileName, int doUndc, float timeLimit, int verbose);
EXTERN int Ddi_AbcReduce(Ddi_Bddarray_t *delta, Ddi_Bddarray_t *lambda, Ddi_Vararray_t *pi, Ddi_Vararray_t *ps, Ddi_Vararray_t *ns, float compactTh);
EXTERN Ddi_Bdd_t *Ddi_AbcRaritySim(Ddi_Bddarray_t *delta, Ddi_Bddarray_t *lambda, Ddi_Vararray_t *pi, Ddi_Vararray_t *ps, Ddi_Vararray_t *ns, Ddi_Vararray_t *is);
EXTERN Ddi_Bdd_t *Ddi_AbcCexFromAig(void *pCex, Ddi_Vararray_t *pi, Ddi_Vararray_t *is, void *pAig);
EXTERN Ddi_Varsetarray_t * Ddi_AigEvalMultipleCoi (Ddi_Bddarray_t *delta, Ddi_Bddarray_t *outputs, Ddi_Vararray_t *ps,int optionCluster,char *clusterFile,int methodCluster,int thresholdCluster);
EXTERN Ddi_Varsetarray_t * Ddi_AigEvalMultipleCoiWithScc (Ddi_Bddarray_t *delta, Ddi_Bddarray_t *outputs, Ddi_Vararray_t *ps,int optionCluster,char *clusterFile,int methodCluster,int thresholdCluster, int th_coi_diff,int th_coi_share, int doSort);
EXTERN int Ddi_AbcTemporPrefixLength(Ddi_Bddarray_t *delta, Ddi_Bddarray_t *lambda, Ddi_Vararray_t *ps, Ddi_Vararray_t *pi);
EXTERN Ddi_SccMgr_t *Ddi_FsmSccTarjan (Ddi_Bddarray_t *delta, Ddi_Bddarray_t *lambda, Ddi_Vararray_t *ps);
EXTERN Ddi_SccMgr_t * Ddi_FsmSccTarjanCheckBridge (Ddi_Bddarray_t *delta, Ddi_Bddarray_t *lambda,  Ddi_Vararray_t *ps, bAigEdge_t node, int isGate );
EXTERN void Ddi_FindSccBridgesNaive(Ddi_SccMgr_t *sccMgr, Ddi_Bddarray_t *delta, Ddi_Bddarray_t *lambda, Ddi_Vararray_t *ps);
EXTERN void Ddi_SccMgrFree(Ddi_SccMgr_t *sccMgr);
EXTERN Ddi_Bddarray_t *Ddi_FindIte(Ddi_Bddarray_t *delta, Ddi_Vararray_t *ps,  int threshold);
Ddi_Bddarray_t *Ddi_FindIteFull(Ddi_Bddarray_t *fA, Ddi_Vararray_t *ps, Ddi_Bddarray_t *iteArray, int threshold);
EXTERN Ddi_Bddarray_t *Ddi_FindAigIte(Ddi_Bdd_t *f, int threshold);
EXTERN Ddi_Bdd_t *Ddi_AigConstRed (Ddi_Bddarray_t *fA, Ddi_Bdd_t *constr, void *coreClauses, void *cnfMappedVars,int doRedRem, int maxGateWindow, int assertF, int checkOnlyRedPhase, void *nnfCoreMgrVoid, float timeLimit);
EXTERN Ddi_Bdd_t *Ddi_AigDisjDecomp (Ddi_Bdd_t *f, int minp,int maxp);
EXTERN Ddi_Bdd_t *Ddi_AigConjDecomp (Ddi_Bdd_t *f, int minp,int maxp);
EXTERN Ddi_Bddarray_t *Ddi_AigDisjDecompRoots (Ddi_Bdd_t *f, Ddi_Var_t *pVar, Ddi_Var_t *cVar, int recurTh);
EXTERN Ddi_Bdd_t *Ddi_ItpWeakenByRefinement (Ddi_Bdd_t *a,Ddi_Bdd_t *b,Ddi_Bdd_t *care);
EXTERN void Ddi_AigArrayClearAuxInt(bAig_Manager_t *bmgr, bAig_array_t *visitedNodes);
EXTERN Ddi_Bdd_t *
Ddi_BddPartFilter (
  Ddi_Bdd_t *f,
  Ddi_Bdd_t *filter 
);
EXTERN Ddi_Bdd_t *
Ddi_AigSat22Constrain(
  Ddi_Bdd_t *F,
  Ddi_Bdd_t *C,
  Ddi_Bdd_t *aForItp
);

/**AutomaticEnd***************************************************************/

#endif /* _DDI */

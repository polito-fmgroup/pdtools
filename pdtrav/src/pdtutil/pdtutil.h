/**CHeaderFile*****************************************************************

  FileName    [pdtutil.h]

  PackageName [pdtutil] 

  Synopsis    [Utility functions for the PdTRAV package.]

  Description [This package contains a set of <em>utility functions</em>
    shared in the <b>PdTRAV</b> package.<br>
    In this package are contained functions to allocate and free memory,
    open and close files, deal with strings, read and write variable orders,
    etc.
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

#ifndef _PDTUTIL
#define _PDTUTIL

#ifndef PDTRAV_DEBUG
#define PDTRAV_DEBUG 1
#endif

#ifndef PDTRAV_VERBOSITY_OnOff
#define PDTRAV_VERBOSITY_OnOff 1
#endif

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include "util.h"
#include <stdio.h>
/*#include <stdlib.h>*/
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>
#include <setjmp.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "cuddInt.h"
#include "dddmp.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

// Version specified within the Makefile
// #define PDTRAV_VERSION "3.0"

#define PDTUTIL_MAX_STR_LEN 1024
#define PDTUTIL_INITIAL_ARRAY_SIZE 3

#ifndef EXTERN
#ifdef __cplusplus
#	define EXTERN extern "C"
#else
#	define EXTERN extern
#endif
#endif
#ifndef ARGS
#if defined(__STDC__) || defined(__cplusplus)
#	define ARGS(protos)	protos  /* ANSI C */
#else                           /* !(__STDC__ || __cplusplus) */
#	define ARGS(protos)	()      /* K&R C */
#endif                          /* !(__STDC__ || __cplusplus) */
#endif

#if SOLARIS
#define JMPBUF sigjmp_buf
#define SETJMP(buf,val) sigsetjmp(buf, val)
#define LONGJMP(buf,val) siglongjmp(buf, val)
#else
#define JMPBUF jmp_buf
#define SETJMP(buf,val) setjmp(buf)
#define LONGJMP(buf,val) longjmp(buf, val)
#endif

#define ERROR_CODE_NO_MES   0
#define ERROR_CODE_MEM_LIMIT   100
#define ERROR_CODE_TIME_LIMIT  102
#define ERROR_CODE_ASSERT  4

typedef enum Pdtutil_EqClassState_e {
  Pdtutil_EqClass_Ldr_c,
  Pdtutil_EqClass_SplitLdr_c,
  Pdtutil_EqClass_JoinSplit_c,
  Pdtutil_EqClass_JoinConst_c,
  Pdtutil_EqClass_SameClass_c,
  Pdtutil_EqClass_Member_c,
  Pdtutil_EqClass_Unassigned_c,
  Pdtutil_EqClass_Singleton_c,
  Pdtutil_EqClass_None_c
} Pdtutil_EqClassState_e;


/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

typedef struct Pdtutil_Array_t Pdtutil_Array_t;
typedef void *Pdtutil_Item_t;

enum Pdtutil_SetStatType_e { INT_T, FLOAT_T, VOIDP_T };

union Pdtutil_SetStatUnion_u {
  int intv;
  float floatv;
  void *voidv;
};

typedef struct Pdtutil_SetStat_s {
  enum Pdtutil_SetStatType_e type;
  char *tag;
  union Pdtutil_SetStatUnion_u value;

} Pdtutil_SetStat_t;


typedef struct Pdtutil_SortInfo_t {
  float weight;
  int id;
} Pdtutil_SortInfo_t;


/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/**Enum************************************************************************

  Synopsis    [Type for verbosity levels.]

  Description [The basic idea is to have verbosity level divided into three
    categories: user, application, and developers. For each of this category
    three levels are defined: minimun, medium, and average.
    ]

******************************************************************************/

typedef enum {
  Pdtutil_VerbLevelNone_c,
  Pdtutil_VerbLevelUsrMin_c,
  Pdtutil_VerbLevelUsrMed_c,
  Pdtutil_VerbLevelUsrMax_c,
  Pdtutil_VerbLevelDevMin_c,
  Pdtutil_VerbLevelDevMed_c,
  Pdtutil_VerbLevelDevMax_c,
  Pdtutil_VerbLevelDebug_c,
  Pdtutil_VerbLevelSame_c
} Pdtutil_VerbLevel_e;

typedef enum {
  Pdt_OptPdt_c,
  Pdt_OptMc_c,
  Pdt_OptTrav_c,
  Pdt_OptTr_c,
  Pdt_OptFsm_c,
  Pdt_OptDdi_c,
  Pdt_OptList_c,
  Pdt_OptNone_c
} Pdt_OptModule_e;

typedef enum {
  Pdt_ListNone_c
} Pdt_OptTagList_e;

typedef enum {
  Pdt_DdiVerbosity_c,

  //Pdt_DdiExistClipTh_c,
  //Pdt_DdiSiftTh_c,
  //Pdt_DdiSiftMaxTh_c,
  //Pdt_DdiSiftMaxCalls_c,
  Pdt_DdiExistOptLevel_c,
  Pdt_DdiExistOptThresh_c,

  Pdt_DdiSatSolver_c,
  Pdt_DdiLazyRate_c,
  Pdt_DdiAigAbcOpt_c,
  Pdt_DdiAigBddOpt_c,
  Pdt_DdiAigRedRem_c,
  Pdt_DdiAigRedRemPeriod_c,
  Pdt_DdiItpIteOptTh_c,
  Pdt_DdiItpStructOdcTh_c,
  Pdt_DdiItpPartTh_c,
  Pdt_DdiItpStoreTh_c,
  Pdt_DdiDynAbstrStrategy_c,
  Pdt_DdiNnfClustSimplify_c,
  Pdt_DdiTernaryAbstrStrategy_c,
  Pdt_DdiSatTimeout_c,
  Pdt_DdiSatIncremental_c,
  Pdt_DdiSatVarActivity_c,
  Pdt_DdiSatTimeLimit_c,
  Pdt_DdiItpOpt_c,
  Pdt_DdiItpActiveVars_c,
  Pdt_DdiItpAbc_c,
  Pdt_DdiItpTwice_c,
  Pdt_DdiItpReverse_c,
  Pdt_DdiItpNoQuantify_c,
  Pdt_DdiItpCore_c,
  Pdt_DdiItpAigCore_c,
  Pdt_DdiItpMem_c,
  Pdt_DdiItpMap_c,
  Pdt_DdiItpStore_c,
  Pdt_DdiItpCompute_c,
  Pdt_DdiItpLoad_c,
  Pdt_DdiItpDrup_c,
  Pdt_DdiItpUseCare_c,
  Pdt_DdiAigCnfLevel_c,
  Pdt_DdiNone_c,
  Pdt_DdiItpCompact_c,
  Pdt_DdiItpClust_c,
  Pdt_DdiItpNorm_c,
  Pdt_DdiItpSimp_c
} Pdt_OptTagDdi_e;

typedef enum {
  Pdt_FsmName_c,
  Pdt_FsmVerbosity_c,
  Pdt_FsmCutThresh_c,
  Pdt_FsmCutAtAuxVar_c,
  Pdt_FsmUseAig_c,
  Pdt_FsmCegarStrategy_c,
  Pdt_FsmInitStubSteps_c,
  Pdt_FsmTargetEnSteps_c,
  Pdt_FsmPhaseAbstr_c,
  Pdt_FsmRemovedPis_c,
  Pdt_FsmExternalConstr_c,
  Pdt_FsmXInitLatches_c,

  Pdt_FsmInvarspec_c,

  Pdt_FsmNone_c
} Pdt_OptTagFsm_e;

typedef enum {
  Pdt_TrName_c,
  Pdt_TrVerbosity_c,
  /* sort */
  Pdt_TrSort_c,
  /* cluster */
  Pdt_TrClustTh_c,
  /* closure */
  Pdt_TrSquaringMaxIter_c,
  /* image */
  Pdt_TrImgClustTh_c,
  Pdt_TrApproxMinSize_c,
  Pdt_TrApproxMaxSize_c,
  Pdt_TrApproxMaxIter_c,
  Pdt_TrCheckFrozen_c,
  /* coi */
  Pdt_TrCoiLimit_c,

  Pdt_TrNone_c
} Pdt_OptTagTr_e;

typedef enum {
  /* general */
  Pdt_TravName_c,
  Pdt_TravVerbosity_c,
  Pdt_TravStdout_c,
  Pdt_TravLogPeriod_c,
  Pdt_TravClk_c,
  Pdt_TravDoCex_c,

  /* unused (almost all missing) */
  Pdt_TravConstrLevel_c,
  Pdt_TravIwls95Variant_c,

  /* tr */
  Pdt_TravTrProfileMethod_c,
  Pdt_TravTrProfileDynamicEnable_c,
  Pdt_TravTrProfileThreshold_c,
  Pdt_TravTrSorting_c,
  Pdt_TravTrSortW1_c,
  Pdt_TravTrSortW2_c,
  Pdt_TravTrSortW3_c,
  Pdt_TravTrSortW4_c,
  Pdt_TravSortForBck_c,
  Pdt_TravTrPreimgSort_c,
  Pdt_TravPartDisjTrThresh_c,
  Pdt_TravClustThreshold_c,
  Pdt_TravApprClustTh_c,
  Pdt_TravImgClustTh_c,
  Pdt_TravSquaring_c,
  Pdt_TravTrProj_c,
  Pdt_TravTrDpartTh_c,
  Pdt_TravTrDpartVar_c,
  Pdt_TravTrAuxVarFile_c,
  Pdt_TravTrEnableVar_c,
  Pdt_TravApproxTrClustFile_c,
  Pdt_TravBwdTrClustFile_c,

  /* aig */
  Pdt_TravSatSolver_c,
  Pdt_TravAbcOptLevel_c,
  Pdt_TravTargetEn_c,
  Pdt_TravDynAbstr_c,
  Pdt_TravDynAbstrInitIter_c,
  Pdt_TravImplAbstr_c,
  Pdt_TravTernaryAbstr_c,
  Pdt_TravStoreAbstrRefRefinedVars_c,
  Pdt_TravAbstrRef_c,
  Pdt_TravAbstrRefGla_c,
  Pdt_TravAbstrRefItp_c,
  Pdt_TravAbstrRefItpMaxIter_c,
  Pdt_TravTrAbstrItp_c,
  Pdt_TravTrAbstrItpMaxFwdStep_c,
  Pdt_TravTrAbstrItpLoad_c,
  Pdt_TravTrAbstrItpStore_c,
  Pdt_TravInputRegs_c,
  Pdt_TravSelfTuning_c,
  Pdt_TravLemmasTimeLimit_c,
  Pdt_TravLazyTimeLimit_c,

  /* itp */
  Pdt_TravItpBdd_c,
  Pdt_TravItpPart_c,
  Pdt_TravItpAppr_c,
  Pdt_TravItpRefineCex_c,
  Pdt_TravItpStructAbstr_c,
  Pdt_TravItpPartCone_c,
  Pdt_TravItpNew_c,
  Pdt_TravItpExact_c,
  Pdt_TravItpInductiveTo_c,
  Pdt_TravItpInnerCones_c,
  Pdt_TravItpTrAbstr_c,
  Pdt_TravItpInitAbstr_c,
  Pdt_TravItpEndAbstr_c,
  Pdt_TravItpReuseRings_c,
  Pdt_TravItpTuneForDepth_c,
  Pdt_TravItpBoundkOpt_c,
  Pdt_TravItpExactBoundDouble_c,
  Pdt_TravItpConeOpt_c,
  Pdt_TravItpForceRun_c,
  Pdt_TravItpMaxStepK_c,
  Pdt_TravItpUseReached_c,
  Pdt_TravItpCheckCompleteness_c,
  Pdt_TravItpGfp_c,
  Pdt_TravItpConstrLevel_c,
  Pdt_TravItpGenMaxIter_c,
  Pdt_TravItpEnToPlusImg_c,
  Pdt_TravItpShareReached_c,
  Pdt_TravItpUsePdrReached_c,
  Pdt_TravItpWeaken_c,
  Pdt_TravItpStrengthen_c,
  Pdt_TravItpReImg_c,
  Pdt_TravItpFromNewLevel_c,
  Pdt_TravItpRpm_c,
  Pdt_TravItpTimeLimit_c,
  Pdt_TravItpPeakAig_c,
  Pdt_TravItpStoreRings_c,

  /* igr */
  Pdt_TravIgrSide_c,
  Pdt_TravIgrItpSeq_c,
  Pdt_TravIgrRestart_c,
  Pdt_TravIgrRewind_c,
  Pdt_TravIgrRewindMinK_c,
  Pdt_TravIgrBwdRefine_c,
  Pdt_TravIgrGrowCone_c,
  Pdt_TravIgrGrowConeMaxK_c,
  Pdt_TravIgrGrowConeMax_c,
  Pdt_TravIgrUseRings_c,
  Pdt_TravIgrUseRingsStep_c,
  Pdt_TravIgrUseBwdRings_c,
  Pdt_TravIgrAssumeSafeBound_c,

  Pdt_TravIgrConeSubsetBound_c,
  Pdt_TravIgrConeSubsetSizeTh_c,
  Pdt_TravIgrConeSubsetPiRatio_c,
  Pdt_TravIgrConeSplitRatio_c,

  Pdt_TravIgrMaxIter_c,
  Pdt_TravIgrMaxExact_c,
  Pdt_TravIgrFwdBwd_c,

  /* pdr */
  Pdt_TravPdrFwdEq_c,
  Pdt_TravPdrInf_c,
  Pdt_TravPdrUnfold_c,
  Pdt_TravPdrIndAssumeLits_c,
  Pdt_TravPdrPostordCnf_c,
  Pdt_TravPdrCexJustify_c,
  Pdt_TravPdrGenEffort_c,
  Pdt_TravPdrIncrementalTr_c,
  Pdt_TravPdrShareReached_c,
  Pdt_TravPdrReuseRings_c,
  Pdt_TravPdrMaxBlock_c,
  Pdt_TravPdrUsePgEncoding_c,
  Pdt_TravPdrBumpActTopologically_c,
  Pdt_TravPdrSpecializedCallsMask_c,
  Pdt_TravPdrRestartStrategy_c,
  Pdt_TravPdrTimeLimit_c,

  /* bmc */
  Pdt_TravBmcTimeLimit_c,
  Pdt_TravBmcMemLimit_c,
  Pdt_TravBmcItpRingsPeriod_c,
  Pdt_TravBmcTrAbstrPeriod_c,
  Pdt_TravBmcTrAbstrInit_c,
  
  /* bdd */
  Pdt_TravFromSelect_c,
  Pdt_TravUnderApproxMaxVars_c,
  Pdt_TravUnderApproxMaxSize_c,
  Pdt_TravUnderApproxTargetRed_c,
  Pdt_TravNoCheck_c,
  Pdt_TravNoMismatch_c,
  Pdt_TravMismOptLevel_c,
  Pdt_TravGroupBad_c,
  Pdt_TravKeepNew_c,
  Pdt_TravPrioritizedMc_c,
  Pdt_TravPmcPrioThresh_c,
  Pdt_TravPmcMaxNestedFull_c,
  Pdt_TravPmcMaxNestedPart_c,
  Pdt_TravPmcNoSame_c,
  Pdt_TravCountReached_c,
  Pdt_TravAutoHint_c,
  Pdt_TravHintsStrategy_c,
  Pdt_TravHintsTh_c,
  Pdt_TravHints_c,
  Pdt_TravAuxPsSet_c,
  Pdt_TravGfpApproxRange_c,
  Pdt_TravUseMeta_c,
  Pdt_TravMetaMethod_c,
  Pdt_TravMetaMaxSmooth_c,
  Pdt_TravConstrain_c,
  Pdt_TravConstrainLevel_c,
  Pdt_TravCheckFrozen_c,
  Pdt_TravApprOverlap_c,
  Pdt_TravApprNP_c,
  Pdt_TravApprMBM_c,
  Pdt_TravApprGrTh_c,
  Pdt_TravApprPreimgTh_c,
  Pdt_TravMaxFwdIter_c,
  Pdt_TravNExactIter_c,
  Pdt_TravSiftMaxTravIter_c,
  Pdt_TravImplications_c,
  Pdt_TravImgDpartTh_c,
  Pdt_TravImgDpartSat_c,
  Pdt_TravBound_c,
  Pdt_TravStrictBound_c,
  Pdt_TravGuidedTrav_c,
  Pdt_TravUnivQuantifyTh_c,
  Pdt_TravOptImg_c,
  Pdt_TravOptPreimg_c,
  Pdt_TravBddTimeLimit_c,
  Pdt_TravBddNodeLimit_c,
  Pdt_TravWP_c,
  Pdt_TravWR_c,
  Pdt_TravWBR_c,
  Pdt_TravWU_c,
  Pdt_TravWOrd_c,
  Pdt_TravRPlus_c,
  Pdt_TravInvarFile_c,
  Pdt_TravStoreCnf_c,
  Pdt_TravStoreCnfTr_c,
  Pdt_TravStoreCnfMode_c,
  Pdt_TravStoreCnfPhase_c,
  Pdt_TravMaxCnfLength_c,

  /* certify */
  Pdt_TravCertFramesK_c,
  Pdt_TravCertTDecompK_c,
  Pdt_TravCertInvarOut_c,
  Pdt_TravCertSimpInvar_c,

  Pdt_TravNone_c
} Pdt_OptTagTrav_e;

typedef enum {
  Pdt_McVerbosity_c,
  Pdt_McName_c,
  Pdt_McMethod_c,

  Pdt_McPdr_c,
  Pdt_McAig_c,
  Pdt_McInductive_c,
  Pdt_McInterpolant_c,
  Pdt_McCustom_c,
  Pdt_McLemmas_c,
  Pdt_McLazy_c,
  Pdt_McLazyRate_c,
  Pdt_McQbf_c,
  Pdt_McDiameter_c,
  Pdt_McCheckMult_c,
  Pdt_McExactTrav_c,

  Pdt_McBmc_c,
  Pdt_McBmcStrategy_c,
  Pdt_McInterpolantBmcSteps_c,
  Pdt_McBmcTr_c,
  Pdt_McBmcStep_c,
  Pdt_McBmcLearnStep_c,
  Pdt_McBmcFirst_c,
  Pdt_McBmcTe_c,

  Pdt_McItpSeq_c,
  Pdt_McItpSeqGroup_c,
  Pdt_McItpSeqSerial_c,
  Pdt_McInitBound_c,
  Pdt_McEnFastRun_c,
  Pdt_McFwdBwdFP_c,

  Pdt_McNone_c
} Pdt_OptTagMc_e;

struct Pdtutil_OptItem_s {
  union {
    Pdt_OptTagList_e eListOpt;
    Pdt_OptTagDdi_e eDdiOpt;
    Pdt_OptTagFsm_e eFsmOpt;
    Pdt_OptTagTr_e eTrOpt;
    Pdt_OptTagTrav_e eTravOpt;
    Pdt_OptTagMc_e eMcOpt;
  } optTag;

  union {
    int inum;
    float fnum;
    char *pchar;
    void *pvoid;
  } optData;

};

typedef struct Pdtutil_List_s Pdtutil_List_t;
typedef struct Pdtutil_OptItem_s Pdtutil_OptItem_t;
typedef struct Pdtutil_OptList_s Pdtutil_OptList_t;

typedef struct Pdtutil_MapInt2Int_s Pdtutil_MapInt2Int_t;

/**Enum************************************************************************

  Synopsis    [Type to Distinguishing Different Operations of Managers]

  Description [Type to Distinguishing Different Operations of Managers.
    For each package, fsm, tr, trav, etc., a manager is defined.
    On each manager a certain number of operations are permitted and encoded
    as defined by this enumeration type.
    ]

******************************************************************************/

typedef enum {
  Pdtutil_MgrOpNone_c,
  /*
   * Generic Operations (Usually on a Bdd o an Array)
   * used whenever type is not known in advance
   */
  Pdtutil_MgrOpDelete_c,
  Pdtutil_MgrOpRead_c,
  Pdtutil_MgrOpSet_c,
  /*
   * Operation on BDDs
   */
  Pdtutil_MgrOpBddDelete_c,
  Pdtutil_MgrOpBddRead_c,
  Pdtutil_MgrOpBddSet_c,
  /*
   * Operation on Array of BDDs
   */
  Pdtutil_MgrOpArrayDelete_c,
  Pdtutil_MgrOpArrayRead_c,
  Pdtutil_MgrOpArraySet_c,
  /*
   * Operation on Managers
   */
  Pdtutil_MgrOpMgrDelete_c,
  Pdtutil_MgrOpMgrRead_c,
  Pdtutil_MgrOpMgrSet_c,
  /*
   * Set, Show, Stats
   */
  Pdtutil_MgrOpOptionSet_c,
  Pdtutil_MgrOpOptionShow_c,
  Pdtutil_MgrOpStats_c
} Pdtutil_MgrOp_t;

/**Enum************************************************************************

  Synopsis    [Type to Establishing The Type Returned]

  Description []

******************************************************************************/

typedef enum {
  Pdtutil_MgrRetNone_c,
  Pdtutil_MgrRetBdd_c,
  Pdtutil_MgrRetBddarray_c,
  Pdtutil_MgrRetDdMgr_c
} Pdtutil_MgrRet_t;

/**Enum************************************************************************

  Synopsis    [Type for Variable Order Format.]

  Description [Defines the format for the variable order file.
    For compatibility reasons, with the cudd/nanotrav package and with
    vis, different formats are defined.
    ]

******************************************************************************/

typedef enum {
  Pdtutil_VariableOrderDefault_c,
  Pdtutil_VariableOrderOneRow_c,
  Pdtutil_VariableOrderPiPs_c,
  Pdtutil_VariableOrderPiPsNs_c,
  Pdtutil_VariableOrderIndex_c,
  Pdtutil_VariableOrderComment_c
} Pdtutil_VariableOrderFormat_e;

/**Enum************************************************************************

  Synopsis    [Operation Codes for DD Operations.]

  Description [Define operation (AND, OR, etc.) on DD types, on operations
    between two BDD, one array of BDDs and on BDD, two arrays of BDDs, etc. 
    ]

******************************************************************************/

typedef enum {
  /*
   *  Nothing to do
   */
  Pdtutil_BddOpNone_c,
  /*
   *  Unary Operators
   */
  /* Insert a New Element */
  Pdtutil_BddOpInsert_c,
  /* Insert a New Element After Freeing the Previous One (if not NULL) */
  Pdtutil_BddOpReplace_c,
  Pdtutil_BddOpNot_c,
  /*
   *  Binary Operators
   */
  Pdtutil_BddOpAnd_c,
  Pdtutil_BddOpOr_c,
  Pdtutil_BddOpNand_c,
  Pdtutil_BddOpNor_c,
  Pdtutil_BddOpXor_c,
  Pdtutil_BddOpXnor_c,
  Pdtutil_BddOpExist_c,
  Pdtutil_BddOpAndExist_c,
  Pdtutil_BddOpConstrain_c,
  Pdtutil_BddOpRestrict_c,
  /*
   *  Ternary Operators
   */
  Pdtutil_BddOpIte_c
} Pdtutil_BddOp_e;

/**Enum************************************************************************

  Synopsis    [Type for dump AIGs.]

  Description [In which format to dump AIGs structures.
    ]

******************************************************************************/

typedef enum {
  Pdtutil_Aig2Verilog_c,
  Pdtutil_Aig2BenchName_c,
  Pdtutil_Aig2BenchId_c,
  Pdtutil_Aig2BenchLocalId_c,
  Pdtutil_Aig2Blif_c,
  Pdtutil_Aig2Slif_c
} Pdtutil_AigDump_e;

typedef enum {
  Pdtutil_ArraySort_Asc_c,
  Pdtutil_ArraySort_Desc_c
} Pdtutil_ArraySort_e;


typedef struct Pdtutil_EqClasses_s Pdtutil_EqClasses_t;

/**Enum************************************************************************

  Synopsis    [Type of MapInt2Int mapping]

  Description [Describe whether the mapping is both direct and inverse or just direct]

******************************************************************************/

typedef enum {
  Pdtutil_MapInt2IntDir_c,
  Pdtutil_MapInt2IntDirInv_c,
  Pdtutil_MapInt2IntNone_c,

} Pdtutil_MapInt2Int_e;


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

extern FILE *pdtStdout;
extern FILE *pdtResOut;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#define util_setlongjmp() SETJMP(*(util_newlongjmp()), 1)
#define CATCH if (util_setlongjmp() == 0) {
#define FAIL  util_cancellongjmp(); } else

#define util_print_time Pdtutil_PrintTime

//DV: to use the CUDD timing function, comment the following define
#define util_cpu_time Pdtutil_CpuTime

#ifdef stdout
#undef stdout
#endif
#define stdout pdtStdout
#ifdef printf
#undef printf
#endif
#define printf Pdtutil_Printf

/**Macro***********************************************************************

  Synopsis    [Log messages under verbosity check.]

  Description [The verbosity level is taken from a manager.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

#if PDTRAV_VERBOSITY_OnOff
#define Pdtutil_LogMsgMgr(mgr, verbLevel, ...) \
  { \
  if ((mgr)!=NULL  && (mgr)->settings.verbosity>=\
    (Pdtutil_VerbLevelNone_c+verbLevel))   \
    {Pdtutil_LogMsg(-1, __VA_ARGS__);} \
  }
#else
#define Pdtutil_LogMsgMgr(mgr, verbLevel, ...)
#endif

/**Macro***********************************************************************

  Synopsis    [read mgr verbosity.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

#define Pdtutil_MgrVerbosity(mgr) \
  ((mgr)->settings.verbosity)

/**Macro***********************************************************************

  Synopsis    [Print out messages under verbosity check.]

  Description [The verbosity level is taken from a manager.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

#if PDTRAV_VERBOSITY_OnOff
#define Pdtutil_VerbosityMgr(mgr, verbLevel, code) \
  { \
    if ((mgr)!=NULL  && (mgr)->settings.verbosity>=verbLevel) \
      {code;  fflush(stdout); fflush(stderr); fflush(NULL);} \
  }
#else
#define Pdtutil_VerbosityMgr(mgr, verbLevel, code)
#endif

/**Macro***********************************************************************

  Synopsis    [Print out messages under verbosity check.]

  Description [The verbosity level is taken from a manager.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

#if PDTRAV_VERBOSITY_OnOff
#define Pdtutil_VerbosityMgrIf(mgr, verbLevel) \
  if ((mgr)!=NULL  && (mgr)->settings.verbosity>=verbLevel)
#else
#define Pdtutil_VerbosityMgrIf(mgr, verbLevel) \
  if (0)
#endif

/**Macro***********************************************************************

  Synopsis    [Print out messages under verbosity check.]

  Description [The verbosity level is given to the macro by the caller.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

#if PDTRAV_VERBOSITY_OnOff
#define Pdtutil_VerbosityLocal(verbLevelLocal, verbLevel, code) \
  { \
    if (verbLevelLocal>=verbLevel) \
      {code; fflush(stdout); fflush(stderr); fflush(NULL);} \
  }
#else
#define Pdtutil_VerbosityLocal(verbLevelLocal, verbLevel, code)
#endif

/**Macro***********************************************************************

  Synopsis    [Print out messages under verbosity check.]

  Description [The verbosity level is given to the macro by the caller.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

#if PDTRAV_VERBOSITY_OnOff
#define Pdtutil_VerbosityLocalIf(verbLevelLocal, verbLevel) \
  if (verbLevelLocal>=verbLevel)
#else
#define Pdtutil_VerbosityLocalIf(verbLevelLocal, verbLevel) \
  if (0)
#endif

/**Macro***********************************************************************

  Synopsis    [Print out messages under verbosity check.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

#if PDTRAV_VERBOSITY_OnOff
#define Pdtutil_Verbosity(code) \
  { \
      {code; fflush(NULL);} \
  }
#else
#define Pdtutil_Verbosity(code)
#endif

/**Macro***********************************************************************

  Synopsis    [Checks for fatal bugs]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

#if PDTRAV_DEBUG
#define Pdtutil_Assert(expr,errMsg) \
     { \
     if ((expr) == 0) { \
       fflush (NULL); \
       fprintf (stderr, "FATAL ERROR: %s\n", errMsg); \
       fprintf (stderr, "             File %s -> Line %d\n", \
         __FILE__, __LINE__); \
       fflush (stderr); \
       assert (expr); \
       } \
     }
#else
#define Pdtutil_Assert(expr,errMsg) \
     {}
#endif

/**Macro***********************************************************************

  Synopsis    [Checks for Warnings: If expr==1 print out the warning on
    stderr]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

#define Pdtutil_Warning(expr,errMsg) \
  { \
  if ((expr) == 1) { \
    fprintf (stderr, "WARNING: %s\n", errMsg); \
    fprintf (stderr, "         File %s -> Line %d\n", \
      __FILE__, __LINE__); \
    fflush (stderr); \
    } \
  }

/**Macro***********************************************************************
 
  Synopsis    [Checks for fatal bugs and return the DDDMP_FAILURE flag.]
 
  Description []
 
  SideEffects [None]
 
  SeeAlso     []
 
******************************************************************************/

#define Pdtutil_CheckAndReturn(expr,errMsg) \
  { \
  if ((expr) == 1) { \
    fprintf (stderr, "FATAL ERROR: %s\n", errMsg); \
    fprintf (stderr, "             File %s -> Line %d\n", \
      __FILE__, __LINE__); \
    fflush (stderr); \
    return (DDDMP_FAILURE); \
    } \
  }

/**Macro***********************************************************************
 
  Synopsis    [Checks for fatal bugs and go to the label to deal with
    the error.
    ]
 
  Description []
 
  SideEffects [None]
 
  SeeAlso     []
 
******************************************************************************/

#define Pdtutil_CheckAndGotoLabel(expr,errMsg,label) \
  { \
  if ((expr) == 1) { \
    fprintf (stderr, "FATAL ERROR: %s\n", errMsg); \
    fprintf (stderr, "             File %s -> Line %d\n", \
      __FILE__, __LINE__); \
    fflush (stderr); \
    goto label; \
    } \
  }

/**Macro***********************************************************************

  Synopsis    []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

#define Pdtutil_Alloc(type,num) \
    ((type *)Pdtutil_AllocCheck(ALLOC(type, num)))

/**Macro***********************************************************************

  Synopsis    []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

#define Pdtutil_Realloc(type,obj,num) \
    ((type *)Pdtutil_AllocCheck(REALLOC(type, obj, num)))

/**Macro***********************************************************************

  Synopsis    []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

#if 0
#define Pdtutil_Free(obj) \
    (FREE(obj))
#else
#define Pdtutil_Free(obj) \
  {Pdtutil_FreeCall((void *)obj);		\
    obj=NULL;}
#endif


/**Macro***********************************************************************
  Synopsis    [opt list operations]
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/

#define Pdtutil_OptListIns(list,tfield,tag,dfield,data) {	\
  Pdtutil_OptItem_t listItem;\
  listItem.optTag.tfield = tag;\
  listItem.optData.dfield = data;\
  Pdtutil_OptListInsertTail(list, listItem);\
}

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EXTERN void Pdtutil_InitCpuTimeScale(
  int num
);
EXTERN void *Pdtutil_AllocCheck(
  void *pointer
);
EXTERN void Pdtutil_ChrPrint(
  FILE * fp,
  char c,
  int n
);
EXTERN Pdtutil_VerbLevel_e Pdtutil_VerbosityString2Enum(
  char *string
);
EXTERN const char *Pdtutil_VerbosityEnum2String(
  Pdtutil_VerbLevel_e enumType
);
EXTERN Pdtutil_VariableOrderFormat_e Pdtutil_VariableOrderFormatString2Enum(
  char *string
);
EXTERN const char *Pdtutil_VariableOrderFormatEnum2String(
  Pdtutil_VariableOrderFormat_e enumType
);
EXTERN char *Pdtutil_FileName(
  char *filename,
  char *attribute,
  char *extension,
  int overwrite
);
EXTERN FILE *Pdtutil_FileOpen(
  FILE * fp,
  char *filename,
  const char *mode,
  int *flag
);
EXTERN void Pdtutil_FileClose(
  FILE * fp,
  int *flag
);
EXTERN void Pdtutil_ReadSubstring(
  char *stringIn,
  char **stringFirstP,
  char **stringSecondP
);
EXTERN void Pdtutil_ReadName(
  char *extName,
  int *nstr,
  char **names,
  int maxNstr
);
EXTERN char *Pdtutil_StrDup(
  char *str
);
EXTERN int Pdtutil_OrdRead(
  char ***pvarnames,
  int **pvarauxids,
  char *filename,
  FILE * fp,
  Pdtutil_VariableOrderFormat_e ordFileFormat
);
EXTERN int Pdtutil_OrdWrite(
  char **varnames,
  int *varauxids,
  int *sortedIds,
  int nvars,
  char *filename,
  FILE * fp,
  Pdtutil_VariableOrderFormat_e ordFileFormat
);
EXTERN char **Pdtutil_StrArrayDup(
  char **array,
  int n
);
EXTERN int Pdtutil_StrArrayRead(
  char ***parray,
  FILE * fp
);
EXTERN int Pdtutil_StrArrayWrite(
  FILE * fp,
  char **array,
  int n
);
EXTERN void Pdtutil_StrArrayFree(
  char **array,
  int n
);
EXTERN int *Pdtutil_IntArrayDup(
  int *array,
  int n
);
EXTERN int Pdtutil_IntArrayRead(
  int **parray,
  FILE * fp
);
EXTERN int Pdtutil_IntArrayWrite(
  FILE * fp,
  int *array,
  int n
);
EXTERN void Pdtutil_CatchExit ARGS(
   (int)
);
EXTERN JMPBUF *util_newlongjmp(
  void
);
EXTERN void util_longjmp ARGS(
   (void)
);
EXTERN void util_cancellongjmp ARGS(
   (void)
);
EXTERN void util_resetlongjmp ARGS(
   (void)
);
EXTERN void Pdtutil_FreeCall(
  void *p
);
EXTERN unsigned long Pdtutil_ConvertTime(
  unsigned long t
);
EXTERN char *Pdtutil_ArrayReadName(
  Pdtutil_Array_t * obj
);
EXTERN void Pdtutil_ArrayWriteName(
  Pdtutil_Array_t * obj,
  char *name
);

/* Pdtutil_List functions */
EXTERN Pdtutil_List_t *Pdtutil_ListCreate(
  void
);
EXTERN void Pdtutil_ListInsertHead(
  Pdtutil_List_t * list,
  Pdtutil_Item_t item
);
EXTERN void Pdtutil_ListInsertTail(
  Pdtutil_List_t * list,
  Pdtutil_Item_t item
);
EXTERN Pdtutil_Item_t Pdtutil_ListExtractHead(
  Pdtutil_List_t * list
);
EXTERN Pdtutil_Item_t Pdtutil_ListExtractTail(
  Pdtutil_List_t * list
);
EXTERN int Pdtutil_ListSize(
  Pdtutil_List_t * list
);
EXTERN void Pdtutil_ListFree(
  Pdtutil_List_t * list
);

/* Pdtutil_OptList functions */
EXTERN Pdtutil_OptList_t *Pdtutil_OptListCreate(
  const Pdt_OptModule_e module
);
EXTERN Pdt_OptModule_e Pdtutil_OptListReadModule(
  Pdtutil_OptList_t * optList
);
EXTERN void Pdtutil_OptListInsertHead(
  Pdtutil_OptList_t * list,
  Pdtutil_OptItem_t item
);
EXTERN void Pdtutil_OptListInsertTail(
  Pdtutil_OptList_t * list,
  Pdtutil_OptItem_t item
);
EXTERN Pdtutil_OptItem_t Pdtutil_OptListExtractHead(
  Pdtutil_OptList_t * list
);
EXTERN int Pdtutil_OptListEmpty(
  Pdtutil_OptList_t * optList
);
EXTERN Pdtutil_OptItem_t Pdtutil_OptListExtractTail(
  Pdtutil_OptList_t * list
);
EXTERN int Pdtutil_OptListSize(
  Pdtutil_OptList_t * list
);
EXTERN void Pdtutil_OptListFree(
  Pdtutil_OptList_t * list
);

/* Pdtutil_PointerArray*() functions */
EXTERN Pdtutil_Array_t *
Pdtutil_PointerArrayAlloc(
  size_t size
);
EXTERN void *
Pdtutil_PointerArrayRead(
  Pdtutil_Array_t * obj,
  size_t idx
);
EXTERN void
Pdtutil_PointerArrayWrite(
  Pdtutil_Array_t * obj,
  size_t idx,
  void *val
);
EXTERN size_t
Pdtutil_PointerArraySize(
  Pdtutil_Array_t * obj
);
EXTERN size_t
Pdtutil_PointerArrayNum(
  Pdtutil_Array_t * obj
);
EXTERN void
Pdtutil_PointerArrayFree(
  Pdtutil_Array_t * obj
);

/* Pdtutil_IntegerArray*() functions */
EXTERN Pdtutil_Array_t *Pdtutil_IntegerArrayAlloc(
  size_t size
);
EXTERN int Pdtutil_IntegerArrayRead(
  Pdtutil_Array_t * obj,
  size_t idx
);
EXTERN void Pdtutil_IntegerArrayWrite(
  Pdtutil_Array_t * obj,
  size_t idx,
  int val
);
EXTERN void Pdtutil_IntegerArrayInsert(
  Pdtutil_Array_t * obj,
  int val
);
EXTERN void Pdtutil_IntegerArrayInsertLast(
  Pdtutil_Array_t * obj,
  int val
);
EXTERN void Pdtutil_IntegerArrayReset(
  Pdtutil_Array_t * obj
);
EXTERN int Pdtutil_IntegerArrayRemove(
  Pdtutil_Array_t * obj,
  size_t idx
);
EXTERN int Pdtutil_IntegerArrayRemoveLast(
  Pdtutil_Array_t * obj
);
EXTERN size_t Pdtutil_IntegerArraySize(
  Pdtutil_Array_t * obj
);
EXTERN size_t Pdtutil_IntegerArrayNum(
  Pdtutil_Array_t * obj
);
EXTERN void Pdtutil_IntegerArrayIncNum(
  Pdtutil_Array_t * obj
);
EXTERN void Pdtutil_IntegerArrayDecNum(
  Pdtutil_Array_t * obj
);
EXTERN void Pdtutil_IntegerArrayResize(
  Pdtutil_Array_t * obj,
  size_t size
);
EXTERN void Pdtutil_IntegerArraySort(
  Pdtutil_Array_t * obj,
  Pdtutil_ArraySort_e dir
);
EXTERN void Pdtutil_IntegerArrayFree(
  Pdtutil_Array_t * obj
);
EXTERN void Pdtutil_LogMsg(
  int thrdId,
  const char *fmt,
  ...
);
EXTERN void
Pdtutil_Printf(
  const char *fmt,
  ...
);
EXTERN Pdtutil_EqClasses_t *Pdtutil_EqClassesAlloc(
  int nCl
);
EXTERN void Pdtutil_EqClassesFree(
  Pdtutil_EqClasses_t * eqCl
);
EXTERN void Pdtutil_EqClassesReset(
  Pdtutil_EqClasses_t * eqCl
);
EXTERN void Pdtutil_EqClassesSplitLdrReset(
  Pdtutil_EqClasses_t * eqCl
);
EXTERN void Pdtutil_EqClassSetVal(
  Pdtutil_EqClasses_t * eqCl,
  int i,
  int val
);
EXTERN int Pdtutil_EqClassVal(
  Pdtutil_EqClasses_t * eqCl,
  int i
);
EXTERN int Pdtutil_EqClassIsCompl(
  Pdtutil_EqClasses_t * eqCl,
  int i
);
EXTERN int Pdtutil_EqClassIsLdr(
  Pdtutil_EqClasses_t * eqCl,
  int i
);
EXTERN int Pdtutil_EqClassNum(
  Pdtutil_EqClasses_t * eqCl,
  int i
);
EXTERN int Pdtutil_EqClassLdr(
  Pdtutil_EqClasses_t * eqCl,
  int i
);
EXTERN void Pdtutil_EqClassSetLdr(
  Pdtutil_EqClasses_t * eqCl,
  int i,
  int l
);
EXTERN void Pdtutil_EqClassSetSingleton(
  Pdtutil_EqClasses_t * eqCl,
  int i
);
EXTERN void Pdtutil_EqClassSetState(
  Pdtutil_EqClasses_t * eqCl,
  int i,
  Pdtutil_EqClassState_e state
);
EXTERN Pdtutil_EqClassState_e Pdtutil_EqClassUpdate(
  Pdtutil_EqClasses_t * eqCl,
  int i
);
EXTERN void Pdtutil_EqClassCnfVarWrite(
  Pdtutil_EqClasses_t * eqCl,
  int i,
  int val
);
EXTERN void Pdtutil_EqClassCnfActWrite(
  Pdtutil_EqClasses_t * eqCl,
  int i,
  int act
);
EXTERN int Pdtutil_EqClassCnfVarRead(
  Pdtutil_EqClasses_t * eqCl,
  int i
);
EXTERN int Pdtutil_EqClassCnfActRead(
  Pdtutil_EqClasses_t * eqCl,
  int i
);
EXTERN int Pdtutil_EqClassesNum(
  Pdtutil_EqClasses_t * eqCl
);
EXTERN void Pdtutil_EqClassesSetUndefined(
  Pdtutil_EqClasses_t * eqCl,
  int undefinedVal
);
EXTERN int Pdtutil_EqClassesSplitNum(
  Pdtutil_EqClasses_t * eqCl
);
EXTERN int Pdtutil_EqClassesRefineStepsNum(
  Pdtutil_EqClasses_t * eqCl
);
EXTERN Pdtutil_EqClassState_e Pdtutil_EqClassState(
  Pdtutil_EqClasses_t * eqCl,
  int i
);
EXTERN int Pdtutil_EqClassesInitialized(
  Pdtutil_EqClasses_t * eqCl
);
EXTERN void Pdtutil_EqClassesSetInitialized(
  Pdtutil_EqClasses_t * eqCl
);
EXTERN char *Pdtutil_PrintTime(
  unsigned long t
);
EXTERN long Pdtutil_CpuTime(
  void
);
EXTERN double Pdtutil_WallClockTime(
  void
);
EXTERN void Pdtutil_WallClockTimeReset(
  void
);
EXTERN long Pdtutil_ProcRss(void);
EXTERN long Pdtutil_ThrdRss(void);
EXTERN long Pdtutil_CpuTimeCuddLegacy(
  void
);
EXTERN int Pdtutil_WresNum(
  char *wRes,
  char *prefix,
  int num,
  int chkMax,
  int chkEnabled,
  int phase,
  int initStubSteps,
  int unfolded
);
EXTERN void
Pdtutil_HwmccOutputSetup(
);
EXTERN void Pdtutil_WresSetup(
  char *name,
  int gblNum
);
EXTERN char *Pdtutil_WresFileName(
  void
);
EXTERN void Pdtutil_WresSetTwoPhase(
);
EXTERN void Pdtutil_WresIncrInitStubSteps(
  int steps
);
EXTERN int Pdtutil_WresReadNum(
  int phase,
  int initStubSteps
);
EXTERN Pdtutil_MapInt2Int_t *Pdtutil_MapInt2IntAlloc(
  int mDir,
  int mInv,
  int defaultValue,
  Pdtutil_MapInt2Int_e type
);
EXTERN void Pdtutil_MapInt2IntWrite(
  Pdtutil_MapInt2Int_t * map,
  int i,
  int j
);
EXTERN int Pdtutil_MapInt2IntReadDir(
  Pdtutil_MapInt2Int_t * map,
  int i
);
EXTERN int Pdtutil_MapInt2IntReadInv(
  Pdtutil_MapInt2Int_t * map,
  int j
);
EXTERN void Pdtutil_MapInt2IntFree(
  Pdtutil_MapInt2Int_t * map
);
EXTERN void Pdtutil_InfoArraySort(
  Pdtutil_SortInfo_t * SortInfoArray,
  int n,
  int increasing
);
EXTERN int Pdtutil_StrRemoveNumSuffix(
  char *str,
  char separator
);
EXTERN int Pdtutil_StrGetNumSuffix(
  char *str,
  char separator
);
EXTERN char *Pdtutil_StrSkipPrefix(
  char *str,
  char *prefix
);

/**AutomaticEnd***************************************************************/

#endif                          /* _PDTUTIL */

#ifdef stdout
#undef stdout
#endif
#define stdout pdtStdout
#ifdef printf
#undef printf
#endif
#define printf Pdtutil_Printf


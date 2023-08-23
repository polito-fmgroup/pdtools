/**CHeaderFile*****************************************************************

  FileName    [ddiInt.h]

  PackageName [ddi]

  Synopsis    [Decision diagram interface for the PdTrav package.]

  Description [External functions and data strucures of the DDI package]

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

#ifndef _DDIINT
#define _DDIINT

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include "ddi.h"
#include "part.h"
#include <stdarg.h>

// StQ 2010.07.07
//#include "SAT_C.h"
#include "Solver.h"

#define __STDC_LIMIT_MACROS
#define __STDC_FORMAT_MACROS
#include <stdint.h>
extern "C" {
#define RMSHFT 0
#include "lglib.h"
#include "lglconst.h"
}

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/


#define DEVEL_OPT_ANDEXIST 0


#define DDI_MAX_CU_VAR_NUM      (CUDD_MAXINDEX-1)  

#define DDI_ARRAY_INIT_SIZE      4  

#define DDI_MAX_BDDARRAY_LEN     100000  

/* 
 *  activate garbage collection any specified number of free 
 *  This is only handle management, BDD nodes are dereferenced 
 *  any time a free is called.
 */
#define DDI_GARBAGE_THRESHOLD    500000


/* 
 * max value in enum. Used for array size and iteration limit 
 */
#define DDI_NTYPE ((int)Ddi_Exprarray_c+1)

#define DDI_META_GROUP_SIZE_MIN_DEFAULT    10  

#define DDI_ITP_ITEOPT_TH_DEFAULT          50000
#define DDI_ITP_PART_TH_DEFAULT            100000
#define DDI_ITP_STORE_TH_DEFAULT            30000

#define DDI_VARARG_LIST ...

#define DDI_MAX_NESTED_ABORT_ON_SIFT       10

#define DDI_AIG_SIGNATURE_SLOTS            32
#define DDI_AIG_SIGNATURE_BITS \
(DDI_AIG_SIGNATURE_SLOTS*8*sizeof(unsigned long))

#define DDI_INTERP_NULL_NODE_INDEX    0
#define DDI_INTERP_ZERO_NODE_INDEX    1

#define interpArrayMaxSize            (1 << 27)

#define Cudd_IsVar(node) \
  (Cudd_IsConstant(Cudd_T(node))&&Cudd_IsConstant(Cudd_E(node)))

#define USE_SIMP 1
#if USE_SIMP
#define Minisat22Solver Minisat::SimpSolver
#else
#define Minisat22Solver Minisat::Solver
#endif
#define Minisat22SolverPdt Minisat::SolverPdt

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

typedef struct Ddi_Meta_t Ddi_Meta_t;
typedef struct Ddi_ArrayData_t Ddi_ArrayData_t;
typedef struct DdNode Ddi_BddNode_t;
typedef struct Ddi_Aig_t Ddi_Aig_t;

/**Enum************************************************************************
  Synopsis    [DDI block types.]
  Description [DDI block types. Used for run time checks]
******************************************************************************/

typedef enum {
  Ddi_Bdd_c,
  Ddi_Var_c,
  Ddi_Varset_c,
  Ddi_Expr_c,
  Ddi_Bddarray_c,
  Ddi_Vararray_c,
  Ddi_Varsetarray_c,
  Ddi_Exprarray_c
}
Ddi_Type_e;

/**Enum************************************************************************
  Synopsis    [DDI block status.]
  Description [DDI block status.]
******************************************************************************/
typedef enum {
  Ddi_Free_c,
  Ddi_Unlocked_c,
  Ddi_Locked_c,
  Ddi_SchedFree_c,
  Ddi_Null_c
}
Ddi_Status_e;

/**Enum************************************************************************
  Synopsis    [Selector for accumulate/generate operation type.]
******************************************************************************/
typedef enum {
  Ddi_Accumulate_c,
  Ddi_Generate_c
} Ddi_OpType_e;

/**Enum************************************************************************
  Synopsis    [Selector for copy operation type.]
******************************************************************************/
typedef enum {
  Ddi_Dup_c,
  Ddi_Mov_c
} Ddi_CopyType_e;

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

/**Struct*********************************************************************
 Synopsis    [AIG signature]
 Description [AIG signatire]
******************************************************************************/

typedef struct Ddi_AigSignature_t {
  unsigned long s[DDI_AIG_SIGNATURE_SLOTS];
} Ddi_AigSignature_t;

typedef struct Ddi_AigSignatureArray_t {
  int currBit;
  int nSigs;
  Ddi_AigSignature_t *sArray;
} Ddi_AigSignatureArray_t;

/**Struct*********************************************************************
 Synopsis    [AIG dynamic signature]
 Description [AIG dynamic signatire]
******************************************************************************/

typedef struct Ddi_AigDynSignature_t {
  unsigned long *s;
  unsigned int size;
} Ddi_AigDynSignature_t;

typedef struct Ddi_AigDynSignatureArray_t {
  int currBit;
  int nSigs;
  int nBits;
  int size;
  Ddi_AigDynSignature_t **sArray;
} Ddi_AigDynSignatureArray_t;

/**Enum************************************************************************
  Synopsis    [COI statistics type.]
  Description [COI statistics type.]
******************************************************************************/
typedef enum {
  Ddi_CoiIntersectionOverUnion_c,
  Ddi_CoiIntersectionOverLeft_c,
  Ddi_CoiIntersectionOverRight_c,
  Ddi_CoiNull_c
}
Ddi_CoiStats_e;


/**Enum************************************************************************
  Synopsis    [DDI ternary gate types.]
  Description []
******************************************************************************/

typedef enum {
  Ddi_And_c,
  Ddi_Const_c,
  Ddi_FreePi_c,
  Ddi_Po_c,
  Ddi_Ps_c,
  Ddi_Pi_c
}
Ddi_TernaryGateType_e;


/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

typedef enum {
  Ddi_MapObj_None_c,
  Ddi_MapObj_Array_c,
  Ddi_MapObj_SymTab_c
} Ddi_MapObj_e;

typedef enum {
  Ddi_SccNode_In_c,
  Ddi_SccNode_Out_c,
  Ddi_SccNode_Internal_c,
  Ddi_SccNode_GateSingle_c,
  Ddi_SccNode_PiSingle_c,
  Ddi_SccNode_LatchSingle_c,
  Ddi_SccNode_LatchSingleWithGates_c,
  Ddi_SccNode_None_c
} Ddi_SccNodeType_e;

typedef enum {
  Ddi_SccNodeStatus_InStack_c = 0x01, //bit 0
  Ddi_SccNodeStatus_Disabled_c = 0x02, //bit 1
  Ddi_SccNodeStatus_RightDisabled_c = 0x04, //bit 2
  Ddi_SccNodeStatus_LeftDisabled_c = 0x08, //bit 3
  Ddi_SccNodeStatus_None_c = 0x00
} Ddi_SccNodeStatusType_e;

typedef enum {
  Ddi_SccBridge_RightBridge_c,
  Ddi_SccBridge_LeftBridge_c,
  Ddi_SccBridge_BothBridge_c,
  Ddi_SccBridge_None_c
} Ddi_SccNodeChildBridge_e;

typedef enum {
  Ddi_Scc_GateSingle_c,
  Ddi_Scc_PiSingle_c,
  Ddi_Scc_LatchSingle_c,
  Ddi_Scc_LatchSingleWithGates_c,
  Ddi_Scc_LatchMultiple_c,
  Ddi_Scc_None_c
} Ddi_SccType_e;

struct Ddi_Signature_t {
  unsigned int *sign;   /* size may be computed using Ddi_VararrayNum() */
  Ddi_Vararray_t *vars; /* the input variables array                    */
};

struct Ddi_Signaturearray_t {
  Ddi_Signature_t **signs; /* signatures array              */
  int num;                 /* number of signatures elements */
  int size;                /* array size                    */
  Solver *S;               /* sat solver                    */
};

struct Ddi_SccMgr_t {
  Ddi_Mgr_t *ddiMgr;            //Ddi manager reference
  int nBitmapMax;
  int nScc;                     //Number of scc
  int nSccGate;                 //Number of scc composed of only one gate
  int nSccWithLatch;            //Number of scc with latches
  int nLatches;                 //Number of latches
  int nNodes;                   //Number of nodes
  int sccWithMaxLatches;        //largest scc
  Ddi_Bddarray_t *delta;        //Riferimento alla delta
  Ddi_Vararray_t *ps;           //Riferimento alle variabili di stato
  int *sccLatchCnt;             //Counters of latches per scc
  int *sccGateCnt;              //Counters of gates per scc
  int *sccInternalNodes;        //Counters of internal nodes per scc
  int *sccInputNodes;           //Counters of input nodes per scc
  int *sccOutputNodes;          //Counters of output nodes per scc
  int *mapScc;                  //Mappa indice nodo in topological nodes ---> scc di cui è leader
  int *sccSize;
  int *sccInGate;
  int *sccInCnt;
  int *sccBridges;
  int *sccBridgesCnt;
  int *sccOutGate;
  int *sccOutCnt;
  int *sccGateFoCnt;
  int *mapSccBit;
  int *mapNs;                   //Mappa indice nodo in topological nodes dei latch ---> indice in topological nodes delle delta root
  int *mapLatches;
  Ddi_AigDynSignatureArray_t *sccSigs;
  bAig_array_t *aigStack;             //Helper stack for tarjan scc algorithm
  bAig_array_t *aigNodes;             //Elenco dei nodi visitati nell'ordine con cui vengono visitati
  bAig_array_t *sccTopologicalNodes;  //Ordinamento topologico delle SCC
  int *sccTopologicalLevel;
  int *sccLatchTopologicalLevel;
  int *sccGateTopologicalLevelMin;
  int *sccGateTopologicalLevelMax;
  int size1;
  int size2;
  Ddi_SccType_e *sccType;
  Ddi_SccNodeType_e *sccNodeTypeTopological;
};

struct Ddi_Map_t {
  Ddi_MapType_e type;       /* map type */
  Ddi_MapObj_e map_obj;     /* map object (array, symbol table) */
  void *a_mgr;              /* a manager (e.g., bAig_Mgr_t) */
  union {
    Pdtutil_Array_t *array; /* a[i] = b_node, mapping array */
    st_table *symtab;       /* a symbol table */
  } a_data;
  void *b_mgr;              /* b manager (e.g., Solver) */
  union {
    Pdtutil_Array_t *array; /* a[j] = a_node, mapping array */
    st_table *symtab;       /* b symbol table */
  } b_data;
  size_t size;              /* initial size of mapping obj */
  size_t num;               /* incremental num of mapping obj */
};

typedef struct {
  Pdtutil_VerbLevel_e verbosity;
  FILE *stdout;
  struct {
    int groupSizeMin;
    int maxSmoothVars;
    Ddi_Meta_Method_e method;
  } meta;
  struct {
    int existClustThresh;
    int existOptClustThresh;
    int existOptLevel;
    int existDisjPartTh;
  } part;
  struct {
    float lazyRate;
    char *satSolver;
    int abcOptLevel;
    int bddOptLevel;
    int redRemLevel;
    int redRemMaxCnt;
    int redRemCnt;
    int itpIteOptTh;
    int itpStructOdcTh;
    int itpPartTh;
    int itpStoreTh;
    int itpDrup;
    int itpUseCare;
    int nnfClustSimplify;
    int dynAbstrStrategy;
    int ternaryAbstrStrategy;
    int maxAllSolutionIter;
    int satIncrByRefinement;
    int satTimeout;
    int satIncremental;
    int satVarActivity;

    int satCompare;
    int bddCompare;
      
    int satTimeLimit;
    int satTimeLimit1;
    int enMetaOpt;
      
    int lemma_done_bound;
      
    int itpOpt;
    int itpActiveVars;
    int itpAbc;
    int itpTwice;
    int itpReverse;
    int itpNnfAbstrAB;
    int itpNoQuantify;
    int itpAbortTh;
    int itpAigCore;
    int itpCore;
    int itpMem;
    int itpMap;
    char *itpStore;
    int itpLoad;
    int itpCompute;
    int partialItp;
    int itpCompact;    
    int itpClust;
    int itpNorm;
    int itpSimp;
      
    int enItpBddOpt;
    int enBddFoConOpt;
    int aigCnfLevel;    
  } aig;
  struct {
    int partOnSingleFile;
  } dump;

} Ddi_Settings_t;

/**Struct*********************************************************************

 Synopsis    [Dd Manager]

 Description [The Ddi Dd manager]

******************************************************************************/

struct Ddi_Mgr_t {
  char *name;                            /* DDI Structure Name */
  DdManager *mgrCU;                      /* CUDD manager */
  struct Ddi_Bdd_t *one;                 /* one constant */
  struct Ddi_Bdd_t *zero;                /* zero constant */
  char **varnames;                       /* names of variables */
  st_table *varNamesIdxHash;             /* var names->idx hash table */
  /*int *revVarAuxids;*/                 /* reversed auxidx->idx vector */
  int *varauxids;                        /* auxiliary variable ids */
  int *varIdsFromCudd;                   /* ids from cudd variable ids */
  bAigEdge_t *varBaigIds;                /* baig variable ids */
  int vararraySize;                      /* size of varnames and varauxids */

  Ddi_Varsetarray_t *varGroups;          /* groups of variables */

  int currNodeId;                        /* integer unique id of DDI node */
  int tracedId;                          /* ids >= tracedId are logged 
                                            at creation */
  int freeNum;
  int lockedNum;
  int allocatedNum;
  int genericNum;
  int typeNum[DDI_NTYPE];
  int typeLockedNum[DDI_NTYPE];

  union Ddi_Generic_t *nodeList;
  union Ddi_Generic_t *freeNodeList;
  int freeNodeListNum;

  struct Ddi_Vararray_t *variables;
  struct Ddi_Vararray_t *cuddVariables;
  struct Ddi_Vararray_t *baigVariables;

  struct {
    int varSize, baigVarSize;
    int *varCnt;
    int *baigVarCnt;
  } varCntSortData;

  Cudd_ReorderingType autodynMethod; /* suspended autodyn method */
  int autodynEnabled;                /* flag active if dynord enabled */
  int autodynMasked;                 /* flag active if dynord masked */
  int autodynSuspended;              /* flag active if dynord suspended */
  int autodynNestedSuspend;          /* counter of nested dynord suspend */
  int abortOnSift;                   /* enable abort op when sifting called */
  int abortOnSiftMasked;             /* mask abort on sift activation */
  int abortedOp;                     /* flagging logging aborted operation */
  int autodynNextDyn;                /* threshold for next autodyn (sift) */
  int abortOnSiftThresh[DDI_MAX_NESTED_ABORT_ON_SIFT];
                                     /* abort on sift threshold */
  int autodynMaxTh;                  /* max autodyn th: disabled if greater */
  int autodynMaxCalls;               /* max num of sift calls; then disabled */

  struct {
    int groupNum;
    Ddi_Varsetarray_t *groups; 
    int nvar;
    int *ord;
    int bddNum;
    int id;
  } meta;

  struct {
    int clipDone;
    int clipRetry;
    int clipLevel;
    int clipThresh;
    Ddi_Bdd_t *clipWindow;
    Ddi_Bdd_t *activeWindow;
    int ddiExistRecurLevel;
    Ddi_Bdd_t *conflictProduct;
    cuplus_and_abs_info_t *infoPtr;
  } exist;

  struct {
    bAig_Manager_t *mgr;
    Ddi_Bddarray_t *auxLits;
    Ddi_Bddarray_t *auxLits2;
    Ddi_Bddarray_t *actVars;
  } aig;

  struct {
    int maxCnfId;
    int currCnfId;
    int cnf2aigSize;
    int *cnf2aig;
    unsigned char *cnfActive;
    int cnf2aigOpen;
    int cnf2aigStrategy;
    int *solver2cnf;
    int solver2cnfSize;
    int useSavedAigsForSat;
    bAig_array_t *itpExtraGlobalNodes;
    bAig_array_t *savedAigNodes;
  } cnf;

  Ddi_Settings_t settings;

  /*
   *  BDD Statistics
   */

  struct {
    int peakProdLocal;
    int peakProdGlobal;
    float siftCompactionLast;
    float siftCompactionPred;
    int siftRunNum;
    int siftCurrOrdNodeId;
    long siftLastInit;
    long siftLastTime;
    long siftTotTime;

    struct {
      int n_merge_1;
      int n_merge_2;
      int n_merge_3;
      int n_check_1;
      int n_check_2;
      int n_check_3;
      int n_diff;
      int n_diff_1;
      int fullAndItpTerms;
      int fullOrItpTerms;
      int itpTerms;
      int itpPartialExist;
    } aig;

  } stats;

  /*
   *  Limits for DDI operations:
   *  * Memory Limit (in bytes) (-1 = NO limit)
   *  * CPU Time Limit (in seconds) (-1 = NO limit)
   */

  struct {
    long int memoryLimit;
    long int timeInitial;
    long int timeLimit;
  } limit;

  struct {
    int *elseFirst;
    int initElseFirst;
  } cuPlus;

};	

/**Struct*********************************************************************
 Synopsis  [Ddi Node Extra Info]
 Description [Ddi Node Extra Info]
******************************************************************************/
union Ddi_Info_t {
  Ddi_Info_e infoCode;
  struct {
    Ddi_Info_e infoCode;
    int mark;
    double activity;
  } var;
  struct {
    Ddi_Info_e infoCode;
    int mark;
    void *auxPtr;
  } bdd;
  struct {
    Ddi_Info_e infoCode;
    int mark;
    Ddi_Vararray_t *vars;
    Ddi_Bddarray_t *subst;
  } eq;
  struct {
    Ddi_Info_e infoCode;
    int mark;
    Ddi_Bdd_t *f;
    Ddi_Bdd_t *care;
    Ddi_Bdd_t *constr;
    Ddi_Bdd_t *cone;
    Ddi_Vararray_t *refVars;
    Ddi_Vararray_t *vars;
    Ddi_Bddarray_t *subst;
  } compose;
};

/**Struct*********************************************************************
 Synopsis  [Meta BDD description]
 Description [Meta BDD description]
******************************************************************************/

struct Ddi_Meta_t {
  Ddi_Bddarray_t *one, *zero, *dc;
  int metaId;
};

/**Struct*********************************************************************
 Synopsis  [Aig description]
 Description [Aig description]
******************************************************************************/

struct Ddi_Aig_t {
  bAig_Manager_t *mgr;
  bAigEdge_t aigNode;
};

/**Struct*********************************************************************
 Synopsis  [DDI Arrays]
******************************************************************************/

struct Ddi_ArrayData_t {
  int num;              /* number of array elements */
  int nSize;            /* size of allocated data (in number of elements) */
  Ddi_Generic_t **data; /* ptr to the array data*/
};

/**Struct*********************************************************************
 Synopsis  [common fields]
 Description [fields present in all DDI node types]
******************************************************************************/

struct Ddi_Common_t {
  Ddi_Type_e              type;   /* type, for run time checks */
  Ddi_Code_e              code;   /* code, for selection within type */
  Ddi_Status_e            status; /* status */
  Ddi_Mgr_t               *mgr;   /* the manager */
  char                    *name;  /* optional name, useful for debug */
  union Ddi_Generic_t     *next;  /* pointer for linked lists */
  union Ddi_Info_t        *info;  /* pointer to info block */
  union Ddi_Generic_t     *supp;  /* support */
  int                     size; /* integer id: set for debug purposes */
  int                     nodeid; /* integer id: set for debug purposes */
};	

/**Struct*********************************************************************
 Synopsis  [Boolean function]
 Description [Handle to a Boolean function (a BDD or a partitioned BDD)]
******************************************************************************/

struct Ddi_Bdd_t {
  struct Ddi_Common_t     common;  /* common fields */
  union {
    Ddi_BddNode_t          *bdd;   /* ptr to the top node of the function */
    struct Ddi_ArrayData_t *part;  /* array of partitions */
    struct Ddi_Meta_t      *meta;  /* Meta BDD */
    struct Ddi_Aig_t       *aig;   /* Aig node */
  } data;
};	

/**Struct*********************************************************************
 Synopsis  [Array of Boolean functions]
 Description [Handle to an array of Boolean functions]
******************************************************************************/

struct Ddi_Bddarray_t {
  struct Ddi_Common_t     common; /* common fields */
  struct Ddi_ArrayData_t  *array; /* array data */
};	

/**Struct*********************************************************************
 Synopsis  [Variable]
 Description [Handle to a variable]
******************************************************************************/

struct Ddi_Var_t {
  struct Ddi_Common_t     common; /* common fields */
  struct {
    union {
      Ddi_BddNode_t         *bdd;  /* ptr to the BDD node of the variable */
      bAigEdge_t            aig;   /* ptr to the baig node of the variable */
    } p;
    int                   index;  /* variable index */
  } data;
};	

/**Struct*********************************************************************
 Synopsis  [Array of variables]
 Description [Handle to an array of variables]
******************************************************************************/

struct Ddi_Vararray_t {
  struct Ddi_Common_t     common; /* common fields */
  struct Ddi_ArrayData_t  *array; /* array data */
};	

/**Struct*********************************************************************
 Synopsis  [Variable set]
 Description [Handle to a set of variables (implemented as a Boolean function).
              Variable sets are repressnted as Boolean cubes. So this set
              completely replicates Ddi_dd_t. Internally, all operations are
              handled by casting Ddi_Varset_t* to Ddi_Bdd_t*
             ]
******************************************************************************/

struct Ddi_Varset_t {
  struct Ddi_Common_t     common; /* common fields */
  union {
    struct {
      Ddi_BddNode_t         *bdd;  
      Ddi_BddNode_t         *curr;  /* ptr to the current node within walk */
    } cu;
    struct {
      Ddi_Vararray_t        *va;
      int                    curr;
    } array;
  } data;
};	

/**Struct*********************************************************************
 Synopsis  [Array of variable sets]
 Description [Handle to an array of set of variables]
******************************************************************************/

struct Ddi_Varsetarray_t {
  struct Ddi_Common_t     common; /* common fields */
  struct Ddi_ArrayData_t  *array; /* array data */
};

/**Struct*********************************************************************
 Synopsis  [Expression]
 Description [Handle to an expression (Logic, CTL, etc.) represented as a tree]
******************************************************************************/

struct Ddi_Expr_t {
  struct Ddi_Common_t     common; /* common fields */
  union {
    Ddi_Generic_t          *bdd;  /* ptr to a decision diagram */
    struct Ddi_ArrayData_t *sub;  /* operand sub-expressions */
    char                   *string;  /* string identifier */
  } data;
  int                     opcode;
};

/**Struct*********************************************************************
 Synopsis  [Array of expressions]
 Description [Handle to an array of expressions (Logic, CTL, ...)]
******************************************************************************/

struct Ddi_Exprarray_t {
  struct Ddi_Common_t     common; /* common fields */
  struct Ddi_ArrayData_t  *array; /* array data */
};


/**Struct*********************************************************************
 Synopsis  [Generic DDI type]
 Description [Generic DDI type. Useful as general purpose block (e.g. as
              function parameter or array entry), mainly for 
              internal operations (bypassing compiler checks). 
              It is also used in block allocation, to guarantee equal size
              for all allocated DDI blocks]
******************************************************************************/

union Ddi_Generic_t {
  struct Ddi_Common_t      common;
  struct Ddi_Bdd_t         Bdd;
  struct Ddi_Var_t         Var;
  struct Ddi_Varset_t      Varset;
  struct Ddi_Expr_t        Expr;
  struct Ddi_Bddarray_t    Bddarray;
  struct Ddi_Vararray_t    Vararray;
  struct Ddi_Varsetarray_t Varsetarray;
  struct Ddi_Exprarray_t   Exprarray;
} ;


/**Struct*********************************************************************
 Synopsis    [CNF clauses]
 Description [CNF clauses]
******************************************************************************/

typedef struct Ddi_Clause_t {
  int sorted;
  int nRef;
  int size;
  int nLits;
  int *lits;
  Ddi_Clause_t *link;
  int id;
  int actLit;
  unsigned int signature0;
  unsigned int signature1;
} Ddi_Clause_t;

typedef struct Ddi_ClauseArray_t {
  int size;
  int nClauses;
  Ddi_Clause_t **clauses;
} Ddi_ClauseArray_t;

/**Struct*********************************************************************
 Synopsis    [Netlist to Clauses map info (with fanin)]
 Description [Netlist to Clauses map info (with fanin)]
******************************************************************************/
typedef struct Ddi_NetlistClausesInfo_t {
  int nRoot;
  int *roots;
  int nGate;
  int *fiIds[2];
  int *gateId2cnfId;		//MARCO: mantiene le corrispondenze tra gate root e cnf delle variabili a loro assegnate
  Pdtutil_Array_t **leaders;
  int *isRoot;
  int *clauseMapIndex;
  int *clauseMapNum;
} Ddi_NetlistClausesInfo_t;

/**Struct*********************************************************************
 Synopsis    [PDR data]
 Description [PDR data]
******************************************************************************/

typedef struct Ddi_PdrMgr_t {
  Ddi_Mgr_t *ddiMgr;
  int nState;
  int nInput;
  int useInitStub;
  int doPostOrderClauseLoad;
  Ddi_Vararray_t *pi;
  Ddi_Vararray_t *ps;
  Ddi_Vararray_t *ns;
  Ddi_Vararray_t *isVars;
  int maxAigVar;
  int freeCnfVar;
  int varRemapMarked;
  int *piCnf;
  int *psCnf;
  int *nsCnf;
  int *initLits;
  int *coiBuf;
  int *varActivity;
  int *varMark;
  Ddi_NetlistClausesInfo_t trNetlist;
  Ddi_Clause_t *piClause;
  Ddi_Clause_t *psClause;
  Ddi_Clause_t *nsClause;
  Ddi_Clause_t *isClause;
  Ddi_Clause_t *actTarget;
  Ddi_Clause_t *targetSuppClause;
  Ddi_ClauseArray_t *latchSupps;
  Ddi_ClauseArray_t *trClausesInit;
  Ddi_ClauseArray_t *trClauses;
  Ddi_ClauseArray_t *trClausesTseitin;
  Ddi_ClauseArray_t *invarClauses;
  Ddi_ClauseArray_t *initClauses;
  Ddi_ClauseArray_t *targetClauses;
  Ddi_Bddarray_t *bddSave;
} Ddi_PdrMgr_t;

/**Struct*********************************************************************
 Synopsis    [ternary simulation manager]
 Description [ternary simulation manager]
******************************************************************************/
typedef struct ternaryGateInfo {
  int *foIds;
  Ddi_TernaryGateType_e type;
  char val, saveVal, reqVal;
  char observed;
  int inStack;
  int fiIds[2];
  int foCnt;
  int level;
} ternaryGateInfo;

/**Struct*********************************************************************
 Synopsis    [ternary simulation manager]
 Description [ternary simulation manager]
******************************************************************************/
typedef struct Ddi_TernarySimMgr_t {
  Ddi_Mgr_t *ddiMgr;
  int nPi, nPo, nPs, nGate;
  ternaryGateInfo *ternaryGates;
  int *po;
  int *ps;
  int *pi;
  int target;
  int *undoList;
  int undoN;
  int *levelSize;
  int **levelStack;
  int *levelStackId;
  int currLevel;
  int nLevels;
} Ddi_TernarySimMgr_t;



/**Struct*********************************************************************
 Synopsis    [Minisat Aig CNF info]
 Description [Minisat Aig CNF info]
******************************************************************************/

typedef struct {
  int foCnt;
  int ldr;
  int level;
  int ref0, ref1;
  int actVar;
  vec<vec<Lit> > *ca;
  char isRoot;
  char isVarFlow;
  char isCutFrontier;
  char isCnfActive;
  char implied;
  int opIds[3];
} Ddi_MinisatAigCnfInfo_t;

/**Struct*********************************************************************
 Synopsis    [Minisat Aig CNF mgr]
 Description [Minisat Aig CNF mgr]
******************************************************************************/

typedef struct {
  Ddi_Mgr_t *ddiMgr;
  Ddi_MinisatAigCnfInfo_t *aigCnfInfo;
  Ddi_AigAbcInfo_t *aigAbcInfo;
  bAig_array_t *visitedNodes; 
  int *cutFrontier;
  int cutFrontierNum;
  int nNodes;
  int nFlow;
  int nNoFlow;
  int nRef;
  int aigCnfLevel;
  int phFilter;
} Ddi_MinisatAigCnfMgr_t;



/**Struct*********************************************************************
 Synopsis    [DDI incr sat mgr]
 Description [DDI incr SAT mgr]
******************************************************************************/

struct Ddi_IncrSatMgr_s {
  Solver        *S;
  void          *S22;
  void          *S22aux;
  void          *S22simp;
  LGL           *lgl;
  Ddi_SatSolver_t *ddiS;
  Ddi_MinisatAigCnfMgr_t *aigCnfMgr;
  Ddi_Mgr_t *ddiMgr;
  Ddi_Bddarray_t *dummyRefs;
  bAig_array_t *cnfMappedVars;
  vec<int> *lglLearned123;
  Ddi_Bddarray_t *implied;
  Ddi_Bddarray_t *refA;
  Ddi_Bddarray_t *eqA;
  int lglMaxFrozen;
  int simpSolver;
  int suspended;
  int enSimplify;
  int enProofLog;
  int enLglLearn;
};

/**Struct*********************************************************************
 Synopsis    [DDI sat solver]
 Description [DDI SAT solver]
******************************************************************************/

typedef struct Ddi_SatSolver_t {
  Solver    *S;
  void      *S22;
  int *freeVars;
  int *vars;
  int *map;
  int *mapInv;
  int nVars;
  int nFreeVars;
  int nFreeTot;
  int nVarsMax;
  int freeVarMinId;
  struct {
    char doIncr;
    Ddi_ClauseArray_t *clauses;
    Pdtutil_MapInt2Int_t *mapping;
  } incrSat;
  struct {
    int nFreeMax;
  } settings;
  float cpuTime;
} Ddi_SatSolver_t;

/**Struct*********************************************************************
 Synopsis    [DDI aig to cnf info]
 Description [DDI aig to cnf info]
******************************************************************************/

typedef struct {
  int foCnt;
  int ldr;
  int mapLdr;
  Ddi_ClauseArray_t *ca;
  char isRoot;
  char isMapped;
  int opIds[3];
} Ddi_AigCnfInfo_t;

/**Struct*********************************************************************
 Synopsis    [DDI resolution node structure]
 Description [DDI resolution node structure]
******************************************************************************/
/*
typedef struct {
  Ddi_ResolutionNodeType_e type;
  Ddi_Clause_t resolvent;
  Pdtutil_Array_t antecedents;
} Ddi_ResolutionNode_t;
*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#define dMgrO(ddiMgr) ((ddiMgr)->settings.stdout)

#define DdiType(n) (((Ddi_Generic_t*)n)->common.type)

#define DdiConsistencyCheck(n,t) \
{\
  Pdtutil_Assert(n!=NULL,"check failed: NULL DDI node");\
  Pdtutil_Assert(DdiType(n)==t,"wrong node type");\
}
#define DdiConsistencyCheck2(n,t1,t2) \
{\
  Pdtutil_Assert(n!=NULL,"check failed: NULL DDI node");\
  Pdtutil_Assert((DdiType(n)==t1)||(DdiType(n)==t2),"wrong node type");\
}
#define DdiConsistencyCheck3(n,t1,t2,t3) \
{\
  Pdtutil_Assert(n!=NULL,"check failed: NULL DDI node");\
  Pdtutil_Assert((DdiType(n)==t1)||(DdiType(n)==t2)||(DdiType(n)==t3),"wrong node type"); \
}

#define DdiGenericOp1(op,acc,f) \
  DdiGenericOp(op,acc,(Ddi_Generic_t*)(f),NULL,NULL)
#define DdiGenericOp2(op,acc,f,g) \
  DdiGenericOp(op,acc,(Ddi_Generic_t*)(f),(Ddi_Generic_t*)(g),NULL)
#define DdiGenericOp3(op,acc,f,g,h) \
  DdiGenericOp(op,acc,(Ddi_Generic_t*)(f),(Ddi_Generic_t*)(g),\
  (Ddi_Generic_t*)(h))

/*
 *  Functions to be implemented
 */

#define DdiGenericPrint(f,fp) \
  fprintf(stderr,"DdiGenericPrint still to implement\n")
#define DdiGenericPrintStats(f,fp) \
  fprintf(stderr,"DdiGenericPrintStats still to implement\n")

#if 0
#define Part_BddMultiwayLinearAndExist(f,g,a,b,c) \
  (fprintf(stderr,"MultiwayAndExist still to implement\n"),NULL)

#define DdiArrayStore(f,g,a,b,c,x,y,z) \
  (fprintf(stderr,"DdiArrayStore still to implement\n"),0)
#endif


#define DdiMetaCopy(m,f) \
  (fprintf(stderr,"DdiMetaCopy still to implement\n"),NULL)

/* #define DdiMapMinisatAlloc((obj)) {
  Pdtutil_Assert((obj)->);
  } */

/*
 * Ddi_Map_t generic macros
 */

#define DdiMapAlloc(map, init_size) {                       \
  (map) = Pdtutil_Alloc(struct Ddi_Map_t, 1);               \
  (map)->type = Ddi_MapType_None_c;                         \
  (map)->map_obj = Ddi_MapObj_None_c;                       \
  (map)->a_mgr = NULL;                                      \
  (map)->b_mgr = NULL;                                      \
  (map)->size = init_size;                                  \
  (map)->num = 0;                                           \
}

#define DdiMapSetType(obj, init_type) {                                                   \
  Pdtutil_Assert((map)->type == Ddi_MapType_None_c, "Map object is already initialized"); \
  (map)->type = init_type;                                                                \
}

#define DdiMapInitArray(map) {                                                                   \
  Pdtutil_Assert((map)->map_obj == Ddi_MapObj_None_c, "Map object type is already initialized"); \
  (map)->map_obj = Ddi_MapObj_Array_c;                                                           \
  (map)->a_data.array = Pdtutil_IntegerArrayAlloc((map)->size);                                  \
  for (int i=0; i<(map)->size; i++) {                                                            \
    Pdtutil_IntegerArrayWrite((map)->a_data.array, i, -1);                                       \
  }                                                                                              \
  (map)->b_data.array = Pdtutil_IntegerArrayAlloc((map)->size);                                  \
  for (int i=0; i<(map)->size; i++) {                                                            \
    Pdtutil_IntegerArrayWrite((map)->b_data.array, i, -1);                                       \
  }                                                                                              \
}

#define DdiMapInitSymTab(map) {                                                                   \
  Pdtutil_Assert((map)->map_obj == Ddi_MapType_None_c, "Map object type is already initialized"); \
  (map)->map_obj = Ddi_MapObj_SymTab_c;                                                           \
  (map)->a_data.symtab = st_init_table(st_ptrcmp, st_ptrhash);                                    \
  (map)->b_data.symtab = st_init_table(st_ptrcmp, st_ptrhash);                                    \
}

#define DdiMapGetSize(map) ((map)->size)
#define DdiMapGetNum(map)  ((map)->num)
#define DdiMapGetType(map) ()
#define DdiMapIsArray(map) ((map)->map_obj)

#define DdiMapSetArray(obj, what, idx, val) {                                             \
  Pdtutil_Assert(idx < (map)->size, "Map is out of bound");                               \
  Pdtutil_Assert(Pdtutil_IntegerArrayRead((map)->what, idx) == -1, "Map already exists"); \
  Pdtutil_IntegerArrayWrite((map)->what, idx, val);                                       \
  Pdtutil_IntegerArrayIncNum((map)->what);                                                \
}

#define DdiMapFree(map) {                             \
  if ((map)->map_obj == Ddi_MapObj_Array_c) {         \
    Pdtutil_IntegerArrayFree((map)->a_data.array);    \
    Pdtutil_IntegerArrayFree((map)->b_data.array);    \
  } else if ((map)->map_obj == Ddi_MapObj_SymTab_c) { \
    st_free_table((map)->b_data.symtab);              \
    st_free_table((map)->b_data.symtab);              \
  }                                                   \
  Pdtutil_Free(map);                                  \
}

#define DdiSignaturearrayNum(sa) ((sa)->num)
#define DdiSignaturearraySize(sa) ((sa)->size)

#define DdiVarIsAfterVar(v1,v0) \
(\
 (((v1)->common.code==Ddi_Var_Aig_c) && ((v0)->common.code==Ddi_Var_Bdd_c)) ||\
 (((v1)->common.code==(v0)->common.code) && Ddi_VarIndex(v1)>Ddi_VarIndex(v0))\
)
#define DdiVarIsAfterVarInOrd(v1,v0) \
(\
 (((v1)->common.code==Ddi_Var_Aig_c) && ((v0)->common.code==Ddi_Var_Bdd_c)) || \
 (((v1)->common.code==(v0)->common.code) && \
  ((v1)->common.code==Ddi_Var_Aig_c && Ddi_VarIndex(v1)>Ddi_VarIndex(v0) || \
   (v1)->common.code==Ddi_Var_Bdd_c && Ddi_VarCurrPos(v1)>Ddi_VarCurrPos(v0)\
  )\
 )\
)

#define LglClause1(L,c) {\
  lgladd (L, c); \
  lgladd (L, 0); \
}

#define LglClause2(L,c1,c2) {    \
  lgladd (L, c1); \
  lgladd (L, c2); \
  lgladd (L, 0); \
}

#define LglClause3(L,c1,c2,c3) {			\
  lgladd (L, c1);					\
  lgladd (L, c2); \
  lgladd (L, c3); \
  lgladd (L, 0); \
}

#if 0
#define aig2CnfId(bmgr,aigId) (bAigNodeID(aigId))
#else
#define aig2CnfId(bmgr,aigId) (aig2CnfIdIntern(bmgr,aigId))
#endif

#define aigIsInv(f) bAig_NodeIsInverted(Ddi_BddToBaig(f))

#define aigCnfLit(s, f) \
(bAig_CnfId(Ddi_ReadMgr(f)->aig.mgr, Ddi_BddToBaig(f)) > 0 ? \
    (bAig_NodeIsInverted(Ddi_BddToBaig(f)) ? \
        -bAig_CnfId(Ddi_ReadMgr(f)->aig.mgr, Ddi_BddToBaig(f)) : \
         bAig_CnfId(Ddi_ReadMgr(f)->aig.mgr, Ddi_BddToBaig(f))) : \
    MinisatClauses(s, f, NULL, NULL, 1))

#define aig2CnfIdSigned(bmgr,aigId) \
  (bAig_NodeIsInverted((aigId))?-aig2CnfId(bmgr,aigId):aig2CnfId(bmgr,aigId))

inline Lit DdiLit2Minisat(Ddi_SatSolver_t *s, int l) { 
  int ls = s->map[abs(l)];
  Pdtutil_Assert(ls>0,"not mapped literal");
  return ((l)<0 ? ~Lit(abs(ls)) : Lit(ls)); 
}

#define MinisatLit(l) \
  ((l)<0 ? ~Lit(abs(l)-1) : Lit((l)-1))

#define MinisatClause1(S,l,c) {\
  while (abs(c)>(S).nVars()) (S).newVar();\
  (l).clear();\
  (l).push(MinisatLit(c));\
  (S).addClause(l);\
}

#define MinisatClause2(S,l,c0,c1) {\
  while (abs(c0)>(S).nVars()||abs(c1)>(S).nVars()) (S).newVar();\
  (l).clear();\
  (l).push(MinisatLit(c0));\
  if ((c1)!=(c0))(l).push(MinisatLit(c1));\
  (S).addClause(l);\
}

#define MinisatClause3(S,l,c0,c1,c2) {\
  while (abs(c0)>(S).nVars()||abs(c1)>(S).nVars()||abs(c2)>(S).nVars())\
    (S).newVar();\
  (l).clear();\
  (l).push(MinisatLit(c0));\
  if ((c1)!=(c0))(l).push(MinisatLit(c1));\
  if ((c2)!=(c0)&&(c2)!=(c1))(l).push(MinisatLit(c2));\
  (S).addClause(l);\
}

#define MinisatClause4(S,l,c0,c1,c2,c3) {\
  while (abs(c0)>(S).nVars()||abs(c1)>(S).nVars()||abs(c2)>(S).nVars()\
    ||abs(c3)>(S).nVars()) (S).newVar();\
  (l).clear();\
  (l).push(MinisatLit(c0));\
  if ((c1)!=(c0))(l).push(MinisatLit(c1));\
  if ((c2)!=(c0)&&(c2)!=(c1))(l).push(MinisatLit(c2));\
  if ((c3)!=(c0)&&(c3)!=(c1)&&(c3)!=(c2))(l).push(MinisatLit(c3));\
  (S).addClause(l);\
}

EXTERN int DdiSolverReadModel22(Ddi_SatSolver_t *solver, int var);

inline lbool DdiSolverReadModel(Ddi_SatSolver_t *solver, int var) {
  if (var>=solver->nVarsMax) return l_Undef;
  int vs = solver->map[var];
  if (vs<0) return l_Undef; 
  if ((solver)->S==NULL) {
    int res = DdiSolverReadModel22(solver, var);
    if (res == -1) return l_False;
    if (res == 1) return l_True;
    return l_Undef;
  }
  return (solver)->S->model[vs];
}

#define DdiSolverCheckVar(solver,lit) \
{\
  while (abs(lit) >= (solver)->nVarsMax-1) {	\
    int i;\
    (solver)->nVarsMax *= 2;\
    (solver)->vars = Pdtutil_Realloc(int,(solver)->vars,(solver)->nVarsMax);\
    (solver)->freeVars = \
	Pdtutil_Realloc(int,(solver)->freeVars,(solver)->nVarsMax);\
    (solver)->map = Pdtutil_Realloc(int,(solver)->map,(solver)->nVarsMax);\
    (solver)->mapInv = \
       Pdtutil_Realloc(int,(solver)->mapInv,(solver)->nVarsMax);\
    for (i=(solver)->nVarsMax/2; i<(solver)->nVarsMax; i++) {\
      (solver)->map[i] = (solver)->mapInv[i] = -1;\
    }\
  }\
  if (abs(lit) >= (solver)->nVars) {		\
    /* printf("incrVars\n");	 */		\
    (solver)->nVars=abs(lit)+1;			\
  }\
  /* printf("chk: %d (v: %d, vm: %d)\n",lit, (solver)->nVars, (solver)->nVarsMax); */  \
  if ((solver)->map[abs(lit)]<0) {\
    int v = ((solver)->S)->nVars();		\
    ((solver)->S)->newVar();		      \
    Pdtutil_Assert(((solver)->S)->nVars()<=(solver)->nVarsMax,	\
      "max num of solver vars exceeded");	\
    /* printf("mapping: %d -> %d\n", abs(lit), v); */	\
    (solver)->map[abs(lit)] = v;\
    (solver)->mapInv[v] = abs(lit);\
  }\
}

inline void DdiSolverSetVar(Ddi_SatSolver_t *solver, int lit) {
  DdiSolverCheckVar(solver,lit);
  (solver)->vars[abs(lit)] = 1; /* non free */
}

#define DdiSolverFreeVar(solver,lit) \
{\
  Pdtutil_Assert(abs(lit) < (solver)->nVarsMax, "free wrong solver var");	\
  Pdtutil_Assert((solver)->vars[abs(lit)]>0, "free already free solver var"); \
  (solver)->vars[abs(lit)] = -1; /* free */ \
  (solver)->freeVars[(solver)->nFreeVars++] = abs(lit);	\
  (solver)->nFreeTot--;					\
 }

#define DdiClauseArrayAddClause1(ca,l) {\
  unsigned int mask = (1 << (abs(l)%31));\
  Ddi_Clause_t *cl = Ddi_ClauseAlloc(1,1);	\
  (cl)->lits[0] = l;\
  cl->signature0 = mask;\
  if ((l)<0) cl->signature1 = mask;\
  Ddi_ClauseArrayWriteLast(ca,cl);\
}
#define DdiClauseArrayAddClause2(ca,l0,l1) {\
  unsigned int mask = (1 << (abs(l0)%31));\
  Ddi_Clause_t *cl = Ddi_ClauseAlloc(2,2); \
  (cl)->lits[0] = l0;\
  (cl)->lits[1] = l1;\
  cl->signature0 = mask;\
  if ((l0)<0) cl->signature1 = mask;\
  mask = (1 << (abs(l1)%31));\
  cl->signature0 |= mask;\
  if ((l1)<0) cl->signature1 |= mask;\
  if (abs(l0)>abs(l1)) {\
    cl->sorted = 0;\
    Ddi_ClauseSort(cl);\
  }\
  Ddi_ClauseArrayWriteLast(ca,cl);\
}
#define DdiClauseArrayAddClause3(ca,l0,l1,l2) {\
  unsigned int mask = (1 << (abs(l0)%31));\
  Ddi_Clause_t *cl = Ddi_ClauseAlloc(3,3); \
  (cl)->lits[0] = l0;\
  (cl)->lits[1] = l1;\
  (cl)->lits[2] = l2;\
  cl->signature0 = mask;\
  if ((l0)<0) cl->signature1 = mask;\
  mask = (1 << (abs(l1)%31));\
  cl->signature0 |= mask;\
  if ((l1)<0) cl->signature1 |= mask;\
  mask = (1 << (abs(l2)%31));\
  cl->signature0 |= mask;\
  if ((l2)<0) cl->signature1 |= mask;\
  if (abs(l0)>abs(l1)||abs(l1)>abs(l2)) {\
    cl->sorted = 0;\
    Ddi_ClauseSort(cl);\
  }\
  Ddi_ClauseArrayWriteLast(ca,cl);\
}

#define DdiSatClause2Minisat(solver,minisatClause,cl)\
{\
  int j;\
  minisatClause.clear();\
  for (j=0; j<(cl)->nLits; j++) {\
    int lit = (cl)->lits[j];\
    if (lit==0) continue; \
    /*   printf("lit: %d\n",lit); */		\
    DdiSolverSetVar(solver,lit);\
    minisatClause.push(DdiLit2Minisat(solver,lit));	\
  }\
}

#define DdiPdrLit2Var(lit) (abs(lit)) 
#define DdiPdrLitIsCompl(lit) (lit<0) 

#define DdiVarIsPi(nState,nInput,var)		\
  (((var)>(2*(nState)))&&			\
   ((var)-(2*(nState))<=nInput))
#define DdiVarIsPs(nState,var) \
  (((var)<=(2*(nState)))&&((var)%2))
#define DdiVarIsNs(nState,var) \
  (((var)<=(2*(nState)))&&(!((var)%2)))

#define DdiVarPiIndex(nState,nInput,var)			\
  (DdiVarIsPi(nState,nInput,var)?((var)-(2*(nState))-1):-1)
#define DdiVarPsIndex(nState,var) \
  (DdiVarIsPs(nState,var)?((var)/2):-1)
#define DdiVarNsIndex(pdrMgr,var) \
  (DdiVarIsNs(nState,var)?((var)/2-1):-1)

#define DdiPdrVarIsPi(pdrMgr,var) \
  (((var)>(2*(pdrMgr->nState)))&&			\
   ((var)-(2*(pdrMgr->nState))<=pdrMgr->nInput))
#define DdiPdrVarIsPs(pdrMgr,var) \
  (((var)<=(2*(pdrMgr->nState)))&&((var)%2))
#define DdiPdrVarIsNs(pdrMgr,var) \
  (((var)<=(2*(pdrMgr->nState)))&&(!((var)%2)))

#define DdiPdrVarPiIndex(pdrMgr,var) \
  (DdiPdrVarIsPi(pdrMgr,var)?((var)-(2*(pdrMgr->nState))-1):-1)
#define DdiPdrVarPsIndex(pdrMgr,var) \
  (DdiPdrVarIsPs(pdrMgr,var)?((var)/2):-1)
#define DdiPdrVarNsIndex(pdrMgr,var) \
  (DdiPdrVarIsNs(pdrMgr,var)?((var)/2-1):-1)

#define DdiPdrPiIndex2Var(pdrMgr,i) \
  (((i<0)||(i>=pdrMgr->nInput))?0:((2*pdrMgr->nState)+1+(i)))
#define DdiPdrPsIndex2Var(pdrMgr,i) \
  (((i<0)||(i>=pdrMgr->nState))?0:((i)*2+1))
#define DdiPdrNsIndex2Var(pdrMgr,i) \
  (((i<0)||(i>=pdrMgr->nState))?0:((i+1)*2))


#define IsRightEnabled(bmgr, baig) \
  ((nodeAuxChar(bmgr,baig) & Ddi_SccNodeStatus_RightDisabled_c) == 0)
#define IsLeftEnabled(bmgr, baig) \
  ((nodeAuxChar(bmgr,baig) & Ddi_SccNodeStatus_LeftDisabled_c) == 0)
#define IsNodeEnabled(bmgr, baig) \
  ((nodeAuxChar(bmgr,baig) & Ddi_SccNodeStatus_NodeDisabled_c) == 0)
#define IsInStack(bmgr, baig) \
  ((nodeAuxChar(bmgr,baig) & Ddi_SccNodeStatus_InStack_c) == 1)

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EXTERN int aig2CnfNewIdIntern(Ddi_Mgr_t *ddm);
EXTERN int MinisatClauses(Solver& S, Ddi_Bdd_t *f, Ddi_Bdd_t *g, int *na, int genRelation);
EXTERN int aig2CnfIdIntern(bAig_Manager_t *manager, bAigEdge_t nodeIndex);
//EXTERN int aig2CnfIdRead(bAig_Manager_t *manager, bAigEdge_t nodeIndex);
EXTERN int DdiVarCompare(Ddi_Var_t *v0, Ddi_Var_t *v1);
EXTERN int ddiAbcOptAcc(Ddi_Bdd_t *aig, float timeLimit);
EXTERN int ddiAbcOptWithLevelAcc(Ddi_Bdd_t *aig, int optLevel, float timeLimit);
EXTERN void DdiAigFree(Ddi_Aig_t *a);
EXTERN Ddi_Aig_t * DdiAigDup(Ddi_Aig_t *a);
EXTERN Ddi_Aig_t *DdiAigCopy (Ddi_Mgr_t *ddm0, Ddi_Mgr_t *ddm, Ddi_Aig_t *a);
EXTERN Ddi_ArrayData_t *DdiAigArrayCopy (Ddi_Mgr_t *ddm0, Ddi_Mgr_t *ddm, Ddi_ArrayData_t *a);
EXTERN bAigEdge_t DdiAigToBaig(Ddi_Aig_t *a);
EXTERN Ddi_Aig_t * DdiAigMakeFromBaig(bAig_Manager_t *mgr, bAigEdge_t bAigNode);
EXTERN Ddi_Varset_t * DdiAigSupp(Ddi_Bdd_t *fAig);
EXTERN Ddi_Varset_t *DdiAigArraySupp (Ddi_Bddarray_t *fAigA);
EXTERN int DdiAigFanoutCount(Ddi_Bdd_t *fAig, Ddi_Var_t *v);
EXTERN int DdiAigFlowCount(Ddi_Bdd_t *fAig, Ddi_Var_t *v);
EXTERN int DdiAigVararraySortByFlow(Ddi_Vararray_t *vA, Ddi_Bdd_t *fAig);
EXTERN int DdiAigVararraySortByFanout(Ddi_Vararray_t *vA, Ddi_Bdd_t *fAig);
EXTERN Ddi_Aig_t * DdiAigNot(Ddi_Aig_t *a);
EXTERN Ddi_Aig_t * DdiAigAnd(Ddi_Aig_t *a, Ddi_Aig_t *b);
EXTERN Ddi_Aig_t * DdiAigOr(Ddi_Aig_t *a, Ddi_Aig_t *b);
EXTERN Ddi_Aig_t * DdiAigXor(Ddi_Aig_t *a, Ddi_Aig_t *b);
EXTERN Ddi_Bdd_t * DdiAigCofactorAcc(Ddi_Bdd_t *fAig, Ddi_Var_t *v, int val);
EXTERN Ddi_Bddarray_t * DdiAigArrayCofactorAcc(Ddi_Bddarray_t *fA, Ddi_Var_t *v, int val);
EXTERN Ddi_Bdd_t * DdiAigComposeAcc(Ddi_Bdd_t *fAig, Ddi_Vararray_t *vA, Ddi_Bddarray_t *gA);
EXTERN int DdiAigArraySize(Ddi_Bddarray_t *aigArray);
EXTERN int DdiAigCnfLit(Solver& S, Ddi_Bdd_t *node);
EXTERN Ddi_Bdd_t * DdiAigExistVarAcc(Ddi_Bdd_t *fAig, Ddi_Var_t *v, Ddi_Var_t *refv, Ddi_Bdd_t *care, int nosat, Ddi_AigSignatureArray_t *varSigs, Ddi_AigSignature_t *careSig, int partial, float timeLimit);
EXTERN Ddi_Bdd_t * DdiAigExistEqVarAcc(Ddi_Bdd_t *fAig, Ddi_Var_t *v, Ddi_Bdd_t *care, Ddi_AigSignatureArray_t *varSigs, Ddi_AigSignature_t *careSig);
EXTERN void DdiAigArrayExistEqVarAcc(Ddi_Bddarray_t *fAigArray, Ddi_Var_t *v, Ddi_Bdd_t *care, Ddi_AigSignatureArray_t *varSigs, Ddi_AigSignature_t *careSig);
EXTERN Ddi_Bdd_t * DdiAigExistOverPartialAcc(Ddi_Bdd_t *fAig, Ddi_Varset_t *smooth, Ddi_Bdd_t *care, int minSize);
EXTERN Ddi_Bdd_t * DdiAigTernaryInterpolantByGroupsAcc(Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Varset_t *abstr, Ddi_Varset_t *smooth, Ddi_Varset_t *ternarySmooth, Ddi_Bdd_t *care, int maxVars, int phase, float timeLimit);
EXTERN Ddi_Bdd_t *DdiAigMonotoneRedAcc (Ddi_Bdd_t *a, Ddi_Bdd_t *care, Ddi_Varset_t *project, float timeLimit);
EXTERN Ddi_Bdd_t * DdiAigTernaryInterpolantAcc(Ddi_Bdd_t *a, Ddi_Bdd_t *b, Ddi_Varset_t *abstr, Ddi_Varset_t *smooth, Ddi_Varset_t *ternarySmooth, Ddi_Bdd_t *care, int chkProd, int phase, float timeLimit);
EXTERN Ddi_Bdd_t *DdiAigExistNnfAcc (Ddi_Bdd_t *a,Ddi_Varset_t *smooth,Ddi_Varset_t *project);
EXTERN Ddi_Bddarray_t *DdiAigarrayExistNnfAcc (Ddi_Bddarray_t *fA,Ddi_Varset_t *smooth,Ddi_Varset_t *project, Ddi_Vararray_t *ps, Ddi_Vararray_t *ns);
EXTERN Ddi_Bdd_t * DdiAigExistMonotoneAcc(Ddi_Bdd_t *a, Ddi_Varset_t *smooth, Ddi_Varset_t *project, Ddi_Bdd_t *care, int genMonotoneCube);
EXTERN Ddi_Bdd_t *DdiAigFuncDepAcc (Ddi_Bdd_t *a, Ddi_Bdd_t *care, Ddi_Varset_t *refVars, Ddi_Varset_t *filterVars, Ddi_Bdd_t *auxEqMerge, Ddi_Bdd_t *eqConstr, Ddi_Vararray_t *eqVars, Ddi_Bddarray_t *substLits, int *nEqTotP);
EXTERN Ddi_Bdd_t * DdiAigEquivVarsSimpleAcc(Ddi_Bdd_t *a, Ddi_Bdd_t *care, Ddi_Varset_t *refVars, Ddi_Varset_t *filterVars, Ddi_Bdd_t *auxEqMerge, Ddi_Vararray_t *eqVars, Ddi_Bddarray_t *substLits, int *nEqTotP);
EXTERN Ddi_Bdd_t * DdiAigEquivVarsAcc0(Ddi_Bdd_t *a, Ddi_Bdd_t *care, Ddi_Varset_t *refVars, Ddi_Varset_t *filterVars, Ddi_Bdd_t *auxEqMerge, Ddi_Vararray_t *eqVars, Ddi_Bddarray_t *substLits);
EXTERN Ddi_Bdd_t * DdiAigEquivVarsAcc1(Ddi_Bdd_t *a, Ddi_Bdd_t *care, Ddi_Varset_t *refVars, Ddi_Varset_t *filterVars, Ddi_Bdd_t *auxEqMerge, Ddi_Vararray_t *eqVars, Ddi_Bddarray_t *substLits);
EXTERN Ddi_Bdd_t * DdiAigEquivVarsAcc2(Ddi_Bdd_t *a, Ddi_Bdd_t *care, Ddi_Varset_t *refVars, Ddi_Varset_t *filterVars, Ddi_Bdd_t *auxEqMerge, Ddi_Vararray_t *eqVars, Ddi_Bddarray_t *substLits);
EXTERN Ddi_Bdd_t * DdiAigEquivVarsAcc(Ddi_Bdd_t *a, Ddi_Bdd_t *care, Ddi_Varset_t *refVars, Ddi_Varset_t *filterVars, Ddi_Bdd_t *auxEqMerge, Ddi_Vararray_t *eqVars, Ddi_Bddarray_t *substLits);
EXTERN Ddi_Bdd_t * DdiAigEquivVarsEqGivenAcc(Ddi_Bdd_t *a, Ddi_Bdd_t *care, Ddi_Varset_t *refVars, Ddi_Varset_t *filterVars, Ddi_Bdd_t *auxEqMerge, Ddi_Vararray_t *eqVarsGiven, Ddi_Bddarray_t *substLitsGiven, Ddi_Vararray_t *eqVars, Ddi_Bddarray_t *substLits);
EXTERN Ddi_Bdd_t *DdiAigEquivVarsInConstrain (Ddi_Bdd_t *constr, Ddi_Vararray_t *filterVars, Ddi_Bdd_t *auxEqMerge, Ddi_Vararray_t *eqVars, Ddi_Bddarray_t *substFuncs);
EXTERN Ddi_Bdd_t * DdiAigImplicationsAcc(Ddi_Bdd_t *a, Ddi_Bdd_t *care, Ddi_Varset_t *refVars, Ddi_Varset_t *filterVars, Ddi_Bddarray_t *implArray);
EXTERN Ddi_Bdd_t * DdiAigImpliedVarsAcc(Ddi_Bdd_t *a, Ddi_Bdd_t *care, Ddi_Varset_t *filterVars);
EXTERN Ddi_Bdd_t * DdiAigTernaryReductionAcc(Ddi_Bdd_t *a, Ddi_Varset_t *smooth, Ddi_Varset_t *project, Ddi_Bdd_t *care);
EXTERN Ddi_Bdd_t * DdiAigMonotoneReductionAcc(Ddi_Bdd_t *a, Ddi_Varset_t *smooth, Ddi_Varset_t *project, Ddi_Bdd_t *care);
EXTERN Ddi_Bdd_t * DdiAigExistOverAcc(Ddi_Bdd_t *fAig, Ddi_Varset_t *smooth, Ddi_Bdd_t *care);
EXTERN Ddi_Bdd_t * DdiAigExistSkolemAcc(Ddi_Bdd_t *fAig, Ddi_Varset_t *smooth, Ddi_Bdd_t *care);
EXTERN Ddi_Bdd_t * DdiAigExistItpAcc(Ddi_Bdd_t *fAig, Ddi_Varset_t *smooth, Ddi_Bdd_t *care, int maxIter);
EXTERN Ddi_Bdd_t * DdiAigExistVarOverAcc(Ddi_Bdd_t *fAig, Ddi_Var_t *smoothVar, Ddi_Bdd_t *care);
EXTERN Ddi_Bddarray_t * DdiAigArrayRedRemovalControlAcc(Ddi_Bddarray_t *fAigarray, Ddi_Bdd_t *care, int maxObserve, float timeLimit);
EXTERN Ddi_Bdd_t * DdiAigRedRemovalControlAcc(Ddi_Bdd_t *fAig, Ddi_Bdd_t *care, int maxObserve, float timeLimit);
EXTERN Ddi_Bddarray_t * DdiAigArrayRedRemovalAcc(Ddi_Bddarray_t *fAigarray, Ddi_Bdd_t *care, int maxObserve, float timeLimit);
EXTERN Ddi_Bdd_t * DdiAigRedRemovalNnfAcc (Ddi_Bdd_t *fAig, Ddi_Bdd_t *care, int maxObserve, float timeLimit, int doNnfEncoding);
EXTERN Ddi_Bdd_t * DdiAigRedRemovalAcc(Ddi_Bdd_t *fAig, Ddi_Bdd_t *care, int maxObserve, float timeLimit);
EXTERN Ddi_Bdd_t *DdiAigRedRemovalOdcAcc (Ddi_Bdd_t *fAig, Ddi_Bdd_t *care, int maxObserve, float timeLimit);
EXTERN int DdiAigMgrCheck(Ddi_Bddarray_t *aigRoots);
EXTERN int DdiAigMgrGarbageCollect(Ddi_Bddarray_t *aigRoots);
EXTERN Ddi_Bdd_t * DdiAigExistAccWithPart(Ddi_Bdd_t *fAig, Ddi_Varset_t *smooth, Ddi_Bdd_t *care, Ddi_Bdd_t *pivotCube, int partial, int nosat, int enPart, int enSubset, float timeLimit);
EXTERN Ddi_Bdd_t * DdiAigExistEqIncrementalAcc(Ddi_Bdd_t *fAig, Ddi_Varset_t *smooth, Ddi_Bdd_t *care, float timeLimit, Ddi_Bdd_t *fAigCore);
EXTERN int DdiAigIsConst(Ddi_Bdd_t *fAig, int phase);
EXTERN int DdiAigIsConstSat(Ddi_Bdd_t *fAig, int phase);
EXTERN Ddi_AigSignatureArray_t * DdiAigSignatureArrayAlloc(int num);
EXTERN Ddi_AigSignatureArray_t * DdiAigSignatureArrayDup(Ddi_AigSignatureArray_t *sA);
EXTERN void DdiAigSignatureArrayClear(Ddi_AigSignatureArray_t *sA);
EXTERN void DdiAigSignatureArrayFree(Ddi_AigSignatureArray_t *sA);
//@US
EXTERN Ddi_AigDynSignatureArray_t * DdiAigDynSignatureArrayAlloc(int num, unsigned int size);
EXTERN void DdiAigDynSignatureArrayFree(Ddi_AigDynSignatureArray_t *sA);
EXTERN Ddi_AigDynSignature_t * DdiAigDynSignatureAlloc(unsigned int size);
EXTERN void DdiAigDynSignatureFree(Ddi_AigDynSignature_t *s);
EXTERN int DdiAigDynSignatureArrayNum(Ddi_AigDynSignatureArray_t *sA);
EXTERN Ddi_AigDynSignature_t *DdiAigDynSignatureArrayRead(Ddi_AigDynSignatureArray_t *sA, int i);
EXTERN int DdiAigDynSignatureAllocCheck(Ddi_AigDynSignatureArray_t *sA, int i, Ddi_AigDynSignatureArray_t *freeSigs);
EXTERN void DdiAigDynSignatureMoveToFreeList(Ddi_AigDynSignatureArray_t *sA, int i, Ddi_AigDynSignatureArray_t *freeSigs);
EXTERN unsigned int DdiAigDynSignatureSize(Ddi_AigDynSignature_t* s);
EXTERN void DdiAigDynSignatureClear(Ddi_AigDynSignature_t *s);
EXTERN void DdiAigDynSignatureSetBit(Ddi_AigDynSignature_t *s, unsigned int i);
EXTERN int DdiAigDynSignatureTestBit(Ddi_AigDynSignature_t *s, unsigned int i);
EXTERN int DdiAigDynSignatureUnion(Ddi_AigDynSignature_t *result, Ddi_AigDynSignature_t *s1, Ddi_AigDynSignature_t *s2, int keepOldValue);
EXTERN int DdiAigDynSignatureIntersection(Ddi_AigDynSignature_t *result, Ddi_AigDynSignature_t *s1, Ddi_AigDynSignature_t *s2, int keepOldValue);
EXTERN unsigned int DdiAigDynSignaturePatternSize(unsigned int size);
EXTERN unsigned int DdiAigDynSignatureIndexToSlot(unsigned int index);
EXTERN int DdiAigDynSignatureCountBits(Ddi_AigDynSignature_t *sig, int val);
EXTERN void DdiAigDynSignatureCopy(Ddi_AigDynSignature_t *dest, Ddi_AigDynSignature_t *src);
EXTERN void DdiAigDynSignatureArrayInit(Ddi_AigDynSignatureArray_t *sA); 
EXTERN Ddi_Varset_t * DdiDynSignatureToVarset(Ddi_AigDynSignature_t *varSig, Ddi_Vararray_t *vars);
EXTERN Ddi_Varset_t *DdiDynSignatureToVarsetWithScc(Ddi_SccMgr_t *sccMgr, Ddi_AigDynSignature_t *varSig, Ddi_Vararray_t *vars);
EXTERN void DdiCoisStatistics (Ddi_Varsetarray_t *cois, float **cM, Ddi_CoiStats_e type);
EXTERN void DdiDeltaCoisVarsRank (Ddi_Varsetarray_t *cois, int *varsRank, Ddi_Vararray_t *ps);
EXTERN void DdiLambdaCoisVarsRank (Ddi_Varsetarray_t *cois, int *varsRank, Ddi_Vararray_t *pi);
//@US
EXTERN Ddi_Signature_t * DdiSignatureAlloc(Ddi_Vararray_t *vars);
EXTERN Ddi_Vararray_t * DdiSignatureVars(Ddi_Signature_t *s);
EXTERN Ddi_Signature_t * DdiSignatureDup(Ddi_Signature_t *s);
EXTERN int DdiSignatureSize(Ddi_Signature_t *s);
EXTERN int DdiSignaturePatternSize(Ddi_Signature_t *s);
EXTERN int DdiSignatureRandom(Ddi_Signature_t *s, Solver *S);
EXTERN void DdiSignatureInitFromCube(Ddi_Signature_t *s, Ddi_Bdd_t *f);
EXTERN void DdiSignatureFree(Ddi_Signature_t *s);
EXTERN unsigned int DdiSignatureRead(Ddi_Signature_t *s, int idx);
EXTERN void DdiSignaturePrint(Ddi_Signature_t *s, FILE *fp, int nCols);
EXTERN Ddi_Signature_t * DdiSignaturearrayRemove(Ddi_Signaturearray_t *sa, int idx);
EXTERN Ddi_Signaturearray_t * DdiSignaturearrayAlloc(int size);
EXTERN Ddi_Signature_t * DdiSignaturearrayRead(Ddi_Signaturearray_t *sa, int idx);
EXTERN void DdiSignaturearrayInsert(Ddi_Signaturearray_t *sa, int idx, Ddi_Signature_t *s);
EXTERN void DdiSignaturearrayInsertLast(Ddi_Signaturearray_t *sa, Ddi_Signature_t *s);
EXTERN void DdiSignaturearrayFree(Ddi_Signaturearray_t *sa);
EXTERN void DdiSignaturearrayPrint(Ddi_Signaturearray_t *sa, FILE *fp, int nCols);
EXTERN void DdiSignatureComposeBddXorAcc(Ddi_Bdd_t *bddXor, Ddi_Varset_t *care_vs, Ddi_Signature_t *s);
EXTERN void DdiSignatureComposeBddAndAcc(Ddi_Bdd_t *bddAnd, Ddi_Signature_t *s);
EXTERN long double DdiAigSigEstimateCoverage(Ddi_Bdd_t *f, Ddi_AigSignatureArray_t *varSigs, int nBits);
EXTERN int DdiAigOptByBddRelation(Ddi_Bdd_t *f, int th, int nFactors, Ddi_Varset_t *projectVars, float timeLimit);
EXTERN Ddi_Bdd_t * DdiAigExistProjectByBdd(Ddi_Bdd_t *f, Ddi_Bdd_t *care, int th, Ddi_Varset_t *projectVars, float timeLimit, int auxVarIdStart, int overAppr);
EXTERN int DdiAigConstrainSignatures(Ddi_Bdd_t *fAig, Ddi_Bdd_t *careAig, Ddi_AigSignatureArray_t *varSigs, int min_pattern, float *pDensity, int timeLimit);
EXTERN void DdiAigCheckRedFull(Ddi_Bdd_t *fAig, Ddi_Bdd_t *care);
EXTERN Ddi_ArrayData_t * DdiArrayAlloc(int length);
EXTERN void DdiArrayFree(Ddi_ArrayData_t *array);
EXTERN DdNode ** DdiArrayToCU(Ddi_ArrayData_t *array);
EXTERN void DdiArrayWrite(Ddi_ArrayData_t *array, int i, Ddi_Generic_t *f, Ddi_CopyType_e copy);
EXTERN void DdiArrayInsert(Ddi_ArrayData_t *array, int i, Ddi_Generic_t *f, Ddi_CopyType_e copy);
EXTERN Ddi_Generic_t * DdiArrayRead(Ddi_ArrayData_t *array, int i);
EXTERN Ddi_Generic_t * DdiArrayExtract(Ddi_ArrayData_t *array, int i);
EXTERN int DdiArrayNum(Ddi_ArrayData_t *array);
EXTERN Ddi_ArrayData_t * DdiArrayDup(Ddi_ArrayData_t *old);
EXTERN void DdiArrayRemoveNull(Ddi_ArrayData_t *array);
EXTERN Ddi_ArrayData_t * DdiArrayCopy(Ddi_Mgr_t *dd2, Ddi_ArrayData_t *old);
EXTERN void DdiArrayAppend(Ddi_ArrayData_t *array1, Ddi_ArrayData_t *array2);
EXTERN int DdiArrayStore(Ddi_ArrayData_t *array, char *ddname, char **vnames, char **rnames, int *vauxids, int mode, char *fname, FILE *fp);
EXTERN int DdiArrayStoreCnf(Ddi_ArrayData_t *array, Dddmp_DecompCnfStoreType mode, int noHeader, char **vnames, int *vbddids, int *vauxids, int *vcnfids, int idInitial, int edgeInMaxTh, int pathLengthMaxTh, char *fname, FILE *fp, int *clauseNPtr, int *varNewNPtr);
EXTERN int DdiArrayStoreBlif(Ddi_ArrayData_t *array, char **inputNames, char **outputNames, char *modelName, char *fileName, FILE *fp);
EXTERN int DdiArrayStorePrefix(Ddi_ArrayData_t *array, char **inputNames, char **outputNames, char *modelName, char *fileName, FILE *fp);
EXTERN int DdiArrayStoreSmv(Ddi_ArrayData_t *array, char **inputNames, char **outputNames, char *modelName, char *fileName, FILE *fp);
EXTERN Ddi_Generic_t * DdiGenericAlloc(Ddi_Type_e type, Ddi_Mgr_t *mgr);
EXTERN void DdiTraceNodeAlloc(Ddi_Generic_t *r);
EXTERN void DdiGenericFree(Ddi_Generic_t *f);
EXTERN Ddi_Generic_t * DdiDeferredFree(Ddi_Generic_t *f);
EXTERN Ddi_Generic_t * DdiGenericDup(Ddi_Generic_t *f);
EXTERN Ddi_Generic_t * DdiGenericCopy(Ddi_Mgr_t *ddm, Ddi_Generic_t *f, Ddi_Vararray_t *varsOld, Ddi_Vararray_t *varsNew);
EXTERN void DdiGenericDataCopy(Ddi_Generic_t *d, Ddi_Generic_t *s);
EXTERN Ddi_Generic_t * DdiGenericOp(Ddi_OpCode_e opcode, Ddi_OpType_e optype, Ddi_Generic_t *f, Ddi_Generic_t *g, Ddi_Generic_t *h);
EXTERN int DdiGenericBddSize(Ddi_Generic_t *f, int bound);
EXTERN Ddi_ArrayData_t * DdiGenBddRoots(Ddi_Generic_t *f);
EXTERN void logBddSupp(Ddi_Bdd_t *f);
EXTERN void logBdd(Ddi_Bdd_t *f);
EXTERN void DdiLogBdd(Ddi_Bdd_t *p, int s);
EXTERN void DdiLogBddarraySizes (Ddi_Bddarray_t *a);
EXTERN void DdiMetaFree(Ddi_Meta_t *m);
EXTERN Ddi_Meta_t * DdiMetaDup(Ddi_Meta_t *m);
EXTERN Ddi_Varset_t * DdiMetaSupp(Ddi_Meta_t *m);
EXTERN void DdiMetaDoCompl(Ddi_Meta_t *m);
EXTERN int DdiMetaIsConst(Ddi_Bdd_t *fMeta, int phase);
EXTERN Ddi_Bdd_t * DdiMetaAndExistAcc(Ddi_Bdd_t *fMeta, Ddi_Bdd_t *gBdd, Ddi_Varset_t *smooth, Ddi_Bdd_t *care);
EXTERN Ddi_Bdd_t * DdiMetaComposeAcc(Ddi_Bdd_t *fMeta, Ddi_Vararray_t *v, Ddi_Bddarray_t *g);
EXTERN Ddi_Bdd_t * DdiMetaConstrainAcc(Ddi_Bdd_t *fMeta, Ddi_Bdd_t *c);
EXTERN Ddi_Bdd_t * DdiMetaRestrictAcc(Ddi_Bdd_t *fMeta, Ddi_Bdd_t *c);
EXTERN Ddi_Bdd_t * DdiMetaSwapVarsAcc(Ddi_Bdd_t *fMeta, Ddi_Vararray_t *v1, Ddi_Vararray_t *v2);
EXTERN Ddi_Bdd_t * DdiMetaSubstVarsAcc(Ddi_Bdd_t *fMeta, Ddi_Vararray_t *v1, Ddi_Vararray_t *v2);
EXTERN Ddi_Bdd_t * DdiMetaAndAcc(Ddi_Bdd_t *fMeta, Ddi_Bdd_t *gMeta);
EXTERN Ddi_Bdd_t * DdiMetaOrAcc(Ddi_Bdd_t *fMeta, Ddi_Bdd_t *gMeta);
EXTERN Ddi_Bdd_t * DdiMetaOrDisjAcc(Ddi_Bdd_t *fMeta, Ddi_Bdd_t *gMeta);
EXTERN Ddi_Bdd_t * DdiBddMetaAlign(Ddi_Bdd_t *fMeta);
EXTERN void DdiMgrCheckVararraySize(Ddi_Mgr_t *dd);
EXTERN Ddi_Generic_t * DdiMgrNodeAlloc(Ddi_Mgr_t *ddm);
EXTERN void DdiMgrGarbageCollect(Ddi_Mgr_t *ddm);
EXTERN int DdiMgrReadIntRef(Ddi_Mgr_t *dd);
EXTERN void DdiMgrMakeVarGroup(Ddi_BddMgr *dd, Ddi_Var_t *v, int grpSize, int method);
EXTERN int DdiPreSiftHookFun(DdManager *manager, const char *str, void *heuristic);
EXTERN int DdiPostSiftHookFun(DdManager * manager, const char *str, void *heuristic);
EXTERN int DdiCheckLimitHookFun(DdManager *manager, const char *str, void *heuristic);
EXTERN void DdiVarNewFromCU(Ddi_BddMgr *ddm, DdNode *varCU);
EXTERN void DdiVarSetCU(Ddi_Var_t *v);
EXTERN Ddi_Var_t * DdiVarNewFromBaig(Ddi_BddMgr *ddm, bAigEdge_t varBaig);
EXTERN int DdiAig2CnfIdInit(Ddi_Mgr_t *ddm);
EXTERN int DdiAig2CnfIdClose(Ddi_Mgr_t *ddm);
EXTERN int DdiAig2CnfIdIsOpen(Ddi_Mgr_t *ddm);
EXTERN int DdiAig2CnfIdCloseNoCheck(Ddi_Mgr_t *ddm);
EXTERN int DdiAig2CnfId(bAig_Manager_t *manager, bAigEdge_t nodeIndex);
EXTERN int DdiAig2CnfNewId(Ddi_Mgr_t *ddm);
int DdiAig2CnfIdSigned(bAig_Manager_t *manager, bAigEdge_t nodeIndex);
EXTERN void DdiPostOrderAigVisitIntern(bAig_Manager_t *manager, bAigEdge_t nodeIndex, bAig_array_t *visitedNodes, int maxDepth);
EXTERN void DdiPostOrderAigVisitInternWithCnfActive(bAig_Manager_t *manager, bAigEdge_t nodeIndex, bAig_array_t *visitedNodes);
EXTERN void DdiPostOrderAigClearVisitedIntern(bAig_Manager_t *bmgr, bAig_array_t *visitedNodes);
EXTERN int DdiAigArraySortByLevel(Ddi_Mgr_t *ddm, bAig_array_t *aigNodes, bAigEdge_t start, int countMaxLevel);
EXTERN int DdiAigArraySortByLevelWithFlowStart(Ddi_Mgr_t *ddm, bAig_array_t *aigNodes, Ddi_Vararray_t *startVars);
EXTERN void DdiAigArrayVarsWithCnfInfo (Ddi_Mgr_t *ddm, bAig_array_t *aigNodes, bAig_array_t *varNodes);
EXTERN void DdiClauseAddLiteral(Ddi_Clause_t *cl, int lit);
EXTERN void DdiClauseRemoveLiteral(Ddi_Clause_t *cl, int i);
EXTERN void DdiClauseClear(Ddi_Clause_t *cl);
EXTERN int DdiSatSolverGetFreeVar(Ddi_SatSolver_t *solver);
EXTERN void DdiSatSolverSetFreeVarMinId (Ddi_SatSolver_t *solver, int minId);
EXTERN int DdiSatSolverReadFreeVarMinId(Ddi_SatSolver_t *solver);
EXTERN Ddi_AigSignatureArray_t * DdiAigEvalSignature(Ddi_Mgr_t *ddm, bAig_array_t *aigNodes, bAigEdge_t constBaig, int constVal, Ddi_AigSignatureArray_t *varSig);
EXTERN Ddi_AigSignatureArray_t *DdiAigArrayEvalSignatureRandomPi(Ddi_Bddarray_t *fA);
EXTERN void DdiSetSignatureConstant(Ddi_AigSignature_t *sig, int val);
EXTERN void DdiSignatureCopy(Ddi_AigSignature_t *dst,Ddi_AigSignature_t *src);
EXTERN int DdiGetSignatureBit(Ddi_AigSignature_t *sig, int bit);
EXTERN void DdiSetSignatureBit(Ddi_AigSignature_t *sig, int bit, int val);
EXTERN int DdiCountSignatureBits(Ddi_AigSignature_t *sig, int val, int bound);
EXTERN void DdiFlipSignatureBit(Ddi_AigSignature_t *sig, int bit);
EXTERN int DdiEqSignatures(Ddi_AigSignature_t *sig0, Ddi_AigSignature_t *sig1, Ddi_AigSignature_t *sigMask);
EXTERN int DdiEqComplSignatures(Ddi_AigSignature_t *sig0, Ddi_AigSignature_t *sig1, Ddi_AigSignature_t *sigMask);
EXTERN int DdiImplySignatures(Ddi_AigSignature_t *sig0, Ddi_AigSignature_t *sig1, Ddi_AigSignature_t *sigMask);
EXTERN void DdiComplementSignature(Ddi_AigSignature_t *sig);
EXTERN void DdiUpdateVarSignature(Ddi_AigSignatureArray_t *sA, Ddi_Bdd_t *f);
EXTERN void DdiUpdateVarSignatureAllBits(Ddi_AigSignatureArray_t *sA, Ddi_Bdd_t *f);
EXTERN int DdiUpdateInputVarSignatureAllBits(Ddi_AigSignatureArray_t *varSigs, Ddi_Vararray_t **psVars, Ddi_Vararray_t **piVars, Solver& S, int assumeFrames, int step, int flipInit);
EXTERN void DdiSetSignaturesRandom(Ddi_AigSignatureArray_t *sig, int n);
EXTERN void DdiSetSignatureRandom(Ddi_AigSignature_t *sig);
EXTERN void DdiEvalSignature(Ddi_AigSignature_t *sig, Ddi_AigSignature_t *sig0, Ddi_AigSignature_t *sig1, int neg0, int neg1);
EXTERN Ddi_Bdd_t *DdiAigArrayFindEquiv (Ddi_Bddarray_t *fAigarray, Ddi_Bdd_t *care, int maxObserve, int filterVars, float timeLimit);
EXTERN Ddi_Bdd_t *DdiAigFindEquiv (Ddi_Bdd_t *fAig, Ddi_Bdd_t *care, int maxObserve, int filterVars, float timeLimit);
EXTERN void DdiAigRedRemovalOdcByEqClasses (Ddi_Bdd_t *fAig, Ddi_Bdd_t *care, int maxLevel, int filterVars, float timeLimit);
EXTERN Ddi_AigCnfInfo_t *DdiGenAigCnfInfo(bAig_Manager_t *bmgr, bAig_array_t *visitedNodes, Ddi_Bddarray_t *roots, int enBlocks);
EXTERN Ddi_MinisatAigCnfMgr_t *DdiMinisatAigCnfMgrAlloc(Ddi_Mgr_t *ddm, int nNodes);
EXTERN void DdiMinisatAigCnfMgrFree(Ddi_MinisatAigCnfMgr_t *aigCnfMgr);


/**AutomaticEnd***************************************************************/
#endif /* _DDIINT */

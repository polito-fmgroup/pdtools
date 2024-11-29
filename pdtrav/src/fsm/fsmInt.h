/**CHeaderFile*****************************************************************

  FileName    [fsmInt.h]

  PackageName [fsm]

  Synopsis    [Internal header file for package Fsm]

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

#ifndef _FSMINT
#define _FSMINT

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include "ddi.h"
#include "fsm.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define FSM_MAX_STR_LEN                1000

/*
 * From nanotrav
 */

/* Different types of nodes. (Used in the "FsmPortBnetNode" type.) */
#define PORT_BNETCONSTANT_NODE 0
#define PORT_BNETINPUT_NODE 1
#define PORT_BNETPRESENT_STATE_NODE 2
#define PORT_BNETINTERNAL_NODE 3
#define PORT_BNETOUTPUT_NODE 4
#define PORT_BNETNEXT_STATE_NODE 5

/* Type of DD/AIG of a node. */
#define PORT_BNETLOCAL_DD 0
#define PORT_BNETGLOBAL_DD 1
#define PORT_BNETGLOBAL_AIG 2

#define PI_PS_FROM_FILE	0
#define PI_PS_DFS	1
#define PI_PS_GIVEN	2

#define PORT_NTRIMAGE_MONO 0
#define PORT_NTRIMAGE_PART 1
#define PORT_NTRIMAGE_CLIP 2
#define PORT_NTRIMAGE_DEPEND 3

#define PORT_NTRUNDER_APPROX 0
#define PORT_NTROVER_APPROX 1

#define PORT_NTRFROM_NEW 0
#define PORT_NTRFROM_REACHED 1
#define PORT_NTRFROM_RESTRICT 2
#define PORT_NTRFROM_COMPACT 3
#define PORT_NTRFROM_SQUEEZE 4
#define PORT_NTRFROM_UNDERAPPROX 5
#define PORT_NTRFROM_OVERAPPROX 6

#define PORT_NTRGROUP_NONE 0
#define PORT_NTRGROUP_DEFAULT 1
#define PORT_NTRGROUP_FIXED 2

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/**Enum************************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

******************************************************************************/

typedef enum {
  KeyFSM_FSM,
  KeyFSM_SIZE,
  KeyFSM_I,
  KeyFSM_O,
  KeyFSM_L,
  KeyFSM_AUXVAR,
  KeyFSM_ORD,
  KeyFSM_FILE_ORD,
  KeyFSM_NAME,
  KeyFSM_PS,
  KeyFSM_NS,
  KeyFSM_INDEX,
  KeyFSM_DELTA,
  KeyFSM_FILE_BDD,
  KeyFSM_FILE_TEXT,
  KeyFSM_STRING,
  KeyFSM_LAMBDA,
  KeyFSM_INITSTATE,
  KeyFSM_TRANS_REL,
  KeyFSM_FROM,
  KeyFSM_REACHED,
  KeyFSM_AUXFUN,
  KeyFSM_END,
  KeyFSM_UNKNOWN
} Fsm_TokenType;

/**Enum************************************************************************
  Synopsis    [TRAV ternary gate types.]
  Description []
******************************************************************************/

typedef enum {
  Fsm_And_c,
  Fsm_Const_c,
  Fsm_FreePi_c,
  Fsm_Po_c,
  Fsm_Ps_c,
  Fsm_Pi_c
} Fsm_XsimGateType_e;

/**Struct***********************************************************************

  Synopsis    [Subfields of the FSM structure.]

  Description []
  
  SideEffects []

  SeeAlso     []

******************************************************************************/

typedef struct buf_struct {
  char buf[FSM_MAX_STR_LEN];
  int index;
} buf_t;

typedef struct FsmSizeStruct {
  int i;
  int o;
  int l;
  int auxVar;
} FsmSize_t;

typedef struct FsmOrdStruct {
  char *name;
  char **nodename;
  int *nodeid;
} FsmOrd_t;

typedef struct FsmNodeNameStruct {
  char **i;
  char **o;
  char **ps;
  char **ns;
  char **nsTrueName;
  char **auxVar;
} FsmNodeName_t;

typedef struct FsmNodeIdStruct {
  int *i;
  int *o;
  int *ns;
  int *ps;
  int *auxVar;
} FsmNodeId_t;

typedef struct FsmVarStruct {
  Ddi_Vararray_t *i;
  Ddi_Vararray_t *o;
  Ddi_Vararray_t *ns;
  Ddi_Vararray_t *ps;
  Ddi_Vararray_t *auxVar;
} FsmVar_t;

typedef struct FsmBddStruct {
  char *name;
  char *string;
  Ddi_Bdd_t *bdd;
} FsmBdd_t;

typedef struct FsmBddarrayStruct {
  char *name;
  Ddi_Bddarray_t *bdd;
} FsmBddarray_t;

typedef struct FsmSettings {
  Pdtutil_VerbLevel_e verbosity;
  int cutThresh;
  int cutAtAuxVar;
  int useAig;
  int cegarStrategy;
} Fsm_Settings_t;

typedef struct FsmStats {
  int retimedConstr;
  int removedPis;
  int phaseAbstr;
  int targetEnSteps;
  int initStubSteps;
  int initStubStepsAtPhaseAbstr;
  int externalConstr;
  int xInitLatches;
} Fsm_Stats_t;

/**Struct***********************************************************************

  Synopsis    [FSM structure.]

  Description []
  
  SideEffects []

  SeeAlso     []

******************************************************************************/

struct Fsm_Mgr_s {
  char *fsmName;                /* FSM Manager Name */
  char *fileName;               /* File for the FSM Manager */
  Ddi_Mgr_t *dd;                /* DD manager */

  //FsmSize_t size;               /* Size of FSM: #PI, #PO, #FF, #auxVar */

  //char **varnames;
  //int *varauxids;
  //int *invauxids;

  int bddFormat;                /* 0 = Ascii, 1 = Binary */

  //FsmOrd_t ord;
  //FsmNodeName_t name;
  //FsmNodeId_t index;
  FsmVar_t var;

  FsmBddarray_t initStub;
  FsmBddarray_t delta;
  FsmBddarray_t lambda;
  FsmBddarray_t auxVar;

  FsmBdd_t cex;
  FsmBdd_t tr;
  FsmBdd_t from;
  FsmBdd_t reached;
  FsmBdd_t care;
  FsmBdd_t constraint;
  FsmBdd_t justice;
  FsmBdd_t fairness;
  FsmBdd_t initStubConstraint;
  FsmBdd_t init;
  FsmBdd_t invarspec;
  FsmBdd_t latchEqClasses;
  FsmBdd_t constrInvar;

  Ddi_Var_t *pdtSpecVar;        /* pdt invarpec fold var */
  Ddi_Var_t *pdtConstrVar;      /* pdt constrain fold var */
  Ddi_Var_t *pdtInitStubVar;    /* pdt init stub fold var */
  Ddi_Bdd_t *initStubPiConstr;

  Ddi_Varsetarray_t *retimedPis;

  int iFoldedProp;
  int iFoldedConstr;
  int iFoldedInit;

  long fsmTime;                 /* Overall Time Spent by the FSM Package */

  Fsm_Fsm_t *fsmList;

  Fsm_Settings_t settings;      /* Settings */
  Fsm_Stats_t stats;            /* Stats */
};

/**Struct***********************************************************************

  Synopsis    [MINI FSM structure.]

  Description []
  
  SideEffects []

  SeeAlso     []

******************************************************************************/

struct Fsm_Fsm_s {

  struct Fsm_Mgr_s *fsmMgr;

  FsmSize_t size;               /* Size of FSM: #PI, #PO, #FF, #auxVar */

  Ddi_Bddarray_t *initStub;
  Ddi_Bddarray_t *delta;
  Ddi_Bddarray_t *lambda;

  Ddi_Vararray_t *pi;
  Ddi_Vararray_t *ps;
  Ddi_Vararray_t *ns;
  Ddi_Vararray_t *is;

  Ddi_Bdd_t *init;
  Ddi_Bdd_t *cex;

  Ddi_Bdd_t *constraint;
  Ddi_Bdd_t *justice;
  Ddi_Bdd_t *fairness;
  Ddi_Bdd_t *initStubConstraint;
  Ddi_Bdd_t *invarspec;
  Ddi_Bdd_t *latchEqClasses;
  Ddi_Bdd_t *constrInvar;
  
  struct Fsm_Fsm_s *prev;
  struct Fsm_Fsm_s *next;

  int iFoldedProp;
  int iFoldedConstr;
  int iFoldedInit;

  Pdtutil_List_t *settings;     /* Settings */
  Pdtutil_List_t *stats;        /* Statistics */

};




/**Struct***********************************************************************

  Synopsis    [Retime structure.]

  Description []
  
  SideEffects []

  SeeAlso     []

******************************************************************************/

/*
struct Fsm_Retime_s {
  int *retimeArray;
  Ddi_Bddarray_t *data;
  Ddi_Bddarray_t *enable;
  int *set;
  int *refEn;
  int *enCnt;
  };
*/

/**Struct***********************************************************************

  Synopsis    [From nanotrav ... structures and declarations.]

  Description []
  
  SideEffects []

  SeeAlso     []

******************************************************************************/

/*
 * The following types implement a very simple data structure for a boolean
 * network. The intent is to be able to read a minimal subset of the blif
 * format in a data structure from which it's easy to build DDs for the
 * circuit.
 */

/*
 * Type to store a line of the truth table of a node. The entire truth table
 * implemented as a linked list of objects of this type.
 */

struct FsmPortBnetTabline_s {
  char *values;                 /* string of 1, 0, and - */
  struct FsmPortBnetTabline_s *next;    /* pointer to next table line */
};

/*
 * Node of the boolean network. There is one node in the network for each
 * primary input and for each .names directive. This structure
 * has a field to point to the DD of the node function. The function may
 * be either in terms of primary inputs, or it may be in terms of the local
 * inputs. The latter implies that each node has a variable index
 * associated to it at some point in time. The field "var" stores that
 * variable index, and "active" says if the association is currently valid.
 * (It is indeed possible for an index to be associated to different nodes
 * at different times.)
 */

struct FsmPortBnetNode_s {
  char *name;                   /* name of the output signal */
  int type;                     /* input, internal, constant, ... */
  int ninp;                     /* number of inputs to the node */
  int nfo;                      /* number of fanout nodes for this node */
  char **inputs;                /* input names */
  struct FsmPortBnetTabline_s *f;   /* truth table for this node */
  int polarity;                 /* f is the onset (0) or the offset (1) */
  int cutpoint;                 /* node is a cutpoint with pseudo-input variable */
  int manualCutpoint;           /* node is a manually generated cutpoint */
  int active;                   /* node has variable associated to it (1) or not (0) */
  Ddi_Var_t *var;               /* DD variable associated to this node */
  Ddi_Bdd_t *dd;                /* decision diagram for the function of this node */
  Ddi_Bdd_t *cutdd;             /* decision diagram for the cut output of this node */
  int exdc_flag;                /* whether an exdc node or not */
  struct FsmPortBnetNode_s *exdc;   /* pointer to exdc of dd node */
  int count;                    /* auxiliary field for DD dropping */
  int level;                    /* maximum distance from the inputs */
  int visited;                  /* flag for search */
  int evaluating;               /* flag to check combinational loops */
  int combLoop;                 /* node on combinational cycle */
  int coiFlag;                  /* used for Cone Of Influence computation */
  struct FsmPortBnetNode_s *next;   /* pointer to implement the linked list of nodes */
};

/* Very simple boolean network data structure. */
struct FsmPortBnetNetwork_s {
  char *name;                   /* network name: from the .model directive */
  int npis;                     /* number of primary inputs */
  int ninputs;                  /* number of inputs */
  char **inputs;                /* primary input names: from the .inputs directive */
  int npos;                     /* number of primary outputs */
  int noutputs;                 /* number of outputs */
  char **outputs;               /* primary output names: from the .outputs directive */
  int *outputIsConstraint;      /* primary output is a .constraint directive */
  int nlatches;                 /* number of latches */
  char ***latches;              /* next state names: from the .latch directives */
  struct FsmPortBnetNode_s *nodes;  /* linked list of the nodes */
  st_table *hash;               /* symbol table to access nodes by name */
  char *slope;                  /* wire_load_slope */
};

struct FsmPortNtrOptions_s {
  long initialTime;             /* this is here for convenience */
  int verify;                   /* read two networks and compare them */
  char *file1;                  /* first network file name */
  char *file2;                  /* second network file name */
  int second;                   /* a second network is given */
  int traverse;                 /* do reachability analysis */
  int depend;                   /* do latch dependence analysis */
  int image;                    /* monolithic, partitioned, or clip */
  double imageClip;             /* clipping depth in image computation */
  int approx;                   /* under or over approximation */
  int threshold;                /* approximation threshold */
  int from;                     /* method to compute from states */
  int groupnsps;                /* group present state and next state vars */
  int closure;                  /* use transitive closure */
  double closureClip;           /* clipping depth in closure computation */
  int envelope;                 /* compute outer envelope */
  int scc;                      /* compute strongly connected components */
  int zddtest;                  /* do zdd test */
  int maxflow;                  /* compute maximum flow in network */
  char *sinkfile;               /* file for externally provided sink node */
  int partition;                /* test McMillan conjunctive partitioning */
  int char2vect;                /* test char-to-vect decomposition */
  int density;                  /* test density-related functions */
  double quality;               /* quality parameter for density functions */
  int decomp;                   /* test decomposition functions */
  int cofest;                   /* test cofactor estimation */
  double clip;                  /* test clipping functions */
  int noBuild;                  /* do not build BDDs; just echo order */
  int stateOnly;                /* ignore primary outputs */
  char *node;                   /* only node for which to build BDD */
  int locGlob;                  /* build global or local BDDs */
  int progress;                 /* refsmPort output names while building BDDs */
  int cacheSize;                /* computed table initial size */
  unsigned long maxMemory;      /* computed table maximum size */
  int slots;                    /* unique subtable initial slots */
  int ordering;                 /* FANIN DFS ... */
  char *orderPiPs;              /* file for externally provided order */
  Cudd_ReorderingType reordering;   /* NONE RANDOM PIVOT SIFTING ... */
  int autoDyn;                  /* ON OFF */
  Cudd_ReorderingType autoMethod;   /* RANDOM PIVOT SIFTING CONVERGE ... */
  char *treefile;               /* file name for variable tree */
  int firstReorder;             /* when to do first reordering */
  int countDead;                /* count dead nodes toward triggering
                                   reordering */
  int maxGrowth;                /* maximum growth during reordering (%) */
  Cudd_AggregationType groupcheck;  /* grouping function */
  int arcviolation;             /* percent violation of arcs in
                                   extended symmetry check */
  int symmviolation;            /* percent symm violation in
                                   extended symmetry check */
  int recomb;                   /* recombination parameter for grouping */
  int nodrop;                   /* don't drop intermediate BDDs ASAP */
  int signatures;               /* computation of signatures */
  int gaOnOff;                  /* whether to run GA at the end */
  int populationSize;           /* population size for GA */
  int numberXovers;             /* number of crossovers for GA */
  int bdddump;                  /* ON OFF */
  int dumpFmt;                  /* 0 -> dot 1 -> blif 2 ->daVinci 3 -> DDcal
                                 ** 4 -> factored form */
  char *dumpfile;               /* filename for dump */
  int store;                    /* iteration at which to store Reached */
  char *storefile;              /* filename for storing Reached */
  int load;                     /* load initial states from file */
  char *loadfile;               /* filename for loading states */
  int verb;                     /* level of verbosity */
  int cutThresh;
  int coiReduction;
  int useAig;
};

typedef struct FsmPortNtrHeapSlot {
  void *item;
  int key;
} FsmPortNtrHeapSlot;

typedef struct FsmPortNtrHeap {
  int size;
  int nslots;
  FsmPortNtrHeapSlot *slots;
} FsmPortNtrHeap;

typedef struct FsmPortNtrPartTR {
  int nparts;                   /* number of parts */
  DdNode **part;                /* array of parts */
  DdNode **icube;               /* quantification cubes for image */
  DdNode **pcube;               /* quantification cubes for preimage */
  DdNode **nscube;              /* next state variables in each part */
  DdNode *preiabs;              /* present state vars and inputs in no part */
  DdNode *prepabs;              /* inputs in no part */
  DdNode *xw;                   /* cube of all present states and PIs */
  FsmPortNtrHeap *factors;      /* factors extracted from the image */
  int nlatches;                 /* number of latches */
  DdNode **x;                   /* array of present state variables */
  DdNode **y;                   /* array of next state variables */
} FsmPortNtrPartTR;

/**Struct*********************************************************************
 Synopsis    [ternary simulation manager]
 Description [ternary simulation manager]
******************************************************************************/
typedef struct xsimGateInfo {
  Fsm_XsimGateType_e type;
  char ternaryVal;
  char isPo;
  bAigEdge_t aigVal;
  bAigEdge_t aigRef;
  int fiIds[2];
  int level;
  int filterId;
} xsimGateInfo;

/**Struct*********************************************************************
 Synopsis    [ternary simulation manager]
 Description [ternary simulation manager]
******************************************************************************/
struct Fsm_XsimMgr_t {
  int strategy;
  Fsm_Mgr_t *fsmMgr;
  Ddi_Mgr_t *ddiMgr;
  Ddi_SatSolver_t * solver;
  int nPi, nPo, nPs, nGate, nFiltered;
  xsimGateInfo *xsimGates;
  Ddi_AigSignatureArray_t *gateSigs;
  Pdtutil_EqClasses_t *eqCl;
  int *po;
  int *ps;
  int *pi;
  int *psEq;
  int *gateFilter;
  int target;
  int nLevels;
};

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**Macro***********************************************************************

  Synopsis     [Returns 1 if the two arguments are identical strings.]

  Description  []

  SideEffects  [none]

  SeeAlso      []
		   
******************************************************************************/

#define STRING_EQUAL(s1,s2) (strcmp((s1),(s2)) == 0)

EXTERN int fsmPortBnetSetLevel(
  FsmPortBnetNetwork_t * net
);
EXTERN int fsmPortBnetLevelDFS(
  FsmPortBnetNetwork_t * net,
  FsmPortBnetNode_t * node
);

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EXTERN void FsmPortBnetGenSliceVars(
  Ddi_Mgr_t * dd,
  FsmPortBnetNetwork_t * net
);
EXTERN int FsmCoiReduction(
  FsmPortBnetNetwork_t * net,
  char **coiList
);
EXTERN Fsm_TokenType FsmString2Token(
  char *keyword
);
EXTERN char *FsmToken2String(
  Fsm_TokenType token
);
EXTERN Ddi_Bdd_t *Pdtutil_ReadConstrain(
  Fsm_Mgr_t * fsmMgr,
  char *diagfile
);


/**AutomaticEnd***************************************************************/

#endif                          /* _FSMINT */

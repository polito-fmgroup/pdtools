/**CFile***********************************************************************

  FileName    [ddiMgr.c]

  PackageName [ddi]

  Synopsis    [Functions to deal with DD Managers]

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

#include "ddiInt.h"
#include "baigInt.h"
#include <pthread.h>

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define DDI_MAX_REGISTERED_MGR 100
#define KEEP_FREE_LIST 1

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

Ddi_Mgr_t *gblMgr[10]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
int gblMgrNum=0;

  static char *typenames[] =
  {
    "Bdd",
    "Var",
    "Varset",
    "Expr",
    "Bddarray",
    "Vararray",
    "Varsetarray",
    "Exprarray"
  };

  static struct {
    DdManager *cuMgr;
    Ddi_Mgr_t *pdtMgr;
  } registeredMgr[DDI_MAX_REGISTERED_MGR];

  static pthread_mutex_t registeredMgrMutex = PTHREAD_MUTEX_INITIALIZER;

  static int nRegisteredMgr = 0;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static int MgrRegister(DdManager * manager, Ddi_Mgr_t * ddiMgr);
static int MgrUnRegister(DdManager * manager);
static Ddi_Mgr_t * MgrCuToPdt(DdManager * manager);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis     [make checks and resizes arrays if required]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
DdiMgrCheckVararraySize(
  Ddi_Mgr_t *dd            /* source dd manager */
  )
{
  int i, sizeOld;
  int nvars = Ddi_MgrReadNumVars(dd);

  if (dd->vararraySize<nvars) {
    sizeOld = dd->vararraySize;
    if (sizeOld==0) {
      dd->vararraySize = nvars;
    }
    else while (dd->vararraySize<nvars) {
      dd->vararraySize *= 2;
    }
    dd->varnames = Pdtutil_Realloc(char *, dd->varnames, dd->vararraySize);
    dd->varauxids = Pdtutil_Realloc(int, dd->varauxids, dd->vararraySize);
    dd->varBaigIds = Pdtutil_Realloc(bAigEdge_t, dd->varBaigIds, dd->vararraySize);
    dd->varIdsFromCudd = Pdtutil_Realloc(int, dd->varIdsFromCudd, dd->vararraySize);
    for (i = sizeOld; i<dd->vararraySize; i++) {
      dd->varnames[i] = NULL;
      dd->varauxids[i] = -1;
      dd->varBaigIds[i] = bAig_NULL;
      dd->varIdsFromCudd[i] = -1;
    }
  }

}

/**Function********************************************************************
  Synopsis     [Garbage collect freed nodes (handles) in manager node list]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
Ddi_Generic_t *
DdiMgrNodeAlloc (
  Ddi_Mgr_t *ddiMgr
)
{
  Ddi_Generic_t *node = ddiMgr->freeNodeList;
  if (node == NULL) {
    node = Pdtutil_Alloc(Ddi_Generic_t,1);
  }
  else {
    //    Pdtutil_Assert(node->common.status==Ddi_Free_c,
    //		   "free node required in free list"); 
    ddiMgr->freeNodeListNum++;
    ddiMgr->freeNodeList = node->common.next;
  }
  return(node);
}

/**Function********************************************************************
  Synopsis     [Garbage collect freed nodes (handles) in manager node list]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
DdiMgrGarbageCollect (
  Ddi_Mgr_t *ddiMgr
)
{
  Ddi_Generic_t *curr, *next, *last;
  int chkList = 0;
#if 0
  Pdtutil_Assert(Ddi_MgrConsistencyCheck(ddiMgr)!=0,
             "Error found in manager check");
#endif

  if (ddiMgr->nodeList == NULL)
    return;

  if (chkList) {
    int i=0;
    last = curr = ddiMgr->nodeList;
    while (curr!=NULL) {
      Pdtutil_Assert(curr->common.nodeid>=0,"repeated node in ddi node list");
      curr->common.nodeid *= -1;
      last = curr;
      curr = curr->common.next;
      i++;
    }
    curr = ddiMgr->nodeList;
    while (curr!=NULL) {
      curr->common.nodeid *= -1;
      curr = curr->common.next;
    }
  }

  last = NULL;
  curr = ddiMgr->nodeList;
  ddiMgr->nodeList = NULL;

  /*
   *  scan list: free nodes in free status,
   *  rebuild ddiMgr->nodeList keeping order
   */
  while (curr!=NULL) {
    next = curr->common.next;
    if (curr->common.status == Ddi_Free_c) {
#if KEEP_FREE_LIST
      curr->common.next = ddiMgr->freeNodeList;
      ddiMgr->freeNodeList = curr;
      ddiMgr->freeNodeListNum++;
#else
      Pdtutil_Free(curr);
#endif
    }
    else {
      if (last == NULL) {
      ddiMgr->nodeList = curr;
      }
      else {
      last->common.next = curr;
      }
      last = curr;
      curr->common.next = NULL;
    }
    curr = next;
  }

  ddiMgr->allocatedNum = ddiMgr->genericNum;
  ddiMgr->freeNum = 0;

  return;
}

/**Function********************************************************************
  Synopsis     [Read the counter of internal references]
  Description  [Read the number of internally referenced DDI handles.
    Variable array and/or one/zero constants.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
DdiMgrReadIntRef(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  int intRef = 0;

  if (dd->varGroups != NULL) {
     intRef++; /* variable group array */
  }
  if (dd->variables != NULL) {
     intRef++; /* variable array */
  }
  if (dd->baigVariables != NULL) {
     intRef++; /* variable array */
  }
  if (dd->cuddVariables != NULL) {
     intRef++; /* variable array */
  }
  if (dd->one != NULL) {
     intRef++; /* one constant */
  }
  if (dd->zero != NULL) {
     intRef++; /* zero constant */
  }
  return (intRef);
}



/**Function********************************************************************
  Synopsis    [Create a variable group]
  Description [Create a variable group. Method is one of MTR_FIXED,
    MTR_DEFAULT]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/
void
DdiMgrMakeVarGroup(
  Ddi_BddMgr   *dd,
  Ddi_Var_t   *v,
  int grpSize,
  int method
)
{
  int i, p0, j;
  Ddi_Varset_t *vs;

  vs = Ddi_VarsetVoid(dd);
  p0=Ddi_VarCurrPos(v);
  for (i=0;i<grpSize;i++) {
    j = Ddi_MgrReadInvPerm(dd,p0+i);
    Ddi_VarsetAddAcc(vs,Ddi_VarFromCuddIndex(dd,j));
  }
  for (i=0;i<grpSize;i++) {
    j = Ddi_MgrReadInvPerm(dd,p0+i);
    Ddi_VarsetarrayWrite(dd->varGroups,j,vs);
  }
  Ddi_Free(vs);

  Cudd_MakeTreeNode(dd->mgrCU,Ddi_VarCuddIndex(v),grpSize,method);
}

/**Function********************************************************************

  Synopsis []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int
DdiPreSiftHookFun(
  DdManager *manager,
  const char *str,
  void *heuristic
  )
{
  Ddi_Mgr_t *ddiMgr;

  Pdtutil_Assert(str!=NULL,"Non NULL string required by Sift hook fun");

  if (strcmp(str,"BDD")==0) {
    ddiMgr = MgrCuToPdt(manager);
    if ((ddiMgr->autodynMasked)||(ddiMgr->autodynSuspended)||
        ((ddiMgr->abortOnSift >= 1) &&
        (Ddi_MgrReadLiveNodes(ddiMgr) < ddiMgr->autodynNextDyn))) {
      return(0);
    }

    ddiMgr->stats.siftLastInit = util_cpu_time ();

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout,
        "\nDynamic Reordering: %d ... ", Ddi_MgrReadLiveNodes(ddiMgr))
      ); fflush(stdout);
  }

  return(1);
}

/**Function********************************************************************

  Synopsis []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int
DdiPostSiftHookFun(
  DdManager * manager,
  const char *str,
  void *heuristic
  )
{
  Ddi_Mgr_t *ddiMgr;

  Pdtutil_Assert(str!=NULL,"Non NULL string required by sift hook fun");

  if (strcmp(str,"BDD")==0) {

    ddiMgr = MgrCuToPdt(manager);
    ddiMgr->stats.siftRunNum++;
    ddiMgr->stats.siftCurrOrdNodeId = ddiMgr->currNodeId;

    ddiMgr->autodynNextDyn = Cudd_ReadNextReordering(manager);

    ddiMgr->stats.siftLastTime = util_cpu_time () - ddiMgr->stats.siftLastInit;
    ddiMgr->stats.siftTotTime += ddiMgr->stats.siftLastTime;

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "%d - sift time (last/tot): %.2f/%.2f.\n", 
	      Ddi_MgrReadLiveNodes(ddiMgr), 
	      ddiMgr->stats.siftLastTime/1000.0,
	      ddiMgr->stats.siftTotTime/1000.0
	      )
    );

#if 0
      Ddi_MgrOrdWrite (ddiMgr, "lastord.ord", NULL,
        Pdtutil_VariableOrderComment_c);
#endif

    if (ddiMgr->abortOnSift >= 1) {
      int nextAbort = ddiMgr->abortOnSiftThresh[ddiMgr->abortOnSift-1];
      if (ddiMgr->autodynNextDyn > nextAbort) {
        Cudd_SetNextReordering(Ddi_MgrReadMgrCU(ddiMgr), nextAbort);
      }
    }

    if ((ddiMgr->autodynMaxCalls>=0) &&
        (ddiMgr->stats.siftRunNum >= ddiMgr->autodynMaxCalls) ||
        (ddiMgr->autodynMaxTh>=0) &&
        (ddiMgr->autodynNextDyn > ddiMgr->autodynMaxTh)) {
#if 0
      int newNext = 1.5*ddiMgr->autodynNextDyn;
      Ddi_MgrSetSiftThresh (ddiMgr,newNext);
      ddiMgr->autodynMaxTh += 0.5*newNext;
#else
      Ddi_MgrSetSiftThresh (ddiMgr,500000000);
#endif
      //      Ddi_MgrSiftDisable(ddiMgr);
    }
  }

  return(1);
}

/**Function********************************************************************

  Synopsis    [Check for Memory and Time Overflow.]

  Description [Check for Memory and Time Overflow.
    Memory and Time limits are set in the DDI manager.
    Current memory usage and time limits are check against set values.
    An error message is issued if an overflow is discovered and the
    execution is stopped.
    ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int
DdiCheckLimitHookFun (
  DdManager *manager,
  const char *str,
  void *heuristic
  )
{
  Ddi_Mgr_t *ddiMgr;
  long timeLimit, timeCurrent;

  ddiMgr = MgrCuToPdt (manager);

  //if (ddiMgr->limit.memoryLimit != (-1)) {
  if (ddiMgr->limit.memoryLimit > 0) {
    /* Check for Memory Limit */
    if ((Cudd_ReadMemoryInUse (manager)/1024) > ddiMgr->limit.memoryLimit) {
      fprintf(stderr, "Memory Limit Reached:.\n");
      fprintf(stderr, "Limit=%ld [KBytes], Current=%ld [KBytes].\n",
        ddiMgr->limit.memoryLimit, Ddi_MgrReadMemoryInUse (ddiMgr)/1024);
      fflush (stderr);
      Pdtutil_CatchExit (ERROR_CODE_MEM_LIMIT);
    }
  }

  //if (ddiMgr->limit.timeInitial != (-1) && ddiMgr->limit.timeLimit != (-1)) {
  if (ddiMgr->limit.timeLimit > 0) {
    /* Check for Time Limit */
    timeCurrent = util_cpu_time() - ddiMgr->limit.timeInitial;
    if (timeCurrent > ddiMgr->limit.timeLimit*1000) {
      fprintf(stderr, "Time limit reached: ");
      fprintf(stderr, "Threshold=%d s, used=%.1f s.\n",
        ddiMgr->limit.timeLimit, timeCurrent/1000.0);
      //fprintf(stderr,
      //  "Initial=%ld, DeltaT=%ld [sec], Limit=%ld, Current=%ld.\n",
      //  ddiMgr->limit.timeInitial, ddiMgr->limit.timeLimit,
      //  timeLimit, timeCurrent/1000);
      fflush (stderr);
      Pdtutil_CatchExit (ERROR_CODE_TIME_LIMIT);
    }
  }

  return(1);
}

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis     [Creates a DdManager.]

  SideEffects  [none]

  SeeAlso      [Ddi_MgrQuit]

******************************************************************************/

Ddi_Mgr_t *
Ddi_MgrInit (
  char *ddiName                /* Name of the DDI structure */,
  DdManager *CUMgr             /* Input CD manager. Created if NULL */,
  unsigned int nvar            /* Initial Number of Variables */,
  unsigned int numSlots        /* Initial Size of Unique Table */,
  unsigned int cacheSize       /* Initial Size of Computed Table (cache) */,
  unsigned long cuddMemoryLimit/* Max size of Memory for CUDD */,
  long int memoryLimit         /* Memory Limit (-1 = No limit) */,
  long int timeLimit           /* CPU Time Limit (-1 = No Limit) */
  )
{
  Ddi_Mgr_t *ddiMgr;
  int i;

  long startTime, cpuTime;

  startTime = util_cpu_time();

  if (cuddMemoryLimit == 0) {
    cuddMemoryLimit = getSoftDataLimit();
#if 0
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "MemoryMax = %d.\n", cuddMemoryLimit)
    );
#endif
  }

  /*------------------- Init random number generator -----------------------*/

  Cudd_Srandom(1);
  srand(100);

  /*------------------------- DDI Structure Creation -----------------------*/

  ddiMgr = Pdtutil_Alloc (Ddi_Mgr_t, 1);
  if (ddiMgr == NULL) {
    return (NULL);
  }

  /*------------------- Set the Name of the DDI Structure ------------------*/

  if (ddiName == NULL) {
     ddiName = "ddiMgr";
  }
  Ddi_MgrSetName(ddiMgr, ddiName);

  /*------------------------------- Set Fields -----------------------------*/

  if (CUMgr == NULL) {
    CUMgr = Cudd_Init (nvar, 0, numSlots, cacheSize, cuddMemoryLimit);
    if (CUMgr == NULL) {
      Pdtutil_Free (ddiMgr);
      return (NULL);
    }
  }

  Cudd_AddHook (CUMgr, DdiPreSiftHookFun, CUDD_PRE_REORDERING_HOOK);
  Cudd_AddHook (CUMgr, DdiPostSiftHookFun, CUDD_POST_REORDERING_HOOK);
  Cudd_AddHook (CUMgr, DdiCheckLimitHookFun, CUDD_PRE_GC_HOOK);
  Cudd_AddHook (CUMgr, DdiCheckLimitHookFun, CUDD_POST_REORDERING_HOOK);

  MgrRegister(CUMgr,ddiMgr);

  ddiMgr->mgrCU = CUMgr;
  ddiMgr->autodynMethod = (Cudd_ReorderingType) 0;
  ddiMgr->autodynEnabled = 0;
  ddiMgr->autodynMasked = 0;
  ddiMgr->autodynSuspended = 0;
  ddiMgr->autodynNestedSuspend = 0;
  ddiMgr->autodynNextDyn = Cudd_ReadNextReordering(CUMgr);
  ddiMgr->autodynMaxTh = -1;
  ddiMgr->autodynMaxCalls = -1;
  ddiMgr->abortOnSift = 0;
  ddiMgr->abortOnSiftMasked = 0;
  ddiMgr->abortedOp = 0;
  ddiMgr->currNodeId = 0;
  ddiMgr->tracedId = -1; /* trace disabled */
  ddiMgr->freeNum = 0;
  ddiMgr->lockedNum = 0;
  ddiMgr->allocatedNum = 0;
  ddiMgr->genericNum = 0;

  for (i=0; i<DDI_NTYPE; i++) {
    ddiMgr->typeNum[i] = 0;
    ddiMgr->typeLockedNum[i] = 0;
  }

  ddiMgr->nodeList = NULL;
  ddiMgr->freeNodeList = NULL;
  ddiMgr->freeNodeListNum = 0;

  ddiMgr->one = Ddi_BddMakeFromCU(ddiMgr,Cudd_ReadOne(CUMgr));
  Ddi_Lock(ddiMgr->one);
  ddiMgr->zero = Ddi_BddMakeFromCU(ddiMgr,Cudd_Not(Cudd_ReadOne(CUMgr)));
  Ddi_Lock(ddiMgr->zero);

  ddiMgr->meta.groupNum = 0;
  ddiMgr->meta.nvar = 0;
  ddiMgr->meta.bddNum = 0;
  ddiMgr->meta.groups = NULL;
  ddiMgr->meta.ord = NULL;
  ddiMgr->meta.id = 0;

  ddiMgr->exist.clipDone = 0;
  ddiMgr->exist.clipRetry = 0;
  ddiMgr->exist.clipLevel = 0;
  ddiMgr->exist.clipThresh = 100000000;
  ddiMgr->exist.clipWindow = NULL;
  ddiMgr->exist.activeWindow = NULL;
  ddiMgr->exist.ddiExistRecurLevel = -1;
  ddiMgr->exist.conflictProduct = NULL;
  ddiMgr->exist.infoPtr = Cuplus_bddAndAbstractInfoInit();

  ddiMgr->aig.mgr = NULL;
  ddiMgr->aig.auxLits = NULL;
  ddiMgr->aig.auxLits2 = NULL;
  ddiMgr->aig.actVars = NULL;

  ddiMgr->cnf.maxCnfId = 0;
  ddiMgr->cnf.cnf2aigSize = 0;
  ddiMgr->cnf.cnf2aig = NULL;
  ddiMgr->cnf.cnfActive = NULL;
  ddiMgr->cnf.cnf2aigOpen=0;
  ddiMgr->cnf.cnf2aigStrategy=0;
  ddiMgr->cnf.solver2cnf = NULL;
  ddiMgr->cnf.solver2cnfSize = 0;
  ddiMgr->cnf.itpExtraGlobalNodes = NULL;
  ddiMgr->cnf.savedAigNodes = NULL;
  ddiMgr->cnf.useSavedAigsForSat = 0;

  /*---------------- Init variable names and auxids ------------------------*/

  ddiMgr->varnames = NULL;
  ddiMgr->varNamesIdxHash = NULL;

  ddiMgr->varauxids = NULL;
  ddiMgr->varBaigIds = NULL;
  ddiMgr->varIdsFromCudd = NULL;
  if (nvar > PDTUTIL_INITIAL_ARRAY_SIZE) {
    ddiMgr->vararraySize = nvar;
  } else {
    ddiMgr->vararraySize = PDTUTIL_INITIAL_ARRAY_SIZE;
  }

  ddiMgr->varnames = Pdtutil_Alloc(char *, ddiMgr->vararraySize);
  ddiMgr->varNamesIdxHash = st_init_table(strcmp, st_strhash);

  ddiMgr->varauxids = Pdtutil_Alloc(int, ddiMgr->vararraySize);
  ddiMgr->varBaigIds = Pdtutil_Alloc(bAigEdge_t, ddiMgr->vararraySize);
  ddiMgr->varIdsFromCudd = Pdtutil_Alloc(bAigEdge_t, ddiMgr->vararraySize);

  for (i=0; i<ddiMgr->vararraySize; i++) {
    ddiMgr->varnames[i] = NULL;
    ddiMgr->varauxids[i] = (-1);
    ddiMgr->varBaigIds[i] = bAig_NULL;
    ddiMgr->varIdsFromCudd[i] = (-1);
  }

  //ddiMgr->variables = Ddi_VararrayAlloc(ddiMgr,nvar);
  ddiMgr->variables = Ddi_VararrayAlloc(ddiMgr,0);
  Ddi_Lock(ddiMgr->variables);
  ddiMgr->varGroups = Ddi_VarsetarrayAlloc(ddiMgr,nvar);
  Ddi_Lock(ddiMgr->varGroups);

  ddiMgr->cuddVariables = Ddi_VararrayAlloc(ddiMgr,0);
  Ddi_Lock(ddiMgr->cuddVariables);

  for (i=0; i<nvar; i++) {
    DdiVarNewFromCU(ddiMgr,Cudd_bddIthVar(CUMgr,i));
  }


  ddiMgr->baigVariables = Ddi_VararrayAlloc(ddiMgr,0);
  Ddi_Lock(ddiMgr->baigVariables);

  ddiMgr->varCntSortData.varSize = 0;
  ddiMgr->varCntSortData.baigVarSize = 0;
  ddiMgr->varCntSortData.varCnt = NULL;
  ddiMgr->varCntSortData.baigVarCnt = NULL;

  /*------------------------- Default Settings -----------------------------*/

  ddiMgr->settings.verbosity = Pdtutil_VerbLevelUsrMax_c;
  ddiMgr->settings.stdout = stdout;

  ddiMgr->settings.part.existOptClustThresh = 1000;
  ddiMgr->settings.part.existClustThresh = -1;
  ddiMgr->settings.part.existOptLevel = 2;
  ddiMgr->settings.part.existDisjPartTh = -1;

  ddiMgr->settings.meta.groupSizeMin = DDI_META_GROUP_SIZE_MIN_DEFAULT;
  ddiMgr->settings.meta.method = Ddi_Meta_Size;
  ddiMgr->settings.meta.maxSmoothVars = 0;

  ddiMgr->settings.dump.partOnSingleFile = 1;

  ddiMgr->settings.aig.lazyRate = -1.0;
  ddiMgr->settings.aig.satSolver = Pdtutil_StrDup("minisat");
  ddiMgr->settings.aig.abcOptLevel = 0; // 3
  ddiMgr->settings.aig.bddOptLevel = 0;
  ddiMgr->settings.aig.redRemLevel = 0; // 1
  ddiMgr->settings.aig.redRemMaxCnt = 1;
  ddiMgr->settings.aig.redRemCnt = 0;
  ddiMgr->settings.aig.itpIteOptTh = DDI_ITP_ITEOPT_TH_DEFAULT;
  ddiMgr->settings.aig.itpPartTh = DDI_ITP_PART_TH_DEFAULT;
  ddiMgr->settings.aig.itpStoreTh = DDI_ITP_STORE_TH_DEFAULT;
  ddiMgr->settings.aig.nnfClustSimplify = 1;
  ddiMgr->settings.aig.dynAbstrStrategy = 0; // 1
  ddiMgr->settings.aig.ternaryAbstrStrategy = 1;
  ddiMgr->settings.aig.maxAllSolutionIter = 100;
  ddiMgr->settings.aig.satIncrByRefinement = 0;
  ddiMgr->settings.aig.satTimeout = 0; // 1
  ddiMgr->settings.aig.satIncremental = 1;
  ddiMgr->settings.aig.satVarActivity = 0;

  ddiMgr->settings.aig.satCompare = 0;
  ddiMgr->settings.aig.bddCompare = 0;

  ddiMgr->settings.aig.satTimeLimit = 30;
  ddiMgr->settings.aig.satTimeLimit1 = 50;
  ddiMgr->settings.aig.enMetaOpt = 0;

  ddiMgr->settings.aig.lemma_done_bound=-1;

  ddiMgr->settings.aig.itpOpt = 2;
  ddiMgr->settings.aig.itpActiveVars = 0;
  ddiMgr->settings.aig.itpAbc = 0;
  ddiMgr->settings.aig.itpTwice = 0;
  ddiMgr->settings.aig.itpReverse = 0;
  ddiMgr->settings.aig.itpNnfAbstrAB = 3;
  ddiMgr->settings.aig.itpNoQuantify = 0;
  ddiMgr->settings.aig.itpAbortTh = -1;
  ddiMgr->settings.aig.itpCore = 0;
  ddiMgr->settings.aig.itpAigCore = 0;
  ddiMgr->settings.aig.itpMem = 4;
  ddiMgr->settings.aig.itpMap = 0;
  ddiMgr->settings.aig.itpStore = NULL;
  ddiMgr->settings.aig.itpLoad = 0;
  ddiMgr->settings.aig.itpDrup = 0;
  ddiMgr->settings.aig.itpUseCare = 0;
  ddiMgr->settings.aig.itpCompute = 0;
  ddiMgr->settings.aig.partialItp = 0;

  ddiMgr->settings.aig.enItpBddOpt = 1;
  ddiMgr->settings.aig.enBddFoConOpt = 1;
  ddiMgr->settings.aig.aigCnfLevel = 1; // 2

  /*---------------------- Stats and other stuff ---------------------------*/

  ddiMgr->stats.peakProdLocal = 0;
  ddiMgr->stats.peakProdGlobal = 0;
  ddiMgr->stats.siftCompactionLast = 0.0;
  ddiMgr->stats.siftCompactionPred = 0.0;
  ddiMgr->stats.siftRunNum = 0;
  ddiMgr->stats.siftCurrOrdNodeId = 0;
  ddiMgr->stats.siftLastInit = 0;
  ddiMgr->stats.siftLastTime = 0;
  ddiMgr->stats.siftTotTime = 0;

  ddiMgr->stats.aig.n_merge_1 = 0;
  ddiMgr->stats.aig.n_merge_2 = 0;
  ddiMgr->stats.aig.n_merge_3 = 0;
  ddiMgr->stats.aig.n_check_1 = 0;
  ddiMgr->stats.aig.n_check_2 = 0;
  ddiMgr->stats.aig.n_check_3 = 0;
  ddiMgr->stats.aig.n_diff = 0;
  ddiMgr->stats.aig.n_diff_1 = 0;
  ddiMgr->stats.aig.fullAndItpTerms=0;
  ddiMgr->stats.aig.fullOrItpTerms=0;
  ddiMgr->stats.aig.itpTerms=0;
  ddiMgr->stats.aig.itpPartialExist=0;

  if (gblMgrNum<7) {
    gblMgr[gblMgrNum++] = ddiMgr;
  }

  ddiMgr->limit.memoryLimit = memoryLimit;
  ddiMgr->limit.timeInitial = util_cpu_time(); //Pdtutil_ConvertTime (util_cpu_time());
  ddiMgr->limit.timeLimit = timeLimit;

  ddiMgr->cuPlus.elseFirst = NULL;
  ddiMgr->cuPlus.initElseFirst = 0;

  /*------------------------- Printing Message -----------------------------*/

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "-- DDI Manager Init.\n")
  );

  cpuTime = util_cpu_time();
  //printf("DDI init time = %s\n", util_print_time (1000*(cpuTime-startTime)));

  return (ddiMgr);
}

/**Function********************************************************************
  Synopsis     [make checks on DDI manager. Return 0 for failure]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrConsistencyCheck (
  Ddi_Mgr_t *ddiMgr
)
{
  int ret;
  int freeNum, totNum;
  Ddi_Generic_t *curr;

  DdiMgrCheckVararraySize(ddiMgr);

  ret = 1;

  if (ddiMgr->aig.mgr != NULL) {
    Ddi_Bddarray_t *aigRoots = Ddi_BddarrayAlloc(ddiMgr,0);
    curr = ddiMgr->nodeList;
    while (curr!=NULL) {
      if (curr->common.status != Ddi_Free_c) {
        if (curr->common.code == Ddi_Bdd_Aig_c) {
        Ddi_BddarrayInsertLast(aigRoots,(Ddi_Bdd_t *)curr);
        }
      }
      curr = curr->common.next;
    }
    if (Ddi_BddarrayNum(aigRoots) > 0) {
      DdiAigMgrGarbageCollect(aigRoots);
      DdiAigMgrCheck(aigRoots);
    }
    Ddi_Free(aigRoots);
  }


  /*
   *  scan node list counting nodes
   */
  totNum = freeNum = 0;
  curr = ddiMgr->nodeList;
  while (curr!=NULL) {
    totNum++;
    if (curr->common.status == Ddi_Free_c) {
      freeNum++;
    }
    curr = curr->common.next;
  }
  if ((ddiMgr->genericNum+ddiMgr->freeNum)!=ddiMgr->allocatedNum) {
    if (Ddi_MgrReadVerbosity(ddiMgr) >= Pdtutil_VerbLevelUsrMin_c) {
      fprintf(stderr,
        "Wrong number of free/allocated DDI nodes found in mgr check.\n");
      fprintf(stderr,"allocated nodes #: %d.\n", ddiMgr->genericNum);
      fprintf(stderr,"free      nodes #: %d.\n", ddiMgr->freeNum);
      fprintf(stderr,"live      nodes #: %d.\n", ddiMgr->genericNum);
    }
    ret = 0;
  }
  if (ddiMgr->allocatedNum!=totNum) {
    fprintf(stderr,"#Wrong total number of DDI nodes found in mgr check.\n");
    fprintf(stderr,"#Recorded nodes: %d.\n", ddiMgr->allocatedNum);
    fprintf(stderr,"#Found nodes: %d.\n", totNum);
    ret = 0;
  }
  if (ddiMgr->freeNum!=freeNum) {
    fprintf(stderr, "Wrong number of free DDI nodes found in mgr check.\n");
    fprintf(stderr, "#Recorded nodes: %d.\n", ddiMgr->freeNum);
    fprintf(stderr, "#Found    nodes: %d.\n", freeNum);
    ret = 0;
  }

#ifndef __alpha__
  /* check CUDD manager */
  if (0 && Cudd_DebugCheck (ddiMgr->mgrCU)!=0) {
    fprintf(stderr, "CUDD Manager Check Failed.\n");
    ret = 0;
  };
#endif

#if 0
  if (Cudd_CheckKeys(ddiMgr->mgrCU)!=0) {
    fprintf(stderr, "CUDD Key Check Failed.\n");
    ret = 0;
  };
#endif

  return(ret);
}

/**Function********************************************************************
  Synopsis     [Check number of externally referenced DDI handles]
  Description  [Check number of externally referenced DDI handles.
    This is the numer of generic nodes (allocated - freed), diminished by
    the number of locked nodes + 3 (2 constants + variable array).
    Return 0 upon failure.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrCheckExtRef (
  Ddi_Mgr_t *ddiMgr,
  int n
)
{
  int ret=1;

  /* allocated DDI nodes include 2 constants and variable array */
  if (n!=Ddi_MgrReadExtRef(ddiMgr)) {
    fprintf(stderr,"Wrong Number of External References in Mgr Check.\n");
    fprintf(stderr,"Required: %d - Found %d.\n", n, Ddi_MgrReadExtRef(ddiMgr));
    Ddi_MgrPrintAllocStats(ddiMgr,stderr);
    Ddi_MgrPrintExtRef(ddiMgr,0);
    ret = 0;
  }

  return(ret);
}


/**Function********************************************************************
  Synopsis     [print ids of external refs]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrPrintExtRef (
  Ddi_Mgr_t *ddiMgr,
  int minNodeId
)
{
  Ddi_Generic_t *curr;

  //  Pdtutil_Assert(Ddi_MgrConsistencyCheck(ddiMgr)!=0,
  //             "Error found in manager check");

  if (ddiMgr->nodeList == NULL)
    return;

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Externally referenced DDI nodes:.\n")
  );
  for (curr=ddiMgr->nodeList; curr!=NULL; curr=curr->common.next) {
    if (curr->common.status == Ddi_Unlocked_c) {
      if (curr->common.nodeid >= minNodeId) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "<%s:%d> ", typenames[DdiType(curr)],
            curr->common.nodeid)
        );
      }
    }
  }
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "\n")
  );

  return;
}

/**Function********************************************************************
  Synopsis     [free externally referenced nodes]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrFreeExtRef (
  Ddi_Mgr_t *ddiMgr,
  int minNodeId
)
{
  bAig_Manager_t *aigMgr = ddiMgr->aig.mgr;
  Ddi_Generic_t *curr;
  int i;

  if (1 && aigMgr) {
    /* clean the aux fields */
    for (i=0; i<aigMgr->nodesArraySize/4; i++) {
      aigMgr->auxInt[i] = -1;
      //aigMgr->auxPtr[i] = aigMgr->auxPtr0[i] = aigMgr->auxPtr1[i] = NULL;
      //aigMgr->cacheAig[i] = aigMgr->auxAig0[i] = aigMgr->auxAig1[i] = bAig_NULL;
      aigMgr->visited[i] = 0;
    }
  }

  Pdtutil_Assert(Ddi_MgrConsistencyCheck(ddiMgr)!=0,
             "Error found in manager check");

  if (ddiMgr->nodeList == NULL)
    return;

  for (curr=ddiMgr->nodeList; curr!=NULL; curr=curr->common.next) {
    if (curr->common.status == Ddi_Unlocked_c) {
      if (curr->common.nodeid >= minNodeId) {
      Ddi_Generic_t *fc = curr; /* duplicated since free set var=0 */
      Ddi_Free(fc);
      }
    }
  }

  if (1 && aigMgr) {
    /* clean the aux fields */
    for (i=0; i<aigMgr->nodesArraySize/4; i++) {
      aigMgr->auxInt[i] = -1;
      //aigMgr->auxPtr[i] = aigMgr->auxPtr0[i] = aigMgr->auxPtr1[i] = NULL;
      //aigMgr->cacheAig[i] = aigMgr->auxAig0[i] = aigMgr->auxAig1[i] = bAig_NULL;
      //aigMgr->visited[i] = 0;
    }
  }
}


/**Function********************************************************************
  Synopsis     [print ids of external refs]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrPrintBySize (
  Ddi_Mgr_t *ddiMgr,
  int minSize,
  int minNodeId
)
{
  Ddi_Generic_t *curr;
  int size;
  char unlocked;
  Pdtutil_Assert(Ddi_MgrConsistencyCheck(ddiMgr)!=0,
             "Error found in manager check");

  if (ddiMgr->nodeList == NULL)
    return;

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Externally referenced DDI nodes:.\n")
  );
  for (curr=ddiMgr->nodeList; curr!=NULL; curr=curr->common.next) {

    if (curr->common.nodeid < minNodeId) {
       continue;
    }
    if (curr->common.status == Ddi_Unlocked_c) {
       unlocked = 'u';
    }
    else if (curr->common.status == Ddi_Locked_c) {
       unlocked = 'l';
    }
    else {
       continue;
    }
    switch (curr->common.type) {

    case Ddi_Bdd_c:
      size = Ddi_BddSize((Ddi_Bdd_t *)curr);
      if (size > minSize) {
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, "<B(%c): %d(s:%d)> ", unlocked, curr->common.nodeid, size)
       );
      }
      break;
    case Ddi_Bddarray_c:
      size = Ddi_BddarraySize((Ddi_Bddarray_t *)curr);
      if (size > minSize) {
       Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
         Pdtutil_VerbLevelNone_c,
         fprintf(stdout, "<BA(%c): %d(s:%d)> ", unlocked, curr->common.nodeid, size)
       );
      }
      break;
    default:
      break;
    }
  }

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "\n")
  );

  return;
}


/**Function********************************************************************
  Synopsis     [update DDI manager after directly working on CUDD manager]
  Description  [update DDI manager after directly working on CUDD manager.
    New variables have possibly been created.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrUpdate(
  Ddi_Mgr_t *ddiMgr
)
{
  int i;
  int nvars = Ddi_MgrReadNumCuddVars(ddiMgr);

  for (i=Ddi_VararrayNum(ddiMgr->cuddVariables); i<nvars; i++) {
    DdiVarNewFromCU(ddiMgr,Cudd_bddIthVar(ddiMgr->mgrCU,i));
    /* no variable group initially present */
    Ddi_VarsetarrayWrite(ddiMgr->varGroups,i,NULL);
  }

  DdiMgrCheckVararraySize(ddiMgr);

}


/**Function********************************************************************
  Synopsis     [Close a DdManager.]
  SideEffects  [none]
  SeeAlso      [Ddi_MgrInit
******************************************************************************/

void
Ddi_MgrQuit(
  Ddi_Mgr_t *dd  /* dd manager */
)
{
  int i;
  Ddi_Generic_t *next, *curr;

  Pdtutil_Free(dd->name);
  Ddi_Unlock(dd->one);
  Ddi_Free(dd->one);
  Ddi_Unlock(dd->zero);
  Ddi_Free(dd->zero);
  Pdtutil_Free (dd->settings.aig.satSolver);
  Pdtutil_Free (dd->settings.aig.itpStore);
  
  Pdtutil_Free (dd->cnf.cnf2aig);
  Pdtutil_Free (dd->cnf.cnfActive);
  Pdtutil_Free (dd->cnf.solver2cnf);

  if (dd->cnf.savedAigNodes != NULL) {
    bAig_Manager_t *bmgr = dd->aig.mgr;
    bAigArrayClearDeref(bmgr, dd->cnf.savedAigNodes);
    bAigArrayFree(dd->cnf.savedAigNodes);
  }

  if (dd->cnf.itpExtraGlobalNodes != NULL) {
    bAig_Manager_t *bmgr = dd->aig.mgr;
    bAigArrayClearDeref(bmgr, dd->cnf.itpExtraGlobalNodes);
    bAigArrayFree(dd->cnf.itpExtraGlobalNodes);
  }

  /* close and abstract handling */
  Ddi_AndAbsQuit(dd);

  // printf("DDI QUIT 1\n");

  /* close meta handling */
  Ddi_MetaQuit(dd);

#if 0
  /* debugging print */
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Variable Groups:.\n")
  );
  for (i=0; i<Ddi_VarsetarrayNum(dd->varGroups); i++){
    if (Ddi_VarsetarrayRead(dd->varGroups,i)!=NULL) {
      Ddi_VarsetPrint (Ddi_VarsetarrayRead(dd->varGroups,i), 3, NULL, stdout);
    }
  }
#endif

  /* free variable groups */
  Ddi_Unlock(dd->varGroups);
  Ddi_Free(dd->varGroups);

  //printf("DDI QUIT 2\n");

  /* free variables */
  /* free nodes: si possono includere le variabili */

  Ddi_Unlock(dd->cuddVariables);
  Ddi_Free(dd->cuddVariables);

  //printf("DDI QUIT 3\n");

  Ddi_Unlock(dd->baigVariables);
  Ddi_Free(dd->baigVariables);

  //printf("DDI QUIT 4\n");
  /* unlock variables so that they can be freed */
  for (i=0; i<Ddi_VararrayNum(dd->variables); i++) {
    //    Ddi_VarDetachName(Ddi_VararrayRead(dd->variables,i));
    Ddi_Unlock(Ddi_VararrayRead(dd->variables,i));
  }

  Ddi_Unlock(dd->variables);
  Ddi_Free(dd->variables);


  //printf("DDI QUIT 5\n");

  if (dd->varCntSortData.varSize > 0) {
    Pdtutil_Free(dd->varCntSortData.varCnt);
  }
  if (dd->varCntSortData.baigVarSize > 0) {
    Pdtutil_Free(dd->varCntSortData.baigVarCnt);
  }

  //printf("DDI QUIT 5\n");

  for (i=0; i<dd->vararraySize; i++) {
    if (dd->varnames[i] != NULL) {
      Pdtutil_Free(dd->varnames[i]);
    }
  }

  Pdtutil_Free(dd->varnames);
  st_free_table(dd->varNamesIdxHash);

  //printf("DDI QUIT 6\n");

  Pdtutil_Free(dd->varauxids);
  Pdtutil_Free(dd->varBaigIds);
  Pdtutil_Free(dd->varIdsFromCudd);

  DdiMgrGarbageCollect(dd);

  //printf("DDI QUIT 7\n");

  /* close aig handling */
  Ddi_AigQuit(dd);

  //printf("AIG QUIT DONE\n");

  Ddi_AbcStop();

  //printf("ABC QUIT DONE\n");

  Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "-- DDI Manager Quit.\n");
  );

  Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelUsrMax_c,
    if (Ddi_MgrCheckExtRef (dd,0)==0) {
      Ddi_MgrPrintExtRef(dd, 0);
      Ddi_MgrFreeExtRef(dd, 0);
      DdiMgrGarbageCollect(dd);
    }
  );

  if (Ddi_MgrConsistencyCheck(dd)==0) {
    Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelNone_c,
      fprintf(stderr, "Error found in manager check before Quit")
    );
  }
  
  //printf("CUDD QUIT\n");

#if 1
  Cudd_Quit(dd->mgrCU);
#endif

  MgrUnRegister(dd->mgrCU);

  Ddi_AbcStop();

  /* free free node list */

  curr = dd->freeNodeList;
  while (curr!=NULL) {
    next = curr->common.next;
    Pdtutil_Free(curr);
    curr = next;
  }

  Pdtutil_Free(dd);
}

/**Function********************************************************************

  Synopsis     [Creates a copy of a DdManager.]

  SideEffects  [none]

  SeeAlso      [Ddi_MgrQuit]

******************************************************************************/

Ddi_Mgr_t *
Ddi_MgrDup (
  Ddi_Mgr_t *dd            /* source dd manager */
  )
{
  Ddi_Mgr_t *dd2;
  char **varnames;
  int *varauxids;
  int *varBaigIds;
  int *varIdsFromCudd;
  st_generator *gen;
  st_table *varNamesIdxHash;
  char *key;
  int data;
  unsigned int nvar         = Ddi_MgrReadNumCuddVars(dd);
  unsigned int numSlots     = DDI_UNIQUE_SLOTS /*Ddi_MgrReadSlots(dd)*/;
  unsigned int cacheSize    = Ddi_MgrReadCacheSlots(dd);
  unsigned int maxCacheSize = DDI_MAX_CACHE_SIZE;

  Cudd_ReorderingType dynordMethod;
  int autodyn = Ddi_MgrReadReorderingStatus (dd, &dynordMethod);
  int i;

  dd2 = Ddi_MgrInit (NULL, NULL, 0/*nvar*/, numSlots, cacheSize, maxCacheSize,
    Ddi_MgrReadMemoryLimit (dd), Ddi_MgrReadTimeLimit (dd));

  if (dd2==NULL) {
    return (NULL);
  }

  if (dd->aig.mgr != NULL) {
    Ddi_AigInit(dd2,100);
  }

  DdiMgrCheckVararraySize(dd2);
  for (i=0; i<nvar; i++) {
    Ddi_Var_t *v2, *v = Ddi_VarFromCuddIndex (dd,i);
    char *name = Ddi_VarName(v);
    DdNode *vCU = Cudd_bddIthVar(dd2->mgrCU,i);
    DdiVarNewFromCU(dd2,vCU);
    v2 = Ddi_VarFromCuddIndex (dd2,i);
    if (name != NULL) {
      Ddi_VarAttachName (v2, name);
    }
  }

  dd2->settings = dd->settings;
  dd2->settings.aig.satSolver = NULL;
  Ddi_MgrSetOption(dd2,Pdt_DdiSatSolver_c,pchar,Ddi_MgrReadSatSolver(dd));


#if 0
  varnames = Ddi_MgrReadVarnames (dd);

  /* hash table duplication */
  varNamesIdxHash = st_init_table(strcmp, st_strhash);
  st_foreach_item(dd->varNamesIdxHash, gen, (char **) &key, (char *) &data) {
    st_insert(varNamesIdxHash, (char *) key, (char *) data);
  }
  st_free_table(dd2->varNamesIdxHash);
  dd2->varNamesIdxHash = varNamesIdxHash;

  varauxids = Ddi_MgrReadVarauxids (dd);
  varBaigIds = Ddi_MgrReadVarBaigIds (dd);
  varIdsFromCudd = Ddi_MgrReadVarIdsFromCudd (dd);

  dd2->varnames = Pdtutil_StrArrayDup (varnames, nvar);
  dd2->varBaigIds = Pdtutil_IntArrayDup (varBaigIds, nvar);
  dd2->varIdsFromCudd = Pdtutil_IntArrayDup (varIdsFromCudd, nvar);

#endif

  //  Ddi_MgrAlign (dd2, dd);

  if (0&&dd->varGroups != NULL) {
    for (i=0; i<Ddi_VarsetarrayNum(dd->varGroups); i++) {
      Ddi_Varset_t *group = Ddi_VarsetarrayRead(dd->varGroups,i);
      if (group != NULL) {
	printf("GROUP: %d - size %d\n", i, Ddi_VarsetNum(group));
      }
    }
  }

  if (autodyn) {
    Ddi_MgrSiftEnable (dd2, dynordMethod);
    Ddi_MgrSetSiftThresh (dd2,Ddi_MgrReadSiftThresh(dd));
  }

  if (dd->autodynSuspended) {
    Ddi_MgrSiftSuspend(dd2);
  }

  return (dd2);

}


/**Function********************************************************************
  Synopsis     [Reorders all DDs in a manager.]
  Description  [Reorders all DDs in a manager according to the input
    order. The input specification may be partial, i.e.
    it may include only a subset of variables.
    ]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

void
Ddi_MgrShuffle (
  Ddi_Mgr_t *dd      /* dd Manager to be Aligned */,
  int *sortedIds     /* Array of sorted ids */,
  int nids           /* Number of ids */
)
{
  int *newperm, *ids;
  int nv, i, j, result;

  nv = Ddi_MgrReadNumCuddVars(dd);

  Pdtutil_Assert (nids<=nv, "Input array for MgrShuffle is too large.");


  cuddGarbageCollect(Ddi_MgrReadMgrCU (dd),1);

  newperm = Pdtutil_Alloc(int,nv);
  ids = Pdtutil_Alloc(int,nv);

  for(i=0; i<nv; i++) {
    newperm[i] = Ddi_MgrReadInvPerm(dd,i);
  }

  for(i=0; i<nv; i++) {
    ids[i] = -1;
  }

  for(i=0; i<nids; i++) {
    Pdtutil_Assert(sortedIds[i]<nv,"imput id out of range in MgrShuffle");
    ids[sortedIds[i]] = 1;
  }

  for(i=0,j=0; i<nv; i++) {
    /* if variable is in sortedIds shuffle (pick up from sortedIds) */
    if (ids[newperm[i]] > 0) {
      newperm[i] = sortedIds[j++];
    }
  }

  Pdtutil_Assert (j==nids, "Not all input ids used for MgrShuffle.");

  Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "Partial Sorted ids: ");
    for(i=0; i<nids; i++) {
      fprintf(stdout, "%d ", sortedIds[i]);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "Full Sorted ids: ");
    for(i=0; i<nv; i++) {
      fprintf(stdout, "%d ", newperm[i]);
    }
    fprintf(stdout, "\n")
  );

  result = Cudd_ShuffleHeap (Ddi_MgrReadMgrCU (dd), newperm);

  Pdtutil_Warning (result==0, "ERROR during ShuffleHeap.");

  Pdtutil_Free (newperm);
  Pdtutil_Free (ids);

  return;
}


/**Function********************************************************************
  Synopsis     [Aligns the order of two managers.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

void
Ddi_MgrAlign (
  Ddi_Mgr_t *dd            /* dd manager to be aligned */,
  Ddi_Mgr_t *ddRef         /* reference dd manager */
)
{
  int *newperm;
  int nv, nvRef, i, j, idRef, result=1;

  nv = Ddi_MgrReadNumCuddVars(dd);
  nvRef = Ddi_MgrReadNumCuddVars(ddRef);

  newperm = Pdtutil_Alloc(int,nv);

  for(i=0,j=0; i<nvRef; i++) {
    idRef = Ddi_MgrReadInvPerm(ddRef,i);
    if (idRef < nv) {
      /* variable has a corresponding one in dd */
      newperm[j++] = idRef;
    }
  }

  Pdtutil_VerbosityMgr(dd, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "Align ids: ");
    for(i=0; i<nv; i++) {
      fprintf(stdout, "%d ", newperm[i]);
    }
    fprintf(stdout, "\n")
  );

  Pdtutil_Assert(j==nv,"missing variables when aligning managers");

  result = Cudd_ShuffleHeap(Ddi_MgrReadMgrCU(dd),newperm);

  if (!result) {
    fprintf(stderr, "Error: During ShuffleHeap.\n");
  }

  Pdtutil_Free(newperm);

  return;
}


/**Function********************************************************************
  Synopsis    [Create groups of 2 variables ]
  Description [Create groups of 2 variables: variables of corresponding
               indexes in vfix and vmov are coupled. If vmov[i] and vfix[i]
               are not adjacent in the ordering, vmov[i] is moved to the
               position below vfix[i].
              ]
  SideEffects [none]
  SeeAlso     []
******************************************************************************/

void
Ddi_MgrCreateGroups2(
  Ddi_Mgr_t *dd              /* manager */,
  Ddi_Vararray_t *vfix       /* first array */,
  Ddi_Vararray_t *vmov       /* first array */
)
{
  int i, j, fixpos, movpos, nvars, nvarstot;
  int *tmpIds, *sortedIds, *grpIds;
  Ddi_Var_t *fv, *mv;
  int do_shuffle = 0;

  if ((vfix == NULL)||(vmov == NULL))
    return;

  Pdtutil_Assert(Ddi_VararrayNum(vfix)==Ddi_VararrayNum(vmov),
    "Variable arrays with different size in grouping");

  nvars = Ddi_VararrayNum(vfix);
  nvarstot = Ddi_MgrReadNumCuddVars(dd);

  /* moving vmov */

  tmpIds = Pdtutil_Alloc(int,nvarstot);
  grpIds = Pdtutil_Alloc(int,nvarstot);
  sortedIds = Pdtutil_Alloc(int,nvarstot);

  /* creating interleaved ordering for group variables */

  /* initialize tmpIds with present ordering */
  for (i=0,j=0; i<nvarstot; i++) {
    tmpIds[i] = Ddi_MgrReadInvPerm(dd,i);
    grpIds[i] = -1;
  }

  for (i=0,j=0; i<nvars; i++) {
    /* obtain variable positions and ids */
    fv = Ddi_VararrayRead(vfix,i);
    mv = Ddi_VararrayRead(vmov,i);
    fixpos = Ddi_VarCurrPos(fv);
    movpos = Ddi_VarCurrPos(mv);

    if (abs(fixpos-movpos)!=1) /* check enabling shuffle */
      do_shuffle = 1;

    /* clear tmpIds, leaving variables outside groups */
    Pdtutil_Assert(tmpIds[fixpos]==Ddi_VarCuddIndex(fv),
      "id mismatch creating groups");
    Pdtutil_Assert(tmpIds[movpos]==Ddi_VarCuddIndex(mv),
       "id mismatch creating groups");
    tmpIds[fixpos] = -1;
    tmpIds[movpos] = -1;

    /* collect fix variable array ids */
    grpIds[fixpos] = i;
  }

  if (do_shuffle) {
    /*
     * now group variables may be retrieved from grpIds, non group
     * variables from tmpIds. Do merging
     */
    for (i=0,j=0; i<nvarstot; i++) {
      if (tmpIds[i] >= 0) {
        /* non group variable: keep as it is in new ordering */
        Pdtutil_Assert(grpIds[i]==-1,
          "a variable appears to be both group and no group");
        sortedIds[j++] = tmpIds[i];
      }
      else if (grpIds[i]>=0) { /* skip old positions of vmov variables */
        /* group variable: put a group (2 vars) in final ordering */
        Pdtutil_Assert(grpIds[i]<nvars,
          "invalid array index while grouping variables");
        fv = Ddi_VararrayRead(vfix,grpIds[i]);
        mv = Ddi_VararrayRead(vmov,grpIds[i]);
        fixpos = Ddi_VarCurrPos(fv);
        movpos = Ddi_VarCurrPos(mv);
      /* keep relative ordering within group */
        if (fixpos<movpos) {
          sortedIds[j++] = Ddi_VarCuddIndex(fv);
          sortedIds[j++] = Ddi_VarCuddIndex(mv);
        }
        else {
          sortedIds[j++] = Ddi_VarCuddIndex(mv);
          sortedIds[j++] = Ddi_VarCuddIndex(fv);
        }
      }
    } /* end for */

    Pdtutil_Assert(j==nvarstot,"sorted id array not completed while grouping");

    /* do variable shuffle */
    Ddi_MgrShuffle (dd, sortedIds, nvarstot);

  }

  for (i=0,j=0; i<nvars; i++) {
    /* obtain variable positions and ids */
    fv = Ddi_VararrayRead(vfix,i);
    mv = Ddi_VararrayRead(vmov,i);
    fixpos = Ddi_VarCurrPos(fv);
    movpos = Ddi_VarCurrPos(mv);

    Pdtutil_Assert(abs(fixpos-movpos)==1,"grouping non adjacent variables");
#if !DEVEL_OPT_ANDEXIST
    if (fixpos<movpos) {
      DdiMgrMakeVarGroup(dd,fv,2,MTR_DEFAULT);
    } else {
      DdiMgrMakeVarGroup(dd,mv,2,MTR_DEFAULT);
    }
#else
    {
      Ddi_Var_t *newv;
      int id;

      if (fixpos<movpos) {
        newv = Ddi_VarNewBeforeVar(fv);
        DdiMgrMakeVarGroup(dd,newv,3,MTR_FIXED);
      } else {
        newv = Ddi_VarNewBeforeVar(mv);
        DdiMgrMakeVarGroup(dd,newv,3,MTR_FIXED);
      }
      if (Cuplus_AuxVarset == NULL) {
        Cuplus_AuxVarset = Ddi_VarsetVoid(dd);
      }
      Ddi_VarsetAddAcc(Cuplus_AuxVarset,newv);
      id = Ddi_VarCuddIndex(fv);
      Cuplus_AuxVars[id] = Ddi_VarToCU(newv);
#if 0
      id = Ddi_VarCuddIndex(mv);
      Cuplus_AuxVars[id] = Ddi_VarToCU(newv);
#endif
      Cuplus_IsAux[Ddi_VarCuddIndex(newv)]=1;
    }
#endif
  }

  Pdtutil_Free(tmpIds);
  Pdtutil_Free(grpIds);
  Pdtutil_Free(sortedIds);

  return;
}

/**Function********************************************************************

  Synopsis    [Stores the variable ordering]

  Description [This function stores the variable ordering of a dd manager.
    Variable names and aux ids are used.
    ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int
Ddi_MgrOrdWrite (
  Ddi_Mgr_t *dd                                 /* Decision Diagram Manager */,
  char *filename                                /* File Name */,
  FILE *fp                                      /* Pointer to Store File */,
  Pdtutil_VariableOrderFormat_e ordFileFormat   /* File Format */
  )
{
  char **vnames = Ddi_MgrReadVarnames (dd);
  int *vauxids = Ddi_MgrReadVarauxids (dd);
  int *iperm, nvars, i;

  nvars = Ddi_MgrReadNumCuddVars (dd);
  iperm = Pdtutil_Alloc (int, nvars);
  for (i=0; i<nvars; i++) {
    iperm[i] = Ddi_MgrReadInvPerm (dd, i);
  }

  Pdtutil_OrdWrite (vnames, vauxids, iperm, nvars, filename, fp,
    ordFileFormat);

  Pdtutil_Free(iperm);

  return (0);
}

/**Function********************************************************************

  Synopsis    [Reads the variable ordering]

  Description [This function reads the variable ordering of a dd manager.
    Existing variables with names in the ordering are shuffled
    to match the ordering.
    ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int
Ddi_MgrReadOrdNamesAuxids (
  Ddi_Mgr_t *dd                                /* Manager */,
  char *filename                               /* File Name */,
  FILE *fp                                     /* Pointer to the Store File */,
  Pdtutil_VariableOrderFormat_e ordFileFormat  /* File Format */
  )
{
  int nvars, i, id, *sortedIds;
  Ddi_Var_t *var;
  char **vnames;
  int *vauxids;

  nvars = Pdtutil_OrdRead (&vnames, &vauxids, filename, fp, ordFileFormat);

  if (nvars <= 0) {
    return (nvars);
  }

  sortedIds = Pdtutil_Alloc (int, nvars);
  Pdtutil_Assert(vnames!=NULL,"not yet supported read ord without varnames");
  if (vnames != NULL) {
    for (i=0; i<nvars; i++) {
      Pdtutil_Assert(vnames[i] != NULL, "NULL varname in OrdRead");
      id = Ddi_VarCuddIndex(Ddi_VarFromName(dd,vnames[i]));
      if (id < 0) { /* create a new var */
        var = Ddi_VarNew(dd);
        Ddi_VarAttachName(var,vnames[i]);
        if (vauxids != NULL)
          Ddi_VarAttachAuxid(var,vauxids[i]);
        id = Ddi_VarCuddIndex(var);
      }
      sortedIds[i] = id;
    }
  }

  Ddi_MgrShuffle (dd, sortedIds, nvars);

  Pdtutil_Free(sortedIds);

  return nvars;
}

/**Function********************************************************************
  Synopsis     [Enable dynamic reordering.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrSiftEnable(
  Ddi_Mgr_t *dd       /* dd manager */,
  int        method
)
{
  dd->autodynEnabled = 1;
  Cudd_AutodynEnable(Ddi_MgrReadMgrCU(dd),(Cudd_ReorderingType)method);
}

/**Function********************************************************************
  Synopsis     [Enable dynamic reordering.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrSiftDisable(
  Ddi_Mgr_t *dd       /* dd manager */
)
{
  dd->autodynEnabled = 0;
  Cudd_AutodynDisable(Ddi_MgrReadMgrCU(dd));
}


/**Function********************************************************************
  Synopsis     [Suspend autodyn if active.]
  SideEffects  [none]
  SeeAlso      [Ddi_MgrInit]
******************************************************************************/

void
Ddi_MgrSiftSuspend(
  Ddi_Mgr_t *dd  /* dd manager */
)
{
  Cudd_ReorderingType dynordMethod;
  int autodyn = Ddi_MgrReadReorderingStatus (dd, &dynordMethod);

  /* increment suspend counter so that nested suspend are supported */
  dd->autodynNestedSuspend++;

  /* actually suspend only if enabled */
  if (dd->autodynEnabled && !dd->autodynSuspended) {
    Ddi_MgrSiftDisable (dd);
    dd->autodynEnabled = 1;
    dd->autodynMethod = dynordMethod; /* suspended autodyn method */
    dd->autodynSuspended = 1;
  }

}

/**Function********************************************************************
  Synopsis     [Suspend abort on sift if active.]
  SideEffects  [none]
  SeeAlso      [Ddi_MgrInit]
******************************************************************************/

void
Ddi_MgrAbortOnSiftSuspend(
  Ddi_Mgr_t *dd  /* dd manager */
)
{

  if (dd->abortOnSift) {
    dd->abortOnSiftMasked = 1;
    Cudd_AutodynDisable(Ddi_MgrReadMgrCU(dd));
  };

}


/**Function********************************************************************
  Synopsis     [Resume autodyn if suspended.]
  SideEffects  [none]
  SeeAlso      [Ddi_MgrInit]
******************************************************************************/
void
Ddi_MgrSiftResume(
  Ddi_Mgr_t *dd  /* dd manager */
  )
{
  Pdtutil_Assert(dd->autodynNestedSuspend>0,
    "Unbalanced dynord suspend/resume");
  if (--dd->autodynNestedSuspend == 0) {
     /* enable only if all nested suspend closed */
    if (dd->autodynSuspended) {
      Ddi_MgrSiftEnable (dd, dd->autodynMethod);
      dd->autodynSuspended = 0;
    }
  }
  return;
}


/**Function********************************************************************
  Synopsis     [Resume abort on sift if suspended.]
  SideEffects  [none]
  SeeAlso      [Ddi_MgrInit]
******************************************************************************/
void
Ddi_MgrAbortOnSiftResume(
  Ddi_Mgr_t *dd  /* dd manager */
  )
{
  Cudd_ReorderingType dynordMethod;
  int autodyn = Ddi_MgrReadReorderingStatus (dd, &dynordMethod);

  if (dd->abortOnSiftMasked) {
    dd->abortOnSiftMasked = 0;
    Cudd_AutodynEnable(Ddi_MgrReadMgrCU(dd),(Cudd_ReorderingType)autodyn);
  }
}


/**Function********************************************************************
  Synopsis     [Enable Abort on sift.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrAbortOnSiftEnable(
  Ddi_Mgr_t *dd  /* dd manager */
)
{
  Ddi_MgrAbortOnSiftEnableWithThresh(
  dd,dd->autodynNextDyn-Ddi_MgrReadLiveNodes(dd)-1,0);
}

/**Function********************************************************************
  Synopsis     [Enable Abort on sift.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrAbortOnSiftEnableWithThresh(
  Ddi_Mgr_t *dd  /* dd manager */,
  int deltaNodes /* allowed increment of manager nodes */,
  int allowSift
)
{
  int abortThresh;
  Cudd_ReorderingType dynordMethod;
  int siftThresh = 0;

  Pdtutil_Assert(dd->abortOnSift>=0,"wrong manager Abort on sift flag");
  Pdtutil_Assert(dd->abortOnSift<DDI_MAX_NESTED_ABORT_ON_SIFT,
   "too many nested abort. Increase DDI_MAX_NESTED_ABORT_ON_SIFT in ddiInt.h");

  if (!dd->autodynEnabled) {
    Ddi_MgrSiftEnable (dd, DDI_REORDER_SIFT);
    dd->autodynEnabled = 0;
  }
  else {
    siftThresh = dd->autodynNextDyn;
  }
  if (dd->autodynSuspended) {
    Cudd_AutodynEnable(Ddi_MgrReadMgrCU(dd),dynordMethod);
  }

  abortThresh = dd->abortOnSiftThresh[dd->abortOnSift]
              = Ddi_MgrReadLiveNodes(dd)+deltaNodes;

  if (0&&abortThresh < ((siftThresh*2)/3)) {
    abortThresh *=2;
    if (abortThresh > ((siftThresh*2)/3)) {
      abortThresh = (siftThresh*2)/3;
    }
  }

  dd->abortOnSift++;
  dd->abortedOp=0;

  dd->autodynMasked = !allowSift;

  if (!allowSift || dd->autodynSuspended ||
     (abortThresh < Cudd_ReadNextReordering(Ddi_MgrReadMgrCU(dd)))) {
    Cudd_SetNextReordering(Ddi_MgrReadMgrCU(dd), abortThresh);
  }
}

/**Function********************************************************************
  Synopsis     [Disable Abort on sift.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrAbortOnSiftDisable(
  Ddi_Mgr_t *dd  /* dd manager */
)
{
  Pdtutil_Assert(dd->abortOnSift>=1,"Abort on sift already disabled");
  dd->abortOnSift--;
  /*dd->abortedOp=0;*/

  dd->autodynMasked = 0;

  if (dd->abortOnSift==0) {
    if (dd->autodynEnabled) {
      Cudd_SetNextReordering(Ddi_MgrReadMgrCU(dd), dd->autodynNextDyn);
      if (dd->autodynSuspended) {
        Cudd_AutodynDisable(Ddi_MgrReadMgrCU(dd));
      }
    }
    else {
      Ddi_MgrSiftDisable(dd);
    }
  }
  else {
    int nextSift = dd->abortOnSiftThresh[dd->abortOnSift];
    if (dd->autodynEnabled && !dd->autodynSuspended &&
        (dd->autodynNextDyn < nextSift)) {
      nextSift = dd->autodynNextDyn;
    }
    Cudd_SetNextReordering(Ddi_MgrReadMgrCU(dd), nextSift);
  }

}

/**Function********************************************************************

  Synopsis           []

  Description        []

  SideEffects        [To be congruent operationFlag should be a Pdtutil_MgrOp_t
   type, and returnFlag of Pdtutil_MgrRet_t type.]

  SeeAlso            []

******************************************************************************/

int
Ddi_MgrOperation (
  Ddi_Mgr_t **ddiMgrP                 /* DD Manager Pointer */,
  char *string                       /* String */,
  Pdtutil_MgrOp_t operationFlag      /* Operation Flag */,
  void **voidPointer                 /* Generic Pointer */,
  Pdtutil_MgrRet_t *returnFlagP      /* Type of the Pointer Returned */
  )
{
  char *stringFirst, *stringSecond;

  /*------------ Check for the Correctness of the Command Sequence ----------*/

  if (*ddiMgrP == NULL) {
    Pdtutil_Warning (1, "Command Out of Sequence.");
    return (1);
  }

  /*--------------------------- Print All Options ---------------------------*/

  /* Nothing for Now */

  /*----------------------- Take Main and Secondary Name --------------------*/

  Pdtutil_ReadSubstring (string, &stringFirst, &stringSecond);

  /*----------------------- Operation on the DD Manager ---------------------*/

  if (stringFirst == NULL) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "DecisionDiagramManager Verbosity %s (%d).\n",
            Pdtutil_VerbosityEnum2String (Ddi_MgrReadVerbosity (*ddiMgrP)),
            Ddi_MgrReadVerbosity (*ddiMgrP))
        );
        break;
      case Pdtutil_MgrOpStats_c:
        Ddi_MgrPrintStats (*ddiMgrP);
        break;
      case Pdtutil_MgrOpMgrRead_c:
        *voidPointer = (void *) *ddiMgrP;
        *returnFlagP = Pdtutil_MgrRetDdMgr_c;
        break;
      case Pdtutil_MgrOpMgrSet_c:
        *ddiMgrP = (Ddi_Mgr_t *) *voidPointer;
        break;
      case Pdtutil_MgrOpMgrDelete_c:
        Ddi_MgrQuit (*ddiMgrP);
        *returnFlagP = Pdtutil_MgrRetNone_c;
        break;
      default:
        Pdtutil_Warning (1, "Operation Non Allowed on DDI Manager");
        break;
     }

    return (0);
  }

  /*-------------------------------- Options --------------------------------*/

  if (strcmp(stringFirst, "verbosity")==0) {
    switch (operationFlag) {
      case Pdtutil_MgrOpOptionSet_c:
        Ddi_MgrSetOption(*ddiMgrP,
			Pdt_DdiVerbosity_c,inum,Pdtutil_VerbosityString2Enum (*(char **)voidPointer));
        break;
      case Pdtutil_MgrOpOptionShow_c:
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Verbosity %s (%d).\n",
            Pdtutil_VerbosityEnum2String (Ddi_MgrReadVerbosity (*ddiMgrP)),
            Ddi_MgrReadVerbosity (*ddiMgrP))
        );
        break;
      default:
        Pdtutil_Warning (1, "Operation Non Allowed on TRAV Manager");
        break;
    }

    Pdtutil_Free (stringFirst);
    Pdtutil_Free (stringSecond);
    return (0);
  }

  /*------------------------------- No Match --------------------------------*/

  Pdtutil_Warning (1, "Operation on DDI manager not allowed");
  return (1);
}

/**Function********************************************************************
  Synopsis     [Prints on standard outputs statistics on a DD manager]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

void
Ddi_MgrPrintStats (
  Ddi_Mgr_t *dd            /* source dd manager */
  )
{
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    Pdtutil_ChrPrint (stdout, '*', 50);
    fprintf(stdout, "Decision Diagram Statistics Summary:.\n");
    Cudd_PrintInfo (dd->mgrCU, stdout);
    Pdtutil_ChrPrint (stdout, '*', 50);
    fprintf(stdout, "\n")
  );

  return;
}


/**Function********************************************************************
  Synopsis     [Reads the Cudd Manager]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
DdManager *
Ddi_MgrReadMgrCU(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return (dd->mgrCU);
}

/**Function********************************************************************
  Synopsis     [Reads run vars]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int Ddi_MgrReadNumVars(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return (Ddi_VararrayNum((dd)->variables));
}


/**Function********************************************************************
  Synopsis     [Reads the Cudd Manager]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
bAig_Manager_t *

Ddi_MgrReadMgrBaig(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return (dd->aig.mgr);
}

/**Function********************************************************************
  Synopsis     [Reads one constant]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
Ddi_Bdd_t *
Ddi_MgrReadOne(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return dd->one;
}

/**Function********************************************************************
  Synopsis     [Reads zero constant]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
Ddi_Bdd_t *
Ddi_MgrReadZero(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return dd->zero;
}

/**Function********************************************************************
  Synopsis     [Read current node id field]
  Description  [Read current node id field. DDI nodes are identified by this
                id, which is incremented at any new node creation.]
  SideEffects  [none]
  SeeAlso      [Ddi_MgrSetTracedId]
******************************************************************************/
int
Ddi_MgrReadCurrNodeId(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return(dd->currNodeId);
}

/**Function********************************************************************
  Synopsis     [Set traced node id field]
  Description  [Set traced node id field. Creation of node >= id are logged
                using DdiTraceNodeAlloc.
                This is expecially useful for debugging BDD leaks and memory
                bugs. To watch generation of node with given ID, put a
                breakpoint on DdiTraceNodeAlloc after setting manager
                tracedId to ID (Ddi_MgrSetTracedId(dd,ID)).]
  SideEffects  [none]
  SeeAlso      [Ddi_MgrReadCurrNodeId]
******************************************************************************/
void
Ddi_MgrSetTracedId(
  Ddi_Mgr_t *dd            /* source dd manager */,
  int id
)
{
  dd->tracedId = id;
}

/**Function********************************************************************
  Synopsis     [Set part clustering threshold]
  Description  [Set part clustering threshold. This is the threshold used for
                quantification operators on partitioned BDDs.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrSetExistClustThresh(
  Ddi_Mgr_t *dd            /* source dd manager */,
  int th
)
{
  dd->settings.part.existClustThresh = th;
}

/**Function********************************************************************
  Synopsis     [Read part clustering threshold]
  Description  [Read part clustering threshold. This is the threshold used for
                quantification operators on partitioned BDDs.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrReadExistClustThresh(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return(dd->settings.part.existClustThresh);
}


/**Function********************************************************************
  Synopsis     [Set part clustering threshold]
  Description  [Set part clustering threshold. This is the threshold used for
                quantification operators on partitioned BDDs.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrSetExistOptClustThresh(
  Ddi_Mgr_t *dd            /* source dd manager */,
  int th
)
{
  dd->settings.part.existOptClustThresh = th;
}

/**Function********************************************************************
  Synopsis     [Read part clustering threshold]
  Description  [Read part clustering threshold. This is the threshold used for
                quantification operators on partitioned BDDs.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrReadExistOptClustThresh(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return(dd->settings.part.existOptClustThresh);
}


/**Function********************************************************************
  Synopsis     [Set multiway exist optimization level]
  Description  []
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrSetExistOptLevel(
  Ddi_Mgr_t *dd            /* source dd manager */,
  int lev
)
{
  dd->settings.part.existOptLevel = lev;
}

/**Function********************************************************************
  Synopsis     [Set multiway exist disj Part threshold]
  Description  []
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrSetExistDisjPartTh(
  Ddi_Mgr_t *dd            /* source dd manager */,
  int th
)
{
  dd->settings.part.existDisjPartTh = th;
}

/**Function********************************************************************
  Synopsis     [Read exist optimization level]
  Description  []
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrReadExistOptLevel(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return(dd->settings.part.existOptLevel);
}

/**Function********************************************************************
  Synopsis     [Read exist optimization level]
  Description  []
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrReadExistDisjPartTh(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return(dd->settings.part.existDisjPartTh);
}

/**Function********************************************************************
  Synopsis     [Reads the variable names]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
char**
Ddi_MgrReadVarnames(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  int i;
  int nvars = Ddi_MgrReadNumVars(dd);

  if (dd->vararraySize<nvars) {
    dd->vararraySize *= 2;
    dd->varnames = Pdtutil_Realloc(char *, dd->varnames, dd->vararraySize);
    dd->varauxids = Pdtutil_Realloc(int, dd->varauxids, dd->vararraySize);
    dd->varBaigIds = Pdtutil_Realloc(int, dd->varBaigIds, dd->vararraySize);
    dd->varIdsFromCudd = Pdtutil_Realloc(int, dd->varIdsFromCudd, dd->vararraySize);
    for (i = dd->vararraySize/2; i<dd->vararraySize; i++) {
      dd->varnames[i] = NULL;
      dd->varauxids[i] = -1;
      dd->varBaigIds[i] = bAig_NULL;
      dd->varIdsFromCudd[i] = -1;
    }
  }

  return dd->varnames;
}

/**Function********************************************************************
  Synopsis     [Reads the variable auxiliary ids]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int *
Ddi_MgrReadVarauxids(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  int i;
  int nvars = Ddi_MgrReadNumVars(dd);

  if (dd->vararraySize<nvars) {
    dd->vararraySize *= 2;
    dd->varnames = Pdtutil_Realloc(char *, dd->varnames, dd->vararraySize);
    dd->varauxids = Pdtutil_Realloc(int, dd->varauxids, dd->vararraySize);
    for (i = dd->vararraySize/2; i<dd->vararraySize; i++) {
      dd->varnames[i] = NULL;
      dd->varauxids[i] = -1;
    }
  }

  return dd->varauxids;
}

/**Function********************************************************************
  Synopsis     [Reads the variable auxiliary ids]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
bAigEdge_t *
Ddi_MgrReadVarBaigIds(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  int i;
  int nvars = Ddi_MgrReadNumVars(dd);

  if (dd->vararraySize<nvars) {
    dd->vararraySize *= 2;
    dd->varnames = Pdtutil_Realloc(char *, dd->varnames, dd->vararraySize);
    dd->varauxids = Pdtutil_Realloc(int, dd->varauxids, dd->vararraySize);
    dd->varBaigIds = Pdtutil_Realloc(bAigEdge_t, dd->varBaigIds, dd->vararraySize);
    for (i = dd->vararraySize/2; i<dd->vararraySize; i++) {
      dd->varnames[i] = NULL;
      dd->varauxids[i] = -1;
      dd->varBaigIds[i] = bAig_NULL;
    }
  }

  return dd->varBaigIds;
}

/**Function********************************************************************
  Synopsis     [Reads the variable auxiliary ids]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int *
Ddi_MgrReadVarIdsFromCudd(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  int i;
  int nvars = Ddi_MgrReadNumVars(dd);

  if (dd->vararraySize<nvars) {
    dd->vararraySize *= 2;
    dd->varnames = Pdtutil_Realloc(char *, dd->varnames, dd->vararraySize);
    dd->varauxids = Pdtutil_Realloc(int, dd->varauxids, dd->vararraySize);
    dd->varBaigIds = Pdtutil_Realloc(bAigEdge_t, dd->varBaigIds, dd->vararraySize);
    dd->varIdsFromCudd = Pdtutil_Realloc(bAigEdge_t, dd->varIdsFromCudd, dd->vararraySize);
    for (i = dd->vararraySize/2; i<dd->vararraySize; i++) {
      dd->varnames[i] = NULL;
      dd->varauxids[i] = -1;
      dd->varBaigIds[i] = bAig_NULL;
      dd->varIdsFromCudd[i] = -1;
    }
  }

  return dd->varIdsFromCudd;
}

/**Function********************************************************************
  Synopsis     [Read the counter of external references]
  Description  [Read the number of externally referenced DDI handles.
    This is the numer of generic nodes (allocated - freed), diminished by
    the number of locked nodes + 3 (2 constants + variable array).]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrReadExtRef(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return (dd->genericNum-dd->lockedNum);
}

/**Function********************************************************************
  Synopsis     [Read the counter of external references to BDDs]
  Description  [Read the number of externally referenced BDD handles.
    This is the number of allocated - freed, diminished by
    the number of locked nodes.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrReadExtBddRef(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return (dd->typeNum[Ddi_Bdd_c]-dd->typeLockedNum[Ddi_Bdd_c]);
}

/**Function********************************************************************
  Synopsis     [Read the counter of external references to BDD arrays]
  Description  [Read the number of externally referenced BDD array handles.
    This is the number of allocated - freed, diminished by
    the number of locked nodes.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrReadExtBddarrayRef(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return (dd->typeNum[Ddi_Bddarray_c]-dd->typeLockedNum[Ddi_Bddarray_c]);
}

/**Function********************************************************************
  Synopsis     [Read the counter of external references to varsets]
  Description  [Read the number of externally referenced varset handles.
    This is the number of allocated - freed, diminished by
    the number of locked nodes.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrReadExtVarsetRef(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return (dd->typeNum[Ddi_Varset_c]-dd->typeLockedNum[Ddi_Varset_c]);
}


/**Function********************************************************************
  Synopsis     [Returns the threshold for the next dynamic reordering.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrReadSiftThresh(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return Cudd_ReadNextReordering(Ddi_MgrReadMgrCU(dd));
}

/**Function********************************************************************
  Synopsis     [Returns the threshold for the next dynamic reordering.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrReadSiftMaxThresh(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return dd->autodynMaxTh;
}

/**Function********************************************************************
  Synopsis     [Returns the threshold for the next dynamic reordering.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrReadSiftMaxCalls(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return dd->autodynMaxCalls;
}

/**Function********************************************************************
  Synopsis     [Returns the threshold for the next dynamic reordering.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

char *
Ddi_MgrReadName (
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return (dd->name);
}


/**Function********************************************************************
  Synopsis     [Returns the threshold for the next dynamic reordering.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

void
Ddi_MgrSetName (
  Ddi_Mgr_t *dd            /* source dd manager */,
  char *ddiName
)
{
  dd->name = Pdtutil_StrDup (ddiName);
}


/**Function********************************************************************
  Synopsis     [Read verbosity]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

Pdtutil_VerbLevel_e
Ddi_MgrReadVerbosity(
  Ddi_Mgr_t *ddiMgr         /* Decision Diagram Interface Manager */
)
{
  return (ddiMgr->settings.verbosity);
}


/**Function********************************************************************
  Synopsis     [Read verbosity]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

FILE *
Ddi_MgrReadStdout(
  Ddi_Mgr_t *ddiMgr         /* Decision Diagram Interface Manager */
)
{
  return (ddiMgr->settings.stdout);
}

/**Function********************************************************************
  Synopsis     [Set verbosity]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

void
Ddi_MgrSetVerbosity(
  Ddi_Mgr_t *ddiMgr              /* Decision Diagram Interface Manager */,
  Pdtutil_VerbLevel_e verbosity   /* Verbosity Level */
)
{
  ddiMgr->settings.verbosity = verbosity;

  return;
}

/**Function********************************************************************
  Synopsis     [Set verbosity]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/

void
Ddi_MgrSetStdout(
  Ddi_Mgr_t *ddiMgr              /* Decision Diagram Interface Manager */,
  FILE *stdout                   /* Verbosity Level */
)
{
  ddiMgr->settings.stdout = stdout;

  return;
}

/**Function********************************************************************
  Synopsis     [Read peak product local]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrReadPeakProdLocal(
  Ddi_Mgr_t *ddiMgr         /* Decision Diagram Interface Manager */
)
{
  return (ddiMgr->stats.peakProdLocal);
}


/**Function********************************************************************
  Synopsis     [Read peak product local]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
long
Ddi_MgrReadSiftLastTime(
  Ddi_Mgr_t *ddiMgr         /* Decision Diagram Interface Manager */
)
{
  return (ddiMgr->stats.siftLastTime);
}

/**Function********************************************************************
  Synopsis     [Read peak product local]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
long
Ddi_MgrReadSiftTotTime(
  Ddi_Mgr_t *ddiMgr         /* Decision Diagram Interface Manager */
)
{
  return (ddiMgr->stats.siftTotTime);
}

/**Function********************************************************************
  Synopsis     [Read peak product local]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrReadSiftRunNum(
  Ddi_Mgr_t *ddiMgr         /* Decision Diagram Interface Manager */
)
{
  return (ddiMgr->stats.siftRunNum);
}

/**Function********************************************************************
  Synopsis     [Read peak product global]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrReadPeakProdGlobal(
  Ddi_Mgr_t *ddiMgr         /* Decision Diagram Interface Manager */
)
{
  return (ddiMgr->stats.peakProdGlobal);
}

/**Function********************************************************************
  Synopsis     [Reset peak product local]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrPeakProdLocalReset(
  Ddi_Mgr_t *ddiMgr              /* Decision Diagram Interface Manager */
)
{
  ddiMgr->stats.peakProdLocal = 0;
}

/**Function********************************************************************
  Synopsis     [Update peak product stats]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrPeakProdUpdate(
  Ddi_Mgr_t *ddiMgr              /* Decision Diagram Interface Manager */,
  int size
)
{
   if (size > ddiMgr->stats.peakProdLocal) {
     ddiMgr->stats.peakProdLocal = size;
     if (size > ddiMgr->stats.peakProdGlobal) {
       ddiMgr->stats.peakProdGlobal = size;
     }
   }
}

/**Function********************************************************************
  Synopsis     [Sets the CUDD manager]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrSetMgrCU(
  Ddi_Mgr_t *dd            /* source dd manager */,
  DdManager *m             /* CUDD manager */
)
{
  dd->mgrCU=m;
  return;
}

/**Function********************************************************************
  Synopsis     [Sets the one constant]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrSetOne(
  Ddi_Mgr_t *dd            /* source dd manager */,
  Ddi_Bdd_t   *one          /* one constant */
)
{
  dd->one = one;
  return;
}

/**Function********************************************************************
  Synopsis     [Sets the zero constant]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrSetZero(
  Ddi_Mgr_t *dd            /* source dd manager */,
  Ddi_Bdd_t   *zero          /* zero constant */
)
{
  dd->zero = zero;
  return;
}

/**Function********************************************************************
  Synopsis     [Sets the names of variables]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrSetVarnames(
  Ddi_Mgr_t *dd            /* source dd manager */,
  char      **vn           /* names of variables */
)
{
  fprintf(stderr, "Error: Function setVarnames has to be Canceled.\n");
  dd->varnames=vn;
  /* Be careful with the hash table! dd->varNamesIdxHash  */
  return;
}

/**Function********************************************************************
  Synopsis     [Sets the auxiliary variable ids]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrSetVarauxids(
  Ddi_Mgr_t *dd            /* source dd manager */,
  int       *va            /* auxiliary variable ids */
)
{
  dd->varauxids=va;
  return;
}

/**Function********************************************************************
  Synopsis     [Sets baig variable ids]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrSetVarBaigIds(
  Ddi_Mgr_t *dd            /* source dd manager */,
  bAigEdge_t *va            /* auxiliary variable ids */
)
{
  dd->varBaigIds=va;
}

/**Function********************************************************************
  Synopsis     [Sets baig variable ids]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrSetVarIdsFromCudd(
  Ddi_Mgr_t *dd            /* source dd manager */,
  bAigEdge_t *va            /* cudd variable ids */
)
{
  dd->varIdsFromCudd=va;
}

/**Function********************************************************************
  Synopsis     [Returns the threshold for the next dynamic reordering.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrSetSiftThresh(
  Ddi_Mgr_t *dd            /* source dd manager */,
  int th                   /* threshold */
)
{
  Cudd_SetNextReordering(Ddi_MgrReadMgrCU(dd), th);
  dd->autodynNextDyn = th;
  return;
}

/**Function********************************************************************
  Synopsis     [Returns the threshold for the next dynamic reordering.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrSetSiftMaxThresh(
  Ddi_Mgr_t *dd            /* source dd manager */,
  int th                   /* threshold */
)
{
  dd->autodynMaxTh = th;
  return;
}

/**Function********************************************************************
  Synopsis     [Returns the threshold for the next dynamic reordering.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
void
Ddi_MgrSetSiftMaxCalls(
  Ddi_Mgr_t *dd            /* source dd manager */,
  int n                    /* num */
)
{
  dd->autodynMaxCalls = n;
  return;
}

/*
*********************** Computed Table (Cache) ********************************
*/

/**Function********************************************************************
  Synopsis     [Reads the number of slots in the cache.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
unsigned int
Ddi_MgrReadCacheSlots(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return Cudd_ReadCacheSlots(Ddi_MgrReadMgrCU(dd));
}

/**Function********************************************************************
  Synopsis     [Returns the number of cache look-ups.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
double
Ddi_MgrReadCacheLookUps(
  Ddi_Mgr_t *dd            /* dd manager */
)
{
  /* \A7DM : questo campo non esiste pi\F9, ma viene calcolato come
  * somma di altri campi
  */
  return Cudd_ReadCacheLookUps(Ddi_MgrReadMgrCU(dd));
}

/**Function********************************************************************
  Synopsis     [Returns the number of cache hits.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
double
Ddi_MgrReadCacheHits(
  Ddi_Mgr_t *dd            /* dd manager */
)
{
  /* \A7DM : fornisce cacheHits+totCacheHits anzich\E9
  * solo cacheHits previsto dalla define
  */
  return Cudd_ReadCacheHits(Ddi_MgrReadMgrCU(dd));
}

/**Function********************************************************************
  Synopsis     [Reads the hit ratio that causes resizing of the computed
  table.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
unsigned int
Ddi_MgrReadMinHit(
  Ddi_Mgr_t *dd            /* dd manager */
)
{
  /* \A7DM : fornisce un'espressione razionale di minHit anzich\E9
  * solo minHit previsto dalla define
  */
  return Cudd_ReadMinHit(Ddi_MgrReadMgrCU(dd));
}

/**Function********************************************************************
  Synopsis     [Reads the maxCacheHard parameter of the manager.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
unsigned int
Ddi_MgrReadMaxCacheHard(
  Ddi_Mgr_t *dd            /* dd manager */
)
{
  return Cudd_ReadMaxCacheHard(Ddi_MgrReadMgrCU(dd));
}

/**Function********************************************************************
  Synopsis     [Returns the soft limit for the cache size.]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
unsigned int
Ddi_MgrReadMaxCache(
  Ddi_Mgr_t* dd            /* dd manager */
)
{
  /* \A7DM : questo campo non esiste pi\F9, ma viene calcolato come
  * somma di altri campi
  */
  return Cudd_ReadMaxCache( Ddi_MgrReadMgrCU( dd ) );
}

/**Function********************************************************************

  Synopsis     [Read max smooth vars in meta ops.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

long int
Ddi_MgrReadMetaMaxSmooth (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.meta.maxSmoothVars);
}

/**Function********************************************************************

  Synopsis     [Set max smooth vars in meta ops.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetMetaMaxSmooth (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  long int maxSmooth      /* IN: max Smoothing Vars */
  )
{
  ddiMgr->settings.meta.maxSmoothVars = maxSmooth;
  return;
}

/**Function********************************************************************

  Synopsis     [Read max smooth vars in meta ops.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

float
Ddi_MgrReadAigLazyRate (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.lazyRate);
}

/**Function********************************************************************

  Synopsis     []

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigSatTimeout (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.satTimeout);
}


/**Function********************************************************************

  Synopsis     []

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigSatTimeLimit (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.satTimeLimit);
}

/**Function********************************************************************

  Synopsis     []

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigSatIncremental (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.satIncremental);
}

/**Function********************************************************************

  Synopsis     []

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigSatVarActivity (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.satVarActivity);
}

/**Function********************************************************************

  Synopsis     []

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigSatIncrByRefinement (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.satIncrByRefinement);
}
/**Function********************************************************************

  Synopsis     []

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigMaxAllSolutionIter (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.maxAllSolutionIter);
}

/**Function********************************************************************

  Synopsis     [Set max smooth vars in meta ops.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigLazyRate (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  float lazyRate          /* IN: lazy rate */
  )
{
  ddiMgr->settings.aig.lazyRate = lazyRate;
  return;
}

/**Function********************************************************************

  Synopsis     []

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigSatTimeout (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int satTimeout          /* IN: timeout */
  )
{
  ddiMgr->settings.aig.satTimeout = satTimeout;
  return;
}

/**Function********************************************************************

  Synopsis     []

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigSatTimeLimit (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int satTimelimit          /* IN: timelimit */
  )
{
  ddiMgr->settings.aig.satTimeLimit = satTimelimit;
  return;
}

/**Function********************************************************************

  Synopsis     []

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigSatIncremental (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int satIncremental      /* IN: incremental */
  )
{
  ddiMgr->settings.aig.satIncremental = satIncremental;
  return;
}

/**Function********************************************************************

  Synopsis     []

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigSatVarActivity (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int satVarActivity      /* IN: var activity */
  )
{
  ddiMgr->settings.aig.satVarActivity = satVarActivity;
  return;
}

/**Function********************************************************************

  Synopsis     []

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigSatIncrByRefinement (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int satIncrByRefinement /* IN: incremental by refinement */
  )
{
  ddiMgr->settings.aig.satIncrByRefinement = satIncrByRefinement;
  return;
}
/**Function********************************************************************

  Synopsis     []

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigMaxAllSolutionIter (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int maxAllSolutionIter  /* IN: all solution max iter */
  )
{
  ddiMgr->settings.aig.maxAllSolutionIter = maxAllSolutionIter;
  return;
}


/**Function********************************************************************

  Synopsis     [Read ABC opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigAbcOptLevel (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.abcOptLevel);
}

/**Function********************************************************************

  Synopsis     [Read Aig cnf level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigCnfLevel (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.aigCnfLevel);
}

/**Function********************************************************************

  Synopsis     [Set ABC opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigAbcOptLevel (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int abcOptLevel         /* IN: abcOptLevel */
  )
{
  ddiMgr->settings.aig.abcOptLevel = abcOptLevel;
  return;
}

/**Function********************************************************************

  Synopsis     [Set Aig cnf level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigCnfLevel (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int aigCnfLevel         /* IN: abcOptLevel */
  )
{
  ddiMgr->settings.aig.aigCnfLevel = aigCnfLevel;
  return;
}

/**Function********************************************************************

  Synopsis     [Read BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigBddOptLevel (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.bddOptLevel);
}

/**Function********************************************************************

  Synopsis     [Set Aig BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigBddOptLevel (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int bddOptLevel         /* IN: abcOptLevel */
  )
{
  ddiMgr->settings.aig.bddOptLevel = bddOptLevel;
  return;
}

/**Function********************************************************************

  Synopsis     [Read BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigRedRemLevel (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.redRemLevel);
}

/**Function********************************************************************

  Synopsis     [Read BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigRedRemCnt (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.redRemCnt);
}

/**Function********************************************************************

  Synopsis     [Read BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigItpPartTh (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.itpPartTh);
}

/**Function********************************************************************

  Synopsis     [Read BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigDynAbstrStrategy (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.dynAbstrStrategy);
}

/**Function********************************************************************

  Synopsis     [Read BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigItpCore (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.itpCore);
}

/**Function********************************************************************

  Synopsis     [Read BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigItpMem (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.itpMem);
}

/**Function********************************************************************

  Synopsis     [Read ITP map level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigItpMap (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.itpMap);
}

/**Function********************************************************************

  Synopsis     [Read BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigItpOpt (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.itpOpt);
}

/**Function********************************************************************

  Synopsis     [Read BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigItpAbc (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.itpAbc);
}

/**Function********************************************************************

  Synopsis     [Read BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

char *
Ddi_MgrReadAigItpStore (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.itpStore);
}

/**Function********************************************************************

  Synopsis     [Read BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigItpLoad (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.itpLoad);
}

/**Function********************************************************************

  Synopsis     [Read BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigItpCompute (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.itpCompute);
}


/**Function********************************************************************

  Synopsis     [Read BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigTernaryAbstrStrategy (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.ternaryAbstrStrategy);
}

/**Function********************************************************************

  Synopsis     [Read BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

int
Ddi_MgrReadAigRedRemMaxCnt (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->settings.aig.redRemMaxCnt);
}

/**Function********************************************************************

  Synopsis     [Set Aig BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigRedRemLevel (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int redRemLevel         /* IN: abcOptLevel */
  )
{
  ddiMgr->settings.aig.redRemLevel = redRemLevel;
  return;
}

/**Function********************************************************************

  Synopsis     [Set Aig BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigRedRemCnt (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int redRemCnt           /* IN: abcOptLevel */
  )
{
  ddiMgr->settings.aig.redRemCnt = redRemCnt;
  return;
}

/**Function********************************************************************

  Synopsis     [Set Aig BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigItpPartTh (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int itpPartTh            /* IN: abcOptLevel */
  )
{
  ddiMgr->settings.aig.itpPartTh = itpPartTh;
  return;
}

/**Function********************************************************************

  Synopsis     [Set Aig BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigDynAbstrStrategy (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int dynAbstrStrategy /* IN: abcOptLevel */
  )
{
  ddiMgr->settings.aig.dynAbstrStrategy = dynAbstrStrategy;
  return;
}

/**Function********************************************************************

  Synopsis     [Set Aig BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigItpCore (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int itpCore              /* IN: itpCore */
  )
{
  ddiMgr->settings.aig.itpCore = itpCore;
  return;
}

/**Function********************************************************************

  Synopsis     [Set Aig BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigItpMem (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int itpMem               /* IN: itpMem */
  )
{
  ddiMgr->settings.aig.itpMem = itpMem;
  return;
}

/**Function********************************************************************

  Synopsis     [Set Aig map level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/


void
Ddi_MgrSetAigItpMap (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int itpMap               /* IN: itpMap */
  )
{
  ddiMgr->settings.aig.itpMap = itpMap;
  return;
}

/**Function********************************************************************

  Synopsis     [Set Aig BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigItpOpt (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int itpOpt               /* IN: itpOpt */
  )
{
  ddiMgr->settings.aig.itpOpt = itpOpt;
  return;
}

/**Function********************************************************************

  Synopsis     [Set Aig BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigItpAbc (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int itpAbc               /* IN: itpAbc */
  )
{
  ddiMgr->settings.aig.itpAbc = itpAbc;
  return;
}

/**Function********************************************************************

  Synopsis     [Set Aig BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigItpLoad (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int itpLoad               /* IN: itpLoad */
  )
{
  ddiMgr->settings.aig.itpLoad = itpLoad;
  return;
}

/**Function********************************************************************

  Synopsis     [Set Aig BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigItpCompute (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int itpCompute               /* IN: itpCompute */
  )
{
  ddiMgr->settings.aig.itpCompute = itpCompute;
  return;
}

/**Function********************************************************************

  Synopsis     [Set Aig BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigItpStore (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  char *itpStore               /* IN: itpStore */
  )
{
  Pdtutil_Free(ddiMgr->settings.aig.itpStore);
  ddiMgr->settings.aig.itpStore = Pdtutil_StrDup(itpStore);
  return;
}


/**Function********************************************************************

  Synopsis     [Set Aig BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigTernaryAbstrStrategy (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int ternaryAbstrStrategy /* IN: abcOptLevel */
  )
{
  ddiMgr->settings.aig.ternaryAbstrStrategy = ternaryAbstrStrategy;
  return;
}

/**Function********************************************************************

  Synopsis     [Set Aig BDD opt level.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetAigRedRemMaxCnt (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  int redRemMaxCnt        /* IN: abcOptLevel */
  )
{
  ddiMgr->settings.aig.redRemMaxCnt = redRemMaxCnt;
  return;
}

/**Function********************************************************************

  Synopsis     [Print DDI manager allocation statistics]

  Description  [Print DDI manager allocation statistics]

  SideEffects  []

  SeeAlso      []

******************************************************************************/
void
Ddi_MgrPrintAllocStats (
  Ddi_Mgr_t *ddiMgr,
  FILE *fp
)
{
  int i;

  fprintf(fp,"DDI Manager alloc statistics (generic).\n");
  fprintf(fp,"#Allocated nodes: %d.\n", ddiMgr->allocatedNum);
  fprintf(fp,"#Free nodes: %d.\n", ddiMgr->freeNum);
  fprintf(fp,"#Gen. nodes (all-free): %d.\n", ddiMgr->genericNum);
  if (ddiMgr->variables != NULL) {
    fprintf(fp,"#variables: %d.\n", Ddi_VararrayNum(ddiMgr->variables));
  }
  fprintf(fp,"#Const+vararray/groups: %d.\n", DdiMgrReadIntRef(ddiMgr));
  fprintf(fp,"#Locked: %d.\n", ddiMgr->lockedNum);
  fprintf(fp,"#Non locked: %d.\n", Ddi_MgrReadExtRef(ddiMgr));
  fprintf(fp,"#DDI Manager alloc statistics (by type).\n");
  for (i=0; i<DDI_NTYPE; i++) {
    fprintf(fp,"%-12s: %3d(ext) = %3d(tot) - %3d(locked).\n",
      typenames[i],
      ddiMgr->typeNum[i]-ddiMgr->typeLockedNum[i],
      ddiMgr->typeNum[i],ddiMgr->typeLockedNum[i]
    );
  }

}

/**Function********************************************************************

  Synopsis     [Garbage collect freed nodes (handles) in manager node list]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

Ddi_Generic_t *
Ddi_NodeFromName (
  Ddi_Mgr_t *ddiMgr,
  char *name
)
{
  Ddi_Generic_t *curr;

  if (ddiMgr->nodeList == NULL)
    return(NULL);

  curr = ddiMgr->nodeList;

  while (curr!=NULL) {
    if ((Ddi_ReadName(curr) != NULL)
      && (strcmp(Ddi_ReadName(curr),name)==0)) {
       return(curr);
    }
    curr = curr->common.next;
  }

  return(NULL);
}

/**Function********************************************************************

  Synopsis     [Read Memory Limit.]

  Description  [Values are in KBytes]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

long int
Ddi_MgrReadMemoryLimit (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->limit.memoryLimit);
}

/**Function********************************************************************

  Synopsis     [Set Memory Limit]

  Description  [Values are in KBytes]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetMemoryLimit (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */,
  long int memoryLimit    /* IN: Memory Limit */
  )
{
  ddiMgr->limit.memoryLimit = memoryLimit;

  return;
}

/**Function********************************************************************

  Synopsis     [Read Time Limit.]

  Description  [Values are in seconds]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

long int
Ddi_MgrReadTimeLimit (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->limit.timeLimit);
}

/**Function********************************************************************

  Synopsis     [Set Time Limit]

  Description  [Values are in seconds]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetTimeLimit (
  Ddi_Mgr_t *ddiMgr      /* IN: Source dd manager */,
  long int timeLimit    /* IN: Memory Limit */
  )
{
  ddiMgr->limit.timeLimit = timeLimit;

  return;
}

/**Function********************************************************************

  Synopsis     [Read Time Initial.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

long int
Ddi_MgrReadTimeInitial (
  Ddi_Mgr_t *ddiMgr        /* IN: Source dd manager */
  )
{
  return (ddiMgr->limit.timeInitial);
}

/**Function********************************************************************

  Synopsis     [Set Time Initial.]

  SideEffects  [none]

  SeeAlso      []

******************************************************************************/

void
Ddi_MgrSetTimeInitial (
  Ddi_Mgr_t *ddiMgr      /* IN: Source dd manager */
  )
{
  ddiMgr->limit.timeInitial = util_cpu_time();
  //Pdtutil_ConvertTime (util_cpu_time());

  return;
}

/**Function********************************************************************
  Synopsis     [Reads zero constant]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrReadAigNodesNum(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return (dd->aig.mgr->nodesArraySize/4 - dd->aig.mgr->freeNum);
}

/**Function********************************************************************
  Synopsis     [Reads zero constant]
  SideEffects  [none]
  SeeAlso      []
******************************************************************************/
int
Ddi_MgrReadPeakAigNodesNum(
  Ddi_Mgr_t *dd            /* source dd manager */
)
{
  return (dd->aig.mgr->nodesArraySize/4);
}

/**Function********************************************************************

  Synopsis    [Read target enlargment iterations]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

char *
Ddi_MgrReadSatSolver(
  Ddi_Mgr_t *ddiMgr       /* DDI Manager */
)
{
  return (ddiMgr->settings.aig.satSolver);
}


/**Function********************************************************************

  Synopsis    [Set mismatch opt level]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Ddi_MgrSetSatSolver(
  Ddi_Mgr_t *dd        /* DDI Manager */,
  char *satSolver      /* sat solver */
)
{
  Pdtutil_Free(dd->settings.aig.satSolver);
  dd->settings.aig.satSolver = Pdtutil_StrDup(satSolver);
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static int
MgrRegister(
  DdManager * manager,
  Ddi_Mgr_t * ddiMgr
)
{
  int i;

  pthread_mutex_lock(&registeredMgrMutex);

  for (i=0; i<nRegisteredMgr; i++) {
    if (manager == registeredMgr[i].cuMgr) {
      Pdtutil_Warning(1,"Manager is already registered");
      return(i);
    }
  }
  Pdtutil_Assert(nRegisteredMgr<DDI_MAX_REGISTERED_MGR,
    "Too many registered managers - increase DDI_MAX_REGISTERED_MGR !");
  registeredMgr[i].cuMgr = manager;
  registeredMgr[i].pdtMgr = ddiMgr;
  nRegisteredMgr++;

  pthread_mutex_unlock(&registeredMgrMutex);

  return(1);
}

/**Function********************************************************************
  Synopsis []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static int
MgrUnRegister(
  DdManager * manager
)
{
  int i;

  pthread_mutex_lock(&registeredMgrMutex);

  for (i=0; i<nRegisteredMgr; i++) {
    if (manager == registeredMgr[i].cuMgr) {
      break;
    }
  }
  Pdtutil_Assert(i<nRegisteredMgr,
      "Unregistering a non registered manager");
    /* remove registered manager from table */
  for (; i<nRegisteredMgr-1; i++) {
    registeredMgr[i]=registeredMgr[i+1];
  }
  nRegisteredMgr--;

  pthread_mutex_unlock(&registeredMgrMutex);

  return (1);
}

/**Function********************************************************************
  Synopsis []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Mgr_t *
MgrCuToPdt(
  DdManager * manager
)
{
  int i;
  Ddi_Mgr_t *pdtMgr=NULL;

  pthread_mutex_lock(&registeredMgrMutex);

  for (i=0; i<nRegisteredMgr; i++) {
    if (manager == registeredMgr[i].cuMgr) {
      pdtMgr = registeredMgr[i].pdtMgr;
      break;
    }
  }

  Pdtutil_Assert(pdtMgr!=NULL, "Manager is not registered");

  pthread_mutex_unlock(&registeredMgrMutex);

  return (pdtMgr);
}

/**Function********************************************************************

  Synopsis    [Transfer options from an option list to a Ddi Manager]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
void
Ddi_MgrSetOptionList(
  Ddi_Mgr_t *ddiMgr             /* DDI Manager */,
  Pdtutil_OptList_t *pkgOpt     /* list of options */
)
{

  Pdtutil_OptItem_t optItem;

  while (!Pdtutil_OptListEmpty(pkgOpt)) {
    optItem = Pdtutil_OptListExtractHead(pkgOpt);
    Ddi_MgrSetOptionItem(ddiMgr, optItem);
  }
}

/**Function********************************************************************

  Synopsis    [Transfer options from an option list to a Ddi Manager]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
void
Ddi_MgrSetOptionItem(
  Ddi_Mgr_t *ddiMgr             /* DDI Manager */,
  Pdtutil_OptItem_t optItem     /* option to set */
)
{
  switch (optItem.optTag.eDdiOpt) {
    case Pdt_DdiVerbosity_c:
      ddiMgr->settings.verbosity = Pdtutil_VerbLevel_e(optItem.optData.inum);
      break;

    case Pdt_DdiExistOptLevel_c:
      ddiMgr->settings.part.existOptLevel = optItem.optData.inum;
      break;
    case Pdt_DdiExistOptThresh_c:
      ddiMgr->settings.part.existOptClustThresh = optItem.optData.inum;
      break;

    case Pdt_DdiSatSolver_c:
      Pdtutil_Free(ddiMgr->settings.aig.satSolver);
      ddiMgr->settings.aig.satSolver = Pdtutil_StrDup(optItem.optData.pchar);
      break;
    case Pdt_DdiLazyRate_c:
      ddiMgr->settings.aig.lazyRate = optItem.optData.fnum;
      break;
    case Pdt_DdiAigAbcOpt_c:
      ddiMgr->settings.aig.abcOptLevel = optItem.optData.inum;
      break;
    case Pdt_DdiAigBddOpt_c:
      ddiMgr->settings.aig.bddOptLevel = optItem.optData.inum;
      break;
    case Pdt_DdiAigRedRem_c:
      ddiMgr->settings.aig.redRemLevel = optItem.optData.inum;
      break;
    case Pdt_DdiAigRedRemPeriod_c:
      ddiMgr->settings.aig.redRemMaxCnt = optItem.optData.inum;
      break;
    case Pdt_DdiItpPartTh_c:
      ddiMgr->settings.aig.itpPartTh = optItem.optData.inum;
      break;
    case Pdt_DdiItpIteOptTh_c:
      ddiMgr->settings.aig.itpIteOptTh = optItem.optData.inum;
      break;
    case Pdt_DdiItpStructOdcTh_c:
      ddiMgr->settings.aig.itpStructOdcTh = optItem.optData.inum;
      break;
    case Pdt_DdiNnfClustSimplify_c:
      ddiMgr->settings.aig.nnfClustSimplify = optItem.optData.inum;
      break;
    case Pdt_DdiItpStoreTh_c:
      ddiMgr->settings.aig.itpStoreTh = optItem.optData.inum;
      break;
    case Pdt_DdiDynAbstrStrategy_c:
      ddiMgr->settings.aig.dynAbstrStrategy = optItem.optData.inum;
      break;
    case Pdt_DdiTernaryAbstrStrategy_c:
      ddiMgr->settings.aig.ternaryAbstrStrategy = optItem.optData.inum;
      break;
    case Pdt_DdiSatTimeout_c:
      ddiMgr->settings.aig.satTimeout = optItem.optData.inum;
      break;
    case Pdt_DdiSatIncremental_c:
      ddiMgr->settings.aig.satIncremental = optItem.optData.inum;
      break;
    case Pdt_DdiSatVarActivity_c:
      ddiMgr->settings.aig.satVarActivity = optItem.optData.inum;
      break;
    case Pdt_DdiSatTimeLimit_c:
      ddiMgr->settings.aig.satTimeLimit = optItem.optData.inum;
      break;
    case Pdt_DdiItpOpt_c:
      ddiMgr->settings.aig.itpOpt = optItem.optData.inum;
      break;
    case Pdt_DdiItpAbc_c:
      ddiMgr->settings.aig.itpAbc = optItem.optData.inum;
      break;
    case Pdt_DdiItpActiveVars_c:
      ddiMgr->settings.aig.itpActiveVars = optItem.optData.inum;
      break;
    case Pdt_DdiItpTwice_c:
      ddiMgr->settings.aig.itpTwice = optItem.optData.inum;
      break;
    case Pdt_DdiItpReverse_c:
      ddiMgr->settings.aig.itpReverse = optItem.optData.inum;
      break;
    case Pdt_DdiItpCore_c:
      ddiMgr->settings.aig.itpCore = optItem.optData.inum;
      break;
    case Pdt_DdiItpAigCore_c:
      ddiMgr->settings.aig.itpAigCore = optItem.optData.inum;
      break;
  case Pdt_DdiItpMem_c:
      ddiMgr->settings.aig.itpMem = optItem.optData.inum;
      break;
  case Pdt_DdiItpMap_c:
    ddiMgr->settings.aig.itpMap = optItem.optData.inum;
    break;
  case Pdt_DdiItpStore_c:
    Pdtutil_Free(ddiMgr->settings.aig.itpStore);
    ddiMgr->settings.aig.itpStore = Pdtutil_StrDup(optItem.optData.pchar);
    break;
  case Pdt_DdiItpLoad_c:
    ddiMgr->settings.aig.itpLoad = optItem.optData.inum;
    break;
  case Pdt_DdiItpDrup_c:
    ddiMgr->settings.aig.itpDrup = optItem.optData.inum;
    break;
  case Pdt_DdiItpUseCare_c:
    ddiMgr->settings.aig.itpUseCare = optItem.optData.inum;
    break;
  case Pdt_DdiItpCompute_c:
    ddiMgr->settings.aig.itpCompute = optItem.optData.inum;
    break;
  case Pdt_DdiAigCnfLevel_c:
    ddiMgr->settings.aig.aigCnfLevel = optItem.optData.inum;
    break;
  case Pdt_DdiItpCompact_c:
    ddiMgr->settings.aig.itpCompact = optItem.optData.inum;
    break;
  case Pdt_DdiItpClust_c:
    ddiMgr->settings.aig.itpClust = optItem.optData.inum;
    break;
  case Pdt_DdiItpNorm_c:
    ddiMgr->settings.aig.itpNorm = optItem.optData.inum;
    break;
  case Pdt_DdiItpSimp_c:
    ddiMgr->settings.aig.itpSimp = optItem.optData.inum;
    break;

  default:
      Pdtutil_Assert(0,"wrong ddi opt list item tag");
  }
}

/**Function********************************************************************

  Synopsis    [Return an option value from a Ddi Manager ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
void
Ddi_MgrReadOption (
  Ddi_Mgr_t *ddiMgr            /* DDI Manager */,
  Pdt_OptTagDdi_e ddiOpt       /* option code */,
  void *optRet                 /* returned option value */
)
{
  switch (ddiOpt) {
    case Pdt_DdiVerbosity_c:
      *(int *)optRet = (int)ddiMgr->settings.verbosity;
      break;

    case Pdt_DdiExistOptLevel_c:
      *(int *)optRet = ddiMgr->settings.part.existOptLevel;
      break;
    case Pdt_DdiExistOptThresh_c:
      *(int *)optRet = ddiMgr->settings.part.existOptClustThresh;
      break;

    case Pdt_DdiSatSolver_c:
      *(char **)optRet = ddiMgr->settings.aig.satSolver;
      break;
    case Pdt_DdiLazyRate_c:
      *(float *)optRet = ddiMgr->settings.aig.lazyRate;
      break;
    case Pdt_DdiAigAbcOpt_c:
      *(int *)optRet = ddiMgr->settings.aig.abcOptLevel;
      break;
    case Pdt_DdiAigBddOpt_c:
      *(int *)optRet = ddiMgr->settings.aig.bddOptLevel;
      break;
    case Pdt_DdiAigRedRem_c:
      *(int *)optRet = ddiMgr->settings.aig.redRemLevel;
      break;
    case Pdt_DdiAigRedRemPeriod_c:
      *(int *)optRet = ddiMgr->settings.aig.redRemMaxCnt;
      break;
    case Pdt_DdiItpStructOdcTh_c:
      *(int *)optRet = ddiMgr->settings.aig.itpStructOdcTh;
      break;
    case Pdt_DdiNnfClustSimplify_c:
      *(int *)optRet = ddiMgr->settings.aig.nnfClustSimplify;
      break;
    case Pdt_DdiItpPartTh_c:
      *(int *)optRet = ddiMgr->settings.aig.itpPartTh;
      break;
    case Pdt_DdiItpIteOptTh_c:
      *(int *)optRet = ddiMgr->settings.aig.itpIteOptTh;
      break;
    case Pdt_DdiItpStoreTh_c:
      *(int *)optRet = ddiMgr->settings.aig.itpStoreTh;
      break;
    case Pdt_DdiDynAbstrStrategy_c:
      *(int *)optRet = ddiMgr->settings.aig.dynAbstrStrategy;
      break;
    case Pdt_DdiTernaryAbstrStrategy_c:
      *(int *)optRet = ddiMgr->settings.aig.ternaryAbstrStrategy;
      break;
    case Pdt_DdiSatTimeout_c:
      *(int *)optRet = ddiMgr->settings.aig.satTimeout;
      break;
    case Pdt_DdiSatIncremental_c:
      *(int *)optRet = ddiMgr->settings.aig.satIncremental;
      break;
    case Pdt_DdiSatVarActivity_c:
      *(int *)optRet = ddiMgr->settings.aig.satVarActivity;
      break;
    case Pdt_DdiSatTimeLimit_c:
      *(int *)optRet = ddiMgr->settings.aig.satTimeLimit;
      break;
    case Pdt_DdiItpOpt_c:
      *(int *)optRet = ddiMgr->settings.aig.itpOpt;
      break;
    case Pdt_DdiItpAbc_c:
      *(int *)optRet = ddiMgr->settings.aig.itpAbc;
      break;
    case Pdt_DdiItpActiveVars_c:
      *(int *)optRet = ddiMgr->settings.aig.itpActiveVars;
      break;
    case Pdt_DdiItpTwice_c:
      *(int *)optRet = ddiMgr->settings.aig.itpTwice;
      break;
    case Pdt_DdiItpReverse_c:
      *(int *)optRet = ddiMgr->settings.aig.itpReverse;
      break;
    case Pdt_DdiItpCore_c:
      *(int *)optRet = ddiMgr->settings.aig.itpCore;
      break;
    case Pdt_DdiItpAigCore_c:
      *(int *)optRet = ddiMgr->settings.aig.itpAigCore;
      break;
    case Pdt_DdiItpMem_c:
      *(int *)optRet = ddiMgr->settings.aig.itpMem;
      break;
  case Pdt_DdiItpMap_c:
      *(int *)optRet = ddiMgr->settings.aig.itpMap;
      break;
    case Pdt_DdiItpStore_c:
      *(char **)optRet = ddiMgr->settings.aig.itpStore;
      break;
  case Pdt_DdiItpLoad_c:
      *(int *)optRet = ddiMgr->settings.aig.itpLoad;
      break;
  case Pdt_DdiItpDrup_c:
      *(int *)optRet = ddiMgr->settings.aig.itpDrup;
      break;
  case Pdt_DdiItpUseCare_c:
      *(int *)optRet = ddiMgr->settings.aig.itpUseCare;
      break;
  case Pdt_DdiItpCompute_c:
      *(int *)optRet = ddiMgr->settings.aig.itpCompute;
      break;
    case Pdt_DdiAigCnfLevel_c:
      *(int *)optRet = ddiMgr->settings.aig.aigCnfLevel;
      break;

    default:
      Pdtutil_Assert(0,"wrong ddi opt item tag");
  }
}

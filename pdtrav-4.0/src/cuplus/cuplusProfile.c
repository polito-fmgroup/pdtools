/**CFile***********************************************************************

  FileName    [cuplusProfile.c]

  PackageName [cuplus]

  Synopsis    [Managing Activity profiles.]

  Description [This file includes routines for creating activity profiles,
    collecting statistics within recursive And-Abstract operations,
    pruning BDDs using activity profiles.
    Since CUDD does not provide room for extra pointers within
    the BDD node structure, we use hashing (st package)
    to implement the correspondence between a node and a profile
    info.
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

#include "cuplusInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define USE_DEAD 1

/*
 *  In the Profile by Var Method:
 *  if COUNTER_COST = 1 use cost to partition
 *  if COUNTER_COST = 0 use recursione counter to partition
 */
#define COUNTER_COST 1

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

struct profile_info_item2 {
  /*
   *  Fields For The Nodes
   */
  int cacheHits;
  int nodeVisitCnt;
  int sizeCost;
  float sizeCostNorm;
  /*
   *  Fields For Then/Else Child of the Node
   */
  int nodeVisitCntThen;
  int nodeVisitCntElse;
  int sizeCostThen;
  int sizeCostElse;
  float sizeCostThenNorm;
  float sizeCostElseNorm;
};

struct profile_info_summary {
  /*
   *  To Select:
   *    0 = DAC'99 profile
   *    1 = TCAD'00 profile
   *    2 = By Var profile
   */
  unsigned char dac99Compatible;

  /*
   *  DAC'99 - TCAD'00 fields
   */
  st_table *table;
  unsigned char totalSummaryDone;
  unsigned char nodeSummaryDone;
  int totalNodes;
  int totalActiveNodes;
  double totalFullSubtrees;
  double totalNonZeroNodes;
  double totalCachedNodes;
  double totalRecursions;
  double totalCacheHits;
  double totalSizeCost;
  double avgRecursions;
  double avgCacheHits;
  double avgSizeCost;
  double stdRecursions;
  double stdCacheHits;
  double stdSizeCost;
  double maxRecursions;
  double maxCacheHits;
  double maxNormSizeCost;
  double minNormSizeCost;
  double maxBDDSize;
  int maxVarIndex;
  /*
  int profile[10];
  int profileByVar[10][10];
  int profileBySize[10][10];
  int rec_profileByVar[10][10];
  int cache_profileByVar[10][10];
  int *sizeByVar;
  */

  /*
   *  New By Var Profile Fields
   */
  profile_info_item2_t *table2;
  int nVar;
  int nPart;
  int currentPart;
};

typedef struct profile_info_item {
  int profileSubtree;
  int cacheHits;
  int nodeVisitCnt;
  int sizeCost;
  int normSizeCost;
  int totAct;
  int sharing;
  DdNode *ref_node;
  DdNode *pruned_bdd;
  struct profile_info_item *next;
  int num;
} profile_info_item_t;

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#define my_square(a) (((double)a)*((double)a))

#if USE_DEAD
# define MANAGER_KEYS(manager,profile) \
((profile->dac99Compatible)?(manager->keys):(manager->keys - manager->dead))
#else
# define MANAGER_KEYS(manager,profile) (manager->keys)
#endif

#define vet2mat(r,c,C) (r*C+c)

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static void ProfileTotalSummary(cuplus_profile_info_t *profile);
static enum st_retval ProfileCentralSummary(char *key, char *value, char *arg);
static enum st_retval ProfileNodeSummary(char *key, char *value, char *arg);
static enum st_retval ProfileInfoItemFree(char * key, char * value, char * arg);
static DdNode * cuplusProfileAndAbstractRecur(DdManager *manager, DdNode *f, DdNode *g, DdNode *cube, cuplus_profile_info_t *profile);
static DdNode * cuplusPruneProfiledRecur(DdManager *manager, DdNode *f, cuplus_profile_info_t *profile, Cuplus_PruneHeuristic_e pruneHeuristic, int threshold, int upperActivity);
static DdNode * cuplusPruneProfiledByVar(DdManager *manager, DdNode *f, cuplus_profile_info_t *profile, Cuplus_PruneHeuristic_e pruneHeuristic, int threshold);
static void cuplusCollectProfileSummaryRecur(DdManager *manager, DdNode *f, cuplus_profile_info_t *profile);
static void BfvProfiles(DdManager *manager, cuplus_profile_info_t *profile);
static enum st_retval ProfileInfoByVars(char * key, char * value, char * arg);
static void ddClearFlag(DdNode * f);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Takes the AND of two BDDs and simultaneously abstracts the
    variables in cube.]

  Description [A patched version of the original CUDD function.]

  SideEffects [None]

  SeeAlso     [Cudd_bddAndAbstract]

******************************************************************************/

DdNode *
Cuplus_ProfileAndAbstract (
  DdManager *manager,
  DdNode *f,
  DdNode *g,
  DdNode *cube,
  cuplus_profile_info_t *profile
)
{
  DdNode *res;

  if (profile->dac99Compatible == 2) {
    Pdtutil_Assert (profile->currentPart!=(-1),
      "profile->currentPart TO SET.");
  }

  /* Reordering NOT allowed in present implementation ! */
  manager->reordered = 0;

  res = cuplusProfileAndAbstractRecur (manager, f, g, cube, profile);

  assert (manager->reordered == 0);

  return(res);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

void
Cuplus_ProfileSetCurrentPart (
  cuplus_profile_info_t *profile,
  int currentPart
  )
{
  if (profile!=NULL) {
    profile->currentPart = currentPart;
  }

  return;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int
Cuplus_ProfileReadCurrentPart (
  cuplus_profile_info_t *profile
  )
{
  int currentPart;

  if (profile==NULL) {
    currentPart = (-1);
  } else {
    currentPart = profile->currentPart;
  }

  return (currentPart);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

DdNode *
Cuplus_ProfilePrune (
  DdManager *manager,
  DdNode *f,
  cuplus_profile_info_t *profile,
  Cuplus_PruneHeuristic_e pruneHeuristic,
  int threshold
  )
{
  DdNode *res;
  st_table *table = profile->table;
  struct profile_info_item *info_item_ptr;
  int nvars;

  /*
   *  Profile By Variable Section
   */

  if (profile->dac99Compatible == 2) {
    res = cuplusPruneProfiledByVar (manager, f, profile,
      pruneHeuristic, threshold);

  return(res);
  }

  /*
   *  Profile By Node (original profile) Section
   */

  nvars = Cudd_ReadSize (manager);

  ProfileTotalSummary (profile);

#if 0
  if ((profile->sizeByVar = Pdtutil_Alloc(int,nvars)) == NULL) {
    fprintf(stderr, "Out of memory collecting profile summary.\n");
    return (NULL);
  }

  for (i=0; i<nvars; i++) {
    profile->sizeByVar[i] = 0;
  }
#endif

  cuplusCollectProfileSummaryRecur (manager, f, profile);
  ddClearFlag (Cudd_Regular (f));

  BfvProfiles (manager, profile);

  if (st_lookup (table, (char *)f, (char **)&info_item_ptr)) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelDevMax_c,
      fprintf(stdout, " Root size cost/BDD size  : %d/%d.\n",
        info_item_ptr->sizeCost, Cudd_DagSize(f))
    );
  }

  /* Reordering NOT allowed in present implementation! */
  manager->reordered = 0;

  res = cuplusPruneProfiledRecur (manager, f, profile,
    pruneHeuristic, threshold, 0);

  assert (manager->reordered == 0);

  return(res);
}

/**Function********************************************************************

  Synopsis    [Create a new profile]

  Description [Allocate a structure for global statistics of a profile.
    Initialize fields. Create a symbol table for the correspondence
    node-profile info. The dac99Compatible flag must be 1
    for experimental results as in the dac99 paper. It must be set
    now.
    ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

cuplus_profile_info_t *
Cuplus_ProfileInfoGen (
  unsigned char dac99Compatible,
  int nVar,
  int nPart
  )
{
  cuplus_profile_info_t *profile;
  st_table *table;
  profile_info_item2_t *table2;
  int i;

  /*
   *  Profile Allocation
   */

  profile = Pdtutil_Alloc (cuplus_profile_info_t, 1);
  if (profile == NULL) {
    Pdtutil_Warning (1, "Out-of-memory, couldn't alloc profile table.");
    return (NULL);
  }

  profile->dac99Compatible = dac99Compatible;

  /*
   *  DAC'00 - TCAD'00 Fields
   */

  table = st_init_table (st_ptrcmp, st_ptrhash);
  if (table == NULL) {
    Pdtutil_Free (profile);
    Pdtutil_Warning (1, "Out-of-memory, couldn't alloc profile info table.");
    return (NULL);
  }

  profile->table = table;
  profile->totalSummaryDone = 0;
  profile->nodeSummaryDone = 0;
  profile->totalNodes = 0;
  profile->totalActiveNodes = 0;
  profile->totalFullSubtrees = 0;
  profile->totalCachedNodes = 0;
  profile->totalNonZeroNodes = 0;
  profile->maxRecursions = 0;
  profile->maxCacheHits = 0;
  profile->maxNormSizeCost = 0;
  profile->minNormSizeCost = 100000;
  profile->totalRecursions = 0;
  profile->totalCacheHits = 0;
  profile->totalSizeCost = 0;
  profile->avgRecursions = 0;
  profile->avgCacheHits = 0;
  profile->avgSizeCost = 0;
  profile->stdRecursions = 0;
  profile->stdCacheHits = 0;
  profile->stdSizeCost = 0;
  profile->maxVarIndex = 0;
  profile->maxBDDSize = 0;

  /*
   *  By Var Fields
   */

  if (dac99Compatible == 2) {
    table2 = Pdtutil_Alloc (profile_info_item2_t, nPart*nVar);
    if (table2 == NULL) {
      Pdtutil_Free (table2);
      Pdtutil_Warning (1,
        "Out-of-memory, couldn't alloc profile info NODE table.");
      return (NULL);
    }

    for (i=0; i<(nPart*nVar); i++) {
      table2[i].cacheHits = 0;
      table2[i].nodeVisitCnt = 0;
      table2[i].sizeCost = 0;
      table2[i].sizeCostNorm = 0.0;
      table2[i].nodeVisitCntThen = 0;
      table2[i].nodeVisitCntElse = 0;
      table2[i].sizeCostThen = 0;
      table2[i].sizeCostThenNorm = 0.0;
      table2[i].sizeCostElse = 0;
      table2[i].sizeCostElseNorm = 0.0;
    }

    profile->table2 = table2;
    profile->nVar = nVar;
    profile->nPart = nPart;
    profile->currentPart = -1;
  } else {
    profile->table2 = NULL;
    profile->nVar = -1;
    profile->nPart = -1;
    profile->currentPart = -1;
  }

  return (profile);
}

/**Function********************************************************************

  Synopsis    [Free a profile]

  Description [Free a profile struct and the associated symbol table.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

void
Cuplus_ProfileInfoFree(
  cuplus_profile_info_t *profile
  )
{
  st_table *table;
  profile_info_item2_t *table2;

  if (profile==NULL) {
    return;
  }

  table = profile->table;
  if (table != NULL) {
    st_foreach (table, ProfileInfoItemFree, NULL);
    st_free_table (table);
  }

  if (profile->dac99Compatible == 2) {
    table2 = profile->table2;
    if (table2 != NULL) {
      Pdtutil_Free (table2);
    }
  }

  Pdtutil_Free (profile);

  return;
}

/**Function********************************************************************

  Synopsis    [Print global profile statistics]

  Description [Print some global statistics about a profile, e.g. average and
    max values of activity indicators.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

void
Cuplus_ProfileInfoPrint (
  cuplus_profile_info_t *profile
  )
{
  if (profile->dac99Compatible < 2) {
    Cuplus_ProfileInfoPrintOriginal (profile);
  } else {
    Cuplus_ProfileInfoPrintByVar (profile);
  }

  return;
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Print global profile statistics]

  Description [Print some global statistics about a profile, e.g. average and
    max values of activity indicators.
    Profile By Node (original profile) Section
   ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

void
Cuplus_ProfileInfoPrintOriginal (
  cuplus_profile_info_t *profile
  )
{
  ProfileTotalSummary (profile);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Profile By Node Summary:.\n");
    fprintf(stdout, "Total active/profiled nodes   : %d/%d.\n",
      profile->totalActiveNodes, profile->totalNodes);
    fprintf(stdout, "Total full subtrees           : %g.\n",
      profile->totalFullSubtrees);
    fprintf(stdout, "Total cached nodes            : %g.\n",
      profile->totalCachedNodes);
    fprintf(stdout, "Total non 0 nodes             : %g.\n",
      profile->totalNonZeroNodes);
    fprintf(stdout, "Recursions Tot/Avg/Std/Max    : %.4g/%.4g/%.4g/%.4g.\n",
      profile->totalRecursions, profile->avgRecursions,
      profile->stdRecursions,  profile->maxRecursions);
    fprintf(stdout, "Cache hits Tot/Avg/Std/Max    : %.4g/%.4g/%.4g/%.4g.\n",
      profile->totalCacheHits, profile->avgCacheHits,
      profile->stdCacheHits, profile->maxCacheHits);
    fprintf(stdout, "Size cost Tot/Avg/Std/Min/Max : %.4g/%.4g/%g/%.4g/%.4g.\n",
      profile->totalSizeCost, profile->avgSizeCost,
      profile->stdSizeCost, profile->minNormSizeCost,
      profile->maxNormSizeCost);
  );

  return;
}

/**Function********************************************************************

  Synopsis    [Print global profile statistics]

  Description [Print some global statistics about a profile, e.g. average and
    max values of activity indicators.
    ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

void
Cuplus_ProfileInfoPrintByVar (
  cuplus_profile_info_t *profile
  )
{
  int flagPrint, i, j, nVar;
  int cacheHits,  nodeVisitCnt, sizeCost,
    sizeCostThen, sizeCostElse,
    nodeVisitCntThen, nodeVisitCntElse;
  int cacheHitsMax, sizeCostMax, nodeVisitCntMax,
    nodeVisitCntThenMax, nodeVisitCntElseMax,
    sizeCostThenMax, sizeCostElseMax;
  int cacheHitsNum, nodeVisitCntNum, sizeCostNum,
    nodeVisitCntThenNum, nodeVisitCntElseNum,
    sizeCostThenNum, sizeCostElseNum;

  flagPrint = 0;

  /*
   *  Print Statistics
   */

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Profile By Var Summary:\n")
  );

  nVar = profile->nVar;

  cacheHitsMax = -1;
  nodeVisitCntMax = -1;
  sizeCostMax = -1;
  nodeVisitCntThenMax = -1;
  nodeVisitCntElseMax = -1;
  sizeCostThenMax = -1;
  sizeCostElseMax = -1;

  cacheHitsNum = 0;
  nodeVisitCntNum = 0;
  sizeCostNum = 0;
  nodeVisitCntThenNum = 0;
  nodeVisitCntElseNum = 0;
  sizeCostThenNum = 0;
  sizeCostElseNum = 0;

  for (i=0; i<profile->nPart; i++) {
    if (flagPrint==1) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Partition %d:\n", i)
      );
    }

    for (j=0; j<nVar; j++) {
      cacheHits = profile->table2[vet2mat(i, j, nVar)].cacheHits;
      nodeVisitCnt = profile->table2[vet2mat(i, j, nVar)].nodeVisitCnt;
      sizeCost = profile->table2[vet2mat(i, j, nVar)].sizeCost;
      nodeVisitCntThen = profile->table2[vet2mat(i, j, nVar)].nodeVisitCntThen;
      nodeVisitCntElse = profile->table2[vet2mat(i, j, nVar)].nodeVisitCntElse;
      sizeCostThen = profile->table2[vet2mat(i, j, nVar)].sizeCostThen;
      sizeCostElse = profile->table2[vet2mat(i, j, nVar)].sizeCostElse;

      if (cacheHits > 0) {
       cacheHitsNum++;
      }
      if (nodeVisitCnt > 0) {
       nodeVisitCntNum++;
      }
      if (sizeCost > 0) {
       sizeCostNum++;
      }

      if (cacheHits != 0 || nodeVisitCnt != 0 || sizeCost != 0) {

        if (cacheHits > cacheHitsMax) {
          cacheHitsMax = cacheHits;
        }
        if (sizeCost > sizeCostMax) {
          sizeCostMax = sizeCost;
        }
        if (nodeVisitCnt > nodeVisitCntMax) {
          nodeVisitCntMax = nodeVisitCnt;
        }
        if (sizeCostThen > sizeCostThenMax) {
          sizeCostThenMax = sizeCostThen;
        }
        if (sizeCostElse > sizeCostElseMax) {
          sizeCostElseMax = sizeCostElse;
        }
        if (nodeVisitCntThen > nodeVisitCntThenMax) {
          nodeVisitCntThenMax = nodeVisitCntThen;
        }
        if (nodeVisitCntElse > nodeVisitCntElseMax) {
          nodeVisitCntElseMax = nodeVisitCntElse;
        }
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          if (flagPrint==1) {
            fprintf(stdout,
              "   #Var=%d - cacheH=%d, visitT,E,Tot=%d,%d,%d sizeT,E,Tot=%d,%d,%d.\n",
              j, cacheHits, nodeVisitCntThen, nodeVisitCntElse,
              nodeVisitCnt, sizeCostThen, sizeCostElse, sizeCost);
          }
        );
      }
    }
  }

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "cacheHitsNum = %d.\n", cacheHitsNum);
    fprintf(stdout, "nodeVisitCntNum = %d.\n", nodeVisitCntNum);
    fprintf(stdout, "sizeCostNum = %d.\n", sizeCostNum);
    fprintf(stdout, "cacheHitsMax = %d.\n", cacheHitsMax);
    fprintf(stdout, "nodeVisitCntMax = %d.\n", nodeVisitCntMax);
    fprintf(stdout, "sizeCostMax = %d.\n", sizeCostMax);
    fprintf(stdout, "nodeVisitCntThenMax = %d.\n", nodeVisitCntThenMax);
    fprintf(stdout, "nodeVisitCntElseMax = %d.\n", nodeVisitCntElseMax);
    fprintf(stdout, "sizeCostThenMax = %d.\n", sizeCostThenMax);
    fprintf(stdout, "sizeCostElseMax = %d.\n", sizeCostElseMax)
  );

  /*
   *  Normalize Size Costs to 100
   */

  /*
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Normalized Size Costs:\n")
  );
  */

  for (i=0; i<profile->nPart; i++) {
    for (j=0; j<nVar; j++) {
      profile->table2[vet2mat(i, j, nVar)].sizeCostNorm =
        ((float) profile->table2[vet2mat(i, j, nVar)].sizeCost /
        (float) sizeCostMax) * 100.0;

      profile->table2[vet2mat(i, j, nVar)].sizeCostThenNorm =
        ((float) profile->table2[vet2mat(i, j, nVar)].sizeCostThen /
        (float) sizeCostThenMax) * 100.0;

      profile->table2[vet2mat(i, j, nVar)].sizeCostElseNorm =
        ((float) profile->table2[vet2mat(i, j, nVar)].sizeCostElse /
        (float) sizeCostElseMax) * 100.0;
    }
  }

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "\n")
  );

  return;
}

/**Function********************************************************************

  Synopsis [Routine computing global profile statistics.]

  Description [Collect final global profile statistics at the end of the
    learning phase.]

  SideEffects [None]

******************************************************************************/

static void
ProfileTotalSummary (
  cuplus_profile_info_t *profile
  )
{
  st_table *table = profile->table;

  /* Avoid a Second Summary */
  if (profile->totalSummaryDone == 1) {
    return;
  }

  st_foreach (table, ProfileCentralSummary, (char *) (profile));

  assert (profile->totalNodes == st_count(table));
  profile->totalActiveNodes = profile->totalNonZeroNodes
                       /* +profile->totalFullSubtrees */ ;
  profile->avgRecursions = profile->totalRecursions/profile->totalNodes;
  profile->avgCacheHits = profile->totalCacheHits/profile->totalNodes;
  profile->avgSizeCost  = profile->totalSizeCost/profile->totalNodes;

  st_foreach (table, ProfileNodeSummary, (char *) (profile));

  profile->stdRecursions = sqrt(profile->stdRecursions/profile->totalNodes-
    profile->avgRecursions*profile->avgRecursions);
  profile->stdCacheHits = sqrt(profile->stdCacheHits/profile->totalNodes-
    profile->avgCacheHits*profile->avgCacheHits);
  profile->stdSizeCost  = sqrt(profile->stdSizeCost/profile->totalNodes-
    profile->avgSizeCost*profile->avgSizeCost);

  profile->totalSummaryDone = 1;

  return;
}

/**Function********************************************************************

  Synopsis    [Profile summary using node statistics.]

  Description [Profile summary using node statistics. Computing total and
    max values.]

  SideEffects [None]

******************************************************************************/

static enum st_retval
ProfileCentralSummary (
  char *key,
  char *value,
  char *arg
  )
{
  cuplus_profile_info_t *profile;
  struct profile_info_item *info_item_ptr;
  double normSizeCost, bddSize;

  profile = (cuplus_profile_info_t *) arg;
  info_item_ptr = (struct profile_info_item *) value;

  if (info_item_ptr->profileSubtree > 0) {
    profile->totalFullSubtrees++;
  }

  profile->totalCacheHits += info_item_ptr->cacheHits;
  if (profile->maxCacheHits < info_item_ptr->cacheHits) {
    profile->maxCacheHits = info_item_ptr->cacheHits;
  }
  if (info_item_ptr->cacheHits > 0) {
    profile->totalCachedNodes++;
  }

  if (info_item_ptr->nodeVisitCnt > 0) {

    profile->totalRecursions += info_item_ptr->nodeVisitCnt;

    if (profile->maxRecursions < info_item_ptr->nodeVisitCnt) {
      profile->maxRecursions = info_item_ptr->nodeVisitCnt;
    }

    bddSize = (double) info_item_ptr->sizeCost;

    if (profile->maxBDDSize < bddSize) {
      profile->maxBDDSize = bddSize;
    }

    normSizeCost = bddSize/info_item_ptr->nodeVisitCnt;

    profile->totalSizeCost += normSizeCost;

    if (profile->maxNormSizeCost < normSizeCost) {
      profile->maxNormSizeCost = normSizeCost;
    }

    if (profile->minNormSizeCost > normSizeCost) {
      profile->minNormSizeCost = normSizeCost;
    }

    if (profile->maxVarIndex < Cudd_Regular(info_item_ptr->ref_node)->index) {
      profile->maxVarIndex = Cudd_Regular(info_item_ptr->ref_node)->index;
    }

    profile->totalNonZeroNodes++;
  }

  return (ST_CONTINUE);
}

/**Function********************************************************************

  Synopsis    [Node statistic summary. ]

  Description [Node statistic summary. Preparing average statistics
    and node normalized size costs.]

  SideEffects [None]

******************************************************************************/

static enum st_retval
ProfileNodeSummary(
  char *key,
  char *value,
  char *arg
  )
{
  cuplus_profile_info_t *profile;
  struct profile_info_item *info_item_ptr;
  double normSizeCost, sizeCost;

  profile = (cuplus_profile_info_t *) arg;
  info_item_ptr = (struct profile_info_item *) value;

  profile->stdRecursions +=
    my_square((info_item_ptr->nodeVisitCnt));

  profile->stdCacheHits +=
    my_square((info_item_ptr->cacheHits));

  if (info_item_ptr->nodeVisitCnt > 0) {
    sizeCost = (double) info_item_ptr->sizeCost;
    normSizeCost = sizeCost/info_item_ptr->nodeVisitCnt;

    profile->stdSizeCost += my_square (normSizeCost);

    info_item_ptr->sizeCost =
      (1000.0 * sizeCost)/profile->maxBDDSize;
    info_item_ptr->normSizeCost =
      (1000.0 * normSizeCost)/profile->maxNormSizeCost;

    assert (info_item_ptr->sizeCost <= 1000);
  }  else {
    info_item_ptr->sizeCost = 0;
    info_item_ptr->normSizeCost = 0;
  }

  return(ST_CONTINUE);
}

/**Function********************************************************************

  Synopsis    [Frees the memory used for info items.]

  Description []

  SideEffects [None]

******************************************************************************/

static enum st_retval
ProfileInfoItemFree(
  char * key,
  char * value,
  char * arg
)
{
  struct profile_info_item *info_item_ptr;

  info_item_ptr = (struct profile_info_item *)value;
  Pdtutil_Free(info_item_ptr);

  return(ST_CONTINUE);
}

/**Function********************************************************************

  Synopsis    [Recursive stap of AndAbstract with profile handling.]

  Description [Takes the AND of two BDDs and simultaneously abstracts
    the variables in cube. The variables are existentially abstracted.
    Returns a pointer to the result is successful; NULL otherwise.
    Profile statistics are upgraded. Profile is associated to the g operand.]

  SideEffects [None]

  SeeAlso     [Cudd_bddAndAbstract]

******************************************************************************/

static DdNode *
cuplusProfileAndAbstractRecur (
  DdManager *manager,
  DdNode *f,
  DdNode *g,
  DdNode *cube,
  cuplus_profile_info_t *profile
  )
{
  DdNode *F, *fv, *fnv, *G, *gv, *gnv;
  DdNode *one, *zero, *r, *t, *e, *Cube;
  unsigned int topf, topg, topcube, top, index, table2Index=0;
  struct profile_info_item *info_item_ptr;
  unsigned char fg_swap;
  unsigned int keys0;

  st_table *table = profile->table;
  /*
  profile_info_item2_t *table2 = profile->table2;
  */

  one = DD_ONE(manager);
  zero = Cudd_Not(one);

  /* Terminal cases. */
  if (f == zero || g == zero || f == Cudd_Not(g)) {
    return (zero);
  }

  if (f == one && g == one) {
    return(one);
  }

/*
  handled as AndAbstractRecur
  if (cube == one) {
    return(cuddBddAndRecur(manager, f, g));
  }
*/

  if (g == one) {
    /* g constant: nothing to profile */
    return (cuddBddExistAbstractRecur(manager, f, cube));
  }

  F = Cudd_Regular(f);
  G = Cudd_Regular(g);

  /*
   *  Profile By Variable Section
   */

  if (profile->dac99Compatible == 2) {
    /* Get element of table2 on which act in parallel on info_item_ptr */
    table2Index = vet2mat (profile->currentPart, Cudd_NodeReadIndex (g),
      profile->nVar);
  }

  if (!st_lookup(table, (char *)g, (char **)&info_item_ptr)) {
    info_item_ptr = Pdtutil_Alloc(struct profile_info_item,1);
    if (info_item_ptr == NULL) {
      return(NULL);
    }

    if (st_add_direct(table,(char *)g,(char *)info_item_ptr)==ST_OUT_OF_MEM) {
      Pdtutil_Free(info_item_ptr);
      return(NULL);
    }

    info_item_ptr->profileSubtree = 0;
    info_item_ptr->cacheHits = 0;
    info_item_ptr->nodeVisitCnt = 0;
    info_item_ptr->ref_node = g;
    info_item_ptr->pruned_bdd = NULL;
    info_item_ptr->sizeCost = 0;
    info_item_ptr->num = (profile->totalNodes)++;
  }

  keys0 = MANAGER_KEYS(manager,profile);

  if (f == one || f == g) {
    /* g must be all profiled */
    info_item_ptr->profileSubtree++;
    r = cuddBddExistAbstractRecur (manager, g, cube);
    info_item_ptr->sizeCost += (MANAGER_KEYS(manager,profile) - keys0);

    if (profile->dac99Compatible == 2) {
      profile->table2[table2Index].sizeCost +=
        (MANAGER_KEYS(manager,profile) - keys0);
    }

    return (r);
  }

  /* At this point f and g are not constant. */

  fg_swap = 0;

  if (profile->dac99Compatible) {
  /* disabled because profiling is on g */
  } else {
    if (f < g) { /* Try to increase cache efficiency. */
      fg_swap = 1;
    }
  }

  /* Check cache. */
  if (fg_swap)
    r = cuddCacheLookup(manager, DD_BDD_AND_ABSTRACT_TAG, g, f, cube);
  else
    r = cuddCacheLookup(manager, DD_BDD_AND_ABSTRACT_TAG, f, g, cube);

  if (r != NULL) {
    if (r != zero)
      info_item_ptr->cacheHits++;
      if (profile->dac99Compatible == 2) {
        profile->table2[table2Index].cacheHits++;
      }
      return (r);
  }

  /*
   *  Here we can skip the use of cuddI, because the operands are known
   *  to be non-constant.
   */

  topf = manager->perm[F->index];
  topg = manager->perm[G->index];
  top = ddMin(topf, topg);

  if (cube != one) {
    topcube = manager->perm[cube->index];
    if (topcube < top) {
      return(cuplusProfileAndAbstractRecur(manager,
        f, g, cuddT(cube), profile));
    }
  } else
    topcube = one->index;
    /* Now, topcube >= top. */

  if (topf == top) {
    index = F->index;
    fv = cuddT(F);
    fnv = cuddE(F);
    if (Cudd_IsComplement(f)) {
      fv = Cudd_Not(fv);
      fnv = Cudd_Not(fnv);
    }
  } else {
    index = G->index;
    fv = fnv = f;
  }

  if (topg == top) {
    gv = cuddT(G);
    gnv = cuddE(G);
    if (Cudd_IsComplement(g)) {
      gv = Cudd_Not(gv);
      gnv = Cudd_Not(gnv);
    }
  } else {
    gv = gnv = g;
  }

  if ((cube != one)&&(topcube == top)) {
    Cube = cuddT(cube);
  } else {
    Cube = cube;
  }

  if (fv == zero) {
    t = zero;
  } else {
    t = cuplusProfileAndAbstractRecur (manager, fv, gv, Cube, profile);
    if (t == NULL) {
      return(NULL);
    }
    if (profile->dac99Compatible == 2) {
      profile->table2[table2Index].nodeVisitCntThen++;
      profile->table2[table2Index].sizeCostThen +=
        (MANAGER_KEYS(manager,profile) - keys0);
    }
  }

#if 0
    /* Special case: 1 OR anything = 1. Hence, no need to compute
    ** the else branch if t is 1.
    */
    if (t == one && topcube == top) {
      info_item_ptr->nodeVisitCnt++;
      if (profile->dac99Compatible == 2) {
        profile->table2[table2Index].nodeVisitCnt++;
      }
      cuddCacheInsert(manager, DD_BDD_AND_ABSTRACT_TAG, f, g, cube, one);
      info_item_ptr->sizeCost += (MANAGER_KEYS(manager,profile) - keys0);
      if (profile->dac99Compatible == 2) {
        profile->table2[table2Index].sizeCost +=
          (MANAGER_KEYS(manager,profile) - keys0);
      }
      return(one);
    }
#endif

  cuddRef(t);

  if (fnv == zero) {
    e = zero;
  } else {
    e = cuplusProfileAndAbstractRecur(manager, fnv, gnv, Cube, profile);
    if (e == NULL) {
      Cudd_RecursiveDeref(manager, t);
      return(NULL);
    }
    if (profile->dac99Compatible == 2) {
      profile->table2[table2Index].nodeVisitCntElse++;
      profile->table2[table2Index].sizeCostElse +=
        (MANAGER_KEYS(manager,profile) - keys0);
    }
  }
  cuddRef(e);

  /* abstract */
  if (topcube == top) {
    r = cuddBddAndRecur(manager, Cudd_Not(t), Cudd_Not(e));
    if (r == NULL) {
      Cudd_RecursiveDeref(manager, t);
      Cudd_RecursiveDeref(manager, e);
      return(NULL);
    }
    r = Cudd_Not(r);
    cuddRef(r);
    Cudd_RecursiveDeref(manager, t);
    Cudd_RecursiveDeref(manager, e);
    cuddDeref(r);
  } else
    if (t == e) {
      r = t;
      cuddDeref(t);
      cuddDeref(e);
    } else {
      if (Cudd_IsComplement(t)) {
         r = cuddUniqueInter(manager,(int)index,Cudd_Not(t),Cudd_Not(e));
         if (r == NULL) {
           Cudd_RecursiveDeref(manager, t);
           Cudd_RecursiveDeref(manager, e);
           return(NULL);
         }
         r = Cudd_Not(r);
      } else {
         r = cuddUniqueInter(manager,(int)index,t,e);
         if (r == NULL) {
           Cudd_RecursiveDeref(manager, t);
           Cudd_RecursiveDeref(manager, e);
           return(NULL);
         }
      }
      cuddDeref(e);
      cuddDeref(t);
    }

  if (fg_swap)
    cuddCacheInsert(manager, DD_BDD_AND_ABSTRACT_TAG, g, f, cube, r);
  else
    cuddCacheInsert(manager, DD_BDD_AND_ABSTRACT_TAG, f, g, cube, r);

  info_item_ptr->sizeCost += (MANAGER_KEYS(manager,profile) - keys0);
  if (profile->dac99Compatible == 2) {
    profile->table2[table2Index].sizeCost +=
      (MANAGER_KEYS(manager,profile) - keys0);
  }

#if 1
  info_item_ptr->nodeVisitCnt++;
  if (profile->dac99Compatible == 2) {
    profile->table2[table2Index].nodeVisitCnt++;
  }
#else
  if (r!=zero) {
    info_item_ptr->nodeVisitCnt++;
    if (profile->dac99Compatible == 2) {
      profile->table2[table2Index].nodeVisitCnt++;
    }
  }
#endif

  return (r);
}

/**Function********************************************************************

  Synopsis    [Recursive step of BDD pruning based on profiles.]

  Description [Several heuristics are allowed to bias the pruning/clipping
    process:
    <ul>
    <li>1: recursions
    <li>2: recursions on active nodes,
    <li>3: size
    <li>4: normalized sizelight (prune lighter branch, recur on heavier one)
    <li>5: normalized sizeheavy (prune heavier branch, recur on lighter),
    <li>6: sizelight (prune lighter branch, recur on heavier one),
    <li>7: sizeheavy (prune heavier branch, recur on lighter), ]
    <li>8: sizelight no rec. (prune lighter branch, don't recur on
    heavier one)
    <li>9: sizeheavy no rec. (prune heavier branch,
    don't recur on lighter).
    <\ul>
    ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static DdNode *
cuplusPruneProfiledRecur (
  DdManager *manager,
  DdNode *f,
  cuplus_profile_info_t *profile,
  Cuplus_PruneHeuristic_e pruneHeuristic,
  int threshold,
  int upperActivity
  )
{
  DdNode *F, *T, *E, *res, *res1, *res2, *one, *zero;
  struct profile_info_item *info_item_ptr, *info_item_ptr_t, *info_item_ptr_e;
  int doSizePruning = 0;

  st_table *table = profile->table;

  one = DD_ONE (manager);
  zero = Cudd_Not (one);

  if (f == zero || f == one) {
    return (f);
  }

  if (pruneHeuristic == Cuplus_None) {
    return (f);
  }

  F = Cudd_Regular (f);
  if (!st_lookup (table, (char *) f, (char **) &info_item_ptr)) {
    /* Not Profiled: Return zero if only active recursions required */
    if (pruneHeuristic == Cuplus_ActiveRecursions) {
      return (zero);
    } else {
      return (f);
    }
  }

  /* From now on, the f top node is profiled */

  /*
   * Check if already computed: no cache table used,
   * caching done in profile info.
   */

  if (info_item_ptr->pruned_bdd != NULL)
    return (info_item_ptr->pruned_bdd);

  /* not cached: check for pruning */

  upperActivity += info_item_ptr->profileSubtree;
  upperActivity += info_item_ptr->cacheHits;

  /* full subtree profiled: decrease threshold for recursions */

  switch (pruneHeuristic) {

    case Cuplus_RecursionsWithSharing:

#if 0
  if (pruneHeuristic == Cuplus_RecursionsWithSharing) {
    if ((info_item_ptr->nodeVisitCnt+upperActivity)*100 <=
      threshold /* *info_item_ptr->sharing*/
      /*(profile->sizeByVar[F->index])*/ ) {
#if 1
          info_item_ptr->pruned_bdd = zero;
#endif
          return zero;
    }
  }
#endif

    case Cuplus_Recursions:
    case Cuplus_ActiveRecursions:
      if ((info_item_ptr->nodeVisitCnt+upperActivity) <= threshold) {
        return (zero);
      }
      if (info_item_ptr->profileSubtree > 0) {
        /* whole BDD profiled - threshold already checked - NO recursion ! */
        return (f);
      }
      break;
    case Cuplus_NormSizePrune:
      if (info_item_ptr->normSizeCost > threshold) {
        return (zero);
      }
      break;
    case Cuplus_NormSizePruneLight:
    case Cuplus_NormSizePruneHeavy:
      if (profile->dac99Compatible) {
        if (info_item_ptr->nodeVisitCnt > 0)
          if (info_item_ptr->normSizeCost > threshold)
            doSizePruning = 1;
      } else {
        if (info_item_ptr->normSizeCost > threshold) {
          doSizePruning = 1;
        }
      }
      break;
    case Cuplus_SizePrune:
      if (info_item_ptr->normSizeCost > threshold) {
        return (zero);
      }
      break;
    case Cuplus_SizePruneLight:
    case Cuplus_SizePruneHeavy:
    case Cuplus_SizePruneLightNoRecur:
    case Cuplus_SizePruneHeavyNoRecur:
      if (profile->dac99Compatible) {
        if (info_item_ptr->nodeVisitCnt > 0)
          if (info_item_ptr->sizeCost > threshold)
            doSizePruning = 1;
      }
      else
        if (info_item_ptr->sizeCost > threshold)
          doSizePruning = 1;
      break;
    default:
      Pdtutil_Assert (0, "Wrong switch in Profile Pruning");
  }

  if (profile->dac99Compatible) {
    if (info_item_ptr->profileSubtree > 0) {
      /* whole BDD profiled - threshold already checked - NO more ! */
      return (f);
    }
  }

  /* Compute the cofactors of f and recur */

  T = cuddT (F);
  E = cuddE (F);
  if (f != F) {
    T = Cudd_Not(T); E = Cudd_Not(E);
  }

  if (((profile->dac99Compatible)
    && (doSizePruning) && (T != one) && (E != one)) ||
     ((!profile->dac99Compatible)
    && (doSizePruning)&&(!Cudd_IsConstant(T))&&(!Cudd_IsConstant(E)))) {

    if ((st_lookup(table, (char *)T, (char **)&info_item_ptr_t)) &&
        (st_lookup(table, (char *)E, (char **)&info_item_ptr_e))) {

      switch (pruneHeuristic) {
        case Cuplus_NormSizePruneLight:
          if (info_item_ptr_t->normSizeCost >
              info_item_ptr_e->normSizeCost)
            E = zero;
          else
            T = zero;
          break;
        case Cuplus_NormSizePruneHeavy:
          if (info_item_ptr_t->normSizeCost >
              info_item_ptr_e->normSizeCost)
            T = zero;
          else
            E = zero;
          break;
        case Cuplus_SizePruneLight:
          if (info_item_ptr_t->sizeCost > info_item_ptr_e->sizeCost)
            E = zero;
          else
            T = zero;
          break;
        case Cuplus_SizePruneLightNoRecur:
          if (info_item_ptr_t->sizeCost > info_item_ptr_e->sizeCost)
            E = zero;
          else
            T = zero;
          pruneHeuristic = Cuplus_None;
          break;
        case Cuplus_SizePruneHeavy:
          if (info_item_ptr_t->sizeCost > info_item_ptr_e->sizeCost)
            T = zero;
          else
            E = zero;
          break;
        case Cuplus_SizePruneHeavyNoRecur:
          if (info_item_ptr_t->sizeCost > info_item_ptr_e->sizeCost)
            T = zero;
          else
            E = zero;
          pruneHeuristic = Cuplus_None;
          break;
        default:
          Pdtutil_Assert(1,"Wrong switch in Profile Pruning");
      }

    }
  }

  /* recur on cofactors */

  res1 = cuplusPruneProfiledRecur(manager, T, profile,
    pruneHeuristic, threshold, upperActivity);
  if (res1 == NULL) {
    return(NULL);
  }
  cuddRef(res1);

  res2 = cuplusPruneProfiledRecur(manager, E, profile,
    pruneHeuristic, threshold, upperActivity);
  if (res2 == NULL) {
    Cudd_IterDerefBdd (manager, res1);
    return (NULL);
  }
  cuddRef(res2);

  /* ITE takes care of possible complementation of res1 */

  res = cuddBddIteRecur (manager, Cudd_bddIthVar (manager, F->index), res1, res2);
  if (res == NULL) {
    Cudd_IterDerefBdd(manager, res1);
    Cudd_IterDerefBdd(manager, res2);
    return(NULL);
  }

  cuddDeref(res1);
  cuddDeref(res2);
  if (F->ref != 1) {
    info_item_ptr->pruned_bdd = res;
  }

  return (res);
}

/**Function********************************************************************

  Synopsis    [Pruning Based on Profiles By Variable.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static DdNode *
cuplusPruneProfiledByVar (
  DdManager *manager,
  DdNode *f,
  cuplus_profile_info_t *profile,
  Cuplus_PruneHeuristic_e pruneHeuristic,
  int threshold
  )
{
  int i, j, nVar, index;
  DdNode *varToAnd, *fTmp1, *fTmp2;

  nVar = profile->nVar;
  i = profile->currentPart;
  fTmp1 = f;
  Cudd_Ref (fTmp1);

  for (j=0; j<nVar; j++) {
    index = vet2mat (i, j, nVar);

#if COUNTER_COST
    if (profile->table2[index].sizeCostNorm > threshold) {
#else
    if (profile->table2[index].nodeVisitCnt > threshold) {
#endif
      /*
       *  Prune Partition (nPart on Variable nVar)
       */

      /* Get Variable From Index */
      varToAnd = Cudd_bddIthVar (manager, j);
      Cudd_Ref (varToAnd);

#if COUNTER_COST
      if (profile->table2[index].sizeCostElseNorm >
        profile->table2[index].sizeCostThenNorm) {
#else
      if (profile->table2[index].nodeVisitCntElse >
        profile->table2[index].nodeVisitCntThen) {
#endif
        varToAnd = Cudd_Not (varToAnd);
      }

      fTmp2 = Cudd_bddAnd (manager, fTmp1, varToAnd);
      Cudd_Ref (fTmp2);
      Cudd_RecursiveDeref (manager, varToAnd);

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "{[(AND: %d->%d)]} ", Cudd_DagSize (fTmp1),
        Cudd_DagSize (fTmp2))
      );

      Cudd_RecursiveDeref (manager, fTmp1);
      fTmp1 = fTmp2;
    }
  }

  return (fTmp1);
}

/**Function********************************************************************

  Synopsis    [DFV visit with statistics upgrading.]

  Description [This is rather a template for BDD visit. Only BDD size by
    variable index is collected so far.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static void
cuplusCollectProfileSummaryRecur(
  DdManager *manager,
  DdNode *f,
  cuplus_profile_info_t *profile
  )
{
  struct profile_info_item *info_item_ptr;
  DdNode *fInt;
  st_table *table=profile->table;

  fInt = Cudd_Regular (f);

  if (Cudd_IsComplement (fInt->next)) {
    return;
  }

  fInt->next = Cudd_Not (fInt->next);
  if (cuddIsConstant (fInt)) {
    return;
  }

  if (!st_lookup (table, (char *) fInt, (char **) &info_item_ptr)) {
    /* Not profiled: return */
    return;
  }

#if 0
  profile->sizeByVar[fInt->index]++;
#endif

  cuplusCollectProfileSummaryRecur (manager, cuddT (fInt), profile);
  cuplusCollectProfileSummaryRecur (manager, Cudd_Regular (cuddE (fInt)),
    profile);

  return;
}

/**Function********************************************************************

  Synopsis    [Undocumented]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

static void
BfvProfiles (
  DdManager *manager,
  cuplus_profile_info_t *profile
  )
{
  struct profile_info_item *info_item_ptr, *info_item_ptr_t, *info_item_ptr_e;
  struct profile_info_item **info_itemsByVar;
  DdNode *f1, *f2, *t, *e;
  int i, j, nvars;
  st_table * table=profile->table;

  nvars = Cudd_ReadSize (manager);

  if ((info_itemsByVar = Pdtutil_Alloc (struct profile_info_item *, nvars)) == NULL) {
    Pdtutil_Warning (1, "Out of memory visiting profiles.");
    return;
  }

  for (i=0; i<nvars; i++) {
    info_itemsByVar[i] = NULL;
  }

  st_foreach (table, ProfileInfoByVars, (char *) info_itemsByVar);

  for (i=0; i<nvars; i++) {
    j = Cudd_ReadInvPerm (manager, i);
    for (info_item_ptr = info_itemsByVar[j]; info_item_ptr != NULL;
      info_item_ptr = info_item_ptr->next) {

      /* visit items */

      f1 = info_item_ptr->ref_node;
      f2 = Cudd_Regular (f1);

      t = cuddT (f2);
      e = cuddE (f2);
      if (Cudd_IsComplement (f1)) {
        t = Cudd_Not (t);
        e = Cudd_Not (e);
      }

      info_item_ptr->totAct += info_item_ptr->profileSubtree;
      info_item_ptr->totAct += info_item_ptr->cacheHits;

      if (info_item_ptr->sharing == 0) {
        info_item_ptr->sharing = 1;
      }

      if (st_lookup (table, (char *) t, (char **) &info_item_ptr_t)) {
        if (info_item_ptr_t->totAct < info_item_ptr->totAct) {
          info_item_ptr_t->totAct = info_item_ptr->totAct;
        }
        info_item_ptr_t->sharing++;
      }

      if (st_lookup(table, (char * )e, (char **) &info_item_ptr_e)) {
        if (info_item_ptr_e->totAct < info_item_ptr->totAct) {
          info_item_ptr_e->totAct = info_item_ptr->totAct;
        }
        info_item_ptr_e->sharing++;
      }

      /* end visit */
    }
  }

  return;
}

/**Function********************************************************************

  Synopsis    [Undocumented]

  Description []

  SideEffects [None]

******************************************************************************/

static enum st_retval
ProfileInfoByVars(
  char * key,
  char * value,
  char * arg
)
{
  struct profile_info_item *info_item_ptr, **info_itemsByVar;
  DdNode *f;
  info_itemsByVar = (struct profile_info_item **)arg;
  info_item_ptr = (struct profile_info_item *)value;

  f = Cudd_Regular(info_item_ptr->ref_node);
  info_item_ptr->next = info_itemsByVar[f->index];
  info_itemsByVar[f->index] = info_item_ptr;

  info_item_ptr->totAct = 0;
  info_item_ptr->sharing = 0;

  return(ST_CONTINUE);
}

/**Function********************************************************************

  Synopsis    [Performs a DFS from f, clearing the LSB of the next
    pointers.]

  Description []

  SideEffects [None]

  SeeAlso     [ddSupportStep ddDagInt]

******************************************************************************/

static void
ddClearFlag(
  DdNode * f)
{
    if (!Cudd_IsComplement(f->next)) {
        return;
    }
    /* Clear visited flag. */
    f->next = Cudd_Regular(f->next);
    if (cuddIsConstant(f)) {
        return;
    }
    ddClearFlag(cuddT(f));
    ddClearFlag(Cudd_Regular(cuddE(f)));
    return;

}

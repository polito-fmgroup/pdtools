/**CHeaderFile*****************************************************************

  FileName    [fbvInt.h]

  PackageName [fbv]

  Synopsis    [Internal header file for fbv]

  Description []
              
  SeeAlso     []  

  Author    [Gianpiero Cabodi and Stefano Quer]

  Copyright [This file was created at the Politecnico di Torino,
    Torino, Italy.
    The  Politecnico di Torino makes no warranty about the suitability of 
    this software for any purpose.  
    It is presented on an AS IS basis.]
  
  Revision    []

******************************************************************************/

#ifndef _FBVINT
#define _FBVINT

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include "pdtutil.h"
#include "tr.h"
#include "trav.h"
#include "fsm.h"
#include "mcInt.h"
#include "fbv.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

typedef enum {
  Fbv_Mult_c,
  Fbv_None_c
} Fbv_Comb_Check_e;

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

struct Fbv_DecompInfo_t {
  Ddi_Bddarray_t *pArray;
  int *partOrdId;
  Ddi_Varsetarray_t *coiArray;
  Ddi_Varset_t *hintVars;
};

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

EXTERN Fbv_Globals_t *FbvParseArgs(
  int argc,
  char *argv[],
  Fbv_Globals_t * opt
);
EXTERN int FbvTwoPhaseReduction(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt,
  int do_find,
  int do_red
);
EXTERN Ddi_Bdd_t *FbvApproxPreimg(
  Tr_Tr_t * TR,
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * window,
  int doApprox,
  Fbv_Globals_t * opt
);
EXTERN int main(
  int argc,
  char *argv[]
);
EXTERN DdNode *fbvBddExistAbstractWeakRecur(
  DdManager * manager,
  DdNode * f,
  DdNode * cube,
  st_table * table,
  int split,
  int splitPhase,
  int phase,
  Fbv_Globals_t * opt
);
EXTERN Ddi_Bdd_t *FbvEvalEG(
  Tr_Tr_t * tr,
  Ddi_Bdd_t * opBdd,
  Fbv_Globals_t * opt
);
EXTERN int fbvAbcLink(
  char *fileNameR,
  char *fileNameW,
  int initialOpt,
  int optLevel,
  int seqOpt
);
EXTERN Ddi_Bdd_t *FbvAigBckVerif(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int maxIter,
  Fbv_Globals_t * opt
);
EXTERN void FbvAigFwdVerif(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Fbv_Globals_t * opt
);
EXTERN Ddi_Bdd_t *FbvAigBckVerif2(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Fbv_Globals_t * opt
);
EXTERN void FbvAigBmcVerif(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int bound,
  int step,
  Fbv_Globals_t * opt
);
EXTERN Ddi_Bdd_t *FbvAigFwdApproxTrav(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invar,
  int step,
  Fbv_Globals_t * opt
);
EXTERN void FbvAigBmcTrVerif(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int bound,
  int step,
  Fbv_Globals_t * opt
);
EXTERN void FbvAigInductiveTrVerif(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int step,
  Fbv_Globals_t * opt
);
EXTERN void FbvLemmaReduction(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  int lemmasSteps,
  char *fsmName
);
EXTERN void FbvCombCheck(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Comb_Check_e check
);
EXTERN Ddi_Bdd_t *FbvBckInvarCheckRecur(
  Ddi_Bdd_t * from,
  Ddi_Bdd_t * target,
  Tr_Tr_t * TR,
  Ddi_Bdd_t * rings,
  Fbv_Globals_t * opt
);
EXTERN int FbvCheckSetIntersect(
  Ddi_Bdd_t * set,
  Ddi_Bdd_t * target,
  Ddi_Bdd_t * care,
  Ddi_Bdd_t * care2,
  int doAnd,
  Fbv_Globals_t * opt
);
EXTERN Tr_Tr_t *FbvTrProjection(
  Tr_Tr_t * tr,
  Ddi_Bdd_t * spec
);
EXTERN int FbvTargetEn(
  Fsm_Mgr_t * fsmMgr,
  int nIter
);
EXTERN Ddi_Bdd_t *FbvGetSpec(
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * partInvarspec,
  Fbv_Globals_t * opt
);
EXTERN void FbvSetMgrOpt(
  Mc_Mgr_t * mcMgr,
  Fbv_Globals_t * opt,
  int phase
);
EXTERN void FbvSetDdiMgrOpt(
  Ddi_Mgr_t * ddiMgr,
  Fbv_Globals_t * opt,
  int phase
);
EXTERN void FbvSetFsmMgrOpt(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt,
  int phase
);
EXTERN Fsm_Mgr_t *FbvFsmMgrLoad(
  Fsm_Mgr_t * fsmMgr,
  char *fsm,
  Fbv_Globals_t * opt
);
EXTERN int FbvFsmReductions(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt,
  int phase
);
EXTERN int FbvTestSec(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt
);
EXTERN int FbvTestRelational(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt
);
EXTERN int FbvConstrainInitStub(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * invar
);
EXTERN int FbvSelectHeuristic(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt,
  int phase
);
EXTERN void FbvSetHeuristicOpt(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt,
  int heuristic
);
EXTERN void FbvFsmMgrLoadRplus(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt
);
EXTERN void FbvSetTravMgrOpt(
  Trav_Mgr_t * travMgr,
  Fbv_Globals_t * opt
);
EXTERN void FbvSetTravMgrFsmData(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr
);
EXTERN void FbvSetTrMgrOpt(
  Tr_Mgr_t * trMgr,
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt,
  int phase
);
EXTERN int FbvFsmCheckComposePi(
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr
);
EXTERN int FbvApproxForwExactBackwVerify(
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  int doFwd,
  Fbv_Globals_t * opt
);
EXTERN int FbvApproxForwExactBckFixPointVerify(
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  Ddi_Bddarray_t * delta,
  Fbv_Globals_t * opt
);
EXTERN int FbvExactForwVerify(
  Trav_Mgr_t * travMgr,
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bddarray_t * delta,
  float timeLimit,
  int bddNodeLimit,
  Fbv_Globals_t * opt
);

EXTERN int FbvThrdVerif(
  char *fsm,
  char *ord,
  Fbv_Globals_t * opt
);
EXTERN Fbv_DecompInfo_t *FbvPropDecomp(
  Fsm_Mgr_t * fsmMgr,
  Fbv_Globals_t * opt
);

EXTERN Pdtutil_OptList_t *FbvOpt2OptList(
  Fbv_Globals_t * opt
);
EXTERN void FbvWriteCex(
  char *fname,
  Fsm_Mgr_t * fsmMgr,
  Fsm_Mgr_t * fsmMgrOriginal
);
EXTERN void FbvWriteCexNormalized(
  char *fname,
  Fsm_Mgr_t * fsmMgr,
  Fsm_Mgr_t * fsmMgrOriginal,
  int badId
);
EXTERN Fbv_Globals_t *FbvDupSettings(
  Fbv_Globals_t * opt0
);
EXTERN void FbvFreeSettings(
  Fbv_Globals_t * opt
);
EXTERN void FbvTask(
  Fbv_Globals_t * opt
);

/**AutomaticEnd***************************************************************/

#endif                          /* _FBVINT */

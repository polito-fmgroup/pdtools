/**CFile***********************************************************************

  FileName    [fbvMc.c]

  PackageName [fbv]

  Synopsis    [Model check routines]

  Description []

  SeeAlso   []

  Author    [Gianpiero Cabodi and Stefano Quer]

  Copyright [This file was created at the Politecnico di Torino,
    Torino, Italy.
    The  Politecnico di Torino makes no warranty about the suitability of
    this software for any purpose.
    It is presented on an AS IS basis.
  ]

  Revision  []

******************************************************************************/

#include <signal.h>
#include "fbvInt.h"
#include "fsmInt.h"
#include "travInt.h"
#include "ddiInt.h"
#include "baigInt.h"
#include "trInt.h"
#include "expr.h"
#include "st.h"
#include "pdtutil.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

#define FILEPAT   "/tmp/pdtpat"
#define FILEINIT  "/tmp/pdtinit"

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#define LOG_CONST(f,msg) \
if (Ddi_BddIsOne(f)) {\
  fprintf(stdout, "%s = 1\n", msg);            \
}\
else if (Ddi_BddIsZero(f)) {\
  fprintf(stdout, "%s = 0\n", msg);            \
}

#define LOG_BDD_CUBE_PLA(f,str) \
\
    {\
      Ddi_Varset_t *supp;\
\
      fprintf(stdout, ".names ");            \
      if (f==NULL) \
        fprintf(stdout, "NULL!\n");\
      else if (Ddi_BddIsAig(f)) {\
        Ddi_Bdd_t *f1 = Ddi_BddMakeMono(f);\
        supp = Ddi_BddSupp(f1);\
        Ddi_VarsetPrint(supp, 10000, NULL, stdout);\
        fprintf(stdout, " %s", str);                        \
        Ddi_BddPrintCubes(f1, supp, 100, 1, NULL, stdout);\
        Ddi_Free(supp);\
        Ddi_Free(f1);\
      }\
      else if (Ddi_BddIsMono(f)&&Ddi_BddIsZero(f)) \
        fprintf(stdout, "ZERO!\n");\
      else if (Ddi_BddIsMono(f)&&Ddi_BddIsOne(f)) \
        fprintf(stdout, "ONE!\n");\
      else {\
        /* print support variables and cubes*/\
        supp = Ddi_BddSupp(f);\
        Ddi_VarsetPrint(supp, 10000, NULL, stdout);\
        fprintf(stdout, " %s", str);                        \
        Ddi_BddPrintCubes(f, supp, 100, 1, NULL, stdout);\
        Ddi_Free(supp);\
      }\
    }

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static Ddi_Bdd_t *findNonVacuumHint(
  Ddi_Bdd_t * from,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * pi,
  Ddi_Vararray_t * aux,
  Ddi_Bdd_t * reached,
  Ddi_Bddarray_t * delta,
  Ddi_Bdd_t * trBdd,
  Ddi_Bdd_t * hConstr,
  Ddi_Bdd_t * prevHint,
  int nVars,
  int hintsStrategy
);
static int FbvBddCexToFsm(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * cex,
  Ddi_Bdd_t * initState,
  int reverse
);
static Ddi_Bdd_t *diffProjectedSetAcc(
  Ddi_Bdd_t * f,
  Ddi_Bdd_t * g,
  Ddi_Var_t * partVar
);
static Ddi_Bdd_t *orProjectedSetAcc(
  Ddi_Bdd_t * f,
  Ddi_Bdd_t * g,
  Ddi_Var_t * partVar
);
static int travOverRingsWithFixPoint(
  Ddi_Bdd_t * from0,
  Ddi_Bdd_t * reached0,
  Ddi_Bdd_t * target,
  Tr_Tr_t * TR,
  Ddi_Bdd_t * inRings,
  Ddi_Bdd_t * outRings,
  Ddi_Bdd_t * overApprox,
  int maxIter,
  char *iterMsg,
  long initialTime,
  int reverse,
  int approx,
  Fbv_Globals_t * opt
);
static int travOverRings(
  Ddi_Bdd_t * from0,
  Ddi_Bdd_t * reached0,
  Ddi_Bdd_t * target,
  Tr_Tr_t * TR,
  Ddi_Bdd_t * inRings,
  Ddi_Bdd_t * outRings,
  Ddi_Bdd_t * overApprox,
  int maxIter,
  char *iterMsg,
  long initialTime,
  int reverse,
  int approx,
  Fbv_Globals_t * opt
);
static void storeCNFRings(
  Ddi_Bdd_t * fwdRings,
  Ddi_Bdd_t * bckRings,
  Ddi_Bdd_t * TrBdd,
  Ddi_Bdd_t * prop,
  Fbv_Globals_t * opt
);
static Ddi_Bdd_t *dynAbstract(
  Ddi_Bdd_t * f,
  int maxSize,
  int maxAbstract
);
static void storeRingsCNF(
  Ddi_Bdd_t * rings,
  Fbv_Globals_t * opt
);


/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of external functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************
  Synopsis    [exact forward verification]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
FbvExactForwVerify(
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
)
{
  int i, verifOK, travAgain;
  Ddi_Bdd_t *from, *fromCube = NULL, *reached, *to;
  Ddi_Vararray_t *ps, *ns, *pi, *aux;
  Ddi_Varset_t *nsv, *auxv = NULL, *nsauxv = NULL;
  long initialTime, currTime, refTime, imgSiftTime, maxTime = -1;
  Tr_Tr_t *tr1, *tr2;
  Ddi_Bddarray_t *frontier;
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);

  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(init);
  Ddi_Var_t *clock_var = NULL;
  int metaFixPoint = 0, optFixPoint = 0;
  int initAig = 1;
  int currHintId = 0;
  Ddi_Bdd_t *hConstr = NULL;
  int hintsAgain = 0, useHints = (opt->trav.autoHint > 0);
  int hintsTh = opt->trav.hintsTh;
  int enHints = 0;
  int hintsStrategy = opt->trav.hintsStrategy;
  int maxHintIter = -1;
  int useHintsAsConstraints = 0;
  Pdtutil_VerbLevel_e verbosityDd = Ddi_MgrReadVerbosity(ddiMgr);
  Pdtutil_VerbLevel_e verbosityTr = Tr_MgrReadVerbosity(Tr_TrMgr(tr));
  Ddi_Bddarray_t *myDeltaAig = NULL;
  int satFpChk = 0;

  if (travMgr != NULL) {
    if (bddNodeLimit <= 0) {
      bddNodeLimit = travMgr->settings.bdd.bddNodeLimit;
    }
    if (bddNodeLimit >= 0) {
      travMgr->settings.bdd.bddNodeLimit = bddNodeLimit;
    }

    if (timeLimit <= 0) {
      timeLimit = travMgr->settings.bdd.bddTimeLimit;
    }
    if (timeLimit >= 0) {
      travMgr->settings.bdd.bddTimeLimit = initialTime + timeLimit * 1000;
    }
  }

  if (timeLimit > 0) {
    maxTime = timeLimit * 1000;
  }
  initialTime = util_cpu_time();

  pi = Tr_MgrReadI(Tr_TrMgr(tr));
  ps = Tr_MgrReadPS(Tr_TrMgr(tr));
  ns = Tr_MgrReadNS(Tr_TrMgr(tr));
  aux = Tr_MgrReadAuxVars(Tr_TrMgr(tr));

  nsv = Ddi_VarsetMakeFromArray(ns);
  if (aux != NULL) {
    auxv = Ddi_VarsetMakeFromArray(aux);
    nsauxv = Ddi_VarsetUnion(nsv, auxv);
  } else {
    nsauxv = Ddi_VarsetDup(nsv);
  }

  Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMed_c) {
    fprintf(stdout, "-- Exact FORWARD.\n");
  }

#if 0
  invarspec = Ddi_MgrReadOne(Ddi_ReadMgr(init));    /*@@@ */
#endif

  Pdtutil_VerbosityMgrIf(travMgr, Pdtutil_VerbLevelUsrMed_c) {
    fprintf(stdout, "|Invar|=%d; ", Ddi_BddSize(invar));
    fprintf(stdout, "|Tr|=%d.\n", Ddi_BddSize(Tr_TrBdd(tr)));
  }

  if (opt->common.clk != NULL) {

    clock_var = Tr_TrSetClkStateVar(tr, opt->common.clk, 1);
    if (opt->common.clk != Ddi_VarName(clock_var)) {
      Ddi_Bdd_t *lit = Ddi_BddMakeLiteral(clock_var, 0);

      opt->common.clk = Ddi_VarName(clock_var);
      Ddi_BddAndAcc(init, lit);
      Ddi_Free(lit);
    }

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c, if (clock_var != NULL) {
      fprintf(stdout, "Clock Variable (%s) Detected.\n", opt->common.clk);}
    ) ;

#if 0
    {
      Ddi_Varset_t *reducedVars = Ddi_VarsetMakeFromArray(ps);
      Ddi_Bddarray_t *l = Ddi_BddarrayAlloc(ddiMgr, 0);

      Ddi_BddarrayInsert(l, 0, invarspec);
      Tr_TrReduceMasterSlave(tr, reducedVars, l, clock_var);
      Ddi_Free(reducedVars);
      DdiGenericDataCopy((Ddi_Generic_t *) invarspec,
        (Ddi_Generic_t *) Ddi_BddarrayRead(l, 0));
      Ddi_Free(l);
    }
#endif

  }
#if 0
  WindowPart(Ddi_BddPartRead(Tr_TrBdd(tr), 0, opt));
#endif

  if (0 /*opt->trav.fbvMeta */ ) {
    Ddi_Bdd_t *trAux, *trBdd;
    Ddi_Bddarray_t *dc;
    int i, n, j;
    Tr_Mgr_t *trMgr = Tr_TrMgr(tr);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "Transition Relation Monolitic (%d nodes).\n",
        Ddi_BddSize(Tr_TrBdd(tr))));

    Tr_TrSortIwls95(tr);
    Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
    Tr_TrSetClustered(tr);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "Transiotion Relation Clustered (%d nodes).\n",
        Ddi_BddSize(Tr_TrBdd(tr))));

    Ddi_BddSetMeta(Tr_TrBdd(tr));
#if 1
    Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh * 10);
    Tr_TrSetClustered(tr);
    Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
#endif

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Transition Relation Meta (%d nodes).\n",
        Ddi_BddSize(Tr_TrBdd(tr))));

    /*exit (1); */
    trAux = Ddi_BddMakePartConjVoid(Ddi_ReadMgr(Tr_TrBdd(tr)));
    trBdd = Tr_TrBdd(tr);
    for (j = 0; j < Ddi_BddPartNum(trBdd); j++) {
      Ddi_BddMetaBfvSetConj(Ddi_BddPartRead(Tr_TrBdd(tr), j));
    }
#if 0
    Ddi_BddSetMono(Ddi_BddPartRead(trBdd, 0));
    Ddi_BddSetMono(trBdd);
    tr1 = Tr_TrDup(tr);
#else
    dc = Ddi_BddPartRead(trBdd, 0)->data.meta->dc;
    n = Ddi_BddarrayNum(dc);
    for (i = 0; i < n - 1; i++) {
      for (j = 0; j < Ddi_BddPartNum(trBdd); j++) {
        dc = Ddi_BddPartRead(trBdd, j)->data.meta->dc;
        Ddi_BddPartInsert(trAux, 0, Ddi_BddarrayRead(dc, i));
      }
    }
    for (j = 0; j < Ddi_BddPartNum(trBdd); j++) {
      dc = Ddi_BddPartRead(trBdd, j)->data.meta->one;
      Ddi_BddPartInsert(trAux, 0, Ddi_BddarrayRead(dc, i));
    }
    tr1 = Tr_TrMakeFromRel(trMgr, trAux);
    Tr_TrSetClustered(tr1);
#endif

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Transition Relation (%d nodes - #part %d).\n",
        Ddi_BddSize(trAux), Ddi_BddPartNum(trAux)));

    Ddi_Free(trAux);
    opt->trav.fbvMeta = 1;
  } else {
    tr1 = Tr_TrDup(tr);
  }


  if (1 || Ddi_BddIsPartConj(Tr_TrBdd(tr)) /*!opt->trav.fbvMeta */ ) {
    Tr_MgrSetClustThreshold(trMgr, 1);
    Tr_TrSetClustered(tr);
    Tr_TrSortIwls95(tr);
    Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
    Ddi_BddSetFlattened(Tr_TrBdd(tr));
    Tr_TrSetClustered(tr);
    //    Tr_TrSortIwls95(tr);
  }

  Tr_MgrSetClustSmoothPi(trMgr, 1);
  Tr_MgrSetClustThreshold(trMgr, 1);
  if (0 /*opt->trav.fbvMeta */ ) {
    int j;
    Ddi_Bdd_t *trBdd;

    Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
    Tr_TrSetClustered(tr1);
    trBdd = Tr_TrBdd(tr1);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|TR_0| = %d ", Ddi_BddSize(Tr_TrBdd(tr1)));
      fprintf(stdout, "#Part_0 = %d\n", Ddi_BddPartNum(Tr_TrBdd(tr1)))
      );

    for (i = Ddi_BddPartNum(trBdd) - 1; i > 0; i--) {
      for (j = i - 1; j >= 0; j--) {
        Ddi_BddRestrictAcc(Ddi_BddPartRead(trBdd, j), Ddi_BddPartRead(trBdd,
            i));
      }
    }
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|Tr_1| = %d ", Ddi_BddSize(Tr_TrBdd(tr1)));
      fprintf(stdout, "#Part_1 = %d\n", Ddi_BddPartNum(Tr_TrBdd(tr1))));
    opt->trav.fbvMeta = 1;
  } else if (!Ddi_BddIsPartDisj(Tr_TrBdd(tr))) {
    Tr_TrSetClustered(tr1);
    Tr_TrSortIwls95(tr1);
  }
#if 0
  if (opt->trav.fbvMeta) {
    for (i = 0; i < Ddi_BddPartNum(Tr_TrBdd(tr1)); i++) {
      Ddi_BddSetMeta(Ddi_BddPartRead(Tr_TrBdd(tr1), i));
    }
  }
#endif
  Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
  Ddi_BddSetFlattened(Tr_TrBdd(tr1));
  Tr_TrSetClustered(tr1);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "|Tr| = %d ", Ddi_BddSize(Tr_TrBdd(tr1)));
    fprintf(stdout, "#Part = %d\n", Ddi_BddPartNum(Tr_TrBdd(tr1))));

#if 1
  if (opt->trav.trproj) {
    Tr_TrRepartition(tr1, opt->trav.clustTh);
    Tr_TrSetClustered(tr1);
    /*return(0); */
  }
#endif

  if (Ddi_BddIsPartConj(init)) {
    Ddi_Varset_t *sm = Ddi_BddSupp(init);
    Ddi_Varset_t *psvars = Ddi_VarsetMakeFromArray(Tr_MgrReadPS(trMgr));

    Ddi_VarsetDiffAcc(sm, psvars);
    Tr_TrBddSortIwls95(trMgr, init, sm, psvars);
    Ddi_BddSetClustered(init, opt->trav.clustTh);
    Ddi_BddExistProjectAcc(init, psvars);
    Ddi_Free(sm);
    Ddi_Free(psvars);
  }
  Ddi_BddSetMono(init);
  from = Ddi_BddDup(init);

  //  Ddi_BddAndAcc(from,invar);

  reached = Ddi_BddDup(from);
  if (1) {
    Ddi_Var_t *iv = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

    if (iv != NULL) {
      Ddi_Bdd_t *invarOut = Ddi_BddMakeLiteral(iv, 0);

      Ddi_BddOrAcc(reached, invarOut);
      Ddi_Free(invarOut);
    }
  }

  verifOK = 1;

  if (opt->trav.fbvMeta) {
#if 0
    Ddi_BddSetMeta(Tr_TrBdd(tr1));
    Tr_MgrSetClustThreshold(trMgr, 3 * opt->trav.clustTh);
    Tr_TrSetClustered(tr1);
#endif
    Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);

    Ddi_BddSetMeta(from);
    Ddi_BddSetMeta(reached);

  }

  if (opt->trav.constrain) {
    Tr_MgrSetImgMethod(Tr_TrMgr(tr), Tr_ImgMethodCofactor_c);
  }

  frontier = Ddi_BddarrayAlloc(Ddi_ReadMgr(init), 1);
  Ddi_BddarrayWrite(frontier, 0, from);

  if (opt->common.checkFrozen) {
    Tr_MgrSetCheckFrozenVars(Tr_TrMgr(tr));
  }

  if (opt->trav.wP != NULL) {
    char name[200];

    sprintf(name, "%s", opt->trav.wP);
    Tr_TrWritePartitions(tr, name);
  }
  if (opt->trav.wOrd != NULL) {
    char name[200];

    sprintf(name, "%s.0", opt->trav.wOrd);
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Writing intermediate variable order to file %s.\n",
        name));
    Ddi_MgrOrdWrite(ddiMgr, name, NULL, Pdtutil_VariableOrderDefault_c);
  }

  do {

    ddiMgr->exist.clipDone = travAgain = 0;

    if (0) {
      /* 24.08.2010 StQ Parte da Rivedere
         Occorre verificare che cosa calcolare a seconda del livello di verbosity */
      tr2 = Tr_TrPartition(tr1, "CLK", 0);
      if (Ddi_BddIsPartDisj(Tr_TrBdd(tr2))) {
        Ddi_Varset_t *psvars = Ddi_VarsetMakeFromArray(Tr_MgrReadPS(trMgr));
        Ddi_Varset_t *nsvars = Ddi_VarsetMakeFromArray(Tr_MgrReadNS(trMgr));
        Ddi_Varset_t *supp3;
        Ddi_Varset_t *supp = Ddi_BddSupp(Tr_TrBdd(tr2));
        Ddi_Varset_t *supp2 = Ddi_VarsetDup(supp);
        int totv = Ddi_VarsetNum(supp);
        int totp, totn;

        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
          Ddi_VarsetIntersectAcc(supp, psvars);
          Ddi_VarsetIntersectAcc(supp2, nsvars);
          totp = Ddi_VarsetNum(supp);
          totn = Ddi_VarsetNum(supp2);
          fprintf(stdout, "TR Variables: Tot: %d - PI: %d, PS: %d, NS: %d\n",
            totv, totv - (totp + totn), totp, totn);
          Ddi_Free(supp);
          Ddi_Free(supp2);
          supp = Ddi_BddSupp(Ddi_BddPartRead(Tr_TrBdd(tr2), 0));
          supp2 = Ddi_VarsetDup(supp);
          totv = Ddi_VarsetNum(supp);
          Ddi_VarsetIntersectAcc(supp, psvars);
          Ddi_VarsetIntersectAcc(supp2, nsvars);
          totp = Ddi_VarsetNum(supp);
          totn = Ddi_VarsetNum(supp2);
          fprintf(stdout, "TR0 Variables: Tot: %d - PI: %d, PS: %d, NS: %d\n",
            totv, totv - (totp + totn), totp, totn);
          supp3 = Ddi_VarsetDup(supp2);
          Ddi_Free(supp);
          Ddi_Free(supp2);
          supp = Ddi_BddSupp(Ddi_BddPartRead(Tr_TrBdd(tr2), 1));
          supp2 = Ddi_VarsetDup(supp);
          totv = Ddi_VarsetNum(supp);
          Ddi_VarsetIntersectAcc(supp, psvars);
          Ddi_VarsetIntersectAcc(supp2, nsvars);
          totp = Ddi_VarsetNum(supp);
          totn = Ddi_VarsetNum(supp2);
          fprintf(stdout, "TR1 Variables: Tot: %d - PI: %d, PS: %d, NS: %d\n",
            totv, totv - (totp + totn), totp, totn););

        Ddi_VarsetDiffAcc(supp2, supp3);
        Ddi_VarsetPrint(supp2, 0, NULL, stdout);

        Ddi_Free(supp);
        Ddi_Free(supp2);
        Ddi_Free(supp3);

        Ddi_Free(psvars);
        Ddi_Free(nsvars);
      }

      Ddi_BddSetClustered(Tr_TrBdd(tr2), opt->trav.clustTh);
    } else {
      tr2 = Tr_TrDup(tr1);
    }

    if (opt->trav.squaring > 0) {
      Ddi_Vararray_t *zv = Ddi_VararrayAlloc(ddiMgr, Ddi_VararrayNum(ps));
      char buf[1000];

      for (i = 0; i < Ddi_VararrayNum(ps); i++) {
        int xlevel = Ddi_VarCurrPos(Ddi_VararrayRead(ps, i)) + 1;
        Ddi_Var_t *var = Ddi_VarNewAtLevel(ddiMgr, xlevel);

        Ddi_VararrayWrite(zv, i, var);
        sprintf(buf, "%s$CLOSURE$Z", Ddi_VarName(Ddi_VararrayRead(ps, i)));
        Ddi_VarAttachName(var, buf);
      }

      Ddi_BddSetMono(Tr_TrBdd(tr2));
      TrBuildTransClosure(trMgr, Tr_TrBdd(tr2), ps, ns, zv);
      Ddi_Free(zv);
    }
#if 1
    {
      int j;

      for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(tr1));) {
        Ddi_Bdd_t *group, *p;

        group = Ddi_BddPartRead(Tr_TrBdd(tr1), j);
        Ddi_BddSetPartConj(group);
        p = Ddi_BddPartRead(Tr_TrBdd(tr1), i);
        Ddi_BddPartInsertLast(group, p);
        if (Ddi_BddSize(group) > opt->trav.apprGrTh) {
          /* resume from group insert */
          p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
          Ddi_Free(p);
          i++;
          j++;
        } else {
          p = Ddi_BddPartExtract(Tr_TrBdd(tr1), i);
          Ddi_Free(p);
        }
      }
    }
#endif

    if (opt->trav.auxVarFile != NULL) {
      Tr_TrAuxVars(tr2, opt->trav.auxVarFile);
    }
    if (opt->common.clk != 0) {
      Tr_Tr_t *newTr =
        Tr_TrPartitionWithClock(tr2, clock_var, opt->trav.implications);
      if (newTr != NULL) {
        Ddi_Bdd_t *p0, *p1, *f, *fpart;

        Tr_TrFree(tr2);
        tr2 = newTr;
#if 1
        if (opt->trav.trDpartTh > 0) {
          p0 = Ddi_BddPartRead(Tr_TrBdd(tr2), 0);
          p1 = Ddi_BddPartRead(Tr_TrBdd(tr2), 1);
          f = Ddi_BddPartRead(p0, 1);
          fpart = Part_PartitionDisjSet(f, NULL, NULL,
            /*Part_MethodCofactor_c, */ Part_MethodCofactor_c,
            opt->trav.trDpartTh, 3, Pdtutil_VerbLevelDevMax_c);
          Ddi_BddPartWrite(p0, 1, fpart);
          Ddi_Free(fpart);
          f = Ddi_BddPartRead(p1, 1);
          fpart = Part_PartitionDisjSet(f, NULL, NULL,
            /*Part_MethodCofactor_c, */ Part_MethodCofactor_c,
            2 * Ddi_BddSize(f) / 3, 3, Pdtutil_VerbLevelDevMax_c);
          Ddi_BddPartWrite(p1, 1, fpart);
          Ddi_Free(fpart);
        }
#endif
      }
    } else if (opt->trav.trDpartVar != NULL) {
      Ddi_Bdd_t *newTrBdd, *f;
      Ddi_Var_t *v2, *v = Ddi_VarFromName(ddiMgr, opt->trav.trDpartVar);
      Ddi_Bdd_t *p0, *p1;
      char name2[100];

      sprintf(name2, "%s$NS", opt->trav.trDpartVar);
      v2 = Ddi_VarFromName(ddiMgr, name2);
      f = Tr_TrBdd(tr2);
      p0 = Ddi_BddCofactor(f, v2, 0);
      p1 = Ddi_BddCofactor(f, v2, 1);
      newTrBdd = Ddi_BddMakePartDisjVoid(ddiMgr);
      Ddi_BddPartInsertLast(newTrBdd, p0);
      Ddi_BddPartInsertLast(newTrBdd, p1);
      TrTrRelWrite(tr2, newTrBdd);
      Ddi_Free(p0);
      Ddi_Free(p1);
      Ddi_Free(newTrBdd);
      //    Tr_TrSetClustered(tr2);
    } else if (opt->trav.trDpartTh > 0) {
      Ddi_Bdd_t *newTrBdd, *f;

      f = Tr_TrBdd(tr2);
      newTrBdd = Part_PartitionDisjSet(f, NULL, NULL, Part_MethodCofactor_c,
        opt->trav.trDpartTh, 3, Pdtutil_VerbLevelDevMax_c);
      TrTrRelWrite(tr2, newTrBdd);
      Ddi_Free(newTrBdd);
      //    Tr_TrSetClustered(tr2);
    }

    fromCube = Ddi_BddDup(from);
    if (Ddi_BddIsMeta(from)) {
      Ddi_BddSetMono(fromCube);
    }

    if (1) {
      Ddi_Free(fromCube);
    } else if (!Ddi_BddIsCube(fromCube)) {
      //    printf("NO FROMCUBE\n");
      Ddi_Free(fromCube);
      fromCube = Ddi_BddMakeConst(ddiMgr, 1);
    } else {
      //    from = Ddi_BddMakeConst(ddiMgr,1);
      printf("FROMCUBE\n");
      opt->trav.fromSel = 't';
    }

    i = 0;
    verbosityDd = Ddi_MgrReadVerbosity(ddiMgr);
    verbosityTr = Tr_MgrReadVerbosity(Tr_TrMgr(tr));

    do {

      hintsAgain = 0;

      for (; !optFixPoint && !metaFixPoint &&
        (!Ddi_BddIsZero(from)) && ((opt->trav.bound < 0)
          || (i < opt->trav.bound)); i++) {

        int enableLog = (i < 1000 || (i % 100 == 0)) &&
          (verbosityTr > Pdtutil_VerbLevelNone_c);
        int prevLive = Ddi_MgrReadLiveNodes(ddiMgr);

        refTime = currTime = util_cpu_time();
        imgSiftTime = Ddi_MgrReadSiftTotTime(ddiMgr);
        if (travMgr != NULL && TravBddOutOfLimits(travMgr)) {
          /* abort */
          verifOK = -1;
          break;
        }

        if (enableLog) {
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
            fprintf(stdout,
              "\n-- Exact Forward Verification; #Iteration=%d.\n", i + 1));
        }
        Fsm_MgrLogUnsatBound(fsmMgr, i + 1, 0);

        if (opt->trav.siftMaxTravIter > 0 && i >= opt->trav.siftMaxTravIter) {
          if (enableLog) {
            Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
              fprintf(stdout, "\nSIFTING disabled.\n"));
          }
          Ddi_MgrSetSiftThresh(ddiMgr, 10000000);
        }

        if (0 && opt->trav.fbvMeta) {
          int l;

          for (l = 0; l < Ddi_BddPartNum(Tr_TrBdd(tr2)); l++) {
            DdiBddMetaAlign(Ddi_BddPartRead(Tr_TrBdd(tr2), l));
          }
        }

        if ((!Ddi_BddIsMeta(from)) && (!Ddi_BddIncluded(from, invarspec))) {
          verifOK = 0;
          break;
        } else if (Ddi_BddIsMeta(from)) {
          Ddi_Bdd_t *bad = Ddi_BddDiff(from, invarspec);

          verifOK = Ddi_BddIsZero(bad);
          if (!verifOK) {
            Ddi_BddSetMono(bad);
            verifOK = Ddi_BddIsZero(bad);
            Ddi_Free(bad);
            if (!verifOK) {
              break;
            }
          } else
            Ddi_Free(bad);
        }

        Ddi_MgrSetAigAbcOptLevel(ddiMgr, opt->common.aigAbcOpt);
        if (initAig) {
          initAig = 0;
          Ddi_AigInit(ddiMgr, 100);
        }

        if (1 && opt->trav.imgDpartTh > 0) {
          Ddi_MgrSetExistDisjPartTh(ddiMgr, opt->trav.imgDpartTh);
        }


        if ((useHints) && !enHints) {
          if (Ddi_BddSize(from) > hintsTh) {
            enHints = 1;
            if (hintsStrategy > 0) {
              Ddi_Bdd_t *h = Ddi_BddMakeConst(ddiMgr, 1);
              Ddi_Bdd_t *cex = NULL;

              Pdtutil_Assert(opt->trav.hints == NULL,
                "no hints needed at this point");
              opt->trav.hints = Ddi_BddarrayAlloc(ddiMgr, 2);
              Ddi_BddarrayWrite(opt->trav.hints, 1, h);
              Ddi_Free(h);
              h =
                findNonVacuumHint(from, ps, pi, aux, reached, delta,
                Tr_TrBdd(tr2), hConstr, NULL, opt->trav.autoHint,
                hintsStrategy);
              Ddi_BddarrayWrite(opt->trav.hints, 0, h);
              Ddi_Free(h);
              currHintId = 0;
              fprintf(stdout, "\nTraversal uses Hint [0].\n");
              maxHintIter = opt->trav.autoHint;
              hConstr = Ddi_BddMakeConstAig(ddiMgr, 1);
            }
            //      Ddi_Free(from);
            //      from = Ddi_BddDup(reached);
          }
        }

        if ((useHints) && enHints &&    /* !useHintsAsConstraints && */
          currHintId < Ddi_BddarrayNum(opt->trav.hints)) {
          int disHint = 0;
          Ddi_Bdd_t *h_i = Ddi_BddarrayRead(opt->trav.hints, currHintId);

          if (opt->trav.nExactIter > 0 && (i < opt->trav.nExactIter)) {
            if (currHintId == 0)
              disHint = 1;
          }
          if (!disHint) {
            if (Ddi_BddIsMeta(from)) {
              Ddi_BddSetMeta(h_i);
            } else {
              Ddi_BddSetMono(h_i);
            }
            Ddi_BddAndAcc(from, h_i);
          }
        }

        if ((opt->trav.opt_img > 0) && (Ddi_BddSize(from) > opt->trav.opt_img)) {
          to = Tr_ImgOpt(tr2, from, NULL);
        } else if (fromCube != NULL) {
          Ddi_Bdd_t *constrain = Ddi_BddMakeConst(ddiMgr, 1);
          Tr_Tr_t *tr3 = Tr_TrDup(tr2);
          Ddi_Bdd_t *t = Tr_TrBdd(tr3);
          Ddi_Bdd_t *fromAig = Ddi_BddMakeAig(from);
          int nAux = 0;

          if (useHints && enHints && useHintsAsConstraints &&
            currHintId < Ddi_BddarrayNum(opt->trav.hints)) {
            Ddi_Bdd_t *h_i = Ddi_BddarrayRead(opt->trav.hints, currHintId);

            Ddi_BddAndAcc(constrain, h_i);
          }
          if (0 && opt->common.clk != 0) {  /* now handled by TR cluster */
            Ddi_Bdd_t *p0, *p1;

            t = Ddi_BddPartRead(Tr_TrBdd(tr3), 0);
            p0 = Ddi_BddPartExtract(t, 0);
            Ddi_BddRestrictAcc(t, from);
            t = Ddi_BddPartRead(Tr_TrBdd(tr3), 1);
            p1 = Ddi_BddPartExtract(t, 0);
            Ddi_BddRestrictAcc(t, from);
            t = Ddi_BddPartRead(Tr_TrBdd(tr3), 0);
            Ddi_BddPartInsert(t, 0, p0);
            t = Ddi_BddPartRead(Tr_TrBdd(tr3), 1);
            Ddi_BddPartInsert(t, 0, p1);
            Ddi_Free(p0);
            Ddi_Free(p1);
            Tr_TrSetClustered(tr3);
          } else if (1 && fromCube != NULL) {
            int l;
            Ddi_Bdd_t *auxCube = NULL;
            int again = 1, run = 0;

            Ddi_BddConstrainAcc(t, fromCube);
            Ddi_BddConstrainAcc(from, fromCube);
            Ddi_Free(fromCube);
            fromCube = Ddi_BddMakeConst(ddiMgr, 1);

            do {

              again = 0;
              auxCube = Ddi_BddMakeConst(ddiMgr, 1);
              run++;

              for (l = 0; l < Ddi_BddPartNum(Tr_TrBdd(tr3)); l++) {
                int j;
                Ddi_Bdd_t *p_l = Ddi_BddPartRead(t, l);
                Ddi_Bdd_t *proj_l = Ddi_BddExistProject(p_l, nsauxv);
                Ddi_Bdd_t *pAig = Ddi_BddMakeAig(p_l);
                Ddi_Varset_t *supp = Ddi_BddSupp(proj_l);
                Ddi_Vararray_t *vA = Ddi_VararrayMakeFromVarset(supp, 1);

                for (j = 0; j < Ddi_VararrayNum(vA); j++) {
                  Ddi_Var_t *ns_j = Ddi_VararrayRead(vA, j);
                  Ddi_Bdd_t *lit = Ddi_BddMakeLiteral(ns_j, 0);
                  Ddi_Bdd_t *litAig0 = Ddi_BddMakeLiteralAig(ns_j, 0);
                  Ddi_Bdd_t *litAig1 = Ddi_BddMakeLiteralAig(ns_j, 1);

                  if (Ddi_BddIncluded(proj_l, lit) ||
                    (run > 1 && !Ddi_AigSatAnd(fromAig, pAig, litAig1))) {
                    if (Ddi_VarInVarset(nsv, ns_j)) {
                      Ddi_BddAndAcc(fromCube, lit);
                    } else {
                      Ddi_BddAndAcc(auxCube, lit);
                      nAux++;
                    }
                  } else if (Ddi_BddIncluded(proj_l, Ddi_BddNotAcc(lit)) ||
                    (run > 1 && !Ddi_AigSatAnd(fromAig, pAig, litAig0))) {
                    if (Ddi_VarInVarset(nsv, ns_j)) {
                      Ddi_BddAndAcc(fromCube, lit);
                    } else {
                      Ddi_BddAndAcc(auxCube, lit);
                      nAux++;
                    }
                  } else {
                    Ddi_VarsetRemoveAcc(supp, ns_j);
                  }
                  Ddi_Free(lit);
                  Ddi_Free(litAig0);
                  Ddi_Free(litAig1);
                }
                Ddi_BddExistAcc(p_l, supp);
                Ddi_Free(supp);
                Ddi_Free(vA);
                Ddi_Free(pAig);
                Ddi_Free(proj_l);
              }
#if 0
              Tr_TrSortIwls95(tr3);
              Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
              Tr_TrSetClustered(tr3);
#endif
              if (!Ddi_BddIsOne(auxCube)) {
                Ddi_BddConstrainAcc(t, auxCube);
                again = 1;
              }
              Ddi_Free(auxCube);
            } while (again || run <= 1);
          }
          if (opt->trav.fbvMeta && Ddi_BddSize(from) > 5
            && !Ddi_BddIsMeta(from)) {
            Ddi_BddSetMeta(from);
          }
          to = Tr_ImgWithConstrain(tr3, from, constrain);
          // logBdd(to);
          Tr_TrFree(tr3);
          Ddi_Free(constrain);
          Ddi_Free(fromAig);
          if (fromCube != NULL && Ddi_BddSize(fromCube) > 0) {
            Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
              fprintf(stdout, "IMG CUBE: %d (aux: %d).\n",
                Ddi_BddSize(fromCube), nAux));
            Ddi_BddSwapVarsAcc(fromCube, Tr_MgrReadPS(trMgr),
              Tr_MgrReadNS(trMgr));
          }

        } else {
          Ddi_Bdd_t *constrain = Ddi_BddMakeConst(ddiMgr, 1);

#if 0
          Ddi_Var_t *v = Ddi_VarFromName(ddiMgr, "i62");
          Ddi_Var_t *v1 = Ddi_VarFromName(ddiMgr, "l132");
          Ddi_Bdd_t *constrain = Ddi_BddMakeLiteral(v, 0);
          Ddi_Bdd_t *c2 = Ddi_BddMakeLiteral(v1, 1);

          if (i > 0)
            Ddi_BddAndAcc(from, c2);
#endif
          if (0 || opt->trav.imgDpartSat > 0) {
            // Tr_MgrSetImgMethod(trMgr,Tr_ImgMethodPartSat_c);
            Ddi_Free(constrain);
            constrain = Ddi_BddSwapVars(reached, ps, ns);
            Ddi_BddNotAcc(constrain);
          }
          if (!enableLog) {
            Ddi_MgrSetVerbosity(ddiMgr, (Pdtutil_VerbLevel_e) ((int)0));
            Tr_MgrSetVerbosity(trMgr, (Pdtutil_VerbLevel_e) ((int)0));
          }
          if ((useHints) && enHints && useHintsAsConstraints &&
            currHintId < Ddi_BddarrayNum(opt->trav.hints)) {
            Ddi_Bdd_t *h_i = Ddi_BddarrayRead(opt->trav.hints, currHintId);

            Ddi_BddAndAcc(constrain, h_i);
          }
          to = Tr_ImgWithConstrain(tr2, from, constrain);
          if (enableLog) {
            Ddi_MgrSetVerbosity(ddiMgr,
              (Pdtutil_VerbLevel_e) ((int)verbosityDd));
            Tr_MgrSetVerbosity(trMgr,
              (Pdtutil_VerbLevel_e) ((int)verbosityTr));
          }
          // logBdd(reached);
          Ddi_Free(constrain);
        }

        if (0) {
          Ddi_Varset_t *s = Ddi_BddSupp(to);
          Ddi_Vararray_t *vA = Ddi_VararrayMakeFromVarset(s, 1);
          int i, j, neq = 0;
          Ddi_Vararray_t *a = Ddi_VararrayAlloc(ddiMgr, 1);
          Ddi_Vararray_t *b = Ddi_VararrayAlloc(ddiMgr, 1);

          for (i = 0; i < Ddi_VararrayNum(vA); i++) {
            Ddi_Var_t *v_i = Ddi_VararrayRead(vA, i);

            Ddi_VararrayWrite(a, 0, v_i);
            Ddi_Bdd_t *to_i_0 = Ddi_BddCofactor(to, v_i, 0);
            Ddi_Bdd_t *to_i_1 = Ddi_BddCofactor(to, v_i, 1);

            for (j = i + 1; j < Ddi_VararrayNum(vA); j++) {
              Ddi_Bdd_t *to2;
              Ddi_Var_t *v_j = Ddi_VararrayRead(vA, j);

              Ddi_VararrayWrite(b, 0, v_j);
              to2 = Ddi_BddSwapVars(to, a, b);
              if (Ddi_BddEqual(to2, to)) {
                Ddi_Bdd_t *to_i_0_j_1 = Ddi_BddCofactor(to_i_0, v_j, 1);
                Ddi_Bdd_t *to_i_1_j_0 = Ddi_BddCofactor(to_i_1, v_j, 0);

                neq++;
                Ddi_Free(to2);
                printf("%d - %d SYMMETRY FOUND (%s-%s) - ", i, j,
                  Ddi_VarName(v_i), Ddi_VarName(v_j));
                if (Ddi_BddIsZero(to_i_0_j_1)) {
                  Pdtutil_Assert(Ddi_BddIsZero(to_i_1_j_0), "wrong symmetry");
                  printf("EQ");
                }
                printf("\n");
                Ddi_Free(to_i_0_j_1);
                Ddi_Free(to_i_1_j_0);
                break;
              }
              Ddi_Free(to2);
            }
            Ddi_Free(to_i_0);
            Ddi_Free(to_i_1);
          }
          printf("%d SYMMETRIES FOUND\n", neq);
          Ddi_Free(a);
          Ddi_Free(vA);
          Ddi_Free(s);
        }

        if (ddiMgr->exist.clipDone) {
          Ddi_Free(ddiMgr->exist.clipWindow);
          ddiMgr->exist.clipWindow = NULL;
          travAgain = 1;
        }

        if (!Ddi_BddIsMeta(to) && !Ddi_BddIsOne(invar)) {
          if (0 && Ddi_BddIsMeta(to)) {
            Ddi_BddSetMeta(invar);
          }
          Ddi_BddAndAcc(to, invar);
        }
        Ddi_Free(from);

        if (Ddi_BddIsMeta(to) || Ddi_BddIsMeta(reached)) {
          Ddi_Varset_t *novars = Ddi_VarsetVoid(Ddi_ReadMgr(reached));

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "[|R(meta)|: %d]", Ddi_BddSize(reached)));
          Ddi_BddExistAcc(reached, novars);
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "[|R(meta2)|: %d]", Ddi_BddSize(reached)));
          Ddi_Free(novars);
        }

        if (fromCube != NULL) {
          Ddi_BddAndAcc(to, fromCube);
        }

        optFixPoint = 0;
        if (Ddi_BddIsMeta(to) &&
          (Ddi_BddIsAig(reached) || Ddi_BddIsPartDisj(reached))) {
          Ddi_Bdd_t *chk = Ddi_BddDup(to);

          if (initAig) {
            initAig = 0;
            Ddi_AigInit(ddiMgr, 100);
          }
          if (fromCube != NULL) {
            Ddi_BddAndAcc(chk, fromCube);
          }
          if (Ddi_BddIsAig(reached)) {
            Ddi_BddSetAig(chk);
          }
          Ddi_BddDiffAcc(chk, reached);
          from = Ddi_BddDup(to);
          Ddi_BddSetAig(chk);
          metaFixPoint = !Ddi_AigSat(chk);
          Ddi_Free(chk);
        } else if (satFpChk) {
          Ddi_Bdd_t *chk = Ddi_BddDup(to);
          Ddi_Bdd_t *rAig = Ddi_BddDup(reached);

          if (initAig) {
            initAig = 0;
            Ddi_AigInit(ddiMgr, 100);
          }
          if (fromCube != NULL) {
            Ddi_BddAndAcc(chk, fromCube);
          }
          Ddi_BddSetAig(chk);
          Ddi_BddSetAig(rAig);

          Ddi_BddDiffAcc(chk, rAig);
          from = Ddi_BddDup(to);
          optFixPoint = !Ddi_AigSat(chk);
          Ddi_Free(chk);
          Ddi_Free(rAig);
        } else {
          from = Ddi_BddDiff(to, reached);
          optFixPoint = Ddi_BddIsZero(from);
        }

        if (!Ddi_BddIsZero(from) && (opt->trav.fromSel == 't')) {
          Ddi_Free(from);
          from = Ddi_BddDup(to);
        } else if (!Ddi_BddIsZero(from) && (opt->trav.fromSel == 'c')) {
          Ddi_BddNotAcc(reached);
          Ddi_Free(from);
          from = Ddi_BddRestrict(to, reached);
          Ddi_BddNotAcc(reached);
        }

        if (Ddi_BddIsAig(reached)) {
          Ddi_Bdd_t *toAig = Ddi_BddDup(to);

          Ddi_BddSetAig(toAig);
          if (opt->common.aigAbcOpt > 2) {
            ddiAbcOptAcc(toAig, -1.0);
          }
          Ddi_BddOrAcc(reached, toAig);
          if (opt->common.aigAbcOpt > 2) {
            ddiAbcOptAcc(reached, -1.0);
          }
          Ddi_Free(toAig);
        } else if (Ddi_BddIsMeta(from) &&
          (Ddi_BddSize(to) > 500000 || Ddi_BddSize(reached) > 1000000)) {
          Ddi_BddSetPartDisj(reached);
          Ddi_BddPartInsertLast(reached, to);
          if (Ddi_BddSize(reached) > 50000) {
            if (initAig) {
              initAig = 0;
              Ddi_AigInit(ddiMgr, 100);
              Ddi_MgrSetAigAbcOptLevel(ddiMgr, opt->common.aigAbcOpt);
            }
            Ddi_BddSetAig(reached);
          }
        } else {
          if (Ddi_BddIsMeta(from)) {
            //      Ddi_MgrSiftSuspend(ddiMgr);
            Ddi_BddSetMeta(reached);
          }
          Ddi_BddOrAcc(reached, to);
          if (Ddi_BddIsMeta(from)) {
            //      Ddi_MgrSiftResume(ddiMgr);
          }
        }

        if (!Ddi_BddIsZero(from) && ((opt->trav.fromSel == 'r') ||
            ((opt->trav.fromSel == 'b')
              && ((Ddi_BddSize(reached) < Ddi_BddSize(from)))))) {
          if (!(Ddi_BddIsMeta(from) && Ddi_BddIsPartDisj(reached))) {
            Ddi_Free(from);
            from = Ddi_BddDup(reached);
          }
        }

        if (opt->trav.cex >= 1) {
          if (opt->trav.guidedTrav) {
            Ddi_BddarrayInsertLast(frontier, reached);
          } else {
            Ddi_BddarrayWrite(frontier, i + 1, from);
          }
        }

        if (enableLog) {
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "|From|=%d; ", Ddi_BddSize(from));
            fprintf(stdout, "|To|=%d; |Reached|=%d; ",
              Ddi_BddSize(to), Ddi_BddSize(reached));
            fprintf(stdout, "#ReachedStates=%g\n",
              Ddi_BddCountMinterm(reached, Ddi_VararrayNum(ps)));
#if 0
            fprintf(stdout, "#ReachedStates(estimated)=%g\n",
              Ddi_AigEstimateMintermCount(reached, Ddi_VararrayNum(ps)))
#endif
            );
          if (opt->trav.auxPsSet != NULL) {
            Ddi_Bdd_t *r = Ddi_BddExist(reached, opt->trav.auxPsSet);

            Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
              printf("# NO AUX Reached states = %g (size:%d)\n",
                Ddi_BddCountMinterm(r,
                  Ddi_VararrayNum(ps) - Ddi_VarsetNum(opt->trav.auxPsSet)),
                Ddi_BddSize(r)));
            Ddi_Free(r);
          }
        }
        Ddi_Free(to);

        if (opt->trav.fbvMeta) {
          Ddi_Varset_t *novars = Ddi_VarsetVoid(Ddi_ReadMgr(from));

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "[|F,R(meta)|: %d,%d]",
              Ddi_BddSize(from), Ddi_BddSize(reached)));

          Ddi_BddExistAcc(reached, novars);
          Ddi_BddExistAcc(from, novars);

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "[|F,R(meta2)|: %d,%d]",
              Ddi_BddSize(from), Ddi_BddSize(reached)));

          Ddi_Free(novars);
        }

        currTime = util_cpu_time();
        imgSiftTime = Ddi_MgrReadSiftTotTime(ddiMgr) - imgSiftTime;
        if (0 || enableLog) {
          int currLive = Ddi_MgrReadLiveNodes(ddiMgr);

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
            fprintf(stdout,
              " Live Nodes (Peak/Incr) = %u (%u/%u\%)\n",
              currLive,
              Ddi_MgrReadPeakLiveNodeCount(Ddi_ReadMgr(reached)),
              ((currLive - prevLive) * 100 / prevLive));
            fprintf(stdout,
              " CPU Time (img/img-no-sift) = %f (%f/%f)\n",
              ((currTime - initialTime) / 1000.0),
              (currTime - refTime) / 1000.0,
              (currTime - refTime - imgSiftTime) / 1000.0););
        }
      }

      if (verifOK && (useHints) && enHints &&
        currHintId < Ddi_BddarrayNum(opt->trav.hints) - 1) {

        int useMeta = 0;
        Ddi_Bdd_t *h_i, *prev_h;
        int vacuum = 0;

        if (Ddi_BddIsMeta(from)) {
          useMeta = 1;
        }
        hintsAgain = 1;
        Ddi_Free(from);
        from = Ddi_BddDup(reached);
        prev_h = Ddi_BddDup(Ddi_BddarrayRead(opt->trav.hints, currHintId));
        Ddi_BddNotAcc(prev_h);
        Ddi_BddConstrainAcc(from, prev_h);
        Ddi_BddNotAcc(prev_h);
        Ddi_BddDiffAcc(from, prev_h);
        currHintId++;
        i--;

        do {
          if (useHints && hintsStrategy > 0 && maxHintIter-- > 0) {
            int myHintsStrategy = hintsStrategy;

            if (0 && hintsStrategy > 1 && opt->trav.autoHint - maxHintIter < 3) {
              //myHintsStrategy = 1;
            }
            h_i =
              findNonVacuumHint(from, ps, pi, aux, reached, delta,
              Tr_TrBdd(tr2), hConstr, prev_h, maxHintIter / 2,
              myHintsStrategy);
            Ddi_BddarrayWrite(opt->trav.hints, 0, h_i);
            Ddi_Free(h_i);
            h_i = Ddi_BddarrayRead(opt->trav.hints, 0);
            currHintId = 0;
          } else {
            Ddi_Bdd_t *trAndNotReached = Ddi_BddNot(reached);
            Ddi_Bdd_t *myFrom = Ddi_BddDup(from);
            Ddi_Var_t *iv = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

            if (myDeltaAig == NULL) {
              int j;

              myDeltaAig = Ddi_BddarrayDup(delta);
              for (j = 0; j < Ddi_BddarrayNum(myDeltaAig); j++) {
                Pdtutil_Assert(Ddi_BddIsAig(Ddi_BddarrayRead(myDeltaAig, j)),
                  "aig delta needed");
              }
            }
            Ddi_BddSetAig(trAndNotReached);
            if (iv != NULL) {
              Ddi_Bdd_t *invar = Ddi_BddMakeLiteralAig(iv, 1);

              Ddi_AigAndCubeAcc(trAndNotReached, invar);
              Ddi_BddSetMono(invar);
              Ddi_BddConstrainAcc(myFrom, invar);
              Ddi_BddAndAcc(myFrom, invar);
              Ddi_Free(invar);
            }

            Ddi_BddComposeAcc(trAndNotReached, ps, myDeltaAig);
            if (iv != NULL) {
              Ddi_Bdd_t *invar = Ddi_BddMakeLiteralAig(iv, 1);

              Ddi_BddAndAcc(trAndNotReached, invar);
              Ddi_Free(invar);
            }

            h_i = Ddi_BddarrayRead(opt->trav.hints, currHintId);
            Ddi_BddConstrainAcc(myFrom, h_i);
            Ddi_BddSetAig(myFrom);
            Ddi_BddSetAig(h_i);
            Ddi_BddAndAcc(myFrom, h_i);
            vacuum = !Ddi_AigSat(myFrom);
            vacuum |=
              Ddi_AigSatAndWithAbort(myFrom, trAndNotReached, NULL, -1.0) == 0;

            if (0 && !vacuum) {
              Ddi_Bdd_t *myTo, *cex = Ddi_AigSatAndWithCexAndAbort(myFrom,
                trAndNotReached, NULL, NULL, -1, NULL);

              Pdtutil_Assert(cex != NULL, "NULL cex");
              Ddi_BddSetMono(cex);
              myTo = Tr_ImgWithConstrain(tr2, cex, NULL);
              Pdtutil_Assert(!Ddi_BddIncluded(myTo, reached),
                "wrong vacuum verdict");
              Ddi_Free(myTo);
              Ddi_Free(cex);
            }
            Ddi_Free(trAndNotReached);
            Ddi_Free(myFrom);

          }
          if (useMeta) {
            Ddi_BddSetMeta(h_i);
          } else {
            Ddi_BddSetMono(h_i);
          }
          if (vacuum) {
            Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
              fprintf(stdout, "\nVacuum hint [%d].\n", currHintId));
            currHintId++;
          }

        } while (vacuum && currHintId < Ddi_BddarrayNum(opt->trav.hints));

        Ddi_Free(prev_h);

        if (vacuum) {
          hintsAgain = 0;
          optFixPoint = metaFixPoint = 1;
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "\nTraversal Stops on vacuum hint [%d].\n",
              currHintId));
        } else {
          Ddi_BddAndAcc(from, h_i);
          optFixPoint = metaFixPoint = 0;
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "\nTraversal Resumes on Hint [%d/%d].\n",
              hintsStrategy >
              0 ? opt->trav.autoHint - maxHintIter : currHintId,
              opt->trav.autoHint));
        }

      }
    } while (hintsAgain);

    Ddi_Free(from);
    from = Ddi_BddDup(reached);
    ddiMgr->exist.clipThresh *= 1.5;

    Tr_TrFree(tr2);

  } while ((travAgain || ddiMgr->exist.clipDone) && verifOK);

  if (Ddi_BddIsMeta(reached) && (opt->mc.method == Mc_VerifTravFwd_c)) {
    Ddi_BddSetMono(reached);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|Reached(mono)|=%d; ", Ddi_BddSize(reached));
      fprintf(stdout, "#ReachedStates=%g.\n",
        Ddi_BddCountMinterm(reached, Ddi_VararrayNum(ps))););
  }

  if (opt->trav.wR != NULL) {
    Ddi_MgrReduceHeap(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"), 0);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "Writing outer reached ring to %s.\n", opt->trav.wR));

    Ddi_BddStore(reached, "reached", DDDMP_MODE_TEXT, opt->trav.wR, NULL);
  }

  if (opt->trav.wU != NULL) {
    int i;
    int n = atoi(opt->trav.wU);
    Ddi_Bdd_t *unr = Ddi_BddMakeConst(ddiMgr, 0);
    Ddi_Var_t *vp = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");
    Ddi_Var_t *vc = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
    Ddi_Varset_t *sm = Ddi_VarsetVoid(ddiMgr);

    Ddi_VarsetAddAcc(sm, vp);
    Ddi_VarsetAddAcc(sm, vc);
    Ddi_BddExistAcc(reached, sm);
    Ddi_BddNotAcc(reached);

    if (n == 0) {
      Ddi_Bdd_t *c;
      Ddi_Varset_t *psvars = Ddi_VarsetMakeFromArray(Tr_MgrReadPS(trMgr));

      Ddi_VarsetRemoveAcc(psvars, vp);
      Ddi_VarsetRemoveAcc(psvars, vc);

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "Init state.\n"));

      c = Ddi_BddPickOneMinterm(reached, psvars);

      logBdd(c);
      Ddi_Free(psvars);
      Ddi_Free(c);
    } else {
      if (n < 0) {
        int iF = Ddi_BddarrayNum(frontier) - 2;

        n *= -1;
        Ddi_Free(reached);
        reached = Ddi_BddDup(Ddi_BddarrayRead(frontier, iF));
        Ddi_BddExistAcc(reached, sm);
        if (opt->trav.rPlus != NULL) {
          Ddi_Bdd_t *r2 = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
            DDDMP_MODE_DEFAULT, opt->trav.rPlus, NULL);

          Ddi_BddExistAcc(r2, sm);
          Ddi_BddDiffAcc(reached, r2);
          while (iF > 1 && Ddi_BddIsZero(reached)) {
            iF--;
            Ddi_Free(reached);
            reached = Ddi_BddDup(Ddi_BddarrayRead(frontier, iF));
            Ddi_BddExistAcc(reached, sm);
            Ddi_BddDiffAcc(reached, r2);
          }
          Ddi_Free(r2);
        }
      }

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, ".outputs p0\n\n"));

      for (i = 0; i < n; i++) {
        Ddi_Bdd_t *c;

        c = Ddi_BddPickOneCube(reached);
        Ddi_BddDiffAcc(reached, c);
        Ddi_BddOrAcc(unr, c);
        Ddi_Free(c);
      }
      LOG_BDD_CUBE_PLA(unr, " p0\n");
    }
    Ddi_Free(unr);
    Ddi_Free(sm);
  }

  Ddi_Free(from);
  Ddi_Free(fromCube);

  if (travMgr != NULL) {
    Trav_MgrSetReached(travMgr, reached);
  }

  Ddi_Free(reached);

  Tr_TrFree(tr1);

  ddiMgr->exist.clipLevel = 0;

  if (verifOK < 0) {
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Invariant Verification: UNDECIDED.\n"));
  } else if (verifOK) {
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Invariant Verification: PASS.\n"));
  } else {
    FILE *fp;
    Trav_Mgr_t *travMgr;
    Ddi_Bddarray_t *patternMisMat;
    Ddi_Varset_t *piv;
    Ddi_Bdd_t *invspec_bar, *end;

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Invariant Verification: FAIL.\n"));

    if (opt->trav.cex >= 1) {

      Tr_MgrSetClustSmoothPi(trMgr, 0);
      travMgr = Trav_MgrInit(NULL, ddiMgr);
      Trav_MgrSetMismatchOptLevel(travMgr, opt->trav.mism_opt_level);

#if 1
      piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));
#if 0
      /* keep PIs for counterexample */
      invspec_bar = Ddi_BddExist(invarspec, piv);
#else
      invspec_bar = Ddi_BddNot(invarspec);
#endif

      Tr_TrReverseAcc(tr);

      patternMisMat = Trav_MismatchPat(travMgr, tr, invspec_bar,
        NULL, NULL, &end, frontier,
        Tr_MgrReadPS(Tr_TrMgr(tr)), Tr_MgrReadNS(Tr_TrMgr(tr)), piv);

      Tr_TrReverseAcc(tr);

      /*---------------------------- Write Patterns --------------------------*/

      if (patternMisMat != NULL) {
        if (opt->trav.cex == 1) {
          FbvBddCexToFsm(fsmMgr, patternMisMat, end, 1 /* reverse */ );
        } else {
          char filename[100];

          sprintf(filename, "%s%d.txt", FILEPAT, getpid());
          fp = fopen(filename, "w");
          //fp = fopen ("pdtpat.txt", "w");
          if (Ddi_BddarrayPrintSupportAndCubes(patternMisMat, 1, -1,
              0, 1, NULL, fp) == 0) {
            Pdtutil_Warning(1, "Store cube Failed.");
          }
          fclose(fp);
        }
      }

      /*----------------------- Write Initial State Set ----------------------*/

      if (end != NULL && !Ddi_BddIsZero(end)) {
        char filename[100];

        sprintf(filename, "%s%d.txt", FILEINIT, getpid());
        fp = fopen(filename, "w");
        //fp = fopen ("pdtinit.txt", "w");
        if (Ddi_BddPrintSupportAndCubes(end, 1, -1, 0, NULL, fp) == 0) {
          Pdtutil_Warning(1, "Store cube Failed.");
        }
        fclose(fp);
        Ddi_Free(end);
      }

      Ddi_Free(piv);
      Ddi_Free(invspec_bar);
      Ddi_Free(patternMisMat);

#endif
    }

  }

  Ddi_Free(hConstr);
  Ddi_Free(myDeltaAig);
  Ddi_Free(frontier);
  Ddi_Free(nsv);
  Ddi_Free(auxv);
  Ddi_Free(nsauxv);

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c, if (verifOK >= 0) {
      currTime = util_cpu_time();
      fprintf(stdout, "Live Nodes (Peak): %u(%u) ",
        Ddi_MgrReadLiveNodes(ddiMgr), Ddi_MgrReadPeakLiveNodeCount(ddiMgr));
      //fprintf(stdout, "TotalTime: %s ", util_print_time (currTime-opt->stats.startTime));
      fprintf(stdout, "TotalTime: %s ",
        util_print_time((opt->stats.setupTime - opt->stats.startTime) +
          (currTime - initialTime)));
      fprintf(stdout, "(setup: %s - ",
        util_print_time(opt->stats.setupTime - opt->stats.startTime));
//fprintf(stdout, "(setup: %s -", util_print_time (initialTime-opt->stats.startTime));
    fprintf(stdout, "trav: %s)\n", util_print_time(currTime - initialTime));}
  ) ;

  return (verifOK);
}


/**Function*******************************************************************
  Synopsis    [exact forward verification]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
FbvApproxForwExactBckFixPointVerify(
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Ddi_Bdd_t * care,
  Ddi_Bddarray_t * delta,
  Fbv_Globals_t * opt
)
{
  int i, j, verifOK, fixPoint, fwdRings, saveClipTh, doApproxRange;
  Ddi_Bdd_t *tmp, *from, *fwdReached, *bckReached, *to, *notInit, *badStates;
  Ddi_Vararray_t *ps, *ns;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(init);
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Tr_Tr_t *bckTr, *fwdTr;
  long initialTime, currTime;
  Ddi_Bddarray_t *frontier;
  int satRefine = opt->trav.itpAppr > 0 ? opt->trav.itpAppr : 0;
  int satRefineInductive = opt->trav.itpAppr < 0 ? -opt->trav.itpAppr : 0;
  Ddi_Bdd_t *trAig = NULL;
  Ddi_Bdd_t *prevRef = NULL;

  Ddi_Bdd_t *clock_tr = NULL;
  Ddi_Var_t *clock_var = NULL;
  Ddi_Var_t *clock_var_ns = NULL;

  ps = Tr_MgrReadPS(trMgr);
  ns = Tr_MgrReadNS(trMgr);

  if (satRefine || satRefineInductive) {
    trAig = Ddi_BddRelMakeFromArray(delta, ns);
    Ddi_BddSetAig(trAig);

    if (satRefine && opt->trav.itpInductiveTo) {
      prevRef = Ddi_BddMakeConstAig(ddiMgr, 1);
    }
  }

  if (opt->tr.en != NULL) {
    Ddi_Var_t *enVar = Ddi_VarFromName(ddiMgr, opt->tr.en);
    Ddi_Var_t *enAuxPs, *enAuxNs;
    char buf[1000];
    Ddi_Bdd_t *trEn, *lit;

    enAuxPs = Ddi_VarNewAfterVar(enVar);
    sprintf(buf, "%s$ENABLE", opt->tr.en);
    Ddi_VarAttachName(enAuxPs, buf);
    enAuxNs = Ddi_VarNewAfterVar(enVar);
    sprintf(buf, "%s$ENABLE$NS", opt->tr.en);
    Ddi_VarAttachName(enAuxNs, buf);
    Ddi_VarMakeGroup(ddiMgr, enAuxNs, 2);
    trEn = Ddi_BddMakeLiteral(enAuxNs, 1);
    lit = Ddi_BddMakeLiteral(enVar, 1);
    Ddi_BddXnorAcc(trEn, lit);
    Ddi_Free(lit);
    Ddi_BddPartInsert(Tr_TrBdd(tr), 1, trEn);
    Ddi_Free(trEn);
    Ddi_VararrayInsert(ps, 1, enAuxPs);
    Ddi_VararrayInsert(ns, 1, enAuxNs);
  }
  if (opt->trav.auxVarFile != NULL) {
    Tr_TrAuxVars(tr, opt->trav.auxVarFile);
  }

  if (opt->common.clk != NULL) {
#if 0
    {
      Ddi_Varset_t *reducedVars = Ddi_VarsetMakeFromArray(ps);
      Ddi_Bddarray_t *l = Ddi_BddarrayAlloc(ddiMgr, 0);

      Ddi_BddarrayInsert(l, 0, invarspec);
      Tr_TrReduceMasterSlave(tr, reducedVars,
        l, Ddi_VarFromName(ddiMgr, opt->common.clk));
      Ddi_Free(reducedVars);
      DdiGenericDataCopy((Ddi_Generic_t *) invarspec,
        (Ddi_Generic_t *) Ddi_BddarrayRead(l, 0));
      Ddi_Free(l);
    }
#endif

#if 0
    clock_var = Ddi_VarFromName(ddiMgr, opt->common.clk);
#else
    clock_var = Tr_TrSetClkStateVar(tr, opt->common.clk, 1);
    if (opt->common.clk != Ddi_VarName(clock_var)) {
      opt->common.clk = Ddi_VarName(clock_var);
    }
#endif

    if (clock_var != NULL) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Clock Variable (%s) Found.\n", opt->common.clk));

      for (i = 0; i < Ddi_VararrayNum(ps); i++) {
        if (Ddi_VararrayRead(ps, i) == clock_var) {
          clock_var_ns = Ddi_VararrayRead(ns, i);
          break;
        }
      }

      if (clock_var_ns == NULL) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Clock is not a state variable.\n"));
      } else {
        for (i = 0; i < Ddi_BddPartNum(Tr_TrBdd(tr)); i++) {
          Ddi_Varset_t *supp = Ddi_BddSupp(Ddi_BddPartRead(Tr_TrBdd(tr), i));

          /*Ddi_VarsetPrint(supp, 0, NULL, stdout); */
          if (Ddi_VarInVarset(supp, clock_var_ns)) {
            clock_tr = Ddi_BddDup(Ddi_BddPartRead(Tr_TrBdd(tr), i));
            Ddi_Free(supp);
            Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
              fprintf(stdout, "Found clock TR.\n"));
            break;
          } else {
            Ddi_Free(supp);
          }
        }
      }
#if 0
      clk_lit = Ddi_BddMakeLiteral(clock_var, 0);
      Ddi_BddOrAcc(invarspec, clk_lit);
      Ddi_Free(clk_lit);
#endif
    }
  }

  badStates = Ddi_BddNot(invarspec);

  opt->ddi.saveClipLevel = ddiMgr->exist.clipLevel;
  ddiMgr->exist.clipLevel = 0;

  initialTime = util_cpu_time();

  {
    Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));

    if (clock_tr != NULL) {
      Ddi_Varset_t *nsv = Ddi_VarsetMakeFromArray(ns);

      Ddi_VarsetUnionAcc(piv, nsv);
      Ddi_BddAndExistAcc(badStates, clock_tr, piv);
      Ddi_Free(nsv);
    } else {
      Ddi_BddExistAcc(badStates, piv);
    }
    Ddi_Free(piv);
  }

  Pdtutil_VerbosityMgrIf(trMgr, Pdtutil_VerbLevelUsrMed_c) {
    fprintf(stdout, "|Invar|=%d; ", Ddi_BddSize(invar));
    fprintf(stdout, "|Tr|=%d.\n", Ddi_BddSize(Tr_TrBdd(tr)));
  }

  if (opt->trav.invarFile != NULL) {
    Ddi_BddPartInsert(Tr_TrBdd(tr), 1, invar);
  } else if (invar != NULL) {
    invar = Ddi_BddDup(invar);
  } else {
#if ENABLE_INVAR
    invar = Ddi_BddPartExtract(Tr_TrBdd(tr), 0);
#else
    invar = Ddi_BddMakeConst(ddiMgr, 1);
#endif
  }

  fwdTr = Tr_TrDup(tr);

  if (0 && opt->trav.approxTrClustFile != NULL) {
    Tr_Tr_t *newTr = Tr_TrManualPartitions(fwdTr, opt->trav.approxTrClustFile);

    Tr_TrFree(fwdTr);
    fwdTr = newTr;
    //    opt->trav.apprClustTh = 0;
  }
  if (0 && opt->trav.approxTrClustFile != NULL) {
    opt->trav.apprClustTh = 0;
  }

  bckTr = Tr_TrDup(tr);

  Tr_MgrSetClustSmoothPi(trMgr, 1);
  Tr_MgrSetClustThreshold(trMgr, 1);
  if (opt->trav.approxTrClustFile != NULL) {
    Tr_TrSetClusteredApprGroups(fwdTr);
    Tr_TrSortIwls95(fwdTr);
    Tr_MgrSetClustThreshold(trMgr, opt->trav.apprClustTh);
    Tr_TrSetClusteredApprGroups(fwdTr);
  } else {
    Tr_TrSetClustered(fwdTr);
    Tr_TrSortIwls95(fwdTr);
    Tr_MgrSetClustThreshold(trMgr, opt->trav.apprClustTh);
    Tr_TrSetClustered(fwdTr);
  }

  Tr_MgrSetClustThreshold(trMgr, 1);
  Tr_TrSetClustered(tr);
  Tr_TrSortIwls95(tr);
  Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
  Tr_TrSetClustered(tr);

#if 1
  {
    Ddi_Bdd_t *group, *p;
    Ddi_Varset_t *suppGroup;
    int nsupp, nv = Ddi_VararrayNum(ps) + Ddi_VararrayNum(Tr_MgrReadI(trMgr));

    for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(fwdTr));) {
      group = Ddi_BddPartRead(Tr_TrBdd(fwdTr), j);
      Ddi_BddSetPartConj(group);
      p = Ddi_BddPartRead(Tr_TrBdd(fwdTr), i);
      Ddi_BddPartInsertLast(group, p);
      suppGroup = Ddi_BddSupp(group);
      nsupp = Ddi_VarsetNum(suppGroup);
      Ddi_Free(suppGroup);
      if ((Ddi_BddSize(group) * nsupp) / nv > opt->trav.apprGrTh) {
        /* resume from group insert */
        p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
        Ddi_Free(p);
#if 0
        if (clock_tr != NULL) {
          Ddi_BddPartInsert(group, 0, clock_tr);
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "#"));
        }
#endif
        i++;
        j++;
      } else {
        p = Ddi_BddPartExtract(Tr_TrBdd(fwdTr), i);
        Ddi_Free(p);
      }
    }
#if 0
    if (clock_tr != NULL) {
      Ddi_BddPartInsert(group, 0, clock_tr);
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "#"));
    }
#endif
  }

  {
    if (opt->trav.apprOverlap) {
      Ddi_Bdd_t *group0, *group1, *overlap, *p;
      int np;

      for (i = Ddi_BddPartNum(Tr_TrBdd(fwdTr)) - 1; i > 0; i--) {
        group0 = Ddi_BddPartRead(Tr_TrBdd(fwdTr), i - 1);
        group1 = Ddi_BddPartRead(Tr_TrBdd(fwdTr), i);
        overlap = Ddi_BddMakePartConjVoid(ddiMgr);
        if (Ddi_BddIsPartConj(group0)) {
          np = Ddi_BddPartNum(group0);
          for (j = np / 2; j < np; j++) {
            p = Ddi_BddPartRead(group0, j);
            Ddi_BddPartInsertLast(overlap, p);
          }
        } else {
          Ddi_BddPartInsertLast(overlap, group0);
        }
        if (Ddi_BddIsPartConj(group1)) {
          np = Ddi_BddPartNum(group1);
          for (j = 0; j < (np + 1) / 2; j++) {
            p = Ddi_BddPartRead(group1, j);
            Ddi_BddPartInsertLast(overlap, p);
          }
        } else {
          Ddi_BddPartInsertLast(overlap, group1);
        }
        Ddi_BddPartInsert(Tr_TrBdd(fwdTr), i, overlap);
        Ddi_Free(overlap);
      }
    }
  }

#if 0
  for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(bckTr));) {
    Ddi_Bdd_t *group, *p;

    group = Ddi_BddPartRead(Tr_TrBdd(bckTr), j);
    Ddi_BddSetPartConj(group);
    p = Ddi_BddPartRead(Tr_TrBdd(bckTr), i);
    Ddi_BddPartInsertLast(group, p);
    if (Ddi_BddSize(group) > 4 * opt->trav.apprGrTh) {
      /* resume from group insert */
      p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
      Ddi_Free(p);
      i++;
      j++;
    } else {
      p = Ddi_BddPartExtract(Tr_TrBdd(bckTr), i);
      Ddi_Free(p);
    }
  }
#endif
#endif


  /* approx forward */

  from = Ddi_BddDup(init);

#if 0
  if (opt->trav.noInit) {
    Ddi_BddAndAcc(init, Ddi_MgrReadZero(ddiMgr));
  }
#endif

  /* disjunctively partitioned reached */
  fwdReached = Ddi_BddMakePartDisjVoid(ddiMgr);
  Ddi_BddPartInsert(fwdReached, 0, from);

  if (opt->trav.auxVarFile != NULL) {
    Tr_TrAuxVars(bckTr, opt->trav.auxVarFile);
  }
  if (opt->common.clk != 0) {
    Tr_Tr_t *newTr =
      Tr_TrPartitionWithClock(fwdTr, clock_var, opt->trav.implications);
    if (newTr != NULL) {
      Tr_TrFree(fwdTr);
      fwdTr = newTr;
    }
  }

  if (opt->trav.rPlus != NULL) {
    Ddi_Bdd_t *r;

    if (care != NULL) {
      Ddi_Varset_t *s = Ddi_BddSupp(init);

      r = Ddi_BddDup(care);
      //Ddi_BddExistProjectAcc(r,s);
      Ddi_Free(s);
    } else if (strcmp(opt->trav.rPlus, "1") == 0) {
      r = Ddi_BddMakeConst(ddiMgr, 1);
    } else {
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "-- Reading CARE set from %s.\n", opt->trav.rPlus));
      r = Ddi_BddLoad(ddiMgr, DDDMP_VAR_MATCHNAMES,
        DDDMP_MODE_DEFAULT, opt->trav.rPlus, NULL);
    }
#if 1
    Ddi_BddPartInsertLast(fwdReached, r);
#else
    care = Ddi_BddDup(r);
#endif
    Ddi_Free(r);
    Ddi_Free(from);
  } else if (opt->trav.approxMBM) {
    Ddi_Bdd_t *r;
    Trav_Mgr_t *travMgr = Trav_MgrInit(NULL, ddiMgr);

    Trav_MgrSetFromSelect(travMgr, Trav_FromSelectRestrict_c);

    Ddi_BddAndAcc(from, invar);
    FbvSetTravMgrOpt(travMgr, opt);

    Trav_MgrSetI(travMgr, Tr_MgrReadI(trMgr));
    Trav_MgrSetPS(travMgr, Tr_MgrReadPS(trMgr));
    Trav_MgrSetNS(travMgr, Tr_MgrReadNS(trMgr));
    Trav_MgrSetFrom(travMgr, from);
    Trav_MgrSetReached(travMgr, from);
    Trav_MgrSetTr(travMgr, fwdTr);
    if (((Pdtutil_VerbLevel_e) opt->verbosity) >= Pdtutil_VerbLevelNone_c &&
      ((Pdtutil_VerbLevel_e) opt->verbosity) <= Pdtutil_VerbLevelSame_c) {
      Trav_MgrSetVerbosity(travMgr, Pdtutil_VerbLevel_e(opt->verbosity));
    }


    if (opt->trav.approxMBM > 1) {
      r = Trav_ApproxLMBM(travMgr, delta);
    } else {
      r = Trav_ApproxMBM(travMgr, delta);
    }

    Ddi_BddPartInsertLast(fwdReached, r);
    Ddi_Free(from);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "-- TRAV Manager Free.\n"));

    Trav_MgrQuit(travMgr);
  } else {

    Ddi_BddAndAcc(from, invar);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "Approximate Forward.\n"));

    if (opt->trav.constrain) {
      Tr_MgrSetImgMethod(Tr_TrMgr(fwdTr), Tr_ImgMethodCofactor_c);
    }

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "|TR|=%d\n", Ddi_BddSize(Tr_TrBdd(fwdTr))));

    saveClipTh = ddiMgr->exist.clipThresh;
    ddiMgr->exist.clipThresh = 100000000;

    doApproxRange = (opt->trav.maxFwdIter == 0);
    for (i = 0, fixPoint = doApproxRange; (!fixPoint); i++) {

      if (i >= opt->trav.nExactIter) {
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
          fprintf(stdout,
            "-- Approximate Forward Verification; #Iteration=%d.\n", i + 1));

#if 0
        if (clock_tr != NULL) {
          Ddi_BddSetPartConj(from);
          Ddi_BddPartInsert(from, 0, clock_tr);
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "#"));
        }
#endif
        if (0 && opt->trav.fbvMeta) {
          int initSize = Ddi_BddSize(from);

          Ddi_BddSetMeta(from);
          if (i > 0) {
            Ddi_Bdd_t *r = Ddi_BddMakeMeta(Ddi_BddPartRead(fwdReached, i - 1));

            Ddi_BddDiffAcc(from, r);
            Ddi_Free(r);
          }

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "From: part:%d -> meta:%d.\n",
              initSize, Ddi_BddSize(from)));
        }
        to = Tr_ImgApprox(fwdTr, from, care);

        if (satRefine) {
          Ddi_Bdd_t *ref =
            Trav_ApproxSatRefine(to, from, trAig, prevRef, ps, ns, init,
            satRefine, 0);

          Ddi_Free(ref);
        }

        if (0 && opt->trav.fbvMeta) {
          Ddi_Bdd_t *r = Ddi_BddMakeMeta(Ddi_BddPartRead(fwdReached, i));

          Ddi_BddOrAcc(to, r);
          Ddi_Free(r);
          Ddi_BddSetPartConj(to);
        }
      } else {
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "-- Exact Forward Verification; #Iteration=%d.\n",
            i + 1));

        if (opt->trav.fbvMeta) {
          int initSize = Ddi_BddSize(from);

          Ddi_BddSetMeta(from);
          if (i > 0) {
            Ddi_Bdd_t *r = Ddi_BddMakeMeta(Ddi_BddPartRead(fwdReached, i - 1));

            Ddi_BddDiffAcc(from, r);
            Ddi_Free(r);
          }
          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "From: part:%d -> meta:%d.\n",
              initSize, Ddi_BddSize(from)));
        }
        to = Tr_Img(fwdTr, from);
        if (opt->trav.fbvMeta) {
          Ddi_Bdd_t *r = Ddi_BddMakeMeta(Ddi_BddPartRead(fwdReached, i));

          Ddi_BddOrAcc(to, r);
          Ddi_Free(r);
          Ddi_BddSetPartConj(to);
        }
      }

      /*      Ddi_BddAndAcc(from,invarspec); */

      if (opt->trav.approxNP > 0) {
        to =
          Ddi_BddEvalFree(Tr_TrProjectOverPartitions(to, NULL, NULL,
            opt->trav.approxNP, opt->trav.approxPreimgTh), to);
      }
      /*Ddi_BddSetMono(to); */

      if (opt->trav.fromSel != 'n') {
        tmp = Ddi_BddAnd(init, to);
        Ddi_BddSetMono(tmp);
        if (!Ddi_BddIsZero(tmp)) {
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "Init States is Reachable from INIT.\n"));
        } else {
          if (!opt->trav.strictBound) {
            if (1 || (clock_var_ns == NULL) || (i % 2 == 0)) {
              orProjectedSetAcc(to, init, clock_var);
            }
          }
        }
        Ddi_Free(tmp);
      }

      Ddi_Free(from);

      from = Ddi_BddDup(to);
      if (Ddi_BddIsMeta(to)) {
        Ddi_BddSetPartConj(to);
      }

      if (opt->trav.fromSel == 'n') {
        Ddi_Bdd_t *r;

        r = Ddi_BddDup(Ddi_BddPartRead(fwdReached, i));
        if (0) {
          diffProjectedSetAcc(from, r, clock_var);
          orProjectedSetAcc(r, to, clock_var);
          Ddi_Free(to);
          to = r;
        } else {
          orProjectedSetAcc(to, r, clock_var);
          Ddi_Free(r);
        }
      }

      if (0 && satRefine && opt->trav.maxFwdIter < 0) {
        Ddi_Bdd_t *prev = Ddi_BddPartRead(fwdReached,
          Ddi_BddPartNum(fwdReached) - 1);

        if (Ddi_BddIsPartConj(prev)) {
          Ddi_Bdd_t *satPart = Ddi_BddPartRead(to, Ddi_BddPartNum(to) - 1);
          Ddi_Bdd_t *satPart0 =
            Ddi_BddPartRead(prev, Ddi_BddPartNum(prev) - 1);
          Ddi_BddOrAcc(satPart, satPart0);
        }
      }
      Ddi_BddPartInsertLast(fwdReached, to);

      if ((opt->trav.maxFwdIter >= 0) && (i >= opt->trav.maxFwdIter - 1)) {
        doApproxRange = 1;
        fixPoint = 1;
      } else if (satRefine) {
        int k;

        for (k = i; k >= 0 && k >= i - 2; k--) {
          fixPoint = Ddi_BddIncluded(to, Ddi_BddPartRead(fwdReached, k));
          if (fixPoint) {
            break;
          }
        }
      } else {
        int k;

        for (k = i; k >= 0; k--) {
          fixPoint = Ddi_BddEqual(to, Ddi_BddPartRead(fwdReached, k));
          if (fixPoint) {
            break;
          }
        }
      }

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "|To|=%d; ", Ddi_BddSize(to));
        fprintf(stdout, "|Reached|=%d", Ddi_BddSize(fwdReached));
        if (Ddi_BddIsMono(to)) {
        fprintf(stdout, "; #ToStates=%g",
            Ddi_BddCountMinterm(to, Ddi_VararrayNum(ps)));}
        fprintf(stdout, ".\n");) ;
      Ddi_Free(to);
    }

    ddiMgr->exist.clipThresh = saveClipTh;

    Ddi_Free(from);

    if (doApproxRange) {
      int k;

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "Forward Verisification; Iteration Limit Reached.\n");
        fprintf(stdout, "Approximate Range used for Outer Rplus.\n"));

      from = Ddi_BddMakeConst(ddiMgr, 1);
      for (k = 0; k < opt->trav.gfpApproxRange; k++) {
        to = Tr_ImgApprox(fwdTr, from, NULL);
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
          fprintf(stdout, "|GFP range[%d]|=%d.\n", k, Ddi_BddSize(to)));
        Ddi_Free(from);
        from = to;
      }
      orProjectedSetAcc(to, init, NULL);
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "|Rplus(range)|=%d\n", Ddi_BddSize(to)));
      Ddi_BddPartInsertLast(fwdReached, to);
      Ddi_Free(to);
    }

  }

  Ddi_Free(clock_tr);

  if (satRefineInductive) {
    int nv = Ddi_VararrayNum(Tr_MgrReadPS(trMgr));
    double cnt0, cnt1;
    Ddi_Bdd_t *ref;
    Ddi_Bdd_t *r = Ddi_BddPartRead(fwdReached, Ddi_BddPartNum(fwdReached) - 1);

    cnt0 = Ddi_BddCountMinterm(r, nv);
    ref =
      Trav_ApproxSatRefine(r, r, trAig, NULL, ps, ns, init, satRefineInductive,
      1);
    cnt1 = Ddi_BddCountMinterm(r, nv);
    Pdtutil_VerbosityMgrIf(trMgr, Pdtutil_VerbLevelUsrMax_c) {
      fprintf(stdout, "#R states: %g -> %g (sat tightening: %g)\n",
        cnt0, cnt1, cnt0 / cnt1);
    }
    Ddi_Free(ref);
  }

  if (opt->trav.wR != NULL) {
    Ddi_Bdd_t *r =
      Ddi_BddDup(Ddi_BddPartRead(fwdReached, Ddi_BddPartNum(fwdReached) - 1));
    Ddi_SetName(r, "rPlus");
    if (Ddi_BddIsPartConj(r)) {
      int i;

      if (care != NULL) {
        Ddi_BddPartInsertLast(r, care);
      }
      for (i = 0; i < Ddi_BddPartNum(r); i++) {
        char name[100];

        sprintf(name, "rPlus_part_%d", i);
        Ddi_SetName(Ddi_BddPartRead(r, i), name);
      }
    } else if (care != NULL) {
      Ddi_BddAndAcc(r, care);
    }

    if (strstr(opt->trav.wR, ".blif") != NULL) {
      //Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      //  Pdtutil_VerbLevelNone_c,
      //  fprintf(stdout, "Writing outer reached ring to %s (BLIF file).\n",
      //    opt->trav.wR));
      Ddi_BddStoreBlif(r, NULL, NULL, "reachedPlus", opt->trav.wR, NULL);
    } else if (strstr(opt->trav.wR, ".aig") != NULL) {
      int k;
      Ddi_Bddarray_t *rings = Ddi_BddarrayMakeFromBddPart(fwdReached);

      if (satRefineInductive) {
        Ddi_Free(rings);
        rings = Ddi_BddarrayAlloc(ddiMgr, 1);
        Ddi_BddarrayWrite(rings, 0, Ddi_BddPartRead(fwdReached,
            Ddi_BddPartNum(fwdReached) - 1));
      }

      Ddi_MgrReduceHeap(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"), 0);
      for (k = 0; k < Ddi_BddarrayNum(rings); k++) {
        Ddi_Bdd_t *rA = Ddi_BddarrayRead(rings, k);

        Ddi_BddSetAig(rA);
      }
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Writing reached rings to %s (AIGER file).\n",
          opt->trav.wR));
      Ddi_AigarrayNetStoreAiger(rings, 0, opt->trav.wR);
      Ddi_Free(rings);
    } else {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Writing outer reached ring to %s\n", opt->trav.wR));
      Ddi_BddStore(r, "reachedPlus", DDDMP_MODE_TEXT, opt->trav.wR, NULL);
    }

    if (0 && opt->trav.wP != NULL) {
      Tr_TrWritePartitions(fwdTr, opt->trav.wP);
    }

    if (opt->trav.countReached) {
      int nv = Ddi_VararrayNum(Tr_MgrReadPS(trMgr));
      Ddi_Var_t *cvarPs = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");

      Ddi_Bdd_t *myR = Ddi_BddMakeMono(r);

      if (0 && cvarPs != NULL) {
        Ddi_BddCofactorAcc(myR, cvarPs, 1);
        nv--;
      }

      if (0 || (Ddi_BddSize(r) < 100)) {
        Ddi_BddSetMono(r);
      }
      // nxr: indent fa casino se si lascia la macro
      //Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
      if (ddiMgr->settings.verbosity >= Pdtutil_VerbLevelUsrMax_c) {
        fprintf(stdout, "|Reached(mono)|=%d; NV: %d\n", Ddi_BddSize(r), nv);
        if (nv < 10000) {
          double ns = Ddi_BddCountMinterm(myR, nv);
          double space = pow(2, nv);

          fprintf(stdout, "#ReachedStates=%g. Space=%g\n", ns, space);
          fprintf(stdout, "space density=%g.\n", ns / space);
#if 0
          fprintf(stdout, "#R States(estimated)=%g\n",
            Ddi_AigEstimateMintermCount(myR, Ddi_VararrayNum(ps)));
#endif
        } else {
          double res = Ddi_BddCountMinterm(myR, nv - 1000);
          double fact = pow((pow(2, 100) / pow(10, 30)), 10);

          res = res * fact;
          fprintf(stdout, "#Reached states = %g * 10^300\n", res);
        }
      }
      //);
      Ddi_Free(myR);
    }

    Ddi_Free(r);

    exit(1);
  } else if (opt->trav.countReached) {
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c, {
      Ddi_Bdd_t * ra = Ddi_BddMakeAig(Ddi_BddPartRead(fwdReached,
            Ddi_BddPartNum(fwdReached) - 1));
        fprintf(stdout, "#R States(estimated)=%g\n",
          Ddi_AigEstimateMintermCount(ra, Ddi_VararrayNum(ps)));
        Ddi_Bdd_t * r = Ddi_BddMakeMono(Ddi_BddPartRead(fwdReached,
            Ddi_BddPartNum(fwdReached) - 1));
        fprintf(stdout, "|Reached(mono)|=%d; ", Ddi_BddSize(r));
        fprintf(stdout, "#ReachedStates=%g.\n",
          Ddi_BddCountMinterm(r, Ddi_VararrayNum(Tr_MgrReadPS(trMgr))));
        Ddi_Free(ra); Ddi_Free(r);}
    );
  }

  /* Ddi_BddSetMono(fwdReached); */

  /* exact backward */

#if 1
  if (opt->trav.fbvMeta && opt->trav.metaMethod == Ddi_Meta_EarlySched) {
    Ddi_MetaInit(Ddi_ReadMgr(init), Ddi_Meta_EarlySched, Tr_TrBdd(tr), NULL,
      Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr), opt->trav.fbvMeta);
    Ddi_MgrSetMetaMaxSmooth(ddiMgr, opt->common.metaMaxSmooth);
    /*Ddi_BddSetMeta(fwdReached); */
  }
#endif

  /* clipLevel = opt->ddi.saveClipLevel; */
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c, Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "-- Exact Backward; Using Fix Point R+.\n"));

#if 1
  opt->tr.auxFwdTr = fwdTr;
  Tr_MgrSetImgApproxMaxIter(trMgr, 1);
#else
  Tr_TrFree(fwdTr);
#endif

  if (opt->trav.bwdTrClustFile != NULL) {
    Tr_Tr_t *newTr = Tr_TrManualPartitions(bckTr, opt->trav.bwdTrClustFile);

    Tr_TrFree(bckTr);
    bckTr = newTr;
  } else if (opt->trav.sort_for_bck != 0) {
    Tr_TrSortForBck(bckTr);
    Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
    Tr_TrSetClustered(bckTr);
  } else {
    Tr_TrFree(bckTr);
    bckTr = Tr_TrDup(fwdTr);
  }

  verifOK = 1;

  Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodIwls95_c);
#if 0
  Tr_MgrSetClustThreshold(trMgr, 100000);
#endif

#if 0
#if 1
  Ddi_BddConstrainAcc(Tr_TrBdd(bckTr), fwdReached);
#endif
#endif

#if 0
  Tr_TrSortIwls95(bckTr);
  Tr_TrSetClustered(bckTr);
#endif

  notInit = Ddi_BddNot(init);

  from = Ddi_BddMakeConst(ddiMgr, 0);
  for (i = Ddi_BddPartNum(fwdReached) - 1; i >= 0; i--) {
    Ddi_Free(from);
    from = Ddi_BddAnd(Ddi_BddPartRead(fwdReached, i), badStates);
    if (!Ddi_BddIsZero(from))
      break;
  }
  Ddi_Free(from);
  from = Ddi_BddDup(badStates);

  Ddi_BddRestrictAcc(Tr_TrBdd(bckTr),
    Ddi_BddPartRead(fwdReached, Ddi_BddPartNum(fwdReached) - 1));

  if (opt->common.clk != 0) {
    Tr_Tr_t *newTr = Tr_TrPartition(bckTr, opt->common.clk, 1);

    if (newTr != NULL) {
      Ddi_Bdd_t *ck_p0, *ck_p1;

      Tr_TrFree(bckTr);
      bckTr = newTr;
      ck_p0 = Ddi_BddPartExtract(Ddi_BddPartRead(Tr_TrBdd(bckTr), 0), 0);
      ck_p1 = Ddi_BddPartExtract(Ddi_BddPartRead(Tr_TrBdd(bckTr), 1), 0);
      Ddi_BddSetClustered(Tr_TrBdd(bckTr), opt->trav.clustTh);
      Ddi_BddSetPartConj(Ddi_BddPartRead(Tr_TrBdd(bckTr), 0));
      Ddi_BddSetPartConj(Ddi_BddPartRead(Tr_TrBdd(bckTr), 1));
      Ddi_BddPartInsert(Ddi_BddPartRead(Tr_TrBdd(bckTr), 0), 0, ck_p0);
      Ddi_BddPartInsert(Ddi_BddPartRead(Tr_TrBdd(bckTr), 1), 0, ck_p1);
      Ddi_Free(ck_p0);
      Ddi_Free(ck_p1);
    } else {
      Ddi_BddSetClustered(Tr_TrBdd(bckTr), opt->trav.clustTh);
    }

    {
      Ddi_Bdd_t *p0, *p1, *f, *fpart;

      if (opt->trav.trDpartTh > 0) {
        p0 = Ddi_BddPartRead(Tr_TrBdd(bckTr), 0);
        p1 = Ddi_BddPartRead(Tr_TrBdd(bckTr), 1);
        f = Ddi_BddPartRead(p0, 1);
        fpart = Part_PartitionDisjSet(f, NULL, NULL,
          /*Part_MethodCofactor_c, */ Part_MethodCofactorSupport_c,
          opt->trav.trDpartTh, 3, Pdtutil_VerbLevelDevMax_c);
        Ddi_BddPartWrite(p0, 1, fpart);
        Ddi_Free(fpart);
        f = Ddi_BddPartRead(p1, 1);
        fpart = Part_PartitionDisjSet(f, NULL, NULL,
          /*Part_MethodCofactor_c, */ Part_MethodCofactorSupport_c,
          2 * Ddi_BddSize(f) / 3, 3, Pdtutil_VerbLevelDevMax_c);
        Ddi_BddPartWrite(p1, 1, fpart);
        Ddi_Free(fpart);
      }

    }

  } else {
    Ddi_BddSetClustered(Tr_TrBdd(bckTr), opt->trav.clustTh);

    if (opt->trav.trDpartTh > 0) {
      Ddi_Bdd_t *newTrBdd, *f;

      f = Tr_TrBdd(bckTr);
      newTrBdd = Part_PartitionDisjSet(f, NULL, NULL, Part_MethodCofactor_c,
        opt->trav.trDpartTh, 3, Pdtutil_VerbLevelDevMax_c);
      TrTrRelWrite(bckTr, newTrBdd);
      Ddi_Free(newTrBdd);
    }
  }

  if (opt->trav.noCheck) {
    Ddi_BddAndAcc(init, Ddi_MgrReadZero(ddiMgr));
  }


  if (opt->trav.wP != NULL) {
    char name[200];

    sprintf(name, "fwd-appr-%s", opt->trav.wP);
    Tr_TrWritePartitions(fwdTr, name);
    sprintf(name, "%s", opt->trav.wP);
    Tr_TrWritePartitions(bckTr, name);
  }
  if (opt->trav.wOrd != NULL) {
    char name[200];

    sprintf(name, "%s.0", opt->trav.wOrd);
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "Writing intermediate variable order to file %s\n",
        name));
    Ddi_MgrOrdWrite(ddiMgr, name, NULL, Pdtutil_VariableOrderDefault_c);
  }

  if (Ddi_BddIsAig(init)) {
    if (invar != NULL && !Ddi_BddIsOne(invar)) {
      Ddi_Bdd_t *i1 = Ddi_BddMakeAig(invar);

      Ddi_BddAndAcc(init, i1);
      Ddi_Free(i1);
    }
  }

  do {
    opt->trav.univQuantifyDone = 0;
    if (!opt->trav.prioritizedMc) {
      bckReached = Ddi_BddMakePartDisjVoid(ddiMgr);

      /*Tr_TrReverseAcc(bckTr); */
      verifOK = !travOverRingsWithFixPoint(from, NULL, init, bckTr, fwdReached,
        bckReached, NULL, Ddi_BddPartNum(fwdReached),
        "Exact Backward Verification", initialTime, 1 /*reverse */ ,
        0 /*approx */ , opt);
    } else {
      bckReached = FbvBckInvarCheckRecur(from, init, bckTr, fwdReached, opt);
      verifOK = (bckReached == NULL);
    }

    if (1 && opt->trav.univQuantifyDone) {
      int nq;

      Ddi_Free(from);
      from = bckReached;
      nq = Ddi_VarsetNum(opt->trav.univQuantifyVars);
      if (nq < 10) {
        Ddi_Free(opt->trav.univQuantifyVars);
        opt->trav.univQuantifyTh = -1;
      } else {
        for (i = 0; i < nq / 2; i++) {
          Ddi_Var_t *v = Ddi_VarsetTop(opt->trav.univQuantifyVars);

          Ddi_VarsetRemoveAcc(opt->trav.univQuantifyVars, v);
        }
      }
    }

  } while (1 && opt->trav.univQuantifyDone);

  frontier = NULL;

  if (opt->trav.wBR != NULL) {
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "Writing backward reached to %s\n", opt->trav.wBR));
    Ddi_BddStore(bckReached, "backReached", DDDMP_MODE_TEXT, opt->trav.wBR,
      NULL);
  }

  Tr_TrFree(opt->tr.auxFwdTr);

  if (!verifOK) {
    frontier = Ddi_BddarrayMakeFromBddPart(bckReached);
    fwdRings = 0;
#if 0
    Ddi_Bdd_t *overApprox = Ddi_BddDup(fwdReached);

    Ddi_BddPartWrite(overApprox, 0, init);
    /* opt->trav.constrain_level = 0; */
    verifOK = !travOverRings(init, NULL, badStates, tr, bckReached,
      fwdReached, overApprox, Ddi_BddPartNum(bckReached),
      "Exact fwd iteration", initialTime, 0 /*reverse */ , 0 /*approx */ ,
      opt);

    Ddi_Free(overApprox);
    frontier = Ddi_BddarrayMakeFromBddPart(fwdReached);
    fwdRings = 1;
#endif
  }

  Ddi_Free(trAig);
  Ddi_Free(prevRef);
  Tr_TrFree(bckTr);
  Ddi_Free(bckReached);
  Ddi_Free(from);
  Ddi_Free(invar);
  Ddi_Free(notInit);
  Ddi_Free(fwdReached);
  //  Ddi_Free(care);

  if (verifOK) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: PASS.\n"));
  } else {
    FILE *fp;
    Trav_Mgr_t *travMgr;
    Ddi_Bddarray_t *patternMisMat;
    Ddi_Varset_t *piv;
    Ddi_Bdd_t *invspec_bar, *end;

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: FAIL.\n"));

    Tr_MgrSetClustSmoothPi(trMgr, 0);
    travMgr = Trav_MgrInit(NULL, ddiMgr);
    Trav_MgrSetMismatchOptLevel(travMgr, opt->trav.mism_opt_level);

#if 0
    ddiMgr->exist.clipThresh = 100000000;
#else
    ddiMgr->exist.clipLevel = 0;
#endif

#if 1
    piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));
#if 0
    /* keep PIs for counterexample */
    invspec_bar = Ddi_BddExist(invarspec, piv);
#else
    invspec_bar = Ddi_BddNot(invarspec);
#endif

#if 0
    Tr_MgrSetImgMethod(Tr_TrMgr(tr), Tr_ImgMethodCofactor_c);
#endif

    if (0 && opt->ddi.exist_opt_level > 2) {
      Ddi_MgrSetExistOptLevel(ddiMgr, 2);
    }

    if (fwdRings) {

#if 0
      if (opt->common.clk != 0) {
        Tr_Tr_t *newTr =
          Tr_TrPartitionWithClock(tr, clock_var, opt->trav.implications);
        if (newTr != NULL) {
          Tr_TrFree(tr);
          tr = newTr;
        }
      }
#endif

      Tr_TrReverseAcc(tr);
      patternMisMat = Trav_MismatchPat(travMgr, tr, invspec_bar,
        NULL, NULL, &end, frontier,
        Tr_MgrReadPS(Tr_TrMgr(tr)), Tr_MgrReadNS(Tr_TrMgr(tr)), piv);
      Tr_TrReverseAcc(tr);
    } else {
      patternMisMat = Trav_MismatchPat(travMgr, tr, NULL,
        invspec_bar, &end, NULL, frontier,
        Tr_MgrReadPS(Tr_TrMgr(tr)), Tr_MgrReadNS(Tr_TrMgr(tr)), piv);
    }

    /*---------------------------- Write Patterns --------------------------*/

    if (patternMisMat != NULL) {
      if (opt->trav.cex == 1) {
        FbvBddCexToFsm(fsmMgr, patternMisMat, end, 1 /* reverse */ );
      } else {
        char filename[100];

        sprintf(filename, "%s%d.txt", FILEPAT, getpid());
        fp = fopen(filename, "w");
        //fp = fopen ("pdtpat.txt", "w");
        if (Ddi_BddarrayPrintSupportAndCubes(patternMisMat, 1, -1,
            0, 0, NULL, fp) == 0) {
          Pdtutil_Warning(1, "Store cube Failed.");
        }
        fclose(fp);
      }
    }

    /*----------------------- Write Initial State Set ----------------------*/

    if (end != NULL && !Ddi_BddIsZero(end)) {
      char filename[100];

      sprintf(filename, "%s%d.txt", FILEINIT, getpid());
      fp = fopen(filename, "w");
      //fp = fopen ("pdtinit.txt", "w");
      if (Ddi_BddPrintSupportAndCubes(end, 1, -1, 0, NULL, fp) == 0) {
        Pdtutil_Warning(1, "Store cube Failed.");
      }
      fclose(fp);
      Ddi_Free(end);
    }

    Ddi_Free(piv);
    Ddi_Free(invspec_bar);
    Ddi_Free(patternMisMat);

#endif

  }

  Ddi_Free(frontier);
  Ddi_Free(badStates);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c,
    currTime = util_cpu_time();
    fprintf(stdout, "Live nodes (peak): %u(%u)\n",
      Ddi_MgrReadLiveNodes(ddiMgr), Ddi_MgrReadPeakLiveNodeCount(ddiMgr));
    // fprintf(stdout, "TotalTime: %s ", util_print_time (currTime-opt->stats.startTime));
    fprintf(stdout, "TotalTime: %s ",
      util_print_time((opt->stats.setupTime - opt->stats.startTime) +
        (currTime - initialTime)));
    fprintf(stdout, "(setup: %s - ",
      util_print_time(opt->stats.setupTime - opt->stats.startTime));
    //fprintf(stdout, "(setup: %s -", util_print_time (initialTime-opt->stats.startTime));
    fprintf(stdout, " trav: %s)\n", util_print_time(currTime - initialTime))
    );

  return (verifOK);
}


/**Function*******************************************************************
  Synopsis    [exact forward verification]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
FbvApproxForwExactBackwVerify(
  Fsm_Mgr_t * fsmMgr,
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  int doFwd,
  Fbv_Globals_t * opt
)
{
  int i, j, backi, verifOK, fixPoint, initReachable;
  Ddi_Bdd_t *tmp, *from, *fwdReached, *bckReached, *to,
    *notInit, *care, *invar, *badi, *ring, *badStates;
  Ddi_Vararray_t *ps;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(init);
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Tr_Tr_t *bckTr, *bckTr0, *fwdTr;
  long initialTime, currTime;
  Ddi_Bddarray_t *frontier;

  ps = Tr_MgrReadPS(trMgr);

  badStates = Ddi_BddNot(invarspec);

  initialTime = util_cpu_time();

  {
    Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));

    Ddi_BddExistAcc(badStates, piv);
    Ddi_Free(piv);
  }

#if ENABLE_INVAR
  invar = Ddi_BddPartExtract(Tr_TrBdd(tr), 0);
#else
  invar = Ddi_BddMakeConst(ddiMgr, 1);
#endif

  Pdtutil_VerbosityMgrIf(ddiMgr, Pdtutil_VerbLevelUsrMed_c) {
    fprintf(stdout, "|Invar|=%d; ", Ddi_BddSize(invar));
    fprintf(stdout, "|Tr|=%d.\n", Ddi_BddSize(Tr_TrBdd(tr)));
  }

  fwdTr = Tr_TrDup(tr);
  bckTr = Tr_TrDup(tr);

  Tr_MgrSetClustThreshold(trMgr, 1);
  Tr_TrSetClustered(tr);
  Tr_TrSortIwls95(tr);
  Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
  Tr_TrSetClustered(tr);

  Tr_MgrSetClustSmoothPi(trMgr, 1);
  Tr_MgrSetClustThreshold(trMgr, 1);
  Tr_TrSetClustered(fwdTr);
  Tr_TrSortIwls95(fwdTr);
  Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
  Tr_TrSetClustered(fwdTr);

  if (opt->trav.sort_for_bck != 0) {
    Tr_TrSortForBck(bckTr);
    Tr_TrSetClustered(bckTr);
  } else {
    Tr_TrFree(bckTr);
    bckTr = Tr_TrDup(fwdTr);
  }


#if 1
  for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(fwdTr));) {
    Ddi_Bdd_t *group, *p;

    group = Ddi_BddPartRead(Tr_TrBdd(fwdTr), j);
    Ddi_BddSetPartConj(group);
    p = Ddi_BddPartRead(Tr_TrBdd(fwdTr), i);
    Ddi_BddPartInsertLast(group, p);
    if (Ddi_BddSize(group) > opt->trav.apprGrTh) {
      /* resume from group insert */
      p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
      Ddi_Free(p);
      i++;
      j++;
    } else {
      p = Ddi_BddPartExtract(Tr_TrBdd(fwdTr), i);
      Ddi_Free(p);
    }
  }
#endif

  if (doFwd) {

    /* approx forward */

    from = Ddi_BddDup(init);
    Ddi_BddAndAcc(from, invar);

    /* disjunctively partitioned reached */
    fwdReached = Ddi_BddMakePartDisjVoid(ddiMgr);
    Ddi_BddPartInsert(fwdReached, 0, from);


    Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodApprox_c);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "-- Approximate Forward.\n"));

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|TR|=%d\n", Ddi_BddSize(Tr_TrBdd(fwdTr))));

    initReachable = 0;

    for (i = 0, fixPoint = 0;
      (!fixPoint) && ((opt->trav.bound < 0) || (i < opt->trav.bound)); i++) {

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout,
          "-- Approximate Forward Verification; #Iteration=%d.\n", i + 1));

      to = Tr_Img(fwdTr, from);
      if (opt->trav.approxNP > 0) {
        to =
          Ddi_BddEvalFree(Tr_TrProjectOverPartitions(to, NULL, NULL,
            opt->trav.approxNP, opt->trav.approxPreimgTh), to);
      }

      /*Ddi_BddSetMono(to); */
      /*if (i==0) */  {
        tmp = Ddi_BddAnd(init, to);
        Ddi_BddSetMono(tmp);
        if (!Ddi_BddIsZero(tmp)) {
          initReachable = i + 1;

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "Init States are Reachable from Init.\n"));
        } else {
          if (!opt->trav.strictBound)
            orProjectedSetAcc(to, init, NULL);
        }
        Ddi_Free(tmp);
      }
      Ddi_Free(from);
      Ddi_BddPartInsertLast(fwdReached, to);

      from = Ddi_BddDup(to);
#if 0
      fixPoint = Ddi_BddIsZero(tmp);
#endif

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "|To|=%d; ", Ddi_BddSize(to));
        fprintf(stdout, "|Reached|=%d.\n", Ddi_BddSize(fwdReached)));

#if 1
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        if (Ddi_BddIsMono(to)) {
        fprintf(stdout, "#To States = %g\n",
            Ddi_BddCountMinterm(to, Ddi_VararrayNum(ps)));}
      ) ;
#endif
      Ddi_Free(to);
    }

    Ddi_Free(from);

  } else {
    fwdReached = Ddi_BddMakeConst(ddiMgr, 1);
    Ddi_BddAndAcc(fwdReached, invar);
  }

  Tr_TrFree(fwdTr);

  /* Ddi_BddSetMono(fwdReached); */

  /* exact backward */

#if 0
  if (opt->trav.fbvMeta) {
    Ddi_MetaInit(Ddi_ReadMgr(init), opt->trav.metaMethod, Tr_TrBdd(tr), NULL,
      Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr), 100);
  }
  if (opt->trav.fbvMeta) {
    Ddi_BddSetMeta(fwdReached);
  }
#endif

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
    fprintf(stdout, "-- Exact Backward.\n"));

  verifOK = 1;
#if 0
  bckReached = Ddi_BddAnd(fwdReached, badStates);
#else
  bckReached = Ddi_BddDup(badStates);
#endif
  /*Ddi_BddAndAcc(bckReached,invar); */

  Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodIwls95_c);
#if 0
  Tr_MgrSetClustThreshold(trMgr, 100000);
#endif

#if 0
#if 1
  Ddi_BddConstrainAcc(Tr_TrBdd(bckTr), fwdReached);
  /*  Ddi_BddConstrainAcc(bckReached,fwdReached); */
#else
  Ddi_BddAndAcc(bckReached, fwdReached);
#endif
#endif

  Tr_TrReverseAcc(bckTr);

#if 0
  Tr_TrSortIwls95(bckTr);
  Tr_TrSetClustered(bckTr);
#endif

  notInit = Ddi_BddNot(init);

  from = Ddi_BddMakeConst(ddiMgr, 0);
  for (i = Ddi_BddPartNum(fwdReached) - 1; i >= 0; i--) {
    Ddi_Free(from);
    from = Ddi_BddAnd(Ddi_BddPartRead(fwdReached, i), badStates);
    if (!Ddi_BddIsZero(from))
      break;
  }
  Ddi_Free(from);
  from = Ddi_BddDup(badStates);

  bckTr0 = Tr_TrDup(bckTr);

  frontier = Ddi_BddarrayAlloc(ddiMgr, 1);
  Ddi_BddarrayWrite(frontier, 0, from);

  for (backi = 0; /*!Ddi_BddIsZero(from) */ i >= 0; i--, backi++) {

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "-- Exact Backward Verification; #Iteration=%d.\n",
        i + 1));

#if 0
    care = Ddi_BddNot(bckReached);
    Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
    Ddi_BddRestrictAcc(Tr_TrBdd(bckTr), care);
    Ddi_Free(care);
#endif

    if (opt->trav.constrain_level > 1)
      care = Ddi_BddPartRead(fwdReached, i);
    else
      care = Ddi_MgrReadOne(ddiMgr);

#if 0
    Ddi_BddConstrainAcc(Tr_TrBdd(bckTr), care);
    Ddi_BddConstrainAcc(from, care);
#else
    Ddi_BddRestrictAcc(from, Ddi_BddPartRead(fwdReached, i));
#endif

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|from0|=%d ", Ddi_BddSize(from));
      fprintf(stdout, "|from|=%d\n", Ddi_BddSize(from)));

    if (opt->trav.groupbad) {
      badi = Ddi_BddDup(badStates);
      Ddi_BddConstrainAcc(badi, care);
      Ddi_BddSetMono(badi);
      if (!Ddi_BddIsZero(badi)) {
        Ddi_BddOrAcc(from, badi);
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
          fprintf(stdout, "|from+i|=%d\n", Ddi_BddSize(from)));
      }
      Ddi_Free(badi);
    }

    Ddi_BddarrayWrite(frontier, backi, care);
    ring = Ddi_BddarrayRead(frontier, backi);

#if PARTITIONED_RINGS
    Ddi_BddSetPartConj(ring);
    Ddi_BddPartInsertLast(ring, from);
#else
    Ddi_BddAndAcc(ring, from);
#endif


    if (Ddi_BddIsZero(from))
      continue;

    tmp = Ddi_BddAnd(Ddi_BddPartRead(fwdReached, i), init);
    Ddi_BddSetMono(tmp);
    if (!Ddi_BddIsZero(tmp)) {
      Ddi_BddAndAcc(tmp, from);
      Ddi_BddSetMono(tmp);
      if (!Ddi_BddIsZero(tmp)) {
        verifOK = 0;
        Ddi_Free(tmp);
#if PARTITIONED_RINGS
        Ddi_BddPartInsert(ring, 0, init);
#else
        Ddi_BddAndAcc(ring, init);
#endif
        break;
      }
    }
    Ddi_Free(tmp);

    if (i == 0)
      continue;

    Pdtutil_Assert(i > 0, "Iteration 0 reached");

    if (opt->trav.constrain_level > 1)
      from = Ddi_BddEvalFree(Ddi_BddAnd(care, from), from);

    care = Ddi_BddPartRead(fwdReached, i - 1);

    if (opt->trav.constrain_level > 1) {
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      Ddi_BddConstrainAcc(Tr_TrBdd(bckTr), care);
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
    }

    if (opt->trav.fbvMeta) {
      Ddi_BddSetMono(from);
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "FROM: mono: %d -> ", Ddi_BddSize(from)));
      Ddi_BddSetMeta(from);
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "meta: %d\n", Ddi_BddSize(from)));
    }

    {
#if 0
      Ddi_Bdd_t *myCare = Ddi_BddNot(bckReached);

      Ddi_BddSetPartConj(myCare);
      Ddi_BddPartInsertLast(myCare, care);
#else
      Ddi_Bdd_t *myCare = Ddi_BddDup(care);
#endif
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "|TR|=%d\n", Ddi_BddSize(Tr_TrBdd(bckTr))));
      Ddi_BddSwapVarsAcc(myCare, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      to = Tr_ImgWithConstrain(bckTr, from, myCare);
      Ddi_BddSwapVarsAcc(myCare, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));

      if (opt->trav.fbvMeta) {
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
          fprintf(stdout, "\nTO: meta: %d -> ", Ddi_BddSize(to)));
        Ddi_BddSetMono(to);
        Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
          fprintf(stdout, "mono: %d\n", Ddi_BddSize(to)));
      }
#if 1
      Tr_TrFree(bckTr);
      bckTr = Tr_TrDup(bckTr0);
#endif

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelDevMin_c, LOG_CONST(to, "to")
        );

      if (opt->trav.constrain_level <= 1) {
        Ddi_BddRestrictAcc(to, myCare);
      }

      Ddi_Free(myCare);
    }

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|To|=%d\n", Ddi_BddSize(to)));

    Ddi_Free(from);

#if 0
    care = Ddi_BddNot(bckReached);
    from = Ddi_BddConstrain(to, care);
    Ddi_Free(care);
#else

    if (opt->trav.constrain_level > 0) {
      from = Ddi_BddDup(to);
    } else {
      from = Ddi_BddAnd(care, to);
    }
#endif

#if 0
    if (Ddi_BddSize(from) > Ddi_BddSize(to)) {
#else
    if (0) {
#endif
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "|To|=%d\n", Ddi_BddSize(to)));

      Ddi_Free(from);
      from = Ddi_BddDup(to);
    } else {
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "|To*BRi|=%d\n", Ddi_BddSize(from)));
    }

#if 0
    Ddi_BddConstrainAcc(from, care);
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|To!BRi|=%d\n", Ddi_BddSize(from)));
#endif

#if 0
    Ddi_BddOrAcc(bckReached, to);

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|To|=%d; |Reached|=%d; ", Ddi_BddSize(to),
        Ddi_BddSize(bckReached));
      fprintf(stdout, "#ReachedStates=%g.\n",
        Ddi_BddCountMinterm(bckReached, Ddi_VararrayNum(ps)))
      );
#endif

    currTime = util_cpu_time();

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "Live Nodes (Peak): %u(%u) - Time: %s\n",
        Ddi_MgrReadLiveNodes(ddiMgr),
        Ddi_MgrReadPeakLiveNodeCount(ddiMgr),
        util_print_time(currTime - initialTime))
      );

    Ddi_Free(to);
  }

  Tr_TrFree(bckTr);
  Tr_TrFree(bckTr0);
  Ddi_Free(bckReached);
  Ddi_Free(from);
  Ddi_Free(invar);
  Ddi_Free(notInit);
  Ddi_Free(fwdReached);

  if (verifOK) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: PASS.\n"));
  } else {
    FILE *fp;
    Trav_Mgr_t *travMgr;
    Ddi_Bddarray_t *patternMisMat;
    Ddi_Varset_t *piv;
    Ddi_Bdd_t *invspec_bar, *end;

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: FAIL.\n"));

    Tr_MgrSetClustSmoothPi(trMgr, 0);
    travMgr = Trav_MgrInit(NULL, ddiMgr);
    Trav_MgrSetMismatchOptLevel(travMgr, opt->trav.mism_opt_level);

#if 1
    piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));
#if 0
    /* keep PIs for counterexample */
    invspec_bar = Ddi_BddExist(invarspec, piv);
#else
    invspec_bar = Ddi_BddNot(invarspec);
#endif

#if 0
    Tr_MgrSetImgMethod(Tr_TrMgr(tr), Tr_ImgMethodCofactor_c);
#endif
    patternMisMat = Trav_MismatchPat(travMgr, tr, NULL,
      invspec_bar, &end, NULL, frontier,
      Tr_MgrReadPS(Tr_TrMgr(tr)), Tr_MgrReadNS(Tr_TrMgr(tr)), piv);

    /*---------------------------- Write Patterns --------------------------*/

    if (patternMisMat != NULL) {
      if (opt->trav.cex == 1) {
        FbvBddCexToFsm(fsmMgr, patternMisMat, end, 1 /* reverse */ );
      } else {
        char filename[100];

        sprintf(filename, "%s%d.txt", FILEPAT, getpid());
        fp = fopen(filename, "w");
        //fp = fopen ("pdtpat.txt", "w");
        if (Ddi_BddarrayPrintSupportAndCubes(patternMisMat, 1, -1,
            0, 0, NULL, fp) == 0) {
          Pdtutil_Warning(1, "Store cube Failed.");
        }
      }
      fclose(fp);
    }

    /*----------------------- Write Initial State Set ----------------------*/

    if (end != NULL && !Ddi_BddIsZero(end)) {
      char filename[100];

      sprintf(filename, "%s%d.txt", FILEINIT, getpid());
      fp = fopen(filename, "w");
      //fp = fopen ("pdtinit.txt", "w");
      if (Ddi_BddPrintSupportAndCubes(end, 1, -1, 0, NULL, fp) == 0) {
        Pdtutil_Warning(1, "Store cube Failed.");
      }
      fclose(fp);
      Ddi_Free(end);
    }

    Ddi_Free(piv);
    Ddi_Free(invspec_bar);
    Ddi_Free(patternMisMat);

#endif

  }

  Ddi_Free(frontier);
  Ddi_Free(badStates);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c,
    currTime = util_cpu_time();
    fprintf(stdout, "Live nodes (peak): %u(%u)\n",
      Ddi_MgrReadLiveNodes(ddiMgr), Ddi_MgrReadPeakLiveNodeCount(ddiMgr));
    fprintf(stdout, "TotalTime: %s ",
      util_print_time(currTime - opt->stats.startTime));
    fprintf(stdout, "(setup: %s -",
      util_print_time(opt->stats.setupTime - opt->stats.startTime));
//fprintf(stdout, "(setup: %s -", util_print_time (initialTime-opt->stats.startTime));
    fprintf(stdout, " trav: %s)\n", util_print_time(currTime - initialTime))
    );
  return (verifOK);
}

/**Function*******************************************************************
  Synopsis    [exact forward verification]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
FbvApproxBackwExactForwVerify(
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Fbv_Globals_t * opt
)
{
  int i, j, backi, verifOK, fixPoint, approx;
  Ddi_Bdd_t *tmp, *from, *bckReached, *to, *care, *invar, *ring, *badStates;
  Ddi_Vararray_t *ps;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(init);
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Tr_Tr_t *bckTr, *fwdTr0, *fwdTr;
  long initialTime, currTime;
  Ddi_Bddarray_t *frontier;

  ps = Tr_MgrReadPS(trMgr);

  badStates = Ddi_BddNot(invarspec);

  initialTime = util_cpu_time();

  {
    Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));

    Ddi_BddExistAcc(badStates, piv);
    Ddi_Free(piv);
  }

#if ENABLE_INVAR
  invar = Ddi_BddPartExtract(Tr_TrBdd(tr), 0);
#else
  invar = Ddi_BddMakeConst(ddiMgr, 1);
#endif

  Pdtutil_VerbosityMgrIf(trMgr, Pdtutil_VerbLevelUsrMed_c) {
    fprintf(stdout, "|Invar|=%d; ", Ddi_BddSize(invar));
    fprintf(stdout, "|Tr|=%d.\n", Ddi_BddSize(Tr_TrBdd(tr)));
  }

  fwdTr = Tr_TrDup(tr);

  Tr_MgrSetClustThreshold(trMgr, 1);
  Tr_TrSetClustered(tr);
  Tr_TrSortIwls95(tr);
  Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
  Tr_TrSetClustered(tr);

  Tr_MgrSetClustSmoothPi(trMgr, 1);
  Tr_MgrSetClustThreshold(trMgr, 1);
  Tr_TrSetClustered(fwdTr);
  Tr_TrSortIwls95(fwdTr);
  Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
  Tr_TrSetClustered(fwdTr);

  bckTr = Tr_TrDup(fwdTr);
  /*Tr_TrReverseAcc(bckTr); */

#if 1
  for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(bckTr));) {
    Ddi_Bdd_t *group, *p;

    group = Ddi_BddPartRead(Tr_TrBdd(bckTr), j);
    Ddi_BddSetPartConj(group);
    p = Ddi_BddPartRead(Tr_TrBdd(bckTr), i);
    Ddi_BddPartInsertLast(group, p);
    if (Ddi_BddSize(group) > opt->trav.apprGrTh) {
      /* resume from group insert */
      p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
      Ddi_Free(p);
      i++;
      j++;
    } else {
      p = Ddi_BddPartExtract(Tr_TrBdd(bckTr), i);
      Ddi_Free(p);
    }
  }
#endif

  /* approx forward */

  from = Ddi_BddDup(badStates);
  Ddi_BddAndAcc(from, invar);

  /* disjunctively partitioned reached */
  bckReached = Ddi_BddMakePartDisjVoid(ddiMgr);
  Ddi_BddPartInsert(bckReached, 0, from);

  Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodApprox_c);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "-- Backward Approximate.\n"));

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
    fprintf(stdout, "|TR|=%d\n", Ddi_BddSize(Tr_TrBdd(bckTr))));

  approx = 1;
  for (i = 0, fixPoint = 0;
    (!fixPoint) && ((opt->trav.bound < 0) || (i < opt->trav.bound)); i++) {

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "-- Approximate Backward Verification Iteration: %d.\n",
        i + 1));

    to = FbvApproxPreimg(bckTr, from, Ddi_MgrReadOne(ddiMgr), NULL, 1, opt);
    Ddi_BddSetMono(to);

#if 0
    if (Ddi_BddIsOne(to)) {
      Ddi_Free(to);
      Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodIwls95_c);
      to = Tr_Img(bckTr, from);
      Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodApprox_c);
    }
#endif

    Ddi_Free(from);
    Ddi_BddPartInsertLast(bckReached, to);

    from = Ddi_BddDup(to);
#if 0
    fixPoint = Ddi_BddIsZero(tmp);
#endif

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|To|=%d; ", Ddi_BddSize(to));
      fprintf(stdout, "|Reached|=%d.\n", Ddi_BddSize(bckReached))
      );

#if 1
    if (Ddi_BddIsMono(to)) {
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "#To States = %g.\n",
          Ddi_BddCountMinterm(to, Ddi_VararrayNum(ps))));
    }
#endif
    Ddi_Free(to);
  }

  Ddi_Free(from);

  Tr_TrFree(bckTr);

  /* Ddi_BddSetMono(fwdReached); */

  /* exact backward */

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "-- Exact Backward.\n"));

  verifOK = 1;

  Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodIwls95_c);

  from = Ddi_BddDup(init);

  fwdTr0 = Tr_TrDup(fwdTr);

  frontier = Ddi_BddarrayAlloc(ddiMgr, 1);
  Ddi_BddarrayWrite(frontier, 0, from);

  for (backi = 0; /*!Ddi_BddIsZero(from) */ i >= 0; i--, backi++) {

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "-- Exact Forward Verification; #Iteration=%d.\n",
        i + 1));

    Ddi_BddConstrainAcc(from, Ddi_BddPartRead(bckReached, i));

    Ddi_BddSetMono(from);

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|From|=%d\n", Ddi_BddSize(from)));

    Ddi_BddarrayWrite(frontier, backi, from);
    ring = Ddi_BddarrayRead(frontier, backi);

    if (Ddi_BddIsZero(from))
      continue;

    tmp = Ddi_BddAnd(Ddi_BddPartRead(bckReached, i), badStates);
    Ddi_BddSetMono(tmp);
    if (!Ddi_BddIsZero(tmp)) {
      Ddi_BddAndAcc(tmp, from);
      Ddi_BddSetMono(tmp);
      if (!Ddi_BddIsZero(tmp)) {
        verifOK = 0;
        Ddi_Free(tmp);
        Ddi_BddAndAcc(ring, badStates);
        break;
      }
    }
    Ddi_Free(tmp);

    if (i == 0)
      continue;

    Pdtutil_Assert(i > 0, "Iteration 0 reached");

    care = Ddi_BddPartRead(bckReached, i - 1);

    if (opt->trav.constrain_level > 1) {
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      Ddi_BddConstrainAcc(Tr_TrBdd(fwdTr), care);
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
    }

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|TR|=%d\n", Ddi_BddSize(Tr_TrBdd(fwdTr))));

    to = Tr_ImgWithConstrain(fwdTr, from, NULL);

#if FULL_COFACTOR
    Tr_TrFree(fwdTr);
    fwdTr = Tr_TrDup(fwdTr0);
#endif

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelDevMin_c,
      LOG_CONST(to, "to")
      );

    Ddi_BddConstrainAcc(to, care);

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|To|=%d\n", Ddi_BddSize(to)));

    Ddi_Free(from);

    from = Ddi_BddDup(to);

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|To|=%d ", Ddi_BddSize(to));
      currTime = util_cpu_time();
      fprintf(stdout, "Live nodes (peak): %u(%u) - Time: %s.\n",
        Ddi_MgrReadLiveNodes(ddiMgr),
        Ddi_MgrReadPeakLiveNodeCount(ddiMgr),
        util_print_time(currTime - initialTime)););

    Ddi_Free(to);
  }

  Tr_TrFree(fwdTr);
  Tr_TrFree(fwdTr0);
  Ddi_Free(from);
  Ddi_Free(invar);
  Ddi_Free(bckReached);

  if (verifOK) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: PASS.\n"));
  } else {
#if 0
    FILE *fp;
    Trav_Mgr_t *travMgr;
    Ddi_Bddarray_t *patternMisMat;
    Ddi_Varset_t *piv;
    Ddi_Bdd_t *invspec_bar, *end;
#endif

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: FAIL.\n"));

#if 0

    Tr_MgrSetClustSmoothPi(trMgr, 0);
    travMgr = Trav_MgrInit(NULL, ddiMgr);
    Trav_MgrSetMismatchOptLevel(travMgr, opt->trav.mism_opt_level);

#if 1
    piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));
#if 0
    /* keep PIs for counterexample */
    invspec_bar = Ddi_BddExist(invarspec, piv);
#else
    invspec_bar = Ddi_BddNot(invarspec);
#endif

#if 0
    Tr_MgrSetImgMethod(Tr_TrMgr(tr), Tr_ImgMethodCofactor_c);
#endif
    patternMisMat = Trav_MismatchPat(travMgr, tr, NULL,
      invspec_bar, &end, NULL, frontier,
      Tr_MgrReadPS(Tr_TrMgr(tr)), Tr_MgrReadNS(Tr_TrMgr(tr)), piv);

    /*---------------------------- Write Patterns --------------------------*/

    if (patternMisMat != NULL) {
      char filename[100];

      sprintf(filename, "%s%d.txt", FILEPAT, getpid());
      fp = fopen(filename, "w");
      //fp = fopen ("pdtpat.txt", "w");
      if (Ddi_BddarrayPrintSupportAndCubes(patternMisMat, 1, -1,
          0, 0, NULL, fp) == 0) {
        Pdtutil_Warning(1, "Store cube Failed.");
      }
      fclose(fp);
    }

    /*----------------------- Write Initial State Set ----------------------*/

    if (end != NULL && !Ddi_BddIsZero(end)) {
      char filename[100];

      sprintf(filename, "%s%d.txt", FILEINIT, getpid());
      fp = fopen(filename, "w");
      //fp = fopen ("pdtinit.txt", "w");
      if (Ddi_BddPrintSupportAndCubes(end, 1, -1, 0, NULL, fp) == 0) {
        Pdtutil_Warning(1, "Store cube Failed.");
      }
      fclose(fp);
      Ddi_Free(end);
    }

    Ddi_Free(piv);
    Ddi_Free(invspec_bar);
    Ddi_Free(patternMisMat);

#endif
#endif

  }

  Ddi_Free(frontier);
  Ddi_Free(badStates);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c,
    currTime = util_cpu_time();
    fprintf(stdout, "Live nodes (peak): %u(%u)",
      Ddi_MgrReadLiveNodes(ddiMgr), Ddi_MgrReadPeakLiveNodeCount(ddiMgr));
    // fprintf (stdout, "TotalTime: %s ", util_print_time (currTime-opt->stats.startTime));
    fprintf(stdout, "TotalTime: %s ",
      util_print_time((opt->stats.setupTime - opt->stats.startTime) +
        (currTime - initialTime)));
    fprintf(stdout, "(setup: %s -",
      util_print_time(opt->stats.setupTime - opt->stats.startTime));
//fprintf(stdout, "(setup: %s -", util_print_time (initialTime-opt->stats.startTime));
    fprintf(stdout, " trav: %s).\n", util_print_time(currTime - initialTime))
    );

  return (verifOK);
}

/**Function*******************************************************************
  Synopsis    [exact forward verification]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
FbvApproxForwApproxBckExactForwVerify(
  Tr_Tr_t * tr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  int doApproxBck,
  Fbv_Globals_t * opt
)
{
  int i, j, verifOK, fixPoint, fwdRings;
  Ddi_Bdd_t *tmp, *from, *fwdReached, *bckReached, *to,
    *notInit, *invar, *badStates;
  Ddi_Vararray_t *ps;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(init);
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Tr_Tr_t *bckTr, *fwdTr;
  long initialTime, currTime;
  Ddi_Bddarray_t *frontier;

  Ddi_Bdd_t *clock_tr = NULL;
  Ddi_Var_t *clock_var = NULL;

  if (opt->common.clk != NULL) {
#if 0
    clock_var = Ddi_VarFromName(ddiMgr, opt->common.clk);
#else
    clock_var = Tr_TrSetClkStateVar(tr, opt->common.clk, 1);
    if (opt->common.clk != Ddi_VarName(clock_var)) {
      Ddi_Bdd_t *lit = Ddi_BddMakeLiteral(clock_var, 0);

      opt->common.clk = Ddi_VarName(clock_var);
      Ddi_BddAndAcc(init, lit);
      Ddi_Free(lit);
    }
#endif

    if (clock_var != NULL) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Clock Variable (%s) Detected.\n", opt->common.clk));

      for (i = 0; i < Ddi_BddPartNum(Tr_TrBdd(tr)); i++) {
        Ddi_Varset_t *supp = Ddi_BddSupp(Ddi_BddPartRead(Tr_TrBdd(tr), i));

        /*Ddi_VarsetPrint(supp, 0, NULL, stdout); */
        if (Ddi_VarInVarset(supp, clock_var)) {
          clock_tr = Ddi_BddDup(Ddi_BddPartRead(Tr_TrBdd(tr), i));
          Ddi_Free(supp);

          Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
            fprintf(stdout, "Found Clock TR.\n"));

          break;
        } else {
          Ddi_Free(supp);
        }
      }
    }
  }

  ps = Tr_MgrReadPS(trMgr);

  badStates = Ddi_BddNot(invarspec);

  {
    Ddi_Varset_t *piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));

    Ddi_BddExistAcc(badStates, piv);
    Ddi_Free(piv);
  }

#if ENABLE_INVAR
  invar = Ddi_BddPartExtract(Tr_TrBdd(tr), 0);
#else
  invar = Ddi_BddMakeConst(ddiMgr, 1);
#endif

  Pdtutil_VerbosityMgrIf(trMgr, Pdtutil_VerbLevelUsrMed_c) {
    fprintf(stdout, "|Invar|=%d; ", Ddi_BddSize(invar));
    fprintf(stdout, "|Tr|=%d.\n", Ddi_BddSize(Tr_TrBdd(tr)));
  }

  fwdTr = Tr_TrDup(tr);
  bckTr = Tr_TrDup(tr);

  Tr_MgrSetClustThreshold(trMgr, 1);
  Tr_TrSetClustered(tr);
  Tr_TrSortIwls95(tr);
  Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);


  if (opt->trav.approxTrClustFile != NULL) {
    Tr_Tr_t *newTr = Tr_TrManualPartitions(fwdTr, opt->trav.approxTrClustFile);

    Tr_TrFree(fwdTr);
    fwdTr = newTr;
  }

  if (opt->pre.doCoi == 1 && (opt->tr.coiLimit > 0)) {
    Ddi_Varsetarray_t *coi;
    Tr_Tr_t *reducedTR;
    int n;

    coi = Tr_TrComputeCoiVars(fwdTr, invarspec, opt->tr.coiLimit);
    n = Ddi_VarsetarrayNum(coi);

    reducedTR =
      Tr_TrCoiReductionWithVars(fwdTr, Ddi_VarsetarrayRead(coi, n - 1),
      opt->tr.coiLimit);

    Tr_TrFree(fwdTr);
    fwdTr = reducedTR;

    Ddi_Free(coi);
  }


  Tr_MgrSetClustSmoothPi(trMgr, 1);
  Tr_MgrSetClustThreshold(trMgr, 1);
  Tr_TrSetClustered(fwdTr);
  Tr_TrSortIwls95(fwdTr);
  Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
  Tr_TrSetClustered(fwdTr);

  if (opt->trav.sort_for_bck != 0) {
    Tr_TrSortForBck(bckTr);
    Tr_TrSetClustered(bckTr);
  } else {
    Tr_TrFree(bckTr);
    bckTr = Tr_TrDup(fwdTr);
  }


#if 1
  {
    Ddi_Bdd_t *group, *p;

    for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(fwdTr));) {
      group = Ddi_BddPartRead(Tr_TrBdd(fwdTr), j);
      Ddi_BddSetPartConj(group);
      p = Ddi_BddPartRead(Tr_TrBdd(fwdTr), i);
      Ddi_BddPartInsertLast(group, p);
      if (Ddi_BddSize(group) > opt->trav.apprGrTh) {
        /* resume from group insert */
        p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
        Ddi_Free(p);
        if (clock_tr != NULL) {
          Ddi_BddPartInsert(group, 0, clock_tr);
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
            fprintf(stdout, "#"));
        }
        i++;
        j++;
      } else {
        p = Ddi_BddPartExtract(Tr_TrBdd(fwdTr), i);
        Ddi_Free(p);
      }
    }
    if (clock_tr != NULL) {
      Ddi_BddPartInsert(group, 0, clock_tr);
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "#"));
    }
  }
#if 0
  for (i = 1, j = 0; i < Ddi_BddPartNum(Tr_TrBdd(bckTr));) {
    Ddi_Bdd_t *group, *p;

    group = Ddi_BddPartRead(Tr_TrBdd(bckTr), j);
    Ddi_BddSetPartConj(group);
    p = Ddi_BddPartRead(Tr_TrBdd(bckTr), i);
    Ddi_BddPartInsertLast(group, p);
    if (Ddi_BddSize(group) > 4 * opt->trav.apprGrTh) {
      /* resume from group insert */
      p = Ddi_BddPartExtract(group, Ddi_BddPartNum(group) - 1);
      Ddi_Free(p);
      i++;
      j++;
    } else {
      p = Ddi_BddPartExtract(Tr_TrBdd(bckTr), i);
      Ddi_Free(p);
    }
  }
#endif
#endif

  Ddi_Free(clock_tr);

  initialTime = util_cpu_time();


  /* approx forward */

  from = Ddi_BddDup(init);
  Ddi_BddAndAcc(from, invar);

  /* disjunctively partitioned reached */
  fwdReached = Ddi_BddMakePartDisjVoid(ddiMgr);
  Ddi_BddPartInsert(fwdReached, 0, from);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "-- Approximate Forward.\n");
    fprintf(stdout, "|TR|=%d.\n", Ddi_BddSize(Tr_TrBdd(fwdTr)))
    );

  for (i = 0, fixPoint = 0;
    (!fixPoint) && ((opt->trav.bound < 0) || (i < opt->trav.bound)); i++) {

    if (i >= opt->trav.nExactIter) {
      Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodApprox_c);
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
        fprintf(stdout,
          "-- Approximate Forward Verification; #Iteration=%d.\n", i + 1));
    } else {
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "-- Exact Forward Verification; #Iteration=%d.\n",
          i + 1));
    }

    /*      Ddi_BddAndAcc(from,invarspec); */

    to = Tr_Img(fwdTr, from);

    if (opt->trav.approxNP > 0) {
      to =
        Ddi_BddEvalFree(Tr_TrProjectOverPartitions(to, NULL, NULL,
          opt->trav.approxNP, opt->trav.approxPreimgTh), to);
    }
    /*Ddi_BddSetMono(to); */

    tmp = Ddi_BddAnd(init, to);
    Ddi_BddSetMono(tmp);
    if (!Ddi_BddIsZero(tmp)) {
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Init States are Reachable from Init.\n"));
    } else {
      if (!opt->trav.strictBound /*&& opt->trav.storeCNF == NULL */ )
        orProjectedSetAcc(to, init, NULL);
    }
    Ddi_Free(tmp);

    Ddi_Free(from);
    Ddi_BddPartInsertLast(fwdReached, to);

    from = Ddi_BddDup(to);
#if 0
    fixPoint = Ddi_BddIsZero(tmp);
#endif
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|To|=%d; ", Ddi_BddSize(to));
      fprintf(stdout, "|Reached|=%d; ", Ddi_BddSize(fwdReached));
      if (Ddi_BddIsMono(to)) {
      fprintf(stdout, "#ToStates=%g.\n",
          Ddi_BddCountMinterm(to, Ddi_VararrayNum(ps)));}
    ) ;

    Ddi_Free(to);
  }

  Ddi_Free(from);


  if (opt->trav.wR != NULL) {
    Ddi_MgrReduceHeap(ddiMgr, Ddi_ReorderingMethodString2Enum("sift"), 0);

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "Writing outer reached ring to %s\n", opt->trav.wR));

    Ddi_BddStore(Ddi_BddPartRead(fwdReached, Ddi_BddPartNum(fwdReached) - 1),
      "reachedPlus", DDDMP_MODE_TEXT, opt->trav.wR, NULL);
  }

  /* Ddi_BddSetMono(fwdReached); */

  if ((opt->trav.storeCNF != NULL) && (opt->trav.storeCNFphase == 0)) {
    storeCNFRings(fwdReached, NULL, Tr_TrBdd(bckTr), badStates, opt);
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      currTime = util_cpu_time();
      fprintf(stdout, "Live nodes (peak): %u(%u)\n",
        Ddi_MgrReadLiveNodes(ddiMgr), Ddi_MgrReadPeakLiveNodeCount(ddiMgr));
      // fprintf(stdout, "TotalTime: %s ", util_print_time (currTime-opt->stats.startTime));
      fprintf(stdout, "TotalTime: %s ",
        util_print_time((opt->stats.setupTime - opt->stats.startTime) +
          (currTime - initialTime)));
      fprintf(stdout, "(setup: %s -",
        util_print_time(opt->stats.setupTime - opt->stats.startTime));
//fprintf(stdout, "(setup: %s -", util_print_time (initialTime-opt->stats.startTime));
      fprintf(stdout, " trav: %s)\n", util_print_time(currTime - initialTime))
      );
    Ddi_Free(invar);
    Ddi_Free(badStates);
    Ddi_Free(fwdReached);
    Tr_TrFree(fwdTr);
    Tr_TrFree(bckTr);
    return (1);
  }


  if (0 && opt->pre.doCoi) {
    Ddi_Varsetarray_t *coi;
    int i, j, k, n;

    coi = Tr_TrComputeCoiVars(fwdTr, invarspec, opt->tr.coiLimit);
    n = Ddi_VarsetarrayNum(coi);

    for (i = Ddi_BddPartNum(fwdReached) - 1, j = n - 1; j >= 0; i--, j--) {
      Ddi_Bdd_t *p = Ddi_BddPartRead(fwdReached, i);
      Ddi_Varset_t *vs = Ddi_VarsetarrayRead(coi, j);

      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "Projecting r+[%d] to %d vars\n", i,
          Ddi_VarsetNum(vs)));
      if (Ddi_BddIsMono(p)) {
        Ddi_BddExistProjectAcc(p, vs);
      } else {
        for (k = 0; k < Ddi_BddPartNum(p); k++) {
          Ddi_Bdd_t *pk = Ddi_BddPartRead(p, k);

          Ddi_BddExistProjectAcc(pk, vs);
        }
      }
    }

    Ddi_Free(coi);
  }


  /* exact backward */

#if 0
  if (opt->trav.fbvMeta) {
    Ddi_MetaInit(Ddi_ReadMgr(init), Ddi_Meta_EarlySched, Tr_TrBdd(tr), NULL,
      Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr), 100);
  }
  if (opt->trav.fbvMeta) {
    Ddi_BddSetMeta(fwdReached);
  }
#endif

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "Backward Approximate.\n"));

  verifOK = 1;


  Tr_MgrSetImgMethod(trMgr, Tr_ImgMethodIwls95_c);
#if 0
  Tr_MgrSetClustThreshold(trMgr, 100000);
#endif

#if 0
#if 1
  Ddi_BddConstrainAcc(Tr_TrBdd(bckTr), fwdReached);
#endif
#endif

#if 0
  Tr_TrSortIwls95(bckTr);
  Tr_TrSetClustered(bckTr);
#endif

  notInit = Ddi_BddNot(init);

  if (!doApproxBck && (!Ddi_BddIsOne(invarspec))) {
    for (i = Ddi_BddPartNum(fwdReached) - 2; i >= 0; i--) {
      Ddi_Bdd_t *p = Ddi_BddPartRead(fwdReached, i);

      Ddi_BddSetPartConj(p);
      Ddi_BddPartInsert(p, 0, invarspec);
    }
  }

  from = Ddi_BddDup(badStates);

  bckReached = Ddi_BddMakePartDisjVoid(ddiMgr);

  if (0) {
    Tr_Tr_t *newTr = Tr_TrPartition(bckTr, "CLK", 0);

    Ddi_BddSetClustered(Tr_TrBdd(bckTr), opt->trav.clustTh);
    Tr_TrFree(bckTr);
    bckTr = newTr;
  }



  /*Tr_TrReverseAcc(bckTr); done in approx preimg */
  verifOK = !travOverRings(from, NULL, init, bckTr, fwdReached,
    bckReached, NULL, Ddi_BddPartNum(fwdReached),
    "Approx backwd iteration", initialTime, 1 /*reverse */ ,
    doApproxBck || (opt->trav.storeCNF != NULL), opt);

  frontier = NULL;

#if 1
  if ((opt->trav.storeCNF != NULL) && (opt->trav.storeCNFphase == 1)) {

    storeCNFRings(fwdReached, bckReached, Tr_TrBdd(bckTr), badStates, opt);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      currTime = util_cpu_time();
      fprintf(stdout, "\nLive nodes (peak): %u(%u)",
        Ddi_MgrReadLiveNodes(ddiMgr), Ddi_MgrReadPeakLiveNodeCount(ddiMgr));
      //fprintf(stdout, "\nTotalTime: %s ", util_print_time(currTime-opt->stats.startTime));
      fprintf(stdout, "TotalTime: %s ",
        util_print_time((opt->stats.setupTime - opt->stats.startTime) +
          (currTime - initialTime)));
      fprintf(stdout, "(setup: %s - ",
        util_print_time(opt->stats.setupTime - opt->stats.startTime));
//fprintf(stdout, "(setup: %s -", util_print_time (initialTime-opt->stats.startTime));
      fprintf(stdout, " trav: %s)\n", util_print_time(currTime - initialTime))
      );
    Tr_TrFree(fwdTr);
    Tr_TrFree(bckTr);
    Ddi_Free(bckReached);
    Ddi_Free(from);
    Ddi_Free(invar);
    Ddi_Free(badStates);
    Ddi_Free(notInit);
    Ddi_Free(fwdReached);
    return (1);
  }
#endif

  if (verifOK) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: PASS.\n"));
  } else {

    if ((opt->trav.storeCNF != NULL) && (opt->trav.storeCNFphase == 1)) {
#if 0
      storeCNFRings(fwdReached, bckReached, Tr_TrBdd(bckTr), badStates, opt);

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelNone_c,
        currTime = util_cpu_time();
        fprintf(stdout, "Live nodes (peak): %u(%u)\n",
          Ddi_MgrReadLiveNodes(ddiMgr), Ddi_MgrReadPeakLiveNodeCount(ddiMgr));
        //fprintf(stdout, "TotalTime: %s ", util_print_time(currTime-opt->stats.startTime));
        fprintf(stdout, "TotalTime: %s ",
          util_print_time((opt->stats.setupTime - opt->stats.startTime) +
            (currTime - initialTime)));
        fprintf(stdout, "(setup: %s - ",
          util_print_time(opt->stats.setupTime - opt->stats.startTime));
//fprintf(stdout, "(setup: %s -", util_print_time (initialTime-opt->stats.startTime));
        fprintf(stdout, " trav: %s)\n",
          util_print_time(currTime - initialTime)););

      Tr_TrFree(fwdTr);
      Tr_TrFree(bckTr);
      Ddi_Free(bckReached);
      Ddi_Free(from);
      Ddi_Free(invar);
      Ddi_Free(badStates);
      Ddi_Free(notInit);
      Ddi_Free(fwdReached);
      return (1);
#endif
    } else if (!doApproxBck) {
      frontier = Ddi_BddarrayMakeFromBddPart(bckReached);
      fwdRings = 0;
    } else {
      Ddi_Bdd_t *overApprox = Ddi_BddDup(fwdReached);

      Ddi_BddPartWrite(overApprox, 0, init);
      /* opt->trav.constrain_level = 0; */
      verifOK = !travOverRings(init, NULL, badStates, fwdTr, bckReached,
        fwdReached, overApprox, Ddi_BddPartNum(bckReached),
        "Exact fwd iteration", initialTime, 0 /*reverse */ , 0 /*approx */ ,
        opt);

      Ddi_Free(overApprox);
      frontier = Ddi_BddarrayMakeFromBddPart(fwdReached);
      fwdRings = 1;
    }
  }

  Tr_TrFree(fwdTr);
  Tr_TrFree(bckTr);
  Ddi_Free(bckReached);
  Ddi_Free(from);
  Ddi_Free(invar);
  Ddi_Free(notInit);
  Ddi_Free(fwdReached);

  if (verifOK) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: PASS.\n"));
  } else {
    FILE *fp;
    Trav_Mgr_t *travMgr;
    Ddi_Bddarray_t *patternMisMat;
    Ddi_Varset_t *piv;
    Ddi_Bdd_t *invspec_bar, *end;

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Verification Result: FAIL.\n"));

    Tr_MgrSetClustThreshold(trMgr, opt->trav.clustTh);
    Tr_TrSetClustered(tr);

    Tr_MgrSetClustSmoothPi(trMgr, 0);
    travMgr = Trav_MgrInit(NULL, ddiMgr);
    Trav_MgrSetMismatchOptLevel(travMgr, opt->trav.mism_opt_level);

#if 1
    piv = Ddi_VarsetMakeFromArray(Tr_MgrReadI(Tr_TrMgr(tr)));
#if 0
    /* keep PIs for counterexample */
    invspec_bar = Ddi_BddExist(invarspec, piv);
#else
    invspec_bar = Ddi_BddNot(invarspec);
#endif

#if 0
    Tr_MgrSetImgMethod(Tr_TrMgr(tr), Tr_ImgMethodCofactor_c);
#endif


    if (fwdRings) {
      Tr_TrReverseAcc(tr);
      patternMisMat = Trav_MismatchPat(travMgr, tr, invspec_bar,
        NULL, NULL, &end, frontier,
        Tr_MgrReadPS(Tr_TrMgr(tr)), Tr_MgrReadNS(Tr_TrMgr(tr)), piv);
      Tr_TrReverseAcc(tr);
    } else {
      patternMisMat = Trav_MismatchPat(travMgr, tr, NULL,
        invspec_bar, &end, NULL, frontier,
        Tr_MgrReadPS(Tr_TrMgr(tr)), Tr_MgrReadNS(Tr_TrMgr(tr)), piv);
    }

    /*---------------------------- Write Patterns --------------------------*/

    if (patternMisMat != NULL) {
      char filename[100];

      sprintf(filename, "%s%d.txt", FILEPAT, getpid());
      fp = fopen(filename, "w");
      //fp = fopen ("pdtpat.txt", "w");
      if (Ddi_BddarrayPrintSupportAndCubes(patternMisMat, 1, -1,
          0, 0, NULL, fp) == 0) {
        Pdtutil_Warning(1, "Store cube Failed.");
      }
      fclose(fp);
    }

    /*----------------------- Write Initial State Set ----------------------*/

    if (end != NULL && !Ddi_BddIsZero(end)) {
      char filename[100];

      sprintf(filename, "%s%d.txt", FILEINIT, getpid());
      fp = fopen(filename, "w");
      //fp = fopen ("pdtinit.txt", "w");
      if (Ddi_BddPrintSupportAndCubes(end, 1, -1, 0, NULL, fp) == 0) {
        Pdtutil_Warning(1, "Store cube Failed.");
      }
      fclose(fp);
      Ddi_Free(end);
    }

    Ddi_Free(piv);
    Ddi_Free(invspec_bar);
    Ddi_Free(patternMisMat);

#endif

  }

  Ddi_Free(frontier);
  Ddi_Free(badStates);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c,
    currTime = util_cpu_time();
    fprintf(stdout, "Live nodes (peak): %u(%u)\n",
      Ddi_MgrReadLiveNodes(ddiMgr), Ddi_MgrReadPeakLiveNodeCount(ddiMgr));
    // fprintf(stdout, "TotalTime: %s ", util_print_time (currTime-opt->stats.startTime));
    fprintf(stdout, "TotalTime: %s ",
      util_print_time((opt->stats.setupTime - opt->stats.startTime) +
        (currTime - initialTime)));
    fprintf(stdout, "(setup: %s - ",
      util_print_time(opt->stats.setupTime - opt->stats.startTime));
    //fprintf(stdout, "(setup: %s -", util_print_time (initialTime-opt->stats.startTime));
    fprintf(stdout, " trav: %s)\n", util_print_time(currTime - initialTime))
    );

  return (verifOK);
}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************
  Synopsis    [find dynamic non vacuum hint]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
findNonVacuumHint(
  Ddi_Bdd_t * from,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * pi,
  Ddi_Vararray_t * aux,
  Ddi_Bdd_t * reached,
  Ddi_Bddarray_t * delta,
  Ddi_Bdd_t * trBdd,
  Ddi_Bdd_t * hConstr,
  Ddi_Bdd_t * prevHint,
  int nVars,
  int hintsStrategy
)
{
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(from);
  Ddi_Bdd_t *trAndNotReached = Ddi_BddNot(reached);
  Ddi_Bdd_t *myFromAig, *myFrom = Ddi_BddDup(from);
  Ddi_Var_t *iv = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
  Ddi_Bdd_t *h, *hFull, *cex0 = NULL, *cex = NULL;
  Ddi_Bdd_t *partBdd = trBdd != NULL ? trBdd : from;
  int abo, maxIter = nVars;
  Ddi_Varset_t *vars = Ddi_VarsetMakeFromArray(ps);
  Ddi_Varset_t *vars1 = Ddi_VarsetMakeFromArray(pi);
  int timeLimit = -1;

  if (nVars == 0) {
    Ddi_Free(vars);
    Ddi_Free(vars1);
    Ddi_Free(myFrom);
    Ddi_Free(trAndNotReached);
    return Ddi_BddMakeConst(ddiMgr, 1);
  }

  Ddi_VarsetUnionAcc(vars, vars1);
  Ddi_Free(vars1);
  if (aux != NULL) {
    Ddi_Varset_t *vars1 = Ddi_VarsetMakeFromArray(aux);

    Ddi_VarsetDiffAcc(vars, vars1);
    Ddi_Free(vars1);
  }

  Ddi_BddSetAig(trAndNotReached);
  if (iv != NULL) {
    Ddi_Bdd_t *invar = Ddi_BddMakeLiteralAig(iv, 1);

    Ddi_AigAndCubeAcc(trAndNotReached, invar);
    Ddi_BddSetMono(invar);
    Ddi_BddConstrainAcc(myFrom, invar);
    Ddi_BddAndAcc(myFrom, invar);
    Ddi_Free(invar);
  }

  Ddi_BddComposeAcc(trAndNotReached, ps, delta);
  if (iv != NULL) {
    Ddi_Bdd_t *invar = Ddi_BddMakeLiteralAig(iv, 1);

    Ddi_BddAndAcc(trAndNotReached, invar);
    Ddi_Free(invar);
  }

  myFromAig = Ddi_BddMakeAig(myFrom);
  if (hConstr != NULL && !Ddi_BddIsOne(hConstr)) {
    Ddi_BddAndAcc(myFromAig, hConstr);
  }

  h = NULL;

  if (hintsStrategy == 1 && prevHint != NULL) {
    int sat;
    Ddi_Bdd_t *myBlock = Ddi_BddMakeAig(prevHint);
    Ddi_Bdd_t *fAig = Ddi_BddAnd(myFromAig, trAndNotReached);
    Ddi_Varset_t *sm;

    sat = Ddi_AigSatMinisatWithAbortAndFinal(fAig, myBlock, 10, 0);
    Pdtutil_Assert(sat <= 0, "unsat or undef required");
    Ddi_Free(fAig);
    sm = Ddi_VarsetMakeFromVar(Ddi_BddTopVar(myBlock));
    Ddi_Free(myBlock);
    h = Ddi_BddExist(prevHint, sm);
    Ddi_Free(sm);
  }

  if (h == NULL) {
    cex0 = Ddi_AigSatWithCex(myFromAig);
    cex =
      Ddi_AigSatAndWithCexAndAbort(cex0, trAndNotReached, NULL, NULL, 5, &abo);
    if (cex == NULL || abo) {
      cex = Ddi_AigSatAndWithCexAndAbort(myFromAig, trAndNotReached,
        NULL, NULL, 10, &abo);
      if (cex == NULL || abo) {
        int k, sat;

        h = Ddi_BddMakeConst(ddiMgr, 1);
        for (k = sat = 0; k < maxIter && !sat; k++) {
          Ddi_Bdd_t *myCex = Ddi_BddDup(cex0);
          Ddi_Bdd_t *fAig = Ddi_BddDup(trAndNotReached);

          sat = Ddi_AigSatMinisatWithAbortAndFinal(fAig, myCex, timeLimit, 4);
          if (!sat) {
            Ddi_Free(cex0);
            Ddi_BddDiffAcc(myFromAig, myCex);
            if (hConstr != NULL)
              Ddi_BddDiffAcc(hConstr, myCex);
            cex0 = Ddi_AigSatWithCex(myFromAig);
            Ddi_BddSetMono(myCex);
            Ddi_BddDiffAcc(h, myCex);
          } else {
            Ddi_Free(h);
            cex = Ddi_BddDup(myCex);
          }
          Ddi_Free(myCex);
          Ddi_Free(fAig);
        }
      }
    }
  }

  if (h == NULL) {
    hFull = Part_PartitionDisjSet(partBdd, cex, vars,
      Part_MethodCofactorKernel_c,
      Ddi_BddSize(partBdd) / 100, nVars, Pdtutil_VerbLevelDevMax_c);
    h = Ddi_BddPartExtract(hFull, 0);
  }

  Ddi_Free(cex0);
  Ddi_Free(cex);

  Ddi_Free(trAndNotReached);
  Ddi_Free(myFromAig);
  Ddi_Free(myFrom);
  Ddi_Free(trAndNotReached);

  Ddi_Free(hFull);
  Ddi_Free(vars);

  return h;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static int
FbvBddCexToFsm(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * cexArray,
  Ddi_Bdd_t * initState,
  int reverse
)
{
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(cexArray);

  // write cex as aig into fsm
  Ddi_Bdd_t *cexAig = Ddi_BddMakePartConjVoid(ddiMgr);
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  int i;

  if (initStub != NULL) {
    Pdtutil_Assert(initState != NULL && !Ddi_BddIsZero(initState),
      "missing init state in cex");
    Ddi_Bdd_t *chk = Ddi_BddRelMakeFromArray(initStub, ps);
    Ddi_Bdd_t *initAig = Ddi_BddMakeAig(initState);

    Ddi_BddSetAig(chk);
    Ddi_AigConstrainCubeAcc(chk, initAig);
    Ddi_Free(initAig);
    Ddi_Bdd_t *cex0 = Ddi_AigSatWithCex(chk);

    Ddi_BddPartInsertLast(cexAig, cex0);
    Ddi_Free(chk);
    Ddi_Free(cex0);
  }

  if (reverse) {
    for (i = Ddi_BddarrayNum(cexArray) - 1; i >= 0; i--) {
      Ddi_Bdd_t *c_i = Ddi_BddDup(Ddi_BddarrayRead(cexArray, i));

      Ddi_BddSetAig(c_i);
      Ddi_BddPartInsertLast(cexAig, c_i);
      Ddi_Free(c_i);
    }
  } else {
    for (i = 0; i < Ddi_BddarrayNum(cexArray); i++) {
      Ddi_Bdd_t *c_i = Ddi_BddDup(Ddi_BddarrayRead(cexArray, i));

      Ddi_BddSetAig(c_i);
      Ddi_BddPartInsertLast(cexAig, c_i);
      Ddi_Free(c_i);
    }
  }
  Fsm_MgrSetCexBDD(fsmMgr, cexAig);

  return 1;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
orProjectedSetAcc(
  Ddi_Bdd_t * f,
  Ddi_Bdd_t * g,
  Ddi_Var_t * partVar
)
{
  Ddi_Varset_t *smooth, *smoothi, *supp;
  Ddi_Bdd_t *p, *gProj;
  int i, j;

  if (Ddi_BddIsMono(f)) {
    return (Ddi_BddOrAcc(f, g));
  }

  if (Ddi_BddIsMeta(f)) {
    return (Ddi_BddOrAcc(f, g));
  }

  if (Ddi_BddIsPartDisj(f)) {
    Ddi_Bdd_t *p = NULL;

    j = -1;
    if (partVar != NULL) {
      Ddi_Bdd_t *lit = Ddi_BddMakeLiteral(partVar, 0);

      if (!Ddi_BddIncluded(g, lit)) {
        Ddi_BddNotAcc(lit);
        if (!Ddi_BddIncluded(g, lit)) {
          Ddi_Free(lit);
        }
      }
      if (lit != NULL) {
        for (i = 0; i < Ddi_BddPartNum(f); i++) {
          if (!Ddi_BddIsZero(Ddi_BddPartRead(f, i)) &&
            Ddi_BddIncluded(Ddi_BddPartRead(f, i), lit)) {
            j = i;
            break;
          }
        }
        if (j < 0) {
          j = 0;
          for (i = 0; i < Ddi_BddPartNum(f); i++) {
            if (Ddi_BddIsZero(Ddi_BddPartRead(f, i))) {
              j = i;
              break;
            }
          }
        }
        Ddi_Free(lit);
      } else {
        j = 0;
      }
    } else {
      j = 0;
    }
    p = Ddi_BddPartRead(f, j);
    if (Ddi_BddIsZero(p)) {
      Ddi_BddPartWrite(f, j, g);
    } else {
      orProjectedSetAcc(p, g, NULL);
    }
    return (f);
  }

  smooth = Ddi_BddSupp(g);

  for (i = 0; i < Ddi_BddPartNum(f); i++) {

    p = Ddi_BddPartRead(f, i);
#if 1
    supp = Ddi_BddSupp(p);
    smoothi = Ddi_VarsetDiff(smooth, supp);
    gProj = Ddi_BddExist(g, smoothi);
    Ddi_BddOrAcc(p, gProj);
    Ddi_Free(gProj);
    Ddi_Free(supp);
    Ddi_Free(smoothi);
#else
    Ddi_BddOrAcc(p, g);
#endif

  }
  Ddi_Free(smooth);

  return (f);

}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
diffProjectedSetAcc(
  Ddi_Bdd_t * f,
  Ddi_Bdd_t * g,
  Ddi_Var_t * partVar
)
{
  Ddi_Bdd_t *p;
  int i, j;

  if (Ddi_BddIsMono(f)) {
    return (Ddi_BddDiffAcc(f, g));
  }

  if (Ddi_BddIsMeta(f)) {
    return (Ddi_BddDiffAcc(f, g));
  }

  if (Ddi_BddIsPartDisj(f)) {
    Ddi_Bdd_t *p = NULL;

    Pdtutil_Assert(0, "Not yet supported diffProjectedSet with dpart");
    j = -1;
    if (partVar != NULL) {
      Ddi_Bdd_t *lit = Ddi_BddMakeLiteral(partVar, 0);

      if (!Ddi_BddIncluded(g, lit)) {
        Ddi_BddNotAcc(lit);
        if (!Ddi_BddIncluded(g, lit)) {
          Ddi_Free(lit);
        }
      }
      if (lit != NULL) {
        for (i = 0; i < Ddi_BddPartNum(f); i++) {
          if (!Ddi_BddIsZero(Ddi_BddPartRead(f, i)) &&
            Ddi_BddIncluded(Ddi_BddPartRead(f, i), lit)) {
            j = i;
            break;
          }
        }
        if (j < 0) {
          j = 0;
          for (i = 0; i < Ddi_BddPartNum(f); i++) {
            if (Ddi_BddIsZero(Ddi_BddPartRead(f, i))) {
              j = i;
              break;
            }
          }
        }
        Ddi_Free(lit);
      } else {
        j = 0;
      }
    } else {
      j = 0;
    }
    p = Ddi_BddPartRead(f, j);
    if (Ddi_BddIsZero(p)) {
      Ddi_BddPartWrite(f, j, g);
    } else {
      orProjectedSetAcc(p, g, NULL);
    }
    return (f);
  }

  for (i = 0; i < Ddi_BddPartNum(f); i++) {

    p = Ddi_BddPartRead(f, i);
    Ddi_BddDiffAcc(p, g);

  }

  return (f);
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static int
travOverRingsWithFixPoint(
  Ddi_Bdd_t * from0,
  Ddi_Bdd_t * reached0,
  Ddi_Bdd_t * target,
  Tr_Tr_t * TR,
  Ddi_Bdd_t * inRings,
  Ddi_Bdd_t * outRings,
  Ddi_Bdd_t * overApprox,
  int maxIter,
  char *iterMsg,
  long initialTime,
  int reverse,
  int approx,
  Fbv_Globals_t * opt
)
{
  int targetReached, i, fixPoint;
  Ddi_Bdd_t *care, *careAig = NULL,
    *careNS, *ring, *tmp, *from, *reached, *to, *oldReached;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(from0);
  Tr_Mgr_t *trMgr = Tr_TrMgr(TR);
  Tr_Tr_t *myTr = Tr_TrDup(TR);
  long currTime;
  Ddi_Bdd_t *toPlus;
  int useAigReached = opt->trav.fbvMeta != 0;

  Pdtutil_Assert(Ddi_BddIsPartDisj(outRings), "Disj PART out rings required");

  /* exact backward */

  targetReached = 0;
  from = Ddi_BddDup(from0);

  if (opt->trav.cntReachedOK) {
    reached = Ddi_BddMakeConst(ddiMgr, 0);
  } else {
    reached = Ddi_BddDup(from);
  }
  /* Ddi_BddSetMono(reached); */

  Ddi_BddPartWrite(outRings, 0, from);
  /*@@@@@ */
#if 0
  tmp = Ddi_BddMakeConst(ddiMgr, 0);
  for (i = Ddi_BddPartNum(inRings) - 1; i >= 0; i--) {
    Ddi_Free(tmp);
    tmp = Ddi_BddAnd(Ddi_BddPartRead(inRings, i), target);
    if (!Ddi_BddIsZero(tmp))
      break;
  }
  Ddi_Free(tmp);
#endif

  care = Ddi_BddPartRead(inRings, Ddi_BddPartNum(inRings) - 1);
  oldReached = NULL;

  for (i = 0, fixPoint = 0; (!fixPoint); i++) {

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "-- %s; Iteration=%d.\n", iterMsg, i + 1));

    //    Ddi_BddRestrictAcc(from,care);

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|From|=%d; ", Ddi_BddSize(from)));

    Ddi_BddPartWrite(outRings, i, care);
    ring = Ddi_BddPartRead(outRings, i);

    Ddi_BddAndAcc(ring, from);

    if (Ddi_BddIsZero(from)) {
      fixPoint = 1;
      break;
    }

    if (Ddi_BddIsAig(target)) {
      tmp = Ddi_BddMakeAig(care);
      Ddi_BddAndAcc(tmp, target);
    } else {
      tmp = Ddi_BddAnd(care, target);
      Ddi_BddSetMono(tmp);
    }
    if (!Ddi_BddIsZero(tmp)) {
      if (Ddi_BddIsAig(tmp)) {
        Ddi_Bdd_t *f = Ddi_BddMakeAig(from);

        Ddi_BddAndAcc(tmp, f);
        Ddi_Free(f);
        targetReached = Ddi_AigSat(tmp);
      } else {
        Ddi_BddAndAcc(tmp, from);
        Ddi_BddSetMono(tmp);
        targetReached = !Ddi_BddIsZero(tmp);
      }
      if (targetReached) {
        Ddi_Free(tmp);
        if (!Ddi_BddIsAig(target)) {
          Ddi_BddAndAcc(ring, target);
          while (Ddi_BddPartNum(outRings) > (i + 1)) {
            Ddi_Bdd_t *p = Ddi_BddPartExtract(outRings,
              Ddi_BddPartNum(outRings) - 1);

            Ddi_Free(p);
          }
        }
        break;
      }
    }

    Ddi_Free(tmp);

    if (overApprox != NULL) {
      Ddi_BddAndAcc(from, Ddi_BddPartRead(overApprox, i));

      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "|from*overAppr|=%d\n", Ddi_BddSize(from)));
    }

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
      fprintf(stdout, "|TR|=%d.\n", Ddi_BddSize(Tr_TrBdd(myTr))));

    if (!Ddi_BddIsAig(reached) &&
      ((opt->ddi.exist_opt_level > 0) || Ddi_BddSize(reached) < 10000)) {
#if 0
      careNS = Ddi_BddNot(reached);
      Ddi_BddSetPartConj(careNS);
#if 0
      Ddi_BddSwapVarsAcc(careNS, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
#endif
      Ddi_BddPartInsertLast(careNS, care);
#else
      careNS = Ddi_BddDup(care);
      if (oldReached != NULL) {
        Ddi_BddNotAcc(oldReached);
        Ddi_BddSwapVarsAcc(oldReached, Tr_MgrReadPS(trMgr),
          Tr_MgrReadNS(trMgr));
        Ddi_BddSetPartConj(careNS);
        Ddi_BddPartInsertLast(careNS, oldReached);
        Ddi_Free(oldReached);
      }
#endif
    } else {
      careNS = Ddi_BddMakeConst(ddiMgr, 1);
    }

    if (opt->trav.fbvMeta
      && /* Ddi_BddSize(from) > 1000 && */ !Ddi_BddIsMeta(from)) {
      Ddi_BddSetMeta(from);
    }

    if (reverse) {
      to = FbvApproxPreimg(myTr, from, careNS, NULL, approx, opt);
    } else {
      Ddi_BddSwapVarsAcc(careNS, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      to = Tr_ImgWithConstrain(myTr, from, careNS);
    }
    Ddi_BddSetMono(to);

    Ddi_Free(careNS);

    toPlus = Ddi_BddDup(to);
    if (0 && !approx && i > 1 && Ddi_BddSize(to) > opt->trav.approxPreimgTh) {
      Ddi_Varset_t *suppF = Ddi_BddSupp(reached);
      Ddi_Varset_t *suppT = Ddi_BddSupp(to);
      Ddi_Varset_t *suppFT = Ddi_VarsetUnion(suppT, suppF);
      Ddi_Varset_t *keep = NULL;
      Ddi_Bdd_t *out;
      int nv = Ddi_VarsetNum(suppT);

      if (Tr_TrClk(TR) != NULL) {
        keep = Ddi_VarsetMakeFromVar(Tr_TrClk(TR));
      }

      approx = 1;               /* enable approx preimg on next iterations */
      Ddi_VarsetDiffAcc(suppT, suppF);
      if (!Ddi_VarsetIsVoid(suppT)) {
        Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
          fprintf(stdout, "Dynamic Abstraction: in=%d -> out=%d vars.\n",
            Ddi_VarsetNum(suppF),
            Ddi_VarsetNum(suppFT) - Ddi_VarsetNum(suppF)));
        if (keep != NULL)
          Ddi_VarsetDiffAcc(suppF, keep);
#if 0
        Ddi_Free(suppT);
        suppT = Ddi_BddSupp(to);
        Ddi_VarsetIntersectAcc(suppF, suppT);
        {
          int k;
          Ddi_Vararray_t *v = Ddi_VararrayMakeFromVarset(suppF, 1);

          for (k = Ddi_VararrayNum(v) / 2; k < Ddi_VararrayNum(v); k++) {
            Ddi_VarsetRemoveAcc(suppF, Ddi_VararrayRead(v, k));
          }
          Ddi_Free(v);
        }
#endif
#if 0
        out = Ddi_BddNot(reached);
        Ddi_BddAndExistAcc(out, to, suppF);
        if (!Ddi_BddIsZero(out)) {
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "Reducing state set (%d-%d)\n",
              Ddi_BddSize(to), Ddi_BddSize(out)));
          Ddi_Free(toPlus);
          toPlus = Ddi_BddDup(out);
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "After smoothing -> %d\n", Ddi_BddSize(toPlus)));
          Ddi_Free(out);
          out = Ddi_BddNot(reached);
          Ddi_BddAndExistAcc(out, to, suppT);
          Ddi_BddAndAcc(toPlus, out);
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
            fprintf(stdout, "After smoothing 2 -> *%d=%d\n",
              Ddi_BddSize(out), Ddi_BddSize(toPlus)));
        }
#else
        out = dynAbstract(to, (Ddi_BddSize(to) * 3) / 4, 3 * nv / 4);
        Ddi_Free(toPlus);
        toPlus = Ddi_BddDup(out);
#endif
        Ddi_Free(out);
      }
      Ddi_Free(suppF);
      Ddi_Free(suppFT);
      Ddi_Free(suppT);
      Ddi_Free(keep);
    }
#if 1
    if (!Ddi_BddIsAig(reached) && opt->ddi.exist_opt_level > 0) {
      Ddi_BddNotAcc(reached);
      Ddi_BddRestrictAcc(to, reached);
      Ddi_BddRestrictAcc(toPlus, reached);
      Ddi_BddNotAcc(reached);
    }

    Ddi_BddRestrictAcc(toPlus, care);
    Ddi_BddRestrictAcc(to, care);
#endif

    Ddi_BddSetMono(to);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelDevMin_c,
      LOG_CONST(to, "to")
      );

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "|To|=%d; ", Ddi_BddSize(to)));

    Ddi_Free(from);

    fixPoint = Ddi_BddIsZero(to);
    if (fixPoint) {
      Ddi_Free(to);
      Ddi_Free(toPlus);
      break;
    }
    from = Ddi_BddDup(toPlus);
    Ddi_Free(toPlus);

    if (reached != NULL && useAigReached && Ddi_BddSize(reached) > 30000) {
      if (careAig == NULL && care != NULL) {
        careAig = Ddi_BddMakeAig(care);
      }
      Ddi_BddSetAig(reached);
      Ddi_BddConstrainAcc(to, careAig);
      Ddi_BddSetAig(to);
      Ddi_BddNotAcc(reached);
      fixPoint = !Ddi_AigSatAnd(to, reached, careAig);
      Ddi_BddNotAcc(reached);
      Ddi_BddOrAcc(reached, to);
      Ddi_MgrSetAigAbcOptLevel(ddiMgr, opt->common.aigAbcOpt);
      ddiAbcOptAcc(reached, -1.0);
    } else if (reached != NULL) {
      Ddi_Bdd_t *newReached = Ddi_BddOr(reached, to);

      if (!fixPoint)
        fixPoint = Ddi_BddEqual(newReached, reached);
      Ddi_BddConstrainAcc(newReached, care);
      //      Ddi_BddRestrictAcc(newReached,care);
      if (!fixPoint)
        fixPoint = Ddi_BddEqual(newReached, reached);
#if 0
      oldReached = Ddi_BddDup(reached);
#endif
      if (opt->trav.fromSel == 'c') {
        Ddi_BddNotAcc(reached);
        Ddi_BddConstrainAcc(from, reached);
        Ddi_BddConstrainAcc(from, care);
      }

      Ddi_Free(reached);
      reached = newReached;

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMax_c,
        fprintf(stdout, "|Reached|=%d; ", Ddi_BddSize(reached));
        fprintf(stdout, "#ReachedStates=%g.\n",
          Ddi_BddCountMinterm(reached, Ddi_VararrayNum(Tr_MgrReadPS(trMgr))))
        );
    }

    if (!useAigReached) {
      if (opt->trav.fromSel == 'r') {
        Ddi_Free(from);
        from = Ddi_BddDup(reached);
      } else if (opt->trav.fromSel == 'b') {
        if (Ddi_BddSize(from) > Ddi_BddSize(reached)) {
          Ddi_Free(from);
          from = Ddi_BddDup(reached);
        }
      }
    }

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMed_c,
      currTime = util_cpu_time();
      fprintf(stdout, "Live Nodes (Peak) = %u (%u); CPU Time=%s\n",
        Ddi_MgrReadLiveNodes(ddiMgr),
        Ddi_MgrReadPeakLiveNodeCount(ddiMgr),
        util_print_time(currTime - initialTime))
      );

    if (0 && opt->tr.coiLimit > 0) {
      Ddi_Varset_t *ps = Ddi_VarsetMakeFromArray(Tr_MgrReadPS(trMgr));
      Ddi_Varset_t *ns = Ddi_VarsetMakeFromArray(Tr_MgrReadNS(trMgr));
      Ddi_Varset_t *supp = Ddi_BddSupp(Tr_TrBdd(TR));
      Ddi_Varset_t *suppPs = Ddi_VarsetIntersect(supp, ps);
      Ddi_Varset_t *suppNs = Ddi_VarsetIntersect(supp, ns);

      Ddi_VarsetSwapVarsAcc(suppNs, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      Ddi_VarsetDiffAcc(suppPs, suppNs);
      Ddi_Free(ps);
      Ddi_Free(ns);
      Ddi_Free(supp);
      Ddi_Free(suppNs);
      Ddi_BddNotAcc(from);
      Ddi_BddExistAcc(from, suppPs);
      Ddi_BddNotAcc(from);
      Ddi_Free(suppPs);
    }

    Ddi_Free(to);
  }

  Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
    fprintf(stdout, "|Reached|=%d; ", Ddi_BddSize(reached));
    fprintf(stdout, "#ReachedStates=%.g.\n",
      Ddi_BddCountMinterm(reached, Ddi_VararrayNum(Tr_MgrReadPS(trMgr))))
    );

  if (0) {
    Ddi_Var_t *iv = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVAR_VAR$PS");
    Ddi_Var_t *ivp = Ddi_VarFromName(ddiMgr, "PDT_BDD_INVARSPEC_VAR$PS");

    if (iv != NULL) {
      Ddi_Bdd_t *invarIn = Ddi_BddMakeLiteral(iv, 1);

      if (useAigReached) {
        Ddi_BddSetAig(invarIn);
      }
      Ddi_BddAndAcc(reached, invarIn);
      Ddi_Free(invarIn);
      if (0 && (ivp != NULL)) {
        Ddi_Bdd_t *invarInP = Ddi_BddMakeLiteral(ivp, 1);

        Ddi_BddAndAcc(reached, invarInP);
        Ddi_Free(invarInP);
      }
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelUsrMed_c,
        fprintf(stdout, "#Reached States in Constraints = %g.\n",
          Ddi_BddCountMinterm(reached, Ddi_VararrayNum(Tr_MgrReadPS(trMgr)))));
    }
  }

  Tr_TrFree(myTr);
  Ddi_Free(oldReached);
  Ddi_Free(careAig);
  Ddi_Free(reached);
  Ddi_Free(from);

  return (targetReached);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static void
storeCNFRings(
  Ddi_Bdd_t * fwdRings,
  Ddi_Bdd_t * bckRings,
  Ddi_Bdd_t * TrBdd,
  Ddi_Bdd_t * prop,
  Fbv_Globals_t * opt
)
{
  int i, j, k;
  Ddi_Bdd_t *fp, *bp, *localTrBdd, *p;
  int numpart;

  if (prop != NULL) {
    Ddi_BddSetMono(prop);
  } else {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "***Prop = NULL!!\n"));
  }

  if (opt->trav.storeCNFphase == 0) {
    switch (opt->trav.storeCNFTR) {
      case -1:
        break;
      case 0:
        numpart = Ddi_BddPartNum(fwdRings);
        for (i = numpart - 1; i >= 0; i--) {
          fp = Ddi_BddPartRead(fwdRings, i);
          Ddi_BddSetPartConj(fp);

          if (i > 0) {
            while (Ddi_BddPartNum(fp) > 0) {
              p = Ddi_BddPartExtract(fp, 0);
              Ddi_Free(p);
            }
          }
          if (i == numpart - 1) {
            Ddi_BddPartInsertLast(fp, prop);
          }

          if (i < numpart - 1) {
            for (k = 0; k < Ddi_BddPartNum(TrBdd); k++) {
              Ddi_BddPartInsertLast(fp, Ddi_BddPartRead(TrBdd, k));
            }
          }

        }
        break;
      case 1:
        numpart = Ddi_BddPartNum(fwdRings);
        for (i = numpart - 1; i >= 0; i--) {
          fp = Ddi_BddPartRead(fwdRings, i);
          Ddi_BddSetPartConj(fp);
          if (i == numpart - 1) {
            Ddi_BddPartInsertLast(fp, prop);
          }
          if (i < numpart - 1) {

            localTrBdd = Ddi_BddRestrict(TrBdd, fp);
            for (k = 0; k < Ddi_BddPartNum(localTrBdd); k++) {
              Ddi_BddPartInsertLast(fp, Ddi_BddPartRead(localTrBdd, k));
            }
            Ddi_Free(localTrBdd);
          }
        }
        break;
    }
    storeRingsCNF(fwdRings, opt);
    return;
  }

  /* NOTICE: the following code assumes that the   */
  /*         property is stored in bckRings[0].    */
  /*         this is NOT the case when a property  */
  /*         passes just with a BDD-analisys       */
  if (opt->trav.storeCNFphase == 1) {
    switch (opt->trav.storeCNFTR) {
      case -1:
        numpart = Ddi_BddPartNum(fwdRings);
        for (i = numpart - 1, j = 0; i >= 0; j++, i--) {
          fp = Ddi_BddPartRead(fwdRings, i);
          bp = Ddi_BddPartRead(bckRings, j);
          Ddi_BddSetPartConj(fp);
          Ddi_BddSetMono(bp);
          Ddi_BddPartInsertLast(fp, bp);
        }
        break;
      case 0:
        numpart = Ddi_BddPartNum(fwdRings);
        for (i = numpart - 1, j = 0; i >= 0; j++, i--) {
          fp = Ddi_BddPartRead(fwdRings, i);
          Ddi_BddSetPartConj(fp);
          if (i > 0) {
            while (Ddi_BddPartNum(fp) > 0) {
              p = Ddi_BddPartExtract(fp, 0);
              Ddi_Free(p);
            }
          }

          if (i == numpart - 1) {
            Ddi_BddPartInsertLast(fp, prop);
          }

          if (i < numpart - 1) {
            localTrBdd = TrBdd;
            for (k = 0; k < Ddi_BddPartNum(localTrBdd); k++) {
              Ddi_BddPartInsertLast(fp, Ddi_BddPartRead(localTrBdd, k));
            }
          }

        }
        break;
      case 1:
        for (i = Ddi_BddPartNum(fwdRings) - 1, j = 0; i >= 0; j++, i--) {
          fp = Ddi_BddPartRead(fwdRings, i);
          bp = Ddi_BddPartRead(bckRings, j);
          Ddi_BddSetPartConj(fp);
          Ddi_BddSetMono(bp);
          Ddi_BddPartInsertLast(fp, bp);
          if (i < Ddi_BddPartNum(fwdRings) - 1) {

            localTrBdd = Ddi_BddRestrict(TrBdd, fp);

            for (k = 0; k < Ddi_BddPartNum(localTrBdd); k++) {
              Ddi_BddPartInsertLast(fp, Ddi_BddPartRead(localTrBdd, k));
            }
            Ddi_Free(localTrBdd);
          }
        }
        break;
    }
    storeRingsCNF(fwdRings, opt);
    return;
  }

}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static int
travOverRings(
  Ddi_Bdd_t * from0,
  Ddi_Bdd_t * reached0,
  Ddi_Bdd_t * target,
  Tr_Tr_t * TR,
  Ddi_Bdd_t * inRings,
  Ddi_Bdd_t * outRings,
  Ddi_Bdd_t * overApprox,
  int maxIter,
  char *iterMsg,
  long initialTime,
  int reverse,
  int approx,
  Fbv_Globals_t * opt
)
{
  int targetReached, i, in_i, iter;
  Ddi_Bdd_t *care, *notReached, *ring, *tmp, *from, *reached, *to;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(from0);
  Tr_Mgr_t *trMgr = Tr_TrMgr(TR);
  Tr_Tr_t *myTr = Tr_TrDup(TR);
  long currTime;

  Pdtutil_Assert(Ddi_BddIsPartDisj(outRings), "Disj PART out rings required");

  /* exact backward */

  targetReached = 0;
  from = Ddi_BddDup(from0);
  if (reached0 != NULL)
    reached = Ddi_BddDup(reached0);
  else
    reached = NULL;

  i = (reverse ? 0 : maxIter - 1);
  Ddi_BddPartWrite(outRings, i, from);
  /*@@@@@ */
#if 0
  tmp = Ddi_BddMakeConst(ddiMgr, 0);
  for (i = Ddi_BddPartNum(inRings) - 1; i >= 0; i--) {
    Ddi_Free(tmp);
    tmp = Ddi_BddAnd(D < di_BddPartRead(inRings, i), target);
    if (!Ddi_BddIsZero(tmp))
      break;
  }
  Ddi_Free(tmp);
#endif

  if ((!approx) && (Ddi_BddIsMono(from) ||
      (Ddi_BddIsPartConj(from) && (Ddi_BddPartNum(from) == 1)))) {
    notReached = Ddi_BddNot(from);
    Ddi_BddSetMono(notReached);
    Ddi_BddSetPartConj(notReached);
  } else {
    notReached = Ddi_BddMakePartConjVoid(ddiMgr);
  }


  for (i = 0; (!Ddi_BddIsZero(from) && (i < maxIter)); i++) {

    iter = (reverse ? maxIter - i - 1 : i);
    in_i = maxIter - i - 1;

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "-- %s iteration: %d.\n", iterMsg, iter));

    care = Ddi_MgrReadOne(ddiMgr);

    if (opt->trav.constrain_level >= 1)
      if (inRings != NULL) {
        if (Ddi_BddIsMono(inRings)) {
          care = inRings;
        } else {
          care = Ddi_BddPartRead(inRings, in_i);
        }
        Ddi_BddRestrictAcc(from, care);
      }

    /*Ddi_BddSetMono(from); */

    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "|From|=%d\n", Ddi_BddSize(from)));

    if ((!approx) && (opt->trav.constrain_level == 1)) {
      Ddi_BddPartWrite(outRings, i, care);
    } else {
      Ddi_BddPartWrite(outRings, i, Ddi_MgrReadOne(ddiMgr));
    }
    ring = Ddi_BddPartRead(outRings, i);

    /*#if PARTITIONED_RINGS */
#if 1
    Ddi_BddSetPartConj(ring);
    Ddi_BddPartInsertLast(ring, from);
#else
    /* can be replaced by from|ring ??? */
    Ddi_BddAndAcc(ring, from);
#endif

    if (overApprox != NULL) {
      Ddi_BddPartInsertLast(ring, Ddi_BddPartRead(overApprox, i));
    }

    if (Ddi_BddIsZero(from))
      continue;

    /* check for intersection with target */
    if (Ddi_BddIsMono(inRings)) {
      tmp = Ddi_BddAnd(inRings, target);
    } else {
      tmp = Ddi_BddAnd(Ddi_BddPartRead(inRings, in_i), target);
    }
    Ddi_BddSetMono(tmp);

    if (!Ddi_BddIsZero(tmp)) {
      /*Ddi_BddSetMono(from); */
      Ddi_BddAndAcc(tmp, from);
      Ddi_BddSetMono(tmp);
      if (overApprox != NULL) {
        Ddi_BddAndAcc(tmp, Ddi_BddPartRead(overApprox, i));
        Ddi_BddSetMono(tmp);
      }
      if (!Ddi_BddIsZero(tmp)) {
        targetReached = 1;
        Ddi_Free(tmp);
        if (!approx) {
          /*#if PARTITIONED_RINGS */
#if 0
          Ddi_BddPartInsert(ring, 0, target);
          Ddi_BddSetMono(ring);
#else
          Ddi_BddAndAcc(ring, target);
#endif
          while (Ddi_BddPartNum(outRings) > (i + 1)) {
            Ddi_Bdd_t *p = Ddi_BddPartExtract(outRings,
              Ddi_BddPartNum(outRings) - 1);

            Ddi_Free(p);
          }
          if (0) {
            int j;

            for (j = 0; j < Ddi_BddPartNum(outRings); j++) {
              Ddi_Bdd_t *p = Ddi_BddPartRead(outRings, j);

              Ddi_BddSetMono(p);
            }
          }
          break;
        }
      }
    }
    Ddi_Free(tmp);

    if (i == maxIter - 1)
      continue;

    Pdtutil_Assert(i < maxIter - 1, "Max Iteration exceeded");

    if (opt->trav.constrain_level == 0) {
      tmp = Ddi_BddAnd(care, from);
      if (Ddi_BddSize(tmp) < Ddi_BddSize(from)) {
        Ddi_Free(from);
        from = tmp;
      } else
        Ddi_Free(tmp);
    }

    care = Ddi_BddDup(notReached);
    Ddi_BddPartInsertLast(care, Ddi_BddPartRead(inRings, in_i - 1));

    if (opt->trav.constrain_level > 1) {
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      Ddi_BddConstrainAcc(Tr_TrBdd(myTr), care);
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
    }
#if 0
    if (overApprox != NULL) {
#if 1
#if 0
      if (i < Ddi_BddPartNum(overApprox) - 1) {
        Ddi_BddPartInsertLast(care, Ddi_BddPartRead(overApprox, i + 1));
      }
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      toPlus = Tr_ImgWithConstrain(myTr, from, care);
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));

      Ddi_BddPartInsertLast(care, toPlus);
#endif
#if 0
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      tmp = Tr_ImgWithConstrain(myTr, Ddi_BddPartRead(overApprox, i), care);
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));

      Ddi_BddPartInsertLast(care, tmp);

      Ddi_BddSetPartConj(toPlus);
      Ddi_BddPartInsertLast(toPlus, tmp);
      Ddi_Free(tmp);
#endif

#if 0
      Ddi_BddRestrictAcc(Tr_TrBdd(myTr), Ddi_BddPartRead(overApprox, i));
#endif
      if (Ddi_BddIsPartConj(from)) {
        Ddi_BddPartInsertLast(from, Ddi_BddPartRead(overApprox, i));
      } else if (Ddi_BddIsPartConj(Ddi_BddPartRead(overApprox, i))) {
        Ddi_Bdd_t *t = Ddi_BddDup(Ddi_BddPartRead(overApprox, i));

        Ddi_BddPartInsert(t, 0, from);
        Ddi_Free(from);
        from = t;
      } else {
        Ddi_BddAndAcc(from, Ddi_BddPartRead(overApprox, i));
      }

      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "|From*OverAppr|=%d\n", Ddi_BddSize(from)));
#else
      Ddi_BddConstrainAcc(Tr_TrBdd(myTr), Ddi_BddPartRead(overApprox, i));
      Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "\n|TR|overAppr|=%d\n", Ddi_BddSize(Tr_TrBdd(myTr))));
#endif
#if 1
      Ddi_BddRestrictAcc(from, Ddi_BddPartRead(inRings, in_i - 1));
#endif
    }
#endif
    Pdtutil_VerbosityMgr(trMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "|TR|=%d\n", Ddi_BddSize(Tr_TrBdd(myTr))));
    if (reverse) {
      to = FbvApproxPreimg(myTr, from, care, NULL, 2 * approx, opt);
    } else {
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
      to = Tr_ImgWithConstrain(myTr, from, care);
      Ddi_BddSwapVarsAcc(care, Tr_MgrReadPS(trMgr), Tr_MgrReadNS(trMgr));
    }
    /*Ddi_BddSetMono(to); */

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c, Pdtutil_VerbLevelDevMin_c,
      LOG_CONST(to, "to")
      );

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMax_c,
      Ddi_Bdd_t * tom = Ddi_BddMakeMono(to); Ddi_BddSetMeta(tom);
      fprintf(stdout, "TO meta: %d\n", Ddi_BddSize(tom)); Ddi_Free(tom)
      );

    if (0 && approx) {
      /* add initial set so that paths shorter than maxIter
         are taken into account */
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "|To-0|=%d\n", Ddi_BddSize(to)));
      orProjectedSetAcc(to, from0, NULL);
    }

    if (opt->trav.constrain_level <= 1) {
      Tr_TrFree(myTr);
      myTr = Tr_TrDup(TR);
    }

    Ddi_Free(from);

    if (opt->trav.constrain_level >= 1) {
      Ddi_BddRestrictAcc(to, care);
    }

    if (opt->trav.constrain_level > 0) {
      from = Ddi_BddDup(to);
    } else {
      from = Ddi_BddAnd(care, to);
    }

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "|To|=%d.\n", Ddi_BddSize(to)));

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMax_c,
      tmp = Ddi_BddDup(to); Ddi_BddSetMono(tmp);
      fprintf(stdout, "|ToMono|=%d.\n", Ddi_BddSize(tmp)); Ddi_Free(tmp)
      );

#if 0
    if (overApprox != NULL) {
#if 1
      if (i < Ddi_BddPartNum(overApprox) - 1) {
        Ddi_BddSetPartConj(Ddi_BddPartRead(overApprox, i + 1));
        Ddi_BddPartInsertLast(Ddi_BddPartRead(overApprox, i + 1), toPlus);
      } else
#endif
      {
        Ddi_BddSetPartConj(from);
        Ddi_BddPartInsertLast(from, toPlus);
      }
      Ddi_Free(toPlus);
    }
#endif

#if 0
    if (Ddi_BddSize(from) > Ddi_BddSize(to)) {
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "|To|=%d\n", Ddi_BddSize(to)));
      Ddi_Free(from);
      from = Ddi_BddDup(to);
    } else
#endif
    {
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "|To*BRi|=%d\n", Ddi_BddSize(from)));
    }

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "|To|=%d", Ddi_BddSize(to)));

    if (reached != NULL) {
      Ddi_BddOrAcc(reached, to);

      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
        fprintf(stdout, "|Reached|=%d; ", Ddi_BddSize(reached));
        fprintf(stdout, "#ReachedStates=%g.\n",
          Ddi_BddCountMinterm(reached, Ddi_VararrayNum(Tr_MgrReadPS(trMgr))))
        );
    }
    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelDevMin_c,
      fprintf(stdout, "\n"));

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMax_c,
      currTime = util_cpu_time();
      fprintf(stdout, "Live nodes (peak): %u(%u) - Time: %s.\n",
        Ddi_MgrReadLiveNodes(ddiMgr),
        Ddi_MgrReadPeakLiveNodeCount(ddiMgr),
        util_print_time(currTime - initialTime))
      );

#if 0
    if ((!approx) && (Ddi_BddIsMono(to) ||
        (Ddi_BddIsPartConj(to) && (Ddi_BddPartNum(to) == 1)))) {
      Ddi_BddAndAcc(to, care);
      Ddi_BddNotAcc(to);
      Ddi_BddSetMono(to);
      Ddi_BddPartInsertLast(notReached, to);
    }
#endif

    Ddi_Free(care);
    Ddi_Free(to);
  }

  Tr_TrFree(myTr);
  Ddi_Free(notReached);
  Ddi_Free(reached);
  Ddi_Free(from);

  return (targetReached);

}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
dynAbstract(
  Ddi_Bdd_t * f,
  int maxSize,
  int maxAbstract
)
{
  Ddi_Varset_t *supp;
  Ddi_Bdd_t *newF;
  int i, j, nv, bestI, initSize;
  Ddi_Vararray_t *va;
  int over_under_approx;
  double refCnt, w, bestW;

  supp = Ddi_BddSupp(f);
  nv = Ddi_VarsetNum(supp);
  refCnt = Ddi_BddCountMinterm(f, nv);

  newF = Ddi_BddDup(f);
  va = Ddi_VararrayMakeFromVarset(supp, 1);
  Ddi_Free(supp);

  over_under_approx = 1;        /* 0: under, 1: over */
  j = 0;
  initSize = Ddi_BddSize(f);

  do {
    Ddi_Var_t *v;
    Ddi_Varset_t *vs;
    Ddi_Bdd_t *f0;
    Ddi_Bdd_t *f1;
    Ddi_Bdd_t *f01;

    bestI = -1;
    bestW = over_under_approx ? -1.0 : 0.0;
    for (i = 0; i < Ddi_VararrayNum(va); i++) {
      v = Ddi_VararrayRead(va, i);
      f0 = Ddi_BddCofactor(newF, v, 0);
      f1 = Ddi_BddCofactor(newF, v, 1);
      if (over_under_approx) {
        f01 = Ddi_BddOr(f0, f1);
      } else {
        f01 = Ddi_BddAnd(f0, f1);
      }
      Ddi_Free(f0);
      Ddi_Free(f1);
      w = Ddi_BddCountMinterm(f01, nv) / refCnt;
      if (bestI < 0 || ((Ddi_BddSize(f01) < Ddi_BddSize(newF)) &&
          (over_under_approx ? w < bestW : w > bestW))) {
        bestI = i;
        bestW = w;
      }
      Ddi_Free(f01);
    }
    Pdtutil_Assert(bestI >= 0, "bestI not found");
    v = Ddi_VararrayRead(va, bestI);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "best abstract var found: %s - cost: %f\n",
        Ddi_VarName(v), bestW));

    vs = Ddi_VarsetMakeFromVar(v);
    if (over_under_approx) {
      Ddi_BddExistAcc(newF, vs);
    } else {
      Ddi_BddForallAcc(newF, vs);
    }
    Ddi_Free(vs);
    Ddi_VararrayRemove(va, bestI);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
      Pdtutil_VerbLevelUsrMax_c,
      fprintf(stdout, "new size: %d\n", Ddi_BddSize(newF)));

    j++;

  } while (Ddi_BddSize(newF) > maxSize && (maxAbstract < 0
      || j < maxAbstract));

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
    Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "Dynamic Abstract (%sapprox): %d(nv:%d) -> %d(nv:%d)\n",
      over_under_approx ? "over" : "under",
      initSize, nv, Ddi_BddSize(newF), nv - j)
    );

  w = Ddi_BddCountMinterm(newF, nv);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMax_c,
    Pdtutil_VerbLevelUsrMax_c,
    fprintf(stdout, "Dynamic Abstract minterms: %g -> %g (%f)\n",
      refCnt, w, w / refCnt));

  Ddi_Free(va);

  return (newF);

}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects [None]
  SeeAlso     []
******************************************************************************/
static void
storeRingsCNF(
  Ddi_Bdd_t * rings,
  Fbv_Globals_t * opt
)
{
  int i, *vauxIds;
  char fname[100], **vnames;
  Ddi_Bdd_t *myRings;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(rings);
  int addedClauses, addedVars;

#if DOUBLE_MGR
  Ddi_Mgr_t *ddiMgr2 = Ddi_MgrDup(ddiMgr);

  Ddi_MgrSiftEnable(ddiMgr2, Ddi_ReorderingMethodString2Enum("sift"));
  Ddi_MgrSetSiftThresh(ddiMgr2, Ddi_MgrReadLiveNodes(ddiMgr));

  myRings = Ddi_BddCopy(ddiMgr2, rings);
  Ddi_MgrReduceHeap(ddiMgr2, Ddi_ReorderingMethodString2Enum("sift"), 0);

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "|rp| (original)  : %5d\n", Ddi_BddSize(rings));
    fprintf(stdout, "|rp| (sifted)  : %5d\n\n", Ddi_BddSize(myRings))
    );

  vauxIds = Ddi_MgrReadVarauxids(ddiMgr2);
  vnames = Ddi_MgrReadVarnames(ddiMgr2);
#else
  myRings = Ddi_BddDup(rings);
  vauxIds = Ddi_MgrReadVarauxids(ddiMgr);
  vnames = Ddi_MgrReadVarnames(ddiMgr);
#endif

  for (i = 0; i < Ddi_BddPartNum(myRings); i++) {
    Ddi_Bdd_t *p = Ddi_BddPartRead(myRings, i);

    sprintf(fname, "%sRP%d.cnf", opt->trav.storeCNF, i);
    switch (opt->trav.storeCNFmode) {
      case 'N':
        Ddi_BddStoreCnf(p, DDDMP_CNF_MODE_NODE, 0, vnames, NULL,
          vauxIds, NULL, -1, -1, -1, fname, NULL, &addedClauses, &addedVars);
        break;
      case 'M':
        Ddi_BddStoreCnf(p, DDDMP_CNF_MODE_MAXTERM, 0, vnames, NULL,
          vauxIds, NULL, -1, -1, -1, fname, NULL, &addedClauses, &addedVars);
        break;
      default:
        Ddi_BddStoreCnf(p, DDDMP_CNF_MODE_BEST, 0, vnames, NULL,
          vauxIds, NULL, -1, 1, opt->trav.maxCNFLength, fname, NULL,
          &addedClauses, &addedVars);
    }
#if 0
    sprintf(fname, "%sRP%d.bdd", opt->trav.storeCNF, i);
    Ddi_BddStore(p, NULL, DDDMP_MODE_TEXT, fname, NULL);
#endif
  }

  Ddi_Free(myRings);

#if DOUBLE_MGR
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "-- DDI Manager Free.\n"));
  Ddi_MgrQuit(ddiMgr2);
#endif

}

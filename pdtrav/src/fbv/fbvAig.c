/**CFile***********************************************************************

  FileName    [fbvAig.c]

  PackageName [fbv]

  Synopsis    [Aig based model check routines]

  Description []

  SeeAlso   []

  Author    [Gianpiero Cabodi]

  Copyright [This file was created at the Politecnico di Torino,
    Torino, Italy.
    The  Politecnico di Torino makes no warranty about the suitability of
    this software for any purpose.
    It is presented on an AS IS basis.
  ]

  Revision  []

******************************************************************************/

#include "fbvInt.h"
#include "ddiInt.h"

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


/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
FbvAigBckVerif(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int maxIter,
  Fbv_Globals_t * opt
)
{
  Ddi_Vararray_t *pi, *ps;
  Ddi_Varset_t *pivars, *over;
  Ddi_Bddarray_t *delta;
  Ddi_Bdd_t *reached, *from;
  int again, step = 0, i, fail = 0, cntR = 0;
  long startTime, currTime;

  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);

  ps = Fsm_MgrReadVarPS(fsmMgr);
  pi = Fsm_MgrReadVarI(fsmMgr);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "AIG backward verif (%d PIs, %d Latches).\n",
      Ddi_VararrayNum(pi), Ddi_VararrayNum(ps))
    );

  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  pivars = Ddi_VarsetMakeFromArray(pi);
  over = Ddi_VarsetDup(pivars);
  for (i = 0; i < Ddi_VararrayNum(ps); i++) {
    Ddi_VarsetAddAcc(over, Ddi_VararrayRead(ps, i));
  }
#if 0
  for (i = 0; i < Ddi_VararrayNum(ps); i++) {
    Ddi_BddSetMono(Ddi_BddarrayRead(delta, i));
    Ddi_BddSetAig(Ddi_BddarrayRead(delta, i));
  }
#endif
#if 0
  for (i = 0; i < Ddi_VararrayNum(ps); i++) {
    Ddi_AigOptByBdd(Ddi_BddarrayRead(delta, i));
  }
#endif

  from = Ddi_BddNot(invarspec);
  /* Ddi_BddSetMono(from); */
  Ddi_BddExistAcc(from, pivars);
  reached = Ddi_BddDup(from);


  startTime = util_cpu_time();

  do {
    int sizeAig;
    Ddi_Bdd_t *checkInit;
    Ddi_Bdd_t *fromAig, *over;
    Ddi_Bdd_t *careAig, *plus;

    checkInit = Ddi_BddAnd(from, init);
    if ((!opt->trav.noCheck) && (Ddi_AigSat(checkInit) == 1)) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "Verification Resut: FAIL.\n")
        );
      fail = 1;
      Ddi_Free(checkInit);
      Ddi_Free(from);
      break;
    }
    Ddi_Free(checkInit);

    if (maxIter > 0 && step >= maxIter) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Backward Verification: Maximum Iteration Reached.\n")
        );
      Ddi_Free(from);
      break;
    }

    fromAig = Ddi_BddDup(from);
    careAig = Ddi_BddNot(reached);

    Ddi_Free(from);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "AIG Traversal Iteration %d.\n", step)
      );

    if (0) {
      Ddi_Varset_t *suppd = Ddi_BddSupp(fromAig);
      Ddi_Bddarray_t *d2 = Ddi_BddarrayAlloc(ddm, 0);
      int k;

      for (i = 0; i < Ddi_VararrayNum(ps); i++) {
        if (Ddi_VarInVarset(suppd, Ddi_VararrayRead(ps, i))) {
          Ddi_BddarrayInsertLast(d2, Ddi_BddarrayRead(delta, i));
        }
      }
      if (Ddi_BddarrayNum(d2) > 0) {
        Ddi_AigArrayExistPartialAcc(d2, pivars, careAig);
      }
      for (i = 0, k = 0; i < Ddi_VararrayNum(ps); i++) {
        if (Ddi_VarInVarset(suppd, Ddi_VararrayRead(ps, i))) {
          Ddi_BddarrayWrite(delta, i, Ddi_BddarrayRead(d2, k++));
        }
      }
      Pdtutil_Assert(k == Ddi_BddarrayNum(d2), "Wrong K in d2");
      Ddi_Free(suppd);
      Ddi_Free(d2);
    }
    if (0 && Ddi_BddSize(reached) > 5000) {
      Ddi_Varset_t *suppd = Ddi_BddSupp(fromAig);

      for (i = 0; i < Ddi_VararrayNum(ps); i++) {
        if (Ddi_VarInVarset(suppd, Ddi_VararrayRead(ps, i))) {
          DdiAigRedRemovalAcc(Ddi_BddarrayRead(delta, i), careAig, -1, -1);
        }
      }
      Ddi_Free(suppd);
    }
    sizeAig = Ddi_BddarraySize(delta);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "AIG Traversal Iteration %d (|delta|=%d).\n", step,
        sizeAig)
      );

    Ddi_BddComposeAcc(fromAig, ps, delta);
    sizeAig = Ddi_BddSize(fromAig);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "AIG Traversal Iteration %d (|fromAig|=%d).\n",
        step++, sizeAig)
      );

    /*    Ddi_BddSetMono(fromAig); */

    if (1 || Ddi_BddSize(fromAig) > 3000) {
      Ddi_Bdd_t *tmp1 = Ddi_BddDup(fromAig);

      over = Ddi_BddDup(fromAig);
      //      Ddi_AigExistAcc (over,pivars,careAig,1,1);
#if 1
      if (opt->common.clk != NULL) {
        Ddi_Varset_t *ck =
          Ddi_VarsetMakeFromVar(Ddi_VarFromName(ddm, opt->common.clk));
        Ddi_BddExistAcc(over, ck);
        Ddi_Free(ck);
      }
      DdiAigExistOverAcc(over, pivars, careAig);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Approximate: %d.\n", Ddi_BddSize(over))
        );
#else
      {
        Ddi_Varset_t *supp = Ddi_BddSupp(over);
        Ddi_Bddarray_t *d2 = Ddi_BddarrayAlloc(ddm, 0);
        int k, i;

        for (i = k = 0; i < Ddi_VararrayNum(ps); i++) {
          if (Ddi_VarInVarset(supp, Ddi_VararrayRead(ps, i))) {
            Ddi_Varset_t *sm = Ddi_VarsetMakeFromVar(Ddi_VararrayRead(ps, i));

            DdiAigExistOverAcc(over, sm, careAig);
            Ddi_Free(sm);
            Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
              Pdtutil_VerbLevelNone_c,
              fprintf(stdout, "Approximate[%d]: %d.\n", k++, Ddi_BddSize(over))
              );
          }
        }
        Ddi_Free(supp);
      }
#endif
      DdiAigRedRemovalAcc(over, careAig, -1, -1);

#if 0
      Ddi_BddSetMono(over);
      Ddi_BddSetMono(tmp1);
      Ddi_BddExistAcc(tmp1, pivars);
      Pdtutil_Assert(Ddi_BddIncluded(tmp1, over),
        "Wrong result of AIG OVER EX.");
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Approximate Factor = %g.\n",
          Ddi_BddCountMinterm(tmp, Ddi_VararrayNum(ps)) /
          Ddi_BddCountMinterm(tmp1, Ddi_VararrayNum(ps)))
        );
#endif
      Ddi_Free(tmp1);
    } else {
      over = Ddi_BddMakeConstAig(ddm, 1);
    }
    if (0 && Ddi_BddSize(over) > 50) {
      Ddi_Free(fromAig);
      fromAig = Ddi_BddDup(over);
    } else {
      Ddi_BddAndAcc(careAig, over);
      DdiAigRedRemovalAcc(fromAig, careAig, -1, -1);
      Ddi_AigExistAcc(fromAig, pivars, careAig, 0, opt->ddi.aigPartial * 2,
        -1);
      DdiAigRedRemovalAcc(fromAig, careAig, -1, -1);
      Ddi_Free(careAig);
      Ddi_BddAndAcc(fromAig, over);
    }
    Ddi_Free(over);
#if 0
    from = Ddi_BddMakeMono(fromAig);
#else
    from = Ddi_BddDup(fromAig);
#endif

    Ddi_BddDiffAcc(from, reached);

#if 0
    again = !Ddi_BddIsZero(from);
#else
    again = Ddi_AigSat(from) == 1;
#endif
#if 1
    Ddi_Free(from);
    from = fromAig;
#if 1
    Ddi_BddNotAcc(from);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "# RED-R: ")
      );
    DdiAigRedRemovalAcc(reached, from, -1, -1);
    Ddi_BddNotAcc(from);
#endif
#endif
    Ddi_BddOrAcc(reached, from);

    if (cntR) {
      Ddi_Bdd_t *r = Ddi_BddMakeMono(reached);

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "# Reached states = %g.\n",
          Ddi_BddCountMinterm(r, Ddi_VararrayNum(ps)))
        );
      Ddi_Free(r);
    }
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "# Reached states = %g - Reached size = %d.\n",
        Ddi_BddCountMinterm(reached, Ddi_VararrayNum(ps)),
        Ddi_BddSize(reached));
      currTime = util_cpu_time();
      fprintf(stdout,
        "merge/checks: [1]%d/%d + [2]%d/%d(%d/%d)[diff: %d,%d] - Time: %s.\n",
        ddm->stats.aig.n_merge_1, ddm->stats.aig.n_check_1,
        ddm->stats.aig.n_merge_2, ddm->stats.aig.n_check_2,
        ddm->stats.aig.n_merge_3, ddm->stats.aig.n_check_3,
        ddm->stats.aig.n_diff, ddm->stats.aig.n_diff_1,
        util_print_time(currTime - startTime))
      );

  } while (again);

  if (!fail) {
    if (maxIter == 0 || step < maxIter) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Verification Result: PASS.\n")
        );
    }
  }

  Ddi_Free(from);
  Ddi_Free(over);
  Ddi_Free(pivars);

  return (reached);

}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
FbvAigFwdVerif(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Fbv_Globals_t * opt
)
{
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Varset_t *pivars, *smooth;
  Ddi_Bddarray_t *delta, *psLit, *nsLit;
  Ddi_Bdd_t *tr, *reached, *from, *bad, *to;
  int again, step = 0, i, nState;
  long startTime, currTime;
  Tr_Mgr_t *trMgr = NULL;

  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);

  if (0) {
    Ddi_Bdd_t *r = FbvAigFwdApproxTrav(fsmMgr, init, invar, 1, opt);

    Ddi_Free(r);
    return;
  }


  ps = Fsm_MgrReadVarPS(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  pi = Fsm_MgrReadVarI(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  nState = Ddi_BddarrayNum(delta);
  psLit = Ddi_BddarrayAlloc(ddm, nState);
  nsLit = Ddi_BddarrayAlloc(ddm, nState);

  pivars = Ddi_VarsetMakeFromArray(pi);
  smooth = Ddi_VarsetMakeFromArray(ps);
  Ddi_VarsetUnionAcc(smooth, pivars);
  Ddi_Free(pivars);

#if 0
  tr = Ddi_BddMakeConstAig(ddm, 1);
#else
  tr = Ddi_BddMakePartConjVoid(ddm);
#endif
  for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
    Ddi_Bdd_t *tr_i, *psLit_i, *nsLit_i;

#if 1
    nsLit_i = Ddi_BddMakeLiteralAig(Ddi_VararrayRead(ns, i), 1);
    psLit_i = Ddi_BddMakeLiteralAig(Ddi_VararrayRead(ps, i), 1);
#else
    nsLit_i = Ddi_BddMakeLiteral(Ddi_VararrayRead(ns, i), 1);
    psLit_i = Ddi_BddMakeLiteral(Ddi_VararrayRead(ps, i), 1);
    Ddi_BddSetMono(Ddi_BddarrayRead(delta, i));
#endif
    Ddi_BddarrayWrite(nsLit, i, nsLit_i);
    Ddi_BddarrayWrite(psLit, i, psLit_i);
    tr_i = Ddi_BddXnor(nsLit_i, Ddi_BddarrayRead(delta, i));
#if 0
    Ddi_BddAndAcc(tr, tr_i);
#else
    Ddi_BddPartInsertLast(tr, tr_i);
#endif
    Ddi_Free(tr_i);
    Ddi_Free(psLit_i);
    Ddi_Free(nsLit_i);
  }


  /**********************************************************************/
  /*                       generate TR manager                          */
  /**********************************************************************/

  trMgr = Tr_MgrInit("TR-manager", ddm);
  /* Iwls95 sorting method */
  if (strcmp(opt->tr.trSort, "iwls95") == 0) {
    Tr_MgrSetSortMethod(trMgr, Tr_SortWeight_c);
  } else if (strcmp(opt->tr.trSort, "pdt") == 0) {
    Tr_MgrSetOption(trMgr, Pdt_TrSort_c, inum, Tr_SortWeightPdt_c);
  } else if (strcmp(opt->tr.trSort, "ord") == 0) {
    Tr_MgrSetOption(trMgr, Pdt_TrSort_c, inum, Tr_SortOrd_c);
  } else if (strcmp(opt->tr.trSort, "size") == 0) {
    Tr_MgrSetOption(trMgr, Pdt_TrSort_c, inum, Tr_SortSize_c);
  } else if (strcmp(opt->tr.trSort, "minmax") == 0) {
    Tr_MgrSetOption(trMgr, Pdt_TrSort_c, inum, Tr_SortMinMax_c);
  } else if (strcmp(opt->tr.trSort, "none") == 0) {
    Tr_MgrSetOption(trMgr, Pdt_TrSort_c, inum, Tr_SortNone_c);
  } else {
    Tr_MgrSetOption(trMgr, Pdt_TrSort_c, inum, Tr_SortNone_c);
  }

  /* enable smoothing PIs while clustering */
  Tr_MgrSetClustSmoothPi(trMgr, 0);
  Tr_MgrSetOption(trMgr, Pdt_TrClustTh_c, inum, opt->trav.clustTh);
  Tr_MgrSetI(trMgr, pi);
  Tr_MgrSetPS(trMgr, ps);
  Tr_MgrSetNS(trMgr, ns);

  {
    Tr_Tr_t *trTr = Tr_TrMakeFromRel(trMgr, tr);

    Tr_MgrSetTr(trMgr, trTr);
    Tr_TrFree(trTr);
    Ddi_Free(tr);
    trTr = Tr_MgrReadTr(trMgr);
    Tr_TrSortIwls95(trTr);
    tr = Ddi_BddDup(Tr_TrBdd(trTr));

    //    Tr_TrFree(trTr);
    Tr_MgrQuit(trMgr);
  }

  Ddi_BddSetClustered(tr, opt->trav.clustTh);
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Clustered TR has %d partitions.\n", Ddi_BddPartNum(tr))
    );
  bad = Ddi_BddNot(invarspec);
  from = Ddi_BddDup(init);

#if 0
  Ddi_BddSetMono(tr);
  Ddi_BddSetMono(from);
  Ddi_BddSetMono(bad);
#endif

  reached = Ddi_BddCompose(from, ps, nsLit);

  startTime = util_cpu_time();


  do {
    int sizeAig;
    Ddi_Bdd_t *checkBad;
    Ddi_Bdd_t *fromAig;
    Ddi_Bdd_t *careAig;

    fromAig = Ddi_BddCompose(from, ns, psLit);
    Ddi_Free(from);

    checkBad = Ddi_BddAnd(fromAig, bad);
    if ((!opt->trav.noCheck) && (Ddi_AigSat(checkBad) == 1)) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Verification Result: FAIL.\n")
        );
      Ddi_Free(checkBad);
      Ddi_Free(fromAig);
      break;
    }

    Ddi_Free(checkBad);

    if (!Ddi_BddIsPartConj(tr)) {
      Ddi_BddAndAcc(fromAig, tr);
    }
    sizeAig = Ddi_BddSize(fromAig);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "AIG Traversal Iteration %d (|fromAig|=%d.\n", ++step,
        sizeAig)
      );

    careAig = Ddi_BddNot(reached);
#if 1
#if 0
    {
      Ddi_Varset_t *ck =
        Ddi_VarsetMakeFromVar(Ddi_VarFromName(ddm, opt->common.clk));
      Ddi_BddExistAcc(fromAig, ck);
      Ddi_Free(ck);
    }
    {
      Ddi_Bdd_t *tmp, *tmp1 = Ddi_BddDup(fromAig);

      DdiAigExistOverAcc(fromAig, smooth, careAig);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Approximate: %d.\n", Ddi_BddSize(fromAig))
        );
#if 0
      tmp = Ddi_BddMakeMono(fromAig);
      Ddi_BddSetMono(tmp1);
      Ddi_BddExistAcc(tmp1, smooth);
      Pdtutil_Assert(Ddi_BddIncluded(tmp1, tmp),
        "Wrong result of AIG OVER EX.");
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "approx factor = %g.\n",
          Ddi_BddCountMinterm(tmp, Ddi_VararrayNum(ps)) /
          Ddi_BddCountMinterm(tmp1, Ddi_VararrayNum(ps)))
        );
      Ddi_Free(tmp);
#endif
      Ddi_Free(tmp1);
    }
#else

    if (Ddi_BddIsPartConj(tr)) {
      Ddi_BddAndExistAcc(fromAig, tr, smooth);
    } else {
      Ddi_AigExistAcc(fromAig, smooth, careAig, 0, 0, -1);
    }
#endif
#else
    Ddi_BddExistAcc(fromAig, smooth);
#endif
    Ddi_Free(careAig);

    to = Ddi_BddDup(fromAig);
    Ddi_BddDiffAcc(to, reached);

#if 1
    again = Ddi_AigSat(to) == 1;
#else
    again = !Ddi_BddIsZero(to);
#endif
    Ddi_BddOrAcc(reached, to);

    Ddi_Free(to);
    from = fromAig;
    if (Ddi_BddSize(reached) < Ddi_BddSize(from)) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "FROM <- REACHED (%d).\n", Ddi_BddSize(reached))
        );
      Ddi_Free(from);
      from = Ddi_BddDup(reached);
    }

    if (1) {
      Ddi_Bdd_t *myR = Ddi_BddMakeMono(reached);

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "#Reached States = %g.\n",
          Ddi_BddCountMinterm(myR, Ddi_VararrayNum(ps)))
        );
      Ddi_Free(myR);
    }
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "#Reached States = %g.\n",
        Ddi_BddCountMinterm(reached, Ddi_VararrayNum(ps)));
      currTime = util_cpu_time();
      fprintf(stdout,
        "merge/checks: [1]%d/%d + [2]%d/%d(%d/%d)[diff: %d,%d] - Time: %s.\n",
        ddm->stats.aig.n_merge_1, ddm->stats.aig.n_check_1,
        ddm->stats.aig.n_merge_2, ddm->stats.aig.n_check_2,
        ddm->stats.aig.n_merge_3, ddm->stats.aig.n_check_3,
        ddm->stats.aig.n_diff, ddm->stats.aig.n_diff_1,
        util_print_time(currTime - startTime))
      );

  } while (again);

  Ddi_Free(reached);
  Ddi_Free(smooth);
  Ddi_Free(psLit);
  Ddi_Free(nsLit);

}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
FbvAigBckVerif2(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  Fbv_Globals_t * opt
)
{
  Ddi_Vararray_t *pi, *ps;
  Ddi_Varset_t *totPiVars;
  Ddi_Bddarray_t *delta;
  Ddi_Bdd_t *reached, *unroll, *to, *myInvarspec;
  int sat = 0, i, j, fixPoint = 0;
  int cntR = 0;
  long startTime, currTime;

  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);

  ps = Fsm_MgrReadVarPS(fsmMgr);
  pi = Fsm_MgrReadVarI(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);

  myInvarspec = Ddi_BddDup(invarspec);
  if (opt->trav.targetEn > 0) {
    Ddi_Bdd_t *r =
      FbvAigBckVerif(fsmMgr, init, invarspec, invar, opt->trav.targetEn, opt);
    Ddi_Free(myInvarspec);
    Ddi_BddNotAcc(r);
    myInvarspec = r;
  }

  unroll = Ddi_BddNot(myInvarspec);

  startTime = util_cpu_time();
  reached = Ddi_BddMakeConstAig(ddm, 0);
  totPiVars = Ddi_VarsetMakeFromArray(pi);

  for (i = 0; !fixPoint && !sat; i++) {
    Ddi_Bdd_t *check;
    int sat1, sat2;

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "AIG check at bound %d (AIG size: %d / r: %d).\n",
        i, Ddi_BddSize(unroll), Ddi_BddSize(reached))
      );

    if (!opt->trav.noCheck) {
      check = Ddi_BddAnd(unroll, init);
      if (strcmp(opt->common.satSolver, "csat") == 0) {
        sat1 = Ddi_AigSatCircuit(check);
      } else {
        sat1 = Ddi_AigSat(check);
      }
      /*Pdtutil_Assert(sat==sat2,"wrong result of circuit sat"); */
      if (sat1 == 1) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c, fprintf(stdout, "Failure at bound %d.\n", i)
          );
        sat = 1;
      }
      Ddi_Free(check);
    }

    if (!sat) {
      int j, sizeAig;
      Ddi_Bddarray_t *delta_i;
      Ddi_Bdd_t *invarspec_i;
      Ddi_Vararray_t *newPi = Ddi_VararrayAlloc(ddm, Ddi_VararrayNum(pi));
      Ddi_Bddarray_t *newPiLit = Ddi_BddarrayAlloc(ddm, Ddi_VararrayNum(pi));
      Ddi_Bdd_t *careAig = Ddi_BddNot(reached);

      for (j = 0; j < Ddi_VararrayNum(pi); j++) {
        char name[1000];
        Ddi_Var_t *v = Ddi_VararrayRead(pi, j);
        Ddi_Var_t *newv = Ddi_VarNewAfterVar(v);
        Ddi_Bdd_t *newvLit;

        sprintf(name, "%s_%d", Ddi_VarName(v), i);
        Ddi_VarAttachName(newv, name);
        Ddi_VararrayWrite(newPi, j, newv);
        Ddi_VarsetAddAcc(totPiVars, newv);
        newvLit = Ddi_BddMakeLiteralAig(newv, 1);
        Ddi_BddarrayWrite(newPiLit, j, newvLit);
        Ddi_Free(newvLit);
      }

      if (0) {
        Ddi_Varset_t *suppd = Ddi_BddSupp(unroll);
        Ddi_Bddarray_t *d2 = Ddi_BddarrayAlloc(ddm, 0);
        int k, i;

        for (i = 0; i < Ddi_VararrayNum(ps); i++) {
          if (Ddi_VarInVarset(suppd, Ddi_VararrayRead(ps, i))) {
            Ddi_BddarrayInsertLast(d2, Ddi_BddarrayRead(delta, i));
          }
        }
        if (Ddi_BddarrayNum(d2) > 0) {
          Ddi_AigArrayExistPartialAcc(d2, totPiVars, careAig);
        }
        for (i = 0, k = 0; i < Ddi_VararrayNum(ps); i++) {
          if (Ddi_VarInVarset(suppd, Ddi_VararrayRead(ps, i))) {
            Ddi_BddarrayWrite(delta, i, Ddi_BddarrayRead(d2, k++));
          }
        }
        Pdtutil_Assert(k == Ddi_BddarrayNum(d2), "Wrong K in d2");
        Ddi_Free(suppd);
        Ddi_Free(d2);
      }

      sizeAig = Ddi_BddarraySize(delta);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "|delta|=%d.\n", sizeAig)
        );

      delta_i = Ddi_BddarrayDup(delta);
      for (j = 0; j < Ddi_BddarrayNum(delta_i); j++) {
        Ddi_BddComposeAcc(Ddi_BddarrayRead(delta_i, j), pi, newPiLit);
      }
      Ddi_BddComposeAcc(unroll, ps, delta_i);
      invarspec_i = Ddi_BddCompose(myInvarspec, pi, newPiLit);

      check = Ddi_BddAnd(unroll, careAig);
      if (strcmp(opt->common.satSolver, "csat") == 0) {
        sat1 = Ddi_AigSatCircuit(check);
      } else {
        sat1 = Ddi_AigSat(check);
      }
      /*Pdtutil_Assert(sat==sat2,"wrong result of circuit sat"); */
      if (sat1 == 0) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "Fix point at bound %d.\n", i)
          );
        fixPoint = 1;
      }
      Ddi_Free(check);

      if (!fixPoint) {
        DdiAigRedRemovalAcc(unroll, careAig, -1, -1);
#if 0
        Ddi_AigExistAcc(unroll, totPiVars, careAig, 1 /*partial */ , 1);
#endif
        to = Ddi_BddDup(unroll);
#if 0
        Ddi_AigExistAcc(to, totPiVars, careAig, 0, 0);
#else
        Ddi_AigExistAllSolutionAcc(to, totPiVars, careAig, NULL, 2);
        //        DdiAigRedRemovalAcc (to,careAig,-1);
#endif
#if 1
        Ddi_BddNotAcc(to);
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c, fprintf(stdout, "# RED-R: ")
          );
        DdiAigRedRemovalAcc(reached, to, -1, -1);
        Ddi_BddNotAcc(to);
#endif
        Ddi_BddOrAcc(reached, to);

        if (cntR) {
          Ddi_Bdd_t *myR = Ddi_BddMakeMono(reached);

          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, "#Reached States = %g.\n",
              Ddi_BddCountMinterm(myR, Ddi_VararrayNum(ps)))
            );
          Ddi_Free(myR);
        }

        if (Ddi_BddSize(to) < Ddi_BddSize(unroll)) {
          Ddi_Free(unroll);
          unroll = Ddi_BddDup(to);
        } else {
          Ddi_AigExistAcc(unroll, totPiVars, careAig, 1 /*partial */ , 1, -1);
        }
        Ddi_Free(to);
      }

      Ddi_Free(careAig);
      Ddi_Free(invarspec_i);
      Ddi_Free(delta_i);
      Ddi_Free(newPi);
      Ddi_Free(newPiLit);
    }
  }

  if (!sat) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Fix Point Reached: Pass at iteration %d.\n", i)
      );
  }

  Ddi_Free(myInvarspec);
  Ddi_Free(unroll);
  Ddi_Free(totPiVars);

  return (reached);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
FbvAigBmcVerif(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int bound,
  int step,
  Fbv_Globals_t * opt
)
{
  Ddi_Vararray_t *pi, *ps;
  Ddi_Varset_t *totPiVars;
  Ddi_Bddarray_t *delta;
  Ddi_Bdd_t *reached, *unroll, *to, *myInvarspec;
  Ddi_Bdd_t *cubeInit;
  int sat = 0, i, j, initIsCube;
  long startTime, currTime;

  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);

  ps = Fsm_MgrReadVarPS(fsmMgr);
  pi = Fsm_MgrReadVarI(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);

  cubeInit = Ddi_BddMakeMono(init);
  initIsCube = Ddi_BddIsCube(cubeInit);
  Ddi_Free(cubeInit);

  myInvarspec = Ddi_BddDup(invarspec);
  if (opt->trav.targetEn > 0) {
    Ddi_Bdd_t *r =
      FbvAigBckVerif(fsmMgr, init, invarspec, invar, opt->trav.targetEn, opt);
    Ddi_Free(myInvarspec);
    Ddi_BddNotAcc(r);
    myInvarspec = r;
  }

  unroll = Ddi_BddNot(myInvarspec);

  startTime = util_cpu_time();
  reached = Ddi_BddMakeConstAig(ddm, 0);
  if (pi == NULL) {
    totPiVars = Ddi_VarsetVoid(ddm);
  } else {
    totPiVars = Ddi_VarsetMakeFromArray(pi);
  }

  for (i = 0; i < bound && !sat; i++) {
    if (((i) % step) == 0) {
      Ddi_Bdd_t *check;
      int sat1, sat2;

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "BMC check at bound %d (AIG size: %d).\n",
          i, Ddi_BddSize(unroll))
        );

      if (initIsCube) {
        check = Ddi_BddDup(unroll);
        Ddi_AigAndCubeAcc(check, init);
      } else {
        check = Ddi_BddAnd(unroll, init);
      }
      if (strcmp(opt->common.satSolver, "csat") == 0) {
        sat1 = Ddi_AigSatCircuit(check);
      } else if (strcmp(opt->common.satSolver, "minisat") == 0) {
        sat1 = Ddi_AigSatMinisat(check);
      } else {
        sat1 = Ddi_AigSat(check);
      }
      /*Pdtutil_Assert(sat==sat2,"wrong result of circuit sat"); */
      if (sat1 == 1) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "BMC failure at bound %d.\n", i)
          );
        sat = 1;
      }
      Ddi_Free(check);
    }
    if (!sat) {
      int j;
      Ddi_Bddarray_t *delta_i;
      Ddi_Bdd_t *invarspec_i;
      Ddi_Vararray_t *newPi = Ddi_VararrayAlloc(ddm, Ddi_VararrayNum(pi));
      Ddi_Bddarray_t *newPiLit = Ddi_BddarrayAlloc(ddm, Ddi_VararrayNum(pi));

      for (j = 0; j < Ddi_VararrayNum(pi); j++) {
        char name[1000];
        Ddi_Var_t *v = Ddi_VararrayRead(pi, j);
        Ddi_Var_t *newv = Ddi_VarNewAfterVar(v);
        Ddi_Bdd_t *newvLit;

        sprintf(name, "%s_%d", Ddi_VarName(v), i);
        Ddi_VarAttachName(newv, name);
        Ddi_VararrayWrite(newPi, j, newv);
        Ddi_VarsetAddAcc(totPiVars, newv);
        newvLit = Ddi_BddMakeLiteralAig(newv, 1);
        Ddi_BddarrayWrite(newPiLit, j, newvLit);
        Ddi_Free(newvLit);
      }
      delta_i = Ddi_BddarrayDup(delta);
      for (j = 0; j < Ddi_BddarrayNum(delta_i); j++) {
        Ddi_BddComposeAcc(Ddi_BddarrayRead(delta_i, j), pi, newPiLit);
      }
      Ddi_BddComposeAcc(unroll, ps, delta_i);
      invarspec_i = Ddi_BddCompose(myInvarspec, pi, newPiLit);
      if (i < step - 1) {
        Ddi_BddNotAcc(invarspec_i);
        Ddi_BddOrAcc(unroll, invarspec_i);
      } else {
        Ddi_BddAndAcc(unroll, invarspec_i);
      }
#if 0
      if (1) {
        Ddi_Bdd_t *careAig = Ddi_BddNot(reached);

        Ddi_AigExistAcc(unroll, totPiVars, careAig, 1 /*partial */ , 3);
        to = Ddi_BddDup(unroll);
        Ddi_AigExistAcc(to, totPiVars, careAig, 0, 0);
        Ddi_BddOrAcc(reached, to);
        Ddi_Free(to);
        Ddi_Free(careAig);
      }
#endif
      Ddi_Free(invarspec_i);
      Ddi_Free(delta_i);
      Ddi_Free(newPi);
      Ddi_Free(newPiLit);
    }
  }

  if (!sat) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "BMC Pass at Bound %d.\n", bound)
      );
  }

  Ddi_Free(myInvarspec);
  Ddi_Free(unroll);
  Ddi_Free(reached);
  Ddi_Free(totPiVars);

}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
FbvAigFwdApproxTrav(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invar,
  int step,
  Fbv_Globals_t * opt
)
{
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Varset_t *totPiVars;
  Ddi_Bddarray_t *delta;
  Ddi_Bdd_t *reached, *unroll, *to;
  Ddi_Bdd_t *cubeInit;
  int sat = 0, i, j, initIsCube, fixPoint = 0, nState;
  long startTime, currTime;
  Ddi_Bddarray_t *psLit, *nsLit;

  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);

  ps = Fsm_MgrReadVarPS(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  pi = Fsm_MgrReadVarI(fsmMgr);
  delta = Fsm_MgrReadDeltaBDD(fsmMgr);

  cubeInit = Ddi_BddMakeMono(init);
  initIsCube = Ddi_BddIsCube(cubeInit);
  Ddi_Free(cubeInit);

  nState = Ddi_BddarrayNum(delta);
  psLit = Ddi_BddarrayAlloc(ddm, nState);
  nsLit = Ddi_BddarrayAlloc(ddm, nState);
  startTime = util_cpu_time();
  reached = Ddi_BddMakeConstAig(ddm, 0);
  totPiVars = Ddi_VarsetMakeFromArray(pi);

  reached = Ddi_BddMakeConstAig(ddm, 0);
  /* build tr */
  unroll = Ddi_BddMakeConstAig(ddm, 1);
  for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
    Ddi_Bdd_t *tr_i, *psLit_i, *nsLit_i;

    nsLit_i = Ddi_BddMakeLiteralAig(Ddi_VararrayRead(ns, i), 1);
    psLit_i = Ddi_BddMakeLiteralAig(Ddi_VararrayRead(ps, i), 1);

    Ddi_BddarrayWrite(nsLit, i, nsLit_i);
    Ddi_BddarrayWrite(psLit, i, psLit_i);
    tr_i = Ddi_BddXnor(nsLit_i, Ddi_BddarrayRead(delta, i));
    Ddi_BddAndAcc(unroll, tr_i);
    Ddi_Free(tr_i);
    Ddi_Free(psLit_i);
    Ddi_Free(nsLit_i);
  }

  for (i = 0; !fixPoint; i++) {
    Ddi_Bdd_t *check;
    int j;
    Ddi_Bddarray_t *delta_i;
    Ddi_Bdd_t *invarspec_i;
    Ddi_Vararray_t *newPi = Ddi_VararrayAlloc(ddm, Ddi_VararrayNum(pi));
    Ddi_Bddarray_t *newPiLit = Ddi_BddarrayAlloc(ddm, Ddi_VararrayNum(pi));

    for (j = 0; j < Ddi_VararrayNum(pi); j++) {
      char name[1000];
      Ddi_Var_t *v = Ddi_VararrayRead(pi, j);
      Ddi_Var_t *newv = Ddi_VarNewAfterVar(v);
      Ddi_Bdd_t *newvLit;

      sprintf(name, "%s_%d", Ddi_VarName(v), i);
      Ddi_VarAttachName(newv, name);
      Ddi_VararrayWrite(newPi, j, newv);
      Ddi_VarsetAddAcc(totPiVars, newv);
      newvLit = Ddi_BddMakeLiteralAig(newv, 1);
      Ddi_BddarrayWrite(newPiLit, j, newvLit);
      Ddi_Free(newvLit);
    }
    delta_i = Ddi_BddarrayDup(delta);
    for (j = 0; j < Ddi_BddarrayNum(delta_i); j++) {
      Ddi_BddComposeAcc(Ddi_BddarrayRead(delta_i, j), pi, newPiLit);
    }
    Ddi_BddComposeAcc(unroll, ps, delta_i);

    if (initIsCube) {
      to = Ddi_BddDup(unroll);
      Ddi_AigAndCubeAcc(to, init);
    } else {
      to = Ddi_BddAnd(unroll, init);
    }

    Ddi_BddNotAcc(reached);
    Ddi_AigExistAcc(to, totPiVars, reached, 0, 0, -1);
    DdiAigExistOverAcc(to, totPiVars, reached);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "APPROX iter %d (AIG size: %d).\n", i, Ddi_BddSize(to))
      );

    check = Ddi_BddAnd(to, reached);
    if (strcmp(opt->common.satSolver, "csat") == 0) {
      sat = Ddi_AigSatCircuit(check);
    } else {
      sat = Ddi_AigSat(check);
    }
    fixPoint = !sat;
    Ddi_Free(check);
#if 0
    Ddi_Free(reached);
    reached = to;
#else
    Ddi_BddNotAcc(reached);
    Ddi_BddOrAcc(reached, to);
    Ddi_Free(to);
#endif

    Ddi_Free(delta_i);
    Ddi_Free(newPi);
    Ddi_Free(newPiLit);
  }

  Ddi_Free(psLit);
  Ddi_Free(nsLit);
  Ddi_Free(unroll);
  Ddi_Free(totPiVars);

  {
    Ddi_Bdd_t *r = Ddi_BddMakeMono(reached);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "#Reached States = %g.\n",
        Ddi_BddCountMinterm(r, Ddi_VararrayNum(ps)))
      );
    Ddi_Free(r);
  }
  return (reached);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
FbvAigBmcTrVerif(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int bound,
  int step
)
{
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Varset_t *pivars, *smooth;
  Ddi_Bddarray_t *delta, *psLit, *nsLit;
  Ddi_Bdd_t *reached, *unroll, *tr, *tr_i;
  int sat = 0, i, j, nState;
  long startTime, currTime;

  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);

  ps = Fsm_MgrReadVarPS(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  pi = Fsm_MgrReadVarI(fsmMgr);

  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  nState = Ddi_BddarrayNum(delta);
  psLit = Ddi_BddarrayAlloc(ddm, nState);
  nsLit = Ddi_BddarrayAlloc(ddm, nState);

  pivars = Ddi_VarsetMakeFromArray(pi);
  smooth = Ddi_VarsetMakeFromArray(ps);
  Ddi_VarsetUnionAcc(smooth, pivars);

  /* build tr */
  tr = Ddi_BddMakeConstAig(ddm, 1);
  for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
    Ddi_Bdd_t *tr_i, *psLit_i, *nsLit_i;

    nsLit_i = Ddi_BddMakeLiteralAig(Ddi_VararrayRead(ns, i), 1);
    psLit_i = Ddi_BddMakeLiteralAig(Ddi_VararrayRead(ps, i), 1);

    Ddi_BddarrayWrite(nsLit, i, nsLit_i);
    Ddi_BddarrayWrite(psLit, i, psLit_i);
    tr_i = Ddi_BddXnor(nsLit_i, Ddi_BddarrayRead(delta, i));
    Ddi_BddAndAcc(tr, tr_i);
    Ddi_Free(tr_i);
    Ddi_Free(psLit_i);
    Ddi_Free(nsLit_i);
  }

#if 0
  Ddi_BddExistAcc(tr, pivars);
#endif

  Ddi_Free(pivars);

  unroll = Ddi_BddNot(invarspec);

  startTime = util_cpu_time();

  for (i = 0; i < bound && !sat; i++) {
    if (((i) % step) == 0) {
      Ddi_Bdd_t *check;

      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "BMC check at bound %d (AIG size: %d).\n",
          i, Ddi_BddSize(unroll))
        );

      check = Ddi_BddAnd(unroll, init);
      if (Ddi_AigSat(check) == 1) {
        Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
          Pdtutil_VerbLevelNone_c,
          fprintf(stdout, "BMC Failure at Bound %d.\n", i)
          );
        sat = 1;
      }
      Ddi_Free(check);
    }
    if (!sat) {
      int j;
      Ddi_Bddarray_t *delta_i;
      Ddi_Bdd_t *invarspec_i;
      Ddi_Vararray_t *newPi = Ddi_VararrayAlloc(ddm, Ddi_VararrayNum(pi));
      Ddi_Bddarray_t *newPiLit = Ddi_BddarrayAlloc(ddm, Ddi_VararrayNum(pi));
      Ddi_Vararray_t *newNs = Ddi_VararrayAlloc(ddm, Ddi_VararrayNum(ns));
      Ddi_Bddarray_t *newNsLit = Ddi_BddarrayAlloc(ddm, Ddi_VararrayNum(ns));

      for (j = 0; j < Ddi_VararrayNum(pi); j++) {
        char name[1000];
        Ddi_Var_t *v = Ddi_VararrayRead(pi, j);
        Ddi_Var_t *newv = Ddi_VarNewAfterVar(v);
        Ddi_Bdd_t *newvLit;

        sprintf(name, "%s_%d", Ddi_VarName(v), i);
        Ddi_VarAttachName(newv, name);
        Ddi_VararrayWrite(newPi, j, newv);
        newvLit = Ddi_BddMakeLiteralAig(newv, 1);
        Ddi_BddarrayWrite(newPiLit, j, newvLit);
        Ddi_Free(newvLit);
      }
      for (j = 0; j < Ddi_VararrayNum(ns); j++) {
        char name[1000];
        Ddi_Var_t *v = Ddi_VararrayRead(ns, j);
        Ddi_Var_t *newv = Ddi_VarNewAfterVar(v);
        Ddi_Bdd_t *newvLit;

        sprintf(name, "%s_%d", Ddi_VarName(v), i);
        Ddi_VarAttachName(newv, name);
        Ddi_VararrayWrite(newNs, j, newv);
        newvLit = Ddi_BddMakeLiteralAig(newv, 1);
        Ddi_BddarrayWrite(newNsLit, j, newvLit);
        Ddi_Free(newvLit);
      }
      tr_i = Ddi_BddCompose(tr, pi, newPiLit);
      Ddi_BddComposeAcc(tr_i, ns, newNsLit);
      Ddi_BddComposeAcc(unroll, ps, newNsLit);
      invarspec_i = Ddi_BddCompose(invarspec, pi, newPiLit);
      Ddi_BddAndAcc(unroll, tr_i);
      if (i < step - 1) {
        Ddi_BddNotAcc(invarspec_i);
        Ddi_BddOrAcc(unroll, invarspec_i);
      } else {
        Ddi_BddAndAcc(unroll, invarspec_i);
      }
      Ddi_Free(invarspec_i);
      Ddi_Free(tr_i);
      Ddi_Free(newPi);
      Ddi_Free(newPiLit);
      Ddi_Free(newNs);
      Ddi_Free(newNsLit);
    }
  }

  if (!sat) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "BMC Pass at Bound %d.\n", bound)
      );
  }

  Ddi_Free(unroll);
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
FbvAigInductiveTrVerif(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bdd_t * init,
  Ddi_Bdd_t * invarspec,
  Ddi_Bdd_t * invar,
  int step,
  Fbv_Globals_t * opt
)
{
  Ddi_Vararray_t *pi, *ps, *ns;
  Ddi_Varset_t *pivars, *smooth;
  Ddi_Bddarray_t *delta, *psLit, *nsLit;
  Ddi_Bdd_t *reached, *unroll, *target, *tr, *tr_i,
    *loopFree, *loopFreeBck, *myInvarspec;
  int sat = 0, i, j, k, l, nState, initIsCube, nCoi = 0;
  long startTime, currTime, timeFrameLitsSize = 1;
  Ddi_Bddarray_t **timeFrameLits =
    Pdtutil_Alloc(Ddi_Bddarray_t *, timeFrameLitsSize);
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(init);
  Ddi_Varsetarray_t *timeFrameCoi = Ddi_VarsetarrayAlloc(ddm, 0);
  Ddi_Varset_t *supp, *psVars;

  ps = Fsm_MgrReadVarPS(fsmMgr);
  ns = Fsm_MgrReadVarNS(fsmMgr);
  pi = Fsm_MgrReadVarI(fsmMgr);
  psVars = Ddi_VarsetMakeFromArray(ps);

  delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  nState = Ddi_BddarrayNum(delta);
  psLit = Ddi_BddarrayAlloc(ddm, nState);
  nsLit = Ddi_BddarrayAlloc(ddm, nState);

  pivars = Ddi_VarsetMakeFromArray(pi);
  smooth = Ddi_VarsetMakeFromArray(ps);
  Ddi_VarsetUnionAcc(smooth, pivars);

  initIsCube = Ddi_BddIsCube(init);

  myInvarspec = Ddi_BddDup(invarspec);
  if (opt->trav.targetEn > 0) {
    Ddi_Bdd_t *r =
      FbvAigBckVerif(fsmMgr, init, invarspec, invar, opt->trav.targetEn, opt);
    Ddi_Free(myInvarspec);
    Ddi_BddNotAcc(r);
    myInvarspec = r;
  }

  /* build tr */
  tr = Ddi_BddMakeConstAig(ddm, 1);
  for (i = 0; i < Ddi_BddarrayNum(delta); i++) {
    Ddi_Bdd_t *tr_i, *psLit_i, *nsLit_i;

    nsLit_i = Ddi_BddMakeLiteralAig(Ddi_VararrayRead(ns, i), 1);
    psLit_i = Ddi_BddMakeLiteralAig(Ddi_VararrayRead(ps, i), 1);

    Ddi_BddarrayWrite(nsLit, i, nsLit_i);
    Ddi_BddarrayWrite(psLit, i, psLit_i);
    tr_i = Ddi_BddXnor(nsLit_i, Ddi_BddarrayRead(delta, i));
    Ddi_BddAndAcc(tr, tr_i);
    Ddi_Free(tr_i);
    Ddi_Free(psLit_i);
    Ddi_Free(nsLit_i);
  }

#if 0
  Ddi_BddExistAcc(tr, pivars);
#endif

  Ddi_Free(pivars);

  loopFree = Ddi_BddMakeConstAig(ddm, 1);
  unroll = Ddi_BddMakeConstAig(ddm, 1);
  DdiAigRedRemovalAcc(tr, myInvarspec, -1, -1);
  Ddi_BddAndAcc(tr, myInvarspec);

  startTime = util_cpu_time();

  timeFrameLits[0] = Ddi_BddarrayDup(psLit);
  supp = Ddi_BddSupp(myInvarspec);
  Ddi_VarsetIntersectAcc(supp, psVars);
  Ddi_VarsetarrayInsertLast(timeFrameCoi, supp);
  Ddi_Free(supp);

  for (i = 0; !sat; i++) {
    Ddi_Bdd_t *check;
    int fixPoint = 0;

    target = Ddi_BddNot(myInvarspec);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Inductive check at depth %d (unroll: %d).\n",
        i, Ddi_BddSize(unroll))
      );
    Ddi_BddComposeAcc(target, ps, timeFrameLits[i]);
    if (initIsCube) {
      check = Ddi_BddDup(unroll);
      Ddi_AigAndCubeAcc(check, init);
    } else {
      check = Ddi_BddAnd(unroll, init);
    }
    Ddi_BddAndAcc(check, target);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, " {COI:%d}", nCoi);
      fprintf(stdout, "BMC (AIG size: %d)", Ddi_BddSize(check))
      );
    startTime = util_cpu_time();
    sat = Ddi_AigSat(check) == 1;
    Ddi_Free(check);
    if (sat) {
      Ddi_Free(target);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "BMC Failure at Bound %d.\n", i)
        );
      break;
    }

    /* backward induction */

    loopFreeBck = Ddi_BddMakeConstAig(ddm, 0);
    for (l = i - 1; l > 0; l--) {
      Ddi_Bdd_t *diff_i, *diff_j, *diff_k;
      Ddi_Varset_t *suppCoi = Ddi_VarsetarrayRead(timeFrameCoi, i - l);

      diff_i = Ddi_BddMakeConstAig(ddm, 1);
      for (j = 0; j < l; j++) {
        Ddi_Bdd_t *diff_j = Ddi_BddMakeConstAig(ddm, 0);

        for (k = 0; k < Ddi_VararrayNum(ns); k++) {
          if (Ddi_VarInVarset(suppCoi, Ddi_VararrayRead(ns, k))) {
            Ddi_Bdd_t *diff_k =
              Ddi_BddXor(Ddi_BddarrayRead(timeFrameLits[l], k),
              Ddi_BddarrayRead(timeFrameLits[j], k));

            Ddi_BddOrAcc(diff_j, diff_k);
            Ddi_Free(diff_k);
          }
        }
        Ddi_BddAndAcc(diff_i, diff_j);
        Ddi_Free(diff_j);
      }
      Ddi_BddAndAcc(loopFree, diff_i);
      Ddi_Free(diff_i);
    }

    check = Ddi_BddAnd(unroll, target);
    Ddi_Free(target);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "BWD (AIG size: %d(noLF)", Ddi_BddSize(check))
      );
    {
      Ddi_Bdd_t *cexAig = Ddi_AigSatWithCex(check);

      fixPoint = (cexAig == NULL);
      if (!fixPoint) {
        int doLF = 0;
        Ddi_Bdd_t *lfDup = Ddi_BddDup(loopFreeBck);

        Ddi_AigAndCubeAcc(lfDup, cexAig);
        doLF = Ddi_AigSat(lfDup) == 1;
        if (doLF) {
          Ddi_BddAndAcc(check, loopFreeBck);
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c, fprintf(stdout, "/%d", Ddi_BddSize(check))
            );
          fixPoint = Ddi_AigSat(check) != 1;
        }
        Ddi_Free(lfDup);
      }
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, ")")
        );
      Ddi_Free(check);
    }
    Ddi_Free(loopFreeBck);
    if (fixPoint) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "BWD Fix Point at Bound %d.\n", i)
        );
      break;
    }

    /* forward induction */
    if (initIsCube) {
      check = Ddi_BddDup(unroll);
      Ddi_AigAndCubeAcc(check, init);
    } else {
      check = Ddi_BddAnd(unroll, init);
    }
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "FWD (AIG size: %d(noL)", Ddi_BddSize(check))
      );
    {
      Ddi_Bdd_t *cexAig = Ddi_AigSatWithCex(check);

      fixPoint = (cexAig == NULL);
      if (!fixPoint) {
        int doLF = 0;
        Ddi_Bdd_t *lfDup = Ddi_BddDup(loopFree);

        Ddi_AigAndCubeAcc(lfDup, cexAig);
        doLF = Ddi_AigSat(lfDup) == 1;
        if (doLF) {
          Ddi_BddAndAcc(check, loopFree);
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c, fprintf(stdout, "/%d", Ddi_BddSize(check))
            );
          fixPoint = Ddi_AigSat(check) != 1;
        }
        Ddi_Free(lfDup);
      }
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, ")")
        );
      Ddi_Free(check);
    }
    if (fixPoint) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "FWD fix point at bound %d.\n", i)
        );
      break;
    }
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "")
      );

    if (1) {
      int j;
      Ddi_Bddarray_t *delta_i;
      Ddi_Bdd_t *invarspec_i, *diff_i;
      Ddi_Vararray_t *newPi = Ddi_VararrayAlloc(ddm, Ddi_VararrayNum(pi));
      Ddi_Bddarray_t *newPiLit = Ddi_BddarrayAlloc(ddm, Ddi_VararrayNum(pi));
      Ddi_Vararray_t *newNs = Ddi_VararrayAlloc(ddm, Ddi_VararrayNum(ns));
      Ddi_Bddarray_t *newNsLit = Ddi_BddarrayAlloc(ddm, Ddi_VararrayNum(ns));
      Ddi_Varset_t *coiSupp = Ddi_VarsetVoid(ddm);

      nCoi = 0;
      for (j = 0; j < Ddi_VararrayNum(pi); j++) {
        char name[1000];
        Ddi_Var_t *v = Ddi_VararrayRead(pi, j);
        Ddi_Var_t *newv = Ddi_VarNewAfterVar(v);
        Ddi_Bdd_t *newvLit;

        sprintf(name, "%s_%d", Ddi_VarName(v), i);
        Ddi_VarAttachName(newv, name);
        Ddi_VararrayWrite(newPi, j, newv);
        newvLit = Ddi_BddMakeLiteralAig(newv, 1);
        Ddi_BddarrayWrite(newPiLit, j, newvLit);
        Ddi_Free(newvLit);
      }
      for (j = 0; j < Ddi_VararrayNum(ns); j++) {
        char name[1000];
        Ddi_Var_t *v = Ddi_VararrayRead(ns, j);
        Ddi_Var_t *newv = Ddi_VarNewAfterVar(v);
        Ddi_Bdd_t *newvLit;

        sprintf(name, "%s_%d", Ddi_VarName(v), i);
        Ddi_VarAttachName(newv, name);
        Ddi_VararrayWrite(newNs, j, newv);
        newvLit = Ddi_BddMakeLiteralAig(newv, 1);
        Ddi_BddarrayWrite(newNsLit, j, newvLit);
        Ddi_Free(newvLit);
        if (Ddi_VarInVarset(Ddi_VarsetarrayRead(timeFrameCoi, i),
            Ddi_VararrayRead(ps, j))) {
          nCoi++;
          supp = Ddi_BddSupp(Ddi_BddarrayRead(delta, j));
          Ddi_VarsetUnionAcc(coiSupp, supp);
          Ddi_Free(supp);
        }
      }
      tr_i = Ddi_BddCompose(tr, pi, newPiLit);
      Ddi_BddComposeAcc(tr_i, ns, newNsLit);
      Ddi_BddComposeAcc(tr_i, ps, timeFrameLits[i]);
      if (i + 1 >= timeFrameLitsSize) {
        timeFrameLitsSize *= 2;
        timeFrameLits = Pdtutil_Realloc(Ddi_Bddarray_t *,
          timeFrameLits, timeFrameLitsSize);
      }
      timeFrameLits[i + 1] = Ddi_BddarrayDup(newNsLit);
      Ddi_VarsetIntersectAcc(coiSupp, psVars);
      Ddi_VarsetarrayInsertLast(timeFrameCoi, coiSupp);
      //printf("{COI:%d} ", nCoi);fflush(stdout);

      Ddi_BddAndAcc(unroll, tr_i);

      Ddi_Free(tr_i);
      Ddi_Free(newPi);
      Ddi_Free(newPiLit);
      Ddi_Free(newNs);
      Ddi_Free(newNsLit);
      Ddi_Free(coiSupp);

      diff_i = Ddi_BddMakeConstAig(ddm, 1);
      for (j = 0; j <= i; j++) {
        Ddi_Bdd_t *diff_j = Ddi_BddMakeConstAig(ddm, 0);

        for (k = 0; k < Ddi_VararrayNum(ns); k++) {
          Ddi_Bdd_t *diff_k =
            Ddi_BddXor(Ddi_BddarrayRead(timeFrameLits[i + 1], k),
            Ddi_BddarrayRead(timeFrameLits[j], k));

          Ddi_BddOrAcc(diff_j, diff_k);
          Ddi_Free(diff_k);
        }
        Ddi_BddAndAcc(diff_i, diff_j);
        Ddi_Free(diff_j);
      }
      Ddi_BddAndAcc(loopFree, diff_i);
      Ddi_Free(diff_i);
    }
  }

  if (!sat) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "Inductive Step at Depth %d.\n", i)
      );
  }

  for (--i; i >= 0; i--) {
    Ddi_Free(timeFrameLits[i]);
  }
  Pdtutil_Free(timeFrameLits);


  Ddi_Free(psVars);
  Ddi_Free(myInvarspec);
  Ddi_Free(loopFree);
  Ddi_Free(unroll);
  Ddi_Free(target);
}


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

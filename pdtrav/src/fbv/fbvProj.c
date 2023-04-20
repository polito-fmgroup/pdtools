/**CFile***********************************************************************

  FileName    [fbvProj.c]

  PackageName [fbv]

  Synopsis    [Projections over levelized cone routines]

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
#include "trav.h"
#include "expr.h"

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

static Ddi_Bdd_t *trProjectionPart(
  Tr_Mgr_t * trMgr,
  Ddi_Vararray_t * ps2,
  Ddi_Vararray_t * ns2,
  Ddi_Varsetarray_t * domain,
  Ddi_Varsetarray_t * range,
  Ddi_Bdd_t * spec,
  int ndpart,
  int np,
  int nv
);
static Ddi_Varsetarray_t *PartFilteredSupp(
  Ddi_Bdd_t * f,
  Ddi_Varset_t * vars
);
static void trAddGuards(
  Ddi_Bdd_t * f,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * ns,
  Ddi_Vararray_t * ps2,
  Ddi_Vararray_t * ns2,
  Ddi_Varsetarray_t * dma,
  Ddi_Varsetarray_t * rnga
);

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
Tr_Tr_t *
FbvTrProjection(
  Tr_Tr_t * tr,
  Ddi_Bdd_t * spec
)
{
  int i, j, np, nv, lev, npspace, removed, *removedFlags;
  Ddi_Vararray_t *ps2, *ns2;
  Ddi_Varset_t *psv, *nsv, *auxv;
  Ddi_Var_t *p, *n, *p2, *n2;
  Ddi_Bdd_t *part, *newTrBdd, *nl, *n2l, *eq, *guardSpace, *tri;
  Tr_Tr_t *trp;
  Tr_Mgr_t *trMgr = Tr_TrMgr(tr);
  Ddi_Bdd_t *trBdd = Tr_TrBdd(tr);
  Ddi_Vararray_t *ps = Tr_MgrReadPS(trMgr);
  Ddi_Vararray_t *ns = Tr_MgrReadNS(trMgr);
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(trBdd);
  char buf[PDTUTIL_MAX_STR_LEN + 1];
  Ddi_Varsetarray_t *domain, *range;

  np = Ddi_BddPartNum(trBdd);
  nv = Ddi_VararrayNum(ps);

  ps2 = Ddi_VararrayAlloc(ddm, nv);
  ns2 = Ddi_VararrayAlloc(ddm, nv);

  psv = Ddi_VarsetMakeFromArray(ps);
  nsv = Ddi_VarsetMakeFromArray(ns);

  for (i = 0; i < nv; i++) {
#if 0
    lev = Ddi_VarCurrPos(Ddi_VararrayRead(ps, i));
#else
    lev = 0;
#endif
    p = Ddi_VarNewAtLevel(ddm, lev);
    sprintf(buf, "G$%d", i);
    Ddi_VarAttachName(p, buf);
    n = Ddi_VarNewAtLevel(ddm, lev);
    strcat(buf, "$NS");
    Ddi_VarAttachName(n, buf);
    Ddi_VararrayWrite(ps2, i, p);
    Ddi_VararrayWrite(ns2, i, n);
    Ddi_VarMakeGroup(ddm, p, 2);
  }

  range = PartFilteredSupp(Tr_TrBdd(tr), nsv);
  for (i = 0; i < np; i++) {
    Ddi_VarsetSwapVarsAcc(Ddi_VarsetarrayRead(range, i), ps, ns);
  }

  tri = Ddi_BddDup(Tr_TrBdd(tr));
  domain = PartFilteredSupp(tri, psv);

  trAddGuards(tri, ps, ns, ps2, ns2, domain, range);

  guardSpace = trProjectionPart(trMgr,
    ps2, ns2, domain, range, spec, 1, np, nv);

  trp = Tr_TrMakeFromRel(trMgr, tri);
  Ddi_Free(tri);
  Tr_TrSortIwls95(trp);
  Tr_TrSetClustered(trp);

  Ddi_Free(domain);

  tri = Tr_TrBdd(trp);

#if 1

  removedFlags = Pdtutil_Alloc(int,
    nv
  );

  for (i = 0; i < nv; i++) {
    removedFlags[i] = 0;
  }
  for (i = 0, removed = 0; i < nv; i++) {
    /* skip removed variables */
    if (removedFlags[i])
      continue;
    p = Ddi_VararrayRead(ps2, i);
    n = Ddi_VararrayRead(ns2, i);
    nl = Ddi_BddMakeLiteral(n, 1);
    for (j = i + 1; j < np; j++) {
      /* skip removed variables */
      if (removedFlags[j])
        continue;
      p2 = Ddi_VararrayRead(ps2, j);
      n2 = Ddi_VararrayRead(ns2, j);
      n2l = Ddi_BddMakeLiteral(n2, 1);
      eq = Ddi_BddXnor(nl, n2l);
      if (Ddi_BddIncluded(guardSpace, eq)) {
        /* p2/n2 can be substituted by p/n */
        Ddi_Vararray_t *va, *vb;

        va = Ddi_VararrayAlloc(ddm, 2);
        vb = Ddi_VararrayAlloc(ddm, 2);
        Ddi_VararrayWrite(va, 0, p2);
        Ddi_VararrayWrite(va, 1, n2);
        Ddi_VararrayWrite(vb, 0, p);
        Ddi_VararrayWrite(vb, 1, n);
        Ddi_BddSubstVarsAcc(guardSpace, va, vb);
        Ddi_BddSubstVarsAcc(tri, va, vb);
        /*Ddi_BddSubstVarsAcc(spec,va,vb); */
        Ddi_Free(va);
        Ddi_Free(vb);
        removedFlags[j] = 1;
        removed++;
      }
      Ddi_Free(eq);
      Ddi_Free(n2l);
    }
    Ddi_Free(nl);
  }
  Pdtutil_Free(removedFlags);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "Removed %d/%d Guard Variables.\n", removed, nv)
    );

#endif

  Ddi_BddPartInsert(tri, 0, guardSpace);

  Ddi_Free(guardSpace);

  Ddi_VararrayAppend(ps, ps2);
  Ddi_VararrayAppend(ns, ns2);

  Ddi_Free(range);
  Ddi_Free(ps2);
  Ddi_Free(ns2);
  Ddi_Free(psv);
  Ddi_Free(nsv);

  return (trp);

}

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/






/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
trProjectionPart(
  Tr_Mgr_t * trMgr,
  Ddi_Vararray_t * ps2,
  Ddi_Vararray_t * ns2,
  Ddi_Varsetarray_t * domain,
  Ddi_Varsetarray_t * range,
  Ddi_Bdd_t * spec,
  int ndpart,
  int np,
  int nv
)
{
  int i, j, k, again, efound, eIncl, maxsupp, nsupp;
  Ddi_Bdd_t *lit, *term, *reached;
  Ddi_Var_t *p, *p2, *n;
  Ddi_Varset_t *psv, *nsv, *nsv2, *dm, *rng, *edm, *aux;
  Ddi_Varsetarray_t *extDomain, *newExt;
  Ddi_Vararray_t *ps = Tr_MgrReadPS(trMgr);
  Ddi_Vararray_t *ns = Tr_MgrReadNS(trMgr);
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(ps2);

  psv = Ddi_VarsetMakeFromArray(ps);
  nsv = Ddi_VarsetMakeFromArray(ns);

  extDomain = Ddi_VarsetarrayAlloc(ddm, 0);
  maxsupp = 0;

#if 0
  for (i = 0; i < np; i++) {
    rng = Ddi_VarsetarrayRead(range, i);
    Ddi_VarsetarrayWrite(extDomain, i, rng);
    nsupp = Ddi_VarsetNum(rng);
    if (maxsupp < nsupp) {
      maxsupp = nsupp;
    }
  }
#endif

  dm = Ddi_BddSupp(spec);
  Ddi_VarsetarrayInsertLast(extDomain, dm);
  nsupp = Ddi_VarsetNum(dm);
  if (maxsupp < nsupp) {
    maxsupp = nsupp;
  }
  Ddi_Free(dm);

  do {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "EXT DOMAIN iteration.\n")
      );
#if 0
    for (j = 0; j < Ddi_VarsetarrayNum(extDomain); j++) {
      dm = Ddi_VarsetarrayRead(extDomain, j);
      if (dm == NULL)
        continue;
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "EXT DOMAIN[%d].\n", j)
        );
      Ddi_VarsetPrint(dm, 100, NULL, stdout);
    }
#endif
    newExt = Ddi_VarsetarrayAlloc(ddm, 0);
    for (i = 0; i < Ddi_VarsetarrayNum(extDomain); i++) {
      dm = Ddi_VarsetarrayRead(extDomain, i);
      if (dm == NULL)
        continue;
      edm = Ddi_VarsetVoid(ddm);
      for (j = 0; j < np; j++) {
        rng = Ddi_VarsetarrayRead(range, j);
        aux = Ddi_VarsetIntersect(dm, rng);
        if (!Ddi_VarsetIsVoid(aux)) {
          Ddi_VarsetUnionAcc(edm, Ddi_VarsetarrayRead(domain, j));
        }
        Ddi_Free(aux);
      }
      Ddi_VarsetarrayInsertLast(newExt, edm);
      Ddi_Free(edm);
    }
#if 0
    for (j = 0; j < Ddi_VarsetarrayNum(newExt); j++) {
      dm = Ddi_VarsetarrayRead(newExt, j);
      if (dm == NULL)
        continue;
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "NEW EXT DOMAIN[%d].\n", j)
        );
      Ddi_VarsetPrint(dm, 100, NULL, stdout);
    }
#endif
    again = 0;
    for (i = 0; i < Ddi_VarsetarrayNum(newExt); i++) {
      edm = Ddi_VarsetarrayRead(newExt, i);
      efound = 0;
      eIncl = 0;
      for (j = 0; j < Ddi_VarsetarrayNum(extDomain); j++) {
        dm = Ddi_VarsetarrayRead(extDomain, j);
        if (dm == NULL)
          continue;
        aux = Ddi_VarsetIntersect(dm, edm);
        if (Ddi_VarsetEqual(aux, dm) && (!Ddi_VarsetEqual(edm, dm))) {
          /* dm is included in edm */
          efound = 1;
          Ddi_VarsetarrayClear(extDomain, j);
        } else if (Ddi_VarsetEqual(aux, edm)) {
          /* edm is included in dm */
          eIncl = 1;
        }
        Ddi_Free(aux);
      }
      if (efound || !eIncl) {
        again = 1;
        Ddi_VarsetarrayInsertLast(extDomain, edm);
      }
      if (edm != NULL) {
        nsupp = Ddi_VarsetNum(edm);
        if (maxsupp < nsupp) {
          maxsupp = nsupp;
#if 0
          Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
            Pdtutil_VerbLevelNone_c,
            fprintf(stdout, "NEW EXT DOMAIN (%d).\n", maxsupp)
            );
          Ddi_VarsetPrint(edm, 100, NULL, stdout);
#endif
        }
      }
    }
    Ddi_Free(newExt);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "MAX SUPP: %d/%d.\n", maxsupp, nv)
      );

  } while (again);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "FINAL EXT DOMAIN.\n")
    );

#if 0
  for (j = 0; j < Ddi_VarsetarrayNum(extDomain); j++) {
    dm = Ddi_VarsetarrayRead(extDomain, j);
    if (dm == NULL)
      continue;
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "EXT DOMAIN[%d](%d/%d).\n", j, Ddi_VarsetNum(dm), nv)
      );
    Ddi_VarsetPrint(dm, 100, NULL, stdout);
  }
#endif

#if 0
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "DOMAIN/RANGE.\n")
    );

  for (j = 0; j < np; j++) {
    for (k = 0; k < ndpart; k++) {
      dm = Ddi_VarsetarrayRead(domain[k], j);
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c, fprintf(stdout, "DOMAIN[%d,%d].\n", j, k)
        );
      Ddi_VarsetPrint(dm, 100, NULL, stdout);
    }
    dm = Ddi_VarsetarrayRead(range, j);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "RANGE[%d].\n", j)
      );
    Ddi_VarsetPrint(dm, 100, NULL, stdout);
  }
#endif

  reached = Ddi_BddMakeConst(ddm, 0);
  aux = Ddi_BddSupp(spec);

  for (i = 0; i < Ddi_VarsetarrayNum(extDomain); i++) {
    dm = Ddi_VarsetarrayRead(extDomain, i);
    if (dm == NULL)
      continue;
    term = Ddi_BddMakeConst(ddm, 1);
    for (j = 0; j < nv; j++) {
      p = Ddi_VararrayRead(ps, j);
      p2 = Ddi_VararrayRead(ps2, j);
      n = Ddi_VararrayRead(ns2, j);
      if (Ddi_VarInVarset(dm, p)) {
#if 1
        lit = Ddi_BddMakeLiteral(n, 1);
#else
        lit = Ddi_BddMakeConst(ddm, 1);
#endif
      } else {
        lit = Ddi_BddMakeLiteral(n, 0);
      }
      Ddi_BddAndAcc(term, lit);
      Ddi_Free(lit);
      if (Ddi_VarInVarset(aux, p)) {
        lit = Ddi_BddMakeLiteral(p2, 0);
        Ddi_BddOrAcc(spec, lit);
        Ddi_Free(lit);
      }
    }
    Ddi_BddOrAcc(reached, term);
    Ddi_Free(term);
  }

  Ddi_Free(aux);
  Ddi_Free(extDomain);

#if 0

  psv2 = Ddi_VarsetMakeFromArray(ps2);
  nsv2 = Ddi_VarsetMakeFromArray(ns2);
  rqtr = Ddi_BddMakePartConjVoid(ddm);
  from = Ddi_BddMakeConst(ddm, 0);

  start = Ddi_BddMakeLiteral(n, 1);

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "PROJECTION2 (%d).\n", np)
    );
  for (i = 0; i < np; i++) {
    rq = Ddi_VarsetarrayRead(required, i);
    n = Ddi_VararrayRead(ns2, i);
    term = Ddi_BddMakeLiteral(n, 1);
    termTi = Ddi_BddMakeLiteral(n, 1);
    init = Ddi_BddMakeLiteral(n, 1);
    Ddi_BddAndAcc(start, init);
    for (j = 0; j < np; j++) {
      p = Ddi_VararrayRead(ps2, j);
      if (Ddi_VarInVarset(rq, p)) {
        lit = Ddi_BddMakeLiteral(p, 1);
        Ddi_BddAndAcc(termTi, lit);
        Ddi_BddAndAcc(term, lit);
        Ddi_Free(lit);
      }
#if 1
      if (i != j) {
        n = Ddi_VararrayRead(ns2, j);
        lit = Ddi_BddMakeLiteral(n, 0);
        Ddi_BddAndAcc(init, lit);
        Ddi_Free(lit);
      }
#endif
    }
    Ddi_BddOrAcc(from, init);
    ti = Ddi_BddPartRead(trBdd, i);

    Ddi_BddAndAcc(ti, termTi);

    n = Ddi_VararrayRead(ns2, i);
    lit = Ddi_BddMakeLiteral(n, 0);
#if 1
    Ddi_BddOrAcc(term, lit);
#endif
    Ddi_BddOrAcc(ti, lit);
    Ddi_Free(lit);

#if 0
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "TERM.\n")
      );
    Ddi_BddSuppAttach(term);
    Ddi_VarsetPrint(Ddi_BddSuppRead(term), 100, NULL, stdout);
    Ddi_BddPrintCubes(term, NULL, 100, 0, NULL, stdout);
    Ddi_BddSuppDetach(term);
#endif
    Ddi_BddPartInsertLast(rqtr, term);

    Ddi_Free(term);
    Ddi_Free(termTi);
    Ddi_Free(init);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, ".")
      );
  }

  i = 0;

#if 0
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "DEPENDENCIES TR.\n")
    );
  Ddi_BddSuppAttach(rqtr);
  Ddi_VarsetPrint(Ddi_BddSuppRead(rqtr), 100, NULL, stdout);
  Ddi_BddPrintCubes(rqtr, NULL, 100, 0, NULL, stdout);
  Ddi_BddSuppDetach(rqtr);
#endif
#if 0
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "NEW TR.\n")
    );
  Ddi_BddSuppAttach(trBdd);
  Ddi_VarsetPrint(Ddi_BddSuppRead(trBdd), 100, NULL, stdout);
  Ddi_BddPrintCubes(trBdd, NULL, 100, 0, NULL, stdout);
  Ddi_BddSuppDetach(trBdd);
#endif

#if 0
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "DEPENDENCIES FROM.\n")
    );
  Ddi_VarsetPrint(nsv2, 100, NULL, stdout);
  Ddi_BddPrintCubes(from, nsv2, 100, 0, NULL, stdout);
#endif
#if 0
  reached2 = Ddi_BddSwapVars(from, ps2, ns2);
  while (!Ddi_BddIsZero(from)) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c,
      fprintf(stdout, "EXTENDED DEPENDENCIES LEVEL %d.\n", ++i)
      );
    to = Ddi_BddAndExist(rqtr, from, nsv2);
    LOG_BDD(to, "TO");
    Ddi_Free(from);
    if (Ddi_BddIncluded(to, reached2)) {
      from = Ddi_BddMakeConst(ddm, 0);
    } else {
      from = Ddi_BddDiff(to, reached2);
      Ddi_BddOrAcc(reached2, to);
      Ddi_BddSwapVarsAcc(from, ps2, ns2);
    }
    Ddi_Free(to);
#if 0
    Ddi_BddPrintCubes(from, nsv2, 100, 0, NULL, stdout);
#endif
  }
#endif


#if 0

  for (j = 0; j < np; j++) {
    p = Ddi_VararrayRead(ps2, j);
    aux = Ddi_VarsetMakeFromVar(p);
    lit = Ddi_BddMakeLiteral(p, 0);
    term = Ddi_BddForall(reached2, aux);
    Ddi_BddDiffAcc(reached2, term);
    Ddi_BddAndAcc(term, lit);
    Ddi_BddOrAcc(reached2, term);
    Ddi_Free(term);
    Ddi_Free(aux);
    Ddi_Free(lit);
  }

#endif

#if 0
  Ddi_BddSwapVarsAcc(reached2, ps2, ns2);
#endif

#if 0
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "MINIMAL DEPENDENCIES REACHED.\n")
    );
  if (Ddi_BddIsZero(reached)) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "ZERO.\n")
      );
  }
  Ddi_VarsetPrint(nsv2, 100, NULL, stdout);
  Ddi_BddPrintCubes(reached, nsv2, 100, 0, NULL, stdout);
#endif
#if 0
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    fprintf(stdout, "MINIMAL DEPENDENCIES REACHED2.\n")
    );
  if (Ddi_BddIsZero(reached2)) {
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, "ZERO.\n")
      );
  }
  Ddi_VarsetPrint(nsv2, 100, NULL, stdout);
  Ddi_BddPrintCubes(reached2, nsv2, 100, 0, NULL, stdout);
#endif

#if 0
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "DEPENDENCIES REACHED.\n")
    );
  Ddi_VarsetPrint(psv2, 100, NULL, stdout);
  Ddi_BddPrintCubes(reached, psv2, 100, 0, NULL, stdout);
#endif


  Ddi_Free(from);
#if 0
  Ddi_Free(reached2);
#endif

  Ddi_Free(rqtr);

  Ddi_Free(required);
  Ddi_Free(start);

#endif

  Ddi_Free(psv);
  Ddi_Free(nsv);


#if 0
  psv = Ddi_VarsetMakeFromArray(ps);
  nsv2 = Ddi_VarsetMakeFromArray(ns2);
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "DEPENDENCIES REACHED.\n")
    );
  Ddi_VarsetPrint(psv, 100, NULL, stdout);
  Ddi_BddPrintCubes(reached, nsv2, 100, 0, NULL, stdout);
  Ddi_Free(psv);
  Ddi_Free(nsv2);
#endif

  return (reached);
}







/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Varsetarray_t *
PartFilteredSupp(
  Ddi_Bdd_t * f,
  Ddi_Varset_t * vars
)
{
  Ddi_Varsetarray_t *partSupp;
  Ddi_Varset_t *supp;
  Ddi_Bdd_t *p;
  int np, i;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);

  np = Ddi_BddPartNum(f);

  partSupp = Ddi_VarsetarrayAlloc(ddm, np);

  for (i = 0; i < np; i++) {
    p = Ddi_BddPartRead(f, i);
    supp = Ddi_BddSupp(p);
    Ddi_VarsetIntersectAcc(supp, vars);
    Ddi_VarsetarrayWrite(partSupp, i, supp);
    Ddi_Free(supp);
  }

  return (partSupp);
}



/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
trAddGuards(
  Ddi_Bdd_t * f,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * ns,
  Ddi_Vararray_t * ps2,
  Ddi_Vararray_t * ns2,
  Ddi_Varsetarray_t * dma,
  Ddi_Varsetarray_t * rnga
)
{
  int i, j, np, nv;
  Ddi_Bdd_t *ti, *lit, *termTi, *termN2;
  Ddi_Var_t *p, *n;
  Ddi_Varset_t *rq, *dm, *rng;
  Ddi_Varsetarray_t *required, *produced;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);

  Pdtutil_Assert(Ddi_BddIsPartConj(f), "Conj PART required");

  np = Ddi_BddPartNum(f);
  nv = Ddi_VararrayNum(ps);

  required = Ddi_VarsetarrayAlloc(ddm, np);
  produced = Ddi_VarsetarrayAlloc(ddm, np);
  for (i = 0; i < np; i++) {
    rq = Ddi_VarsetVoid(ddm);
    Ddi_VarsetarrayWrite(required, i, rq);
    rng = Ddi_VarsetSwapVars(Ddi_VarsetarrayRead(rnga, i), ps, ns2);
    Ddi_VarsetarrayWrite(produced, i, rng);
    Ddi_Free(rq);
    Ddi_Free(rng);
  }

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "ADDING GUARDS1 (%d).\n", np)
    );
  for (i = 0; i < np; i++) {
    dm = Ddi_VarsetarrayRead(dma, i);
    rq = Ddi_VarsetSwapVars(dm, ps, ps2);
    Ddi_VarsetarrayWrite(required, i, rq);
    /*Ddi_VarsetPrint(rq,100,NULL,stdout); */
    Ddi_Free(rq);
    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, ".")
      );
  }
  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "")
    );

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "ADDING GUARDS2 (%d).\n", np)
    );

  for (i = 0; i < np; i++) {
    rq = Ddi_VarsetarrayRead(required, i);
    rng = Ddi_VarsetarrayRead(produced, i);
    termN2 = Ddi_BddMakeConst(ddm, 1);
    for (Ddi_VarsetWalkStart(rng);
      !Ddi_VarsetWalkEnd(rng); Ddi_VarsetWalkStep(rng)) {
      n = Ddi_VarsetWalkCurr(rng);
      lit = Ddi_BddMakeLiteral(n, 1);
      Ddi_BddAndAcc(termN2, lit);
      Ddi_Free(lit);
    }
    termTi = Ddi_BddDup(termN2);
    for (j = 0; j < nv; j++) {
      p = Ddi_VararrayRead(ps2, j);
      if (Ddi_VarInVarset(rq, p)) {
        lit = Ddi_BddMakeLiteral(p, 1);
        Ddi_BddAndAcc(termTi, lit);
        Ddi_Free(lit);
      }
    }
    ti = Ddi_BddPartRead(f, i);

#if 0
    if (i == 5) {
      LOG_BDD(ti, "Ti[5]");
    }
#endif

    Ddi_BddAndAcc(ti, termTi);
    Ddi_BddNotAcc(termN2);
    Ddi_BddOrAcc(ti, termN2);
    Ddi_Free(termN2);

    Ddi_Free(termTi);

    Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
      Pdtutil_VerbLevelNone_c, fprintf(stdout, ".")
      );
  }

  Ddi_Free(required);
  Ddi_Free(produced);

  return;
}

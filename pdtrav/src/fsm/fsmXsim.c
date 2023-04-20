/**CFile***********************************************************************

  FileName    [travXsim.c]

  PackageName [trav]

  Synopsis    [Functions managing ternary and symbolic simulation]

  Description [Functions managing ternary and symbolic simulation]

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

#include "fsmInt.h"
#include "ddiInt.h"
#include "baigInt.h"
#include "pdtutilInt.h"

#include "File.h"

#include <time.h>


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

#define fsmMgrO(ddiMgr) ((fsmMgr)->settings.stdout)


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

//static Ddi_Clause_t *eqClassesSplitClauses(Pdtutil_EqClasses_t *eqCl, Ddi_SatSolver_t *solver);
static int xSimRefineEqClasses(
  Fsm_XsimMgr_t * xMgr,
  int mode
);
static int xSimRefineEqClassesBySignatures(
  Fsm_XsimMgr_t * xMgr
);
static Ddi_AigSignatureArray_t *xSimRandomSimulate(
  Fsm_XsimMgr_t * xMgr,
  Ddi_AigSignatureArray_t * psSigVals
);
static Pdtutil_EqClassState_e xSimEqClassUpdateBySignature(
  Pdtutil_EqClasses_t * eqCl,
  Ddi_AigSignatureArray_t * aigSigs,
  int i
);
static int xSimCexSimulate(
  Fsm_XsimMgr_t * xMgr,
  Ddi_SatSolver_t * solver,
  int i0,
  int partialSolver
);
static Ddi_Bddarray_t *xSimInductiveEq(
  Fsm_Mgr_t * fsmMgr,
  Fsm_XsimMgr_t * xMgr,
  int indDepth,
  int indMaxLevel,
  int assumeMiters,
  int *nEqP
);
static void
xSimUseSolver(
  Fsm_XsimMgr_t * xMgr,
  Ddi_SatSolver_t * solver
);
static int
xSimRefineEqClassesWithSolver(
  Fsm_XsimMgr_t * xMgr,
  int i0,
  int mode
);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************
  Synopsis    [ternary evaluation]
  Description [ternary evaluation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Fsm_XsimInductiveEq(
  Fsm_Mgr_t * fsmMgr,
  Fsm_XsimMgr_t * xMgr,
  int indDepth,
  int indMaxLevel,
  int assumeMiters,
  int *nEqP
)
{
  return xSimInductiveEq(fsmMgr, xMgr, indDepth, indMaxLevel, assumeMiters,
    nEqP);
}

/**Function********************************************************************
  Synopsis    [ternary evaluation]
  Description [ternary evaluation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Pdtutil_Array_t *
Fsm_XsimSimulatePsBit(
  Fsm_XsimMgr_t * xMgr,
  int bitId,
  int bitVal,
  int useTarget,
  int *splitDoneP
)
{
  int i;
  int splitDone = 1;
  Pdtutil_Array_t *poBinaryVals;

  if (bitId < 0) {
    return NULL;
  } else {
    int gateId, psVal;
    char Xval;

    Pdtutil_Assert(bitId < xMgr->nPs, "wrong bit val");
    i = bitId;
    gateId = xMgr->ps[i];
    psVal = bitVal;
    Pdtutil_Assert(gateId >= 0, "wrong gate id");
    switch (psVal) {
      case 0:
      case 1:
        Xval = '0' + psVal;
        break;
      case 2:
        Xval = 'x';
        break;
      default:
        Pdtutil_Assert(0, "wrong ternary value in int array");
    }
    Pdtutil_Assert(gateId >= 0
      && gateId < xMgr->nGate, "invalid array reference");
    xMgr->xsimGates[gateId].ternaryVal = Xval;
  }

  for (i = 0; i < xMgr->nGate; i++) {
    char newVal;

    if (xMgr->xsimGates[i].type == Ddi_Const_c) {
      newVal = xMgr->xsimGates[i].ternaryVal = '0';
    } else if (xMgr->xsimGates[i].type == Ddi_And_c) {
      int ir = xMgr->xsimGates[i].fiIds[0];
      int il = xMgr->xsimGates[i].fiIds[1];
      char vr = xMgr->xsimGates[abs(ir) - 1].ternaryVal;
      char vl = xMgr->xsimGates[abs(il) - 1].ternaryVal;

      if (vr != 'x' && ir < 0)
        vr = (vr == '0' ? '1' : '0');
      if (vl != 'x' && il < 0)
        vl = (vl == '0' ? '1' : '0');
      if (vl == '0' || vr == '0') {
        newVal = '0';
      } else if (vl == 'x' || vr == 'x') {
        newVal = 'x';
      } else {
        Pdtutil_Assert(vl == '1' && vr == '1', "wrong and vals");
        newVal = '1';
      }
      xMgr->xsimGates[i].ternaryVal = newVal;
    }

  }

  if (xMgr->strategy > 2) {
    splitDone = xSimRefineEqClasses(xMgr, 0);
  }
  if (splitDoneP != NULL)
    *splitDoneP = splitDone;

  if (useTarget) {
    Pdtutil_Assert(xMgr->target != 0, "target required in xmgr");
    poBinaryVals = Pdtutil_IntegerArrayAlloc(1);
  } else {
    poBinaryVals =
      xMgr->nPo == 0 ? NULL : Pdtutil_IntegerArrayAlloc(xMgr->nPo);
  }

  for (i = 0; i < (useTarget ? 1 : xMgr->nPo); i++) {
    int gate = useTarget ? xMgr->target : xMgr->po[i];
    int gateId = abs(gate) - 1;
    int isCompl = gate < 0;
    int val;

    switch (xMgr->xsimGates[gateId].ternaryVal) {
      case '0':
        val = isCompl ? 1 : 0;
        break;
      case '1':
        val = isCompl ? 0 : 1;
        break;
      case 'x':
        val = 2;
        break;
      default:
        Pdtutil_Assert(0, "wrong ternary value in int array");
    }
    Pdtutil_IntegerArrayInsertLast(poBinaryVals, val);
  }

  return poBinaryVals;

}

/**Function********************************************************************
  Synopsis    [ternary evaluation]
  Description [ternary evaluation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Pdtutil_Array_t *
Fsm_XsimSimulate(
  Fsm_XsimMgr_t * xMgr,
  int restart,
  Pdtutil_Array_t * psBinaryVals,
  int useTarget,
  int *splitDoneP
)
{
  int i, enSim = 0, okX;
  int splitDone = 1;
  Pdtutil_Array_t *poBinaryVals;

  if (restart) {
    for (i = 0; i < xMgr->nGate; i++) {
      xMgr->xsimGates[i].ternaryVal = 'x';
      if (restart > 1 && (xMgr->xsimGates[i].type == Ddi_Pi_c ||
          xMgr->xsimGates[i].type == Ddi_FreePi_c)) {
        xMgr->xsimGates[i].ternaryVal = '0';
      }
    }
  }

  if (psBinaryVals != NULL) {
    Pdtutil_Assert(Pdtutil_IntegerArrayNum(psBinaryVals) == xMgr->nPs,
      "wrong num of ps bin vals");
    for (i = 0; i < xMgr->nPs; i++) {
      int gateId = xMgr->ps[i];
      int psVal = Pdtutil_IntegerArrayRead(psBinaryVals, i);
      char Xval;

      if (gateId < 0)
        continue;
      switch (psVal) {
        case 0:
        case 1:
          Xval = '0' + psVal;
          break;
        case 2:
          Xval = 'x';
          break;
        default:
          Pdtutil_Assert(0, "wrong ternary value in int array");
      }
      Pdtutil_Assert(gateId >= 0
        && gateId < xMgr->nGate, "invalid array reference");
      xMgr->xsimGates[gateId].ternaryVal = Xval;
    }
  }

  for (i = 0; i < xMgr->nGate; i++) {
    char newVal;

    if (xMgr->xsimGates[i].type == Ddi_Const_c) {
      newVal = xMgr->xsimGates[i].ternaryVal = '0';
    } else if (xMgr->xsimGates[i].type == Ddi_And_c) {
      int ir = xMgr->xsimGates[i].fiIds[0];
      int il = xMgr->xsimGates[i].fiIds[1];
      char vr = xMgr->xsimGates[abs(ir) - 1].ternaryVal;
      char vl = xMgr->xsimGates[abs(il) - 1].ternaryVal;

      if (vr != 'x' && ir < 0)
        vr = (vr == '0' ? '1' : '0');
      if (vl != 'x' && il < 0)
        vl = (vl == '0' ? '1' : '0');
      if (vl == '0' || vr == '0') {
        newVal = '0';
      } else if (vl == 'x' || vr == 'x') {
        newVal = 'x';
      } else {
        Pdtutil_Assert(vl == '1' && vr == '1', "wrong and vals");
        newVal = '1';
      }
      xMgr->xsimGates[i].ternaryVal = newVal;
    }

  }

  if (xMgr->strategy > 2) {
    splitDone = xSimRefineEqClasses(xMgr, 0);
  }
  if (splitDoneP != NULL)
    *splitDoneP = splitDone;

  if (useTarget) {
    Pdtutil_Assert(xMgr->target != 0, "target required in xmgr");
    poBinaryVals = Pdtutil_IntegerArrayAlloc(1);
  } else {
    poBinaryVals =
      xMgr->nPo == 0 ? NULL : Pdtutil_IntegerArrayAlloc(xMgr->nPo);
  }

  for (i = 0; i < (useTarget ? 1 : xMgr->nPo); i++) {
    int gate = useTarget ? xMgr->target : xMgr->po[i];
    int gateId = abs(gate) - 1;
    int isCompl = gate < 0;
    int val;

    switch (xMgr->xsimGates[gateId].ternaryVal) {
      case '0':
        val = isCompl ? 1 : 0;
        break;
      case '1':
        val = isCompl ? 0 : 1;
        break;
      case 'x':
        val = 2;
        break;
      default:
        Pdtutil_Assert(0, "wrong ternary value in int array");
    }
    Pdtutil_IntegerArrayInsertLast(poBinaryVals, val);
  }

  return poBinaryVals;

}


/**Function********************************************************************
  Synopsis    [ternary evaluation]
  Description [ternary evaluation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Pdtutil_Array_t *
Fsm_XsimSymbolicSimulate(
  Fsm_XsimMgr_t * xMgr,
  int restart,
  Pdtutil_Array_t * psBinaryVals,
  int useTarget,
  int *splitDoneP
)
{
  Ddi_Mgr_t *ddm = xMgr->ddiMgr;
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  int i, enSim = 0, okX;
  int splitDone = 1;
  Pdtutil_Array_t *poBinaryVals;

  if (restart) {
    for (i = 0; i < xMgr->nGate; i++) {
      xMgr->xsimGates[i].aigVal = bAig_NULL;
    }
  }

  Pdtutil_Assert(xMgr->nPo == xMgr->nPs, "#po and #ps do not match");
  if (psBinaryVals != NULL) {
    Pdtutil_Assert(Pdtutil_IntegerArrayNum(psBinaryVals) == xMgr->nPs,
      "wrong num of ps bin vals");
    for (i = 0; i < xMgr->nPs; i++) {
      int gateId = xMgr->ps[i];
      int psVal = Pdtutil_IntegerArrayRead(psBinaryVals, i);
      bAigEdge_t baigVal;
      int eqId = abs(xMgr->psEq[i]) - 1;
      int eqIsCompl = xMgr->psEq[i] < 0;

      if (gateId < 0)
        continue;
      switch (psVal) {
        case 0:
          baigVal = bAig_Zero;
          break;
        case 1:
          baigVal = bAig_One;
          break;
        case 2:
          if (eqId != i) {
            baigVal = xMgr->xsimGates[xMgr->ps[eqId]].aigRef;
            if (eqIsCompl)
              baigVal = bAig_Not(baigVal);
          } else {
            baigVal = xMgr->xsimGates[gateId].aigRef;
          }
          break;
        default:
          Pdtutil_Assert(0, "wrong ternary value in int array");
      }
      bAig_Ref(bmgr, baigVal);
      xMgr->xsimGates[gateId].aigVal = baigVal;
    }
  }

  for (i = 0; i < xMgr->nGate; i++) {
    bAigEdge_t newVal;

    if (xMgr->xsimGates[i].type == Ddi_Const_c) {
      newVal = bAig_Zero;
      bAig_Ref(bmgr, newVal);
      xMgr->xsimGates[i].aigVal = newVal;
    } else if (xMgr->xsimGates[i].type == Ddi_And_c) {
      int ir = xMgr->xsimGates[i].fiIds[0];
      int il = xMgr->xsimGates[i].fiIds[1];
      bAigEdge_t vr = xMgr->xsimGates[abs(ir) - 1].aigVal;
      bAigEdge_t vl = xMgr->xsimGates[abs(il) - 1].aigVal;

      if (ir < 0)
        vr = bAig_Not(vr);
      if (il < 0)
        vl = bAig_Not(vl);
      newVal = bAig_And(bmgr, vr, vl);
      bAig_Ref(bmgr, newVal);
      xMgr->xsimGates[i].aigVal = newVal;
    } else {
      if (xMgr->xsimGates[i].aigVal == bAig_NULL) {
        newVal = xMgr->xsimGates[i].aigRef;
        bAig_Ref(bmgr, newVal);
        xMgr->xsimGates[i].aigVal = newVal;
      }
    }
  }

  if (xMgr->strategy > 2) {
    Pdtutil_EqClassesSetUndefined(xMgr->eqCl, 0);
    splitDone = xSimRefineEqClasses(xMgr, 1);
    Pdtutil_EqClassesSetUndefined(xMgr->eqCl, 2);
  }
  if (splitDoneP != NULL)
    *splitDoneP = splitDone;

  poBinaryVals = Pdtutil_IntegerArrayAlloc(xMgr->nPo);

  for (i = 0; i < xMgr->nPo; i++) {
    int gateId = abs(xMgr->po[i]) - 1;
    int isCompl = xMgr->po[i] < 0;
    int val;

    xMgr->psEq[i] = i + 1;
    switch (xMgr->xsimGates[gateId].aigVal) {
      case bAig_Zero:
        val = isCompl ? 1 : 0;
        break;
      case bAig_One:
        val = isCompl ? 0 : 1;
        break;
      default:
        val = 2;
        if (!Pdtutil_EqClassIsLdr(xMgr->eqCl, gateId + 1)) {
          int ldrGate = Pdtutil_EqClassLdr(xMgr->eqCl, gateId + 1) - 1;
          int jj, complEq = 0;

          for (jj = 0; jj < i; jj++) {
            int refGateId = abs(xMgr->po[jj]) - 1;

            if (refGateId == ldrGate) {
              complEq =
                isCompl ^ Pdtutil_EqClassIsCompl(xMgr->eqCl, gateId + 1);
              break;
            }
          }
          if (jj < i) {
            xMgr->psEq[i] = complEq ? -(jj + 1) : jj + 1;
          }
        }
        break;
        Pdtutil_Assert(0, "wrong ternary value in int array");
    }
    Pdtutil_IntegerArrayInsertLast(poBinaryVals, val);
  }

  for (i = 0; i < xMgr->nGate; i++) {
    bAig_RecursiveDeref(bmgr, xMgr->xsimGates[i].aigVal);
    xMgr->xsimGates[i].aigVal = bAig_NULL;
  }

  return poBinaryVals;

}

/**Function********************************************************************
  Synopsis    [ternary evaluation]
  Description [ternary evaluation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Fsm_XsimSymbolicSimulateAndRefineEqClasses(
  Fsm_XsimMgr_t * xMgr,
  Ddi_Bddarray_t * psSymbolicVals,
  Ddi_Bdd_t * assume,
  int maxLevel,
  int *splitDoneP
)
{
  Ddi_Mgr_t *ddm = xMgr->ddiMgr;
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  int i, enSim = 0, okX;
  int splitDone = 0;
  Ddi_SatSolver_t *solver = Ddi_SatSolverAlloc();
  bAig_array_t *visitedNodes = bAigArrayAlloc();
  bAig_array_t *visitedNodes2 = bAigArrayAlloc();
  int lastLoaded = 0;
  int maxMerge = 2;

  for (i = 0; i < xMgr->nGate; i++) {
    xMgr->xsimGates[i].aigVal = bAig_NULL;
    if (0 && xMgr->xsimGates[i].type == Fsm_Ps_c) {
      bAigEdge_t baigVal = xMgr->xsimGates[i].aigRef;
      int dummy = DdiAig2CnfId(bmgr, baigVal);
    }
  }

  DdiAig2CnfIdInit(ddm);

  Pdtutil_Assert(xMgr->nPs == 0 || (xMgr->nPo == xMgr->nPs),
    "#po and #ps do not match");
  if (psSymbolicVals != NULL) {
    Ddi_ClauseArray_t *ca;

    Pdtutil_Assert(Ddi_BddarrayNum(psSymbolicVals) == xMgr->nPs,
      "wrong num of ps bin vals");
    for (i = 0; i < xMgr->nPs; i++) {
      int gateId = xMgr->ps[i];
      Ddi_Bdd_t *psVal = Ddi_BddarrayRead(psSymbolicVals, i);
      bAigEdge_t baigVal = Ddi_BddToBaig(psVal);

      if (gateId < 0)
        continue;
      bAig_Ref(bmgr, baigVal);
      xMgr->xsimGates[gateId].aigVal = baigVal;
      DdiPostOrderAigVisitIntern(bmgr, baigVal, visitedNodes, -1);
    }
#if 0
    ca = Ddi_bAigArrayClauses(ddm, visitedNodes, lastLoaded);
    lastLoaded = visitedNodes->num;
    Ddi_SatSolverAddClauses(solver, ca);
    Ddi_ClauseArrayFree(ca);
#endif
  }

  if (assume != NULL) {
    Ddi_ClauseArray_t *aClauses, *ca = Ddi_ClauseArrayAlloc(1);

    if (Ddi_BddIsPartConj(assume)) {
      int j;

      for (j = 0; j < Ddi_BddPartNum(assume); j++) {
        int fCnf;
        bAigEdge_t baigVal = Ddi_BddToBaig(Ddi_BddPartRead(assume, j));

        if (baigVal == bAig_One)
          continue;
        Pdtutil_Assert(baigVal != bAig_Zero, "zero in conj part BDD");
        DdiPostOrderAigVisitIntern(bmgr, baigVal, visitedNodes2, -1);
        fCnf = bAig_NodeIsInverted(baigVal) ? -DdiAig2CnfId(bmgr, baigVal) :
          DdiAig2CnfId(bmgr, baigVal);
        DdiClauseArrayAddClause1(ca, fCnf);
      }
    } else {
      int fCnf;
      bAigEdge_t baigVal = Ddi_BddToBaig(assume);

      if (baigVal != bAig_One) {
        Pdtutil_Assert(baigVal != bAig_Zero, "zero in conj part BDD");
        DdiPostOrderAigVisitIntern(bmgr, baigVal, visitedNodes2, -1);
        fCnf = bAig_NodeIsInverted(baigVal) ? -DdiAig2CnfId(bmgr, baigVal) :
          DdiAig2CnfId(bmgr, baigVal);
        DdiClauseArrayAddClause1(ca, fCnf);
      }
    }
    aClauses = Ddi_bAigArrayClauses(ddm, visitedNodes2, 0);
    Ddi_SatSolverAddClauses(solver, ca);
    Ddi_SatSolverAddClauses(solver, aClauses);
    Ddi_ClauseArrayFree(ca);
    Ddi_ClauseArrayFree(aClauses);
  }

  if (!Pdtutil_EqClassesInitialized(xMgr->eqCl)) {
    Pdtutil_EqClassesSetInitialized(xMgr->eqCl);
    /* solver already loaded */
    int sat = Ddi_SatSolve(solver, NULL, -1);

    if (!sat) {
      Pdtutil_Assert(0, "UNSAT solver");
    }
    splitDone += xSimCexSimulate(xMgr, solver, xMgr->nGate, 1);
    Pdtutil_EqClassesSetInitialized(xMgr->eqCl);
  }

  xSimUseSolver(xMgr,solver);

  for (i = 0; i < xMgr->nGate; i++) {
    bAigEdge_t newVal;
    int level = xMgr->xsimGates[i].level;
    int isPo = xMgr->xsimGates[i].isPo;

    if (xMgr->xsimGates[i].type == Ddi_Const_c) {
      newVal = bAig_Zero;
      bAig_Ref(bmgr, newVal);
      xMgr->xsimGates[i].aigVal = newVal;
    } else if (xMgr->xsimGates[i].type == Ddi_And_c) {
      int ir = xMgr->xsimGates[i].fiIds[0];
      int il = xMgr->xsimGates[i].fiIds[1];
      bAigEdge_t vr = xMgr->xsimGates[abs(ir) - 1].aigVal;
      bAigEdge_t vl = xMgr->xsimGates[abs(il) - 1].aigVal;

      if (ir < 0)
        vr = bAig_Not(vr);
      if (il < 0)
        vl = bAig_Not(vl);
      newVal = bAig_And(bmgr, vr, vl);
      bAig_Ref(bmgr, newVal);
      xMgr->xsimGates[i].aigVal = newVal;
      DdiPostOrderAigVisitIntern(bmgr, newVal, visitedNodes, -1);
    } else if (xMgr->xsimGates[i].aigVal == bAig_NULL) {
      /* free input */
      newVal = xMgr->xsimGates[i].aigRef;
      bAig_Ref(bmgr, newVal);
      xMgr->xsimGates[i].aigVal = newVal;
      DdiPostOrderAigVisitIntern(bmgr, newVal, visitedNodes, -1);
      DdiSolverSetVar(solver, DdiAig2CnfId(bmgr, newVal));
    } else {
      /* already accumulated */
      newVal = xMgr->xsimGates[i].aigVal;
      DdiSolverSetVar(solver, DdiAig2CnfId(bmgr, newVal));
    }

    if (maxLevel > 0 && level > maxLevel) {
      Pdtutil_EqClassSetSingleton(xMgr->eqCl, i + 1);
      continue;
    }

    if (!Pdtutil_EqClassIsLdr(xMgr->eqCl, i + 1)) {
      int equal, constEq = 0;
      bAigEdge_t aigLdr;
      int ldrGate = Pdtutil_EqClassLdr(xMgr->eqCl, i + 1);
      int complEq = Pdtutil_EqClassIsCompl(xMgr->eqCl, i + 1);

      if (ldrGate > 0) {
        /* gate ldr */
        ldrGate--;
        aigLdr = xMgr->xsimGates[ldrGate].aigVal;
      } else {
        /* constant class */
        constEq = 1;
        aigLdr = bAig_Zero;
      }
      if (complEq)
        aigLdr = bAig_Not(aigLdr);

      equal = (aigLdr == newVal);
      if (maxLevel > 0 && level > maxLevel && !equal) {
        splitDone++;
        Pdtutil_EqClassSetSingleton(xMgr->eqCl, i + 1);
      } else if (maxLevel > 0 && level == maxLevel && !equal &&
        maxMerge-- <= 0) {
        splitDone++;
        Pdtutil_EqClassSetSingleton(xMgr->eqCl, i + 1);
      } else if (maxLevel < 0 && isPo && !equal) {
        /* just check POs */
        splitDone++;
        Pdtutil_EqClassSetSingleton(xMgr->eqCl, i + 1);
      } else if (maxLevel < -1 && !equal) {
        /* just check POs */
        splitDone++;
        Pdtutil_EqClassSetSingleton(xMgr->eqCl, i + 1);
      } else if (!equal) {
        /* check using SAT */
        int sat;
        Ddi_Clause_t *assumeDif = NULL;
        Ddi_ClauseArray_t *ca = Ddi_bAigArrayClauses(ddm,
          visitedNodes, lastLoaded);

        lastLoaded = visitedNodes->num;
        Ddi_SatSolverAddClauses(solver, ca);
        Ddi_ClauseArrayFree(ca);

        int fCnf = bAig_NodeIsInverted(newVal) ? -DdiAig2CnfId(bmgr, newVal) :
          DdiAig2CnfId(bmgr, newVal);
        Pdtutil_Assert(abs(fCnf) < solver->nVars, "missing var in solver");
        if (constEq) {
          assumeDif = Ddi_ClauseAlloc(0, 1);
          DdiClauseAddLiteral(assumeDif, fCnf);
          sat =
            Ddi_SatSolveCustomized(solver, assumeDif, aigLdr != bAig_Zero, 0,
            -1);
        } else {
          int eqCnf = bAig_NodeIsInverted(aigLdr) ? -DdiAig2CnfId(bmgr,
            aigLdr) : DdiAig2CnfId(bmgr, aigLdr);

          assumeDif = Ddi_ClauseAlloc(0, 2);
          DdiClauseAddLiteral(assumeDif, fCnf);
          DdiClauseAddLiteral(assumeDif, -eqCnf);
          sat = Ddi_SatSolveCustomized(solver, assumeDif, 0, 0, -1);
          if (!sat) {
            /* complemented assume literals */
            sat = Ddi_SatSolveCustomized(solver, assumeDif, 1, 0, -1);
          }
        }
        if (sat) {
          /* get CEX and simulate ! */
          splitDone += xSimCexSimulate(xMgr, solver, i + 1, 0);
	  //	  splitDone = xSimRefineEqClassesWithSolver(xMgr, 0, 0);
        } else {
          equal = 1;
        }
        Ddi_ClauseFree(assumeDif);
      }
      if (equal) {
        /* merge */
        bAig_Ref(bmgr, aigLdr);
        bAig_RecursiveDeref(bmgr, xMgr->xsimGates[i].aigVal);
        xMgr->xsimGates[i].aigVal = aigLdr;
      }
    }

  }

  DdiPostOrderAigClearVisitedIntern(bmgr, visitedNodes);
  DdiPostOrderAigClearVisitedIntern(bmgr, visitedNodes2);
  DdiAig2CnfIdClose(ddm);

  if (splitDoneP != NULL)
    *splitDoneP = splitDone;

  for (i = 0; i < xMgr->nGate; i++) {
    bAig_RecursiveDeref(bmgr, xMgr->xsimGates[i].aigVal);
    xMgr->xsimGates[i].aigVal = bAig_NULL;
  }

  bAigArrayFree(visitedNodes);
  bAigArrayFree(visitedNodes2);
  Ddi_SatSolverQuit(solver);

  return NULL;

}

/**Function********************************************************************
  Synopsis    [parallel pattern evaluation]
  Description [parallel pattern evaluation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Fsm_XsimRandomSimulateSeq(
  Fsm_XsimMgr_t * xMgr,
  Ddi_Bddarray_t * initStub,
  Ddi_Bdd_t * init,
  int nTimeFrames
)
{
  Ddi_Mgr_t *ddm = xMgr->ddiMgr;
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(xMgr->fsmMgr);
  int i;
  int splitDone = 1;
  Ddi_AigSignatureArray_t *initSig = NULL, *psSigVals = NULL;

  Pdtutil_Assert(initStub != NULL
    || init != NULL, "init stub or init required");

  if (initStub != NULL) {

    initSig = DdiAigArrayEvalSignatureRandomPi(initStub);

  } else {
    Ddi_Bddarray_t *initArray = Ddi_BddarrayMakeLiteralsAig(ps, 1);

    Ddi_AigarrayConstrainCubeAcc(initArray, init);
    initSig = DdiAigArrayEvalSignatureRandomPi(initArray);
    Ddi_Free(initArray);
  }

  psSigVals = initSig;

  for (i = 0; i < nTimeFrames; i++) {

    Ddi_AigSignatureArray_t *nsSigVals = xSimRandomSimulate(xMgr, psSigVals);

    DdiAigSignatureArrayFree(psSigVals);
    psSigVals = nsSigVals;

  }

  Pdtutil_EqClassesSetInitialized(xMgr->eqCl);

  DdiAigSignatureArrayFree(psSigVals);
  //  Fsm_XsimPrintEqClasses(xMgr);

}


/**Function********************************************************************
  Synopsis    [ternary evaluation]
  Description [ternary evaluation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Fsm_XsimSymbolicMergeEq(
  Fsm_XsimMgr_t * xMgr,
  Ddi_Bdd_t * miters,
  Ddi_Bdd_t * invarspec
)
{
  Ddi_Mgr_t *ddm = xMgr->ddiMgr;
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  int i, enSim = 0, okX;
  int evDriven = 1, nEq=0;
  Pdtutil_Array_t *poBinaryVals;
  Ddi_Bddarray_t *fA;

  if (miters != NULL) {
    Ddi_BddSetPartConj(miters);
  }

  for (i = 0; i < xMgr->nGate; i++) {
    xMgr->xsimGates[i].aigVal = bAig_NULL;
  }

  Pdtutil_Assert(xMgr->nPs == 0 || (xMgr->nPo == xMgr->nPs),
    "#po and #ps do not match");
  for (i = 0; i < xMgr->nGate; i++) {
    bAigEdge_t newVal, eqVal = bAig_NULL;
    int isEq = 0;

    if (!Pdtutil_EqClassIsLdr(xMgr->eqCl, i + 1)) {
      int ldrGate = Pdtutil_EqClassLdr(xMgr->eqCl, i + 1);

      if (ldrGate == 0) {
        eqVal = bAig_Zero;
      } else {
        eqVal = xMgr->xsimGates[ldrGate - 1].aigVal;
      }
      if (Pdtutil_EqClassIsCompl(xMgr->eqCl, i + 1)) {
        eqVal = bAig_Not(eqVal);
      }
      bAig_Ref(bmgr, eqVal);
      xMgr->xsimGates[i].aigVal = eqVal;
      isEq = 1; nEq++;
    }
    if (!isEq || miters != NULL) {
      if (xMgr->xsimGates[i].type == Ddi_Const_c) {
        newVal = bAig_Zero;
        bAig_Ref(bmgr, newVal);
      } else if (xMgr->xsimGates[i].type == Ddi_And_c) {
        int ir = xMgr->xsimGates[i].fiIds[0];
        int il = xMgr->xsimGates[i].fiIds[1];
        bAigEdge_t vr = xMgr->xsimGates[abs(ir) - 1].aigVal;
        bAigEdge_t vl = xMgr->xsimGates[abs(il) - 1].aigVal;

        if (ir < 0)
          vr = bAig_Not(vr);
        if (il < 0)
          vl = bAig_Not(vl);
        newVal = bAig_And(bmgr, vr, vl);
        bAig_Ref(bmgr, newVal);
      } else {
        newVal = xMgr->xsimGates[i].aigRef;
        bAig_Ref(bmgr, newVal);
      }
      if (isEq) {
        bAigEdge_t miterBaig;
        Ddi_Bdd_t *miterBdd;

        Pdtutil_Assert(miters != NULL, "miters required");
        miterBaig = bAig_Not(bAig_Xor(bmgr, newVal, eqVal));
        miterBdd = Ddi_BddMakeFromBaig(ddm, miterBaig);
        bAig_RecursiveDeref(bmgr, newVal);
        Ddi_BddPartInsertLast(miters, miterBdd);
        Ddi_Free(miterBdd);
      } else {
        xMgr->xsimGates[i].aigVal = newVal;
      }
    }
  }

  fA = Ddi_BddarrayAlloc(ddm, xMgr->nPo);

  for (i = 0; i < xMgr->nPo; i++) {
    int gateId = abs(xMgr->po[i]) - 1;
    int isCompl = xMgr->po[i] < 0;
    Ddi_Bdd_t *f_i = Ddi_BddMakeFromBaig(ddm, xMgr->xsimGates[gateId].aigVal);

    if (isCompl)
      Ddi_BddNotAcc(f_i);
    Ddi_BddarrayWrite(fA, i, f_i);
    Ddi_Free(f_i);
  }

  if (invarspec != NULL) {
    Pdtutil_Assert(xMgr->target != 0, "target required in xmgr");
    int gateId = abs(xMgr->target) - 1;
    int isCompl = xMgr->target < 0;
    Ddi_Bdd_t *myI = Ddi_BddMakeFromBaig(ddm, xMgr->xsimGates[gateId].aigVal);

    if (!isCompl)
      Ddi_BddNotAcc(myI);       // target is complemented invarspec
    Ddi_DataCopy(invarspec, myI);
    Ddi_Free(myI);
  }

  for (i = 0; i < xMgr->nGate; i++) {
    bAig_RecursiveDeref(bmgr, xMgr->xsimGates[i].aigVal);
    xMgr->xsimGates[i].aigVal = bAig_NULL;
  }

  return fA;

}


/**Function********************************************************************

  Synopsis    [Inner steps of aig to BDD conversion]
  Description [Inner steps of aig to BDD conversion]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Fsm_XsimMgr_t *
Fsm_XsimInit(
  Fsm_Mgr_t * fsmMgr,
  Ddi_Bddarray_t * fA,
  Ddi_Bdd_t * invarspec,
  Ddi_Vararray_t * ps,
  Ddi_Vararray_t * maxLevelVars,
  int strategy
)
{
  Ddi_Mgr_t *ddm = fA==NULL ? Ddi_ReadMgr(invarspec) : Ddi_ReadMgr(fA);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  Fsm_XsimMgr_t *xMgr;
  bAig_array_t *visitedNodes;
  int i;
  Ddi_Varset_t *piVars, *psVars = NULL;
  Ddi_Vararray_t *pi = NULL;
  int setPiLevelMax = 0;

  psVars = ps == NULL ? Ddi_VarsetVoid(ddm) : Ddi_VarsetMakeFromArray(ps);
  piVars = fA == NULL ? Ddi_VarsetVoid(ddm) : Ddi_BddarraySupp(fA);
  if (invarspec != NULL) {
    Ddi_Varset_t *supp = Ddi_BddSupp(invarspec);

    Ddi_VarsetUnionAcc(piVars, supp);
    Ddi_Free(supp);
  }

  Ddi_VarsetDiffAcc(piVars, psVars);
  pi = Ddi_VararrayMakeFromVarset(piVars, 1);

  Ddi_Free(psVars);
  Ddi_Free(piVars);

  xMgr = Pdtutil_Alloc(Fsm_XsimMgr_t, 1);
  xMgr->strategy = strategy;
  xMgr->fsmMgr = fsmMgr;
  xMgr->ddiMgr = ddm;
  xMgr->solver = NULL;
  xMgr->nPo = fA == NULL ? 0 : Ddi_BddarrayNum(fA);
  xMgr->nPi = Ddi_VararrayNum(pi);
  xMgr->nPs = Ddi_VararrayNum(ps);

  xMgr->gateFilter = NULL;
  xMgr->nFiltered = 0;

  visitedNodes = bAigArrayAlloc();
  xMgr->eqCl = NULL;

  for (i = 0; i < xMgr->nPo; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA, i);
    bAigEdge_t nodeIndex = Ddi_BddToBaig(f_i);

    DdiPostOrderAigVisitIntern(bmgr, nodeIndex, visitedNodes, -1);
  }
  if (invarspec != NULL) {
    bAigEdge_t nodeIndex = Ddi_BddToBaig(invarspec);

    DdiPostOrderAigVisitIntern(bmgr, nodeIndex, visitedNodes, -1);
  }
  DdiPostOrderAigClearVisitedIntern(bmgr, visitedNodes);
  DdiAigArraySortByLevel(ddm, visitedNodes, bAig_NULL, -1);

  xMgr->nGate = visitedNodes->num;
  xMgr->xsimGates = Pdtutil_Alloc(xsimGateInfo, xMgr->nGate);
  xMgr->gateSigs = NULL;
  xMgr->po = Pdtutil_Alloc(int,
    xMgr->nPo
  );
  xMgr->pi = Pdtutil_Alloc(int,
    xMgr->nPi
  );
  xMgr->ps = xMgr->nPs == 0 ? NULL : Pdtutil_Alloc(int,
    xMgr->nPs
  );
  xMgr->psEq = xMgr->nPs == 0 ? NULL : Pdtutil_Alloc(int,
    xMgr->nPs
  );

  xMgr->target = 0;
  xMgr->nLevels = 0;

  if (strategy > 2) {
    xMgr->eqCl = Pdtutil_EqClassesAlloc(xMgr->nGate + 1);
    /* all menbers unassigned and classes not active */
    Pdtutil_EqClassesReset(xMgr->eqCl);
    /* define constant class */
    Pdtutil_EqClassSetLdr(xMgr->eqCl, 0, 0);
    /* set X value */
    Pdtutil_EqClassesSetUndefined(xMgr->eqCl, 2);
    if (strategy > 4) {
      xMgr->gateSigs = DdiAigSignatureArrayAlloc(xMgr->nGate + 1);
    }
  }

  for (i = 0; i < visitedNodes->num; i++) {
    bAigEdge_t baig = bAig_NonInvertedEdge(visitedNodes->nodes[i]);

    bAig_AuxInt(bmgr, baig) = i;
    xMgr->xsimGates[i].aigRef = baig;
    xMgr->xsimGates[i].level = 0;
    xMgr->xsimGates[i].filterId = -1;
    xMgr->xsimGates[i].isPo = 0;
    if (bAig_NodeIsConstant(baig)) {
      xMgr->xsimGates[i].type = Fsm_Const_c;
      /* non complemented constant is 0 */
      xMgr->xsimGates[i].ternaryVal = '0';
    } else if (bAig_isVarNode(bmgr, baig)) {
      xMgr->xsimGates[i].type = Fsm_FreePi_c;
    } else {
      int levelR, levelL;
      bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr, baig);
      bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(bmgr, baig);
      int ir = bAig_AuxInt(bmgr, right);
      int il = bAig_AuxInt(bmgr, left);

      xMgr->xsimGates[i].type = Fsm_And_c;
      Pdtutil_Assert(ir >= 0 && il >= 0, "Invalid aux int");
      xMgr->xsimGates[i].fiIds[0] =
        bAig_NodeIsInverted(right) ? -(ir + 1) : (ir + 1);
      xMgr->xsimGates[i].fiIds[1] =
        bAig_NodeIsInverted(left) ? -(il + 1) : (il + 1);
      levelR = xMgr->xsimGates[ir].level;
      levelL = xMgr->xsimGates[il].level;
      xMgr->xsimGates[i].level = (levelR < levelL) ? levelL + 1 : levelR + 1;
      if (xMgr->xsimGates[i].level >= xMgr->nLevels) {
        xMgr->nLevels = xMgr->xsimGates[i].level + 1;
      }
    }
  }

  for (i = 0; i < xMgr->nPi; i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(pi, i);
    bAigEdge_t varIndex = Ddi_VarBaigId(v);
    int id = bAig_AuxInt(bmgr, varIndex);

    xMgr->pi[i] = id;
    if (id < 0)
      continue;
    Pdtutil_Assert(id >= 0 && id <= xMgr->nGate, "pi error in ternary init");
    Pdtutil_Assert(xMgr->xsimGates[id].type == Fsm_FreePi_c,
      "pi error in ternary init");
    xMgr->xsimGates[id].type = Fsm_Pi_c;
  }

  for (i = 0; i < xMgr->nPs; i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(ps, i);
    bAigEdge_t varIndex = Ddi_VarBaigId(v);
    int id = bAig_AuxInt(bmgr, varIndex);

    xMgr->ps[i] = id;
    xMgr->psEq[i] = i + 1;
    if (id < 0)
      continue;
    Pdtutil_Assert(id >= 0 && id <= xMgr->nGate, "pi error in ternary init");
    Pdtutil_Assert(xMgr->xsimGates[id].type == Fsm_FreePi_c,
      "pi error in ternary init");
    xMgr->xsimGates[id].type = Fsm_Ps_c;
  }

  if (maxLevelVars != NULL) {
    for (i = 0; i < Ddi_VararrayNum(maxLevelVars); i++) {
      Ddi_Var_t *v = Ddi_VararrayRead(maxLevelVars, i);
      bAigEdge_t varIndex = Ddi_VarBaigId(v);
      int id = bAig_AuxInt(bmgr, varIndex);

      if (id < 0)
        continue;
      Pdtutil_Assert(id >= 0 && id <= xMgr->nGate, "pi error in ternary init");
      Pdtutil_Assert(xMgr->xsimGates[id].type == Fsm_Pi_c ||
        xMgr->xsimGates[id].type == Fsm_Ps_c, "pi error in ternary init");
      xMgr->xsimGates[id].level = 1000;
    }
    // recompute levels
    for (i = 0; i < xMgr->nGate; i++) {
      if (xMgr->xsimGates[i].type == Ddi_And_c) {
        int levelR, levelL;
        int ir = xMgr->xsimGates[i].fiIds[0];
        int il = xMgr->xsimGates[i].fiIds[1];

        levelR = xMgr->xsimGates[abs(ir) - 1].level;
        levelL = xMgr->xsimGates[abs(il) - 1].level;
        xMgr->xsimGates[i].level = (levelR < levelL) ? levelL + 1 : levelR + 1;
      }
    }
  }

  for (i = 0; i < xMgr->nPo; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA, i);
    bAigEdge_t nodeIndex = Ddi_BddToBaig(f_i);
    int id = bAig_AuxInt(bmgr, nodeIndex);

    Pdtutil_Assert(id >= 0 && id < xMgr->nGate, "pi error in ternary init");
    xMgr->po[i] = bAig_NodeIsInverted(nodeIndex) ? -(id + 1) : (id + 1);
    xMgr->xsimGates[id].isPo = 1;
  }

  if (invarspec != NULL) {
    bAigEdge_t nodeIndex = Ddi_BddToBaig(invarspec);
    int id = bAig_AuxInt(bmgr, nodeIndex);

    Pdtutil_Assert(id >= 0 && id < xMgr->nGate, "pi error in ternary init");
    /* target is complemented w.r.t. invarspec */
    xMgr->target = bAig_NodeIsInverted(nodeIndex) ? (id + 1) : -(id + 1);
  }

  /* reset auxInt field */
  for (i = 0; i < visitedNodes->num; i++) {
    bAigEdge_t baig = visitedNodes->nodes[i];

    bAig_AuxInt(bmgr, baig) = -1;
  }

  bAigArrayFree(visitedNodes);
  Ddi_Free(pi);

  return (xMgr);
}

/**Function********************************************************************
  Synopsis    [Inner steps of aig to BDD conversion]
  Description [Inner steps of aig to BDD conversion]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Fsm_XsimQuit(
  Fsm_XsimMgr_t * xMgr
)
{
  int i;

  Pdtutil_Free(xMgr->xsimGates);

  Pdtutil_Free(xMgr->po);
  Pdtutil_Free(xMgr->pi);
  if (xMgr->ps != NULL) {
    Pdtutil_Free(xMgr->ps);
  }
  if (xMgr->psEq != NULL) {
    Pdtutil_Free(xMgr->psEq);
  }
  if (xMgr->eqCl != NULL) {
    Pdtutil_EqClassesFree(xMgr->eqCl);
  }
  if (xMgr->gateSigs != NULL) {
    DdiAigSignatureArrayFree(xMgr->gateSigs);
  }
  if (xMgr->gateFilter != NULL) {
    Pdtutil_Free(xMgr->gateFilter);
  }

  Pdtutil_Free(xMgr);
}

/**Function********************************************************************
  Synopsis    [Inner steps of aig to BDD conversion]
  Description [Inner steps of aig to BDD conversion]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Fsm_XsimSetGateFilter(
  Fsm_XsimMgr_t * xMgr,
  int maxLevel
)
{
  int i, nFiltered = 0;

  if (maxLevel < 0)
    return maxLevel;
  if (xMgr->gateFilter != NULL) {
    Pdtutil_Free(xMgr->gateFilter);
  }
  Pdtutil_Assert(maxLevel >= 0, "max level is negative");
  for (i = 0; i < xMgr->nGate; i++) {
    if (xMgr->xsimGates[i].level <= maxLevel) {
      nFiltered++;
    }
  }
  xMgr->gateFilter = Pdtutil_Alloc(int,
    nFiltered
  );

  nFiltered = 0;
  for (i = 0; i < xMgr->nGate; i++) {
    xMgr->xsimGates[i].filterId = nFiltered;
    if (xMgr->xsimGates[i].level <= maxLevel) {
      xMgr->gateFilter[nFiltered++] = i;
    }
  }
  xMgr->nFiltered = nFiltered;
  return (nFiltered);

}

/**Function********************************************************************
  Synopsis    [Inner steps of aig to BDD conversion]
  Description [Inner steps of aig to BDD conversion]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Fsm_XsimPrintEqClasses(
  Fsm_XsimMgr_t * xMgr
)
{
  int i;

  printf("\nOUTPUT EQ (trivial):\n");
  for (i = 0; i < xMgr->nPo; i++) {
    int j;

    for (j = 0; j < i; j++) {
      if (xMgr->po[i] == xMgr->po[j]) {
        printf("EQ: %d <- %d\n", j, i);
        break;
      }
    }
    int gate = xMgr->po[i];
  }
  printf("OUTPUT EQ (simul):\n");
  for (i = 0; i < xMgr->nPo; i++) {
    int j, ldrGate, gate = abs(xMgr->po[i]) - 1;

    if (Pdtutil_EqClassLdr(xMgr->eqCl, gate + 1) != (gate + 1)) {
      /* find leader */
      int ldrGate = Pdtutil_EqClassLdr(xMgr->eqCl, gate + 1) - 1;

      for (j = 0; j < xMgr->nPo; j++) {
        if (xMgr->po[i] == ldrGate && j != i) {
          printf("EQ: %d <- %d - gates: %d <- %d\n", i, j, ldrGate, gate);
          break;
        }
      }
    }
  }
  printf("GATE EQ (simul):\n");
  for (i = 1; i <= xMgr->nGate; i++) {
    if (Pdtutil_EqClassLdr(xMgr->eqCl, i) != i) {
      int gate = i - 1;
      int ldrGate = Pdtutil_EqClassLdr(xMgr->eqCl, i);
      int isCompl = Pdtutil_EqClassIsCompl(xMgr->eqCl, i);

      if (ldrGate == 0) {
        printf("EQ: gates: C <- %s%d(%d)\n", isCompl ? "-" : "", gate,
          xMgr->xsimGates[gate].aigRef);
      } else {
        printf("EQ: gates: %d(%d) <- %s%d(%d)\n", ldrGate - 1,
          xMgr->xsimGates[ldrGate - 1].aigRef, isCompl ? "-" : "",
          gate, xMgr->xsimGates[gate].aigRef);
      }
    }
  }

}


/**Function********************************************************************
  Synopsis    [Inner steps of aig to BDD conversion]
  Description [Inner steps of aig to BDD conversion]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Fsm_XsimCountEquiv(
  Fsm_XsimMgr_t * xMgr
)
{
  int i, n = 0;

  for (i = 1; i <= xMgr->nGate; i++) {
    if (Pdtutil_EqClassLdr(xMgr->eqCl, i) != i) {
      n++;
    }
  }
  return n;
}


/**Function********************************************************************
  Synopsis    [Inner steps of aig to BDD conversion]
  Description [Inner steps of aig to BDD conversion]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Fsm_XsimCheckEqClasses(
  Fsm_XsimMgr_t * xMgr
)
{
  int i;

  for (i = 1; i <= xMgr->nGate; i++) {
    if (Pdtutil_EqClassLdr(xMgr->eqCl, i) != i) {
      int gate = i - 1;
      int ldrGate = Pdtutil_EqClassLdr(xMgr->eqCl, i);
      int val_i = Pdtutil_EqClassVal(xMgr->eqCl, i);
      int val_ldr = Pdtutil_EqClassVal(xMgr->eqCl, ldrGate);

      if (xMgr->gateSigs != NULL) {
        Ddi_AigSignature_t *ldrSig =
          Ddi_AigSignatureArrayRead(xMgr->gateSigs, ldrGate);
        Ddi_AigSignature_t *gateSig =
          Ddi_AigSignatureArrayRead(xMgr->gateSigs, i);
        Pdtutil_Assert(DdiEqSignatures(gateSig, ldrSig, NULL) ||
          DdiEqComplSignatures(gateSig, ldrSig, NULL), "wrong equiv");
      } else if (ldrGate == 0) {
        Pdtutil_Assert(val_i / 2 == 0, "wrong const equiv");
        Pdtutil_Assert(val_i == Pdtutil_EqClassIsCompl(xMgr->eqCl, i),
          "wrong const equiv");
      } else {
        Pdtutil_Assert(val_i / 2 == val_ldr / 2, "wrong equiv");
        Pdtutil_Assert((val_i % 2 ^ val_ldr % 2) ==
          Pdtutil_EqClassIsCompl(xMgr->eqCl, i), "wrong equiv");
      }
    }
  }

}



/**Function********************************************************************
  Synopsis    [ternary evaluation]
  Description [ternary evaluation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Fsm_XsimTraverse(
  Fsm_Mgr_t * fsmMgr,
  int enTernary,
  int strategy,
  int *pProved,
  int timeLimit
)
{
  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  Ddi_Vararray_t *ns = Fsm_MgrReadVarNS(fsmMgr);
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Bddarray_t *lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
  Ddi_Bddarray_t *initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
  Ddi_Bdd_t *init = Fsm_MgrReadInitBDD(fsmMgr);
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(lambda);
  Fsm_XsimMgr_t *xMgr;

  int j, fixPoint, nEq, i, n = 0;
  int nState = Ddi_VararrayNum(ns);
  Ddi_Bdd_t *from;
  Ddi_Varset_t *ternaryVars;
  Ddi_Varset_t *nsv = Ddi_VarsetMakeFromArray(ns);
  Ddi_Vararray_t *deltaVars = Ddi_BddarraySuppVararray(delta);
  Pdtutil_Array_t *fromArray;
  Ddi_Bdd_t *reached;
  int initStubInReached = 0, proved = 0;
  unsigned long int time_limit = ~0;
  long initTime, endTime;
  int maxIter = 100;
  int enFp=1;

  if (timeLimit > 0) {
    /* actually NOT used */
    time_limit = util_cpu_time() + timeLimit * 1000;
  }

  xMgr = Fsm_XsimInit(fsmMgr, delta, NULL, ps, NULL, strategy);

  if (initStub != NULL) {
    reached = Ddi_BddMakeConstAig(ddiMgr, initStubInReached);
    fromArray = Pdtutil_IntegerArrayAlloc(nState);
    for (j = 0; j < nState; j++) {
      Ddi_Bdd_t *f_j = Ddi_BddarrayRead(initStub, j);
      int val = 2;

      if (Ddi_BddIsConstant(f_j)) {
        val = Ddi_BddIsOne(f_j) ? 1 : 0;
        if (initStubInReached) {
          Ddi_Var_t *v_j = Ddi_VararrayRead(ps, j);
          Ddi_Bdd_t *lit_j = Ddi_BddMakeLiteralAig(v_j, val);

          Ddi_BddAndAcc(reached, lit_j);
          Ddi_Free(lit_j);
        }
      }
      Pdtutil_IntegerArrayInsertLast(fromArray, val);
    }
  } else {
    fromArray = Ddi_AigCubeMakeIntegerArrayFromBdd(init, ps);
    reached = Ddi_BddDup(init);
  }

  //    Ddi_BddSetMono(reached);
  Ddi_BddSetPartDisj(reached);

  fixPoint = 0;
  initTime = util_cpu_time();

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
    Pdtutil_VerbLevelNone_c, fprintf(stdout, "Ternary Simulation: "));

  for (j = 0; j < maxIter && !fixPoint; j++) {
    int k, eqSplit;
    Ddi_Bdd_t *to;
    Pdtutil_Array_t *toArray;

    if (enTernary > 2) {
      toArray = Fsm_XsimSymbolicSimulate(xMgr, 1, fromArray, 0, &eqSplit);
    } else {
      toArray = Fsm_XsimSimulate(xMgr, 1, fromArray, 0, &eqSplit);
    }

    to = Ddi_AigCubeMakeBddFromIntegerArray(toArray, ps);
    //      Ddi_BddSetMono(to);
    //      logBdd(to);
    fixPoint = 0;
    if (enFp)
    for (k = Ddi_BddPartNum(reached) - 1; !eqSplit && k >= 0; k--) {
      Ddi_Bdd_t *r_k = Ddi_BddPartRead(reached, k);

      if (Ddi_BddIncluded(to, r_k)) {
        fixPoint = 1;
        break;
      }
    }
    //      fixPoint = Ddi_BddIncluded(to,reached);
    Pdtutil_IntegerArrayFree(fromArray);
    fromArray = toArray;

    Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
      fprintf(stdout, "(%d)", Ddi_BddSize(to)));

    Ddi_BddPartInsertLast(reached, to);
    Ddi_Free(to);
  }
  Pdtutil_IntegerArrayFree(fromArray);

  if (fixPoint) {

    nEq = Fsm_XsimCountEquiv(xMgr);
    if (nEq > 0) {
      Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "TSIM found %d EQUIVALENCES\n", nEq));
    }
    //    Fsm_XsimPrintEqClasses(xMgr);
    if (1 || enTernary > 4) {
      Ddi_Bddarray_t *newD = Fsm_XsimSymbolicMergeEq(xMgr, NULL, NULL);

      if (newD != NULL) {
        Ddi_Bdd_t *invar =
          Ddi_BddDup(Ddi_BddarrayRead(delta, Ddi_BddarrayNum(delta) - 2));
        Ddi_Bdd_t *prop = Ddi_BddarrayRead(newD, Ddi_BddarrayNum(delta) - 1);

        Ddi_BddarrayWrite(newD, Ddi_BddarrayNum(delta) - 2, invar);
        Ddi_BddNotAcc(invar);
        //      Ddi_BddOrAcc(prop,invar);
        Ddi_Free(invar);
        Ddi_DataCopy(delta, newD);
        Fsm_FsmStructReduction(fsmMgr, NULL);   /* missing clock info */
        Ddi_Free(newD);

        pi = Fsm_MgrReadVarI(fsmMgr);
        ps = Fsm_MgrReadVarPS(fsmMgr);
        ns = Fsm_MgrReadVarNS(fsmMgr);
        delta = Fsm_MgrReadDeltaBDD(fsmMgr);
        lambda = Fsm_MgrReadLambdaBDD(fsmMgr);
        initStub = Fsm_MgrReadInitStubBDD(fsmMgr);
        init = Fsm_MgrReadInitBDD(fsmMgr);
        n = Ddi_BddarrayNum(delta);

      }
    }
  }

  Fsm_XsimQuit(xMgr);

  //    Ddi_BddSetMono(reached);

  Ddi_Free(nsv);
  //    Ddi_Free(myTr);
  Ddi_Free(deltaVars);
  endTime = util_cpu_time() - initTime;

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c, fprintf(stdout, "\n")
    );

  Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Ternary Simulation Performed: ");
    fprintf(stdout, "%d iterations in %.2f s; |R|=%d.\n",
      j, endTime / 1000.0, Ddi_BddSize(reached))
    );

  if (!fixPoint && n > 0) {
    for (i = n - 1; i >= 0; i--) {
      Ddi_Var_t *pv_i = Ddi_VararrayRead(ps, i);

      if (strcmp(Ddi_VarName(pv_i), "PDT_BDD_INVARSPEC_VAR$PS") == 0) {
        Ddi_Bdd_t *rAig = Ddi_BddMakeAig(reached);
        Ddi_Bdd_t *lit = Ddi_BddMakeLiteralAig(pv_i, 0);

        if (!Ddi_AigSatAnd(rAig, lit, NULL)) {
          Pdtutil_VerbosityMgr(ddiMgr, Pdtutil_VerbLevelUsrMin_c,
            fprintf(stdout,
              "Verification Result: PASS (by ternary simulation).\n"));
          proved = 1;
        }
        Ddi_Free(lit);
        Ddi_Free(rAig);
        break;
      }
    }
  }

  if (pProved != NULL)
    *pProved = proved;

  return reached;

}



/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [parallel pattern evaluation]
  Description [parallel pattern evaluation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_AigSignatureArray_t *
xSimRandomSimulate(
  Fsm_XsimMgr_t * xMgr,
  Ddi_AigSignatureArray_t * psSigVals
)
{
  Ddi_Mgr_t *ddm = xMgr->ddiMgr;
  Ddi_AigSignatureArray_t *nsSigVals;
  int i;

  Pdtutil_Assert(Ddi_AigSignatureArrayNum(psSigVals) == xMgr->nPs,
    "wrong num of ps bin vals");
  for (i = 0; i < xMgr->nPs; i++) {
    int gateId = xMgr->ps[i];
    Ddi_AigSignature_t *psSig = Ddi_AigSignatureArrayRead(psSigVals, i);
    Ddi_AigSignature_t *gateSig =
      Ddi_AigSignatureArrayRead(xMgr->gateSigs, gateId + 1);
    DdiSignatureCopy(gateSig, psSig);
  }

  for (i = 0; i < xMgr->nGate; i++) {
    Ddi_AigSignature_t *gateSig =
      Ddi_AigSignatureArrayRead(xMgr->gateSigs, i + 1);
    if (xMgr->xsimGates[i].type == Ddi_Const_c) {
      DdiSetSignatureConstant(gateSig, 0);
    } else if (xMgr->xsimGates[i].type == Ddi_And_c) {
      int ir = xMgr->xsimGates[i].fiIds[0];
      int il = xMgr->xsimGates[i].fiIds[1];
      Ddi_AigSignature_t *rSig =
        Ddi_AigSignatureArrayRead(xMgr->gateSigs, abs(ir) /*-1*/ );
      Ddi_AigSignature_t *lSig =
        Ddi_AigSignatureArrayRead(xMgr->gateSigs, abs(il) /*-1*/ );
      DdiEvalSignature(gateSig, rSig, lSig, ir < 0, il < 0);
    } else if (xMgr->xsimGates[i].type == Ddi_Ps_c) {
      /* ps: already assigned */
    } else {
      /* pi: generate random signature */
      Pdtutil_Assert(xMgr->xsimGates[i].type == Ddi_Pi_c, "wrong garte code");
      DdiSetSignatureRandom(gateSig);
    }
  }

  xSimRefineEqClassesBySignatures(xMgr);

  nsSigVals = DdiAigSignatureArrayAlloc(xMgr->nPo);

  for (i = 0; i < xMgr->nPo; i++) {
    int gateId = abs(xMgr->po[i]) - 1;
    int isCompl = xMgr->po[i] < 0;
    Ddi_AigSignature_t *nsSig = Ddi_AigSignatureArrayRead(nsSigVals, i);
    Ddi_AigSignature_t *gateSig =
      Ddi_AigSignatureArrayRead(xMgr->gateSigs, gateId + 1);
    DdiSignatureCopy(nsSig, gateSig);
    if (isCompl)
      DdiComplementSignature(nsSig);
  }

  return nsSigVals;

}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
xSimCexSimulate(
  Fsm_XsimMgr_t * xMgr,
  Ddi_SatSolver_t * solver,
  int i0,
  int partialSolver
)
{
  Ddi_Mgr_t *ddm = xMgr->ddiMgr;
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  int j, i, j0, maxN;
  int splitDone = 0;

  Pdtutil_Assert(i0 > 0 && i0 <= xMgr->nGate, "wrong cex sim index");
  for (i = 0; i < i0; i++) {
    bAigEdge_t baigVal = xMgr->xsimGates[i].aigVal;
    lbool valSolver;
    char val;
    int fCnf = 0;

    if (baigVal == bAig_NULL) {
      if (xMgr->xsimGates[i].type != Ddi_And_c) {
        baigVal = xMgr->xsimGates[i].aigRef;
      }
    }
    if (baigVal == bAig_NULL) {
      valSolver = l_Undef;
    } else {
      fCnf = bAig_NodeIsInverted(baigVal) ? -DdiAig2CnfId(bmgr, baigVal) :
        DdiAig2CnfId(bmgr, baigVal);
      int v = abs(fCnf);

      valSolver = DdiSolverReadModel(solver, v);
    }
    if (xMgr->xsimGates[i].type == Ddi_Const_c) {
      xMgr->xsimGates[i].ternaryVal = '0';
      continue;
    } else if (bAig_NodeIsConstant(baigVal)) {
      xMgr->xsimGates[i].ternaryVal = baigVal == bAig_Zero ? '0' : '1';
      continue;
    }
    if (valSolver == l_Undef) {
      Pdtutil_Assert(partialSolver, "undef var in Sat cex");
      val = 'x';
    } else if (valSolver == l_True) {
      val = (fCnf < 0) ? '0' : '1';
    } else if (valSolver == l_False) {
      val = (fCnf < 0) ? '1' : '0';
    }

    xMgr->xsimGates[i].ternaryVal = val;

  }

  if (partialSolver) {
    /* repeat sim */
    i0 = 0;
  }
  if (xMgr->gateFilter != NULL && i0 < xMgr->nGate) {
    maxN = xMgr->nFiltered;
    if (i0 == xMgr->nGate) {
      j0 = maxN;
    } else {
      j0 = xMgr->xsimGates[i0].filterId;
      Pdtutil_Assert(j0 >= 0, "filter id problem");
    }
  } else {
    j0 = i0;
    maxN = xMgr->nGate;
  }

  for (j = j0; j < maxN; j++) {
    char newVal;

    if (xMgr->gateFilter != NULL) {
      i = xMgr->gateFilter[j];
      Pdtutil_Assert(i >= 0 && i < xMgr->nGate, "wrong gate array index");
    } else {
      i = j;
    }
    if (xMgr->xsimGates[i].type == Ddi_Const_c) {
      newVal = xMgr->xsimGates[i].ternaryVal = '0';
    } else if (xMgr->xsimGates[i].type == Ddi_And_c) {
      int ir = xMgr->xsimGates[i].fiIds[0];
      int il = xMgr->xsimGates[i].fiIds[1];
      char vr = xMgr->xsimGates[abs(ir) - 1].ternaryVal;
      char vl = xMgr->xsimGates[abs(il) - 1].ternaryVal;

      if (vr != 'x' && ir < 0)
        vr = (vr == '0' ? '1' : '0');
      if (vl != 'x' && il < 0)
        vl = (vl == '0' ? '1' : '0');
      if (vl == '0' || vr == '0') {
        newVal = '0';
      } else if (vl == 'x' || vr == 'x') {
        newVal = 'x';
      } else {
        Pdtutil_Assert(vl == '1' && vr == '1', "wrong and vals");
        newVal = '1';
      }
      xMgr->xsimGates[i].ternaryVal = newVal;
    } else if (1 && xMgr->xsimGates[i].type == Ddi_Ps_c) {
      bAigEdge_t baigVal = xMgr->xsimGates[i].aigVal;
      lbool valSolver;
      char val;
      int fCnf = 0;

      if (baigVal == bAig_NULL) {
        xMgr->xsimGates[i].ternaryVal = '1';
      } else {
        fCnf = bAig_NodeIsInverted(baigVal) ? -DdiAig2CnfId(bmgr, baigVal) :
          DdiAig2CnfId(bmgr, baigVal);
        int v = abs(fCnf);

        valSolver = DdiSolverReadModel(solver, v);
        if (valSolver == l_Undef) {
          //          Pdtutil_Assert(partialSolver,"undef var in Sat cex");
          val = '1';
        } else if (valSolver == l_True) {
          val = (fCnf < 0) ? '0' : '1';
        } else if (valSolver == l_False) {
          val = (fCnf < 0) ? '1' : '0';
        }
        xMgr->xsimGates[i].ternaryVal = val;
      }
    } else {
      if (partialSolver) {
        /* leave non x values */
        if (xMgr->xsimGates[i].ternaryVal == 'x') {
          xMgr->xsimGates[i].ternaryVal = '1';
        }
      } else {
        /* set other inputs randomly: 1 */
        xMgr->xsimGates[i].ternaryVal = '1';
      }
    }
  }

  splitDone = xSimRefineEqClasses(xMgr, 0);

  return splitDone;

}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
xSimUseSolver(
  Fsm_XsimMgr_t * xMgr,
  Ddi_SatSolver_t * solver
)
{
  Ddi_Mgr_t *ddm = xMgr->ddiMgr;
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  int j, i, maxN;
  int splitDone = 0;

  if (xMgr->gateFilter != NULL) {
    maxN = xMgr->nFiltered;
  } else {
    maxN = xMgr->nGate;
  }

  xMgr->solver = solver;

  for (j = 0; j < maxN; j++) {
    char newVal;
    bAigEdge_t baigVal = xMgr->xsimGates[j].aigRef;
    int fCnf = 0;

    if (xMgr->gateFilter != NULL) {
      i = xMgr->gateFilter[j];
      Pdtutil_Assert(i >= 0 && i < xMgr->nGate, "wrong gate array index");
    } else {
      i = j;
    }
    if (xMgr->xsimGates[i].type != Ddi_Const_c) {
      fCnf = bAig_NodeIsInverted(baigVal) ? -DdiAig2CnfId(bmgr, baigVal) :
	DdiAig2CnfId(bmgr, baigVal);
    } 
    Pdtutil_EqClassCnfVarWrite(xMgr->eqCl,i+1,fCnf);
  }

}

/**Function********************************************************************
  Synopsis    [parallel pattern evaluation]
  Description [parallel pattern evaluation]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bddarray_t *
xSimInductiveEq(
  Fsm_Mgr_t * fsmMgr,
  Fsm_XsimMgr_t * xMgr,
  int indDepth,
  int indMaxLevel,
  int assumeMiters,
  int *nEqP
)
{
  Ddi_Mgr_t *ddm = xMgr->ddiMgr;
  Ddi_AigSignatureArray_t *nsSigVals;
  Ddi_Bddarray_t *delta = Fsm_MgrReadDeltaBDD(fsmMgr);
  Ddi_Vararray_t *pi = Fsm_MgrReadVarI(fsmMgr);
  Ddi_Vararray_t *ps = Fsm_MgrReadVarPS(fsmMgr);
  int i, refined;
  int checkEq = 0;
  static int nCalls = 0;

  do {
    int j;
    Ddi_Bdd_t *miters = assumeMiters ? Ddi_BddMakePartConjVoid(ddm) : NULL;
    Ddi_Bddarray_t *unroll,
      *deltaMerged = Fsm_XsimSymbolicMergeEq(xMgr, miters, NULL);
    Ddi_Bdd_t *assume = NULL;
    char suffix[20];

    refined = 0;

    unroll = Ddi_BddarrayMakeLiteralsAig(ps, 1);
    for (j = 0; j < indDepth; j++) {
      sprintf(suffix, "%d", j);
      Ddi_Vararray_t *newPiVars =
        Ddi_VararrayMakeNewVars(pi, "PDT_ITP_INITFWD_PI", suffix, 1);
      Ddi_Bddarray_t *newPiLits = Ddi_BddarrayMakeLiteralsAig(newPiVars, 1);
      Ddi_Bddarray_t *newDelta = Ddi_BddarrayDup(deltaMerged);

      Ddi_BddarrayComposeAcc(newDelta, pi, newPiLits);
      Ddi_BddarrayComposeAcc(unroll, ps, newDelta);
      if (assumeMiters && Ddi_BddPartNum(miters) > 0) {
        Ddi_Bdd_t *myAssume = Ddi_BddDup(miters);

        Ddi_BddComposeAcc(myAssume, pi, newPiLits);
        if (j == 0) {
          assume = Ddi_BddDup(myAssume);
        } else {
          int j;

          Ddi_BddComposeAcc(assume, ps, newDelta);
          for (j = 0; j < Ddi_BddPartNum(myAssume); j++) {
            Ddi_BddPartInsertLast(assume, Ddi_BddPartRead(myAssume, j));
          }
        }
        Ddi_Free(myAssume);
      }
      Ddi_Free(newPiVars);
      Ddi_Free(newPiLits);
      Ddi_Free(newDelta);
    }
    Ddi_Free(deltaMerged);

    if (assume)
      Ddi_BddSetAig(assume);
    Fsm_XsimSymbolicSimulateAndRefineEqClasses(xMgr,
      unroll, assume, indMaxLevel, &refined);
    nCalls++;

    Ddi_Free(miters);

    printf("EQ refinements: %d\n", refined);
    if (!refined && checkEq && Fsm_XsimCountEquiv(xMgr) > 0) {
      Ddi_Bddarray_t *newD = Fsm_XsimSymbolicMergeEq(xMgr, NULL, NULL);
      Ddi_Bddarray_t *oldD = Ddi_BddarrayDup(delta);

      Ddi_BddarrayComposeAcc(newD, ps, unroll);
      Ddi_BddarrayComposeAcc(oldD, ps, unroll);
      if (assume)
        Ddi_BddSetAig(assume);
      for (i = 0; i < Ddi_BddarrayNum(newD); i++) {
        Ddi_Bdd_t *n_i = Ddi_BddarrayRead(newD, i);
        Ddi_Bdd_t *o_i = Ddi_BddarrayRead(oldD, i);

        if (!Ddi_AigEqualSatWithCare(n_i, o_i, assume)) {
          Ddi_Bddarray_t *myD = Fsm_XsimSymbolicMergeEq(xMgr, NULL, NULL);

          n_i = Ddi_BddarrayRead(myD, i);
          o_i = Ddi_BddarrayRead(delta, i);
          Pdtutil_Assert(0, "wrong ind EQ merge");
        }
      }
      Ddi_Free(newD);
      Ddi_Free(oldD);
    }

    Ddi_Free(assume);

    Ddi_Free(unroll);

  } while (refined);

  if (nEqP != NULL) {
    *nEqP = Fsm_XsimCountEquiv(xMgr);
  }


  return NULL;
}

/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
xSimRefineEqClasses(
  Fsm_XsimMgr_t * xMgr,
  int mode
)
{
  Pdtutil_EqClasses_t *eqCl = xMgr->eqCl;
  int i, splitNum;
  static int nCalls = 0;

  nCalls++;
  Pdtutil_Assert(eqCl != NULL, "missing eq cl manager");

  splitNum = Pdtutil_EqClassesSplitNum(eqCl);

  for (i = 0; i < xMgr->nGate; i++) {
    int val;

    if (mode == 0) {
      /* ternary */
      switch (xMgr->xsimGates[i].ternaryVal) {
      case '0':
      case '1':
	val = xMgr->xsimGates[i].ternaryVal - '0';
	break;
      case 'x':
	val = 2;
	break;
      }
    } else {
      /* symbolic */
      switch (xMgr->xsimGates[i].aigVal) {
      case bAig_Zero:
	val = 0;
	break;
      case bAig_One:
	val = 1;
	break;
      default:
	val = (int)xMgr->xsimGates[i].aigVal;
	break;
      }
    }
    Pdtutil_EqClassSetVal(eqCl, i + 1, val);
  }

  if (xMgr->gateFilter != NULL) {
    for (i = 0; i < xMgr->nFiltered; i++) {
      int j = xMgr->gateFilter[i];

      Pdtutil_EqClassUpdate(eqCl, j + 1);
    }
  } else {
    for (i = 1; i <= xMgr->nGate; i++) {
      Pdtutil_EqClassUpdate(eqCl, i);
    }
  }

  if (!Pdtutil_EqClassesInitialized(xMgr->eqCl)) {
    Pdtutil_EqClassesSetInitialized(xMgr->eqCl);
  }

  Pdtutil_EqClassesSplitLdrReset(eqCl);

  //  Fsm_XsimCheckEqClasses(xMgr); 

  splitNum = Pdtutil_EqClassesSplitNum(eqCl) - splitNum;



  return (splitNum > 0);

}

/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
xSimRefineEqClassesWithSolver(
  Fsm_XsimMgr_t * xMgr,
  int i0,
  int mode
)
{
  Pdtutil_EqClasses_t *eqCl = xMgr->eqCl;
  int i, splitNum, max;
  static int nCalls = 0;

  nCalls++;
  Pdtutil_Assert(eqCl != NULL, "missing eq cl manager");

  splitNum = Pdtutil_EqClassesSplitNum(eqCl);

  Pdtutil_Assert (xMgr->solver != NULL, "Missing solver");

  max = xMgr->nGate;
  if (xMgr->gateFilter != NULL) {
    max = xMgr->nFiltered;
  }

  for (i = i0; i < max; i++) {
    int j=i;
    if (xMgr->gateFilter != NULL) {
      j = xMgr->gateFilter[i];
    }
    int v = Pdtutil_EqClassCnfVarRead(eqCl,j+1);
    int val;
    lbool valSolver = DdiSolverReadModel(xMgr->solver, v);
    if (valSolver == l_Undef) {
      val = 2;
    } else if (valSolver == l_True) {
      val = 1;
    } else if (valSolver == l_False) {
      val = 0;
    }
    Pdtutil_EqClassSetVal(eqCl, j + 1, val);
  }
  for (i = i0; i < max; i++) {
    int j=i;
    if (xMgr->gateFilter != NULL) {
      j = xMgr->gateFilter[i];
    }
    Pdtutil_EqClassUpdate(eqCl, j + 1);
  }

  if (!Pdtutil_EqClassesInitialized(xMgr->eqCl)) {
    Pdtutil_EqClassesSetInitialized(xMgr->eqCl);
  }

  Pdtutil_EqClassesSplitLdrReset(eqCl);

  //  Fsm_XsimCheckEqClasses(xMgr); 

  splitNum = Pdtutil_EqClassesSplitNum(eqCl) - splitNum;



  return (splitNum > 0);

}


/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
xSimRefineEqClassesBySignatures(
  Fsm_XsimMgr_t * xMgr
)
{
  Pdtutil_EqClasses_t *eqCl = xMgr->eqCl;
  int i, splitNum;
  static int nCalls = 0;
  Ddi_AigSignature_t *gateSig0 = Ddi_AigSignatureArrayRead(xMgr->gateSigs, 0);

  DdiSetSignatureConstant(gateSig0, 0);

  nCalls++;
  Pdtutil_Assert(eqCl != NULL, "missing eq cl manager");

  splitNum = Pdtutil_EqClassesSplitNum(eqCl);

  for (i = 1; i <= xMgr->nGate; i++) {
    xSimEqClassUpdateBySignature(eqCl, xMgr->gateSigs, i);
  }

  Pdtutil_EqClassesSplitLdrReset(eqCl);

  //  Fsm_XsimCheckEqClasses(xMgr); 

  splitNum = Pdtutil_EqClassesSplitNum(eqCl) - splitNum;



  return (splitNum > 0);

}


/**Function********************************************************************
  Synopsis    [Update classes based on new element values]
  Description [Update classes based on new element values]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Pdtutil_EqClassState_e
xSimEqClassUpdateBySignature(
  Pdtutil_EqClasses_t * eqCl,
  Ddi_AigSignatureArray_t * aigSigs,
  int i
)
{
  int j;

  eqCl->activeClass[i] = 0;
  eqCl->refineSteps++;

  if (eqCl->state[i] == Pdtutil_EqClass_Singleton_c) {
    /* do nothing */
  } else if (eqCl->eqClass[i] == i) {
    /* class leader */
    eqCl->activeClass[i] = 1;   /* this is a class: possibly unchanged */
    eqCl->splitLdr[i] = -1;
    Pdtutil_Assert(eqCl->state[i] == Pdtutil_EqClass_Ldr_c,
      "wrong class state");
    return Pdtutil_EqClass_Ldr_c;
  } else if (eqCl->eqClass[i] == -1) {
    /* eval signature */
    Ddi_AigSignature_t *sig_i = Ddi_AigSignatureArrayRead(aigSigs, i);

    for (j = 0; j < i; j++) {
      if (eqCl->eqClass[j] == j) {
        Ddi_AigSignature_t *sig_j = Ddi_AigSignatureArrayRead(aigSigs, j);
        int eq;

        if ((eq = DdiEqSignatures(sig_i, sig_j, NULL)) ||
          DdiEqComplSignatures(sig_i, sig_j, NULL)) {
          eqCl->eqClass[i] = j;
          eqCl->eqClassNum[j]++;
          eqCl->complEq[i] = !eq;
          eqCl->activeClass[j] = 2;
          eqCl->splitDone++;
          Pdtutil_Assert(eqCl->state[i] == Pdtutil_EqClass_Unassigned_c,
            "wrong class state");
          eqCl->state[i] = Pdtutil_EqClass_Member_c;
          return Pdtutil_EqClass_JoinSplit_c;
        }
      }
    }
    /* do not join - become ldr */
    eqCl->eqClass[i] = i;
    eqCl->eqClassNum[i] = 1;
    eqCl->complEq[i] = 0;
    eqCl->activeClass[i] = 2;
    Pdtutil_Assert(eqCl->state[i] == Pdtutil_EqClass_Unassigned_c,
      "wrong class state");
    eqCl->state[i] = Pdtutil_EqClass_Ldr_c;
    return Pdtutil_EqClass_SplitLdr_c;
  } else {
    /* class member: get leader */
    int isCompl = eqCl->complEq[i];
    int ldr_i = eqCl->eqClass[i];
    int singletonLdr = eqCl->state[ldr_i] == Pdtutil_EqClass_Singleton_c;
    Ddi_AigSignature_t *sig_i = Ddi_AigSignatureArrayRead(aigSigs, i);
    Ddi_AigSignature_t *sig_ldr_i = Ddi_AigSignatureArrayRead(aigSigs, ldr_i);

    Pdtutil_Assert(eqCl->state[i] == Pdtutil_EqClass_Member_c,
      "wrong class state");

    int isEqual = isCompl ? DdiEqComplSignatures(sig_i, sig_ldr_i, NULL) :
      DdiEqSignatures(sig_i, sig_ldr_i, NULL);

    if (!isEqual) {
      /* split */
      eqCl->splitDone++;
      eqCl->eqClassNum[ldr_i]--;
      Pdtutil_Assert(eqCl->eqClassNum[ldr_i] > 0, "neg class num");
      if (eqCl->splitLdr[ldr_i] < 0) {
        /* this is a new ldr */
        eqCl->eqClassNum[i]++;
        eqCl->splitLdr[ldr_i] = i;
        eqCl->eqClass[i] = i;
        //  eqCl->complEq[i] = 0; /* new ldr: reset compl flag */
        eqCl->activeClass[i] = eqCl->activeClass[ldr_i] = 2;
        eqCl->state[i] = Pdtutil_EqClass_Ldr_c;
        return Pdtutil_EqClass_SplitLdr_c;
      } else {
        /* join existing new ldr
           join can be temporary - can be speculated and require
           recursive split */
        int testRecur = 1;

        Pdtutil_Assert(eqCl->splitLdr[ldr_i] < i, "illegal cl leader");
        ldr_i = eqCl->splitLdr[ldr_i];
        eqCl->eqClassNum[ldr_i]++;
        eqCl->eqClass[i] = ldr_i;
        eqCl->complEq[i] = eqCl->complEq[i] != eqCl->complEq[ldr_i];
        if (testRecur) {
          int doRecur;

          sig_ldr_i = Ddi_AigSignatureArrayRead(aigSigs, ldr_i);

          isEqual = isCompl ? DdiEqComplSignatures(sig_i, sig_ldr_i, NULL) :
            DdiEqSignatures(sig_i, sig_ldr_i, NULL);
          if (eqCl->complEq[i]) {
            doRecur = !DdiEqComplSignatures(sig_i, sig_ldr_i, NULL);
          } else {
            doRecur = !DdiEqSignatures(sig_i, sig_ldr_i, NULL);
          }
          if (doRecur) {
            return xSimEqClassUpdateBySignature(eqCl, aigSigs, i);
          }
        } else {
          Pdtutil_Assert(eqCl->complEq[i] ==
            DdiEqComplSignatures(sig_i, sig_ldr_i, NULL),
            "error setting split compl flags");
        }
        return Pdtutil_EqClass_JoinSplit_c;
      }
    } else {
      /* keep in class */
      return Pdtutil_EqClass_SameClass_c;
    }
  }

}

/**CFile***********************************************************************

  FileName    [ddiSim.c]

  PackageName [ddi]

  Synopsis    [simulation functions and data structures]

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
#include "cluster.h"
#include "data.h"

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

static unsigned char byteCountOnes[256];
int byteCountSet = 0;
static unsigned int nAllocatedBitmaps = 0;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#define dMgrO(ddiMgr) ((ddiMgr)->settings.stdout)

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static Ddi_SccMgr_t *sccMgrAlloc(Ddi_Bddarray_t *delta, Ddi_Vararray_t *ps);
static void sccMgrFree(Ddi_SccMgr_t *sccMgr);
static int fsmSccTarjanIntern (Ddi_SccMgr_t *sccMgr, bAigEdge_t nodeIndex);
static void postOrderAigVisitInternWithScc(bAig_Manager_t *manager, bAigEdge_t nodeIndex, bAig_array_t *visitedNodes);
static int *BddarrayClusterByCoiAcc(Ddi_SccMgr_t *sccMgr, Ddi_Bddarray_t *fA, int min, int max, int th_coi_diff, int th_coi_share/*default disabled*/, int methodCluster/*default cluster by factor*/,int doSort);
static int fsmSccTarjanCheckBridgeIntern (Ddi_SccMgr_t *sccMgr,bAigEdge_t nodeIndex);
static Ddi_AigDynSignatureArray_t *BddarrayClusterProperties(  Ddi_SccMgr_t *sccMgr, Ddi_Bddarray_t *fA, int *clusterMap);
static int findIte(bAig_Manager_t *bmgr, bAigEdge_t baig, int gateId);
static int computeLevel(bAig_Manager_t *bmgr, bAigEdge_t baig);
static void logScc(Ddi_SccMgr_t *sccMgr, int iScc);

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_AigSignatureArrayNum(
  Ddi_AigSignatureArray_t *sig
)
{
  return sig->nSigs;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_AigSignature_t *
Ddi_AigSignatureArrayRead(
  Ddi_AigSignatureArray_t *sig,
  int i
)
{
  return &(sig->sArray[i]);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_AigSignatureArray_t *
DdiAigSignatureArrayAlloc(
  int num
)
{
  Ddi_AigSignatureArray_t *sA;

  sA = Pdtutil_Alloc(Ddi_AigSignatureArray_t,1);
  sA->sArray = Pdtutil_Alloc(Ddi_AigSignature_t,num);
  sA->currBit = 0;
  sA->nSigs = num;
  return (sA);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiAigSignatureArrayClear(
  Ddi_AigSignatureArray_t *sA
)
{
  int i, num = sA->nSigs;

  for (i=0; i<num; i++) {
    DdiSetSignatureConstant(&sA->sArray[i],0);
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_AigSignatureArray_t *
DdiAigSignatureArrayDup(
  Ddi_AigSignatureArray_t *sA
)
{
  Ddi_AigSignatureArray_t *sA2;
  int i, num = sA->nSigs;

  sA2 = Pdtutil_Alloc(Ddi_AigSignatureArray_t,1);
  sA2->sArray = Pdtutil_Alloc(Ddi_AigSignature_t,num);
  sA2->currBit = 0;
  sA2->nSigs = num;
  for (i=0; i<num; i++) {
    sA2->sArray[i] = sA->sArray[i];
  }

  return (sA2);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiAigSignatureArrayFree(
  Ddi_AigSignatureArray_t *sA
)
{
  if (sA != NULL) {
    Pdtutil_Free(sA->sArray);
    Pdtutil_Free(sA);
  }
}

/**Function********************************************************************
  Synopsis    [Optimizations for exist cofactors]
  Description [Optimizations for exist cofactors]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_AigSignatureArray_t *
DdiAigEvalSignature
(
  Ddi_Mgr_t *ddm,
  bAig_array_t *aigNodes,
  bAigEdge_t constBaig,
  int constVal,
  Ddi_AigSignatureArray_t *varSig
)
{
  bAig_Manager_t *manager = ddm->aig.mgr;
  int i, il, ir;
  bAigEdge_t right, left;
  Ddi_AigSignatureArray_t *sig;
  int nVars = Ddi_MgrReadNumVars(ddm);

  sig = DdiAigSignatureArrayAlloc(aigNodes->num);
  for (i=0; i<aigNodes->num; i++) {
    bAigEdge_t baig = aigNodes->nodes[i];
    Pdtutil_Assert(bAig_AuxInt(manager,baig) == -1, "wrong auxId field");
    bAig_AuxInt(manager,baig) = i;

    if (bAig_NodeIsConstant(baig)) {
      DdiSetSignatureConstant(&sig->sArray[i],0);
    }
    else if (bAig_NonInvertedEdge(baig)==bAig_NonInvertedEdge(constBaig)) {
      DdiSetSignatureConstant(&sig->sArray[i],constVal);
    }
    else if (bAig_isVarNode(manager,baig)) {
      /* read val */
      Ddi_Var_t *v = Ddi_VarFromBaig(ddm,baig);
      int varId = Ddi_VarIndex(v);
      sig->sArray[i] = varSig->sArray[varId];
    }
    else {
      right = bAig_NodeReadIndexOfRightChild(manager,baig);
      left = bAig_NodeReadIndexOfLeftChild(manager,baig);
      ir = bAig_AuxInt(manager,right);
#if 0
      if (ir < 0) {
	int j;
	for (j=0; j<aigNodes->num; j++) {
          bAigEdge_t baig_j = aigNodes->nodes[j];
          if (bAig_NonInvertedEdge(right)==bAig_NonInvertedEdge(baig_j)) {
	    fprintf(dMgrO(ddm),"%d\n",j);
	  }
	}
	printf("error\n");
      }
#endif
      il = bAig_AuxInt(manager,left);
      DdiEvalSignature(&sig->sArray[i],&sig->sArray[ir],&sig->sArray[il],
        bAig_NodeIsInverted(right),bAig_NodeIsInverted(left));
    }
  }

  for (i=0; i<aigNodes->num; i++) {
    bAigEdge_t baig = aigNodes->nodes[i];
    Pdtutil_Assert(bAig_AuxInt(manager,baig) >=0, "wrong auxId field");
    bAig_AuxInt(manager,baig) = -1;
  }

  return(sig);
}


/**Function********************************************************************
  Synopsis    [Optimizations for exist cofactors]
  Description [Optimizations for exist cofactors]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_AigSignatureArray_t *
DdiAigArrayEvalSignatureRandomPi
(
  Ddi_Bddarray_t *fA
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(fA);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  int i, il, ir;
  bAigEdge_t right, left;
  Ddi_AigSignatureArray_t *sig, *outSig;
  bAig_array_t *aigNodes = bAigArrayAlloc();

  int nRoots = Ddi_BddarrayNum(fA);

  for (i=0; i<nRoots; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    bAigEdge_t fBaig = Ddi_BddToBaig(f_i);

    if (!bAig_NodeIsConstant(fBaig)) {
      DdiPostOrderAigVisitIntern(bmgr,fBaig,aigNodes,-1);
    }
  }
  DdiPostOrderAigClearVisitedIntern(bmgr,aigNodes);

  sig = DdiAigSignatureArrayAlloc(aigNodes->num);
  outSig = DdiAigSignatureArrayAlloc(nRoots);

  for (i=0; i<aigNodes->num; i++) {
    bAigEdge_t baig = aigNodes->nodes[i];
    Pdtutil_Assert(bAig_AuxInt(bmgr,baig) == -1, "wrong auxId field");
    bAig_AuxInt(bmgr,baig) = i;

    if (bAig_NodeIsConstant(baig)) {
      DdiSetSignatureConstant(&sig->sArray[i],0);
    }
    else if (bAig_isVarNode(bmgr,baig)) {
      /* read val */
      DdiSetSignatureRandom(&sig->sArray[i]);
    }
    else {
      right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
      left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);
      ir = bAig_AuxInt(bmgr,right);
      il = bAig_AuxInt(bmgr,left);
      DdiEvalSignature(&sig->sArray[i],&sig->sArray[ir],&sig->sArray[il],
        bAig_NodeIsInverted(right),bAig_NodeIsInverted(left));
    }
  }

  for (i=0; i<nRoots; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    int isCompl = Ddi_BddIsComplement(f_i);
    bAigEdge_t fBaig = Ddi_BddToBaig(f_i);
    if (bAig_NodeIsConstant(fBaig)) {
      DdiSetSignatureConstant(&outSig->sArray[i],0);
    }
    else {
      int j = bAig_AuxInt(bmgr,fBaig);
      outSig->sArray[i] = sig->sArray[j];
    }
    if (isCompl) DdiComplementSignature(&outSig->sArray[i]);
  }

  for (i=0; i<aigNodes->num; i++) {
    bAigEdge_t baig = aigNodes->nodes[i];
    Pdtutil_Assert(bAig_AuxInt(bmgr,baig) >=0, "wrong auxId field");
    bAig_AuxInt(bmgr,baig) = -1;
  }

  bAigArrayFree(aigNodes);
  DdiAigSignatureArrayFree(sig);

  return(outSig);
}

/**Function********************************************************************
  Synopsis    [Optimizations for exist cofactors]
  Description [Optimizations for exist cofactors]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_AigSignatureArray_t *
DdiAigArrayEvalSignature
(
  Ddi_Bddarray_t *fA,
  Ddi_AigSignatureArray_t *varSigs,
  float randFlipRatio
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(fA);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  int i, il, ir;
  bAigEdge_t right, left;
  Ddi_AigSignatureArray_t *sig, *outSig;
  bAig_array_t *aigNodes = bAigArrayAlloc();
  Ddi_AigSignatureArray_t *varSigs2 = DdiAigSignatureArrayDup(varSigs);

  int nRoots = Ddi_BddarrayNum(fA);

  for (i=0; i<nRoots; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    bAigEdge_t fBaig = Ddi_BddToBaig(f_i);

    if (!bAig_NodeIsConstant(fBaig)) {
      DdiPostOrderAigVisitIntern(bmgr,fBaig,aigNodes,-1);
    }
  }
  DdiPostOrderAigClearVisitedIntern(bmgr,aigNodes);

  sig = DdiAigSignatureArrayAlloc(aigNodes->num);

  sig = DdiAigEvalSignature(ddm, aigNodes, bAig_NULL, 0, varSigs2);

  outSig = DdiAigSignatureArrayAlloc(nRoots);

  for (i=0; i<aigNodes->num; i++) {
    bAigEdge_t baig = aigNodes->nodes[i];
    Pdtutil_Assert(bAig_AuxInt(bmgr,baig) == -1, "wrong auxId field");
    bAig_AuxInt(bmgr,baig) = i;
  }

  for (i=0; i<nRoots; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    int isCompl = Ddi_BddIsComplement(f_i);
    bAigEdge_t fBaig = Ddi_BddToBaig(f_i);
    if (bAig_NodeIsConstant(fBaig)) {
      DdiSetSignatureConstant(&outSig->sArray[i],0);
    }
    else {
      int j = bAig_AuxInt(bmgr,fBaig);
      outSig->sArray[i] = sig->sArray[j];
    }
    if (isCompl) DdiComplementSignature(&outSig->sArray[i]);
  }

  for (i=0; i<aigNodes->num; i++) {
    bAigEdge_t baig = aigNodes->nodes[i];
    Pdtutil_Assert(bAig_AuxInt(bmgr,baig) >=0, "wrong auxId field");
    bAig_AuxInt(bmgr,baig) = -1;
  }

  bAigArrayFree(aigNodes);
  DdiAigSignatureArrayFree(sig);
  DdiAigSignatureArrayFree(varSigs2);

  return(outSig);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiSetSignatureConstant
(
  Ddi_AigSignature_t *sig,
  int val
)
{
  int i;
  unsigned long uval = 0;
  if (val) {
    uval = ~uval;
  }
  for (i=0;i<DDI_AIG_SIGNATURE_SLOTS;i++) {
    sig->s[i] = uval;
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiSignatureCopy
(
  Ddi_AigSignature_t *dst,
  Ddi_AigSignature_t *src
)
{
  Pdtutil_Assert(dst!=NULL && src!=NULL,"src & dst required by sig copy");
  *dst = *src;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
DdiGetSignatureBit
(
  Ddi_AigSignature_t *sig,
  int bit
)
{
  int iBit, iSlot;

  iSlot = bit / (sizeof(unsigned long) * 8);
  iBit = bit % (sizeof(unsigned long) * 8);

  return (sig->s[iSlot] & (1<<iBit)) >> iBit;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiSetSignatureBit
(
  Ddi_AigSignature_t *sig,
  int bit,
  int val
)
{
  int iBit, iSlot;

  iSlot = bit / (sizeof(unsigned long) * 8);
  iBit = bit % (sizeof(unsigned long) * 8);

  if (val) {
    sig->s[iSlot] |= 1<<iBit;
  }
  else {
    sig->s[iSlot] &= ~(1<<iBit);
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
DdiCountSignatureBits
(
  Ddi_AigSignature_t *sig,
  int val,
  int bound
)
{
  int i;
  unsigned int j;
  unsigned char cval;
  int bBit=-1, bSlot=-1;

  if (bound>0) {
    bSlot = (bound-1) / (sizeof(unsigned long) * 8);
    bBit = (bound-1) % (sizeof(unsigned long) * 8);
  }

  static union {
    unsigned long uval;
    unsigned char cvals[sizeof(unsigned long)];
  } sval;
  int cntOnes = 0, totBits = DDI_AIG_SIGNATURE_BITS;

  if (!byteCountSet) {
    byteCountSet = 1;
    for (i=0; i<256; i++) {
      unsigned char bcnt = (unsigned char)0;
      unsigned char c = (unsigned char) i;
      for (j=0; j<8; j++) {
	bcnt += c%2;
	c = c>>1;
      }
      byteCountOnes[i] = bcnt;
    }
  }

  for (i=0;i<DDI_AIG_SIGNATURE_SLOTS;i++) {
    sval.uval = sig->s[i];
    if ((bSlot>=0) && (i==bSlot)) {
      i = DDI_AIG_SIGNATURE_SLOTS;
      if (bBit<63) {
	sval.uval &= ((((unsigned long)1)<<(bBit+1)) - 1);
      }
    }
    for (j=0;j<sizeof(unsigned long);j++) {
      cval = sval.cvals[j];
      cntOnes += byteCountOnes[(int)cval];
    }
  }

  if (val) return cntOnes;
  else return (totBits-cntOnes);

}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiFlipSignatureBit
(
  Ddi_AigSignature_t *sig,
  int bit
)
{
  int iBit, iSlot;

  iSlot = bit / (sizeof(unsigned long) * 8);
  iBit = bit % (sizeof(unsigned long) * 8);

  sig->s[iSlot] ^= (1<<iBit);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
DdiEqSignatures
(
  Ddi_AigSignature_t *sig0,
  Ddi_AigSignature_t *sig1,
  Ddi_AigSignature_t *sigMask
)
{
  int i;
  if (sigMask) {
     for (i=0;i<DDI_AIG_SIGNATURE_SLOTS;i++) {
	if ((sig0->s[i] ^ sig1->s[i]) & sigMask->s[i]) {
	   return (0);
	}
     }
     return (1);
  } else {
     return memcmp(sig0->s, sig1->s, DDI_AIG_SIGNATURE_SLOTS*sizeof(unsigned long))==0;
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
DdiEqComplSignatures
(
  Ddi_AigSignature_t *sig0,
  Ddi_AigSignature_t *sig1,
  Ddi_AigSignature_t *sigMask
)
{
  int i;
  if (sigMask) {
     for (i=0;i<DDI_AIG_SIGNATURE_SLOTS;i++) {
       if ((sig0->s[i] ^ (~sig1->s[i])) & sigMask->s[i]) {
	   return (0);
	}
     }
     return (1);
  } else {
     for (i=0;i<DDI_AIG_SIGNATURE_SLOTS;i++) {
       if ((sig0->s[i] ^ (~sig1->s[i]))) {
	   return (0);
	}
     }
     return (1);
  }
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
DdiImplySignatures
(
  Ddi_AigSignature_t *sig0,
  Ddi_AigSignature_t *sig1,
  Ddi_AigSignature_t *sigMask
)
{
  int i;
  if (sigMask) {
     for (i=0;i<DDI_AIG_SIGNATURE_SLOTS;i++) {
	if ((sig0->s[i] & ~(sig1->s[i])) & sigMask->s[i]) {
	   return (0);
	}
     }
  } else {
     for (i=0;i<DDI_AIG_SIGNATURE_SLOTS;i++) {
	if (sig0->s[i] & ~(sig1->s[i])) {
	   return (0);
	}
     }
  }
  return(1);
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiComplementSignature
(
  Ddi_AigSignature_t *sig
)
{
  int i;
  for (i=0;i<DDI_AIG_SIGNATURE_SLOTS;i++) {
    sig->s[i] = ~sig->s[i];
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiUpdateVarSignature(
  Ddi_AigSignatureArray_t *sA,
  Ddi_Bdd_t *f
)
{
  int bit, iSlot, k, n=sA->nSigs;
  unsigned long val1, val0;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  DdManager *mgrCU = Ddi_MgrReadMgrCU(ddm);
  int nVars = Ddi_MgrReadNumVars(ddm)/*+Ddi_MgrReadNumAigVars(ddm)*/;

  Pdtutil_Assert( n>=nVars, "Wrong var signature size");

  iSlot = sA->currBit / (sizeof(unsigned long) * 8);
  bit = sA->currBit % (sizeof(unsigned long) * 8);
  val1 = 1<<bit;
  val0 = ~val1;

  sA->currBit = ++sA->currBit % 
    (DDI_AIG_SIGNATURE_SLOTS*(sizeof(unsigned long)*8));

  if (Ddi_BddIsPartConj(f)) {
    int i;
    for (i=0; i<Ddi_BddPartNum(f); i++) {
      Ddi_Bdd_t *p_i = Ddi_BddPartRead(f,i);
      Ddi_Var_t *v = Ddi_BddTopVar(p_i);
      k = Ddi_VarIndex(v);
      if (Ddi_BddIsComplement(p_i)) {
	sA->sArray[k].s[iSlot] &= val0;
      }
      else {
	sA->sArray[k].s[iSlot] &= val1;
      }
    }
  }
  else {

    DdNode *f_cu = Ddi_BddToCU(f);
    int *cube = Pdtutil_Alloc(int, nVars);
    Cudd_BddToCubeArray(mgrCU,f_cu,cube);
    
    for (k=0;k<n;k++) {
       switch (cube[k]) {
      case 0:
	sA->sArray[k].s[iSlot] &= val0;
	break;
      case 1:
	sA->sArray[k].s[iSlot] |= val1;
	break;
      case 2:
	break;
      default:
	break;
      }
    }    
    Pdtutil_Free(cube);
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiUpdateVarSignatureAllBits(
  Ddi_AigSignatureArray_t *sA,
  Ddi_Bdd_t *f
)
{
  int k, n=sA->nSigs;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  DdManager *mgrCU = Ddi_MgrReadMgrCU(ddm);
  DdNode *f_cu = Ddi_BddToCU(f);
  int *cube = Pdtutil_Alloc(int, n);

  Pdtutil_Assert(Ddi_MgrReadNumVars(ddm)==n,"wrong num of vars in sig upd");

  Cudd_BddToCubeArray(mgrCU,f_cu,cube);

  for (k=0;k<n;k++) {
    switch (cube[k]) {
    case 0:
      DdiSetSignatureConstant(&(sA->sArray[k]),0);
      break;
    case 1:
      DdiSetSignatureConstant(&(sA->sArray[k]),1);
      break;
    case 2:
      break;
    default:
      break;
    }
  }
  Pdtutil_Free(cube);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
DdiUpdateInputVarSignatureAllBits (
  Ddi_AigSignatureArray_t *varSigs,
  Ddi_Vararray_t **psVars,
  Ddi_Vararray_t **piVars,
  Solver& S,
  int assumeFrames,
  int step, 
  int flipInit
)
{
   Ddi_Vararray_t *vars;
   Ddi_Var_t *v;
   Ddi_Bdd_t *node;
   int i, j, lit, numVars, numFlipped, numTotalFlip;
   int varTime, varIndex, signatureSize;

   /* set primary inputs */
   for (i=0; i<=assumeFrames; i++) {
      for (j=0; j<Ddi_VararrayNum(piVars[i]); j++) {
	 v = Ddi_VararrayRead(piVars[i], j);
	 node = Ddi_BddMakeLiteralAig(v, 1);
	 lit = DdiAigCnfLit(S, node) - 1;
	 assert(lit >= 0);
	 Pdtutil_Assert(S.model[lit] != l_Undef, "undefined var in cex");
	 lit = (S.model[lit] == l_True);
	 DdiSetSignatureConstant(&varSigs->sArray[Ddi_VarIndex(v)], lit);
	 Ddi_Free(node);
      }
   }

#if 0
   /* set states */
   for (i=0; i<=assumeFrames; i++) {
      for (j=0; j<Ddi_VararrayNum(psVars[i]); j++) {
	 v = Ddi_VararrayRead(psVars[i], j);
	 node = Ddi_BddMakeLiteralAig(v, 1);
	 lit = DdiAigCnfLit(S, node) - 1;
	 assert(lit >= 0);
	 Pdtutil_Assert(S.model[lit] != l_Undef, "undefined var in cex");
	 lit = (S.model[lit] == l_True);
	 SetSignatureConstant(&varSigs->sArray[Ddi_VarIndex(v)], lit);
	 Ddi_Free(node);
      }
   }
#else
   /* set initial state */
   for (i=0; i<Ddi_VararrayNum(psVars[0]); i++) {
      v = Ddi_VararrayRead(psVars[0], i);
      node = Ddi_BddMakeLiteralAig(v, 1);
      lit = DdiAigCnfLit(S, node) - 1;
      assert(lit >= 0);
      Pdtutil_Assert(S.model[lit] != l_Undef, "undefined var in cex");
      lit = (S.model[lit] == l_True);
      DdiSetSignatureConstant(&varSigs->sArray[Ddi_VarIndex(v)], lit);
      Ddi_Free(node);
   }
#endif
   //return 1;
   /* flip signature bits */
   numVars = Ddi_VararrayNum(piVars[0]);
   numTotalFlip = numVars*(assumeFrames+1);
   signatureSize = DDI_AIG_SIGNATURE_SLOTS*sizeof(unsigned long)*8;
   numFlipped = step * signatureSize;
   for (i=numFlipped; i<numFlipped+signatureSize; i++) {
      if (i == numTotalFlip) {
	 return 1;
      }
      varTime = i / numVars;
      varIndex = i % numVars;
      v = Ddi_VararrayRead(piVars[varTime], varIndex);
      DdiFlipSignatureBit(&varSigs->sArray[Ddi_VarIndex(v)], i%signatureSize);
   }
   return 0;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiSetSignaturesRandom(
  Ddi_AigSignatureArray_t *sig,
  int n
)
{
  int k, i;
  unsigned int j;
  unsigned long uval;

  //srand(1000);

  for (k=0;k<n;k++) {
    for (i=0;i<DDI_AIG_SIGNATURE_SLOTS;i++) {
      uval = (((unsigned long)rand())<<32) ^ ((unsigned)rand());
      sig->sArray[k].s[i] = uval;
    }
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiSetSignatureRandom(
  Ddi_AigSignature_t *sig
)
{
  int i;
  unsigned long uval;

  //srand(1000);

  for (i=0;i<DDI_AIG_SIGNATURE_SLOTS;i++) {
    uval = (((unsigned long)rand())<<32) ^ ((unsigned)rand());
    sig->s[i] = uval;
  }
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiEvalSignature(
  Ddi_AigSignature_t *sig,
  Ddi_AigSignature_t *sig0,
  Ddi_AigSignature_t *sig1,
  int neg0,
  int neg1
)
{
  int i;

  for (i=0;i<DDI_AIG_SIGNATURE_SLOTS;i++) {

    unsigned long uval, uval0, uval1;

    uval0 = neg0 ? ~(sig0->s[i]) : sig0->s[i];
    uval1 = neg1 ? ~(sig1->s[i]) : sig1->s[i];

    uval = uval0 & uval1;
    sig->s[i] = uval;

  }

}



#define PC_VERSION 1
#if PC_VERSION

/**Function********************************************************************
  Synopsis     [compute binomial coefficient, i.e. state space estimation 
                for n variables with patterns at distance d.
  SideEffects  []
******************************************************************************/
float 
BinCoeff( 
  int n, 
  int d
)
{
  int i, j;
  static float C[20000][20000];
  if (n>=20000 || d>=20000) return 0.0;
  
  for (j = 1; j <= d; j++)
    C[0][j] = 0.0;
  for (i = 0; i <= n; i++)
    C[i][0] = 1.0;

  for (i = 1; i <= n; i++)
    for (j = 1; j <= d; j++)
      C[i][j] = C[i-1][j-1] + C[i-1][j];
  return C[n][d];
}



/**Function********************************************************************
  Synopsis     [estimate space coverage starting from a constrained signature
  Description  [estimate space coverage starting from a constrained signature.
  The signature is a set of onset assignments for f. 
  Try to randomly pick nearby 
  assignents and heuristically estimate the ratio of covered space.
  SideEffects  []
******************************************************************************/
long double
DdiAigSigEstimateCoverage(
  Ddi_Bdd_t *f,
  Ddi_AigSignatureArray_t *varSigs,
  int nBits
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  Ddi_Vararray_t *suppA = Ddi_BddSuppVararray(f);
  Ddi_AigSignatureArray_t *outSig, *outSig0, *varSigsNew;
  Ddi_Bddarray_t *fA;
  int i, j, cnt, cnt0, d, newCnt;
  long double ratio, prevRatio, estimate=0.0;
  float modifyRate;
  int nFlippedSigBits;
  int activeBits=0, nv = Ddi_VararrayNum(suppA);

  fA = Ddi_BddarrayAlloc(ddm, 1);
  Ddi_BddarrayWrite(fA,0,f);

  outSig0 = DdiAigArrayEvalSignature(fA,varSigs,1.0);
  cnt0 = DdiCountSignatureBits(&outSig0->sArray[0], 1, nBits);

  Pdtutil_Assert(cnt0>=nBits,"wrong constrained sig");
  srand(1);  

  printf("%d vars value balance 1 \% (space is %g):\n", nv, pow(2,nv));
  for (i=0; i<Ddi_VararrayNum(suppA); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(suppA,i);
    int varId = Ddi_VarIndex(v);
      // get signature for i-th var
    Ddi_AigSignature_t *sig = &(varSigs->sArray[varId]);
    int cnt = DdiCountSignatureBits(sig, 1, nBits); 
    float myRatio = ((float)cnt)/nBits;
    printf("%4.2f ", myRatio);
  }
  printf("\n");

  prevRatio = 1.0;
  for (d = 1; (d <= nv)&&(d<10) ; d++) {
    int bit;
    
    varSigsNew = DdiAigSignatureArrayDup(varSigs);
    outSig = DdiAigArrayEvalSignature(fA,varSigsNew,1.0);
    cnt = DdiCountSignatureBits(&outSig->sArray[0], 1, nBits);
    DdiAigSignatureArrayFree(outSig);
    Pdtutil_Assert(cnt0 == cnt,"signature simulation mismatch");


    for (bit=0; bit<nBits; bit++) {
      for (i=1; i<=d; i++) {
	int var = ((float)rand()/((float)RAND_MAX)) * (nv);
      
	Pdtutil_Assert((var>=0) && (var<nv),"wrong flip var");

	Ddi_Var_t *v = Ddi_VararrayRead(suppA,var);
	int varId = Ddi_VarIndex(v);
	// get signature for i-th var
	Ddi_AigSignature_t *sig = &(varSigsNew->sArray[varId]);
	DdiFlipSignatureBit (sig,bit);
      }
    }

    outSig = DdiAigArrayEvalSignature(fA,varSigsNew,1.0);
    cnt = DdiCountSignatureBits(&outSig->sArray[0], 1, nBits);
    Pdtutil_Assert((cnt0-cnt)<=nBits,"too many modified bits in signature");
    DdiAigSignatureArrayFree(outSig);

    //    printf("nBits: %d, cnt0: %d, cnt: %d\n", nBits, cnt0, cnt);
    newCnt = (nBits-(cnt0-cnt));
    if (newCnt == 0) {
      ratio = 0.0;
    }
    else { 
      ratio = ((double) (nBits-(cnt0-cnt))) / nBits;
    }
    DdiAigSignatureArrayFree(varSigsNew);
    printf("%d patterns - ratio[%f] = %g, %g patterns at distance %d\n",  nBits,  modifyRate, ratio, nBits*ratio, d);
    printf("Space size at distance %d is: %g\n", d, BinCoeff(nv, d));
    printf("Estimated minterms  at distance %d is: %g\n", d, ratio*BinCoeff(nv, d));

    if (prevRatio<0.05 && ratio<0.05) {
      break;
    }
    activeBits++;
    prevRatio = ratio;

    if (ratio*nBits >= 1)
      estimate += ratio*BinCoeff(nv, d);
  }
  
  DdiAigSignatureArrayFree(outSig0);
  if (nv > 1000) {
    int dd = nv-1000;
    printf("Space too BIG %d bits downscaled by %d (10^%.2f )\n", nv, dd,
	   (float)(dd/3.321928));
    nv = 1000; 
  }

  ratio = estimate / powl((long double)2,(long double)nv);

  Ddi_Free(fA);
  Ddi_Free(suppA);
  printf("estimate with binomial Coeficient: %Lg\n", estimate);
  printf("estimate with binomial coeficient: %lg\n", estimate);

  return ratio;
}

/**Function********************************************************************
  Synopsis     [Check for inclusion (f in g). Return non 0 if true]
  Description  [Check for inclusion (f in g). Return non 0 if true.
    This test requires the second operand (g) to be monolithic, whereas
    monolithic and disjunctively partitioned forms are allowed for first
    operand (f).]
  SideEffects  []
******************************************************************************/
long double
Ddi_AigEstimateMintermCount (
  Ddi_Bdd_t *f,
  int numv
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  Ddi_Bdd_t *cubeF, *myF, *eqF;
  Ddi_AigSignatureArray_t *outSig;
  Ddi_Bddarray_t *fA;
  int i, nF=0, cnt, nSupp;
  long double totCnt;
  Ddi_Varset_t *vars;
  int strategy = 2;


  myF = Ddi_BddDup(f);
  Ddi_BddSetAig(myF);
  vars = Ddi_BddSupp(f);
  nSupp = Ddi_VarsetNum(vars);
  /* look for implied vars */
  cubeF = DdiAigImpliedVarsAcc(myF,NULL,vars);

  //  eqF = DdiAigEquivVarsAcc(myF,NULL,NULL,vars,NULL,NULL,NULL);

  Ddi_Free(vars);
  //  Ddi_Free(eqF);

  if (cubeF!=NULL) {
    Ddi_AigConstrainCubeAcc(myF,cubeF);
    Ddi_BddSetMono(cubeF);
    nF = Ddi_BddSize(cubeF)-1;
    Ddi_Free(cubeF);
  }

  if (strategy == 1) {
    fA = Ddi_BddarrayAlloc(ddm, 1);
    Ddi_BddarrayWrite(fA,0,myF);
    
    outSig = DdiAigArrayEvalSignatureRandomPi(fA);
    
    cnt = DdiCountSignatureBits(&outSig->sArray[0], 1, -1);
    if (cnt==0) {
      DdiAigSignatureArrayFree(outSig);
      outSig = DdiAigArrayEvalSignatureRandomPi(fA);
      cnt = DdiCountSignatureBits(&outSig->sArray[0], 1, -1);
    }
    Ddi_Free(fA);
    totCnt = ((double) cnt) / DDI_AIG_SIGNATURE_BITS;
    DdiAigSignatureArrayFree(outSig);

    printf("minterm estim: cnt=%d, nSupp=%d/%d, ncube=%d\n",
	 cnt, nSupp, numv, nF);

  }
  else if (strategy == 2) {
    int nVars = Ddi_MgrReadNumVars(ddm);
    long double t;
    float density;

    Ddi_AigSignatureArray_t *varSigs = DdiAigSignatureArrayAlloc(nVars);
    DdiAigSignatureArrayClear(varSigs);
    int num = DdiAigConstrainSignatures(NULL, myF, varSigs, 10, &density, 1000);

    if (!num) {
      fprintf(dMgrO(ddm),"Signature constraining failed.\n");
    }

    t = totCnt = DdiAigSigEstimateCoverage(myF,varSigs,num);
    //totCnt *= density;

    DdiAigSignatureArrayFree(varSigs);
    //    totCnt = ((double) density);

    printf("minterm estim: cnt=%LG, tot=%LG, effort=%g, nSupp=%d/%d, ncube=%d\n",
	   t, totCnt, density, nSupp, numv, nF);
    printf("spaces are: %g (%d vars) - %g (%d vars)\n", 
	   pow(2,nSupp-nF), nSupp-nF,
	   pow(2,numv-nF), numv-nF);

  }
  else {
    int nVars = Ddi_MgrReadNumVars(ddm);
    float density;
    Ddi_AigSignatureArray_t *varSigs = DdiAigSignatureArrayAlloc(nVars);
    int num = DdiAigConstrainSignatures(NULL, myF, varSigs, 10, &density, 1000);
    if (!num) {
      fprintf(dMgrO(ddm),"Signature constraining failed.\n");
    }
    DdiAigSignatureArrayFree(varSigs);
    totCnt = ((double) density);

    printf("minterm estim: dens=%g, nSupp=%d/%d, ncube=%d\n",
	density, nSupp, numv, nF);

  }

  Ddi_Free(myF);

  while (nF-- > 0) {
    totCnt /= 2;
  }
  if (numv > 1000) {
    int dd = numv-1000;
    printf("Space too BIG %d bits downscaled (for multiplication) by %d (10^%.2f )\n", 
	   numv,dd,(float)(dd/3.321928));
    numv = 1000; 
  }
  while (numv-- > 0) {
    totCnt *= 2;
  }

  return totCnt;

}

#else
/**Function********************************************************************
  Synopsis     [estimate space coverage starting from a constrained signature
  Description  [estimate space coverage starting from a constrained signature.
  The signature is a set of onset assignments for f. 
  Try to randomly pick nearby 
  assignents and heuristically estimate the ratio of covered space.
  SideEffects  []
******************************************************************************/
long double
DdiAigSigEstimateCoverage(
  Ddi_Bdd_t *f,
  Ddi_AigSignatureArray_t *varSigs,
  int nBits
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  Ddi_Vararray_t *suppA = Ddi_BddSuppVararray(f);
  Ddi_AigSignatureArray_t *outSig, *varSigsNew;
  Ddi_Bddarray_t *fA;
  int i, j, cnt, cnt0;
  double ratio;
  float modifyRate;
  int nFlippedSigBits;

  fA = Ddi_BddarrayAlloc(ddm, 1);
  Ddi_BddarrayWrite(fA,0,f);

  outSig = DdiAigArrayEvalSignature(fA,varSigs,1.0);
  cnt0 = DdiCountSignatureBits(&outSig->sArray[0], 1, -1);
  DdiAigSignatureArrayFree(outSig);

  Pdtutil_Assert(cnt0>=nBits,"wrong constrained sig");

  varSigsNew = DdiAigSignatureArrayDup(varSigs);
  modifyRate = 0.01;  
  nFlippedSigBits = modifyRate * nBits;

  for (i=0; i<Ddi_VararrayNum(suppA); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(suppA,i);
    int varId = Ddi_VarIndex(v);
    // get signature for i-th var
    Ddi_AigSignature_t *sig = &(varSigsNew->sArray[varId]);
    for (j=0; j<nFlippedSigBits; j++) {
      int bit = ((float)rand()/((float)RAND_MAX)) * (nBits-1);
      DdiFlipSignatureBit (sig,bit);
    }
  }

  outSig = DdiAigArrayEvalSignature(fA,varSigsNew,1.0);
  cnt = DdiCountSignatureBits(&outSig->sArray[0], 1, -1);
  DdiAigSignatureArrayFree(outSig);

  ratio = ((double) (nBits-(cnt0-cnt))) / nBits;

  Ddi_Free(fA);
  Ddi_Free(suppA);
  DdiAigSignatureArrayFree(varSigsNew);

  return ratio;
}

/**Function********************************************************************
  Synopsis     [Check for inclusion (f in g). Return non 0 if true]
  Description  [Check for inclusion (f in g). Return non 0 if true.
    This test requires the second operand (g) to be monolithic, whereas
    monolithic and disjunctively partitioned forms are allowed for first
    operand (f).]
  SideEffects  []
******************************************************************************/
double
Ddi_AigEstimateMintermCount (
  Ddi_Bdd_t *f,
  int numv
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  Ddi_Bdd_t *cubeF, *myF;
  Ddi_AigSignatureArray_t *outSig;
  Ddi_Bddarray_t *fA;
  int i, nF=0, cnt, nSupp;
  double totCnt;
  Ddi_Varset_t *vars;
  int strategy = 2;


  myF = Ddi_BddDup(f);
  Ddi_BddSetAig(myF);
  vars = Ddi_BddSupp(f);
  nSupp = Ddi_VarsetNum(vars);
  /* look for implied vars */
  cubeF = DdiAigImpliedVarsAcc(myF,NULL,vars);
  Ddi_Free(vars);

  if (cubeF!=NULL) {
    Ddi_AigConstrainCubeAcc(myF,cubeF);
    Ddi_BddSetMono(cubeF);
    nF = Ddi_BddSize(cubeF)-1;
    Ddi_Free(cubeF);
  }

  if (strategy == 1) {
    fA = Ddi_BddarrayAlloc(ddm, 1);
    Ddi_BddarrayWrite(fA,0,myF);
    
    outSig = DdiAigArrayEvalSignatureRandomPi(fA);
    
    cnt = DdiCountSignatureBits(&outSig->sArray[0], 1, -1);
    if (cnt==0) {
      DdiAigSignatureArrayFree(outSig);
      outSig = DdiAigArrayEvalSignatureRandomPi(fA);
      cnt = DdiCountSignatureBits(&outSig->sArray[0], 1, -1);
    }
    Ddi_Free(fA);
    totCnt = ((double) cnt) / DDI_AIG_SIGNATURE_BITS;
    DdiAigSignatureArrayFree(outSig);

    printf("minterm estim: cnt=%d, nSupp=%d/%d, ncube=%d\n",
	 cnt, nSupp, numv, nF);

  }
  else if (strategy == 2) {
    int nVars = Ddi_MgrReadNumVars(ddm);
    float density, t;

    Ddi_AigSignatureArray_t *varSigs = DdiAigSignatureArrayAlloc(nVars);
    int num = DdiAigConstrainSignatures(NULL, myF, varSigs, 10, &density, 1000);

    if (!num) {
      fprintf(dMgrO(ddm),"Signature constraining failed.\n");
    }

    t = totCnt = DdiAigSigEstimateCoverage(myF,varSigs,num);
    totCnt *= density;

    DdiAigSignatureArrayFree(varSigs);
    //    totCnt = ((double) density);

    printf("minterm estim: cnt=%g, dens=%g, nSupp=%d/%d, ncube=%d\n",
	t, totCnt, nSupp, numv, nF);

  }
  else {
    int nVars = Ddi_MgrReadNumVars(ddm);
    float density;
    Ddi_AigSignatureArray_t *varSigs = DdiAigSignatureArrayAlloc(nVars);
    int num = DdiAigConstrainSignatures(NULL, myF, varSigs, 10, &density, 1000);
    if (!num) {
      fprintf(dMgrO(ddm),"Signature constraining failed.\n");
    }
    DdiAigSignatureArrayFree(varSigs);
    totCnt = ((double) density);

    printf("minterm estim: dens=%g, nSupp=%d/%d, ncube=%d\n",
	density, nSupp, numv, nF);

  }

  //Ddi_Free(myF);

  while (nF-- > 0) {
    totCnt /= 2;
  }
  while (numv-- > 0) {
    totCnt *= 2;
  }

  return totCnt;

}
#endif

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_AigDynSignatureArray_t *
DdiAigDynSignatureArrayAlloc(
  int num, unsigned int size
)
{
  Ddi_AigDynSignatureArray_t *sA;
  int i;

  sA = Pdtutil_Alloc(Ddi_AigDynSignatureArray_t,1);
  sA->sArray = Pdtutil_Alloc(Ddi_AigDynSignature_t*,num);
  for(i=0;i<num;i++) {
    sA->sArray[i] = DdiAigDynSignatureAlloc(size);
  }
  sA->currBit = 0;
  sA->nBits = size;
  sA->nSigs = num;
  sA->size = num;
  return (sA);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_AigDynSignatureArray_t *
DdiAigDynSignatureArrayAllocVoid(
  int num, unsigned int size
)
{
  Ddi_AigDynSignatureArray_t *sA;
  int i;

  sA = Pdtutil_Alloc(Ddi_AigDynSignatureArray_t,1);
  sA->sArray = Pdtutil_Alloc(Ddi_AigDynSignature_t*,num);
  for(i=0;i<num;i++) {
    sA->sArray[i] = NULL;
  }
  sA->currBit = 0;
  sA->nBits = size;
  sA->nSigs = 0;
  sA->size = num;
  return (sA);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
DdiAigDynSignatureAllocCheck(
  Ddi_AigDynSignatureArray_t *sA,
  int i,
  Ddi_AigDynSignatureArray_t *freeSigs
)
{
  if (sA->sArray[i] != NULL) return -1;

  Pdtutil_Assert(sA->nBits==freeSigs->nBits,"signature bits do not match");

  if (freeSigs->nSigs>0) {
    freeSigs->nSigs--;
    sA->sArray[i] = freeSigs->sArray[freeSigs->nSigs];
    DdiAigDynSignatureClear(sA->sArray[i]);
    return 0;
  }
  else {
    sA->sArray[i] = DdiAigDynSignatureAlloc(freeSigs->nBits);
    return 1;
  }
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiAigDynSignatureMoveToFreeList(
  Ddi_AigDynSignatureArray_t *sA,
  int i,
  Ddi_AigDynSignatureArray_t *freeSigs
)
{
  Pdtutil_Assert(freeSigs->nSigs<freeSigs->size,"free sig atack full");

  freeSigs->sArray[freeSigs->nSigs++] = sA->sArray[i];
  sA->sArray[i] = NULL;
}



/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiAigDynSignatureArrayFree(
  Ddi_AigDynSignatureArray_t *sA
)
{
  if (sA != NULL) {
    int i;
    for(i=0;i<sA->nSigs;i++) {
      if (sA->sArray[i]!=NULL)
        DdiAigDynSignatureFree(sA->sArray[i]);
    } 
    Pdtutil_Free(sA->sArray);
    Pdtutil_Free(sA);
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_AigDynSignature_t *
DdiAigDynSignatureAlloc(
  unsigned int size
)
{
  Ddi_AigDynSignature_t *s;

  nAllocatedBitmaps++;

  s = Pdtutil_Alloc(Ddi_AigDynSignature_t,1);
  s->s = Pdtutil_Alloc(unsigned long,DdiAigDynSignaturePatternSize(size));
  memset(s->s, 0, sizeof(unsigned long) * DdiAigDynSignaturePatternSize(size)); 
  s->size = size;
  return (s);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
DdiAigDynSignatureFree(
  Ddi_AigDynSignature_t *s
)
{
  if (s != NULL) {
    Pdtutil_Free(s->s);
    Pdtutil_Free(s);
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
DdiAigDynSignatureArrayNum(
  Ddi_AigSignatureArray_t *s
)
{
  return s->nSigs;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_AigDynSignature_t *
DdiAigDynSignatureArrayRead(
  Ddi_AigDynSignatureArray_t *sA,
  int i
)
{
  return (sA->sArray[i]);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
unsigned int 
DdiAigDynSignatureSize(
  Ddi_AigDynSignature_t* s
)
{
  if (s != NULL)
    return s->size;

  return 0;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void 
DdiAigDynSignatureClear(
  Ddi_AigDynSignature_t *s
)
{
  if (s != NULL) 
    memset(s->s, 0, sizeof(unsigned long) * DdiAigDynSignaturePatternSize(
      s->size)); 
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void 
DdiAigDynSignatureSetBit(
  Ddi_AigDynSignature_t *s, 
  unsigned int i
)
{
  Pdtutil_Assert(i < s->size, "signature index is not valid");

  unsigned long bitmask = 1;  
  unsigned int location = DdiAigDynSignatureIndexToSlot(i);
  unsigned int position = (8*sizeof (unsigned long)) - (i % (8*sizeof(unsigned long))) -1;
  s->s[location] |= (bitmask << position);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
DdiAigDynSignatureTestBit(
  Ddi_AigDynSignature_t *s, 
  unsigned int i
)
{
  Pdtutil_Assert(i < s->size, "signature index is not valid");

  unsigned long bitmask = 1;  
  unsigned int location = DdiAigDynSignatureIndexToSlot(i);
  unsigned int position = (8*sizeof (unsigned long)) - (i % (8*sizeof(unsigned long))) -1;
  return ((s->s[location] & (bitmask << position)) != 0);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int 
DdiAigDynSignatureUnion(
  Ddi_AigDynSignature_t *result, 
  Ddi_AigDynSignature_t *s1, 
  Ddi_AigDynSignature_t *s2,
  int keepOldValue
)
{
  unsigned int i;
  int changed = 0;
  unsigned long oldValue;
  int slots = DdiAigDynSignaturePatternSize(s1->size);

  for(i=0;i<slots;i++) {
    oldValue = result->s[i];

    if (keepOldValue) 
      result->s[i] |= s1->s[i] | s2->s[i];
    else
      result->s[i] = s1->s[i] | s2->s[i];

    if (oldValue != result->s[i])
      changed = 1;
  }
  return changed;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int 
DdiAigDynSignatureIntersection(
  Ddi_AigDynSignature_t *result, 
  Ddi_AigDynSignature_t *s1, 
  Ddi_AigDynSignature_t *s2,
  int keepOldValue
)
{
  unsigned int i;
  int changed = 0;
  unsigned long oldValue;
  int slots = DdiAigDynSignaturePatternSize(s1->size);

  for(i=0;i<slots;i++) {
    oldValue = result->s[i];

    if (keepOldValue) 
      result->s[i] &= s1->s[i] & s2->s[i];
    else
      result->s[i] = s1->s[i] & s2->s[i];

    if (oldValue != result->s[i])
      changed = 1;
  }
  return changed;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void 
DdiAigDynSignatureCopy(
  Ddi_AigDynSignature_t *dest, 
  Ddi_AigDynSignature_t *src
)
{
  unsigned int i;

  if (dest == NULL) {
    dest = DdiAigDynSignatureAlloc(src->size);
  }

  int slots = DdiAigDynSignaturePatternSize(dest->size);
  for(i=0;i<slots;i++) {
    dest->s[i] = src->s[i];
  }
  dest->size = src->size;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
unsigned int 
DdiAigDynSignaturePatternSize(
  unsigned int size
)
{
  return ((size % (8 * sizeof(unsigned long))) ? 
    ((size / (8 * sizeof(unsigned long))) + 1) : (size / (8 * sizeof(unsigned long))));
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
unsigned int 
DdiAigDynSignatureIndexToSlot(
  unsigned int index
)
{
  return (index / (8 * sizeof(unsigned long))) ;
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void 
DdiAigDynSignatureArrayInit(
  Ddi_AigDynSignatureArray_t *sA
)
{
  unsigned int i;
  if (sA == NULL)
    return;
  
  for (i=0;i<sA->nSigs;i++)
    DdiAigDynSignatureSetBit(sA->sArray[i], i);
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
DdiAigDynSignatureCountBits
(
  Ddi_AigDynSignature_t *sig,
  int val
)
{
  int i;
  unsigned int j;
  unsigned char cval;
  union {
    unsigned long uval;
    unsigned char cvals[sizeof(unsigned long)];
  } sval;
  int slots = DdiAigDynSignaturePatternSize(sig->size);
  int cntOnes = 0, totBits = sig->size;

  if (!byteCountSet) {
    byteCountSet = 1;
    for (i=0; i<256; i++) {
      unsigned char bcnt = (unsigned char)0;
      unsigned char c = (unsigned char) i;
      for (j=0; j<8; j++) {
	bcnt += c%2;
	c = c>>1;
      }
      byteCountOnes[i] = bcnt;
    }
  }

  for (i=0;i<slots;i++) {
    sval.uval = sig->s[i];
    for (j=0;j<sizeof(unsigned long);j++) {
      cval = sval.cvals[j];
      cntOnes += byteCountOnes[(int)cval];
    }
  }

  if (val) return cntOnes;
  else return (totBits-cntOnes);

}



/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Varset_t *
DdiDynSignatureToVarset(
  Ddi_AigDynSignature_t *varSig,
  Ddi_Vararray_t *vars/*,
	double **data,
	int rigaCluster,
	int *column,
	int **mask*/
)
{

  Ddi_Mgr_t *ddm = Ddi_ReadMgr(vars);
  int i, ii, nVars = Ddi_VararrayNum(vars);
  Ddi_Varset_t *supp;
	FILE *fout;
	//fout=fopen("outBitmap.txt","w");
  Pdtutil_Assert(nVars==varSig->size,"signature bits and var num mismatch");
  supp = Ddi_VarsetVoid(ddm);
  Ddi_VarsetSetBdd(supp);
  /*ALLOCO MATRICE*/
	//data[rigaCluster] = (double *) malloc (sizeof(double)*nVars);
	//data[rigaCluster] = Pdtutil_Alloc(double,nVars);
	//mask[rigaCluster] = (int *) malloc (sizeof(int)*nVars);
	//mask[rigaCluster] = Pdtutil_Alloc(int,nVars);
	//*column = nVars;
	/*ALLOCO MATRICE*/
  for (i=nVars-1; i>=0; i--) {
		
    Ddi_Var_t *v = Ddi_VararrayRead(vars,i);
    ii = Ddi_VarReadMark(v);
    Pdtutil_Assert(ii>=0 && ii<nVars,"wrong var mark");
              
#if 0
		printf("%d ",DdiAigDynSignatureTestBit(varSig, ii));
#endif 

		//printf("%d ",DdiAigDynSignatureTestBit(varSig, ii));
		//data[rigaCluster][i] =(double) DdiAigDynSignatureTestBit(varSig, ii);
		//mask[rigaCluster][i] = 1;
		//printf("%d ",DdiAigDynSignatureTestBit(varSig, ii));

    if (DdiAigDynSignatureTestBit(varSig, ii)) {
			//printf("%d ",ii);
      Ddi_VarsetBddAddAccFast(supp,v);
    }
  }
#if 0
    	
      printf("\n"); 
     
#endif
  //printf("COI: %d\n", Ddi_VarsetNum(supp)); 
	//printf("\n");	
  //printf("COI: %d\n", Ddi_VarsetNum(supp)); 
  return supp;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Vararray_t *
DdiSccLatches(
  Ddi_SccMgr_t *sccMgr,
  int idScc,
  Ddi_Vararray_t *vars
)
{
  Ddi_Vararray_t *varsOut = Ddi_VararrayAlloc(sccMgr->ddiMgr, 0);
  int i, bit = sccMgr->mapSccBit[idScc];
  for (i=0; i<sccMgr->nLatches; i++) {
    if (sccMgr->mapLatches[i]==bit) {
      Ddi_Var_t *v = Ddi_VararrayRead(vars,i);
      Ddi_VararrayInsertLast(varsOut,v);
    }
  }
  return varsOut;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Varset_t *
DdiDynSignatureToVarsetWithScc(
  Ddi_SccMgr_t *sccMgr,
  Ddi_AigDynSignature_t *varSig,
  Ddi_Vararray_t *vars
)
{

  Ddi_Mgr_t *ddm = Ddi_ReadMgr(vars);
  int i, ii, nVars = Ddi_VararrayNum(vars);
  Ddi_Varset_t *supp;
  int nSigBits;

  Pdtutil_Assert(varSig!=NULL,"NULL signature");
  Pdtutil_Assert(nVars>=varSig->size,"signature bits and var num mismatch");
  supp = Ddi_VarsetMakeArrayVoid(ddm);

  nSigBits = varSig->size;
  Pdtutil_Assert(sccMgr->nLatches==nVars,"wrong num of vars");

  for (i=0; i<nVars; i++) {
		
    Ddi_Var_t *v = Ddi_VararrayRead(vars,i);
    int bit;
    ii = Ddi_VarReadMark(v);
    Pdtutil_Assert(ii>=0 && ii<nVars,"wrong var mark");
    bit = sccMgr->mapLatches[ii];
    if (bit>=0) {
      Pdtutil_Assert(bit<nSigBits,"wrong bit index");
      if (DdiAigDynSignatureTestBit(varSig, bit)) {
        Ddi_VarsetAddAcc(supp,v);
      }
    }
  }

  return supp;
}

/**Function********************************************************************
  Synopsis     []
  Description  []
  SideEffects  []
******************************************************************************/
Ddi_AigDynSignatureArray_t *
Ddi_AigEvalVarSignatures (
  Ddi_Mgr_t *ddm,
  bAig_array_t *aigNodes,
  Ddi_AigDynSignatureArray_t *varSig,
  int *mapPs,
  int *mapNs,
  int nMap
)
{
  bAig_Manager_t *manager = ddm->aig.mgr;
  int i, il, ir;
  bAigEdge_t right, left;
  Ddi_AigDynSignatureArray_t *sig;
  int again;

  sig = DdiAigDynSignatureArrayAlloc(aigNodes->num,nMap);
  for (i=0; i<nMap; i++) {

    int j=mapPs[i];
    DdiAigDynSignatureCopy(sig->sArray[j], varSig->sArray[i]);
  }

  do {
    again=0;

    for (i=0; i<aigNodes->num; i++) {

      bAigEdge_t baig = aigNodes->nodes[i];
      // Pdtutil_Assert(bAig_AuxInt(manager,baig) == -1, "wrong auxId field");
      bAig_AuxInt(manager,baig) = i;
      
      if (bAig_NodeIsConstant(baig) || bAig_isVarNode(manager,baig)) {
      }
      else {
        int changed;
        right = bAig_NodeReadIndexOfRightChild(manager,baig);
        left = bAig_NodeReadIndexOfLeftChild(manager,baig);
        ir = bAig_AuxInt(manager,right);
        il = bAig_AuxInt(manager,left);
        changed = DdiAigDynSignatureUnion(sig->sArray[i], 
                                          sig->sArray[ir], sig->sArray[il],1);
        again |= changed;
      }
    }
    if (again) {
      for (i=0; i<nMap; i++) {
	int j=mapPs[i];
	int k=mapNs[i];
	if (k<0) {
	  // constant delta
	  DdiAigDynSignatureClear(sig->sArray[j]);
	}
	else {
	  DdiAigDynSignatureCopy(sig->sArray[j],sig->sArray[k]);
	}
      }
    }
  } while (again);

  return(sig);
}

/**Function********************************************************************
  Synopsis     []
  Description  []
  SideEffects  []
******************************************************************************/
Ddi_AigDynSignatureArray_t *
Ddi_AigEvalVarSignaturesWithScc (
  Ddi_SccMgr_t *sccMgr,
  Ddi_Mgr_t *ddm,
  bAig_array_t *aigNodes,
  Ddi_Bddarray_t *lambda,
  Ddi_AigDynSignatureArray_t *sccSig,
  int *mapScc,
  int *mapNs,
  int nScc
)
{
  bAig_Manager_t *manager = ddm->aig.mgr;
  int i, il, ir, iLdr, iLdrR, iLdrL;
  bAigEdge_t right, left, ldr;
  Ddi_AigDynSignatureArray_t *freeSigs;
  int freeNum = 0;
  int *foCnt;

  freeSigs = DdiAigDynSignatureArrayAllocVoid(sccSig->nSigs,sccSig->nBits);
  foCnt = Pdtutil_Alloc(int, sccSig->nSigs);

  for (i=0; i<sccSig->nSigs; i++) {
    foCnt[i] = 0;
  }

  for (i=0; i<aigNodes->num; i++) {
    bAigEdge_t baig = aigNodes->nodes[i];
    iLdr = mapScc[i];

    if (bAig_NodeIsConstant(baig)) continue;
    if (bAig_isVarNode(manager,baig)) {
      int iNs = mapNs[i];
      if (iNs>=0) {
        /* latch */
        iLdrR = iLdrL = mapScc[iNs];
        if (0) {
          printf("LATCH: %s - scc #: %d\n", 
                 bAig_NodeReadName(manager,baig),iLdr);
        }

        if (iLdr != iLdrR) {
          foCnt[iLdrR]++;
        }
      }
    }
    else {
      int changed;
      right = bAig_NodeReadIndexOfRightChild(manager,baig);
      left = bAig_NodeReadIndexOfLeftChild(manager,baig);
      /* take SCC leaders */
      ir = bAig_AuxInt(manager,right);
      il = bAig_AuxInt(manager,left);
      iLdrR = mapScc[ir];
      iLdrL = mapScc[il];

      if (iLdrR != iLdr || iLdrL != iLdr) {
        foCnt[iLdrR]++;
        foCnt[iLdrL]++;
      }
    }
  }

  for (i=0; i<Ddi_BddarrayNum(lambda); i++) {		
    Ddi_Bdd_t *fAig = Ddi_BddarrayRead(lambda, i);
    if (!Ddi_BddIsConstant(fAig)) {
      bAigEdge_t fBaig = Ddi_BddToBaig(fAig);
      int j = bAig_AuxInt(manager,fBaig);
      iLdr = mapScc[j];
      foCnt[iLdr]++;
    }
  }

  for (i=0; i<aigNodes->num; i++) {
    bAigEdge_t baig = aigNodes->nodes[i];
    iLdr = mapScc[i];

    if (bAig_NodeIsConstant(baig)) continue;
    if (bAig_isVarNode(manager,baig)) {
      int iNs = mapNs[i];
      if (iNs>=0) {
        /* latch */
        iLdrR = iLdrL = mapScc[iNs];
        if (iLdr != iLdrR) {
          DdiAigDynSignatureAllocCheck(sccSig,iLdr,freeSigs);
          Pdtutil_Assert(sccSig->sArray[iLdrR]!=NULL,"missing signature");
          DdiAigDynSignatureUnion(sccSig->sArray[iLdr], 
                              sccSig->sArray[iLdrR], sccSig->sArray[iLdrL],1);
          if (1 &&--foCnt[iLdrR]==0 && sccMgr->sccLatchCnt[iLdrR]==0) {
            DdiAigDynSignatureMoveToFreeList(sccSig,iLdrR,freeSigs);
          }
        }
      }
      else {
        /* free input */
        DdiAigDynSignatureAllocCheck(sccSig,iLdr,freeSigs);
      }
    }
    else {
      int changed;
      right = bAig_NodeReadIndexOfRightChild(manager,baig);
      left = bAig_NodeReadIndexOfLeftChild(manager,baig);
      /* take SCC leaders */
      ir = bAig_AuxInt(manager,right);
      il = bAig_AuxInt(manager,left);
      iLdrR = mapScc[ir];
      iLdrL = mapScc[il];

      if (iLdrR != iLdr || iLdrL != iLdr) {
        DdiAigDynSignatureAllocCheck(sccSig,iLdr,freeSigs);
        Pdtutil_Assert(sccSig->sArray[iLdrR]!=NULL,"missing signature");
        Pdtutil_Assert(sccSig->sArray[iLdrL]!=NULL,"missing signature");
        DdiAigDynSignatureUnion(sccSig->sArray[iLdr], 
                              sccSig->sArray[iLdrR], sccSig->sArray[iLdrL],1);
        if (1 &&--foCnt[iLdrR]==0 && sccMgr->sccLatchCnt[iLdrR]==0) {
          DdiAigDynSignatureMoveToFreeList(sccSig,iLdrR,freeSigs);
        }
        if (1 &&--foCnt[iLdrL]==0 && sccMgr->sccLatchCnt[iLdrL]==0) {
          DdiAigDynSignatureMoveToFreeList(sccSig,iLdrL,freeSigs);
        }
      }
    }
  }

  DdiAigDynSignatureArrayFree(freeSigs);

  return(sccSig);
}



/**Function********************************************************************

  Synopsis    [Count nodes of a bAig. Internal recursion]
  Description [Count nodes of a bAig. Internal recursion]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
logScc(
  Ddi_SccMgr_t *sccMgr,
  int iScc
)
{
  int i;
  bAig_Manager_t *bmgr = sccMgr->ddiMgr->aig.mgr;

  fprintf(stdout,"SCC %d: %d latches - %d gates\n", iScc,
          sccMgr->sccLatchCnt[iScc], sccMgr->sccGateCnt[iScc]);

  for (i=sccMgr->sccTopologicalNodes->num-1; i>=0; i--) {
    if (sccMgr->mapScc[i]==iScc) {
      char isOutGate = sccMgr->sccOutGate[i];
      char isInGate = sccMgr->sccOutGate[i];
      bAigEdge_t baig = sccMgr->sccTopologicalNodes->nodes[i];
      if (bAig_isVarNode(bmgr, baig)) {
        fprintf(stdout,"%d: v=%s%s%s\n", bAigNodeID(baig),
                bAig_NodeReadName(bmgr,baig),
                isInGate ? " IN": "", isOutGate ? " OUT": "");
      }
      else {
        char nr = bAig_NodeIsInverted(rightChild(bmgr,baig)) ? '!' : ' ';
        char nl = bAig_NodeIsInverted(leftChild(bmgr,baig)) ? '!' : ' ';
        fprintf(stdout,"%5d = AND ( %c%5d%s, %c%5d%s )%s%s\n",
              bAigNodeID(baig),
              nr, bAigNodeID(rightChild(bmgr,baig)),
              bAig_isVarNode(bmgr,rightChild(bmgr,baig)) ? "V":"",
              nl, bAigNodeID(leftChild(bmgr,baig)),
              bAig_isVarNode(bmgr,leftChild(bmgr,baig)) ? "V":"",
                isInGate ? " IN": "", isOutGate ? " OUT": "");
      }
    }
  }
}




/**Function********************************************************************
  Synopsis     []
  Description  []
  SideEffects  []
******************************************************************************/
Ddi_SccMgr_t *
Ddi_FsmSccTarjan (
  Ddi_Bddarray_t *delta,
  Ddi_Bddarray_t *lambda,
  Ddi_Vararray_t *ps
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(delta);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  int i, currBit, sccCnt, sccWithLatchCnt, nL = Ddi_BddarrayNum(delta), 
    nO = lambda==NULL ? 0 : Ddi_BddarrayNum(lambda);
  Ddi_SccMgr_t *sccMgr;
  int chkSize=1;

  sccMgr = sccMgrAlloc(delta,ps);

  for (i=0; i<Ddi_VararrayNum(ps); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(ps,i);
    Ddi_Bdd_t *d = Ddi_BddarrayRead(delta,i);
    bAigEdge_t varIndex = Ddi_VarToBaig(v);
    bAigEdge_t dBaig = Ddi_BddToBaig(d);
    Pdtutil_Assert(bAig_CacheAig(bmgr,varIndex)==bAig_NULL,
                   "invalid cache aig");
    bAig_CacheAig(bmgr,varIndex) = dBaig;
  }

  for (i=0; i<nL; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(delta,i);
    bAigEdge_t fBaig = Ddi_BddToBaig(f_i);
    if (!bAig_NodeIsConstant(fBaig)) {
      fsmSccTarjanIntern(sccMgr,fBaig);
    }
  }
  for (i=0; i<nO; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(lambda,i);
    if (!Ddi_BddIsConstant(f_i)) {
      bAigEdge_t fBaig = Ddi_BddToBaig(f_i);
      fsmSccTarjanIntern(sccMgr,fBaig);
    }
  }

  DdiPostOrderAigClearVisitedIntern(bmgr,sccMgr->aigNodes);
  for (i=0; i<sccMgr->aigNodes->num; i++) {
    bAigEdge_t baig = sccMgr->aigNodes->nodes[i];
    bAig_AuxInt(bmgr,baig) = -1;
    bAig_AuxInt1(bmgr,baig) = -1;
  }

  sccMgr->nSccWithLatch = sccMgr->nScc-sccMgr->nSccGate;

  printf("#SCCs - #tot: %d, #L: %d, #1: %d / (L: %d - T: %d)\n", 
         sccMgr->nScc, sccMgr->nSccWithLatch, sccMgr->nSccGate, 
         sccMgr->nLatches, sccMgr->aigNodes->num);

  for (i=0; i<nL; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(delta,i);
    if (!Ddi_BddIsConstant(f_i)) {
      bAigEdge_t fBaig = Ddi_BddToBaig(f_i);
      postOrderAigVisitInternWithScc(bmgr, fBaig, 
                                     sccMgr->sccTopologicalNodes);
    }
  }
  for (i=0; i<nO; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(lambda,i);
    if (!Ddi_BddIsConstant(f_i)) {
      bAigEdge_t fBaig = Ddi_BddToBaig(f_i);
      postOrderAigVisitInternWithScc(bmgr, fBaig, 
                                     sccMgr->sccTopologicalNodes);
    }
  }
  DdiPostOrderAigClearVisitedIntern(bmgr,sccMgr->sccTopologicalNodes);

  sccMgr->nNodes = sccMgr->sccTopologicalNodes->num;
  sccMgr->sccLatchCnt = Pdtutil_Alloc(int, sccMgr->nScc);
  sccMgr->sccGateCnt = Pdtutil_Alloc(int, sccMgr->nScc);
  sccMgr->mapScc = Pdtutil_Alloc(int, sccMgr->nNodes);
  sccMgr->mapNs = Pdtutil_Alloc(int, sccMgr->nNodes);
  sccMgr->mapLatches = Pdtutil_Alloc(int, sccMgr->nLatches);
  sccMgr->mapSccBit = Pdtutil_Alloc(int, sccMgr->nScc);
  sccMgr->sccBridges = Pdtutil_Alloc(int, sccMgr->nNodes);
  sccMgr->sccBridgesCnt = Pdtutil_Alloc(int, sccMgr->nScc);
  sccMgr->sccInGate = Pdtutil_Alloc(int, sccMgr->nNodes);
  sccMgr->sccInCnt = Pdtutil_Alloc(int, sccMgr->nScc);
  sccMgr->sccOutGate = Pdtutil_Alloc(int, sccMgr->nNodes);
  sccMgr->sccGateFoCnt = Pdtutil_Alloc(int, sccMgr->nNodes);
  sccMgr->sccOutCnt = Pdtutil_Alloc(int, sccMgr->nScc);
  sccMgr->sccTopologicalLevel = Pdtutil_Alloc(int, sccMgr->nScc);
  sccMgr->sccLatchTopologicalLevel = Pdtutil_Alloc(int, sccMgr->nLatches);
  sccMgr->sccGateTopologicalLevelMin = Pdtutil_Alloc(int, sccMgr->nScc);
  sccMgr->sccGateTopologicalLevelMax = Pdtutil_Alloc(int, sccMgr->nScc);

  sccCnt = sccWithLatchCnt = 0;

  for (i=0; i<sccMgr->sccTopologicalNodes->num; i++) {
    bAigEdge_t baig = sccMgr->sccTopologicalNodes->nodes[i];
    sccMgr->sccOutGate[i] = 0;
    sccMgr->sccGateFoCnt[i] = 0;
    sccMgr->sccInGate[i] = 0;
    sccMgr->mapScc[i] = -1;
    sccMgr->mapNs[i] = -1;
    Pdtutil_Assert(bAig_AuxInt(bmgr,baig)==-1,"wrong aux int field");
    bAig_AuxInt(bmgr,baig) = i;
    Pdtutil_Assert(bAig_AuxAig1(bmgr,baig)!=bAig_NULL,
                   "wrong aux aig1 field");

    if (bAig_AuxAig1(bmgr,baig) == bAig_NonInvertedEdge(baig)) {
      /* scc leader */
      sccMgr->sccLatchCnt[sccCnt] = 0;
      sccMgr->sccGateCnt[sccCnt] = 1;
      if (bAig_CacheAig(bmgr,baig)!=bAig_NULL) {
        /* it's a latch */
        sccMgr->sccLatchCnt[sccCnt] = 1;
        sccWithLatchCnt++;
      }
      sccMgr->mapScc[i] = sccCnt++;
    }
  }

  for (i=0; i<sccMgr->nScc; i++) {
    sccMgr->sccOutCnt[i] = 0;
    sccMgr->sccInCnt[i] = 0;
    sccMgr->sccTopologicalLevel[i] = 0;
    sccMgr->sccGateTopologicalLevelMin[i] = 0;
    sccMgr->sccGateTopologicalLevelMax[i] = 0;
  }
  for (i=0; i<sccMgr->nLatches; i++) {
    sccMgr->sccLatchTopologicalLevel[i] = 0;
  }

  for (i=0; i<sccMgr->sccTopologicalNodes->num; i++) {
    bAigEdge_t baigLdr, baig = sccMgr->sccTopologicalNodes->nodes[i];
    bAigEdge_t d = bAig_CacheAig(bmgr,baig);
    if (d != bAig_NULL) {
      /* latch, link to delta baig */
      int iD = bAig_AuxInt(bmgr,d);
      if (iD>=0) { // if -1: constant
	Pdtutil_Assert(iD>=0,"wrong aux int field");
	sccMgr->mapNs[i] = iD;
      }
    }
    Pdtutil_Assert(bAig_AuxAig1(bmgr,baig)!=bAig_NULL,
                   "wrong aux aig1 field");
    baigLdr = bAig_AuxAig1(bmgr,baig);
    if (baigLdr != bAig_NonInvertedEdge(baig)) {
      /* scc leader */
      int iLdr = bAig_AuxInt(bmgr,baigLdr);
      int sccId;
      Pdtutil_Assert(iLdr>=0 && iLdr<sccMgr->nNodes && 
                     sccMgr->mapScc[iLdr]>=0, "error in scc mapping");
      sccId = sccMgr->mapScc[i] = sccMgr->mapScc[iLdr];
      if (bAig_CacheAig(bmgr,baig)!=bAig_NULL) {
        /* it's a latch */
        sccMgr->sccLatchCnt[sccId]++;
      }
      sccMgr->sccGateCnt[sccId]++;
    }
    //    bAig_CacheAig(bmgr,baig) = bAig_NULL;
  }
  

  for (i=0; i<sccMgr->sccTopologicalNodes->num; i++) {
    bAigEdge_t baig = sccMgr->sccTopologicalNodes->nodes[i];
    bAigEdge_t baigLdr = bAig_AuxAig1(bmgr,baig);
    int iLdr = bAig_AuxInt(bmgr,baigLdr);
    int level = 0, levelGate = 0;
    int iScc = sccMgr->mapScc[iLdr];
    if(!bAig_isVarNode(bmgr, baig)){
      bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
      bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);
      int ir = bAig_AuxInt(bmgr,right);
      int il = bAig_AuxInt(bmgr,left);
      int lgr=0, lr=0, iSccr = sccMgr->mapScc[ir];
      int lgl=0, ll=0, iSccl = sccMgr->mapScc[il];
      sccMgr->sccGateFoCnt[ir]++;
      sccMgr->sccGateFoCnt[il]++;
      if (iSccr!=iScc) { 
	lr = sccMgr->sccTopologicalLevel[iSccr]+1;
	lgr = sccMgr->sccGateTopologicalLevelMin[iSccr];
	if (sccMgr->sccLatchCnt[iSccr]>0) {
	  lgr++; // latch: increment
	}
        if (!sccMgr->sccOutGate[ir]) {
          sccMgr->sccOutCnt[iSccr]++;
        }
        sccMgr->sccOutGate[ir]++;
        if (!sccMgr->sccInGate[i]) {
          sccMgr->sccInCnt[iScc]++;
        }
        sccMgr->sccInGate[i]++;
      }
      if (iSccl!=iScc) {
	ll = sccMgr->sccTopologicalLevel[iSccl]+1;
	lgl = sccMgr->sccGateTopologicalLevelMin[iSccl];
	if (sccMgr->sccLatchCnt[iSccl]>0) {
	  lgl++; // latch: increment
	}
        if (!sccMgr->sccOutGate[il]) {
          sccMgr->sccOutCnt[iSccl]++;
        }
        sccMgr->sccOutGate[il]++;
        if (!sccMgr->sccInGate[i]) {
          sccMgr->sccInCnt[iScc]++;
        }
        sccMgr->sccInGate[i]++;
      }
      level = (lr>ll ? lr:ll);
      levelGate = (lgr>lgl ? lgr:lgl); 
    }
    else {
      bAigEdge_t d = bAig_CacheAig(bmgr,baig);
      if (d != bAig_NULL) {
	/* latch, link to delta baig */
	int iD = bAig_AuxInt(bmgr,d);
	if (iD>=0) { // if -1: constant
	  Pdtutil_Assert(iD>=0,"wrong aux int field");
	  int iSccIn = sccMgr->mapScc[iD];
          sccMgr->sccGateFoCnt[iD]++;
	  if (iSccIn!=iScc) {
	    level = sccMgr->sccTopologicalLevel[iSccIn]+1;
	    levelGate = sccMgr->sccGateTopologicalLevelMin[iSccIn];
	    if (sccMgr->sccLatchCnt[iSccIn]>0) {
	      levelGate++; // latch: increment
	    }
            if (!sccMgr->sccOutGate[iD]) {
              sccMgr->sccOutCnt[iSccIn]++;
            }
            sccMgr->sccOutGate[iD]++;
            if (!sccMgr->sccInGate[i]) {
              sccMgr->sccInCnt[iScc]++;
            }
            sccMgr->sccInGate[i]++;
	  }
	}
      }
    }
    if (level > sccMgr->sccTopologicalLevel[iScc]) {
      sccMgr->sccTopologicalLevel[iScc] = level;
    }
    if (levelGate > sccMgr->sccGateTopologicalLevelMin[iScc]) {
      sccMgr->sccGateTopologicalLevelMin[iScc] = levelGate;
    }
    //    bAig_AuxAig0(bmgr,baig) = bAig_NULL;
    //    bAig_AuxAig1(bmgr,baig) = bAig_NULL;
    //    bAig_CacheAig(bmgr,baig) = bAig_NULL;
  }

  for (i=0; i<sccMgr->sccTopologicalNodes->num; i++) {
    bAigEdge_t baig = sccMgr->sccTopologicalNodes->nodes[i];
    bAigEdge_t baigLdr = bAig_AuxAig1(bmgr,baig);
    int iLdr = bAig_AuxInt(bmgr,baigLdr);
    int iScc = sccMgr->mapScc[iLdr];
    int levelGate = sccMgr->sccGateTopologicalLevelMin[iScc];
    if(!bAig_isVarNode(bmgr, baig)){
      bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
      bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);
      int ir = bAig_AuxInt(bmgr,right);
      int il = bAig_AuxInt(bmgr,left);
      int lgr=0, lr=0, iSccr = sccMgr->mapScc[ir];
      int lgl=0, ll=0, iSccl = sccMgr->mapScc[il];
      if (iSccr!=iScc) { 
	bAigEdge_t d = bAig_CacheAig(bmgr,right);
	if (d == bAig_NULL) {
	  int levelGateIn = sccMgr->sccGateTopologicalLevelMax[iSccr];
	  /* GATE: compute max level */
	  if (levelGateIn<levelGate) {
	    sccMgr->sccGateTopologicalLevelMax[iSccr] = levelGate;
	  }
	}
      }
      if (iSccl!=iScc) { 
	bAigEdge_t d = bAig_CacheAig(bmgr,left);
	if (d == bAig_NULL) {
	  /* GATE: compute max level */
	  int levelGateIn = sccMgr->sccGateTopologicalLevelMax[iSccl];
	  if (levelGateIn<levelGate) {
	    sccMgr->sccGateTopologicalLevelMax[iSccl] = levelGate;
	  }
	}
      }
    }
    else {
      bAigEdge_t d = bAig_CacheAig(bmgr,baig);
      if (d != bAig_NULL && bAig_CacheAig(bmgr,d) == bAig_NULL) {
	/* latch with gate input */
	int iD = bAig_AuxInt(bmgr,d);
	if (iD>=0) { // if -1: constant
	  Pdtutil_Assert(iD>=0,"wrong aux int field");
	  int iSccIn = sccMgr->mapScc[iD];
	  if (iSccIn!=iScc) {
	    int levelGateIn = sccMgr->sccGateTopologicalLevelMax[iSccIn];
	    if (levelGateIn<levelGate) {
	      sccMgr->sccGateTopologicalLevelMax[iSccIn] = levelGate;
	    }
	  }
	}
      }
    }
  }

  for (i=0; i<sccMgr->sccTopologicalNodes->num; i++) {
    bAigEdge_t baig = sccMgr->sccTopologicalNodes->nodes[i];
    if (bAig_isVarNode(bmgr, baig)) { 
      Ddi_Var_t *v = Ddi_VarFromBaig(ddm, baig);
      char *name = Ddi_VarName(v);
      if (bAig_CacheAig(bmgr,baig) == bAig_NULL) {
	// PI: not a latch
	int id = bAig_AuxInt(bmgr,baig);
	int iScc = sccMgr->mapScc[id];
	int levelGateMin = sccMgr->sccGateTopologicalLevelMin[iScc];
	int levelGateMax = sccMgr->sccGateTopologicalLevelMax[iScc];
	if (0 && levelGateMax>levelGateMin+1) 
	  printf("PI %20s: level Gate %d - %d\n",
	       name, levelGateMin, levelGateMax);
      }
      else {
	int id = bAig_AuxInt(bmgr,baig);
	int iScc = sccMgr->mapScc[id];
        if (sccMgr->sccLatchCnt[iScc]==1 &&
            sccMgr->sccGateCnt[iScc]==1) {
          bAigEdge_t d = bAig_CacheAig(bmgr,baig);
          int iD = bAig_AuxInt(bmgr,d);
          char isVar = bAig_isVarNode(bmgr, d);
          int levelGate = sccMgr->sccGateTopologicalLevelMin[iScc];
          int levelGate1 = sccMgr->sccGateTopologicalLevelMax[iScc];
          if (0)
          printf("1 latch Scc (size: %d) - fo of pi node%s: %d (level: %d-%d)\n",
                sccMgr->sccGateCnt[iScc],
                 isVar?" (V)":"", sccMgr->sccGateFoCnt[iD],
                 levelGate, levelGate1);
        }
      }
    }
    else {
      // gate
      int id = bAig_AuxInt(bmgr,baig);
      int iScc = sccMgr->mapScc[id];
      int levelGateMin = sccMgr->sccGateTopologicalLevelMin[iScc];
      int levelGateMax = sccMgr->sccGateTopologicalLevelMax[iScc];
      if (0 && levelGateMax>levelGateMin+1) 
	printf("GATE: level Gate %d - %d\n",
	       levelGateMin, levelGateMax);
    }
  }

  for (i=0; i<sccMgr->sccTopologicalNodes->num; i++) {
    bAigEdge_t baig = sccMgr->sccTopologicalNodes->nodes[i];
    bAig_AuxAig0(bmgr,baig) = bAig_NULL;
    bAig_AuxAig1(bmgr,baig) = bAig_NULL;
    bAig_CacheAig(bmgr,baig) = bAig_NULL;
    //    bAig_AuxInt(bmgr,baig) = -1;
  }

  for (i=0; i<Ddi_VararrayNum(ps); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(ps,i);
    Ddi_Bdd_t *d = Ddi_BddarrayRead(delta,i);
    bAigEdge_t varIndex = Ddi_VarToBaig(v);
    bAig_CacheAig(bmgr,varIndex) = bAig_NULL;
  }

  for (i=currBit=0; i<sccMgr->nScc; i++) {
    if (sccMgr->sccLatchCnt[i]>0) {
      sccMgr->mapSccBit[i] = currBit++;
    }
    else {
      sccMgr->mapSccBit[i] = -1;
    }
  }

  int level0LatchCnt=0;
  for (i=0; i<Ddi_VararrayNum(ps); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(ps,i);
    bAigEdge_t varIndex = Ddi_VarToBaig(v);
    int map, bit, level, id = bAig_AuxInt(bmgr,varIndex);
    if (id<0) {
      /* out of support */
      sccMgr->mapLatches[i]=-1;
    }
    else {
      Pdtutil_Assert(id>=0&&id<sccMgr->nNodes,"wrong aux int");
      map = sccMgr->mapScc[id];
      Pdtutil_Assert(map>=0&&map<sccMgr->nScc,"wrong scc id");
      bit = sccMgr->mapSccBit[map];
      level = sccMgr->sccTopologicalLevel[map];
      if (sccMgr->sccGateTopologicalLevelMin[map]==0 &&
	  sccMgr->sccLatchCnt[map]==1) {
        //	printf("level 0 latch[%d]: %s (scc id: %d)\n", 
        //	       i, Ddi_VarName(v), map);
        level0LatchCnt++;
      }
      if (0) {
        char *names[]={"l238","l1352","l248",
                       "l250","l252","l836",NULL};
        int j;
        for (j=0; names[j]!=NULL; j++) {
          if (strcmp(Ddi_VarName(v),names[j])==0) {
            printf("found %s(%d) -scc bit %d - map %d - level %d - latch cound: %d\n",
                   Ddi_VarName(v), i, bit, map,
                   level, sccMgr->sccLatchCnt[map]); break;
          }
        }
      }
      
      Pdtutil_Assert(bit>=0&&bit<currBit,"wrong scc bit");
      sccMgr->mapLatches[i]=bit;
      sccMgr->sccLatchTopologicalLevel[i]=level;
    }
  }

  /* check scc size count */
  if (chkSize) {
    int cntL=0, cntL1=0, cntL1plus=0, cntG=0; 
    float avgLatchSize=0.0;
    float avgGateSize=0.0;
    float avgL1GateSize=0.0;
    Pdtutil_Assert(sccCnt==sccMgr->nScc,"wrong scc count");
    for (i=0; i<sccMgr->nScc; i++) {
      if (sccMgr->sccLatchCnt[i]==0) {
        cntG++;
      }
      else if (sccMgr->sccLatchCnt[i]==1) {
	int levelGate = sccMgr->sccGateTopologicalLevelMin[i];
        //     	printf("1 latch Scc (size: %d) - gate level: %d\n",
        //               sccMgr->sccGateCnt[i], levelGate);
        if (sccMgr->sccGateCnt[i]==1) {
          cntL1++;
        }
        else {
          cntL1plus++;
          avgL1GateSize += sccMgr->sccGateCnt[i];
        }
      }
      else {
	int levelGate = sccMgr->sccGateTopologicalLevelMin[i];
        if (0)
      	printf("latch Scc - L/G num: %d/%d - gate level: %d (out Gates: %d / in Gates: %d)\n", 
       	       sccMgr->sccLatchCnt[i], sccMgr->sccGateCnt[i], levelGate,
               sccMgr->sccOutCnt[i], sccMgr->sccInCnt[i]);
        avgLatchSize += sccMgr->sccLatchCnt[i];
        avgGateSize += sccMgr->sccGateCnt[i];
        cntL++;
      }
    }
    avgLatchSize /= cntL;
    avgGateSize /= cntL;
    avgL1GateSize /= cntL1plus;
    Pdtutil_Assert(sccCnt==(cntG+cntL+cntL1+cntL1plus),"wrong scc count");
    Pdtutil_Assert(sccMgr->nSccWithLatch==(cntL+cntL1+cntL1plus),
                   "wrong scc count");
    printf("#SCC with latch: %d(1) + %d(1p)(avg:%f) + %d(avg:%f/%f)\n", 
           cntL1, cntL1plus, avgL1GateSize, cntL, avgLatchSize, avgGateSize);
    printf("#LEVEL 0 1latch: %d\n", level0LatchCnt);
  }

  //  sccMgrFree(sccMgr);

  return sccMgr;
}


void 
Ddi_FindSccBridgesNaive(
  Ddi_SccMgr_t *sccMgr,
  Ddi_Bddarray_t *delta,
  Ddi_Bddarray_t *lambda,
  Ddi_Vararray_t *ps
)
{
  int i;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(delta);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  Ddi_SccMgr_t *newSccMgr;
  int *mapScc = sccMgr->mapScc;
  bAig_array_t *aigNodes = sccMgr->sccTopologicalNodes; //TODO: topological nodes o aigNodes?
  
  sccMgr->sccBridges = Pdtutil_Alloc(int, sccMgr->sccTopologicalNodes->num);
  sccMgr->sccBridgesCnt = Pdtutil_Alloc(int, sccMgr->nScc);
  for(i=0; i<sccMgr->nScc; i++){
    sccMgr->sccBridgesCnt[i] = 0;
  }
  
  //Scansione delle scc
  for(i=0; i<aigNodes->num; i++){
    bAigEdge_t node = aigNodes->nodes[i];
    int iScc = mapScc[i];
    
    //Disabilitazione arco
    int iD = sccMgr->mapNs[i];
    if (iD != -1) {
      //Latch
      nodeAuxChar(bmgr, node) |= Ddi_SccNodeStatus_RightDisabled_c;
      
      //Ricalcolo scc
      newSccMgr = Ddi_FsmSccTarjanCheckBridge(delta, lambda, ps, node, 0);
      
      
      
      //Verifico se l'arco tolto era un bridge
      if(newSccMgr->nScc > sccMgr->nScc){
        //Bridge
        sccMgr->sccBridges[i] = Ddi_SccBridge_RightBridge_c;
        sccMgr->sccBridgesCnt[iScc]++;
        
        FILE *outfile = fopen("bridge_split.txt", "a");
        fprintf(outfile, "%d ", newSccMgr->nScc - sccMgr->nScc);
        char *sccFlags = Pdtutil_Alloc(char, newSccMgr->nScc );
        int j, k;
        for(j=0; j<newSccMgr->nScc; j++) sccFlags[j] = 0;
        bAig_array_t *sccMembers = bAigArrayAlloc();
        int numMembers = 0;
        for(j=0; j<aigNodes->num; j++){
          if(sccMgr->mapScc[j] == iScc){
            numMembers++;
            bAigArrayWriteLast(sccMembers, aigNodes->nodes[j]);
          }
        }
        
        int n=0;
        for(j=0; j<newSccMgr->nNodes; j++){
          bAigEdge_t baig = newSccMgr->aigNodes->nodes[j];
          for(k=0; k<numMembers; k++){
            bAigEdge_t member = sccMembers->nodes[k];
            if(baig == member){
              int newSccId = newSccMgr->mapScc[j];
              if(sccFlags[newSccId] == 0){
                sccFlags[newSccId] = 1;
                n++;
                fprintf(outfile, "%d ", newSccMgr->sccSize[newSccId]);
              }
              break;
            }
          }
        }
        fprintf(outfile, "\n");
        fclose(outfile);
        
        bAigArrayFree(sccMembers);
        Pdtutil_Free(sccFlags);
        //Pdtutil_Assert(n == newSccMgr->nScc - sccMgr->nScc+1, "Wrong number of Scc.");
      }
      
      sccMgrFree(newSccMgr);
    }
    else if(!bAig_isVarNode(bmgr, node)){
      //Disabilito arco figlio destro
      nodeAuxChar(bmgr, node) |= Ddi_SccNodeStatus_RightDisabled_c;
      
      //Ricalcolo scc
      newSccMgr = Ddi_FsmSccTarjanCheckBridge(delta, lambda, ps, node, 1);
      
      //Verifico se l'arco tolto era un bridge
      if(newSccMgr->nScc > sccMgr->nScc){
        //Bridge
        sccMgr->sccBridges[i] = Ddi_SccBridge_RightBridge_c;
        sccMgr->sccBridgesCnt[iScc]++;
        
        FILE *outfile = fopen("bridge_split.txt", "a");
        fprintf(outfile, "%d ", newSccMgr->nScc - sccMgr->nScc);
        char *sccFlags = Pdtutil_Alloc(char, newSccMgr->nScc );
        int j, k;
        for(j=0; j<newSccMgr->nScc; j++) sccFlags[j] = 0;
        bAig_array_t *sccMembers = bAigArrayAlloc();
        int numMembers=0;
        for(j=0; j<aigNodes->num; j++){
          if(sccMgr->mapScc[j] == iScc){
            numMembers++;
            bAigArrayWriteLast(sccMembers, aigNodes->nodes[j]);
          }
        }
        
        int n=0;
        for(j=0; j<newSccMgr->nNodes; j++){
          bAigEdge_t baig = newSccMgr->aigNodes->nodes[j];
          for(k=0; k<numMembers; k++){
            bAigEdge_t member = sccMembers->nodes[k];
            if(baig == member){
              int newSccId = newSccMgr->mapScc[j];
              if(sccFlags[newSccId] == 0){
                sccFlags[newSccId] = 1;
                n++;
                fprintf(outfile, "%d ", newSccMgr->sccSize[newSccId]);
              }
              break;
            }
          }
        }
        fprintf(outfile, "\n");
        fclose(outfile);
        
        bAigArrayFree(sccMembers);
        Pdtutil_Free(sccFlags);
        //Pdtutil_Assert(n == newSccMgr->nScc - sccMgr->nScc +1, "Wrong number of Scc.");
      }
      
      Pdtutil_Assert(IsRightEnabled(bmgr, node), "Right disabled");
      
      //Disabilito arco figlio sinistro
      nodeAuxChar(bmgr, node) |= Ddi_SccNodeStatus_LeftDisabled_c;
      
      //Ricalcolo scc
      newSccMgr = Ddi_FsmSccTarjanCheckBridge(delta, lambda, ps, node, 1);
      
      Pdtutil_Assert(IsLeftEnabled(bmgr, node), "Left disabled");
      
      //Verifico se l'arco tolto era un bridge
      if(newSccMgr->nScc > sccMgr->nScc){
        //Bridge
        if(sccMgr->sccBridges[i] == Ddi_SccBridge_RightBridge_c){
          sccMgr->sccBridges[i] = Ddi_SccBridge_BothBridge_c;
        } else {
          sccMgr->sccBridges[i] = Ddi_SccBridge_LeftBridge_c;
        }
        sccMgr->sccBridgesCnt[iScc]++;
        
        FILE *outfile = fopen("bridge_split.txt", "a");
        fprintf(outfile, "%d ", newSccMgr->nScc - sccMgr->nScc);
        char *sccFlags = Pdtutil_Alloc(char, newSccMgr->nScc );
        int j, k;
        for(j=0; j<newSccMgr->nScc; j++) sccFlags[j] = 0;
        bAig_array_t *sccMembers = bAigArrayAlloc();
        int numMembers = 0;
        for(j=0; j<aigNodes->num; j++){
          if(sccMgr->mapScc[j] == iScc){
            numMembers++;
            bAigArrayWriteLast(sccMembers, aigNodes->nodes[j]);
          }
        }
        
        int n=0;
        for(j=0; j<newSccMgr->nNodes; j++){
          bAigEdge_t baig = newSccMgr->aigNodes->nodes[j];
          for(k=0; k<numMembers; k++){
            bAigEdge_t member = sccMembers->nodes[k];
            if(baig == member){
              int newSccId = newSccMgr->mapScc[j];
              if(sccFlags[newSccId] == 0){
                sccFlags[newSccId] = 1;
                n++;
                fprintf(outfile, "%d ", newSccMgr->sccSize[newSccId]);
              }
              break;
            }
          }
        }
        fprintf(outfile, "\n");
        fclose(outfile);
        
        bAigArrayFree(sccMembers);
        Pdtutil_Free(sccFlags);
        //Pdtutil_Assert(n == newSccMgr->nScc - sccMgr->nScc+1, "Wrong number of Scc.");
      }
      
      sccMgrFree(newSccMgr);
    }
    
  }
    
  printf("Number of bridges: \n");
  for(i=0; i<sccMgr->nScc;i++){
    printf("%d --> %d\n", i, sccMgr->sccBridgesCnt[i]);
  }
    
}

/**Function********************************************************************
  Synopsis     []
  Description  []
  SideEffects  []
******************************************************************************/
Ddi_SccMgr_t *
Ddi_FsmSccTarjanCheckBridge (
  Ddi_Bddarray_t *delta,
  Ddi_Bddarray_t *lambda,
  Ddi_Vararray_t *ps,
  bAigEdge_t node,
  int isGate
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(delta);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  int i, currBit, sccCnt, sccWithLatchCnt, nL = Ddi_BddarrayNum(delta), 
    nO = Ddi_BddarrayNum(lambda);
  Ddi_SccMgr_t *sccMgr;
  int chkSize=1;

  sccMgr = sccMgrAlloc(delta,ps);

  for (i=0; i<Ddi_VararrayNum(ps); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(ps,i);
    Ddi_Bdd_t *d = Ddi_BddarrayRead(delta,i);
    bAigEdge_t varIndex = Ddi_VarToBaig(v);
    bAigEdge_t dBaig = Ddi_BddToBaig(d);
    Pdtutil_Assert(bAig_CacheAig(bmgr,varIndex)==bAig_NULL,
                   "invalid cache aig");
    bAig_CacheAig(bmgr,varIndex) = dBaig;
  }

  for (i=0; i<nL; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(delta,i);
    bAigEdge_t fBaig = Ddi_BddToBaig(f_i);
    if (!bAig_NodeIsConstant(fBaig)) {
      fsmSccTarjanCheckBridgeIntern(sccMgr,fBaig);
    }
  }
  for (i=0; i<nO; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(lambda,i);
    if (!Ddi_BddIsConstant(f_i)) {
      bAigEdge_t fBaig = Ddi_BddToBaig(f_i);
      fsmSccTarjanCheckBridgeIntern(sccMgr,fBaig);
    }
  }

  if(!isGate){
    //Latch
    bAigEdge_t d = bAig_CacheAig(bmgr,node);
    
    if(d != bAig_NULL && !nodeVisited(bmgr, d)){
      fsmSccTarjanCheckBridgeIntern(sccMgr,d);
    }
  } else {
    //Gate
    bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr,node);
    bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(bmgr,node);
    
    if(right != bAig_NULL && !IsRightEnabled(bmgr,node) && !nodeVisited(bmgr, right)){
      fsmSccTarjanCheckBridgeIntern(sccMgr,right);
    } else if(left != bAig_NULL && !IsLeftEnabled(bmgr,node) && !nodeVisited(bmgr, left)){
      fsmSccTarjanCheckBridgeIntern(sccMgr,left);
    }
  }
   
   
  DdiPostOrderAigClearVisitedIntern(bmgr,sccMgr->aigNodes);
   
  for (i=0; i<nL; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(delta,i);
    if (!Ddi_BddIsConstant(f_i)) {
      bAigEdge_t fBaig = Ddi_BddToBaig(f_i);
      postOrderAigVisitInternWithScc(bmgr, fBaig, 
                                     sccMgr->sccTopologicalNodes);
    }
  }
  for (i=0; i<nO; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(lambda,i);
    if (!Ddi_BddIsConstant(f_i)) {
      bAigEdge_t fBaig = Ddi_BddToBaig(f_i);
      postOrderAigVisitInternWithScc(bmgr, fBaig, 
                                     sccMgr->sccTopologicalNodes);
    }
  }
  DdiPostOrderAigClearVisitedIntern(bmgr,sccMgr->sccTopologicalNodes);
  
  sccMgr->nNodes = sccMgr->sccTopologicalNodes->num;
  sccMgr->sccSize = Pdtutil_Alloc(int, sccMgr->nScc);
  sccMgr->mapScc = Pdtutil_Alloc(int, sccMgr->nNodes);
  sccCnt = 0;
  for (i=0; i<sccMgr->aigNodes->num; i++) {
    bAigEdge_t baig = sccMgr->aigNodes->nodes[i];
    sccMgr->mapScc[i] = -1;
    if (bAig_AuxAig1(bmgr,baig) == bAig_NonInvertedEdge(baig)) {
          /* scc leader */
          sccMgr->mapScc[i] = sccCnt++;
    }
  }
  
  for (i=0; i<sccMgr->aigNodes->num; i++) {
    bAigEdge_t baigLdr, baig = sccMgr->aigNodes->nodes[i];

    baigLdr = bAig_AuxAig1(bmgr,baig);
    if (baigLdr != bAig_NonInvertedEdge(baig)) {
      /* scc leader */
      int iLdr = bAig_AuxInt(bmgr,baigLdr);
      Pdtutil_Assert(iLdr>=0 && iLdr<sccMgr->nNodes && sccMgr->mapScc[iLdr]>=0, "error in scc mapping");
      sccMgr->mapScc[i] = sccMgr->mapScc[iLdr];
    }
    bAig_AuxAig0(bmgr,baig) = bAig_NULL;
    bAig_AuxAig1(bmgr,baig) = bAig_NULL;
    bAig_CacheAig(bmgr,baig) = bAig_NULL;
  }
  
  for (i=0; i<sccMgr->nScc; i++) {
    sccMgr->sccSize[i] = 0;
  }
    
  for (i=0; i<sccMgr->aigNodes->num; i++) {
    int iScc = sccMgr->mapScc[i];
    sccMgr->sccSize[iScc]++;
  }
  
  if(!isGate){
    //Latch
    bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr,node);
    int rLdr = bAig_AuxInt(bmgr,right);
    int nLdr = bAig_AuxInt(bmgr,node);
    
    int rScc = sccMgr->mapScc[rLdr];
    int nScc = sccMgr->mapScc[nLdr];
    
    int rSizeScc = sccMgr->sccSize[rScc];
    int nSizeScc = sccMgr->sccSize[nScc];
    sccMgr->size1 = rSizeScc;
    sccMgr->size2 = nSizeScc;

  } else {
    //Gate
    bAigEdge_t right = bAig_NodeReadIndexOfRightChild(bmgr,node);
    bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(bmgr,node);
    
    char auxChar = nodeAuxChar(bmgr,node);
    int isRightDisabled = !IsRightEnabled(bmgr,node);
    if(right != bAig_NULL && isRightDisabled){
      int rLdr = bAig_AuxInt(bmgr,right);
      int nLdr = bAig_AuxInt(bmgr,node);
      
      int rScc = sccMgr->mapScc[rLdr];
      int nScc = sccMgr->mapScc[nLdr];
      
      int rSizeScc = sccMgr->sccSize[rScc];
      int nSizeScc = sccMgr->sccSize[nScc];
      
      sccMgr->size1 = rSizeScc;
      sccMgr->size2 = nSizeScc;
    } else if(left != bAig_NULL && !IsLeftEnabled(bmgr,node)){
      int lLdr = bAig_AuxInt(bmgr,left);
      int nLdr = bAig_AuxInt(bmgr,node);
      
      int lScc = sccMgr->mapScc[lLdr];
      int nScc = sccMgr->mapScc[nLdr];
      
      int lSizeScc = sccMgr->sccSize[lScc];
      int nSizeScc = sccMgr->sccSize[nScc];
      
      sccMgr->size1 = lSizeScc;
      sccMgr->size2 = nSizeScc;
    }
  }

  for (i=0; i<Ddi_VararrayNum(ps); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(ps,i);
    Ddi_Bdd_t *d = Ddi_BddarrayRead(delta,i);
    bAigEdge_t varIndex = Ddi_VarToBaig(v);
    bAig_CacheAig(bmgr,varIndex) = bAig_NULL;
  }

  
  for (i=0; i<sccMgr->aigNodes->num; i++) {
    bAigEdge_t baig = sccMgr->aigNodes->nodes[i];
    bAig_AuxInt1(bmgr,baig) = -1;
    bAig_AuxInt(bmgr,baig) = -1;
    nodeAuxChar(bmgr, baig) = 0;
  }
  
  return sccMgr;
}



/**Function********************************************************************
  Synopsis     []
  Description  []
  SideEffects  []
******************************************************************************/
Ddi_Varsetarray_t *
Ddi_AigEvalMultipleCoi (
  Ddi_Bddarray_t *delta,
  Ddi_Bddarray_t *outputs,
  Ddi_Vararray_t *ps,
  int optionCluster,
  char *clusterFile,
  int methodCluster,
  int thresholdCluster 
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(delta);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  int i, nRoots, nStateVars, nOutputs;
  int *mapPs, *mapNs;
  bAig_array_t *visitedNodes;
  Ddi_AigDynSignatureArray_t *aigSigs, *varSigs;
  Ddi_Varsetarray_t *cois;
  Ddi_Vararray_t *psSorted=NULL;
  Ddi_Varset_t *psVarset=NULL;
  int thresholdFinal = 1;

  nStateVars = Ddi_VararrayNum(ps);
  
  Pdtutil_Assert(nStateVars==Ddi_BddarrayNum(delta),"error in array num");
  nOutputs = Ddi_BddarrayNum(outputs); 
  //printf("%d\n",nOutputs);
  //printf("%d\n",nStateVars);
  visitedNodes = bAigArrayAlloc();

  mapPs = Pdtutil_Alloc(int, nStateVars);
  mapNs = Pdtutil_Alloc(int, nStateVars);

  Ddi_MgrSiftSuspend(ddm);

  for (i=0; i<nStateVars; i++) {
    Ddi_Var_t *psVar = Ddi_VararrayRead(ps, i);
    bAigEdge_t psBaig = Ddi_VarToBaig(psVar);
    DdiPostOrderAigVisitIntern(bmgr,psBaig,visitedNodes,-1);
  }

  for (i=nRoots=0; i<Ddi_BddarrayNum(delta); i++) {
    Ddi_Bdd_t *fAig = Ddi_BddarrayRead(delta, i);
    bAigEdge_t fBaig = Ddi_BddToBaig(fAig);
    DdiPostOrderAigVisitIntern(bmgr,fBaig,visitedNodes,-1);
    nRoots++;
  }
  Pdtutil_Assert(nRoots==Ddi_BddarrayNum(delta),"wrong num of roots");

  for (i=nRoots=0; i<Ddi_BddarrayNum(outputs); i++) {
    Ddi_Bdd_t *fAig = Ddi_BddarrayRead(outputs, i);
    bAigEdge_t fBaig = Ddi_BddToBaig(fAig);
    DdiPostOrderAigVisitIntern(bmgr,fBaig,visitedNodes,-1);
    nRoots++;
  }
  Pdtutil_Assert(nRoots==Ddi_BddarrayNum(outputs),"wrong num of roots");

  DdiPostOrderAigClearVisitedIntern(bmgr,visitedNodes);

  for (i=0; i<visitedNodes->num; i++) {
    bAigEdge_t baig = visitedNodes->nodes[i];
    Pdtutil_Assert(bAig_AuxInt(bmgr,baig)<0, "wrong auxId field");
    bAig_AuxInt(bmgr,baig) = i;
  }

  varSigs = DdiAigDynSignatureArrayAlloc(nStateVars,nStateVars);
  DdiAigDynSignatureArrayInit(varSigs);

  for (i=0; i<nStateVars; i++) {
    Ddi_Var_t *psVar = Ddi_VararrayRead(ps, i);
    Ddi_Bdd_t *dAig = Ddi_BddarrayRead(delta, i);
    bAigEdge_t psBaig = Ddi_VarToBaig(psVar);
    bAigEdge_t dBaig = Ddi_BddToBaig(dAig);

    mapPs[i] = bAig_AuxInt(bmgr,psBaig);
    mapNs[i] = bAig_AuxInt(bmgr,dBaig);
  }

  aigSigs = Ddi_AigEvalVarSignatures (ddm,visitedNodes,
				      varSigs,mapPs,mapNs,nStateVars);

  cois = Ddi_VarsetarrayAlloc(ddm,nOutputs);

  for (i=0; i<nStateVars; i++) {
    Ddi_Var_t *psVar = Ddi_VararrayRead(ps, i);
    Pdtutil_Assert(Ddi_VarReadMark(psVar) == 0,"0 var mark required");
    Ddi_VarWriteMark(psVar,i);
  }
  psVarset = Ddi_VarsetMakeFromArray(ps);
  psSorted = Ddi_VararrayMakeFromVarset(psVarset,1); // sorted by ord
  Ddi_Free(psVarset);
	

  Ddi_Vararray_t *vA/*,**vACluster*/;
  Ddi_Var_t *v;
  double **data;
  int **mask;
  if(optionCluster){
    //vACluster = Pdtutil_Alloc(Ddi_Vararray_t *,Ddi_BddarrayNum(outputs));
    data = Pdtutil_Alloc(double *,Ddi_BddarrayNum(outputs));
    mask = Pdtutil_Alloc(int *,Ddi_BddarrayNum(outputs));  
    int j;
    printf("noutputs %d\n",Ddi_BddarrayNum(outputs));
    printf("nvars %d\n",Ddi_VararrayNum(psSorted));
    for(i = 0;i < Ddi_BddarrayNum(outputs);i++){
      data[i] = Pdtutil_Alloc(double,Ddi_VararrayNum(psSorted));
      mask[i] = Pdtutil_Alloc(int,Ddi_VararrayNum(psSorted));
      for(j = 0;j < Ddi_VararrayNum(psSorted);j++){
        data[i][j] = 0.0;
        mask[i][j] = 1;
      }
    }
  }
  
  for (i=0; i<Ddi_BddarrayNum(outputs); i++) {
		
    Ddi_Bdd_t *fAig = Ddi_BddarrayRead(outputs, i);
    bAigEdge_t fBaig = Ddi_BddToBaig(fAig);
    int j = bAig_AuxInt(bmgr,fBaig);
    Ddi_Varset_t *sigSupp = DdiDynSignatureToVarset(aigSigs->sArray[j],
						    psSorted);
    if(optionCluster){
      vA = Ddi_VararrayMakeFromVarset(sigSupp,0);
      //vACluster[i] = Ddi_VararrayDup(vA);
      for(j = 0;j < Ddi_VararrayNum(vA);j++){
        v = Ddi_VararrayRead(vA,j);
        int id = Ddi_VarReadMark(v);
        //printf("%d ",id);
        data[i][id] = 1.0;
      }
      //printf("\n");
      Ddi_Free(vA);
    }
    Ddi_VarsetarrayWrite(cois,i,sigSupp);
    Ddi_Free(sigSupp);
  }
	//printf("optionCluster %d\n",optionCluster);
	if(optionCluster){
		/*VARIABILI PER CLUSTERIZZAZIONE*/
		//double **data;
		long start = util_cpu_time();
		int *clusterid;
		double* weight = NULL;
		int row = 0,column = 0,j;
		Node *tree;	
		row = Ddi_BddarrayNum(outputs);
		column = 1/*Ddi_VararrayNum(psSorted)*/;
		/*VARIABILI PER CLUSTERIZZAZIONE*/

		/*ALLOCO MATRICE*/
		//data = (double **) malloc (sizeof(double *)*Ddi_BddarrayNum(outputs));
		//data = Pdtutil_Alloc(double *,Ddi_BddarrayNum(outputs));
		//mask = (int **) malloc (sizeof(int *)*Ddi_BddarrayNum(outputs));
		//mask = Pdtutil_Alloc(int *,Ddi_BddarrayNum(outputs));
		/*ALLOCO MATRICE*/
		
		/*for (i=0; i<row; i++) {
			//int pos = 0;
			//data[i] = Pdtutil_Alloc(double,Ddi_VararrayNum(psSorted));
			//mask[i] = Pdtutil_Alloc(int,Ddi_VararrayNum(psSorted));
			//for(j = 0; j<column; j++){data[i][j]=0.0;mask[i][j]=1;}
			for(j = 0;j < Ddi_VararrayNum(vACluster[i]);j++){
				Ddi_Var_t *v = Ddi_VararrayRead(vACluster[i], j);
				int id = Ddi_VarReadMark(v);
				data[i][id] = 1.0;
			}
		}*/
		/*CLUSTERIZZAZIONE*/
		printf("BEGIN CLUSTERIZATION\n");
		//weight = (double *) malloc(column*sizeof(double));
		int flagFile = 0;
  printf("row: %d\nthreshold: %d\n",row,thresholdCluster);
		if(thresholdCluster == 1){
			thresholdFinal = (row / 30) + 1; 
		}else{
			thresholdFinal = thresholdCluster;
		}
		int nCluster = row/thresholdFinal; /*default*/
		double error;
		int ifound = 0;
		//char *fileName = "out.txt";
		weight = Pdtutil_Alloc(double,column);
		for (i = 0; i < column; i++) weight[i] = 1.0;
		//clusterid =(int *) malloc(row*sizeof(int));
		clusterid = Pdtutil_Alloc(int,row);
  if (methodCluster == 1)
		  tree = treecluster(row, column, data, mask, weight, 0, 'e', 's', 0);
		if (row < nCluster){
				nCluster = row;
    if(methodCluster == 1)
				  cuttree (row, tree, nCluster, clusterid);
    else
      kcluster(nCluster,row,column,data,mask,weight,0,100,'a','e',clusterid,&error,&ifound);
		}else{
   if(methodCluster  == 1)
			  cuttree (row, tree, nCluster, clusterid);
   else
					kcluster(nCluster,row,column,data,mask,weight,0,100,'a','e',clusterid,&error,&ifound);
		}
		for(i = 0; i < row; i++){ Pdtutil_Free(data[i]);}
		Pdtutil_Free(data);
		FILE *fp;
		fp = fopen(clusterFile,"w");
		fprintf(fp,"%d\n",row);
		fprintf(fp,"%d\n",nCluster);
		//Pdtutil_FileOpen(fp,fileName,"w",&flagFile);
		for(i=0; i<row; i++){
		  //printf("Gene %2d: cluster %2d\n", i, clusterid[i]);
			//printf("bitmap %d: %d\n",i, clusterid[i]);
			fprintf(fp,"%2d %2d\n", i+1, clusterid[i]+1);
		}
		//printf("%d %d\n",row,column);
		//long end = util_cpu_time();
		//fprintf(fp,"FINE CLUSTERIZZAZIONE DURATA %s\n",util_print_time(end-start));
		printf("END CLUSTERIZATION\n");
		fclose(fp);		
		//Pdtutil_FileClose(fp,&flagFile);
		/*CLUSTERIZZAZIONE*/
	}	

  for (i=0; i<nStateVars; i++) {
    Ddi_Var_t *psVar = Ddi_VararrayRead(ps, i);
    Ddi_VarWriteMark(psVar,0);
  }
  Ddi_Free(psSorted);

  Ddi_MgrSiftResume(ddm);

  for (i=0; i<visitedNodes->num; i++) {
    bAigEdge_t baig = visitedNodes->nodes[i];
    Pdtutil_Assert(bAig_AuxInt(bmgr,baig)>=0, "wrong auxId field");
    bAig_AuxInt(bmgr,baig) = -1;
  }

	//printf("\n\nState vars :%d\n\n",Ddi_BddarrayNum(outputs));
			
  /*for (i=0; i<Ddi_BddarrayNum(outputs); i++) {
  			 	
  }*/
	//printf("\n\nState vars :%d\n\n",Ddi_BddarrayNum(outputs));
			
  /*for (i=0; i<Ddi_BddarrayNum(outputs); i++) {
  			 	
  }*/



  DdiAigDynSignatureArrayFree(aigSigs);
  DdiAigDynSignatureArrayFree(varSigs);

  Pdtutil_Free(mapPs);
  Pdtutil_Free(mapNs);

  bAigArrayFree(visitedNodes);

  return(cois);
}


/**Function********************************************************************
  Synopsis    [Return the i-th partition (conj/disj), and remove it from f.]
  Description [Return the i-th partition (conj/disj), and remove it from f.]
  SideEffects []
******************************************************************************/
static int *
BddarrayClusterByCoiAcc(
  Ddi_SccMgr_t *sccMgr,
  Ddi_Bddarray_t *fA,
  int min,
  int max,
  int th_coi_diff,//[default 0]    
  int th_coi_share, //[default 0] -> disabled 
  int methodCluster,
  int doSort
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(fA);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  int i, n = Ddi_BddarrayNum(fA);
  Pdtutil_SortInfo_t *SortInfoArray = Pdtutil_Alloc(Pdtutil_SortInfo_t,n);
  Ddi_AigDynSignatureArray_t *sccSigs = sccMgr->sccSigs;
  int amountClust = 1;
  Ddi_AigDynSignature_t *clustSig = DdiAigDynSignatureAlloc(sccSigs->nBits);
  int nCoiClust = 0;
  int clId = -1;
  int maxCoi, minCoi, tryCompact=1;
  int factor = max>0 ? n/max+1 : 1;
  int *bitmapIds = Pdtutil_Alloc(int,n);
  int *clusterIds = Pdtutil_Alloc(int,n);
  int forceClust2=0;

  Ddi_AigDynSignature_t **clusterSigs ;

  clusterSigs=Pdtutil_Alloc( Ddi_AigDynSignature_t * ,1);
  clusterSigs[0]=DdiAigDynSignatureAlloc(sccSigs->nBits); 

   
  int constClust[2] = {-1,-1};
  int nConst=0;
  int currentWeight=0;

  for (i=0; i<n; i++) {
    Ddi_Bdd_t *fAig = Ddi_BddarrayRead(fA, i);
    SortInfoArray[i].id = i;
    bitmapIds[i] = -1;
    if (Ddi_BddIsConstant(fAig)) {
      SortInfoArray[i].weight = 0.0;
    }
    else {
      bAigEdge_t fBaig = Ddi_BddToBaig(fAig);
      int jScc = bAig_AuxInt(bmgr,fBaig);
      int j = sccMgr->mapScc[jScc];
      SortInfoArray[i].weight = 
        (float) DdiAigDynSignatureCountBits(sccSigs->sArray[j],1);
      bitmapIds[i] = j;
    }
  }
  if (doSort)
    Pdtutil_InfoArraySort(SortInfoArray,n,1);

  minCoi = (int)SortInfoArray[0].weight;
  maxCoi = (int)SortInfoArray[n-1].weight;

  DdiAigDynSignatureClear(clustSig);

  clId = -1; // skip constant clusters
  /*for (i=0; i<n; i++) {
    printf("%1.1f ",SortInfoArray[i].weight);
  }
  printf("\n");*/
  int nCurrentCoi=0;
  
  for (i=0; i<n; i++) { 
    int k = SortInfoArray[i].id;
    int nClustCoi, nCoi_prev,nCoi = (int)SortInfoArray[i].weight;
    Ddi_Bdd_t *p_k = Ddi_BddarrayRead(fA,k);
    int changed;
    int doCluster = 1;    
    int sigId ;
    int prev_sigId;
    Ddi_AigDynSignature_t *mySig = NULL;
    Ddi_AigDynSignature_t *prevSig = NULL;
   
    int dist;
    int sh;
    int k_prev;

    if (i!=0)
      k_prev=SortInfoArray[i-1].id;
    else
      k_prev=k;
    sigId = bitmapIds[k];
    prev_sigId = bitmapIds[k_prev]; 
    if (sigId<0) {
      //      printf("CONSTANT\n");
      /* constant: leave it alone */
      int myCl = -1;
      int isOne = Ddi_BddIsOne(p_k)?1:0;
      forceClust2=1;
      if(methodCluster == 3){
        clId++;
        clusterIds[k]=clId;
        continue;
      }
      if (nConst==0) {
        constClust[0] = isOne;
        nConst=1;
        myCl = 0;
        clId++;
        amountClust = 1;
	//        printf("cluster: %d\n",clId);
      }
      else if (isOne==constClust[0]) {
        /* cluster already present */
	//        printf("cluster already present\n");
        amountClust++;
        myCl = 0;
      }
      else if (nConst==1) {
        constClust[1] = isOne;
        nConst=2;
        myCl = 1;
        clId++;
        amountClust = 1; 
        printf("cluster: %d\n",clId);
      }
      else {
        /* merge to clust 1 */
        myCl = 1;
      }

      clusterIds[k]=clId;
      DdiAigDynSignatureClear(clustSig);
    }
    else {
      switch(methodCluster) {
      case 0: 
							Pdtutil_Assert(sigId<sccMgr->nScc,"wrong sigId");
							mySig = sccSigs->sArray[sigId];
							changed = DdiAigDynSignatureUnion(clustSig, clustSig, mySig,0);
							if (changed) {
									DdiAigDynSignatureCountBits(clustSig,1);
							}
							if ((i%factor)==0){ doCluster = 0; }//printf("K:%d Factor:%d\n",k,factor);}
							//    if (k==0 || Ddi_VarsetNum(partCoi) > nCoi*coiRatio) {
							if (forceClust2) {
							  doCluster=0;
							  forceClust2 = 0;
							}
							if (!doCluster) {
         if (clId != -1) printf("cluster %d:%d \n",clId,amountClust);
         amountClust = 1;
									clId++;
									if (mySig!=NULL)
											DdiAigDynSignatureCopy(clustSig,mySig);
							}else{
								 amountClust++;
							}
							if(clId == -1) clId++;
							clusterIds[k]=clId;
							break;

      case 1:
        //Weak Cluster with thresholds --> ths set to 0 equivalent to case 2!
							Pdtutil_Assert(sigId<sccMgr->nScc,"wrong sigId");
							Pdtutil_Assert(prev_sigId<sccMgr->nScc,"wrong prev_sigId");

					

							//int th_coi_diff=0;  //[default 0]    
							//int th_coi_share=0; //[default 0] -> disabled 
												  
							/* comparing signature with previuos one (ones with COI size less then the current) */  
							sh=-1;
							mySig = sccSigs->sArray[sigId];
												

							dist=abs(nCurrentCoi - nCoi);
						

							if (th_coi_share>0)
									{ 
											prevSig = sccSigs->sArray[prev_sigId];      
											DdiAigDynSignatureClear(clustSig);       
											DdiAigDynSignatureIntersection(clustSig, prevSig, mySig,0);  

											sh = DdiAigDynSignatureCountBits(clustSig,1);

									}else
									{
											//disable bitmap sharing
											th_coi_share=1; 
									}  
							if (/*(k%factor)==0&&*/dist>th_coi_diff&&sh<(nCoi/th_coi_share)) doCluster = 0;
							//    if (k==0 || nClustCoi > nCoi*coiRatio) {
							if (!doCluster) {
									//add new cluster 
									clId++;
									nCurrentCoi=SortInfoArray[i].weight; 
							}
							if (clId==-1){
									clId++;
							}
							clusterIds[k]=clId;   

							break;

      case 2: 

        // weak cluster without thresholds 
        Pdtutil_Assert(sigId<sccMgr->nScc,"wrong sigId");
								mySig = sccSigs->sArray[sigId];
														 //printf("cluster\n;");
							 if(currentWeight != (int) SortInfoArray[i].weight){
         if (clId != -1) printf("cluster %d:%d \n",clId,amountClust);
         amountClust = 1;
									clId++;
									currentWeight = (int) SortInfoArray[i].weight;
							 }else{
          amountClust++;
								}
								if(clId == -1) clId++;
								clusterIds[k]=clId;
								break;
      case 3:
        Pdtutil_Assert(sigId<sccMgr->nScc,"wrong sigId");
        clId++;
        if(clId == -1) clId++;
        //printf("cluster %d",clId);
        clusterIds[k]=clId;

      }// end switch metodcluster

    }
  }
  if ((methodCluster == 0) || (methodCluster == 2)){
    printf("cluster %d:%d \n",clId,amountClust);
  }
  printf("\n");
  Pdtutil_Free(clustSig);
  Pdtutil_Free(SortInfoArray);
  Pdtutil_Free(bitmapIds);

  return clusterIds;
}

/**Function********************************************************************
  Synopsis    [Return the i-th partition (conj/disj), and remove it from f.]
  Description [Return the i-th partition (conj/disj), and remove it from f.]
  SideEffects []
******************************************************************************/
static Ddi_AigDynSignatureArray_t *
BddarrayClusterProperties(
  Ddi_SccMgr_t *sccMgr,
  Ddi_Bddarray_t *fA,
  int *clusterMap
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(fA);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  int i, ncl=-1, n = Ddi_BddarrayNum(fA);
  Ddi_Bddarray_t *newFA;
  Ddi_Bdd_t *myOne = Ddi_BddMakeConstAig(ddm,1);
  Ddi_Bdd_t *voidPart = Ddi_BddMakePartConjVoid(ddm);
  Ddi_AigDynSignatureArray_t *clusterSigs;
  Ddi_AigDynSignature_t *mySig = NULL;
  int nBits = sccMgr->sccSigs->nBits;

  for (i=0;i<n;i++) {
    if (clusterMap[i]>ncl) ncl = clusterMap[i];
  }
  Pdtutil_Assert(ncl>=0 && ncl<=n,"wrong cluster map");

  ncl++;

  newFA = Ddi_BddarrayAlloc(ddm,ncl);
  clusterSigs = DdiAigDynSignatureArrayAllocVoid(ncl,nBits);

  for (i=0;i<ncl;i++) {
    mySig = clusterSigs->sArray[i] = DdiAigDynSignatureAlloc(nBits);
    DdiAigDynSignatureClear(mySig);
    Ddi_BddarrayWrite(newFA,i,voidPart);
  }

  for (i=0;i<n;i++) {
    int j = clusterMap[i];
    Ddi_Bdd_t *cl_j, *f_i = Ddi_BddarrayRead(fA,i);
    Pdtutil_Assert(j>=0 && j<ncl,"wrong cluster map");
    cl_j = Ddi_BddarrayRead(newFA,j);
    //    Ddi_BddAndAcc(cl_j,f_i);
    Ddi_BddPartInsertLast(cl_j,f_i);
    if (!Ddi_BddIsConstant(f_i)) {
      bAigEdge_t fBaig = Ddi_BddToBaig(f_i);
      int jScc = bAig_AuxInt(bmgr,fBaig);
      int k = sccMgr->mapScc[jScc];
      mySig = clusterSigs->sArray[j];
      DdiAigDynSignatureUnion(mySig, mySig, sccMgr->sccSigs->sArray[k],0);
    }
  }

  for (i=0;i<ncl;i++) {
    Pdtutil_Assert(Ddi_BddIsPartConj(Ddi_BddarrayRead(newFA,i)),
		   "conj part needed");
    Pdtutil_Assert(Ddi_BddPartNum(Ddi_BddarrayRead(newFA,i))>0,
		   "void conj part");
  }

  Ddi_DataCopy(fA,newFA);

  Ddi_Free(newFA);
  Ddi_Free(myOne);
  Ddi_Free(voidPart);

  return clusterSigs;
}

/**Function********************************************************************
  Synopsis     []
  Description  []
  SideEffects  []
******************************************************************************/
Ddi_Varsetarray_t *
Ddi_AigEvalMultipleCoiWithScc (
  Ddi_Bddarray_t *delta,
  Ddi_Bddarray_t *lambda,
  Ddi_Vararray_t *ps,
  int optionCluster,
  char *clusterFile,
  int methodCluster,
  int thresholdCluster,
  int th_coi_diff,
  int th_coi_share,
  int doSort
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(delta);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  int i, nRoots, nStateVars, nOutputs;
  int *mapPs, *mapNs;
  Ddi_AigDynSignatureArray_t *sccSigs, *clusterSigs;
  Ddi_Varsetarray_t *cois;
  Ddi_Vararray_t *psSorted=NULL;
  Ddi_Varset_t *psVarset=NULL;
  int thresholdFinal = 1;
  int nBits;
  int *clusterMap=NULL;

  Ddi_SccMgr_t *sccMgr = Ddi_FsmSccTarjan (delta,lambda,ps);
  int min=10, max=thresholdCluster, np=Ddi_BddarrayNum(lambda);

  nStateVars = Ddi_VararrayNum(ps);
  
  Pdtutil_Assert(nStateVars==Ddi_BddarrayNum(delta),"error in array num");
  nOutputs = Ddi_BddarrayNum(lambda); 

  Ddi_MgrSiftSuspend(ddm);

  nBits = sccMgr->nSccWithLatch;
  sccSigs = sccMgr->sccSigs = 
    DdiAigDynSignatureArrayAllocVoid(sccMgr->nScc,nBits);
  sccSigs->nSigs = sccMgr->nScc;

  for (i=0; i<sccMgr->nScc; i++) {
    int bit = sccMgr->mapSccBit[i];
    Pdtutil_Assert(bit<nBits,"wrong scc bit");
    if (bit>=0) {
      sccSigs->sArray[i] = DdiAigDynSignatureAlloc(nBits);
      DdiAigDynSignatureSetBit(sccSigs->sArray[i], bit);
    }
  }


  for (i=0; i<sccMgr->sccTopologicalNodes->num; i++) {
    bAigEdge_t baig = sccMgr->sccTopologicalNodes->nodes[i];
    bAig_AuxInt(bmgr,baig) = i;
  }

  Ddi_AigEvalVarSignaturesWithScc (sccMgr,ddm,sccMgr->sccTopologicalNodes,
    lambda,sccSigs,sccMgr->mapScc,sccMgr->mapNs,nStateVars);

  printf("#ALLOCATED bitmaps: %d / %d\n", 
         nAllocatedBitmaps, sccSigs->nSigs);

  cois = Ddi_VarsetarrayAlloc(ddm,nOutputs);



  clusterMap = BddarrayClusterByCoiAcc(sccMgr,lambda,min,max,th_coi_diff,th_coi_share,methodCluster,doSort); 

  clusterSigs = BddarrayClusterProperties(sccMgr,lambda,clusterMap);

  for (i=0; i<nStateVars; i++) {
    Ddi_Var_t *psVar = Ddi_VararrayRead(ps, i);
    Pdtutil_Assert(Ddi_VarReadMark(psVar) == 0,"0 var mark required");
    Ddi_VarWriteMark(psVar,i);
  }
  psVarset = Ddi_VarsetMakeFromArray(ps);
  psSorted = Ddi_VararrayMakeFromVarset(psVarset,1); // sorted by ord
  Ddi_Free(psVarset);
 
  for (i=0; i<Ddi_BddarrayNum(lambda); i++) {		
    Ddi_Varset_t *sigSupp;
    Ddi_Bdd_t *fAig = Ddi_BddarrayRead(lambda, i);
    if (Ddi_BddIsConstant(fAig)) {
      sigSupp = Ddi_VarsetVoid(ddm);
    }
    else {
      sigSupp = DdiDynSignatureToVarsetWithScc(
        sccMgr,clusterSigs->sArray[i],psSorted);
    }
    Ddi_VarsetarrayWrite(cois,i,sigSupp);
    Ddi_Free(sigSupp);
  }

  for (i=0; i<nStateVars; i++) {
    Ddi_Var_t *psVar = Ddi_VararrayRead(ps, i);
    Ddi_VarWriteMark(psVar,0);
  }
  Ddi_Free(psSorted);

  for (i=0; i<sccMgr->sccTopologicalNodes->num; i++) {
    bAigEdge_t baig = sccMgr->sccTopologicalNodes->nodes[i];
    Pdtutil_Assert(bAig_AuxInt(bmgr,baig)>=0, "wrong auxId field");
    bAig_AuxInt(bmgr,baig) = -1;
  }

  Ddi_MgrSiftResume(ddm);

  DdiAigDynSignatureArrayFree(clusterSigs);

  sccMgrFree(sccMgr);

  Pdtutil_Free(clusterMap);

  return(cois);
}

/**Function********************************************************************
  Synopsis     []
  Description  []
  SideEffects  []
******************************************************************************/
void
DdiCoisStatistics (
  Ddi_Varsetarray_t * cois, 
  float **correlationMatrix,
  Ddi_CoiStats_e type
)
{
  //FILE * fout;	
  Pdtutil_Assert(correlationMatrix != NULL, "Null pointer in destination");
  int nOutputs = Ddi_VarsetarrayNum(cois);
  int i, j;
  //fout=fopen("out.txt","w");	
  for(i=0;i<nOutputs;i++) {
    Ddi_Varset_t* vi = Ddi_VarsetarrayRead(cois, i);
    int iSize = Ddi_VarsetNum(vi);
    for(j=0;j<nOutputs;j++) {
      Ddi_Varset_t* vj = Ddi_VarsetarrayRead(cois, j);

      if (i == j) {
        correlationMatrix[i][j] = 1;
				//fprintf(fout,"%1.2f ",correlationMatrix[i][j]);
        continue;
      }
      
      Ddi_Varset_t* in = Ddi_VarsetIntersect(vi, vj); 

      int intersectVarNum=  Ddi_VarsetNum(in);

      Ddi_Free(in);
    
      
      switch (type) {
      case Ddi_CoiIntersectionOverUnion_c:
        {

	  if (iSize == 0 && Ddi_VarsetNum(vj) == 0) {
          correlationMatrix[i][j] = -1;
	    //fprintf(fout,"%1.2f ",correlationMatrix[i][j]);
          continue;
        }
        
        Ddi_Varset_t* un = Ddi_VarsetUnion(vi, vj);
        correlationMatrix[i][j] =intersectVarNum / (float) Ddi_VarsetNum(un) ;
        Ddi_Free(un);			

        }
        break;  
      case Ddi_CoiIntersectionOverLeft_c:
        {

        if (iSize == 0) {
          correlationMatrix[i][j] = -1;
	  			//fprintf(fout,"%1.2f ",correlationMatrix[i][j]);
          continue;
        }
        
        correlationMatrix[i][j] =intersectVarNum / (float) Ddi_VarsetNum(vi) ;
       
        }
        break;  
      case Ddi_CoiIntersectionOverRight_c:
        {

        if (Ddi_VarsetNum(vj) == 0) {
          correlationMatrix[i][j] = -1;
	  			//fprintf(fout,"%1.2f ",correlationMatrix[i][j]);
          continue;
        }
        
        correlationMatrix[i][j] = intersectVarNum / (float) Ddi_VarsetNum(vj) ;
        
        }

		   

        break;  
      case Ddi_CoiNull_c:
        {
        }
        break;    

      }
      
      //fprintf(fout,"%1.2f ",correlationMatrix[i][j]); 	 

      //fprintf(fout,"%1.2f ",correlationMatrix[i][j]); 	 

    }
    //fprintf(fout,"\n");	
  }
}

/**Function********************************************************************
  Synopsis     []
  Description  []
  SideEffects  []
******************************************************************************/
void
DdiDeltaCoisVarsRank (
  Ddi_Varsetarray_t * cois, 
  int *varsRank,
  Ddi_Vararray_t *ps
)
{
  Pdtutil_Assert(varsRank != NULL, "Null pointer in destination");
  int nCois = Ddi_VarsetarrayNum(cois);
  int i, j, nVars = Ddi_VararrayNum(ps);
  for(i=0;i<nCois;i++) {
    Ddi_Varset_t* coi = Ddi_VarsetarrayRead(cois, i);
    for(j=0;j<nVars;j++) {
      if (Ddi_VarInVarset(coi, Ddi_VararrayRead(ps, j)))
        varsRank[j]++; 
    }    
  }
}


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_SccMgr_t *
sccMgrAlloc(
  Ddi_Bddarray_t *delta,
  Ddi_Vararray_t *ps
)
{
  //Alloco lo spazio
  Ddi_SccMgr_t *sccMgr = Pdtutil_Alloc(Ddi_SccMgr_t,1);
  
  //Inizializzo i valori
  sccMgr->ddiMgr = Ddi_ReadMgr(delta);
  sccMgr->nScc = 0;
  sccMgr->nSccGate = 0;
  sccMgr->nSccWithLatch = 0;
  sccMgr->nBitmapMax = 0;
  sccMgr->size1 = 0;
  sccMgr->size2 = 0;
  sccMgr->nLatches = Ddi_BddarrayNum(delta);
  sccMgr->delta = delta;
  sccMgr->ps = ps;
  sccMgr->aigStack = bAigArrayAlloc();
  sccMgr->aigNodes = bAigArrayAlloc();
  sccMgr->sccTopologicalNodes = bAigArrayAlloc();
  sccMgr->sccTopologicalLevel = NULL;
  sccMgr->sccLatchTopologicalLevel = NULL;

  sccMgr->sccSize = NULL;
  sccMgr->sccLatchCnt = NULL;
  sccMgr->sccGateCnt = NULL;
  sccMgr->mapScc = NULL;
  sccMgr->mapSccBit = NULL;
  sccMgr->mapNs = NULL;
  sccMgr->mapLatches = NULL;
  sccMgr->sccSigs = NULL;
  
  sccMgr->sccBridges = NULL;
  sccMgr->sccBridgesCnt = NULL;
  sccMgr->sccOutCnt = NULL;
  sccMgr->sccOutGate = NULL;
  sccMgr->sccGateFoCnt = NULL;
  sccMgr->sccInCnt = NULL;
  sccMgr->sccInGate = NULL;
  

  return sccMgr;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_SccMgrFree(
  Ddi_SccMgr_t *sccMgr
)
{
  sccMgrFree(sccMgr);
}

/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
sccMgrFree(
  Ddi_SccMgr_t *sccMgr
)
{
  int i;
  bAig_Manager_t *bmgr = sccMgr->ddiMgr->aig.mgr;

  Pdtutil_Free(sccMgr->sccLatchCnt);
  Pdtutil_Free(sccMgr->sccSize);
  Pdtutil_Free(sccMgr->sccGateCnt);
  Pdtutil_Free(sccMgr->mapScc);
  Pdtutil_Free(sccMgr->mapSccBit);
  Pdtutil_Free(sccMgr->mapNs);
  Pdtutil_Free(sccMgr->mapLatches);
  Pdtutil_Free(sccMgr->sccBridges);
  Pdtutil_Free(sccMgr->sccBridgesCnt);
  Pdtutil_Free(sccMgr->sccOutCnt);
  Pdtutil_Free(sccMgr->sccOutGate);
  Pdtutil_Free(sccMgr->sccGateFoCnt);
  Pdtutil_Free(sccMgr->sccInCnt);
  Pdtutil_Free(sccMgr->sccInGate);

  Pdtutil_Free(sccMgr->sccTopologicalLevel);
  Pdtutil_Free(sccMgr->sccLatchTopologicalLevel);
  Pdtutil_Free(sccMgr->sccGateTopologicalLevelMin);
  Pdtutil_Free(sccMgr->sccGateTopologicalLevelMax);
  
  DdiAigDynSignatureArrayFree(sccMgr->sccSigs);

  bAigArrayFree(sccMgr->aigStack);
  bAigArrayFree(sccMgr->aigNodes);
  bAigArrayFree(sccMgr->sccTopologicalNodes);

  Pdtutil_Free(sccMgr);
}



/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
fsmSccTarjanIntern (
  Ddi_SccMgr_t *sccMgr,
  bAigEdge_t nodeIndex
)
{
  int ldrId, isLatch=0;
  bAig_Manager_t *manager = sccMgr->ddiMgr->aig.mgr;

  nodeIndex = bAig_NonInvertedEdge(nodeIndex);

  Pdtutil_Assert(nodeVisited(manager,nodeIndex) < 8, "visited error in PO");

  if (nodeVisited(manager,nodeIndex) >= 4) {
    return 0;
  }
  if (bAig_NodeIsConstant(nodeIndex)) {
    return -1;
  }

  nodeVisited(manager,nodeIndex) += 4;

  /* pre-order visit */

  ldrId = bAig_AuxInt(manager,nodeIndex) = sccMgr->aigNodes->num;
  Pdtutil_Assert(bAig_AuxInt1(manager,nodeIndex)==-1,"wrong aux int1 field");
  bAig_AuxInt1(manager,nodeIndex) = sccMgr->aigNodes->num;

  bAigArrayWriteLast(sccMgr->aigNodes,nodeIndex);
  bAigArrayWriteLast(sccMgr->aigStack,nodeIndex);
  Pdtutil_Assert(nodeAuxChar(manager,nodeIndex)==0,"wrong aux char");
  nodeAuxChar(manager,nodeIndex)=1;

  if (bAig_isVarNode(manager,nodeIndex)) {
    bAigEdge_t d = bAig_CacheAig(manager,nodeIndex);
    if (d != bAig_NULL) {
      /* it's a latch. Recur */
      int ret = fsmSccTarjanIntern (sccMgr,d);
      isLatch = 1;
      if (ret==1) {
        /* visited */
        if (bAig_AuxInt1(manager,nodeIndex) >  bAig_AuxInt1(manager,d)) {
          bAig_AuxInt1(manager,nodeIndex) =  bAig_AuxInt1(manager,d);
        }
      }
      else if (ret==0) {
        if (nodeAuxChar(manager,d)==1) {
        /* in stack */
          if (bAig_AuxInt1(manager,nodeIndex) >  bAig_AuxInt(manager,d)) {
            bAig_AuxInt1(manager,nodeIndex) =  bAig_AuxInt(manager,d);
          }
        }
      }
    }
  }
  else {
    bAigEdge_t right = bAig_NodeReadIndexOfRightChild(manager,nodeIndex);
    bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(manager,nodeIndex);
    int ret = fsmSccTarjanIntern (sccMgr,right);
    if (ret==1) {
      /* visited */
      if (bAig_AuxInt1(manager,nodeIndex) >  bAig_AuxInt1(manager,right)) {
        bAig_AuxInt1(manager,nodeIndex) =  bAig_AuxInt1(manager,right);
      }
    }
    else if (ret==0) {
      if (nodeAuxChar(manager,right)==1) {
        /* in stack */
        if (bAig_AuxInt1(manager,nodeIndex) >  bAig_AuxInt(manager,right)) {
          bAig_AuxInt1(manager,nodeIndex) =  bAig_AuxInt(manager,right);
        }
      }
    }
    ret = fsmSccTarjanIntern (sccMgr,left);
    if (ret==1) {
      /* visited */
      if (bAig_AuxInt1(manager,nodeIndex) >  bAig_AuxInt1(manager,left)) {
        bAig_AuxInt1(manager,nodeIndex) =  bAig_AuxInt1(manager,left);
      }
    }
    else if (ret==0) {
      if (nodeAuxChar(manager,left)==1) {
        /* in stack */
        if (bAig_AuxInt1(manager,nodeIndex) >  bAig_AuxInt(manager,left)) {
          bAig_AuxInt1(manager,nodeIndex) =  bAig_AuxInt(manager,left);
        }
      }
    }
  }

  if (bAig_AuxInt1(manager,nodeIndex) ==  bAig_AuxInt(manager,nodeIndex)) {
    /* leader of SCC */
    bAigEdge_t prev, first, baig = bAigArrayPop(sccMgr->aigStack);
    Pdtutil_Assert(nodeAuxChar(manager,baig)==1,"wrong aux char");
    nodeAuxChar(manager,baig)=0;
    first = baig;
    prev = bAig_NULL;
    while (baig != nodeIndex) {
      int id = bAig_AuxInt(manager,baig);
      prev = baig;
      Pdtutil_Assert(bAig_AuxAig0(manager,prev)==bAig_NULL,"invalid aux aig0");
      baig = bAigArrayPop(sccMgr->aigStack);
      Pdtutil_Assert(nodeAuxChar(manager,baig)==1,"wrong aux char");
      nodeAuxChar(manager,baig)=0;
      bAig_AuxAig0(manager,prev)=baig;
      bAig_AuxAig1(manager,prev)=nodeIndex;
    }
    if (prev==bAig_NULL) {
      if (!isLatch) sccMgr->nSccGate++;
    }
    Pdtutil_Assert(bAig_AuxAig0(manager,baig)==bAig_NULL,"invalid aux aig0");
    bAig_AuxAig0(manager,baig)=first;
    bAig_AuxAig1(manager,baig)=nodeIndex;
    sccMgr->nScc++;
  }

  return 1;

}


/**Function********************************************************************
  Synopsis    [refine state eq classes]
  Description [refine state eq classes]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
fsmSccTarjanCheckBridgeIntern (
  Ddi_SccMgr_t *sccMgr,
  bAigEdge_t nodeIndex
)
{

  int ldrId, isLatch=0;
  bAig_Manager_t *manager = sccMgr->ddiMgr->aig.mgr;

  nodeIndex = bAig_NonInvertedEdge(nodeIndex);

  Pdtutil_Assert(nodeVisited(manager,nodeIndex) < 8, "visited error in PO");

  if (nodeVisited(manager,nodeIndex) >= 4) {
    return 0;
  }
  
  if (bAig_NodeIsConstant(nodeIndex)) {
    return -1;
  }

  nodeVisited(manager,nodeIndex) += 4;

  /* pre-order visit */
  ldrId = bAig_AuxInt(manager,nodeIndex) = sccMgr->aigNodes->num;
  Pdtutil_Assert(bAig_AuxInt1(manager,nodeIndex)==-1,"wrong aux int1 field");
  
  bAig_AuxInt1(manager,nodeIndex) = sccMgr->aigNodes->num;

  bAigArrayWriteLast(sccMgr->aigNodes,nodeIndex);
  
  bAigArrayWriteLast(sccMgr->aigStack,nodeIndex);
  
  Pdtutil_Assert(!IsInStack(manager,nodeIndex),"wrong aux char");
  nodeAuxChar(manager,nodeIndex) |= Ddi_SccNodeStatus_InStack_c;

  if (bAig_isVarNode(manager,nodeIndex)) {
  
    bAigEdge_t d = bAig_CacheAig(manager,nodeIndex);
    
    if (d != bAig_NULL && IsRightEnabled(manager, nodeIndex)) {
      /* it's a latch. Recur */
      
      int ret = fsmSccTarjanCheckBridgeIntern (sccMgr,d);
      
      isLatch = 1;
      
      if (ret==1) {
        /* visited */
        
        if (bAig_AuxInt1(manager,nodeIndex) >  bAig_AuxInt1(manager,d)) {
          bAig_AuxInt1(manager,nodeIndex) =  bAig_AuxInt1(manager,d);
        }
      }
      else if (ret==0) {
        if (IsInStack(manager,d)) {
        /* in stack */
          if (bAig_AuxInt1(manager,nodeIndex) >  bAig_AuxInt(manager,d)) {
            bAig_AuxInt1(manager,nodeIndex) =  bAig_AuxInt(manager,d);
          }
        }
      }
    }
  }
  
  else {
  
    bAigEdge_t right = bAig_NodeReadIndexOfRightChild(manager,nodeIndex);
    bAigEdge_t left = bAig_NodeReadIndexOfLeftChild(manager,nodeIndex);
    
    int ret;
    if(IsRightEnabled(manager, nodeIndex)){
      ret = fsmSccTarjanCheckBridgeIntern (sccMgr,right);
      if (ret==1) {
        /* visited */
        if (bAig_AuxInt1(manager,nodeIndex) >  bAig_AuxInt1(manager,right)) {
          bAig_AuxInt1(manager,nodeIndex) =  bAig_AuxInt1(manager,right);
        }
      }
      else if (ret==0) {
        if (IsInStack(manager,right)) {
          /* in stack */
          if (bAig_AuxInt1(manager,nodeIndex) >  bAig_AuxInt(manager,right)) {
            bAig_AuxInt1(manager,nodeIndex) =  bAig_AuxInt(manager,right);
          }
        }
      }
    }
    
    if(IsLeftEnabled(manager, nodeIndex)){
      ret = fsmSccTarjanCheckBridgeIntern (sccMgr,left);
      if (ret==1) {
        /* visited */
        if (bAig_AuxInt1(manager,nodeIndex) >  bAig_AuxInt1(manager,left)) {
          bAig_AuxInt1(manager,nodeIndex) =  bAig_AuxInt1(manager,left);
        }
      }
      else if (ret==0) {
        if (IsInStack(manager,left)) {
          /* in stack */
          if (bAig_AuxInt1(manager,nodeIndex) >  bAig_AuxInt(manager,left)) {
            bAig_AuxInt1(manager,nodeIndex) =  bAig_AuxInt(manager,left);
          }
        }
      }
    }
  }


  if (bAig_AuxInt1(manager,nodeIndex) ==  bAig_AuxInt(manager,nodeIndex)) {
    /* leader of SCC */
    
    bAigEdge_t prev, first, baig = bAigArrayPop(sccMgr->aigStack);
    Pdtutil_Assert(IsInStack(manager,baig),"wrong aux char");
    nodeAuxChar(manager,baig) &= ~Ddi_SccNodeStatus_InStack_c;
        
    first = baig;
    prev = bAig_NULL;
    
    while (baig != nodeIndex) {
    
      int id = bAig_AuxInt(manager,baig);
      prev = baig;
      Pdtutil_Assert(bAig_AuxAig0(manager,prev)==bAig_NULL,"invalid aux aig0");
      
      baig = bAigArrayPop(sccMgr->aigStack);
      Pdtutil_Assert(IsInStack(manager,baig),"wrong aux char");
      nodeAuxChar(manager,baig) &= ~Ddi_SccNodeStatus_InStack_c;
      
      bAig_AuxAig0(manager,prev)=baig;
      
      bAig_AuxAig1(manager,prev)=nodeIndex;
    }
    
    if (prev==bAig_NULL) {
      if (!isLatch) sccMgr->nSccGate++;
    }
    Pdtutil_Assert(bAig_AuxAig0(manager,baig)==bAig_NULL,"invalid aux aig0");
    
    bAig_AuxAig0(manager,baig)=first;
    
    bAig_AuxAig1(manager,baig)=nodeIndex;
    
    sccMgr->nScc++;
  }

  return 1;

}


/**Function********************************************************************
  Synopsis    [Post order AIG visit with SCC equiv]
  Description [Post order AIG visit with SCC equiv]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static void
postOrderAigVisitInternWithScc(
  bAig_Manager_t *manager,
  bAigEdge_t nodeIndex,
  bAig_array_t *visitedNodes
)
{
  bAigEdge_t baig;

  if (bAig_NodeIsConstant(nodeIndex)) return;

  Pdtutil_Assert(nodeVisited(manager,nodeIndex) < 8, "visited error in PO");

  if (nodeVisited(manager,nodeIndex) >= 4) {
    return;
  }

  /* process nodes in same SCC: set visited */
  baig = nodeIndex;
  do {
    Pdtutil_Assert(baig!=bAig_NULL, "NULL baig");
    Pdtutil_Assert(nodeVisited(manager,baig) < 4, "visited error in PO");
    nodeVisited(manager,baig) += 4;
    baig = bAig_AuxAig0(manager,baig);
  } while (bAig_NonInvertedEdge(baig) != bAig_NonInvertedEdge(nodeIndex));

  /* process nodes in same SCC: recur */
  baig = nodeIndex;
  do {
    Pdtutil_Assert (!bAig_NodeIsConstant(baig),"constant baig found");
    if (bAig_isVarNode(manager,baig)) {
      /* var: if state var go to delta node */
      bAigEdge_t d = bAig_CacheAig(manager,baig);
      if (d != bAig_NULL) {
        /* it's a latch. Recur if delta is not constant */
	if (!bAig_NodeIsConstant(d))
	  postOrderAigVisitInternWithScc(manager,d,visitedNodes);
      }
    }
    else {
      postOrderAigVisitInternWithScc(manager,
        rightChild(manager,baig),visitedNodes);
      postOrderAigVisitInternWithScc(manager,
        leftChild(manager,baig),visitedNodes);
    }
    baig = bAig_AuxAig0(manager,baig);
  } while (bAig_NonInvertedEdge(baig) != bAig_NonInvertedEdge(nodeIndex));

  /* process nodes in same SCC: write to array */
  baig = nodeIndex;
  do {
    bAigArrayWriteLast(visitedNodes,baig);
    baig = bAig_AuxAig0(manager,baig);
  } while (bAig_NonInvertedEdge(baig) != bAig_NonInvertedEdge(nodeIndex));

}


/**Function********************************************************************
  Synopsis    [Looks for ITE constructs and tracks ENABLE signals]
  Description [Looks for ITE constructs and tracks ENABLE signals]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Ddi_FindAigIte(
  Ddi_Bdd_t *f,
  int threshold
) {
  Ddi_Bddarray_t *en;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(f);
  Ddi_Bddarray_t *fA = Ddi_BddarrayAlloc(ddm, 1);
  Ddi_BddarrayWrite(fA,0,f);
  en = Ddi_FindIte(fA,NULL,threshold);
  Ddi_Free(fA);
  return en;
}


/**Function********************************************************************
  Synopsis    [Looks for ITE constructs and tracks ENABLE signals]
  Description [Looks for ITE constructs and tracks ENABLE signals]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Ddi_FindIte(
  Ddi_Bddarray_t *fA,
  Ddi_Vararray_t *ps,
  int threshold
) {
  return Ddi_FindIteFull(fA,ps,NULL,threshold);
}

/**Function********************************************************************
  Synopsis    [Looks for ITE constructs and tracks ENABLE signals]
  Description [Looks for ITE constructs and tracks ENABLE signals]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ddi_Bddarray_t *
Ddi_FindIteFull(
  Ddi_Bddarray_t *fA,
  Ddi_Vararray_t *ps,
  Ddi_Bddarray_t *iteArray,
  int threshold
) {
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(fA);
  bAig_Manager_t *bmgr = ddm->aig.mgr;
  bAig_array_t *visitedNodes = bAigArrayAlloc();
  Ddi_Bddarray_t *enables = Ddi_BddarrayAlloc(ddm, 0);
  Pdtutil_MapInt2Int_t *mapEnableCnt = Pdtutil_MapInt2IntAlloc (10,10,0,Pdtutil_MapInt2IntDir_c);
  int nRoots = Ddi_BddarrayNum(fA), i, ret = 0, cnt = 0, cntl = 0;

  for (i=0; i<nRoots; i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    bAigEdge_t fBaig = Ddi_BddToBaig(f_i);

    if (!bAig_NodeIsConstant(fBaig)) {
      DdiPostOrderAigVisitIntern(bmgr,fBaig,visitedNodes,-1);
    }
  }
  DdiPostOrderAigClearVisitedIntern(bmgr,visitedNodes);

/*  for (i=0; i<visitedNodes->num; i++) {*/

/*    bAigEdge_t baig = visitedNodes->nodes[i];*/
/*    bAig_AuxInt(bmgr,baig) = i;*/
/*  }*/

/*  bAig_array_t *latchPSmap = bAigArrayAlloc();*/
/*  for (i=0; i<visitedNodes->num; i++) {*/
/*    bAigArrayWriteLast(latchPSmap, bAig_NULL);*/
/*  }*/

  if (ps!=NULL)
  for (i=0; i<Ddi_VararrayNum(ps); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(ps,i);
    Ddi_Bdd_t *d = Ddi_BddarrayRead(fA,i);
    bAigEdge_t varIndex = Ddi_VarToBaig(v);
    bAigEdge_t dBaig = Ddi_BddToBaig(d);
    bAig_array_t* rootPsList = (bAig_array_t*) bAig_AuxPtr(bmgr,dBaig);
    if(rootPsList == NULL){
      rootPsList = bAigArrayAlloc();
      bAig_AuxPtr(bmgr,dBaig) = rootPsList;
    }
    bAigArrayWriteLast(rootPsList, varIndex);
    //int Id = bAig_AuxInt(bmgr,dBaig);
    //latchPSmap->nodes[Id] = varIndex;
    // Pdtutil_Assert(bAig_AuxAig0(bmgr,dBaig)==bAig_NULL, "invalid cache aig");
    // printf ("dBaig %d (var = %d) :: PRE (%d) > POST (%d)\n", dBaig, i, bAig_AuxAig0(bmgr,dBaig), varIndex); 
    // bAig_AuxAig0(bmgr,dBaig) = varIndex;
  }


  int max = -1, j, nGate=0; 
  for (i=0; i<visitedNodes->num; i++) {
    bAigEdge_t baig = visitedNodes->nodes[i];

    if ((ret = findIte(bmgr, baig, i)) > 0) {
      cnt++;
      int isLatchEn = 0;
      bAig_array_t* rootPS = (bAig_array_t*) bAig_AuxPtr(bmgr,baig);
      if (rootPS != NULL) {
	for(j=0;j<rootPS->num;j++) {
	  bAigEdge_t latchPS = rootPS->nodes[j];
	  if (latchPS!=bAig_NULL) {
	    /* is it a latch enable? */
	    bAigEdge_t right, left, rr, rl, lr, ll;
	    right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
	    left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);
	    rr = rl = lr = ll = bAig_NULL;
	    rr = bAig_NonInvertedEdge(bAig_NodeReadIndexOfRightChild(bmgr,right));
	    rl = bAig_NonInvertedEdge(bAig_NodeReadIndexOfLeftChild(bmgr,right));
	    lr = bAig_NonInvertedEdge(bAig_NodeReadIndexOfRightChild(bmgr,left));
	    ll = bAig_NonInvertedEdge(bAig_NodeReadIndexOfLeftChild(bmgr,left));
	    latchPS = bAig_NonInvertedEdge(latchPS);

	    if ( ret == 1 ) {
	      if (ll == latchPS || rl == latchPS){
		//		printf("%d -> %s\n", lr, bAig_NodeReadName(bmgr,latchPS));          
		Pdtutil_MapInt2IntWrite(mapEnableCnt, lr, lr <= max ? Pdtutil_MapInt2IntReadDir(mapEnableCnt, lr)+1 : 1);
		max = lr > max ? lr : max ;
		cntl++; isLatchEn = 1;
	      }
	    } else if (ret == 2 ) {
	      if (rl == latchPS || lr  == latchPS){
		//		printf("%d -> %s\n", ll, bAig_NodeReadName(bmgr,latchPS));  
		Pdtutil_MapInt2IntWrite(mapEnableCnt, ll, ll <= max ? Pdtutil_MapInt2IntReadDir(mapEnableCnt, ll)+1 : 1);  
		max = ll > max ? ll : max ;      
		cntl++; isLatchEn = 1;
	      }
	    } else if (ret == 3 ) {
	      if (ll == latchPS || rr == latchPS){
		//		printf("%d -> %s\n", lr, bAig_NodeReadName(bmgr,latchPS));    
		Pdtutil_MapInt2IntWrite(mapEnableCnt, lr, lr <= max ? Pdtutil_MapInt2IntReadDir(mapEnableCnt, lr)+1 : 1);   
		max = lr > max ? lr : max ;   
		cntl++; isLatchEn = 1;
	      }
	    } else if (ret == 4 ) {
	      if (lr == latchPS || rr == latchPS){
		//		printf("%d -> %s\n", ll, bAig_NodeReadName(bmgr,latchPS));   
		Pdtutil_MapInt2IntWrite(mapEnableCnt, ll, ll  <= max ? Pdtutil_MapInt2IntReadDir(mapEnableCnt, ll)+1 : 1);    
		max = ll > max ? ll : max ;   
		cntl++; isLatchEn = 1;
	      }
	    }
	  }
	}
      }
      if (!isLatchEn) {
	bAigEdge_t right, left, rr, rl, lr, ll, en;
	right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
	left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);
	rr = rl = lr = ll = bAig_NULL;
	rr = bAig_NonInvertedEdge(bAig_NodeReadIndexOfRightChild(bmgr,right));
	rl = bAig_NonInvertedEdge(bAig_NodeReadIndexOfLeftChild(bmgr,right));
	lr = bAig_NonInvertedEdge(bAig_NodeReadIndexOfRightChild(bmgr,left));
	ll = bAig_NonInvertedEdge(bAig_NodeReadIndexOfLeftChild(bmgr,left));
	switch (ret) {
	case 1:
	case 2: en = rr;
          break;
	case 3:
	case 4: en = rl;
          break;
	default: Pdtutil_Assert(0,"wrong case");
	}
	if (1 || threshold<0) {
          if (iteArray!=NULL) {
            Ddi_Bdd_t *ite_i = Ddi_BddMakeFromBaig(ddm, baig);
            Ddi_BddarrayInsertLast(iteArray,ite_i);
            Ddi_Free(ite_i);
          }
	  Ddi_Bdd_t *en_i = Ddi_BddMakeFromBaig(ddm, en);
	  int cnt = en>=max ? 0 : Pdtutil_MapInt2IntReadDir(mapEnableCnt, en);
	  if (cnt<=0) {
	    Pdtutil_MapInt2IntWrite(mapEnableCnt, en, 1);
	    //	    Ddi_BddarrayInsertLast(enables,en_i);
	  }
	  else {
	    Pdtutil_MapInt2IntWrite(mapEnableCnt, en, cnt+1);
	  }
	  max = en > max ? en : max ;
	  Ddi_Free(en_i);
	}
	continue;
      }
    }
  }


  //  printf("\n# ITEs found > %d (%d go[es] to a latch)\nEnable signals counting (SIGNAL > OCCURRENCES)\n", cnt, cntl);
  int maxCnt = -1, iMaxCnt=-1;
  for (i=0; i<=max; i++) {
    int cnt = Pdtutil_MapInt2IntReadDir(mapEnableCnt, i);
    if (cnt > 0){
      bAigEdge_t enable = i;
      if (cnt>maxCnt) {
	maxCnt = cnt; iMaxCnt = i;
      }
      if (cnt>=threshold) {
	Ddi_Bdd_t *en_i = Ddi_BddMakeFromBaig(ddm, enable);
	Ddi_BddarrayInsertLast(enables,en_i);
	Ddi_Free(en_i);
      }
      if(bAig_NodeIsConstant(enable)){
	//        printf("%d (CONSTANT) > %d\n", i, cnt);
      } else if(bAig_isVarNode(bmgr,enable)){
        int isStateVar = 0;
        for (j=0; j<Ddi_VararrayNum(ps); j++) {
          Ddi_Var_t *v = Ddi_VararrayRead(ps,j);
          bAigEdge_t latchPS = Ddi_VarToBaig(v);
          if (latchPS!=bAig_NULL && enable == latchPS) {  
            isStateVar = 1;
            break;    
          }
        }
	//        if(isStateVar) printf("%d (STATE) > %d\n", i, cnt);
	//        else printf("%d (INPUT) > %d\n", i, cnt);
	if (0&& (cnt>0)) {
	  Ddi_Var_t *v = Ddi_VarFromBaig(ddm, enable);
	  Ddi_Bddarray_t *fA0 = Ddi_BddarrayCofactor(fA,v,0);
	  Ddi_Bddarray_t *fA1 = Ddi_BddarrayCofactor(fA,v,1);
	  //	  printf("split sizes: %d ->(%d,%d)\n",
	  //		 Ddi_BddarraySize(fA),
	  //		 Ddi_BddarraySize(fA0),
	  //		 Ddi_BddarraySize(fA1));
	  Ddi_Free(fA0);
	  Ddi_Free(fA1);
	}
      } else {
        int level = computeLevel(bmgr, enable);
	//        printf("%d (GATE %d) > %d\n", i, level, cnt);
      }
    }  
  }

  printf("\nMax ENABLE control count: %d)\n", maxCnt);
  if (0 && (iMaxCnt >= 0)) {
    Ddi_Bdd_t *en_i = Ddi_BddMakeFromBaig(ddm, iMaxCnt);
    Ddi_BddarrayWrite(enables,0,en_i);
    Ddi_Free(en_i);
  }

  Pdtutil_MapInt2IntFree(mapEnableCnt);

  for (i=0; i<Ddi_VararrayNum(ps); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(ps,i);
    Ddi_Bdd_t *d = Ddi_BddarrayRead(fA,i);
    bAigEdge_t varIndex = Ddi_VarToBaig(v);
    bAigEdge_t dBaig = Ddi_BddToBaig(d);
    // bAig_AuxAig0(bmgr,dBaig) = bAig_NULL;
    bAig_array_t* dump = (bAig_array_t*) bAig_AuxPtr(bmgr,dBaig);
    if (dump != NULL) bAigArrayFree(dump);
    bAig_AuxPtr(bmgr,dBaig) = NULL;
  }

  bAigArrayFree(visitedNodes);

  return enables;

}

static int
computeLevel(
  bAig_Manager_t *bmgr,
  bAigEdge_t baig
)
{
  bAigEdge_t right, left;
  int rightLevel, leftLevel;
  if (bAig_AuxInt(bmgr,baig) >= 0) return bAig_AuxInt(bmgr,baig);
  if (bAig_isVarNode(bmgr,baig) || bAig_NodeIsConstant(baig)) return 0;
  right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
  left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);
  Pdtutil_Assert(left!=bAig_NULL && right!=bAig_NULL,"NULL child");

  leftLevel = computeLevel(bmgr, left);
  rightLevel = computeLevel(bmgr, right);
  bAig_AuxInt(bmgr,baig) = leftLevel > rightLevel ? leftLevel + 1 : rightLevel + 1;
  return leftLevel > rightLevel ? leftLevel + 1 : rightLevel + 1;
}

/**Function********************************************************************
  Synopsis    [Looks for ITE constructs and tracks ENABLE signals]
  Description [Looks for ITE constructs and tracks ENABLE signals]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static int
findIte(
  bAig_Manager_t *bmgr,
  bAigEdge_t baig,
  int gateId
)
{

  bAigEdge_t right, left, rr, rl, lr, ll;

  if (bAig_isVarNode(bmgr,baig) || bAig_NodeIsConstant(baig)) return 0;

  right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
  left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);
  Pdtutil_Assert(left!=bAig_NULL && right!=bAig_NULL,"NULL child");

  if (bAig_isVarNode(bmgr,right) || bAig_NodeIsConstant(right)) return 0;
  if (bAig_isVarNode(bmgr,left) || bAig_NodeIsConstant(left)) return 0;
  if (!bAig_NodeIsInverted(right)) return 0;
  if (!bAig_NodeIsInverted(left)) return 0;

  rr = rl = lr = ll = bAig_NULL;

  /* negation taken to account for OR = !AND(! ,! ) */
  rr = bAig_Not(bAig_NodeReadIndexOfRightChild(bmgr,right));
  rl = bAig_Not(bAig_NodeReadIndexOfLeftChild(bmgr,right));
  lr = bAig_Not(bAig_NodeReadIndexOfRightChild(bmgr,left));
  ll = bAig_Not(bAig_NodeReadIndexOfLeftChild(bmgr,left));

  if (rr == bAig_Not(lr) && rl != ll)  {
      return 1;
    /*  ite(rr,rl,ll) */
  } else if (rr == bAig_Not(ll) && rl != lr) {
      return 2;
    /*  ite(rr,rl,lr) */
  }
  else if (rl == bAig_Not(lr) && ll != rr) {
      return 3;
    /*  ite(rl,rr,ll) */
  }
  else if (rl == bAig_Not(ll) && lr != rr) {
      return 4;
    /*  ite(rl,rr,lr) */
  }

  return 0;
}

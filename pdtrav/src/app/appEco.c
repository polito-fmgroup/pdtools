/**CFile***********************************************************************

  FileName    [appIso.c]

  PackageName [app]

  Synopsis    [Functions to handle ECO (Engineering Change Orders) rectification]

  Description []

  SeeAlso     []

  Author      [Gianpiero Cabodi]

  Copyright [  Copyright (c) 2001 by Politecnico di Torino.
    All Rights Reserved. This software is for educational purposes only.
    Permission is given to academic institutions to use, copy, and modify
    this software and its documentation provided that this introductory
    message is not removed, that this software and its documentation is
    used for the institutions' internal research and educational purposes,
    and that no monies are exchanged. No guarantee is expressed or implied
    by the distribution of this code.
    Send bug-reports and/or questions to: cabodi@polito.it. ]

  Revision    []

******************************************************************************/

#include "appInt.h"

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

static Ddi_Vararray_t *
ecoPerformItp(
  Ddi_Bddarray_t *netF,
  Ddi_Bddarray_t *netG,
  Ddi_Bddarray_t *patchF,
  int strategy,
  int *divWeights,
  int nw,
  int nMUSiter
);

int *readWeights(
  char *name,
  int *nwP
);
  
/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Main program of eco app.]
  Description [Main program of eco app.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int App_Eco (
  int argc,
  char *argv[]
) {
  App_Mgr_t *appMgr;
  int iter=1;
  int strategy=0;
  
  char *prefixName = NULL;
  char *gName = NULL;
  char *mapName = NULL;

  printf("eco app:");
  for (int i=0; i<argc; i++) {
    printf(" %s", argv[i]);
    if (strcmp(argv[i],"-h")==0) {
      printf("usage\n");
      printf("iso [-s <strategy>] <prefix>\n");
      printf("<strategy>: 0: base, 1: single itp\n");
    }
    else if (strcmp(argv[i],"-s")==0) {
      i++;
      if (i>=argc) {
        printf("\nmissing strategy value (-s)\n");
        return 0;
      }
      strategy = atoi(argv[i]);
    }
    else if (strcmp(argv[i],"-i")==0) {
      i++;
      if (i>=argc) {
        printf("\nmissing MUS iterations value (-i)\n");
        return 0;
      }
      iter = atoi(argv[i]);
    }
    else if (prefixName==NULL) {
      prefixName = argv[i];
    }
    else {
      printf("\nunknown argument/option: %s\n", argv[i]);
    }
  }
  printf("\n");

  appMgr = App_MgrInit("eco", App_TaskEco_c);

  char name[1000];
  sprintf(name, "%s_F.aig", prefixName);
  Ddi_Bddarray_t *netF = Ddi_AigarrayNetLoadAiger(appMgr->ddiMgr,
                           NULL, name);
  sprintf(name, "%s_G.aig", prefixName);
  Ddi_Bddarray_t *netG = Ddi_AigarrayNetLoadAiger(appMgr->ddiMgr,
                           NULL, name);                         
  Ddi_Bddarray_t *patch = Ddi_BddarrayAlloc(appMgr->ddiMgr,0);
  sprintf(name, "%s_map.tsv", prefixName);
  int nw;
  int *divWeights = readWeights(name, &nw);

  Ddi_Vararray_t *divVars = ecoPerformItp(netF,netG,patch,strategy,divWeights,
                                          nw,iter);

  if (patch!=NULL && (Ddi_BddarrayNum(patch)>0)) {
    sprintf(name, "%s_patch.aig", prefixName);
    Ddi_AigarrayNetStoreAigerWithPis(patch,divVars,0,name);
  }
  Pdtutil_Free(divWeights);
  
  Ddi_Free(divVars);
  Ddi_Free(netF);
  Ddi_Free(netG);
  Ddi_Free(patch);

  App_MgrQuit(appMgr);
  
  return 1;
}



/**Function********************************************************************
  Synopsis    [Function reading divisor weights.]
  Description [Function reading divisor weights.
  The function assumes that divisors are expressed in the right order as they
  appear as outputs in the F aiger file.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int *readWeights(
  char *name,
  int *nwP
)
{
  FILE *fp;
  int *weights=NULL, nw=0;
  char buf[10000];
  int error = 0;
  
  fp = fopen (name,"r");
  if (fp == NULL) {
    fprintf(stderr,"error opening %s\n", name);
    return NULL;
  }
  // count divisors
  while (fgets(buf,1000,fp)!=NULL) {
    if (strncmp(buf,"divisor_co",10)==0) nw++;
  }
  weights = Pdtutil_Alloc(int, nw);
  if (weights != NULL) {
    int i=0, w;
    rewind(fp);
    while (fgets(buf,1000,fp)!=NULL) {
      if (strncmp(buf,"divisor_co",10)==0) {
        int nr = sscanf(buf,"%*s%*d%*d%*s%d",&w); 
        if (nr!=1) {
          fprintf(stderr,"format error reading %s\n", name);
          error = 1;
          break;
        }
        weights[i++]=w;
      }
    }
  }
  else {
    error=1;
  }
  if (error) {
    Pdtutil_Free(weights);
    weights=NULL; nw=0;
  }
  fclose(fp);
  *nwP = nw;
  return weights;
} 

/**Function********************************************************************
  Synopsis    [Function performing eco interpolant iteration.]
  Description [Function performing eco interpolant iteration.
               One interpolant per target is done and optimized.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bdd_t *
ecoMUS(
  Ddi_Bdd_t *A,
  Ddi_Bdd_t *B,
  Ddi_Vararray_t *usedDivVars,
  Ddi_Vararray_t *divVars,
  int *divWeights,
  int nMUSiter
) {
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(A);
  Ddi_Vararray_t *varsAB = Ddi_BddSuppVararray(A);
  Ddi_Vararray_t *varsB = Ddi_BddSuppVararray(B);

  Ddi_VararrayIntersectAcc(varsAB,varsB);
  Ddi_Free(varsB);

  int sortDescending = 1; // high costs are on last variables (swap order)
  if (sortDescending) {
    int nv = Ddi_VararrayNum(varsAB);
    for (int i=0; i<nv/2; i++) {
      Ddi_Var_t *v0 = Ddi_VararrayRead(varsAB,i);
      Ddi_Var_t *v1 = Ddi_VararrayRead(varsAB,nv-1-i);
      Ddi_VararrayWrite(varsAB,i,v1);
      Ddi_VararrayWrite(varsAB,nv-1-i,v0);
    }
  }

  Ddi_Vararray_t *cutVarsA =
    Ddi_VararrayMakeNewVars (varsAB, "PDT_ECO_Cut", NULL, 1);
  Ddi_Vararray_t *enVarsA =
    Ddi_VararrayMakeNewVars (varsAB, "PDT_ECO_En", NULL, 1);

  Ddi_Bdd_t *auxA = Ddi_BddSubstVars(A,varsAB,cutVarsA);

  int nVars = Ddi_VararrayNum(varsAB);
  unsigned char *enAbstr = Pdtutil_Alloc(unsigned char, nVars);
  Ddi_VararrayWriteMark (usedDivVars, 1);
  // mark div vars already used in previous targets
  for (int i=0; i<nVars; i++) {
    enAbstr[i] = (unsigned char) !Ddi_VarReadMark(Ddi_VararrayRead(varsAB,i));
  }
  Ddi_VararrayWriteMark (usedDivVars, 0);
  for (int i=0; i<Ddi_VararrayNum(divVars); i++) {
    Ddi_Var_t *dv = Ddi_VararrayRead(divVars,i);
    Ddi_VarWriteMark(dv,divWeights[i]);
  }
  int *usedDivWeights = Pdtutil_Alloc(int, nVars);
  for (int i=0; i<nVars; i++) {
    Ddi_Var_t *dv = Ddi_VararrayRead(varsAB,i);
    usedDivWeights[i] = Ddi_VarReadMark(dv);
  }
  Ddi_VararrayWriteMark (divVars, 0);
  
  int doAbort=0;
  Ddi_MgrSetAigDynAbstrStrategy(ddm, nMUSiter);
  Ddi_Bdd_t *newA = Ddi_AigAbstrVarsForInterpolant22(auxA, B,
    cutVarsA, varsAB, enVarsA, NULL, enAbstr, usedDivWeights, &doAbort);

  Pdtutil_Free(enAbstr);
  Pdtutil_Free(usedDivWeights);
  Ddi_Free(auxA);
  Ddi_Free(cutVarsA);
  Ddi_Free(enVarsA);
  Ddi_Free(varsAB);
  return newA;  
}

/**Function********************************************************************
  Synopsis    [Function performing eco interpolant iteration.]
  Description [Function performing eco interpolant iteration.
               One interpolant per target is done and optimized.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bddarray_t *
ecoIterateItp(
  Ddi_Bdd_t *miter,
  Ddi_Vararray_t *targetA,
  Ddi_Vararray_t *auxTargetA,
  Ddi_Vararray_t *piA,
  Ddi_Vararray_t *auxPiA,
  Ddi_Vararray_t *divVars,
  int *divWeights,
  int nMUSiter
) {
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(miter);
  int nt = Ddi_VararrayNum(targetA);
  Ddi_Bddarray_t *patchF=Ddi_BddarrayAlloc(ddm,nt);
  Ddi_MgrSetAigAbcOptLevel(ddm,4);
  int useGenClauses = 0;
  
  Ddi_Vararray_t *usedDivVars = Ddi_VararrayAlloc(ddm,0); 

  for (int i=nt-1; i>=0; i--) {
    Ddi_Bdd_t *miter_i = Ddi_BddDup(miter);
    Ddi_Var_t *t_i = Ddi_VararrayRead(targetA,i);

    for (int j=0; j<i; j++) {
      // for (int j=0; j<nt; j++) {
      // if (j==i) continue;
      Ddi_Var_t *t_j = Ddi_VararrayRead(targetA,j);
      Ddi_Bdd_t *m_0 = Ddi_BddCofactor(miter_i,t_j,0);
      Ddi_Bdd_t *m_1 = Ddi_BddCofactor(miter_i,t_j,1);
      Ddi_Free(miter_i);
      miter_i = Ddi_BddAnd(m_0,m_1);
      Ddi_Free(m_0);
      Ddi_Free(m_1);
    }
    Ddi_Bdd_t *A = Ddi_BddCofactor(miter_i,t_i,0);
    Ddi_Bdd_t *B = Ddi_BddCofactor(miter_i,t_i,1);
    Ddi_BddSubstVarsAcc(B, piA, auxPiA);

    //    assert(!Ddi_AigSatAnd(A,B,0));

    Ddi_Bdd_t *newA = ecoMUS(A,B,usedDivVars,divVars,divWeights,nMUSiter);
    Ddi_Free(A); A = newA;
    
    int sat=0;
    Ddi_Bdd_t *itp = Ddi_AigSat22AndWithInterpolant(NULL,A,B,NULL,
           NULL, NULL,NULL,0,NULL,NULL, &sat, 0, 1, 0, -1.0);
    assert(!sat);
    int size0 = Ddi_BddSize(itp);
    int again=size0>0, k=0;
    while (again) {
      if (1 && (k<5)) {
        Ddi_Bdd_t *itp2;
        if (useGenClauses) {
          int res=0;     
          itp2 = Ddi_AigInterpolantByGenClausesSimple(itp,B,1000,&res);
          if (res) {
            Ddi_AigAbcOptAcc(itp2, -1.0);
            if (Ddi_BddSize(itp2)<Ddi_BddSize(itp)) {
              Ddi_Free(itp);
              itp=Ddi_BddDup(itp2);
            }
            Ddi_Free(itp2);
          }
        }
        itp2 = Ddi_AigSat22AndWithInterpolant(NULL,itp,B,NULL,
                   NULL, NULL,NULL,0,NULL,NULL, &sat, 0, 1, 0, -1.0);
        if (Ddi_BddSize(itp2)<Ddi_BddSize(itp)) {
          Ddi_Free(itp);
          itp=Ddi_BddDup(itp2);
        }
        Ddi_Free(itp2);
      }
      Ddi_AigOptByMonotoneCoreAcc(itp, B, NULL, 0, -1.0);
      Ddi_AigOptByMonotoneCoreAcc(itp, A, NULL, 1, -1.0);
      Ddi_AigAbcOptAcc(itp, -1.0);
      int size1 = Ddi_BddSize(itp);
      again = size1<size0 && k++<10;
      size0=size1;
    }
    Ddi_Vararray_t *divSupp = Ddi_BddSuppVararray(itp);
    Ddi_VararrayUnionAcc(usedDivVars,divSupp);
    Ddi_Free(divSupp);
    
    Ddi_BddarrayWrite(patchF,i,itp);
    // apply patch
    Ddi_Vararray_t *tA_i = Ddi_VararrayAlloc(ddm,1);
    Ddi_VararrayWrite(tA_i,0,t_i);
    Ddi_Bddarray_t *pA_i = Ddi_BddarrayAlloc(ddm,1);
    Ddi_BddarrayWrite(pA_i,0,itp);
    Ddi_BddComposeAcc(miter,tA_i,pA_i);

    Ddi_Free(tA_i);
    Ddi_Free(pA_i);
    Ddi_Free(itp);
    Ddi_Free(A);
    Ddi_Free(B);

    Ddi_Free(miter_i);
  }
  Ddi_AigarrayAbcOptAcc(patchF, -1.0);  
  Ddi_Free(usedDivVars);
  
  return patchF;
}

/**Function********************************************************************
  Synopsis    [Function performing eco interpolant iteration.]
  Description [Function performing eco interpolant iteration.
               One interpolant per target is done and optimized.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Bddarray_t *
ecoSingleItp(
  Ddi_Bdd_t *miter,
  Ddi_Vararray_t *targetA,
  Ddi_Vararray_t *auxTargetA,
  Ddi_Vararray_t *piA,
  Ddi_Vararray_t *auxPiA
) {
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(miter);
  int nt = Ddi_VararrayNum(targetA);
  Ddi_Bddarray_t *patchF=Ddi_BddarrayAlloc(ddm,nt);
  Ddi_MgrSetAigAbcOptLevel(ddm,4);

  Ddi_Vararray_t *enTargetA =
    Ddi_VararrayMakeNewVars (targetA, "PDT_ECO_EnTarget", NULL, 1);
  Ddi_Bddarray_t *enTargetLits = Ddi_BddarrayMakeLiteralsAig(enTargetA, 1);

  unsigned char disabledTarget[nt] = {(unsigned char)0};
  int selectedTargets[nt]={0};
  
  for (int t=0; t<nt; t++) {
    // for each iteration select one target
    Ddi_Bdd_t *enableTarget = Ddi_BddMakeConstAig(ddm, 1);
    Ddi_Bdd_t *A = Ddi_BddMakeConstAig(ddm, 0);
    Ddi_Bdd_t *B = Ddi_BddMakeConstAig(ddm, 0);
    int prev_i=nt;
    for (int i=nt-1; i>=0; i--) {
      if (disabledTarget[i]) continue;
      
      Ddi_Bdd_t *miter_i = Ddi_BddDup(miter);
      Ddi_Var_t *t_i = Ddi_VararrayRead(targetA,i);

      for (int j=0; j<nt; j++) {
        if (j==i || disabledTarget[j]) continue;

        Ddi_Var_t *t_j = Ddi_VararrayRead(targetA,j);
        Ddi_Bdd_t *m_0 = Ddi_BddCofactor(miter_i,t_j,0);
        Ddi_Bdd_t *m_1 = Ddi_BddCofactor(miter_i,t_j,1);
        Ddi_Free(miter_i);
        miter_i = Ddi_BddAnd(m_0,m_1);
        Ddi_Free(m_0);
        Ddi_Free(m_1);
      }
      if (prev_i<nt) {
        Ddi_BddDiffAcc(enableTarget,Ddi_BddarrayRead(enTargetLits,prev_i));
      }
      Ddi_Bdd_t *enabledMiter_i = Ddi_BddAnd(miter_i,enableTarget);
      Ddi_BddAndAcc(enabledMiter_i,Ddi_BddarrayRead(enTargetLits,i));
      Ddi_Bdd_t *A_i = Ddi_BddCofactor(enabledMiter_i,t_i,0);
      Ddi_Bdd_t *B_i = Ddi_BddCofactor(enabledMiter_i,t_i,1);
      //      assert(!Ddi_AigSatAnd(A_i,B_i,0));
      Ddi_BddOrAcc(A,A_i);
      Ddi_BddOrAcc(B,B_i);

      prev_i = i;
      Ddi_Free(miter_i);
      Ddi_Free(A_i);
      Ddi_Free(B_i);
      Ddi_Free(enabledMiter_i);
    }
    Ddi_BddSubstVarsAcc(B, piA, auxPiA);

    assert(!Ddi_AigSatAnd(A,B,0));

    Ddi_Bdd_t *newA = ecoMUS(A,B,NULL,NULL,NULL,8);
    Ddi_Free(A); A = newA;
    
    int sat=0;
    Ddi_Bdd_t *itp = Ddi_AigSat22AndWithInterpolant(NULL,A,B,NULL,
                                                    NULL, NULL,NULL,0,NULL,NULL, &sat, 0, 1, 0, -1.0);
    assert(!sat);
    int size0 = Ddi_BddSize(itp);
    int again=size0>0, k=0;
    while (again) {
      if (1 && (k<5)) {
        Ddi_Bdd_t *itp2 = Ddi_AigSat22AndWithInterpolant(NULL,itp,B,NULL,
                                                         NULL, NULL,NULL,0,NULL,NULL, &sat, 0, 1, 0, -1.0);
        if (Ddi_BddSize(itp2)<Ddi_BddSize(itp)) {
          Ddi_Free(itp);
          itp=Ddi_BddDup(itp2);
        }
        Ddi_Free(itp2);
      }
      Ddi_AigOptByMonotoneCoreAcc(itp, B, NULL, 0, -1.0);
      Ddi_AigOptByMonotoneCoreAcc(itp, A, NULL, 1, -1.0);
      Ddi_AigAbcOptAcc(itp, -1.0);
      int size1 = Ddi_BddSize(itp);
      again = size1<size0 && k++<10;
      size0=size1;
    }

    /* we use one hot encoding: start with all zeroes */
    Ddi_Bddarray_t *oneHotSel = Ddi_BddarrayMakeConstAig(ddm, nt, 0);
    Ddi_Bdd_t *max_p_i = NULL;
    int max_i = -1;
    for (int i=nt-1; i>=0; i--) {
      if (disabledTarget[i]) continue;
      Ddi_BddNotAcc(Ddi_BddarrayRead(oneHotSel,i));
      //  assign selection variables with one hot encoding
      Ddi_Bdd_t *p_i = Ddi_BddCompose(itp,enTargetA,oneHotSel);
      Ddi_Bdd_t *myA_i = Ddi_BddCompose(A,enTargetA,oneHotSel);
      Ddi_Bdd_t *myB_i = Ddi_BddCompose(B,enTargetA,oneHotSel);
      //      assert(!Ddi_AigSatAnd(myA_i,myB_i,0));
      Ddi_Free(myA_i);
      Ddi_Free(myB_i);
      if (max_i<0 || Ddi_BddSize(p_i)>Ddi_BddSize(max_p_i)) {
        Ddi_Free(max_p_i); max_p_i = Ddi_BddDup(p_i);
        max_i = i; 
      }   
      Ddi_BddNotAcc(Ddi_BddarrayRead(oneHotSel,i));
      Ddi_Free(p_i);
    }
    Ddi_Free(itp);
    Ddi_Free(oneHotSel);
    Ddi_BddarrayWrite(patchF,max_i,max_p_i);
    selectedTargets[t]=max_i;
    /* now apply selected patch func */
    disabledTarget[max_i] = (unsigned char)1;
    Ddi_Vararray_t *tA_i = Ddi_VararrayAlloc(ddm,1);
    Ddi_Var_t *t_i = Ddi_VararrayRead(targetA,max_i);
    Ddi_VararrayWrite(tA_i,0,t_i);
    Ddi_Bddarray_t *pA_i = Ddi_BddarrayAlloc(ddm,1);
    Ddi_BddarrayWrite(pA_i,0,max_p_i);
    Ddi_BddComposeAcc(miter,tA_i,pA_i);
    Ddi_Free(tA_i);
    Ddi_Free(pA_i);

    Ddi_Free(max_p_i);    
    Ddi_Free(enableTarget);
    Ddi_Free(A);
    Ddi_Free(B);
  }
  Ddi_AigarrayAbcOptAcc(patchF, -1.0);  
  
  Ddi_Free(enTargetA);
  Ddi_Free(enTargetLits);

  printf("selected targets are:");
  for (int t=0; t<nt; t++) {
    printf(" %d", selectedTargets[t]);
  }
  printf("\n");
  
  return patchF;
}


/**Function********************************************************************
  Synopsis    [Function performing eco patch.]
  Description [Function performing eco patch.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
static Ddi_Vararray_t *
ecoPerformItp(
  Ddi_Bddarray_t *netF,
  Ddi_Bddarray_t *netG,
  Ddi_Bddarray_t *patchF,
  int strategy,
  int *divWeights,
  int nw,
  int nMUSiter
) {
  Ddi_Mgr_t *ddm = Ddi_ReadMgr(netF);
  Ddi_Vararray_t *piA = Ddi_BddarraySuppVararray(netG);
  Ddi_Vararray_t *piPlusA = Ddi_BddarraySuppVararray(netF);
  Ddi_Vararray_t *targetA = Ddi_VararrayDiff(piPlusA,piA);
  
  int no = Ddi_BddarrayNum(netG);
  int nd = Ddi_BddarrayNum(netF)-no;
  int ni = Ddi_VararrayNum(piA);
  int nt = Ddi_VararrayNum(targetA);
  
  if (nw!=nd) {
    printf("mismatch in numbers of divisors/weights\n");
    Ddi_Free(piA);
    Ddi_Free(piPlusA);
    Ddi_Free(targetA);
    return NULL;
  }

  Ddi_Vararray_t *auxPiA =
    Ddi_VararrayMakeNewVars (piA, "PDT_ECO_AuxPi", NULL, 1);
  Ddi_Vararray_t *auxTargetA =
    Ddi_VararrayMakeNewVars (targetA, "PDT_ECO_AuxTarget", NULL, 1);


  // make miter
  Ddi_Bdd_t *miter = Ddi_BddMakeConstAig(ddm, 0);
  Ddi_Bddarray_t *netFAux = Ddi_BddarraySubstVars(netF,targetA,auxTargetA); 
  for (int i=0; i<no; i++) {
    Ddi_Bdd_t *m_i = Ddi_BddXor(Ddi_BddarrayRead(netF,i),Ddi_BddarrayRead(netG,i));
    Ddi_BddOrAcc(miter,m_i);
    Ddi_Free(m_i);
  }
  // create divisor vars and divisor relations
  Ddi_Vararray_t *divVars = Ddi_VararrayAlloc(ddm, 0);
  Ddi_Bdd_t *divRel = Ddi_BddMakeConstAig(ddm, 1);
  Ddi_Bddarray_t *divA = Ddi_BddarrayAlloc(ddm,0);
  for (int i=0; i<nd; i++) {
    char name[1000];
    Ddi_Bdd_t *div;
    sprintf(name,"Pdtrav_Div_Var_%d", i);
    Ddi_Var_t *div_i = Ddi_VarFromName(ddm, name);
    if (div_i == NULL) {
      div_i = Ddi_VarNewBaig(ddm,name);
    }
    Ddi_VararrayInsertLast(divVars,div_i);
    Ddi_Bdd_t *divR_i = Ddi_BddMakeLiteralAig(div_i,1);
    Ddi_BddarrayWrite(divA,i,Ddi_BddarrayRead(netF,no+i));
    Ddi_BddXnorAcc(divR_i,Ddi_BddarrayRead(netF,no+i));
    Ddi_BddAndAcc(divRel,divR_i);
    Ddi_Free(divR_i);
  }
  Ddi_BddAndAcc(miter,divRel);

  // now compute patches
  Ddi_Bddarray_t *myPatchF = strategy==0 ?
    ecoIterateItp(miter,targetA,auxTargetA,piA,auxPiA,
                  divVars,divWeights,nMUSiter) :
    ecoSingleItp(miter,targetA,auxTargetA,piA,auxPiA);

  if (myPatchF != NULL) {
    // check patch
    Ddi_BddComposeAcc(miter,divVars,divA);
    assert(Ddi_BddIsZero(miter) || !Ddi_AigSat(miter));
    for (int i=0; i<nt; i++) {
      Ddi_BddarrayWrite(patchF,i,Ddi_BddarrayRead(myPatchF,i));      
    }
    Ddi_Free(myPatchF);
  }
  
  Ddi_Free(netFAux);
  Ddi_Free(divRel);
  Ddi_Free(divA);
  Ddi_Free(miter);
  Ddi_Free(piA);
  Ddi_Free(auxPiA);
  Ddi_Free(piPlusA);
  Ddi_Free(targetA);
  Ddi_Free(auxTargetA);
  return divVars;
}

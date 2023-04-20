/**CFile****************************************************************

  FileName    [ddiAbcLink.c]

  PackageName [ddi]

  Synopsis    [Aig based model check routines]

  Description []

  SeeAlso     []

  Author      [Gianpiero Cabodi and Stefano Quer]

  Affiliation []

  Date        []

  Copyright [This file was created at the Politecnico di Torino,
    Torino, Italy.
    The  Politecnico di Torino makes no warranty about the suitability of
    this software for any purpose.
    It is presented on an AS IS basis.
  ]

  Revision  []

***********************************************************************/

#include "ddi.h"
#include "fsm.h"
#include <time.h>
#include <pthread.h>
#include "gia.h"
#include "fsm.h"
#include "intInt.h"
#include "baigInt.h"
#include "opt/dar/dar.h"
#include "base/main/main.h"
#include "proof/ssw/ssw.h"
#include "abc_global.h"
#include "abc.h"

EXTERN void   Abc_Start();
EXTERN void   Abc_Stop();
EXTERN Aig_Man_t * Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );//EXTERN void * Abc_FrameGetGlobalFrame();
//EXTERN int    Cmd_CommandExecute( void * pAbc, char * sCommand );
EXTERN Abc_Ntk_t * Abc_NtkFromAigPhase( Aig_Man_t * pMan );
EXTERN int    Abc_GetNtkLevelNum(void * pAbc);
EXTERN int    Abc_GetNtkNodeNum(void * pAbc);
EXTERN int Abc_GetNtkLatchesNum(Abc_Frame_t * pAbc);
EXTERN int Abc_GetFrameStatus(Abc_Frame_t * pAbc);
EXTERN Gia_Man_t * Abs_RpmPerform( Gia_Man_t * p, int nCutMax, int fVerbose, int fVeryVerbose );
EXTERN Gia_Man_t * Gia_ManCompress2( Gia_Man_t * p, int fUpdateLevel, int fVerbose );
EXTERN Gia_Man_t * Gia_ManFromAig(Aig_Man_t * p);
EXTERN Aig_Man_t * Gia_ManToAig(Gia_Man_t *, int);
EXTERN void * Abc_FrameAllocate();
EXTERN void Abc_FrameInit( void * );

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define FILE1 "./tmpFile"
#define FILE2 "./tmpFileOpt"
//#define FILE1 "/tmp/tmpFile"
//#define FILE2 "/tmp/tmpFileOpt"


/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

struct Ddi_AigAbcInfo_s {
  Ddi_Mgr_t *ddiMgr;
  int abcCnfMaxId;        //Massima etichetta assegnata alle variabili da ABC
  int *abc2PdtIdMap;      //Vettore di mapping per i cnf id da quelli di ABC a quelli di PDT
  int *objId2gateId;
  Ddi_Bddarray_t *fA;     //I BDD che rappresentano la TR
  Ddi_Vararray_t *vars;   //Le varaibili di input (input primari) della TR
  Aig_Man_t *aigAbcMan;   //AIG manager di ABC
  Cnf_Dat_t *pCnf;        //Oggetto di ABC che mantiene le informazioni sulla CNF (numero variabili, clausole, ecc)
  bAig_array_t *visitedNodes; //Gate visitati nella nostra rappresentazione
  bAig_array_t *varNodes; 
  Aig_Obj_t **visitedAbcObj; //Gate visitati nella rappresentazione di ABC
};

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

extern float cpuTimeScaleFactor;

static pthread_mutex_t abcSharedUse;

static int abcStarted = 0;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#define MY_CLOCKS_PER_SEC \
(cpuTimeScaleFactor < 0 ? CLOCKS_PER_SEC : \
 ((long)(CLOCKS_PER_SEC*cpuTimeScaleFactor)))


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static int ddiAbcOptAccInt (int, int, char *, char *, float);
static int ddiAigarrayAbcOptWithLevelAcc (Ddi_Bddarray_t *aigA, int optLevel, float timeLimit);
static int ddiAbcOptWithLevelAcc (Ddi_Bdd_t *aig, int optLevel, float timeLimit);

static inline bAigEdge_t Aig_ObjChild0Baig (Aig_Obj_t *pObj)  { assert(!Aig_IsComplement(pObj)); return Aig_ObjFanin0(pObj)? Aig_NotCond((bAigEdge_t)Aig_ObjFanin0(pObj)->iData, Aig_ObjFaninC0(pObj)) : bAig_NULL; }
static inline bAigEdge_t Aig_ObjChild1Baig (Aig_Obj_t *pObj)  { assert(!Aig_IsComplement(pObj)); return Aig_ObjFanin1(pObj)? Aig_NotCond((bAigEdge_t)Aig_ObjFanin1(pObj)->iData, Aig_ObjFaninC1(pObj)) : bAig_NULL; }
static inline int Aig_ObjChild0Id (Aig_Obj_t *pObj)  { assert(!Aig_IsComplement(pObj)); return Aig_ObjFanin0(pObj)? Aig_NotCond((int)Aig_ObjFanin0(pObj)->Id, Aig_ObjFaninC0(pObj)) : -1; }
static inline int Aig_ObjChild1Id (Aig_Obj_t *pObj)  { assert(!Aig_IsComplement(pObj)); return Aig_ObjFanin1(pObj)? Aig_NotCond((int)Aig_ObjFanin1(pObj)->Id, Aig_ObjFaninC1(pObj)) : -1; }
static Aig_Man_t *abcBddToAig (Ddi_Bdd_t *f, Ddi_Vararray_t *vars);
static Aig_Man_t *abcBddarrayToAig (Ddi_Bddarray_t *f, bAig_array_t *varNodes,bAig_array_t *visitedNodes);
static Ddi_Bdd_t *abcBddFromAig (Aig_Man_t *pAig, Ddi_Vararray_t *vars, Ddi_Bdd_t *refAig);
static int *abcLatchEquivClasses(Aig_Man_t * pAig);

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
void
Ddi_AbcLock (void)
{
  pthread_mutex_lock(&abcSharedUse);
  if (!abcStarted) {
    Abc_Start();
    abcStarted=1;
  }
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Ddi_AbcLocked (void)
{
  if (pthread_mutex_trylock(&abcSharedUse)!=0) return 1;
  else {
    pthread_mutex_unlock(&abcSharedUse);
    return 0;
  }
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_AbcStop (void)
{
  pthread_mutex_lock(&abcSharedUse);
  if (abcStarted) {
    Abc_Stop();
    abcStarted=0;
  }
  pthread_mutex_unlock(&abcSharedUse);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_AbcUnlock (void)
{
  pthread_mutex_unlock(&abcSharedUse);
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Ddi_AigAbcInfoFree (
  Ddi_AigAbcInfo_t *aigAbcInfo
)
{
  if (aigAbcInfo==NULL) return;

  Ddi_Free(aigAbcInfo->fA);
  Ddi_Free(aigAbcInfo->vars);

  bAigArrayFree(aigAbcInfo->visitedNodes);
  bAigArrayFree(aigAbcInfo->varNodes);
  Aig_ManStop(aigAbcInfo->aigAbcMan);
  Cnf_DataFree(aigAbcInfo->pCnf);

  Pdtutil_Free(aigAbcInfo->visitedAbcObj);
  Pdtutil_Free(aigAbcInfo->abc2PdtIdMap);
  Pdtutil_Free(aigAbcInfo->objId2gateId);

  Pdtutil_Free(aigAbcInfo);
}

/**Function*************************************************************
  Synopsis    [Reproduces script "compress2Pdt".]
  Description []               
  SideEffects []
  SeeAlso     []

***********************************************************************/
Aig_Man_t * Dar_ManCompress2Pdt (
  Aig_Man_t * pAig, 
  float abcTimeLimit,
  int optLevel,
  int fVerbose)
  //alias compress2   "b -l; rw -l; rf -l; b -l; rw -l; rwz -l; b -l; rfz -l; rwz -l; b -l"
{
  Aig_Man_t * pTemp;
  int fBalance = 1;
  int fUpdateLevel = 1;
  int timeInit, timeFinal;
  float OKgain = 0.90;
  float TotOKgain = 0.80;
  int size0, size1, totSize0, totSize1;

  Dar_RwrPar_t ParsRwr, * pParsRwr = &ParsRwr;
  Dar_RefPar_t ParsRef, * pParsRef = &ParsRef;

  if (!abcStarted) {
    Abc_Start();
    abcStarted=1;
  }

  timeInit = clock();

  Dar_ManDefaultRwrParams( pParsRwr );
  Dar_ManDefaultRefParams( pParsRef );
  
  pParsRwr->fUpdateLevel = fUpdateLevel;
  pParsRef->fUpdateLevel = fUpdateLevel;
  
  pParsRwr->fVerbose = 0;//fVerbose;
  pParsRef->fVerbose = 0;//fVerbose;
  
  pAig = Aig_ManDupDfs( pAig ); 
  if ( fVerbose ) Aig_ManPrintStats( pAig );

  do {
    totSize0 = Aig_ManNodeNum(pAig);
    // balance
    if ( fBalance ) {
      do {
	size0 = Aig_ManNodeNum(pAig);
	
	pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel);
	Aig_ManStop( pTemp );
	size1 = Aig_ManNodeNum(pAig);
	
	if (abcTimeLimit >= 0 &&
	    (clock() - timeInit) > abcTimeLimit*MY_CLOCKS_PER_SEC) break;
      } while (size0*OKgain > size1);
    }

    if (optLevel <= 1 || (abcTimeLimit>=0 &&
        (clock() - timeInit) > abcTimeLimit*MY_CLOCKS_PER_SEC)) break;

    if (0) {
      // csweep
      pAig = Csw_Sweep( pTemp = pAig, 8, 6, 0 );
      if (Aig_ManNodeNum(pAig) < Aig_ManNodeNum(pTemp)) {
	printf ("CSWEEP: %d -> %d\n",
		Aig_ManNodeNum(pTemp), Aig_ManNodeNum(pAig));
      }
      Aig_ManStop( pTemp );
      
      pAig = Aig_ManDupDfs( pTemp = pAig ); 
      Aig_ManStop( pTemp );
    }

    // rewrite
    //    Dar_ManRewrite( pAig, pParsRwr );
    pParsRwr->fUpdateLevel = 0;  // disable level update
    Dar_ManRewrite( pAig, pParsRwr );
    pParsRwr->fUpdateLevel = fUpdateLevel;  // reenable level update if needed
  
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );
  
    if (abcTimeLimit >= 0 &&
      (clock() - timeInit) > abcTimeLimit*MY_CLOCKS_PER_SEC) break;

    // refactor
    Dar_ManRefactor( pAig, pParsRef );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );
  
    if (optLevel <= 1 || (abcTimeLimit>=0 &&
        (clock() - timeInit) > abcTimeLimit*MY_CLOCKS_PER_SEC)) break;

    // balance
    //    if ( fBalance )
    {
      pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
      Aig_ManStop( pTemp );
      if ( fVerbose ) Aig_ManPrintStats( pAig );
    }
  
#if 0
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );
#endif    
    pParsRwr->fUseZeros = 1;
    pParsRef->fUseZeros = 1;
    
    if (optLevel <= 1 || (abcTimeLimit>=0 &&
        (clock() - timeInit) > abcTimeLimit*MY_CLOCKS_PER_SEC)) break;

    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );
    
    // balance
    if ( fBalance )
      {
	pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
	Aig_ManStop( pTemp );
	if ( fVerbose ) Aig_ManPrintStats( pAig );
      }
  
    if (optLevel <= 1 || (abcTimeLimit>=0 &&
        (clock() - timeInit) > abcTimeLimit*MY_CLOCKS_PER_SEC)) break;

    // refactor
    Dar_ManRefactor( pAig, pParsRef );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );
    
    if (optLevel <= 1 || (abcTimeLimit>=0 &&
        (clock() - timeInit) > abcTimeLimit*MY_CLOCKS_PER_SEC)) break;

    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );
    
    // balance
    if ( fBalance )
      {
	pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
	Aig_ManStop( pTemp );
	if ( fVerbose ) Aig_ManPrintStats( pAig );
      }
    
    if (optLevel <= 1 || (abcTimeLimit>=0 &&
        (clock() - timeInit) > abcTimeLimit*MY_CLOCKS_PER_SEC)) break;

    totSize1 = Aig_ManNodeNum(pAig);
  } while (totSize0*TotOKgain > totSize1);

  return pAig;
}

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/

int
ddiAbcOptAcc (
  Ddi_Bdd_t *aig,
  float timeLimit
  )
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr (aig);
  char cmd[200];
  int ret, size0;
  int directToAbc = 1;
  int verbose=0;
  int chkRes=0;
  int useGia = 0;
  int optLevel = Ddi_MgrReadAigAbcOptLevel(ddm);

  if (Ddi_MgrReadVerbosity(ddm) > Pdtutil_VerbLevelDevMin_c) {
    verbose = 1;
  }
  if (Ddi_MgrReadVerbosity(ddm) > Pdtutil_VerbLevelDevMin_c) {
    verbose = 2;
  }

  size0 = Ddi_BddSize(aig);

  if (size0 < 3) return 1;

  if (optLevel == 0) {
    return (1);
  }

  if (Ddi_BddIsPartConj(aig) || Ddi_BddIsPartDisj(aig)) {
    int i;
    Ddi_Bddarray_t *fA = Ddi_BddarrayMakeFromBddPart(aig);
    int ret = Ddi_AigarrayAbcOptAcc (fA,timeLimit);
    for (i=0; i<Ddi_BddarrayNum(fA); i++) {
      Ddi_BddPartWrite(aig,i,Ddi_BddarrayRead(fA,i));
    }
    Ddi_Free(fA);
    return ret;
  }

  if (size0 > 100000) {
    /* force level 1 (just balance) */
    optLevel = 1;
  }

  pthread_mutex_lock(&abcSharedUse);

  if (directToAbc) {
    Aig_Man_t *aigAbcOpt;
    Ddi_Bdd_t *aigOpt;
    Ddi_Vararray_t *vA = Ddi_BddSuppVararray(aig);
    Aig_Man_t *aigAbc = abcBddToAig (aig,vA);

    if (useGia) {
      Gia_Man_t *pGiaOpt, *pGia = Gia_ManFromAig(aigAbc);
      Aig_ManStop(aigAbc);

      /* do the job */
      //      printf("GIA\n");

      Gia_ManStop(pGia);
      aigAbc = Gia_ManToAig(pGiaOpt, 0 );
      Gia_ManStop(pGiaOpt);
    }
    else {
      aigAbcOpt = Dar_ManCompress2Pdt (aigAbc, timeLimit,
			     optLevel,verbose);
    }

    aigOpt = abcBddFromAig(aigAbcOpt,vA,aig);

    Aig_ManStop(aigAbc);
    Aig_ManStop(aigAbcOpt);

    if (chkRes) {
      Pdtutil_Assert(Ddi_AigEqualSat(aig,aigOpt),"wrong abc optimization");
    }
    if (1) {
      Pdtutil_VerbosityLocal(Ddi_MgrReadVerbosity(ddm),
			     Pdtutil_VerbLevelDevMax_c,
			     fprintf(stdout, "ABC Optimization: %d -> %d.\n",
        size0, Ddi_BddSize(aigOpt))
      );
      Ddi_DataCopy (aig, aigOpt);
    }

    Ddi_Free(vA);
    Ddi_Free(aigOpt);

    pthread_mutex_unlock(&abcSharedUse);

    return (1);
  }


  CATCH {

    //    assert(0);

    if (Ddi_BddSize(aig) > 100000) {
      /* force level 1 (just balance) */
      ret = ddiAbcOptWithLevelAcc(aig,1,timeLimit);
    }
    else if (Ddi_BddSize(aig) > 50000 && Ddi_MgrReadAigAbcOptLevel(ddm)==1) {
      /* force level 1 (just balance) */
      ret = ddiAbcOptWithLevelAcc(aig,1,timeLimit);
    }

    else {
      ret = ddiAbcOptWithLevelAcc(aig,Ddi_MgrReadAigAbcOptLevel(ddm),timeLimit);
    }


  } FAIL {
    fprintf(stderr, "Error: Caught ABC Error; Result Ignored.\n");
    ret = 0;
  }

  pthread_mutex_unlock(&abcSharedUse);

  sprintf(cmd, "rm -f %s%d_%d.bench %s%d_%d.bench", 
	  FILE1, getpid(), (int)pthread_self(), 
	  FILE2, getpid(), (int)pthread_self());
  system(cmd);
  return ret;
}

/**Function*************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/
Aig_Man_t * 
DdiAbcBddarrayToAig (
  Ddi_Bddarray_t *fA,
  Ddi_Vararray_t *vars
)
{
  bAig_array_t *visitedNodes = bAigArrayAlloc();
  bAig_array_t *varNodes = bAigArrayAlloc();
  Aig_Man_t * pAig;
  int i;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr (fA);
  bAig_Manager_t  *bmgr = Ddi_MgrReadMgrBaig(ddm);

  for (i=0; i<Ddi_BddarrayNum(fA); i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    Ddi_PostOrderAigVisitIntern(bmgr,Ddi_BddToBaig(f_i),visitedNodes,-1);
  }

  Ddi_PostOrderAigClearVisitedIntern(bmgr,visitedNodes);

  for (i=0; i<Ddi_VararrayNum(vars); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(vars,i);
    bAigEdge_t varIndex = Ddi_VarToBaig(v);
    bAigArrayWriteLast(varNodes,varIndex);
  }
  
  pAig = abcBddarrayToAig (fA, varNodes, visitedNodes);

  for (i=0; i<varNodes->num; i++) {
    bAigEdge_t varIndex = varNodes->nodes[i];
    bAig_AuxPtr(bmgr,varIndex) = NULL;
  }
  for (i=0; i<visitedNodes->num; i++) {
    bAigEdge_t baig = visitedNodes->nodes[i];
    bAig_AuxPtr(bmgr,baig) = NULL;
  }

  bAigArrayFree(varNodes);

  bAigArrayFree(visitedNodes);
  
  return pAig;
}

/**Function*************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/
Ddi_AigAbcInfo_t * 
Ddi_BddarrayToAbcCnfInfo (        //Genera l'AIG e la CNF con ABC e predispone le strutture dati per il mapping
  Ddi_Bddarray_t *fA,
  Ddi_Vararray_t *vars,
  bAig_array_t *visitedNodes
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr (fA);
  bAig_Manager_t  *bmgr = Ddi_MgrReadMgrBaig(ddm);
  bAig_array_t *varNodes = bAigArrayAlloc();
  int i,n;
  int staticCnfMan = 1;
  
  //Alloca l'oggettp
  Ddi_AigAbcInfo_t *aigAbcInfo = Pdtutil_Alloc(Ddi_AigAbcInfo_t,1);

  //Inizializza alcuni campi
  aigAbcInfo->ddiMgr = ddm;
  aigAbcInfo->abcCnfMaxId = -1;
  aigAbcInfo->fA = Ddi_BddarrayDup(fA);
  aigAbcInfo->visitedNodes = aigAbcInfo->varNodes = NULL;
  aigAbcInfo->vars = NULL;

  //Se ho passato un vettore di nodi dell'aig lo copio nell'oggetto
  if (visitedNodes!=NULL) {
    aigAbcInfo->visitedNodes = bAigArrayDup(visitedNodes);
  }
  else {
  
    //Se lo genero visitandolo il baig in post order
    aigAbcInfo->visitedNodes = bAigArrayAlloc();

    for (i=0; i<Ddi_BddarrayNum(fA); i++) {
      Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
      Ddi_PostOrderAigVisitIntern(bmgr,Ddi_BddToBaig(f_i),
				aigAbcInfo->visitedNodes,-1);
    }
    Ddi_PostOrderAigClearVisitedIntern(bmgr,aigAbcInfo->visitedNodes);
  } 

  //Converte l'aig della TR nella rappresentazione di ABC
  //  pthread_mutex_lock(&abcSharedUse);

  if (vars!=NULL) {
    aigAbcInfo->vars = Ddi_VararrayDup(vars);
    for (i=0; i<Ddi_VararrayNum(vars); i++) {
      Ddi_Var_t *v = Ddi_VararrayRead(vars,i);
      bAigEdge_t varIndex = Ddi_VarToBaig(v);
      bAigArrayWriteLast(varNodes,varIndex);
    }
  }
  else {
    DdiAigArrayVarsWithCnfInfo(ddm,aigAbcInfo->visitedNodes,varNodes);
  }

  aigAbcInfo->aigAbcMan = abcBddarrayToAig (fA,varNodes,aigAbcInfo->visitedNodes);
  aigAbcInfo->varNodes = bAigArrayDup(varNodes);
  
  //Converte l'AIG in CNF mediante techonology mapping
  if (staticCnfMan) {
    //  pthread_mutex_lock(&abcSharedUse);
    aigAbcInfo->pCnf = Cnf_DeriveOther(aigAbcInfo->aigAbcMan, 1); //Con 1 che succede?  TODO:cambiare
    //  pthread_mutex_unlock(&abcSharedUse);
  }
  else { 
    Cnf_Man_t * s_pManCnf = Cnf_ManStart();
    aigAbcInfo->pCnf = Cnf_DeriveOtherWithMan(s_pManCnf, 
                                              aigAbcInfo->aigAbcMan, 1);
    Cnf_ManStop(s_pManCnf);
    //Con 1 che succede?  TODO:cambiare
  }
  //  pthread_mutex_unlock(&abcSharedUse);

  //Setta il massimo cnf id convertito da abc
  aigAbcInfo->abcCnfMaxId = aigAbcInfo->pCnf->nVars+1;

  //Alloca un vettore per i nodi visitati dell'AIG di ABC
  n = aigAbcInfo->visitedNodes->num;
  aigAbcInfo->visitedAbcObj = Pdtutil_Alloc(Aig_Obj_t *,n);
  for (i=0; i<n; i++) {
    aigAbcInfo->visitedAbcObj[i] = NULL;
  }
  
  //Alloca un vettore per il mapping abc obj id -> gate id
  aigAbcInfo->objId2gateId = Pdtutil_Alloc(int, aigAbcInfo->abcCnfMaxId+1);
  for (i=0; i<aigAbcInfo->abcCnfMaxId; i++) {
    aigAbcInfo->objId2gateId[i] = -1;
  }
  
  //Alloca un vettore per il mapping tra ABC e PDTrav (vettore di puntatori a nodi dell'AIG di ABC, dimensionato sul massimo cnf id convertito da ABC)--->in realtà è usato come vettore di interi (cnfId)
  aigAbcInfo->abc2PdtIdMap = Pdtutil_Alloc(Aig_Obj_t *,aigAbcInfo->abcCnfMaxId+1);
  for (i=0; i<=aigAbcInfo->abcCnfMaxId; i++) {
    aigAbcInfo->abc2PdtIdMap[i] = 0;
  }

  //Scandisce i gate del baig in post order popolando il vettore parallelo di nodi nella rappresentazione di ABC
  for (i=0; i<aigAbcInfo->visitedNodes->num; i++) {
    bAigEdge_t baig = aigAbcInfo->visitedNodes->nodes[i];
    Pdtutil_Assert(aigAbcInfo->visitedAbcObj[i]==NULL,"abc pdt is not null")
    if (bAig_NodeIsConstant(baig)) {
      aigAbcInfo->visitedAbcObj[i] = Aig_ManConst0(aigAbcInfo->aigAbcMan);
    }
    else {
      Aig_Obj_t * pObj = bAig_AuxPtr(bmgr,baig);
      aigAbcInfo->visitedAbcObj[i] = pObj;
      Pdtutil_Assert(Aig_ObjId(pObj) >= 0, "wrong abc obj");
      aigAbcInfo->objId2gateId[Aig_ObjId(pObj)] = i;
    }
    
    bAig_AuxPtr(bmgr,baig) = NULL;
  }
  
  //Ora si hanno due vettori visited paralleli, uno con i nodi del baig e l'altro con i nodi di ABC

   
  for (i=0; i<Ddi_BddarrayNum(fA); i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    Aig_Obj_t *pObj = Aig_ManCo(aigAbcInfo->aigAbcMan,i);
    //    aigAbcInfo->visitedAbcObj[i] = pObj;
  }

  for (i=0; i<varNodes->num; i++) {
    bAigEdge_t varIndex = varNodes->nodes[i];
    bAig_AuxPtr(bmgr,varIndex) = NULL;
  }

  bAigArrayFree(varNodes);

  return aigAbcInfo;
}

/**Function*************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/
bAig_array_t *
Ddi_AbcCnfInfoReadVisitedNodes (
  Ddi_AigAbcInfo_t *aigAbcInfo 
)
{
  return aigAbcInfo->visitedNodes;
}

/**Function*************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/
int
Ddi_AigAbcCnfMaxId (
  Ddi_AigAbcInfo_t *aigAbcInfo 
)
{
  return aigAbcInfo->abcCnfMaxId;
}




/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
int
Ddi_AigAbcCnfSetAbc2PdtMap(
  Ddi_AigAbcInfo_t *aigAbcInfo,
  int abcId,
  int pdtId
)
{
  Pdtutil_Assert(abcId>0&&pdtId!=0,"wrong abc or pdt cnf id");
  Pdtutil_Assert(aigAbcInfo->abcCnfMaxId>=abcId,"wrong max id");
  if (aigAbcInfo->abc2PdtIdMap[abcId]!=0) {
    Pdtutil_Assert(aigAbcInfo->abc2PdtIdMap[abcId]==pdtId,
		   "abc -> pdt cnf re-mapping");
    return 0;
  }
  aigAbcInfo->abc2PdtIdMap[abcId] = pdtId;

  return 1;
}

/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
int
Ddi_AigAbcCnfId (
  Ddi_AigAbcInfo_t *aigAbcInfo,
  int i
)
{
  Aig_Obj_t *pObj;
  Cnf_Dat_t *pCnf;
  int id;

  Pdtutil_Assert(i>=0&&i<aigAbcInfo->visitedNodes->num,"wrong aig2abc index");
  pObj = aigAbcInfo->visitedAbcObj[i];
  Pdtutil_Assert(pObj!=NULL,"NULL abc obj");
  pCnf = aigAbcInfo->pCnf;
  if (!Aig_ObjIsCi(pObj)&&(pCnf->pObj2Count[Aig_ObjId(pObj)]<=0)) {
    return 0;
  }
  id = pObj->Id;
  Pdtutil_Assert(id>0&&id<pCnf->nVars,"invalid abc id");
  return id;
}

/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
Ddi_ClauseArray_t *
Ddi_AigClausesFromAbcCnf (
  Ddi_AigAbcInfo_t *aigAbcInfo,
  int i
)
{
  Ddi_ClauseArray_t *ca;
  Aig_Obj_t *pObj;
  Cnf_Dat_t *pCnf;
  int *pLit, j, iVar, nClauses, iFirstClause;

  Pdtutil_Assert(i>=0&&i<aigAbcInfo->visitedNodes->num,"wrong index");
  
  //Prendo il nodo di aig di abc
  pObj = aigAbcInfo->visitedAbcObj[i];
  Pdtutil_Assert(pObj!=NULL,"NULL abc obj");

  //Prende il numero di clausole e l'indice della prima clausola
  pCnf = aigAbcInfo->pCnf;
  nClauses = pCnf->pObj2Count[Aig_ObjId(pObj)];
  iFirstClause = pCnf->pObj2Clause[Aig_ObjId(pObj)];

  //Alloca il clause array
  ca = Ddi_ClauseArrayAlloc(1);

  Pdtutil_Assert(nClauses>0 && iFirstClause>=0,"wrong abc clause info");
  
  //Scandisce tutte le clausole generate da ABC
  for (j = iFirstClause; j < iFirstClause + nClauses; j++) {
  
    //Alloca una clausola
    Ddi_Clause_t *cl = Ddi_ClauseAlloc(0,1);
    
    //Scandisce i letterali della clausola di abc e li rimappa in quelli di pdtrav
    for (pLit = pCnf->pClauses[j]; pLit < pCnf->pClauses[j+1]; pLit++) {
      int v = lit_var(*pLit);
      int s = lit_sign(*pLit);
      int lit = aigAbcInfo->abc2PdtIdMap[v];
      if (s) lit = -lit;
      Ddi_ClauseAddLiteral(cl,lit);
    }
    Ddi_ClauseArrayWriteLast(ca,cl);
  }

  return ca;
}


/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
static Aig_Man_t * 
abcBddarrayToAig (
  Ddi_Bddarray_t *fA,
  bAig_array_t *varNodes,
  bAig_array_t *visitedNodes      //Converte l'AIG dalla rappresentazione di PDTrav a quella di ABC
)
{
  Aig_Man_t * pAig;
  Aig_Obj_t * pObj, * pObjR, * pObjL;
  int i, nNodes, nVars;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr (fA);
  bAig_Manager_t  *bmgr = Ddi_MgrReadMgrBaig(ddm);
  bAigEdge_t baig;

  //  static int ncalls=0;
  // ncalls++;

  nNodes = visitedNodes->num+1; /* ??? */
  nVars = varNodes->num; /* ??? */

  pAig = Aig_ManStart(nNodes);      //Inizializza la struttura aig manager di ABC
  // map the constant node
  //    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pFrames ); ???
  // create variables 

  for (i=0; i<visitedNodes->num; i++) {
    bAigEdge_t baig = visitedNodes->nodes[i];
    bAig_AuxPtr(bmgr,baig) = NULL;
  }

  //Per ogni variabile di input crea un nodo nella struttura di ABC e ne salva un puntatore nell'edge del baig
  for (i=0; i<varNodes->num; i++) {
    bAigEdge_t varIndex = varNodes->nodes[i];
    Pdtutil_Assert(bAig_AuxPtr(bmgr,varIndex)==NULL,"NULL auxptr required");
    bAig_AuxPtr(bmgr,varIndex) = Aig_ObjCreateCi(pAig);
  }
  
  //Per ogni nodo interno del baig visitati dalle foglie alla radice prende i nodi figli, li complementa eventualmente
  //e crea il corrispondente AND nell'AIG manager di ABC
  for (i=0; i<visitedNodes->num; i++) {
    int rCompl, lCompl;
    bAigEdge_t right, left;
    bAigEdge_t baig = visitedNodes->nodes[i];
    if (bAig_NodeIsConstant(baig) || bAig_isVarNode(bmgr,baig)) continue;
    if (bAig_AuxPtr(bmgr,baig) != NULL) {
      continue;
    }
    right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
    left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);
    rCompl = bAig_NodeIsInverted(right);
    lCompl = bAig_NodeIsInverted(left);
    Pdtutil_Assert(right!=bAig_NULL&&left!=bAig_NULL,"NULL BAIG");
    pObjR = (Aig_Obj_t *) bAig_AuxPtr(bmgr,right);
    if (rCompl) pObjR = Aig_Not(pObjR); 
    pObjL = (Aig_Obj_t *) bAig_AuxPtr(bmgr,left);
    if (lCompl) pObjL = Aig_Not(pObjL); 
    pObj = Aig_And(pAig, pObjR, pObjL);
    bAig_AuxPtr(bmgr,baig) = pObj;
  }
  
  //Per ogni uscita della TR (next state) crea il corrispondente nodo di uscita nell'AIG di ABC
  for (i=0; i<Ddi_BddarrayNum(fA); i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    if (bAig_NodeIsConstant(Ddi_BddToBaig(f_i))) {
      pObj = Aig_ManConst0(pAig);
    }
    else {
      pObj = bAig_AuxPtr(bmgr,Ddi_BddToBaig(f_i));
    }
    if (Ddi_BddIsComplement(f_i)) pObj = Aig_Not(pObj);
    Aig_ObjCreateCo( pAig, pObj);
  }

  //Pulisce il manager di ABC
  Aig_ManCleanup( pAig );
  
  return pAig;
}

/**Function*************************************************************

  Synopsis    [Convert a FSM to the ABC Aig data structure]
  Description [Convert a FSM to the ABC Aig data structure]
  SideEffects [None]
  SeeAlso     [DdiAbcFsmFromAig]

***********************************************************************/
Aig_Man_t * 
DdiAbcFsmToAig (
  Ddi_Bddarray_t *delta,
  Ddi_Bddarray_t *lambda,
  Ddi_Vararray_t *pi,
  Ddi_Vararray_t *ps
)
{
  // fprintf(stdout, "\n\nfsm2aig\n\n");
  Aig_Man_t * pAig;
  Aig_Obj_t * pObj, * pObjR, * pObjL;
  int i, nNodes;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr (delta);
  bAig_Manager_t  *bmgr = Ddi_MgrReadMgrBaig(ddm);
  bAig_array_t *visitedNodes = bAigArrayAlloc();
  bAigEdge_t baig;

  Ddi_Bddarray_t *fA = Ddi_BddarrayDup(lambda);
  Ddi_Vararray_t *vars = Ddi_VararrayDup(pi);

  Ddi_BddarrayAppend(fA,delta);
  Ddi_VararrayAppend(vars,ps);
  for (i=0; i<Ddi_BddarrayNum(fA); i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    Ddi_PostOrderAigVisitIntern(bmgr,Ddi_BddToBaig(f_i),visitedNodes,-1);
  }

  Ddi_PostOrderAigClearVisitedIntern(bmgr,visitedNodes);

  nNodes = visitedNodes->num+1; /* ??? */
  // fprintf(stdout, "Num. of Nodes = %d, var = %d, L = %d, D = %d\n", nNodes-1, Ddi_VararrayNum(vars), Ddi_BddarrayNum(lambda), Ddi_BddarrayNum(delta));

  pAig = Aig_ManStart(nNodes);
  // map the constant node
  //    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pFrames ); ???
  // create variables 

  for (i=0; i<Ddi_VararrayNum(vars); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(vars,i);
    bAigEdge_t varIndex = Ddi_VarToBaig(v);
    Pdtutil_Assert(bAig_AuxPtr(bmgr,varIndex)==NULL,"NULL auxptr required");
    bAig_AuxPtr(bmgr,varIndex) = Aig_ObjCreateCi(pAig);

    // fprintf(stdout, "CI > %d\n", varIndex);
  }

  for (i=0; i<visitedNodes->num; i++) {
    int rCompl, lCompl;
    bAigEdge_t right, left;
    bAigEdge_t baig = visitedNodes->nodes[i];
    if (bAig_NodeIsConstant(baig) || bAig_isVarNode(bmgr,baig)) {
      // fprintf(stdout, "NODE > %d :: baig %d VAR/COST\n", i, baig);
      continue;
    }
    right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
    left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);
    rCompl = bAig_NodeIsInverted(right);
    lCompl = bAig_NodeIsInverted(left);

    pObjR =  (Aig_Obj_t *) bAig_AuxPtr(bmgr,right);
    if (rCompl) {; 
      pObjR = Aig_Not(pObjR);
    }
   
    pObjL = (Aig_Obj_t *) bAig_AuxPtr(bmgr,left);
    if (lCompl) {
      pObjL = Aig_Not(pObjL); 
    }
    
    pObj = Aig_And(pAig, pObjR, pObjL);

    bAig_AuxPtr(bmgr,baig) = pObj;
    // fprintf(stdout, "PDT_NODE > %d (%d, %d)\n", baig, left, right);
    // fprintf(stdout, "AIG_NODE > %d (%d, %d)\n", pObj->Id, Aig_ObjChild0Id(pObj), Aig_ObjChild1Id(pObj));
    // fprintf(stdout, "pObj > %p %d :: L > %p R > %p\n", pObj, pObj->Id, pObjR, pObjL);
  }
  
  for (i=0; i<Ddi_BddarrayNum(fA); i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);

    if (bAig_NodeIsConstant(Ddi_BddToBaig(f_i))) {
      pObj = Aig_ManConst0(pAig);
    }
    else {
      pObj = bAig_AuxPtr(bmgr,Ddi_BddToBaig(f_i));
    }

    if (Ddi_BddIsComplement(f_i)) pObj = Aig_Not(pObj);
    Aig_ObjCreateCo( pAig, pObj);
  }

  Aig_ManCleanup( pAig );
  
  for (i=0; i<Ddi_VararrayNum(vars); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(vars,i);
    bAigEdge_t varIndex = Ddi_VarToBaig(v);
    bAig_AuxPtr(bmgr,varIndex) = NULL;
  }
  for (i=0; i<visitedNodes->num; i++) {
    bAigEdge_t baig = visitedNodes->nodes[i];
    bAig_AuxPtr(bmgr,baig) = NULL;
  }
  
  Ddi_Free(vars);
  Ddi_Free(fA);
  bAigArrayFree(visitedNodes);
  
  return pAig;
}

/**Function*************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/
static Aig_Man_t * 
abcBddToAig (
  Ddi_Bdd_t *f,
  Ddi_Vararray_t *vars
)
{
  Aig_Man_t * pAig;
  Aig_Obj_t * pObj, * pObjR, * pObjL;
  int i, nNodes;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr (f);
  bAig_Manager_t  *bmgr = Ddi_MgrReadMgrBaig(ddm);
  bAig_array_t *visitedNodes = bAigArrayAlloc();
  bAigEdge_t baig;
  
  if (Ddi_BddIsPartConj(f)) {
    int i;
    for (i=0; i<Ddi_BddPartNum(f); i++) {
      Ddi_Bdd_t *f_i = Ddi_BddPartRead(f,i);
      Ddi_PostOrderAigVisitIntern(bmgr,Ddi_BddToBaig(f_i),visitedNodes,-1);
    }
  }
  else {
    Ddi_PostOrderAigVisitIntern(bmgr,Ddi_BddToBaig(f),visitedNodes,-1);
  }

  Ddi_PostOrderAigClearVisitedIntern(bmgr,visitedNodes);

  nNodes = visitedNodes->num+1; /* ???+1 for po - constant nodes 
				 1 in aig, 0 in gia  */

  pAig = Aig_ManStart(nNodes);
  // map the constant node
  //    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pFrames ); ???
  // create variables 

  for (i=0; i<Ddi_VararrayNum(vars); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(vars,i);
    bAigEdge_t varIndex = Ddi_VarToBaig(v);
    Pdtutil_Assert(bAig_AuxPtr(bmgr,varIndex)==NULL,"NULL auxptr required");
    bAig_AuxPtr(bmgr,varIndex) = Aig_ObjCreateCi(pAig);
  }
  
  for (i=0; i<visitedNodes->num; i++) {
    int rCompl, lCompl;
    bAigEdge_t right, left;
    bAigEdge_t baig = visitedNodes->nodes[i];
    if (bAig_NodeIsConstant(baig) || bAig_isVarNode(bmgr,baig)) continue;
    right = bAig_NodeReadIndexOfRightChild(bmgr,baig);
    left = bAig_NodeReadIndexOfLeftChild(bmgr,baig);
    rCompl = bAig_NodeIsInverted(right);
    lCompl = bAig_NodeIsInverted(left);
    pObjR = (Aig_Obj_t *) bAig_AuxPtr(bmgr,right);
    if (rCompl) pObjR = Aig_Not(pObjR); 
    pObjL = (Aig_Obj_t *) bAig_AuxPtr(bmgr,left);
    if (lCompl) pObjL = Aig_Not(pObjL); 
    pObj = Aig_And(pAig, pObjR, pObjL);
    bAig_AuxPtr(bmgr,baig) = pObj;
  }
  
  if (Ddi_BddIsPartConj(f)) {
    int i;
    for (i=0; i<Ddi_BddPartNum(f); i++) {
      Ddi_Bdd_t *f_i = Ddi_BddPartRead(f,i);
      pObj = bAig_AuxPtr(bmgr,Ddi_BddToBaig(f_i));
      if (Ddi_BddIsComplement(f_i)) pObj = Aig_Not(pObj);
      Aig_ObjCreateCo( pAig, pObj);
    }
  }
  else {
    pObj = bAig_AuxPtr(bmgr,Ddi_BddToBaig(f));
    if (Ddi_BddIsComplement(f)) pObj = Aig_Not(pObj);
    Aig_ObjCreateCo( pAig, pObj);
  }
  
  Aig_ManCleanup( pAig );
  
  for (i=0; i<Ddi_VararrayNum(vars); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(vars,i);
    bAigEdge_t varIndex = Ddi_VarToBaig(v);
    bAig_AuxPtr(bmgr,varIndex) = NULL;
  }
  for (i=0; i<visitedNodes->num; i++) {
    bAigEdge_t baig = visitedNodes->nodes[i];
    bAig_AuxPtr(bmgr,baig) = NULL;
  }
  
  bAigArrayFree(visitedNodes);
  
  return pAig;
}

/**Function*************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/
Ddi_Bddarray_t *
DdiAbcBddarrayFromAig (
  Aig_Man_t *pAig,
  Ddi_Vararray_t *vars
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr (vars);
  Ddi_Bddarray_t *newAigA = Ddi_BddarrayAlloc (ddm,0);
  bAig_Manager_t  *bmgr = Ddi_MgrReadMgrBaig(ddm);
  bAigEdge_t baigNew;

  Aig_Obj_t *pObj, *pObjNew;

  int i, nNodes;

  pAig->vFlopNums = 0; // Pas: to check!
  Pdtutil_Assert(pAig->vFlopNums==0,"converting an AIG with latches");
  Pdtutil_Assert(Aig_ManCiNum(pAig)<=Ddi_VararrayNum(vars),
                 "wrong num of aig vars");

  Aig_ManCleanData(pAig);

  Aig_ManForEachObj(pAig, pObj, i) {
    pObj->iData = (int)bAig_NULL;
  }

  Aig_ManConst1(pAig)->iData = (int)bAig_One; 
  Aig_ManForEachCi( pAig, pObj, i) {
    int id = i;
    Ddi_Var_t *v = Ddi_VararrayRead(vars,id);
    baigNew = Ddi_VarToBaig(v);
    pObj->iData = (int)baigNew;
    bAig_Ref(bmgr, baigNew);
    // printf("CI: id: %d, baig: %d\n", pObj->Id, pObj->iData);
  } 

  //  Aig_ManOrderStart(pAig);
  //  Aig_ManForEachNodeInOrder(pAig, pObj) {
  Aig_ManForEachObj(pAig, pObj, i) {
    // fprintf(stdout, "OBJ %d:", i);
    Pdtutil_Assert (!Aig_ObjIsBuf(pObj), "no buffer allowed here");

    if (Aig_ObjIsNode(pObj)) {
      
      Pdtutil_Assert(Aig_ObjChild0Baig(pObj)!=bAig_NULL,"null baig in abc");
      Pdtutil_Assert(Aig_ObjChild1Baig(pObj)!=bAig_NULL,"null baig in abc");

      if (Aig_ObjType(pObj) == AIG_OBJ_AND) {
        baigNew = bAig_And(bmgr,
                           Aig_ObjChild0Baig(pObj), Aig_ObjChild1Baig(pObj)); 
        // fprintf(stdout, "AND > (%d, %d) - (%d, %d) :: ", Aig_ObjChild0Id(pObj), Aig_ObjChild0Baig(pObj), Aig_ObjChild1Id(pObj), Aig_ObjChild1Baig(pObj));      
      }
      else if (Aig_ObjType(pObj) == AIG_OBJ_EXOR) {
        baigNew = bAig_Xor(bmgr,
                           Aig_ObjChild0Baig(pObj), Aig_ObjChild1Baig(pObj)); 
        // fprintf(stdout, "EXOR > %d - %d :: ", Aig_ObjChild0Baig(pObj), Aig_ObjChild1Baig(pObj));     
      }
    }
    else if (Aig_ObjIsCi(pObj)) {
      // printf("id: %d, baig: %d >> current node is CI\n", pObj->Id, pObj->iData);
      continue;
    }
    else if (Aig_ObjIsConst1(pObj)) {
      // printf("CONST1 :: ", pObj->Id, pObj->iData);
      baigNew = bAig_One;
    }
    else if (Aig_ObjIsCo(pObj)) {
      // printf("CO :: ", pObj->Id, pObj->iData);
      // baigNew = Aig_ObjChild0Baig(pObj);
      // Ddi_Bdd_t *newAig = Ddi_BddMakeFromBaig(ddm,baigNew);
      continue;
    }
    pObj->iData = (int)baigNew;
    bAig_Ref(bmgr, baigNew);
    // printf("NODE: id: %d, baig: %d\n", pObj->Id, pObj->iData);
  }
  //  Aig_ManOrderStop(pAig);
  // It was PoSeq, but we need CO to reconstruct everything within the pAig
  Aig_ManForEachCo( pAig, pObj, i) {
    Ddi_Bdd_t *newAig = NULL;
    baigNew = Aig_ObjChild0Baig(pObj);
    newAig = Ddi_BddMakeFromBaig(ddm,baigNew);
    Ddi_BddarrayInsertLast(newAigA,newAig);
    Ddi_Free(newAig);
  }

  Aig_ManForEachObj(pAig, pObj, i) {
    baigNew = pObj->iData;
    bAig_RecursiveDeref(bmgr,baigNew);
  }

  Pdtutil_Assert (newAigA!=NULL,"no po aig");

  return newAigA;

}

/**Function*************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/
static Ddi_Bdd_t *
abcBddFromAig (
  Aig_Man_t *pAig,
  Ddi_Vararray_t *vars,
  Ddi_Bdd_t *refAig
)
{
  Ddi_Bdd_t *newAig = NULL;
  Ddi_Bddarray_t *newAigA = DdiAbcBddarrayFromAig(pAig,vars);
  if (refAig==NULL || Ddi_BddIsAig(refAig)) {
    Pdtutil_Assert(Ddi_BddarrayNum(newAigA)==1,"wrong aig array num");
    newAig = Ddi_BddDup(Ddi_BddarrayRead(newAigA,0));
  }
  else {
    Pdtutil_Assert((Ddi_BddIsPartConj(refAig)||Ddi_BddIsPartDisj(refAig)),
      "part conj or disj required for abc to aig");
    Pdtutil_Assert(Ddi_BddarrayNum(newAigA)==Ddi_BddPartNum(refAig),
		   "wrong aig array num");
    if (Ddi_BddIsPartConj(refAig)) {
      newAig = Ddi_BddMakePartConjFromArray(newAigA);
    }
    else {
      newAig = Ddi_BddMakePartDisjFromArray(newAigA);
    }
  }
  Ddi_Free(newAigA);
  return (newAig);
}

/**Function*************************************************************

  Synopsis    [Extract from the ABC Aig data structure FSM elements]
  Description [Extract from the ABC Aig data structure FSM elements. 
               It requires the original Vararray pi and ps used to create the ABC Aig in the first place, in case it is used to 
               return back to a FSM created within PdTRAV.]
  SideEffects [None]
  SeeAlso     [DdiAbcFsmToAig, DdiAbcBddarrayFromAig]

***********************************************************************/
int   
DdiAbcFsmFromAig (
  Aig_Man_t * pAig,
  Ddi_Bddarray_t * delta,
  Ddi_Bddarray_t * lambda,
  Ddi_Vararray_t * pi,
  Ddi_Vararray_t * ps
)
{
  // fprintf(stdout, "\n\naig2fsm\n\n");
  Aig_Obj_t * pObj;
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(delta);
  bAig_Manager_t  *bmgr = Ddi_MgrReadMgrBaig(ddiMgr);
  bAigEdge_t baig;
  int i, j;
  Aig_Obj_t *aigObj;
  char name[10];

  // Aig_ManCleanData(pAig);
  Aig_ManConst1(pAig)->iData = (int)bAig_One; 

#if 0
  // fprintf(stdout, "aig2fsm :: Processing PIs\n");
  Aig_ManForEachPiSeq(pAig, aigObj, i) {
    Ddi_Var_t *new_pi = Ddi_VarNew(ddiMgr);
    sprintf(name, "pi%d", i);
    Ddi_VarAttachName (new_pi, name);
    Ddi_VararrayInsertLast (pi, new_pi);
  }

  // fprintf(stdout, "aig2fsm :: Processing PSs\n");
  Aig_ManForEachLiSeq(pAig, aigObj, i) {
    Ddi_Var_t *new_ps = Ddi_VarNew(ddiMgr);
    sprintf(name, "ps%d", i);
    Ddi_VarAttachName (new_ps, name);
    Ddi_VararrayInsertLast (ps, new_ps);
  }
#endif

  Ddi_Vararray_t *vars = Ddi_VararrayDup(pi);
  Ddi_VararrayAppend(vars, ps);

  // fprintf(stdout, "aig2fsm :: Processing D/L\n");
  Ddi_Bddarray_t *fA = DdiAbcBddarrayFromAig(pAig, vars);

  for(i=0;i<pAig->nTruePos;i++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    Ddi_BddarrayInsertLast (lambda, f_i);
  }

  for(j=0; j<pAig->nRegs; i++,j++) {
    Ddi_Bdd_t *f_i = Ddi_BddarrayRead(fA,i);
    Ddi_BddarrayInsertLast (delta, f_i);
  }
    
  Ddi_Free(vars);
  Ddi_Free(fA);

  return 1;
}

/**Function*************************************************************

  Synopsis    [Extract from the ABC Aig data structure FSM elements]
  Description [Extract from the ABC Aig data structure FSM elements. 
               return back to a FSM created within PdTRAV.]
  SideEffects [None]
  SeeAlso     [DdiAbcFsmToAig, DdiAbcBddarrayFromAig]

***********************************************************************/
static int *abcLatchEquivClasses(Aig_Man_t * pAig)
{
    int num_orig_latches = pAig->nRegs;

    Aig_Obj_t * pFlop, * pRepr;
    int repr_idx;
    int i, j;
    int *reprIds = NULL;

    reprIds = Pdtutil_Alloc(int,num_orig_latches);

    Aig_ManSetCioIds( pAig );
    j=0;
    Saig_ManForEachLo( pAig, pFlop, i )
    {
      Pdtutil_Assert(i==j,"problem in ABC index sequence");
      j++;
      //      printf("FID: %d\n", pFlop->Id);
      pRepr = Aig_ObjRepr(pAig, pFlop);
      reprIds[i] = i;
      if (pRepr == NULL) {
	// Abc_Print( 1, "Nothing equivalent to flop %s\n", pFlopName);
	continue;
      }
      // pRepr is representative of the equivalence class, to which pFlop belongs
      if (Aig_ObjIsConst1(pRepr)) {
	// Abc_Print( 1, "Original flop # %d is proved equivalent to constant.\n", i );
	reprIds[i] = -1;
	continue;
      }

      assert( Saig_ObjIsLo( pAig, pRepr ) );
      repr_idx = Aig_ObjCioId(pRepr) - Saig_ManPiNum(pAig);
      reprIds[i] = repr_idx;
      // Abc_Print( 1, "Original flop # %d is proved equivalent to flop # %d.\n",  i, repr_idx );
    }
    return reprIds;

}


/**Function*************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/

Ddi_Bdd_t *
Ddi_AbcInterpolant (
  Ddi_Bdd_t *a,
  Ddi_Bdd_t *b,
  Ddi_Varset_t *globalVars,
  Ddi_Varset_t *domainVars,
  int *psatRes,
  float timeLimit
  )
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr (a);
  sat_solver * pSat;
  void * pSatCnf = NULL;
  int nVarsTot, nVarsB, sat;
  Aig_Man_t *aAbc, *bAbc, *pInterNew;
  Inta_Man_t * pManInterA; 
  Ddi_Bdd_t *interpolant=NULL;

  Ddi_Varset_t *aSupp = Ddi_BddSupp(a);
  Ddi_Varset_t *bSupp = Ddi_BddSupp(b);
  Ddi_Varset_t *abSupp = NULL;
  Ddi_Vararray_t *aVars = Ddi_VararrayMakeFromVarset(aSupp,1);
  Ddi_Vararray_t *bVars = Ddi_VararrayMakeFromVarset(bSupp,1);
  Ddi_Vararray_t *abVars = Ddi_VararrayAlloc (ddm,0);

  Cnf_Dat_t *pCnfA, *pCnfB;
  Aig_Obj_t *pObjA, *pObjB;
  int i, Lits[2], status, Var;
  int confLimit = 100000000;
  Vec_Int_t *vVarsAB;

  sat = Ddi_AigSatAnd(a,b,NULL);

  Ddi_VarsetSetArray(aSupp);
  Ddi_VarsetSetArray(bSupp);

  abSupp = Ddi_VarsetUnion(aSupp,bSupp);

  nVarsTot = Ddi_VarsetNum(abSupp);
  pSat = sat_solver_new();
  sat_solver_store_alloc(pSat);
  sat_solver_setnvars(pSat, nVarsTot);

  aAbc = abcBddToAig (a,aVars);
  bAbc = abcBddToAig (b,bVars);

  if (0) {
    Ddi_Bdd_t *a2 = abcBddFromAig(aAbc,aVars,a);
    Ddi_Bdd_t *b2 = abcBddFromAig(bAbc,bVars,b);

    Aig_ManDumpBlif( aAbc, "a.blif", NULL, NULL );
    Aig_ManDumpBlif( bAbc, "b.blif", NULL, NULL );
    
    Pdtutil_Assert(Ddi_AigEqualSat(a,a2),"wrong baig-abc conversion");
    Pdtutil_Assert(Ddi_AigEqualSat(b,b2),"wrong baig-abc conversion");
    Ddi_Free(a2);
    Ddi_Free(b2);
  }

  pCnfB = Cnf_Derive(bAbc,0);  
  nVarsB = Aig_ManCiNum(bAbc);
  Pdtutil_Assert(nVarsB==Ddi_VarsetNum(bSupp),"wrong var num");
  pCnfA = Cnf_Derive(aAbc,0);  
  Cnf_DataLift(pCnfA,pCnfB->nVars);

  /* load A clauses */
  for (i=0; i < pCnfA->nClauses; i++) {
    if (!sat_solver_addclause(pSat,pCnfA->pClauses[i],pCnfA->pClauses[i+1]))
      assert(0);
  }

  /* connector clauses */
  for (i=0; i<Ddi_VararrayNum(bVars); i++) {
    Ddi_Var_t *v = Ddi_VararrayRead(bVars,i);
    Pdtutil_Assert(Ddi_VarReadMark(v) == 0,"0 var mark required");
    Ddi_VarWriteMark(v,i+1);
  }

  vVarsAB = Vec_IntAlloc(Ddi_VarsetNum(abSupp));
  Vec_IntClear(vVarsAB);

  for (i=0; i<Ddi_VararrayNum(aVars); i++) {
    Ddi_Var_t *vb, *va = Ddi_VararrayRead(aVars,i);
    int ib;
    if ((ib=Ddi_VarReadMark(va))==0) continue;
    ib--;
    vb = Ddi_VararrayRead(bVars,ib);
    /* link vars */

    pObjA = Aig_ManCi(aAbc,i);
    pObjB = Aig_ManCi(bAbc,ib);

    Lits[0] = toLitCond(pCnfA->pVarNums[pObjA->Id], 0);
    Lits[1] = toLitCond(pCnfB->pVarNums[pObjB->Id], 1);
    if (!sat_solver_addclause(pSat,Lits,Lits+2))
      assert( 0 );
    Lits[0] = toLitCond(pCnfA->pVarNums[pObjA->Id], 1);
    Lits[1] = toLitCond(pCnfB->pVarNums[pObjB->Id], 0);
    if (!sat_solver_addclause(pSat,Lits,Lits+2))
      assert( 0 );

    /* global var */
    Ddi_VararrayInsertLast(abVars,vb);
    Vec_IntPush(vVarsAB, pCnfB->pVarNums[pObjB->Id]);
  }

  for (i=0; i<Ddi_VararrayNum(bVars); i++) {
    /* reset marks */
    Ddi_Var_t *v = Ddi_VararrayRead(bVars,i);
    Ddi_VarWriteMark(v,0);
  }

  sat_solver_store_mark_clauses_a(pSat);

  /* load B clauses */
  for (i=0; i < pCnfB->nClauses; i++) {
    if (!sat_solver_addclause(pSat,pCnfB->pClauses[i],pCnfB->pClauses[i+1])) {
      pSat->fSolved = 1;
      break;
    }
  }

  sat_solver_store_mark_roots(pSat);

  // collect global variables
  pSat->pGlobalVars = ABC_CALLOC( int, sat_solver_nvars(pSat));
  Vec_IntForEachEntry(vVarsAB, Var, i) {
    pSat->pGlobalVars[Var] = 1;
  }

  Aig_ManStop(aAbc);
  Aig_ManStop(bAbc);

  status = sat_solver_solve( pSat, NULL, NULL, 
                             (ABC_INT64_T)confLimit, 
                             (ABC_INT64_T)0, 
                             (ABC_INT64_T)0, 
                             (ABC_INT64_T)0 );

  ABC_FREE(pSat->pGlobalVars);
  pSat->pGlobalVars = NULL;

  if (status == l_False) {
    pSatCnf = sat_solver_store_release( pSat );
    *psatRes = 0;
    Pdtutil_Assert(!sat,"minisat-abc solver mismatch");
  }
  else if (status == l_True) {
    *psatRes = 1;
    Pdtutil_Assert(sat,"minisat-abc solver mismatch");
  } 
  else {
    *psatRes = -1;
  }
  sat_solver_delete(pSat);

  Cnf_DataFree(pCnfA);
  Cnf_DataFree(pCnfB);
  Ddi_Free(aVars);
  Ddi_Free(bVars);
  Ddi_Free(aSupp);
  Ddi_Free(bSupp);
  Ddi_Free(abSupp);

  if (pSatCnf == NULL) {
    Ddi_Free(abVars);
    return NULL;
  }

  pManInterA = Inta_ManAlloc();
  pInterNew = Inta_ManInterpolate(pManInterA, pSatCnf, vVarsAB, 0 );
  Inta_ManFree(pManInterA);
  
  Sto_ManFree( pSatCnf );

  interpolant = abcBddFromAig(pInterNew,abVars,NULL);

  Ddi_Free(abVars);

  Aig_ManStop(pInterNew);

  return(interpolant);
}



/**Function*************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/

int
Ddi_AigarrayAbcOptAcc (
  Ddi_Bddarray_t *aigA,
  float timeLimit
  )
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr (aigA);
  int ret;
  int directToAbc = 1;
  int verbose=0;
  int chkRes=0;
  int useGia = 0;
  int size0;
  int optLevel = Ddi_MgrReadAigAbcOptLevel(ddm);

  if (Ddi_MgrReadVerbosity(ddm) > Pdtutil_VerbLevelDevMin_c) {
    verbose = 1;
  }
  if (Ddi_MgrReadVerbosity(ddm) > Pdtutil_VerbLevelDevMin_c) {
    verbose = 2;
  }

  size0 = Ddi_BddarraySize(aigA);

  if (size0 < 3) return 1;

  if (optLevel == 0) {
    return (1);
  }

  if (size0 > 100000) {
    /* force level 1 (just balance) */
    optLevel = 1;
  }

  pthread_mutex_lock(&abcSharedUse);

  if (directToAbc) {

  CATCH {

    Aig_Man_t *aigAbcOpt;
    Ddi_Bddarray_t *aigOptA;
    Ddi_Vararray_t *vA = Ddi_BddarraySuppVararray(aigA);
    Aig_Man_t *aigAbc = DdiAbcBddarrayToAig (aigA,vA);

    //    assert(0);
    if (useGia) {
      Gia_Man_t *pGiaOpt, *pGia = Gia_ManFromAig(aigAbc);
      Aig_ManStop(aigAbc);

      /* do the job */
      printf("GIA\n");

      Gia_ManStop(pGia);
      aigAbc = Gia_ManToAig(pGiaOpt, 0 );
      Gia_ManStop(pGiaOpt);
    }
    else {
      aigAbcOpt = Dar_ManCompress2Pdt (aigAbc, timeLimit,
			     optLevel,verbose);
    }

    aigOptA = DdiAbcBddarrayFromAig(aigAbcOpt,vA);

    Aig_ManStop(aigAbc);
    Aig_ManStop(aigAbcOpt);

    if (1) {
      Pdtutil_VerbosityLocal(Ddi_MgrReadVerbosity(ddm),
			     Pdtutil_VerbLevelDevMax_c,
			     fprintf(stdout, "ABC Optimization: %d -> %d.\n",
        size0, Ddi_BddarraySize(aigOptA))
      );
      Ddi_DataCopy (aigA, aigOptA);
    }

    Ddi_Free(vA);
    Ddi_Free(aigOptA);

    pthread_mutex_unlock(&abcSharedUse);

  } FAIL {
    fprintf(stderr, "Error: Caught ABC Error; Result Ignored.\n");
    pthread_mutex_unlock(&abcSharedUse);
    ret = 0;
  }

  return (1);

  }



  CATCH {
    //    assert(0);

    if (Ddi_BddarraySize(aigA) > 100000) {
      /* force level 1 (just balance) */
      ret = ddiAigarrayAbcOptWithLevelAcc(aigA,1,timeLimit);
    }
    else {
      ret = ddiAigarrayAbcOptWithLevelAcc(aigA,
        Ddi_MgrReadAigAbcOptLevel(ddm),timeLimit);
    }
  } FAIL {
    fprintf(stderr, "Error: Caught ABC Error; Result Ignored.\n");
    ret = 0;
  }

  return ret;
}

/**Function*************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/
int
Ddi_AigAbcRpmAcc (
  Ddi_Bdd_t *aig,
  Ddi_Vararray_t *ps,
  int nCutMax,
  float timeLimit
)
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr (aig);
  Ddi_Bddarray_t *aigA = Ddi_BddarrayAlloc (ddm,1);
  int ret;

  Ddi_BddarrayWrite(aigA,0,aig);
  
  ret = Ddi_AigarrayAbcRpmAcc (aigA,ps,nCutMax,timeLimit);

  Ddi_DataCopy(aig,Ddi_BddarrayRead(aigA,0));
  Ddi_Free(aigA);
  return ret;
}

/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/

int
Ddi_AigarrayAbcRpmAcc (
  Ddi_Bddarray_t *aigA,
  Ddi_Vararray_t *ps,
  int nCutMax,
  float timeLimit
  )
{
  Ddi_Mgr_t *ddm = Ddi_ReadMgr (aigA);
  int ret;
  int directToAbc = 1;
  int verbose=0;
  int chkRes=0;
  int useGia = 0;
  int i, size0, nPi, nPs, nPiNew;
  Aig_Man_t *aigAbcOpt;
  Ddi_Bddarray_t *aigA2;
  Ddi_Bddarray_t *aigOptA;
  Ddi_Vararray_t *vA;
  Aig_Man_t *aigAbc;
  Ddi_Bddarray_t *piSupp, *psSupp;
  Gia_Man_t *pGiaOpt, *pGia;
  Ddi_Bdd_t *myOne;
  int improved, nPiPrev;

  static int ncall=0;

  if (nCutMax <= 0) nCutMax = 16; // ABC default

  if (Ddi_MgrReadVerbosity(ddm) > Pdtutil_VerbLevelDevMin_c) {
    verbose = 1;
  }
  if (Ddi_MgrReadVerbosity(ddm) > Pdtutil_VerbLevelDevMin_c) {
    verbose = 2;
  }

  size0 = Ddi_BddarraySize(aigA);

  aigA2 = Ddi_BddarrayDup(aigA);
  myOne = Ddi_BddMakeConstAig(ddm, 1);

  if (size0 < 3) return 1;

  pthread_mutex_lock(&abcSharedUse);

  vA = Ddi_BddarraySuppVararray(aigA);
  psSupp = Ddi_VararrayIntersect(ps,vA);
  piSupp = Ddi_VararrayDiff(vA,ps);
  Ddi_Free(vA);
  vA = Ddi_VararrayDup(piSupp);
  Ddi_VararrayAppend(vA,psSupp);
  nPi = Ddi_VararrayNum(piSupp);
  nPs = Ddi_VararrayNum(psSupp);

  // dummy entries for reg outputs
  for (i=0; i<nPs; i++) {
    Ddi_BddarrayInsertLast(aigA2,myOne);
  }		  

  aigAbc = DdiAbcBddarrayToAig (aigA2,vA);
  Aig_ManSetRegNum(aigAbc,nPs);
 

  do {
    pGia = Gia_ManFromAig(aigAbc);
    Aig_ManStop(aigAbc);

    ncall++;
    nPiPrev = Gia_ManPiNum(pGia);

    pGiaOpt = Abs_RpmPerform(pGia, nCutMax, 0, 0);
    Gia_ManStop(pGia);

    aigAbc = Gia_ManToAig(pGiaOpt, 0 );
    Gia_ManStop(pGiaOpt);
    pGia = Gia_ManFromAig(aigAbc);
    Aig_ManStop(aigAbc);

    pGiaOpt = Gia_ManCompress2(pGia, 1, 0);
    Gia_ManStop(pGia);

    aigAbc = Gia_ManToAig(pGiaOpt, 0 );
    nPiNew = Gia_ManPiNum(pGiaOpt);
    Gia_ManStop(pGiaOpt);

    improved = nPiNew < 0.95*nPiPrev;
  } while (improved);

  if (nPiNew != nPi) {
    char name[1000], ii=0;
    printf("RPM - PI: %d -> %d\n", nPi, nPiNew);
    Ddi_Free(vA);
    while (Ddi_VararrayNum(piSupp) > nPiNew) {
      Ddi_VararrayRemove(piSupp,Ddi_VararrayNum(piSupp)-1);
    }
    while (Ddi_VararrayNum(piSupp) < nPiNew) {
      Ddi_Var_t *newv;
      sprintf(name,"pdtRpmInp_%d", ii++);
      newv = Ddi_VarFindOrAdd(ddm,name,0);
      Ddi_VararrayInsertLast(piSupp,newv);
    }
    vA = Ddi_VararrayDup(piSupp);
    Ddi_VararrayAppend(vA,psSupp);
  }

  Ddi_Free(piSupp);
  Ddi_Free(psSupp);

  aigOptA = DdiAbcBddarrayFromAig(aigAbc,vA);

  Aig_ManStop(aigAbc);

  if (1) {
    Pdtutil_VerbosityLocal(Ddi_MgrReadVerbosity(ddm),
                           Pdtutil_VerbLevelUsrMax_c,
                           fprintf(stdout, "ABC Rpm: %d -> %d.\n",
                                   size0, Ddi_BddarraySize(aigOptA))
    );
    for (i=0; i<Ddi_BddarrayNum(aigA); i++) {
      Ddi_BddarrayWrite(aigA, i, Ddi_BddarrayRead(aigOptA,i));
    }
  }

  Ddi_Free(myOne);
  Ddi_Free(vA);
  Ddi_Free(aigA2);
  Ddi_Free(aigOptA);
  
  pthread_mutex_unlock(&abcSharedUse);
    
  return (1);

}


/**Function*************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/

int
ddiAigarrayAbcOptWithLevelAcc (
  Ddi_Bddarray_t *aigA,
  int optLevel,
  float timeLimit
)
{
  void *pAbc;
  char command[PDTUTIL_MAX_STR_LEN];
  int timeInit, timeFinal;
  FILE *fp = NULL;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr (aigA);
  bAig_Manager_t  *bmgr = Ddi_MgrReadMgrBaig(ddm);
  int n, flagFile, result, retValue;
  Ddi_Bdd_t *newAig;
  Ddi_Bddarray_t *newA;
  st_table *NameToAigTable;
  char buf [200], filename1[100], filename2[100];
  char tmpStr[100],i0Str[100],i1Str[100],oStr[100],poName[100];
  int i, i0Int,i1Int,oInt;
  int nMax, size0;
  bAigEdge_t *aigNodes, outNode, oAig, i0Aig, i1Aig;
  int satCompare = 0;
  int verbose=0;

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelUsrMin_c) {
    verbose = 1;
  }
  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
    verbose = 2;
  }

  /* Check for NULL AIG */
  if (aigA == NULL) {
    fprintf(stderr, "Error: NULL AIG To Store in ddiAbcLink. Nothing done.\n");
    return (1);
  }

  if (optLevel == 0) {
    return (1);
  }

  size0 = Ddi_BddarraySize(aigA);

  if (size0 < 4) {
    return (1);
  }

  sprintf(filename1, "%s%d_%d.bench", FILE1, getpid(), (int)pthread_self());
  Ddi_AigarrayNetStore(aigA,filename1,fp,Pdtutil_Aig2BenchName_c);

  sprintf(filename2, "%s%d_%d.bench", FILE2, getpid(), (int)pthread_self());

  CATCH {
    ddiAbcOptAccInt (optLevel, verbose, filename1, filename2, timeLimit);
    newA = Ddi_AigarrayNetLoadBench(ddm, filename2, NULL);
    Ddi_DataCopy (aigA, newA);
    Ddi_Free(newA);
  }
  FAIL {
    fprintf(stderr, "Error: Error Caught in ddiAbcLink.c.\n");
  }

  sprintf(buf, "rm -f %s %s", filename1, filename2);
  system(buf);

  return (1);

}


/**Function*************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/

static int
ddiAbcOptWithLevelAcc (
  Ddi_Bdd_t *aig,
  int optLevel,
  float timeLimit
)
{
  void *pAbc;
  char command[PDTUTIL_MAX_STR_LEN];
  int timeInit, timeFinal;
  FILE *fp = NULL;
  Ddi_Mgr_t *ddm = Ddi_ReadMgr (aig);
  bAig_Manager_t  *bmgr = Ddi_MgrReadMgrBaig(ddm);
  int n, flagFile, result, retValue;
  Ddi_Bdd_t *newAig;
  Ddi_Bddarray_t *lambdas;
  st_table *NameToAigTable;
  char buf [200], filename1[100], filename2[100];
  char tmpStr[100],i0Str[100],i1Str[100],oStr[100],poName[100];
  int i, i0Int,i1Int,oInt;
  int nMax, size0;
  bAigEdge_t *aigNodes, outNode, oAig, i0Aig, i1Aig;
  int satCompare = 0;
  int verbose=0;

  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelUsrMin_c) {
    verbose = 1;
  }
  if (Ddi_MgrReadVerbosity(ddm) >= Pdtutil_VerbLevelDevMin_c) {
    verbose = 2;
  }

  if (bAig_NodeIsConstant(Ddi_BddToBaig(aig))
   || bAig_isVarNode(bmgr,Ddi_BddToBaig(aig))) {
    return (1);
  }

  if (optLevel == 0) {
    return (1);
  }

  /* Check for NULL AIG */
  if (aig == NULL) {
    fprintf(stderr, "Error: NULL AIG To Store in ddiAbcLink. Nothing done.\n");
    return (1);
  }

  size0 = Ddi_BddSize(aig);

  /* Store AIG */
  //  Ddi_AigNetStore (aig, FILE1, NULL, Pdtutil_Aig2Blif_c);
  //Ddi_AigNetStore (aig, FILE1, NULL, Pdtutil_Aig2BenchName_c);
  sprintf(filename1, "%s%d_%d.bench", FILE1, getpid(), (int)pthread_self());
  Ddi_AigNetStore(aig, filename1, NULL, Pdtutil_Aig2BenchName_c);
  //  Ddi_AigNetStore (aig, "a.blif", NULL, Pdtutil_Aig2Blif_c);
  if (0)
  {
    Ddi_Bddarray_t *a;
    Ddi_Bdd_t *t;
    Ddi_AigNetStore (aig, "prova", 0, Pdtutil_Aig2BenchLocalId_c);
    a = Ddi_AigarrayNetLoadBench(ddm, "prova", 0);
    t = Ddi_BddarrayRead(a,0);
    Ddi_BddXorAcc(t,aig);
    Pdtutil_Assert(!Ddi_AigSat(t),"Erron in bench store-load");
    Ddi_Free(a);
  }

  /* Call ABC */
  sprintf(filename2, "%s%d_%d.bench", FILE2, getpid(), (int)pthread_self());
  //  try {
  ddiAbcOptAccInt (optLevel, verbose, filename1, filename2, timeLimit);
  sprintf(buf, "rm -f %s", filename1);
  system(buf);

  /* Read Back BENCH */
  fp = Pdtutil_FileOpen (NULL, filename2, "r", &flagFile);
  for (nMax=0; fgets(buf,199,fp)!=NULL; nMax++);
  rewind(fp);
  aigNodes = Pdtutil_Alloc(bAigEdge_t, nMax);
  for (i=0; i<nMax; i++) aigNodes[i] = bAig_NULL;
  outNode = bAig_NULL;

  while (fgets(buf,199,fp)!=NULL) {
    if (strstr(buf,"ENDBENCH")!=NULL) {
      /* END file */
      break;
    }
    if (strstr(buf,"INPUT(")!=NULL) {
      /* input */
    }
    else if (strstr(buf,"OUTPUT(")!=NULL) {
      /* output */
      sscanf (buf, "%s", tmpStr);
      tmpStr[strlen(tmpStr)-1]='\0';
      sscanf (tmpStr, "OUTPUT(%s", poName);
    }
    else if (strstr(buf,"= vdd")!=NULL) {
      /* one */
      oInt = -1;
      sscanf (buf, "%s%*s", oStr);
      oAig = bAig_One;
      bAig_Ref(bmgr, oAig);
      if (oStr[0]=='n') {
        sscanf(oStr,"n%d",&oInt);
      aigNodes[oInt] = oAig;
      }
      else {
      outNode = oAig;
      }
      Pdtutil_Assert((oInt>=0||(strcmp(oStr,poName)==0)),
        "invalid output in BENCH");
    }
    else if (strstr(buf,"= NOT(")!=NULL) {
      /* not */
      sscanf (buf, "%s%*s%s", oStr,tmpStr);
      tmpStr[strlen(tmpStr)-1]='\0';
      sscanf (tmpStr, "NOT(%s", i0Str);
      oInt = i0Int = i1Int = -1;
      if (i0Str[0]=='n') {
        sscanf(i0Str,"n%d",&i0Int);
      i0Aig = aigNodes[i0Int];
      }
      else {
        i0Aig = bAig_VarNodeFromName(bmgr,i0Str);
      }
      Pdtutil_Assert(i0Aig!=bAig_NULL,
                 "NULL Aig");
      oAig = bAig_Not(i0Aig);
      bAig_Ref(bmgr, oAig);
      if (oStr[0]=='n') {
        sscanf(oStr,"n%d",&oInt);
      aigNodes[oInt] = oAig;
      }
      else {
      outNode = oAig;
      }
      Pdtutil_Assert((oInt>=0||(strcmp(oStr,poName)==0)),
        "invalid output in BENCH");
    }
    else if (strstr(buf,"= BUFF(")!=NULL) {
      /* not */
      sscanf (buf, "%s%*s%s", oStr, tmpStr);
      tmpStr[strlen(tmpStr)-1]='\0';
      sscanf (tmpStr, "BUFF(%s", i0Str);
      oInt = i0Int = i1Int = -1;
      if (i0Str[0]=='n') {
        sscanf(i0Str,"n%d",&i0Int);
      i0Aig = aigNodes[i0Int];
      }
      else {
        i0Aig = bAig_VarNodeFromName(bmgr,i0Str);
      }
      Pdtutil_Assert(i0Aig!=bAig_NULL,
                 "NULL Aig");
      oAig = i0Aig;
      bAig_Ref(bmgr, oAig);
      if (oStr[0]=='n') {
        sscanf(oStr,"n%d",&oInt);
      aigNodes[oInt] = oAig;
      }
      else {
      outNode = oAig;
      }
      Pdtutil_Assert((oInt>=0||(strcmp(oStr,poName)==0)),
        "invalid output in BENCH");
    }
    else if (strstr(buf,"= AND(")!=NULL) {
      /* and */
      int isIntern, k;
      sscanf (buf, "%s%*s%s%s", oStr,tmpStr,i1Str);
      tmpStr[strlen(tmpStr)-1]='\0';
      i1Str[strlen(i1Str)-1]='\0';
      sscanf (tmpStr, "AND(%s", i0Str);
      oInt = i0Int = i1Int = -1;
      isIntern = i0Str[0]=='n';
      if (isIntern) {
        for (k=1; isdigit(i0Str[k]); k++);
      isIntern = i0Str[k]=='\0';
      }
      if (isIntern) {
        sscanf(i0Str,"n%d",&i0Int);
      i0Aig = aigNodes[i0Int];
      }
      else {
        i0Aig = bAig_VarNodeFromName(bmgr,i0Str);
      }

      isIntern = i1Str[0]=='n';
      if (isIntern) {
        for (k=1; isdigit(i1Str[k]); k++);
      isIntern = i1Str[k]=='\0';
      }
      if (isIntern) {
        sscanf(i1Str,"n%d",&i0Int);
      i1Aig = aigNodes[i0Int];
      }
      else {
        i1Aig = bAig_VarNodeFromName(bmgr,i1Str);
      }
      Pdtutil_Assert(i0Aig!=bAig_NULL && i1Aig!=bAig_NULL,
                 "NULL Aigs");
      oAig = bAig_And(bmgr,i0Aig,i1Aig);
      bAig_Ref(bmgr, oAig);

      isIntern = oStr[0]=='n';
      if (isIntern) {
        for (k=1; isdigit(oStr[k]); k++);
      isIntern = oStr[k]=='\0';
      }
      if (isIntern) {
        sscanf(oStr,"n%d",&oInt);
      aigNodes[oInt] = oAig;
      }
      else {
      outNode = oAig;
      }
      Pdtutil_Assert((oInt>=0||(strcmp(oStr,poName)==0)),
        "invalid output in BENCH");
    }

  }

  Pdtutil_Assert(outNode!=bAig_NULL,"NULL out node");

  Pdtutil_FileClose (fp, &flagFile);
  sprintf(buf, "rm -f %s", filename2);
  system(buf);

  newAig = Ddi_BddMakeFromBaig(ddm,outNode);

  if (satCompare) {
    Ddi_Bdd_t *diff = Ddi_BddXor(newAig,aig);
    //    Pdtutil_Assert(!Ddi_AigSat(diff),"wrong ABC opt");
    if (!Ddi_AigSat(diff)) {
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelUsrMin_c,
        Pdtutil_VerbLevelDevMax_c,
        fprintf(stdout, "ABC Optimization: %d -> %d.\n",
          Ddi_BddSize(aig), Ddi_BddSize(newAig))
      );
      DdiGenericDataCopy((Ddi_Generic_t *)aig,(Ddi_Generic_t *)newAig);
    }
    else {
      Pdtutil_Warning(1,"Wrong ABC Optimization.\n");
    }
    Ddi_Free(diff);
  }
  else {
    Pdtutil_VerbosityLocal(Ddi_MgrReadVerbosity(ddm),
      Pdtutil_VerbLevelDevMax_c,
      fprintf(stdout, "ABC Optimization: %d -> %d.\n",
        size0, Ddi_BddSize(newAig))
    );
    DdiGenericDataCopy((Ddi_Generic_t *)aig,(Ddi_Generic_t *)newAig);
  }
  Ddi_Free(newAig);

  bAig_RecursiveDeref(bmgr,outNode);
  for (i=0; i<nMax; i++) {
    if (aigNodes[i] != bAig_NULL) {
      bAig_RecursiveDeref(bmgr,aigNodes[i]);
    }
  }
  Pdtutil_Free(aigNodes);

  return (1);
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/

static int
ddiAbcOptAccInt (
  int optLevel,
  int verbose,
  char *fileNameR,
  char *fileNameW,
  float timeLimit
  )
{
  void *pAbc;
  char command[PDTUTIL_MAX_STR_LEN+1], new_command[PDTUTIL_MAX_STR_LEN+1];
  int timeInit, timeFinal;
  FILE *fp = NULL;
  int n, flagFile, result;
  int size0, size1, totSize0, totSize1;
  float OKgain = 0.95;
  float TotOKgain = 0.90;
  int fraigTimeLimit = 10;
  float abcTimeLimit = timeLimit;

  if (optLevel <= 0) {
    return (1);
  }

  if (optLevel <= 2) {
    abcTimeLimit /= 5;
  }

  /*                          */
  /*  Start the ABC framework */
  /*                          */

  Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "-- ABC Running.\n")
  );
  if (!abcStarted) {
    Abc_Start();
    abcStarted=1;
  }
  pAbc = Abc_FrameGetGlobalFrame();

  /*                 */
  /*  Issue Commands */
  /*                 */

  /* Build Command List */
  snprintf(command, PDTUTIL_MAX_STR_LEN, "read_bench %s", fileNameR);
  snprintf(new_command, PDTUTIL_MAX_STR_LEN, "%s; strash ; balance", command);

  Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "Issuing Command: %s.\n", new_command)
  );
  if ( Cmd_CommandExecute (pAbc, new_command) ) {
    fprintf(stderr, "Error: Cannot Execute Command \"%s\".\n", new_command);
    return (0);
  }

  Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMin_c,
    fprintf(stdout, "Initial Node: Number=%d; Level=%d.\n",
      Abc_GetNtkNodeNum(pAbc), Abc_GetNtkLevelNum(pAbc))
  );

  timeInit = clock();

  do {
    totSize0 = Abc_GetNtkNodeNum(pAbc);
    do {
      if (abcTimeLimit >= 0 &&
        (clock() - timeInit) > abcTimeLimit*MY_CLOCKS_PER_SEC) break;
      size0 = Abc_GetNtkNodeNum(pAbc);
      snprintf(command, PDTUTIL_MAX_STR_LEN, "balance");
      Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "Issuing Command: %s.\n", command)
      );
      if ( Cmd_CommandExecute (pAbc, command) ) {
        fprintf(stderr, "Cannot Execute Command \"%s\".\n", command);
        return (0);
      }
      size1 = Abc_GetNtkNodeNum(pAbc);
    } while (size0*OKgain > size1);

    Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "Balance Node: Number=%d; Level=%d.\n",
        Abc_GetNtkNodeNum(pAbc), Abc_GetNtkLevelNum(pAbc))
    );

    if (optLevel <= 1 || (abcTimeLimit>=0 &&
        (clock() - timeInit) > abcTimeLimit*MY_CLOCKS_PER_SEC)) break;

    do {
      int k, size00, size01;
      if (abcTimeLimit>=0 &&
         (clock() - timeInit) > abcTimeLimit*MY_CLOCKS_PER_SEC) break;
      size0 = Abc_GetNtkNodeNum(pAbc);
      snprintf(command, PDTUTIL_MAX_STR_LEN, "strash");
      snprintf(new_command, PDTUTIL_MAX_STR_LEN,
        "%s; balance", command);
      size00 = size0;
      for (k=0; k<3; k++) {
	if ( Cmd_CommandExecute (pAbc, command) ) {
	  fprintf(stderr, "Error: Cannot Execute Command \"%s\".\n", command);
	  return (0);
	}
	size01 = Abc_GetNtkNodeNum(pAbc);
	if (size01*1.2 > size00) break;
	size00 = size01;
      }

      //  snprintf(command, PDTUTIL_MAX_STR_LEN, "%s; refactor -l", new_command);
      if (size01 < 50000) {
	snprintf(command, PDTUTIL_MAX_STR_LEN, "%s; strash; refactor", 
	       new_command);
      }
      if (0&&size0 > 10000) {
        snprintf(command, PDTUTIL_MAX_STR_LEN, "%s; renode", new_command);
      }
      snprintf(new_command, PDTUTIL_MAX_STR_LEN,
        "balance; strash; rewrite -l ; balance");
      //        "%s; balance; strash; dcompress2 -b -l ; balance", command);
      Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "Issuing Command: %s.\n", command)
      );
      if ( Cmd_CommandExecute (pAbc, command) ) {
        fprintf(stderr, "Error: Cannot Execute Command \"%s\".\n", new_command);
        return (0);
      }
      Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "Issuing Command: %s.\n", new_command)
      );
      if (0&& Cmd_CommandExecute (pAbc, new_command) ) {
        fprintf(stderr, "Error: Cannot Execute Command \"%s\".\n", new_command);
        return (0);
      }
      size1 = Abc_GetNtkNodeNum(pAbc);
    } while (size0*OKgain > size1);


    Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "Balanced Node: Number=%d; Level=%d.\n",
        Abc_GetNtkNodeNum(pAbc), Abc_GetNtkLevelNum(pAbc))
   );

    size0 = Abc_GetNtkNodeNum(pAbc);
    snprintf(command, PDTUTIL_MAX_STR_LEN,
            "fraig -s -T %d %s ; balance", fraigTimeLimit, verbose>2 ? "-v":"");
    Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "Issuing Command: %s.\n", command)
    );
    if (0&& Cmd_CommandExecute (pAbc, command) ) {
       fprintf(stderr, "Error: Cannot Execute Command \"%s\".\n", command);
       return (0);
    }
    size1 = Abc_GetNtkNodeNum(pAbc);
    if (size0*OKgain > size1) fraigTimeLimit *= 2;
    else fraigTimeLimit /= 2;

    if (abcTimeLimit>=0 &&
       (clock() - timeInit) > 3*abcTimeLimit*MY_CLOCKS_PER_SEC/2) break;

    do {
      if (abcTimeLimit>=0 &&
       (clock() - timeInit) > abcTimeLimit*MY_CLOCKS_PER_SEC) break;
      size0 = Abc_GetNtkNodeNum(pAbc);
      snprintf(command, PDTUTIL_MAX_STR_LEN, "strash ; balance;");
      snprintf(new_command, PDTUTIL_MAX_STR_LEN,
        "%s; refactor -l; balance", command);
      //        "%s; refactor -l; balance; rewrite -l", command);
      //        "%s; refactor -l; balance; dcompress2 -b -l", command);
      snprintf(command, PDTUTIL_MAX_STR_LEN, "%s ; balance", new_command);
      Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMed_c,
        fprintf(stdout, "Issuing Command: %s.\n", command)
      );
      if ( Cmd_CommandExecute (pAbc, command) ) {
        fprintf(stderr, "Error: Cannot Execute Command \"%s\".\n", command);
        return (0);
      }
      size1 = Abc_GetNtkNodeNum(pAbc);
    } while (size0*OKgain > size1);

    Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMed_c,
      fprintf(stdout, "Rewr+Refc Node: Number=%d; Level=%d.\n",
      Abc_GetNtkNodeNum(pAbc), Abc_GetNtkLevelNum(pAbc))
    );
    totSize1 = Abc_GetNtkNodeNum(pAbc);
  } while (totSize0*TotOKgain > totSize1);

  if (verbose) snprintf(command, PDTUTIL_MAX_STR_LEN, "print_stats");
  snprintf(new_command, PDTUTIL_MAX_STR_LEN, "write_bench -l %s", fileNameW);
  //  sprintf(command, "%s; cec %s %s", command, fileNameR, fileNameW);
  Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "Issuing Command: %s.\n", new_command)
  );

  if ( Cmd_CommandExecute (pAbc, new_command) ) {
    fprintf(stderr, "Error: Cannot Execute Command \"%s\".\n", new_command);
    return (0);
  }

  timeFinal = clock() - timeInit;

  Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "Finale Node: Number=%d; Level=%d.\n",
      Abc_GetNtkNodeNum(pAbc), Abc_GetNtkLevelNum(pAbc));
    fprintf(stdout, "ABC Total Time = %6.2f sec.\n",
      (float)(timeFinal)/(float)(MY_CLOCKS_PER_SEC))
  );

  /*                         */
  /*  Stop the ABC framework */
  /*                         */

  //  Abc_Stop();
  Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "-- ABC Stopped.\n")
  );

  return (1);
}

#define CMD(command) \
{\
  if (abcTimeLimit >= (clock() - timeInit) > abcTimeLimit*MY_CLOCKS_PER_SEC) goto time_exceded;\
  if (verbose) printf("ABCSYN: %s\n", command);				\
  if ( Cmd_CommandExecute (pAbc, command) ) goto error;\
}


/**Function*************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/
int
Ddi_AbcSynVerif (
  char *fileName,
  int doUndc,
  float timeLimit,
  int pdtVerbose
)
{
  Abc_Frame_t *pAbc;
  char command[PDTUTIL_MAX_STR_LEN+1], new_command[PDTUTIL_MAX_STR_LEN+1];
  int timeInit, timeFinal;
  FILE *fp = NULL;
  int n, flagFile, result;
  int size0, size1, totSize0, totSize1, lNum0, lNum;
  float OKgain = 0.95;
  float TotOKgain = 0.90;
  int fraigTimeLimit = 10;
  float abcTimeLimit = timeLimit;
  int verbose = pdtVerbose > 3;
  result = 0;

  /*                          */
  /*  Start the ABC framework */
  /*                          */

  Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "-- ABC SYNthesis Running.\n")
  );

  if (!abcStarted) {
    Abc_Start();
    abcStarted=1;
  }
  Ddi_AbcLock ();

  pAbc = Abc_FrameGetGlobalFrame();

  /*                 */
  /*  Issue Commands */
  /*                 */

  /* Build Command List */
  snprintf(command, PDTUTIL_MAX_STR_LEN, "read %s", fileName);

  if ( Cmd_CommandExecute (pAbc, command) ) return (0);

  if (verbose) {
    fprintf(stdout, "Initial Node: Number=%d; Level=%d.\n",
	    Abc_GetNtkNodeNum(pAbc), Abc_GetNtkLevelNum(pAbc));
  }

  timeInit = clock();

  totSize0 = Abc_GetNtkNodeNum(pAbc);

  if (Abc_GetNtkLatchesNum(pAbc)>5000) goto end;

  if (doUndc) {
    CMD( "logic;undc;strash" );
  }
  
  if (Abc_GetNtkLatchesNum(pAbc)>0) {
    CMD( "&get;&lcorr;&put" );
  }
  if (Abc_GetNtkLatchesNum(pAbc)>0) {
    CMD( "&get;&scorr;&put" );
  }
  size0 = Abc_GetNtkNodeNum(pAbc); lNum0 = Abc_GetNtkLatchesNum(pAbc);
  if (Abc_GetNtkLatchesNum(pAbc)>0) {
    CMD( "dretime" );
  }
  lNum = Abc_GetNtkLatchesNum(pAbc);
  if (Abc_GetNtkLatchesNum(pAbc)>0) {
    CMD( "&get;&lcorr;&put" );
  }
  if (verbose) CMD( "print_stats" );
  if (Abc_GetNtkLatchesNum(pAbc)>0) {
    CMD( "lcorr" );
    if (verbose) CMD( "print_stats" );
  }
  if (Abc_GetNtkLatchesNum(pAbc)>0) {
    CMD( "scorr" );
    if (verbose) CMD( "print_stats" );
  }
  if (Abc_GetNtkLatchesNum(pAbc)>0) {
    CMD( "scorr -F 2" );
    if (verbose) CMD( "print_stats" );
  }
  if (Abc_GetNtkLatchesNum(pAbc)>0) {
    CMD( "scorr -F 4" );
    if (verbose) CMD( "print_stats" );
  }
  if (Abc_GetNtkLatchesNum(pAbc)>0) {
    CMD( "scorr -F 8" );
    if (verbose) CMD( "print_stats" );
  }
  if (Abc_GetNtkLatchesNum(pAbc)==0) {
    CMD( "dprove" );
  }
  if (Abc_GetFrameStatus(pAbc)==1) result = 1;
  //  if (pAbc->Status==1) resut=1;
  Ddi_AbcUnlock ();

  /*                         */
  /*  Stop the ABC framework */
  /*                         */

  //  Abc_Stop();
  Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "-- ABC Stopped.\n")
  );

 end:

  if (verbose) printf("ABCSYN: %s\n", result?"proved":"undefined result");

  return (result);

 prop_proved:

  if (verbose) printf("ABCSYN: proved\n");

  return (1);

 time_exceded:
 error:

  if (verbose) printf("ABCSYN: time/err\n");

  return (0);

}

/**Function*************************************************************

  Synopsis    [Perform scorr reduction]
  Description [Perform scorr reduction of a PdTRAV FSM after passing it to the ABC Aig form]
  SideEffects []
  SeeAlso     []

***********************************************************************/

void  
Abc_ScorrReduceFsm(
  Fsm_Fsm_t *fsm,
  Ddi_Bdd_t *care,
  Ddi_Bdd_t *latchEq,
  int indK
)
{
  Ddi_Bddarray_t *myLambda = Ddi_BddarrayDup(Fsm_FsmReadLambda(fsm));
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(myLambda);
  Ddi_Bdd_t *constrFsm = Fsm_FsmReadConstraint(fsm);
  Ddi_Bdd_t *constrPart = NULL, *carePart;
  int nc=0, nc2=0, ni0, nl0, ni1, nl1;
  int doReplace = 1;
  int outLatchCorr = latchEq!=NULL;
  int doKeepLatches = 0;

  if (constrFsm!=NULL && !Ddi_BddIsOne(constrFsm)) {
    int jj;
    constrPart = Ddi_AigPartitionTop(constrFsm, 0);
    for (jj=0; jj<Ddi_BddPartNum(constrPart); jj++) {
      Ddi_Bdd_t *c_i = Ddi_BddNot(Ddi_BddPartRead(constrPart,jj));
      Ddi_BddarrayInsertLast(myLambda,c_i);
      Ddi_Free(c_i);
      nc++;
    }
    Ddi_Free(constrPart);
  }
  if (care!=NULL && !Ddi_BddIsOne(care)) {
    int jj;
    carePart = Ddi_AigPartitionTop(care, 0);
    for (jj=0; jj<Ddi_BddPartNum(carePart); jj++) {
      Ddi_Bdd_t *c_i = Ddi_BddNot(Ddi_BddPartRead(carePart,jj));
      Ddi_BddarrayInsertLast(myLambda,c_i);
      Ddi_Free(c_i);
      nc2++; nc++;
    }
    Ddi_Free(carePart);
  }

  if (doKeepLatches) {
    Ddi_Vararray_t *ps = Fsm_FsmReadPS(fsm);
    Ddi_Bddarray_t *dummyLits = Ddi_BddarrayMakeLiteralsAig(ps, 1);
    Ddi_BddarrayAppend(myLambda,dummyLits);
    Ddi_Free(dummyLits);
  }

  Ddi_AbcLock();

  if (1) {
    Ddi_Vararray_t *v0 = Ddi_BddarraySuppVararray(Fsm_FsmReadDelta(fsm));
    Ddi_Vararray_t *v1 = Ddi_BddarraySuppVararray(myLambda);
    Ddi_VararrayDiffAcc(v0,Fsm_FsmReadPI(fsm));
    Ddi_VararrayDiffAcc(v0,Fsm_FsmReadPS(fsm));
    Ddi_VararrayDiffAcc(v1,Fsm_FsmReadPI(fsm));
    Ddi_VararrayDiffAcc(v1,Fsm_FsmReadPS(fsm));
    Pdtutil_Assert(Ddi_VararrayNum(v0)==0,"unknown var");
    Pdtutil_Assert(Ddi_VararrayNum(v1)==0,"unknown var");
    Ddi_Free(v0);
    Ddi_Free(v1);
  }

  Aig_Man_t * pTemp, *pMan = DdiAbcFsmToAig (
    Fsm_FsmReadDelta(fsm),
    myLambda,
    Fsm_FsmReadPI(fsm),
    Fsm_FsmReadPS(fsm)
  );
  Ddi_Free(myLambda);

  Aig_ManCheck(pMan);
  Aig_ManSetRegNum(pMan, Fsm_FsmReadNL(fsm));
  pMan->nConstrs = nc;

  Ssw_Pars_t Pars, * pPars = &Pars;
  Ddi_Bddarray_t *delta = Fsm_FsmReadDelta(fsm);
  Ddi_Bddarray_t *lambda = Fsm_FsmReadLambda(fsm);
  Ddi_Vararray_t *ps = Fsm_FsmReadPS(fsm);
  Ddi_Vararray_t *ns = Fsm_FsmReadNS(fsm);
  Ddi_Vararray_t *pi = Fsm_FsmReadPI(fsm);

  Ssw_ManSetDefaultParams( pPars );

  nl0 = pMan->nRegs;
  ni0 = pMan->nTruePis;
 
  fprintf(stdout, "ORIGINAL\n");
  Aig_ManPrintStats( pMan );
  pPars->fConstrs = (nc>0);
  pPars->fMergeFull = 1;
  //  pPars->fFlopVerbose = 1;
  //  pPars->fLatchCorr = 1;
  pPars->fConstCorr = 1;
  if (indK>1) {
    pPars->nFramesK = indK;
  }
  pMan = Ssw_SignalCorrespondence( pTemp = pMan, pPars );

  fprintf(stdout, "SCORR RESULT (with constrs)\n");
  Aig_ManPrintStats( pMan );

  nl1 = pMan->nRegs;
  ni1 = pMan->nTruePis;

  Pdtutil_Assert(!doKeepLatches||(nl1==nl0),"missing latches");

  if ((nl1<nl0) && outLatchCorr) {
    int i;
    int *lCorrArray = NULL;
    Ddi_Vararray_t *eqVars = Ddi_VararrayAlloc(ddiMgr,0);
    Ddi_Bddarray_t *eqSubst = Ddi_BddarrayAlloc(ddiMgr,0);

    lCorrArray = abcLatchEquivClasses(pTemp);

    for (i=0; i<nl0; i++) {
      int rep = lCorrArray[i];
      Ddi_Var_t *v = Ddi_VararrayRead(ps,i);
      Ddi_Bdd_t *subst = NULL;
      Pdtutil_Assert(rep>=-1 && rep<=nl0,"wrong lcorr id");
      if (rep == -1) {
	subst = Ddi_BddMakeConstAig(ddiMgr, 1);
      }
      else if (rep != i) {
	Ddi_Var_t *vR = Ddi_VararrayRead(ps,rep);
	subst = Ddi_BddMakeLiteralAig(vR, 1);
      }
      if (subst!=NULL) {
	Ddi_VararrayInsertLast(eqVars,v);
	Ddi_BddarrayInsertLast(eqSubst,subst);
	Ddi_Free(subst);
      }
    }

    if (Ddi_VararrayNum(eqVars)>0) {

      Ddi_Bdd_t *myEq = Ddi_BddMakeEq (eqVars,eqSubst);
      
      Ddi_DataCopy(latchEq,myEq);
      Ddi_Free(myEq);
    }
    Ddi_Free(eqVars);
    Ddi_Free(eqSubst);
    Pdtutil_Free(lCorrArray);
  }

  Aig_ManStop( pTemp );
  
  Pdtutil_Assert(ni1==ni0,"pi mapping lost by ABC");
  Pdtutil_Assert(nl1<=nl0,"latch num increased by ABC");

  if (doReplace) {
    Ddi_Vararray_t *dSupp, *lSupp;
    Ddi_Bddarray_t *newdelta = Ddi_BddarrayAlloc(ddiMgr, 0);
    Ddi_Bddarray_t *newlambda = Ddi_BddarrayAlloc(ddiMgr,0);

    while (nl0>nl1) {
      Ddi_VararrayRemove(ps,Ddi_VararrayNum(ps)-1);
      Ddi_VararrayRemove(ns,Ddi_VararrayNum(ns)-1);
      nl0--;
    }

    DdiAbcFsmFromAig(pMan, newdelta, newlambda, pi, ps);

    dSupp = Ddi_BddarraySuppVararray(newdelta);
    lSupp = Ddi_BddarraySuppVararray(newlambda);
    Ddi_VararrayUnionAcc(dSupp,lSupp);
    Ddi_Free(lSupp);
    Ddi_VararrayIntersectAcc(pi,dSupp);
    Ddi_Free(dSupp);

    ni1 = Ddi_VararrayNum(pi);

    Ddi_AbcUnlock();

    printf("Delta: %d -> %d\n", 
         Ddi_BddarrayNum(delta), Ddi_BddarrayNum(newdelta));
    printf("Lambda: %d -> %d\n", 
         Ddi_BddarrayNum(lambda), Ddi_BddarrayNum(newlambda));
    printf("latches: %d -> %d\n", nl0, nl1);
    printf("inputs: %d -> %d\n", ni0, ni1);

    Ddi_DataCopy(delta,newdelta);

    if (doKeepLatches) {
      int nlRef = Ddi_VararrayNum(Fsm_FsmReadPS(fsm));
      int i;
      for (i=0; i<nlRef; i++) {
	int last = Ddi_BddarrayNum(newlambda)-1;
	Ddi_BddarrayRemove(newlambda,last);
      }
    }

    if (nc>0) {
      int jj, nl = Ddi_BddarrayNum(newlambda)-nc;
      if (nc-nc2>0) {
	constrPart = Ddi_BddMakePartConjVoid(ddiMgr);
	for (jj=0; jj<nc; jj++) {
	  Ddi_Bdd_t *c_jj = Ddi_BddarrayRead(newlambda,nl+jj);
	  Ddi_BddPartInsertLast(constrPart,c_jj); 
	}
	Ddi_BddSetAig(constrPart);
	Ddi_DataCopy(constrFsm,constrPart);
	Ddi_Free(constrPart);
      }
      if (nc2>0) {
	carePart = Ddi_BddMakePartConjVoid(ddiMgr);
	for (jj=nc-nc2; jj<nc; jj++) {
	  Ddi_Bdd_t *c_jj = Ddi_BddarrayRead(newlambda,nl+jj);
	  Ddi_BddPartInsertLast(carePart,c_jj); 
	}
	Ddi_BddSetAig(carePart);
	Ddi_DataCopy(care,carePart);
	Ddi_Free(carePart);
      }
      for (jj=nc-1; jj>=0; jj--) {
	Ddi_BddarrayRemove(newlambda,nl+jj);
      }
    }

    Ddi_DataCopy(lambda,newlambda);
    Ddi_Free(newdelta);  
    Ddi_Free(newlambda);  
  }
  else {
    Ddi_AbcUnlock();
  }
  
  Aig_ManStop( pMan );

  // return pMan;
  return;
}

/*---------------------------------------------------------------------------*/
/* Testing purposes                                                          */
/* Last modified NOV '13                                                     */
/* Current use: Check Abc <-> Fsm convertion + SIMP                          */
/* Invoked using: -custom 1 file_name                                        */
/*---------------------------------------------------------------------------*/

void myLink(Fsm_Fsm_t *fsm)
{
// TEST CODE
  printf("Rarity  in myLink\n");
  Abc_Frame_t *pAbc;
  char command[PDTUTIL_MAX_STR_LEN+1], new_command[PDTUTIL_MAX_STR_LEN+1];
  int timeInit, timeFinal, result = 0;
  FILE *fp = NULL;
  float abcTimeLimit = 10000.0;
  char *fileName = "bench/6s31-te8.aig";
  int verbose = 0;

  /*                          */
  /*  Start the ABC framework */
  /*                          */

  Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "-- ABC SYNthesis Running.\n")
  );

  if (!abcStarted) {
    Abc_Start();
    abcStarted=1;
  }
  Ddi_AbcLock ();

  pAbc = Abc_FrameGetGlobalFrame();

  /*                 */
  /*  Issue Commands */
  /*                 */

  /* Build Command List */
  snprintf(command, PDTUTIL_MAX_STR_LEN, "read %s", fileName);

  if ( Cmd_CommandExecute (pAbc, command) ) return (0);
  if (verbose) Cmd_CommandExecute (pAbc, "print_stats");
  Cmd_CommandExecute (pAbc, "strash");
  //  Cmd_CommandExecute (pAbc, "sim3 -v");
  Cmd_CommandExecute (pAbc, "sim3 -T 4000 -O 1");
  //  Cmd_CommandExecute (pAbc, "write_aiger_cex pdt.cex");

  timeInit = clock();



  if (Abc_GetFrameStatus(pAbc)==1) result = 1;
  Ddi_AbcUnlock ();

  /*                         */
  /*  Stop the ABC framework */
  /*                         */

  //  Abc_Stop();
  Pdtutil_VerbosityLocal(verbose, Pdtutil_VerbLevelDevMed_c,
    fprintf(stdout, "-- ABC Stopped.\n")
  );

    return;
// OLDER CODE
#if 0
  Aig_Man_t * pTemp, *pMan = DdiAbcFsmToAig (
    Fsm_FsmReadDelta(fsm),
    Fsm_FsmReadLambda(fsm),
    Fsm_FsmReadPI(fsm),
    Fsm_FsmReadPS(fsm)
  );

  if (!abcStarted) {
    Abc_Start();
    abcStarted=1;
  }
  void * pAbc = Abc_FrameGetGlobalFrame();

  Aig_ManSetRegNum(pMan, Fsm_FsmReadNL(fsm));
  Aig_ManCheck(pMan);
  Aig_ManPrintStats( pMan );

  Abc_Ntk_t *ntk =  Abc_NtkFromAigPhase(pMan);
  Abc_FrameReplaceCurrentNetwork(pAbc, ntk);
  if (verbose) Cmd_CommandExecute (pAbc, "print_stats");
  Cmd_CommandExecute (pAbc, "source ../abcLink.rc;&smp");
  if (verbose) Cmd_CommandExecute (pAbc, "print_stats");
  ntk = Abc_FrameReadNtk(pAbc);
  pTemp = Abc_NtkToDar( ntk, 0, 1 );
  fprintf(stdout, "Back to aig\n");
  Aig_ManPrintStats( pTemp );

  // Get back Abc -> Fsm
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(Fsm_FsmReadDelta(fsm));
  Ddi_Bddarray_t *delta = Ddi_BddarrayAlloc(ddiMgr,0), *lambda = Ddi_BddarrayAlloc(ddiMgr,0);
  // Ddi_Vararray_t *pi = Ddi_VararrayAlloc(ddiMgr, 0), *ps = Ddi_VararrayAlloc(ddiMgr, 0);
   
  DdiAbcFsmFromAig(pTemp, delta, lambda, Fsm_FsmReadPI(fsm), Fsm_FsmReadPS(fsm));

  // Code used  to test straight back and forth, w/o any simplification step in the middle!
  fprintf(stdout, "\nAigEqualSAT\n");

  int num = Ddi_BddarrayNum(lambda), i, ok = 0;
  for(i=0;i<num;i++) {
    Ddi_Bdd_t *a = Ddi_BddarrayRead(lambda,i);
    Ddi_Bdd_t *b = Ddi_BddarrayRead(Fsm_FsmReadLambda(fsm),i);   
    if (Ddi_AigEqualSat(a,b)) ok++;
  }
  fprintf(stdout, "Lambdas matched: %d of %d\n",ok, num); 

  num = Ddi_BddarrayNum(delta);

  for(i=0, ok=0;i<num;i++) {
    Ddi_Bdd_t *a = Ddi_BddarrayRead(delta,i);
    Ddi_Bdd_t *b = Ddi_BddarrayRead(Fsm_FsmReadDelta(fsm),i);
    if (Ddi_AigEqualSat(a,b)) ok++;
  }
  fprintf(stdout, "Deltas matched: %d of %d\n",ok, num); 

  Ddi_Free(delta);
  Ddi_Free(lambda);
  // Ddi_Free(ps);
  // Ddi_Free(pi);
#endif
  return;
}

// FINE TEST

#if 0

/*---------------------------------------------------------------------------*/
/* Trying to reproduce ABC PRE_SIMP funcionalities...                        */
/* Added march 2013, still under test.                                       */
/*---------------------------------------------------------------------------*/

#define IS_TRIM_ALLOWED 0
#define SAT_TRUE 1
#define UNSAT 2
#define UNDECIDED 3
#define NO_REDUCTION 4

int G_C = 1;
int G_T = 1;
int x_factor = 1;

static void Abc_PrintStatus( 
  void *pAbc
) 
{
  fprintf(stdout, "PIs=%d,POs=%d,FF=%d,ANDs=%d\n",
    Abc_GetNtkCiNum(pAbc), Abc_GetNtkCoNum(pAbc), 
    Abc_GetNtkLatchesNum(pAbc), Abc_GetNtkAndsNum(pAbc));
  return;
}

static int Abc_Reparam (
  void *pAbc
) 
{    
    int rep_change = 0;
    int initial_pi = Abc_GetNtkCiNum(pAbc);
    char cmd[1000];
    char *f_name = Abc_GetNtkName(pAbc);

    sprintf(cmd, "&get;reparam -aig=%s_rpm.aig; r %s_rpm.aig", f_name, f_name);
    Cmd_CommandExecute (pAbc, cmd);

    if(Abc_GetNtkCiNum(pAbc) == 0) {
        fprintf(stdout, "Number of PIs reduced to 0. Added a dummy PI");
        Cmd_CommandExecute (pAbc, "addpi");
    }

    if(Abc_GetNtkCiNum(pAbc) < initial_pi) {
        fprintf(stdout, "Reparam: PIs %d => %d", initial_pi, Abc_GetNtkCiNum(pAbc));
        rep_change = 1;
    }

    return rep_change; 
}

static int Abc_IsUnsat (
  void *pAbc
) 
{ 
  return Abc_GetFrameProbStatus(pAbc) ? 1 : 0;
}

static int Abc_IsSat (
  void *pAbc
) 
{ 
  return Abc_GetFrameProbStatus(pAbc) ? 0 : 1;
}

static void Abc_Trim (
  void *pAbc
) 
{    
  if (IS_TRIM_ALLOWED)
    Abc_Reparam(pAbc);
}

static int Abc_ProcessStatus (
  void *pAbc,
  int status
)
{
  if (Abc_GetNtkLatchesNum(pAbc) == 0)
    return Abc_CheckSat(pAbc);
  return status;
}

static int Abc_InferiorSize (
  void * pAbc,
  int npi, 
  int npo, 
  int nands
)
{
  return ((npi < Abc_GetNtkCiNum(pAbc)) || (npo < Abc_GetNtkCoNum(pAbc)) || (nands < Abc_GetNtkAndsNum(pAbc)));
}


static int Abc_ScorrConstr (
  void * pAbc
)
{
    char cmd[1000];
    char *f_name = Abc_GetNtkName(pAbc);
    int na = Abc_GetNtkAndsNum(pAbc); 
    na = na < 1 ? 1 : na;

    int npo_before = Abc_GetNtkCoNum(pAbc);
    if ((na > 40000) || Abc_GetNtkCoNum(pAbc)>1)
        return NO_REDUCTION;

    sprintf(cmd, "write %s_savetemp.aig", f_name);
    Cmd_CommandExecute(pAbc, cmd);

    na = Abc_GetNtkAndsNum(pAbc); 
    na = na < 1 ? 1 : na;

    int f = 18000/na;  // THIS can create a bug 10/15/11
    f = f < 4 ? f : 4;
    f = f > 1 ? f : 1;
    if (Abc_GetNtkAndsNum(pAbc) > 18000) {
        sprintf(cmd, "unfold -s -F 2");
    } else {
        sprintf(cmd, "unfold -F %d -C 5000", f);
    }

    Cmd_CommandExecute(pAbc, cmd);

    if (Abc_GetNtkCoNum(pAbc) == npo_before)
        return NO_REDUCTION;
    if (Abc_GetNtkAndsNum(pAbc) > na) {
        sprintf(cmd, "read %s_savetemp.aig", f_name);
        Cmd_CommandExecute(pAbc, cmd);
        return NO_REDUCTION;
    }

    na = Abc_GetNtkAndsNum(pAbc); 
    na = na < 1 ? 1 : na;
    f = 1;

    fprintf(stdout, "Number of constraints = %d", Abc_GetNtkCoNum(pAbc) - npo_before);

    sprintf(cmd, "scorr -c -F %d", f);
    Cmd_CommandExecute(pAbc, cmd);
    sprintf(cmd, "fold");
    Cmd_CommandExecute(pAbc, cmd);
    Abc_Trim(pAbc);
    fprintf(stdout, "Constrained simplification: ,");
    Abc_PrintStatus(pAbc);
    return NO_REDUCTION;
}


static int Abc_TryScorrConstr ( 
  void *pAbc
)
{
  int npi = Abc_GetNtkCiNum(pAbc);
  int npo = Abc_GetNtkCoNum(pAbc);
  int nands = Abc_GetNtkAndsNum(pAbc);
  char *f_name = Abc_GetNtkName(pAbc);
  char cmd[1000];
  
  sprintf(cmd, "write %s_savetemp.aig", f_name);
  Cmd_CommandExecute(pAbc, cmd);

  int status = Abc_ScorrConstr(pAbc);
  if (Abc_InferiorSize(pAbc, npi, npo, nands)) {
    sprintf(cmd, "write %s_savetemp.aig", f_name);
    Cmd_CommandExecute(pAbc, cmd);
  }
  return status;
}

int Abc_CheckSat (
  void *pAbc
) 
{
  char cmd[1000];

  if (Abc_GetNtkLatchesNum(pAbc) != 0)
      fprintf(stdout, "Circuit is not combinational. Undecided\n");
      return -3;

  Abc_CommandExecute(pAbc, "&get");
  sprintf(cmd, "orpos;dsat -C %d", G_C);
  Abc_CommandExecute(pAbc, cmd);
  
  if(Abc_IsSat(pAbc)) {
      Abc_CommandExecute(pAbc, "&put");
      if (Abc_GetNtkAndsNum(pAbc) == 1) {
          return SAT_TRUE; //SAT_TRUE
      } else { 
          return NO_REDUCTION; // Undecided no reductions
      }
  } else if (Abc_IsUnsat(pAbc)) {
      return UNSAT; // UNSAT
  } else {
      Abc_CommandExecute(pAbc, "&put");
      return NO_REDUCTION; // Undecided no reduction
  }
}


static int Abc_RealInputsNum (
  void * pAbc
)
{
  int nri = Abc_GetNtkCiNum(pAbc);
  char cmd[1000], real_name[1000];
  char *f_name = Abc_GetNtkName(pAbc);
  strncpy(real_name, f_name, 1000);
  
  // sprintf(cmd, "write %s_savetempreal.aig", f_name); 
  sprintf(cmd, "write %s_savetempreal.aig; logic; trim; strash ;addpi", f_name);
  Cmd_CommandExecute(pAbc, cmd);
  // Abc_Reparam(pAbc);
  nri = Abc_GetNtkCiNum(pAbc);

  sprintf(cmd, "read %s_savetempreal.aig", real_name);
  Cmd_CommandExecute(pAbc, cmd);
  return nri;
}

static float Abc_RelCost (
  void * pAbc,
  int pi,
  int latches,
  int ands  
)
{

  int current_pi = Abc_GetNtkCiNum(pAbc);
  int current_latches = Abc_GetNtkLatchesNum(pAbc);
  int current_ands = Abc_GetNtkAndsNum(pAbc);

  if (current_latches == 0 && latches > 0)
    return -10.0;

  if (pi == 0 || latches == 0 || ands == 0)
    return 100.0;

  int real_pi = Abc_RealInputsNum(pAbc);
  float ri = ((float)(real_pi)-(float)(pi))/((float)(pi));
  float rl = ((float)(current_latches)-(float)(latches))/((float)(latches));
  float ra = ((float)(current_ands)-(float)(ands))/((float)(ands));

  return (float)(1*ri + 5*rl + 0.2*ra);
}

static void Abc_BestFwrdMin (
  void *pAbc
) 
{
  char *f_name = Abc_GetNtkName(pAbc);
  char cmd[1000], real_name[1000];
  strncpy(real_name, f_name, 1000);

  int best_method = -1;
  int best_pi = Abc_GetNtkCiNum(pAbc);
  int best_latches = Abc_GetNtkLatchesNum(pAbc);
  int best_ands = Abc_GetNtkAndsNum(pAbc);

  sprintf(cmd, "write %s_best_aig.aig", real_name);
  Cmd_CommandExecute(pAbc, cmd);

  char funcs[2][1000] = {
    "dretime;&get;&lcorr;&dc2;&scorr;&put;dretime",
    "dretime -m;&get;&lcorr;&dc2;&scorr;&put;dretime"
  };

  int i = 0;
  for(i=0;i<2;i++) {
    sprintf(cmd, funcs[i]);
    Cmd_CommandExecute(pAbc, cmd);
    
    int status = Abc_GetFrameProbStatus(pAbc);
    if (status != -1) {
      best_method = i;
      if (status == UNSAT) {
        fprintf(stdout, "BestFwd %d proved UNSAT\n", i);
      } else {
        fprintf(stdout, "BestFwd %d found cex\n", i);
      }
      break;
    } else {
      float cost = Abc_RelCost(pAbc, best_pi, best_latches, best_ands);
      // printf("%d > ", i);
      // Abc_PrintStatus(pAbc);
      if (cost < 0.0) {
        best_pi = Abc_GetNtkCiNum(pAbc);
        best_latches = Abc_GetNtkLatchesNum(pAbc);
        best_ands = Abc_GetNtkAndsNum(pAbc);
        best_method = i;
        sprintf(cmd, "write %s_best_aig.aig", real_name);
        Cmd_CommandExecute(pAbc, cmd);
      }
    }
  }

  sprintf(cmd, "read %s_best_aig.aig", real_name);
  Cmd_CommandExecute(pAbc, cmd);
}

static int Abc_CheckSize (
  void *pAbc,
  int npi,
  int npo,
  int nands,
  int nlatches
) 
{
  return ((npi == Abc_GetNtkCiNum(pAbc)) && 
          (npo == Abc_GetNtkCoNum(pAbc)) &&
          (nands == Abc_GetNtkAndsNum(pAbc)) &&
          (nlatches == Abc_GetNtkLatchesNum(pAbc)));
}

static int Abc_GetStatus (
  void *pAbc
) 
{
  if (Abc_GetNtkAndsNum(pAbc) == 0)
      return Abc_CheckSat(pAbc);
  int status = Abc_GetFrameProbStatus(pAbc); 
  if (status == 1)
      status = UNSAT;
  if (status == -1) 
      status = UNDECIDED;
  return status;
}

static int Abc_Simplify (
  void *pAbc
) 
{
  char real_name[1000], cmd[1000];
  char *f_name = Abc_GetNtkName(pAbc);
  Abc_SetGlobals(pAbc);

  sprintf(cmd, "&get;&scl;&lcorr;&put");
  Cmd_CommandExecute(pAbc, cmd);

  int p_40 = 0;
  int n = Abc_GetNtkAndsNum(pAbc);

  if (n >= 70000) {
    sprintf(cmd, "&get;&scorr -C 0;&put");
    Cmd_CommandExecute(pAbc, cmd);
  }
  
  n = Abc_GetNtkAndsNum(pAbc);
  if (n >= 100000) {
    sprintf(cmd, "&get;&scorr -k;&put");
    Cmd_CommandExecute(pAbc, cmd);
  }
  if (70000 < n && n < 100000) {
    p_40 = 1;
    sprintf(cmd, "&get;&dc2;&put;dretime;&get;&lcorr;&dc2;&put;dretime;&get;&scorr;&fraig;&dc2;&put;dretime");
    Cmd_CommandExecute(pAbc, cmd);
    n = Abc_GetNtkAndsNum(pAbc);

    if (n < 80000) {
      sprintf(cmd, "&get;&scorr -F 2;&put;set autoexec; balance -l; resub -K 6 -l; drw -l; resub -K 6 -N 2 -l; refactor -l; resub -K 8 -l; balance -l; resub -K 8 -N 2 -l; drw -l; resub -K 10 -l; rwz -l; resub -K 10 -N 2 -l; balance -l; resub -K 12 -l; refactor -z -l; resub -K 12 -N 2 -l; drw -z -l; balance -l; ps");
      Cmd_CommandExecute(pAbc, cmd);
    } else {
      sprintf(cmd, "set autoexec; balance -l; resub -K 6 -l; drw -l; resub -K 6 -N 2 -l; refactor -l; resub -K 8 -l; balance -l; resub -K 8 -N 2 -l; drw -l; resub -K 10 -l; rwz -l; resub -K 10 -N 2 -l; balance -l; resub -K 12 -l; refactor -z -l; resub -K 12 -N 2 -l; drw -z -l; balance -l; ps");
      Cmd_CommandExecute(pAbc, cmd);
    }
  }
  n = Abc_GetNtkAndsNum(pAbc);

  if (60000 < n  && n <= 70000) {
    if (!p_40) {
      sprintf(cmd, "&get;&dc2;&put;dretime;&get;&lcorr;&dc2;&put;dretime;&get;&scorr;&fraig;&dc2;&put;dretime");
      Cmd_CommandExecute(pAbc, cmd);
      sprintf(cmd, "&get;&scorr -F 2;&put;set autoexec; balance -l; resub -K 6 -l; drw -l; resub -K 6 -N 2 -l; refactor -l; resub -K 8 -l; balance -l; resub -K 8 -N 2 -l; drw -l; resub -K 10 -l; rwz -l; resub -K 10 -N 2 -l; balance -l; resub -K 12 -l; refactor -z -l; resub -K 12 -N 2 -l; drw -z -l; balance -l; ps");
      Cmd_CommandExecute(pAbc, cmd);
    } else {
      sprintf(cmd, "set autoexec; balance -l; resub -K 6 -l; drw -l; resub -K 6 -N 2 -l; refactor -l; resub -K 8 -l; balance -l; resub -K 8 -N 2 -l; drw -l; resub -K 10 -l; rwz -l; resub -K 10 -N 2 -l; balance -l; resub -K 12 -l; refactor -z -l; resub -K 12 -N 2 -l; drw -z -l; balance -l; ps");
      Cmd_CommandExecute(pAbc, cmd);
    }
  }          
  n = Abc_GetNtkAndsNum(pAbc);
  if (n <= 70000) {
    // NOTE: The following command DOESN'T WORK with the current version of ABC embedeed in PdTRAV
    // sprintf(cmd, "scl -m;drw;dretime;lcorr;drw;dretime");
    // Cmd_CommandExecute(pAbc, cmd);

    int nn = n < 1 ? 1 : n;
    int m = 70000/nn < 16 ? (int) 70000/nn : 16; 
    if (m >= 1) {
      int j = 1;
      while (j <= m) {
        int npi = Abc_GetNtkCiNum(pAbc);
        int npo = Abc_GetNtkCoNum(pAbc);
        int nands = Abc_GetNtkAndsNum(pAbc);
        int nlatches = Abc_GetNtkLatchesNum(pAbc);
        if (j<8) {
          sprintf(cmd, "dc2");
          Cmd_CommandExecute(pAbc, cmd);
        } else {
          sprintf(cmd, "set autoexec; &get; balance -l; resub -K 6 -l; drw -l; resub -K 6 -N 2 -l; refactor -z -l; resub -K 8 -l; balance -l; resub -K 8 -N 2 -l; drw -l; resub -K 10 -l; rwz -l; resub -K 10 -N 2 -l; balance -l; resub -K 12 -l; refactor -z -l; resub -K 12 -N 2 -l; drw -z -l; balance -l; &put; ps");
          Cmd_CommandExecute(pAbc, cmd);
        }
        sprintf(cmd, "scorr -C 5000 -F %d", j);
        Cmd_CommandExecute(pAbc, cmd);

        if (Abc_CheckSize(pAbc, npi, npo, nands, nlatches))
            break;
        j = 2*j;
        
        if (Abc_GetNtkAndsNum(pAbc) >= 0.98 * nands)
             break;
      }
      int npi = Abc_GetNtkCiNum(pAbc);
      int npo = Abc_GetNtkCoNum(pAbc);
      int nands = Abc_GetNtkAndsNum(pAbc);
      int nlatches = Abc_GetNtkLatchesNum(pAbc);
      if (! Abc_CheckSize(pAbc, npi, npo, nands, nlatches)) {
          fprintf(stdout, "\n");
      }
    }
  }
  return Abc_GetStatus(pAbc);
}

static void Abc_TryTemp
(
  void *pAbc,
  int maxTime
)
{
  Abc_Trim(pAbc);
  char cmd[1000], real_name[1000];
  char *f_name = Abc_GetNtkName(pAbc);
  strncpy(real_name, f_name, 1000);
  sprintf(cmd, "write %s_best.aig", real_name);
  Cmd_CommandExecute(pAbc, cmd);

  int npi = Abc_GetNtkCiNum(pAbc);
  int nands = Abc_GetNtkAndsNum(pAbc);
  int nlatches = Abc_GetNtkLatchesNum(pAbc);

  // In the original version there is a timer that limits the time spent doing this.
  // For now, no timer is issued...
  sprintf(cmd, "tempor -s; &get; &trim -o; &put; &get; &scorr; &put; &get; &trim -o; &put; tempor; &get; &trim -o; &put; &get; &scorr; &put; &get; &trim -o; &put");
  Cmd_CommandExecute(pAbc, cmd);

  float cost = Abc_RelCost(pAbc, npi, nlatches, nands);

  if (cost < 0.01) {
    Abc_PrintStatus(pAbc);
    return;
  } else {
    sprintf(cmd, "read %s_best.aig", real_name);
    Cmd_CommandExecute(pAbc, cmd);
  }
}

static void Abc_TryTemps(
  void *pAbc,
  int maxTime
)
{
  int nl = Abc_GetNtkLatchesNum(pAbc);
  int na = Abc_GetNtkAndsNum(pAbc);
  int np = Abc_GetNtkCiNum(pAbc);
  int keepIterating = 3;
  
  while(keepIterating--) {

    Abc_TryTemp(pAbc, maxTime);
    if ((nl == Abc_GetNtkLatchesNum(pAbc) && na == Abc_GetNtkAndsNum(pAbc) && np == Abc_GetNtkCiNum(pAbc)) ||
        (Abc_GetNtkAndsNum(pAbc) > 0.9 * na)) {
      break;
    } else if (Abc_GetNtkLatchesNum(pAbc) == 0) {
      break;
    } else {
      nl = Abc_GetNtkLatchesNum(pAbc);
      na = Abc_GetNtkAndsNum(pAbc);
      np = Abc_GetNtkCiNum(pAbc);
    }
  }
}

static int compare (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

static int * Abc_Factors (
  int n,
  int *fLen
)
{
  int factors[16]  = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53};
  int *l = calloc (18, sizeof(int));
  l[0] = 1;
  int n_f = 16;
  int i = 0, last_f = 1, current_f;
  int nn = n;

  while (n > 1) {
    for(i=0;i<n_f;i++) {
      current_f = factors[i];
      if (current_f>=nn)
        break;
      if (n%current_f == 0) {
        l[last_f++] = current_f;
        n = n/current_f;
      }
    }
    if (n != 1) {
      l[last_f++] = n;
    }
    break;
  }
  qsort (l, last_f, sizeof(int), compare);
  *fLen = last_f;
  return l;
}

static void calcProduct (
  int *sp,
  int *mark,
  int *f,
  int fLen,
  int i,
  int *index
)
{
  if (i >= fLen) {
    int p = 1;
    for(i=0;i<fLen;i++) 
      p = p * (mark[i] ? f[i] : 1);
    sp[(*index)++] = p;
    return;
  } else {
    calcProduct(sp, mark, f, fLen, i+1, index);
    mark[i] = 1;
    calcProduct(sp, mark, f, fLen, i+1, index);
    mark[i] = 0;
  }  
}

static void dropDuplicates (
  int *sp_tmp,
  int comb,
  int *spLen
)
{
  int i, last = -1;
  *spLen = 0;
  for(i=0;i<comb;i++) {
    if (sp_tmp[i] == last) 
      continue;
    else {
      sp_tmp[(*spLen)++] = sp_tmp[i];
      last = sp_tmp[i];
    }
  }
  sp_tmp[(*spLen)] = -1; // Just a safety sentinel...
}

static int * Abc_Subproducts (
  int *f,
  int fLen,
  int *spLen
)
{
  int comb = pow(2,fLen), index = 0;
  int *sp = (int*) calloc(comb, sizeof(int));
  if (sp == NULL)
    exit(-1);
  int *mark = (int*) calloc(fLen, sizeof(int));
  if (mark == NULL)
    exit(-1);
  int i = 0;
  calcProduct(sp, mark, f, fLen, i, &index);
  qsort (sp, comb, sizeof(int), compare);
  dropDuplicates(sp, comb, spLen);
  free(mark);

  return sp;
}

static int * Abc_OkPhases (
  void *pAbc,
  int n,
  int *zLen
)
{
  int fLen = -1, spLen = -1;
  int *f = Abc_Factors(n, &fLen);
  int *sp = Abc_Subproducts(f, fLen, &spLen);
  int nands = Abc_GetNtkAndsNum(pAbc), i = 0;
  for(i=0;i<spLen;i++)
    if (sp[i] * nands >= 90000)
      break;
  *zLen = i;
  return sp;
}

static void Abc_TryPhase(
  void *pAbc
)
{
  char cmd[1000], real_name[1000];
  char *f_name = Abc_GetNtkName(pAbc);
  int *z;
  strncpy(real_name, f_name, 1000);

  Abc_Trim(pAbc);
  int n = Abc_GetNtkPhases(pAbc);

  if ((n == 1) || (Abc_GetNtkAndsNum(pAbc) > 45000))
      return;

  fprintf(stdout, "Trying phase abstraction - Max phase = %d\n", n);
  sprintf(cmd, "write %s_phase_temp.aig", real_name);
  Cmd_CommandExecute(pAbc, cmd);

  int npi = Abc_GetNtkCiNum(pAbc);
  int npo = Abc_GetNtkCoNum(pAbc);
  int nlatches = Abc_GetNtkLatchesNum(pAbc);
  int nands = Abc_GetNtkAndsNum(pAbc);
  int zLen = -1;
  z = Abc_OkPhases(pAbc, n, &zLen);

  if (zLen == 1)
      return;

  int p = z[1];
  sprintf(cmd, "phase -F %d", p);
  Cmd_CommandExecute(pAbc, cmd);

  if (npo == Abc_GetNtkCoNum(pAbc)) {
    fprintf(stdout, "Phase %d is incompatible\n", p);
    sprintf(cmd, "read %s_phase_temp.aig", real_name);
    Cmd_CommandExecute(pAbc, cmd);
    if (zLen < 3) {
        return;
    } else {
      p = z[2];
      fprintf(stdout, "Trying phase %d\n", p);
      sprintf(cmd, "phase -F %d", p);
      Cmd_CommandExecute(pAbc, cmd);
      if (npo == Abc_GetNtkCoNum(pAbc)) {
        fprintf(stdout, "Phase %d is incompatible\n", p);
        sprintf(cmd, "read %s_phase_temp.aig", real_name);
        Cmd_CommandExecute(pAbc, cmd);
        return;
      }
    }
  }
  fprintf(stdout, "Simplifying with %d phases: => ", p);
  Abc_Simplify(pAbc);
  Abc_Trim(pAbc);
  Abc_PrintStatus(pAbc);

  npi = Abc_GetNtkCiNum(pAbc);
  npo = Abc_GetNtkCoNum(pAbc);
  nlatches = Abc_GetNtkLatchesNum(pAbc);
  nands = Abc_GetNtkAndsNum(pAbc);
  float cost = Abc_RelCost(pAbc, npi, nlatches, nands);
     
  fprintf(stdout, "New relative cost = %f\n", cost);
  if (cost <  -0.01) {
    sprintf(cmd, "write %s_phase_temp.aig", real_name);
    Cmd_CommandExecute(pAbc, cmd);

    if ((Abc_GetNtkLatchesNum(pAbc) == 0) || (Abc_GetNtkAndsNum(pAbc) == 0))
        return;
    if (Abc_GetNtkPhases(pAbc) == 1)
        return;
    else {
        Abc_TryPhase(pAbc);
        return;
    }
  } else if (zLen > 2) {
    sprintf(cmd, "read %s_phase_temp.aig", real_name);
    Cmd_CommandExecute(pAbc, cmd);
    if (p == z[2])
      return;
    p = z[2];
    
    fprintf(stdout, "Trying phase = %d: => ", p);
    sprintf(cmd, "phase -F %d", p);
    Cmd_CommandExecute(pAbc, cmd);
    if (npo == Abc_GetNtkCoNum(pAbc)) {
      fprintf(stdout, "Phase %d is incompatible\n", p);
      return;
    }
    Abc_PrintStatus(pAbc);
    fprintf(stdout, "Simplifying with %d phases: => ", p);
    Abc_Simplify(pAbc);
    Abc_Trim(pAbc);
    Abc_PrintStatus(pAbc);
    cost = Abc_RelCost(pAbc, npi, nlatches, nands);
    fprintf(stdout, "New relative cost = %f\n", cost);
    if (cost < -0.01) {
      fprintf(stdout, "Phase abstraction with %d phases obtained:", p);
      Abc_PrintStatus(pAbc);
      sprintf(cmd, "write %s_phase_temp.aig", real_name);
      Cmd_CommandExecute(pAbc, cmd);
      if ((Abc_GetNtkLatchesNum(pAbc) == 0) || (Abc_GetNtkAndsNum(pAbc) == 0))
          return;
      if (Abc_GetNtkPhases(pAbc) == 1)
          return;
      else {
          Abc_TryPhase(pAbc);
          return;
      }
    }
  }
  sprintf(cmd, "read %s_phase_temp.aig", real_name);
  Cmd_CommandExecute(pAbc, cmd);
  return;
}

static int Abc_PreSimp (
  void *pAbc
)
{
  char cmd[1000];

  fprintf(stdout, "PdTRAV_ABC: PRE_SIMP started\n"); 
  
  sprintf(cmd, "&get; &scl; &put");
  if ( Cmd_CommandExecute (pAbc, cmd) ) {
    fprintf(stderr, "Cannot Execute Command \"%s\".\n", cmd);
    return -1;
  }

  if (Abc_GetNtkAndsNum(pAbc) > 200000 || Abc_GetNtkLatchesNum(pAbc) > 50000 || Abc_GetNtkCiNum(pAbc) > 40000) {
    fprintf(stdout, "Problem too large to try pre_simp\n");
    return -1;
  }
  if ((Abc_GetNtkAndsNum(pAbc) > 0) || (Abc_GetNtkLatchesNum(pAbc) > 0))
    Abc_Trim(pAbc);
  if (Abc_GetNtkLatchesNum(pAbc) == 0)
    return Abc_CheckSat(pAbc);

  Abc_BestFwrdMin(pAbc); // Execute MinRetime and FwdRetime

  Abc_PrintStatus(pAbc);

  int status = Abc_TryScorrConstr(pAbc);
  if ((Abc_GetNtkAndsNum(pAbc) > 0) || (Abc_GetNtkLatchesNum(pAbc)>0))
    Abc_Trim(pAbc);
  if (Abc_GetNtkLatchesNum(pAbc) == 0)
    return Abc_CheckSat(pAbc);
  
  status = Abc_ProcessStatus(pAbc, status);
  if (status <= UNSAT) 
    return status;

  fprintf(stdout, "Performing simplify\n");
  Abc_Simplify(pAbc);
  fprintf(stdout, "Simplify: \n");
  Abc_PrintStatus(pAbc);

  if ( Abc_GetNtkLatchesNum(pAbc) == 0)
      return Abc_CheckSat(pAbc);
  if (1 || IS_TRIM_ALLOWED) {
      int t = 0.3 * G_T < 15 ? 0.3 *G_T : 15; 
      if (1) { 
        Abc_TryTemps(pAbc, 15);
        if ( Abc_GetNtkLatchesNum == 0)
          return Abc_CheckSat(pAbc);

        Abc_TryPhase(pAbc);
        if ( Abc_GetNtkLatchesNum(pAbc) == 0)
          return Abc_CheckSat(pAbc);
      }
      if ((Abc_GetNtkAndsNum(pAbc) > 0) || (Abc_GetNtkLatchesNum(pAbc) > 0)) {
        Abc_Trim(pAbc);
      } 
  }
  
  fprintf(stdout, "PdTRAV_ABC: PRE_SIMP terminated\n");

  // DUMP THE OUTCOME!
  sprintf(cmd, "write /home/loiacono/pdtAbc.aig");
  Cmd_CommandExecute(pAbc, cmd);
  exit(-1);
  return Abc_ProcessStatus(pAbc, status);
}

void Abc_SetGlobals(
  void *pAbc
)
{
    int nl = Abc_GetNtkLatchesNum(pAbc);
    int na = Abc_GetNtkAndsNum(pAbc);
    int np = Abc_GetNtkCiNum(pAbc);
    int gcFactor = 3*na+500*(nl+np);

    G_C = x_factor * (gcFactor < 100000 ? gcFactor : 100000);
    G_T = x_factor * (G_C/2000 < 75 ? G_C / 2000 : 75);
    G_T = G_T > 1 ? G_T : 1 ;
}

int Abc_CustomLink (
  char *designName
)
{
  void *pAbc;
  char cmd[1000];

  if (designName == NULL)
    return -1;

  sprintf(cmd, "read_aiger %s", designName);

  if (!abcStarted) {
    Abc_Start();
    abcStarted=1;
  }
  pAbc = Abc_FrameGetGlobalFrame();
  if ( Cmd_CommandExecute (pAbc, cmd) ) {
    fprintf(stderr, "Cannot Execute Command \"%s\".\n", cmd);
    return -1;
  }

  fprintf(stdout, "Initial stats: ");
  Abc_PrintStatus(pAbc);
  Abc_SetGlobals(pAbc);
  Abc_PreSimp(pAbc);
  //  Abc_Stop();

  return 0;
}

#endif

Pdtutil_Array_t* getFaninsGateIds(Ddi_AigAbcInfo_t *aigAbcInfo, int j, int rootCnfId){
  Pdtutil_Array_t* outFaninsGateId = NULL;
  Aig_Obj_t * pObj = aigAbcInfo->visitedAbcObj[j];
  

  //Salta i nodi non mappati
  if (!Aig_ObjIsCi(pObj)&&(aigAbcInfo->pCnf->pObj2Count[Aig_ObjId(pObj)]<=0)) {
    return outFaninsGateId;
  }
  
  Cnf_Cut_t * pCut = Cnf_ObjBestCut(pObj);
  if(pCut != NULL){
    int k, n = (int)pCut->nFanins;
    outFaninsGateId = Pdtutil_IntegerArrayAlloc(n);
    for ( k = 0; k < n; k++ )
    {
      int fiAbcCnfId = pCut->pFanins[k];
      Pdtutil_Assert(fiAbcCnfId <= aigAbcInfo->abcCnfMaxId, "abc id out of bounds" );
      int gateId = aigAbcInfo->objId2gateId[fiAbcCnfId];
      //int fiCnfId = aigAbcInfo->abc2PdtIdMap[fiAbcCnfId];
      Pdtutil_IntegerArrayInsertLast(outFaninsGateId, gateId);
    }
  }
  
  return outFaninsGateId;
}

/**Function*************************************************************

  Synopsis    [Perform scorr reduction]
  Description [Perform scorr reduction of a PdTRAV FSM after passing it to the ABC Aig form]
  SideEffects []
  SeeAlso     []

***********************************************************************/

int
Ddi_AbcReduce(
  Ddi_Bddarray_t *delta,
  Ddi_Bddarray_t *lambda,
  Ddi_Vararray_t *pi,
  Ddi_Vararray_t *ps,
  Ddi_Vararray_t *ns,
  float compactTh
)
{
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(delta);
  Ddi_Bddarray_t *newdelta, *newlambda;
  Aig_Man_t *pMan;
  Abc_Ntk_t *ntk;
  void *pAbc;
  int ni0, ni1, nl0, nl1;
  char cmd[1000];
  int ret = 0;
  int abcNum, abcNum0, lNum, lNum0;
  int verbose=0;

  if (Ddi_MgrReadVerbosity(ddiMgr) > Pdtutil_VerbLevelDevMin_c) {
    verbose = 1;
  }

  nl0 = Ddi_VararrayNum(ps);
  ni0 = Ddi_VararrayNum(pi);

  if (nl0>200000 && Ddi_BddarraySize(delta)>3000000) return 0;

  Ddi_AbcLock();

  pAbc = Abc_FrameGetGlobalFrame();

  pMan = DdiAbcFsmToAig(delta, lambda, pi, ps);
  Aig_ManSetRegNum(pMan, Ddi_VararrayNum(ps));
  Aig_ManCheck(pMan);
  ntk = Abc_NtkFromAigPhase(pMan);
  Abc_FrameReplaceCurrentNetwork(pAbc, ntk);

  if (verbose) Cmd_CommandExecute (pAbc, "print_stats");
  Cmd_CommandExecute (pAbc, "&get;&scl;&put");
  abcNum = abcNum0 = Abc_GetNtkNodeNum(pAbc);
  lNum = lNum0 = Abc_GetNtkLatchesNum(pAbc);
  if (abcNum>0) {
    Cmd_CommandExecute (pAbc, "&get;&lcorr;&put");
    abcNum = Abc_GetNtkNodeNum(pAbc);
    lNum = Abc_GetNtkLatchesNum(pAbc);
    if (abcNum>0) {
      Cmd_CommandExecute (pAbc, "&get;&scorr -t 30.0;&put");
      abcNum = Abc_GetNtkNodeNum(pAbc);
      lNum = Abc_GetNtkLatchesNum(pAbc);
    }
    if (0 && abcNum>0 && lNum>0) {
      Cmd_CommandExecute (pAbc, "dretime");
      lNum = Abc_GetNtkLatchesNum(pAbc);
      Cmd_CommandExecute (pAbc, "lcorr");
      lNum = Abc_GetNtkLatchesNum(pAbc);
      Cmd_CommandExecute (pAbc, "scorr");
      lNum = Abc_GetNtkLatchesNum(pAbc);
      Cmd_CommandExecute (pAbc, "scorr -C 1000 -F 2");
      abcNum = Abc_GetNtkNodeNum(pAbc);
      lNum = Abc_GetNtkLatchesNum(pAbc);
    }
    if (abcNum>0 && abcNum<100000) {
      Cmd_CommandExecute (pAbc, "&get;&dc2;&lcorr;&dc2;&scorr -t 30.0;&fraig;&dc2;&put");
      lNum = Abc_GetNtkLatchesNum(pAbc);
      if (lNum<lNum0 && lNum<5000) {
	int k;
	for (k=1; lNum>0 && k<=4; k *= 2) {
	  int n0 = lNum;
	  int a0 = abcNum;
	  sprintf(cmd,"scorr -F %d; &get;&scl;&put",k);
	  Cmd_CommandExecute (pAbc,cmd); 
	  lNum = Abc_GetNtkLatchesNum(pAbc);
	  abcNum = Abc_GetNtkNodeNum(pAbc);
          if (lNum==n0 && abcNum>a0*0.95) break;
	  if (verbose) Cmd_CommandExecute (pAbc, "print_stats");
	}
      }
      abcNum = Abc_GetNtkNodeNum(pAbc);
      if (abcNum>0 && abcNum<100000) {
	//  Cmd_CommandExecute (pAbc, "phase -F 2");
	Cmd_CommandExecute (pAbc, "&get;&scl;&lcorr;&put");
	abcNum = Abc_GetNtkNodeNum(pAbc);
	if (abcNum>0 && abcNum<100000) {
	  Cmd_CommandExecute (pAbc, "&get;&dc2;&lcorr;&dc2;&scorr;&fraig;&dc2;&put");
	}
      }
    }
  }
  Cmd_CommandExecute (pAbc, "&get;&scl;&put");
  if (verbose) Cmd_CommandExecute (pAbc, "print_stats");

  ntk = Abc_FrameReadNtk(pAbc);
  pMan = Abc_NtkToDar(ntk, 0, 1);
  newdelta = Ddi_BddarrayAlloc(ddiMgr, 0);
  newlambda = Ddi_BddarrayAlloc(ddiMgr,0);

  nl1 = pMan->nRegs;
  ni1 = pMan->nTruePis;

  Pdtutil_Assert(ni1==ni0,"pi mapping lost by ABC");
  Pdtutil_Assert(nl1<=nl0,"latch num increased by ABC");

  if (compactTh<=0.0 || nl1 < nl0*compactTh || abcNum < abcNum0*compactTh) {
    Ddi_Vararray_t *dSupp, *lSupp;
    ret = 1;
    while (nl0>nl1) {
      Ddi_VararrayRemove(ps,Ddi_VararrayNum(ps)-1);
      Ddi_VararrayRemove(ns,Ddi_VararrayNum(ns)-1);
      nl0--;
    }

    DdiAbcFsmFromAig(pMan, newdelta, newlambda, pi, ps);

    dSupp = Ddi_BddarraySuppVararray(newdelta);
    lSupp = Ddi_BddarraySuppVararray(newlambda);
    Ddi_VararrayUnionAcc(dSupp,lSupp);
    Ddi_Free(lSupp);
    Ddi_VararrayIntersectAcc(pi,dSupp);
    Ddi_Free(dSupp);

    ni1 = Ddi_VararrayNum(pi);

    Ddi_AbcUnlock();

    printf("Delta: %d -> %d\n", 
         Ddi_BddarrayNum(delta), Ddi_BddarrayNum(newdelta));
    printf("Lambda: %d -> %d\n", 
         Ddi_BddarrayNum(lambda), Ddi_BddarrayNum(newlambda));
    printf("latches: %d -> %d\n", nl0, nl1);
    printf("inputs: %d -> %d\n", ni0, ni1);

    Ddi_DataCopy(delta,newdelta);
    Ddi_DataCopy(lambda,newlambda);
    
  }
  else {
    Ddi_AbcUnlock();
  }
  
  Ddi_Free(newdelta);
  Ddi_Free(newlambda);

  return ret;
}

/**Function*************************************************************

  Synopsis    [Perform scorr reduction]
  Description [Perform scorr reduction of a PdTRAV FSM after passing it to the ABC Aig form]
  SideEffects []
  SeeAlso     []

***********************************************************************/
Ddi_Bdd_t *
Ddi_AbcCexFromAig(
  void *pCexV, 
  Ddi_Vararray_t *pi,
  Ddi_Vararray_t *is,
  void *pAigV
)
{
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(pi);
  Ddi_Bdd_t *returnCex = Ddi_BddMakePartConjVoid(ddiMgr);
  Abc_Cex_t *pCex = (Abc_Cex_t *)pCexV; 
  Aig_Man_t *pAig = (Aig_Man_t *)pAigV;
  int f, k, b;

  b = pCex->nRegs;

  for (k = 0; k < Ddi_VararrayNum(pi); k++) {
    Ddi_Var_t *v_k = Ddi_VararrayRead(pi,k);
    if (strncmp(Ddi_VarName(v_k),"PDT_STUB_PI_",12)==0) continue;
    Ddi_VarWriteMark(v_k,1);
  } 
  for ( f = 0; f <= pCex->iFrame; f++) {
    Ddi_Bdd_t *cex_f = Ddi_BddMakeConstAig(ddiMgr, 1);
    for ( k = 0; k < pCex->nPis; k++) {
      int useStub = (f==0 && is!=NULL);
      int val = Abc_InfoHasBit(pCex->pData, b++);
      Ddi_Var_t *v_k = Ddi_VararrayRead(useStub?is:pi,k);
      if (useStub) {
	if (Ddi_VarReadMark(v_k)!=0) continue;
      }
      else {
	if (Ddi_VarReadMark(v_k)==0) continue;
      }
      Ddi_Bdd_t *l_i = Ddi_BddMakeLiteralAig(v_k, val);
      Ddi_BddAndAcc(cex_f,l_i);
      Ddi_Free(l_i);
    } 
    Ddi_BddPartInsertLast(returnCex,cex_f);
    Ddi_Free(cex_f);
  } 
  assert( b == pCex->nBits );
  Ddi_VararrayWriteMark(pi,0);

  return returnCex;
}


/**Function*************************************************************

  Synopsis    [Perform scorr reduction]
  Description [Perform scorr reduction of a PdTRAV FSM after passing it to the ABC Aig form]
  SideEffects []
  SeeAlso     []

***********************************************************************/
Ddi_Bdd_t *
Ddi_AbcRaritySim(
  Ddi_Bddarray_t *delta,
  Ddi_Bddarray_t *lambda,
  Ddi_Vararray_t *pi,
  Ddi_Vararray_t *ps,
  Ddi_Vararray_t *ns,
  Ddi_Vararray_t *is
)
{
  Ddi_Mgr_t *ddiMgr = Ddi_ReadMgr(delta);
  Aig_Man_t *pMan;
  Abc_Ntk_t *ntk;
  void *pAbc;
  int ni0, ni1, nl0, nl1;
  char cmd[1000];
  Ddi_Bdd_t *cex=NULL;
  int verbose=0;

  if (Ddi_MgrReadVerbosity(ddiMgr) > Pdtutil_VerbLevelDevMin_c) {
    verbose = 1;
  }

  Ddi_AbcLock();

  //  pAbc = Abc_FrameAllocate();
  //Abc_FrameInit( pAbc );

  pMan = DdiAbcFsmToAig(delta, lambda, pi, ps);
  Aig_ManSetRegNum(pMan, Ddi_VararrayNum(ps));
  
  Aig_ManCheck(pMan);
  ntk = Abc_NtkFromAigPhase(pMan);
  pAbc = Abc_FrameGetGlobalFrame();
  Abc_FrameReplaceCurrentNetwork(pAbc, ntk);

  Ddi_AbcUnlock();

  if (verbose) Cmd_CommandExecute (pAbc, "print_stats");
  Cmd_CommandExecute (pAbc, "strash");
  //  Cmd_CommandExecute (pAbc, "sim3 -v");
  Cmd_CommandExecute (pAbc, "sim3 -T 4000");
  Cmd_CommandExecute (pAbc, "write_aiger_cex pdt.cex");

  Ddi_AbcLock();

  void * pCex = Abc_FrameReadCex((Abc_Frame_t *)pAbc);
  if (pCex != NULL) {
    cex = Ddi_AbcCexFromAig((void *)pCex, pi, is, (void *)pMan);
  }

  Ddi_AbcUnlock();

  return cex;
}

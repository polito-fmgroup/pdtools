/**CFile****************************************************************

  FileName    [mainFrame.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [The main package.]

  Synopsis    [The global framework resides in this file.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: mainFrame.c,v 1.1.1.1 2021/09/11 09:57:46 cabodi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "mainInt.h"
#include "bool/dec/dec.h"
#include "misc/extra/extraBdd.h"


ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Abc_Frame_t * s_GlobalFrame = NULL;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [APIs to access parameters in the flobal frame.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_FrameReadStore()                             { return s_GlobalFrame->vStore;       } 
int         Abc_FrameReadStoreSize()                         { return Vec_PtrSize(s_GlobalFrame->vStore); }
void *      Abc_FrameReadLibLut()                            { return s_GlobalFrame->pLibLut;      } 
void *      Abc_FrameReadLibBox()                            { return s_GlobalFrame->pLibBox;      } 
void *      Abc_FrameReadLibGen()                            { return s_GlobalFrame->pLibGen;      } 
void *      Abc_FrameReadLibGen2()                           { return s_GlobalFrame->pLibGen2;     } 
void *      Abc_FrameReadLibSuper()                          { return s_GlobalFrame->pLibSuper;    } 
void *      Abc_FrameReadLibVer()                            { return s_GlobalFrame->pLibVer;      } 
void *      Abc_FrameReadManDd()                             { if ( s_GlobalFrame->dd == NULL )      s_GlobalFrame->dd = Cudd_Init( 0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );  return s_GlobalFrame->dd;      } 
void *      Abc_FrameReadManDec()                            { if ( s_GlobalFrame->pManDec == NULL ) s_GlobalFrame->pManDec = Dec_ManStart();                                        return s_GlobalFrame->pManDec; } 
char *      Abc_FrameReadFlag( char * pFlag )                { return Cmd_FlagReadByName( s_GlobalFrame, pFlag );   } 

int         Abc_FrameReadBmcFrames( Abc_Frame_t * p )        { return s_GlobalFrame->nFrames;      }               
int         Abc_FrameReadProbStatus( Abc_Frame_t * p )       { return s_GlobalFrame->Status;       }               
Abc_Cex_t * Abc_FrameReadCex( Abc_Frame_t * p )              { return s_GlobalFrame->pCex;         }        
Vec_Ptr_t * Abc_FrameReadCexVec( Abc_Frame_t * p )           { return s_GlobalFrame->vCexVec;      }        
Vec_Ptr_t * Abc_FrameReadPoEquivs( Abc_Frame_t * p )         { return s_GlobalFrame->vPoEquivs;    }        
       
int         Abc_FrameReadCexPiNum( Abc_Frame_t * p )         { return s_GlobalFrame->pCex->nPis;   }               
int         Abc_FrameReadCexRegNum( Abc_Frame_t * p )        { return s_GlobalFrame->pCex->nRegs;  }               
int         Abc_FrameReadCexPo( Abc_Frame_t * p )            { return s_GlobalFrame->pCex->iPo;    }               
int         Abc_FrameReadCexFrame( Abc_Frame_t * p )         { return s_GlobalFrame->pCex->iFrame; }               

void        Abc_FrameSetLibLut( void * pLib )                { s_GlobalFrame->pLibLut   = pLib;    } 
void        Abc_FrameSetLibBox( void * pLib )                { s_GlobalFrame->pLibBox   = pLib;    } 
void        Abc_FrameSetLibGen( void * pLib )                { s_GlobalFrame->pLibGen   = pLib;    } 
void        Abc_FrameSetLibGen2( void * pLib )               { s_GlobalFrame->pLibGen2  = pLib;    } 
void        Abc_FrameSetLibSuper( void * pLib )              { s_GlobalFrame->pLibSuper = pLib;    } 
void        Abc_FrameSetLibVer( void * pLib )                { s_GlobalFrame->pLibVer   = pLib;    } 
void        Abc_FrameSetFlag( char * pFlag, char * pValue )  { Cmd_FlagUpdateValue( s_GlobalFrame, pFlag, pValue );               } 
void        Abc_FrameSetCex( Abc_Cex_t * pCex )              { ABC_FREE( s_GlobalFrame->pCex ); s_GlobalFrame->pCex = pCex;       }
void        Abc_FrameSetNFrames( int nFrames )               { ABC_FREE( s_GlobalFrame->pCex ); s_GlobalFrame->nFrames = nFrames; }
void        Abc_FrameSetStatus( int Status )                 { ABC_FREE( s_GlobalFrame->pCex ); s_GlobalFrame->Status = Status;   }

int         Abc_FrameIsBatchMode()                           { return s_GlobalFrame ? s_GlobalFrame->fBatchMode : 0;              } 

int         Abc_FrameIsBridgeMode()                          { return s_GlobalFrame ? s_GlobalFrame->fBridgeMode : 0;             } 
void        Abc_FrameSetBridgeMode()                         { if ( s_GlobalFrame ) s_GlobalFrame->fBridgeMode = 1;               } 

/**Function*************************************************************

  Synopsis    [Returns 1 if the flag is enabled without value or with value 1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_FrameIsFlagEnabled( char * pFlag )
{
    char * pValue;
    // if flag is not defined, it is not enabled
    pValue = Abc_FrameReadFlag( pFlag );
    if ( pValue == NULL )
        return 0;
    // if flag is defined but value is not empty (no parameter) or "1", it is not enabled
    if ( strcmp(pValue, "") && strcmp(pValue, "1") )
        return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Frame_t * Abc_FrameAllocate()
{
    Abc_Frame_t * p;
    extern void define_cube_size( int n );
    extern void set_espresso_flags();
    // allocate and clean
    p = ABC_CALLOC( Abc_Frame_t, 1 );
    // get version
    p->sVersion = Abc_UtilsGetVersion( p );
    // set streams
    p->Err = stderr;
    p->Out = stdout;
    p->Hst = NULL;
    p->Status     = -1;
    p->nFrames    = -1;
    // set the starting step
    p->nSteps     =  1;
	p->fBatchMode =  0;
    // networks to be used by choice
    p->vStore = Vec_PtrAlloc( 16 );
    // initialize decomposition manager
//    define_cube_size(20);
//    set_espresso_flags();
    // initialize the trace manager
//    Abc_HManStart();
    p->vPlugInComBinPairs = Vec_PtrAlloc( 100 );
    return p;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FrameDeallocate( Abc_Frame_t * p )
{
    extern void Rwt_ManGlobalStop();
    extern void undefine_cube_size();
//    extern void Ivy_TruthManStop();
//    Abc_HManStop();
//    undefine_cube_size();
    Rwt_ManGlobalStop();
//    Ivy_TruthManStop();
    if ( p->vCexVec   )  Vec_PtrFreeFree( p->vCexVec );
    if ( p->vPoEquivs )  Vec_VecFree( (Vec_Vec_t *)p->vPoEquivs );
    if ( p->pLibVer   )  Abc_LibFree( (Abc_Lib_t *)p->pLibVer, NULL );
    if ( p->pManDec   )  Dec_ManStop( (Dec_Man_t *)p->pManDec );
    if ( p->dd        )  Extra_StopManager( p->dd );
    if ( p->vStore    )  Vec_PtrFree( p->vStore );
    if ( p->pSave1    )  Aig_ManStop( (Aig_Man_t *)p->pSave1 );
    if ( p->pSave2    )  Aig_ManStop( (Aig_Man_t *)p->pSave2 );
    if ( p->pSave3    )  Aig_ManStop( (Aig_Man_t *)p->pSave3 );
    if ( p->pSave4    )  Aig_ManStop( (Aig_Man_t *)p->pSave4 );
    if ( p->vPlugInComBinPairs ) 
    {
        char * pTemp;
        int i;
        Vec_PtrForEachEntry( char *, p->vPlugInComBinPairs, pTemp, i )
            ABC_FREE( pTemp );
        Vec_PtrFree( p->vPlugInComBinPairs );
    }
    Vec_PtrFreeP( &p->vLTLProperties_global );
    Abc_FrameDeleteAllNetworks( p );
    ABC_FREE( p->pCex2 );
    ABC_FREE( p->pCex );
    ABC_FREE( p );
    s_GlobalFrame = NULL;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FrameRestart( Abc_Frame_t * p )
{
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FrameClearVerifStatus( Abc_Frame_t * p )
{
    p->nFrames = -1;
    p->Status  = -1;
    ABC_FREE( p->pCex );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_FrameShowProgress( Abc_Frame_t * p )
{
    return Abc_FrameIsFlagEnabled( "progressbar" );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_FrameReadNtk( Abc_Frame_t * p )
{
    return p->pNtkCur;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Abc_FrameReadGia( Abc_Frame_t * p )
{
    return p->pGia;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
FILE * Abc_FrameReadOut( Abc_Frame_t * p )
{
    return p->Out;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
FILE * Abc_FrameReadErr( Abc_Frame_t * p )
{
    return p->Err;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_FrameReadMode( Abc_Frame_t * p )
{
    int fShortNames;
    char * pValue;
    pValue = Cmd_FlagReadByName( p, "namemode" );
    if ( pValue == NULL )
        fShortNames = 0;
    else 
        fShortNames = atoi(pValue);
    return fShortNames;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_FrameSetMode( Abc_Frame_t * p, int fNameMode )
{
    char Buffer[2];
    int fNameModeOld;
    fNameModeOld = Abc_FrameReadMode( p );
    Buffer[0] = '0' + fNameMode;
    Buffer[1] = 0;
    Cmd_FlagUpdateValue( p, "namemode", (char *)Buffer );
    return fNameModeOld;
}


/**Function*************************************************************

  Synopsis    [Sets the given network to be the current one.]

  Description [Takes the network and makes it the current network.
  The previous current network is attached to the given network as 
  a backup copy. In the stack of backup networks contains too many
  networks (defined by the paramater "savesteps"), the bottom
  most network is deleted.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FrameSetCurrentNetwork( Abc_Frame_t * p, Abc_Ntk_t * pNtkNew )
{
    Abc_Ntk_t * pNtk, * pNtk2, * pNtk3;
    int nNetsPresent;
    int nNetsToSave;
    char * pValue;

    if ( p->pNtkCur == pNtkNew )
        return;

    // link it to the previous network
    Abc_NtkSetBackup( pNtkNew, p->pNtkCur );
    // set the step of this network
    Abc_NtkSetStep( pNtkNew, ++p->nSteps );
    // set this network to be the current network
    p->pNtkCur = pNtkNew;

    // remove any extra network that may happen to be in the stack
    pValue = Cmd_FlagReadByName( p, "savesteps" );
    // if the value of steps to save is not set, assume 1-level undo
    if ( pValue == NULL )
        nNetsToSave = 1;
    else 
        nNetsToSave = atoi(pValue);
    
    // count the network, remember the last one, and the one before the last one
    nNetsPresent = 0;
    pNtk2 = pNtk3 = NULL;
    for ( pNtk = p->pNtkCur; pNtk; pNtk = Abc_NtkBackup(pNtk2) )
    {
        nNetsPresent++;
        pNtk3 = pNtk2;
        pNtk2 = pNtk;
    }

    // remove the earliest backup network if it is more steps away than we store
    if ( nNetsPresent - 1 > nNetsToSave )
    { // delete the last network
        Abc_NtkDelete( pNtk2 );
        // clean the pointer of the network before the last one
        Abc_NtkSetBackup( pNtk3, NULL );
    }
}

/**Function*************************************************************

  Synopsis    [This procedure swaps the current and the backup network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FrameSwapCurrentAndBackup( Abc_Frame_t * p )
{
    Abc_Ntk_t * pNtkCur, * pNtkBack, * pNtkBack2;
    int iStepCur, iStepBack;

    pNtkCur  = p->pNtkCur;
    pNtkBack = Abc_NtkBackup( pNtkCur );
    iStepCur = Abc_NtkStep  ( pNtkCur );

    // if there is no backup nothing to reset
    if ( pNtkBack == NULL )
        return;

    // remember the backup of the backup
    pNtkBack2 = Abc_NtkBackup( pNtkBack );
    iStepBack = Abc_NtkStep  ( pNtkBack );

    // set pNtkCur to be the next after the backup's backup
    Abc_NtkSetBackup( pNtkCur, pNtkBack2 );
    Abc_NtkSetStep  ( pNtkCur, iStepBack );

    // set pNtkCur to be the next after the backup
    Abc_NtkSetBackup( pNtkBack, pNtkCur );
    Abc_NtkSetStep  ( pNtkBack, iStepCur );

    // set the current network
    p->pNtkCur = pNtkBack;
}


/**Function*************************************************************

  Synopsis    [Replaces the current network by the given one.]

  Description [This procedure does not modify the stack of saved
  networks.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FrameReplaceCurrentNetwork( Abc_Frame_t * p, Abc_Ntk_t * pNtk )
{
    if ( pNtk == NULL )
        return;

    // transfer the parameters to the new network
    if ( p->pNtkCur && Abc_FrameIsFlagEnabled( "backup" ) )
    {
        Abc_NtkSetBackup( pNtk, Abc_NtkBackup(p->pNtkCur) );
        Abc_NtkSetStep( pNtk, Abc_NtkStep(p->pNtkCur) );
        // delete the current network
        Abc_NtkDelete( p->pNtkCur );
    }
    else
    {
        Abc_NtkSetBackup( pNtk, NULL );
        Abc_NtkSetStep( pNtk, ++p->nSteps );
        // delete the current network if present but backup is disabled
        if ( p->pNtkCur )
            Abc_NtkDelete( p->pNtkCur );
    }
    // set the new current network
    p->pNtkCur = pNtk;
}

/**Function*************************************************************

  Synopsis    [Removes library binding of all currently stored networks.]

  Description [This procedure is called when the library is freed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FrameUnmapAllNetworks( Abc_Frame_t * p )
{
    Abc_Ntk_t * pNtk;
    for ( pNtk = p->pNtkCur; pNtk; pNtk = Abc_NtkBackup(pNtk) )
        if ( Abc_NtkHasMapping(pNtk) )
            Abc_NtkMapToSop( pNtk );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FrameDeleteAllNetworks( Abc_Frame_t * p )
{
    Abc_Ntk_t * pNtk, * pNtk2;
    // delete all the currently saved networks
    for ( pNtk  = p->pNtkCur, 
          pNtk2 = pNtk? Abc_NtkBackup(pNtk): NULL; 
          pNtk; 
          pNtk  = pNtk2, 
          pNtk2 = pNtk? Abc_NtkBackup(pNtk): NULL )
        Abc_NtkDelete( pNtk );
    // set the current network empty
    p->pNtkCur = NULL;
//    fprintf( p->Out, "All networks have been deleted.\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FrameSetGlobalFrame( Abc_Frame_t * p )
{
	s_GlobalFrame = p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Frame_t * Abc_FrameGetGlobalFrame()
{
	if ( s_GlobalFrame == 0 )
	{
		// start the framework
		s_GlobalFrame = Abc_FrameAllocate();
		// perform initializations
		Abc_FrameInit( s_GlobalFrame );
	}
	return s_GlobalFrame;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Frame_t * Abc_FrameReadGlobalFrame()
{
	return s_GlobalFrame;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FrameSetSave1( void * pAig )
{
    Abc_Frame_t * pFrame = Abc_FrameGetGlobalFrame();
    if ( pFrame->pSave1 )
        Aig_ManStop( (Aig_Man_t *)pFrame->pSave1 );
    pFrame->pSave1 = pAig;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FrameSetSave2( void * pAig )
{
    Abc_Frame_t * pFrame = Abc_FrameGetGlobalFrame();
    if ( pFrame->pSave2 )
        Aig_ManStop( (Aig_Man_t *)pFrame->pSave2 );
    pFrame->pSave2 = pAig;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Abc_FrameReadSave1()  { void * pAig = Abc_FrameGetGlobalFrame()->pSave1; Abc_FrameGetGlobalFrame()->pSave1 = NULL; return pAig; }
void * Abc_FrameReadSave2()  { void * pAig = Abc_FrameGetGlobalFrame()->pSave2; Abc_FrameGetGlobalFrame()->pSave2 = NULL; return pAig; }

/**Function*************************************************************

  Synopsis    [Returns 0/1 if pNtkCur is an AIG and PO is 0/1; -1 otherwise.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_FrameCheckPoConst( Abc_Frame_t * p, int iPoNum )
{
    Abc_Obj_t * pObj;
    if ( p->pNtkCur == NULL )
        return -1;
    if ( !Abc_NtkIsStrash(p->pNtkCur) )
        return -1;
    if ( iPoNum < 0 || iPoNum >= Abc_NtkPoNum(p->pNtkCur) )
        return -1;
    pObj = Abc_NtkPo( p->pNtkCur, iPoNum );
    if ( !Abc_AigNodeIsConst(Abc_ObjFanin0(pObj)) )
        return -1;
    return !Abc_ObjFaninC0(pObj);
}
void Abc_FrameCheckPoConstTest( Abc_Frame_t * p )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachPo( p->pNtkCur, pObj, i )
        printf( "%d = %d\n", i, Abc_FrameCheckPoConst(p, i) );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END


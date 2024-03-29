/**CFile****************************************************************

  FileName    [abcapis.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Include this file in the external code calling ABC.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 29, 2012.]

  Revision    [$Id: abcapis.h,v 1.1.1.1 2021/09/11 09:57:46 cabodi Exp $]

***********************************************************************/
 
#ifndef MINI_AIG__abc_apis_h
#define MINI_AIG__abc_apis_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

// procedures to start and stop the ABC framework
extern void   Abc_Start();
extern void   Abc_Stop();

// procedures to get the ABC framework (pAbc) and execute commands in it
extern void * Abc_FrameGetGlobalFrame();
extern int    Cmd_CommandExecute( void * pAbc, char * pCommandLine );

// procedures to input/output 'mini AIG'
extern void   Abc_NtkInputMiniAig( void * pAbc, void * pMiniAig );
extern void * Abc_NtkOutputMiniAig( void * pAbc );

// procedures to access verifization status and a counter-example
extern int    Abc_FrameReadProbStatus( void * pAbc );   
extern void * Abc_FrameReadCex( void * pAbc );    


#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


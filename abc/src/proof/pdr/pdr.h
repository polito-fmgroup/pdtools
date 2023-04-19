/**CFile****************************************************************

  FileName    [pdr.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Property driven reachability.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 20, 2010.]

  Revision    [$Id: pdr.h,v 1.1.1.1 2021/09/11 09:57:47 cabodi Exp $]

***********************************************************************/
 
#ifndef ABC__sat__pdr__pdr_h
#define ABC__sat__pdr__pdr_h


ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Pdr_Par_t_ Pdr_Par_t;
struct Pdr_Par_t_
{
//    int iOutput;          // zero-based number of primary output to solve
    int nRecycle;         // limit on vars for recycling
    int nFrameMax;        // limit on frame count
    int nConfLimit;       // limit on SAT solver conflicts
    int nRestLimit;       // limit on the number of proof-obligations
    int nTimeOut;         // timeout in seconds
    int nTimeOutGap;      // approximate timeout in seconds since the last change
    int fTwoRounds;       // use two rounds for generalization
    int fMonoCnf;         // monolythic CNF
    int fDumpInv;         // dump inductive invariant
    int fShortest;        // forces bug traces to be shortest
    int fSkipGeneral;     // skips expensive generalization step
    int fVerbose;         // verbose output`
    int fVeryVerbose;     // very verbose output
    int fNotVerbose;      // not printing line by line progress
    int fSilent;          // totally silent execution
    int fSolveAll;        // do not stop when found a SAT output
    int nFailOuts;        // the number of failed outputs
    int iFrame;           // explored up to this frame
    int RunId;            // PDR id in this run 
    int(*pFuncStop)(int); // callback to terminate
    clock_t timeLastSolved; // the time when the last output was solved
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== pdrCore.c ==========================================================*/
extern void               Pdr_ManSetDefaultParams( Pdr_Par_t * pPars );
extern int                Pdr_ManSolve( Aig_Man_t * p, Pdr_Par_t * pPars );


ABC_NAMESPACE_HEADER_END


#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


/**CFile****************************************************************

  FileName    [scl.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Standard-cell library representation.]

  Synopsis    [Relevant command handlers.]

  Author      [Alan Mishchenko, Niklas Een]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 24, 2012.]

  Revision    [$Id: scl.c,v 1.1.1.1 2021/09/11 09:57:47 cabodi Exp $]

***********************************************************************/

#include "sclInt.h"
#include "base/main/mainInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Scl_CommandRead    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int Scl_CommandWrite   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int Scl_CommandPrint   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int Scl_CommandPrintGS ( Abc_Frame_t * pAbc, int argc, char **argv );
static int Scl_CommandStime   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int Scl_CommandTopo    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int Scl_CommandBuffer  ( Abc_Frame_t * pAbc, int argc, char **argv );
static int Scl_CommandGsize   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int Scl_CommandUpsize  ( Abc_Frame_t * pAbc, int argc, char **argv );
static int Scl_CommandMinsize ( Abc_Frame_t * pAbc, int argc, char **argv );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Scl_Init( Abc_Frame_t * pAbc )
{
    Cmd_CommandAdd( pAbc, "SCL mapping",  "read_scl",   Scl_CommandRead,     0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "write_scl",  Scl_CommandWrite,    0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "print_scl",  Scl_CommandPrint,    0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "print_gs",   Scl_CommandPrintGS,  0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "stime",      Scl_CommandStime,    0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "topo",       Scl_CommandTopo,     1 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "buffer",     Scl_CommandBuffer,   1 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "minsize",    Scl_CommandMinsize,  1 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "upsize",     Scl_CommandUpsize,   1 ); 
//    Cmd_CommandAdd( pAbc, "SCL mapping",  "gsize",      Scl_CommandGsize,    1 ); 
}
void Scl_End( Abc_Frame_t * pAbc )
{
    Abc_SclLoad( NULL, (SC_Lib **)&pAbc->pLibScl );
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandRead( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    char * pFileName;
    FILE * pFile;
    int c, fVerbose = 0;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "vh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;

    // get the input file name
    pFileName = argv[globalUtilOptind];
    if ( (pFile = fopen( pFileName, "rb" )) == NULL )
    {
        fprintf( pAbc->Err, "Cannot open input file \"%s\". \n", pFileName );
        return 1;
    }
    fclose( pFile );

    // read new library
    Abc_SclLoad( pFileName, (SC_Lib **)&pAbc->pLibScl );
    if ( fVerbose )
        Abc_SclWriteText( "scl_out.txt", (SC_Lib *)pAbc->pLibScl );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_scl [-vh] <file>\n" );
    fprintf( pAbc->Err, "\t         reads Liberty library from file\n" );
    fprintf( pAbc->Err, "\t-v     : toggle writing the result into file \"scl_out.txt\" [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\t<file> : the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandWrite( Abc_Frame_t * pAbc, int argc, char **argv )
{
    FILE * pFile;
    char * pFileName;
    int c;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    if ( pAbc->pLibScl == NULL )
    {
        fprintf( pAbc->Err, "There is no Liberty library available.\n" );
        return 1;
    }
    // get the input file name
    pFileName = argv[globalUtilOptind];
    if ( (pFile = fopen( pFileName, "wb" )) == NULL )
    {
        fprintf( pAbc->Err, "Cannot open output file \"%s\". \n", pFileName );
        return 1;
    }
    fclose( pFile );

    // save current library
    Abc_SclSave( pFileName, (SC_Lib *)pAbc->pLibScl );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_scl [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         write Liberty library into file\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\t<file> : the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandPrint( Abc_Frame_t * pAbc, int argc, char **argv )
{
    int c;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( pAbc->pLibScl == NULL )
    {
        fprintf( pAbc->Err, "There is no Liberty library available.\n" );
        return 1;
    }

    // save current library
    Abc_SclPrintCells( (SC_Lib *)pAbc->pLibScl );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: print_scl [-h]\n" );
    fprintf( pAbc->Err, "\t         prints statistics of Liberty library\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandPrintGS( Abc_Frame_t * pAbc, int argc, char **argv )
{
    int c;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( Abc_FrameReadNtk(pAbc) == NULL )
    {
        fprintf( pAbc->Err, "There is no current network.\n" );
        return 1;
    }
    if ( !Abc_NtkHasMapping(Abc_FrameReadNtk(pAbc)) )
    {
        fprintf( pAbc->Err, "The current network is not mapped.\n" );
        return 1;
    }
    if ( pAbc->pLibScl == NULL )
    {
        fprintf( pAbc->Err, "There is no Liberty library available.\n" );
        return 1;
    }

    // save current library
    Abc_SclPrintGateSizes( (SC_Lib *)pAbc->pLibScl, Abc_FrameReadNtk(pAbc) );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: print_gs [-h]\n" );
    fprintf( pAbc->Err, "\t         prints gate sizes in the current mapping\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandStime( Abc_Frame_t * pAbc, int argc, char **argv )
{
    int c;
    int fShowAll = 0;
    int fUseWireLoads = 1;
    int fShort = 0;
    int fDumpStats = 0;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "casdh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'c':
                fUseWireLoads ^= 1;
                break;
            case 'a':
                fShowAll ^= 1;
                break;
            case 's':
                fShort ^= 1;
                break;
            case 'd':
                fDumpStats ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( Abc_FrameReadNtk(pAbc) == NULL )
    {
        fprintf( pAbc->Err, "There is no current network.\n" );
        return 1;
    }
    if ( !Abc_NtkHasMapping(Abc_FrameReadNtk(pAbc)) )
    {
        fprintf( pAbc->Err, "The current network is not mapped.\n" );
        return 1;
    }
    if ( !Abc_SclCheckNtk(Abc_FrameReadNtk(pAbc), 0) )
    {
        fprintf( pAbc->Err, "The current networks is not in a topo order (run \"topo\").\n" );
        return 1;
    }
    if ( pAbc->pLibScl == NULL )
    {
        fprintf( pAbc->Err, "There is no Liberty library available.\n" );
        return 1;
    }

    Abc_SclTimePerform( (SC_Lib *)pAbc->pLibScl, Abc_FrameReadNtk(pAbc), fUseWireLoads, fShowAll, fShort, fDumpStats );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: stime [-casdh]\n" );
    fprintf( pAbc->Err, "\t         performs STA using Liberty library\n" );
    fprintf( pAbc->Err, "\t-c     : toggle using wire-loads if specified [default = %s]\n", fUseWireLoads? "yes": "no" );
    fprintf( pAbc->Err, "\t-a     : display timing information for all nodes [default = %s]\n", fShowAll? "yes": "no" );
    fprintf( pAbc->Err, "\t-s     : display timing information without critical path [default = %s]\n", fShort? "yes": "no" );
    fprintf( pAbc->Err, "\t-d     : toggle dumping statistics into a file [default = %s]\n", fDumpStats? "yes": "no" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandTopo( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    Abc_Ntk_t * pNtkRes;
    int c, fVerbose = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "vh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'v':
            fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }

    if ( pNtk == NULL )
    {
        Abc_Print( -1, "Empty network.\n" );
        return 1;
    }
    if ( !Abc_NtkIsLogic(pNtk) )
    {
        Abc_Print( -1, "This command can only be applied to a logic network.\n" );
        return 1;
    }

    // modify the current network
    pNtkRes = Abc_NtkDupDfs( pNtk );
    if ( pNtkRes == NULL )
    {
        Abc_Print( -1, "The command has failed.\n" );
        return 1;
    }
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtkRes );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: topo [-vh]\n" );
    fprintf( pAbc->Err, "\t           rearranges nodes to be in a topological order\n" );
    fprintf( pAbc->Err, "\t-v       : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : print the command usage\n");
    return 1;
} 

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandBuffer( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    Abc_Ntk_t * pNtkRes;
    int Degree;
    int c, fVerbose;
    Degree   = 4;
    fVerbose = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "Nvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'N':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-N\" should be followed by a positive integer.\n" );
                goto usage;
            }
            Degree = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( Degree < 0 ) 
                goto usage;
            break;
        case 'v':
            fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }

    if ( pNtk == NULL )
    {
        Abc_Print( -1, "Empty network.\n" );
        return 1;
    }
    if ( !Abc_NtkIsLogic(pNtk) )
    {
        Abc_Print( -1, "This command can only be applied to a logic network.\n" );
        return 1;
    }

    // modify the current network
    pNtkRes = Abc_SclPerformBuffering( pNtk, Degree, fVerbose );
    if ( pNtkRes == NULL )
    {
        Abc_Print( -1, "The command has failed.\n" );
        return 1;
    }
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtkRes );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: buffer [-N num] [-vh]\n" );
    fprintf( pAbc->Err, "\t           performs buffering of the mapped network\n" );
    fprintf( pAbc->Err, "\t-N <num> : the max allowed fanout count of node/buffer [default = %d]\n", Degree );
    fprintf( pAbc->Err, "\t-v       : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : print the command usage\n");
    return 1;
} 

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandMinsize( Abc_Frame_t * pAbc, int argc, char **argv )
{
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    int c, fVerbose = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "vh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'v':
            fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }

    if ( Abc_FrameReadNtk(pAbc) == NULL )
    {
        fprintf( pAbc->Err, "There is no current network.\n" );
        return 1;
    }
    if ( !Abc_NtkHasMapping(Abc_FrameReadNtk(pAbc)) )
    {
        fprintf( pAbc->Err, "The current network is not mapped.\n" );
        return 1;
    }
    if ( !Abc_SclCheckNtk(Abc_FrameReadNtk(pAbc), 0) )
    {
        fprintf( pAbc->Err, "The current networks is not in a topo order (run \"topo\").\n" );
        return 1;
    }
    if ( pAbc->pLibScl == NULL )
    {
        fprintf( pAbc->Err, "There is no Liberty library available.\n" );
        return 1;
    }

    Abc_SclMinsizePerform( (SC_Lib *)pAbc->pLibScl, pNtk, fVerbose );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: minsize [-vh]\n" );
    fprintf( pAbc->Err, "\t           downsized all gates to their minimum size\n" );
    fprintf( pAbc->Err, "\t-v       : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandGsize( Abc_Frame_t * pAbc, int argc, char **argv )
{
    SC_SizePars Pars, * pPars = &Pars;
    int c;
    memset( pPars, 0, sizeof(SC_SizePars) );
    pPars->nSteps        = 1000000;
    pPars->nRange        = 0;
    pPars->nRangeF       = 10;
    pPars->nTimeOut      = 300;
    pPars->fTryAll       = 1;
    pPars->fUseWireLoads = 1;
    pPars->fPrintCP      = 0;
    pPars->fVerbose      = 0;
    pPars->fVeryVerbose  = 0;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "NWUTacpvwh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'N':
                if ( globalUtilOptind >= argc )
                {
                    Abc_Print( -1, "Command line switch \"-N\" should be followed by a positive integer.\n" );
                    goto usage;
                }
                pPars->nSteps = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( pPars->nSteps <= 0 ) 
                    goto usage;
                break;
            case 'W':
                if ( globalUtilOptind >= argc )
                {
                    Abc_Print( -1, "Command line switch \"-W\" should be followed by a positive integer.\n" );
                    goto usage;
                }
                pPars->nRange = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( pPars->nRange < 0 ) 
                    goto usage;
                break;
            case 'U':
                if ( globalUtilOptind >= argc )
                {
                    Abc_Print( -1, "Command line switch \"-U\" should be followed by a positive integer.\n" );
                    goto usage;
                }
                pPars->nRangeF = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( pPars->nRangeF < 0 ) 
                    goto usage;
                break;
            case 'T':
                if ( globalUtilOptind >= argc )
                {
                    Abc_Print( -1, "Command line switch \"-T\" should be followed by a positive integer.\n" );
                    goto usage;
                }
                pPars->nTimeOut = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( pPars->nTimeOut < 0 ) 
                    goto usage;
                break;
            case 'a':
                pPars->fTryAll ^= 1;
                break;
            case 'c':
                pPars->fUseWireLoads ^= 1;
                break;
            case 'p':
                pPars->fPrintCP ^= 1;
                break;
            case 'v':
                pPars->fVerbose ^= 1;
                break;
            case 'w':
                pPars->fVeryVerbose ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( Abc_FrameReadNtk(pAbc) == NULL )
    {
        fprintf( pAbc->Err, "There is no current network.\n" );
        return 1;
    }
    if ( !Abc_NtkHasMapping(Abc_FrameReadNtk(pAbc)) )
    {
        fprintf( pAbc->Err, "The current network is not mapped.\n" );
        return 1;
    }
    if ( !Abc_SclCheckNtk(Abc_FrameReadNtk(pAbc), 0) )
    {
        fprintf( pAbc->Err, "The current networks is not in a topo order (run \"topo\").\n" );
        return 1;
    }
    if ( pAbc->pLibScl == NULL )
    {
        fprintf( pAbc->Err, "There is no Liberty library available.\n" );
        return 1;
    }

//    Abc_SclSizingPerform( (SC_Lib *)pAbc->pLibScl, Abc_FrameReadNtk(pAbc), pPars );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: gsize [-NWUT num] [-acpvwh]\n" );
    fprintf( pAbc->Err, "\t           performs gate sizing using Liberty library\n" );
    fprintf( pAbc->Err, "\t-N <num> : the number of gate-sizing steps performed [default = %d]\n", pPars->nSteps );
    fprintf( pAbc->Err, "\t-W <num> : delay window (in percent) of near-critical COs [default = %d]\n", pPars->nRange );
    fprintf( pAbc->Err, "\t-U <num> : delay window (in percent) of near-critical fanins [default = %d]\n", pPars->nRangeF );
    fprintf( pAbc->Err, "\t-T <num> : an approximate timeout, in seconds [default = %d]\n", pPars->nTimeOut );
    fprintf( pAbc->Err, "\t-a       : try resizing all gates (not only critical) [default = %s]\n", pPars->fTryAll? "yes": "no" );
    fprintf( pAbc->Err, "\t-c       : toggle using wire-loads if specified [default = %s]\n", pPars->fUseWireLoads? "yes": "no" );
    fprintf( pAbc->Err, "\t-p       : toggle printing critical path before and after sizing [default = %s]\n", pPars->fPrintCP? "yes": "no" );
    fprintf( pAbc->Err, "\t-v       : toggle printing verbose information [default = %s]\n", pPars->fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-w       : toggle printing even more information [default = %s]\n", pPars->fVeryVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : print the help massage\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandUpsize( Abc_Frame_t * pAbc, int argc, char **argv )
{
    SC_UpSizePars Pars, * pPars = &Pars;
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    int c;
    memset( pPars, 0, sizeof(SC_UpSizePars) );
    pPars->nIters        = 1000;
    pPars->nIterNoChange =   50;
    pPars->Window        =    2;
    pPars->Ratio         =   10;
    pPars->Notches       =   20;
    pPars->TimeOut       =    0;
    pPars->fUseDept      =    1;
    pPars->fDumpStats    =    0;
    pPars->fVerbose      =    0;
    pPars->fVeryVerbose  =    0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "IJWRNTsdvwh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'I':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-I\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->nIters = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nIters < 0 ) 
                goto usage;
            break;
        case 'J':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-J\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->nIterNoChange = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nIterNoChange < 0 ) 
                goto usage;
            break;
        case 'W':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-W\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->Window = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->Window < 0 ) 
                goto usage;
            break;
        case 'R':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-R\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->Ratio = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->Ratio < 0 ) 
                goto usage;
            break;
        case 'N':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-N\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->Notches = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->Notches < 0 ) 
                goto usage;
            break;
        case 'T':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-T\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->TimeOut = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->TimeOut < 0 ) 
                goto usage;
            break;
        case 's':
            pPars->fUseDept ^= 1;
            break;
        case 'd':
            pPars->fDumpStats ^= 1;
            break;
        case 'v':
            pPars->fVerbose ^= 1;
            break;
        case 'w':
            pPars->fVeryVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }

    if ( Abc_FrameReadNtk(pAbc) == NULL )
    {
        fprintf( pAbc->Err, "There is no current network.\n" );
        return 1;
    }
    if ( !Abc_NtkHasMapping(Abc_FrameReadNtk(pAbc)) )
    {
        fprintf( pAbc->Err, "The current network is not mapped.\n" );
        return 1;
    }
    if ( !Abc_SclCheckNtk(Abc_FrameReadNtk(pAbc), 0) )
    {
        fprintf( pAbc->Err, "The current networks is not in a topo order (run \"topo\").\n" );
        return 1;
    }
    if ( pAbc->pLibScl == NULL )
    {
        fprintf( pAbc->Err, "There is no Liberty library available.\n" );
        return 1;
    }

    Abc_SclUpsizePerform( (SC_Lib *)pAbc->pLibScl, pNtk, pPars );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: upsize [-IJWRNT num] [-dvwh]\n" );
    fprintf( pAbc->Err, "\t           selectively increases gate sizes on the critical path\n" );
    fprintf( pAbc->Err, "\t-I <num> : the number of upsizing iterations to perform [default = %d]\n", pPars->nIters );
    fprintf( pAbc->Err, "\t-J <num> : the number of iterations without improvement to stop [default = %d]\n", pPars->nIterNoChange );
    fprintf( pAbc->Err, "\t-W <num> : delay window (in percent) of near-critical COs [default = %d]\n", pPars->Window );
    fprintf( pAbc->Err, "\t-R <num> : ratio of critical nodes (in percent) to update [default = %d]\n", pPars->Ratio );
    fprintf( pAbc->Err, "\t-N <num> : limit on discrete upsizing steps at a node [default = %d]\n", pPars->Notches );
    fprintf( pAbc->Err, "\t-T <num> : approximate timeout in seconds [default = %d]\n", pPars->TimeOut );
    fprintf( pAbc->Err, "\t-s       : toggle using slack based on departure times [default = %s]\n", pPars->fUseDept? "yes": "no" );
    fprintf( pAbc->Err, "\t-d       : toggle dumping statistics into a file [default = %s]\n", pPars->fDumpStats? "yes": "no" );
    fprintf( pAbc->Err, "\t-v       : toggle printing verbose information [default = %s]\n", pPars->fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-w       : toggle printing more verbose information [default = %s]\n", pPars->fVeryVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : print the command usage\n");
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END


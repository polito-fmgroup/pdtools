/**CFile****************************************************************

  FileName    [fbvAbcLink.c]

  PackageName [fbv]

  Synopsis    [Aig based model check routines]

  Description []

  SeeAlso     []

  Author      [Stefano Quer]

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

#include "fbvInt.h"
#include "ddiInt.h"
#include "trInt.h"
#include "trav.h"
#include "expr.h"
#include "st.h"
#include <time.h>

EXTERN void Abc_Start(
);
EXTERN void Abc_Stop(
);
EXTERN void *Abc_FrameGetGlobalFrame(
);
EXTERN int Cmd_CommandExecute(
  void *pAbc,
  char *sCommand
);


/**Function*************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/

int
fbvAbcLink(
  char *fileNameR,
  char *fileNameW,
  int initialOpt,
  int optLevel,
  int seqOpt
)
{
  void *pAbc;
  char command[PDTUTIL_MAX_STR_LEN];
  int timeInit, timeFinal;
  int i;

  /*                          */
  /*  Start the ABC framework */
  /*                          */

  if (!initialOpt) {
    fprintf(stdout, "\nABC Running ...\n");
  } else {
    fprintf(stdout, "\n*** Optimizer Running ***\n");
  }
  Abc_Start();
  pAbc = Abc_FrameGetGlobalFrame();

  /*                 */
  /*  Issue Commands */
  /*                 */

  timeInit = clock();

  /* Build Command List */
  sprintf(command, "read %s", fileNameR);
  if (!initialOpt) {
    fprintf(stdout, "Issuing command: %s\n", command);
  }

  if (Cmd_CommandExecute(pAbc, command)) {
    fprintf(stdout, "Cannot execute command \"%s\".\n", command);
    return (0);
  }

  if (seqOpt) {
    sprintf(command, "%s; scleanup -l", command);
    sprintf(command, "%s; balance", command);
    sprintf(command, "%s; ssweep", command);
    sprintf(command, "%s; balance", command);
  }

  for (i = 0; i < optLevel; i++) {
    sprintf(command, "print_stats");
    sprintf(command, "%s; balance", command);
    sprintf(command, "%s; strash", command);

    sprintf(command, "%s; rewrite ", command);
    sprintf(command, "%s; balance", command);
    sprintf(command, "%s; refactor ", command);
    sprintf(command, "%s; balance", command);
    sprintf(command, "%s; resub ", command);
    sprintf(command, "%s; balance", command);
    sprintf(command, "%s; csweep ", command);
    sprintf(command, "%s; balance", command);
    sprintf(command, "%s; fraig ", command);
    sprintf(command, "%s; balance", command);
    //      sprintf (command, "%s; dcompress ", command);
    sprintf(command, "%s; balance", command);

    if (!initialOpt) {
      fprintf(stdout, "Issuing command: %s\n", command);
    }

    if (Cmd_CommandExecute(pAbc, command)) {
      fprintf(stdout, "Cannot execute command \"%s\".\n", command);
      return (0);
    }
  }

  // last command
  sprintf(command, "print_stats");
  if (!initialOpt) {
    sprintf(command, "%s; cec %s %s", command, fileNameR, fileNameW);
    fprintf(stdout, "Issuing command: %s\n", command);
  }

  if (Cmd_CommandExecute(pAbc, command)) {
    fprintf(stdout, "Cannot execute command \"%s\".\n", command);
    return (0);
  }

/*     // sequential sweep */
/*     sprintf (command, "ssweep ; balance ; fraig ; balance"); */

/*     if ( Cmd_CommandExecute (pAbc, command) ) { */
/*       fprintf( stdout, "Cannot execute command \"%s\".\n", command ); */
/*       return (0); */
/*     } */

  // write blif command
  sprintf(command, "write_blif %s", fileNameW);

  if (Cmd_CommandExecute(pAbc, command)) {
    fprintf(stdout, "Cannot execute command \"%s\".\n", command);
    return (0);
  }

  timeFinal = clock() - timeInit;
  if (!initialOpt) {
    fprintf(stdout, "ABC Total Time = %6.2f sec\n",
      (float)(timeFinal) / (float)(CLOCKS_PER_SEC));
  }

  /*                         */
  /*  Stop the ABC framework */
  /*                         */

  Abc_Stop();

  if (!initialOpt) {
    fprintf(stdout, "Stopping ABC ...\n");
  } else {
    fprintf(stdout, "*** Optimizer Stopped ***\n");
  }

  return (1);
}

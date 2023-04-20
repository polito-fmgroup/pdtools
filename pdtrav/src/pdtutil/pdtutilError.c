/**CFile***********************************************************************

  FileName    [pdtutilError.c]

  PackageName [pdtutil]

  Synopsis    [Utility Functions for Error handling]

  Description [This package contains general utility functions for the
    pdtrav package for error handling (chatch mechanism).]

  SeeAlso     []

  Author      [Gianpiero Cabodi and Stefano Quer]

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

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include <signal.h>
#include "pdtutil.h"

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

/**Variable********************************************************************

  Synopsis    [Stack context for non-local goto]

  Description [This variable is used to save the stack environment for
    later use.]

  SideEffects []

  SeeAlso     [util_newlongjmp util_longjmp]

******************************************************************************/

#define MAXJMP 20
static JMPBUF jmp_buf_arr[MAXJMP];
static int jmp_buf_pos = 0;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Save stack context for non-local goto]

  Description [Saves the stack environment in the global array
    <code>jmp_buf_arr</code> for later use by <code>util_longjmp</code>.
    ]

  SideEffects []

  SeeAlso     [util_longjmp util_cancellongjmp]

******************************************************************************/

JMPBUF *
util_newlongjmp(
  void
)
{
  Pdtutil_Assert(jmp_buf_pos < MAXJMP, "In util_newlongjmp");
  return (&(jmp_buf_arr[jmp_buf_pos++]));
}

/**Function********************************************************************

  Synopsis    [Restore the environment saved in <code>jmp_buf</code>.]

  Description [Restores the environment saved by the last call of
    <code>SETJMP(*(util_newlongjmp()), 1)</code>.
    After <code>util_longjmp()</code> is completed, program execution
    continues as if the corresponding call of <code>SETJMP()</code>
    had just returned a value different from <code>0</code> (zero).
    ]

  SideEffects []

  SeeAlso     [util_newlongjmp util_cancellongjmp]

******************************************************************************/

void
util_longjmp(
  void
)
{
  if (jmp_buf_pos > 0) {
    LONGJMP(jmp_buf_arr[--jmp_buf_pos], 2);
  }

  return;
}

/**Function********************************************************************

  Synopsis    [Pop one of the environments saved in <code>jmp_buf</code>.]

  Description [Removes the last envirnoment saved in
    <code>jmp_buf</code> by a <code>SETJMP(*(util_newlongjmp()), 1)</code>
    call.
    ]

  SideEffects []

  SeeAlso     [util_newlongjmp util_longjmp]

******************************************************************************/

void
util_cancellongjmp(
  void
)
{
  assert(jmp_buf_pos > 0);
  jmp_buf_pos--;

  return;
}

/**Function********************************************************************

  Synopsis    [Reset environment saved in <code>jmp_buf</code>.]

  Description [Resets the environment saved by the calls to
    <code>SETJMP(*(util_newlongjmp()), 1)<code>. After
    this call, all the longjump points previously stored are
    cancelled.
    ]

  SideEffects []

  SeeAlso     [util_newlongjmp]

******************************************************************************/

void
util_resetlongjmp(
  void
)
{
  jmp_buf_pos = 0;

  return;
}

/**Function********************************************************************

  Synopsis    [General exit routine.]

  Description [If non local goto are anebaled, instead of exiting from the
    program, then the non local goto is executed.
    ]

  SideEffects []

  SeeAlso     [util_setjmp util_longjmp]

******************************************************************************/

void
Pdtutil_CatchExit(
  int n
)
{
  util_longjmp();

  /*
   *  If Exit and NO CATCH exists above the exit ... do this ...
   *  ... Print Error Message ...
   */

  switch (n) {
    case ERROR_CODE_NO_MES:
      break;
    case ERROR_CODE_MEM_LIMIT:
      fprintf(stderr, "Memory Limit Reached.\n");
      break;
    case ERROR_CODE_TIME_LIMIT:
      fprintf(stderr, "Time Limit Reached.\n");
      break;
    case ERROR_CODE_ASSERT:
      fprintf(stderr, "Assertion Failed.\n");
      break;
    case SIGFPE:
      fprintf(stderr, "SIGFPE catched.\n");
      break;
    case SIGABRT:
      fprintf(stderr, "SIGABRT catched.\n");
      break;
    case SIGHUP:
      fprintf(stderr, "SIGHUP catched.\n");
      break;
    case SIGKILL:
      fprintf(stderr, "SIGKILL catched.\n");
      break;
    default:
      fprintf(stderr, "Generic Error.\n");
      break;
  }

  exit(n);
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

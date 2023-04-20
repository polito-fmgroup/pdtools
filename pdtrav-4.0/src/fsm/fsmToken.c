/**CFile***********************************************************************

  FileName    [fsmToken.c]

  PackageName [fsm]

  Synopsis    [Functions to convert tokens (keywords) between string and
    integer (enum) format.
    ]

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

#include "fsmInt.h"

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

static struct {
  Fsm_TokenType index;
  char *name;
} TokenTable[] = {
  {
  KeyFSM_FSM, "Fsm"}, {
  KeyFSM_SIZE, "Size"}, {
  KeyFSM_I, "i"}, {
  KeyFSM_O, "o"}, {
  KeyFSM_L, "l"}, {
  KeyFSM_AUXVAR, "auxVar"}, {
  KeyFSM_ORD, "Ord"}, {
  KeyFSM_FILE_ORD, "ordFile"}, {
  KeyFSM_NAME, "Name"}, {
  KeyFSM_PS, "ps"}, {
  KeyFSM_NS, "ns"}, {
  KeyFSM_INDEX, "Index"}, {
  KeyFSM_DELTA, "Delta"}, {
  KeyFSM_FILE_BDD, "bddFile"}, {
  KeyFSM_FILE_TEXT, "bdd"}, {
  KeyFSM_STRING, "string"}, {
  KeyFSM_LAMBDA, "Lambda"}, {
  KeyFSM_INITSTATE, "InitState"}, {
  KeyFSM_TRANS_REL, "Tr"}, {
  KeyFSM_FROM, "From"}, {
  KeyFSM_REACHED, "Reached"}, {
  KeyFSM_AUXFUN, "AuxFun"},
    /*
     *  The End Key (or whatever) has to be in the last position as all
     *  the 'key' are terminated by 'KeyEnd'
     */
  {
  KeyFSM_END, "End"}
};

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

#define FsmTokenNum (sizeof(TokenTable)/sizeof(TokenTable[0]))

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/


/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Converts a token from string to enum]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

Fsm_TokenType
FsmString2Token(
  char *keyword                 /* String representing the keyword */
)
{
  int i, n;

  /*
   *  A token is a keyword if first char is '.'
   *  otherwise it is considered a simple string.
   */

  if (keyword[0] != '.') {
    return (KeyFSM_STRING);
  }

  /*
   *  Comparison to get the End key
   */

  n = strlen(TokenTable[FsmTokenNum - 1].name);
  if (strncmp(keyword + 1, TokenTable[FsmTokenNum - 1].name, n) == 0) {
    return (KeyFSM_END);
  }

  /*
   *  Other keys
   */

  for (i = 0; i < FsmTokenNum; i++) {
    if (strcmp(keyword + 1, TokenTable[i].name) == 0) {
      return (TokenTable[i].index);
    }
  }

  return (KeyFSM_UNKNOWN);
}

/**Function********************************************************************

  Synopsis    [Converts a token from enum to string]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

char *
FsmToken2String(
  Fsm_TokenType token           /* token in enum (integer) format */
)
{
  int i;

  for (i = 0; i < FsmTokenNum; i++) {
    if (token == TokenTable[i].index)
      return (TokenTable[i].name);
  }

  return (NULL);
}

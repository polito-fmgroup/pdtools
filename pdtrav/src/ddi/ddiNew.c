/**CFile***********************************************************************

  FileName    [ddiNew.c]

  PackageName [ddi]

  Synopsis    [Utility Functions]

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

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include "ddiInt.h"

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


/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis           []

  Description        []

  SideEffects        []

  SeeAlso            []

******************************************************************************/

int
Ddi_BddOperation (
  Ddi_Mgr_t *defaultDdMgr         /* Default DD Manager */,
  Ddi_Bdd_t **bddP                 /* BDD Pointer to manipulate */,
  char *string                    /* String */,
  Pdtutil_MgrOp_t operationFlag    /* Operation Flag */,
  void **voidPointer              /* Generic Pointer */,
  Pdtutil_MgrRet_t *returnFlag     /* Type of the Pointer Returned */
  )
{
  char *stringFirst, *stringSecond;
  Ddi_Mgr_t *tmpDdMgr;

  /*----------------------- Take Main and Secondary Name --------------------*/

  Pdtutil_ReadSubstring (string, &stringFirst, &stringSecond);

  /*------------------------ Operation on the BDD Manager -------------------*/

  if (stringFirst == NULL) {
    switch (operationFlag) {
      case Pdtutil_MgrOpRead_c:
      case Pdtutil_MgrOpBddRead_c:
        *voidPointer = (void *) *bddP;
        *returnFlag = Pdtutil_MgrRetBdd_c;
        break;

      case Pdtutil_MgrOpDelete_c:
      case Pdtutil_MgrOpBddDelete_c:
        if (*bddP != NULL) {
          Ddi_Free (*bddP);
        }
        *bddP = NULL;
        break;

      case Pdtutil_MgrOpSet_c:
      case Pdtutil_MgrOpBddSet_c:
        if (*bddP != NULL) {
          Ddi_Free (*bddP);
        }
        *bddP = Ddi_BddCopy (defaultDdMgr, (Ddi_Bdd_t *) *voidPointer);
        break;

     case Pdtutil_MgrOpStats_c:
        Ddi_BddPrintStats ((Ddi_Bdd_t *) *bddP,stdout);
        break;

      case Pdtutil_MgrOpMgrRead_c:
        tmpDdMgr = Ddi_ReadMgr (*bddP);
        *voidPointer = (void *) tmpDdMgr;
        *returnFlag = Pdtutil_MgrRetDdMgr_c;
        break;

      case Pdtutil_MgrOpMgrSet_c:
        tmpDdMgr = (Ddi_Mgr_t *) *voidPointer;
        break;

      default:
        Pdtutil_Warning (1, "Operation Non Allowed on BDD Manager");
        break;
    }

    Pdtutil_Free (stringFirst);
    Pdtutil_Free (stringSecond);
    return (0);
  }

  /*------------------------ Operation on the BDD Manager -------------------*/

  if (strcmp (stringFirst, "ddm") == 0) {
    switch (operationFlag) {
      case Pdtutil_MgrOpMgrRead_c:
        tmpDdMgr = Ddi_ReadMgr (*bddP);
        *voidPointer = (void *) tmpDdMgr;
        *returnFlag = Pdtutil_MgrRetDdMgr_c;
        break;

      case Pdtutil_MgrOpMgrSet_c:
        tmpDdMgr = (Ddi_Mgr_t *) *voidPointer;
        break;

      default:
        Pdtutil_Warning (1, "Operation Non Allowed on BDD Manager");
        break;
    }

    Pdtutil_Free (stringFirst);
    Pdtutil_Free (stringSecond);
    return (0);
  }

  /*------------------------------- No Match --------------------------------*/

  Pdtutil_Warning (1, "Operation on BDD not allowed");
  return (1);
}

/**Function********************************************************************

  Synopsis           []

  Description        []

  SideEffects        []

  SeeAlso            []

******************************************************************************/

int
Ddi_BddarrayOperation (
  Ddi_Mgr_t *defaultDdMgr         /* Default BDD Manager */,
  Ddi_Bddarray_t **bddArrayP       /* BDD Array Pointer to manipulate */,
  char *string                    /* String */,
  Pdtutil_MgrOp_t operationFlag    /* Operation Flag */,
  void **voidPointer              /* Generic Pointer */,
  Pdtutil_MgrRet_t *returnFlag     /* Type of the Pointer Returned */
  )
{
  int i;
  char *stringFirst, *stringSecond;
  Ddi_Bdd_t *bdd;

  /*----------------------- Take Main and Secondary Name --------------------*/

  Pdtutil_ReadSubstring (string, &stringFirst, &stringSecond);

  /*------------------------ Operation on the BDD Manager -------------------*/

  if (stringFirst == NULL) {
    switch (operationFlag) {
      case Pdtutil_MgrOpDelete_c:
      case Pdtutil_MgrOpArrayDelete_c:
        if (*bddArrayP != NULL) {
          Ddi_Free (*bddArrayP);
        }
        *bddArrayP = NULL;
        break;

      case Pdtutil_MgrOpRead_c:
      case Pdtutil_MgrOpArrayRead_c:
        *voidPointer = (void *) *bddArrayP;
        *returnFlag = Pdtutil_MgrRetBddarray_c;
        break;

      case Pdtutil_MgrOpSet_c:
      case Pdtutil_MgrOpArraySet_c:
        if (*bddArrayP != NULL) {
          Ddi_Free (*bddArrayP);
        }
        *bddArrayP = Ddi_BddarrayCopy (defaultDdMgr,
        (Ddi_Bddarray_t *) *voidPointer);
        break;

      case Pdtutil_MgrOpStats_c:
        fprintf(stderr, "Error: Operation Not Implemented in ddiNew.c.\n");
      /*
        Ddi_BddarrayPrintStats ((Ddi_Bddarray_t *) *bddArrayP,stdout);
      */
        break;

      default:
        Pdtutil_Warning (1, "Operation Non Allowed on BDD Manager");
        break;
    }

    Pdtutil_Free (stringFirst);
    Pdtutil_Free (stringSecond);
    return (0);
  }

  /*-------------------------- Operation on one Element ---------------------*/

  if (stringSecond == NULL) {
    i = atoi (stringFirst);
    if ( (i>=0) && (i<Ddi_BddarrayNum (*bddArrayP)) ) {
      bdd = Ddi_BddarrayRead (*bddArrayP, i);
      Ddi_BddOperation (defaultDdMgr, &bdd, stringSecond,
        operationFlag, voidPointer, returnFlag);
      return (0);
    } else {
      Pdtutil_Warning (1,
        "Operation on BDD not allowed or Array index out of bounds");
      *returnFlag = Pdtutil_MgrRetNone_c;
      return (1);
    }
  }

  /*------------------------------- No Match --------------------------------*/

  Pdtutil_Warning (1, "Operation on BDD not allowed");
  return (1);
}




/**Function********************************************************************

 Synopsis    [Reads a cube from stdin]

 Description [The user can make a cube typing the index of variables]

 SideEffects [none]

 SeeAlso     []

******************************************************************************/

Ddi_Bdd_t *
Ddi_BddLoadCube(
  Ddi_Mgr_t *dd,
  FILE *fp,
  int idOrName
  )
{
  Ddi_Bdd_t *cube,*V,*V2;
  int n, n2, isCompl, op2;
  char buf[100], *name, name1[100], name2[100];

  op2 = 0;
  cube = Ddi_BddDup(Ddi_MgrReadOne (dd));
  while (fgets(buf,100,fp)!=NULL) {
    if (buf[0] == '#') continue; /* comment */
    if (buf[0]=='!') {
      isCompl = 1;
      name = &buf[1];
    }
    else {
      isCompl = 0;
      name = buf;
    }
    /* load from file */
    if (name[0]=='$') {
      name++;
      sscanf(name, "%s", name1); /* removes \n */
      Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
        Pdtutil_VerbLevelNone_c,
        fprintf(stdout, "Constraints Loading from File %s.\n", name1)
      );
      V = Ddi_BddLoad(dd,DDDMP_VAR_MATCHNAMES,
          DDDMP_MODE_DEFAULT,name1,NULL);
      if (isCompl)
        Ddi_BddNotAcc(V);
    }
    else {
      if ((name[0]=='=')||(name[0]=='&')||(name[0]=='|')) {
        op2 = name[0];
        name++;
      }
      if (idOrName) {
        if (op2)
          sscanf(name,"%d %d",&n, &n2);
        else
          sscanf(name,"%d",&n);
      }
      else {
        while(name[0]==' ') name++;
        sscanf(name,"%s",name1);
      if (!Ddi_VarFromName(dd,name1))
         continue; /* SeN modified: ignore undefined variables */
        n = Ddi_VarCuddIndex(Ddi_VarFromName(dd,name1));
        if (op2) {
          sscanf(name,"%*s %s",name2);
          n2 = Ddi_VarCuddIndex(Ddi_VarFromName(dd,name2));
        }
        /*§§§ occorre restituire NULL ?*/
        if (n<0)
          fprintf(stderr, "Error: Variable Name not Found for Cube: %s.\n",
            name);
      }
      if (n <= 0)
        break;
      V = Ddi_BddMakeLiteral(Ddi_VarFromIndex (dd,n),1);
      if (op2) {
        V2 = Ddi_BddMakeLiteral(Ddi_VarFromIndex (dd,n2),1);
        switch (op2) {
          case '&':
            Ddi_BddAndAcc(V,V2);
            break;
          case '|':
            Ddi_BddOrAcc(V,V2);
            break;
          case '=':
            Ddi_BddXnorAcc(V,V2);
            break;
          default:
            fprintf(stderr, "Error: Unknown Operator in Cube File %c.\n", op2);
        }
        Ddi_Free(V2);
      }
    }
    if (isCompl)
      Ddi_BddNotAcc(V);

    Ddi_BddAndAcc(cube,V);
    Ddi_Free(V);
  }

  return cube;
}




/**Function********************************************************************
 Synopsis    [Reads a variable set from file]
 Description [The user can make a cube typing the index of variables]
 SideEffects [none]
 SeeAlso     []
******************************************************************************/

Ddi_Varset_t *
Ddi_VarsetLoad(
  Ddi_Mgr_t *dd,
  FILE *fp,
  int idOrName
)
{
  Ddi_Varset_t *vars;
  int n, op2;
  char buf[PDTUTIL_MAX_STR_LEN];

  op2 = 0;
  vars = Ddi_VarsetVoid (dd);
  while (fscanf(fp,"%s",buf)>0) {
    if (idOrName)
      sscanf(buf,"%d",&n);
    else
      n = Ddi_VarCuddIndex(Ddi_VarFromName(dd,buf));
    vars = Ddi_VarsetEvalFree(
             Ddi_VarsetAdd (vars, Ddi_VarFromIndex (dd,n)),
           vars);
  }
  return vars;
}


/**Function********************************************************************
 Synopsis    [The function does a substitution of variables of a BDD]
 Description [The function returns the pointer to a new function with the
              variables swapped (x replace y), or NULL otherwise.]
 SideEffects [none]
 SeeAlso     []
******************************************************************************/
Ddi_Bdd_t *
Ddi_VarSubst(
  Ddi_Bdd_t *f         /* BDD */,
  Ddi_Vararray_t *x   /* first array of variables */,
  Ddi_Vararray_t *y   /* second array of variables */
)
{
#if 1
  Pdtutil_Warning (1, "Not implemented Pdtutil_VarSubst.");
  return (NULL);
#else
  Ddi_Bdd_t *f0,*f1,*xi,*yi, *newf;
  Ddi_Varset_t *supp,*supp1;
  int i;

  newf = Ddi_BddDup(f);

  supp = Ddi_BddSupp(newf);

  for (i=0; i<Ddi_VararrayNum(x); i++) {
    xi = Ddi_VararrayRead(x,i);
    supp1 = Ddi_BddConstrain(supp,xi);
    if (Ddi_Varsetequal(supp1,supp)) {
      Ddi_Free(supp1);
      continue;
    }
    Ddi_Free(supp1);

    yi = Ddi_VararrayRead(y,i);
    f0 = Ddi_BddConstrain(newf,Ddi_Not(xi));
    f1 = Ddi_BddConstrain(newf,xi);
    Ddi_Free(newf);
    newf = Ddi_BddIte(yi,f1,f0);
    Ddi_Free(f0);
    Ddi_Free(f1);
  }

  Ddi_Free(supp);

  return newf;
#endif
}


/**Function********************************************************************
  Synopsis    [Prints the indices of a Vararray]
  SideEffects [None]
  SeeAlso     [Ddi_BddarrayPrint]
******************************************************************************/

void
Ddi_PrintVararray (
  Ddi_Vararray_t *array )
{
  int i;

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    for (i=0;i<Ddi_VararrayNum(array);i++) {
      fprintf(stdout, "[%d] id: %d - name: %s\n",i,
        Ddi_VarIndex(Ddi_VararrayRead(array,i)),
        Ddi_VarName(Ddi_VararrayRead(array,i)));
    }
    fprintf(stdout, "\n")
  );
}

/**Function********************************************************************
  Synopsis    [Prints the size of each function in a DdArray]
  SideEffects [None]
  SeeAlso     [Ddi_PrintVararray]
******************************************************************************/
void
Ddi_BddarrayPrint (
  Ddi_Bddarray_t *array )
{
  int i;
  Ddi_Bdd_t *F;

  Pdtutil_VerbosityLocal(Pdtutil_VerbLevelNone_c,
    Pdtutil_VerbLevelNone_c,
    for (i=0;i<Ddi_BddarrayNum(array);i++) {
      F = Ddi_BddarrayRead(array,i);
      fprintf(stdout, "%d ", Ddi_BddSize (F));
    }
    fprintf(stdout, "\n")
  );
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/



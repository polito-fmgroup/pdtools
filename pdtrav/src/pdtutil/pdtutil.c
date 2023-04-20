/**CFile***********************************************************************

  FileName    [pdtutil.c]

  PackageName [pdtutil]

  Synopsis    [Utility Functions]

  Description [This package contains general utility functions for the
    pdtrav package.]

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

// put here to avoid any redefinition in pdtutil.h
#include <stdio.h>
FILE *pdtStdout = stdout;
FILE *pdtResOut = NULL;

#include "pdtutilInt.h"
#include "fcntl.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define NUM_PER_ROW 5

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

extern float cpuTimeScaleFactor;
extern long util_time2seconds;

static int wResGblNum = -1;
char *wResGbl = NULL;
int wResTwoPhase = 0;
int wResInitStubSteps = 0;

static double startWallClockTime = 0.0;
/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static void PdtutilIntegerArraySwap(
  Pdtutil_Array_t * obj,
  size_t i,
  size_t j
);
static ssize_t PdtutilIntegerArrayPartition(
  Pdtutil_Array_t * obj,
  size_t p,
  size_t r,
  Pdtutil_ArraySort_e dir
);
static void PdtutilIntegerArrayQSort(
  Pdtutil_Array_t * obj,
  size_t p,
  size_t r,
  Pdtutil_ArraySort_e dir
);

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Pdtutil_HwmccOutputSetup(
)
{
  if (pdtResOut==NULL) {
    // protect from multiple calls
    pdtResOut = pdtStdout;
    pdtStdout = stderr;
  }
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Pdtutil_WresSetup(
  char *name,
  int gblNum
)
{
  if (name != NULL) {
    wResGbl = Pdtutil_StrDup(name);
  }
  wResGblNum = gblNum;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Pdtutil_WresSetTwoPhase(
)
{
  wResTwoPhase = 1;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Pdtutil_WresIncrInitStubSteps(
  int steps
)
{
  wResInitStubSteps += steps;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
char *
Pdtutil_WresFileName(
  void
)
{
  return wResGbl;
}


/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Pdtutil_WresNum(
  char *wRes,
  char *prefix,
  int num,
  int chkMax,
  int chkEnabled,
  int phase,
  int initStubSteps,
  int unfolded
)
{
  static char buf[100];

  if (num<0) return num;

  if (chkEnabled) {
    if (wResGblNum<0) return num;
  }

  if (wRes == NULL) {
    wRes = wResGbl;
  }

  if (wRes == NULL) {
    return 0;
  }

  if (prefix != NULL) {
    if (phase) {
      num = num * 2;
      if (num > 0)
	num--;
    }
    num += initStubSteps;
  }

  if (chkMax && wRes != NULL) {
    int fd, n;
    ssize_t nr, nw, nb = sizeof(int);

    if (wResGblNum >= 0) {
      if (wResGblNum >= num) {
        return 0;
      }
      wResGblNum = num;
    } else {

      sprintf(buf, "/tmp/%s.tmp", wRes);

      fd = open(buf, O_RDWR | O_CREAT, 0666);
      if (fd == -1) {
        fd = open(buf, O_WRONLY | O_CREAT);
        n = -1;
        nr = nb;
      } else {
        nr = read(fd, &n, (size_t) nb);
      }
      if (fd == -1) {
        fprintf(stdout, "Error opening file %s\n", buf);
        return 1;
      }
      if (nr == nb && n >= num) {
        close(fd);
        return 0;
      } else {
        lseek(fd, 0, SEEK_SET);
        nw = write(fd, &num, (size_t) nb);
        close(fd);
      }
    }
  }

  if (wRes == NULL) {
    //    printf("PDT RES: %d\n", num);
  } else {
    int fd;
    ssize_t nb, nw;

    if (!unfolded) num--;
    if (num>=0) {
      sprintf(buf, "%s%d\n", prefix ? prefix : "", num);
      nb = strlen(buf);
      if (pdtResOut != NULL) {
	fprintf(pdtResOut,"%s",buf); fflush(pdtResOut);
      }
      else {
	fd = open(wRes, O_APPEND | O_WRONLY);
	if (fd == -1) {
	  fprintf(stdout, "Error opening file %s\n", wRes);
	  return 1;
	}
	
	nw = write(fd, &buf, (size_t) nb);
	close(fd);
	if (nw != nb) {
	  fprintf(stdout, "Error writing file %s\n", wRes);
	  return 1;
	}
      }
    }
  }
  return 0;
}

/**Function*******************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Pdtutil_WresReadNum(
  int phase,
  int initStubSteps
)
{
  int ret = wResGblNum;

  if (ret > 0) {
    ret -= initStubSteps;
    if (phase) {
      if (ret > 0)
	ret++;
      ret = ret / 2;
    }
  }

  return ret;
}


/**Function********************************************************************

  Synopsis    [Init time scale factors]

  Description [Init time scale factors]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_InitCpuTimeScale(
  int num
)
{
  int i;
  float **v = NULL;
  long startTime, cpuTime;
  static long refTime = 450;    /* msec of ref benchmark */

  if (cpuTimeScaleFactor >= 0)
    return;

  if (0) {
    double a = 1.3456, b = 35.67;
    startTime = util_cpu_time();
    for (i=0; i<100000000; i++) {
      a = (a*b)/0.3456789;
    }
    cpuTime = util_cpu_time() - startTime;
    printf("CPU time for double: %s - ", util_print_time(cpuTime));
  }

  startTime = util_cpu_time();

  v = Pdtutil_Alloc(float *, num);

  for (i = 0; i < num; i++) {
    v[i] = Pdtutil_Alloc(float,
      10
    );
  }

  for (i = 0; i < num * 2; i++) {
    int j = rand() % num;
    float *f = v[j];
    v[j] = Pdtutil_Alloc(float,
      10
    );

    *v[j] = (*f) * 7.2;
    Pdtutil_Free(f);
  }

  for (i = 0; i < num; i++) {
    Pdtutil_Free(v[i]);
  }
  Pdtutil_Free(v);

  cpuTime = util_cpu_time() - startTime;


  cpuTimeScaleFactor = ((float)cpuTime) / refTime;
  util_time2seconds /= cpuTimeScaleFactor;

  printf("CPU time adjustment: %s - ", util_print_time(cpuTime));
  printf("scale factor: %f\n", cpuTimeScaleFactor);

}

/**Function********************************************************************

  Synopsis    [Check The Pdtutil_Alloc Macro.]

  Description [The Alloc mecanism is actually implemented this way in PdTRAV.
    We call Pdtutil_Alloc that it is a macro that basically maps the call on
    the Alloc function of the Cudd package. To check for a null result we
    use Pdtutil_AllocCheck.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void *
Pdtutil_AllocCheck(
  void *pointer
)
{
  if (pointer != NULL) {
    return (pointer);
  } else {
    fprintf(stderr, "Fatal Error: NULL Alloc Value.\n");
    exit(1);
  }
}

/**Function********************************************************************

  Synopsis    [Print on fp the desired number of charaters on a file.]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_ChrPrint(
  FILE * fp /* Where to Print */ ,
  char c /* Which character to Print */ ,
  int n                         /* How many Times */
)
{
  int i;

  for (i = 0; i < n; i++) {
    fprintf(fp, "%c", c);
  }
  fflush(fp);

  return;
}



/**Function********************************************************************

  Synopsis    [Given a string it Returns an Enumerated type for the verbosity
    type.]

  Description [It receives a string; to facilitate the user that string
    can be an easy-to-remember predefined code or an integer number
    (interpreted as a string).
    It returns the verbosity enumerated type.
    This verbosity mechanism is used all over the PdTRAV package.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Pdtutil_VerbLevel_e
Pdtutil_VerbosityString2Enum(
  char *string                  /* String to Analyze */
)
{
  if (strcmp(string, "none") == 0 || strcmp(string, "0") == 0) {
    return (Pdtutil_VerbLevelNone_c);
  }

  if (strcmp(string, "usrMin") == 0 || strcmp(string, "1") == 0) {
    return (Pdtutil_VerbLevelUsrMin_c);
  }

  if (strcmp(string, "usrMed") == 0 || strcmp(string, "2") == 0) {
    return (Pdtutil_VerbLevelUsrMed_c);
  }

  if (strcmp(string, "usrMax") == 0 || strcmp(string, "3") == 0) {
    return (Pdtutil_VerbLevelUsrMax_c);
  }

  if (strcmp(string, "devMin") == 0 || strcmp(string, "4") == 0) {
    return (Pdtutil_VerbLevelDevMin_c);
  }

  if (strcmp(string, "devMed") == 0 || strcmp(string, "5") == 0) {
    return (Pdtutil_VerbLevelDevMed_c);
  }

  if (strcmp(string, "devMax") == 0 || strcmp(string, "6") == 0) {
    return (Pdtutil_VerbLevelDevMax_c);
  }

  if (strcmp(string, "same") == 0 || strcmp(string, "7") == 0) {
    return (Pdtutil_VerbLevelNone_c);
  }

  if (strcmp(string, "debug") == 0 || strcmp(string, "8") == 0) {
    return (Pdtutil_VerbLevelDebug_c);
  }

  Pdtutil_Warning(1, "Choice Not Allowed For Verbosity.");
  return (Pdtutil_VerbLevelNone_c);
}

/**Function********************************************************************

 Synopsis    [Given an Enumerated type Returns a string]

 Description []

 SideEffects [none]

 SeeAlso     []

******************************************************************************/

const char *
Pdtutil_VerbosityEnum2String(
  Pdtutil_VerbLevel_e enumType
)
{
  switch (enumType) {
    case Pdtutil_VerbLevelNone_c:
      return ("none");
      break;
    case Pdtutil_VerbLevelUsrMin_c:
      return ("usrMin");
      break;
    case Pdtutil_VerbLevelUsrMed_c:
      return ("usrMed");
      break;
    case Pdtutil_VerbLevelUsrMax_c:
      return ("usrMax");
      break;
    case Pdtutil_VerbLevelDevMin_c:
      return ("devMin");
      break;
    case Pdtutil_VerbLevelDevMed_c:
      return ("devMed");
      break;
    case Pdtutil_VerbLevelDevMax_c:
      return ("devMax");
      break;
    case Pdtutil_VerbLevelDebug_c:
      return ("debug");
      break;
    case Pdtutil_VerbLevelSame_c:
      return ("same");
      break;
    default:
      Pdtutil_Warning(1, "Choice Not Allowed.");
      return ("none");
      break;
  }
}

/**Function********************************************************************

  Synopsis    [Given an Enumerated type Returns an Integer]

  Description [It receives a string; to facilitate the user that string
    can be an easy-to-remember predefined code or an integer number
    (interpreted as a string).
    It returns the variable file format method type.]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Pdtutil_VariableOrderFormat_e
Pdtutil_VariableOrderFormatString2Enum(
  char *string                  /* String to Analyze */
)
{
  if (strcmp(string, "default") == 0 || strcmp(string, "0") == 0) {
    return (Pdtutil_VariableOrderDefault_c);
  }

  if (strcmp(string, "oneRow") == 0 || strcmp(string, "1") == 0) {
    return (Pdtutil_VariableOrderOneRow_c);
  }

  if (strcmp(string, "ip") == 0 || strcmp(string, "2") == 0) {
    return (Pdtutil_VariableOrderPiPs_c);
  }

  if (strcmp(string, "ipn") == 0 || strcmp(string, "3") == 0) {
    return (Pdtutil_VariableOrderPiPsNs_c);
  }

  if (strcmp(string, "index") == 0 || strcmp(string, "4") == 0) {
    return (Pdtutil_VariableOrderIndex_c);
  }

  if (strcmp(string, "comment") == 0 || strcmp(string, "5") == 0) {
    return (Pdtutil_VariableOrderComment_c);
  }

  Pdtutil_Warning(1, "Choice Not Allowed For Profile Method.");
  return (Pdtutil_VariableOrderComment_c);
}

/**Function********************************************************************

 Synopsis    [Given an Enumerated type Returns a string]

 Description []

 SideEffects [none]

 SeeAlso     []

******************************************************************************/

const char *
Pdtutil_VariableOrderFormatEnum2String(
  Pdtutil_VariableOrderFormat_e enumType
)
{
  switch (enumType) {
    case Pdtutil_VariableOrderDefault_c:
      return ("default");
      break;
    case Pdtutil_VariableOrderOneRow_c:
      return ("oneRow");
      break;
    case Pdtutil_VariableOrderPiPs_c:
      return ("ip");
      break;
    case Pdtutil_VariableOrderPiPsNs_c:
      return ("ipn");
      break;
    case Pdtutil_VariableOrderIndex_c:
      return ("index");
      break;
    case Pdtutil_VariableOrderComment_c:
      return ("comment");
      break;
    default:
      Pdtutil_Warning(1, "Choice Not Allowed.");
      return ("default");
      break;
  }
}

/**Function********************************************************************

 Synopsis    [Returns a complete file name (name, attribute, extension.]

 Description [Takes a file name, an attribute, an extension, and an overwrite
   flag.
   If the name is stdin or stdout return the name as it is.
   Add the attribute to the name.
   Add the extension to the name if it doesn't contains an
   extension already.
   If there is an extension it substitutes it if overwrite = 1.
   Create and returns the new name.]

 SideEffects [none]

 SeeAlso     []

******************************************************************************/

char *
Pdtutil_FileName(
  char *filename /* file name */ ,
  char *attribute /* attribute */ ,
  char *extension /* extension */ ,
  int overwrite                 /* overwrite the extension if 1 */
)
{
  char *name = NULL, *attr = NULL, *ext = NULL, *newfilename = NULL;
  int i, lname, lattr, lext, pointPos;

  /*------------------- Deal with NULL parameters ----------------------*/

  if (filename != NULL) {
    lname = strlen(filename);
    name = Pdtutil_StrDup(filename);
  } else {
    lname = 0;
    name = Pdtutil_StrDup((char *)"");
  }
  if (attribute != NULL) {
    lattr = strlen(attribute);
    attr = Pdtutil_StrDup(attribute);
  } else {
    lattr = 0;
    attr = Pdtutil_StrDup((char *)"");
  }
  if (extension != NULL) {
    lext = strlen(extension);
    ext = Pdtutil_StrDup(extension);
  } else {
    lext = 0;
    ext = Pdtutil_StrDup((char *)"");
  }

  /*----------------------- Check For stdin-stdout --------------------------*/

  if (strcmp(name, "stdin") == 0 || strcmp(name, "stdout") == 0) {
    Pdtutil_Free(attr);
    Pdtutil_Free(ext);
    return (name);
  }

  /*----------------------- Check For Name Extension ------------------------*/

  pointPos = (-1);
  for (i = 0; i < lname; i++) {
    if (name[i] == '.') {
      pointPos = i;
      break;
    }
  }

  /*--------------- Add Attribute and Extension if Necessary ----------------*/

  if (pointPos < 0) {
    /*
     * There is no . in the name
     */

    /* Add both Attribute and Extension at the end of the name */
    newfilename = Pdtutil_Alloc(char,
       (lname + lattr + lext + 1 + 1)
    );

    sprintf(newfilename, "%s%s.%s", name, attr, ext);
  } else {
    /*
     * There is a . in the name
     * (strcpy, strncpy, strcat seem to give some problem here ...)
     */

    newfilename = Pdtutil_Alloc(char,
       (lname + lattr + 1)
    );

    /* Copy the Name */
    for (i = 0; i < pointPos; i++) {
      newfilename[i] = name[i];
    }

    /* Add Attribute in the Middle */
    for (i = 0; i < lattr; i++) {
      newfilename[pointPos + i] = attr[i];
    }

    if (overwrite == 0) {
      /* Add old extension */
      for (i = pointPos; i < lname; i++) {
        newfilename[i + lattr] = name[i];
      }
      newfilename[lname + lattr] = '\0';
    } else {
      /* Add new extension */
      newfilename[pointPos + lattr] = '.';
      for (i = 0; i < lext; i++) {
        newfilename[pointPos + lattr + 1 + i] = ext[i];
      }
      newfilename[pointPos + lattr + lext + 1] = '\0';
    }
  }

  Pdtutil_Free(attr);
  Pdtutil_Free(ext);
  Pdtutil_Free(name);
  return (newfilename);
}


/**Function********************************************************************

 Synopsis    [Opens a file checking for files already opened or impossible
   to open.]

 Description [It receives a file pointer, a file name, and a mode.
   If the file pointer is not null, the file has been already opened
   at this pointer is returned.
   If the filename is "stdin" or "stdout" it considers that as a synonym
   for standard input/output.
   Open mode can be "r" for read or "w" for write.
   In particularly, "rt" open for read a text file and "rb" open
   for read a binary file; "wt" write a text file and "wb" write
   a binary file.<BR>
   Returns a pointer to the file and an integer flag that is 1 if a file
   has been opened.]

 SideEffects [none]

 SeeAlso     [Pdtutil_FileClose]

******************************************************************************/

FILE *
Pdtutil_FileOpen(
  FILE * fp /* IN: File Pointer */ ,
  char *filename /* IN: File Name */ ,
  const char *mode /* IN: Open Mode */ ,
  int *flag                     /* OUT: File Opened (if 1) */
)
{
  if (flag != NULL) {
    *flag = 0;
  }

  if (fp != NULL) {
    return (fp);
  }

  /*----------------------- Check for NULL filename -------------------------*/

  if (filename == NULL) {
    if (strcmp(mode, "r") == 0 || strcmp(mode, "rt") == 0 ||
      strcmp(mode, "rb") == 0) {
      fp = stdin;
    } else {
      fp = stdout;
    }
    return (fp);
  }

  /*--------------------------- Check for stdin -----------------------------*/

  if (strcmp(filename, "stdin") == 0 &&
    (strcmp(mode, "r") || strcmp(mode, "rt") || strcmp(mode, "rb"))) {
    fp = stdin;
    return (fp);
  }

  /*--------------------------- Check for stdout ----------------------------*/

  if (strcmp(filename, "stdout") == 0 &&
    (strcmp(mode, "w") || strcmp(mode, "wt") || strcmp(mode, "wb"))) {
    fp = stdout;
    return (fp);
  }

  /*----------------------------- Open File ---------------------------------*/

  if (flag != NULL) {
    *flag = 1;
  }
  fp = fopen(filename, mode);
  if (fp == NULL) {
    fprintf(stderr, "Error: Cannot open file %s.\n", filename);
    return NULL;
  }

  return (fp);
}



/**Function********************************************************************

 Synopsis    [Closes a file.]

 Description [It closes a file if it is not the standard input or output
   and flag is 1.
   This flag has to be used with the one returned by Pdtutil_FileOpen: If
   a file has been opened is closed too.]

 SideEffects [none]

 SeeAlso     [Pdtutil_FileOpen]

******************************************************************************/

void
Pdtutil_FileClose(
  FILE * fp /* IN: File Pointer */ ,
  int *flag                     /* IN-OUT: File to be Closed: assumed as true if NULL */
)
{
  if ((fp != stdin) && (fp != stdout) && (flag == NULL || *flag == 1)) {
    if (flag != NULL) {
      *flag = 0;
    }
    fclose(fp);
  }

  return;
}

/**Function********************************************************************

 Synopsis    [Parses hierarchical names separated by '.']

 Description [It receives a string. It returns two strings: The first one
   is the content of the original string before the first character "."
   found in the string. The second one is the remaining part of the
   string.
   If the initial string is NULL two NULL pointers are returned.
   If the initial string doen't contain a sharater '.' the first string is
   equal to the original one and the second is NULL.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_ReadSubstring(
  char *stringIn /* String to be parsed */ ,
  char **stringFirstP /* First Sub-String Pointer */ ,
  char **stringSecondP          /* Second Sub-String Pointer */
)
{
  int i, pointPos;

  *stringFirstP = NULL;
  *stringSecondP = NULL;

  if (stringIn == NULL) {
    return;
  }

  pointPos = (-1);
  for (i = 0; i < strlen(stringIn); i++) {
    if (stringIn[i] == '.') {
      pointPos = i;
      break;
    }
  }

  /*--------------- Add Attribute and Extension if Necessary ----------------*/

  if (pointPos < 0) {
    /*
     * There is no . in the name
     */

    *stringFirstP = Pdtutil_StrDup(stringIn);
  } else {
    /*
     * There is a . in the name
     */

    *stringFirstP = Pdtutil_Alloc(char,
      pointPos + 1
    );

    for (i = 0; i < pointPos; i++) {
      (*stringFirstP)[i] = stringIn[i];
    }
    (*stringFirstP)[i] = '\0';
    *stringSecondP = Pdtutil_StrDup(&stringIn[pointPos + 1]);
  }

  return;
}

/**Function********************************************************************

  Synopsis    [parses hierarchical names separated by '.']

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_ReadName(
  char *extName,                /* String to be parsed */
  int *nstr,                    /* Number of names extracted */
  char **names,                 /* Array of names */
  int maxNstr                   /* Maximum number of names */
)
{
  char *string;
  char *token;

  *nstr = 0;

  if (extName[0] == '\0') {
    return;
  }

  /*Duplicate extName, because strtok writes the string */
  string = Pdtutil_StrDup(extName);
  /*Extract first name */
  if ((token = strtok(string, ".")) == NULL) {
    return;
  } else {
    names[*nstr] = Pdtutil_StrDup(token);
    (*nstr)++;
  }
  /*extract remaining names */
  while ((token = strtok(NULL, ".")) != NULL) {
    if (*nstr > maxNstr) {
      fprintf(stderr, "Error: Name Too Long.\n");
      break;
    }
    names[*nstr] = Pdtutil_StrDup(token);
    (*nstr)++;
  };
  free(string);

  return;
}



/**Function*******************************************************************
  Synopsis    [Duplicates a string]
  Description [Duplicates a string and returns it.]
  SideEffects [none]
  SeeAlso     []
*****************************************************************************/
char *
Pdtutil_StrDup(
  char *str
)
{
  char *str2 = NULL;

  if (str != NULL) {
    str2 = Pdtutil_Alloc(char,
      strlen(str) + 1
    );

    if (str2 != NULL) {
      strcpy(str2, str);
    }
  }
  return (str2);
}

/**Function********************************************************************

  Synopsis           [Reads the order in the <em>.ord</em> file.]

  Description        []

  SideEffects        []

  SeeAlso            []

******************************************************************************/

int
Pdtutil_OrdRead(
  char ***pvarnames /* Varnames Array */ ,
  int **pvarauxids /* Varauxids Array */ ,
  char *filename /* File Name */ ,
  FILE * fp /* File Pointer */ ,
  Pdtutil_VariableOrderFormat_e ordFileFormat   /* File Format */
)
{
  int flagFile;
  int nvars, size, id, i, continueFlag;
  char buf[PDTUTIL_MAX_STR_LEN + 1], name[PDTUTIL_MAX_STR_LEN + 1];
  char **varnames = NULL;
  int *varauxids = NULL;

  fp = Pdtutil_FileOpen(fp, filename, "r", &flagFile);
  if (fp == NULL) {
    return (-1);
  }

  size = PDTUTIL_INITIAL_ARRAY_SIZE;
  varnames = Pdtutil_Alloc(char *, size);
  varauxids = Pdtutil_Alloc(int,
    size
  );

  i = 0;
  continueFlag = 1;
  while (continueFlag) {
    /* Check for Read Format */
    if (ordFileFormat == Pdtutil_VariableOrderOneRow_c) {
      if (fscanf(fp, "%s", name) == EOF) {
        break;
      } else {
        id = i;
      }
    } else {
      if (fgets(buf, PDTUTIL_MAX_STR_LEN, fp) == NULL) {
        break;
      } else {
        if (buf[0] == '#') {
          continue;
        }

        if (sscanf(buf, "%s %d", name, &id) < 2) {
          id = i;
          if (sscanf(buf, "%s", name) < 1) {
            continue;
          }
        }
      }
    }

    if (i >= size) {
      /* No more room in arrays: resize */
      size *= 2;
      varnames = Pdtutil_Realloc(char *,
        varnames,
        size
      );
      varauxids = Pdtutil_Realloc(int,
        varauxids,
        size
      );

      if ((varnames == NULL) || (varauxids == NULL)) {
        return (-1);
      }
    }

    varnames[i] = Pdtutil_StrDup(name);
    varauxids[i] = id;
    i++;
  }

  nvars = i;

  if (nvars < size) {
    /* Too much memory allocated: resize */
    varnames = Pdtutil_Realloc(char *,
      varnames,
      nvars
    );
    varauxids = Pdtutil_Realloc(int,
      varauxids,
      nvars
    );

    if ((varnames == NULL) || (varauxids == NULL)) {
      return (-1);
    }
  }

  *pvarnames = varnames;
  *pvarauxids = varauxids;

  Pdtutil_FileClose(fp, &flagFile);

  return (nvars);
}


/**Function********************************************************************

  Synopsis           [Write the order in the <em>.ord</em> file.]

  Description        [Write the order to file, using variable names or
    auxids (or both of them). Array of sorted ids is used if the sortedIds
    parameter is not NULL. Produce different slightly different format
    depending on the format parameter:
    Pdtutil_VariableOrderPiPs_c
      only variables names for Primary Input and Present State Variables are
      stored (one for each row)
    Pdtutil_VariableOrderPiPsNs_c
      store previous information + Next State Variables Names
    Pdtutil_VariableOrderIndex_c
      store previous information + Variable Auxiliary Index
    Pdtutil_VariableOrderComment_c
      store previous information + Comments (row starting for #)
    ]

  SideEffects        []

  SeeAlso            []

******************************************************************************/

int
Pdtutil_OrdWrite(
  char **varnames /* Varnames Array */ ,
  int *varauxids /* Varauxids Array */ ,
  int *sortedIds /* Variable Permutations */ ,
  int nvars /* Number of Variables */ ,
  char *filename /* File Name */ ,
  FILE * fp /* File Pointer */ ,
  Pdtutil_VariableOrderFormat_e ordFileFormat   /* File Format */
)
{
  int flagFile;
  int i, j, k;

  fp = Pdtutil_FileOpen(fp, filename, "w", &flagFile);
  if (fp == NULL) {
    return (1);
  }

  /*
   *  Print out initial comment
   */

  if (ordFileFormat == Pdtutil_VariableOrderComment_c) {
    fprintf(fp, "# ");
    if (varnames != NULL) {
      fprintf(fp, "%-38s", "names");
    }
    if (varauxids != NULL) {
      fprintf(fp, "auxids");
    }
    fprintf(fp, "\n");
  }

  /*
   *  Print out element
   */

  for (k = 0, i = 0; i < nvars; i++) {
    if (sortedIds != NULL) {
      j = sortedIds[i];
    } else {
      j = i;
    }

    Pdtutil_Assert(j < nvars, "Wrong permutation in WriteOrder.");

    /* Skip Next State Variables */
    if (((ordFileFormat == Pdtutil_VariableOrderOneRow_c) ||
        (ordFileFormat == Pdtutil_VariableOrderPiPs_c)) &&
      varnames != NULL && varnames[j] != NULL) {
      if (strstr(varnames[j], "$NS") != NULL) {
        continue;
      }
    }

    /* Store Variable Names For One Row Case */
    if ((ordFileFormat == Pdtutil_VariableOrderOneRow_c) && (varnames != NULL)) {
      Pdtutil_Assert(varnames[j] != NULL,
        "Not supported NULL varname while writing ord.");
      k++;
      if ((k % NUM_PER_ROW) == 0) {
        fprintf(fp, "%s\n", varnames[j]);
      } else {
        fprintf(fp, "%s ", varnames[j]);
      }
      continue;
    }

    /* Store Variable Name */
    if (varnames != NULL) {
      Pdtutil_Assert(varnames[j] != NULL,
        "Not supported NULL varname while writing ord");
      fprintf(fp, "%-40s", varnames[j]);
    }

    /* Store Variable Auxiliary Index */
    if (ordFileFormat >= Pdtutil_VariableOrderIndex_c) {
      if (varauxids != NULL) {
        fprintf(fp, " %d", varauxids[j]);
      }
    }

    /* Next Row */
    fprintf(fp, "\n");
  }                             /* End of for i ... */

  Pdtutil_FileClose(fp, &flagFile);

  return (0);
}

/**Function********************************************************************

  Synopsis    [Duplicates an array of strings]

  Description [Allocates memory and copies source array]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

char **
Pdtutil_StrArrayDup(
  char **array /* array of strings to be duplicated */ ,
  int n                         /* size of the array */
)
{
  char **array2;
  int i;

  if (array == NULL) {
    return (NULL);
  }

  array2 = Pdtutil_Alloc(char *, n);
  if (array2 == NULL) {
    Pdtutil_Warning(1, "Error allocating memory.");
    return (NULL);
  }

  /*
   *  Initialize all slots to NULL for fair FREEing in case of failure
   */

  for (i = 0; i < n; i++) {
    array2[i] = NULL;
  }


  for (i = 0; i < n; i++) {
    if (array[i] != NULL) {
      if ((array2[i] = Pdtutil_StrDup(array[i])) == NULL) {
        Pdtutil_StrArrayFree(array2, n);
        return (NULL);
      }
    }
  }

  return (array2);
}


/**Function********************************************************************

  Synopsis    [Inputs an array of strings]

  Description [Allocates memory and inputs source array. Skips anything from
    '#' to the end-of-line (a comment).
    ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int
Pdtutil_StrArrayRead(
  char ***parray /* array of strings (by reference) */ ,
  FILE * fp                     /* file pointer */
)
{
  int i, nvars, size;
  char buf[PDTUTIL_MAX_STR_LEN + 1];
  char **array;

  Pdtutil_Assert(fp != NULL, "reading from a NULL file");

  size = PDTUTIL_INITIAL_ARRAY_SIZE;
  array = Pdtutil_Alloc(char *,
    size
  );

  for (i = 0; (fscanf(fp, "%s", buf) != EOF);) {
    if (buf[0] == '#') {        /* comment: skip to end-of-line */
      fgets(buf, PDTUTIL_MAX_STR_LEN, fp);
      continue;
    }
    if (i >= size) {            /* no more room in array: resize */
      size *= 2;
      array = Pdtutil_Realloc(char *,
        array,
        size
      );

      if (array == NULL)
        return (-1);
    }
    array[i++] = Pdtutil_StrDup(buf);
  }

  nvars = i;

  if (nvars < size) {           /* too much memory allocated: resize */
    array = Pdtutil_Realloc(char *,
      array,
      size
    );

    if (array == NULL)
      return (-1);
  }

  *parray = array;

  return (nvars);
}

/**Function********************************************************************

  Synopsis    [Outputs an array of strings]

  Description [Allocates memory and inputs source array]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int
Pdtutil_StrArrayWrite(
  FILE * fp /* output file */ ,
  char **array /* array of strings */ ,
  int n                         /* size of the array */
)
{
  int i;

  Pdtutil_Assert(fp != NULL, "writing to a NULL file");

  for (i = 0; i < n; i++) {
    if (fprintf(fp, "%s.\n", array[i]) == EOF) {
      fprintf(stderr, "Pdtutil_StrArrayWrite: Error writing to file.\n");
      return (EOF);
    }
  }

  return n;
}


/**Function********************************************************************

  Synopsis    [Frees an array of strings]

  Description [Frees memory for strings and the array of pointers]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_StrArrayFree(
  char **array /* array of strings */ ,
  int n                         /* size of the array */
)
{
  int i;

  if (array == NULL) {
    return;
  }

  if (n >= 0) {
    for (i = 0; i < n; i++) {
      Pdtutil_Free(array[i]);
    }
  } else {
    for (i = 0; array[i] != NULL; i++) {
      Pdtutil_Free(array[i]);
    }
  }

  Pdtutil_Free(array);

  return;
}


/**Function********************************************************************

  Synopsis    [Duplicates an array of ints]

  Description [Allocates memory and copies source array]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int *
Pdtutil_IntArrayDup(
  int *array /* Array of Integers to be Duplicated */ ,
  int n                         /* size of the array */
)
{
  int i, *array2;

  if (array == NULL) {
    return (NULL);
  }

  array2 = Pdtutil_Alloc(int, n);
  if (array2 == NULL) {
    fprintf(stderr, "Pdtutil_IntArrayDup: Error allocating memory.\n");
    return NULL;
  }

  for (i = 0; i < n; i++) {
    array2[i] = array[i];
  }

  return (array2);
}

/**Function********************************************************************

  Synopsis    [Inputs an array of ints]

  Description [Allocates memory and inputs source array. Skips anything from
    '#' to the end-of-line (a comment).
    ]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int
Pdtutil_IntArrayRead(
  int **parray /* array of strings (by reference) */ ,
  FILE * fp                     /* file pointer */
)
{
  int i, nvars, size;
  char buf[PDTUTIL_MAX_STR_LEN + 1];
  int *array;

  Pdtutil_Assert(fp != NULL, "reading from a NULL file");

  size = PDTUTIL_INITIAL_ARRAY_SIZE;
  array = Pdtutil_Alloc(int,
    size
  );

  for (i = 0; (fscanf(fp, "%s", buf) != EOF);) {
    if (buf[0] == '#') {        /* comment: skip to end-of-line */
      fgets(buf, PDTUTIL_MAX_STR_LEN, fp);
      continue;
    }
    if (i >= size) {            /* no more room in array: resize */
      size *= 2;
      array = Pdtutil_Realloc(int,
        array,
        size
      );

      if (array == NULL)
        return (-1);
    }
    sscanf(buf, "%d", &array[i++]);
  }

  nvars = i;

  if (nvars < size) {           /* too much memory allocated: resize */
    array = Pdtutil_Realloc(int,
      array,
      size
    );

    if (array == NULL)
      return (-1);
  }

  *parray = array;

  return (nvars);
}


/**Function********************************************************************

  Synopsis    [Outputs an array of ints]

  Description [Outputs an array of ints]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int
Pdtutil_IntArrayWrite(
  FILE * fp /* output file */ ,
  int *array /* array of ints */ ,
  int n                         /* size of the array */
)
{
  int i;

  Pdtutil_Assert(fp != NULL, "writing to a NULL file");

  for (i = 0; i < n; i++) {
    if (fprintf(fp, "%d.\n", array[i]) == EOF) {
      fprintf(stderr, "Pdtutil_IntArrayWrite: Error writing to file.\n");
      return (EOF);
    }
  }

  return n;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_FreeCall(
  void *p
)
{
  if (p)
    FREE(p);
}

/**Function********************************************************************

  Synopsis    [Tranform time in seconds.]

  Description []

  SideEffects [none]

  SeeAlso     [util_print_time]

******************************************************************************/

unsigned long
Pdtutil_ConvertTime(
  unsigned long t
)
{
  return ((unsigned long)(t / 1000));
}

/**Function********************************************************************

  Synopsis    [Allocs an array of integers]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Pdtutil_Array_t *
Pdtutil_PointerArrayAlloc(
  size_t size
)
{
  Pdtutil_Array_t *obj;

  PdtutilArrayScalAlloc(obj, void *,
    size
  );

  return obj;
}

/**Function********************************************************************

  Synopsis    [Reads the data field of an integer array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void *
Pdtutil_PointerArrayRead(
  Pdtutil_Array_t * obj,
  size_t idx
)
{
  void *val;

  PdtutilArrayScalRead(obj, void *,
    idx,
    val
  );

  return val;
}

/**Function********************************************************************

  Synopsis    [Writes the data field of an integer array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_PointerArrayWrite(
  Pdtutil_Array_t * obj,
  size_t idx,
  void *val
)
{
  while (idx>=Pdtutil_IntegerArraySize(obj)) {
    PdtutilArrayScalRealloc(obj, void *);
  }
  if (obj->num<=idx) obj->num=idx+1;
  PdtutilArrayScalWrite(obj, void *,
    idx,
    val
  );
}

/**Function********************************************************************

  Synopsis    [Provides the size of the array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

size_t
Pdtutil_PointerArraySize(
  Pdtutil_Array_t * obj
)
{
  return PdtutilArraySize(obj);
}

/**Function********************************************************************

  Synopsis    [Provides the size of the array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

size_t
Pdtutil_PointerArrayNum(
  Pdtutil_Array_t * obj
)
{
  return PdtutilArrayNum(obj);
}

/**Function********************************************************************

  Synopsis    [Frees an array of integers]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_PointerArrayFree(
  Pdtutil_Array_t * obj
)
{
  if (obj!=NULL)
    PdtutilArrayFree(obj);
}


/**Function********************************************************************

  Synopsis    [Allocs an array of integers]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Pdtutil_Array_t *
Pdtutil_IntegerArrayAlloc(
  size_t size
)
{
  Pdtutil_Array_t *obj;

  PdtutilArrayScalAlloc(obj, int,
    size
  );

  return obj;
}

/**Function********************************************************************

  Synopsis    [Reads the data field of an integer array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Pdtutil_IntegerArrayRead(
  Pdtutil_Array_t * obj,
  size_t idx
)
{
  int val;

  PdtutilArrayScalRead(obj, int,
    idx,
    val
  );

  return val;
}

/**Function********************************************************************

  Synopsis    [set array name]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_ArrayWriteName(
  Pdtutil_Array_t * obj,
  char *name
)
{
  obj->name = Pdtutil_StrDup(name);
}

/**Function********************************************************************

  Synopsis    [read array name]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

char *
Pdtutil_ArrayReadName(
  Pdtutil_Array_t * obj
)
{
  return obj->name;
}

/**Function********************************************************************

  Synopsis    [Writes the data field of an integer array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_IntegerArrayWrite(
  Pdtutil_Array_t * obj,
  size_t idx,
  int val
)
{
  if (idx>Pdtutil_IntegerArrayNum(obj)) {

  }
  PdtutilArrayScalWrite(obj, int,
    idx,
    val
  );
}

/**Function********************************************************************

  Synopsis    [Inserts a value into the array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_IntegerArrayInsert(
  Pdtutil_Array_t * obj,
  int val
)
{
  size_t idx;

  if (PdtutilArrayHasFreeItems(obj)) {
    PdtutilArrayGetLastFreeItem(obj, idx);
  } else if (PdtutilArrayNeedsRealloc(obj)) {
    PdtutilArrayScalRealloc(obj, int
    );

    PdtutilArrayGetNextIdx(obj, idx);
  } else {
    PdtutilArrayGetNextIdx(obj, idx);
  }

  PdtutilArrayScalWrite(obj, int,
    idx,
    val
  );

  PdtutilArrayIncNum(obj);
}

/**Function********************************************************************

  Synopsis    [Inserts a value into the array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_IntegerArrayInsertLast(
  Pdtutil_Array_t * obj,
  int val
)
{
  size_t idx;

  if (PdtutilArrayNeedsRealloc(obj)) {
    PdtutilArrayScalRealloc(obj, int
    );
  }
  PdtutilArrayGetNextIdx(obj, idx);

  PdtutilArrayScalWrite(obj, int,
    idx,
    val
  );

  PdtutilArrayIncNum(obj);
}

/**Function********************************************************************

  Synopsis    [Removes a value from the array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_IntegerArrayReset(
  Pdtutil_Array_t * obj
)
{
  PdtutilArraySetNum(obj, 0);
}

/**Function********************************************************************

  Synopsis    [Removes a value from the array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Pdtutil_IntegerArrayRemove(
  Pdtutil_Array_t * obj,
  size_t idx
)
{
  int val;

  PdtutilArrayScalRead(obj, int,
    idx,
    val
  );

  PdtutilArraySetNextFreeItem(obj, idx);
  PdtutilArrayDecNum(obj);

  return val;
}

/**Function********************************************************************

  Synopsis    [Removes the last value from the array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

int
Pdtutil_IntegerArrayRemoveLast(
  Pdtutil_Array_t * obj
)
{
  size_t idx;
  int val;

  PdtutilArrayGetLastIdx(obj, idx);
  PdtutilArrayScalRead(obj, int,
    idx,
    val
  );

  PdtutilArrayDecNum(obj);

  return val;
}

/**Function********************************************************************

  Synopsis    [Provides the size of the array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

size_t
Pdtutil_IntegerArraySize(
  Pdtutil_Array_t * obj
)
{
  return PdtutilArraySize(obj);
}

/**Function********************************************************************

  Synopsis    [Provides the num of elements of the array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

size_t
Pdtutil_IntegerArrayNum(
  Pdtutil_Array_t * obj
)
{
  return PdtutilArrayNum(obj);
}

/**Function********************************************************************

  Synopsis    [Increments the number of elements]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_IntegerArrayIncNum(
  Pdtutil_Array_t * obj
)
{
  PdtutilArrayIncNum(obj);
}

/**Function********************************************************************

  Synopsis    [Decrements the number of elements]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_IntegerArrayDecNum(
  Pdtutil_Array_t * obj
)
{
  PdtutilArrayDecNum(obj);
}

/**Function********************************************************************

  Synopsis    [Resizes the array]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_IntegerArrayResize(
  Pdtutil_Array_t * obj,
  size_t size
)
{
  PdtutilArrayScalResize(obj, int,
    size
  );
}


/**Function********************************************************************

  Synopsis    [Sorts an array of integers by means of the quick sort algo]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_IntegerArraySort(
  Pdtutil_Array_t * obj,
  Pdtutil_ArraySort_e dir
)
{
  size_t num = PdtutilArrayNum(obj);

  Pdtutil_Assert(!PdtutilArrayHasFreeItems(obj), "Cannot sort sparse array");
  if (num > 0) {
    PdtutilIntegerArrayQSort(obj, 0, num - 1, dir);
  }
}

/**Function********************************************************************

  Synopsis    [Frees an array of integers]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_LogMsg(
  int thrdId,
  const char *fmt,
  ...
)
{
  va_list ap;

  if (thrdId >= 0) {
    fprintf(stdout, "THRD %d: ", thrdId);
    fflush(stdout);
    fflush(stderr);
  }
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  fflush(stdout);
  fflush(stderr);
}

/**Function********************************************************************

  Synopsis    [Frees an array of integers]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_Printf(
  const char *fmt,
  ...
)
{
  va_list ap;

  va_start(ap, fmt);
  vfprintf(pdtStdout, fmt, ap);
  fflush(pdtStdout);
}

/**Function********************************************************************

  Synopsis    [Frees an array of integers]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Pdtutil_IntegerArrayFree(
  Pdtutil_Array_t * obj
)
{
  if (obj!=NULL)
    PdtutilArrayFree(obj);
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Swaps the content of two cells into an array of integers]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

static void
PdtutilIntegerArraySwap(
  Pdtutil_Array_t * obj,
  size_t i,
  size_t j
)
{
  int a, b;

  PdtutilArrayScalRead(obj, int,
    i,
    a
  );
  PdtutilArrayScalRead(obj, int,
    j,
    b
  );
  PdtutilArrayScalWrite(obj, int,
    i,
    b
  );
  PdtutilArrayScalWrite(obj, int,
    j,
    a
  );
}

/**Function********************************************************************

  Synopsis    [Quicksort partitioning procedure]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

static ssize_t
PdtutilIntegerArrayPartition(
  Pdtutil_Array_t * obj,
  size_t p,
  size_t r,
  Pdtutil_ArraySort_e dir
)
{
  ssize_t i, j;
  int x, tmp;
  int res;

  PdtutilArrayScalRead(obj, int,
    p,
    x
  );

  i = p - 1;
  j = r + 1;

  while (1) {
    /* proceedes from last element backward,
       if A[j] is greater than the first element */
    do {
      j--;
      PdtutilArrayScalRead(obj, int,
        j,
        tmp
      )
       res = tmp - x;
    } while ((res > 0 && dir == Pdtutil_ArraySort_Asc_c) ||
      (res < 0 && dir == Pdtutil_ArraySort_Desc_c));
    /* proceedes from first element forward,
       if A[j] is smaller than the first element */
    do {
      i++;
      PdtutilArrayScalRead(obj, int,
        i,
        tmp
      );

      res = tmp - x;
    } while ((res < 0 && dir == Pdtutil_ArraySort_Asc_c) ||
      (res > 0 && dir == Pdtutil_ArraySort_Desc_c));

    if (i < j) {
      PdtutilIntegerArraySwap(obj, i, j);
    } else {
      return j;
    }
  }

  return -1;
}



/**Function********************************************************************

  Synopsis    [Quicksort implementation]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

static void
PdtutilIntegerArrayQSort(
  Pdtutil_Array_t * obj,
  size_t p,
  size_t r,
  Pdtutil_ArraySort_e dir
)
{
  ssize_t q;

  if (p < r) {
    q = PdtutilIntegerArrayPartition(obj, p, r, dir);
    Pdtutil_Assert(q >= 0, "Quicksort failed, partitioning error");
    PdtutilIntegerArrayQSort(obj, p, q, dir);
    PdtutilIntegerArrayQSort(obj, q + 1, r, dir);
  }
}



/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
char *
Pdtutil_PrintTime(
  unsigned long t
)
{
  static char s[80];
  extern long util_normTime2seconds;

  (void)sprintf(s, "%.2f s (norm %.2f s)",
		((float)t) / util_time2seconds,
		((float)t) / util_normTime2seconds);
  return s;
}

/**Function********************************************************************

  Synopsis    []

  Description [return a long which represents the elapsed processor
     time in milliseconds since some constant reference]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
long
Pdtutil_CpuTime(
  void
)
{
  extern float cpuTimeScaleFactor;

  long t = 0;

#if 1
  struct rusage rusage;

  (void)getrusage(RUSAGE_THREAD, &rusage);
  t = (long)rusage.ru_utime.tv_sec * 1000 + rusage.ru_utime.tv_usec / 1000;
#else
  struct timespec timespec;
  (void)clock_gettime(RUSAGE_THREAD, &rusage);
  t = (long)rusage.ru_utime.tv_sec * 1000 + rusage.ru_utime.tv_usec / 1000;
#endif

  return cpuTimeScaleFactor < 0 ? t : ((long)(t / cpuTimeScaleFactor));
}

/**Function********************************************************************

  Synopsis    []

  Description [return a long which represents the elapsed processor
     time in milliseconds since some constant reference]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
double
Pdtutil_WallClockTime(
  void
)
{
  double res = -1;
  struct timeval tv;
  if (!gettimeofday (&tv, 0))
    {
      res = 1e-6 * tv.tv_usec;
      res += tv.tv_sec;
    }
  return res-startWallClockTime;
}

/**Function********************************************************************

  Synopsis    []

  Description [return a long which represents the elapsed processor
     time in milliseconds since some constant reference]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
void
Pdtutil_WallClockTimeReset(
  void
)
{
  double res = -1;
  struct timeval tv;
  if (!gettimeofday (&tv, 0))
    {
      res = 1e-6 * tv.tv_usec;
      res += tv.tv_sec;
    }
  startWallClockTime = res;
}


/**Function*******************************************************************
*
  Synopsis    []

  Description [return a long which represents the elapsed processor
     time in milliseconds since some constant reference]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
long
Pdtutil_ThrdRss(
  void
)
{
  long m = 0;

  struct rusage rusage;

  (void)getrusage(RUSAGE_THREAD, &rusage);
  //  m = (long)rusage.ru_maxrss;
  m = (long)rusage.ru_ixrss;

  return (m);
}
/**Function********************************************************************

  Synopsis    []

  Description [return a long which represents the elapsed processor
     time in milliseconds since some constant reference]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
long
Pdtutil_ProcRss(
  void
)
{
  long m = 0;

  struct rusage rusage;

  (void)getrusage(RUSAGE_SELF, &rusage);
  m = (long)rusage.ru_maxrss;

  return (m);
}


/**Function********************************************************************

  Synopsis    []

  Description [CUDD function wrapper for retrieving the current time]

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
long
Pdtutil_CpuTimeCuddLegacy(
  void
)
{
  return util_cpu_time();
}


#if 1

/**Function********************************************************************
  Synopsis    [Alloc eq class]
  Description [Alloc eq class]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Pdtutil_EqClasses_t *
Pdtutil_EqClassesAlloc(
  int nCl
)
{
  Pdtutil_EqClasses_t *eqCl = Pdtutil_Alloc(Pdtutil_EqClasses_t, 1);

  if (eqCl == NULL)
    return eqCl;


  eqCl->nCl = nCl;
  eqCl->splitDone = 0;
  eqCl->refineSteps = 0;
  eqCl->undefinedVal = -1;
  eqCl->initialized = 0;

  eqCl->eqClass = Pdtutil_Alloc(int, nCl);
  eqCl->eqClassNum = Pdtutil_Alloc(int,
    nCl
  );

  eqCl->state = Pdtutil_Alloc(Pdtutil_EqClassState_e, nCl);
  eqCl->cnf.var = Pdtutil_Alloc(int,
    nCl
  );
  eqCl->cnf.act = Pdtutil_Alloc(int,
    nCl
  );
  eqCl->splitLdr = Pdtutil_Alloc(int,
    nCl
  );
  eqCl->val = Pdtutil_Alloc(int,
    nCl
  );
  eqCl->complEq = Pdtutil_Alloc(char,
    nCl
  );
  eqCl->activeClass = Pdtutil_Alloc(char,
    nCl
  );

  //  vec<vec<Lit> > splitClasses0, splitClasses1;
  //  int *potentialImpl;
  return eqCl;

}


/**Function********************************************************************
  Synopsis    [Free eq class]
  Description [Free eq class]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Pdtutil_EqClassesFree(
  Pdtutil_EqClasses_t * eqCl
)
{
  Pdtutil_Free(eqCl->eqClass);
  Pdtutil_Free(eqCl->eqClassNum);
  Pdtutil_Free(eqCl->state);
  Pdtutil_Free(eqCl->cnf.var);
  Pdtutil_Free(eqCl->cnf.act);
  Pdtutil_Free(eqCl->splitLdr);
  Pdtutil_Free(eqCl->val);
  Pdtutil_Free(eqCl->complEq);
  Pdtutil_Free(eqCl->activeClass);

  Pdtutil_Free(eqCl);
}

/**Function********************************************************************
  Synopsis    [Reset eq class]
  Description [Reset eq class]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
 Pdtutil_EqClassesReset(
  Pdtutil_EqClasses_t * eqCl
) {
  int i;

  eqCl->splitDone = 0;
  eqCl->refineSteps = 0;
  eqCl->initialized = 0;
  for (i = 0; i < eqCl->nCl; i++) {
    eqCl->activeClass[i] = 0;
    eqCl->eqClass[i] = -1;      /* unassigned */
    eqCl->eqClassNum[i] = 0;
    eqCl->state[i] = Pdtutil_EqClass_Unassigned_c;
    eqCl->splitLdr[i] = -1;
    eqCl->val[i] = 0;
    eqCl->complEq[i] = 0;
  }

}

/**Function********************************************************************
  Synopsis    [Reset eq class]
  Description [Reset eq class]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
 Pdtutil_EqClassesSplitLdrReset(
  Pdtutil_EqClasses_t * eqCl
) {
  int i;

  for (i = 0; i < eqCl->nCl; i++) {
    if (eqCl->splitLdr[i] >= 0) {
      int ldr = eqCl->splitLdr[i];

      eqCl->complEq[ldr] = 0;   /* new ldr: reset compl flag */
    }
    eqCl->splitLdr[i] = -1;
  }

}

/**Function********************************************************************
  Synopsis    [Reset eq class]
  Description [Reset eq class]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
 Pdtutil_EqClassesPrint(
  Pdtutil_EqClasses_t * eqCl
) {
  int i, tot = 0;

  printf("CL #: %d\n", eqCl->nCl);
  for (i = 0; i < eqCl->nCl; i++) {
    printf("CL[%2d] - L:%2d - V:%d - C:%d - N:%d\n", i,
      eqCl->eqClass[i], eqCl->val[i], eqCl->complEq[i], eqCl->eqClassNum[i]);
    tot += eqCl->eqClassNum[i];
  }
  printf("TOT #: %d\n", tot);

}

/**Function********************************************************************
  Synopsis    [Set val of i-th element]
  Description [Set val of i-th element]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
 Pdtutil_EqClassVal(
  Pdtutil_EqClasses_t * eqCl,
  int i
) {
  Pdtutil_Assert(i < eqCl->nCl, "wrong index in eq class processing");
  return (eqCl->val[i]);
}

/**Function********************************************************************
  Synopsis    [Set val of i-th element]
  Description [Set val of i-th element]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
 Pdtutil_EqClassSetVal(
  Pdtutil_EqClasses_t * eqCl,
  int i,
  int val
) {
  Pdtutil_Assert(i < eqCl->nCl, "wrong index in eq class processing");
  eqCl->val[i] = val;
}

/**Function********************************************************************
  Synopsis    [Set cnf var of i-th element]
  Description [Set cnf var of i-th element]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
 Pdtutil_EqClassCnfVarWrite(
  Pdtutil_EqClasses_t * eqCl,
  int i,
  int var
) {
  Pdtutil_Assert(i < eqCl->nCl, "wrong index in eq class processing");
  eqCl->cnf.var[i] = var;
}

/**Function********************************************************************
  Synopsis    [Set cnf var of i-th element]
  Description [Set cnf act of i-th element]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
 Pdtutil_EqClassCnfActWrite(
  Pdtutil_EqClasses_t * eqCl,
  int i,
  int act
) {
  Pdtutil_Assert(i < eqCl->nCl, "wrong index in eq class processing");
  eqCl->cnf.act[i] = act;
}

/**Function********************************************************************
  Synopsis    [Read cnf var of i-th element]
  Description [Read cnf var of i-th element]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
 Pdtutil_EqClassCnfVarRead(
  Pdtutil_EqClasses_t * eqCl,
  int i
) {
  Pdtutil_Assert(i < eqCl->nCl, "wrong index in eq class processing");
  return eqCl->cnf.var[i];
}

/**Function********************************************************************
  Synopsis    [Read cnf act of i-th element]
  Description [Read cnf act of i-th element]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
 Pdtutil_EqClassCnfActRead(
  Pdtutil_EqClasses_t * eqCl,
  int i
) {
  Pdtutil_Assert(i < eqCl->nCl, "wrong index in eq class processing");
  return eqCl->cnf.act[i];
}

/**Function********************************************************************
  Synopsis    [Check if class element is complemented w.r.t. leader]
  Description [Check if class element is complemented w.r.t. leader]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Pdtutil_EqClassIsCompl(
  Pdtutil_EqClasses_t * eqCl,
  int i
)
{
  Pdtutil_Assert(i >= 0
    && i < eqCl->nCl, "wrong index in eq class processing");
  return (eqCl->complEq[i] != 0);
}


/**Function********************************************************************
  Synopsis    [Check if class element is leader]
  Description [Check if class element is leader]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Pdtutil_EqClassIsLdr(
  Pdtutil_EqClasses_t * eqCl,
  int i
)
{
  Pdtutil_Assert(i >= 0
    && i < eqCl->nCl, "wrong index in eq class processing");
  return (eqCl->eqClass[i] == i);
}


/**Function********************************************************************
  Synopsis    [Get size of class]
  Description [Get size of class]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Pdtutil_EqClassNum(
  Pdtutil_EqClasses_t * eqCl,
  int i
)
{
  Pdtutil_Assert(i >= 0
    && i < eqCl->nCl, "wrong index in eq class processing");
  return (eqCl->eqClassNum[i]);
}


/**Function********************************************************************
  Synopsis    [Get size of class]
  Description [Get size of class]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Pdtutil_EqClassesNum(
  Pdtutil_EqClasses_t * eqCl
)
{
  return (eqCl->nCl);
}

/**Function********************************************************************
  Synopsis    [Get size of class]
  Description [Get size of class]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Pdtutil_EqClassesSetUndefined(
  Pdtutil_EqClasses_t * eqCl,
  int undefinedVal
)
{
  eqCl->undefinedVal = undefinedVal;
}


/**Function********************************************************************
  Synopsis    [Get size of class]
  Description [Get size of class]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Pdtutil_EqClassesSplitNum(
  Pdtutil_EqClasses_t * eqCl
)
{
  return (eqCl->splitDone);
}

/**Function********************************************************************
  Synopsis    [Get size of class]
  Description [Get size of class]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Pdtutil_EqClassesInitialized(
  Pdtutil_EqClasses_t * eqCl
)
{
  return (eqCl->initialized);
}

/**Function********************************************************************
  Synopsis    [Get size of class]
  Description [Get size of class]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Pdtutil_EqClassesSetInitialized(
  Pdtutil_EqClasses_t * eqCl
)
{
  eqCl->initialized = 1;
}

/**Function********************************************************************
  Synopsis    [Get size of class]
  Description [Get size of class]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Pdtutil_EqClassesRefineStepsNum(
  Pdtutil_EqClasses_t * eqCl
)
{
  return (eqCl->refineSteps);
}

/**Function********************************************************************
  Synopsis    [Get index of class leader]
  Description [Get index of class leader]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Pdtutil_EqClassLdr(
  Pdtutil_EqClasses_t * eqCl,
  int i
)
{
  Pdtutil_Assert(i >= 0
    && i < eqCl->nCl, "wrong index in eq class processing");
  return (eqCl->eqClass[i]);
}

/**Function********************************************************************
  Synopsis    [Get index of class leader]
  Description [Get index of class leader]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Pdtutil_EqClassSetLdr(
  Pdtutil_EqClasses_t * eqCl,
  int i,
  int l
)
{
  Pdtutil_Assert(i >= 0
    && i < eqCl->nCl, "wrong index in eq class processing");
  Pdtutil_Assert(l >= 0
    && l < eqCl->nCl, "wrong leader in eq class processing");
  eqCl->eqClass[i] = l;
  eqCl->eqClassNum[i]++;
  eqCl->state[i] = Pdtutil_EqClass_Ldr_c;
}

/**Function********************************************************************
  Synopsis    [Get index of class leader]
  Description [Get index of class leader]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Pdtutil_EqClassSetState(
  Pdtutil_EqClasses_t * eqCl,
  int i,
  Pdtutil_EqClassState_e state
)
{
  Pdtutil_Assert(i >= 0
    && i < eqCl->nCl, "wrong index in eq class processing");
  eqCl->state[i] = state;
}

/**Function********************************************************************
  Synopsis    [Get index of class leader]
  Description [Get index of class leader]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Pdtutil_EqClassSetSingleton(
  Pdtutil_EqClasses_t * eqCl,
  int i
)
{
  Pdtutil_Assert(i >= 0
    && i < eqCl->nCl, "wrong index in eq class processing");

  if (eqCl->state[i] == Pdtutil_EqClass_Singleton_c) {
    /* do nothing */
  } else {
    int ldr_i = eqCl->eqClass[i];

    /* remove from class */
    if (ldr_i >= 0 && ldr_i != i) {
      Pdtutil_Assert(ldr_i >= 0, "invalid set singleton operation");
      eqCl->eqClass[i] = i;
      eqCl->splitDone++;
      eqCl->eqClassNum[ldr_i]--;
    } else {
      eqCl->eqClass[i] = i;
      eqCl->eqClassNum[i] = 1;
    }
    eqCl->state[i] = Pdtutil_EqClass_Singleton_c;
  }
}


/**Function********************************************************************
  Synopsis    [Update classes based on new element values]
  Description [Update classes based on new element values]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Pdtutil_EqClassState_e Pdtutil_EqClassUpdate(Pdtutil_EqClasses_t * eqCl, int i
) {
  int j;

  eqCl->activeClass[i] = 0;
  eqCl->refineSteps++;

  if (eqCl->state[i] == Pdtutil_EqClass_Singleton_c) {
    /* do nothing */
  } else if (eqCl->undefinedVal > 0 && (eqCl->val[i] == eqCl->undefinedVal)) {
    /* X value: become singleton class */
    int ldr_i = eqCl->eqClass[i];

    if (ldr_i != i) {
      /* remove from class */
      eqCl->eqClass[i] = i;
      if (ldr_i >= 0) {
        eqCl->splitDone++;
        eqCl->eqClassNum[ldr_i]--;
      }
    }
    eqCl->state[i] = Pdtutil_EqClass_Singleton_c;
  } else if (eqCl->eqClass[i] == i) {
    /* class leader */
    eqCl->activeClass[i] = 1;   /* this is a class: possibly unchanged */
    eqCl->splitLdr[i] = -1;
    Pdtutil_Assert(eqCl->state[i] == Pdtutil_EqClass_Ldr_c,
      "wrong class state");
    return Pdtutil_EqClass_Ldr_c;
  } else if (eqCl->eqClass[i] == -1) {
    if (eqCl->undefinedVal != 0 || eqCl->val[i] < 2) {
      /* not yet assigned: put in const class */
      eqCl->eqClass[i] = 0;
      eqCl->eqClassNum[0]++;
      eqCl->complEq[i] = eqCl->val[i];
      eqCl->activeClass[0] = 2;
      eqCl->splitDone++;
      Pdtutil_Assert(eqCl->state[i] == Pdtutil_EqClass_Unassigned_c,
        "wrong class state");
      eqCl->state[i] = Pdtutil_EqClass_Member_c;
      return Pdtutil_EqClass_JoinConst_c;
    } else {
      /* symbolic eval */
      int myAbsVal = eqCl->val[i] / 2;

      for (j = 1; j < i; j++) {
        if (eqCl->eqClass[j] == j && eqCl->val[j] / 2 == myAbsVal) {
          eqCl->eqClass[i] = j;
          eqCl->eqClassNum[j]++;
          eqCl->complEq[i] = eqCl->val[i] != eqCl->val[j];
          eqCl->activeClass[j] = 2;
          eqCl->splitDone++;
          Pdtutil_Assert(eqCl->state[i] == Pdtutil_EqClass_Unassigned_c,
            "wrong class state");
          eqCl->state[i] = Pdtutil_EqClass_Member_c;
          return Pdtutil_EqClass_JoinSplit_c;
        }
      }
      /* do not join - become ldr */
      eqCl->eqClass[i] = i;
      eqCl->eqClassNum[i] = 1;
      eqCl->complEq[i] = 0;
      eqCl->activeClass[i] = 2;
      Pdtutil_Assert(eqCl->state[i] == Pdtutil_EqClass_Unassigned_c,
        "wrong class state");
      eqCl->state[i] = Pdtutil_EqClass_Ldr_c;
      return Pdtutil_EqClass_SplitLdr_c;
    }
  } else {
    /* class member: get leader */
    int isCompl = eqCl->complEq[i];
    int ldr_i = eqCl->eqClass[i];
    int ldrVal = (isCompl ? ((eqCl->val[ldr_i] + 1) % 2) : eqCl->val[ldr_i]);
    int singletonLdr = eqCl->state[ldr_i] == Pdtutil_EqClass_Singleton_c;

    if (eqCl->undefinedVal == 0 && eqCl->val[ldr_i] >= 2) {
      ldrVal = isCompl ? (eqCl->val[ldr_i] ^ 0x1) : eqCl->val[ldr_i];
    }

    Pdtutil_Assert(eqCl->state[i] == Pdtutil_EqClass_Member_c,
      "wrong class state");
    if (singletonLdr || eqCl->val[i] != ldrVal) {
      /* split */
      eqCl->splitDone++;
      eqCl->eqClassNum[ldr_i]--;
      Pdtutil_Assert(eqCl->eqClassNum[ldr_i] > 0, "neg class num");
      if (eqCl->splitLdr[ldr_i] < 0) {
        /* this is a new ldr */
        eqCl->eqClassNum[i]++;
        eqCl->splitLdr[ldr_i] = i;
        eqCl->eqClass[i] = i;
        //  eqCl->complEq[i] = 0; /* new ldr: reset compl flag */
        eqCl->activeClass[i] = eqCl->activeClass[ldr_i] = 2;
        eqCl->state[i] = Pdtutil_EqClass_Ldr_c;
        return Pdtutil_EqClass_SplitLdr_c;
      } else {
        /* join existing new ldr
           join can be temporary - can be speculated and require
           recursive split */
        int testRecur = singletonLdr;

        Pdtutil_Assert(eqCl->splitLdr[ldr_i] < i, "illegal cl leader");
        ldr_i = eqCl->splitLdr[ldr_i];
        eqCl->eqClassNum[ldr_i]++;
        eqCl->eqClass[i] = ldr_i;
        eqCl->complEq[i] = eqCl->complEq[i] != eqCl->complEq[ldr_i];
        testRecur |= (eqCl->undefinedVal == 0);
        if (testRecur) {
          int doRecur = eqCl->complEq[i] ^ (eqCl->val[i] != eqCl->val[ldr_i]);

          doRecur |= (eqCl->undefinedVal == 0 &&
            (eqCl->val[ldr_i] / 2 != eqCl->val[i] / 2));
          if (doRecur) {
            return Pdtutil_EqClassUpdate(eqCl, i);
          }
        } else {
          Pdtutil_Assert(eqCl->complEq[i] == (eqCl->val[i] !=
              eqCl->val[ldr_i]), "error setting split compl flags");
        }
        return Pdtutil_EqClass_JoinSplit_c;
      }
    } else {
      /* keep in class */
      return Pdtutil_EqClass_SameClass_c;
    }
  }

}

/**Function********************************************************************
  Synopsis    [Update classes based on new element values]
  Description [Update classes based on new element values]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Pdtutil_EqClassState_e Pdtutil_EqClassState(Pdtutil_EqClasses_t * eqCl, int i) {
  return eqCl->state[i];
}


/**Function********************************************************************
  Synopsis    [create list]
  Description [create and initialize the list]
  SideEffects []
  SeeAlso     []
******************************************************************************/

Pdtutil_List_t *
Pdtutil_ListCreate(
  void
)
{
  Pdtutil_List_t *retList;

  retList = Pdtutil_Alloc(Pdtutil_List_s, 1);
  retList->head = retList->tail = NULL;
  retList->N = 0;
  return retList;
}

/**Function********************************************************************
  Synopsis    [insert an Item in the head of the list]
  Description [insert an Item in the head of the list]
  SideEffects []
  SeeAlso     []
******************************************************************************/

void
Pdtutil_ListInsertHead(
  Pdtutil_List_t * list,
  Pdtutil_Item_t item
)
{

  NodeList_t node = NodeListCreate(item);

  if (list->head == NULL) {
    list->head = list->tail = node;
  } else {
    node->next = list->head;
    list->head = node;
  }
  list->N++;
}


/**Function********************************************************************
  Synopsis    [insert an Item in the tail of the list]
  Description [insert an Item in the tail of the list]
  SideEffects []
  SeeAlso     []
******************************************************************************/

void
Pdtutil_ListInsertTail(
  Pdtutil_List_t * list,
  Pdtutil_Item_t item
)
{

  NodeList_t node = NodeListCreate(item);

  if (list->tail == NULL) {
    list->head = list->tail = node;
  } else {
    list->tail->next = node;
    list->tail = node;
  }
  list->N++;
}

/**Function********************************************************************
  Synopsis    [extract an Item from the head of the list]
  Description [extract an Item from the head of the list]
  SideEffects []
  SeeAlso     []
******************************************************************************/

Pdtutil_Item_t
Pdtutil_ListExtractHead(
  Pdtutil_List_t * list
)
{

  Pdtutil_Item_t item;

  item = list->head->item;


  if (list->head == NULL) {
    return NULL;
  } else if (list->head == list->tail) {
    NodeListFree(list->head);
    list->tail = list->head = NULL;
  } else {
    NodeList_t node = list->head;

    list->head = node->next;

    NodeListFree(node);

  }

  list->N--;
  return item;
}


/**Function********************************************************************
  Synopsis    [extract an Item from the tail of the list]
  Description [extract an Item from the tail of the list]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Pdtutil_Item_t
Pdtutil_ListExtractTail(
  Pdtutil_List_t * list
)
{

  Pdtutil_Item_t item;

  item = list->tail->item;


  if (list->tail == NULL) {
    return NULL;
  } else if (list->head == list->tail) {
    NodeListFree(list->head);
    list->tail = list->head = NULL;
  } else {


    NodeList_t node = list->head;

    while (node->next->next != NULL)
      node = node->next;

    NodeListFree(node->next);
    node->next = NULL;

    list->tail = node;

  }

  list->N--;
  return item;
}

int
Pdtutil_ListSize(
  Pdtutil_List_t * list
)
{
  return list->N;
}

NodeList_t
NodeListCreate(
  Pdtutil_Item_t item
)
{
  NodeList_t nodeRet;

  nodeRet = Pdtutil_Alloc(NodeList_s, 1);
  nodeRet->item = item;
  nodeRet->next = NULL;
  return nodeRet;
}


void
NodeListFree(
  NodeList_t node
)
{
  //Pdtutil_Free(node->item);
  Pdtutil_Free(node);
}

void
Pdtutil_ListFree(
  Pdtutil_List_t * list
)
{
  if (list != NULL) {
    NodeList_t node;

    while (list->head != NULL) {
      node = list->head;
      list->head = node->next;
      NodeListFree(node);
    }
    Pdtutil_Free(list);
  }
}


/**Function********************************************************************
  Synopsis    [create Option list]
  Description [create and initialize the option list]
  SideEffects []
  SeeAlso     []
******************************************************************************/

Pdtutil_OptList_t *
Pdtutil_OptListCreate(
  const Pdt_OptModule_e module
)
{
  Pdtutil_OptList_t *retList;

  retList = Pdtutil_Alloc(Pdtutil_OptList_s, 1);
  retList->module = module;
  retList->head = retList->tail = NULL;
  retList->N = 0;
  return retList;
}

/**Function********************************************************************
  Synopsis    [create Option list]
  Description [create and initialize the option list]
  SideEffects []
  SeeAlso     []
******************************************************************************/

Pdt_OptModule_e
Pdtutil_OptListReadModule(
  Pdtutil_OptList_t * optList
)
{
  return optList->module;
}

/**Function********************************************************************
  Synopsis    [create Option list]
  Description [create and initialize the option list]
  SideEffects []
  SeeAlso     []
******************************************************************************/

int
Pdtutil_OptListEmpty(
  Pdtutil_OptList_t * optList
)
{
  return (optList->head == NULL);
}

/**Function********************************************************************
  Synopsis    [insert an Item in the head of the list]
  Description [insert an Item in the head of the list]
  SideEffects []
  SeeAlso     []
******************************************************************************/

void
Pdtutil_OptListInsertHead(
  Pdtutil_OptList_t * list,
  Pdtutil_OptItem_t item
)
{

  NodeOptList_t *node = NodeOptListCreate(item);

  if (list->head == NULL) {
    list->head = list->tail = node;
  } else {
    node->next = list->head;
    list->head = node;
  }
  list->N++;
}


/**Function********************************************************************
  Synopsis    [insert an Item in the tail of the list]
  Description [insert an Item in the tail of the list]
  SideEffects []
  SeeAlso     []
******************************************************************************/

void
Pdtutil_OptListInsertTail(
  Pdtutil_OptList_t * list,
  Pdtutil_OptItem_t item
)
{
  NodeOptList_t *node = NodeOptListCreate(item);

  if (list->tail == NULL) {
    list->head = list->tail = node;
  } else {
    list->tail->next = node;
    list->tail = node;
  }
  list->N++;
}

/**Function********************************************************************
  Synopsis    [extract an Item from the head of the list]
  Description [extract an Item from the head of the list]
  SideEffects []
  SeeAlso     []
******************************************************************************/

Pdtutil_OptItem_t
Pdtutil_OptListExtractHead(
  Pdtutil_OptList_t * list
)
{

  Pdtutil_OptItem_t item;

  if (list->N == 0) {
    return item;
  }


  item = list->head->item;

  if (list->head == list->tail) {
    NodeOptListFree(list->head);
    list->tail = list->head = NULL;
  } else {
    NodeOptList_t *node = list->head;

    list->head = node->next;
    NodeOptListFree(node);

  }

  list->N--;
  return item;
}


/**Function********************************************************************
  Synopsis    [extract an Item from the tail of the list]
  Description [extract an Item from the tail of the list]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Pdtutil_OptItem_t
Pdtutil_OptListExtractTail(
  Pdtutil_OptList_t * list
)
{

  Pdtutil_OptItem_t item;

  if (list->N == 0)
    return item;

  item = list->tail->item;

  if (list->head == list->tail) {
    NodeOptListFree(list->head);
    list->tail = list->head = NULL;
  } else {

    NodeOptList_t *node = list->head;

    while (node->next->next != NULL)
      node = node->next;

    NodeOptListFree(node->next);
    node->next = NULL;

    list->tail = node;


  }

  list->N--;
  return item;
}

int
Pdtutil_OptListSize(
  Pdtutil_OptList_t * list
)
{
  return list->N;
}


void
Pdtutil_OptListFree(
  Pdtutil_OptList_t * list
)
{
  if (list != NULL) {
    NodeOptList_t *node;

    while (list->head != NULL) {
      node = list->head;
      list->head = node->next;
      NodeOptListFree(node);
    }
    Pdtutil_Free(list);
  }
}

NodeOptList_t *
NodeOptListCreate(
  Pdtutil_OptItem_t item
)
{
  NodeOptList_t *nodeRet;

  nodeRet = Pdtutil_Alloc(NodeOptList_t, 1);
  nodeRet->item = item;
  nodeRet->next = NULL;
  return nodeRet;
}

void
NodeOptListFree(
  NodeOptList_t * node
)
{
  //Pdtutil_Free(node->item);
  Pdtutil_Free(node);
}

#endif


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Pdtutil_MapInt2Int_t *
Pdtutil_MapInt2IntAlloc(
  int mDir,
  int mInv,
  int defaultValue,
  Pdtutil_MapInt2Int_e type
)
{
  Pdtutil_MapInt2Int_t *r;
  int i = 0;

  r = Pdtutil_Alloc(Pdtutil_MapInt2Int_t, 1);
  switch (type) {
    case Pdtutil_MapInt2IntDirInv_c:
      {
        r->inv.mapArray = Pdtutil_IntegerArrayAlloc(mInv);
        r->inv.max = mInv;
        for (i = 0; i < mInv; i++) {
          Pdtutil_IntegerArrayWrite(r->inv.mapArray, i, defaultValue);
        }
      }
    case Pdtutil_MapInt2IntDir_c:
      {
        r->dir.mapArray = Pdtutil_IntegerArrayAlloc(mDir);
        r->dir.max = mDir;
        for (i = 0; i < mDir; i++) {
          Pdtutil_IntegerArrayWrite(r->dir.mapArray, i, defaultValue);
        }
      }
      break;
    case Pdtutil_MapInt2IntNone_c:
      return NULL;
  }
  r->defaultValue = defaultValue;
  r->type = type;

  return r;
}

/**Function********************************************************************
  Synopsis    [Write a value j in position i]
  Description [Write a value j in position i]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Pdtutil_MapInt2IntWrite(
  Pdtutil_MapInt2Int_t * map,
  int i,
  int j
)
{
  switch (map->type) {
    case Pdtutil_MapInt2IntDirInv_c:
      {
        int invMax = map->inv.max;

        if (j >= invMax) {
          int k = invMax, s;

          while (k < j)
            k *= 2;
          Pdtutil_IntegerArrayResize(map->inv.mapArray, j + 1);
          for (s = invMax; s <= k; s++)
            Pdtutil_IntegerArrayWrite(map->inv.mapArray, s, map->defaultValue);
          map->inv.max = k + 1;
        }
        Pdtutil_IntegerArrayWrite(map->inv.mapArray, j, i);
      }
    case Pdtutil_MapInt2IntDir_c:
      {
        int dirMax = map->dir.max;

        if (i >= dirMax) {
          int k = dirMax, s;

          while (k < i)
            k *= 2;
          Pdtutil_IntegerArrayResize(map->dir.mapArray, k + 1);
          for (s = dirMax; s <= k; s++)
            Pdtutil_IntegerArrayWrite(map->dir.mapArray, s, map->defaultValue);
          map->dir.max = k + 1;
        }
        Pdtutil_IntegerArrayWrite(map->dir.mapArray, i, j);
      }
      break;
    case Pdtutil_MapInt2IntNone_c:
      break;
  }
}

/**Function********************************************************************
  Synopsis    [Read the value in position i in the direct mapping array]
  Description [Read the value in position i in the direct mapping array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Pdtutil_MapInt2IntReadDir(
  Pdtutil_MapInt2Int_t * map,
  int i
)
{
  return Pdtutil_IntegerArrayRead(map->dir.mapArray, i);
}

/**Function********************************************************************
  Synopsis    [Read the value in position j in the inverse mapping array]
  Description [Read the value in position j in the inverse mapping array]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Pdtutil_MapInt2IntReadInv(
  Pdtutil_MapInt2Int_t * map,
  int j
)
{
  switch (map->type) {
    case Pdtutil_MapInt2IntDirInv_c:
      return Pdtutil_IntegerArrayRead(map->inv.mapArray, j);
    default:
      return -1;
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Pdtutil_MapInt2IntFree(
  Pdtutil_MapInt2Int_t * map
)
{
  switch (map->type) {
    case Pdtutil_MapInt2IntDirInv_c:
      {
        Pdtutil_IntegerArrayFree(map->inv.mapArray);
      }
    case Pdtutil_MapInt2IntDir_c:
      {
        Pdtutil_IntegerArrayFree(map->dir.mapArray);
      }
      break;
    case Pdtutil_MapInt2IntNone_c:
      break;
  }
  Pdtutil_Free(map);
}


/**Function*******************************************************************

  Synopsis    [Compares weights of sort infos]
  Description [Compares weights of sort infos]
  SideEffects [none]
  SeeAlso     []

*****************************************************************************/
static int
SortInfoCompare(
  const void *s1,
  const void *s2
)
{
  int r;
  Pdtutil_SortInfo_t *pSI1 = (Pdtutil_SortInfo_t *) s1;
  Pdtutil_SortInfo_t *pSI2 = (Pdtutil_SortInfo_t *) s2;

  r = 0;

  if (pSI1->weight > pSI2->weight)
    r = 1;
  else if (pSI1->weight < pSI2->weight)
    r = -1;
  else
    r = 0;

  return (r);
}

/**Function********************************************************************
  Synopsis    [sort clause by increasing order]
  Description []
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
void
Pdtutil_InfoArraySort(
  Pdtutil_SortInfo_t * SortInfoArray,
  int n,
  int increasing
)
{

  qsort((void **)SortInfoArray, n, sizeof(Pdtutil_SortInfo_t),
    SortInfoCompare);
  if (!increasing) {
    /* invert order */
    int i;

    for (i = 0; i < n / 2; i++) {
      Pdtutil_SortInfo_t tmp = SortInfoArray[n - i - 1];

      SortInfoArray[n - i - 1] = SortInfoArray[i];
      SortInfoArray[i] = tmp;
    }
  }

}



/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
char *
Pdtutil_StrSkipPrefix(
  char *str,
  char *prefix
)
{
  int n = strlen(prefix);

  if (strncmp(str, prefix, n) == 0) {
    return str + n;
  } else {
    return NULL;
  }
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     [Ddi_BddMakeFromCU]
******************************************************************************/
int
Pdtutil_StrRemoveNumSuffix(
  char *str,
  char separator
)
{
  int k, n = strlen(str);
  int ret = -1;

  for (k = n - 1; k > 0 && str[k] != separator; k--) ;

  if (str[k] == separator) {
    str[k] = '\0';
    ret = atoi(str + k + 1);
  }
  return ret;
}

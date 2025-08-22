/**CHeaderFile*****************************************************************

  FileName    [pdtutilInt.h]

  PackageName [pdtutil] 

  Synopsis    [Internal header file of PdUtil package]

  Description [In the pdtutil package no internal functions are declared.]

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

#ifndef _PDTUTILINT
#define _PDTUTILINT

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/

#include "pdtutil.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

typedef enum Pdtutil_ArrayType_e {
  Pdtutil_ArrayType_Scal_c,
  Pdtutil_ArrayType_Ptr_c
} Pdtutil_ArrayType_e;

/*---------------------------------------------------------------------------*/
/* Structure & type declarations                                             */
/*---------------------------------------------------------------------------*/


struct NodeList_s {
  Pdtutil_Item_t item;
  struct NodeList_s *next;
};

typedef struct NodeList_s *NodeList_t;


struct Pdtutil_List_s {
  NodeList_t head;
  NodeList_t tail;
  int N;
};

struct NodeOptList_s {
  Pdtutil_OptItem_t item;
  struct NodeOptList_s *next;
};
typedef struct NodeOptList_s NodeOptList_t;

struct Pdtutil_OptList_s {
  Pdt_OptModule_e module;
  NodeOptList_t *head;
  NodeOptList_t *tail;
  int N;
};

struct Pdtutil_Array_t {
  char *name;
  union {
    void *array;
    void **parray;
  } data;
  Pdtutil_ArrayType_e array_type;
  size_t size;
  size_t num;
  size_t *free_list;
  size_t free_list_num;
};

struct Pdtutil_EqClasses_s {
  int nCl, splitDone, refineSteps, undefinedVal;
  int initialized;
  int *eqClass;
  int *eqClassNum;
  int *splitLdr;
  Pdtutil_EqClassState_e *state;
  struct {
    int *var, *act;
  } cnf;
  int *val;
  char *complEq, *activeClass;
  //  vec<vec<Lit> > splitClasses0, splitClasses1;
  //  int *potentialImpl;
};

struct Pdtutil_MapInt2Int_s {
  Pdtutil_MapInt2Int_e type;
  int defaultValue;
  struct {
    int max;
    Pdtutil_Array_t *mapArray;
  } dir;
  struct {
    int max;
    Pdtutil_Array_t *mapArray;
  } inv;
};

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/* togliere underscore */

#define PdtutilArrayScalAlloc(obj, type, size) {          \
  (obj) = Pdtutil_Alloc(struct Pdtutil_Array_t, 1);       \
  (obj)->data.array = (void *) Pdtutil_Alloc(type, size); \
  (obj)->array_type = Pdtutil_ArrayType_Scal_c;           \
  (obj)->size = size;                                     \
  (obj)->num = 0;                                         \
  (obj)->free_list = Pdtutil_Alloc(size_t, size);         \
  (obj)->free_list_num = 0;                               \
  (obj)->name = NULL;                                     \
}

#define PdtutilArrayScalResize(obj, type, size) {                                            \
  Pdtutil_Assert((obj)->array_type == Pdtutil_ArrayType_Scal_c, "Array type is not scalar"); \
  Pdtutil_Assert(size > (obj)->size, "Cannot downgrade the array size");                     \
  (obj)->size = size;                                                                        \
  (obj)->data.array = (void *) Pdtutil_Realloc(type, (obj)->data.array, (obj)->size);        \
  (obj)->free_list = (size_t *) Pdtutil_Realloc(size_t, (obj)->free_list, (obj)->size);      \
}

#define PdtutilArrayScalRealloc(obj, type) {                                                 \
  Pdtutil_Assert((obj)->array_type == Pdtutil_ArrayType_Scal_c, "Array type is not scalar"); \
  (obj)->size = ((obj)->size > 0 ? 2 * (obj)->size : 1);                                     \
  (obj)->data.array = (void *) Pdtutil_Realloc(type, (obj)->data.array, (obj)->size);        \
  (obj)->free_list = (size_t *) Pdtutil_Realloc(size_t, (obj)->free_list, (obj)->size);      \
}

#define PdtutilArrayPtrAlloc(obj, type, size) {             \
  (obj) = Pdtutil_Alloc(struct Pdtutil_Array_t, 1);         \
  (obj)->data.parray = (void **) Pdtutil_Alloc(type, size); \
  (obj)->array_type = Pdtutil_ArrayType_Ptr_c;              \
  (obj)->size = size;                                       \
  (obj)->num = 0;                                           \
  (obj)->free_list = Pdtutil_Alloc(size_t, size);           \
  (obj)->free_list_num = 0;                                 \
  (obj)->name = NULL;                                       \
}

#define PdtutilArrayPtrResize(obj, type, size) {                                                \
  Pdtutil_Assert((obj)->array_type == Pdtutil_ArrayType_Ptr_c, "Array type is not vectorial");  \
  Pdtutil_Assert(size > (obj)->size, "Cannot downgrade the array size");                        \
  (obj)->size = size;                                                                           \
  (obj)->data.parray = (void **) Pdtutil_Realloc(type, (obj)->data.parray, (obj)->size);        \
  (obj)->free_list = (size_t *) Pdtutil_Realloc(size_t, (obj)->free_list, (obj)->size);         \
}

#define PdtutilArrayPtrRealloc(obj, type) {                                                     \
  Pdtutil_Assert((obj)->array_type == Pdtutil_ArrayType_Ptr_c, "Array type is not vectorial");  \
  (obj)->size = ((obj)->size > 0 ? 2 * (obj)->size : 1);                                        \
  (obj)->data.parray = (void **) Pdtutil_Realloc(type, (obj)->data.parray, (obj)->data.parray); \
  (obj)->free_list = (size_t *) Pdtutil_Realloc(size_t, (obj)->free_list, (obj)->size);         \
}

#define PdtutilArrayFree(obj) {                              \
  Pdtutil_Free((obj)->free_list);                            \
  if ((obj)->array_type == Pdtutil_ArrayType_Scal_c) {       \
    Pdtutil_Free((obj)->data.array);                         \
  } else if ((obj)->array_type == Pdtutil_ArrayType_Ptr_c) { \
    Pdtutil_Free((obj)->data.parray);                        \
  }                                                          \
  if ((obj)->name != NULL) Pdtutil_Free((obj)->name);        \
  Pdtutil_Free(obj);                                         \
}

#define PdtutilArrayScalRead(obj, type, idx, val)  {                                         \
  Pdtutil_Assert((obj)->array_type == Pdtutil_ArrayType_Scal_c, "Array type is not scalar"); \
  Pdtutil_Assert((int)(idx) < (int)(obj)->size, "Array index out of bound"); \
  val = ((type *) (obj)->data.array)[idx];                                                   \
}

#define PdtutilArrayPtrRead(obj, type, idx, val)  {                                            \
  Pdtutil_Assert((obj)->array_type == Pdtutil_ArrayType_Ptr_c, "Array type is not vectorial"); \
  Pdtutil_Assert((int)(idx) < (int)(obj)->size, "Array index out of bound"); \
  val = ((type **) (obj)->data.parray)[idx];                                                   \
}

#define PdtutilArrayScalWrite(obj, type, idx, val)  {                                        \
  Pdtutil_Assert((obj)->array_type == Pdtutil_ArrayType_Scal_c, "Array type is not scalar"); \
  Pdtutil_Assert((int)(idx) < (int)(obj)->size, "Array index out of bound"); \
  ((type *) (obj)->data.array)[idx] = val;                                                   \
}

#define PdtutilArrayPtrWrite(obj, type, idx, val)  {                                           \
  Pdtutil_Assert((obj)->array_type == Pdtutil_ArrayType_Ptr_c, "Array type is not vectorial"); \
  Pdtutil_Assert((int)(idx) < (obj)->size, "Array index out of bound");      \
  ((type **) (obj)->data.parray)[idx] = val;                                                   \
}

#define PdtutilArrayFreeItemRemove(obj, idx) {           \
  for (int i = 0; i < (obj)->free_list_num; i++) {       \
    if ((obj)->free_list[i] == idx) {                    \
      for (int j = i+1; j < (obj)->free_list_num; j++) { \
	(obj)->free_list[j-1] = (obj)->free_list[j];     \
      }                                                  \
      (obj)->free_list_num--;                            \
      break;                                             \
    }                                                    \
  }                                                      \
}

#define PdtutilArraySize(obj)                  (obj)->size
#define PdtutilArrayNum(obj)                   (obj)->num
#define PdtutilArrayNeedsRealloc(obj)          ((obj)->num >= (obj)->size)
#define PdtutilArrayHasFreeItems(obj)          ((obj)->free_list_num > 0)

#define PdtutilArrayGetLastFreeItem(obj, idx) {                                         \
  Pdtutil_Assert((obj)->free_list_num > 0, "No free item is available for this array"); \
  idx = ((obj)->free_list[--(obj)->free_list_num]);                                     \
}

#define PdtutilArraySetNextFreeItem(obj, idx) {                  \
  Pdtutil_Assert(idx < (obj)->size, "Array index out of bound"); \
  (obj)->free_list[(obj)->free_list_num++] = idx;                \
}

#define PdtutilArrayGetLastIdx(obj, idx) {          \
  Pdtutil_Assert((obj)->num > 0, "Array is empty"); \
  idx = ((obj)->num - 1);                           \
}

#define PdtutilArrayGetNextIdx(obj, idx) {           \
  Pdtutil_Assert((obj)->size > 0, "Array is empty"); \
  idx = ((obj)->num);                                \
}

#define PdtutilArrayIncNum(obj) {                                     \
  Pdtutil_Assert((obj)->num < (obj)->size, "Array num out of bound"); \
  ((obj)->num)++;                                                     \
}

#define PdtutilArrayDecNum(obj) {                           \
  Pdtutil_Assert((obj)->num > 0, "Array num out of bound"); \
  ((obj)->num)--;                                           \
}

#define PdtutilArraySetNum(obj, idx) {                         \
  Pdtutil_Assert(idx < (obj)->size, "Array num out of bound"); \
  (obj)->num = idx;                                            \
}

#define PdtutilArrayIsScal(obj)                ((obj)->array_type == Pdtutil_ArrayType_Scal_c)
#define PdtutilArrayIsPtr(obj)                ((obj)->array_type == Pdtutil_ArrayType_Ptr_c)

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
NodeList_t NodeListCreate(
  Pdtutil_Item_t item
);
void NodeListFree(
  NodeList_t node
);

static NodeOptList_t *NodeOptListCreate(
  Pdtutil_OptItem_t item
);
static void NodeOptListFree(
  NodeOptList_t * node
);

/*---------------------------------------------------------------------------*/


/**AutomaticEnd***************************************************************/

#endif                          /* _PDTUTILINT */

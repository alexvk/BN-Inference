/********************************************************************
$RCSfile: clique.h,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:54:41 $
********************************************************************/

#ifndef CLIQUE_INCLUDED
#define CLIQUE_INCLUDED

#include "set.h"

static void *swap_ptr;  /* Pointer used in following macro */
#define swap(A,B)   (swap_ptr=A,A=B,B=swap_ptr)

typedef struct {
  int numNodes;
  int *nodes;
  int sortIndex;
  int node;
} TMPCLIQUE;

typedef struct {
  TMPCLIQUE **cliqueTree;
  int cliqueCount;
  int *cliqueStack;
  int stackptr;
  int maxCount;
} CLIQUECONTROL;

#endif /* CLIQUE_INCLUDED */

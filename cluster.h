/********************************************************************
$RCSfile: cluster.h,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:54:41 $
********************************************************************/

#ifndef CLUSTER_INCLUDED
#define CLUSTER_INCLUDED

#include <stdlib.h>
#include "utils.h"

typedef enum {EDGE_NO_MSG, EDGE_MSG_U2D, EDGE_MSG_D2U} STATUS;

/* a cluster node with probability distribution */
typedef struct {
  int numNodes;
  int *nodes;
  long *odometer;
  long stateSpaceSize;
  VECTOR *probDistr;
  int numNghb;
  int *edges;

  /* optfact specific */
  int inclNode;

  /* parallel implementation specific */
  long pars, pare;

  /* MR specific */
  int numProbs;

} CLUSTER;

/* an edge with precomputed summation indices */
typedef struct {
  int numNodes;
  int *nodes;
  long msgLen;
  int unode;
  long uindicesLen;
  long *uindices;
  int dnode;
  long dindicesLen;
  long *dindices;
  STATUS status;

  /* parallel implementation specific */
  long pars, pare;
  int numToWait;
  int masterID;

  /* MR specific */
  int numOrigProbs;
  int* origProbs;

} EDGE;

#endif /* CLUSTER_INCLUDED */

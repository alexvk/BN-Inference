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
  int inclNode;
  VECTOR *probDistr;
  int numNghb;
  int *edges;

  /* parallel implementation specific */
  long pars, pare;

} CLUSTER;

/* an edge with precomputed summation indices */
typedef struct {
  int numNodes;
  int *nodes;
  long msgLen;
  int unode;
  long uindicesLen;
  int *uindices;
  int dnode;
  long dindicesLen;
  int *dindices;
  STATUS status;

  /* parallel implementation specific */
  long pars, pare;
  int numToWait;
  int masterID;

} EDGE;

#endif /* CLUSTER_INCLUDED */

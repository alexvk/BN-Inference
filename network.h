/********************************************************************
$RCSfile: network.h,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:54:41 $
********************************************************************/

#ifndef NETWORK_INCLUDED
#define NETWORK_INCLUDED

#include "cluster.h"
#include "utils.h"
#include "set.h"

/* Algorithms */
#define ALG_UNKNOWN			0
#define ALG_CLUSTER			1
#define ALG_OPTFACT			2

/* Formats */
#define FORMAT_ERGO			0
#define FORMAT_STD			1

/* Default values */
#define EMPTY				-1
#define NOTSET				-1

/* Maximum memory requirement */
#define AV_MEMORY	0x1000000000

/* Parallel implementation specific */
#ifdef MULTIPROC
#define UPDATEEDGE(net, e, d)		CompparUpdateEdge(net, e, d)
#define UPDATENODE(net, n)		CompparUpdateNode(net, n)
#define BELIEFS(net, n, k)		CompparBeliefs(net, n, k)
#define EVIDENCE(net, n)		CompparIntroduceEvidence(net, n)
#define SUMFACTORS(net)			CompparSumFactors(net)
#define STOREPRIORS(net)		CompparStorePriors(net)
#define RESTORETREE(net)		CompparRestoreTree(net)
#else
#define UPDATEEDGE(net, e, d)		ComputeUpdateEdge(net, e, d)
#define UPDATENODE(net, n)		ComputeUpdateNode(net, n)
#define BELIEFS(net, n, k)		ComputeBeliefs(net, n, k)
#define EVIDENCE(net, n)		ComputeIntroduceEvidence(net, n)
#define SUMFACTORS(net)			OptfactSumFactors(net)
#define STOREPRIORS(net)		ComputeStorePriors(net)
#define RESTORETREE(net)		ComputeRestoreTree(net)
#define CompparMeasBeg()		{;}
#define CompparMeasEnd()		{;}
#endif /* MULTIPROC */

/* Node structure */
typedef struct node_t {
  char *nodeName;
  char **nodeLabels;
  int numValues;
  VECTOR *beliefs;		/* inferred beliefs */
  int numParents;
  int *parentIndices;
  long *odometer;
  int numChildren;
  int *childIndices;
  int numProbs;
  VECTOR *probMatrix;		/* used for table representation */

  /* NOISY-OR specific */
  VECTOR *leak;
  VECTOR ***c;

  /* used for tree construction */
  int marked;
  int dirty;

} NODE;


/* Network structure */
typedef struct network_t {

  char *netName;
  int algorithm;
  int numNodes;
  NODE *nodes;
  int *nodeSizes;		/* helper buffers to improve */
  int *nodeStates;		/* the data locality */
  int *nodeMarked;
  char **graphTable;

  /* Inference specific */
  int *nodeCliqueDFS;		/* node assignments */
  int *scratchBuffer;		/* used arbitrary as needed */
  int *nodeEvidence;		/* evidence so far */

  /* Cluster tree specific */
  int numCliques;
  long totPotentials;
  int *cliqueMarked;
  CLUSTER **cnodes;
  VECTOR **priors;		/* prior PD's */
  EDGE **cedges;
  char **ctgt;

  /* Optimal factoring specific */
  int *ab;
  int aNum;
  int abNum;
  int numGroups;
  CLUSTER **groups;
  EDGE **gedges;
  int *uindices;
  int *dindices;

  /* parallel specific */
  int maxCTIL, maxGTIL;

} NETWORK;

/* global normalization variable */
extern VECTOR probEvid;

/* forward declarations */
VECTOR ComputeCondProb();
void NetworkFree(NETWORK *net);

#endif /* NETWORK_INCLUDED */

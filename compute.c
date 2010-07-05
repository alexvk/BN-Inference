/********************************************************************
$RCSfile: compute.c,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:52:53 $
********************************************************************/
static char rcsid[] = "$Id: compute.c,v 1.1 1997/10/15 02:52:53 alexvk Exp alexvk $";

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "cluster.h"
#include "network.h"
#include "utils.h"
#include "set.h"


/*******************************************************************
  Helper functions for node operations
*******************************************************************/

/*
 * Computes the states of nodes based on a multidimensional index
 */
void ComputeDecomposeIndex(net, nodes, numNodes, index)
     NETWORK *net;
     int *nodes;
     int numNodes;
     long index;
{
  int i, node, state;

  bzero(net->nodeStates, net->numNodes*sizeof(int));

  for(i=numNodes; i>0; i--) {
    node = nodes[i - 1];
    state = index % (long) net->nodeSizes[node];
    index /= (long) net->nodeSizes[node];
    net->nodeStates[node] = state;
  }
}

/*
 * The same as previous, but does not zero the rest of the states
 */
void ComputeIncorporateIndex(net, nodes, numNodes, index)
     NETWORK *net;
     int *nodes;
     int numNodes;
     long index;
{
  int i, node, state;

  for(i=numNodes; i>0; i--) {
    node = nodes[i - 1];
    state = index % (long) net->nodeSizes[node];
    index /= (long) net->nodeSizes[node];
    net->nodeStates[node] = state;
  }
}

/*
 * Computes the multidimensional index based on the states of nodes
 */
void ComputeIndex(net, nodes, numNodes, index)
     NETWORK *net;
     int *nodes;
     int numNodes;
     long *index;
{
  int i;
  long result;

  for(i=0, result=0; i<numNodes; i++) {
    result *= net->nodeSizes[nodes[i]];
    result += net->nodeStates[nodes[i]];
  }

  *index = result;
}

/*
 * Changes the state vector according to a node ordering
 */
void ComputeNextState(net, nodes, numNodes)
     NETWORK *net;
     int *nodes;
     int numNodes;
{
  int i, node, state;

  for(i=numNodes; i>0; i--) {
    node = nodes[i - 1];
    state = net->nodeStates[node] + 1;
    if(state < net->nodeSizes[node]) {
      net->nodeStates[node] = state;
      break;
    }
    net->nodeStates[node] = 0;
  }
}

/*
 * Compute the belief of a node by summing the probDistr
 */
void ComputeBeliefs(net, node, k)
     NETWORK *net;
     int node, k;
{
  long s;
  int pos, state;
  CLUSTER *X = net->cnodes[node];
  NODE *x = net->nodes + k;

  Dbg(printf("Computing beliefs for x%d in X%d\n", k, node));

  bzero(x->beliefs, x->numValues*sizeof(VECTOR));
  SetMemberPos(k, X->nodes, X->numNodes, &pos);
  for(s=0; s<X->stateSpaceSize; s++) {
    state = (s / X->odometer[pos]) % (long) x->numValues;
    x->beliefs[state] += X->probDistr[s];
  }
}

/*
 * Computes the indices over the free indices of a node
 */
void ComputeEdgeIndices(net, X, E, indices, size)
     NETWORK *net;
     CLUSTER *X;
     EDGE *E;
     long *indices;
     long size;
{
  int i, j, numFree;
  long s;

  for(i=j=numFree=0; i<X->numNodes; i++) {
    if(j<E->numNodes && X->nodes[i] == E->nodes[j]) {
      j++;
    } else {
      net->scratchBuffer[numFree++] = X->nodes[i];
    }
    net->nodeStates[X->nodes[i]] = 0;
  }

  for(s=0; s<size; s++) {
    ComputeIndex(net, X->nodes, X->numNodes, indices + s);
    ComputeNextState(net, net->scratchBuffer, numFree);
  }
}


/********************************************************************
  Probabilistic inference related procedures
********************************************************************/

/*
 * Computes a partial sum of a vector
 */
void ComputePartialSum(net, parsum, vector, indices, size)
     NETWORK *net;
     VECTOR *parsum;
     VECTOR *vector;
     long *indices;
     long size;
{
  long s;
  VECTOR result = 0;
  for(s=0; s<size; s++) {
    result += vector[indices[s]];
  }
  *parsum = result;
}

/*
 * Multiplies some entries of a vector
 */
void ComputeMultiplyPD(net, factor, vector, indices, size)
     NETWORK *net;
     VECTOR factor;
     VECTOR *vector;
     long *indices;
     long size;
{
  long s;
  for(s=0; s<size; s++) {
    vector[indices[s]] *= factor;
  }
}

/*
 * Sums the joint probability distribution up the tree
 */
void ComputeSumJoint(net)
     NETWORK *net;
{
  long i;
  long index;
  int level = net->numCliques - 1;
  CLUSTER *XU, *XD;
  VECTOR sum;
  EDGE *E;

  while(level-->0) {

    E = net->cedges[level];
    XU = net->cnodes[E->unode];
    XD = net->cnodes[E->dnode];

    if(E->uindices == NULL) {
      E->uindices = (long *) a_calloc(E->uindicesLen, sizeof(long));
      ComputeEdgeIndices(net, XU, E, E->uindices, E->uindicesLen);
    }
      
    if(E->dindices == NULL) {
      E->dindices = (long *) a_calloc(E->dindicesLen, sizeof(long));
      ComputeEdgeIndices(net, XD, E, E->dindices, E->dindicesLen);
    }
      
    bzero(net->nodeStates, net->numNodes*sizeof(int));
      
    for(i=0; i<E->msgLen; i++) {
      ComputeIndex(net, XD->nodes, XD->numNodes, &index);
      ComputePartialSum(net, &sum, XD->probDistr + index, E->dindices, E->dindicesLen);
      ComputeIndex(net, XU->nodes, XU->numNodes, &index);
      ComputeMultiplyPD(net, sum,  XU->probDistr + index, E->uindices, E->uindicesLen);
      ComputeNextState(net, E->nodes, E->numNodes);
    }
  }
}

/*
 * Normalizes the probability distribution of a node to one
 */
void ComputeNormalizeDistr(net, node)
     NETWORK *net;
     int node;
{
  long s;
  VECTOR sum = 0;
  CLUSTER *X = net->cnodes[node];

  for(s=0; s<X->stateSpaceSize; s++) {
    sum += X->probDistr[s];
  }

  Dbg(printf("The node X%d normalization is %10.6f\n", node, sum));

  for(s=0; s<X->stateSpaceSize; s++) {
    X->probDistr[s] /= sum;
  }
}

/*
 * Propagates evidence from one node to another node if necessary
 */
void ComputeUpdateEdge(net, E, direction)
     NETWORK *net;
     EDGE *E;
     int direction;
{
  int i;
  long s;
  long index;
  int node0, node1;
  int numInd0, numInd1;
  long *indices0, *indices1;
  VECTOR sum0, sum1, ratio;
  int edge = E->dnode - 1;
  CLUSTER *X0, *X1;

  if(E->status != direction) return;

  switch(direction) {
  case EDGE_MSG_U2D:
    /* propagate evidence down */
    node0 = E->dnode;
    node1 = E->unode;
    numInd0 = E->dindicesLen;
    numInd1 = E->uindicesLen;
    indices0 = E->dindices;
    indices1 = E->uindices;
    break;
  case EDGE_MSG_D2U:
    /* propagate evidence up */
    node0 = E->unode;
    node1 = E->dnode;
    numInd0 = E->uindicesLen;
    numInd1 = E->dindicesLen;
    indices0 = E->uindices;
    indices1 = E->dindices;
    break;
  default:
    return;
  }

  /* update the probability distribution of X0 by X1 */
  bzero(net->nodeStates, net->numNodes*sizeof(int));

  X0 = net->cnodes[node0];
  X1 = net->cnodes[node1];

  for(s=0; s<E->msgLen; s++) {
    ComputeIndex(net, X1->nodes, X1->numNodes, &index);
    ComputePartialSum(net, &sum1, X1->probDistr + index, indices1, numInd1);
    ComputeIndex(net, X0->nodes, X0->numNodes, &index);
    ComputePartialSum(net, &sum0, X0->probDistr + index, indices0, numInd0);
    ratio = sum0 == 0 ? 0 : sum1 / sum0;
    ComputeMultiplyPD(net, ratio,  X0->probDistr + index, indices0, numInd0);
    ComputeNextState(net, E->nodes, E->numNodes);
  }

  E->status = EDGE_NO_MSG;

  for(i=0; i<X0->numNghb; i++) {
    if(X0->edges[i] == edge) continue;
    E = net->cedges[X0->edges[i]];
    E->status = (E->dnode == node0) ? EDGE_MSG_D2U : EDGE_MSG_U2D;
  }
}

/*
 * Recursively propagates evidence to a node 
 */
void ComputeUpdateNodeRecursive(net, pred, node)
     NETWORK *net;
     int pred;
     int node;
{
  int succ;

  for(succ=0; succ<net->numCliques; succ++) {
    if(!net->ctgt[node][succ]) continue;
    if(pred == succ) continue;
    ComputeUpdateNodeRecursive(net, node, succ);
    if(node > succ) {
      ComputeUpdateEdge(net, net->cedges[node-1], EDGE_MSG_U2D);
    } else {
      ComputeUpdateEdge(net, net->cedges[succ-1], EDGE_MSG_D2U);
    }
  }
}

/*
 * Propagates evidence to a node 
 */
void ComputeUpdateNode(net, node)
     NETWORK *net;
     int node;
{
  ComputeUpdateNodeRecursive(net, EMPTY, node);
}

/*
 * Eliminates entries contradicting evidence from a probability distribition
 */
void ComputeIntroduceEvidence(net, node)
     NETWORK *net;
     int node;
{
  long s;
  int j, k;
  int state, numNodes;
  CLUSTER *X = net->cnodes[node];      
  EDGE *E;

  for(j=numNodes=0; j<X->numNodes; j++) {
    k = X->nodes[j];
    net->nodeStates[k] = 0;
    if(net->nodeMarked[k]) {
      net->nodeMarked[k] = 0;
      net->scratchBuffer[numNodes++] = k;
    }
  }

  Dbg2(printf("Evidence in node X%d ", node), SetDump(net->scratchBuffer, numNodes));

  if(numNodes == 0) return;

  for(s=0; s<X->stateSpaceSize; s++) {
    for(j=0; j<numNodes; j++) {
      k = net->scratchBuffer[j];
      if(net->nodeStates[k] == net->nodeEvidence[k]) continue;
      X->probDistr[s] = 0;
      break;
    }
    ComputeNextState(net, X->nodes, X->numNodes);
  }

  for(j=0; j<X->numNghb; j++) {
    E = net->cedges[X->edges[j]];
    E->status = (E->dnode == node) ? EDGE_MSG_D2U : EDGE_MSG_U2D;
  }
}

/*
 * Marks nodes that are not completely updated
 */
void ComputeMarkDirty(net)
     NETWORK *net;
{
  int i;
  EDGE *E;

  bzero(net->cliqueMarked, net->numCliques*sizeof(int));

  /* mark up */
  for(i=net->numCliques-1; i>0; i--) {
    E = net->cedges[i - 1];
    net->cliqueMarked[E->unode] =
      net->cliqueMarked[E->dnode] || E->status == EDGE_MSG_D2U;
  }

  /* mark down */
  for(i=0; i<net->numCliques-1; i++) {
    E = net->cedges[i];
    if(E->status == EDGE_MSG_D2U) continue;
    net->cliqueMarked[E->dnode] =
      net->cliqueMarked[E->unode] || E->status == EDGE_MSG_U2D;
  }

}


/********************************************************************
  Tree initialization procedures
********************************************************************/

/*
 * Computes the compound value of the noisy-OR probability for the
 * first s states; s should not be less than numValues - 1;
 */ 
VECTOR ComputeNoisyOrCompound(net, node, state)
     NETWORK *net;
     int node, state;
{
  int i, parentState;
  NODE *x = net->nodes + node;
  VECTOR prob;

  if(state < 0) return 0;
  if(state >= x->numValues - 1) return 1;
  
  prob = x->leak[state];
  
  for(i=0; i<x->numParents; i++) {
    parentState = net->nodeStates[x->parentIndices[i]];
    if(parentState > 0) prob *= x->c[i][parentState-1][state];
  }

  return prob;
}

/*
 * Computes the value of a conditional probability given node states
 */
VECTOR ComputeCondProb(net, k)
     NETWORK *net;
     int k;
{
  int i, p;
  NODE *x = net->nodes + k;
  VECTOR probu, probd;

  if(x->numProbs) {
    for(i=p=0; i<x->numParents; i++) {
      p *= net->nodeSizes[x->parentIndices[i]];
      p += net->nodeStates[x->parentIndices[i]];
    }

    p *= net->nodeSizes[k];
    p += net->nodeStates[k];

    return x->probMatrix[p];
  } else {

    probu = ComputeNoisyOrCompound(net, k, net->nodeStates[k]);
    probd = ComputeNoisyOrCompound(net, k, net->nodeStates[k] - 1);

    return (probu - probd);
  }
}

/*
 * Computes contributions to a compound node from marked simple nodes
 */
void ComputeNodeProbDistr(net, node, list, size)
     NETWORK *net;
     int node;
     int *list;
     int size;
{
  long s;
  register i;
  CLUSTER *X = net->cnodes[node];
  VECTOR prod;

  if(X->probDistr == NULL) {
    X->probDistr = (VECTOR *) a_calloc(X->stateSpaceSize, sizeof(VECTOR));
  }

  if(size == 0) {
    for(s=0; s<X->stateSpaceSize; s++) {
      X->probDistr[s] = 1;
    }
  } else {
    bzero((char *) net->nodeStates, net->numNodes*sizeof(int));
    for(s=0; s<X->stateSpaceSize; s++) {
      for(i=0, prod = 1; i<size; i++) {
        prod *= ComputeCondProb(net, list[i]);
      }
      X->probDistr[s] = prod;
      ComputeNextState(net, X->nodes, X->numNodes);
    }
  }

  Dbg2(VectorSum(X->probDistr, X->stateSpaceSize, &prod),
       printf("X%d %10.6f\n", node, prod));
}


/*******************************************************************
  Procedures to store and restore the prior trees
*******************************************************************/

void ComputeStorePriors(net)
     NETWORK *net;
{
  register i;
  CLUSTER *X;
  VECTOR *P;

  assert(!net->priors);

  net->priors = (VECTOR **) a_calloc(net->numCliques, sizeof(VECTOR*));

  for(i=0; i<net->numCliques; i++) {
    if(i != 0 && net->cedges[i - 1]->status != EDGE_NO_MSG)
      ErrorFatal("ComputeStorePriors", "Tree is not updated\n");
    X = net->cnodes[i];
    P = net->priors[i] = (VECTOR *) a_calloc(X->stateSpaceSize, sizeof(VECTOR));
    bcopy(X->probDistr, P, X->stateSpaceSize * sizeof(VECTOR));
  }
}

void ComputeRestoreTree(net)
     NETWORK *net;
{
  register i;
  CLUSTER *X;

  if(net->priors == NULL) return;

  for(i=net->numCliques; i>0; i--) {
    X = net->cnodes[i - 1];
    bcopy(net->priors[i - 1], X->probDistr, X->stateSpaceSize * sizeof(VECTOR));
    if(i > 1) net->cedges[i - 2]->status = EDGE_NO_MSG;
  }
}

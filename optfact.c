/********************************************************************
$RCSfile: optfact.c,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:52:53 $
********************************************************************/
static char rcsid[] = "$Id: optfact.c,v 1.1 1997/10/15 02:52:53 alexvk Exp alexvk $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "network.h"
#include "clique.h"
#include "utils.h"
#include "set.h"

void OptfactPrintQuery(net)
     NETWORK *net;
{
  register i;
  for(i=0; i<net->abNum; i++) {
    if(i==0) {
      printf("p(");
    } else if(i==net->aNum) {
      printf("|");
    } else {
      printf(",");
    }
    printf("x%d", net->ab[i]);
  }
  printf(")\n");
}

void OptfactPrintDecomp(net)
     NETWORK *net;
{
  register i;
  for(i=0; i<net->abNum; i++) {
    if(i==0) {
      printf("p(");
    } else if(i==net->aNum) {
      printf("|");
    } else {
      printf(",");
    }
    printf("x%d", net->ab[i]);
  }
  printf(") x ");
  for(i=net->aNum; i<net->abNum; i++) {
    if(i==net->aNum) {
      printf("p(");
    } else {
      printf(",");
    }
    printf("x%d", net->ab[i]);
  }
  printf(")\n");
}

void OptfactGroupFree(net, node)
     NETWORK *net;
     int node;
{
  CLUSTER *X = net->groups[node];
  if(X->nodes) free ((char *) X->nodes);
  if(X->edges) free ((char *) X->edges);
  if(X->odometer) free ((char *) X->odometer);
  if(X->probDistr) free ((char *) X->probDistr);
  free((char *) X);
  X->numNghb = 0;
  net->groups[node] = NULL;		      
}

void OptfactEdgeFree(net, edge)
     NETWORK *net;
     int edge;
{
  EDGE *E = net->gedges[edge];
  if(E->nodes) free ((char *) E->nodes);
  if(E->origProbs) free ((char *) E->origProbs);
  free((char *) E);
  net->gedges[edge] = NULL;		      
}

void OptfactTreeFree(net)
     NETWORK *net;
{
  int i;

  /* free groups */
  for(i=0; i<net->numGroups; i++) {
    if(net->groups[i]) OptfactGroupFree(net, i);
  }

  /* free edges */
  for(i=0; i<net->numGroups - 1; i++) {
    if(net->gedges[i]) OptfactEdgeFree(net, i);
  }

  net->numGroups = 0;

  /* free indices */
  if(net->uindices) {
    free((char *) net->uindices);
    net->uindices = NULL;
  }
  if(net->dindices) {
    free((char *) net->dindices);
    net->dindices = NULL;
  }

  /* the rest will be set free by NetworkFree(net) */
}

void OptfactEdgesGenerate(net)
     NETWORK *net;
{
  int i, j, k, l, p;
  long maxSumLen = 2;
  CLUSTER *XD, *XU;
  int numNodes, numEdges;
  long maxu = 1;
  long maxd = 1;
  EDGE *E;
   
  for(i=0; i<net->numGroups; i++) {

    XD = net->groups[i];

    for(j=i+1, numEdges=1; j<net->numGroups; j++) {
      if(net->graphTable[i][j]) numEdges++;
    }

    XD->edges = (int *) a_calloc(numEdges, sizeof(int));

    for(j=0; j<i; j++) {

      if(!net->graphTable[i][j]) continue;

      XU = net->groups[j];

      E = net->gedges[i - 1] = (EDGE *) a_calloc(1, sizeof(EDGE));
      E->unode = j;
      E->dnode = i;
      XU->edges[XU->numNghb++] = i - 1;
      XD->edges[XD->numNghb++] = i - 1;
      for(k=p=numNodes=0; k<XU->numNodes; k++) {
        for(l=p; l<XD->numNodes; l++) {
          if(XU->nodes[k] == XD->nodes[l]) {
            net->scratchBuffer[numNodes++] = XU->nodes[k];
            p = l + 1;
            break;
          }
        }
      }
      E->numNodes = numNodes;
      E->nodes = (int *) a_calloc(numNodes, sizeof(int));
      for(k=0, E->msgLen=1; k<numNodes; k++) {
        E->nodes[k] = net->scratchBuffer[k];
        E->msgLen *= net->nodeSizes[E->nodes[k]];
      }

      if(E->msgLen) {
        E->uindicesLen = XU->stateSpaceSize / E->msgLen;
        E->dindicesLen = XD->stateSpaceSize / E->msgLen;
        if(maxu < E->uindicesLen) maxu = E->uindicesLen;
        if(maxd < E->dindicesLen) maxd = E->dindicesLen;
      } else {
        /* we have an overflow problem */
        E->uindicesLen = E->dindicesLen = 0;
      }

      Dbg2(printf("E%d (X%d->(%ld)->X%d) ",
                  i-1, E->unode, E->msgLen, E->dnode),
           SetDump(E->nodes, E->numNodes));

      if(maxSumLen < E->uindicesLen + E->dindicesLen) {
        maxSumLen = E->uindicesLen + E->dindicesLen;
      }

      break;
    }
  }
  /* allocate space for indices which are computed on the fly */
  net->uindices = (long*) a_calloc(maxu, sizeof(long));
  net->dindices = (long*) a_calloc(maxd, sizeof(long));
  net->maxGTIL = maxSumLen;
}

void OptfactGroupProbDistr(net, node)
     NETWORK *net;
     int node;
{
  long s;
  CLUSTER *X = net->groups[node];
  VECTOR sum;

  X->probDistr = (VECTOR*) a_calloc((int)X->stateSpaceSize, sizeof(VECTOR));

  if(X->inclNode == EMPTY) {
    for(s=0; s<X->stateSpaceSize; s++) {
      X->probDistr[s] = 1;
    }
  } else {
    for(s=0; s<X->stateSpaceSize; s++) {
      X->probDistr[s] = ComputeCondProb(net, X->inclNode);
      ComputeNextState(net, X->nodes, X->numNodes);
    }
  }

  Dbg2(VectorSum(X->probDistr, X->stateSpaceSize, &sum),
       printf("X%d [%d] %10.6f\n", node, X->inclNode, sum));
}

int OptfactFindIncludedProbs(net, level)
     NETWORK *net;
     int level;
{
  register i, j;
  int retVal = 0;
  EDGE *E = net->gedges[level];
  CLUSTER *X = net->groups[E->dnode];
  int *nodes = E->origProbs;

  if(X->inclNode != EMPTY) {
    nodes[retVal++] = X->inclNode;
  }
  for(i=0; i<X->numNghb; i++) {
    if(X->edges[i] != level) {
      if(net->gedges[X->edges[i]]->numOrigProbs > 0) {
        for(j=0; j < net->gedges[X->edges[i]]->numOrigProbs; j++) {
          nodes[retVal++] = net->gedges[X->edges[i]]->origProbs[j];
        }
      }
    }
  }
  return retVal;
}

void OptfactCountProbs(net)
     NETWORK *net;
{
  int i;
  int level = net->numGroups - 1;
  CLUSTER *X;
  EDGE *E;
  int numOrigProbs;

  for(i=0; i<net->numGroups; i++) {
    X = net->groups[i];
    X->numProbs=(X->inclNode != EMPTY) ? 1 : 0;
  }

  while(level-->0) {
    E = net->gedges[level];
    net->groups[E->unode]->numProbs += net->groups[E->dnode]->numProbs;
    int expOrigProb = net->groups[E->dnode]->numProbs;
    E->origProbs = (int *) a_calloc(expOrigProb, sizeof(int));
    E->numOrigProbs = OptfactFindIncludedProbs(net, level);
    assert(E->numOrigProbs == expOrigProb);
  }
}

void OptfactSumFactors(net)
     NETWORK *net;
{
  long i, index;
  int level = net->numGroups - 1;
  CLUSTER *XU, *XD;
  VECTOR sum;
  EDGE *E;

  /* indices are used only once and discarded! */

  while(level-->0) {

    E = net->gedges[level];
    XU = net->groups[E->unode];
    XD = net->groups[E->dnode];

    if(E->uindicesLen == 1) {
      net->uindices[0] = 0;
    } else {
      ComputeEdgeIndices(net, XU, E, net->uindices, E->uindicesLen);
    }
      
    if(E->dindicesLen == 1) {
      net->dindices[0] = 0;
    } else {
      ComputeEdgeIndices(net, XD, E, net->dindices, E->dindicesLen);
    }
      
    if(XU->probDistr == NULL) {
      OptfactGroupProbDistr(net, E->unode);
    }

    if(XD->probDistr == NULL) {
      OptfactGroupProbDistr(net, E->dnode);
    }

    for(i=0; i<E->msgLen; i++) {
      ComputeIndex(net, XD->nodes, XD->numNodes, &index);
      ComputePartialSum(net, &sum, XD->probDistr + index, net->dindices, E->dindicesLen);
      ComputeIndex(net, XU->nodes, XU->numNodes, &index);
      ComputeMultiplyPD(net, sum,  XU->probDistr + index, net->uindices, E->uindicesLen);
      ComputeNextState(net, E->nodes, E->numNodes);
    }

    Dbg2(VectorSum(XU->probDistr, XU->stateSpaceSize, &sum),
         printf("X%d %10.6f\n", E->unode, sum));

    /* we do not need XD or E any longer */
    OptfactGroupFree(net, E->dnode);
    OptfactEdgeFree(net, level);
  }

  /* check if the clique 0 has been computed */
  if(net->groups[0]->probDistr == NULL) {
    OptfactGroupProbDistr(net, 0);
  }

}

void OptfactQuery(net)
     NETWORK *net;
{
  register i;
  /* mark barren nodes dirty */
  for(i=0; i<net->abNum; i++) {
    assert(net->ab[i]>0);
    assert(net->ab[i]<net->numNodes);
  }
  BNSearch(net, net->ab, net->abNum);
  GraphNetworkNew(net);
  GraphMoralize(net);
  GraphMakeClique(net, net->ab, net->abNum);
  CliqueTreeGenerate(net, ALG_OPTFACT);
  if(net->numGroups) SUMFACTORS(net);
}

void OptfactDumpResult(net)
     NETWORK *net;
{
  long i, j;
  long index;
  long *indices;
  size_t numA, numB;
  CLUSTER *X0 = net->groups[0];
  VECTOR sum;

  /* with evidence, we need to normailize */
  VectorNormalize(X0->probDistr, X0->stateSpaceSize);

  if(net->aNum < net->abNum) {

    for(i=0, numA=numB=1; i<net->abNum; i++) {
      if(i < net->aNum) {
        numA *= net->nodeSizes[net->ab[i]];
      } else {
        numB *= net->nodeSizes[net->ab[i]];
      }
    }

    indices = (long*) a_calloc(numA, sizeof(long));
    bzero(net->nodeStates, net->numNodes*sizeof(int));
    for(i=0; i<numA; i++) {
      ComputeIndex(net, X0->nodes, X0->numNodes, indices + i);
      ComputeNextState(net, net->ab, net->aNum);
    }

    for(i=0; i<numB; i++) {
      ComputeIndex(net, X0->nodes, X0->numNodes, &index);
      ComputePartialSum(net, &sum, X0->probDistr + index, indices, numA);
      if(sum > 0) {
        for(j=0; j<numA; j++) {
          printf("%6.4lf ", X0->probDistr[index + indices[j]] / sum);
        }
      } else {
        for(j=0; j<numA; j++) {
          printf(" undef ");
        }
      }
      printf("x %6.4lf\n", sum);
      ComputeNextState(net, net->ab + net->aNum, net->abNum - net->aNum);
    }

    free((char *) indices);
  } else {
    VectorDump(X0->probDistr, X0->stateSpaceSize);
  }
}

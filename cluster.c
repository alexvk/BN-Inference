/********************************************************************
$RCSfile: cluster.c,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:50:15 $
********************************************************************/
static char rcsid[] = "$Id: cluster.c,v 1.1 1997/10/15 02:50:15 alexvk Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cluster.h"
#include "network.h"
#include "utils.h"
#include "set.h"

/*******************************************************************
  Cluster tree procedures
*******************************************************************/

void ClusterNodeNew(net, XP, nodes, numNodes)
     NETWORK *net;
     CLUSTER **XP;
     int *nodes;
     int numNodes;
{
  int i, ss;
  CLUSTER *X = (CLUSTER *) a_calloc(1, sizeof(CLUSTER));
   
  X->numNodes = numNodes;
  X->nodes = (int *) a_calloc(numNodes, sizeof(int));
  X->odometer = (int *) a_calloc(numNodes, sizeof(int));
  for(i=numNodes, ss=1; i>0; i--) {
    X->odometer[i - 1] = ss;
    ss *= net->nodeSizes[nodes[i - 1]];
  }
  X->stateSpaceSize = ss;
  bcopy(nodes, X->nodes, numNodes*sizeof(int));

  Dbg2(printf("X %d ", X->stateSpaceSize), SetDump(X->nodes, X->numNodes));
  *XP = X;
}

void ClusterEdgesGenerate(net)
     NETWORK *net;
{
  int i, j, k, l, p;
  int numNodes, numEdges;
  int maxSumLen = 2;
  CLUSTER *XD, *XU;
  EDGE *E;
   
  /* create the graph table in net->ctgt */
  GraphNew(&net->ctgt, net->numCliques);
  GraphTreeTable(net, net->cnodes, net->ctgt, net->numCliques);
  Dbg2(printf("Join tree table graph:\n"),
       GraphTableDump(net->ctgt, net->numCliques));

  net->cedges = (EDGE **) a_calloc(net->numCliques-1, sizeof(EDGE*));
   
  /* connect corresponding edges and nodes */
  for(i=0; i<net->numCliques; i++) {

    XD = net->cnodes[i];

    for(j=i+1, numEdges=1; j<net->numCliques; j++) {
      if(net->ctgt[i][j]) numEdges++;
    }

    XD->edges = (int *) a_calloc(numEdges, sizeof(int));

    for(j=0; j<i; j++) {
      if(!net->ctgt[i][j]) continue;
      XU = net->cnodes[j];
      E = net->cedges[i - 1] = (EDGE *) a_calloc(1, sizeof(EDGE));
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

      E->uindicesLen = XU->stateSpaceSize / E->msgLen;
      E->dindicesLen = XD->stateSpaceSize / E->msgLen;

      Dbg2(printf("E%d (X%d->(%d)->X%d) ",
                  i-1, E->unode, E->msgLen, E->dnode),
           SetDump(E->nodes, E->numNodes));

      if(maxSumLen < E->uindicesLen + E->dindicesLen) {
        maxSumLen = E->uindicesLen + E->dindicesLen;
      }

      break;
    }
  }

  Dbg(printf("max indices length sum %d\n", maxSumLen));
  net->maxCTIL = maxSumLen;
}

void ClusterTreeGenerate(net)
     NETWORK *net;
{
  int i;

  if(net->numCliques) return;
  /* all nodes should be clean at this point */
  GraphNetworkNew(net);
  GraphMoralize(net);
  Dbg2(printf("Belief net:\n"), GraphTableDump(net->graphTable, net->numNodes));
  CliqueTreeGenerate(net, ALG_CLUSTER);
}

void ClusterPrintIndent(net, node)
     NETWORK *net;
     int node;
{
  while(node) {
    node = net->cedges[node - 1]->unode;
    printf(" ");
  }
}


void ClusterNodeDump(net, node)
     NETWORK *net;
     int node;
{
  CLUSTER *X = net->cnodes[node];
  ClusterPrintIndent(net, node);
  printf("X%d ", node);
  SetDisplay(X->nodes, X->numNodes);
  printf(" %d ", X->stateSpaceSize);
  SetDisplay(X->edges, X->numNghb);
  printf("\n");
}

void ClusterEdgeDump(net, edge)
     NETWORK *net;
     int edge;
{
  EDGE *E = net->cedges[edge];
  ClusterPrintIndent(net, E->unode);
  printf("E%d (X%d->X%d) ", edge, E->unode, E->dnode);
  SetDisplay(E->nodes, E->numNodes);
  printf(" %d (%d %d)", E->msgLen, E->uindicesLen, E->dindicesLen);
  switch (E->status) {
  case EDGE_NO_MSG:
    printf("\n");
    break;
  case EDGE_MSG_D2U:
    printf(" message up\n");
    break;
  case EDGE_MSG_U2D:
    printf(" message down\n");
    break;
  }
}

void ClusterDumpProb (net, node)
     NETWORK *net;
     int node;
{
  CLUSTER *X = net->cnodes[node];
  VectorDump(X->probDistr, X->stateSpaceSize);
}

void ClusterTreeDisplay (net)
     NETWORK *net;
{
  int i;
  if(net->numCliques) {
    printf("Join tree:\n");
    for(i=0; i<net->numCliques; i++) {
      if(i) ClusterEdgeDump(net, i - 1);
      ClusterNodeDump(net, i);
    }
  }
}

void ClusterNodeFree(net, node)
     NETWORK *net;
     int node;
{
  CLUSTER *X = net->cnodes[node];
  if(X->nodes) free ((char *) X->nodes);
  if(X->odometer) free ((char *) X->odometer);
  if(X->probDistr) free ((char *) X->probDistr);
  if(X->edges) free ((char *) X->edges);
  free((char *) X);
  net->cnodes[node] = NULL;		      
}

void ClusterEdgeFree(net, edge)
     NETWORK *net;
     int edge;
{
  EDGE *E = net->cedges[edge];
  if(E->nodes) free ((char *) E->nodes);
  if(E->dindices) free((char *) E->dindices);
  if(E->uindices) free((char *) E->uindices);
  free((char *) E);
  net->cedges[edge] = NULL;		      
}

void ClusterTreeFree(net)
     NETWORK *net;
{
  int i;

  /* free the helper array */
  if(net->cliqueMarked)
    free ((char *) net->cliqueMarked);
  net->cliqueMarked = NULL;

  /* free cluster nodes */
  if(net->cnodes) {
    for(i=0; i<net->numCliques; i++) {
      if(net->cnodes[i]) ClusterNodeFree(net, i);
    }
    free ((char *) net->cnodes);
    net->cnodes = NULL;
  }

  /* free priors */
  if(net->priors) {
    for(i=0; i<net->numCliques; i++) {
      if(net->priors[i]) free ((char *) net->priors[i]);
    }
    free ((char *) net->priors);
    net->priors = NULL;
  }

  /* free edges */
  if(net->cedges) {
    for(i=0; i<net->numCliques-1; i++) {
      if(net->cedges[i]) ClusterEdgeFree(net, i);
    }
    free ((char *) net->cedges);
    net->cedges = NULL;
  }

  /* free the graph table */
  if(net->ctgt)
    GraphFree(&net->ctgt, net->numCliques);

  net->numCliques = 0;
}


/*******************************************************************
  Procedures for probabilistic inference
*******************************************************************/

/*
 * makes a tree traversal to introduce evidence
 */
void ClusterIntroduceEvidenceRecursive(net, pred, node)
     NETWORK *net;
     int pred;
     int node;
{
  int succ;
   
  if(net->cliqueMarked[node]) {
    UPDATENODE(net, node);
    EVIDENCE(net, node);
  }
   
  for(succ=0; succ<net->numCliques; succ++) {
    if(!net->ctgt[node][succ]) continue;
    if(pred == succ) continue;
    ClusterIntroduceEvidenceRecursive(net, node, succ);
  }
}

/*
 * A wrapper function to introduce all evidence
 */
void ClusterTreeIntroduceEvidence(net)
     NETWORK *net;
{
  int i, newEvid;

  Dbg2(printf("Instantiated nodes "), SetDump(net->nodeMarked, net->numNodes));
  bzero(net->cliqueMarked, net->numCliques*sizeof(int));
  for(i=newEvid=0; i<net->numNodes; i++) {
    if(net->nodeMarked[i]) {
      net->cliqueMarked[net->nodeCliqueDFS[i]] = newEvid = 1;
    }
  }
  Dbg2(printf("Instantiated clusters "), SetDump(net->cliqueMarked, net->numCliques));
  if(newEvid) ClusterIntroduceEvidenceRecursive(net, EMPTY, 0);

}

void ClusterTreeSingleNodeQuery(net, k)
     NETWORK *net;
     int k;
{
  ClusterTreeIntroduceEvidence(net);
  UPDATENODE(net, net->nodeCliqueDFS[k]);
  BELIEFS(net, net->nodeCliqueDFS[k], k);
}

void ClusterTreeUpdateBeliefs(net)
     NETWORK *net;
{
  int i, k, node;
  int probEvidSet = 0;
  CLUSTER *X;

  for(k=0; k<net->numNodes; k++) {
    net->nodes[k].dirty = TRUE;
  }

  ComputeMarkDirty(net);

  for(node=net->numCliques; node>0; node--) {
    if(net->cliqueMarked[node - 1]) continue;
    X = net->cnodes[node - 1];
    for(i=0; i<X->numNodes; i++) {
      if(!net->nodes[k = X->nodes[i]].dirty) continue;
      Dbg(printf("Updating beliefs for x%d in X%d ... \n", i, node));
      BELIEFS(net, node - 1, k);
      net->nodes[k].dirty = FALSE;
      if(probEvidSet) continue;
      if(net->nodeMarked[k]) continue;
      if(net->nodeEvidence[k] == NOTSET) continue;
      probEvid = net->nodes[k].beliefs[net->nodeEvidence[k]];
      probEvidSet = 1;
    }
  }
}

void ClusterTreeUpdate(net)
     NETWORK *net;
{
  int i;
  EDGE *E;

  ClusterTreeIntroduceEvidence(net);

  Dbg(printf("Propagating up ... \n"));

  /* propagate evidence up */
  for(i=net->numCliques-1; i>0; i--) {
    E = net->cedges[i - 1];
    if(E->status != EDGE_MSG_D2U) continue;
    UPDATEEDGE(net, E, EDGE_MSG_D2U);
  }

  Dbg(printf("Propagating down ... \n"));

  /* propagate evidence down */
  for(i=0; i<net->numCliques-1; i++) {
    E = net->cedges[i];
    if(E->status != EDGE_MSG_U2D) continue;
    UPDATEEDGE(net, E, EDGE_MSG_U2D);
  }
}

/*******************************************************************
  Procedures to initialize the tree
*******************************************************************/

int ClusterParentsIn(net, node, k)
     NETWORK *net;
     int node;
     int k;
{
  int i;
  CLUSTER *X = net->cnodes[node];
  NODE *x = net->nodes + k;

  for(i=0; i<x->numParents; i++) {
    if(SetMember(x->parentIndices[i], X->nodes, X->numNodes)) continue;
    return FALSE;
  }

  return TRUE;
}
      
void ClusterTreeInit(net)
     NETWORK *net;
{
  int i, j, k, num;
  CLUSTER *X;
  EDGE *E;

  /* mark the nodes to be included in cliques */
  for(i=0; i<net->numNodes; i++) {
    net->nodeMarked[i] = NOTSET;
  }
  for(i=net->numCliques; i>0; i--) {
    X = net->cnodes[i - 1];
    for(j=num=0; j<X->numNodes; j++) {
      k = X->nodes[j];
      if(net->nodeMarked[k] != NOTSET) continue;
      if(ClusterParentsIn(net, i - 1, k)) {
        Dbg(printf("Node x%d included in X%d\n", k, i - 1));
        net->nodeMarked[k] = i - 1;
        net->scratchBuffer[num++] = k;
      }
    }
#ifdef MULTIPROC
  }

  CompparSumJoint(net);

#else
    ComputeNodeProbDistr(net, i-1, net->scratchBuffer, num);

  }

  ComputeSumJoint(net);

#endif /* MULTIPROC */

/* update all nodes */
  CompparMeasBeg();
  for(i=0; i<net->numCliques - 1; i++) {
    E = net->cedges[i];
    UPDATEEDGE(net, E, E->status = EDGE_MSG_U2D);
  }
  CompparMeasEnd();

/* initialize evidence */
  NetworkResetEvidence(net);
}

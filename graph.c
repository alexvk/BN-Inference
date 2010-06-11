/*******************************************************************
$RCSfile: graph.c,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:52:53 $
********************************************************************/
static char rcsid[] = "$Id: graph.c,v 1.1 1997/10/15 02:52:53 alexvk Exp alexvk $";

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network.h"
#include "cluster.h"
#include "utils.h"
#include "graph.h"

/*******************************************************************
  Graph routines
*******************************************************************/

void GraphNew(graphTable, size)
     char ***graphTable;
     int size;
{
  int i;
  char **gt;

  gt = *graphTable = (char **) a_calloc (size, sizeof(char *));
  for (i = 0; i < size; i++) {
    gt[i] = a_calloc (size, sizeof(char));
  }
}

void GraphZero(graphTable, size)
     char **graphTable;
     int size;
{
  int i;
  for (i = 0; i < size; i++) {
    bzero(graphTable[i], size*sizeof(char));
  }
}

void GraphNetworkNew(net)
     NETWORK *net;
{
  int i, j;
  NODE *node;

  assert(net->graphTable);

  GraphZero(net->graphTable, net->numNodes);
    
  for (i = 0; i < net->numNodes; i++) {
    node=net->nodes + i;
    if(node->dirty) continue;
    for (j = 0; j < node->numParents; j++) {
      if(net->nodes[node->parentIndices[j]].dirty) continue;
      net->graphTable[i][node->parentIndices[j]] = 1;
      net->graphTable[node->parentIndices[j]][i] = 1;
    }
    /* a bit redundant, but it can't hurt */
    for (j = 0; j < node->numChildren; j++) {
      if(net->nodes[node->childIndices[j]].dirty) continue;
      net->graphTable[i][node->childIndices[j]] = 1;
      net->graphTable[node->childIndices[j]][i] = 1;
    }
  }
}

void GraphTreeTable(net, clusters, graphTable, num)
     NETWORK *net;
     CLUSTER **clusters;
     char **graphTable;
     int num;
{
  int i, j, numNodes;
  CLUSTER *CD, *CU;
  SET nodes;

  SetInit(net->numNodes, &nodes);
   
  for(i=numNodes=0; i<num; i++) {
    CD = clusters[i];
    SetMakeEmpty(&nodes);
    for(j=0; j<CD->numNodes; j++) {
      if(SetMember(CD->nodes[j], net->scratchBuffer, numNodes)) {
        SetAddNode(&nodes, CD->nodes[j]);
      } else {
        net->scratchBuffer[numNodes++] = CD->nodes[j];
      }
    }
      
    if(i==0) continue;
      
    /* find the parent of this clique */
    for(j=i; j>0; j--) {
      CU = clusters[j - 1];
      if(SetIncluded(&nodes, CU->nodes, CU->numNodes)) break;
    }

    if(j==0) ErrorFatal("GraphTreeTable", "No parent for clique C%d\n", i);

    graphTable[i][j-1] = graphTable[j-1][i] = 1;
  }

  SetFree(&nodes);
}

void GraphFree(graphTable, size)
     char ***graphTable;
     int size;
{
  int i;
  char **gt = *graphTable;
  for (i = 0; i < size; i++) {
    free ((char *) gt[i]);
  }
  free ((char *) gt);
  *graphTable = NULL;
}

void GraphTableDump(graphTable, size)
     char **graphTable;
     int size;
{
  int i,j,cnt = 0;

  if(size > 80) return;

  for (i = 0; i<size; i++) {
    for (j = 0; j<size; j++) {
	    printf("%d ",graphTable[i][j]);
	    cnt += graphTable[i][j];
    }
    printf("\n");
  }
  printf("Total Links = %d\n",cnt/2);
}

void GraphMakeClique(net, list, size)
     NETWORK *net;
     int *list;
     int size;
{
  int i, j;

  for(i=0; i<size; i++) {
    for(j=0; j<i; j++) {
      net->graphTable[list[i]][list[j]] = 1;
      net->graphTable[list[j]][list[i]] = 1;
    }
  }
}

void GraphMoralize(net)
     NETWORK *net;
{
  int i, j;
  NODE *node;
    
  for(i = 0; i < net->numNodes; i++) {
    node = net->nodes + i;
    if(node->dirty) continue;
    if(node->numParents < 2) continue;
    GraphMakeClique(net, node->parentIndices, node->numParents);
  }
}

void GraphTriangulate(net, invOrder)
     NETWORK *net;
     int *invOrder;
{
  int v, w, x, i, j;
  int *follower, *index;
  int lastNode = net->numNodes;
    
  follower = (int *) calloc (lastNode, sizeof(int));
  index = (int *) calloc (lastNode, sizeof(int));
  while(invOrder[--lastNode] == NOTSET);
  for(i=lastNode; i>=0; i--) {
    w = invOrder[i];
    follower[w] = w;
    index[w] = i;
    for(j=i; j<lastNode; j++) {
      v = invOrder[j+1];
      if(net->graphTable[v][w]) {
        x = v;
        while(index[x] > i) {
          index[x] = i;
          if (!net->graphTable[x][w]) {
            net->graphTable[x][w] = 1;
            net->graphTable[w][x] = 1;
          }
          x = follower[x];
        }
        if(follower[x] == x) follower[x] = w;
      }
    }
  }
  free ((char *) follower);
  free ((char *) index);
}

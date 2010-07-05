/********************************************************************
$RCSfile: decompose.c,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:52:53 $
********************************************************************/
static char rcsid[] = "";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network.h"
#include "utils.h"

#define		MAX_NODES	1000

VECTOR zero = 0;
VECTOR one = 1;
char* fstr = "false";
char* tstr = "true";

static NETWORK *gNet;

int DecomposeNodeCompare(a, b)
     int *a, *b;
{
  return (gNet->nodes[*a].marked - gNet->nodes[*b].marked);
}

void DecomposeNode(net, node, beg, end)
     NETWORK *net;
     int node, beg, end;
{
  int i, j, k;
  char str[120];
  NODE *x0 = net->nodes + node;
  NODE *x1 = net->nodes + net->numNodes;
  int fail;
  NODE *xp;

  bcopy(x0, x1, sizeof(NODE));

  /* form the new node name */
  sprintf(str, "%s-%#x", x0->nodeName, x0->numParents);
  x1->nodeName = (char*) calloc(strlen(str) + 1, sizeof(char));
  strcpy(x1->nodeName, str);
  x1->numParents = end - beg;
  x1->parentIndices = (int*) calloc(x1->numParents, sizeof(int));
  bcopy(x0->parentIndices+beg, x1->parentIndices, x1->numParents*sizeof(int));

  /* check if the additional node can be reduced to binary */
  xp = net->nodes + x1->parentIndices[0];
  fail = (xp->numValues != 2);
  for(i=1; !fail && i<x1->numParents; i++) {
    xp = net->nodes + x1->parentIndices[i];
    fail = (xp->numValues != 2);
    for(k=1; !fail && k<x0->numValues; k++) {
      fail = x0->c[beg][0][k-1] != x0->c[beg+i][0][k-1];
    }
  }

  if(fail) {

    Dbg(printf("Creating new node %s\n", x1->nodeName));

    /* leaks will be in the child, set to zero */
    x1->leak = (VECTOR*) calloc(x1->numValues-1, sizeof(VECTOR));
    for(i=1; i<x1->numValues; i++) {
      x1->leak[i-1] = 1;
    }

    /* form the noisy-OR coeffitients for the parents */
    x1->c = (VECTOR***) calloc(x1->numParents, sizeof(VECTOR**));
    for(i=0; i<x1->numParents; i++) {
      x1->c[i] = x0->c[beg+i];
      x0->c[beg+i] = NULL;
    }

    /* free the noisy-OR coefficients for this parent */
    if(x0->c[x0->numParents]) {
      xp = net->nodes + x0->parentIndices[x0->numParents];
      for(k=1; k<xp->numValues; k++) {
        free((char*) x0->c[x0->numParents][k-1]);
      }
      free((char*) x0->c[x0->numParents]);
    }

    /* form the noisy-OR coeffitients for the child */
    x0->c[x0->numParents] = (VECTOR**) calloc(x1->numValues-1, sizeof(VECTOR*));
    for(j=1; j<x1->numValues; j++) {
      x0->c[x0->numParents][j-1] = (VECTOR*) calloc(x0->numValues-1, sizeof(VECTOR));
      for(k=1; k<x0->numValues; k++) {
        x0->c[x0->numParents][j-1][k-1] = (k <= j) ? zero : one;
      }
    }

  } else {

    Dbg(printf("Creating new binary node %s\n", x1->nodeName));

    x1->numValues = 2;
    x1->nodeLabels = (char**) calloc(2, sizeof(char*));
    x1->nodeLabels[0] = fstr;
    x1->nodeLabels[1] = tstr;
    x1->leak = &one;
    x1->c = (VECTOR***) calloc(x1->numParents, sizeof(VECTOR**));
    for(i=0; i<x1->numParents; i++) {
      x1->c[i] = (VECTOR **) malloc(sizeof(VECTOR*));
      x1->c[i][0] = (VECTOR *) malloc(sizeof(VECTOR));
      x1->c[i][0][0] = 0;
    }
    if(x0->numParents != beg) {
      /* free the noisy-OR coefficients for this parent */
      if(x0->c[x0->numParents]) {
        xp = net->nodes + x0->parentIndices[x0->numParents];
        for(k=1; k<xp->numValues; k++) {
          free((char*) x0->c[x0->numParents][k-1]);
        }
        free((char*) x0->c[x0->numParents]);
      }
      x0->c[x0->numParents] = x0->c[beg];
      x0->c[beg] = NULL;
    }

  }

  /* connect the new node to the old */
  x0->parentIndices[x0->numParents++] = net->numNodes++;

}

void DecomposeUsageMessage(s)
     char *s;
{
  printf("usage: %s networkFile max_parents\n",s);
}

int main(argc, argv)
     int argc;
     char **argv;
{
  int i, j, k;
  int pos, maxp, nump, numv = 0;
  int order[MAX_NODES];
  VECTOR **c[MAX_NODES];
  NETWORK *net;
  NODE *x;
  
  if (argc < 3) {
    DecomposeUsageMessage(argv[0]);
    return(1);
  }
  
  NetworkNew(&net);
  net->algorithm = ALG_UNKNOWN;
  net->netName = argv[1];
  NetworkFileReadErgo(net, net->netName);
  net->nodes = (NODE*) realloc(net->nodes, MAX_NODES*sizeof(NODE));
  GraphNew(&net->graphTable, net->numNodes);
  GraphNetworkNew(net);

  /*
    GraphMoralize(net);
    POSearch(net);
    net->scratchBuffer = order;
    MCSearch(net, &numv, 1);
    for(i=0; i<net->numNodes; i++) {
    if(net->nodes[i].marked == NOTSET) continue;
    order[net->nodes[i].marked] = i;
    }
    GraphTriangulate(net, order);
  */

  BFSearch(net, net->graphTable, 0);

  maxp = atoi(argv[2]); 
  gNet = net;
 
  for(i=0; i<net->numNodes; i++) {
    x = net->nodes + i;

    if(!x->c || x->numParents <= maxp) continue;

    /* sort the nodes in some order */
    bcopy(x->parentIndices, order, x->numParents*sizeof(int));
    qsort(order, x->numParents, sizeof(int), (CMPFUN) DecomposeNodeCompare);
    for(j=0; j<x->numParents; j++) {
      SetMemberPos(x->parentIndices[j], order, x->numParents, &pos);
      c[pos] = x->c[j];
    }
    bcopy(c, x->c, x->numParents*sizeof(VECTOR**));
    bcopy(order, x->parentIndices, x->numParents*sizeof(int));

    nump = x->numParents;
    x->numParents = 0;
    Dbg2(printf("Node x%d ", i), SetDump(x->parentIndices, nump));

    /*
      DecomposeNode(net, i, 0, maxp);
      DecomposeNode(net, i, maxp, nump);
    */

    DecomposeNode(net, i, 0, nump/2);
    DecomposeNode(net, i, nump/2, nump);

    /* free noisy-OR coeffitients for the rest of parents */
    for(j=2; j<nump; j++) {
      if(x->c[j]) {
        numv = net->nodes[x->parentIndices[j]].numValues;
        for(k=1; k<numv; k++) {
          free((char*) x->c[j][k-1]);
        }
        free((char*) x->c[j]);
      }
    }

    if(net->numNodes < MAX_NODES - 1) continue;
    printf("Increase MAX_NODES (%d)!\n", MAX_NODES);
    break;
  }

  NetworkFileWriteErgo(net, stdout);

}

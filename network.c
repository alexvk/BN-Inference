/********************************************************************
$RCSfile: network.c,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:52:53 $
********************************************************************/
static char rcsid[] = "$Id: network.c,v 1.1 1997/10/15 02:52:53 alexvk Exp alexvk $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "network.h"
#include "utils.h"

VECTOR probEvid;

void NetworkNew(new)
     NETWORK **new;
{
  *new = a_calloc (1, sizeof(NETWORK));
}

void NetworkFreeNodes(net)
     NETWORK *net;
{
  int i, j, k;
  NODE *xp;

  for (i = 0; i < net->numNodes; i++) {
    if(net->nodes[i].c) {
      free((char*) net->nodes[i].leak);
      for(j=0; j<net->nodes[i].numParents; j++) {
        xp = net->nodes + net->nodes[i].parentIndices[j];
        for(k=1; k<xp->numValues; k++) {
          free((char*) net->nodes[i].c[j][k-1]);
        }
        free((char*) net->nodes[i].c[j]);
      }
      free((char*) net->nodes[i].c);
    }
    if (net->nodes[i].nodeName)
      free((char *) net->nodes[i].nodeName);
    if (net->nodes[i].beliefs)
      free((char *) net->nodes[i].beliefs);
    if (net->nodes[i].parentIndices)
      free((char *) net->nodes[i].parentIndices);
    if (net->nodes[i].childIndices)
      free((char *) net->nodes[i].childIndices);
    if (net->nodes[i].probMatrix)
      free((char *) net->nodes[i].probMatrix);
    if (net->nodes[i].odometer)
      free((char *) net->nodes[i].odometer);
    if (net->nodes[i].nodeLabels) {
      for (j = 0; j < net->nodes[i].numValues; j++)
        free((char *) net->nodes[i].nodeLabels[j]);
      free((char *) net->nodes[i].nodeLabels);
    }
  }
  free((char *) net->nodes);
}

void NetworkFree(net)
     NETWORK *net;
{
  /* net->netName is argv[1] */
  if (net->nodes)
    NetworkFreeNodes(net);
  if (net->graphTable)
    GraphFree(&net->graphTable, 2*net->numNodes);
  if (net->nodeSizes)
    free((char *) net->nodeSizes);
  if (net->nodeStates)
    free((char *) net->nodeStates);
  if (net->nodeMarked)
    free((char *) net->nodeMarked);
  if (net->nodeCliqueDFS)
    free((char *) net->nodeCliqueDFS);
  if (net->scratchBuffer)
    free((char *) net->scratchBuffer);
  if (net->nodeEvidence)
    free((char *) net->nodeEvidence);

  ClusterTreeFree(net);

  OptfactTreeFree(net);

  if (net->ab)
    free((char *) net->ab);
  if (net->groups)
    free((char *) net->groups);
  if (net->gedges)
    free((char *) net->gedges);

  free((char *) net);
}

void NetworkAnalyze(net)
     NETWORK *net;
{
  int i, j;
  NODE *x;
    
  /* allocate belief vectors */
  for (i = 0; i < net->numNodes; i++) {
    x = net->nodes + i;
    if (!x->beliefs) x->beliefs =
                       (VECTOR *) a_calloc(x->numValues, sizeof(VECTOR));
  }

  /* allocate the graph table */
  if (!net->graphTable)
    GraphNew(&net->graphTable, 2*net->numNodes);
    
  /* set up helper arrays */
  if (!net->nodeSizes)
    net->nodeSizes = (int *) a_calloc(net->numNodes,sizeof(int));
  if (!net->nodeStates)
    net->nodeStates = (int *) a_calloc(net->numNodes,sizeof(int));
  if (!net->nodeMarked)	
    net->nodeMarked = (int *) a_calloc(net->numNodes,sizeof(int));
  if (!net->nodeCliqueDFS)
    net->nodeCliqueDFS = (int *) a_calloc(net->numNodes,sizeof(int));
  if (!net->scratchBuffer)
    net->scratchBuffer = (int *) a_calloc(net->numNodes,sizeof(int));
  if (!net->nodeEvidence)
    net->nodeEvidence = (int *) a_calloc(net->numNodes,sizeof(int));
  if (!net->ab)
    net->ab = (int *) a_calloc(net->numNodes,sizeof(int));
  if (!net->groups)
    net->groups = (CLUSTER **) a_calloc(2*net->numNodes,sizeof(CLUSTER*));
  if (!net->gedges)
    net->gedges = (EDGE **) a_calloc(2*net->numNodes,sizeof(EDGE*));

  for (i = 0; i < net->numNodes; i++) {
    x = net->nodes + i;
    net->nodeSizes[i] = x->numValues;
    net->nodeEvidence[i] = NOTSET;
    for(j=0; j<x->numParents; j++)
      net->scratchBuffer[j] = net->nodes[x->parentIndices[j]].numValues;
    net->scratchBuffer[j++] = x->numValues;
    x->odometer = (int *) a_calloc(j, sizeof(int));
    OdometerInitialize(x->odometer, net->scratchBuffer, j);
  }
}

void NetworkGetNodeValByName(net, nodeName, result)
     NETWORK *net;
     char *nodeName;
     int *result;
{
  int i;
  NODE *node;

  *result = EMPTY;
  for (i = 0; i < net->numNodes; i++) {
    node = net->nodes + i;
    if (!strcmp(nodeName, node->nodeName)) {
	    *result = i;
	    break;
    }
  }
}

void NetworkGetNodeValIndex(net, node, valName, result)
     NETWORK *net;
     NODE *node;
     char *valName;
     int *result;
{
  int i;
  *result = EMPTY;
  for (i = 0; i < node->numValues; i++) {
    if (!strcmp(node->nodeLabels[i], valName)) {
	    *result = i;
	    break;
    }
  }
}	

void NetworkResetEvidence(net)
     NETWORK *net;
{
  int i;
  for (i = 0; i < net->numNodes; i++) {
    net->nodeMarked[i] = FALSE;
    net->nodeEvidence[i] = NOTSET;
  }
  probEvid = 1;
}

void NetworkDumpEvidence(net)
     NETWORK *net;
{
  int i;
  NODE *node;
  for (i = 0; i < net->numNodes; i++) {
    node = net->nodes + i;
    if (net->nodeEvidence[i] != NOTSET)
	    printf("%s set to %s%s\n", node->nodeName,
             node->nodeLabels[net->nodeEvidence[i]],
             net->nodeMarked[i] ? " (new)" : "");
  }
}

void NetworkDumpCondProbs(net)
     NETWORK *net;
{
  int i, j, k;
  NODE *node;
  for (i = 0; i < net->numNodes; i++) {
    node = net->nodes + i;
    printf("Node: %s\n", node->nodeName);
    for (j = 0; j < node->numProbs;) {
	    for (k = 0; k < node->numValues; k++)
        printf("%6.4lf ", node->probMatrix[j++]);
	    printf("\n");
    }
    printf("\n");
  }
}

void NetworkDisplay(net)
     NETWORK *net;
{
  int i, j, k;
  NODE *x;
  FILE *fp = stdout;
  for (i = 0; i < net->numNodes; i++) {
    x = net->nodes + i;
    fprintf(fp,"x%d. %-20s <--- ", i, x->nodeName);
    for (j = 0; j < x->numParents; j++) {
	    k = x->parentIndices[j];
	    fprintf(fp,"(%d %s) ", k, net->nodes[k].nodeName);
    }
    fprintf(fp,"\n");
  }
}

void NetworkDumpBeliefs(net)
     NETWORK *net;
{
  int i, j;
  NODE *node;
  for (i = 0; i < net->numNodes; i++) {
    if((node = net->nodes + i)->dirty) continue;
    printf("x%d. %-20s [ ", i, node->nodeName);
    for(j=0; j<node->numValues; j++) {
      printf("%6.4lf ", node->beliefs[j]/probEvid);
    }
    printf("]\n");	
  }
}

void NetworkFileReadErgo(net, fname)
     NETWORK *net;
     char *fname;
{
  NODE *x;
  FILE *fp;
  int i,j,k,l,npval;
  int clique;
  char temp[80];
    
  if ((fp = fopen(fname ,"r")) == NULL)
    ErrorFatal("NetworkFileReadErgo", "Could not read from file %s\n", fname);
  fscanf(fp,"%d\n",&net->numNodes);
  net->nodes = (NODE *) a_calloc(net->numNodes,sizeof(NODE));
  for (i = 0; i < net->numNodes; i++) { /* number of values per node */
    x = net->nodes + i;
    fscanf(fp,"%d ",&x->numValues);
  }
  for (i = 0; i < net->numNodes; i++) { /* number of parents and parents */
    x = net->nodes + i;
    fscanf(fp,"%d ",&x->numParents);
    if (x->numParents) {
	    x->parentIndices = (int *) a_calloc(x->numParents,sizeof(int));
    }
    for (j = 0; j < x->numParents; j++) {
	    fscanf(fp,"%d ",&x->parentIndices[j]);
	    x->parentIndices[j]--;
    }
  }

  fgets(temp,80,fp);		/* probabilities */
  for (i = 0; i < net->numNodes; i++) {
    x = net->nodes + i;
    fscanf(fp,"%d ",&x->numProbs);
    if(x->numProbs > 0) {
      x->probMatrix = (VECTOR *) a_calloc(x->numProbs,sizeof(VECTOR));
      for (j = 0; j < x->numProbs; j++) fscanf(fp,"%lf ",&x->probMatrix[j]);
    } else {		/* noisy-OR coefficients */
      x->leak = (VECTOR *) calloc(x->numValues-1, sizeof(VECTOR));
      for(j = 0; j < x->numValues-1; j++) {
        fscanf(fp,"%lf ", &x->leak[j]);
      }
      x->c = (VECTOR ***) calloc(x->numParents, sizeof(VECTOR **));
      for (j = 0; j < x->numParents; j++) {
        npval = net->nodes[x->parentIndices[j]].numValues;
        x->c[j] = (VECTOR **) calloc(npval-1, sizeof(VECTOR*));
        for(k = 0; k < npval-1; k++) {
          x->c[j][k] = (VECTOR *) calloc(x->numValues-1, sizeof(VECTOR));
          for(l = 0; l < x->numValues-1; l++) {
            fscanf(fp, "%lf ", &x->c[j][k][l]);
          }
        }
      }
    }
  }

  fgets(temp,80,fp);		/* names of nodes */
  *temp = '\0';
  for (i = 0; i < net->numNodes; i++) {
    x = net->nodes + i;
    fscanf(fp,"%s ",temp);
    x->nodeName = (char *)a_calloc(strlen(temp)+1,sizeof(char));
    strcpy(x->nodeName,temp);
  }
  fgets(temp,80,fp);		/* names of node labels */
  *temp = '\0';
  for (i = 0; i < net->numNodes; i++) {
    x = net->nodes + i;
    x->nodeLabels = (char **)a_calloc(x->numValues,sizeof(char *));
    for (j = 0; j < x->numValues; j++) {
	    fscanf(fp,"%s ",temp);
	    x->nodeLabels[j] = (char *)a_calloc(strlen(temp)+1,sizeof(char));
	    strcpy(x->nodeLabels[j],temp);
    }
  }

  /*  fgets(temp,80,fp);		/* cliques nodes */

  /* leave for later

   *temp = '\0';
   net->numCliques = 0;
   for (i = 0; i < net->numNodes; i++) {
   x = net->nodes + i;
   if(fscanf(fp,"%d ", &nn) != 1) continue;
   if(nn <= 0) break;
   SetInit(nn, &x->inClique);
   for (j = 0; j < nn; j++) {
   fscanf(fp,"%d ", clique);
   SetAddNode(&x->inClique, clique - 1);
   if (net->numCliques < clique) {
   net->numCliques = clique;
   }
   }
   }

  */

  fclose(fp);
}

void NetworkFileWriteErgo(net, fp)
     NETWORK *net;
     FILE *fp;
{
  int i, j, k, l, npval;
  int noNames, noLabels;
  NODE *x;
    
  fprintf(fp,"%d\n",net->numNodes);
  for (i = 0; i < net->numNodes; i++) {
    x = net->nodes + i;
    fprintf(fp,"%s%d", i ? " " : "", x->numValues);
  }
  fprintf(fp,"\n\n");

  for (i = 0; i < net->numNodes; i++) {
    x = net->nodes + i;
    fprintf(fp,"%d", x->numParents);
    for (j = 0; j < x->numParents; j++)
	    fprintf(fp," %d", x->parentIndices[j] + 1);
    fprintf(fp,"\n");
  }
    
  fprintf(fp,"\n/* Probabilities */\n");
  for (i = 0; i < net->numNodes; i++) {
    x = net->nodes + i;
    if(x->c) {
   	  /* write down leaks */
      fprintf(fp,"0\n");
      fprintf(fp,"%8.6f", x->leak[0]);
      for(j = 1; j < x->numValues-1; j++) {
        fprintf(fp," %8.6f", x->leak[j]);
      }
      fprintf(fp,"\n");
   	  /* write down coeffitients */
      for (j = 0; j < x->numParents; j++) {
        npval = net->nodes[x->parentIndices[j]].numValues;
        for(k = 1; k < npval; k++) {
          fprintf(fp,"%8.6f", x->c[j][k-1][0]);
          for(l = 1; l < x->numValues-1; l++) {
            fprintf(fp," %8.6f", x->c[j][k-1][l]);
          }
          fprintf(fp,"\n");
        }
      }
    } else {
      fprintf(fp,"%d\n", x->numProbs);
      for (j = 0; j < x->numProbs; j++) {
        fprintf(fp,"%s%8.6f", j % x->numValues ? " " : "", x->probMatrix[j]);
        if ((j+1) % x->numValues == 0) fprintf(fp,"\n");
      }
    }
    fprintf(fp,"\n");
  }

  noNames = 0;
  for (i = 0; i < net->numNodes; i++) {
    x = net->nodes + i;
    if (!x->nodeName)
	    noNames = 1;
  }
  if (!noNames) {
    fprintf(fp,"/* Names */\n");
    for (i = 0; i < net->numNodes; i++) {
	    x = net->nodes + i;
	    if (x->nodeName)
        fprintf(fp,"%s\n", x->nodeName);
    }
    fprintf(fp,"\n");
  }

  noLabels = 0;
  for (i = 0; i < net->numNodes; i++) {
    x = net->nodes + i;
    if (!x->nodeLabels)
	    noLabels = 1;
  }
    
  if (!noLabels) {
    fprintf(fp,"/* Labels */\n");
    for (i = 0; i < net->numNodes; i++) {
	    x = net->nodes + i;
	    if (x->nodeLabels) {
        for (j = 0; j< x->numValues; j++)
          fprintf(fp,"%s%s", j ? " " : "", x->nodeLabels[j]);
        fprintf(fp,"\n");
	    }
    }
    fprintf(fp,"\n");
  }
}

void NetworkFileWriteStd(net, fp)
     NETWORK *net;
     FILE *fp;
{
  NODE *x, *parent;
  int i,j,k;
  char temp[80];

  fprintf(fp,"; The %s network\n", net->netName);
  fprintf(fp,"(network %s\n", net->netName ? net->netName : "NONAME");
  for (i = 0; i < net->numNodes; i++) {
    x = net->nodes + i;
    fprintf(fp, " (node ");
    if (x->nodeName) 
	    fprintf(fp, "%s\n", x->nodeName);
    else
	    fprintf(fp, "node%d\n", i);
    fprintf(fp, "  (values");
    for (j = 0; j < x->numValues; j++) {
	    if (x->nodeLabels && x->nodeLabels[j])  
        fprintf(fp, " %s", x->nodeLabels[j]);
	    else
        fprintf(fp, " label%d", j);
    }
    fprintf(fp, ")\n");
    fprintf(fp, "  (parents");
    for (j = 0; j < x->numParents; j++) {
	    parent = net->nodes + x->parentIndices[j];
	    if (parent->nodeName) 
        fprintf(fp, " %s", parent->nodeName);
	    else
        fprintf(fp, " node%d\n", x->parentIndices[j]);
    }
    fprintf(fp, ")\n");

    fprintf(fp, "  (probs\n");
    for (j = 0; j < x->numProbs; ) {
	    for (k = 0; k < x->numValues; k++) {
        if (k == 0)
          fprintf(fp, "   ");
        fprintf(fp, "%8.6lf ", x->probMatrix[j++]);
        if (k == x->numValues - 1 && j < x->numProbs)
          fprintf(fp, "\n");
	    }
    }
    fprintf(fp, "))\n");
  }
  fprintf(fp,")\n");
}

void NetworkFileWrite(net, fname, mode)
     NETWORK *net;
     char *fname;
     int mode;
{
  FILE *fp;
  if ((fp = fopen(fname, "wa")) == NULL)
    ErrorFatal("NetworkFileWrite", "Could not write to %s.\n", fname);
  if (mode == FORMAT_STD)
    NetworkFileWriteStd(net, fp);
  else if (mode == FORMAT_ERGO)
    NetworkFileWriteErgo(net, fp);
  fclose(fp);
}

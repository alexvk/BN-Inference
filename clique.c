/********************************************************************
$RCSfile: clique.c,v $
$Author: alexvk $
$Revision: 1.2 $
$Date: 1997/10/20 23:35:09 $
********************************************************************/
static char rcsid[] = "$Id: clique.c,v 1.2 1997/10/20 23:35:09 alexvk Exp alexvk $";

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network.h"
#include "clique.h"
#include "utils.h"
#include "set.h"

static NETWORK *gNet;

int CliqueNodeCompare(a, b)
     int *a, *b;
{
  return (gNet->nodes[*a].marked - gNet->nodes[*b].marked);
}

int CliqueCompare (a, b)
     TMPCLIQUE **a, **b;
{
  return ((**a).sortIndex - (**b).sortIndex);
}

void CliqueControlNew (cc, numNodes)
     CLIQUECONTROL **cc;
     int numNodes;
{
  CLIQUECONTROL *pcc;
  pcc = *cc = (CLIQUECONTROL *) calloc (1, sizeof(CLIQUECONTROL));
  pcc->stackptr = 0;
  pcc->cliqueCount = 0;
  pcc->maxCount = numNodes;
  pcc->cliqueTree = (TMPCLIQUE **) calloc (numNodes, sizeof(TMPCLIQUE *));
  pcc->cliqueStack = (int *) calloc (numNodes, sizeof(int));
}

void CliqueControlFree (cc)
     CLIQUECONTROL **cc;
{
  CLIQUECONTROL *pcc;
  TMPCLIQUE *tc;
  int i;
    
  pcc = *cc;
  for(i = 0; i < pcc->maxCount; i++) {
    if(tc = pcc->cliqueTree[i]) {
      if(tc->nodes) free ((char *) tc->nodes);
      free ((char *) tc);
    }
  }
  free ((char *) pcc->cliqueStack);
  free ((char *) pcc->cliqueTree);
  free ((char *) pcc);
  *cc = NULL;
}

void CliqueNew (cc, clq, nodes, numNodes)
     CLIQUECONTROL *cc;
     int clq, *nodes;
     int numNodes;
{
  TMPCLIQUE *C;
  C = cc->cliqueTree[clq] = (TMPCLIQUE *) calloc (1, sizeof(TMPCLIQUE));
  C->nodes = (int *) calloc (numNodes, sizeof(int));
  bcopy(nodes, C->nodes, numNodes*sizeof(int));
  C->numNodes = numNodes;
  C->node = EMPTY;
}

void CliqueTreeExpand (net, cc, old, notEnd, candEnd)
     NETWORK *net; 
     CLIQUECONTROL *cc;
     int *old, notEnd, candEnd;
{
  int *new;
  int numDisc, fixp;
  int newNotEnd, newCandEnd, i, j, count, p, s, sel;
  int minNumDisc, potentialCandidate;
    
  Dbg2(printf("Entering CliqueTreeExpand(notEnd=%d, candEnd=%d): ",
              notEnd, candEnd), SetDump(old, candEnd + 1));

  new = (int *) calloc (candEnd + 1, sizeof(int));
  minNumDisc = candEnd + 1;
  numDisc = 0;
  for (i = 0; i <= candEnd && minNumDisc; i++) {
    p = old[i];
    count = 0;
    for (j = notEnd + 1; j <= candEnd && count < minNumDisc; j++) {
      if (net->graphTable[p][old[j]]) continue;
      count++;
      potentialCandidate = j;
    }
    if (count < minNumDisc) {
	    fixp = p;
	    minNumDisc = count;
	    if (i <= notEnd)
        s = potentialCandidate;
	    else {
        s = i;
        numDisc = 1;
	    }
    }
  }
    
  for (numDisc += minNumDisc; numDisc > 0; numDisc--) {
    p = old[s];
    old[s] = old[notEnd + 1];
    sel = old[notEnd + 1] = p;
    newNotEnd = EMPTY;
    for (i = 0; i <= notEnd; i++) {
	    if (net->graphTable[sel][old[i]]) {
        new[++newNotEnd] = old[i];
	    }
    }
    for (i = notEnd + 2, newCandEnd = newNotEnd; i <= candEnd; i++) {
	    if (net->graphTable[sel][old[i]]) {
        new[++newCandEnd] = old[i];
	    }
    }
    /* push cliqueStack */
    cc->cliqueStack[cc->stackptr++] = sel;
    if (newNotEnd < newCandEnd) {
	    CliqueTreeExpand (net, cc, new, newNotEnd, newCandEnd);
    } else
      if (newCandEnd == EMPTY) {
        CliqueNew(cc, cc->cliqueCount++, cc->cliqueStack, cc->stackptr);
      }
    /* pop cliqueStack */
    cc->stackptr--;
    notEnd++;
    if (numDisc > 1) {
	    for(s = notEnd+1; net->graphTable[fixp][old[s]]; s++);
    }
  }
  free ((char *)new);
}

void CliqueTreePrepare (net, cc)
     NETWORK *net;
     CLIQUECONTROL *cc;
{
  int i, numNodes;
    
  for(i = numNodes = 0; i < net->numNodes; i++) {
    net->graphTable[i][i] = 1;	/* diagonal must be true */
    if(net->nodes[i].marked == NOTSET) continue;
    net->scratchBuffer[numNodes++] = i;
  }

  CliqueTreeExpand (net, cc, net->scratchBuffer, EMPTY, numNodes - 1);
}

int CliqueMaximal (net, clq)
     NETWORK *net;
     TMPCLIQUE *clq;
{
  int i, j;
  int numNodes = net->numNodes;

  for (i=0; i<numNodes; i++) {
    if (SetMember (i, clq->nodes, clq->numNodes)) continue;
    for (j = 0; j < clq->numNodes; j++) {
      if(!net->graphTable[i][clq->nodes[j]]) break;
    }
    if (j == clq->numNodes) {
      return (0);
    }
  }

  return (1);
}

void CliqueTreeCheck (net, cc)
     NETWORK *net;
     CLIQUECONTROL *cc;
{
  TMPCLIQUE *clq;
  int i, j, k, x, y;
  int *nodeFoundList;
  int numNodes = net->numNodes;
    
  bzero(net->scratchBuffer, net->numNodes*sizeof(int));
  for (i = 0; i < cc->cliqueCount; i++) {
    clq = cc->cliqueTree[i];
    for (j = 0; j < clq->numNodes; j++) {
	    x = clq->nodes[j];
	    net->scratchBuffer[x] = 1;
	    for (k = j + 1; k < clq->numNodes; k++) {
        y = clq->nodes[k];
        if (!net->graphTable[x][y])
          ErrorFatal("CliqueTreeCheck", "Error in clique formation\n");
	    }
    }
    if (!CliqueMaximal (net, clq)) {
	    SetDump(clq->nodes, clq->numNodes);
	    ErrorFatal("CliqueTreeCheck", "Not a maximal clique: %d\n", i);
    }
  }
  for (i = 0; i < numNodes; i++) {
    if (net->scratchBuffer[i]) continue;
    if (net->nodes[i].marked == NOTSET) continue;
    ErrorFatal("CliqueTreeCheck", "A node is not in the clique tree\n");
  }
  Dbg(printf ("All subgraphs found are maximal cliques\n"));
}

void CliqueSortBySize (net, cc)
     NETWORK *net;
     CLIQUECONTROL *cc;
{
  int i, j;
  TMPCLIQUE *C;

  for(i=0; i<cc->cliqueCount; i++) {
    C = cc->cliqueTree[i];
    for(j=0, C->sortIndex = -1; j<C->numNodes; j++) {
      C->sortIndex *= net->nodeSizes[C->nodes[j]];
    }
  }
  qsort(cc->cliqueTree, cc->cliqueCount, sizeof(TMPCLIQUE*), (CMPFUN) CliqueCompare);
}

void CliqueSortByMark (net, cc)
     NETWORK *net;
     CLIQUECONTROL *cc;
{
  int i, j;
  TMPCLIQUE *C;

  for(i=0; i<cc->cliqueCount; i++) {
    C = cc->cliqueTree[i];
    for(j=0, C->sortIndex = 0; j<C->numNodes; j++) {
      if(C->sortIndex > net->nodes[C->nodes[j]].marked) continue;
      C->sortIndex = net->nodes[C->nodes[j]].marked;
    }
  }
  qsort(cc->cliqueTree, cc->cliqueCount, sizeof(TMPCLIQUE*), CliqueCompare);
}

void CliqueDump (net, cc)
     NETWORK *net;
     CLIQUECONTROL *cc;
{
  int i, j;
  TMPCLIQUE *C;

  for(i=0; i<cc->maxCount; i++) {
    C = cc->cliqueTree[i];
    if(C == NULL) continue;
    printf("C%d [%d] ", i, C->node);
    SetDump(C->nodes, C->numNodes);
  }
}

int CliqueParentsIn(net, C, k)
     NETWORK *net;
     TMPCLIQUE *C;
     int k;
{
  int i;
  NODE *x = net->nodes + k;

  for(i=0; i<x->numParents; i++) {
    if(SetMember(x->parentIndices[i], C->nodes, C->numNodes)) continue;
    return FALSE;
  }

  return TRUE;
}

void CliqueFormClusterTree(net, cc)
     NETWORK *net;
     CLIQUECONTROL *cc;
{
  int i, j;
  TMPCLIQUE *C;
  unsigned tm;
  int size;

  net->numCliques = cc->cliqueCount;
  Dbg(printf("Number of cliques found %d\n", net->numCliques));
  net->cnodes = (CLUSTER**) a_calloc(net->numCliques,sizeof(CLUSTER*));
  for(j=0; j<net->numNodes; j++) {
    net->nodeCliqueDFS[j] = NOTSET;
  }
  for(i=0; i<net->numCliques; i++) {
    C = cc->cliqueTree[i];
    /* sort nodes according to predefined order (MC) */
    qsort(C->nodes, C->numNodes, sizeof(int), (CMPFUN) CliqueNodeCompare);
    ClusterNodeNew(net, &net->cnodes[i], C->nodes, C->numNodes);
    for(j=0; j<C->numNodes; j++) {
      if(net->nodeCliqueDFS[C->nodes[j]] == NOTSET) {
        net->nodeCliqueDFS[C->nodes[j]] = i;
      }
    }
  }
  net->cliqueMarked = (int *) a_calloc(net->numCliques, sizeof(int));
  Dbg2(printf("Node cliques "), SetDump(net->nodeCliqueDFS, net->numNodes));

  /* estimate memory requirements */
  net->totPotentials = 0;
  for(i=0; i<net->numCliques; i++) {
    size = net->cnodes[i]->stateSpaceSize;
    printf("Clique %d -> %d\n", i, size);
    if(size == 0) {
      net->totPotentials = 0;
      break;
    }
    net->totPotentials += size;
  }

  if(net->totPotentials && net->totPotentials < AV_MEMORY / sizeof(VECTOR)) {
    printf("The network has a total of %d potentials\n", net->totPotentials);
    ClusterEdgesGenerate(net);
  } else {
    tm = net->totPotentials * sizeof(VECTOR);
    if(tm) {
      printf("The network will require 0x%x bytes (%d Mb)\n", tm, tm >> 20);
    } else {
      printf("Counter overflow\n");
    }
    for(i=0; i<cc->cliqueCount; i++) {
      C = cc->cliqueTree[i];
      printf("C%d ", i);
      SetDump(C->nodes, C->numNodes);
      if(i > 8) break;
    }
    printf("The join tree cannot be generated\n");
    ClusterTreeFree(net);
  }

}

void CliqueFormOptfactTree(net, cc)
     NETWORK *net;
     CLIQUECONTROL *cc;
{
  int i, j, k, l;
  int num, pnt, numEvid, tm, size;
  long m;
  CLIQUECONTROL *ncc;
  TMPCLIQUE *C, *C2;
  CLUSTER *X;
  SET nodes;
  NODE *x;

  Dbg2(printf("Number of cliques found %d\n", cc->cliqueCount), CliqueDump(net, cc));

  /* add evidence nodes to the cliques */
  CliqueControlNew(&ncc, 2*net->numNodes);
  for(i=numEvid=0; i<net->numNodes; i++) {
    if(net->nodeEvidence[i] == NOTSET) continue;
    net->scratchBuffer[numEvid++] = i;
  }
  C = cc->cliqueTree[0];
  if(C->numNodes != net->abNum) {
    bcopy(net->ab, net->scratchBuffer + numEvid, net->abNum*sizeof(int));
    CliqueNew(ncc, ncc->cliqueCount++, net->scratchBuffer, net->abNum + numEvid);
  }
  for(i=0; i<cc->cliqueCount; i++) {
    C = cc->cliqueTree[i];
    bcopy(C->nodes, net->scratchBuffer + numEvid, C->numNodes*sizeof(int));
    CliqueNew(ncc, ncc->cliqueCount++, net->scratchBuffer, C->numNodes + numEvid);
    free((char*) C->nodes);
    free((char*) C);
    cc->cliqueTree[i] = NULL;
  }

  /* the purpose of the code below is to break the original cliques
     into smaller cliques that contain only a single factor,
     i.e. conditional probability, and assign the nodes to the
     new cliques */
 
  /* initialize the list of assigned nodes */
  bzero(net->scratchBuffer, net->numNodes*sizeof(int));

  for(i=ncc->cliqueCount, pnt=ncc->maxCount; i>0; i--) {

    C = ncc->cliqueTree[i-1];

    for(j=num=0; j<C->numNodes; j++) {
      k = C->nodes[j];
      if(net->scratchBuffer[k]) continue;

      /* try to form a new clique for this distribution */
      if(CliqueParentsIn(net, C, k)) {

        x = net->nodes + k;

        /* make a new clique C2 */
        CliqueNew(ncc, --pnt, C->nodes + numEvid, C->numNodes - numEvid);
        C2 = ncc->cliqueTree[pnt];

        /* leave only the nodes in the k distribution */
        for(l=m=0; l<C2->numNodes; l++) {
          if(C2->nodes[l] == k || SetMember(C2->nodes[l], x->parentIndices, x->numParents)) {
            C2->nodes[m++] = C2->nodes[l];
          }
        }

        /* if all nodes are here, assign the clique itself */
        if(m == C->numNodes - numEvid && C->node == EMPTY) {
          net->scratchBuffer[k] = 1;
          C->node = k;
          m = 0;
        }

        /* free the clique if all nodes are evidence or node k has
           been assigned to C */

        if (m == 0) {
          free((char*) C2->nodes);
          free((char*) C2);
          ncc->cliqueTree[pnt++] = NULL;
          continue;
        }

        assert(m > 0);

        /* assign k to the new clique C2 */
        net->scratchBuffer[k] = 1;
        C2->numNodes = m;
        C2->sortIndex = -m;
        C2->node = k;
        num++;

      }
    }
     
    qsort(ncc->cliqueTree + pnt, num, sizeof(TMPCLIQUE*), CliqueCompare);
     
    /* if the original clique is unassigned and is not an
       intermediate result, free it */
    if(C->node == EMPTY && num > 0 &&
       C->numNodes == ncc->cliqueTree[pnt]->numNodes + numEvid) {
      free((char*) C->nodes);
      free((char*) C);
    } else {
      /* copy the larger clique to the final tree */
      ncc->cliqueTree[--pnt] = C;
      m = C->numNodes - numEvid;
      for(j=0; j<m; j++) {
        C->nodes[j] = C->nodes[numEvid+j];
      }
      C->numNodes = m;
      C->sortIndex = -m;
    }
     
    assert(pnt >= i);

    ncc->cliqueTree[i-1] = NULL;

  }

  size = ncc->maxCount - pnt;
  Dbg(printf("Number of groups found %d\n", size));

  /* form the group tree in the net structure */
  SetInit(net->numNodes, &nodes);
  GraphZero(net->graphTable, size);
  for(i=num=net->numGroups=m=0; i<size; i++) {
    C = ncc->cliqueTree[pnt + i];
    /* sort nodes according to predefined order (MC) */
    qsort(C->nodes, C->numNodes, sizeof(int), (CMPFUN) CliqueNodeCompare);
    SetMakeEmpty(&nodes);
    for(j=0; j<C->numNodes; j++) {
      if(SetMember(C->nodes[j], net->scratchBuffer, num)) {
        SetAddNode(&nodes, C->nodes[j]);
      } else {
        net->scratchBuffer[num++] = C->nodes[j];
      }
    }
    if(i) {
      for(k=net->numGroups; k>0; k--) {
        X = net->groups[k-1];
        if(SetIncluded(&nodes, X->nodes, X->numNodes)) break;
      }
      assert(k>0);
      net->graphTable[i][k-1] = net->graphTable[k-1][i] = 1;
    }
    /* make a new cluster */
    ClusterNodeNew(net, net->groups + i, C->nodes, C->numNodes);
    X = net->groups[i];
    X->inclNode = C->node;
    Dbg2(printf("G%d [%d] ", i, C->node), SetDump(C->nodes, C->numNodes));
    net->numGroups++;
    /* estimate memory requirements */
    if(X->stateSpaceSize) {
      if(m < X->stateSpaceSize) m = X->stateSpaceSize;
    } else {
      m = 0;
      break;
    }
  }

  SetFree(&nodes);

  Dbg2(printf("Group join tree graph:\n"),
       GraphTableDump(net->graphTable, net->numGroups));
   
  tm = 2 * sizeof(VECTOR) * m;

  if(m && m < AV_MEMORY) {
    printf("The query will require about 0x%x bytes (%d Mb)\n", tm, tm >> 20);
    OptfactEdgesGenerate(net);
  } else {
    printf("The query requires too much memory, tm 0x%x\n", tm);
    for(i=0; i<size; i++) {
      C = ncc->cliqueTree[pnt + i];
      printf("C%d [%d] ", i, C->node);
      SetDump(C->nodes, C->numNodes);
      if(i > 8) break;
    }
    OptfactTreeFree(net);
  }

  CliqueControlFree(&ncc);

}

void CliqueProcessEvidence(net)
     NETWORK *net;
{
  int i, j;

  for(i=0; i<net->numNodes; i++) {
    if(net->nodeEvidence[i] == NOTSET) {
      /* the counters revolve to initial state */
      net->nodeStates[i] = 0;
    } else {
      net->nodes[i].dirty = 1;
      net->nodeStates[i] = net->nodeEvidence[i];
      /* we will need this for the clique decomposition */
      for(j = 0; j < net->numNodes; j++) {
        net->graphTable[i][j] = 0;
        net->graphTable[j][i] = 0;
      }
    }
  }

}

void CliqueTreeGenerate (net, algorithm)
     NETWORK *net;
     int algorithm;
{
  int i, j;
  TMPCLIQUE *C;
  CLIQUECONTROL *cc;
  int startSearch = 0;
    
  gNet = net;

  if(algorithm == ALG_OPTFACT) {
    CliqueProcessEvidence(net);
    /* use the ab set to start the search */
    MCSearch(net, net->ab, net->abNum);
    if(net->scratchBuffer[0] == NOTSET) return;
  } else {
    /* use parent ordering for breaking the ties in MC search */
    POSearch(net);
    /* can be optimized with respect to startSearch */
    MCSearch(net, &startSearch, 1);
    for(i=0; i<net->numNodes; i++) {
      if(net->nodes[i].marked != NOTSET) continue;
      printf("Warning: node x%d %s is disconnected\n", i, net->nodes[i].nodeName);
      /*
        ErrorFatal("CliqueTreeGenerate", "node x%d %s is disconnected\n", i, net->nodes[i].nodeName);
      */
    }
  }
  /* scratch buffer contains the result */
  Dbg2(printf("inverse MC order "), SetDump(net->scratchBuffer, net->numNodes));
  GraphTriangulate(net, net->scratchBuffer);
  Dbg2(printf("Triangulated network:\n"), GraphTableDump(net->graphTable, net->numNodes));

  /* find the cliques */
  CliqueControlNew(&cc, net->numNodes);
  CliqueTreePrepare(net, cc);
  CliqueTreeCheck(net, cc);
  CliqueSortBySize(net, cc);

  /* use the number of cliques for breaking the ties in MC search */
  for(i=0; i<net->numNodes; i++) {
    net->nodes[i].marked = 0;
  }
  for(i=0; i<cc->cliqueCount; i++) {
    C = cc->cliqueTree[i];
    for(j=0; j<C->numNodes; j++) {
      net->nodes[C->nodes[j]].marked--;
    }
  }

  /* choose the first clique and sort by perfect order */
  if(algorithm == ALG_OPTFACT) {
    MCSearch(net, net->ab, net->abNum);
  } else {
    MCSearch(net, cc->cliqueTree[0]->nodes, cc->cliqueTree[0]->numNodes);
  }
  CliqueSortByMark(net, cc);

  /* copy the cliques to the net structure */
  switch (algorithm) {
  case ALG_CLUSTER:
    CliqueFormClusterTree(net, cc);
    break;
  case ALG_OPTFACT:
    CliqueFormOptfactTree(net, cc);
    break;
  default:
    break;
  }

  CliqueControlFree(&cc);

}

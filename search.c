/********************************************************************
$RCSfile: search.c,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:52:53 $
********************************************************************/
static char rcsid[] = "$Id: search.c,v 1.1 1997/10/15 02:52:53 alexvk Exp alexvk $";

#include <stdio.h>
#include <assert.h>
#include "network.h"

typedef struct olist_t {
   int order;
   int element;
   struct olist_t *next;
} OLIST;

OLIST *head = NULL;

int ListIsEmpty()
{
  return (head == NULL);
}

void ListFree()
{
  struct olist_t *temp;

  while (head != NULL) {
    temp = head;
    head = head->next;
    free ((char *) temp);
  }
}

void ListAppend(order, element)
     int order, element;
{
  struct olist_t *temp = NULL;
  struct olist_t *pnt;

  /* check if the element is in the list already */

  if (head != NULL) {
    if (head->element == element) {
      if (head->order <= order) return;
      temp = head;
      head = head->next;
    } else {
      for(pnt = head; pnt->next != NULL; pnt = pnt->next) {
        if (pnt->next->element == element) {
          if (pnt->next->order <= order) return;
          temp = pnt->next;
          pnt->next = pnt->next->next;
        }
      }
    }
  }

  /* make a new element */

  if (temp == NULL) {
    temp = (OLIST *) malloc(sizeof(OLIST));
    temp->element = element;
  }

  temp->order = order;

  /* boundary case */

  if (head == NULL || head->order >= order) {
    temp->next = head;
    head = temp;
    return;
  }

  /* find new position */

  for(pnt = head; pnt->next != NULL; pnt = pnt->next) {
    if (pnt->next->order >= order) break;
  }
   
  /* insert the new element */

  temp->next = pnt->next;
  pnt->next = temp;
}

int ListGetFirst(order)
     int *order;
{
  struct olist_t *temp;
  int retVal;

  if(ListIsEmpty()) {
    return EMPTY;
  }

  *order = head->order;
  retVal = head->element;

  temp = head;
  head = temp->next;

  free ((char *) temp);
  return retVal;
}

void BFSearch(net, graphTable, node)
     NETWORK *net;
     char **graphTable;
     int node;
{
  int i, next, order;

  /* initialize the marked fields */
  for(i=0; i<net->numNodes; i++) {
    net->nodes[i].marked = NOTSET;
  }

  assert(ListIsEmpty());

  ListAppend(0, node);

  /* do search of minimum path for all nodes */
  while(!ListIsEmpty()) {
    next = ListGetFirst(&order);
    net->nodes[next].marked = order;
    /* append the neighbors to the open list */
    for(i=0; i<net->numNodes; i++) {
      if(!graphTable[next][i]) continue;
      if(net->nodes[i].marked != NOTSET) continue;
      if(net->nodes[i].dirty) continue;
      ListAppend(order + 1, i);
    }
  }
}

void DFSearchRecursive(net, graphTable, node, order)
     NETWORK *net;
     char **graphTable;
     int node, *order;
{
  int i;

  net->nodes[node].marked = *order;
  *order += 1;

  for(i=0; i<net->numNodes; i++) {
    if(!graphTable[node][i]) continue;
    if(net->nodes[i].marked != NOTSET) continue;
    if(net->nodes[i].dirty) continue;
    DFSearchRecursive(net, graphTable, i, order);
  }
}

void DFSearch(net, graphTable, node)
     NETWORK *net;
     char **graphTable;
     int node;
{
  int i, order = 0;

  /* initialize the marked fields */
  for(i=0; i<net->numNodes; i++) {
    net->nodes[i].marked = NOTSET;
  }

  DFSearchRecursive(net, graphTable, node, &order);
}

int SearchAllParentsMarked(net, node)
     NETWORK *net;
     int node;
{
  int i;
  NODE *x = net->nodes + node;

  for(i=0; i<x->numParents; i++) {
    if(net->nodes[x->parentIndices[i]].marked == NOTSET)
      return FALSE;
  }

  return TRUE;
}

void POSearch(net)
     NETWORK *net;
{
  int i, order = 0;

  for(i=0; i<net->numNodes; i++) {
    if(net->nodes[i].numParents == 0) {
      net->nodes[i].marked = order++;
    } else {
      net->nodes[i].marked = NOTSET;
    }
  }

  while(order < net->numNodes) {
    for(i=0; i<net->numNodes; i++) {
      if(net->nodes[i].marked != NOTSET) continue;
      if(!SearchAllParentsMarked(net, i)) continue;
      net->nodes[i].marked = order++;
    }
  }
}

void MCSearch(net, list, size)
     NETWORK *net;
     int *list;
     int size;
{
  int i, p = 0;
  int *cardinality;
  int pnt, max;
  int num = 0;
  NODE *x;

  cardinality = (int*) calloc(net->numNodes, sizeof(int));

  /* start with the nodes in the list */
  for(i=0; i<size; i++) {
    cardinality[list[i]] = 1;
  }

  while(TRUE) {
    /* find the max cardinality */
    for(i=0, max=1, pnt=EMPTY; i<net->numNodes; i++) {
      if(max > cardinality[i]) continue;
      if(max < cardinality[i]) {
        max = cardinality[pnt = i];
      } else {
        if(pnt == EMPTY) pnt = i;
        /* alternate between two branches */
        if(p) {
          if(net->nodes[i].marked < net->nodes[pnt].marked) pnt = i;
          p = 0;
        } else {
          if(net->nodes[i].marked > net->nodes[pnt].marked) pnt = i;
          p = 1;
        }
      }
    }

    if(pnt == EMPTY) break;

    cardinality[pnt] = NOTSET;
    net->scratchBuffer[num++] = pnt;

    for(i=0; i<net->numNodes; i++) {
      if(!net->graphTable[pnt][i]) continue;
      if(cardinality[i] == NOTSET) continue;
      if(net->nodes[i].dirty) continue;
      cardinality[i]++;
    }
  }

  free((char*) cardinality);

  /* now scratch buffer gives MC order */

  for(i=0; i<net->numNodes; i++) {
    net->nodes[i].marked = NOTSET;
  }

  for(i=0; i<num; i++) {
    net->nodes[net->scratchBuffer[i]].marked = i;
  }

  for(i=num; i<net->numNodes; i++) {
    net->scratchBuffer[i] = NOTSET;
  }
}

void searchUnmarkRecursive(net, node)
     NETWORK *net;
     int node;
{
  int i;
  NODE *x = net->nodes + node;

  if(x->dirty) {
    x->dirty = FALSE;
    for(i=0; i<x->numParents; i++) {
      searchUnmarkRecursive(net, x->parentIndices[i]);
    }
  }
}

void BNSearch(net, list, size)
     NETWORK *net;
     int *list;
     int size;
{
  int i, j;

  /* mark all nodes dirty */
  for(i=0; i<net->numNodes; i++) {
    net->nodes[i].dirty = TRUE;
  }

  /* recursively unmark the ancestors of the nodes in the list */
  for(i=0; i<size; i++) {
    searchUnmarkRecursive(net, list[i]);
  }

  /* if there is any evidence, unmark the ancestors of the evidence nodes */
  for(i=0; i<net->numNodes; i++) {
    if (net->nodeEvidence[i] != NOTSET) searchUnmarkRecursive(net, i);
  }

}

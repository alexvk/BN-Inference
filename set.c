/********************************************************************
$RCSfile: set.c,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:52:53 $
********************************************************************/
static char rcsid[] = "$Id: set.c,v 1.1 1997/10/15 02:52:53 alexvk Exp alexvk $";

#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "set.h"

void SetAddNode (s, node)
     SET *s;
     int node;
{
  if (s->numMemb == s->maxMemb)
    ErrorFatal("SetAddNode", "Set overflow\n");
  s->set[s->numMemb++] = node;
}

void SetDeleteNode (s, node)
     SET *s;
     int node;
{
  int i,j,found = 0;
  for (i = 0; i < s->numMemb; i++) {
    if (s->set[i] == node) {
	    for (j = i; j < s->numMemb - 1; j++)
        s->set[j] = s->set[j+1];
	    s->numMemb--;
	    found = 1;
	    break;
    }
  }
    
  if(!found)
    ErrorFatal("SetDeleteNode", "Couldn't find node %d in a set\n", node);
}

int SetMember (x, list, size)
     int x;
     int *list;
     int size;
{
  int i;
  for (i = 0; i < size; i++)
    if (x == list[i]) return (1);
  return (0);
}

void SetMemberPos (x, list, size, pos)
     int x;
     int *list;
     int size;
     int *pos;
{
  register i;
  for (i = 0; i < size; i++)
    if (x == list[i]) {
	    *pos = i;
	    return;
    }
  ErrorFatal("SetMemberPos", "Element %d is not a member of a set\n", x);
}

int SetIsEmpty (s)
     SET *s;
{
  return (s->numMemb == 0);
}

void SetMakeEmpty (s)
     SET *s;
{
  s->numMemb = 0;
}

void SetInit (n, s)
     int n;
     SET *s;
{
  s->set = (int *) calloc (n, sizeof(int));
  if(!s->set) ErrorFatal("SetInit", "Not enough memory\n");
  s->maxMemb = n;
}

void SetFree (s)
     SET *s;
{
  if(s->set) free ((char *) s->set);
}

int SetIncluded(s, list, size)
     SET *s;
     int *list;
     int size;
{
  int i, j;

  for(i=0; i<s->numMemb; i++) {
    if(!SetMember(s->set[i], list, size)) return FALSE;
  }

  return TRUE;
}

void SetDisplay(list, size)
     int *list;
     int size;
{
  register i;
  printf("[ ");
  for (i = 0; i < size; i++)
    printf("%d ", list[i]);
  printf("]");
}

void SetArrayDisplay(list, size)
     int *list;
     int size;
{
  register i;
  printf("{");
  for (i = 0; i < size; i++)
    printf("%d,", list[i]);
  printf("};");
}

void SetLongDisplay(list, size)
     long *list;
     int size;
{
  register i;
  printf("[ ");
  for (i = 0; i < size; i++)
    printf("%ld ", list[i]);
  printf("]");
}

void SetLongArrayDisplay(list, size)
     long *list;
     int size;
{
  int i;
  printf("{");
  for (i = 0; i < size; i++)
    printf("%ldL,", list[i]);
  printf("};");
}

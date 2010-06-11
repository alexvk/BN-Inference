/********************************************************************
$RCSfile: set.h,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:54:41 $
********************************************************************/

#ifndef SET_INCLUDED
#define SET_INCLUDED

typedef struct {
  int* set;
  int numMemb;
  int maxMemb;
} SET;

#endif

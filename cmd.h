/********************************************************************
$RCSfile: cmd.h,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:54:41 $
********************************************************************/

#ifndef CMD_INCLUDED
#define CMD_INCLUDED

typedef struct {
  char *cmdstr;
  char *itemstr;
  int (*funcptr)();
} CMD;

#endif

/********************************************************************
$RCSfile: utils.h,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:54:41 $
********************************************************************/

#ifndef UTILS_INCLUDED
#define UTILS_INCLUDED

typedef double VECTOR;

#define TRUE           1
#define FALSE          0
#define true           1
#define false          0

#ifndef ABS
#define ABS(x) ((x) > 0) ? (x) : -(x)
#endif

#ifndef MAX
#define MAX(x,y) ((x) > (y)) ? (x) : (y)
#endif

#define TIMER(str,f) { long start = clock(); f; printf("%s took %.4f seconds\n", str, (clock() - start) / 1000000.0); }

#ifdef DEBUG
#define Dbg(x) if (DbgFlag) x
#define Dbg2(x,y) if (DbgFlag) x,y
#else  /* DEBUG */
#define Dbg(x)
#define Dbg2(x,y)
#endif /* DEBUG */

typedef int (*CMPFUN)(const void*, const void*);

extern int DbgFlag;

extern void* a_calloc(size_t n1, size_t n2);

extern size_t TotalMemGet();

#endif /* UTILS_INCLUDED */

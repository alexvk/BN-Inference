/********************************************************************
$RCSfile: utils.c,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:52:53 $
********************************************************************/
static char rcsid[] = "$Id: utils.c,v 1.1 1997/10/15 02:52:53 alexvk Exp alexvk $";

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdarg.h>
#include "utils.h"

int DbgFlag = true;

unsigned long TotalMem = 0;
unsigned long TotalMemGet() { return TotalMem; }

void ErrorFatal(const char* name, const char* fmt, ...)
{
  va_list args;

  fprintf(stderr, "ERROR in %s: ", name);

  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  exit(-1);
}

void ErrorSumOne(str, sum)
     char *str;
     VECTOR sum;
{
  if((ABS(sum - 1)) > 0.0001) {
    fprintf(stderr, "%s: WARNING probabilities sum to %8.6f instead of 1\n", str, sum);
  }
}

void *a_calloc(n, size)
     unsigned int n;
     unsigned int size;
{
  void *temp = calloc(n,size);

  if (temp == NULL && n != 0) {
    ErrorFatal("a_calloc",
               "Not enough memory (wanted %u x %u).\n", n, size);
  }
  TotalMem += (n * size);
  return temp;
}

char *strlwr(s)
     char *s;
{
  while (*s) {
    *s++ = tolower(*s);
  }    
}

void SetDump(set, len)
     int *set;
     int len;
{
  int i;
  printf("[ ");
  for (i = 0; i < len; i++)
    printf("%d ",set[i]);
  printf("]\n");
}

void VectorDump(set, len)
     VECTOR *set;
     int len;
{
  int i;
  printf("[ ");
  for (i = 0; i < len; i++)
    printf("%6.4lf ", set[i]);
  printf("]\n");
}

void VectorSum(set, len, result)
     VECTOR *set;
     int len;
     VECTOR *result;
{
  int i;
  VECTOR sum = 0;
  for(i = 0; i < len; i++) {
    sum += set[i];
  }
  *result = sum;
}


void VectorNormalize(set, len)
     VECTOR *set;
     int len;
{
  int i;
  VECTOR sum = 0;
  for(i = 0; i < len; sum += set[i++]);
  for(i = 0; i < len; set[i++] /= sum);
}

void OdometerInitialize(odometer, nodeSizes, length)
     long *odometer;
     int *nodeSizes;
     int length;
{
  int i;
  odometer[length - 1] = 1;
  for (i = length - 2; i >= 0; i--)
    odometer[i] = odometer[i+1] * nodeSizes[i+1];
}

#define A       16807
#define M       2147483647
#define Q       127773
#define R       2836

static long 	_randseed;

void RandomZeroOne(prob)
     double *prob;
{
  long test;
  test = A * (_randseed % Q) - R * (_randseed / Q);
  _randseed = (test > 0 ? test : test + M);
  *prob = (double) ((double) _randseed / (double) M);
}

/* Get a seed number */
void Randomize()
{
  time(&_randseed);
  /*  printf("randseed: %ld\n", _randseed);  */
}


/* Pick a random integer between low and high */
int RandomLowHigh(low, high)
     int low;
     int high;
{
  register int i;
  
  if (low >= high)
    i = low;
  else {
    double temp;
    RandomZeroOne(&temp);
    i = (int) (temp * (high - low + 1) + low);
    if (i > high)
      i = high;
  }
  return(i);
}

/* Flip a biased coin -- return 0 or 1 */
int CoinFlip(prob)
     double prob;
{
  double temp;
  RandomZeroOne(&temp);
  return((prob == 1.0 || temp <= prob));
}

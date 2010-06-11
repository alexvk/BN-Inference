%{

/********************************************************************
$RCSfile: process.l,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:54:41 $
********************************************************************/
static char rcsid[] = "";

/*
 * A program to process cases
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "network.h"
#include "utils.h"

#define MAXNODES	500

int commentdepth = 0;  		/* depth of comment nesting */

typedef enum {
 TZERO,
 TCASEID,
 TREPEAT,
 TTRUEDX,
 TFINDING,
 TINTEGER,
 TSTRIN,
 TEMPTY
} TOKEN;

%}

LETTER		[a-zA-Z]
DIGIT 		[0-9]
NL              \n
WS		[ \t]+

INTEGER		{DIGIT}+
REAL		{DIGIT}*"."{DIGIT}*("E"[+\-]?{DIGIT}+)?
DISTR		({REAL}{WS}+)*{REAL}
STRING		[-,.a-zA-Z0-9]+
EMPTY		{NL}{WS}*{NL}

COMMENTBEG	"(*"
COMMENTEND	"*)"

%S	COMMENT

%%


{COMMENTBEG}		if(commentdepth++ == 0) BEGIN COMMENT;

<COMMENT>[^(*]*		;
<COMMENT>[(*]		;
<COMMENT>{COMMENTEND}	if(--commentdepth == 0) BEGIN 0;

case-id			return TCASEID;
repetitions		return TREPEAT;
true-dx			return TTRUEDX;
finding			return TFINDING;
{INTEGER}		return TINTEGER;
{STRING}		return TSTRIN;
{EMPTY}			return TEMPTY;
{WS}			;
{NL}			;
. 	 	 	{ /* None of the above rules applicable */
 	 	 	    printf("Bad input char '%c' on line %d\n",
 	 	 	 	    yytext[0], yylineno);
 	 	 	}

%%

TOKEN lookahead;
NETWORK *net;
int *deseases;
int dNum;

void error()
{
   ErrorFatal("convert", "Unexpected token %s at line %d.\n", yytext, yylineno);
}

void match(t)
TOKEN t;
{
   if(lookahead != t) error();
   lookahead = yylex();
}

int yyFindNodeIndex()
{
   int result;
   NetworkGetNodeValByName(net, yytext, &result);
   if(result == EMPTY) ErrorFatal("convert", "Unknown node %s, line %d\n", yytext, yylineno);
   return result;
}

int yyFindNodeState(x)
NODE *x;
{
   int result;
   NetworkGetNodeValIndex(net, x, yytext, &result);
   if(result == EMPTY) ErrorFatal("convert", "Unknown state %s, line %d\n", yytext, yylineno);
   return result;
}

int CmdInitializeTree(net)
NETWORK *net;
{
   if(!net->priors) {
      if(net->numCliques) {
	printf("This will take a while ...\n");
	TIMER("ClusterTreeInit", ClusterTreeInit(net));
	TIMER("StorePriors", STOREPRIORS(net));
	Dbg(printf("Local memory used: 0x%o bytes\n", TotalMemGet()));
      } else {
	printf("No join tree in memory ...\n");
      }	
   } else {
      TIMER("RestoreTree", RESTORETREE(net));
      NetworkResetEvidence(net);
   }

   return(0);
}

int CmdResetEvidence(net)
NETWORK *net;
{
    if(net->numCliques) {
      return CmdInitializeTree(net);
    } else {
      NetworkResetEvidence(net);
      return(0);
    }
}

void VectorDump6(VECTOR *set, int len)
{
    int i;
    printf("[ ");
    for (i = 0; i < len; i++)
	printf("%8.6lf ", set[i]);
    printf("]\n");
}

void ProcessCases()
{
   int i, j, node, state;
   NODE *x;

   lookahead = yylex();

   while(lookahead) {
      switch(lookahead) {
       case TCASEID:
	 match(TCASEID);
	 printf("case %s\n", yytext);
	 match(TSTRIN);
       case TREPEAT:
         match(TREPEAT);
         match(TINTEGER);
         break;
       case TTRUEDX:
	 match(TTRUEDX);
	 deseases[dNum] = yyFindNodeIndex();
#ifdef CASES
	 printf("true-dx %s in state ", yytext);
#endif /* CASES */
	 match(TSTRIN);
#ifdef CASES
	 printf("%s\n", yytext);
#endif /* CASES */
	 match(TSTRIN);
         dNum++;
	 break;
       case TFINDING:
	 match(TFINDING);
	 node = yyFindNodeIndex();
#ifdef CASES
	 printf("node %s in state ", yytext);
#endif /* CASES */
	 match(TSTRIN);
	 state = yyFindNodeState(net->nodes + node);
	 net->nodeEvidence[node] = state;
	 net->nodeMarked[node] = TRUE;
#ifdef CASES
	 printf("%s\n", yytext);
#endif /* CASES */
	 match(TSTRIN);
	 break;
       case TEMPTY:
	 match(TEMPTY);
         if(dNum == 0) break;
	 printf("Making %d queries ... \n", dNum);
#ifndef TEST
#ifdef JOIN
         for(i=0; i<dNum; i++) {
	    node = deseases[i];
	    x = net->nodes + node;
	    TIMER("ClusterTreeSingleNodeQuery", ClusterTreeSingleNodeQuery(net, node));
	    VectorSum(x->beliefs, x->numValues, &probEvid);
	    printf("Distribution for x%d %s [ ", node, x->nodeName);
	    for(j=0; j<x->numValues; j++) {
	      printf("%6.4lf ", x->beliefs[j]/probEvid);
	    }
	    printf("]\n");	
	 }
	 printf("**. p(\"evidence\") = %.6lg\n", probEvid);
#else
         for(i=0; i<dNum; i++) {
	    node = deseases[i];
	    net->ab[0] = node;
	    net->aNum = net->abNum = 1;
	    TIMER("OptfactQuery", OptfactQuery(net));
	    if(!net->numGroups) break;
	    printf("Distribution for x%d %s ", node, net->nodes[node].nodeName);
	    VectorNormalize(net->groups[0]->probDistr, net->groups[0]->stateSpaceSize);
	    VectorDump6(net->groups[0]->probDistr, net->groups[0]->stateSpaceSize);
	    OptfactTreeFree(net);
	 }
#endif /* JOIN */
	 CmdResetEvidence(net);
#endif /* JOIN */
         dNum = 0;
	 break;
       default:
	 error();
	 break;
      }
   }
}   

yywrap()
{
    return(1);
}

void ProcessUsageMessage(s)
char *s;
{
#ifdef MULTIPROC
    printf("usage: %s networkFile numProcs < netview_case_file\n",s);
#else
    printf("usage: %s networkFile < netview_case_file\n",s);
#endif /* MULTIPROC */
}

int main(argc, argv)
int argc;
char **argv;
{

    DbgFlag = 1;

#ifdef MULTIPROC
    if (argc < 3) {
#else
    if (argc < 2) {
#endif /* MULTIPROC */
        ProcessUsageMessage(argv[0]);
	return(1);
    }

    NetworkNew(&net);
    net->algorithm = ALG_UNKNOWN;
    net->netName = argv[1];
    NetworkFileReadErgo(net, net->netName);
    NetworkAnalyze(net);
    deseases = (int*) calloc(net->numNodes, sizeof(int));
    Dbg(printf("Local memory used: 0x%o bytes\n", TotalMemGet()));

#ifdef JOIN
    TIMER("ClusterTreeGenerate", ClusterTreeGenerate(net));
    Dbg(printf("Local memory used: 0x%o bytes\n", TotalMemGet()));
#endif /* JOIN */

#ifdef MULTIPROC
    CompparInit(net, (int) atoi(argv[2]));
#endif /* MULTIPROC */

    CmdResetEvidence(net);
    TIMER("ProcessCases", ProcessCases());
    free((char*) deseases);

#ifdef MULTIPROC
    CompparExit();
#else
    NetworkFree(net);
    printf("Total memory used 0x%o bytes (%.3f Mb)\n",
	   TotalMemGet(), 0.001 * (double) (TotalMemGet() >> 10));
#endif /* MULTIPROC */

    return(0);
}

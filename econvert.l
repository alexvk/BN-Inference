%{

/********************************************************************
$RCSfile: econvert.l,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:54:41 $
********************************************************************/
static char rcsid[] = "$Id: econvert.l,v 1.1 1997/10/15 02:54:41 alexvk Exp alexvk $";

/*
 * A simple recursive descent parser of the netview file format.
 * The netview format is described in:
 * http://camis.stanford.edu/people/pradhan/cpcs/nvformat.html
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network.h"
#include "utils.h"

#define MAXNODES	500

int commentdepth = 0;  		/* depth of comment nesting */

typedef enum {
 TZERO,
 TNODE,
 TNAME,
 TLABS,
 TTYPE,
 TSHORT,
 TSLABS,
 TARCS,
 TNORD,
 TTABD,
 TSTRIN,
 TDISTR,
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

NODE		"node"
NAME		"name:"
LABS		"state-labels:"
TYPE		"type:"
SHORT		"short-name:"
SLABS		"semantic-labels:"
ARCS		"add-arcs"
NORD		"set-gnor-dist"
TABD		"set-dist"

COMMENTBEG	"(*"
COMMENTEND	"*)"


%S	COMMENT

%%


{COMMENTBEG}		if(commentdepth++ == 0) BEGIN COMMENT;

<COMMENT>[^(*]*		;
<COMMENT>[(*]		;
<COMMENT>{COMMENTEND}	if(--commentdepth == 0) BEGIN 0;

{NODE}			return TNODE;
{NAME}			return TNAME;
{LABS}			return TLABS;
{TYPE}			return TTYPE;
{SHORT}			return TSHORT;
{SLABS}			return TSLABS;
{ARCS} 			return TARCS;
{NORD}			return TNORD;
{TABD}			return TTABD;
{DISTR}			return TDISTR;
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
NODE *node;

void lexinit()
{
   NetworkNew(&net);
   net->nodes = (NODE *) a_calloc(MAXNODES, sizeof(NODE));
}

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

int yyLabsStore()
{
   int cnt, i, leng;
   char *pnt0, *pnt1;
   char **s;

   for(i=cnt=0; i<yyleng; i++) if (yytext[i] == ',') cnt++;
   cnt++;
   Dbg(printf("Node %s has %d states:", node->nodeName, cnt));
   s = node->nodeLabels = (char **) calloc(cnt, sizeof(char *));
   for(i=0, pnt0=(char*)yytext; i<cnt; pnt0 = pnt1, i++) {
      pnt1 = strpbrk(pnt0, ",");
      if(pnt1 == NULL) pnt1 = (char*) (yytext + yyleng);
      leng = pnt1 - pnt0;
      s[i] = calloc(leng + 1, sizeof(char));
      strncpy(s[i], pnt0, leng);
      s[i][leng] = '\0';
      Dbg(printf("%s%s", i ? ", " : " ", s[i]));
      while(!isalpha(*pnt1)) pnt1++;
   }
   Dbg(printf("\n"));

   return (cnt);
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

void ConvertReadNode()
{
   node = net->nodes + net->numNodes;
   node->parentIndices = (int *) calloc(MAXNODES, sizeof(int));

   while(lookahead != TEMPTY) {
      switch(lookahead) {
       case TNAME:
	 match(TNAME);
	 Dbg(printf("Reading node %s ...\n", yytext));
	 node->nodeName = (char *) calloc(strlen(yytext)+1, sizeof(char));
	 strcpy(node->nodeName, yytext);
	 match(TSTRIN);
	 break;
       case TLABS:
	 match(TLABS);
	 node->numValues = yyLabsStore();
	 match(TSTRIN);
	 break;
       case TTYPE:
	 match(TTYPE);
	 match(TSTRIN);
	 break;
       case TSHORT:
	 match(TSHORT);
	 match(TSTRIN);
	 break;
       case TSLABS:
	 match(TSLABS);
	 match(TSTRIN);
	 break;
       default:
	 error();
      }
   }

   if(++net->numNodes == MAXNODES)
      ErrorFatal("convert", "Too many nodes, increase MAXNODES");

   match(TEMPTY);
}

void ConverReadArcs()
{
   node = net->nodes + yyFindNodeIndex();
   match(TSTRIN);
   while(lookahead != TEMPTY) {
      Dbg(printf("Adding arc %s ---> %s\n", yytext, node->nodeName));
      node->parentIndices[node->numParents++] = yyFindNodeIndex();
      match(TSTRIN);
   }
   match(TEMPTY);
}

void ConvertReadNORD()
{
   int i;
   NODE *xp;
   VECTOR sum;
   int pnode, pos;
   int state = 0;
   char *pnt;

   node = net->nodes + yyFindNodeIndex();
   if(!node->c) node->c = (VECTOR ***) calloc(node->numParents, sizeof(VECTOR**));
   match(TSTRIN);
   xp = net->nodes + (pnode = yyFindNodeIndex());
   SetMemberPos(pnode, node->parentIndices, node->numParents, &pos);
   if(!node->c[pos]) node->c[pos] = (VECTOR **) calloc(xp->numValues-1, sizeof(VECTOR*));
   match(TSTRIN);
   while(lookahead != TEMPTY) {
      if(lookahead == TSTRIN) {
	 state = yyFindNodeState(xp);
	 match(TSTRIN);
      }
      if(state == 0) {
	/* these are the leaks, initialize if not created */
	if(!node->leak) {
	  node->leak = (VECTOR *) calloc(node->numValues-1, sizeof(VECTOR));
	  pnt = (char*) yytext;
	  for(i=0, sum=0; i<node->numValues-1; i++) {
	    sum += strtod(pnt, &pnt);
	    node->leak[i] = sum;
	  }
	  /* the last value should sum to 1 */
	  ErrorSumOne("ConvertReadNORD", sum + strtod(pnt, &pnt));
	}
      } else {
	/* these are ditributions which tell about coeffitients */
	if(!node->c[pos][state-1]) {
	  node->c[pos][state-1] = (VECTOR *) calloc(node->numValues-1, sizeof(VECTOR));
	  pnt = (char*) yytext;
	  for(i=0, sum=0; i<node->numValues-1; i++) {
	    sum += strtod(pnt, &pnt);
	    node->c[pos][state-1][i] = sum / node->leak[i];	/* find the coeffitient */
	  }
	  /* the last value should sum to 1 */
	  ErrorSumOne("ConvertReadNORD", sum + strtod(pnt, &pnt));
	}
      }
      match(TDISTR);
      state++;
   }
   match(TEMPTY);
}

void ConvertReadTable()
{
   int i;
   char *pnt;
   VECTOR sum;

   node = net->nodes + yyFindNodeIndex();
   node->probMatrix = (VECTOR *) calloc(node->numValues, sizeof(VECTOR));
   match(TSTRIN);
   pnt = (char*) yytext;
   for(i=0, sum=0; i<node->numValues; i++) {
      node->probMatrix[i] = strtod(pnt, &pnt);
      sum += node->probMatrix[i];
   }
   ErrorSumOne("ConvertReadTable", sum);
   node->numProbs = node->numValues;
   match(TDISTR);
   match(TEMPTY);
}

void ConvertReadNetview()
{
   lookahead = yylex();

   while(lookahead) {
      switch(lookahead) {
       case TNODE:
	 match(TNODE);
	 ConvertReadNode();
	 break;
       case TARCS:
	 match(TARCS);
	 ConverReadArcs();
	 break;
       case TNORD:
	 match(TNORD);
	 ConvertReadNORD();
	 break;
       case TTABD:
	 match(TTABD);
	 ConvertReadTable(); 
	 break;
       case TEMPTY:
	 match(TEMPTY);
	 break;
       default:
	 error();
	 break;
      }
   }
}   

yywrap()
{
   Dbg(NetworkDisplay(net));
   NetworkFileWriteErgo(net, yyout);   
   return (1);
}

main()
{
  DbgFlag = 1;
  lexinit();
  ConvertReadNetview();
}

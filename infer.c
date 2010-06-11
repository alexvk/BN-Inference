/********************************************************************
$RCSfile: infer.c,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:52:53 $
********************************************************************/
static char rcsid[] = "$Id: infer.c,v 1.1 1997/10/15 02:52:53 alexvk Exp alexvk $";

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "network.h"
#include "utils.h"
#include "cmd.h"

char *AppName;
char cmdLine[512];

/*******************************************************************
  Cmd Functions
*******************************************************************/

CMD cmdList[];

int CmdExit(net)
     NETWORK *net;
{
#ifdef MULTIPROC
  CompparExit();
#else
  NetworkFree(net);
  printf("Total memory used 0x%o bytes (%d Mb)\n",
         TotalMemGet(), (TotalMemGet() >> 20));
#endif /* MULTIPROC */
  return(1);
}

int CmdOptFact(net)
     NETWORK *net;
{
  char str[32];
  int node;

  net->abNum = 0;

  while(1) {
    printf("Enter query node (0 ... %d): ", net->numNodes - 1);
    gets(str);
    if (!*str || sscanf(str, "%d", &node) !=  1) break;
    if (node < 0 || node >= net->numNodes) continue;
    if(SetMember(node, net->ab, net->abNum)) continue;
    if(net->nodeEvidence[node] != NOTSET) continue;
    net->ab[net->abNum++] = node;
  }

  /*
    printf("Query nodes ");
    SetDump(net->ab, net->abNum);
  */
  net->aNum = net->abNum;

  while(1) {
    printf("Enter conditioning node (0 ... %d): ", net->numNodes - 1);
    gets(str);
    if (!*str || sscanf(str, "%d", &node) !=  1) break;
    if (node < 0 || node >= net->numNodes) continue;
    if(SetMember(node, net->ab, net->abNum)) continue;
    if(net->nodeEvidence[node] != NOTSET) continue;
    net->ab[net->abNum++] = node;
  }

  /*
    printf("Conditioning nodes ");
    SetDump(net->ab + net->aNum, net->abNum - net->aNum);
  */
  if(net->abNum == 0) return(0);

  if(net->aNum == 0) net->aNum = net->abNum;

  printf("You have entered query ");
  OptfactPrintQuery(net);
  printf("proceed (y/n)?");

  gets(str);

  if(tolower(*str) == 'y') {
    printf("Making a query\n");
    TIMER("OptfactQuery", OptfactQuery(net));
    if(!net->numGroups) return (0);
    OptfactDumpResult(net);
    OptfactTreeFree(net);
  }

  return (0);
}

int CmdDisplayNetwork(net)
     NETWORK *net;
{
  NetworkDisplay(net);
  return(0);
}

int CmdDisplayTree(net)
     NETWORK *net;
{
  ClusterTreeDisplay(net);
  return(0);
}

int CmdProbDump(net)
     NETWORK *net;
{
  char str[32];
  int node;

  if(net->priors == NULL) {
    printf("Network not initialized\n");
    return (0);
  }

  while(1) {
    printf("Enter a node (0 ... %d): ", net->numCliques - 1);
    gets(str);
    if (!*str || sscanf(str, "%d", &node) !=  1) break;
    if (node < 0 || node >= net->numCliques) continue;
    ClusterDumpProb(net, node);
  }

  return(0);
}

int CmdEnterEvidence(net)
     NETWORK *net;
{
  int i, node, val;
  NODE *x;
  char str[32];

  while(1) {
    printf("Enter node to instantiate (0 ... %d): ", net->numNodes - 1);
    gets(str);
    if (!*str || (node = atoi(str)) == -1) break;
    if(node < 0 || node >= net->numNodes) continue;
    x = net->nodes + node;
    if(net->nodeEvidence[node] != NOTSET) {
      printf("   Node %s is already instantiated to %s\n",
             x->nodeName, x->nodeLabels[net->nodeEvidence[node]]);
    } else {
      printf("   Enter value of node %s (0 ... %d): ",
             x->nodeName, x->numValues - 1);
      gets(str);
      if ((val = atoi(str)) < 0) continue;
      if ( val >= x->numValues ) continue;
      net->nodeEvidence[node] = val;
      net->nodeMarked[node] = TRUE;
    }
  }

  NetworkDumpEvidence(net);
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

int CmdTreeUpdate(net)
     NETWORK *net;
{
  if(net->priors == NULL) {
    printf("Network not initialized\n");
    return (0);
  } else {
    TIMER("ClusterTreeUpdate", ClusterTreeUpdate(net));
  }

  return(0);
}

int CmdDisplayBeliefs(net)
     NETWORK *net;
{
  if(net->priors == NULL) {
    printf("Network not initialized\n");
  } else {
    TIMER("ClusterTreeUpdateBeliefs", ClusterTreeUpdateBeliefs(net));
    printf("**. p(\"evidence\") = %.6lg\n", probEvid);
    NetworkDumpBeliefs(net);
  }

  return(0);
}

int CmdNodeQuery(net)
     NETWORK *net;
{
  int i;
  char str[32];
  int node;
  NODE *x;

  if(net->priors == NULL) {
    printf("Network not initialized\n");
    return (0);
  }

  while(1) {
    printf("Enter a node (0 ... %d): ", net->numNodes - 1);
    gets(str);
    if (!*str || sscanf(str, "%d", &node) !=  1) break;
    if (node < 0 || node >= net->numNodes) continue;
    x = net->nodes + node;
    TIMER("ClusterTreeSingleNodeQuery", ClusterTreeSingleNodeQuery(net, node));
    VectorSum(x->beliefs, x->numValues, &probEvid);
    printf("x%d. %-20s [ ", node, x->nodeName);
    for(i=0; i<x->numValues; i++) {
      printf("%6.4lf ", x->beliefs[i]/probEvid);
    }
    printf("]\n");	
  }

  return(0);
}

int CmdDisplayEvidence(net)
     NETWORK *net;
{
  NetworkDumpEvidence(net);
  return(0);
}

int CmdInitializeTree(net)
     NETWORK *net;
{
  if(!net->priors) {
    if(net->numCliques) {
      printf("This will take a while ...\n");
      TIMER("ClusterTreeInit", ClusterTreeInit(net));
      TIMER("StorePriors", STOREPRIORS(net));
      Dbg(printf("Local memory used: 0x%x bytes\n", TotalMemGet()));
    } else {
      printf("No join tree in memory ...\n");
    }	
  } else {
    TIMER("RestoreTree", RESTORETREE(net));
    NetworkResetEvidence(net);
  }

  return(0);
}

int CmdToggleDebug(net)
     NETWORK *net;
{
  DbgFlag = (DbgFlag ? false : true);
  printf("Debugging is %s\n", DbgFlag ? "on" : "off");
  return(0);
}

int CmdHelp(net)
     NETWORK *net;
{
  CMD *item;
  for (item = cmdList; item->cmdstr != 0; item++)
    printf("%-15s %s\n", item->cmdstr, item->itemstr);
  return(0);
}

CMD cmdList[] = {
  { "q", "Quit", CmdExit },
  { "n", "Display network", CmdDisplayNetwork },
  { "t", "Display join tree", CmdDisplayTree },
  { "i", "Initialize join tree", CmdInitializeTree },
  { "e", "Enter evidence", CmdEnterEvidence },
  { "a", "Display all evidence", CmdDisplayEvidence },
  { "r", "Reset evidence", CmdResetEvidence },
  { "u", "Update join tree", CmdTreeUpdate },
  { "b", "Display beliefs", CmdDisplayBeliefs },
  { "s", "Evaluate a single node query", CmdNodeQuery },
  { "o", "Optimal factoring query", CmdOptFact },
  { "d", "Dump probability distribution", CmdProbDump },
  { "=", "Toggle debugging", CmdToggleDebug },
  { "h", "Display Help", CmdHelp },
  { 0 }
};

/*******************************************************************
  Application Functions
*******************************************************************/

void AppInit(s)
     char *s;
{
  AppName = s;
  Randomize();            /* initialize random number generator */
}

void AppRunCmd(net)
     NETWORK *net;
{
  char *q;
  CMD *item;
  printf("Commands available:\n");
  CmdHelp(net);
  while(true) {
    printf("infer > ");
    gets(cmdLine);
    if (*cmdLine) {
	    q = strtok(cmdLine, " \t\n");
	    for (item = cmdList; item->cmdstr; item++) {
        if (!strcasecmp(q, item->cmdstr)) {
          if ((item->funcptr)(net))
            return;
          break;
        }
	    }
	    if (!item->cmdstr)
        printf("Unrecognized command: %s\n", q);
    }
  }
}

void AppUsageMessage(s)
     char *s;
{
#ifdef MULTIPROC
  printf("usage: %s networkFile numProcs\n",s);
#else
  printf("usage: %s networkFile\n",s);
#endif /* MULTIPROC */
}

int main(argc, argv)
     int argc;
     char **argv;
{
  NETWORK *net;

  AppInit(argv[0]);
#ifdef MULTIPROC
  if (argc < 3) {
#else
  if (argc < 2) {
#endif /* MULTIPROC */
    AppUsageMessage(argv[0]);
    return(1);
  }

  NetworkNew(&net);
  net->algorithm = ALG_UNKNOWN;
  net->netName = argv[1];
  NetworkFileReadErgo(net, net->netName);
  NetworkAnalyze(net);
  TIMER("ClusterTreeGenerate", ClusterTreeGenerate(net));
  Dbg(printf("Local memory used: 0x%x bytes\n", TotalMemGet()));

#ifdef MULTIPROC
  CompparInit(net, (int) atoi(argv[2]));
#endif /* MULTIPROC */

#ifdef MEASURE
  TIMER("ClusterTreeInit", ClusterTreeInit(net));
  CmdExit(net);
#else
  AppRunCmd(net);
#endif /* MEASURE */

  return(0);
}

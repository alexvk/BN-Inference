/********************************************************************
$RCSfile: sample.c,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:52:53 $

********************************************************************/
static char rcsid[] = "$Id: sample.c,v 1.1 1997/10/15 02:52:53 alexvk Exp alexvk $";

#include <stdio.h>
#include <string.h>
#include "network.h"

void main(argc, argv)
int argc;
char **argv;
{
    if (argc < 3)
	printf("usage: %s networkFile numSamples\n",argv[0]);
    else {
	NETWORK *net;
	VECTOR *probs = NULL;
	int i, numCases, maxNumVals;

	Randomize();
	NetworkNew(&net);
	net->netName = strdup(argv[1]);
	NetworkFileReadErgo(net, argv[1]);
	NetworkAnalyze(net);

	/* Output K3 database format */
	fprintf(stdout,"%d\n", net->numNodes);
	for (i = 0; i < net->numNodes; i++)
	    fprintf(stdout,"%d ", net->parentOrdering[i]+1);
	fprintf(stdout, "\n");
	for (i = 0; i < net->numNodes; i++)
	    fprintf(stdout,"%d ", (net->nodes + i)->numValues);
	fprintf(stdout,"\n");
	numCases = atoi(argv[2]);
	fprintf(stdout,"%d\n", numCases);

	maxNumVals = 0;
	for (i = 0; i < net->numNodes; i++) {
	    NODE *node = net->nodes + net->parentOrdering[i];
	    if (node->numValues > maxNumVals)
		maxNumVals = node->numValues;
	}
	probs = (VECTOR *) calloc(maxNumVals, sizeof(VECTOR));
	for (i = 0; i < numCases; i++) {
	    NetworkLogicSample(net, probs);
	    NetworkCaseOutput(stdout,net);
	}
	free((char *) probs);
    }
}

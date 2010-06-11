/********************************************************************
$RCSfile: logicsample.c,v $
$Author: alexvk $
$Revision: 1.1 $
$Date: 1997/10/15 02:52:53 $

********************************************************************/
static char rcsid[] = "$Id: logicsample.c,v 1.1 1997/10/15 02:52:53 alexvk Exp alexvk $";

#include <stdio.h>
#include "network.h"

void NetworkLogicSample(net, probs)
NETWORK *net;
VECTOR *probs;
{
    int i, j, jointidx, index, maxNumVals;
    NODE *node, *parent;
    VECTOR randomNum, lastprob;

    for (i = 0; i < net->numNodes; i++) {
	node = net->nodes + net->parentOrdering[i];
	jointidx = 0;
	for (j = 0; j < node->numParents; j++) {
	    parent = net->nodes + node->parentIndices[j];
	    jointidx += node->odometer[j] * parent->setVal;
	}
	for (lastprob = 0.0, j = 0; j < node->numValues; j++) {
	    index = jointidx + j;
	    probs[j] = node->probMatrix[index] + lastprob;
	    lastprob = probs[j];
	}
	RandomZeroOne(&randomNum);
	for (j = 0; j < node->numValues; j++) {
	    if (randomNum <= probs[j])
		break;
	}
	node->setVal = j;
    }
}

void NetworkCaseOutput(fp, net)
FILE *fp;
NETWORK *net;
{
    int i;
    NODE *node;
    for (i = 0; i < net->numNodes; i++) {
	node = net->nodes + i;
	fprintf(fp, "%d ", node->setVal+1);
    }
    fprintf(fp, "\n");
}

	    
    

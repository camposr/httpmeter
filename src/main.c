#include "httpmelt.h"
/*
	## Copyright 2013, Rodrigo Albani de Campos (camposr@gmail.com)
	## All rights reserved.
	##
	## This program is free software, you can redistribute it and/or
	## modify it under the terms of the "Artistic License 2.0".
	##
	## A copy of the "Artistic License 2.0" can be obtained at
	## http://www.opensource.org/licenses/artistic-license-2.0
	##
	## This program is distributed in the hope that it will be useful,
	## but WITHOUT ANY WARRANTY; without even the implied warranty of
	## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

short int verbose = 0;
const char *version = "0.6 (Tethys)";


int main(int argc, char **argv)
{
	/* use SSL for socket communications */
	bool useSSL = false;
	bool showProgress = false;
	bool onlyIpv4 = false;
	bool unixTime = false;
	/* generic variable for return code and loop control */
	int rc;
	/* http request file name */
	char *reqFileName = NULL;
	char *reply = NULL;
	char *outputFileName = NULL;
	httpReq *head = NULL;
	httpReq *currentRequest = NULL;
	connection *connHandle;
	int totalItems = 0;
	/* Default timeout will be 300 seconds */
	unsigned int timeout = 300;
	/* buffers for time() and localtime() */
	time_t timeBuffer;
	struct tm *tmp;


	/* parse command line options */
	while ((rc = getopt(argc,argv, "shvVf:t:o:p4dD")) != -1)
	{
		switch(rc)
		{
			case 'v':
				verbose++;
				break;
			case 's':
				useSSL = true;
				break;
			case 'h':
				printHelp();
				exit(EXIT_SUCCESS);
				break;
			case 'f':
				reqFileName = strdup(optarg);
				break;
			case 'V':
				printf("HTTPMETER version %s\n",version);
				exit(EXIT_SUCCESS);
				break;
			case 't':
				timeout = atoi(optarg);
				break;
			case 'o':
				outputFileName = strdup(optarg);
				break;
			case 'p':
				showProgress = true;
				break;
			case '4':
				onlyIpv4 = true;
				break;
			case 'D':
				unixTime = true;
				break;
			default:
				printHelp();
				exit(EXIT_FAILURE);
				break;
		}
	}

	/* check and parse http request file */
	/* return head of the linked list containing requests */
	if (reqFileName == NULL)
	{
		printHelp();
		exit(EXIT_FAILURE);
	}


	head = parseFile(reqFileName);
	if (head == NULL)
	{
		fprintf(stderr, "%s: Fatal error while reading request file\n", __FUNCTION__); 
		exit(EXIT_FAILURE);
	}
	else
	{
		currentRequest = getRequest(head);
		totalItems = countItems(head);
	}

	/* rc is used here as a generic counter */
	rc = 0;
	/* this is the main loop */
	while(currentRequest != NULL)
	{
		rc++;
		if (verbose > 1)
			fprintf(stderr, "index: %d\nhostname: %s\ndata:%s--\n",rc, currentRequest->host, currentRequest->data);
		/* Print a progress meter for every request */
		if (showProgress == true)
			fprintf(stderr, "Processing %d/%d (%.0f%%)\n", rc, totalItems, ((float)rc/(float)totalItems)*100);

		/* set connection timeout */
		currentRequest->timeout = timeout;
		/* set date and time */
		timeBuffer = time(NULL);
		tmp = localtime(&timeBuffer);
		if (unixTime == true)
			strftime(currentRequest->dateTime, sizeof(currentRequest->dateTime), "%s", tmp);
		else
			strftime(currentRequest->dateTime, sizeof(currentRequest->dateTime), "%F %T %z", tmp);

		/* set ai_family based on cmd line options */
		if (onlyIpv4 == true)
		{
			currentRequest->family = AF_INET;
		}
		else
		{
			currentRequest->family = AF_UNSPEC;
		}

		if (useSSL == true)
		{	
			connHandle = sslConnect(currentRequest);
			sslSend(connHandle, currentRequest);
			reply = sslRecv(connHandle, currentRequest);
			if (verbose > 2)
				fprintf(stderr, "DATA:\n%s\n", reply);
		}
		else
		{
			connHandle = plainConnect(currentRequest);
			plainSend(connHandle, currentRequest);
			reply = plainRecv(connHandle, currentRequest);
			if (verbose > 2)
				fprintf(stderr, "DATA:\n%s\n", reply);
		}

		free(reply);		

		currentRequest = getRequest(head);
	}
	if (verbose > 2)
		dumpList(head);


	printResults(head, outputFileName);
	

	exit(EXIT_SUCCESS);
}

#include "httpmelt.h" 

httpReq *parseFile(char *fileName)
{
	FILE *fp = NULL;
	char line[MAXLINELEN];
	char requestData[MAXREQLEN];
	char *requestHost = NULL;
	char *cp;
	int lineNumber = 0;
	extern short int verbose;
	httpReq *head = NULL;

	memset(requestData, 0, sizeof(requestData));

	fp = fopen(fileName, "r");

	if (fp == NULL)
	{
		if (verbose > 0)
			fprintf(stderr, "%s: Unable to open request file: %s\n",__FUNCTION__, strerror(errno));
		return(NULL);
	}
	else
	{
		if (verbose > 1)
			fprintf(stderr, "%s: Succesfully opened request file\n", __FUNCTION__);
	}

	/* check if head is null and allocate memory */
	if (head == NULL)
	{
		if (verbose > 1)
			fprintf(stderr, "%s: Allocating memory for the head of the linked list\n", __FUNCTION__);
		head = malloc(sizeof(httpReq));

		memset(head, 0, sizeof(httpReq));
		head->nextReq = NULL;
	}


	while (((fgets(line, MAXLINELEN, fp) != NULL) && (strlen(requestData) < MAXREQLEN)))
	{
		lineNumber++;
		/* chop newline and carriage return */
		cp = strpbrk(line, "\r\n");
		if (cp != NULL)
			*cp = '\0';
		
		if (line[0] == '@')
		{
			requestHost = strndup(&line[1], MAXLINELEN);
			if (verbose > 1)
				fprintf(stderr, "%s: Found a host definition at line %d: %s\n",__FUNCTION__, lineNumber, requestHost);
		}
		else
		{

			if (line[0] == '-')
			{
				strcat(requestData, "\r\n");
				addRequest(requestHost, requestData, head);
				if (verbose > 1)
					fprintf(stderr, "%s: Found end of request at line %d:\n%s\n--\n",__FUNCTION__, lineNumber, requestData);
				memset(requestData, 0, sizeof(requestData));
			}
			else
			{
				strcat(requestData, line);
				strcat(requestData, "\r\n");
			}
		}

	}

	fclose(fp);
	return(head);
}

void addRequest(char *hostName, char *requestData, httpReq *head)
{
	httpReq *newRequest;
	char *cp, *name;
	static httpReq *lastRequest = NULL;
	extern short int verbose;

	/* check if theres a "friendly" name after the hostname */
	cp = strchr(hostName, ',');
	if (cp == NULL)
	{
		name = NULL;
	}
	else
	{
		*cp = '\0';
		name = cp+1;
	}


	if (lastRequest == NULL)
	{
		if (verbose > 1)
			fprintf(stderr, "%s: No last request found so this is the head\n", __FUNCTION__);
		if (name == NULL)
		{
			head->name = NULL;
		}
		else
		{
			head->name = calloc(strlen(name)+1, sizeof(char));
			strncpy(head->name, name, strlen(name));
		}
		head->host = calloc(strlen(hostName)+1, sizeof(char));
		strncpy(head->host, hostName, strlen(hostName));
		head->data = calloc(strlen(requestData)+1, sizeof(char));
		strncpy(head->data, requestData, strlen(requestData));
		head->nextReq = NULL;
		lastRequest = head;
	}
	else
	{
		if (verbose > 1)
			fprintf(stderr, "%s: Last request found so this is NOT the head\n", __FUNCTION__);
		newRequest = malloc(sizeof(httpReq));
		lastRequest->nextReq = newRequest;
		if (name == NULL)
		{
			newRequest->name = NULL;
		}
		else
		{
			newRequest->name = calloc(strlen(name)+1, sizeof(char));
			strncpy(newRequest->name, name, strlen(name));
		}
		newRequest->host = calloc(strlen(hostName)+1, sizeof(char));
		strncpy(newRequest->host, hostName, strlen(hostName));
		newRequest->data = calloc(strlen(requestData)+1, sizeof(char));
		strncpy(newRequest->data, requestData, strlen(requestData));
		newRequest->nextReq = NULL;
		lastRequest = newRequest;
	}

}

httpReq *getRequest(httpReq *head)
{
	static httpReq *currentRequest = NULL;

	if (currentRequest == NULL)
	{
		currentRequest = head;
	}
	else
	{
		currentRequest = currentRequest->nextReq;
	}

	return(currentRequest);
}

void dumpList(httpReq *head)
{
	httpReq *currentRequest = head;
	int i = 0;

	currentRequest = getRequest(head);

	while(currentRequest != NULL)
	{
		fprintf(stderr, "index: %d\nhostname: %s\ndata:%s\n",++i, currentRequest->host, currentRequest->data);
		fprintf(stderr, "conn/1st/total = %f/%f/%f\n",
					currentRequest->connectTime,
					currentRequest->firstByteTime,
					currentRequest->totalTime);
		fprintf(stderr, "bytes = %ld\n", currentRequest->totalBytes);
		currentRequest = getRequest(head);
	}
}

int countItems(httpReq *head)
{
	httpReq *req = head;
	int i = 0;
	while (req != NULL)
	{
		i++;
		req = req->nextReq;
	}
		
	return(i);
}

char *getHttpStatus(char *buf)
{
	/* return the HTTP status code */
	char *status;
	char *cp = NULL;

	cp = strstr(buf, "HTTP/");
	if (cp == NULL)
	{
		return(NULL);
	}
	else
	{
		status = calloc(4, sizeof(char));
		snprintf(status, 4, "%s", cp+9);
	}
	return(status);
}

void printResults(httpReq *head, char *outputFileName)
{
	httpReq *req = head;
	int i = 0;
	FILE *fp = NULL;

	req = getRequest(head);

	if (outputFileName != NULL)
	{
		fp = fopen(outputFileName, "a+");
		if (fp == NULL)
		{
			fprintf(stderr, "%s: Unable to open output file, writing to stdout instead", __FUNCTION__);
			fp = stdout;
		}	
	}
	else
		{fp = stdout;}

	while(req != NULL)
	{
		fprintf(fp, "%d;%s;%s;%s;%f;%f;%f;%ld;%s\n",++i,
			req->dateTime,req->host, req->name ? req->name : req->host, req->connectTime,req->firstByteTime,req->totalTime,req->totalBytes, req->status);
		req = getRequest(head);
	}
	fclose(fp);
}

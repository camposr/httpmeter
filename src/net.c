#include "httpmelt.h"

int tcpConnect(char *host, char *proto, int timeout, int family)
{
	struct addrinfo hints, *res;
	char ipString[INET6_ADDRSTRLEN];
	void *address;
	int rc, sock = 0;
	extern short int verbose;
	/* tv will be used to hold the timeout values */
	struct timeval tv;
	fd_set fdSet;
	/* for getsockopt */
	int optval;
	socklen_t optlen;

	tv.tv_sec = (long)timeout;
	tv.tv_usec = 0;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = SOCK_STREAM;

	if (verbose > 1)
		fprintf(stderr, "%s: Resolving address and service information for %s:%s\n",__FUNCTION__, host, proto);
	
	rc = getaddrinfo(host, proto, &hints, &res);

	if (rc != 0)
	{
		fprintf(stderr, "%s: Fatal error when resolving host name: %s\n", __FUNCTION__, strerror(errno));
		return(-1);
	}

	if (res->ai_family == AF_INET)
	{
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
		address = &(ipv4->sin_addr);
	}
	else
	{
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)res->ai_addr;
		address = &(ipv6->sin6_addr);
	}

	inet_ntop(res->ai_family, address, ipString, sizeof(ipString));

	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	/* set non-blocking on socket */
	rc = fcntl(sock, F_GETFL, NULL);
	if (rc < 0)
	{
		fprintf(stderr, "%s: Error while reading socket fd attributes: %s\n", __FUNCTION__, strerror(errno));
		return(-1);
	}
	rc |= O_NONBLOCK;
	if(fcntl(sock, F_SETFL, rc) < 0)
	{
		fprintf(stderr, "%s: Error while setting socket fd attributes: %s\n", __FUNCTION__, strerror(errno));
		return(-1);
	}

	/* connect and check for timeout */
	if (verbose > 0)
		fprintf(stderr, "%s: Connecting to %s(%s:%s)\n",__FUNCTION__, host, ipString,proto);

	rc = connect(sock, res->ai_addr, res->ai_addrlen); 

	if (rc < 0)
	{
		if (errno == EINPROGRESS)
		{
			if (verbose > 1)
				fprintf(stderr, "%s: EINPROGRESS during connect(), waiting\n", __FUNCTION__);

		for (;;)
		{
			FD_ZERO(&fdSet);
			FD_SET(sock, &fdSet);
			rc = select(sock+1, NULL, &fdSet, NULL, &tv);
			if (rc < 0 && errno != EINTR)
			{
				fprintf(stderr, "%s: Error connecting: %s\n", __FUNCTION__, strerror(errno));
				return(-1);
			}
			else if (rc > 0)
			{
				optlen = sizeof(int);
				if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)(&optval), &optlen) < 0)
				{
					fprintf(stderr, "%s: Error in getsockopt: %s\n",__FUNCTION__, strerror(errno));
					return(-1);
				}
				if (optval)
				{
					fprintf(stderr, "%s: Error in delayed connect: %s\n",__FUNCTION__, strerror(errno));
					return(-1);
				}
				break;
			}
		else 
		{
			fprintf(stderr,"%s: Timeout while waiting for connection\n", __FUNCTION__);
			return(-1);
		}
		}
		}
	}

	rc = fcntl(sock, F_GETFL, NULL);
	if (rc < 0)
	{
		fprintf(stderr, "%s: Error while getting socket options: %s\n", __FUNCTION__, strerror(errno));
		return(-1);
	}
	rc &= (~O_NONBLOCK);
	if(fcntl(sock, F_SETFL, rc) < 0)
	{
		fprintf(stderr, "%s: Error while setting socket options: %s\n", __FUNCTION__, strerror(errno));
		return(-1);
	}


	free(res);
	return(sock);
}

void tcpDisconnect(connection *c)
{
	if (c->socket)
		close(c->socket);

	if (c->sslHandle)
	{
		SSL_shutdown(c->sslHandle);
		SSL_free(c->sslHandle);
	}

	if (c->sslContext)
		SSL_CTX_free(c->sslContext);
	
	free(c);
}

connection *sslConnect(httpReq *request)
{
	connection *c;
	extern short int verbose;
	double startTime;

	c = malloc(sizeof(connection));
	c->sslHandle = NULL;
	c->sslContext = NULL;

	/* time how long to connect */
	startTime = getTime();	
	c->socket = tcpConnect(request->host, "443", request->timeout, request->family);
	request->connectTime = getDelta(startTime);

	
	if (c->socket)
	{
		SSL_load_error_strings();
		SSL_library_init();

		c->sslContext = SSL_CTX_new(SSLv23_client_method ());
		if (c->sslContext == NULL)
			ERR_print_errors_fp(stderr);
		c->sslHandle = SSL_new(c->sslContext);
		if (c->sslHandle == NULL)
			ERR_print_errors_fp(stderr);

		if(!(SSL_set_fd(c->sslHandle, c->socket)))
			ERR_print_errors_fp(stderr);

		if(SSL_connect(c->sslHandle) != 1)
			ERR_print_errors_fp(stderr);
		if (verbose >1)
			fprintf(stderr, "%s: Successfully returning a socket descriptor\n",__FUNCTION__);
	}
	else
	{
		fprintf(stderr, "%s: Unable to get a socket descriptor\n",__FUNCTION__);
		free(c);
		return(NULL);
	}

	return(c);
}

void sslSend(connection *c, httpReq *request)
{
	int rc = 0;
	if (c)
		rc = SSL_write(c->sslHandle, request->data, strlen(request->data));
	
	if (rc <= 0)
		ERR_print_errors_fp(stderr);
}

char *sslRecv(connection *c, httpReq *request)
{
	char *rc = NULL;
	int received, count = 0;
	char buffer[RCVBUFFERSIZE];
	double startTime = 0;
	double firstByteTime = 0;


	if (c)
	{
		startTime = getTime();
		for(;;)
		{
			if(!rc)
			{
				rc = malloc(RCVBUFFERSIZE * sizeof(char) +1);
				memset(rc, 0, sizeof(rc));
			}
			else
				rc = realloc(rc, (count + 1) * RCVBUFFERSIZE * sizeof(char) +1);

			received = SSL_read(c->sslHandle, buffer, RCVBUFFERSIZE);
			buffer[received] = '\0';
			if (firstByteTime == 0)
				firstByteTime = getDelta(startTime);

			if (received > 0)
				strcat(rc, buffer);
			else
				break;

/*			if (received < readSize)
			{
				break;
			}
*/

			count++;
		}
		request->firstByteTime = firstByteTime;
		request->totalTime = getDelta(startTime);
		request->totalBytes = strlen(rc);
		request->status = getHttpStatus(rc);


	}
	tcpDisconnect(c);
	return(rc);
}

connection *plainConnect(httpReq *request)
{
	connection *c = NULL;
	extern short int verbose;
	double startTime;

	c = malloc(sizeof(connection));
	memset(c,0,sizeof(connection));

	startTime = getTime();
	c->socket = tcpConnect(request->host, "80", request->timeout, request->family);
	request->connectTime = getDelta(startTime);

	if (!c->socket)
	{
		fprintf(stderr, "%s: Unable to get a socket descriptor\n",__FUNCTION__);
		free(c);
		return(NULL);
	}
	else
	{
		if (verbose >1)
			fprintf(stderr, "%s: Successfully returning a socket descriptor\n",__FUNCTION__);
	}
	return(c);
}

void plainSend(connection *c, httpReq *request)
{
	int rc = 0, requestLen;
	extern short int verbose;

	requestLen = strlen(request->data);

	rc = send(c->socket, request->data, requestLen, 0);

	if (rc != requestLen)
	{
		fprintf(stderr, "%s: Error while sending data: %s\n",__FUNCTION__, strerror(errno));
	}
	else
	{
		if (verbose > 1)
			fprintf(stderr, "%s: Successfully sent data to server\n",__FUNCTION__);
	}
}

	
char *plainRecv(connection *c, httpReq *request)
{
	extern short int verbose;
	int received, count = 0;
	char *rc = NULL;
	char buffer[RCVBUFFERSIZE+1];
	double startTime = 0;
	double firstByteTime = 0;




	if (c->socket)
	{
		startTime = getTime();
		for (;;)
		{
			if(!rc)
			{
				rc = malloc(RCVBUFFERSIZE * sizeof(char) +1);
				memset(rc, 0, sizeof(rc));
			}
			else
			{
				rc = realloc(rc, (count + 1) * RCVBUFFERSIZE * sizeof(char) + 1);
				if (rc == NULL)
					fprintf(stderr, "%s: Failed realloc\n",__FUNCTION__);
			}


			received = recv(c->socket, buffer, RCVBUFFERSIZE, 0);
         if (firstByteTime == 0)
            firstByteTime = getDelta(startTime);

         if (received > 0)
			{
				buffer[received] = '\0';
            strcat(rc, buffer);
			}
         else
            break;

			count++;
		}
		request->firstByteTime = firstByteTime;
		request->totalTime = getDelta(startTime);
		request->totalBytes = strlen(rc);
		request->status = getHttpStatus(rc);
	

	}
	tcpDisconnect(c);

	return(rc);
}

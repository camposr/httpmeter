#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <netdb.h>		/* for getaddrinfo() */
#include <time.h>		/* for strftime() */
#include <netinet/in.h>
#include <stdbool.h>
#include <fcntl.h>

/* leave all hope behind */
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define RCVBUFFERSIZE		1024
#define MAXLINELEN			1024
#define MAXREQLEN				16384

typedef struct httpReq
{		
	char *host;		/* hostname or ip address */
	char *ipAddr;	/* the resolved IP address */
	char *data;		/* http request data */
	char *status;	/* http response code */
	char *name;		/* a optional friendly to identify the tested host/uri */
	double connectTime;	/* time to connect */
	double firstByteTime;	/* time to receive first byte */
	double totalTime;		/* total connection and transfer time */
	unsigned long totalBytes;	/* total bytes received */
	struct httpReq *nextReq;	/* pointer to the next request */
	int timeout;	/* socket timeout */
	int family;		/* ai_family */
	char dateTime[200];
} httpReq;

typedef struct
{
	int socket;
	SSL *sslHandle;
	SSL_CTX *sslContext;
} connection;

extern void printHelp(void);
/* from parser.c */
extern httpReq *parseFile(char *);
extern void addRequest(char *, char *, httpReq *);
extern httpReq *getRequest(httpReq *);
extern void dumpList(httpReq *);
extern void printResults(httpReq *, char *);
extern int countItems(httpReq *);
extern char *getHttpStatus(char *);
/* from net.c */
extern int tcpConnect(char *, char *, int, int);
extern void tcpDisconnect(connection *c);
extern connection *sslConnect(httpReq *);
extern connection *plainConnect(httpReq *);
extern void sslSend(connection *, httpReq *);
extern void plainSend(connection *, httpReq *);
extern char *sslRecv(connection *, httpReq *);
extern char *plainRecv(connection *, httpReq *);
/* from time.c */
extern double getTime(void);
extern double getDelta(double);


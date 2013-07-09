#include "httpmelt.h"

void printHelp(void)
{
	extern char *version;
	short int i;
	typedef struct
	{
		char option;
		char *desc;
	} options;

	const options entries[] =
	{
		{ 'f', "Name of the file containing HTTP requests" },
		{ 's', "Use SSL for socket communications (HTTPS)" },
		{ 'v', "Verbose level, use multiple times to increase verbosity" },
		{ 'V', "Display version number" },
		{ 'h', "Display usage information" },
		{ 't', "Timeout in seconds (default: 300 seconds)" },
		{ 'p', "Show progress" },
		{ 'o', "Write results to file instead of standard output"},
		{ 'D', "Show time as seconds since epoch"},
		{ '4', "Do not use IPv6 even when the AAAA record exists"},
		{0}
	};

	printf("HTTPMETER version %s\nUsage:\n",version);

	for (i = 0; entries[i].option !=0;i++)
	{
		printf("\t-%c\t%s\n",entries[i].option, entries[i].desc);
	}
}
		

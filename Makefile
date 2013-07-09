VERSION		= 0.6
CC				= gcc
CFLAGS		= -O -Wall 
LIBS			= -lssl -lcrypto
EFILE			= httpmeter
SRCFILES		= src/httpmelt.h src/main.c src/opts.c src/parser.c src/net.c src/time.c

httpmeter:		$(SRCFILES)
				$(CC) $(CFLAGS) -o $(EFILE) $(SRCFILES) $(LIBS)

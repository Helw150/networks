IDIR =./include
CC=gcc
CFLAGS=-I$(IDIR)

ODIR=build

CLIENT_DEPS = FTPclient.h helpers.h
CLIENTDEPS = $(patsubst %,$(IDIR)/%,$(CLIENT_DEPS))

CLIENT_OBJ = FTPclient.o helpers.o
CLIENTOBJ = $(patsubst %,$(ODIR)/%,$(CLIENT_OBJ))

SERVER_DEPS = FTPserver.h helper.h
SERVERDEPS = $(patsubst %,$(IDIR)/%,$(SERVER_DEPS))

SERVER_OBJ = FTPserver.o helpers.o
SERVEROBJ = $(patsubst %,$(ODIR)/%,$(SERVER_OBJ))


$(ODIR)/%.o: %.c $(CLIENTDEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

fresh: clean FTPclient FTPserver

FTPclient: $(CLIENTOBJ)
	gcc -o $@ $^ $(CFLAGS)

FTPserver: $(SERVEROBJ)
	gcc -o $@ $^ $(CFLAGS)

FTPserverdebug: $(SERVEROBJ)
	gcc -g -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ FTPserver FTPclient

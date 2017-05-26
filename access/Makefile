CC=g++
CFLAGS= -g -Wall -Wextra -c -O0
OFLAGS= -g -Wall -Wextra -O0
OBJECTS=log.o csockethandler.o ctcpconnection.o ctcpserver.o ctcpevent.o main.o

all:webSocketServer

log.o:log.c
	$(CC) $(CFLAGS) $< -o $@
csockethandler.o:csockethandler.cc
	$(CC) $(CFLAGS) $< -o $@
ctcpconnection.o:ctcpconnection.cc
	$(CC) $(CFLAGS) $< -o $@
ctcpserver.o:ctcpserver.cc
	$(CC) $(CFLAGS) $< -o $@
ctcpevent.o:ctcpevent.cc
	$(CC) $(CFLAGS) $< -o $@
main.o:main.cc
	$(CC) $(CFLAGS) $< -o $@
webSocketServer:$(OBJECTS)
	$(CC) $(OFLAGS) -o $@ $(OBJECTS)
clean:
	rm -vf *.o
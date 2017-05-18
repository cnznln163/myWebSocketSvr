
HEADER=-I../ -I../../../../pLib/base/
CFLAGS=-g -pg -Wall -Werror
LFLAGS=-L../../../../pLib/api/lib/ -lbase
OBJECT=../TcpServer.o ../TcpConnection.o ../EventPoll.o ../timer_set.o

all:client_test server_test

client_test:client_main.o $(OBJECT)
	g++ $(CFLAGS) -o client_test client_main.o $(OBJECT) $(LFLAGS)

server_test:server_main.o $(OBJECT)
	g++ $(CFLAGS) -o server_test server_main.o $(OBJECT) $(LFLAGS)
    
%.o:%.cpp
	g++ -o $@ $(CFLAGS) $(HEADER) -c $<

clean:
	rm -f *.o $(OBJECT) client_test server_test


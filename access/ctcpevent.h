#ifndef CTCPEVENT_H
#define CTCPEVENT_H
#include <stdio.h>
#include <stdlib.h> //相关函数atoi，atol，strtod，strtol，strtoul

#include <unistd.h>
#include <crypt.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string>
#include <map>
#define MAX_FDS 4096
#define MAX_EVENTS 4096

typedef struct fd_info{
	int fd;
	uint32_t events;
	void *pv_obj;
} fd_info_t;

class CTcpserver;
class CTcpConnection;

class CTcpevent
{
public:
	CTcpevent();
	//int createSocket(char *ip,int port);
	bool init();
	int processEvent(int wait_time);
	int addEvent(int sock_fd , void *pv_obj , uint32_t events);
	int delEvent(int sock_fd);
	int serverSocketEvents(int sock_fd, CTcpserver * p_tcp_server, uint32_t events);
	//int createSocket( char *ip , int port );
	~CTcpevent();

	/* data */
private:
	void setNonblockingSocket( int fd );
	int dealServerSocketEvents(int sock_fd, CTcpserver * p_tcp_server, uint32_t events);
	int dealDataSocketEvents(int sock_fd, CTcpConnection * p_tcp_connection, uint32_t events);
	int _sfd;
	int _evfd;
	struct epoll_event _event_array[MAX_EVENTS];
	std::map<int,fd_info_t> _fd_info_map;
};


#endif

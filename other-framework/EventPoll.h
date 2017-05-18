
// EventPoll.h

#ifndef EVENT_POLL_H
#define EVENT_POLL_H

#include "Public.h"

#include <map>

#define MAX_MONITORED_FDS_CNT 4096
#define MAX_EVENTS_CNT 256


typedef struct fd_info
{
    int fd;
    uint32_t events;
    void * pv_obj;
} fd_info_t;


class CTcpConnection;
class CTcpServer;

class CEventPoll
{
public:
    CEventPoll(void);
    
    ~CEventPoll(void);
    
    int Init(void);
    
    int Add2EventLoop(int sock_fd, void * pv_obj, uint32_t events);
    
    int DeleteFromEventLoop(int sock_fd);
    
    int StartMonitoringEvents(int sock_fd, void * pv_obj, uint32_t events);
    
    int StartMonitoringRead(int sock_fd, void * pv_obj)
    {
        return StartMonitoringEvents(sock_fd, pv_obj, EPOLLIN);
    }
    
    int StartMonitoringWrite(int sock_fd, void * pv_obj)
    {
        return StartMonitoringEvents(sock_fd, pv_obj, EPOLLOUT);
    }
    
    int StopMonitoringEvents(int sock_fd, void * pv_obj, uint32_t events);
    
    int StopMonitoringRead(int sock_fd, void * pv_obj)
    {
        return StopMonitoringEvents(sock_fd, pv_obj, EPOLLIN);
    }
    
    int StopMonitoringWrite(int sock_fd, void * pv_obj)
    {
        return StopMonitoringEvents(sock_fd, pv_obj, EPOLLOUT);
    }
    
    int DealServerSocketEvents(int sock_fd, CTcpServer * p_tcp_server, uint32_t events);
    
    int DealDataSocketEvents(int sock_fd, CTcpConnection * p_tcp_connection, uint32_t events);
    
    int RunEventLoop(int wait_time);    // wait_time 的单位是毫秒
    
    int DumpActiveSockFdInfo(char * buffer, int buffer_len);
    
public:
    int _epoll_fd;
    struct epoll_event _event_array[MAX_EVENTS_CNT];
    std::map<int, fd_info_t> _fd_info_map;
};


#endif


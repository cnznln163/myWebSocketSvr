
// TcpServer.h

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "Public.h"

class CEventPoll;
struct user_timer;
struct timer_set;

int is_tcp_server(void * pv_obj);

class CTcpServer
{
public:
    CTcpServer(void);
    
    ~CTcpServer(void);
    
    void SetLocalIp(uint32_t local_ip)
    {
        _local_ip = local_ip;
        memset(_local_ip_string, 0, sizeof(_local_ip_string));
        inet_ntop(AF_INET, &_local_ip, _local_ip_string, sizeof(_local_ip_string));
    }
    
    uint32_t GetLocalIp(void)
    {
        return _local_ip;
    }
    
    void SetLocalPort(uint16_t local_port)
    {
        _local_port = local_port;
    }
    
    uint16_t GetLocalPort(void)
    {
        return _local_port;
    }
    
    int GetSockFd(void)
    {
        return _sock_fd;
    }
    
    void SetEventPoll(CEventPoll * p_event_poll)
    {
        _p_event_poll = p_event_poll;
    }
    
    CEventPoll * GetEventPoll(void)
    {
        return _p_event_poll;
    }
    
    void SetTimerSet(struct timer_set * p_timer_set)
    {
        _p_timer_set = p_timer_set;
    }
    
    int Init(void);
    
    int CreateInitTimer(void);
    
    int OnNewConnArrived(void);
    
    int OnSockError(void);
    
    void SetSvrId(int svr_id)
    {
        _svr_id = svr_id;
    }
    
    int GetSvrId(void)
    {
        return _svr_id;
    }
    
    void SetSvrType(int svr_type)
    {
        _svr_type = svr_type;
    }
    
    int GetSvrType(void)
    {
        return _svr_type;
    }
    
public:
    uint32_t _local_ip;     // network bytes order
    uint16_t _local_port;   // network bytes order
    char _local_ip_string[16];
    int _sock_fd;
    CEventPoll * _p_event_poll;
    struct user_timer * _p_init_timer;
    struct timer_set * _p_timer_set;
    
    int _svr_id;    // local server id
    int _svr_type;  // local server type  
};

#endif



// EventPoll.cpp

#include "TcpServer.h"
#include "ObjectManager.h"
#include "TcpConnection.h"
#include "EventPoll.h"

#include <map>

#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0x2000
#endif

static char * get_events_string(uint32_t events)
{
    static char events_string[512];
    int index = 0;
    
    memset(events_string, 0, sizeof(events_string));
    
    if (events & EPOLLERR)
    {
        strcpy(&events_string[index], "EPOLLERR");
        index += strlen("EPOLLERR");
    }
    
    if (events & EPOLLHUP)
    {
        if (index > 0)
        {
            events_string[index++] = '|';
        }
        
        strcpy(&events_string[index], "EPOLLHUP");
        index += strlen("EPOLLHUP");
    }
    
    if (events & EPOLLRDHUP)
    {
        if (index > 0)
        {
            events_string[index++] = '|';
        }
        
        strcpy(&events_string[index], "EPOLLRDHUP");
        index += strlen("EPOLLRDHUP");
    }
    
    if (events & EPOLLIN)
    {
        if (index > 0)
        {
            events_string[index++] = '|';
        }
        
        strcpy(&events_string[index], "EPOLLIN");
        index += strlen("EPOLLIN");
    }
    
    if (events & EPOLLOUT)
    {
        if (index > 0)
        {
            events_string[index++] = '|';
        }
        
        strcpy(&events_string[index], "EPOLLOUT");
        index += strlen("EPOLLOUT");
    }

    return events_string;
}


CEventPoll::CEventPoll(void)
{
    _epoll_fd = -1;
    memset(_event_array, 0, sizeof(struct epoll_event)*MAX_EVENTS_CNT);
}
    
CEventPoll::~CEventPoll(void)
{
    if (_epoll_fd >= 0)
    {
        close(_epoll_fd);
    }
}

int CEventPoll::Init(void)
{
    int errno_cache = 0;
    
    _epoll_fd = epoll_create(MAX_MONITORED_FDS_CNT);
    errno_cache = errno;
    if (_epoll_fd < 0)
    {
        log_error("epoll_create fail : %s ", strerror(errno_cache));
        return _epoll_fd;
    }
    
    memset(_event_array, 0, sizeof(struct epoll_event)*MAX_EVENTS_CNT);
    
    return 1;
}

int CEventPoll::Add2EventLoop(int sock_fd, void * pv_obj, uint32_t events)
{
    struct epoll_event epoll_event_obj;
    fd_info_t fd_info;
    fd_info_t * p_fd_info = NULL;
    
    if (_epoll_fd < 0)
    {
        log_error("the event_poll is not initialized ");
        return -1;
    }
    
    log_debug("add sock_fd:%d pv_obj:%p events:%s to event loop ", sock_fd, pv_obj, get_events_string(events));
    
    std::map<int, fd_info_t>::iterator fd_info_itr = _fd_info_map.find(sock_fd);
    if (fd_info_itr != _fd_info_map.end())
    {
        p_fd_info = &(fd_info_itr->second);
        
        if (p_fd_info->pv_obj != pv_obj)
        {
            log_error("sock_fd:%d p_fd_info->pv_obj:%p pv_obj:%p ", sock_fd, p_fd_info->pv_obj, pv_obj);
            if (!is_tcp_server(pv_obj))
            {
                CTcpConnection * p_tcp_conn = (CTcpConnection *)(p_fd_info->pv_obj);
                p_tcp_conn->ResetObject();
                p_tcp_conn->SetUsingStatus(0);
                TcpConnectionManager::Instance()->ReleaseOneObject(p_tcp_conn);
            }
        }
        
        p_fd_info->pv_obj = pv_obj;
        p_fd_info->events = events;
    }
    else
    {
        fd_info.fd = sock_fd;
        fd_info.events = events;
        fd_info.pv_obj = pv_obj;
        _fd_info_map[sock_fd] = fd_info;
    }
    
    epoll_event_obj.events = events;
    epoll_event_obj.data.fd = sock_fd;
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, sock_fd, &epoll_event_obj) < 0)
    {
        if (errno == EEXIST)
        {
            epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, sock_fd, &epoll_event_obj);
        }
        else
        {
            log_error("add sock_fd:%d to event loop fail : %s ", sock_fd, strerror(errno));
            return -1;
        }
    }
    
    return 1;
}

int CEventPoll::DeleteFromEventLoop(int sock_fd)
{
    struct epoll_event epoll_event_obj;
    
    if (_epoll_fd < 0)
    {
        log_error("the event_poll is not initialized ");
        return -1;
    }
    
    log_debug("delete sock_fd:%d from event loop ", sock_fd);
    
    std::map<int, fd_info_t>::iterator fd_info_itr = _fd_info_map.find(sock_fd);
    if (fd_info_itr == _fd_info_map.end())
    {
        log_warning("sock_fd:%d is not in _fd_info_map ", sock_fd);
    }
    else
    {
        _fd_info_map.erase(sock_fd);
    }
    
    epoll_event_obj.events = 0;
    epoll_event_obj.data.fd = sock_fd;
    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, sock_fd, &epoll_event_obj); //在EPOLL_CTL_DEL时实际上并不需要第四个参数，可置为NULL
    
    return 1;
}

int CEventPoll::StartMonitoringEvents(int sock_fd, void * pv_obj, uint32_t events)
{
    struct epoll_event epoll_event_obj;
    int errno_cache = 0;
    fd_info_t * p_fd_info = NULL;
    
    if (_epoll_fd < 0)
    {
        log_error("the event_poll is not initialized ");
        return -1;
    }
    
    log_debug("sock_fd:%d pv_obj:%p events:%s ", sock_fd, pv_obj, get_events_string(events));
    
    std::map<int, fd_info_t>::iterator fd_info_itr = _fd_info_map.find(sock_fd);
    if (fd_info_itr == _fd_info_map.end())
    {
        return Add2EventLoop(sock_fd, pv_obj, events);
    }
    
    p_fd_info = &(fd_info_itr->second);
    p_fd_info->events |= events;
    
    log_debug("sock_fd:%d pv_obj:%p p_fd_info->events:%s ", sock_fd, pv_obj, get_events_string(p_fd_info->events));
    
    epoll_event_obj.events = p_fd_info->events;
    epoll_event_obj.data.fd = sock_fd;
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, sock_fd, &epoll_event_obj) < 0)
    {
        errno_cache = errno;
        if (errno_cache == ENOENT)
        {
            if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, sock_fd, &epoll_event_obj) < 0)
            {
                errno_cache = errno;
                log_error("add sock_fd:%d to event loop fail : %s ", sock_fd, strerror(errno_cache));
                return -1;
            }
        }
        else
        {
            log_error("modify monitoring events of sock_fd:%d fail : %s ", sock_fd, strerror(errno_cache));
            return -1;
        }
    }
    
    return 1;
}

int CEventPoll::StopMonitoringEvents(int sock_fd, void * pv_obj, uint32_t events)
{
    fd_info_t * p_fd_info = NULL;
    struct epoll_event epoll_event_obj;
    int errno_cache = 0;
    
    if (_epoll_fd < 0)
    {
        log_error("the event_poll is not initialized ");
        return -1;
    }
    
    std::map<int, fd_info_t>::iterator fd_info_itr = _fd_info_map.find(sock_fd);
    if (fd_info_itr == _fd_info_map.end())
    {
        log_error("sock_fd:%d is not in _fd_info_map ", sock_fd);
        return -1;
    }
    
    p_fd_info = &(fd_info_itr->second);
    p_fd_info->events &= ~events;
    
    log_debug("sock_fd:%d pv_obj:%p p_fd_info->events:%s ", sock_fd, pv_obj, get_events_string(p_fd_info->events));
    
    epoll_event_obj.events = p_fd_info->events;
    epoll_event_obj.data.fd = sock_fd;
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, sock_fd, &epoll_event_obj) < 0)
    {
        errno_cache = errno;
        log_error("modify monitoring events of sock_fd:%d fail : %s ", sock_fd, strerror(errno_cache));
        return -1;
    }
    
    return 1;
}

int CEventPoll::DealServerSocketEvents(int sock_fd, CTcpServer * p_tcp_server, uint32_t events)
{
    log_debug("sock_fd:%d events:%s ", sock_fd, get_events_string(events));
    
    if (events & EPOLLERR)
    {
        return p_tcp_server->OnSockError();
    }
    else if (events & EPOLLIN)
    {
        return p_tcp_server->OnNewConnArrived();
    }
    else
    {
        return 0;
    }
}

int CEventPoll::DealDataSocketEvents(int sock_fd, CTcpConnection * p_tcp_connection, uint32_t events)
{
    int deal_data_len = 0;
    int conn_fd = p_tcp_connection->GetSockFd();
    
    log_debug("sock_fd:%d tcp_conn:%p conn_fd:%d events:%s ",
               sock_fd, p_tcp_connection, conn_fd, get_events_string(events));
    
    if (sock_fd != conn_fd || p_tcp_connection->GetUsingStatus() != 1)
    {   // 正常情况下不可能出现
        log_crit("sock_fd:%d tcp_conn:%p conn_fd:%d events:%s error ",
                 sock_fd, p_tcp_connection, conn_fd, get_events_string(events));
        
        if (sock_fd != conn_fd)
        {
            DeleteFromEventLoop(sock_fd);
            close(sock_fd);
        }
        
        p_tcp_connection->OnSockError();
        return -1;
    }
    
    if (events & (EPOLLERR|EPOLLHUP|EPOLLRDHUP))
    {
        return p_tcp_connection->OnSockError();
    }
    
    if (events & EPOLLOUT)
    {
        int write_len = p_tcp_connection->OnCanWrite();
        if (write_len < 0)
        {
            return write_len;
        }
        deal_data_len += write_len;
    }
    
    if (events & EPOLLIN)
    {
        int read_len = p_tcp_connection->OnCanRead();
        if (read_len < 0)
        {
            return read_len;
        }
        deal_data_len += read_len;
    }
    
    return deal_data_len;
}

int CEventPoll::RunEventLoop(int wait_time) // wait_time 的单位是毫秒
{
    int events_cnt = 0;
    int i = 0;
    
    if (_epoll_fd < 0)
    {
        log_error("the event_poll is not initialized ");
        return -1;
    }
    
    // 当前状态下只可能有 EINTR 的错误
    events_cnt = epoll_wait(_epoll_fd, _event_array, MAX_EVENTS_CNT, wait_time);
    if (events_cnt > 0)
    {
        for (i=0; i<events_cnt; i++)
        {
            int sock_fd = _event_array[i].data.fd;
            void * pv_obj = NULL;
            fd_info_t * p_fd_info = NULL;
            
            std::map<int, fd_info_t>::iterator fd_info_itr = _fd_info_map.find(sock_fd);
            if (fd_info_itr == _fd_info_map.end())
            {
                log_error("sock_fd:%d is not in _fd_info_map ", sock_fd);
                DeleteFromEventLoop(sock_fd);
                continue;
            }
            p_fd_info = &(fd_info_itr->second);
            pv_obj = p_fd_info->pv_obj;
            
            if (sock_fd != p_fd_info->fd)
            {
                log_error("sock_fd:%d not equal p_fd_info->fd:%d ", sock_fd, p_fd_info->fd);
                DeleteFromEventLoop(sock_fd);
                continue;
            }
            
            // log_debug("sock_fd:%d pv_obj:%p events:%s _fd_info_map.size:%d ", sock_fd, pv_obj, get_events_string(_event_array[i].events), _fd_info_map.size());
            
            if (is_tcp_server(pv_obj)) //链接未建立，建立链接
            {
                DealServerSocketEvents(sock_fd, (CTcpServer *)pv_obj, _event_array[i].events);
                continue;
            }
            else
            {
                DealDataSocketEvents(sock_fd, (CTcpConnection *)pv_obj, _event_array[i].events);
                continue;
            }
        }
    }
    
    return events_cnt;
}


int CEventPoll::DumpActiveSockFdInfo(char * buffer, int buffer_len)
{
    fd_info_t * p_fd_info = NULL;
    int printed_len = 0;
    int active_fd_cnt = _fd_info_map.size();
    std::map<int, fd_info_t>::iterator itr = _fd_info_map.begin();
    
    log_debug("active_fd_cnt:%d", active_fd_cnt);
    
    for (; itr != _fd_info_map.end(); itr++)
    {
        p_fd_info = &(itr->second);
        
        if (is_tcp_server(p_fd_info->pv_obj))
        {
            continue;
        }
        
        CTcpConnection * p_tcp_conn = (CTcpConnection *)p_fd_info->pv_obj;
        printed_len += p_tcp_conn->DumpActiveConnInfo(&buffer[printed_len], buffer_len-printed_len);
    }
    
    return printed_len;
}




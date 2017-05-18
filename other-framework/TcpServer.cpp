#include "ObjectManager.h"
#include "EventPoll.h"
#include "timer_set.h"
#include "TcpConnection.h"
#include "TcpServer.h"

std::set<void *> g_server_set;
int is_tcp_server(void * pv_obj)
{
    if (g_server_set.find(pv_obj) != g_server_set.end())
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

CTcpServer::CTcpServer(void)
{
    _local_ip = 0;
    _local_port = 0;
    _sock_fd = -1;
    _p_event_poll = NULL;
    _p_init_timer = NULL;
    _p_timer_set = NULL;
    _svr_id = -1;
    _svr_type = -1;
}

CTcpServer::~CTcpServer(void)
{
    if (_sock_fd >= 0)
    {
        if (_p_event_poll != NULL)
        {
            _p_event_poll->DeleteFromEventLoop(_sock_fd);
        }
        close(_sock_fd);
    }
    
    if (_p_timer_set != NULL && _p_init_timer != NULL)
    {
        destroy_one_timer(_p_timer_set, _p_init_timer->i_timer_id);
    }
}

int CTcpServer::Init(void)
{
    int flags = 1;
    int sock_fd = -1;
    int errno_cached = 0;
    struct sockaddr_in local_address;
    int address_len = sizeof(struct sockaddr_in);
    
    if (_local_port == 0 || _p_event_poll == NULL || _p_timer_set == NULL || _svr_id == -1 || _svr_type == -1)
    {
        return -1;
    }
    
    if (_sock_fd >= 0)
    {
        return _sock_fd;
    }
    
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    errno_cached = errno;
    if (sock_fd < 0)    
    {        
        log_error("create socket fail, local(%s:%u) : %s ", _local_ip_string, ntohs(_local_port), strerror(errno_cached));
        return -1;    
    }
    
    flags = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));
    setsockopt(sock_fd, SOL_TCP, TCP_NODELAY, &flags, sizeof(flags));
    fcntl(sock_fd, F_SETFL, fcntl(sock_fd, F_GETFL, 0)|O_NONBLOCK);
    
    if (_local_port != 0)
    {
        memset(&local_address, 0, sizeof(struct sockaddr_in));
        local_address.sin_family = AF_INET;
        local_address.sin_port = _local_port;
        local_address.sin_addr.s_addr = _local_ip;
        
        if (bind(sock_fd, (struct sockaddr *)&local_address, address_len) < 0)
        {
            errno_cached = errno;
            log_error("bind sock_fd:%d to %s:%u failed, %s", sock_fd, _local_ip_string, ntohs(_local_port), strerror(errno_cached));
            close(sock_fd);
            return -1;
        }
    }
    
    // 当前的实现中 backlog 是没有意义的
    if (listen(sock_fd, 64) < 0)
    {
        errno_cached = errno;
        log_error("listen sock_fd:%d failed, %s", sock_fd, strerror(errno_cached));
        close(sock_fd);
        return -1;
    }
    
    if (g_server_set.find(this) == g_server_set.end())
    {
       g_server_set.insert(this); 
    }
    
    _p_event_poll->Add2EventLoop(sock_fd, this, EPOLLIN);
    
    _sock_fd = sock_fd;
    
    return _sock_fd;
}

static int init_server_timer_callback(void * pv_user_timer)
{
    user_timer_t * p_user_timer = (user_timer_t *)pv_user_timer;
    CTcpServer * p_tcp_server = (CTcpServer *)(p_user_timer->pv_param1);
    
    if (p_tcp_server->Init() < 0)
    {
        p_tcp_server->_p_init_timer->u32_loop_cnt++;   // 给定时器的触发次数加 1 --- 相当于重置定时器
    }
    else
    {
        p_tcp_server->_p_init_timer = NULL;
    }
    
    return 0;
}

int CTcpServer::CreateInitTimer(void)
{
    user_timer_t user_timer;

    init_user_timer(user_timer, 1, 1000, init_server_timer_callback, this, NULL, NULL, NULL, NULL);
    
    _p_init_timer = create_one_timer(_p_timer_set, &user_timer);
    
    return _p_init_timer->i_timer_id;
}

int CTcpServer::OnNewConnArrived(void)
{
    struct sockaddr_in peer_address;
    socklen_t address_len = sizeof(struct sockaddr_in);
    int sock_fd = -1;
    int retry_times = 0;
    int errno_cached = 0;
    
    while (1)
    {
        sock_fd = accept(_sock_fd, (struct sockaddr *)&peer_address, &address_len);
        errno_cached = errno;
        if (sock_fd < 0)
        {
            if (errno_cached == EINTR)
            {
                if (retry_times < 5)
                {
                    retry_times++;
                    continue;
                }
                else
                {
                    return 0;
                }
            }
            else if (errno_cached == EAGAIN) //非阻塞模式 等待接收队列为空 立即返回
            {
                return 0;
            }
            else
            {
                log_error("accept client connecting fail : %s ", strerror(errno_cached));
                return -1;
            }
        }
        else
        {
            CTcpConnection * p_tcp_conn = TcpConnectionManager::Instance()->AllocateOneObject();
            if (p_tcp_conn == NULL)
            {
                log_error("no free CTcpConnection object ");
                return 0;
            }
            p_tcp_conn->SetUsingStatus(1);
            
            p_tcp_conn->ResetObject();
            p_tcp_conn->SetRemoteIp(peer_address.sin_addr.s_addr);
            p_tcp_conn->SetRemotePort(peer_address.sin_port);
            p_tcp_conn->SetLocalIp(_local_ip);
            p_tcp_conn->SetLocalPort(_local_port);
            p_tcp_conn->SetSockFd(sock_fd);
            p_tcp_conn->SetConnectDone();
            p_tcp_conn->SetEventPoll(_p_event_poll);
            
            log_info("accept tcp_conn:%p sock_fd:%d address %s:%u ", p_tcp_conn, sock_fd,
                      p_tcp_conn->GetRemoteIpString(), ntohs(p_tcp_conn->GetRemotePort()));
            
            p_tcp_conn->Init();
            p_tcp_conn->OnConnected();
            _p_event_poll->Add2EventLoop(sock_fd, p_tcp_conn, EPOLLIN);
            
            continue;
        }
    }
}

int CTcpServer::OnSockError(void)
{
    _p_event_poll->DeleteFromEventLoop(_sock_fd);
    close(_sock_fd);
    _sock_fd = -1;
    CreateInitTimer();  // 等 1s 之后再重新初始化
    return 0;
}



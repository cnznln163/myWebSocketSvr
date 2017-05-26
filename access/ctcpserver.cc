#include "ctcpserver.h"
#include "objectmanger.h"
#include "ctcpconnection.h"

std::set<void *> g_server_set;

int is_tcp_server(void * pv_obj){
    if (g_server_set.find(pv_obj) != g_server_set.end()){
        return 1;
    }else{
        return 0;
    }
}

CTcpserver::CTcpserver(){
	memset(_local_ip_string,0,16);
	_local_port = 0;
	_p_event_poll = NULL;
	_svrId = -1;
	_svrType = -1;
	_sock = -1;
}

CTcpserver::~CTcpserver(){
	if(_sock>=0){
		if (_p_event_poll != NULL){
            _p_event_poll->delEvent(_sock);
        }
        close(_sock);
	}
}

int CTcpserver::init(){
	int flags = 1;
    int sock_fd = -1;
    int errno_cached = 0;
    struct sockaddr_in local_address;
    int address_len = sizeof(struct sockaddr_in);
    
    if (_local_port == 0 || _p_event_poll == NULL || _svrId == -1 || _svrType == -1){
        return -1;
    }
    
    if (_sock >= 0){
        return _sock;
    }
    
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    errno_cached = errno;
    if (sock_fd < 0){        
        log_write(LOG_ERR,"create socket fail, local(%s:%u) : %s ", _local_ip_string, _local_port, strerror(errno_cached));
        return -1;    
    }
    
    flags = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));
    setsockopt(sock_fd, SOL_TCP, TCP_NODELAY, &flags, sizeof(flags));
    fcntl(sock_fd, F_SETFL, fcntl(sock_fd, F_GETFL, 0)|O_NONBLOCK);
    
    memset(&local_address, 0, sizeof(struct sockaddr_in));
    local_address.sin_family = AF_INET;
    local_address.sin_port = htons(_local_port);
    local_address.sin_addr.s_addr = inet_addr(_local_ip_string);
    
    if (bind(sock_fd, (struct sockaddr *)&local_address, address_len) < 0){
        errno_cached = errno;
        log_write(LOG_ERR,"bind sock_fd:%d to %s:%u failed, %s", sock_fd, _local_ip_string, _local_port, errno_cached);
        close(sock_fd);
        return -1;
    }
    int kernel_buffer_size = 0x10000;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &kernel_buffer_size, sizeof(kernel_buffer_size)) != 0){//设置接收缓冲区大小为200K
        perror("set sync SOL_SOCKET:SO_RCVBUF fail ");
        return 0;
    }
    // 当前的实现中 backlog 是没有意义的
    if (listen(sock_fd, 64) < 0){
        errno_cached = errno;
        log_write(LOG_ERR,"listen sock_fd:%d failed, %s", sock_fd, strerror(errno_cached));
        close(sock_fd);
        return -1;
    }
    if (g_server_set.find(this) == g_server_set.end()){
       g_server_set.insert(this); 
    }
    
    _p_event_poll->addEvent(sock_fd, this, EPOLLIN);
    
    _sock = sock_fd;
    
    return _sock;
}

int CTcpserver::onNewConnArrived(void){
    struct sockaddr_in peer_address;
    socklen_t address_len = sizeof(struct sockaddr_in);
    int sock_fd = -1;
    int retry_times = 0;
    int errno_cached = 0;
    
    while (1){
        sock_fd = accept(_sock, (struct sockaddr *)&peer_address, &address_len);
        errno_cached = errno;
        if (sock_fd < 0){
            if (errno_cached == EINTR){
                if (retry_times < 5){
                    retry_times++;
                    continue;
                }else{
                    return 0;
                }
            }else if (errno_cached == EAGAIN){ //非阻塞模式 等待接收队列为空 立即返回
                return 0;
            }else{
                log_write(LOG_ERR,"accept client connecting fail : %s ", strerror(errno_cached));
                return -1;
            }
        }else{
            CTcpConnection * p_tcp_conn = TcpConnectionManager::Instance()->AllocateOneObject();
            if (p_tcp_conn == NULL){
                log_write(LOG_ERR,"no free CTcpConnection object ");
                return 0;
            }
            p_tcp_conn->setUsingStatus(1);
            
            p_tcp_conn->resetObject();
            p_tcp_conn->setRemoteIp(inet_ntoa(peer_address.sin_addr));
            p_tcp_conn->setRemotePort(ntohs(peer_address.sin_port));
            p_tcp_conn->setLocalIp(_local_ip_string);
            p_tcp_conn->setLocalPort(_local_port);
            p_tcp_conn->setSockFd(sock_fd);
            p_tcp_conn->setConnectDone();
            p_tcp_conn->setTcpEvent(_p_event_poll);
            
            log_write(LOG_INFO,"accept tcp_conn:%p sock_fd:%d address %s:%u ", p_tcp_conn, sock_fd,
                      p_tcp_conn->getRemoteIp(), p_tcp_conn->getRemotePort());
            
            p_tcp_conn->init();
            p_tcp_conn->onConnected();
            _p_event_poll->addEvent(sock_fd, p_tcp_conn, EPOLLIN);
            
            continue;
        }
    }
}

int CTcpserver::onSockError(void){
    _p_event_poll->delEvent(_sock);
    close(_sock);
    _sock = -1;
    return 0;
}

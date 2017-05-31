#include "ctcpevent.h"
#include "log.h"
#include "ctcpserver.h"
#include "ctcpconnection.h"


#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0x2000
#endif

static char * get_events_string(uint32_t events){
    static char events_string[512];
    int index = 0;
    
    memset(events_string, 0, sizeof(events_string));
    
    if (events & EPOLLERR){
        strcpy(&events_string[index], "EPOLLERR");
        index += strlen("EPOLLERR");
    }
    
    if (events & EPOLLHUP){
        if (index > 0){
            events_string[index++] = '|';
        }
        
        strcpy(&events_string[index], "EPOLLHUP");
        index += strlen("EPOLLHUP");
    }
    
    if (events & EPOLLRDHUP){
        if (index > 0){
            events_string[index++] = '|';
        }
        
        strcpy(&events_string[index], "EPOLLRDHUP");
        index += strlen("EPOLLRDHUP");
    }
    
    if (events & EPOLLIN){
        if (index > 0){
            events_string[index++] = '|';
        }
        
        strcpy(&events_string[index], "EPOLLIN");
        index += strlen("EPOLLIN");
    }
    
    if (events & EPOLLOUT){
        if (index > 0){
            events_string[index++] = '|';
        }
        
        strcpy(&events_string[index], "EPOLLOUT");
        index += strlen("EPOLLOUT");
    }

    return events_string;
}

CTcpevent::CTcpevent(){
	_evfd = -1;
	memset(_event_array, 0, sizeof(struct epoll_event)*MAX_EVENTS);
}

CTcpevent::~CTcpevent(void){
    if (_evfd >= 0){
        close(_evfd);
    }
}

/*
int CTcpevent::createSocket( char *ip , int port ){
	struct sockaddr_in local_address;
	
	memcpy();
	_port = port;

	int optval = 1; // 关闭之后重用socket
    unsigned int optlen = sizeof (optval);
	//	创建socket，并绑定到固定端口
	_sfd	=	socket( AF_INET , SOCK_STREAM , 0 );
	if( _sfd < 0 ){
		log_write( LOG_ERR ," 创建socket失败 , %s , %d " , __FILE__ , __LINE__ );
		return 0;
	}
	setsockopt(_sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optlen)); //端口重用，如意外没有释放端口，还可以绑定成功
	memset(&local_address, 0, sizeof(struct sockaddr_in));
    local_address.sin_family = AF_INET;
    local_address.sin_port = htons(port);
    local_address.sin_addr.s_addr = inet_addr(ip);
	if (bind(_sfd, (struct sockaddr*) & local_address, sizeof (local_address)) < 0) {
        log_write(LOG_ERR, "bind error, %s, %d", __FILE__, __LINE__);
		return 0;
    }
	int kernel_buffer_size = 0x10000;
	if (setsockopt(_sfd, SOL_SOCKET, SO_RCVBUF, &kernel_buffer_size, sizeof(kernel_buffer_size)) != 0){//设置接收缓冲区大小为200K
		perror("set sync SOL_SOCKET:SO_RCVBUF fail ");
		return 0;
	}
	if (listen(_sfd, 50) < 0) {
        log_write(LOG_ERR, "listen error, %s, %d", __FILE__, __LINE__);
        return 0;
    }
	if( fcntl(_sfd , F_SETFL , fcntl(sfd , F_GETFD , 0)|O_NONBLOCK) == -1   ){
		log_write(LOG_ERR, "set listen fd nonblock error, %s, %d", __FILE__, __LINE__);
        return 0;
	}
	return 1;
}
*/
bool CTcpevent::init(){
	// 创建epoll
	_evfd  = epoll_create( MAX_FDS );
	if( _evfd<0 ){
		return false;
	}
	return true;
}

int CTcpevent::addEvent(int sock_fd, void * pv_obj, uint32_t events){
    struct epoll_event epoll_event_obj;
    fd_info_t fd_info;
    fd_info_t * p_fd_info = NULL;
    
    if (_evfd < 0){
        log_write(LOG_ERR,"the event_poll is not initialized ");
        return 0;
    }
    
    log_write(LOG_DEBUG,"add sock_fd:%d pv_obj:%p events:%s to event loop ", sock_fd, pv_obj, get_events_string(events));
    
    std::map<int, fd_info_t>::iterator fd_info_itr = _fd_info_map.find(sock_fd);
    if (fd_info_itr != _fd_info_map.end()){
        p_fd_info = &(fd_info_itr->second);
        if (p_fd_info->pv_obj != pv_obj){
            log_write(LOG_ERR,"sock_fd:%d p_fd_info->pv_obj:%p pv_obj:%p ", sock_fd, p_fd_info->pv_obj, pv_obj);
            /*
            if (!is_tcp_server(pv_obj)){
                CTcpConnection * p_tcp_conn = (CTcpConnection *)(p_fd_info->pv_obj);
                p_tcp_conn->ResetObject();
                p_tcp_conn->SetUsingStatus(0);
                TcpConnectionManager::Instance()->ReleaseOneObject(p_tcp_conn);
            }
            */
        }
        
        p_fd_info->pv_obj = pv_obj;
        p_fd_info->events = events;
    }else{
        fd_info.fd = sock_fd;
        fd_info.events = events;
        fd_info.pv_obj = pv_obj;
        _fd_info_map[sock_fd] = fd_info;
    }
    
    epoll_event_obj.events = events;
    epoll_event_obj.data.fd = sock_fd;
    if (epoll_ctl(_evfd, EPOLL_CTL_ADD, sock_fd, &epoll_event_obj) < 0){
        if (errno == EEXIST){
            epoll_ctl(_evfd, EPOLL_CTL_MOD, sock_fd, &epoll_event_obj);
        }else{
            log_write(LOG_ERR,"add sock_fd:%d to event loop fail : %s ", sock_fd, strerror(errno));
            return 0;
        }
    }
    
    return 1;
}

int CTcpevent::delEvent(int sock_fd){
    struct epoll_event epoll_event_obj;
    
    if (_evfd < 0){
        log_write(LOG_ERR,"the event_poll is not initialized ");
        return 0;
    }
    
    log_write(LOG_DEBUG,"delete sock_fd:%d from event loop ", sock_fd);
    
    std::map<int, fd_info_t>::iterator fd_info_itr = _fd_info_map.find(sock_fd);
    if (fd_info_itr == _fd_info_map.end()){
        log_write(LOG_ERR,"sock_fd:%d is not in _fd_info_map ", sock_fd);
    }else{
        _fd_info_map.erase(sock_fd);
    }
    
    epoll_event_obj.events = 0;
    epoll_event_obj.data.fd = sock_fd;
    epoll_ctl(_evfd, EPOLL_CTL_DEL, sock_fd, &epoll_event_obj); //在EPOLL_CTL_DEL时实际上并不需要第四个参数，可置为NULL
    
    return 1;
}

int CTcpevent::serverSocketEvents(int sock_fd, CTcpserver * p_tcp_server, uint32_t events){
	log_write(LOG_DEBUG,"sock_fd:%d events:%s ", sock_fd, get_events_string(events));
    if (events & EPOLLERR){
        return p_tcp_server->onSockError();
    }else if (events & EPOLLIN){
        return p_tcp_server->onNewConnArrived();
    }else{
        return 0;
    }
}

int CTcpevent::processEvent(int wait_time){
	int events_cnt = 0;
    int i = 0;
    
    if (_evfd < 0){
        log_write(LOG_ERR,"the event_poll is not initialized ");
        return -1;
    }
    
    // 当前状态下只可能有 EINTR 的错误
    events_cnt = epoll_wait(_evfd, _event_array, MAX_EVENTS, wait_time);
    if (events_cnt > 0){
        for (i=0; i<events_cnt; i++){
            int sock_fd = _event_array[i].data.fd;
            void * pv_obj = NULL;
            fd_info_t * p_fd_info = NULL;
            
            std::map<int, fd_info_t>::iterator fd_info_itr = _fd_info_map.find(sock_fd);
            if (fd_info_itr == _fd_info_map.end()){
                log_write(LOG_ERR,"sock_fd:%d is not in _fd_info_map ", sock_fd);
                delEvent(sock_fd);
                continue;
            }
            p_fd_info = &(fd_info_itr->second);
            pv_obj = p_fd_info->pv_obj;
            
            if (sock_fd != p_fd_info->fd){
                log_write(LOG_ERR,"sock_fd:%d not equal p_fd_info->fd:%d ", sock_fd, p_fd_info->fd);
                delEvent(sock_fd);
                continue;
            }
            
            // log_debug("sock_fd:%d pv_obj:%p events:%s _fd_info_map.size:%d ", sock_fd, pv_obj, get_events_string(_event_array[i].events), _fd_info_map.size());
            
            if (is_tcp_server(pv_obj)){ //链接未建立，建立链接
                dealServerSocketEvents(sock_fd, (CTcpserver *)pv_obj, _event_array[i].events);
                continue;
            }else{
                dealDataSocketEvents(sock_fd, (CTcpConnection *)pv_obj, _event_array[i].events);
                continue;
            }
        }
    }
    
    return events_cnt;
}

int CTcpevent::dealServerSocketEvents(int sock_fd, CTcpserver * p_tcp_server, uint32_t events){
    log_write(LOG_DEBUG,"sock_fd:%d events:%s ", sock_fd, get_events_string(events));
    
    if (events & EPOLLERR){
        return p_tcp_server->onSockError();
    }else if (events & EPOLLIN){
        return p_tcp_server->onNewConnArrived();
    }else{
        return 0;
    }
}

int CTcpevent::dealDataSocketEvents(int sock_fd, CTcpConnection * p_tcp_connection, uint32_t events){
    int deal_data_len = 0;
    int conn_fd = p_tcp_connection->getSockFd();
    
    log_write(LOG_DEBUG,"sock_fd:%d tcp_conn:%p conn_fd:%d events:%s \n", sock_fd, p_tcp_connection, conn_fd, get_events_string(events));
    
    if (sock_fd != conn_fd || p_tcp_connection->getUsingStatus() != 1){   // 正常情况下不可能出现
        log_write(LOG_ERR,"sock_fd:%d tcp_conn:%p conn_fd:%d events:%s error ", sock_fd, p_tcp_connection, conn_fd, get_events_string(events));
        
        if (sock_fd != conn_fd){
            delEvent(sock_fd);
            close(sock_fd);
        }
        p_tcp_connection->onSockError();
        return -1;
    }
    
    if (events & (EPOLLERR|EPOLLHUP|EPOLLRDHUP)){
        return p_tcp_connection->onSockError();
    }
    
    if (events & EPOLLOUT){
        /*
        int write_len = p_tcp_connection->onCanWrite();
        if (write_len < 0){
            return write_len;
        }
        */
        //deal_data_len += write_len;
    }
    
    if (events & EPOLLIN){
        int read_len = p_tcp_connection->onCanRead();
        if (read_len < 0){
            return read_len;
        }
        deal_data_len += read_len;
    }
    
    return deal_data_len;
}

int CTcpevent::stopMonitoringEvents(int sock_fd, void * pv_obj, uint32_t events){
    fd_info_t * p_fd_info = NULL;
    struct epoll_event epoll_event_obj;
    int errno_cache = 0;
    
    if (_evfd < 0){
        log_write(LOG_ERR,"the event_poll is not initialized ");
        return -1;
    }
    
    std::map<int, fd_info_t>::iterator fd_info_itr = _fd_info_map.find(sock_fd);
    if (fd_info_itr == _fd_info_map.end()){
        log_write(LOG_ERR,"sock_fd:%d is not in _fd_info_map ", sock_fd);
        return -1;
    }
    
    p_fd_info = &(fd_info_itr->second);
    p_fd_info->events &= ~events;
    
    log_write(LOG_DEBUG,"sock_fd:%d pv_obj:%p p_fd_info->events:%s ", sock_fd, pv_obj, get_events_string(p_fd_info->events));
    
    epoll_event_obj.events = p_fd_info->events;
    epoll_event_obj.data.fd = sock_fd;
    if (epoll_ctl(_evfd, EPOLL_CTL_MOD, sock_fd, &epoll_event_obj) < 0){
        errno_cache = errno;
        log_write(LOG_ERR,"modify monitoring events of sock_fd:%d fail : %s ", sock_fd, strerror(errno_cache));
        return -1;
    }
    return 1;
}

void setNonblockingSocket(int fd){
	if( fcntl(fd , F_SETFL , fcntl(fd , F_GETFD , 0)|O_NONBLOCK) == -1 ){
		 log_write(LOG_ERR,"set_nonblocking_socket fail:%d\n",fd);
	}
}

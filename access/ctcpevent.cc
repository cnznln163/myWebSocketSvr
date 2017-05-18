#include "ctcpevent.h"
#include "log.h"


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
    
    if (events & EPOLLRDHUP{
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


int CTcpevent::createSocket( char *ip , int port ){
	struct sockaddr_in local_address;
	
	_ip = ip;
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
bool CTcpevent::init(){
	// 创建epoll
	_epfd  = epoll_create( MAX_FDS );
	if( _epfd<0 ){
		return false;
	}
	return true;
}

int CEventPoll::addEvent(int sock_fd, void * pv_obj, uint32_t events){
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

int CEventPoll::delEvent(int sock_fd){
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

int CTcpevent::serverSocketEvents(int sock_fd, CTcpServer * p_tcp_server, uint32_t events){
	log_write(LOG_DEBUG,"sock_fd:%d events:%s ", sock_fd, get_events_string(events));
    if (events & EPOLLERR){
        return p_tcp_server->OnSockError();
    }else if (events & EPOLLIN){
        return p_tcp_server->OnNewConnArrived();
    }else{
        return 0;
    }
}

int CTcpevent::processEvent(){
	struct sockaddr_in peer_address;
    socklen_t address_len = sizeof(struct sockaddr_in);
	listener *c;
	struct event *event;
	struct epoll_event events[MAX_EVENTS];
	int new_fd,nfds,i;
	char peer_ip[16]={0};
	unsigned short int peer_port;
	int status;
	
	while(true){
		//等待epoll事件的发生
        nfds = epoll_wait(_evfd, events, MAX_EVENTS, 500); //没有任何事件时500ms返回
        for(i=0; i<nfds;i++){
			event = (struct event *)events[i].data.ptr;
			if( event->type == event_listener ){//accpet
				new_fd = accept(event->fd,(struct sockaddr* )&peer_address, &address_len);
                if(new_fd<0){
					log_write(LOG_ERR ,"new-accept-fail:fd-%d\n", new_fd );
					continue;
                }
				setNonblockingSocket(new_fd);
				
				snprintf(peer_ip,sizeof(peer_ip),"%s",inet_ntoa(peer_address.sin_addr));
				peer_ip[sizeof(peer_ip)] = 0;
				peer_port = peer_address.sin_port;
				
				c = get_new_listener(new_fd,peer_ip,peer_port,BUFFER_SIZE);
				if( !c ){
					log_write(LOG_ERR ,"new-accept-fail:alloc data fd-%d\n", new_fd );
					close_socket(new_fd);
				}
				add_event(new_fd,event_normal,EPOLLIN,c);
			}else if( event->type == event_signals && (events[i].events & EPOLLIN) ){
				main_got_signal( event->fd );
			}else if( event->type == event_normal && (events[i].events & EPOLLIN)  ){	//	读操作-有客户端发消息过来
				status = read_event(event->listener);
				if( status && event->listener->status == conn_close ){
					delete_event(event);
				}
			}else{
				log_write(LOG_ERR,"Other event:%d\n",events[i].events	);
			}
		}
		handle_timer();
	}
	return 1;
}
void setNonblockingSocket(int fd){
	if( fcntl(fd , F_SETFL , fcntl(fd , F_GETFD , 0)|O_NONBLOCK) == -1 ){
		 log_write(LOG_ERR,"set_nonblocking_socket fail:%d\n",fd);	
	}
}
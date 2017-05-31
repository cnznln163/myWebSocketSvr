
#include "objectmanger.h"
#include "ctcpevent.h"
#include "cpacketsocket.h"
#include "ctcpserver.h"
#include "ctcpconnection.h"


CTcpConnection::CTcpConnection(void){
    _p_recv_packet = NULL;
    
    _p_event_poll = NULL;
    
    _p_chandler = NULL;

    _in_using = 0;
    _sock_fd = -1;
    resetObject();
}


CTcpConnection::~CTcpConnection(void){
    if (_sock_fd >= 0){
        _p_event_poll->delEvent(_sock_fd);
        close(_sock_fd);
    }
    _sock_fd = -1;
    if (_p_recv_packet != NULL){
        delete _p_recv_packet;
    }
    if( _p_chandler != NULL ){
        delete _p_chandler;
    }
    _p_recv_packet = NULL;
    _p_chandler = NULL;
}

void CTcpConnection::resetObject(void){
    _will_reconnect = 0;
    _ep_type = EP_TYPE_SERVER;
        
    reset();
}

void CTcpConnection::reset(void){
    log_write(LOG_INFO,"reset tcp_conn:%p _ep_type:%d sock_fd:%d \n", this, _ep_type, _sock_fd);
    
    if (_p_recv_packet != NULL){
        _p_recv_packet->reset();
    }
    
    if (_sock_fd >= 0){
        if (_p_event_poll != NULL){
            _p_event_poll->delEvent(_sock_fd);
        }
        close(_sock_fd);
    }
    _sock_fd = -1;
    
    if (_ep_type == EP_TYPE_SERVER){
        _remote_port = 0;
        memset(_remote_ip_string, 0, sizeof(_remote_ip_string));
    }
    
    _connection_status = CONN_STATUS_CLOSED;
    
    recv_cnt_bytes = 0;
    recv_cnt_pkts = 0;
    send_cnt_bytes = 0;
    send_cnt_pkts = 0;
    
}


int CTcpConnection::init(void){
    if (_p_recv_packet == NULL){
        _p_recv_packet = new CPacketSocket();
    }else{
        _p_recv_packet->reset();
    }
    
    if (_p_recv_packet == NULL ){
        log_write(LOG_ERR,"init context is illegal");
        return -1;
    }
    if( _p_chandler == NULL ){
        _p_chandler = new CSocketHandler();
    }
    log_write(LOG_INFO,"init tcp_conn:%p _ep_type:%d sock_fd:%d ", this, _ep_type, _sock_fd);
    
    if (_ep_type == EP_TYPE_CLIENT){
        if (_remote_port == 0){
            log_write(LOG_ERR,"tcp_conn:%p _remote_port:0", this);
            return -1;
        }
        
        createConnection();
        return 0;
    }else{
        if (_sock_fd < 0 || _local_port == 0){
            log_write(LOG_ERR,"tcp_conn:%p sock_fd or local_port is illegal", this);
            return -1;
        }
        return 0;
    }
    
}

int CTcpConnection::createConnection(void){
    struct sockaddr_in server_address, local_address;
    socklen_t address_len = sizeof(struct sockaddr_in);
    int reconnect_times = 0;
    int ret_val = 0;
    int flags = 1;
    int sock_fd = -1;
    int errno_cached = 0;
    
    if (_connection_status > CONN_STATUS_CLOSED && _sock_fd > 0){
        return _sock_fd;
    }else if (_sock_fd > 0){
        close(_sock_fd);
        _sock_fd = -1;
    }
    
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    errno_cached = errno;
    if (sock_fd < 0){        
        log_write(LOG_ERR,"create socket fail, peer(%s:%u) : %s ", _remote_ip_string, _remote_port, strerror(errno_cached));
        return -1;    
    }
    
    flags = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));
    setsockopt(sock_fd, SOL_TCP, TCP_NODELAY, &flags, sizeof(flags));
    
    
    
    fcntl(sock_fd, F_SETFL, fcntl(sock_fd, F_GETFL, 0)|O_NONBLOCK);
    
    memset(&server_address, 0, sizeof(struct sockaddr_in));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(_remote_port);
    server_address.sin_addr.s_addr = inet_addr(_remote_ip_string);
    
    log_write(LOG_DEBUG,"connect to %s:%u ", _remote_ip_string, _remote_port);
    
    reconnect_times = 0;
label_reconnect:
    ret_val = connect(sock_fd, (struct sockaddr *)&server_address, address_len);
    errno_cached = errno;
    if (ret_val < 0){
        if (errno_cached == EINTR){
            if (reconnect_times < 5){
                goto label_reconnect;
            }
        }else if (errno_cached != EINPROGRESS){
            log_write(LOG_ERR,"connect to %s:%u failed : %s ", _remote_ip_string, _remote_port, strerror(errno_cached));
            close(sock_fd);
            return -3;
        }
        
        _connection_status = CONN_STATUS_CONNECTING;
        _p_event_poll->addEvent(sock_fd, this, EPOLLOUT|EPOLLIN);
    }else{
        _connection_status = CONN_STATUS_CONNECTED;
        _p_event_poll->addEvent(sock_fd, this, EPOLLIN);
        onConnected();
    }
    _sock_fd = sock_fd;
    
    return _sock_fd;
}

int CTcpConnection::onConnected(void){
    int flags = 1;
    
    setsockopt(_sock_fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));
    setsockopt(_sock_fd, SOL_TCP, TCP_NODELAY, &flags, sizeof(flags));
    
    fcntl(_sock_fd, F_SETFL, fcntl(_sock_fd, F_GETFL, 0)|O_NONBLOCK);
    
    return 0;
}

int CTcpConnection::readData(char * buffer, int read_len, int at_least){
    int index = 0;
    int recv_len = 0;
    int recv_times = 0;
    int errno_cached = 0;
    
label_recv:
    recv_len = recv(_sock_fd, buffer+index, read_len-index, 0);
    errno_cached = errno;
    
    log_write(LOG_DEBUG,"sock_fd:%d recv data length:%d from peer %s:%d \n", _sock_fd, recv_len, _remote_ip_string, _remote_port);
    
    if (recv_len == read_len-index){
        recv_cnt_bytes += recv_len;
        return read_len;
    }else if (recv_len > 0){
        recv_cnt_bytes += recv_len;
        if (at_least <= 0){
            return recv_len;
        }
        
        index += recv_len;
        
        if (recv_times < 5 && index < at_least){
            recv_times++;
            goto label_recv;
        }else{
            return index;
        }
    }else if (recv_len == 0){
        log_write(LOG_ERR,"peer close the connection : sock_fd:%d peer(%s:%u) \n", _sock_fd, _remote_ip_string, _remote_port);
        closeConnection();
        return -1;
    }else{
        if (errno_cached == EINTR){
            if (recv_times < 5 && index < at_least){
                recv_times++;
                goto label_recv;
            }else{
                return index;
            }
        }else if (errno_cached == EAGAIN){
            return index;
        }else{
            log_write(LOG_ERR,"recv error : sock_fd:%d peer(%s:%u) : %s ", _sock_fd, _remote_ip_string, ntohs(_remote_port), strerror(errno_cached));
            closeConnection();
            return -1;
        }
    }
}

int CTcpConnection::onCanRead(void){
    int read_len = 0;
    int at_least = 0;
    int recv_len = 0;
    int data_len = 0;
    int msg_len = 0;
    int ret_val = 0;
    int inputRet = -1;
    char *buffer=NULL;
    
    read_len = _p_recv_packet->getFreeBuffSize();//还有多少字节可写
    data_len = _p_recv_packet->write_index;
    
    if (data_len == 0){
        _p_recv_packet->read_index = 0;
        _p_recv_packet->write_index = 0;
        at_least = 4;
    }else if (data_len < 4){
        at_least = 4 - data_len;
    }else{
        at_least = 0;
    }
    buffer   = _p_recv_packet->getPacketBuffer();
    recv_len = readData(buffer, read_len, at_least);
    if (recv_len <= 0){
        return recv_len;
    }
    _p_recv_packet->write_index += recv_len;
    
    data_len = _p_recv_packet->getRecvDataLen();//已接收的buff字节数量
    
    log_write(LOG_DEBUG,"tcp_conn:%p recv_len:%d data_len:%d sock_fd:%d remote(%s:%u) \n",
              this, recv_len, data_len, _sock_fd, _remote_ip_string, _remote_port);
    
    while (data_len > 4){
        msg_len = _p_recv_packet->getHeadPacketSize();//获取整个包体长度字段
        if (msg_len == -1){
            log_write(LOG_ERR,"tcp_conn:%p msg too short msg_len:%d data_len:%d sock_fd:%d remote(%s:%u) ",
                      this, msg_len, data_len, _sock_fd, _remote_ip_string, _remote_port);
            closeConnection();
            return -1;
        }else if ((msg_len+4 - data_len) > _p_recv_packet->getFreeBuffSize() ){
            log_write(LOG_ERR,"tcp_conn:%p msg too long msg_len:%d data_len:%d sock_fd:%d remote(%s:%u) \n",
                      this, msg_len, data_len, _sock_fd, _remote_ip_string, _remote_port);
            closeConnection();
            return -1;
        }
        
        log_write(LOG_DEBUG,"tcp_conn:%p recv_len:%d msg_len:%d data_len:%d sock_fd:%d remote(%s:%u) ",
                  this, recv_len, msg_len, data_len, _sock_fd, _remote_ip_string, _remote_port);
        
        if (data_len >= msg_len+4){
            recv_cnt_pkts++;
            ret_val = _p_recv_packet->parsePacket(msg_len);
            if (ret_val < 0){
                log_write(LOG_ERR,"deal msg fail : msg_len:%d sock_fd:%d remote(%s:%d) : close the connection \n",
                           msg_len, _sock_fd, _remote_ip_string, _remote_port);
                closeConnection();
                return -1;
            }
            inputRet = _p_chandler->inputNotify(this,_p_recv_packet);
            if( inputRet == 1 ){
                writeData();
            }else if( inputRet == 0 ){
                log_write(LOG_ERR, "deal input notify fail:%d ",inputRet);
            }else{
                log_write(LOG_ERR, "deal input notify fail:%d ",inputRet);
                closeConnection();
                return -1;
            }
            _p_recv_packet->read_index += msg_len+4;
            data_len -= msg_len+4;
        }else{
            break;
        }
    }
    
    if (data_len > 0 && _p_recv_packet->read_index > 0){//存在超过1个完整包以上的数据
        _p_recv_packet->movePacket(data_len);
    }
    
    if (data_len > 0){
        _p_recv_packet->write_index = data_len;
    }else{
        _p_recv_packet->write_index = 0;
    }
    
    _p_recv_packet->read_index = 0;
    
    return recv_len;
}

int CTcpConnection::sendMsgInternal(char * data_buffer, int data_len){
    int send_len = 0;
    int send_times = 0;
    int errno_cached = 0;
    
    if (data_buffer == NULL || data_len <= 0){
        log_write(LOG_ERR,"data_buffer:%p data_len:%d ", data_buffer, data_len);
        return -1;
    }
    
label_send:
    send_len = send(_sock_fd, data_buffer, data_len, 0);
    errno_cached = errno;
    
    log_write(LOG_DEBUG,"sock_fd:%d send msg length:%d to peer %s:%d ", _sock_fd, send_len, _remote_ip_string, _remote_port);
    
    if (send_len == data_len){
        return send_len;
    }else if (send_len > 0){   // 内核缓冲区满了, 但当前数据只有部分拷贝到了内核
        return send_len;
    }else if (send_len == 0 && send_times < 5){
        send_times++;
        log_write(LOG_INFO,"send_len:0 send_times:%d ", send_times);
        goto label_send;
    }else{
        if (errno_cached == EAGAIN){
            return 0;
        }else if (errno_cached == EINTR && send_times < 5){
            send_times++;
            log_write(LOG_INFO,"errno_cached == EINTR, send_times:%d ", send_times);
            goto label_send;
        }else{
            log_write(LOG_ERR,"tcp_conn:%p sock_fd:%d send data to address %s:%u failed send_times:%d : %s ",
                       this, _sock_fd, _remote_ip_string, _remote_port, send_times, strerror(errno_cached));
            closeConnection();
            return -1;
        }
    }
}

int CTcpConnection::writeData(void){
    int data_len = 0;
    int send_len = 0;
    
    if (_connection_status != CONN_STATUS_CONNECTED){
        log_write(LOG_ERR,"_connection_status != CONN_STATUS_CONNECTED ");
        return 0;
    }
    char *sendBuf = _p_recv_packet->getWriteBuff();
    data_len = _p_recv_packet->getWriteBuffLen();
    if (data_len > 0){
        send_len = sendMsgInternal(sendBuf, data_len);
        if (send_len > 0){//暂时不考虑缓存区已满，只发送部分情况
            if (send_len < data_len){// 发送缓冲中的数据未发送完全，忽略掉
                log_write(LOG_ERR, "write data not send done:send_len:%d,data_len:%d,sock_fd:%d",send_len,data_len,_sock_fd);
            }
            _p_event_poll->stopMonitoringWrite(_sock_fd, this);
        }else{
            log_write(LOG_ERR,"send_len:%d data_len:%d", send_len, data_len);
        }
        return send_len;
    }else{
        return 0;
    }
    return 1;
}

int CTcpConnection::OnCanWrite(void){

    /*
    if (_connection_status < CONN_STATUS_CONNECTED)   // 建立连接尚未成功
    {
        int sock_err = 0;
        int sock_err_len = sizeof(sock_err);
        
        if (_sock_fd < 0)
        {
            log_error("tcp_conn:%p have not created connection to %s:%u ", this, _remote_ip_string, ntohs(_remote_port));
            return -1;
        }
        
        getsockopt(_sock_fd, SOL_SOCKET, SO_ERROR, &sock_err, (socklen_t*)&sock_err_len);
        if (sock_err != 0)
        {
            log_error("tcp_conn:%p connect to %s:%u failed, %s ", this, _remote_ip_string, ntohs(_remote_port), strerror(sock_err));
            closeConnection();
            return -1;
        }
        else
        {
            _connection_status = CONN_STATUS_CONNECTED;
            _p_event_poll->StartMonitoringEvents(_sock_fd, this, EPOLLIN);
            OnConnected();
        }
    }
    
    return WriteData();
    */
    return 1;
}

int CTcpConnection::SendMsg(uint8_t * msg, int msg_len){
    /*
    int send_len = 0;
    int free_space = get_buffer_free_space(_p_send_buffer);
    
    if (msg == NULL || msg_len <= 0 || msg_len > free_space)
    {
        log_error("msg:%p msg_len:%d free_space:%d", msg, msg_len, free_space);
        return -1;
    }
    
    if (_sock_fd < 0)
    {
        log_warning("_sock_fd:%d msg:%p msg_len:%d free_space:%d", _sock_fd, msg, msg_len, free_space);
                      
        if (_will_reconnect == 0)
        {
            log_error("tcp_conn:%p sock_fd:%d _will_reconnect:%d peer %s:%d ",
                      this, _sock_fd, _will_reconnect, _remote_ip_string, ntohs(_remote_port));
            
            closeConnection();
            return -1;
        }
        
        if (CreateConnection() < 0)
        {
            log_error("tcp_conn:%p create connection to peer %s:%d fail ", this, _remote_ip_string, ntohs(_remote_port));
            return -1;
        }
    }
    
    log_debug("sock_fd:%d send msg length:%d to peer %s:%d ", _sock_fd, msg_len, _remote_ip_string, ntohs(_remote_port));
    
    if (get_buffer_data_len(_p_send_buffer) == 0 && _connection_status == CONN_STATUS_CONNECTED)
    {
        send_len = SendMsgInternal(msg, msg_len);
        if (send_len < 0)
        {
            log_error("send_len:%d msg_len:%d ", send_len, msg_len);
            return send_len;
        }
        else if (send_len == msg_len)
        {
            send_cnt_pkts++;
            send_cnt_bytes += msg_len;
            
            _p_event_poll->StopMonitoringWrite(_sock_fd, this);
            
            return msg_len;
        }
        else
        {
            send_cnt_bytes += send_len;
            msg = msg + send_len;
            msg_len = msg_len - send_len;
            _p_event_poll->StartMonitoringWrite(_sock_fd, this);
        }
    }
    
    free_space = get_buffer_free_space(_p_send_buffer);
    if (free_space < msg_len)
    {
        log_error("tcp_conn:%p no enough space for writing msg sock_fd:%d remote(%s:%u) free_space:%d msg_len:%d ",
                  this, _sock_fd, _remote_ip_string, ntohs(_remote_port), free_space, msg_len);
        return -1;
    }
    
    send_cnt_pkts++;
    send_cnt_bytes += msg_len;
    
    write_buffer(_p_send_buffer, msg, msg_len);
    
    WriteData();
    
    return msg_len;
    */
    return 1;
}
    
void CTcpConnection::closeConnection(void){
    log_write(LOG_INFO,"tcp_conn:%p sock_fd:%d address %s:%u close connection : "
             "recv_cnt_bytes:%lu recv_cnt_pkts:%lu send_cnt_bytes:%lu send_cnt_pkts:%lu ",
              this, _sock_fd, _remote_ip_string, _remote_port,
              recv_cnt_bytes, recv_cnt_pkts, send_cnt_bytes, send_cnt_pkts);
    
    
    if (_sock_fd >= 0){
        _p_event_poll->delEvent(_sock_fd); 
        close(_sock_fd);
    }
    
    _connection_status = CONN_STATUS_CLOSED;
    _sock_fd = -1;
    
    if (_will_reconnect == 0){
        this->setUsingStatus(0);
        this->resetObject();
        TcpConnectionManager::Instance()->ReleaseOneObject(this);
    }else{
        reset();
    }
}

int CTcpConnection::onSockError(void){
    closeConnection();
    return 0;
}



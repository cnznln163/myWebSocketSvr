#include "ring_buffer.h"
#include "objectmanager.h"
#include "EventPoll.h"
#include "PacketProcessor.h"
#include "timer_set.h"
#include "TcpServer.h"
#include "ctcpconnection.h"

CTcpConnection::CTcpConnection(void){
    _p_recv_buffer = NULL;
    _p_send_buffer = NULL;
    
    _p_pkt_proc = NULL;
    _p_event_poll = NULL;
    
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
    if (_p_recv_buffer != NULL)
    {
        destroy_ring_buffer(_p_recv_buffer);
    }
    _p_recv_buffer = NULL;
    if (_p_send_buffer != NULL)
    {
        destroy_ring_buffer(_p_send_buffer);
    }
    _p_send_buffer = NULL;
}

void CTcpConnection::resetObject(void){
    if (_p_pkt_proc != NULL){
        _p_pkt_proc->ResetObject();
    }
    
    _will_reconnect = 0;
    _ep_type = EP_TYPE_SERVER;
        
    reset();
}

void CTcpConnection::reset(void)
{
    log_wirte(LOG_INFO,"reset tcp_conn:%p _ep_type:%d sock_fd:%d ", this, _ep_type, _sock_fd);
    
    if (_p_pkt_proc != NULL){
        _p_pkt_proc->Reset();
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
    
    if (_p_recv_buffer == NULL){
        _p_recv_buffer = create_ring_buffer(MAX_RECV_BUFF_LEN);
    }else{
        clear_ring_buffer(_p_recv_buffer);
    }
    
    if (_p_send_buffer == NULL){
        _p_send_buffer = create_ring_buffer(MAX_SEND_BUFF_LEN*2);
    }else{
        clear_ring_buffer(_p_send_buffer);
    }
}

void CTcpConnection::InitRingBuffer(uint32_t recv_buffer_size, uint32_t send_buffer_size)
{
    if (recv_buffer_size < MAX_RECV_BUFF_LEN)
    {
        recv_buffer_size = MAX_RECV_BUFF_LEN;
    }
    
    if (send_buffer_size < MAX_SEND_BUFF_LEN)
    {
        send_buffer_size = MAX_SEND_BUFF_LEN;
    }
    
    if (_p_recv_buffer != NULL)
    {
        destroy_ring_buffer(_p_recv_buffer);
        _p_recv_buffer = NULL;
    }
    
    if (_p_send_buffer != NULL)
    {
        destroy_ring_buffer(_p_send_buffer);
        _p_send_buffer = NULL;
    }
    
    _p_recv_buffer = create_ring_buffer(recv_buffer_size);
    _p_send_buffer = create_ring_buffer(send_buffer_size);
}

int CTcpConnection::init(void){
    if (_p_recv_buffer == NULL){
        _p_recv_buffer = create_ring_buffer(MAX_RECV_BUFF_LEN);
    }else{
        clear_ring_buffer(_p_recv_buffer);
    }
    
    if (_p_send_buffer == NULL){
        _p_send_buffer = create_ring_buffer(MAX_SEND_BUFF_LEN*2);
    }else{
        clear_ring_buffer(_p_send_buffer);
    }
    
    if (_p_recv_buffer == NULL || _p_send_buffer == NULL || _p_pkt_proc == NULL || _p_event_poll == NULL)
    {
        log_error("init context is illegal");
        return -1;
    }
    
    log_info("init tcp_conn:%p _ep_type:%d sock_fd:%d ", this, _ep_type, _sock_fd);
    
    if (_ep_type == EP_TYPE_CLIENT){
        if (_remote_port == 0){
            log_wirte(LOG_ERR,"tcp_conn:%p _remote_port:0", this);
            return -1;
        }
        
        createConnection();
        return _p_pkt_proc->Init();
    }else{
        if (_sock_fd < 0 || _local_port == 0){
            log_wirte(LOG_ERR,"tcp_conn:%p sock_fd or local_port is illegal", this);
            return -1;
        }
        return _p_pkt_proc->Init();
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
        log_error("create socket fail, peer(%s:%u) : %s ", _remote_ip_string, ntohs(_remote_port), strerror(errno_cached));
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

int CTcpConnection::onConnected(void)
{
    int flags = 1;
    
    setsockopt(_sock_fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));
    setsockopt(_sock_fd, SOL_TCP, TCP_NODELAY, &flags, sizeof(flags));
    
    fcntl(_sock_fd, F_SETFL, fcntl(_sock_fd, F_GETFL, 0)|O_NONBLOCK);
    
    return _p_pkt_proc->onConnected();
}

int CTcpConnection::ReadData(uint8_t * buffer, int read_len, int at_least)
{
    int index = 0;
    int recv_len = 0;
    int recv_times = 0;
    int errno_cached = 0;
    
label_recv:
    recv_len = recv(_sock_fd, buffer+index, read_len-index, 0);
    errno_cached = errno;
    
    log_debug("sock_fd:%d recv data length:%d from peer %s:%d ", _sock_fd, recv_len, _remote_ip_string, ntohs(_remote_port));
    
    if (recv_len == read_len-index)
    {
        recv_cnt_bytes += recv_len;
        return read_len;
    }
    else if (recv_len > 0)
    {
        recv_cnt_bytes += recv_len;
        if (at_least <= 0)
        {
            return recv_len;
        }
        
        index += recv_len;
        
        if (recv_times < 5 && index < at_least)
        {
            recv_times++;
            goto label_recv;
        }
        else
        {
            return index;
        }
    }
    else if (recv_len == 0)
    {
        log_error("peer close the connection : sock_fd:%d peer(%s:%u) ", _sock_fd, _remote_ip_string, ntohs(_remote_port));
        CloseConnection();
        return -1;
    }
    else
    {
        if (errno_cached == EINTR)
        {
            if (recv_times < 5 && index < at_least)
            {
                recv_times++;
                goto label_recv;
            }
            else
            {
                return index;
            }
        }
        else if (errno_cached == EAGAIN)
        {
            return index;
        }
        else
        {
            log_error("recv error : sock_fd:%d peer(%s:%u) : %s ", _sock_fd, _remote_ip_string, ntohs(_remote_port), strerror(errno_cached));
            CloseConnection();
            return -1;
        }
    }
}

int CTcpConnection::OnCanRead(void)
{
    int read_len = 0;
    int at_least = 0;
    int recv_len = 0;
    int data_len = 0;
    int msg_len = 0;
    int ret_val = 0;
    
    read_len = get_buffer_free_space(_p_recv_buffer);
    data_len = get_buffer_data_len(_p_recv_buffer);
    
    if (data_len == 0)
    {
        _p_recv_buffer->read_index = 0;
        _p_recv_buffer->write_index = 0;
        at_least = 4;
    }
    else if (data_len < 4)
    {
        at_least = 4 - data_len;
    }
    else
    {
        at_least = 0;
    }
    
    recv_len = ReadData(&(_p_recv_buffer->ring_buffer[_p_recv_buffer->write_index]), read_len, at_least);
    if (recv_len <= 0)
    {
        return recv_len;
    }
    _p_recv_buffer->write_index += recv_len;
    
    data_len = get_buffer_data_len(_p_recv_buffer);
    
    log_debug("tcp_conn:%p recv_len:%d data_len:%d sock_fd:%d remote(%s:%u) ",
              this, recv_len, data_len, _sock_fd, _remote_ip_string, ntohs(_remote_port));
    
    while (data_len > 4)
    {
        msg_len = _p_pkt_proc->GetMsgLen(&(_p_recv_buffer->ring_buffer[_p_recv_buffer->read_index]), data_len);
        if (msg_len <= 4)
        {
            log_error("tcp_conn:%p msg too short msg_len:%d data_len:%d sock_fd:%d remote(%s:%u) ",
                      this, msg_len, data_len, _sock_fd, _remote_ip_string, ntohs(_remote_port));
            CloseConnection();
            return -1;
        }
        else if ((msg_len - data_len) > get_buffer_free_space(_p_recv_buffer))
        {
            log_error("tcp_conn:%p msg too long msg_len:%d data_len:%d sock_fd:%d remote(%s:%u) ",
                      this, msg_len, data_len, _sock_fd, _remote_ip_string, ntohs(_remote_port));
            CloseConnection();
            return -1;
        }
        
        log_debug("tcp_conn:%p recv_len:%d msg_len:%d data_len:%d sock_fd:%d remote(%s:%u) ",
                  this, recv_len, msg_len, data_len, _sock_fd, _remote_ip_string, ntohs(_remote_port));
        
        if (data_len >= msg_len)
        {
            recv_cnt_pkts++;
            
            ret_val = _p_pkt_proc->OnPacketComplete((char *)&(_p_recv_buffer->ring_buffer[_p_recv_buffer->read_index]), msg_len);
            if (ret_val < 0)
            {
                log_error("deal msg fail : msg_len:%d sock_fd:%d remote(%s:%d) : close the connection ",
                           msg_len, _sock_fd, _remote_ip_string, ntohs(_remote_port));
                CloseConnection();
                return -1;
            }
            
            _p_recv_buffer->read_index += msg_len;
            data_len -= msg_len;
        }
        else
        {
            break;
        }
    }
    
    if (data_len > 0 && _p_recv_buffer->read_index > 0)
    {
        memmove(_p_recv_buffer->ring_buffer, &(_p_recv_buffer->ring_buffer[_p_recv_buffer->read_index]), data_len);
    }
    
    if (data_len > 0)
    {
        _p_recv_buffer->write_index = data_len;
    }
    else
    {
        _p_recv_buffer->write_index = 0;
    }
    
    _p_recv_buffer->read_index = 0;
    
    return recv_len;
}

int CTcpConnection::SendMsgInternal(uint8_t * data_buffer, int data_len)
{
    int send_len = 0;
    int send_times = 0;
    int errno_cached = 0;
    
    if (data_buffer == NULL || data_len <= 0)
    {
        log_error("data_buffer:%p data_len:%d ", data_buffer, data_len);
        return -1;
    }
    
label_send:
    send_len = send(_sock_fd, data_buffer, data_len, 0);
    errno_cached = errno;
    
    log_debug("sock_fd:%d send msg length:%d to peer %s:%d ", _sock_fd, send_len, _remote_ip_string, ntohs(_remote_port));
    
    if (send_len == data_len)
    {
        return send_len;
    }
    else if (send_len > 0)
    {   // 内核缓冲区满了, 但当前数据只有部分拷贝到了内核
        return send_len;
    }
    else if (send_len == 0 && send_times < 5)
    {
        send_times++;
        log_warning("send_len:0 send_times:%d ", send_times);
        goto label_send;
    }
    else
    {
        if (errno_cached == EAGAIN)
        {
            return 0;
        }
        else if (errno_cached == EINTR && send_times < 5)
        {
            send_times++;
            log_warning("errno_cached == EINTR, send_times:%d ", send_times);
            goto label_send;
        }
        else
        {
            log_error("tcp_conn:%p sock_fd:%d send data to address %s:%u failed send_times:%d : %s ",
                       this, _sock_fd, _remote_ip_string, ntohs(_remote_port), send_times, strerror(errno_cached));
            CloseConnection();
            return -1;
        }
    }
}

int CTcpConnection::WriteData(void)
{
    int data_len = 0;
    int send_len = 0;
    
    if (_connection_status != CONN_STATUS_CONNECTED)
    {
        log_error("_connection_status != CONN_STATUS_CONNECTED ");
        return 0;
    }
    
    data_len = get_buffer_data_len(_p_send_buffer);
    if (data_len > 0)
    {
        if (_p_send_buffer->read_index < _p_send_buffer->write_index)
        {
            send_len = SendMsgInternal(&(_p_send_buffer->ring_buffer[_p_send_buffer->read_index]), data_len);
            if (send_len > 0)
            {
                move_read_index(_p_send_buffer, send_len);
                if (send_len == data_len)
                {   // 发送缓冲中的数据已经发完, 暂时关闭对可写事件的监听
                    _p_event_poll->StopMonitoringWrite(_sock_fd, this);
                }
            }
            else
            {
                log_error("send_len:%d data_len:%d", send_len, data_len);
            }
            return send_len;
        }
        else
        {
            int end_data_len = _p_send_buffer->buffer_length - _p_send_buffer->read_index;
            int start_data_len = _p_send_buffer->write_index;
            
            send_len = SendMsgInternal(_p_send_buffer->ring_buffer + _p_send_buffer->read_index, end_data_len);
            if (send_len > 0)
            {
                move_read_index(_p_send_buffer, send_len);
            }
            
            if (send_len < end_data_len)
            {
                return send_len;
            }
            
            send_len = SendMsgInternal(_p_send_buffer->ring_buffer, start_data_len);
            if (send_len > 0)
            {
                move_read_index(_p_send_buffer, send_len);
            }
            
            if (send_len < start_data_len)
            {
                if (send_len >= 0)
                {
                    return send_len + end_data_len;
                }
                else
                {
                    log_error("send_len:%d start_data_len:%d ", send_len, start_data_len);
                    return send_len;
                }
            }
            else
            {   // 发送缓冲中的数据已经发完, 暂时关闭对可写事件的监听
                _p_event_poll->StopMonitoringWrite(_sock_fd, this);
                return data_len;
            }
        }
    }
    else
    {
        _p_event_poll->StopMonitoringWrite(_sock_fd, this);
        return 0;
    }
}

int CTcpConnection::OnCanWrite(void)
{
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
            CloseConnection();
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
}

int CTcpConnection::SendMsg(uint8_t * msg, int msg_len)
{
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
            
            CloseConnection();
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
}
    
void CTcpConnection::CloseConnection(void)
{
    log_info("tcp_conn:%p sock_fd:%d address %s:%u close connection : "
             "recv_cnt_bytes:%lu recv_cnt_pkts:%lu send_cnt_bytes:%lu send_cnt_pkts:%lu ",
              this, _sock_fd, _remote_ip_string, ntohs(_remote_port),
              recv_cnt_bytes, recv_cnt_pkts, send_cnt_bytes, send_cnt_pkts);
    
    _p_pkt_proc->OnClose();
    
    if (_sock_fd >= 0)
    {
        _p_event_poll->DeleteFromEventLoop(_sock_fd); 
        close(_sock_fd);
    }
    
    _connection_status = CONN_STATUS_CLOSED;
    _sock_fd = -1;
    
    if (_will_reconnect == 0)
    {
        this->SetUsingStatus(0);
        this->ResetObject();
        TcpConnectionManager::Instance()->ReleaseOneObject(this);
    }
    else
    {
        Reset();
    }
}

int CTcpConnection::OnSockError(void)
{
    CloseConnection();
    return 0;
}

int CTcpConnection::DumpActiveConnInfo(char * buffer, int buffer_len)
{
    return snprintf(buffer, buffer_len, "%-4d  %15s:%-5u  %15s:%-5u  %d  %d  %d  %-10lu  %-10lu  %-14lu  %-14lu \r\n",
                    _sock_fd, _local_ip_string, ntohs(_local_port), _remote_ip_string, ntohs(_remote_port),
                    _connection_status, _will_reconnect, _ep_type, recv_cnt_pkts, send_cnt_pkts,
                    recv_cnt_bytes, send_cnt_bytes);
}

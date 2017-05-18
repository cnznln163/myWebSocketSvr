#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include "log.h"
#include "objectmanger.h"
#define MAX_MSG_LEN         0x20000             // 128KB
#define MAX_SEND_BUFF_LEN   (MAX_MSG_LEN*2)     // 256KB
#define MAX_RECV_BUFF_LEN   (MAX_MSG_LEN*2)     // 256KB

#define EP_TYPE_SERVER 0
#define EP_TYPE_CLIENT 1

#define CONN_STATUS_CLOSED      0
#define CONN_STATUS_CONNECTING  1
#define CONN_STATUS_CONNECTED   2

#define MAX_IP_STRING_LEN   16

struct ring_buffer;
class CPacketProcessor;
class CTcpevent;

class CTcpConnection
{
public:
    CTcpConnection(void);
    
    ~CTcpConnection(void);
    
    void InitRingBuffer(uint32_t recv_buffer_size, uint32_t send_buffer_size);
    
	void resetObject(void);
    
	void Reset(void);
    
    int Init(void);
    
    void setRemoteIp(char* remote_ip){
        memset(_remote_ip_string, 0, sizeof(_remote_ip_string));
        memcpy(_remote_ip_string,remote_ip,16);
    }
    
    const char* getRemoteIp(void){
        return _remote_ip_string;
    }
    
	void setLocalIp(char* local_ip){
        memset(_local_ip_string, 0, sizeof(_local_ip_string));
        memcpy(_local_ip_string,local_ip,16);
    }
    
    const char* getRemoteIp(void){
        return _local_ip_string;
    }
	
    void setRemotePort(uint32_t remote_port){
        _remote_port = remote_port;
    }
    
    uint32_t getRemotePort(void){
        return _remote_port;
    }
    
	void setLocalPort(uint32_t local_port){
        _local_port = local_port;
    }
    
    uint32_t getLocalPort(void){
        return _local_port;
    }
	
    void setSockFd(int sock_fd){
        _sock_fd = sock_fd;
    }
    
    int getSockFd(void){
        return _sock_fd;
    }
    
    void setConnectDone(void){
        _connection_status = CONN_STATUS_CONNECTED;
    }
    
    void clearConnectDone(void){
        _connection_status = CONN_STATUS_CLOSED;
    }
    
    int getReconnectFlags(void){
        return _will_reconnect;
    }
    
    void setReconnectFlags(int will_reconnect){
        if (will_reconnect >= 1){
            will_reconnect = 1;
        }else{
            will_reconnect = 0;
        }
        
        _will_reconnect = will_reconnect;
    }
    
    void setEpType(int ep_type){
        _ep_type = ep_type;
    }
    
    int getEpType(void){
        return _ep_type;
    }
   
    CPacketProcessor * GetPktProc(void){
        return _p_pkt_proc;
    }
    
    void setTcpEvent(CTcpevent * p_event_poll){
        _p_event_poll = p_event_poll;
    }
    
    CTcpevent * getEventPoll(void){
        return _p_event_poll;
    }
    
    int getUsingStatus(void){
        return _in_using;
    }
    void setUsingStatus(int in_using){
        _in_using = in_using;
    }
    
    int createConnection(void);
    
    int onConnected(void);
    
    int readData(uint8_t * buffer, int read_len, int waiting_entire_msg);
    
    int OnCanRead(void);
    
    int SendMsgInternal(uint8_t * data_buffer, int data_len);
    
    int WriteData(void);
    
    int OnCanWrite(void);
    
    int SendMsg(uint8_t * msg, int msg_len);
    
    void CloseConnection(void);
    
    int OnSockError(void);
    
    int DumpActiveConnInfo(char * buffer, int buffer_len);
    void SetPktProc(CPacketProcessor * pkt_proc)
    {
        _p_pkt_proc = pkt_proc;
    }
    
public:
    char _remote_ip_string[MAX_IP_STRING_LEN];//内部服务IP
    uint32_t _remote_port;
	char _local_ip_string[MAX_IP_STRING_LEN];//对端IP
    uint32_t _local_port;  
    int _sock_fd;
    int _connection_status;
    int _will_reconnect;
    int _ep_type;           // client / server
    int _in_using;
    
    
    uint64_t recv_cnt_bytes;
    uint64_t recv_cnt_pkts;
    uint64_t send_cnt_bytes;
    uint64_t send_cnt_pkts;
    
	struct ring_buffer * _p_recv_buffer;
	struct ring_buffer * _p_send_buffer;
    
    CPacketProcessor * _p_pkt_proc;
    CEventPoll * _p_event_poll;
};


#define TcpConnectionManager CObjectManager<CTcpConnection>

#endif



#endif
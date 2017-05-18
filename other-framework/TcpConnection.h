
// TcpConnection.h

#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include "Public.h"

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
class CEventPoll;

class CTcpConnection
{
public:
    CTcpConnection(void);
    
    ~CTcpConnection(void);
    
    void InitRingBuffer(uint32_t recv_buffer_size, uint32_t send_buffer_size);
    
	void ResetObject(void);
    
	void Reset(void);
    
    int Init(void);
    
    void SetRemoteIp(uint32_t remote_ip)
    {
        _remote_ip = remote_ip;
        memset(_remote_ip_string, 0, sizeof(_remote_ip_string));
        inet_ntop(AF_INET, &_remote_ip, _remote_ip_string, sizeof(_remote_ip_string));
    }
    
    uint32_t GetRemoteIp(void)
    {
        return _remote_ip;
    }
    
    const char * GetRemoteIpString(void)
    {
        return _remote_ip_string;
    }
    
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
    
    const char * GetLocalIpString(void)
    {
        return _local_ip_string;
    }
    
    void SetRemotePort(uint16_t remote_port)
    {
        _remote_port = remote_port;
    }
    
    uint16_t GetRemotePort(void)
    {
        return _remote_port;
    }
    
    void SetLocalPort(uint16_t local_port)
    {
        _local_port = local_port;
    }
    
    uint16_t GetLocalPort(void)
    {
        return _local_port;
    }
    
    void SetSockFd(int sock_fd)
    {
        _sock_fd = sock_fd;
    }
    
    int GetSockFd(void)
    {
        return _sock_fd;
    }
    
    void SetConnectDone(void)
    {
        _connection_status = CONN_STATUS_CONNECTED;
    }
    
    void ClearConnectDone(void)
    {
        _connection_status = CONN_STATUS_CLOSED;
    }
    
    int GetReconnectFlags(void)
    {
        return _will_reconnect;
    }
    
    void SetReconnectFlags(int will_reconnect)
    {
        if (will_reconnect >= 1)
        {
            will_reconnect = 1;
        }
        else
        {
            will_reconnect = 0;
        }
        
        _will_reconnect = will_reconnect;
    }
    
    void SetEpType(int ep_type)
    {
        _ep_type = ep_type;
    }
    
    int GetEpType(void)
    {
        return _ep_type;
    }
    
    void SetPktProc(CPacketProcessor * pkt_proc)
    {
        _p_pkt_proc = pkt_proc;
    }
    
    CPacketProcessor * GetPktProc(void)
    {
        return _p_pkt_proc;
    }
    
    void SetEventPoll(CEventPoll * p_event_poll)
    {
        _p_event_poll = p_event_poll;
    }
    
    CEventPoll * GetEventPoll(void)
    {
        return _p_event_poll;
    }
    
    int GetUsingStatus(void)
    {
        return _in_using;
    }
    void SetUsingStatus(int in_using)
    {
        _in_using = in_using;
    }
    
    int CreateConnection(void);
    
    int OnConnected(void);
    
    int ReadData(uint8_t * buffer, int read_len, int waiting_entire_msg);
    
    int OnCanRead(void);
    
    int SendMsgInternal(uint8_t * data_buffer, int data_len);
    
    int WriteData(void);
    
    int OnCanWrite(void);
    
    int SendMsg(uint8_t * msg, int msg_len);
    
    void CloseConnection(void);
    
    int OnSockError(void);
    
    int DumpActiveConnInfo(char * buffer, int buffer_len);
    
public:
    uint32_t _remote_ip;    // network order
    uint16_t _remote_port;  // network order
    uint32_t _local_ip;     // network order
    uint16_t _local_port;   // network order
    int _sock_fd;
    int _connection_status;
    int _will_reconnect;
    int _ep_type;           // client / server
    int _in_using;
    
    char _remote_ip_string[MAX_IP_STRING_LEN];
    char _local_ip_string[MAX_IP_STRING_LEN];
    
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


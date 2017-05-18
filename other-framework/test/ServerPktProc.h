
// AccessPktProc.h

#ifndef ACCESS_PACKET_PROCESSOR_H
#define ACCESS_PACKET_PROCESSOR_H

#include "PacketProcessor.h"

class CServerPktProc : public CPacketProcessor 
{
public:
    CServerPktProc(void)
    {
        recv_cnt_bytes = 0;
        recv_cnt_pkts = 0;
    }
    
    ~CServerPktProc(void)
    {
        
    }
    
    int ResetObject(void)
    {
        recv_cnt_bytes = 0;
        recv_cnt_pkts = 0;
        return 0;
    }
    
    int Init(void)
    {
        return 0;
    }
    
    void Reset(void)
    {
        recv_cnt_bytes = 0;
        recv_cnt_pkts = 0;
    }
    
    int OnConnected(void)
    {
        log_debug("one client connected, sock_fd:%d tcp_conn:%p pkt_proc:%p client address %s:%u ",
                  _p_tcp_conn->GetSockFd(), _p_tcp_conn, this, _p_tcp_conn->GetRemoteIpString(),
                  ntohs(_p_tcp_conn->GetRemotePort()));
        return 0;
    }
    
    int OnPacketComplete(uint8_t * data, int len)
    {
        recv_cnt_bytes += len;
        recv_cnt_pkts++;
        
        if (_p_tcp_conn != NULL)
        {
            _p_tcp_conn->SendMsg(data, len);
        }
        
        return len;
    }
    
    int OnClose(void)
    {
        log_info("one client closed connection, sock_fd:%d tcp_conn:%p pkt_proc:%p client address %s:%u "
                 "recv_cnt_bytes:%lu recv_cnt_pkts:%lu ",
                 _p_tcp_conn->GetSockFd(), _p_tcp_conn, this, _p_tcp_conn->GetRemoteIpString(),
                 ntohs(_p_tcp_conn->GetRemotePort()), recv_cnt_bytes, recv_cnt_pkts);
                  
        return 0;
    }
    
public:
    uint64_t recv_cnt_bytes;
    uint64_t recv_cnt_pkts;
};


#endif



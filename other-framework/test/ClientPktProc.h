
// ClientPktProc.h

#ifndef CLIENT_PACKET_PROCESSOR_H
#define CLIENT_PACKET_PROCESSOR_H

#include "PacketProcessor.h"
#include "timer_set.h"


static int send_msg_timer_callback(void * pv_user_timer);


class CClientPktProc : public CPacketProcessor 
{
public:
    CClientPktProc(void)
    {
        recv_cnt_bytes = 0;
        recv_cnt_pkts = 0;
        _p_send_msg_timer = NULL;
        _p_timer_set = NULL;
    }
    
    ~CClientPktProc(void)
    {
        
    }
    
    int ResetObject(void)
    {
        recv_cnt_bytes = 0;
        recv_cnt_pkts = 0;
        
        if (_p_timer_set != NULL && _p_send_msg_timer != NULL)
        {
            destroy_one_timer(_p_timer_set, _p_send_msg_timer->i_timer_id);
            _p_send_msg_timer = NULL;
        }
        
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
        
        if (_p_timer_set == NULL)
        {
            return -1;
        }
        
        user_timer_t user_timer;

        init_user_timer(user_timer, 0xFFFFFFFF, 20, send_msg_timer_callback, this, NULL, NULL, NULL, NULL);
        
        _p_send_msg_timer = create_one_timer(_p_timer_set, &user_timer);
        
        return 0;
    }
    
    int OnPacketComplete(uint8_t * data, int len)
    {
        recv_cnt_bytes += len;
        recv_cnt_pkts++;
        
        return len;
    }
    
    int OnClose(void)
    {
        log_info("one client closed connection, sock_fd:%d tcp_conn:%p pkt_proc:%p client address %s:%u "
                 "recv_cnt_bytes:%lu recv_cnt_pkts:%lu ",
                 _p_tcp_conn->GetSockFd(), _p_tcp_conn, this, _p_tcp_conn->GetRemoteIpString(),
                 ntohs(_p_tcp_conn->GetRemotePort()), recv_cnt_bytes, recv_cnt_pkts);
        
        if (_p_timer_set != NULL && _p_send_msg_timer != NULL)
        {
            destroy_one_timer(_p_timer_set, _p_send_msg_timer->i_timer_id);
            _p_send_msg_timer = NULL;
        }
        
        return 0;
    }
    
    void SetTimerSet(struct timer_set * p_timer_set)
    {
        _p_timer_set = p_timer_set;
    }
    
public:
    uint64_t recv_cnt_bytes;
    uint64_t recv_cnt_pkts;
    struct timer_set * _p_timer_set;
    user_timer_t * _p_send_msg_timer;
};


static int send_msg_timer_callback(void * pv_user_timer)
{
    user_timer_t * p_user_timer = (user_timer_t *)pv_user_timer;
    CClientPktProc * p_pkt_proc = (CClientPktProc *)(p_user_timer->pv_param1);
    uint8_t msg[1000];
    int i = 0;
    
    *(int *)msg = htonl(1000);
    for (i=4; i<1000; i+=2)
    {
        *(short *)(msg + i) = i;
    }
    
    log_debug("send msg p_pkt_proc:%p p_tcp_conn:%p ", p_pkt_proc, p_pkt_proc->GetTcpConnObj());
    
    p_pkt_proc->GetTcpConnObj()->SendMsg(msg, 1000);
    
    return 0;
}

#endif



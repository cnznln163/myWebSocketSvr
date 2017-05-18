#ifndef PACKET_PROCESSOR_H
#define PACKET_PROCESSOR_H

#include <vector>
#include <map>

#include "Public.h"
#include "timer_set.h"

class CTcpConnection;

class CPacketProcessor
{
public:
    CPacketProcessor(void)
    {
        _p_tcp_conn = NULL;
        _p_timer_set = NULL;
    }
    
    virtual ~CPacketProcessor(void){};
    
    virtual int ResetObject(void)   // 将对象还原到刚构造完成的状态 --- 该对象不一定能正常工作, 需要 Init() 做进一步初始化
    {
        return 0;
    }
    
    virtual int Init(void)      // 将对象初始化到可正常工作状态
    {
        log_debug("init PacketProcessor");
        
        if (_p_tcp_conn == NULL || _p_timer_set == NULL)
        {
            log_error("tcp_conn or timer_set is illegal");
            return -1;
        }
        else
        {
            return 0;
        }
    }
    
    virtual void Reset(void){}  // 将对象还原到 Init() 结束时的状态
    
	virtual int OnConnected(void)
    {
        return 0;
    }
    
    virtual int GetMsgLen(uint8_t * p_data, int data_len)
    {
        if (p_data == NULL || data_len <= 0)
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }
    
	virtual int OnPacketComplete(char * data, int len)
    {
        if (data == NULL || len < 0)
        {
            return -1;
        }
        else
        {
            return len;
        }
    }
    
    virtual int OnClose(void)
    {
        return 0;
    }
    
    virtual int DumpPktProc(char * buffer, int buffer_len)
    {
        if (buffer == NULL || buffer_len <= 0)
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }
    
    CTcpConnection * GetTcpConnObj(void)
    {
        return _p_tcp_conn;
    }
    
    void SetTcpConnObj(CTcpConnection * p_tcp_conn)
    {
        _p_tcp_conn = p_tcp_conn;
    }
    
    timer_set_t * GetTimerSet(void)
    {
        return _p_timer_set;
    }
    
    void SetTimerSet(timer_set_t * p_timer_set)
    {
        _p_timer_set = p_timer_set;
    }
    
public:

    CTcpConnection * _p_tcp_conn;
    timer_set_t * _p_timer_set;
};


typedef std::vector<CPacketProcessor *> PktProcPV;
typedef std::map<long long, PktProcPV> GroupMap;


#endif


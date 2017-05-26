#ifndef TCP_SERVER_H
#define TCP_SERVER_H
#include <stdlib.h>
#include <stdio.h>
#include "log.h"
#include "ctcpevent.h"

class CTcpevent;

int is_tcp_server(void * pv_obj);

class CTcpserver
{
public:
	CTcpserver();
	~CTcpserver();
	void setIp(char *ip){
		memcpy(_local_ip_string,ip,16);
	}
	void setPort(uint32_t port){
		_local_port = port;
	}
	void setEvent(CTcpevent *p_event){
		_p_event_poll = p_event;
	}
	void setSvrId(int svr_id){
		_svrId = svr_id;
	}
	void setSvrType(int svr_type){
		_svrType = svr_type;
	}
	int init();
	int onNewConnArrived(void);
	int onSockError(void);
	/* data */
private:
	char _local_ip_string[16];
	uint32_t _local_port;
	CTcpevent *_p_event_poll;
	int _svrId;
	int _svrType;
	int _sock;
};

#endif

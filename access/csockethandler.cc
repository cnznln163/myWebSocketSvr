#include "csockethandler.h"
#include "ctcpconnection.h"
#include "log.h"

//返回值非1 0则关闭连接
int CSocketHandler::inputNotify(CTcpConnection *c,CPacketSocket *packet){
	if( packet == NULL ){
		return 0;
	}
	int ret = 0;
	switch(packet->getCommond()){
		case CMD_LOGIN:
			ret = login(c,packet);
			break;
		default:
			return 0;
	}
	return ret;
}

int CSocketHandler::login(CTcpConnection *c,CPacketSocket *packet){
	int sid = packet->readInt();
	int svrtype = packet->readInt();
	int gameid = packet->readInt();
	if( svrtype == 1 ){//client
		c->setEpType(EP_TYPE_CLIENT);
	}else{
		c->setEpType(EP_TYPE_SERVER);
	}
	log_write(LOG_DEBUG,"Login//Sid:%d,svrtype:%d,gameid:%d \n",sid,svrtype,gameid);
	packet->beginWrite(CMD_RESPONSE_LOGIN);
	packet->writeInt(1);
	packet->endWrite();
	return 1;
}

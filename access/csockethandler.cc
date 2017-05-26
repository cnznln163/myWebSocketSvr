#include "csockethandler.h"
#include "log.h"

int CSocketHandler::inputNotify(CPacketSocket *packet){
	if( packet == NULL ){
		return 0;
	}
	int ret = 0;
	switch(packet->getCommond()){
		case CMD_LOGIN:
			ret = login(packet);
			break;
		default:
			return 0;
	}
	return ret;
}

int CSocketHandler::login(CPacketSocket *packet){
	int sid = packet->readInt();
	int svrtype = packet->readInt();
	int gameid = packet->readInt();
	log_write(LOG_DEBUG,"Login//Sid:%d,svrtype:%d,gameid:%d \n",sid,svrtype,gameid);
	return 1;
}

#ifndef CSCOKETHANDLER_H
#define CSCOKETHANDLER_H
#include "cpacketsocket.h"
#include "commond.h"
class CTcpConnection;

class CSocketHandler
{
public:
	CSocketHandler(){};
	~CSocketHandler(){};

	int inputNotify(CTcpConnection *c , CPacketSocket *packet);
	/* data */
private:
	int login(CTcpConnection *c , CPacketSocket *packet);
};

#endif

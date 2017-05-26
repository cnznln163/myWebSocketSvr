#ifndef CSCOKETHANDLER_H
#define CSCOKETHANDLER_H
#include "cpacketsocket.h"
#include "commond.h"
class CSocketHandler
{
public:
	CSocketHandler(){};
	~CSocketHandler(){};

	int inputNotify(CPacketSocket *packet);
	/* data */
private:
	int login(CPacketSocket *packet);
};

#endif

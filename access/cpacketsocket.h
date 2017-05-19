#ifndef C_PACKETSOCKET_H
#define C_PACKETSOCKET_H
#include <strlib.h>
#define MAX_PACKET_BUF 0x20000 //128k

class CPacketSocket
{
public:
	CPacketSocket(void){};
	~CPacketSocket(void){};
	void setBuf(char *buf , int buff_size){
		if( buf == NULL || buff_size <= 0 || buff_size > MAX_PACKET_BUF ){
			return ;	
		}
		memset(m_strbuf,0,MAX_PACKET_BUF);
		memcpy(m_strbuf,buf,buff_size);
		m_packetSize = buff_size;
		m_pos = 0;
	};
	void reset(){
		memset(m_strbuf,0,MAX_PACKET_BUF);
		m_packetSize = 0;
		m_packetHeadSize = 0 ;
		m_pos = 0;
		m_buffType = 0;
	}
	int readHeader(){
		
	}
	/* data */
private:
	char m_strbuf[MAX_PACKET_BUF];
	int m_packetSize;
	int m_packetHeadSize;
	int m_pos;
	int m_buffType;
};

#endif
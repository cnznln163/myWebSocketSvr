#ifndef C_PACKETSOCKET_H
#define C_PACKETSOCKET_H
#include <strlib.h>
#define MAX_PACKET_BUF 0x20000 //128k
//[int body len|unsigned char ver|int cmd]
#define PACKET_HEADER_SIZE 9

typedef unsigned char BYTE;

class CPacketSocket
{
public:
	CPacketSocket(void){};
	~CPacketSocket(void){};
	void appendBuf(char *buf , int buff_size){
		if( buf == NULL || buff_size <= 0 || buff_size+m_pos > MAX_PACKET_BUF ){
			return ;	
		}
		memcpy(m_strbuf,buf,buff_size);
		m_packetSize += buff_size;
		m_pos = buff_size;
	};
	int getPos(){
		return m_pos;
	}
	int getPacketSize(){
		return m_packetSize;
	}
	int getFreeBuffSize(){
		return MAX_PACKET_BUF-m_packetSize;
	}
	void reset(){
		memset(m_strbuf,0,MAX_PACKET_BUF);
		m_packetSize = 0;
		m_packetHeadSize = 0 ;
		m_pos = 0;
		m_buffType = 0;
	}
	// 处理PACKET数据
	int parsePacket(char *data, int length){
		int ret = -1; //出错返回
		int ndx = 0;
		while(ndx < length && m_nStatus != REQ_ERROR){//可能会同时来两个包 
			switch(m_nStatus){
				case REQ_REQUEST:
				case REQ_HEADER:
					ret = read_header(data, length, ndx);
					if(ret < 0){	
						m_nStatus = REQ_ERROR;
						break;
					}else if(0 == ret){
						break;  //继续接受包头
					}
					ret = parse_header();
					if(ret != 0){
						m_nStatus = REQ_ERROR;
						break;
					}else
						m_nStatus = REQ_BODY;			
				
				case REQ_BODY:
					if(parse_body(data, length, ndx))
						m_nStatus = REQ_DONE;
					break;
				default:
					return -1;
			}

			if(m_nStatus == REQ_ERROR)
			{
				this->reset();
				return ret;
			}
			if(m_nStatus == REQ_DONE)
			{
				if(OnPacketComplete(&m_Packet) == -1)
				{
					this->reset();
					return -1;
				}
				this->reset();
			}
		}
		return 0; // return 0 to continue
	}
	bool WriteIntExtend(int nValue) {int value = htonl(nValue); return _WriteExtend((char*)&value, sizeof(int));}
	bool WriteShortExtend(short nValue) {short value = htons(nValue); return _WriteExtend((char*)&value, sizeof(short));}
	bool WriteByteExtend(BYTE nValue) {return _WriteExtend((char*)&nValue, sizeof(BYTE));}
    
	int ReadIntExtend(int pos)		{int nValue = -1; _ReadExtend((char*)&nValue, pos, sizeof(int)); return ntohl(nValue);} 
	short ReadShortExtend(int pos)	{short nValue = -1; _ReadExtend((char*)&nValue, pos, sizeof(short)); return ntohs(nValue);}
	BYTE ReadByteExtend(int pos)	{BYTE nValue = -1; _ReadExtend((char*)&nValue, pos, sizeof(BYTE)); return nValue;}
	/* data */
private://private function
	// 读取Packet头数据   //1:成功,  0:继续接受包头,   -1:错误包头
	int read_header(char *data, int length, int &ndx){
		while(m_nPacketPos < PACKET_BY_HEADER_SIZE && ndx < length){
			m_strbuf[m_nPacketPos++] = data[ndx++];
		}

		if(m_nPacketPos < PACKET_BY_HEADER_SIZE){//继续读
			return 0;    
		}
		m_packetHeadSize = PACKET_BY_HEADER_SIZE;
		return 1;
	}
	// 解析Packet头信息
	int parse_header(void){ //0:成功 -1:命令范围错误 -2:版本错误 -3:长度错误
		int nCmd;
		_readHeader((char *)&nCmd,sizeof(int),5);
		m_cmd = ntohl(nCmd);
		if(m_cmd <= 0 || m_cmd >= 0xFFFF){
			m_cmd = 0;
			return -1;
		}
		char ver;
		_readHeader((char *)&ver,sizeof(char),4);
		if( m_ver != ver ){
			return -2;
		}
		int bodyLen,mBodyLen;
		_readHeader((char*)&bodyLen, sizeof(int), 0);// 包正文长度
		mBodyLen = ntohl(bodyLen);
		if(m_nBodyLen <= 0 && m_nBodyLen > (MAX_PACKET_BUF - m_packetHeadSize))
			return -3;
		m_packetSize = mBodyLen;
		return 0;
	}

	// readHeader
	void _readHeader(char *pOut, int nLen, int nPos){
		memcpy(pOut, m_strbuf+nPos, nLen) ;
	}
	// 解析BODY数据
	bool parse_body(char *data, int length, int &ndx){
		int nNeed = (m_packetSize+m_packetHeadSize) - m_nPacketPos;
		int nBuff = length - ndx;

		if(nNeed <= 0)
			return true;
		if(nBuff <= 0)
			return false;

		int nCopy = nBuff < nNeed ? nBuff : nNeed;
		if(writeBody(data + ndx, nCopy))
			return false;

		m_nPacketPos += nCopy;
		ndx += nCopy;

		if(m_nPacketPos < (m_packetSize+m_packetHeadSize) )
			return false;
       
		return true;
	}

	bool writeBody(const char *pIn, int nLen){
		int m_nPacketSize = m_nPacketPos;
		if((m_nPacketSize < 0) || ((nLen + m_nPacketSize) > MAX_PACKET_BUF))
			return false ;
		memcpy(m_strbuf+m_nPacketSize, pIn, nLen);
		return true;
	}
	bool _WriteExtend(const char *pIn, int nLen){
		if((m_nPacketSize < 0) || ((nLen + m_nPacketSize) > MAX_PACKET_BUF))
			return false ;

		memcpy(m_strBuf+m_nHeadSize, pIn, nLen);
		m_nHeadSize+=nLen;
		m_nBufPos = m_nHeadSize;
		m_nPacketSize = m_nHeadSize;

        _WriteExtenSize();
		return true;
	}

private:
	char m_strbuf[MAX_PACKET_BUF];
	int m_packetSize;
	int m_packetHeadSize;
	int m_nPacketPos;
	int m_pos;
	int m_buffType;
	int m_nStatus;
	int m_cmd;
	int m_ver;
	enum REQSTATUS{	REQ_REQUEST=0, REQ_HEADER, REQ_BODY, REQ_DONE, REQ_PROCESS, REQ_ERROR };
};

#endif
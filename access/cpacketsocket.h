#ifndef C_PACKETSOCKET_H
#define C_PACKETSOCKET_H
#include <stdlib.h>
#include <string>
#include <arpa/inet.h>
#define MAX_PACKET_BUF 0x20000 //128k
//[int body len|unsigned char ver|int cmd]
#define PACKET_HEADER_SIZE 9
using namespace std;
typedef unsigned char BYTE;

class CPacketSocket
{
public:
	CPacketSocket(void){reset();};
	~CPacketSocket(void){};
	
	char* getPacketBuffer(){
		return &(m_strbuf[write_index]);
	};
	int getPos(){
		return m_pos;
	};
	int getPacketSize(){
		return m_packetSize;
	};
	int getRecvDataLen(){
		return write_index;
	}
	int getFreeDataLen(){
		return m_buffLen-read_index;
	}
	int getFreeBuffSize(){
		return m_buffLen-write_index;
	}
	int getHeadPacketSize(){
		int size;
		memcpy((char *)&size, m_strbuf+read_index, sizeof(int));
		size = ntohl(size);
		if( size < PACKET_HEADER_SIZE-4 ){
			return -1;
		}
		return size;
	}
	int getCommond(){
		return m_cmd;
	}
	void reset(){
		//read reset
		memset(m_strbuf,0,MAX_PACKET_BUF);
		m_buffLen = MAX_PACKET_BUF;
		m_packetSize = 0;
		m_packetHeadSize = 0 ;
		m_pos = 0;
		m_buffType = 0;
		read_index=0;
		write_index=0;
		m_ver = 'J';
		//write reset
		memset(m_wstrBuf,0,MAX_PACKET_BUF);
		m_wHeadSize = 0;
		m_wPacketSize = 0;
	}
	// 处理PACKET数据
	int parsePacket(int length){
		int ret = -1; //出错返回
		ret = beginPacket(length);
		if( ret != 0 ){
			return ret;
		}
		return 0; // return 0 to continue
	}
	//处理1个以上完整包数据时当已取出了一个完整包时，需要把前一个完整包走buff中移除
    void movePacket(int moveLen){
    	memmove(m_strbuf,m_strbuf+read_index,moveLen);
    }
	int readInt(void)		{int nValue = -1; _read((char*)&nValue, sizeof(int)); return ntohl(nValue);} //这里必需初始化
	bool readInt(int& nValue){nValue=-1; bool ret=_read((char*)&nValue, sizeof(int)); nValue=ntohl(nValue); return ret;}
	unsigned long readULong(void) {unsigned long nValue = -1; _read((char*)&nValue, sizeof(unsigned long)); return ntohl(nValue);}
	short readShort(void)	{short nValue = -1; _read((char*)&nValue, sizeof(short)); return ntohs(nValue);}
	BYTE readByte(void)		{BYTE nValue = -1; _read((char*)&nValue, sizeof(BYTE)); return nValue;}
	
	int readString(char *pOutString,int maxLen){
		int nLen = readInt();
		if(nLen == -1)  //这里必需判断
			return -1;
		if(nLen > maxLen){
			_readundo(sizeof(int));
			return -1;
		}
		_read(pOutString, nLen);
		return nLen;
	}
	
	void beginWrite(int cmd){
		memset(m_wstrBuf,0,MAX_PACKET_BUF);
		m_wHeadSize = PACKET_HEADER_SIZE;
		int nCmd = htonl(cmd);
		_writeHeader((char *)&m_ver, sizeof(char), 4);	// 主版本号
		_writeHeader((char*)&nCmd, sizeof(int), 5);	// 命令码
		m_wPacketSize = m_wHeadSize;
	}
	void endWrite(){
		int nBody = m_wPacketSize - 4;	//数据包长度包括命令头和body,4个字节是数据包长度
		int len = htonl(nBody);
		_writeHeader((char*)&len, sizeof(int), 0);	// 包正文长度
	}
	char *getWriteBuff(){
		return m_wstrBuf;
	}
	int getWriteBuffLen(){
		return m_wPacketSize;
	}
	bool writeInt(int nValue)		{int value = htonl(nValue); return _write((char*)&value, sizeof(int));}
	bool writeULong(unsigned long nValue) {unsigned long value = htonl(nValue);return _write((char*)&value, sizeof(unsigned long));}
	bool writeByte(BYTE nValue)		{return _write((char*)&nValue, sizeof(BYTE));}
	bool writeShort(short nValue)	{short value = htons(nValue); return _write((char*)&value, sizeof(short));}
	//在正文首插入数据
	//bool InsertInt(int nValue)		{int value = htonl(nValue); return base::_Insert((char*)&value, sizeof(int));}
	//bool InsertByte(BYTE nValue)	{return base::_Insert((char*)&nValue, sizeof(BYTE));}
	bool WriteString(const char *pString){
		int nLen = (int)strlen(pString) ;
		writeInt(nLen + 1) ;
		if ( nLen > 0 ){
			return _write(pString, nLen) && _writezero();
		}else{
			return _writezero();
		}
	}

	bool writeString(const string &strDate){
		int nLen = (int)strDate.size();
		writeInt(nLen + 1);
		if ( nLen > 0 ){
			return _write(strDate.c_str(), nLen) && _writezero();
		}else{
			return _writezero();
		}
	}

	bool writeBinary(const char *pBuf, int nLen){
		writeInt(nLen) ;
		return _write(pBuf, nLen) ;
	}
	
	/* data */
private:// private function
	
	// 解析Packet头信息
	int beginPacket(int length){ //0:成功 -1:命令范围错误 -2:版本错误 -3:长度错误
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
		m_packetSize = length+4;//包含包体长度4字节int
		m_pos = PACKET_HEADER_SIZE;
		return 0;
	}

	// readHeader
	void _readHeader(char *pOut, int nLen, int nPos){
		memcpy(pOut, m_strbuf+nPos, nLen) ;
	}
	// writeHeader
	void _writeHeader(char *pIn, size_t nLen, int nPos){
		memcpy(m_wstrBuf+nPos, pIn, nLen);
	}
	
	// 取出一个变量
	bool _read(char *pOut, int nLen){
		if((nLen + m_pos) > m_packetSize || (nLen + m_pos) > MAX_PACKET_BUF)
			return false ;

		memcpy(pOut, m_strbuf + m_pos, nLen);
		m_pos += nLen;
		return true;
	}
	// 写入一个变量
	bool _write(const char *pIn, int nLen){
		if((m_wPacketSize < 0) || ((nLen + m_wPacketSize) > MAX_PACKET_BUF))
			return false ;
		memcpy(m_wstrBuf+m_wPacketSize, pIn, nLen);
		m_wPacketSize += nLen;
		return true;
	}
	bool _writezero(void){
		if((m_wPacketSize + 1) > MAX_PACKET_BUF)
			return false ;
		memset(m_wstrBuf+m_wPacketSize, '\0', sizeof(char)) ;
		m_wPacketSize ++;
		return true;
	}
	//读撤消
	void _readundo(int nLen){
		m_pos -= nLen;
	}
public:
	//read data define
	int read_index;
	int write_index;

private:
	//读背
	char m_strbuf[MAX_PACKET_BUF];
	int m_buffLen;
	int m_packetSize;
	int m_packetHeadSize;
	int m_nPacketPos;
	int m_pos;
	int m_buffType;
	int m_cmd;
	int m_ver;
	//写包
	char m_wstrBuf[MAX_PACKET_BUF];
	int  m_wHeadSize;
	int  m_wPacketSize;


	enum REQSTATUS{	REQ_REQUEST=0, REQ_HEADER, REQ_BODY, REQ_DONE, REQ_PROCESS, REQ_ERROR };
};

#endif

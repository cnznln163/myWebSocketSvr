#pragma once
////////////////////////////////////////////////////////////////////////////////
// PacketBase
////////////////////////////////////////////////////////////////////////////////
#include "ichatlib.h"
#include "ICHAT_PacketParser.h"
#include "ace/Log_Msg.h"



#ifndef __QE__
#define __QE__
#endif 

#define GAMEID 0x01


/*
extern short GetGameId();
extern short GetRegionId();
extern BYTE GetDeviceId();
*/
template <int _buffer_size>
class PacketBase
{
public:
	PacketBase(void){}
	virtual ~PacketBase(void){}

	char *packet_buf(void)	{return m_strBuf;}
	int packet_size(void)	{return m_nPacketSize;}
	void set_packet_type(int type) { m_nType = type; }
	int get_packet_type() { return m_nType; }

	enum
	{
       PACKET_BY_TYPE = 0,
	   PACKET_QE_TYPE = 1
	};
	enum
	{
		PACKET_BY_HEADER_SIZE = 9,
		PACKET_QE_HEADER_SIZE = 15,
		PACKET_BUFFER_SIZE = _buffer_size
	};
private:
	char m_strBuf[PACKET_BUFFER_SIZE];	// 报文包缓存
	int m_nPacketSize ;	// 实际报文总长度
	int m_nHeadSize;
	int m_nBufPos;
    int m_nType;
protected:
	////////////////////////////////////////////////////////////////////////////////
	bool _copy(const void *pInBuf, int nLen)
	{
		if(nLen > PACKET_BUFFER_SIZE)
			return false;
        int  type = GetPacketTypeFromBuf((char*)pInBuf);
		if(PACKET_BY_TYPE == type)
			_resetBY();
		else if(PACKET_QE_TYPE == type)
			_resetQE();
		else
			return false;
        
		memcpy(m_strBuf, pInBuf, nLen);

		m_nType = type;
		m_nHeadSize = _GetPacketHeadSize();
		m_nBufPos = m_nHeadSize;
		m_nPacketSize = nLen;

		if(m_nPacketSize < m_nHeadSize)
			return false;

		return true;
	}

	bool _convert(const void *pInBufObj, int type)
	{
		PacketBase* pInBuf = (PacketBase*)pInBufObj;
		if(pInBuf->packet_size() > PACKET_BUFFER_SIZE)
			return false;
         
		if(PACKET_BY_TYPE == type)
			_resetBY();
		else
			_resetQE();

		if(type == pInBuf->GetPacketType())
		{//	equal
			memcpy(m_strBuf, pInBuf->packet_buf(), pInBuf->packet_size());
			m_nHeadSize = pInBuf->GetPacketHeadSize();
			m_nBufPos = m_nHeadSize;
			m_nPacketSize = pInBuf->packet_size();
		}
		else if(PACKET_BY_TYPE == type)
		{// qe to by 
			short nCmdType = pInBuf->GetCmdType();
			char cVersion = pInBuf->GetVersion(); 
			char cSubVersion = SERVER_PACKET_DEFAULT_SUBVER; 
			BYTE code = pInBuf->GetcbCheckCode();

			short cmd = htons(nCmdType);
			_writeHeader("BY", sizeof(char)*2, 2);			// magic word
			_writeHeader(&cVersion, sizeof(char), 4);		// 主版本号
			_writeHeader(&cSubVersion, sizeof(char), 5);	// 子版本号
			_writeHeader((char*)&cmd, sizeof(short), 6);	// 命令码

			m_nType = PACKET_BY_TYPE;
			m_nHeadSize = _GetPacketHeadSize();
			m_nBufPos = m_nHeadSize;
			m_nPacketSize = m_nHeadSize;

			_Write(pInBuf->packet_buf()+pInBuf->GetPacketHeadSize(),  pInBuf->packet_size()-pInBuf->GetPacketHeadSize());

			short nBody = m_nPacketSize - 2;	//数据包长度包括命令头和body,2个字节是数据包长度
			short len = htons(nBody);
			_writeHeader((char*)&len, sizeof(short), 0);	// 包正文长度
			_writeHeader((char*)&code, sizeof(BYTE), 8);	//效验码

		}
		else
		{// by to qe
			short nCmdType = pInBuf->GetCmdType();
			char cVersion = pInBuf->GetVersion(); 
			char cSubVersion = pInBuf->GetSubVersion();
			BYTE code = pInBuf->GetcbCheckCode();

			int cmd = htonl(nCmdType);
			_writeHeader("QE", sizeof(char)*2, 4);			// magic word
			_writeHeader(&cVersion, sizeof(char), 6);		// 主版本号
			BYTE extendlen = 0;
			_writeHeader(&extendlen, sizeof(char), 7);	// Extend
			_writeHeader((char*)&cmd, sizeof(int), 8);		// 命令码
			short gameid = GAMEID;//GetGameId();//GetGameId();

			short regionid = 0;//GetRegionId();
			BYTE deviceid = 0;//GetDeviceId();
			int  uid= 0;
			_writeHeader((char*)&gameid, sizeof(short), 12);		// 游戏ID
			//_writeHeader((char*)&regionid, sizeof(short), 14);	// 平台ID
			//_writeHeader((char*)&deviceid, sizeof(BYTE), 16);		// 业务ID
			//_writeHeader((char*)&uid, sizeof(int), 17);			// 用户ID
			
			m_nType = PACKET_QE_TYPE;
			m_nHeadSize = _GetPacketHeadSize();
			m_nBufPos = m_nHeadSize;
			m_nPacketSize = m_nHeadSize;

			_Write(pInBuf->packet_buf()+pInBuf->GetPacketHeadSize(),  pInBuf->packet_size()-pInBuf->GetPacketHeadSize());


			int nBody = m_nPacketSize - 4;	//数据包长度包括命令头和body,4个字节是数据包长度
			int len = htonl(nBody);
			_writeHeader((char*)&len, sizeof(int), 0);	// 包正文长度
			//BYTE code = 0;
			_writeHeader((char*)&code, sizeof(BYTE), 14);	//效验码
	
		}

		if(m_nPacketSize>=m_nHeadSize)
			return true;
		else
			return false;
	}
	
	////////////////////////////////////////////////////////////////////////////////
	void _begin(short nCmdType, char cVersion, char cSubVersion)
	{
#if defined(__QE__)
		_beginQE(nCmdType, cVersion, cSubVersion);
		return;
#endif
		_beginBY(nCmdType, cVersion, cSubVersion);
	}

	void _beginBY(short nCmdType, char cVersion, char cSubVersion)
	{
		_resetBY();
		short cmd = htons(nCmdType);
		_writeHeader("BY", sizeof(char)*2, 2);			// magic word
		_writeHeader(&cVersion, sizeof(char), 4);		// 主版本号
		_writeHeader(&cSubVersion, sizeof(char), 5);	// 子版本号
		_writeHeader((char*)&cmd, sizeof(short), 6);	// 命令码


		m_nType = PACKET_BY_TYPE;
		m_nHeadSize = _GetPacketHeadSize();
		m_nBufPos = m_nHeadSize;
		m_nPacketSize = m_nHeadSize;
	}

	void _beginQE(int nCmdType, char cVersion, char cSubVersion)
	{
		_resetQE();		
		short gameid = GAMEID;//GetGameId();		
		//short regionid = GetRegionId();		//BYTE deviceid = GetDeviceId();

       // nCmdType = (regionid<<16)|(nCmdType&0xFFFF);
		int cmd = htonl(nCmdType);
		_writeHeader("QE", sizeof(char)*2, 4);			// magic word
		_writeHeader(&cVersion, sizeof(char), 6);		// 主版本号
		BYTE extendlen = 0;
		_writeHeader((char*)&extendlen, sizeof(char), 7);	// Extend
		_writeHeader((char*)&cmd, sizeof(int), 8);		// 命令码
		
		gameid = htons(gameid);
		_writeHeader((char*)&gameid, sizeof(short), 12);		// 游戏ID

		//_writeHeader((char*)&regionid, sizeof(short), 14);		// 平台ID
		//_writeHeader((char*)&deviceid, sizeof(BYTE), 16);			// 业务ID
		//int  uid= 0;
		//_writeHeader((char*)&uid, sizeof(int), 17);				// 用户ID

		m_nType = PACKET_QE_TYPE;
		m_nHeadSize = _GetPacketHeadSize();
		m_nBufPos = m_nHeadSize;
		m_nPacketSize = m_nHeadSize;
	}

	int _GetPacketHeadSize(void)
	{
		if(m_nType ==  PACKET_QE_TYPE)
		{
			return (PACKET_QE_HEADER_SIZE+GetExtendLen());
		}
		else
			return PACKET_BY_HEADER_SIZE;

	}

	void _WriteExtenSize()
	{
		BYTE extendlen = m_nHeadSize - PACKET_QE_HEADER_SIZE;
		_writeHeader((char*)&extendlen, sizeof(char), 7);	// Extend
	}

	bool _WriteExtend(const char *pIn, int nLen)
	{
		if((m_nPacketSize < 0) || ((nLen + m_nPacketSize) > PACKET_BUFFER_SIZE))
			return false ;

		memcpy(m_strBuf+m_nHeadSize, pIn, nLen);
		m_nHeadSize+=nLen;
		m_nBufPos = m_nHeadSize;
		m_nPacketSize = m_nHeadSize;

        _WriteExtenSize();
		return true;
	}

	bool _ReadExtend(char *pOut, int nPos, int nLen)
	{
		if((nPos + nLen) > m_nHeadSize )
			return false ;

		memcpy(pOut, m_strBuf+PACKET_QE_HEADER_SIZE+nPos, nLen);
		return true;
	}

public:
	void SetPacketType(int type) { m_nType = type; }
	int GetPacketType() {return m_nType; }

	int GetPacketHeadSize(void){ return m_nHeadSize;}

	void ResetHeadSize(int headsize)
	{
		m_nHeadSize = headsize;
		m_nBufPos = headsize;
		m_nPacketSize = headsize;
	}

	int GetExtendLen()
	{
		if(m_nType ==  PACKET_QE_TYPE)
		{
			BYTE extendlen;
			_readHeader((char*)&extendlen, sizeof(BYTE), 7);

			return (int)extendlen;
		}
		else
			return 0;
	}
   
	bool WriteIntExtend(int nValue) {int value = htonl(nValue); return _WriteExtend((char*)&value, sizeof(int));}
	bool WriteShortExtend(short nValue) {short value = htons(nValue); return _WriteExtend((char*)&value, sizeof(short));}
	bool WriteByteExtend(BYTE nValue) {return _WriteExtend((char*)&nValue, sizeof(BYTE));}
    
	int ReadIntExtend(int pos)		{int nValue = -1; _ReadExtend((char*)&nValue, pos, sizeof(int)); return ntohl(nValue);} 
	short ReadShortExtend(int pos)	{short nValue = -1; _ReadExtend((char*)&nValue, pos, sizeof(short)); return ntohs(nValue);}
	BYTE ReadByteExtend(int pos)	{BYTE nValue = -1; _ReadExtend((char*)&nValue, pos, sizeof(BYTE)); return nValue;}
    


	static int GetPacketTypeFromBuf(char* buf)
	{
		if(buf[4] == 'Q' && buf[5] == 'E')
		{
			return  PACKET_QE_TYPE;
		}
		else if(buf[2] == 'B' && buf[3] == 'Y')
		{
			return PACKET_BY_TYPE;
		}
        return -1;
	}

	short GetCmdType(void)
	{
		if(PACKET_QE_TYPE == m_nType)
			return GetCmdTypeQE();

		short nCmdType;
		_readHeader((char*)&nCmdType, sizeof(short), 6);// 命令码
		short cmd = ntohs(nCmdType);
		return cmd;
	}
    
	short GetCmdTypeQE()
	{
		 int nCmdType;
		 _readHeader((char*)&nCmdType, sizeof(int), 8);// 命令码
		 int cmd = ntohl(nCmdType);
		 return (short)(0xFFFF&cmd);
	}

	char GetVersion(void)
	{
		if(PACKET_QE_TYPE == m_nType)
			return GetVersionQE();

		char c;
		_readHeader(&c, sizeof(char), 4);	// 主版本号
		return c;
	}
    
	char GetVersionQE(void)
	{
		char c;
		_readHeader(&c, sizeof(char), 6);	// 主版本号
		return c;
	}

	char GetSubVersion(void)
	{
		if(PACKET_QE_TYPE == m_nType)
			return GetSubVersionQE();

		char c;
		_readHeader(&c, sizeof(char), 5);	// 子版本号
		return c;
	}

	char GetSubVersionQE(void)
	{
		return SERVER_PACKET_DEFAULT_SUBVER;
		
		//char c;
		//_readHeader(&c, sizeof(char), 7);	// 子版本号
		//return c;
	}
	short GetBodyLength(void)
	{
		if(PACKET_QE_TYPE == m_nType)
			return GetBodyLengthQE();

		short nLen;
		_readHeader((char*)&nLen, sizeof(short), 0);// 包正文长度
		short len = ntohs(nLen);
		return len;
	}

	short GetBodyLengthQE(void)
	{
		int nLen;
		_readHeader((char*)&nLen, sizeof(int), 0);// 包正文长度
		int len = ntohl(nLen);
		return (short)len;
	}

	int GetSizePlkLen()
	{
		if(PACKET_QE_TYPE == m_nType)
			return 4;
		
        return 2;
	}

	BYTE GetcbCheckCode(void)
	{
		if(PACKET_QE_TYPE == m_nType)
			return GetcbCheckCodeQE();

		BYTE code;
		_readHeader((char*)&code, sizeof(BYTE), 8);// cb code
		return code;
	}

	BYTE GetcbCheckCodeQE(void)
	{
		BYTE code;
		_readHeader((char*)&code, sizeof(BYTE), 14);// cb code
		return code;
	}
protected:
	void _end()
	{
		if(PACKET_QE_TYPE == m_nType)
		{
			_endQE();
			return;
		}

		short nBody = m_nPacketSize - 2;	//数据包长度包括命令头和body,2个字节是数据包长度
		short len = htons(nBody);
		_writeHeader((char*)&len, sizeof(short), 0);	// 包正文长度
		BYTE code = 0;
		_writeHeader((char*)&code, sizeof(BYTE), 8);	//效验码
	}

	void _endQE()
	{
		int nBody = m_nPacketSize - 4;	//数据包长度包括命令头和body,4个字节是数据包长度
		int len = htonl(nBody);
		_writeHeader((char*)&len, sizeof(int), 0);	// 包正文长度
		BYTE code = 0;
		_writeHeader((char*)&code, sizeof(BYTE), 14);	//效验码
	}

	void _oldend()
	{
		short nBody = m_nPacketSize - 2;
		short len = ntohs(nBody);
		_writeHeader((char*)&len, sizeof(short), 0);	// 包正文长度
	}
	/////////////////////////////////////////////////////////////////////////////////
	void _reset(void)
	{
		//memset(m_strBuf, 0, PACKET_BUFFER_SIZE);
		m_nHeadSize = 0;
		m_nBufPos = 0;
		m_nPacketSize = 0;
	}

	void _resetBY(void)
	{
		//memset(m_strBuf, 0, PACKET_BUFFER_SIZE);
		m_nHeadSize = 0;
		m_nBufPos = 0;
		m_nPacketSize = 0;
	}

	void _resetQE(void)
	{
		//memset(m_strBuf, 0, PACKET_BUFFER_SIZE);
		m_nHeadSize = 0;
		m_nBufPos = 0;
		m_nPacketSize = 0;
	}

	// 取出一个变量
	bool _Read(char *pOut, int nLen)
	{
		if((nLen + m_nBufPos) > m_nPacketSize || (nLen + m_nBufPos) > PACKET_BUFFER_SIZE)
			return false ;

		memcpy(pOut, m_strBuf + m_nBufPos, nLen);
		m_nBufPos += nLen;
		return true;
	}
	//取出变量并从包中移除
	bool _ReadDel(char *pOut, int nLen)
	{
		if(!_Read(pOut, nLen))
			return false;
		memcpy(m_strBuf + m_nBufPos - nLen, m_strBuf + m_nBufPos, PACKET_BUFFER_SIZE - m_nBufPos);
		m_nBufPos -= nLen;
		m_nPacketSize -= nLen;
		_end();
		return true;
	}
	//读撤消
	void _readundo(int nLen)
	{
		m_nBufPos -= nLen;
	}
	//读出当前POS位置的BUFFER指针
	char *_readpoint(int nLen) //注意返回的是指针 请慎重使用string
	{
		if((nLen + m_nBufPos) > m_nPacketSize)
			return NULL; 
		char *p = &m_strBuf[m_nBufPos];
		m_nBufPos += nLen;
		return p;

	}
	// 写入一个变量
	bool _Write(const char *pIn, int nLen)
	{
		if((m_nPacketSize < 0) || ((nLen + m_nPacketSize) > PACKET_BUFFER_SIZE))
			return false ;
		memcpy(m_strBuf+m_nPacketSize, pIn, nLen);
		m_nPacketSize += nLen;
		return true;
	}
	//插入一个变量
	bool _Insert(const char *pIn, int nLen)
	{
		if((nLen + m_nPacketSize) > PACKET_BUFFER_SIZE)
			return false;
		memcpy(m_strBuf+GetPacketHeadSize()+nLen, m_strBuf+GetPacketHeadSize(), m_nPacketSize-GetPacketHeadSize());
		memcpy(m_strBuf+GetPacketHeadSize(), pIn, nLen);
		m_nPacketSize += nLen;
		_end();
		return true;
	}
	// 写入一个变量
	bool _writezero(void)
	{
		if((m_nPacketSize + 1) > PACKET_BUFFER_SIZE)
			return false ;
		memset(m_strBuf+m_nPacketSize, '\0', sizeof(char)) ;
		m_nPacketSize ++;
		return true;
	}
	// readHeader
	void _readHeader(char *pOut, int nLen, int nPos)
	{
		memcpy(pOut, m_strBuf+nPos, nLen) ;
	}
	// writeHeader
	void _writeHeader(char *pIn, int nLen, int nPos)
	{
		memcpy(m_strBuf+nPos, pIn, nLen) ;
	}
};

template <int _buffer_size>
class InputPacket: public PacketBase<_buffer_size>
{
public:
	typedef PacketBase<_buffer_size> base;

	int ReadInt(void)		{int nValue = -1; base::_Read((char*)&nValue, sizeof(int)); return ntohl(nValue);} //这里必需初始化
	bool ReadInt(int& nValue){nValue=-1; bool ret=base::_Read((char*)&nValue, sizeof(int)); nValue=ntohl(nValue); return ret;}
	unsigned long ReadULong(void) {unsigned long nValue = -1; base::_Read((char*)&nValue, sizeof(unsigned long)); return ntohl(nValue);}
	int ReadIntDel(void)	{int nValue = -1; base::_ReadDel((char*)&nValue, sizeof(int)); return ntohl(nValue);} 
	short ReadShort(void)	{short nValue = -1; base::_Read((char*)&nValue, sizeof(short)); return ntohs(nValue);}
	BYTE ReadByte(void)		{BYTE nValue = -1; base::_Read((char*)&nValue, sizeof(BYTE)); return nValue;}

	bool ReadString(char *pOutString, int nMaxLen)
	{
		int nLen = ReadInt();
		if(nLen == -1)  //这里必需判断
			return false;
		if(nLen > nMaxLen)
		{
			base::_readundo(sizeof(short));
			return false;
		}
		return base::_Read(pOutString, nLen);
	}

	char *ReadChar(void)
	{
		int nLen = ReadInt();
		if(nLen == -1) 
			return NULL;
		return base::_readpoint(nLen);
	}

	string ReadString(void)
	{
		char *p = ReadChar();
		return (p == NULL ? "" : p);
	}

	int ReadBinary(char *pBuf, int nMaxLen)
	{
		int nLen = ReadInt();
		if(nLen == -1) 
		{
			return -1;
		}
		if(nLen > nMaxLen)
		{
			base::_readundo(sizeof(int));
			return -1;
		}
		if(base::_Read(pBuf, nLen))
			return nLen ;
		return 0;
	}
	void Reset(void)
	{
		base::_reset();
	}
	bool Copy(const void *pInBuf, int nLen)
	{
		return base::_copy(pInBuf, nLen);
	}
	bool WriteBody(const char *pIn, int nLen)
	{
		return base::_Write(pIn, nLen);
	}
	////用于伪造接收包
	void Begin(short nCommand, char cVersion = SERVER_PACKET_DEFAULT_VER, char cSubVersion = SERVER_PACKET_DEFAULT_SUBVER)
	{
		base::_begin(nCommand, cVersion, cSubVersion);
	}
	void End(void)
	{
		base::_end();
	}
};

template <int BUFFER_SIZE>
class OutputPacket: public PacketBase<BUFFER_SIZE>
{
	bool m_isCheckCode;
public:
	OutputPacket(void){m_isCheckCode = false;}
public:
	typedef PacketBase<BUFFER_SIZE> base;

	bool WriteInt(int nValue)		{int value = htonl(nValue); return base::_Write((char*)&value, sizeof(int));}
	bool WriteULong(unsigned long nValue) {unsigned long value = htonl(nValue);return base::_Write((char*)&value, sizeof(unsigned long));}
	bool WriteByte(BYTE nValue)		{return base::_Write((char*)&nValue, sizeof(BYTE));}
	bool WriteShort(short nValue)	{short value = htons(nValue); return base::_Write((char*)&value, sizeof(short));}
	//在正文首插入数据
	bool InsertInt(int nValue)		{int value = htonl(nValue); return base::_Insert((char*)&value, sizeof(int));}
	bool InsertByte(BYTE nValue)	{return base::_Insert((char*)&nValue, sizeof(BYTE));}
	bool WriteString(const char *pString)
	{
		int nLen = (int)strlen(pString) ;
		WriteInt(nLen + 1) ;
		if ( nLen > 0 )
		{
			return base::_Write(pString, nLen) && base::_writezero();
		}
		else
		{
			return base::_writezero();
		}
	}

	bool WriteString(const string &strDate)
	{
		int nLen = (int)strDate.size();
		WriteInt(nLen + 1);
		if ( nLen > 0 )
		{
			return base::_Write(strDate.c_str(), nLen) && base::_writezero();
		}
		else
		{
			return base::_writezero();
		}
	}

	bool WriteBinary(const char *pBuf, int nLen)
	{
		WriteInt(nLen) ;
		return base::_Write(pBuf, nLen) ;
	}
	bool Copy(const void *pInBuf, int nLen)
	{
		return base::_copy(pInBuf, nLen);
	}
	bool Convert(const void *pInBufObj, int type)
	{
		return base::_convert(pInBufObj, type);
	}

	void Begin(short nCommand, char cVersion = SERVER_PACKET_DEFAULT_VER, char cSubVersion = SERVER_PACKET_DEFAULT_SUBVER)
	{
		base::_begin(nCommand, cVersion, cSubVersion);
		m_isCheckCode = false;
	}

    void BeginBY(short nCommand, char cVersion = SERVER_PACKET_DEFAULT_VER, char cSubVersion = SERVER_PACKET_DEFAULT_SUBVER)
	{
		base::_beginBY(nCommand, cVersion, cSubVersion);
		m_isCheckCode = false;
	}

	void BeginQE(short nCommand, char cVersion = SERVER_PACKET_DEFAULT_VER, char cSubVersion = SERVER_PACKET_DEFAULT_SUBVER)
	{
		base::_beginQE(nCommand, cVersion, cSubVersion);
		m_isCheckCode = false;
	}

	void End(void)
	{
		m_isCheckCode = false;
		base::_end();
	}
	void oldEnd(void)
	{
		m_isCheckCode = false;
		base::_oldend();
	}
	//增加
	void SetBegin(short nCommand)
	{
		//base::_SetBegin(nCommand);
	}
	//效验码
	void WritecbCheckCode(BYTE nValue)
	{
		//base::_writeHeader((char*)&nValue, sizeof(BYTE), 8); //效验码
		//m_isCheckCode = true;

		if(base::PACKET_QE_TYPE == base::GetPacketType())
		{
			WritecbCheckCodeQE(nValue);
			return;
		}
		base::_writeHeader((char*)&nValue, sizeof(BYTE), 8); //效验码
		m_isCheckCode = true;
	}

	//效验码
	void WritecbCheckCodeQE(BYTE nValue)
	{
		base::_writeHeader((char*)&nValue, sizeof(BYTE), 14); //效验码
		m_isCheckCode = true;
	}

	bool IsWritecbCheckCode(void)
	{
		return m_isCheckCode;
	}
};

typedef InputPacket<ICHAT_TCP_DEFAULT_BUFFER>	NETInputPacket;
typedef OutputPacket<ICHAT_TCP_DEFAULT_BUFFER>	NETOutputPacket;
typedef OutputPacket<ICHAT_TCP_MIN_BUFFER>		NETMINOutputPacket;
/* 测试代码
typedef InputPacket<8192>	MyInputPacket;
typedef OutputPacket<512>	MyOutputPacket;

class MyParser:public ICHAT_PacketParser<MyInputPacket>
{
	int OnPacketComplete(MyInputPacket *packet)
	{
		short nCmd = packet->GetCmdType();
		char v1 = packet->GetVersion();
		char v2 = packet->GetSubVersion();
		
		int i1 = packet->ReadInt();
		int n1 = packet->ReadInt();
		int n2 = packet->ReadInt();
		short s1 = packet->ReadShort();
		short s2 = packet->ReadShort();
		char c1 = packet->ReadByte();
		char c2 = packet->ReadByte();
		char str[255];
		if(!packet->ReadString(str, 2))
			packet->ReadString(str, 255);

		char buf[20];
		assert(packet->ReadBinary(buf, 20) == 10);
		return 0;
	}
};
int main(int argc, char *argv[])
{
	///////////////////////////////////////////////////////////////////////////test
	MyParser Parser;
	MyInputPacket m_In;
	MyOutputPacket m_Out;

	m_Out.Begin(1234);
	m_Out.WriteInt(0);
	m_Out.WriteInt(0xFFFFFFFF);
	m_Out.WriteShort(0);
	m_Out.WriteShort(32767);
	m_Out.WriteByte(0);
	m_Out.WriteByte(255);
	m_Out.WriteString("0123456789");
	m_Out.WriteBinary("9876543210", 10);
	m_Out.End();
	

	m_Out.InsertInt(54321);
	char temp[1024] ;
	memcpy(temp, m_Out.packet_buf(), m_Out.packet_size());
	memcpy(temp+m_Out.packet_size(), m_Out.packet_buf(), m_Out.packet_size());

	for(int i = 0; i < m_Out.packet_size() * 2; i ++)
	{
		if(Parser.ParsePacket(temp+i, 1) == -1)
			break;
	}
	m_In.Copy(temp, m_Out.packet_size());
	int d1 = m_In.ReadIntDel();
	//int nCmp = memcmp(packet->packet_buf(), m_Out.packet_buf(), m_Out.packet_size());

	short nCmd = m_In.GetCmdType();
	char v1 = m_In.GetVersion();
	char v2 = m_In.GetSubVersion();

	int n1 = m_In.ReadInt();
	int n2 = m_In.ReadInt();
	short s1 = m_In.ReadShort();
	short s2 = m_In.ReadShort();
	char c1 = m_In.ReadByte();
	char c2 = m_In.ReadByte();
	char str[255];
	if(!m_In.ReadString(str, 2))
		m_In.ReadString(str, 255);

	char buf[10];
	assert(m_In.ReadBinary(buf, 10) > 0);
}
*/

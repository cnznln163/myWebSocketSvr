#pragma once

#include "ace/Log_Msg.h"

enum PACKETVER
{
	SERVER_PACKET_DEFAULT_VER = 1,
	SERVER_PACKET_DEFAULT_SUBVER = 1
};


template <class INPUT_PACKET>
class ICHAT_PacketParser
{
public:
	ICHAT_PacketParser(void)
	{
		m_pBuf = m_Packet.packet_buf();
		m_version = SERVER_PACKET_DEFAULT_VER;
		m_subVersion = SERVER_PACKET_DEFAULT_SUBVER;
		reset();
	}
	virtual ~ICHAT_PacketParser(void){}
	// 返回Packet
	virtual int OnPacketComplete(INPUT_PACKET *) = 0;
	// 处理PACKET数据
	int ParsePacket(char *data, int length)
	{
		int ret = -1; //出错返回
		int ndx = 0;
		while(ndx < length && m_nStatus != REQ_ERROR)//可能会同时来两个包 
		{
			switch(m_nStatus)
			{
			case REQ_REQUEST:
			case REQ_HEADER:
				ret = read_header(data, length, ndx);
				if(ret < 0)
			    {	
					m_nStatus = REQ_ERROR;
					break;
				}
				else if(0 == ret)
					break;  //继续接受包头

				ret = parse_header();
				if(ret != 0)
				{
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
protected:
	void reset(void) 
	{
		m_nStatus = REQ_REQUEST;
		m_nPacketPos = 0;
		m_nBodyLen = 0;
		m_nType = -1;
		m_nHeadSize = 0;
		m_Packet.Reset();//包完成复位
	}

public:
	short m_version;
	short m_subVersion;

private:
	// 当前处理状态
	int m_nStatus; 
	// PacketPos
	int	m_nPacketPos;
	// BODY长度
	int m_nBodyLen;
	// PacketBuffer 指针
	char *m_pBuf;
	// PacketBuffer
	INPUT_PACKET m_Packet;
	int m_nType;
	int m_nHeadSize;
	// 状态
	enum REQSTATUS{	REQ_REQUEST=0, REQ_HEADER, REQ_BODY, REQ_DONE, REQ_PROCESS, REQ_ERROR };

private:

	// 读取Packet头数据   //1:成功,  0:继续接受包头,   -1:错误包头
	int read_header(char *data, int length, int &ndx)
	{
		//接收至少 BY 包头大小
		while(m_nPacketPos < INPUT_PACKET::PACKET_BY_HEADER_SIZE && ndx < length)//
		{
			m_pBuf[m_nPacketPos++] = data[ndx++];
		}

		if(m_nPacketPos < INPUT_PACKET::PACKET_BY_HEADER_SIZE)
			return 0;    

		int pack_type;
		int head_size;
		if(m_pBuf[4] == 'Q' && m_pBuf[5] == 'E')
		{
			pack_type = INPUT_PACKET::PACKET_QE_TYPE;
			head_size = INPUT_PACKET::PACKET_QE_HEADER_SIZE;
		}
		else if(m_pBuf[2] == 'B' && m_pBuf[3] == 'Y')
		{
			pack_type = INPUT_PACKET::PACKET_BY_TYPE;
			head_size = INPUT_PACKET::PACKET_BY_HEADER_SIZE;
		}
		else
		{
			return -1;
		}

		if(pack_type == INPUT_PACKET::PACKET_QE_TYPE)
		{
			//接收 QE 包头
			while(m_nPacketPos < head_size && ndx < length)//
			{
				m_pBuf[m_nPacketPos++] = data[ndx++];
			}

			if(m_nPacketPos < head_size)
				return 0;  

			//接收 QE 扩展包头
			head_size = get_qe_header_size(m_pBuf);
			while(m_nPacketPos < head_size && ndx < length)//
			{
				m_pBuf[m_nPacketPos++] = data[ndx++];
			}

			if(m_nPacketPos < head_size)
				return 0;  
			
		}
        m_nHeadSize = head_size;
		m_nType = pack_type;
		return 1;
	}

	//计算 QE包头长
	int get_qe_header_size(char *data)
	{
		BYTE extendlen;
		memcpy((char*)&extendlen, data+7, sizeof(BYTE)) ;
		
        int head_size = INPUT_PACKET::PACKET_QE_HEADER_SIZE+(int)extendlen;
		return head_size;
	}

	// 解析Packet头信息
	int parse_header(void) //0:成功 -1:包错误 -2:命令范围错误 -3:版本错误 -4:长度错误
	{
		m_Packet.SetPacketType(m_nType);
		m_Packet.ResetHeadSize(m_nHeadSize);
		//m_Packet.m_nHeadSize = m_nHeadSize;
		//m_Packet.m_nBufPos = m_nHeadSize;
		//m_Packet.m_nPacketSize = m_nHeadSize;

		short nCmdType = m_Packet.GetCmdType();
		if(nCmdType <= 0 || nCmdType >= 32000)
			return -2;

		char v1 = m_Packet.GetVersion();
		char v2 = m_Packet.GetSubVersion();

		if(v1 != m_version || v2 != m_subVersion)
			return -3;
		


		m_nBodyLen = m_Packet.GetBodyLength();
		if(m_nBodyLen >= 0 && m_nBodyLen < (INPUT_PACKET::PACKET_BUFFER_SIZE - 4))
			return 0;


		return -4;
	}


	// 解析BODY数据
	bool parse_body(char *data, int length, int &ndx)
	{
		int pkglen = m_Packet.GetSizePlkLen();
		int nNeed = (m_nBodyLen+pkglen) - m_nPacketPos;
		int nBuff = length - ndx;

		if(nNeed <= 0)
			return true;
		if(nBuff <= 0)
			return false;

		int nCopy = nBuff < nNeed ? nBuff : nNeed;
		if(!m_Packet.WriteBody(data + ndx, nCopy))
			return false;

		m_nPacketPos += nCopy;
		ndx += nCopy;

		if(m_nPacketPos < (m_nBodyLen + pkglen))
			return false;
        

		return true;
	}
    
	int RecvPaket(char* data, int len, int& ndx)
	{
		int pack_type;
		int pkglen = 0;

		if(REQ_REQUEST == m_nStatus)
		{
			if(data[4] == 'Q' && data[5] == 'E')
			{
				pack_type = INPUT_PACKET::PACKET_QE_TYPE;

				int nLen;
				memcpy((char*)&nLen, data, sizeof(int)) ;  
				pkglen = ntohl(nLen) + sizeof(int);
			}
			else if(data[2] == 'B' && data[3] == 'Y')
			{
				pack_type = INPUT_PACKET::PACKET_BY_TYPE;

				short nLen;
				memcpy((char*)&nLen, data, sizeof(short)) ; 
				pkglen = ntohs(nLen) + sizeof(short);	
			}
			else
			{
				ACE_DEBUG((LM_ERROR, ACE_TEXT("[%D]RecvPaket_head_err!!  %s %s %d  \r\n"), __FUNCTION__,  __FILE__, __LINE__ ));
				return -1;
			}

			if(pkglen<=0 || pkglen>INPUT_PACKET::PACKET_BUFFER_SIZE)
			{
				ACE_DEBUG((LM_ERROR, ACE_TEXT("[%D]RecvPaket_len_err!!  pkglen=%d, buf_len=%d, BUFFER_SIZE=%d,   %s %s %d  \r\n"), 
					pkglen, len, INPUT_PACKET::PACKET_BUFFER_SIZE, __FUNCTION__,  __FILE__, __LINE__ ));
				return -1;
			}

			//if(pkglen>len)
			//{//对于分包的情况, 第一个包必须大于包头
   //             m_nStatus =  REQ_BODY;
			//}

		}


		if(m_Packet.Copy(data, pkglen))
		{
			m_nPacketPos += pkglen;
			m_nBodyLen = m_Packet.GetBodyLength();
			ndx += pkglen;

			short nCmdType = m_Packet.GetCmdType();
			if(nCmdType <= 0 || nCmdType >= 32000)    //0x7D00
				return false;

			//char v1 = m_Packet.GetVersion();
			//char v2 = m_Packet.GetSubVersion();
			//if(v1 != m_version || v2 != m_subVersion)
			//	return false;
		}
		else
		{
			ACE_DEBUG((LM_ERROR, ACE_TEXT("[%D]RecvPaket_Copy_packet_err!!  %s %s %d  \r\n"),  __FUNCTION__,  __FILE__, __LINE__ ));
			return false;
		}

		return true;
	}

};

//short ICHAT_PacketParser<NETInputPacket>::m_version = SERVER_PACKET_DEFAULT_VER;
//short ICHAT_PacketParser<NETInputPacket>::m_subVersion = SERVER_PACKET_DEFAULT_SUBVER;

#pragma once

#define HTTP_SVR_NS_BEGIN namespace __http_svr_ns__ {
#define HTTP_SVR_NS_END }
#define USING_HTTP_SVR_NS using namespace __http_svr_ns__;

#define MAX_WEB_RECV_LEN            102400
#define MAX_ACCEPT_COUNT            256

enum TDecodeStatus
{
	DECODE_FATAL_ERROR,
	DECODE_DATA_ERROR,
	DECODE_WAIT_HEAD,
	DECODE_WAIT_CONTENT,
	DECODE_HEAD_DONE,
	DECODE_CONTENT_DONE,
	DECODE_DONE,
	DECODE_DISCONNECT_BY_USER,
	DECODE_SAME_SERVER_REG = 8        //����ͬ����Server����ͬServeridע��
};

typedef struct tagRspList
{
	char*   _buffer;
	int     _len;
	int     _sent;
	int     _type;

	struct tagRspList* _next;

}TRspList;

enum
{
	CLIENT_CMD_SYC = 0x0002,
	SERVER_CMD_SYC = 0x0001
};

enum ConnType
{
	CONN_CLINET,
	CONN_OTHER
};

#define THREE_PRIVATE_ROOM_LEVEL		0
#define DOUBLE_PRIVATE_ROOM_LEVEL	4001

//server��ͨ��Э��
#define CLIENT_PACKET  		 0x0001//�û������������ݰ�����server�����û��İ�





#define SERVER_CLOSE_PACKET  0x0003//�߼�server�����Ͽ����ӵİ�
#define SYS_RELOAD_CONFIG    0x0010//���¼�������
const int SYS_RELOAD_IP_MAP = 0x0011;//���¼���ipӳ��
const int SYS_RESET_CONNECT_TIMER = 0x0012;//��������connect��ʱʱ��


const int LOGIN_SERVER_CMD_LOGIN = 0x0031;//��¼SERVER���͵�¼
const int LOGIN_SERVER_CMD_LOGIN_GAME = 0x0053;//��¼SERVER���͵�¼


const int CLIENT_CMD_CLIENT_LOGIN = 0x0116;//�û�������¼��
const int SERVER_CMD_LOGIN_SUCCESS = 0x0201;//�û���¼�����ɹ�

//���ȼ�������
const int CLIENT_CMD_JOIN_GAME = 0x0113;//�û�������뷿��
const int SERVER_CMD_JOIN_GAME = 0x0210;//�����û����뷿��
const int CLIENT_CMD_JOIN_MATCH = 0x0120; //�û�������������
const int CLIENT_CMD_APPLY_MATCH = 0x0121; //�û�������
const int CLIENT_CMD_JOIN_HUODONG = 0x0122; //�û����������
const int SERVER_CMD_JOIN_MATCH= 0x0211;//�����û�������������

const int CMD_GET_NEW_ROOM = 0x0117;//����������׼��,�������󷿼��
//˽�˷�����������
const int CLIENT_CMD_CREATE_PRIVATEROOM = 0x114;//�û����󴴽�˽�˷�
const int CLIENT_CMD_CREATE_PRIVATEROOM_NEW = 0x124;//�û����󴴽�˽�˷�

const int CLIENT_CMD_LIST_PRIVATEROOM = 0x10D;//�û�����˽�˷����б�
const int CLIENT_CMD_LIST_PRIVATEROOM_NEW = 0x11D;//�û�����˽�˷����б�

const int CLIENT_CMD_ENTER_PRIVATEROOM = 0x0115;//�û��������˽�˷�
const int CLIENT_CMD_ENTER_PRIVATEROOM_NEW = 0x0125;//�û��������˽�˷�

const int CLIENT_CMD_FIND_PRIVATEROOM_LIKE= 0x010e;//�û�����ģ����ѯ˽�˷���
const int CLIENT_CMD_FIND_PRIVATEROOM_LIKE_NEW= 0x011e;//�û�����ģ����ѯ˽�˷���

//˽�˷����˳�������
const int CLIENT_CMD_CREATE_PRIVATEROOM_DOUBLE = 0x134;	//�û����󴴽�����˽�˷�
const int CLIENT_CMD_LIST_PRIVATEROOM_DOUBLEE  = 0x135; //�û��������˽�˷��б�
const int CLIENT_CMD_ENTER_PRIVATEROOM_DOUBLE  = 0x136; //�û�������˽������˽�˷�
const int CLIENT_CMD_PRIVATEROOM_DOUBLE_LIKE    = 0x137; //�û�����ģ����ѯ����˽�˷�


const int SERVER_CMD_ENTER_PRIVATEROOM = 0x0212;//�����û�����˽�˷���
const int SERVER_CMD_CREATE_PRIVATEROOM = 0x206;//���ط���������˽�˷����


const int CMD_REQUIRE_IP_PORT = 0x0604;//����server��ip�Ͷ˿�

const int SERVER_CMD_KICK_OUT = 0x0203;//�޳��û�

const int CMD_HALL_SERVER_MAX = 0x0800;//����ָ�

const int CLIENT_COMMAND_LOGIN = 0x1001;//�û���¼ĳ������
const int SERVER_COMMAND_LOGOUT_SUCCESS = 0x1008;//�û��ɹ��˳�����

const int CLIENT_COMMAND_BREAK_TIME = 0x2008;//������
const int SERVER_COMMAND_BREAK_TIME = 0x600D;//��������

const int SERVER_BROADCAST_INFO = 0x7050;  // broadcast system
const int SERVER_BROADCAST_INFO_NEW = 0x7054;//�µĹ㲥Э��

const int SERVER_BROADCAST_INFO_SINGAL= 0x7052;//���͸����˵Ĺ㲥
const int SERVER_SEND_PROTOCOL_SINGAL = 0x7053;//���͸����˵�Э��
const int REQUEST_SENDPRIVT2INVATE_TOHALL_SINGAL = 0x7060;	//2��˽�˷������������Э��


const int SERVER_COMMAND_LOGIN_ERR = 0x1005;	//��½����
const int SERVER_COMMAND_OTHER_ERR = 0x1006;	//��������
const int SERVER_COMMAND_PRVTROOM_KICKUSER = 0x2111;//˽�˷�����

const int SERVER_COMMAND_CHANGE_GAMESERVER = 0x7213;


const int SERVER_LOGIN_RETURN = 0x0002;


const int UDP_SERVER_LOG	=0x0901;	//�ϱ���������־server
const int UDP_SERVER_CLIENT_CLOSE_LOG = 0x0902;//�ϱ��û��뿪����
const int UDP_SERVER_USER_COUNT 	 = 0x0903;//�ϱ���ǰ�û��ж�����

const int CMD_REQ_HALL_SERVER_REGISTER = 0x0051;
const int CMD_RES_HALL_SERVER_REGISTER = 0x0052;

const int CMD_LOGIN_SERVER_MAX = 0x0050;
const int CMD_REQ_HALL_USER_IP = 0x0057;

const int LOGIN_PACKET = 0x0050;


const int LOGIN_CLIENT_COMMAND_LOGIN = 0x0001;	//�û���½
const int CLIENT_COMMAND_PHP_RETURN = 0x0002;  //php����
const int CLIENT_COMMAND_LOGIN_ERROR = 0x0003; //server��֤���󷵻�
const int CLIENT_COMMAND_GET_BROKE_INFO = 0x0005;		//�û������Ʋ���Ϣ
const int SERVER_RESPONSE_BROKE_INFO = 0x0006;		//��Ӧ�û������Ʋ���Ϣ
const int CLIENT_COMMAND_GET_SERVER_INFO =0x0007;		//�û�����server��Ϣ
const int SERVER_RESPONSE_SERVER_INFO = 0x0008;	//��Ӧ�û�����serve��Ϣ
const int CILENT_COMMAND_REQUIRE_BROKE = 0x0009;		//�û������Ʋ�����
const int SERVER_RESPONSE_REQUIRE_BROKE = 0x0010;		//��Ӧ�û������Ʋ��������
const int CLIENT_CONMAND_GET_TELE_INFO = 0x0011;		//�û������ȡ�绰��Ϣ
const int SERVER_RESPONSE_TELE_INFO = 0x0012;			//�����û������ȡ�绰��Ϣ
const int CLIENT_COMMAND_GET_PSW = 0x0013;			//�û������ȡ����
const int SERVER_RESPONSE_GET_PSW = 0x0014;
const int CLIENT_COMMAND_GET_BROKE_COUNT = 0x0015;
const int SERVER_RESPONSE_BROKE_COUNT	 = 0x0016;
const int CLIENT_COMMAND_GET_GOOD_LIST = 0x0017;
const int SERVER_RESPONSE_GOOD_LIST = 0x0018;
const int CLIENT_COMMAND_BUY_GOOD = 0x0019;
const int SERVER_RESPONSE_BUY_GOOD = 0x0020;
const int CLIENT_COMMAND_CHANGE_INFO = 0x0021;
const int SERVER_RESPONSE_CHANGE_INFO = 0x0022;
const int CLIENT_COMMAND_GET_PHP_REQUIER = 0x0023;
const int SERVER_RESPONSE_GET_PHP_REQUIER= 0x0024;
const int CLIENT_COMMAND_GET_MONEY = 0x0025;
const int SERVER_RESPONSE_GET_MONEY = 0x0026;
const int SERVER_CMD_PHP_OUT_TIME = 0x0027;
const int SERVER_CMD_SEND_LOGIN_LEVEL = 0x0028;
const int CLIENT_COMMAND_GET_LOGIN_LEVEL = 0x0029;
const int SERVER_CMD_TELL_CLIENT_ERROR = 0x0030;
const int SERVER_CMD_TO_HALL_USER_LOGIN= 0x0031;

const int CLIENT_COMMAND_LOGIN_GAME = 0x0051;
const int SERVER_CMD_SEND_LOGIN_GAME = 0x0052;

const int SERVER_COMMAND_CHANGE_LEVEL_TO_LOGIN = 0x2205;//�ı�ȼ�����������������½
const int SERVER_COMMAND_KICK_OUT = 0x4004;



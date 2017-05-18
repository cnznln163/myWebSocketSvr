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
	DECODE_SAME_SERVER_REG = 8        //有相同类型Server用相同Serverid注册
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

//server间通信协议
#define CLIENT_PACKET  		 0x0001//用户发过来的数据包或者server发给用户的包





#define SERVER_CLOSE_PACKET  0x0003//逻辑server主动断开连接的包
#define SYS_RELOAD_CONFIG    0x0010//重新加载配置
const int SYS_RELOAD_IP_MAP = 0x0011;//重新加载ip映射
const int SYS_RESET_CONNECT_TIMER = 0x0012;//重新设置connect超时时间


const int LOGIN_SERVER_CMD_LOGIN = 0x0031;//登录SERVER发送登录
const int LOGIN_SERVER_CMD_LOGIN_GAME = 0x0053;//登录SERVER发送登录


const int CLIENT_CMD_CLIENT_LOGIN = 0x0116;//用户大厅登录包
const int SERVER_CMD_LOGIN_SUCCESS = 0x0201;//用户登录大厅成功

//带等级命令字
const int CLIENT_CMD_JOIN_GAME = 0x0113;//用户请求进入房间
const int SERVER_CMD_JOIN_GAME = 0x0210;//返回用户进入房间
const int CLIENT_CMD_JOIN_MATCH = 0x0120; //用户请求进入比赛场
const int CLIENT_CMD_APPLY_MATCH = 0x0121; //用户请求报名
const int CLIENT_CMD_JOIN_HUODONG = 0x0122; //用户请求进入活动赛
const int SERVER_CMD_JOIN_MATCH= 0x0211;//返回用户请求进入比赛场

const int CMD_GET_NEW_ROOM = 0x0117;//随机场点击了准备,重新请求房间号
//私人房请求命令字
const int CLIENT_CMD_CREATE_PRIVATEROOM = 0x114;//用户请求创建私人房
const int CLIENT_CMD_CREATE_PRIVATEROOM_NEW = 0x124;//用户请求创建私人房

const int CLIENT_CMD_LIST_PRIVATEROOM = 0x10D;//用户请求私人房间列表
const int CLIENT_CMD_LIST_PRIVATEROOM_NEW = 0x11D;//用户请求私人房间列表

const int CLIENT_CMD_ENTER_PRIVATEROOM = 0x0115;//用户请求进入私人房
const int CLIENT_CMD_ENTER_PRIVATEROOM_NEW = 0x0125;//用户请求进入私人房

const int CLIENT_CMD_FIND_PRIVATEROOM_LIKE= 0x010e;//用户请求模糊查询私人房间
const int CLIENT_CMD_FIND_PRIVATEROOM_LIKE_NEW= 0x011e;//用户请求模糊查询私人房间

//私人房二人场命令字
const int CLIENT_CMD_CREATE_PRIVATEROOM_DOUBLE = 0x134;	//用户请求创建二人私人房
const int CLIENT_CMD_LIST_PRIVATEROOM_DOUBLEE  = 0x135; //用户请求二人私人房列表
const int CLIENT_CMD_ENTER_PRIVATEROOM_DOUBLE  = 0x136; //用户请求二人进入二人私人房
const int CLIENT_CMD_PRIVATEROOM_DOUBLE_LIKE    = 0x137; //用户请求模糊查询二人私人房


const int SERVER_CMD_ENTER_PRIVATEROOM = 0x0212;//返回用户进入私人房间
const int SERVER_CMD_CREATE_PRIVATEROOM = 0x206;//返回服务器创建私人房结果


const int CMD_REQUIRE_IP_PORT = 0x0604;//请求server的ip和端口

const int SERVER_CMD_KICK_OUT = 0x0203;//剔除用户

const int CMD_HALL_SERVER_MAX = 0x0800;//命令分隔

const int CLIENT_COMMAND_LOGIN = 0x1001;//用户登录某个房间
const int SERVER_COMMAND_LOGOUT_SUCCESS = 0x1008;//用户成功退出房间

const int CLIENT_COMMAND_BREAK_TIME = 0x2008;//心跳包
const int SERVER_COMMAND_BREAK_TIME = 0x600D;//返回心跳

const int SERVER_BROADCAST_INFO = 0x7050;  // broadcast system
const int SERVER_BROADCAST_INFO_NEW = 0x7054;//新的广播协议

const int SERVER_BROADCAST_INFO_SINGAL= 0x7052;//发送给单人的广播
const int SERVER_SEND_PROTOCOL_SINGAL = 0x7053;//发送给单人的协议
const int REQUEST_SENDPRIVT2INVATE_TOHALL_SINGAL = 0x7060;	//2人私人房房主邀请玩家协议


const int SERVER_COMMAND_LOGIN_ERR = 0x1005;	//登陆错误
const int SERVER_COMMAND_OTHER_ERR = 0x1006;	//其他错误
const int SERVER_COMMAND_PRVTROOM_KICKUSER = 0x2111;//私人房踢人

const int SERVER_COMMAND_CHANGE_GAMESERVER = 0x7213;


const int SERVER_LOGIN_RETURN = 0x0002;


const int UDP_SERVER_LOG	=0x0901;	//上报命令字日志server
const int UDP_SERVER_CLIENT_CLOSE_LOG = 0x0902;//上报用户离开大厅
const int UDP_SERVER_USER_COUNT 	 = 0x0903;//上报当前用户有多少人

const int CMD_REQ_HALL_SERVER_REGISTER = 0x0051;
const int CMD_RES_HALL_SERVER_REGISTER = 0x0052;

const int CMD_LOGIN_SERVER_MAX = 0x0050;
const int CMD_REQ_HALL_USER_IP = 0x0057;

const int LOGIN_PACKET = 0x0050;


const int LOGIN_CLIENT_COMMAND_LOGIN = 0x0001;	//用户登陆
const int CLIENT_COMMAND_PHP_RETURN = 0x0002;  //php返回
const int CLIENT_COMMAND_LOGIN_ERROR = 0x0003; //server验证错误返回
const int CLIENT_COMMAND_GET_BROKE_INFO = 0x0005;		//用户请求破产信息
const int SERVER_RESPONSE_BROKE_INFO = 0x0006;		//回应用户请求破产信息
const int CLIENT_COMMAND_GET_SERVER_INFO =0x0007;		//用户请求server信息
const int SERVER_RESPONSE_SERVER_INFO = 0x0008;	//回应用户请求serve信息
const int CILENT_COMMAND_REQUIRE_BROKE = 0x0009;		//用户请求破产奖励
const int SERVER_RESPONSE_REQUIRE_BROKE = 0x0010;		//回应用户请求破产奖励结果
const int CLIENT_CONMAND_GET_TELE_INFO = 0x0011;		//用户请求获取电话信息
const int SERVER_RESPONSE_TELE_INFO = 0x0012;			//会用用户请求获取电话信息
const int CLIENT_COMMAND_GET_PSW = 0x0013;			//用户请求获取密码
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

const int SERVER_COMMAND_CHANGE_LEVEL_TO_LOGIN = 0x2205;//改变等级重新请求陪桌并登陆
const int SERVER_COMMAND_KICK_OUT = 0x4004;



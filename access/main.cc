#include "objectmanger.h"
#include "log.h"
#include "ctcpevent.h"
#include "ctcpserver.h"
#include "ctcpconnection.h"

#define MAX_CONN_CNT 1024

char g_log_dir[256];

int main(int argc , char *argv[]){
	char local_ip[128];
    int local_port;
    int i;
    memset(local_ip, 0, sizeof(local_ip));
    
    if (argc < 3){
        printf("please input like this:\r\n");
        printf("%s ip=192.168.56.101 p=10001 \r\n", argv[0]);
        return -1;
    }
    
    for (i=1; i<argc; i++){
        if (strncmp(argv[i], "ip=", strlen("ip=")) == 0){
            strncpy(local_ip, &(argv[i][strlen("ip=")]), sizeof(local_ip)-1);
        }else if (strncmp(argv[i], "p=", strlen("p=")) == 0){
            local_port = atoi(&(argv[i][strlen("p=")]));
            if (local_port < 1024 || local_port > 65500){
                local_port = 10001;
            }
        }
    }
    strcpy(g_log_dir, "./data");
    CTcpevent * p_event_poll = new CTcpevent();
    if (p_event_poll == NULL || p_event_poll->init() < 0){
        printf("create event poll fail \n");
        return -1;
    }

    CTcpserver * p_tcp_server = new CTcpserver();
    if (p_tcp_server == NULL){
        printf("allocate tcp server objects fail \r\n");
        return -1;
    }
    CTcpConnection * p_tcp_conn_array = NULL;
    p_tcp_conn_array = new CTcpConnection[MAX_CONN_CNT];
    for (i=0; i<MAX_CONN_CNT; i++){
        p_tcp_conn_array[i].setTcpEvent(p_event_poll);
    }
    TcpConnectionManager::Instance()->Init(p_tcp_conn_array, MAX_CONN_CNT);
    p_tcp_server->setIp(local_ip);
    p_tcp_server->setPort(local_port);
    p_tcp_server->setEvent(p_event_poll);
    p_tcp_server->setSvrId(1000);
    p_tcp_server->setSvrType(100);
    if (p_tcp_server->init() < 0){
        printf("init tcp server fail \r\n");
        return -1;
    }
    while (1){
        p_event_poll->processEvent(1000);
    }
    return 0;
}
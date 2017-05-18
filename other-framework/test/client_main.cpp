

#include "../EventPoll.h"
#include "../timer_set.h"
#include "../TcpConnection.h"
#include "../TcpServer.h"
#include "../ObjectManager.h"

#include "ClientPktProc.h"

#define MAX_CONN_CNT    1024
#define TIMER_SET_BASE_TIMER    10

#define DUMP_BUFFER_LEN 0x100000
char g_dump_buffer[DUMP_BUFFER_LEN];

int main(int argc, char * argv[])
{
    char server_ip[128];
    int server_port = 10001;
    char local_ip[128];
    int local_port_start = 20001;
    int client_cnt = 8;
    char log_file_name[256];
    
    CClientPktProc * p_pkt_proc_array = NULL;
    CTcpConnection * p_tcp_conn_array = NULL;
    int i = 0;
    
    if (argc < 6)
    {
        printf("please input like this:\r\n");
        printf("%s server_ip=192.168.56.101 server_port=10001 local_ip=192.168.56.101 local_port_start=20001 client_cnt=8 \r\n", argv[0]);
        return -1;
    }
    
    memset(server_ip, 0, sizeof(server_ip));
    memset(local_ip, 0, sizeof(local_ip));
    
    for (i=1; i<argc; i++)
    {
        if (strncmp(argv[i], "server_ip=", strlen("server_ip=")) == 0)
        {
            strncpy(server_ip, &(argv[i][strlen("server_ip=")]), sizeof(server_ip)-1);
        }
        else if (strncmp(argv[i], "server_port=", strlen("server_port=")) == 0)
        {
            server_port = atoi(&(argv[i][strlen("server_port=")]));
            if (server_port < 1024 || server_port > 65500)
            {
                server_port = 10001;
            }
        }
        else if (strncmp(argv[i], "local_ip=", strlen("local_ip=")) == 0)
        {
            strncpy(local_ip, &(argv[i][strlen("local_ip=")]), sizeof(local_ip)-1);
        }
        else if (strncmp(argv[i], "local_port_start=", strlen("local_port_start=")) == 0)
        {
            local_port_start = atoi(&(argv[i][strlen("local_port_start=")]));
            if (local_port_start < 1024 || local_port_start > 65500)
            {
                local_port_start = 20001;
            }
        }
        else if (strncmp(argv[i], "client_cnt=", strlen("client_cnt=")) == 0)
        {
            client_cnt = atoi(&(argv[i][strlen("client_cnt=")]));
            if (client_cnt < 1 || client_cnt > MAX_CONN_CNT)
            {
                client_cnt = 8;
            }
        } 
    }
    
    sprintf(log_file_name, "test_client_%d", getpid());
    init_log(log_file_name);
	set_log_level(6);
    
    timer_set_t * p_timer_set = create_timer_set(10);
    if (p_timer_set == NULL)
    {
        printf("create timer set fail \r\n");
        return -1;
    }
    
    log_debug("create timer set ok");
    
    CEventPoll * p_event_poll = new CEventPoll;
    if (p_event_poll == NULL || p_event_poll->Init() < 0)
    {
        printf("create event poll fail \r\n");
        return -1;
    }
    
    log_debug("create and init event poll ok");
    
    p_pkt_proc_array = new CClientPktProc[MAX_CONN_CNT];
    p_tcp_conn_array = new CTcpConnection[MAX_CONN_CNT];
    
    if (p_pkt_proc_array == NULL || p_tcp_conn_array == NULL)
    {
        printf("allocate connection objects fail \r\n");
        return -1;
    }
    
    for (i=0; i<MAX_CONN_CNT; i++)
    {
        p_tcp_conn_array[i].SetPktProc(&p_pkt_proc_array[i]);
        p_tcp_conn_array[i].SetEventPoll(p_event_poll);
        p_pkt_proc_array[i].SetTcpConnObj(&p_tcp_conn_array[i]);
    }
    
    TcpConnectionManager::Instance()->Init(p_tcp_conn_array, MAX_CONN_CNT);
    
    log_debug("create and init tcp connection array ok");
    
    for (i=0; i<client_cnt; i++)
    {
        CTcpConnection * p_tcp_conn = TcpConnectionManager::Instance()->AllocateOneObject();
        if (p_tcp_conn == NULL)
        {
            log_error("no free CTcpConnection object ");
            return 0;
        }
        p_tcp_conn->SetUsingStatus(1);
        
        p_tcp_conn->ResetObject();
        p_tcp_conn->SetRemoteIp(inet_addr(server_ip));
        p_tcp_conn->SetRemotePort(htons(server_port));
        p_tcp_conn->SetLocalIp(inet_addr(local_ip));
        p_tcp_conn->SetLocalPort(htons(local_port_start+i));
        p_tcp_conn->SetEpType(EP_TYPE_CLIENT);
        p_tcp_conn->SetReconnectFlags(1);
        
        CClientPktProc * p_pkt_proc = (CClientPktProc *)p_tcp_conn->GetPktProc();
        p_pkt_proc->SetTimerSet(p_timer_set);
        
        p_tcp_conn->Init();
    }
    
    uint64_t curr_time = get_curr_time();
    uint64_t next_expiration = curr_time + TIMER_SET_BASE_TIMER;
    uint64_t last_print_time = curr_time;
    
    while (1)
    {
        p_event_poll->RunEventLoop(1);
        
        curr_time = get_curr_time();
        if (next_expiration <= curr_time)
        {
            while (next_expiration <= curr_time)
            {
                if (curr_time > 20*TIMER_SET_BASE_TIMER + next_expiration)  // NTP 同步了或管理员手工设置了系统时钟
                {
                    next_expiration = curr_time;
                }
                
                next_expiration += TIMER_SET_BASE_TIMER;
                run_timer_set(p_timer_set, curr_time);
            }
        }
        else if (next_expiration > 2*TIMER_SET_BASE_TIMER + curr_time)      // NTP 同步了或管理员手工设置了系统时钟
        {
            next_expiration = curr_time + TIMER_SET_BASE_TIMER;
        }
        
        if (curr_time - last_print_time >= 10000)
        {
            g_dump_buffer[0] = 0;
            p_event_poll->DumpActiveSockFdInfo(g_dump_buffer, DUMP_BUFFER_LEN);
            printf("active socket info list [%lu] : \r\n%s \r\n", curr_time, g_dump_buffer);
            
            last_print_time = curr_time;
        }
    }
    
    return 0;
}



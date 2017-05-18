
// gcc -Wall -Wextra -g -o udp_log_server udp_log_server.c

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <endian.h>
#include <libgen.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../timer_set.h"

#define MAX_APP_NAME_LEN        47
#define MAX_EXT_STRING_LEN      32
#define MAX_EXT_APP_NAME_LEN    (MAX_APP_NAME_LEN + MAX_EXT_STRING_LEN)

#define MAX_KEY_LEN (MAX_EXT_APP_NAME_LEN)

typedef struct _user_data
{
    int log_fd;
    int file_index;
    int write_index;
    int file_length;
    uint64_t log_seq;
    uint64_t log_cnt;
    user_timer_t * p_user_timer;
    uint64_t last_write_time;
    uint64_t last_check_time;
    char file_path_name[512];
    char app_name_ext[MAX_EXT_APP_NAME_LEN+1];
    char * log_buffer;
} user_data_t;

#include "hash_table.h"

hash_table_t g_app_name_hash_table;

typedef struct _log_pkt_header
{
    char app_name_ext[MAX_EXT_APP_NAME_LEN+1];
    uint64_t u64_ext;
    int command;
    int log_level;
    uint64_t log_seq;
    char log_data[0];
}log_pkt_header_t;

#define MAX_RECV_BUFFER_LEN  0x10000
char g_recv_buffer[MAX_RECV_BUFFER_LEN];

int g_max_file_cnt = 8;
int g_max_file_size = 0x4000000;

#define LOG_BUFFER_LEN  0x40000
#define MAX_LOG_BUFFER_CNT  (0x80000000/LOG_BUFFER_LEN)     // 每个 udp_log_server 实例最多占 2GB 内存
uint32_t g_log_buffer_cnt = 0;


static inline uint64_t hton64(uint64_t h64)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint64_t n64 = htonl((uint32_t)h64);
    n64 <<= 32;
    n64 += htonl((uint32_t)(h64>>32));
    return n64;
#else
    return h64;
#endif
}

#define ntoh64 hton64


static void daemonize(void)
{   
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);
	sigset_t sset;
	sigemptyset(&sset);
	sigaddset(&sset, SIGSEGV);
	sigaddset(&sset, SIGBUS);
	sigaddset(&sset, SIGABRT);
	sigaddset(&sset, SIGILL);
	sigprocmask(SIG_UNBLOCK, &sset, &sset);
	daemon(1, 1);
}


static inline char * get_base_file_name(char * base_name, char * name)
{
    char * curr = name;
    char * end = name;
    int len = 0;
    
    while (curr != NULL)
    {
        curr = strchr(curr, '_');
        if (curr == NULL)
        {
            return NULL;
        }
        end = curr;
        curr++;
        if (isdigit(*curr))
        {
            break;
        }
    }
    
    len = end - name;
    if (curr == NULL || len <= 0)
    {
        len = strlen(name);
    }
    
    strncpy(base_name, name, len);
    base_name[len] = 0;
    
    return base_name;
}


static void write_log_to_file(char * dir_prefix, char * app_name_ext, user_data_t * p_user_data)
{
    int write_index = 0;
    int intr_times = 0;
    int write_times = 0;
    int write_len = 0;
    int log_fd = -1;
    char base_name[256];
    
    if (dir_prefix == NULL || app_name_ext == NULL || p_user_data == NULL)
    {
        return;
    }
    
    log_fd = p_user_data->log_fd;
    
    do
    {
        write_len = write(log_fd, &(p_user_data->log_buffer[write_index]), p_user_data->write_index-write_index);
        if (write_len < 0)
        {
            if (errno == EINTR)
            {
                if (intr_times < 5)
                {
                    intr_times++;
                    continue;
                }
            }
            
            printf("write_len:%d app_name_ext:%s %m\r\n", write_len, app_name_ext);
            
            close(log_fd);
            p_user_data->write_index = 0;
            p_user_data->log_fd = -1;
            return;
        }
        else if (write_len == 0)
        {
            if (write_times < 5)
            {
                write_times++;
                continue;
            }
            
            printf("write_len:0 over 5 times, app_name_ext:%s \r\n", app_name_ext);
            
            close(log_fd);
            p_user_data->write_index = 0;
            p_user_data->log_fd = -1;
            return;
        }
        else
        {
            write_index += write_len;
        }
    } while (write_index < p_user_data->write_index);
    
    p_user_data->file_length += write_index;
    p_user_data->write_index = 0;
    
    if (p_user_data->file_length > g_max_file_size)
    {
        char new_file_path_name[512];
        char cmd_string[512];
        
        time_t curr_sec_time = time(NULL);
        struct tm tm;
        
        memset(&tm, 0, sizeof(tm));
        localtime_r(&curr_sec_time, &tm);
        
        close(log_fd);
        p_user_data->log_fd = -1;
        
        snprintf(cmd_string, sizeof(cmd_string)-1, "rm -f %s/%s/%s_%d_*",
                 dir_prefix, get_base_file_name(base_name, app_name_ext),
                 app_name_ext, p_user_data->file_index);
        
        system(cmd_string);
        
        snprintf(new_file_path_name, sizeof(new_file_path_name)-1, "%s/%s/%s_%d_%4d-%02d-%02d_%02d-%02d-%02d.log",
                 dir_prefix, get_base_file_name(base_name, app_name_ext), app_name_ext,
                 p_user_data->file_index, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                 tm.tm_hour, tm.tm_min, tm.tm_sec);
        
        rename(p_user_data->file_path_name, new_file_path_name);
        
        p_user_data->file_index++;
        if (p_user_data->file_index > g_max_file_cnt)
        {
            p_user_data->file_index = 1;
        }
        
        p_user_data->file_length = 0;
    }
}

#define TIMER_SET_BASE_TIMER    10
char g_dir_prefix[256];

int write_log_timer_callback(void * pv_user_timer)
{
    user_timer_t * p_user_timer = (user_timer_t *)pv_user_timer;
    user_data_t * p_user_data = (user_data_t *)p_user_timer->pv_param1;
    
    if (p_user_data->log_fd >= 0 && p_user_data->write_index > 0 && p_user_data->log_buffer != NULL)
    {
        write_log_to_file(g_dir_prefix, p_user_data->app_name_ext, p_user_data);
    }
    
    return 0;
}

int main(int argc, char * argv[])
{
    int sock_fd = -1;
    struct sockaddr_in local_address;
    struct sockaddr_in peer_address;
    socklen_t address_len = sizeof(struct sockaddr_in);
    int recv_len = 0;
    char local_ip_string[16];
    int local_port = 20000;
    int i = 0;
    char dir_prefix[256];
    char cmd_string[512];
    char base_name[256];
    int kernel_buffer_size = 0;
    struct timeval tv;
    user_data_t user_data;
    user_data_t * p_user_data = NULL;
    unsigned long curr_time = 0;
    uint64_t next_expiration = 0UL;
    timer_set_t * p_timer_set = NULL;
    int clients_cnt = 0;
    
    if (argc < 6)
    {
        printf("please input like this : \r\n");
        printf("    %s local_ip=0.0.0.0 local_port=6900 dir_prefix=/var/log/log_udp max_file_size=67000000 max_file_cnt=8 \r\n", argv[0]);
        return -1;
    }
    
    for (i=1; i<argc; i++)
    {
        if (strncmp(argv[i], "local_ip=", strlen("local_ip=")) == 0)
        {
            strncpy(local_ip_string, &(argv[i][strlen("local_ip=")]), sizeof(local_ip_string)-1);
            local_ip_string[strlen(local_ip_string)]=0;
        }
        else if (strncmp(argv[i], "local_port=", strlen("local_port=")) == 0)
        {
            local_port = atoi(&(argv[i][strlen("local_port=")]));
            if (local_port < 1024 || local_port > 65500)
            {
                local_port = 20000;
            }
        }
        else if (strncmp(argv[i], "dir_prefix=", strlen("dir_prefix=")) == 0)
        {
            strncpy(dir_prefix, &(argv[i][strlen("dir_prefix=")]), sizeof(dir_prefix)-1);
        }
        else if (strncmp(argv[i], "max_file_cnt=", strlen("max_file_cnt=")) == 0)
        {
            g_max_file_cnt = atoi(&(argv[i][strlen("max_file_cnt=")]));
            if (g_max_file_cnt < 3)
            {
                g_max_file_cnt = 3;
            }
            if (g_max_file_cnt > 1024)
            {
                g_max_file_cnt = 1024;
            }
        }
        else if (strncmp(argv[i], "max_file_size=", strlen("max_file_size=")) == 0)
        {
            g_max_file_size = atoi(&(argv[i][strlen("max_file_size=")]));
            if (g_max_file_size < 0x100000)
            {
                g_max_file_size = 0x100000;
            }
            if (g_max_file_size > 0x8000000)
            {
                g_max_file_size = 0x8000000;
            }
        }
    }
    
    printf("\r\n  local_ip=%s local_port=%d dir_prefix=%s max_file_size=%d max_file_cnt=%d \r\n",
           local_ip_string, local_port, dir_prefix, g_max_file_size, g_max_file_cnt);
    
    strcpy(g_dir_prefix, dir_prefix);
    
    daemonize();
    
    init_hash_table(&g_app_name_hash_table);
    
    p_timer_set = create_timer_set(TIMER_SET_BASE_TIMER);
    if (p_timer_set == NULL)
    {
        printf("create timer set fail : %m");
        exit(-1);
    }
    
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0)
    {
        printf("create socket fail : %m");
        exit(-1);
    }
    
    memset(&local_address, 0, sizeof(struct sockaddr_in));
    local_address.sin_family = AF_INET;
    local_address.sin_port = htons(local_port);
    local_address.sin_addr.s_addr = inet_addr(local_ip_string);
    
    if (bind(sock_fd, (struct sockaddr *)&local_address, address_len) < 0)
    {
        printf("bind socket to local address fail : %m");
        exit(-1);
    }
    
    kernel_buffer_size = 0x1000000;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &kernel_buffer_size, sizeof(kernel_buffer_size)) != 0)
    {
        perror("set sync SOL_SOCKET:SO_RCVBUF fail ");
        exit(-1);
    }
    
    // 设置接收 2 毫秒超时
    tv.tv_sec = 0;
    tv.tv_usec = 2000;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0)
    {
        perror("set sync SOL_SOCKET:SO_RCVBUF fail ");
        exit(-1);
    }
    
    sprintf(cmd_string, "mkdir -p %s", dir_prefix);
    system(cmd_string);
    
    next_expiration = get_curr_time() + TIMER_SET_BASE_TIMER;
    while (1)
    {
        p_user_data = NULL;
        recv_len = recvfrom(sock_fd, g_recv_buffer, sizeof(g_recv_buffer)-1, 0, (struct sockaddr *)&peer_address, &address_len);
        curr_time = get_curr_time();
        if (recv_len > (int)sizeof(log_pkt_header_t))
        {
            log_pkt_header_t * p_log_pkt_header = (log_pkt_header_t *)g_recv_buffer;
            int log_fd = -1;
            uint64_t log_seq = 0UL;
            int send_len = 0;
            
            g_recv_buffer[recv_len] = 0;
            p_log_pkt_header->app_name_ext[MAX_EXT_APP_NAME_LEN] = 0;
            
            log_seq = ntoh64(p_log_pkt_header->log_seq);
            
            p_user_data = get_from_hash_table(&g_app_name_hash_table, p_log_pkt_header->app_name_ext,
                                              strlen(p_log_pkt_header->app_name_ext));
            if (p_user_data != NULL)
            {
                log_fd = p_user_data->log_fd;
                
                if (log_seq != p_user_data->log_seq)
                {
                    printf("log_file:%s recv log_seq:%lu expect log_seq:%lu \r\n",
                            p_log_pkt_header->app_name_ext, log_seq, p_user_data->log_seq);
                }
                p_user_data->log_seq = log_seq + 1;
            }
            else
            {
                user_data.write_index = 0;
                user_data.file_index = 1;
                user_data.file_length = 0;
                user_data.log_seq = log_seq + 1;
                user_data.log_cnt = 0UL;
                user_data.last_write_time = curr_time;
                user_data.last_check_time = curr_time;
                strcpy(user_data.app_name_ext, p_log_pkt_header->app_name_ext);
                
                sprintf(user_data.file_path_name, "%s/%s/%s.log", dir_prefix,
                        get_base_file_name(base_name, p_log_pkt_header->app_name_ext),
                        p_log_pkt_header->app_name_ext);
                
                user_data.log_buffer = NULL;
                p_user_data = &user_data;
            }
            
            if (log_fd < 0)
            {
label_open_log_file:
                sprintf(cmd_string, "mkdir -p %s/%s", dir_prefix,
                        get_base_file_name(base_name, p_log_pkt_header->app_name_ext));
                
                system(cmd_string);
                
                log_fd = open(p_user_data->file_path_name, O_CREAT|O_WRONLY|O_APPEND, 0644);
                if (log_fd < 0)
                {
                    printf("open file %s fail : %m \r\n", p_user_data->file_path_name);
                    goto label_run_timer_set;
                }
                
                p_user_data->log_fd = log_fd;
                p_user_data->last_check_time = curr_time;
                
                if (p_user_data == &user_data)
                {
                    user_timer_t user_timer;
                    
                    p_user_data->last_write_time = curr_time;
                    p_user_data = insert_to_hash_table(&g_app_name_hash_table, p_log_pkt_header->app_name_ext,
                                                       strlen(p_log_pkt_header->app_name_ext), p_user_data);
                    if (p_user_data == NULL)
                    {
                        goto label_run_timer_set;
                    }
                    
                    init_user_timer(user_timer, 0xFFFFFFFF, 3000, write_log_timer_callback, p_user_data, NULL, NULL, NULL, NULL);
                    p_user_data->p_user_timer = create_one_timer(p_timer_set, &user_timer);
                    if (p_user_data->p_user_timer == NULL)
                    {
                        printf("create timer fail, log_fd:%d app_name_ext:%s \r\n",
                                p_user_data->log_fd, p_log_pkt_header->app_name_ext);
                    }
                    
                    clients_cnt++;
                    if (clients_cnt > 10000)
                    {
                        printf("too many clients, please check your applications and restart the log server, "
                               "log_fd:%d app_name_ext:%s local_ip=%s local_port=%d "
                               "dir_prefix=%s max_file_size=%d max_file_cnt=%d \r\n",
                                p_user_data->log_fd, p_log_pkt_header->app_name_ext, local_ip_string, local_port,
                                dir_prefix, g_max_file_size, g_max_file_cnt);
                    }
                }
            }
            else
            {
                if (curr_time - p_user_data->last_check_time >= 3000)    // 每 3 秒检查一次文件是否存在, 不存在则重新创建
                {
                    p_user_data->last_check_time = curr_time;
                    if (access(p_user_data->file_path_name, R_OK|W_OK) != 0)
                    {
                        close(log_fd);
                        log_fd = -1;
                        p_user_data->log_fd = -1;
                        goto label_open_log_file;
                    }
                }
            }
            
            send_len = recv_len-sizeof(log_pkt_header_t);
            
            if (p_user_data->log_buffer == NULL)
            {
                if (g_log_buffer_cnt >= MAX_LOG_BUFFER_CNT)
                {
                    printf("g_log_buffer_cnt:%d \r\n", g_log_buffer_cnt);
                    goto label_run_timer_set;
                }
                
                p_user_data->log_buffer = malloc(LOG_BUFFER_LEN);
                if (p_user_data->log_buffer == NULL)
                {
                    printf("allocate log buffer fail \r\n");
                    goto label_run_timer_set;
                }
                
                printf("log_fd:%d app_name_ext:%s log_buffer:%p \r\n",
                        log_fd, p_log_pkt_header->app_name_ext, p_user_data->log_buffer);
                        
                p_user_data->write_index = 0;
                g_log_buffer_cnt++;
            }
                
            if (p_user_data->write_index + send_len < LOG_BUFFER_LEN)
            {
                memcpy(&(p_user_data->log_buffer[p_user_data->write_index]), p_log_pkt_header->log_data, send_len);
                p_user_data->write_index += send_len;
            }
            else
            {
                write_log_to_file(dir_prefix, p_log_pkt_header->app_name_ext, p_user_data);
                memcpy(p_user_data->log_buffer, p_log_pkt_header->log_data, send_len);
                p_user_data->write_index = send_len;
                p_user_data->last_write_time = curr_time;
            }
            
            p_user_data->log_cnt++;
        }
        
label_run_timer_set:

        if (next_expiration > curr_time + 200 || next_expiration < curr_time - 200)   // ntp 同步了系统时间
        {   // 修正下一次执行定时器集合的时间
            next_expiration = curr_time + TIMER_SET_BASE_TIMER;
            run_timer_set(p_timer_set);
        }
        while (next_expiration <= curr_time)
        {
            next_expiration += TIMER_SET_BASE_TIMER;
            run_timer_set(p_timer_set);
        }
    }
    
    return 0;
}



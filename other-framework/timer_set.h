 
/**
 * timer_set.h
**/

#ifndef TIMER_SET_H
#define TIMER_SET_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*timer_call_back_t)(void * pv_user_timer);

typedef struct user_timer
{
    int i_timer_id;
    uint32_t u32_loop_cnt;
    uint64_t u64_hold_time;
    timer_call_back_t timer_call_back;
    void * pv_param1;
    void * pv_param2;
    void * pv_param3;
    void * pv_param4;
    void * pv_param5;
}user_timer_t;


#define MAX_TIMER_NODE_CNT   100000

struct timer_set;

typedef struct timer_set timer_set_t;


timer_set_t * create_timer_set(uint16_t u16_base_timer_ms);

// void run_timer_set(timer_set_t * p_timer_set, uint64_t u64_curr_time);
void run_timer_set(timer_set_t * p_timer_set);

void destroy_timer_set(timer_set_t * p_timer_set);

#define init_user_timer(user_timer, loop_cnt, hold_time, call_back, param1, param2, param3, param4, param5) \
            do {                                                                                            \
                user_timer.u32_loop_cnt = (loop_cnt);                                                       \
                user_timer.u64_hold_time = (hold_time);                                                     \
                user_timer.timer_call_back = (call_back);                                                   \
                user_timer.pv_param1 = (param1);                                                            \
                user_timer.pv_param2 = (param2);                                                            \
                user_timer.pv_param3 = (param3);                                                            \
                user_timer.pv_param4 = (param4);                                                            \
                user_timer.pv_param5 = (param5);                                                            \
            } while (0)

user_timer_t * create_one_timer(timer_set_t * p_timer_set, user_timer_t * p_user_timer);

int destroy_one_timer(timer_set_t * p_timer_set, int i_timer_id);

user_timer_t * reset_one_timer(timer_set_t * p_timer_set, user_timer_t * p_user_timer);


user_timer_t * get_user_timer_by_id(timer_set_t * p_timer_set, uint32_t u32_timer_id);


static inline unsigned long get_curr_time(void)
{
    struct timeval tv;
    unsigned long curr_time = 0;
    
    gettimeofday(&tv, NULL);
    
    curr_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    
    return curr_time;
}

#define MAX_CURR_TIME_STRING_LEN  27  // strlen("2015-10-23 09:15:59.737940") + 1

static inline char * get_curr_time_string(char * time_string_buffer, int buffer_len)
{
    struct timeval tv;
    struct tm tm;
    
    if (buffer_len < MAX_CURR_TIME_STRING_LEN)
    {
        return NULL;
    }
    
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm);
    
    tm.tm_year += 1900;
    tm.tm_mon += 1;
    
    snprintf(time_string_buffer, buffer_len, "%4d-%02d-%02d %02d:%02d:%02d.%06d",
             tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, (uint32_t)(tv.tv_usec));
    
    return time_string_buffer;
}
    
#ifdef __cplusplus
}
#endif
    
#endif  // TIMER_SET_H


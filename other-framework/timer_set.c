 
/**
 * timer_set.cpp
**/

#include "timer_set.h"

#define MAX_TIMER_SET_MAGIC_LEN     31  // 2^5 - 1
#define TIMER_SET_MAGIC_STRING      "timer_set_magic"

typedef struct _timer_list_head
{
    uint32_t u32_head;
    uint32_t u32_tail;
    uint32_t u32_cnt;
}timer_list_head_t;

typedef struct _timer_node
{
    uint32_t u32_flags;
    uint64_t u64_hold_jiffies;
    uint64_t u64_expiration_jiffies;
    user_timer_t user_timer;
    timer_list_head_t * p_timer_list;
    uint32_t u32_next_work_index;
    uint32_t u32_prev_work_index;
    uint32_t u32_next_free_index;
}timer_node_t;

struct timer_set
{
    char pch_timer_set_magic[MAX_TIMER_SET_MAGIC_LEN+1];
    
    uint32_t u32_base_timer_ms;
    uint64_t u64_current_jiffies;
    uint32_t curr_list_index[8];
    timer_list_head_t timer_lists[8][256];
    
    uint32_t u32_first_free_timer;
    uint32_t u32_last_free_timer;
    uint32_t u32_free_timer_cnt;
    timer_node_t timer_node_array[MAX_TIMER_NODE_CNT];
};

static inline uint32_t get_timer_index(timer_set_t * p_timer_set, timer_node_t * p_timer_node)
{
    if (p_timer_set == NULL || p_timer_node == NULL || (void *)p_timer_node < (void *)(p_timer_set->timer_node_array))
    {
        return MAX_TIMER_NODE_CNT;
    }
    
    return (uint32_t)(p_timer_node - (timer_node_t *)(p_timer_set->timer_node_array));
}

static inline timer_node_t * get_timer_node_by_index(timer_set_t * p_timer_set, uint32_t u32_timer_index)
{
    if (u32_timer_index >= MAX_TIMER_NODE_CNT)
    {
        return NULL;
    }
    
    return &(p_timer_set->timer_node_array[u32_timer_index]);
}

user_timer_t * get_user_timer_by_id(timer_set_t * p_timer_set, uint32_t u32_timer_id)
{
    if (u32_timer_id >= MAX_TIMER_NODE_CNT)
    {
        return NULL;
    }
    
    return &(p_timer_set->timer_node_array[u32_timer_id].user_timer);
}

static inline user_timer_t * clear_user_timer(user_timer_t * p_user_timer)
{
    if (p_user_timer == NULL)
    {
        return NULL;
    }
    
    p_user_timer->i_timer_id = -1;
    p_user_timer->u32_loop_cnt = 0;
    p_user_timer->u64_hold_time = 0;
    p_user_timer->timer_call_back = NULL;
    p_user_timer->pv_param1 = NULL;
    p_user_timer->pv_param2 = NULL;
    p_user_timer->pv_param3 = NULL;
    p_user_timer->pv_param4 = NULL;
    p_user_timer->pv_param5 = NULL;
    
    return p_user_timer;
}

static inline timer_node_t * clear_timer_node(timer_node_t * p_timer_node)
{
    if (p_timer_node == NULL)
    {
        return NULL;
    }
    
    p_timer_node->u32_flags = 0;
    p_timer_node->u64_hold_jiffies = 0;
    p_timer_node->u64_expiration_jiffies = 0;
    clear_user_timer(&(p_timer_node->user_timer));
    p_timer_node->p_timer_list = NULL;
    p_timer_node->u32_next_work_index = MAX_TIMER_NODE_CNT;
    p_timer_node->u32_prev_work_index = MAX_TIMER_NODE_CNT;
    p_timer_node->u32_next_free_index = MAX_TIMER_NODE_CNT;
    
    return p_timer_node;
}

static inline timer_list_head_t * init_timer_list_head(timer_list_head_t * p_timer_list_head)
{
    if (p_timer_list_head == NULL)
    {
        return NULL;
    }

    p_timer_list_head->u32_head = MAX_TIMER_NODE_CNT;
    p_timer_list_head->u32_tail = MAX_TIMER_NODE_CNT;
    p_timer_list_head->u32_cnt = 0;

    return p_timer_list_head;
}

static timer_set_t * init_timer_set(timer_set_t * p_timer_set, uint16_t u16_base_timer_ms)
{
    int i = 0, j = 0;
    
    memset(p_timer_set, 0, sizeof(timer_set_t));
    
    p_timer_set->u64_current_jiffies = 0;
    p_timer_set->u32_base_timer_ms = u16_base_timer_ms;
    
    for (i=0; i<8; i++)
    {
        for (j=0; j<256; j++)
        {
            init_timer_list_head(&(p_timer_set->timer_lists[i][j]));
        }
    }

    for (i=0; i<MAX_TIMER_NODE_CNT; i++)
    {
        clear_timer_node(&p_timer_set->timer_node_array[i]);
        p_timer_set->timer_node_array[i].u32_next_free_index = i + 1;
    }
    p_timer_set->u32_first_free_timer = 0;
    p_timer_set->u32_last_free_timer = MAX_TIMER_NODE_CNT - 1;
    p_timer_set->u32_free_timer_cnt = MAX_TIMER_NODE_CNT;
    
    strcpy(p_timer_set->pch_timer_set_magic, TIMER_SET_MAGIC_STRING);
    
    return p_timer_set;
}

timer_set_t * create_timer_set(uint16_t u16_base_timer_ms)
{
    timer_set_t * p_timer_set = NULL;
    
    if (u16_base_timer_ms < 10 || u16_base_timer_ms > 1000)
    {
        printf("%s --> %s --> L%d : u16_base_timer_ms:%u \r\n", __FILE__, __FUNCTION__, __LINE__, u16_base_timer_ms);
        return NULL;
    }
    
    p_timer_set = (timer_set_t *)calloc(1, sizeof(timer_set_t));
    if (p_timer_set == NULL)
    {
        printf("%s --> %s --> L%d : create timer set fail \r\n", __FILE__, __FUNCTION__, __LINE__);
        return NULL;
    }
    
    return init_timer_set(p_timer_set, u16_base_timer_ms);
}

void destroy_timer_set(timer_set_t * p_timer_set)
{
    if (p_timer_set != NULL)
    {
        free(p_timer_set);
    }
}

static inline timer_node_t * alloc_timer_node(timer_set_t * p_timer_set)
{
    timer_node_t * p_timer_node = NULL;

    if (p_timer_set == NULL)
    {
        return NULL;
    }

    if (p_timer_set->u32_free_timer_cnt == 0 || p_timer_set->u32_first_free_timer >= MAX_TIMER_NODE_CNT)
    {
        return 0;
    }

    p_timer_node = &p_timer_set->timer_node_array[p_timer_set->u32_first_free_timer];
    p_timer_set->u32_first_free_timer = p_timer_node->u32_next_free_index;
    p_timer_set->u32_free_timer_cnt--;
    
    if (p_timer_set->u32_free_timer_cnt == 0)
    {
        p_timer_set->u32_first_free_timer = MAX_TIMER_NODE_CNT;
        p_timer_set->u32_last_free_timer = MAX_TIMER_NODE_CNT;
    }

    clear_timer_node(p_timer_node);
    p_timer_node->u32_flags = 1;
    
    return p_timer_node;
}

static inline int free_timer_node(timer_set_t * p_timer_set, timer_node_t * p_timer_node)
{
    uint32_t u32_timer_index = get_timer_index(p_timer_set, p_timer_node);

    if (u32_timer_index >= MAX_TIMER_NODE_CNT)
    {
        return -1;
    }

    if (p_timer_node->u32_flags == 0)
    {
        return 0;
    }

    clear_timer_node(p_timer_node);
    
    p_timer_node->u32_next_free_index = p_timer_set->u32_first_free_timer;
    p_timer_set->u32_first_free_timer = u32_timer_index;
    if (p_timer_set->u32_last_free_timer >= MAX_TIMER_NODE_CNT)
    {
        p_timer_set->u32_last_free_timer = u32_timer_index;
    }

    p_timer_set->u32_free_timer_cnt++;

    return 0;
}

static inline int get_timer_level(timer_set_t * p_timer_set, uint64_t u64_expiration_jiffies)
{
    uint64_t u64_remain_jiffies = 0UL;
    
    if (u64_expiration_jiffies > p_timer_set->u64_current_jiffies)
    {
        u64_remain_jiffies = u64_expiration_jiffies - p_timer_set->u64_current_jiffies;
    }
    
    // 每一级的当前链都是下一次执行 run_timer_set() 时才有可能处理
    
    if (u64_remain_jiffies <= 0x100UL)
    {
        return 0;
    }
    else if (u64_remain_jiffies <= 0x10000UL)
    {
        return 1;
    }
    else if (u64_remain_jiffies <= 0x1000000UL)
    {
        return 2;
    }
    else if (u64_remain_jiffies <= 0x100000000UL)
    {
        return 3;
    }
    else if (u64_remain_jiffies <= 0x10000000000UL)
    {
        return 4;
    }
    else if (u64_remain_jiffies <= 0x1000000000000UL)
    {
        return 5;
    }
    else if (u64_remain_jiffies <= 0x100000000000000UL)
    {
        return 6;
    }
    else if (u64_remain_jiffies <= 0xFFFFFFFFFFFFFFFFUL)
    {
        return 7;
    }
    else
    {
        return 8;
    }
}

static inline int get_timer_list_index(timer_set_t * p_timer_set, int i_level, uint64_t u64_expiration_jiffies)
{
    uint64_t u64_remain_jiffies = 0UL;
    int i_list_offset = 0;
    
    if (u64_expiration_jiffies > p_timer_set->u64_current_jiffies)
    {
        u64_remain_jiffies = u64_expiration_jiffies - p_timer_set->u64_current_jiffies;
    }
    
    i_list_offset = ((u64_remain_jiffies >> (i_level * 8)) & 0x000001FF) - 1;
    if (i_list_offset < 0)
    {
        i_list_offset = 0;
    }
    
    // 每一级的当前链都是下一次执行 run_timer_set() 时才有可能处理
    
    return ((p_timer_set->curr_list_index[i_level] + i_list_offset) & 0x000000FF);
}

static inline timer_list_head_t * get_timer_list_head(timer_set_t * p_timer_set, timer_node_t * p_timer_node)
{
    int i_level = 0;
    int i_list_index = 0;
    timer_list_head_t * p_list_head = NULL;
    
    // 每一级的当前链都是下一次执行 run_timer_set() 时才有可能处理
    
    i_level = get_timer_level(p_timer_set, p_timer_node->u64_expiration_jiffies);
    if (i_level >= 8)
    {
        printf("%s --> %s --> L%d : no level %d \r\n", __FILE__, __FUNCTION__, __LINE__, i_level);
        return NULL;
    }
    
    i_list_index = get_timer_list_index(p_timer_set, i_level, p_timer_node->u64_expiration_jiffies);
    p_list_head = &(p_timer_set->timer_lists[i_level][i_list_index]);
    
    return p_list_head;
}

static inline int add_timer_node_to_list(timer_set_t * p_timer_set, timer_list_head_t * p_list_head, timer_node_t * p_timer_node)
{
    uint32_t u32_timer_index = get_timer_index(p_timer_set, p_timer_node);
    
    if (u32_timer_index >= MAX_TIMER_NODE_CNT)
    {
        printf("%s --> %s --> L%d : u32_timer_index:%u >= MAX_TIMER_NODE_CNT:%u \r\n",
                __FILE__, __FUNCTION__, __LINE__, u32_timer_index, MAX_TIMER_NODE_CNT);
        return -1;
    }
    
    p_timer_node->p_timer_list = p_list_head;
    
    // printf("%s --> %s --> L%d : p_list_head->u32_head:%d p_list_head->u32_tail:%d p_list_head->u32_cnt:%d u32_timer_index:%d \r\n",
    //         __FILE__, __FUNCTION__, __LINE__, p_list_head->u32_head, p_list_head->u32_tail, p_list_head->u32_cnt, u32_timer_index);
                
    if (p_list_head->u32_tail < MAX_TIMER_NODE_CNT)
    {
        p_timer_set->timer_node_array[p_list_head->u32_tail].u32_next_work_index = u32_timer_index;
        p_timer_node->u32_prev_work_index = p_list_head->u32_tail;
    }
    
    p_list_head->u32_tail = u32_timer_index;
    
    if (p_list_head->u32_head >= MAX_TIMER_NODE_CNT)
    {
        p_list_head->u32_head = u32_timer_index;
    }
    
    p_list_head->u32_cnt++;
    
    // printf("%s --> %s --> L%d : p_list_head->u32_head:%d p_list_head->u32_tail:%d p_list_head->u32_cnt:%d u32_timer_index:%d \r\n",
    //         __FILE__, __FUNCTION__, __LINE__, p_list_head->u32_head, p_list_head->u32_tail, p_list_head->u32_cnt, u32_timer_index);
            
    return (int)u32_timer_index;
}

static inline int insert_timer_node(timer_set_t * p_timer_set, timer_node_t * p_timer_node)
{
    timer_list_head_t * p_list_head = NULL;
    
    p_list_head = get_timer_list_head(p_timer_set, p_timer_node);
    if (p_list_head == NULL)
    {
        printf("%s --> %s --> L%d : get timer list head fail \r\n", __FILE__, __FUNCTION__, __LINE__);
        return -1;
    }
    
    return add_timer_node_to_list(p_timer_set, p_list_head, p_timer_node);
}

user_timer_t * create_one_timer(timer_set_t * p_timer_set, user_timer_t * p_user_timer)
{
    timer_node_t * p_timer_node = NULL;
    
    if (p_timer_set == NULL || p_user_timer == NULL || p_user_timer->timer_call_back == NULL)
    {
        printf("%s --> %s --> L%d : input params illegal \r\n", __FILE__, __FUNCTION__, __LINE__);
        return NULL;
    }
    
    if (strcmp(p_timer_set->pch_timer_set_magic, TIMER_SET_MAGIC_STRING) != 0)
    {
        printf("%s --> %s --> L%d : timer magic fail \r\n", __FILE__, __FUNCTION__, __LINE__);
        return NULL;
    }
    
    p_timer_node = alloc_timer_node(p_timer_set);
    if (p_timer_node == NULL)
    {
        printf("%s --> %s --> L%d : alloc_timer_node() fail \r\n", __FILE__, __FUNCTION__, __LINE__);
        return NULL;
    }
    
    p_user_timer->i_timer_id = get_timer_index(p_timer_set, p_timer_node);
    p_timer_node->user_timer = *p_user_timer;
    p_timer_node->u64_hold_jiffies = p_user_timer->u64_hold_time / p_timer_set->u32_base_timer_ms;
    p_timer_node->u64_expiration_jiffies = p_timer_node->u64_hold_jiffies + p_timer_set->u64_current_jiffies;
    
    // printf("%s --> %s --> L%d : p_timer_node->user_timer.i_timer_id = %d \r\n", __FILE__, __FUNCTION__, __LINE__, p_timer_node->user_timer.i_timer_id);
    
    if (insert_timer_node(p_timer_set, p_timer_node) < 0)
    {
        printf("%s --> %s --> L%d : insert_timer_node() fail, timer_id = %d \r\n", __FILE__, __FUNCTION__, __LINE__, 
               p_timer_node->user_timer.i_timer_id);
        free_timer_node(p_timer_set, p_timer_node);
        return NULL;
    }
    
    return &p_timer_node->user_timer;
}

static timer_node_t * delete_one_timer_from_list(timer_set_t * p_timer_set, timer_node_t * p_timer_node)
{
    uint32_t u32_timer_index = get_timer_index(p_timer_set, p_timer_node);
    timer_list_head_t * p_list_head = NULL;
    
    if (u32_timer_index >= MAX_TIMER_NODE_CNT || p_timer_node->p_timer_list == NULL)
    {
        char curr_time_string[MAX_CURR_TIME_STRING_LEN];
        
        printf("%s --> %s --> L%d : u32_timer_index:%u %s \r\n", __FILE__, __FUNCTION__, __LINE__, u32_timer_index,
               get_curr_time_string(curr_time_string, MAX_CURR_TIME_STRING_LEN));
        
        return NULL;
    }
    
    p_list_head = p_timer_node->p_timer_list;
    
    if (p_list_head->u32_head == u32_timer_index && p_list_head->u32_tail == u32_timer_index)
    {
        p_list_head->u32_head = MAX_TIMER_NODE_CNT;
        p_list_head->u32_tail = MAX_TIMER_NODE_CNT;
    }
    else if (p_list_head->u32_head == u32_timer_index)
    {
        p_list_head->u32_head = p_timer_node->u32_next_work_index;
        p_timer_set->timer_node_array[p_list_head->u32_head].u32_prev_work_index = MAX_TIMER_NODE_CNT;
    }
    else if (p_list_head->u32_tail == u32_timer_index)
    {
        p_list_head->u32_tail = p_timer_node->u32_prev_work_index;
        p_timer_set->timer_node_array[p_list_head->u32_tail].u32_next_work_index = MAX_TIMER_NODE_CNT;
    }
    else
    {
        uint32_t u32_prev_index = p_timer_node->u32_prev_work_index;
        uint32_t u32_next_index = p_timer_node->u32_next_work_index;
        
        p_timer_set->timer_node_array[u32_prev_index].u32_next_work_index = u32_next_index;
        p_timer_set->timer_node_array[u32_next_index].u32_prev_work_index = u32_prev_index;
    }
    
    p_list_head->u32_cnt--;
    
    p_timer_node->u32_next_work_index = MAX_TIMER_NODE_CNT;
    p_timer_node->u32_prev_work_index = MAX_TIMER_NODE_CNT;
    
    p_timer_node->p_timer_list = NULL;
    
    return p_timer_node;
}

static inline timer_node_t * delete_first_timer_from_list(timer_set_t * p_timer_set, timer_list_head_t * p_list_head)
{
    if (p_list_head->u32_cnt == 0 || p_list_head->u32_head >= MAX_TIMER_NODE_CNT)
    {
        printf("%s --> %s --> L%d p_list_head->u32_cnt == %u  p_list_head->u32_head == %u \r\n", __FILE__, __FUNCTION__, __LINE__, p_list_head->u32_cnt, p_list_head->u32_head);
        return NULL;
    }
    
    return delete_one_timer_from_list(p_timer_set, &(p_timer_set->timer_node_array[p_list_head->u32_head]));
}

int destroy_one_timer(timer_set_t * p_timer_set, int i_timer_id)
{
    timer_node_t * p_timer_node = NULL;
    
    if (p_timer_set == NULL || i_timer_id < 0 || i_timer_id >= MAX_TIMER_NODE_CNT)
    {
        return -1;
    }
    
    p_timer_node = get_timer_node_by_index(p_timer_set, i_timer_id);
    if (p_timer_node == NULL)
    {
        char curr_time_string[MAX_CURR_TIME_STRING_LEN];
        
        printf("%s --> %s --> L%d : not found timer, i_timer_id:%d %s \r\n", __FILE__, __FUNCTION__, __LINE__,
               i_timer_id, get_curr_time_string(curr_time_string, MAX_CURR_TIME_STRING_LEN));
               
        return -3;
    }
    
    if (p_timer_node->u32_flags == 0)
    {
        char curr_time_string[MAX_CURR_TIME_STRING_LEN];
        
        printf("%s --> %s --> L%d : u32_flags:0 %s \r\n", __FILE__, __FUNCTION__, __LINE__,
               get_curr_time_string(curr_time_string, MAX_CURR_TIME_STRING_LEN));
               
        return 0;
    }
    
    if (delete_one_timer_from_list(p_timer_set, p_timer_node) == NULL)
    {
        char curr_time_string[MAX_CURR_TIME_STRING_LEN];
        
        printf("%s --> %s --> L%d : delete_one_timer_from_list() fail %s \r\n", __FILE__, __FUNCTION__, __LINE__,
               get_curr_time_string(curr_time_string, MAX_CURR_TIME_STRING_LEN));
               
        return -4;
    }
    
    return free_timer_node(p_timer_set, p_timer_node);
}

user_timer_t * reset_one_timer(timer_set_t * p_timer_set, user_timer_t * p_user_timer)
{
    timer_node_t * p_timer_node = NULL;
    int i_timer_id = -1;
    
    if (p_timer_set == NULL || p_user_timer == NULL || p_user_timer->timer_call_back == NULL)
    {
        return NULL;
    }
    
    if (p_user_timer->i_timer_id < 0 || p_user_timer->i_timer_id >= MAX_TIMER_NODE_CNT)
    {
        char curr_time_string[MAX_CURR_TIME_STRING_LEN];
        
        printf("%s --> %s --> L%d : i_timer_id:%d %s \r\n", __FILE__, __FUNCTION__, __LINE__,
               p_user_timer->i_timer_id, get_curr_time_string(curr_time_string, MAX_CURR_TIME_STRING_LEN));
               
        return NULL;
    }
    
    i_timer_id = p_user_timer->i_timer_id;
    
    p_timer_node = get_timer_node_by_index(p_timer_set, i_timer_id);
    if (p_timer_node == NULL || p_timer_node->user_timer.i_timer_id != i_timer_id)
    {
        return NULL;
    }
    
    if (delete_one_timer_from_list(p_timer_set, p_timer_node) == NULL)
    {
        char curr_time_string[MAX_CURR_TIME_STRING_LEN];
        
        printf("%s --> %s --> L%d : delete_one_timer_from_list() fail %s \r\n", __FILE__, __FUNCTION__, __LINE__,
               get_curr_time_string(curr_time_string, MAX_CURR_TIME_STRING_LEN));
               
        return NULL;
    }
    
    p_user_timer->i_timer_id = get_timer_index(p_timer_set, p_timer_node);
    p_timer_node->user_timer = *p_user_timer;
    p_timer_node->u64_hold_jiffies = p_user_timer->u64_hold_time / p_timer_set->u32_base_timer_ms;
    p_timer_node->u64_expiration_jiffies = p_timer_node->u64_hold_jiffies + p_timer_set->u64_current_jiffies;
    
    if (insert_timer_node(p_timer_set, p_timer_node) < 0)
    {
        char curr_time_string[MAX_CURR_TIME_STRING_LEN];
        
        printf("%s --> %s --> L%d : insert_timer_node() fail, timer_id = %d %s \r\n",
               __FILE__, __FUNCTION__, __LINE__, p_timer_node->user_timer.i_timer_id,
               get_curr_time_string(curr_time_string, MAX_CURR_TIME_STRING_LEN));
        
        free_timer_node(p_timer_set, p_timer_node);
        return NULL;
    }
    
    return &p_timer_node->user_timer;
}

static inline int run_one_timer(timer_set_t * p_timer_set, timer_node_t * p_timer_node)
{
    if (p_timer_set == NULL || p_timer_node == NULL)
    {
        printf("%s --> %s --> L%d : p_timer_set == NULL || p_timer_node == NULL \r\n", __FILE__, __FUNCTION__, __LINE__);
        return -1;
    }
    
    return p_timer_node->user_timer.timer_call_back(&(p_timer_node->user_timer));
}

static int move_timer_to_low_level(timer_set_t * p_timer_set, timer_list_head_t * p_list_head)
{
    timer_node_t * p_timer_node = NULL;
    int loop_cnt = 0;
    int i_ret = 0;
    
    loop_cnt = p_list_head->u32_cnt;
    if (loop_cnt == 0)
    {
        return 0;
    }
    
    while (i_ret < loop_cnt)
    {
        p_timer_node = delete_first_timer_from_list(p_timer_set, p_list_head);
        if (p_timer_node == NULL)
        {
            break;
        }
        
        if (insert_timer_node(p_timer_set, p_timer_node) < 0)
        {
            printf("%s --> %s --> L%d : insert_timer_node() fail, timer_id = %d \r\n",
                   __FILE__, __FUNCTION__, __LINE__, p_timer_node->user_timer.i_timer_id);
            
            free_timer_node(p_timer_set, p_timer_node);
        }
        
        i_ret++;
    }
    
    return i_ret;
}

static inline int move_timer_from_higher_level(timer_set_t * p_timer_set, int curr_level)
{
    timer_list_head_t * p_list_head = NULL;
    int higher_level = curr_level + 1;
    
    if (higher_level >= 8)
    {
        return 0;
    }
    
    // 获取将本级当前链表
    p_list_head = &(p_timer_set->timer_lists[higher_level][p_timer_set->curr_list_index[higher_level]]);
    
    // 移动本级当前链指针
    p_timer_set->curr_list_index[higher_level] = (p_timer_set->curr_list_index[higher_level] + 1) & 0x00FF;
    
    // 分发目标链表到低一级
    move_timer_to_low_level(p_timer_set, p_list_head);
    
    if (p_timer_set->curr_list_index[higher_level] == 0)
    {   // 将高一级的分发到本级
        move_timer_from_higher_level(p_timer_set, higher_level);
    }
    
    return 0;
}

void run_timer_set(timer_set_t * p_timer_set)
{
    timer_list_head_t * p_curr_list_head = NULL;
    timer_node_t * p_timer_node = NULL;
    uint32_t u32_loop_cnt = 0;
    
    if (p_timer_set == NULL)
    {
        printf("%s --> %s --> L%d : p_timer_set == NULL \r\n", __FILE__, __FUNCTION__, __LINE__);
        return;
    }
    
    p_timer_set->u64_current_jiffies++;
    
    p_curr_list_head = &(p_timer_set->timer_lists[0][p_timer_set->curr_list_index[0]]);
    
    p_timer_set->curr_list_index[0] = (p_timer_set->curr_list_index[0] + 1) & 0x00FF;
    
    if (p_curr_list_head->u32_cnt > 0)
    {
        u32_loop_cnt = p_curr_list_head->u32_cnt;
        while (u32_loop_cnt > 0)
        {
            p_timer_node = delete_first_timer_from_list(p_timer_set, p_curr_list_head);
            if (p_timer_node == NULL)
            {
                printf("%s --> %s --> L%d : p_curr_list_head->u32_cnt == %u \r\n", __FILE__, __FUNCTION__, __LINE__, p_curr_list_head->u32_cnt);
                init_timer_list_head(p_curr_list_head);
                break;
            }
            
            run_one_timer(p_timer_set, p_timer_node);
            
            if (p_timer_node->user_timer.u32_loop_cnt >= 0x80000000)
            {
                p_timer_node->u64_expiration_jiffies += p_timer_node->u64_hold_jiffies;
                if (insert_timer_node(p_timer_set, p_timer_node) < 0)
                {
                    printf("%s --> %s --> L%d : insert_timer_node() fail, timer_id = %d \r\n", __FILE__, __FUNCTION__, __LINE__, 
                           p_timer_node->user_timer.i_timer_id);
                    
                    free_timer_node(p_timer_set, p_timer_node);
                }
            }
            else if (p_timer_node->user_timer.u32_loop_cnt > 1)
            {
                p_timer_node->user_timer.u32_loop_cnt--;
                p_timer_node->u64_expiration_jiffies += p_timer_node->u64_hold_jiffies;
                if (insert_timer_node(p_timer_set, p_timer_node) < 0)
                {
                    printf("%s --> %s --> L%d : insert_timer_node() fail, timer_id = %d \r\n",
                           __FILE__, __FUNCTION__, __LINE__, p_timer_node->user_timer.i_timer_id);
                    
                    free_timer_node(p_timer_set, p_timer_node);
                }
            }
            else
            {
                free_timer_node(p_timer_set, p_timer_node);
            }
            
            u32_loop_cnt--;
        }
    }
    
    if (p_timer_set->curr_list_index[0] == 0)
    {
        move_timer_from_higher_level(p_timer_set, 0);
    }
}



#ifndef HASH_TABLE_H
#define HASH_TABLE_H

// 应用必须定义:
// #define MAX_KEY_LEN         63
// typedef struct _user_data
// {
//     ...
//     ...
//     ...
// } user_data_t;

uint32_t hash_func_u32_31(char * key_string, int key_len)
{
    uint32_t hash_u32 = key_string[0];
    int i = 0;
    
    for (i=1; i<key_len; i++)
    {
        hash_u32 = hash_u32*31 + key_string[i];
    }
    return hash_u32;
}

#define HASH_BUCKET_CNT     8191

typedef struct _hash_node
{
    char key[MAX_KEY_LEN+1];
    int key_len;
    uint32_t key_hash;
    struct _hash_node * next;
    user_data_t user_data;
} hash_node_t;

typedef struct _hash_table
{
    void * hash_bucket[HASH_BUCKET_CNT];
} hash_table_t;

static int init_hash_table(hash_table_t * p_hash_table)
{
    int i = 0;
    
    if (p_hash_table == NULL)
    {
        return -1;
    }
    
    for (i=0; i<HASH_BUCKET_CNT; i++)
    {
        p_hash_table->hash_bucket[i] = NULL;
    }
    
    return 1;
}

static hash_node_t * get_hash_node(hash_table_t * p_hash_table, char * key, int key_len, uint32_t key_hash)
{
    int bucket = 0;
    hash_node_t * p_hash_node = NULL;
    
    bucket = key_hash % HASH_BUCKET_CNT;
    
    p_hash_node = (hash_node_t *)(p_hash_table->hash_bucket[bucket]);
    while (p_hash_node != NULL)
    {
        if (p_hash_node->key_hash == key_hash && p_hash_node->key_len == key_len &&
            memcmp(p_hash_node->key, key, key_len) == 0)
        {
            break;
        }
        
        p_hash_node = p_hash_node->next;
    }
    
    return p_hash_node;
}

static user_data_t * get_from_hash_table(hash_table_t * p_hash_table, char * key, int key_len)
{
    uint32_t key_hash = 0;
    hash_node_t * p_hash_node = NULL;
    
    if (p_hash_table == NULL || key == NULL || key_len <= 0 || key_len > MAX_KEY_LEN)
    {
        return NULL;
    }
    
    key_hash = hash_func_u32_31(key, key_len);
    
    p_hash_node = get_hash_node(p_hash_table, key, key_len, key_hash);
    if (p_hash_node == NULL)
    {
        return NULL;
    }
    else
    {
        return &(p_hash_node->user_data);
    }
}

static user_data_t * insert_to_hash_table(hash_table_t * p_hash_table, char * key, int key_len, user_data_t * p_user_data)
{
    hash_node_t * p_hash_node = NULL;
    uint32_t key_hash = 0;
    hash_node_t ** pp_curr_node = NULL;
    int bucket = 0;
    
    if (p_hash_table == NULL || key == NULL || key_len <= 0 || key_len > MAX_KEY_LEN || p_user_data == NULL)
    {
        return NULL;
    }
    
    key_hash = hash_func_u32_31(key, key_len);
    p_hash_node = get_hash_node(p_hash_table, key, key_len, key_hash);
    if (p_hash_node != NULL)
    {
        p_hash_node->user_data = *p_user_data;
        return &(p_hash_node->user_data);
    }
    
    p_hash_node = (hash_node_t *)calloc(1, sizeof(hash_node_t));
    if (p_hash_node == NULL)
    {
        return NULL;
    }
    
    memcpy(p_hash_node->key, key, key_len);
    p_hash_node->key_hash = key_hash;
    p_hash_node->key_len = key_len;
    p_hash_node->user_data = *p_user_data;
    p_hash_node->next = NULL;
    
    bucket = key_hash % HASH_BUCKET_CNT;
    
    pp_curr_node = (hash_node_t **)&(p_hash_table->hash_bucket[bucket]);
    while (*pp_curr_node != NULL)
    {
        pp_curr_node = &((*pp_curr_node)->next);
    }
    *pp_curr_node = p_hash_node;
    
    return &(p_hash_node->user_data);
}

static int delete_from_hash_table(hash_table_t * p_hash_table, char * key, int key_len)
{
    hash_node_t * p_hash_node = NULL;
    uint32_t key_hash = 0;
    hash_node_t ** pp_curr_node = NULL;
    int bucket = 0;
    
    if (p_hash_table == NULL || key == NULL || key_len <= 0 || key_len > MAX_KEY_LEN)
    {
        return -1;
    }
    
    key_hash = hash_func_u32_31(key, key_len);
    p_hash_node = get_hash_node(p_hash_table, key, key_len, key_hash);
    if (p_hash_node == NULL)
    {
        return -2;
    }
    bucket = key_hash % HASH_BUCKET_CNT;
    pp_curr_node = (hash_node_t **)&(p_hash_table->hash_bucket[bucket]);
    while (*pp_curr_node != NULL)
    {
        if (*pp_curr_node == p_hash_node)
        {
            break;
        }
        
        pp_curr_node = &((*pp_curr_node)->next);
    }
    
    if (*pp_curr_node != NULL)
    {
        *pp_curr_node = (*pp_curr_node)->next;
        free(p_hash_node);
        return 1;
    }
    else
    {
        return -3;
    }
}


#endif


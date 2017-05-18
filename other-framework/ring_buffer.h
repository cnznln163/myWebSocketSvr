
/**
 * ring_buffer.h
 **/

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "Public.h"

typedef struct ring_buffer
{
    uint32_t buffer_length;
    uint32_t read_index;
    uint32_t write_index;
    uint8_t ring_buffer[0];
} ring_buffer_t;

static inline ring_buffer_t * clear_ring_buffer(ring_buffer_t * p_ring_buffer)
{
    if (p_ring_buffer != NULL)
    {
        p_ring_buffer->write_index = 0;
        p_ring_buffer->read_index = 0;
    }
    
    return p_ring_buffer;
}

static ring_buffer_t * create_ring_buffer(uint32_t buffer_length)
{
    ring_buffer_t * p_ring_buffer = NULL;
    
    p_ring_buffer = (ring_buffer_t *)calloc(1, sizeof(ring_buffer_t)+buffer_length);
    if (p_ring_buffer == NULL)
    {
        return NULL;
    }
    
    p_ring_buffer->buffer_length = buffer_length;
    return p_ring_buffer;
}

static inline void destroy_ring_buffer(ring_buffer_t * p_ring_buffer)
{
    if (p_ring_buffer != NULL)
    {
        free(p_ring_buffer);
    }
}

static inline int get_buffer_data_len(ring_buffer_t * p_ring_buffer)
{
    if (p_ring_buffer == NULL || p_ring_buffer->write_index == p_ring_buffer->read_index)
    {
        return 0;
    }
    
    if (p_ring_buffer->write_index > p_ring_buffer->read_index)
    {
        return (p_ring_buffer->write_index - p_ring_buffer->read_index);
    }
    else
    {
        return (p_ring_buffer->buffer_length - p_ring_buffer->read_index + p_ring_buffer->write_index);
    }
}

static inline int get_buffer_free_space(ring_buffer_t * p_ring_buffer)
{
    int free_space = 0;
    
    if (p_ring_buffer == NULL)
    {
        return 0;
    }
    
    free_space = p_ring_buffer->buffer_length - get_buffer_data_len(p_ring_buffer);
    
    if (free_space > 64)
    {
        return (free_space - 64);
    }
    else
    {
        return 0;
    }
}

static inline int buffer_is_empty(ring_buffer_t * p_ring_buffer)
{
    if (p_ring_buffer->write_index == p_ring_buffer->read_index)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static inline int buffer_is_full(ring_buffer_t * p_ring_buffer)
{
    if (get_buffer_free_space(p_ring_buffer) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static inline int write_buffer(ring_buffer_t * p_ring_buffer, uint8_t * data, int data_len)
{
    int copy_len = 0;
    
    if (p_ring_buffer == NULL || data == NULL || data_len <= 0)
    {
        return -1;
    }
    
    if (get_buffer_free_space(p_ring_buffer) < data_len)
    {
        return -2;
    }
    
    copy_len = p_ring_buffer->buffer_length - p_ring_buffer->write_index;
    if (copy_len > data_len)
    {
        copy_len = data_len;
    }
    memcpy(&(p_ring_buffer->ring_buffer[p_ring_buffer->write_index]), data, copy_len);
    if (copy_len < data_len)
    {
        memcpy(p_ring_buffer->ring_buffer, &(data[copy_len]), data_len - copy_len);  
    }
    p_ring_buffer->write_index = (p_ring_buffer->write_index + data_len) % p_ring_buffer->buffer_length;
    
    return data_len;
}

static inline int read_buffer(ring_buffer_t * p_ring_buffer, uint8_t * data_buffer, int read_length)
{
    int data_len = 0;
    int copy_len = 0;
    
    if (p_ring_buffer == NULL || data_buffer == NULL || read_length <= 0)
    {
        return -1;
    }
    
    if (buffer_is_empty(p_ring_buffer))
    {
        return 0;
    }
    
    data_len = get_buffer_data_len(p_ring_buffer);
    if (data_len > read_length)
    {
        data_len = read_length;
    }
    
    copy_len = p_ring_buffer->buffer_length - p_ring_buffer->read_index;
    if (copy_len > data_len)
    {
        copy_len = data_len;
    }
    memcpy(data_buffer, &(p_ring_buffer->ring_buffer[p_ring_buffer->read_index]), copy_len);
    if (copy_len < data_len)
    {
        memcpy(&(data_buffer[copy_len]), p_ring_buffer->ring_buffer, data_len - copy_len);  
    }
    p_ring_buffer->read_index = (p_ring_buffer->read_index + data_len) % p_ring_buffer->buffer_length;
    
    return data_len;
}

static inline int peek_buffer(ring_buffer_t * p_ring_buffer, uint8_t * data_buffer, int read_length)
{
    int data_len = 0;
    int copy_len = 0;
    
    if (p_ring_buffer == NULL || data_buffer == NULL || read_length <= 0)
    {
        return -1;
    }
    
    if (buffer_is_empty(p_ring_buffer))
    {
        return 0;
    }
    
    data_len = get_buffer_data_len(p_ring_buffer);
    if (data_len > read_length)
    {
        data_len = read_length;
    }
	
    copy_len = p_ring_buffer->buffer_length - p_ring_buffer->read_index;
    if (copy_len > data_len)
    {
        copy_len = data_len;
    }
    memcpy(data_buffer, &(p_ring_buffer->ring_buffer[p_ring_buffer->read_index]), copy_len);
    if (copy_len < data_len)
    {
        memcpy(&(data_buffer[copy_len]), p_ring_buffer->ring_buffer, data_len - copy_len);  
    }
    
    return data_len;
}

static inline int peek_i32_from_buffer(ring_buffer_t * p_ring_buffer)
{
    uint32_t u32_data = 0;
    uint8_t * p_data = (uint8_t *)&u32_data;
    int read_index = 0;
    int offset = 0;
    
    if (p_ring_buffer == NULL || get_buffer_data_len(p_ring_buffer) < 4)
    {
        return -1;
    }
    
    read_index = p_ring_buffer->read_index;
    offset = p_ring_buffer->buffer_length - read_index;
    if (offset >= 4)
    {
        p_data[0] = p_ring_buffer->ring_buffer[read_index++];
        p_data[1] = p_ring_buffer->ring_buffer[read_index++];
        p_data[3] = p_ring_buffer->ring_buffer[read_index++];
        p_data[3] = p_ring_buffer->ring_buffer[read_index];
    }
    else if (offset == 3)
    {
        p_data[0] = p_ring_buffer->ring_buffer[read_index++];
        p_data[1] = p_ring_buffer->ring_buffer[read_index++];
        p_data[2] = p_ring_buffer->ring_buffer[read_index];
        p_data[3] = p_ring_buffer->ring_buffer[0];
    }
    else if (offset == 2)
    {
        p_data[0] = p_ring_buffer->ring_buffer[read_index++];
        p_data[1] = p_ring_buffer->ring_buffer[read_index];
        p_data[2] = p_ring_buffer->ring_buffer[0];
        p_data[3] = p_ring_buffer->ring_buffer[1];
    }
    else if (offset == 1)
    {
        p_data[0] = p_ring_buffer->ring_buffer[read_index];
        p_data[1] = p_ring_buffer->ring_buffer[0];
        p_data[2] = p_ring_buffer->ring_buffer[1];
        p_data[3] = p_ring_buffer->ring_buffer[2];
    }
    
    return u32_data;
}

static inline int move_read_index(ring_buffer_t * p_ring_buffer, uint32_t u32_offset)
{
    if (u32_offset <= (uint32_t)get_buffer_data_len(p_ring_buffer))
	{
		p_ring_buffer->read_index = (p_ring_buffer->read_index + u32_offset) % p_ring_buffer->buffer_length;
		return 1;
	}
	else
	{
		return -1;
	}
}

static inline int move_write_index(ring_buffer_t * p_ring_buffer, uint32_t u32_offset)
{
    if (u32_offset <= (uint32_t)get_buffer_free_space(p_ring_buffer))
	{
		p_ring_buffer->write_index = (p_ring_buffer->write_index + u32_offset) % p_ring_buffer->buffer_length;
		return 1;
	}
	else
	{
		return -1;
	}
}

#endif  // RING_BUFFER_H


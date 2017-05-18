#ifndef C_OBJECT_MANAGER_H
#define C_OBJECT_MANAGER_H

#include "Public.h"

#include <list>
#include <set>

#include "log_udp.h"

template <class CObject>
class CObjectManager
{
private:
    CObjectManager()
    {
        _object_cnt = 0;
        _p_object_array = NULL;
        _free_object_list.clear();
        _allocated_object_set.clear();
    }
    
public:
    ~CObjectManager()
    {
        delete [] _p_object_array;
    }
    
    static CObjectManager * Instance(void)
    {
        static CObjectManager * _instance = NULL;
        
        if (_instance == NULL)
        {
            _instance = new CObjectManager;
        }
        
        return _instance;
    }
    
    int Init(CObject * p_object_array, int object_cnt)
    {
        int i = 0;
        
        if (p_object_array == NULL || object_cnt <= 0)
        {
            return -1;
        }
        
        if (_p_object_array != NULL && _object_cnt > 0)
        {
            _free_object_list.clear();
            _allocated_object_set.clear();
            delete [] _p_object_array;
        }
        
        _p_object_array = p_object_array;
        _object_cnt = object_cnt;
        
        for (i=0; i<_object_cnt; i++)
        {
            _free_object_list.push_back((void *)&_p_object_array[i]);
        }
        
        return _free_object_list.size();
    }
    
#if 0
	const CObject * GetObjectArray() const
	{
		return _p_object_array;
	}

	inline int GetObjectCount() const 
	{
		return _object_cnt; 
	}
#endif

	const std::set<void *>& GetAllocatedObjectSet() const 
	{
		return _allocated_object_set;
	}

    CObject * AllocateOneObject(void)
    {
        CObject * p_obj = NULL;
        
        if (_free_object_list.size() > 0)
        {
            p_obj = (CObject *)_free_object_list.front();
            _free_object_list.pop_front();
            _allocated_object_set.insert(p_obj);
        }
        
        log_info("p_obj:%p allocated_object_cnt:%d free_object_cnt:%d \r\n",
                  p_obj, (int)_allocated_object_set.size(), (int)_free_object_list.size());
        
        return p_obj;
    }
    
    void ReleaseOneObject(void * p_obj)
    {
        if (p_obj != NULL)
        {
            _free_object_list.push_front(p_obj);
            std::set<void *>::iterator itr = _allocated_object_set.find(p_obj);
            if (itr != _allocated_object_set.end())
            {
                _allocated_object_set.erase(itr);
            }
            
            log_info("p_obj:%p allocated_object_cnt:%d free_object_cnt:%d \r\n",
                     p_obj, (int)_allocated_object_set.size(), (int)_free_object_list.size());
        }
    }
    
    int DumpRuntimeInfo(char * buffer, int buffer_len)
    {
        return snprintf(buffer, buffer_len, "total:%d  allocated:%d  freed:%d \r\n",
                        _object_cnt, _allocated_object_set.size(), _free_object_list.size());
    }
    
private:
    int _object_cnt;            // 管理的对象数
    CObject * _p_object_array;  // 管理的对象数组
    std::list<void *> _free_object_list;    // 空闲链表
    std::set<void *> _allocated_object_set; // 已分配集合
};


#endif


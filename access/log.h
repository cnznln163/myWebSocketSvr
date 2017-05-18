#ifndef LOG_H
#define	LOG_H
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#ifdef	__cplusplus
extern "C" 
{
#endif 

#define MAX_LOG_LINE 1024    
#define LOG_EMERG 0 
#define LOG_ALERT 1
#define LOG_CRIT  2 
#define LOG_ERR   3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6 
#define LOG_DEBUG 7
#define LOG_GAMB 8

#define LOG_ROOT_DIR "./"

#define LOG_SUCCESSFUL 1
#define LOG_FAIL  0
#define LOG_SWITCH_DEBUG 0

extern char g_log_dir[256];
int dir_exists( time_t *t , const char *interface );   
int log_write(int level, const char *msgfmt, ...);
#ifdef	__cplusplus
}
#endif
#endif	
/* LOG_H */


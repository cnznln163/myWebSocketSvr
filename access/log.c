#include "log.h"

#define LOCAL_LOG_CONTENT_MAX 0x4000
static const char *log_level[] ={       
									   "emergency",       
										"alert",       
										"critical",        
										"error",        
										"warning",        
										"notice",        
										"info",        
										"debug" ,
										"gamb"
					}; 

static char g_log_contents[LOCAL_LOG_CONTENT_MAX];					
					
/**
* 判断目录是否存在,不存在则创建
*/
int dir_exists( time_t *t , const char *interface ){
	char interface_dir[100];
	char year_dir[120];
	char month_dir[120];
	struct tm *stm;
	stm = gmtime(t);
	snprintf(interface_dir , sizeof(interface_dir) , "%s/%s" , LOG_ROOT_DIR , interface );
	if(  0 != access( interface_dir , F_OK ) ){
		if(mkdir( interface_dir , 0755 ) == -1){
			perror( "mkdir error interface" );
			return LOG_FAIL;
		}
	}
	snprintf(year_dir , sizeof(year_dir) , "%s/%s/%d" , LOG_ROOT_DIR , interface , stm->tm_year + 1900 );
	if(  0 != access( year_dir , F_OK ) ){
		if(mkdir( year_dir , 0755 ) == -1){
			perror( "mkdir error year" );
			return LOG_FAIL;
		}
	}
	snprintf( month_dir , sizeof( month_dir ) , "%s/%d" , year_dir , 1+stm->tm_mon );
	if(  0 != access( month_dir , F_OK)){
		if(mkdir( month_dir , 0755 ) == -1){
			perror( "mkdir error month" );
			return LOG_FAIL;
		}
	}
	return LOG_SUCCESSFUL;
}

/**
* 通用写入
*/
int log_write(  int level , const char *fmt, ... ){
	time_t t;
	struct tm stm;
	char *pos;
	int len,vslen;
	
	char dir[120]={0};
	FILE *log_fd;
	time(&t);
	localtime_r(&t,&stm);
	memset(g_log_contents,0,LOCAL_LOG_CONTENT_MAX);
	len	= snprintf(g_log_contents , LOCAL_LOG_CONTENT_MAX , "[%d-%02d-%02d %02d:%02d:%02d][%ld][%s]-" ,
									stm.tm_year + 1900,1+stm.tm_mon,stm.tm_mday,stm.tm_hour,stm.tm_min , stm.tm_sec , t , log_level[level] );
	snprintf(dir , sizeof(dir) , "%s/%s_%02d_%02d_%02d.log" , g_log_dir , log_level[level] , stm.tm_year + 1900 , 1+stm.tm_mon, stm.tm_mday);
	log_fd	=	fopen(dir , "a+");
	if( !log_fd ){
		return LOG_FAIL;
	}
	pos	=	g_log_contents;
	pos +=  len;
	va_list arg_ptr; 
	va_start(arg_ptr, fmt);
	vslen = vsnprintf(pos, LOCAL_LOG_CONTENT_MAX - len, fmt, arg_ptr);
	va_end(arg_ptr);
	fwrite( g_log_contents , len+vslen, 1 , log_fd );
	fclose(log_fd);
	return LOG_SUCCESSFUL;
}



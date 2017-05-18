#ifndef LOCALPHP_CONFIG_H
#define LOCALPHP_CONFIG_H

#include <iostream>
#include <string>
#include <stdio.h>
#include <cstring>
#include <map>
using namespace std;

class CConfig
{
public:
	CConfig(string filename){ 
		ok=0;
		fp = fopen(filename.c_str(),"r");
		if( !fp ){
			return ;
		}
		ok = 1;
		char buf[256];
		for(;;){
			char* p = fgets(buf, 256, fp);
			if (!p){
				break;
			}
			size_t len = strlen(buf);
			if (buf[len - 1] == '\n'){
				buf[len - 1] = 0;			// remove \n at the end
			}

			char* ch = strchr(buf, '#');	// remove string start with #
			if (ch){
				*ch = 0;
			}
			if (strlen(buf) == 0){
				continue;
			}
			char* q = strchr(buf, '=');
			if (q == NULL){
				continue;
			}
			*q = 0;
			char* key =  _trimspace(buf);
			//string key_str( key );
			char* value = _trimspace(q + 1);
			//string value_str( value );
			
			if (key && value){
				vals.insert( make_pair(key,value) );
			}
		}

	}
	~CConfig(){
		fclose(fp);
	};
	string getval(string key){
		if(!ok){
			return NULL;
		}
		map <string,string>::iterator iter;
		iter = vals.find(key);
		if(iter != vals.end()){
			return iter->second;
		}
		return NULL;
	}
	/* data */
private:
	bool ok;
	FILE *fp;
	map<string, string> vals;
	char* _trimspace(char *name){
		char* start_pos = name;
		while ( (*start_pos == ' ') || (*start_pos == '\t') ){
			start_pos++;
		}
		if (strlen(start_pos) == 0)
			return NULL;

		// remove ending space or tab
		char* end_pos = name + strlen(name) - 1;
		while ( (*end_pos == ' ') || (*end_pos == '\t') ){
			*end_pos = 0;
			end_pos--;
		}

		int len = (int)(end_pos - start_pos) + 1;
		if (len <= 0)
			return NULL;

		return start_pos;
	}
};



#endif

#include <pthread.h>
#include<bits/stdc++.h>
#include "storage.h"
using namespace std; 
class cache
{
public:
class entry{
    public:
    string key="";
    string value="";
    bool valid=1;
    bool modified=0;
    int last_used=-1;
    int frequency=0;
};
int max_capacity;
const int KEY_SIZE= 256;
const int VAL_SIZE =256;
entry (cache)[10024];
long long int current_capacity;
long long unsigned int counter;
pthread_mutex_t mutex;
// bool delete_key(string);
// void find_new_slot();


string get_entry(string key){
    pthread_mutex_lock(&mutex);
	//cout<<"GETTING "<<key<<endl;
    for(int i=0;i<max_capacity;i++){
        if(cache[i].last_used!=-1 && cache[i].key==key){
        cache[i].last_used=++counter;
        cache[i].frequency++;
       	show_all_elements();
        pthread_mutex_unlock(&mutex);
    	return cache[i].value;
        }
    }
    char val[256];
    int return_ =file_get(toCharArray(key),val);
    cout<<return_<<endl;
    if(return_){
 	   pthread_mutex_unlock(&mutex);
       put_into_cache(key,toString(val),1);
       show_all_elements();
    	return toString(val);
	}
    show_all_elements();
    pthread_mutex_unlock(&mutex);
    return "KEY NOT EXIST";
}

void initialize_cache(int size_){
    storage_init();
    //cout<<"Size oF  Cache is "<<size_<<endl;
    current_capacity=size_;
    max_capacity=size_;
//    cache=(entry**)malloc(sizeof(entry *)*size_);
    //cout<<"Initialized\n";
    pthread_mutex_init(&mutex, NULL);
    counter=0;
}
void show_all_elements(){
	cout<<"===================================================\n";
    for(int i=0;i<max_capacity;i++){
        cout<<cache[i].key<<" --- "<<cache[i].value<<" --- "<<cache[i].last_used<<endl;
    }
   	cout<<"===================================================\n";

}
bool put_into_cache(string key1,string value2,int code){  
	cout<<"Put Called\n";
	pthread_mutex_lock(&mutex);
    cout<<"Putting11 "<<key1<<" - "<<value2<<endl;
    int loc=-1;
    for(int i=0;i<max_capacity;i++){
        if(cache[i].key==key1){
            //cout<<"Putting ";
            loc=i;
            break;
        }
    }
    if(loc!=-1){
        cache[loc].value=value2;
        cache[loc].last_used=counter++;
    	show_all_elements();
    	pthread_mutex_unlock(&mutex);
        return 1;
    }
    ////cout<<"Putting "<<key1<<" - "<<value2<<endl;
    //cout<<key1.compare(value2)<<endl;
    if(key1.length()>256 || value2.length()>256 || key1.length()==0 ||value2.length()==0 || !key1.compare(value2)){
 		show_all_elements();
	    pthread_mutex_unlock(&mutex);
        return 0;
    }
    int slot;
        //cout<<"Debug7"<<endl;
    slot=find_new_slot(code);
    
    int k; 
    //cout<<"Debug1"<<endl;
    cache[slot].key=key1;
    //cout<<"Debug2"<<endl;
    cache[slot].value =value2;
    //cout<<"Debug3"<<endl;
    cache[slot].valid=1;
    //cout<<"Debug4"<<endl;
    cache[slot].modified=0;
    //cout<<"Debug5"<<endl;
    cache[slot].last_used=counter++;
    cache[slot].frequency++;
    //cout<<"Debug6"<<endl;
    //cout<<"Added Entry:"<<key1<<"  -  "<<cache[slot].value<<endl;
    show_all_elements();
    file_put(toCharArray(key1),toCharArray(value2));
    pthread_mutex_unlock(&mutex);
    return 1;
}

int find_new_slot(int code){
    int lowest;
    int last_mod=INT_MAX;
    for(int i=0;i<max_capacity;i++){
            if(code==1){
                if(cache[i].last_used==-1)
                    return i;
                if(cache[i].last_used<last_mod){
                    lowest=i;
                    last_mod=cache[i].last_used;
                }
            }
            else{
                if(cache[i].last_used==-1)
                    return i;
                if(cache[i].frequency<last_mod){
                    lowest=i;
                    last_mod=cache[i].frequency;
                }
            }
    }
    return lowest;
}
bool delete_key(string key){
    //cout<<"DELETEING ENTRY KEY : "<<key<<endl;  
	 pthread_mutex_lock(&mutex);
    int loc=-1;
    for(int i=0;i<max_capacity;i++){
        if(cache[i].key==key){
            loc=i;
            break;
        }
    }
    char val[256];
    if(loc==-1){
    	int return_ =file_get(toCharArray(key),val);
    	if(return_){
    		file_del(toCharArray(key));				
	   		pthread_mutex_unlock(&mutex);
    		return true;
    	}
    	pthread_mutex_unlock(&mutex);
        return false;
    }
    file_del(toCharArray(cache[loc].key));
    cache[loc].key="";
    cache[loc].value="";
    cache[loc].last_used=-1;
    show_all_elements();
    pthread_mutex_unlock(&mutex);
    return true;
}
};

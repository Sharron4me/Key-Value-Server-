#include <pthread.h>
#include<bits/stdc++.h>
#include "storage.h"
using namespace std; 
class cache
{
public:
struct entry{
    string key;
    string value;
    bool valid;
    bool modified;
    int last_used=-1;
    int frequency=0;
    pthread_mutex_t lock;
};

const int KEY_SIZE= 256;
const int VAL_SIZE =256;
map<string,entry*>cache;
long long int current_capacity;
long long unsigned int counter;

// bool delete_key(string);
// void find_new_slot();


string get_entry(string key){
    if(cache.find(key)!=cache.end()){
        return cache[key]->value;
    }
    return "KEY NOT EXIST";
}

void initialize_cache(int size_){
    storage_init();
    current_capacity=size_;
    counter=0;
}
bool put_into_cache(string key1,string value2,int code){
    if(key1.length()>256 || value2.length()>256 || key1.length()==0 ||value2.length()==0 ){
        return 0;
    }
    struct entry * temp=(struct entry*)malloc(sizeof(struct entry));
    int k; 
    pthread_mutex_lock(&(temp->lock));
    temp->key=key1;
    temp->value =value2;
    temp->valid=1;
    temp->modified=0;
    if(code=1)
        temp->last_used=counter++;
    else
        temp->frequency++;
    if(current_capacity<=0){
        find_new_slot(code);
        current_capacity++;
    }
    cache[key1]=temp;
    current_capacity--;
    counter++;
    file_put(toCharArray(key1),toCharArray(value2));
    pthread_mutex_unlock(&(temp->lock));
    return 1;
}

void find_new_slot(int code){
    cout<<"Problem here"<<endl;
    string lowest;
    int last_mod=INT_MAX;
    for(map<string,entry*>::const_iterator it=cache.begin();it!=cache.end();++it){
            if(code==1){
                if(it->second->last_used<last_mod){
                    lowest=it->first;
                    last_mod=it->second->last_used;
                }
            }
            else{
                if(it->second->frequency<last_mod){
                    lowest=it->first;
                    last_mod=it->second->frequency;
                }
            }
    }
    entry* temp = cache[lowest];
    cout<<"DELETEING ENTRY KEY : "<<lowest<<endl;
    file_del(toCharArray(lowest));
    cache.erase(lowest);
//    free(temp);
}

void print_cache(){
    for(map<string,entry*>::const_iterator it=cache.begin();it!=cache.end();++it)
        cout<<it->first<<" "<<it->second->value<<endl;
}

bool delete_key(string key){
//    cout<<"DELETEING ENTRY KEY : "<<key<<endl;  
    auto it = cache.find(key);
    if(it==cache.end())
        return false;
    pthread_mutex_lock(&(it->second->lock));
    file_del(toCharArray(it->first));
    cache.erase(it->first);
    pthread_mutex_unlock(&(it->second->lock));
 //   free(it->second);
    return true;
}
};

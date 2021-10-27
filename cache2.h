#include <pthread.h>
#include<bits/stdc++.h>

using namespace std; 
class cache
{
public:
struct entry{
    string key;
    string value;
    bool valid;
    bool modified;
    int frequency=0;
    pthread_mutex_t lock;
};

const int KEY_SIZE= 256;
const int VAL_SIZE =256;
const int max_size =3;
map<string,entry*>cache;
long long int current_capacity;
long long unsigned int counter;

// bool delete_key(string);
// void find_new_slot();


string get_entry(string key){
    if(cache.find(key)!=cache.end()){
        cache[key]->frequency++;    
        return cache[key]->value;
    }
    return "Entry Not Found";
}

void initialize_cache(){
    current_capacity=max_size;
    counter=0;
}
bool put_into_cache(string key1,string value2){
    if(key1.length()>256 || value2.length()>256){
        return false;
    }
    struct entry * temp=(struct entry*)malloc(sizeof(struct entry));
    int k; 
    pthread_mutex_lock(&(temp->lock));
    temp->key=key1;
    temp->value =value2;
    temp->valid=1;
    temp->modified=0;
    temp->frequency++;
    if(current_capacity<=0){
        find_new_slot();
        current_capacity++;
    }
    cache[key1]=temp;
    current_capacity--;
    counter++;
    pthread_mutex_unlock(&(temp->lock));
    return true;
}

void find_new_slot(){
    string lowest;
    int last_mod=INT_MAX;
    for(map<string,entry*>::const_iterator it=cache.begin();it!=cache.end();++it){
        if(it->second->frequency<last_mod){
            lowest=it->first;
            last_mod=it->second->frequency;
        }
    }
    pthread_mutex_lock(&(cache[lowest]->lock));
    entry* temp = cache[lowest];	
    cout<<"DELETEING ENTRY KEY : "<<lowest<<endl;
    cache.erase(lowest);
    pthread_mutex_unlock(&(cache[lowest]->lock));
//    free(temp);
}

void print_cache(){
    for(map<string,entry*>::const_iterator it=cache.begin();it!=cache.end();++it)
        cout<<it->first<<" "<<it->second->value<<endl;
}

bool delete_key(string key){
    cout<<"DELETEING ENTRY KEY : "<<key<<endl;  
    auto it = cache.find(key);
    if(it==cache.end())
        return false;
    pthread_mutex_lock(&(it->second->lock));
    cache.erase(it->first);
    pthread_mutex_unlock(&(it->second->lock));
//    free(it->second);
    return true;
}
};

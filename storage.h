#include<stdlib.h>
#include<semaphore.h>
#include <stdint.h>
#include<unistd.h>
#include<pthread.h>
#include<stdio.h>
#include<fcntl.h> 
#include<sys/stat.h>
#include <limits.h>
#include <sys/types.h>
#include<string.h>
#include<unistd.h>
#include<iostream>
using namespace std;



int *fds;
int setSize=2;
int *readCounters;
sem_t *mutex1;
sem_t *readerLocks;
sem_t x,y;
pthread_t tid;
pthread_t writerthreads[100],readerthreads[100];
int readercount = 0;



unsigned modulus1( char *num, size_t size, unsigned divisor) {
  unsigned rem = 0,temp = 0;
  
  int i=0;
 while(1)
 {
     if(i>=divisor)
        break;
     temp=num[i]%divisor;
     rem += temp;
     i++;
 }
 unsigned ans = rem%divisor;
  return ans;
}


int file_search(char *key, char *value, int index) {
    
    // Setting pointer to the start of file
    lseek(fds[index], 0, SEEK_SET);
    char fkey[256],fval[256];
    int offset = 0;


    int size;


    do {
        // Reading key 
        size = read(fds[index], fkey, sizeof(fkey));

        if (size<=0)        break;
        // Reading value         
        size=read(fds[index], fval, sizeof(fval));
        if(size<=0)         break;

        // Comparing keys if found same we will update the key on offset and copy the value in value incase of get call 
      if(strcmp(key,fkey)  ==0 ) {
            if(value!=0) 
            {
                // copy key's value 
                memcpy(value, fval, sizeof(fval));

            }



            return offset;
        }
        offset += sizeof(fkey) + sizeof(fval);
    } while (1);
    return -1;
}


char *toCharArray(string surname)
{
    char *temp = new char[265];
    for(int i=0;i<=surname.length();i++)
    {
        temp[i]=surname[i];
    }
    return temp;
}

string toString(char* a)
{
    int i;
    string s = "";
    for (i = 0; i < 256; i++) {
        if(a[i]=='\0'){
            s=s+"\0";
            break;
        }
        else
            s = s + a[i];
    }
    return s;
}

int file_del( char *key)
{   
    
    int offset;
    // finding index of the key
    int index=modulus1(key,256,setSize);
    
    char fkey[256] ,fval[256];

    // Locking the mutex1 
    sem_wait(&mutex1[index]);
    
    // flag variable 
    int flag=0;

    //looking for offset 
    offset = file_search(key, NULL, index);

    if(offset >= 0) {
        
        lseek(fds[index], offset, SEEK_SET);
        // setting value to all zeros
        memset(fkey, 0, 256);
        memset(fval, 0, 256);

        write(fds[index], fkey, 256);
        write(fds[index], fval, 256);
        flag = 1;
    }
    
    sem_post(&mutex1[index]);
    return flag;
    
}
int file_get(char *key, char *value)
{   
    /* Gets the value stored at offset */
    /* Does not depend on key argument */
    int index=modulus1(key,256,setSize);
    int flag =0;
    sem_wait(&readerLocks[index]);
    // Reader started reading 
    readCounters[index]+=1;
    
    // Readers are there lock the mutex1
    if(readCounters[index]==1)
        sem_wait(&mutex1[index]);
   
   
    sem_post(&readerLocks[index]);
    
    if(file_search(key, value, index)>=0) {
         flag = 1;
    }
    
    sem_wait(&readerLocks[index]);

    readCounters[index]-=1;
    
    if(readCounters[index]==0)
    {
        sem_post(&mutex1[index]);
    }
    cout<<"StorageSays: "<<value<<endl;
    sem_post(&readerLocks[index]);
    return flag;
}

void file_put(char *key,char *value) {
     /*
   
        if found then ask the offset where it is present and if the value noy matches with the present value ,update the given line
        if not present then search for empty line and insert there!
     */
    int offset;
    int index=modulus1(key,256,setSize);
    sem_wait(&mutex1[index]);

    offset = file_search(key, NULL, index);

    // if we got nothing in the existing set
    if(offset < 0) {
        //adding value at the end of file
        lseek(fds[index], 0, SEEK_END);
        write(fds[index], key, 256);
        write(fds[index], value, 256);

    } else {
        //adding value at returned offset
        lseek(fds[index], offset, SEEK_SET);
        write(fds[index], key, 256);
        write(fds[index], value, 256);

    }
    sem_post(&mutex1[index]);

}
int storage_init()
{
    //  Creating Array of File Descriptors
    fds=(int *)malloc(sizeof(int)*setSize);
    int i=0;
    /*
        define the array of file descriptors  depending on the prefix 
        define the array of readCount as well as the semaphore (read x and write y) for the same
        PUT,DEL would use write lock 
        GET would use read lock
        each write should return the line number
    */
    // Creating array of read locks
    readerLocks=(sem_t *) malloc(sizeof(sem_t)*setSize);
    // Creating array of readcounters
    readCounters=(int *) malloc(sizeof(int)*setSize);



    mutex1=(sem_t *)malloc(sizeof(sem_t)*setSize);
    char fileName[22+setSize];
    for(i=0;i<setSize;i++)
    {
        snprintf(fileName,sizeof(fileName),"File%d.txt",i);


        fds[i]=open(fileName, O_CREAT|O_RDWR,S_IRWXU);
        
        if(fds[i]<0)
        {
            printf("\n[Error : Cannot open File%d.txt]\n",i);
        }
        //Initializing semaphores 
        sem_init(&readerLocks[i],0,1);

        //Initializing mutex1es 
        sem_init(&mutex1[i],0,1);
        readCounters[i]=0;
    }
    return 0;
}

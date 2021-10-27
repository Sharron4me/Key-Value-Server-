#include<stdlib.h>
#include<semaphore.h>
#include <stdint.h>
#include<unistd.h>
#include<pthread.h>
#include<stdio.h>
#include<iostream>
#include<fcntl.h> 
#include<sys/stat.h>
#include <limits.h>
#include <sys/types.h>
#include<string.h>
#include<unistd.h>


int *fds ,setSize=4,*readCounters,readercount = 0;;
sem_t *mutex,*readingLock,x,y;
pthread_t tid,writerthreads[100],readerthreads[100];



unsigned modulusIndex(  char *num, size_t size, unsigned divisor) {
  unsigned rem = 0,temp = 0, i;
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
    int offset = 0,size,temp;


    do {
        // Reading key 
        size = read(fds[index], fkey, sizeof(fkey));

        if (size<=0)        break;
        else temp=1;
        // Reading value         
        size=read(fds[index], fval, sizeof(fval));
        if(size<=0)         break;
        else temp=1;


        // Comparing keys if found same we will update the key on offset and copy the value in value incase of get call 
      if(strcmp(key,fkey)  ==0 ) {

            if(value!=0) 
            {
                // copy key's value 
                memcpy(value, fval, sizeof(fval));

            }
            else{
            }

            return offset;
        }
        offset += sizeof(fval)+sizeof(fkey);
    } while (1);
    temp=1;
    return -1;
}
int file_get(char *key, char *value)
{   
    int index=modulusIndex(key,256,setSize),temp;
    int flag =0;
    sem_wait(&readingLock[index]);
    // Reader started reading 
    readCounters[index]+=1;
    
    // Readers are there lock the mutex
    if(readCounters[index]==1)
        sem_wait(&mutex[index]);
   
   
    sem_post(&readingLock[index]);
    
    if(file_search(key, value, index)>=0) {
         flag = 1;
    }
    
    sem_wait(&readingLock[index]);

    readCounters[index]-=1;
    
    if(readCounters[index]==0)
        sem_post(&mutex[index]);
    else
        temp=1;
    sem_post(&readingLock[index]);
    return flag;
}

int file_del( char *key)
{   
    // flag variable 
    long long int flag=0,l=10;
    // finding index of the key
    int offset,index=modulusIndex(key,256,setSize);
    char fkey[256] ,fval[256];

    // Locking the mutex 
    sem_wait(&mutex[index]);
    

    //looking for offset 
    offset = file_search(key, NULL, index);
    if(!(offset<0)) {
        
        lseek(fds[index], offset, SEEK_SET);
        // setting value to all zeros
        char data[256];
        read(fds[index], data, sizeof(fkey));
        lseek(fds[index], offset, SEEK_SET);
        memset(fkey, 0, 256);
        flag = 1;
        lseek(fds[index], offset, SEEK_SET);
        read(fds[index], data, sizeof(fkey));
        memset(fval, 0, 256);
        while(l>0)
            l--;    
        lseek(fds[index], offset, SEEK_SET);

        write(fds[index], fkey, 256);
        lseek(fds[index], offset, SEEK_SET);

        write(fds[index], fval, 256);
        flag = 1;
    }
    
    sem_post(&mutex[index]);
    return flag;
    
}

int storage_init()
{
    readCounters=(int *) malloc(sizeof(int)*setSize);
    //  Creating Array of File Descriptors
    fds=(int*)malloc(sizeof(int)*setSize);

    // Creating array of read locks
    readingLock=(sem_t *) malloc(sizeof(sem_t)*setSize);


            mutex=(sem_t *)malloc(sizeof(sem_t)*setSize);
    char nameOfFile[22+setSize];
    long long int i=0;
    while(i<setSize)
    {
        int temp=0;
        readCounters[i]=0;
        snprintf(nameOfFile,sizeof(nameOfFile),"data%d.txt",i);
        if(temp)
            temp=1;
        else
            temp=0;
        temp++;

        fds[i]=open(nameOfFile, O_CREAT|O_RDWR,S_IRWXU);
        
        if(fds[i]<0)std::cout<<("\n[Error : Cannot open File%d.txt]\n",i);
        else
            temp=1;

        //Initializing semaphores 
        sem_init(&readingLock[i],0,1);

        //Initializing mutexes 
        sem_init(&mutex[i],0,1);
        i=i+1;
    }
    return 0;
}
void file_put(char *key,char *value) {
     /*
   
        if found then ask the offset where it is present and if the value noy matches with the present value ,update the given line
        if not present then search for empty line and insert there!
     */
    int offset,temp=0;
    int index=modulusIndex(key,256,setSize);
    temp=0;
    sem_wait(&mutex[index]);

    offset = file_search(key, NULL, index);

    // if we got nothing in the existing set
    if(offset < 0) {
        //adding value at the end of file
        lseek(fds[index], 0, SEEK_END);
        write(fds[index], key, 256);
        temp=1;
        write(fds[index], value, 256);

    } else {
        //adding value at returned offset
        lseek(fds[index], offset, SEEK_SET);
        write(fds[index], key, 256);
        temp=1;
        write(fds[index], value, 256);

    }
    sem_post(&mutex[index]);

}


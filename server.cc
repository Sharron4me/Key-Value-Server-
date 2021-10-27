#ifndef METEO_GRPC_SERVER_H
#define METEO_GRPC_SERVER_H

#include "threadpool.h"
#include <functional>
#include <stdexcept>
#include <fstream>
#include <algorithm>
#include <memory>
#include <iostream>
#include <cmath>
#include <string>

#include "assert.h"
#include "cache.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <error.h>
#include <pthread.h>
#include <grpc++/grpc++.h>

#include "kv.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerAsyncWriter;
using grpc::ServerAsyncReader;
using grpc::ServerAsyncReaderWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using helloworld::GetRequest;
using helloworld::GetReply;
using helloworld::PutRequest;
using helloworld::PutReply;
using helloworld::DelRequest;
using helloworld::DelReply;
using helloworld::KeyValueStore;

using namespace std;
pthread_mutex_t lock1;

int tech=0;
string port="2021";
ofstream MyFile("server.logs");
struct configuration
{
   int cacheSize, threadPSize,listeningPort;
   std::string cacheRType;
} settings; 

cache c;
std::string getFromMap(std::string key)
{
// Key is not present
 	MyFile <<"Get "<<key;
	if(!(c.get_entry(key).empty()))
	{
		string ans= c.get_entry(key);
		MyFile<<" Succesfull"<<endl;
		return ans;
	}
	else{
		char val[256];
        char *temp=toCharArray(key);
        if(file_get(temp,val)){
			free(temp);
			MyFile<<" Succesfull"<<endl;
			return toString(val);
		}

		MyFile<<" Failed"<<endl;
		return "KEY NOT EXIST";
	}
}

std::string putIntoMap(std::string key ,std::string value){
	MyFile <<"Put "<<key;
	if(c.put_into_cache(key,value,tech)){
		MyFile<<" Succesfull"<<endl;
		return "Key: "+key+" Added Successfully";
	}
	MyFile<<" Failed"<<endl;
	return "Error Occured While Adding Key : "+key;
}

std::string deleteFromMap(std::string key)
{
	MyFile <<"Delete"<<key;
	if(c.delete_key(key)){
		MyFile<<" Succesfull"<<endl;
		return "Deleted Key : "+key;
	}
	MyFile<<" Failed"<<endl;
	return "KEY NOT EXIST";
}


class CommonCallData
{
	public:
    // The means of communication with the gRPC runtime for an asynchronous
    // server.
    KeyValueStore::AsyncService* service_;
    // The producer-consumer queue where for asynchronous server notifications.
    ServerCompletionQueue* cq_;
    // Context for the rpc, allowing to tweak aspects of it such as the use
    // of compression, authentication, as well as to send metadata back to the
    // client.
    ServerContext ctx_;
    // What we get from the client.
    
	// Let's implement a tiny state machine with the following states.
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;  // The current serving state.

    public:
	explicit CommonCallData(KeyValueStore::AsyncService* service, ServerCompletionQueue* cq):
						service_(service), cq_(cq),status_(CREATE)
	{}

	virtual ~CommonCallData()
	{
//		std::cout << "CommonCallData destructor" << std::endl;
	}

	virtual void Proceed(bool = true) = 0;
};

class CallData: public CommonCallData
{
    ServerAsyncResponseWriter<GetReply> responder_;
	public:
    GetRequest request_;
    GetReply reply_;

	CallData(KeyValueStore::AsyncService* service, ServerCompletionQueue* cq):
		CommonCallData(service, cq), responder_(&ctx_){Proceed();}

	virtual void Proceed(bool = true) override
	{
		if (status_ == CREATE)
		{
//			std::cout << "GetKey Value Function " << std::endl;
	        status_ = PROCESS;
	        service_->RequestGetValue(&ctx_, &request_, &responder_, cq_, cq_, this);
      	}
		else if (status_ == PROCESS)
		{
	        new CallData(service_, cq_);
//			std::cout << "key = " << request_.key() << std::endl;
       		reply_.set_value(getFromMap(request_.key()));
			if(!getFromMap(request_.key()).compare("KEY NOT EXIST"))
			{
				reply_.set_status(400);
			}
			else
			{
				reply_.set_status(200);
			}
	        status_ = FINISH;
    	    responder_.Finish(reply_, Status::OK, this);
      	}
		else
		{
        	GPR_ASSERT(status_ == FINISH);
//			std::cout << " Get Function Done" << std::endl;
    	    delete this;
		}
	}
};


class CallData1M: public CommonCallData
{
    ServerAsyncResponseWriter<PutReply> responder_;
	
public:
    PutRequest request_;
    PutReply reply_;
	CallData1M(KeyValueStore::AsyncService* service, ServerCompletionQueue* cq):
		CommonCallData(service, cq), responder_(&ctx_){Proceed();}
	virtual void Proceed(bool = true) override
	{
		if(status_ == CREATE)
		{
//			std::cout << "PutKeyValue Function" << std::endl;
			service_->RequestPutKValue(&ctx_, &request_, &responder_, cq_, cq_, this);
			status_ = PROCESS ;
		}
		else if(status_ == PROCESS)
		{
			new CallData1M(service_, cq_);
//			std::cout << "Key:" << request_.key()<<"Value: "<<request_.value() << std::endl;
			reply_.set_message(putIntoMap(request_.key(),request_.value()));
	        if(!putIntoMap(request_.key(),request_.value()).compare("Error Occured While Adding Key"))
			{
				reply_.set_status(400);
			}
			else
			{
				reply_.set_status(200);
			}
			status_ = FINISH;
    	    responder_.Finish(reply_, Status::OK, this);
      	}
		else
		{
        	GPR_ASSERT(status_ == FINISH);
//s			std::cout << "PutKeyValue Function Done" << std::endl;
		
    	    delete this;
		}
	}
};

class CallDataMM: public CommonCallData
{
    ServerAsyncResponseWriter<DelReply> responder_;
	
public:
    DelRequest request_;
    DelReply reply_;
	CallDataMM(KeyValueStore::AsyncService* service, ServerCompletionQueue* cq):
		CommonCallData(service, cq), responder_(&ctx_){Proceed();}
	virtual void Proceed(bool = true) override
	{ 
		if(status_ == CREATE)
		{
//			std::cout << "DelKeyValue Function" << std::endl;
			service_->RequestDelKValue(&ctx_, &request_, &responder_, cq_, cq_, this);
			status_ = PROCESS ;
		}
		else if(status_ == PROCESS)
		{
				new CallDataMM(service_, cq_);
//				std::cout << "Key:" << request_.key()<< std::endl;
			    reply_.set_message(deleteFromMap(request_.key()));
			if(!deleteFromMap(request_.key()).compare("Could Not Delete Key"))
				{
					reply_.set_status(400);
				}
				else
				{
					reply_.set_status(200);
				}
	        status_ = FINISH;
    	    responder_.Finish(reply_, Status::OK, this);
      	}
		else
		{
        	GPR_ASSERT(status_ == FINISH);
//			std::cout << "DelKeyValue Function Done" << std::endl;
    	    delete this;
		}
	}
};





class ServerImpl
{
public:
	~ServerImpl()
	{
	    server_->Shutdown();
   		 // Always shutdown the completion queue after the server.
   		cq_->Shutdown();
  	}

    void Run(threadpool *pool)
    {
        std::string server_address("0.0.0.0:"+port);

        ServerBuilder builder;
        // Listen on the given address without any authentication mechanism.
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        // Register "service_" as the instance through which we'll communicate with
        // clients. In this case it corresponds to an *asynchronous* service.
        builder.RegisterService(&service_);
        // Get hold of the completion queue used for the asynchronous communication
        // with the gRPC runtime.
        cq_ = builder.AddCompletionQueue();
        // Finally assemble the server.
        server_ = builder.BuildAndStart();
        std::cout << "Server listening on " << server_address << std::endl;

        // Proceed to the server's main loop.
        // Spawn a new CallData instance to serve new clients.
        new CallData(&service_, cq_.get());
        new CallData1M(&service_, cq_.get());
        new CallDataMM(&service_, cq_.get());



        void* tag;  // uniquely identifies a request.
        bool ok;
        while(true)
        {
            GPR_ASSERT(cq_->Next(&tag, &ok));
			GPR_ASSERT(ok);
            CommonCallData* calldata = static_cast<CommonCallData*>(tag);
			pool->enqueue([calldata](){calldata->Proceed();});

        }
    }

private:
	std::unique_ptr<ServerCompletionQueue> cq_;
	KeyValueStore::AsyncService service_;
	std::unique_ptr<Server> server_;
};

int readFile()
{
	FILE *fp;
	fp = fopen("client.cc", "r");
	if (fp == NULL)
	{
		cout<<"Error in reading file:"<< errno << std::endl;
        return -1;
	}
	fclose(fp);
	return 0;
	/*
	FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    char *param;
    char *value;
    int i;

    fp = fopen("settings.conf", "r");
    if (fp == NULL)
	{
		cout<<"Error in reading file:"<< errno << std::endl;
        return -1;
	}
    while ((read = getline(&line, &len, fp)) != -1) {
        i = 0;
        if(line[0]=='#') {
            break;
        } else {
            param=strtok(line,"=");
            value=strtok(NULL,"=");
            if(strcmp(param,"LISTENING_PORT")==0) {

                settings.listeningPort = atoi(value);
            }
			else if (strcmp(param,"CACHE_REPLACEMENT_TYPE")==0) {
                settings.cacheRType = atoi(value);
            }
			else if (strcmp(param,"CACHE_SIZE")==0) {
                settings.cacheSize = atoi(value);
            } else if (strcmp(param,"THREAD_POOL_SIZE")==0) {
                settings.threadPSize = atoi(value);
            } else {

            }
        }
    }

    fclose(fp);
   */
}
int main(int argc, char* argv[])
{
	ifstream file("server.config");
	string str1,str2,str3; 
	int k=0;
	getline(file, str1);
	port = str1;
	getline(file, str2);
	if(!str2.compare("LRU")){
		tech=1;
	}
	getline(file, str2);
	int cache_size=stoi(str2);	
	getline(file, str2);
	int num_threads=stoi(str2);

	ServerImpl server;

	//readFile();

	c.initialize_cache(cache_size);
	threadpool pool{num_threads};
    server.Run(&pool);
	MyFile.close();
}
#endif

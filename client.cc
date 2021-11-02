
#ifndef METEO_GRPC_CLIENT_H
#define METEO_GRPC_CLIENT_H

#include <functional>
#include <stdexcept>
#include <bits/stdc++.h>
#include <memory>
#include <fstream>
#include <iostream>
#include <cmath>
#include "assert.h"

#include <grpc++/grpc++.h>
#include <thread>

#include "kv.grpc.pb.h"
#include <chrono>

using namespace std::chrono;

using grpc::Channel;
using grpc::ChannelArguments;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::ClientAsyncReader;
using grpc::ClientAsyncWriter;
using grpc::ClientAsyncReaderWriter;
using grpc::CompletionQueue;
using grpc::Status;
using helloworld::GetRequest;
using helloworld::GetReply;
using helloworld::PutRequest;
using helloworld::PutReply;
using helloworld::DelRequest;
using helloworld::DelReply;
using helloworld::KeyValueStore;
#include <chrono>
using namespace std::chrono;

using namespace std;

time_t start;
time_t end;

class AbstractAsyncClientCall
{
public:
	enum CallStatus { PROCESS, FINISH, DESTROY };

	explicit AbstractAsyncClientCall():callStatus(PROCESS){}
	virtual ~AbstractAsyncClientCall(){}
	ClientContext context;
	Status status;
	CallStatus callStatus ;

	virtual void Proceed(bool = true) = 0;
};

class AsyncClientCall: public AbstractAsyncClientCall
{
	std::unique_ptr< ClientAsyncResponseReader<GetReply> > responder;
public:
    GetReply reply; 
	AsyncClientCall(const GetRequest& request, CompletionQueue& cq_, std::unique_ptr<KeyValueStore::Stub>& stub_):AbstractAsyncClientCall()
	{
//		std::cout << "Get Key" << std::endl;
	    responder = stub_->AsyncGetValue(&context, request, &cq_);
	    responder->Finish(&reply, &status, (void*)this);
		callStatus = PROCESS ;
	}
	virtual void Proceed(bool ok = true) override
	{
		if(callStatus == PROCESS)
		{
        // Verify that the request was completed successfully. Note that "ok"
        // corresponds solely to the request for updates introduced by Finish().
	        GPR_ASSERT(ok);
			if(status.ok())
			{
			    std::cout << "Value received: " <<reply.value()<<"Status:"<<reply.status() << std::endl;
								// time(&end);

			}
			delete this;
		}
	}
};

class AsyncClientCall1M : public AbstractAsyncClientCall
{

	std::unique_ptr< ClientAsyncResponseReader<PutReply> > responder;
public:
    PutReply reply;
	AsyncClientCall1M(const PutRequest& request, CompletionQueue& cq_, std::unique_ptr<KeyValueStore::Stub>& stub_):AbstractAsyncClientCall()
	{
//		std::cout << "Put Key" << std::endl;
	    responder = stub_->AsyncPutKValue(&context, request, &cq_);
        responder->Finish(&reply, &status, (void*)this);
		callStatus = PROCESS ;
	}
	virtual void Proceed(bool ok = true) override
	{
        if(callStatus == PROCESS)
		{
        // Verify that the request was completed successfully. Note that "ok"
        // corresponds solely to the request for updates introduced by Finish().
	        GPR_ASSERT(ok);
			if(status.ok())
			{
                std::cout << "Message received: " <<reply.message_() <<"Status:"<<reply.status()<< std::endl;
								// time(&end);

			}
			delete this;
		}
	}
};

class AsyncClientCallMM : public AbstractAsyncClientCall
{

	std::unique_ptr< ClientAsyncResponseReader<DelReply> > responder;
public:
    DelReply reply;
	AsyncClientCallMM(const DelRequest& request, CompletionQueue& cq_, std::unique_ptr<KeyValueStore::Stub>& stub_):AbstractAsyncClientCall()
	{
//		std::cout << "Delete Key" << std::endl;
	    responder = stub_->AsyncDelKValue(&context, request, &cq_);
        responder->Finish(&reply, &status, (void*)this);
		callStatus = PROCESS ;
	}
	virtual void Proceed(bool ok = true) override
	{
        if(callStatus == PROCESS)
		{
        // Verify that the request was completed successfully. Note that "ok"
        // corresponds solely to the request for updates introduced by Finish().
	        GPR_ASSERT(ok);
			if(status.ok())
			{
				std::cout << "Message received: " <<reply.message_()<<"Status:"<<reply.status() << std::endl;
				// time(&end);

			}
			delete this;

		}
	}
};
class KVClient
{
public:
    explicit KVClient(std::shared_ptr<Channel> channel)
            :stub_(KeyValueStore::NewStub(channel))
	{}

    void GetValue(const std::string& key) {    
    // Data we are sending to the server.
    GetRequest request;
    request.set_key(key);	// Assembles the client's payload and sends it to the server.
    new AsyncClientCall(request, cq_, stub_);
    }

    void PutKValue(const std::string& key,const std::string& value)
    {
         // Data we are sending to the server.
    PutRequest request;
    request.set_key(key);	// Assembles the client's payload and sends it to the server.
    request.set_value(value);
    new AsyncClientCall1M(request, cq_, stub_);
    }

    void DelKValue(const std::string& key)
    {
         // Data we are sending to the server.
    DelRequest request;
    request.set_key(key);	// Assembles the client's payload and sends it to the server.
    new AsyncClientCallMM(request, cq_, stub_);
    }
    
	void AsyncCompleteRpc()
	{
		void* got_tag;
    	bool ok = false;
		while(cq_.Next(&got_tag, &ok))
		{
        	AbstractAsyncClientCall* call = static_cast<AbstractAsyncClientCall*>(got_tag);
			call->Proceed(ok);
    	}
		std::cout << "Completion queue is shutting down." << std::endl;
	}

private:
    // Out of the passed in Channel comes the stub, stored here, our view of the
    // server's exposed services.
    std::unique_ptr<KeyValueStore::Stub> stub_;

    // The producer-consumer queue we use to communicate asynchronously with the
    // gRPC runtime.
    CompletionQueue cq_;
};
// for string delimiter
vector<string> split (string s, string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> res;

    while ((pos_end = s.find (delimiter, pos_start)) != string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}
 
int main(int argc, char* argv[])
{

  
	  ChannelArguments args;
 	 // Set the load balancing policy for the channel.
  	args.SetLoadBalancingPolicyName("round_robin");
	KVClient client(grpc::CreateCustomChannel("localhost:50051", grpc::InsecureChannelCredentials(),args));
	std::thread thread_ = std::thread(&KVClient::AsyncCompleteRpc, &client);
	
	std::string userInput;
	int mode;
	cout<<"1.Batch Mode 2.Interactive mode"<<endl;
	cin>>mode;
	if(mode==1)
	{

		double totalB=0;
		fstream newfile;
		newfile.open("batch.txt",ios::in); //open a file to perform read operation using file object
		int numB=0;
  		if (newfile.is_open())
		  {   //checking whether the file is open
      string tp;
	//   cout<<"Reached here"<<endl;
      while(getline(newfile, tp))
	  { //read data from file object and put it into string.
		std::string delimiter = " ";
    	std::vector<std::string> v = split (tp, delimiter);

	string func=v.front();
    
	if(!func.compare("Get"))
	{
		numB++;
		v.erase(v.begin());
		string key=v.front();  
    // unsync the I/O of C and C++.
    	ios_base::sync_with_stdio(false);
		auto start = high_resolution_clock::now();	
		client.GetValue(key);
		auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);
		totalB+=duration.count();
		cout<<"Get Time:"<<duration.count()<< " microseconds"<<endl;
    

	}
	else if(!func.compare("Put"))
	{
		numB++;
		v.erase(v.begin());
		string key=v.front();
		v.erase(v.begin());
		string value=v.front();
  
    // unsync the I/O of C and C++.
    ios_base::sync_with_stdio(false);	
	auto start = high_resolution_clock::now();		
	client.PutKValue(key,value);
	auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
	totalB+=duration.count();
	cout<<"Put Time:"<<duration.count()<< " microseconds"<<endl;
	}

	else if(!func.compare("Del"))
	{
		numB++;
		v.erase(v.begin());
		string key=v.front();
  
    // unsync the I/O of C and C++.
    ios_base::sync_with_stdio(false);	
	auto start = high_resolution_clock::now();		
	client.DelKValue(key);
	auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
	totalB+=duration.count();
	cout<<"Delete Time:"<<duration.count()<< " microseconds"<<endl;

	}
	    }
	  
      newfile.close(); //close the file object.
	  cout<<"Requests Batch:"<<numB<<endl;
	  cout<<"Total Time Batch:"<<totalB<<endl;
	  cout<<"Average Time Batch:"<<double(totalB/numB)<<endl;
   }
   	  
	}
	else if(mode==2)
	{
	int numI=0;
	double totalI=0;
	std::cout<<"1.Get Key 2.Put Key Value 3.Del Key"<<std::endl;
	std::cout << "Press control-c to quit" << std::endl << std::endl;
	while(1)
	{
	getline(std::cin,userInput);
std::string delimiter = " ";
    std::vector<std::string> v = split (userInput, delimiter);

	string func=v.front();
	if(!func.compare("Get"))
	{
		numI++;
		v.erase(v.begin());
		string key=v.front();  
    // unsync the I/O of C and C++.
    ios_base::sync_with_stdio(false);	
		auto start = high_resolution_clock::now();	
		client.GetValue(key);
		auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);
		totalI+=duration.count();
		cout<<"Get Time:"<<duration.count()<< " microseconds"<<endl;
			
		}
	else if(!func.compare("Put"))
	{
		numI++;
		v.erase(v.begin());
		string key=v.front();
		v.erase(v.begin());
		string value=v.front();
  
    // unsync the I/O of C and C++.
    ios_base::sync_with_stdio(false);		
	auto start = high_resolution_clock::now();		
	client.PutKValue(key,value);
	auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
	totalI+=duration.count();
	cout<<"Put Time:"<<duration.count()<< " microseconds"<<endl;
	
	}
	else if(!func.compare("Del"))
	{
		numI++;
		v.erase(v.begin());
		string key=v.front();
  
    // unsync the I/O of C and C++.
    ios_base::sync_with_stdio(false);
auto start = high_resolution_clock::now();		
	client.DelKValue(key);
	auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
	totalI+=duration.count();
	cout<<"Delete Time:"<<duration.count()<< " microseconds"<<endl;	

	}
	else if(!func.compare("Exit"))
	{
		cout<<"Requests Interactive:"<<numI<<endl;
		cout<<"Total Time Interactive:"<<totalI<<endl;
		cout<<"Average Time Interactive:"<<double(totalI/numI)<<endl;
		break;
	}	
	}
	
	}
	std::cout << "Press control-c to quit" << std::endl << std::endl;
  
	thread_.join();

}


#endif

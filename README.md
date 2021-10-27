1)To run to project do the following :
$ mkdir -p cmake/build
$ pushd cmake/build
$ cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../..
$ make -j

2)server.config is to placed in cmake/build in the following manner (or Copy server.config to cmake/build)  :
50051					//LISTENING_PORT
LRU						//CACHE_REPLACEMENT_TYPE
15						//CACHE_SIZE
10						//THREAD_POOL_SIZE

For example : 
50051					
LRU						
15						
10	

3)For using Batch Mode Place batch.txt file in cmake/build , It may contain Command in the following (case sensitive Manner):
Get key
Put key value
Del key

4)For using Iterative mode , type the commands in case sensetive manner :

Get key
Put key value
Del key

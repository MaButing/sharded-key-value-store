#include <vector>
#include <iostream>
#include <fstream>
#include <string>

#include "shard.h"
#include "shard_info.h"

using namespace std;

int main(int argc, char const *argv[])
{
	if (argc != 3){
		cerr<<"usage: "<<argv[0]<<" <replica_id> <shard_id>"<<endl;
		return 0;
	}
	
	int replica_id = stoi(argv[1]);
	size_t shard_id = stoull(argv[2]);

	shard_info_t shard_info = config2info(shard_id);

	//start server
	shard server(shard_info.n, shard_info.f, replica_id, 0, shard_info.begin, shard_info.end);

	if (server.repl_init(shard_info.addr_list)!=0){
		cerr << "[Error, replica init fail]"<<endl;
		return -1;
	}

	server.repl_run();

	server.repl_close();



	return 0;
}
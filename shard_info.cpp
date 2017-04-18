#include "shard_info.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>


using namespace std;

shard_info_t config2info(size_t shard_id){


	string configFile_name = "shard_"+to_string(shard_id)+".config";
	ifstream configFile(configFile_name.c_str());


	shard_info_t shard;

	configFile >> shard.begin >> shard.end;
	configFile >> shard.n >> shard.f >> shard.king;

	shard.addr_list.resize(shard.n);
	memset(shard.addr_list.data(), 0, shard.addr_list.size()*sizeof(sockaddr_in));

	
	for (int i = 0; i < shard.n; ++i)
	{
		string ip_str; int port;
		configFile >> ip_str >>port;
		// cout << ip_str <<":" << port<<endl;

		shard.addr_list[i].sin_family = AF_INET;
    	shard.addr_list[i].sin_port = htons(port);
    	inet_pton(AF_INET, ip_str.c_str(), &(shard.addr_list[i].sin_addr));
	}

	return shard;
}

// int main(int argc, char const *argv[])
// {
// 	size_t id = stoull(argv[1]);

// 	config2info(id);

// 	return 0;
// }
int master::master_init(const shard_info_t& initShard)
{
	if (initShard.begin != 0 || initShard.end != size_t(-1))
		return -1;
	

	shardList[initShard.end] = initShard;



	sockaddr_in masterAddr;
	memset(&masterAddr, 0, sizeof(sockaddr_in));
	masterAddr.sin_family = AF_INET;
	masterAddr.sin_port = htons(master_port);
	inet_pton(AF_INET, master_ip_str.c_str(), &(masterAddr.sin_addr));

	std::vector<sockaddr_in> v;
	v.push_back(masterAddr);

	return x = comm.comm_init(v);
}

int master::master_run();


int master_send(const string& str, size_t shard_id){
	
}



int master::master_close()
{
	comm.comm_close();
}
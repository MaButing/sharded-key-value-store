#include "master.h"
#include <cassert>
#include <iostream>

using namespace std;

int master::master_init(const shard_info_t& initShard)
{
	cerr << initShard.begin<<" "<<initShard.end<<" "<<size_t(-1)<<endl;
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

	return comm_client.comm_init(v);
}

int master::master_run()
{
	char buff[MAXBUFFSIZE];
    int source_id;
    while(1){
	    if (comm_client.comm_recv(&source_id, (void*)buff, MAXBUFFSIZE) > 0){
	    	string str(buff);
	    	// CLIENTREQ:<id>:<seq>:<op.str()>

	    	size_t pos0 = str.find(":");
			if (str.substr(0,pos0) != "CLIENTREQ") continue;

            thread t(master::handle_helper, this, str);
        	t.detach();
        }
        else{
        	cout << "recving client request error."<<endl;
        	continue;
        }
	}
}

int master::handle_helper(master* m, const string& str)
{
	return m->master_handle(str);
}


int master::master_handle(const string& str)
{

	// CLIENTREQ:<client_id>:<client_seq>:<op_str>
	size_t pos0 = str.find(":");
	if (str.substr(0,pos0) != "CLIENTREQ") return -1;

	//parse request string
	size_t pos1 = str.find(":", pos0+1);
	size_t client_id = stoi(str.substr(pos0+1, pos1-pos0-1));

	size_t pos2 = str.find(":", pos1+1);
	size_t client_seq = stoi(str.substr(pos1+1, pos2-pos1-1));

	string op_str = str.substr(pos2+1);

	//set up communicator to shard
	communicator comm_shard(1,0);

	sockaddr_in destaddr;
    memset(&destaddr, 0, sizeof(sockaddr_in));
    destaddr.sin_family = AF_INET;
    inet_pton(AF_INET, master_ip_str.c_str(), &(destaddr.sin_addr));


	int port;
    while (1){
		std::unique_lock<std::mutex> L(m);
		port = ++temp_port;
		if (temp_port > master_port+1000) temp_port = master_port+1;
		L.unlock();

	    destaddr.sin_port = htons(port);

	    std::vector<sockaddr_in> v(1, destaddr);

	    if (comm_shard.comm_init(v) == 0) break;
	}

	// prepare request string
	request_t req;
	req.client_id = client_id;
	req.client_seq = client_seq;
	req.client_ip_str = master_ip_str;
	req.client_port = port;
	req.msg = op_str;
	string req_str = "MASTERREQ:"+req.str();

	//communicate to shard
	op_t op(op_str);
	string key = op.content.substr(0, op.content.find(':'));// <begin> if op.type == CUT
	

	string reply_str;
	request_t reply_req;
	op_t reply_op;
	resp_t reply_resp;

	do{
		size_t shard_id = key2shard(key);

		std::unique_lock<std::mutex> L(m);
		shard_info_t shard_info = shardList[shard_id];
		L.unlock();

		int timeout_sec = 5;

		char buff[MAXBUFFSIZE];
    	int source_id;

    	// send to shard and wait for response
		while (1){
			
			comm_shard.comm_sendOut(shard_info.addr_list[shard_info.king], 
				req_str.c_str(), req_str.size()+1);
			
			int x = comm_shard.comm_recv(&source_id, (void*)buff, MAXBUFFSIZE, timeout_sec);

			if (x <= 0){
				timeout_sec *=2;
				
				std::unique_lock<std::mutex> L(m);
					if (++shard_info.king >= shard_info.n)
						shard_info.king = 0;
					shardList[shard_id].king = shard_info.king;
				L.unlock();
				
				continue;
			}
		}

		reply_str = string(buff);

		//process reply
		size_t pos0 = reply_str.find(':');
		assert(reply_str.substr(0, pos0) == "REPLY");
		// if (reply_str.substr(0, pos0) != "REPLY")
		// 	continue;
		size_t pos1 = reply_str.find(':', pos0+1);
		assert(shard_id == stoull(reply_str.substr(pos0+1, pos1-pos0-1)));
		size_t pos2 = reply_str.find(':', pos1+1);
		int new_king = stoi(reply_str.substr(pos1+1, pos2-pos1-1));

		if (new_king != shard_info.king){
			std::unique_lock<std::mutex> L(m);
				shardList[shard_id].king = new_king;
			L.unlock();
		}

		reply_req = request_t(reply_str.substr(pos2+1));

		size_t pos = reply_req.msg.find('|');
		assert(pos != string::npos);
		reply_op = op_t(reply_req.msg.substr(0,pos));
		reply_resp = resp_t(reply_req.msg.substr(pos+1));

	}while(reply_resp.code == -2); // key out of range

	assert(reply_op == op);


	if (op.type != "CUT"){//reply to client
		comm_shard.comm_sendOut(op.ip_str, op.port,
			reply_str.c_str(), reply_str.size()+1);
	}
	else{// op.type == "CUT", addShard process
		if (reply_resp.code != 0){
			comm_shard.comm_sendOut(op.ip_str, op.port,
				reply_str.c_str(), reply_str.size()+1);
		}else{
			size_t shard_id = stoull(op.content.substr(op.content.find(':')+1));

			shard_info_t shard_info = config2info(shard_id);//TODO
			reply_str = initShard(shard_info, reply_req);
			comm_shard.comm_sendOut(op.ip_str, op.port,
				reply_str.c_str(), reply_str.size()+1);
		}
	}


	comm_shard.comm_close();
}


string master::initShard(shard_info_t shard_info, request_t req)//pass by value
{
	//add shard to shardList
	size_t shard_id = shard_info.end;

	std::unique_lock<std::mutex> L(m);
		shardList[shard_id] = shard_info;
	L.unlock();


	//set up communicator to shard
	communicator comm_shard(1,0);

	sockaddr_in destaddr;
    memset(&destaddr, 0, sizeof(sockaddr_in));
    destaddr.sin_family = AF_INET;
    inet_pton(AF_INET, master_ip_str.c_str(), &(destaddr.sin_addr));


	int port;
    while (1){
		std::unique_lock<std::mutex> L(m);
		port = ++temp_port;
		if (temp_port > master_port+1000) temp_port = master_port+1;
		L.unlock();

	    destaddr.sin_port = htons(port);

	    std::vector<sockaddr_in> v(1, destaddr);

	    if (comm_shard.comm_init(v) == 0) break;
	}

	//prepare request
	size_t pos = req.msg.find('|');
	assert(pos != string::npos);
	op_t op(req.msg.substr(0,pos));
	resp_t resp(req.msg.substr(pos+1));

	op.type = "INIT";
	op.content = resp.content;

	req.client_port = port;
	req.msg = op.str();
	string req_str = "MASTERREQ:"+req.str();


	//communicate to shard
	


	int timeout_sec = 5;

	

	// send to shard and wait for response
	char buff[MAXBUFFSIZE];
	int source_id;
	while (1){
		
		comm_shard.comm_sendOut(shard_info.addr_list[shard_info.king], 
			req_str.c_str(), req_str.size()+1);
		
		int x = comm_shard.comm_recv(&source_id, (void*)buff, MAXBUFFSIZE, timeout_sec);

		if (x <= 0){
			timeout_sec *=2;
			
			std::unique_lock<std::mutex> L(m);
				if (++shard_info.king >= shard_info.n)
					shard_info.king = 0;
				shardList[shard_id].king = shard_info.king;
			L.unlock();
			
			continue;
		}
	}

	string reply_str(buff);

	//process reply
	size_t pos0 = reply_str.find(':');
	assert(reply_str.substr(0, pos0) == "REPLY");
	// if (reply_str.substr(0, pos0) != "REPLY")
	// 	continue;
	size_t pos1 = reply_str.find(':', pos0+1);
	assert(shard_id == stoull(reply_str.substr(pos0+1, pos1-pos0-1)));
	size_t pos2 = reply_str.find(':', pos1+1);
	int new_king = stoi(reply_str.substr(pos1+1, pos2-pos1-1));

	if (new_king != shard_info.king){
		std::unique_lock<std::mutex> L(m);
			shardList[shard_id].king = new_king;
		L.unlock();
	}

	// request_t reply_req(reply_str.substr(pos2+1));

	// size_t pos = reply_req.msg.find('|');
	// assert(pos != string::npos);
	// op_t reply_op(reply_req.msg.substr(0,pos));
	// resp_t reply_resp(reply_req.msg.substr(pos+1));

	comm_shard.comm_close();

	return reply_str;
}

size_t master::key2shard(const string& key)
{
	size_t hash = hash_fn(key);
	size_t shard_id;
	std::unique_lock<std::mutex> L(m);
	for (const auto& pair: shardList)
		if (pair.first > hash){
			shard_id = pair.first;
			break;
		}
	L.unlock();

	return shard_id;
}


int master::master_close()
{
	return comm_client.comm_close();
}
#include <map>
#include <vector>
#include <mutex>
#include <thread>
#include <functional>

#include "ReqOrd.h"
#include "shard_info.h"
#include "communicator.h"

// struct record_t
// {
// 	size_t shard_id;
// 	op_t op;
// };

// PUT:<key>:<value>
// GET:<key>
// DEL:<key>
// CUT:<begin>:<end>
// INIT:{<key>:<value>}

class master
{
private:
	//map shard_id(end) -> shard_info
	std::map<size_t, shard_info_t> shardList;
	//lock on shardList
	std::mutex m;
	// to lock: std::unique_lock<std::mutex> L(m);
	// to unlock: L.unlock();
	std::hash<std::string> hash_fn;

	//optional
	// std::vector<record_t> log;

	string master_ip_str;
	int master_port, temp_port;
	communicator comm_client;

	string initShard(shard_info_t shard_info, request_t req);

	size_t key2shard(const string& key);

public:
	master(const string& ip_str, int port):
	master_ip_str(ip_str),
	master_port(port)
	// comm(1, 0) //default
	{temp_port = master_port+1;};
	int master_init(const shard_info_t& initShard);
	int master_run();
	int master_handle(const string& str);
	int master_close();

	static int handle_helper(master *m, const string& str);
};




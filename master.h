#include <map>
#include <vector>
#include "ReqOrd.h"
#include <mutex>
#include <thread>

struct shard_info_t
{

	size_t begin, end;//responsible for key in [begin,end], end is where is shard locate.
	int n, king;//king is the primary_id
	vector<sockaddr_in> addr_list;
};

struct record_t
{
	size_t shard_id;
	op_t op;
};

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

	//optional
	std::vector<record_t> log;

	string master_ip_str;
	int master_port, temp_port;
	communicator comm_client;

	int master_handle();

	size_t key2shard(const string& key);

public:
	master(const string& ip_str, int port):
	master_ip_str(ip_str),
	master_port(port)
	// comm(1, 0) //default
	{temp_port = master_port+1;};
	~master();
	int master_init(const shard_info_t& initShard);
	int master_run();
	
	int master_close();

	
};



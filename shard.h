#include <string>
#include <map>
#include <functional>
#include "paxos_replica.h"

class shard: public paxos_replica
{
private:
	size_t begin, end;//end also serve as shard_id
	std::map<string, string> table;//storage media
	
	std::hash<std::string> hash_fn;

	int exec(const order_t& ord);
	int reply(const request_t& req, int code);
public:
	shard(int n_in = 1, int f_in = 0, int id_in = 0, int x_in = 0, 
		size_t begin_in = 0, size_t end_in = size_t(-1)):
	paxos_replica(n_in, f_in, id_in, x_in),
	begin(begin_in), end(end_in)
	{};
	int exec(order_t& ord); //overriden
	int reply(const request_t& req);//overriden
};
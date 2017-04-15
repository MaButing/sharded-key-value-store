#include<>

struct shard_info_t
{

	int begin, end;//responsible for key in [begin,end], end is where is shard locate.
	int n, f, king;//king is the primary_id
	vector<sockaddr_in> addr_list;
};


class shard_master_t
{
public:
	shard_master();
	~shard_master();
	int init();
	int put();

	
};



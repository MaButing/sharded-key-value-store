#ifndef __SHARD_INFO_H__
#define __SHARD_INFO_H__


#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <vector>


struct shard_info_t
{

	size_t begin, end;//responsible for key in [begin,end], end is where is shard locate.
	int n, f, king;//king is the primary_id
	std::vector<sockaddr_in> addr_list;
};

shard_info_t config2info(size_t end);

#endif
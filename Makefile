shard:
	g++ shard_main.cpp shard.cpp paxos_replica.cpp communicator.cpp shard_info.cpp -o shard -Wall -Wextra -std=c++11 -C

master:
	g++ master_main.cpp master.cpp communicator.cpp shard_info.cpp -o master -Wall -Wextra -std=c++11 -C -pthread
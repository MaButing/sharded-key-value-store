shard:
	g++ shard_main.cpp shard.cpp paxos_replica.cpp communicator.cpp -o shard -Wall -Wextra -std=c++11 -C
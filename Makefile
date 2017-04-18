shard:
	g++ shard_main.cpp shard.cpp paxos_replica.cpp communicator.cpp shard_info.cpp hash.cpp -o shard -Wall -Wextra -std=c++11 -C;

master:
	g++ master_main.cpp master.cpp communicator.cpp shard_info.cpp hash.cpp -o master -Wall -Wextra -std=c++11 -C -pthread;

client:
	g++ client_main.cpp communicator.cpp -o client -Wall -Wextra -std=c++11 -C

recv:
	g++ recv.cpp communicator.cpp -o recv -std=c++11

send:
	g++ send.cpp communicator.cpp -o send -std=c++11
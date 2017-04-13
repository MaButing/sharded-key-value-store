#include <vector>
#include <iostream>
#include <string>

#include "paxos_replica.h"

using namespace std;

int main(int argc, char const *argv[])
{
	if (argc != 5){
		cerr<<"usage: "<<argv[0]<<" <n> <f> <id> <x>"<<endl;
		return 0;
	}
	
	int n = stoi(argv[1]);
	int f = stoi(argv[2]);
	int id = stoi(argv[3]);
	int x = stoi(argv[4]);

	paxos_replica server(n, f, id, x);


	vector<sockaddr_in> addr_list;
	addr_list.resize(n);
	cerr<<"addr_list size = "<<addr_list.size()<<endl;
	memset(addr_list.data(), 0, addr_list.size()*sizeof(sockaddr_in));

	cout<<"input the ip_addr and port_num for all "<<n<<" replicas: "<<endl;
	for (int i = 0; i < n; ++i)
	{
		string ip_str; int port;
		cin >> ip_str >>port;

		addr_list[i].sin_family = AF_INET;
    	addr_list[i].sin_port = htons(port);
    	inet_pton(AF_INET, ip_str.c_str(), &(addr_list[i].sin_addr));
	}

	if (server.repl_init(addr_list)!=0){
		cerr << "[Error, replica init fail]"<<endl;
		return -1;
	}

	server.repl_run();

	server.repl_close();



	return 0;
}
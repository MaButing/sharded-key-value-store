#include "master.h"
#include <iostream>


using namespace std;
int main(int argc, char const *argv[])
{
	if (argc != 3){
		cout << "usage: "<<argv[0]<<" <master_ip> <master_port>"<<endl;
		return -1;
	}

	cerr<<"HASHLIMIT = "<<HASHLIMIT<<endl;
	shard_info_t shard_info = config2info(HASHLIMIT);

	master m(argv[1], stoi(argv[2]));
	
	if (m.master_init(shard_info) != 0)
		return -1;

	m.master_run();

	m.master_close();
	return 0;
}
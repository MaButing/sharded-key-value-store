#include "communicator.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main(int argc, char const *argv[])
{
	string ip_str = "127.0.0.1";
	int port = 6667;

	sockaddr_in destaddr;
    memset(&destaddr, 0, sizeof(sockaddr_in));
    destaddr.sin_family = AF_INET;
    destaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip_str.c_str(), &(destaddr.sin_addr));

    std::vector<sockaddr_in> v;
    v.push_back(destaddr);

    communicator comm;
    comm.comm_init(v);

    string str;
    while(1){
	    cin>>str;
        comm.comm_sendOut(ip_str, 6666, (void*)str.c_str(), str.size()+1);
	}

	return 0;
}
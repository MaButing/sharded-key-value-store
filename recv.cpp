#include "communicator.h"
#include "ReqOrd.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main(int argc, char const *argv[])
{
	string ip_str = "127.0.0.1";
	int port = 8000;

	sockaddr_in destaddr;
    memset(&destaddr, 0, sizeof(sockaddr_in));
    destaddr.sin_family = AF_INET;
    destaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip_str.c_str(), &(destaddr.sin_addr));

    std::vector<sockaddr_in> v;
    v.push_back(destaddr);

    communicator comm;
    comm.comm_init(v);

    char buff[MAXBUFFSIZE];
    int source_id;
    while(1){
	    if (comm.comm_recv(&source_id, (void*)buff, MAXBUFFSIZE) == 0)
            cout << "timeout"<<endl;
        else
	        cout<<"receiving:"<<buff<<endl;
        
        string str(buff);
        request_t req(str.substr(str.find(':')+1));
        req.msg += "|0:";

        str = "REPLY:65535:0:"+req.str();
        cout<<"sending "<<str<<endl;
        comm.comm_sendOut(req.client_ip_str, req.client_port, str.c_str(), str.size()+1);


	}

	return 0;
}
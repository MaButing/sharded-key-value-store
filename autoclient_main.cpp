#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>

#include "communicator.h"
#include "ReqOrd.h"





using namespace std;
int main(int argc, char const *argv[])
{
	if (argc != 6){
		cout <<"usage: "<<argv[0]<<" <client_ip> <client_port> <client_id> <master_ip> <master_port>"<<endl;
		return -1;
	}

	string ip_str = argv[1];
	int port = stoi(argv[2]);
	int id = stoi(argv[3]);
	string master_ip_str = argv[4];
	int master_port = stoi(argv[5]);

	sockaddr_in destaddr;
    memset(&destaddr, 0, sizeof(sockaddr_in));
    destaddr.sin_family = AF_INET;
    destaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip_str.c_str(), &(destaddr.sin_addr));

    std::vector<sockaddr_in> v(1, destaddr);

    communicator comm;
    if (comm.comm_init(v) < 0)
    	return -1;

    int seq = 0;
    op_t op;
    op.ip_str = ip_str;
    op.port = port;

    string type, content;
    while(1){
    	// cout<<"input <OPERATION> <content>"
    	// cin >> type >> content;

        int a = rand() % 4;
        if (a >= 1){// 1/2
            type = "GET";
            content = "KEY"+to_string(rand()%10);
        }
        else if (a == 0){
            type = "PUT";
            content = "KEY"+to_string(rand()%5)+":"+to_string(rand()%1000);
        }
        

    	op.type = type;
    	op.content = content;

    	string req_str = "CLIENTREQ:"+to_string(id)+":"+to_string(seq)+":"+op.str();
    	cout<<op.type<<" "<<op.content<<endl;
    	comm.comm_sendOut(master_ip_str, master_port, req_str.c_str(), req_str.size()+1);

    	char buff[MAXBUFFSIZE];
        int source_id;
        int x = comm.comm_recv(&source_id, (void*)buff, MAXBUFFSIZE);
        if (x < 0){
        	cout << "comm error."<<endl;
        	continue;
        }

        string reply_str(buff);

        size_t pos = reply_str.find('|');
        resp_t resp(reply_str.substr(pos+1));

        // cout << resp.str() <<endl;
        // -1 DEL|GET non-exist key
		// -2 key out of range 
		// -3 cut out of range 
		// -4 unknown error
        if (resp.code == 0)
        	cout<<"success "<< resp.content <<endl;
        else if (resp.code == -1)
        	cout<<"DEL|GET non-exist key "<< resp.content <<endl;
        else if (resp.code == -2)
        	cout<<"key out of range "<< resp.content <<endl;
        else if (resp.code == -3)
        	cout<<"CUT out of range "<< resp.content <<endl;
        else if (resp.code == -4)
        	cout<<"unknown error "<< resp.content <<endl;
        cout<<endl;

        seq++;
        sleep(1);
    }




    comm.comm_close();
	return 0;
}
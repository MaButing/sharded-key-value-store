#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


#include <vector>
#include <iostream>
#include <string>
#include <limits>

#include "ReqOrd.h"
#include "communicator.h"

using namespace std;

//REQUESTDONE:<king>:<client_id>:<client_seq>
struct response_t
{
	int king, client_id, client_seq;
	response_t(const string& str):
	king(-1), client_id(-1), client_seq(-1)
	{
		int pos0 = str.find(":");
		if (str.substr(0,pos0) != "REQUESTDONE")
			return;

		int pos1 = str.find(":", pos0+1);
		king = stoi(str.substr(pos0+1, pos1-pos0-1));

		int pos2 = str.find(":", pos1+1);
		client_id = stoi(str.substr(pos1+1, pos2-pos1-1));

		client_seq = stoi(str.substr(pos2+1));
	}
};

int main(int argc, char const *argv[])
{
	if (argc != 5){
		cerr<<"usage: "<<argv[0]<<" <client_id> <client_seq(default 0)> <client_ip> <client_port>"<<endl;
		return 0;
	}


	int client_id = stoi(argv[1]);
	int seq = stoi(argv[2]);
	string client_ip_str = argv[3];
	int client_port_num = stoi(argv[4]);

	cout<<"init client: "<<endl;

	sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(sockaddr_in));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(client_port_num);
	inet_pton(AF_INET, client_ip_str.c_str(), &(servaddr.sin_addr));

	int sock_listen = socket(AF_INET, SOCK_STREAM, 0);
	if( sock_listen < 0){
        cerr << "[Error, create listening socket]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }

	if( bind(sock_listen, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
        cerr << "[Error, bind socket]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        close(sock_listen);
        return -1;
    }

    if( listen(sock_listen, SOMAXCONN) == -1){
        cerr << "[Error, listen socket]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        close(sock_listen);
        return -1;
    }



	int n;
	cout<<"input number of replicas: "<<endl;
	cin >> n;

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
	cin.ignore ( numeric_limits<std::streamsize>::max(), '\n' ); 
	int king = 0;

	
	int wait_second = 5;


	string msg_str;
	while(1){
		msg_str.clear();
		cout<<"Enter the message (!q to quit):"<<endl;
		getline(cin, msg_str);
		if (msg_str == "!q") break;

		request_t req;
		req.client_id = client_id;
		req.client_seq = seq;
		req.client_ip_str = client_ip_str;
		req.client_port = client_port_num;
		req.msg = msg_str;

	    for(; king < n; king++){

	    	//send message
	    	cout<<"...Send msg to replica "<<king<<endl;

	    	int sock_send = socket(AF_INET, SOCK_STREAM, 0);
		    if( sock_send < 0){
		        cerr << "[Error, create sending socket]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
		        break;
		    }
		    if( connect(sock_send, (sockaddr*)(addr_list.data()+king), sizeof(sockaddr)) < 0){
		        cerr << "[Error, connect to"<<king<<"]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
		        close(sock_send);
		        continue;// move to next replica
		    }
			//send
		    //CLIENTREQ:<client_id>:<client_seq>:<client_ip>:<client_port>:msg

		    string client_req_str = "CLIENTREQ:"+req.str();
		    //send msg_len
		    size_t msg_len = client_req_str.size()+1+sizeof(size_t)+sizeof(int);
		    int send_len = send(sock_send, &msg_len, sizeof(size_t), MSG_NOSIGNAL);
		    if (send_len < 0){
		        cerr << "[send, send msg_size error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
		        return -1;
		    }
		    //send source_id
		    int source_id = -1*req.client_id;
		    send_len = send(sock_send, &source_id, sizeof(int), MSG_NOSIGNAL);
		    if (send_len < 0){
		        cerr << "[send, send source_id error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
		        return -1;
		    }

		    send_len = send(sock_send, client_req_str.c_str(), client_req_str.size()+1, MSG_NOSIGNAL);
		    if (send_len < 0){
		        cerr << "[send, send msg error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
		        return -1;
		    }
		    
			close(sock_send);



			//wait for respond for time out
			bool replied = false;

			while(1){
				
			    fd_set rfds;
			    FD_ZERO(&rfds);
	            FD_SET(sock_listen, &rfds);

	            timeval to_len = {wait_second, 0};
	            cerr<<"wait for "<<to_len.tv_sec<<" secends"<<endl;

	            int retval = select(sock_listen+1, &rfds, NULL, NULL, &to_len);
	            if (retval == -1){
	                cerr << "[Error, select]:"<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
	                continue;
	            }
	            else if (retval == 0){
	            	cerr << "receive timeout, replica "<<king<<"is down."<<endl;
	                if (wait_second < 60) wait_second *= 2;
	                break; //move to next king
	            }

	            //accept
	            sockaddr_in clientaddr;
	    		socklen_t peer_addr_size = sizeof(struct sockaddr_in);
	    		int sock_recv;
	            if( (sock_recv = accept(sock_listen, (struct sockaddr*)&clientaddr, &peer_addr_size)) == -1){
			        cerr << "[Error, accept socket]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
			        return -1;
			    }
	            //receive

			    char buffer[MAXBUFFSIZE];
			    char *buffer_ptr = buffer;
			    size_t sum_len = 0;
			    msg_len = 0;

			    while(1){
			        int recv_len = recv(sock_recv, buffer_ptr, MAXBUFFSIZE-sum_len, 0);
			        if (recv_len < 0){
			            cerr << "recv error: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
			            break;
			        }
			        if (recv_len == 0) break;
			        
			        sum_len += recv_len;
			        buffer_ptr = buffer_ptr + recv_len;

			        if (msg_len == 0){
			            if ( sum_len > 2*sizeof(uint)){
			                msg_len = *((size_t*) buffer);
			                source_id = *(((int*)buffer)+1);
			                //cerr<<"msg_len: "<<msg_len<<",source_id:"<<*source_id<<endl;
			            }
			        }
			        else{ // msg_len > 0
			            if (sum_len >= msg_len)
			                break;
			        }
			    }

			    //REQUESTDONE:<king>:<client_id>:<client_seq>
			    string res_str(buffer+sizeof(size_t)+sizeof(int));
			    cerr<<res_str<<endl;
			    response_t res(res_str);
			    if (res.client_id == req.client_id && res.client_seq == req.client_seq){
			    	if (res.king > king) king = res.king;
			    	close(sock_recv);
			    	replied = true;
			    	break;
			    }
			    
			    if (replied) break;
			}

		    if (replied) break;
		}


		if (king == n){
			cerr << "system not alive, exit!"<<endl;
			break;
		}
		seq++;
	}


	return 0;
}

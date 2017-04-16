#include "communicator.h"

int communicator::comm_init(const vector<sockaddr_in>& addr_list_in)
{
    if (id < 0 || id >= n){
        cerr<<"[COMM-Error, id out of range]"<<endl;
        return -1;
    }
    if ((size_t)n > addr_list_in.size()){
    	cerr<<"[COMM-Error, addr_list too short]"<<endl;
    	return -1;
    }
    

    addr_list = addr_list_in;

    //open the listening socket
    if( (sock = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
        cerr << "[COMM-Error, create listening socket error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }

    sockaddr_in servaddr = addr_list[id];
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if( bind(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
        cerr << "[COMM-Error, bind socket error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }

    if( listen(sock, SOMAXCONN) == -1){
        cerr << "[COMM-Error, listen socket error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }


    return 0;
}

int communicator::comm_send(int dest_id, void* buff, size_t buff_size) const
{
	
	if (buff_size > MAXSENDSIZE){
        cerr << "[COMM-Error, buff_size error], MAXSENDSIZE is "<<MAXSENDSIZE<<endl;
		return -1;
	}

	int sock_send;
    if( (sock_send = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cerr << "[COMM-Error, create socket error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }
    if( connect(sock_send, (sockaddr*)(addr_list.data()+dest_id), sizeof(sockaddr)) < 0){
        cerr << "[COMM-Error, connect error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }

    //send msg_len
    size_t msg_len = buff_size+sizeof(size_t)+sizeof(int);
    int send_len = send(sock_send, &msg_len, sizeof(size_t), MSG_NOSIGNAL);
    if (send_len < 0){
        cerr << "[COMM-Error, send msg_size error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }
    //send source_id
    send_len = send(sock_send, &id, sizeof(int), MSG_NOSIGNAL);
    if (send_len < 0){
        cerr << "[COMM-Error, send sourse_id error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }
    //send buff
    send_len = send(sock_send, buff, buff_size, MSG_NOSIGNAL);
    if (send_len < 0){
        cerr << "[COMM-Error, send buff error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }
    

    close(sock_send);
    return msg_len;
}



int communicator::comm_recv(int* source_id, void* buff, size_t buff_size, int timeout_sec /*= 0*/) const
{    

    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    timeval to_len = {timeout_sec, 0};

    if (timeout_sec > 0){
        int retval = select(sock+1, &rfds, NULL, NULL, &to_len);
        if (retval == -1){
            cerr << "[Error, select]:"<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
            return -1;
        }
        else if (retval == 0){
            // cerr << "receive timeout."<<endl;
            return 0;
        }
    }


    sockaddr_in clientaddr;
    socklen_t peer_addr_size = sizeof(struct sockaddr_in);

    int sock_recv;
    if( (sock_recv = accept(sock, (struct sockaddr*)&clientaddr, &peer_addr_size)) == -1){
        cerr << "[COMM-Error, accept socket]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }


    byte buffer[MAXBUFFSIZE];
    memset(buffer, 0, MAXBUFFSIZE);
    byte *buffer_ptr = buffer;
    size_t sum_len = 0;
    size_t msg_len = 0;

    while(1){
        int recv_len = recv(sock_recv, buffer_ptr, MAXBUFFSIZE-sum_len, 0);
        if (recv_len < 0){
            cerr << "[COMM-Error, recv]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
            break;
        }
        if (recv_len == 0) break;
        
        sum_len += recv_len;
        buffer_ptr = buffer_ptr + recv_len;

        if (msg_len == 0){
            if ( sum_len > sizeof(int)+sizeof(size_t)){
                msg_len = *((size_t*) buffer);
                *source_id = *((int*)(buffer+sizeof(size_t)));
                // cerr<<"COMM: msg_len: "<<msg_len<<",source_id:"<<*source_id<<endl;
            }
        }
        else{ // msg_len > 0
            if (sum_len >= msg_len)
                break;
        }

    }
    //err checking, sum_len should be smaller than buffsize
    if (sum_len > buff_size){
        cerr<<"[COMM-Error, recv buff_size too small]"<<endl;
        return -1;
    }
    
    memcpy(buff, buffer+sizeof(size_t)+sizeof(uint), buff_size);
	close(sock_recv);
    return sum_len;
    
}

int communicator::comm_sendOut(const std::string& ip_str, int port, void* buff, size_t buff_size) const
{
    if (buff_size > MAXSENDSIZE){
        cerr << "[COMM-Error, buff_size error], MAXSENDSIZE is "<<MAXSENDSIZE<<endl;
        return -1;
    }

    int sock_send;
    if( (sock_send = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cerr << "[COMM-Error, create socket error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }

    sockaddr_in destaddr;
    memset(&destaddr, 0, sizeof(sockaddr_in));
    destaddr.sin_family = AF_INET;
    destaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip_str.c_str(), &(destaddr.sin_addr));

    if( connect(sock_send, (sockaddr*)&destaddr, sizeof(sockaddr)) < 0){
        cerr << "[COMM-Error, connect error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }

    //send msg_len
    size_t msg_len = buff_size+sizeof(size_t)+sizeof(int);
    int send_len = send(sock_send, &msg_len, sizeof(size_t), MSG_NOSIGNAL);
    if (send_len < 0){
        cerr << "[COMM-Error, send msg_size error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }
    //send source_id
    send_len = send(sock_send, &id, sizeof(int), MSG_NOSIGNAL);
    if (send_len < 0){
        cerr << "[COMM-Error, send sourse_id error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }
    //send buff
    send_len = send(sock_send, buff, buff_size, MSG_NOSIGNAL);
    if (send_len < 0){
        cerr << "[COMM-Error, send buff error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }
    

    close(sock_send);
    return msg_len;
}

int communicator::comm_sendOut(const sockaddr_in& destaddr, void* buff, size_t buff_size) const
{
    if (buff_size > MAXSENDSIZE){
        cerr << "[COMM-Error, buff_size error], MAXSENDSIZE is "<<MAXSENDSIZE<<endl;
        return -1;
    }

    int sock_send;
    if( (sock_send = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cerr << "[COMM-Error, create socket error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }


    if( connect(sock_send, (sockaddr*)&destaddr, sizeof(sockaddr)) < 0){
        cerr << "[COMM-Error, connect error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }

    //send msg_len
    size_t msg_len = buff_size+sizeof(size_t)+sizeof(int);
    int send_len = send(sock_send, &msg_len, sizeof(size_t), MSG_NOSIGNAL);
    if (send_len < 0){
        cerr << "[COMM-Error, send msg_size error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }
    //send source_id
    send_len = send(sock_send, &id, sizeof(int), MSG_NOSIGNAL);
    if (send_len < 0){
        cerr << "[COMM-Error, send sourse_id error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }
    //send buff
    send_len = send(sock_send, buff, buff_size, MSG_NOSIGNAL);
    if (send_len < 0){
        cerr << "[COMM-Error, send buff error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }
    

    close(sock_send);
    return msg_len;
}


int communicator::comm_close(){
    close(sock);
    return 0;
}
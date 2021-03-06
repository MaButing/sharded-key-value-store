#include <cerrno>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


#include <vector>
#include <iostream>

#define MAXBUFFSIZE 4096
#define MAXSENDSIZE MAXBUFFSIZE-sizeof(uint)-sizeof(size_t)

using std::vector;
using std::cerr;
using std::endl;
using std::strerror;

// a new session for every msg



typedef char byte;

class communicator
{
private:
    int n;
    int id;
    int sock;//listening socket
    vector<sockaddr_in> addr_list;

public:
	communicator(int n_in = 1, int id_in = 0):n(n_in),id(id_in)
    {cerr<<"COMM "<<n<<" "<<id<<endl;};
    int comm_init(const vector<sockaddr_in>& addr_list_in);
    int comm_send(int dest_id, const void* buff, size_t buff_size) const;
    int comm_recv(int* source_id, void* buff, size_t buff_size, int timeout_sec = 0) const;
    int comm_close();
    int comm_sendOut(const std::string& ip_str, int port, const void* buff, size_t buff_size) const;
    int comm_sendOut(const sockaddr_in& destaddr, const void* buff, size_t buff_size) const;

    int port() const
    {return ntohs(addr_list[id].sin_port);};
};
#include <vector>
#include <set>
#include "communicator.h"
#include "ReqOrd.h"


#ifndef __PAXOSREPL_H__
#define __PAXOSREPL_H__

using std::vector;
using std::set;





class paxos_replica
{
private:
	int n;
	int f;
	int id;
	int my_king;
	int exe_end;
	vector<order_t> log;//seq -> order
	vector<set<int>> certf;//seq -> certificate_set
	int x;//number of seq to skip
	int reprop_begin;//OPTIMIZATION

	communicator comm;

	//process of view change, the candidate king should not propose client request
	//because not sure about prevous log/state, cannot make decision: whether assign a new seq_num
	set<int> couping; //follower in couping process
	vector<request_t> pending_req;

	


	int recv_req(const request_t& req);
	bool req_exist(const request_t& req);
	int bcast_order(const order_t& ord);
	int update_log(const order_t& ord);
	int accept_learn(const order_t& ord, int source_id);//on receiving a PROPOSAL
	int process();
	virtual int exec(const order_t& ord);
	virtual int reply(const request_t& req, int code);
	int coup();
	int follow(int new_king, int reprop_begin); //on receiving OLDKINGISDEAD
	int admit(int new_king, int follower, const vector<order_t>& hist); //on receiving LONGLIVETHEKING


public:
	paxos_replica(int n_in = 1, int f_in = 0, int id_in = 0, int x_in = 0):
	n(n_in), f(f_in), id(id_in),
	my_king(0),exe_end(0),x(x_in),reprop_begin(0),
	comm(n_in, id_in)
	{};

	int repl_init(const vector<sockaddr_in>& addr_list_in);
	int repl_run();
	int repl_close();
	
};

#endif
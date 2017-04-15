#include "paxos_replica.h"
#include <cassert>
#include <fstream>

using std::ofstream;
using std::cout;



int paxos_replica::repl_init(const vector<sockaddr_in>& addr_list_in)
{
	if (comm.comm_init(addr_list_in) != 0)
		return -1;

	string log_name = "ChatLog_"+to_string(id)+".txt";
	ofstream ofs(log_name.c_str(), ofstream::out);
	ofs << "========replica "<<id<<" log=======" << endl;
	ofs.close();

	if (n >= 2*f+1)
		return 0;
	else
		return -1;
}


int paxos_replica::repl_close()
{
	return comm.comm_close();
}

int paxos_replica::repl_run()
{
	cout<<"replica running"<<endl;
	char buff[MAXSENDSIZE];
	while(1){
		cout<<"recv: ";
		int source_id;
		comm.comm_recv(&source_id, buff, MAXSENDSIZE);
		string str(buff);
		cout<< str <<endl;

		size_t pos0 = str.find(":");
		string type = str.substr(0, pos0);
		//CLIENTREQ:request_str
		if (type == "CLIENTREQ"){
			string req_str = str.substr(pos0+1);
			request_t req(req_str);
			assert(source_id == -1*req.client_id);
			recv_req(req);

		}
		//ORDER:<sender_id>:order_str
		else if (type == "ORDER"){
			size_t pos1 = str.find(":", pos0+1);
			int sender_id = stoi(str.substr(pos0+1, pos1-pos0-1));
			assert(source_id == sender_id);
			string ord_str = str.substr(pos1+1);
			order_t ord(ord_str);
			accept_learn(ord, sender_id);
		}
		//OLDKINGISDEAD:<new_king>:<reprop_begin>
		else if (type == "OLDKINGISDEAD"){
			size_t pos1 = str.find(":", pos0+1);
			int new_king = stoi(str.substr(pos0+1, pos1-pos0-1));
			assert(source_id == new_king);
			size_t pos2 = str.find(":", pos1+1);
			int reprop_begin = stoi(str.substr(pos1+1, pos2-pos1-1));
			follow(new_king, reprop_begin);
		}
		//LONGLIVETHEKING:<new_king>:<follower>:{HIST:order_str}
		else if (type == "LONGLIVETHEKING"){
			size_t pos1 = str.find(":", pos0+1);
			int new_king = stoi(str.substr(pos0+1, pos1-pos0-1));
			assert(new_king == id);
			size_t pos2 = str.find(":", pos1+1);
			int follower = stoi(str.substr(pos1+1, pos2-pos1-1));
			assert(follower == source_id);

			vector<order_t> hist;
			size_t hist_end = 0;
			size_t hist_begin = str.find("ORD:", hist_end);
			while(hist_begin != string::npos){
				hist_end = str.find("ORD:", hist_begin+4);
				string ord_str = str.substr(hist_begin, hist_end - hist_begin);
				hist.push_back(order_t(ord_str));

				hist_begin = hist_end;
			}

			admit(new_king, follower, hist);
			
		}

		else 
			continue; //ignore
	}
}






bool paxos_replica::req_exist(const request_t& req)
{
	//go through all the log
	// cout<<"req_exist"<<endl;

	for (auto i = log.begin(); i != log.end(); ++i)
	{
		if (i->req == req)
			return true;
	}
	return false;
}

//CLIENTREQ:request_str
int paxos_replica::recv_req(const request_t& req)// on receiving a client request
{
	//I am not the king, but I shall be the king, because client(god) want me to.
	cout<<"recv_req: "<<req.str()<<endl;
	if (my_king < id)
		coup();

	if (!couping.empty()){
		cout<<"\tpending_req: "<<req.str()<<endl;
		pending_req.push_back(req);
		return 0;
	}

	if (req_exist(req)) //this request has been assigned a seq num
		return 0;
	
	//create a new log record
	order_t ord;
	ord.seq = log.size();//assign new seq_num
	ord.view = my_king; //my king should be myself
	ord.req = req;

	accept_learn(ord, id);//assert(id == my king);

	// if (x != 0){
	// 	// int temp = log.size();
	// 	// log.resize(temp+x);
	// 	// for (int i = temp; i < (int)log.size(); ++i){
	// 	// 	log[i].seq = i;
	// 	// 	log[i].view = id;
	// 	// }
	// 	// certf.resize(temp+x);
	// 	// for (int i = temp; i < (int)log.size(); ++i)//forge certification for skipped slots
	// 	// 	for (int j = 0; j < n; ++j)
	// 	// 		certf[i].insert(j);	
	// }
	if (log.size() == (uint)x)
			log.push_back(order_t());


	return 0;
}




int paxos_replica::bcast_order(const order_t& ord)
{
	cout<<"bcast_order: "<<ord.str()<<endl;

	string str = "ORDER:"+to_string(id)+":"+ord.str();
	for (int i = 0; i < n; ++i){
		if (i == id) continue;
		comm.comm_send(i, (void*)str.c_str(), str.size()+1);
	}

	return 0;
}

int paxos_replica::update_log(const order_t& ord)
{
	if (ord.seq < (int)log.size()){// an seen order
		if (ord.view > log[ord.seq].view){ // a newer version
			/*OPTIMIZATION*/
			// if (ord.req == log[ord.seq].req){
			// 	log[ord.seq].view = ord.view;
			// }
			// else{
			// 	log[ord.seq] = ord;
			// 	certf[ord.seq].clear();
			// 	certf[ord.seq].insert(id);
			// }
			/**************/
			log[ord.seq] = ord;
			certf[ord.seq].clear();
			certf[ord.seq].insert(id);
		}
		else
			return -1;
	}
	else{//a new order
		log.resize(ord.seq+1);
		certf.resize(ord.seq+1);
		log[ord.seq] = ord;
		certf[ord.seq].insert(id);
	}
	return 0;
}

//ORDER:<sender_id>:order_str
int paxos_replica::accept_learn(const order_t& ord, int sender_id) //on receiving a PROPOSAL
{
	
	if (ord.view < my_king)//from a old/dead king
		return 0;//ignore

	cout<<"accept_learn: "<<ord.str()<<endl;

	if (ord.seq < exe_end)//executed
		assert(ord.req == log[ord.seq].req);

	if (ord.view > my_king) //there is new king
		follow(ord.view, reprop_begin); //follow the new king
	
	if (update_log(ord) == 0){//I accept the order
		bcast_order(ord);
	}

	certf[ord.seq].insert(sender_id);//sender should not be self, even it is, doesn't matter
	if (certf[ord.seq].size() == (size_t)f+1 && ord.seq == exe_end)
		process();

	return 0;
}


int paxos_replica::process()
{
	cout<<"process"<<endl;
	for (int i = exe_end; i < (int)log.size(); ++i)
	{
		if (log[i].view < my_king) break;//???do not exec outdated log
		if (certf[i].size() >= (size_t)f+1){
			int code = exec(log[i]);
			if (id == my_king && !(log[i].req == request_t())) 
				reply(log[i].req, code);//if I am the king
			exe_end++;
		}
		else
			break;
	}
	return 0;
}

int paxos_replica::exec(const order_t& ord)
{
	assert(ord.view == my_king);
	cout<<"exec "<<ord.str()<<endl;
	
	string log_name = "ChatLog_"+to_string(id)+".txt";
	ofstream ofs(log_name.c_str(), ofstream::app);
	if (ord.req == request_t()) 
		ofs << "NOOP" << endl;
	else
		ofs << ord.req.str() << endl;
	ofs.close();
	return 0;
}

//REQUESTDONE:<king>:<client_id>:<client_seq>
int paxos_replica::reply(const request_t& req, int code)
{
	cout<<"reply "<<req.str()<<endl;
	sockaddr_in clientaddr;
	memset(&clientaddr, 0, sizeof(sockaddr_in));
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_port = htons(req.client_port);
	inet_pton(AF_INET, req.client_ip_str.c_str(), &(clientaddr.sin_addr));

	int sock_send = socket(AF_INET, SOCK_STREAM, 0);
    if( sock_send < 0){
        cerr << "[Error, create sending socket]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }
    if( connect(sock_send, (sockaddr*)&clientaddr, sizeof(sockaddr)) < 0){
        cerr << "[Error, connect to"<<req.client_id<<"]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        close(sock_send);
        return -1;
    }
	//send
    //REQUESTDONE:<king>:<client_id>:<client_seq>
    string str = "REQUESTDONE:"+to_string(my_king)
    	+":"+to_string(req.client_id)+":"+to_string(req.client_seq);
    //send msg_len
    size_t msg_len = str.size()+1+sizeof(size_t)+sizeof(int);
    int send_len = send(sock_send, &msg_len, sizeof(size_t), MSG_NOSIGNAL);
    if (send_len < 0){
        cerr << "[send, send msg_size error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }
    //send source_id
    send_len = send(sock_send, &id, sizeof(int), MSG_NOSIGNAL);
    if (send_len < 0){
        cerr << "[send, send source_id error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }

    send_len = send(sock_send, str.c_str(), str.size()+1, MSG_NOSIGNAL);
    if (send_len < 0){
        cerr << "[send, send msg error]: "<<strerror(errno)<<"(errno: "<<errno<<")"<<endl;
        return -1;
    }
    
	close(sock_send);
	return 0;
}


int paxos_replica::coup()
{
	cout<<"coup"<<endl;

	my_king = id;
	couping.insert(id);//myself
	// pending_req.clear();

	//OPTIMIZATION
	//reprop_begin = exe_end;

	//broadcast OLDKINGISDEAD
	string str = "OLDKINGISDEAD:"+to_string(my_king)+":"+to_string(reprop_begin);
	for (int i = 0; i < n; ++i){
		if (i == id) continue;
		comm.comm_send(i, (void*)str.c_str(), str.size()+1);
	}

	return 0;
}

//OLDKINGISDEAD:<new_king>:<reprop_begin>
int paxos_replica::follow(int new_king, int reprop_begin) //on receiving OLDKINGISDEAD
{
	if (new_king < my_king)//from a old/dead king
		return 0;//ignore
	
	cout<<"follow "<<new_king<<endl;

	if (!couping.empty()){ //there is a newer king than me, I shall abandon couping
		couping.clear();
		pending_req.clear();
	}

	my_king = new_king;
	string str;
	str += "LONGLIVETHEKING:"+to_string(my_king)+":"+to_string(id);
	
	for (auto i = log.begin(); i != log.end(); ++i){
		str+=i->str();
	}
	/****OPTIMIZATION****/
	// int min = reprop_begin < exe_end? reprop_begin: exe_end;//smaller one
	// for (int i = min; i < log.size(); ++i)
	// {
	// 	str+=":HIST:"+i->str();
	// }
	/*******************/

	//send str to new king
	comm.comm_send(my_king, (void*)str.c_str(), str.size()+1);
	return 0;
}



//LONGLIVETHEKING:<new_king>:<follower>:{HIST:order_str}
int paxos_replica::admit(int new_king, int follower, const vector<order_t>& hist) //on receiving LONGLIVETHEKING
{
	cout<<"admit "<<follower<<endl;

	if (couping.empty())//not couping
		return 0;//ignore
	if (new_king < my_king)//not follow me(my_king)
		return 0;
	couping.insert(follower);

	for (auto i = hist.begin(); i != hist.end(); ++i){
		/****OPTIMIZATION****/
		// if (i->seq < reprop_begin) 
		// 	reprop_begin = i->seq;
		/********************/
		if (update_log(*i)==0)
			certf[i->seq].insert(follower);
	}


	if (couping.size() >= (size_t)f+1){
		cout<<"coup success"<<endl;
		//finish couping;
		couping.clear();
		//repropose
		assert(log.size() == certf.size());
		for (int i = 0; i < (int)log.size(); ++i){
			if (log[i].seq == -1){//empty slot
				log[i].seq = i;
				log[i].view = my_king;
				certf[i].insert(id);
			}
			else{
				assert(log[i].seq == i);
				log[i].view = my_king;//should be me
			}
			bcast_order(log[i]);
		}
		/****OPTIMIZATION****/
		// for (int i = reprop_begin; i < log.size(); ++i){
		// 	if (log[i].seq = -1){//empty slot
		// 		log[i].seq = i;
		// 		log[i].view = my_king;
		// 	}
		// 	else{
		// 		assert(log[i].seq == i);
		// 		log[i].view = my_king;//should be me
		// 	}
		// 	propose(log[i]);
		// }
		/*******************/
		
		//propose pending request
		for (auto i = pending_req.begin(); i != pending_req.end(); ++i){
			recv_req(*i);
		}
		pending_req.clear();

	}
	
	return 0;
}


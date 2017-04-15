#include "shard.h"

int shard::exec(order_t& ord) //overriden
{

	string str = ord.req.msg;
	int pos = str.find('|');

	op_t op(str.substr(0, pos));
	resp_t resp;

	if (op.type == "GET"){
		string key = op.content;

		size_t hash = hash_fn(key);
		if (hash >= begin && hash <= end){//with in range
			if (table.count(key) == 0){// key not exist
				resp.code = -1;
			}
			else{
				resp.code = 0;
				resp.content = table.at(key);
			}
		}
		else{//out of range
			resp.code = -2;
		}

	}
	else if (op.type == "PUT"){
		int pos = op.content.find(':');
		string key = op.content.substr(0,pos);
		string value = op.content.substr(pos+1);

		size_t hash = hash_fn(key);
		if (hash >= begin && hash <= end){//with in range
			table[key] = value;
			resp.code = 0;
		}
		else{//out of range
			resp.code = -2;
		}
	}
	else if (op.type == "DEL"){
		string key = op.content;

		size_t hash = hash_fn(key);
		if (hash >= begin && hash <= end){//with in range
			if (table.count(key) == 0){// key not exist
				resp.code = -1;
			}
			else{
				table.erase(key);
				resp.code = 0;		
			}
		}
		else{//out of range
			resp.code = -2;
		}
	}
	else if (op.type == "CUT"){
		size_t x = stoul(op.content);

		if (x < begin || x >= end){
			resp.code = -3;
		}
		else{
			for (auto it = table.begin(); it != table.end(); ++it){
				if (hash_fn(it->first) < x){
					resp.content += it->first+":"+it->second+":";
					//remove
				}
			}
			resp.code = 0;
			begin = x;
		}
	}
	else if (op.type == "INIT"){
		string list = op.content;

		while (!list.empty()){
			int pos0 = list.find(':');
			string key = list.substr(0,pos);
			int pos1 = list.find(':', pos0+1);
			string value = list.substr(pos0+1, pos1-pos0-1);
			list = list.substr(pos1+1);

			size_t hash = hash_fn(key);
			if (hash >= begin && hash <= end){//with in range
				table[key] = value;
			}
			else{//out of range, should not happen
				resp.content += key+":";
				continue;
			}
		}
		if (resp.content.empty()) 
			resp.code = 0;
		else //shold not happen
			resp.code = -2;
	}

	ord.req.msg += "|"+resp.str();


	paxos_replica::exec(ord);

	return resp.code;

}


//REPLY:<shard_id(end)>:<king>:<req.str()>
int shard::reply(const request_t& req)//overriden
{
	string str = "REPLY:"+to_string(end)+":"+to_string(my_king)+":"+req.str();

	return comm.comm_sendOut(req.client_ip_str, req.client_port, (void*)str.c_str(), str.size()+1);

}
#include <string>

#ifndef __REQORD_H__
#define __REQORD_H__


using std::string;
using std::stoi;
using std::to_string;



//REQ:<client_id>:<client_seq>:<client_ip>:<client_port>:op.str()
struct request_t
{
	int client_id; //always 0
	int client_seq;//assigned by master
	string client_ip_str;//master's ip
	int client_port;//master's port
	string msg;//op_t.str()

	request_t():client_id(-1),client_seq(-1),client_port(-1){};
	request_t(const string& str)
	{
		int pos0 = str.find(":");
		if (str.substr(0,pos0) != "REQ")
			return;
		
		int pos1 = str.find(":", pos0+1);
		client_id = stoi(str.substr(pos0+1, pos1-pos0-1));

		int pos2 = str.find(":", pos1+1);
		client_seq = stoi(str.substr(pos1+1, pos2-pos1-1));
		
		int pos3 = str.find(":", pos2+1);
		client_ip_str = str.substr(pos2+1, pos3-pos2-1);

		int pos4 = str.find(":", pos3+1);
		client_port = stoi(str.substr(pos3+1, pos4-pos3-1));

		msg = str.substr(pos4+1);
	}
	
	string str() const
	{
		return "REQ:"+to_string(client_id) +":"+ to_string(client_seq) +":"+ 
			client_ip_str +":"+ to_string(client_port) +":"+ msg;
	}
	bool operator==(const request_t& req) const
	{return client_id == req.client_id && client_seq == req.client_seq;}
};


//ORD:<view>:<seq>:req.str()
struct order_t
{
	int view;
	int seq;
	request_t req;

	order_t():view(-1),seq(-1){};
	order_t(const string& str) 
	{
		int pos0 = str.find(":");
		if (str.substr(0,pos0) != "ORD")
			return;		

		int pos1 = str.find(":", pos0+1);
		view = stoi(str.substr(pos0+1, pos1-pos0-1));

		int pos2 = str.find(":", pos1+1);
		seq = stoi(str.substr(pos1+1, pos2-pos1-1));

		req = request_t(str.substr(pos2+1));
	}
	string str() const
	{
		return "ORD:"+to_string(view) +":"+ to_string(seq) +":"+ req.str();
	}
};



// what is in req.msg
// from master to shard:
// msg = op.str()|<> = <ip>:<port>:<op>:<content>|<>
struct op_t
{
	string ip_str;
	int port;
	string type;// PUT GET DEL CUT INIT
	string content;

	// PUT:<key>:<value>
	// GET:<key>
	// DEL:<key>
	// CUT:<begin>:<end>
	// INIT:{<key>:<value>}

	op_t():port(0){};
	op_t(const string & str)
	{
		int pos0 = str.find(":");
		ip_str = str.substr(0,pos0);

		int pos1 = str.find(":", pos0+1);
		port = stoi(str.substr(pos0+1, pos1-pos0-1));

		int pos2 = str.find(":", pos1+1);
		type = str.substr(pos1+1, pos2-pos1-1);

		int pos3 = str.find("|", pos2+1);
		content = str.substr(pos2+1, pos3-pos2-1);

	}
	string str() const
	{
		return ip_str+":"+to_string(port)+":"+type+":"+content;
	}

};

// from shard to master
// msg = <op.str()>|<resp.str()> = <ip>:<port>:<op>:<content>|<code>:<content>
struct resp_t
{
	int code;
	// 0 success
	// -1 DEL|GET non-exist key
	// -2 key out of range 
	// -3 cut out of range 
	// -4 unknown error
	string content;

	// PUT: <>
	// GET: <value>
	// DEL: <>
	// CUT: {<key>:<value>}
	// INIT: <>
	resp_t():code(-4){};
	resp_t(const string& str)
	{
		int pos0 = str.find(":");
		code = stoi(str.substr(0,pos0));

		content = str.substr(pos0+1);
	}
	string str() const
	{
		return to_string(code)+":"+content;
	}
};


#endif
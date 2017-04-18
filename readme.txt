<key>:<value> are both strings

!!!!!!!!!!!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!!!!!!!!!!!!!! 
!!! char ':' and '|' are not allowed in <key> and <value> !!!
!!! corrupt protocol parsing, leads to undefined behavior !!!
!!! escape them manually                                  !!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


usage:
  setup shard 65535: ./shard 0 65535
  setup shard 32767: ./setup.sh #need to manully kill the processes to release the port
  setup shard 50000: ./shard 0 50000

  ./master 127.0.0.1 6666

  ./autoclient 127.0.0.1 6001 1 127.0.0.1 6666
  ./client 127.0.0.1 6000 0 127.0.0.1 6666
    
    PUT KEY5:VALUE5
    GET KEY6
    DEL KEY5
    CUT 0:32767
    CUT 32768:50000










protocol detail:

from client to master:
    CLIENTREQ:<client_id>:<client_seq>:<op.type>:<op.ip_str>:<op.port>:<op.content>
                                      | op.str()                                   |
from master to shard:
    MASTERREQ:
        REQ:<client_id>:<client_seq>:<master_ip>:<master_port>:<op.type>:<op.ip_str>:<op.port>:<op.content>
       |                            req.str()                                                              |  
                                                              |  req.msg                                   |

from shard to master:
    REPLY:<shard_id>:<king>:
        REQ:<client_id>:<client_seq>:<master_ip>:<master_port>:
            <op.type>:<op.ip_str>:<op.port>:<op.content>|<resp.code>:<resp.content>
       |                     req.msg                                               |


resp.code:
// 0 success
// -1 DEL|GET non-exist key
// -2 key out of range 
// -3 cut out of range 
// -4 unknown error

from master to client: //same as shard to master
    REPLY:<shard_id>:<king>:
        REQ:<client_id>:<client_seq>:<master_ip>:<master_port>:
            <op.type>:<op.ip_str>:<op.port>:<op.content>|<resp.code>:<resp.content>
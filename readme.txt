readme:

from client to master:
    CLIENTREQ:<client_id>:<client_seq>:<op.type>:<op.ip_str>:<op.port>:<op.content>
                                      | op.str()   |
from master to shard:
    MASTERREQ:
        REQ:<client_id>:<client_seq>:<master_ip>:<master_port>:<op.type>:<op.ip_str>:<op.port>:<op.content>
       |                            req.str()                                                              |  
                                                              |  req.msg                                   |

MASTERREQ:REQ:1:0:127.0.0.1:6666:PUT:127.0.0.1:6666:abc:xyz
MASTERREQ:REQ:1:4:127.0.0.1:6666:GET:127.0.0.1:6666:abc
MASTERREQ:REQ:1:2:127.0.0.1:6666:PUT:127.0.0.1:6666:abc:uvw




from shard to master:
    REPLY:<shard_id>:<king>:
        REQ:<client_id>:<client_seq>:<master_ip>:<master_port>:
            <op.type>:<op.ip_str>:<op.port>:<op.content>|<resp.code>:<resp.content>
           |                     req.msg                                           |

from master to client: //same as shard to master
    REPLY:<shard_id>:<king>:
        REQ:<client_id>:<client_seq>:<master_ip>:<master_port>:
            <op.type>:<op.ip_str>:<op.port>:<op.content>|<resp.code>:<resp.content>
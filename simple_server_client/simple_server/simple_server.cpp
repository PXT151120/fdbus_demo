#include <iostream>
#include <string>
#include "fdbus/fdbus.h"

/*
+ client and server are endpoints
+ endpoints can be deployed on worker: event processing of endpoint is  executed on worker


+ Jobs is the object that FDBus transfers between threads

+ FDbus does not has serialization method (parse complex data) -> protobuf
+ Data communicate between processes using socket (UDS (Unix domain socket) and TCP)
    + Same host : UDS
    + Others : TCP

Server address : identiers on bus that client can find
    + UDS : file://socket filename
    + TCP socket: tcp://ip address: port



+ Service name server : mapping addresses and server's name
+ Using server name addressing : svc://servername

┌─────────┐                        ┌─────────┐                       ┌─────────────┐
│ Client  │                        │ Server  │                       │ Name server │
└────┬────┘                        └────┬────┘                       └──────┬──────┘
     │                                  │                                   │
     │ connect("svc://simpleServer")    │                                   │
     ├──────────────────────────────────────────────────────────────────────>
     │                                  │                                   │
     │                                  │ bind("svc://simpleServer")        │
     │                                  ├───────────────────────────────────>
     │                                  │                                   │--|
     │                                  │ "file:///tmp/fdb-ipc1"            │  |  Allocate
     │                                  │ "tcp://192.168.1.1:60004"         │<-|  address
     │                                  │◄──────────────────────────────────┤
     │                                  │                                   │
     │                                  │   │ bind("/tmp/fdb-ipc1")         │
     │                                  │   │ bind("192.168.1.1:60004")     │
     │                                  │ <-|                               │
     │                                  │ done                              │
     │                                  │───────────────────────────────────>
     │                                  │                                   │
     │ "file:///tmp/fdb-ipc1"           │                                   │
     │◄─────────────────────────────────┤                                   │
     │                                  │                                   │
     │ connect("/tmp/fdb-ipc1")         │                                   │
     ├─────────────────────────────────>│                                   │
     │                                  │                                   │
     │───|                              │───|                               │
     │   │ onOnline()                   │   │ onOnline()                    │
     │<--|                              │<--|                               │
     │                                  │                                   │
*/

class CSimpleServer : public ipc::fdbus::CBaseServer
{
public:
    CSimpleServer() : CBaseServer("simpleServer")
    {

    }

    ~CSimpleServer() {}

    void onOnline(const ipc::fdbus::CFdbOnlineInfo &info)
    {
        std::cout << "Server Session is created...\n";
    }


};


int main(int argc, char** argv)
{
    std::cout << "[Server]: Main Function run ...\n";

    ipc::fdbus::CFdbContext::getInstance()->enableLogger(true);
    ipc::fdbus::CFdbContext::getInstance()->start();


    CSimpleServer simpleServer;
    simpleServer.bind("svc://simpleServer");

    ipc::fdbus::CBaseWorker bkgrWorker;
    bkgrWorker.start(FDB_WORKER_EXE_IN_PLACE);

    return 0;
}


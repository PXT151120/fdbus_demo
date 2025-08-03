#include <iostream>
#include "fdbus/fdbus.h"






class CSimpleClient : public ipc::fdbus::CBaseClient
{
public:
    CSimpleClient() : CBaseClient("simpleClient")
    {

    }

    ~CSimpleClient() {}

    void onOnline(const ipc::fdbus::CFdbOnlineInfo &info)
    {
        std::cout << "Client Session is created...\n";
    }

};

int main (int argc, char** argv)
{
    std::cout << "[Client]: Main Function run ...\n";


    ipc::fdbus::CFdbContext::getInstance()->enableLogger(true);
    ipc::fdbus::CFdbContext::getInstance()->start();

    CSimpleClient simpleClient;
    simpleClient.connect("svc://simpleServer");

    ipc::fdbus::CBaseWorker bkgrWorker;
    bkgrWorker.start(FDB_WORKER_EXE_IN_PLACE);

    return 0;
}
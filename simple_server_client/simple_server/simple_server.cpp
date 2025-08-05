#include <iostream>
#include <string>
#include <cstdint>
#include <cstdio>
#include <chrono>
#include <iomanip>
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

using namespace ipc::fdbus;

enum EGrpId : uint8_t
{
    MEDIA_GROUP_MASTER = 0,
    MEDIA_GROUP_1
};

enum EMsgId : uint8_t
{
    REQ_METADATA = 0,
    REQ_RAWDATA,

    NTF_ELAPSE_TIME,
    NTF_MEDIAPLAYER_CREATED,

    NTF_CJSON_TEST = 128,
};


static CBaseWorker main_worker;


std::string getTimestamp(void)
{
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);

    std::tm local_tm = *std::localtime(&now_time_t);
    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%H:%M:%S");
    return oss.str();
}



class CSimpleServer;
class CBroadcastTimer : public CMethodLoopTimer<CSimpleServer>
{
public:
    CBroadcastTimer(CSimpleServer* server);
    ~CBroadcastTimer();
};


class CSimpleServer : public CBaseServer
{
public:
    CSimpleServer(const char* name, CBaseWorker *worker = (CBaseWorker*)0)
    : CBaseServer(name, worker)
    {
        mTimer = new CBroadcastTimer(this);
        mTimer->attach(&main_worker, false);
    }
    ~CSimpleServer() {}

    void broadcastElapseTimer(CMethodLoopTimer<CSimpleServer> *timer);

protected:
    void onOnline(const CFdbOnlineInfo& info);
    void onOffline(const CFdbOnlineInfo& info);
    void onInvoke(CBaseJob::Ptr &msg_ref);
    void onSubscribe(CBaseJob::Ptr &msg_ref);
private:
    CBroadcastTimer *mTimer;
};


void CSimpleServer::broadcastElapseTimer(CMethodLoopTimer<CSimpleServer> *timer)
{
    // char raw_data[256];
    // memset(raw_data, '=', sizeof(raw_data));
    // raw_data[255] = '\0';
    // broadcast(NTF_ELAPSE_TIME, raw_data, 256, "raw_buffer");
}

void CSimpleServer::onOnline(const CFdbOnlineInfo& info)
{
    printf("[%s | Server]: session up: %d, secure: %d\n", getTimestamp().c_str(), info.mSid, info.mQOS);
    if (info.mFirstOrLast)
    {
        printf("[%s | Server]: Timer enabled\n", getTimestamp().c_str());
        mTimer->enable();
    }
}

void CSimpleServer::onOffline(const CFdbOnlineInfo& info)
{
    printf("[%s | Server]: session shutdown: %d, secure: %d\n", getTimestamp().c_str(), info.mSid, info.mQOS);
    if (info.mFirstOrLast)
    {
        mTimer->disable();
    }
}

void CSimpleServer::onInvoke(CBaseJob::Ptr &msg_ref)
{
    auto msg = castToMessage<CBaseMessage *>(msg_ref);
    static int32_t elapse_time = 0;

    switch(msg->code())
    {
        case REQ_METADATA:
        {
            printf("[%s | Server]: onInvoke METADATA\n", getTimestamp().c_str());
            break;
        }

        case REQ_RAWDATA:
        {
            printf("[%s | Server]: onInvoke RAWDATA: %s\n", getTimestamp().c_str(), (char*)msg->getPayloadBuffer());
            break;
        }

        default: break;
    }
}

void CSimpleServer::onSubscribe(CBaseJob::Ptr &msg_ref)
{
    // auto msg = castToMessage<CFdbMessage *>(msg_ref);
    // const CFdbMsgSubscribeItem *sub_item;

    // FDB_BEGIN_FOREACH_SIGNAL(msg, sub_item)
    // {
    //     FdbMsgCode_t msg_code = sub_item->msg_code();
    //     const char *topic = "";
    //     if (sub_item->has_topic())
    //     {
    //         topic = sub_item->topic().c_str();
    //     }
    //     FdbSessionId_t sid = msg->session();
    //     printf("single message %d, topic %s of session %d is registered! sender: %s\n\n", msg_code, topic, sid, msg->senderName().c_str());
    //     switch (msg_code)
    //     {
    //         case NTF_ELAPSE_TIME:
    //         {
    //             std::string str_topic(topic);
    //             if (!str_topic.compare("my_topic"))
    //             {
    //                 printf("SERVER_STRH: my_topic");
    //             }
    //             else if (!str_topic.compare("raw_buffer"))
    //             {
    //                 printf("SERVER_STRH: raw_buffer");
    //                 std::string raw_data = "raw buffer test for broadcast.";
    //                 msg->broadcast(NTF_ELAPSE_TIME, raw_data.c_str(), raw_data.length() + 1, "raw_buffer");
    //             }
    //             else{}
    //         }
    //         default:
    //         break;
    //     }
    // }
    // FDB_END_FOREACH_SIGNAL()
}

CBroadcastTimer::CBroadcastTimer(CSimpleServer* server)
    : CMethodLoopTimer<CSimpleServer>(1500, true, server, &CSimpleServer::broadcastElapseTimer)
{}

CBroadcastTimer::~CBroadcastTimer()
{}


int main(int argc, char** argv)
{
    printf("[%s | Server]: Main Function run ...\n", getTimestamp().c_str());

    FDB_CONTEXT->enableLogger(true);
    FDB_CONTEXT->start();

    CBaseWorker *worker_ptr = &main_worker;
    worker_ptr->start();

    if (argc > 1)
    {
        std::string server_name = argv[1];
        std::string url(FDB_URL_SVC);
        url += server_name;
        server_name += "_server";
        auto server = new CSimpleServer(server_name.c_str(), worker_ptr);
        server->enableWatchdog(true);
        server->enableUDP(true);
        server->setExportableLevel(FDB_EXPORTABLE_SITE);
        server->bind(url.c_str());

        CBaseWorker bkgrWorker;
        bkgrWorker.start(FDB_WORKER_EXE_IN_PLACE);
    }
    else
    {
        printf("Input name for name_server!!!\n");
    }

    return 0;
}


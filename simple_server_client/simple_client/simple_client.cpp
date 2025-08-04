#include <iostream>
#include <cstdint>
#include "fdbus/fdbus.h"

using namespace ipc::fdbus;

static CBaseWorker main_worker;


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


class CSimpleClient;

class CInvokeTimer : public CMethodLoopTimer<CSimpleClient>
{
public:
    CInvokeTimer(CSimpleClient* client);
    ~CInvokeTimer();
};

class CSimpleClient : public CBaseClient
{
public:
    CSimpleClient(const char* name, CBaseWorker *worker = (CBaseWorker*)0)
    : CBaseClient(name, worker)
    {
        mTimer = new CInvokeTimer(this);
        mTimer->attach(&main_worker, false);
    }
    ~CSimpleClient() {}

    void callServer(CMethodLoopTimer<CSimpleClient> *timer);

protected:
    void onOnline(const CFdbOnlineInfo& info);
    void onOffline(const CFdbOnlineInfo& info);
    void onBroadcast(CBaseJob::Ptr &msg_ref);
    void onKickDog(CBaseJob::Ptr &msg_ref);
    void onReply(CBaseJob::Ptr &msg_ref);
    void onStatus(CBaseJob::Ptr &msg_ref, int32_t error_code, const char *description);

private:
    CInvokeTimer *mTimer;
};

void CSimpleClient::callServer(CMethodLoopTimer<CSimpleClient> *timer)
{
    std::cout << "[Client]: callServer is called\n";
    invoke(REQ_RAWDATA);
}

void CSimpleClient::onOnline(const CFdbOnlineInfo &info)
{
    FDB_LOG_I("[Client]: session online: %d, secure: %d\n", info.mSid, info.mQOS);
    mTimer->enable();

    CFdbMsgSubscribeList subscribe_list;
    subscribe_list.addNotifyItem(NTF_ELAPSE_TIME, "my_topic");
    subscribe_list.addNotifyItem(NTF_ELAPSE_TIME, "raw_buffer");
    subscribe_list.addNotifyItem(NTF_CJSON_TEST);
    subscribe_list.addNotifyGroup(MEDIA_GROUP_1);

    subscribe(subscribe_list, 0, info.mQOS);
}

void CSimpleClient::onOffline(const CFdbOnlineInfo &info)
{
    FDB_LOG_I("[Client]: session shutdown: %d, secure: %d\n", info.mSid, info.mQOS);

    if (info.mFirstOrLast)
    {
        mTimer->disable();
    }
}


void CSimpleClient::onBroadcast(CBaseJob::Ptr &msg_ref)
{
    auto msg = castToMessage<CBaseMessage *>(msg_ref);
    FDB_LOG_I("[Client]: Broadcast is received: %d; topic: %s, qos: %d\n", msg->code(), msg->topic().c_str(), msg->qos());

    switch (msg->code())
    {
        case NTF_ELAPSE_TIME:
        {
            std::string topic {msg->topic()};
            if (!topic.compare("my_topic"))
            {
                FDB_LOG_I("[Client]: elapse time is received: hour: %d, minute: %d, second: %d\n",
                                    et.hour(), et.minute(), et.second());
            }
            else if (!topic.compare("raw_buffer"))
            {
                char *buffer = new char;
                memcpy(buffer, (char*)msg->getPayloadBuffer(), msg->getPayloadSize());
                FDB_LOG_I("[Client]: Broadcast of raw_buffer: %d\n", buffer);
                delete buffer;
            }
            else{}
            break;
        }
        case NTF_CJSON_TEST:
        {
            FDB_LOG_I("[Client]: Broadcast with JSON\n");
            break;
        }
        default:    break;
    }
}

void CSimpleClient::onKickDog(CBaseJob::Ptr &msg_ref)
{
    CFdbMessage::kickDog(msg_ref, worker(), [](CBaseJob::Ptr &msg_ref)
                        {
                            CFdbMessage::feedDog(msg_ref);
                        });
}

void CSimpleClient::onReply(CBaseJob::Ptr &msg_ref)
{
    auto msg = castToMessage<CBaseMessage *>(msg_ref);
    FDB_LOG_I("[Client]: response is received. sn = %d\n", msg->sn());

    switch(msg->code())
    {
        case REQ_METADATA:
        {
            if (msg->isStatus())
            {
                if (msg->isError())
                {
                    int32_t error_code;
                    std::string reason;

                    if (!msg->decodeStatus(error_code, reason))
                    {
                        FDB_LOG_I("[Client]: fail to decode status\n");
                        return;
                    }
                    FDB_LOG_I("[Client]: onReply(): status is received: msg code : %d, error_code: %d, reason: %s\n",
                        msg->code(), error_code, reason.c_str());
                }
                return;
            }
            FDB_LOG_I("[Client]: onReply of METADATA");
            break;
        }

        case REQ_RAWDATA:
        {
            if (msg->isStatus())
            {
                if (msg->isError())
                {
                    int32_t error_code;
                    std::string reason;

                    if (!msg->decodeStatus(error_code, reason))
                    {
                        FDB_LOG_I("[Client]: fail to decode status\n");
                        return;
                    }
                    FDB_LOG_I("[Client]: onReply(): status is received: msg code : %d, error_code: %d, reason: %s\n",
                        msg->code(), error_code, reason.c_str());
                }
                return;
            }
            FDB_LOG_I("[Client]: onReply of RAWDATA: %s\n", (char*)msg->getPayloadBuffer());
            break;
        }

        default: break;
    }
}

void CSimpleClient::onStatus(CBaseJob::Ptr &msg_ref, int32_t error_code, const char *description)
{
    auto msg = castToMessage<CBaseMessage *>(msg_ref);

    if (msg->isSubscribe())
    {
        if (!msg->isError())
        {
            FDB_LOG_I("[Client]: subscribe is ok! sn: %d is received.\n", msg->sn());
        }
    }
    FDB_LOG_I("[Client]: Reason: %s\n", description);
}

CInvokeTimer::CInvokeTimer(CSimpleClient* client)
    : CMethodLoopTimer<CSimpleClient>(1000, true, client, &CSimpleClient::callServer)
{}

CInvokeTimer::~CInvokeTimer()
{}


int main (int argc, char** argv)
{
    std::cout << "[Client]: Main Function run ...\n";

    FDB_CONTEXT->start();

    FDB_CONTEXT->enableLogger(true);
    fdbLogAppendLineEnd(true);
    FDB_CONTEXT->registerNsWatchdogListener([](const tNsWatchdogList &dropped_list)
        {
            for (auto it = dropped_list.begin(); it != dropped_list.end(); ++it)
            {
                FDB_LOG_F("Error!!! Endpoint drops - name: %s, pid: %d\n", it->mClientName.c_str(), it->mPid);
            }
        });

    CBaseWorker *worker_ptr = &main_worker;
    worker_ptr->start();

    if (argc > 1)
    {
        std::string server_name = argv[1];
        std::string url {FDB_URL_SVC};

        url += server_name;
        server_name += "_client";
        auto client = new CSimpleClient(server_name.c_str(), worker_ptr);
        client->enableReconnect(true);
        client->enableUDP(true);
        client->enableTimeStamp(true);
        client->connect(url.c_str());

        CBaseWorker bkgrWorker;
        bkgrWorker.start(FDB_WORKER_EXE_IN_PLACE);
    }
    else
    {
        std::cout << "Input name for name_server !!!\n";
    }

    return 0;
}
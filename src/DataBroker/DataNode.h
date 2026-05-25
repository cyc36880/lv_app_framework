/**
 * @file DataNode.h
 * @brief DataNode class for pub/sub communication
 */
#ifndef FRAMEWORK_DATANODE_H
#define FRAMEWORK_DATANODE_H

#include <cstddef>
#include <cstdint>
#include <list>

class DataBroker;
class DataTimer;

class DataNode {
public:
    typedef enum {
        EVENT_NONE = 0,
        EVENT_PUBLISH = 1 << 0,
        EVENT_PULL = 1 << 1,
        EVENT_NOTIFY = 1 << 2,
        EVENT_TIMER = 1 << 3,
        EVENT_ALL = 0xFFFFFFFF
    } EventCode_t;

    typedef enum {
        RES_OK = 0,
        RES_UNKNOWN = -1,
        RES_SIZE_MISMATCH = -2,
        RES_UNSUPPORTED_REQUEST = -3,
        RES_NO_CALLBACK = -4,
        RES_NO_DATA = -5,
        RES_NOT_FOUND = -6,
        RES_PARAM_ERROR = -7,
        RES_STOP_PROCESS = -8,
    } ResCode_t;

    typedef struct EventParam {
        EventParam(const DataNode* _tran, const DataNode* _recv, EventCode_t _event, void* _data_p, size_t _size)
            : tran(_tran), recv(_recv), event(_event), data_p(_data_p), size(_size) {}
        const DataNode* tran;
        const DataNode* recv;
        EventCode_t event;
        void* data_p;
        size_t size;
    } EventParam_t;

    typedef int (*EventCallback_t)(DataNode* node, EventParam_t* param);

    typedef std::list<DataNode*> DataNodeList_t;

public:
    DataNode(const char* id, DataBroker* broker, void* userData = nullptr);
    virtual ~DataNode();

    const DataNode* subscribe(const char* pubID);
    bool unsubscribe(const char* pubID);

    size_t getPublishersNumber();
    size_t getSubscribersNumber();

    int publish(const void* data_p, size_t size);
    int pull(const char* pubID, void* data_p, size_t size);
    int pull(const DataNode* pub, void* data_p, size_t size);
    int notify(const char* pubID, const void* data_p, size_t size);
    int notify(const DataNode* pub, const void* data_p, size_t size);

    void setEventCallback(EventCallback_t callback, uint32_t eventFilter = EVENT_ALL);
    void setEventFilter(uint32_t eventFilter);
    virtual int onEvent(EventParam_t* param);

    void startTimer(uint32_t period);
    void setTimerPeriod(uint32_t period);
    void resetTimer();
    void stopTimer();

    void setUserData(void* userData) { _userData = userData; }
    void* getUserData() { return _userData; }
    const char* getID() const { return _ID; }

private:
    DataNodeList_t _publishers;
    DataNodeList_t _subscribers;

    const char* _ID;
    DataBroker* _broker;
    void* _userData;

    EventCallback_t _eventCallback;
    uint32_t _eventFilterMask;
    DataTimer* _timer;

private:
    static void timerCallbackHandler(DataTimer* timer);
    static int sendEvent(DataNode* node, EventParam_t* param);
};

#endif  // FRAMEWORK_DATANODE_H
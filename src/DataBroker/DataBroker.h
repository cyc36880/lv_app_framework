/**
 * @file DataBroker.h
 * @brief DataBroker class manages all DataNodes
 */
#ifndef FRAMEWORK_DATABROKER_H
#define FRAMEWORK_DATABROKER_H

#include "DataNode.h"

class DataTimerManager;

class DataBroker {
    friend class DataNode;

public:
    DataBroker(const char* name);
    ~DataBroker();

    size_t getNodeNumber();
    DataNode* mainNode() { return _mainNode; }

    void initTimerManager(uint32_t (*tickFunction)(void));
    uint32_t handleTimer();

    /** @brief 获取定时器管理器（用于创建独立 DataTimer） */
    DataTimerManager* timerManager() { return _timerManager; }

protected:
    bool add(DataNode* node);
    bool remove(DataNode* node);
    bool remove(DataNode::DataNodeList_t* vec, DataNode* node);
    DataNode* search(const char* id);
    DataNode* search(DataNode::DataNodeList_t* vec, const char* id);

private:
    DataNode::DataNodeList_t* _nodePool;
    DataNode* _mainNode;
    DataTimerManager* _timerManager;
    const char* _name;
};

#endif  // FRAMEWORK_DATABROKER_H
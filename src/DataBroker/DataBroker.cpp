/**
 * @file DataBroker.cpp
 * @brief Implementation of DataBroker class
 */
#include "DataBroker.h"
#include "DataTimer.h"
#include "DataBrokerLog.h"
#include <algorithm>
#include <cstring>

DataBroker::DataBroker(const char* name)
    : _nodePool(nullptr)
    , _mainNode(nullptr)
    , _timerManager(nullptr)
{
    _name = name;
    _nodePool = new DataNode::DataNodeList_t;
    _mainNode = new DataNode(name, this);
}

DataBroker::~DataBroker()
{
    DN_LOG_WARN("DataBroker[%s] closing...", _name);
    DN_LOG_INFO("Deleting main node: %p", _mainNode);
    delete _mainNode;
    _mainNode = nullptr;

    for (auto iter : *_nodePool) {
        (void)iter;
        DN_LOG_WARN("Leak: DataNode[%s]", iter->getID());
    }

    DN_LOG_INFO("Deleting node pool: %p", _nodePool);
    delete _nodePool;
    _nodePool = nullptr;

    if (_timerManager) {
        DN_LOG_INFO("Deleting timer manager: %p", _timerManager);
        delete _timerManager;
        _timerManager = nullptr;
    }

    DN_LOG_WARN("DataBroker[%s] closed.", _name);
}

DataNode* DataBroker::search(const char* id)
{
    return search(_nodePool, id);
}

DataNode* DataBroker::search(DataNode::DataNodeList_t* vec, const char* id)
{
    for (auto iter : *vec) {
        if (strcmp(id, iter->getID()) == 0) {
            return iter;
        }
    }
    return nullptr;
}

bool DataBroker::add(DataNode* node)
{
    if (_mainNode == nullptr || node == _mainNode) {
        return false;
    }

    if (search(node->getID()) != nullptr) {
        DN_LOG_ERROR("Multi add DataNode[%s]", node->getID());
        return false;
    }

    _nodePool->push_back(node);
    _mainNode->subscribe(node->getID());
    return true;
}

bool DataBroker::remove(DataNode* node)
{
    return remove(_nodePool, node);
}

bool DataBroker::remove(DataNode::DataNodeList_t* vec, DataNode* node)
{
    auto iter = std::find(vec->begin(), vec->end(), node);
    if (iter == vec->end()) {
        DN_LOG_ERROR("DataNode[%s] was not found", node->getID());
        return false;
    }
    vec->erase(iter);
    return true;
}

size_t DataBroker::getNodeNumber()
{
    return _nodePool->size();
}

void DataBroker::initTimerManager(uint32_t (*tickFunction)(void))
{
    if (!_timerManager) {
        _timerManager = new DataTimerManager;
    }
    _timerManager->setTickCallback(tickFunction);
}

uint32_t DataBroker::handleTimer()
{
    return _timerManager->handler();
}
/**
 * @file DeviceManager.cpp
 * @brief Implementation of DeviceManager
 */
#include "DeviceManager.h"
#include "DeviceObject.h"
#include <string.h>

DeviceManager::DeviceManager(DeviceObject** devArray, size_t len)
{
    _devArrayLen = len;
    _devArray = new DeviceObject*[len];
    for (size_t i = 0; i < len; i++) {
        _devArray[i] = devArray[i];
    }
}

DeviceManager::~DeviceManager()
{
    delete[] _devArray;
}

void DeviceManager::init(InitFinishCallback_t callback)
{
    for (size_t i = 0; i < _devArrayLen; i++) {
        DeviceObject* dev = _devArray[i];
        int retval = dev->init();
        if (callback) {
            callback(this, dev, retval);
        }
    }
}

DeviceObject* DeviceManager::getDevice(const char* name)
{
    for (size_t i = 0; i < _devArrayLen; i++) {
        DeviceObject* dev = _devArray[i];
        if (strcmp(name, dev->getName()) == 0) {
            return dev;
        }
    }
    return nullptr;
}
/**
 * @file DeviceManager.h
 * @brief Device manager for initializing and managing device objects
 */
#ifndef FRAMEWORK_DEVICE_MANAGER_H
#define FRAMEWORK_DEVICE_MANAGER_H

#include <stddef.h>

class DeviceObject;

class DeviceManager {
public:
    typedef void (*InitFinishCallback_t)(DeviceManager* manager, DeviceObject* dev, int retval);

public:
    DeviceManager(DeviceObject** devArray, size_t len);
    ~DeviceManager();

    void init(InitFinishCallback_t callback = nullptr);
    DeviceObject* getDevice(const char* name);

private:
    DeviceObject** _devArray;
    size_t _devArrayLen;
};

#endif  // FRAMEWORK_DEVICE_MANAGER_H
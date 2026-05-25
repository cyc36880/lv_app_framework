/**
 * @file DeviceObject.h
 * @brief Base class for device objects with standard I/O interface
 */
#ifndef FRAMEWORK_DEVICE_OBJECT_H
#define FRAMEWORK_DEVICE_OBJECT_H

#include <stddef.h>
#include <stdint.h>

#define DEVICE_OBJECT_IOCMD_DEF(dir, size, type, number) \
    (uint32_t)(                                          \
        (((uint32_t)(dir) & 0x0003) << 30)               \
        | (((uint32_t)(size) & 0x3FFF) << 16)            \
        | (((uint32_t)(type) & 0x00FF) << 8)              \
        | ((uint32_t)(number) & 0x00FF))

class DeviceObject {
public:
    typedef enum {
        DIR_NONE = 0x00,
        DIR_IN = 0x01,
        DIR_OUT = 0x02,
        DIR_IN_OUT = 0x03,
    } Direction_t;

    typedef union {
        uint32_t full;
        struct {
            uint32_t number : 8;
            uint32_t type : 8;
            uint32_t size : 14;
            uint32_t dir : 2;
        } ch;
    } IO_Cmd_t;

    typedef enum {
        RES_OK = 0,
        RES_UNKNOWN = -1,
        RES_NO_IMPLEMENTED = -2,
        RES_INIT_FAILED = -3,
        RES_PARAM_ERROR = -4,
        RES_UNSUPPORT = -5,
    } ResCode_t;

public:
    DeviceObject(const char* name)
        : _name(name), initOK(false) {}

    virtual ~DeviceObject() {}

    int init() {
        int retval = onInit();
        initOK = retval >= RES_OK;
        return retval;
    }

    int read(void* buffer, size_t size) {
        if (!initOK) return RES_INIT_FAILED;
        if (!buffer) return RES_PARAM_ERROR;
        return onRead(buffer, size);
    }

    int write(const void* buffer, size_t size) {
        if (!initOK) return RES_INIT_FAILED;
        if (!buffer) return RES_PARAM_ERROR;
        return onWrite(buffer, size);
    }

    int ioctl(uint32_t cmd, void* data = nullptr, size_t size = 0) {
        IO_Cmd_t* c = (IO_Cmd_t*)&cmd;
        if (!initOK) return RES_INIT_FAILED;
        if (size != c->ch.size) return RES_PARAM_ERROR;
        return onIoctl(*c, data);
    }

    const char* getName() { return _name; }

private:
    const char* _name;
    bool initOK;

protected:
    virtual int onInit() { return RES_NO_IMPLEMENTED; }
    virtual int onRead(void* buffer, size_t size) { return RES_NO_IMPLEMENTED; }
    virtual int onWrite(const void* buffer, size_t size) { return RES_NO_IMPLEMENTED; }
    virtual int onIoctl(IO_Cmd_t cmd, void* data) { return RES_NO_IMPLEMENTED; }
};

#endif  // FRAMEWORK_DEVICE_OBJECT_H
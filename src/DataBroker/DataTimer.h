/**
 * @file DataTimer.h
 * @brief Timer management for DataNodes
 */
#ifndef FRAMEWORK_DATATIMER_H
#define FRAMEWORK_DATATIMER_H

#include <cstdint>
#include <list>

class DataTimerManager;

class DataTimer {
    friend class DataTimerManager;

public:
    typedef void (*TimerFunction_t)(DataTimer*);

public:
    DataTimer(DataTimerManager* manager, TimerFunction_t timerFunc,
              uint32_t period, void* userData = nullptr);
    ~DataTimer();

    void pause();
    void resume();
    void reset();
    uint32_t remain();
    void setPeriod(uint32_t period);
    uint32_t getPeriod();
    void* getUserData();

protected:
    bool isReady();
    bool isPause();
    void invoke();

private:
    DataTimerManager* _manager;
    TimerFunction_t _timerFunc;
    void* _userData;
    uint32_t _period;
    uint32_t _lastTick;
    bool _isPause;
};

class DataTimerManager {
public:
    typedef uint32_t (*TickFunction_t)(void);

public:
    DataTimerManager(TickFunction_t tickCallback = nullptr);
    ~DataTimerManager();

    void setTickCallback(TickFunction_t tickCallback);
    void add(DataTimer* timer);
    void remove(DataTimer* timer);
    uint32_t getTick();
    uint32_t getTickElaps(uint32_t prevTick);
    uint32_t handler();

private:
    TickFunction_t _getTick;
    std::list<DataTimer*> _timerList;
};

#endif  // FRAMEWORK_DATATIMER_H
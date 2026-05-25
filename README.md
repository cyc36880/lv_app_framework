# X-Track Framework

A lightweight UI framework built on LVGL with page management, pub/sub data communication, and device management.

## Directory Structure

```
framework/
├── PageManager/          # Page navigation with animations
│   ├── PageDef.h        # Animation type definitions
│   ├── PageBase.h/cpp   # Base class for all pages
│   ├── PageFactory.h    # Factory interface for page creation
│   └── PageManager.h/cpp # Navigation manager
├── DataBroker/          # Pub/Sub data communication
│   ├── DataBroker.h/cpp # Broker that manages nodes
│   ├── DataNode.h/cpp   # Node for publishing/subscribing
│   └── DataTimer.h/cpp  # Timer management
├── DeviceManager/       # Device initialization/management
│   ├── DeviceObject.h    # Base device class
│   └── DeviceManager.h/cpp # Device manager
├── ResourceManager/     # Resource lookup utilities
│   ├── ResourceManager.h # Template-based resource manager
│   └── ResourceManagerStatic.h # Static resource pool
├── port/                # Platform adaptation layer
│   ├── Port.h           # Platform interface declarations
│   └── Port_Stub.cpp    # Stub implementation (reference)
└── Framework.h          # Main include file
```

## Quick Start

### 1. Implement Platform Adapter

Create your platform-specific implementation based on `port/Port.h`:

```cpp
// your_port.cpp
#include "Port.h"

uint32_t lv_tick_get(void) {
    // Return milliseconds since startup
}

int32_t port_get_screen_width(void) {
    return 320;  // or get from display driver
}

// ... implement other functions
```

### 2. Create Pages

```cpp
// MyPage.h
#include "Framework.h"

class MyPage : public PageBase {
public:
    MyPage();

private:
    void onViewLoad() override;
    void onViewDidLoad() override;
    void onViewWillAppear() override;
    void onViewDidAppear() override;
    void onViewWillDisappear() override;
    void onViewDidDisappear() override;

private:
    lv_obj_t* _label;
};

// MyPage.cpp
#include "MyPage.h"

APP_DESCRIPTOR_DEF(MyPage);  // Register page

MyPage::MyPage() : _label(nullptr) {}

void MyPage::onViewLoad() {
    _label = lv_label_create(getRoot());
    lv_label_set_text(_label, "Hello");
}
```

### 3. Initialize and Navigate

```cpp
// main.cpp
#include "Framework.h"

// Your factory implementation
class MyFactory : public PageFactory {
public:
    PageBase* create(const char* name) override {
        if (strcmp(name, "MyPage") == 0) return new MyPage();
        return nullptr;
    }
};

int main() {
    // Initialize platform
    port_init();

    // Create factory and manager
    MyFactory factory;
    PageManager manager(&factory);

    // Install and show first page
    manager.install("MyPage", "MyPage");
    manager.push("MyPage");

    // Main loop
    while (1) {
        uint32_t next = broker.handleTimer();
        lv_timer_handler();
        port_delay(next > 10 ? 10 : next);
    }
}
```

## Key Features

### PageManager
- Page lifecycle management (LOAD → WILL_APPEAR → DID_APPEAR → ACTIVITY → ...)
- Navigation: push, pop, replace, backHome
- Animation support: OVER_LEFT/RIGHT/TOP/BOTTOM, MOVE, FADE
- Cache management for pages

### DataBroker
- Pub/Sub communication between components
- Timer support for periodic data publishing
- Event filtering and callbacks

### DeviceManager
- Automatic device initialization
- Device lookup by name
- init()/read()/write()/ioctl() interface

## Platform Interface

Implement these functions in your platform port:

| Function | Description |
|----------|-------------|
| `port_init()` | Initialize platform |
| `lv_tick_get()` | Get system time in ms |
| `port_get_screen_width()` | Get screen width |
| `port_get_screen_height()` | Get screen height |
| `port_delay(ms)` | Delay milliseconds |
| `port_malloc(size)` | Allocate memory |
| `port_free(ptr)` | Free memory |
| `port_debug_log(fmt, ...)` | Debug output |

## License

MIT License
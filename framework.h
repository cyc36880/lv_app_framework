/**
 * @mainpage X-Track Framework
 *
 * @section intro Introduction
 * The X-Track Framework is a lightweight UI framework built on LVGL, providing:
 * - PageManager: Page navigation with animations
 * - DataBroker: Pub/Sub data communication
 * - DeviceManager: Device initialization and management
 * - ResourceManager: Resource lookup utilities
 *
 * @section platform Platform Adaptation
 *
 * To port this framework to a new platform, you need to implement the functions
 * declared in @ref Port.h. See @ref Port_Stub.cpp for a reference implementation.
 *
 * @section usage Basic Usage
 *
 * @subsection page_usage Creating Pages
 *
 * Define your page by extending PageBase:
 * @code
 * class MyPage : public PageBase {
 * public:
 *     MyPage();
 * private:
 *     // Implement lifecycle callbacks
 *     void onViewLoad() override;
 *     void onViewDidLoad() override;
 *     void onViewWillAppear() override;
 *     void onViewDidAppear() override;
 *     // ...
 * };
 * @endcode
 *
 * @subsection factory_usage Page Factory
 *
 * Register your page using the APP_DESCRIPTOR_DEF macro:
 * @code
 * APP_DESCRIPTOR_DEF(MyPage);
 * @endcode
 *
 * @subsection nav_usage Navigation
 *
 * @code
 * // Push a new page
 * manager->push("MyPage");
 *
 * // Pop current page
 * manager->pop();
 *
 * // Replace current page
 * manager->replace("OtherPage");
 * @endcode
 *
 * @subsection data_usage DataBroker Usage
 *
 * @code
 * // Subscribe to data
 * m_GNSS.subscribe("GNSS");
 *
 * // Handle data updates in onEvent callback
 * int MyModel::onEvent(DataNode* node, DataNode::EventParam_t* param) {
 *     if (param->event == DataNode::EVENT_PUBLISH) {
 *         auto info = (GNSS_Info_t*)param->data_p;
 *         // Process data...
 *     }
 *     return DataNode::RES_OK;
 * }
 * @endcode
 */

#ifndef FRAMEWORK_H
#define FRAMEWORK_H

/**
 * @file Framework.h
 * @brief Main include file for the X-Track Framework
 */

// PageManager module
#include "src/PageManager/PageDef.h"
#include "src/PageManager/PageBase.h"
#include "src/PageManager/PageFactory.h"
#include "src/PageManager/PageManager.h"

// DataBroker module
#include "src/DataBroker/DataBroker.h"
#include "src/DataBroker/DataNode.h"
#include "src/DataBroker/DataTimer.h"

// DeviceManager module
#include "src/DeviceManager/DeviceObject.h"
#include "src/DeviceManager/DeviceManager.h"

// ResourceManager module
#include "src/ResourceManager/ResourceManager.h"
#include "src/ResourceManager/ResourceManagerStatic.h"

#include "src/AppFactory/AppFactory.h"

#endif  // FRAMEWORK_H
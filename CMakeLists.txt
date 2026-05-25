#ifndef FRAMEWORK_CMakeLists.txt
#define FRAMEWORK_CMakeLists.txt

# X-Track Framework CMake Configuration
# Copy this to your project's CMakeLists.txt and adjust as needed

# Set framework source files
set(FW_APP_FACTORY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AppFactory/AppFactory.cpp
)

set(FW_PAGE_MANAGER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PageManager/PageBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PageManager/PageManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PageManager/PageManagerState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PageManager/PageManagerAnim.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PageManager/PageManagerRouter.cpp
)

set(FW_DATA_BROKER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DataBroker/DataNode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DataBroker/DataBroker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DataBroker/DataTimer.cpp
)

set(FW_DEVICE_MANAGER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DeviceManager/DeviceManager.cpp
)

# Create framework library
add_library(framework STATIC
    ${FW_APP_FACTORY_SOURCES}
    ${FW_PAGE_MANAGER_SOURCES}
    ${FW_DATA_BROKER_SOURCES}
    ${FW_DEVICE_MANAGER_SOURCES}
)

# Link LVGL (adjust path to your LVGL installation)
find_package(LVGL REQUIRED)
target_link_libraries(framework PUBLIC lvgl)

# Compiler flags
target_compile_options(framework PRIVATE
    -Wall
    -Wextra
    -pedantic
)

# Set C++ standard
set_target_properties(framework PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

#endif  // FRAMEWORK_CMakeLists.txt
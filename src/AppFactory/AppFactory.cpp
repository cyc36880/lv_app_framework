/**
 * @file AppFactory.cpp
 * @brief 页面工厂实现
 *
 * 【功能】
 * 实现 AppFactory 的页面注册、查找创建和批量安装逻辑。
 *
 * 【依赖】
 * - AppFactory.h：自身头文件
 * - <cstring>：strcmp 用于页面名称匹配
 */

#include "AppFactory.h"
#include "../PageManager/PageManager.h"
#include <cstring>

/**
 * @brief 构造函数
 *
 * 初始化空注册表。页面通过 AppDescriptor 构造函数自动调用 add() 注册，
 * 不在此处手动添加。
 */
AppFactory::AppFactory()
    : _nameArray{0}
    , _descriptorArray{0}
    , _num(0)
{
}

AppFactory::~AppFactory() { }

/**
 * @brief 注册页面描述符
 *
 * 由 AppDescriptor 构造函数调用，将页面名称和描述符指针存入数组。
 * 如果数组已满（超过 APP_FACTORY_MAX_NUM），静默丢弃。
 *
 * @param name 页面名称（字符串字面量，生命周期与程序一致）
 * @param descriptor 描述符指针
 */
void AppFactory::add(const char* name, AppDescriptor* descriptor)
{
    if (_num >= APP_FACTORY_MAX_NUM) {
        return;
    }
    _nameArray[_num] = name;
    _descriptorArray[_num] = descriptor;
    _num++;
}

/**
 * @brief 根据名称创建页面实例
 *
 * 遍历注册表，用 strcmp 匹配名称。找到后调用描述符的 create()
 * 方法创建页面对象（new 对应的页面类）。
 * PageManager 在 push/replace 时调用此方法。
 *
 * @param name 页面名称
 * @return 新创建的 PageBase 子类对象，未找到返回 nullptr
 */
PageBase* AppFactory::create(const char* name)
{
    for (int i = 0; i < _num; i++) {
        if (strcmp(name, _nameArray[i]) == 0) {
            return _descriptorArray[i]->create();
        }
    }
    return nullptr;
}

/**
 * @brief 批量安装页面到 PageManager
 *
 * PageManager 需要在页面池中预先注册页面名称（install），
 * 才能通过 push(name) 触发创建和状态机转换。
 * 此函数对所有已注册的页面调用 install()。
 *
 * @param manager 目标 PageManager 实例
 */
void AppFactory::installAll(PageManager* manager)
{
    for (int i = 0; i < _num; i++) {
        manager->install(_nameArray[i], nullptr);
    }
}

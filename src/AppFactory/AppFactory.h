/**
 * @file AppFactory.h
 * @brief 页面工厂——通过 APP_DESCRIPTOR_DEF 宏实现页面自动注册
 *
 * 【功能】
 * 继承 Framework 的 PageFactory，提供页面的自动注册和创建能力。
 * 使用 AppDescriptor 抽象类描述页面类型，AppFactory 持有描述符数组，
 * 在 PageManager 需要创建页面时调用对应描述符的 create() 方法。
 *
 * 【依赖】
 * - Framework.h：PageFactory、PageBase、PageManager
 *
 * 【关键设计——静态注册机制】
 * APP_DESCRIPTOR_DEF(PageName) 宏展开后定义了一个 static 全局 AppDescriptor
 * 子类实例。C++ 保证 static 全局对象的构造函数在 main() 之前运行，
 * 因此每个页面的描述符会在程序启动时自动调用 AppFactory::add() 注册。
 * 这避免了手动维护页面注册表的繁琐和遗忘。
 *
 * 使用示例：
 *   在 MyPage.cpp 中加入：
 *     APP_DESCRIPTOR_DEF(MyPage);
 *   然后确保包含 MyPage.h 以实例化该宏生成的静态对象。
 */

#ifndef APP_FACTORY_H
#define APP_FACTORY_H

#include "../PageManager/PageFactory.h"

/** 工厂最大支持的页面种类数 */
#ifndef APP_FACTORY_MAX_NUM
#define APP_FACTORY_MAX_NUM 32
#endif

/**
 * @brief 页面描述符自动注册宏
 *
 * 在页面 .cpp 文件中使用此宏，展开后会：
 * 1. 定义一个 PageNameDescriptor 类，继承 AppDescriptor
 * 2. 创建一个该类的 static 全局实例
 * 3. 实例的构造函数调用 AppFactory::add("PageName", this) 完成注册
 *
 * @param name 页面类名（不需要引号），例如 APP_DESCRIPTOR_DEF(HomePage)
 */
#define APP_DESCRIPTOR_DEF(name)                        \
    class name##Descriptor : public AppDescriptor {      \
    public:                                              \
        name##Descriptor()                               \
            : AppDescriptor(#name)                       \
        {                                                \
        }                                                \
        virtual PageBase* create() override { return new name; } \
    };                                                   \
    static name##Descriptor name##DescriptorInstance;

class AppDescriptor;

/**
 * @class AppFactory
 * @brief 页面工厂类，单例模式
 *
 * 管理页面名称到 AppDescriptor 的映射。
 * 当 PageManager 调用 create("PageName") 时，遍历已注册的描述符，
 * 找到匹配项后调用其 create() 方法实例化页面对象。
 *
 * 依赖：PageFactory（父类）、AppDescriptor（描述符抽象类）
 */
class AppFactory : public PageFactory {
public:
    AppFactory();
    virtual ~AppFactory();
    virtual PageBase* create(const char* name) override;

    /**
     * @brief 注册一个页面描述符
     * @param name 页面名称（用于查找匹配）
     * @param descriptor 描述符指针（由 APP_DESCRIPTOR_DEF 宏自动传入）
     */
    void add(const char* name, AppDescriptor* descriptor);

    /** @brief 获取单例实例 */
    static AppFactory* getInstance()
    {
        static AppFactory instance;
        return &instance;
    }

    /**
     * @brief 将所有已注册的页面安装到指定的 PageManager
     *
     * 对每个注册的页面调用 manager->install(name, nullptr)，
     * 这样 PageManager 的页面池中就有该页面的占位，可以通过 push(name) 导航过去。
     */
    void installAll(PageManager* manager);

private:
    const char* _nameArray[APP_FACTORY_MAX_NUM];        // 页面名称数组
    AppDescriptor* _descriptorArray[APP_FACTORY_MAX_NUM]; // 对应的描述符指针数组
    int _num;                                             // 已注册的页面数量
};

/**
 * @class AppDescriptor
 * @brief 页面描述符抽象基类
 *
 * 每个页面类型对应一个 AppDescriptor 子类实例。
 * 构造函数自动将自己注册到 AppFactory 单例中。
 * create() 纯虚函数由 APP_DESCRIPTOR_DEF 宏生成的子类实现，
 * 直接 new 对应的页面对象。
 *
 * 依赖：AppFactory（构造函数需要 getInstance() 注册自身）
 */
class AppDescriptor {
public:
    /**
     * @brief 构造函数——自动向 AppFactory 注册
     * @param name 页面名称
     */
    AppDescriptor(const char* name)
    {
        AppFactory::getInstance()->add(name, this);
    }

    /** @brief 创建对应页面类的实例 */
    virtual PageBase* create() = 0;
};

#endif // APP_FACTORY_H

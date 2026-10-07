/**
 * @file PageManager.h
 * @brief 页面导航管理器——驱动页面生命周期状态机与切换动画
 *
 * 【核心职责】
 * - 管理页面导航栈（push / pop / replace / backHome）
 * - 驱动 PageBase 的 7 个生命周期状态流转（Load → Appear → Disappear → Unload）
 * - 执行页面切换动画（水平滑动、淡入淡出等）
 * - 维护页面对象池（PagePool），按名称查找页面实例
 *
 * 【状态机关键行为】
 * - 首次 push 页面时，PageManager 在 Pool 中找不到则通过工厂创建，执行 LOAD 流程
 * - stateLoad() 执行 onViewLoad() + onViewDidLoad()，完成后设置 isCached = true（默认缓存）
 * - pop() 会将栈顶页面的 isCached 设为 false，导致该页面进入 UNLOAD 而非缓存
 * - stateUnload() 先调用 onViewUnload()，再 lv_obj_clean(root) + lv_obj_del(root)
 *   因此 onViewUnload() 执行时页面控件仍存在，可在此清理订阅、释放资源
 *
 * 【关键注意事项】
 * - 如果页面在 onViewDidLoad() 中创建了 DataNode 订阅，必须在 onViewUnload() 中删除，
 *   否则 pop 返回后订阅节点残留，定时器回调访问已删除的 LVGL 控件 → 崩溃/卡死
 * - 页面根 lv_obj 在 PageBase 构造函数中创建，stateLoad() 中通过
 *   lv_obj_set_size(root, LV_PCT(100), LV_PCT(100)) 确保非零尺寸（LVGL v8 中普通
 *   lv_obj 默认 LV_SIZE_CONTENT = 0×0，必须显式设尺寸）
 * - 所有导航操作（push/pop/replace）内部都会检查 _animState.isBusy，动画进行中时
 *   新请求会被拒绝（返回 ERR_BUSY）。UI 按钮的快速连击需注意此限制
 *
 * 【架构位置】
 * HAL 层 → 框架层（本文件）→ 应用层（AppFactory + PageBase 子类）
 */

#ifndef FRAMEWORK_PAGEMANAGER_H
#define FRAMEWORK_PAGEMANAGER_H

#include "PageBase.h"
#include "PageDef.h"
#include "PageFactory.h"
#include <stack>
#include <vector>

class PageManager {
    friend class PageBase;  // PageBase 通过 navigate() 回调管理器的 push/pop/replace

public:
    /**
     * @brief 页面事件类型——通过 setEventCallback 注册的回调中接收
     *
     * 每当导航操作完成或状态变化时，PageManager 触发对应事件。
     * 回调签名：RES_TYPE callback(EVENT* event)
     *
     * 注意事项：
     * - 回调在导航操作的末尾同步执行，不应执行耗时操作
     * - PAGE_STATE_CHANGED 在每次状态机推进时触发，频率较高
     * - 回调返回 STOP_PROCESS 可中断当前流程
     */
    enum class EVENT_TYPE {
        PAGE_STATE_CHANGED,  // 页面状态变化（Load/Appear/Disappear/Unload 任一完成时）
        PAGE_PUSH,           // push() 操作完成
        PAGE_POP,            // pop() 操作完成
        PAGE_REPLACE,        // replace() 操作完成
        PAGE_BACK_HOME,      // backHome() 操作完成
    };

    /**
     * @brief 操作结果码——所有导航操作和安装操作的返回值
     *
     * 注意事项：
     * - 调用方应检查返回值，ERR_BUSY 表示动画进行中需稍后重试
     * - ERR_NOT_FOUND 通常意味着页面未通过 APP_DESCRIPTOR_DEF 注册
     * - OK 仅表示操作被接受并开始执行，不保证已执行完毕（动画异步）
     */
    enum class RES_TYPE {
        OK,                  // 操作成功
        STOP_PROCESS,        // 事件回调返回此值可中断状态机推进
        ERR_NOT_FOUND,       // 指定页面在 Pool 和 Factory 中均未找到
        ERR_DUPLICATE,       // 同名页面已安装
        ERR_BUSY,            // 动画进行中，拒绝新导航请求
        ERR_PARAM,           // 参数无效
        ERR_OPERATION,       // 操作不合法（如栈空时 pop）
        ERR_UNKNOWN,         // 未知错误
    };

    /**
     * @brief 事件结构体——传递给 EVENT_CALLBACK
     *
     * @param type     事件类型
     * @param param    事件附加参数（取决于事件类型，通常为 PageBase* 或 nullptr）
     * @param userData 用户注册回调时传入的自定义数据指针
     */
    struct EVENT {
        EVENT(EVENT_TYPE t, void* p, void* u)
            : type(t), param(p), userData(u) {}
        EVENT_TYPE type;
        void* param;
        void* userData;
    };

    /// 事件回调函数指针类型。返回 STOP_PROCESS 可中断状态机
    typedef RES_TYPE (*EVENT_CALLBACK)(EVENT* event);

public:
    /**
     * @brief 构造函数
     * @param factory 页面工厂指针（通常为 AppFactory::getInstance()）。
     *                传入 nullptr 则仅使用 registerPage() 手动注册的页面，
     *                push 时不会尝试通过工厂创建。
     *
     * 注意事项：
     * - 工厂的生命周期必须长于 PageManager，析构时不会 delete factory
     * - 推荐使用 AppFactory 单例作为参数
     */
    PageManager(PageFactory* factory = nullptr);

    /**
     * @brief 析构函数
     *
     * 销毁 PagePool 中所有页面（调用 delete），清空导航栈。
     * 注意事项：
     * - 不会 delete _factory（工厂生命周期由调用方管理）
     * - 不会自动调用页面的 onViewUnload()——清理应在 pop/backHome 时已完成
     */
    ~PageManager();

    /**
     * @brief 通过工厂安装页面（类名 → 工厂创建函数的映射）
     *
     * 调用工厂的 createApp(className) 创建 PageBase 实例，加入 PagePool。
     * 此后 push(className) 可直接从 Pool 获取已创建的实例。
     *
     * @param className 页面类名（与 APP_DESCRIPTOR_DEF 中一致）
     * @param appName   页面别名（可选），push 时可用别名替代类名。
     *                  传入 nullptr 则使用 className 作为唯一名称。
     * @return OK / ERR_DUPLICATE / ERR_NOT_FOUND
     *
     * 注意事项：
     * - 通常由 AppFactory::installAll() 批量调用，不需要手动调用
     * - 安装时即创建页面实例（包括其 lv_obj 根对象），但不执行 onViewLoad
     * - 重复安装同名页面返回 ERR_DUPLICATE
     */
    RES_TYPE install(const char* className, const char* appName = nullptr);

    /**
     * @brief 卸载页面——从 Pool 中移除并 delete
     * @param appName 页面名称（安装时的 className 或 appName）
     * @return OK / ERR_NOT_FOUND
     *
     * 注意事项：
     * - 如果页面当前在栈中（正在显示），卸载行为未定义，应避免
     * - 通常在应用生命周期结束时批量清理，运行时较少使用
     */
    RES_TYPE uninstall(const char* appName);

    /**
     * @brief 手动注册已创建的页面实例（不通过工厂）
     *
     * 适用于不需要工厂模式、手动管理页面实例的场景。
     * 与 install() 互斥：如果已通过工厂安装同名页面，registerPage 会失败。
     *
     * @param base 页面实例指针（PageManager 不接管所有权，析构时不 delete）
     * @param name 页面名称
     * @return OK / ERR_DUPLICATE
     */
    RES_TYPE registerPage(PageBase* base, const char* name);

    /**
     * @brief 手动注销页面
     * @param name 页面名称
     * @return OK / ERR_NOT_FOUND
     *
     * 注意：不会 delete 页面实例（因为 registerPage 不接管所有权）
     */
    RES_TYPE unregisterPage(const char* name);

    /**
     * @brief 替换当前页面（当前页 Unload，新页 Load）
     *
     * 与 push 的区别：replace 会销毁当前页面，不保留在栈中。
     * 适用场景：登录后跳转主页（不需要返回登录页）。
     *
     * @param name  目标页面名称
     * @param param 传递给目标页面 onViewLoad 的参数（可为 nullptr）
     * @return OK / ERR_BUSY / ERR_NOT_FOUND
     *
     * 注意事项：
     * - 动画进行中调用返回 ERR_BUSY（_animState.isBusy = true）
     * - 当前页会走完整 UNLOAD 流程（包括 onViewUnload）
     */
    RES_TYPE replace(const char* name, const PageBase::PARAM* param = nullptr);

    /**
     * @brief 推入新页面（当前页保留在栈中并 Disappear，新页 Load 并 Appear）
     *
     * 最常用的页面跳转方式。当前页面会被缓存（进入 DID_DISAPPEAR 状态），
     * 新页面执行完整 LOAD → WILL_APPEAR → DID_APPEAR 流程。
     *
     * @param name  目标页面名称
     * @param param 传递给目标页面 onViewDidLoad 的参数（可为 nullptr）
     * @return OK / ERR_BUSY / ERR_NOT_FOUND
     *
     * 注意事项：
     * - 栈深度无硬限制，但过多页面会耗尽内存
     * - 如果目标页面已在 Pool 中（之前被 pop 后缓存），则跳过 onViewLoad，
     *   直接从 WILL_APPEAR 开始
     */
    RES_TYPE push(const char* name, const PageBase::PARAM* param = nullptr);

    /**
     * @brief 弹出当前页面，返回上一页
     *
     * 当前页面被标记 isCached = false → 进入 UNLOAD 流程（调用 onViewUnload +
     * lv_obj_clean + lv_obj_del）。上一页从 DID_DISAPPEAR 恢复到 WILL_APPEAR →
     * DID_APPEAR。
     *
     * @return OK / ERR_BUSY / ERR_OPERATION（栈中只剩一页时拒绝）
     *
     * 【重要注意事项】
     * - pop() 会触发当前页面的 UNLOAD：onViewUnload() → lv_obj_clean(root)
     *   → lv_obj_del(root)。页面上的所有 LVGL 控件将被销毁
     * - 如果当前页面在 onViewDidLoad() 中创建了 DataNode 订阅，必须在
     *   onViewUnload() 中删除这些订阅节点！！！否则定时器回调会访问已释放的
     *   LVGL 控件，导致程序崩溃/卡死
     * - 动画期间新导航请求返回 ERR_BUSY
     * - 栈底页面（首页）不可弹出
     */
    RES_TYPE pop();

    /**
     * @brief 返回首页——清空整个导航栈，只保留栈底页面
     *
     * 所有中间页面被 Unload（走完整 UNLOAD 流程），栈底页面恢复到
     * WILL_APPEAR → DID_APPEAR。
     *
     * @return OK / ERR_BUSY
     *
     * 注意事项：
     * - 会逐个 Unload 栈中所有非栈底页面，每个页面都会执行 onViewUnload
     * - 动画进行中返回 ERR_BUSY
     */
    RES_TYPE backHome();

    /**
     * @brief 获取前一个页面的名称（栈中倒数第二个页面）
     * @return 页面名称字符串，栈中仅一页时返回 nullptr
     *
     * 常用于当前页面知道"我是从哪里来的"。
     */
    const char* getPagePrevName();

    /**
     * @brief 获取当前页面的名称（栈顶页面）
     * @return 页面名称字符串，栈中仅一页时返回 nullptr
     *
     * 用于当前页面知道"我是哪里"。
     */
    const char* getPageName();
    /**
     * @brief 获取当前导航栈深度
     * @return 栈中页面数量（首页也算一个）
     */
    size_t getStackDepth() { return _pageStack.size(); }

    /**
     * @brief 设置全局默认的页面切换动画类型
     *
     * 所有 push/pop/replace 操作默认使用此动画，除非页面自身覆盖了动画类型。
     *
     * @param anim 动画类型枚举（OVER_LEFT, OVER_RIGHT, OVER_TOP, OVER_BOTTOM,
     *             FADE_ON, FADE_OFF, NONE 等）
     * @param time 动画持续时间（毫秒），默认 PAGE_ANIM_TIME_DEFAULT
     * @param path 动画缓动曲线（lv_anim_path_t），默认 lv_anim_path_ease_out
     *
     * 注意事项：
     * - 必须在 push 之前调用，已开始的动画不受影响
     * - 设置为 NONE 可禁用全局动画（页面切换瞬间完成）
     * - 页面可通过重写 PageBase 的 getLoadAnimType() 覆盖此全局设置
     */
    void setGlobalLoadAnim(PAGE_ANIM anim, uint32_t time = PAGE_ANIM_TIME_DEFAULT,
                          lv_anim_path_cb_t path = PAGE_ANIM_PATH_DEFAULT);

    /**
     * @brief 设置所有页面的父容器
     *
     * 每个 PageBase 的根 lv_obj 会被创建为此 parent 的子对象。
     * 如果不设置，默认使用 lv_scr_act()（LVGL 默认屏幕对象）。
     *
     * @param par 父容器对象。传入 nullptr 则回退到 lv_scr_act()
     *
     * 注意事项：
     * - 应在创建任何页面之前设置，已创建页面的 parent 不会自动更新
     * - 典型场景：在 lv_scr_act() 之上创建一个全屏容器作为页面容器，
     *   便于在页面之上叠加全局弹窗、Toast 等
     */
    void setRootParent(lv_obj_t* par) { _rootParent = par; }

    /**
     * @brief 获取根父容器
     * @return _rootParent 非空则返回它，否则返回 lv_scr_act()
     */
    lv_obj_t* getRootParent() { return _rootParent ? _rootParent : lv_scr_act(); }

    /**
     * @brief 设置根对象的默认样式
     *
     * 每个页面在创建根 lv_obj 时应用此样式。
     * 如果不设置，页面根对象使用 LVGL 默认样式（透明背景、无边框）。
     *
     * @param style 样式指针。传入 nullptr 则清除默认样式
     *
     * 注意事项：
     * - 样式在 PageBase 构造函数中应用，因此必须在安装页面之前设置
     * - 通常在 main.cpp 的 app_init() 中，push 首页之前调用
     * - 样式指针生命周期需长于 PageManager
     */
    void setRootDefaultStyle(lv_style_t* style) { _rootDefaultStyle = style; }

    /**
     * @brief 设置顶层图层——弹窗、对话框等覆盖在页面之上的控件放置在此层
     *
     * @param obj 图层对象。传入 nullptr 则使用 lv_layer_top()
     *
     * 注意事项：
     * - 此图层在所有页面之上、系统光标之下
     * - 典型场景：Toast 通知、全局 Loading 遮罩、确认对话框
     */
    void setLayerTop(lv_obj_t* obj) { _layerTop = obj; }

    /**
     * @brief 获取顶层图层
     * @return _layerTop 非空则返回它，否则返回 lv_layer_top()
     */
    lv_obj_t* getLayerTop() { return _layerTop ? _layerTop : lv_layer_top(); }

    /**
     * @brief 设置页面事件回调
     *
     * 每次导航操作或页面状态变化时触发。可用于日志记录、埋点统计、全局状态同步。
     *
     * @param callback  回调函数指针（C 风格函数或静态 lambda）
     * @param userData  自定义数据指针，回调时通过 EVENT::userData 传回
     *
     * 注意事项：
     * - 回调在导航流程中同步执行，不应包含耗时操作
     * - 回调返回 STOP_PROCESS 可中断当前状态机推进
     * - 回调中不应再次调用 push/pop 等导航方法（会导致重入）
     *
     * 使用示例：
     * @code
     * pageManager->setEventCallback([](PageManager::EVENT* event) -> PageManager::RES_TYPE {
     *     printf("事件: %d, 页面: %s\n", (int)event->type,
     *            event->param ? ((PageBase*)event->param)->getName() : "null");
     *     return PageManager::RES_TYPE::OK;
     * }, nullptr);
     * @endcode
     */
    void setEventCallback(EVENT_CALLBACK callback, void* userData);

private:
    /**
     * @brief 动画参数——描述页面切入/切出的起止位置
     *
     * push 动画：新页面从 enter.start 滑到 enter.end，旧页面从 exit.start 滑到 exit.end
     * pop 动画：方向相反
     *
     * 坐标含义取决于动画类型：
     * - OVER_LEFT/RIGHT：水平 X 坐标（基于屏幕宽度偏移）
     * - OVER_TOP/BOTTOM：垂直 Y 坐标（基于屏幕高度偏移）
     * - FADE_ON/OFF：opacity 值
     */
    struct ANIM_PARAM {
        ANIM_PARAM() : enter { 0 }, exit { 0 } {}
        struct { int32_t start, end; } enter;  // 切入页面的起止值
        struct { int32_t start, end; } exit;    // 切出页面的起止值
    };

    /**
     * @brief 加载动画属性——封装动画参数 + 读写回调
     *
     * 不同动画类型通过不同的 setter/getter 操作 lv_obj 的不同属性：
     * - 滑动动画：setter = lv_obj_set_x, getter = lv_obj_get_x
     * - 淡入淡出：setter = lv_obj_set_style_opa, getter = lv_obj_get_style_opa
     *
     * push 和 pop 的 ANIM_PARAM 方向相反（如 push 从右滑入 → pop 向右滑出）
     */
    struct LOAD_ANIM_ATTR {
        LOAD_ANIM_ATTR() : setter(nullptr), getter(nullptr) {}
        void (*setter)(void*, int32_t);  // 设置动画属性值（如 lv_obj_set_x）
        int32_t (*getter)(void*);        // 读取动画属性值（如 lv_obj_get_x）
        ANIM_PARAM push;                 // push 操作的起止参数
        ANIM_PARAM pop;                  // pop 操作的起止参数（通常与 push 相反）
    };

private:
    PageFactory* _factory;               // 页面工厂（可选），用于按类名创建页面实例
    uint16_t _pageCnt;                   // 已创建的页面总数（用于生成默认名称）
    std::vector<PageBase*> _pagePool;    // 页面对象池：所有已安装的页面实例（含不在栈中的缓存页）
    std::stack<PageBase*> _pageStack;    // 导航栈：当前可见的页面层级，栈顶为当前页
    PageBase* _pagePrev;                 // 前一个页面（导航操作前的栈顶）
    PageBase* _pageCurrent;              // 当前页面（导航操作后的栈顶）

    /**
     * @brief 动画状态机上下文
     *
     * isSwitchReq：有新的导航请求等待执行（当前动画结束后处理）
     * isBusy：动画进行中，拒绝新的导航请求
     * isEntering：正在进入新页面（push/replace），还是离开当前页（pop）
     * current：当前页的动画属性（可由页面重写覆盖）
     * global：全局默认动画属性（由 setGlobalLoadAnim 设置）
     *
     * 注意事项：
     * - 动画期间 isBusy = true，所有 push/pop/replace 返回 ERR_BUSY
     * - 动画结束后 onSwitchAnimFinish 重置 isBusy，处理排队的请求
     */
    struct {
        bool isSwitchReq;
        bool isBusy;
        bool isEntering;
        PageBase::ANIM_ATTR current;
        PageBase::ANIM_ATTR global;
    } _animState;

    lv_obj_t* _rootParent;               // 页面根对象父容器（NULL = lv_scr_act()）
    lv_style_t* _rootDefaultStyle;       // 页面根对象默认样式（NULL = LVGL 默认）
    lv_obj_t* _layerTop;                 // 顶层图层（NULL = lv_layer_top()）

    EVENT_CALLBACK _eventCallback;       // 事件回调函数指针
    void* _eventUserData;                // 事件回调用户数据

private:
    /**
     * @brief 在页面池中按名称查找页面
     * @param name 页面名称
     * @return 找到返回 PageBase*，未找到返回 nullptr
     *
     * 遍历 _pagePool，匹配 PageBase::getAppName()
     */
    PageBase* findPageInPool(const char* name);

    /**
     * @brief 在导航栈中按名称查找页面
     * @param name 页面名称
     * @return 找到返回 PageBase*，未找到返回 nullptr
     *
     * 遍历 _pageStack（底层容器为 deque），匹配 PageBase::getAppName()。
     * 与 findPageInPool 的区别：Pool 包含所有页面（含已缓存不在栈中的），
     * Stack 仅包含当前导航路径上的页面。
     */
    PageBase* findPageInStack(const char* name);

    /**
     * @brief 获取栈顶页面
     * @return 栈非空返回栈顶 PageBase*，空栈返回 nullptr
     */
    PageBase* getStackTop();

    /**
     * @brief 获取栈顶之下一个页面（导航返回的目标页）
     * @return 栈中至少 2 个元素时返回倒数第二个，否则返回 nullptr
     *
     * 用于 pop() 操作时确定返回目标。
     */
    PageBase* getStackTopAfter();

    /**
     * @brief 清空导航栈
     * @param keepBottom 是否保留栈底页面（首页）
     *
     * backHome() 传 true（保留首页），其他场景传 false。
     * 被清出的页面不执行 Unload 流程，仅从栈中移除。
     * 配合 stateUnload() 使用：先 stateUnload 销毁 UI，再 clearStack 移除引用。
     */
    void clearStack(bool keepBottom = false);

    /**
     * @brief 强制卸载页面——跳过缓存检查，直接执行 UNLOAD 流程
     * @param base 要卸载的页面
     * @return OK / 错误码
     *
     * 调用链：stateUnload(base) → onViewUnload() → lv_obj_clean(root) → lv_obj_del(root)
     * 与 pop() 的区别：不走动画，不检查 isCached 标志。
     * 用于 replace() 中销毁旧页面。
     */
    RES_TYPE fourceUnload(PageBase* base);

    /**
     * @brief 根据动画类型枚举获取对应的动画属性配置
     * @param anim  动画类型
     * @param attr  输出参数——填充 setter/getter + push/pop 的起止坐标
     * @return OK / ERR_PARAM（不支持的动画类型）
     *
     * 内部实现根据 anim 类型计算 LOAD_ANIM_ATTR：
     * - OVER_LEFT：水平滑动，新页从右(+screen_w)滑到 0，旧页从 0 滑到左(-screen_w)
     * - OVER_RIGHT：水平滑动，方向相反
     * - OVER_TOP/BOTTOM：垂直滑动
     * - FADE_ON：透明度从 0 到 255
     * - NONE：瞬间切换（动画时间 0）
     */
    RES_TYPE getLoadAnimAttr(PAGE_ANIM anim, LOAD_ANIM_ATTR* attr);

    /**
     * @brief 初始化 lv_anim_t 的默认属性
     *
     * 设置动画执行周期 exec_cb、就绪回调 ready_cb、完成回调 set_cb
     */
    void animDefaultInit(lv_anim_t* a);

    /**
     * @brief 获取当前页面的加载动画属性
     *
     * 优先级：页面自定义 > 全局设置（_animState.current 在 switchAnimTypeUpdate 中确定）
     */
    RES_TYPE getCurrentLoadAnimAttr(LOAD_ANIM_ATTR* attr)
    {
        return getLoadAnimAttr(getCurrentLoadAnimType(), attr);
    }

    /**
     * @brief 获取当前生效的动画类型
     * @return 页面自定义类型（如果有）或全局默认类型
     */
    PAGE_ANIM getCurrentLoadAnimType() { return _animState.current.type; }

    /**
     * @brief 执行页面切换（核心方法）
     *
     * 所有导航操作（push/pop/replace/backHome）最终都调用此方法：
     * 1. 销毁当前页动画（如有残留）
     * 2. 更新 _pagePrev / _pageCurrent
     * 3. 对新旧页面调用 stateNext() 推进状态机
     * 4. 创建切换动画（switchAnimCreate）
     * 5. 动画完成后回调 onSwitchAnimFinish
     *
     * @param base       目标页面（即将显示的页面）
     * @param isEnterAct 是否执行进入动画（push=true, pop=false）
     * @param param      传递给页面的参数
     * @return OK / 错误码
     *
     * 注意事项：
     * - 调用前必须通过 switchReqCheck() 和 switchAnimStateCheck() 前置检查
     * - 动画异步执行，方法会立即返回 OK（不等动画完成）
     * - 动画期间 _animState.isBusy = true
     */
    RES_TYPE switchTo(PageBase* base, bool isEnterAct, const PageBase::PARAM* param = nullptr);

    /**
     * @brief 动画完成回调（静态函数）
     *
     * 1. 设置 _animState.isBusy = false
     * 2. 如果有排队请求（isSwitchReq），立即处理
     * 3. 发送对应 EVENT_TYPE 事件给用户回调
     */
    static void onSwitchAnimFinish(lv_anim_t* a);

    /**
     * @brief 创建页面切换的 lv_anim 动画
     *
     * 为切入页面和切出页面各创建一个 lv_anim_t，绑定到对应 setter/getter。
     * 动画同步执行（相同 duration、相同 path）。
     */
    void switchAnimCreate(PageBase* base);

    /**
     * @brief 确定动画类型优先级并更新 _animState.current
     *
     * 优先级：页面自定义（PageBase::_context.animAttr）> 全局设置（_animState.global）
     * 如果页面未自定义动画，则从 global 复制到 current
     */
    void switchAnimTypeUpdate(PageBase* base);

    /**
     * @brief 前置检查——是否可以发起导航请求
     * @return OK / ERR_BUSY
     *
     * 如果 _animState.isBusy，设置 isSwitchReq = true 作为排队标志，
     * 返回 ERR_BUSY 拒绝当前请求。动画结束后 onSwitchAnimFinish 会处理排队请求。
     */
    RES_TYPE switchReqCheck();

    /**
     * @brief 前置检查——动画状态是否合法
     * @return OK / ERR_OPERATION
     *
     * 防止在动画进行中修改动画参数。
     */
    RES_TYPE switchAnimStateCheck();

    /**
     * @brief 执行页面的 LOAD 状态
     *
     * 调用链：
     * 1. 如果根对象尚未设置尺寸，lv_obj_set_size(root, LV_PCT(100), LV_PCT(100))
     * 2. 调用 base->onViewLoad() —— 页面创建 LVGL 控件
     * 3. 调用 base->onViewDidLoad() —— 页面初始化数据、创建订阅
     * 4. 设置 _context.isCached = true（默认缓存，pop 时会被设为 false）
     *
     * @return STATE_LOAD 完成后的下一个状态（通常为 STATE_WILL_APPEAR）
     *
     * 注意事项：
     * - onViewLoad 和 onViewDidLoad 仅在页面首次创建时调用一次
     * - 缓存命中时跳过此状态，直接从 WILL_APPEAR 开始
     * - 根对象尺寸设置（LV_PCT(100), LV_PCT(100)）解决了 LVGL v8 中
     *   普通 lv_obj 默认 0×0 导致子控件不可见的问题
     */
    PageBase::STATE stateLoad(PageBase* base);

    /**
     * @brief 执行页面的 WILL_APPEAR 状态
     *
     * 调用 base->onViewWillAppear() —— 每次页面即将显示时都会调用。
     * 适用场景：刷新数据、更新 UI 状态（从其他页面返回时需要同步最新数据）。
     *
     * @return STATE_WILL_APPEAR 完成后的下一个状态（通常为 STATE_DID_APPEAR）
     *
     * 注意事项：
     * - 与 onViewDidLoad 不同，此方法每次进入页面都会调用
     * - 如果页面被缓存后再次进入（pop 的上一页恢复），跳过 Load 直接到这里
     */
    PageBase::STATE stateWillAppear(PageBase* base);

    /**
     * @brief 执行页面的 DID_APPEAR 状态
     *
     * 调用 base->onViewDidAppear() —— 页面显示完成后回调。
     * 适用场景：启动进入动画后的操作、获取焦点等。
     *
     * @return STATE_DID_APPEAR 完成后的下一个状态（通常为 STATE_ACTIVITY）
     */
    PageBase::STATE stateDidAppear(PageBase* base);

    /**
     * @brief 执行页面的 WILL_DISAPPEAR 状态
     *
     * 调用 base->onViewWillDisappear() —— 页面即将被覆盖/离开。
     * 适用场景：暂停动画、保存临时状态、停止定时刷新。
     *
     * @return STATE_WILL_DISAPPEAR 完成后的下一个状态（通常为 STATE_DID_DISAPPEAR）
     */
    PageBase::STATE stateWillDisappear(PageBase* base);

    /**
     * @brief 执行页面的 DID_DISAPPEAR 状态
     *
     * 调用 base->onViewDidDisappear() —— 页面已完全隐藏。
     *
     * 此状态有两个分支：
     * - 如果 isCached = true：页面缓存在 Pool 中（保留 lv_obj），下次进入从 WILL_APPEAR 开始
     * - 如果 isCached = false：进入 UNLOAD → lv_obj 被销毁，下次进入需重新 LOAD
     *
     * @return STATE_DID_DISAPPEAR 完成后的下一个状态
     *
     * 注意事项：
     * - pop() 会设置 isCached = false，导致页面 UNLOAD
     * - push() 的当前页默认 isCached = true（由 stateLoad 设置），页面被缓存
     */
    PageBase::STATE stateDidDisappear(PageBase* base);

    /**
     * @brief 执行页面的 UNLOAD 状态（关键）
     *
     * 调用链：
     * 1. 调用 base->onViewUnload() —— 【页面清理订阅的最后机会】
     * 2. lv_obj_clean(base->getRoot()) —— 删除根对象的所有子控件
     * 3. lv_obj_del(base->getRoot()) —— 删除根对象本身
     *
     * @return STATE_UNLOAD 完成后的下一个状态（通常为 STATE_IDLE）
     *
     * 【极其重要的注意事项】
     * - onViewUnload() 在 lv_obj_clean() 之前调用，此时页面控件仍存在
     * - 如果页面在 onViewDidLoad() 中创建了 DataNode 订阅/DataTimer，
     *   必须在 onViewUnload() 中 delete 它们！！否则这些对象在页面销毁后
     *   仍然存活，定时器回调会访问已删除的 LVGL 控件 → 未定义行为 → 崩溃/卡死
     * - 本项目中 DashboardPage 的冻结问题即由此引起
     * - onViewUnload 是页面最后一次安全访问其 LVGL 控件的机会
     * - 页面被 UNLOAD 后，如果再次 push，会重新走完整 LOAD 流程（重新创建控件）
     *
     * 规则：在 onViewDidLoad 中 new 了什么订阅/定时器，就在 onViewUnload 中 delete 什么。
     */
    PageBase::STATE stateUnload(PageBase* base);

    /**
     * @brief 推进页面到下一个状态
     *
     * 根据当前状态查找状态转移表，执行对应的 stateXxx() 方法。
     * 状态转移链：
     *   IDLE → stateLoad       → STATE_LOAD → ...
     *   LOAD → stateWillAppear → STATE_WILL_APPEAR → stateDidAppear → ...
     *   ...
     * 每次调用推进一个状态。switchTo() 中循环调用直到页面进入稳定状态。
     *
     * 注意事项：
     * - 如果当前状态没有注册转移（如 STATE_ACTIVITY），调用无效果
     * - 转移失败时页面停留在当前状态
     */
    void stateNext(PageBase* base);

    /**
     * @brief 获取当前页面的状态
     * @return PageBase::STATE 枚举值
     */
    PageBase::STATE getState() { return _pageCurrent->_context.state; }

    /**
     * @brief 发送事件给用户注册的回调
     *
     * @param eventType 事件类型
     * @param param     事件参数（通常为相关 PageBase*）
     * @return OK / STOP_PROCESS（如果回调返回 STOP_PROCESS）
     *
     * 如果用户未注册回调（_eventCallback == nullptr），直接返回 OK。
     * 回调返回 STOP_PROCESS 可中断当前状态机。
     */
    RES_TYPE sendEvent(EVENT_TYPE eventType, void* param = nullptr);
};

#endif  // FRAMEWORK_PAGEMANAGER_H

/**
 * @file PageBase.h
 * @brief 页面基类——定义页面生命周期、动画属性与参数传递
 *
 * 【页面生命周期状态机】
 *
 *   IDLE ──(首次 push)──→ LOAD ──→ WILL_APPEAR ──→ DID_APPEAR ──→ ACTIVITY
 *                              ←── 缓存命中（跳过 LOAD）──            │
 *                                                                      │
 *                           ┌──────────────────────────────────────────┘
 *                           ↓ (被新页面覆盖 / pop)
 *                      WILL_DISAPPEAR ──→ DID_DISAPPEAR
 *                                            ├── isCached=true  → WILL_APPEAR（再次进入时）
 *                                            └── isCached=false → UNLOAD → IDLE
 *
 * 【缓存行为关键规则】
 * - 页面首次加载后，stateLoad() 自动设置 isCached = true（默认自动缓存）
 * - push 新页面时，当前页被缓存（保留 lv_obj 树），下次 pop 回来跳过 LOAD 直接从 WILL_APPEAR 开始
 * - pop() 将栈顶页面的 isCached 设为 false → 进入 UNLOAD → lv_obj 被销毁，下次 push 重新走完整 LOAD 流程
 * - 调用 setCacheEnable(false) 或 setAutoCacheEnable(false) 可改变缓存策略
 *
 * 【订阅者生命周期——最重要的注意事项】
 * 如果页面在 onViewDidLoad() 中创建了 DataNode 订阅，必须在 onViewUnload() 中 delete 它们。
 * 因为 onViewUnload() 在 lv_obj_clean(root) 之前调用（此时控件仍存在），
 * 是清理订阅的最后安全时机。不清理会导致定时器回调访问已删除的 LVGL 控件 → 崩溃/卡死。
 * 规则：onViewDidLoad 中 new 了什么订阅/定时器，就在 onViewUnload 中 delete 什么。
 *
 * 【onViewDidLoad vs onViewWillAppear 的区别】
 * - onViewDidLoad：仅首次加载时调用一次。适合创建设备订阅、设置事件回调（一次性初始化）
 * - onViewWillAppear：每次页面显示前都调用。适合刷新数据、同步设备状态（缓存页再次进入时也会执行）
 *   典型场景：SettingsPage 在 onViewDidLoad 中从设备同步开关状态（因为 pop 后页面被 UNLOAD 重建）
 *
 * 【页面参数传递】
 * push/pop/replace 可通过 PARAM 传递参数。参数仅在本次导航的 LOAD 流程中有效，
 * 使用后即被清除。通过 PAGE_PARAM_MAKE 打包参数，PAGE_GET_PARAM 在页面内部解包。
 */

#ifndef FRAMEWORK_PAGEBASE_H
#define FRAMEWORK_PAGEBASE_H

#include "PageDef.h"
#include "framework_conf.h"

/**
 * @brief 打包页面参数——在 push/replace 调用点使用
 *
 * 将栈上或堆上的参数结构体打包为 PageBase::PARAM{ptr, size}。
 * 参数指向的内存必须在导航操作完成（动画结束）前保持有效。
 *
 * 使用示例：
 * @code
 * MyParam param = { .id = 42, .name = "hello" };
 * pageManager->push("DetailPage", PAGE_PARAM_MAKE(param));
 * @endcode
 *
 * 注意事项：
 * - 参数通过指针传递，不会拷贝。如果 param 是局部变量，需确保在动画完成前不被销毁
 * - 动画默认 300ms，通常局部变量在 push() 返回后仍有效（动画异步但调用者函数不会立即退出）
 * - 如果参数在堆上分配（new），接收方（页面）需负责 delete
 */
#define PAGE_PARAM_MAKE(PARAM)  \
    {                           \
        &(PARAM), sizeof(PARAM) \
    }

/**
 * @brief 在页面内部解包参数——通常在 onViewLoad() 或 onViewDidLoad() 中调用
 *
 * 从 PageBase 内部取出本次导航传入的参数，拷贝到局部变量。
 * 返回 false 表示参数不存在或大小不匹配。
 *
 * 使用示例：
 * @code
 * void DetailPage::onViewDidLoad()
 * {
 *     MyParam param;
 *     if (PAGE_GET_PARAM(param)) {
 *         printf("id=%d, name=%s\n", param.id, param.name);
 *     }
 * }
 * @endcode
 *
 * 注意事项：
 * - 参数仅在本次 LOAD 流程中有效，之后被清除
 * - 如果在 onViewWillAppear 中调用（缓存命中时跳过了 LOAD），可能取不到参数
 * - 建议在 onViewDidLoad 中读取并使用参数，不要在后续回调中依赖参数存在
 */
#define PAGE_GET_PARAM(PARAM) this->getParam(&(PARAM), sizeof(PARAM))

class PageManager;

class PageBase {
    friend class PageManager;  // PageManager 通过 stateXxx() 方法驱动本类的状态转移

public:
    /**
     * @brief 页面状态枚举——PageManager 通过 stateNext() 推进状态转移
     *
     * 状态流向（单向，不可逆）：
     *   IDLE → LOAD → WILL_APPEAR → DID_APPEAR → ACTIVITY
     *                ←──────────────────────────────────┘
     *                WILL_DISAPPEAR → DID_DISAPPEAR → UNLOAD → IDLE
     *                                   (缓存命中时跳回 WILL_APPEAR)
     *
     * 注意事项：
     * - 状态转移由 PageManager 完全控制，页面本身不应主动修改状态
     * - 每个状态有对应的虚函数回调（stateLoad → onViewLoad + onViewDidLoad）
     * - ACTIVITY 是稳定态：页面完全可见且空闲，等待用户交互
     */
    enum class STATE {
        IDLE,              // 初始状态：页面未加载或已被销毁
        LOAD,              // 加载中：创建 LVGL 控件、初始化数据（首次进入时）
        WILL_APPEAR,       // 即将显示：每次显示前都会经过（含缓存恢复）
        DID_APPEAR,        // 已显示：动画完成后的回调
        ACTIVITY,          // 活跃态：页面完全可见、可交互的稳定状态
        WILL_DISAPPEAR,    // 即将隐藏：暂停动画、保存状态
        DID_DISAPPEAR,     // 已隐藏：分支——isCached ? 缓存 : UNLOAD
        UNLOAD,            // 卸载中：销毁 LVGL 控件、释放资源
    };

public:
    /**
     * @brief 构造函数
     *
     * 初始化动作：
     * 1. 创建根 lv_obj（parent 由 PageManager::getRootParent() 决定）
     * 2. 应用 _rootDefaultStyle（如果 PageManager 设置过）
     * 3. 根对象初始不可见（lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN)），
     *    在 WILL_APPEAR 阶段才显示
     * 4. _context.state = IDLE
     * 5. _context.reqEnableCache = true（默认请求启用缓存）
     *
     * 注意事项：
     * - 不要在子类构造函数中创建 LVGL 控件——应在 onViewLoad() 中创建
     * - 构造函数在页面被 install/register 时调用（早于 onViewLoad）
     * - 根 lv_obj 默认大小为 LV_SIZE_CONTENT（0×0），stateLoad() 中会修正为
     *   LV_PCT(100) × LV_PCT(100)
     */
    PageBase();

    /**
     * @brief 虚析构函数
     *
     * 注意事项：
     * - 如果页面仍持有 DataNode/DataTimer 引用，析构前应已在 onViewUnload() 中清理
     * - PageManager 在析构时会 delete PagePool 中所有页面
     * - 不要在析构函数中访问 PageManager（_manager 可能已销毁）
     */
    virtual ~PageBase();

    /**
     * @brief 页面被安装到 PageManager 时回调（安装 ≠ 显示）
     *
     * 触发时机：PageManager::install() 或 registerPage() 完成后。
     * 此时页面实例已创建，根 lv_obj 已存在，但 onViewLoad 尚未调用。
     *
     * 适用场景：在首次显示之前做一些预初始化（极少使用）。
     * 注意事项：大多数初始化应放在 onViewLoad/onViewDidLoad 中，而不是这里。
     */
    virtual void onInstalled() { }

    /**
     * @brief 创建 LVGL 控件——页面首次加载时调用，仅一次
     *
     * 触发时机：页面从 IDLE 进入 LOAD 状态时，stateLoad() 调用。
     * 此时根 lv_obj 已存在但不可见（LV_OBJ_FLAG_HIDDEN），
     * 所有控件应创建为 getRoot() 的子对象。
     *
     * 典型操作：
     * - lv_btn_create(getRoot())——创建按钮
     * - lv_label_create(getRoot())——创建标签
     * - lv_obj_set_size / lv_obj_align——设置布局
     * - lv_obj_add_event_cb——绑定 LVGL 事件回调
     *
     * 注意事项：
     * - 仅调用一次（除非页面被 UNLOAD 后重新 push）
     * - 不要在这里创建 DataNode 订阅——应放在 onViewDidLoad() 中
     * - 不要在这里访问 g_appContext 中的设备数据——设备可能尚未初始化
     * - 此方法中创建的控件在 onViewUnload() 时被 lv_obj_clean() 批量销毁
     * - 如果页面需要缓存（isCached=true），控件树会一直保留在内存中
     * - 控件的事件回调中需要使用静态函数 + user_data 传递 this 指针
     */
    virtual void onViewLoad() { }

    /**
     * @brief 数据初始化——在 onViewLoad() 之后调用，仅一次
     *
     * 触发时机：stateLoad() 中 onViewLoad() 执行完毕后。
     * 此时所有 LVGL 控件已创建完成，可以安全地进行数据操作。
     *
     * 典型操作：
     * - 创建 DataNode 订阅（new DataNode + subscribe + setEventCallback）
     * - 创建 DataTimer
     * - 从 DeviceManager 查询设备初始状态并更新 UI
     * - 通过 PAGE_GET_PARAM 读取导航参数
     *
     * 【极其重要的注意事项】
     * - 此处 new 的 DataNode/DataTimer，必须在 onViewUnload() 中 delete！！
     *   这是本框架最易出错的点——忘记清理订阅导致 pop 后崩溃。
     * - 与 onViewWillAppear 的区别：onViewDidLoad 仅首次加载时调用一次，
     *   onViewWillAppear 每次显示前都调用（含缓存恢复）
     * - 如果页面永远不会被 UNLOAD（始终缓存），订阅只需创建一次
     * - 但为了安全，建议始终在 onViewUnload 中清理
     */
    virtual void onViewDidLoad() { }

    /**
     * @brief 页面即将显示——每次页面变为可见前都调用
     *
     * 触发时机：进入 WILL_APPEAR 状态时。
     * 以下两种情况都会触发：
     * 1. 首次 push（在 onViewLoad/onViewDidLoad 之后）
     * 2. 缓存恢复（pop 返回上一页，跳过 LOAD 直接到这里）
     *
     * 典型操作：
     * - 刷新列表数据（如 DeviceListPage 重建设备列表）
     * - 更新状态显示（如从 DeviceManager 重新查询设备启用状态）
     * - 恢复动画、重新开始定时刷新
     *
     * 注意事项：
     * - 不要每次都创建新的 DataNode 订阅——这会导致重复订阅
     * - 订阅的创建应放在 onViewDidLoad（仅一次），这里只刷新数据
     * - 如果页面总是被 UNLOAD（从不缓存），则每次进入都会走 onViewDidLoad，
     *   此时刷新逻辑放在 onViewDidLoad 中即可
     */
    virtual void onViewWillAppear() { }

    /**
     * @brief 页面已显示——显示动画完成后回调
     *
     * 触发时机：DID_APPEAR 状态。
     * 此时页面完全可见，切换动画（滑动/淡入）已执行完毕。
     *
     * 典型操作：
     * - 启动进入后的动画（如欢迎页面的文字逐字显示）
     * - 获取焦点（lv_group_focus_obj）
     * - 触发首次数据拉取
     */
    virtual void onViewDidAppear() { }

    /**
     * @brief 页面即将隐藏——被新页面覆盖或 pop 前调用
     *
     * 触发时机：进入 WILL_DISAPPEAR 状态。
     *
     * 典型操作：
     * - 暂停动画（如仪表盘的实时数据动画）
     * - 保存临时编辑状态（如用户正在输入但未提交的内容）
     * - 停止定时刷新（减少不必要的后台更新）
     *
     * 注意事项：
     * - 此方法在页面切换动画开始前调用
     * - 如果之后页面被 UNLOAD（pop），WILL_DISAPPEAR → DID_DISAPPEAR → UNLOAD
     * - 如果页面被缓存（push 新页），WILL_DISAPPEAR → DID_DISAPPEAR → 保留
     */
    virtual void onViewWillDisappear() { }

    /**
     * @brief 页面已隐藏——切换动画完成后回调
     *
     * 触发时机：DID_DISAPPEAR 状态。
     * 此时页面完全不可见。此状态的分支：
     * - isCached = true：页面缓存在 Pool 中，lv_obj 保留
     * - isCached = false：进入 UNLOAD，销毁 lv_obj
     */
    virtual void onViewDidDisappear() { }

    /**
     * @brief 页面卸载——销毁控件前调用（清理资源的最后机会）
     *
     * 触发时机：进入 UNLOAD 状态，在 lv_obj_clean(root) 之前调用。
     *
     * 【这是页面生命周期中最关键的方法】
     *
     * 必须在此处执行的操作：
     * - delete 所有在 onViewDidLoad() 中创建的 DataNode
     * - delete 所有在 onViewDidLoad() 中创建的 DataTimer
     * - 释放任何在 onViewLoad/onViewDidLoad 中分配的非 LVGL 资源
     *
     * 不需要执行的操作（框架自动处理）：
     * - 删除 LVGL 控件——stateUnload() 后续会调用 lv_obj_clean + lv_obj_del
     * - 取消 DataNode 订阅——DataNode 析构函数自动取消订阅并从 broker 移除
     *
     * 【为什么必须清理订阅】
     * 执行顺序：onViewUnload() → lv_obj_clean(root) → lv_obj_del(root)
     * - onViewUnload 执行时，LVGL 控件仍然存在（可以安全引用）
     * - lv_obj_clean 之后，所有控件被销毁
     * - 但如果 DataNode 订阅未删除，200ms 后 DataTimer 触发回调
     *   → 回调中 lv_label_set_text() 访问已删除的控件 → 崩溃/卡死
     * - DataNode 析构时会自动取消订阅，关键是必须 delete 它
     *
     * 典型实现：
     * @code
     * void MyPage::onViewUnload()
     * {
     *     for (int i = 0; i < DEVICE_COUNT; i++) {
     *         if (_subNodes[i]) {
     *             delete _subNodes[i];  // 析构自动取消订阅
     *             _subNodes[i] = nullptr;
     *         }
     *     }
     * }
     * @endcode
     */
    virtual void onViewUnload() { }

    /**
     * @brief 页面已卸载——UNLOAD 流程完全结束后回调
     *
     * 触发时机：onViewUnload() + lv_obj_clean + lv_obj_del 全部完成后。
     * 此时根对象已销毁，不能访问任何 LVGL 控件。
     * 页面状态回到 IDLE。
     *
     * 适用场景：日志记录、统计页面存活时间等（极少需要重写）。
     */
    virtual void onViewDidUnload() { }

    /**
     * @brief 设置页面缓存开关
     *
     * 当页面被新页面覆盖（push）时：
     * - enable = true：页面保留在内存中（缓存在 Pool），下次进入跳过 onViewLoad
     * - enable = false：页面被 UNLOAD，下次进入重新走完整 LOAD 流程
     *
     * @param en 是否启用缓存
     *
     * 注意事项：
     * - 此设置影响 push 时的行为，不影响 pop（pop 始终 UNLOAD 当前页）
     * - 如果同时设置了 setAutoCacheEnable(false)，则完全不缓存（pop 和 push 都不缓存）
     * - 默认缓存是启用的（_context.reqEnableCache = true）
     */
    void setCacheEnable(bool en);

    /**
     * @brief 设置自动缓存开关
     *
     * 自动缓存控制 stateLoad() 结束后是否自动设置 isCached = true。
     * - enable = true（默认）：stateLoad() 完成后 isCached = true，push 时页面被缓存
     * - enable = false：stateLoad() 完成后 isCached = false，push 时页面被 UNLOAD
     *
     * @param en 是否启用自动缓存
     *
     * 注意事项：
     * - 默认启用（reqDisableAutoCache = false），即每页首次加载后自动缓存
     * - pop() 内部通过设置 isDisableAutoCache = true 来禁止缓存当前页
     * - 如果设置 disableAutoCache = true，该页面每次被覆盖都会 UNLOAD 重建
     * - 适合内存敏感的页面（如图片浏览页）设置为 false
     */
    void setAutoCacheEnable(bool en);

    /**
     * @brief 设置页面切换动画类型（覆盖全局设置）
     *
     * 仅影响当前页面。设置后该页面的切入/切出动画将取代 PageManager 的全局动画。
     *
     * @param type 动画类型（PAGE_ANIM::OVER_LEFT, FADE_ON, NONE 等）
     *
     * 注意事项：
     * - 默认值为 PAGE_ANIM::GLOBAL（使用 PageManager 的全局设置）
     * - 必须在 push 之前调用，页面进入 LOAD 后修改无效
     * - 设置 NONE 可单独为某个页面禁用动画
     * - 通常在页面构造函数或 onInstalled() 中调用
     */
    void setLoadAnimType(PAGE_ANIM type);

    /**
     * @brief 设置页面切换动画持续时间（覆盖全局设置）
     *
     * @param time 动画持续时间（毫秒），默认 PAGE_ANIM_TIME_DEFAULT（300ms）
     *
     * 注意事项：
     * - 仅在 setLoadAnimType 为非 GLOBAL 时生效
     * - 时间越短切换越快但可能感觉生硬，太长时间会让用户觉得"卡"
     * - 300ms 是经过验证的合适值
     */
    void setLoadAnimTime(uint32_t time);

    /**
     * @brief 设置页面切换动画缓动曲线（覆盖全局设置）
     *
     * @param path LVGL 动画路径回调（lv_anim_path_ease_out, lv_anim_path_overshoot 等）
     *
     * 注意事项：
     * - 仅在 setLoadAnimType 为非 GLOBAL 时生效
     * - 默认 lv_anim_path_ease_out（快进慢出，视觉效果最自然）
     */
    void setLoadAnimPath(lv_anim_path_cb_t path);

    /**
     * @brief 设置返回手势方向
     *
     * 在支持手势的平台上（触摸屏），向指定方向滑动可触发 pop()。
     *
     * @param dir LVGL 方向枚举（LV_DIR_LEFT, LV_DIR_RIGHT, LV_DIR_TOP, LV_DIR_BOTTOM, LV_DIR_ALL, LV_DIR_NONE）
     *
     * 注意事项：
     * - 默认值为 LV_DIR_NONE（禁用手势返回）
     * - PC 模拟器上此设置通常无效果（鼠标而非触摸），需调试验证
     * - 与框架的返回按钮（代码调用 pop()）不冲突，两者并存
     */
    void setBackGestureDirection(lv_dir_t dir);

    /**
     * @brief 获取页面导航参数（内部使用——推荐用 PAGE_GET_PARAM 宏替代直接调用）
     *
     * 从 _context.param 拷贝数据到用户提供的缓冲区。
     * 参数仅在本次 LOAD 流程中有效，使用后由 PageManager 清除。
     *
     * @param ptr   用户缓冲区指针
     * @param size  期望的数据大小（必须与 PAGE_PARAM_MAKE 时一致）
     * @return true 参数存在且大小匹配，false 参数不存在或大小不匹配
     *
     * 注意事项：
     * - 直接使用 PAGE_GET_PARAM(param) 宏更方便，自动计算 sizeof
     * - 如果页面被缓存后再次进入（跳过 LOAD），参数可能为上次或空
     * - 页面被缓存时旧参数可能残留，不应在 onViewWillAppear 中依赖参数
     */
    bool getParam(void* ptr, uint32_t size);

    /**
     * @brief 获取所属的 PageManager 指针
     *
     * 页面通过此方法调用 PageManager 的导航方法：
     * @code
     * getManager()->pop();        // 返回上一页
     * getManager()->push("Xxx");  // 跳转到其他页面
     * @endcode
     *
     * 注意事项：
     * - 页面安装后 _manager 即被设置，卸载前始终有效
     * - 在页面构造函数中 _manager 可能为 nullptr（尚未安装）
     */
    PageManager* getManager() { return _manager; }

    /**
     * @brief 获取页面名称（安装时的 appName 或 className）
     *
     * 此名称与 push/pop/replace 中的 name 参数对应。
     * 用于日志输出、页面查找等场景。
     */
    const char* getName() { return _name; }

    /**
     * @brief 获取页面内部 ID
     *
     * 每个页面实例在 PageManager 中的唯一数字标识。
     * 按安装顺序递增，用于调试和日志区分同名页面的不同实例。
     */
    uint16_t getID() { return _id; }

    /**
     * @brief 获取页面的根 lv_obj 对象
     *
     * 所有页面控件应为根对象的子对象（直接或间接）。
     * 根对象由 PageBase 构造函数创建，在 stateLoad() 中设置尺寸，
     * 在 stateUnload() 中销毁。
     *
     * 注意事项：
     * - 根对象的初始尺寸为 0×0（LV_SIZE_CONTENT），stateLoad() 中修正为全屏
     * - 不要手动删除或替换根对象
     * - 根对象在创建时被隐藏（LV_OBJ_FLAG_HIDDEN），WILL_APPEAR 时显示
     */
    lv_obj_t* getRoot() { return _root; }

protected:
    /**
     * @brief 页面参数结构体——打包参数指针和大小
     *
     * 由 PAGE_PARAM_MAKE 宏创建，通过 push/replace 的 param 参数传递。
     * 参数指向的内存由调用方管理，页面通过 PAGE_GET_PARAM 读取。
     *
     * 注意事项：
     * - ptr 指向的原始数据不会被拷贝——必须保证在动画完成前有效
     * - size 用于校验：push 端和页面端的大小不匹配时 getParam 返回 false
     */
    struct PARAM {
        void* ptr;          // 参数数据指针
        uint32_t size;      // 参数数据大小（字节）
    };

private:
    /**
     * @brief 页面动画属性——可覆盖全局设置
     *
     * type = PAGE_ANIM::GLOBAL 时使用 PageManager 的全局动画设置。
     * 设置为其他值则覆盖全局设置，仅影响当前页面。
     */
    struct ANIM_ATTR {
        ANIM_ATTR()
            : type(PAGE_ANIM::GLOBAL)              // 默认跟随全局
            , duration(PAGE_ANIM_TIME_DEFAULT)     // 默认 300ms
            , path(PAGE_ANIM_PATH_DEFAULT)         // 默认 ease_out
        {
        }
        PAGE_ANIM type;              // 动画类型（GLOBAL 表示继承 PageManager 全局设置）
        uint32_t duration;           // 动画持续时间（毫秒）
        lv_anim_path_cb_t path;      // 动画缓动曲线
    };

    /**
     * @brief 页面运行时上下文——PageManager 通过此结构管理页面状态
     *
     * 各字段含义：
     * - reqEnableCache：用户通过 setCacheEnable 请求的缓存开关
     * - reqDisableAutoCache：用户通过 setAutoCacheEnable 请求禁用自动缓存
     * - isDisableAutoCache：实际的自动缓存禁用标志（pop 时由 PageManager 设置）
     * - isCached：当前是否已缓存（true = DISAPPEAR 后保留，false = DISAPPEAR 后 UNLOAD）
     * - param：本次导航传入的参数（使用后清除）
     * - state：当前生命周期状态
     * - anim.isEnter：是否正在执行进入动画
     * - anim.isBusy：动画进行中
     * - anim.attr：该页面的动画属性
     * - backGestureDir：返回手势方向
     *
     * 缓存相关字段的协作关系：
     * - stateLoad() 结束时：isCached = !isDisableAutoCache（默认 true）
     * - pop() 时：isDisableAutoCache = true → isCached = false → UNLOAD
     * - 用户调用 setAutoCacheEnable(false)：reqDisableAutoCache = true
     */
    struct CONTEXT {
        bool reqEnableCache;           // 用户请求启用缓存
        bool reqDisableAutoCache;      // 用户请求禁用自动缓存
        bool isDisableAutoCache;       // 实际自动缓存开关（PageManager pop 时动态修改）
        bool isCached;                 // 当前页面是否有缓存副本（决定 DISAPPEAR 后的分支走向）
        PARAM param;                   // 导航参数（仅本次 LOAD 有效）
        STATE state;                   // 当前生命周期状态
        struct {
            bool isEnter;              // 是否进入动画（true = 进入，false = 退出）
            bool isBusy;               // 动画进行中
            ANIM_ATTR attr;            // 本页动画属性
        } anim;
        lv_dir_t backGestureDir;       // 返回手势方向
    };

    PageManager* _manager;             // 所属 PageManager（安装时设置，卸载前有效）
    const char* _name;                 // 页面名称（安装时指定）
    uint16_t _id;                      // 页面内部 ID（按安装顺序递增）
    lv_obj_t* _root;                   // 根 lv_obj——所有页面控件的祖先
    CONTEXT _context;                  // 运行时上下文——PageManager 驱动状态机时读写
};

#endif  // FRAMEWORK_PAGEBASE_H

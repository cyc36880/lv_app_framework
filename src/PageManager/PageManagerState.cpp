/**
 * @file PageManagerState.cpp
 * @brief 页面生命周期状态机实现
 *
 * 【功能】
 * 实现 PageManager 的核心——页面状态机（stateNext）。
 * 页面在以下状态间迁移：
 *   IDLE → LOAD → WILL_APPEAR → DID_APPEAR → ACTIVITY
 *   ACTIVITY → WILL_DISAPPEAR → DID_DISAPPEAR → UNLOAD → IDLE
 *                                        ↘ (若缓存) → WILL_APPEAR
 *
 * 每个状态的切换由对应的 stateXxx() 函数处理，
 * 其中 stateLoad 和 stateDidDisappear 可能触发链式调用 stateNext()。
 *
 * 【关键修复】
 * stateLoad() 中为根对象设置 lv_obj_set_size(root_obj, LV_PCT(100), LV_PCT(100))，
 * 否则裸 lv_obj 的默认尺寸不足以填充父容器，导致子控件全部挤在左上角。
 */
#include "PageManager.h"
#include "PageLog.h"

void PageManager::stateNext(PageBase* base)
{
    if (!base) {
        return;
    }

    sendEvent(EVENT_TYPE::PAGE_STATE_CHANGED, (void*)&base->_context.state);

    switch (base->_context.state) {
    case PageBase::STATE::IDLE:
        PAGE_LOG_INFO("Page(%s) state idle", base->_name);
        break;

    case PageBase::STATE::LOAD:
        base->_context.state = stateLoad(base);
        stateNext(base);
        break;

    case PageBase::STATE::WILL_APPEAR:
        base->_context.state = stateWillAppear(base);
        break;

    case PageBase::STATE::DID_APPEAR:
        base->_context.state = stateDidAppear(base);
        PAGE_LOG_INFO("Page(%s) state active", base->_name);
        break;

    case PageBase::STATE::ACTIVITY:
        PAGE_LOG_INFO("Page(%s) state active break", base->_name);
        base->_context.state = PageBase::STATE::WILL_DISAPPEAR;
        stateNext(base);
        break;

    case PageBase::STATE::WILL_DISAPPEAR:
        base->_context.state = stateWillDisappear(base);
        break;

    case PageBase::STATE::DID_DISAPPEAR:
        base->_context.state = stateDidDisappear(base);
        if (base->_context.state == PageBase::STATE::UNLOAD) {
            stateNext(base);
        }
        break;

    case PageBase::STATE::UNLOAD:
        base->_context.state = stateUnload(base);
        break;

    default:
        PAGE_LOG_ERROR("Page(%s) state[%d] was NOT FOUND!", base->_name, base->_context.state);
        break;
    }
}

PageBase::STATE PageManager::stateLoad(PageBase* base)
{
    PAGE_LOG_INFO("Page(%s) state load", base->_name);

    if (base->_root != nullptr) {
        PAGE_LOG_ERROR("Page(%s) root must be nullptr", base->_name);
    }

    lv_obj_t* parent = base->_manager->getRootParent();
    lv_obj_t* root_obj = lv_obj_create(parent);

    /*
     * 根对象必须设置为填满父容器（屏幕），否则 LVGL v8 中裸 lv_obj 的默认
     * LV_SIZE_CONTENT 行为会使根对象大小接近 0×0，子控件通过 lv_obj_align()
     * 定位时全部堆叠在左上角。
     */
    lv_obj_remove_style_all(root_obj);
    lv_obj_set_size(root_obj, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(root_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_user_data(root_obj, base);

    if (base->_context.backGestureDir != LV_DIR_NONE) {
        lv_obj_clear_flag(root_obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
        lv_obj_add_event_cb(root_obj,
            [](lv_event_t* e) {
                auto self = (PageBase*)lv_event_get_user_data(e);
                auto dir = lv_indev_get_gesture_dir(lv_indev_get_act());
                if (dir == self->_context.backGestureDir) {
                    self->getManager()->pop();
                }
            },
            LV_EVENT_GESTURE,
            base);
    }

    if (_rootDefaultStyle) {
        lv_obj_add_style(root_obj, _rootDefaultStyle, LV_PART_MAIN);
    }

    base->_root = root_obj;
    base->onViewLoad();
    base->onViewDidLoad();

    if (base->_context.isDisableAutoCache) {
        PAGE_LOG_INFO("Page(%s) disable auto cache, reqEnableCache = %d",
                      base->_name, base->_context.reqEnableCache);
        base->_context.isCached = base->_context.reqEnableCache;
    } else {
        PAGE_LOG_INFO("Page(%s) AUTO cached", base->_name);
        base->_context.isCached = true;
    }

    return PageBase::STATE::WILL_APPEAR;
}

PageBase::STATE PageManager::stateWillAppear(PageBase* base)
{
    PAGE_LOG_INFO("Page(%s) state will appear", base->_name);
    base->onViewWillAppear();
    lv_obj_clear_flag(base->_root, LV_OBJ_FLAG_HIDDEN);
    return PageBase::STATE::DID_APPEAR;
}

PageBase::STATE PageManager::stateDidAppear(PageBase* base)
{
    PAGE_LOG_INFO("Page(%s) state did appear", base->_name);
    base->onViewDidAppear();
    return PageBase::STATE::ACTIVITY;
}

PageBase::STATE PageManager::stateWillDisappear(PageBase* base)
{
    PAGE_LOG_INFO("Page(%s) state will disappear", base->_name);
    base->onViewWillDisappear();
    return PageBase::STATE::DID_DISAPPEAR;
}

PageBase::STATE PageManager::stateDidDisappear(PageBase* base)
{
    PAGE_LOG_INFO("Page(%s) state did disappear", base->_name);
    lv_obj_add_flag(base->_root, LV_OBJ_FLAG_HIDDEN);
    base->onViewDidDisappear();
    if (base->_context.isCached) {
        PAGE_LOG_INFO("Page(%s) has cached", base->_name);
        return PageBase::STATE::WILL_APPEAR;
    } else {
        return PageBase::STATE::UNLOAD;
    }
}

PageBase::STATE PageManager::stateUnload(PageBase* base)
{
    PAGE_LOG_INFO("Page(%s) state unload", base->_name);
    if (!base->_root) {
        PAGE_LOG_WARN("Page is unloaded!");
        return PageBase::STATE::IDLE;
    }

    base->onViewUnload();
    if (base->_context.param.ptr != nullptr && base->_context.param.size != 0) {
        PAGE_LOG_INFO("Page(%s) free param(%p)[%d]", base->_name,
                      base->_context.param.ptr, base->_context.param.size);
        lv_mem_free(base->_context.param.ptr);
        base->_context.param.ptr = nullptr;
        base->_context.param.size = 0;
    }

    lv_obj_clean(base->_root);
    lv_obj_remove_style_all(base->_root);
    lv_obj_del_async(base->_root);
    base->_root = nullptr;
    base->_context.isCached = false;
    base->onViewDidUnload();
    return PageBase::STATE::IDLE;
}
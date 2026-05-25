/**
 * @file PageManagerAnim.cpp
 * @brief Page transition animation implementation
 */
#include "PageManager.h"
#include "PageLog.h"
#include <string.h>

PageManager::RES_TYPE PageManager::replace(const char* name, const PageBase::PARAM* param)
{
    auto res = sendEvent(EVENT_TYPE::PAGE_REPLACE, (void*)name);
    if (res != RES_TYPE::OK) return res;

    res = switchAnimStateCheck();
    if (res != RES_TYPE::OK) return res;

    if (findPageInStack(name) != nullptr) {
        PAGE_LOG_ERROR("Page(%s) was multi push", name);
        return RES_TYPE::ERR_DUPLICATE;
    }

    PageBase* base = findPageInPool(name);
    if (!base) {
        PAGE_LOG_ERROR("Page(%s) was not install", name);
        return RES_TYPE::ERR_NOT_FOUND;
    }

    PageBase* top = getStackTop();
    if (!top) {
        PAGE_LOG_ERROR("Stack top is NULL");
        return RES_TYPE::ERR_OPERATION;
    }

    top->_context.isCached = false;
    base->_context.isDisableAutoCache = base->_context.reqDisableAutoCache;
    _pageStack.pop();
    _pageStack.push(base);

    PAGE_LOG_INFO("Page(%s) replace Page(%s) (param = %p)", name, top->_name, param);
    return switchTo(base, true, param);
}

PageManager::RES_TYPE PageManager::push(const char* name, const PageBase::PARAM* param)
{
    auto res = sendEvent(EVENT_TYPE::PAGE_PUSH, (void*)name);
    if (res != RES_TYPE::OK) return res;

    res = switchAnimStateCheck();
    if (res != RES_TYPE::OK) return res;

    if (findPageInStack(name) != nullptr) {
        PAGE_LOG_ERROR("Page(%s) was multi push", name);
        return RES_TYPE::ERR_DUPLICATE;
    }

    PageBase* base = findPageInPool(name);
    if (!base) {
        PAGE_LOG_ERROR("Page(%s) was not install", name);
        return RES_TYPE::ERR_NOT_FOUND;
    }

    base->_context.isDisableAutoCache = base->_context.reqDisableAutoCache;
    _pageStack.push(base);
    PAGE_LOG_INFO("Page(%s) push >> [Screen] (param = %p)", name, param);
    return switchTo(base, true, param);
}

PageManager::RES_TYPE PageManager::pop()
{
    auto res = sendEvent(EVENT_TYPE::PAGE_POP);
    if (res != RES_TYPE::OK) return res;

    res = switchAnimStateCheck();
    if (res != RES_TYPE::OK) return res;

    if (_pageStack.size() <= 1) {
        PAGE_LOG_WARN("Bottom of stack, cat't pop");
        return RES_TYPE::ERR_OPERATION;
    }

    PageBase* top = getStackTop();
    LV_ASSERT_NULL(top);
    if (!top) {
        PAGE_LOG_ERROR("Stack top is NULL");
        return RES_TYPE::ERR_OPERATION;
    }

    if (!top->_context.isDisableAutoCache) {
        PAGE_LOG_INFO("Page(%s) has auto cache, cache disabled", top->_name);
        top->_context.isCached = false;
    }

    PAGE_LOG_INFO("Page(%s) pop << [Screen]", top->_name);
    _pageStack.pop();
    top = getStackTop();
    return switchTo(top, false, nullptr);
}

PageManager::RES_TYPE PageManager::switchTo(PageBase* newNode, bool isEnterAct,
                                            const PageBase::PARAM* param)
{
    if (!newNode) {
        PAGE_LOG_ERROR("newNode is nullptr");
        return RES_TYPE::ERR_PARAM;
    }

    if (_animState.isSwitchReq) {
        PAGE_LOG_WARN("Page switch busy, reqire(%s) is ignore", newNode->_name);
        return RES_TYPE::ERR_BUSY;
    }

    _animState.isSwitchReq = true;

    if (param != nullptr) {
        PAGE_LOG_INFO("param is detect, %s >> param(%p) >> %s",
                      getPagePrevName(), param, newNode->_name);

        void* buffer = nullptr;
        if (!newNode->_context.param.ptr) {
            buffer = lv_mem_alloc(param->size);
            LV_ASSERT_MALLOC(buffer);
            if (!buffer) {
                PAGE_LOG_ERROR("param malloc failed");
            } else {
                PAGE_LOG_INFO("param(%p) malloc[%d]", buffer, param->size);
            }
        } else if (newNode->_context.param.size == param->size) {
            buffer = newNode->_context.param.ptr;
            PAGE_LOG_INFO("param(%p) is exist", buffer);
        }

        if (buffer != nullptr) {
            memcpy(buffer, param->ptr, param->size);
            PAGE_LOG_INFO("param memcpy[%d] %p >> %p", param->size, param->ptr, buffer);
            newNode->_context.param.ptr = buffer;
            newNode->_context.param.size = param->size;
        }
    }

    _pageCurrent = newNode;

    if (_pageCurrent->_context.isCached) {
        PAGE_LOG_INFO("Page(%s) has cached, appear driectly", _pageCurrent->_name);
        _pageCurrent->_context.state = PageBase::STATE::WILL_APPEAR;
    } else {
        _pageCurrent->_context.state = PageBase::STATE::LOAD;
    }

    if (_pagePrev != nullptr) {
        _pagePrev->_context.anim.isEnter = false;
    }
    _pageCurrent->_context.anim.isEnter = true;
    _animState.isEntering = isEnterAct;

    if (_animState.isEntering) {
        switchAnimTypeUpdate(_pageCurrent);
    }

    stateNext(_pagePrev);
    stateNext(_pageCurrent);

    switchAnimCreate(_pagePrev);
    switchAnimCreate(_pageCurrent);

    if (_animState.isEntering) {
        PAGE_LOG_INFO("Page ENTER is detect, move Page(%s) to foreground", _pageCurrent->_name);
        if (_pagePrev) lv_obj_move_foreground(_pagePrev->_root);
        lv_obj_move_foreground(_pageCurrent->_root);
    } else {
        PAGE_LOG_INFO("Page EXIT is detect, move Page(%s) to foreground", getPagePrevName());
        lv_obj_move_foreground(_pageCurrent->_root);
        if (_pagePrev) lv_obj_move_foreground(_pagePrev->_root);
    }
    return RES_TYPE::OK;
}

PageManager::RES_TYPE PageManager::fourceUnload(PageBase* base)
{
    if (!base) {
        PAGE_LOG_ERROR("Page is nullptr, Unload failed");
        return RES_TYPE::ERR_PARAM;
    }

    PAGE_LOG_INFO("Page(%s) Fource unloading...", base->_name);
    if (base->_context.state == PageBase::STATE::ACTIVITY) {
        PAGE_LOG_INFO("Page state is ACTIVITY, Disappearing...");
        base->onViewWillDisappear();
        base->onViewDidDisappear();
    }
    base->_context.state = stateUnload(base);
    return RES_TYPE::OK;
}

PageManager::RES_TYPE PageManager::backHome()
{
    auto res = sendEvent(EVENT_TYPE::PAGE_BACK_HOME);
    if (res != RES_TYPE::OK) return res;

    res = switchAnimStateCheck();
    if (res != RES_TYPE::OK) return res;

    clearStack(true);
    _pagePrev = nullptr;
    PageBase* home = getStackTop();
    return switchTo(home, false);
}

PageManager::RES_TYPE PageManager::switchAnimStateCheck()
{
    if (_animState.isSwitchReq || _animState.isBusy) {
        PAGE_LOG_WARN("Page switch busy[AnimState.isSwitchReq = %d, AnimState.isBusy = %d], request ignored",
                      _animState.isSwitchReq, _animState.isBusy);
        return RES_TYPE::ERR_BUSY;
    }
    return RES_TYPE::OK;
}

PageManager::RES_TYPE PageManager::switchReqCheck()
{
    auto res = RES_TYPE::ERR_BUSY;
    bool lastNodeBusy = _pagePrev && _pagePrev->_context.anim.isBusy;

    if (!_pageCurrent->_context.anim.isBusy && !lastNodeBusy) {
        PAGE_LOG_INFO("----Page switch was all finished----");
        _animState.isSwitchReq = false;
        res = RES_TYPE::OK;
        _pagePrev = _pageCurrent;
    } else {
        if (_pageCurrent->_context.anim.isBusy) {
            PAGE_LOG_WARN("Page PageCurrent(%s) is busy", _pageCurrent->_name);
        } else {
            PAGE_LOG_WARN("Page PagePrev(%s) is busy", getPagePrevName());
        }
    }
    return res;
}

void PageManager::onSwitchAnimFinish(lv_anim_t* a)
{
    auto base = (PageBase*)lv_anim_get_user_data(a);
    auto manager = base->_manager;

    PAGE_LOG_INFO("Page(%s) anim finish", base->_name);
    manager->stateNext(base);
    base->_context.anim.isBusy = false;
    auto res = manager->switchReqCheck();

    if (!manager->_animState.isEntering && res == RES_TYPE::OK) {
        manager->switchAnimTypeUpdate(manager->_pageCurrent);
    }
}

void PageManager::switchAnimCreate(PageBase* base)
{
    if (!base) return;

    LOAD_ANIM_ATTR animAttr;
    if (getCurrentLoadAnimAttr(&animAttr) != RES_TYPE::OK) return;

    lv_anim_t a;
    animDefaultInit(&a);
    lv_anim_set_user_data(&a, base);
    lv_anim_set_var(&a, base->_root);
    lv_anim_set_ready_cb(&a, onSwitchAnimFinish);
    lv_anim_set_exec_cb(&a, animAttr.setter);

    int32_t start = 0;
    if (animAttr.getter) {
        start = animAttr.getter(base->_root);
    }

    if (_animState.isEntering) {
        if (base->_context.anim.isEnter) {
            lv_anim_set_values(&a, animAttr.push.enter.start, animAttr.push.enter.end);
        } else {
            lv_anim_set_values(&a, start, animAttr.push.exit.end);
        }
    } else {
        if (base->_context.anim.isEnter) {
            lv_anim_set_values(&a, animAttr.pop.enter.start, animAttr.pop.enter.end);
        } else {
            lv_anim_set_values(&a, start, animAttr.pop.exit.end);
        }
    }

    lv_anim_start(&a);
    base->_context.anim.isBusy = true;
}

void PageManager::setGlobalLoadAnim(PAGE_ANIM anim, uint32_t time, lv_anim_path_cb_t path)
{
    _animState.global.type = anim;
    _animState.global.duration = time;
    _animState.global.path = path;
    PAGE_LOG_INFO("Set global load anim type = %d", anim);
}

void PageManager::switchAnimTypeUpdate(PageBase* base)
{
    if (base->_context.anim.attr.type == PAGE_ANIM::GLOBAL) {
        PAGE_LOG_INFO("Page(%s) anim.type was not set, use AnimState.global.type = %d",
                      base->_name, _animState.global.type);
        _animState.current = _animState.global;
    } else {
        PAGE_LOG_INFO("Page(%s) custom anim.type set = %d",
                      base->_name, base->_context.anim.attr.type);
        _animState.current = base->_context.anim.attr;
    }
}

void PageManager::animDefaultInit(lv_anim_t* a)
{
    lv_anim_init(a);
    uint32_t time = (getCurrentLoadAnimType() == PAGE_ANIM::NONE) ? 0 : _animState.current.duration;
    lv_anim_set_time(a, time);
    lv_anim_set_path_cb(a, _animState.current.path);
}
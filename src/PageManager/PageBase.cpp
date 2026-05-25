/**
 * @file PageBase.cpp
 * @brief Implementation of PageBase class
 */
#include "PageBase.h"
#include "PageManager.h"
#include "PageLog.h"

PageBase::PageBase()
    : _manager(nullptr)
    , _name(nullptr)
    , _id(0)
    , _root(nullptr)
    , _context { 0 }
{
}

PageBase::~PageBase()
{
}

void PageBase::setCacheEnable(bool en)
{
    PAGE_LOG_INFO("Page(%s) enable %d", _name, en);
    setAutoCacheEnable(false);
    _context.reqEnableCache = en;
}

void PageBase::setAutoCacheEnable(bool en)
{
    PAGE_LOG_INFO("Page(%s) enable %d", _name, en);
    _context.reqDisableAutoCache = !en;
}

void PageBase::setLoadAnimType(PAGE_ANIM type)
{
    _context.anim.attr.type = type;
}

void PageBase::setLoadAnimTime(uint32_t time)
{
    _context.anim.attr.duration = time;
}

void PageBase::setLoadAnimPath(lv_anim_path_cb_t path)
{
    _context.anim.attr.path = path;
}

void PageBase::setBackGestureDirection(lv_dir_t dir)
{
    _context.backGestureDir = dir;
}

bool PageBase::getParam(void* ptr, uint32_t size)
{
    if (!_context.param.ptr) {
        PAGE_LOG_WARN("No param found");
        return false;
    }

    if (_context.param.size != size) {
        PAGE_LOG_WARN("param[%p](%" PRIu32 ") does not match the size(%" PRIu32 ")",
                      _context.param.ptr, _context.param.size, size);
        return false;
    }

    lv_memcpy(ptr, _context.param.ptr, _context.param.size);
    lv_mem_free(_context.param.ptr);
    _context.param.ptr = nullptr;
    return true;
}
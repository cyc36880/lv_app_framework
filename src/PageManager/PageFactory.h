/**
 * @file PageFactory.h
 * @brief Factory interface for creating page instances
 */
#ifndef FRAMEWORK_PAGEFACTORY_H
#define FRAMEWORK_PAGEFACTORY_H

#include "PageBase.h"

class PageFactory {
public:
    PageFactory() {};
    virtual ~PageFactory() {};

    /**
     * @brief Create a page by name
     * @param name Page class name
     * @return Pointer to the created page, or nullptr if not found
     */
    virtual PageBase* create(const char* name)
    {
        (void)name;
        return nullptr;
    }
};

#endif  // FRAMEWORK_PAGEFACTORY_H
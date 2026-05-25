/**
 * @file PageDef.h
 * @brief Page animation type definitions and defaults
 */
#ifndef FRAMEWORK_PAGEDEF_H
#define FRAMEWORK_PAGEDEF_H

#define PAGE_ANIM_TIME_DEFAULT 300  // [ms]
#define PAGE_ANIM_PATH_DEFAULT lv_anim_path_ease_out

/**
 * @brief Page switching animation type
 */
enum class PAGE_ANIM {
    GLOBAL = 0,    /**< Default (global) animation type */
    NONE,         /**< No animation */
    OVER_LEFT,    /**< New page overwrites old page from left */
    OVER_RIGHT,   /**< New page overwrites old page from right */
    OVER_TOP,     /**< New page overwrites old page from top */
    OVER_BOTTOM,  /**< New page overwrites old page from bottom */
    MOVE_LEFT,    /**< New page pushes old page to left */
    MOVE_RIGHT,   /**< New page pushes old page to right */
    MOVE_TOP,     /**< New page pushes old page to top */
    MOVE_BOTTOM,  /**< New page pushes old page to bottom */
    FADE_ON,      /**< The new interface fades in, old page fades out */
};

#endif  // FRAMEWORK_PAGEDEF_H
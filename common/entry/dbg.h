/*
 * Copyright 2011-2025 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#ifndef DBG_H_HEADER_GUARD
#define DBG_H_HEADER_GUARD

#include <bx/debug.h>
#include <cstring>

#define DBG_STRINGIZE(_x) DBG_STRINGIZE_(_x)
#define DBG_STRINGIZE_(_x) #_x
// #define DBG_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define DBG_FILE_LINE_LITERAL "" __FILE__ "(" DBG_STRINGIZE(__LINE__) "): "
// #define DBG(_format, ...) bx::debugPrintf(DBG_FILE_LINE_LITERAL "" _format "\n", ##__VA_ARGS__)

#define DBG(_format, ...)                                                                 \
    do {                                                                                  \
        const char* __file__ = strrchr(__FILE__, '/');                                    \
        if (!__file__) __file__ = strrchr(__FILE__, '\\');                                \
        __file__ = __file__ ? __file__ + 1 : __FILE__;                                     \
        bx::debugPrintf("%s(%d): " _format "\n", __file__, __LINE__, ##__VA_ARGS__);      \
    } while (0)

#endif // DBG_H_HEADER_GUARD

/*
 * New debug channel
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * Munkyu Im <munkyu.im@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 * refer to debug_ch.h
 * Copyright 1999 Patrik Stridvall
 *
 */

#ifndef __NEW_DEBUG_CH_H
#define __NEW_DEBUG_CH_H

#include <sys/types.h>

#define MAX_NAME_LEN    15
// #define NO_DEBUG

#ifdef __cplusplus
extern "C" {
#endif

enum _debug_class
{
    __DBCL_SEVERE,
    __DBCL_WARNING,
    __DBCL_INFO,
    __DBCL_CONFIG,
    __DBCL_FINE,
    __DBCL_TRACE,

    __DBCL_INIT = 7  /* lazy init flag */
};

struct _debug_channel
{
    unsigned char flags;
    char name[MAX_NAME_LEN + 1];
};

#ifndef NO_DEBUG
#define __GET_DEBUGGING_SEVERE(dbch)    ((dbch)->flags & (1 << __DBCL_SEVERE))
#define __GET_DEBUGGING_WARNING(dbch)   ((dbch)->flags & (1 << __DBCL_WARNING))
#define __GET_DEBUGGING_INFO(dbch)      ((dbch)->flags & (1 << __DBCL_INFO))
#define __GET_DEBUGGING_CONFIG(dbch)    ((dbch)->flags & (1 << __DBCL_CONFIG))
#define __GET_DEBUGGING_FINE(dbch)      ((dbch)->flags & (1 << __DBCL_FINE))
#define __GET_DEBUGGING_TRACE(dbch)     ((dbch)->flags & (1 << __DBCL_TRACE))
#else
#define __GET_DEBUGGING_SEVERE(dbch)    0
#define __GET_DEBUGGING_WARNING(dbch)   0
#define __GET_DEBUGGING_INFO(dbch)      0
#define __GET_DEBUGGING_CONFIG(dbch)    0
#define __GET_DEBUGGING_FINE(dbch)      0
#define __GET_DEBUGGING_TRACE(dbch)     0
#endif

#define __GET_DEBUGGING(dbcl,dbch)  __GET_DEBUGGING##dbcl(dbch)

#define __IS_DEBUG_ON(dbcl,dbch) \
    (__GET_DEBUGGING##dbcl(dbch) && \
     (_dbg_get_channel_flags(dbch) & (1 << __DBCL##dbcl)))

#define __DPRINTF(dbcl,dbch) \
    do{ if(__GET_DEBUGGING(dbcl,(dbch))){ \
        struct _debug_channel * const __dbch =(struct _debug_channel *)(dbch); \
        const enum _debug_class __dbcl = __DBCL##dbcl; \
        _DBG_LOG

#define _DBG_LOG(args...) \
        dbg_log(__dbcl, (struct _debug_channel *)(__dbch), args); } }while(0)

extern unsigned char _dbg_get_channel_flags(struct _debug_channel *channel);

extern int dbg_log(enum _debug_class cls, struct _debug_channel *ch,
        const char *format, ...);

#ifndef LOG_SEVERE
#define LOG_SEVERE              __DPRINTF(_SEVERE,&_dbch_)
#else
#error
#endif
#define IS_SEVERE_ON            __IS_DEBUG_ON(_SEVERE,&_dbch_)

#ifndef LOG_WARNING
#define LOG_WARNING             __DPRINTF(_WARNING,&_dbch_)
#else
#error
#endif
#define IS_WARNING_ON           __IS_DEBUG_ON(_WARNING,&_dbch_)

#ifndef LOG_INFO
#define LOG_INFO                __DPRINTF(_INFO,&_dbch_)
#else
#error
#endif
#define IS_INFO_ON              __IS_DEBUG_ON(_INFO,&_dbch_)

#ifndef LOG_CONFIG
#define LOG_CONFIG              __DPRINTF(_CONFIG,&_dbch_)
#else
#error
#endif
#define IS_CONFIG_ON            __IS_DEBUG_ON(_CONFIG,&_dbch_)

#ifndef LOG_FINE
#define LOG_FINE                __DPRINTF(_FINE,&_dbch_)
#else
#error
#endif
#define IS_FINE_ON              __IS_DEBUG_ON(_FINE,&_dbch_)

#ifndef LOG_TRACE
#define LOG_TRACE               __DPRINTF(_TRACE,&_dbch_)
#else
#error
#endif
#define IS_TRACE_ON             __IS_DEBUG_ON(_TRACE,&_dbch_)

#define DECLARE_DEBUG_CHANNEL(ch) \
    static struct _debug_channel _dbch_ = { ~0, #ch };

#ifdef __cplusplus
}
#endif

#endif // __NEW_DEBUG_CH_H

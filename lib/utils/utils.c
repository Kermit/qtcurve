/*****************************************************************************
 *   Copyright 2012 - 2013 Yichao Yu <yyc1992@gmail.com>                     *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU Lesser General Public License as          *
 *   published by the Free Software Foundation; either version 2.1 of the    *
 *   License, or (at your option) version 3, or any later version accepted   *
 *   by the membership of KDE e.V. (or its successor approved by the         *
 *   membership of KDE e.V.), which shall act as a proxy defined in          *
 *   Section 6 of version 3 of the license.                                  *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 *   Lesser General Public License for more details.                         *
 *                                                                           *
 *   You should have received a copy of the GNU Lesser General Public        *
 *   License along with this library. If not,                                *
 *   see <http://www.gnu.org/licenses/>.                                     *
 *****************************************************************************/

#include "utils.h"
#include "atomic.h"
#include <dlfcn.h>

#ifdef __QTC_ATOMIC_USE_SYNC_FETCH
/**
 * Also define lib function when there is builtin function for
 * atomic operation in case the function address is needed or the builtin
 * is not available when compiling other modules.
 **/
#define QTC_DEFINE_ATOMIC(name, op, type)               \
    QTC_EXPORT type                                     \
    (qtcAtomic##name)(volatile type *atomic, type val)  \
    {                                                   \
        return __qtcAtomic##name(atomic, val);          \
    }
#else
#include <pthread.h>
static pthread_mutex_t __qtc_atomic_lock = PTHREAD_MUTEX_INITIALIZER;
#define QTC_DEFINE_ATOMIC(name, op, type)               \
    QTC_EXPORT type                                     \
    (qtcAtomic##name)(volatile type *atomic, type val)  \
    {                                                   \
        type oldval;                                    \
        pthread_mutex_lock(&__qtc_atomic_lock);         \
        oldval = *atomic;                               \
        *atomic = oldval op val;                        \
        pthread_mutex_unlock(&__qtc_atomic_lock);       \
        return oldval;                                  \
    }
#endif

QTC_DEFINE_ATOMIC(Add, +, int32_t)
QTC_DEFINE_ATOMIC(And, &, uint32_t)
QTC_DEFINE_ATOMIC(Or, |, uint32_t)
QTC_DEFINE_ATOMIC(Xor, ^, uint32_t)

#undef QTC_DEFINE_ATOMIC

// Return the position of the element that is greater than or equal to the key.
QTC_EXPORT void*
qtcBSearch(const void *key, const void *base, size_t nmemb, size_t size,
           int (*compar)(const void*, const void*))
{
    size_t l = 0;
    size_t u = nmemb;
    while (l < u) {
        size_t idx = (l + u) / 2;
        const void *p = ((const char*)base) + (idx * size);
        int comparison = (*compar)(key, p);
        if (comparison == 0) {
            l = idx;
            break;
        } else if (comparison < 0) {
            u = idx;
        } else if (comparison > 0) {
            l = idx + 1;
        }
    }
    return (void*)(((const char*)base) + (l * size));
}

const char*
_qtcGetProgName()
{
    static char buff[1024] = "";
    void *hdl = dlopen(NULL, RTLD_NOW);
    const char *name = NULL;
    char *const *progname = NULL;
    const char *(*progname_func)() = NULL;
    // I do not use the procfs here since any system that has procfs (Linux)
    // should support one of these variables/functions.
    if (!hdl) {
        return "";
    } else if ((progname = dlsym(hdl, "program_invocation_short_name")) ||
               (progname = dlsym(hdl, "program_invocation_name")) ||
               (progname = dlsym(hdl, "__progname"))) {
        name = *progname;
    } else if ((progname_func = dlsym(hdl, "getprogname"))) {
        name = progname_func();
    }
    if (!name) {
        return "";
    }
    strncpy(buff, name, 1024);
    buff[1023] = '\0';
    buff[strcspn(buff, " ")] = '\0';
    if (qtcUnlikely(name = strrchr(buff, '/'))) {
        name++;
        if (*name) {
            return name;
        }
    }
    return buff;
}

QTC_EXPORT const char*
qtcGetProgName()
{
    static const char *name = NULL;
    if (!name) {
        name = _qtcGetProgName();
    }
    return name;
}

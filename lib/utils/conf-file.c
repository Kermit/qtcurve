/*****************************************************************************
 *   Copyright 2013 - 2013 Yichao Yu <yyc1992@gmail.com>                     *
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

#include "conf-file.h"

static size_t
_qtcConfValueSize(const QtcConfValueDesc *desc)
{
    const QtcConfConstrain *constrain = &desc->constrain;
    switch (desc->type) {
    default:
    case QTC_CONF_STR:
        if (!constrain->str_c.is_static ||
            qtcUnlikely(constrain->str_c.max_len <= 0))
            return sizeof(char*);
        return constrain->str_c.max_len + 1;
    case QTC_CONF_INT:
    case QTC_CONF_ENUM:
        return sizeof(int);
    case QTC_CONF_FLOAT:
        return sizeof(double);
    case QTC_CONF_BOOL:
        return sizeof(bool);
    case QTC_CONF_COLOR:
        return sizeof(QtcColor);
    case QTC_CONF_STR_LIST:
        if (!constrain->str_list_c.is_array_static ||
            qtcUnlikely(constrain->str_list_c.max_count <= 0))
            return sizeof(char**); // or sizeof(char*)
        if (!constrain->str_list_c.is_str_static ||
            qtcUnlikely(constrain->str_list_c.max_strlen <= 0))
            return sizeof(char*) * constrain->str_list_c.max_count;
        return ((constrain->str_list_c.max_strlen + 1) *
                constrain->str_list_c.max_count);
    case QTC_CONF_INT_LIST:
        if (!constrain->int_list_c.is_array_static ||
            qtcUnlikely(constrain->int_list_c.max_count <= 0))
            return sizeof(int*);
        return (sizeof(int) * constrain->int_list_c.max_count);
    case QTC_CONF_FLOAT_LIST:
        if (!constrain->float_list_c.is_array_static ||
            qtcUnlikely(constrain->float_list_c.max_count <= 0))
            return sizeof(double*);
        return (sizeof(double) * constrain->float_list_c.max_count);
    }
}

QTC_EXPORT size_t
qtcConfValueSize(const QtcConfValueDesc *desc)
{
    return qtcMax(_qtcConfValueSize(desc), sizeof(QtcConfValue));
}
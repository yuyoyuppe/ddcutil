/* glib_string_util.h
 *
 * Functions that depend on both glib_util.c and string_util.c
 *
 * glib_string_util.c/h exists to avoid circular dependencies.
 *
 * <copyright>
 * Copyright (C) 2014-2017 Sanford Rockowitz <rockowitz@minsoft.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * </endcopyright>
 */

/** @file glib_string_util.h
 * Functions that depend on both glib_util.c and string_util.c
 */

/** \cond */
#include <glib.h>
/** \endcond */

#ifndef GLIB_STRING_UTIL_H_
#define GLIB_STRING_UTIL_H_

char * join_string_g_ptr_array(GPtrArray * strings, char * sepstr);

int gaux_string_ptr_array_find(GPtrArray * haystack, const char * needle);

#endif /* GLIB_STRING_UTIL_H_ */

/** @file core.h
 * Core functions and global variables.
 *
 * File core.c provides a collection of inter-dependent services at the core
 * of the **ddcutil** application.
 *
 * These include
 * - message destination redirection
 * - abnormal termination
 * - standard function call options
 * - timestamp generation
 * - message level control
 * - debug and trace messages
 */

/* core.h
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

#ifndef BASE_CORE_H_
#define BASE_CORE_H_

/** \cond */
#include <climits>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
/** \endcond */

#include "public/ddcutil_types.h"

#include "util/coredefs.h"


// /** @addtogroup abnormal_termination
//  */

//
// Initialization
//
void init_msg_control();


//
// For aborting out of shared library
//
#ifdef OBSOLETE
void register_jmp_buf(jmp_buf * jb);
#endif

#ifdef UNUSED
void ddc_abort(const char * funcname, const int lineno, const char * fn, int status);

#define DDC_ABORT(status) ddc_abort(__func__, __LINE__, __FILE__, status)
#endif

#ifdef OBSOLETE
extern DDCA_Global_Failure_Information global_failure_information;
#endif


//
// Standard function call arguments and return values
//

/** Byte of standard call options */
typedef Byte Call_Options;
#define CALLOPT_NONE 0x00    ///< no options
#define CALLOPT_ERR_MSG 0x80 ///< issue message if error
// #define CALLOPT_ERR_ABORT    0x40    ///< terminate execution if error
#define CALLOPT_RDONLY 0x20      ///< open read-only
#define CALLOPT_WARN_FINDEX 0x10 ///< issue warning msg re hiddev_field_info.field_index change
#define CALLOPT_FORCE 0x08       ///< ignore various validity checks

char * interpret_call_options_t(Call_Options calloptions);

#ifdef FUTURE
// A way to return both a status code and a pointer.
// Has the benefit of avoiding the "Something** parm" construct,
// but requires a cast by the caller, so loses type checking.
// How useful will this be?

typedef struct
{
  int    rc;
  void * result;
} RC_And_Result;
#endif


//
// Global redirection for messages that normally go to stdout and stderr,
// used within functions that are part of the shared library.
//
extern FILE * FOUT;
extern FILE * FERR;

void set_fout(FILE * fout);
void set_ferr(FILE * ferr);
void set_fout_to_default();
void set_ferr_to_default();


//
// Message level control
//

DDCA_Output_Level get_output_level();
void              set_output_level(DDCA_Output_Level newval);
char *            output_level_name(DDCA_Output_Level val);


//
// Trace message control
//

extern bool dbgtrc_show_time; // include elapsed time in debug/trace timestamps

void add_traced_function(const char * funcname);
bool is_traced_function(const char * funcname);
void show_traced_functions();

void add_traced_file(const char * filename);
bool is_traced_file(const char * filename);
void show_traced_files();

typedef enum {
  TRC_BASE = 0x80,
  TRC_I2C  = 0x40,
  TRC_ADL  = 0x20,
  TRC_DDC  = 0x10,
  TRC_USB  = 0x08,
  TRC_TOP  = 0x04,
  TRC_ENV  = 0x02,

  TRC_NEVER  = 0x00,
  TRC_ALWAYS = 0xff
} Trace_Group;

#define TRC_NONE TRC_NEVER

Trace_Group trace_class_name_to_value(char * name);
void        set_trace_levels(Trace_Group trace_flags);
char *      get_active_trace_group_names();
void        show_trace_groups();

bool is_tracing(Trace_Group trace_group, const char * filename, const char * funcname);

/** Checks if tracking is currently active for the globally defined TRACE_GROUP value,
 *  current file and function.
 *
 *  Wrappers call to **is_tracing()**, using the current **TRACE_GROUP** value,
 *  filename, and function as implicit arguments.
 */
#define IS_TRACING() is_tracing(TRACE_GROUP, __FILE__, __func__)

#define IS_TRACING_GROUP(grp) is_tracing(grp, __FILE__, __func__)

#define IS_TRACING_BY_FUNC_OR_FILE() is_tracing(TRC_NEVER, __FILE__, __func__)


//
//  Error_Info reporting
//

extern bool report_freed_exceptions;


//
// DDC data error reporting
//

// Controls display of messages regarding I2C error conditions that can be retried.
extern bool report_ddc_errors;

bool is_reporting_ddc(Trace_Group trace_group, const char * filename, const char * funcname);
#define IS_REPORTING_DDC() is_reporting_ddc(TRACE_GROUP, __FILE__, __func__)

bool ddcmsg(Trace_Group trace_group, const char * funcname, const int lineno, const char * fn, char * format, ...);
#define DDCMSG0(format, ...) ddcmsg(TRACE_GROUP, __func__, __LINE__, __FILE__, format, ##__VA_ARGS__)

/** Variant of **DDCMSG** that takes an explicit trace group as an argument.
 *
 * @param debug_flag
 * @param trace_group
 * @param format
 * @param ...
 */
#define DDCMSGX(debug_flag, trace_group, format, ...)                                                                  \
  ddcmsg(((debug_flag)) ? 0xff : (trace_group), __func__, __LINE__, __FILE__, format, ##__VA_ARGS__)


/** Macro that wrappers function **ddcmsg()**, passing the current TRACE_GROUP,
 *  file name, line number, and function name as arguments.
 *
 * @param debug_flag
 * @param format
 * @param ...
 */
#define DDCMSG(debug_flag, format, ...)                                                                                \
  ddcmsg(((debug_flag)) ? 0xff : (TRACE_GROUP), __func__, __LINE__, __FILE__, format, ##__VA_ARGS__)


// Show report levels for all types
void show_reporting();


//
// Issue messages of various types
//

void severemsg(const char * funcname, const int lineno, const char * fn, char * format, ...);

bool dbgtrc(Trace_Group trace_group, const char * funcname, const int lineno, const char * fn, char * format, ...);


#define SEVEREMSG(format, ...) severemsg(__func__, __LINE__, __FILE__, format, ##__VA_ARGS__)


// cannot map to dbgtrc, writes to stderr, not stdout
// #define SEVEREMSG(format, ...) dbgtrc(0xff,       __func__, __LINE__, __FILE__, format, ##__VA_ARGS__)

#define DBGMSG(format, ...) dbgtrc(0xff, __func__, __LINE__, __FILE__, format, ##__VA_ARGS__)
// workaround: if -Wpedantic, ISO C does not allow variadic macro calls that have no variable arguments
#define DBGMSG0(text) dbgtrc(0xff, __func__, __LINE__, __FILE__, text)

#define DBGMSF(debug_flag, format, ...)                                                                                \
  do                                                                                                                   \
  {                                                                                                                    \
    if(debug_flag)                                                                                                     \
      dbgtrc(0xff, __func__, __LINE__, __FILE__, format, ##__VA_ARGS__);                                               \
  } while(0)
#define DBGMSF0(debug_flag, text)                                                                                      \
  do                                                                                                                   \
  {                                                                                                                    \
    if(debug_flag)                                                                                                     \
      dbgtrc(0xff, __func__, __LINE__, __FILE__, text);                                                                \
  } while(0)


#define TRCMSG(format, ...) dbgtrc(TRACE_GROUP, __func__, __LINE__, __FILE__, format, ##__VA_ARGS__)

// which of these are really useful?
// not currently used: TRCALWAYS, TRCMSGTG, TRCMSGTF
#define TRCALWAYS(format, ...) dbgtrc(0xff, __func__, __LINE__, __FILE__, format, ##__VA_ARGS__)
#define TRCMSGTG(trace_group, format, ...) dbgtrc(trace_group, __func__, __LINE__, __FILE__, format, ##__VA_ARGS__)
#define TRCMSGTF(trace_flag, format, ...)                                                                              \
  do                                                                                                                   \
  {                                                                                                                    \
    if(trace_flag)                                                                                                     \
      dbgtrc(0xff, __func__, __LINE__, __FILE__, format, ##__VA_ARGS__);                                               \
  } while(0)
// alt: dbgtrc( ( (trace_flag) ? (0xff) : TRACE_GROUP ), __func__, __LINE__, __FILE__, format, ##__VA_ARGS__)

// For messages that are issued either if tracing is enabled for the appropriate trace group or
// if a debug flag is set.
#define DBGTRC(debug_flag, trace_group, format, ...)                                                                   \
  dbgtrc(((debug_flag)) ? 0xff : (trace_group), __func__, __LINE__, __FILE__, format, ##__VA_ARGS__)

#define DBGTRC0(debug_flag, trace_group, format)                                                                       \
  dbgtrc(((debug_flag)) ? 0xff : (trace_group), __func__, __LINE__, __FILE__, format)


//
// Error handling
//

#ifdef OLD
void report_ioctl_error_old(int errnum, const char * funcname, int lineno, char * filename, bool fatal);
#endif

void report_ioctl_error(const char * ioctl_name, int errnum, const char * funcname, const char * filename, int lineno);

#define REPORT_IOCTL_ERROR(_ioctl_name, _errnum) report_ioctl_error(_ioctl_name, _errnum, __func__, __FILE__, __LINE__);

// reports a program logic error
void program_logic_error(const char * funcname, const int lineno, const char * fn, char * format, ...);

/** @def PROGRAM_LOGIC_ERROR(format,...)
 *  Wraps call to program_logic_error()
 *
 *  Reports an error in program logic.
 * @ingroup abnormal_termination
 */
#define PROGRAM_LOGIC_ERROR(format, ...) program_logic_error(__func__, __LINE__, __FILE__, format, ##__VA_ARGS__)


#ifdef OLD
void terminate_execution_on_error(
  Trace_Group trace_group, const char * funcname, const int lineno, const char * fn, char * format, ...);

/** @def TERMINATE_EXECUTION_ON_ERROR(format,...)
 *  Wraps call to terminate_execution_on_error()
 * @ingroup abnormal_termination
 */
#define TERMINATE_EXECUTION_ON_ERROR(format, ...)                                                                      \
  terminate_execution_on_error(TRACE_GROUP, __func__, __LINE__, __FILE__, format, ##__VA_ARGS__)
#endif

#endif /* BASE_CORE_H_ */

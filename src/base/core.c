/* core.c
 *
 * <copyright>
 * Copyright (C) 2014-2016 Sanford Rockowitz <rockowitz@minsoft.com>
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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "util/file_util.h"
#include "util/string_util.h"

#include "base/linux_errno.h"

#include "base/core.h"


//
// Timestamp Generation
//

// For debugging timestamp generation, maintain a timestamp history.
bool  tracking_timestamps = false;    // set true to enable timestamp history
#define MAX_TIMESTAMPS 1000
static long  timestamp[MAX_TIMESTAMPS];
static int   timestamp_ct = 0;


/* Returns the current value of the realtime clock in nanoseconds.
 * If debugging timestamp generation, remember the timestamp as well.
 *
 * Arguments:   none
 * Returns:     timestamp, in nanoseconds
 */
long cur_realtime_nanosec() {
   struct timespec tvNow;
   clock_gettime(CLOCK_REALTIME, &tvNow);
   // long result = (tvNow.tv_sec * 1000) + (tvNow.tv_nsec / (1000 * 1000) );  // milliseconds
   // long result = (tvNow.tv_sec * 1000 * 1000) + (tvNow.tv_nsec / 1000);     // microseconds
   long result = tvNow.tv_sec * (1000 * 1000 * 1000) + tvNow.tv_nsec;          // NANOSEC
   if (tracking_timestamps && timestamp_ct < MAX_TIMESTAMPS)
      timestamp[timestamp_ct++] = result;
   // printf("(%s) Returning: %ld\n", result);
   return result;
}


/* Reports history of generated timestamps
 */
void show_timestamp_history() {
   if (tracking_timestamps) {
      DBGMSG("total timestamps: %d", timestamp_ct);
      bool monotonic = true;
      int ctr = 0;
      for (; ctr < timestamp_ct; ctr++) {
         printf("  timestamp[%d] =  %15ld\n", ctr, timestamp[ctr] );
         if (ctr > 0 && timestamp[ctr] <= timestamp[ctr-1]) {
            printf("   !!! NOT STRICTLY MONOTONIC !!!\n");
            monotonic = false;
         }
      }
      printf("Timestamps are%s strictly monotonic\n", (monotonic) ? "" : " NOT");
   }
   else
      DBGMSG("Not tracking timestamps");
}


//
// Global SDTOUT and STDERR redirection, for controlling message output in API
//

FILE * FOUT = NULL;
FILE * FERR = NULL;

void init_msg_control() {
   FOUT = stdout;
   FERR = stderr;
}

void set_fout(FILE * fout) {
   FOUT = fout;
}

void set_ferr(FILE * ferr) {
   FERR = ferr;
}


// Local definitions and functions shared by all message control categories

#define SHOW_REPORTING_TITLE_START 0
#define SHOW_REPORTING_MIN_TITLE_SIZE 28


static void
print_simple_title_value(int    offset_start_to_title,
                         char * title,
                         int    offset_title_start_to_value,
                         char * value)
{
   printf("%.*s%-*s%s\n",
          offset_start_to_title,"",
          offset_title_start_to_value, title,
          value);
}


//
// Message level control for normal output
//

static Output_Level output_level;

Output_Level get_output_level() {
   return output_level;
}

void set_output_level(Output_Level newval) {
   // printf("(%s) newval=%s  \n", __func__, msgLevelName(newval) );
   output_level = newval;
}

char * output_level_name(Output_Level val) {
   char * result = NULL;
   switch (val) {
      case OL_DEFAULT:
         result = "Default";
         break;
      case OL_PROGRAM:
         result = "Program";
         break;
      case OL_TERSE:
         result = "Terse";
         break;
      case OL_NORMAL:
         result = "Normal";
         break;
      case OL_VERBOSE:
         result = "Verbose";
         break;
      default:
         PROGRAM_LOGIC_ERROR("Invalid Output_Level value: %d", val);
   }
   // printf("(%s) val=%d 0x%02x, returning: %s\n", __func__, val, val, result);
   return result;
}

void show_output_level() {
   // printf("Output level:           %s\n", output_level_name(output_level));
   print_simple_title_value(SHOW_REPORTING_TITLE_START,
                              "Output level: ",
                              SHOW_REPORTING_MIN_TITLE_SIZE,
                              output_level_name(output_level));
}


//
// Debug trace message control
//

#ifdef REFERENCE
typedef Byte Trace_Group;
#define TRC_BASE 0x80
#define TRC_I2C  0x40
#define TRC_ADL  0x20
#define TRC_DDC  0x10
#define TRC_USB  0x08
#define TRC_TOP  0x04
#endif

// same order as flags in TraceGroup
const Byte   trace_group_ids[]   = {TRC_BASE, TRC_I2C, TRC_ADL, TRC_DDC, TRC_USB, TRC_TOP};
const char * trace_group_names[] = {"BASE",   "I2C",   "ADL",   "DDC",   "USB",   "TOP"};
const int    trace_group_ct = sizeof(trace_group_names)/sizeof(char *);

Trace_Group trace_class_name_to_value(char * name) {
   Trace_Group trace_group = 0x00;
   int ndx = 0;
   for (; ndx < trace_group_ct; ndx++) {
      if (strcmp(name, trace_group_names[ndx]) == 0) {
         trace_group = 0x01 << (7-ndx);
      }
   }
   // printf("(%s) name=|%s|, returning 0x%2x\n", __func__, name, traceGroup);

   return trace_group;
}

static Byte trace_levels = 0x00;

void set_trace_levels(Trace_Group trace_flags) {
   bool debug = false;
   if (debug)
      DBGMSG("trace_flags=0x%02x\n", trace_flags);
   trace_levels = trace_flags;
}

// n. takes filename parm but used only in debug message, not result calculation
bool is_tracing(Trace_Group trace_group, const char * filename) {
   bool result =  (trace_group == 0xff) || (trace_levels & trace_group); // is trace_group being traced?
   // printf("(%s) traceGroup = %02x, filename=%s, traceLevels=0x%02x, returning %d\n",
   //        __func__, traceGroup, filename, traceLevels, result);
   return result;
}

void show_trace_groups() {
   char buf[100] = "";
   int ndx;
   for (ndx=0; ndx< trace_group_ct; ndx++) {
      if ( trace_levels & trace_group_ids[ndx]) {
         if (strlen(buf) > 0)
            strcat(buf, ", ");
         strcat(buf, trace_group_names[ndx]);
      }
   }
   if (strlen(buf) == 0)
      strcpy(buf,"none");
   // printf("Trace groups active:      %s\n", buf);
   print_simple_title_value(SHOW_REPORTING_TITLE_START,
                              "Trace groups active: ",
                              SHOW_REPORTING_MIN_TITLE_SIZE,
                              buf);
}


//
// Report DDC data errors
//

// global variable - controls display of messages regarding DDC data errors
bool show_recoverable_errors = true;

// Normally wrapped in macro IS_REPORTING_DDC
bool is_reporting_ddc(Trace_Group traceGroup, const char * fn) {
  bool result = (is_tracing(traceGroup,fn) || show_recoverable_errors);
  return result;
}


/* Submits a message regarding a DDC data error for possible output.
 * Whether a message is actually output depends on whether DDC errors are
 * being shown and (currently unimplemented) the trace group for the
 * message.
 *
 * Normally, invocation of this function is wrapped in macro DDCMSG.
 */
void ddcmsg(Trace_Group  traceGroup,
            const char * funcname,
            const int    lineno,
            const char * fn,
            char *       format,
            ...)
{
//  if ( is_reporting_ddc(traceGroup, fn) ) {   // wrong
    if (show_recoverable_errors) {
      char buffer[200];
      va_list(args);
      va_start(args, format);
      vsnprintf(buffer, 200, format, args);
      printf("(%s) %s\n", funcname, buffer);
   }
}


/* Tells whether DDC data errors are reported
 *
 * Arguments:   none
 * Returns:     nothing
 */
void show_ddcmsg() {
   // printf("Reporting DDC data errors: %s\n", bool_repr(show_recoverable_errors));
   print_simple_title_value(SHOW_REPORTING_TITLE_START,
                              "Reporting DDC data errors: ",
                              SHOW_REPORTING_MIN_TITLE_SIZE,
                              bool_repr(show_recoverable_errors));
}


/* Reports output levels for:
 *   - general output level (terse, verbose, etc)
 *   - DDC data errors
 *   - trace groups
 *
 * Arguments:    none
 * Returns:      nothing
 */
void show_reporting() {
   show_output_level();
   show_ddcmsg();
   show_trace_groups();
   puts("");
}


//
// Issue messages of various types
//

// n. used within macro LOADFUNC of adl_intf.c


// NB CANNOT MAP TO dbgtrc - writes to stderr, not stdout


void severemsg(
        const char * funcname,
        const int    lineno,
        const char * fn,
        char *       format,
        ...)
{
      char buffer[200];
      char buf2[250];
      va_list(args);
      va_start(args, format);
      vsnprintf(buffer, 200, format, args);
      snprintf(buf2, 250, "(%s) %s\n", funcname, buffer);
      fputs(buf2, stderr);
      va_end(args);
}



#ifdef OLD
// normally wrapped in one of the DBSMGS macros
void dbgmsg(
        const char * funcname,
        const int    lineno,
        const char * fn,
        char *       format,
        ...)
{
   // char buffer[200];
   // char buf2[250];
   static char * buffer = NULL;
   static int    bufsz  = 200;     // initial value
   static char * buf2   = NULL;

   if (!buffer) {      // first call
      buffer = calloc(bufsz,    sizeof(char*));
      buf2   = calloc(bufsz+50, sizeof(char*));
   }
   va_list(args);
   va_start(args, format);
   int ct = vsnprintf(buffer, bufsz, format, args);
   if (ct >= bufsz) {
      // printf("(dbgmsg) Reallocting buffer, new size = %d\n", ct+1);
      // buffer too small, reallocate and try again
      free(buffer);
      free(buf2);
      bufsz = ct+1;
      buffer = calloc(bufsz, sizeof(char*));
      buf2   = calloc(bufsz+50, sizeof(char*));
      va_list(args);
      va_start(args, format);
      ct = vsnprintf(buffer, bufsz, format, args);
      assert(ct < bufsz);
   }
   snprintf(buf2, bufsz+50, "(%s) %s", funcname, buffer);
   puts(buf2);
   va_end(args);
}
#endif


/* Core function for emitting debug or trace messages.
 * Normally wrapped in a DBGMSG or TRCMSG macro to
 * simplify calling.
 *
 * Arguments:
 *    trace_group   trace group of caller, to determine whether to output msg
 *                  0xff to always output
 *    funcname      function name in message
 *    lineno        line number in message
 *    fn            file name
 *    format        format string for message
 *    ...           format arguments
 *
 * Returns:         nothing
 */
void dbgtrc(
        Byte         trace_group,
        const char * funcname,
        const int    lineno,
        const char * fn,
        char *       format,
        ...)
{
   static char * buffer = NULL;
   static int    bufsz  = 200;     // initial value
   static char * buf2   = NULL;

   if (!buffer) {      // first call
      buffer = calloc(bufsz,    sizeof(char*));
      buf2   = calloc(bufsz+50, sizeof(char*));
   }

   if ( is_tracing(trace_group, fn) ) {
      va_list(args);
      va_start(args, format);
      int ct = vsnprintf(buffer, bufsz, format, args);
      if (ct >= bufsz) {   // if buffer too small, reallocate
         // printf("(dbgmsg) Reallocting buffer, new size = %d\n", ct+1);
         // buffer too small, reallocate and try again
         free(buffer);
         free(buf2);
         bufsz = ct+1;
         buffer = calloc(bufsz, sizeof(char*));
         buf2   = calloc(bufsz+50, sizeof(char*));
         va_list(args);
         va_start(args, format);
         ct = vsnprintf(buffer, bufsz, format, args);
         assert(ct < bufsz);
      }

      snprintf(buf2, bufsz+50, "(%s) %s\n", funcname, buffer);
      // puts(buf2);        // automatic terminating null
      fputs(buf2, FOUT);    // no automatic terminating null
      va_end(args);
   }
}


//
// Standardized handling of exceptional conditions, including
// error messages and possible program termination.
//

void report_ioctl_error(
      int   errnum,
      const char* funcname,   // const to avoid warning msg on references at compile time
      int   lineno,
      char* filename,
      bool fatal) {
   int errsv = errno;
   // fprintf(stderr, "(report_ioctl_error)\n");
   fprintf(stderr, "ioctl error in function %s at line %d in file %s: errno=%s\n",
           funcname, lineno, filename, linux_errno_desc(errnum) );
   // fprintf(stderr, "  %s\n", strerror(errnum));  // linux_errno_desc now calls strerror
   // will contain at least sterror(errnum), possibly more:
   // not worth the linkage issues:
   // fprintf(stderr, "  %s\n", explain_errno_ioctl(errnum, filedes, request, data));
   if (fatal)
      exit(EXIT_FAILURE);
   errno = errsv;
}


#ifdef UNUSED
// variant that can use libexplain, unused
void report_ioctl_error2(
      int   errnum,
      int   fh,
      int   request,
      void* data,
      const char* funcname,   // const to avoid warning msg on references at compile time
      int   lineno,
      char* filename,
      bool fatal)
{
   int errsv = errno;
   // fprintf(stderr, "(report_ioctl_error2)\n");
   report_ioctl_error(errno, funcname, lineno, filename, false /* non-fatal */ );
#ifdef USE_LIBEXPLAIN
   // fprintf(stderr, "(report_ioctl_error2) within USE_LIBEXPLAIN\n");
   fprintf(stderr, "%s\n", explain_ioctl(fh, request, data));
#endif
   if (fatal)
      exit(EXIT_FAILURE);
   errno = errsv;
}
#endif



/* Called when a condition that should be impossible has been detected.
 * Issues messages to stderr and terminates execution.
 *
 * This function is normally invoked using macro PROGRAM_LOGIC_ERROR
 * defined in util.h.
 *
 * Arguments:
 *    funcname    function name
 *    lineno      line number in source file
 *    fn          source file name
 *    format      format string, as in printf()
 *    ...         or or more substitution values for the format string
 *
 * Returns:
 *    nothing (terminates execution)
 */
void program_logic_error(
      const char * funcname,
      const int    lineno,
      const char * fn,
      char *       format,
      ...)
{
  // assemble the error message
  char buffer[200];
  va_list(args);
  va_start(args, format);
  vsnprintf(buffer, 200, format, args);

  // assemble the location message:
  char buf2[250];
  snprintf(buf2, 250, "Program logic error in function %s at line %d in file %s:\n",
                      funcname, lineno, fn);

  // don't combine into 1 line, might be very long.  just output 2 lines:
  fputs(buf2,   stderr);
  fputs(buffer, stderr);
  fputc('\n',   stderr);

  fputs("Terminating execution.\n", stderr);
  exit(EXIT_FAILURE);
}



// normally wrapped in macro TERMINATE_EXECUTION_ON_ERROR
void terminate_execution_on_error(
        Trace_Group   trace_group,
        const char * funcname,
        const int    lineno,
        const char * fn,
        char *       format,
        ...)
{
   char buffer[200];
   char buf2[250];
   char * finalBuffer = buffer;
   va_list(args);
   va_start(args, format);
   vsnprintf(buffer, 200, format, args);

   if ( is_tracing(trace_group, fn) ) {
      snprintf(buf2, 250, "(%s) %s", funcname, buffer);
      finalBuffer = buf2;
   }

   puts(finalBuffer);
   puts("Terminating execution.");
   exit(EXIT_FAILURE);
}

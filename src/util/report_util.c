/* report_util.c
 *
 * Functions for creating messages in a stardardized format, with flexible
 * indentation.  It is particularly used for tracing data structures.
 *
 * This source file maintains state in static variables so is not thread safe.
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

/** @file report_util.c
 * Report utility functions
 *
 * TODO: describe
 * - indentation depth
 *     - indentation stack
 * - destination stack
 */

/** \cond */
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
/** \endcond */

#include "coredefs.h"
#include "file_util.h"
#include "string_util.h"

#include "report_util.h"


#define DEFAULT_INDENT_SPACES_PER_DEPTH 3
#define INDENT_SPACES_STACK_SIZE 16

static int indent_spaces_stack[INDENT_SPACES_STACK_SIZE];
static int indent_spaces_stack_pos = -1;

#define OUTPUT_DEST_STACK_SIZE 8

static FILE * output_dest_stack[OUTPUT_DEST_STACK_SIZE];
static int    output_dest_stack_pos = -1;

// work around for fact that can't initialize the initial stack entry to stdout
static FILE * alt_initial_output_dest     = NULL;
static bool   initial_output_dest_changed = false;


//
// Indentation
//

// Functions that allow for temporarily changing the number of
// indentation spaces per logical indentation depth.
// 10/16/15: not currently used

void rpt_push_indent(int new_spaces_per_depth)
{
  assert(indent_spaces_stack_pos < INDENT_SPACES_STACK_SIZE - 1);
  indent_spaces_stack[++indent_spaces_stack_pos] = new_spaces_per_depth;
}


void rpt_pop_indent()
{
  if(indent_spaces_stack_pos >= 0)
    indent_spaces_stack_pos--;
}


void rpt_reset_indent_stack() { indent_spaces_stack_pos = -1; }


/** Given a logical indentation depth, returns the number of spaces
 * of indentation to be used.
 *
 * @param depth logical indentation depth
 * @return number of indentation spaces
 */
int rpt_get_indent(int depth)
{
  int spaces_ct = DEFAULT_INDENT_SPACES_PER_DEPTH;
  if(indent_spaces_stack_pos >= 0)
    spaces_ct = indent_spaces_stack[indent_spaces_stack_pos];
  return depth * spaces_ct;
}


// Functions that allow for temporarily changing the output destination.

/** Sets the output destination to be used for report functions.
 *  The current output destination is saved on the output destination stack.
 *
 *  @param new_dest  new output destination
 */
void rpt_push_output_dest(FILE * new_dest)
{
  assert(output_dest_stack_pos < OUTPUT_DEST_STACK_SIZE - 1);
  output_dest_stack[++output_dest_stack_pos] = new_dest;
}


/** Pops the output destination stack, and sets the output destination
 *  to be used for report functions to the new top of the stack.
 */
void rpt_pop_output_dest()
{
  if(output_dest_stack_pos >= 0)
    output_dest_stack_pos--;
}


/** Clears the output destination stack.
 * The output destination to be used for report functions to the default (stdout).
 */
void rpt_reset_output_dest_stack() { output_dest_stack_pos = -1; }


/** Gets the current output destination.
 *
 * @return current output destination
 */
FILE * rpt_cur_output_dest()
{
  // special handling for unpushed case because can't statically initialize
  // output_dest_stack[0] to stdout
  FILE * result = NULL;
  if(output_dest_stack_pos < 0)
    result = (initial_output_dest_changed) ? alt_initial_output_dest : stdout;
  else
    result = output_dest_stack[output_dest_stack_pos];
  return result;
}


/** Debugging function to show output destination.
 */
void rpt_debug_output_dest()
{
  FILE * dest = rpt_cur_output_dest();
  char * addl = (dest == stdout) ? " (stdout)" : "";
  printf("(%s) output_dest_stack[%d] = %p %s\n", __func__, output_dest_stack_pos, dest, addl);
}

/** Changes the current output destination, without saving
 * the current output destination on the destination stack.
 *
 * @param new_dest new output destination
 *
 * @remark Needed for set_fout() in core.c
 */
void rpt_change_output_dest(FILE * new_dest)
{
  if(output_dest_stack_pos >= 0)
    output_dest_stack[output_dest_stack_pos] = new_dest;
  else
  {
    initial_output_dest_changed = true;
    alt_initial_output_dest     = new_dest;
  }
}


// should not be needed, for diagnosing a problem
void rpt_flush() { fflush(rpt_cur_output_dest()); }


/** Writes a newline to the current output destination.
 */
void rpt_nl() { f0printf(rpt_cur_output_dest(), "\n"); }


/* Writes a constant string to the current output destination.
 *
 * A newline is appended to the string specified.
 *
 * The output is indented per the specified indentation depth.
 *
 * @param title string to write
 * @param depth logical indentation depth.
 *
 * @remark This is the core function through which all output is funneled.
 */
void rpt_title(char * title, int depth)
{
  bool debug = false;
  if(debug)
    printf("(%s) Writing to %p\n", __func__, rpt_cur_output_dest());
  f0printf(rpt_cur_output_dest(), "%*s%s\n", rpt_get_indent(depth), "", title);
}


/* Writes a constant string to the current output destination.
 *
 * A newline is appended to the string specified.
 *
 * The output is indented per the specified indentation depth.
 *
 * @param depth logical indentation depth.
 * @param title string to write
 *
 * @remark
 * This function is logically equivalent to #rpt_title(), except that
 * the **depth** parameter is first, not last.
 * Experience wih the API has shown that #rpt_title() tends not to be
 * used along with #rpt_vstring() because the different position of the
 * **depth** parameter makes the code harder to read.
 */
void rpt_label(int depth, char * text) { rpt_title(text, depth); }

/** Writes a formatted string to the current output destination.
 *
 * A newline is appended to the string specified
 *
 * @param  depth    logical indentation depth
 * @param  format   format string (normal printf)
 * @param  ...      arguments
 *
 * @remark Note that the depth parm is first on this function because of variable args
 */
void rpt_vstring(int depth, char * format, ...)
{
  int    buffer_size = 200;
  char * buffer      = malloc(buffer_size);
  char * buf         = buffer;
  va_list(args);
  va_start(args, format);
  int reqd_size = vsnprintf(buffer, buffer_size, format, args);
  // if buffer wasn't sufficiently large, allocate a temporary buffer
  if(reqd_size >= buffer_size)
  {
    // printf("(%s) Allocating temp buffer, reqd_size=%d\n", __func__, reqd_size);
    buf = malloc(reqd_size + 1);
    va_start(args, format);
    vsnprintf(buf, reqd_size + 1, format, args);
  }
  va_end(args);

  rpt_title(buf, depth);

  free(buf);
}


/** Convenience function that writes multiple constant strings.
 *
 *   @param depth    logical indentation depth
 *   @param ...      pointers to constant strings,
 *                   last pointer is NULL to terminate list
 */
void rpt_multiline(int depth, ...)
{
  va_list args;
  va_start(args, depth);
  char * s = NULL;
  while((s = va_arg(args, char *)) != NULL)
  {
    rpt_title(s, depth);
  }
  va_end(args);
}


/** Writes all strings in a GPtrArray to the current report destination
 *
 * @param  depth   logical indentation depth
 * @param  strings pointer to GPtrArray of strings
 */
void rpt_g_ptr_array(int depth, GPtrArray * strings)
{
  for(int ndx = 0; ndx < strings->len; ndx++)
  {
    char * s = g_ptr_array_index(strings, ndx);
    rpt_title(s, depth);
  }
}


/** Writes a hex dump with indentation.
 *  Output is written to the current report destination
 *
 * @param data  start of bytes to dump
 * @param size  number of bytes to dump
 * @param depth logical indentation depth
 */
void rpt_hex_dump(const Byte * data, int size, int depth)
{
  fhex_dump_indented(rpt_cur_output_dest(), data, size, rpt_get_indent(depth));
}


/** Writes a string to the current output destination, describing a pointer
 * to a named data structure.
 *
 * The output is indented per the specified indentation depth.
 *
 * @param name  struct name
 * @param ptr   pointer to struc
 * @param depth logical indentation depth
 */
void rpt_structure_loc(const char * name, const void * ptr, int depth)
{
  // fprintf(rpt_cur_output_dest(), "%*s%s at: %p\n", rpt_indent(depth), "", name, ptr);
  rpt_vstring(depth, "%s at: %p", name, ptr);
}

/** Writes a pair of strings to the current output destination.
 *
 * If offset_absolute is true, then the s2 value will start in the same column,
 * irrespective of the line indentation.   This may make some reports easier to read.
 *
 * @param s1 first string
 * @param s2 second string
 * @param col2offset  offset from start of line where s2 starts
 * @param offset_absolute  if true,  col2offset is relative to the start of the line, before indentation
 *                         if false, col2offset is relative to the indented start of s1
 * @param depth logical indentation depth
 */
void rpt_2col(char * s1, char * s2, int col2offset, bool offset_absolute, int depth)
{
  int col1sz   = col2offset;
  int indentct = rpt_get_indent(depth);
  if(offset_absolute)
    col1sz = col1sz - indentct;
  rpt_vstring(depth, "%-*s%s", col1sz, s1, s2);
}

/* Reports the contents of a file.
 *
 * @param   fn        name of file
 * @param   verbose   if true, emit message if error reading file
 * @depth   depth     logical indentation depth
 */
int rpt_file_contents(const char * fn, bool verbose, int depth)
{
  GPtrArray * line_array = g_ptr_array_new();
  int         rc         = file_getlines(fn, line_array, false);
  if(rc < 0)
  {
    if(verbose)
      rpt_vstring(depth, "Error reading file %s: %s", fn, strerror(-rc));
  }
  else if(rc == 0)
  {
    if(verbose)
      rpt_vstring(depth, "Empty file: %s", fn);
  }
  else if(rc > 0)
  {
    int ndx = 0;
    for(; ndx < line_array->len; ndx++)
    {
      char * curline = g_ptr_array_index(line_array, ndx);
      // trim_in_place(curline);     // strip trailing newline - now done in file_getlines()
      rpt_title(curline, depth);
    }
  }
  return rc;
}


/* The remaining rpt_ functions various data types share a common formatting so that they can
 * be use together.  All channel their output through rpt_str().
 *
 * Depending on whether the info parm is null, output takes one of the following forms:
 *    name       (info) : value
 *    name              : value
 */

/** Writes a string to the current output destination describing a named character string value.
 *
 * The output is indented per the specified indentation depth.
 *
 * Optionally, a description string can be specified along with the name.
 * The description string will be surrounded by parentheses.
 *
 * The string value is prefixed with a colon.
 *
 * @param name  name of value
 * @param info  if non-null, description of value
 * @param val   string value
 * @param depth logical indentation depth
 */
void rpt_str(const char * name, char * info, const char * val, int depth)
{
  bool debug = false;
  if(debug)
    printf("(%s) Writing to %p\n", __func__, rpt_cur_output_dest());

  char infobuf[100];
  if(info)
    snprintf(infobuf, 99, "(%s)", info);
  else
    infobuf[0] = '\0';
  rpt_vstring(depth, "%-25s %30s : %s", name, infobuf, val);
}


/** Writes a string to the current output destination describing a boolean value.
 *
 * The value is displayed as "true" or "false".
 *
 * The output is indented per the specified indentation depth.
 * The description string will be surrounded by parentheses.
 *
 * The value is prefixed with a colon.
 *
 * @param name  name of value
 * @param info  if non-null, description of value
 * @param val   value to show
 * @param depth logical indentation depth
 *
 * The value is formatted as "true" or "false".
 */
void rpt_bool(char * name, char * info, bool val, int depth)
{
  char * valName = (val) ? "true" : "false";
  rpt_str(name, info, valName, depth);
}


/** Writes a string to the current output destination, describing a named integer value.
 *
 * The output is indented per the specified indentation depth.
 *
 * Optionally, a description string can be specified along with the name.
 * The description string will be surrounded by parentheses.
 *
 * The value is prefixed with a colon.
 *
 * @param name  name of value
 * @param info  if non-null, description of value
 * @param val   integer value
 * @param depth logical indentation depth
 */
void rpt_int(char * name, char * info, int val, int depth)
{
  char buf[10];
  snprintf(buf, 9, "%d", val);
  rpt_str(name, info, buf, depth);
}


/** Writes a string to the current output destination describing a 4 byte integer value,
 * indented per the specified indentation depth.
 *
 * The integer value is formatted as printable hex.
 *
 * Optionally, a description string can be specified along with the name.
 * The description string will be surrounded by parentheses.
 *
 * The value is prefixed with a colon.
 *
 * @param name  name of value
 * @param info  if non-null, description of value
 * @param val   integer value
 * @param depth logical indentation depth
 */
void rpt_int_as_hex(char * name, char * info, int val, int depth)
{
  char buf[16];
  snprintf(buf, 15, "0x%08x", val);
  rpt_str(name, info, buf, depth);
}


/** Writes a string to the current output destination describing a single byte value,
 * indented per the specified indentation depth.
 *
 * The value is formatted as printable hex.
 *
 * Optionally, a description string can be specified along with the name.
 * The description string will be surrounded by parentheses.
 *
 * The value is prefixed with a colon.
 *
 * @param name  name of value
 * @param info  if non-null, description of value
 * @param val   value
 * @param depth logical indentation depth
 */
void rpt_uint8_as_hex(char * name, char * info, unsigned char val, int depth)
{
  char buf[16];
  snprintf(buf, 15, "0x%02x", val);
  rpt_str(name, info, buf, depth);
}


/** Writes a string to the current output destination describing a named integer
 * value having a symbolic string representation.
 *
 * The output is indented per the specified indentation depth.
 *
 * The integer value is converted to a string using the specified function.
 *
 * Optionally, a description string can be specified along with the name.
 * The description string will be surrounded by parentheses.
 *
 * The value is prefixed with a colon.
 *
 * @param name  name of value
 * @param info  if non-null, description of value
 * @param val   integer value
 * @param func  interpretation function
 * @param depth logical indentation depth
 */
void rpt_mapped_int(char * name, char * info, int val, Value_To_Name_Function func, int depth)
{
  char * valueName = func(val);
  char   buf[100];
  snprintf(buf, 100, "%d - %s", val, valueName);
  rpt_str(name, info, buf, depth);
}


/** Writes a string to the current output destination describing a sequence of bytes,
 * indented per the specified indentation depth.
 *
 * The value is formatted as printable hex.
 *
 * Optionally, a description string can be specified along with the name.
 * The description string will be surrounded by parentheses.
 *
 * The value is prefixed with a colon.
 *
 * @param name  name of value
 * @param info  if non-null, description of value
 * @param bytes pointer to start of bytes to show
 * @param ct    number of bytes to show
 * @param hex_prefix_flag if true, the printable hex value will begin with "0x"
 * @param depth logical indentation depth
 */
void rpt_bytes_as_hex(const char * name, char * info, Byte * bytes, int ct, bool hex_prefix_flag, int depth)
{
  // printf("(%s) bytes=%p, ct=%d\n", __func__, bytes, ct);
  int bufsz = 2 * ct + 1;
  bufsz++; // hack
  if(hex_prefix_flag)
    bufsz += 2;
  char * buf        = malloc(bufsz);
  char * hex_prefix = (hex_prefix_flag) ? "0x" : "";
  char * hs         = hexstring(bytes, ct);

  snprintf(buf, bufsz - 1, "%s%s", hex_prefix, hs);
  rpt_str(name, info, buf, depth);
  free(buf);
  free(hs);
}


//  Functions for reporting integers that are collections of named bits

#ifdef DEBUG
static void report_flag_info(Flag_Info * pflag_info, int depth)
{
  assert(pflag_info);
  rpt_structure_loc("FlagInfo", pflag_info, depth);
  int d1 = depth + 1;
  rpt_str("flag_name", NULL, pflag_info->flag_name, d1);
  rpt_str("flag_info", NULL, pflag_info->flag_info, d1);
  rpt_int_as_hex("flag_val", NULL, pflag_info->flag_val, d1);
}


/* Function for debugging findFlagInfoDictionary.
 *
 * Reports the contents of a FlagDictionay record.
 */
static void report_flag_info_dictionary(Flag_Dictionary * pDict, int depth)
{
  assert(pDict);
  rpt_structure_loc("Flag_Dictionary", pDict, depth);
  int d1 = depth + 1;
  rpt_int("flag_info_ct", NULL, pDict->flag_info_ct, d1);
  int ndx = 0;
  for(; ndx < pDict->flag_info_ct; ndx++)
  {
    report_flag_info(&pDict->flag_info_recs[ndx], d1);
  }
}
#endif


static Flag_Info * find_flag_info_in_dictionary(char * flag_name, Flag_Dictionary * pdict)
{
  Flag_Info * result = NULL;
  // printf("(%s) Starting.  flag_name=%s, pdict=%p   \n", __func__, flagName, pdict );
  // printf("(%s) pdict->flag_info_ct=%d  \n", __func__, pdict->flag_info_ct );
  // report_flag_info_dictionary(pdict, 2);
  int ndx;
  for(ndx = 0; ndx < pdict->flag_info_ct; ndx++)
  {
    // printf("(%s) ndx=%d  \n", __func__, ndx );
    // Flag_Info pcur_info = &(pdict->flag_info_recs[ndx]);
    // printf("(%s) pdict->flag_info_recs[ndx].flag_name=%s    \n", __func__, pdict->flag_info_recs[ndx].flag_name );
    if(streq(flag_name, pdict->flag_info_recs[ndx].flag_name))
    {
      // printf("(%s) Match  \n", __func__ );
      result = &pdict->flag_info_recs[ndx];
      break;
    }
  }
  // printf("(%s) Returning: %p  \n", __func__, result );
  return result;
}


static void char_buf_append(char * buffer, int bufsize, char * val_to_append)
{
  assert(strlen(buffer) + strlen(val_to_append) < bufsize);
  strcat(buffer, val_to_append);
}


static void flag_val_to_string_using_dictionary(
  int flags_val, Flag_Name_Set * pflag_name_set, Flag_Dictionary * pdict, char * buffer, int bufsize)
{
  // printf("(%s) flagsVal=0x%02x, pflagNameSet=%p, pDict=%p \n", __func__, flagsVal, pflagNameSet, pDict );
  // printf("(%s) pflagNameSet->flagNameCt=%d  \n", __func__, pflagNameSet->flagNameCt );
  // printf("(%s) pDict->flagInfoCt=%d  \n", __func__, pDict->flagInfoCt );
  assert(buffer && bufsize > 1);
  buffer[0] = '\0';
  int  ndx;
  bool first = true;
  for(ndx = 0; ndx < pflag_name_set->flag_name_ct; ndx++)
  {
    Flag_Info * pflag_info = find_flag_info_in_dictionary(pflag_name_set->flag_names[ndx], pdict);
    // printf("(%s) ndx=%d, pFlagInfp=%p   \n", __func__, ndx, pFlagInfo );
    if(flags_val & pflag_info->flag_val)
    {
      if(first)
        first = false;
      else
        char_buf_append(buffer, bufsize, ", ");
      char_buf_append(buffer, bufsize, pflag_info->flag_name);
    }
  }
  // printf("(%s) Returning |%s|\n", __func__, buffer );
}


/** Writes a string to the current output destination describing an integer
 * that is to be interpreted as a named collection of named bits.
 *
 * Output is indented per the specified indentation depth.
 * The description string will be surrounded by parentheses.
 *
 * The value is prefixed with a colon.
 *
 * @param name  name of value
 * @param info  if non-null, description of value
 * @param val   value to interpret
 * @param p_flag_name_set
 * @param p_dict
 * @param depth logical indentation depth
 */
void rpt_ifval2(char * name, char * info, int val, Flag_Name_Set * p_flag_name_set, Flag_Dictionary * p_dict, int depth)
{
  char buf[1000];
  buf[0] = '\0';
  snprintf(buf, 7, "0x%04x", val);
  char_buf_append(buf, sizeof(buf), " - ");
  flag_val_to_string_using_dictionary(val, p_flag_name_set, p_dict, buf, sizeof(buf));
  rpt_str(name, info, buf, depth);
}

/* linux_errno.c
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

/** \file
 * Linux errno descriptions
 */

/** \cond */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/** \endcond */

#include "util/string_util.h"

#include "base/linux_errno.h"

char * linux_errno_name(int error_number) { return ""; }
char * linux_errno_desc(int error_number) { return ""; }

// To consider:  use libexplain.

// Forward declarations
// static Status_Code_Info * get_negative_errno_info(int errnum);
Status_Code_Info * find_errno_description(int errnum);
void               show_errno_desc_table();

/**  Initialize linux_errno.c
 */
// n. called from main before command line parsed, trace control not yet established
void init_linux_errno()
{
#ifdef OLD
  register_retcode_desc_finder(RR_ERRNO, get_negative_errno_info,
                               false); // finder_arg_is_modulated
#endif
  // show_errno_desc_table();
}


//
// Error handling
//

//
// Known system error numbers
//

// Because macro EDENTRY ignores the description value supplied and sets
// the description field to NULL, find_eerno_description(), called by
// linux_errno_desc(), will lookup the description using strerror().
#define EDENTRY(id, desc)                                                                                              \
  {                                                                                                                    \
    id, #id, NULL                                                                                                      \
  }

static Status_Code_Info errno_desc[] = {
  EDENTRY(0, "success"),
  EDENTRY(EPERM, "Operation not permitted"),
  EDENTRY(ENOENT, "No such file or directory"),
  EDENTRY(ESRCH, "No such process"),           //  3
  EDENTRY(EINTR, "Interrupted system call"),   //  4
  EDENTRY(EIO, "I/O error"),                   //  5
  EDENTRY(ENXIO, "No such device or address"), // 6
  EDENTRY(E2BIG, "Argument list too long"),
  EDENTRY(ENOEXEC, "Exec format error"),
  EDENTRY(EBADF, "Bad file number"), //  9
  EDENTRY(ECHILD, "No child processes"),
  EDENTRY(EAGAIN, "Try again"),
  EDENTRY(ENOMEM, "Out of memory"),
  EDENTRY(EACCES, "Permission denied"), // 13
  EDENTRY(EFAULT, "Bad address"),       // 14
  EDENTRY(ENOTBLK, "Block device required"),
  EDENTRY(EBUSY, "Device or resource busy"),
  EDENTRY(EEXIST, "File exists"),
  EDENTRY(EXDEV, "Cross-device link"),
  EDENTRY(ENODEV, "No such device"),
  EDENTRY(ENOTDIR, "Not a directory"),
  EDENTRY(EISDIR, "Is a directory"),
  EDENTRY(EINVAL, "Invalid argument"), // 22
  EDENTRY(ENFILE, "File table overflow"),
  EDENTRY(EMFILE, "Too many open files"),
  EDENTRY(ENOTTY, "Not a typewriter"),
  EDENTRY(ETXTBSY, "Text file busy"),
  EDENTRY(EFBIG, "File too large"),
  EDENTRY(ENOSPC, "No space left on device"),
  EDENTRY(ESPIPE, "Illegal seek"),
  EDENTRY(EROFS, "Read-only file system"),
  EDENTRY(EMLINK, "Too many links"),                    // 31
  EDENTRY(EPIPE, "Broken pipe"),                        // 32
  EDENTRY(EDOM, "Math argument out of domain of func"), // 33
  EDENTRY(ERANGE, "Math result not representable"),     // 34
  // break in seq
  EDENTRY(EPROTO, "Protocol error"), // 71

  EDENTRY(ENOTSOCK, "Socket operation on non-socket"),                  //  88
  EDENTRY(EDESTADDRREQ, "Destination address required"),                //  89
  EDENTRY(EMSGSIZE, "Message too long"),                                //  90
  EDENTRY(EPROTOTYPE, "Protocol wrong type for socket"),                //  91
  EDENTRY(ENOPROTOOPT, "Protocol not available"),                       //  92
  EDENTRY(EPROTONOSUPPORT, "Protocol not supported"),                   //  93
  EDENTRY(ESOCKTNOSUPPORT, "Socket type not supported"),                //  94
  EDENTRY(EOPNOTSUPP, "Operation not supported on transport endpoint"), //  95
  EDENTRY(EPFNOSUPPORT, "Protocol family not supported"),               //  96
  EDENTRY(EAFNOSUPPORT, "Address family not supported by protocol"),    //  97
  EDENTRY(EADDRINUSE, "Address already in use"),                        //  98
  EDENTRY(EADDRNOTAVAIL, "Cannot assign requested address"),            //  99

  EDENTRY(ENETDOWN, "Network is down"),                                // 100
  EDENTRY(ENETUNREACH, "Network is unreachable"),                      // 101
  EDENTRY(ENETRESET, "Network dropped connection because of reset"),   // 102
  EDENTRY(ECONNABORTED, "Software caused connection abort"),           // 103
  EDENTRY(ECONNRESET, "Connection reset by peer"),                     // 104
  EDENTRY(ENOBUFS, "No buffer space available"),                       // 105
  EDENTRY(EISCONN, "Transport endpoint is already connected"),         // 106
  EDENTRY(ENOTCONN, "Transport endpoint is not connected"),            // 107
  EDENTRY(ESHUTDOWN, "Cannot send after transport endpoint shutdown"), // 108
  EDENTRY(ETOOMANYREFS, "Too many references: cannot splice"),         // 109
  EDENTRY(ETIMEDOUT, "Connection timed out"),                          // 110
  EDENTRY(ECONNREFUSED, "Connection refused"),                         // 111
  EDENTRY(EHOSTDOWN, "Host is down"),                                  // 112
  EDENTRY(EHOSTUNREACH, "No route to host"),                           // 113
  EDENTRY(EALREADY, "Operation already in progress"),                  // 114
  EDENTRY(EINPROGRESS, "Operation now in progress"),                   // 115
  EDENTRY(ESTALE, "Stale file handle"),                                // 116
  EDENTRY(EUCLEAN, "Structure needs cleaning"),                        // 117

  EDENTRY(ENOTNAM, "Not a XENIX named type file"),   // 118
  EDENTRY(ENAVAIL, "No XENIX semaphores available"), // 119
  EDENTRY(EISNAM, "Is a named type file"),           // 120
  EDENTRY(EREMOTEIO, "Remote I/O error"),            // 121
  EDENTRY(EDQUOT, "Quota exceeded"),                 // 122
};
#undef EDENTRY
static const int errno_desc_ct = sizeof(errno_desc) / sizeof(Status_Code_Info);

#define WORKBUF_SIZE 300
static char             workbuf[WORKBUF_SIZE];
static char             dummy_errno_description[WORKBUF_SIZE];
static Status_Code_Info dummy_errno_desc;

/** Debugging function that displays the errno description table.
 */
void show_errno_desc_table()
{
  printf("(%s) errno_desc table:\n", __func__);
  for(int ndx = 0; ndx < errno_desc_ct; ndx++)
  {
    Status_Code_Info cur = errno_desc[ndx];
    printf("(%3d, %-20s, %s\n", cur.code, cur.name, cur.description);
  }
}


/* Simple call to get a description string for a Linux errno value.
 *
 * For use in specifically reporting an unmodulated Linux error number.
 *
 * Arguments:
 *    error_number  system errno value
 *
 * Returns:
 *    string describing the error.
 *
 * The string returned is valid until the next call to this function.
 */

char * linux_errno_desc(int error_number)
{
  bool debug = false;
  if(debug)
    printf("(%s) error_number = %d\n", __func__, error_number);
  assert(error_number >= 0);
  Status_Code_Info * pdesc = find_errno_description(error_number);
  if(pdesc)
  {
    snprintf(workbuf, WORKBUF_SIZE, "%s(%d): %s", pdesc->name, error_number, pdesc->description);
  }
  else
  {
    snprintf(workbuf, WORKBUF_SIZE, "%d: %s", error_number, strerror(error_number));
  }
  if(debug)
    printf("(%s) error_number=%d, returning: |%s|\n", __func__, error_number, workbuf);
  return workbuf;
}


char * linux_errno_name(int error_number)
{
  Status_Code_Info * pdesc = find_errno_description(error_number);
  return pdesc->name;
}


/* Returns the Status_Code_Info record for the specified error number
 *
 * @param  errnum    linux error number, in positive, unmodulated form
 *
 * @return Status_Code_Description record, NULL if not found
 *
 * @remark
 * If the description field of the found Status_Code_Info struct is NULL, it is set
 * by calling strerror()
 */
Status_Code_Info * find_errno_description(int errnum)
{
  bool debug = false;
  if(debug)
    printf("(%s) errnum=%d\n", __func__, errnum);
  Status_Code_Info * result = NULL;
  int                ndx;
  for(ndx = 0; ndx < errno_desc_ct; ndx++)
  {
    if(errnum == errno_desc[ndx].code)
    {
      result = &errno_desc[ndx];
      break;
    }
  }
  if(result && !result->description)
  {
    // char * desc = strerror(errnum);
    // result->description  = malloc(strlen(desc)+1);
    // strcpy(result->description, desc);
    result->description = strdup(strerror(errnum));
  }
  if(debug)
    printf("(%s) Returning %p\n", __func__, result);
  return result;
}


// n. returned value is valid only until next call
Status_Code_Info * create_dynamic_errno_info(int errnum)
{
  Status_Code_Info * result = &dummy_errno_desc;
  result->code              = errnum;
  result->name              = NULL;

  // would be simpler to use strerror_r(), but the return value of
  // that function depends on compiler switches.
  char * s  = strerror(errnum); // generates an unknown code message for unrecognized errnum
  int    sz = sizeof(dummy_errno_description);
  strncpy(dummy_errno_description, s, sz);
  dummy_errno_description[sz - 1] = '\0'; // ensure trailing null in case strncpy truncated
  result->description             = dummy_errno_description;

  return result;
}


Status_Code_Info * get_errno_info(int errnum)
{
  Status_Code_Info * result = find_errno_description(errnum);
  return result;
}


// returns NULL if not found
Status_Code_Info * get_negative_errno_info(int errnum)
{
  bool debug = false;
  if(debug)
    printf("(%s) errnum=%d\n", __func__, errnum);
  return get_errno_info(-errnum);
}


/** Gets the Linux error number for a symbolic name.
 * The value is returned as a negative number.
 *
 * @param   errno_name    symbolic name, e.g. EBUSY
 * @param   perrno        where to return error number
 *
 * @return  true if found, false if not
 */
bool errno_name_to_number(const char * errno_name, int * perrno)
{
  int found = false;
  *perrno   = 0;
  for(int ndx = 0; ndx < errno_desc_ct; ndx++)
  {
    if(streq(errno_desc[ndx].name, errno_name))
    {
      *perrno = -errno_desc[ndx].code;
      found   = true;
      break;
    }
  }
  return found;
}

/** Gets the Linux error number for a symbolic name.
 * The value is returned as a negative, modulated number.
 *
 * @param   errno_name      symbolic name, e.g. EBUSY
 * @param   p_error_number  where to return error number
 *
 * @return  true if found, false if not
 */
#ifdef OLD
bool errno_name_to_modulated_number(const char * errno_name, Global_Status_Code * p_error_number)
{
  int  result = 0;
  bool found  = errno_name_to_number(errno_name, &result);
  assert(result >= 0);
  if(result != 0)
  {
    result = modulate_rc(result, RR_ERRNO);
    assert(result <= 0);
  }
  *p_error_number = result;
  return found;
}
#endif

#ifdef OLD
bool errno_name_to_modulated_number(const char * errno_name, Public_Status_Code * p_error_number)
{
  return errno_name_to_number(errno_name, p_error_number);
}
#endif

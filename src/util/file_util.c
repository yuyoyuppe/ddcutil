/* file_util.c
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

/** \cond */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
//#include <unistd.h>
/** \endcond */

#include "string_util.h"

#include "file_util.h"

#define PATH_MAX 256
size_t _getline(char ** lineptr, size_t * n, FILE * stream)
{
  char * bufptr = NULL;
  char * p      = bufptr;
  size_t size;
  int    c;

  if(lineptr == NULL)
  {
    return -1;
  }
  if(stream == NULL)
  {
    return -1;
  }
  if(n == NULL)
  {
    return -1;
  }
  bufptr = *lineptr;
  size   = *n;

  c = fgetc(stream);
  if(c == EOF)
  {
    return -1;
  }
  if(bufptr == NULL)
  {
    bufptr = malloc(128);
    if(bufptr == NULL)
    {
      return -1;
    }
    size = 128;
  }
  p = bufptr;
  while(c != EOF)
  {
    if((p - bufptr) > (size - 1))
    {
      size   = size + 128;
      bufptr = realloc(bufptr, size);
      if(bufptr == NULL)
      {
        return -1;
      }
    }
    *p++ = c;
    if(c == '\n')
    {
      break;
    }
    c = fgetc(stream);
  }

  *p++     = '\0';
  *lineptr = bufptr;
  *n       = size;

  return p - bufptr - 1;
}
/** @file file_util.c
 * File utility functions
 */

/** Reads the lines of a text file into a GPtrArray.
 *
 *  @param  fn          file name
 *  @param  line_array  pointer to GPtrArray of strings where lines will be saved
 *  @param  verbose     if true, write message to stderr if unable to open file or other error
 *
 *  @retval >=0:  number of lines added to line_array
 *  @retval <0    -errno
 *
 *  The caller is responsible for freeing the lines added to line_array.
 */
int file_getlines(const char * fn, GPtrArray * line_array, bool verbose)
{
  bool debug = false;
  if(debug)
    printf("(%s) Starting. fn=%s  \n", __func__, fn);

  int    rc = 0;
  FILE * fp = fopen(fn, "r");
  if(!fp)
  {
    int errsv = errno;
    rc        = -errno;
    if(verbose)
      fprintf(stderr, "Error opening file %s: %s\n", fn, strerror(errsv));
  }
  else
  {
    // if line == NULL && len == 0, then getline allocates buffer for line
    char * line = NULL;
    size_t len  = 0;
    size_t read;
    int    linectr = 0;
    errno          = 0;
    while((read = _getline(&line, &len, fp)) != -1)
    {
      linectr++;
      rtrim_in_place(line); // strip trailing newline
      g_ptr_array_add(line_array, line);
      // printf("(%s) Retrieved line of length %zu: %s\n", __func__, read, line);
      line = NULL; // reset for next getline() call
      len  = 0;
    }
    if(errno != 0)
    { // getline error?
      rc = -errno;
      if(verbose)
        fprintf(stderr, "Error reading file %s: %s\n", fn, strerror(-rc));
    }
    free(line);
    rc = linectr;

    fclose(fp);
  }

  if(debug)
    printf("(%s) Done. returning: %d\n", __func__, rc);
  return rc;
}


/** Reads the first line of a file.
 *
 *  @param  fn          file name
 *  @param  verbose     if true, write message to stderr if unable to open file
 *
 *  @retval non-NULL pointer to line read (caller responsible for freeing)
 *  @retval NULL     if error or no lines in file
 */
char * file_get_first_line(const char * fn, bool verbose)
{
  FILE * fp          = fopen(fn, "r");
  char * single_line = NULL;
  if(!fp)
  {
    if(verbose)
      fprintf(stderr, "Error opening %s: %s\n", fn, strerror(errno)); // TODO: strerror() not thread safe
  }
  else
  {
    size_t len = 0;
    size_t read;
    // just one line:
    read = _getline(&single_line, &len, fp);
    if(read == -1)
    {
      if(verbose)
        printf("Nothing to read from %s\n", fn);
    }
    else
    {
      if(strlen(single_line) > 0)
        single_line[strlen(single_line) - 1] = '\0';
      // printf("\n%s", single_line);     // single_line has trailing \n
    }
    fclose(fp);
  }
  // printf("(%s) fn=|%s|, returning: |%s|\n", __func__, fn, single_line);
  return single_line;
}


/** Reads a binary file, returning it as a **GByteArray**.
 *
 *  \param  fn        file name
 *  \param  est_size  estimated size
 *  \param  verbose   if open fails, write message to stderr
 *  \return if successful, a **GByteArray** of bytes, caller is responsible for freeing
 *          if failure, then NULL
 */
GByteArray * read_binary_file(char * fn, int est_size, bool verbose)
{
  assert(fn);

  bool debug = false;

  // DBGMSG("fn=%s", fn);

  Byte buf[1];

  GByteArray * gbarray = NULL;

  FILE * fp;
  if(!(fp = fopen(fn, "r")))
  {
    int errsv = errno;
    if(verbose)
    {
      fprintf(stderr, "Error opening \"%s\", %s\n", fn, strerror(errsv));
    }
    goto bye;
  }

  if(est_size <= 0)
    gbarray = g_byte_array_new();
  else
    gbarray = g_byte_array_sized_new(est_size);

  // TODO: read bigger hunks
  size_t ct = 0;
  while((ct = fread(buf, /*size*/ 1, /*nmemb*/ 1, fp)) > 0)
  {
    assert(ct == 1);
    g_byte_array_append(gbarray, buf, ct);
  }
  fclose(fp);

bye:
  if(debug)
  {
    if(gbarray)
      printf("(%s) Returning GByteArray of size %d", __func__, gbarray->len);
    else
      printf("(%s) Returning NULL", __func__);
  }
  return gbarray;
}


/** Checks if a regular file exists.
 *
 * @param fqfn fully qualified file name
 * @return true/false
 */
bool regular_file_exists(const char * fqfn)
{
  bool        result = false;
  struct stat stat_buf;
  int         rc = stat(fqfn, &stat_buf);
  if(rc == 0)
  {
    //result = S_ISREG(stat_buf.st_mode);
  }
  return result;
}


/** Checks if a directory exists.
 *
 * @param fqfn fully qualified directory name
 * @return true/false
 */
bool directory_exists(const char * fqfn)
{
  bool        result = false;
  struct stat stat_buf;
  int         rc = stat(fqfn, &stat_buf);
  if(rc == 0)
  {
    //result = S_ISDIR(stat_buf.st_mode);
  }
  return result;
}


/** Scans list of directories to obtain file names matching a criterion
 *
 *  @param dirnames     null terminated array of pointers to directory names
 *  @param filter_func  tests directory entry
 *
 *  @return  GPtrArray of fully qualified file names
 *
 *  A free function is set on the returned GPtrArray, so g_ptr_array_free() releases
 *  all the file names
 *
 * Adapted from usbmonctl
 */
//GPtrArray * get_filenames_by_filter(const char * dirnames[], Dirent_Filter filter_func)
//{
//  return 0;
//
//// const char *hiddev_paths[] = { "/dev/", "/dev/usb/", NULL };
//bool        debug    = false;
//GPtrArray * devnames = g_ptr_array_new();
//g_ptr_array_set_free_func(devnames, free);
//char path[PATH_MAX];

//for(int i = 0; dirnames[i] != NULL; i++)
//{
//  struct dirent ** filelist;

//  int count = scandir(dirnames[i], &filelist, filter_func, alphasort);
//  if(count < 0)
//  {
//    assert(count == -1);
//    fprintf(stderr, "(%s) scandir() error: %s\n", __func__, strerror(errno));
//    continue;
//  }
//  for(int j = 0; j < count; j++)
//  {
//    snprintf(path, PATH_MAX, "%s%s", dirnames[i], filelist[j]->d_name);
//    g_ptr_array_add(devnames, strdup(path));
//    free(filelist[j]);
//  }
//  free(filelist);
//}

//if(debug)
//{
//  printf("(%s) Found %d file names:\n", __func__, devnames->len);
//  for(int ndx = 0; ndx < devnames->len; ndx++)
//    printf("   %s\n", (char *)g_ptr_array_index(devnames, ndx));
//}
//return devnames;
//}


/** Gets the file name for a file descriptor
 *
 * @param  fd    file descriptor
 * @param  p_fn  where to return a pointer to the file name
 *               The caller is responsible for freeing this memory
 *
 * @retval 0      success
 * @retval -errno if error (see readlink() doc for possible error numbers
 */
int filename_for_fd(int fd, char ** p_fn)
{
  //char * result = calloc(1, PATH_MAX + 1);
  //char   workbuf[40];
  //int    rc = 0;
  //snprintf(workbuf, 40, "/proc/self/fd/%d", fd);
  //size_t ct = readlink(workbuf, result, PATH_MAX);
  //if(ct < 0)
  //{
  //  rc = -errno;
  //  free(result);
  //  *p_fn = NULL;
  //}
  //else
  //{
  //  assert(ct <= PATH_MAX);
  //  result[ct] = '\0';
  //  *p_fn      = result;
  //}
  // printf("(%s) fd=%d, returning: %d, *pfn=%p -> |%s|\n",
  //        __func__, fd, rc, *pfn, *pfn);
  return 0;
}

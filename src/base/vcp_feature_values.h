/* vcp_feature_values.h
 *
 * Created on: Jan 26, 2016
 *     Author: rock
 *
 * <copyright>
 * Copyright (C) 2014-2015 Sanford Rockowitz <rockowitz@minsoft.com>
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

#ifndef SRC_BASE_VCP_FEATURE_VALUES_H_
#define SRC_BASE_VCP_FEATURE_VALUES_H_

#include <glib.h>

#include "util/coredefs.h"
#include "util/data_structures.h"

#include "base/ddc_packets.h"

typedef struct {
   Byte          opcode;
   VCP_Call_Type value_type;      // probably a different type would be better
#ifdef OLD
   ushort        value;
#endif
   union {
      struct {
         Byte *  bytes;
         ushort  bytect;
         Buffer* buffer;    // temp for transition
      }          t;
#ifdef NO
      // Can't overlay max_val/cur_val on top of bytes mh,ml/sh,sl: depends on endian-ness !
      struct {
         ushort max_val;
         ushort cur_val;
      }         c;
#endif
      struct {
         // little-endian
         Byte ml;
         Byte mh;
         Byte sl;
         Byte sh;
         ushort max_val;
         ushort cur_val;
      }         nt;
   }            val;
} Single_Vcp_Value;

Single_Vcp_Value *
create_nontable_vcp_value(
      Byte     feature_code,
      Byte     mh,
      Byte     ml,
      Byte     sh,
      Byte     sl);

Single_Vcp_Value *
create_cont_vcp_value(
      Byte     feature_code,
      ushort   max_val,
      ushort   cur_val);

Single_Vcp_Value *
create_table_vcp_value_by_bytes(
      Byte     feature_code,
      Byte *   bytes,
      ushort   bytect);

Single_Vcp_Value *
create_table_vcp_value_by_buffer(
      Byte     feature_code,
      Buffer*  buffer);

Single_Vcp_Value *
create_single_vcp_value_by_parsed_vcp_response(
      Byte feature_id,
      Parsed_Vcp_Response * presp);

Parsed_Vcp_Response * single_vcp_value_to_parsed_vcp_response(Single_Vcp_Value * valrec);

void free_single_vcp_value(Single_Vcp_Value * vcp_value);

void report_single_vcp_value(Single_Vcp_Value * val, int depth);


typedef GPtrArray *  Vcp_Value_Set;

Vcp_Value_Set vcp_value_set_new(int initial_size);

void free_vcp_value_set(Vcp_Value_Set vset);

void vcp_value_set_add(Vcp_Value_Set vset,  Single_Vcp_Value * pval);

int vcp_value_set_size(Vcp_Value_Set vset);

Single_Vcp_Value * vcp_value_set_get(Vcp_Value_Set vset, int ndx);


void report_vcp_value_set(Vcp_Value_Set vset, int depth);



#endif /* SRC_BASE_VCP_FEATURE_VALUES_H_ */
/* ddcutil_types.h
 *
 * Publicly visible type definitions.
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

#ifndef DDCUTIL_TYPES_H_
#define DDCUTIL_TYPES_H_

#include <stdint.h>

/** @file ddcutil_types.h
 *  @brief ddcutil public types
 */


//
// Build Information
//

/** ddcutil version */
typedef struct {
   uint8_t    major;
   uint8_t    minor;
   uint8_t    build;
} DDCA_Ddcutil_Version_Spec;


//
// I2C Protocol Control
//

/** I2C timeout types */
typedef enum{
   DDCA_TIMEOUT_STANDARD,      /**< Normal retry interval */
   DDCA_TIMEOUT_TABLE_RETRY    /**< Special timeout for Table reads and writes */
} DDCA_Timeout_Type;

/** I2C retry limit types */
typedef enum{
   DDCA_WRITE_ONLY_TRIES,     /**< Maximum write-only operation tries */
   DDCA_WRITE_READ_TRIES,     /**< Maximum read-write operation tries */
   DDCA_MULTI_PART_TRIES      /**< Maximum multi-part operation tries */
} DDCA_Retry_Type;


//
// Message Control
//

/** Output Level
 *
 * Values assigned to constants allow them to be or'd in bit flags.
 *
 * Values are ascending in order of verbosity
 */
typedef enum {
   // TODO: prefix constants with DDCA_ ??
   // OL_DEFAULT=0x01,    // used only within command line parser
   // OL_PROGRAM=0x02,
   OL_TERSE  =0x04,         /**< Brief   output  */
   OL_NORMAL =0x08,         /**< Normal  output */
   OL_VERBOSE=0x10          /**< Verbose output */
} DDCA_Output_Level;


//
//  Display Specification
//

/** Opaque display identifier
 *
 * A display identifier holds the criteria for selecting a display,
 * typically as specified by the user.
 *
 * It can take several forms:
 * - the display number assigned by dccutil
 * - an I2C bus number
 * - an ADL (adapter index, display index) pair
 * - a  USB (bus number, device number) pair
 * - an EDID
 * - model and serial number strings
 * */
typedef void * DDCA_Display_Identifier;

/** Opaque display reference.
 *
 * A display reference references a display using the identifiers by which it is accessed
 * in Linux.  It takes one of three forms:
 * - an I2C bus number
 * - an ADL (adapter index, display index) pair
 * - a  USB (bus number, device number pair)
 */
typedef void * DDCA_Display_Ref;


/** Opaque display handle
 *
 * A display handle represents an open display on which actions can be performed.
 */
typedef void * DDCA_Display_Handle;


//
// Display Information
//

typedef enum {
   DDC_IO_DEVI2C,
   DDC_IO_ADL,
   USB_IO
} MCCS_IO_Mode;

// Or make this DDCA_Display_Info  ??, with DDCA_Display_Ref as field?
#define DDCA_DISPLAY_INFO_MARKER "DDIN"
typedef struct {
   char             marker[4];
   int              dispno;
   MCCS_IO_Mode     io_mode;
   int              i2c_busno;
   int              iAdapterIndex;
   int              iDisplayIndex;
   int              usb_bus;
   int              usb_device;

   // or should these be actual character/byte arrays instead of pointers?
   const char *     mfg_id;
   const char *     model_name;
   const char *     sn;
   const uint8_t *  edid_bytes;
   DDCA_Display_Ref ddca_dref;
} DDCA_Display_Info;


typedef struct {
   int                ct;
   DDCA_Display_Info  info[];   // array whose size is determined by ct
} DDCA_Display_Info_List;


//
// VCP Feature Information
//

/** MCCS Version in binary form */
typedef struct {
   uint8_t    major;
   uint8_t    minor;
} DDCA_MCCS_Version_Spec;

// in sync w constants MCCS_V.. in vcp_feature_codes.c
/** MCCS (VCP) Feature Version IDs */
typedef enum {
   DDCA_VNONE =  0,     /**< As query, match any MCCS version, as response, version unknown */
   DDCA_V10   =  1,     /**< MCCS v1.0 */
   DDCA_V20   =  2,     /**< MCCS v2.0 */
   DDCA_V21   =  4,     /**< MCCS v2.1 */
   DDCA_V30   =  8,     /**< MCCS v3.0 */
   DDCA_V22   = 16      /**< MCCS v2.2 */
} DDCA_MCCS_Version_Id;

#define DDCA_VUNK  DDCA_VNONE    /**< For use on queries,   indicates match any version */
#define DDCA_VANY  DDCA_VNONE    /**< For use on responses, indicates version unknown   */

/** MCCS VCP Feature Id */
typedef uint8_t VCP_Feature_Code;

/** Flags specifying VCP feature attributes, which can be VCP version dependent. */
typedef uint16_t DDCA_Version_Feature_Flags;

// Bits in DDCA_Version_Feature_Flags:

// Exactly 1 of DDCA_RO, DDCA_WO, DDCA_RW is set
#define DDCA_RO           0x0400               /**< Read only feature */
#define DDCA_WO           0x0200               /**< Write only feature */
#define DDCA_RW           0x0100               /**< Feature is both readable and writable */
#define DDCA_READABLE     (DDCA_RO | DDCA_RW)  /**< Feature is either RW or RO */
#define DDCA_WRITABLE     (DDCA_WO | DDCA_RW)  /**< Feature is either RW or WO */

// Further refine the C/NC/TABLE categorization of the MCCS spec
// Exactly 1 of the following 7 bits is set
#define DDCA_STD_CONT       0x80       /**< Normal continuous feature */
#define DDCA_COMPLEX_CONT   0x40       /**< Continuous feature with special interpretation */
#define DDCA_SIMPLE_NC      0x20       /**< Non-continuous feature, having a defined list of values in byte SL */
#define DDCA_COMPLEX_NC     0x10       /**< Non-continuous feature, having a complex interpretation using one or more of SL, SH, ML, MH */
// For WO NC features.  There's no interpretation function or lookup table
// Used to mark that the feature is defined for a version
#define DDCA_WO_NC          0x08       // TODO: CHECK USAGE
#define DDCA_READABLE_TABLE 0x04       /**< Normal table type feature */
#define DDCA_WO_TABLE       0x02       /**< Write only table feature */

#define DDCA_CONT           (DDCA_STD_CONT|DDCA_COMPLEX_CONT)            /**< Continuous feature, of any subtype */
#define DDCA_NC             (DDCA_SIMPLE_NC|DDCA_COMPLEX_NC|DDCA_WO_NC)  /**< Non-continuous feature of any subtype */
#define DDCA_NON_TABLE      (DDCA_CONT | DDCA_NC)                        /**< Non-table feature of any type */

#define DDCA_TABLE          (DDCA_READABLE_TABLE | DDCA_WO_TABLE)        /**< Table type feature, of any subtype */
#define DDCA_KNOWN          (DDCA_CONT | DDCA_NC | DDCA_TABLE)           // TODO: Usage??? Check

// Additional bits:
#define DDCA_DEPRECATED     0x01     /**< Feature is deprecated in the specified VCP version */


// Bits in vcp_global_flags:
#define DDCA_SYNTHETIC      0x80


/** One entry in array listing defined simple NC values.
 *
 * An entry of {0x00,NULL} terminates the list.
 */
typedef
struct {
   uint8_t   value_code;
   char *    value_name;
} DDCA_Feature_Value_Entry;

// Makes reference to feature value table less implementation specific
typedef DDCA_Feature_Value_Entry * DDCA_Feature_Value_Table;


#define VCP_VERSION_SPECIFIC_FEATURE_INFO_MARKER "VSFI"
/** Describes a VCP feature code, tailored for a specific VCP version */
typedef
struct {
   char                                  marker[4];      /**< equals VCP_VERSION_SPECIFIC_FEATURE_INFO_MARKER */
   VCP_Feature_Code                 feature_code;   /**< VCP Feature code */
   DDCA_MCCS_Version_Spec                vspec;            // ???
   DDCA_MCCS_Version_Id                  version_id;       // which ?
   char *                                desc;
   // Format_Normal_Feature_Detail_Function nontable_formatter;
   // Format_Table_Feature_Detail_Function  table_formatter;
    DDCA_Feature_Value_Table             sl_values;     /**< valid when DDCA_SIMPLE_NC set */
   uint8_t                               global_flags;
   // VCP_Feature_Subset                 vcp_subsets;   // Need it?
   char *                                feature_name;
   DDCA_Version_Feature_Flags            feature_flags;
} Version_Feature_Info;


//
// Get and set VCP feature values
//

// include interpreted string?
typedef struct {
   uint8_t  mh;
   uint8_t  ml;
   uint8_t  sh;
   uint8_t  sl;
   int      max_value;
   int      cur_value;
} DDCA_Non_Table_Value_Response;

typedef struct {
   int      bytect;
   uint8_t  bytes[];     // or uint8_t * ?
} DDCA_Table_Value_Response;


typedef enum {
   NON_TABLE_VCP_VALUE,
   TABLE_VCP_VALUE,
} Vcp_Value_Type;


typedef struct {
   uint8_t        opcode;
   Vcp_Value_Type value_type;      // probably a different type would be better
   union {
      struct {
         uint8_t *  bytes;
         uint16_t   bytect;
      }         t;
      struct {
         uint16_t   max_val;
         uint16_t   cur_val;
      }         c;
      struct {
#ifdef WORDS_BIGENDIAN
         uint8_t    mh;
         uint8_t    ml;
         uint8_t    sh;
         uint8_t    sl;
#else
         uint8_t    ml;
         uint8_t    mh;
         uint8_t    sl;
         uint8_t    sh;
#endif
      }         nc;
   }       val;
} Single_Vcp_Value;



#endif /* DDCUTIL_TYPES_H_ */
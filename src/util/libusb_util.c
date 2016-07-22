/* libusb_util.c
 *
 * <copyright>
 * Copyright (C) 2016 Sanford Rockowitz <rockowitz@minsoft.com>
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

// Adapted from usbplay2 file libusb_util.c

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <wchar.h>

#include "util/string_util.h"
#include "util/report_util.h"
#include "util/device_id_util.h"
#include "util/hid_report_descriptor.h"
#include "util/usb_hid_common.h"
#include "util/libusb_reports.h"

#include "util/libusb_util.h"


//
// Utility functions
//

// copied and modified from make_path() in libusb/hid.c in hidapi

char *make_path(int bus_number, int device_address, int interface_number)
{
   char str[64];
   snprintf(str, sizeof(str), "%04x:%04x:%02x",
      bus_number,
      device_address,
      interface_number);
   str[sizeof(str)-1] = '\0';

   return strdup(str);
}


char *make_path_from_libusb_device(libusb_device *dev, int interface_number)
{
   return make_path(libusb_get_bus_number(dev), libusb_get_device_address(dev), interface_number);
}


//
// Possible_Monitor_Device reporting and lifecycle
//

struct possible_monitor_device * new_possible_monitor_device() {
   struct possible_monitor_device * cur = calloc(1, sizeof(struct possible_monitor_device));
   return cur;
}


void report_possible_monitor_device(struct possible_monitor_device * mondev, int depth) {
   int d1 = depth+1;

   rpt_structure_loc("possible_monitor_device", mondev, depth);

   rpt_vstring(d1, "%-20s   %p",     "libusb_device",       mondev->libusb_device);
   rpt_vstring(d1, "%-20s   %d",     "bus",                 mondev->bus);
   rpt_vstring(d1, "%-20s   %d",     "device_address",      mondev->device_address);
   rpt_vstring(d1, "%-20s   0x%04x", "vid",                 mondev->vid);
   rpt_vstring(d1, "%-20s   0x%04x", "pid",                 mondev->pid);
   rpt_vstring(d1, "%-20s   %d",     "interface",           mondev->interface);
   rpt_vstring(d1, "%-20s   %d",     "alt_setting",         mondev->alt_setting);
   rpt_vstring(d1, "%-20s   %s",     "manufacturer_name",   mondev->manufacturer_name);
   rpt_vstring(d1, "%-20s   %s",     "product_name",        mondev->product_name);
   rpt_vstring(d1, "%-20s   %s",     "serial_number_ascii", mondev->serial_number);
// rpt_vstring(d1, "%-20s   %S",     "serial_number_wide",  mondev->serial_number_wide);
   rpt_vstring(d1, "%-20s   %p",      "next_sibling",       mondev->next);
}


void report_possible_monitors(struct possible_monitor_device * mondev_head, int depth) {
   rpt_title("Possible monitor devices:", depth);
   if (!mondev_head) {
      rpt_title("None", depth+1);
   }
   else {
      struct possible_monitor_device * cur = mondev_head;
      while(cur) {
         report_possible_monitor_device(cur, depth+1);
         cur = cur->next;
      }
   }
}


typedef struct descriptor_path {
   unsigned short busno;
   unsigned short devno;
   struct libusb_device_descriptor *    desc;
   struct libusb_device *               dev;
   struct libusb_config_descriptor *    config;
   struct libusb_interface *            interface;
   struct libusb_interface_descriptor * inter;
} Descriptor_Path;


void report_descriptor_path(Descriptor_Path * pdpath, int depth) {
   int d1 = depth+1;

   rpt_structure_loc("Descriptor_Path", pdpath, depth);
   rpt_vstring(d1, "%-20s %d", "busno:", pdpath->busno);
   rpt_vstring(d1, "%-20s %d", "devno:", pdpath->devno);
   rpt_vstring(d1, "%-20s %p", "desc:",  pdpath->desc);
   rpt_vstring(d1, "%-20s %p", "dev:",  pdpath->dev);
   rpt_vstring(d1, "%-20s %p", "config:",  pdpath->config);
   rpt_vstring(d1, "%-20s %p", "interface:",  pdpath->interface);
   rpt_vstring(d1, "%-20s %p", "inter:",  pdpath->inter);
}


//
// Identify HID interfaces that that are not keyboard or mouse
//

static bool possible_monitor_interface_descriptor(
      const struct libusb_interface_descriptor * inter, Descriptor_Path dpath)
{
   bool debug = false;
   if (debug)
      printf("(%s) Starting.\n", __func__);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
   dpath.inter = inter;
#pragma GCC diagnostic pop

   bool result = false;
   if (inter->bInterfaceClass == LIBUSB_CLASS_HID) {
      if (debug) {
         rpt_vstring(0, "bInterfaceClass:     0x%02x (%d)", inter->bInterfaceClass,    inter->bInterfaceClass);
         rpt_vstring(0, "bInterfaceSubClass:  0x%02x (%d)", inter->bInterfaceSubClass, inter->bInterfaceSubClass);
         rpt_int("bInterfaceProtocol", NULL, inter->bInterfaceProtocol, 0);
      }
      if (inter->bInterfaceProtocol != 1 && inter->bInterfaceProtocol != 2)
         result = true;
   }
   if (debug)
      printf("(%s) Returning %s\n", __func__, bool_repr(result));
   return result;
}


static bool possible_monitor_interface(
      const struct libusb_interface * interface, Descriptor_Path dpath)
{
   bool debug = false;
   if (debug)
      printf("(%s) Starting.\n", __func__);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
   dpath.interface = interface;
#pragma GCC diagnostic pop

   bool result = false;
   int ndx;
   for (ndx=0; ndx<interface->num_altsetting; ndx++) {
      const struct libusb_interface_descriptor * idesc = &interface->altsetting[ndx];
      // report_interface_descriptor(idesc, d1);
      result |= possible_monitor_interface_descriptor(idesc, dpath);
   }

   if (debug)
      printf("(%s) Returning: %s\n", __func__, bool_repr(result));
   return result;
}


static bool possible_monitor_config_descriptor(
      const struct libusb_config_descriptor * config, Descriptor_Path dpath)
{
    bool result = false;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
    dpath.config = config;
#pragma GCC diagnostic pop

    int ndx = 0;
    if (config->bNumInterfaces > 1) {
       // report_descriptor_path(&dpath, 0);
       uint16_t vid = dpath.desc->idVendor;
       uint16_t pid = dpath.desc->idProduct;

       Pci_Usb_Id_Names names;
       names = devid_get_usb_names(
                       vid,
                       pid,
                       0,
                       2);
       printf("(%s) Examining only interface 0 for device %d:%d, vid=0x%04x, pid=0x%04x  %s %s\n",
              __func__, dpath.busno, dpath.devno, vid, pid, names.vendor_name, names.device_name);
    }
    // HACK: look at only interface 0
    // for (ndx=0; ndx<config->bNumInterfaces; ndx++) {
       const struct libusb_interface *inter = &(config->interface[ndx]);
       result |= possible_monitor_interface(inter, dpath);
    // }

    // if (result)
    //    printf("(%s) Returning: %d\n", __func__, result);
    return result;
}


bool possible_monitor_dev(libusb_device * dev, bool check_forced_monitor, Descriptor_Path dpath) {
   bool debug = false;
   if (debug)
      printf("(%s) Starting. dev=%p\n", __func__, dev);
   bool result = false;

   struct libusb_config_descriptor *config;
   libusb_get_config_descriptor(dev, 0, &config);

   struct libusb_device_descriptor desc;
    // copies data into struct pointed to by desc, does not allocate:
    int rc = libusb_get_device_descriptor(dev, &desc);
    if (rc < 0)
       REPORT_LIBUSB_ERROR("libusb_get_device_descriptor",  rc, LIBUSB_EXIT);
   dpath.desc = &desc;
   dpath.dev  = dev;
   result = possible_monitor_config_descriptor(config, dpath);

   libusb_free_config_descriptor(config);

   if (!result && check_forced_monitor) {
        struct libusb_device_descriptor desc;
      int rc = libusb_get_device_descriptor(dev, &desc);
      CHECK_LIBUSB_RC("libusb_device_descriptor", rc, LIBUSB_EXIT);
      ushort vid = desc.idVendor;
      ushort pid = desc.idProduct;

      if (debug)
           printf("(%s) Callling force_hid_monitor_by_vid_pid(0x%04x, 0x%04x)\n",
                  __func__, desc.idVendor, desc.idProduct );
      result = force_hid_monitor_by_vid_pid(vid, pid);
   }

   if (debug)
      printf("(%s) Returning: %s\n" , __func__, bool_repr(result));
   return result;
}


// Not currently used.
// check_forced_monitor not implemented
// apparently can get by without collected information in struct possible_monitor_device

struct possible_monitor_device *
alt_possible_monitor_dev(
      libusb_device *  dev,
      bool             check_forced_monitor)   // NOT CURRENTLY USED!
{
   bool debug = true;
   if (debug)
      printf("(%s) Starting. dev=%p, check_forced_monitor=%s\n",
             __func__, dev, bool_repr(check_forced_monitor));

   struct possible_monitor_device * new_node = NULL;

   // report_dev(dev, NULL, false /* show hubs */, 0);
   int bus            = libusb_get_bus_number(dev);
   int device_address = libusb_get_device_address(dev);
   ushort vid = 0;
   ushort pid = 0;

   struct libusb_device_descriptor desc;
   int rc = libusb_get_device_descriptor(dev, &desc);
   CHECK_LIBUSB_RC("libusb_device_descriptor", rc, LIBUSB_EXIT);
   vid = desc.idVendor;
   pid = desc.idProduct;

   struct libusb_config_descriptor * config;
   rc = libusb_get_config_descriptor(dev, 0, &config);   // returns a pointer
   CHECK_LIBUSB_RC("libusb_config_descriptor", rc, LIBUSB_EXIT);

   // Logitech receiver has subclass 0 on interface 2,
   // try ignoring all interfaces other than 0
   int inter_no = 0;
   // for (inter_no=0; inter_no<config->bNumInterfaces; inter_no++) {
   {
      const struct libusb_interface *inter = &(config->interface[inter_no]);

      int altset_no;
      for (altset_no=0; altset_no<inter->num_altsetting; altset_no++) {
         const struct libusb_interface_descriptor * idesc  = &inter->altsetting[altset_no];

         if (idesc->bInterfaceClass == LIBUSB_CLASS_HID) {
            rpt_vstring(0, "bInterfaceClass:     0x%02x (%d)", idesc->bInterfaceClass,    idesc->bInterfaceClass);
            rpt_vstring(0, "bInterfaceSubClass:  0x%02x (%d)", idesc->bInterfaceSubClass, idesc->bInterfaceSubClass);
            rpt_int("bInterfaceProtocol", NULL, idesc->bInterfaceProtocol, 0);

            if (idesc->bInterfaceProtocol != 1 && idesc->bInterfaceProtocol != 2)
            {
               // TO ADDRESS: WHAT IF MULTIPLE altsettings?  what if they conflict?

               new_node = new_possible_monitor_device();
               libusb_ref_device(dev);
               new_node->libusb_device = dev;
               new_node->bus = bus;
               new_node->device_address = device_address;
               new_node->alt_setting = altset_no;
               new_node->interface = inter_no;
               new_node->vid = vid;
               new_node->pid = pid;

               // if (debug)
               //    report_dev(dev, NULL, false, 0);

               struct libusb_device_handle * dh = NULL;
               rc = libusb_open(dev, &dh);
               if (rc < 0) {
                  REPORT_LIBUSB_ERROR("libusb_open", rc, LIBUSB_CONTINUE);
                  dh = NULL;   // belt and suspenders
               }
               else {
                  printf("(%s) Successfully opened\n", __func__);
                  rc = libusb_set_auto_detach_kernel_driver(dh, 1);
               }



               if (dh) {
                  if (true) {     // TEMP - should be debug
                  printf("Manufacturer:  %d - %s\n",
                            desc.iManufacturer,
                            lookup_libusb_string(dh, desc.iManufacturer) );
                  printf("Product:  %d - %s\n",
                            desc.iProduct,
                            lookup_libusb_string(dh, desc.iProduct) );
                  printf("Serial number:  %d - %s\n",
                                       desc.iSerialNumber,
                                       lookup_libusb_string(dh, desc.iSerialNumber) );
                  }
                  new_node->manufacturer_name = strdup(lookup_libusb_string(dh, desc.iManufacturer));
                  new_node->product_name      = strdup(lookup_libusb_string(dh, desc.iProduct));
                  new_node->serial_number     = strdup(lookup_libusb_string(dh, desc.iSerialNumber));
               // new_node->serial_number_wide = wcsdup(lookup_libusb_string_wide(dh, desc.iSerialNumber));
                  // printf("(%s) serial_number_wide = |%S|\n", __func__, new_node->serial_number_wide);

                  // report_device_descriptor(&desc, NULL, d1);
                  // report_open_libusb_device(dh, 1);
                  libusb_close(dh);
               }
            }
         }
      }
   }
   libusb_free_config_descriptor(config);

   if (debug)
      printf("(%s) Returning %p\n", __func__, new_node);
   return new_node;
}


/** Examines a null terminated list of pointers to struct libusb_device
 *
 *  Returns a linked list of struct possible_monitor_device that represents
 *  possible monitors
 */
// not currently used
struct possible_monitor_device *
get_possible_monitors(
       libusb_device **devs      // null terminated list
      )
{
   bool debug = true;
   if (debug)
      printf("(%s) Starting\n", __func__);

   Possible_Monitor_Device * last_device = new_possible_monitor_device();
   Possible_Monitor_Device * head_device = last_device;
   libusb_device *dev;
   int i = 0;
   while ((dev = devs[i++]) != NULL) {
      Possible_Monitor_Device * possible_monitor =
            alt_possible_monitor_dev(dev, /* check_forced_monitor= */ true);
      if (possible_monitor) {
         last_device->next = possible_monitor;
         last_device = possible_monitor;
      }
   }

   struct possible_monitor_device * true_head;
   true_head = head_device->next;
   if (debug)
      printf("(%s) Returning: %p\n", __func__, true_head);
   return true_head;
}



// From usbplay2.c

libusb_device ** collect_possible_monitor_devs( libusb_device **devs) {
   bool debug = false;
   if (debug)
      printf("(%s) Starting.\n", __func__);

   libusb_device *dev;

#ifdef OUT
   int has_detach_kernel_capability =
         libusb_has_capability(LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER);
   // printf("(%s) %s kernel detach driver capability\n",
   //        __func__,
   //        (has_detach_kernel_capability) ? "Has" : "Does not have");
#endif
   int devct = 0;
   while ( devs[devct++] ) {}

   libusb_device ** result = calloc(devct+1, sizeof(libusb_device *));
   int foundndx = 0;

   int i = 0;
   while ((dev = devs[i++]) != NULL) {
      unsigned short busno =  libusb_get_bus_number(dev);
      unsigned short devno =  libusb_get_device_address(dev);

      Descriptor_Path dpath;
      memset(&dpath, 0, sizeof(Descriptor_Path));
      dpath.busno = busno;
      dpath.devno = devno;


      struct libusb_device_descriptor desc;
      // copies data into struct pointed to by desc, does not allocate:
      int rc = libusb_get_device_descriptor(dev, &desc);
      if (rc < 0)
         REPORT_LIBUSB_ERROR("libusb_get_device_descriptor",  rc, LIBUSB_EXIT);
      dpath.desc = &desc;


      if (debug)
         printf("(%s) Examining device. bus=0x%04x, device=0x%04x\n", __func__, busno, devno);

      bool possible = possible_monitor_dev(dev, /*check_forced_monitor=*/ true, dpath);

      if (possible) {
         uint16_t vid = dpath.desc->idVendor;
         uint16_t pid = dpath.desc->idProduct;

         Pci_Usb_Id_Names names;
         names = devid_get_usb_names(
                         vid,
                         pid,
                         0,
                         2);


         printf("Found potential HID device %d:%d, vid=0x%04x, pid=0x%04x  %s %s\n",
                dpath.busno, dpath.devno, vid, pid, names.vendor_name,
                (names.device_name) ? names.device_name : "(unrecognized pid)");

         result[foundndx++] = dev;

#ifdef OUT
         struct libusb_device_handle * dh = NULL;
         int rc = libusb_open(dev, &dh);
         if (rc < 0) {
            REPORT_LIBUSB_ERROR("libusb_open", rc, LIBUSB_CONTINUE);
            dh = NULL;   // belt and suspenders
         }
         else {
            printf("(%s) Successfully opened\n", __func__);
            if (has_detach_kernel_capability) {
               rc = libusb_set_auto_detach_kernel_driver(dh, 1);
               if (rc < 0) {
                  REPORT_LIBUSB_ERROR("libusb_set_auto_detach_kernel_driver", rc, LIBUSB_CONTINUE);
               }
            }
         }

         report_dev(dev, dh, /*show_hubs=*/ false, 0);


         if (dh) {
            struct libusb_device_descriptor desc;
            // copies data into struct pointed to by desc, does not allocate:
            rc = libusb_get_device_descriptor(dev, &desc);
            if (rc < 0)
               REPORT_LIBUSB_ERROR("libusb_get_device_descriptor",  rc, LIBUSB_EXIT);

            printf("Manufacturer:  %d - %s\n",
                      desc.iManufacturer,
                      lookup_libusb_string(dh, desc.iManufacturer) );

            wprintf(L"Manufacturer (wide) %d -%ls\n",
                  desc.iManufacturer,
                    lookup_libusb_string_wide(dh, desc.iManufacturer) );


            printf("Product:  %d - %s\n",
                      desc.iProduct,
                      lookup_libusb_string(dh, desc.iProduct) );

            printf("Serial number:  %d - %s\n",
                                 desc.iSerialNumber,
                                 lookup_libusb_string(dh, desc.iSerialNumber) );

            // report_device_descriptor(&desc, NULL, d1);
            // report_open_libusb_device(dh, 1);
            libusb_close(dh);

         }
#endif
      }
   }

   if (debug)
      printf("(%s) Returing: %p\n", __func__, result);
   return result;
}


/* Probe USB HID devices using libusb facilities.
 *
 * Arguments:
 *    possible_monitors_only   if true, only show detail for possible monitors
 *
 * Returns:  nothing
 */
void probe_libusb(bool possible_monitors_only) {
   bool debug = false;
   if (debug)
      printf("(%s) Starting\n", __func__);

   bool ok = devid_ensure_initialized();
   if (!ok) {
      printf("(%s) devid_ensure_initialized() failed.  Terminating probe_libusb()\n", __func__);
      return;
   }

   libusb_device **devs;
   // libusb_context *ctx = NULL; //a libusb session
   int rc;
   ssize_t cnt;   // number of devices in list

   // rc = libusb_init(&ctx);   // initialize a library session
   rc = libusb_init(NULL);      // initialize a library session, use default context
   CHECK_LIBUSB_RC("libusb_init", rc, LIBUSB_EXIT);
   // libusb_set_debug(ctx,3);
   libusb_set_debug(NULL /*default context*/, LIBUSB_LOG_LEVEL_INFO);

   // cnt = libusb_get_device_list(ctx, &devs);
   cnt = libusb_get_device_list(NULL /* default context */, &devs);
   CHECK_LIBUSB_RC("libusb_get_device_list", (int) cnt, LIBUSB_EXIT);

   libusb_device ** devs_to_show = devs;
   if (possible_monitors_only)
      devs_to_show = collect_possible_monitor_devs(devs);
   report_libusb_devices(devs_to_show,
                         false,         // show_hubs
                         0);            // depth

   libusb_free_device_list(devs, 1 /* unref the devices in the list */);

   // libusb_exit(ctx);
   libusb_exit(NULL);
   if (debug)
      printf("(%s) Done\n", __func__);

}


//  Linux driver for the uDraw graphic tablet
//  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmail.com>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <libusb-1.0/libusb.h>
#include <boost/format.hpp>
#include <memory>
#include <stdio.h>
#include <string.h>
#include <boost/format.hpp>
#include <signal.h>
#include <usb.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include "usb_device.hpp"
#include "evdev.hpp"
#include "udraw_decoder.hpp"

class USBDevice;

void print_raw_data(std::ostream& out, uint8_t* data, int len);
bool global_interrupt = false;

struct usb_device*
find_usb_device(uint16_t idVendor, uint16_t idProduct)
{
  struct usb_bus* busses = usb_get_busses();

  for (struct usb_bus* bus = busses; bus; bus = bus->next)
  {
    for (struct usb_device* dev = bus->devices; dev; dev = dev->next) 
    {
      if (dev->descriptor.idVendor  == idVendor &&
          dev->descriptor.idProduct == idProduct)
      {
        return dev;
      }
    }
  }
  return 0;
}

void print_raw_data(std::ostream& out, uint8_t* data, int len)
{
  std::cout << "[" << len 
            << "] ";
      
  for(int i = 0; i < len; ++i)
  {
    std::cout << boost::format("[%d]%02x") % i % int(data[i]);
    if (i != len-1)
      std::cout << " ";
  }

}

struct Options
{
  bool gamepad_mode  = false;
  bool keyboard_mode = false;
  bool touchpad_mode = false;
  bool tablet_mode   = false;
  bool accelerometer_mode = false;
};

void print_help(const char* argv0)
{
  std::cout << "Usage: " << argv0 << "[OPTION]...\n" 
            << "Basic driver for the PS3 uDraw graphic tablet\n"
            << "\n"
            << "Options:\n"
            << "  -h, --help  display this help\n"
            << "  --touchpad  use the device as touchpad\n"
            << "  --tablet    use the device as graphic tablet\n"
            << "  --gamepad   use the device as gamepad\n"
            << "  --keyboard  use the device as keyboard\n"
            << "  --accelerometer  use the accelerometer\n"
            << std::endl;
}

Options parse_args(int argc, char** argv)
{
  Options opts;

  for(int i = 1; i < argc; ++i)
  {
    if (strcmp("--gamepad", argv[i]) == 0)
    {
      opts.gamepad_mode = true;
    }
    else if (strcmp("--keyboard", argv[i]) == 0)
    {
      opts.keyboard_mode = true;
    }
    else if (strcmp("--tablet", argv[i]) == 0)
    {
      opts.tablet_mode = true;
    }
    else if (strcmp("--touchpad", argv[i]) == 0)
    {
      opts.touchpad_mode = true;
    }
    else if (strcmp("--accelerometer", argv[i]) == 0)
    {
      opts.accelerometer_mode = true;
    }
    else if (strcmp("--help", argv[i]) == 0 ||
             strcmp("-h", argv[i]) == 0)
    {
      print_help(argv[0]);
      exit(EXIT_SUCCESS);
    }
    else
    {
      std::cerr << "unknown option: " << argv[i] << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  return opts;
}

int main(int argc, char** argv)
{
  Options opts = parse_args(argc, argv);

  usb_init();
  usb_find_busses();
  usb_find_devices();

  struct usb_device* dev = find_usb_device(0x20d6, 0xcb17);

  int acc_x_min = 0;
  int acc_y_min = 0;
  int acc_z_min = 0;

  int acc_x_max = 0;
  int acc_y_max = 0;
  int acc_z_max = 0;

  int touch_pos_x = 0;
  int touch_pos_y = 0;
  bool finger_touching = false;
  bool pinch_touching = false;
  bool scroll_wheel = false;
  int wheel_distance = 0;
            
  if (dev)
  {
    std::unique_ptr<USBDevice> usbdev(new USBDevice(dev));

    usbdev->print_info();
    usbdev->detach_kernel_driver(0);
    usbdev->claim_interface(0);

    Evdev evdev;
    
    // init evdev
    if (opts.gamepad_mode)
    {
      evdev.add_abs(ABS_X, -1, 1, 0, 0);
      evdev.add_abs(ABS_Y, -1, 1, 0, 0);

      evdev.add_key(BTN_A);
      evdev.add_key(BTN_B);
      evdev.add_key(BTN_X);
      evdev.add_key(BTN_Y);

      evdev.add_key(BTN_START);
      evdev.add_key(BTN_SELECT);
    }
    else if (opts.keyboard_mode)
    {
      evdev.add_key(KEY_LEFT);
      evdev.add_key(KEY_RIGHT);
      evdev.add_key(KEY_UP);
      evdev.add_key(KEY_DOWN);

      evdev.add_key(KEY_ENTER);
      evdev.add_key(KEY_SPACE);
      evdev.add_key(KEY_A);
      evdev.add_key(KEY_Z);

      evdev.add_key(KEY_ESC);
      evdev.add_key(KEY_TAB);
    }
    else if (opts.tablet_mode)
    {
      evdev.add_abs(ABS_X, 0, 1913, 0, 0);
      evdev.add_abs(ABS_Y, 0, 1076, 0, 0);
      evdev.add_abs(ABS_PRESSURE, 0, 143, 0, 0);

      evdev.add_key(BTN_TOUCH);
      evdev.add_key(BTN_TOOL_PEN);

      evdev.add_rel(REL_WHEEL);
      evdev.add_rel(REL_HWHEEL);

      evdev.add_rel(REL_X);
      evdev.add_rel(REL_Y);
    }
    else if (opts.touchpad_mode)
    {
      evdev.add_key(BTN_LEFT);
      evdev.add_key(BTN_RIGHT);
      evdev.add_key(BTN_MIDDLE);

    /*
      add_key(KEY_FORWARD);
      add_key(KEY_BACK);
    */

      evdev.add_rel(REL_WHEEL);
      evdev.add_rel(REL_HWHEEL);

      evdev.add_rel(REL_X);
      evdev.add_rel(REL_Y);
    }

    evdev.finish();

    usbdev->listen
      (3,
       [&](uint8_t* data, int size)
       {
         UDrawDecoder decoder(data, size);

         if (opts.keyboard_mode)
         {
           evdev.send(EV_KEY, KEY_LEFT,  decoder.get_left());
           evdev.send(EV_KEY, KEY_RIGHT, decoder.get_right());
           evdev.send(EV_KEY, KEY_UP,    decoder.get_up());
           evdev.send(EV_KEY, KEY_DOWN,  decoder.get_down());

           evdev.send(EV_KEY, KEY_ENTER, decoder.get_cross());
           evdev.send(EV_KEY, KEY_SPACE, decoder.get_circle());
           evdev.send(EV_KEY, KEY_A, decoder.get_square());
           evdev.send(EV_KEY, KEY_Z, decoder.get_triangle());

           evdev.send(EV_KEY, KEY_ESC,  decoder.get_start());
           evdev.send(EV_KEY, KEY_TAB, decoder.get_select());

           evdev.send(EV_SYN, SYN_REPORT, 0);
         }
         else if (opts.gamepad_mode)
         {
           evdev.send(EV_ABS, ABS_X, -1 * decoder.get_left() + 1 * decoder.get_right());
           evdev.send(EV_ABS, ABS_Y, -1 * decoder.get_up()   + 1 * decoder.get_down());

           evdev.send(EV_KEY, BTN_A, decoder.get_cross());
           evdev.send(EV_KEY, BTN_B, decoder.get_circle());
           evdev.send(EV_KEY, BTN_X, decoder.get_square());
           evdev.send(EV_KEY, BTN_Y, decoder.get_triangle());

           evdev.send(EV_KEY, BTN_START,  decoder.get_start());
           evdev.send(EV_KEY, BTN_SELECT, decoder.get_select());

           evdev.send(EV_SYN, SYN_REPORT, 0);
         }
         else if (opts.tablet_mode)
         {
           if (decoder.get_mode() == UDrawDecoder::PEN_MODE)
           {
             evdev.send(EV_ABS, ABS_X, decoder.get_x());
             evdev.send(EV_ABS, ABS_Y, decoder.get_y());
             evdev.send(EV_ABS, ABS_PRESSURE, decoder.get_pressure());
             evdev.send(EV_KEY, BTN_TOOL_PEN, 1);
             
             if (decoder.get_pressure() > 0)
             {
               evdev.send(EV_KEY, BTN_TOUCH, 1);
             }
             else
             {
               evdev.send(EV_KEY, BTN_TOUCH, 0);
             }

             evdev.send(EV_SYN, SYN_REPORT, 0);
           }
           else
           {
             evdev.send(EV_KEY, BTN_TOOL_PEN, 0);
             evdev.send(EV_SYN, SYN_REPORT, 0);
           }
         }
         else if (opts.touchpad_mode)
         {
           evdev.send(EV_KEY, BTN_LEFT,  decoder.get_right());
           evdev.send(EV_KEY, BTN_RIGHT, decoder.get_left());

           if (decoder.get_mode() == UDrawDecoder::FINGER_MODE)
           {
             if (!finger_touching)
             {
               touch_pos_x = decoder.get_x();
               touch_pos_y = decoder.get_y();
               finger_touching = true;

               if (touch_pos_x > 1800)
               {
                 scroll_wheel = true;
                 wheel_distance = 0;
               }
               else
               {
                 scroll_wheel = false;
               }
             }

             if (scroll_wheel)
             {
               wheel_distance += (decoder.get_y() - touch_pos_y) / 10;

               int rel = wheel_distance/10;
               if (rel != 0)
               {
                 evdev.send(EV_REL, REL_WHEEL, -rel);

                 wheel_distance -= rel;
                 touch_pos_x = decoder.get_x();
                 touch_pos_y = decoder.get_y();
                 //std::cout << rel << " " << wheel_distance << std::endl;
               }
             }
             else
             {
               evdev.send(EV_REL, REL_X, decoder.get_x() - touch_pos_x);
               evdev.send(EV_REL, REL_Y, decoder.get_y() - touch_pos_y);

               touch_pos_x = decoder.get_x();
               touch_pos_y = decoder.get_y();
             }
           }
           else
           {
             finger_touching = false;
           }  
           evdev.send(EV_SYN, SYN_REPORT, 0);
         }
         else if (opts.accelerometer_mode)
         {
           if (size != 27)
           {
             std::cerr << "unknown read size: " << size << std::endl;
           }
           else
           {
             //data[0];
             int x = data[15] * 255 + data[17]; // x - pen: 3px resolution
             int y = data[16] * 255 + data[18]; // y - finger: 1px resolution

             if (data[11] == 0x00)
             {
               std::cout << "nothing";
             }
             else if (data[11] == 0x80)
             {
               std::cout << boost::format("finger: x: %4d y: %4d") % x % y;
             }
             else if (data[11] == 0x40)
             {
               std::cout << boost::format("pen: x: %4d y: %4d  - pressure: %3d") % x % y % (int(data[13]) - 0x70);
             }
             else
             {
               std::cout << boost::format("pinch: x: %4d y: %4d  distance: %4d  orientation: %02x") % x % y % int(data[12]) % (int(data[11]) - 0xc0);
             }

             int acc_x = ((data[19] + data[20] * 255) - 512);
             int acc_y = ((data[21] + data[22] * 255) - 512);
             int acc_z = ((data[23] + data[24] * 255) - 512);

             acc_x_min = std::min(acc_x, acc_x_min);
             acc_y_min = std::min(acc_y, acc_y_min);
             acc_z_min = std::min(acc_z, acc_z_min);

             acc_x_max = std::max(acc_x, acc_x_max);
             acc_y_max = std::max(acc_y, acc_y_max);
             acc_z_max = std::max(acc_z, acc_z_max);

             // acc_min -31  -33  -33 
             // acc_max  31   28   29
             // ~22 == 1g

             // accelerometer
             std::cout << boost::format("%4d %4d %4d - %4d %4d %4d - %4d %4d %4d") %
               acc_x % acc_y % acc_z %
               acc_x_min % acc_y_min % acc_z_min %
               acc_x_max % acc_y_max % acc_z_max;

             
             std::cout << std::endl;
           }
         }
         else
         {
           print_raw_data(std::cout, data, size);
           std::cout << std::endl;
         }
           
         if (false)
         {
           evdev.send(EV_KEY, BTN_RIGHT,  decoder.get_left());
           evdev.send(EV_KEY, BTN_MIDDLE, decoder.get_up());
           evdev.send(EV_KEY, BTN_LEFT,   decoder.get_right());

           evdev.send(EV_REL, REL_WHEEL, decoder.get_triangle() ? 1 : 0);
           evdev.send(EV_REL, REL_WHEEL, decoder.get_cross() ? -1 : 0);

           evdev.send(EV_KEY, KEY_BACK,    decoder.get_circle());
           evdev.send(EV_KEY, KEY_FORWARD, decoder.get_square());

           if (false)
           {
             // FIXME: does not work as is, needs throttling

             if (decoder.get_mode() == UDrawDecoder::PINCH_MODE)
             {
               if (!pinch_touching)
               {
                 touch_pos_x = decoder.get_x();
                 touch_pos_y = decoder.get_y();
                 pinch_touching = true;
               }

               evdev.send(EV_REL, REL_HWHEEL, decoder.get_x() - touch_pos_x);
               evdev.send(EV_REL, REL_WHEEL,  decoder.get_y() - touch_pos_y);

               touch_pos_x = decoder.get_x();
               touch_pos_y = decoder.get_y();
             }
             else
             {
               pinch_touching = false;
             }

             evdev.send(EV_SYN, SYN_REPORT, 0);
           }
         }
       });
  }

  return 0;
}

/* EOF */

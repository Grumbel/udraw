//  Linux driver for the uDraw graphic tablet
//  Copyright (C) 2012-2022 Ingo Ruhnke <grumbel@gmail.com>
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

#include "udraw_driver.hpp"

#include <linux/uinput.h>
#include <iostream>

#include <fmt/format.h>

#include "evdev.hpp"
#include "options.hpp"
#include "udraw_decoder.hpp"
#include "usb_device.hpp"

namespace udraw {

namespace {

void print_raw_data(std::ostream& out, uint8_t* data, int len)
{
  std::cout << "[" << len
            << "] ";

  for(int i = 0; i < len; ++i)
  {
    std::cout << fmt::format("[{:d}]{:02x}", i, int(data[i]));
    if (i != len-1)
      std::cout << " ";
  }

}

} // namespace

UDrawDriver::UDrawDriver(USBDevice& usbdev, Evdev& evdev, Options const& opts) :
  m_usbdev(usbdev),
  m_evdev(evdev),
  m_opts(opts)
{
  init_evdev();

  m_usbdev.print_info();
  m_usbdev.detach_kernel_driver(0);
  m_usbdev.claim_interface(0);

  m_usbdev.listen
    (3,
     [&](uint8_t* data, int size)
     {
       UDrawDecoder decoder(data, size);

       if (m_opts.keyboard_mode)
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
       else if (m_opts.gamepad_mode)
       {
         evdev.send(EV_ABS, ABS_X, -1 * decoder.get_left() + 1 * decoder.get_right());
         evdev.send(EV_ABS, ABS_Y, -1 * decoder.get_up()   + 1 * decoder.get_down());

         evdev.send(EV_KEY, BTN_A, decoder.get_cross());
         evdev.send(EV_KEY, BTN_B, decoder.get_circle());
         evdev.send(EV_KEY, BTN_X, decoder.get_square());
         evdev.send(EV_KEY, BTN_Y, decoder.get_triangle());

         evdev.send(EV_KEY, BTN_START,  decoder.get_start());
         evdev.send(EV_KEY, BTN_SELECT, decoder.get_select());
         evdev.send(EV_KEY, BTN_Z, decoder.get_guide());

         evdev.send(EV_SYN, SYN_REPORT, 0);
       }
       else if (m_opts.tablet_mode)
       {
         if (decoder.get_mode() == UDrawDecoder::PEN_MODE)
         {
           evdev.send(EV_ABS, ABS_X, decoder.get_x());
           evdev.send(EV_ABS, ABS_Y, decoder.get_y());
           evdev.send(EV_ABS, ABS_PRESSURE, decoder.get_pressure());
           evdev.send(EV_KEY, BTN_TOOL_PEN, 1);

           if (decoder.get_pressure() > 5)
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
       else if (m_opts.touchpad_mode)
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
       else if (m_opts.accelerometer_mode)
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
             std::cout << fmt::format("finger: x: {:4d} y: {:4d}", x, y);
           }
           else if (data[11] == 0x40)
           {
             std::cout << fmt::format("pen: x: {:4d} y: {:4d}  - pressure: {:3d}", x, y, (int(data[13]) - 0x70));
           }
           else
           {
             std::cout << fmt::format("pinch: x: {:4d) y: {:4d}  distance: {:4d}  orientation: {:02x}", x, y, int(data[12]), (int(data[11]) - 0xc0));
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
           std::cout << fmt::format("{:4d} {:4d} {:4d} - {:4d} {:4d} {:4d} - {:4d} {:4d} {:4d}",
                                    acc_x, acc_y, acc_z,
                                    acc_x_min, acc_y_min, acc_z_min,
                                    acc_x_max, acc_y_max, acc_z_max);

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

void
UDrawDriver::init_evdev()
{
  // init evdev
  if (m_opts.gamepad_mode)
  {
    m_evdev.add_abs(ABS_X, -1, 1, 0, 0);
    m_evdev.add_abs(ABS_Y, -1, 1, 0, 0);

    m_evdev.add_key(BTN_A);
    m_evdev.add_key(BTN_B);
    m_evdev.add_key(BTN_X);
    m_evdev.add_key(BTN_Y);

    m_evdev.add_key(BTN_START);
    m_evdev.add_key(BTN_SELECT);
    m_evdev.add_key(BTN_Z);
  }
  else if (m_opts.keyboard_mode)
  {
    m_evdev.add_key(KEY_LEFT);
    m_evdev.add_key(KEY_RIGHT);
    m_evdev.add_key(KEY_UP);
    m_evdev.add_key(KEY_DOWN);

    m_evdev.add_key(KEY_ENTER);
    m_evdev.add_key(KEY_SPACE);
    m_evdev.add_key(KEY_A);
    m_evdev.add_key(KEY_Z);

    m_evdev.add_key(KEY_ESC);
    m_evdev.add_key(KEY_TAB);
  }
  else if (m_opts.tablet_mode)
  {
    m_evdev.add_abs(ABS_X, 0, 1913, 0, 0);
    m_evdev.add_abs(ABS_Y, 0, 1076, 0, 0);
    m_evdev.add_abs(ABS_PRESSURE, 0, 143, 0, 0);

    m_evdev.add_key(BTN_TOUCH);
    m_evdev.add_key(BTN_TOOL_PEN);

    m_evdev.add_rel(REL_WHEEL);
    m_evdev.add_rel(REL_HWHEEL);

    m_evdev.add_rel(REL_X);
    m_evdev.add_rel(REL_Y);
  }
  else if (m_opts.touchpad_mode)
  {
    m_evdev.add_key(BTN_LEFT);
    m_evdev.add_key(BTN_RIGHT);
    m_evdev.add_key(BTN_MIDDLE);

    /*
      add_key(KEY_FORWARD);
      add_key(KEY_BACK);
    */

    m_evdev.add_rel(REL_WHEEL);
    m_evdev.add_rel(REL_HWHEEL);

    m_evdev.add_rel(REL_X);
    m_evdev.add_rel(REL_Y);
  }

  m_evdev.finish();
}

} // namespace udraw

/* EOF */

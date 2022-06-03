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
#include <logmich/log.hpp>

#include "evdev.hpp"
#include "options.hpp"
#include "udraw_decoder.hpp"
#include "usb_device.hpp"

namespace udraw {

namespace {

void print_raw_data(std::ostream& out, uint8_t const* data, size_t len)
{
  out << "[" << len
            << "] ";

  for(size_t i = 0; i < len; ++i)
  {
    //out << fmt::format("[{:d}]{:02x}", i, int(data[i]));
    out << fmt::format("[{:d}]{:08b}", i, int(data[i]));
    if (i != len-1)
      out << " ";
  }

}

} // namespace

UDrawDriver::UDrawDriver(USBDevice& usbdev, Evdev& evdev, Options const& opts) :
  m_usbdev(usbdev),
  m_evdev(evdev),
  m_opts(opts)
{
}

void
UDrawDriver::run()
{
  init_evdev();

  m_usbdev.print_info(std::cout);
  m_usbdev.detach_kernel_driver(0);
  m_usbdev.claim_interface(0);

  m_usbdev.listen(3, [this](uint8_t* data, int size){
    on_data(data, size);
  });
}

void
UDrawDriver::init_evdev()
{
  // init evdev
  if (m_opts.mode == Options::Mode::GAMEPAD)
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
  else if (m_opts.mode == Options::Mode::KEYBOARD)
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
  else if (m_opts.mode == Options::Mode::TABLET)
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
  else if (m_opts.mode == Options::Mode::TOUCHPAD)
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

void
UDrawDriver::on_data(uint8_t const* data, size_t size)
{
  UDrawDecoder decoder(data, size);

  if (m_opts.mode == Options::Mode::KEYBOARD)
  {
    m_evdev.send(EV_KEY, KEY_LEFT,  decoder.left());
    m_evdev.send(EV_KEY, KEY_RIGHT, decoder.right());
    m_evdev.send(EV_KEY, KEY_UP,    decoder.up());
    m_evdev.send(EV_KEY, KEY_DOWN,  decoder.down());

    m_evdev.send(EV_KEY, KEY_ENTER, decoder.cross());
    m_evdev.send(EV_KEY, KEY_SPACE, decoder.circle());
    m_evdev.send(EV_KEY, KEY_A, decoder.square());
    m_evdev.send(EV_KEY, KEY_Z, decoder.triangle());

    m_evdev.send(EV_KEY, KEY_ESC,  decoder.start());
    m_evdev.send(EV_KEY, KEY_TAB, decoder.select());

    m_evdev.send(EV_SYN, SYN_REPORT, 0);
  }
  else if (m_opts.mode == Options::Mode::GAMEPAD)
  {
    m_evdev.send(EV_ABS, ABS_X, -1 * decoder.left() + 1 * decoder.right());
    m_evdev.send(EV_ABS, ABS_Y, -1 * decoder.up()   + 1 * decoder.down());

    m_evdev.send(EV_KEY, BTN_A, decoder.cross());
    m_evdev.send(EV_KEY, BTN_B, decoder.circle());
    m_evdev.send(EV_KEY, BTN_X, decoder.square());
    m_evdev.send(EV_KEY, BTN_Y, decoder.triangle());

    m_evdev.send(EV_KEY, BTN_START,  decoder.start());
    m_evdev.send(EV_KEY, BTN_SELECT, decoder.select());
    m_evdev.send(EV_KEY, BTN_Z, decoder.guide());

    m_evdev.send(EV_SYN, SYN_REPORT, 0);
  }
  else if (m_opts.mode == Options::Mode::TABLET)
  {
    if (decoder.mode() == UDrawDecoder::Mode::PEN)
    {
      m_evdev.send(EV_ABS, ABS_X, decoder.x());
      m_evdev.send(EV_ABS, ABS_Y, decoder.y());
      m_evdev.send(EV_ABS, ABS_PRESSURE, decoder.pressure());
      m_evdev.send(EV_KEY, BTN_TOOL_PEN, 1);

      if (decoder.pressure() > 5)
      {
        m_evdev.send(EV_KEY, BTN_TOUCH, 1);
      }
      else
      {
        m_evdev.send(EV_KEY, BTN_TOUCH, 0);
      }

      m_evdev.send(EV_SYN, SYN_REPORT, 0);
    }
    else
    {
      m_evdev.send(EV_KEY, BTN_TOOL_PEN, 0);
      m_evdev.send(EV_SYN, SYN_REPORT, 0);
    }
  }
  else if (m_opts.mode == Options::Mode::TOUCHPAD)
  {
    m_evdev.send(EV_KEY, BTN_LEFT,  decoder.right());
    m_evdev.send(EV_KEY, BTN_RIGHT, decoder.left());

    if (decoder.mode() == UDrawDecoder::Mode::FINGER)
    {
      if (!m_finger_touching)
      {
        m_touch_pos_x = decoder.x();
        m_touch_pos_y = decoder.y();
        m_finger_touching = true;

        if (m_touch_pos_x > 1800)
        {
          m_scroll_wheel = true;
          m_wheel_distance = 0;
        }
        else
        {
          m_scroll_wheel = false;
        }
      }

      if (m_scroll_wheel)
      {
        m_wheel_distance += (decoder.y() - m_touch_pos_y) / 10;

        int rel = m_wheel_distance/10;
        if (rel != 0)
        {
          m_evdev.send(EV_REL, REL_WHEEL, -rel);

          m_wheel_distance -= rel;
          m_touch_pos_x = decoder.x();
          m_touch_pos_y = decoder.y();
        }
      }
      else
      {
        m_evdev.send(EV_REL, REL_X, decoder.x() - m_touch_pos_x);
        m_evdev.send(EV_REL, REL_Y, decoder.y() - m_touch_pos_y);

        m_touch_pos_x = decoder.x();
        m_touch_pos_y = decoder.y();
      }
    }
    else
    {
      m_finger_touching = false;
    }
    m_evdev.send(EV_SYN, SYN_REPORT, 0);
  }
  else if (m_opts.mode == Options::Mode::TEST)
  {
    std::cout << decoder << std::endl;
  }
  else if (m_opts.mode == Options::Mode::RAW)
  {
    print_raw_data(std::cout, data, size);
    std::cout << std::endl;
  }

  if (false)
  {
    m_evdev.send(EV_KEY, BTN_RIGHT,  decoder.left());
    m_evdev.send(EV_KEY, BTN_MIDDLE, decoder.up());
    m_evdev.send(EV_KEY, BTN_LEFT,   decoder.right());

    m_evdev.send(EV_REL, REL_WHEEL, decoder.triangle() ? 1 : 0);
    m_evdev.send(EV_REL, REL_WHEEL, decoder.cross() ? -1 : 0);

    m_evdev.send(EV_KEY, KEY_BACK,    decoder.circle());
    m_evdev.send(EV_KEY, KEY_FORWARD, decoder.square());

    if (false)
    {
      // FIXME: does not work as is, needs throttling

      if (decoder.mode() == UDrawDecoder::Mode::PINCH)
      {
        if (!m_pinch_touching)
        {
          m_touch_pos_x = decoder.x();
          m_touch_pos_y = decoder.y();
          m_pinch_touching = true;
        }

        m_evdev.send(EV_REL, REL_HWHEEL, decoder.x() - m_touch_pos_x);
        m_evdev.send(EV_REL, REL_WHEEL,  decoder.y() - m_touch_pos_y);

        m_touch_pos_x = decoder.x();
        m_touch_pos_y = decoder.y();
      }
      else
      {
        m_pinch_touching = false;
      }

      m_evdev.send(EV_SYN, SYN_REPORT, 0);
    }
  }
}

} // namespace udraw

/* EOF */

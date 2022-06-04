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
#include <uinpp/multi_device.hpp>

#include "options.hpp"
#include "udraw_decoder.hpp"
#include "usb_device.hpp"

#include "gamepad_driver.hpp"
#include "keyboard_driver.hpp"
#include "tablet_driver.hpp"
#include "touchpad_driver.hpp"

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

UDrawDriver::UDrawDriver(USBDevice& usbdev, uinpp::MultiDevice& evdev, Options const& opts) :
  m_usbdev(usbdev),
  m_evdev(evdev),
  m_opts(opts),
  m_driver()
{
  if (m_opts.mode == Options::Mode::KEYBOARD)
  {
    m_driver = std::make_unique<KeyboardDriver>(evdev);
  }
  else if (m_opts.mode == Options::Mode::GAMEPAD)
  {
    m_driver = std::make_unique<GamepadDriver>(evdev);
  }
  else if (m_opts.mode == Options::Mode::TABLET)
  {
    m_driver = std::make_unique<TabletDriver>(evdev);
  }
  else if (m_opts.mode == Options::Mode::TOUCHPAD)
  {
    m_driver = std::make_unique<TouchpadDriver>(evdev);
  }
}

UDrawDriver::~UDrawDriver()
{
}

void
UDrawDriver::run()
{
  m_usbdev.print_info(std::cout);
  m_usbdev.detach_kernel_driver(0);
  m_usbdev.claim_interface(0);

  if (m_driver) {
    m_driver->init();
  }

  m_usbdev.listen(3, [this](uint8_t* data, int size){
    on_data(data, size);
  });
}

void
UDrawDriver::on_data(uint8_t const* data, size_t size)
{
  if (m_driver) {
    m_driver->receive_data(data, size);
  }

  if (m_opts.mode == Options::Mode::TEST)
  {
    UDrawDecoder decoder(data, size);
    std::cout << decoder << std::endl;
  }
  else if (m_opts.mode == Options::Mode::RAW)
  {
    print_raw_data(std::cout, data, size);
    std::cout << std::endl;
  }

#if 0
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
#endif
}

} // namespace udraw

/* EOF */

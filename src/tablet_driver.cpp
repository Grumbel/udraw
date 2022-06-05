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

#include "tablet_driver.hpp"

#include <uinpp/multi_device.hpp>
#include <uinpp/event_emitter.hpp>

#include "udraw_decoder.hpp"

namespace udraw {

TabletDriver::TabletDriver(uinpp::MultiDevice& evdev) :
  m_evdev(evdev),
  m_em_x(),
  m_em_y(),
  m_em_pressure(),
  m_em_touch(),
  m_em_tool_pen(),
  m_em_wheel(),
  m_em_hwheel()
{
}

TabletDriver::~TabletDriver()
{
}

void
TabletDriver::init()
{
  uinpp::VirtualDevice* tablet = m_evdev.create_device(0, uinpp::DeviceType::GENERIC);

  //tablet->set_name("uDraw Tablet Driver (tablet)");
  tablet->set_name("THQ uDraw Game Tablet for PS3 Pen");
  tablet->set_usbid(0x3, 0x20d6, 0xcb17, 0x110);
  tablet->set_phys("uDraw tablet");
  tablet->set_prop(INPUT_PROP_POINTER);

  m_em_x = tablet->add_abs(ABS_X, 0, 1920, 1, 0, 12);
  m_em_y = tablet->add_abs(ABS_Y, 0, 1080, 1, 0, 12);
  m_em_pressure = tablet->add_abs(ABS_PRESSURE, 0, 143, 0, 0, 0);

  m_em_touch = tablet->add_key(BTN_TOUCH);
  m_em_tool_pen = tablet->add_key(BTN_TOOL_PEN);

  if (false){
    uinpp::VirtualDevice* mouse = m_evdev.create_device(0, uinpp::DeviceType::MOUSE);

    mouse->set_name("uDraw Tablet Driver (mouse)");
    mouse->set_usbid(0x3, 0x20d6, 0xcb17, 0x110);
    mouse->set_phys("uDraw mouse");
    // tablet->set_prop(INPUT_PROP_POINTER);

    m_em_wheel = mouse->add_rel(REL_WHEEL);
    m_em_hwheel = mouse->add_rel(REL_HWHEEL);
  }

  m_evdev.finish();
}

void
TabletDriver::receive_data(uint8_t const* data, size_t size)
{
  UDrawDecoder decoder(data, size);

  if (decoder.mode() == UDrawDecoder::Mode::PEN)
  {
    m_em_x->send(decoder.x());
    m_em_y->send(decoder.y());
    m_em_pressure->send(decoder.pressure());
    m_em_tool_pen->send(1);

    if (decoder.pressure() > 5)
    {
      m_em_touch->send(1);
    }
    else
    {
      m_em_touch->send(0);
    }
  }
  else
  {
    m_em_tool_pen->send(0);
  }

  m_evdev.sync();
}

} // namespace udraw

/* EOF */

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

#include "evdev.hpp"
#include "udraw_decoder.hpp"

namespace udraw {

TabletDriver::TabletDriver(Evdev& evdev) :
  m_evdev(evdev)
{
}

TabletDriver::~TabletDriver()
{
}

void
TabletDriver::init()
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

  m_evdev.finish();
}

void
TabletDriver::receive_data(uint8_t const* data, size_t size)
{
  UDrawDecoder decoder(data, size);

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

} // namespace udraw

/* EOF */

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

#include "gamepad_driver.hpp"

#include "evdev.hpp"
#include "udraw_decoder.hpp"

namespace udraw {

GamepadDriver::GamepadDriver(Evdev& evdev) :
  m_evdev(evdev)
{
}

GamepadDriver::~GamepadDriver()
{
}

void
GamepadDriver::init()
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

  m_evdev.finish();
}

void
GamepadDriver::receive_data(uint8_t const* data, size_t size)
{
  UDrawDecoder decoder(data, size);

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

} // namespace udraw

/* EOF */

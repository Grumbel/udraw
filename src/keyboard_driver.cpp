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

#include "keyboard_driver.hpp"

#include "evdev.hpp"
#include "udraw_decoder.hpp"

namespace udraw {

KeyboardDriver::KeyboardDriver(Evdev& evdev) :
  m_evdev(evdev)
{
}

KeyboardDriver::~KeyboardDriver()
{
}

void
KeyboardDriver::init()
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

  m_evdev.finish();
}

void
KeyboardDriver::receive_data(uint8_t const* data, size_t size)
{
  UDrawDecoder decoder(data, size);

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

} // namespace udraw

/* EOF */

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

#ifndef HEADER_EVDEV_HPP
#define HEADER_EVDEV_HPP

#include <sstream>

#include <linux/uinput.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>

namespace udraw {

class Evdev
{
public:
  Evdev();

  void finish();
  void send(uint16_t type, uint16_t code, int32_t value);

  void add_key(int code);
  void add_rel(int code);
  void add_abs(int code, int min, int max, int fuzz, int flat);

private:
  int m_fd;
  uinput_user_dev m_user_dev;
  bool ev_abs_bit;
  bool ev_rel_bit;
  bool ev_key_bit;
};

} // namespace udraw

#endif

/* EOF */

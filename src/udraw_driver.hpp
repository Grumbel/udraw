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

#ifndef HEADER_UDRAW_DRIVER_HPP
#define HEADER_UDRAW_DRIVER_HPP

#include "fwd.hpp"

#include <cstddef>
#include <cstdint>

namespace udraw {

class UDrawDriver
{
public:
  UDrawDriver(USBDevice& usbdev, Evdev& evdev, Options const& opts);

  void run();

private:
  void init_evdev();
  void on_data(uint8_t const* data, size_t size);

private:
  USBDevice& m_usbdev;
  Evdev& m_evdev;
  Options const& m_opts;

  int touch_pos_x = 0;
  int touch_pos_y = 0;
  bool finger_touching = false;
  bool pinch_touching = false;
  bool scroll_wheel = false;
  int wheel_distance = 0;

private:
  UDrawDriver(const UDrawDriver&) = delete;
  UDrawDriver& operator=(const UDrawDriver&) = delete;
};

} // namespace udraw

#endif

/* EOF */

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

#ifndef HEADER_TOUCHPAD_DRIVER_HPP
#define HEADER_TOUCHPAD_DRIVER_HPP

#include "driver.hpp"

#include <chrono>

#include "fwd.hpp"

namespace udraw {

class TouchpadDriver : public Driver
{
public:
  TouchpadDriver(Evdev& evdev);
  ~TouchpadDriver() override;

  void init() override;
  void receive_data(uint8_t const* data, size_t size) override;

private:
  Evdev& m_evdev;

  int m_touchdown_pos_x;
  int m_touchdown_pos_y;
  int m_touch_pos_x;
  int m_touch_pos_y;
  bool m_finger_touching;
  bool m_pinch_touching;
  bool m_scroll_wheel;
  int m_wheel_distance;
  std::chrono::steady_clock::time_point m_touch_time;

private:
  TouchpadDriver(const TouchpadDriver&) = delete;
  TouchpadDriver& operator=(const TouchpadDriver&) = delete;
};

} // namespace udraw

#endif

/* EOF */

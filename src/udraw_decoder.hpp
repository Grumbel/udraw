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

#ifndef HEADER_UDRAW_DECODER_HPP
#define HEADER_UDRAW_DECODER_HPP

#include <iosfwd>
#include <cstdint>
#include <stdexcept>

#include <fmt/format.h>

/*
  data[7]; // right
  data[8]; // left
  data[9]; // up
  data[10]; // down

  data[0] & 1 // square
  data[0] & 2 // cross
  data[0] & 8 // triangle
  data[0] & 4 // circle

  data[1] & 1 // select
  data[1] & 2 // start
  data[1] & 0x10 // guide

  // 0x00 - nothing
  // 0x80 - finger
  // 0x40 - pen
  // weird stuff when pinching (angle)
  data[11]

  data[12]; // pinch distance

  data[13]; // pressure, 0x70 neutral
*/
class UDrawDecoder
{
public:
  enum class Mode {
    UNKNOWN,
    PEN,
    FINGER,
    PINCH,
  };

public:
  UDrawDecoder(uint8_t const* data, size_t len) :
    m_data(data),
    m_len(len)
  {
    if (m_len < 19) {
      throw std::runtime_error(fmt::format("package size to small: {}", m_len));
    }
  }

  Mode mode() const
  {
    int m = (m_data[11] & 0b11000000) >> 6;

    if (m == 3) {
      return Mode::PINCH;
    } else if (m == 1) {
      return Mode::PEN;
    } else if (m == 2) {
      return Mode::FINGER;
    } else {
      return Mode::UNKNOWN;
    }
  }

  // pen: 3px resolution
  // finger: 1px resolution
  int x() const { return m_data[15] * 255 + m_data[17]; }
  int y() const { return m_data[16] * 255 + m_data[18]; }

  /** Pressure is registered all the time, even if fingers are used or
      when the pen isn't on the table, maximum value is 255,
      flips 0x71/0x72 without touch */
  int pressure() const { return m_data[13] - 0x71; }
  int max_pressure() const { return 142; }

  /** first two bits seem to be for the two fingers, precision is poor
   more data hiding in 12 */
  int orientation() const { return m_data[11] & 0b00111111; }
  int max_orientation() const { return 63; }

  int pinch_distance() const { return m_data[12]; }
  int max_pinch_distance() const { return 255; }

  /* values from -32 to 31 */
  int accel_x() const { return ((m_data[20] << 8) | m_data[19]) - 512; };
  int accel_y() const { return ((m_data[22] << 8) | m_data[21]) - 512; };
  int accel_z() const { return ((m_data[24] << 8) | m_data[23]) - 512; };

  bool up() const { return m_data[9]; }
  bool down() const { return m_data[10]; }
  bool left() const { return m_data[8]; }
  bool right() const { return m_data[7]; }

  bool square() const { return m_data[0] & 1; }
  bool cross() const { return m_data[0] & 2; }
  bool triangle() const { return m_data[0] & 8; }
  bool circle() const { return m_data[0] & 4; }

  bool start() const { return m_data[1] & 1; }
  bool select() const { return m_data[1] & 2; }
  bool guide() const { return m_data[1] & 0x10; }

private:
  uint8_t const* m_data;
  size_t m_len;
};

std::ostream& operator<<(std::ostream& os, UDrawDecoder const& decoder);

#endif

/* EOF */

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

namespace udraw {

/*
  // Device will sleep after 5 minutes
struct Message
{
  // data[0]
  unsigned zeros0: 4;
  unsigned triangle: 1;
  unsigned circle: 1;
  unsigned cross: 1;
  unsigned square: 1;

  // data[1]
  unsigned zeroes1: 3;
  unsigned guide: 1;
  unsigned zeroes2: 2;
  unsigned select: 1;
  unsigned start: 1;

  // data[2]
  unsigned zeroes3: 4;
  unsigned hat: 4;

  // data[3]
  unsigned const1: 8; // always: 1000 000
  // data[4]
  unsigned const2: 8; // always: 1000 000
  // data[5]
  unsigned const3: 8; // always: 1000 000
  // data[6]
  unsigned const4: 8; // always: 1000 000

  // data[7]
  unsigned right: 8; // 0 or 255

  // data[8]
  unsigned left: 8; // 0 or 255

  // data[9]
  unsigned up: 8; // 0 or 255

  // data[10]
  unsigned down: 8; // 0 or 255

  // data[11]
  unsigned mode: 2; // 10: finger, 01: pen, 11: pinch
  unsigned pinch_orientation: 8;

  // data[12]
  unsigned pinch_distance: 8;

  // data[13]
  unsigned pressure: 8; // 142-255, rest unused

  // data[14]
  unsigned cross: 8; // 0 or 255

  // data[15]
  unsigned x_hi : 8; // ~ 0 - 1920

  // data[16]
  unsigned y_hi : 8;

  // data[17]
  unsigned x_lo : 8; // ~ 0 - 1080

  // data[18]
  unsigned y_lo : 8;

  // data[19]
  unsigned accel_x_lo : 8; // -512, low resolution

  // data[20]
  unsigned accel_x_hi : 8;

  // data[21]
  unsigned accel_y_lo : 8;

  // data[22]
  unsigned accel_y_hi : 8;

  // data[23]
  unsigned accel_z_lo : 8;

  // data[24]
  unsigned accel_z_hi : 8;

  // data[25]
  unsigned zeroes : 8;

  // data[26]
  unsigned const2 : 8; // always: 0000 0010
};
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

} // namespace udraw

#endif

/* EOF */

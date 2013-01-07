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
private:
  uint8_t* m_data;
  int m_len;

public:
  enum { 
    PEN_MODE    = 0x40, 
    FINGER_MODE = 0x80,
    PINCH_MODE
  };

  UDrawDecoder(uint8_t* data, int len) :
    m_data(data), m_len(len)
  {
  }

  int get_x() const
  {
    // pen: 3px resolution
    // finger: 1px resolution
    return m_data[15] * 255 + m_data[17]; 
  }

  int get_y() const
  {
    return m_data[16] * 255 + m_data[18];
  }

  int get_pressure() const
  {
    return m_data[13] - 0x70;
  }

  int get_orientation() const
  {
    return m_data[11] - 0xc0;
  }

  int get_mode() const
  {
    //std::cout << int(m_data[11]) << std::endl;
    if (0xc0 <= m_data[11] && m_data[11] <= 253)
      return PINCH_MODE;
    else
      return m_data[11];
  }

  bool get_up() const
  {
    return m_data[9];
  }

  bool get_down() const
  {
    return m_data[10];
  }

  bool get_left() const
  {
    return m_data[8];
  }

  bool get_right() const
  {
    return m_data[7];
  }

  bool get_square() const { return m_data[0] & 1; }
  bool get_cross() const { return m_data[0] & 2; }
  bool get_triangle() const { return m_data[0] & 8; }
  bool get_circle() const { return m_data[0] & 4; }

  bool get_start() const { return m_data[1] & 1; }
  bool get_select() const { return m_data[1] & 2; }
};


#endif

/* EOF */

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

#ifndef HEADER_UDRAW_OPTIONS_HPP
#define HEADER_UDRAW_OPTIONS_HPP

namespace udraw {

struct Options
{
  enum class Mode {
    TEST,
    RAW,
    GAMEPAD,
    KEYBOARD,
    TOUCHPAD,
    TABLET,
    ACCELEROMETER
  };

  bool verbose = false;
  Mode mode = Mode::TEST;
};

} // namespace udraw

#endif

/* EOF */

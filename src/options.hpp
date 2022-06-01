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

struct Options
{
  bool gamepad_mode;
  bool keyboard_mode;
  bool touchpad_mode;
  bool tablet_mode;
  bool accelerometer_mode;

  Options() :
    gamepad_mode(false),
    keyboard_mode(false),
    touchpad_mode(false),
    tablet_mode(false),
    accelerometer_mode(false)
  {}
};

#endif

/* EOF */

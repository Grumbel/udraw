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

#ifndef HEADER_DRIVER_HPP
#define HEADER_DRIVER_HPP

#include <cstdint>
#include <cstddef>

namespace udraw {

class Driver
{
public:
  Driver() {}
  virtual ~Driver() {}

  virtual void init() = 0;
  virtual void receive_data(uint8_t const* data, size_t size) = 0;

private:
  Driver(const Driver&) = delete;
  Driver& operator=(const Driver&) = delete;
};

} // namespace udraw

#endif

/* EOF */

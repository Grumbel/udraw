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

#include "udraw_decoder.hpp"

#include <ostream>

std::ostream& operator<<(std::ostream& os, UDrawDecoder const& decoder)
{
  os << fmt::format(
    "m:{} x:{:4} y:{:4} - p:{:3}% o:{:3}° d:{:3d}% - "
    "↑:{:d} →:{:d} ↓:{:d} ←:{:d} - "
    "△:{:d} ○:{:d} ×:{:d} □:{:d} - start:{:d} select:{:d} guide:{:d} "
    "accel:{:3d} {:3d} {:3d}",
    static_cast<int>(decoder.mode()),
    decoder.x(), decoder.y(),
    100 * decoder.pressure() / decoder.max_pressure(),
    360 * decoder.orientation() / decoder.max_orientation(),
    100 * decoder.pinch_distance() / decoder.max_pinch_distance(),
    decoder.up(), decoder.right(),
    decoder.down(), decoder.left(),

    decoder.triangle(), decoder.circle(),
    decoder.cross(), decoder.square(),

    decoder.start(), decoder.select(),
    decoder.guide(),

    decoder.accel_x(), decoder.accel_y(), decoder.accel_z()
    );
  return os;
}

/* EOF */

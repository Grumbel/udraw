# Linux userspace driver for the uDraw graphic tablet
# Copyright (C) 2015 Ingo Ruhnke <grumbel@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.0)
project(udraw)

set(WARNINGS 0 CACHE BOOL "Switch on extra warnings")

add_compile_options(-std=c++11)

if(WARNINGS)
  set(WARNINGS_CXX_FLAGS
    -pedantic -Wall -Wextra -Wno-c++0x-compat -Wnon-virtual-dtor -Weffc++
    -Wconversion -Werror -Wshadow -Wcast-qual -Winit-self -Wno-unused-parameter)
endif()

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(USB REQUIRED libusb)

add_executable(udraw-driver udraw-driver.cpp)
target_compile_options(udraw-driver PRIVATE ${WARNINGS_CXX_FLAGS})
target_link_libraries(udraw-driver
  ${USB_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT})

# EOF #


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

cmake_minimum_required(VERSION 3.6)
project(udraw)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(WARNINGS "Switch on extra warnings" OFF)
option(WERROR "Turn warnings into errors" OFF)

if(WARNINGS)
  set(WARNINGS_CXX_FLAGS
    -pedantic -Wall -Wextra -Wno-c++0x-compat -Wnon-virtual-dtor -Weffc++
    -Wconversion -Werror -Wshadow -Wcast-qual -Winit-self -Wno-unused-parameter
    -Wno-stringop-overread # produces bogus warnings
    )
endif()

if(WERROR)
  set(WARNINGS_CXX_FLAGS ${WARNINGS_CXX_FLAGS}
    -Werror)
endif()

find_package(fmt REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(LIBUSB REQUIRED libusb IMPORTED_TARGET)

add_executable(udraw-driver src/udraw-driver.cpp)
target_compile_options(udraw-driver PRIVATE ${WARNINGS_CXX_FLAGS})
target_link_libraries(udraw-driver
  fmt::fmt
  PkgConfig::LIBUSB
  Threads::Threads)

install(TARGETS udraw-driver
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# EOF #


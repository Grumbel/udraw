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

set(TINYCMMC_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/external/tinycmmc/modules/")
find_package(tinycmmc CONFIG)
message(STATUS "tinycmmc module path: ${TINYCMMC_MODULE_PATH}")
list(APPEND CMAKE_MODULE_PATH ${TINYCMMC_MODULE_PATH})

include(GNUInstallDirs)
include(MaximumWarnings)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND WARNINGS_CXX_FLAGS
  -Wno-stringop-overread # produces bogus warnings
  )

find_package(fmt REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(LIBUSB REQUIRED libusb-1.0 IMPORTED_TARGET)

function(build_dependencies)
  find_package(logmich QUIET)
  if(NOT TARGET logmich::logmich)
    add_subdirectory(external/logmich/ EXCLUDE_FROM_ALL)
  endif()

  find_package(uinpp)
  if(NOT TARGET uinpp::uinpp)
    add_subdirectory(external/uinpp/)
  endif()
endfunction()
build_dependencies()

file(GLOB UDRAW_SOURCES_CXX src/*.cpp)
add_executable(udraw-driver ${UDRAW_SOURCES_CXX})
target_compile_options(udraw-driver PRIVATE ${WARNINGS_CXX_FLAGS})
target_link_libraries(udraw-driver
  fmt::fmt
  logmich::logmich
  uinpp::uinpp
  PkgConfig::LIBUSB
  Threads::Threads)

install(TARGETS udraw-driver
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# EOF #


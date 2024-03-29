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

#include <memory>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <libusb.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include <fmt/format.h>
#include <logmich/log.hpp>
#include <uinpp/multi_device.hpp>

#include "options.hpp"
#include "udraw_decoder.hpp"
#include "udraw_driver.hpp"
#include "usb_device.hpp"

namespace udraw {

uint16_t const UDRAW_VENDOR_ID = 0x20d6;
uint16_t const UDRAW_PRODUCT_ID = 0xcb17;

class USBDevice;

bool global_interrupt = false;

void print_help(const char* argv0)
{
  std::cout << "Usage: " << argv0 << "[OPTION]...\n"
            << "Basic driver for the PS3 uDraw graphic tablet\n"
            << "\n"
            << "Options:\n"
            << "  -h, --help     display this help\n"
            << "  -v, --verbose  be more verbose\n"
            << "  -v, --version  print version number\n"
            << "\n"
            << "Modes:\n"
            << "  --test         pretty print data (default)\n"
            << "  --raw          print raw data\n"
            << "  --touchpad     use the device as touchpad\n"
            << "  --tablet       use the device as graphic tablet\n"
            << "  --gamepad      use the device as gamepad\n"
            << "  --keyboard     use the device as keyboard\n"
            << std::endl;
}

Options parse_args(int argc, char** argv)
{
  Options opts;

  for(int i = 1; i < argc; ++i)
  {
    if (strcmp("--test", argv[i]) == 0) {
      opts.mode = Options::Mode::TEST;
    } else if (strcmp("--raw", argv[i]) == 0) {
      opts.mode = Options::Mode::RAW;
    } else if (strcmp("--gamepad", argv[i]) == 0) {
      opts.mode = Options::Mode::GAMEPAD;
    } else if (strcmp("--keyboard", argv[i]) == 0) {
      opts.mode = Options::Mode::KEYBOARD;
    } else if (strcmp("--tablet", argv[i]) == 0) {
      opts.mode = Options::Mode::TABLET;
    } else if (strcmp("--touchpad", argv[i]) == 0) {
      opts.mode = Options::Mode::TOUCHPAD;
    } else if (strcmp("--verbose", argv[i]) == 0 ||
               strcmp("-v", argv[i]) == 0) {
      opts.verbose = true;
    } else if (strcmp("--version", argv[i]) == 0 ||
               strcmp("-V", argv[i]) == 0)
    {
      std::cout << PROJECT_NAME << " " << PROJECT_VERSION << std::endl;
      exit(EXIT_SUCCESS);
    } else if (strcmp("--help", argv[i]) == 0 ||
               strcmp("-h", argv[i]) == 0)
    {
      print_help(argv[0]);
      exit(EXIT_SUCCESS);
    } else {
      throw std::runtime_error(fmt::format("unknown option: {}", argv[i]));
    }
  }

  return opts;
}

void run(int argc, char** argv)
{
  Options const opts = parse_args(argc, argv);

  if (opts.verbose) {
    logmich::g_logger.set_log_level(logmich::LogLevel::DEBUG);
  }

  libusb_context* usb_ctx;
  int err = libusb_init(&usb_ctx);
  if (err != LIBUSB_SUCCESS) {
    throw std::runtime_error(libusb_strerror(err));
  }

  {
    USBDevice usbdev(usb_ctx, UDRAW_VENDOR_ID, UDRAW_PRODUCT_ID);
    //uinpp::MultiDevice evdev;
    uinpp::MultiDevice evdev;
    UDrawDriver driver(usbdev, evdev, opts);
    driver.run();
  }

  libusb_exit(usb_ctx);
}

} // namespace udraw

int main(int argc, char** argv) try
{
  udraw::run(argc, argv);
  return EXIT_SUCCESS;
} catch (std::exception const& err) {
  log_error("exception: {}", err.what());
  return EXIT_FAILURE;
}

/* EOF */

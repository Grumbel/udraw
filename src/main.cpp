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
#include <usb.h>
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

#include "evdev.hpp"
#include "options.hpp"
#include "udraw_decoder.hpp"
#include "udraw_driver.hpp"
#include "usb_device.hpp"

namespace udraw {

class USBDevice;

bool global_interrupt = false;

namespace {

struct usb_device*
find_usb_device(uint16_t idVendor, uint16_t idProduct)
{
  struct usb_bus* busses = usb_get_busses();

  for (struct usb_bus* bus = busses; bus; bus = bus->next)
  {
    for (struct usb_device* dev = bus->devices; dev; dev = dev->next)
    {
      if (dev->descriptor.idVendor  == idVendor &&
          dev->descriptor.idProduct == idProduct)
      {
        return dev;
      }
    }
  }
  return nullptr;
}

} // namespace

void print_help(const char* argv0)
{
  std::cout << "Usage: " << argv0 << "[OPTION]...\n"
            << "Basic driver for the PS3 uDraw graphic tablet\n"
            << "\n"
            << "Options:\n"
            << "  -h, --help  display this help\n"
            << "  --touchpad  use the device as touchpad\n"
            << "  --tablet    use the device as graphic tablet\n"
            << "  --gamepad   use the device as gamepad\n"
            << "  --keyboard  use the device as keyboard\n"
            << "  --accelerometer  use the accelerometer\n"
            << std::endl;
}

Options parse_args(int argc, char** argv)
{
  Options opts;

  for(int i = 1; i < argc; ++i)
  {
    if (strcmp("--gamepad", argv[i]) == 0)
    {
      opts.gamepad_mode = true;
    }
    else if (strcmp("--keyboard", argv[i]) == 0)
    {
      opts.keyboard_mode = true;
    }
    else if (strcmp("--tablet", argv[i]) == 0)
    {
      opts.tablet_mode = true;
    }
    else if (strcmp("--touchpad", argv[i]) == 0)
    {
      opts.touchpad_mode = true;
    }
    else if (strcmp("--accelerometer", argv[i]) == 0)
    {
      opts.accelerometer_mode = true;
    }
    else if (strcmp("--help", argv[i]) == 0 ||
             strcmp("-h", argv[i]) == 0)
    {
      print_help(argv[0]);
      exit(EXIT_SUCCESS);
    }
    else
    {
      std::cerr << "unknown option: " << argv[i] << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  return opts;
}

void run(int argc, char** argv)
{
  Options opts = parse_args(argc, argv);

  usb_init();
  usb_find_busses();
  usb_find_devices();

  struct usb_device* dev = find_usb_device(0x20d6, 0xcb17);
  if (!dev) {
    throw std::runtime_error("error: no udraw tablet found (20d6:cb17)");
  }

  USBDevice usbdev(dev);
  Evdev evdev;
  UDrawDriver driver(usbdev, evdev, opts);
  driver.run();
}

} // namespace udraw

int main(int argc, char** argv) try
{
  udraw::run(argc, argv);
  return EXIT_SUCCESS;
} catch (std::exception const& err) {
  std::cerr << "error: " << err.what() << std::endl;
  return EXIT_FAILURE;
}

/* EOF */

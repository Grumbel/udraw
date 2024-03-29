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

#ifndef HEADER_USB_DEVICE_HPP
#define HEADER_USB_DEVICE_HPP

#include <functional>
#include <iosfwd>

#include <libusb.h>

namespace udraw {

class USBDevice
{
public:
  USBDevice(libusb_context* ctx, uint16_t vendor_id, uint16_t product_id);
  ~USBDevice();

  void reset();
  void detach_kernel_driver(int iface);
  void claim_interface(int iface);
  void release_interface(int iface);
  void set_configuration(int configuration);
  size_t read(int endpoint, uint8_t* data, int len);
  size_t write(int endpoint, uint8_t* data, int len);

  /* uint8_t  requesttype
     uint8_t  request
     uint16_t value;
     uint16_t index;
     uint16_t length;
  */
  size_t ctrl_msg(int requesttype, int request,
                  int value, int index,
                  uint8_t* data, int size);
  void print_info(std::ostream& out);
  void listen(int endpoint, std::function<void (uint8_t* data, size_t)> callback);

private:
  libusb_context* m_ctx;
  libusb_device_handle* m_handle;

private:
  USBDevice(const USBDevice&) = delete;
  USBDevice& operator=(const USBDevice&) = delete;
};

} // namespace udraw

#endif

/* EOF */

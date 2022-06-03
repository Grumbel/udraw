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

#include "usb_device.hpp"

#include <iostream>
#include <sstream>

#include <fmt/format.h>

namespace udraw {

USBDevice::USBDevice(libusb_context* ctx, libusb_device_handle* handle) :
  m_ctx(ctx),
  m_handle(handle)
{
}

USBDevice::~USBDevice()
{
  libusb_close(m_handle);
}

void
USBDevice::reset()
{
  int err = libusb_reset_device(m_handle);
  if (err != LIBUSB_SUCCESS) {
    std::cerr << "Failure to reset:" << libusb_strerror(err) << std::endl;
  }
}

void
USBDevice::detach_kernel_driver(int iface)
{
  int err = libusb_detach_kernel_driver(m_handle, iface);
  if (err != LIBUSB_SUCCESS) {
    std::cerr << "couldn't detach interface " << iface << std::endl;
  }
}

void
USBDevice::claim_interface(int iface)
{
  int err = libusb_claim_interface(m_handle, iface);
  if (err != LIBUSB_SUCCESS) {
    std::ostringstream str;
    str << "Couldn't claim interface " << iface;
    throw std::runtime_error(str.str());
  }
}

void
USBDevice::release_interface(int iface)
{
  int err = libusb_release_interface(m_handle, iface);
  if (err != LIBUSB_SUCCESS) {
    std::ostringstream str;
    str << "Couldn't release interface " << iface;
    throw std::runtime_error(str.str());
  }
}

void
USBDevice::set_configuration(int configuration)
{
  int err = libusb_set_configuration(m_handle, configuration);
  if (err != LIBUSB_SUCCESS) {
    std::ostringstream str;
    str << "Couldn't set configuration " << configuration;
    throw std::runtime_error(str.str());
  }
}

int
USBDevice::read(int endpoint, uint8_t* data, int len)
{
  int transfered = 0;
  int err = libusb_interrupt_transfer(m_handle,
                                      static_cast<unsigned char>(endpoint) | LIBUSB_ENDPOINT_IN,
                                      data,len,
                                      &transfered,
                                      0);

  if (transfered != len) {
    std::cerr << fmt::format("USBDevice::read(): short read: expected {} got {}", len, transfered);
  }

  return err;
}

int
USBDevice::write(int endpoint, uint8_t* data, int len)
{
  return libusb_interrupt_transfer(m_handle,
                                   static_cast<unsigned char>(endpoint) | LIBUSB_ENDPOINT_OUT,
                                   data,len,
                                   nullptr, /* bytes actually transfered */
                                   0);
}

/* uint8_t  requesttype
   uint8_t  request
   uint16_t value;
   uint16_t index;
   uint16_t length;
*/
int
USBDevice::ctrl_msg(int requesttype, int request,
                    int value, int index,
                    uint8_t* data, int size)
{
  return libusb_control_transfer(m_handle,
                                 static_cast<uint8_t>(requesttype),
                                 static_cast<uint8_t>(request),
                                 static_cast<uint16_t>(value),
                                 static_cast<uint16_t>(index),
                                 data, static_cast<uint16_t>(size),
                                 0 /* timeout */);
}

void
USBDevice::print_info()
{
#if 0
  for(int i = 0; i < dev->descriptor.bNumConfigurations; ++i)
  {
    std::cout << "Configuration: " << i << std::endl;
    for(int j = 0; j < dev->config[i].bNumInterfaces; ++j)
    {
      std::cout << "  Interface " << j << ":" << std::endl;
      for(int k = 0; k < dev->config[i].interface[j].num_altsetting; ++k)
      {
        for(int l = 0; l < dev->config[i].interface[j].altsetting[k].bNumEndpoints; ++l)
        {
          std::cout << "    Endpoint: "
                    << int(dev->config[i].interface[j].altsetting[k].endpoint[l].bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK)
                    << ((dev->config[i].interface[j].altsetting[k].endpoint[l].bEndpointAddress & USB_ENDPOINT_DIR_MASK) ? " (IN)" : " (OUT)")
                    << std::endl;
        }
      }
    }
  }
#endif
}

void
USBDevice::listen(int endpoint, std::function<void (uint8_t* data, int)> callback)
{
  try
  {
    bool this_quit = false;
    std::cout << "Reading from endpoint " << endpoint << std::endl;;
    while(!this_quit)
    {
      uint8_t data[8192];
      int ret = read(endpoint, data, sizeof(data));
      if (ret < 0)
      {
        std::cerr << "USBError: " << libusb_error_name(ret) << "\n" << libusb_strerror(ret) << std::endl;
        std::cerr << "Shutting down" << std::endl;
        this_quit = true;
      }
      else
      {
        callback(data, ret);
      }
    }

  }
  catch(std::exception& err)
  {
    std::cout << "Error: " << err.what() << std::endl;
  }
}

} // namespace udraw

/* EOF */

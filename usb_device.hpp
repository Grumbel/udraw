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

class USBDevice
{
private:
  struct usb_device*     dev;
  struct usb_dev_handle* handle;

public: 
  USBDevice(struct usb_device* dev_) :
  dev(dev_)
  {
    handle = usb_open(dev);
    if (!handle)
    {
      throw std::runtime_error("Error opening usb device");
    }
  }

  ~USBDevice()
  {
    usb_close(handle);
  }

  void clear_halt(int ep)
  {
    if (usb_clear_halt(handle, ep) != 0)
    {
      std::cout << "Failure to reset_ep: " << ep << std::endl;
    }    
  }

  void reset()
  {
    if (usb_reset(handle) != 0)
    {
      std::cout << "Failure to reset" << std::endl;
    }
  }

  void detach_kernel_driver(int iface)
  {
    if (usb_detach_kernel_driver_np(handle, iface) < 0)
    {
      std::cerr << "couldn't detach interface " << iface << std::endl;
    }
  }

  void claim_interface(int iface)
  {
    if (usb_claim_interface(handle, iface) != 0)
    {
      std::ostringstream str;
      str << "Couldn't claim interface " << iface;
      throw std::runtime_error(str.str());
    }
  }


  void release_interface(int iface)
  {
    if (usb_release_interface(handle, iface) != 0)
    {
      std::ostringstream str;
      str << "Couldn't release interface " << iface;
      throw std::runtime_error(str.str());
    }
  }

  void set_configuration(int configuration)
  {
    if (usb_set_configuration(handle, configuration) != 0)
    {
      std::ostringstream str;
      str << "Couldn't set configuration " << configuration;
      throw std::runtime_error(str.str());
    }
  }

  void set_altinterface(int interface)
  {
    if (usb_set_altinterface(handle, interface) != 0)
    {
      std::ostringstream str;
      str << "Couldn't set alternative interface " << interface;
      throw std::runtime_error(str.str());
    }
  }

  int read(int endpoint, uint8_t* data, int len)
  {
    return usb_interrupt_read(handle, endpoint, (char*)data, len, 0);
  }

  int write(int endpoint, uint8_t* data, int len)
  {
    return usb_interrupt_write(handle, endpoint, (char*)data, len, 0);
  }

  /* uint8_t  requesttype
     uint8_t  request
     uint16_t value;
     uint16_t index;
     uint16_t length;
  */
  int ctrl_msg(int requesttype, int request, 
               int value, int index,
               uint8_t* data, int size) 
  {
    return usb_control_msg(handle, 
                           requesttype,  request, 
                           value,  index,
                           (char*)data, size, 
                           0 /* timeout */);
  }
  
  void print_info()
  {
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
  }

  void listen(int endpoint, std::function<void (uint8_t* data, int)> callback)
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
          std::cerr << "USBError: " << ret << "\n" << usb_strerror() << std::endl;
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
};


#endif

/* EOF */

#include <libusb-1.0/libusb.h>
#include <boost/format.hpp>
#include <memory>
#include <stdio.h>
#include <string.h>
#include <boost/format.hpp>
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

class USBDevice;

void print_raw_data(std::ostream& out, uint8_t* data, int len);
bool global_interrupt = false;

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
  return 0;
}

void print_raw_data(std::ostream& out, uint8_t* data, int len)
{
  std::cout << "[" << len 
            << "] ";
      
  for(int i = 0; i < len; ++i)
  {
    std::cout << boost::format("[%d]%02x") % i % int(data[i]);
    if (i != len-1)
      std::cout << " ";
  }

}

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

class Evdev
{
private:
  int m_fd;
  uinput_user_dev m_user_dev;
  
public:
  Evdev()
  {
    m_fd = open("/dev/uinput", O_RDWR | O_NDELAY);
    if (m_fd < 0)
    {
      throw std::runtime_error(strerror(errno));
    }

    ioctl(m_fd, UI_SET_EVBIT, EV_ABS);
    ioctl(m_fd, UI_SET_EVBIT, EV_KEY);

    add_abs(ABS_X, 0, 1913, 0, 0);
    add_abs(ABS_Y, 0, 1076, 0, 0);
    add_abs(ABS_PRESSURE, 0, 143, 0, 0);

    add_key(BTN_LEFT);
    add_key(BTN_TOOL_PEN);
    add_key(BTN_TOOL_FINGER);
      
    strncpy(m_user_dev.name, "uDraw Game Tablet for PS3", UINPUT_MAX_NAME_SIZE);
    m_user_dev.id.version = 0;
    m_user_dev.id.bustype = 0;
    m_user_dev.id.vendor  = 0;
    m_user_dev.id.product = 0;

    write(m_fd, &m_user_dev, sizeof(m_user_dev));

    if (ioctl(m_fd, UI_DEV_CREATE))
    {
      throw std::runtime_error(strerror(errno));
    }
  }

  void send(uint16_t type, uint16_t code, int32_t value)
  {
    struct input_event ev;      
    memset(&ev, 0, sizeof(ev));

    gettimeofday(&ev.time, NULL);

    ev.type  = type;
    ev.code  = code;

    if (ev.type == EV_KEY)
    {
      ev.value = (value>0) ? 1 : 0;
    }
    else
    {
      ev.value = value;
    }

    if (write(m_fd, &ev, sizeof(ev)) < 0)
      throw std::runtime_error(std::string("uinput:send_button: ") + strerror(errno)); 

    // sync: send(EV_SYN, SYN_REPORT, 0);
  }

private:
  void add_key(int code)
  {
    ioctl(m_fd, UI_SET_KEYBIT, code);
  }

  void add_abs(int code, int min, int max, int fuzz, int flat)
  {
    m_user_dev.absmin[code] = min;
    m_user_dev.absmax[code] = max; 
    m_user_dev.absfuzz[code] = fuzz;
    m_user_dev.absflat[code] = flat;

    ioctl(m_fd, UI_SET_ABSBIT, code);
  }
};

/*
  data[7]; // right
  data[8]; // left
  data[9]; // up
  data[10]; // down

  data[0] & 1 // square
  data[0] & 2 // cross
  data[0] & 8 // triangle
  data[0] & 4 // circle

  data[1] & 1 // select
  data[1] & 2 // start
  data[1] & 0x10 // guide

  // 0x00 - nothing
  // 0x80 - finger
  // 0x40 - pen
  // weird stuff when pinching (angle)
  data[11]             

  data[12]; // pinch distance

  data[13]; // pressure, 0x70 neutral
*/
class UDrawDecoder
{
private:
  uint8_t* m_data;
  int m_len;

public:
  enum { 
    PEN_MODE    = 0x40, 
    FINGER_MODE = 0x80,
    PINCH_MODE
  };

  UDrawDecoder(uint8_t* data, int len) :
    m_data(data), m_len(len)
  {
  }

  int get_x() const
  {
    // pen: 3px resolution
    // finger: 1px resolution
    return m_data[15] * 255 + m_data[17]; 
  }

  int get_y() const
  {
    return m_data[16] * 255 + m_data[18];
  }

  int get_pressure() const
  {
    return m_data[13] - 0x70;
  }

  int get_orientation() const
  {
    return m_data[11] - 0xc0;
  }

  int get_mode() const
  {
    return m_data[11];
  }
};

int main(int argc, char** argv)
{
  usb_init();
  usb_find_busses();
  usb_find_devices();

  struct usb_device* dev = find_usb_device(0x20d6, 0xcb17);

  int acc_x_min = 0;
  int acc_y_min = 0;
  int acc_z_min = 0;

  int acc_x_max = 0;
  int acc_y_max = 0;
  int acc_z_max = 0;
            
  if (dev)
  {
    std::unique_ptr<USBDevice> usbdev(new USBDevice(dev));

    usbdev->print_info();
    usbdev->detach_kernel_driver(0);
    usbdev->claim_interface(0);

    Evdev evdev;

    usbdev->listen
      (3,
       [&](uint8_t* data, int size)
       {
         if (false)
         {
           print_raw_data(std::cout, data, size);
           std::cout << std::endl;
         }
         else if (true)
         {
           UDrawDecoder decoder(data, size);
           if (decoder.get_mode() == UDrawDecoder::PEN_MODE)
           {
             evdev.send(EV_ABS, ABS_X, decoder.get_x());
             evdev.send(EV_ABS, ABS_Y, decoder.get_y());
             evdev.send(EV_ABS, ABS_PRESSURE, decoder.get_pressure());
             evdev.send(EV_KEY, BTN_TOOL_PEN, 1);
             
             if (decoder.get_pressure() > 0)
             {
               evdev.send(EV_KEY, BTN_LEFT, 1);
             }
             else
             {
               evdev.send(EV_KEY, BTN_LEFT, 0);
             }

             evdev.send(EV_SYN, SYN_REPORT, 0);
           }
           else
           {
             evdev.send(EV_KEY, BTN_TOOL_PEN, 0);
             evdev.send(EV_SYN, SYN_REPORT, 0);
           }

           #if 0
           else if (decoder.get_mode() == UDrawDecoder::FINGER_MODE)
           {
             /*
             evdev.send(EV_ABS, ABS_X, decoder.get_x());
             evdev.send(EV_ABS, ABS_Y, decoder.get_y());
             evdev.send(EV_ABS, ABS_PRESSURE, 0);
             evdev.send(EV_SYN, SYN_REPORT, 0);
             */
           }
           else if (decoder.get_mode() == UDrawDecoder::PINCH_MODE)
           {
             
           }
#endif
         }
         else
         {
           if (size != 27)
           {
             std::cerr << "unknown read size: " << size << std::endl;
           }
           else
           {
             //data[0];
             int x = data[15] * 255 + data[17]; // x - pen: 3px resolution
             int y = data[16] * 255 + data[18]; // y - finger: 1px resolution

             if (data[11] == 0x00)
             {
               std::cout << "nothing";
             }
             else if (data[11] == 0x80)
             {
               std::cout << boost::format("finger: x: %4d y: %4d") % x % y;
             }
             else if (data[11] == 0x40)
             {
               std::cout << boost::format("pen: x: %4d y: %4d  - pressure: %3d") % x % y % (int(data[13]) - 0x70);
             }
             else
             {
               std::cout << boost::format("pinch: x: %4d y: %4d  distance: %4d  orientation: %02x") % x % y % int(data[12]) % (int(data[11]) - 0xc0);
             }

             int acc_x = ((data[19] + data[20] * 255) - 512);
             int acc_y = ((data[21] + data[22] * 255) - 512);
             int acc_z = ((data[23] + data[24] * 255) - 512);

             acc_x_min = std::min(acc_x, acc_x_min);
             acc_y_min = std::min(acc_y, acc_y_min);
             acc_z_min = std::min(acc_z, acc_z_min);

             acc_x_max = std::max(acc_x, acc_x_max);
             acc_y_max = std::max(acc_y, acc_y_max);
             acc_z_max = std::max(acc_z, acc_z_max);

             // acc_min -31  -33  -33 
             // acc_max  31   28   29
             // ~22 == 1g

             // accelerometer
             std::cout << boost::format("%4d %4d %4d - %4d %4d %4d - %4d %4d %4d") %
               acc_x % acc_y % acc_z %
               acc_x_min % acc_y_min % acc_z_min %
               acc_x_max % acc_y_max % acc_z_max;

             
             std::cout << std::endl;
           }
         }
       });
  }

  return 0;
}

/* EOF */

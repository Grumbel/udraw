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

#ifndef HEADER_EVDEV_HPP
#define HEADER_EVDEV_HPP

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
    ioctl(m_fd, UI_SET_EVBIT, EV_REL);

    //add_abs(ABS_X, 0, 1913, 0, 0);
    //add_abs(ABS_Y, 0, 1076, 0, 0);
    add_rel(REL_X);
    add_rel(REL_Y);
    add_rel(REL_WHEEL);
    add_rel(REL_HWHEEL);
    add_abs(ABS_PRESSURE, 0, 143, 0, 0);

    add_key(BTN_LEFT);
    add_key(BTN_RIGHT);
    add_key(BTN_MIDDLE);

    add_key(KEY_FORWARD);
    add_key(KEY_BACK);

    //add_key(BTN_TOOL_PEN);
    //add_key(BTN_TOOL_FINGER);
      
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

  void add_rel(int code)
  {
    ioctl(m_fd, UI_SET_RELBIT, code);
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

#endif

/* EOF */

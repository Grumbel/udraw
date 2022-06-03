//  Linux driver for the uDraw graphic tablet
//  Copyright (C) 2012-2022 Ingo Ruhnke <grumbel@gmail.com>
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

#include "touchpad_driver.hpp"

#include <cmath>

#include <logmich/log.hpp>

#include "udraw_decoder.hpp"
#include "evdev.hpp"

namespace udraw {

TouchpadDriver::TouchpadDriver(Evdev& evdev) :
  m_evdev(evdev),
  m_touchdown_pos_x(0),
  m_touchdown_pos_y(0),
  m_touch_pos_x(0),
  m_touch_pos_y(0),
  m_finger_touching(false),
  m_pinch_touching(false),
  m_scroll_wheel(false),
  m_wheel_distance(0),
  m_touch_time()
{
}

TouchpadDriver::~TouchpadDriver()
{
}

void
TouchpadDriver::init()
{
  m_evdev.add_key(BTN_LEFT);
  m_evdev.add_key(BTN_RIGHT);
  m_evdev.add_key(BTN_MIDDLE);

  /*
    add_key(KEY_FORWARD);
    add_key(KEY_BACK);
  */

  m_evdev.add_rel(REL_WHEEL);
  m_evdev.add_rel(REL_HWHEEL);

  m_evdev.add_rel(REL_X);
  m_evdev.add_rel(REL_Y);

  m_evdev.finish();
}

void
TouchpadDriver::receive_data(uint8_t const* data, size_t size)
{
  UDrawDecoder decoder(data, size);

  m_evdev.send(EV_KEY, BTN_LEFT,  decoder.right() || decoder.square());
  m_evdev.send(EV_KEY, BTN_RIGHT, decoder.left() || decoder.circle());
  m_evdev.send(EV_KEY, BTN_MIDDLE, decoder.up() || decoder.triangle());

  if (decoder.mode() == UDrawDecoder::Mode::FINGER)
  {
    if (!m_finger_touching)
    {
      m_finger_touching = true;
      m_touchdown_pos_x = decoder.x();
      m_touchdown_pos_y = decoder.y();

      m_touch_pos_x = decoder.x();
      m_touch_pos_y = decoder.y();

      m_touch_time = std::chrono::steady_clock::now();

      if (m_touch_pos_x < 120 || m_touch_pos_x > 1800)
      {
        m_scroll_wheel = true;
        m_wheel_distance = 0;
      }
      else
      {
        m_scroll_wheel = false;
      }
    }

    if (m_scroll_wheel)
    {
      m_wheel_distance += (decoder.y() - m_touch_pos_y) / 10;

      int rel = m_wheel_distance/10;
      if (rel != 0)
      {
        m_evdev.send(EV_REL, REL_WHEEL, -rel);

        m_wheel_distance -= rel;
        m_touch_pos_x = decoder.x();
        m_touch_pos_y = decoder.y();
      }
    }
    else
    {
      m_evdev.send(EV_REL, REL_X, decoder.x() - m_touch_pos_x);
      m_evdev.send(EV_REL, REL_Y, decoder.y() - m_touch_pos_y);

      m_touch_pos_x = decoder.x();
      m_touch_pos_y = decoder.y();
    }
  }
  else
  {
    if (m_finger_touching) {
      std::chrono::steady_clock::time_point new_touch_time = std::chrono::steady_clock::now();
      auto const click_duration_msec = std::chrono::duration_cast<std::chrono::milliseconds>(new_touch_time - m_touch_time).count();

      log_debug("click duration: {:4} msec - offset: {:4} {:4}",
                click_duration_msec,
                std::abs(m_touchdown_pos_x - m_touch_pos_x),
                std::abs(m_touchdown_pos_y - m_touch_pos_y));

      int const click_duration_threshold_msec = 150;
      if (click_duration_msec < click_duration_threshold_msec) {
        int const threshold = 16;
        if (std::abs(m_touch_pos_x - m_touchdown_pos_x) < threshold &&
            std::abs(m_touch_pos_y - m_touchdown_pos_y) < threshold)
        {
          log_debug("sending click");
          m_evdev.send(EV_KEY, BTN_LEFT, 1);
          m_evdev.send(EV_SYN, SYN_REPORT, 0);
          m_evdev.send(EV_KEY, BTN_LEFT, 0);
        }
      }
    }

    m_finger_touching = false;
  }
  m_evdev.send(EV_SYN, SYN_REPORT, 0);
}

} // namespace driver

/* EOF */
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
#include <uinpp/multi_device.hpp>
#include <uinpp/event_emitter.hpp>

#include "udraw_decoder.hpp"

namespace udraw {

TouchpadDriver::TouchpadDriver(uinpp::MultiDevice& evdev) :
  m_evdev(evdev),
  m_touchclick(),
  m_up(),
  m_down(),
  m_left(),
  m_right(),
  m_triangle(),
  m_cross(),
  m_square(),
  m_circle(),
  m_start(),
  m_select(),
  m_guide(),
  m_rel_wheel(),
  m_rel_hwheel(),
  m_rel_x(),
  m_rel_y(),
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
  m_touchclick = m_evdev.add_key(static_cast<uint32_t>(uinpp::DeviceType::MOUSE), BTN_LEFT);

  m_start = m_evdev.add_key(static_cast<uint32_t>(uinpp::DeviceType::KEYBOARD), KEY_FORWARD);
  m_select = m_evdev.add_key(static_cast<uint32_t>(uinpp::DeviceType::KEYBOARD), KEY_BACK);
  m_guide = m_evdev.add_key(static_cast<uint32_t>(uinpp::DeviceType::KEYBOARD), KEY_ESC);
  m_down = m_evdev.add_key(static_cast<uint32_t>(uinpp::DeviceType::KEYBOARD), KEY_SPACE);
  m_cross = m_evdev.add_key(static_cast<uint32_t>(uinpp::DeviceType::KEYBOARD), KEY_ENTER);

  m_right = m_evdev.add_key(static_cast<uint32_t>(uinpp::DeviceType::MOUSE), BTN_LEFT);
  m_left = m_evdev.add_key(static_cast<uint32_t>(uinpp::DeviceType::MOUSE), BTN_RIGHT);
  m_up = m_evdev.add_key(static_cast<uint32_t>(uinpp::DeviceType::MOUSE), BTN_MIDDLE);

  m_square = m_evdev.add_key(static_cast<uint32_t>(uinpp::DeviceType::MOUSE), BTN_LEFT);
  m_circle = m_evdev.add_key(static_cast<uint32_t>(uinpp::DeviceType::MOUSE), BTN_RIGHT);
  m_triangle = m_evdev.add_key(static_cast<uint32_t>(uinpp::DeviceType::MOUSE), BTN_MIDDLE);

  m_rel_wheel = m_evdev.add_rel(static_cast<uint32_t>(uinpp::DeviceType::MOUSE), REL_WHEEL);
  m_rel_hwheel = m_evdev.add_rel(static_cast<uint32_t>(uinpp::DeviceType::MOUSE), REL_HWHEEL);

  m_rel_x = m_evdev.add_rel(static_cast<uint32_t>(uinpp::DeviceType::MOUSE), REL_X);
  m_rel_y = m_evdev.add_rel(static_cast<uint32_t>(uinpp::DeviceType::MOUSE), REL_Y);

  m_evdev.finish();
}

void
TouchpadDriver::receive_data(uint8_t const* data, size_t size)
{
  bool send_click = false;

  UDrawDecoder decoder(data, size);

  m_start->send(decoder.start());
  m_select->send(decoder.select());
  m_guide->send(decoder.guide());

  m_up->send(decoder.up());
  m_down->send(decoder.down());
  m_left->send(decoder.left());
  m_right->send(decoder.right());

  m_triangle->send(decoder.triangle());
  m_cross->send(decoder.cross());
  m_square->send(decoder.square());
  m_circle->send(decoder.circle());

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
        m_rel_wheel->send(-rel);

        m_wheel_distance -= rel;
        m_touch_pos_x = decoder.x();
        m_touch_pos_y = decoder.y();
      }
    }
    else
    {
      m_rel_x->send(decoder.x() - m_touch_pos_x);
      m_rel_y->send(decoder.y() - m_touch_pos_y);

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
          send_click = true;
        }
      }
    }

    m_finger_touching = false;
  }

  if (send_click) {
    log_debug("sending click");
    m_touchclick->send(1);
    m_evdev.sync();

    m_touchclick->send(0);
    m_evdev.sync();
  } else {
    m_evdev.sync();
  }
}

} // namespace driver

/* EOF */

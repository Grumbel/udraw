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
  m_previous_mode(UDrawDecoder::Mode::NONE),
  m_discard_events(0),
  m_touchdown_pos_x(0),
  m_touchdown_pos_y(0),
  m_touch_pos_x(0),
  m_touch_pos_y(0),
  m_multitouch_pos_x(0),
  m_multitouch_pos_y(0),
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
  uinpp::VirtualDevice* keyboard = m_evdev.create_device(0, uinpp::DeviceType::KEYBOARD);
  keyboard->set_name("uDraw Touchpad Driver (keyboard)");
  keyboard->set_usbid(0x3, 0x20d6, 0xcb17, 0x110);

  m_start = keyboard->add_key(KEY_FORWARD);
  m_select = keyboard->add_key(KEY_BACK);
  m_guide = keyboard->add_key(KEY_ESC);
  m_down = keyboard->add_key(KEY_SPACE);
  m_cross = keyboard->add_key(KEY_ENTER);

  uinpp::VirtualDevice* mouse = m_evdev.create_device(0, uinpp::DeviceType::MOUSE);
  mouse->set_name("uDraw Touchpad Driver (mouse)");
  mouse->set_usbid(0x3, 0x20d6, 0xcb17, 0x110);

  m_touchclick = mouse->add_key(BTN_LEFT);

  m_right = mouse->add_key(BTN_LEFT);
  m_left = mouse->add_key(BTN_RIGHT);
  m_up = mouse->add_key(BTN_MIDDLE);

  m_square = mouse->add_key(BTN_LEFT);
  m_circle = mouse->add_key(BTN_RIGHT);
  m_triangle = mouse->add_key(BTN_MIDDLE);

  m_rel_wheel = mouse->add_rel(REL_WHEEL_HI_RES);
  m_rel_hwheel = mouse->add_rel(REL_HWHEEL_HI_RES);

  m_rel_x = mouse->add_rel(REL_X);
  m_rel_y = mouse->add_rel(REL_Y);

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

  if (decoder.mode() == UDrawDecoder::Mode::TOUCH)
  {
    if (m_discard_events > 0) {
      m_discard_events -= 1;
    }

    if (m_previous_mode == UDrawDecoder::Mode::MULTITOUCH) {
      // when switching between TOUCH and MULTITOUCH, the reported
      // position takes a bit to settle back into a steady state
      m_discard_events = 16;
    } else if (m_previous_mode != UDrawDecoder::Mode::TOUCH || m_discard_events > 0) {
      m_touchdown_pos_x = decoder.x();
      m_touchdown_pos_y = decoder.y();

      m_touch_time = std::chrono::steady_clock::now();

      m_touch_pos_x = decoder.x();
      m_touch_pos_y = decoder.y();

      if (m_touch_pos_x < 120 || m_touch_pos_x > 1800)
      {
        m_scroll_wheel = true;
        m_wheel_distance = 0;
      }
      else
      {
        m_scroll_wheel = false;
      }
    } else if (m_discard_events == 0) {
      if (m_scroll_wheel)
      {
        m_wheel_distance += (decoder.y() - m_touch_pos_y);

        int rel = m_wheel_distance;
        if (rel != 0)
        {
          m_rel_wheel->send(-rel * 5);

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
  }
  else if (decoder.mode() == UDrawDecoder::Mode::MULTITOUCH)
  {
    if (m_previous_mode != UDrawDecoder::Mode::MULTITOUCH) {
      m_multitouch_pos_x = decoder.x();
      m_multitouch_pos_y = decoder.y();
    } else {
      int const offset = (m_multitouch_pos_y - decoder.y());

      m_rel_wheel->send(offset * 5);

      m_multitouch_pos_x = decoder.x();
      m_multitouch_pos_y = decoder.y();
    }
  }
  else if (decoder.mode() == UDrawDecoder::Mode::NONE)
  {
    if (m_previous_mode == UDrawDecoder::Mode::TOUCH) {
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

  m_previous_mode = decoder.mode();
}

} // namespace driver

/* EOF */

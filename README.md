[![Build Status](https://travis-ci.org/Grumbel/udraw.svg?branch=master)](https://travis-ci.org/Grumbel/udraw)

uDraw PS3 Linux Driver
======================

Quick&dirty driver for the PS3 uDraw graphics tablet. If customization
is needed, hack the source.

Linux has a udraw driver in the kernel itself as well, rendering this
mostly unnecessary unless some customization is desired.

For a Windows driver, see: http://brandonw.net/udraw/


Requirements:
-------------
* cmake
* git
* libusb-1.0
* fmt
* g++


Compilation:
------------

    mkdir build
    cd build
    cmake ..
    make


Running:
--------

Plug in the USB dongle for the graphics tablet, switch on the graphics
tablet and sync up as usual, then run one of the following commands
depending on your needs:

    udraw-driver --touchpad

    udraw-driver --tablet

    udraw-driver --gamepad

    udraw-driver --keyboard

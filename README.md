uDraw PS3 Linux Driver
======================

Quick&dirty driver for the PS3 uDraw graphics tablet. If customization
is needed, hack the source.

For a Windows driver, see: http://brandonw.net/udraw/


Requirements:
-------------
* scons
* git
* libusb-1.0
* boost
* g++-4.5 or above


Compilation:
------------

    $ scons


Running:
--------

Plug in the USB dongle for the graphics tablet, switch on the graphics
tablet and sync up as usual, then run one of the following commands
depending on your needs:

    $ udraw-driver --touchpad

    $ udraw-driver --tablet

    $ udraw-driver --gamepad

    $ udraw-driver --keyboard

    $ udraw-driver --accelerometer

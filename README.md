# Unofficial Acer Nitro 17 (AN17-71) RGB keyboard and fan controlling driver.

## General information

Inspired by [acer-predator-turbo-and-rbg-keyboard-linux-module](https://github.com/JafarAkhondali/acer-predator-turbo-and-rgb-keyboard-linux-module).
The project has been developed to have a simple and understandable code to control the
RGB keyboard and fan turbo mode for the Acer Nitro 17 (AN17-71).

As in referenced project this kernel module creates a character device for the keyboard
and one for the fan.

> **Warning**
> ## This module has been developed specially for Acer Nitro 17 (model AN17-71). It has not been fully tested so use it at your own risk.

#### Useless info
The aim of the project was to learn to create a kernel module with character devices.
Though the code seems to do the same thing as the module by [@JafarAkhondali](https://github.com/JafarAkhondali), I wanted to know
where do values come from and that's why I did the reverse-engineering again by myself.

## Usage

### Fans
Fans can be set to automatic, turbo and manual mode.

The ``/dev/acer-nitro17_fan`` accepts a 3 bytes length array which consists of :

1. Fan mode.
   - ``0x00`` : automatic.
   - ``0x01`` : turbo.
   - ``0x02`` : manual.
2. Fan index. Valid values are 0 or 1. Used only if in manual mode.
3. Fan speed (in percents). Valid values are 0-100. Used only if in manual mode.

### RGB Keyboard
RGB Keyboard has more settings. Currently, area definitions are not supported.

The ``/dev/acer-nitro17_kbd`` accepts a 7 bytes length array which consists of :

1. Backlight mode.
    - ``0x00`` : Static mode.
    - ``0x01`` : Breathing.
    - ``0x02`` : Neon. Color info is not used.
    - ``0x03`` : Wave. Color info is not used.
    - ``0x04`` : Shifting.
    - ``0x05`` : Zoom.
    - ``0x06`` : Meteor.
    - ``0x07`` : Twinkling.
2. Speed. It can be from ``1`` to ``9``, where ``1`` is slowest and ``9`` fastest.
However, if the mode is static (``0x00``), speed should be set to ``0``.
3. Brightness. Lowest value is ``0`` (off) and highest is ``100`` (``0x64``).
NitroSense allows only ``0``, ``25``, ``50``, ``75`` and ``100``. Middle values has not been tested yet.
4. Direction. It is ``1`` for left-to-right and ``2`` for right-to-left. If the mode is
static, the direction should be set to ``0``.
5. Red component. Goes from ``0`` to ``255``.
6. Green component. Goes from ``0`` to ``255``.
7. Blue component. Goes from ``0`` to ``255``.

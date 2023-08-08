#!/bin/bash

if test $EUID -ne 0; then
  echo "Uninstall script should be run as root"
  exit 1
fi

if test ! -f "/lib/modules/$(uname -r)/kernel/drivers/acer/acer-nitro17.ko"; then
  echo "Module not present! Aborting"
  exit 2
fi

rm "/lib/modules/$(uname -r)/kernel/drivers/acer/acer-nitro17.ko"
if test -f "/etc/modules-load.d/acer-nitro17.conf"; then
  rm "/etc/modules-load.d/acer-nitro17.conf"
fi

echo "Module uninstalled."
exit 0

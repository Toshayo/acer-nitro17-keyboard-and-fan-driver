#!/bin/bash
declare IS_REINSTALL=0

if test "$1" == "reinstall"; then
  IS_REINSTALL=1
fi

if test $EUID -ne 0; then
  echo "Uninstall script should be run as root"
  exit 1
fi

if test ! -f "/lib/modules/$(uname -r)/kernel/drivers/acer/acer-nitro17.ko"; then
  echo "Module not present! Aborting"
  exit 2
fi

if test $IS_REINSTALL -eq 0; then
  echo "Uninstalling module"
else
  echo "Uninstalling old module version"
fi

rm "/lib/modules/$(uname -r)/kernel/drivers/acer/acer-nitro17.ko"
if test -f "/etc/modules-load.d/acer-nitro17.conf"; then
  rm "/etc/modules-load.d/acer-nitro17.conf"
fi

rm /etc/udev/rules.d/50-acer-nitro17.rules

if test $IS_REINSTALL -eq 0; then
  echo "Removing group 'acer-nitro17-driver'"
  groupdel acer-nitro17-driver
fi

echo "Module uninstalled."

unset IS_REINSTALL

exit 0

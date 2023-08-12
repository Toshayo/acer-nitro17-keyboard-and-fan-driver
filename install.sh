#!/bin/bash
declare IS_REINSTALLING=0

if test $EUID -ne 0; then
  echo "Install script should be run as root"
  exit 1
fi

if test -f "/lib/modules/$(uname -r)/kernel/drivers/acer/acer-nitro17.ko"; then
  chmod o+x "/lib/modules/$(uname -r)/kernel/drivers/acer/acer-nitro17-uninstall.sh"
  "/lib/modules/$(uname -r)/kernel/drivers/acer/acer-nitro17-uninstall.sh" reinstall
  IS_REINSTALLING=1
  if test $? -ne 0; then
    echo "Failed to uninstall previous version. Aborting."
    exit 2
  fi
  rm "/lib/modules/$(uname -r)/kernel/drivers/acer/acer-nitro17-uninstall.sh"
fi

if test ! -f "cmake-build-debug/acer-nitro17.ko"; then
  echo "Kernel module not found. Aborting."
  exit 2
fi

mkdir -p "/lib/modules/$(uname -r)/kernel/drivers/acer/"
cp "cmake-build-debug/acer-nitro17.ko" "/lib/modules/$(uname -r)/kernel/drivers/acer/acer-nitro17.ko"
echo "acer-nitro17" > "/etc/modules-load.d/acer-nitro17.conf"

cp udev-rules/50-acer-nitro17.rules /etc/udev/rules.d/50-acer-nitro17.rules

# Uninstall script will be copied to be the uninstalled of the specific version. (In case of an structure change)
cp "uninstall.sh" "/lib/modules/$(uname -r)/kernel/drivers/acer/acer-nitro17-uninstall.sh"

if test $IS_REINSTALLING -eq 0; then
  echo "Creating group 'acer-nitro17-driver'"
  groupadd acer-nitro17-driver
fi

echo "Module installed."

unset IS_REINSTALLING

exit 0

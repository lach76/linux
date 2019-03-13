#!/bin/bash
make Image dtbs modules
make modules_install
cp -f arch/arm64/boot/Image arch/arm64/boot/dts/meson64_odroidc2.dtb /media/boot
sync

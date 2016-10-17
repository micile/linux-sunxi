#!/bin/sh

set -e

rm -rf lib_kernel; make -j9 zImage dtbs modules modules_install INSTALL_MOD_PATH=lib_kernel

ssh root@192.168.1.16 "mount /dev/mmcblk0p1 mntdir"
scp arch/arm/boot/zImage root@192.168.1.16:/root/mntdir/
scp arch/arm/boot/dts/micile-A33G6.dtb root@192.168.1.16:/root/mntdir/micile.dtb

#scp drivers/iio/accel/bma180.ko root@192.168.1.19:/root/
#scp drivers/iio/accel/bma220_spi.ko root@192.168.1.19:/root/
#scp drivers/iio/accel/bmc150-accel-core.ko root@192.168.1.19:/root/
#scp drivers/iio/accel/bmc150-accel-spi.ko root@192.168.1.19:/root/
#scp drivers/iio/buffer/industrialio-triggered-buffer.ko root@192.168.1.19:/root/
#scp drivers/iio/buffer/kfifo_buf.ko root@192.168.1.19:/root/
#scp drivers/iio/accel/bmc150-accel-i2c.ko root@192.168.1.19:/root/
#scp drivers/iio/accel/mxc622x.ko root@192.168.1.19:/root/
#scp drivers/misc/micile_io_spy.ko root@192.168.1.19:/root/
ssh root@192.168.1.16 "umount mntdir"

tar -C lib_kernel -cf - ./lib/ | ssh root@192.168.1.16 "tar -C / -xf -"

ssh root@192.168.1.16 "reboot"

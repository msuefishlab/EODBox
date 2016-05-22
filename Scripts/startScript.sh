#!/bin/bash

echo EBB-PRU-ADC > /sys/devices/bone_capemgr.*/slots

rmmod uio_pruss

modprobe uio_pruss extram_pool_sz=0x7a1200

hwclock -s -f /dev/rtc1

hwclock -w





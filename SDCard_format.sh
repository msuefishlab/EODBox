#!/bin/bash

umount /dev/mmcblk1p1
mkfx.ext4 /dev/mmcblk1p1
cd /
mkdir /media/store
mount -t ext4 /dev/mmcblk1p1 /media/store

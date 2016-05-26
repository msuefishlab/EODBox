############
Enable the Real-Time Clock
############


1. First declare the Real Time Clock to the kernel::

	echo ds1307 > /sys/class/i2c-adapter/i2c-1/new_device

2. Check if the Real Time Clock has been loaded to address 68::

	i2cdetect –y –r 1

 If the Real Time Clock has not loaded, then the result should look like this

 .. image:: ./images/rtc_not_loaded.png
	:height: 200pt
	:align: center

 It is also possible that “UU” will appear at address 68. If this is the case a driver is possibly loaded to that address. Remove the driver::

	echo 0x68 /sys/class/i2c-adapter/i2c-1/delete_device
	rmmod ds1307

 The driver is now unloaded. Repeat the commands from above. If the driver has loaded correctly “68” will appear at address 68.

3. To set the time, first connect an Ethernet cord to the BeagleBone. To get the current UTC time and push that time to the RTC enter the command::

	ntpdate –b –s –u pool.ntp.org
	hwclock –w –f /dev/rtc1

4. To have the RTC enabled and read from on startup, add these lines to the beginning of the script that modifies the PRU memory::

	echo ds1307 0x68 > /syds/class/i2c-adapter-i2c-1/new_device
	hwclock –s –f /dev/rtc1
	hwclock –w

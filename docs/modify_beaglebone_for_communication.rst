############
Modifying the BeagleBone for Communication
############


The BeagleBone must be modified to enable communication with the shield in order to function properly.  Follow these steps to do so: 

1. Enable the pins and slots.
To do so, enter the following commands into the command line::

	export SLOTS=/sys/devices/bone_capemgr.*/slots
	export PINS=/sys/devices/debug/pinctrl/44e10800.pinmux/pins

Note: the “*” indicates that this value differs depending on which version of Linux your BeagleBone has. To have this value automatically filled out for you, hit tab when typing the “bone_capemgr.*” portion of the command.

2. To enable these on startup the ,profile file needs to be edited. To do this enter the command::

	nano ~/.profile

And copy the following configuration::

	~/.profile: executed by Bourne-compatible login shells.

	if [ "$BASH" ]; then
		if [-f ~/.bashrc ]; then
			.~/.bashrc
		fi
	fi
	mesg n
	export SLOTS=/sys/devices/bone_capemgr.9/slots
	export PINS=/sys/devices/debug/pinctrl/44e10800.pinmux/pins

3. Save and close the file
4. The HDMI needs to be disabled in order to access the PRUs, disable HDMI by the following commands::

	sudo mkdir /mnt/vfat
	sudo mount /dev/mmcblk0p1 /mnt/vfat
	
5. Edit the uEnv.txt Configuration File by the following::

	cd /mnt/vfat
	nano uEnv.txt

The unedited file will have a “#” before the optargs= line. Delete the (#) symbol to disable the HDMI::

	##Disable HDMI
	optargs=capemgr.disable_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN

.. note::
 Warning!  There are similar lines also in this configuration file (labeled by the "##Disable HDMI/eMMC"), Uncommenting this line will disable the eMMC as well and the BeagleBone will not boot. This can be corrected by flashing the BeagleBone using an SD card and instructions on how to do this are included in Appendix B.


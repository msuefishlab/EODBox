############
External Storage
############

The BeagleBone has limited storage, but has the capability of using SD card for additional storage space. A script that includes the necessary commands to format the SD card is already written; however, they are also included in the event the script is not available. 

1. Insert a new micro-SD card into the slot on the BeagleBone while it is running. You can see it by entering the command::

	mount –l
	/dev/mmcblk1p1 on /media/*file location* type vfat …

2. This will automatically mount the SD card to /media/*file location* with type vfat. You can check if there is any data on the card by using the following command::

	cd /media/*file location*
	ls –l

3. Next the SD card will be formatted as an ext4 file system. Be careful to select the correct device. In this instance the device is mmcblk1p1 and it may be different when you go through these steps. If there is data on the SD card, it will be erased by these steps::

	~/media/*file location*# cd/
	umount /dev/mmcblk1p1
	mkfx.ext4 /dev/mmcblk1p1

4. Next a mount point will be created for the card, in this case /media/store (note: the mount point does not have to be /media/store; however, the script specifies this location. If you want a different location you will need to edit the script to reflect this change.), and the card will be mounted at this point::
	
	cd /
	mkdir /media/store
	mount –t ext4 /dev/mmcblk1p1 /media/store
	cd /media/store
	mount –l

 .. note:: 
  The –t provided to the mount command indicates the file type. If omitted, “mount” will try to auto-detect the file system type.

5. The available capacity can be checked using the command::
	
	df –k

6. Now, the BeagleBone needs to modified to automatically mount the SD card when the BeagleBone boots. This process cannot be automated. You have to mount the device using its UUID (universally unique identifier). Type the following command to find the UUID::
	
	blkid /dev/mmcblk1p1

7. Write down the UUID number. In order to mount the SD card automatically the /etc/fstab file needs to be edited. Type the following command::
	
	nano /etc/fstab

8. At the end of the file add the following line::

	UUID=”UUID number”  /media/store  ext4  defaults,nofail,user,auto  0  0

 .. note:: 
	 The options listed in the line are:

	 Defaults: Use default settings

	 nofail: Mount the device when present but ignore if absent

	 user: The user gives permission to mount the file system

	 auto: The card will be mounted on start-up, or if the user types mount -a 
	    
	 Be very careful when editing fstab, particularly this line as it can cause the BeagleBone to not reboot properly or preventing it from booting at all. Where there is separation, two spaces should be present, or an error will occur.
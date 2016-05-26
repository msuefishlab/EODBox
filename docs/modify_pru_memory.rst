############
Modifying the PRU Memory
############

1. The PRU memory needs to be modified to the size specified in the code. First the device tree overlay needs to be loaded to the slots. Compile EBB-PRU-ADC.dts. Enter the following command::
	
	echo EBB-PRU-ADC > $SLOTS

2. The UIO driver must be removed in order to be able to modify the memory. Remove the UIO driver::
	
	rmmod uio_pruss

3. Then modify the memory size::

	modprobe uio_pruss extram_pool_sz=0x7a1200
 
 This sets the memory size to 8,000,000 bytes. 


4.  The memory must be modified each time the BeagleBone boots up. This process can be automated using a script and creating a service to run the script. The script will include the previous commands. The finished script is shown below::

	#!/bin/bash
	echo EBB-PRU-ADC > /sys/devices/bone_capemgr.*/slots
	rmmod uio_pruss
	modprobe uio_pruss extram_pool_sz=0x7a1200

 .. note::
  Make sure that in whatever source code editor that the script is written in, the EOL is set to Unix/OSX. 

5. The script needs to be made executable before it can run. To make it executable enter the following command::
		
		chmod a+x <filename>.sh

    This will make the file executable by all users. 

6. Move the script to the /usr/bin directory::
	
		mv <filename>.sh /usr/bin/

7. Next a systemd service must be created that initiates the script on startup. A systemd service is a file that encodes information about a process controlled by systemd.  Create the service by typing::

		nano /lib/system/<scriptname>.service

 And enter the following::

		[Unit]
		Description = load device tree and modify memory
		After= syslog.target
		[Service]
		Type = simple
		ExecStart = /usr/bin/<scriptname>.sh
		[Install]
		WantedBy = multi-user.target

 .. note:: 
   The “[Unit]” section contains the description and start dependencies. 

   The “[Service]” section contains the information about the service and the process it supervises. With the type set to “simple” the process configured in “ExecStart” is the main process of the service. 
   
   The “[Install]” section is used to define the behavior or a unit if it is enabled or disabled. Enabling a unit marks it to be automatically started at boot.
		
8. A symbolic link must be created to let the device know the location of the service::

		cd /etc/systemd/sytem
		ln /lb/systemd/<scriptname>.service <scriptname>.service

9. Make systemd reload the configuration file. Start the service immediately to see if the service is functioning correctly and enable the unit files specified in the command line::

	systemctl daemon-reload
	systemctl start <scriptname>.service
	systemctl enable <scriptname>.service

 Now the service is enabled and will execute the script on startup. 

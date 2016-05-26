.. EODBox documentation master file, created by
   sphinx-quickstart on Thu May 26 09:26:58 2016.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to EODBox!
==================================

Weakly electric fish are capable of both emitting and detecting electric signals called electric organ discharges (EODs). Current methods of recording EODs use multiple pieces of equipment that are fragile, expensive and inconvenient to transport, and rely on commercial software to record and analyze data. The goal of this project was to create a low cost and portable alternative to current research equipment. 

We created a device that we beleive circumvents these limitations, based the BeagleBone Black Rev C (BBB). The BBB uses a Texas Instruments AM335x 1GHz ARM© Cortex-A8, a reduced instruction set computing microprocessor that is capable of running two billion instructions per second. This is powerful processor for its size and is capable of running a full operating system.  Our goal was to create a device with a user interface that allows for an adjustable, high-speed sampling rate, a designated run time, adjustable gain, flexible storage, and two ‘run modes’: a raw data recording a mode where EODs are automatically extracted.  The device can be broken up into two units: a microcontroller that supplies power and processes the data, and a shield that amplifies and digitizes the signal.


=================
How Does It Work??
=================

.. toctree::
   :numbered:

   pru_background
   device_details
   data_storage

=================
How Do I Make My Own??
=================

.. toctree::
   :numbered:

   getting_started
   modify_beaglebone_for_communication
   external_storage
   modify_pru_memory
   rtc_enabling


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`


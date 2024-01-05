# Green House Control (2008)

This software acted along with a circuit projected and built by me to monitor and act in a greenhouse system. It records data of Relative humidity and temperature and acts in the system irrigating when necessary.

### Hardware

The system is an **autonomous board** equipped with a **PIC16F877** and some peripherals such as a temperature sensor, relative humidity sensor, some relays with optical couplers, and a serial communication module.
It reads air humidity and temperature data and records it in the microcontroller's ROM (the software can later download it).
Once programmed, it acts autonomously within the times and parameters established during setup.

## Software

This module communicates via serial (COM) with the board to get the data logged into the ROM of PIC16F877 and also sets up the hours and times of the irrigation system.

It was able to **save the irrigation routines** into files .prg to be loaded when necessary. Ex: _winter.prg/summer.prg/rooting.prg_

*Screenshot* 

![image](https://github.com/alexandredrefahl/greenhouse_control/assets/24326296/40707ed7-67d0-44f5-9934-09ac75674120)

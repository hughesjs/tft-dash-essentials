README.txt
------------------------------------

TFT Dash Firmware / Display Software


Contents of Folders
	3DModels:
		- STL files needed for 3D printing the covering enclosure
	HardwareGen3:
		- Gen 3 (and currently shipped) - version of the board using through hole components.
	HardwareGen4:
		- Gen 4 is the move from through hole components to surface mounted components using an ATMega32u4 as the MP of choice.
		- Contains the schematic and board design for the Gen 4 board with integrated ATMega32u4.
		- .brd and .sch requires EAGLE to be installed.
	tftdashdisplay:
		- Display Software designed to run on Raspberry Pi. Compiled using gnu C/C++ compiler (g++). Requires the SDL library to be installed on the RPi. (more info at www.libsdl.org)
		- Main source file is testsdl.cpp
		- Graphic / Sprite files needed in the same folder as the running binary.
	tftdashfirmwareGEN4:
		- Arduino Code for Firmwware. Reads / Interprets signals from the Bike via dedicated Shield / USB Interface hardware & produces a constant stream of String data at 115200 bps over USB Serial. Deployed and compatible with Gen3 and Gen4 boards using Compiler flags to specify board revision used.



2018-2024 TFTDashProject / Danny Draper.




Contact:
ddraper.cedesoft@gmail.com
support@tftdashproject.co.uk

http://www.tftdashproject.co.uk


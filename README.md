# Bebump Coolth fan controller

Bebump Coolth is a Windows utility that can dynamically control 4 fans
connected to an Arduino Nano.

![System diagram](docs/img/overview.png)

The code and build instructions for the Arduino part are also shared in this
repository. That includes a PCB and 3D printable enclosure if you want to make
it nice.

# Getting started

## Setting up your Arduino

Looking at the wiring diagram in the picture up top, you can quickly assemble
a working prototype using a breadboard.

You need to program the Nano board with the source file in
`arduino_nano/fan_controller.ino`.

If you want you can set persistent fan speeds stored on the Arduino by sending
the following command to it over serial.

    c 2 60 60 60 60

Where the last four `0 <=` integers <= `255`  are the duty cycles for the four
fans. During active operation the PC control software can dynamically overrule
these values. In case the PC software is not running, or communication fails
for more than 3 seconds the Arduino will go back to these persistent values.

## Installing the PC control software

You can download a Windows installer from the releases section. You have the
option to select automatic launch upon Windows startup from the installer.

After launching Bebump Coolth the program will scan the available COM ports
and find the programmed Arduino. It remembers the right COM port, so next time
it will open the same port first.

Everyting can be configured from the GUI and using the program should be
self-explanatory after a bit of experimentation.

The program must run to keep controlling your fans. If you minimize the
application it will become a background process and it will sit on the taskbar
notification area consuming next to no resources.


# Building the PC control software

## The standard way

* Visual Studio 2017 (C# and C++)
* CMake 3.15+

You can use these to build every binary artifact. You have to build
`temperature_reader/temperature_reader.sln` and the `bebump_coolth` target of
`CMakeLists.txt`.


## The completely automatic way

If you want, you can run `scripts/build_everything.py` which will probably do
everything for you from a completely clean checkout. In this case, you also
need

* Python 3.6+

Running the script file will ask you to specify the build output directory (or
use default). Then it will try to find your Visual Studio installation, and
CMake (should be available on your PATH), and then try to build everything.

Worked on my machine.


## Creating the installer

If you choose the automatic way, there will be a bunch of files in the build
output directory.

If you have 

* InnoSetup 6.1.2

you can double click and run the `coolth.iss` file from the build output
directory, press the green play button, and you will have your own installer.


# Building a nicer controller hardware

If you don't want a fire hazard lying around or inside your computer, you may
want to build a proper device with a PCB and 3D printed enclosure.

You can order a PCB by sending the files in `arduino_nano/gerber` to a PCB
manufacturer.

![Fan controller PCB](docs/img/pcb.png)

You need to solder the folling components on the PCB.

* 4x Molex 47053-1000 connector
* 1x 5.08 mm PCB terminal block
* 2.54 mm pin headers for connecting the Arduino Nano

# Mobile Speaker: Vehicle Controller

An OSC based proxy controller for Linux-based hosts (RPi 4) for a vehicle driven by Trinamic PD60-3-1160 stepper motors,
plus servo controls through PWM.

Part of the ICST research project [Sound Moving Sources](https://www.zhdk.ch/forschungsprojekt/sound-moving-sources-577831) that
entails motorized, mobile loudspeakers.

# Building

## Dependencies et al

Requires cmake >= 3.19

### macOS

```bash
brew install libserialport
```

### Linux

```bash
sudo apt install libserialport0 libserialport-dev
```


#### RPi

```bash
sudo apt update
sudo apt install snapd
sudo reboot
....
sudo snap install core
sudo snap install cmake --classic
```



## Compiling

```bash
cd ~/Desktop
git clone --recursive https://github.com/tschiemer/vehicle-controller
```

Change `oscpack`s `CMakeLists.txt` as follows (as of May 4th 2021):

```diff
@@ -23,7 +23,7 @@ ELSE()
        cmake_minimum_required(VERSION 2.6)
        PROJECT(TestOscpack)

-       INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
+       INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

        # separate versions of NetworkingUtils.cpp and UdpSocket.cpp are provided for Win32 and POSIX
        # the IpSystemTypePath selects the correct ones based on the current platform
@@ -63,6 +63,7 @@ ELSE()
        osc/OscOutboundPacketStream.cpp

        )
+       target_include_directories(oscpack PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


        ADD_EXECUTABLE(OscUnitTests tests/OscUnitTests.cpp)
```

# Setup

These steps are additional to compiling on the target system.

## Raspberry Pi

The subfolders of folder [setup](setup) contain the necessary files from the file root as need to be added; each subfolder contains the files for a particular mobile speaker configuration, ie:
- Rotary model (2 steppers for movement, 1 stepper for rotation)
- Aggregat model (4 steppers for movement, 2 servos for rotation/direction)

Please be aware of the "hidden" (dot-notation) folders typically not visible on macOS/linux (ex /home/pi/.config).

In the following some general notes that *should* already be included in the files as mentioned above.

---

To generate USB port specific serial port links add file `/etc/udev/rules.d/99-trinamic-motor.rules`
```txt

KERNEL=="ttyACM[0-9]*", SUBSYSTEM=="tty", ATTRS{devpath}=="1.1", ATTRS{idVendor}=="2a3c", ATTRS{idProduct}=="0100", SYMLINK="ttyMotor1"
KERNEL=="ttyACM[0-9]*", SUBSYSTEM=="tty", ATTRS{devpath}=="1.2", ATTRS{idVendor}=="2a3c", ATTRS{idProduct}=="0100", SYMLINK="ttyMotor2"
KERNEL=="ttyACM[0-9]*", SUBSYSTEM=="tty", ATTRS{devpath}=="1.3", ATTRS{idVendor}=="2a3c", ATTRS{idProduct}=="0100", SYMLINK="ttyMotor3"
KERNEL=="ttyACM[0-9]*", SUBSYSTEM=="tty", ATTRS{devpath}=="1.4", ATTRS{idVendor}=="2a3c", ATTRS{idProduct}=="0100", SYMLINK="ttyMotor4"
```
and reboot (or run `sudo udevadm control --reload-rules && sudo udevadm trigger`).

---
To automatically start basic script as follows:

Add starter script `/home/pi/Desktop/Starte-Motoren.sh`:
```txt
#!/bin/bash

vehicle-controller/cmake-build/mobspkr-vehicle-ctrl -d0:l -d1:r /dev/ttyMotor1 /dev/ttyMotor2
#vehicle-controller/cmake-build/mobspkr-vehicle-ctrl -d0:l -d1:r /dev/ttyACM0 /dev/ttyACM1


echo ">>> Application terminate <<<"
echo "> Hit enter to close window (please make a photo first and send to philip) <"

read
```

Create autostart file `/home/pi/.config/autostart/motor.desktop`:
```txt
[Desktop Entry]
Type=Application
Name=Motor Starter
Exec=lxterminal --command="/bin/bash -c 'cd /home/pi/Desktop; ./Starte-Motoren.sh; /bin/bash'"
```

## OSC commands

### rpi-osc-stepper (mobspkr-vehicle-ctrl)
OSC receive port 9494

- `/motor/init <motor-index>` initializes motor with necessary parameters
- `/motor/msr <motor-index> <msr-value>` sets microstep resolution (don't use unless you know whacha doin)
- `/motor/stop <motor-index>` stops motor
- `/motor/rotate <motor-index> <speed>` rotate motor with <speed>
- `/motor/reset-position <motor-index>` stops motor and sets current position as origin (0 degrees) position
- `/motor/move-by-angle <motor-index> <angle>` moves motor by <angle> (-360 .. 360) from *current* position (relative)
- `/motor/move-to-angle <motor-index> <angle>` moves motor to <angle> (-360 .. 360) from origin position; when rotating, positive values will cause a rotation until <angle> in the current rotational direction whereas negative values will be in the anti-direction
- `/motor/temp <motor-index> <host> <port>` request motor temperature to be sent to <host> on <port> using message `/temp <device-name> <motor-index> <temp>` 
- `/motor/volt <motor-index> <host> <port>` request voltage on motor to be sent to <host> on <port> using message `/volt <device-name> <motor-index> <volt>`

### rspi-osc-pwm (mobspkr-osc-pwm)
OSC receive port 9393
- `/pwm <pwm-index> <pwm-width>`

## Devices



## Control Patches (Max/MSP)

Also see folder [control-patches](control-patches):

- `SMS_Control_Angela.maxpat` provides controls for the AGGREGAT servo motors (IP 192.168.0.71)
- `SMS_Control_Roger.maxpat` provides controls for the two-motor with rotary speaker mobile using a wheel/pedal HID (IP 192.168.0.72)

*Note that the AGGERGAT patch (at this time) comes with two externals that will be blocked for security reasons by macOS. To unblock follow [these instructions](https://cycling74.com/articles/using-unsigned-max-externals-on-mac-os-10-15-catalina) by cycling74.

*Important note* the combination of macOS/MaxMSP may use different indices for the Joystick HID controls, so possibly the respective `route` blocks have to be adjusted.

# License

MIT License

Copyright 2021 ICST / ZHdK

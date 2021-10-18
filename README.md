# Mobile Speaker: Vehicle Controller

An OSC based proxy controller for a vehicle driven by two Trinamic PD60-3-1160 stepper motors.

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

## Raspberry Pi

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

## Control Patches (Max/MSP)

Also see folder [control-patches](control-patches):

- `Motor Control.maxpat` provides the control for the Trinamic stepper motors
- `AGGREGAT Control.maxpat` provides the control for the AGGREGAT servo motors*

*Note that the AGGERGAT patch (at this time) comes with two externals that will be blocked for security reasons by macOS. To unblock follow [these instructions](https://cycling74.com/articles/using-unsigned-max-externals-on-mac-os-10-15-catalina) by cycling74.

# License

MIT License

Copyright 2021 ICST / ZHdK

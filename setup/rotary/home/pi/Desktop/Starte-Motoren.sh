#!/bin/bash

cd /home/pi/Desktop

# symlinks
#vehicle-controller/cmake-build/mobspkr-vehicle-ctrl -d0:l -d1:r /dev/ttyMotor1 /dev/ttyMotor2

# 2 motors
#vehicle-controller/cmake-build/mobspkr-vehicle-ctrl -d0:l -d1:r /dev/ttyACM0 /dev/ttyACM1

# 3 motors
vehicle-controller/cmake-build/mobspkr-vehicle-ctrl -d0:l -d1:r /dev/ttyACM0 /dev/ttyACM1 /dev/ttyACM2

# 1 motor
#vehicle-controller/cmake-build/mobspkr-vehicle-ctrl -d0:l -d1:r /dev/ttyACM0

echo ">>> Application terminate <<<"
echo "> Hit enter to close window (please make a photo first and send to philip) <"

read

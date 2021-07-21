#!/bin/bash

cd /home/pi/Desktop

vehicle-controller/cmake-build/mobspkr-vehicle-ctrl -d0:l -d1:r /dev/ttyMotor1 /dev/ttyMotor2
#vehicle-controller/cmake-build/mobspkr-vehicle-ctrl -d0:l -d1:r /dev/ttyACM0 /dev/ttyACM1


echo ">>> Application terminate <<<"
echo "> Hit enter to close window (please make a photo first and send to philip) <"

read

# Mobile Speaker: Vehicle Controller

An OSC based proxy controller for a vehicle driven by two Trinamic PD60-3-1160 stepper motors.

# Building

```bash
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

## macOS

```bash
brew install libserialport
```

## Linux

```bash
sudo apt install libserialport0 libserialport-dev
```

# License

MIT License

Copyright 2021 ICST / ZHdK

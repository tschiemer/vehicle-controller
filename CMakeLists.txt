cmake_minimum_required(VERSION 3.19)
project(mobspkr_vehicle_ctrl)

#set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(SERIALPORT_INCLUDE_DIRS "/usr/local/Cellar/libserialport/0.1.1/include")

set(CMAKE_CXX_STANDARD 14)

set(INCLUDE_DIRS src)
set(MOTOR_SOURCE_FILES src/motor.hpp src/motor.cpp)
set(RPI_OSC_PWM_FILES src/rpi-osc-pwm.cpp)

add_subdirectory(src/third_party/oscpack EXCLUDE_FROM_ALL)


include_directories(${INCLUDE_DIRS} ${SERIALPORT_INCLUDE_DIRS})
link_directories("/usr/local/lib/")
link_libraries(serialport)

add_executable(list-ports src/utils/list_ports.c)
add_executable(port-info src/utils/port_info.c)
add_executable(motor-cmd src/utils/motor-cmd.cpp ${MOTOR_SOURCE_FILES})

add_executable(mobspkr-vehicle-ctrl src/main.cpp ${MOTOR_SOURCE_FILES})
target_link_libraries(mobspkr-vehicle-ctrl oscpack)


add_executable(mobspkr-osc-pwm ${RPI_OSC_PWM_FILES})
target_link_libraries(mobspkr-osc-pwm oscpack pigpio)

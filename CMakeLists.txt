cmake_minimum_required(VERSION 3.19)
project(mobspkr_vehicle_ctrl)

#set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(SERIALPORT_INCLUDE_DIRS "/usr/local/Cellar/libserialport/0.1.1/include")

set(CMAKE_CXX_STANDARD 14)


include_directories(${SERIALPORT_INCLUDE_DIRS})
link_directories("/usr/local/lib/")
link_libraries(serialport)

add_executable(list-ports src/utils/list_ports.c)
add_executable(port-info src/utils/port_info.c)

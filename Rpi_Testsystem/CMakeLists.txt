cmake_minimum_required(VERSION 3.19)
project(Rpi_Testsystem)

# Boost directories
set(BOOST_INCLUDEDIR "$ENV{HOME}/rpi/rootfs/usr/include")
set(BOOST_LIBRARYDIR "$ENV{HOME}/rpi/rootfs/lib/arm-linux-gnueabihf")
# ICU directory library for boost regex
set(ICU_ROOT "$ENV{HOME}/rpi/rootfs/lib/arm-linux-gnueabihf")

# Locate libraries and headers
find_package(Boost 1.67 REQUIRED COMPONENTS system timer regex chrono thread)
find_package(ICU REQUIRED COMPONENTS uc dt in io)

# Include headers
include_directories(${Boost_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Set source files
set(SOURCE_FILES "main.cpp" "RpiPinout.h" "RPITestHandler.h" "RPITestHandler.cpp" "RPIUnderTest.h" "RPIUnderTest.cpp")
add_executable(Rpi_Testsystem ${SOURCE_FILES})

# Link against libraries
target_link_libraries(Rpi_Testsystem ${ICU_LIBRARIES})
target_link_libraries(Rpi_Testsystem ${Boost_LIBRARIES} -lpthread)
find_library(WIRINGPI_LIBRARIES NAMES wiringPi)
target_link_libraries(Rpi_Testsystem ${WIRINGPI_LIBRARIES}) 

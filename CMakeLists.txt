cmake_minimum_required(VERSION 3.10)

# Set the project name
project(CHIP-8)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#set(SFML_DIR "C:\\SFML-3.0.0\\lib\\cmake\\SFML")
set(SFML_DIR $ENV{SFML_DIR})
if(NOT DEFINED SFML_DIR)
    message(FATAL_ERROR "SFML_DIR is not set. Please provide it via -DSFML_DIR=<path>")
endif()
find_package(SFML 3.0.0 COMPONENTS Graphics Window System Audio REQUIRED)

# Ensure CMake knows where to find the SFML libraries
#link_directories("C:/SFML-3.0.0/lib")

# Add the executable target
add_executable(chip8 main.cpp
        chip8.cpp
        chip8.h)

#---------


# Add SFML include directory
set(SFML_INCLUDE_DIR $ENV{SFML_INCLUDE_DIR})
if(NOT DEFINED SFML_INCLUDE_DIR)
    message(FATAL_ERROR "SFML_DIR is not set. Please provide it via -DSFML_DIR=<path>")
endif()
target_include_directories(chip8 PRIVATE ${SFML_INCLUDE_DIR})
target_link_libraries(chip8 SFML::Graphics SFML::Window SFML::System SFML::Audio)
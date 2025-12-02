cmake_minimum_required(VERSION 3.16)
project(RTSPProcessor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV
find_package(OpenCV REQUIRED)

# Include directories
include_directories(${OpenCV_INCLUDE_DIRS} src/)

# Add executable
add_executable(RTSPProcessor src/main.cpp)

# Link libraries
target_link_libraries(RTSPProcessor ${OpenCV_LIBS})

# Set properties
set_target_properties(RTSPProcessor PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
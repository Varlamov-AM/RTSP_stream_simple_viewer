cmake_minimum_required(VERSION 3.28)
project(RTSPProcessor)

# Find external libs
find_package(OpenCV 4.6.0 REQUIRED)
find_package(nlohmann_json 3.11.2 REQUIRED)

# Define src directory
file(GLOB APP_SOURCES "src/*.cpp")

# Add executable
add_executable(RTSPProcessor ${APP_SOURCES})

# Include directories
target_include_directories(RTSPProcessor PUBLIC include/)

# Link libraries
target_link_libraries(RTSPProcessor ${OpenCV_LIBS})

# Set properties
set_target_properties(RTSPProcessor PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

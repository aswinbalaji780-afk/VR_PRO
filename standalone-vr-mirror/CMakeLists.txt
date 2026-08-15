cmake_minimum_required(VERSION 3.16)
project(standalone-vr-mirror VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Add the main executable
add_executable(standalone-vr-mirror src/main.cpp)

# Link standard libraries if needed
# target_link_libraries(standalone-vr-mirror PRIVATE ...)

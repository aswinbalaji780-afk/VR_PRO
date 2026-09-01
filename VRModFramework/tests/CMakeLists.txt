cmake_minimum_required(VERSION 3.24)

include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(VRModTests test_math.cpp)
target_include_directories(VRModTests PRIVATE ../InputTranslator/include ../MemoryManager/include)
target_link_libraries(VRModTests PRIVATE Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(VRModTests)

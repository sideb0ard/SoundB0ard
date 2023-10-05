cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
  #set(CMAKE_BUILD_TYPE Debug)
endif()

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_definitions(-Wall -Wextra)
endif(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")


message("YO")

set(CMAKE_CXX_FLAGS_DEBUG "-g")
# set(CMAKE_CXX_FLAGS_RELEASE "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g")

set(CMAKE_CXX_STANDARD 20 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_definitions(-D_WEBSOCKETPP_CPP11_STRICT_)

## MODIFY THESE TO YOUR PATHS

set (AbletonLinkPath "/Users/thorstensideboard/Code/link/")
set (HomebrewIncDir "/opt/homebrew/include/")
set (ReadlineIncDir "/opt/homebrew/opt/readline/include")
set (PerlinIncDir "/Users/thorstensideboard/Code/PerlinNoise")
set (JSON_INC "/opt/homebrew/Cellar/jsoncpp/1.9.5/include")

set (READLINE_LIB "/opt/homebrew/opt/readline/lib/libreadline.dylib")
set (HISTORY_LIB "/opt/homebrew/opt/readline/lib/libhistory.dylib")
set (JSON_LIB "/opt/homebrew/Cellar/jsoncpp/1.9.5/lib/libjsoncpp.dylib")
######################################################################

project(Sbsh VERSION 3.0
  DESCRIPTION "Audio REPL"
  LANGUAGES CXX)

file (GLOB_RECURSE Sbsh_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file (GLOB_RECURSE Sbsh_HEADERS CONFIGURE_DEPENDS "include/*.h")

set (Sbsh_INCLUDE_DIRS "")
foreach (_headerFile ${Sbsh_HEADERS})
  get_filename_component(_dir ${_headerFile} PATH)
  list (APPEND Sbsh_INCLUDE_DIRS ${_dir})
endforeach()
list (REMOVE_DUPLICATES Sbsh_INCLUDE_DIRS)

add_executable(Sbsh ${Sbsh_SOURCES})
# add_executable(Sbsh src/main.cpp src/websocket/web_socket_server.cpp)

find_library(PORTAUDIO_LIB portaudio)
find_library(PORTMIDI_LIB portmidi)
find_library(SNDFILE_LIB sndfile)

target_include_directories(Sbsh PRIVATE ${Sbsh_INCLUDE_DIRS})
target_include_directories(Sbsh PRIVATE ${HomebrewIncDir})
target_include_directories(Sbsh PRIVATE ${ReadlineIncDir})
target_include_directories(Sbsh PRIVATE ${PerlinIncDir})
target_include_directories(Sbsh PRIVATE ${JSON_INC})

get_directory_property (inc_dirs INCLUDE_DIRECTORIES)
message("YOOOO ${INCLUDE_DIRECTORIES}")
list(TRANSFORM inc_dirs PREPEND "-I ")
list(JOIN inc_dirs " " INCLUDE_STRING)

include(${AbletonLinkPath}/AbletonLinkConfig.cmake)
target_link_libraries(Sbsh Ableton::Link)
target_link_libraries(Sbsh ${PORTAUDIO_LIB})
target_link_libraries(Sbsh ${PORTMIDI_LIB})
target_link_libraries(Sbsh ${READLINE_LIB})
target_link_libraries(Sbsh ${HISTORY_LIB})
target_link_libraries(Sbsh ${SNDFILE_LIB})
target_link_libraries(Sbsh ${JSON_LIB})


#################################################################
# TESTS
#
enable_testing()
#
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.13.0
)
FetchContent_MakeAvailable(googletest)
#
add_executable(
  stepper_test
  tests/stepper_test.cpp
  src/stepper.cpp
)

target_include_directories(stepper_test PRIVATE ${Sbsh_INCLUDE_DIRS})
target_include_directories(stepper_test PRIVATE ${HomebrewIncDir})
target_link_libraries(
  stepper_test
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(stepper_test)

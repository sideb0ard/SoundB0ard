cmake_minimum_required(VERSION 3.16)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 20 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_definitions(-D_WEBSOCKETPP_CPP11_STRICT_)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find Ableton Link - try to find it in common locations
set(ABLETON_LINK_PATHS
    "/Users/thorstensideboard/Code/link/"
    "${CMAKE_SOURCE_DIR}/../link/"
    "${CMAKE_SOURCE_DIR}/third_party/link/"
    "/usr/local/include/ableton/"
    "/opt/homebrew/include/ableton/"
)

set(AbletonLinkPath "")
foreach(PATH ${ABLETON_LINK_PATHS})
    if(EXISTS "${PATH}/AbletonLinkConfig.cmake")
        set(AbletonLinkPath ${PATH})
        break()
    endif()
endforeach()

if(NOT AbletonLinkPath)
    message(FATAL_ERROR "Ableton Link not found. Please install it or set AbletonLinkPath manually.")
endif()

# Find PerlinNoise - try to find it in common locations
set(PERLIN_NOISE_PATHS
    "/Users/thorstensideboard/Code/PerlinNoise"
    "${CMAKE_SOURCE_DIR}/../PerlinNoise/"
    "${CMAKE_SOURCE_DIR}/third_party/PerlinNoise/"
    "/usr/local/include/"
    "/opt/homebrew/include/"
)

set(PerlinIncDir "")
foreach(PATH ${PERLIN_NOISE_PATHS})
    if(EXISTS "${PATH}/PerlinNoise.hpp")
        set(PerlinIncDir ${PATH})
        break()
    endif()
endforeach()

if(NOT PerlinIncDir)
    message(FATAL_ERROR "PerlinNoise header not found. Please install it or set PerlinIncDir manually.")
endif()

# Find libraries with proper error checking
find_library(PORTAUDIO_LIB portaudio)
if(NOT PORTAUDIO_LIB)
    message(FATAL_ERROR "PortAudio library not found")
endif()

find_library(PORTMIDI_LIB portmidi)
if(NOT PORTMIDI_LIB)
    message(FATAL_ERROR "PortMidi library not found")
endif()

find_library(SNDFILE_LIB sndfile)
if(NOT SNDFILE_LIB)
    message(FATAL_ERROR "libsndfile library not found")
endif()

# Find readline library - try different approaches
find_library(READLINE_LIB readline PATHS 
    /opt/homebrew/Cellar/readline/*/lib
    /usr/local/lib
    /usr/lib
)

if(NOT READLINE_LIB)
    # Try to find it in the homebrew cellar directly
    file(GLOB READLINE_PATHS "/opt/homebrew/Cellar/readline/*/lib/libreadline.*.dylib")
    if(READLINE_PATHS)
        list(GET READLINE_PATHS 0 READLINE_LIB)
    endif()
endif()

if(NOT READLINE_LIB)
    message(FATAL_ERROR "Readline library not found. Please install with: brew install readline")
endif()

find_library(HISTORY_LIB history PATHS
    /opt/homebrew/Cellar/readline/*/lib
    /usr/local/lib
    /usr/lib
)

if(NOT HISTORY_LIB)
    # Try to find it in the homebrew cellar directly
    file(GLOB HISTORY_PATHS "/opt/homebrew/Cellar/readline/*/lib/libhistory.*.dylib")
    if(HISTORY_PATHS)
        list(GET HISTORY_PATHS 0 HISTORY_LIB)
    endif()
endif()

if(NOT HISTORY_LIB)
    message(FATAL_ERROR "History library not found. Please install with: brew install readline")
endif()

find_library(FFTW3_LIB fftw3)
if(NOT FFTW3_LIB)
    message(FATAL_ERROR "FFTW3 library not found")
endif()

# Use pkg-config for jsoncpp if available, otherwise find manually
pkg_check_modules(JSONCPP jsoncpp)
if(NOT JSONCPP_FOUND)
    find_library(JSONCPP_LIB jsoncpp)
    if(NOT JSONCPP_LIB)
        message(FATAL_ERROR "jsoncpp library not found")
    endif()
    find_path(JSONCPP_INCLUDE_DIR json/json.h)
    if(NOT JSONCPP_INCLUDE_DIR)
        message(FATAL_ERROR "jsoncpp headers not found")
    endif()
endif()

project(Sbsh VERSION 3.0
  DESCRIPTION "Soundb0ard - Interactive Music Making Environment"
  LANGUAGES CXX)

# Find Threads after project declaration
find_package(Threads REQUIRED)

file (GLOB_RECURSE Sbsh_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file (GLOB_RECURSE Sbsh_HEADERS CONFIGURE_DEPENDS "src/*.h")

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
target_include_directories(Sbsh PRIVATE ${PerlinIncDir})

# Add readline include directories - force homebrew version
file(GLOB READLINE_INCLUDE_DIRS "/opt/homebrew/Cellar/readline/*/include")
if(READLINE_INCLUDE_DIRS)
    list(GET READLINE_INCLUDE_DIRS 0 READLINE_INCLUDE_DIR)
    message(STATUS "Found readline include dir: ${READLINE_INCLUDE_DIR}")
    target_include_directories(Sbsh PRIVATE ${READLINE_INCLUDE_DIR})
else()
    message(STATUS "Readline include dir not found, using system default")
endif()

# Add jsoncpp include directories
if(JSONCPP_FOUND)
    target_include_directories(Sbsh PRIVATE ${JSONCPP_INCLUDE_DIRS})
else()
    target_include_directories(Sbsh PRIVATE ${JSONCPP_INCLUDE_DIR})
endif()

# Set compiler flags using target properties
target_compile_options(Sbsh PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-g -O0>
)

include(${AbletonLinkPath}/AbletonLinkConfig.cmake)
target_link_libraries(Sbsh 
    Ableton::Link
    ${PORTAUDIO_LIB}
    ${PORTMIDI_LIB}
    ${READLINE_LIB}
    ${HISTORY_LIB}
    ${SNDFILE_LIB}
    ${FFTW3_LIB}
    Threads::Threads
)

# Link jsoncpp
if(JSONCPP_FOUND)
    target_link_libraries(Sbsh ${JSONCPP_LIBRARIES})
else()
    target_link_libraries(Sbsh ${JSONCPP_LIB})
endif()


# Installation rules
install(TARGETS Sbsh RUNTIME DESTINATION bin)
install(DIRECTORY wavs/ DESTINATION share/sbsh/wavs 
    PATTERN "*.wav" PATTERN "*.aiff" PATTERN "*.aif" PATTERN "*.mp3")
install(DIRECTORY settings/ DESTINATION share/sbsh/settings)
install(FILES startup.sb DESTINATION share/sbsh/)

# Optional tests
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.13.0
    )
    FetchContent_MakeAvailable(googletest)
    
    # Create a common test library for shared dependencies
    add_library(sbsh_test_common INTERFACE)
    target_include_directories(sbsh_test_common INTERFACE ${Sbsh_INCLUDE_DIRS})
    target_include_directories(sbsh_test_common INTERFACE ${PerlinIncDir})
    
    # Add jsoncpp include directories for tests
    if(JSONCPP_FOUND)
        target_include_directories(sbsh_test_common INTERFACE ${JSONCPP_INCLUDE_DIRS})
    else()
        target_include_directories(sbsh_test_common INTERFACE ${JSONCPP_INCLUDE_DIR})
    endif()
    
    add_executable(
        stepper_test
        tests/stepper_test.cpp
        src/stepper.cpp
    )
    
    target_link_libraries(
        stepper_test
        sbsh_test_common
        GTest::gtest_main
    )
    
    include(GoogleTest)
    gtest_discover_tests(stepper_test)
endif()

# CPack configuration for packaging
include(CPack)
set(CPACK_PACKAGE_NAME "Soundb0ard")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Interactive Music Making Environment")
set(CPACK_PACKAGE_VENDOR "Soundb0ard Project")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

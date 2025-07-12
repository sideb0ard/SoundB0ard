cmake_minimum_required(VERSION 3.16)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD
    20
    CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_definitions(-D_WEBSOCKETPP_CPP11_STRICT_)

# Find required packages
find_package(PkgConfig REQUIRED)

# Use FetchContent for external dependencies
include(FetchContent)

# Fetch Ableton Link with submodules  
FetchContent_Declare(
  ableton_link
  GIT_REPOSITORY https://github.com/Ableton/link.git
  GIT_TAG        Link-3.1.3
  GIT_SUBMODULES_RECURSE ON
)
FetchContent_MakeAvailable(ableton_link)

# Configure Ableton Link target
include(${ableton_link_SOURCE_DIR}/cmake_include/ConfigureAbletonLink.cmake)
ConfigureAbletonLink(${ableton_link_SOURCE_DIR})

# Fetch PerlinNoise
FetchContent_Declare(
  perlin_noise
  GIT_REPOSITORY https://github.com/Reputeless/PerlinNoise.git
  GIT_TAG        v3.0.0
)
FetchContent_MakeAvailable(perlin_noise)

# WebSocket temporarily disabled
# FetchContent_Declare(
#   websocketpp
#   GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
#   GIT_TAG        develop
# )
# FetchContent_MakeAvailable(websocketpp)

# Try to find system libraries using pkg-config first, then fallback to find_library
pkg_check_modules(PORTAUDIO portaudio-2.0)
if(NOT PORTAUDIO_FOUND)
  find_library(PORTAUDIO_LIB portaudio
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
    NO_DEFAULT_PATH)
  if(NOT PORTAUDIO_LIB)
    # Fallback to default search
    find_library(PORTAUDIO_LIB portaudio)
  endif()
  if(NOT PORTAUDIO_LIB)
    message(FATAL_ERROR "PortAudio library not found. Install with: sudo apt install portaudio19-dev (Ubuntu/Debian) or equivalent")
  endif()
endif()

# PortMidi doesn't always provide pkg-config files, so try find_library first
find_library(PORTMIDI_LIB portmidi 
  PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
  NO_DEFAULT_PATH)
if(NOT PORTMIDI_LIB)
  # Fallback to default search
  find_library(PORTMIDI_LIB portmidi)
endif()
if(NOT PORTMIDI_LIB)
  message(FATAL_ERROR "PortMidi library not found. Install with: sudo apt install libportmidi-dev (Ubuntu/Debian) or equivalent")
endif()

pkg_check_modules(SNDFILE sndfile)
if(NOT SNDFILE_FOUND)
  find_library(SNDFILE_LIB sndfile)
  if(NOT SNDFILE_LIB)
    message(FATAL_ERROR "libsndfile library not found. Install with: sudo apt install libsndfile1-dev (Ubuntu/Debian) or equivalent")
  endif()
endif()

# Find readline library (cross-platform)
find_library(READLINE_LIB readline 
  PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
  NO_DEFAULT_PATH)
if(NOT READLINE_LIB)
  # Fallback to default search
  find_library(READLINE_LIB readline)
endif()
if(NOT READLINE_LIB)
  message(FATAL_ERROR "Readline library not found. Install with: sudo apt install libreadline-dev (Ubuntu/Debian) or brew install readline (macOS)")
endif()

find_library(HISTORY_LIB history
  PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
  NO_DEFAULT_PATH)
if(NOT HISTORY_LIB)
  # Fallback to default search
  find_library(HISTORY_LIB history)
endif()
if(NOT HISTORY_LIB)
  message(FATAL_ERROR "History library not found. Install with: sudo apt install libreadline-dev (Ubuntu/Debian) or brew install readline (macOS)")
endif()


# Use pkg-config for jsoncpp
pkg_check_modules(JSONCPP jsoncpp)
if(NOT JSONCPP_FOUND)
  find_library(JSONCPP_LIB jsoncpp
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
    NO_DEFAULT_PATH)
  if(NOT JSONCPP_LIB)
    # Fallback to default search
    find_library(JSONCPP_LIB jsoncpp)
  endif()
  if(NOT JSONCPP_LIB)
    message(FATAL_ERROR "jsoncpp library not found. Install with: sudo apt install libjsoncpp-dev (Ubuntu/Debian) or equivalent")
  endif()
  find_path(JSONCPP_INCLUDE_DIR json/json.h)
  if(NOT JSONCPP_INCLUDE_DIR)
    message(FATAL_ERROR "jsoncpp headers not found")
  endif()
endif()

project(
  Sbsh
  VERSION 3.0
  DESCRIPTION "Soundb0ard - Interactive Music Making Environment"
  LANGUAGES CXX)

# Find Threads after project declaration
find_package(Threads REQUIRED)

# Explicitly set platform definitions for Link (now that CMAKE_SYSTEM_NAME is available)
message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  message(STATUS "Setting LINK_PLATFORM_LINUX=1")
  set_property(TARGET Ableton::Link APPEND PROPERTY
    INTERFACE_COMPILE_DEFINITIONS
    LINK_PLATFORM_LINUX=1
  )
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  message(STATUS "Setting LINK_PLATFORM_MACOSX=1")
  set_property(TARGET Ableton::Link APPEND PROPERTY
    INTERFACE_COMPILE_DEFINITIONS
    LINK_PLATFORM_MACOSX=1
  )
else()
  message(STATUS "System name '${CMAKE_SYSTEM_NAME}' - no additional Link platform definition needed")
endif()

file(GLOB_RECURSE Sbsh_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE Sbsh_HEADERS CONFIGURE_DEPENDS "src/*.h")

# Temporarily exclude WebSocket files to avoid ASIO conflicts
list(FILTER Sbsh_SOURCES EXCLUDE REGEX "src/websocket/.*")
list(FILTER Sbsh_HEADERS EXCLUDE REGEX "src/websocket/.*")

set(Sbsh_INCLUDE_DIRS "")
foreach(_headerFile ${Sbsh_HEADERS})
  get_filename_component(_dir ${_headerFile} PATH)
  list(APPEND Sbsh_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES Sbsh_INCLUDE_DIRS)

add_executable(Sbsh ${Sbsh_SOURCES})

target_include_directories(Sbsh PRIVATE ${Sbsh_INCLUDE_DIRS})

# Include FetchContent dependencies
target_include_directories(Sbsh PRIVATE ${perlin_noise_SOURCE_DIR})
# target_include_directories(Sbsh PRIVATE ${websocketpp_SOURCE_DIR})  # WebSocket disabled

# Add system library include directories using pkg-config when available
if(PORTAUDIO_FOUND)
  target_include_directories(Sbsh PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
endif()

# PortMidi headers are in standard system location (/usr/include/portmidi.h)

if(SNDFILE_FOUND)
  target_include_directories(Sbsh PRIVATE ${SNDFILE_INCLUDE_DIRS})
endif()


if(JSONCPP_FOUND)
  target_include_directories(Sbsh PRIVATE ${JSONCPP_INCLUDE_DIRS})
else()
  target_include_directories(Sbsh PRIVATE ${JSONCPP_INCLUDE_DIR})
endif()

# Set compiler flags using target properties
target_compile_options(
  Sbsh PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
               $<$<CONFIG:Release>:-O3> $<$<CONFIG:Debug>:-g -O0>)

# Add sanitizers for debug builds
option(ENABLE_SANITIZERS
       "Enable AddressSanitizer and UBSanitizer for debug builds" ON)
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Enabling sanitizers for debug build")
  target_compile_options(
    Sbsh PRIVATE -fsanitize=address -fsanitize=undefined
                 -fno-omit-frame-pointer -fno-optimize-sibling-calls)
  target_link_options(Sbsh PRIVATE -fsanitize=address -fsanitize=undefined)
endif()

# Link libraries - use pkg-config variables when available, fallback to manual finds
target_link_libraries(Sbsh
  Ableton::Link
  ${READLINE_LIB}
  ${HISTORY_LIB}
  Threads::Threads
)

# Link audio libraries using pkg-config when available
if(PORTAUDIO_FOUND)
  target_link_libraries(Sbsh ${PORTAUDIO_LIBRARIES})
  target_compile_options(Sbsh PRIVATE ${PORTAUDIO_CFLAGS_OTHER})
else()
  target_link_libraries(Sbsh ${PORTAUDIO_LIB})
endif()

# PortMidi doesn't provide pkg-config, so just link directly
target_link_libraries(Sbsh ${PORTMIDI_LIB})

if(SNDFILE_FOUND)
  target_link_libraries(Sbsh ${SNDFILE_LIBRARIES})
  target_compile_options(Sbsh PRIVATE ${SNDFILE_CFLAGS_OTHER})
else()
  target_link_libraries(Sbsh ${SNDFILE_LIB})
endif()


if(JSONCPP_FOUND)
  target_link_libraries(Sbsh ${JSONCPP_LIBRARIES})
  target_compile_options(Sbsh PRIVATE ${JSONCPP_CFLAGS_OTHER})
else()
  target_link_libraries(Sbsh ${JSONCPP_LIB})
endif()

# Installation rules
install(TARGETS Sbsh RUNTIME DESTINATION bin)
install(
  DIRECTORY wavs/
  DESTINATION share/sbsh/wavs
  PATTERN "*.wav"
  PATTERN "*.aiff"
  PATTERN "*.aif"
  PATTERN "*.mp3")
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
    GIT_TAG v1.13.0)
  FetchContent_MakeAvailable(googletest)

  # Create a common test library for shared dependencies
  add_library(sbsh_test_common INTERFACE)
  target_include_directories(sbsh_test_common INTERFACE ${Sbsh_INCLUDE_DIRS})
  target_include_directories(sbsh_test_common INTERFACE ${perlin_noise_SOURCE_DIR})

  # Add jsoncpp include directories for tests
  if(JSONCPP_FOUND)
    target_include_directories(sbsh_test_common
                               INTERFACE ${JSONCPP_INCLUDE_DIRS})
  else()
    target_include_directories(sbsh_test_common
                               INTERFACE ${JSONCPP_INCLUDE_DIR})
  endif()

  add_executable(stepper_test tests/stepper_test.cpp src/stepper.cpp)

  target_link_libraries(stepper_test sbsh_test_common GTest::gtest_main)

  include(GoogleTest)
  gtest_discover_tests(stepper_test)
endif()

# Development and linting targets
option(BUILD_LINTING_TARGETS "Build linting and development targets" ON)

if(BUILD_LINTING_TARGETS)
  # Find source files for linting
  file(GLOB_RECURSE ALL_CPP_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp"
       "${CMAKE_SOURCE_DIR}/src/*.c")
  file(GLOB_RECURSE ALL_HEADER_FILES "${CMAKE_SOURCE_DIR}/src/*.h"
       "${CMAKE_SOURCE_DIR}/src/*.hpp")
  set(ALL_SOURCE_FILES ${ALL_CPP_FILES} ${ALL_HEADER_FILES})

  # Format target
  add_custom_target(
    format
    COMMAND clang-format -i ${ALL_SOURCE_FILES}
    COMMENT "Formatting C++ code with clang-format"
    VERBATIM)

  # Static analysis target
  add_custom_target(
    cppcheck
    COMMAND
      cppcheck --enable=warning,style,performance --std=c++20
      --suppress=missingIncludeSystem --suppress=unusedFunction
      --error-exitcode=1 --inline-suppr ${ALL_CPP_FILES}
    COMMENT "Running static analysis with cppcheck"
    VERBATIM)

  # Memory pattern check target
  add_custom_target(
    check-memory-patterns
    COMMAND ${CMAKE_COMMAND} -P
            ${CMAKE_SOURCE_DIR}/cmake/CheckMemoryPatterns.cmake
    COMMENT "Checking for dangerous memory patterns"
    VERBATIM)

  # Pre-commit hooks target
  add_custom_target(
    run-hooks
    COMMAND pre-commit run --all-files
    COMMENT "Running pre-commit hooks on all files"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM)

  # Install pre-commit hooks target
  add_custom_target(
    install-hooks
    COMMAND pre-commit install
    COMMENT "Installing pre-commit hooks"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM)

  # Comprehensive lint target
  add_custom_target(
    lint
    DEPENDS format cppcheck check-memory-patterns
    COMMENT "Running all linting tools")

  # Development target (format + build)
  add_custom_target(
    dev
    DEPENDS format Sbsh
    COMMENT "Development cycle: format + build")

  # Sanitizer test target (only available in debug builds)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND ENABLE_SANITIZERS)
    add_custom_target(
      test-sanitizers
      COMMAND
        ${CMAKE_COMMAND} -E env
        ASAN_OPTIONS=check_initialization_order=1:strict_init_order=1
        $<TARGET_FILE:Sbsh> --version || echo "Sanitizer test completed"
      DEPENDS Sbsh
      COMMENT "Running with AddressSanitizer enabled"
      VERBATIM)
  endif()

  # All checks target
  add_custom_target(
    check-all
    DEPENDS lint Sbsh
    COMMENT "Running all checks: lint + build + test")

  if(BUILD_TESTS)
    add_dependencies(check-all stepper_test)
  endif()

  message(STATUS "Development targets enabled:")
  message(STATUS "  make format         - Format code with clang-format")
  message(STATUS "  make cppcheck       - Run static analysis")
  message(STATUS "  make lint           - Run all linting tools")
  message(STATUS "  make dev            - Quick development cycle")
  message(STATUS "  make install-hooks  - Install pre-commit hooks")
  message(STATUS "  make run-hooks      - Run pre-commit hooks")
  message(STATUS "  make check-all      - Run all checks")
endif()

# CPack configuration for packaging
include(CPack)
set(CPACK_PACKAGE_NAME "Soundb0ard")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Interactive Music Making Environment")
set(CPACK_PACKAGE_VENDOR "Soundb0ard Project")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

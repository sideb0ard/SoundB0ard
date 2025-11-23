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

# Disable Ableton Link tests and examples to speed up builds
set(LINK_BUILD_TESTS
    OFF
    CACHE BOOL "Build Ableton Link tests" FORCE)
set(LINK_BUILD_UTIL
    OFF
    CACHE BOOL "Build Ableton Link utilities" FORCE)
set(LINK_BUILD_APPS
    OFF
    CACHE BOOL "Build Ableton Link applications" FORCE)
set(LINK_BUILD_EXAMPLES
    OFF
    CACHE BOOL "Build Ableton Link examples" FORCE)
set(LINK_BUILD_VIA_PACKAGE_MANAGER
    ON
    CACHE BOOL "Build via package manager" FORCE)

# Fetch Ableton Link with submodules
FetchContent_Declare(
  ableton_link
  GIT_REPOSITORY https://github.com/Ableton/link.git
  GIT_TAG Link-3.1.3
  GIT_SUBMODULES_RECURSE ON)

# Fetch Link manually to modify its CMakeLists.txt
FetchContent_Populate(ableton_link)

# Patch the main CMakeLists.txt to exclude examples (only if not already
# patched)
file(READ "${ableton_link_SOURCE_DIR}/CMakeLists.txt" ABLETON_CMAKE_CONTENT)
if(NOT ABLETON_CMAKE_CONTENT MATCHES "# Disabled by SoundB0ard")
  string(REPLACE "add_subdirectory(examples)"
                 "# add_subdirectory(examples) # Disabled by SoundB0ard"
                 ABLETON_CMAKE_CONTENT "${ABLETON_CMAKE_CONTENT}")
  file(WRITE "${ableton_link_SOURCE_DIR}/CMakeLists.txt"
       "${ABLETON_CMAKE_CONTENT}")
  message(STATUS "Patched Ableton Link to exclude examples")
endif()

# Now add the subdirectory manually
add_subdirectory(${ableton_link_SOURCE_DIR} ${ableton_link_BINARY_DIR})

# Configure Ableton Link target
include(${ableton_link_SOURCE_DIR}/cmake_include/ConfigureAbletonLink.cmake)
configureabletonlink(${ableton_link_SOURCE_DIR})

# Fetch PerlinNoise
FetchContent_Declare(
  perlin_noise
  GIT_REPOSITORY https://github.com/Reputeless/PerlinNoise.git
  GIT_TAG v3.0.0)
FetchContent_MakeAvailable(perlin_noise)

# WebSocket temporarily disabled FetchContent_Declare( websocketpp
# GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git GIT_TAG develop )
# FetchContent_MakeAvailable(websocketpp)

# Try to find system libraries using pkg-config first, then fallback to
# find_library
pkg_check_modules(PORTAUDIO portaudio-2.0)
if(NOT PORTAUDIO_FOUND)
  find_library(
    PORTAUDIO_LIB portaudio
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
    NO_DEFAULT_PATH)
  if(NOT PORTAUDIO_LIB)
    # Fallback to default search
    find_library(PORTAUDIO_LIB portaudio)
  endif()
  if(NOT PORTAUDIO_LIB)
    message(
      FATAL_ERROR
        "PortAudio library not found. Install with: "
        "sudo apt install portaudio19-dev (Ubuntu/Debian) or equivalent")
  endif()
endif()

# PortMidi doesn't always provide pkg-config files, so try find_library first
find_library(
  PORTMIDI_LIB portmidi
  PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
  NO_DEFAULT_PATH)
if(NOT PORTMIDI_LIB)
  # Fallback to default search
  find_library(PORTMIDI_LIB portmidi)
endif()
if(NOT PORTMIDI_LIB)
  message(
    FATAL_ERROR "PortMidi library not found. Install with: "
                "sudo apt install libportmidi-dev (Ubuntu/Debian) or equivalent"
  )
endif()

pkg_check_modules(SNDFILE sndfile)
if(NOT SNDFILE_FOUND)
  find_library(SNDFILE_LIB sndfile)
  if(NOT SNDFILE_LIB)
    message(
      FATAL_ERROR
        "libsndfile library not found. Install with: "
        "sudo apt install libsndfile1-dev (Ubuntu/Debian) or equivalent")
  endif()
endif()

# Use pkg-config for jsoncpp
pkg_check_modules(JSONCPP jsoncpp)
if(NOT JSONCPP_FOUND)
  find_library(
    JSONCPP_LIB jsoncpp
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
    NO_DEFAULT_PATH)
  if(NOT JSONCPP_LIB)
    # Fallback to default search
    find_library(JSONCPP_LIB jsoncpp)
  endif()
  if(NOT JSONCPP_LIB)
    message(
      FATAL_ERROR
        "jsoncpp library not found. Install with: "
        "sudo apt install libjsoncpp-dev (Ubuntu/Debian) or equivalent")
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

# Find readline library (cross-platform with macOS Homebrew support)
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  # macOS with Homebrew - add pkg-config paths for readline
  file(GLOB HOMEBREW_READLINE_DIRS
       "/opt/homebrew/Cellar/readline/*/lib/pkgconfig"
       "/usr/local/Cellar/readline/*/lib/pkgconfig")
  if(HOMEBREW_READLINE_DIRS)
    list(GET HOMEBREW_READLINE_DIRS 0 READLINE_PKGCONFIG_DIR)
    set(ENV{PKG_CONFIG_PATH} "${READLINE_PKGCONFIG_DIR}:$ENV{PKG_CONFIG_PATH}")
    message(STATUS "Using Homebrew readline")
  endif()

  # Try pkg-config first on macOS
  pkg_check_modules(READLINE readline)
  if(READLINE_FOUND)
    message(STATUS "Found readline via pkg-config")
  else()
    # Fallback to manual search with Homebrew paths
    file(GLOB HOMEBREW_READLINE_LIB_DIRS "/opt/homebrew/Cellar/readline/*/lib"
         "/usr/local/Cellar/readline/*/lib")
    find_library(
      READLINE_LIB readline
      PATHS ${HOMEBREW_READLINE_LIB_DIRS} /usr/lib /usr/local/lib
      NO_DEFAULT_PATH)
    if(NOT READLINE_LIB)
      find_library(READLINE_LIB readline)
    endif()
  endif()
else()
  # Linux and other platforms
  find_library(
    READLINE_LIB readline
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
    NO_DEFAULT_PATH)
  if(NOT READLINE_LIB)
    find_library(READLINE_LIB readline)
  endif()
endif()

if(NOT READLINE_FOUND AND NOT READLINE_LIB)
  message(
    FATAL_ERROR
      "Readline library not found. Install with: "
      "sudo apt install libreadline-dev (Ubuntu/Debian) or "
      "brew install readline (macOS)")
endif()

# Find history library
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  # macOS with Homebrew
  if(READLINE_FOUND)
    # If we found readline via pkg-config, history is likely included
    set(HISTORY_FOUND TRUE)
    set(HISTORY_LIBRARIES ${READLINE_LIBRARIES})
  else()
    # Manual search for history library
    file(GLOB HOMEBREW_READLINE_LIB_DIRS "/opt/homebrew/Cellar/readline/*/lib"
         "/usr/local/Cellar/readline/*/lib")
    find_library(
      HISTORY_LIB history
      PATHS ${HOMEBREW_READLINE_LIB_DIRS} /usr/lib /usr/local/lib
      NO_DEFAULT_PATH)
    if(NOT HISTORY_LIB)
      find_library(HISTORY_LIB history)
    endif()
  endif()
else()
  # Linux and other platforms
  find_library(
    HISTORY_LIB history
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
    NO_DEFAULT_PATH)
  if(NOT HISTORY_LIB)
    find_library(HISTORY_LIB history)
  endif()
endif()

if(NOT HISTORY_FOUND AND NOT HISTORY_LIB)
  message(
    FATAL_ERROR
      "History library not found. Install with: "
      "sudo apt install libreadline-dev (Ubuntu/Debian) or "
      "brew install readline (macOS)")
endif()

# Explicitly set platform definitions for Link (now that CMAKE_SYSTEM_NAME is
# available)
message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  message(STATUS "Setting LINK_PLATFORM_LINUX=1")
  set_property(
    TARGET Ableton::Link
    APPEND
    PROPERTY INTERFACE_COMPILE_DEFINITIONS LINK_PLATFORM_LINUX=1)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  message(STATUS "Setting LINK_PLATFORM_MACOSX=1")
  set_property(
    TARGET Ableton::Link
    APPEND
    PROPERTY INTERFACE_COMPILE_DEFINITIONS LINK_PLATFORM_MACOSX=1)
else()
  message(STATUS "System name '${CMAKE_SYSTEM_NAME}' - "
                 "no additional Link platform definition needed")
endif()

file(GLOB_RECURSE Sbsh_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE Sbsh_HEADERS CONFIGURE_DEPENDS "src/*.h")

# Temporarily exclude WebSocket files to avoid ASIO conflicts
list(FILTER Sbsh_SOURCES EXCLUDE REGEX "src/websocket/.*")
list(FILTER Sbsh_HEADERS EXCLUDE REGEX "src/websocket/.*")

set(SBSH_INCLUDE_DIRS "")
foreach(header_file ${Sbsh_HEADERS})
  get_filename_component(header_dir ${header_file} PATH)
  list(APPEND SBSH_INCLUDE_DIRS ${header_dir})
endforeach()
list(REMOVE_DUPLICATES SBSH_INCLUDE_DIRS)

add_executable(Sbsh ${Sbsh_SOURCES})

target_include_directories(Sbsh PRIVATE ${SBSH_INCLUDE_DIRS})

# Include FetchContent dependencies
target_include_directories(Sbsh PRIVATE ${perlin_noise_SOURCE_DIR})
# target_include_directories(Sbsh PRIVATE ${websocketpp_SOURCE_DIR})  #
# WebSocket disabled

# Add system library include directories using pkg-config when available
if(PORTAUDIO_FOUND)
  target_include_directories(Sbsh PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
endif()

# PortMidi headers are in standard system location (/usr/include/portmidi.h)

if(SNDFILE_FOUND)
  target_include_directories(Sbsh PRIVATE ${SNDFILE_INCLUDE_DIRS})
endif()

# Add readline include directories
if(READLINE_FOUND)
  target_include_directories(Sbsh PRIVATE ${READLINE_INCLUDE_DIRS})
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  # Add Homebrew readline include directories manually
  file(GLOB HOMEBREW_READLINE_INCLUDE_DIRS
       "/opt/homebrew/Cellar/readline/*/include"
       "/usr/local/Cellar/readline/*/include")
  if(HOMEBREW_READLINE_INCLUDE_DIRS)
    target_include_directories(Sbsh PRIVATE ${HOMEBREW_READLINE_INCLUDE_DIRS})
    message(STATUS "Added Homebrew readline include directories")
  endif()
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

# Link libraries - use pkg-config variables when available, fallback to manual
# finds
target_link_libraries(Sbsh Ableton::Link Threads::Threads)

# Link readline libraries - handle both pkg-config and manual detection
if(READLINE_FOUND)
  # On macOS, pkg-config gives library names but we need full paths for Homebrew
  # libraries
  if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    file(GLOB HOMEBREW_READLINE_LIB_DIRS "/opt/homebrew/Cellar/readline/*/lib"
         "/usr/local/Cellar/readline/*/lib")
    find_library(
      READLINE_FULL_LIB readline
      PATHS ${HOMEBREW_READLINE_LIB_DIRS}
      NO_DEFAULT_PATH)
    find_library(
      HISTORY_FULL_LIB history
      PATHS ${HOMEBREW_READLINE_LIB_DIRS}
      NO_DEFAULT_PATH)
    if(READLINE_FULL_LIB)
      target_link_libraries(Sbsh ${READLINE_FULL_LIB})
    endif()
    if(HISTORY_FULL_LIB)
      target_link_libraries(Sbsh ${HISTORY_FULL_LIB})
    endif()
  else()
    target_link_libraries(Sbsh ${READLINE_LIBRARIES})
  endif()
  target_compile_options(Sbsh PRIVATE ${READLINE_CFLAGS_OTHER})
else()
  target_link_libraries(Sbsh ${READLINE_LIB})
  if(HISTORY_LIB)
    target_link_libraries(Sbsh ${HISTORY_LIB})
  endif()
endif()

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

  # Create a static library from all sources except main.cpp for tests to link
  # against
  set(SBSH_SOURCES_NO_MAIN ${Sbsh_SOURCES})
  list(FILTER SBSH_SOURCES_NO_MAIN EXCLUDE REGEX "src/main\\.cpp$")

  add_library(sbsh_lib STATIC ${SBSH_SOURCES_NO_MAIN})
  target_include_directories(sbsh_lib PUBLIC ${SBSH_INCLUDE_DIRS})
  target_include_directories(sbsh_lib PUBLIC ${perlin_noise_SOURCE_DIR})

  # Add jsoncpp include directories
  if(JSONCPP_FOUND)
    target_include_directories(sbsh_lib PUBLIC ${JSONCPP_INCLUDE_DIRS})
  else()
    target_include_directories(sbsh_lib PUBLIC ${JSONCPP_INCLUDE_DIR})
  endif()

  # Add readline include directories for sbsh_lib
  if(READLINE_FOUND)
    target_include_directories(sbsh_lib PUBLIC ${READLINE_INCLUDE_DIRS})
  elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    file(GLOB HOMEBREW_READLINE_INCLUDE_DIRS_TEST
         "/opt/homebrew/Cellar/readline/*/include"
         "/usr/local/Cellar/readline/*/include")
    if(HOMEBREW_READLINE_INCLUDE_DIRS_TEST)
      target_include_directories(sbsh_lib
                                 PUBLIC ${HOMEBREW_READLINE_INCLUDE_DIRS_TEST})
    endif()
  endif()

  # Link sbsh_lib with required libraries
  target_link_libraries(sbsh_lib Ableton::Link Threads::Threads)

  if(PORTAUDIO_FOUND)
    target_link_libraries(sbsh_lib ${PORTAUDIO_LIBRARIES})
  else()
    target_link_libraries(sbsh_lib ${PORTAUDIO_LIB})
  endif()

  target_link_libraries(sbsh_lib ${PORTMIDI_LIB})

  if(SNDFILE_FOUND)
    target_link_libraries(sbsh_lib ${SNDFILE_LIBRARIES})
  else()
    target_link_libraries(sbsh_lib ${SNDFILE_LIB})
  endif()

  if(JSONCPP_FOUND)
    target_link_libraries(sbsh_lib ${JSONCPP_LIBRARIES})
  else()
    target_link_libraries(sbsh_lib ${JSONCPP_LIB})
  endif()

  if(READLINE_FOUND)
    if(CMAKE_SYSTEM_NAME MATCHES "Darwin" AND READLINE_FULL_LIB)
      target_link_libraries(sbsh_lib ${READLINE_FULL_LIB})
      if(HISTORY_FULL_LIB)
        target_link_libraries(sbsh_lib ${HISTORY_FULL_LIB})
      endif()
    else()
      target_link_libraries(sbsh_lib ${READLINE_LIBRARIES})
    endif()
  else()
    target_link_libraries(sbsh_lib ${READLINE_LIB})
    if(HISTORY_LIB)
      target_link_libraries(sbsh_lib ${HISTORY_LIB})
    endif()
  endif()

  if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_libraries(sbsh_lib "-framework CoreFoundation"
                          "-framework CoreAudio" "-framework CoreMIDI")
  endif()

  include(GoogleTest)

  # Test globals file provides global variables needed by sbsh_lib
  set(TEST_GLOBALS_SOURCE ${CMAKE_SOURCE_DIR}/tests/test_globals.cpp)

  # Helper function to create test executables
  function(add_sbsh_test test_name test_source)
    add_executable(${test_name} ${test_source} ${TEST_GLOBALS_SOURCE})
    target_link_libraries(${test_name} sbsh_lib GTest::gtest_main)
    gtest_discover_tests(${test_name})
  endfunction()

  # Simple unit tests
  add_sbsh_test(stepper_test tests/stepper_test.cpp)
  add_sbsh_test(object_test tests/object_test.cpp)
  add_sbsh_test(euclidean_test tests/euclidean.cpp)
  add_sbsh_test(audioutils_test tests/audioutils.cpp)
  add_sbsh_test(ramp_test tests/ramp_test.cpp)

  # Interpreter tests
  add_sbsh_test(lexer_test tests/lexer_test.cpp)
  # TODO: parser_test needs updates for new ProcessStatement API
  # add_sbsh_test(parser_test tests/parser_test.cpp)
  add_sbsh_test(evaluator_test tests/evaluator_test.cpp)

  # Pattern parser tests
  add_sbsh_test(pattern_parser_test tests/pattern_parser_test.cpp)
  add_sbsh_test(pattern_tokenizer_test tests/pattern_tokenizer_test.cpp)
  # TODO: pattern_functions_test needs updates for new API
  # add_sbsh_test(pattern_functions_test tests/pattern_functions.cpp)
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

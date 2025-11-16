# CheckMemoryPatterns.cmake Script to check for dangerous memory patterns in C++
# code

message(STATUS "Checking for dangerous memory patterns...")

# Find all C++ source files
file(GLOB_RECURSE CPP_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp"
     "${CMAKE_SOURCE_DIR}/src/*.c")

# Define dangerous patterns
set(DANGEROUS_PATTERNS "strcpy\\s*\\(" "strcat\\s*\\(" "sprintf\\s*\\("
                       "gets\\s*\\(")

set(FOUND_ISSUES FALSE)

# Check each file for dangerous patterns
foreach(source_file ${CPP_FILES})
  foreach(pattern ${DANGEROUS_PATTERNS})
    file(READ ${source_file} FILE_CONTENTS)

    # Use regex to find dangerous patterns
    string(REGEX MATCH ${pattern} MATCH_RESULT ${FILE_CONTENTS})

    if(MATCH_RESULT)
      message(
        WARNING "Dangerous function found in ${source_file}: ${MATCH_RESULT}")
      message(WARNING "Consider using safer alternatives:")
      message(WARNING "  strcpy  -> strncpy")
      message(WARNING "  strcat  -> strncat")
      message(WARNING "  sprintf -> snprintf")
      message(WARNING "  gets    -> fgets")
      set(FOUND_ISSUES TRUE)
    endif()
  endforeach()
endforeach()

if(FOUND_ISSUES)
  message(
    FATAL_ERROR "Dangerous memory patterns found! Please fix before continuing."
  )
else()
  message(STATUS "✅ No dangerous memory patterns found")
endif()

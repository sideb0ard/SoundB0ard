#include <unistd.h>

#include <filereader.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
bool starts_with_comment(std::string line) {
  std::size_t first_char_pos = line.find_first_not_of(" \t");

  if (first_char_pos != std::string::npos && line[first_char_pos] == '#') {
    return true;
  }

  return false;
}

std::string strip_inline_comment(std::string line) {
  // Find the first '#' that's not inside a string and is after a semicolon
  // (to distinguish from pattern syntax like "p31 # kick_comp")
  bool in_string = false;
  char string_char = '\0';
  size_t last_semicolon = std::string::npos;

  for (size_t i = 0; i < line.length(); i++) {
    char c = line[i];

    // Track if we're inside a string literal
    if (!in_string && (c == '"' || c == '\'')) {
      in_string = true;
      string_char = c;
    } else if (in_string && c == string_char &&
               (i == 0 || line[i - 1] != '\\')) {
      in_string = false;
      string_char = '\0';
    }

    // Track last semicolon position (outside strings)
    if (!in_string && c == ';') {
      last_semicolon = i;
    }

    // If we find a '#' outside of a string AND after a semicolon, strip from
    // there
    if (!in_string && c == '#') {
      // Only treat as comment if it comes after a semicolon
      if (last_semicolon != std::string::npos && i > last_semicolon) {
        return line.substr(0, i);
      }
      // Otherwise, it might be pattern syntax like "p31 # kick_comp"
      // so we keep it
    }
  }

  return line;
}
}  // namespace

std::string ReadFileContents(std::string filepath) {
  std::ifstream ifs(filepath);
  if (ifs.is_open()) {
    std::stringstream buffer;

    std::string line;
    while (getline(ifs, line)) {
      if (!starts_with_comment(line)) {
        // Strip inline comments (# at end of line)
        std::string cleaned_line = strip_inline_comment(line);
        buffer << cleaned_line;
      }
    }

    return buffer.str();
  }
  return "";
}

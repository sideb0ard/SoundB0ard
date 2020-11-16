#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include <filereader.hpp>

namespace
{
bool starts_with_comment(std::string line)
{
    std::size_t first_char_pos = line.find_first_not_of(" \t");

    if (line[first_char_pos] == '#')
        return true;

    return false;
}
} // namespace

std::string ReadFileContents(std::string filepath)
{
    std::ifstream ifs(filepath);
    if (ifs.is_open())
    {
        std::stringstream buffer;

        std::string line;
        while (getline(ifs, line))
        {
            if (!starts_with_comment(line))
            {
                std::cout << line << std::endl;
                buffer << line;
            }
        }

        return buffer.str();
    }
    else
    {
        return "File didnae open!";
    }
}

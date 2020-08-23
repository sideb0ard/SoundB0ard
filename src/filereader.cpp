#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include <filereader.hpp>

std::string ReadFileContents(std::string filename)
{
    char cwd[1024];
    getcwd(cwd, 1023);
    std::string full_filename = std::string(cwd) + "/" + filename;
    std::cout << "Opening " << full_filename << std::endl;

    std::ifstream ifs(full_filename);
    if (ifs.is_open())
    {
        std::stringstream buffer;
        buffer << ifs.rdbuf();

        return buffer.str();
    }
    else
    {
        return "File didnae open!";
    }
}

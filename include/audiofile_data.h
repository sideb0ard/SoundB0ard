#pragma once

#include <string>

struct audiofile_data
{
    std::string filename;
    double *filecontents{nullptr};
    int samplecount;
    int channels;
};

void audiofile_data_import_file_contents(audiofile_data *afd,
                                         std::string filename);

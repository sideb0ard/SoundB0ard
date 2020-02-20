#pragma once

struct audiofile_data
{
    char filename[2048];
    double *filecontents{nullptr};
    int samplecount;
    int channels;
};

void audiofile_data_import_file_contents(audiofile_data *afd, char *filename);

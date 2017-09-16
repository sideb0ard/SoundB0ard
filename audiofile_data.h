#pragma once

typedef struct audiofile_data
{
    char filename[2048];
    double *filecontents;
    unsigned int filelength;
    unsigned int samplerate;
    unsigned int samplecount;
    unsigned int channels;
} audiofile_data;

void audiofile_data_import_file_contents(audiofile_data *afd, char *filename);

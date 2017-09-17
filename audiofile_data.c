#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "audiofile_data.h"

void audiofile_data_import_file_contents(audiofile_data *afd, char *filename)
{
    SNDFILE *snd_file;
    SF_INFO sf_info;

    char cwd[1024];
    getcwd(cwd, 1024);
    char full_filename[strlen(cwd) + 7 /* '/wavs/' is 6 and 1 for null */ +
                       strlen(filename)];
    strcpy(full_filename, cwd);
    strcat(full_filename, "/wavs/");
    strcat(full_filename, filename);

    sf_info.format = 0;
    snd_file = sf_open(full_filename, SFM_READ, &sf_info);
    if (!snd_file)
    {
        printf("Barfed opening %s : %d", full_filename, sf_error(snd_file));
        return;
    }

    int audio_buffer_len = sf_info.channels * sf_info.frames;
    double *audio_buffer = (double *)calloc(audio_buffer_len, sizeof(double));
    if (audio_buffer == NULL)
    {
        perror("deid!\n");
        sf_close(snd_file);
        return;
    }
    if (afd->filecontents) // already have old contents
        free(afd->filecontents);

    afd->filecontents = audio_buffer;

    // done with actual file now, close it
    sf_readf_double(snd_file, afd->filecontents, audio_buffer_len);
    sf_close(snd_file);

    // set meta info - filename
    memset(afd->filename, 0, sizeof(afd->filename));
    strncpy(afd->filename, full_filename, 2047);
    // set meta info - samplecount etc...
    afd->samplecount = audio_buffer_len;

}

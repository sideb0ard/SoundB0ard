#include "audiofile_data.h"

#include <sndfile.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <string>

void audiofile_data_import_file_contents(audiofile_data *afd,
                                         std::string filename) {
  SNDFILE *snd_file;
  SF_INFO sf_info;

  char cwd[1024];
  getcwd(cwd, 1024);
  std::string full_filename = std::string(cwd) + "/wavs/" + filename;

  sf_info.format = 0;
  snd_file = sf_open(full_filename.c_str(), SFM_READ, &sf_info);
  if (!snd_file) {
    std::cerr << "Barfed opening " << full_filename << " : "
              << sf_error(snd_file);
    return;
  }

  int audio_buffer_len = sf_info.channels * sf_info.frames;
  double *audio_buffer = (double *)calloc(audio_buffer_len, sizeof(double));
  if (audio_buffer == NULL) {
    perror("deid!\n");
    sf_close(snd_file);
    return;
  }
  if (afd->filecontents)  // already have old contents
    free(afd->filecontents);

  afd->filecontents = audio_buffer;

  // done with actual file now, close it
  sf_readf_double(snd_file, afd->filecontents, audio_buffer_len);
  sf_close(snd_file);

  afd->filename = full_filename;
  afd->samplecount = audio_buffer_len;
  afd->channels = sf_info.channels;
}

#include <stdio.h>
#include "digisynth_voice.h"

void digisynth_voice_init(digisynth_voice *dv, char *filename)
{
    printf("DIGISYNTH VOICE BEING CALLED!\n");
    sampleosc_init(&dv->m_osc1, filename);
}

void digisynth_voice_open_wav(digisynth_voice *dv, char *filename)
{
}

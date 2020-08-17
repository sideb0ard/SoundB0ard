#include "digisynth_voice.h"
#include <stdio.h>

#include <iostream>

void digisynth_voice_init(digisynth_voice *dv, std::string filename)
{
    std::cout << "DIGISYNTH VOICE INNNNIT!" << filename << std::endl;

    // initializes 4 x envelope generators and
    // 2 x lfos, and dca in base class
    voice_init(&dv->m_voice);

    sampleosc_init(&dv->m_osc1, filename);
    // attach oscillators to my base class
    dv->m_voice.m_osc1 = (oscillator *)&dv->m_osc1;

    eg_set_eg_mode(&dv->m_voice.m_eg1, ANALOG);
    dv->m_voice.m_eg1.m_output_eg = true;

    dca_initialize(&dv->m_voice.m_dca);
    dv->m_voice.m_dca.m_mod_source_eg = DEST_DCA_EG;
}

void digisynth_voice_open_wav(digisynth_voice *dv, char *filename)
{
    sampleosc_init(&dv->m_osc1, filename);
}

bool digisynth_voice_gennext(digisynth_voice *dv, double *out_left,
                             double *out_right)
{

    if (!voice_gennext(&dv->m_voice, out_left, out_right))
    {
        return false;
    }

    ////// update layer 1 modulators
    eg_update(&dv->m_voice.m_eg1);
    // osc_update((oscillator *)&msv->m_voice.m_lfo1);

    ////// gen next val layer 1 mods
    double env_out = eg_do_envelope(&dv->m_voice.m_eg1, NULL);
    // lfo_do_oscillate((oscillator *)&msv->m_voice.m_lfo1, NULL);

    dca_update(&dv->m_voice.m_dca);
    // moog_update((filter *)&msv->m_moog_ladder_filter);

    sampleosc_update((oscillator *)&dv->m_osc1);
    double osc_out =
        sampleosc_do_oscillate((oscillator *)&dv->m_osc1, NULL) * env_out;

    dca_gennext(&dv->m_voice.m_dca, osc_out, osc_out, out_left, out_right);

    return true;
}

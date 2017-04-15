#include "synthdrum_sequencer.h"
#include <stdlib.h>

synthdrum_sequencer *new_synthdrum_seq(void)
{
    printf("New Drum Synth!\n");
    synthdrum_sequencer *sds = calloc(1, sizeof(synthdrum_sequencer));

    osc_new_settings(&sds->m_osc1.osc);
    qb_set_soundgenerator_interface(&sds->m_osc1);
    sds->m_osc1.osc.m_waveform = NOISE;

    osc_new_settings(&sds->m_osc2.osc);
    qb_set_soundgenerator_interface(&sds->m_osc2);
    sds->m_osc2.osc.m_waveform = SINE;

    filter_moog_init(&sds->m_filter);

    envelope_generator_init(&sds->m_eg1);
    envelope_generator_init(&sds->m_eg2);

    sds->sg.gennext = &sds_gennext;
    sds->sg.status = &sds_status;
    sds->sg.getvol = &sds_getvol;
    sds->sg.setvol = &sds_setvol;

    return sds;
}

void sds_status(void *self, wchar_t *ss) { printf("fine!\n"); }

void sds_setvol(void *self, double v) { return; }

double sds_gennext(void *self)
{
    printf("HI!\n");
    return 0.0;
}

double sds_getvol(void *self) { return 0.0; }

void sds_trigger(synthdrum_sequencer *sds)
{
    eg_start_eg(&sds->m_eg1);
    eg_start_eg(&sds->m_eg2);

    osc_reset(&sds->m_osc1.osc);
    sds->m_osc1.osc.m_note_on = true;

    osc_reset(&sds->m_osc2.osc);
    sds->m_osc2.osc.m_note_on = true;
}

void sds_stop(synthdrum_sequencer *sds)
{
    sds->m_osc1.osc.m_note_on = false;
    sds->m_osc2.osc.m_note_on = false;
}

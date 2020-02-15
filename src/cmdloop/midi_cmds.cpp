#include <stdlib.h>
#include <string.h>

#include <drumsynth.h>
#include <midi_cmds.h>
#include <midimaaan.h>

extern mixer *mixr;

void midi_launch_init(mixer *mixr)
{
    if (!mixr->have_midi_controller)
    {
        pthread_t midi_th;
        if (pthread_create(&midi_th, NULL, midi_init, NULL))
        {
            fprintf(stderr, "Errrr, wit tha midi..\n");
        }
        pthread_detach(midi_th);
    }
}

void midi_set_destination(mixer *mixr, int soundgen_num)
{
    if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
    {
        if (mixr->SoundGenerators[soundgen_num]->IsSynth())
        {
            mixr->midi_control_destination = MIDI_CONTROL_SYNTH_TYPE;
            mixr->active_midi_soundgen_num = soundgen_num;
        }
        else if (mixr->SoundGenerators[soundgen_num]->type == DRUMSYNTH_TYPE)
        {
            mixr->midi_control_destination = MIDI_CONTROL_DRUMSYNTH_TYPE;
            mixr->active_midi_soundgen_num = soundgen_num;
        }
    }
}

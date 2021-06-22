#include <stdlib.h>
#include <string.h>

#include <drumsynth.h>
#include <midi_cmds.h>
#include <midimaaan.h>

extern Mixer *mixr;

void midi_launch_init(Mixer *mixr)
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

void midi_set_destination(Mixer *mixr, int soundgen_num)
{
    if (mixr->IsValidSoundgenNum(soundgen_num))
    {
        if (mixr->sound_generators_[soundgen_num]->IsSynth())
        {
            mixr->midi_control_destination = MIDI_CONTROL_SYNTH_TYPE;
            mixr->active_midi_soundgen_num = soundgen_num;
        }
        else if (mixr->sound_generators_[soundgen_num]->type == DRUMSYNTH_TYPE)
        {
            mixr->midi_control_destination = MIDI_CONTROL_DRUMSYNTH_TYPE;
            mixr->active_midi_soundgen_num = soundgen_num;
        }
    }
}

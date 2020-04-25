#include <math.h>
#include <portmidi.h>
#include <porttime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#include <defjams.h>
#include <digisynth.h>
#include <drumsampler.h>
#include <drumsynth.h>
#include <dxsynth.h>
#include <midi_freq_table.h>
#include <midimaaan.h>
#include <minisynth.h>
#include <mixer.h>
#include <utils.h>

extern mixer *mixr;

extern char *s_synth_waves[6];

// static void print_midi_event_rec(midi_event ev)
//{
//    printf("[Midi] note: %d\n", ev.data1);
//}

void *midi_init(void *)
{
    printf("MIDI maaaaan!\n");
    pthread_setname_np("Midimaaaan");

    PmError retval = Pm_Initialize();
    if (retval != pmNoError)
        printf("Err running Pm_Initialize: %s\n", Pm_GetErrorText(retval));

    int cnt;
    const PmDeviceInfo *info;

    int dev = 0;

    if ((cnt = Pm_CountDevices()))
    {
        for (int i = 0; i < cnt; i++)
        {
            info = Pm_GetDeviceInfo(i);
            if (info->input && (strncmp(info->name, "MPKmini2", 8) == 0))
            {
                dev = i;
                strncpy(mixr->midi_controller_name, info->name, 127);
                break;
            }
        }
    }
    else
    {
        Pm_Terminate();
        return NULL;
    }

    retval = Pm_OpenInput(&mixr->midi_stream, dev, NULL, 512L, NULL, NULL);
    if (retval != pmNoError)
    {
        printf("Err opening input for MPKmini2: %s\n", Pm_GetErrorText(retval));
        Pm_Terminate();
        return NULL;
    }

    mixr->have_midi_controller = true;

    return NULL;
}

midi_event new_midi_event(unsigned int event_type, unsigned int data1,
                          unsigned int data2)
{
    midi_event ev = {.event_type = event_type,
                     .data1 = data1,
                     .data2 = data2,
                     .delete_after_use = false};
    return ev;
}

std::ostream &operator<<(std::ostream &out, const midi_event &ev)
{
    out << "MIDIEVENT:"
        << "Type:" << ev.event_type << " D1:" << ev.data1 << " D2:" << ev.data2
        << " Delete:" << (ev.delete_after_use ? "true" : "false");
    return out;
}

void midi_pattern_quantize(midi_pattern *pattern)
{
    printf("Quantizzzzzzing\n");

    midi_pattern quantized_loop = {};

    for (int i = 0; i < PPBAR; i++)
    {
        midi_event ev = (*pattern)[i];
        if (ev.event_type)
        {
            int amendedtick = 0;
            int tickdiv16 = i / PPSIXTEENTH;
            int lower16th = tickdiv16 * PPSIXTEENTH;
            int upper16th = lower16th + PPSIXTEENTH;
            if ((i - lower16th) < (upper16th - i))
                amendedtick = lower16th;
            else
                amendedtick = upper16th;

            quantized_loop[amendedtick] = ev;
            printf("Amended TICK: %d\n", amendedtick);
        }
    }

    for (int i = 0; i < PPBAR; i++)
        (*pattern)[i] = quantized_loop[i];
}

void midi_pattern_print(midi_event *loop)
{
    for (int i = 0; i < PPBAR; i++)
    {
        midi_event ev = loop[i];
        if (ev.event_type)
        {
            char type[20] = {0};
            switch (ev.event_type)
            {
            case (144):
                strcpy(type, "note_on");
                break;
            case (128):
                strcpy(type, "note_off");
                break;
            case (176):
                strcpy(type, "midi_control");
                break;
            case (224):
                strcpy(type, "pitch_bend");
                break;
            default:
                strcpy(type, "no_idea");
                break;
            }
            printf("[Tick: %5d] - note: %4d velocity: %4d type: %s "
                   "delete_after_use: %s\n",
                   i, ev.data1, ev.data2, type,
                   ev.delete_after_use ? "true" : "false");
        }
    }
}
void midi_event_cp(midi_event *from, midi_event *to)
{
    to->event_type = from->event_type;
    to->data1 = from->data1;
    to->data2 = from->data2;
    to->delete_after_use = from->delete_after_use;
}

void midi_event_clear(midi_event *ev) { memset(ev, 0, sizeof(midi_event)); }

void midi_event_print(midi_event *ev)
{
    char event_type[10] = {};
    switch (ev->event_type)
    {
    case (144):
        strncpy(event_type, "ON", 9);
        break;
    case (128):
        strncpy(event_type, "OFF", 9);
        break;
    case (176):
        strncpy(event_type, "CONTROL", 9);
        break;
    case (224):
        strncpy(event_type, "PITCHBEND", 9);
        break;
    }
    printf("EVENT! type:%s data1:%d data2:%d delete?%s\n", event_type,
           ev->data1, ev->data2, ev->delete_after_use ? "true" : "false");
}

int get_midi_note_from_string(char *string)
{
    if (strlen(string) > 4)
    {
        printf("DINGIE!\n");
        return -1;
    }
    char note[3] = {0};
    int octave = 0; // default if none provided
    sscanf(string, "%[a-z#]%d", note, &octave);
    note[2] = 0; // safety

    // convert octave to midi semitones
    octave = 12 + (octave * 12);

    // printf("MIDI NOTE:%s %d \n", note, octave);
    //// twelve semitones:
    //// C C#/Db D D#/Eb E F F#/Gb G G#/Ab A A#/Bb B
    ////
    int midinotenum = -1;
    if (!strcasecmp("c", note))
        midinotenum = 0 + octave;
    else if (!strcasecmp("c#", note) || !strcasecmp("db", note) ||
             !strcasecmp("dm", note))
        midinotenum = 1 + octave;
    else if (!strcasecmp("d", note))
        midinotenum = 2 + octave;
    else if (!strcasecmp("d#", note) || !strcasecmp("eb", note) ||
             !strcasecmp("em", note))
        midinotenum = 3 + octave;
    else if (!strcasecmp("e", note))
        midinotenum = 4 + octave;
    else if (!strcasecmp("f", note))
        midinotenum = 5 + octave;
    else if (!strcasecmp("f#", note) || !strcasecmp("gb", note) ||
             !strcasecmp("gm", note))
        midinotenum = 6 + octave;
    else if (!strcasecmp("g", note))
        midinotenum = 7 + octave;
    else if (!strcasecmp("g#", note) || !strcasecmp("ab", note) ||
             !strcasecmp("am", note))
        midinotenum = 8 + octave;
    else if (!strcasecmp("a", note))
        midinotenum = 9 + octave;
    else if (!strcasecmp("a#", note) || !strcasecmp("bb", note) ||
             !strcasecmp("bm", note))
        midinotenum = 10 + octave;
    else if (!strcasecmp("b", note))
        midinotenum = 11 + octave;
    // printf("MIDI NOTE NUM:%d \n", midinotenum);

    return midinotenum;
}
int get_midi_note_from_mixer_key(unsigned int key, int octave)
{
    int midi_octave = 12 + (octave * 12);
    return key + midi_octave;
}

void midi_pattern_set_velocity(midi_event *pattern, unsigned int midi_tick,
                               unsigned int velocity)
{
    if (!pattern)
    {
        printf("Dingie, gimme a REAL pattern!\n");
        return;
    }

    if (midi_tick < PPBAR && velocity < 128)
        pattern[midi_tick].data2 = velocity;
    else
        printf("Nae valid!? Midi_tick:%d // velocity:%d\n", midi_tick,
               velocity);
}

void midi_pattern_rand_amp(midi_event *pattern)
{
    for (int i = 0; i < PPBAR; i++)
    {
        if (pattern[i].event_type == MIDI_ON)
            pattern[i].data2 = rand() % 127;
    }
}

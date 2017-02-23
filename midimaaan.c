#include <math.h>
#include <portmidi.h>
#include <porttime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "midi_freq_table.h"
#include "midimaaan.h"
#include "minisynth.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;

void *midiman()
{
    printf("MIDI maaaaan!\n");
    Pm_Initialize();

    int cnt;
    const PmDeviceInfo *info;

    int dev;

    if ((cnt = Pm_CountDevices())) {
        for (int i = 0; i < cnt; i++) {
            info = Pm_GetDeviceInfo(i);
            if (info->input && (strncmp(info->name, "MPKmini2", 8) == 0)) {
                dev = i;
            }
        }
    }
    else {
        printf(
            "No MIDI inputs detected. If you think there should be, run 'midi' "
            "to try and reinitialize. (NOT YET IMPLEMENTED\n"); // TODO
                                                                // implement
        return NULL;
    }

    PortMidiStream *mstream;
    PmEvent msg[32];
    PmError retval;

    retval = Pm_OpenInput(&mstream, dev, NULL, 512L, NULL, NULL);
    if (retval != pmNoError)
        printf("Err opening input for MPKmini2: %s\n", Pm_GetErrorText(retval));
    while (1) {
        if (Pm_Poll(mstream)) {
            cnt = Pm_Read(mstream, msg, 32);
            for (int i = 0; i < cnt; i++) {
                int status = Pm_MessageStatus(msg[i].message);
                int data1 = Pm_MessageData1(msg[i].message);
                int data2 = Pm_MessageData2(msg[i].message);

                if (mixr->midi_control_destination == SYNTH) {

                    minisynth *ms =
                        (minisynth *)mixr
                            ->sound_generators[mixr->active_midi_soundgen_num];

                    if (ms->recording) {
                        int tick = mixr->tick % PPNS;
                        midi_event *ev =
                            new_midi_event(tick, status, data1, data2);
                        print_midi_event_rec(ev);
                        // TODO - maybe something better than just add a tick?
                        // perhaps 16 x ? or be able to add additional events to a midiEvent
                        while (ms->melodies[ms->cur_melody][tick] != NULL)
                            tick++;
                        ms->melodies[ms->cur_melody][tick] = ev;
                    }

                    midi_event ev;
                    ev.event_type = status;
                    ev.data1 = data1;
                    ev.data2 = data2;
                    print_midi_event_rec(&ev);
                    midi_parse_midi_event(ms, &ev);
                }
                else if (mixr->midi_control_destination == DELAYFX) {
                    printf("MIDI CONTROLS! DELAY\n");
                    EFFECT *d =
                        mixr->sound_generators[mixr->active_midi_soundgen_num]
                            ->effects[mixr->active_midi_soundgen_effect_num];
                    switch (status) {
                    case (176): {
                        midi_delay_control(d, data1, data2);
                    }
                    }
                }
                else {
                    printf("Got midi but not connected to synth\n");
                }
            }
        }
    }
    Pm_Close(mstream);
    Pm_Terminate();

    return NULL;
}

void print_midi_event_rec(midi_event *ev)
{
    printf("[Midi] %d note: %d\n", ev->tick, ev->data1);
}

midi_event *new_midi_event(int tick, int event_type, int data1, int data2)
{
    midi_event *ev = calloc(1, sizeof(midi_event));
    if (ev == NULL) {
        printf("BIG PROBS MATE\n");
        return NULL;
    }
    ev->tick = tick;
    ev->event_type = event_type;
    ev->data1 = data1;
    ev->data2 = data2;

    return ev;
}

void midi_delay_control(EFFECT *e, int data1, int data2)
{
    if (e->type != DELAY) {
        printf("OOft, mate, i'm no a delay - cannae help you out\n");
        return;
    }
    stereodelay *d = e->delay;

    double scaley_val;
    switch (data1) {
    case 1:
        scaley_val = scaleybum(1, 128, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
        printf("OPTION1!\n");
        break;
    case 2:
        scaley_val = scaleybum(1, 128, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
        printf("OPTION2!\n");
        break;
    case 3:
        scaley_val = scaleybum(1, 128, 0, 1, data2);
        printf("OPTION3!\n");
        break;
    case 4:
        scaley_val = scaleybum(1, 128, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
        printf("OPTION4!\n");
        break;
    case 5:
        scaley_val = scaleybum(0, 128, 0, 2000, data2);
        printf("DELAY TIME! %f\n", scaley_val);
        delay_set_delay_time_ms(d, scaley_val);
        break;
    case 6:
        // scaley_val = scaleybum(0, 128, -100, 100, data2);
        scaley_val = scaleybum(0, 128, 20, 100, data2);
        printf("DELAY FEEDBACK! %f\n", scaley_val);
        delay_set_feedback_percent(d, scaley_val);
        break;
    case 7:
        scaley_val = scaleybum(1, 128, -0.9, 0.9, data2);
        printf("DELAY RATIO! %f\n", scaley_val);
        delay_set_delay_ratio(d, scaley_val);
        break;
    case 8:
        scaley_val = scaleybum(1, 128, 0, 1, data2);
        printf("DELAY MIX! %f\n", scaley_val);
        delay_set_wet_mix(d, scaley_val);
        break;
    default:
        printf("SOMthing else\n");
    }
}

void midi_parse_midi_event(minisynth *ms, midi_event *ev)
{
    switch (ev->event_type) {
    case (144): { // Hex 0x80
        minisynth_midi_note_on(ms, ev->data1, ev->data2);
        break;
    }
    case (128): { // Hex 0x90
        minisynth_midi_note_off(ms, ev->data1, ev->data2, true);
        break;
    }
    case (176): { // Hex 0xB0
        minisynth_midi_control(ms, ev->data1, ev->data2);
        break;
    }
    case (224): { // Hex 0xE0
        minisynth_midi_pitchbend(ms, ev->data1, ev->data2);
        break;
    }
    default:
        printf("HERE PAL, I've NAE IDEA WHIT KIND OF MIDI EVENT THAT WiS\n");
    }
    if (ev->delete_after_use) {
        free(ev);
        ms->melodies[ms->cur_melody][ev->tick] = NULL;
    }
}

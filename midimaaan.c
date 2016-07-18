#include <math.h>
#include <portmidi.h>
#include <porttime.h>
#include <stdio.h>
#include <string.h>

#include "bpmrrr.h"
#include "defjams.h"
#include "midi_freq_table.h"
#include "midimaaan.h"
#include "mixer.h"
#include "oscil.h"
#include "utils.h"

extern bpmrrr *b;
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
                // double timestamp = msg[i].timestamp/1000.;

                // printf("HAS ACTIVE FM? %d\n", mixr->has_active_fm);
                if (mixr->has_active_fm) {
                    switch (status) {
                    case (144): { // Hex 0x80
                        midinoteon(data1, data2);
                        break;
                    }
                    case (128): { // Hex 0x90
                        midinoteoff(data1, data2);
                        break;
                    }
                    case (176): { // Hex 0xB0
                        midicontrol(data1, data2);
                        break;
                    }
                    case (224): { // Hex 0xE0
                        midipitchbend(data1, data2);
                        break;
                    }
                    default:
                        printf("SOMETHING ELSE\n");
                    }
                }
                else {
                    printf("Got midi but not connected to FM\n");
                }
            }
        }
    }
    Pm_Close(mstream);
    Pm_Terminate();

    return NULL;
}

void midinoteon(unsigned int midinote, int velocity)
{
    double freq = midi_freq_table[midinote];
    (void)velocity;
    // TODO : put this somewhere else
    FM *fm = (FM *)mixr->sound_generators[mixr->active_fm_soundgen_num];
    set_midi_note_num(fm->osc1, midinote);
    set_midi_note_num(fm->osc2, midinote);
    keypress_on(fm, freq);
}

void midinoteoff(unsigned int midinote, int velocity)
{
    (void)velocity;
    // printf("Note OFF! note: %d velocity: %d\n", midinote, velocity);
    // keypress_off(mixr->sound_generators[mixr->active_fm_soundgen_num]);
    FM *fm = (FM *)mixr->sound_generators[mixr->active_fm_soundgen_num];
    if (midinote == fm->osc1->m_midi_note_number) {
        note_off(fm->env);
    }
}

void midipitchbend(int data1, int data2)
{
    printf("Pitch bend, babee: %d %d\n", data1, data2);
    int actual_pitch_bent_val = (int)((data1 & 0x7F) | ((data2 & 0x7F) << 7));

    FM *fm = (FM *)mixr->sound_generators[mixr->active_fm_soundgen_num];
    if (actual_pitch_bent_val != 8192) {
        // double normalized_pitch_bent_val =
        //    (float)(actual_pitch_bent_val - 0x2000) / (float)(0x2000);
        // printf("Actzl: %d and norm %f\n", actual_pitch_bent_val,
        //       normalized_pitch_bent_val);
        double scaley_val =
            // scaleybum(0, 16383, -100, 100, normalized_pitch_bent_val);
            scaleybum(0, 16383, -600, 600, actual_pitch_bent_val);
        printf("Cents to bend - %f\n", scaley_val);
        fm->osc1->m_cents = scaley_val;
        fm->osc2->m_cents = scaley_val + 2.5;
    }
    else {
        fm->osc1->m_cents = 0;
        fm->osc2->m_cents = 2.5;
    }
}

void midicontrol(int data1, int data2)
{
    printf("MIDI Mind Control! %d %d\n", data1, data2);
    FM *fm = (FM *)mixr->sound_generators[mixr->active_fm_soundgen_num];
    double scaley_val;
    switch (data1) {
    case 1: // K1 - Envelope Attack Time Msec
        scaley_val = scaleybum(0, 128, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
        set_attack_time_msec(fm->env, scaley_val);
        break;
    case 2: // K2 - Envelope Decay Time Msec
        scaley_val = scaleybum(0, 128, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
        set_attack_time_msec(fm->env, scaley_val);
        break;
    case 3: // K3 - Envelope Sustain Level
        scaley_val = scaleybum(0, 128, 0, 1, data2);
        set_sustain_level(fm->env, scaley_val);
        break;
    case 4: // K4 - Envelope Release Time Msec
        scaley_val = scaleybum(0, 128, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
        set_release_time_msec(fm->env, scaley_val);
        break;
    case 5: // K5 - LFO rate
        scaley_val = scaleybum(0, 128, MIN_LFO_RATE, MAX_LFO_RATE, data2);
        set_freq(fm->lfo, scaley_val);
        break;
    case 6: // K6 - LFO amplitude
        scaley_val = scaleybum(0, 128, 0.0, 1.0, data2);
        oscil_setvol(fm->lfo, scaley_val);
        break;
    case 7: // K7 - Filter Frequency Cut
        scaley_val = scaleybum(0, 128, FILTER_FC_MIN, FILTER_FC_MAX, data2);
        printf("FILTER CUTOFF! %f\n", scaley_val);
        filter_set_fc_control(fm->filter->bc_filter, scaley_val);
        break;
    case 8: // K8 - Filter Q control
        scaley_val = scaleybum(0, 128, 1, 10, data2);
        printf("FILTER Q control! %f\n", scaley_val);
        filter_set_q_control(fm->filter->bc_filter, scaley_val);
        break;
    default:
        printf("SOMthing else\n");
    }
}

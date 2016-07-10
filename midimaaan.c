#include <portmidi.h>
#include <porttime.h>
#include <stdio.h>
#include <string.h>

#include "midimaaan.h"
#include "oscil.h"
#include "midi_freq_table.h"
#include "bpmrrr.h"
#include "mixer.h"
#include "defjams.h"
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
        for (int i=0; i<cnt; i++) {
            info = Pm_GetDeviceInfo(i);
            if (info->input && (strncmp(info->name, "MPKmini2", 8) == 0)) {
                dev = i;
            }

        }
    } else {
        printf("No MIDI inputs detected. If you think there should be, run 'midi' to try and reinitialize. (NOT YET IMPLEMENTED\n"); // TODO implement
        return NULL;
    }

    PortMidiStream *mstream;
    PmEvent msg[32];
    PmError retval;

    retval = Pm_OpenInput(&mstream, dev, NULL, 512L, NULL, NULL);
    if (retval != pmNoError)
        printf("Err opening input for MPKmini2: %s\n", Pm_GetErrorText(retval));
    while (1) {
        if(Pm_Poll(mstream)) {
            cnt = Pm_Read(mstream, msg, 32);
            for (int i = 0; i < cnt; i++) {
                int status = Pm_MessageStatus(msg[i].message);
                int data1 = Pm_MessageData1(msg[i].message);
                int data2 = Pm_MessageData2(msg[i].message);
                //double timestamp = msg[i].timestamp/1000.;

                //printf("HAS ACTIVE FM? %d\n", mixr->has_active_fm);
                if ( mixr->has_active_fm ) {
                    switch(status) {
                        case(144): { // Hex 0x80
                                       midinoteon(data1, data2);
                                       break;
                                   }
                        case(128): { // Hex 0x90
                                       midinoteoff(data1, data2);
                                       break;
                                   }
                        case(176): {  // Hex 0xB0
                                       midicontrol(data1, data2);
                                       break;
                                   }
                        case(224): {  // Hex 0xE0
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

void midinoteon(int midinote, int velocity) {
    double freq = midi_freq_table[midinote];
    // TODO : put this somewhere else
    FM *fm = (FM*) mixr->sound_generators[mixr->active_fm_soundgen_num];
    set_midi_note_num(fm->osc1, midinote);
    set_midi_note_num(fm->osc2, midinote);
    keypress_on(mixr->sound_generators[mixr->active_fm_soundgen_num], freq);
}

void midinoteoff(int midinote, int velocity) {
    //printf("Note OFF! note: %d velocity: %d\n", midinote, velocity);
    //keypress_off(mixr->sound_generators[mixr->active_fm_soundgen_num]);
    FM *fm = (FM*) mixr->sound_generators[mixr->active_fm_soundgen_num];
    if ( midinote == fm->osc1->m_midi_note_number ) {
        note_off(fm->env);
    }
}

void midipitchbend(int data1, int data2) {
    printf("Pitch bend, babee: %d %d\n", data1, data2);
}

void midicontrol(int data1, int data2) {
    printf("MIDI Mind Control! %d %d\n", data1, data2);
    FM *fm = (FM*) mixr->sound_generators[mixr->active_fm_soundgen_num];
    double scaley_val;
    switch(data1) {
        case 1: // K1 - Envelope Attack Time Msec
            printf("ENv attck time!\n");
            scaley_val = scaleybum(0, 128, 
                                   EG_MINTIME_MS, EG_MAXTIME_MS,
                                   data2);
            set_attack_time_msec(fm->env, scaley_val);
            break;
        case 2: // K2 - Envelope Decay Time Msec
            printf("ENv decay time!\n");
            scaley_val = scaleybum(0, 128, 
                                   EG_MINTIME_MS, EG_MAXTIME_MS,
                                   data2);
            set_attack_time_msec(fm->env, scaley_val);
            break;
        case 3: // K3 - Envelope Sustain Level
            printf("ENv sustain level !\n");
            scaley_val = scaleybum(0, 128, 
                                   0, 1,
                                   data2);
            set_sustain_level(fm->env, scaley_val);
            break;
        case 4: // K4 - Envelope Release Time Msec
            printf("ENv attck time!\n");
            scaley_val = scaleybum(0, 128, 
                                   EG_MINTIME_MS, EG_MAXTIME_MS,
                                   data2);
            set_release_time_msec(fm->env, scaley_val);
            break;
        case 5: // K5 - LFO rate
            printf("LFO rate!\n");
            scaley_val = scaleybum(0, 128, 
                                   MIN_LFO_RATE, MAX_LFO_RATE,
                                   data2);
            set_freq(fm->lfo, scaley_val);
            break;
        case 6: // K6 - LFO amplitude
            printf("LFO amp!\n");
            scaley_val = scaleybum(0, 128, 
                                   0.0, 1.0,
                                   data2);
            //set_freq(fm->lfo, scaley_val);
            oscil_setvol(fm->lfo, scaley_val);
            break;
        case 7: // K5 - Filter Frequency Cut ogg
            printf("Filter Freq Cut!!\n");
            scaley_val = scaleybum(0, 128, 
                                   FILTER_FC_MIN, FILTER_FC_MAX,
                                   data2);
            //set_freq(fm->lfo, scaley_val);
            filter_set_fc_control(fm->filter->bc_filter, scaley_val);
            break;
        default:
            printf("SOMthing else\n");
    }
}

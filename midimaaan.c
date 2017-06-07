#include <math.h>
#include <portmidi.h>
#include <porttime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "looper.h"
#include "midi_freq_table.h"
#include "midimaaan.h"
#include "minisynth.h"
#include "mixer.h"
#include "sample_sequencer.h"
#include "spork.h"
#include "synthdrum_sequencer.h"
#include "utils.h"

extern mixer *mixr;

void *midiman()
{
    pthread_setname_np("Midimaaaan");

    Pm_Initialize();

    int cnt;
    const PmDeviceInfo *info;

    int dev = 0;

    if ((cnt = Pm_CountDevices())) {
        for (int i = 0; i < cnt; i++) {
            info = Pm_GetDeviceInfo(i);
            if (info->input && (strncmp(info->name, "MPKmini2", 8) == 0)) {
                dev = i;
            }
        }
    }
    else {
        return NULL;
    }
    printf("MIDI maaaaan!\n");

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

                if (mixr->debug_mode)
                    printf("[MIDI message] status:%d data1:%d data2:%d\n",
                           status, data1, data2);

                if (mixr->midi_control_destination == SYNTH) {

                    minisynth *ms =
                        (minisynth *)mixr
                            ->sound_generators[mixr->active_midi_soundgen_num];

                    if (ms->recording) {
                        int tick = mixr->midi_tick % PPNS;
                        midi_event *ev =
                            new_midi_event(tick, status, data1, data2);
                        minisynth_add_event(ms, ms->cur_melody, ev);
                    }

                    midi_event ev;
                    ev.event_type = status;
                    ev.data1 = data1;
                    ev.data2 = data2;
                    ev.delete_after_use = false;
                    midi_parse_midi_event(ms, &ev);
                }
                else if (mixr->midi_control_destination == DELAYFX) {
                    printf("MIDI CONTROLS! DELAY\n");
                    fx *d =
                        mixr->sound_generators[mixr->active_midi_soundgen_num]
                            ->effects[mixr->active_midi_soundgen_effect_num];
                    switch (status) {
                    case (176): {
                        midi_delay_control(d, data1, data2);
                    }
                    }
                }
                else if (mixr->midi_control_destination == MIDILOOPER) {
                    printf("LOOPER MIDI CONTROL!\n");
                    looper *l =
                        (looper *)mixr
                            ->sound_generators[mixr->active_midi_soundgen_num];
                    looper_parse_midi(l, data1, data2);
                }
                else if (mixr->midi_control_destination == MIDISEQUENCER) {
                    printf("SAMPLE SEQUENCER MIDI CONTROL!\n");
                    sample_sequencer *s =
                        (sample_sequencer *)mixr
                            ->sound_generators[mixr->active_midi_soundgen_num];
                    sample_seq_parse_midi(s, data1, data2);
                }
                else if (mixr->midi_control_destination == MIDISPORK) {
                    printf("MIDI CONTROLS! SPORK\n");
                    spork *s =
                        (spork *)mixr
                            ->sound_generators[mixr->active_midi_soundgen_num];
                    spork_parse_midi(s, data1, data2);
                }
                else if (mixr->midi_control_destination == MIDISYNTHDRUM) {
                    printf("MIDI CONTROLS! SYNTHDRUM\n");
                    synthdrum_sequencer *sds =
                        (synthdrum_sequencer *)mixr
                            ->sound_generators[mixr->active_midi_soundgen_num];
                    sds_parse_midi(sds, status, data1, data2);
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
    midi_event *ev = (midi_event *)calloc(1, sizeof(midi_event));
    if (ev == NULL) {
        printf("BIG PROBS MATE\n");
        return NULL;
    }
    ev->tick = tick;
    ev->event_type = event_type;
    ev->data1 = data1;
    ev->data2 = data2;
    ev->delete_after_use = false;

    return ev;
}

void midi_event_free(midi_event *ev) { free(ev); }

void spork_parse_midi(spork *s, int data1, int data2)
{
    printf("SPORKMIDIiii!\n");
    double scaley_val = 0.0;
    switch (data1) {
    case 1:
        scaley_val = scaleybum(0, 127, 0.0, 1.0, data2);
        printf("VOLUME!\n");
        s->m_volume = scaley_val;
        break;
    case 2:
        scaley_val = scaleybum(0, 127, MIN_LFO_RATE, MAX_LFO_RATE, data2);
        s->m_lfo.osc.m_osc_fo = scaley_val;
        printf("LFO RATE! %f\n", s->m_lfo.osc.m_osc_fo);

        break;
    case 3:
        scaley_val = scaleybum(0, 127, 0, 1, data2);
        s->m_lfo.osc.m_amplitude = scaley_val;
        printf("LFO AMP!! %f\n", s->m_lfo.osc.m_amplitude);
        break;
    case 4:
        scaley_val = scaleybum(0, 127, 10, 220, data2);
        printf("OSC FREQ!!\n");
        s->m_osc1.osc.m_osc_fo = scaley_val;
        s->m_osc2.osc.m_osc_fo = scaley_val * 2;
        s->m_osc3.osc.m_osc_fo = scaley_val / 3;
        break;
    case 5:
        scaley_val = scaleybum(0, 128, FILTER_FC_MIN, FILTER_FC_MAX, data2);
        printf("FILTER FC! %f\n", scaley_val);
        s->m_filter.f.m_fc_control = scaley_val;
        break;
    case 6:
        scaley_val = scaleybum(0, 127, 0, 100, data2);
        printf("Reverb Pre Delay Ms!\n");
        s->m_reverb->m_pre_delay_msec = scaley_val;
        break;
    case 7:
        scaley_val = scaleybum(0, 127, 0, 5000, data2);
        printf("Reverb Time!\n");
        s->m_reverb->m_rt60 = scaley_val;
        break;
    case 8:
        scaley_val = scaleybum(0, 127, 0, 100, data2);
        printf("Reverb Wet Mix!\n");
        s->m_reverb->m_wet_pct = scaley_val;
        break;
    default:
        printf("SOMthing else\n");
    }
}

void midi_delay_control(fx *e, int data1, int data2)
{
    if (e->type != DELAY && e->type != MODDELAY && e->type != REVERB) {
        printf("OOft, mate, i'm no an accepted FX - cannae help you out\n");
        return;
    }

    double scaley_val;
    //if (e->type == DELAY) {
    //    stereodelay *d = e->delay;
    //    switch (data1) {
    //    case 1:
    //        scaley_val = scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
    //        printf("OPTION1!\n");
    //        break;
    //    case 2:
    //        scaley_val = scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
    //        printf("OPTION2!\n");
    //        break;
    //    case 3:
    //        scaley_val = scaleybum(0, 127, 0, 1, data2);
    //        printf("OPTION3!\n");
    //        break;
    //    case 4:
    //        scaley_val = scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
    //        printf("OPTION4!\n");
    //        break;
    //    case 5:
    //        scaley_val = scaleybum(0, 127, 0, 2000, data2);
    //        printf("DELAY TIME! %f\n", scaley_val);
    //        stereo_delay_set_delay_time_ms(d, scaley_val);
    //        break;
    //    case 6:
    //        // scaley_val = scaleybum(0, 128, -100, 100, data2);
    //        scaley_val = scaleybum(0, 127, 20, 100, data2);
    //        printf("DELAY FEEDBACK! %f\n", scaley_val);
    //        stereo_delay_set_feedback_percent(d, scaley_val);
    //        break;
    //    case 7:
    //        scaley_val = scaleybum(0, 127, -0.9, 0.9, data2);
    //        printf("DELAY RATIO! %f\n", scaley_val);
    //        stereo_delay_set_delay_ratio(d, scaley_val);
    //        break;
    //    case 8:
    //        scaley_val = scaleybum(0, 127, 0, 1, data2);
    //        printf("DELAY MIX! %f\n", scaley_val);
    //        stereo_delay_set_wet_mix(d, scaley_val);
    //        break;
    //    default:
    //        printf("SOMthing else\n");
    //    }
    //}
    //else if (e->type == MODDELAY) {
    //    mod_delay *d = e->moddelay;
    //    switch (data1) {
    //    case 1:
    //        scaley_val = scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
    //        printf("OPTION1!\n");
    //        break;
    //    case 2:
    //        scaley_val = scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
    //        printf("OPTION2!\n");
    //        break;
    //    case 3:
    //        scaley_val = scaleybum(0, 127, 0, 1, data2);
    //        printf("OPTION3!\n");
    //        break;
    //    case 4:
    //        scaley_val = scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
    //        printf("OPTION4!\n");
    //        break;
    //    case 5:
    //        scaley_val = scaleybum(0, 128, 0, 100, data2);
    //        printf("MOD DELAY DDEPTH ! %f\n", scaley_val);
    //        d->m_depth = scaley_val;
    //        break;
    //    case 6:
    //        // scaley_val = scaleybum(0, 128, -100, 100, data2);
    //        scaley_val = scaleybum(0, 128, 0.02, 5.0, data2);
    //        printf("MOD DELAY RATE ! %f\n", scaley_val);
    //        d->m_rate = scaley_val;
    //        break;
    //    case 7:
    //        scaley_val = scaleybum(0, 127, -100, 100, data2);
    //        printf("MOD DELAY FEEDBACK PCT! %f\n", scaley_val);
    //        d->m_feedback_percent = scaley_val;
    //        break;
    //    case 8:
    //        scaley_val = scaleybum(0, 127, 0, 30, data2);
    //        printf("MODDELAY CHORUS OFFSET! %f\n", scaley_val);
    //        d->m_chorus_offset = scaley_val;
    //        break;
    //    default:
    //        printf("SOMthing else\n");
    //    }
    //}
    //else if (e->type == REVERB) {
    //    reverb *r = e->r;
    //    switch (data1) {
    //    case 1:
    //        scaley_val = scaleybum(0, 127, 0, 100, data2);
    //        printf("Reverb Pre Delay Ms!\n");
    //        r->m_pre_delay_msec = scaley_val;
    //        break;
    //    case 2:
    //        scaley_val = scaleybum(0, 127, -96, 0, data2);
    //        printf("Reverb Pre Delay Atten DB!\n");
    //        r->m_pre_delay_atten_db = scaley_val;
    //        break;
    //    case 3:
    //        scaley_val = scaleybum(0, 127, 0, 5000, data2);
    //        printf("Reverb Time!\n");
    //        r->m_rt60 = scaley_val;
    //        break;
    //    case 4:
    //        scaley_val = scaleybum(0, 127, 0, 100, data2);
    //        printf("Reverb Wet Mix!\n");
    //        r->m_wet_pct = scaley_val;
    //        break;
    //    case 5:
    //        scaley_val = scaleybum(0, 128, 0, 1, data2);
    //        printf("Reverb Bandwidth - Input Diffusion! %f\n", scaley_val);
    //        r->m_input_lpf_g = scaley_val;
    //        break;
    //    case 6:
    //        scaley_val = scaleybum(0, 128, 0, 100, data2);
    //        printf("Reverb Input Diffusion Delay ms! %f\n", scaley_val);
    //        r->m_apf_1_delay_msec = scaley_val;
    //        break;
    //    case 7:
    //        scaley_val = scaleybum(0, 127, -1, 1, data2);
    //        printf("Reverb Input Diffusion APF 1 co-efficient ! %f\n",
    //               scaley_val);
    //        r->m_apf_1_g = scaley_val;
    //        break;
    //    case 8:
    //        scaley_val = scaleybum(0, 127, 0, 100, data2);
    //        printf("Reverb Input APF 2 delay msec ! %f\n", scaley_val);
    //        r->m_apf_2_delay_msec = scaley_val;
    //        break;
    //    default:
    //        printf("SOMthing else\n");
    //    }
    //    reverb_cook_variables(r);
    //}
}

void midi_parse_midi_event(minisynth *ms, midi_event *ev)
{
    switch (ev->event_type) {
    case (144): { // Hex 0x80
        ms->m_last_midi_note = ev->data1;
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
        ms->melodies[ms->cur_melody][ev->tick] = NULL;
        if (mixr->debug_mode)
            printf("DELETing TEMP TICK! %d note: %d\n", ev->tick, ev->data1);
        free(ev);
    }
}

void midi_melody_quantize(midi_event **melody)
{
    printf("Quantizzzzzzing\n");
    for (int i = 0; i < PPNS; i++) {
        if (melody[i]) {
            int tick = melody[i]->tick;
            int amendedtick = 0;
            printf("TICK NOM: %d\n", melody[i]->tick);
            int tickdiv16 = tick / PPSIXTEENTH;
            int lower16th = tickdiv16 * PPSIXTEENTH;
            int upper16th = lower16th + PPSIXTEENTH;
            if ((tick - lower16th) < (upper16th - tick))
                amendedtick = lower16th;
            else
                amendedtick = upper16th;

            // TODO - do i need a mutex or protection here - melody[tick] is
            // being read from other thread
            melody[i]->tick = amendedtick;
            printf("Amended TICK: %d\n", amendedtick);
        }
    }
}

void midi_melody_print(midi_event **melody)
{
    for (int i = 0; i < PPNS; i++) {
        if (melody[i]) {
            int tick = melody[i]->tick;
            int data1 = melody[i]->data1;
            int data2 = melody[i]->data2;
            int typeint = melody[i]->event_type;
            char type[20] = {0};
            switch (typeint) {
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
            int delete = melody[i]->delete_after_use;
            printf("[Tick: %5d] - note: %4d velocity: %4d type: %s "
                   "delete_after_use: %s\n",
                   tick, data1, data2, type, delete ? "true" : "false");
        }
    }
}

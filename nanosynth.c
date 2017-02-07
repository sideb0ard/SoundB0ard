#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "filter_ckthreefive.h"
#include "filter_moogladder.h"
#include "filter_onepole.h"
#include "filter_sem.h"
#include "lfo.h"
#include "midi_freq_table.h"
#include "mixer.h"
#include "nanosynth.h"
#include "qblimited_oscillator.h"
#include "table.h"
#include "utils.h"
#include "wt_oscillator.h"

extern wtable *wave_tables[5];
extern mixer *mixr;
extern const wchar_t *sparkchars;

nanosynth *new_nanosynth()
{
    nanosynth *ns;
    ns = (nanosynth *)calloc(1, sizeof(nanosynth));
    if (ns == NULL)
        return NULL;

    for (int i = 0; i < PPNS; i++) {
        ns->melodies[ns->cur_melody][i] = NULL;
    }
    for (int i = 0; i < MAX_NUM_MIDI_LOOPS; i++) {
        ns->melody_multiloop_count[i] = 1;
    }

    ns->g_modmatrix = new_modmatrix();

    matrixrow *row = NULL;
    // LFO -> ALL OSC FO
    row = create_matrix_row(SOURCE_LFO1, DEST_ALL_OSC_FO,
                            &ns->m_default_mod_intensity,
                            &ns->m_osc_fo_mod_range, TRANSFORM_NONE, true);
    add_matrix_row(ns->g_modmatrix, row);

    // LFO1 -> FILTER1 Fc
    row = create_matrix_row(SOURCE_LFO1, DEST_ALL_FILTER_FC,
                            &ns->m_default_mod_intensity,
                            &ns->m_filter_mod_range, TRANSFORM_NONE, false);
    add_matrix_row(ns->g_modmatrix, row);

    // EG1 -> ALL OSC FO
    row = create_matrix_row(SOURCE_BIASED_EG1, DEST_ALL_OSC_FO,
                            &ns->m_eg1_osc_intensity, &ns->m_osc_fo_mod_range,
                            TRANSFORM_NONE, true);
    add_matrix_row(ns->g_modmatrix, row);

    // EG1 -> FILTER1 FC
    row = create_matrix_row(SOURCE_BIASED_EG1, DEST_ALL_FILTER_FC,
                            &ns->m_default_mod_intensity,
                            &ns->m_filter_mod_range, TRANSFORM_NONE, true);
    add_matrix_row(ns->g_modmatrix, row);

    // EG1 -> DCA EG
    row = create_matrix_row(SOURCE_EG1, DEST_DCA_EG, &ns->m_eg1_dca_intensity,
                            &ns->m_default_mod_range, TRANSFORM_NONE, true);
    add_matrix_row(ns->g_modmatrix, row);

    // NOTE NUMBER -> FILTER FC CONTROL
    row = create_matrix_row(SOURCE_MIDI_NOTE_NUM, DEST_ALL_FILTER_KEYTRACK,
                            &ns->m_filter_keytrack_intensity,
                            &ns->m_default_mod_range,
                            TRANSFORM_NOTE_NUMBER_TO_FREQUENCY, true);
    add_matrix_row(ns->g_modmatrix, row);

    // VELOCITY -> DCA VEL
    row = create_matrix_row(SOURCE_VELOCITY, DEST_DCA_VELOCITY,
                            &ns->m_default_mod_intensity,
                            &ns->m_default_mod_range, TRANSFORM_NONE, true);
    add_matrix_row(ns->g_modmatrix, row);

    // PITCHBEND -> PITCHBEND
    row = create_matrix_row(
        SOURCE_PITCHBEND, DEST_ALL_OSC_FO, &ns->m_default_mod_intensity,
        &ns->m_osc_fo_pitchbend_mod_range, TRANSFORM_NONE, true);
    add_matrix_row(ns->g_modmatrix, row);

    // MIDI Vol CC07
    row = create_matrix_row(SOURCE_MIDI_VOLUME_CC07, DEST_DCA_AMP,
                            &ns->m_default_mod_intensity, &ns->m_amp_mod_range,
                            TRANSFORM_INVERT_MIDI_NORMALIZE, true);
    add_matrix_row(ns->g_modmatrix, row);

    // MIDI Pan CC10
    row = create_matrix_row(
        SOURCE_MIDI_PAN_CC10, DEST_DCA_PAN, &ns->m_default_mod_intensity,
        &ns->m_default_mod_range, TRANSFORM_MIDI_TO_PAN, false);
    add_matrix_row(ns->g_modmatrix, row);

    // MIDI Sustain Pedal
    row = create_matrix_row(SOURCE_SUSTAIN_PEDAL, DEST_ALL_EG_SUSTAIN_OVERRIDE,
                            &ns->m_default_mod_intensity,
                            &ns->m_default_mod_range, TRANSFORM_MIDI_SWITCH,
                            false);
    add_matrix_row(ns->g_modmatrix, row);

    // VELOCITY -> EG ATTACK SOURCE_VELOCITY
    // 0 velocity -> scalar = 1, normal attack time
    // 128 velocity -> scalar = 0, fastest (0) attack time:
    row = create_matrix_row(SOURCE_VELOCITY, DEST_ALL_EG_ATTACK_SCALING,
                            &ns->m_default_mod_intensity,
                            &ns->m_default_mod_range, TRANSFORM_MIDI_NORMALIZE,
                            true);
    add_matrix_row(ns->g_modmatrix, row);

    // NOTE NUMBER -> EG DECAY SCALING
    row = create_matrix_row(SOURCE_MIDI_NOTE_NUM, DEST_ALL_EG_DECAY_SCALING,
                            &ns->m_default_mod_intensity,
                            &ns->m_default_mod_range, TRANSFORM_MIDI_NORMALIZE,
                            true);
    add_matrix_row(ns->g_modmatrix, row);

    ns->g_modmatrix->m_sources[SOURCE_MIDI_VOLUME_CC07] = 127;
    ns->g_modmatrix->m_sources[SOURCE_MIDI_PAN_CC10] = 64;

#ifdef DEBUG
    printf("NUM ROWS GLOBAL MOD %d\n",
           ns->g_modmatrix->m_num_rows_in_matrix_core);
#endif

    // end mod matrix setup ///////////////////////////////////

    ns->m_default_mod_intensity = 1.0;
    ns->m_default_mod_range = 1.0;

    ns->m_osc_fo_mod_range = OSC_FO_MOD_RANGE;
    ns->m_filter_mod_range = FILTER_FC_MOD_RANGE;

    ns->m_osc_fo_pitchbend_mod_range = OSC_PITCHBEND_MOD_RANGE;
    ns->m_amp_mod_range = AMP_MOD_RANGE;

    ns->m_eg1_dca_intensity = 1.0;
    ns->m_eg1_osc_intensity = 0.0;

    for (int i = 0; i < MAX_VOICES; i++) {

        ns->m_voices[i].g_modmatrix = new_modmatrix();
        set_matrix_core(ns->m_voices[i].g_modmatrix,
                        get_matrix_core(ns->g_modmatrix));

        // Objects ////////////////////////////////////////////
        ns->m_voices[i].osc1 = (oscillator *)qb_osc_new();
        ns->m_voices[i].osc2 = (oscillator *)qb_osc_new();
        ns->m_voices[i].osc2->m_cents = 2.5;

        ns->m_voices[i].lfo = (oscillator *)lfo_new();

        ns->m_voices[i].eg1 = new_envelope_generator();

        // ns->m_voices[i]->f = (filter *) new_filter_onepole();
        // ns->m_voices[i]->f = (filter *) new_filter_sem();
        // ns->m_voices[i]->f = (filter *)new_filter_ck35();
        ns->m_voices[i].f = (filter *)new_filter_moog();

        ns->m_voices[i].dca = new_dca();
        // ENd Objects //////////////////////////////////////////

        // mmmmmMatrix ///////////////////////
        ns->m_voices[i].osc1->g_modmatrix = ns->m_voices[i].g_modmatrix;
        ns->m_voices[i].osc1->m_mod_source_fo = DEST_OSC1_FO;
        ns->m_voices[i].osc1->m_mod_source_amp = DEST_OSC1_OUTPUT_AMP;

        ns->m_voices[i].osc2->g_modmatrix = ns->m_voices[i].g_modmatrix;
        ns->m_voices[i].osc2->m_mod_source_fo = DEST_OSC2_FO;
        ns->m_voices[i].osc2->m_mod_source_amp = DEST_OSC2_OUTPUT_AMP;

        ns->m_voices[i].f->g_modmatrix = ns->m_voices[i].g_modmatrix;
        ns->m_voices[i].f->m_mod_source_fc = DEST_FILTER1_FC;
        ns->m_voices[i].f->m_mod_source_fc_control = DEST_ALL_FILTER_KEYTRACK;

        // modulators - they write their outputs into
        // what will be a Source for something else

        ns->m_voices[i].lfo->g_modmatrix = ns->m_voices[i].g_modmatrix;
        ns->m_voices[i].lfo->m_mod_dest_output1 = SOURCE_LFO1;
        ns->m_voices[i].lfo->m_mod_dest_output2 = SOURCE_LFO1Q;

        ns->m_voices[i].eg1->g_modmatrix = ns->m_voices[i].g_modmatrix;
        ns->m_voices[i].eg1->m_mod_dest_eg_output = SOURCE_EG1;
        ns->m_voices[i].eg1->m_mod_dest_eg_biased_output = SOURCE_BIASED_EG1;
        ns->m_voices[i].eg1->m_mod_source_eg_attack_scaling =
            DEST_EG1_ATTACK_SCALING;
        ns->m_voices[i].eg1->m_mod_source_eg_decay_scaling =
            DEST_EG1_DECAY_SCALING;
        ns->m_voices[i].eg1->m_mod_source_sustain_override =
            DEST_EG1_SUSTAIN_OVERRIDE;

        ns->m_voices[i].dca->g_modmatrix = ns->m_voices[i].g_modmatrix;
        ns->m_voices[i].dca->m_mod_source_eg = DEST_DCA_EG;
        ns->m_voices[i].dca->m_mod_source_amp_db = DEST_NONE;
        ns->m_voices[i].dca->m_mod_source_velocity = DEST_DCA_VELOCITY;
        ns->m_voices[i].dca->m_mod_source_pan = DEST_DCA_PAN;
        ns->m_voices[i].dca->m_gain = 0.7;

        ns->m_pending_midi_note[i] = -1;
        ns->m_pending_midi_velocity[i] = -1;
    }

    ns->vol = 0.7;
    ns->cur_octave = 0;
    ns->sustain = 0;
    ns->num_melodies = 1;

    ns->m_filter_keytrack = false;
    ns->m_filter_keytrack_intensity = 0.5;

    ns->sound_generator.gennext = &nanosynth_gennext;
    ns->sound_generator.status = &nanosynth_status;
    ns->sound_generator.setvol = &nanosynth_setvol;
    ns->sound_generator.getvol = &nanosynth_getvol;
    ns->sound_generator.type = NANOSYNTH_TYPE;

    // start loop player running
    pthread_t melody_looprrr;
    if (pthread_create(&melody_looprrr, NULL, play_melody_loop, ns)) {
        fprintf(stderr, "Err running loop\n");
    }
    else {
        pthread_detach(melody_looprrr);
    }

    ns->last_val = 0;

    return ns;
}

void nanosynth_add_melody(nanosynth *ns)
{
    ns->num_melodies++;
    ns->cur_melody++;
}

void nanosynth_switch_melody(nanosynth *ns, unsigned int melody_num)
{
    if (melody_num < (unsigned)ns->num_melodies) {
        ns->cur_melody = melody_num;
    }
}

void nanosynth_reset_melody_all(nanosynth *ns)
{
    for (int i = 0; i < ns->num_melodies; i++) {
        nanosynth_reset_melody(ns, i);
    }
}

void nanosynth_reset_melody(nanosynth *ns, unsigned int melody_num)
{
    if (melody_num < (unsigned)ns->num_melodies) {
        for (int i = 0; i < PPNS; i++) {
            if (ns->melodies[melody_num][i] != NULL) {
                midi_event *tmp = ns->melodies[melody_num][i];
                ns->melodies[melody_num][i] = NULL;
                free(tmp);
            }
        }
    }
}

void nanosynth_change_osc_wave_form(nanosynth *self, unsigned int voice_no,
                                    int oscil, bool all_voices)
{
    if (all_voices) {
        for (int i = 0; i < MAX_VOICES; i++)
            p_nanosynth_change_osc_wave_form(self, i, oscil);
    }
    else
        p_nanosynth_change_osc_wave_form(self, voice_no, oscil);
}

void p_nanosynth_change_osc_wave_form(nanosynth *self, unsigned int voice_no,
                                      int oscil)
{
    unsigned cur_type = 0;
    unsigned next_type = 0;

    switch (oscil) {
    case 0:
        cur_type = self->m_voices[voice_no].osc1->m_waveform;
        next_type = (cur_type + 1) % MAX_OSC;
        self->m_voices[voice_no].osc1->m_waveform = next_type;
    case 1:
        cur_type = self->m_voices[voice_no].osc2->m_waveform;
        next_type = (cur_type + 1) % MAX_OSC;
        self->m_voices[voice_no].osc2->m_waveform = next_type;
    case 2:
        cur_type = self->m_voices[voice_no].lfo->m_waveform;
        next_type = (cur_type + 1) % MAX_LFO_OSC;
        self->m_voices[voice_no].lfo->m_waveform = next_type;
    }
    printf("now set to %d\n", next_type);
}

void nanosynth_setvol(void *self, double v)
{
    nanosynth *ns = (nanosynth *)self;
    if (v < 0.0 || v > 1.0) {
        return;
    }
    ns->vol = v;
}

double nanosynth_getvol(void *self)
{
    nanosynth *ns = (nanosynth *)self;
    return ns->vol;
}

void change_octave(void *self, int direction)
{
    nanosynth *ns = (nanosynth *)self;
    if (direction == UP)
        ns->cur_octave++;
    else
        ns->cur_octave--;
}

void nanosynth_melody_to_string(nanosynth *ns, int melody_num,
                                wchar_t melodystr[33])
{
    int cur_quart = 0;
    for (int i = 0; i < PPNS; i += PPS) {
        melodystr[cur_quart] = sparkchars[0];
        for (int j = i; j < (i + PPS); j++) {
            if (ns->melodies[melody_num][j] != NULL &&
                ns->melodies[melody_num][j]->event_type ==
                    144) { // 144 is midi note on
                melodystr[cur_quart] = sparkchars[5];
            }
        }
        cur_quart++;
    }
}

void nanosynth_status(void *self, wchar_t *status_string)
{
    // TODO - a shit load of error checking on boundaries and size
    nanosynth *ns = (nanosynth *)self;
    swprintf(status_string, 119,
             WCOOL_COLOR_PINK "[SYNTH] - Vol: %.2f Sustain: %d "
                              "Multi: %d, Cur: %d",
             ns->vol, ns->sustain, ns->multi_melody_mode, ns->cur_melody);
    for (int i = 0; i < ns->num_melodies; i++) {
        wchar_t melodystr[33] = {0};
        wchar_t scratch[128] = {0};
        nanosynth_melody_to_string(ns, i, melodystr);
        swprintf(scratch, 127, L"\n      [%d]  %ls  numloops: %d", i, melodystr,
                 ns->melody_multiloop_count[i]);
        wcscat(status_string, scratch);
    }
    wcscat(status_string, WANSI_COLOR_RESET);
}

void nanosynth_midi_note_on(nanosynth *self, unsigned int midi_num,
                            unsigned int velocity)
{
    if (!self->m_voices[0].osc1->m_note_on) {
        nanosynth_start_note(self, 0, midi_num, velocity);
    }
    else if (!self->m_voices[1].osc1->m_note_on) {
        nanosynth_start_note(self, 1, midi_num, velocity);
    }
    else {
        unsigned int note0 = self->m_voices[0].osc1->m_midi_note_number;
        unsigned int note1 = self->m_voices[1].osc1->m_midi_note_number;
        // if new note is higher than both, steal the lower of the two
        if (midi_num < note0 && midi_num < note1) {
            if (note0 < note1) {
                nanosynth_steal_note(self, 0, midi_num, velocity);
            }
            else {
                nanosynth_steal_note(self, 1, midi_num, velocity);
            }
        }
        else { // steal higher note
            if (note0 > note1) {
                nanosynth_steal_note(self, 0, midi_num, velocity);
            }
            else {
                nanosynth_steal_note(self, 1, midi_num, velocity);
            }
        }
    }
}

bool nanosynth_midi_note_off(nanosynth *self, unsigned int midi_num,
                             unsigned int velocity, bool all_notes_off)
{
    (void)velocity;
    if (all_notes_off) {
        eg_note_off(self->m_voices[0].eg1);
        eg_note_off(self->m_voices[1].eg1);
        return true;
    }
    if (midi_num == self->m_voices[0].osc1->m_midi_note_number)
        eg_note_off(self->m_voices[0].eg1);
    if (midi_num == self->m_voices[1].osc1->m_midi_note_number)
        eg_note_off(self->m_voices[1].eg1);
    return true;
}

double nanosynth_gennext(void *self)
{
    nanosynth *ns = (nanosynth *)self;

    double out_left = 0.0;
    double out_right = 0.0;

    double accum_out_left = 0.0;
    double accum_out_right = 0.0;

    for (int i = 0; i < MAX_VOICES; i++) {

        // clear for loop
        out_left = 0.0;
        out_right = 0.0;

        if (ns->m_voices[i].osc1->m_note_on) {

            // layer 0 //////////////////////////////
            do_modulation_matrix(ns->m_voices[i].g_modmatrix, 0);
            ///////////////////////////////////////////

            eg_update(ns->m_voices[i].eg1);
            ns->m_voices[i].lfo->update_oscillator(ns->m_voices[i].lfo);

            eg_do_envelope(ns->m_voices[i].eg1, NULL);
            ns->m_voices[i].lfo->do_oscillate(ns->m_voices[i].lfo, NULL);

            //// layer 1 /////////////////////////////
            do_modulation_matrix(ns->m_voices[i].g_modmatrix, 1);
            ///////////////////////////////////////////

            dca_update(ns->m_voices[i].dca);
            ns->m_voices[i].f->update(ns->m_voices[i].f);

            ns->m_voices[i].osc1->update_oscillator(ns->m_voices[i].osc1);
            ns->m_voices[i].osc2->update_oscillator(ns->m_voices[i].osc2);

            //// audio engine block ///////////////////////////////
            double osc1_val =
                ns->m_voices[i].osc1->do_oscillate(ns->m_voices[i].osc1, NULL);
            double osc2_val =
                ns->m_voices[i].osc2->do_oscillate(ns->m_voices[i].osc2, NULL);

            double osc_out = 0.5 * osc1_val + 0.5 * osc2_val;
            // printf("OSC_OUT: %f\n", osc_out);
            double filter_out =
                ns->m_voices[i].f->gennext(ns->m_voices[i].f, osc_out);
            // if (filter_out != 0.0)
            //   printf("FILTER_OUT%f\n", osc_out);
            dca_gennext(ns->m_voices[i].dca, filter_out, filter_out, &out_left,
                        &out_right);
            // printf("OUT_LEFT%f\n", out_left);
            // if (out_left != 0.0)
            //    printf("AFTER DCA_GENNEXT  %f\n", out_left);

            out_left = effector(&ns->sound_generator, out_left);
            out_right = effector(&ns->sound_generator, out_right);

            out_left = envelopor(&ns->sound_generator, out_left);
            out_right = envelopor(&ns->sound_generator, out_right);

            accum_out_left += out_left;
            accum_out_right += out_right;

            // if note is on but EG is off, note is finished
            if ((eg_get_state(ns->m_voices[i].eg1)) == 0) { // OFF
                if (ns->m_pending_midi_note[i] >= 0) {
                    ns->m_voices[i].osc1->m_midi_note_number =
                        ns->m_pending_midi_note[i];
                    ns->m_voices[i].osc1->m_osc_fo =
                        get_midi_freq(ns->m_pending_midi_note[i]);

                    ns->m_voices[i].osc2->m_midi_note_number =
                        ns->m_pending_midi_note[i];
                    ns->m_voices[i].osc2->m_osc_fo =
                        get_midi_freq(ns->m_pending_midi_note[i]);

                    osc_update(ns->m_voices[i].osc1);
                    osc_update(ns->m_voices[i].osc2);

                    eg_start_eg(ns->m_voices[i].eg1);
                    ns->m_voices[i]
                        .g_modmatrix->m_sources[SOURCE_MIDI_NOTE_NUM] =
                        ns->m_pending_midi_note[i];
                    ns->m_voices[i].g_modmatrix->m_sources[SOURCE_VELOCITY] =
                        ns->m_pending_midi_velocity[i];

                    // --- reset
                    ns->m_pending_midi_note[i] = -1;
                    ns->m_pending_midi_velocity[i] = -1;
                }
                else {
                    ns->m_voices[i].osc1->stop_oscillator(ns->m_voices[i].osc1);
                    ns->m_voices[i].osc2->stop_oscillator(ns->m_voices[i].osc2);
                    ns->m_voices[i].lfo->stop_oscillator(ns->m_voices[i].lfo);
                    eg_stop_eg(ns->m_voices[i].eg1);
                }
            }
        }
    }

    return accum_out_left * ns->vol;
}

void nanosynth_set_multi_melody_mode(nanosynth *self, bool melody_mode)
{
    self->multi_melody_mode = melody_mode;
    // self->multi_melody_loop_countdown_started = true;
    self->cur_melody_iteration = self->melody_multiloop_count[self->cur_melody];
}

void nanosynth_set_melody_loop_num(nanosynth *self, int melody_num,
                                   int loop_num)
{
    self->melody_multiloop_count[melody_num] = loop_num;
}

void nanosynth_set_sustain(nanosynth *self, int val)
{
    self->sustain = val;
    printf("Set sustain to %d\n", val);
}

void nanosynth_midi_control(nanosynth *ns, unsigned int data1,
                            unsigned int data2)
{
    // printf("MIDI Mind Control! %d %d\n", data1, data2);

    for (int i = 0; i < MAX_VOICES; i++) {
        double scaley_val;
        switch (data1) {
        case 1: // K1 - Envelope Attack Time Msec
            scaley_val = scaleybum(1, 128, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
            eg_set_attack_time_msec(ns->m_voices[i].eg1, scaley_val);
            break;
        case 2: // K2 - Envelope Decay Time Msec
            scaley_val = scaleybum(1, 128, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
            eg_set_decay_time_msec(ns->m_voices[i].eg1, scaley_val);
            break;
        case 3: // K3 - Envelope Sustain Level
            scaley_val = scaleybum(1, 128, 0, 1, data2);
            eg_set_sustain_level(ns->m_voices[i].eg1, scaley_val);
            break;
        case 4: // K4 - Envelope Release Time Msec
            scaley_val = scaleybum(1, 128, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
            eg_set_release_time_msec(ns->m_voices[i].eg1, scaley_val);
            break;
        case 5: // K5 - LFO rate
            scaley_val = scaleybum(0, 128, MIN_LFO_RATE, MAX_LFO_RATE, data2);
            ns->m_voices[i].lfo->m_osc_fo = scaley_val;
            osc_update(ns->m_voices[i].lfo);
            break;
        case 6: // K6 - LFO amplitude
            scaley_val = scaleybum(0, 128, 0.0, 1.0, data2);
            ns->m_voices[i].lfo->m_amplitude = scaley_val;
            osc_update(ns->m_voices[i].lfo);
            break;
        case 7: // K7 - Filter Frequency Cut
            scaley_val = scaleybum(1, 128, FILTER_FC_MIN, FILTER_FC_MAX, data2);
            ns->m_voices[i].f->m_fc_control = scaley_val;
            break;
        case 8: // K8 - Filter Q control
            scaley_val = scaleybum(1, 128, 1, 10, data2);
            printf("FILTER Q control! %f\n", scaley_val);
            filter_set_q_control(ns->m_voices[i].f, scaley_val);
            break;
        default:
            printf("SOMthing else\n");
        }
    }
}

void nanosynth_midi_pitchbend(nanosynth *ns, unsigned int data1,
                              unsigned int data2)
{
    // printf("Pitch bend, babee: %d %d\n", data1, data2);
    int actual_pitch_bent_val = (int)((data1 & 0x7F) | ((data2 & 0x7F) << 7));

    if (actual_pitch_bent_val != 8192) {
        double normalized_pitch_bent_val =
            (float)(actual_pitch_bent_val - 0x2000) / (float)(0x2000);
        double scaley_val =
            // scaleybum(0, 16383, -100, 100, normalized_pitch_bent_val);
            scaleybum(0, 16383, -600, 600, actual_pitch_bent_val);
        // printf("Cents to bend - %f\n", scaley_val);
        for (int i = 0; i < MAX_VOICES; i++) {
            ns->m_voices[i].osc1->m_cents = scaley_val;
            ns->m_voices[i].osc2->m_cents = scaley_val + 2.5;
            ns->m_voices[i].g_modmatrix->m_sources[SOURCE_PITCHBEND] =
                normalized_pitch_bent_val;
        }
    }
    else {
        for (int i = 0; i < MAX_VOICES; i++) {
            ns->m_voices[i].osc1->m_cents = 0;
            ns->m_voices[i].osc2->m_cents = 2.5;
        }
    }
}

void nanosynth_start_note(nanosynth *ns, int index, unsigned int midinote,
                          unsigned int velocity)
{
    if (index > MAX_VOICES - 1)
        return;

    // --- set pitches
    ns->m_voices[index].osc1->m_midi_note_number = midinote;
    ns->m_voices[index].osc1->m_osc_fo = get_midi_freq(midinote);

    ns->m_voices[index].osc2->m_midi_note_number = midinote;
    ns->m_voices[index].osc2->m_osc_fo = get_midi_freq(midinote);

    // --- start the modulators FIRST
    ns->m_voices[index].lfo->start_oscillator(ns->m_voices[index].lfo);
    eg_start_eg(ns->m_voices[index].eg1);

    // --- not playing, reset and do updateOscillator()
    ns->m_voices[index].osc1->start_oscillator(ns->m_voices[index].osc1);
    ns->m_voices[index].osc2->start_oscillator(ns->m_voices[index].osc2);

    // --- set the note number in the mod matrix
    ns->m_voices[index].g_modmatrix->m_sources[SOURCE_MIDI_NOTE_NUM] = midinote;

    // --- velocity modulation
    ns->m_voices[index].g_modmatrix->m_sources[SOURCE_VELOCITY] = velocity;
}

void nanosynth_steal_note(nanosynth *ns, int index,
                          unsigned int pending_midinote,
                          unsigned int pending_velocity)
{
    if (index > MAX_VOICES - 1)
        return;

    // --- shutdown the EG with fast linear taper
    //         - if in Legato mode, EG will ignore this
    //     - if in RTZ mode, EG will use shutdown linear taper
    //           otherwise it goes directly to off state for instant
    //       re-trigger starting at current EG output level
    eg_shutdown(ns->m_voices[index].eg1);

    // --- save the pending note and velocity
    ns->m_pending_midi_note[index] = pending_midinote;
    ns->m_pending_midi_velocity[index] = pending_velocity;
}

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

nanosynth *new_nanosynth()
{
    nanosynth *ns;
    ns = (nanosynth *)calloc(1, sizeof(nanosynth));
    if (ns == NULL)
        return NULL;

    ns->osc1 = (oscillator *)qb_osc_new();
    ns->osc2 = (oscillator *)qb_osc_new();
    ns->osc2->m_cents = 2.5;

    ns->lfo = (oscillator *)lfo_new();

    ns->eg1 = new_envelope_generator();

    // ns->f = (filter *) new_filter_onepole();
    // ns->f = (filter *) new_filter_sem();
    // ns->f = (filter *)new_filter_ck35();
    ns->f = (filter *)new_filter_moog();

    ns->dca = new_dca();

    // ns->mloop = (melody_loop *)calloc(1, sizeof(melody_loop));

    ns->m_modmatrix = new_modmatrix();

    ns->m_default_mod_range = 1.0;
    ns->m_osc_fo_mod_range = OSC_FO_MOD_RANGE;
    ns->m_filter_mod_range = FILTER_FC_MOD_RANGE;
    ns->m_osc_fo_pitchbend_mod_range = OSC_PITCHBEND_MOD_RANGE;
    ns->m_amp_mod_range = AMP_MOD_RANGE;

    ns->m_default_mod_intensity = 1.0;
    ns->m_eg1_dca_intensity = 1.0;
    ns->m_eg1_osc_intensity = 0.0;

    matrixrow *row = NULL;
    // LFO -> ALL OSC FO
    row = create_matrix_row(SOURCE_LFO1, DEST_ALL_OSC_FO,
                            &ns->m_default_mod_intensity,
                            &ns->m_osc_fo_mod_range, TRANSFORM_NONE, true);
    add_matrix_row(ns->m_modmatrix, row);

    // EG1 -> ALL OSC FO
    row = create_matrix_row(SOURCE_BIASED_EG1, DEST_ALL_OSC_FO,
                            &ns->m_eg1_osc_intensity, &ns->m_osc_fo_mod_range,
                            TRANSFORM_NONE, true);
    add_matrix_row(ns->m_modmatrix, row);

    // EG1 -> FILTER1 FC
    row = create_matrix_row(SOURCE_BIASED_EG1, DEST_ALL_FILTER_FC,
                            &ns->m_default_mod_intensity,
                            &ns->m_filter_mod_range, TRANSFORM_NONE, true);
    add_matrix_row(ns->m_modmatrix, row);

    // EG1 -> DCA EG
    row = create_matrix_row(SOURCE_EG1, DEST_DCA_EG, &ns->m_eg1_dca_intensity,
                            &ns->m_default_mod_range, TRANSFORM_NONE, true);
    add_matrix_row(ns->m_modmatrix, row);

    // NOTE NUMBER -> FILTER FC CONTROL
    row = create_matrix_row(SOURCE_MIDI_NOTE_NUM, DEST_ALL_FILTER_KEYTRACK,
                            &ns->m_filter_keytrack_intensity,
                            &ns->m_default_mod_range,
                            TRANSFORM_NOTE_NUMBER_TO_FREQUENCY, true);
    add_matrix_row(ns->m_modmatrix, row);

    // VELOCITY -> DCA VEL
    row = create_matrix_row(SOURCE_VELOCITY, DEST_DCA_VELOCITY,
                            &ns->m_default_mod_intensity,
                            &ns->m_default_mod_range, TRANSFORM_NONE, true);
    add_matrix_row(ns->m_modmatrix, row);

    // PITCHBEND -> PITCHBEND
    row = create_matrix_row(
        SOURCE_PITCHBEND, DEST_ALL_OSC_FO, &ns->m_default_mod_intensity,
        &ns->m_osc_fo_pitchbend_mod_range, TRANSFORM_NONE, true);
    add_matrix_row(ns->m_modmatrix, row);

    // MIDI Vol CC07
    row = create_matrix_row(SOURCE_MIDI_VOLUME_CC07, DEST_DCA_AMP,
                            &ns->m_default_mod_intensity, &ns->m_amp_mod_range,
                            TRANSFORM_INVERT_MIDI_NORMALIZE, true);
    add_matrix_row(ns->m_modmatrix, row);

    // MIDI Pan CC10
    row = create_matrix_row(
        SOURCE_MIDI_PAN_CC10, DEST_DCA_PAN, &ns->m_default_mod_intensity,
        &ns->m_default_mod_range, TRANSFORM_MIDI_TO_PAN, true);
    add_matrix_row(ns->m_modmatrix, row);

    // MIDI Sustain Pedal
    row = create_matrix_row(SOURCE_SUSTAIN_PEDAL, DEST_ALL_EG_SUSTAIN_OVERRIDE,
                            &ns->m_default_mod_intensity,
                            &ns->m_default_mod_range, TRANSFORM_MIDI_SWITCH,
                            true);
    add_matrix_row(ns->m_modmatrix, row);

    // VELOCITY -> EG ATTACK SOURCE_VELOCITY
    // 0 velocity -> scalar = 1, normal attack time
    // 128 velocity -> scalar = 0, fastest (0) attack time:
    row = create_matrix_row(SOURCE_VELOCITY, DEST_ALL_EG_ATTACK_SCALING,
                            &ns->m_default_mod_intensity,
                            &ns->m_default_mod_range, TRANSFORM_MIDI_NORMALIZE,
                            false);
    add_matrix_row(ns->m_modmatrix, row);

    // NOTE NUMBER -> EG DECAY SCALING
    row = create_matrix_row(SOURCE_MIDI_NOTE_NUM, DEST_ALL_EG_DECAY_SCALING,
                            &ns->m_default_mod_intensity,
                            &ns->m_default_mod_range, TRANSFORM_MIDI_NORMALIZE,
                            false);
    add_matrix_row(ns->m_modmatrix, row);

    ns->m_modmatrix->m_sources[SOURCE_MIDI_VOLUME_CC07] = 127;
    ns->m_modmatrix->m_sources[SOURCE_MIDI_PAN_CC10] = 64;

    // end mod matrix setup ///////////////////////////////////
    //
    // mod matrix routings ////////////////////////////////////

    ns->osc1->g_modmatrix = ns->m_modmatrix;
    ns->osc1->m_mod_source_fo = DEST_OSC1_FO;
    ns->osc1->m_mod_source_amp = DEST_OSC1_OUTPUT_AMP;

    ns->osc2->g_modmatrix = ns->m_modmatrix;
    ns->osc2->m_mod_source_fo = DEST_OSC2_FO;
    ns->osc2->m_mod_source_amp = DEST_OSC2_OUTPUT_AMP;

    ns->f->global_modmatrix = ns->m_modmatrix;
    ns->f->m_mod_source_fc = DEST_FILTER1_FC;
    ns->f->m_mod_source_fc_control = DEST_ALL_FILTER_KEYTRACK;

    // modulators - they write their outputs into
    // what will be a Source for something else

    ns->lfo->g_modmatrix = ns->m_modmatrix;
    ns->lfo->m_mod_dest_output1 = SOURCE_LFO1;
    ns->lfo->m_mod_dest_output2 = SOURCE_LFO1Q;

    ns->eg1->global_modmatrix = ns->m_modmatrix;
    ns->eg1->m_mod_dest_eg_output = SOURCE_EG1;
    ns->eg1->m_mod_dest_eg_biased_output = SOURCE_BIASED_EG1;
    ns->eg1->m_mod_source_eg_attack_scaling = DEST_EG1_ATTACK_SCALING;
    ns->eg1->m_mod_source_eg_decay_scaling = DEST_EG1_DECAY_SCALING;
    ns->eg1->m_mod_source_sustain_override = DEST_EG1_SUSTAIN_OVERRIDE;

    ns->dca->global_modmatrix = ns->m_modmatrix;
    ns->dca->m_mod_source_eg = DEST_DCA_EG;
    ns->dca->m_mod_source_amp_db = DEST_NONE;
    ns->dca->m_mod_source_velocity = DEST_DCA_VELOCITY;
    ns->dca->m_mod_source_pan = DEST_DCA_PAN;

    ns->vol = 1.0;
    ns->cur_octave = 4;
    ns->sustain = 0;

    ns->m_filter_keytrack = true;
    ns->m_filter_keytrack_intensity = 0.5;

    ns->sound_generator.gennext = &nanosynth_gennext;
    ns->sound_generator.status = &nanosynth_status;
    ns->sound_generator.setvol = &nanosynth_setvol;
    ns->sound_generator.getvol = &nanosynth_getvol;
    ns->sound_generator.type = NANOSYNTH_TYPE;

    for (int i = 0; i < PPL; i++)
        ns->mloop[i] = 0;

    // start loop player running
    pthread_t melody_looprrr;
    if (pthread_create(&melody_looprrr, NULL, play_melody_loop, ns)) {
        fprintf(stderr, "Err running loop\n");
    }
    else {
        pthread_detach(melody_looprrr);
    }

    nanosynth_update(ns);

    return ns;
}

void nanosynth_change_osc_wave_form(nanosynth *self, int oscil)
{
    unsigned cur_type = 0;
    unsigned next_type = 0;

    if (oscil == 0) {
        cur_type = self->osc1->m_waveform;
        next_type = (cur_type + 1) % MAX_OSC;
        self->osc1->m_waveform = next_type;
    }
    else if (oscil == 1) {
        cur_type = self->osc2->m_waveform;
        next_type = (cur_type + 1) % MAX_OSC;
        self->osc2->m_waveform = next_type;
    }
    else if (oscil == 2) {
        cur_type = self->lfo->m_waveform;
        next_type = (cur_type + 1) % MAX_LFO_OSC;
        self->lfo->m_waveform = next_type;
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
    int octave = ns->cur_octave;
    if (direction == UP)
        octave++;
    else
        octave--;

    if (octave >= 0 && octave < 6)
        ns->cur_octave = octave;
}

void nanosynth_print_melodies(nanosynth *self)
{
    for (int i = 0; i < PPL; i++) {
        if (self->mloop[i] != 0) {
            printf("%d: %d ", i, self->mloop[i]);
        }
    }
    printf("\n");
}

void nanosynth_status(void *self, char *status_string)
{
    nanosynth *ns = (nanosynth *)self;
    snprintf(status_string, 119, ANSI_COLOR_RED "nanosynth! %.2f(freq) "
                                                "vol: %.2f" ANSI_COLOR_RESET,
             ns->osc1->m_fo, ns->vol);
    nanosynth_print_melodies(ns);
}

void note_on(nanosynth *self, int midi_num)
{
    int midi_freq = get_midi_freq(midi_num);

    self->osc1->m_midi_note_number = midi_num;
    self->osc1->m_osc_fo = midi_freq;

    self->osc2->m_midi_note_number = midi_num;
    self->osc2->m_osc_fo = midi_freq;

    self->lfo->start_oscillator(self->lfo);
    start_eg(self->eg1);

    if (!self->osc1->m_note_on) {
        self->osc1->start_oscillator(self->osc1);
        self->osc2->start_oscillator(self->osc2);
    }
    else {
        self->osc1->update_oscillator(self->osc1);
        self->osc2->update_oscillator(self->osc2);
    }

    self->m_modmatrix->m_sources[SOURCE_MIDI_NOTE_NUM] = midi_num;
    // TODO: send velocity self->m_modmatrix->m_sources[SOURCE_VELOCITY] =
    // velocity;
}

// void nanosynth_add_note(nanosynth *self, int midi_num)
// {
//     if (self->recording) {
//         printf("NOTE RECORDED\n");
//         melody_event *me =
//             make_melody_event(mixr->tick % PPL, midi_num);
//         add_melody_event(self->mloop, me);
//         // add recording event
//     }
// }

double nanosynth_gennext(void *self)
{
    nanosynth *ns = (nanosynth *)self;

    if (ns->osc1->m_note_on) {

        do_modulation_matrix(ns->m_modmatrix, 0);

        eg_update(ns->eg1);
        ns->lfo->update_oscillator(ns->lfo);

        eg_generate(ns->eg1, NULL);
        ns->lfo->do_oscillate(ns->lfo, NULL);

        do_modulation_matrix(ns->m_modmatrix, 1);

        dca_update(ns->dca);
        ns->f->update(ns->f);

        ns->osc1->update_oscillator(ns->osc1);
        ns->osc2->update_oscillator(ns->osc2);

        double osc1_val = ns->osc1->do_oscillate(ns->osc1, NULL);
        double osc2_val = ns->osc2->do_oscillate(ns->osc2, NULL);

        double osc_out = 0.5 * osc1_val + 0.5 * osc2_val;

        // double filter_out = osc_out;
        double filter_out = ns->f->gennext(ns->f, osc_out);

        double out_left = 0.0;
        double out_right = 0.0;
        dca_gennext(ns->dca, filter_out, filter_out, &out_left, &out_right);

        // if ((get_state(ns->eg1)) == 0) {
        //     ns->osc1->stop_oscillator(ns->osc1);
        //     ns->osc2->stop_oscillator(ns->osc2);
        //     ns->lfo->stop_oscillator(ns->lfo);
        //     stop_eg(ns->eg1);
        // }

        double val = out_left * ns->vol;
        val = effector(&ns->sound_generator, val);
        val = envelopor(&ns->sound_generator, val);

        return val;
    }
    else {
        return 0.0;
    }
}

void nanosynth_set_sustain(nanosynth *self, int val)
{
    self->sustain = val;
    printf("Set sustain to %d\n", val);
}

// void nanosynth_add_melody_loop(void *self, melody_loop *mloop)
// {
//     nanosynth *ns = (nanosynth *)self;
//     if (ns->melody_loop_num < 10)
//         ns->mloops[ns->melody_loop_num++] = mloop;
// }

void nanosynth_update(nanosynth *self)
{
    self->osc1->m_waveform = self->m_osc_waveform;
    self->osc2->m_waveform = self->m_osc_waveform;
    // osc_update(self->osc1);
    // osc_update(self->osc2);

    self->f->m_fc_control = self->m_fc_control;
    self->f->m_q_control = self->m_q_control;
    // filter_update(self->f);

    self->lfo->m_waveform = self->m_lfo_waveform;
    self->lfo->m_amplitude = self->m_lfo_amplitude;
    self->lfo->m_osc_fo = self->m_lfo_rate;
    self->lfo->m_lfo_mode = self->m_lfo_mode;
    // osc_update(self->lfo);

    set_attack_time_msec(self->eg1, self->m_attack_time_msec);
    set_decay_time_msec(self->eg1, self->m_decay_time_msec);
    set_sustain_level(self->eg1, self->m_sustain_level);
    set_release_time_msec(self->eg1, self->m_release_time_msec);

    self->eg1->m_reset_to_zero = self->m_reset_to_zero;
    self->eg1->m_legato_mode = self->m_legato_mode;

    dca_set_pan_control(self->dca, self->m_volume_db);
    dca_set_amplitude_db(self->dca, self->m_volume_db);
    // dca_update(self->dca);

    if (self->m_filter_keytrack) {
        enable_matrix_row(self->m_modmatrix, SOURCE_MIDI_NOTE_NUM,
                          DEST_ALL_FILTER_KEYTRACK, true);
    }
    else {
        enable_matrix_row(self->m_modmatrix, SOURCE_MIDI_NOTE_NUM,
                          DEST_ALL_FILTER_KEYTRACK, false);
    }

    if (self->m_velocity_to_attack_scaling) {
        enable_matrix_row(self->m_modmatrix, SOURCE_VELOCITY,
                          DEST_ALL_EG_ATTACK_SCALING, true);
    }
    else {
        enable_matrix_row(self->m_modmatrix, SOURCE_VELOCITY,
                          DEST_ALL_EG_ATTACK_SCALING, false);
    }

    if (self->m_note_number_to_decay_scaling) {
        enable_matrix_row(self->m_modmatrix, SOURCE_MIDI_NOTE_NUM,
                          DEST_ALL_EG_DECAY_SCALING, true);
    }
    else {
        enable_matrix_row(self->m_modmatrix, SOURCE_MIDI_NOTE_NUM,
                          DEST_ALL_EG_DECAY_SCALING, false);
    }
}

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

    ns->osc1 = (oscillator *)qb_osc_new();
    ns->osc2 = (oscillator *)qb_osc_new();
    ns->osc2->m_cents = 2.5;

    ns->lfo = (oscillator *)lfo_new();
    ns->m_lfo1_dest = OSC;
    ns->m_lfo_dest_string[0] = "Oscillators";
    ns->m_lfo_dest_string[1] = "Filters";

    ns->eg1 = new_envelope_generator();

    // ns->f = (filter *) new_filter_onepole();
    // ns->f = (filter *) new_filter_sem();
    // ns->f = (filter *)new_filter_ck35();
    ns->f = (filter *)new_filter_moog();

    ns->dca = new_dca();

    ns->m_modmatrix = new_modmatrix();

    ns->m_default_mod_intensity = 1.0;
    ns->m_default_mod_range = 1.0;

    ns->m_osc_fo_mod_range = OSC_FO_MOD_RANGE;
    ns->m_filter_mod_range = FILTER_FC_MOD_RANGE;

    ns->m_osc_fo_pitchbend_mod_range = OSC_PITCHBEND_MOD_RANGE;
    ns->m_amp_mod_range = AMP_MOD_RANGE;

    ns->m_eg1_dca_intensity = 1.0;
    ns->m_eg1_osc_intensity = 0.0;

    matrixrow *row = NULL;
    // LFO -> ALL OSC FO
    row = create_matrix_row(SOURCE_LFO1, DEST_ALL_OSC_FO,
                            &ns->m_default_mod_intensity,
                            &ns->m_osc_fo_mod_range, TRANSFORM_NONE, true);
    add_matrix_row(ns->m_modmatrix, row);

    // LFO1 -> FILTER1 Fc
    row = create_matrix_row(SOURCE_LFO1, DEST_ALL_FILTER_FC,
                            &ns->m_default_mod_intensity,
                            &ns->m_filter_mod_range, TRANSFORM_NONE, false);
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
        &ns->m_default_mod_range, TRANSFORM_MIDI_TO_PAN, false);
    add_matrix_row(ns->m_modmatrix, row);

    // MIDI Sustain Pedal
    row = create_matrix_row(SOURCE_SUSTAIN_PEDAL, DEST_ALL_EG_SUSTAIN_OVERRIDE,
                            &ns->m_default_mod_intensity,
                            &ns->m_default_mod_range, TRANSFORM_MIDI_SWITCH,
                            false);
    add_matrix_row(ns->m_modmatrix, row);

    // VELOCITY -> EG ATTACK SOURCE_VELOCITY
    // 0 velocity -> scalar = 1, normal attack time
    // 128 velocity -> scalar = 0, fastest (0) attack time:
    row = create_matrix_row(SOURCE_VELOCITY, DEST_ALL_EG_ATTACK_SCALING,
                            &ns->m_default_mod_intensity,
                            &ns->m_default_mod_range, TRANSFORM_MIDI_NORMALIZE,
                            true);
    add_matrix_row(ns->m_modmatrix, row);

    // NOTE NUMBER -> EG DECAY SCALING
    row = create_matrix_row(SOURCE_MIDI_NOTE_NUM, DEST_ALL_EG_DECAY_SCALING,
                            &ns->m_default_mod_intensity,
                            &ns->m_default_mod_range, TRANSFORM_MIDI_NORMALIZE,
                            true);
    add_matrix_row(ns->m_modmatrix, row);

    ns->m_modmatrix->m_sources[SOURCE_MIDI_VOLUME_CC07] = 127;
    ns->m_modmatrix->m_sources[SOURCE_MIDI_PAN_CC10] = 64;

    // end mod matrix setup ///////////////////////////////////

    // mod matrix routings ////////////////////////////////////

    ns->osc1->g_modmatrix = ns->m_modmatrix;
    ns->osc1->m_mod_source_fo = DEST_OSC1_FO;
    ns->osc1->m_mod_source_amp = DEST_OSC1_OUTPUT_AMP;

    ns->osc2->g_modmatrix = ns->m_modmatrix;
    ns->osc2->m_mod_source_fo = DEST_OSC2_FO;
    ns->osc2->m_mod_source_amp = DEST_OSC2_OUTPUT_AMP;

    ns->f->g_modmatrix = ns->m_modmatrix;
    ns->f->m_mod_source_fc = DEST_FILTER1_FC;
    ns->f->m_mod_source_fc_control = DEST_ALL_FILTER_KEYTRACK;

    // modulators - they write their outputs into
    // what will be a Source for something else

    ns->lfo->g_modmatrix = ns->m_modmatrix;
    ns->lfo->m_mod_dest_output1 = SOURCE_LFO1;
    ns->lfo->m_mod_dest_output2 = SOURCE_LFO1Q;

    ns->eg1->g_modmatrix = ns->m_modmatrix;
    ns->eg1->m_mod_dest_eg_output = SOURCE_EG1;
    ns->eg1->m_mod_dest_eg_biased_output = SOURCE_BIASED_EG1;
    ns->eg1->m_mod_source_eg_attack_scaling = DEST_EG1_ATTACK_SCALING;
    ns->eg1->m_mod_source_eg_decay_scaling = DEST_EG1_DECAY_SCALING;
    ns->eg1->m_mod_source_sustain_override = DEST_EG1_SUSTAIN_OVERRIDE;

    ns->dca->g_modmatrix = ns->m_modmatrix;
    ns->dca->m_mod_source_eg = DEST_DCA_EG;
    ns->dca->m_mod_source_amp_db = DEST_NONE;
    ns->dca->m_mod_source_velocity = DEST_DCA_VELOCITY;
    ns->dca->m_mod_source_pan = DEST_DCA_PAN;

    ns->vol = 0.7;
    ns->dca->m_gain = 0.7;
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

    // nanosynth_update(ns);

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
             ns->vol, ns->sustain, ns->multi_melody_mode,
             ns->cur_melody);
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

void note_on(nanosynth *self, int midi_num)
{
    int midi_freq = get_midi_freq(midi_num + (self->cur_octave * 12));

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

double nanosynth_gennext(void *self)
{
    nanosynth *ns = (nanosynth *)self;

    double out_left = 0.0;
    double out_right = 0.0;

    if (ns->osc1->m_note_on) {

        // layer 0 //////////////////////////////
        do_modulation_matrix(ns->m_modmatrix, 0);
        ///////////////////////////////////////////

        eg_update(ns->eg1);
        ns->lfo->update_oscillator(ns->lfo);

        do_envelope(ns->eg1, NULL);
        ns->lfo->do_oscillate(ns->lfo, NULL);

        //// layer 1 /////////////////////////////
        do_modulation_matrix(ns->m_modmatrix, 1);
        ///////////////////////////////////////////

        dca_update(ns->dca);
        ns->f->update(ns->f);

        ns->osc1->update_oscillator(ns->osc1);
        ns->osc2->update_oscillator(ns->osc2);

        //// audio engine block ///////////////////////////////
        double osc1_val = ns->osc1->do_oscillate(ns->osc1, NULL);
        double osc2_val = ns->osc2->do_oscillate(ns->osc2, NULL);

        double osc_out = 0.5 * osc1_val + 0.5 * osc2_val;

        double filter_out = ns->f->gennext(ns->f, osc_out);

        dca_gennext(ns->dca, filter_out, filter_out, &out_left, &out_right);

        if ((get_state(ns->eg1)) == 0) {
            ns->osc1->stop_oscillator(ns->osc1);
            ns->osc2->stop_oscillator(ns->osc2);
            ns->lfo->stop_oscillator(ns->lfo);
            stop_eg(ns->eg1);
        }

        out_left = effector(&ns->sound_generator, out_left);
        out_left = envelopor(&ns->sound_generator, out_left);
    }

    return out_left * ns->vol;
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

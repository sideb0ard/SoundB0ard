#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "midi_freq_table.h"
#include "minisynth.h"
#include "mixer.h"
#include "sequencer_utils.h"
#include "utils.h"

extern mixer *mixr;
extern const wchar_t *sparkchars;
extern const char *key_names[NUM_KEYS];
extern const int key_midi_mapping[NUM_KEYS];
extern const compat_key_list compat_keys[NUM_KEYS];
extern const char *s_source_enum_to_name[];
extern const char *s_dest_enum_to_name[];

// defined in minisynth_voice.h
const wchar_t *s_mode_names[] = {L"SAW3",    L"SQR3",    L"SAW2SQR",
                                 L"TRI2SAW", L"TRI2SQR", L"SIN2SQR"};

// defined in oscillator.h
const char *s_lfo_mode_names[] = {"SINE",   "USAW", "DSAW", "TRI",
                                  "SQUARE", "EXPO", "RSH",  "QRSH"};

const char *s_filter_type_names[] = {"LPF1", "HPF1", "LPF2", "HPF2", "BPF2",
                                     "BSF2", "LPF4", "HPF4", "BPF4"};

const char *arp_mode_to_string[] = {"UP", "DOWN", "UPDOWN", "RANDOM"};
const char *arp_cur_step_to_string[] = {"ROOT", "THIRD", "FIFTH"};
const char *arp_rate_to_string[] = {"THIRTYSECOND", "SIXTEENTH", "EIGHTH",
                                    "QUARTER"};

minisynth *new_minisynth(void)
{
    minisynth *ms = (minisynth *)calloc(1, sizeof(minisynth));
    if (ms == NULL)
        return NULL; // barf

    ms->num_melodies = 1;

    ms->sound_generator.gennext = &minisynth_gennext;
    ms->sound_generator.status = &minisynth_status;
    ms->sound_generator.setvol = &minisynth_setvol;
    ms->sound_generator.getvol = &minisynth_getvol;
    ms->sound_generator.start = &minisynth_sg_start;
    ms->sound_generator.stop = &minisynth_sg_stop;
    ms->sound_generator.get_num_tracks = &minisynth_get_num_tracks;
    ms->sound_generator.make_active_track = &minisynth_make_active_track;
    ms->sound_generator.type = SYNTH_TYPE;

    strncpy(ms->m_settings.m_settings_name, "DEFAULT", 7);

    ms->m_settings.m_voice_mode = 0;
    ms->m_settings.m_detune_cents = 0.0;

    ms->m_settings.m_lfo1_waveform = 0;
    ms->m_settings.m_lfo1_rate = DEFAULT_LFO_RATE;
    ms->m_settings.m_lfo1_amplitude = 1.0;
    ms->m_settings.m_lfo1_amp_intensity = 0.0;
    ms->m_settings.m_lfo1_osc_pitch_intensity = 0.0;
    ms->m_settings.m_lfo1_filter_fc_intensity = 0.0;
    ms->m_settings.m_lfo1_pan_intensity = 0.0;

    ms->m_settings.m_lfo2_waveform = 0;
    ms->m_settings.m_lfo2_rate = DEFAULT_LFO_RATE;
    ms->m_settings.m_lfo2_amplitude = 1.0;
    ms->m_settings.m_lfo2_amp_intensity = 0.0;
    ms->m_settings.m_lfo2_osc_pitch_intensity = 0.0;
    ms->m_settings.m_lfo2_filter_fc_intensity = 0.0;
    ms->m_settings.m_lfo2_pan_intensity = 0.0;

    ms->m_settings.m_fc_control = FILTER_FC_DEFAULT;
    ms->m_settings.m_q_control = FILTER_Q_DEFAULT;

    ms->m_settings.m_eg1_dca_intensity = 1.0;
    ms->m_settings.m_eg1_osc_intensity = 0.0;
    ms->m_settings.m_eg1_filter_intensity = 0.0;
    ms->m_settings.m_attack_time_msec = EG_DEFAULT_STATE_TIME;
    ms->m_settings.m_delay_time_msec = EG_DEFAULT_STATE_TIME;
    ms->m_settings.m_decay_time_msec = EG_DEFAULT_STATE_TIME;
    ms->m_settings.m_release_time_msec = EG_DEFAULT_STATE_TIME;

    ms->m_settings.m_pulse_width_pct = OSC_PULSEWIDTH_DEFAULT;
    ms->m_settings.m_delay_feedback_pct = 0;
    ms->m_settings.m_delay_ratio = 0;
    ms->m_settings.m_delay_wet_mix = 0.0;
    ms->m_settings.m_octave = 1;
    ms->m_settings.m_portamento_time_msec = DEFAULT_PORTAMENTO_TIME_MSEC;
    ms->m_settings.m_sub_osc_db = -96.000000;

    ms->m_settings.m_sustain_level = 0.9;
    ms->m_settings.m_noise_osc_db = -96.000000;
    ms->m_settings.m_volume_db = 0.7;
    ms->m_settings.m_legato_mode = DEFAULT_LEGATO_MODE;
    ms->m_settings.m_pitchbend_range = 1;
    ms->m_settings.m_reset_to_zero = DEFAULT_RESET_TO_ZERO;
    ms->m_settings.m_filter_keytrack = DEFAULT_FILTER_KEYTRACK;
    ms->m_settings.m_filter_type = FILTER_TYPE_DEFAULT;
    ms->m_settings.m_filter_keytrack_intensity =
        DEFAULT_FILTER_KEYTRACK_INTENSITY;

    ms->m_settings.m_velocity_to_attack_scaling = 0;
    ms->m_settings.m_note_number_to_decay_scaling = 0;
    ms->m_settings.m_delay_mode = 0;

    ms->m_settings.m_sustain_override = false;
    ms->m_settings.m_sustain_time_ms = 400;
    ms->m_settings.m_sustain_time_sixteenth = 4;

    ms->m_last_midi_note = 0;
    ms->morph_mode = false;
    ms->morph_every_n_loops = 0;
    ms->morph_generation = 0;
    ms->max_generation = 0;

    ms->multi_melody_mode = true;
    ms->cur_melody_iteration = 1;

    for (int i = 0; i < MAX_VOICES; i++) {
        ms->m_voices[i] = new_minisynth_voice();
        if (!ms->m_voices[i])
            return NULL; // would be bad

        minisynth_voice_init_global_parameters(ms->m_voices[i],
                                               &ms->m_global_synth_params);
    }

    // clears out modmatrix sources and resets all oscs, lfos, eg's etc.
    minisynth_prepare_for_play(ms);

    // use first voice to setup global
    minisynth_voice_initialize_modmatrix(ms->m_voices[0], &ms->m_ms_modmatrix);

    for (int i = 0; i < MAX_VOICES; i++) {
        voice_set_modmatrix_core(&ms->m_voices[i]->m_voice,
                                 get_matrix_core(&ms->m_ms_modmatrix));
    }
    for (int i = 0; i < MAX_NUM_MIDI_LOOPS; i++) {
        ms->melody_multiloop_count[i] = 1;
        for (int j = 0; j < PPNS; j++) {
            ms->melodies[i][j] = NULL;
        }
    }

    minisynth_update(ms);
    arpeggiator_init(&ms->m_arp);

    ms->active = true;
    return ms;
}

////////////////////////////////////

bool minisynth_prepare_for_play(minisynth *ms)
{
    for (int i = 0; i < MAX_VOICES; i++) {
        if (ms->m_voices[i]) {
            minisynth_voice_prepare_for_play(ms->m_voices[i]);
        }
    }

    stereo_delay_prepare_for_play(&ms->m_delay_fx);

    minisynth_update(ms);
    ms->m_last_note_frequency = -1.0;

    return true;
}

void minisynth_update(minisynth *ms)
{
    ms->m_global_synth_params.voice_params.voice_mode =
        ms->m_settings.m_voice_mode;
    ms->m_global_synth_params.voice_params.portamento_time_msec =
        ms->m_settings.m_portamento_time_msec;

    ms->m_global_synth_params.voice_params.osc_fo_pitchbend_mod_range =
        ms->m_settings.m_pitchbend_range;

    // --- intensities
    ms->m_global_synth_params.voice_params.filter_keytrack_intensity =
        ms->m_settings.m_filter_keytrack_intensity;
    ms->m_global_synth_params.voice_params.lfo1_filter1_mod_intensity =
        ms->m_settings.m_lfo1_filter_fc_intensity;
    ms->m_global_synth_params.voice_params.lfo1_osc_mod_intensity =
        ms->m_settings.m_lfo1_osc_pitch_intensity;
    ms->m_global_synth_params.voice_params.lfo1_dca_amp_mod_intensity =
        ms->m_settings.m_lfo1_amp_intensity;
    ms->m_global_synth_params.voice_params.lfo1_dca_pan_mod_intensity =
        ms->m_settings.m_lfo1_pan_intensity;

    ms->m_global_synth_params.voice_params.eg1_osc_mod_intensity =
        ms->m_settings.m_eg1_osc_intensity;
    ms->m_global_synth_params.voice_params.eg1_filter1_mod_intensity =
        ms->m_settings.m_eg1_filter_intensity;
    ms->m_global_synth_params.voice_params.eg1_dca_amp_mod_intensity =
        ms->m_settings.m_eg1_dca_intensity;

    // --- oscillators:
    double noise_amplitude =
        ms->m_settings.m_noise_osc_db == -96.0
            ? 0.0
            : pow(10.0, ms->m_settings.m_noise_osc_db / 20.0);
    double sub_amplitude = ms->m_settings.m_sub_osc_db == -96.0
                               ? 0.0
                               : pow(10.0, ms->m_settings.m_sub_osc_db / 20.0);

    // --- osc3 is sub osc
    ms->m_global_synth_params.osc3_params.amplitude = sub_amplitude;

    // --- osc4 is noise osc
    ms->m_global_synth_params.osc4_params.amplitude = noise_amplitude;

    // --- pulse width
    ms->m_global_synth_params.osc1_params.pulse_width_control =
        ms->m_settings.m_pulse_width_pct;
    ms->m_global_synth_params.osc2_params.pulse_width_control =
        ms->m_settings.m_pulse_width_pct;
    ms->m_global_synth_params.osc3_params.pulse_width_control =
        ms->m_settings.m_pulse_width_pct;

    // --- octave
    ms->m_global_synth_params.osc1_params.octave = ms->m_settings.m_octave;
    ms->m_global_synth_params.osc2_params.octave = ms->m_settings.m_octave;
    ms->m_global_synth_params.osc3_params.octave =
        ms->m_settings.m_octave - 1; // sub-oscillator

    // --- detuning for minisynth
    ms->m_global_synth_params.osc1_params.cents = ms->m_settings.m_detune_cents;
    ms->m_global_synth_params.osc2_params.cents =
        -ms->m_settings.m_detune_cents;
    // no detune on 3rd oscillator

    // --- filter:
    ms->m_global_synth_params.filter1_params.fc_control =
        ms->m_settings.m_fc_control;
    ms->m_global_synth_params.filter1_params.q_control =
        ms->m_settings.m_q_control;
    ms->m_global_synth_params.filter1_params.filter_type =
        ms->m_settings.m_filter_type;
    ms->m_global_synth_params.filter1_params.saturation =
        ms->m_settings.m_filter_saturation;
    ms->m_global_synth_params.filter1_params.nlp = ms->m_settings.m_nlp;

    // --- lfo1:
    ms->m_global_synth_params.lfo1_params.waveform =
        ms->m_settings.m_lfo1_waveform;
    ms->m_global_synth_params.lfo1_params.amplitude =
        ms->m_settings.m_lfo1_amplitude;
    ms->m_global_synth_params.lfo1_params.osc_fo = ms->m_settings.m_lfo1_rate;

    // --- eg1:
    ms->m_global_synth_params.eg1_params.attack_time_msec =
        ms->m_settings.m_attack_time_msec;
    ms->m_global_synth_params.eg1_params.decay_time_msec =
        ms->m_settings.m_decay_time_msec;
    ms->m_global_synth_params.eg1_params.sustain_level =
        ms->m_settings.m_sustain_level;
    ms->m_global_synth_params.eg1_params.release_time_msec =
        ms->m_settings.m_release_time_msec;
    ms->m_global_synth_params.eg1_params.reset_to_zero =
        (bool)ms->m_settings.m_reset_to_zero;
    ms->m_global_synth_params.eg1_params.legato_mode =
        (bool)ms->m_settings.m_legato_mode;

    // --- dca:
    ms->m_global_synth_params.dca_params.amplitude_db =
        ms->m_settings.m_volume_db;

    // --- enable/disable mod matrix stuff
    if (ms->m_settings.m_velocity_to_attack_scaling == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_VELOCITY,
                          DEST_ALL_EG_ATTACK_SCALING, true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_VELOCITY,
                          DEST_ALL_EG_ATTACK_SCALING, false);

    if (ms->m_settings.m_note_number_to_decay_scaling == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_MIDI_NOTE_NUM,
                          DEST_ALL_EG_DECAY_SCALING, true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_MIDI_NOTE_NUM,
                          DEST_ALL_EG_DECAY_SCALING, false);

    if (ms->m_settings.m_filter_keytrack == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_MIDI_NOTE_NUM,
                          DEST_ALL_FILTER_KEYTRACK, true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_MIDI_NOTE_NUM,
                          DEST_ALL_FILTER_KEYTRACK, false);

    // // --- update master FX delay
    stereo_delay_set_delay_time_ms(&ms->m_delay_fx,
                                   ms->m_settings.m_delay_time_msec);
    stereo_delay_set_feedback_percent(&ms->m_delay_fx,
                                      ms->m_settings.m_delay_feedback_pct);
    stereo_delay_set_delay_ratio(&ms->m_delay_fx, ms->m_settings.m_delay_ratio);
    stereo_delay_set_wet_mix(&ms->m_delay_fx, ms->m_settings.m_delay_wet_mix);
    stereo_delay_set_mode(&ms->m_delay_fx, ms->m_settings.m_delay_mode);
    stereo_delay_update(&ms->m_delay_fx);
}

void minisynth_generate_melody(minisynth *ms, int melody_num, int max_notes,
                               int max_steps)
{
    if (!is_valid_melody_num(ms, melody_num)) {
        printf("Not a valid melody number\n");
        return;
    }
    minisynth_reset_melody(ms, melody_num);

    if (max_notes == 0)
        max_notes = 5;
    if (max_steps == 0)
        max_steps = 9;

    // printf("MAX NOTES %d MAX STEPS: %d\n", max_notes, max_steps);
    int rand_num_notes = (rand() % max_notes);
    if (rand_num_notes == 0)
        rand_num_notes = 1;
    int generated_melody_note_num[NUM_COMPAT_NOTES];
    for (int i = 0; i < NUM_COMPAT_NOTES; i++)
        generated_melody_note_num[i] = -99;
    int generated_melody_note_num_idx = 0;
    while (generated_melody_note_num_idx < rand_num_notes) {
        int randy = rand() % (NUM_COMPAT_NOTES);
        if (!is_int_member_in_array(randy, generated_melody_note_num,
                                    NUM_COMPAT_NOTES)) {
            generated_melody_note_num[generated_melody_note_num_idx++] = randy;
        }
    }

    // printf("They are: ");
    // for (int i = 0; i < NUM_COMPAT_NOTES; i++)
    //    if (generated_melody_note_num[i] != -99)
    //        printf("%d ", generated_melody_note_num[i]);
    // printf("\n");

    for (int i = 0; i < NUM_COMPAT_NOTES; i++) {
        if (generated_melody_note_num[i] != -99) {
            int idx = generated_melody_note_num[i];
            // printf("Compat: %s\n", key_names[compat_keys[0][idx]]);
            // printf("Compat: %d\n", key_midi_mapping[compat_keys[0][idx]]);

            int rand_steps = (rand() % max_steps) + 1;
            int bitpattern = create_euclidean_rhythm(rand_steps, 32);
            if (rand() % 2 == 1)
                bitpattern = shift_bits_to_leftmost_position(bitpattern, 32);

            // printf("Pattern is %d\n", bitpattern);
            char cbitpattern[33];
            for (int i = 31; i >= 0; i--) {
                if (bitpattern & 1 << i) {
                    cbitpattern[31 - i] = '1';
                    minisynth_add_note(
                        ms, melody_num, 31 - i,
                        key_midi_mapping[compat_keys[mixr->key][idx]]); // THIS!
                }
                else
                    cbitpattern[31 - i] = '0';
            }
            cbitpattern[32] = '\0';
            // printf("Pattern is %d - %s\n", bitpattern, cbitpattern);
        }
    }
}

bool minisynth_midi_note_on(minisynth *ms, unsigned int midinote,
                            unsigned int velocity)
{
    bool steal_note = true;
    for (int i = 0; i < MAX_VOICES; i++) {
        minisynth_voice *msv = ms->m_voices[i];
        if (!msv)
            return false; // should never happen
        if (!msv->m_voice.m_note_on) {
            minisynth_increment_voice_timestamps(ms);
            voice_note_on(&msv->m_voice, midinote, velocity,
                          get_midi_freq(midinote), ms->m_last_note_frequency);

            ms->m_last_note_frequency = get_midi_freq(midinote);
            steal_note = false;
            break;
        }
    }

    if (steal_note) {
        if (mixr->debug_mode)
            printf("STEAL NOTE\n");
        minisynth_voice *msv = minisynth_get_oldest_voice(ms);
        if (msv) {
            minisynth_increment_voice_timestamps(ms);
            voice_note_on(&msv->m_voice, midinote, velocity,
                          get_midi_freq(midinote), ms->m_last_note_frequency);
        }
        ms->m_last_note_frequency = get_midi_freq(midinote);
    }

    return true;
}

bool minisynth_midi_note_off(minisynth *ms, unsigned int midinote,
                             unsigned int velocity, bool all_notes_off)
{
    (void)velocity;

    if (all_notes_off) {
        for (int i = 0; i < MAX_VOICES; i++) {
            if (ms->m_voices[i])
                voice_note_off(&ms->m_voices[i]->m_voice, midinote);
        }
    }

    for (int i = 0; i < MAX_VOICES; i++) {
        minisynth_voice *msv =
            minisynth_get_oldest_voice_with_note(ms, midinote);
        if (msv) {
            voice_note_off(&msv->m_voice, midinote);
        }
    }
    return true;
}

void minisynth_midi_control(minisynth *ms, unsigned int data1,
                            unsigned int data2)
{
    double scaley_val;
    // switch (mixr->m_midi_controller_mode) {
    // case MIDI_KNOB_MODE_ONE:
    switch (data1) {
    case 9:
        ms->m_settings.m_lfo1_waveform =
            (++ms->m_settings.m_lfo1_waveform) % MAX_LFO_OSC;
        printf("LFO! Mode Toggle: %d MaxLFO: %d\n",
               ms->m_settings.m_lfo1_waveform, MAX_LFO_OSC);
        break;
    case 10:
        ms->m_settings.m_voice_mode = (++ms->m_settings.m_voice_mode) % 5;
        printf("Voice! Mode Toggle - %ls\n",
               s_mode_names[ms->m_settings.m_voice_mode]);
        break;
    case 11:
        ms->m_settings.m_legato_mode = 1 - ms->m_settings.m_legato_mode;
        printf("Legato! Mode %d\n", ms->m_settings.m_legato_mode);
        break;
    case 12:
        ms->m_settings.m_reset_to_zero = 1 - ms->m_settings.m_reset_to_zero;
        printf("Reset To Zero! Mode\n");
        break;
    case 13:
        ms->m_settings.m_filter_keytrack = 1 - ms->m_settings.m_filter_keytrack;
        printf("Filter Keytrack! Mode\n");
        break;
    case 14:
        ms->m_settings.m_velocity_to_attack_scaling =
            1 - ms->m_settings.m_velocity_to_attack_scaling;
        printf("Velocity To Attack! Mode %d\n",
               ms->m_settings.m_velocity_to_attack_scaling);
        break;
    case 15:
        ms->m_settings.m_note_number_to_decay_scaling =
            1 - ms->m_settings.m_note_number_to_decay_scaling;
        printf("Note To Decay Scaling! Mode %d\n",
               ms->m_settings.m_note_number_to_decay_scaling);
        break;
    case 16:
        printf("Toggle! MIDI Knob Modee!\n");
        break;
    /// BANK B on MPK Mini MKII
    case 17:
        printf("Delay Mode! Mode\n");
        break;
    case 18:
        printf("Sustain Override! Mode\n");
        break;
    case 19:
        printf("19! \n");
        break;
    case 20:
        printf("20! Mode\n");
        break;
    case 21:
        printf("21! MIDI Mode\n");
        break;
    case 22:
        printf("22! MIDI Mode\n");
        break;
    case 23:
        printf("23! MIDI Mode\n");
        break;
    case 24:
        printf("24! MIDI Mode\n");
        break;
    case 1: // K1 - Envelope Attack Time Msec
        if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_ONE) {
            if (mixr->debug_mode)
                printf("Envelope attack!\n");
            scaley_val = scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
            ms->m_settings.m_attack_time_msec = scaley_val;
        }
        else if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_TWO) {
            scaley_val = scaleybum(0, 127, 0, 1000, data2);
            ms->m_settings.m_delay_time_msec = scaley_val;
            if (mixr->debug_mode)
                printf("DELAY MS: %f\n", ms->m_settings.m_delay_time_msec);
        }
        else if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_THREE) {
            if (mixr->debug_mode)
                printf("EG1->DCA intensity!\n");
            scaley_val = scaleybum(0, 127, 0, 10, data2);
            ms->m_settings.m_eg1_dca_intensity = scaley_val;
        }
        break;
    case 2: // K2 - Envelope Decay Time Msec
        if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_ONE) {
            if (mixr->debug_mode)
                printf("Envelope decay!\n");
            scaley_val = scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
            ms->m_settings.m_decay_time_msec = scaley_val;
        }
        else if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_TWO) {
            scaley_val = scaleybum(0, 127, 0, 10, data2);
            // ms->X = scaley_val;
        }
        else if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_THREE) {
            // scaley_val = scaleybum(0, 127, X, X, data2);
            // ms->X = scaley_val;
        }
        break;
    case 3: // K3 - Envelope Sustain Level
        if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_ONE) {
            if (mixr->debug_mode)
                printf("Env Sustain level\n");
            scaley_val = scaleybum(0, 127, 0, 1, data2);
            ms->m_settings.m_sustain_level = scaley_val;
        }
        else if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_TWO) {
            // scaley_val = scaleybum(0, 127, X, X, data2);
            // ms->X = scaley_val;
        }
        else if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_THREE) {
            // scaley_val = scaleybum(0, 127, X, X, data2);
            // ms->X = scaley_val;
        }
        break;
    case 4: // K4 - Synth Volume
        if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_ONE) {
            printf("Synth volume\n");
            scaley_val = scaleybum(0, 127, 0, 1, data2);
            ms->m_settings.m_volume_db = scaley_val;
        }
        else if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_TWO) {
            //    scaley_val = scaleybum(0, 127, X, X, data2);
            //    ms->X = scaley_val;
        }
        else if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_THREE) {
            //    scaley_val = scaleybum(0, 127, X, X, data2);
            //    ms->X = scaley_val;
        }
        break;
    case 5: // K6 - LFO amplitude
        if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_ONE) {
            if (mixr->debug_mode)
                printf("LFO Amplitude\n");
            scaley_val = scaleybum(0, 128, 0.0, 1.0, data2);
            ms->m_settings.m_lfo1_amplitude = scaley_val;
        }
        else if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_TWO) {
            //    scaley_val = scaleybum(0, 127, X, X, data2);
            //    ms->X = scaley_val;
        }
        else if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_THREE) {
            //    scaley_val = scaleybum(0, 127, X, X, data2);
            //    ms->X = scaley_val;
        }
        break;
    case 6: // K5 - LFO rate
        if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_ONE) {
            if (mixr->debug_mode)
                printf("LFO Rate\n");
            scaley_val = scaleybum(0, 128, MIN_LFO_RATE, MAX_LFO_RATE, data2);
            ms->m_settings.m_lfo1_rate = scaley_val;
        }
        else if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_TWO) {
            //    scaley_val = scaleybum(0, 127, X, X, data2);
            //    ms->X = scaley_val;
        }
        else if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_THREE) {
            //    scaley_val = scaleybum(0, 127, X, X, data2);
            //    ms->X = scaley_val;
        }
        break;
    case 7: // K7 - Filter Frequency Cut
        if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_ONE) {
            if (mixr->debug_mode)
                printf("Filter Frequency!\n");
            scaley_val = scaleybum(0, 127, FILTER_FC_MIN, FILTER_FC_MAX, data2);
            ms->m_settings.m_fc_control = scaley_val;
        }
        else if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_TWO) {
            //    scaley_val = scaleybum(0, 127, X, X, data2);
            //    ms->X = scaley_val;
        }
        else if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_THREE) {
            //    scaley_val = scaleybum(0, 127, X, X, data2);
            //    ms->X = scaley_val;
        }
        break;
    case 8: // K8 - Filter Q control
        if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_ONE) {
            if (mixr->debug_mode)
                printf("Filter Q!\n");
            scaley_val = scaleybum(0, 127, 1, 10, data2);
            ms->m_settings.m_q_control = scaley_val;
        }
        else if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_TWO) {
            //    scaley_val = scaleybum(0, 127, X, X, data2);
            //    ms->X = scaley_val;
        }
        else if (ms->m_midi_knob_mode == MIDI_KNOB_MODE_THREE) {
            //    scaley_val = scaleybum(0, 127, X, X, data2);
            //    ms->X = scaley_val;
        }
        break;
    default:
        printf("DATANUM: %d\n", data1);
    }

    minisynth_update(ms);
}

void minisynth_midi_pitchbend(minisynth *ms, unsigned int data1,
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
            ms->m_voices[i]->m_voice.m_osc1->m_cents = scaley_val;
            ms->m_voices[i]->m_voice.m_osc2->m_cents = scaley_val + 2.5;
            ms->m_voices[i]->m_voice.m_osc3->m_cents = scaley_val;
            ms->m_voices[i]->m_voice.m_osc4->m_cents = scaley_val + 2.5;
            ms->m_voices[i]->m_voice.m_v_modmatrix.m_sources[SOURCE_PITCHBEND] =
                normalized_pitch_bent_val;
        }
    }
    else {
        for (int i = 0; i < MAX_VOICES; i++) {
            ms->m_voices[i]->m_voice.m_osc1->m_cents = 0;
            ms->m_voices[i]->m_voice.m_osc2->m_cents = 2.5;
            ms->m_voices[i]->m_voice.m_osc3->m_cents = 0;
            ms->m_voices[i]->m_voice.m_osc4->m_cents = 2.5;
        }
    }
}

void minisynth_set_multi_melody_mode(minisynth *ms, bool melody_mode)
{
    ms->multi_melody_mode = melody_mode;
    ms->cur_melody_iteration = ms->melody_multiloop_count[ms->cur_melody];
}

void minisynth_set_melody_loop_num(minisynth *self, int melody_num,
                                   int loop_num)
{
    self->melody_multiloop_count[melody_num] = loop_num;
}

int minisynth_add_melody(minisynth *ms)
{
    // minisynth_stop(ms);
    // ms->cur_melody++;
    return ms->num_melodies++;
}

void minisynth_dupe_melody(midi_event **from, midi_event **to)
{
    for (int i = 0; i < PPNS; i++) {
        if (to[i] != NULL) {
            midi_event_free(to[i]);
            to[i] = NULL;
        }
        if (from[i] != NULL) {
            midi_event *ev = from[i];
            to[i] =
                new_midi_event(ev->tick, ev->event_type, ev->data1, ev->data2);
        }
    }
}

void minisynth_switch_melody(minisynth *ms, unsigned int melody_num)
{
    if (melody_num < (unsigned)ms->num_melodies) {
        ms->cur_melody = melody_num;
    }
}

void minisynth_reset_melody_all(minisynth *ms)
{
    for (int i = 0; i < ms->num_melodies; i++) {
        minisynth_reset_melody(ms, i);
    }
}

void minisynth_reset_voices(minisynth *ms)
{
    for (int i = 0; i < MAX_VOICES; i++) {
        minisynth_voice_reset(ms->m_voices[i]);
    }
}

void minisynth_reset_melody(minisynth *ms, unsigned int melody_num)
{
    if (melody_num < (unsigned)ms->num_melodies) {
        for (int i = 0; i < PPNS; i++) {
            if (ms->melodies[melody_num][i] != NULL) {
                midi_event *tmp = ms->melodies[melody_num][i];
                ms->melodies[melody_num][i] = NULL;
                free(tmp);
            }
        }
    }
    minisynth_stop(ms);
}

void minisynth_melody_to_string(minisynth *ms, int melody_num,
                                wchar_t melodystr[33])
{
    int cur_quart = 0;
    for (int i = 0; i < PPNS; i += PPSIXTEENTH) {
        melodystr[cur_quart] = sparkchars[0];
        for (int j = i; j < (i + PPSIXTEENTH); j++) {
            if (ms->melodies[melody_num][j] != NULL &&
                ms->melodies[melody_num][j]->event_type ==
                    144) { // 144 is midi note on
                melodystr[cur_quart] = sparkchars[5];
            }
        }
        cur_quart++;
    }
}

void minisynth_increment_voice_timestamps(minisynth *ms)
{
    for (int i = 0; i < MAX_VOICES; i++) {
        if (ms->m_voices[i]) {
            if (ms->m_voices[i]->m_voice.m_note_on)
                ms->m_voices[i]->m_voice.m_timestamp++;
        }
    }
}

minisynth_voice *minisynth_get_oldest_voice(minisynth *ms)
{
    int timestamp = -1;
    minisynth_voice *found_voice = NULL;
    for (int i = 0; i < MAX_VOICES; i++) {
        if (ms->m_voices[i]) {
            if (ms->m_voices[i]->m_voice.m_note_on &&
                (int)ms->m_voices[i]->m_voice.m_timestamp > timestamp) {
                found_voice = ms->m_voices[i];
                timestamp = (int)ms->m_voices[i]->m_voice.m_timestamp;
            }
        }
    }
    return found_voice;
}

minisynth_voice *minisynth_get_oldest_voice_with_note(minisynth *ms,
                                                      unsigned int midi_note)
{
    int timestamp = -1;
    minisynth_voice *found_voice = NULL;
    for (int i = 0; i < MAX_VOICES; i++) {
        if (ms->m_voices[i]) {
            if (voice_can_note_off(&ms->m_voices[i]->m_voice) &&
                (int)ms->m_voices[i]->m_voice.m_timestamp > timestamp &&
                ms->m_voices[i]->m_voice.m_midi_note_number == midi_note) {
                found_voice = ms->m_voices[i];
                timestamp = (int)ms->m_voices[i]->m_voice.m_timestamp;
            }
        }
    }
    return found_voice;
}

// sound generator interface //////////////
void minisynth_status(void *self, wchar_t *status_string)
{
    minisynth *ms = (minisynth *)self;

    if (mixr->debug_mode) {
        minisynth_print(ms);
    }

    swprintf(
        status_string, MAX_PS_STRING_SZ, WCOOL_COLOR_PINK
        "[SYNTH '%s'] - Vol: %.2f Active: %s Multi: %s, Morph: %s, Morph "
        "EveryN: %d "
        "Morph Generation: %d CurMelody:%d DelayMode: %d Mode: %ls"
        "\n      A:%.2f D:%.2f R:%.2f S:%.2f Amp: %2.f LFO1 amp: %.2f "
        "rate:%.2f "
        "SustainOverride: %s Sustain Time(ms): %.2f Filter FC: %.2f Filter Q: "
        "%2.f"
        "\n      Delay ms: %.2f Feedback Pct:%.2f Delay Ratio: %.2f Wet Mix: "
        "%2.f"
        "\n      Detune Cents: %.2f Pulse Width Pct:%.2f SubOsc Db: %.2f "
        "NoiseOsc Db: %2.f EG1sus:%d Eg2sus:%d Eg3sus:%d Eg4sus:%d",
        ms->m_settings.m_settings_name, ms->m_settings.m_volume_db,
        ms->active ? "true" : "false", ms->multi_melody_mode ? "true" : "false",
        ms->morph_mode ? "true" : "false", ms->morph_every_n_loops,
        ms->morph_generation, ms->cur_melody, ms->m_settings.m_delay_mode,
        s_mode_names[ms->m_settings.m_voice_mode],
        ms->m_settings.m_attack_time_msec, ms->m_settings.m_decay_time_msec,
        ms->m_settings.m_release_time_msec, ms->m_settings.m_sustain_level,
        ms->m_settings.m_volume_db, ms->m_settings.m_lfo1_amplitude,
        ms->m_settings.m_lfo1_rate,
        ms->m_settings.m_sustain_override ? "true" : "false",
        ms->m_settings.m_sustain_time_ms,
        ms->m_voices[0]->m_moog_ladder_filter.f.m_fc,
        ms->m_settings.m_q_control, ms->m_settings.m_delay_time_msec,
        ms->m_settings.m_delay_feedback_pct, ms->m_settings.m_delay_ratio,
        ms->m_settings.m_delay_wet_mix, ms->m_settings.m_detune_cents,
        ms->m_settings.m_pulse_width_pct, ms->m_settings.m_sub_osc_db,
        ms->m_settings.m_noise_osc_db,
        ms->m_voices[0]->m_voice.m_eg1.m_sustain_override,
        ms->m_voices[0]->m_voice.m_eg2.m_sustain_override,
        ms->m_voices[0]->m_voice.m_eg3.m_sustain_override,
        ms->m_voices[0]->m_voice.m_eg4.m_sustain_override);

    for (int i = 0; i < ms->num_melodies; i++) {
        wchar_t melodystr[33] = {0};
        wchar_t scratch[128] = {0};
        minisynth_melody_to_string(ms, i, melodystr);
        swprintf(scratch, 127, L"\n      [%d]  %ls  numloops: %d", i, melodystr,
                 ms->melody_multiloop_count[i]);
        wcscat(status_string, scratch);
    }
    wcscat(status_string, WANSI_COLOR_RESET);
}

void minisynth_setvol(void *self, double v)
{
    minisynth *ms = (minisynth *)self;
    if (v < 0.0 || v > 1.0) {
        return;
    }
    ms->m_settings.m_volume_db = v;
}

double minisynth_getvol(void *self)
{
    minisynth *ms = (minisynth *)self;
    return ms->m_settings.m_volume_db;
}

// void minisynth_gennext(void* self, double* frame_vals, int framesPerBuffer);
double minisynth_gennext(void *self)
{

    minisynth *ms = (minisynth *)self;

    if (!ms->active)
        return 0.0;

    if (mixr->sixteenth_note_tick != ms->tick) {
        ms->tick = mixr->sixteenth_note_tick;
        // if (mixr->start_of_loop) {
        if (ms->tick % 64 == 0) {
            if (ms->generate_mode) {
                if (ms->generate_every_n_loops > 0) {
                    if (ms->generate_generation % ms->generate_every_n_loops ==
                        0) {
                        minisynth_set_backup_mode(ms, true);
                        minisynth_generate_melody(ms, 0, 0, 0);
                    }
                    else {
                        minisynth_set_backup_mode(ms, false);
                    }
                }
                else if (ms->max_generation > 0) {
                    if (ms->morph_generation >= ms->max_generation) {
                        ms->morph_generation = 0;
                        minisynth_set_generate_mode(ms, false);
                        minisynth_set_backup_mode(ms, false);
                    }
                }
                else {
                    minisynth_generate_melody(ms, 0, 0, 0);
                }
                ms->generate_generation++;
            }
        }
    }

    if (mixr->is_midi_tick) {
        int idx = mixr->midi_tick % PPNS;
        // top of the MS loop, which is two bars, check if we need to progress
        // to next loop
        if (idx == 0) {
            if (ms->multi_melody_mode && ms->num_melodies > 1) {
                ms->cur_melody_iteration--;
                if (ms->cur_melody_iteration == 0) {
                    int next_melody = (ms->cur_melody + 1) % ms->num_melodies;
                    if (!ms->m_settings.m_sustain_override) {
                        for (int i = 0; i < PPNS; i++) {
                            if (ms->melodies[ms->cur_melody][i] != NULL) {
                                midi_event *ev =
                                    ms->melodies[ms->cur_melody][i];
                                // COPY all note off events
                                if (ev->event_type == 128) {
                                    midi_event *tmp =
                                        new_midi_event(ev->tick, ev->event_type,
                                                       ev->data1, ev->data2);
                                    tmp->delete_after_use = true;
                                    minisynth_add_event(ms, next_melody, tmp);
                                }
                            }
                        }
                    }
                    ms->cur_melody = next_melody;
                    ms->cur_melody_iteration =
                        ms->melody_multiloop_count[ms->cur_melody];
                }
            }
        }

        if (ms->melodies[ms->cur_melody][idx] != NULL) {
            midi_event *ev = ms->melodies[ms->cur_melody][idx];
            midi_parse_midi_event(ms, ev);
        }
    }

    minisynth_update(ms);

    if (ms->m_arp.active) {
        arpeggiate(ms, &ms->m_arp);
    }
    double accum_out_left = 0.0;
    double accum_out_right = 0.0;

    float mix = 0.25;

    double out_left = 0.0;
    double out_right = 0.0;

    for (int i = 0; i < MAX_VOICES; i++) {
        if (ms->m_voices[i]) {
            minisynth_voice_gennext(ms->m_voices[i], &out_left, &out_right);
        }
        accum_out_left += mix * out_left;
        accum_out_right += mix * out_right;
    }

    stereo_delay_process_audio(&ms->m_delay_fx, &accum_out_left,
                               &accum_out_left, &accum_out_left,
                               &accum_out_right);

    accum_out_left = effector(&ms->sound_generator, accum_out_left);
    accum_out_left = envelopor(&ms->sound_generator, accum_out_left);

    accum_out_left *= ms->m_settings.m_volume_db;

    return accum_out_left;
}

midi_event **minisynth_get_midi_loop(minisynth *self)
{
    return self->melodies[self->cur_melody];
}

int minisynth_add_event(minisynth *ms, int melody_num, midi_event *ev)
{
    int tick = ev->tick;
    while (ms->melodies[melody_num][tick] != NULL) {
        if (mixr->debug_mode)
            printf("Gotsz a tick already - bump!\n");
        tick++;
        if (tick == PPNS) // wrap around
            tick = 0;
    }
    ev->tick = tick;
    ms->melodies[melody_num][tick] = ev;
    return tick;
}

void minisynth_clear_melody_ready_for_new_one(minisynth *ms, int melody_num)
{
    for (int i = 0; i < PPNS; i++) {
        if (ms->melodies[melody_num][i] != NULL) {
            midi_event *ev = ms->melodies[melody_num][i];
            if (ev->event_type == 144) {
                free(ms->melodies[melody_num][i]);
                ms->melodies[melody_num][i] = NULL;
            }
        }
    }
}

midi_event **minisynth_copy_midi_loop(minisynth *self, int melody_num)
{
    if (melody_num >= self->num_melodies) {
        printf("Dingjie!\n");
        return NULL;
    }
    // midi_event_loop defined in midimaaan.h
    midi_event **new_midi_events_loop =
        (midi_event **)calloc(PPNS, sizeof(midi_event *));
    for (int i = 0; i < PPNS; i++) {
        if (self->melodies[melody_num][i] != NULL) {
            midi_event *ev = self->melodies[melody_num][i];
            new_midi_events_loop[i] =
                new_midi_event(ev->tick, ev->event_type, ev->data1, ev->data2);
        }
    }

    return new_midi_events_loop;
}

void minisynth_add_midi_loop(minisynth *ms, midi_event **events, int melody_num)
{
    if (melody_num >= MAX_NUM_MIDI_LOOPS) {
        printf("Dingjie!\n");
        return;
    }
    for (int i = 0; i < PPNS; i++) {
        if (events[i] != NULL)
            ms->melodies[melody_num][i] = events[i];
    }
    ms->num_melodies++;
    ms->cur_melody++;
    free(events); // get rid of container
    printf("Added new Melody\n");
}

void minisynth_replace_midi_loop(minisynth *ms, midi_event **events,
                                 int melody_num)
{
    if (melody_num >= MAX_NUM_MIDI_LOOPS) {
        printf("Dingjie!\n");
        return;
    }
    for (int i = 0; i < PPNS; i++) {
        if (ms->melodies[melody_num][i] != NULL) {
            free(ms->melodies[melody_num][i]);
            ms->melodies[melody_num][i] = NULL;
        }
        if (events[i] != NULL)
            ms->melodies[melody_num][i] = events[i];
    }
    free(events); // get rid of container
    printf("Replaced Melody %d\n", melody_num);
}

void minisynth_toggle_delay_mode(minisynth *ms)
{
    ms->m_settings.m_delay_mode =
        ++(ms->m_settings.m_delay_mode) % MAX_NUM_DELAY_MODE;
}

void minisynth_nudge_melody(minisynth *ms, int melody_num, int sixteenth)
{
    if (sixteenth >= 16) {
        printf("Nah, mate, nudge needs to be less than 16\n");
        return;
    }
    int sixteenth_of_loop = PPNS / 16.0;
    midi_event **orig_loop = minisynth_copy_midi_loop(ms, melody_num);

    midi_event **new_midi_events_loop =
        (midi_event **)calloc(PPNS, sizeof(midi_event *));

    for (int i = 0; i < PPNS; i++) {
        if (orig_loop[i] != NULL) {
            midi_event *ev = orig_loop[i];
            int new_tick = (ev->tick + (sixteenth * sixteenth_of_loop)) % PPNS;
            printf("Old tick: %d with new: %d\n", ev->tick, new_tick);
            new_midi_events_loop[new_tick] =
                new_midi_event(new_tick, ev->event_type, ev->data1, ev->data2);
        }
    }
    free(orig_loop);
    minisynth_replace_midi_loop(ms, new_midi_events_loop, melody_num);
}

bool is_valid_melody_num(minisynth *ms, int melody_num)
{
    if (melody_num < ms->num_melodies) {
        return true;
    }
    return false;
}

// TODO - better function name - this is programatic calls, which
// basically adds a matching delete after use event i.e. == a note off
void minisynth_handle_midi_note(minisynth *ms, int note, int velocity,
                                bool update_last_midi)
{
    if (mixr->debug_mode)
        print_midi_event(note);

    if (update_last_midi) {
        ms->m_last_midi_note = note;
    }
    minisynth_midi_note_on(ms, note, velocity);

    int note_off_tick =
        (mixr->midi_tick +
         (PPSIXTEENTH * (int)ms->m_settings.m_sustain_time_sixteenth - 7)) %
        PPNS;

    midi_event *off_event = new_midi_event(note_off_tick, 128, note, velocity);
    ////////////////////////

    if (ms->recording) {
        printf("Recording note!\n");
        int note_on_tick = mixr->midi_tick % PPNS;
        midi_event *on_event =
            new_midi_event(note_on_tick, 144, note, velocity);

        int final_note_off_tick =
            minisynth_add_event(ms, ms->cur_melody, off_event);
        on_event->tick_off = final_note_off_tick;

        minisynth_add_event(ms, ms->cur_melody, on_event);
    }
    else {
        off_event->delete_after_use = true; // _THIS_ is the magic
        minisynth_add_event(ms, ms->cur_melody, off_event);
    }
}

void minisynth_rand_settings(minisynth *ms)
{
    printf("Randomizing SYNTH!\n");

    ms->m_settings.m_voice_mode = rand() % MAX_VOICE_CHOICE;
    ms->m_settings.m_lfo1_waveform = rand() % MAX_LFO_OSC;
    ms->m_settings.m_lfo1_rate =
        ((float)rand()) / RAND_MAX * (MAX_LFO_RATE - MIN_LFO_RATE) +
        MIN_LFO_RATE;
    ms->m_settings.m_lfo1_amplitude = ((float)rand()) / RAND_MAX;
    ms->m_settings.m_lfo1_osc_pitch_intensity =
        (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    ms->m_settings.m_lfo1_filter_fc_intensity =
        (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    // ms->m_settings.m_lfo1_amp_intensity = ((float)rand() /
    // (float)(RAND_MAX));
    // ms->m_settings.m_lfo1_pan_intensity = ((float)rand() /
    // (float)(RAND_MAX));

    ms->m_settings.m_lfo2_waveform = rand() % MAX_LFO_OSC;
    ms->m_settings.m_lfo2_amplitude = ((float)rand()) / RAND_MAX;
    ms->m_settings.m_lfo2_osc_pitch_intensity =
        (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    ms->m_settings.m_lfo2_filter_fc_intensity =
        (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    // ms->m_settings.m_lfo2_amp_intensity = ((float)rand() /
    // (float)(RAND_MAX));
    // ms->m_settings.m_lfo2_pan_intensity = ((float)rand() /
    // (float)(RAND_MAX));

    ms->m_settings.m_detune_cents = (rand() % 200) - 100;

    ms->m_settings.m_fc_control =
        ((float)rand()) / RAND_MAX * (FILTER_FC_MAX - FILTER_FC_MIN) +
        FILTER_FC_MIN;
    ms->m_settings.m_q_control = rand() % 10;

    ms->m_settings.m_attack_time_msec = (rand() % 400) + 50;
    ms->m_settings.m_decay_time_msec = (rand() % 400) + 50;
    ms->m_settings.m_release_time_msec = (rand() % 400) + 50;
    ms->m_settings.m_pulse_width_pct = rand() % 100;

    ms->m_settings.m_delay_ratio =
        (((float)rand() / (float)(RAND_MAX)) * 2.0) - 1;
    ms->m_settings.m_delay_time_msec = rand() % 300;
    ms->m_settings.m_delay_feedback_pct = rand() % 30;
    ms->m_settings.m_delay_wet_mix = rand() % 60;
    minisynth_set_delay_wetmix(ms, ((float)rand() / (float)(RAND_MAX)));

    // ms->m_settings.m_sustain_level = ((float)rand()) / RAND_MAX;
    ms->m_settings.m_octave = rand() % 3 + 1;

    ms->m_settings.m_portamento_time_msec = rand() % 400;

    ms->m_settings.m_sub_osc_db = -1.0 * (rand() % 96);
    // ms->m_settings.m_eg1_osc_intensity =
    //    (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    ms->m_settings.m_eg1_filter_intensity =
        (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    ms->m_settings.m_noise_osc_db = -1.0 * (rand() % 96);

    //////// ms->m_settings.m_volume_db = 1.0;
    ms->m_settings.m_legato_mode = rand() % 2;
    // ms->m_settings.m_pitchbend_range = rand() % 12;
    // ms->m_settings.m_reset_to_zero = rand() % 2;
    // ms->m_settings.m_filter_keytrack = rand() % 2;
    // ms->m_settings.m_filter_keytrack_intensity =
    //    (((float)rand() / (float)(RAND_MAX)) * 10) + 0.51;
    // ms->m_settings.m_velocity_to_attack_scaling = rand() % 2;
    // ms->m_settings.m_note_number_to_decay_scaling = rand() % 2;
    ms->m_settings.m_delay_mode = rand() % MAX_NUM_DELAY_MODE;
    ////ms->m_settings.m_eg1_dca_intensity =
    ////    (((float)rand() / (float)(RAND_MAX)) * 2.0) - 1;
    // ms->m_settings.m_sustain_override = rand() % 2;
    ////ms->m_settings.m_sustain_time_ms = rand() % 1000;
    ////ms->m_settings.m_sustain_time_sixteenth = rand() % 5;

    minisynth_print_settings(ms);
}

bool minisynth_save_settings(minisynth *ms, char *preset_name)
{
    if (strlen(preset_name) == 0) {
        printf("Play tha game, pal, need a name to save yer synth settings "
               "with\n");
        return false;
    }
    printf("Saving '%s' settings for Minisynth to file %s\n", preset_name,
           PRESET_FILENAME);
    FILE *presetzzz = fopen(PRESET_FILENAME, "a");

    fprintf(
        presetzzz, "::%s"    // m_settings_name
                   "::%d"    // m_voice_mode
                   "::%d"    // ms->m_settings.m_lfo1_waveform);
                   "::%d"    // ms->m_settings.m_lfo2_waveform);
                   "::%f"    // ms->m_settings.m_lfo1_rate);
                   "::%f"    // ms->m_settings.m_lfo1_amplitude);
                   "::%f"    // ms->m_settings.m_lfo1_amp_intensity);
                   "::%f"    // ms->m_settings.m_lfo1_filter_fc_intensity);
                   "::%f"    // ms->m_settings.m_lfo1_osc_pitch_intensity);
                   "::%f"    // ms->m_settings.m_lfo1_pan_intensity);
                   "::%f"    // ms->m_settings.m_lfo2_rate);
                   "::%f"    // ms->m_settings.m_lfo2_amplitude);
                   "::%f"    // ms->m_settings.m_lfo2_amp_intensity);
                   "::%f"    // ms->m_settings.m_lfo2_filter_fc_intensity);
                   "::%f"    // ms->m_settings.m_lfo2_osc_pitch_intensity);
                   "::%f"    // ms->m_settings.m_lfo2_pan_intensity);
                   "::%f"    // ms->m_settings.m_detune_cents
                   "::%f"    // ms->m_settings.m_fc_control);
                   "::%f"    // ms->m_settings.m_q_control);
                   "::%f"    // ms->m_settings.m_attack_time_msec);
                   "::%f"    // ms->m_settings.m_delay_time_msec);
                   "::%f"    // ms->m_settings.m_decay_time_msec);
                   "::%f"    // ms->m_settings.m_release_time_msec);
                   "::%f"    // ms->m_settings.m_pulse_width_pct);
                   "::%f"    // ms->m_settings.m_feedback_pct);
                   "::%f"    // ms->m_settings.m_delay_ratio);
                   "::%f"    // ms->m_settings.m_wet_mix);
                   "::%d"    // ms->m_settings.m_octave);
                   "::%f"    // ms->m_settings.m_portamento_time_msec);
                   "::%f"    // ms->m_settings.m_sub_osc_db);
                   "::%f"    // ms->m_settings.m_eg1_osc_intensity);
                   "::%f"    // ms->m_settings.m_eg1_filter_intensity);
                   "::%f"    // ms->m_settings.m_sustain_level);
                   "::%f"    // ms->m_settings.m_noise_osc_db);
                   "::%f"    // ms->m_settings.m_eg1_dca_intensity);
                   "::%f"    // ms->m_settings.m_volume_db);
                   "::%d"    // ms->m_settings.m_legato_mode);
                   "::%d"    // ms->m_settings.m_pitchbend_range);
                   "::%d"    // ms->m_settings.m_reset_to_zero);
                   "::%d"    // ms->m_settings.m_filter_keytrack);
                   "::%f"    // ms->m_settings.m_filter_keytrack_intensity);
                   "::%d"    // ms->m_settings.m_velocity_to_attack_scaling);
                   "::%d"    // ms->m_settings.m_note_number_to_decay_scaling);
                   "::%d"    // ms->m_settings.m_delay_mode);
                   "::%f"    // ms->m_settings.m_eg1_dca_intensity);
                   "::%f"    // ms->m_settings.m_sustain_time_ms);
                   "::%f"    // ms->m_settings.m_sustain_time_sixteenth);
                   "::%d\n", // ms->m_settings.m_sustain_override);
        preset_name, ms->m_settings.m_voice_mode,
        ms->m_settings.m_lfo1_waveform, ms->m_settings.m_lfo2_waveform,
        ms->m_settings.m_lfo1_rate, ms->m_settings.m_lfo1_amplitude,
        ms->m_settings.m_lfo1_amp_intensity,
        ms->m_settings.m_lfo1_filter_fc_intensity,
        ms->m_settings.m_lfo1_osc_pitch_intensity,
        ms->m_settings.m_lfo1_pan_intensity, ms->m_settings.m_lfo2_rate,
        ms->m_settings.m_lfo2_amplitude, ms->m_settings.m_lfo2_amp_intensity,
        ms->m_settings.m_lfo2_filter_fc_intensity,
        ms->m_settings.m_lfo2_osc_pitch_intensity,
        ms->m_settings.m_lfo2_pan_intensity, ms->m_settings.m_detune_cents,
        ms->m_settings.m_fc_control, ms->m_settings.m_q_control,
        ms->m_settings.m_attack_time_msec, ms->m_settings.m_delay_time_msec,
        ms->m_settings.m_decay_time_msec, ms->m_settings.m_release_time_msec,
        ms->m_settings.m_pulse_width_pct, ms->m_settings.m_delay_feedback_pct,
        ms->m_settings.m_delay_ratio, ms->m_settings.m_delay_wet_mix,
        ms->m_settings.m_octave, ms->m_settings.m_portamento_time_msec,
        ms->m_settings.m_sub_osc_db, ms->m_settings.m_eg1_osc_intensity,
        ms->m_settings.m_eg1_filter_intensity, ms->m_settings.m_sustain_level,
        ms->m_settings.m_noise_osc_db, ms->m_settings.m_eg1_dca_intensity,
        ms->m_settings.m_volume_db, ms->m_settings.m_legato_mode,
        ms->m_settings.m_pitchbend_range, ms->m_settings.m_reset_to_zero,
        ms->m_settings.m_filter_keytrack,
        ms->m_settings.m_filter_keytrack_intensity,
        ms->m_settings.m_velocity_to_attack_scaling,
        ms->m_settings.m_note_number_to_decay_scaling,
        ms->m_settings.m_delay_mode, ms->m_settings.m_eg1_dca_intensity,
        ms->m_settings.m_sustain_time_ms,
        ms->m_settings.m_sustain_time_sixteenth,
        ms->m_settings.m_sustain_override);

    fclose(presetzzz);
    return true;
}

bool minisynth_list_presets()
{
    FILE *presetzzz = fopen(PRESET_FILENAME, "r");

    char line[256];

    while (fgets(line, sizeof(line), presetzzz)) {
        printf("%s", line);
    }

    fclose(presetzzz);

    return true;
}

bool minisynth_check_if_preset_exists(char *preset_to_find)
{
    FILE *presetzzz = fopen(PRESET_FILENAME, "r");

    char line[2048];
    char const *sep = "::";
    char *preset_name, *last_s;

    while (fgets(line, sizeof(line), presetzzz)) {
        preset_name = strtok_r(line, sep, &last_s);
        if (strncmp(preset_to_find, preset_name, 255) == 0)
            return true;
    }

    fclose(presetzzz);
    return false;
}
bool minisynth_load_settings(minisynth *ms, char *preset_to_load)
{
    if (strlen(preset_to_load) == 0) {
        printf("Play tha game, pal, need a name to LOAD yer synth settings "
               "with\n");
        return false;
    }

    char line[2048];
    char chompyline[2048];
    FILE *presetzzz = fopen(PRESET_FILENAME, "r");
    char const *sep = "::";
    char *preset_name, *last_s;

    while (fgets(line, sizeof(line), presetzzz)) {
        strcpy(chompyline, line);
        preset_name = strtok_r(line, sep, &last_s);
        if (strncmp(preset_to_load, preset_name, 255) == 0) {
            printf("Found yer PRESET!\n");
            int matches = sscanf(
                chompyline,
                "::%[^:]" // m_settings_name
                "::%d"    // m_voice_mode
                "::%d"    // ms->m_settings.m_lfo1_waveform;
                "::%d"    // ms->m_settings.m_lfo2_waveform;
                "::%lf"   // ms->m_settings.m_lfo1_rate;
                "::%lf"   // ms->m_settings.m_lfo1_amplitude
                "::%lf"   // ms->m_settings.m_lfo1_amp_intensity);
                "::%lf"   // ms->m_settings.m_lfo1_filter_fc_intensity);
                "::%lf"   // ms->m_settings.m_lfo1_osc_pitch_intensity);
                "::%lf"   // ms->m_settings.m_lfo1_pan_intensity);
                "::%lf"   // ms->m_settings.m_lfo2_rate;
                "::%lf"   // ms->m_settings.m_lfo2_amplitude
                "::%lf"   // ms->m_settings.m_lfo2_amp_intensity);
                "::%lf"   // ms->m_settings.m_lfo2_filter_fc_intensity);
                "::%lf"   // ms->m_settings.m_lfo2_osc_pitch_intensity);
                "::%lf"   // ms->m_settings.m_lfo2_pan_intensity);
                "::%lf"   // ms->m_settings.m_detune_cents
                "::%lf"   // ms->m_settings.m_fc_control);
                "::%lf"   // ms->m_settings.m_q_control);
                "::%lf"   // ms->m_settings.m_attack_time_msec);
                "::%lf"   // ms->m_settings.m_delay_time_msec);
                "::%lf"   // ms->m_settings.m_decay_time_msec);
                "::%lf"   // ms->m_settings.m_release_time_msec);
                "::%lf"   // ms->m_settings.m_pulse_width_pct);
                "::%lf"   // ms->m_settings.m_feedback_pct);
                "::%lf"   // ms->m_settings.m_delay_ratio);
                "::%lf"   // ms->m_settings.m_wet_mix);
                "::%d"    // ms->m_settings.m_octave);
                "::%lf"   // ms->m_settings.m_portamento_time_msec);
                "::%lf"   // ms->m_settings.m_sub_osc_db);
                "::%lf"   // ms->m_settings.m_eg1_osc_intensity);
                "::%lf"   // ms->m_settings.m_eg1_filter_intensity);
                "::%lf"   // ms->m_settings.m_sustain_level);
                "::%lf"   // ms->m_settings.m_noise_osc_db);
                "::%lf"   // ms->m_settings.m_eg1_dca_intensity);
                "::%lf"   // ms->m_settings.m_volume_db);
                "::%d"    // ms->m_settings.m_legato_mode);
                "::%d"    // ms->m_settings.m_pitchbend_range);
                "::%d"    // ms->m_settings.m_reset_to_zero);
                "::%d"    // ms->m_settings.m_filter_keytrack);
                "::%lf"   // ms->m_settings.m_filter_keytrack_intensity);
                "::%d"    // ms->m_settings.m_velocity_to_attack_scaling);
                "::%d"    // ms->m_settings.m_note_number_to_decay_scaling);
                "::%d"    // ms->m_settings.m_delay_mode);
                "::%lf"   // ms->m_settings.m_eg1_dca_intensity);
                "::%lf"   // ms->m_settings.m_sustain_time_ms);
                "::%lf"   // ms->m_settings.m_sustain_time_sixteenth);
                "::%d\n", // ms->m_settings.m_sustain_override);
                ms->m_settings.m_settings_name, &ms->m_settings.m_voice_mode,
                &ms->m_settings.m_lfo1_waveform,
                &ms->m_settings.m_lfo2_waveform, &ms->m_settings.m_lfo1_rate,
                &ms->m_settings.m_lfo1_amplitude,
                &ms->m_settings.m_lfo1_amp_intensity,
                &ms->m_settings.m_lfo1_filter_fc_intensity,
                &ms->m_settings.m_lfo1_osc_pitch_intensity,
                &ms->m_settings.m_lfo1_pan_intensity,
                &ms->m_settings.m_lfo2_rate, &ms->m_settings.m_lfo2_amplitude,
                &ms->m_settings.m_lfo2_amp_intensity,
                &ms->m_settings.m_lfo2_filter_fc_intensity,
                &ms->m_settings.m_lfo2_osc_pitch_intensity,
                &ms->m_settings.m_lfo2_pan_intensity,
                &ms->m_settings.m_detune_cents, &ms->m_settings.m_fc_control,
                &ms->m_settings.m_q_control, &ms->m_settings.m_attack_time_msec,
                &ms->m_settings.m_delay_time_msec,
                &ms->m_settings.m_decay_time_msec,
                &ms->m_settings.m_release_time_msec,
                &ms->m_settings.m_pulse_width_pct,
                &ms->m_settings.m_delay_feedback_pct,
                &ms->m_settings.m_delay_ratio, &ms->m_settings.m_delay_wet_mix,
                &ms->m_settings.m_octave,
                &ms->m_settings.m_portamento_time_msec,
                &ms->m_settings.m_sub_osc_db,
                &ms->m_settings.m_eg1_osc_intensity,
                &ms->m_settings.m_eg1_filter_intensity,
                &ms->m_settings.m_sustain_level, &ms->m_settings.m_noise_osc_db,
                &ms->m_settings.m_eg1_dca_intensity,
                &ms->m_settings.m_volume_db, &ms->m_settings.m_legato_mode,
                &ms->m_settings.m_pitchbend_range,
                &ms->m_settings.m_reset_to_zero,
                &ms->m_settings.m_filter_keytrack,
                &ms->m_settings.m_filter_keytrack_intensity,
                &ms->m_settings.m_velocity_to_attack_scaling,
                &ms->m_settings.m_note_number_to_decay_scaling,
                &ms->m_settings.m_delay_mode,
                &ms->m_settings.m_eg1_dca_intensity,
                &ms->m_settings.m_sustain_time_ms,
                &ms->m_settings.m_sustain_time_sixteenth,
                &ms->m_settings.m_sustain_override);

            printf("Matched %d fields\n", matches);
            minisynth_update(ms);
        }
    }

    fclose(presetzzz);
    return true;
}

void minisynth_print_settings(minisynth *ms)
{
    printf(ANSI_COLOR_WHITE); // CONTROL PANEL
    printf("///////////////////// SYNTHzzz! ///////////////////////\n");
    printf("voice: %ls - %d [0-5] "
           "(saw3,sqr3,saw2sqr,tri2saw,tri2sqr,sin2sqr)\n",
           s_mode_names[ms->m_settings.m_voice_mode],
           ms->m_settings.m_voice_mode); // unsigned
    printf(COOL_COLOR_GREEN);            // LFO1
    printf(
        "lfo1wave: %s - %d [0-7] (sine,usaw,dsaw,tri,square,expo,rsh,qrsh)\n",
        s_lfo_mode_names[ms->m_settings.m_lfo1_waveform],
        ms->m_settings.m_lfo1_waveform); // unsigned
    printf("lfo1dest: %s - %d [0-3]\n",
           s_dest_enum_to_name[ms->m_settings.m_lfo1_dest],
           ms->m_settings.m_lfo1_dest);
    printf("lfo1rate: %.2f [0.02-20]", ms->m_settings.m_lfo1_rate);
    printf(" lfo1ampint: %.2f [0-1]", ms->m_settings.m_lfo1_amp_intensity);
    printf(" lfo1amp: %.2f [0-1]\n", ms->m_settings.m_lfo1_amplitude);
    printf("lfo1filterint: %.2f [-1-1]",
           ms->m_settings.m_lfo1_filter_fc_intensity);
    printf(" lfo1panint: %.2f [0-1]", ms->m_settings.m_lfo1_pan_intensity);
    printf(" lfo1pitch %.2f [-1-1]\n",
           ms->m_settings.m_lfo1_osc_pitch_intensity);

    printf(COOL_COLOR_YELLOW); // LFO2
    printf(
        "lfo2wave: %s - %d [0-7] (sine,usaw,dsaw,tri,square,expo,rsh,qrsh)\n",
        s_lfo_mode_names[ms->m_settings.m_lfo2_waveform],
        ms->m_settings.m_lfo2_waveform); // unsigned
    printf("lfo1dest: %s - %d [0-3]\n",
           s_dest_enum_to_name[ms->m_settings.m_lfo2_dest],
           ms->m_settings.m_lfo2_dest);
    printf("lfo2rate: %.2f [0.02-20]", ms->m_settings.m_lfo2_rate);
    printf(" lfo2ampint: %.2f [0-1]", ms->m_settings.m_lfo2_amp_intensity);
    printf(" lfo2amp: %.2f [0-1]\n", ms->m_settings.m_lfo2_amplitude);
    printf("lfo2filterint: %.2f [-1-1]",
           ms->m_settings.m_lfo2_filter_fc_intensity);
    printf(" lfo2panint: %.2f [0-1]", ms->m_settings.m_lfo2_pan_intensity);
    printf(" lfo2pitch %.2f [-1-1]\n",
           ms->m_settings.m_lfo2_osc_pitch_intensity);

    printf(COOL_COLOR_GREEN);
    printf("Filter Keytrack (kt): %d [0-1]\n",
           ms->m_settings.m_filter_keytrack); // unsigned
    printf("Detune Cents (detune): %f [-100-100]\n",
           ms->m_settings.m_detune_cents);
    printf("LEGATO MODE (legato): %d [0-1]\n",
           ms->m_settings.m_legato_mode);                        // unsigned
    printf("Note Number To Decay Scaling (ndscale): %d [0-1]\n", // unsigned
           ms->m_settings.m_note_number_to_decay_scaling);
    printf("Noise OSC Db (noisedb): %f [-96-0]\n",
           ms->m_settings.m_noise_osc_db);
    printf("Octave (oct): %d [-4-4]\n", ms->m_settings.m_octave); // int
    printf("Pitchbend Range (pitchrange): %d [0-12]\n",
           ms->m_settings.m_pitchbend_range); // int
    printf("Portamento Time ms (porta): %f [0-5000]\n",
           ms->m_settings.m_portamento_time_msec);
    printf("Pulse Width Pct (pw): %f [1-99]\n",
           ms->m_settings.m_pulse_width_pct);
    printf("Sub OSC Db (subosc): %f [-96-0]\n", ms->m_settings.m_sub_osc_db);
    printf("Velocity to Attack Scaling (vascale): %d [0,1]\n",
           ms->m_settings.m_velocity_to_attack_scaling); // unsigned
    printf("Volume (vol): %f [0-1]\n", ms->m_settings.m_volume_db);
    printf("Reset To Zero (zero): %d [0,1]\n",
           ms->m_settings.m_reset_to_zero); // unsigned

    printf(COOL_COLOR_PINK); // EG
    printf("EG1 Attack time ms (attackms): %f [%d-%d]\n",
           ms->m_settings.m_attack_time_msec, EG_MINTIME_MS, EG_MAXTIME_MS);
    printf("EG1 Decay Time ms (decayms): %f [%d-%d]\n",
           ms->m_settings.m_decay_time_msec, EG_MINTIME_MS, EG_MAXTIME_MS);
    printf("EG1 Release Time ms (releasems): %f [%d-%d]\n",
           ms->m_settings.m_release_time_msec, EG_MINTIME_MS, EG_MAXTIME_MS);
    printf("EG1 Sustain Level (sustainlvl): %f [0-1]\n",
           ms->m_settings.m_sustain_level);
    printf("EG1 Sustain Time ms  (sustainms): %f [10-2000]\n",
           ms->m_settings.m_sustain_time_ms);
    printf("EG1 Sustain Time Sixteenths  (sustain16th): %f [1-16]\n",
           ms->m_settings.m_sustain_time_sixteenth);
    printf("EG1 Sustain Override (sustain): %d [0,1]\n",
           ms->m_settings.m_sustain_override); // bool
    printf("EG1 DCA Intensity (eg1dcaint): %f [-1 - 1]\n",
           ms->m_settings.m_eg1_dca_intensity);
    printf("EG1 Filter Intensity (eg1filterint): %f [-1 - 1]\n",
           ms->m_settings.m_eg1_filter_intensity);
    printf("EG1 OSc Intensity (eg1oscint): %f [-1 - 1]\n",
           ms->m_settings.m_eg1_osc_intensity);

    printf(ANSI_COLOR_MAGENTA); // FILTER
    printf("Filter Cutoff (fc): %f [80-18000]\n", ms->m_settings.m_fc_control);
    printf("Filter TYPE! (filtertype): %s [0-8]\n",
           s_filter_type_names[ms->m_settings.m_filter_type]);
    printf("Filter NLP! (nlp): %s [0-1]\n",
           ms->m_settings.m_nlp ? "on" : "off");
    printf("Filter NLP Saturation! (saturation): %f [0-100?]\n",
           ms->m_settings.m_filter_saturation);
    printf("Filter Q Control (fq): [1-10]%f\n", ms->m_settings.m_q_control);
    printf("Filter Keytrack Intensity (ktint): %f [0.5-10]\n",
           ms->m_settings.m_filter_keytrack_intensity);
    // TODO - where's type? // whats NLP?

    // printf(COOL_COLOR_MAUVE); // ARPEGGIATORRRR
    // printf("Arp Active (arp): %s [0,1]\n", ms->m_arp.active ? "true" :
    // "false");
    // printf("Arp Latch (arplatch): %s [0,1]\n",
    //       ms->m_arp.latch ? "true" : "false");
    // printf("Arp Single Note Repeat (arprepeat): %s [0,1]\n",
    //       ms->m_arp.single_note_repeat ? "true" : "false");
    // printf("Arp Octave Range (arpoctrange): %d [1,4]\n",
    //       ms->m_arp.octave_range);
    // printf("Arp Mode (arpmode): %s [0,3]\n",
    //       arp_mode_to_string[ms->m_arp.mode]);
    // printf("Arp Rate (arprate): %s [0,2]\n",
    //       arp_rate_to_string[ms->m_arp.rate]);
    // printf("Arp Cur Step (arpcurstep): %s\n",
    //       arp_cur_step_to_string[ms->m_arp.cur_step]);

    printf(ANSI_COLOR_WHITE); // DELAY
    printf("Delay Feedback Pct (delayfb): %f [0-100]\n",
           ms->m_settings.m_delay_feedback_pct);
    printf("Delay Ratio (delayr): %f [-1-1]\n", ms->m_settings.m_delay_ratio);
    printf("Delay Mode (delaymode): %d [0-%d]\n", ms->m_settings.m_delay_mode,
           MAX_NUM_DELAY_MODE - 1); // unsigned int
    printf("Delay Time ms (delayms): %f [%d-%d]\n",
           ms->m_settings.m_delay_time_msec, EG_MINTIME_MS, 2000);
    printf("Delay Wet Mix (delaymx): %f [0-1]\n",
           ms->m_settings.m_delay_wet_mix);

    printf(ANSI_COLOR_RESET);
    print_modulation_matrix(&ms->m_ms_modmatrix);
}

void minisynth_set_arpeggiate(minisynth *ms, bool b) { ms->m_arp.active = b; }

void minisynth_set_arpeggiate_latch(minisynth *ms, bool b)
{
    ms->m_arp.latch = b;
}

void minisynth_set_arpeggiate_single_note_repeat(minisynth *ms, bool b)
{
    ms->m_arp.single_note_repeat = b;
}
void minisynth_set_arpeggiate_octave_range(minisynth *ms, int val)
{
    if (val >= 1 && val <= 4)
        ms->m_arp.octave_range = val;
    else
        printf("Val must be between 1 and 4\n");
}
void minisynth_set_arpeggiate_mode(minisynth *ms, unsigned int mode)
{
    if (mode < MAX_ARP_MODE)
        ms->m_arp.mode = mode;
    else
        printf("Val must be < %d\n", MAX_ARP_MODE);
}
void minisynth_set_arpeggiate_rate(minisynth *ms, unsigned int mode)
{
    if (mode < MAX_ARP_RATE)
        ms->m_arp.rate = mode;
    else
        printf("Val must be < %d\n", MAX_ARP_RATE);
}

void minisynth_import_midi_from_file(minisynth *ms, char *filename)
{
    printf("Importing MIDI from %s\n", filename);
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Couldn't open yer file!\n");
        return;
    }

    char *item, *last_s;
    char const *sep = "::";
    minisynth_morph(ms);
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
        int count = 0;
        int tick = 0;
        int status = 0;
        int midi_note = 0;
        int midi_vel = 0;
        for (item = strtok_r(line, sep, &last_s); item;
             item = strtok_r(NULL, sep, &last_s)) {
            switch (count) {
            case 0:
                tick = atoi(item);
                if (tick >= PPNS) {
                    printf("TICK OVER!: %d\n", tick);
                    minisynth_add_melody(ms);
                    // tick = tick % PPNS;
                }
                break;
            case 1:
                status = atoi(item);
                break;
            case 2:
                midi_note = atoi(item);
                break;
            case 3:
                midi_vel = atoi(item);
                break;
            }
            count++;
            printf("ITEM! %s\n", item);
        }
        if (count == 4) {
            printf("GOtzz %d %d %d %d %d\n", count, tick, status, midi_note,
                   midi_vel);
            midi_event *ev = new_midi_event(tick, status, midi_note, midi_vel);
            minisynth_add_event(ms, ms->cur_melody, ev);
        }
    }

    minisynth_set_multi_melody_mode(ms, true);
    fclose(fp);
}

void minisynth_set_filter_mod(minisynth *ms, double mod)
{
    for (int i = 0; i < MAX_VOICES; i++) {
        minisynth_voice_set_filter_mod(ms->m_voices[i], mod);
    }
}

void minisynth_del_self(minisynth *ms)
{
    for (int i = 0; i < MAX_VOICES; i++) {
        minisynth_voice_free_self(ms->m_voices[i]);
    }
    for (int i = 0; i < MAX_NUM_MIDI_LOOPS; i++) {
        for (int j = 0; j < PPNS; j++) {
            if (ms->melodies[i][j] != NULL) {
                midi_event_free(ms->melodies[i][j]);
                ms->melodies[i][j] = NULL;
            }
        }
    }
    printf("Deleting MINISYNTH self\n");
    free(ms);
}

void minisynth_set_morph_mode(minisynth *ms, bool b)
{
    ms->morph_mode = b;
    minisynth_set_backup_mode(ms, b);
}

void minisynth_set_generate_mode(minisynth *ms, bool b)
{
    ms->generate_mode = b;
    minisynth_set_backup_mode(ms, b);
}

void minisynth_set_backup_mode(minisynth *ms, bool b)
{
    if (b) {
        minisynth_dupe_melody(ms->melodies[0],
                              ms->backup_melody_while_getting_crazy);
        ms->m_settings_backup_while_getting_crazy = ms->m_settings;
        ms->multi_melody_mode = false;
        ms->cur_melody = 0;
    }
    else {
        minisynth_dupe_melody(ms->backup_melody_while_getting_crazy,
                              ms->melodies[0]);
        ms->m_settings = ms->m_settings_backup_while_getting_crazy;
        ms->multi_melody_mode = true;
    }
}

void minisynth_morph(minisynth *ms)
{
    int midinotes_seen[10] = {0};
    int midinotes_seen_idx = 0;
    for (int i = 0; i < PPNS; i++) {

        midi_event *e = ms->melodies[0][i];

        if (e != NULL &&
            !is_int_member_in_array(e->data1, midinotes_seen, 10) &&
            midinotes_seen_idx < 10) {
            midinotes_seen[midinotes_seen_idx++] = e->data1;
        }

        int rand_num = (rand() % 3) + 1;
        int rand_num2 = (rand() % 3) + 1;
        int rand_num3 = rand() % 2;
        int rand_num4 = rand() % 2;

        int num_notes = minisynth_get_num_notes(ms);

        if (e != NULL && e->event_type == 144) {
            // printf("KEY! %d - %d %d %d\n", i, e->event_type, e->data1,
            //       e->data2);

            int randy = rand() % 4;
            int j = 0;
            int new_note_num = 0;
            switch (randy) {
            case 0:
                if (num_notes > 3) {
                    // printf("Removing note\n");
                    minisynth_rm_micro_note(ms, 0, i);
                }
                break;
            case 1:
                if (num_notes < 15) {
                    // printf("Duping note\n");
                    for (j = 1; j < rand_num; j++) {
                        int next_note =
                            (i + rand_num2 * PPSIXTEENTH * j) % PPNS;
                        minisynth_add_micro_note(ms, 0, next_note, e->data1);
                    }
                }
                break;
            case 2:
                if (rand_num3)
                    if (rand_num4)
                        new_note_num = e->data1 + 7;
                    else
                        new_note_num = e->data1 - 7;
                else if (rand_num4)
                    new_note_num = e->data1 + 5;
                else
                    new_note_num = e->data1 - 5;

                if (new_note_num > 0) {
                    minisynth_rm_micro_note(ms, 0, i);
                    minisynth_add_micro_note(ms, 0, i, new_note_num);
                }
                break;
            case 3: // no-op - leave the note as is
            default:
                break;
            }
        }
    }
    // minisynth_print_melodies(ms);
    // minisynth_update(ms);
}

void minisynth_stop(minisynth *ms)
{
    for (int i = 0; i < MAX_VOICES; i++) {
        voice_reset(&ms->m_voices[i]->m_voice);
    }
}

int minisynth_get_num_notes(minisynth *ms)
{
    int notecount = 0;
    for (int i = 0; i < ms->num_melodies; i++) {
        for (int j = 0; j < PPNS; j++) {
            midi_event *e = ms->melodies[i][j];
            if (e != NULL && e->event_type == 144)
                notecount++;
        }
    }
    return notecount;
}

int minisynth_get_notes_from_melody(midi_event **melody,
                                    int return_midi_notes[10])
{
    int idx = 0;
    for (int i = 0; i < PPNS; i++) {
        if (melody[i] != NULL) {
            midi_event *e = melody[i];
            if (e->event_type == 144) { // note on
                if (!is_int_member_in_array(e->data1, return_midi_notes, 10)) {
                    return_midi_notes[idx++] = e->data1;
                    if (idx == 10)
                        return idx;
                }
            }
        }
    }
    return idx;
}

void minisynth_sg_start(void *self)
{
    minisynth *ms = (minisynth *)self;
    ms->active = true;
    minisynth_stop(ms);
}

void minisynth_sg_stop(void *self)
{
    minisynth *ms = (minisynth *)self;
    ms->active = false;
    minisynth_stop(ms);
}

int minisynth_get_num_tracks(void *self)
{
    minisynth *ms = (minisynth *)self;
    return ms->num_melodies;
}

void minisynth_make_active_track(void *self, int pattern_num)
{
    minisynth *ms = (minisynth *)self;
    ms->cur_melody =
        pattern_num; // TODO - standardize - PATTERN? TRACK? MELODY?!?!
}

void minisynth_print_melodies(minisynth *ms)
{
    for (int i = 0; i < ms->num_melodies; i++) {
        printf("Pattern Num %d\n", i);
        midi_event **melody = ms->melodies[i];
        midi_melody_print(melody);
    }
}

void minisynth_print(minisynth *ms)
{
    minisynth_print_melodies(ms);
    minisynth_print_settings(ms);
}

void minisynth_add_note(minisynth *ms, int pattern_num, int step, int midi_note)
{
    int mstep = step * PPSIXTEENTH;
    minisynth_add_micro_note(ms, pattern_num, mstep, midi_note);
}

void minisynth_add_micro_note(minisynth *ms, int pattern_num, int mstep,
                              int midi_note)
{
    if (is_valid_melody_num(ms, pattern_num) && mstep < PPNS) {
        // printf("New Notes!! %d - %d\n", mstep, midi_note);
        midi_event *on = new_midi_event(mstep, 144, midi_note, 128);
        // int note_off_tick = (mstep + (PPSIXTEENTH * 4 - 7)) % PPNS;
        int note_off_tick =
            (mstep +
             (int)(PPSIXTEENTH * ms->m_settings.m_sustain_time_sixteenth) - 7) %
            PPNS;
        midi_event *off = new_midi_event(note_off_tick, 128, midi_note, 128);

        int final_note_off_tick = minisynth_add_event(ms, pattern_num, off);
        on->tick_off = final_note_off_tick;
        minisynth_add_event(ms, pattern_num, on);
    }
    else {
        printf("Adding MICRO note - not valid melody-num(%d) || step no "
               "good(%d)\n",
               pattern_num, mstep);
    }
}

void minisynth_rm_note(minisynth *ms, int pattern_num, int step)
{
    int mstep = step * PPSIXTEENTH;
    minisynth_rm_micro_note(ms, pattern_num, mstep);
}

void minisynth_rm_micro_note(minisynth *ms, int pat_num, int tick)
{
    if (is_valid_melody_num(ms, pat_num) && tick < PPNS) {
        if (ms->melodies[pat_num][tick] != NULL) {
            midi_event *ev = ms->melodies[ms->cur_melody][tick];
            minisynth_midi_note_off(ms, ev->data1, ev->data2, false);
            ms->melodies[ms->cur_melody][tick] = NULL;
            int tick_off = ev->tick_off;
            free(ev);
            // printf("Deleted midi event at tick %d\n", tick);
            if (tick_off)
                minisynth_rm_micro_note(ms, pat_num, tick_off);
        }
        else {
            if (mixr->debug_mode)
                printf("Not a valid midi event at tick: %d\n", tick);
        }
    }
    else {
        printf("Not a valid pattern num: %d \n", pat_num);
    }
}

void minisynth_mv_note(minisynth *ms, int pattern_num, int fromstep, int tostep)
{
    int mfromstep = fromstep * PPSIXTEENTH;
    int mtostep = tostep * PPSIXTEENTH;
    minisynth_mv_micro_note(ms, pattern_num, mfromstep, mtostep);
}

void minisynth_mv_micro_note(minisynth *ms, int pattern_num, int fromstep,
                             int tostep)
{
    if (is_valid_melody_num(ms, pattern_num)) {
        if (ms->melodies[pattern_num][fromstep] != NULL) {
            minisynth_rm_micro_note(ms, pattern_num, tostep);
            ms->melodies[pattern_num][tostep] =
                ms->melodies[pattern_num][fromstep];
            ms->melodies[pattern_num][fromstep] = NULL;
        }
        else {
            printf(
                "Woof, cannae move micro note - either fromstep(%d) is NULL or "
                "tostep(%d) is not less than %d\n",
                fromstep, tostep, PPNS);
        }
    }
}

void minisynth_set_attack_time_ms(minisynth *ms, double val)
{
    if (val >= EG_MINTIME_MS && val <= EG_MAXTIME_MS)
        ms->m_settings.m_attack_time_msec = val;
    else
        printf("val must be between %d and %d\n", EG_MINTIME_MS, EG_MAXTIME_MS);
}

void minisynth_set_decay_time_ms(minisynth *ms, double val)
{
    if (val >= EG_MINTIME_MS && val <= EG_MAXTIME_MS)
        ms->m_settings.m_decay_time_msec = val;
    else
        printf("val must be between %d and %d\n", EG_MINTIME_MS, EG_MAXTIME_MS);
}

void minisynth_set_release_time_ms(minisynth *ms, double val)
{
    if (val >= EG_MINTIME_MS && val <= EG_MAXTIME_MS)
        ms->m_settings.m_release_time_msec = val;
    else
        printf("val must be between %d and %d\n", EG_MINTIME_MS, EG_MAXTIME_MS);
}

void minisynth_set_delay_feedback_pct(minisynth *ms, double val)
{
    if (val >= 0 && val <= 100)
        ms->m_settings.m_delay_feedback_pct = val;
    else
        printf("val must be between 0 and 100\n");
}

void minisynth_set_delay_ratio(minisynth *ms, double val)
{
    if (val >= -1 && val <= 1)
        ms->m_settings.m_delay_ratio = val;
    else
        printf("val must be between -1 and 1\n");
}

void minisynth_set_delay_mode(minisynth *ms, unsigned int val)
{
    if (val < MAX_NUM_DELAY_MODE)
        ms->m_settings.m_delay_mode = val;
    else
        printf("val must be between 0 and %d\n", MAX_NUM_DELAY_MODE);
}

void minisynth_set_delay_time_ms(minisynth *ms, double val)
{
    if (val >= 0 && val <= 2000)
        ms->m_settings.m_delay_time_msec = val;
    else
        printf("val must be between 0 and 2000\n");
}

void minisynth_set_delay_wetmix(minisynth *ms, double val)
{
    if (val >= 0 && val <= 1)
        ms->m_settings.m_delay_wet_mix = val;
    else
        printf("val must be between 0 and 1\n");
}

void minisynth_set_detune(minisynth *ms, double val)
{
    if (val >= -100 && val <= 100)
        ms->m_settings.m_detune_cents = val;
    else
        printf("val must be between -100 and 100\n");
}

void minisynth_set_eg1_dca_int(minisynth *ms, double val)
{
    if (val >= -1 && val <= 1)
        ms->m_settings.m_eg1_dca_intensity = val;
    else
        printf("val must be between -1 and 1\n");
}

void minisynth_set_eg1_filter_int(minisynth *ms, double val)
{
    if (val >= -1 && val <= 1)
        ms->m_settings.m_eg1_filter_intensity = val;
    else
        printf("val must be between -1 and 1\n");
}

void minisynth_set_eg1_osc_int(minisynth *ms, double val)
{
    if (val >= -1 && val <= 1)
        ms->m_settings.m_eg1_osc_intensity = val;
    else
        printf("val must be between -1 and 1\n");
}

void minisynth_set_filter_fc(minisynth *ms, double val)
{
    if (val >= 80 && val <= 18000)
        ms->m_settings.m_fc_control = val;
    else
        printf("val must be between 80 and 18000\n");
}

void minisynth_set_filter_fq(minisynth *ms, double val)
{
    if (val >= 0.5 && val <= 10)
        ms->m_settings.m_q_control = val;
    else
        printf("val must be between 0.5 and 10\n");
}

void minisynth_set_filter_type(minisynth *ms, unsigned int val)
{
    if (val < NUM_FILTER_TYPES)
        ms->m_settings.m_filter_type = val;
    else
        printf("Val must be between 0 and %d\n", NUM_FILTER_TYPES - 1);
}

void minisynth_set_filter_saturation(minisynth *ms, double val)
{
    if (val >= 0 && val <= 100)
        ms->m_settings.m_filter_saturation = val;
    else
        printf("Val must be between 0 and 100\n");
}

void minisynth_set_filter_nlp(minisynth *ms, unsigned int val)
{
    if (val < 2)
        ms->m_settings.m_nlp = val;
    else
        printf("Val must be 0 or 1\n");
}

void minisynth_set_keytrack_int(minisynth *ms, double val)
{
    if (val >= 0.5 && val <= 10)
        ms->m_settings.m_filter_keytrack_intensity = val;
    else
        printf("val must be between 0.5 and 10\n");
}

void minisynth_set_keytrack(minisynth *ms, unsigned int val)
{
    if (val != 0 && val != 1) {
        printf("Val must be zero or one\n");
        return;
    }
    ms->m_settings.m_filter_keytrack = val;
}

void minisynth_set_legato_mode(minisynth *ms, unsigned int val)
{
    if (val != 0 && val != 1) {
        printf("Val must be zero or one\n");
        return;
    }
    ms->m_settings.m_legato_mode = val;
}

void minisynth_set_lfo_amp_int(minisynth *ms, int lfo_num, double val)
{
    if (val >= 0 && val <= 1) {
        switch (lfo_num) {
        case (1):
            ms->m_settings.m_lfo1_amp_intensity = val;
        case (2):
            ms->m_settings.m_lfo2_amp_intensity = val;
        }
    }
    else
        printf("val must be between 0 and 1\n");
}

void minisynth_set_lfo_amp(minisynth *ms, int lfo_num, double val)
{
    if (val >= 0 && val <= 1) {
        switch (lfo_num) {
        case (1):
            ms->m_settings.m_lfo1_amplitude = val;
        case (2):
            ms->m_settings.m_lfo2_amplitude = val;
        }
    }
    else
        printf("val must be between 0 and 1\n");
}

void minisynth_set_lfo_filter_fc_int(minisynth *ms, int lfo_num, double val)
{
    if (val >= -1 && val <= 1) {
        switch (lfo_num) {
        case (1):
            ms->m_settings.m_lfo1_filter_fc_intensity = val;
        case (2):
            ms->m_settings.m_lfo2_filter_fc_intensity = val;
        }
    }
    else
        printf("val must be between -1 and 1\n");
}

void minisynth_set_lfo_rate(minisynth *ms, int lfo_num, double val)
{
    if (val >= 0.02 && val <= 20) {
        switch (lfo_num) {
        case (1):
            ms->m_settings.m_lfo1_rate = val;
        case (2):
            ms->m_settings.m_lfo2_rate = val;
        }
    }
    else
        printf("val must be between 0.02 and 20\n");
}

void minisynth_set_lfo_pan_int(minisynth *ms, int lfo_num, double val)
{
    if (val >= 0 && val <= 1) {
        switch (lfo_num) {
        case (1):
            ms->m_settings.m_lfo1_pan_intensity = val;
        case (2):
            ms->m_settings.m_lfo2_pan_intensity = val;
        }
    }
    else
        printf("val must be between 0 and 1\n");
}

void minisynth_set_lfo_pitch(minisynth *ms, int lfo_num, double val)
{
    if (val >= -1 && val <= 1) {
        switch (lfo_num) {
        case (1):
            ms->m_settings.m_lfo1_osc_pitch_intensity = val;
        case (2):
            ms->m_settings.m_lfo2_osc_pitch_intensity = val;
        }
    }
    else
        printf("val must be between -1 and 1\n");
}

void minisynth_set_lfo_wave(minisynth *ms, int lfo_num, unsigned int val)
{
    if (val < MAX_LFO_OSC) {
        switch (lfo_num) {
        case (1):
            ms->m_settings.m_lfo1_waveform = val;
        case (2):
            ms->m_settings.m_lfo2_waveform = val;
        }
    }
    else
        printf("val must be between 0 and %d\n", MAX_LFO_OSC);
}

void minisynth_set_note_to_decay_scaling(minisynth *ms, unsigned int val)
{
    if (val != 0 && val != 1) {
        printf("Val must be zero or one\n");
        return;
    }
    ms->m_settings.m_note_number_to_decay_scaling = val;
}

void minisynth_set_noise_osc_db(minisynth *ms, double val)
{
    if (val >= -96 && val <= 0)
        ms->m_settings.m_noise_osc_db = val;
    else
        printf("val must be between -96 and 0\n");
}

void minisynth_set_octave(minisynth *ms, int val)
{
    if (val >= -4 && val <= 4)
        ms->m_settings.m_octave = val;
    else
        printf("val must be between -4 and 4\n");
}

void minisynth_set_pitchbend_range(minisynth *ms, int val)
{
    if (val >= 0 && val <= 12)
        ms->m_settings.m_pitchbend_range = val;
    else
        printf("val must be between 0 and 12\n");
}

void minisynth_set_portamento_time_ms(minisynth *ms, double val)
{
    if (val >= 0 && val <= 5000)
        ms->m_settings.m_portamento_time_msec = val;
    else
        printf("val must be between 0 and 5000\n");
}

void minisynth_set_pulsewidth_pct(minisynth *ms, double val)
{
    if (val >= 1 && val <= 99)
        ms->m_settings.m_pulse_width_pct = val;
    else
        printf("val must be between 1 and 99\n");
}

void minisynth_set_sub_osc_db(minisynth *ms, double val)
{
    if (val >= -96 && val <= 0)
        ms->m_settings.m_sub_osc_db = val;
    else
        printf("val must be between -96 and 0\n");
}

void minisynth_set_sustain(minisynth *ms, double val)
{
    if (val >= 0 && val <= 1)
        // ms->m_settings.m_sub_osc_db = val;
        ms->m_settings.m_sustain_level = val;
    else
        printf("val must be between 0 and 1\n");
}

void minisynth_set_sustain_override(minisynth *ms, bool b)
{
    ms->m_settings.m_sustain_override = b;

    for (int i = 0; i < MAX_VOICES; i++) {
        if (ms->m_voices[i]) {
            voice_set_sustain_override(&ms->m_voices[i]->m_voice, b);
        }
    }
}

void minisynth_set_velocity_to_attack_scaling(minisynth *ms, unsigned int val)
{
    if (val != 0 && val != 1) {
        printf("Val must be zero or one\n");
        return;
    }
    ms->m_settings.m_velocity_to_attack_scaling = val;
}

void minisynth_set_voice_mode(minisynth *ms, unsigned int val)
{
    if (val < MAX_VOICE_CHOICE)
        ms->m_settings.m_voice_mode = val;
    else
        printf("val must be between 0 and %d\n", MAX_VOICE_CHOICE);
}

void minisynth_set_vol(minisynth *ms, double val)
{
    if (val >= 0 && val <= 1)
        ms->m_settings.m_volume_db = val;
    else
        printf("val must be between 0 and 1\n");
}

void minisynth_set_reset_to_zero(minisynth *ms, unsigned int val)
{
    if (val != 0 && val != 1) {
        printf("Val must be zero or one\n");
        return;
    }
    ms->m_settings.m_reset_to_zero = val;
}

void minisynth_set_sustain_time_sixteenth(minisynth *ms, double val)
{
    if (val >= 1 && val <= 16)
        ms->m_settings.m_sustain_time_sixteenth = val;
    else
        printf("val must be between 1 and 16\n");
}

void minisynth_set_sustain_time_ms(minisynth *ms, double val)
{
    if (val >= 10 && val <= 2000)
        ms->m_settings.m_sustain_time_ms = val;
    else
        printf("val must be between 10 and 2000\n");
}

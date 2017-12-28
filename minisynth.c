#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "bitshift.h"
#include "midi_freq_table.h"
#include "minisynth.h"
#include "mixer.h"
#include "sequencer_utils.h"
#include "utils.h"

extern mixer *mixr;
extern const wchar_t *sparkchars;
extern const char *s_source_enum_to_name[];
extern const char *s_dest_enum_to_name[];

// defined in minisynth_voice.h
const wchar_t *s_voice_names[] = {L"SAW3",    L"SQR3",    L"SAW2SQR",
                                  L"TRI2SAW", L"TRI2SQR", L"SIN2SQR"};

// defined in oscillator.h
const char *s_lfo_mode_names[] = {"SYNC", "SHOT", "FREE"};
const char *s_lfo_wave_names[] = {"SINE",   "USAW", "DSAW", "TRI",
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

    synthbase_init(&ms->base, (void *)ms, MINISYNTH_TYPE);

    ms->sound_generator.gennext = &minisynth_gennext;
    ms->sound_generator.status = &minisynth_status;
    ms->sound_generator.setvol = &minisynth_setvol;
    ms->sound_generator.getvol = &minisynth_getvol;
    ms->sound_generator.start = &minisynth_sg_start;
    ms->sound_generator.stop = &minisynth_sg_stop;
    ms->sound_generator.get_num_tracks = &minisynth_get_num_tracks;
    ms->sound_generator.event_notify = &synthbase_event_notify;
    ms->sound_generator.make_active_track = &minisynth_make_active_track;
    ms->sound_generator.self_destruct = &minisynth_del_self;
    ms->sound_generator.type = MINISYNTH_TYPE;

    strncpy(ms->m_settings.m_settings_name, "DEFAULT", 7);

    ms->m_settings.m_monophonic = false;
    ms->m_settings.m_voice_mode = 0;
    ms->m_settings.m_detune_cents = 0.0;

    // LFO1
    ms->m_settings.m_lfo1_waveform = 0;
    ms->m_settings.m_lfo1_rate = DEFAULT_LFO_RATE;
    ms->m_settings.m_lfo1_amplitude = 1.0;

    // LFO1 routings
    ms->m_settings.m_lfo1_osc_pitch_intensity = 0.0;
    ms->m_settings.m_lfo1_osc_pitch_enabled = false;

    ms->m_settings.m_lfo1_filter_fc_intensity = 0.0;
    ms->m_settings.m_lfo1_filter_fc_enabled = false;

    ms->m_settings.m_lfo1_amp_intensity = 0.0;
    ms->m_settings.m_lfo1_amp_enabled = false;

    ms->m_settings.m_lfo1_pan_intensity = 0.0;
    ms->m_settings.m_lfo1_pan_enabled = false;

    ms->m_settings.m_lfo1_pulsewidth_intensity = 0.0;
    ms->m_settings.m_lfo1_pulsewidth_enabled = false;

    // LFO2
    ms->m_settings.m_lfo2_waveform = 0;
    ms->m_settings.m_lfo2_rate = DEFAULT_LFO_RATE;
    ms->m_settings.m_lfo2_amplitude = 1.0;

    // LFO2 routings
    ms->m_settings.m_lfo2_osc_pitch_intensity = 0.0;
    ms->m_settings.m_lfo2_osc_pitch_enabled = false;

    ms->m_settings.m_lfo2_filter_fc_intensity = 0.0;
    ms->m_settings.m_lfo2_filter_fc_enabled = false;

    ms->m_settings.m_lfo2_amp_intensity = 0.0;
    ms->m_settings.m_lfo2_amp_enabled = false;

    ms->m_settings.m_lfo2_pan_intensity = 0.0;
    ms->m_settings.m_lfo2_pan_enabled = false;

    ms->m_settings.m_lfo2_pulsewidth_intensity = 0.0;
    ms->m_settings.m_lfo2_pulsewidth_enabled = false;

    ms->m_settings.m_fc_control = FILTER_FC_DEFAULT;
    ms->m_settings.m_q_control = FILTER_Q_DEFAULT;

    ms->m_settings.m_eg1_osc_intensity = 0.0;
    ms->m_settings.m_eg1_osc_enabled = false;
    ms->m_settings.m_eg1_filter_intensity = 0.0;
    ms->m_settings.m_eg1_filter_enabled = false;
    ms->m_settings.m_eg1_dca_intensity = 1.0;
    ms->m_settings.m_eg1_dca_enabled = true;

    ms->m_settings.m_attack_time_msec = EG_DEFAULT_STATE_TIME;
    ms->m_settings.m_decay_time_msec = EG_DEFAULT_STATE_TIME;
    ms->m_settings.m_release_time_msec = EG_DEFAULT_STATE_TIME;

    ms->m_settings.m_pulse_width_pct = OSC_PULSEWIDTH_DEFAULT;
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

    ms->m_settings.m_sustain_override = false;
    ms->m_settings.m_sustain_time_ms = 400;
    ms->m_settings.m_sustain_time_sixteenth = 4;

    ms->m_settings.m_bitshift_active = false;
    ms->m_settings.m_bitshift_src = -99;

    for (int i = 0; i < MAX_VOICES; i++)
    {
        ms->m_last_midi_notes[i] = 0;

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

    for (int i = 0; i < MAX_VOICES; i++)
    {
        voice_set_modmatrix_core(&ms->m_voices[i]->m_voice,
                                 get_matrix_core(&ms->m_ms_modmatrix));
    }
    minisynth_update(ms);
    arpeggiator_init(&ms->m_arp);

    ms->sound_generator.active = true;
    return ms;
}

////////////////////////////////////

bool minisynth_prepare_for_play(minisynth *ms)
{
    for (int i = 0; i < MAX_VOICES; i++)
    {
        if (ms->m_voices[i])
        {
            minisynth_voice_prepare_for_play(ms->m_voices[i]);
        }
    }

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

    ms->m_global_synth_params.voice_params.lfo2_filter1_mod_intensity =
        ms->m_settings.m_lfo2_filter_fc_intensity;
    ms->m_global_synth_params.voice_params.lfo2_osc_mod_intensity =
        ms->m_settings.m_lfo2_osc_pitch_intensity;
    ms->m_global_synth_params.voice_params.lfo2_dca_amp_mod_intensity =
        ms->m_settings.m_lfo2_amp_intensity;
    ms->m_global_synth_params.voice_params.lfo2_dca_pan_mod_intensity =
        ms->m_settings.m_lfo2_pan_intensity;

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
    ms->m_global_synth_params.lfo1_params.lfo_mode = ms->m_settings.m_lfo1_mode;

    // --- lfo2:
    ms->m_global_synth_params.lfo2_params.waveform =
        ms->m_settings.m_lfo2_waveform;
    ms->m_global_synth_params.lfo2_params.amplitude =
        ms->m_settings.m_lfo2_amplitude;
    ms->m_global_synth_params.lfo2_params.osc_fo = ms->m_settings.m_lfo2_rate;
    ms->m_global_synth_params.lfo2_params.lfo_mode = ms->m_settings.m_lfo2_mode;

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
    ms->m_global_synth_params.eg1_params.sustain_override =
        (bool)ms->m_settings.m_sustain_override;

    // --- dca:
    ms->m_global_synth_params.dca_params.amplitude_db =
        ms->m_settings.m_volume_db;

    // --- enable/disable mod matrix stuff
    // LFO1 routings
    if (ms->m_settings.m_lfo1_osc_pitch_enabled == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO1, DEST_ALL_OSC_FO,
                          true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO1, DEST_ALL_OSC_FO,
                          false);

    if (ms->m_settings.m_lfo1_filter_fc_enabled == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO1, DEST_ALL_FILTER_FC,
                          true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO1, DEST_ALL_FILTER_FC,
                          false);

    if (ms->m_settings.m_lfo1_amp_enabled == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO1, DEST_DCA_AMP,
                          true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO1, DEST_DCA_AMP,
                          false);

    if (ms->m_settings.m_lfo1_pan_enabled == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO1, DEST_DCA_PAN,
                          true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO1, DEST_DCA_PAN,
                          false);

    if (ms->m_settings.m_lfo1_pulsewidth_enabled == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO1,
                          DEST_ALL_OSC_PULSEWIDTH, true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO1,
                          DEST_ALL_OSC_PULSEWIDTH, false);

    // LFO2
    if (ms->m_settings.m_lfo2_osc_pitch_enabled == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO2, DEST_ALL_OSC_FO,
                          true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO2, DEST_ALL_OSC_FO,
                          false);

    if (ms->m_settings.m_lfo2_filter_fc_enabled == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO2, DEST_ALL_FILTER_FC,
                          true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO2, DEST_ALL_FILTER_FC,
                          false);

    if (ms->m_settings.m_lfo2_amp_enabled == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO2, DEST_DCA_AMP,
                          true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO2, DEST_DCA_AMP,
                          false);

    if (ms->m_settings.m_lfo2_pan_enabled == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO2, DEST_DCA_PAN,
                          true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO2, DEST_DCA_PAN,
                          false);

    if (ms->m_settings.m_lfo2_pulsewidth_enabled == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO2,
                          DEST_ALL_OSC_PULSEWIDTH, true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_LFO2,
                          DEST_ALL_OSC_PULSEWIDTH, false);

    // EG1 routings
    if (ms->m_settings.m_eg1_osc_enabled == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_BIASED_EG1,
                          DEST_ALL_OSC_FO, true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_BIASED_EG1,
                          DEST_ALL_OSC_FO, false);

    if (ms->m_settings.m_eg1_filter_enabled == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_BIASED_EG1,
                          DEST_ALL_FILTER_FC, true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_BIASED_EG1,
                          DEST_ALL_FILTER_FC, false);

    if (ms->m_settings.m_eg1_dca_enabled == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_EG1, DEST_DCA_EG,
                          true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_EG1, DEST_DCA_EG, false);

    // Velocity to Attack
    if (ms->m_settings.m_velocity_to_attack_scaling == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_VELOCITY,
                          DEST_ALL_EG_ATTACK_SCALING, true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_VELOCITY,
                          DEST_ALL_EG_ATTACK_SCALING, false);

    // Note Number to Decay
    if (ms->m_settings.m_note_number_to_decay_scaling == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_MIDI_NOTE_NUM,
                          DEST_ALL_EG_DECAY_SCALING, true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_MIDI_NOTE_NUM,
                          DEST_ALL_EG_DECAY_SCALING, false);

    // Filter Keytrack
    if (ms->m_settings.m_filter_keytrack == 1)
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_MIDI_NOTE_NUM,
                          DEST_ALL_FILTER_KEYTRACK, true); // enable
    else
        enable_matrix_row(&ms->m_ms_modmatrix, SOURCE_MIDI_NOTE_NUM,
                          DEST_ALL_FILTER_KEYTRACK, false);

    for (int i = 0; i < MAX_VOICES; i++)
        if (ms->m_voices[i])
            minisynth_voice_update(ms->m_voices[i]);
}

void minisynth_midi_control(minisynth *ms, unsigned int data1,
                            unsigned int data2)
{
    double scaley_val = 0;
    switch (data1)
    {
    case (1):
        printf("Attack\n");
        scaley_val = scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
        ms->m_settings.m_attack_time_msec = scaley_val;
        break;
    case (2):
        printf("Decay\n");
        scaley_val = scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
        ms->m_settings.m_decay_time_msec = scaley_val;
        break;
    case (3):
        printf("Sustain\n");
        scaley_val = scaleybum(0, 127, 0, 1, data2);
        ms->m_settings.m_sustain_level = scaley_val;
        break;
    case (4):
        printf("Release\n");
        scaley_val = scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
        ms->m_settings.m_release_time_msec = scaley_val;
        break;
    case (5):
        printf("LFO rate\n");
        scaley_val = scaleybum(0, 128, MIN_LFO_RATE, MAX_LFO_RATE, data2);
        ms->m_settings.m_lfo1_rate = scaley_val;
        break;
    case (6):
        printf("LFO amp\n");
        scaley_val = scaleybum(0, 128, 0.0, 1.0, data2);
        ms->m_settings.m_lfo1_amplitude = scaley_val;
        break;
    case (7):
        printf("Filter CutOff\n");
        scaley_val = scaleybum(0, 127, FILTER_FC_MIN, FILTER_FC_MAX, data2);
        ms->m_settings.m_fc_control = scaley_val;
        break;
    case (8):
        printf("Filter Q\n");
        scaley_val = scaleybum(0, 127, 0.02, 10, data2);
        ms->m_settings.m_q_control = scaley_val;
        break;
    default:
        printf("nah\n");
    }
    minisynth_update(ms);
}

bool minisynth_midi_note_on(minisynth *ms, unsigned int midinote,
                            unsigned int velocity)
{

    if (ms->m_settings.m_monophonic)
    {
        minisynth_voice *msv = ms->m_voices[0];
        voice_note_on(&msv->m_voice, midinote, velocity,
                      get_midi_freq(midinote), ms->m_last_note_frequency);
        ms->m_last_note_frequency = get_midi_freq(midinote);
        return true;
    }

    bool steal_note = true;
    for (int i = 0; i < MAX_VOICES; i++)
    {
        minisynth_voice *msv = ms->m_voices[i];
        if (!msv)
            return false; // should never happen
        if (!msv->m_voice.m_note_on)
        {
            minisynth_increment_voice_timestamps(ms);
            voice_note_on(&msv->m_voice, midinote, velocity,
                          get_midi_freq(midinote), ms->m_last_note_frequency);

            ms->m_last_note_frequency = get_midi_freq(midinote);
            steal_note = false;
            break;
        }
    }

    if (steal_note)
    {
        if (mixr->debug_mode)
            printf("STEAL NOTE\n");
        minisynth_voice *msv = minisynth_get_oldest_voice(ms);
        if (msv)
        {
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

    if (all_notes_off)
    {
        for (int i = 0; i < MAX_VOICES; i++)
        {
            if (ms->m_voices[i])
                voice_note_off(&ms->m_voices[i]->m_voice, midinote);
        }
        return true;
    }

    for (int i = 0; i < MAX_VOICES; i++)
    {
        minisynth_voice *msv =
            minisynth_get_oldest_voice_with_note(ms, midinote);
        if (msv)
        {
            voice_note_off(&msv->m_voice, midinote);
        }
    }
    return true;
}

void minisynth_midi_pitchbend(minisynth *ms, unsigned int data1,
                              unsigned int data2)
{
    // printf("Pitch bend, babee: %d %d\n", data1, data2);
    int actual_pitch_bent_val = (int)((data1 & 0x7F) | ((data2 & 0x7F) << 7));

    if (actual_pitch_bent_val != 8192)
    {
        double normalized_pitch_bent_val =
            (float)(actual_pitch_bent_val - 0x2000) / (float)(0x2000);
        double scaley_val =
            // scaleybum(0, 16383, -100, 100, normalized_pitch_bent_val);
            scaleybum(0, 16383, -600, 600, actual_pitch_bent_val);
        // printf("Cents to bend - %f\n", scaley_val);
        for (int i = 0; i < MAX_VOICES; i++)
        {
            ms->m_voices[i]->m_voice.m_osc1->m_cents = scaley_val;
            ms->m_voices[i]->m_voice.m_osc2->m_cents = scaley_val + 2.5;
            ms->m_voices[i]->m_voice.m_osc3->m_cents = scaley_val;
            ms->m_voices[i]->m_voice.m_osc4->m_cents = scaley_val + 2.5;
            ms->m_voices[i]->m_voice.m_v_modmatrix.m_sources[SOURCE_PITCHBEND] =
                normalized_pitch_bent_val;
        }
    }
    else
    {
        for (int i = 0; i < MAX_VOICES; i++)
        {
            ms->m_voices[i]->m_voice.m_osc1->m_cents = 0;
            ms->m_voices[i]->m_voice.m_osc2->m_cents = 2.5;
            ms->m_voices[i]->m_voice.m_osc3->m_cents = 0;
            ms->m_voices[i]->m_voice.m_osc4->m_cents = 2.5;
        }
    }
}

void minisynth_reset_voices(minisynth *ms)
{
    for (int i = 0; i < MAX_VOICES; i++)
    {
        minisynth_voice_reset(ms->m_voices[i]);
    }
}

void minisynth_increment_voice_timestamps(minisynth *ms)
{
    for (int i = 0; i < MAX_VOICES; i++)
    {
        if (ms->m_voices[i])
        {
            if (ms->m_voices[i]->m_voice.m_note_on)
                ms->m_voices[i]->m_voice.m_timestamp++;
        }
    }
}

minisynth_voice *minisynth_get_oldest_voice(minisynth *ms)
{
    int timestamp = -1;
    minisynth_voice *found_voice = NULL;
    for (int i = 0; i < MAX_VOICES; i++)
    {
        if (ms->m_voices[i])
        {
            if (ms->m_voices[i]->m_voice.m_note_on &&
                (int)ms->m_voices[i]->m_voice.m_timestamp > timestamp)
            {
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
    for (int i = 0; i < MAX_VOICES; i++)
    {
        if (ms->m_voices[i])
        {
            if (voice_can_note_off(&ms->m_voices[i]->m_voice) &&
                (int)ms->m_voices[i]->m_voice.m_timestamp > timestamp &&
                ms->m_voices[i]->m_voice.m_midi_note_number == midi_note)
            {
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

    if (mixr->debug_mode)
    {
        minisynth_print(ms);
    }

    swprintf(
        status_string, MAX_PS_STRING_SZ,
        L"[" WANSI_COLOR_WHITE "MINISYNTH '%s'" WCOOL_COLOR_PINK
        "] - Vol: %.2f voice:%ls(%d)[0-%d] mono:%d "
        "bitshift:%d bitshift_src:%d last_midi_notes:%d %d %d\n"
        "      filter keytrack(kt)[0-1]: %d detune[-100-100]:%0.2f legato:%d "
        "note decay scale(ndscale)[01]:%d\n      noisedb[-96-0]:%0.2f "
        "octave[-4-4]:%d pitchrange[0-12]:%d porta ms(porta)[0-5000]:%.2f"
        "\n      pulse width(pw)[1-99]:%.2f subosc[-96-0]:%.2f "
        "velocity scale(vascale)[0-1]:%d reset to zero[0-1]:%d\n"
        "      [" WANSI_COLOR_WHITE "---------lfo1--------"
        "------------------------------------" WCOOL_COLOR_PINK "]\n"
        "      lfo1wave:%s(%d)[0-7] lfo1mode:%s(%d) lfo1rate:%.2f "
        "lfo1amp:%.2f\n"
        "      lfo1_osc_enabled:%d lfo1_osc_intensity:%.2f\n"
        "      lfo1_filter_enabled:%d lfo1_filter_intensity:%.2f\n"
        "      lfo1_amp_enabled:%d lfo1_amp_intensity:%.2f\n"
        "      lfo1_pan_enabled:%d lfo1_pan_intensity:%.2f\n"
        "      [" WANSI_COLOR_WHITE "---------lfo2--------"
        "------------------------------------" WCOOL_COLOR_PINK "]\n"
        "      lfo2wave:%s(%d)[0-7] lfo2mode:%s(%d) lfo2rate:%.2f "
        "lfo2amp:%.2f\n"
        "      lfo2_osc_enabled:%d lfo2_osc_intensity:%.2f\n"
        "      lfo2_filter_enabled:%d lfo2_filter_intensity:%.2f\n"
        "      lfo2_amp_enabled:%d lfo2_amp_intensity:%.2f\n"
        "      lfo2_pan_enabled:%d lfo2_pan_intensity:%.2f\n"
        "      [" WANSI_COLOR_WHITE "---------eg1---------"
        "------------------------------------" WCOOL_COLOR_PINK "]\n"
        "      attackms:%.2f decayms:%.2f sustainlvl:%.2f releasems:%.2f "
        "sustain? %s\n"
        "      eg1_osc_enabled:%d eg1_osc_intensity%.2f\n"
        "      eg1_filter_enabled:%d eg1_filter_intensity%.2f\n"
        "      eg1_dca_enabled:%d eg1_dca_intensity%.2f\n"
        "      [" WANSI_COLOR_WHITE "---------filter------"
        "------------------------------------" WCOOL_COLOR_PINK "]\n"
        "      filtertype:%s[0-8] fc:%.2f fq:%.2f\n"
        "      [" WANSI_COLOR_WHITE "arp" WCOOL_COLOR_PINK "] arp:%d "
        "arprepeat:%d arpmode:%d[0-3] arprate[0-3]:%d arpoctrange[1-4]:%d",

        // VOICE + GENERAL
        ms->m_settings.m_settings_name, ms->m_settings.m_volume_db,
        s_voice_names[ms->m_settings.m_voice_mode], ms->m_settings.m_voice_mode,
        MAX_VOICE_CHOICE - 1, ms->m_settings.m_monophonic,
        ms->m_settings.m_bitshift_active, ms->m_settings.m_bitshift_src,
        ms->m_last_midi_notes[0], ms->m_last_midi_notes[1],
        ms->m_last_midi_notes[2], ms->m_settings.m_filter_keytrack,
        ms->m_settings.m_detune_cents, ms->m_settings.m_legato_mode,
        ms->m_settings.m_note_number_to_decay_scaling,
        ms->m_settings.m_noise_osc_db, ms->m_settings.m_octave,
        ms->m_settings.m_pitchbend_range, ms->m_settings.m_portamento_time_msec,
        ms->m_settings.m_pulse_width_pct, ms->m_settings.m_sub_osc_db,
        ms->m_settings.m_velocity_to_attack_scaling,
        ms->m_settings.m_reset_to_zero,

        // LFO1
        s_lfo_wave_names[ms->m_settings.m_lfo1_waveform],
        ms->m_settings.m_lfo1_waveform,
        s_lfo_mode_names[ms->m_settings.m_lfo1_mode],
        ms->m_settings.m_lfo1_mode, ms->m_settings.m_lfo1_rate,
        ms->m_settings.m_lfo1_amplitude,
        ms->m_settings.m_lfo1_osc_pitch_enabled,
        ms->m_settings.m_lfo1_osc_pitch_intensity,
        ms->m_settings.m_lfo1_filter_fc_enabled,
        ms->m_settings.m_lfo1_filter_fc_intensity,
        ms->m_settings.m_lfo1_amp_enabled, ms->m_settings.m_lfo1_amp_intensity,
        ms->m_settings.m_lfo1_pan_enabled, ms->m_settings.m_lfo1_pan_intensity,

        // LFO2
        s_lfo_wave_names[ms->m_settings.m_lfo2_waveform],
        ms->m_settings.m_lfo2_waveform,
        s_lfo_mode_names[ms->m_settings.m_lfo2_mode],
        ms->m_settings.m_lfo2_mode, ms->m_settings.m_lfo2_rate,
        ms->m_settings.m_lfo2_amplitude,
        ms->m_settings.m_lfo2_osc_pitch_enabled,
        ms->m_settings.m_lfo2_osc_pitch_intensity,
        ms->m_settings.m_lfo2_filter_fc_enabled,
        ms->m_settings.m_lfo2_filter_fc_intensity,
        ms->m_settings.m_lfo2_amp_enabled, ms->m_settings.m_lfo2_amp_intensity,
        ms->m_settings.m_lfo2_pan_enabled, ms->m_settings.m_lfo2_pan_intensity,

        // EG1
        ms->m_settings.m_attack_time_msec, ms->m_settings.m_decay_time_msec,
        ms->m_settings.m_sustain_level, ms->m_settings.m_release_time_msec,
        ms->m_settings.m_sustain_override ? "true" : "false",
        ms->m_settings.m_eg1_osc_enabled, ms->m_settings.m_eg1_osc_intensity,
        ms->m_settings.m_eg1_filter_enabled,
        ms->m_settings.m_eg1_filter_intensity, ms->m_settings.m_eg1_dca_enabled,
        ms->m_settings.m_eg1_dca_intensity,

        // FILTER1
        s_filter_type_names[ms->m_settings.m_filter_type],
        ms->m_settings.m_fc_control, ms->m_settings.m_q_control,

        // ARP
        ms->m_arp.active, ms->m_arp.single_note_repeat, ms->m_arp.mode,
        ms->m_arp.rate, ms->m_arp.octave_range

        );

    wchar_t scratch[1024];
    synthbase_status(&ms->base, scratch);
    wcscat(status_string, scratch);
}

void minisynth_print_lfo1_routing_info(minisynth *ms, wchar_t *scratch)
{
    print_modulation_matrix_info_lfo1(&ms->m_ms_modmatrix, scratch);
}

void minisynth_print_eg1_routing_info(minisynth *ms, wchar_t *scratch)
{
    print_modulation_matrix_info_eg1(&ms->m_ms_modmatrix, scratch);
}

void minisynth_setvol(void *self, double v)
{
    minisynth *ms = (minisynth *)self;
    if (v < 0.0 || v > 1.0)
    {
        return;
    }
    ms->m_settings.m_volume_db = v;
}

double minisynth_getvol(void *self)
{
    minisynth *ms = (minisynth *)self;
    return ms->m_settings.m_volume_db;
}

int minisynth_get_num_tracks(void *self)
{
    minisynth *ms = (minisynth *)self;
    return synthbase_get_num_tracks(&ms->base);
}

void minisynth_make_active_track(void *self, int tracknum)
{
    minisynth *ms = (minisynth *)self;
    synthbase_make_active_track(&ms->base, tracknum);
}

stereo_val minisynth_gennext(void *self)
{

    minisynth *ms = (minisynth *)self;

    if (!ms->sound_generator.active)
        return (stereo_val){0, 0};

    double accum_out_left = 0.0;
    double accum_out_right = 0.0;

    if (ms->m_settings.m_bitshift_active &&
        mixer_is_valid_seq_gen_num(mixr, ms->m_settings.m_bitshift_src))
    {
        sequence_generator *sg =
            mixr->sequence_generators[ms->m_settings.m_bitshift_src];

        short nom_left = bitshift_generate((void *)sg, NULL);
        short nom_right = bitshift_generate((void *)sg, NULL);

        nom_left = (nom_left - 0x80) << 8;
        nom_right = (nom_right - 0x80) << 8;

        accum_out_left += scaleybum(-32768, 32787, -1, 1, nom_left);
        accum_out_right += scaleybum(-32768, 32787, -1, 1, nom_right);
    }
    else
    {
        float mix = 1.0 / MAX_VOICES;
        if (ms->m_arp.active)
            arpeggiate(ms, &ms->m_arp);

        double out_left = 0.0;
        double out_right = 0.0;

        for (int i = 0; i < MAX_VOICES; i++)
        {
            if (ms->m_voices[i])
                minisynth_voice_gennext(ms->m_voices[i], &out_left, &out_right);

            accum_out_left += mix * out_left;
            accum_out_right += mix * out_right;
        }
    }

    accum_out_left = effector(&ms->sound_generator, accum_out_left);
    accum_out_left = envelopor(&ms->sound_generator, accum_out_left);
    accum_out_left *= ms->m_settings.m_volume_db;

    accum_out_right = effector(&ms->sound_generator, accum_out_right);
    accum_out_right = envelopor(&ms->sound_generator, accum_out_right);
    accum_out_right *= ms->m_settings.m_volume_db;

    return (stereo_val){.left = accum_out_left, .right = accum_out_right};
}

void minisynth_rand_settings(minisynth *ms)
{
    printf("Randomizing SYNTH!\n");

    strncpy(ms->m_settings.m_settings_name, "-- random UNSAVED--", 256);
    ms->m_settings.m_voice_mode = rand() % MAX_VOICE_CHOICE;
    ms->m_settings.m_monophonic = rand() % 2;

    ms->m_settings.m_lfo1_waveform = rand() % MAX_LFO_OSC;
    ms->m_settings.m_lfo1_rate =
        ((float)rand()) / RAND_MAX * (MAX_LFO_RATE - MIN_LFO_RATE) +
        MIN_LFO_RATE;
    ms->m_settings.m_lfo1_amplitude = ((float)rand()) / RAND_MAX;
    ms->m_settings.m_lfo1_osc_pitch_intensity =
        (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    ms->m_settings.m_lfo1_osc_pitch_enabled = rand() % 2;
    ms->m_settings.m_lfo1_filter_fc_intensity =
        (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    ms->m_settings.m_lfo1_filter_fc_enabled = rand() % 2;
    ms->m_settings.m_filter_type = rand() % NUM_FILTER_TYPES;
    // ms->m_settings.m_lfo1_amp_intensity = ((float)rand() /
    //(float)(RAND_MAX));
    // ms->m_settings.m_lfo1_amp_enabled = rand() % 2;
    // ms->m_settings.m_lfo1_pan_intensity = ((float)rand() /
    //(float)(RAND_MAX));
    // ms->m_settings.m_lfo1_pan_enabled = rand() % 2;
    ms->m_settings.m_lfo1_pulsewidth_intensity =
        ((float)rand() / (float)(RAND_MAX));
    ms->m_settings.m_lfo1_pulsewidth_enabled = rand() % 2;

    ms->m_settings.m_lfo2_waveform = rand() % MAX_LFO_OSC;
    ms->m_settings.m_lfo2_rate =
        ((float)rand()) / RAND_MAX * (MAX_LFO_RATE - MIN_LFO_RATE) +
        MIN_LFO_RATE;
    ms->m_settings.m_lfo2_amplitude = ((float)rand()) / RAND_MAX;
    ms->m_settings.m_lfo2_osc_pitch_intensity =
        (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    ms->m_settings.m_lfo2_osc_pitch_enabled = rand() % 2;
    ms->m_settings.m_lfo2_filter_fc_intensity =
        (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    ms->m_settings.m_lfo2_filter_fc_enabled = rand() % 2;
    // ms->m_settings.m_lfo2_amp_intensity = ((float)rand() /
    //(float)(RAND_MAX));
    // ms->m_settings.m_lfo2_amp_enabled = rand() % 2;
    // ms->m_settings.m_lfo2_pan_intensity = ((float)rand() /
    //(float)(RAND_MAX));
    // ms->m_settings.m_lfo2_pan_enabled = rand() % 2;
    ms->m_settings.m_lfo2_pulsewidth_intensity =
        ((float)rand() / (float)(RAND_MAX));
    ms->m_settings.m_lfo2_pulsewidth_enabled = rand() % 2;

    ms->m_settings.m_detune_cents = (rand() % 200) - 100;

    ms->m_settings.m_fc_control =
        ((float)rand()) / RAND_MAX * (FILTER_FC_MAX - FILTER_FC_MIN) +
        FILTER_FC_MIN;
    ms->m_settings.m_q_control = (rand() % 9) + 1;

    ms->m_settings.m_attack_time_msec = (rand() % 700) + 5;
    ms->m_settings.m_decay_time_msec = (rand() % 700) + 5;
    ms->m_settings.m_release_time_msec = (rand() % 600) + 5;
    ms->m_settings.m_pulse_width_pct = (rand() % 99) + 1;

    // ms->m_settings.m_sustain_level = ((float)rand()) / RAND_MAX;
    ms->m_settings.m_octave = rand() % 3 + 1;

    ms->m_settings.m_portamento_time_msec = rand() % 5000;

    ms->m_settings.m_sub_osc_db = -1.0 * (rand() % 96);
    // ms->m_settings.m_eg1_osc_intensity =
    //    (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    ms->m_settings.m_eg1_filter_intensity =
        (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    ms->m_settings.m_noise_osc_db = -1.0 * (rand() % 96);

    //////// ms->m_settings.m_volume_db = 1.0;
    ms->m_settings.m_legato_mode = rand() % 2;
    // ms->m_settings.m_pitchbend_range = rand() % 12;
    ms->m_settings.m_reset_to_zero = rand() % 2;
    ms->m_settings.m_filter_keytrack = rand() % 2;
    ms->m_settings.m_filter_keytrack_intensity =
        (((float)rand() / (float)(RAND_MAX)) * 9) + 0.51;
    ms->m_settings.m_velocity_to_attack_scaling = rand() % 2;
    ms->m_settings.m_note_number_to_decay_scaling = rand() % 2;
    ////ms->m_settings.m_eg1_dca_intensity =
    ////    (((float)rand() / (float)(RAND_MAX)) * 2.0) - 1;
    // ms->m_settings.m_sustain_override = rand() % 2;
    ms->m_settings.m_sustain_time_ms = rand() % 1000;
    ms->m_settings.m_sustain_time_sixteenth = rand() % 5;

    minisynth_update(ms);

    minisynth_print_settings(ms);
}

bool minisynth_save_settings(minisynth *ms, char *preset_name)
{
    if (strlen(preset_name) == 0)
    {
        printf("Play tha game, pal, need a name to save yer synth settings "
               "with\n");
        return false;
    }
    printf("Saving '%s' settings for Minisynth to file %s\n", preset_name,
           PRESET_FILENAME);
    FILE *presetzzz = fopen(PRESET_FILENAME, "a+");
    if (presetzzz == NULL)
    {
        printf("Couldn't save settings!!\n");
        return false;
    }

    int settings_count = 0;
    strncpy(ms->m_settings.m_settings_name, preset_name, 256);

    fprintf(presetzzz, "::name=%s", ms->m_settings.m_settings_name);
    settings_count++;

    fprintf(presetzzz, "::voice_mode=%d", ms->m_settings.m_voice_mode);
    settings_count++;

    fprintf(presetzzz, "::monophonic=%d", ms->m_settings.m_monophonic);
    settings_count++;

    // LFO1
    fprintf(presetzzz, "::lfo1_waveform=%d", ms->m_settings.m_lfo1_waveform);
    settings_count++;
    fprintf(presetzzz, "::lfo1_dest=%d", ms->m_settings.m_lfo1_dest);
    settings_count++;
    fprintf(presetzzz, "::lfo1_mode=%d", ms->m_settings.m_lfo1_mode);
    settings_count++;
    fprintf(presetzzz, "::lfo1_rate=%f", ms->m_settings.m_lfo1_rate);
    settings_count++;
    fprintf(presetzzz, "::lfo1_amp=%f", ms->m_settings.m_lfo1_amplitude);
    settings_count++;
    fprintf(presetzzz, "::lfo1_osc_pitch_intensity=%f",
            ms->m_settings.m_lfo1_osc_pitch_intensity);
    settings_count++;
    fprintf(presetzzz, "::lfo1_osc_pitch_enabled=%d",
            ms->m_settings.m_lfo1_osc_pitch_enabled);
    settings_count++;
    fprintf(presetzzz, "::lfo1_filter_fc_intensity=%f",
            ms->m_settings.m_lfo1_filter_fc_intensity);
    settings_count++;
    fprintf(presetzzz, "::lfo1_filter_fc_enabled=%d",
            ms->m_settings.m_lfo1_filter_fc_enabled);
    settings_count++;
    fprintf(presetzzz, "::lfo1_amp_intensity=%f",
            ms->m_settings.m_lfo1_amp_intensity);
    settings_count++;
    fprintf(presetzzz, "::lfo1_amp_enabled=%d",
            ms->m_settings.m_lfo1_amp_enabled);
    settings_count++;
    fprintf(presetzzz, "::lfo1_pan_intensity=%f",
            ms->m_settings.m_lfo1_pan_intensity);
    settings_count++;
    fprintf(presetzzz, "::lfo1_pan_enabled=%d",
            ms->m_settings.m_lfo1_pan_enabled);
    settings_count++;
    fprintf(presetzzz, "::lfo1_pulsewidth_intensity=%f",
            ms->m_settings.m_lfo1_pulsewidth_intensity);
    settings_count++;
    fprintf(presetzzz, "::lfo1_pulsewidth_enabled=%d",
            ms->m_settings.m_lfo1_pulsewidth_enabled);
    settings_count++;

    // LFO2
    fprintf(presetzzz, "::lfo2_waveform=%d", ms->m_settings.m_lfo2_waveform);
    settings_count++;
    fprintf(presetzzz, "::lfo2_dest=%d", ms->m_settings.m_lfo2_dest);
    settings_count++;
    fprintf(presetzzz, "::lfo2_mode=%d", ms->m_settings.m_lfo2_mode);
    settings_count++;
    fprintf(presetzzz, "::lfo2_rate=%f", ms->m_settings.m_lfo2_rate);
    settings_count++;
    fprintf(presetzzz, "::lfo2_amp=%f", ms->m_settings.m_lfo2_amplitude);
    settings_count++;
    fprintf(presetzzz, "::lfo2_osc_pitch_intensity=%f",
            ms->m_settings.m_lfo2_osc_pitch_intensity);
    settings_count++;
    fprintf(presetzzz, "::lfo2_osc_pitch_enabled=%d",
            ms->m_settings.m_lfo2_osc_pitch_enabled);
    settings_count++;
    fprintf(presetzzz, "::lfo2_filter_fc_intensity=%f",
            ms->m_settings.m_lfo2_filter_fc_intensity);
    settings_count++;
    fprintf(presetzzz, "::lfo2_filter_fc_enabled=%d",
            ms->m_settings.m_lfo2_filter_fc_enabled);
    settings_count++;
    fprintf(presetzzz, "::lfo2_amp_intensity=%f",
            ms->m_settings.m_lfo2_amp_intensity);
    settings_count++;
    fprintf(presetzzz, "::lfo2_amp_enabled=%d",
            ms->m_settings.m_lfo2_amp_enabled);
    settings_count++;
    fprintf(presetzzz, "::lfo2_pan_intensity=%f",
            ms->m_settings.m_lfo2_pan_intensity);
    settings_count++;
    fprintf(presetzzz, "::lfo2_pan_enabled=%d",
            ms->m_settings.m_lfo2_pan_enabled);
    settings_count++;
    fprintf(presetzzz, "::lfo2_pulsewidth_intensity=%f",
            ms->m_settings.m_lfo2_pulsewidth_intensity);
    settings_count++;
    fprintf(presetzzz, "::lfo2_pulsewidth_enabled=%d",
            ms->m_settings.m_lfo2_pulsewidth_enabled);
    settings_count++;
    // EG1
    fprintf(presetzzz, "::attack_time_msec=%f",
            ms->m_settings.m_attack_time_msec);
    settings_count++;
    fprintf(presetzzz, "::decay_time_msec=%f",
            ms->m_settings.m_decay_time_msec);
    settings_count++;
    fprintf(presetzzz, "::release_time_msec=%f",
            ms->m_settings.m_release_time_msec);
    settings_count++;
    fprintf(presetzzz, "::sustain_level=%f", ms->m_settings.m_sustain_level);
    settings_count++;

    fprintf(presetzzz, "::volume_db=%f", ms->m_settings.m_volume_db);
    settings_count++;
    fprintf(presetzzz, "::fc_control=%f", ms->m_settings.m_fc_control);
    settings_count++;
    fprintf(presetzzz, "::q_control=%f", ms->m_settings.m_q_control);
    settings_count++;

    fprintf(presetzzz, "::detune_cents=%f", ms->m_settings.m_detune_cents);
    settings_count++;
    fprintf(presetzzz, "::pulse_width_pct=%f",
            ms->m_settings.m_pulse_width_pct);
    settings_count++;
    fprintf(presetzzz, "::sub_osc_db=%f", ms->m_settings.m_sub_osc_db);
    settings_count++;
    fprintf(presetzzz, "::noise_osc_db=%f", ms->m_settings.m_noise_osc_db);
    settings_count++;

    fprintf(presetzzz, "::eg1_osc_intensity=%f",
            ms->m_settings.m_eg1_osc_intensity);
    settings_count++;
    fprintf(presetzzz, "::eg1_osc_enabled=%d",
            ms->m_settings.m_eg1_osc_enabled);
    settings_count++;

    fprintf(presetzzz, "::eg1_filter_intensity=%f",
            ms->m_settings.m_eg1_filter_intensity);
    settings_count++;
    fprintf(presetzzz, "::eg1_filter_enabled=%d",
            ms->m_settings.m_eg1_filter_enabled);
    settings_count++;

    fprintf(presetzzz, "::eg1_dca_intensity=%f",
            ms->m_settings.m_eg1_dca_intensity);
    settings_count++;
    fprintf(presetzzz, "::eg1_dca_enabled=%d",
            ms->m_settings.m_eg1_dca_enabled);
    settings_count++;

    fprintf(presetzzz, "::filter_keytrack_intensity=%f",
            ms->m_settings.m_filter_keytrack_intensity);
    settings_count++;

    fprintf(presetzzz, "::octave=%d", ms->m_settings.m_octave);
    settings_count++;
    fprintf(presetzzz, "::pitchbend_range=%d",
            ms->m_settings.m_pitchbend_range);
    settings_count++;

    fprintf(presetzzz, "::legato_mode=%d", ms->m_settings.m_legato_mode);
    settings_count++;
    fprintf(presetzzz, "::reset_to_zero=%d", ms->m_settings.m_reset_to_zero);
    settings_count++;
    fprintf(presetzzz, "::filter_keytrack=%d",
            ms->m_settings.m_filter_keytrack);
    settings_count++;
    fprintf(presetzzz, "::filter_type=%d", ms->m_settings.m_filter_type);
    settings_count++;
    fprintf(presetzzz, "::filter_saturation=%f",
            ms->m_settings.m_filter_saturation);
    settings_count++;

    fprintf(presetzzz, "::nlp=%d", ms->m_settings.m_nlp);
    settings_count++;
    fprintf(presetzzz, "::velocity_to_attack_scaling=%d",
            ms->m_settings.m_velocity_to_attack_scaling);
    settings_count++;
    fprintf(presetzzz, "::note_number_to_decay_scaling=%d",
            ms->m_settings.m_note_number_to_decay_scaling);
    settings_count++;
    fprintf(presetzzz, "::portamento_time_msec=%f",
            ms->m_settings.m_portamento_time_msec);
    settings_count++;

    fprintf(presetzzz, "::sustain_override=%d",
            ms->m_settings.m_sustain_override);
    settings_count++;
    fprintf(presetzzz, "::sustain_time_ms=%f",
            ms->m_settings.m_sustain_time_ms);
    settings_count++;
    fprintf(presetzzz, "::sustain_time_sixteenth=%f",
            ms->m_settings.m_sustain_time_sixteenth);
    settings_count++;
    fprintf(presetzzz, ":::\n");

    fclose(presetzzz);
    printf("Wrote %d settings\n", settings_count++);
    return true;
}

bool minisynth_list_presets()
{
    FILE *presetzzz = fopen(PRESET_FILENAME, "r+");
    if (presetzzz == NULL)
        return false;

    char line[256];
    while (fgets(line, sizeof(line), presetzzz))
    {
        printf("%s\n", line);
    }

    fclose(presetzzz);

    return true;
}

bool minisynth_check_if_preset_exists(char *preset_to_find)
{
    FILE *presetzzz = fopen(PRESET_FILENAME, "r+");
    if (presetzzz == NULL)
        return false;

    char line[2048];
    char const *sep = "::";
    char *preset_name, *last_s;

    while (fgets(line, sizeof(line), presetzzz))
    {
        preset_name = strtok_r(line, sep, &last_s);
        if (strncmp(preset_to_find, preset_name, 255) == 0)
            return true;
    }

    fclose(presetzzz);
    return false;
}
bool minisynth_load_settings(minisynth *ms, char *preset_to_load)
{
    if (strlen(preset_to_load) == 0)
    {
        printf("Play tha game, pal, need a name to LOAD yer synth settings "
               "with\n");
        return false;
    }

    char line[2048];
    char setting_key[512];
    char setting_val[512];
    double scratch_val = 0.;

    FILE *presetzzz = fopen(PRESET_FILENAME, "r+");
    if (presetzzz == NULL)
        return false;

    char *tok, *last_tok;
    char const *sep = "::";

    while (fgets(line, sizeof(line), presetzzz))
    {
        int settings_count = 0;

        for (tok = strtok_r(line, sep, &last_tok); tok;
             tok = strtok_r(NULL, sep, &last_tok))
        {
            sscanf(tok, "%[^=]=%s", setting_key, setting_val);
            sscanf(setting_val, "%lf", &scratch_val);
            printf("key:%s val:%f\n", setting_key, scratch_val);
            if (strcmp(setting_key, "name") == 0)
            {
                if (strcmp(setting_val, preset_to_load) != 0)
                    break;
                else
                    printf("Found yer preset:%s!\n", setting_val);
                strcpy(ms->m_settings.m_settings_name, setting_val);
                settings_count++;
            }
            else if (strcmp(setting_key, "voice_mode") == 0)
            {
                ms->m_settings.m_voice_mode = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "monophonic") == 0)
            {
                ms->m_settings.m_monophonic = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo1_waveform") == 0)
            {
                ms->m_settings.m_lfo1_waveform = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo1_dest") == 0)
            {
                ms->m_settings.m_lfo1_dest = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo1_mode") == 0)
            {
                ms->m_settings.m_lfo1_mode = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo1_rate") == 0)
            {
                ms->m_settings.m_lfo1_rate = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo1_amp") == 0)
            {
                ms->m_settings.m_lfo1_amplitude = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo1_osc_pitch_intensity") == 0)
            {
                ms->m_settings.m_lfo1_osc_pitch_intensity = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo1_osc_pitch_enabled") == 0)
            {
                ms->m_settings.m_lfo1_osc_pitch_enabled = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo1_filter_fc_intensity") == 0)
            {
                ms->m_settings.m_lfo1_filter_fc_intensity = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo1_filter_fc_enabled") == 0)
            {
                ms->m_settings.m_lfo1_filter_fc_enabled = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo1_amp_intensity") == 0)
            {
                ms->m_settings.m_lfo1_amp_intensity = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo1_amp_enabled") == 0)
            {
                ms->m_settings.m_lfo1_amp_enabled = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo1_pan_intensity") == 0)
            {
                ms->m_settings.m_lfo1_pan_intensity = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo1_pan_enabled") == 0)
            {
                ms->m_settings.m_lfo1_pan_enabled = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo1_pulsewidth_intensity") == 0)
            {
                ms->m_settings.m_lfo1_pulsewidth_intensity = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo1_pulsewidth_enabled") == 0)
            {
                ms->m_settings.m_lfo1_pulsewidth_enabled = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo2_waveform") == 0)
            {
                ms->m_settings.m_lfo2_waveform = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo2_dest") == 0)
            {
                ms->m_settings.m_lfo2_dest = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo2_mode") == 0)
            {
                ms->m_settings.m_lfo2_mode = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo2_rate") == 0)
            {
                ms->m_settings.m_lfo2_rate = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo2_amp") == 0)
            {
                ms->m_settings.m_lfo2_amplitude = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo2_osc_pitch_intensity") == 0)
            {
                ms->m_settings.m_lfo2_osc_pitch_intensity = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo2_osc_pitch_enabled") == 0)
            {
                ms->m_settings.m_lfo2_osc_pitch_enabled = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo2_filter_fc_intensity") == 0)
            {
                ms->m_settings.m_lfo2_filter_fc_intensity = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo2_filter_fc_enabled") == 0)
            {
                ms->m_settings.m_lfo2_filter_fc_enabled = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo2_amp_intensity") == 0)
            {
                ms->m_settings.m_lfo2_amp_intensity = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo2_amp_enabled") == 0)
            {
                ms->m_settings.m_lfo2_amp_enabled = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo2_pan_intensity") == 0)
            {
                ms->m_settings.m_lfo2_pan_intensity = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo2_pan_enabled") == 0)
            {
                ms->m_settings.m_lfo2_pan_enabled = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo2_pulsewidth_intensity") == 0)
            {
                ms->m_settings.m_lfo2_pulsewidth_intensity = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "lfo2_pulsewidth_enabled") == 0)
            {
                ms->m_settings.m_lfo2_pulsewidth_enabled = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "attack_time_msec") == 0)
            {
                ms->m_settings.m_attack_time_msec = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "decay_time_msec") == 0)
            {
                ms->m_settings.m_decay_time_msec = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "release_time_msec") == 0)
            {
                ms->m_settings.m_release_time_msec = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "sustain_level") == 0)
            {
                ms->m_settings.m_sustain_level = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "volume_db") == 0)
            {
                ms->m_settings.m_volume_db = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "fc_control") == 0)
            {
                ms->m_settings.m_fc_control = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "q_control") == 0)
            {
                ms->m_settings.m_q_control = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "detune_cents") == 0)
            {
                ms->m_settings.m_detune_cents = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "pulse_width_pct") == 0)
            {
                ms->m_settings.m_pulse_width_pct = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "sub_osc_db") == 0)
            {
                ms->m_settings.m_sub_osc_db = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "noise_osc_db") == 0)
            {
                ms->m_settings.m_noise_osc_db = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "eg1_osc_intensity") == 0)
            {
                ms->m_settings.m_eg1_osc_intensity = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "eg1_osc_enabled") == 0)
            {
                ms->m_settings.m_eg1_osc_enabled = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "eg1_filter_intensity") == 0)
            {
                ms->m_settings.m_eg1_filter_intensity = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "eg1_filter_enabled") == 0)
            {
                ms->m_settings.m_eg1_filter_enabled = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "eg1_dca_intensity") == 0)
            {
                ms->m_settings.m_eg1_dca_intensity = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "eg1_dca_enabled") == 0)
            {
                ms->m_settings.m_eg1_dca_enabled = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "filter_keytrack_intensity") == 0)
            {
                ms->m_settings.m_filter_keytrack_intensity = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "octave") == 0)
            {
                ms->m_settings.m_octave = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "pitchbend_range") == 0)
            {
                ms->m_settings.m_pitchbend_range = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "legato_mode") == 0)
            {
                ms->m_settings.m_legato_mode = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "reset_to_zero") == 0)
            {
                ms->m_settings.m_reset_to_zero = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "filter_keytrack") == 0)
            {
                ms->m_settings.m_filter_keytrack = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "filter_type") == 0)
            {
                ms->m_settings.m_filter_type = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "filter_saturation") == 0)
            {
                ms->m_settings.m_filter_saturation = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "nlp") == 0)
            {
                ms->m_settings.m_nlp = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "velocity_to_attack_scaling") == 0)
            {
                ms->m_settings.m_velocity_to_attack_scaling = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "note_number_to_decay_scaling") == 0)
            {
                ms->m_settings.m_note_number_to_decay_scaling = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "portamento_time_msec") == 0)
            {
                ms->m_settings.m_portamento_time_msec = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "sustain_override") == 0)
            {
                ms->m_settings.m_sustain_override = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "sustain_time_ms") == 0)
            {
                ms->m_settings.m_sustain_time_ms = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "sustain_time_sixteenth") == 0)
            {
                ms->m_settings.m_sustain_time_sixteenth = scratch_val;
                settings_count++;
            }
        }
        if (settings_count > 0)
            printf("Loaded %d settings\n", settings_count);
        minisynth_update(ms);
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
           s_voice_names[ms->m_settings.m_voice_mode],
           ms->m_settings.m_voice_mode); // unsigned
    printf(COOL_COLOR_GREEN);            // LFO1
    printf(
        "lfo1wave: %s - %d [0-7] (sine,usaw,dsaw,tri,square,expo,rsh,qrsh)\n",
        s_lfo_wave_names[ms->m_settings.m_lfo1_waveform],
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
        s_lfo_wave_names[ms->m_settings.m_lfo2_waveform],
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

    printf(COOL_COLOR_MAUVE); // ARPEGGIATORRRR
    printf("Arp Active (arp): %s [0,1]\n", ms->m_arp.active ? "true" : "false");
    printf("Arp Latch (arplatch): %s [0,1]\n",
           ms->m_arp.latch ? "true" : "false");
    printf("Arp Single Note Repeat (arprepeat): %s [0,1]\n",
           ms->m_arp.single_note_repeat ? "true" : "false");
    printf("Arp Octave Range (arpoctrange): %d [1,4]\n",
           ms->m_arp.octave_range);
    printf("Arp Mode (arpmode): %s [0,3]\n",
           arp_mode_to_string[ms->m_arp.mode]);
    printf("Arp Rate (arprate): %s [0,2]\n",
           arp_rate_to_string[ms->m_arp.rate]);
    printf("Arp Cur Step (arpcurstep): %s\n",
           arp_cur_step_to_string[ms->m_arp.cur_step]);

    printf(ANSI_COLOR_RESET);
}

void minisynth_print_melodies(minisynth *ms)
{
    synthbase_print_melodies(&ms->base);
}

void minisynth_print_modulation_routings(minisynth *ms)
{
    print_modulation_matrix(&ms->m_ms_modmatrix);
}

void minisynth_del_self(void *self)
{
    minisynth *ms = (minisynth *)self;
    for (int i = 0; i < MAX_VOICES; i++)
    {
        minisynth_voice_free_self(ms->m_voices[i]);
    }
    printf("Deleting MINISYNTH self\n");
    free(ms);
}

void minisynth_stop(minisynth *ms)
{
    for (int i = 0; i < MAX_VOICES; i++)
    {
        voice_reset(&ms->m_voices[i]->m_voice);
    }
}

void minisynth_sg_start(void *self)
{
    minisynth *ms = (minisynth *)self;
    ms->sound_generator.active = true;
}

void minisynth_sg_stop(void *self)
{
    minisynth *ms = (minisynth *)self;
    ms->sound_generator.active = false;
    minisynth_stop(ms);
}

void minisynth_set_arpeggiate(minisynth *ms, bool b) { ms->m_arp.active = b; }

void minisynth_set_bitshift_src(minisynth *ms, int src)
{
    if (mixer_is_valid_seq_gen_num(mixr, src))
        ms->m_settings.m_bitshift_src = src;
}

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

void minisynth_set_filter_mod(minisynth *ms, double mod)
{
    for (int i = 0; i < MAX_VOICES; i++)
    {
        minisynth_voice_set_filter_mod(ms->m_voices[i], mod);
    }
}

void minisynth_print(minisynth *ms) { minisynth_print_settings(ms); }

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

void minisynth_set_detune(minisynth *ms, double val)
{
    if (val >= -100 && val <= 100)
        ms->m_settings.m_detune_cents = val;
    else
        printf("val must be between -100 and 100\n");
}

void minisynth_set_eg1_dca_enable(minisynth *ms, int val)
{
    if (val == 0 || val == 1)
        ms->m_settings.m_eg1_dca_enabled = val;
    else
        printf("val must be boolean 0 or 1\n");
}

void minisynth_set_eg1_dca_int(minisynth *ms, double val)
{
    if (val >= -1 && val <= 1)
        ms->m_settings.m_eg1_dca_intensity = val;
    else
        printf("val must be between -1 and 1\n");
}

void minisynth_set_eg1_filter_enable(minisynth *ms, int val)
{
    if (val == 0 || val == 1)
        ms->m_settings.m_eg1_filter_enabled = val;
    else
        printf("val must be boolean 0 or 1\n");
}

void minisynth_set_eg1_filter_int(minisynth *ms, double val)
{
    if (val >= -1 && val <= 1)
        ms->m_settings.m_eg1_filter_intensity = val;
    else
        printf("val must be between -1 and 1\n");
}

void minisynth_set_eg1_osc_enable(minisynth *ms, int val)
{
    if (val == 0 || val == 1)
        ms->m_settings.m_eg1_osc_enabled = val;
    else
        printf("val must be boolean 0 or 1\n");
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
    if (val != 0 && val != 1)
    {
        printf("Val must be zero or one\n");
        return;
    }
    ms->m_settings.m_filter_keytrack = val;
}

void minisynth_set_legato_mode(minisynth *ms, unsigned int val)
{
    if (val != 0 && val != 1)
    {
        printf("Val must be zero or one\n");
        return;
    }
    ms->m_settings.m_legato_mode = val;
}

void minisynth_set_lfo_osc_enable(minisynth *ms, int lfo_num, int val)
{
    if (val == 0 || val == 1)
    {
        switch (lfo_num)
        {
        case (1):
            ms->m_settings.m_lfo1_osc_pitch_enabled = val;
            break;
        case (2):
            ms->m_settings.m_lfo2_osc_pitch_enabled = val;
            break;
        }
    }
    else
        printf("Must be a boolean 0 or 1\n");
}

void minisynth_set_lfo_amp_enable(minisynth *ms, int lfo_num, int val)
{
    if (val == 0 || val == 1)
    {
        switch (lfo_num)
        {
        case (1):
            ms->m_settings.m_lfo1_amp_enabled = val;
            break;
        case (2):
            ms->m_settings.m_lfo2_amp_enabled = val;
            break;
        }
    }
    else
        printf("Must be a boolean 0 or 1\n");
}

void minisynth_set_lfo_filter_enable(minisynth *ms, int lfo_num, int val)
{
    if (val == 0 || val == 1)
    {
        switch (lfo_num)
        {
        case (1):
            ms->m_settings.m_lfo1_filter_fc_enabled = val;
            break;
        case (2):
            ms->m_settings.m_lfo2_filter_fc_enabled = val;
            break;
        }
    }
    else
        printf("Must be a boolean 0 or 1\n");
}

void minisynth_set_lfo_pan_enable(minisynth *ms, int lfo_num, int val)
{
    if (val == 0 || val == 1)
    {
        switch (lfo_num)
        {
        case (1):
            ms->m_settings.m_lfo1_pan_enabled = val;
            break;
        case (2):
            ms->m_settings.m_lfo2_pan_enabled = val;
            break;
        }
    }
    else
        printf("Must be a boolean 0 or 1\n");
}

void minisynth_set_lfo_amp_int(minisynth *ms, int lfo_num, double val)
{
    if (val >= 0 && val <= 1)
    {
        switch (lfo_num)
        {
        case (1):
            ms->m_settings.m_lfo1_amp_intensity = val;
            break;
        case (2):
            ms->m_settings.m_lfo2_amp_intensity = val;
            break;
        }
    }
    else
        printf("val must be between 0 and 1\n");
}

void minisynth_set_lfo_amp(minisynth *ms, int lfo_num, double val)
{
    if (val >= 0 && val <= 1)
    {
        switch (lfo_num)
        {
        case (1):
            ms->m_settings.m_lfo1_amplitude = val;
            break;
        case (2):
            ms->m_settings.m_lfo2_amplitude = val;
            break;
        }
    }
    else
        printf("val must be between 0 and 1\n");
}

void minisynth_set_lfo_filter_fc_int(minisynth *ms, int lfo_num, double val)
{
    if (val >= -1 && val <= 1)
    {
        switch (lfo_num)
        {
        case (1):
            ms->m_settings.m_lfo1_filter_fc_intensity = val;
            break;
        case (2):
            ms->m_settings.m_lfo2_filter_fc_intensity = val;
            break;
        }
    }
    else
        printf("val must be between -1 and 1\n");
}

void minisynth_set_lfo_rate(minisynth *ms, int lfo_num, double val)
{
    if (val >= 0.02 && val <= 20)
    {
        switch (lfo_num)
        {
        case (1):
            ms->m_settings.m_lfo1_rate = val;
            break;
        case (2):
            ms->m_settings.m_lfo2_rate = val;
            break;
        }
    }
    else
        printf("val must be between 0.02 and 20\n");
}

void minisynth_set_lfo_pan_int(minisynth *ms, int lfo_num, double val)
{
    if (val >= 0 && val <= 1)
    {
        switch (lfo_num)
        {
        case (1):
            ms->m_settings.m_lfo1_pan_intensity = val;
            break;
        case (2):
            ms->m_settings.m_lfo2_pan_intensity = val;
            break;
        }
    }
    else
        printf("val must be between 0 and 1\n");
}

void minisynth_set_lfo_osc_int(minisynth *ms, int lfo_num, double val)
{
    if (val >= -1 && val <= 1)
    {
        switch (lfo_num)
        {
        case (1):
            ms->m_settings.m_lfo1_osc_pitch_intensity = val;
            break;
        case (2):
            ms->m_settings.m_lfo2_osc_pitch_intensity = val;
            break;
        }
    }
    else
        printf("val must be between -1 and 1\n");
}

void minisynth_set_lfo_wave(minisynth *ms, int lfo_num, unsigned int val)
{
    if (val < MAX_LFO_OSC)
    {
        switch (lfo_num)
        {
        case (1):
            ms->m_settings.m_lfo1_waveform = val;
            break;
        case (2):
            ms->m_settings.m_lfo2_waveform = val;
            break;
        }
    }
    else
        printf("val must be between 0 and %d\n", MAX_LFO_OSC);
}

void minisynth_set_lfo_mode(minisynth *ms, int lfo_num, unsigned int val)
{
    if (val < LFO_MAX_MODE)
    {
        switch (lfo_num)
        {
        case (1):
            ms->m_settings.m_lfo1_mode = val;
            break;
        case (2):
            ms->m_settings.m_lfo2_mode = val;
            break;
        }
    }
    else
        printf("val must be between 0 and %d\n", LFO_MAX_MODE - 1);
}

void minisynth_set_note_to_decay_scaling(minisynth *ms, unsigned int val)
{
    if (val != 0 && val != 1)
    {
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
        ms->m_settings.m_sustain_level = val;
    else
        printf("val must be between 0 and 1\n");
}

void minisynth_set_sustain_override(minisynth *ms, bool b)
{
    ms->m_settings.m_sustain_override = b;
}

void minisynth_set_velocity_to_attack_scaling(minisynth *ms, unsigned int val)
{
    if (val != 0 && val != 1)
    {
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
    if (val != 0 && val != 1)
    {
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

void minisynth_set_monophonic(minisynth *ms, bool b)
{
    ms->m_settings.m_monophonic = b;
}
void minisynth_set_bitshift(minisynth *ms, bool b)
{
    ms->m_settings.m_bitshift_active = b;
}

void minisynth_add_last_note(minisynth *ms, unsigned int val)
{
    for (int i = 1; i < MAX_VOICES; i++)
        ms->m_last_midi_notes[i - 1] = ms->m_last_midi_notes[i];
    ms->m_last_midi_notes[MAX_VOICES - 1] = val;
}

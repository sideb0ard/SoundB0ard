#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "dxsynth.h"
#include "midi_freq_table.h"
#include "mixer.h"
#include "sequencer_utils.h"
#include "utils.h"

extern mixer *mixr;
extern const wchar_t *sparkchars;
extern const char *s_source_enum_to_name[];
extern const char *s_dest_enum_to_name[];

static char *s_dx_dest_names[] = {"DX_DEST_NONE", "DX_DEST_AMP_MOD",
                                  "DX_DEST_VIBRATO"};

dxsynth *new_dxsynth(void)
{
    dxsynth *dx = (dxsynth *)calloc(1, sizeof(dxsynth));
    if (dx == NULL)
        return NULL; // barf

    synthbase_init(&dx->base, (void *)dx, DXSYNTH_TYPE);

    dx->sound_generator.gennext = &dxsynth_gennext;
    dx->sound_generator.ps_status = &dxsynth_status;
    dx->sound_generator.full_status = &dxsynth_status;
    dx->sound_generator.setvol = &dxsynth_setvol;
    dx->sound_generator.getvol = &dxsynth_getvol;
    dx->sound_generator.start = &dxsynth_sg_start;
    dx->sound_generator.stop = &dxsynth_sg_stop;
    dx->sound_generator.get_num_tracks = &dxsynth_get_num_tracks;
    dx->sound_generator.event_notify = &synthbase_event_notify;
    dx->sound_generator.make_active_track = &dxsynth_make_active_track;
    dx->sound_generator.self_destruct = &dxsynth_del_self;
    dx->sound_generator.set_pattern = &dxsynth_set_pattern;
    dx->sound_generator.get_pattern = &dxsynth_get_pattern;
    dx->sound_generator.type = DXSYNTH_TYPE;

    strncpy(dx->m_settings.m_settings_name, "DEFAULT", 7);
    dx->m_settings.m_volume_db = 0;
    dx->m_settings.m_voice_mode = 0;
    dx->m_settings.m_portamento_time_ms = 0;
    dx->m_settings.m_pitchbend_range = 1; // 0 -12
    dx->m_settings.m_velocity_to_attack_scaling = 0;
    dx->m_settings.m_note_number_to_decay_scaling = 0;
    dx->m_settings.m_reset_to_zero = 0;
    dx->m_settings.m_legato_mode = 0;

    dx->m_settings.m_lfo1_intensity = 1;
    dx->m_settings.m_lfo1_rate = 0.5;
    dx->m_settings.m_lfo1_waveform = 0;
    dx->m_settings.m_lfo1_mod_dest1 = DX_LFO_DEST_NONE;
    dx->m_settings.m_lfo1_mod_dest2 = DX_LFO_DEST_NONE;
    dx->m_settings.m_lfo1_mod_dest3 = DX_LFO_DEST_NONE;
    dx->m_settings.m_lfo1_mod_dest4 = DX_LFO_DEST_NONE;

    dx->m_settings.m_op1_waveform = SINE;
    dx->m_settings.m_op1_ratio = 1; // 0.01-10
    dx->m_settings.m_op1_detune_cents = 0;
    dx->m_settings.m_eg1_attack_ms = 100;
    dx->m_settings.m_eg1_decay_ms = 100;
    dx->m_settings.m_eg1_sustain_lvl = 0.707;
    dx->m_settings.m_eg1_release_ms = 2000;
    dx->m_settings.m_op1_output_lvl = 75;

    dx->m_settings.m_op2_waveform = SINE;
    dx->m_settings.m_op2_ratio = 1; // 0.01-10
    dx->m_settings.m_op2_detune_cents = 0;
    dx->m_settings.m_eg2_attack_ms = 100;
    dx->m_settings.m_eg2_decay_ms = 100;
    dx->m_settings.m_eg2_sustain_lvl = 0.707;
    dx->m_settings.m_eg2_release_ms = 2000;
    dx->m_settings.m_op2_output_lvl = 75;

    dx->m_settings.m_op3_waveform = SINE;
    dx->m_settings.m_op3_ratio = 1; // 0.01-10
    dx->m_settings.m_op3_detune_cents = 0;
    dx->m_settings.m_eg3_attack_ms = 100;
    dx->m_settings.m_eg3_decay_ms = 100;
    dx->m_settings.m_eg3_sustain_lvl = 0.707;
    dx->m_settings.m_eg3_release_ms = 2000;
    dx->m_settings.m_op3_output_lvl = 75;

    dx->m_settings.m_op4_waveform = SINE;
    dx->m_settings.m_op4_ratio = 1; // 0.01-10
    dx->m_settings.m_op4_detune_cents = 0;
    dx->m_settings.m_eg4_attack_ms = 100;
    dx->m_settings.m_eg4_decay_ms = 100;
    dx->m_settings.m_eg4_sustain_lvl = 0.707;
    dx->m_settings.m_eg4_release_ms = 2000;
    dx->m_settings.m_op4_output_lvl = 75;
    dx->m_settings.m_op4_feedback = 0; // 0-70

    for (int i = 0; i < MAX_DX_VOICES; i++)
    {
        dx->m_voices[i] = new_dxsynth_voice();
        if (!dx->m_voices[i])
            return NULL; // would be bad

        dxsynth_voice_init_global_parameters(dx->m_voices[i],
                                             &dx->m_global_synth_params);
    }

    dxsynth_prepare_for_play(dx);

    // use first voice to setup global
    dxsynth_voice_initialize_modmatrix(dx->m_voices[0],
                                       &dx->m_global_modmatrix);

    for (int i = 0; i < MAX_DX_VOICES; i++)
    {
        voice_set_modmatrix_core(&dx->m_voices[i]->m_voice,
                                 get_matrix_core(&dx->m_global_modmatrix));
    }
    dxsynth_update(dx);

    dx->m_last_note_frequency = -1.0;

    dx->vol = 1;
    dx->sound_generator.active = true;
    printf("BOOM!\n");
    return dx;
}

////////////////////////////////////

bool dxsynth_prepare_for_play(dxsynth *dx)
{
    for (int i = 0; i < MAX_DX_VOICES; i++)
    {
        if (dx->m_voices[i])
        {
            dxsynth_voice_prepare_for_play(dx->m_voices[i]);
        }
    }

    dxsynth_update(dx);

    return true;
}

void dxsynth_update(dxsynth *dx)
{
    dx->m_global_synth_params.voice_params.voice_mode =
        dx->m_settings.m_voice_mode;
    dx->m_global_synth_params.voice_params.op4_feedback =
        dx->m_settings.m_op4_feedback / 100.0;
    dx->m_global_synth_params.voice_params.portamento_time_msec =
        dx->m_settings.m_portamento_time_ms;

    dx->m_global_synth_params.voice_params.osc_fo_pitchbend_mod_range =
        dx->m_settings.m_pitchbend_range;

    dx->m_global_synth_params.voice_params.lfo1_osc_mod_intensity =
        dx->m_settings.m_lfo1_intensity;

    dx->m_global_synth_params.osc1_params.amplitude =
        calculate_dx_amp(dx->m_settings.m_op1_output_lvl);
    dx->m_global_synth_params.osc2_params.amplitude =
        calculate_dx_amp(dx->m_settings.m_op2_output_lvl);
    dx->m_global_synth_params.osc3_params.amplitude =
        calculate_dx_amp(dx->m_settings.m_op3_output_lvl);
    dx->m_global_synth_params.osc4_params.amplitude =
        calculate_dx_amp(dx->m_settings.m_op4_output_lvl);

    dx->m_global_synth_params.osc1_params.fo_ratio = dx->m_settings.m_op1_ratio;
    dx->m_global_synth_params.osc2_params.fo_ratio = dx->m_settings.m_op2_ratio;
    dx->m_global_synth_params.osc3_params.fo_ratio = dx->m_settings.m_op3_ratio;
    dx->m_global_synth_params.osc4_params.fo_ratio = dx->m_settings.m_op4_ratio;

    dx->m_global_synth_params.osc1_params.waveform =
        dx->m_settings.m_op1_waveform;
    dx->m_global_synth_params.osc2_params.waveform =
        dx->m_settings.m_op2_waveform;
    dx->m_global_synth_params.osc3_params.waveform =
        dx->m_settings.m_op3_waveform;
    dx->m_global_synth_params.osc4_params.waveform =
        dx->m_settings.m_op4_waveform;

    dx->m_global_synth_params.osc1_params.cents =
        dx->m_settings.m_op1_detune_cents;
    dx->m_global_synth_params.osc2_params.cents =
        dx->m_settings.m_op2_detune_cents;
    dx->m_global_synth_params.osc3_params.cents =
        dx->m_settings.m_op3_detune_cents;
    dx->m_global_synth_params.osc4_params.cents =
        dx->m_settings.m_op4_detune_cents;

    // EG1
    dx->m_global_synth_params.eg1_params.attack_time_msec =
        dx->m_settings.m_eg1_attack_ms;
    dx->m_global_synth_params.eg1_params.decay_time_msec =
        dx->m_settings.m_eg1_decay_ms;
    dx->m_global_synth_params.eg1_params.sustain_level =
        dx->m_settings.m_eg1_sustain_lvl;
    dx->m_global_synth_params.eg1_params.release_time_msec =
        dx->m_settings.m_eg1_release_ms;
    dx->m_global_synth_params.eg1_params.reset_to_zero =
        (bool)dx->m_settings.m_reset_to_zero;
    dx->m_global_synth_params.eg1_params.legato_mode =
        (bool)dx->m_settings.m_legato_mode;

    // EG2
    dx->m_global_synth_params.eg2_params.attack_time_msec =
        dx->m_settings.m_eg2_attack_ms;
    dx->m_global_synth_params.eg2_params.decay_time_msec =
        dx->m_settings.m_eg2_decay_ms;
    dx->m_global_synth_params.eg2_params.sustain_level =
        dx->m_settings.m_eg2_sustain_lvl;
    dx->m_global_synth_params.eg2_params.release_time_msec =
        dx->m_settings.m_eg2_release_ms;
    dx->m_global_synth_params.eg2_params.reset_to_zero =
        (bool)dx->m_settings.m_reset_to_zero;
    dx->m_global_synth_params.eg2_params.legato_mode =
        (bool)dx->m_settings.m_legato_mode;

    // EG3
    dx->m_global_synth_params.eg3_params.attack_time_msec =
        dx->m_settings.m_eg3_attack_ms;
    dx->m_global_synth_params.eg3_params.decay_time_msec =
        dx->m_settings.m_eg3_decay_ms;
    dx->m_global_synth_params.eg3_params.sustain_level =
        dx->m_settings.m_eg3_sustain_lvl;
    dx->m_global_synth_params.eg3_params.release_time_msec =
        dx->m_settings.m_eg3_release_ms;
    dx->m_global_synth_params.eg3_params.reset_to_zero =
        (bool)dx->m_settings.m_reset_to_zero;
    dx->m_global_synth_params.eg3_params.legato_mode =
        (bool)dx->m_settings.m_legato_mode;

    // EG4
    dx->m_global_synth_params.eg4_params.attack_time_msec =
        dx->m_settings.m_eg4_attack_ms;
    dx->m_global_synth_params.eg4_params.decay_time_msec =
        dx->m_settings.m_eg4_decay_ms;
    dx->m_global_synth_params.eg4_params.sustain_level =
        dx->m_settings.m_eg4_sustain_lvl;
    dx->m_global_synth_params.eg4_params.release_time_msec =
        dx->m_settings.m_eg4_release_ms;
    dx->m_global_synth_params.eg4_params.reset_to_zero =
        (bool)dx->m_settings.m_reset_to_zero;
    dx->m_global_synth_params.eg4_params.legato_mode =
        (bool)dx->m_settings.m_legato_mode;

    // LFO1
    dx->m_global_synth_params.lfo1_params.waveform =
        dx->m_settings.m_lfo1_waveform;
    dx->m_global_synth_params.lfo1_params.osc_fo = dx->m_settings.m_lfo1_rate;

    // DCA
    dx->m_global_synth_params.dca_params.amplitude_db =
        dx->m_settings.m_volume_db;

    if (dx->m_settings.m_lfo1_mod_dest1 == DX_LFO_DEST_NONE)
    {
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1,
                          DEST_OSC1_OUTPUT_AMP, false);
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1, DEST_OSC1_FO,
                          false);
    }
    else if (dx->m_settings.m_lfo1_mod_dest1 == DX_LFO_DEST_AMP_MOD)
    {
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1,
                          DEST_OSC1_OUTPUT_AMP, true);
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1, DEST_OSC1_FO,
                          false);
    }
    else // vibrato
    {
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1,
                          DEST_OSC1_OUTPUT_AMP, true);
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1, DEST_OSC1_FO,
                          false);
    }

    // LFO1 DEST2
    if (dx->m_settings.m_lfo1_mod_dest2 == DX_LFO_DEST_NONE)
    {
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1,
                          DEST_OSC2_OUTPUT_AMP, false);
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1, DEST_OSC2_FO,
                          false);
    }
    else if (dx->m_settings.m_lfo1_mod_dest2 == DX_LFO_DEST_AMP_MOD)
    {
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1,
                          DEST_OSC2_OUTPUT_AMP, true);
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1, DEST_OSC2_FO,
                          false);
    }
    else // vibrato
    {
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1,
                          DEST_OSC2_OUTPUT_AMP, true);
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1, DEST_OSC2_FO,
                          false);
    }

    // LFO1 DEST3
    if (dx->m_settings.m_lfo1_mod_dest3 == DX_LFO_DEST_NONE)
    {
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1,
                          DEST_OSC3_OUTPUT_AMP, false);
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1, DEST_OSC3_FO,
                          false);
    }
    else if (dx->m_settings.m_lfo1_mod_dest3 == DX_LFO_DEST_AMP_MOD)
    {
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1,
                          DEST_OSC3_OUTPUT_AMP, true);
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1, DEST_OSC3_FO,
                          false);
    }
    else // vibrato
    {
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1,
                          DEST_OSC3_OUTPUT_AMP, true);
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1, DEST_OSC3_FO,
                          false);
    }

    // LFO1 DEST4
    if (dx->m_settings.m_lfo1_mod_dest4 == DX_LFO_DEST_NONE)
    {
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1,
                          DEST_OSC4_OUTPUT_AMP, false);
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1, DEST_OSC4_FO,
                          false);
    }
    else if (dx->m_settings.m_lfo1_mod_dest4 == DX_LFO_DEST_AMP_MOD)
    {
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1,
                          DEST_OSC4_OUTPUT_AMP, true);
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1, DEST_OSC4_FO,
                          false);
    }
    else // vibrato
    {
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1,
                          DEST_OSC4_OUTPUT_AMP, true);
        enable_matrix_row(&dx->m_global_modmatrix, SOURCE_LFO1, DEST_OSC4_FO,
                          false);
    }
}

void dxsynth_midi_control(dxsynth *dx, unsigned int data1, unsigned int data2)
{
    double val = 0;
    switch (data1)
    {
    case (1):
        printf("op1ratio\n");
        val = scaleybum(0, 127, 0.01, 10, data2);
        dxsynth_set_op_ratio(dx, 1, val);
        break;
    case (2):
        printf("op2ratio\n");
        val = scaleybum(0, 127, 0.01, 10, data2);
        dxsynth_set_op_ratio(dx, 2, val);
        break;
    case (3):
        printf("op3ratio\n");
        val = scaleybum(0, 127, 0.01, 10, data2);
        dxsynth_set_op_ratio(dx, 3, val);
        break;
    case (4):
        printf("op4ratio\n");
        val = scaleybum(0, 127, 0.01, 10, data2);
        dxsynth_set_op_ratio(dx, 4, val);
        break;
    case (5):
        printf("LFO Rate\n");
        val = scaleybum(0, 128, MIN_LFO_RATE, MAX_LFO_RATE, data2);
        dxsynth_set_lfo1_rate(dx, val);
        break;
    case (6):
        printf("LFO Intensity\n");
        val = scaleybum(0, 128, 0.0, 1.0, data2);
        dxsynth_set_lfo1_intensity(dx, val);
        break;
    case (7):
        printf("Op4Feedback\n");
        val = scaleybum(0, 127, 0, 70, data2);
        dxsynth_set_op4_feedback(dx, val);
        break;
    case (8):
        printf("Algo\n");
        val = scaleybum(0, 127, 1, 8, data2);
        dxsynth_set_voice_mode(dx, val);
        break;
    default:
        printf("nah\n");
    }
    dxsynth_update(dx);
}

bool dxsynth_midi_note_on(dxsynth *ms, unsigned int midinote,
                          unsigned int velocity)
{

    bool steal_note = true;
    for (int i = 0; i < MAX_DX_VOICES; i++)
    {
        dxsynth_voice *msv = ms->m_voices[i];
        if (!msv)
            return false; // should never happen
        if (!msv->m_voice.m_note_on)
        {
            dxsynth_increment_voice_timestamps(ms);
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
        dxsynth_voice *msv = dxsynth_get_oldest_voice(ms);
        if (msv)
        {
            dxsynth_increment_voice_timestamps(ms);
            voice_note_on(&msv->m_voice, midinote, velocity,
                          get_midi_freq(midinote), ms->m_last_note_frequency);
        }
        ms->m_last_note_frequency = get_midi_freq(midinote);
    }

    return true;
}

bool dxsynth_midi_note_off(dxsynth *dx, unsigned int midinote,
                           unsigned int velocity, bool all_notes_off)
{
    (void)velocity;

    if (all_notes_off)
    {
        for (int i = 0; i < MAX_DX_VOICES; i++)
        {
            dxsynth_voice *dxv = dx->m_voices[i];
            if (dxv)
                voice_note_off(&dxv->m_voice, midinote);
        }
        return true;
    }

    for (int i = 0; i < MAX_DX_VOICES; i++)
    {
        dxsynth_voice *dxv = dxsynth_get_oldest_voice_with_note(dx, midinote);
        if (dxv)
        {
            voice_note_off(&dxv->m_voice, midinote);
        }
    }
    return true;
}

void dxsynth_midi_pitchbend(dxsynth *ms, unsigned int data1, unsigned int data2)
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
        for (int i = 0; i < MAX_DX_VOICES; i++)
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
        for (int i = 0; i < MAX_DX_VOICES; i++)
        {
            ms->m_voices[i]->m_voice.m_osc1->m_cents = 0;
            ms->m_voices[i]->m_voice.m_osc2->m_cents = 2.5;
            ms->m_voices[i]->m_voice.m_osc3->m_cents = 0;
            ms->m_voices[i]->m_voice.m_osc4->m_cents = 2.5;
        }
    }
}

void dxsynth_reset_voices(dxsynth *ms)
{
    for (int i = 0; i < MAX_DX_VOICES; i++)
    {
        dxsynth_voice_reset(ms->m_voices[i]);
    }
}

void dxsynth_increment_voice_timestamps(dxsynth *ms)
{
    for (int i = 0; i < MAX_DX_VOICES; i++)
    {
        if (ms->m_voices[i])
        {
            if (ms->m_voices[i]->m_voice.m_note_on)
                ms->m_voices[i]->m_voice.m_timestamp++;
        }
    }
}

dxsynth_voice *dxsynth_get_oldest_voice(dxsynth *ms)
{
    int timestamp = -1;
    dxsynth_voice *found_voice = NULL;
    for (int i = 0; i < MAX_DX_VOICES; i++)
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

dxsynth_voice *dxsynth_get_oldest_voice_with_note(dxsynth *ms,
                                                  unsigned int midi_note)
{
    int timestamp = -1;
    dxsynth_voice *found_voice = NULL;
    for (int i = 0; i < MAX_DX_VOICES; i++)
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
void dxsynth_status(void *self, wchar_t *status_string)
{
    dxsynth *dx = (dxsynth *)self;

    swprintf(
        status_string, MAX_PS_STRING_SZ,
        L"[" WANSI_COLOR_WHITE
        "DXSYNTH '%s' algo:%d] - Vol: %.2f Active:%s porta:%.2f "
        "pitchrange:%d vel2attack:%d note2decay:%d dxreset2zero:%d legato:%d\n"
        "      lfo1_intensity:%.2f lfo1_rate:%0.2f lfo1_waveform:%d "
        "op4feedback:%.2f\n"
        "      lfo1dest1:%s lfo1dest2:%s lfo1dest3:%s lfo1dest4:%s\n"
        "      [Op1] op1wave:%d op1ratio:%.2f op1detune:%.2f eg1attackms:%.2f "
        "eg1decayms:%.2f eg1sustainlvl:%.2f eg1releasems:%.2f op1output:%.2f\n"
        "      [Op2] op2wave:%d op2ratio:%.2f op2detune:%.2f eg2attackms:%.2f "
        "eg2decayms:%.2f eg2sustainlvl:%.2f eg2releasems:%.2f op2output:%.2f\n"
        "      [Op3] op3wave:%d op3ratio:%.2f op3detune:%.2f eg3attackms:%.2f "
        "eg3decayms:%.2f eg3sustainlvl:%.2f eg3releasems:%.2f op3output:%.2f\n"
        "      [Op4] op4wave:%d op4ratio:%.2f op4detune:%.2f eg4attackms:%.2f "
        "eg4decayms:%.2f eg4sustainlvl:%.2f eg4releasems:%.2f op4output:%.2f\n",

        dx->m_settings.m_settings_name, dx->m_settings.m_voice_mode, dx->vol,
        dx->sound_generator.active ? "true" : " false",
        dx->m_settings.m_portamento_time_ms, dx->m_settings.m_pitchbend_range,
        dx->m_settings.m_velocity_to_attack_scaling,
        dx->m_settings.m_note_number_to_decay_scaling,
        dx->m_settings.m_reset_to_zero, dx->m_settings.m_legato_mode,

        dx->m_settings.m_lfo1_intensity, dx->m_settings.m_lfo1_rate,
        dx->m_settings.m_lfo1_waveform, dx->m_settings.m_op4_feedback,
        s_dx_dest_names[dx->m_settings.m_lfo1_mod_dest1],
        s_dx_dest_names[dx->m_settings.m_lfo1_mod_dest2],
        s_dx_dest_names[dx->m_settings.m_lfo1_mod_dest3],
        s_dx_dest_names[dx->m_settings.m_lfo1_mod_dest4],

        dx->m_settings.m_op1_waveform, dx->m_settings.m_op1_ratio,
        dx->m_settings.m_op1_detune_cents, dx->m_settings.m_eg1_attack_ms,
        dx->m_settings.m_eg1_decay_ms, dx->m_settings.m_eg1_sustain_lvl,
        dx->m_settings.m_eg1_release_ms, dx->m_settings.m_op1_output_lvl,

        dx->m_settings.m_op2_waveform, dx->m_settings.m_op2_ratio,
        dx->m_settings.m_op2_detune_cents, dx->m_settings.m_eg2_attack_ms,
        dx->m_settings.m_eg2_decay_ms, dx->m_settings.m_eg2_sustain_lvl,
        dx->m_settings.m_eg2_release_ms, dx->m_settings.m_op2_output_lvl,

        dx->m_settings.m_op3_waveform, dx->m_settings.m_op3_ratio,
        dx->m_settings.m_op3_detune_cents, dx->m_settings.m_eg3_attack_ms,
        dx->m_settings.m_eg3_decay_ms, dx->m_settings.m_eg3_sustain_lvl,
        dx->m_settings.m_eg3_release_ms, dx->m_settings.m_op3_output_lvl,

        dx->m_settings.m_op4_waveform, dx->m_settings.m_op4_ratio,
        dx->m_settings.m_op4_detune_cents, dx->m_settings.m_eg4_attack_ms,
        dx->m_settings.m_eg4_decay_ms, dx->m_settings.m_eg4_sustain_lvl,
        dx->m_settings.m_eg4_release_ms, dx->m_settings.m_op4_output_lvl

        );
    wchar_t scratch[1024];
    synthbase_status(&dx->base, scratch);
    wcscat(status_string, scratch);
}

void dxsynth_setvol(void *self, double v)
{
    dxsynth *dx = (dxsynth *)self;
    if (v < 0.0 || v > 1.0)
    {
        return;
    }
    dx->vol = v;
}

double dxsynth_getvol(void *self)
{
    dxsynth *dx = (dxsynth *)self;
    return dx->vol;
}

int dxsynth_get_num_tracks(void *self)
{
    dxsynth *ms = (dxsynth *)self;
    return synthbase_get_num_tracks(&ms->base);
}

void dxsynth_make_active_track(void *self, int tracknum)
{
    dxsynth *ms = (dxsynth *)self;
    synthbase_make_active_track(&ms->base, tracknum);
}

stereo_val dxsynth_gennext(void *self)
{
    dxsynth *dx = (dxsynth *)self;

    if (!dx->sound_generator.active)
        return (stereo_val){0, 0};

    double accum_out_left = 0.0;
    double accum_out_right = 0.0;

    // float mix = 1.0 / MAX_DX_VOICES;
    float mix = 0.25;

    double out_left = 0.0;
    double out_right = 0.0;

    for (int i = 0; i < MAX_DX_VOICES; i++)
    {
        if (dx->m_voices[i])
            dxsynth_voice_gennext(dx->m_voices[i], &out_left, &out_right);

        accum_out_left += mix * out_left;
        accum_out_right += mix * out_right;
    }

    accum_out_left = effector(&dx->sound_generator, accum_out_left);
    accum_out_left = envelopor(&dx->sound_generator, accum_out_left);
    accum_out_left *= dx->vol;

    accum_out_right = effector(&dx->sound_generator, accum_out_right);
    accum_out_right = envelopor(&dx->sound_generator, accum_out_right);
    accum_out_right *= dx->vol;

    return (stereo_val){.left = accum_out_left, .right = accum_out_right};
}

void dxsynth_rand_settings(dxsynth *dx)
{
    printf("Randomizing DXSYNTH!\n");

    dx->m_settings.m_voice_mode = rand() % 8;
    dx->m_settings.m_portamento_time_ms = rand() % 5000;
    dx->m_settings.m_pitchbend_range = (rand() % 12) + 1;
    dx->m_settings.m_velocity_to_attack_scaling = rand() % 2;
    dx->m_settings.m_note_number_to_decay_scaling = rand() % 2;
    dx->m_settings.m_reset_to_zero = rand() % 2;
    dx->m_settings.m_legato_mode = rand() % 2;

    dx->m_settings.m_lfo1_intensity = ((float)rand()) / RAND_MAX;
    dx->m_settings.m_lfo1_rate = 0.02 + ((float)rand()) / (RAND_MAX / 20);
    dx->m_settings.m_lfo1_waveform = rand() % MAX_LFO_OSC;
    dx->m_settings.m_lfo1_mod_dest1 = rand() % 3;
    dx->m_settings.m_lfo1_mod_dest2 = rand() % 3;
    dx->m_settings.m_lfo1_mod_dest3 = rand() % 3;
    dx->m_settings.m_lfo1_mod_dest4 = rand() % 3;

    // dx->m_settings.m_op1_waveform = rand() % MAX_OSC;
    dx->m_settings.m_op1_ratio = 0.1 + ((float)rand()) / (RAND_MAX / 10);
    // dx->m_settings.m_op1_detune_cents = (rand() % 200) - 100;
    // dx->m_settings.m_eg1_attack_ms = rand()% EG_MAXTIME_MS;
    // dx->m_settings.m_eg1_decay_ms = rand()% EG_MAXTIME_MS;
    // dx->m_settings.m_eg1_sustain_lvl = ((float)rand()) / RAND_MAX;
    // dx->m_settings.m_eg1_release_ms = rand()% EG_MAXTIME_MS;
    // dx->m_settings.m_op1_output_lvl = ((float)rand()) / RAND_MAX;

    // dx->m_settings.m_op2_waveform = rand() % MAX_OSC;
    dx->m_settings.m_op2_ratio = 0.1 + ((float)rand()) / (RAND_MAX / 10);
    // dx->m_settings.m_op2_detune_cents = (rand() % 200) - 100;
    // dx->m_settings.m_eg2_attack_ms = rand()% EG_MAXTIME_MS;
    // dx->m_settings.m_eg2_decay_ms = rand()% EG_MAXTIME_MS;
    // dx->m_settings.m_eg2_sustain_lvl = ((float)rand()) / RAND_MAX;
    // dx->m_settings.m_eg2_release_ms = rand()% EG_MAXTIME_MS;
    // dx->m_settings.m_op2_output_lvl = ((float)rand()) / RAND_MAX;

    // dx->m_settings.m_op3_waveform = rand() % MAX_OSC;
    dx->m_settings.m_op3_ratio = 0.1 + ((float)rand()) / (RAND_MAX / 10);
    // dx->m_settings.m_op3_detune_cents = (rand() % 200) - 100;
    // dx->m_settings.m_eg3_attack_ms = rand()% EG_MAXTIME_MS;
    // dx->m_settings.m_eg3_decay_ms = rand()% EG_MAXTIME_MS;
    // dx->m_settings.m_eg3_sustain_lvl = ((float)rand()) / RAND_MAX;
    // dx->m_settings.m_eg3_release_ms = rand()% EG_MAXTIME_MS;
    // dx->m_settings.m_op3_output_lvl = ((float)rand()) / RAND_MAX;

    // dx->m_settings.m_op4_waveform = rand() % MAX_OSC;
    dx->m_settings.m_op4_ratio = 0.1 + ((float)rand()) / (RAND_MAX / 10);
    // dx->m_settings.m_op4_detune_cents = (rand() % 200) - 100;
    // dx->m_settings.m_eg4_attack_ms = rand()% EG_MAXTIME_MS;
    // dx->m_settings.m_eg4_decay_ms = rand()% EG_MAXTIME_MS;
    // dx->m_settings.m_eg4_sustain_lvl = ((float)rand()) / RAND_MAX;
    // dx->m_settings.m_eg4_release_ms = rand()% EG_MAXTIME_MS;
    // dx->m_settings.m_op4_output_lvl = ((float)rand()) / RAND_MAX;
    // dx->m_settings.m_op4_feedback = rand() % 70;

    dxsynth_update(dx);
    // dxsynth_print_settings(dx);
}

bool dxsynth_save_settings(dxsynth *ms, char *preset_name)
{
    if (strlen(preset_name) == 0)
    {
        printf("Play tha game, pal, need a name to save yer synth settings "
               "with\n");
        return false;
    }
    printf("Saving '%s' settings for dxsynth to file %s\n", preset_name,
           PRESET_FILENAME);
    FILE *presetzzz = fopen(PRESET_FILENAME, "a+");
    if (presetzzz == NULL)
    {
        printf("Couldn't save settings!!\n");
        return false;
    }

    int settings_count = 0;
    // strncpy(ms->m_settings.m_settings_name, preset_name, 256);

    // fprintf(presetzzz, "::name=%s", ms->m_settings.m_settings_name);
    // settings_count++;

    // fprintf(presetzzz, "::voice_mode=%d", ms->m_settings.m_voice_mode);
    // settings_count++;

    // fprintf(presetzzz, "::monophonic=%d", ms->m_settings.m_monophonic);
    // settings_count++;
    fprintf(presetzzz, ":::\n");

    fclose(presetzzz);
    printf("Wrote %d settings\n", settings_count++);
    return true;
}

bool dxsynth_list_presets()
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

bool dxsynth_check_if_preset_exists(char *preset_to_find)
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
bool dxsynth_load_settings(dxsynth *ms, char *preset_to_load)
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
        }
        if (settings_count > 0)
            printf("Loaded %d settings\n", settings_count);
        dxsynth_update(ms);
    }

    fclose(presetzzz);
    return true;
}

void dxsynth_print_settings(dxsynth *ms)
{
    printf(ANSI_COLOR_WHITE); // CONTROL PANEL
    printf("///////////////////// SYNTHzzz! ///////////////////////\n");

    printf(ANSI_COLOR_RESET);
}

void dxsynth_print_melodies(dxsynth *ms)
{
    synthbase_print_melodies(&ms->base);
}

void dxsynth_print_modulation_routings(dxsynth *ms)
{
    print_modulation_matrix(&ms->m_global_modmatrix);
}

void dxsynth_del_self(void *self)
{
    dxsynth *dx = (dxsynth *)self;
    for (int i = 0; i < MAX_DX_VOICES; i++)
    {
        dxsynth_voice_free_self(dx->m_voices[i]);
    }
    printf("Deleting dxsynth self\n");
    free(dx);
}

void dxsynth_stop(dxsynth *dx) { dxsynth_midi_note_off(dx, 0, 0, true); }

void dxsynth_sg_start(void *self)
{
    dxsynth *ms = (dxsynth *)self;
    ms->sound_generator.active = true;
}

void dxsynth_sg_stop(void *self)
{
    dxsynth *ms = (dxsynth *)self;
    ms->sound_generator.active = false;
    dxsynth_stop(ms);
}

void dxsynth_set_lfo1_intensity(dxsynth *d, double val)
{
    if (val >= 0.0 && val <= 1.0)
        d->m_settings.m_lfo1_intensity = val;
    else
        printf("Val has to be between 0.0-1.0\n");
}

void dxsynth_set_lfo1_rate(dxsynth *d, double val)
{
    if (val >= 0.02 && val <= 20.0)
        d->m_settings.m_lfo1_rate = val;
    else
        printf("Val has to be between 0.02 - 20.0\n");
}

void dxsynth_set_lfo1_waveform(dxsynth *d, unsigned int val)
{
    if (val < MAX_LFO_OSC)
        d->m_settings.m_lfo1_waveform = val;
    else
        printf("Val has to be between [0-%d]\n", MAX_LFO_OSC);
}

void dxsynth_set_lfo1_mod_dest(dxsynth *d, unsigned int mod_dest,
                               unsigned int dest)
{
    if (dest > 2)
    {
        printf("Dest has to be [0-2]\n");
        return;
    }
    switch (mod_dest)
    {
    case (1):
        d->m_settings.m_lfo1_mod_dest1 = dest;
        break;
    case (2):
        d->m_settings.m_lfo1_mod_dest2 = dest;
        break;
    case (3):
        d->m_settings.m_lfo1_mod_dest3 = dest;
        break;
    case (4):
        d->m_settings.m_lfo1_mod_dest4 = dest;
        break;
    default:
        printf("Huh?! Only got 4 destinations, brah..\n");
    }
}

void dxsynth_set_op_waveform(dxsynth *d, unsigned int op, unsigned int val)
{
    if (val >= MAX_OSC)
    {
        printf("WAV has to be [0-%d)\n", MAX_OSC);
        return;
    }
    switch (op)
    {
    case (1):
        d->m_settings.m_op1_waveform = val;
        break;
    case (2):
        d->m_settings.m_op2_waveform = val;
        break;
    case (3):
        d->m_settings.m_op3_waveform = val;
        break;
    case (4):
        d->m_settings.m_op4_waveform = val;
        break;
    default:
        printf("Huh?! Only got 4 operators, brah..\n");
    }
}

void dxsynth_set_op_ratio(dxsynth *d, unsigned int op, double val)
{
    if (val < 0.01 || val > 10)
    {
        printf("val has to be [0.01-10]\n");
        return;
    }
    switch (op)
    {
    case (1):
        d->m_settings.m_op1_ratio = val;
        break;
    case (2):
        d->m_settings.m_op2_ratio = val;
        break;
    case (3):
        d->m_settings.m_op3_ratio = val;
        break;
    case (4):
        d->m_settings.m_op4_ratio = val;
        break;
    default:
        printf("Huh?! Only got 4 operators, brah..\n");
    }
}
void dxsynth_set_op_detune(dxsynth *d, unsigned int op, double val)
{
    if (val < -100 || val > 100)
    {
        printf("val has to be [-100-100]\n");
        return;
    }
    switch (op)
    {
    case (1):
        d->m_settings.m_op1_detune_cents = val;
        break;
    case (2):
        d->m_settings.m_op2_detune_cents = val;
        break;
    case (3):
        d->m_settings.m_op3_detune_cents = val;
        break;
    case (4):
        d->m_settings.m_op4_detune_cents = val;
        break;
    default:
        printf("Huh?! Only got 4 operators, brah..\n");
    }
}

void dxsynth_set_eg_attack_ms(dxsynth *d, unsigned int eg, double val)
{
    if (val < EG_MINTIME_MS || val > EG_MAXTIME_MS)
    {
        printf("val has to be [%d - %d]\n", EG_MINTIME_MS, EG_MAXTIME_MS);
        return;
    }
    switch (eg)
    {
    case (1):
        d->m_settings.m_eg1_attack_ms = val;
        break;
    case (2):
        d->m_settings.m_eg2_attack_ms = val;
        break;
    case (3):
        d->m_settings.m_eg3_attack_ms = val;
        break;
    case (4):
        d->m_settings.m_eg4_attack_ms = val;
        break;
    default:
        printf("Huh?! Only got 4 operators, brah..\n");
    }
}

void dxsynth_set_eg_decay_ms(dxsynth *d, unsigned int eg, double val)
{
    if (val < EG_MINTIME_MS || val > EG_MAXTIME_MS)
    {
        printf("val has to be [%d - %d]\n", EG_MINTIME_MS, EG_MAXTIME_MS);
        return;
    }
    switch (eg)
    {
    case (1):
        d->m_settings.m_eg1_decay_ms = val;
        break;
    case (2):
        d->m_settings.m_eg2_decay_ms = val;
        break;
    case (3):
        d->m_settings.m_eg3_decay_ms = val;
        break;
    case (4):
        d->m_settings.m_eg4_decay_ms = val;
        break;
    default:
        printf("Huh?! Only got 4 operators, brah..\n");
    }
}

void dxsynth_set_eg_release_ms(dxsynth *d, unsigned int eg, double val)
{
    if (val < EG_MINTIME_MS || val > EG_MAXTIME_MS)
    {
        printf("val has to be [%d - %d]\n", EG_MINTIME_MS, EG_MAXTIME_MS);
        return;
    }
    switch (eg)
    {
    case (1):
        d->m_settings.m_eg1_release_ms = val;
        break;
    case (2):
        d->m_settings.m_eg2_release_ms = val;
        break;
    case (3):
        d->m_settings.m_eg3_release_ms = val;
        break;
    case (4):
        d->m_settings.m_eg4_release_ms = val;
        break;
    default:
        printf("Huh?! Only got 4 operators, brah..\n");
    }
}

void dxsynth_set_eg_sustain_lvl(dxsynth *d, unsigned int eg, double val)
{
    if (val < 0 || val > 1)
    {
        printf("val has to be [0-1]\n");
        return;
    }
    switch (eg)
    {
    case (1):
        d->m_settings.m_eg1_sustain_lvl = val;
        break;
    case (2):
        d->m_settings.m_eg2_sustain_lvl = val;
        break;
    case (3):
        d->m_settings.m_eg3_sustain_lvl = val;
        break;
    case (4):
        d->m_settings.m_eg4_sustain_lvl = val;
        break;
    default:
        printf("Huh?! Only got 4 operators, brah..\n");
    }
}

void dxsynth_set_op_output_lvl(dxsynth *d, unsigned int op, double val)
{
    if (val < 0 || val > 99)
    {
        printf("val has to be [0-99]\n");
        return;
    }
    switch (op)
    {
    case (1):
        d->m_settings.m_op1_output_lvl = val;
        break;
    case (2):
        d->m_settings.m_op2_output_lvl = val;
        break;
    case (3):
        d->m_settings.m_op3_output_lvl = val;
        break;
    case (4):
        d->m_settings.m_op4_output_lvl = val;
        break;
    default:
        printf("Huh?! Only got 4 operators, brah..\n");
    }
}

void dxsynth_set_portamento_time_ms(dxsynth *d, double val)
{
    if (val >= 0 && val <= 5000.0)
        d->m_settings.m_portamento_time_ms = val;
    else
        printf("Val has to be between 0 - 5000.0\n");
}

void dxsynth_set_volume_db(dxsynth *d, double val)
{
    if (val >= -96 && val <= 20)
        d->m_settings.m_volume_db = val;
    else
        printf("Val has to be between -96 and 20\n");
}

void dxsynth_set_pitchbend_range(dxsynth *d, unsigned int val)
{
    if (val <= 12)
        d->m_settings.m_pitchbend_range = val;
    else
        printf("Val has to be between 0 and 12\n");
}
void dxsynth_set_voice_mode(dxsynth *d, unsigned int val)
{
    if (val < MAXDX)
        d->m_settings.m_voice_mode = val;
    else
        printf("Val has to be [0-%d)\n", MAXDX);
}

void dxsynth_set_velocity_to_attack_scaling(dxsynth *d, bool b)
{
    d->m_settings.m_velocity_to_attack_scaling = b;
}
void dxsynth_set_note_number_to_decay_scaling(dxsynth *d, bool b)
{
    d->m_settings.m_note_number_to_decay_scaling = b;
}

void dxsynth_set_reset_to_zero(dxsynth *d, bool b)
{
    d->m_settings.m_reset_to_zero = b;
}

void dxsynth_set_legato_mode(dxsynth *d, bool b)
{
    d->m_settings.m_legato_mode = b;
}

void dxsynth_set_op4_feedback(dxsynth *d, double val)
{
    if (val >= 0 && val <= 70)
        d->m_settings.m_op4_feedback = val;
    else
        printf("Op4 feedback val has to be [0-70]\n");
}

midi_event *dxsynth_get_pattern(void *self, int pattern_num)
{
    synthbase *base = get_synthbase(self);
    if (base)
        return synthbase_get_pattern(base, pattern_num);

    return NULL;
}

void dxsynth_set_pattern(void *self, int pattern_num, midi_event *pattern)
{
    synthbase *base = get_synthbase(self);
    if (base)
        synthbase_set_pattern(base, pattern_num, pattern);
}

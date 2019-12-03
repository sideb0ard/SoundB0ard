#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "bitshift.h"
#include "midi_freq_table.h"
#include "minisynth.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;
extern const wchar_t *sparkchars;
extern const char *s_source_enum_to_name[];
extern const char *s_dest_enum_to_name[];

// defined in minisynth_voice.h
const wchar_t *s_voice_names[] = {L"saw3",    L"sqr3",    L"saw2sqr",
                                  L"tri2saw", L"tri2sqr", L"sin2sqr"};

char *s_waveform_names[] = {
    (char *)"SINE",  (char *)"SAW1",   (char *)"SAW2",
    (char *)"SAW3",  (char *)"TRI",    (char *)"SQUARE",
    (char *)"NOISE", (char *)"PNOISE", (char *)"MAX_OSC"};
// defined in oscillator.h
const char *s_lfo_mode_names[] = {"sync", "shot", "free"};
const char *s_lfo_wave_names[] = {"sine", "usaw", "dsaw", "tri ",
                                  "squa", "expo", "rsh ", "qrsh"};

const char *s_filter_type_names[] = {"lpf1", "hpf1", "lpf2", "hpf2", "bpf2",
                                     "bsf2", "lpf4", "hpf4", "bpf4"};

minisynth::minisynth()
{
    type = MINISYNTH_TYPE;

    for (int i = 0; i < MAX_VOICES; i++)
    {
        m_voices[i] = new_minisynth_voice();
        if (!m_voices[i])
            return; // would be bad

        minisynth_voice_init_global_parameters(m_voices[i],
                                               &m_global_synth_params);
    }

    // clears out modmatrix sources and resets all oscs, lfos, eg's etc.
    minisynth_prepare_for_play(this);

    // use first voice to setup global
    minisynth_voice_initialize_modmatrix(m_voices[0], &m_ms_modmatrix);

    for (int i = 0; i < MAX_VOICES; i++)
    {
        voice_set_modmatrix_core(&m_voices[i]->m_voice,
                                 get_matrix_core(&m_ms_modmatrix));
    }

    minisynth_load_defaults(this);
    minisynth_update(this);
    active = true;
}

minisynth::~minisynth()
{
    for (int i = 0; i < MAX_VOICES; i++)
    {
        minisynth_voice_free_self(m_voices[i]);
    }
    printf("Deleting MINISYNTH self\n");
}

stereo_val minisynth::genNext()
{

    if (!active)
        return (stereo_val){0, 0};

    double accum_out_left = 0.0;
    double accum_out_right = 0.0;

    float mix = 1.0 / MAX_VOICES;

    double out_left = 0.0;
    double out_right = 0.0;

    for (int i = 0; i < MAX_VOICES; i++)
    {
        if (m_voices[i])
            minisynth_voice_gennext(m_voices[i], &out_left, &out_right);

        accum_out_left += mix * out_left;
        accum_out_right += mix * out_right;
    }

    pan = fmin(pan, 1.0);
    pan = fmax(pan, -1.0);
    double pan_left = 0.707;
    double pan_right = 0.707;
    calculate_pan_values(pan, &pan_left, &pan_right);

    stereo_val out = {.left = accum_out_left * volume * pan_left,
                      .right = accum_out_right * volume * pan_right};

    out = effector(this, out);
    return out;
}
void minisynth::status(wchar_t *status_string)
{
    if (mixr->debug_mode)
    {
        minisynth_print(this);
    }

    char *INSTRUMENT_YELLOW = (char *)ANSI_COLOR_RESET;
    char *INSTRUMENT_ORANGE = (char *)ANSI_COLOR_RESET;
    char *INSTRUMENT_PINK = (char *)ANSI_COLOR_RESET;
    if (active)
    {
        INSTRUMENT_YELLOW = (char *)COOL_COLOR_YELLOW;
        INSTRUMENT_ORANGE = (char *)COOL_COLOR_ORANGE;
        INSTRUMENT_PINK = (char *)COOL_COLOR_PINK;
    }

    // clang-format off
    swprintf(
        status_string, MAX_STATIC_STRING_SZ,
        WANSI_COLOR_WHITE "%s\n"
        "%s"
        "vol:%.1f pan:%.1f voice:" WANSI_COLOR_WHITE "%ls" "%s" "(%d) "
        "mono:%d hard_sync:%d detune:%.0f legato:%d kt:%d ndscale:%d\n"

        "osc1:%s(%d) o1amp:%f o1oct:%d o1semi:%d o1cents%d\n"
        "osc2:%s(%d) o2amp:%f o2oct:%d o2semi:%d o2cents%d\n"
        "osc3:%s(%d) o3amp:%f o3oct:%d o3semi:%d o3cents%d\n"
        "osc4:%s(%d) o4amp:%f o4oct:%d o4semi:%d o4cents%d\n"

        "noisedb:%3.0f octave:%d  pitchrange:%d porta:%.0f  pw:%.0f subosc:%3.0f vascale:%d zero:%d\n"

        "%s"
        "l1wave:%s(%d)      l1mode:%s(%d)   l1rate:%05.2f     l1amp:%.1f\n"
        "l1_filter_en:%d      l1_osc_en:%d      l1_pan_en:%d      l1_amp_en:%d     l1_pw_en:%d\n"
        "l1_filter_int:%4.1f  l1_osc_int:%4.1f  l1_pan_int:%4.1f  l1_amp_int:%4.1f l1_pw_int:%3.1f\n"

        "%s"
        "l2wave:%s(%d)      l2mode:%s(%d)   l2rate:%05.2f     l2amp:%.1f\n"
        "l2_filter_en:%d      l2_osc_en:%d      l2_pan_en:%d      l2_amp_en:%d     l2_pw_en:%d\n"
        "l2_filter_int:%4.1f  l2_osc_int:%4.1f  l2_pan_int:%4.1f  l2_amp_int:%4.1f l2_pw_int:%3.1f\n"


        "%s"
        "eg1_filter_en:%d     eg1_osc_en:%d     eg1_dca_en:%d     eg1_sustain:%d\n"
        "eg1_filter_int:%4.1f eg1_osc_int:%4.1f eg1_dca_int:%4.1f eg1_sustainlvl:%.1f\n"
        "eg1_attack:%4.0f     eg1_decay:%4.0f   eg1_release:%4.0f\n"

        "%s"
        "eg2_filter_en:%d     eg2_osc_en:%d     eg2_dca_en:%d     eg2_sustain:%d\n"
        "eg2_filter_int:%4.1f eg2_osc_int:%4.1f eg2_dca_int:%4.1f eg2_sustainlvl:%.1f\n"
        "eg2_attack:%4.0f     eg2_decay:%4.0f   eg2_release:%4.0f\n"

        "%s"
        "filter:%s(%d)      fc:%7.1f       fq:%4.1f          aux:%0.2f sat:%0.2f"

        "%s",

        m_settings.m_settings_name,
        INSTRUMENT_YELLOW,
        volume,
        pan,
        s_voice_names[m_settings.m_voice_mode],
        INSTRUMENT_YELLOW,
        m_settings.m_voice_mode,

        m_settings.m_monophonic,
        m_settings.hard_sync,
        m_settings.m_detune_cents,
        m_settings.m_legato_mode,
        m_settings.m_filter_keytrack,
        m_settings.m_note_number_to_decay_scaling,

        s_waveform_names[m_global_synth_params.osc1_params.waveform],
        m_global_synth_params.osc1_params.waveform,
        m_global_synth_params.osc1_params.amplitude,
        m_global_synth_params.osc1_params.octave,
        m_global_synth_params.osc1_params.semitones,
        m_global_synth_params.osc1_params.cents,

        s_waveform_names[m_global_synth_params.osc2_params.waveform],
        m_global_synth_params.osc2_params.waveform,
        m_global_synth_params.osc2_params.amplitude,
        m_global_synth_params.osc2_params.octave,
        m_global_synth_params.osc2_params.semitones,
        m_global_synth_params.osc2_params.cents,

        s_waveform_names[m_global_synth_params.osc3_params.waveform],
        m_global_synth_params.osc3_params.waveform,
        m_global_synth_params.osc3_params.amplitude,
        m_global_synth_params.osc3_params.octave,
        m_global_synth_params.osc3_params.semitones,
        m_global_synth_params.osc3_params.cents,

        s_waveform_names[m_global_synth_params.osc4_params.waveform],
        m_global_synth_params.osc4_params.waveform,
        m_global_synth_params.osc4_params.amplitude,
        m_global_synth_params.osc4_params.octave,
        m_global_synth_params.osc4_params.semitones,
        m_global_synth_params.osc4_params.cents,


        m_settings.m_noise_osc_db,
        m_settings.m_octave,
        m_settings.m_pitchbend_range,
        m_settings.m_portamento_time_msec,
        m_settings.m_pulse_width_pct,
        m_settings.m_sub_osc_db,
        m_settings.m_velocity_to_attack_scaling,
        m_settings.m_reset_to_zero,

        // LFO1
        INSTRUMENT_ORANGE,
        s_lfo_wave_names[m_settings.m_lfo1_waveform],
        m_settings.m_lfo1_waveform,
        s_lfo_mode_names[m_settings.m_lfo1_mode],
        m_settings.m_lfo1_mode,
        m_settings.m_lfo1_rate,
        m_settings.m_lfo1_amplitude,
        m_settings.m_lfo1_filter_fc_enabled,
        m_settings.m_lfo1_osc_pitch_enabled,
        m_settings.m_lfo1_pan_enabled,
        m_settings.m_lfo1_amp_enabled,
        m_settings.m_lfo1_pulsewidth_enabled,
        m_settings.m_lfo1_filter_fc_intensity,
        m_settings.m_lfo1_osc_pitch_intensity,
        m_settings.m_lfo1_pan_intensity,
        m_settings.m_lfo1_amp_intensity,
        m_settings.m_lfo1_pulsewidth_intensity,

        // LFO2
        INSTRUMENT_PINK,
        s_lfo_wave_names[m_settings.m_lfo2_waveform],
        m_settings.m_lfo2_waveform,
        s_lfo_mode_names[m_settings.m_lfo2_mode],
        m_settings.m_lfo2_mode,
        m_settings.m_lfo2_rate,
        m_settings.m_lfo2_amplitude,
        m_settings.m_lfo2_filter_fc_enabled,
        m_settings.m_lfo2_osc_pitch_enabled,
        m_settings.m_lfo2_pan_enabled,
        m_settings.m_lfo2_amp_enabled,
        m_settings.m_lfo2_pulsewidth_enabled,
        m_settings.m_lfo2_filter_fc_intensity,
        m_settings.m_lfo2_osc_pitch_intensity,
        m_settings.m_lfo2_pan_intensity,
        m_settings.m_lfo2_amp_intensity,
        m_settings.m_lfo2_pulsewidth_intensity,

        // EG1
        INSTRUMENT_ORANGE,
        m_settings.m_eg1_filter_enabled,
        m_settings.m_eg1_osc_enabled,
        m_settings.m_eg1_dca_enabled,
        m_settings.m_eg1_sustain_override,
        m_settings.m_eg1_filter_intensity,
        m_settings.m_eg1_osc_intensity,
        m_settings.m_eg1_dca_intensity,
        m_settings.m_eg1_sustain_level,
        m_settings.m_eg1_attack_time_msec,
        m_settings.m_eg1_decay_time_msec,
        m_settings.m_eg1_release_time_msec,

        // EG2
        INSTRUMENT_PINK,
        m_settings.m_eg2_filter_enabled,
        m_settings.m_eg2_osc_enabled,
        m_settings.m_eg2_dca_enabled,
        m_settings.m_eg2_sustain_override,
        m_settings.m_eg2_filter_intensity,
        m_settings.m_eg2_osc_intensity,
        m_settings.m_eg2_dca_intensity,
        m_settings.m_eg2_sustain_level,
        m_settings.m_eg2_attack_time_msec,
        m_settings.m_eg2_decay_time_msec,
        m_settings.m_eg2_release_time_msec,


        // FILTER1
        INSTRUMENT_ORANGE,
        s_filter_type_names[m_settings.m_filter_type],
        m_settings.m_filter_type,
        m_settings.m_fc_control,
        m_settings.m_q_control,
        m_settings.m_q_control, //TODO
        m_settings.m_filter_saturation,

        INSTRUMENT_YELLOW
        );
    // clang-format on

    wchar_t scratch[1024] = {};
    sequence_engine_status(&engine, scratch);
    wcscat(status_string, scratch);
}
void minisynth::start()
{
    active = true;
    engine.cur_step = mixr->timing_info.sixteenth_note_tick % 16;
}

void minisynth::stop()
{
    active = false;
    allNotesOff();
}

void minisynth_load_defaults(minisynth *ms)
{
    strncpy(ms->m_settings.m_settings_name, "default", 7);

    ms->m_settings.m_monophonic = false;
    ms->m_settings.m_voice_mode = Sqr3;
    ms->m_settings.m_detune_cents = 0.0;

    // OSC1
    ms->m_settings.osc1_wave = SQUARE;
    ms->m_settings.osc1_amp = 1;
    ms->m_settings.osc1_oct = 1;
    ms->m_settings.osc1_semis = 0;
    ms->m_settings.osc1_cents = 0;
    // OSC2
    ms->m_settings.osc2_wave = SQUARE;
    ms->m_settings.osc2_amp = 1;
    ms->m_settings.osc2_oct = 1;
    ms->m_settings.osc2_semis = 0;
    ms->m_settings.osc2_cents = 0;
    // OSC3
    ms->m_settings.osc3_wave = SQUARE;
    ms->m_settings.osc3_amp = 0;
    ms->m_settings.osc3_oct = 0;
    ms->m_settings.osc3_semis = 0;
    ms->m_settings.osc3_cents = 0;
    // OSC4
    ms->m_settings.osc4_wave = NOISE;
    ms->m_settings.osc4_amp = 0;
    ms->m_settings.osc4_oct = 0;
    ms->m_settings.osc4_semis = 0;
    ms->m_settings.osc4_cents = 0;

    // LFO1
    ms->m_settings.m_lfo1_waveform = 0;
    ms->m_settings.m_lfo1_rate = DEFAULT_LFO_RATE;
    ms->m_settings.m_lfo1_amplitude = 1.0;

    // LFO1 routings
    ms->m_settings.m_lfo1_osc_pitch_intensity = 0.7;
    ms->m_settings.m_lfo1_osc_pitch_enabled = false;

    ms->m_settings.m_lfo1_filter_fc_intensity = 0.7;
    ms->m_settings.m_lfo1_filter_fc_enabled = false;

    ms->m_settings.m_lfo1_amp_intensity = 0.7;
    ms->m_settings.m_lfo1_amp_enabled = false;

    ms->m_settings.m_lfo1_pan_intensity = 0.7;
    ms->m_settings.m_lfo1_pan_enabled = false;

    ms->m_settings.m_lfo1_pulsewidth_intensity = 0.7;
    ms->m_settings.m_lfo1_pulsewidth_enabled = false;

    // LFO2
    ms->m_settings.m_lfo2_waveform = 0;
    ms->m_settings.m_lfo2_rate = DEFAULT_LFO_RATE;
    ms->m_settings.m_lfo2_amplitude = 1.0;

    // LFO2 routings
    ms->m_settings.m_lfo2_osc_pitch_intensity = 0.7;
    ms->m_settings.m_lfo2_osc_pitch_enabled = false;

    ms->m_settings.m_lfo2_filter_fc_intensity = 0.7;
    ms->m_settings.m_lfo2_filter_fc_enabled = false;

    ms->m_settings.m_lfo2_amp_intensity = 0.7;
    ms->m_settings.m_lfo2_amp_enabled = false;

    ms->m_settings.m_lfo2_pan_intensity = 0.7;
    ms->m_settings.m_lfo2_pan_enabled = false;

    ms->m_settings.m_lfo2_pulsewidth_intensity = 0.7;
    ms->m_settings.m_lfo2_pulsewidth_enabled = false;

    ms->m_settings.m_fc_control = FILTER_FC_DEFAULT;
    ms->m_settings.m_q_control = FILTER_Q_DEFAULT;

    /// EG1 //////////////////////////////////////////
    ms->m_settings.m_eg1_osc_intensity = 0.7;
    ms->m_settings.m_eg1_osc_enabled = false;
    ms->m_settings.m_eg1_filter_intensity = 0.7;
    ms->m_settings.m_eg1_filter_enabled = false;
    ms->m_settings.m_eg1_dca_intensity = 1.0;
    ms->m_settings.m_eg1_dca_enabled = true;

    ms->m_settings.m_eg1_attack_time_msec = 300;
    ms->m_settings.m_eg1_decay_time_msec = 300;
    ms->m_settings.m_eg1_release_time_msec = 300;

    ms->m_settings.m_eg1_sustain_level = 0.9;
    ms->m_settings.m_eg1_sustain_override = false;

    /// EG2 //////////////////////////////////////////
    ms->m_settings.m_eg2_osc_intensity = 0.7;
    ms->m_settings.m_eg2_osc_enabled = false;
    ms->m_settings.m_eg2_filter_intensity = 0.7;
    ms->m_settings.m_eg2_filter_enabled = false;
    ms->m_settings.m_eg2_dca_intensity = 1.0;
    ms->m_settings.m_eg2_dca_enabled = false;

    ms->m_settings.m_eg2_attack_time_msec = 52;
    ms->m_settings.m_eg2_decay_time_msec = 30;
    ms->m_settings.m_eg2_release_time_msec = 35;

    ms->m_settings.m_eg2_sustain_level = 0.9;
    ms->m_settings.m_eg2_sustain_override = false;
    ///////////////////////////////////////////////////

    ms->m_settings.m_pulse_width_pct = OSC_PULSEWIDTH_DEFAULT;
    ms->m_settings.m_octave = ms->engine.octave;
    ms->m_settings.m_portamento_time_msec = DEFAULT_PORTAMENTO_TIME_MSEC;
    ms->m_settings.m_sub_osc_db = -96.000000;

    ms->m_settings.m_noise_osc_db = -96.000000;
    ms->m_settings.m_legato_mode = DEFAULT_LEGATO_MODE;
    ms->m_settings.m_pitchbend_range = 1;
    ms->m_settings.m_reset_to_zero = DEFAULT_RESET_TO_ZERO;
    ms->m_settings.m_filter_keytrack = DEFAULT_FILTER_KEYTRACK;
    ms->m_settings.m_filter_type = FILTER_TYPE_DEFAULT;
    ms->m_settings.m_filter_keytrack_intensity =
        DEFAULT_FILTER_KEYTRACK_INTENSITY;

    ms->m_settings.m_velocity_to_attack_scaling = 0;
    ms->m_settings.m_note_number_to_decay_scaling = 0;

    ms->m_settings.m_generate_active = false;
    ms->m_settings.m_generate_src = -99;
}

void minisynth::control(midi_event ev)
{
    double scaley_val = 0;
    switch (ev.data1)
    {
    case (1):
        scaley_val = scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, ev.data2);
        m_settings.m_eg1_attack_time_msec = scaley_val;
        break;
    case (2):
        scaley_val = scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, ev.data2);
        m_settings.m_eg1_decay_time_msec = scaley_val;
        break;
    case (3):
        scaley_val = scaleybum(0, 127, 0, 1, ev.data2);
        m_settings.m_eg1_sustain_level = scaley_val;
        break;
    case (4):
        scaley_val = scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, ev.data2);
        m_settings.m_eg1_release_time_msec = scaley_val;
        break;
    case (5):
        // printf("LFO rate\n");
        scaley_val = scaleybum(0, 128, MIN_LFO_RATE, MAX_LFO_RATE, ev.data2);
        m_settings.m_lfo1_rate = scaley_val;
        break;
    case (6):
        // printf("LFO amp\n");
        scaley_val = scaleybum(0, 128, 0.0, 1.0, ev.data2);
        m_settings.m_lfo1_amplitude = scaley_val;
        break;
    case (7):
        // printf("Filter CutOff\n");
        scaley_val = scaleybum(0, 127, FILTER_FC_MIN, FILTER_FC_MAX, ev.data2);
        m_settings.m_fc_control = scaley_val;
        break;
    case (8):
        // printf("Filter Q\n");
        scaley_val = scaleybum(0, 127, 0.02, 10, ev.data2);
        m_settings.m_q_control = scaley_val;
        break;
    default:
        printf("nah\n");
    }
    minisynth_update(this);
}

void minisynth::noteOn(midi_event ev)
{
    unsigned int midinote = ev.data1;
    unsigned int velocity = ev.data2;

    if (m_settings.m_monophonic)
    {
        minisynth_voice *msv = m_voices[0];
        voice_note_on(&msv->m_voice, midinote, velocity,
                      get_midi_freq(midinote), m_last_note_frequency);
        m_last_note_frequency = get_midi_freq(midinote);
        return;
    }

    bool steal_note = true;
    for (int i = 0; i < MAX_VOICES; i++)
    {
        minisynth_voice *msv = m_voices[i];
        if (!msv)
            return; // should never happen
        if (!msv->m_voice.m_note_on)
        {
            minisynth_increment_voice_timestamps(this);
            voice_note_on(&msv->m_voice, midinote, velocity,
                          get_midi_freq(midinote), m_last_note_frequency);

            m_last_note_frequency = get_midi_freq(midinote);
            steal_note = false;
            break;
        }
    }

    if (steal_note)
    {
        if (mixr->debug_mode)
            printf("STEAL NOTE\n");
        minisynth_voice *msv = minisynth_get_oldest_voice(this);
        if (msv)
        {
            minisynth_increment_voice_timestamps(this);
            voice_note_on(&msv->m_voice, midinote, velocity,
                          get_midi_freq(midinote), m_last_note_frequency);
        }
        m_last_note_frequency = get_midi_freq(midinote);
    }
}

void minisynth::allNotesOff()
{
    for (int i = 0; i < MAX_VOICES; i++)
    {
        if (m_voices[i])
            voice_note_off(&m_voices[i]->m_voice, -1);
    }
}

void minisynth::noteOff(midi_event ev)
{
    for (int i = 0; i < MAX_VOICES; i++)
    {
        minisynth_voice *msv =
            minisynth_get_oldest_voice_with_note(this, ev.data1);
        if (msv)
        {
            voice_note_off(&msv->m_voice, ev.data1);
        }
    }
}

void minisynth::pitchBend(midi_event ev)
{
    unsigned int data1 = ev.data1;
    unsigned int data2 = ev.data2;
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
            m_voices[i]->m_voice.m_osc1->m_cents = scaley_val;
            m_voices[i]->m_voice.m_osc2->m_cents = scaley_val + 2.5;
            m_voices[i]->m_voice.m_osc3->m_cents = scaley_val;
            m_voices[i]->m_voice.m_osc4->m_cents = scaley_val + 2.5;
            m_voices[i]->m_voice.m_v_modmatrix.m_sources[SOURCE_PITCHBEND] =
                normalized_pitch_bent_val;
        }
    }
    else
    {
        for (int i = 0; i < MAX_VOICES; i++)
        {
            m_voices[i]->m_voice.m_osc1->m_cents = 0;
            m_voices[i]->m_voice.m_osc2->m_cents = 2.5;
            m_voices[i]->m_voice.m_osc3->m_cents = 0;
            m_voices[i]->m_voice.m_osc4->m_cents = 2.5;
        }
    }
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
    ms->m_global_synth_params.voice_params.hard_sync = ms->m_settings.hard_sync;
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
    ms->m_global_synth_params.osc1_params.amplitude = ms->m_settings.osc1_amp;
    ms->m_global_synth_params.osc1_params.octave = ms->m_settings.osc1_oct;
    ms->m_global_synth_params.osc1_params.semitones = ms->m_settings.osc1_semis;
    ms->m_global_synth_params.osc1_params.cents = ms->m_settings.osc1_cents;

    ms->m_global_synth_params.osc2_params.amplitude = ms->m_settings.osc2_amp;
    ms->m_global_synth_params.osc2_params.octave = ms->m_settings.osc2_oct;
    ms->m_global_synth_params.osc2_params.semitones = ms->m_settings.osc2_semis;
    ms->m_global_synth_params.osc2_params.cents = ms->m_settings.osc2_cents;

    double noise_amplitude =
        ms->m_settings.m_noise_osc_db == -96.0
            ? 0.0
            : pow(10.0, ms->m_settings.m_noise_osc_db / 20.0);
    double sub_amplitude = ms->m_settings.m_sub_osc_db == -96.0
                               ? 0.0
                               : pow(10.0, ms->m_settings.m_sub_osc_db / 20.0);

    // --- osc3 is sub osc
    ms->m_global_synth_params.osc3_params.amplitude = sub_amplitude;
    ms->m_global_synth_params.osc3_params.octave = ms->m_settings.osc3_oct;
    ms->m_global_synth_params.osc3_params.semitones = ms->m_settings.osc3_semis;
    ms->m_global_synth_params.osc3_params.cents = ms->m_settings.osc3_cents;

    // --- osc4 is noise osc
    ms->m_global_synth_params.osc4_params.amplitude = noise_amplitude;
    ms->m_global_synth_params.osc4_params.octave = ms->m_settings.osc4_oct;
    ms->m_global_synth_params.osc4_params.semitones = ms->m_settings.osc4_semis;
    ms->m_global_synth_params.osc4_params.cents = ms->m_settings.osc4_cents;

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
        ms->m_settings.m_eg1_attack_time_msec;
    ms->m_global_synth_params.eg1_params.decay_time_msec =
        ms->m_settings.m_eg1_decay_time_msec;
    ms->m_global_synth_params.eg1_params.sustain_level =
        ms->m_settings.m_eg1_sustain_level;
    ms->m_global_synth_params.eg1_params.release_time_msec =
        ms->m_settings.m_eg1_release_time_msec;
    ms->m_global_synth_params.eg1_params.reset_to_zero =
        (bool)ms->m_settings.m_reset_to_zero;
    ms->m_global_synth_params.eg1_params.legato_mode =
        (bool)ms->m_settings.m_legato_mode;
    ms->m_global_synth_params.eg1_params.sustain_override =
        (bool)ms->m_settings.m_eg1_sustain_override;

    // --- dca:
    ms->m_global_synth_params.dca_params.amplitude_db = ms->volume;

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

void minisynth_print_lfo1_routing_info(minisynth *ms, wchar_t *scratch)
{
    print_modulation_matrix_info_lfo1(&ms->m_ms_modmatrix, scratch);
}

void minisynth_print_eg1_routing_info(minisynth *ms, wchar_t *scratch)
{
    print_modulation_matrix_info_eg1(&ms->m_ms_modmatrix, scratch);
}

void minisynth::randomize()
{
    // printf("Randomizing SYNTH!\n");

    strncpy(m_settings.m_settings_name, "-- random UNSAVED--", 256);
    m_settings.m_voice_mode = rand() % MAX_VOICE_CHOICE;
    m_settings.m_monophonic = rand() % 2;

    m_settings.m_lfo1_waveform = rand() % MAX_LFO_OSC;
    m_settings.m_lfo1_rate =
        ((float)rand()) / RAND_MAX * (MAX_LFO_RATE - MIN_LFO_RATE) +
        MIN_LFO_RATE;
    m_settings.m_lfo1_amplitude = ((float)rand()) / RAND_MAX;
    m_settings.m_lfo1_osc_pitch_intensity =
        (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    m_settings.m_lfo1_osc_pitch_enabled = rand() % 2;
    m_settings.m_lfo1_filter_fc_intensity =
        (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    m_settings.m_lfo1_filter_fc_enabled = rand() % 2;
    m_settings.m_filter_type = rand() % NUM_FILTER_TYPES;
    m_settings.m_lfo1_amp_intensity = ((float)rand() / (float)(RAND_MAX));
    // m_settings.m_lfo1_amp_enabled = rand() % 2;
    // m_settings.m_lfo1_pan_intensity = ((float)rand() /
    //(float)(RAND_MAX));
    // m_settings.m_lfo1_pan_enabled = rand() % 2;
    m_settings.m_lfo1_pulsewidth_intensity =
        ((float)rand() / (float)(RAND_MAX));
    m_settings.m_lfo1_pulsewidth_enabled = rand() % 2;

    m_settings.m_lfo2_waveform = rand() % MAX_LFO_OSC;
    m_settings.m_lfo2_rate =
        ((float)rand()) / RAND_MAX * (MAX_LFO_RATE - MIN_LFO_RATE) +
        MIN_LFO_RATE;
    m_settings.m_lfo2_amplitude = ((float)rand()) / RAND_MAX;
    m_settings.m_lfo2_osc_pitch_intensity =
        (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    m_settings.m_lfo2_osc_pitch_enabled = rand() % 2;
    m_settings.m_lfo2_filter_fc_intensity =
        (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    m_settings.m_lfo2_filter_fc_enabled = rand() % 2;
    m_settings.m_lfo2_amp_intensity = ((float)rand() / (float)(RAND_MAX));
    // m_settings.m_lfo2_amp_enabled = rand() % 2;
    // m_settings.m_lfo2_pan_intensity = ((float)rand() /
    //(float)(RAND_MAX));
    // m_settings.m_lfo2_pan_enabled = rand() % 2;
    m_settings.m_lfo2_pulsewidth_intensity =
        ((float)rand() / (float)(RAND_MAX));
    m_settings.m_lfo2_pulsewidth_enabled = rand() % 2;

    m_settings.m_detune_cents = (rand() % 200) - 100;

    m_settings.m_fc_control =
        ((float)rand()) / RAND_MAX * (FILTER_FC_MAX - FILTER_FC_MIN) +
        FILTER_FC_MIN;
    m_settings.m_q_control = (rand() % 8) + 1;

    m_settings.m_eg1_attack_time_msec = (rand() % 700) + 5;
    m_settings.m_eg1_decay_time_msec = (rand() % 700) + 5;
    m_settings.m_eg1_release_time_msec = (rand() % 600) + 5;
    m_settings.m_pulse_width_pct = (rand() % 99) + 1;

    // m_settings.m_sustain_level = ((float)rand()) / RAND_MAX;
    m_settings.m_octave = rand() % 3 + 1;

    m_settings.m_portamento_time_msec = rand() % 5000;

    m_settings.m_sub_osc_db = -1.0 * (rand() % 96);
    // m_settings.m_eg1_osc_intensity =
    //    (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    m_settings.m_eg1_filter_intensity =
        (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
    m_settings.m_noise_osc_db = -1.0 * (rand() % 96);

    //////// m_settings.m_volume_db = 1.0;
    m_settings.m_legato_mode = rand() % 2;
    // m_settings.m_pitchbend_range = rand() % 12;
    m_settings.m_reset_to_zero = rand() % 2;
    m_settings.m_filter_keytrack = rand() % 2;
    m_settings.m_filter_keytrack_intensity =
        (((float)rand() / (float)(RAND_MAX)) * 9) + 0.51;
    m_settings.m_velocity_to_attack_scaling = rand() % 2;
    m_settings.m_note_number_to_decay_scaling = rand() % 2;
    ////m_settings.m_eg1_dca_intensity =
    ////    (((float)rand() / (float)(RAND_MAX)) * 2.0) - 1;
    // m_settings.m_sustain_override = rand() % 2;

    minisynth_update(this);

    // minisynth_print_settings(ms);
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
           MOOG_PRESET_FILENAME);
    FILE *presetzzz = fopen(MOOG_PRESET_FILENAME, "a+");
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
            ms->m_settings.m_eg1_attack_time_msec);
    settings_count++;
    fprintf(presetzzz, "::decay_time_msec=%f",
            ms->m_settings.m_eg1_decay_time_msec);
    settings_count++;
    fprintf(presetzzz, "::release_time_msec=%f",
            ms->m_settings.m_eg1_release_time_msec);
    settings_count++;
    fprintf(presetzzz, "::sustain_level=%f",
            ms->m_settings.m_eg1_sustain_level);
    settings_count++;

    fprintf(presetzzz, "::volume_db=%f", ms->volume);
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
            ms->m_settings.m_eg1_sustain_override);
    settings_count++;

    fprintf(presetzzz, ":::\n");
    fclose(presetzzz);
    printf("Wrote %d settings\n", settings_count++);
    return true;
}

bool minisynth_check_if_preset_exists(char *preset_to_find)
{
    FILE *presetzzz = fopen(MOOG_PRESET_FILENAME, "r+");
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

    FILE *presetzzz = fopen(MOOG_PRESET_FILENAME, "r+");
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
                ms->m_settings.m_eg1_attack_time_msec = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "decay_time_msec") == 0)
            {
                ms->m_settings.m_eg1_decay_time_msec = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "release_time_msec") == 0)
            {
                ms->m_settings.m_eg1_release_time_msec = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "sustain_level") == 0)
            {
                ms->m_settings.m_eg1_sustain_level = scratch_val;
                settings_count++;
            }
            else if (strcmp(setting_key, "volume_db") == 0)
            {
                ms->volume = scratch_val;
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
                ms->m_settings.m_eg1_sustain_override = scratch_val;
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
    printf("Volume (vol): %f [0-1]\n", ms->volume);
    printf("Reset To Zero (zero): %d [0,1]\n",
           ms->m_settings.m_reset_to_zero); // unsigned

    printf(COOL_COLOR_PINK); // EG
    printf("EG1 Attack time ms (attackms): %f [%d-%d]\n",
           ms->m_settings.m_eg1_attack_time_msec, EG_MINTIME_MS, EG_MAXTIME_MS);
    printf("EG1 Decay Time ms (decayms): %f [%d-%d]\n",
           ms->m_settings.m_eg1_decay_time_msec, EG_MINTIME_MS, EG_MAXTIME_MS);
    printf("EG1 Release Time ms (releasems): %f [%d-%d]\n",
           ms->m_settings.m_eg1_release_time_msec, EG_MINTIME_MS,
           EG_MAXTIME_MS);
    printf("EG1 Sustain Level (sustainlvl): %f [0-1]\n",
           ms->m_settings.m_eg1_sustain_level);
    printf("EG1 Sustain Override (sustain): %d [0,1]\n",
           ms->m_settings.m_eg1_sustain_override); // bool
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

    printf(ANSI_COLOR_RESET);
}

void minisynth_print_patterns(minisynth *ms)
{
    sequence_engine_print_patterns(&ms->engine);
}

void minisynth_print_modulation_routings(minisynth *ms)
{
    print_modulation_matrix(&ms->m_ms_modmatrix);
}

void minisynth_set_generate_src(minisynth *ms, int src)
{
    if (mixer_is_valid_pattern_gen_num(mixr, src))
        ms->m_settings.m_generate_src = src;
}

void minisynth_set_filter_mod(minisynth *ms, double mod)
{
    for (int i = 0; i < MAX_VOICES; i++)
    {
        minisynth_voice_set_filter_mod(ms->m_voices[i], mod);
    }
}

void minisynth_print(minisynth *ms) { minisynth_print_settings(ms); }

void minisynth_set_eg_attack_time_ms(minisynth *ms, unsigned int eg_num,
                                     double val)
{
    if (val >= EG_MINTIME_MS && val <= EG_MAXTIME_MS)
    {
        if (eg_num == 1)
            ms->m_settings.m_eg1_attack_time_msec = val;
        else if (eg_num == 2)
            ms->m_settings.m_eg2_attack_time_msec = val;
    }
    else
        printf("val must be between %d and %d\n", EG_MINTIME_MS, EG_MAXTIME_MS);
}

void minisynth_set_eg_decay_time_ms(minisynth *ms, unsigned int eg_num,
                                    double val)
{
    if (val >= EG_MINTIME_MS && val <= EG_MAXTIME_MS)
    {
        if (eg_num == 1)
            ms->m_settings.m_eg1_decay_time_msec = val;
        else if (eg_num == 2)
            ms->m_settings.m_eg2_decay_time_msec = val;
    }
    else
        printf("val must be between %d and %d\n", EG_MINTIME_MS, EG_MAXTIME_MS);
}

void minisynth_set_eg_release_time_ms(minisynth *ms, unsigned int eg_num,
                                      double val)
{
    if (val >= EG_MINTIME_MS && val <= EG_MAXTIME_MS)
    {
        if (eg_num == 1)
            ms->m_settings.m_eg1_release_time_msec = val;
        else if (eg_num == 2)
            ms->m_settings.m_eg2_release_time_msec = val;
    }
    else
        printf("val must be between %d and %d\n", EG_MINTIME_MS, EG_MAXTIME_MS);
}

void minisynth_set_osc_amp(minisynth *ms, unsigned int osc_num, double val)
{
    if (osc_num == 0 || osc_num > 4)
        return;

    if (val >= -1 && val <= 1)
    {
        switch (osc_num)
        {
        case (1):
            ms->m_settings.osc1_amp = val;
            break;
        case (2):
            ms->m_settings.osc2_amp = val;
            break;
        case (3):
            ms->m_settings.osc3_amp = val;
            break;
        case (4):
            ms->m_settings.osc4_amp = val;
            break;
        }
    }
    else
        printf("val must be between -100 and 100\n");
}

void minisynth_set_osc_cents(minisynth *ms, unsigned int osc_num, double val)
{
    if (osc_num == 0 || osc_num > 4)
        return;

    if (val >= -100 && val <= 100)
    {
        switch (osc_num)
        {
        case (1):
            ms->m_settings.osc1_cents = val;
            break;
        case (2):
            ms->m_settings.osc2_cents = val;
            break;
        case (3):
            ms->m_settings.osc3_cents = val;
            break;
        case (4):
            ms->m_settings.osc4_cents = val;
            break;
        }
    }
    else
        printf("val must be between -100 and 100\n");
}
void minisynth_set_detune(minisynth *ms, double val)
{
    if (val >= -100 && val <= 100)
        ms->m_settings.m_detune_cents = val;
    else
        printf("val must be between -100 and 100\n");
}

void minisynth_set_eg_dca_enable(minisynth *ms, unsigned int osc_num, int val)
{
    if (val == 0 || val == 1)
    {
        if (osc_num == 1)
            ms->m_settings.m_eg1_dca_enabled = val;
        else if (osc_num == 2)
            ms->m_settings.m_eg2_dca_enabled = val;
    }
    else
        printf("val must be boolean 0 or 1\n");
}

void minisynth_set_eg_dca_int(minisynth *ms, unsigned int eg_num, double val)
{
    if (val >= -1 && val <= 1)
    {
        if (eg_num == 1)
            ms->m_settings.m_eg1_dca_intensity = val;
        else if (eg_num == 2)
            ms->m_settings.m_eg2_dca_intensity = val;
    }
    else
        printf("val must be between -1 and 1\n");
}

void minisynth_set_eg_filter_enable(minisynth *ms, unsigned int eg_num, int val)
{
    if (val == 0 || val == 1)
    {
        if (eg_num == 1)
            ms->m_settings.m_eg1_filter_enabled = val;
        else if (eg_num == 2)
            ms->m_settings.m_eg2_filter_enabled = val;
    }
    else
        printf("val must be boolean 0 or 1\n");
}

void minisynth_set_eg_filter_int(minisynth *ms, unsigned int eg_num, double val)
{
    if (val >= -1 && val <= 1)
    {
        if (eg_num == 1)
            ms->m_settings.m_eg1_filter_intensity = val;
        else if (eg_num == 2)
            ms->m_settings.m_eg2_filter_intensity = val;
    }
    else
        printf("val must be between -1 and 1\n");
}

void minisynth_set_eg_osc_enable(minisynth *ms, unsigned int eg_num, int val)
{
    if (val == 0 || val == 1)
    {
        if (eg_num == 1)
            ms->m_settings.m_eg1_osc_enabled = val;
        else if (eg_num == 2)
            ms->m_settings.m_eg2_osc_enabled = val;
    }
    else
        printf("val must be boolean 0 or 1\n");
}

void minisynth_set_eg_osc_int(minisynth *ms, unsigned int eg_num, double val)
{
    if (val >= -1 && val <= 1)
    {
        if (eg_num == 1)
            ms->m_settings.m_eg1_osc_intensity = val;
        else if (eg_num == 2)
            ms->m_settings.m_eg2_osc_intensity = val;
    }
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
    if (val == BSF2 || val == LPF1 || val == HPF1)
        printf("warning! useless change - %d not possible with moog\n", val);
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

void minisynth_set_lfo_pulsewidth_enable(minisynth *ms, int lfo_num,
                                         unsigned int val)
{
    if (val == 0 || val == 1)
    {
        switch (lfo_num)
        {
        case (1):
            ms->m_settings.m_lfo1_pulsewidth_enabled = val;
            break;
        case (2):
            ms->m_settings.m_lfo2_pulsewidth_enabled = val;
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

void minisynth_set_lfo_pulsewidth_int(minisynth *ms, int lfo_num, double val)
{
    if (val >= -1 && val <= 1)
    {
        switch (lfo_num)
        {
        case (1):
            ms->m_settings.m_lfo1_pulsewidth_intensity = val;
            break;
        case (2):
            ms->m_settings.m_lfo2_pulsewidth_intensity = val;
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

void minisynth_set_eg_sustain(minisynth *ms, unsigned int eg_num, double val)
{
    if (val >= 0 && val <= 1)
    {
        if (eg_num == 1)
            ms->m_settings.m_eg1_sustain_level = val;
        else if (eg_num == 2)
            ms->m_settings.m_eg2_sustain_level = val;
    }
    else
        printf("val must be between 0 and 1\n");
}

void minisynth_set_eg_sustain_override(minisynth *ms, unsigned int eg_num,
                                       bool b)
{
    if (eg_num == 1)
        ms->m_settings.m_eg1_sustain_override = b;
    else if (eg_num == 2)
        ms->m_settings.m_eg2_sustain_override = b;
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

void minisynth_set_reset_to_zero(minisynth *ms, unsigned int val)
{
    if (val != 0 && val != 1)
    {
        printf("Val must be zero or one\n");
        return;
    }
    ms->m_settings.m_reset_to_zero = val;
}

void minisynth_set_monophonic(minisynth *ms, bool b)
{
    ms->m_settings.m_monophonic = b;
}
void minisynth_set_generate(minisynth *ms, bool b)
{
    ms->m_settings.m_generate_active = b;
}

void minisynth_set_osc_type(minisynth *ms, int osc, unsigned int osc_type)
{
    if (osc > 0 && osc < 4 && osc_type < MAX_OSC)
    {
        printf("Setting OSC %d to %s(%d)\n", osc, s_waveform_names[osc_type],
               osc_type);
        switch (osc)
        {
        case (1):
            ms->m_global_synth_params.osc1_params.waveform = osc_type;
            break;
        case (2):
            ms->m_global_synth_params.osc2_params.waveform = osc_type;
            break;
        case (3):
            ms->m_global_synth_params.osc3_params.waveform = osc_type;
            break;
        case (4):
            ms->m_global_synth_params.osc4_params.waveform = osc_type;
            break;
        }
    }
}

void minisynth_set_hard_sync(minisynth *ms, bool val)
{
    // TODO - add to export / load functions
    ms->m_settings.hard_sync = val;
}

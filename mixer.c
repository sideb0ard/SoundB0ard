#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#include <portaudio.h>

#include "algorithm.h"
#include "ableton_link_wrapper.h"
#include "bitshift.h"
#include "chaosmonkey.h"
#include "defjams.h"
#include "digisynth.h"
#include "dxsynth.h"
#include "envelope.h"
#include "euclidean.h"
#include "fx.h"
#include "granulator.h"
#include "looper.h"
#include "metronome.h"
#include "minisynth.h"
#include "mixer.h"
#include "sample_sequencer.h"
#include "sbmsg.h"
#include "sequencer_utils.h"
#include "sound_generator.h"
#include "spork.h"
#include "synthdrum_sequencer.h"
#include "utils.h"

extern ENVSTREAM *ampstream;

extern mixer *mixr;

extern const char *key_names[NUM_KEYS];
extern const int key_midi_mapping[NUM_KEYS];
extern const compat_key_list compat_keys[NUM_KEYS];

const wchar_t *s_status_colors[] = {
    WCOOL_COLOR_PINK,      // MINISYNTH_TYPE
    WCOOL_COLOR_ORANGE,    // DIGISYNTH_TYPE
    WCOOL_COLOR_MAUVE,     // LOOPER_TYPE
    WCOOL_COLOR_YELLOW,    // BITWIZE_TYPE
    WANSI_COLOR_DEEP_RED,  // GRANULATOR_TYPE
    WANSI_COLOR_GREEN_TOO, // SEQUENCER_TYPE
    WANSI_COLOR_MAGENTA,   // SYNTHDRUM_TYPE
    WANSI_COLOR_CYAN,      // ALGORITHM_TYPE
    WANSI_COLOR_GREEN,     // CHAOSMONKEY_TYPE
    WANSI_COLOR_BLUE       // SPORK_TYPE
};

const char *s_midi_control_type_name[] = {"NONE", "SYNTH"};

const double micros_per_sample = 1e6 / SAMPLE_RATE;
const double midi_tick_len_as_percent = 1.0 / PPQN;

mixer *new_mixer(double output_latency)
{
    mixer *mixr = (mixer *)calloc(1, sizeof(mixer));
    if (mixr == NULL)
    {
        printf("Nae mixer, fucked up!\n");
        return NULL;
    }
    mixr->m_ableton_link = new_ableton_link(DEFAULT_BPM);
    if (!mixr->m_ableton_link)
    {
        printf("Something fucked up with yer Ableton link, mate."
                " ye wanna get that seen tae\n");
        return NULL;
    }
    link_set_latency(mixr->m_ableton_link, output_latency);

    mixr->volume = 0.7;
    mixer_update_bpm(mixr, DEFAULT_BPM);
    mixr->m_midi_controller_mode =
        KEY_MODE_ONE; // dunno whether this should be on mixer or synth
    mixr->midi_control_destination = NONE;

    // the lifetime of these booleans is a single sample

    mixr->timing_info.midi_tick = -1;
    mixr->timing_info.time_of_last_midi_tick = 0;
    mixr->timing_info.is_midi_tick = true;
    mixr->timing_info.start_of_loop = true;
    mixr->timing_info.is_thirtysecond = true;
    mixr->timing_info.is_sixteenth = true;
    mixr->timing_info.is_eighth = true;
    mixr->timing_info.is_quarter = true;

    mixr->scene_mode = false;
    mixr->scene_start_pending = false;
    mixr->scenes[0].num_bars_to_play = 4;
    mixr->num_scenes = 1;
    mixr->current_scene = -1;

    mixr->active_midi_soundgen_num = -99;

    mixr->key = C_MAJOR;

    return mixr;
}

void mixer_ps(mixer *mixr)
{
    LinkData data = link_get_timing_data_for_display(mixr->m_ableton_link);
    printf(COOL_COLOR_GREEN
           "::::: [" ANSI_COLOR_WHITE "MIXING dESK" COOL_COLOR_GREEN
           "] Volume:" ANSI_COLOR_WHITE "%.2f" COOL_COLOR_GREEN
           "] Key:" ANSI_COLOR_WHITE "%s" COOL_COLOR_GREEN
           " // BPM:" ANSI_COLOR_WHITE "%.2f" COOL_COLOR_GREEN
           " // TICK:" ANSI_COLOR_WHITE "%d" COOL_COLOR_GREEN
           " // Qtick:" ANSI_COLOR_WHITE "%d" COOL_COLOR_GREEN
           " // Debug:" ANSI_COLOR_WHITE "%s" COOL_COLOR_GREEN " :::::\n"
           "::::: MIDI Controller:%s MidiReceiverSG:%d MidiType:%s\n"
           "::::: PPQN:%d PPSIXTEENTH:%d PPTWENTYFOURTH:%d PPBAR:%d PPNS:%d \n"
           "::::: LINK::Quantum:%.2f Tempo:%.2f Beat:%.2f Phase:%.2f NumPeers:%d" ANSI_COLOR_RESET,
           mixr->volume, key_names[mixr->key], mixr->bpm,
           mixr->timing_info.midi_tick, mixr->timing_info.sixteenth_note_tick,
           mixr->debug_mode ? "true" : "false",
           mixr->have_midi_controller ? mixr->midi_controller_name : "NONE",
           mixr->active_midi_soundgen_num,
           mixr->active_midi_soundgen_num == -99
               ? "NONE"
               : s_midi_control_type_name
                     [mixr->sound_generators[mixr->active_midi_soundgen_num]
                          ->type],
           PPQN, PPSIXTEENTH, PPTWENTYFOURTH, PPBAR, PPNS,
           data.quantum, data.tempo, data.beat, data.phase, data.num_peers);

    if (mixr->env_var_count > 0)
    {
        printf(COOL_COLOR_GREEN "::::: Environment :::::\n");
        for (int i = 0; i < mixr->env_var_count; i++)
        {
            printf("%s - %d\n", mixr->environment[i].key,
                   mixr->environment[i].val);
        }
        printf(ANSI_COLOR_RESET);
    }
    printf("\n");

    if (mixr->num_scenes > 0)
    {
        printf(COOL_COLOR_GREEN "::::: [" ANSI_COLOR_WHITE
                                "scenes" COOL_COLOR_GREEN "] .....]\n");
        for (int i = 0; i < mixr->num_scenes; i++)
        {
            printf("::::: [%d] - %d bars - ", i,
                   mixr->scenes[i].num_bars_to_play);
            for (int j = 0; j < mixr->scenes[i].num_tracks; j++)
            {
                if (mixr->scenes[i].soundgen_tracks[j].soundgen_num != -1)
                {
                    printf(
                        "(%d,%d)",
                        mixr->scenes[i].soundgen_tracks[j].soundgen_num,
                        mixr->scenes[i].soundgen_tracks[j].soundgen_track_num);
                }
            }
            printf("\n");
        }
        printf(ANSI_COLOR_RESET "\n");
    }

    for (int i = 0; i < mixr->sequence_gen_num; i++)
    {
        if (mixr->sequence_generators[i] != NULL)
        {
            wchar_t wss[MAX_PS_STRING_SZ];
            memset(wss, 0, MAX_PS_STRING_SZ);
            mixr->sequence_generators[i]->status(mixr->sequence_generators[i],
                                                 wss);
            wprintf(WANSI_COLOR_WHITE "[%2d]" WANSI_COLOR_RESET, i);
            wprintf(L"  %ls\n", wss);
            wprintf(WANSI_COLOR_RESET);
        }
    }

    for (int i = 0; i < mixr->soundgen_num; i++)
    {
        if (mixr->sound_generators[i] != NULL)
        {
            wchar_t wss[MAX_PS_STRING_SZ];
            memset(wss, 0, MAX_PS_STRING_SZ);
            mixr->sound_generators[i]->status(mixr->sound_generators[i], wss);

            wprintf(WANSI_COLOR_WHITE "[%2d]" WANSI_COLOR_RESET, i);

            if (mixr->sound_generators[i]->active)
            {
                wprintf(s_status_colors[mixr->sound_generators[i]->type]);
            }
            wprintf(L"  %ls\n", wss);
            wprintf(WANSI_COLOR_RESET);

            if (mixr->sound_generators[i]->effects_num > 0 ||
                mixr->sound_generators[i]->envelopes_num > 0)
            {
                printf("      ");
                for (int j = 0; j < mixr->sound_generators[i]->effects_num; j++)
                {
                    fx *f = mixr->sound_generators[i]->effects[j];
                    if (f->enabled)
                        printf(COOL_COLOR_YELLOW);
                    else
                        printf(ANSI_COLOR_RESET);
                    char fx_status[512];
                    f->status(f, fx_status);
                    printf("\n      [fx %d:%d %s]", i, j, fx_status);
                }
                printf(ANSI_COLOR_RESET);
                printf(COOL_COLOR_GREEN);
                for (int j = 0; j < mixr->sound_generators[i]->envelopes_num;
                     j++)
                {
                    printf("[envelope]\n");
                }
                printf(ANSI_COLOR_RESET);
            }
            printf("\n\n");
        }
    }

    printf(ANSI_COLOR_RESET);
}

void mixer_print_compat_keys(mixer *mixr)
{
    printf("Current KEY is %s. Compats are ", key_names[mixr->key]);
    for (int i = 0; i < 6; ++i)
    {
        printf("%s ", key_names[compat_keys[mixr->key][i]]);
    }
    printf("\n");
}

void mixer_emit_event(mixer *mixr, unsigned int event_type)
{
    for (int i = 0; i < mixr->soundgen_num; ++i)
    {
        soundgenerator *sg = mixr->sound_generators[i];
        if (sg != NULL)
            sg->event_notify(sg, event_type);
    }
    for (int i = 0; i < mixr->sequence_gen_num; ++i)
    {
        sequence_generator *sg = mixr->sequence_generators[i];
        if (sg != NULL)
            sg->event_notify(sg, event_type);
    }
}

void mixer_update_bpm(mixer *mixr, int bpm)
{
    printf("Changing bpm to %d\n", bpm);

    mixr->bpm = bpm;
    mixr->timing_info.frames_per_midi_tick = (60.0 / bpm * SAMPLE_RATE) / PPQN;
    mixr->timing_info.loop_len_in_frames =
        mixr->timing_info.frames_per_midi_tick * PPBAR;
    mixr->timing_info.loop_len_in_ticks = PPBAR;

    mixr->timing_info.size_of_thirtysecond_note =
        (PPSIXTEENTH / 2) * mixr->timing_info.frames_per_midi_tick;
    mixr->timing_info.size_of_sixteenth_note =
        mixr->timing_info.size_of_thirtysecond_note * 2;
    mixr->timing_info.size_of_eighth_note =
        mixr->timing_info.size_of_sixteenth_note * 2;
    mixr->timing_info.size_of_quarter_note =
        mixr->timing_info.size_of_eighth_note * 2;

    mixr->timing_info.sixteenth_note_tick = -1;
    mixr->timing_info.midi_tick = -1;
    mixr->timing_info.cur_sample = 0;

    for (int i = 0; i < mixr->soundgen_num; i++)
    {
        if (mixr->sound_generators[i] != NULL)
        {
            for (int j = 0; j < mixr->sound_generators[i]->envelopes_num; j++)
            {
                update_envelope_stream_bpm(
                    mixr->sound_generators[i]->envelopes[j]);
            }
            if (mixr->sound_generators[i]->type == LOOPER_TYPE)
            {
                looper_resample_to_loop_size(
                    (looper *)mixr->sound_generators[i]);
            }
        }
    }
    link_set_bpm(mixr->m_ableton_link, bpm);
}

void mixer_vol_change(mixer *mixr, float vol)
{
    printf("Changing volume to %f\n", vol);
    if (vol >= 0.0 && vol <= 1.0)
    {
        mixr->volume = vol;
    }
}

void vol_change(mixer *mixr, int sg, float vol)
{
    printf("SG: %d // soungen_num : %d\n", sg, mixr->soundgen_num);
    if (!mixer_is_valid_soundgen_num(mixr, sg))
    {
        printf("Nah mate, returning\n");
        return;
    }
    mixr->sound_generators[sg]->setvol(mixr->sound_generators[sg], vol);
}

int add_sound_generator(mixer *mixr, soundgenerator *sg)
{
    soundgenerator **new_soundgens = NULL;

    if (mixr->soundgen_size <= mixr->soundgen_num)
    {
        if (mixr->soundgen_size == 0)
        {
            mixr->soundgen_size = DEFAULT_ARRAY_SIZE;
        }
        else
        {
            mixr->soundgen_size *= 2;
        }

        new_soundgens = (soundgenerator **)realloc(
            mixr->sound_generators,
            mixr->soundgen_size * sizeof(soundgenerator *));
        if (new_soundgens == NULL)
        {
            printf("Ooh, burney - cannae allocate memory for new sounds");
            return -1;
        }
        else
        {
            mixr->sound_generators = new_soundgens;
        }
    }
    mixr->sound_generators[mixr->soundgen_num] = sg;
    return mixr->soundgen_num++;
}

int add_sequence_generator(mixer *mixr, sequence_generator *sg)
{
    sequence_generator **new_sequence_gens = NULL;

    if (mixr->sequence_gen_size <= mixr->sequence_gen_num)
    {
        if (mixr->sequence_gen_size == 0)
        {
            mixr->sequence_gen_size = DEFAULT_ARRAY_SIZE;
        }
        else
        {
            mixr->sequence_gen_size *= 2;
        }

        new_sequence_gens = (sequence_generator **)realloc(
            mixr->sequence_generators,
            mixr->sequence_gen_size * sizeof(sequence_generator *));
        if (new_sequence_gens == NULL)
        {
            printf("Ooh, burney - cannae allocate memory for new sequences");
            return -1;
        }
        else
        {
            mixr->sequence_generators = new_sequence_gens;
        }
    }
    mixr->sequence_generators[mixr->sequence_gen_num] = sg;
    return mixr->sequence_gen_num++;
}

int mixer_add_spork(mixer *mixr, double freq)
{
    printf("Adding an SPORK, mo!\n");
    spork *s = new_spork(freq);
    return add_sound_generator(mixr, (soundgenerator *)s);
}

int mixer_add_bitshift(mixer *mixr, int num_wurds, char wurds[][SIZE_OF_WURD])
{
    printf("Adding an BITSHIFT SEQUENCE GENERATOR, yo!\n");
    sequence_generator *sg = new_bitshift(num_wurds, wurds);
    if (sg)
        return add_sequence_generator(mixr, sg);
    else
        return -99;
}

int mixer_add_euclidean(mixer *mixr, int num_hits, int num_steps)
{
    printf("Adding an EUCLIDEAN SEQUENCE GENERATOR, yo!\n");
    sequence_generator *sg = new_euclidean(num_hits, num_steps);
    if (sg)
        return add_sequence_generator(mixr, sg);
    else
        return -99;
}

int mixer_add_metronome(mixer *mixr)
{
    printf("Adding metronome!\n");
    metronome *m = new_metronome();
    return add_sound_generator(mixr, (soundgenerator *)m);
}

int add_algorithm(char *line)
{
    algorithm *a = new_algorithm(line);
    return add_sound_generator(mixr, (soundgenerator *)a);
}

int add_chaosmonkey(int soundgen)
{
    chaosmonkey *cm = new_chaosmonkey(soundgen);
    return add_sound_generator(mixr, (soundgenerator *)cm);
}

int add_minisynth(mixer *mixr)
{
    printf("Adding a MINISYNTH!!...\n");
    minisynth *ms = new_minisynth();
    return add_sound_generator(mixr, (soundgenerator *)ms);
}

int add_digisynth(mixer *mixr, char *filename)
{
    printf("Adding a DIGISYNTH!!...\n");
    digisynth *ds = new_digisynth(filename);
    return add_sound_generator(mixr, (soundgenerator *)ds);
}
int add_dxsynth(mixer *mixr)
{
    printf("Adding a DXSYNTH!!...\n");
    dxsynth *dx = new_dxsynth();
    return add_sound_generator(mixr, (soundgenerator *)dx);
}

int add_looper(mixer *mixr, char *filename, double loop_len)
{
    printf("ADD looper - LOOP LEN %f\n", loop_len);
    looper *l = new_looper(filename, loop_len);
    if (l == NULL)
    {
        printf("Barfed on looper creation\n");
        return -1;
    }
    return add_sound_generator(mixr, (soundgenerator *)l);
}

int add_granulator(mixer *mixr, char *filename)
{
    printf("ADDING A GRANNY!\n");
    granulator *g = new_granulator(filename);
    printf("GOT A GRAANY\n");
    return add_sound_generator(mixr, (soundgenerator *)g);
}

inline void mixer_update_timing_info(mixer *mixr, long long int frame_time)
{

    mixr->timing_info.start_of_loop = false;
    mixr->timing_info.is_midi_tick = false;
    mixr->timing_info.is_thirtysecond = false;
    mixr->timing_info.is_sixteenth = false;
    mixr->timing_info.is_eighth = false;
    mixr->timing_info.is_quarter = false;

    double phase = link_get_phase_at_time(mixr->m_ableton_link, frame_time);
    double phase_at_last_sample = link_get_phase_at_time(mixr->m_ableton_link, frame_time - micros_per_sample);
    if (phase < phase_at_last_sample)
    {
        //printf("YA FUCING DANCER - START OF LOOOOOP!\n");
        mixr->timing_info.start_of_loop = true;
        mixer_emit_event(mixr, TIME_START_OF_LOOP_TICK);
    }


    double beat_time = link_get_beat_at_time(mixr->m_ableton_link, frame_time);
    double next_midi_tick = mixr->timing_info.time_of_last_midi_tick + midi_tick_len_as_percent;
    //printf("MIDI TICK! beattime:%f midi_tick_len_as_percent: %f next_midi_tick:%f\n", beat_time, midi_tick_len_as_percent, next_midi_tick);
    if (beat_time  > next_midi_tick)
    {
        //printf("MIDI TICK! beattime:%f next_midi_tick:%f\n", beat_time, next_midi_tick);
        mixr->timing_info.time_of_last_midi_tick = beat_time;
        mixr->timing_info.midi_tick++;
        mixr->timing_info.is_midi_tick = true;
        mixer_emit_event(mixr, TIME_MIDI_TICK);
    }

    if (mixr->timing_info.is_midi_tick)
    {
        // TODO - magic number defined -- it's PPQN(960) / 4
        // e.g. split quarter into smaller units
        if (mixr->timing_info.midi_tick % 120 == 0)
        {
            mixr->timing_info.is_thirtysecond = true;
            mixer_emit_event(mixr, TIME_THIRTYSECOND_TICK);

            if (mixr->timing_info.midi_tick % 240 == 0)
            {
                mixr->timing_info.is_sixteenth = true;
                mixer_emit_event(mixr, TIME_SIXTEENTH_TICK);

                if (mixr->timing_info.midi_tick % 480 == 0)
                {
                    mixr->timing_info.is_eighth = true;
                    mixer_emit_event(mixr, TIME_EIGHTH_TICK);

                    if (mixr->timing_info.midi_tick % PPQN == 0)
                    {
                        //printf("BEAT -- midi tick:%d\n", mixr->timing_info.midi_tick);
                        mixr->timing_info.is_quarter = true;
                        mixer_emit_event(mixr, TIME_QUARTER_TICK);
                    }
                }
            }
        }
    }

    if (mixr->timing_info.start_of_loop)
    {
        if (mixr->scene_start_pending)
        {
            mixer_play_scene(mixr, mixr->current_scene);
            mixr->scene_start_pending = false;
        }
    }
}

int mixer_gennext(mixer *mixr, float *out, int frames_per_buffer)
{

    double buffer_begin_at_output = link_update_from_main_callback(mixr->m_ableton_link, frames_per_buffer);

    for (int i = 0, j = 0; i < frames_per_buffer; i++, j += 2)
    {
        long long frame_time = llround(i * micros_per_sample);
        mixer_update_timing_info(mixr, buffer_begin_at_output + frame_time);

        double output_left = 0.0;
        double output_right = 0.0;
        if (mixr->soundgen_num > 0)
        {
            for (int i = 0; i < mixr->soundgen_num; i++)
            {
                if (mixr->sound_generators[i] != NULL)
                {
                    mixr->soundgen_cur_val[i] =
                        mixr->sound_generators[i]->gennext(
                            mixr->sound_generators[i]);
                    output_left += mixr->soundgen_cur_val[i].left;
                    output_right += mixr->soundgen_cur_val[i].right;
                }
            }
        }

        out[j] = mixr->volume * (output_left / 1.53);
        out[j + 1] = mixr->volume * (output_right / 1.53);
    }

    return 0;
}

void mixer_play_scene(mixer *mixr, int scene_num)
{
    scene *s = &mixr->scenes[scene_num];
    for (int i = 0; i < mixr->soundgen_num; i++)
    {
        if (!mixer_is_soundgen_in_scene(i, s) &&
            mixer_is_valid_soundgen_num(mixr, i))
        {
            mixr->sound_generators[i]->stop(mixr->sound_generators[i]);
        }
    }

    for (int i = 0; i < s->num_tracks; i++)
    {
        int soundgen_num = s->soundgen_tracks[i].soundgen_num;
        if (soundgen_num == -1)
        {
            continue;
        }
        int soundgen_track_num = s->soundgen_tracks[i].soundgen_track_num;
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
        {
            mixr->sound_generators[soundgen_num]->start(
                mixr->sound_generators[soundgen_num]);
            mixr->sound_generators[soundgen_num]->make_active_track(
                mixr->sound_generators[soundgen_num], soundgen_track_num);
        }
        else
        {
            printf("Oh, a deleted soundgen, better remove that from "
                   "the scene\n");
            s->soundgen_tracks[i].soundgen_num = -1;
        }
    }
}

void update_environment(char *key, int val)
{
    int env_item_index = 0;
    bool is_update = false;
    for (int i = 0; i < mixr->env_var_count; i++)
    {
        if (strncmp(key, mixr->environment[i].key, ENVIRONMENT_KEY_SIZE) == 0)
        {
            is_update = true;
            env_item_index = i;
        }
    }
    if (is_update)
    {
        mixr->environment[env_item_index].val = val;
    }
    else
    {
        strncpy((char *)&mixr->environment[mixr->env_var_count].key, key,
                ENVIRONMENT_KEY_SIZE);
        mixr->environment[mixr->env_var_count].val = val;
        mixr->env_var_count++;
    }
}

int get_environment_val(char *key, int *return_val)
{
    for (int i = 0; i < mixr->env_var_count; i++)
    {
        if (strncmp(key, mixr->environment[i].key, ENVIRONMENT_KEY_SIZE) == 0)
        {
            *return_val = mixr->environment[i].val;
            return 1;
        }
    }
    return 0;
}

bool mixer_del_soundgen(mixer *mixr, int soundgen_num)
{
    if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
    {
        printf("MIXR!! Deleting SOUND GEN %d\n", soundgen_num);
        soundgenerator *sg = mixr->sound_generators[soundgen_num];
        mixr->sound_generators[soundgen_num] = NULL;
        sg->self_destruct(sg);
    }
    return true;
}

bool mixer_is_valid_soundgen_num(mixer *mixr, int soundgen_num)
{
    if (soundgen_num >= 0 && soundgen_num < mixr->soundgen_num &&
        mixr->sound_generators[soundgen_num] != NULL)
        return true;
    return false;
}

bool mixer_is_valid_env_var(mixer *mixr, char *key)
{
    for (int i = 0; i < mixr->env_var_count; i++)
        if (strncmp(key, mixr->environment[i].key, ENVIRONMENT_KEY_SIZE) == 0)
            return true;
    return false;
}

bool mixer_is_valid_seq_gen_num(mixer *mixr, int sgnum)
{
    if (sgnum >= 0 && sgnum < mixr->sequence_gen_num &&
        mixr->sequence_generators[sgnum] != NULL)
        return true;
    return false;
}

bool mixer_is_valid_scene_num(mixer *mixr, int scene_num)
{
    if (mixr->num_scenes > 0 && scene_num < mixr->num_scenes)
        return true;
    return false;
}

bool mixer_is_valid_soundgen_track_num(mixer *mixr, int soundgen_num,
                                       int track_num)
{
    if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
        track_num < mixr->sound_generators[soundgen_num]->get_num_tracks(
                        mixr->sound_generators[soundgen_num]))
        return true;

    return false;
}

int mixer_add_scene(mixer *mixr, int num_bars)
{
    if (mixr->num_scenes >= MAX_SCENES)
    {
        printf("Dingie mate\n");
        return false;
    }
    printf("NUM BARSZZ! %d\n", num_bars);

    mixr->scenes[mixr->num_scenes].num_bars_to_play = num_bars;

    return mixr->num_scenes++;
}

bool mixer_add_soundgen_track_to_scene(mixer *mixr, int scene_num,
                                       int soundgen_num, int soundgen_track)
{
    if (!mixer_is_valid_scene_num(mixr, scene_num))
    {
        printf("%d is not a valid scene number\n", scene_num);
        return false;
    }
    if (!mixer_is_valid_soundgen_track_num(mixr, soundgen_num, soundgen_track))
    {
        printf("%d is not a valid soundgen number\n", soundgen_num);
        return false;
    }

    if (mixr->scenes[scene_num].num_tracks >= MAX_TRACKS_PER_SCENE)
    {
        printf("Too many tracks for this scene\n");
        return false;
    }

    scene *s = &mixr->scenes[scene_num];
    s->soundgen_tracks[s->num_tracks].soundgen_num = soundgen_num;
    s->soundgen_tracks[s->num_tracks].soundgen_track_num = soundgen_track;

    s->num_tracks++;

    return true;
}

bool mixer_rm_soundgen_track_from_scene(mixer *mixr, int scene_num,
                                        int soundgen_num, int soundgen_track)
{
    if (!mixer_is_valid_scene_num(mixr, scene_num))
    {
        printf("%d is not a valid scene number\n", scene_num);
        return false;
    }
    if (!mixer_is_valid_soundgen_track_num(mixr, soundgen_num, soundgen_track))
    {
        printf("%d is not a valid soundgen number\n", soundgen_num);
        return false;
    }

    scene *s = &mixr->scenes[scene_num];
    for (int i = 0; i < s->num_tracks; i++)
    {
        if (s->soundgen_tracks[i].soundgen_num == soundgen_num &&
            s->soundgen_tracks[i].soundgen_track_num == soundgen_track)
        {
            s->soundgen_tracks[i].soundgen_num = -1;
            return true;
        }
    }

    return false;
}
bool mixer_is_soundgen_in_scene(int soundgen_num, scene *s)
{
    for (int i = 0; i < s->num_tracks; i++)
    {
        if (soundgen_num == s->soundgen_tracks[i].soundgen_num)
            return true;
    }
    return false;
}

bool mixer_cp_scene(mixer *mixr, int scene_num_from, int scene_num_to)
{
    if (!mixer_is_valid_scene_num(mixr, scene_num_from))
    {
        printf("%d is not a valid scene number\n", scene_num_from);
        return false;
    }
    if (!mixer_is_valid_scene_num(mixr, scene_num_to))
    {
        printf("%d is not a valid scene number\n", scene_num_from);
        return false;
    }

    mixr->scenes[scene_num_to] = mixr->scenes[scene_num_from];

    return true;
}

void mixer_preview_track(mixer *mixr, char *filename)
{
    (void)mixr;
    (void)filename;
    // TODO
}

synthbase *get_synthbase(soundgenerator *self)
{
    if (self->type == MINISYNTH_TYPE)
    {
        minisynth *ms = (minisynth *)self;
        return &ms->base;
    }
    else if (self->type == DIGISYNTH_TYPE)
    {
        digisynth *ds = (digisynth *)self;
        return &ds->base;
    }
    else if (self->type == DXSYNTH_TYPE)
    {
        dxsynth *dx = (dxsynth *)self;
        return &dx->base;
    }
    else
    {
        printf("Error! Don't know what type of SYNTH this is\n");
        return NULL;
    }
}

// TODO - better function name - this is programatic calls, which
// basically adds a matching delete after use event i.e. == a note off
void synth_handle_midi_note(soundgenerator *sg, int note, int velocity,
                            bool update_last_midi)
{
    if (mixr->debug_mode)
        print_midi_event(note);

    if (sg->type == MINISYNTH_TYPE)
    {
        minisynth *ms = (minisynth *)sg;
        if (update_last_midi)
        {
            minisynth_add_last_note(ms, note);
        }
        minisynth_midi_note_on(ms, note, velocity);

        // note_off_tick =
        //    (int)(mixr->timing_info.midi_tick +
        //          (PPSIXTEENTH * ms->m_settings.m_sustain_time_sixteenth - 7)
        //          +
        //          (ms->m_settings.m_attack_time_msec *
        //           mixr->timing_info.midi_ticks_per_ms)) %
        //    PPNS;
    }
    else if (sg->type == DIGISYNTH_TYPE)
    {
        digisynth *ds = (digisynth *)sg;
        digisynth_midi_note_on(ds, note, velocity);
    }
    else if (sg->type == DXSYNTH_TYPE)
    {
        dxsynth *dx = (dxsynth *)sg;
        dxsynth_midi_note_on(dx, note, velocity);
    }

    int note_off_tick =
        (mixr->timing_info.midi_tick + (PPSIXTEENTH * 4 - 7)) % PPNS;

    synthbase *base = get_synthbase(sg);
    midi_event off_event = new_midi_event(note_off_tick, 128, note, velocity);
    ////////////////////////

    if (base->recording)
    {
        printf("Recording note!\n");
        int note_on_tick = mixr->timing_info.midi_tick % PPNS;
        midi_event on_event = new_midi_event(note_on_tick, 144, note, velocity);

        int final_note_off_tick =
            synthbase_add_event(base, base->cur_melody, off_event);

        on_event.tick_off = final_note_off_tick;
        synthbase_add_event(base, base->cur_melody, on_event);
    }
    else
    {
        off_event.delete_after_use = true; // _THIS_ is the magic
        synthbase_add_event(base, base->cur_melody, off_event);
    }
}

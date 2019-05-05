#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#include <portaudio.h>

#include "ableton_link_wrapper.h"
#include "algorithm.h"
#include "bitshift.h"
#include "defjams.h"
#include "digisynth.h"
#include "drumsampler.h"
#include "drumsynth.h"
#include "dxsynth.h"
#include "envelope.h"
#include "euclidean.h"
#include "fx.h"
#include "intdiv.h"
#include "juggler.h"
#include "looper.h"
#include "markov.h"
#include "minisynth.h"
#include "mixer.h"
#include "sbmsg.h"
#include "sound_generator.h"
#include "utils.h"

extern mixer *mixr;

extern const char *key_names[NUM_KEYS];
extern char *chord_type_names[NUM_CHORD_TYPES];

static const char *s_progressions[NUM_PROGRESSIONS] = {
    "I-IV-V", "I-V-vi-IV", "I-vi-IV-V", "vi-ii-V-I"};

const wchar_t *s_status_colors[] = {
    WCOOL_COLOR_PINK,      // MINISYNTH_TYPE
    WCOOL_COLOR_ORANGE,    // DIGISYNTH_TYPE
    WCOOL_COLOR_MAUVE,     // LOOPER_TYPE
    WCOOL_COLOR_YELLOW,    // BITWIZE_TYPE
    WANSI_COLOR_DEEP_RED,  // LOOPER_TYPE
    WANSI_COLOR_GREEN_TOO, // DRUMSAMPLER_TYPE
    WANSI_COLOR_MAGENTA,   // DRUMSYNTH_TYPE
    WANSI_COLOR_CYAN,      // ALGORITHM_TYPE
    WANSI_COLOR_GREEN,     // CHAOSMONKEY_TYPE
    WANSI_COLOR_BLUE       //
};

const char *s_midi_control_type_name[] = {"NONE", "SYNTH", "DRUMSYNTH"};

const char *s_sg_names[] = {"MOOG", "DIGI", "DX", "LOOP", "STEP", "STEP"};

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

    mixr->timing_info.cur_sample = -1;
    mixr->timing_info.midi_tick = -1;
    mixr->timing_info.sixteenth_note_tick = -1;
    mixr->timing_info.loop_beat = 0;
    mixr->timing_info.time_of_next_midi_tick = 0;
    mixr->timing_info.has_started = false;
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

    mixr->key = C;
    mixr->octave = 3;
    mixer_set_notes(mixr);
    mixer_set_chord_progression(mixr, 1);
    mixr->bars_per_chord = 4;
    mixr->should_progress_chords = false;

    // mixr->worker.running = true;
    // mixr->worker.have_midi_tick = true;
    // pthread_mutex_init(&mixr->worker.midi_tick_mutex, NULL);
    // pthread_cond_init(&mixr->worker.midi_tick_cond, NULL);
    mixr->processing_addr = lo_address_new(NULL, "7770");

    return mixr;
}

void mixer_status_mixr(mixer *mixr)
{
    LinkData data = link_get_timing_data_for_display(mixr->m_ableton_link);
    // clang-format off
    printf(COOL_COLOR_GREEN
           "\n::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n"
           ":::::::::: vol:" ANSI_COLOR_WHITE "%.2f" COOL_COLOR_GREEN
           " bpm:" ANSI_COLOR_WHITE "%.2f" COOL_COLOR_GREEN
           " quantum:" ANSI_COLOR_WHITE "%.2f" COOL_COLOR_GREEN
           " beat:" ANSI_COLOR_WHITE "%.2f" COOL_COLOR_GREEN
           " phase:" ANSI_COLOR_WHITE "%.2f" COOL_COLOR_GREEN
           " num_peers:" ANSI_COLOR_WHITE "%d\n" COOL_COLOR_GREEN
           ":::::::::: cur_16th:%d preview_enabled:%d filename:%s\n"
           ":::::::::: MIDI cont:%s sg_midi:%d midi_type:%s midi_print:%d midi_bank:%d\n"
           ":::::::::: key:%s chord:%s %s octave:%d bars_per_chord:%d move:%d prog:(%d)%s\n"
           ANSI_COLOR_RESET,
           mixr->volume, data.tempo, data.quantum, data.beat, data.phase, data.num_peers,
           mixr->timing_info.sixteenth_note_tick % 16,
           mixr->preview.enabled, mixr->preview.filename,
           mixr->have_midi_controller ? mixr->midi_controller_name : "NONE",
           mixr->active_midi_soundgen_num,
           mixr->active_midi_soundgen_num == -99
               ? "NONE"
               : s_midi_control_type_name
                     [mixr->midi_control_destination],
           mixr->midi_print_notes,
           mixr->midi_bank_num,
           key_names[mixr->key], key_names[mixr->chord],
           chord_type_names[mixr->chord_type], mixr->octave, mixr->bars_per_chord,
           mixr->should_progress_chords,
           mixr->progression_type, s_progressions[mixr->progression_type]);
    // clang-format on

    if (mixr->num_scenes > 1)
    {
        printf(COOL_COLOR_GREEN "\n[" ANSI_COLOR_WHITE "scenes" COOL_COLOR_GREEN
                                "]\n");
        for (int i = 0; i < mixr->num_scenes; i++)
        {
            printf(":::::::::: [%d] ", i);
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
}
void mixer_status_algoz(mixer *mixr, bool all)
{
    wchar_t wss[MAX_STATIC_STRING_SZ] = {};
    if (mixr->algorithm_num > 0)
    {
        printf(COOL_COLOR_GREEN "\n[" ANSI_COLOR_WHITE
                                "algorithms" COOL_COLOR_GREEN "]\n");
        for (int i = 0; i < mixr->algorithm_num; i++)
        {
            if (mixr->algorithms[i] != NULL)
            {
                if (mixr->algorithms[i]->active || all)
                {
                    wmemset(wss, 0, MAX_STATIC_STRING_SZ);
                    algorithm_status(mixr->algorithms[i], wss);
                    wprintf(WCOOL_COLOR_GREEN "[" WANSI_COLOR_WHITE
                                              "algo %d" WCOOL_COLOR_GREEN
                                              "] " WANSI_COLOR_RESET,
                            i);
                    wprintf(L"%ls\n", wss);
                    wprintf(WANSI_COLOR_RESET);
                }
            }
        }
    }
}

void mixer_status_valz(mixer *mixr)
{
    wchar_t wss[MAX_STATIC_STRING_SZ] = {};
    if (mixr->value_gen_num > 0)
    {
        printf(COOL_COLOR_GREEN "\n[" ANSI_COLOR_WHITE
                                "value generators" COOL_COLOR_GREEN "]\n");
        for (int i = 0; i < mixr->value_gen_num; i++)
        {
            if (mixr->value_generators[i] != NULL)
            {
                wmemset(wss, 0, MAX_STATIC_STRING_SZ);
                mixr->value_generators[i]->status(mixr->value_generators[i],
                                                  wss);
                wprintf(WANSI_COLOR_WHITE "[%2d]" WANSI_COLOR_RESET, i);
                wprintf(L"  %ls\n", wss);
                wprintf(WANSI_COLOR_RESET);
            }
        }
    }
}

void mixer_status_patz(mixer *mixr)
{
    wchar_t wss[MAX_STATIC_STRING_SZ] = {};
    if (mixr->pattern_gen_num > 0)
    {
        printf(COOL_COLOR_GREEN "\n[" ANSI_COLOR_WHITE
                                "pattern generators" COOL_COLOR_GREEN "]\n");
        for (int i = 0; i < mixr->pattern_gen_num; i++)
        {
            if (mixr->pattern_generators[i] != NULL)
            {
                wmemset(wss, 0, MAX_STATIC_STRING_SZ);
                mixr->pattern_generators[i]->status(mixr->pattern_generators[i],
                                                    wss);
                wprintf(WANSI_COLOR_WHITE "[%2d]" WANSI_COLOR_RESET, i);
                wprintf(L"  %ls\n", wss);
                wprintf(WANSI_COLOR_RESET);
            }
        }
    }
}
void mixer_status_sgz(mixer *mixr, bool all)
{
    wchar_t wss[MAX_STATIC_STRING_SZ] = {};
    if (mixr->soundgen_num > 0)
    {
        printf(COOL_COLOR_GREEN "\n[" ANSI_COLOR_WHITE
                                "sound generators" COOL_COLOR_GREEN "]\n");
        for (int i = 0; i < mixr->soundgen_num; i++)
        {
            if (mixr->sound_generators[i] != NULL)
            {
                if ((mixr->sound_generators[i]->active &&
                     mixr->sound_generators[i]->get_volume(
                         mixr->sound_generators[i]) > 0.0) ||
                    all)
                {
                    wmemset(wss, 0, MAX_STATIC_STRING_SZ);
                    mixr->sound_generators[i]->status(mixr->sound_generators[i],
                                                      wss);

                    // clang-format off
                    wprintf(WCOOL_COLOR_GREEN
                            "[" WANSI_COLOR_WHITE "%s %d"
                            WCOOL_COLOR_GREEN"] " WANSI_COLOR_RESET,
                            s_sg_names[mixr->sound_generators[i]->type], i);
                    wprintf(L"%ls", wss);
                    wprintf(WANSI_COLOR_RESET);
                    // clang-format on

                    if (mixr->sound_generators[i]->effects_num > 0)
                    {
                        printf("      ");
                        for (int j = 0;
                             j < mixr->sound_generators[i]->effects_num; j++)
                        {
                            fx *f = mixr->sound_generators[i]->effects[j];
                            if (f->enabled)
                                printf(COOL_COLOR_YELLOW);
                            else
                                printf(ANSI_COLOR_RESET);
                            char fx_status[512];
                            f->status(f, fx_status);
                            printf("\n[fx %d:%d %s]", i, j, fx_status);
                        }
                        printf(ANSI_COLOR_RESET);
                    }
                    printf("\n\n");
                }
            }
        }
    }
}

void mixer_ps(mixer *mixr, bool all)
{

    print_logo();
    mixer_status_mixr(mixr);
    mixer_status_algoz(mixr, all);
    mixer_status_patz(mixr);
    mixer_status_sgz(mixr, all);
    mixer_status_valz(mixr);
    printf(ANSI_COLOR_RESET);
}

void mixer_print_notes(mixer *mixr)
{
    printf("Current KEY is %s. Compat NOTEs are:", key_names[mixr->key]);
    // for (int i = 0; i < 6; ++i)
    //{
    //    printf("%s ", key_names[compat_keys[mixr->key][i]]);
    //}
    printf("\n");
}

void mixer_emit_event(mixer *mixr, broadcast_event event)
{
    for (int i = 0; i < mixr->algorithm_num; ++i)
    {
        algorithm *a = mixr->algorithms[i];
        if (a != NULL)
            algorithm_event_notify(a, event);
    }

    for (int i = 0; i < mixr->pattern_gen_num; ++i)
    {
        pattern_generator *pg = mixr->pattern_generators[i];
        if (pg != NULL)
            pg->event_notify(pg, event);
    }

    for (int i = 0; i < mixr->soundgen_num; ++i)
    {
        sound_generator *sg = mixr->sound_generators[i];
        if (sg != NULL)
        {
            sg->event_notify(sg, event);
            if (sg->effects_num > 0)
            {
                for (int j = 0; j < sg->effects_num; j++)
                {
                    if (sg->effects[j])
                    {
                        fx *f = sg->effects[j];
                        f->event_notify(f, event);
                    }
                }
            }
        }
    }
}

void mixer_update_bpm(mixer *mixr, int bpm)
{
    mixr->bpm = bpm;
    mixr->timing_info.frames_per_midi_tick = (60.0 / bpm * SAMPLE_RATE) / PPQN;
    mixr->timing_info.loop_len_in_frames =
        mixr->timing_info.frames_per_midi_tick * PPBAR;
    mixr->timing_info.loop_len_in_ticks = PPBAR;

    mixr->timing_info.ms_per_midi_tick = 60000.0 / (bpm * PPQN);

    mixr->timing_info.size_of_thirtysecond_note =
        (PPSIXTEENTH / 2) * mixr->timing_info.frames_per_midi_tick;
    mixr->timing_info.size_of_sixteenth_note =
        mixr->timing_info.size_of_thirtysecond_note * 2;
    mixr->timing_info.size_of_eighth_note =
        mixr->timing_info.size_of_sixteenth_note * 2;
    mixr->timing_info.size_of_quarter_note =
        mixr->timing_info.size_of_eighth_note * 2;

    mixer_emit_event(mixr, (broadcast_event){.type = TIME_BPM_CHANGE});
    link_set_bpm(mixr->m_ableton_link, bpm);
}

void mixer_vol_change(mixer *mixr, float vol)
{
    if (vol >= 0.0 && vol <= 1.0)
    {
        mixr->volume = vol;
    }
}

void vol_change(mixer *mixr, int sg, float vol)
{
    if (!mixer_is_valid_soundgen_num(mixr, sg))
    {
        printf("Nah mate, returning\n");
        return;
    }
    mixr->sound_generators[sg]->set_volume(mixr->sound_generators[sg], vol);
}

void pan_change(mixer *mixr, int sg, float val)
{
    if (!mixer_is_valid_soundgen_num(mixr, sg))
    {
        printf("Nah mate, returning\n");
        return;
    }
    mixr->sound_generators[sg]->set_pan(mixr->sound_generators[sg], val);
}

int add_sound_generator(mixer *mixr, sound_generator *sg)
{
    if (mixr->soundgen_num == MAX_NUM_SOUND_GENERATORS)
        return -99;

    sg->mixer_idx = mixr->soundgen_num;
    mixr->sound_generators[mixr->soundgen_num] = sg;
    return mixr->soundgen_num++;
}

int add_value_generator(mixer *mixr, value_generator *vg)
{
    if (mixr->value_gen_num == MAX_NUM_VALUE_GENERATORS)
        return -99;

    mixr->value_generators[mixr->value_gen_num] = vg;
    return mixr->value_gen_num++;
}

int add_pattern_generator(mixer *mixr, pattern_generator *sg)
{
    if (mixr->pattern_gen_num == MAX_NUM_PATTERN_GENERATORS)
        return -99;

    mixr->pattern_generators[mixr->pattern_gen_num] = sg;
    return mixr->pattern_gen_num++;
}

int mixer_add_algorithm(mixer *mixr, algorithm *a)
{
    printf("Adding an ALGORITHM, yo!\n");
    if (mixr->soundgen_num == MAX_NUM_ALGORITHMS)
        return -99;

    mixr->algorithms[mixr->algorithm_num] = a;
    return mixr->algorithm_num++;
}

int mixer_add_bitshift(mixer *mixr, int num_wurds, char wurds[][SIZE_OF_WURD])
{
    printf("Adding an BITSHIFT PATTERN GENERATOR, yo!\n");
    pattern_generator *sg = new_bitshift(num_wurds, wurds);
    if (sg)
        return add_pattern_generator(mixr, sg);
    else
        return -99;
}

int mixer_add_markov(mixer *mixr, unsigned int type)
{
    printf("Adding an MARKOV PATTERN GENERATOR, yo!\n");
    pattern_generator *sg = new_markov(type);
    if (sg)
        return add_pattern_generator(mixr, sg);
    else
        return -99;
}

int mixer_add_intdiv(mixer *mixr)
{
    printf("Adding an INTDIV!\n");
    pattern_generator *sg = new_intdiv();
    if (sg)
        return add_pattern_generator(mixr, sg);
    else
        return -99;
}

int mixer_add_euclidean(mixer *mixr, int num_hits, int num_steps)
{
    printf("Adding an EUCLIDEAN PATTERN GENERATOR, yo!\n");
    pattern_generator *sg = new_euclidean(num_hits, num_steps);
    if (sg)
        return add_pattern_generator(mixr, sg);
    else
        return -99;
}

int mixer_add_juggler(mixer *mixr, unsigned int style)
{
    printf("Adding an JUGGLER PATTERN GENERATOR, yo!\n");
    pattern_generator *sg = new_juggler(style);
    if (sg)
        return add_pattern_generator(mixr, sg);
    else
        return -99;
}

int mixer_add_value_list(mixer *mixr, unsigned int values_type, int values_len,
                         void *values)
{
    printf("Adding a new VALUE LIST GENERATOR!\n");
    value_generator *vg = new_value_generator(values_type, values_len, values);
    if (vg)
        return add_value_generator(mixr, vg);
    else
        return -99;
}

int add_minisynth(mixer *mixr)
{
    printf("Adding a MINISYNTH!!...\n");
    minisynth *ms = new_minisynth();
    return add_sound_generator(mixr, (sound_generator *)ms);
}

int add_digisynth(mixer *mixr, char *filename)
{
    printf("Adding a DIGISYNTH!!...\n");
    digisynth *ds = new_digisynth(filename);
    return add_sound_generator(mixr, (sound_generator *)ds);
}
int add_dxsynth(mixer *mixr)
{
    printf("Adding a DXSYNTH!!...\n");
    dxsynth *dx = new_dxsynth();
    printf("GOT NEW  DXSYNTH!!...\n");
    return add_sound_generator(mixr, (sound_generator *)dx);
}

int add_looper(mixer *mixr, char *filename)
{
    printf("ADDING A GRANNY!\n");
    looper *g = new_looper(filename);
    printf("GOT A GRAANY\n");
    return add_sound_generator(mixr, (sound_generator *)g);
}

void mixer_events_output(mixer *mixr)
{
    mixr->timing_info.is_thirtysecond = false;
    mixr->timing_info.is_sixteenth = false;
    mixr->timing_info.is_eighth = false;
    mixr->timing_info.is_quarter = false;

    if (mixr->timing_info.is_midi_tick)
    {
        mixer_check_for_midi_messages(mixr);

        if (mixr->timing_info.midi_tick % 120 == 0)
        {
            mixr->timing_info.is_thirtysecond = true;
            mixer_emit_event(mixr,
                             (broadcast_event){.type = TIME_THIRTYSECOND_TICK});

            if (mixr->timing_info.midi_tick % 240 == 0)
            {
                mixr->timing_info.is_sixteenth = true;
                mixr->timing_info.sixteenth_note_tick++;

                // printf("\nSIXTEENTH! tick:%d\n",
                // mixr->timing_info.sixteenth_note_tick);
                // mixer_print_timing_info(mixr);

                mixer_emit_event(
                    mixr, (broadcast_event){.type = TIME_SIXTEENTH_TICK});

                if (mixr->timing_info.midi_tick % 480 == 0)
                {
                    mixr->timing_info.is_eighth = true;
                    mixer_emit_event(
                        mixr, (broadcast_event){.type = TIME_EIGHTH_TICK});

                    if (mixr->timing_info.midi_tick % PPQN == 0)
                    {
                        mixr->timing_info.is_quarter = true;
                        mixer_emit_event(
                            mixr, (broadcast_event){.type = TIME_QUARTER_TICK});
                    }
                }
            }
        }

        // so far only used for ARP engines
        if (mixr->timing_info.midi_tick % 160 == 0)
        {
            mixer_emit_event(mixr,
                             (broadcast_event){.type = TIME_TWENTYFOURTH_TICK});

            if (mixr->timing_info.midi_tick % 320 == 0)
            {
                mixer_emit_event(mixr,
                                 (broadcast_event){.type = TIME_TWELTH_TICK});

                if (mixr->timing_info.midi_tick % 640 == 0)
                {
                    mixer_emit_event(
                        mixr, (broadcast_event){.type = TIME_SIXTH_TICK});

                    if (mixr->timing_info.midi_tick % 1280 == 0)
                        mixer_emit_event(
                            mixr, (broadcast_event){.type = TIME_THIRD_TICK});
                }
            }
        }

        if (mixr->timing_info.midi_tick % PPBAR == 0)
        {
            mixer_emit_event(
                mixr, (broadcast_event){.type = TIME_START_OF_LOOP_TICK});
            if (mixr->scene_start_pending)
            {
                mixer_play_scene(mixr, mixr->current_scene);
                mixr->scene_start_pending = false;
            }
        }

        mixer_emit_event(mixr, (broadcast_event){.type = TIME_MIDI_TICK});
        // lo_send(mixr->processing_addr, "/bpm", NULL);
    }
}

bool should_progress_chords(mixer *mixr, int tick)
{
    int chance = rand() % 100;
    if (mixr->should_progress_chords == false)
        return false;

    if (tick == 0)
    {
        if (chance > 75)
            return true;
    }
    else if (tick == PPQN * 2)
    {
        if (chance > 97)
            return true;
    }
    else if (tick == PPQN * 3)
    {
        if (chance > 99)
            return true;
    }

    return false;
}

// static bool first_run = true;
int mixer_gennext(mixer *mixr, float *out, int frames_per_buffer)
{

    link_update_from_main_callback(mixr->m_ableton_link, frames_per_buffer);

    for (int i = 0, j = 0; i < frames_per_buffer; i++, j += 2)
    {
        double output_left = 0.0;
        double output_right = 0.0;

        mixr->timing_info.cur_sample++;

        if (mixr->preview.enabled)
        {
            stereo_val preview_audio = preview_buffer_generate(&mixr->preview);
            output_left += preview_audio.left * 0.6;
            output_right += preview_audio.right * 0.6;
        }

        if (link_is_midi_tick(mixr->m_ableton_link, &mixr->timing_info, i))
        {
            int current_tick_within_bar = mixr->timing_info.midi_tick % PPBAR;
            if (should_progress_chords(mixr, current_tick_within_bar))
                mixer_next_chord(mixr);

            mixer_events_output(mixr);
        }

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
            sequence_engine_make_active_pattern(
                get_sequence_engine(mixr->sound_generators[soundgen_num]),
                soundgen_track_num);
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
        sound_generator *sg = mixr->sound_generators[soundgen_num];

        if (mixr->active_midi_soundgen_num == soundgen_num)
            mixr->active_midi_soundgen_num = -99;

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

bool mixer_is_valid_algo(mixer *mixr, int algo_num)
{
    if (algo_num >= 0 && algo_num < mixr->algorithm_num &&
        mixr->algorithms[algo_num] != NULL)
        return true;
    return false;
}

bool mixer_is_valid_fx(mixer *mixr, int soundgen_num, int fx_num)
{
    if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
    {
        sound_generator *sg = mixr->sound_generators[soundgen_num];
        if (fx_num >= 0 && fx_num < sg->effects_num && sg->effects[fx_num])
            return true;
    }
    return false;
}
bool mixer_is_valid_env_var(mixer *mixr, char *key)
{
    for (int i = 0; i < mixr->env_var_count; i++)
        if (strncmp(key, mixr->environment[i].key, ENVIRONMENT_KEY_SIZE) == 0)
            return true;
    return false;
}

bool mixer_is_valid_pattern_gen_num(mixer *mixr, int sgnum)
{
    if (sgnum >= 0 && sgnum < mixr->pattern_gen_num &&
        mixr->pattern_generators[sgnum] != NULL)
        return true;
    return false;
}

bool mixer_is_valid_value_gen_num(mixer *mixr, int vgnum)
{
    if (vgnum >= 0 && vgnum < mixr->value_gen_num &&
        mixr->value_generators[vgnum] != NULL)
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
    if (mixer_is_valid_soundgen_num(mixr, soundgen_num) && track_num >= 0 &&
        track_num < sequence_engine_get_num_patterns(get_sequence_engine(
                        mixr->sound_generators[soundgen_num])))
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

void synth_handle_midi_note(sound_generator *sg, int note, int velocity,
                            bool update_last_midi)
{
    sequence_engine *engine = get_sequence_engine(sg);
    bool is_chord_mode = engine->chord_mode;

    int midi_notes[3] = {note, 0, 0};
    int midi_notes_len = 1; // default single note
    if (is_chord_mode)
    {
        midi_notes_len = 3;
        if (mixr->chord_type == MAJOR_CHORD)
            midi_notes[1] = note + 4;
        else
            midi_notes[1] = note + 3;
        midi_notes[2] = note + 7;
    }

    for (int i = 0; i < midi_notes_len; i++)
    {
        int note = midi_notes[i];
        printf("Adding NOTE %d\n", note);

        if (sg->type == MINISYNTH_TYPE)
        {
            minisynth *ms = (minisynth *)sg;
            minisynth_midi_note_on(ms, note, velocity);
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

        int sustain_time_in_ticks =
            engine->sustain_note_ms * mixr->timing_info.ms_per_midi_tick;
        int note_off_tick =
            (mixr->timing_info.midi_tick + sustain_time_in_ticks) % PPBAR;

        midi_event off_event = new_midi_event(128, note, velocity);
        ////////////////////////

        if (engine->recording)
        {
            printf("Recording note!\n");
            int note_on_tick = mixr->timing_info.midi_tick % PPBAR;
            midi_event on_event = new_midi_event(144, note, velocity);

            sequence_engine_add_event(engine, engine->cur_pattern,
                                      note_off_tick, off_event);
            sequence_engine_add_event(engine, engine->cur_pattern, note_on_tick,
                                      on_event);
        }
        else
        {
            off_event.delete_after_use = true; // _THIS_ is the magic
            sequence_engine_add_event(engine, engine->cur_pattern,
                                      note_off_tick, off_event);
        }
    }
}

void mixer_set_notes(mixer *mixr)
{
    mixr->notes[0] = mixr->key;
    mixr->notes[1] = (mixr->key + 2) % NUM_KEYS;  // W step
    mixr->notes[2] = (mixr->key + 4) % NUM_KEYS;  // W step
    mixr->notes[3] = (mixr->key + 5) % NUM_KEYS;  // H step
    mixr->notes[4] = (mixr->key + 7) % NUM_KEYS;  // W step
    mixr->notes[5] = (mixr->key + 9) % NUM_KEYS;  // W step
    mixr->notes[6] = (mixr->key + 11) % NUM_KEYS; // W step
    mixr->notes[7] = (mixr->key + 12) % NUM_KEYS; // H step
}

int mixer_print_timing_info(mixer *mixr)
{
    mixer_timing_info *info = &mixr->timing_info;
    printf("TIMING INFO!\n");
    printf("============\n");
    printf("FRAMES per midi tick:%d\n", info->frames_per_midi_tick);
    printf("MS per MIDI tick:%f\n", info->ms_per_midi_tick);
    printf("TIME of next MIDI tick:%f\n", info->time_of_next_midi_tick);
    printf("SIXTEENTH NOTE tick:%d\n", info->sixteenth_note_tick);
    printf("MIDI tick:%d\n", info->midi_tick);
    printf("LOOP beat:%d\n", info->loop_beat);
    printf("LOOP Started:%d\n", info->loop_started);
    printf("CUR SAMPLE:%d\n", info->cur_sample);
    printf("Loop_len_in_frames:%d\n", info->loop_len_in_frames);
    printf("Loop_len_in_ticks:%d\n", info->loop_len_in_ticks);
    printf("Size of 1/32 note:%d\n", info->size_of_thirtysecond_note);
    printf("Size of 1/16 note:%d\n", info->size_of_sixteenth_note);
    printf("Size of 1/8 note:%d\n", info->size_of_eighth_note);
    printf("Size of 1/4 note:%d\n", info->size_of_quarter_note);

    printf("Has_started:%d\n", info->has_started);
    printf("Start of loop:%d\n", info->start_of_loop);
    printf("Is 1/32:%d\n", info->is_thirtysecond);
    printf("Is 1/16:%d\n", info->is_sixteenth);
    printf("Is 1/8:%d\n", info->is_eighth);
    printf("Is 1/4:%d\n", info->is_quarter);
    printf("Is midi_tick:%d\n", info->is_midi_tick);
    return 0;
}

double mixer_get_hz_per_bar(mixer *mixr)
{

    double hz_per_beat = (60. / mixr->bpm);
    return hz_per_beat;
}

int mixer_get_ticks_per_cycle_unit(mixer *mixr, unsigned int event_type)
{
    int ticks = 0;
    switch (event_type)
    {
    case (TIME_START_OF_LOOP_TICK):
        ticks = mixr->timing_info.loop_len_in_ticks;
        break;
    case (TIME_MIDI_TICK):
        ticks = 1;
        break;
    case (TIME_QUARTER_TICK):
        ticks = PPQN;
        break;
    case (TIME_EIGHTH_TICK):
        ticks = PPQN / 2;
        break;
    case (TIME_SIXTEENTH_TICK):
        ticks = PPQN / 4;
        break;
    case (TIME_THIRTYSECOND_TICK):
        ticks = PPQN / 8;
        break;
    }
    return ticks;
}
void mixer_set_octave(mixer *mixr, int octave)
{
    if (octave > -10 && octave < 10)
        mixr->octave = octave;
}
void mixer_set_bars_per_chord(mixer *mixr, int bars)
{
    if (bars > 0 && bars < 32)
        mixr->bars_per_chord = bars;
}

void mixer_set_chord_progression(mixer *mixr, unsigned int prog_num)
{
    if (prog_num < NUM_PROGRESSIONS)
    {
        switch (prog_num)
        {
        case (0):
            mixr->progression_type = 0;
            mixr->prog_len = 3;
            mixr->prog_degrees[0] = 0; // I
            mixr->prog_degrees[1] = 3; // IV
            mixr->prog_degrees[2] = 4; // V
            break;
        case (1):
            mixr->progression_type = 1;
            mixr->prog_len = 4;
            mixr->prog_degrees[0] = 0; // I
            mixr->prog_degrees[1] = 4; // V
            mixr->prog_degrees[2] = 5; // vi
            mixr->prog_degrees[3] = 3; // IV
            break;
        case (2):
            mixr->progression_type = 2;
            mixr->prog_len = 4;
            mixr->prog_degrees[0] = 0; // I
            mixr->prog_degrees[1] = 5; // vi
            mixr->prog_degrees[2] = 3; // IV
            mixr->prog_degrees[3] = 4; // V
            break;
        case (3):
            mixr->progression_type = 3;
            mixr->prog_len = 4;
            mixr->prog_degrees[0] = 5; // vi
            mixr->prog_degrees[1] = 1; // ii
            mixr->prog_degrees[2] = 4; // V
            mixr->prog_degrees[3] = 0; // I
            break;
        }
    }
}
void mixer_change_chord(mixer *mixr, unsigned int root, unsigned int chord_type)
{
    if (root < NUM_KEYS && chord_type < NUM_CHORD_TYPES)
    {
        mixr->chord = root;
        mixr->chord_type = chord_type;
        mixer_emit_event(mixr, (broadcast_event){.type = TIME_CHORD_CHANGE});
    }
}

int mixer_get_key_from_degree(mixer *mixr, unsigned int scale_degree)
{
    return mixr->key + scale_degree;
}

void mixer_preview_audio(mixer *mixr, char *filename)
{
    mixr->preview.enabled = false;
    preview_buffer_import_file(&mixr->preview, filename);
}

void preview_buffer_import_file(preview_buffer *buffy, char *filename)
{
    strncpy(buffy->filename, filename, 512);
    audio_buffer_details deetz =
        import_file_contents(&buffy->audio_buffer, filename);
    buffy->audio_buffer_len = deetz.buffer_length;
    buffy->num_channels = deetz.num_channels;
    buffy->audio_buffer_read_idx = 0;
    buffy->enabled = true;
}

stereo_val preview_buffer_generate(preview_buffer *buffy)
{
    stereo_val ret = {.0, .0};
    if (!buffy->enabled || !buffy->audio_buffer)
        return ret;

    ret.left = buffy->audio_buffer[buffy->audio_buffer_read_idx];
    if (buffy->num_channels == 1)
        ret.right = ret.left;
    else
        ret.right = buffy->audio_buffer[buffy->audio_buffer_read_idx + 1];

    buffy->audio_buffer_read_idx += buffy->num_channels;
    if (buffy->audio_buffer_read_idx >= buffy->audio_buffer_len)
    {
        buffy->audio_buffer_read_idx = 0;
        buffy->enabled = false;
    }

    return ret;
}

void mixer_enable_print_midi(mixer *mixr, bool b)
{
    mixr->midi_print_notes = b;
}
void mixer_check_for_midi_messages(mixer *mixr)
{
    PmEvent msg[32];
    if (Pm_Poll(mixr->midi_stream))
    {
        int cnt = Pm_Read(mixr->midi_stream, msg, 32);
        for (int i = 0; i < cnt; i++)
        {
            int status = Pm_MessageStatus(msg[i].message);
            int data1 = Pm_MessageData1(msg[i].message);
            int data2 = Pm_MessageData2(msg[i].message);

            if (status == 176)
            {
                if (data1 == 9)
                    mixer_set_midi_bank(mixr, 0);
                if (data1 == 10)
                    mixer_set_midi_bank(mixr, 1);
                if (data1 == 11)
                    mixer_set_midi_bank(mixr, 2);
                if (data1 == 12)
                    mixer_set_midi_bank(mixr, 3);
            }

            if (mixr->midi_print_notes)
                printf("[MIDI message] status:%d data1:%d "
                       "data2:%d\n",
                       status, data1, data2);

            if (mixr->midi_control_destination != NONE &&
                mixer_is_valid_soundgen_num(mixr,
                                            mixr->active_midi_soundgen_num))
            {

                sound_generator *sg =
                    mixr->sound_generators[mixr->active_midi_soundgen_num];

                sequence_engine *engine = get_sequence_engine(sg);

                if (engine->recording)
                {
                    int tick = mixr->timing_info.midi_tick % PPBAR;
                    midi_event ev = new_midi_event(status, data1, data2);
                    sequence_engine_add_event(engine, engine->cur_pattern, tick,
                                              ev);
                }

                midi_event ev;
                ev.source = EXTERNAL_DEVICE;
                ev.event_type = status;
                ev.data1 = data1;
                ev.data2 = data2;
                ev.delete_after_use = false;
                midi_parse_midi_event(sg, &ev);
            }
            else
            {
                printf("Got midi but not connected to "
                       "synth\n");
            }
        }
    }
}

void mixer_set_midi_bank(mixer *mixr, int num)
{
    if (num >= 0 && num < 4)
        mixr->midi_bank_num = num;
}

sequence_engine *get_sequence_engine(sound_generator *self)
{
    if (self->type == MINISYNTH_TYPE)
    {
        minisynth *ms = (minisynth *)self;
        return &ms->engine;
    }
    else if (self->type == DIGISYNTH_TYPE)
    {
        digisynth *ds = (digisynth *)self;
        return &ds->engine;
    }
    else if (self->type == DXSYNTH_TYPE)
    {
        dxsynth *dx = (dxsynth *)self;
        return &dx->engine;
    }
    else if (self->type == DRUMSYNTH_TYPE)
    {
        drumsynth *ds = (drumsynth *)self;
        return &ds->engine;
    }
    else if (self->type == DRUMSAMPLER_TYPE)
    {
        drumsampler *ds = (drumsampler *)self;
        return &ds->engine;
    }
    else if (self->type == LOOPER_TYPE)
    {
        looper *lp = (looper *)self;
        return &lp->engine;
    }
    else
    {
        printf("Error! Don't know what type of SYNTH this is\n");
        return NULL;
    }
}

void mixer_set_should_progress_chords(mixer *mixr, bool b)
{
    mixr->should_progress_chords = b;
}

void mixer_next_chord(mixer *mixr)
{
    unsigned int scale_degree = mixr->prog_degrees[mixr->prog_degrees_idx];
    mixr->prog_degrees_idx = (mixr->prog_degrees_idx + 1) % mixr->prog_len;
    unsigned int root = mixr->notes[scale_degree];
    unsigned int chord_type = get_chord_type(scale_degree);
    mixer_change_chord(mixr, root, chord_type);
}

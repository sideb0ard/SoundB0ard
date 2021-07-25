#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#include <iomanip>
#include <iostream>
#include <portaudio.h>
#include <sstream>

#include <ableton_link_wrapper.h>
#include <audio_action_queue.h>
#include <defjams.h>
#include <drumsampler.h>
#include <dxsynth.h>
#include <event_queue.h>
#include <filereader.hpp>
#include <fx/envelope.h>
#include <fx/fx.h>
#include <interpreter/object.hpp>
#include <interpreter/sound_cmds.hpp>
#include <looper.h>
#include <minisynth.h>
#include <mixer.h>
#include <obliquestrategies.h>
#include <soundgenerator.h>
#include <tsqueue.hpp>
#include <utils.h>

Mixer *mixr;
extern std::shared_ptr<object::Environment> global_env;
extern Tsqueue<event_queue_item> process_event_queue;
extern Tsqueue<std::string> repl_queue;
extern Tsqueue<audio_action_queue_item> audio_queue;
extern Tsqueue<int> audio_reply_queue;
extern Tsqueue<std::string> eval_command_queue;

const char *key_names[] = {"C", "C_SHARP", "D", "D_SHARP", "E", "F", "F_SHARP",
                           "G", "G_SHARP", "A", "A_SHARP", "B"};

const char *chord_type_names[] = {"MAJOR", "MINOR", "DIMINISHED"};

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

Mixer::Mixer(double output_latency)
{
    m_ableton_link = new_ableton_link(DEFAULT_BPM);
    if (!m_ableton_link)
    {
        printf("Something fucked up with yer Ableton link, mate."
               " ye wanna get that seen tae\n");
        return;
    }
    link_set_latency(m_ableton_link, output_latency);

    volume = 0.7;
    UpdateBpm(DEFAULT_BPM);
    m_midi_controller_mode =
        KEY_MODE_ONE; // dunno whether this should be on mixer or synth
    midi_control_destination = NONE;

    for (int i = 0; i < MAX_NUM_PROC; i++)
        processes_[i] = std::make_shared<Process>();
    proc_initialized_ = true;

    // the lifetime of these booleans is a single sample

    timing_info.cur_sample = -1;
    timing_info.midi_tick = -1;
    timing_info.sixteenth_note_tick = -1;
    timing_info.loop_beat = 0;
    timing_info.time_of_next_midi_tick = 0;
    timing_info.has_started = false;
    timing_info.is_midi_tick = true;
    timing_info.is_start_of_loop = true;
    timing_info.is_thirtysecond = true;
    timing_info.is_twentyfourth = true;
    timing_info.is_sixteenth = true;
    timing_info.is_twelth = true;
    timing_info.is_eighth = true;
    timing_info.is_sixth = true;
    timing_info.is_quarter = true;
    timing_info.is_third = true;

    active_midi_soundgen_num = -99;

    timing_info.key = C;
    timing_info.octave = 3;
    SetNotes();
    SetChordProgression(1);
    bars_per_chord = 4;
    should_progress_chords = false;

    std::string contents = ReadFileContents(kStartupConfigFile);
    eval_command_queue.push(contents);
}

std::string Mixer::StatusMixr()
{
    LinkData data = link_get_timing_data_for_display(m_ableton_link);
    // clang-format off
    std::stringstream ss;
    ss << COOL_COLOR_GREEN << "\n"
    << "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n"
    << ":::::::::: vol:" << ANSI_COLOR_WHITE << volume << COOL_COLOR_GREEN
    << " bpm:" << ANSI_COLOR_WHITE <<data.tempo << COOL_COLOR_GREEN
    << " quantum:" << ANSI_COLOR_WHITE << data.quantum << COOL_COLOR_GREEN
    << " beat:" << ANSI_COLOR_WHITE << std::setprecision(2) << data.beat << COOL_COLOR_GREEN
    << " phase:" << ANSI_COLOR_WHITE << std::setprecision(2) << data.phase << COOL_COLOR_GREEN
    << " num_peers:" << ANSI_COLOR_WHITE << data.num_peers << COOL_COLOR_GREEN
    << "::::::::::\n"
    << ":::::::::: key:" << ANSI_COLOR_WHITE <<  key_names[timing_info.key] << COOL_COLOR_GREEN
    << " type:"<< ANSI_COLOR_WHITE << chord_type_names[timing_info.chord_type] << COOL_COLOR_GREEN
    << " octave:" << ANSI_COLOR_WHITE << timing_info.octave << COOL_COLOR_GREEN << " soundgen_num:"<< ANSI_COLOR_WHITE << soundgen_num << COOL_COLOR_GREEN
    << " move:" << ANSI_COLOR_WHITE << should_progress_chords << COOL_COLOR_GREEN << " prog:("<< ANSI_COLOR_WHITE << progression_type << COOL_COLOR_GREEN << ")"
    << ANSI_COLOR_WHITE << s_progressions[progression_type] << "\n";
    // clang-format on

    return ss.str();
}

std::string Mixer::StatusProcz(bool all)
{
    std::stringstream ss;
    ss << COOL_COLOR_ORANGE << "\n[" << ANSI_COLOR_WHITE << "Procz"
       << COOL_COLOR_ORANGE << "]\n";

    for (int i = 0; i < MAX_NUM_PROC; i++)
    {
        auto &p = processes_[i];
        if (p->active_ || all)
        {
            ss << ANSI_COLOR_WHITE << "p" << i << ANSI_COLOR_RESET << " "
               << p->Status() << std::endl;
        }
    }

    return ss.str();
}

std::string Mixer::StatusEnv()
{
    std::stringstream ss;
    ss << COOL_COLOR_GREEN << "\n[" << ANSI_COLOR_WHITE << "Varz"
       << COOL_COLOR_GREEN << "]" << std::endl;

    // ss << global_env->Debug();

    std::map<std::string, int> soundgens = global_env->GetSoundGenerators();
    for (auto &[var_name, sg_idx] : soundgens)
    {
        if (IsValidSoundgenNum(sg_idx))
        {
            auto sg = sound_generators_[sg_idx];
            if (!sg->active)
                continue;
            ss << ANSI_COLOR_WHITE << var_name << ANSI_COLOR_RESET " = "
               << sg->Status() << ANSI_COLOR_RESET << std::endl;

            std::stringstream margin;
            for (auto c : var_name)
                margin << " ";
            margin << "   "; // for the ' = '

            for (int i = 0; i < sg->effects_num; i++)
            {
                ss << margin.str();
                Fx *f = sg->effects[i];
                if (f->enabled_)
                    ss << COOL_COLOR_YELLOW;
                else
                    ss << ANSI_COLOR_RESET;
                char fx_status[512];
                f->Status(fx_status);
                ss << "fx" << i << " " << fx_status << std::endl;
            }
            ss << ANSI_COLOR_RESET;
        }
    }
    return ss.str();
}

std::string Mixer::StatusSgz(bool all)
{
    std::cout << "MIXER STATUS SGZ!\n";
    std::stringstream ss;
    if (soundgen_num > 0)
    {
        ss << COOL_COLOR_GREEN << "\n[" << ANSI_COLOR_WHITE
           << "sound generators" << COOL_COLOR_GREEN << "]\n";
        for (int i = 0; i < soundgen_num; i++)
        {
            if (sound_generators_[i] != NULL)
            {
                if ((sound_generators_[i]->active &&
                     sound_generators_[i]->GetVolume() > 0.0) ||
                    all)
                {

                    ss << COOL_COLOR_GREEN << "[" << ANSI_COLOR_WHITE << "s"
                       << i << COOL_COLOR_GREEN << "] " << ANSI_COLOR_RESET;
                    ss << sound_generators_[i]->Info() << ANSI_COLOR_RESET
                       << "\n";

                    if (sound_generators_[i]->effects_num > 0)
                    {
                        ss << "      ";
                        for (int j = 0; j < sound_generators_[i]->effects_num;
                             j++)
                        {
                            Fx *f = sound_generators_[i]->effects[j];
                            if (f->enabled_)
                                ss << COOL_COLOR_YELLOW;
                            else
                                ss << ANSI_COLOR_RESET;
                            char fx_status[512];
                            f->Status(fx_status);
                            ss << "\n[fx " << i << ":" << j << fx_status << "]";
                        }
                        ss << ANSI_COLOR_RESET;
                    }
                    ss << "\n\n";
                }
            }
        }
    }
    return ss.str();
}

void Mixer::Ps(bool all)
{
    std::stringstream ss;
    ss << get_string_logo();
    ss << StatusMixr();
    ss << StatusEnv();
    ss << StatusProcz();
    // ss << mixer_status_procz(mixr, all);
    ss << ANSI_COLOR_RESET;

    if (all)
        ss << StatusSgz(all);

    repl_queue.push(ss.str());
}

void Mixer::Help()
{
    std::string reply;
    if (rand() % 100 > 90)
    {
        reply = oblique_strategy();
    }
    else
    {
        std::stringstream ss;
        ss << ANSI_COLOR_WHITE;
        ss << "###### Haaaalp! ################################\n";
        ss << COOL_COLOR_MAUVE << "ps" << ANSI_COLOR_RESET
           << " // show Program Status i.e. vars, bpm, key etc.\n";
        ss << COOL_COLOR_MAUVE << "ls" << ANSI_COLOR_RESET
           << " // list sample directory\n";
        ss << COOL_COLOR_MAUVE << "midi_info();" << ANSI_COLOR_RESET
           << " // tells you how many midi ticks in a loop, and pitchs -> midi "
              "ref\n";
        ss << COOL_COLOR_MAUVE << "timing_info();" << ANSI_COLOR_RESET
           << " // tells you various timing data\n";
        ss << COOL_COLOR_MAUVE << "let x = 4;" << ANSI_COLOR_RESET
           << " // create a var named 'x' with val 1\n";
        ss << COOL_COLOR_MAUVE << "let add1 = fn(val) { return val + 1; };";
        ss << ANSI_COLOR_RESET << " // create function and assign to a var\n";
        ss << COOL_COLOR_MAUVE << "strategy;";
        ss << ANSI_COLOR_RESET
           << " // get a random Brian Eno Oblique Strategy!\n";
        ss << ANSI_COLOR_WHITE;
        ss << "######## Add Sound Generators ########################\n";
        ss << ANSI_COLOR_RESET;
        ss << COOL_COLOR_MAUVE << "let dx100 = fm();" << ANSI_COLOR_RESET
           << " // create an instance of FM Synth assigned to var dx100\n";
        ss << COOL_COLOR_MAUVE << "let mo = moog();" << ANSI_COLOR_RESET
           << " // create an instance of a MiniMoog Synth assigned to var mo\n";
        ss << COOL_COLOR_MAUVE << "let bd = sample(kicks/808kick.aif);"
           << ANSI_COLOR_RESET
           << " // create an instance of an 808 kick sample assigned to var "
              "bd\n";
        ss << COOL_COLOR_MAUVE << "note_on(dx100, 45);" << ANSI_COLOR_RESET
           << " // play an 'A2' on the 'dx100'\n";
        ss << COOL_COLOR_MAUVE << "note_on_at(dx100, 45, 104);"
           << ANSI_COLOR_RESET
           << " // play 'A2' at 104 midi ticks from now()\n";
        ss << ANSI_COLOR_WHITE;
        ss << "######## Array Value generators and modifiers "
              "###################\n";
        ss << COOL_COLOR_MAUVE << "notes_in_key();" << ANSI_COLOR_RESET
           << " // returns array of midi notes in scale of\n";
        ss << "                // current key (see ps for key)\n";
        ss << COOL_COLOR_MAUVE << "play_array(soundgen, array_of_midi_nums);"
           << ANSI_COLOR_RESET << " // plays the array\n";
        ss << "                                          // spaced evenly\n";
        ss << "                                          // over one loop on\n";
        ss << "                                          // 'soundgen'\n";
        ss << "   // e.g. play_array(dx100, notes_in_key());\n";
        ss << COOL_COLOR_MAUVE << "rand_array(array_len, low, high);"
           << ANSI_COLOR_RESET << " // returns array of\n";
        ss << "                                  // array_len, with\n";
        ss << "                                  // rand numbers between\n";
        ss << "                                  //low and high inclusive\n";
        ss << COOL_COLOR_MAUVE << "notes_in_chord(midi_num_root, chord_type);"
           << ANSI_COLOR_RESET << " // chord_type is\n";
        ss << "                                           //0:MAJOR\n";
        ss << "                                           //1:MINOR\n";
        ss << "                                           //2:DIM\n";
        ss << COOL_COLOR_MAUVE << "bjork(n, m);" << ANSI_COLOR_RESET
           << " // return array of bjorklund rhythm with n hits over m slots\n";
        ss << COOL_COLOR_MAUVE << "rand_beat();" << ANSI_COLOR_RESET
           << " // random 16 hit beat, one of shiko, son, rumba,\n";
        ss << "             // soukous, gahu, or bossa-nova\n";
        ss << COOL_COLOR_MAUVE << "riff();" << ANSI_COLOR_RESET
           << " // returns array of 16 hits with random riff taken\n";
        ss << "        // from notes_in_key()\n";
        ss << COOL_COLOR_MAUVE << "map(array_name, func);" << ANSI_COLOR_RESET
           << " // returns array containing each element from array_name after "
              "applying func to their value\n";
        ss << COOL_COLOR_MAUVE << "incr(starting_val, min_val, max_val);"
           << ANSI_COLOR_RESET
           << " // returns number one more than starting_val, modulo max_val "
              "plus min_val\n";
        ss << COOL_COLOR_MAUVE << "push(src_array, val));" << ANSI_COLOR_RESET
           << " // returns a new array, with val appended to copy of "
              "src_array\n";
        ss << COOL_COLOR_MAUVE << "reverse(src);" << ANSI_COLOR_RESET
           << " // returns a (copy of) reversed string or array\n";
        ss << COOL_COLOR_MAUVE << "rotate(src_array, num_pos);"
           << ANSI_COLOR_RESET
           << " // returns a (copy of) src_array, rotated left by num_pos\n";

        ss << ANSI_COLOR_RESET;

        reply = ss.str();
    }
    repl_queue.push(reply);
}

// static void mixer_print_notes()
//{
//    printf("Current KEY is %s. Compat NOTEs are:",
//           key_names[timing_info.key]);
//    // for (int i = 0; i < 6; ++i)
//    //{
//    //    printf("%s ", key_names[compat_keys[key][i]]);
//    //}
//    printf("\n");
//}

void Mixer::EmitEvent(broadcast_event event)
{
    event_queue_item ev;
    ev.type = Event::TIMING_EVENT;
    ev.timing_info = timing_info;
    process_event_queue.push(ev);

    for (int i = 0; i < soundgen_num; ++i)
    {
        std::shared_ptr<SoundGenerator> sg = sound_generators_[i];
        if (sg != NULL)
        {
            sg->eventNotify(event, timing_info);
            if (sg->effects_num > 0)
            {
                for (int j = 0; j < sg->effects_num; j++)
                {
                    if (sg->effects[j])
                    {
                        Fx *f = sg->effects[j];
                        f->EventNotify(event);
                    }
                }
            }
        }
    }
}

void Mixer::UpdateBpm(int new_bpm)
{
    bpm = new_bpm;
    timing_info.frames_per_midi_tick = (60.0 / bpm * SAMPLE_RATE) / PPQN;
    timing_info.loop_len_in_frames = timing_info.frames_per_midi_tick * PPBAR;
    timing_info.loop_len_in_ticks = PPBAR;

    timing_info.ms_per_midi_tick = 60000.0 / (bpm * PPQN);
    timing_info.midi_ticks_per_ms = PPQN / (60000.0 / bpm);

    timing_info.size_of_thirtysecond_note =
        (PPSIXTEENTH / 2) * timing_info.frames_per_midi_tick;
    timing_info.size_of_sixteenth_note =
        timing_info.size_of_thirtysecond_note * 2;
    timing_info.size_of_eighth_note = timing_info.size_of_sixteenth_note * 2;
    timing_info.size_of_quarter_note = timing_info.size_of_eighth_note * 2;

    EmitEvent((broadcast_event){.type = TIME_BPM_CHANGE});
    link_set_bpm(m_ableton_link, bpm);
}

void Mixer::VolChange(float vol)
{
    if (vol >= 0.0 && vol <= 1.0)
    {
        volume = vol;
    }
}

void Mixer::VolChange(int sg, float vol)
{
    if (!IsValidSoundgenNum(sg))
    {
        printf("Nah mate, returning\n");
        return;
    }
    sound_generators_[sg]->SetVolume(vol);
}

void Mixer::PanChange(int sg, float val)
{
    if (!IsValidSoundgenNum(sg))
    {
        printf("Nah mate, returning\n");
        return;
    }
    sound_generators_[sg]->SetPan(val);
}

void Mixer::AddSoundGenerator(std::shared_ptr<SoundGenerator> sg)
{
    if (soundgen_num < MAX_NUM_SOUND_GENERATORS)
    {
        sound_generators_[soundgen_num] = sg;
        audio_reply_queue.push(soundgen_num++);
    }
    else
    {
        std::cerr << "BARF - HIT MAX!\n";
        audio_reply_queue.push(-1);
    }
}

void Mixer::AddMinisynth()
{
    auto ms = std::make_shared<MiniSynth>();
    AddSoundGenerator(ms);
}

void Mixer::AddSample(std::string sample_path)
{
    auto ds = std::make_shared<DrumSampler>(sample_path.data());
    AddSoundGenerator(ds);
}

void Mixer::AddDxsynth()
{
    auto dx = std::make_shared<DXSynth>();
    AddSoundGenerator(dx);
}

void Mixer::AddLooper(std::string filename, bool loop_mode)
{
    auto loopr = std::make_shared<looper>(filename.data(), loop_mode);
    AddSoundGenerator(loopr);
}

void Mixer::MidiTick()
{

    timing_info.is_thirtysecond = false;
    timing_info.is_twentyfourth = false;
    timing_info.is_sixteenth = false;
    timing_info.is_twelth = false;
    timing_info.is_eighth = false;
    timing_info.is_sixth = false;
    timing_info.is_quarter = false;
    timing_info.is_third = false;
    timing_info.is_start_of_loop = false;

    CheckForAudioActionQueueMessages();
    CheckForMidiMessages(); // from external

    if (timing_info.midi_tick % PPBAR == 0)
        timing_info.is_start_of_loop = true;

    if (timing_info.midi_tick % 120 == 0)
    {
        timing_info.is_thirtysecond = true;

        if (timing_info.midi_tick % 240 == 0)
        {
            timing_info.is_sixteenth = true;
            timing_info.sixteenth_note_tick++;

            if (timing_info.midi_tick % 480 == 0)
            {
                timing_info.is_eighth = true;

                if (timing_info.midi_tick % PPQN == 0)
                    timing_info.is_quarter = true;
            }
        }
    }

    // so far only used for ARP engines
    if (timing_info.midi_tick % 160 == 0)
    {
        timing_info.is_twentyfourth = true;

        if (timing_info.midi_tick % 320 == 0)
        {
            timing_info.is_twelth = true;

            if (timing_info.midi_tick % 640 == 0)
            {
                timing_info.is_sixth = true;

                if (timing_info.midi_tick % 1280 == 0)
                    timing_info.is_third = true;
            }
        }
    }

    // std::cout << "Mixer -- midi_tick:" << timing_info.midi_tick
    //          << " 16th:" << timing_info.sixteenth_note_tick
    //          << " Start of Loop:" <<
    //          timing_info.is_start_of_loop
    //          << std::endl;

    repl_queue.push("tick");
    EmitEvent((broadcast_event){.type = TIME_MIDI_TICK});
    // lo_send(processing_addr, "/bpm", NULL);
    CheckForDelayedEvents();
}

bool Mixer::ShouldProgressChords(int tick)
{
    int chance = rand() % 100;
    if (should_progress_chords == false)
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

int Mixer::GenNext(float *out, int frames_per_buffer)
{

    link_update_from_main_callback(m_ableton_link, frames_per_buffer);

    for (int i = 0, j = 0; i < frames_per_buffer; i++, j += 2)
    {
        double output_left = 0.0;
        double output_right = 0.0;

        timing_info.cur_sample++;

        if (preview.enabled)
        {
            stereo_val preview_audio = preview_buffer_generate(&preview);
            output_left += preview_audio.left * 0.6;
            output_right += preview_audio.right * 0.6;
        }

        if (link_is_midi_tick(m_ableton_link, &timing_info, i))
        {
            int current_tick_within_bar = timing_info.midi_tick % PPBAR;
            if (ShouldProgressChords(current_tick_within_bar))
                NextChord();

            MidiTick();
        }

        if (soundgen_num > 0)
        {
            for (int i = 0; i < soundgen_num; i++)
            {
                if (sound_generators_[i] != NULL)
                {
                    soundgen_cur_val[i] = sound_generators_[i]->genNext();
                    output_left += soundgen_cur_val[i].left;
                    output_right += soundgen_cur_val[i].right;
                }
            }
        }

        out[j] = volume * (output_left / 1.53);
        out[j + 1] = volume * (output_right / 1.53);
    }

    return 0;
}

bool Mixer::DelSoundgen(int soundgen_num)
{
    if (IsValidSoundgenNum(soundgen_num))
    {
        printf("MIXR!! Deleting SOUND GEN %d\n", soundgen_num);
        std::shared_ptr<SoundGenerator> sg = sound_generators_[soundgen_num];

        if (active_midi_soundgen_num == soundgen_num)
            active_midi_soundgen_num = -99;

        sound_generators_[soundgen_num] = nullptr;
    }
    return true;
}

bool Mixer::IsValidSoundgenNum(int sg_num)
{
    if (sg_num >= 0 && sg_num < soundgen_num &&
        sound_generators_[sg_num] != NULL)
        return true;
    return false;
}

bool Mixer::IsValidFx(int soundgen_num, int fx_num)
{
    if (IsValidSoundgenNum(soundgen_num))
    {
        std::shared_ptr<SoundGenerator> sg = sound_generators_[soundgen_num];
        if (fx_num >= 0 && fx_num < sg->effects_num && sg->effects[fx_num])
            return true;
    }
    return false;
}

void Mixer::SetNotes()
{
    timing_info.notes[0] = timing_info.key;
    timing_info.notes[1] = (timing_info.key + 2) % NUM_KEYS;  // W step
    timing_info.notes[2] = (timing_info.key + 4) % NUM_KEYS;  // W step
    timing_info.notes[3] = (timing_info.key + 5) % NUM_KEYS;  // H step
    timing_info.notes[4] = (timing_info.key + 7) % NUM_KEYS;  // W step
    timing_info.notes[5] = (timing_info.key + 9) % NUM_KEYS;  // W step
    timing_info.notes[6] = (timing_info.key + 11) % NUM_KEYS; // W step
    timing_info.notes[7] = (timing_info.key + 12) % NUM_KEYS; // H step
}

void Mixer::PrintMidiInfo()
{

    std::stringstream ss;
    ss << "Midi Notes (octave 3):\n";
    ss << "C3:60 C#:61 D:62 D#:63 E:64 F:65 F#:66 G:67 G#:68 A:69 A#:70 B:71\n";
    ss << "(For other octaves, add or subtract 12)\n";

    repl_queue.push(ss.str());
}

void Mixer::PrintTimingInfo()
{
    mixer_timing_info *info = &timing_info;
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
    printf("Start of loop:%d\n", info->is_start_of_loop);
    printf("Is 1/32:%d\n", info->is_thirtysecond);
    printf("Is 1/16:%d\n", info->is_sixteenth);
    printf("Is 1/8:%d\n", info->is_eighth);
    printf("Is 1/4:%d\n", info->is_quarter);
    printf("Is midi_tick:%d\n", info->is_midi_tick);
}

double Mixer::GetHzPerBar()
{

    double hz_per_beat = (60. / bpm);
    return hz_per_beat / 4;
}

double Mixer::GetHzPerTimingUnit(unsigned int timing_unit)
{
    double return_val = 0;
    double hz_per_beat = (60. / bpm);
    if (timing_unit == Q2)
        return_val = hz_per_beat / 2.;
    if (timing_unit == Q4)
        return_val = hz_per_beat;
    else if (timing_unit == Q8)
        return_val = hz_per_beat * 2;
    else if (timing_unit == Q16)
        return_val = hz_per_beat * 4;
    else if (timing_unit == Q32)
        return_val = hz_per_beat * 8;

    return return_val;
}

int Mixer::GetTicksPerCycleUnit(unsigned int event_type)
{
    int ticks = 0;
    switch (event_type)
    {
    case (TIME_START_OF_LOOP_TICK):
        ticks = timing_info.loop_len_in_ticks;
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
void Mixer::SetOctave(int octave)
{
    if (octave > -10 && octave < 10)
        timing_info.octave = octave;
}
void Mixer::SetBarsPerChord(int bars)
{
    if (bars > 0 && bars < 32)
        bars_per_chord = bars;
}

void Mixer::SetKey(unsigned int key)
{
    if (key < NUM_KEYS)
        timing_info.key = key;
}

void Mixer::SetKey(std::string str_key)
{
    str_lower(str_key);

    int key = -1;
    if (str_key == "c#" || str_key == "dm")
        key = 1;
    else if (str_key == "d#" || str_key == "em")
        key = 3;
    else if (str_key == "f#" || str_key == "gm")
        key = 6;
    else if (str_key == "g#" || str_key == "am")
        key = 8;
    else if (str_key == "a#" || str_key == "bm")
        key = 10;
    else if (str_key == "c")
        key = 0;
    else if (str_key == "d")
        key = 2;
    else if (str_key == "e")
        key = 4;
    else if (str_key == "f")
        key = 5;
    else if (str_key == "g")
        key = 7;
    else if (str_key == "a")
        key = 9;
    else if (str_key == "b")
        key = 11;

    if (key != -1)
        SetKey(key);
}

void Mixer::SetChordProgression(unsigned int prog_num)
{
    if (prog_num < NUM_PROGRESSIONS)
    {
        switch (prog_num)
        {
        case (0):
            progression_type = 0;
            prog_len = 3;
            prog_degrees[0] = 0; // I
            prog_degrees[1] = 3; // IV
            prog_degrees[2] = 4; // V
            break;
        case (1):
            progression_type = 1;
            prog_len = 4;
            prog_degrees[0] = 0; // I
            prog_degrees[1] = 4; // V
            prog_degrees[2] = 5; // vi
            prog_degrees[3] = 3; // IV
            break;
        case (2):
            progression_type = 2;
            prog_len = 4;
            prog_degrees[0] = 0; // I
            prog_degrees[1] = 5; // vi
            prog_degrees[2] = 3; // IV
            prog_degrees[3] = 4; // V
            break;
        case (3):
            progression_type = 3;
            prog_len = 4;
            prog_degrees[0] = 5; // vi
            prog_degrees[1] = 1; // ii
            prog_degrees[2] = 4; // V
            prog_degrees[3] = 0; // I
            break;
        }
        prog_degrees_idx = 0;
        timing_info.chord_progression_index = prog_degrees_idx;
        timing_info.chord = prog_degrees[prog_degrees_idx];
    }
}
void Mixer::ChangeChord(unsigned int root, unsigned int chord_type)
{
    if (root < NUM_KEYS && chord_type < NUM_CHORD_TYPES)
    {
        timing_info.chord = root;
        timing_info.chord_type = chord_type;
        EmitEvent((broadcast_event){.type = TIME_CHORD_CHANGE});
    }
}

int Mixer::GetKeyFromDegree(unsigned int scale_degree)
{
    return timing_info.key + scale_degree;
}

void Mixer::PreviewAudio(char *filename)
{
    if (is_valid_file(filename))
    {
        preview.enabled = false;
        preview_buffer_import_file(&preview, filename);
    }
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

void Mixer::EnablePrintMidi(bool b) { midi_print_notes = b; }

void Mixer::CheckForAudioActionQueueMessages()
{
    while (auto action = audio_queue.try_pop())
    {
        if (action)
        {
            if (action->type == AudioAction::STATUS)
                Ps(action->status_all);
            else if (action->type == AudioAction::HELP)
                mixr->Help();
            else if (action->type == AudioAction::MONITOR)
            {
                AddFileToMonitor(action->filepath);
            }
            else if (action->type == AudioAction::ADD)
            {
                switch (action->soundgenerator_type)
                {
                case (MINISYNTH_TYPE):
                    AddMinisynth();
                    break;
                case (DXSYNTH_TYPE):
                    AddDxsynth();
                    break;
                case (LOOPER_TYPE):
                    AddLooper(action->filepath, action->loop_mode);
                    break;
                case (DRUMSAMPLER_TYPE):
                    AddSample(action->filepath);
                    break;
                }
            }
            else if (action->type == AudioAction::ADD_FX)
                interpreter_sound_cmds::ParseFXCmd(action->args);
            else if (action->type == AudioAction::BPM)
                UpdateBpm(action->new_bpm);
            else if (action->type == AudioAction::MIXER_UPDATE)
            {
                if (action->param_name == "key")
                    SetKey(action->param_val);
                else if (action->param_name == "prog")
                    SetChordProgression(
                        std::stoi(action->param_val, nullptr, 0));
            }
            else if (action->type == AudioAction::MIDI_EVENT_ADD ||
                     action->type == AudioAction::MIDI_EVENT_ADD_DELAYED)
            {

                if (IsValidSoundgenNum(action->soundgen_num))
                {
                    auto sg = sound_generators_[action->soundgen_num];

                    for (auto midinum : action->notes)
                    {

                        midi_event event_on =
                            new_midi_event(MIDI_ON, midinum, action->velocity);
                        event_on.source = EXTERNAL_OSC;

                        // used later for MIDI OFF MESSAGE
                        int midi_note_on_time = timing_info.midi_tick;

                        if (action->type == AudioAction::MIDI_EVENT_ADD_DELAYED)
                        {
                            midi_note_on_time += action->note_start_time;

                            event_on.delete_after_use = true;

                            auto ev = DelayedMidiEvent(midi_note_on_time,
                                                       event_on, sg);
                            _action_items.push_back(ev);
                        }
                        else
                        {
                            sg->noteOn(event_on);
                        }
                        int midi_off_tick =
                            midi_note_on_time + action->duration;

                        midi_event event_off =
                            new_midi_event(MIDI_OFF, midinum, action->velocity);

                        event_off.delete_after_use = true;
                        auto ev =
                            DelayedMidiEvent(midi_off_tick, event_off, sg);

                        _action_items.push_back(ev);
                    }
                }
            }
            else if (action->type == AudioAction::UPDATE)
            {
                if (IsValidSoundgenNum(action->mixer_soundgen_idx))
                {
                    double param_val = 0.;

                    if (action->param_val == "sync32")
                        param_val = GetHzPerTimingUnit(Q32);
                    if (action->param_val == "sync16")
                        param_val = GetHzPerTimingUnit(Q16);
                    else if (action->param_val == "sync8")
                        param_val = GetHzPerTimingUnit(Q8);
                    else if (action->param_val == "sync4")
                        param_val = GetHzPerTimingUnit(Q4);
                    else if (action->param_val == "sync2")
                        param_val = GetHzPerTimingUnit(Q2);
                    else
                        param_val = std::stod(action->param_val);

                    auto sg = sound_generators_[action->mixer_soundgen_idx];
                    if (!sg)
                    {
                        std::cerr << "WHOE NELLY! Naw SG! bailing out!\n";
                        return;
                    }
                    if (action->delayed_by > 0)
                    {
                        // TODO - this is hardcoded for pitch - should add an
                        // ENUM to be able to do others
                        midi_event event =
                            new_midi_event(MIDI_PITCHBEND, param_val * 10, 0);
                        event.delete_after_use = true;

                        _action_items.push_back(DelayedMidiEvent(
                            timing_info.midi_tick + action->delayed_by, event,
                            sg));
                        return;
                    }
                    if (action->param_name == "volume")
                        sg->SetVolume(param_val);
                    else if (action->param_name == "pan")
                        sg->SetPan(param_val);
                    else
                    {
                        // first check if we're setting an FX param
                        if (action->fx_id != -1)
                        {
                            int fx_num = action->fx_id;
                            if (IsValidFx(action->mixer_soundgen_idx, fx_num))
                            {
                                Fx *f = sg->effects[fx_num];
                                if (action->param_name == "active")
                                    f->enabled_ = param_val;
                                else
                                    f->SetParam(action->param_name, param_val);
                            }
                        }
                        else // must be a SoundGenerator param
                        {
                            sg->SetParam(action->param_name, param_val);
                        }
                    }
                }
                else
                {
                    // no soundgen - must be mixer
                    float param_val = std::stod(action->param_val);
                    if (action->param_name == "volume")
                        VolChange(param_val);
                }
            }
            else if (action->type == AudioAction ::INFO)
            {
                if (IsValidSoundgenNum(action->mixer_soundgen_idx))
                {
                    auto sg = sound_generators_[action->mixer_soundgen_idx];
                    repl_queue.push(sg->Info());
                }
            }
            else if (action->type == AudioAction ::SAVE_PRESET ||
                     action->type == AudioAction::LOAD_PRESET)
            {
                interpreter_sound_cmds::ParseSynthCmd(action->args);
            }
            else if (action->type == AudioAction::RAND)
            {
                sound_generators_[action->mixer_soundgen_idx]->randomize();
            }
            else if (action->type == AudioAction::PREVIEW)
            {
                char *fname = action->preview_filename.data();
                PreviewAudio(fname);
            }
        }
    }
}

void Mixer::CheckForMidiMessages()
{
    PmEvent msg[32];
    if (Pm_Poll(midi_stream))
    {
        int cnt = Pm_Read(midi_stream, msg, 32);
        for (int i = 0; i < cnt; i++)
        {
            int status = Pm_MessageStatus(msg[i].message);
            int data1 = Pm_MessageData1(msg[i].message);
            int data2 = Pm_MessageData2(msg[i].message);

            if (status == 176)
            {
                if (data1 == 9)
                    SetMidiBank(0);
                if (data1 == 10)
                    SetMidiBank(1);
                if (data1 == 11)
                    SetMidiBank(2);
                if (data1 == 12)
                    SetMidiBank(3);
            }

            if (midi_print_notes)
                printf("[MIDI message] status:%d data1:%d "
                       "data2:%d\n",
                       status, data1, data2);

            if (midi_control_destination != NONE &&
                IsValidSoundgenNum(active_midi_soundgen_num))
            {

                std::shared_ptr<SoundGenerator> sg =
                    sound_generators_[active_midi_soundgen_num];

                midi_event ev = new_midi_event(status, data1, data2);

                ev.source = EXTERNAL_DEVICE;
                ev.delete_after_use = false;

                sg->parseMidiEvent(ev, timing_info);
            }
            else
            {
                printf("Got midi but not connected to "
                       "synth\n");
            }
        }
    }
}

void Mixer::SetMidiBank(int num)
{
    if (num >= 0 && num < 4)
        midi_bank_num = num;
}

void Mixer::SetShouldProgressChords(bool b) { should_progress_chords = b; }

void Mixer::NextChord()
{
    prog_degrees_idx = (prog_degrees_idx + 1) % prog_len;
    timing_info.chord_progression_index = prog_degrees_idx;
    timing_info.chord = prog_degrees[prog_degrees_idx];
}

void Mixer::AddFileToMonitor(std::string filepath)
{
    file_monitors.push_back(file_monitor{.function_file_filepath = filepath});
}

void Mixer::CheckForDelayedEvents()
{
    auto it = _action_items.begin();
    while (it != _action_items.end())
    {
        if (it->target_tick == timing_info.midi_tick)
        {
            // TODO - push to action queue not call function
            if (it->sg)
            {
                it->sg->parseMidiEvent(it->event, timing_info);
            }
            // `erase()` invalidates the iterator, use returned iterator
            it = _action_items.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

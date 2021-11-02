#ifndef LOOPER_H
#define LOOPER_H

#include <envelope_generator.h>
#include <soundgenerator.h>
#include <stdbool.h>
#include <wchar.h>

#define MAX_CONCURRENT_GRAINS 1000
#define MAX_GRAIN_STREAM_LEN_SEC 10 // assuming a loop is never more than 10sec

struct sound_grain_params
{
    int dur;
    int starting_idx;
    int attack_pct;
    int release_pct;
    bool reverse_mode;
    double pitch;
    int num_channels;
    int degrade_by;
    bool debug;
    unsigned int envelope_mode;
};

struct sound_grain
{
    int grain_len_frames;
    int grain_counter_frames;
    int audiobuffer_num;
    int audiobuffer_start_idx;
    int audiobuffer_num_channels;
    double audiobuffer_cur_pos;
    double audiobuffer_pitch;
    double incr;

    int degrade_by;

    int attack_time_pct; // percent of grain_len_frames
    // int attack_time_samples;
    int release_time_pct; // percent of grain_len_frames
    // int release_time_samples;
    bool active;
    double amp;
    // double slope;
    // double curve;
    bool reverse_mode;
    bool debug;

    unsigned int envelope_mode;

    // Parabolic Env vars
    float slope;
    float curve;

    int attack_time_samples;
    int release_time_samples;
    int attack_to_sustain_boundary_sample_idx;
    int sustain_to_decay_boundary_sample_idx;
    float previous_amplitude;

    // Trapezoidal Env vars
    float amplitude_increment;

    // Exponential / Logarithmic
    float exp_min = 0.2;
    float exp_mul = 0;
    float exp_now = 0;

    EnvelopeGenerator eg;
};

enum
{
    GRAIN_SELECTION_STATIC,
    GRAIN_SELECTION_RANDOM,
    GRAIN_NUM_SELECTION_MODES
};

enum
{
    LOOPER_ENV_PARABOLIC,
    LOOPER_ENV_TRAPEZOIDAL,
    LOOPER_ENV_TUKEY_WINDOW,
    LOOPER_ENV_GENERATOR,
    LOOPER_ENV_EXPONENTIAL_CURVE,
    LOOPER_ENV_LOGARITHMIC_CURVE,
    LOOPER_ENV_NUM
};

enum
{
    LOOPER_LOOP_MODE,
    LOOPER_STATIC_MODE,
    LOOPER_SMUDGE_MODE,
    LOOPER_MAX_MODES,
};

enum
{
    LOOPER_EXTERNAL_MODE_FOLLOW,
    LOOPER_EXTERNAL_MODE_CAPTURE_ONCE,
    LOOPER_EXTERNAL_MODE_NUM,
};

class looper : public SoundGenerator
{
  public:
    looper(char *filename, bool loop_mode);
    ~looper();
    stereo_val genNext() override;
    std::string Status() override;
    std::string Info() override;
    void start() override;
    void stop() override;
    void eventNotify(broadcast_event event, mixer_timing_info tinfo) override;
    void noteOn(midi_event ev) override;
    void noteOff(midi_event ev) override;
    void SetParam(std::string name, double val) override;
    double GetParam(std::string name) override;

  public:
    bool started{false};
    bool have_active_buffer{false};

    char filename[512];
    double *audio_buffer{nullptr};
    int num_channels{0};
    int audio_buffer_len{0};
    int size_of_sixteenth{0};
    double audio_buffer_read_idx{0};
    double normalized_audio_buffer_read_idx{0};
    int audio_buffer_write_idx{0};
    int external_source_sg{-1};           // XOR - external or file
    unsigned int external_source_mode{0}; // capture once or follow
    bool recording{false};
    bool record_pending{false}; // wait for start of loop
    pthread_mutex_t extsource_lock;

    int num_active_grains{0};
    int highest_grain_num{0};
    int cur_grain_num{0};
    std::array<sound_grain, MAX_CONCURRENT_GRAINS> m_grains{};

    int granular_spray_frames{0}; // random off-set from starting idx
    int quasi_grain_fudge{0};     // random variation from length of grain
    int grain_duration_ms{0};
    int grains_per_sec{0};
    bool density_duration_sync{false}; // keep duration and per_sec aligned
    double fill_factor{0};             // used for density_duration_sync
    double grain_pitch{1};

    int num_grains_per_looplen{0};
    unsigned int selection_mode{0};
    unsigned int envelope_mode{0};
    double envelope_taper_ratio{0}; // 0.0...1.0
    bool reverse_mode{false};

    int last_grain_launched_sample_time{0};
    int grain_attack_time_pct{0};
    int grain_release_time_pct{0};

    EnvelopeGenerator m_eg1; // start/stop amp
    EnvelopeGenerator m_eg2; // unused so far

    unsigned int loop_mode_{0};
    double loop_len{1}; // bars
    int loop_counter{0};

    bool scramble_pending{false};
    bool scramble_mode{false};
    int scramble_diff{0};

    bool stutter_pending{false};
    bool stutter_mode{false};
    int stutter_idx{0};

    bool step_mode{false};
    int step_diff{0};

    bool stop_pending{false}; // allow eg to stop
    bool gate_mode{false};    // use midi to trigger env amp

    int degrade_by{0}; // percent change to drop bits

    int cur_sixteenth{0}; // used to track scramble

    // TODO - need a reset i would guess
    double incr_speed_{1};
    double cur_midi_idx_{0};

    bool debug_pending{false};
};

void looper_import_file(looper *g, char *filename);
void looper_set_gate_mode(looper *g, bool b);

int looper_calculate_grain_spacing(looper *g);
void looper_set_grain_pitch(looper *g, double pitch);
void looper_set_incr_speed(looper *g, double speed);
void looper_set_grain_duration(looper *g, int dur);
void looper_set_grain_density(looper *g, int gps);
void looper_set_grain_attack_size_pct(looper *g, int att);
void looper_set_grain_release_size_pct(looper *g, int rel);
void looper_set_audio_buffer_read_idx(looper *g, int position);
void looper_set_granular_spray(looper *g, int spray_ms);
void looper_set_quasi_grain_fudge(looper *g, int fudgefactor);
void looper_set_selection_mode(looper *g, unsigned int mode);
void looper_set_envelope_mode(looper *g, unsigned int mode);
void looper_set_reverse_mode(looper *g, bool b);
void looper_set_loop_mode(looper *g, unsigned int m);
void looper_set_loop_len(looper *g, double bars);
void looper_set_scramble_pending(looper *g);
void looper_set_stutter_pending(looper *g);
void looper_set_step_mode(looper *g, bool b);

int looper_get_available_grain_num(looper *g);
int looper_count_active_grains(looper *g);

void sound_grain_init(sound_grain *g, sound_grain_params params);
stereo_val sound_grain_generate(sound_grain *g, double *audio_buffer,
                                int buffer_len);
double sound_grain_env(sound_grain *g, unsigned int envelope_mode);

void looper_del_self(void *self);

void looper_set_fill_factor(looper *l, double fill_factor);
void looper_set_density_duration_sync(looper *l, bool b);
void looper_dump_buffer(looper *l);

void looper_set_grain_env_attack_pct(looper *l, int percent);
void looper_set_grain_env_release_pct(looper *l, int percent);
void looper_set_grain_external_source_mode(looper *l, unsigned int mode);
void looper_set_degrade_by(looper *l, int degradation);
void looper_set_trace_envelope(looper *l);

#endif // LOOPER

#pragma once

#include <stdbool.h>

#include "modmatrix.h"
#include "synthfunctions.h"

#define EG_MINTIME_MS 1 // these two used for attacjtime, decay and release
#define EG_MAXTIME_MS 5000
#define EG_DEFAULT_STATE_TIME 100

enum
{
    ANALOG,
    DIGITAL
};

enum
{
    OFFF, // name clash in defjams
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE,
    SHUTDOWN
};

class EnvelopeGenerator
{

  public:
    EnvelopeGenerator() = default;
    ~EnvelopeGenerator() = default;

    ModulationMatrix *modmatrix{nullptr};
    GlobalEgParams *global_eg_params{nullptr};

    ///////// MOD MATRIXXXXXX....
    unsigned m_mod_source_eg_attack_scaling{DEST_NONE};
    unsigned m_mod_source_eg_decay_scaling{DEST_NONE};
    unsigned m_mod_source_sustain_override{DEST_NONE};

    unsigned m_mod_dest_eg_output{SOURCE_NONE};
    unsigned m_mod_dest_eg_biased_output{SOURCE_NONE};

    /////////////////////////////////////////

    bool m_sustain_override{false};
    bool m_release_pending{false};

    unsigned m_eg_mode{0}; // enum above, analog or digital
    // special modes
    bool m_reset_to_zero{false};
    bool m_legato_mode{false};
    bool m_output_eg{false}; // i.e. this instance is going direct to output,
                             // rather than into an intermediatery
    bool ramp_mode{false};   // used for no sustain

    // double m_eg1_osc_intensity;
    double m_envelope_output{0};

    double m_attack_coeff{0};
    double m_attack_offset{0};
    double m_attack_tco{0};

    double m_decay_coeff{0};
    double m_decay_offset{0};
    double m_decay_tco{0};

    double m_release_coeff{0};
    double m_release_offset{0};
    double m_release_tco{0};

    double m_attack_time_msec{EG_DEFAULT_STATE_TIME};
    double m_decay_time_msec{EG_DEFAULT_STATE_TIME};
    double m_release_time_msec{EG_DEFAULT_STATE_TIME};

    double m_shutdown_time_msec{10.};

    double m_sustain_level{0.7};

    double m_attack_time_scalar{1.}; // for velocity -> attack time mod
    double m_decay_time_scalar{1.};  // for note# -> decay time mod

    double m_inc_shutdown{0};

    // enum above
    unsigned int m_state{OFFF};

  public:
    unsigned int GetState();
    bool IsActive();

    bool CanNoteOff();

    void Reset();

    void SetEgMode(unsigned int mode);

    void CalculateAttackTime();
    void CalculateDecayTime();
    void CalculateReleaseTime();

    void NoteOff();

    void Shutdown();

    void SetState(unsigned int state);

    void SetAttackTimeMsec(double time);
    void SetDecayTimeMsec(double time);
    void SetReleaseTimeMsec(double time);
    void SetShutdownTimeMsec(double time_msec);

    void SetSustainLevel(double level);
    void SetSustainOverride(bool b);

    void StartEg();
    void StopEg();

    void InitGlobalParameters(GlobalEgParams *params);

    void Update();
    double DoEnvelope(double *p_biased_output);

    void Release();
};

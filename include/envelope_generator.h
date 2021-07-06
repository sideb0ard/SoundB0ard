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
    EnvelopeGenerator();
    ~EnvelopeGenerator() = default;

    ModulationMatrix *modmatrix{nullptr};
    GlobalEgParams *global_eg_params{nullptr};

    unsigned m_mod_source_eg_attack_scaling;
    unsigned m_mod_source_eg_decay_scaling;
    unsigned m_mod_source_sustain_override;

    unsigned m_mod_dest_eg_output;
    unsigned m_mod_dest_eg_biased_output;

    /////////////////////////////////////////

    bool drum_mode; // no sustain
    bool m_sustain_override;
    bool m_release_pending;

    unsigned m_eg_mode; // enum above, analog or digital
    // special modes
    bool m_reset_to_zero;
    bool m_legato_mode;
    bool m_output_eg; // i.e. this instance is going direct to output, rather
                      // than into an intermediatery

    // double m_eg1_osc_intensity;
    double m_envelope_output;

    double m_attack_coeff;
    double m_attack_offset;
    double m_attack_tco;

    double m_decay_coeff;
    double m_decay_offset;
    double m_decay_tco;

    double m_release_coeff;
    double m_release_offset;
    double m_release_tco;

    double m_attack_time_msec;
    double m_decay_time_msec;
    double m_release_time_msec;

    double m_shutdown_time_msec;

    double m_sustain_level;

    double m_attack_time_scalar; // for velocity -> attack time mod
    double m_decay_time_scalar;  // for note# -> decay time mod

    double m_inc_shutdown;

    // enum above
    unsigned int m_state;

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
    void SetDrumMode(bool b);
};

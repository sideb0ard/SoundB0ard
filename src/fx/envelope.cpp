#include <stdio.h>
#include <stdlib.h>

#include <defjams.h>
#include <fx/envelope.h>
#include <mixer.h>

extern Mixer *mixr;

const char *s_eg_state[] = {"OFF",     "ATTACK",  "DECAY",
                            "SUSTAIN", "RELEASE", "SHUTDOWN"};

const char *s_eg_type[] = {"ANALOG", "DIGITAL"};
const char *s_eg_mode[] = {"TRIGGER", "SUSTAIN"};

Envelope::Envelope()
{
    Reset();

    type_ = ENVELOPE;
    enabled_ = true;
}

void Envelope::Reset()
{
    eg_.m_state = SUSTAIN;
    eg_.SetAttackTimeMsec(300);
    eg_.SetDecayTimeMsec(300);
    eg_.SetReleaseTimeMsec(300);
    SetLengthBars(1);
    debug_ = false;
}

void Envelope::Status(char *status_string)
{
    snprintf(status_string, MAX_STATIC_STRING_SZ,
             "ENV state:%s len_bars:%.2f len_ticks:%d type:%s mode:%s\n"
             " attack:%.2f decay:%.2f sustain:%.2f release:%.2f // "
             "release_tick:%d\n"
             "debug:%d",
             s_eg_state[eg_.m_state], env_length_bars_, env_length_ticks_,
             s_eg_type[eg_.m_eg_mode], s_eg_mode[env_mode_],
             eg_.m_attack_time_msec, eg_.m_decay_time_msec, eg_.m_sustain_level,
             eg_.m_release_time_msec, release_tick_, debug_);
}

stereo_val Envelope::Process(stereo_val input)
{
    double env_out = eg_.DoEnvelope(NULL);
    input.left *= env_out;
    input.right *= env_out;
    return input;
}

void Envelope::SetLengthBars(double length_bars)
{
    if (length_bars > 0)
    {
        env_length_bars_ = length_bars;
        CalculateTimings();
    }
}

void Envelope::CalculateTimings()
{
    mixer_timing_info info = mixr->timing_info;
    // wtf?! - i've no idea why i need to divide by 2 here -
    // obviously i'm crock at math!
    int release_time_ticks =
        (eg_.m_release_time_msec / info.ms_per_midi_tick) / 2;

    if (debug_)
    {
        printf("RELtimeMS:%.2f // ms per tick:%.2f // rel time ticks:%d\n",
               eg_.m_release_time_msec, info.ms_per_midi_tick,
               release_time_ticks);
    }

    env_length_ticks_ = info.loop_len_in_ticks * env_length_bars_;
    int release_tick = env_length_ticks_ - release_time_ticks;
    if (release_tick > 0)
        release_tick_ = release_tick;
    else
    {
        printf("Barfed on yer envelope -- not enuff runway.\n");
    }

    env_length_ticks_counter_ = info.midi_tick % env_length_ticks_;
}

void Envelope::EventNotify(broadcast_event event)
{
    switch (event.type)
    {
    case (TIME_BPM_CHANGE):
        CalculateTimings();
        break;
    case (TIME_MIDI_TICK):

        (env_length_ticks_counter_)++;

        if (env_length_ticks_counter_ >= env_length_ticks_)
        {
            eg_.StartEg();
            env_length_ticks_counter_ = 0;
        }
        else if (eg_.m_state == SUSTAIN &&
                 env_length_ticks_counter_ >= release_tick_)
        {
            eg_.NoteOff();
        }

        if (eg_.m_state != eg_state_)
        {
            eg_state_ = eg_.m_state;
            if (debug_)
                printf("NEW STATE:%s tick:%d\n", s_eg_state[eg_state_],
                       env_length_ticks_counter_);
        }

        break;
    }
}

void Envelope::SetType(unsigned int type) { eg_.SetEgMode(type); }

void Envelope::SetMode(unsigned int mode)
{
    if (mode < 2)
        env_mode_ = mode;
}

void Envelope::SetAttackMs(double val)
{
    eg_.SetAttackTimeMsec(val);
    CalculateTimings();
}

void Envelope::SetDecayMs(double val)
{
    eg_.SetDecayTimeMsec(val);
    CalculateTimings();
}

void Envelope::SetSustainLvl(double val) { eg_.SetSustainLevel(val); }

void Envelope::SetReleaseMs(double val)
{
    eg_.SetReleaseTimeMsec(val);
    CalculateTimings();
}

void Envelope::SetDebug(bool b) { debug_ = b; }
void Envelope::SetParam(std::string name, double val) {}
double Envelope::GetParam(std::string name) { return 0; }

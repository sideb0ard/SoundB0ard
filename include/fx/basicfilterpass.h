#pragma once

#include <stdbool.h>

#include <defjams.h>
#include <fx/filter_moogladder.h>
#include <fx/fx.h>
#include <lfo.h>

enum
{
    LOWPASS,
    HIGHPASS,
    ALLPASS,
    BANDPASS
};

class Filterpass : Fx
{
  public:
    Filterpass();
    ~Filterpass();
    void Status(char *string) override;
    stereo_val Process(stereo_val input) override;
    void EventNotify(broadcast_event event) override;
    void SetParam(std::string name, double val) override;
    double GetParam(std::string name) override;

  private:
    void SetLfoActive(int lfo_num, bool b);
    void SetLfoRate(int lfo_num, double val);
    void SetLfoAmp(int lfo_num, double val);
    void SetLfoType(int lfo_num, unsigned int type);

  private:
    filter_moogladder m_filter_;

    lfo m_lfo1_; // route to freq
    bool m_lfo1_active_;

    lfo m_lfo2_; // route to qv
    bool m_lfo2_active_;
};

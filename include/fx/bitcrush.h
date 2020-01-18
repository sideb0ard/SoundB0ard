#pragma once

#include <defjams.h>
#include <fx/fx.h>

class Bitcrush : Fx
{
    Filterpass();
    ~Filterpass();
    void Status(char *string) override;
    stereo_val Process(stereo_val input) override;
    void EventNotify(broadcast_event event) override;
    void SetParam(std::string name, double val) override;
    double GetParam(std::string name) override;
    fx m_fx;
    int bitdepth;
    int bitrate;
    double sample_hold_freq;
    double step;
    double inv_step;
    double phasor;
    double last;
} bitcrush;

bitcrush *new_bitcrush(void);
void bitcrush_init(bitcrush *bc);

void bitcrush_status(void *self, char *string);
stereo_val bitcrush_process_audio(void *self, stereo_val input);

void bitcrush_set_bitdepth(bitcrush *bc, int val);
void bitcrush_set_bitrate(bitcrush *bc, int val);
void bitcrush_set_sample_hold_freq(bitcrush *bc, double val);
void bitcrush_update(bitcrush *bc);

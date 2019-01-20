#pragma once

#include "afx/delay.h"
#include "afx/delayapf.h"
#include "afx/lpfcombfilter.h"
#include "afx/onepolelpf.h"

#include "envelope_detector.h"
#include "fx.h"
#include <defjams.h>

typedef struct dynamics_processor
{
    fx m_fx; // API

    envelope_detector m_left_detector;
    envelope_detector m_right_detector;

    delay m_left_delay;
    delay m_right_delay;

    double m_inputgain_db;
    double m_threshold;
    double m_attack_ms;
    double m_release_ms;
    double m_ratio;
    double m_outputgain_db;
    double m_knee_width;
    double m_lookahead_delay_ms;
    unsigned int m_stereo_link;    // on, off
    unsigned int m_processor_type; // comp, limit, expand, gate
    unsigned int m_time_constant;  // digital, analog
    int m_external_source; // a sound_generator id that will correspond to mixer
                           // input cache

} dynamics_processor;

enum
{
    COMP,
    LIMIT,
    EXPAND,
    GATE
};

dynamics_processor *new_dynamics_processor(void);
void dynamics_processor_init(dynamics_processor *dp);

double dynamics_processor_calc_compression_gain(double detector_val,
                                                double threshold, double rratio,
                                                double kneewidth, bool limit);

double dynamics_processor_calc_downward_expander_gain(double detector_val,
                                                      double threshold,
                                                      double rratio,
                                                      double kneewidth,
                                                      bool gate);

void dynamics_processor_status(void *self, char *status_string);
stereo_val dynamics_processor_process(void *self, stereo_val input);

void dynamics_processor_set_inputgain_db(dynamics_processor *dp, double val);
void dynamics_processor_set_threshold(dynamics_processor *dp, double val);
void dynamics_processor_set_attack_ms(dynamics_processor *dp, double val);
void dynamics_processor_set_release_ms(dynamics_processor *dp, double val);
void dynamics_processor_set_ratio(dynamics_processor *dp, double val);
void dynamics_processor_set_outputgain_db(dynamics_processor *dp, double val);
void dynamics_processor_set_knee_width(dynamics_processor *dp, double val);
void dynamics_processor_set_lookahead_delay_ms(dynamics_processor *dp,
                                               double val);
void dynamics_processor_set_stereo_link(dynamics_processor *dp,
                                        unsigned int val);
void dynamics_processor_set_processor_type(dynamics_processor *dp,
                                           unsigned int val);
void dynamics_processor_set_time_constant(dynamics_processor *dp,
                                          unsigned int val);
void dynamics_processor_set_external_source(dynamics_processor *dp,
                                            unsigned int val);
void dynamics_processor_set_default_sidechain_params(dynamics_processor *dp);

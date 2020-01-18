#pragma once

#include "afx/combfilter.h"
#include "afx/delay.h"
#include "afx/delayapf.h"
#include "afx/lpfcombfilter.h"
#include "afx/onepolelpf.h"
#include "fx.h"

typedef struct reverb
{
    fx m_fx; // API

    // pre-delay block
    delay m_pre_delay;

    // input diffusion
    one_pole_lpf m_input_lpf;
    delay_apf m_input_apf_1;
    delay_apf m_input_apf_2;

    // parrallel comb bank 1
    comb_filter m_parallel_cf_1;
    comb_filter m_parallel_cf_2;
    lpf_comb_filter m_parallel_cf_3;
    lpf_comb_filter m_parallel_cf_4;

    // parrallel comb bank 2
    comb_filter m_parallel_cf_5;
    comb_filter m_parallel_cf_6;
    lpf_comb_filter m_parallel_cf_7;
    lpf_comb_filter m_parallel_cf_8;

    // damping
    one_pole_lpf m_damping_lpf_1;
    one_pole_lpf m_damping_lpf_2;

    // output diffusion
    delay_apf m_output_apf_3;
    delay_apf m_output_apf_4;
    // cooking with gas

    // GUI ///////////////////////////
    double m_pre_delay_msec;
    double m_pre_delay_atten_db;
    // reverb output
    double m_rt60;
    double m_wet_pct;
    // input diffusion
    double m_input_lpf_g;
    double m_lpf2_g2;
    // APF -- all pass filter
    double m_apf_1_delay_msec;
    double m_apf_1_g;
    double m_apf_2_delay_msec;
    double m_apf_2_g;
    double m_apf_3_delay_msec;
    double m_apf_3_g;
    double m_apf_4_delay_msec;
    double m_apf_4_g;
    // COMBover
    double m_comb_1_delay_msec;
    double m_comb_2_delay_msec;
    double m_comb_3_delay_msec;
    double m_comb_4_delay_msec;
    double m_comb_5_delay_msec;
    double m_comb_6_delay_msec;
    double m_comb_7_delay_msec;
    double m_comb_8_delay_msec;

} reverb;

reverb *new_reverb(void);
void reverb_init_reverb(reverb *r);
void reverb_cook_variables(reverb *r);

stereo_val reverb_process_audio(void *self, stereo_val in);

void reverb_status(void *self, char *string);

void reverb_set_pre_delay_msec(reverb *r, double val);
void reverb_set_pre_delay_atten_db(reverb *r, double val);
void reverb_set_rt60(reverb *r, double val);
void reverb_set_wet_pct(reverb *r, double val);
void reverb_set_input_lpf_g(reverb *r, double val);
void reverb_set_lpf2_g2(reverb *r, double val);
void reverb_set_apf_delay_msec(reverb *r, int apf_num, double val);
void reverb_set_apf_g(reverb *r, int apf_num, double val);
void reverb_set_comb_delay_msec(reverb *r, int comb_num, double val);

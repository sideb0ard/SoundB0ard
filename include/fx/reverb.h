#pragma once

#include <afx/combfilter.h>
#include <afx/delay.h>
#include <afx/delayapf.h>
#include <afx/lpfcombfilter.h>
#include <afx/onepolelpf.h>
#include <fx/fx.h>

class Reverb : Fx
{
  public:
    Reverb();
    void Status(char *string) override;
    stereo_val Process(stereo_val input) override;
    void SetParam(std::string name, double val) override;
    double GetParam(std::string name) override;

  private:
    void Init();
    void CookVariables();

    void SetPreDelayMsec(double val);
    void SetPreDelayAttenDb(double val);
    void SetRt60(double val);
    void SetWetPct(double val);
    void SetInputLpfG(double val);
    void SetLpf2G2(double val);
    void SetApfDelayMsec(int apf_num, double val);
    void SetApfG(int apf_num, double val);
    void SetCombDelayMsec(int comb_num, double val);

  private:
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
};

#pragma once

#include <stdbool.h>

#include <defjams.h>
#include <fx/fx.h>

class WaveShaper : Fx
{
  public:
    WaveShaper();
    void Status(char *string) override;
    stereo_val Process(stereo_val input) override;
    void SetParam(std::string name, double val) override;
    double GetParam(std::string name) override;

  private:
    void Init();
    double ProcessAudio(double input);
    void SetArcTanKPos(double val);
    void SetArcTanKNeg(double val);
    void SetStages(unsigned int val);
    void SetInvertStages(unsigned int val);

  private:
    double m_arc_tan_k_pos;       // 0.10 - 20
    double m_arc_tan_k_neg;       // 0.10 - 20
    unsigned int m_stages;        // 1 - 10
    unsigned int m_invert_stages; // OFF, ON
};

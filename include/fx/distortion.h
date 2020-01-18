#pragma once

#include <defjams.h>
#include <fx/fx.h>

class Distortion : Fx
{
    Distortion();
    ~Distortion();
    void Status(char *string) override;
    stereo_val Process(stereo_val input) override;
    void EventNotify(broadcast_event event) override;
    void SetParam(std::string name, double val) override;
    double GetParam(std::string name) override;

  private:
    void Init();
    void SetThreshold(double val);

  private:
    double m_threshold_;
};

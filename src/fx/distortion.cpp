#include <stdlib.h>

#include <math.h>

#include <fx/distortion.h>
#include <mixer.h>

extern mixer *mixr;

Distortion::Distortion()
{
    type_ = DISTORTION;
    enabled_ = true;

    m_threshold_ = 0.707;
}

void Distortion::Status(char *status_string)
{
    snprintf(status_string, MAX_STATIC_STRING_SZ, "Distortion! threshold:%.2f",
             m_threshold_);
}

stereo_val Distortion::Process(stereo_val input)
{
    stereo_val out = {};

    if (input.left >= 0)
        out.left = fmin(input.left, m_threshold_);
    else
        out.left = fmax(input.left, -m_threshold_);
    out.left /= m_threshold_;

    if (input.right >= 0)
        out.right = fmin(input.right, m_threshold_);
    else
        out.right = fmax(input.right, -m_threshold_);
    out.right /= m_threshold_;

    return out;
}

void Distortion::SetParam(std::string name, double val) {}
double Distortion::GetParam(std::string name) { return 0; }

void Distortion::SetThreshold(double val)
{
    if (val >= 0.01 && val <= 1.0)
        m_threshold_ = val;
    else
        printf("Val must be between 0.01 and 1\n");
}

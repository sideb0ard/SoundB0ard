#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <fx/bitcrush.h>
#include <utils.h>

BitCrush::BitCrush()
{
    Init();
    type_ = BITCRUSH;
    enabled_ = true;
}

void BitCrush::Status(char *status_string)
{
    snprintf(status_string, MAX_STATIC_STRING_SZ,
             "bitdepth:%d bitrate:%d sample_hold_freq:%.2f", bitdepth_,
             bitrate_, sample_hold_freq_);
}

stereo_val BitCrush::Process(stereo_val input)
{
    phasor_ += sample_hold_freq_;
    if (phasor_ >= 1)
    {
        phasor_ -= 1;
        last_ = step_ * floor((input.left * inv_step_) + 0.5);
    }

    input.left = last_;
    input.right = last_;
    return input;
}

void BitCrush::SetParam(std::string name, double val) {}

double BitCrush::GetParam(std::string name) { return 0; }

void BitCrush::Init()
{
    bitdepth_ = 6;
    bitrate_ = 4096;
    sample_hold_freq_ = 1;
    phasor_ = 0;
    last_ = 0;
    Update();
}

void BitCrush::Update()
{
    step_ = 2 * fast_pow(0.5, bitdepth_);
    inv_step_ = 1 / step_;
}

void BitCrush::SetBitdepth(int val)
{
    if (val >= 1 && val <= 16)
    {
        bitdepth_ = val;
        Update();
    }
    else
        printf("Val must be between 1 and 16:%d\n", val);
}

void BitCrush::SetSampleHoldFreq(double val)
{
    val = clamp(0, 1, val);
    sample_hold_freq_ = val;
    Update();
}

void BitCrush::SetBitrate(int val)
{
    if (val >= 200 && val <= SAMPLE_RATE)
    {
        bitrate_ = val;
        Update();
    }
    else
        printf("Val must be between 200 and %d\n", SAMPLE_RATE);
}

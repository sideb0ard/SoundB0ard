#pragma once

#include <defjams.h>
#include <pattern_parser/ast.hpp>

enum ArpDirection
{
    ARP_UP,
    ARP_DOWN,
    ARP_UPDOWN,
    ARP_RAND,
    ARP_REPEAT,
};

enum ArpSpeed
{
    ARP_16,
    ARP_8,
    ARP_4,
};

class PatternFunction
{
  public:
    PatternFunction() = default;
    virtual ~PatternFunction() = default;
    virtual std::string String() const = 0;
    virtual void TransformPattern(
        std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
        int loop_num, mixer_timing_info tinfo) = 0;
    bool active_{true};
};

class PatternEvery : public PatternFunction
{
  public:
    PatternEvery(int every_n, std::shared_ptr<PatternFunction> func)
        : every_n_{every_n}, func_{func}
    {
    }
    void TransformPattern(
        std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
        int loop_num, mixer_timing_info tinfo) override;
    std::string String() const override;

  private:
    int every_n_;
    std::shared_ptr<PatternFunction> func_;
};

class PatternReverse : public PatternFunction
{
  public:
    PatternReverse() = default;
    void TransformPattern(
        std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
        int loop_num, mixer_timing_info tinfo) override;
    std::string String() const override;
};

class PatternRotate : public PatternFunction
{
  public:
    PatternRotate(unsigned int direction, int num_sixteenth_steps)
        : direction_{direction}, num_sixteenth_steps_{num_sixteenth_steps} {};
    void TransformPattern(
        std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
        int loop_num, mixer_timing_info tinfo) override;
    std::string String() const override;

  private:
    unsigned int direction_;
    int num_sixteenth_steps_;
};

class PatternTranspose : public PatternFunction
{
  public:
    PatternTranspose(unsigned int direction, int num_octaves)
        : direction_{direction}, num_octaves_{num_octaves} {};
    void TransformPattern(
        std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
        int loop_num, mixer_timing_info tinfo) override;
    std::string String() const override;

  private:
    unsigned int direction_;
    int num_octaves_;
};

class PatternSwing : public PatternFunction
{
  public:
    PatternSwing(int swing_setting) : swing_setting_{swing_setting} {};
    void TransformPattern(
        std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
        int loop_num, mixer_timing_info tinfo) override;
    std::string String() const override;

  private:
    int swing_setting_;
};

class PatternMask : public PatternFunction
{
  public:
    PatternMask(std::string mask);
    void TransformPattern(
        std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
        int loop_num, mixer_timing_info tinfo) override;
    std::string String() const override;

  private:
    std::string mask_;
    uint16_t bin_mask_{0};
};

class PatternArp : public PatternFunction
{
  public:
    PatternArp(){};
    void TransformPattern(
        std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
        int loop_num, mixer_timing_info tinfo) override;
    std::string String() const override;

  public:
    int last_midi_note_{0};
    ArpDirection direction_{ArpDirection::ARP_UP};
    ArpSpeed speed_{ArpSpeed::ARP_16};
};

class PatternBrak : public PatternFunction
{
  public:
    PatternBrak(){};
    void TransformPattern(
        std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
        int loop_num, mixer_timing_info tinfo) override;
    std::string String() const override;

  public:
};

class PatternFast : public PatternFunction
{
  public:
    PatternFast(){};
    void TransformPattern(
        std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
        int loop_num, mixer_timing_info tinfo) override;
    std::string String() const override;

  public:
};

class PatternSlow : public PatternFunction
{
  public:
    PatternSlow(){};
    void TransformPattern(
        std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
        int loop_num, mixer_timing_info tinfo) override;
    std::string String() const override;

  public:
};

class PatternChord : public PatternFunction
{
  public:
    PatternChord(){};
    void TransformPattern(
        std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
        int loop_num, mixer_timing_info tinfo) override;
    std::string String() const override;

  public:
};

class PatternPowerChord : public PatternFunction
{
  public:
    PatternPowerChord(){};
    void TransformPattern(
        std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
        int loop_num, mixer_timing_info tinfo) override;
    std::string String() const override;

  public:
};

class PatternScramble : public PatternFunction
{
  public:
    PatternScramble(){};
    void TransformPattern(
        std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
        int loop_num, mixer_timing_info tinfo) override;
    std::string String() const override;

  public:
};

class PatternBump : public PatternFunction
{
  public:
    PatternBump(){};
    void TransformPattern(
        std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
        int loop_num, mixer_timing_info tinfo) override;
    std::string String() const override;

  public:
    int bump_amount_{PPSIXTEENTH / 2}; // range - 0 to PPSIXTEENTH (260)
};

class PatternSpeed : public PatternFunction
{
  public:
    PatternSpeed(double speed) : speed_multiplier{speed} {};
    void TransformPattern(
        std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
        int loop_num, mixer_timing_info tinfo) override;
    std::string String() const override;

  public:
    double speed_multiplier{0};
    double cur_index{0};
};

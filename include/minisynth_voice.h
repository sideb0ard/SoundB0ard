#pragma once

#include <filter_ckthreefive.h>
#include <filter_moogladder.h>
#include <filter_onepole.h>
#include <filter_sem.h>

#include <qblimited_oscillator.h>
#include <voice.h>
#include <wt_oscillator.h>

typedef enum
{
    Saw3,
    Sqr3,
    Saw2Sqr,
    Tri2Saw,
    Tri2Sqr,
    Sin2Sqr,
    MAX_VOICE_CHOICE
} minisynth_voice_choice;

struct MiniSynthVoice : Voice
{
    MiniSynthVoice();
    ~MiniSynthVoice() = default;

    QBLimitedOscillator m_op1;
    QBLimitedOscillator m_op2;
    QBLimitedOscillator m_op3;
    QBLimitedOscillator m_op4;
    // WTOscillator m_op1;
    // WTOscillator m_op2;
    // WTOscillator m_op3;
    // WTOscillator m_op4;

    // MoogLadder m_filter;
    CKThreeFive m_filter;
    // FilterSem m_filter;

    void InitGlobalParameters(GlobalSynthParams *sp) override;
    void InitializeModMatrix(ModulationMatrix *matrix) override;
    void PrepareForPlay() override;
    void Update() override;
    void Reset() override;

    bool DoVoice(double *left_output, double *right_output) override;
    void SetFilterMod(double mod);
};

#pragma once
#include "oscillator.h"

#define WT_LENGTH 1024
#define NUM_TABLES 9

struct WTOscillator : public Oscillator
{
    WTOscillator();
    ~WTOscillator() = default;
    double m_read_idx;
    double m_wt_inc;

    double m_read_idx2;
    double m_wt_inc2;

    double m_sine_table[WT_LENGTH];
    double *m_saw_tables[NUM_TABLES]{nullptr};
    double *m_tri_tables[NUM_TABLES]{nullptr};

    double *m_current_table{nullptr};
    int m_current_table_idx;

    // correction factor
    double m_square_corr_factor[NUM_TABLES];

    void StartOscillator() override;
    void StopOscillator() override;
    void Reset() override;
    void Update() override;
    double DoOscillate(double *quad_output) override;

    double DoWaveTable(double *read_idx, double wt_inc);
    double DoSquareWave();
    double DoSquareWaveCore(double *read_idx, double wt_inc);

    void SelectTable();
    int GetTableIndex();

    void CreateWaveTables();
    void DestroyWaveTables();
};

#pragma once
#include <array>

#include "oscillator.h"

constexpr unsigned kWavetableLength = 1024;
constexpr unsigned kNumTables = 9;

struct WTOscillator : public Oscillator {
  WTOscillator();
  virtual ~WTOscillator();
  double m_read_idx{};
  double m_wt_inc{};

  double m_read_idx2{};
  double m_wt_inc2{};

  double m_sine_table[kWavetableLength]{};
  double* m_saw_tables[kNumTables]{};
  double* m_tri_tables[kNumTables]{};

  double* m_current_table{m_sine_table};
  int m_current_table_idx{};

  // correction factor
  std::array<double, kNumTables> m_square_corr_factor{};

  void StartOscillator() override;
  void StopOscillator() override;
  void Reset() override;
  void Update() override;
  double DoOscillate(double* quad_output) override;

  double DoWaveTable(double& read_idx, double wt_inc);
  double DoSquareWave();
  double DoSquareWaveCore(double& read_idx, double wt_inc);

  void SelectTable();
  int GetTableIndex();

  void CreateWaveTables();
  void DestroyWaveTables();

  inline void CheckWrapIndex(double& dIndex) {
    while (dIndex < 0.0) dIndex += kWavetableLength;
    while (dIndex >= kWavetableLength) dIndex -= kWavetableLength;
  }
};

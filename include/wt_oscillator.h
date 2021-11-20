#pragma once
#include <array>

#include "oscillator.h"

constexpr unsigned kWavetableLength = 16384;
constexpr unsigned kNumTables = 9;

struct WTOscillator : public Oscillator {
  WTOscillator();
  ~WTOscillator() = default;
  double m_read_idx;
  double m_wt_inc;

  double m_read_idx2;
  double m_wt_inc2;

  std::array<double, kWavetableLength> m_sine_table{};
  std::array<std::array<double, kWavetableLength>, kNumTables> m_saw_tables{};
  std::array<std::array<double, kWavetableLength>, kNumTables> m_tri_tables{};

  std::array<double, kWavetableLength> &m_current_table{m_sine_table};
  int m_current_table_idx;

  // correction factor
  std::array<double, kNumTables> m_square_corr_factor{};

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

  constexpr void CreateWaveTables();
};

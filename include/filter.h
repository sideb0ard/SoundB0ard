#pragma once

#include "defjams.h"
#include "modmatrix.h"
#include "synthfunctions.h"

// 46.88.. = semitones between frequencies (80, 18000.0) / 2
// taken from Will Pirkle book 'designing software synths..'
#define FILTER_FC_MOD_RANGE 46.881879936465680
#define FILTER_FC_MIN 80         // 80Hz
#define FILTER_FC_MAX 18000      // 18 kHz
#define FILTER_FC_DEFAULT 10000  // 10kHz
#define FILTER_Q_DEFAULT 0.707   // butterworth (noidea!)
#define FILTER_Q_MOD_RANGE 10    // dunno if this will work!
#define FILTER_TYPE_DEFAULT LPF4

struct Filter {
  Filter() = default;
  Filter(unsigned int ftype) : m_filter_type{ftype} {};
  virtual ~Filter() = default;

  ModulationMatrix *modmatrix{nullptr};
  GlobalFilterParams *global_filter_params{nullptr};

  // sources
  unsigned m_mod_source_fc{DEST_NONE};
  unsigned m_mod_source_fc_control{DEST_NONE};

  // GUI controls
  double m_fc_control{FILTER_FC_DEFAULT};  // filter cut-off
  double m_q_control{1};                   // 'qualvity factor' 1-10
  double m_aux_control{0};  // a spare control, used in SEM and ladder filters

  unsigned m_nlp{1};        // Non Linear Processing on/off switch
  double m_saturation{10};  // used in NLP

  unsigned m_filter_type{LPF1};

  double m_fc{FILTER_FC_DEFAULT};  // current filter cut-off val
  double m_q{FILTER_Q_DEFAULT};    // current q value
  double m_fc_mod{0};              // frequency cutoff modulation input

  virtual void SetFcMod(double d);
  virtual void SetQControl(double d);
  virtual void SetFcControl(double val);
  virtual void Update();
  virtual void Reset();
  virtual double DoFilter(double xn) = 0;

  void SetType(unsigned int type);
  void InitGlobalParameters(GlobalFilterParams *params);
};

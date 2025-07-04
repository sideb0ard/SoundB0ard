#include "filter.h"

#include <stdlib.h>

#include <iostream>

#include "defjams.h"
#include "utils.h"

void Filter::SetFcControl(double val) {
  if (val >= FILTER_FC_MIN && val <= FILTER_FC_MAX) {
    m_fc_control = val;
    //  Update();
  } else {
    std::cout << "FC must be between " << FILTER_FC_MIN << " and "
              << FILTER_FC_MAX << std::endl;
  }
}

void Filter::SetQControlGUI(double val) {
  if (val >= 1 && val <= 10) {
    m_q_control = val;
    // Update();
  } else {
    std::cout << "Q must be between " << 1 << " and " << 10 << std::endl;
  }
}

void Filter::SetFcMod(double val) {
  m_fc_mod = val;
}

void Filter::SetType(unsigned int type) {
  if (type < NUM_FILTER_TYPES) {
    m_filter_type = type;
    // Update();
  } else {
    std::cout << "Type must be between 0 and " << NUM_FILTER_TYPES - 1
              << std::endl;
  }
}

void Filter::Update() {
  if (global_filter_params) {
    m_aux_control = global_filter_params->aux_control;
    m_fc_control = global_filter_params->fc_control;
    m_q_control = global_filter_params->q_control;
    m_saturation = global_filter_params->saturation;
    m_filter_type = global_filter_params->filter_type;
    m_nlp = global_filter_params->nlp;
  }

  if (modmatrix) {
    m_fc_mod = modmatrix->destinations[m_mod_source_fc];
    if (modmatrix->destinations[m_mod_source_fc_control] > 0)
      m_fc_control = modmatrix->destinations[m_mod_source_fc_control];
  }
  SetQControl(m_q_control);

  m_fc = m_fc_control * pitch_shift_multiplier(m_fc_mod);

  if (m_fc > FILTER_FC_MAX) m_fc = FILTER_FC_MAX;
  if (m_fc < FILTER_FC_MIN) m_fc = FILTER_FC_MIN;
}

void Filter::InitGlobalParameters(GlobalFilterParams *params) {
  global_filter_params = params;
  global_filter_params->aux_control = m_aux_control;
  global_filter_params->fc_control = m_fc_control;
  global_filter_params->q_control = m_q_control;
  global_filter_params->saturation = m_saturation;
  global_filter_params->filter_type = m_filter_type;
  global_filter_params->nlp = m_nlp;
}

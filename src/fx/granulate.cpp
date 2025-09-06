#include "granulate.h"

#include <mixer.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <sstream>

Granulate::Granulate() {
  type_ = fx_type::GRANULATE;
  enabled_ = true;
}

void Granulate::SetWetMix(double wet_mix) {
  if (wet_mix >= 0 && wet_mix <= 1) {
    m_wet_mix_ = wet_mix;
    Update();
  }
}

void Granulate::Update() {}

std::string Granulate::Status() {
  std::stringstream ss;
  ss << " mix:" << m_wet_mix_;
  return ss.str();
}

StereoVal Granulate::Process(StereoVal input) {
  StereoVal output = {};
  return output;
}

void Granulate::SetParam(std::string name, double val) {
  if (name == "mix") SetWetMix(val);
}

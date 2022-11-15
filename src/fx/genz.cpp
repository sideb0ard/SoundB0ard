#include <fx/genz.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

#include <sstream>

GenZ::GenZ() {
  Init();
  enabled_ = true;
}

std::string GenZ::Status() {
  std::stringstream ss;
  ss << "some setting:" << some_setting_;

  return ss.str();
}

stereo_val GenZ::Process(stereo_val input) {
  // do something
  return input;
}

void GenZ::SetParam(std::string name, double val) {
  if (name == "some_setting") {
    // update it;
    // Update();
  }
}

void GenZ::Init() {
  some_setting_ = 0;
  Update();
}

void GenZ::Update() { some_setting_ = 0; }

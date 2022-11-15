#include <fx/template.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

#include <sstream>

Template::Template() {
  Init();
  enabled_ = true;
}

std::string Template::Status() {
  std::stringstream ss;
  ss << "some setting:" << some_setting_;

  return ss.str();
}

stereo_val Template::Process(stereo_val input) {
  // do something
  return input;
}

void Template::SetParam(std::string name, double val) {
  if (name == "some_setting") {  // update it}
    Update();
  }
}

void Template::Init() {
  some_setting_ = 0;
  Update();
}

void Template::Update() { some_setting_ = 0; }

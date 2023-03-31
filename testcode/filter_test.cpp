#include <iomanip>
#include <iostream>

#include "include/defjams.h"
// # #include "include/filter_moogladder.h"
#include "include/qblimited_oscillator.h"

int main() {
  QBLimitedOscillator osc;
  // MoogLadder filter;
  //
  osc.m_note_on = true;
  osc.m_osc_fo = 1;
  osc.Update();
  osc.StartOscillator();

  while (!osc.CheckWrapModulo()) {
    double normout = osc.DoOscillate(nullptr);
    std::cout << normout << std::endl;
  }
}

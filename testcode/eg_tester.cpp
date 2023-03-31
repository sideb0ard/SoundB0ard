#include <iomanip>
#include <iostream>

#include "include/defjams.h"
#include "include/envelope_generator.h"

int main() {
  EnvelopeGenerator eg;

  eg.SetRampMode(true);
  eg.StartEg();
  eg.SetAttackTimeMsec(2);
  eg.SetDecayTimeMsec(0);
  eg.SetSustainLevel(1);
  eg.SetReleaseTimeMsec(20);
  while (eg.GetState() != OFFF) {
    auto env_val = eg.DoEnvelope(nullptr);
    std::cout << std::fixed << std::setprecision(10) << env_val << std::endl;
  }
}

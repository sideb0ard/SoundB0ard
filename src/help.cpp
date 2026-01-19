#include "help.h"

#include <stdio.h>

#include "defjams.h"

void print_help() {
  printf("\n=== SoundB0ard Help ===\n\n");

  printf("Documentation:\n");
  printf("  USER_GUIDE.md    - Complete user guide with examples\n");
  printf("  FX_REFERENCE.md  - Quick reference for all effects\n");
  printf("  CLAUDE.md        - Project context and architecture\n\n");

  printf("Quick Start:\n");
  printf("  let drums = drum();              // Create drum machine\n");
  printf("  load_preset(drums, \"TR808\");     // Load preset\n");
  printf("  note_on(drums, 0);               // Trigger kick\n");
  printf("  fx(drums, distort);              // Add distortion\n");
  printf("  setparam drums:distortion_mode 2;  // Tube saturation\n\n");

  printf("Sound Generators:\n");
  printf("  drum()       - Drum synthesizer (9 voices)\n");
  printf("  minisynth()  - Subtractive synthesizer\n");
  printf("  dxsynth()    - FM synthesizer (6 operators)\n");
  printf("  granular()   - Granular sampler\n");
  printf("  sample(path) - Load audio sample\n\n");

  printf("Effects:\n");
  printf("  distort      - Multi-mode distortion (4 algorithms)\n");
  printf("  lofi         - Lo-fi crusher (bit depth + sample rate)\n");
  printf("  sculptor     - Waveform manipulation (was geometer)\n");
  printf("  diffuser     - Multi-stage diffusion (was nnirror)\n");
  printf("  reverb       - Reverb\n");
  printf("  delay        - Stereo delay\n");
  printf("  moddelay     - Modulated delay (chorus/flanger)\n");
  printf("  compressor   - Dynamics compression\n\n");

  printf("Commands:\n");
  printf("  bpm(tempo)              - Set tempo\n");
  printf("  run(\"file.sb\")          - Run script file\n");
  printf("  info(instrument)        - Show all parameters\n");
  printf("  list_presets()          - List available presets\n");
  printf("  p31 # comp_name;        - Assign computation to process\n\n");

  printf("Example Beats:\n");
  printf("  run(\"SBTraxx/AUTECHRE_808.sb\")      - Glitchy polyrhythms\n");
  printf("  run(\"SBTraxx/DILLA_DRUNK_SWING.sb\") - J Dilla style\n");
  printf("  run(\"SBTraxx/PREMIER_BOOM_BAP.sb\")  - DJ Premier style\n\n");

  printf("For more help:\n");
  printf("  Read USER_GUIDE.md for comprehensive documentation\n");
  printf("  Read FX_REFERENCE.md for effects parameters\n");
  printf("  Use info(instrument) to see all available parameters\n\n");
}

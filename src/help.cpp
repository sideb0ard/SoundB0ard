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
  printf("  fmsynth()    - FM synthesizer (6 operators)\n");
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

  printf(
      "========================================================================"
      "====\n");
  printf("                    Built-In Functions Reference\n");
  printf(
      "========================================================================"
      "====\n\n");

  printf("ARRAY/COLLECTION FUNCTIONS\n");
  printf(
      "------------------------------------------------------------------------"
      "\n");
  printf("  len(arr|str|map)      - Returns the length/size\n");
  printf("  head(arr)             - Returns the first element\n");
  printf("  tail(arr)             - Returns all elements except the first\n");
  printf("  last(arr)             - Returns the last element\n");
  printf("  push(arr, item)       - Appends item to array\n");
  printf("  take_n(arr, n)        - Returns first n elements\n");
  printf("  take_random_n(arr, n) - Returns n random unique elements\n");
  printf("  reverse(arr)          - Reverses the order\n");
  printf("  rotate(arr, n)        - Rotates elements by n positions\n");
  printf("  sort(arr)             - Sorts in ascending order\n");
  printf("  shuffle(arr)          - Randomly shuffles elements\n");
  printf("  is_array(val)         - Returns true if value is an array\n");
  printf("  is_in(arr, item)      - Returns true if item is found\n");
  printf("  keys(map)             - Returns array of all keys\n");
  printf("  invert(arr)           - Inverts 0s and 1s (for rhythms)\n\n");

  printf("MATH FUNCTIONS\n");
  printf(
      "------------------------------------------------------------------------"
      "\n");
  printf("  floor(num)            - Returns largest integer <= num\n");
  printf("  abs(num)              - Returns absolute value\n");
  printf("  sin(num), cos(num)    - Trig functions (radians)\n");
  printf("  max(a, b), min(a, b)  - Returns larger/smaller value\n");
  printf("  incr(num, min, max)   - Increments, wrapping at max\n");
  printf("  dincr(num, min, max)  - Drunk walk (+1 or -1 randomly)\n");
  printf("  scale(val, in_min, in_max, out_min, out_max) - Map range\n\n");

  printf("RANDOM/GENERATIVE FUNCTIONS\n");
  printf(
      "------------------------------------------------------------------------"
      "\n");
  printf("  rand(min, max)        - Random number in range\n");
  printf("  rand_array(size, min, max) - Array of random numbers\n");
  printf("  perlin(x)             - Perlin noise value (smooth random)\n");
  printf("  bjork(pulses, steps)  - Euclidean rhythm pattern\n");
  printf("  gen_perc(len, density) - Random percussion pattern\n\n");

  printf("MUSIC THEORY FUNCTIONS\n");
  printf(
      "------------------------------------------------------------------------"
      "\n");
  printf("  midi_ref()            - Map of note names to MIDI numbers\n");
  printf("  notes_in_key(root, scale) - MIDI notes in key/scale\n");
  printf("  notes_in_chord(root, type) - MIDI notes in chord\n");
  printf("  scale_note(note, key, scale) - Quantize note to scale\n");
  printf("  algoz()               - Available scale/mode names\n");
  printf("  ratioz()              - Available chord types\n\n");

  printf("SOUND GENERATOR CONTROL\n");
  printf(
      "------------------------------------------------------------------------"
      "\n");
  printf("  note_on(gen, note, vel) - Trigger note\n");
  printf("  note_on_at(gen, note, time, vel=, dur=) - Schedule note\n");
  printf("  note_off(gen, note)   - Stop note\n");
  printf("  stop(gen)             - Stop all notes\n");
  printf("  solo(gen), unsolo()   - Solo/unsolo generator\n\n");

  printf("PRESET MANAGEMENT\n");
  printf(
      "------------------------------------------------------------------------"
      "\n");
  printf("  list_presets(gen)     - List available presets\n");
  printf("  load_preset(gen, name) - Load named preset\n");
  printf("  rand_preset(gen)      - Load random preset\n");
  printf("  save_preset(gen, name) - Save current settings\n\n");

  printf("MIXER/ROUTING FUNCTIONS\n");
  printf(
      "------------------------------------------------------------------------"
      "\n");
  printf("  send(effect, gen)     - Send generator to effect\n");
  printf("  add_fx(gen, fx_name)  - Add effect to generator\n");
  printf("  monitor(filepath)     - Monitor file for live coding\n\n");

  printf("SCHEDULING\n");
  printf(
      "------------------------------------------------------------------------"
      "\n");
  printf("  sched(delay, start, end, time, action) - Parameter automation\n\n");

  printf("UTILITY\n");
  printf(
      "------------------------------------------------------------------------"
      "\n");
  printf("  print(val)            - Print value to console\n");
  printf("  funcz()               - List environment variables\n");
  printf("  type(val)             - Returns the type of value\n\n");

  printf("For more help:\n");
  printf("  Type 'help' to see this again\n");
  printf("  Type 'funcz()' to see current environment\n");
  printf("  Use info(instrument) to see all parameters\n\n");
}

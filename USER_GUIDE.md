# SBShell User Guide

Yo, Terminal Punk,
    you are now about to witness the strength of street knowledge.

SoundB0ard - a Unix-style shell for making music! This guide will take you from your first sound to complex live-coded performances.

---

## 0. Where am i?

You're goto command is `ps`. In Unix this would be 'process status', for SBShell, it's more like 'program status' - it shows you mixer stats like volume and BPM; it shows you environment variables, which can be standard objects like numbers, strings and booleans, and also sound generator objects like FMSynth, MiniSynth, DrumSynth, or Sampler; and it shows the running Processes. More about all of that below..

## 1. Quick Start / First Sounds

Let's make some noise! Start SoundB0ard and type these commands at the `SB#>` prompt:

```javascript
// whats going on?
ps

// you'll see various synths already created in the environment.
// such as dx, dx2 and dx2, sbdrum and more.

// Trigger a kick drum (note 0)
note_on(sbdrum, 0);

// Try a clap (note 1)
note_on(sbdrum, 3);

// play a long bass note (midi 20 - G#)
note_on(dx, 20, dur = 5000);
```


```javascript
// Load a classic preset
load_preset(sbdrum, "TR808");
note_on(sbdrum, 0);

// Try different velocities (0-127)
note_on(sbdrum, 1, vel = 127);  // Loud snare
note_on(sbdrum, 1, vel = 40);   // Quiet ghost note

```

---

## 2. Sound Generators

SoundB0ard has four main types of sound generators:

### Drum Machine - drum()
Synthesized drums with 9 voices: kick, snare, closed hat, clap, open hat, 3 FM toms, and a laser zap.

```javascript
let drums = drum();
load_preset(drums, "TR808");   // Classic 808
load_preset(drums, "TR909");   // Punchy 909
load_preset(drums, "DILLA");   // Warm, lo-fi

// Drum voices (MIDI note numbers):
// 0 = Kick
// 1 = Snare
// 2 = Closed Hi-Hat
// 3 = Clap
// 4 = Open Hi-Hat
// 5-7 = FM Toms
// 8 = Laser
```

### FM Synth - fmsynth()
6-operator FM synthesis for bells, bass, keys, and experimental sounds.

```javascript
let dx1 = fmsynth();
load_preset(dx1, "BASS");
note_on(dx1, 36);  // C1 - deep bass note
note_on(dx1, 60);  // C3 - middle C
```

### Subtractive Synth - minisynth()
Classic analog-style synthesis with oscillators and filters.

```javascript
let synth = minisynth();
load_preset(synth, "PAD");
note_on(synth, 48, vel = 100, dur = 2000);  // Long pad note
```

### Granular Sampler - granular()
Granular synthesis engine for texture and atmospheric sounds.

```javascript
let grain = granular();
// Load a sample and granularize it
setparam grain:grain_size 100;
setparam grain:density 20;
note_on(grain, 60);
```

---

## 3. Basic Interaction

### Playing Notes

```javascript
// Basic syntax
note_on(instrument, note_number);

// With velocity (0-127, default 100)
note_on(instrument, note_number, vel = 80);

// With duration in milliseconds (default 100)
note_on(instrument, note_number, dur = 500);

// With both
note_on(instrument, note_number, vel = 110, dur = 250);
```

### Changing Parameters

Every sound generator has dozens of parameters you can tweak:

```javascript
let drums = drum();

// Set a parameter
setparam drums:bd_vol 1.0;        // Kick volume
setparam drums:bd_decay 200;      // Kick decay time
setparam drums:bd_pitch_env_range 12;  // Pitch sweep depth

// Get current value
getparam drums:bd_vol;

// See all available parameters
info(drums);
```

### Working with Presets

```javascript
// Load a built-in preset
load_preset(drums, "TR909");

// Save your tweaked settings
save_preset(drums, "MY_KICKS");

// Load your custom preset
load_preset(drums, "MY_KICKS");

// List available presets
list_presets();
```

---

## 4. Timing & Sync

SoundB0ard runs on a global clock synced via Ableton Link.

### BPM and Tempo

```javascript
// Set tempo (also syncs with other Link-enabled apps)
bpm(120);

// Check current BPM
bpm();
```

### Beat Divisions and pp

The magic variable `pp` means "pulses per step" - it represents one 16th note in the current tempo:

```javascript
// At 120 BPM:
// pp ≈ 125ms (one 16th note)
// pp * 2 = 8th note
// pp * 4 = quarter note
// pp * 16 = one bar

// Schedule notes in musical time
note_on_at(drums, 0, 0);        // On the 1
note_on_at(drums, 1, pp * 4);   // Beat 2
note_on_at(drums, 0, pp * 8);   // Beat 3
note_on_at(drums, 1, pp * 12);  // Beat 4
```

### Scheduling Notes with note_on_at()

While `note_on()` plays immediately, `note_on_at()` schedules notes at specific times:

```javascript
// Syntax: note_on_at(instrument, note, time_in_ms, vel, dur)
note_on_at(drums, 0, 0);           // Now
note_on_at(drums, 1, pp * 4);      // One beat later
note_on_at(drums, 2, pp * 2);      // Half beat later

// With swing (offset timing)
let swing = 15;  // milliseconds
note_on_at(drums, 2, pp * 1 + swing);
note_on_at(drums, 2, pp * 3 - swing);
```

### Ableton Link

SoundB0ard automatically syncs with other apps via Ableton Link:

```javascript
// Just set the BPM and Link handles the rest
bpm(128);

// Check Link status
link_status();
```

---

## 5. Samples

Load and play audio samples from the `wavs/` directory:

### Loading Samples

```javascript
// Load a sample
let kick = sample(bd/kick8.aif);
let snare = sample(sd/2snare.aif);
let vocal = sample(vox/yeah.wav);

// Samples are organized in directories:
// bd/   - bass drums
// sd/   - snares
// cp/   - claps
// ch/   - closed hats
// oh/   - open hats
// perc/ - percussion
// vox/  - vocals
// noises/ - sound effects
```

### Playing Samples

```javascript
// Trigger a sample
note_on(kick, 1);  // Note number usually 1 for samples

// Control playback
vol kick 0.8;
pan kick 0.2;   // Pan right (range -1.0 to 1.0)

// Pitch shifting
set kick:pitch 1.5 at = 0;   // 1.5x speed (higher pitch)
set kick:pitch 0.5 at = 0;   // 0.5x speed (lower pitch)
```

### Layering Samples with Synths

Combine samples and synths for richer sounds:

```javascript
let drums = drum();
load_preset(drums, "TR808");
let kick_sample = sample(bd/wuk77.aiff);

// Layer them
note_on_at(drums, 0, 0);           // 808 kick
note_on_at(kick_sample, 1, 5);     // Add sample 5ms later for thickness
```

---

## 6. Patterns & Arrays

Patterns are the heart of rhythm programming in SoundB0ard.

### Basic Pattern Arrays

```javascript
// 16-step kick pattern (1 = hit, 0 = rest)
let kicks = [1, 0, 0, 0,  0, 0, 1, 0,  0, 0, 0, 0,  1, 0, 0, 0];

// Classic 2 and 4 snare
let snares = [0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0];

// Hi-hat 8ths
let hats = [1, 0, 1, 0,  1, 0, 1, 0,  1, 0, 1, 0,  1, 0, 1, 0];
```

### Using Patterns in Loops

```javascript
let drums = drum();
let kicks = [1, 0, 0, 0,  0, 0, 1, 0,  0, 0, 0, 0,  1, 0, 0, 0];

for (let i = 0; i < 16; i++) {
  if (kicks[i] == 1) {
    note_on_at(drums, 0, i * pp);
  }
}
```

### Velocity Patterns

```javascript
// Human-feeling velocity variations
let vels = [110, 95, 105, 100, 90, 108];
let vx = 0;

for (let i = 0; i < 16; i++) {
  if (kicks[i] == 1) {
    note_on_at(drums, 0, i * pp, vel = vels[vx]);
    vx = incr(vx, 0, len(vels));  // Cycle through velocities
  }
}
```

### Timing Offset Patterns (Swing)

```javascript
// J Dilla-style drunk swing
let swings = [0, -18, 12, -25, 15, -22, 18, -28, 10, -20, 16, -24];

for (let i = 0; i < 16; i++) {
  if (kicks[i] == 1) {
    note_on_at(drums, 0, i * pp + swings[i]);  // Add timing offset
  }
}
```

### Pattern Manipulation Functions

```javascript
// Euclidean rhythm generator
let pattern = bjork(5, 16);  // 5 hits distributed over 16 steps
// Returns: [1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0]

// Array functions
len(pattern);          // Get length
pattern[3];            // Access element
append(arr, value);    // Add to array
```

---

## 7. Computations (Live Coding)

Computations (`comp()`) are where the magic happens - they let you create evolving, generative patterns.

### Basic Structure

```javascript
let my_comp = comp()
{
  setup()
  {
    // Initialize variables (runs once)
    let pattern = [1, 0, 1, 0,  1, 0, 1, 0];
    let counter = 0;
  }
  run()
  {
    // Executes every bar (runs repeatedly)
    print("Bar:", counter);
    counter++;
  }
}
```

### Simple Drum Pattern

```javascript
let drums = drum();
load_preset(drums, "TR808");

let kick_comp = comp()
{
  setup()
  {
    let kicks = [1, 0, 0, 0,  0, 0, 1, 0,  0, 0, 0, 0,  1, 0, 0, 0];
  }
  run()
  {
    for (let i = 0; i < 16; i++) {
      if (kicks[i] == 1) {
        note_on_at(drums, 0, i * pp, vel = 100);
      }
    }
  }
}
```

### Pattern Switching

```javascript
let snare_comp = comp()
{
  setup()
  {
    // Multiple pattern variations
    let s1 = [0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0];
    let s2 = [0, 0, 0, 0,  1, 0, 0, 1,  0, 0, 0, 0,  1, 0, 0, 0];
    let s3 = [0, 0, 0, 0,  1, 0, 1, 0,  0, 0, 0, 0,  1, 0, 1, 1];
  }
  run()
  {
    let spat = s1;  // Default pattern

    // Switch patterns based on count
    if (count % 4 == 3) {
      spat = s2;  // Every 4th bar
    }
    if (count % 8 == 7) {
      spat = s3;  // Every 8th bar (fill)
    }

    for (let i = 0; i < 16; i++) {
      if (spat[i] == 1) {
        note_on_at(drums, 1, i * pp, vel = 100 + rand(20));
      }
    }
  }
}
```

### The count Variable

Every computation automatically has a `count` variable that increments each bar:

```javascript
run()
{
  print("This is bar:", count);

  // Do different things on different bars
  if (count % 8 == 0) {
    print("New 8-bar section!");
  }

  if (count == 16) {
    // Drop the drums!
  }
}
```

### Randomization and Evolution

```javascript
let evolving_comp = comp()
{
  setup()
  {
    let base_pattern = [1, 0, 0, 0,  1, 0, 0, 0];
    let variation = 0;
  }
  run()
  {
    // Every 4 bars, add more randomness
    if (count % 4 == 0) {
      variation = variation + 5;
    }

    for (let i = 0; i < 16; i++) {
      if (base_pattern[i % 8] == 1) {
        // Randomly offset timing
        let offset = rand(variation) - variation/2;
        note_on_at(drums, 0, i * pp + offset);
      }
    }
  }
}
```

---

## 8. Mixing & Routing

### Volume and Panning

```javascript
let drums = drum();
let bass = dxsynth();
let pad = minisynth();

// Set volumes (0.0 to 1.0+)
vol drums 0.9;
vol bass 0.7;
vol pad 0.4;

// Pan in stereo field (-1.0 = left, 0 = center, 1.0 = right)
pan drums 0.0;    // Center
pan bass -0.3;    // Slightly left
pan pad 0.4;      // Right
```

### Process IDs and Pattern Assignment

When you start computations, assign them to process IDs (p1-p99) to control them:

```javascript
let kick_comp = comp() { /* ... */ }
let snare_comp = comp() { /* ... */ }
let hats_comp = comp() { /* ... */ }

// Assign to processes
p31 # kick_comp;   // Start on process 31
p32 # snare_comp;  // Start on process 32
p33 # hats_comp;   // Start on process 33

// Stop a process
p32 # "";          // Stop snares

// Restart it
p32 # snare_comp;  // Bring snares back
```

**Important:** Process IDs (p1, p2, p31, etc.) are reserved - don't use them as variable names!

### Master Arrangement Computation

```javascript
let main_comp = comp()
{
  setup()
  {
    let bar = 0;
  }
  run()
  {
    // Build up arrangement over time
    if (bar == 0) {
      p31 # kick_comp;
      p32 # snare_comp;
    }
    if (bar == 4) {
      p33 # hats_comp;
    }
    if (bar == 8) {
      p34 # bass_comp;
    }
    if (bar == 16) {
      // Drop everything except kick
      p32 # "";
      p33 # "";
      p34 # "";
    }
    if (bar == 20) {
      // Bring it all back
      p32 # snare_comp;
      p33 # hats_comp;
      p34 # bass_comp;
    }

    print("Section bar:", bar);
    bar++;
  }
}
```

---

## 9. Effects

SoundB0ard has a powerful FX system for sound design and creative processing.

### Adding Effects to Sound Generators

Use the `fx()` command to add effects to any instrument:

```javascript
let synth = minisynth();

// Add effects to the chain
fx(synth, distort);    // Add distortion
fx(synth, reverb);     // Add reverb
fx(synth, delay);      // Add delay

// Multiple effects process in order
```

### Saturation & Distortion

#### Distortion (Multi-Mode)

The distortion effect now has **4 different algorithms**:

```javascript
let drums = drum();
fx(drums, distort);

// Mode 0: HARD_CLIP - Brick wall clipping (classic)
setparam drums:distortion_mode 0;
setparam drums:distortion_threshold 0.5;  // 0.01-1.0
setparam drums:distortion_drive 2.0;      // 1.0-10.0 (input gain)

// Mode 1: SOFT_CLIP - Smooth tanh saturation
setparam drums:distortion_mode 1;
setparam drums:distortion_drive 3.5;  // Warm, musical saturation

// Mode 2: TUBE - Asymmetric tube-style warmth
setparam drums:distortion_mode 2;
setparam drums:distortion_drive 2.5;  // Vintage tube sound

// Mode 3: FOLDBACK - Wavefold/foldback distortion
setparam drums:distortion_mode 3;
setparam drums:distortion_drive 5.0;  // Extreme, gnarly tones
```

#### Waveshaper

Arctan-based waveshaping for smooth saturation:

```javascript
fx(synth, waveshape);
setparam synth:waveshaper_k_pos 5.0;    // Positive curve (0.1-20)
setparam synth:waveshaper_k_neg 3.0;    // Negative curve (0.1-20)
setparam synth:waveshaper_stages 2;     // Number of stages (1-10)
setparam synth:waveshaper_invert 1;     // Invert stages (0 or 1)
```

### Degradation / Lo-Fi

#### LoFiCrusher

Unified bit depth and sample rate reduction (replaces bitcrush/decimate):

```javascript
let synth = minisynth();
fx(synth, lofi);  // Or: bitcrush, decimate, lofi_crusher

// Bit depth reduction (quantization)
setparam synth:lofi_crusher_bitdepth 6;  // 1-16 bits (lower = more crushed)

// Sample rate reduction (sample & hold)
setparam synth:lofi_crusher_sample_hold_freq 0.3;  // 0.0-1.0 (1.0 = full rate)

// Destructive quantization (adds harmonics)
setparam synth:lofi_crusher_destruct 0.5;  // 0.0-1.0 (0 = clean, 1 = destroyed)
```

**Examples:**
```javascript
// Subtle lo-fi warmth
setparam synth:lofi_crusher_bitdepth 12;
setparam synth:lofi_crusher_sample_hold_freq 0.9;

// Extreme bit crushing
setparam synth:lofi_crusher_bitdepth 3;
setparam synth:lofi_crusher_sample_hold_freq 0.2;
setparam synth:lofi_crusher_destruct 0.8;
```

### Time-Based Effects

#### Delay

```javascript
fx(synth, delay);
setparam synth:stereodelay_time_left 250;   // Left delay in ms
setparam synth:stereodelay_time_right 375;  // Right delay in ms
setparam synth:stereodelay_feedback 40;     // Feedback % (0-99)
setparam synth:stereodelay_wetmix 0.3;      // Dry/wet mix (0.0-1.0)
```

#### Modulated Delay (Chorus/Flanger)

```javascript
fx(synth, moddelay);
setparam synth:moddelay_time 100;        // Base delay time (ms)
setparam synth:moddelay_feedback 30;     // Feedback %
setparam synth:moddelay_mod_depth 5;     // Modulation depth
setparam synth:moddelay_mod_rate 2.0;    // Modulation rate (Hz)
```

#### Reverb

```javascript
fx(synth, reverb);
setparam synth:reverb_roomsize 0.7;   // Room size (0.0-1.0)
setparam synth:reverb_damping 0.5;    // High frequency damping
setparam synth:reverb_wet 0.3;        // Wet/dry mix
setparam synth:reverb_width 1.0;      // Stereo width
```

#### Transverb

Advanced delay/reverb hybrid:

```javascript
fx(synth, transverb);
setparam synth:transverb_time 500;       // Delay time (ms)
setparam synth:transverb_feedback 50;    // Feedback %
setparam synth:transverb_diffusion 0.6;  // Diffusion amount
```

### Creative / Experimental

#### WaveformSculptor

Landmark-based waveform manipulation (was Geometer):

```javascript
fx(synth, sculptor);  // Or: geometer, waveform_sculptor

// Window size for processing
setparam synth:waveform_sculptor_window_size 1024;  // 256-4096 samples

// Landmark generation style
setparam synth:waveform_sculptor_landmark_style 0;  // 0-4
  // 0 = ExtNCross (extremities & zero-crossings)
  // 1 = Span (amplitude-based spacing)
  // 2 = DyDx (derivative/slope-based)
  // 3 = Freq (regular frequency)
  // 4 = Random

// Waveform recreation style
setparam synth:waveform_sculptor_interp_style 0;  // 0-5
  // 0 = Polygon (straight lines, lo-fi)
  // 1 = Wrongygon (backwards lines, harsh)
  // 2 = Sing (sine waves, tonal)
  // 3 = Reversi (reverse intervals)
  // 4 = Smoothie (smooth curves)
  // 5 = Pulse (just pulses)

// Wet/dry mix
setparam synth:waveform_sculptor_wet_mix 0.5;  // 0.0-1.0
```

#### Diffuser

Multi-stage diffusion/blur/reverb (was Nnirror):

```javascript
fx(pad, diffuser);  // Or: nnirror

// Core parameters
setparam pad:diffuser_wet 0.5;           // Wet/dry mix
setparam pad:diffuser_size 0.7;          // Size/length
setparam pad:diffuser_feedback 0.4;      // Feedback amount
setparam pad:diffuser_unison 0.3;        // Unison voices

// Diffusion stages (0.0-1.0 each)
setparam pad:diffuser_diffuse0 0.6;
setparam pad:diffuser_diffuse1 0.4;
setparam pad:diffuser_diffuse2 0.2;
setparam pad:diffuser_diffuse3 0.1;
```

### Dynamics

#### Compressor

```javascript
fx(synth, compressor);
setparam synth:compressor_threshold -10;    // Threshold (dB)
setparam synth:compressor_ratio 4.0;        // Compression ratio
setparam synth:compressor_attack 10;        // Attack (ms)
setparam synth:compressor_release 100;      // Release (ms)
setparam synth:compressor_knee_width 6;     // Knee width (dB)
```

### Filters

#### Modulated Filter

```javascript
fx(synth, modfilter);
setparam synth:modfilter_cutoff 1000;      // Cutoff frequency (Hz)
setparam synth:modfilter_resonance 2.0;    // Resonance/Q
setparam synth:modfilter_mod_depth 500;    // Modulation depth (Hz)
setparam synth:modfilter_mod_rate 2.0;     // Modulation rate (Hz)
```

### Effect Chains

Stack multiple effects for complex sound design:

```javascript
let lead = dxsynth();
load_preset(lead, "BASS");

// Build effect chain
fx(lead, distort);      // 1. Add grit
fx(lead, moddelay);     // 2. Add movement
fx(lead, reverb);       // 3. Add space

// Configure each effect
setparam lead:distortion_mode 2;        // Tube saturation
setparam lead:distortion_drive 3.0;
setparam lead:moddelay_time 150;
setparam lead:moddelay_mod_depth 10;
setparam lead:reverb_roomsize 0.5;
setparam lead:reverb_wet 0.2;
```

### Per-Voice Effects (Drum Synth)

Drum synth voices have built-in distortion and delay:

```javascript
let drums = drum();

// Kick drum with distortion
setparam drums:bd_use_distortion 1;
setparam drums:bd_distortion_threshold 0.5;

// Snare with tempo-synced delay
setparam drums:sd_use_delay 1;
setparam drums:sd_delay_sync_tempo 1;     // Enable tempo sync
setparam drums:sd_delay_sync_len 2;       // 0=16th, 1=8th, 2=quarter, 3=half
setparam drums:sd_delay_feedback_pct 30;
setparam drums:sd_delay_wetmix 0.4;
```

---

## 10. Control Flow & Programming

SoundB0ard has a full programming language built in.

### Variables

```javascript
// Declare with let
let x = 5;
let name = "kick";
let pattern = [1, 0, 1, 0];

// Variable scope:
// - Top-level: global, persists across commands
// - Inside setup(): persists for that computation
// - Inside run(): local to that bar
```

**Reserved words:** Don't use keywords (let, if, for, etc.) or process IDs (p1, p2, p31, etc.) as variable names!

### Conditionals

```javascript
if (count % 4 == 0) {
  print("New 4-bar section");
}

if (count < 8) {
  // Intro
} else if (count < 16) {
  // Verse
} else {
  // Chorus
}
```

### Loops

```javascript
// For loop
for (let i = 0; i < 16; i++) {
  note_on_at(drums, 2, i * pp);
}

// While loop
let i = 0;
while (i < len(pattern)) {
  if (pattern[i] == 1) {
    note_on_at(drums, 0, i * pp);
  }
  i++;
}
```

### Useful Built-in Functions

```javascript
// Random
rand(10);           // Random 0-9
rand(20) - 10;      // Random -10 to 9

// Increment with wrapping
let x = 0;
x = incr(x, 0, 5);  // Increments 0->1->2->3->4->0->1...

// Random increment/decrement (drunk walk)
let offset = 0;
offset = rincr(offset, -40, 40);  // Randomly walk within range

// Array operations
len(array);         // Length
append(arr, val);   // Add element
min(a, b);          // Minimum
max(a, b);          // Maximum

// Euclidean rhythms
bjork(5, 16);       // 5 hits over 16 steps

// See all functions
help();
```

### Debugging

```javascript
// Print to console
print("Value:", x);
print("Bar:", count, "Pattern:", pattern);

// Check instrument state
info(drums);

// Get parameter value
getparam drums:bd_vol;
```

---

## 11. File-Based Workflow

Move beyond the REPL and write reusable scripts.

### Writing .sb Files

Create files with `.sb` extension in the `SBTraxx/` directory:

**my_beat.sb:**
```javascript
// My awesome beat
// This is a comment

let drums = drum();
load_preset(drums, "TR808");

let kick_comp = comp()
{
  setup()
  {
    let kicks = [1, 0, 0, 0,  0, 0, 1, 0,  0, 0, 0, 0,  1, 0, 0, 0];
  }
  run()
  {
    for (let i = 0; i < 16; i++) {
      if (kicks[i] == 1) {
        note_on_at(drums, 0, i * pp, vel = 100);
      }
    }
  }
}

// Start it
p31 # kick_comp;
```

### Running Scripts

```javascript
// Run a script
run("SBTraxx/my_beat.sb");

// Or with relative path
run("my_beat.sb");
```

### Comments

```javascript
// Full-line comment

let x = 5;  // Inline comment after code

load_preset(drums, "TR808");  // This works too
```

### Project Organization

```
SoundB0ard/
├── SBTraxx/              # Your tracks and patterns
│   ├── my_beat.sb
│   ├── DILLA_DRUNK_SWING.sb
│   └── bass_lines.sb
├── settings/             # Presets
│   ├── drumpresets.dat
│   └── BEAT_STYLE_DRUM_KITS.txt
└── wavs/                 # Sample library
    ├── bd/
    ├── sd/
    └── vox/
```

### Startup Script

Create `startup.sb` in the root directory - it runs automatically on launch:

```javascript
// startup.sb - auto-loads on startup
print("Welcome to my SoundB0ard setup!");
bpm(120);

// Load your default instruments
let drums = drum();
load_preset(drums, "TR808");
```

---

## 12. Advanced Topics

### Live Coding with Track Watching

Watch a file for changes and automatically reload it:

```javascript
// Start watching a file
track("SBTraxx/my_beat.sb");

// Now edit my_beat.sb in your editor
// Save the file - it auto-reloads!

// Stop watching
untrack("SBTraxx/my_beat.sb");
```

### Modulation

Use LFOs and envelopes for movement:

```javascript
let synth = minisynth();

// LFO modulation
setparam synth:lfo_rate 2.0;    // Hz
setparam synth:lfo_depth 0.5;   // Amount

// Envelope
setparam synth:attack 10;       // ms
setparam synth:decay 200;
setparam synth:sustain 0.5;     // 0-1
setparam synth:release 500;
```

### MIDI Integration

```javascript
// List MIDI devices
midi_devices();

// Connect to a MIDI controller
midi_in(0);   // Device index

// MIDI notes will trigger your instruments
// Set up which instrument responds to MIDI
midi_target(drums);
```

### WebSocket Control

Control SoundB0ard from other applications via WebSocket:

```javascript
// WebSocket server runs on port 9002 by default
// Send JSON commands from your browser/app

// Example: trigger from JavaScript
ws.send(JSON.stringify({
  command: "note_on",
  instrument: "drums",
  note: 0,
  velocity: 100
}));
```

---

## 13. Reference

### Quick Command Cheat Sheet

```javascript
// Sound Generators
drum()                  // Drum machine
dxsynth()              // FM synth
minisynth()            // Subtractive synth
granular()             // Granular sampler
sample(path)           // Load audio sample

// Playback
note_on(inst, note, vel, dur)
note_on_at(inst, note, time, vel, dur)

// Parameters
setparam inst:param value
getparam inst:param
info(inst)

// Presets
load_preset(inst, "NAME")
save_preset(inst, "NAME")
list_presets()

// Mixing
vol inst value
pan inst value

// Timing
bpm(tempo)
pp                     // Pulses per 16th note

// Patterns
comp() { setup() {} run() {} }
p31 # comp_name
count                  // Current bar number

// File Operations
run("file.sb")
track("file.sb")

// Utilities
print(...)
help()
help("function_name")
```

### Common Patterns

**Basic Four-on-Floor:**
```javascript
let kicks = [1, 0, 0, 0,  1, 0, 0, 0,  1, 0, 0, 0,  1, 0, 0, 0];
```

**2 and 4 Snare:**
```javascript
let snares = [0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0];
```

**Eighth-Note Hats:**
```javascript
let hats = [1, 0, 1, 0,  1, 0, 1, 0,  1, 0, 1, 0,  1, 0, 1, 0];
```

**Euclidean Rhythm (5 hits in 16 steps):**
```javascript
let pattern = bjork(5, 16);
```

### Keyboard Shortcuts

```
Ctrl+C    - Stop current computation
Ctrl+D    - Exit SoundB0ard
Tab       - Auto-complete
↑/↓       - Command history
```

### Sample Library Organization

```
wavs/
├── bd/       - Bass drums (kick8.aif, wuk77.aiff, etc.)
├── sd/       - Snares (2snare.aif, etc.)
├── cp/       - Claps (clap17.aif, kNackr.aiff, etc.)
├── ch/       - Closed hi-hats (2hat.aif, etc.)
├── oh/       - Open hi-hats
├── perc/     - Percussion (uus.wav, chezShaker.aiff, etc.)
├── vox/      - Vocals and voice samples
└── noises/   - Sound effects (powerup.wav, etc.)
```

### Function Categories

Use `help()` to explore functions by category:

- **Arrays**: len, append, bjork
- **Math**: min, max, abs, pow, sqrt
- **Random**: rand, rincr
- **Control**: incr, dincr
- **Timing**: note_on, note_on_at, bpm
- **Sound**: drum, sample, dxsynth, minisynth
- **Effects**: setparam, getparam
- **File I/O**: run, track, save_preset, load_preset
- **Debug**: print, info

---

## Example Gallery

### Example 1: J Dilla Drunk Swing

The famous loose, swung timing that J Dilla pioneered:

```javascript
let drums = drum();
load_preset(drums, "DILLA");

let kick_comp = comp()
{
  setup()
  {
    let kicks = [1, 0, 0, 0,  0, 0, 1, 0,  0, 0, 0, 0,  1, 0, 0, 0];
    let drunk_offset = 0;
    let swings = [0, -18, 12, -25, 15, -22, 18, -28];
  }
  run()
  {
    for (let i = 0; i < 16; i++) {
      if (kicks[i] == 1) {
        // Drunk walk creates stumbling feel
        drunk_offset = rincr(drunk_offset, -40, 40);
        let total_offset = drunk_offset + swings[i % 8];
        note_on_at(drums, 0, i * pp + total_offset, vel = 95 + rand(15));
      }
    }
  }
}
```

### Example 2: Autechre Polyrhythms

Complex layered polyrhythms with different cycle lengths:

```javascript
let drums = drum();
load_preset(drums, "AUTECHRE");

let poly_comp = comp()
{
  setup()
  {
    // Different length patterns create polyrhythms
    let poly7 = [1, 0, 1, 0, 1, 1, 0];           // 7 steps
    let poly5 = [1, 0, 1, 1, 0];                 // 5 steps
    let poly13 = [1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0];  // 13 steps
  }
  run()
  {
    // Each pattern cycles at its own rate over the 16-step grid
    for (let i = 0; i < 16; i++) {
      // FM drum 1 with 7-step cycle
      if (poly7[i % len(poly7)] == 1) {
        note_on_at(drums, 5, i * pp + rand(30) - 15, vel = 40 + rand(60));
      }

      // FM drum 2 with 5-step cycle
      if (poly5[i % len(poly5)] == 1) {
        note_on_at(drums, 6, i * pp + rand(20) - 10, vel = 50 + rand(50));
      }

      // Lazer with 13-step cycle
      if (poly13[i % len(poly13)] == 1) {
        note_on_at(drums, 8, i * pp + rand(30), vel = 20 + rand(80));
      }
    }
  }
}
```

### Example 3: DJ Premier Boom Bap

Classic hip-hop with tight timing and scratch samples:

```javascript
let drums = drum();
load_preset(drums, "PREMIER");

// Load scratch samples
let scratch1 = sample(perc/dinkSweep.wav);
let scratch2 = sample(noises/powerup.wav);

vol scratch1 0.5;
vol scratch2 0.4;
pan scratch1 -0.6;
pan scratch2 0.7;

let premier_comp = comp()
{
  setup()
  {
    let kicks = [1, 0, 0, 0,  0, 0, 1, 0,  0, 0, 0, 0,  1, 0, 0, 0];
    let snares = [0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0];
    let scratches = [0, 0, 0, 1,  0, 0, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1];
    let pitches = [0.8, 1.2, 0.6, 1.5];
    let px = 0;
  }
  run()
  {
    // Tight kick and snare
    for (let i = 0; i < 16; i++) {
      if (kicks[i] == 1) {
        note_on_at(drums, 0, i * pp, vel = 110);
      }
      if (snares[i] == 1) {
        note_on_at(drums, 1, i * pp, vel = 110 + rand(15));
      }
    }

    // Scratches on certain bars
    if (count % 4 == 3) {
      for (let i = 0; i < 16; i++) {
        if (scratches[i] == 1) {
          note_on_at(scratch1, 1, i * pp, vel = 60 + rand(30), dur = 100);
          set scratch1:pitch pitches[px] at = i * pp;
          px = incr(px, 0, len(pitches));
        }
      }
    }
  }
}
```

---

## Common Pitfalls

### Reserved Words
```javascript
// DON'T:
let p1 = 5;        // ERROR! p1 is a process ID
let if = 10;       // ERROR! if is a keyword

// DO:
let pattern1 = 5;  // OK
let condition = 10; // OK
```

### Timing Issues
```javascript
// DON'T: Use absolute milliseconds
note_on_at(drums, 0, 500);  // Will drift from tempo changes

// DO: Use pp for musical timing
note_on_at(drums, 0, pp * 4);  // Stays in sync
```

### Variable Scope in Computations
```javascript
let my_comp = comp()
{
  setup()
  {
    let pattern = [1, 0, 1, 0];  // Available in both setup() and run()
  }
  run()
  {
    let x = 5;  // Only available in THIS bar's run()
    // Next bar, x is reset!
  }
}
```

### Array Indexing
```javascript
let pattern = [1, 0, 1, 0];
pattern[4];  // ERROR! Index out of bounds (0-3 are valid)

// Check length first
if (i < len(pattern)) {
  let val = pattern[i];
}
```

---

## Tips for Success

1. **Start small** - Get one drum pattern working before building a full track
2. **Use print() liberally** - Debug by printing values to see what's happening
3. **Save your work** - Use save_preset() and write .sb files frequently
4. **Listen to the examples** - Run AUTECHRE_808.sb, DILLA_DRUNK_SWING.sb, PREMIER_BOOM_BAP.sb
5. **Experiment with parameters** - Use info(drums) to see what you can tweak
6. **Build a library** - Create reusable comp() patterns you can mix and match
7. **Use track() for live coding** - Edit .sb files and hear changes immediately

---

## Where to Go Next

- Explore the example beats in `SBTraxx/`
- Read through the preset files in `settings/` to see how different sounds are made
- Try combining multiple synths (drums + bass + pads)
- Experiment with granular synthesis for textures
- Build a full track structure with arrangement sections
- Connect a MIDI controller for live performance
- Share your creations!

**Welcome to the SoundB0ard community. Happy live coding!**

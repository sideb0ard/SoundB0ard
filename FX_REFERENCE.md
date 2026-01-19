# SoundB0ard FX Reference

Quick reference for all audio effects and their parameters.

## How to Use Effects

```javascript
let synth = minisynth();
fx(synth, effect_name);  // Add effect to chain
setparam synth:effect_param value;  // Configure parameters
```

---

## Saturation & Distortion

### distort - Multi-Mode Distortion
**NEW: 4 distortion algorithms!**

```javascript
fx(inst, distort);
setparam inst:distortion_mode 0-3;         // Algorithm selection
setparam inst:distortion_threshold 0.01-1.0;  // Clipping point
setparam inst:distortion_drive 1.0-10.0;   // Input gain
```

**Modes:**
- **0: HARD_CLIP** - Brick wall limiter (classic)
- **1: SOFT_CLIP** - Smooth tanh saturation
- **2: TUBE** - Asymmetric tube warmth
- **3: FOLDBACK** - Wavefold/foldback distortion

### waveshape - Arctan Waveshaper

```javascript
fx(inst, waveshape);
setparam inst:waveshaper_k_pos 0.1-20;     // Positive curve shaping
setparam inst:waveshaper_k_neg 0.1-20;     // Negative curve shaping
setparam inst:waveshaper_stages 1-10;      // Number of stages
setparam inst:waveshaper_invert 0-1;       // Invert alternate stages
```

---

## Degradation / Lo-Fi

### lofi - LoFi Crusher
**NEW: Unified bit crushing + sample rate reduction!**

Replaces `bitcrush` and `decimate` (old names still work).

```javascript
fx(inst, lofi);  // Also: bitcrush, decimate, lofi_crusher
setparam inst:lofi_crusher_bitdepth 1-16;           // Bit depth
setparam inst:lofi_crusher_sample_hold_freq 0.0-1.0;  // Sample rate
setparam inst:lofi_crusher_destruct 0.0-1.0;        // Destruction amount
```

**Examples:**
- Vintage lo-fi: `bitdepth=12, sample_hold=0.9, destruct=0.0`
- Extreme crush: `bitdepth=3, sample_hold=0.2, destruct=0.8`

---

## Time-Based Effects

### delay - Stereo Delay

```javascript
fx(inst, delay);
setparam inst:stereodelay_time_left 0-5000;    // Left delay (ms)
setparam inst:stereodelay_time_right 0-5000;   // Right delay (ms)
setparam inst:stereodelay_feedback 0-99;       // Feedback %
setparam inst:stereodelay_wetmix 0.0-1.0;      // Wet/dry mix
```

### moddelay - Modulated Delay

Chorus/flanger effects via modulated delay time.

```javascript
fx(inst, moddelay);
setparam inst:moddelay_time 0-1000;         // Base delay (ms)
setparam inst:moddelay_feedback 0-99;       // Feedback %
setparam inst:moddelay_mod_depth 0-100;     // Modulation depth
setparam inst:moddelay_mod_rate 0.0-20.0;   // Modulation rate (Hz)
```

### reverb - Reverb

```javascript
fx(inst, reverb);
setparam inst:reverb_roomsize 0.0-1.0;   // Room size
setparam inst:reverb_damping 0.0-1.0;    // HF damping
setparam inst:reverb_wet 0.0-1.0;        // Wet/dry mix
setparam inst:reverb_width 0.0-1.0;      // Stereo width
```

### transverb - Transverb

Advanced delay/reverb hybrid.

```javascript
fx(inst, transverb);
setparam inst:transverb_time 0-2000;        // Delay time (ms)
setparam inst:transverb_feedback 0-99;      // Feedback %
setparam inst:transverb_diffusion 0.0-1.0;  // Diffusion amount
```

---

## Creative / Experimental

### sculptor - Waveform Sculptor
**NEW NAME: Was "geometer"** (old name still works)

Landmark-based waveform manipulation for creative sound design.

```javascript
fx(inst, sculptor);  // Also: geometer, waveform_sculptor
setparam inst:waveform_sculptor_window_size 256-4096;      // Window size
setparam inst:waveform_sculptor_landmark_style 0-4;        // Detection method
setparam inst:waveform_sculptor_interp_style 0-5;          // Recreation method
setparam inst:waveform_sculptor_wet_mix 0.0-1.0;           // Wet/dry
```

**Landmark Styles:**
- 0: ExtNCross (extremities & zero-crossings)
- 1: Span (amplitude-based)
- 2: DyDx (derivative/slope)
- 3: Freq (regular frequency)
- 4: Random

**Interpolation Styles:**
- 0: Polygon (straight lines, lo-fi)
- 1: Wrongygon (backwards, harsh)
- 2: Sing (sine waves, tonal)
- 3: Reversi (reverse intervals)
- 4: Smoothie (smooth curves)
- 5: Pulse (just pulses)

### diffuser - Diffuser
**NEW NAME: Was "nnirror"** (old name still works)

Multi-stage diffusion/blur/reverb hybrid.

```javascript
fx(inst, diffuser);  // Also: nnirror
setparam inst:diffuser_wet 0.0-1.0;        // Wet/dry mix
setparam inst:diffuser_size 0.0-1.0;       // Size/length
setparam inst:diffuser_feedback 0.0-1.0;   // Feedback
setparam inst:diffuser_unison 0.0-1.0;     // Unison voices
setparam inst:diffuser_diffuse0 0.0-1.0;   // Diffusion stage 1
setparam inst:diffuser_diffuse1 0.0-1.0;   // Diffusion stage 2
setparam inst:diffuser_diffuse2 0.0-1.0;   // Diffusion stage 3
setparam inst:diffuser_diffuse3 0.0-1.0;   // Diffusion stage 4
```

### granulate - Granulator

Granular synthesis for textures.

```javascript
fx(inst, granulate);  // Also: gran
setparam inst:granulate_grain_size 10-500;     // Grain size (ms)
setparam inst:granulate_density 1-100;         // Grains per second
setparam inst:granulate_spray 0.0-1.0;         // Time randomness
```

---

## Dynamics

### compressor - Compressor/Limiter

```javascript
fx(inst, compressor);
setparam inst:compressor_threshold -60-0;      // Threshold (dB)
setparam inst:compressor_ratio 1.0-20.0;       // Compression ratio
setparam inst:compressor_attack 0.1-500;       // Attack (ms)
setparam inst:compressor_release 10-5000;      // Release (ms)
setparam inst:compressor_knee_width 0-20;      // Knee width (dB)
```

---

## Filters

### filter - Basic Filter

```javascript
fx(inst, filter);
setparam inst:basicfilter_cutoff 20-20000;    // Cutoff (Hz)
setparam inst:basicfilter_resonance 0.0-10.0;  // Resonance/Q
```

### modfilter - Modulated Filter

Filter with LFO modulation.

```javascript
fx(inst, modfilter);
setparam inst:modfilter_cutoff 20-20000;       // Base cutoff (Hz)
setparam inst:modfilter_resonance 0.0-10.0;    // Resonance/Q
setparam inst:modfilter_mod_depth 0-5000;      // Modulation depth (Hz)
setparam inst:modfilter_mod_rate 0.0-20.0;     // Modulation rate (Hz)
```

---

## Effect Chains

Multiple effects are processed in the order they're added:

```javascript
let lead = dxsynth();
fx(lead, distort);      // 1st: Distortion
fx(lead, moddelay);     // 2nd: Modulated delay
fx(lead, reverb);       // 3rd: Reverb

// Configure in any order
setparam lead:distortion_mode 2;  // Tube
setparam lead:reverb_roomsize 0.8;
```

---

## Backward Compatibility

Old effect names still work:
- `bitcrush` → creates LoFiCrusher
- `decimate` → creates LoFiCrusher
- `geometer` / `geom` → creates WaveformSculptor
- `nnirror` / `nnrrr` → creates Diffuser

---

## Quick Examples

### Warm Analog-Style Lead
```javascript
let lead = minisynth();
fx(lead, distort);
fx(lead, reverb);
setparam lead:distortion_mode 2;      // Tube
setparam lead:distortion_drive 2.5;
setparam lead:reverb_roomsize 0.6;
```

### Lo-Fi Hip-Hop Drums
```javascript
let drums = drum();
fx(drums, lofi);
setparam drums:lofi_crusher_bitdepth 10;
setparam drums:lofi_crusher_sample_hold_freq 0.85;
```

### Experimental Pad
```javascript
let pad = minisynth();
fx(pad, diffuser);
fx(pad, sculptor);
setparam pad:diffuser_size 0.8;
setparam pad:sculptor_interp_style 4;  // Smoothie
```

### Gnarly Bass
```javascript
let bass = dxsynth();
fx(bass, distort);
setparam bass:distortion_mode 3;      // Foldback
setparam bass:distortion_drive 7.0;
```

---

## Tips

1. **Order matters**: Distortion → Delay → Reverb is typical
2. **Start subtle**: Effects can quickly overwhelm
3. **Use wet/dry**: Most effects have wet_mix or wetmix parameters
4. **Experiment**: The creative FX (sculptor, diffuser) reward exploration
5. **Stack wisely**: 3-4 effects max per instrument for CPU efficiency

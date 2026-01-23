#include <defjams.h>
#include <drumsampler.h>
#include <fmsynth.h>
#include <math.h>
#include <midi_freq_table.h>
#include <midimaaan.h>
#include <minisynth.h>
#include <mixer.h>
#include <portmidi.h>
#include <porttime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

#include <iostream>

midi_event new_midi_event(int event_type, int data1, int data2) {
  midi_event ev = {.event_type = event_type, .data1 = data1, .data2 = data2};
  return ev;
}

std::ostream &operator<<(std::ostream &out, const midi_event &ev) {
  out << "MIDIEVENT:"
      << "Type:" << ev.event_type << " D1:" << ev.data1 << " D2:" << ev.data2;
  return out;
}

void midi_pattern_quantize(midi_pattern *pattern) {
  printf("Quantizzzzzzing\n");

  midi_pattern quantized_loop = {};

  for (int i = 0; i < PPBAR; i++) {
    midi_event ev = (*pattern)[i];
    if (ev.event_type) {
      int amendedtick = 0;
      int tickdiv16 = i / PPSIXTEENTH;
      int lower16th = tickdiv16 * PPSIXTEENTH;
      int upper16th = lower16th + PPSIXTEENTH;
      if ((i - lower16th) < (upper16th - i))
        amendedtick = lower16th;
      else
        amendedtick = upper16th;

      quantized_loop[amendedtick] = ev;
      printf("Amended TICK: %d\n", amendedtick);
    }
  }

  for (int i = 0; i < PPBAR; i++) (*pattern)[i] = quantized_loop[i];
}

void midi_pattern_print(midi_event *loop) {
  for (int i = 0; i < PPBAR; i++) {
    midi_event ev = loop[i];
    if (ev.event_type) {
      // TODO: This function appears incomplete - event_type is checked but not
      // used
      (void)ev;  // Suppress unused variable warning
    }
  }
}
void midi_event_cp(midi_event *from, midi_event *to) {
  to->event_type = from->event_type;
  to->data1 = from->data1;
  to->data2 = from->data2;
}

void midi_event_clear(midi_event *ev) {
  memset(ev, 0, sizeof(midi_event));
}

void midi_event_print(midi_event *ev) {
  // TODO: This function appears incomplete - no printing is performed
  (void)ev;  // Suppress unused variable warning
}

int get_midi_note_from_string(char *string) {
  if (strlen(string) > 4) {
    printf("DINGIE!\n");
    return -1;
  }
  char note[3] = {0};
  int octave = 0;  // default if none provided
  sscanf(string, "%2[a-z#]%d", note, &octave);
  note[2] = 0;  // safety

  // convert octave to midi semitones
  octave = 12 + (octave * 12);

  //// twelve semitones:
  //// C C#/Db D D#/Eb E F F#/Gb G G#/Ab A A#/Bb B
  int midinotenum = -1;
  if (!strcasecmp("c", note))
    midinotenum = 0 + octave;
  else if (!strcasecmp("c#", note) || !strcasecmp("db", note) ||
           !strcasecmp("dm", note))
    midinotenum = 1 + octave;
  else if (!strcasecmp("d", note))
    midinotenum = 2 + octave;
  else if (!strcasecmp("d#", note) || !strcasecmp("eb", note) ||
           !strcasecmp("em", note))
    midinotenum = 3 + octave;
  else if (!strcasecmp("e", note))
    midinotenum = 4 + octave;
  else if (!strcasecmp("f", note))
    midinotenum = 5 + octave;
  else if (!strcasecmp("f#", note) || !strcasecmp("gb", note) ||
           !strcasecmp("gm", note))
    midinotenum = 6 + octave;
  else if (!strcasecmp("g", note))
    midinotenum = 7 + octave;
  else if (!strcasecmp("g#", note) || !strcasecmp("ab", note) ||
           !strcasecmp("am", note))
    midinotenum = 8 + octave;
  else if (!strcasecmp("a", note))
    midinotenum = 9 + octave;
  else if (!strcasecmp("a#", note) || !strcasecmp("bb", note) ||
           !strcasecmp("bm", note))
    midinotenum = 10 + octave;
  else if (!strcasecmp("b", note))
    midinotenum = 11 + octave;
  // printf("MIDI NOTE NUM:%d \n", midinotenum);

  return midinotenum;
}
int get_midi_note_from_mixer_key(unsigned int key, int octave) {
  int midi_octave = 12 + (octave * 12);
  return key + midi_octave;
}

void midi_pattern_set_velocity(midi_event *pattern, unsigned int midi_tick,
                               unsigned int velocity) {
  if (!pattern) {
    printf("Dingie, gimme a REAL pattern!\n");
    return;
  }

  if (midi_tick < PPBAR && velocity < 128)
    pattern[midi_tick].data2 = velocity;
  else
    printf("Nae valid!? Midi_tick:%u // velocity:%u\n", midi_tick, velocity);
}

void midi_pattern_rand_amp(midi_event *pattern) {
  for (int i = 0; i < PPBAR; i++) {
    if (pattern[i].event_type == MIDI_ON) pattern[i].data2 = rand() % 127;
  }
}

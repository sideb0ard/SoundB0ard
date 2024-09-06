#include <audioutils.h>
#include <midimaaan.h>
#include <portaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

#include <iostream>

// used by GetNthDegree
namespace {

int GetMidiNumForKey(char c) {
  char lc_c = tolower(c);
  switch (lc_c) {
    case ('c'):
      return 0;
    case ('d'):
      return 2;
    case ('e'):
      return 4;
    case ('f'):
      return 5;
    case ('g'):
      return 7;
    case ('a'):
      return 9;
    case ('b'):
      return 11;
    default:
      return -1;
  }
}

}  // namespace

int GetRootNote(int key, int index) {
  index = index % 7;
  switch (index) {
    case (0):
      return key;
    case (1):
      return (key + 2) % 12;
    case (2):
      return (key + 4) % 12;
    case (3):
      return (key + 5) % 12;
    case (4):
      return (key + 7) % 12;
    case (5):
      return (key + 9) % 12;
    case (6):
      return (key + 11) % 12;
    default:
      return index;
  }
}

int GetScaleIndex(int note, char key) {
  if (!IsMidiNoteInKeyChar(note, key)) return -1;
  note = note % 12;
  int key_midi_num = GetMidiNumForKey(key);
  if (note == key_midi_num)
    return 0;
  else if (note == (key_midi_num + 2) % 12)
    return 1;
  else if (note == (key_midi_num + 4) % 12)
    return 2;
  else if (note == (key_midi_num + 5) % 12)
    return 3;
  else if (note == (key_midi_num + 7) % 12)
    return 4;
  else if (note == (key_midi_num + 9) % 12)
    return 5;
  else if (note == (key_midi_num + 11) % 12)
    return 6;
  return -1;
}

int GetStepsToNextDegree(int scale_index) {
  if (scale_index > 7) return -1;
  switch (scale_index) {
    case (0):
      return 2;
    case (1):
      return 2;
    case (2):
      return 1;
    case (3):
      return 2;
    case (4):
      return 2;
    case (5):
      return 2;
    case (6):
      return 1;
  }
  return -1;
}

int GetMidiNumFromNote(std::string note) {
  if (note == "c")
    return 0;
  else if (note == "c#")
    return 1;
  else if (note == "d")
    return 2;
  else if (note == "d#")
    return 3;
  else if (note == "e")
    return 4;
  else if (note == "f")
    return 5;
  else if (note == "f#")
    return 6;
  else if (note == "g")
    return 7;
  else if (note == "g#")
    return 8;
  else if (note == "a")
    return 9;
  else if (note == "a#")
    return 10;
  else if (note == "b")
    return 11;
  else
    return -1;
}

std::string GetNoteFromMidiNum(int midi_num) {
  int base_midi_num = midi_num % 12;
  switch (base_midi_num) {
    case (0):
      return "C";
    case (1):
      return "C#";
    case (2):
      return "D";
    case (3):
      return "D#";
    case (4):
      return "E";
    case (5):
      return "F";
    case (6):
      return "F#";
    case (7):
      return "G";
    case (8):
      return "G#";
    case (9):
      return "A";
    case (10):
      return "A#";
    case (11):
      return "B";
    default:
      return "error";
  }
}

double pa_setup(void) {
  // PA start me up!
  PaError err;
  err = Pa_Initialize();
  if (err != paNoError) {
    printf("Errrrr! couldn't initialize Portaudio: %s\n", Pa_GetErrorText(err));
    exit(-1);
  }
  PaStreamParameters params;
  params.device = Pa_GetDefaultOutputDevice();
  if (params.device == paNoDevice) {
    printf("BARFd on PA_GetDefaultOutputDevice!\n");
    exit(1);
  }
  double suggested_latency =
      Pa_GetDeviceInfo(params.device)->defaultLowOutputLatency;
  printf("SUGGESTED LATENCY: %f\n", suggested_latency);
  return suggested_latency;
}

void pa_teardown(void) {
  //  time to go home!
  PaError err;
  err = Pa_Terminate();
  if (err != paNoError) {
    printf("Errrrr while terminating Portaudio: %s\n", Pa_GetErrorText(err));
    exit(-1);
  }
}

int get_chord_type(unsigned int scale_degree) {
  if (scale_degree == 0 || scale_degree == 3 || scale_degree == 4 ||
      scale_degree == 7)
    return MAJOR_CHORD;
  else if (scale_degree == 1 || scale_degree == 2 || scale_degree == 5)
    return MINOR_CHORD;
  else if (scale_degree == 6)
    return DIMINISHED_CHORD;
  else
    return -1;
}

// TODO - maybe delete? New version in sound_cmds.cpp
std::vector<int> GetMidiNotesInChord(unsigned int root_note,
                                     unsigned int chord_type,
                                     unsigned int modification) {
  std::vector<int> notes_in_chord{};
  notes_in_chord.push_back(root_note);

  if (chord_type == POWER_CHORD) {
    notes_in_chord.push_back(root_note + 7);
    notes_in_chord.push_back(root_note + 12);
  } else if (chord_type == MAJOR_CHORD) {
    notes_in_chord.push_back(root_note + 4);
    notes_in_chord.push_back(root_note + 7);
  } else if (chord_type == MINOR_CHORD) {
    notes_in_chord.push_back(root_note + 3);
    notes_in_chord.push_back(root_note + 7);
  } else if (chord_type == DIMINISHED_CHORD) {
    notes_in_chord.push_back(root_note + 3);
    notes_in_chord.push_back(root_note + 6);
  } else if (chord_type == SUS2_CHORD) {
    notes_in_chord.push_back(root_note + 2);
    notes_in_chord.push_back(root_note + 7);
  } else if (chord_type == SUS4_CHORD) {
    notes_in_chord.push_back(root_note + 5);
    notes_in_chord.push_back(root_note + 7);
  }

  switch (modification) {
    case (1):  // minor 7th
      notes_in_chord.push_back(root_note + 10);
      break;
    case (2):  // major 7th
      notes_in_chord.push_back(root_note + 11);
      break;
    case (3):  // inverted minor 7th
      notes_in_chord.push_back(root_note - 2);
      break;
    case (4):  // inverted major 7th
      notes_in_chord.push_back(root_note - 1);
      break;
    case (5):  // inverted minor 7th, no root
      notes_in_chord[0] -= 2;
      break;
    case (6):  // inverted major 7th, no root
      notes_in_chord[0] -= 1;
      break;
    case (7):  // augment the bass +Octave
      notes_in_chord[0] += 12;
      break;
      // case (3):  // augmentation
      //   notes_in_chord.push_back(root_note + 10);
      //   notes_in_chord.push_back(root_note + 12);
      //   break;
      // case (4):  // augmentation
      //   notes_in_chord.push_back(root_note + 10);
      //   notes_in_chord.push_back(root_note - 24);
      //   break;
      // case (5):  // augmentation
      //   notes_in_chord.push_back(root_note + 10);
      //   notes_in_chord.push_back(root_note + 24);
      //   break;
  }

  return notes_in_chord;
}

void get_midi_notes_from_chord(unsigned int note, unsigned int chord_type,
                               int octave, chord_midi_notes *chnotes) {
  int root_midi = get_midi_note_from_mixer_key(note, octave);

  int third_note = 0;
  int third_midi = 0;
  if (chord_type == MAJOR_CHORD)
    third_note = (note + 4);
  else
    third_note = (note + 3);
  if (third_note >= NUM_KEYS)
    third_midi =
        get_midi_note_from_mixer_key(third_note % NUM_KEYS, octave + 1);
  else
    third_midi = get_midi_note_from_mixer_key(third_note, octave);

  int fifth_note = 0;
  int fifth_midi = 0;
  if (chord_type == DIMINISHED_CHORD)
    fifth_note = note + 6;
  else
    fifth_note = note + 7;
  if (fifth_note >= NUM_KEYS)
    fifth_midi =
        get_midi_note_from_mixer_key(fifth_note % NUM_KEYS, octave + 1);
  else
    fifth_midi = get_midi_note_from_mixer_key(fifth_note, octave);

  // printf("ROOT MIDI:%d THIRD:%d FIFTH:%d\n", root_midi, third_midi,
  //       fifth_midi);

  int randy = rand() % 100;
  if (randy > 90) {
    int randy_root = 0;
    int randy_fifth = 0;

    if (randy > 97)
      randy_root = root_midi + 12;
    else if (randy > 95)
      randy_root = root_midi - 12;
    else if (randy > 92)
      randy_fifth = fifth_midi + 12;
    else if (randy > 90)
      randy_fifth = fifth_midi - 12;

    if (randy_root > 0 && randy_root < 128) root_midi = randy_root;
    if (randy_fifth > 0 && randy_fifth < 128) fifth_midi = randy_fifth;
  }

  chnotes->root = root_midi;
  chnotes->third = third_midi;
  chnotes->fifth = fifth_midi;
}

// returns an int to indicate midi num representing n'th
// degree from input note. Returns -1 if note not in Key.
int GetNthDegree(int note, int degree, char key) {
  if (!IsMidiNoteInKeyChar(note, key)) {
    std::cout << "NOT IN KEY KEY YO\n";
    return -1;
  }
  int scale_index = GetScaleIndex(note, key);
  int return_midi_num = note;
  for (int i = 0; i < degree; i++) {
    int steps_to_next = GetStepsToNextDegree(scale_index);
    return_midi_num += steps_to_next;
    // std::cout << "i:" << i << " Scale IDX:" << scale_index
    //          << " steps_to_next:" << steps_to_next
    //          << " return_midi:" << return_midi_num << std::endl;
    scale_index = ++scale_index % 7;  // 7 distinct notes in scale
  }
  return return_midi_num;
}

int GetThird(int note, char key) { return GetNthDegree(note, 2, key); }
int GetFifth(int note, char key) { return GetNthDegree(note, 4, key); }

bool IsMidiNoteInKeyChar(int note, char key) {
  int key_midi_num = GetMidiNumForKey(key);
  return IsMidiNoteInKey(note, key_midi_num);
}

bool IsMidiNoteInKey(int note, int key_midi_num) {
  note = note % 12;
  if (note == key_midi_num)
    return true;
  else if (note == (key_midi_num + 2) % 12)
    return true;
  else if (note == (key_midi_num + 4) % 12)
    return true;
  else if (note == (key_midi_num + 5) % 12)
    return true;
  else if (note == (key_midi_num + 7) % 12)
    return true;
  else if (note == (key_midi_num + 9) % 12)
    return true;
  else if (note == (key_midi_num + 11) % 12)
    return true;
  else if (note == (key_midi_num + 12) % 12)
    return true;
  return false;
}

bool IsNote(std::string input) {
  if (input.size() > 0) {
    char firstchar = ::tolower(input[0]);
    if (firstchar == 'a' || firstchar == 'b' || firstchar == 'c' ||
        firstchar == 'd' || firstchar == 'e' || firstchar == 'f' ||
        firstchar == 'g')
      return true;
  }

  return false;
}

std::vector<int> ScaleMelodyToKey(const std::vector<int> &melody_in,
                                  int root_midi, int scale) {
  std::vector<int> melody_out{};
  int base_midi = root_midi % 12;
  for (const auto &n : melody_in) {
    if (IsMidiNoteInKey(n, base_midi))
      melody_out.push_back(n);
    else
      melody_out.push_back(n - 1);
  }
  return melody_out;
}

#pragma once

// taken from http://www.willpirkle.com/synthbook/

extern const float kMidiFreqTable[128];
float Midi2Freq(int midinum);
int Freq2Midi(float freq);

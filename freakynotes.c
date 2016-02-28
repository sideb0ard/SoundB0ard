#include <math.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void offend()
{
  printf("play tha game, eh, mate?!\n");
}

int notelookup(char *n)
{
  // twelve semitones:
  // C C#/Db D D#/Eb E F F#/Gb G G#/Ab A A#/Bb B
  //
  if (!strcasecmp("c", n)) return 0;
  else if (!strcasecmp("c#", n)) return 1;
  else if (!strcasecmp("db", n)) return 1;
  else if (!strcasecmp("d",  n)) return 2;
  else if (!strcasecmp("d#", n)) return 3;
  else if (!strcasecmp("eb", n)) return 3;
  else if (!strcasecmp("e",  n)) return 4;
  else if (!strcasecmp("f",  n)) return 5;
  else if (!strcasecmp("f#", n)) return 6;
  else if (!strcasecmp("gb", n)) return 6;
  else if (!strcasecmp("g",  n)) return 7;
  else if (!strcasecmp("g#", n)) return 8;
  else if (!strcasecmp("ab", n)) return 8;
  else if (!strcasecmp("a",  n)) return 9;
  else if (!strcasecmp("a#", n)) return 10;
  else if (!strcasecmp("bb", n)) return 10;
  else if (!strcasecmp("b",  n)) return 11;
  else
    return -1;
}

float freqval(char *n)
{
  // algo from http://www.phy.mtu.edu/~suits/NoteFreqCalcs.html
  float a4 = 440.0; // fixed note freq to use as baseline
  const float twelfth_root_of_two = 1.059463094359;

  regmatch_t nmatch[3];
  regex_t single_letter_rx;
  regcomp(&single_letter_rx, "^([[:alpha:]#]{1,2})([[:digit:]])$", REG_EXTENDED|REG_ICASE);
  if (regexec(&single_letter_rx, n, 3, nmatch, 0) == 0) {

    int note_str_len = nmatch[1].rm_eo - nmatch[1].rm_so;
    char note[note_str_len + 1];
    strncpy(note, n+nmatch[1].rm_so, note_str_len);
    note[note_str_len] = '\0';

    char str_octave[2];
    strncpy(str_octave, n+note_str_len, 1);
    str_octave[1] = '\0';

    // purpose of this is working out how many semitones the given note is from A4
    int n_num = (12 * atoi(str_octave)) + notelookup(note);
    // fixed note, which we compare against is A4 - '4' is the fourth
    // octave, so 4 * 12 semitones, plus lookup val of A is '9' - so 57
    int diff = n_num - 57;

    float freqval = a4 * (pow(twelfth_root_of_two, diff));
    return freqval;
  } else {
    return -1.0;
  }
}

int main(int argc, char **argv)
{
  if (argc < 2){
    offend();
    return -1;
  }
  for (int i = 1; i < argc; i++) {
    float f = freqval(argv[i]);
    if (f == -1.0)
      offend();
    else
      printf("%s : %.2f ", argv[i], f);
  }
  printf("\n");
}

#include <stdio.h>
#include "defjams.h"
#include "help.h"

void print_help() {
    printf(ANSI_COLOR_WHITE "\n");
    printf("SBSHELL - interactive music making shell=================\n");
    printf("Commands::::\n");
    printf("ps                                  - shows you status of Mixing desk - BPM, cur tick, and list of channels\n");
    printf("ls                                  - show you a list of playable samples in the 'wav/' directory\n");
    printf("sine  <FREQ>                        - create a sine wave of FREQ Khz, e.g. `sine 440` for a middle A\n");
    printf("fm    <MODFREQ> <CARFREQ>           - create an Frequency Modulator.\n");
    printf("play  <FILE>    <ticks...>          - play sample on ticks (tick are 0-15> e.g. `play kick2.wav 0 4 8 12`\n");
    printf("loop  <FREQ...> <BARS>              - loop the list of FREQs, changing FREQ every num of BARS e.g. `loop 440 220 2`\n");
    printf("sloop <FILE>    <BARS>              - loop sample for BARS e.g. `sloop organ.wav 2`\n");
    printf("floop <MODFREQ> <CARFREQ...> <BARS> - loop FMs, changing through list of CARFREQS every num of BARS\n");
    printf(ANSI_COLOR_RESET);
}



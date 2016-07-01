#include "help.h"
#include "defjams.h"
#include <stdio.h>

void print_help()
{
    printf(COOL_COLOR_GREEN "\n");
    printf(":::: SBSHELL - interactive music making shell :::::::::\n");
    printf(":::: \n");
    printf(":::: Status & Global Commands ::::\n");
    printf(ANSI_COLOR_WHITE);
    printf("ps                                      - shows you status of "
           "Mixing desk - BPM, cur tick, and list of channels\n");
    printf("ls                                      - show you a list of "
           "playable samples in the 'wav/' directory\n");
    printf(
        "delay                                   - switch on global delay\n");
    printf("exit                                    - ...\n");
    printf("bpm       <BPM>                         - change BPM\n");
    printf("vol       <VAL>                         - change volume, a float "
           "between 0 and 1\n");
    printf(COOL_COLOR_GREEN);
    printf(":::: \n");
    printf(":::: Sound Generator Commands ::::\n");
    printf(ANSI_COLOR_WHITE);
    printf("sine      <FREQ>                        - create a sine wave of "
           "FREQ Khz, e.g. `sine 440` for a middle A\n");
    printf("sawd      <FREQ>                        - create a downward saw "
           "wave of FREQ Khz, e.g. `sawd 440` for a middle A\n");
    printf("sawu      <FREQ>                        - create a upward saw wave "
           "of FREQ Khz, e.g. `sawu 440` for a middle A\n");
    printf("tri       <FREQ>                        - create a triangle wave "
           "of FREQ Khz, e.g. `tri 440` for a middle A\n");
    printf("square    <FREQ>                        - create a square wave of "
           "FREQ Khz, e.g. `square 440` for a middle A\n");
    printf("fm        <MODFREQ> <CARFREQ>           - create an Frequency "
           "Modulator which is two oscillators, the mod and car\n");
    printf("play      <FILE>    <ticks...>          - play sample on ticks "
           "(tick are 0-15> e.g. `play kick2.wav 0 4 8 12`\n");
    printf(COOL_COLOR_GREEN);
    printf(":::: \n");
    printf(":::: Loop Generator Commands ::::\n");
    printf(ANSI_COLOR_WHITE);
    printf("loop      <FREQ...> <BARS>              - loop the list of FREQs, "
           "changing FREQ every num of BARS e.g. `loop 440 220 2`\n");
    printf("sloop     <FILE>    <BARS>              - loop sample for BARS "
           "e.g. `sloop organ.wav 2`\n");
    printf("floop     <MODFREQ> <CARFREQ...> <BARS> - loop FMs, changing "
           "through list of CARFREQS every num of BARS\n");
    printf(COOL_COLOR_GREEN);
    printf(":::: \n");
    printf(":::: Effects Commands ::::\n");
    printf(ANSI_COLOR_WHITE);
    printf("mfm       <sig_num> <mod|car> <FREQ>    - Modify the given FM "
           "signal_num, change modulator or carrier freq to <FREQ>.\n");
    printf("solo      <sig_num>                     - Solo the given signal "
           "number\n");
    printf("duck      <sig_num>                     - Fade out the given "
           "signal number for a random time period up to 30 seconds\n");
    printf("vol       <sig_num> <VAL>               - change volume of given "
           "signal number, a float between 0 and 1\n");
    printf("delay     <sig_num> <TIME_VAL>          - add delay to signal "
           "number, TIME_VAL is a float in seconds.\n");
    printf("randdrum  <sig_num> <BARS>              - randomizes the drum "
           "pattern of sig_num sampler every BARS number of bars\n");
    printf("lowpass   <sig_num> <VAL>               - don't think this quite "
           "works - supposedly puts a lowpass on sig num, of VAL freq\n");
    printf("highpass  <sig_num> <VAL>               - don't think this quite "
           "works - supposedly puts a highpass on sig num, of VAL freq\n");
    printf("bandpass  <sig_num> <VAL>               - don't think this quite "
           "works - supposedly puts a bandpass on sig num, of VAL freq\n");
    printf(ANSI_COLOR_RESET);
}

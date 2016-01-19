#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "audioutils.h"
#include "defjams.h"
#include "cmdloop.h"
#include "mixer.h"

extern mixer *mixr;

void loopy(void)
{
  char *line;
  while ((line = readline(ANSI_COLOR_MAGENTA "SB#> " ANSI_COLOR_RESET))!= NULL) {
    if (line[0] != 0) {
      add_history(line);
      interpret(line);
    }
  }
}

void ps() 
{
  mixer_ps(mixr);
}

void gen() 
{
  gen_next(mixr);
}

int exxit()
{
    printf("\nBeat it, ya val jerk...\n");
    pa_teardown();
    exit(0);
    //return 0;
}

void interpret(char *line)
{
  // easy string comparisons
  if (strcmp(line, "ps") == 0) {
    ps();
    return;
  } else if (strcmp(line, "exit") == 0) {
    exxit();
  }

  // TODO: move regex outside function to compile once
  // SINE|SAW|TRI (FREQ)
  regmatch_t pmatch[3];
  regex_t sigtype_rx;
  regcomp(&sigtype_rx, "(sine|sawd|sawu|tri|square) ([[:digit:]]+)", REG_EXTENDED|REG_ICASE);

  if (regexec(&sigtype_rx, line, 3, pmatch, 0) == 0) {
    int freq = 0;
    char sig_type[10];
    sscanf(line, "%s %d", sig_type, &freq);
    if (strcmp(sig_type, "sine") == 0) {
        add_osc(mixr, freq, &sinetick);
    } else if (strcmp(sig_type, "sawd") == 0) {
        add_osc(mixr, freq, &sawdtick);
    } else if (strcmp(sig_type, "sawu") == 0) {
        add_osc(mixr, freq, &sawutick);
    } else if (strcmp(sig_type, "tri") == 0) {
        add_osc(mixr, freq, &tritick);
    } else if (strcmp(sig_type, "square") == 0) {
        add_osc(mixr, freq, &sqtick);
    }
  }
}

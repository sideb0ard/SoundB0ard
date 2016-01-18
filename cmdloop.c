#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "defjams.h"
#include "cmdloop.h"
#include "mixer.h"

extern mixer *mixr;

// regex used below - but up here so only compiled once

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

void osc(int freq) 
{
  add_osc(mixr, freq);
}

int exxit()
{
    printf("\nBeat it, ya val jerk...\n");
    return 0;
}

void interpret(char *line)
{
  // SINE|SAW|TRI (FREQ)
  regmatch_t pmatch[3];
  regex_t sigtype_rx;
  regcomp(&sigtype_rx, "(sine|saw|tri) ([[:digit:]]+)", REG_EXTENDED|REG_ICASE);

  if (regexec(&sigtype_rx, line, 3, pmatch, 0) == 0) {
    int freq = 0;
    char sig_type[10];
    sscanf(line, "%s %d", sig_type, &freq);
    osc(freq);
  }
}

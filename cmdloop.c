#include <pthread.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "algoriddim.h"
#include "audioutils.h"
#include "bpmrrr.h"
#include "envelope.h"
#include "cmdloop.h"
#include "defjams.h"
#include "mixer.h"
#include "table.h"

extern mixer *mixr;
extern bpmrrr *b;
extern ENVSTREAM *ampstream;

// TODO: make into a single array of lookup tables
extern GTABLE *sine_table;
extern GTABLE *tri_table;
extern GTABLE *square_table;
extern GTABLE *saw_down_table;
extern GTABLE *saw_up_table;

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
  bpm_info(b);
  ps_envelope_stream(ampstream);
  //table_info(gtable);
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
  } else if (strcmp(line, "song") == 0) {
    pthread_t songrun_th;
    if ( pthread_create (&songrun_th, NULL, algo_run, NULL)) {
      fprintf(stderr, "Errr running song\n");
      return;
    }
    pthread_detach(songrun_th);
  } else if (strcmp(line, "exit") == 0) {
    exxit();
  }

  // TODO: move regex outside function to compile once
  // SINE|SAW|TRI (FREQ)
  regmatch_t pmatch[3];
  regex_t sigtype_rx;
  regcomp(&sigtype_rx, "(bpm|sine|sawd|sawu|tri|square) ([[:digit:]]+)", REG_EXTENDED|REG_ICASE);

  if (regexec(&sigtype_rx, line, 3, pmatch, 0) == 0) {
    int val = 0;
    char sig_type[10];
    sscanf(line, "%s %d", sig_type, &val);
    if (strcmp(sig_type, "bpm") == 0) {
      bpm_change(b, val);
      update_envelope_stream_bpm(ampstream);
    } else if (strcmp(sig_type, "sine") == 0) {
        add_osc(mixr, val, sine_table);
    } else if (strcmp(sig_type, "sawd") == 0) {
        add_osc(mixr, val, saw_down_table);
    } else if (strcmp(sig_type, "sawu") == 0) {
        add_osc(mixr, val, saw_up_table);
    } else if (strcmp(sig_type, "tri") == 0) {
        add_osc(mixr, val, tri_table);
    } else if (strcmp(sig_type, "square") == 0) {
        add_osc(mixr, val, square_table);
    //} else if (strcmp(sig_type, "tsine") == 0) {
    //    add_tosc(mixr, val, sine_table);
    }
  }

  regmatch_t tpmatch[4];
  regex_t tsigtype_rx;
  regcomp(&tsigtype_rx, "(vol|freq) ([[:digit:]]+) ([[:digit:]]+)", REG_EXTENDED|REG_ICASE);
  if (regexec(&tsigtype_rx, line, 3, tpmatch, 0) == 0) {
    int sig = 0;
    double val = 0;
    char cmd_type[10];
    sscanf(line, "%s %d %lf", cmd_type, &sig, &val);
    if (strcmp(cmd_type, "vol") == 0) {
      vol_change(mixr, sig, val);
      printf("VOL! %s %d %lf\n", cmd_type, sig, val);
    }
    if (strcmp(cmd_type, "freq") == 0) {
      freq_change(mixr, sig, val);
      printf("FREQ! %s %d %lf\n", cmd_type, sig, val);
    }
  }
}

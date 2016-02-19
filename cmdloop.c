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
#include "fm.h"
#include "cmdloop.h"
#include "defjams.h"
#include "mixer.h"
#include "table.h"
#include "utils.h"

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
  } else if (strcmp(line, "hat") == 0) {
    printf("SAMPLER!\n");
    add_sample(mixr);
  } else if (strcmp(line, "exit") == 0) {
    exxit();
  }

  // TODO: move regex outside function to compile once
  // SINE|SAW|TRI (FREQ)
  regmatch_t pmatch[3];
  regex_t cmdtype_rx;
  regcomp(&cmdtype_rx, "^(bpm|stop|sine|sawd|sawu|tri|square|vol) ([[:digit:].]+)$", REG_EXTENDED|REG_ICASE);

  if (regexec(&cmdtype_rx, line, 3, pmatch, 0) == 0) {

    float val = 0;
    char cmd[20];
    sbmsg *msg = calloc(1, sizeof(sbmsg));

    sscanf(line, "%s %f", cmd, &val);
    msg->freq = val;

    if (strcmp(cmd, "bpm") == 0) {
      bpm_change(b, val);
      update_envelope_stream_bpm(ampstream);
    } else if (strcmp(cmd, "vol") == 0) {
      printf("VOLLY BALL\n");
      mixer_vol_change(mixr, val);
    } else if (strcmp(cmd, "stop") == 0) {
      strncpy(msg->cmd, "faderrr", 19);
      thrunner(msg);
    } else {
      strncpy(msg->cmd, "timed_sig_start", 19);
      strncpy(msg->params, cmd, 10);
      thrunner(msg);


      //if ( pthread_create (&timed_start_th, NULL, timed_sig_start, params)) {
      //  printf("Ooft!\n");
      //}
    }
  }

	// Modify an FM
	regex_t mfm_rx;
	regcomp(&mfm_rx, "^mfm ([[:digit:]]+) (mod|car) ([[:digit:]]+)$", REG_EXTENDED|REG_ICASE);
	if (regexec(&mfm_rx, line, 0, NULL, 0) == 0) {
		char mfm_cmd[3]; // TODO: this is redundant, only here so i can do the sscanf below - can prob get rid somehow
		int fm_no = 0;
		char osc[3];
		int freq = 0;
		sscanf(line, "%s %d %s %d", mfm_cmd, &fm_no, osc, &freq);
		if (fm_no + 1 <= mixr->fmsig_num) {
			printf("Ooh, gotsa an mfm for FM %d - changing %s to %d\n", fm_no, osc, freq);
			mfm(mixr->fmsignals[fm_no], osc, freq);
		} else {
		  printf("Beat it, ya chancer - gimme an FM number for one that exists!\n");
		}

	}

  regmatch_t tpmatch[4];
  regex_t tsigtype_rx;
  regcomp(&tsigtype_rx, "^(vol|freq|fm) ([[:digit:]]+) ([[:digit:]]+)$", REG_EXTENDED|REG_ICASE);
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
    if (strcmp(cmd_type, "fm") == 0) {
      add_fm(mixr, sig, val);
      printf("FML!\n");
    }
  }
  regmatch_t lmatch[3];
  regex_t loop_rx;
  //regcomp(&loop_rx, "^loop ([[:digit:]]+) ([[:digit:]]+) ([[:digit:]]+)$", REG_EXTENDED|REG_ICASE);
  regcomp(&loop_rx, "loop ([[:digit:][:space:]]+) ([[:digit:]]{1,2})$", REG_EXTENDED|REG_ICASE);
  if (regexec(&loop_rx, line, 3, lmatch, 0) == 0) {

    int loop_match_len = lmatch[2].rm_eo - lmatch[2].rm_so;
    char loop_len_char[loop_match_len];
    strncpy(loop_len_char, line+lmatch[2].rm_so, loop_match_len);
    int loop_len = atoi(loop_len_char);

    int frq_len = lmatch[1].rm_eo - lmatch[1].rm_so;
    char freaks[frq_len];
    strncpy(freaks, line+lmatch[1].rm_so, frq_len);
    printf("Freaks! %s\n", freaks);

    freaky* freqs = new_freqs_from_string(freaks);

    printf("LOOP LEN IS %d\n", loop_len);
    printf("NUM OF FREAKS IN HERE IS %d\n", freqs->num_freaks);
    for (int i = 0; i < freqs->num_freaks; i++) {
      printf("FREQ[%d] = %d\n", i, freqs->freaks[i]);
    }

    melody_msg *mm = new_melody_msg(freqs->freaks, freqs->num_freaks, loop_len);
    pthread_t melody_th;
    if ( pthread_create (&melody_th, NULL, algo_run, mm)) {
      fprintf(stderr, "Errr running melody\n");
      return;
    }
    pthread_detach(melody_th);
  }
}

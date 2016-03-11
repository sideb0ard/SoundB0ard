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
}

int exxit()
{
    printf("\nBeat it, ya val jerk...\n");
    pa_teardown();
    exit(0);
}

void interpret(char *line)
{
  char *tok, *last_s;
  char *sep = ",";
  for (tok = strtok_r(line, sep, &last_s);
       tok;
       tok = strtok_r(NULL, sep, &last_s))
  {
    char *trim_tok = calloc(strlen(tok), sizeof(char));
    strim(tok, trim_tok);

    if (strcmp(trim_tok, "ps") == 0) {
      ps();
      return;
    } else if (strcmp(trim_tok, "song") == 0) {
      pthread_t songrun_th;
      if ( pthread_create (&songrun_th, NULL, algo_run, NULL)) {
        fprintf(stderr, "Errr running song\n");
        return;
      }
      pthread_detach(songrun_th);
    } else if (strcmp(trim_tok, "ls") == 0) {
      list_sample_dir();
    } else if (strcmp(trim_tok, "addeffect") == 0) {
      add_effect(mixr);
    } else if (strcmp(trim_tok, "delay") == 0) {
      delay_toggle(mixr);
    } else if (strcmp(trim_tok, "exit") == 0) {
      exxit();
    }

    // TODO: move regex outside function to compile once
    // SINE|SAW|TRI (FREQ)
    regmatch_t pmatch[3];
    regex_t cmdtype_rx;
    regcomp(&cmdtype_rx, "^(bpm|duck|stop|sine|sawd|sawu|tri|square|vol) ([[:digit:].]+)$", REG_EXTENDED|REG_ICASE);

    if (regexec(&cmdtype_rx, trim_tok, 3, pmatch, 0) == 0) {

      float val = 0;
      char cmd[20];
      SBMSG *msg = new_sbmsg();

      sscanf(trim_tok, "%s %f", cmd, &val);
      msg->freq = val;

      if (strcmp(cmd, "bpm") == 0) {
        bpm_change(b, val);
        update_envelope_stream_bpm(ampstream);
      //} else if (strcmp(cmd, "vol") == 0) {
      //  printf("VOLLY BALL\n");
      //  mixer_vol_change(mixr, val);
      } else if (strcmp(cmd, "stop") == 0) {
        msg->sound_gen_num = val;
        strncpy(msg->cmd, "faderrr", 19);
        thrunner(msg);
      } else if (strcmp(cmd, "duck") == 0) {
        msg->sound_gen_num = val;
        strncpy(msg->cmd, "duckrrr", 19);
        thrunner(msg);
      } else {
        strncpy(msg->cmd, "timed_sig_start", 19);
        strncpy(msg->params, cmd, 10);
        thrunner(msg);
      }
    }

    // Modify an FM
    regex_t mfm_rx;
    regcomp(&mfm_rx, "^mfm ([[:digit:]]+) (mod|car) ([[:digit:]]+)$", REG_EXTENDED|REG_ICASE);
    if (regexec(&mfm_rx, trim_tok, 0, NULL, 0) == 0) {
      // TODO: check this can only work on an FM
      //char osc[3];
      //int fmno;
      //int freq = 0;
      //printf("ORIG trim_tok: %s\n", trim_tok);
      int fmno;
      int freq;
      char osc[4];
      sscanf(trim_tok, "mfm %d %s %d", &fmno, osc, &freq);
      if (fmno + 1 <= mixr->soundgen_num) {
      	printf("Ooh, gotsa an mfm for FM %d - changing %s to %d\n", fmno, osc, freq);
      	mfm(mixr->sound_generators[fmno], osc, freq);
      } else {
        printf("Beat it, ya chancer - gimme an FM number for one that exists!\n");
      }
    }

    regmatch_t tpmatch[4];
    regex_t tsigtype_rx;
    regcomp(&tsigtype_rx, "^(vol|freq|effect|fm) ([[:digit:]]+) ([[:digit:]]+)$", REG_EXTENDED|REG_ICASE);
    if (regexec(&tsigtype_rx, trim_tok, 3, tpmatch, 0) == 0) {
      double val1 = 0;
      double val2 = 0;
      char cmd_type[10];
      sscanf(trim_tok, "%s %lf %lf", cmd_type, &val1, &val2);
      if (strcmp(cmd_type, "vol") == 0) {
        vol_change(mixr, val1, val2);
        printf("VOL! %s %f %lf\n", cmd_type, val1, val2);
      }
      if (strcmp(cmd_type, "freq") == 0) {
        freq_change(mixr, val1, val2);
        printf("FREQ! %s %lf %lf\n", cmd_type, val1, val2);
      }
      if (strcmp(cmd_type, "effect") == 0) {
        if ( mixr->soundgen_num > val1 && mixr->effects_num > val2) {
          printf("EFFECT CALLED FOR! %s %.lf %.lf\n", cmd_type, val1, val2);
          link_effect(mixr->sound_generators[(int)val1], val2);
        } else {
          printf("Oofft mate, you don't have enough sound_gens or effects for that..\n");
        }
      }
      if (strcmp(cmd_type, "fm") == 0) {
        printf("FML!\n");
        SBMSG *msg = new_sbmsg();
        strncpy(msg->cmd, "timed_sig_start", 19);
        strncpy(msg->params, cmd_type, 10);
        msg->modfreq = val1;
        msg->carfreq = val2;
        thrunner(msg);

        //add_fm(mixr, sig, val);
      }
    }
    // loop sine waves
    regmatch_t lmatch[3];
    regex_t loop_rx;
    regcomp(&loop_rx, "^loop ([[:digit:][:space:]]+) ([[:digit:]]{1,2})$", REG_EXTENDED|REG_ICASE);
    if (regexec(&loop_rx, trim_tok, 3, lmatch, 0) == 0) {

      int loop_match_len = lmatch[2].rm_eo - lmatch[2].rm_so;
      char loop_len_char[loop_match_len + 1];
      strncpy(loop_len_char, trim_tok+lmatch[2].rm_so, loop_match_len);
      loop_len_char[loop_match_len] = '\0';
      int loop_len = atoi(loop_len_char);

      int frq_len = lmatch[1].rm_eo - lmatch[1].rm_so;
      char freaks[frq_len + 1];
      strncpy(freaks, trim_tok+lmatch[1].rm_so, frq_len);
      freaks[frq_len] = '\0';
      printf("Freaks! %s\n", freaks);

      freaky* freqs = new_freqs_from_string(freaks);

      printf("LOOP LEN IS %d\n", loop_len);
      printf("NUM OF FREAKS IN HERE IS %d\n", freqs->num_freaks);
      for (int i = 0; i < freqs->num_freaks; i++) {
        printf("FREQ[%d] = %d\n", i, freqs->freaks[i]);
      }

      melody_msg *mm = new_melody_msg(freqs->freaks, freqs->num_freaks, loop_len);
      pthread_t melody_th;
      if ( pthread_create (&melody_th, NULL, loop_run, mm)) {
        fprintf(stderr, "Errr running melody\n");
        return;
      }
      pthread_detach(melody_th);
    }

    // loop FMs
    regmatch_t flmatch[4];
    regex_t floop_rx;
    regcomp(&floop_rx, "^floop ([[:digit:]]+) ([[:digit:][:space:]]+) ([[:digit:]]{1,2})$", REG_EXTENDED|REG_ICASE);
    if (regexec(&floop_rx, trim_tok, 4, flmatch, 0) == 0) {

      printf("FLOOP CALLED\n");

      int mod_freq_len = flmatch[1].rm_eo - flmatch[1].rm_so;
      char mod_freq_char[mod_freq_len + 1];
      strncpy(mod_freq_char, trim_tok+flmatch[1].rm_so, mod_freq_len);
      mod_freq_char[mod_freq_len] = '\0';
      int mod_freq = atoi(mod_freq_char);
      printf("FLOOP PASSED #1 - modulator freq us %d\n", mod_freq);

      int floop_match_len = flmatch[3].rm_eo - flmatch[3].rm_so;
      printf("START: %lld, END: %lld\n", flmatch[3].rm_eo, flmatch[3].rm_so);
      printf("FLOOP MATCH LEN is %d\n", floop_match_len);
      char floop_len_char[floop_match_len + 1];
      strncpy(floop_len_char, trim_tok+flmatch[3].rm_so, floop_match_len);
      floop_len_char[floop_match_len] = '\0';
      int floop_len = atoi(floop_len_char);
      printf("FLOOP LEN %d\n", floop_len);

      int frq_len = flmatch[2].rm_eo - flmatch[2].rm_so;
      char freaks[frq_len + 1];
      strncpy(freaks, trim_tok+flmatch[2].rm_so, frq_len);
      freaks[frq_len] = '\0';
      printf("FMreaks! %s\n", freaks);
      printf("FLOOP PASSED #3\n");

      freaky* freqs = new_freqs_from_string(freaks);

      printf("FMLOOP LEN IS %d\n", floop_len);
      printf("NUM OF FMMMM FREAKS IN HERE IS %d\n", freqs->num_freaks);
      for (int i = 0; i < freqs->num_freaks; i++) {
        printf("FMFREQ[%d] = %d\n", i, freqs->freaks[i]);
      }

      melody_msg *mm = new_melody_msg(freqs->freaks, freqs->num_freaks, floop_len);
      mm->mod_freq = mod_freq;
      pthread_t melody_th;
      if ( pthread_create (&melody_th, NULL, floop_run, mm)) {
        fprintf(stderr, "Errr running melody\n");
        return;
      }
      pthread_detach(melody_th);
    }

    // file sample play
    regmatch_t fmatch[4];
    regex_t file_rx;
    regcomp(&file_rx, "^(file|play) ([.[:alnum:]]+) ([[:digit:][:space:]]+)$", REG_EXTENDED|REG_ICASE);
    if (regexec(&file_rx, trim_tok, 4, fmatch, 0) == 0) {

      int filename_len = fmatch[2].rm_eo - fmatch[2].rm_so;
      char filename[filename_len + 1];
      strncpy(filename, trim_tok+fmatch[2].rm_so, filename_len);
      filename[filename_len] = '\0';

      int pattern_len = fmatch[3].rm_eo - fmatch[3].rm_so;
      char pattern[pattern_len + 1];
      strncpy(pattern, trim_tok+fmatch[3].rm_so, pattern_len);
      pattern[pattern_len] = '\0';

      add_sample(mixr, filename, pattern);

    }
  }
}

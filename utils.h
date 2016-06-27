#include "sbmsg.h"

// void startrrr(int sig_num);
// void timed_sig_start(const char *sigtype, int freq, int freq2);
// void *algo_run(void *);

typedef struct freaky {
    int num_freaks;
    double *freaks;
} freaky;

void thrunner(SBMSG *msg);
void *timed_sig_start(void *arg);
void *fadeup_runrrr(void *arg);
void *fadedown_runrrr(void *arg);
void *duck_runrrr(void *arg);
void faderrr(int sig_num, direction d);
freaky *new_freqs_from_string(char *string);
float freqval(char *n);
int notelookup(char *n);
float chfreqlookup(int ch);
void list_sample_dir(void);
void strim(const char *input, char *result);
void chordie(char *n);
int conv_bitz(int num);
int is_valid_osc(char *string);

#include "defjams.h"

typedef void (*tune)();

typedef struct t_algoriddim 
{
  tune t;
} algo;


sbmsg* new_sb_msg(void);
algo *new_algo(void);

void *algo_run(void *);
void play_melody(const int osc_num, int *mlock, int *note, int *notes, const int note_len);
void mk_sbmsg_sine(int freq);

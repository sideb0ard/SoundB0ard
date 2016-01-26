typedef void (*tune)();

typedef struct t_algoriddim 
{
  tune t;
} algo;


algo *new_algo(void);
void *algo_run(void *);
//void algo_run(void *b);
//void bpm_info(bpmrrr *b);


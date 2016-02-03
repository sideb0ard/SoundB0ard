typedef struct {
  char sig_type[10];
  int freq;
} sig_msg;

void *timed_sig_start(void *arg);
void startrrr(int sig_num);
//void timed_sig_start(const char *sigtype, int freq, int freq2);
//void *algo_run(void *);

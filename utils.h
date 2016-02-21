void thrunner(sbmsg *msg);
void *timed_sig_start(void *arg);
void *fade_runrrr(void *arg);
//void startrrr(int sig_num);
void faderrr(int sig_num, direction d);
//void timed_sig_start(const char *sigtype, int freq, int freq2);
//void *algo_run(void *);
//
typedef struct freaky {
    int num_freaks;
    int *freaks;
} freaky;

freaky* new_freqs_from_string(char* string);
void list_sample_dir(void);

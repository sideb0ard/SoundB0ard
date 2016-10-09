#ifndef KEYS_H_
#define KEYS_H_

void keys(int soundgen_num);

// void play_note(int sg_num, double freq, int drone);
void play_note(void *self, double freq);
void *play_melody_loop(void *m);

// melody_loop *mloop_from_pattern(char *pattern);
void keys_start_melody_player(int sig_num, char *pattern);

#endif // KEYS_H_

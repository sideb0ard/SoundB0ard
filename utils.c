#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <pthread.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "algoriddim.h"
#include "bpmrrr.h"
#include "defjams.h"
#include "mixer.h"
#include "utils.h"

extern bpmrrr *b;
extern mixer *mixr;
extern GTABLE *sine_table;
extern GTABLE *saw_down_table;
extern GTABLE *saw_up_table;
extern GTABLE *tri_table;
extern GTABLE *square_table;

static char *rev_lookup[12] = {"c",  "c#", "d",  "d#", "e",  "f",
                               "f#", "g",  "g#", "a",  "a#", "b"};

void *timed_sig_start(void *arg)
{
    SBMSG *msg = arg;
    int sg = -1; // signal generator

    if (strcmp(msg->params, "sine") == 0) {
        sg = add_osc(mixr, msg->freq, sine_table);
    }
    else if (strcmp(msg->params, "sawd") == 0) {
        sg = add_osc(mixr, msg->freq, saw_down_table);
    }
    else if (strcmp(msg->params, "sawu") == 0) {
        sg = add_osc(mixr, msg->freq, saw_up_table);
    }
    else if (strcmp(msg->params, "tri") == 0) {
        sg = add_osc(mixr, msg->freq, tri_table);
    }
    else if (strcmp(msg->params, "square") == 0) {
        sg = add_osc(mixr, msg->freq, square_table);
    }
    else if (strcmp(msg->params, "fmx") == 0) {
        sg = add_fm_x(mixr, msg->mod_osc, msg->modfreq, msg->car_osc,
                      msg->carfreq);
    }
    else if (strcmp(msg->params, "fm") == 0) {
        sg = add_fm(mixr, msg->modfreq, msg->carfreq);
    }
    else if (strcmp(msg->params, "sloop") == 0) {
        printf("TIMED .... %f\n", msg->looplen);
        sg = add_sampler(mixr, msg->filename, msg->looplen);
    }
    else if (strcmp(msg->params, "bitwize") == 0) {
        sg = add_bitwize(mixr, msg->freq);
    }

    // faderrr(sg, UP);

    free(msg);
    return NULL;
}

void *fadeup_runrrr(void *arg)
{
    SBMSG *msg = arg;
    faderrr(msg->sound_gen_num, UP);

    return NULL;
}
void *fadedown_runrrr(void *arg)
{
    SBMSG *msg = arg;
    faderrr(msg->sound_gen_num, DOWN);

    return NULL;
}

void *duck_runrrr(void *arg)
{
    SBMSG *msg = arg;
    printf("Duckin' %d\n", msg->sound_gen_num);
    faderrr(msg->sound_gen_num, DOWN);
    sleep(rand() % 15);
    faderrr(msg->sound_gen_num, UP);

    return NULL;
}

void thrunner(SBMSG *msg)
{
    // need to ensure and free(msg) in all subtasks from here
    printf("Got CMD: %s\n", msg->cmd);
    pthread_t pthrrrd;
    if (strcmp(msg->cmd, "timed_sig_start") == 0) {
        if (pthread_create(&pthrrrd, NULL, timed_sig_start, msg)) {
            fprintf(stderr, "Err, running phrrread..\n");
            return;
        }
    }
    else if (strcmp(msg->cmd, "fadeuprrr") == 0) {
        if (pthread_create(&pthrrrd, NULL, fadeup_runrrr, msg)) {
            fprintf(stderr, "Err, running phrrread..\n");
            return;
        }
    }
    else if (strcmp(msg->cmd, "fadedownrrr") == 0) {
        if (pthread_create(&pthrrrd, NULL, fadedown_runrrr, msg)) {
            fprintf(stderr, "Err, running phrrread..\n");
            return;
        }
    }
    else if (strcmp(msg->cmd, "randdrum") == 0) {
        if (pthread_create(&pthrrrd, NULL, randdrum_run, msg)) {
            fprintf(stderr, "Err, running RANDRUND phrrread..\n");
            return;
        }
    }
    else if (strcmp(msg->cmd, "duckrrr") == 0) {
        if (pthread_create(&pthrrrd, NULL, duck_runrrr, msg)) {
            fprintf(stderr, "Err, running phrrread..\n");
            return;
        }
    }
}

void faderrr(int sg_num, direction d)
{

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 500000;
    double vol = 0;

    if (d == UP) {
        while (vol < 0.6) {
            vol += 0.0001;
            mixr->sound_generators[sg_num]->setvol(
                mixr->sound_generators[sg_num], vol);
            nanosleep(&ts, NULL);
        }
        mixr->sound_generators[sg_num]->setvol(mixr->sound_generators[sg_num],
                                               0.6);
    }
    else {
        double vol = mixr->sound_generators[sg_num]->getvol(
            mixr->sound_generators[sg_num]);
        while (vol > 0.0) {
            vol -= 0.0001;
            mixr->sound_generators[sg_num]->setvol(
                mixr->sound_generators[sg_num], vol);
            nanosleep(&ts, NULL);
        }
        mixr->sound_generators[sg_num]->setvol(mixr->sound_generators[sg_num],
                                               0.0);
    }
}

freaky *new_freqs_from_string(char *string)
{
    char *ap, *ap_last, *fargv[8];
    int freq_count = 0;
    char *sep = " ";

    for (ap = strtok_r(string, sep, &ap_last); ap;
         ap = strtok_r(NULL, sep, &ap_last)) {
        fargv[freq_count++] = ap;
    }

    freaky *f = calloc(1, sizeof(freaky));
    f->num_freaks = freq_count;
    f->freaks = calloc(freq_count, sizeof(int));

    for (int i = 0; i < freq_count; i++) {
        f->freaks[i] = atof(fargv[i]);
    }
    return f;
}

void list_sample_dir()
{
    DIR *dp;
    struct dirent *ep;
    dp = opendir("./wavs");
    if (dp != NULL) {
        while ((ep = readdir(dp)))
            puts(ep->d_name);
        (void)closedir(dp);
    }
    else {
        perror("Couldn't open wavs dir\n");
    }
}

void chordie(char *n)
{
    int root_note_num = notelookup(n);
    int second_note_num = (root_note_num + 4) % 12;
    int third_note_num = (second_note_num + 3) % 12;
    // TODO : cleanup - assuming always middle chord here:
    char rootnote[4];
    char sec_note[4];
    char thr_note[4];
    strcpy(rootnote, rev_lookup[root_note_num]);
    strcpy(sec_note, rev_lookup[second_note_num]);
    strcpy(thr_note, rev_lookup[third_note_num]);

    printf("%s chord is %s(%.2f) %s(%.2f) %s(%.2f)\n", n, rootnote,
           freqval(strcat(rootnote, "4")), sec_note,
           freqval(strcat(sec_note, "4")), thr_note,
           freqval(strcat(thr_note, "4")));
}

void related_notes(char note[4], double *second_note, double *third_note)
{
    char root_note;
    int scale;
    sscanf(note, "%[a-z#]%d", &root_note, &scale);

    char scale_ch[2];
    sprintf(scale_ch, "%d", scale);
    int second_note_num = (root_note + 4) % 12;
    int third_note_num = (root_note + 3) % 12;

    char sec_note[4];
    char thr_note[4];

    strcpy(sec_note, rev_lookup[second_note_num]);
    strcpy(thr_note, rev_lookup[third_note_num]);

    strcat(sec_note, scale_ch);
    strcat(thr_note, scale_ch);
    *second_note = freqval(sec_note);
    *third_note = freqval(thr_note);
}

int notelookup(char *n)
{
    // twelve semitones:
    // C C#/Db D D#/Eb E F F#/Gb G G#/Ab A A#/Bb B
    //
    if (!strcasecmp("c", n))
        return 0;
    else if (!strcasecmp("c#", n))
        return 1;
    else if (!strcasecmp("db", n))
        return 1;
    else if (!strcasecmp("d", n))
        return 2;
    else if (!strcasecmp("d#", n))
        return 3;
    else if (!strcasecmp("eb", n))
        return 3;
    else if (!strcasecmp("e", n))
        return 4;
    else if (!strcasecmp("f", n))
        return 5;
    else if (!strcasecmp("f#", n))
        return 6;
    else if (!strcasecmp("gb", n))
        return 6;
    else if (!strcasecmp("g", n))
        return 7;
    else if (!strcasecmp("g#", n))
        return 8;
    else if (!strcasecmp("ab", n))
        return 8;
    else if (!strcasecmp("a", n))
        return 9;
    else if (!strcasecmp("a#", n))
        return 10;
    else if (!strcasecmp("bb", n))
        return 10;
    else if (!strcasecmp("b", n))
        return 11;
    else
        return -1;
}

float chfreqlookup(int ch, void *fm)
{
    FM *self = (FM *)fm;
    char cur_octave[2];
    char cur_octave_plus_one[2];
    itoa(self->cur_octave, cur_octave);
    itoa(self->cur_octave + 1, cur_octave_plus_one);

    char tmpval[4] = "";
    if (ch == 97) {
        strcat(tmpval, "c");
        strcat(tmpval, cur_octave);
        return freqval(tmpval); // 'a' key
    }
    else if (ch == 119) {
        strcat(tmpval, "c#");
        strcat(tmpval, cur_octave);
        return freqval(tmpval); // 'w' key
    }
    else if (ch == 115) {
        strcat(tmpval, "d");
        strcat(tmpval, cur_octave);
        return freqval(tmpval); // 's' key
    }
    else if (ch == 101) {
        strcat(tmpval, "d#");
        strcat(tmpval, cur_octave);
        return freqval(tmpval); // 'e' key
    }
    else if (ch == 100) {
        strcat(tmpval, "e");
        strcat(tmpval, cur_octave);
        return freqval(tmpval); // 'd' key
    }
    else if (ch == 102) {
        strcat(tmpval, "f");
        strcat(tmpval, cur_octave);
        return freqval(tmpval); // 'f' key
    }
    else if (ch == 116) {
        strcat(tmpval, "f#");
        strcat(tmpval, cur_octave);
        return freqval(tmpval); // 't' key
    }
    else if (ch == 103) {
        strcat(tmpval, "g");
        strcat(tmpval, cur_octave);
        return freqval(tmpval); // 'g' key
    }
    else if (ch == 121) {
        strcat(tmpval, "g#");
        strcat(tmpval, cur_octave);
        return freqval(tmpval); // 'y' key
    }
    else if (ch == 104) {
        strcat(tmpval, "a");
        strcat(tmpval, cur_octave);
        return freqval(tmpval); // 'h' key
    }
    else if (ch == 117) {
        strcat(tmpval, "a#");
        strcat(tmpval, cur_octave);
        return freqval(tmpval); // 'u' key
    }
    else if (ch == 106) {
        strcat(tmpval, "b");
        strcat(tmpval, cur_octave);
        return freqval(tmpval); // 'j' key
    }
    else if (ch == 107) {
        strcat(tmpval, "c");
        strcat(tmpval, cur_octave_plus_one);
        return freqval(tmpval); // 'k' key
    }
    else if (ch == 111) {
        strcat(tmpval, "c#");
        strcat(tmpval, cur_octave_plus_one);
        return freqval(tmpval); // 'o' key
    }
    else if (ch == 108) {
        strcat(tmpval, "d");
        strcat(tmpval, cur_octave_plus_one);
        return freqval(tmpval); // 'l' key
    }
    else
        return -1;
}

float freqval(char *n)
{
    // algo from http://www.phy.mtu.edu/~suits/NoteFreqCalcs.html

    static float a4 = 440.0; // fixed note freq to use as baseline
    static const float twelfth_root_of_two = 1.059463094359;

    regmatch_t nmatch[3];
    regex_t single_letter_rx;
    regcomp(&single_letter_rx, "^([[:alpha:]#]{1,2})([[:digit:]])$",
            REG_EXTENDED | REG_ICASE);
    if (regexec(&single_letter_rx, n, 3, nmatch, 0) == 0) {

        int note_str_len = nmatch[1].rm_eo - nmatch[1].rm_so;
        char note[note_str_len + 1];
        strncpy(note, n + nmatch[1].rm_so, note_str_len);
        note[note_str_len] = '\0';

        char str_octave[2];
        strncpy(str_octave, n + note_str_len, 1);
        str_octave[1] = '\0';

        // purpose of this is working out how many semitones the given note is
        // from A4
        int n_num = (12 * atoi(str_octave)) + notelookup(note);
        // fixed note, which we compare against is A4 - '4' is the fourth
        // octave, so 4 * 12 semitones, plus lookup val of A is '9' - so 57
        int diff = n_num - 57;

        float freqval = a4 * (pow(twelfth_root_of_two, diff));
        return freqval;
    }
    else {
        return -1.0;
    }
}

void strim(const char *input, char *result)
{
    int flag = 0;

    while (*input) {
        if (!isspace((unsigned char)*input) && flag == 0) {
            *result++ = *input;
            flag = 1;
        }
        input++;
        if (flag == 1) {
            *result++ = *input;
        }
    }

    while (1) {
        result--;
        if (!isspace((unsigned char)*input) && flag == 0) {
            break;
        }
        flag = 0;
        *result = '\0';
    }
}

int conv_bitz(int num)
{
    for (int i = 0; i < 16; i++) {
        if ((num & (1 << i)) == num) {
            // printf("%d is %d\n", num, i);
            return i;
        }
    }
    return -1;
}

int is_valid_osc(char *string)
{
    if (strncmp(string, "square", 9) == 0) {
        return 1;
    }
    else if (strncmp(string, "saw_d", 9) == 0) {
        return 1;
    }
    else if (strncmp(string, "saw_u", 9) == 0) {
        return 1;
    }
    else if (strncmp(string, "tri", 9) == 0) {
        return 1;
    }
    else if (strncmp(string, "sine", 9) == 0) {
        return 1;
    }
    else {
        return 0;
    }
}

double pitch_shift_multiplier(double pitch_shift_semitones)
// from Will Pirkle book 'Designing Software Synthesizer Plug-Ins...'
{
    if (pitch_shift_semitones == 0)
        return 1.0;

    // 2^(N/12)
    //      return fastPow(2.0, dPitchShiftSemitones/12.0);
    return pow(2.0, pitch_shift_semitones / 12.0);
}

void calculate_pan_values(double pan_total, double *pan_left, double *pan_right)
{
    *pan_left = cos((M_PI / 4.0) * (pan_total + 1.0));
    *pan_right = sin((M_PI / 4.0) * (pan_total + 1.0));

    *pan_left = fmax(*pan_left, (double)0.0);
    *pan_left = fmin(*pan_left, (double)1.0);

    *pan_right = fmax(*pan_right, (double)0.0);
    *pan_right = fmin(*pan_right, (double)1.0);
}

// From K&R C - cut n pasted from
// https://en.wikibooks.org/wiki/C_Programming/C_Reference/stdlib.h/itoa
/* itoa:  convert n to characters in s */
void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0) /* record sign */
        n = -n;         /* make n positive */
    i = 0;
    do {                       /* generate digits in reverse order */
        s[i++] = n % 10 + '0'; /* get next digit */
    } while ((n /= 10) > 0);   /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

/* reverse:  reverse string s in place */
void reverse(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

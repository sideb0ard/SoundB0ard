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

#include "cmdloop.h"
#include "defjams.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;
extern char *strategies[NUM_STATEGIES];

static char *rev_lookup[12] = {"c",  "c#", "d",  "d#", "e",  "f",
                               "f#", "g",  "g#", "a",  "a#", "b"};

#define CONVEX_LIMIT 0.00398107
#define CONCAVE_LIMIT 0.99601893
#define ARC4RANDOMMAX 4294967295 // (2^32 - 1)

#define EXTRACT_BITS(the_val, bits_start, bits_len)                            \
    ((the_val >> (bits_start - 1)) & ((1 << bits_len) - 1))

// TODO ( this is superflous - no timing is going on )
void *timed_sig_start(void *arg)
{
    SBMSG *msg = (SBMSG *)arg;
    int sg = -1; // signal generator

    if (strcmp(msg->params, "sloop") == 0) {
        printf("TIMED .... %f\n", msg->looplen);
        sg = add_looper(mixr, msg->filename, msg->looplen);
    }
    // faderrr(sg, UP);

    // free(msg);
    return NULL;
}

void *fadeup_runrrr(void *arg)
{
    SBMSG *msg = (SBMSG *)arg;
    faderrr(msg->sound_gen_num, UP);

    return NULL;
}
void *fadedown_runrrr(void *arg)
{
    SBMSG *msg = (SBMSG *)arg;
    faderrr(msg->sound_gen_num, DOWN);

    return NULL;
}

void *duck_runrrr(void *arg)
{
    SBMSG *msg = (SBMSG *)arg;
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
        while (vol < 0.7) {
            vol += 0.0001;
            mixr->sound_generators[sg_num]->setvol(
                mixr->sound_generators[sg_num], vol);
            nanosleep(&ts, NULL);
        }
        mixr->sound_generators[sg_num]->setvol(mixr->sound_generators[sg_num],
                                               0.7);
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

// float chfreqlookup(int ch, void *p)
int ch_midi_lookup(int ch, void *p)
{
    minisynth *ms = (minisynth *)p;
    // int cur_octave = ns->cur_octave * 12;
    int default_octave = 4;
    int cur_octave = default_octave * 12;
    if (ms->m_octave != 0) {
        cur_octave = cur_octave + (ms->m_octave * 12);
    }
    int next_octave = cur_octave + 12;

    int midi_num = -1;

    switch (ch) {
    case 97:
        midi_num = 0 + cur_octave;
        break;
    case 119:
        midi_num = 1 + cur_octave;
        break;
    case 115:
        midi_num = 2 + cur_octave;
        break;
    case 101:
        midi_num = 3 + cur_octave;
        break;
    case 100:
        midi_num = 4 + cur_octave;
        break;
    case 102:
        midi_num = 5 + cur_octave;
        break;
    case 116:
        midi_num = 6 + cur_octave;
        break;
    case 103:
        midi_num = 7 + cur_octave;
        break;
    case 121:
        midi_num = 8 + cur_octave;
        break;
    case 104:
        midi_num = 9 + cur_octave;
        break;
    case 117:
        midi_num = 10 + cur_octave;
        break;
    case 106:
        midi_num = 11 + cur_octave;
        break;
    case 107:
        midi_num = 0 + next_octave;
        break;
    case 111:
        midi_num = 1 + next_octave;
        break;
    case 108:
        midi_num = 2 + next_octave;
        break;
        // default:
        //    midi_num = -1;
    }
    return midi_num;
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
    printf("CALC PAN VALS! PI: %f, pan_tota: %f\n", M_PI, pan_total);
    printf("PI / 4.0 : %f\n", M_PI / 4.0);
    printf("PI / 4.0 * pan_total : %f\n", (M_PI / 4.0) * (pan_total + 1.0));
    printf("COS! : %f\n", cos((M_PI / 4.0) * (pan_total + 1.0)));
    *pan_left = cos((M_PI / 4.0) * (pan_total + 1.0));
    *pan_right = sin((M_PI / 4.0) * (pan_total + 1.0));
    printf("PANLEFT[1] %f\n", *pan_left);
    printf("PANRIGHT[1] %f\n", *pan_right);

    *pan_left = fmax(*pan_left, (double)0.0);
    *pan_left = fmin(*pan_left, (double)1.0);
    printf("PANLEFT[2] %f\n", *pan_left);
    printf("PANRIGHT[2] %f\n", *pan_right);

    *pan_right = fmax(*pan_right, (double)0.0);
    *pan_right = fmin(*pan_right, (double)1.0);
    printf("PANLEFT[3] %f\n", *pan_left);
    printf("PANRIGHT[3] %f\n", *pan_right);
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

// scales cur_val which is range of cur_min, cur_max, to be a new_val within
// new_min, new_max
// algo from
// http://stackoverflow.com/questions/929103/convert-a-number-range-to-another-range-maintaining-ratio
double scaleybum(double cur_min, double cur_max, double new_min, double new_max,
                 double cur_val)
{
    double cur_range = cur_max - cur_min;
    double new_range = new_max - new_min;
    return (((cur_val - cur_min) / cur_range) * new_range) + new_min;
}

const double B = 4.0 / (float)M_PI;
const double C = -4.0 / ((float)M_PI * (float)M_PI);
const double P = 0.225;
// http://devmaster.net/posts/9648/fast-and-accurate-sine-cosine
// input is -pi to +pi
// i took it from the Will Pirkle Synth book.
double parabolic_sine(double x, bool high_precision)
{
    double y = B * x + C * x * fabs(x);

    if (high_precision)
        y = P * (y * fabs(y) - y) + y;
    return y;
}

double unipolar_to_bipolar(double value) { return 2.0 * value - 1.0; }

double convex_transform(double value)
{
    if (value <= CONVEX_LIMIT)
        return 0.0;

    return 1.0 + (5.0 / 12.0) * log10(value);
}

/* convexInvertedTransform()

        calculates the convexInvertedTransform of the input

        value = the unipolar (0 -> 1) value to convert
*/
double convex_inverted_transform(double value)
{
    if (value >= CONCAVE_LIMIT)
        return 0.0;

    return 1.0 + (5.0 / 12.0) * log10(1.0 - value);
}

/* concaveTransform()

        calculates the concaveTransform of the input

        value = the unipolar (0 -> 1) value to convert
*/
double concave_transform(double value)
{
    if (value >= CONCAVE_LIMIT)
        return 1.0;

    return -(5.0 / 12.0) * log10(1.0 - value);
}

/* concaveInvertedTransform()

        calculates the concaveInvertedTransform of the input

        value = the unipolar (0 -> 1) value to convert
*/
double concave_inverted_transform(double value)
{
    if (value <= CONVEX_LIMIT)
        return 1.0;

    return -(5.0 / 12.0) * log10(value);
}

double do_white_noise()
{
    float noise = 0.0;

    // fNoise is 0 -> ARC4RANDOMMAX
    noise = (float)arc4random();

    // normalize and make bipolar
    noise = 2.0 * (noise / ARC4RANDOMMAX) - 1.0;

    return noise;
}

double do_pn_sequence(unsigned *pn_register)
{
    // get the bits
    unsigned b0 =
        EXTRACT_BITS(*pn_register, 1, 1); // 1 = b0 is FIRST bit from right
    unsigned b1 =
        EXTRACT_BITS(*pn_register, 2, 1); // 1 = b1 is SECOND bit from right
    unsigned b27 =
        EXTRACT_BITS(*pn_register, 28, 1); // 28 = b27 is 28th bit from right
    unsigned b28 =
        EXTRACT_BITS(*pn_register, 29, 1); // 29 = b28 is 29th bit from right

    // form the XOR
    unsigned b31 = b0 ^ b1 ^ b27 ^ b28;

    // form the mask to OR with the register to load b31
    if (b31 == 1)
        b31 = 0x10000000;

    // shift one bit to right
    *pn_register >>= 1;

    // set the b31 bit
    *pn_register |= b31;

    // convert the output into a floating point number, scaled by
    // experimentation
    // to a range of o to +2.0
    float out = (float)(*pn_register) / ((pow((float)2.0, (float)32.0)) / 16.0);

    // shift down to form a result from -1.0 to +1.0
    out -= 1.0;

    return out;
}

void check_wrap_index(double *index)
{
    while (*index < 0.0)
        *index += 1.0;

    while (*index >= 1.0)
        *index -= 1.0;
}

double do_blep_n(const double *blep_table, double table_len, double modulo,
                 double inc, double height, bool rising_edge,
                 double points_per_side, bool interpolate)
{
    // return value
    double blep = 0.0;

    // t = the distance from the discontinuity
    double t = 0.0;

    // --- find the center of table (discontinuity location)
    double dTableCenter = table_len / 2.0 - 1;

    // LEFT side of edge
    // -1 < t < 0
    for (int i = 1; i <= (int)points_per_side; i++) {
        if (modulo > 1.0 - (double)i * inc) {
            // --- calculate distance
            t = (modulo - 1.0) / (points_per_side * inc);

            // --- get index
            float fIndex = (1.0 + t) * dTableCenter;

            // --- truncation
            if (interpolate) {
                blep = blep_table[(int)fIndex];
            }
            else {
                float fIndex = (1.0 + t) * dTableCenter;
                float frac = fIndex - (int)fIndex;
                blep = lin_terp(0, 1, blep_table[(int)fIndex],
                                blep_table[(int)fIndex + 1], frac);
            }

            // --- subtract for falling, add for rising edge
            if (!rising_edge)
                blep *= -1.0;

            return height * blep;
        }
    }

    // RIGHT side of discontinuity
    // 0 <= t < 1
    for (int i = 1; i <= (int)points_per_side; i++) {
        if (modulo < (double)i * inc) {
            // calculate distance
            t = modulo / (points_per_side * inc);

            // --- get index
            float fIndex = t * dTableCenter + (dTableCenter + 1.0);

            // truncation
            if (interpolate) {
                blep = blep_table[(int)fIndex];
            }
            else {
                float frac = fIndex - (int)fIndex;
                if ((int)fIndex + 1 >= table_len)
                    blep = lin_terp(0, 1, blep_table[(int)fIndex],
                                    blep_table[0], frac);
                else
                    blep = lin_terp(0, 1, blep_table[(int)fIndex],
                                    blep_table[(int)fIndex + 1], frac);
            }

            // subtract for falling, add for rising edge
            if (!rising_edge)
                blep *= -1.0;

            return height * blep;
        }
    }

    return 0.0;
}

float lin_terp(float x1, float x2, float y1, float y2, float x)
{
    float denom = x2 - x1;
    if (denom == 0)
        return y1; // should not ever happen

    // calculate decimal position of x
    float dx = (x - x1) / (x2 - x1);

    // use weighted sum method of interpolating
    float result = dx * y2 + (1 - dx) * y1;

    return result;
}

void print_midi_event(int midi_num)
{
    printf("MIDI ON tick: %d // relative tick: %d midi: %d\n", mixr->midi_tick,
           mixr->midi_tick % PPBAR, midi_num);
}

float fasttan(float x)
{
    static const float halfpi = 1.5707963267948966f;
    return sin(x) / sin(x + halfpi);
}

float fasttanh(float p) { return p / (fabs(2 * p) + 3 / (2 + 2 * p * 2 * p)); }

// no idea! taken from Will Pirkle books
float fastlog2(float x)
{
    union {
        float f;
        unsigned int i;
    } vx = {x};
    union {
        unsigned int i;
        float f;
    } mx = {(vx.i & 0x007FFFFF) | 0x3f000000};
    float y = vx.i;
    y *= 1.1920928955078125e-7f;

    return y - 124.22551499f - 1.498030302f * mx.f -
           1.72587999f / (0.3520887068f + mx.f);
}

double semitones_between_frequencies(double start_freq, double end_freq)
{
    return fastlog2(end_freq / start_freq) * 12.0;
}

double mma_midi_to_atten_db(unsigned int midi_val)
{
    if (midi_val == 0)
        return -96.0; // dB floor
    return 20.0 * log10((127.0 * 127.0) / ((float)midi_val * (float)midi_val));
}

bool is_int_member_in_array(int member_to_look_for, int *array_to_look_in, int size_of_array)
{
    for (int i = 0; i < size_of_array; i++)
    {
        if (array_to_look_in[i] == member_to_look_for) return true;
    }
    return false;
}

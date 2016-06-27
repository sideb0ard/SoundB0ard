#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "table.h"

static GTABLE *_new_gtable(void) {
    GTABLE *gtable = NULL;
    gtable = (GTABLE *)calloc(1, sizeof(GTABLE));
    if (gtable == NULL)
        return NULL;
    gtable->table = (double *)calloc(1, (TABLEN + 1) * sizeof(double));
    if (gtable->table == NULL) {
        free(gtable);
        return NULL;
    }
    gtable->length = TABLEN;
    return gtable;
}

static void norm_gtable(GTABLE *gtable) {
    unsigned long i;
    double val, maxamp = 0.0;

    for (i = 0; i < gtable->length; i++) {
        val = fabs(gtable->table[i]);
        if (maxamp < val)
            maxamp = val;
    }

    maxamp = 1.0 / maxamp;

    for (i = 0; i < gtable->length; i++)
        gtable->table[i] *= maxamp;

    gtable->table[i] = gtable->table[0];
}

// TABLE GENERATION SIGNALS
//
GTABLE *new_sine_table() {
    unsigned long i;
    double step;

    GTABLE *gtable = _new_gtable();
    if (gtable == NULL)
        return NULL;

    step = TWO_PI / TABLEN;
    for (i = 0; i < TABLEN; i++)
        gtable->table[i] = sin(step * i);
    gtable->table[i] = gtable->table[0]; // guard point

    return gtable;
}

GTABLE *new_tri_table() {
    unsigned long i, j;
    double step, amp;
    int harmonic = 1;

    GTABLE *gtable = _new_gtable();
    if (gtable == NULL)
        return NULL;

    step = TWO_PI / TABLEN;
    for (i = 0; i < NHARMS; i++) {
        amp = 1.0 / (harmonic * harmonic);
        for (j = 0; j < TABLEN; j++) {
            gtable->table[j] += amp * cos(step * harmonic * j);
        }
        harmonic += 2;
    }
    norm_gtable(gtable);
    return gtable;
}

GTABLE *new_square_table() {
    unsigned long i, j;
    double step, amp;
    int harmonic = 1;

    GTABLE *gtable = _new_gtable();
    if (gtable == NULL)
        return NULL;

    step = TWO_PI / TABLEN;

    for (i = 0; i < NHARMS; i++) {
        amp = 1.0 / harmonic;
        for (j = 0; j < TABLEN; j++) {
            gtable->table[j] += amp * sin(step * harmonic * j);
        }
        harmonic += 2;
    }
    norm_gtable(gtable);

    return gtable;
}

GTABLE *new_saw_table(int up) {
    unsigned long i, j;
    double step, val, amp = 1.0;
    int harmonic = 1;

    GTABLE *gtable = _new_gtable();
    if (gtable == NULL)
        return NULL;

    step = TWO_PI / TABLEN;
    if (up)
        amp = -1;

    for (i = 0; i < NHARMS; i++) {
        val = amp / harmonic;
        for (j = 0; j < gtable->length; j++)
            gtable->table[j] += val * sin(step * harmonic * j);
        harmonic++;
    }
    norm_gtable(gtable);
    return gtable;
}
// END TABLE GENERATION SIGNALS

void table_info(GTABLE *gtable) {
    printf("TABLE LEN: %lu\n", gtable->length);
    for (double i = 0; i < gtable->length; i++)
        printf("%f", gtable->table[(int)i]);
}

void gtable_free(GTABLE **gtable) {
    if (gtable && *gtable && (*gtable)->table) {
        free((*gtable)->table);
        free(*gtable);
        *gtable = NULL;
    }
}

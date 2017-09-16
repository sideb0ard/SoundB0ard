#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "table.h"

static wtable *_new_wtable(void)
{
    wtable *t = NULL;
    t = (wtable *)calloc(1, sizeof(wtable));
    if (t == NULL)
        return NULL;
    t->table = (double *)calloc(1, (TABLEN + 1) * sizeof(double));
    if (t->table == NULL)
    {
        free(t);
        return NULL;
    }
    t->length = TABLEN;
    return t;
}

static void norm_wtable(wtable *t)
{
    unsigned long i;
    double val, maxamp = 0.0;

    for (i = 0; i < t->length; i++)
    {
        val = fabs(t->table[i]);
        if (maxamp < val)
            maxamp = val;
    }

    maxamp = 1.0 / maxamp;

    for (i = 0; i < t->length; i++)
        t->table[i] *= maxamp;

    t->table[i] = t->table[0];
}

// TABLE GENERATION SIGNALS
//
wtable *new_sine_table()
{
    unsigned long i;
    double step;

    wtable *t = _new_wtable();
    if (t == NULL)
        return NULL;

    step = TWO_PI / TABLEN;
    for (i = 0; i < TABLEN; i++)
        t->table[i] = sin(step * i);
    t->table[i] = t->table[0]; // guard point

    return t;
}

wtable *new_tri_table()
{
    unsigned long i, j;
    double step, amp;
    int harmonic = 1;

    wtable *t = _new_wtable();
    if (t == NULL)
        return NULL;

    step = TWO_PI / TABLEN;
    for (i = 0; i < NHARMS; i++)
    {
        amp = 1.0 / (harmonic * harmonic);
        for (j = 0; j < TABLEN; j++)
        {
            t->table[j] += amp * cos(step * harmonic * j);
        }
        harmonic += 2;
    }
    norm_wtable(t);
    return t;
}

wtable *new_square_table()
{
    unsigned long i, j;
    double step, amp;
    int harmonic = 1;

    wtable *t = _new_wtable();
    if (t == NULL)
        return NULL;

    step = TWO_PI / TABLEN;

    for (i = 0; i < NHARMS; i++)
    {
        amp = 1.0 / harmonic;
        for (j = 0; j < TABLEN; j++)
        {
            t->table[j] += amp * sin(step * harmonic * j);
        }
        harmonic += 2;
    }
    norm_wtable(t);

    return t;
}

wtable *new_saw_table(int up)
{
    unsigned long i, j;
    double step, val, amp = 1.0;
    int harmonic = 1;

    wtable *t = _new_wtable();
    if (t == NULL)
        return NULL;

    step = TWO_PI / TABLEN;
    if (up)
        amp = -1;

    for (i = 0; i < NHARMS; i++)
    {
        val = amp / harmonic;
        for (j = 0; j < t->length; j++)
            t->table[j] += val * sin(step * harmonic * j);
        harmonic++;
    }
    norm_wtable(t);
    return t;
}
// END TABLE GENERATION SIGNALS

void wtable_info(wtable *t)
{
    printf("TABLE LEN: %lu\n", t->length);
    for (unsigned int i = 0; i < t->length; i++)
        printf("%f", t->table[i]);
}

void wtable_free(wtable **gtable)
{
    if (gtable && *gtable && (*gtable)->table)
    {
        free((*gtable)->table);
        free(*gtable);
        *gtable = NULL;
    }
}

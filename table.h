#pragma once

typedef struct wtable {
    double *table;
    unsigned long length;
} wtable;

wtable *new_sine_table(void);
wtable *new_tri_table(void);
wtable *new_square_table(void);
wtable *new_saw_table(int up); // 1 for Saw Up, 0 for down
wtable *new_env_table(void);

void wtable_free(wtable **table);
void wtable_info(wtable *table);

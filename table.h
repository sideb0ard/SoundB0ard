#ifndef GTABLE_H
#define GTABLE_H

typedef struct t_gtable {
    double *table;
    unsigned long length;
} GTABLE;

GTABLE *new_sine_table(void);
GTABLE *new_tri_table(void);
GTABLE *new_square_table(void);
GTABLE *new_saw_table(int up); // 1 for Saw Up, 0 for down
GTABLE *new_env_table(void);

void gtable_free(GTABLE **gtable);

void table_info(GTABLE *gtable);

#endif

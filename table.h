typedef struct t_gtable
{
  double* table;
  unsigned long length;
} GTABLE;

TABLE* new_sine_table(unsigned long length);
void gtable_free(GTABLE** gtable);

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  if (argc != 2) {
    printf("Beat it ya val jerk!\n");
    return -1;
  }

  int num = atoi(argv[1]);
  printf("Num: %d - 2/3: %f, 3/2: %f\n", num, ((double)num)*2/3, ((double)num)*3/2);
}


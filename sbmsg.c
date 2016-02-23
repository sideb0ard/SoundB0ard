#include <stdlib.h>
#include "sbmsg.h"

SBMSG* new_sbmsg()
{
  SBMSG* m = calloc(1, sizeof(SBMSG));
  return m;
}

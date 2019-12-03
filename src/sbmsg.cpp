#include "sbmsg.h"
#include <stdlib.h>

SBMSG *new_sbmsg()
{
    SBMSG *m = (SBMSG *)calloc(1, sizeof(SBMSG));
    return m;
}

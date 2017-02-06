#include "voice.h"
#include <stdlib.h>

voice *new_voice()
{
    voice *v = (voice *)calloc(1, sizeof(voice));
    if (!v) {
        printf("youch");
        return NULL;
    }

    return v;
}

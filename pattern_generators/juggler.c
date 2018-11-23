#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "juggler.h"
#include "mixer.h"
#include "pattern_generator.h"
#include "pattern_utils.h"
#include "utils.h"
#include <euclidean.h>

extern mixer *mixr;
static char *s_juggler_styles[] = {"COMPLEX"};

pattern_generator *new_juggler(unsigned int style)
{
    juggler *m = calloc(1, sizeof(juggler));
    if (!m)
    {
        printf("WOOF!\n");
        return NULL;
    }
    m->juggler_style = style;

    m->sg.status = &juggler_status;
    m->sg.generate = &juggler_generate;
    m->sg.event_notify = &juggler_event_notify;
    m->sg.set_debug = &juggler_set_debug;
    m->sg.type = JUGGLER;
    return (pattern_generator *)m;
}

void juggler_status(void *self, wchar_t *wstring)
{
    juggler *m = (juggler *)self;
    swprintf(wstring, MAX_STATIC_STRING_SZ,
             L"[" WANSI_COLOR_WHITE "JUGGLER PATTERN GEN ] - " WCOOL_COLOR_PINK
             "style:(%d)%s\n",
             m->juggler_style, s_juggler_styles[m->juggler_style]);
}

void juggler_generate(void *self, void *data)
{
    juggler *m = (juggler *)self;
    midi_event *midi_pattern = (midi_event*) data;

}

void juggler_set_debug(void *self, bool b) {}

void juggler_event_notify(void *self, unsigned int event_type) {}

void juggler_set_style(juggler *m, unsigned int style)
{
    if (style < NUM_JUGGLER_STYLES)
        m->juggler_style = style;
}

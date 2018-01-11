#include <stdio.h>
#include <stdlib.h>

#include <metronome.h>
#include <defjams.h>

//typedef struct metronome
//{
//    soundgenerator sound_generator;
//
//} metronome;

metronome *new_metronome()
{
    metronome *m = (metronome*) calloc(1, sizeof(metronome));
    printf("New Metronome!\n");

    m->sound_generator.gennext = &metronome_gennext;
    m->sound_generator.status = &metronome_status;
    m->sound_generator.setvol = &metronome_setvol;
    m->sound_generator.getvol = &metronome_getvol;
    m->sound_generator.start = &metronome_start;
    m->sound_generator.stop = &metronome_stop;
    m->sound_generator.event_notify = &metronome_event_notify;
    m->sound_generator.self_destruct = &metronome_del_self;
    m->sound_generator.type = METRONOME_TYPE;

    return m;
}


stereo_val metronome_gennext(void *self)
{
    return (stereo_val) {0, 0};
}

void metronome_status(void *self, wchar_t *ss)
{
    printf("I'm fine\n");
}
void metronome_setvol(void *self, double v)
{
    // noop
}
double metronome_getvol(void *self)
{
    return 0.;
}
void metronome_start(void *self)
{
    //noop
}

void metronome_stop(void *self)
{
    // noop
}
void metronome_make_active_track(void *self, int track_num)
{
    // noop
}

int metronome_get_num_tracks(void *self)
{
    // noop
    return 1;
}

void metronome_event_notify(void *self, unsigned int event_type)
{
    // noop
}
void metronome_del_self(void *self)
{
    metronome *m = (metronome*) self;
    free(m);
}

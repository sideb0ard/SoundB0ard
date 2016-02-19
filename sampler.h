#ifndef SAMPLER_H
#define SAMPLER_H

typedef struct t_sampler SAMPLER;
typedef double (*fp_gennext) (SAMPLER* sampler);

typedef struct t_sampler
{
  int pattern[32];
  int *buffer;
  int bufsize;
  int position;
  int playing;
  int samplerate;
  int channels;
  double vol;

  fp_gennext gen_next;

} SAMPLER;

SAMPLER* new_sampler(char * filename);

double f_gennext(SAMPLER *sampler);

#endif // SAMPLER_H

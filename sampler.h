#ifndef SAMPLER_H
#define SAMPLER_H

#define SAMPLE_PATTERN_LEN 16

typedef struct t_sampler SAMPLER;
typedef double (*fp_gennext) (SAMPLER* sampler);

typedef struct t_sampler
{
  char *filename;
  int pattern[SAMPLE_PATTERN_LEN];
  int *buffer;
  int bufsize;
  int position;
  int playing;
  int played;
  int samplerate;
  int channels;
  double vol;

  fp_gennext gen_next;

} SAMPLER;

SAMPLER* new_sampler(char *filename, char *pattern);

double f_gennext(SAMPLER *sampler);
void sample_status(SAMPLER *sampler, char *ss);

#endif // SAMPLER_H

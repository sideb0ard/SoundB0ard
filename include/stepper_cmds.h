#include <defjams.h>
#include <step_sequencer.h>

bool parse_stepper_cmd(int num_wurds, char wurds[][SIZE_OF_WURD]);
void parse_sample_sequencer_command(sequencer *seq, char wurds[][SIZE_OF_WURD],
                                    int num_wurds, char *pattern);

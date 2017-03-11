CC = clang
SRC = \
	  afx/combfilter.c \
 	  afx/delay.c \
	  afx/delayapf.c  \
	  afx/lpfcombfilter.c \
	  afx/onepolelpf.c \
      algorithm.c \
      audioutils.c \
      beatrepeat.c \
      bitwize.c \
      bytebeatrrr.c \
      bytebeat/interpreter.c \
      bytebeat/clist.c \
      bytebeat/dlist.c \
      bytebeat/list.c \
      bytebeat/stack.c \
      bytebeat/queue.c \
      chaosmonkey.c \
      cmdloop.c \
      dca.c \
	  ddlmodule.c \
      delayline.c \
      drumr.c \
      drumr_utils.c \
      effect.c \
      envelope.c \
      envelope_generator.c \
      filter.c \
      filter_ckthreefive.c \
      filter_moogladder.c \
      filter_onepole.c \
      filter_sem.c \
      help.c \
      keys.c \
      lfo.c \
      main.c \
      midi_freq_table.c \
      midimaaan.c \
      minisynth.c \
      minisynth_voice.c \
      mixer.c \
      modmatrix.c \
	  modular_delay.c \
      obliquestrategies.c \
      oscillator.c \
      qblimited_oscillator.c \
	  reverb.c \
      sampler.c \
      sbmsg.c \
      sparkline.c \
      stereodelay.c \
      sound_generator.c \
      table.c \
      utils.c \
      voice.c \
      wt_oscillator.c \

LIBS=-lportaudio -lportmidi -lreadline -lm -lpthread -lsndfile

INCDIR=/usr/local/include
LIBDIR=/usr/local/lib
CFLAGS = -std=c11 -Wall -Wextra -pedantic -Wstrict-prototypes -Wmissing-prototypes -g -I$(INCDIR)

OBJ=$(SRC:.c=.o)

TARGET = sbsh

.PHONE: depend clean

all: $(TARGET)
	@ctags *{h,c}
	@echo "\n\x1b[37mBoom! make some noise...\x1b[0m"

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -L$(LIBDIR) -o $(TARGET) $(OBJ) $(LIBS) $(INCS)

clean:
	rm -f *.o *~ $(TARGET)
	@echo "\n\x1b[37mCleannnnd..!\x1b[0m"

format:
	clang-format -i *{c,h}

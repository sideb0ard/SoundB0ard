CC = clang
CFLAGS = -std=c11 -Wall -Wextra -pedantic -Wstrict-prototypes -Wmissing-prototypes -g

SRC = \
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
      mixer.c \
      modmatrix.c \
      nanosynth.c \
      obliquestrategies.c \
      oscillator.c \
      qblimited_oscillator.c \
      sampler.c \
      sbmsg.c \
      stereodelay.c \
      sound_generator.c \
      table.c \
      utils.c \
      wt_oscillator.c \

LIBS = -lportaudio -lportmidi -lreadline -lm -lpthread -lsndfile

INCS = -Iinclude/ -I/Users/sideboard/NewCodez/SBShell -I/Users/sideboard/NewCodez/SBShell/bytebeat

OBJ = $(SRC:.c=.o)

TARGET = sbsh

.PHONE: depend clean

all: $(TARGET)
	@echo "\n\x1b[37mBoom! make some noise...\x1b[0m"

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LIBS) $(INC)

clean:
	rm -f *.o *~ $(TARGET)
	@echo "\n\x1b[37mCleannnnd..!\x1b[0m"

format:
	clang-format -i *{c,h}

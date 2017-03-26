CC = clang++
OBJ = \
	afx/combfilter.o \
	afx/delay.o \
	afx/delayapf.o  \
	afx/lpfcombfilter.o \
	afx/onepolelpf.o \
	algorithm.o \
	audioutils.o \
	beatrepeat.o \
	bitwize.o \
	bytebeatrrr.o \
	bytebeat/interpreter.o \
	bytebeat/clist.o \
	bytebeat/dlist.o \
	bytebeat/list.o \
	bytebeat/stack.o \
	bytebeat/queue.o \
	chaosmonkey.o \
	cmdloop.o \
	dca.o \
	ddlmodule.o \
	delayline.o \
	drumr.o \
	drumr_utils.o \
	effect.o \
	envelope.o \
	envelope_generator.o \
	filter.o \
	filter_ckthreefive.o \
	filter_moogladder.o \
	filter_onepole.o \
	filter_sem.o \
	help.o \
	keys.o \
	lfo.o \
	main.o \
	midi_freq_table.o \
	midimaaan.o \
	minisynth.o \
	minisynth_voice.o \
	mixer.o \
	modmatrix.o \
	modular_delay.o \
	obliquestrategies.o \
	oscillator.o \
	qblimited_oscillator.o \
	reverb.o \
	sampler.o \
	sbmsg.o \
	sparkline.o \
	standalonelfo.o \
	stereodelay.o \
	sound_generator.o \
	table.o \
	utils.o \
	voice.o \
	wt_oscillator.o \

LIBS=-lportaudio -lportmidi -lreadline -lm -lpthread -lsndfile

INCDIR=/usr/local/include
LIBDIR=/usr/local/lib
CFLAGS = -std=c11 -Wall -Wextra -pedantic -Wstrict-prototypes -Wmissing-prototypes -g -I$(INCDIR)

%.o: %.c
	$(CC) -c -o $@ -x c $< $(CFLAGS)

TARGET = sbsh

.PHONE: depend clean

all: $(TARGET)
	@ctags *{h,c}
	@echo "\n\x1b[37mBoom! make some noise...\x1b[0m"

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -L$(LIBDIR) -o $@ $^ $(LIBS) $(INCS)

clean:
	rm -f *.o *~ $(TARGET)
	@echo "\n\x1b[37mCleannnnd..!\x1b[0m"

format:
	clang-format -i *{c,h}

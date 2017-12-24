CC = clang++
SRC = \
	afx/biquad.c \
	afx/combfilter.c \
	afx/delay.c \
	afx/delayapf.c  \
	afx/lpfcombfilter.c \
	afx/onepolelpf.c \
	algorithm.c \
	arpeggiator.c \
	audiofile_data.c \
	audioutils.c \
	fx/basicfilterpass.c \
	fx/beatrepeat.c \
	bytebeat/bytebeat.c \
	chaosmonkey.c \
	cmdloop.c \
	dca.c \
	digisynth.c \
	digisynth_voice.c \
	dxsynth.c \
	dxsynth_voice.c \
	filterz/filter.c \
	filterz/filter_ckthreefive.c \
	filterz/filter_moogladder.c \
	filterz/filter_onepole.c \
	filterz/filter_sem.c \
	fx/bitcrush.c \
	fx/ddlmodule.c \
	fx/delayline.c \
	fx/distortion.c \
	fx/dynamics_processor.c \
	fx/envelope.c \
	fx/envelope_detector.c \
	fx/envelope_follower.c \
	fx/envelope_generator.c \
	fx/modular_delay.c \
	fx/stereodelay.c \
	fx/waveshaper.c \
	granulator.c \
	help.c \
	keys.c \
	lfo.c \
	looper.c \
	main.c \
	midi_freq_table.c \
	midimaaan.c \
	minisynth.c \
	minisynth_voice.c \
	mixer.c \
	modfilter.c \
	modmatrix.c \
	obliquestrategies.c \
	oscillator.c \
	qblimited_oscillator.c \
	reverb.c \
	sample_oscillator.c \
	sample_sequencer.c \
	sbmsg.c \
	sequencer.c \
	sequencer_utils.c \
	sequence_generator/sequence_generator.c \
	sequence_generator/interpreter.c \
	sequence_generator/stack.c \
	sequence_generator/queue.c \
	sequence_generator/list.c \
	sound_generator.c \
	sparkline.c \
	spork.c \
	synthbase.c \
	synthdrum_sequencer.c \
	synthfunctions.c \
	table.c \
	utils.c \
	voice.c \
	wt_oscillator.c

OBJDIR = obj
OBJ = $(patsubst %.c, $(OBJDIR)/%.o, $(SRC))
LIBS=-lportaudio -lportmidi -lreadline -lm -lpthread -lsndfile -lprofiler
INCDIRS=-I/usr/local/include -Iinclude -Iinclude/afx -Iinclude/stack
LIBDIR=/usr/local/lib
WARNFLASGS = -Wall -Wextra -pedantic -Wstrict-prototypes -Wmissing-prototypes
#CFLAGS = -std=c11 $(WARNFLAGS) -g -pg $(INCDIRS) -O3
CFLAGS = -std=c11 $(WARNFLAGS) -g -pg $(INCDIRS)

$(OBJDIR)/%.o: %.c
	$(CC) -c -o $@ -x c $< $(CFLAGS)

TARGET = sbsh

.PHONE: depend clean

all: objdir $(TARGET)
	@ctags -R *
	@echo "\n\x1b[37mBoom! make some noise...\x1b[0m"

objdir:
	mkdir -p obj/fx obj/filterz

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -L$(LIBDIR) -o $@ $^ $(LIBS) $(INCS)

clean:
	rm -f *.o *~ $(TARGET)
	find $(OBJDIR) -name "*.o" -exec rm {} \;
	@echo "\n\x1b[37mCleannnnd..!\x1b[0m"

format:
	find . -type f -name "*.[ch]" -exec clang-format -i {} \;

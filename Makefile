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
	audioutils.c \
	audiofile_data.c \
	basicfilterpass.c \
	beatrepeat.c \
	bitcrush.c \
	bytebeat/bytebeat.c \
	chaosmonkey.c \
	cmdloop.c \
	dca.c \
	ddlmodule.c \
	digisynth.c \
	digisynth_voice.c \
	distortion.c \
	delayline.c \
	dynamics_processor.c \
	sample_sequencer.c \
	sequencer.c \
	sequencer_utils.c \
	envelope.c \
	envelope_generator.c \
	envelope_follower.c \
	envelope_detector.c \
	filter.c \
	filter_ckthreefive.c \
	filter_moogladder.c \
	filter_onepole.c \
	filter_sem.c \
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
	modmatrix.c \
	modfilter.c \
	modular_delay.c \
	obliquestrategies.c \
	oscillator.c \
	qblimited_oscillator.c \
	reverb.c \
	sample_oscillator.c \
	sbmsg.c \
	sparkline.c \
	spork.c \
	stereodelay.c \
	sound_generator.c \
	synthbase.c \
	synthdrum_sequencer.c \
	table.c \
	utils.c \
	voice.c \
	wt_oscillator.c \
	waveshaper.c

OBJDIR = obj
OBJ = $(patsubst %.c, $(OBJDIR)/%.o, $(SRC))
LIBS=-lportaudio -lportmidi -lreadline -lm -lpthread -lsndfile -lprofiler
INCDIRS=-I/usr/local/include -Iinclude -Iinclude/afx
LIBDIR=/usr/local/lib
WARNFLASGS = -Wall -Wextra -pedantic -Wstrict-prototypes -Wmissing-prototypes
CFLAGS = -std=c11 $(WARNFLAGS) -g -pg $(INCDIRS) -O3

$(OBJDIR)/%.o: %.c
	$(CC) -c -o $@ -x c $< $(CFLAGS)

TARGET = sbsh

.PHONE: depend clean

all: $(TARGET)
	@ctags -R *
	@echo "\n\x1b[37mBoom! make some noise...\x1b[0m"

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -L$(LIBDIR) -o $@ $^ $(LIBS) $(INCS)

clean:
	rm -f *.o *~ $(TARGET)
	@echo "\n\x1b[37mCleannnnd..!\x1b[0m"

format:
	clang-format -i *{c,h}

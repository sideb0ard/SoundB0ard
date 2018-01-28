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
	metronome.c \
	midi_freq_table.c \
	midimaaan.c \
	minisynth.c \
	minisynth_voice.c \
	mixer.c \
	modfilter.c \
	modmatrix.c \
	obliquestrategies.c \
	oscillator.c \
	pattern_parser.c \
	pattern_transformers/pattern_transformers.c \
	qblimited_oscillator.c \
	reverb.c \
	sample_oscillator.c \
	sample_sequencer.c \
	sbmsg.c \
	step_sequencer.c \
	sequencer_utils.c \
	sound_generator.c \
	sequence_generators/bitshift.c \
	sequence_generators/euclidean.c \
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

ABLETONASIOINC=-I/Users/sideboard/Code/link/modules/asio-standalone/asio/include
INCDIRS=-I/usr/local/include -Iinclude -Iinclude/afx -Iinclude/stack -I/Users/sideboard/Code/link/include
LIBDIR=/usr/local/lib
WARNFLASGS = -Wall -Wextra -pedantic -Wstrict-prototypes -Wmissing-prototypes
#CFLAGS = -std=gnu11 $(WARNFLAGS) -g -pg $(INCDIRS) -O3
#CPPFLAGS = -std=gnu++11 $(WARNFLAGS) -g -pg $(INCDIRS) $(ABLETONASIOINC) -O0
CFLAGS = -std=c11 $(WARNFLAGS) -g -pg $(INCDIRS) -O3
CPPFLAGS = -std=c++11 $(WARNFLAGS) -g -pg $(INCDIRS) $(ABLETONASIOINC) -O3

$(OBJDIR)/%.o: %.c
	$(CC) -c -o $@ -x c $< $(CFLAGS)

TARGET = sbsh

.PHONE: depend clean

all: objdir $(TARGET)
	@ctags -R *
	@cscope -b
	@echo "\n\x1b[37mBoom! make some noise...\x1b[0m"

objdir:
	mkdir -p obj/fx obj/filterz obj/pattern_transformers

$(TARGET): $(OBJ)
	$(CC) $(CPPFLAGS) -L$(LIBDIR) -o $@ $^ ableton_link_wrapper.cpp $(LIBS) $(INCS)

clean:
	rm -f *.o *~ $(TARGET)
	find $(OBJDIR) -name "*.o" -exec rm {} \;
	@echo "\n\x1b[37mCleannnnd..!\x1b[0m"

format:
	find . -type f -name "*.[ch]" -exec clang-format -i {} \;

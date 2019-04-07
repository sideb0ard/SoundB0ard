CC = clang++
SRC = \
	afx/biquad.c \
	afx/combfilter.c \
	afx/delay.c \
	afx/delayapf.c  \
	afx/lpfcombfilter.c \
	afx/onepolelpf.c \
	algorithm.c \
	audiofile_data.c \
	audioutils.c \
	cmdloop.c \
	cmdloop/algo_cmds.c \
	cmdloop/fx_cmds.c \
	cmdloop/looper_cmds.c \
	cmdloop/midi_cmds.c \
	cmdloop/mixer_cmds.c \
	cmdloop/new_item_cmds.c \
	cmdloop/pattern_generator_cmds.c \
	cmdloop/sequence_engine_cmds.c \
	cmdloop/stepper_cmds.c \
	cmdloop/synth_cmds.c \
	cmdloop/value_generator_cmds.c \
	dca.c \
	digisynth.c \
	digisynth_voice.c \
	drumsampler.c \
	drumsynth.c \
	dxsynth.c \
	dxsynth_voice.c \
	filterz/filter.c \
	filterz/filter_ckthreefive.c \
	filterz/filter_moogladder.c \
	filterz/filter_onepole.c \
	filterz/filter_sem.c \
	fx/basicfilterpass.c \
	fx/beatrepeat.c \
	fx/bitcrush.c \
	fx/ddlmodule.c \
	fx/delayline.c \
	fx/distortion.c \
	fx/dynamics_processor.c \
	fx/envelope.c \
	fx/envelope_detector.c \
	fx/envelope_follower.c \
	fx/envelope_generator.c \
	fx/fx.c \
	fx/modular_delay.c \
	fx/stereodelay.c \
	fx/waveshaper.c \
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
	pattern_parser.c \
	pattern_utils.c \
	pattern_transformers/pattern_transformers.c \
	qblimited_oscillator.c \
	reverb.c \
	sample_oscillator.c \
	sbmsg.c \
	sound_generator.c \
	pattern_generators/bitshift.c \
	pattern_generators/euclidean.c \
	pattern_generators/intdiv.c \
	pattern_generators/juggler.c \
	pattern_generators/markov.c \
	sparkline.c \
	sequence_engine.c \
	synthfunctions.c \
	table.c \
	utils.c \
	voice.c \
	value_generators/value_generator.c \
	wt_oscillator.c

OBJDIR = obj
OBJ = $(patsubst %.c, $(OBJDIR)/%.o, $(SRC))
LIBS=-lportaudio -lportmidi -lreadline -lm -lpthread -lsndfile -lprofiler -llo

ABLETONASIOINC=-I/Users/sideboard/Code/link/modules/asio-standalone/asio/include
INCDIRS=-I/Users/sideboard/homebrew/Cellar/readline/7.0.3_1/include -I/usr/local/include -Iinclude -Iinclude/afx -Iinclude/stack -I/Users/sideboard/Code/link/include -I/Users/sideboard/homebrew/include
LIBDIR=/usr/local/lib
HOMEBREWLIBDIR=/Users/sideboard/homebrew/lib
READLINELIBDIR=/Users/sideboard/homebrew/Cellar/readline/7.0.3_1/lib
WARNFLASGS = -Wall -Wextra -pedantic -Wstrict-prototypes -Wmissing-prototypes
#CFLAGS = -std=gnu11 $(WARNFLAGS) -g -pg $(INCDIRS) -O3
#CPPFLAGS = -std=gnu++11 $(WARNFLAGS) -g -pg $(INCDIRS) $(ABLETONASIOINC) -O0
CFLAGS = -std=c11 $(WARNFLAGS) -g -pg $(INCDIRS) -O3
CPPFLAGS = -std=c++11 $(WARNFLAGS) -g -pg $(INCDIRS) $(ABLETONASIOINC) -O3 -fsanitize=address

$(OBJDIR)/%.o: %.c
	$(CC) -c -o $@ -x c $< $(CFLAGS)

TARGET = sbsh

.PHONE: depend clean

all: objdir $(TARGET)
	@ctags -R *
	@cscope -b
	@echo "\n\x1b[37mBoom! make some noise...\x1b[0m"

objdir:
	mkdir -p obj/fx obj/filterz obj/pattern_transformers obj/cmdloop obj/pattern_generators obj/value_generators obj/afx

$(TARGET): $(OBJ)
	$(CC) $(CPPFLAGS) -L$(READLINELIBDIR) -L$(LIBDIR) -L$(HOMEBREWLIBDIR) -o $@ $^ ableton_link_wrapper.cpp $(LIBS) $(INCS)

clean:
	rm -f *.o *~ $(TARGET)
	find $(OBJDIR) -name "*.o" -exec rm {} \;
	@echo "\n\x1b[37mCleannnnd..!\x1b[0m"

format:
	find . -type f -name "*.[ch]" -exec clang-format -i {} \;

CC = clang
CFLAGS = -std=c11 -Wall -Wextra -pedantic -Wstrict-prototypes -Wmissing-prototypes -g

SRC = main.c \
      audioutils.c \
      bitwize.c \
      bpmrrr.c \
      cmdloop.c \
      dca.c \
      drumr.c \
      effect.c \
      envelope.c \
      envelope_generator.c \
      fm.c \
      help.c \
      keys.c \
      mixer.c \
      oscil.c \
      sampler.c \
      sbmsg.c \
      sound_generator.c \
      table.c \
      utils.c \

LIBS = -lportaudio -lreadline -lm -lpthread -lsndfile

OBJ = $(SRC:.c=.o)

TARGET = sbsh

.PHONE: depend clean

all: $(TARGET)
	@echo "\n\x1b[37mBoom! make some noise...\x1b[0m"

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

clean:
	rm -f *.o *~ $(TARGET)
	@echo "\n\x1b[37mCleannnnd..!\x1b[0m"

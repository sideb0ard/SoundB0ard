CC = clang
CFLAGS = -std=c11 -Wall -Wextra -pedantic -Wstrict-prototypes -Wmissing-prototypes
SRC = main.c mixer.c oscil.c envelope.c cmdloop.c audioutils.c bpmrrr.c table.c algoriddim.c utils.c
LIBS = -lportaudio -lreadline -lm -lpthread
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

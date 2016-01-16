CC = clang
LIBS = -lportaudio
SRC = main.c mixer.c wave.c cmdinterpreter.c
OBJ = $(SRC:.c=.o)

TARGET = sbsh

.PHONE: depend clean

all: $(TARGET)
	@echo Boom! make some noise..

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $(OBJ) $(LIBS)

clean:
	rm *.o *~ $(TARGET)

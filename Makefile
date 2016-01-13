CC = clang

sbsh: main.c mixer.c wave.c cmdinterpreter.c
	$(CC) -o sbsh main.c mixer.c wave.c cmdinterpreter.c

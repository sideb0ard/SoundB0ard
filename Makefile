CC = clang

sbsh: main.c mixer.c
	$(CC) -o sbsh main.c mixer.c

CC=cc
main: main.c src/*.c src/include/*.h
	$(CC) main.c src/*.c -o main -Wno-unused-function -lSDL3 -lSDL3_image -std=c23 -fsanitize=address -Wimplicit -g

gdb:
	$(CC) main.c src/*.c -o main -Wno-unused-function -lSDL3 -lSDL3_image -std=c23 -Wimplicit -g
	gdb ./main

release:
	$(CC) main.c src/*.c -o main -Wno-unused-function -lSDL3 -lSDL3_image -std=c23 -Wimplicit -O3 -flto

clean:
	rm main

run: main
	./main

.PHONY: clean, run, release, gdb

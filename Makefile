CC ?= cc

build/debug: build main.c src/*.c src/include/*.h
	$(CC) main.c src/*.c -o build/debug -lSDL3 -lSDL3_image -std=c99 -fsanitize=address -Wimplicit -g -Wall -Wextra -Wno-unused-function

build:
	mkdir build

test: main.c src/*.c src/include/*.h
	$(CC) test/*.c src/*.c -o build/test -lSDL3 -lSDL3_image -std=c99 -fsanitize=address -O2 -flto
	./build/test

release:
	$(CC) main.c src/*.c -o build/release -lSDL3 -lSDL3_image -std=c99 -Wimplicit -O3 -flto

clean:
	rm -r build

run: build/debug
	./build/debug

.PHONY: clean, release, test

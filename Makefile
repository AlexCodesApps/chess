build/main: build
	cmake --build build

build: CMakeLists.txt
	cmake -S . -B build -G Ninja

test: build
	cmake --build build
	./build/test

release:
	cmake --build build

clean:
	rm -r build

run: build/main
	./build/main

.PHONY: clean, release, test

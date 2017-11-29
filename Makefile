# Edit this if your computer has SDL set up differently.

.PHONY: test sketch

test:
	gcc -std=c99 -pedantic -Wall -O3 sketch.c test.c -o sketch

sketch:
	gcc -std=c99 -pedantic -Wall -O3 sketch.c display.c -lSDL2 -o sketch

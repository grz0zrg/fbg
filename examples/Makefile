CC=gcc
STANDARD_FLAGS=-std=c11 -pedantic -D_GNU_SOURCE -D_POSIX_SOURCE -Wno-unused-value -Wno-unused-variable -Wno-unknown-pragmas
DEBUG_FLAGS=-DDEBUG -g -Wall
RELEASE_FLAGS=-O2 -Wall
DEFP=-DFBG_PARRALEL
SRC1=../src/lodepng/lodepng.c ../src/fbgraphics.c quickstart.c
SRC2=../src/lodepng/lodepng.c ../src/fbgraphics.c simple_parralel_example.c
SRC3=../src/lodepng/lodepng.c ../src/fbgraphics.c perlin.c full_example.c
OUT1=quickstart
OUT2=simple_parralel_example
OUT3=full_example
LIBS1=-lm
LIBS2=-lm -lpthread liblfds711.a
LIBS3=-lm -lpthread liblfds711.a
INCS=-I ../src/ -I.

all:
	$(CC) $(SRC1) $(INCS) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS1) -o $(OUT1)
	$(CC) $(SRC2) $(INCS) $(STANDARD_FLAGS) $(DEFP) $(RELEASE_FLAGS) $(LIBS2) -o $(OUT2)
	$(CC) $(SRC3) $(INCS) $(STANDARD_FLAGS) $(DEFP) $(RELEASE_FLAGS) $(LIBS3) -o $(OUT3)

debug:
	$(CC) $(SRC1) $(INCS) $(STANDARD_FLAGS) $(DEBUG_FLAGS) $(LIBS1) -o $(OUT1)
	$(CC) $(SRC2) $(INCS) $(STANDARD_FLAGS) $(DEFP) $(DEBUG_FLAGS) $(LIBS2) -o $(OUT2)
	$(CC) $(SRC3) $(INCS) $(STANDARD_FLAGS) $(DEFP) $(DEBUG_FLAGS) $(LIBS3) -o $(OUT3)

clean:
	rm -f *.o $(OUT)

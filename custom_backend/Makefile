CC=gcc
STANDARD_FLAGS=-std=c11 -pedantic -D_GNU_SOURCE -D_POSIX_SOURCE -Wno-unused-value -Wno-unknown-pragmas
DEBUG_FLAGS=-DDEBUG -g -Wall
RELEASE_FLAGS=-O2 -Wall
DEFP=-DFBG_PARALLEL
SRC_LIBS=../src/lodepng/lodepng.c ../src/fbgraphics.c glfw/fbg_glfw.c
SRC1=$(SRC_LIBS) glfw_example.c
OUT1=glfw_example
LIBS2=-lm -lpthread ../examples/liblfds720.a `pkg-config --static --libs glfw3` `pkg-config --libs glu` `pkg-config --libs glew`
INCS=-I ../src/ -I. `pkg-config --cflags glfw3 glu`

all:
	$(CC) $(SRC1) $(INCS) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS2) -o $(OUT1)

debug:
	$(CC) $(SRC1) $(INCS) $(STANDARD_FLAGS) $(DEBUG_FLAGS) $(LIBS2) -o $(OUT1)

clean:
	rm -f *.o $(OUT1)

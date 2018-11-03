CC=gcc
STANDARD_FLAGS=-std=c11 -pedantic -D_GNU_SOURCE -D_POSIX_SOURCE -Wno-unused-value -Wno-unknown-pragmas
DEBUG_FLAGS=-DDEBUG -g -Wall
RELEASE_FLAGS=-O2 -Wall
DEFP=-DFBG_PARALLEL
SRC_LIBS=../src/lodepng/lodepng.c ../src/nanojpeg/nanojpeg.c ../src/fbgraphics.c glfw/fbg_glfw.c
SRC_LIB1=../src/fbgraphics.c
SRC1=$(SRC_LIBS) glfw_example.c
SRC2=$(SRC_LIBS) glfw_example_advanced.c
SRC3=$(SRC_LIBS) glfw_parallel_raycasting.c
SRC4=$(SRC_LIBS) glfw_parallel.c
SRC5=$(SRC_LIBS) glfw_lua.c
OUT1=glfw_example
OUT2=glfw_example_advanced
OUT3=glfw_parallel_raycasting
OUT4=glfw_parallel
OUT5=glfw_lua
LIBS1=-lm -lpthread `pkg-config --static --libs glfw3` `pkg-config --libs glu` `pkg-config --libs glew`
LIBS2=../examples/liblfds720.a -lm -lpthread `pkg-config --static --libs glfw3` `pkg-config --libs glu` `pkg-config --libs glew`
LIBS3=../examples/liblfds711.a -lm -lpthread `pkg-config --static --libs glfw3` `pkg-config --libs glu` `pkg-config --libs glew`
INCS=-I ../src/ -I. -Iliblfds720 -Iluajit `pkg-config --cflags glfw3 glu`
INCS2=-I ../src/ -I. -Iliblfds711 -IIluajit `pkg-config --cflags glfw3 glu`

all:
	$(CC) $(SRC_LIB1) $(INCS) $(STANDARD_FLAGS) $(RELEASE_FLAGS) -fPIC -shared -o libfbg.so
	$(CC) $(SRC1) $(INCS) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS1) -o $(OUT1)
	$(CC) $(SRC2) $(INCS) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS1) -o $(OUT2)
	$(CC) $(SRC3) $(INCS) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS2) $(DEFP) -o $(OUT3)
	$(CC) $(SRC4) $(INCS) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS2) $(DEFP) -o $(OUT4)
	$(CC) $(SRC5) $(INCS) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS2) libluajit.a -ldl $(DEFP) -o $(OUT5)

debug:
	$(CC) $(SRC1) $(INCS) $(STANDARD_FLAGS) $(DEBUG_FLAGS) $(LIBS1) -o $(OUT1)
	$(CC) $(SRC2) $(INCS) $(STANDARD_FLAGS) $(DEBUG_FLAGS) $(LIBS1) -o $(OUT2)
	$(CC) $(SRC3) $(INCS) $(STANDARD_FLAGS) $(DEBUG_FLAGS) $(LIBS2) $(DEFP) -o $(OUT3)
	$(CC) $(SRC4) $(INCS) $(STANDARD_FLAGS) $(DEBUG_FLAGS) $(LIBS2) $(DEFP) -o $(OUT4)
	$(CC) $(SRC5) $(INCS) $(STANDARD_FLAGS) $(DEBUG_FLAGS) $(LIBS2) libluajit.a -ldl $(DEFP) -o $(OUT5)

lfds711:
	$(CC) $(SRC1) $(INCS2) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS1) -DLFDS711 -o $(OUT1)
	$(CC) $(SRC2) $(INCS2) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS1) -DLFDS711 -o $(OUT2)
	$(CC) $(SRC3) $(INCS2) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS3) -DLFDS711 $(DEFP) -o $(OUT3)
	$(CC) $(SRC4) $(INCS2) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS3) -DLFDS711 $(DEFP) -o $(OUT4)
	$(CC) $(SRC5) $(INCS2) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS3) libluajit.a -ldl -DLFDS711 $(DEFP) -o $(OUT5)

clean:
	rm -f *.o $(OUT1) $(OUT2) $(OUT3) $(OUT4) $(OUT5)

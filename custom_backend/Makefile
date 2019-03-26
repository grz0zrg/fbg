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
	$(CC) $(SRC1) $(INCS2) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS1) -DFBG_LFDS -DLFDS711 -o $(OUT1)
	$(CC) $(SRC2) $(INCS2) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS1) -DFBG_LFDS -DLFDS711 -o $(OUT2)
	$(CC) $(SRC3) $(INCS2) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS3) -DFBG_LFDS -DLFDS711 $(DEFP) -o $(OUT3)
	$(CC) $(SRC4) $(INCS2) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS3) libluajit.a -ldl -DFBG_LFDS -DLFDS711 $(DEFP) -o $(OUT4)

fbdeves2:
	$(CC) $(SRC_LIBS) opengl_es2/fbg_opengl_es2.c opengl_es2_example.c -I ../src/ $(STANDARD_FLAGS) -lEGL -lm -lpthread -o opengl_es2_example
	$(CC) $(SRC_LIBS) opengl_es2/fbg_opengl_es2.c opengl_es2_parallel.c -I ../src/ $(STANDARD_FLAGS) -I. -Iliblfds720 -DFBG_LFDS $(DEFP) ~/liblfds7.2.0/src/liblfds720/bin/liblfds720.a -lEGL -lm -lpthread -o opengl_es2_parallel

rpi:
	$(CC) $(SRC_LIBS) opengl_es2/fbg_opengl_es2.c opengl_es2_example.c -I ../src/ $(STANDARD_FLAGS) -DFBG_RPI -I/opt/vc/include/ -L/opt/vc/lib/ -lbrcmGLESv2 -lbrcmEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm -o opengl_es2_example
	$(CC) $(SRC_LIBS) opengl_es2/fbg_opengl_es2.c opengl_es2_parallel.c -I ../src/ $(STANDARD_FLAGS) -DFBG_RPI -I/opt/vc/include/ -I. -Iliblfds720 -DFBG_LFDS $(DEFP) ~/liblfds7.2.0/src/liblfds720/bin/liblfds720.a -L/opt/vc/lib/ -lbrcmGLESv2 -lbrcmEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm -o opengl_es2_parallel

clean:
	rm -f *.o $(OUT1) $(OUT2) $(OUT3) $(OUT4) $(OUT5)

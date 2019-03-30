CC=gcc
STANDARD_FLAGS=-std=c11 -pedantic -D_GNU_SOURCE -D_POSIX_SOURCE -Wno-unused-value -Wno-unknown-pragmas
DEBUG_FLAGS=-DDEBUG -g -Wall
RELEASE_FLAGS=-O2 -Wall
DEFP=-DFBG_PARALLEL
SRC_LIBS=../src/lodepng/lodepng.c ../src/nanojpeg/nanojpeg.c ../src/fbgraphics.c
SRC_LIB1=../src/fbgraphics.c
SRC_GLFW=glfw/fbg_glfw.c
SRC1=$(SRC_LIBS) $(SRC_GLFW) glfw_example.c
SRC2=$(SRC_LIBS) $(SRC_GLFW) glfw_example_advanced.c
SRC3=$(SRC_LIBS) $(SRC_GLFW) glfw_parallel.c
SRC4=$(SRC_LIBS) $(SRC_GLFW) glfw_lua.c
OUT1=glfw_example
OUT2=glfw_example_advanced
OUT3=glfw_parallel
OUT4=glfw_lua
LIBS1=-lm -lpthread `pkg-config --static --libs glfw3` `pkg-config --libs glu` `pkg-config --libs glew`
LIBS2= -lm -lpthread `pkg-config --static --libs glfw3` `pkg-config --libs glu` `pkg-config --libs glew`
LIBS3=-lm -lpthread `pkg-config --static --libs glfw3` `pkg-config --libs glu` `pkg-config --libs glew`
LIBS_LFDS_711=../examples/liblfds711.a
LIBS_LFDS_720=../examples/liblfds720.a
INCS=-I ../src/ -I. -Iluajit `pkg-config --cflags glfw3 glu`
INCS2=-I ../src/ -I. -IIluajit `pkg-config --cflags glfw3 glu`
INCS_LFDS_711=-Iliblfds711
INCS_LFDS_720=-Iliblfds720

tiny:
	$(CC) $(SRC_LIB1) $(INCS) $(STANDARD_FLAGS) -O1 -fPIC -shared -o libfbg.so
	$(CC) ../src/fbgraphics.c glfw/fbg_glfw.c glfw_example.c $(INCS) $(STANDARD_FLAGS) -fdata-sections -ffunction-sections -flto -DWITHOUT_PNG -DWITHOUT_JPEG -Os -lpthread `pkg-config --static --libs glfw3` `pkg-config --libs glew` -o $(OUT1) -Wl,--gc-sections,-flto
	$(CC) ../src/fbgraphics.c glfw/fbg_glfw.c glfw_parallel.c $(INCS) $(STANDARD_FLAGS) -fdata-sections -ffunction-sections -flto -DWITHOUT_PNG -DWITHOUT_JPEG -Os -lpthread `pkg-config --static --libs glfw3` `pkg-config --libs glew` $(DEFP) -o $(OUT3) -Wl,--gc-sections,-flto

all:
	$(CC) $(SRC_LIB1) $(INCS) $(STANDARD_FLAGS) $(RELEASE_FLAGS) -fPIC -shared -o libfbg.so
	$(CC) $(SRC1) $(INCS) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS1) -o $(OUT1)
	$(CC) $(SRC2) $(INCS) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS1) -o $(OUT2)
	$(CC) $(SRC3) $(INCS) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS2) $(DEFP) -o $(OUT3)
	$(CC) $(SRC4) $(INCS) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS2) libluajit.a -ldl $(DEFP) -o $(OUT4)

debug:
	$(CC) $(SRC1) $(INCS) $(STANDARD_FLAGS) $(DEBUG_FLAGS) $(LIBS1) -o $(OUT1)
	$(CC) $(SRC2) $(INCS) $(STANDARD_FLAGS) $(DEBUG_FLAGS) $(LIBS1) -o $(OUT2)
	$(CC) $(SRC3) $(INCS) $(STANDARD_FLAGS) $(DEBUG_FLAGS) $(LIBS2) $(DEFP) -o $(OUT3)
	$(CC) $(SRC4) $(INCS) $(STANDARD_FLAGS) $(DEBUG_FLAGS) $(LIBS2) libluajit.a -ldl $(DEFP) -o $(OUT4)

lfds711:
	$(CC) $(SRC1) $(INCS2) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS1) -DFBG_LFDS -DLFDS711 -o $(OUT1)
	$(CC) $(SRC2) $(INCS2) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS1) -DFBG_LFDS -DLFDS711 -o $(OUT2)
	$(CC) $(SRC3) $(INCS2) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS3) -DFBG_LFDS -DLFDS711 $(DEFP) -o $(OUT3)
	$(CC) $(SRC4) $(INCS2) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS3) libluajit.a -ldl -DFBG_LFDS -DLFDS711 $(DEFP) -o $(OUT4)

fbdeves2:
	$(CC) $(SRC_LIBS) opengl_es2/fbg_opengl_es2.c opengl_es2_example.c -I ../src/ $(STANDARD_FLAGS) -lEGL -lm -lpthread -o opengl_es2_example
	$(CC) $(SRC_LIBS) opengl_es2/fbg_opengl_es2.c opengl_es2_parallel.c -I ../src/ $(STANDARD_FLAGS) -I. -Iliblfds720 -DFBG_LFDS $(DEFP) ~/liblfds7.2.0/src/liblfds720/bin/liblfds720.a -lEGL -lm -lpthread -o opengl_es2_parallel

rpies2:
	$(CC) $(SRC_LIBS) opengl_es2/fbg_opengl_es2.c opengl_es2_example.c -I ../src/ $(STANDARD_FLAGS) -DFBG_RPI -I/opt/vc/include/ -L/opt/vc/lib/ -lbrcmGLESv2 -lbrcmEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm -o opengl_es2_example
	$(CC) $(SRC_LIBS) opengl_es2/fbg_opengl_es2.c opengl_es2_parallel.c -I ../src/ $(STANDARD_FLAGS) -DFBG_RPI -I/opt/vc/include/ -I. $(DEFP) -L/opt/vc/lib/ -lbrcmGLESv2 -lbrcmEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm -o opengl_es2_parallel

dispman:
	$(CC) $(SRC_LIBS) dispmanx/fbg_dispmanx.c dispmanx_example.c -I ../src/ $(STANDARD_FLAGS) -I/opt/vc/include/ -L/opt/vc/lib/ -lbrcmGLESv2 -lbrcmEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm -o dispmanx_example
	$(CC) $(SRC_LIBS) dispmanx/fbg_dispmanx.c dispmanx_pure_parallel.c -I ../src/ $(STANDARD_FLAGS) -DFBG_RGBA -o2 -I/opt/vc/include/ -L/opt/vc/lib/ -lbrcmGLESv2 -lbrcmEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm -o dispmanx_pure_parallel
	$(CC) $(SRC_LIBS) dispmanx/fbg_dispmanx.c dispmanx_parallel.c -I ../src/ $(STANDARD_FLAGS) $(DEFP) -DFBG_RGBA -DFBG_LFDS -I. -Iliblfds720 -I/opt/vc/include/ -L/opt/vc/lib/ ~/liblfds7.2.0/src/liblfds720/bin/liblfds720.a -lbrcmGLESv2 -lbrcmEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm -o dispmanx_parallel


clean:
	rm -f *.o $(OUT1) $(OUT2) $(OUT3) $(OUT4)

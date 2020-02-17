/**
  * Advanced example which use Lua scripting language to provide a sort of multithreaded Processing-like environment
  * to build FB Graphics graphical sketches from a Lua script without the need to compile any C code.
  *
  * This use 8 cores, all threads (fbg fragments) have their own Lua state and call the Lua draw() function at each frames
  *
  * See 'sketch.lua'
  *
  * This could be extended to be a complete Processing-like environment easily.
  *
  * Note : It only offer clear / rect and image functions inside Lua code and the compositing function is useless (not yet complete)
  */

#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "glfw/fbg_glfw.h"

#include <lualib.h>
#include <lauxlib.h>
#include "luajit.h"

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

// fbg specific LUA code that we add automatically to any loaded LUA scripts
// this actually expose a sample of the C fbg library to the LUA script with short function name matching the Processing language
const char *fbg_lua_header = "local ffi = require(\"ffi\")\n"
                             "local fbg = ffi.load(\"libfbg.so\")\n"
                             "ffi.cdef[[\n"
                             "void fbg_background(struct _fbg *fbg, unsigned char r, unsigned char g, unsigned char b);\n"
                             "void fbg_frect(struct _fbg *fbg, int x, int y, int w, int h);\n"
                             "void fbg_fill(struct _fbg *fbg, unsigned char r, unsigned char g, unsigned char b);\n"
                             "struct _fbg_img *fbg_loadImage(struct _fbg *fbg, const char *filename);\n"
                             "void fbg_image(struct _fbg *fbg, struct _fbg_img *img, int x, int y);\n"
                             "]]\n"
                             // short function names definition (Processing-like interface)
                             "function background (r, g, b) fbg.fbg_background(C_fbg, r, g, b) end\n"
                             "function rect (x, y, w, h, r, g, b) fbg.fbg_frect(C_fbg, x, y, w, h) end\n"
                             "function fill (r, g, b) fbg.fbg_fill(C_fbg, r, g, b) end\n"
                             "function loadImage (filename) return fbg.fbg_loadImage(C_fbg, filename) end\n"
                             "function image (img, x, y) fbg.fbg_image(C_fbg, img, x, y) end\n"
                             "\n\n";

struct _fragment_user_data {
    lua_State *lua_state;
};

struct _file_data {
    char *buffer;
    long numbytes;
};

// load a file into a buffer with reserved space for a header
struct _file_data *loadFileInBuffer(const char *filename, size_t header_size) {
    FILE *f = fopen(filename, "r");

    if (f == NULL) {
        return NULL;
    }

    fseek(f, 0L, SEEK_END);
    long numbytes = ftell(f);

    fseek(f, 0L, SEEK_SET);

    struct _file_data *fd = (struct _file_data *)malloc(sizeof(struct _file_data));
    if (fd == NULL) {
        fclose(f);

        return NULL;
    }

    fd->numbytes = numbytes;

    fd->buffer = (char*)calloc(numbytes + header_size, sizeof(char));
    if (fd->buffer == NULL) {
        fclose(f);

        free(fd);

        return NULL;
    }

    fread(&fd->buffer[header_size], sizeof(char), numbytes, f);
    fclose(f);

    return fd;
}

// load a 'sketch', a lua script that will get evaluated per fragments (dedicated graphics thread)
struct _fragment_user_data *loadSketch(const char *sketch_filename) {
    struct _fragment_user_data *user_data = (struct _fragment_user_data *)calloc(1, sizeof(struct _fragment_user_data));

    // we create a new lua state per thread
    user_data->lua_state = luaL_newstate();

    luaL_openlibs(user_data->lua_state);

    size_t header_size = strlen(fbg_lua_header);

    // load file in memory
    struct _file_data *sketch_content = loadFileInBuffer(sketch_filename, header_size);
    if (sketch_content == NULL) {
        printf("Couldn't load file: %s\n", lua_tostring(user_data->lua_state, -1));
        fflush(stdout);

        lua_close(user_data->lua_state);

        free(user_data);

        return NULL;
    }

    // add our fbg specific Lua code
    memcpy(sketch_content->buffer, fbg_lua_header, header_size);

    // pass buffer to Lua
    int status = luaL_loadbuffer(user_data->lua_state, sketch_content->buffer, sketch_content->numbytes + header_size, "sketch_filename");
    if (status) {
        printf("luaL_loadbuffer failed: %s\n", lua_tostring(user_data->lua_state, -1));
        fflush(stdout);

        free(sketch_content->buffer);
        free(sketch_content);

        lua_close(user_data->lua_state);

        free(user_data);

        return NULL;
    }

    // file data no more needed
    free(sketch_content->buffer);
    free(sketch_content);

    // eval script
    lua_pcall(user_data->lua_state, 0, 0, 0);

    return user_data;
}

void freeSketch(struct _fragment_user_data *user_data) {
    lua_close(user_data->lua_state);

    free(user_data);
}

void *fragmentStart(struct _fbg *fbg) {
    struct _fragment_user_data *sketch = loadSketch("sketch.lua");

    if (sketch == NULL) {
        return NULL;
    }

    // register global variables (for fbg context stuff that we will need for our drawing operations)
    lua_pushlightuserdata(sketch->lua_state, fbg);
    lua_setglobal(sketch->lua_state, "C_fbg");

    lua_pushnumber(sketch->lua_state, fbg->task_id);
    lua_setglobal(sketch->lua_state, "C_frag_id");

    lua_pushnumber(sketch->lua_state, fbg->parallel_tasks);
    lua_setglobal(sketch->lua_state, "C_frag_len");

    lua_pushnumber(sketch->lua_state, fbg->size);
    lua_setglobal(sketch->lua_state, "C_size");

    lua_pushnumber(sketch->lua_state, fbg->width);
    lua_setglobal(sketch->lua_state, "C_width");

    lua_pushnumber(sketch->lua_state, fbg->height);
    lua_setglobal(sketch->lua_state, "C_height");

    return sketch;
}

void fragment(struct _fbg *fbg, struct _fragment_user_data *user_data) {
    if (!user_data) {
        return;
    }

    // evaluate sketch draw function
    lua_getglobal(user_data->lua_state, "draw");
    lua_pcall(user_data->lua_state, 0, 0, 0);
}

void fragmentStop(struct _fbg *fbg, struct _fragment_user_data *data) {
    freeSketch(data);
}

struct _fragment_user_data *main_sketch = NULL;

void fbg_compositing(struct _fbg *fbg, unsigned char *buffer, int task_id) {
    // this doesn't work yet (the idea is to allow compositing function to be defined from the Lua script)
    lua_getglobal(main_sketch->lua_state, "compositing");
    lua_pcall(main_sketch->lua_state, 0, 0, 0);

    for (int j = 0; j < fbg->size; j += 1) {
        fbg->back_buffer[j] = fbg->back_buffer[j] + buffer[j];
    }
}

int program() {
    struct _fbg *fbg = fbg_glfwSetup(800, 600, "glfw example", 0, 0);
    if (fbg == NULL) {
        return 0;
    }

    struct _fbg_img *bb_font_img = fbg_loadImage(fbg, "../examples/bbmode1_8x8.png");

    struct _fbg_font *bbfont = fbg_createFont(fbg, bb_font_img, 8, 8, 33);

    fbg_createFragment(fbg, fragmentStart, fragment, fragmentStop, 7);

    srand(time(NULL));

    signal(SIGINT, int_handler);

    main_sketch = fragmentStart(fbg);

    do {
        fbg_glfwClear();

        // evaluate sketch draw function
        lua_getglobal(main_sketch->lua_state, "draw");
        lua_pcall(main_sketch->lua_state, 0, 0, 0);

        //fbg_clear(fbg, 0);

        for (int j = 0; j < fbg->parallel_tasks; j += 1) {
            fbg_write(fbg, fbg->fps_char, 2, 2 + j * 10);
        }

        fbg_draw(fbg, fbg_compositing);
        fbg_flip(fbg);
    } while (keep_running && !fbg_glfwShouldClose(fbg));

    fragmentStop(fbg, main_sketch);

    fbg_freeImage(bb_font_img);
    fbg_freeFont(bbfont);

    fbg_close(fbg);

    return 0;
}

int main(int argc, char* argv[]) {
    return program();
}
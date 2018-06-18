FBGraphics : Lightweight C graphics library for the Linux framebuffer with parallelism support
=====

FBGraphics (FBG) is a simple C 24 bpp graphics library for the Linux framebuffer with parallelism support.

Features :
 * Optional : Full parallelism, execute graphics code on multiple CPU cores **with a single function**
 * Displaying and loading PNG images (provided by [LodePNG](https://lodev.org/lodepng/))
 * Bitmap fonts for drawing texts
 * Bare-metal graphics primitive (pixels, rectangles)
 * Double buffering
 * 24 bpp only (may change in the future!)
 * Easy to do fading and screen-clearing related effects (motion blur etc.)
 * Framerate tracking & display for all cores
 * Lightweight enough to be hackable; adapt to all kinds of needs (and still support parallelism easily)

The library is relatively generic,  it only manipulate buffers and render them to the framebuffer device so it should be easy to extract some parts to adapt it to other means.

Table of Contents
=================

* FBGraphics
   * [About](#about)
      * [Quickstart](#quickstart)
      * [Parallelism](#parallelism)
      * [Technical implementation](#technical-implementation)
   * [Benchmark](#benchmark)
   * [Documentation](#documentation)
   * [Building](#building)
   * [Screenshots](#screenshots)
   * [License](#license)

## About

FBGraphics was made to produce fullscreen pixels effects easily with non-accelerated framebuffer by leveraging multi-core processors, it is a bit like a software GPU (much less complex and featured!), the initial target platform is a Raspberry PI 3B and extend to the NanoPI (and many others embedded devices), the library should just work with many others devices with a Linux framebuffer altough there is at the moment some restrictions on the supported framebuffer format (24 bits).

FBGraphics is lightweight and does not intend to be a fully featured graphics library, it provide a limited set of graphics primitive and a small set of useful functions to start doing framebuffer graphics right away with or without multi-core support.

If you want to use the parallelism features with more advanced graphics primitives, take a look at great libraries such as [libgd](http://libgd.github.io/) or [Adafruit GFX library](https://github.com/adafruit/Adafruit-GFX-Library) which should be easy to integrate.

FBGraphics is fast but should be used with caution, no bounds checking happen on most primitives.

Multi-core support is totally optional and is only enabled when `FBG_PARALLEL` C definition is present.

Note : This library does not let you setup the framebuffer, it expect the framebuffer to be configured prior launch with a command such as :

```
fbset -fb /dev/fb0 -g 512 240 512 240 24 -vsync high
setterm -cursor off > /dev/tty0
```

### Quickstart

The simplest example (without texts and images) :

```c
#include <sys/stat.h>
#include <signal.h>

#include "fbgraphics.h"

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, int_handler);

    struct _fbg *fbg = fbg_setup("/dev/fb0"); // you can also directly use fbg_init(); for "/dev/fb0"

    do {
        fbg_clear(fbg, 0); // can also be replaced by fbg_fill(fbg, 0, 0, 0);

        fbg_draw(fbg);

        fbg_rect(fbg, fbg->width / 2 - 32, fbg->height / 2 - 32, 16, 16, 0, 255, 0);

        fbg_pixel(fbg, fbg->width / 2, fbg->height / 2, 255, 0, 0);

        fbg_flip(fbg);

    } while (keep_running);

    fbg_close(fbg);

    return 0;
}

```

A simple quickstart example with most features (but no parallelism, see below) :

```c
#include <sys/stat.h>
#include <signal.h>

#include "fbgraphics.h"

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, int_handler);

    struct _fbg *fbg = fbg_init();

    struct _fbg_img *texture = fbg_loadPNG(fbg, "texture.png");
    struct _fbg_img *bb_font_img = fbg_loadPNG(fbg, "bbmode1_8x8.png");

    struct _fbg_font *bbfont = fbg_createFont(fbg, bb_font_img, 8, 8, 33);

    do {
        fbg_clear(fbg, 0);

        fbg_draw(fbg);

        fbg_image(fbg, texture, 0, 0, fbg->width, fbg->height);

        fbg_write(fbg, "Quickstart example\nFPS:", 4, 2);
        fbg_write(fbg, fbg->fps_char, 32 + 8, 2 + 8);

        fbg_rect(fbg, fbg->width / 2 - 32, fbg->height / 2 - 32, 16, 16, 0, 255, 0);

        fbg_pixel(fbg, fbg->width / 2, fbg->height / 2, 255, 0, 0);

        fbg_flip(fbg);

    } while (keep_running);

    fbg_freeImage(texture);
    fbg_freeImage(bb_font_img);
    fbg_freeFont(bbfont);

    fbg_close(fbg);

    return 0;
}
```

Note : Functions like `fbg_clear` or `fbg_fpixel` are fast functions, there is slower equivalent (but more parametrable) such as `fbg_background`

### Parallelism

Exploiting multiple cores with FBGraphics is really easy, first you have to prepare 3 functions (of which two are optional if you don't have any allocations to do) of the following definition :

```c
// optional function
void *fragmentStart(struct _fbg *fbg) {
    // typically used to allocate your per-thread data
    // see full_example.c for more informations

    return NULL; // return your user data here
}
```

```c
void fragment(struct _fbg *fbg, struct _fragment_user_data *user_data) {
    // this function will be executed by each threads
    // you are free to call any FBG graphics primitive here
    
    fbg_clear(fbg, 0);
    
    // you are also free to fill each threads back buffer the way you want to
    // fbg->task_id : thread identifier (starting at 1, 0 is reserved for the main thread)
    // each threads will draw an horizontal line, the shade of the blue color will change based on the thread it is drawn from
    int x = 0, y = 0;
    for (y = fbg->task_id; y < fbg->height; y += 4) {
        for (x = 0; x < fbg->width; x += 1) {
            int i = (x + y * fbg->width) * 3;
            fbg->back_buffer[i] = fbg->task_id * 85; // note : BGR format
            fbg->back_buffer[i + 1] = 0;
            fbg->back_buffer[i + 2] = 0;
        }
    }
    
    // simple graphics primitive (4 blue rectangle which will be handled by different threads in parallel)
    fbg_rect(fbg, fbg->task_id * 32, 0, 32, 32, 0, 0, 255);
}
```

```c
// optional function
void fragmentStop(struct _fbg *fbg, struct _fragment_user_data *data) {
    // typically used to free your per-thread data
    // see full_example.c for more informations
}
```

Then you have to create a 'Fragment' which is a FBG multi-core task :

```c
fbg_createFragment(fbg, fragmentStart, fragment, fragmentStop, 3, 7);
```

Where :

* `fbg` is the main library data structure returned by `fbg_init` or `fbg_setup`
* `fragmentStart`is a C function which will be executed when the thread start (can be NULL)
* `fragment`is a C function which will be executed indefinitly for each threads and where all the draw code will happen
* `fragmentStop` is a C function which will be executed when the thread end  (can be NULL)
* `3`is the number of parallel tasks (this will launch 3 threads)
* `7` is the buffer queue length for each threads (the amount of frame buffers stored in the internal ringbuffer queue)

And finally you just have to make a call to your fragment function in your drawing loop and call  `fbg_draw`!

```c
fragment(fbg, NULL);
fbg_draw(fbg, 1, NULL);
```

The second argument to `fbg_draw` tell FBG to synchronize with all the threads, it will wait until all the data are received from all the threads, if disabled, data from some threads may not arrive in time and make it into the second frame.

Note : This example will use 4 threads (including your app one) for drawing things on the screen but calling the fragment function in your drawing loop is totally optional, you could for example make use of threads for intensive drawing tasks and just use the main thread to draw the GUI or the inverse etc. it is up to you!

And that is all you have to do!

Note : By default, the resulting buffer of each tasks are additively mixed into the main back buffer, you can override this behavior by specifying a mixing function as the last argument of `fbg_draw` such as :

```c
// function called for each tasks in the fbg_draw function
void selectiveMixing(struct _fbg *fbg, unsigned char *buffer, int task_id) {
    // fbg is the main fbg structure defined by fbg_setup or fbg_init
    // buffer is the current task buffer
    // task_id is the current task id
    int j = 0;
    for (j = 0; j < fbg->size; j += 1) {
        fbg->back_buffer[j] = (fbg->back_buffer[j] > buffer[j]) ? fbg->back_buffer[j] : buffer[j];
    }
}
```

Then you just have to specify it to the `fbg_draw` function :

```c
fbg_draw(fbg, 1, additiveMixing);
```

By using the mixing function, you can have different layers handled by different cores with different compositing rule.

See `simple_parallel_example.c` and `full_example.c` for more informations.

Note : You can only create one Fragment per fbg instance, another call to `fbg_createFragment` will stop all tasks for the passed fbg context and will create a new set of tasks.

### Technical implementation

FBGraphics threads come with their own fbg context data which is essentialy a copy of the actual fbg context, they make use of C atomic types.

Each threads begin by fetching a pre-allocated buffer from a freelist, then the fragment function is called to fill that buffer, the thread wait (using a pthread barrier) for all others to finish their drawing task and then place the buffer into a ringbuffer data structure which will be fetched upon calling `fbg_draw`, the buffers are then mixed into the main back buffer and put back into the freelist.

FBGraphics parallelism make use of the [liblfds](http://liblfds.org/) library for the Ringbuffer and Freelist data structure.

## Benchmark

A simple unoptimized per pixels screen clearing with 4 cores on a Raspberry PI 3B :  30 FPS @ 1280x768 and 370 FPS @ 320x240

### Full example

Fullscreen per pixels perlin noise with texture mapping and scrolling (unoptimized)

**Device** : Raspberry PI 3B ( Quad Core 1.2GHz )

**Settings** : 320x240

| Cores used to draw graphics | FPS     |
| :-------------------------- | :------ |
| 1                           | 42 FPS  |
| 2                           | 81 FPS  |
| 3                           | 120 FPS |

See screenshots below.

### Tunnel example

Fullscreen texture-mapped and animated tunnel made of 40800 2px rectangles with motion blur (unoptimized)

**Device** : Raspberry PI 3B ( Quad Core 1.2GHz )

**Settings** : 320x240

| Cores used to draw graphics | FPS     |
| :-------------------------- | :------ |
| 1                           | 36 FPS  |
| 2                           | 69 FPS  |
| 3                           | 99 FPS |
| 4                           | 66 FPS |

Note : The framerate drop with 4 cores is due to the main thread being too busy which make all the other threads follow due to the synchronization.

See screenshots below.

## Documentation

All usable functions and structures are documented in the `fbgraphics.h` file

Examples demonstrating all features are also available in the `examples` directory.

Some effects come from [my Open Processing sketches](https://www.openprocessing.org/user/130883#sketches)

## Building

C11 standard should be supported by the C compiler.

All examples make use of the framebuffer device `/dev/fb0` and can be built by typing `make` into the examples directory then run them by typing `./run_quickstart` for example (this handle the framebuffer setup prior launch), you will need to compile liblfds for the parallelism features. (see below)

All examples were tested on a Raspberry PI 3B with framebuffer settings : 512x240 24 bpp

For the default build (no parallelism), FBGraphics come with a header file `fbgraphics.h` and a C file `fbgraphics.c` to be included / compiled with your program, you will also need to compile the `lodepng.c` library, see the examples directory for examples of Makefile.

For parallelism support, `FBG_PARALLEL` need to be defined and you will need the [liblfds](http://liblfds.org/) library :

 * Get latest liblfds 7.1.1 package on the official website
 * uncompress, go into the directory `liblfds711`
 * go into the directory `build/gcc_gnumake`
 * type `make` in a terminal
 * `liblfds711.a` can now be found in the `bin` directory, you need to link against it when compiling (see examples)

To compile parallel examples, just copy `liblfds711.a` / `liblfds711.h` file and `liblfds711` directory into the `examples` directory then type `make`.

## Screenshots

![Full example screenshot with three threads](/screenshot1.png?raw=true "Full example screenshot with three threads")

![Tunnel with four threads](/screenshot2.png?raw=true "Tunnel with four threads")

![Earth with four threads](/screenshot3.png?raw=true "Earth with four threads")

![Flags of the world with four threads](/screenshot4.png?raw=true "Flags of the world with four threads")

## License

BSD, see LICENSE file

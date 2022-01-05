FBGraphics : Lightweight C 2D graphics API agnostic library with parallelism support
=====

FBGraphics (FBG) is a simple C 24, 32 bpp (internal format) graphics library with parallelism and custom rendering backend support (graphics API agnostic).

The library is only two .c files on most use cases, the renderer agnostic library `fbgraphics.c` and one of the rendering backend found in `custom_backend` directory.

The library come with five backend (see `custom_backend` folder) : 
 * a Linux framebuffer rendering backend (with 16 bpp support through 24/32 bpp conversion)
 * OpenGL backend which use the [GLFW](http://www.glfw.org/) library
 * OpenGL ES 2.0 backend for fbdev or Raspberry PI
 * fast dispmanx backend (Video Core IV; Raspberry PI)
 * GBA backend (slow due to 24/32 bpp -> 16 bpp support, mostly done as a proof of concept for portability on low memory hardware)

Features :

 * Easy to write / use custom rendering backend support flexible enough to target low memory hardware!
 * Cross-platform with the GLFW backend (some examples may need to be adapted to the target OS)
 * Linux framebuffer (fbdev) rendering backend support
    * Double buffering (with optional page flipping mechanism)
    * 16, 24 (BGR/RGB), 32 bpp support
 * GBA rendering backend
 * OpenGL rendering backend through GLFW
 * OpenGL ES 2.0 rendering backend for Raspberry PI or through fbdev (tested on Nano PI Fire 3 board)
 * dispmanx rendering backend (Video Core IV; Raspberry PI)
 * Optional : Full parallelism, execute graphics code on multiple CPU cores **with a single function**
 * Image loading (provided by [LodePNG](https://lodev.org/lodepng/), [NanoJPEG](http://keyj.emphy.de/nanojpeg/), and [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h))
 * Bitmap fonts for drawing texts
 * Bare-metal graphics primitive (pixels, rectangles, lines, polygon)
 * Easy to do fading, clipping and screen-clearing related effects (motion blur etc.)
 * Drawing calls can be used to render into a specified target buffer such as fbg_image etc.
 * Framerate tracking & display for all cores
 * Lightweight enough to be hackable; adapt to all kinds of needs (and still support parallelism easily)

The library is generic, most functions (including parallel ones) only manipulate buffers and you can build a custom rendering backend pretty easily with few functions call, see the `custom_backend` folder.

Doxygen documentation : https://grz0zrg.github.io/fbg/

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
   * [Rendering backend](#Rendering-backend)
   * [GLFW backend](#GLFW-backend)
   * [OpenGL ES 2.0 backend](#OpenGL-ES-2-backend)
   * [Screenshots](#screenshots)
   * [License](#license)

## About

FBGraphics was built to produce fullscreen pixels effects easily (think of Processing-like creative coding etc.) with non-accelerated framebuffer by leveraging multi-core processors, it is a bit like a software GPU but much less complex and featured, the initial target platform was a Raspberry PI 3B / NanoPI.

FBGraphics was extended to support any numbers of custom rendering backend; all graphics calls manipulate internal buffers and a simple interface allow to draw the result the way you want to.

FBGraphics can support low memory hardware such as GBA. It should be noted that all internal buffers are manipulated in 24/32 bpp so it has to convert to 16bpp on GBA.

An OpenGL rendering backend which use the [GLFW library](http://www.glfw.org/) was created to demonstrate the custom backend feature, it allow to draw the non-accelerated FB Graphics buffer into an OpenGL context through a texture and thus allow to interwine 3D or 2D graphics produced with standard OpenGL calls with CPU-only graphics produced by FBGraphics draw calls.

An OpenGL ES 2.0 backend is also available with similar features, it target platforms with support for OpenGL ES 2.0 through fbdev (tested on Nano PI Fire 3 SBC) or Raspberry PI dispmanx and similar platforms, it wouldn't be hard to extend this for more OpenGL ES 2.0 platforms...

There is also a dispmanx backend targeting Raspberry PI, it have better performances than the OpenGL ES 2 backend on this platform and is recommended if you don't need 3D stuff.

FBGraphics was built so that it is possible to create any number of rendering context using different backend running at the same time while exploiting multi-core processors... the content of any rendering context can be transfered into other context through images when calling `fbg_drawInto`

FBGraphics framebuffer settings support 16, 24 (BGR/RGB), 32 bpp, 16 bpp mode is handled by converting from 24 bpp to 16 bpp upon drawing, page flipping mechanism is disabled in 16 bpp mode, **24 bpp is the fastest mode**.

FBGraphics is lightweight and does not intend to be a fully featured graphics library, it provide a limited set of graphics primitive and a small set of useful functions to start doing computer graphics anywhere right away with or without multi-core support.

If you want to use the parallelism features with advanced graphics primitives, take a look at great libraries such as [libgd](http://libgd.github.io/), [Adafruit GFX library](https://github.com/adafruit/Adafruit-GFX-Library) or even [ImageMagick](https://imagemagick.org) which should be easy to integrate.

FBGraphics is fast but should be used with caution, display bounds checking is not implemented on most primitives, this allow raw performances at the cost of crashs if not careful.

Multi-core support is optional and is only enabled when `FBG_PARALLEL` C definition is present.

FBGraphics framebuffer backend support a mechanism known as page flipping, it allow fast double buffering by doubling the framebuffer virtual area, it is disabled by default because it is actually slower on some devices. You can enable it with a `fbg_fbdevSetup` call.

VSync is automatically enabled if supported.

**Note** : FBGraphics framebuffer backend does not let you setup the framebuffer, it expect the framebuffer to be configured prior launch with a command such as :

```
fbset -fb /dev/fb0 -g 512 240 512 240 24 -vsync high
setterm -cursor off > /dev/tty0
```

`fbset` should be available in your package manager.

### Framebuffer Quickstart

The simplest example (no parallelism, without texts and images) :

```c
#include <sys/stat.h>
#include <signal.h>

#include "fbg_fbdev.h"
#include "fbgraphics.h"

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, int_handler);

    struct _fbg *fbg = fbg_fbdevSetup("/dev/fb0", 0); // you can also directly use fbg_fbdevInit(); for "/dev/fb0", last argument mean that will not use page flipping mechanism  for double buffering (it is actually slower on some devices!)

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

#include "fbg_fbdev.h"
#include "fbgraphics.h"

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, int_handler);

    struct _fbg *fbg = fbg_fbdevInit();

    struct _fbg_img *texture = fbg_loadImage(fbg, "texture.png");
    struct _fbg_img *bb_font_img = fbg_loadImage(fbg, "bbmode1_8x8.png");

    struct _fbg_font *bbfont = fbg_createFont(fbg, bb_font_img, 8, 8, 33);

    do {
        fbg_clear(fbg, 0);

        fbg_draw(fbg);

        // you can also use fbg_image(fbg, texture, 0, 0)
        // but you must be sure that your image size fit on the display
        fbg_imageClip(fbg, texture, 0, 0, 0, 0, fbg->width, fbg->height);

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

**Note** : Functions like `fbg_clear` or `fbg_fpixel` are fast functions, there is slower equivalent (but more parametrable) such as `fbg_background` or `fbg_pixel`, some functions variant also support transparency such as ``fbg_pixela` or `fbg_recta`.

**Note** : You can generate monospace bitmap fonts to be used with `fbg_createFont` function by using my [monoBitmapFontCreator](https://github.com/grz0zrg/monoBitmapFontCreator) tool available [here](https://fbg-bitmap-font-creator.netlify.com/)

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
fbg_createFragment(fbg, fragmentStart, fragment, fragmentStop, 3);
```

Where :

* `fbg` is the main library data structure returned by `fbg_customSetup` and any backend `fbg_backendnameSetup` calls (see available backends in `custom_backend` directory)
* `fragmentStart`is a C function which will be executed when the thread start (can be NULL)
* `fragment`is a C function which will be executed indefinitly for each threads and where all the draw code will happen
* `fragmentStop` is a C function which will be executed when the thread end  (can be NULL)
* `3`is the number of parallel tasks (this will launch 3 threads)

And finally you just have to make a call to your fragment function in your drawing loop and call  `fbg_draw`!

```c
fragment(fbg, NULL);
fbg_draw(fbg, NULL);
```

`fbg_draw` will wait until all the data are received from all the threads then draw to screen

**Note** : This example will use 4 threads (including your app one) for drawing things on the screen but calling the fragment function in your drawing loop is totally optional, you could for example make use of threads for intensive drawing tasks and just use the main thread to draw the GUI or the inverse etc. it is up to you!

And that is all you have to do!

See `simple_parallel_example.c` and `full_example.c` for more informations.

**Note** : By default, the resulting buffer of each tasks are additively mixed into the main back buffer, you can override this behavior by specifying a mixing function as the last argument of `fbg_draw` such as :

```c
// function called for each tasks in the fbg_draw function
void selectiveMixing(struct _fbg *fbg, unsigned char *buffer, int task_id) {
    // fbg is the main fbg structure returned by fbg_customSetup calls and any backend setup calls
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
fbg_draw(fbg, additiveMixing);
```

By using the mixing function, you can have different layers handled by different cores with different compositing rule, see `compositing.c` for an example of alpha blending compositing 2 layers running on their own cores.

**Note** : You can only create one Fragment per fbg instance, another call to `fbg_createFragment` will stop all tasks for the passed fbg context and will create a new set of tasks.

**Note** : On low performances platforms you may encounter performance issues at high resolution and with a high number of fragments, this is because all the threads buffer need to be mixed back onto the main thread before being displayed and at high resolution / threads count that is alot of pixels to process! You can see an alternative implementation using pure pthread in the `custom_backend` folder and `dispmanx_pure_parallel.c` but it doesn't have compositing. If your platform support some sort of SIMD instructions you could also do all the compositing using SIMD which should result in a 5x or more speed increase!

### Technical implementation

FBGraphics threads come with their own fbg context data which is essentialy a copy of the actual fbg context, they make use of C atomic types.

Initially parallelism was implemented using [liblfds](http://liblfds.org/) library for its Ringbuffer and Freelist data structure.

Now parallelism has two implementation, liblfds and a custom synchronization mechanism which has the advantage to not require additional libraries and thus execute on more platforms.

You can still use the liblfds implementation using the `FBG_LFDS` define, it may be faster.

#### With liblfds

Each threads begin by fetching a pre-allocated buffer from a freelist, then the fragment function is called to fill that buffer, the thread then place the buffer into a ringbuffer data structure which will be fetched upon calling `fbg_draw`, the buffers are then mixed into the main back buffer and put back into the freelist.

#### Without liblfds

Each threads fragment function is called to fill the local buffer, each threads then wait till that buffer is consumed by the main thread upon calling `fbg_draw`, the buffers are then mixed into the main back buffer and `fbg_draw` wake up all threads.

## Benchmark (framebuffer)

A simple unoptimized per pixels screen clearing with 4 cores on a Raspberry PI 3B :  30 FPS @ 1280x768 and 370 FPS @ 320x240

Note : Using the dispmanx backend a screen clearing + rectangle moving on a Raspberry PI 3B : 60 FPS @ 1920x1080

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

All usable functions and structures are documented in the `fbgraphics.h` file with [Doxygen](http://www.stack.nl/~dimitri/doxygen/)

The HTML documentation can be found in the `docs` directory, it is also hosted on GitHub [here](https://grz0zrg.github.io/fbg/)

Examples demonstrating all features are available in the `examples` directory.

Some effects come from [my Open Processing sketches](https://www.openprocessing.org/user/130883#sketches)

## Building

C11 standard should be supported by the C compiler.

All examples found in `examples` directory make use of the framebuffer device `/dev/fb0` and can be built by typing `make` into the examples directory then run them by typing `./run_quickstart` for example (this handle the framebuffer setup prior launch), you will need to compile liblfds for the parallelism features. (see below)

All examples were tested on a Raspberry PI 3B with framebuffer settings : 320x240 24 bpp

For the default build (no parallelism), FBGraphics come with a header file `fbgraphics.h` and a C file `fbgraphics.c` to be included / compiled / linked with your program plus one of the rendering backend found in `custom_backend` directory, you will also need to compile the `lodepng.c` library and `nanojpeg.c` library, see the examples directory for examples of Makefile.

For parallelism support, `FBG_PARALLEL` need to be defined.

If you need to use the slightly different parallelism implementation (see technical implementation section) you will need the [liblfds](http://liblfds.org/) library :

 * Get latest liblfds 7.1.1 package on the official website
 * uncompress, go into the directory `liblfds711`
 * go into the directory `build/gcc_gnumake`
 * type `make` in a terminal
 * `liblfds711.a` can now be found in the `bin` directory, you need to link against it when compiling (see examples)

To compile liblfds parallel examples, just copy `liblfds711.a` / `liblfds711.h` file and `liblfds711` directory into the `examples` directory then type `make lfds711`.

**Note** : FBGraphics with liblfds work on ARM64 platforms but you will need liblfds720 which is not yet released.

### Executable size optimization

This library may be used for size optimized executable for things like [demos](https://en.wikipedia.org/wiki/Demoscene)

Image support can be disabled with the following defines:
- `WITHOUT_JPEG`
- `WITHOUT_PNG`
- `WITHOUT_STB_IMAGE`

See `tiny` makefile rule inside the `custom_backend` or `examples` folder for some compiler optimizations related to executable size.

Under Linux [sstrip](https://github.com/BR903/ELFkickers/tree/master/sstrip) and [UPX](https://upx.github.io/) can be used to bring the size down even futher.

## Rendering backend

See `README` into `custom_backend` folder

## GLFW backend

See `README` into `custom_backend` folder

The GLFW backend was made to demonstrate how to write a backend but it is complete enough to be used by default.

The GLFW backend has a cool lightweight Lua example which setup a Processing-like environment making use of the parallelism feature of the library, allowing the user to prototype multithreaded graphical stuff without C code compilation through the Lua language.

## OpenGL ES 2 backend

See `README` into `custom_backend` folder

## GBA backend

See `README` into `custom_backend` folder

## Screenshots

![Full example screenshot with three threads](/screenshot1.png?raw=true "Full example screenshot with three threads")

![Tunnel with four threads](/screenshot2.png?raw=true "Tunnel with four threads")

![Earth with four threads](/screenshot3.png?raw=true "Earth with four threads")

![Flags of the world with four threads](/screenshot4.png?raw=true "Flags of the world with four threads")

![Compositing with three threads](/screenshot5.png?raw=true "Compositing with three threads")

## License

BSD, see LICENSE file

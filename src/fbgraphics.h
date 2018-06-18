/*
    Copyright (c) 2018, Julien Verneuil
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the organization nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL Julien Verneuil BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef FB_GRAPHICS_H
#define FB_GRAPHICS_H

    #include <linux/fb.h>
    #include <time.h>
    #include <sys/time.h>
    #include <stdint.h>

#ifdef FBG_PARALLEL
    #include <stdatomic.h>
    #include <pthread.h>

    #include "liblfds711.h"
#endif

// ### Library structures
    struct _fbg_rgb {
        unsigned char r;
        unsigned char g;
        unsigned char b;
    };

    struct _fbg_img {
        unsigned char *data;

        unsigned int width;
        unsigned int height;
    };

    struct _fbg_font {
        int *glyph_coord_x;
        int *glyph_coord_y;

        int glyph_width;
        int glyph_height;

        int first_char;

        struct _fbg_img *bitmap;
    };

    struct _fbg {
        // file descriptor (framebuffer device)
        int fd;

        // framebuffer informations
        struct fb_var_screeninfo vinfo;
        struct fb_fix_screeninfo finfo;

        // framebuffer real size (take into account BPP)
        int size;

        // framebuffer
        unsigned char *buffer;
        // displayed buffer
        unsigned char *disp_buffer;
        // back buffer
        unsigned char *back_buffer;

        // fill color state
        struct _fbg_rgb fill_color;

        // text color state
        struct _fbg_rgb text_color;

        // current font
        struct _fbg_font current_font;

        // display resolution
        int width;
        int height;
        int width_n_height;

        // fps
#ifdef FBG_PARALLEL
        atomic_uint_fast16_t fps;
#else
        int16_t fps;
#endif

        char fps_char[10];

        // fps computation stuff
        struct timeval fps_start;
        struct timeval fps_stop;

        // frame counter
        int frame;

        int bgr;

#ifdef FBG_PARALLEL
        // the number of fragment tasks
        unsigned int parallel_tasks;

        pthread_t *tasks;

        struct _fbg_fragment **fragments;

        // the current task id
        int task_id;

        pthread_barrier_t *sync_barrier;

        atomic_int state;
#endif
    };

#ifdef FBG_PARALLEL
    struct _fbg_freelist_data {
        struct lfds711_freelist_element freelist_element;

        unsigned char *buffer;
    };

    struct _fbg_fragment {
        atomic_int *state;

        struct _fbg *fbg;

        struct lfds711_ringbuffer_element *ringbuffer_element;
        struct lfds711_ringbuffer_state ringbuffer_state;

        struct lfds711_freelist_state freelist_state;
        struct _fbg_freelist_data *fbg_freelist_data;

        struct _fbg_freelist_data *tmp_fbg_freelist_data;

        // user-defined fragment function
        void *(*user_fragment_start)(struct _fbg *fbg);
        void (*user_fragment)(struct _fbg *fbg, void *user_data);
        void (*user_fragment_stop)(struct _fbg *fbg, void *user_data);

        void *user_data;

        unsigned int queue_size;
    };
#endif

    // ### Library functions

    // initialize the library
    //  fb_device : framebuffer device (example : /dev/fb0)
    // return _fbg structure pointer to pass to any FBG library functions
    extern struct _fbg *fbg_setup(char *fb_device);

    // free up the library and close all devices
    //  fbg : pointer returned by fbg_setup
    extern void fbg_close(struct _fbg *fbg);

    // background fade to black with controllable factor
    //  fbg : pointer returned by fbg_setup
    //  rgb_fade_amount : the RGB fade amount
    extern void fbg_fade_down(struct _fbg *fbg, unsigned char rgb_fade_amount);

    // background fade to white with controllable factor
    //  fbg : pointer returned by fbg_setup
    //  rgb_fade_amount : the RGB fade amount
    extern void fbg_fade_up(struct _fbg *fbg, unsigned char rgb_fade_amount);

    // fast grayscale background clearing
    //  fbg : pointer returned by fbg_setup
    //  brightness : pixel brightness
    extern void fbg_clear(struct _fbg *fbg, unsigned char brightness);

    // set the filling color for drawing operations
    //  fbg : pointer returned by fbg_setup
    //  r, g, b : fill color
    extern void fbg_fill(struct _fbg *fbg, unsigned char r, unsigned char g, unsigned char b);

    // get the RGB value of a pixel
    //  fbg : pointer returned by fbg_setup
    //  x, y : pixel position (upper left coordinate)
    //  color : _fbg_rgb structure which will contain the pixel colors
    extern void fbg_getPixel(struct _fbg *fbg, int x, int y, struct _fbg_rgb *color);

    // standard pixel drawing
    //  fbg : pointer returned by fbg_setup
    //  x, y : pixel position (upper left coordinate)
    //  r, g, b : pixel color
    extern void fbg_pixel(struct _fbg *fbg, int x, int y, unsigned char r, unsigned char g, unsigned char b);

    // fast pixel drawing which use the fill color set by fbg_fill()
    //  fbg : pointer returned by fbg_setup
    //  x, y : pixel position (upper left coordinate)
    extern void fbg_fpixel(struct _fbg *fbg, int x, int y);

    // draw a rectangle
    //  fbg : pointer returned by fbg_setup
    //  x, y : rectangle position (upper left coordinate)
    //  r, g, b : rectangle color
    extern void fbg_rect(struct _fbg *fbg, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b);

    // fill the background with a color
    //  fbg : pointer returned by fbg_setup
    //  r, g, b : background color
    extern void fbg_background(struct _fbg *fbg, unsigned char r, unsigned char g, unsigned char b);

#ifdef FBG_PARALLEL
    // draw to screen
    //  fbg : pointer returned by fbg_setup
    //  sync_with_task : 1 = wait for all fragment tasks to be finished before rendering
    extern void fbg_draw(struct _fbg *fbg, int sync_with_task);
#else
    // draw to screen
    //  fbg : pointer returned by fbg_setup
    extern void fbg_draw(struct _fbg *fbg);
#endif

    // flip buffers
    //  fbg : pointer returned by fbg_setup
    extern void fbg_flip(struct _fbg *fbg);

    // create empty image
    //  width : image width
    //  height : image height
    // return _fbg_img structure
    extern struct _fbg_img *fbg_createImage(unsigned int width, unsigned int height);

    // load a PNG image
    //  filename : PNG image filename
    // return _fbg_img structure
    extern struct _fbg_img *fbg_loadPNG(struct _fbg *fbg, const char *filename);

    // draw an image
    //  fbg : pointer returned by fbg_setup
    //  img : image structure pointer
    //  x, y : draw position (upper left coordinate)
    //  w, h : width / height of the image (cannot be higher than the image width / height)
    extern void fbg_image(struct _fbg *fbg, struct _fbg_img *img, int x, int y, int w, int h);

    // flip an image vertically
    //  fbg : pointer returned by fbg_setup
    extern void fbg_imageFlip(struct _fbg_img *img);

    // free image
    //  img : image structure pointer
    extern void fbg_freeImage(struct _fbg_img *img);

    // create a bitmap font from an image file
    //  fbg : pointer returned by fbg_setup
    //  img : image structure pointer
    //  glyph_width : single glyph width
    //  glyph_height : single glyph height
    //  first_char : the first character of the bitmap font
    // return _fbg_font structure
    extern struct _fbg_font *fbg_createFont(struct _fbg *fbg, struct _fbg_img *img, int glyph_width, int glyph_height, unsigned char first_char);

    // set the current font
    //  fnt : _fbg_font structure
    extern void fbg_textFont(struct _fbg *fbg, struct _fbg_font *font);

    // set the current text color
    //  fnt : _fbg_font structure
    //  r, g, b : current text color
    extern void fbg_textColor(struct _fbg *fbg, unsigned char r, unsigned char g, unsigned char b);

    // draw a text
    //  fbg : pointer returned by fbg_setup
    //  font : font structure pointer
    //  text : the text to draw ('\n' and ' ' are treated automatically)
    //  x, y : text position (upper left coordinate)
    //  r, g, b : text color
    extern void fbg_text(struct _fbg *fbg, struct _fbg_font *fnt, char *text, int x, int y, int r, int g, int b);

    // free font
    //  font : font structure pointer
    extern void fbg_freeFont(struct _fbg_font *font);

    // draw the framerate of a particular task
    //  fbg : pointer returned by fbg_setup
    //  font : font structure pointer
    //  task : the task id (0 = main thread)
    //  x, y : text position (upper left coordinate)
    //  r, g, b : text color
    extern void fbg_drawFramerate(struct _fbg *fbg, struct _fbg_font *fnt, int task, int x, int y, int r, int g, int b);

    // get the framerate of a particular task
    //  fbg : pointer returned by fbg_setup
    //  task : the task id (0 = main thread)
    // return task framerate
    extern int fbg_getFramerate(struct _fbg *fbg, int task);

#ifdef FBG_PARALLEL
    // create a fbg parallel task (also called 'fragments')
    //  fbg : pointer returned by fbg_setup
    //  fragment : a function taking a _fbg_fragment structure as argument
    //  parallel_tasks : the number of parallel tasks to register
    extern void fbg_createFragment(struct _fbg *fbg, void *(*fragment_start)(struct _fbg *fbg), void (*fragment)(struct _fbg *fbg, void *user_data), void (*fragment_stop)(struct _fbg *fbg, void *user_data), unsigned int parallel_tasks, unsigned int queue_size);
#endif

    // ### Helper functions

    // initialize the library with default framebuffer device (/dev/fb0) and no parallel tasks
    #define fbg_init() fbg_setup(NULL)

    // draw a rectangle which use the fill color set by fbg_fill()
    //  fbg : pointer returned by fbg_setup
    //  x, y : rectangle position (upper left coordinate)
    #define fbg_frect(fbg, x, y, w, h) fbg_rect(fbg, x, y, w, h, fbg->fill_color.r, fbg->fill_color.g, fbg->fill_color.b)

    // fade to black
    //  fbg : pointer returned by fbg_setup
    //  fade_amount : RGB fade amount
    #define fbg_fade(fbg, fade_amount) fbg_fade_down(fbg, fade_amount)

    // write text to screen by using the current font and current color
    //  fbg : pointer returned by fbg_setup
    //  text : the text to draw ('\n' and ' ' are treated automatically)
    //  x, y : text location
    #define fbg_write(fbg, text, x, y) fbg_text(fbg, &fbg->current_font, text, x, y, fbg->text_color.r, fbg->text_color.g, fbg->text_color.b)

    #define _FBG_MAX(a,b) ((a) > (b) ? a : b)
    #define _FBG_MIN(a,b) ((a) < (b) ? a : b)

#endif
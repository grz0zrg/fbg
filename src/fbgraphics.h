/*
    Copyright (c) 2018, 2019, Julien Verneuil
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
    #include <math.h>

#ifdef FBG_PARALLEL
    #include <stdatomic.h>
    #include <pthread.h>

// compatibility layer for liblfds 711 version (because 720 was unreleased at the time of writing this)
#ifdef FBG_LFDS
#ifdef LFDS711
    #include "liblfds711.h"

    #define lfds720_freelist_n_element lfds711_freelist_element
    #define lfds720_ringbuffer_n_element lfds711_ringbuffer_element
    #define lfds720_ringbuffer_n_state lfds711_ringbuffer_state
    #define lfds720_freelist_n_state lfds711_freelist_state
    #define LFDS720_PAL_ATOMIC_ISOLATION_LENGTH_IN_BYTES LFDS711_PAL_ATOMIC_ISOLATION_IN_BYTES
    #define lfds720_ringbuffer_n_init_valid_on_current_logical_core lfds711_ringbuffer_init_valid_on_current_logical_core
    #define lfds720_freelist_n_init_valid_on_current_logical_core lfds711_freelist_init_valid_on_current_logical_core
    #define LFDS720_FREELIST_N_SET_VALUE_IN_ELEMENT LFDS711_FREELIST_SET_VALUE_IN_ELEMENT
    #define LFDS720_FREELIST_N_GET_VALUE_FROM_ELEMENT LFDS711_FREELIST_GET_VALUE_FROM_ELEMENT
    #define lfds720_freelist_n_threadsafe_push lfds711_freelist_push
    #define lfds720_freelist_n_threadsafe_pop lfds711_freelist_pop
    #define lfds720_ringbuffer_n_cleanup lfds711_ringbuffer_cleanup
    #define lfds720_freelist_n_cleanup lfds711_freelist_cleanup
    #define lfds720_ringbuffer_n_read lfds711_ringbuffer_read
    #define lfds720_ringbuffer_n_write lfds711_ringbuffer_write
    #define lfds720_misc_flag lfds711_misc_flag
    #define LFDS720_MISC_FLAG_RAISED LFDS711_MISC_FLAG_RAISED
    #define LFDS720_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_PHYSICAL_CORE LFDS711_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE
    #define lfds720_pal_uint_t lfds711_pal_uint_t
#else
    #include "liblfds720.h"
#endif
#endif
#endif

// ### Library structures

    //! RGBA color data structure
    /*! Hold RGBA components [0,255]*/
    struct _fbg_rgb {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    };

    //! HSL color data structure
    /*! Hold HSL components S/L [0,1], HUE [0, 360]*/
    struct _fbg_hsl {
        int h;
        float s;
        float l;
    };

    //! Image data structure
    /*! Hold images informations and data */
    struct _fbg_img {
        //! RGB image data (bpp depend on framebuffer settings)
        unsigned char *data;

        //! Image width in pixels
        unsigned int width;
        //! Image height in pixels
        unsigned int height;
    };

    //! Bitmap font data structure
    /*! Hold bitmap font informations and associated image */
    struct _fbg_font {
        //! Pre-computed X glyphs coordinates
        int *glyph_coord_x;
        //! Pre-computed Y glyphs coordinates
        int *glyph_coord_y;

        //! Width of a glyph
        int glyph_width;
        //! Height of a glyph
        int glyph_height;

        //! First ASCII character of the bitmap font file
        unsigned char first_char;

        //! Associated font image data structure
        struct _fbg_img *bitmap;
    };

    //! FB Graphics context data structure
    /*! Hold all data related to a FBG context */
    struct _fbg {
        //! Framebuffer device file descriptor
        int fd;

        //! Framebuffer device var. informations
        struct fb_var_screeninfo vinfo;
        //! Framebuffer device fix. informations
        struct fb_fix_screeninfo finfo;

        //! Framebuffer real data length (with BPP)
        int size;

        //! Memory-mapped framebuffer
        unsigned char *buffer;
        //! Front / display buffer
        unsigned char *disp_buffer;
        //! Back buffer
        /*! All FB Graphics functions draw into this buffer. */
        unsigned char *back_buffer;
        //! Temporary buffer
        unsigned char *temp_buffer;

        //! Current fill color
        /*! Default to black. */
        struct _fbg_rgb fill_color;

        //! Current text color
        /*! Default to white. */
        struct _fbg_rgb text_color;

        //! Current text background color (based on colorkey value)
        /*! Default to black. */
        struct _fbg_rgb text_background;

        //! Current text color key
        /*! Default to black. */
        unsigned char text_colorkey;

        //! Text background alpha value
        /*! Default to transparent. */
        int text_alpha;

        //! Current font
        /*! No fonts is loaded by default and the first loaded font will be assigned automatically as the current font. */
        struct _fbg_font current_font;

        //! Display width in pixels
        int width;
        //! Display height in pixels
        int height;
        //! Display lenght in pixels (width * height)
        int width_n_height;
        //! Display components amount (3 = 24 BPP / 4 = 32 BPP)
        int components;
        //! Offset to add in case of 32 BPP
        int comp_offset;
        //! Internal buffers line length
        int line_length;

        //! Requested new display width (resize event)
        int new_width;
        //! Requested new display height (resize event)
        int new_height;

        //! Current FPS
#ifdef FBG_PARALLEL
        atomic_uint_fast16_t fps;
#else
        int16_t fps;
#endif

        //! Current FPS as a string
        char fps_char[10];

        //! First frame time for the current second
        struct timeval fps_start;
        //! Last frame time for the current second
        struct timeval fps_stop;

        //! Frame counter for the current second
        int frame;

        //! Flag indicating a BGR framebuffer
        int bgr;
        //! Flag indicating that page flipping is enabled
        int page_flipping;

        //! Backend resize function
        void (*backend_resize)(struct _fbg *fbg, unsigned int new_width, unsigned int new_height);
        //! User-defined resize function
        void (*user_resize)(struct _fbg *fbg, unsigned int new_width, unsigned int new_height);
        //! User-defined flip function
        void (*user_flip)(struct _fbg *fbg);
        //! User-defined draw function
        void (*user_draw)(struct _fbg *fbg);
        //! User-defined free function
        void (*user_free)(struct _fbg *fbg);
        //! User-defined context structure
        void *user_context;

        //! currently processed task buffer (assigned before compositing function is called in fbg_draw)
        //unsigned char *curr_task_buffer;

#ifdef FBG_PARALLEL
        //! Total number of actual parallel tasks
        unsigned int parallel_tasks;

        //! pthread array of tasks
        pthread_t *tasks;

        //! Array of tasks data structure
        struct _fbg_fragment **fragments;

        //! FBG synchronization barrier
        pthread_barrier_t *sync_barrier;

        //! Task id associated to that FBG context
        int task_id;

        //! FBG context running state
        atomic_int state;

        //! Ringbuffer queue length (1 by default, best settings with sync. since we just wait till all threads finish)
        //! Note : This settings may have an impact on feedback effects (since it will pick sequentially buffers from the list, thus allowing a number of "past" rendered frame which may not be what you want with regular feedback effects but also may be what you want for other effects like unlimited blobs...)
        unsigned int fragment_queue_size;
#endif
    };

#ifdef FBG_PARALLEL
#ifdef FBG_LFDS
    //! Freelist data structure
    /*! Hold pre-allocated data associated with a task */
    struct _fbg_freelist_data {
        struct lfds720_freelist_n_element freelist_element;

        unsigned char *buffer;
    };
  #endif

    //! Task (fragment) data structure
    /*! Hold a task data */
    struct _fbg_fragment {
        //! Fragment running state
        atomic_int state;

        //! Task own FBG context
        struct _fbg *fbg;

#ifdef FBG_LFDS
        //! Ringbuffer element
        struct lfds720_ringbuffer_n_element *ringbuffer_element;
        //! Ringbuffer state
        struct lfds720_ringbuffer_n_state *ringbuffer_state;

        //! Freelist state
        struct lfds720_freelist_n_state *freelist_state;

        //! Pre-allocated tasks data
        struct _fbg_freelist_data *fbg_freelist_data;

        //! Temporary task data
        struct _fbg_freelist_data *tmp_fbg_freelist_data; 
#endif

        //! thread <> main thread synchronization
        atomic_int sync_wait;

        //! User-defined task start function
        void *(*user_fragment_start)(struct _fbg *fbg);
        //! User-defined task function
        void (*user_fragment)(struct _fbg *fbg, void *user_data);
        //! User-defined task end function
        void (*user_fragment_stop)(struct _fbg *fbg, void *user_data);

        //! User-defined data
        void *user_data;
    };
#endif

// ### Library functions

    //! initialize a FB Graphics context (framebuffer)
    /*!
      \param fb_device framebuffer device (example : /dev/fb0)
      \param page_flipping wether to use page flipping mechanism for double buffering (slow on some devices)
      \return _fbg structure pointer to pass to any FBG library functions
      \sa fbg_close(), fbg_init(), fbg_customSetup()
    */
    extern struct _fbg *fbg_setup(char *fb_device, int page_flipping);

    //! initialize a FB Graphics context with a custom rendering backend
    /*!
      \param width render width
      \param height render height
      \param user_context user rendering data storage (things like window context etc.)
      \param user_draw function to call upon fbg_draw()
      \param user_flip function to call upon fbg_flip()
      \param backend_resize function to call upon fbg_resize()
      \param user_free function to call upon fbg_close()
      \return _fbg structure pointer to pass to any FBG library functions
      \sa fbg_close(), fbg_init(), fbg_setup()
    */
    extern struct _fbg *fbg_customSetup(int width, int height, void *user_context, void (*user_draw)(struct _fbg *fbg), void (*user_flip)(struct _fbg *fbg), void (*backend_resize)(struct _fbg *fbg, unsigned int new_width, unsigned int new_height), void (*user_free)(struct _fbg *fbg));

    //! free up the memory associated with a FB Graphics context and close the framebuffer device
    /*!
      \param fbg pointer to a FBG context / data structure
      \sa fbg_setup(), fbg_init(), fbg_customSetup()
    */
    extern void fbg_close(struct _fbg *fbg);

    //! register a user resize callback
    /*!
      \param fbg pointer to a FBG context / data structure
      \param user_resize resize function
      \sa fbg_resize(), fbg_pushResize()
    */
    void fbg_setResizeCallback(struct _fbg *fbg, void (*user_resize)(struct _fbg *fbg, unsigned int new_width, unsigned int new_height));

    //! resize the FB Graphics context immediately
    //! note : prefer the usage of fbg_pushResize when integrating the resize event of a custom backend (fbg_pushResize is thread safe all the time)
    //! note : resizing is not yet allowed in framebuffer mode
    /*!
      \param fbg pointer to a FBG context / data structure
      \param new_width new render width
      \param new_height new render height
      \sa fbg_pushResize(), fbg_setResizeCallback()
    */
    extern void fbg_resize(struct _fbg *fbg, int new_width, int new_height);

    //! push a resize event for the FB Graphics context
    //! note : the resize event is processed into the fbg_draw function
    //! note : resizing is not yet allowed in framebuffer mode
    //! note : if you want to immediately resize the context, see fbg_resize
    /*!
      \param fbg pointer to a FBG context / data structure
      \param new_width new render width
      \param new_height new render height
      \sa fbg_resize(), fbg_setResizeCallback()
    */
    extern void fbg_pushResize(struct _fbg *fbg, int new_width, int new_height);

    //! background fade to black with controllable factor
    /*!
      \param fbg pointer to a FBG context / data structure
      \param rgb_fade_amount fade amount
      \sa fbg_fade(), fbg_fadeUp()
    */
    extern void fbg_fadeDown(struct _fbg *fbg, unsigned char rgb_fade_amount);

    //! background fade to white with controllable factor
    /*!
      \param fbg pointer to a FBG context / data structure
      \param rgb_fade_amount fade amount
      \sa fbg_fadeDown()
    */
    extern void fbg_fadeUp(struct _fbg *fbg, unsigned char rgb_fade_amount);

    //! fast grayscale background clearing
    /*!
      \param fbg pointer to a FBG context / data structure
      \param brightness pixel brightness (grayscale)
      \sa fbg_background()
    */
    extern void fbg_clear(struct _fbg *fbg, unsigned char brightness);

    //! set the filling color for fast drawing operations
    /*!
      \param fbg pointer to a FBG context / data structure
      \param r
      \param g
      \param b
      \sa fbg_fpixel(), fbg_frect()
    */
    extern void fbg_fill(struct _fbg *fbg, unsigned char r, unsigned char g, unsigned char b);

    //! get the RGB value of a pixel
    /*!
      \param fbg pointer to a FBG context / data structure
      \param x
      \param y
      \param color a pointer to a _fbg_rgb data structure
    */
    extern void fbg_getPixel(struct _fbg *fbg, int x, int y, struct _fbg_rgb *color);

    //! draw a pixel
    /*!
      \param fbg pointer to a FBG context / data structure
      \param x pixel X position (upper left coordinate)
      \param y pixel Y position (upper left coordinate)
      \param r
      \param g
      \param b
      \sa fbg_fpixel(), fbg_pixela()
    */
    extern void fbg_pixel(struct _fbg *fbg, int x, int y, unsigned char r, unsigned char g, unsigned char b);

    //! draw a pixel with alpha component (alpha blending)
    /*!
      \param fbg pointer to a FBG context / data structure
      \param x pixel X position (upper left coordinate)
      \param y pixel Y position (upper left coordinate)
      \param r
      \param g
      \param b
      \param a
      \sa fbg_fpixel(), fbg_pixel()
    */
    extern void fbg_pixela(struct _fbg *fbg, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a);

    //! fast pixel drawing which use the fill color set by fbg_fill()
    /*!
      \param fbg pointer to a FBG context / data structure
      \param x pixel X position (upper left coordinate)
      \param y pixel Y position (upper left coordinate)
      \sa fbg_pixel(), fbg_fill(), fbg_pixela()
    */
    extern void fbg_fpixel(struct _fbg *fbg, int x, int y);

    //! direct pixel access from index value
    /*!
      \param fbg pointer to a FBG context / data structure
      \param index pixel index in the buffer
      \param value color value
      \sa fbg_pixel(), fbg_fill(), fbg_pixela()
    */
    extern void fbg_plot(struct _fbg *fbg, int index, unsigned char value);

    //! draw a rectangle
    /*!
      \param fbg pointer to a FBG context / data structure
      \param x rectangle X position (upper left coordinate)
      \param y rectangle Y position (upper left coordinate)
      \param w rectangle width
      \param h rectangle height
      \param r
      \param g
      \param b
      \sa fbg_frect(), fbg_recta()
    */
    extern void fbg_rect(struct _fbg *fbg, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b);

    //! draw a rectangle with alpha transparency
    /*!
      \param fbg pointer to a FBG context / data structure
      \param x rectangle X position (upper left coordinate)
      \param y rectangle Y position (upper left coordinate)
      \param w rectangle width
      \param h rectangle height
      \param r
      \param g
      \param b
      \param a
      \sa fbg_frect(), fbg_rect()
    */
    extern void fbg_recta(struct _fbg *fbg, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b, unsigned char a);

    //! fast rectangle drawing which use the fill color set by fbg_fill()
    /*!
      \param fbg pointer to a FBG context / data structure
      \param x rectangle X position (upper left coordinate)
      \param y rectangle Y position (upper left coordinate)
      \param w rectangle width
      \param h rectangle height
      \sa fbg_fill, fbg_rect(), fbg_recta()
    */
    extern void fbg_frect(struct _fbg *fbg, int x, int y, int w, int h);

    //! draw a horizontal line
    /*!
      \param fbg pointer to a FBG context / data structure
      \param x line X position (upper left coordinate)
      \param y line Y position (upper left coordinate)
      \param w line width
      \param r
      \param g
      \param b
      \sa fbg_vline, fbg_line()
    */
    extern void fbg_hline(struct _fbg *fbg, int x, int y, int w, unsigned char r, unsigned char g, unsigned char b);

    //! draw a vertical line
    /*!
      \param fbg pointer to a FBG context / data structure
      \param x line X position (upper left coordinate)
      \param y line Y position (upper left coordinate)
      \param h line height
      \param r
      \param g
      \param b
      \sa fbg_hline, fbg_line()
    */
    extern void fbg_vline(struct _fbg *fbg, int x, int y, int h, unsigned char r, unsigned char g, unsigned char b);

    //! draw a line from two points (Bresenham algorithm)
    /*!
      \param fbg pointer to a FBG context / data structure
      \param x1 point 1 X position (upper left coordinate)
      \param y1 point 1 Y position (upper left coordinate)
      \param x2 point 2 X position (upper left coordinate)
      \param y2 point 2 Y position (upper left coordinate)
      \param r
      \param g
      \param b
      \sa fbg_hline(), fbg_vline(), fbg_polygon()
    */
    extern void fbg_line(struct _fbg *fbg, int x1, int y1, int x2, int y2, unsigned char r, unsigned char g, unsigned char b);

    //! draw a polygon
    /*!
      \param fbg pointer to a FBG context / data structure
      \param num_vertices the number of vertices
      \param vertices pointer to a list of vertices (a list of X/Y points)
      \param r
      \param g
      \param b
    */
    extern void fbg_polygon(struct _fbg *fbg, int num_vertices, int *vertices, unsigned char r, unsigned char g, unsigned char b);

    //! clear the background with a color
    /*!
      \param fbg pointer to a FBG context / data structure
      \param r
      \param g
      \param b
      \sa fbg_clear()
    */
    extern void fbg_background(struct _fbg *fbg, unsigned char r, unsigned char g, unsigned char b);

    //! convert HSL values to RGB color
    /*!
      \param color pointer to a _fbg_rgb data structure
      \param h the hue
      \param s the saturation
      \param l the lightness
      \sa rgbToHsl()
    */
    extern void fbg_hslToRGB(struct _fbg_rgb *color, float h, float s, float l);

    //! convert RGB values to HSL color
    /*!
      \param color pointer to a _fbg_hsl data structure
      \param r
      \param g
      \param b
      \sa fbg_hslToRGB()
    */
    extern void fbg_rgbToHsl(struct _fbg_hsl *color, float r, float g, float b);

#ifdef FBG_PARALLEL
    //! draw to the screen
    /*!
      \param fbg pointer to a FBG context / data structure
      \param sync_with_task 1 = wait for all fragment tasks to be finished before rendering
      \param user_mixing a function used to mix the result of fragment tasks
    */
    extern void fbg_draw(struct _fbg *fbg, void (*user_mixing)(struct _fbg *fbg, unsigned char *buffer, int task_id));
#else
    //! draw to the screen
    /*!
      \param fbg pointer to a FBG context / data structure
    */
    extern void fbg_draw(struct _fbg *fbg);
#endif

    //! flip the buffers
    /*!
      \param fbg pointer to a FBG context / data structure
    */
    extern void fbg_flip(struct _fbg *fbg);

    //! create an empty image
    /*!
      \param fbg pointer to a FBG context / data structure
      \param width image width
      \param height image height
      \return _fbg_img data structure pointer
      \sa fbg_freeImage(), fbg_image(), fbg_imageFlip(), fbg_createFont()
    */
    extern struct _fbg_img *fbg_createImage(struct _fbg *fbg, unsigned int width, unsigned int height);

    //! load a PNG image from a file (lodePNG library)
    /*!
      \param fbg pointer to a FBG context / data structure
      \param filename PNG image filename
      \return _fbg_img data structure pointer
      \sa fbg_freeImage(), fbg_image(), fbg_imageFlip(), fbg_createFont(), fbg_imageClip(), fbg_loadJPEG(), fbg_loadImage(), fbg_imageEx(), fbg_imageScale(), fbg_imageColorkey()
    */
#ifndef WITHOUT_PNG
    extern struct _fbg_img *fbg_loadPNG(struct _fbg *fbg, const char *filename);
#endif

    //! load a JPEG image from a file (NanoJPEG library)
    /*!
      \param fbg pointer to a FBG context / data structure
      \param filename JPEG image filename
      \return _fbg_img data structure pointer
      \sa fbg_freeImage(), fbg_image(), fbg_imageFlip(), fbg_createFont(), fbg_imageClip(), fbg_loadPNG(), fbg_loadImage(), fbg_imageEx(), fbg_imageScale(), fbg_imageColorkey()
    */
#ifndef WITHOUT_JPEG
    extern struct _fbg_img *fbg_loadJPEG(struct _fbg *fbg, const char *filename);
#endif

    //! load an image (PNG or JPEG)
    /*!
      \param fbg pointer to a FBG context / data structure
      \param filename JPEG/PNG image filename
      \return _fbg_img data structure pointer
      \sa fbg_freeImage(), fbg_image(), fbg_imageFlip(), fbg_createFont(), fbg_imageClip(), fbg_loadPNG(), fbg_loadJPEG(), fbg_imageEx(), fbg_imageScale(), fbg_imageColorkey()
    */
    extern struct _fbg_img *fbg_loadImage(struct _fbg *fbg, const char *filename);

    //! draw an image
    /*!
      \param fbg pointer to a FBG context / data structure
      \param img image structure pointer
      \param x image X position (upper left coordinate)
      \param y image Y position (upper left coordinate)
      \sa fbg_createImage(), fbg_loadPNG(), fbg_loadJPEG(), fbg_loadImage(), fbg_imageClip(), fbg_freeImage(), fbg_imageFlip(), fbg_imageEx(), fbg_imageScale(), fbg_imageColorkey()
    */
    extern void fbg_image(struct _fbg *fbg, struct _fbg_img *img, int x, int y);

    //! draw an image with colorkey support (image colorkey value will be ignored)
    /*!
      \param fbg pointer to a FBG context / data structure
      \param img image structure pointer
      \param x image X position (upper left coordinate)
      \param y image Y position (upper left coordinate)
      \param cr colorkey red component
      \param cg colorkey green component
      \param cb colorkey blue component
      \sa fbg_createImage(), fbg_loadPNG(), fbg_loadJPEG(), fbg_loadImage(), fbg_imageClip(), fbg_freeImage(), fbg_imageFlip(), fbg_imageEx(), fbg_imageScale(), fbg_image()
    */
    extern void fbg_imageColorkey(struct _fbg *fbg, struct _fbg_img *img, int x, int y, int cr, int cg, int cb);

    //! draw a clipped image
    /*!
      \param fbg pointer to a FBG context / data structure
      \param img image structure pointer
      \param x image X position (upper left coordinate)
      \param y image Y position (upper left coordinate)
      \param cx The X coordinate where to start clipping
      \param cy The Y coordinate where to start clipping
      \param cw The width of the clipped image (from cx)
      \param ch The height of the clipped image (from cy)
      \sa fbg_createImage(), fbg_loadPNG(), fbg_loadJPEG(), fbg_loadImage(), fbg_freeImage(), fbg_image(), fbg_imageFlip(), fbg_imageEx(), fbg_imageScale(), fbg_imageColorkey()
    */
    extern void fbg_imageClip(struct _fbg *fbg, struct _fbg_img *img, int x, int y, int cx, int cy, int cw, int ch);

    //! flip an image vertically
    /*!
      \param img image structure pointer
      \sa fbg_createImage(), fbg_loadPNG(), fbg_loadJPEG(), fbg_loadImage()
    */
    extern void fbg_imageFlip(struct _fbg_img *img);

    //! draw an image with support for clipping and scaling
    /*!
      \param fbg pointer to a FBG context / data structure
      \param img image structure pointer
      \param x image X position (upper left coordinate)
      \param y image Y position (upper left coordinate)
      \param sx The X scale factor
      \param sy The Y scale factor
      \param cx The X coordinate where to start clipping
      \param cy The Y coordinate where to start clipping
      \param cw The width of the clipped image (from cx)
      \param ch The height of the clipped image (from cy)
      \sa fbg_createImage(), fbg_loadPNG(), fbg_loadJPEG(), fbg_loadImage(), fbg_imageClip(), fbg_freeImage(), fbg_image(), fbg_imageFlip(), fbg_imageScale(), fbg_imageColorkey()
    */
    extern void fbg_imageEx(struct _fbg *fbg, struct _fbg_img *img, int x, int y, float sx, float sy, int cx, int cy, int cw, int ch);

    //! free the memory associated with an image
    /*!
      \param img image structure pointer
      \sa fbg_createImage(), fbg_loadPNG(), fbg_loadJPEG(), fbg_loadImage()
    */
    extern void fbg_freeImage(struct _fbg_img *img);

    //! create a bitmap font from an image
    /*!
      \param fbg pointer to a FBG context / data structure
      \param img image structure pointer
      \param glyph_width glyph / character width
      \param glyph_height glyph / character height
      \param first_char the first character of the bitmap font
      \return _fbg_font structure pointer
      \sa fbg_freeFont(), fbg_textFont(), fbg_text(), fbg_write(), fbg_drawFramerate()
    */
    extern struct _fbg_font *fbg_createFont(struct _fbg *fbg, struct _fbg_img *img, int glyph_width, int glyph_height, unsigned char first_char);

    //! set the current font
    /*!
      \param fbg pointer to a FBG context / data structure
      \param font _fbg_font structure pointer
      \sa fbg_createFont(), fbg_text(), fbg_write(), fbg_drawFramerate()
    */
    extern void fbg_textFont(struct _fbg *fbg, struct _fbg_font *font);

    //! set the current text color
    /*!
      \param fbg pointer to a FBG context / data structure
      \param r
      \param g
      \param b
      \sa fbg_createFont(), fbg_write(), fbg_textColorKey(), fbg_textBackground()
    */
    extern void fbg_textColor(struct _fbg *fbg, unsigned char r, unsigned char g, unsigned char b);

    //! set the current text background color (based on colorkey value!)
    /*!
      \param fbg pointer to a FBG context / data structure
      \param r
      \param g
      \param b
      \param a 0 = transparent background (based on colorkey), 255 = full text background
      \sa fbg_createFont(), fbg_write(), fbg_textColorKey(), fbg_textColor()
    */
    extern void fbg_textBackground(struct _fbg *fbg, int r, int g, int b, int a);

    //! set the current text color key
    /*!
      \param fbg pointer to a FBG context / data structure
      \param v grayscale value
      \sa fbg_createFont(), fbg_write(), fbg_textColor()
    */
    extern void fbg_textColorKey(struct _fbg *fbg, unsigned char v);

    //! draw a text
    /*!
      \param fbg pointer to a FBG context / data structure
      \param fnt _fbg_font structure pointer
      \param text the text to draw ('\n' and ' ' are treated automatically)
      \param x
      \param y
      \param r
      \param g
      \param b
      \sa fbg_createFont(), fbg_write(), fbg_textColorkey(), fbg_textBackground()
    */
    extern void fbg_text(struct _fbg *fbg, struct _fbg_font *fnt, char *text, int x, int y, int r, int g, int b);

    //! free the memory associated with a font
    /*!
      \param font _fbg_font structure pointer
      \sa fbg_createFont()
    */
    extern void fbg_freeFont(struct _fbg_font *font);

    //! draw the framerate of a particular parallel task
    /*!
      \param fbg pointer to a FBG context / data structure
      \param fnt _fbg_font structure pointer
      \param task the task id
      \param x
      \param y
      \param r
      \param g
      \param b
    */
    extern void fbg_drawFramerate(struct _fbg *fbg, struct _fbg_font *fnt, int task, int x, int y, int r, int g, int b);

    //! get the framerate of a particular task
    /*!
      \param fbg pointer to a FBG context / data structure
      \param task the task id
      \return task framerate
    */
    extern int fbg_getFramerate(struct _fbg *fbg, int task);

    //! set an offscreen target for all subsequent fbg context draw calls, it is important to reset back to display target once done by calling fbg_drawInto(NULL)
    /*!
      \param fbg pointer to a FBG context / data structure
      \param buffer a buffer to render to, it should be the format of the display, target is the display if NULL
    */
    extern void fbg_drawInto(struct _fbg *fbg, unsigned char *buffer);

    //! pseudo random number between min / max
    /*!
      \param min
      \param max
      \return pseudo random number between min / max
    */
    extern float fbg_randf(float min, float max);

#ifdef FBG_PARALLEL
    //! create a FB Graphics parallel task (also called a 'fragment')
    /*!
      \param fbg pointer to a FBG context / data structure
      \param fragment_start a function taking a _fbg structure as argument
      \param fragment a function taking a _fbg structure as argument and user_data pointer
      \param fragment_stop a function taking user_data pointer as argument
      \param parallel_tasks the number of parallel tasks to register
    */
    extern void fbg_createFragment(struct _fbg *fbg, void *(*fragment_start)(struct _fbg *fbg), void (*fragment)(struct _fbg *fbg, void *user_data), void (*fragment_stop)(struct _fbg *fbg, void *user_data), unsigned int parallel_tasks);
#endif

// ### Helper functions
    //! initialize a FB Graphics context with '/dev/fb0' as framebuffer device and no page flipping
    /*!
      \sa fbg_init(), fbg_customSetup(), fbg_close()
    */
    #define fbg_init() fbg_setup(NULL, 0)

    //! background fade to black with controllable factor
    /*!
      \param fbg pointer to a FBG context / data structure
      \param fade_amount fade amount
      \sa fbg_fadeUp(), fbg_fadeDown()
    */
    #define fbg_fade(fbg, fade_amount) fbg_fadeDown(fbg, fade_amount)

    //! draw a text by using the current font and the current color
    /*!
      \param fbg pointer to a FBG context / data structure
      \param text the text to draw ('\n' and ' ' are treated automatically)
      \param x
      \param y
      \sa fbg_textFont(), fbg_textColor(), fbg_text(), fbg_textColorkey(), fbg_textBackground()
    */
    #define fbg_write(fbg, text, x, y) fbg_text(fbg, &fbg->current_font, text, x, y, fbg->text_color.r, fbg->text_color.g, fbg->text_color.b)

    //! draw a scaled image
    /*!
      \param fbg pointer to a FBG context / data structure
      \param img image structure pointer
      \param x image X position (upper left coordinate)
      \param y image Y position (upper left coordinate)
      \param sx The X scale factor
      \param sy The Y scale factor
      \sa fbg_createImage(), fbg_loadPNG(), fbg_loadJPEG(), fbg_loadImage(), fbg_imageClip(), fbg_freeImage(), fbg_image(), fbg_imageFlip(), fbg_imageEx()
    */
    #define fbg_imageScale(fbg, img, x, y, sx, sy) fbg_imageEx(fbg, img, x, y, sx, sy, 0, 0, img->width, img->height)

    //! integer MAX Math function
    #define _FBG_MAX(a,b) ((a) > (b) ? a : b)
    //! integer MIN Math function
    #define _FBG_MIN(a,b) ((a) < (b) ? a : b)
    //! integer SIGN function
    #define _FBG_SGN(x) ((x<0)?-1:((x>0)?1:0))

    //! convert a degree angle to radians
    #define _FBG_DEGTORAD(angle_degree) ((angle_degree) * M_PI / 180.0)
    //! convert a radian angle to degree
    #define _FBG_RADTODEG(angle_radians) ((angle_radians) * 180.0 / M_PI)
#endif

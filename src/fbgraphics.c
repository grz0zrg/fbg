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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include "lodepng/lodepng.h"

#include "fbgraphics.h"

#ifdef FBG_PARALLEL
void fbg_freelistCleanup(struct lfds711_freelist_state *fs, struct lfds711_freelist_element *fe) {
    struct _fbg_freelist_data *freelist_data;
    freelist_data = LFDS711_FREELIST_GET_VALUE_FROM_ELEMENT(*fe);

    free(freelist_data->buffer);
}

void fbg_ringbufferCleanup(struct lfds711_ringbuffer_state *rs, void *key, void *value, enum lfds711_misc_flag unread_flag) {

}
#endif

struct _fbg *fbg_setup(char *user_fb_device) {
    int x = 0, y = 0;

    struct _fbg *fbg = (struct _fbg *)calloc(1, sizeof(struct _fbg));
    if (!fbg) {
        fprintf(stderr, "fbg_init: fbg malloc failed!\n");
        return NULL;
    }

    char *default_fb_device = "/dev/fb0";
    char *fb_device = user_fb_device ? user_fb_device : default_fb_device;

    fbg->fd = open(fb_device, O_RDWR);

    if (fbg->fd == -1) {
        fprintf(stderr, "fbg_init: Cannot open '%s'!\n", fb_device);
        return NULL;
    }
    
    if (ioctl(fbg->fd, FBIOGET_VSCREENINFO, &fbg->vinfo)) {
        fprintf(stderr, "fbg_init: '%s' Cannot obtain framebuffer FBIOGET_VSCREENINFO informations!\n", fb_device);

        close(fbg->fd);

        return NULL;
    }

    if (ioctl(fbg->fd, FBIOGET_FSCREENINFO, &fbg->finfo)) {
        fprintf(stderr, "fbg_init: '%s' Cannot obtain framebuffer FBIOGET_FSCREENINFO informations!\n", fb_device);

        close(fbg->fd);

        return NULL;
    }

    fbg->width = fbg->vinfo.xres;
    fbg->height = fbg->vinfo.yres;

    fbg->width_n_height = fbg->width * fbg->height;

    fbg->size = fbg->vinfo.xres * fbg->vinfo.yres * (fbg->vinfo.bits_per_pixel / 8);

    fprintf(stdout, "fbg_init: '%s' (%dx%d (%dx%d virtual) %d bpp (%d/%d, %d/%d, %d/%d) (%d smen_len) %d line_length)\n", 
        fb_device,
        fbg->vinfo.xres, fbg->vinfo.yres, 
        fbg->vinfo.xres_virtual, fbg->vinfo.yres_virtual,
        fbg->vinfo.bits_per_pixel,
        fbg->vinfo.red.length,
        fbg->vinfo.red.offset,
        fbg->vinfo.green.length,
        fbg->vinfo.green.offset,
        fbg->vinfo.blue.length,
        fbg->vinfo.blue.offset,
        fbg->finfo.smem_len,
        fbg->finfo.line_length);

    if (fbg->vinfo.bits_per_pixel != 24) {
        fprintf(stderr, "fbg_init: '%s' Unsupported format (only 24 bits framebuffer is supported)!\n", fb_device);

        close(fbg->fd);

        return NULL;
    }

    if (fbg->vinfo.bits_per_pixel == 24 &&
        fbg->vinfo.red.length == 8 &&
        fbg->vinfo.red.offset == 16 &&
        fbg->vinfo.green.length == 8 &&
        fbg->vinfo.blue.length == 8 &&
        fbg->vinfo.blue.offset == 0 &&
        fbg->vinfo.green.offset == 8) {
        fbg->bgr = 1;
    }

    fbg->back_buffer = calloc(1, fbg->size * sizeof(char));
    if (!fbg->back_buffer) {
        fprintf(stderr, "fbg_init: back_buffer calloc failed!\n");

        close(fbg->fd);

        return NULL;
    }

    fbg->disp_buffer = calloc(1, fbg->size * sizeof(char));
    if (!fbg->disp_buffer) {
        fprintf(stderr, "fbg_init: disp_buffer calloc failed!\n");

        free(fbg->back_buffer);
        close(fbg->fd);

        return NULL;
    }

    // initialize framebuffer
    fbg->buffer = (unsigned char *)mmap(0, fbg->finfo.smem_len,
        PROT_WRITE,
        MAP_SHARED,
        fbg->fd, 0);


    gettimeofday(&fbg->fps_start, NULL);

    fbg_textColor(fbg, 255, 255, 255);

#ifdef FBG_PARALLEL
    fbg->state = 1;
    fbg->frame = 0;
    fbg->fps = 0;
#endif

    return fbg;
}

#ifdef FBG_PARALLEL
void fbg_terminateFragments(struct _fbg *fbg) {
    fbg->state = 0;
}

void fbg_clearQueue(struct lfds711_ringbuffer_state *rs, struct lfds711_freelist_state *fs) {
    void *key;
    struct _fbg_freelist_data *freelist_data;
    while (lfds711_ringbuffer_read(rs, &key, NULL)) {
        freelist_data = (struct _fbg_freelist_data *)key;

        LFDS711_FREELIST_SET_VALUE_IN_ELEMENT(freelist_data->freelist_element, freelist_data);
        lfds711_freelist_push(fs, &freelist_data->freelist_element, NULL);
    }
}

void fbg_freeTasks(struct _fbg *fbg) {
    int i = 0;
    for (i = 0; i < fbg->parallel_tasks; i += 1) {
        pthread_join(fbg->tasks[i], NULL);

        struct _fbg_fragment *frag = fbg->fragments[i];

        free(frag->fbg);

        fbg_clearQueue(&frag->ringbuffer_state, &frag->freelist_state);

        lfds711_ringbuffer_cleanup(&frag->ringbuffer_state, fbg_ringbufferCleanup);
        lfds711_freelist_cleanup(&frag->freelist_state, fbg_freelistCleanup);
        free(frag->ringbuffer_element);
        free(frag->fbg_freelist_data);

        free(frag);
    }

    if (fbg->parallel_tasks > 0) {
        pthread_barrier_destroy(fbg->sync_barrier);

        free(fbg->sync_barrier);
    }

    free(fbg->tasks);
    free(fbg->fragments);

    fbg->parallel_tasks = 0;
}
#endif

void fbg_close(struct _fbg *fbg) {
#ifdef FBG_PARALLEL
    fbg_terminateFragments(fbg);

    fbg_freeTasks(fbg);
#endif

    free(fbg->back_buffer);
    free(fbg->disp_buffer);
    munmap(fbg->buffer, fbg->finfo.smem_len);
    close(fbg->fd);

    free(fbg);
}


void fbg_computeFramerate(struct _fbg *fbg, int to_string) {
    gettimeofday(&fbg->fps_stop, NULL);

    double ms = (fbg->fps_stop.tv_sec - fbg->fps_start.tv_sec) * 1000000.0 - (fbg->fps_stop.tv_usec - fbg->fps_start.tv_usec);
    if (ms >= 1000.0) {
        gettimeofday(&fbg->fps_start, NULL);

#ifdef FBG_PARALLEL
        atomic_exchange_explicit(&fbg->fps, fbg->frame, memory_order_relaxed);
#else
        fbg->fps = fbg->frame;
#endif
        fbg->frame = 0;

        if (to_string) {
            sprintf(fbg->fps_char, "%i", fbg->fps);
        }
    }

    fbg->frame += 1;
}

#ifdef FBG_PARALLEL
void fbg_drawFramerate(struct _fbg *fbg, struct _fbg_font *fnt, int task, int x, int y, int r, int g, int b) {
    if (task > fbg->parallel_tasks) {
        return;
    }

    if (!fnt) {
        fnt = &fbg->current_font;
    }

#ifdef FBG_PARALLEL
    if (task > 0) {
        task -= 1;

        static char fps_char[10];

        atomic_uint_fast16_t fps = atomic_load_explicit(&fbg->fragments[task]->fbg->fps, memory_order_relaxed);

        sprintf(fps_char, "%i", (int)fps);

        fbg_text(fbg, fnt, fps_char, x, y, r, g, b);

        return;
    }
#endif

    fbg_text(fbg, fnt, fbg->fps_char, x, y, r, g, b);
}

int fbg_getFramerate(struct _fbg *fbg, int task) {
    if (task > fbg->parallel_tasks) {
        return -1;
    }

#ifdef FBG_PARALLEL
    if (task > 0) {
        task -= 1;
        
        atomic_uint_fast16_t fps = atomic_load_explicit(&fbg->fragments[task]->fbg->fps, memory_order_relaxed);

        return (int)fps;
    }
#endif

    return fbg->fps;
}

int fbg_fragmentState(struct _fbg_fragment *fbg_fragment) {
    return (*fbg_fragment->state);
}

void fbg_fragmentPull(struct _fbg_fragment *fbg_fragment) {
    struct lfds711_freelist_element *freelist_element;

    int pop_result = lfds711_freelist_pop(&fbg_fragment->freelist_state, &freelist_element, NULL);
    if (pop_result == 0) {
#ifdef DEBUG
        fprintf(stderr, "fbg_fragmentPull: Buffers freelist is empty.\n");
        fflush(stdout);
#endif

        fbg_fragment->fbg->back_buffer = 0;

        return;
    }

    fbg_fragment->tmp_fbg_freelist_data = LFDS711_FREELIST_GET_VALUE_FROM_ELEMENT(*freelist_element);

    fbg_fragment->fbg->back_buffer = fbg_fragment->tmp_fbg_freelist_data->buffer;
}

void fbg_fragmentPush(struct _fbg_fragment *fbg_fragment) {
    enum lfds711_misc_flag overwrite_occurred_flag;

    struct _fbg_freelist_data *overwritten_data = NULL;
    lfds711_ringbuffer_write(&fbg_fragment->ringbuffer_state, (void *) (lfds711_pal_uint_t) fbg_fragment->tmp_fbg_freelist_data, NULL, &overwrite_occurred_flag, (void *)&overwritten_data, NULL);
    if (overwrite_occurred_flag == LFDS711_MISC_FLAG_RAISED) {
#ifdef DEBUG
        fprintf(stderr, "fbg_fragmentPush: Overwrite occured.\n");
        fflush(stdout);
#endif

        // okay, push it back!
        LFDS711_FREELIST_SET_VALUE_IN_ELEMENT(overwritten_data->freelist_element, overwritten_data);
        lfds711_freelist_push(&fbg_fragment->freelist_state, &overwritten_data->freelist_element, NULL);
    }
}

void fbg_fragment(struct _fbg_fragment *fbg_fragment) {
    LFDS711_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;

    struct _fbg *fbg = fbg_fragment->fbg;

    fprintf(stdout, "fbg_fragment: Task started\n");

    if (fbg_fragment->user_fragment_start) {
        fbg_fragment->user_data = fbg_fragment->user_fragment_start(fbg);
    } else {
        fbg_fragment->user_data = NULL;
    }

    while (fbg_fragmentState(fbg_fragment)) {
        fbg_fragmentPull(fbg_fragment);

        if (fbg->back_buffer) {
            // execute user fragment
            fbg_fragment->user_fragment(fbg, fbg_fragment->user_data);

            // wait till all fragments are completed
            pthread_barrier_wait(fbg->sync_barrier);

            // push to main thread
            fbg_fragmentPush(fbg_fragment);

            fbg_computeFramerate(fbg, 0);
        }
    }

    if (fbg_fragment->user_fragment_stop) {
        fbg_fragment->user_fragment_stop(fbg, fbg_fragment->user_data);
    }

    fprintf(stdout, "fbg_fragment: Task ended successfully.\n");
}

void fbg_createFragment(struct _fbg *fbg,
        void *(*user_fragment_start)(struct _fbg *fbg),
        void (*user_fragment)(struct _fbg *fbg, void *user_data),
        void (*user_fragment_stop)(struct _fbg *fbg, void *user_data),
        unsigned int parallel_tasks,
        unsigned int queue_size) {
    if (parallel_tasks < 1) {
        return;
    }

    if (fbg->tasks) {
        fbg_freeTasks(fbg);
    }

    fbg->parallel_tasks = parallel_tasks;

    fbg->tasks = (pthread_t *)malloc(sizeof(pthread_t) * fbg->parallel_tasks);
    if (!fbg->tasks) {
        fprintf(stderr, "fbg_createFragment: tasks malloc failed!\n");
        return;
    }

    fbg->fragments = (struct _fbg_fragment **)malloc(sizeof(struct _fbg_fragment *) * fbg->parallel_tasks);
    if (!fbg->fragments) {
        fprintf(stderr, "fbg_createFragment: fragments malloc failed!\n");

        free(fbg->tasks);

        return;
    }

    pthread_barrier_t *sync_barrier = (pthread_barrier_t *)malloc(sizeof(pthread_barrier_t));

    int err = pthread_barrier_init(sync_barrier, NULL, fbg->parallel_tasks);
    if (err) {
        fprintf(stderr, "fbg_createFragment: pthread_barrier_init failed!\n");

        free(fbg->tasks);
        free(fbg->fragments);

        return;
    }

    fbg->sync_barrier = sync_barrier;

    int i = 0, j = 0;
    int created_tasks = 0;
    for (i = 0; i < fbg->parallel_tasks; i += 1) {
        // create a task fbg structure for each threads
        struct _fbg *task_fbg = (struct _fbg *)calloc(1, sizeof(struct _fbg));
        if (!task_fbg) {
            fprintf(stderr, "fbg_createFragment: task_fbg malloc failed!\n");
            continue;
        }

        memcpy(&task_fbg->vinfo, &fbg->vinfo, sizeof(struct fb_var_screeninfo));
        memcpy(&task_fbg->finfo, &fbg->finfo, sizeof(struct fb_fix_screeninfo));

        task_fbg->width = task_fbg->vinfo.xres;
        task_fbg->height = task_fbg->vinfo.yres;

        task_fbg->parallel_tasks = fbg->parallel_tasks;

        task_fbg->width_n_height = task_fbg->width * task_fbg->height;

        task_fbg->size = task_fbg->vinfo.xres * task_fbg->vinfo.yres * (task_fbg->vinfo.bits_per_pixel / 8);

        struct _fbg_fragment *frag = (struct _fbg_fragment *)calloc(1, sizeof(struct _fbg_fragment));
        if (!frag) {
            fprintf(stderr, "fbg_createFragment: frag malloc failed!\n");

            free(task_fbg);

            continue;
        }

        // liblfds
        frag->ringbuffer_element = aligned_alloc(queue_size + 1, sizeof(struct lfds711_ringbuffer_element) * (queue_size + 1));
        if (frag->ringbuffer_element == NULL) {
            fprintf(stderr, "fbg_createFragment: liblfds ringbuffer data structures aligned_alloc error.\n");

            free(task_fbg);
            free(frag);

            continue;
        }

        lfds711_ringbuffer_init_valid_on_current_logical_core(&frag->ringbuffer_state, frag->ringbuffer_element, (queue_size + 1), NULL);
        lfds711_freelist_init_valid_on_current_logical_core(&frag->freelist_state, NULL, 0, NULL);

        frag->fbg_freelist_data = malloc(sizeof(struct _fbg_freelist_data) * queue_size);
        if (frag->fbg_freelist_data == NULL) {
            fprintf(stderr, "fbg_createFragment: fbg_freelist_data malloc error.\n");

            free(task_fbg);
            free(frag->ringbuffer_element);
            free(frag);

            continue;
        }

        // allocate buffers
        for (j = 0; j < queue_size; j += 1) {
            frag->fbg_freelist_data[j].buffer = calloc(1, sizeof(unsigned char) * task_fbg->size);

            LFDS711_FREELIST_SET_VALUE_IN_ELEMENT(frag->fbg_freelist_data[j].freelist_element, &frag->fbg_freelist_data[j]);
            lfds711_freelist_push(&frag->freelist_state, &frag->fbg_freelist_data[j].freelist_element, NULL);
        }
        //

        task_fbg->sync_barrier = fbg->sync_barrier;
        task_fbg->task_id = created_tasks + 1;

        frag->queue_size = queue_size;
        frag->fbg = task_fbg;
        frag->state = &fbg->state;

        frag->user_fragment_start = user_fragment_start;
        frag->user_fragment = user_fragment;
        frag->user_fragment_stop = user_fragment_stop;

        fbg->fragments[created_tasks] = frag;

        err = pthread_create(&fbg->tasks[created_tasks], NULL, fbg_fragment, frag);
        if (err) {
            fprintf(stderr, "fbg_createFragment: pthread_create error '%i'!\n", err);

            free(task_fbg);
            lfds711_ringbuffer_cleanup(&frag->ringbuffer_state, fbg_ringbufferCleanup);
            lfds711_freelist_cleanup(&frag->freelist_state, fbg_freelistCleanup);
            free(frag->ringbuffer_element);
            free(frag->fbg_freelist_data);
            free(frag);

            continue;
        }

        created_tasks += 1;
    }

    if (fbg->parallel_tasks != created_tasks) {
        fprintf(stderr, "fbg_createFragment: Some of the specified number of tasks failed to initialize, as such no tasks were created.\n");

        fbg_freeTasks(fbg);

        fbg->parallel_tasks = 0;
    }
}
#endif

void fbg_fill(struct _fbg *fbg, unsigned char r, unsigned char g, unsigned char b) {
    fbg->fill_color.r = r;
    fbg->fill_color.g = g;
    fbg->fill_color.b = b;
}

void fbg_pixel(struct _fbg *fbg, int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    char *pix_pointer = (char *)(fbg->back_buffer + (y * fbg->finfo.line_length + ((x << 1) + x)));

    *pix_pointer++ = r;
    *pix_pointer++ = g;
    *pix_pointer++ = b;
}

void fbg_fpixel(struct _fbg *fbg, int x, int y) {
    char *pix_pointer = (char *)(fbg->back_buffer + (y * fbg->finfo.line_length));

    memcpy(pix_pointer, &fbg->fill_color, 3);
}

void fbg_rect(struct _fbg *fbg, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    int xx = 0, yy = 0, w3 = w * 3;

    char *pix_pointer = (char *)(fbg->back_buffer + (y * fbg->finfo.line_length + ((x << 1) + x)));

    for (yy = 0; yy < h; yy += 1) {
        for (xx = 0; xx < w; xx += 1) {
            *pix_pointer++ = r;
            *pix_pointer++ = g;
            *pix_pointer++ = b;
        }

        pix_pointer += (fbg->finfo.line_length - w3);
    }
}

void fbg_getPixel(struct _fbg *fbg, int x, int y, struct _fbg_rgb *color) {
    int ofs = y * fbg->finfo.line_length + x * 3;

    memcpy(color, (char *)(fbg->disp_buffer + ofs), 3);
}

#ifdef FBG_PARALLEL
void fbg_additiveMixing(struct _fbg *fbg, unsigned char *buffer, int task_id) {
    int j = 0;
    for (j = 0; j < fbg->size; j += 1)
    {
        fbg->back_buffer[j] = _FBG_MIN(fbg->back_buffer[j] + buffer[j], 255);
    }
}

void fbg_draw(struct _fbg *fbg, int sync_with_tasks, void (*user_mixing)(struct _fbg *fbg, unsigned char *buffer, int task_id)) {
    int i = 0;
    int j = 0;
    int ringbuffer_read_status = 0;
    void *key;

    if (user_mixing == NULL) {
        user_mixing = fbg_additiveMixing;
    }

    for (i = 0; i < fbg->parallel_tasks; i += 1) {
        struct _fbg_fragment *fragment = fbg->fragments[i];
        struct _fbg_freelist_data *freelist_data;
        unsigned char *task_buffer;

        if (sync_with_tasks) {
            while ((ringbuffer_read_status = lfds711_ringbuffer_read(&fragment->ringbuffer_state, &key, NULL)) != 1) {

            }
        } else {
            ringbuffer_read_status = lfds711_ringbuffer_read(&fragment->ringbuffer_state, &key, NULL);
        }

        if (ringbuffer_read_status == 1) {
            freelist_data = (struct _fbg_freelist_data *)key;

            task_buffer = freelist_data->buffer;

            user_mixing(fbg, task_buffer, i + 1);

            LFDS711_FREELIST_SET_VALUE_IN_ELEMENT(freelist_data->freelist_element, freelist_data);
            lfds711_freelist_push(&fragment->freelist_state, &freelist_data->freelist_element, NULL);
        }
    }
#else
void fbg_draw(struct _fbg *fbg) {
#endif

    memcpy(fbg->buffer, fbg->disp_buffer, fbg->size);
}

void fbg_flip(struct _fbg *fbg) {
    unsigned char *tmp_buffer = fbg->disp_buffer;
    fbg->disp_buffer = fbg->back_buffer;
    fbg->back_buffer = tmp_buffer;

    fbg_computeFramerate(fbg, 1);
}

void fbg_clear(struct _fbg *fbg, unsigned char color) {
    memset(fbg->back_buffer, color, fbg->size);
}

void fbg_fadeDown(struct _fbg *fbg, unsigned char rgb_fade_amount) {
    int i = 0;

    char *pix_pointer = (char *)(fbg->back_buffer);

    for (i = 0; i < fbg->width_n_height; i += 1) {
        *pix_pointer = _FBG_MAX(*pix_pointer - rgb_fade_amount, 0);
        *pix_pointer++;
        *pix_pointer = _FBG_MAX(*pix_pointer - rgb_fade_amount, 0);
        *pix_pointer++;
        *pix_pointer = _FBG_MAX(*pix_pointer - rgb_fade_amount, 0);
        *pix_pointer++;
    }
}

void fbg_fadeUp(struct _fbg *fbg, unsigned char rgb_fade_amount) {
    int i = 0;

    char *pix_pointer = (char *)(fbg->back_buffer);

    for (i = 0; i < fbg->width_n_height; i += 1) {
        *pix_pointer = _FBG_MIN(*pix_pointer + rgb_fade_amount, 255);
        *pix_pointer++;
        *pix_pointer = _FBG_MIN(*pix_pointer + rgb_fade_amount, 255);
        *pix_pointer++;
        *pix_pointer = _FBG_MIN(*pix_pointer + rgb_fade_amount, 255);
        *pix_pointer++;
    }
}

void fbg_background(struct _fbg *fbg, unsigned char r, unsigned char g, unsigned char b) {
    int i = 0;

    char *pix_pointer = (char *)(fbg->back_buffer);

    for (i = 0; i < fbg->width_n_height; i += 1) {
        *pix_pointer = r;
        *pix_pointer++;
        *pix_pointer = g;
        *pix_pointer++;
        *pix_pointer = b;
        *pix_pointer++;
    }
}

struct _fbg_font *fbg_createFont(struct _fbg *fbg, struct _fbg_img *img, int glyph_width, int glyph_height, unsigned char first_char) {
    struct _fbg_font *fnt = (struct _fbg_font *)calloc(1, sizeof(struct _fbg_font));
    if (!fnt) {
        fprintf(stderr, "fbg_createFont : calloc failed!\n");
    }

    int glyph_count = (img->width / glyph_width) * (img->height / glyph_height);

    fnt->glyph_coord_x = calloc(1, glyph_count * sizeof(int));
    if (!fnt->glyph_coord_x) {
        fprintf(stderr, "fbg_createFont (%ix%i '%c'): glyph_coord_x calloc failed!\n", glyph_width, glyph_height, first_char);

        free(fnt);

        return NULL;
    }

    fnt->glyph_coord_y = calloc(1, glyph_count * sizeof(int));
    if (!fnt->glyph_coord_y) {
        fprintf(stderr, "fbg_createFont (%ix%i '%c'): glyph_coord_y calloc failed!\n", glyph_width, glyph_height, first_char);

        free(fnt->glyph_coord_x);
        free(fnt);

        return NULL;
    }

    fnt->glyph_width = glyph_width;
    fnt->glyph_height = glyph_height;
    fnt->first_char = first_char;

    int i = 0;

    for (i = 0; i < glyph_count; i += 1) {
        int gcoord = i * glyph_width;
        int gcoordx = gcoord % img->width;
        int gcoordy = (gcoord / img->width) * glyph_height;

        fnt->glyph_coord_x[i] = gcoordx;
        fnt->glyph_coord_y[i] = gcoordy;
    }

    fnt->bitmap = img;

    // assign it by default if there is no default fonts
    if (fbg->current_font.bitmap == 0) {
        fbg_textFont(fbg, fnt);
    }

    return fnt;
}

void fbg_textFont(struct _fbg *fbg, struct _fbg_font *fnt) {
    fbg->current_font = *fnt;
}

void fbg_textColor(struct _fbg *fbg, unsigned char r, unsigned char g, unsigned char b) {
    fbg->text_color.r = r;
    fbg->text_color.g = g;
    fbg->text_color.b = b;
}

void fbg_text(struct _fbg *fbg, struct _fbg_font *fnt, char *text, int x, int y, int r, int g, int b) {
    int i = 0, c = 0, gx, gy;

    if (!fnt) {
        fnt = &fbg->current_font;
    }

    for (i = 0; i < strlen(text); i += 1) {
        char glyph = text[i];

        if (glyph == ' ') {
            c += 1;

            continue;
        }

        if (glyph == '\n') {
            c = 0;
            y += fnt->glyph_height;

            continue;
        }

        unsigned char font_glyph = glyph - fnt->first_char;

        int gcoordx = fnt->glyph_coord_x[font_glyph];
        int gcoordy = fnt->glyph_coord_y[font_glyph];

        for (gy = 0; gy < 8; gy += 1) {
            int ly = gcoordy + gy;
            int fly = ((ly << 1) + ly) * fnt->bitmap->width;
            int py = y + gy;

            for (gx = 0; gx < 8; gx += 1) {
                int lx = gcoordx + gx;
                unsigned char fl = fnt->bitmap->data[fly + ((lx << 1) + lx)];

                if (fl) {
                    fbg_pixel(fbg, x + gx + c * fnt->glyph_width, py, r, g, b);
                }
            }
        }

        c += 1;
    }
}

void fbg_freeFont(struct _fbg_font *font) {
    free(font->glyph_coord_x);
    free(font->glyph_coord_y);

    free(font);
}

struct _fbg_img *fbg_createImage(unsigned int width, unsigned int height) {
    struct _fbg_img *img = (struct _fbg_img *)calloc(1, sizeof(struct _fbg_img));
    if (!img) {
        fprintf(stderr, "fbg_createImage : calloc failed!\n");
    }

    img->data = calloc(1, (width * height * 4) * sizeof(char));
    if (!img->data) {
        fprintf(stderr, "fbg_createImage (%ix%i): calloc failed!\n", width, height);

        free(img);

        return NULL;
    }

    img->width = width;
    img->height = height;

    return img;
}

struct _fbg_img *fbg_loadPNG(struct _fbg *fbg, const char *filename) {
    unsigned char *data;
    unsigned int width;
    unsigned int height;

    unsigned int error = lodepng_decode24_file(&data, &width, &height, filename);
    if (error) {
        fprintf(stderr, "fbg_loadPNG %u: %s\n", error, lodepng_error_text(error));

        return NULL;
    }

    struct _fbg_img *img = fbg_createImage(width, height);
    if (!img) {
        fprintf(stderr, "fbg_loadPNG : Image '%s' data allocation failed\n", filename);

        free(data);

        return NULL;
    }

    if (fbg->bgr) {
        unsigned char *pix_pointer = data;
        unsigned char *pix_pointer2 = data;

        int y, x;
        for (y = 0; y < height; y += 1) {
            for (x = 0; x < width; x += 1) {
                int b = *pix_pointer2++;
                *pix_pointer2++;
                int r = *pix_pointer2++;

                *pix_pointer++ = r;
                *pix_pointer++;
                *pix_pointer++ = b;
            }
        }
    }

    img->data = data;

    return img;
}

void fbg_image(struct _fbg *fbg, struct _fbg_img *img, int x, int y, int w, int h) {
    char *pix_pointer = (char *)(fbg->back_buffer + (y * fbg->finfo.line_length));
    unsigned char *img_pointer = img->data;

    int i = 0;
    int w3 = w * 3;

    for (i = 0; i < h; i += 1) {
        memcpy(pix_pointer, img_pointer, w3);
        pix_pointer += fbg->finfo.line_length;
        img_pointer += img->width * 3;
    }
}

void fbg_imageFlip(struct _fbg_img *img) {
    int height_m1 = img->height - 1;
    int height_d2 = img->height >> 1;

    int i, j;
    for (i = 0; i < height_d2; i += 1) {
        int fy = (height_m1 - i) * img->width;
        int y = i * img->width;

        for (j = 0; j < img->width; j += 1) {
            img->data[y + j] = img->data[fy + j];
        }
    }
}

void fbg_freeImage(struct _fbg_img *img) {
    free(img->data);

    free(img);
}

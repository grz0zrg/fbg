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

#define _NJ_INCLUDE_HEADER_ONLY
#include "nanojpeg/nanojpeg.c"

#include "fbgraphics.h"

#ifdef FBG_PARALLEL
void fbg_freelistCleanup(struct lfds720_freelist_n_state *fs, struct lfds720_freelist_n_element *fe) {
    struct _fbg_freelist_data *freelist_data;
    freelist_data = LFDS720_FREELIST_N_GET_VALUE_FROM_ELEMENT(*fe);

    free(freelist_data->buffer);

    freelist_data->buffer = NULL;
}

void fbg_ringbufferCleanup(struct lfds720_ringbuffer_n_state *rs, void *key, void *value, enum lfds720_misc_flag unread_flag) {

}
#endif

struct _fbg *fbg_customSetup(
        int width, int height,
        void *user_context,
        void (*user_draw)(struct _fbg *fbg),
        void (*user_flip)(struct _fbg *fbg),
        void (*backend_resize)(struct _fbg *fbg, unsigned int new_width, unsigned int new_height),
        void (*user_free)(struct _fbg *fbg)) {
    struct _fbg *fbg = (struct _fbg *)calloc(1, sizeof(struct _fbg));
    if (!fbg) {
        fprintf(stderr, "fbg_customSetup: fbg calloc failed!\n");

        return NULL;
    }

    fbg->user_flip = user_flip;
    fbg->user_draw = user_draw;
    fbg->user_free = user_free;
    fbg->backend_resize = backend_resize;

    fbg->width = width;
    fbg->height = height;

    fbg->components = 3;
    fbg->comp_offset = 0;

    fbg->line_length = fbg->width * fbg->components;

    fbg->width_n_height = fbg->width * fbg->height;

    fbg->vinfo.xres = fbg->width;
    fbg->vinfo.yres = fbg->height;

    fbg->size = fbg->vinfo.xres * fbg->vinfo.yres * fbg->components;

    fbg->user_context = user_context;

    fbg->back_buffer = calloc(1, fbg->size * sizeof(char));
    if (!fbg->back_buffer) {
        fprintf(stderr, "fbg_customSetup: back_buffer calloc failed!\n");

        user_free(fbg);

        free(fbg);

        return NULL;
    }

    fbg->disp_buffer = calloc(1, fbg->size * sizeof(char));
    if (!fbg->disp_buffer) {
        fprintf(stderr, "fbg_customSetup: disp_buffer calloc failed!\n");

        user_free(fbg);

        free(fbg->back_buffer);
        free(fbg);

        return NULL;
    }

    gettimeofday(&fbg->fps_start, NULL);

    fbg_textColor(fbg, 255, 255, 255);

#ifdef FBG_PARALLEL
    fbg->state = 1;
    fbg->frame = 0;
    fbg->fps = 0;

    fbg->fragment_queue_size = 7;
#endif

    fbg->new_width = 0;
    fbg->new_height = 0;

    return fbg;
}

struct _fbg *fbg_setup(char *user_fb_device, int page_flipping) {
    struct _fbg *fbg = (struct _fbg *)calloc(1, sizeof(struct _fbg));
    if (!fbg) {
        fprintf(stderr, "fbg_init: fbg calloc failed!\n");
        return NULL;
    }

    char *default_fb_device = "/dev/fb0";
    char *fb_device = user_fb_device ? user_fb_device : default_fb_device;

    fbg->fd = open(fb_device, O_RDWR);

    if (fbg->fd == -1) {
        fprintf(stderr, "fbg_init: Cannot open '%s'!\n", fb_device);

        return NULL;
    }

    if (ioctl(fbg->fd, FBIOGET_VSCREENINFO, &fbg->vinfo) == -1) {
        fprintf(stderr, "fbg_init: '%s' Cannot obtain framebuffer FBIOGET_VSCREENINFO informations!\n", fb_device);

        close(fbg->fd);

        return NULL;
    }

    if (ioctl(fbg->fd, FBIOGET_FSCREENINFO, &fbg->finfo) == -1) {
        fprintf(stderr, "fbg_init: '%s' Cannot obtain framebuffer FBIOGET_FSCREENINFO informations!\n", fb_device);

        close(fbg->fd);

        return NULL;
    }

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

    if (fbg->vinfo.bits_per_pixel != 16 && fbg->vinfo.bits_per_pixel != 24 && fbg->vinfo.bits_per_pixel != 32) {
        fprintf(stderr, "fbg_init: '%s' Unsupported format (only 16, 24 or 32 bits framebuffer is supported)!\n", fb_device);

        close(fbg->fd);

        return NULL;
    }

    if (fbg->vinfo.bits_per_pixel == 16) {
        fprintf(stdout, "fbg_init: 16 bpp framebuffer detected; page flipping option not supported; all graphics operations will be managed in 24 bits then converted to 16 bpp before drawing.\n");

        fbg->components = 3;

        page_flipping = 0;
    } else {
        fbg->components = fbg->vinfo.bits_per_pixel / 8;
        fbg->comp_offset = fbg->components - 3;
    }

    fbg->width = fbg->vinfo.xres;
    fbg->height = fbg->vinfo.yres;

    fbg->line_length = fbg->width * fbg->components;

    fbg->width_n_height = fbg->width * fbg->height;

    fbg->size = fbg->vinfo.xres * fbg->vinfo.yres * fbg->components;

    if ((fbg->vinfo.bits_per_pixel == 24 || fbg->vinfo.bits_per_pixel == 32) &&
        fbg->vinfo.red.length == 8 &&
        fbg->vinfo.red.offset == 16 &&
        fbg->vinfo.green.length == 8 &&
        fbg->vinfo.blue.length == 8 &&
        fbg->vinfo.blue.offset == 0 &&
        fbg->vinfo.green.offset == 8) {
        fbg->bgr = 1;
    }

    if (fbg->vinfo.bits_per_pixel == 16 &&
        fbg->vinfo.red.offset == 11) {
        fbg->bgr = 1;
    }

    if (page_flipping) {
        // check for page flipping support
        if (ioctl(fbg->fd, FBIOPAN_DISPLAY, &fbg->vinfo) == -1) {
            fprintf(stderr, "fbg_init: '%s' FBIOPAN_DISPLAY / page flipping not supported!\n", fb_device);
        } else {
            // double the virtual height
            fbg->vinfo.yres_virtual = fbg->vinfo.yres_virtual * 2;
            if (ioctl(fbg->fd, FBIOPUT_VSCREENINFO, &fbg->vinfo) == -1) {
                fprintf(stderr, "fbg_init: '%s' FBIOPUT_VSCREENINFO failed, page flipping disabled!\n", fb_device);
            } else {
                fbg->page_flipping = 1;

                fprintf(stdout, "fbg_init: '%s' Page flipping enabled (virtual height was doubled)!\n", fb_device);

                if (ioctl(fbg->fd, FBIOGET_FSCREENINFO, &fbg->finfo) == -1) {
                    fprintf(stderr, "fbg_init: '%s' Cannot obtain framebuffer FBIOGET_FSCREENINFO informations!\n", fb_device);

                    close(fbg->fd);

                    return NULL;
                }
            }
        }

        if (!fbg->page_flipping) {
            fprintf(stderr, "fbg_init: '%s' FBIOPAN_DISPLAY / page flipping not supported!\n", fb_device);
        }
    }

    // initialize framebuffer
    fbg->buffer = (unsigned char *)mmap(0, fbg->finfo.smem_len,
        PROT_WRITE,
        MAP_SHARED,
        fbg->fd, 0);

    memset(fbg->buffer, 0, fbg->finfo.smem_len);

    // setup page flipping
    if (fbg->page_flipping) {
        fbg->disp_buffer = fbg->buffer;
        fbg->back_buffer = fbg->buffer + fbg->width * fbg->components * fbg->height;
    } else {
        // setup front & back buffers
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
    }

    gettimeofday(&fbg->fps_start, NULL);

    fbg_textColor(fbg, 255, 255, 255);

#ifdef FBG_PARALLEL
    fbg->state = 1;
    fbg->frame = 0;
    fbg->fps = 0;

    fbg->fragment_queue_size = 7;
#endif

    fbg->new_width = 0;
    fbg->new_height = 0;

    return fbg;
}

void fbg_setResizeCallback(struct _fbg *fbg, void (*user_resize)(struct _fbg *fbg, unsigned int new_width, unsigned int new_height)) {
    fbg->user_resize = user_resize;
}

void fbg_resize(struct _fbg *fbg, int new_width, int new_height) {
    if (fbg->backend_resize) {
        fbg->backend_resize(fbg, new_width, new_height);
    }

    // we don't allow resizing in framebuffer mode
    if (fbg->buffer == NULL) {
        int new_size = new_width * new_height * fbg->components;

        unsigned char *back_buffer = calloc(1, new_size * sizeof(char));
        if (!back_buffer) {
            fprintf(stderr, "fbg_resize: back_buffer realloc failed!\n");

            return;
        }

        unsigned char *disp_buffer = calloc(1, new_size * sizeof(char));
        if (!disp_buffer) {
            fprintf(stderr, "fbg_resize: disp_buffer realloc failed!\n");

            free(back_buffer);

            return;
        }

        free(fbg->back_buffer);
        free(fbg->disp_buffer);

        fbg->back_buffer = back_buffer;
        fbg->disp_buffer = disp_buffer;

        fbg->width = new_width;
        fbg->height = new_height;

        fbg->line_length = fbg->width * fbg->components;

        fbg->width_n_height = fbg->width * fbg->height;

        fbg->vinfo.xres = fbg->width;
        fbg->vinfo.yres = fbg->height;

        fbg->size = new_size;

#ifdef FBG_PARALLEL
        if (fbg->tasks) {
            fbg_createFragment(fbg, fbg->fragments[0]->user_fragment_start, fbg->fragments[0]->user_fragment, fbg->fragments[0]->user_fragment_stop, fbg->parallel_tasks);
        }
#endif
    }

    if (fbg->user_resize) {
        fbg->user_resize(fbg, new_width, new_height);
    }
}

void fbg_pushResize(struct _fbg *fbg, int new_width, int new_height) {
    if (new_width > 0 && new_height > 0) {
        fbg->new_width = new_width;
        fbg->new_height = new_height;
    }
}

#ifdef FBG_PARALLEL
// basically tell all threads/fragments that they should stop
void fbg_terminateFragments(struct _fbg *fbg) {
    // this is the context state because all fragments actually share the main context state
    // as such if fragments are terminated temporarily by calling this function, main context state should be restored again afterward
    // TODO : provide a state per fragment to avoid confusions
    fbg->state = 0;
}

void fbg_clearQueue(struct lfds720_ringbuffer_n_state *rs, struct lfds720_freelist_n_state *fs) {
    void *key;
    struct _fbg_freelist_data *freelist_data;
    while (lfds720_ringbuffer_n_read(rs, &key, NULL)) {
        freelist_data = (struct _fbg_freelist_data *)key;

        LFDS720_FREELIST_N_SET_VALUE_IN_ELEMENT(freelist_data->freelist_element, freelist_data);
#ifdef LFDS711
        lfds720_freelist_n_threadsafe_push(fs, &freelist_data->freelist_element, NULL);
#else
        lfds720_freelist_n_threadsafe_push(fs, NULL, &freelist_data->freelist_element);
#endif
    }
}

void fbg_freeTasks(struct _fbg *fbg) {
    int i = 0;
    for (i = 0; i < fbg->parallel_tasks; i += 1) {
        pthread_join(fbg->tasks[i], NULL);

        struct _fbg_fragment *frag = fbg->fragments[i];

        free(frag->fbg);

        fbg_clearQueue(frag->ringbuffer_state, frag->freelist_state);

        lfds720_ringbuffer_n_cleanup(frag->ringbuffer_state, fbg_ringbufferCleanup);
        lfds720_freelist_n_cleanup(frag->freelist_state, fbg_freelistCleanup);
        free(frag->ringbuffer_element);
        free(frag->ringbuffer_state);
        free(frag->freelist_state);
        free(frag->fbg_freelist_data);

        free(frag);
    }

    if (fbg->parallel_tasks > 0) {
        pthread_barrier_destroy(fbg->sync_barrier);

        free(fbg->sync_barrier);

        fbg->sync_barrier = NULL;
    }

    free(fbg->tasks);
    free(fbg->fragments);

    fbg->tasks = NULL;
    fbg->fragments = NULL;

    fbg->parallel_tasks = 0;
}
#endif

void fbg_close(struct _fbg *fbg) {
#ifdef FBG_PARALLEL
    fbg_terminateFragments(fbg);

    fbg_freeTasks(fbg);
#endif

    if (fbg->user_free) {
        fbg->user_free(fbg);
    }

    if (!fbg->page_flipping) {
        free(fbg->back_buffer);
        free(fbg->disp_buffer);
    }

    if (fbg->buffer) {
        munmap(fbg->buffer, fbg->finfo.smem_len);
        close(fbg->fd);
    }

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
            sprintf(fbg->fps_char, "%lu", (long unsigned int)fbg->fps);
        }
    }

    fbg->frame += 1;
}


void fbg_drawFramerate(struct _fbg *fbg, struct _fbg_font *fnt, int task, int x, int y, int r, int g, int b) {
#ifdef FBG_PARALLEL
    if (task > fbg->parallel_tasks) {
        return;
    }
#endif

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
#ifdef FBG_PARALLEL
    if (task > fbg->parallel_tasks) {
        return -1;
    }

    if (task > 0) {
        task -= 1;

        atomic_uint_fast16_t fps = atomic_load_explicit(&fbg->fragments[task]->fbg->fps, memory_order_relaxed);

        return (int)fps;
    }
#endif

    return fbg->fps;
}

#ifdef FBG_PARALLEL
int fbg_fragmentState(struct _fbg_fragment *fbg_fragment) {
    return (*fbg_fragment->state);
}

void fbg_fragmentPull(struct _fbg_fragment *fbg_fragment) {
    struct lfds720_freelist_n_element *freelist_element;

#ifdef LFDS711
    int pop_result = lfds720_freelist_n_threadsafe_pop(fbg_fragment->freelist_state, &freelist_element, NULL);
#else
    int pop_result = lfds720_freelist_n_threadsafe_pop(fbg_fragment->freelist_state, NULL, &freelist_element);
#endif
    if (pop_result == 0) {
        fbg_fragment->fbg->back_buffer = 0;

        return;
    }

    fbg_fragment->tmp_fbg_freelist_data = LFDS720_FREELIST_N_GET_VALUE_FROM_ELEMENT(*freelist_element);

    fbg_fragment->fbg->back_buffer = fbg_fragment->tmp_fbg_freelist_data->buffer;
}

void fbg_fragmentPush(struct _fbg_fragment *fbg_fragment) {
    enum lfds720_misc_flag overwrite_occurred_flag;

    struct _fbg_freelist_data *overwritten_data = NULL;
    lfds720_ringbuffer_n_write(fbg_fragment->ringbuffer_state, (void *) (lfds720_pal_uint_t) fbg_fragment->tmp_fbg_freelist_data, NULL, &overwrite_occurred_flag, (void *)&overwritten_data, NULL);
    if (overwrite_occurred_flag == LFDS720_MISC_FLAG_RAISED) {
#ifdef DEBUG
        fprintf(stderr, "fbg_fragmentPush: Overwrite occured.\n");
        fflush(stdout);
#endif

        // okay, push it back!
        LFDS720_FREELIST_N_SET_VALUE_IN_ELEMENT(overwritten_data->freelist_element, overwritten_data);
#ifdef LFDS711
        lfds720_freelist_n_threadsafe_push(fbg_fragment->freelist_state, &overwritten_data->freelist_element, NULL);
#else
        lfds720_freelist_n_threadsafe_push(fbg_fragment->freelist_state, NULL, &overwritten_data->freelist_element);
#endif
    }
}

void fbg_fragment(struct _fbg_fragment *fbg_fragment) {
    LFDS720_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_PHYSICAL_CORE;

    struct _fbg *fbg = fbg_fragment->fbg;

    //fprintf(stdout, "fbg_fragment: Task started\n");

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

    //fprintf(stdout, "fbg_fragment: Task ended successfully.\n");
}

void fbg_createFragment(struct _fbg *fbg,
        void *(*user_fragment_start)(struct _fbg *fbg),
        void (*user_fragment)(struct _fbg *fbg, void *user_data),
        void (*user_fragment_stop)(struct _fbg *fbg, void *user_data),
        unsigned int parallel_tasks) {
    if (parallel_tasks < 1) {
        return;
    }

    if (fbg->tasks) {
        fbg_terminateFragments(fbg);
        
        fbg_freeTasks(fbg);

        // see fbg_terminateFragments function for explanations
        fbg->state = 1;
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
            fprintf(stderr, "fbg_createFragment: task_fbg calloc failed!\n");
            continue;
        }

        memcpy(&task_fbg->vinfo, &fbg->vinfo, sizeof(struct fb_var_screeninfo));
        memcpy(&task_fbg->finfo, &fbg->finfo, sizeof(struct fb_fix_screeninfo));

        task_fbg->components = fbg->components;
        task_fbg->comp_offset = fbg->comp_offset;
        task_fbg->line_length = fbg->line_length;

        task_fbg->width = fbg->vinfo.xres;
        task_fbg->height = fbg->vinfo.yres;

        task_fbg->parallel_tasks = fbg->parallel_tasks;

        task_fbg->width_n_height = task_fbg->width * task_fbg->height;

        task_fbg->size = task_fbg->vinfo.xres * task_fbg->vinfo.yres * task_fbg->components;

        struct _fbg_fragment *frag = (struct _fbg_fragment *)calloc(1, sizeof(struct _fbg_fragment));
        if (!frag) {
            fprintf(stderr, "fbg_createFragment: frag calloc failed!\n");

            free(task_fbg);

            continue;
        }

        // liblfds
        frag->ringbuffer_element = aligned_alloc(LFDS720_PAL_ATOMIC_ISOLATION_LENGTH_IN_BYTES, sizeof(struct lfds720_ringbuffer_n_element) * (fbg->fragment_queue_size + 1));
        if (frag->ringbuffer_element == NULL) {
            fprintf(stderr, "fbg_createFragment: liblfds ringbuffer data structures aligned_alloc error.\n");

            free(task_fbg);
            free(frag);

            continue;
        }

        frag->ringbuffer_state = aligned_alloc(LFDS720_PAL_ATOMIC_ISOLATION_LENGTH_IN_BYTES, sizeof(struct lfds720_ringbuffer_n_state));
        if (frag->ringbuffer_state == NULL) {
            fprintf(stderr, "fbg_createFragment: liblfds ringbuffer state aligned_alloc error.\n");

            free(task_fbg);
            free(frag);
            free(frag->ringbuffer_element);

            continue;
        }

        frag->freelist_state = aligned_alloc(LFDS720_PAL_ATOMIC_ISOLATION_LENGTH_IN_BYTES, sizeof(struct lfds720_freelist_n_state));
        if (frag->freelist_state == NULL) {
            fprintf(stderr, "fbg_createFragment: liblfds freelist state aligned_alloc error.\n");

            free(task_fbg);
            free(frag);
            free(frag->ringbuffer_element);
            free(frag->ringbuffer_state);

            continue;
        }

        lfds720_ringbuffer_n_init_valid_on_current_logical_core(frag->ringbuffer_state, frag->ringbuffer_element, (fbg->fragment_queue_size + 1), NULL);
#ifdef LFDS711
        lfds720_freelist_n_init_valid_on_current_logical_core(frag->freelist_state, NULL, 0, NULL);
#else
        lfds720_freelist_n_init_valid_on_current_logical_core(frag->freelist_state, NULL);
#endif

        frag->fbg_freelist_data = malloc(sizeof(struct _fbg_freelist_data) * fbg->fragment_queue_size);
        if (frag->fbg_freelist_data == NULL) {
            fprintf(stderr, "fbg_createFragment: fbg_freelist_data malloc error.\n");

            free(task_fbg);
            free(frag->ringbuffer_element);
            free(frag->ringbuffer_state);
            free(frag->freelist_state);
            free(frag);

            continue;
        }

        // allocate buffers
        for (j = 0; j < fbg->fragment_queue_size; j += 1) {
            frag->fbg_freelist_data[j].buffer = calloc(1, sizeof(unsigned char) * task_fbg->size);

            LFDS720_FREELIST_N_SET_VALUE_IN_ELEMENT(frag->fbg_freelist_data[j].freelist_element, &frag->fbg_freelist_data[j]);
#ifdef LFDS711
            lfds720_freelist_n_threadsafe_push(frag->freelist_state, &frag->fbg_freelist_data[j].freelist_element, NULL);
#else
            lfds720_freelist_n_threadsafe_push(frag->freelist_state, NULL, &frag->fbg_freelist_data[j].freelist_element);
#endif
        }
        //

        task_fbg->sync_barrier = fbg->sync_barrier;
        task_fbg->task_id = created_tasks + 1;

        //frag->queue_size = fbg->fragment_queue_size;
        frag->fbg = task_fbg;
        frag->state = &fbg->state;

        frag->user_fragment_start = user_fragment_start;
        frag->user_fragment = user_fragment;
        frag->user_fragment_stop = user_fragment_stop;

        fbg->fragments[created_tasks] = frag;

        err = pthread_create(&fbg->tasks[created_tasks], NULL, (void * (*)(void *))fbg_fragment, frag);
        if (err) {
            fprintf(stderr, "fbg_createFragment: pthread_create error '%i'!\n", err);

            free(task_fbg);
            lfds720_ringbuffer_n_cleanup(frag->ringbuffer_state, fbg_ringbufferCleanup);
            lfds720_freelist_n_cleanup(frag->freelist_state, fbg_freelistCleanup);
            free(frag->ringbuffer_element);
            free(frag->ringbuffer_state);
            free(frag->freelist_state);
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
    char *pix_pointer = (char *)(fbg->back_buffer + (y * fbg->line_length + x * fbg->components));

    *pix_pointer++ = r;
    *pix_pointer++ = g;
    *pix_pointer++ = b;
    pix_pointer += fbg->comp_offset;
}

void fbg_pixela(struct _fbg *fbg, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    char *pix_pointer = (char *)(fbg->back_buffer + (y * fbg->line_length + x * fbg->components));

    *pix_pointer = ((a * r + (255 - a) * (*pix_pointer)) >> 8);
    pix_pointer += 1;
    *pix_pointer = ((a * g + (255 - a) * (*pix_pointer)) >> 8);;
    pix_pointer += 1;
    *pix_pointer = ((a * b + (255 - a) * (*pix_pointer)) >> 8);;
    pix_pointer += 1;
    pix_pointer += fbg->comp_offset;
}

void fbg_fpixel(struct _fbg *fbg, int x, int y) {
    char *pix_pointer = (char *)(fbg->back_buffer + (y * fbg->line_length));

    memcpy(pix_pointer, &fbg->fill_color, fbg->components);
}

void fbg_plot(struct _fbg *fbg, int index, unsigned char value) {
    fbg->back_buffer[index] = value;
}

void fbg_hline(struct _fbg *fbg, int x, int y, int w, unsigned char r, unsigned char g, unsigned char b) {
    int xx;

    char *pix_pointer = (char *)(fbg->back_buffer + (y * fbg->line_length + x * fbg->components));

    for (xx = 0; xx < w; xx += 1) {
        *pix_pointer++ = r;
        *pix_pointer++ = g;
        *pix_pointer++ = b;
        pix_pointer += fbg->comp_offset;
    }
}

void fbg_vline(struct _fbg *fbg, int x, int y, int h, unsigned char r, unsigned char g, unsigned char b) {
    int yy;

    char *pix_pointer = (char *)(fbg->back_buffer + (y * fbg->line_length + x * fbg->components));

    for (yy = 0; yy < h; yy += 1) {
        *pix_pointer++ = r;
        *pix_pointer++ = g;
        *pix_pointer++ = b;

        pix_pointer += fbg->line_length - 3;
    }
}

// source : http://www.brackeen.com/vga/shapes.html
void fbg_line(struct _fbg *fbg, int x1, int y1, int x2, int y2, unsigned char r, unsigned char g, unsigned char b) {
    int i, dx, dy, sdx, sdy, dxabs, dyabs, x, y, px, py;

    dx = x2 - x1;
    dy = y2 - y1;
    dxabs = abs(dx);
    dyabs = abs(dy);
    sdx = _FBG_SGN(dx);
    sdy = _FBG_SGN(dy);
    x = dyabs >> 1;
    y = dxabs >> 1;
    px = x1;
    py = y1;

    char *pix_pointer = (char *)(fbg->back_buffer + (py * fbg->line_length + px * fbg->components));

    *pix_pointer++ = r;
    *pix_pointer++ = g;
    *pix_pointer++ = b;
    pix_pointer += fbg->comp_offset;

    if (dxabs >= dyabs) {
        for (i = 0; i < dxabs; i += 1) {
            y += dyabs;
            if (y >= dxabs)
            {
                y -= dxabs;
                py += sdy;
            }
            px += sdx;

            fbg_pixel(fbg, px, py, r, g, b);
        }
    } else {
        for (i = 0; i < dyabs; i += 1) {
            x += dxabs;
            if (x >= dyabs)
            {
                x -= dyabs;
                px += sdx;
            }
            py += sdy;

            fbg_pixel(fbg, px, py, r, g, b);
        }
    }
}

void fbg_polygon(struct _fbg *fbg, int num_vertices, int *vertices, unsigned char r, unsigned char g, unsigned char b) {
    int i;

    for (i = 0; i < num_vertices - 1; i += 1) {
        fbg_line(fbg, vertices[(i << 1) + 0],
            vertices[(i << 1) + 1],
            vertices[(i << 1) + 2],
            vertices[(i << 1) + 3],
            r, g, b);
    }

    fbg_line(fbg, vertices[0],
         vertices[1],
         vertices[(num_vertices << 1) - 2],
         vertices[(num_vertices << 1) - 1],
         r, g, b);
}

void fbg_recta(struct _fbg *fbg, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    int xx = 0, yy = 0, w3 = w * fbg->components;

    char *pix_pointer = (char *)(fbg->back_buffer + (y * fbg->line_length + x * fbg->components));

    for (yy = 0; yy < h; yy += 1) {
        for (xx = 0; xx < w; xx += 1) {
            *pix_pointer = ((a * r + (255 - a) * (*pix_pointer)) >> 8);
            pix_pointer += 1;
            *pix_pointer = ((a * g + (255 - a) * (*pix_pointer)) >> 8);
            pix_pointer += 1;
            *pix_pointer = ((a * b + (255 - a) * (*pix_pointer)) >> 8);
            pix_pointer += 1;
            pix_pointer += fbg->comp_offset;
        }

        pix_pointer += (fbg->line_length - w3);
    }
}

void fbg_rect(struct _fbg *fbg, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    int xx = 0, yy = 0, w3 = w * fbg->components;

    char *pix_pointer = (char *)(fbg->back_buffer + (y * fbg->line_length + x * fbg->components));

    for (yy = 0; yy < h; yy += 1) {
        for (xx = 0; xx < w; xx += 1) {
            *pix_pointer++ = r;
            *pix_pointer++ = g;
            *pix_pointer++ = b;
            pix_pointer += fbg->comp_offset;
        }

        pix_pointer += (fbg->line_length - w3);
    }
}

void fbg_frect(struct _fbg *fbg, int x, int y, int w, int h) {
    int xx, yy, w3 = w * fbg->components;

    char *fpix_pointer = (char *)(fbg->back_buffer + (y * fbg->line_length + x * fbg->components));

    char *org_pointer = fpix_pointer;
    char *pix_pointer = fpix_pointer;

    for (xx = 0; xx < w; xx += 1) {
        *pix_pointer++ = fbg->fill_color.r;
        *pix_pointer++ = fbg->fill_color.g;
        *pix_pointer++ = fbg->fill_color.b;
        pix_pointer += fbg->comp_offset;
    }

    for (yy = 1; yy < h; yy += 1) {
        fpix_pointer += fbg->line_length;

        memcpy(fpix_pointer, org_pointer, w3);
    }
}

void fbg_getPixel(struct _fbg *fbg, int x, int y, struct _fbg_rgb *color) {
    int ofs = y * fbg->line_length + x * fbg->components;

    memcpy(color, (char *)(fbg->back_buffer + ofs), fbg->components);
}

#ifdef FBG_PARALLEL
void fbg_additiveMixing(struct _fbg *fbg, unsigned char *buffer, int task_id) {
    int j = 0;
    for (j = 0; j < fbg->size; j += 1) {
        fbg->back_buffer[j] = _FBG_MIN(fbg->back_buffer[j] + buffer[j], 255);
    }
}

void fbg_draw(struct _fbg *fbg, int sync_with_tasks, void (*user_mixing)(struct _fbg *fbg, unsigned char *buffer, int task_id)) {
    int i = 0;
    int ringbuffer_read_status = 0;
    void *key;

    if (user_mixing == NULL) {
        user_mixing = fbg_additiveMixing;
    }

    for (i = 0; i < fbg->parallel_tasks; i += 1) {
        struct _fbg_fragment *fragment = fbg->fragments[i];
        struct _fbg_freelist_data *freelist_data;
        unsigned char *task_buffer = NULL;

        if (sync_with_tasks) {
            while ((ringbuffer_read_status = lfds720_ringbuffer_n_read(fragment->ringbuffer_state, &key, NULL)) != 1) {

            }
        } else {
            ringbuffer_read_status = lfds720_ringbuffer_n_read(fragment->ringbuffer_state, &key, NULL);
        }

        if (ringbuffer_read_status == 1) {
            freelist_data = (struct _fbg_freelist_data *)key;

            task_buffer = freelist_data->buffer;

            //fbg->curr_task_buffer = task_buffer;

            user_mixing(fbg, task_buffer, i + 1);

            LFDS720_FREELIST_N_SET_VALUE_IN_ELEMENT(freelist_data->freelist_element, freelist_data);
#ifdef LFDS711
            lfds720_freelist_n_threadsafe_push(fragment->freelist_state, &freelist_data->freelist_element, NULL);
#else
            lfds720_freelist_n_threadsafe_push(fragment->freelist_state, NULL, &freelist_data->freelist_element);
#endif
            
        }
    }
#else
void fbg_draw(struct _fbg *fbg) {
#endif
#ifdef FBIO_WAITFORVSYNC
    static int dummy = 0;
    ioctl(fbg->fd, FBIO_WAITFORVSYNC, &dummy);
#endif

    if (fbg->user_draw) {
        fbg->user_draw(fbg);
    } else if (fbg->page_flipping == 0) {
        if (fbg->vinfo.bits_per_pixel == 16) {
            unsigned char *pix_pointer_src = fbg->disp_buffer;
            unsigned char *pix_pointer_dst = fbg->buffer;

            int i = 0;

            for (i = 0; i < fbg->width_n_height; i += 1) {
                unsigned int v = ((*pix_pointer_src++ >> 3) & 0x1f);
                v |= ((*pix_pointer_src++ >> 2) & 0x3f) << 5;
                v |= ((*pix_pointer_src++ >> 3) & 0x1f) << 11;

                *pix_pointer_dst++ = v;
                *pix_pointer_dst++ = v >> 8;;
            }
        } else {
            memcpy(fbg->buffer, fbg->disp_buffer, fbg->size);
        }
    }

    // resize the context (registered) by fbg_pushResize if needed
    // note : we process the resize event here to be sure that a single resize is processed in the main thread, avoiding potential issues with fragments / caller thread
    if (fbg->new_width > 0 && fbg->new_height > 0) {
        fbg_resize(fbg, fbg->new_width, fbg->new_height);

        fbg->new_width = 0;
        fbg->new_height = 0;
    }
}

void fbg_flip(struct _fbg *fbg) {
    if (fbg->page_flipping) {
        if (fbg->vinfo.yoffset == 0) {
            fbg->vinfo.yoffset = fbg->height;
        } else {
            fbg->vinfo.yoffset = 0;
        }

        if (ioctl(fbg->fd, FBIOPAN_DISPLAY, &fbg->vinfo) == -1) {
            fprintf(stderr, "fbg_flip: FBIOPAN_DISPLAY failed!\n");
        }
    }

    unsigned char *tmp_buffer = fbg->disp_buffer;
    fbg->disp_buffer = fbg->back_buffer;
    fbg->back_buffer = tmp_buffer;

    if (fbg->user_flip) {
        fbg->user_flip(fbg);
    }

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
        pix_pointer += fbg->comp_offset;
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
        pix_pointer += fbg->comp_offset;
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
        pix_pointer += fbg->comp_offset;
    }
}

float fbg_hue2rgb(float v1, float v2, float vH) {
	if (vH < 0)
		vH += 1;

	if (vH > 1)
		vH -= 1;

	if ((6 * vH) < 1)
		return (v1 + (v2 - v1) * 6 * vH);

	if ((2 * vH) < 1)
		return v2;

	if ((3 * vH) < 2)
		return (v1 + (v2 - v1) * ((2.0f / 3) - vH) * 6);

	return v1;
}

void fbg_hslToRGB(struct _fbg_rgb *color, float h, float s, float l) {
	if (s == 0) {
		color->r = color->g = color->b = (unsigned char)(l * 255);
	} else {
		float v1, v2;
		float hue = (float)h / 360;

		v2 = (l < 0.5) ? (l * (1 + s)) : ((l + s) - (l * s));
		v1 = 2 * l - v2;

		color->r = (unsigned char)(255 * fbg_hue2rgb(v1, v2, hue + (1.0f / 3)));
		color->g = (unsigned char)(255 * fbg_hue2rgb(v1, v2, hue));
		color->b = (unsigned char)(255 * fbg_hue2rgb(v1, v2, hue - (1.0f / 3)));
	}
}

void rgbToHsl(struct _fbg_hsl *color, float r, float g, float b) {
    r /= 255.0f, g /= 255.0f, b /= 255.0f;
    int max = fmaxf(fmaxf(r, g), b), min = fminf(fminf(r, g), b);
    float h = 0, s, l = (max + min) / 2.0f;

    int ri = r, gi = g, bi = b;

    if (max == min){
        h = s = 0; // achromatic
    } else {
        float d = max - min;
        s = l > 0.5f ? d / (2.0f - max - min) : d / (max + min);

        if (max == ri)
            h = (g - b) / d + (g < b ? 6.0f : 0);
        else if (max == gi)
            h = (b - r) / d + 2.0f;
        else if (max == bi)
            h = (r - g) / d + 4.0f;

        h /= 6.0f;
    }

    color->h = h;
    color->s = s;
    color->l = l;
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

void fbg_textColorKey(struct _fbg *fbg, unsigned char v) {
    fbg->text_colorkey = v;
}

void fbg_textBackground(struct _fbg *fbg, int r, int g, int b, int a) {
    fbg->text_background.r = r;
    fbg->text_background.g = g;
    fbg->text_background.b = b;
    fbg->text_alpha = a;
}

void fbg_text(struct _fbg *fbg, struct _fbg_font *fnt, char *text, int x, int y, int r, int g, int b) {
    int i = 0, c = 0, gx, gy;

    if (!fnt) {
        fnt = &fbg->current_font;
    }

    for (i = 0; i < strlen(text); i += 1) {
        char glyph = text[i];

        if (glyph == ' ') {
            fbg_recta(fbg, x + c * fnt->glyph_width, y, fnt->glyph_width, fnt->glyph_height, fbg->text_background.r, fbg->text_background.g, fbg->text_background.b, fbg->text_alpha);
            
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

        for (gy = 0; gy < fnt->glyph_height; gy += 1) {
            int ly = gcoordy + gy;
            int fly = ly * fnt->bitmap->width;
            int py = y + gy;

            for (gx = 0; gx < fnt->glyph_width; gx += 1) {
                int lx = gcoordx + gx;
                unsigned char fl = fnt->bitmap->data[(fly + lx) * fbg->components];

                if (fl == fbg->text_colorkey) {
                    fbg_pixela(fbg, x + gx + c * fnt->glyph_width, py, fbg->text_background.r, fbg->text_background.g, fbg->text_background.b, fbg->text_alpha);
                } else {
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

struct _fbg_img *fbg_createImage(struct _fbg *fbg, unsigned int width, unsigned int height) {
    struct _fbg_img *img = (struct _fbg_img *)calloc(1, sizeof(struct _fbg_img));
    if (!img) {
        fprintf(stderr, "fbg_createImage : calloc failed!\n");
    }

    img->data = calloc(1, (width * height * fbg->components) * sizeof(char));
    if (!img->data) {
        fprintf(stderr, "fbg_createImage (%ix%i): calloc failed!\n", width, height);

        free(img);

        return NULL;
    }

    img->width = width;
    img->height = height;

    return img;
}

struct _fbg_img *fbg_loadJPEG(struct _fbg *fbg, const char *filename) {
    unsigned char *data;
    unsigned int width;
    unsigned int height;

    size_t size;

    FILE *f = fopen(filename, "rb");

    if (!f) {
        fprintf(stderr, "fbg_loadJPEG '%s' : fopen failed.\n", filename);

        return NULL;
    }

    fseek(f, 0, SEEK_END);
    size = (int)ftell(f);
    data = (unsigned char*) malloc(size);

    if (!data) {
        fprintf(stderr, "fbg_loadJPEG '%s' : malloc failed.\n", filename);

        fclose(f);

        return NULL;
    }

    fseek(f, 0, SEEK_SET);
    size = (int)fread(data, 1, size, f);
    fclose(f);

    njInit();

    nj_result_t nj_err = njDecode(data, size);
    if (nj_err != NJ_OK) {
        free(data);

        fprintf(stderr, "fbg_loadJPEG '%s' : njDecode failed with error code '%i'.\n", filename, nj_err);

        return NULL;
    }

    width = njGetWidth();
    height = njGetHeight();

    free(data);

    struct _fbg_img *img = fbg_createImage(fbg, width, height);
    if (!img) {
        fprintf(stderr, "fbg_loadJPEG '%s' : Image data allocation failed\n", filename);

        njDone();

        return NULL;
    }

    data = njGetImage();

    unsigned char *pix_pointer = data;
    unsigned char *pix_pointer2 = data;

    if (fbg->bgr) {
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

    pix_pointer = data;
    pix_pointer2 = img->data;

    int i;
    for (i = 0; i < njGetImageSize(); i += 1) {
        *pix_pointer2++ = *pix_pointer++;
        *pix_pointer2++ = *pix_pointer++;
        *pix_pointer2++ = *pix_pointer++;
        pix_pointer2 += fbg->comp_offset;
    }

    njDone();

    return img;
}

struct _fbg_img *fbg_loadPNG(struct _fbg *fbg, const char *filename) {
    unsigned char *data;
    unsigned int width;
    unsigned int height;
    unsigned int error;

    if (fbg->components == 3) {
        error = lodepng_decode24_file(&data, &width, &height, filename);
    } else {
        error = lodepng_decode32_file(&data, &width, &height, filename);
    }

    if (error) {
        fprintf(stderr, "fbg_loadPNG %u: %s\n", error, lodepng_error_text(error));

        return NULL;
    }

    struct _fbg_img *img = fbg_createImage(fbg, width, height);
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
                pix_pointer2 += fbg->comp_offset;

                *pix_pointer++ = r;
                *pix_pointer++;
                *pix_pointer++ = b;
                pix_pointer += fbg->comp_offset;
            }
        }
    }

    memcpy(img->data, data, width * height * fbg->components);

    free(data);

    return img;
}

struct _fbg_img *fbg_loadImage(struct _fbg *fbg, const char *filename) {
    struct _fbg_img *img = fbg_loadPNG(fbg, filename);

    if (img == NULL) {
        img = fbg_loadJPEG(fbg, filename);
    }

    return img;
}

void fbg_image(struct _fbg *fbg, struct _fbg_img *img, int x, int y) {
    unsigned char *pix_pointer = (unsigned char *)(fbg->back_buffer + (y * fbg->line_length));
    unsigned char *img_pointer = img->data;

    int i = 0;
    int w3 = img->width * fbg->components;

    for (i = 0; i < img->height; i += 1) {
        memcpy(pix_pointer, img_pointer, w3);
        pix_pointer += fbg->line_length;
        img_pointer += w3;
    }
}

void fbg_imageColorkey(struct _fbg *fbg, struct _fbg_img *img, int x, int y, int cr, int cg, int cb) {
    unsigned char *img_pointer = img->data;

    int i = 0, j = 0;
    
    for (i = 0; i < img->height; i += 1) {
        unsigned char *pix_pointer = (unsigned char *)(fbg->back_buffer + (i * fbg->line_length));
        for (j = 0; j < img->width; j += 1) {
            int ir = *img_pointer++,
                ig = *img_pointer++,
                ib = *img_pointer++;

            img_pointer += fbg->comp_offset;

            if (ir == cr && ig == cg && ib == cb) {
                pix_pointer += fbg->components;
                continue;
            }

            *pix_pointer++ = ir;
            *pix_pointer++ = ig;
            *pix_pointer++ = ib;
            pix_pointer += fbg->comp_offset;
        }
    }
}

void fbg_imageClip(struct _fbg *fbg, struct _fbg_img *img, int x, int y, int cx, int cy, int cw, int ch) {
    unsigned char *pix_pointer = (unsigned char *)(fbg->back_buffer + (y * fbg->line_length + x * fbg->components));
    unsigned char *img_pointer = (unsigned char *)(img->data + (cy * img->width * fbg->components));

    img_pointer += cx * fbg->components;

    int i = 0;
    int w3 = _FBG_MIN((cw - cx) * fbg->components, (fbg->width - x) * fbg->components);
    int h = ch - cy;

    for (i = 0; i < h; i += 1) {
        memcpy(pix_pointer, img_pointer, w3);
        pix_pointer += fbg->line_length;
        img_pointer += img->width * fbg->components;
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

void fbg_imageEx(struct _fbg *fbg, struct _fbg_img *img, int x, int y, float sx, float sy, int cx, int cy, int cw, int ch) {
    float x_ratio_inv = 1.0f / sx;
    float y_ratio_inv = 1.0f / sy;

    int px, py;
    int cx2 = (float)cx * sx;
    int cy2 = (float)cy * sy;
    int w2 = (float)(cw + cx) * sx;
    int h2 = (float)(ch + cy) * sy;
    int i, j;

    int d = w2 - cx2;

    if (d >= (fbg->width - x)) {
        w2 -= (d - (fbg->width - x));
    }

    unsigned char *pix_pointer = (unsigned char *)(fbg->back_buffer + (y * fbg->line_length + x * fbg->components));

    for (i = cy2; i < h2; i += 1) {
        py = floorf(x_ratio_inv * (float)i);

        for (j = cx2; j < w2; j += 1) {
            px = floorf(y_ratio_inv * (float)j);
            
            unsigned char *img_pointer = (unsigned char *)(img->data + ((px + py * img->width) * fbg->components));

            memcpy(pix_pointer, img_pointer, fbg->components);

            pix_pointer += fbg->components;
        }

        pix_pointer += fbg->line_length - (w2 - cx2) * fbg->components;
    }
}

void fbg_freeImage(struct _fbg_img *img) {
    free(img->data);

    free(img);
}

void fbg_drawInto(struct _fbg *fbg, unsigned char *buffer) {
    if (buffer == NULL) {
        fbg->back_buffer = fbg->temp_buffer;
        fbg->temp_buffer = NULL;
    } else {
        fbg->temp_buffer = fbg->back_buffer;
        fbg->back_buffer = buffer;
    }
}

float fbg_randf(float a, float b) {
    float random = ((float) rand()) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

unsigned char *fbg_getBackBuffer(struct _fbg *fbg) {
    return fbg->back_buffer;
}

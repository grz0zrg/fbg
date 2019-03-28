/*
    example of parallelism without libraries / fbg builtin parallelism

    sometimes fbg parallelism may be uneeded; this is the case when you don't want to mix stuff and just want to push some pixels!
    
    this example split the screen vertically in 4 parts handled by one thread each, it set pixels color / parts (a clear)

    the only synchronization mechanism is a pthread barrier to wait threads result before being sent to GPU

    run at 13 fps @ 1080p on Raspberry PI 3B with OpenGL ES 2 backend
    run at 20 fps @ 1080p on Raspberry PI 3B with dispmanx backend

    without barrier it run at 50 fps @ 1080p on Raspberry PI 3B with dispmanx backend
    note : for each threads except the main thread which run at 20 fps due to its additional task of CPU -> GPU transfer

    run at 60 fps @ 720p on Raspberry PI 3B with dispmanx backend
*/

#include <stdatomic.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "dispmanx/fbg_dispmanx.h"

atomic_int keep_running = 1;

struct _fbg *fbg;

static pthread_barrier_t sync_barrier;

void int_handler(int dummy) {
    keep_running = 0;
}

void compute(int id) {
    int n = 4;// threads

    int xx = 0, yy = 0, w3 = fbg->width * fbg->components;
    int x = 0;
    int y = fbg->height / 4 * id;

    char *pix_pointer = (char *)(fbg->back_buffer + (y * fbg->line_length + x * fbg->components));

    for (yy = 0; yy < fbg->height / n; yy += 1) {
        for (xx = 0; xx < fbg->width; xx += 1) {
            *pix_pointer++ = id == 1 ? 255 : 0;
            *pix_pointer++ = id == 2 ? 255 : 0;
            *pix_pointer++ = id == 3 ? 255 : 0;
            pix_pointer += fbg->comp_offset;
        }

        pix_pointer += (fbg->line_length - w3);
    }
}

void *process(void *t) {
    int id = *((int *)t);

    int frame;
    int print_fps = 0;

    struct timeval fps_start;
    struct timeval fps_stop;
    gettimeofday(&fps_stop, NULL);

    struct _fbg *my_fbg = fbg;

    while (keep_running) {
        compute(id);

        // sync
        pthread_barrier_wait(&sync_barrier);

        // framerate
        gettimeofday(&fps_stop, NULL);
        double ms = (fps_stop.tv_sec - fps_start.tv_sec) * 1000000.0 - (fps_stop.tv_usec - fps_start.tv_usec);
        if (ms >= 1000.0) {
            gettimeofday(&fps_start, NULL);

            print_fps += 1;
            if ((print_fps % 5) == 0) {
                printf("%i: %lu fps\n", id, (long unsigned int)frame);
                fflush(stdout);
            }

            frame = 0;
        }

        frame += 1;
    }
}

int main(int argc, char* argv[]) {
    fbg = fbg_dispmanxSetup(0);
    if (fbg == NULL) {
        return 0;
    }

    struct _fbg_img *bb_font_img = fbg_loadPNG(fbg, "../examples/bbmode1_8x8.png");

    struct _fbg_font *bbfont = fbg_createFont(fbg, bb_font_img, 8, 8, 33);

    signal(SIGINT, int_handler);

    // threads setup
    pthread_t threads[3];
    int t1 = 1;
    pthread_create(&threads[0], NULL, process, &t1);
    int t2 = 2;
    pthread_create(&threads[1], NULL, process, &t2);
    int t3 = 3;
    pthread_create(&threads[2], NULL, process, &t3);

    pthread_barrier_init(&sync_barrier, NULL, 4);

    do {
        //fbg_clear(fbg, 0);
        fbg_draw(fbg);

        compute(0);

        fbg_write(fbg, fbg->fps_char, 2, 2);

        pthread_barrier_wait(&sync_barrier);

        fbg_flip(fbg);
    } while (keep_running);

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);

    pthread_barrier_destroy(&sync_barrier);

    fbg_freeImage(bb_font_img);
    fbg_freeFont(bbfont);

    fbg_close(fbg);
}
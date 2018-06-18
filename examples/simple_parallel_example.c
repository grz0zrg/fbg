#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>

#include "fbgraphics.h"

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

// user data example
struct _fragment_user_data {
    float offset_x;
};

void *fragmentStart(struct _fbg *fbg) {
    struct _fragment_user_data *user_data = (struct _fragment_user_data *)calloc(1, sizeof(struct _fragment_user_data));

    user_data->offset_x = fbg->task_id * 2.0f;

    return user_data;
}

void fragment(struct _fbg *fbg, struct _fragment_user_data *user_data) {
    float offset_x = abs(sin(user_data->offset_x * 8.f)) * 8.f;

    // this function will be executed by each threads
    // you are free to call any FBG graphics primitive here
    
    fbg_clear(fbg, 0);
    
    // you are also free to fill each threads back buffer the way you want to
    // fbg->task_id : thread identifier (starting at 1, 0 is reserved for the main thread)
    // each threads will draw an horizontal line, the shade of the blue color will change based on the thread it is drawn from
    int x = 0, y = 0;
    for (y = fbg->task_id; y < fbg->height; y += (fbg->parallel_tasks + 1)) {
        for (x = 0; x < fbg->width; x += 1) {
            int i = (x + y * fbg->width) * 3;
            fbg->back_buffer[i] = fbg->task_id * 85; // note : BGR format
            fbg->back_buffer[i + 1] = 0;
            fbg->back_buffer[i + 2] = 0;
        }
    }
    
    // simple graphics primitive (4 blue rectangle which will be handled by different threads)
    fbg_rect(fbg, fbg->width / 2 - 32 + fbg->task_id * 32 + offset_x, 0, 32, 32, 255, 0, 0);

    user_data->offset_x += 0.01f;
}

void fragmentStop(struct _fbg *fbg, struct _fragment_user_data *data) {
    free(data);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, int_handler);

    struct _fbg *fbg = fbg_init();
    if (fbg == NULL) {
        return 0;
    }

    struct _fbg_img *bb_font_img = fbg_loadPNG(fbg, "bbmode1_8x8.png");

    struct _fbg_font *bbfont = fbg_createFont(fbg, bb_font_img, 8, 8, 33);

    fbg_createFragment(fbg, fragmentStart, fragment, fragmentStop, 3, 63);

    // we will make a call to fragment function in our main drawing loop here so we
    // also need to call fragmentStart and fragmentStop for this thread
    struct _fragment_user_data *user_data = fragmentStart(fbg);

    int i = 0;
    do {
        fbg_clear(fbg, 0);

        fragment(fbg, user_data);
        fbg_draw(fbg, 1, NULL);

        // we use a utility function to draw the framerate of each cores (including main app #0)
        fbg_write(fbg, "FPS:\n#0:\n#1:\n#2:\n#3:", 4, 2);
        for (i = 0; i <= fbg->parallel_tasks; i += 1) {
            fbg_drawFramerate(fbg, bbfont, i, 4 + 8 + 8 + 8 + 8, 2 + 8 + 8 * i, 255, 255, 255);
        }

        fbg_flip(fbg);

    } while (keep_running);

    fragmentStop(fbg, user_data);

    fbg_freeImage(bb_font_img);
    fbg_freeFont(bbfont);

    fbg_close(fbg);

    return 0;
}

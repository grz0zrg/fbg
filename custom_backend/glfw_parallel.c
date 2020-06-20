#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>

#include "glfw/fbg_glfw.h"

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

// user data example
struct _fragment_user_data {
    float offset_x;
    float offset_y;
    float velx;
    float vely;
};

void *fragmentStart(struct _fbg *fbg) {
    struct _fragment_user_data *user_data = (struct _fragment_user_data *)calloc(1, sizeof(struct _fragment_user_data));

    user_data->offset_x = fbg->task_id * 32.0f;
    user_data->offset_y = fbg->task_id * 32.0f;

    float signx = 1;
    float signy = 1;

    if (fbg_randf(0, 1) > 0.5) {
        signx = -1;
    }

    if (fbg_randf(0, 1) > 0.5) {
        signy = -1;
    }

    user_data->velx = fbg_randf(4, 8) * signx;
    user_data->vely = fbg_randf(4, 8) * signy;

    return user_data;
}

void fragment(struct _fbg *fbg, void *user_data) {
    struct _fragment_user_data *ud = (struct _fragment_user_data *)user_data;

    float c = (float)fbg->task_id / fbg->parallel_tasks * 255;

    fbg_recta(fbg,
        ud->offset_x,
        ud->offset_y, 32, 32,
        c,
        255 - c,
        abs(128 - c),
        fbg_randf(0, 255));

    fbg_recta(fbg,
        fbg->width - ud->offset_x,
        fbg->height - ud->offset_y, 32, 32,
        c,
        255 - c,
        abs(128 - c),
        fbg_randf(0, 255));
        
    fbg_recta(fbg,
        fbg->width - ud->offset_x,
        ud->offset_y, 32, 32,
        c,
        255 - c,
        abs(128 - c),
        fbg_randf(0, 255));

    fbg_recta(fbg,
        ud->offset_x,
        fbg->height - ud->offset_y, 32, 32,
        c,
        255 - c,
        abs(128 - c),
        fbg_randf(0, 255));

    ud->offset_x += ud->velx;
    ud->offset_y += ud->vely;

    if (ud->offset_x <= 32) {
        ud->velx = -ud->velx;
        ud->offset_x = 32;
    } else if (ud->offset_x > fbg->width - 32) {
        ud->velx = -ud->velx;
        ud->offset_x = fbg->width - 32;
    }

    if (ud->offset_y <= 32) {
        ud->vely = -ud->vely;
        ud->offset_y = 32;
    } else if (ud->offset_y > fbg->height - 32) {
        ud->vely = -ud->vely;
        ud->offset_y = fbg->height - 32;
    }
}

void fragmentStop(struct _fbg *fbg, void *data) {
    struct _fragment_user_data *ud = (struct _fragment_user_data *)data;

    free(ud);
}

void fbg_XORMixing(struct _fbg *fbg, unsigned char *buffer, int task_id) {
    for (int j = 0; j < fbg->size; j += 1) {
        fbg->back_buffer[j] = fbg->back_buffer[j] ^ buffer[j];
    }
}

int main(int argc, char* argv[]) {
    struct _fbg *fbg = fbg_glfwSetup(800, 600, 3, "glfw example", 0, 0);
    if (fbg == NULL) {
        return 0;
    }

    struct _fbg_img *bb_font_img = fbg_loadPNG(fbg, "../examples/bbmode1_8x8.png");

    struct _fbg_font *bbfont = fbg_createFont(fbg, bb_font_img, 8, 8, 33);

    fbg_createFragment(fbg, fragmentStart, fragment, fragmentStop, 7);

    srand(time(NULL));

    signal(SIGINT, int_handler);

    do {
        fbg_glfwClear();
        
        fbg_clear(fbg, 0);
    
        for (int j = 0; j < fbg->parallel_tasks; j += 1) {
            fbg_write(fbg, fbg->fps_char, 2, 2 + j * 10);
        }

        fbg_draw(fbg, fbg_XORMixing);

        fbg_flip(fbg);
    } while (keep_running && !fbg_glfwShouldClose(fbg));

    fbg_freeImage(bb_font_img);
    fbg_freeFont(bbfont);

    fbg_close(fbg);
}
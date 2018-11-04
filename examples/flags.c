#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>

#include "fbgraphics.h"

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

struct _fbg_img *flags_texture;

struct _fragment_user_data {
    float xmotion;
    float ymotion;
};

void *fragmentStart(struct _fbg *fbg) {
    struct _fragment_user_data *user_data = (struct _fragment_user_data *)calloc(1, sizeof(struct _fragment_user_data));

    user_data->xmotion = 0.0f;
    user_data->ymotion = 0.0f;

    return user_data;
}

void fragmentStop(struct _fbg *fbg, struct _fragment_user_data *data) {
    free(data);
}

void fragment(struct _fbg *fbg, struct _fragment_user_data *user_data) {
    fbg_clear(fbg, 0);
    
    // https://www.openprocessing.org/sketch/560394

    int xoff = 64;
    int yoff = 128;

    int rect_size = 1;
    float x_step_size = 2;

    float x_end = fbg->width - xoff;
    float y_end = fbg->height - yoff;

    for (int y = fbg->task_id + 1; y < y_end; y += (fbg->parallel_tasks + 1)) {
        float ys = (y_end - yoff);
        float yo = (y - yoff);
        float yy = yo / ys;
        int yyd = ((int)(yy * flags_texture->height) % flags_texture->height) * flags_texture->width;
        float yyy = yy - 0.5;
        
        for (int x = xoff; x < x_end; x += x_step_size) {
            float xs = (x_end - xoff);
            float xo = (x - xoff);
            float xx = xo / xs;
            
            float xxd = xx * flags_texture->width;
            
            int cl = (int)(xxd + yyd) * fbg->components;
            
            int r = flags_texture->data[cl];
            int g = flags_texture->data[cl + 1];
            int b = flags_texture->data[cl + 2];
            
            float xxx = xx - 0.5;

            fbg_rect(fbg, _FBG_MAX(_FBG_MIN(x + cosf(xxx*yyy * M_PI * 4 + user_data->xmotion) * 24, fbg->width - rect_size), 0), _FBG_MAX(_FBG_MIN(y + yoff / 2 + sinf(xxx*yyy * M_PI * 7 + user_data->ymotion) * 24, fbg->height - rect_size), 0), rect_size, rect_size, r, g, b);
        }
    }
    
    user_data->xmotion += 0.0075;
    user_data->ymotion += 0.05;
}

void selectiveMixing(struct _fbg *fbg, unsigned char *buffer, int task_id) {
    int j = 0;
    for (j = 0; j < fbg->size; j += 1) {
        fbg->back_buffer[j] = (fbg->back_buffer[j] > buffer[j]) ? fbg->back_buffer[j] : buffer[j];
    }
}

int main(int argc, char* argv[]) {
    struct _fbg *fbg = fbg_init();
    if (fbg == NULL) {
        return 0;
    }

    signal(SIGINT, int_handler);

    flags_texture = fbg_loadPNG(fbg, "flags.png");
    struct _fbg_img *bbimg = fbg_loadPNG(fbg, "bbmode1_8x8.png");

    struct _fbg_font *bbfont = fbg_createFont(fbg, bbimg, 8, 8, 33);

    fbg_createFragment(fbg, fragmentStart, fragment, fragmentStop, 3);

    struct _fragment_user_data *user_data = fragmentStart(fbg);

    do {
        fragment(fbg, user_data);

        fbg_draw(fbg, 1, selectiveMixing);

        fbg_write(fbg, "FBGraphics: Flags of the world", 4, 2);

        fbg_write(fbg, "FPS", 4, 12+8);
        fbg_write(fbg, "#0 (Main): ", 4, 22+8);
        fbg_write(fbg, "#1: ", 4, 32+8);
        fbg_write(fbg, "#2: ", 4, 32+8+2+8);
        fbg_write(fbg, "#3: ", 4, 32+16+4+8);
        fbg_drawFramerate(fbg, NULL, 0, 4 + 32 + 48 + 8, 22 + 8, 255, 255, 255);
        fbg_drawFramerate(fbg, NULL, 1, 4+32, 32+8, 255, 255, 255);
        fbg_drawFramerate(fbg, NULL, 2, 4+32, 32+8+2+8, 255, 255, 255);
        fbg_drawFramerate(fbg, NULL, 3, 4+32, 32+16+4+8, 255, 255, 255);

        fbg_flip(fbg);
    } while (keep_running);

    fragmentStop(fbg, user_data);

    fbg_close(fbg);

    fbg_freeImage(flags_texture);
    fbg_freeImage(bbimg);
    fbg_freeFont(bbfont);

    return 0;
}

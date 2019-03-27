#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>

#include "fbgraphics.h"

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

// utility function to draw a rectangle with borders
void border_rectangle(struct _fbg *fbg, int x, int y, int w, int h, int br, int bg, int bb, int r, int g, int b) {
    // set the fill color for frect function
    fbg_fill(fbg, r, g, b);

    fbg_hline(fbg, x - 1, y - 1, w + 2, br, bg, bb);
    fbg_vline(fbg, x - 1, y, h, br, bg, bb);
    fbg_hline(fbg, x - 1, y + h, w + 2, br, bg, bb);
    fbg_vline(fbg, x + w, y, h, br, bg, bb);
    fbg_frect(fbg, x, y, w, h);
}

void fragment(struct _fbg *fbg, struct _fragment_user_data *data) {
    fbg_clear(fbg, 0);

    if (fbg->task_id == 1) {
        // rectangle for the framerate infos
        fbg_rect(fbg, 2, 2, 8 * 24 - 4, 8 * 8 - 6, 32, 32, 32);
    } else if (fbg->task_id == 2) {
        // draw a rectangle with colored borders
        border_rectangle(fbg, 16, 16, fbg->width - 16 * 2, fbg->height - 16 * 2, 0, 0, 255, 8, 8, 8);

        // draw a diagonal line
        fbg_line(fbg, 16, 16, fbg->width - 16, fbg->height - 16, 0, 0, 255);

        // draw a polygon
        int vertices[6] = {100, 100, 200, 100, 100, 200};
        fbg_polygon(fbg, 3, &vertices[0], 255, 0, 0);
    }
}

void alphaBlending(struct _fbg *fbg, unsigned char *buffer, int task_id) {
    int alpha = 164; // the layers alpha blend amount

    if (task_id == 1) {
        alpha = 128;
    }

    int j = 0;
    for (j = 0; j < fbg->size; j += 1) {
        fbg->back_buffer[j] = (buffer[j] > 0) ? ((alpha * buffer[j] + (255 - alpha) * fbg->back_buffer[j]) >> 8) : fbg->back_buffer[j];
    }
}

int main(int argc, char* argv[]) {
    struct _fbg *fbg = fbg_init();
    if (fbg == NULL) {
        return 0;
    }

    signal(SIGINT, int_handler);

    struct _fbg_img *texture = fbg_loadPNG(fbg, "texture.png");
    struct _fbg_img *bbimg = fbg_loadPNG(fbg, "bbmode1_8x8.png");

    struct _fbg_font *bbfont = fbg_createFont(fbg, bbimg, 8, 8, 33);

    fbg_createFragment(fbg, NULL, fragment, NULL, 2);

    do {
        fbg_clear(fbg, 0);

        fbg_imageClip(fbg, texture, 0, 0, 0, 0, _FBG_MIN(fbg->width, texture->width), _FBG_MIN(fbg->height, texture->height));

        // we draw with alpha blending function to mix our layer with the background
        fbg_draw(fbg, alphaBlending);

        // draw fps
        fbg_write(fbg, "FBGraphics: Compositing", 4, 2);

        fbg_write(fbg, "FPS", 4, 12+8);
        fbg_write(fbg, "#0 (Main): ", 4, 22+8);
        fbg_write(fbg, "#1: ", 4, 32+8);
        fbg_write(fbg, "#2: ", 4, 32+8+8);
        fbg_drawFramerate(fbg, NULL, 0, 4 + 32 + 48 + 8, 22 + 8, 255, 255, 255);
        fbg_drawFramerate(fbg, NULL, 1, 4+32, 32+8, 255, 255, 255);
        fbg_drawFramerate(fbg, NULL, 2, 4+32, 32+8+8, 255, 255, 255);

        fbg_flip(fbg);
    } while (keep_running);

    fbg_close(fbg);

    fbg_freeImage(texture);
    fbg_freeImage(bbimg);
    fbg_freeFont(bbfont);

    return 0;
}

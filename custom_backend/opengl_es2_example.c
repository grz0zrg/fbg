#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>

#include "opengl_es2/fbg_opengl_es2.h"

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

int main(int argc, char* argv[]) {
    // fbdev version
#ifdef FBG_FBDEV
    struct _fbg *fbg = fbg_gles2Setup("/dev/fb0", 3);
#else
    // rpi version
    struct _fbg *fbg = fbg_gles2Setup(3);
#endif
    if (fbg == NULL) {
        return 0;
    }

    struct _fbg_img *bb_font_img = fbg_loadPNG(fbg, "../examples/bbmode1_8x8.png");

    struct _fbg_font *bbfont = fbg_createFont(fbg, bb_font_img, 8, 8, 33);

    float x = 0, y = 0, velx = 3.4, vely = 3.425;

    signal(SIGINT, int_handler);

    do {
        fbg_gles2Clear();
        
        fbg_clear(fbg, 128);
        

        fbg_rect(fbg, x, y, 40, 40, 255, 0, 0);

        fbg_write(fbg, fbg->fps_char, 2, 2);

        fbg_draw(fbg);
        
        fbg_flip(fbg);

        x += velx;
        y += vely;

        if (x <= 0 || x > fbg->width - 40) {
            velx = -velx;
        }

        if (y <= 0 || y > fbg->height - 40) {
            vely = -vely;
        }
    } while (keep_running);

    fbg_freeImage(bb_font_img);
    fbg_freeFont(bbfont);

    fbg_close(fbg);
}
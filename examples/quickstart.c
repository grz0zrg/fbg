#include <sys/stat.h>
#include <signal.h>

#include "fbgraphics.h"
#include "fbg_fbdev.h" // insert any backends from ../custom_backend/backend_name folder

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, int_handler);

    // open "/dev/fb0" by default, use fbg_fbdevSetup("/dev/fb1", 0) if you want to use another framebuffer
    // note : fbg_fbdevInit is the linux framebuffer backend, you can use a different backend easily by including the proper header and compiling with the appropriate backend file found in ../custom_backend/backend_name
    struct _fbg *fbg = fbg_fbdevInit();
    if (fbg == NULL) {
        return 0;
    }
    
    struct _fbg_img *texture = fbg_loadPNG(fbg, "texture.png");
    struct _fbg_img *bb_font_img = fbg_loadPNG(fbg, "bbmode1_8x8.png");

    struct _fbg_font *bbfont = fbg_createFont(fbg, bb_font_img, 8, 8, 33);

    do {
        fbg_clear(fbg, 0); // can also be replaced by fbg_background(fbg, 0, 0, 0);

        fbg_draw(fbg);

        // you can also use fbg_image(fbg, texture, 0, 0)
        // but you must be sure that your image size fit on the display
        fbg_imageClip(fbg, texture, 0, 0, 0, 0, fbg->width, fbg->height);

        fbg_write(fbg, "Quickstart example\nFPS:", 4, 2);
        fbg_write(fbg, fbg->fps_char, 32 + 8, 2 + 8);

        fbg_rect(fbg, fbg->width / 2 - 32, fbg->height / 2 - 32, 16, 16, 0, 255, 0);

        fbg_pixel(fbg, fbg->width / 2, fbg->height / 2, 255, 0, 0);

        fbg_flip(fbg);

    } while (keep_running);

    fbg_freeImage(texture);
    fbg_freeImage(bb_font_img);
    fbg_freeFont(bbfont);

    fbg_close(fbg);

    return 0;
}

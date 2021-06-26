#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>

#include "glfw/fbg_glfw.h"

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

int main(int argc, char* argv[]) {
    struct _fbg *fbg = fbg_glfwSetup(800, 600, 4, "glfw example", 0, 0, 1);
    if (fbg == NULL) {
        return 0;
    }

    // struct _fbg_glfw_context *glfw_context = fbg->user_context;
    // note: when SSAA argument of fbg_glfwSetup is set higher than 1 fbg->width may return more than the display resolution due to this upscale factor
    //       so if you need to refer to the real display width / height you must then use glfw_context->width / glfw_context->height
    //       and use fbg->width / fbg->height when you refer to the fbg drawing context

    float x = 0, y = 0, velx = 3.4, vely = 3.425;

    signal(SIGINT, int_handler);

    do {
        fbg_glfwClear();
        
        fbg_clear(fbg, 0);

        fbg_rect(fbg, x, y, 40, 40, 255, 0, 0);

        fbg_line(fbg, 0, 0, fbg->width - 1, fbg->height-1, 255, 255, 255);
        int vertices[6] = {100, 100, 200, 100, 100, 200};
        fbg_polygon(fbg, 3, &vertices[0], 255, 0, 0);

        fbg_draw(fbg);
        fbg_flip(fbg);

        x += velx;
        y += vely;

        if (x <= 0) {
            x = 0;
            velx = -velx;
        } else if (x > fbg->width - 40) {
            x = fbg->width - 40;
            velx = -velx;
        }

        if (y <= 0) {
            y = 0;
            vely = -vely;
        } else if (y > fbg->height - 40) {
            y = fbg->height - 40;
            vely = -vely;
        }
    } while (keep_running && !fbg_glfwShouldClose(fbg));

    fbg_close(fbg);
}

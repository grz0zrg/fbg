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
    struct _fbg *fbg = fbg_glfwSetup(800, 600, "glfw example", 0, 0);
    if (fbg == NULL) {
        return 0;
    }

    float x = 0, y = 0, velx = 3.4, vely = 3.425;

    signal(SIGINT, int_handler);

    do {
        fbg_glfwClear();
        
        fbg_clear(fbg, 0);
        fbg_draw(fbg);

        fbg_rect(fbg, x, y, 40, 40, 255, 0, 0);

        fbg_flip(fbg);

        x += velx;
        y += vely;

        if (x <= 0 || x > fbg->width - 40) {
            velx = -velx;
        }

        if (y <= 0 || y > fbg->height - 40) {
            vely = -vely;
        }
    } while (keep_running && !fbg_glfwShouldClose(fbg));

    fbg_close(fbg);
}
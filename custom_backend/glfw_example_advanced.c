#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>

#include "glfw/fbg_glfw.h"

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

float randf(float a, float b) {
    float random = ((float) rand()) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

int main(int argc, char* argv[]) {
    struct _fbg *fbg = fbg_glfwSetup(800, 600, "glfw example", 0, 0);
    if (fbg == NULL) {
        return 0;
    }

    fbg_glfwUpdateBuffer(fbg, 1);

    struct _fbg_img *img = fbg_createImage(fbg, fbg->width, fbg->height);

    signal(SIGINT, int_handler);

    int pcount = 1024 * 4;

    float points[pcount];

    for (int i = 0; i < pcount; i += 4) {
        points[i] = randf(0, fbg->width - 1);
        points[i + 1] = randf(0, fbg->height - 1);

        float vx = randf(-4, 4);
        float vy = randf(-4, 4);

        points[i + 2] = vx;
        points[i + 3] = vy;

        if (vx < 0.25 && vx > -0.25) {
            points[i + 2] = randf(1, 4);
        }

        if (vy < 0.25 && vy > -0.25) {
            points[i + 3] = randf(1, 4);
        }
    }

    float motion = 0;

    srand((unsigned int)time(NULL));

    struct _fbg_rgb color;

    do {
        //fbg_clear(fbg, 0);
        fbg_draw(fbg);

        fbg_image(fbg, img, 0, 0);

        int c = 0;
        for (int i = 0; i < pcount; i += 4) {
            float x = points[i];
            float y = points[i + 1];
            float vx = points[i + 2];
            float vy = points[i + 3];

            fbg_hslToRGB(&color, abs(sin(x / (float)fbg->width + motion)), 0.5, 0.5);

            fbg_rect(fbg, x, y, 1, 1, color.r, color.g, color.b);

            points[i] += vx;
            points[i + 1] += vy;

            x = points[i];
            y = points[i + 1];

            if (x <= 0 || x > fbg->width - 1) {
                points[i + 2] = -vx;
            }

            if (y <= 0 || y > fbg->height - 1) {
                points[i + 3] = -vy;
            }

            c += 1;
        }

        fbg_flip(fbg);

        fbg_drawInto(fbg, img->data);

        motion += 0.5f;
    } while (keep_running && !fbg_glfwShouldClose(fbg));

    fbg_freeImage(img);

    fbg_close(fbg);
}
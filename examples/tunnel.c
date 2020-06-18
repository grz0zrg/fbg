#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>

#include "fbg_fbdev.h"

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

struct _fbg_img *tunnel_texture;

struct _fragment_user_data {
    float xmotion;
    float ymotion;
    float rmotion;
};

void *fragmentStart(struct _fbg *fbg) {
    struct _fragment_user_data *user_data = (struct _fragment_user_data *)calloc(1, sizeof(struct _fragment_user_data));

    user_data->xmotion = 0.0f;
    user_data->ymotion = 0.0f;
    user_data->rmotion = 0.0f;

    return user_data;
}

void fragmentStop(struct _fbg *fbg, struct _fragment_user_data *data) {
    free(data);
}

void fragment(struct _fbg *fbg, struct _fragment_user_data *user_data) {
    if (fbg->task_id > 0) {
        fbg_clear(fbg, 0);
    }
    
    // https://www.openprocessing.org/sketch/560781

    int rect_size = 2;

    int elems = 240;
    int dots = 170;

    int bsize = 12;
    
    int xoff = fbg->width / 2;
    int yoff = fbg->height / 2;
    
    int xrad_step = 1;
    int yrad_step = 1;
    
    int xdeform = 34;
    int ydeform = 34;
    
    float xrmotion_size = 8;
    float yrmotion_size = 8;
    
    for (int e = 0; e < elems; e += 1) {
        float de = (float)e / elems;
        
        float bd = fminf(de * 4, 1.);
        
        float ex = sin(de * 360.0f * (M_PI / 180.0f) + user_data->xmotion);
        float ey = cos(de * 360.0f * (M_PI / 180.0f) + user_data->ymotion);
        
        float xrad = e * xrad_step + bsize + sin(de * 360.0f * (M_PI / 180.0f) + user_data->xmotion) * xrmotion_size;
        float yrad = e * yrad_step + bsize + cos(de * 360.0f * (M_PI / 180.0f) + user_data->xmotion) * yrmotion_size;
        
        float final_ex = xoff + ex * xdeform;
        
        float xpp = (de * 360.0f * (M_PI / 180.0f)) / 2;

        float fy = yoff + ey * ydeform;

        int xxd = ((int)(de * tunnel_texture->width));
        
        for (int d = fbg->task_id; d < dots; d += (fbg->parallel_tasks + 1)) {
            float dd = (float)d / dots;
            
            float xp = dd * 360.0f * (M_PI / 180.0f);
            float yp = xp;
            
            xp += xpp + user_data->rmotion;
            yp += xpp + user_data->rmotion;
            
            float final_x = final_ex + sin(xp) * xrad;
            float final_y = fy + cos(yp) * yrad;
            
            if (final_x >= (fbg->width-rect_size) || final_x <= rect_size || final_y >= (fbg->height-rect_size) | final_y <= rect_size) {
                continue;
            }
            
            // apply colormap / texture
            int yyd = (((int)(dd * tunnel_texture->height))) * tunnel_texture->width;
        
            int cl = (int)(xxd + yyd) * fbg->components;
            
            int r = (int)((float)tunnel_texture->data[cl] * bd);
            int g = (int)((float)tunnel_texture->data[cl + 1] * bd);
            int b = (int)((float)tunnel_texture->data[cl + 2] * bd);
            
            fbg_rect(fbg, final_x, final_y, rect_size, rect_size, r, g, b);
        }
    }
    
    user_data->xmotion += 0.011;
    user_data->ymotion += 0.008;
    user_data->rmotion += 0.006;
}

void selectiveMixing(struct _fbg *fbg, unsigned char *buffer, int task_id) {
    int j = 0;
    for (j = 0; j < fbg->size; j += 1) {
        fbg->back_buffer[j] = (fbg->back_buffer[j] > buffer[j]) ? fbg->back_buffer[j] : buffer[j];
    }
}

int main(int argc, char* argv[]) {
    struct _fbg *fbg = fbg_fbdevInit();
    if (fbg == NULL) {
        return 0;
    }

    signal(SIGINT, int_handler);

    tunnel_texture = fbg_loadPNG(fbg, "tunnel.png");
    struct _fbg_img *bbimg = fbg_loadPNG(fbg, "bbmode1_8x8.png");

    struct _fbg_font *bbfont = fbg_createFont(fbg, bbimg, 8, 8, 33);

    fbg_createFragment(fbg, fragmentStart, fragment, fragmentStop, 3);

    struct _fragment_user_data *user_data = fragmentStart(fbg);

    do {
        fbg_fadeDown(fbg, 16);

        fragment(fbg, user_data);

        fbg_draw(fbg, selectiveMixing);

        fbg_rect(fbg, 0, 2, 8 * 19, 8 * 9 - 4, 0, 0, 0);
        fbg_write(fbg, "FBGraphics: Tunnel", 4, 2);

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

    fbg_freeImage(tunnel_texture);
    fbg_freeImage(bbimg);
    fbg_freeFont(bbfont);

    return 0;
}

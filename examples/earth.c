#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>

#include "fbgraphics.h"

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

struct _fbg_img *earth_texture;
struct _fbg_img *earth_night_texture;
struct _fbg_img *earth_text_texture;
struct _fbg_img *earth_cycle_texture;

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
    if (fbg->task_id > 0) {
        fbg_clear(fbg, 0);
    }
    
    // https://www.openprocessing.org/sketch/561193
    int dots_step = 1;

    int elems = 300;
    int rect_size = 1;

    float xoff = fbg->width / 2.0f;
    
    float sphere_radius = fbg->height / 2.5;
    float yoff = fbg->height / 2.0f - sphere_radius;

    for (int s = fbg->task_id; s < elems; s += (fbg->parallel_tasks + 1)) {
        float de = (float)s / elems;
        
        float bd = fminf(fmaxf((0.5f-fabsf(de - 0.5f)) * 4.0f, 0.05f), 1.0f);
        
        float yrepeat = 1;
        int yyd = (((int)(de * (earth_texture->height * yrepeat))%earth_texture->height)) * earth_texture->width;
        
        float tyrepeat = 4.0f;
        int yyd2 = (((int)(de * (earth_text_texture->height * tyrepeat) + user_data->xmotion)%earth_text_texture->height)) * earth_text_texture->width;
        
        int yydc = (((int)(de * (earth_cycle_texture->height * yrepeat))%earth_cycle_texture->height)) * earth_cycle_texture->width;
        
        // cartesian circle equation
        float sphere_radius_squared = sphere_radius * sphere_radius;
        float sh_radius = de * (sphere_radius * 2) - sphere_radius;
        float xc_off = sqrtf(sphere_radius_squared - sh_radius*sh_radius);
        
        float final_x;
        float final_y = yoff + de * (sphere_radius * 2); // flabby planet bonus : + sin((de * 360 + 180) * (PI / 180) + user_data->ymotion / 24.) * 4;
        
        for (int e = 0; e <= (int)xc_off / 2; e += dots_step) {
            float dd = (float)e / xc_off;

            rect_size = 1 + (1.0f - dd) * 4;

            // / (dd+0.5) : shrinking near the center (otherwise : equal distribution of the dots)
            final_x = xoff - (xc_off) + (xc_off / (dd+0.5f)) * fabsf(0.5f-(dd+0.5f)) * 2 ;
            
            float yc_off;
            
            // compute UV texels for the colormap
            float dd2 = 1.0f - dd;
                
            yc_off = 1.0f-sqrtf(1.0f - dd2*dd2);
            
            // apply colormaps
            float xrepeat = 0.5f;
            int xxd = ((earth_texture->width - 1) - (int)(yc_off * (earth_texture->width * xrepeat) + user_data->xmotion)%earth_texture->width);
        
            int cl = (int)(xxd + yyd) * fbg->components;
            
            int r = (int)((float)earth_texture->data[cl] * bd);
            int g = (int)((float)earth_texture->data[cl + 1] * bd);
            int b = (int)((float)earth_texture->data[cl + 2] * bd);
            
            // second texture (text mask)
            float txrepeat = 1.;
            
            int xxd2 = fmaxf(0, ((earth_text_texture->width - 1) - (int)(yc_off * (earth_text_texture->width * txrepeat) + user_data->xmotion)%earth_text_texture->width));
        
            int cl2 = (int)(xxd2 + yyd2) * fbg->components;
            
            int r2 = (int)((float)earth_text_texture->data[cl2] * bd);
            int g2 = (int)((float)earth_text_texture->data[cl2 + 1] * bd);
            int b2 = (int)((float)earth_text_texture->data[cl2 + 2] * bd);
            
            // third texture (night version), reuse the first texture UV since they are the same width/height
            int rn = (int)((float)earth_night_texture->data[cl]);
            int gn = (int)((float)earth_night_texture->data[cl + 1]);
            int bn = (int)((float)earth_night_texture->data[cl + 2]);
            
            // fourth texture (cycle mask), only use single component since it is black/white
            int xxdc = ((earth_cycle_texture->width - 1) - (int)(yc_off * (earth_cycle_texture->width * xrepeat) + user_data->xmotion * 4)%earth_cycle_texture->width);
            int clc = (int)(xxdc + yydc) * fbg->components;
            float rc = (float)earth_cycle_texture->data[clc] / 255.0f;
            
            float final_r = r * rc + rn * (1.0f - rc);
            float final_g = g * rc + gn * (1.0f - rc);
            float final_b = b * rc + bn * (1.0f - rc);
            
            // some compositing 
            fbg_rect(fbg, final_x, final_y, rect_size, rect_size, fminf(255, r * (r2 / 255.0f) + final_r), fminf(255, g * (g2 / 255.0f) + final_g), fminf(255, b * (b2 / 255.0f) + final_b));
        }
        
        for (int e = 0; e <= (int)xc_off / 2; e += dots_step) {
            float dd = (float)e / xc_off;

            rect_size = 1 + (1.0f - dd) * 4;

            final_x = xoff - (xc_off) + (xc_off-(xc_off / (dd+0.5)) * fabsf(0.5f-(dd+0.5f))) * 2;
            
            float yc_off;
            
            // compute UV texels for the colormap
            float dd2 = 1.0f - dd;
                
            yc_off = sqrtf(1.0f - dd2*dd2) + 1.270f;
            
            // apply colormaps
            float xrepeat = 0.5;
            int xxd = ((earth_texture->width - 1) - (int)(yc_off * (earth_texture->width * xrepeat) + user_data->xmotion)%earth_texture->width);
        
            int cl = (int)(xxd + yyd) * fbg->components;
            
            int r = (int)((float)earth_texture->data[cl] * bd);
            int g = (int)((float)earth_texture->data[cl + 1] * bd);
            int b = (int)((float)earth_texture->data[cl + 2] * bd);
            
            // second texture (text mask)
            float txrepeat = 1.;
            
            int xxd2 = fmaxf(0, ((earth_text_texture->width - 1) - (int)(yc_off * (earth_text_texture->width * txrepeat) + user_data->xmotion)%earth_text_texture->width));
        
            int cl2 = (int)(xxd2 + yyd2) * fbg->components;
            
            int r2 = (int)((float)earth_text_texture->data[cl2] * bd);
            int g2 = (int)((float)earth_text_texture->data[cl2 + 1] * bd);
            int b2 = (int)((float)earth_text_texture->data[cl2 + 2] * bd);
            
            // third texture (night version), reuse the first texture UV since they are the same width/height
            int rn = (int)((float)earth_night_texture->data[cl]);
            int gn = (int)((float)earth_night_texture->data[cl + 1]);
            int bn = (int)((float)earth_night_texture->data[cl + 2]);
            
            // fourth texture (cycle mask), only use single component since it is black/white
            int xxdc = ((earth_cycle_texture->width - 1) - (int)(yc_off * (earth_cycle_texture->width * xrepeat) + user_data->xmotion * 4)%earth_cycle_texture->width);
            int clc = (int)(xxdc + yydc) * fbg->components;
            float rc = (float)earth_cycle_texture->data[clc] / 255.0f;
            
            float final_r = r * rc + rn * (1.0f - rc);
            float final_g = g * rc + gn * (1.0f - rc);
            float final_b = b * rc + bn * (1.0f - rc);
            
            // some compositing 
            fbg_rect(fbg, final_x, final_y, rect_size, rect_size, fminf(255, r * (r2 / 255.0f) + final_r), fminf(255, g * (g2 / 255.0f) + final_g), fminf(255, b * (b2 / 255.0f) + final_b));
        }
    }
    
    user_data->xmotion += 0.5;
    user_data->ymotion -= 0.25;
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

    earth_texture = fbg_loadPNG(fbg, "earth.png");
    earth_night_texture = fbg_loadPNG(fbg, "earth_night.png");
    earth_cycle_texture = fbg_loadPNG(fbg, "earth_cycle.png");
    earth_text_texture = fbg_loadPNG(fbg, "earth_text.png");

    struct _fbg_img *bbimg = fbg_loadPNG(fbg, "bbmode1_8x8.png");

    struct _fbg_font *bbfont = fbg_createFont(fbg, bbimg, 8, 8, 33);

    fbg_createFragment(fbg, fragmentStart, fragment, fragmentStop, 3);

    struct _fragment_user_data *user_data = fragmentStart(fbg);

    do {
        fbg_fadeDown(fbg, 8);

        fragment(fbg, user_data);

        fbg_draw(fbg, selectiveMixing);

        fbg_rect(fbg, 0, 2, 8 * 19, 8 * 9 - 4, 0, 0, 0);
        fbg_write(fbg, "FBGraphics: Earth", 4, 2);

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

    fbg_freeImage(earth_night_texture);
    fbg_freeImage(earth_text_texture);
    fbg_freeImage(earth_cycle_texture);
    fbg_freeImage(earth_texture);
    fbg_freeImage(bbimg);
    fbg_freeFont(bbfont);

    return 0;
}

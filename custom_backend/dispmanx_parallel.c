#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>

#include "dispmanx/fbg_dispmanx.h"

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

// we can use regular buffers mixing but be aware that it may be very slow at high resolution due to the amount of pixels to be mixed on the main thread
// as such in this example we don't use this mixing function but the accelerated one below
void fbg_XORMixing(struct _fbg *fbg, unsigned char *buffer, int task_id) {
    for (int j = 0; j < fbg->size; j += 1) {
        fbg->back_buffer[j] = fbg->back_buffer[j] ^ buffer[j];
    }
}

// in the following functions we let the mixing be done by dispmanx by sending each buffer data to an assigned layer (see fbg_mixing)
// we use a mask for each threads to select which pixel we want to display
DISPMANX_RESOURCE_HANDLE_T *back_resources;
DISPMANX_RESOURCE_HANDLE_T *front_resources;
DISPMANX_ELEMENT_HANDLE_T *elems;
void setupDispmanxMixing(struct _fbg *fbg) {
    struct _fbg_dispmanx_context *dispmanx_context = (struct _fbg_dispmanx_context *)fbg->user_context;

    back_resources = (DISPMANX_RESOURCE_HANDLE_T *)malloc(sizeof(DISPMANX_RESOURCE_HANDLE_T) * fbg->parallel_tasks);
    front_resources = (DISPMANX_RESOURCE_HANDLE_T *)malloc(sizeof(DISPMANX_RESOURCE_HANDLE_T) * fbg->parallel_tasks);
    elems = (DISPMANX_ELEMENT_HANDLE_T *)malloc(sizeof(DISPMANX_ELEMENT_HANDLE_T) * fbg->parallel_tasks);

    // you can change how layers should be mixed here
    VC_DISPMANX_ALPHA_T alpha = {
        DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS | DISPMANX_FLAGS_ALPHA_PREMULT,
        128, /*alpha 0->255*/
        0
    };

    dispmanx_context->update = vc_dispmanx_update_start(0);

    uint32_t vc_image_ptr;
    for (int i = 0; i < fbg->parallel_tasks; i += 1) {
        back_resources[i] = vc_dispmanx_resource_create(dispmanx_context->resource_type, fbg->width, fbg->height, &vc_image_ptr);
        front_resources[i] = vc_dispmanx_resource_create(dispmanx_context->resource_type, fbg->width, fbg->height, &vc_image_ptr);
        elems[i] = vc_dispmanx_element_add(dispmanx_context->update, dispmanx_context->display, (i + 1), dispmanx_context->dst_rect, dispmanx_context->front_resource, dispmanx_context->src_rect, 
            DISPMANX_PROTECTION_NONE, &alpha, NULL, DISPMANX_NO_ROTATE);
    }

    vc_dispmanx_update_submit_sync(dispmanx_context->update);
}

// 'accelerated' mixing by using dispmanx, this is much faster (about 4x than single thread mixing at 1920x1080; around 8fps)
void fbg_mixing(struct _fbg *fbg, unsigned char *buffer, int task_id) {
    struct _fbg_dispmanx_context *dispmanx_context = (struct _fbg_dispmanx_context *)fbg->user_context;

    int ret = vc_dispmanx_resource_write_data(back_resources[task_id - 1], dispmanx_context->resource_type, dispmanx_context->pitch, buffer, dispmanx_context->dst_rect);
}

void fbg_mixingFlip(struct _fbg *fbg) {
    struct _fbg_dispmanx_context *dispmanx_context = (struct _fbg_dispmanx_context *)fbg->user_context;

    for (int i = 0; i < fbg->parallel_tasks; i += 1) {
        //dispmanx_context->update = vc_dispmanx_update_start(0);
        vc_dispmanx_element_change_source(dispmanx_context->update, elems[i], back_resources[i]);

        DISPMANX_RESOURCE_HANDLE_T tmp = front_resources[i];
        front_resources[i] = back_resources[i];
        back_resources[i] = tmp;
        //vc_dispmanx_update_submit_sync(dispmanx_context->update);
    }
}

void freeDispmanxMixing(struct _fbg *fbg) {
    struct _fbg_dispmanx_context *dispmanx_context = (struct _fbg_dispmanx_context *)fbg->user_context;
    
    int result;
    for (int i = 0; i < fbg->parallel_tasks; i += 1) {
        result = vc_dispmanx_resource_delete(back_resources[i]);
        result = vc_dispmanx_resource_delete(front_resources[i]);
        result = vc_dispmanx_element_remove(dispmanx_context->update, elems[i]);
    }
}
//

int main(int argc, char* argv[]) {
    struct _fbg *fbg = fbg_dispmanxSetup(0, VC_IMAGE_RGB888);
    if (fbg == NULL) {
        return 0;
    }

    struct _fbg_img *bb_font_img = fbg_loadPNG(fbg, "../examples/bbmode1_8x8.png");

    struct _fbg_font *bbfont = fbg_createFont(fbg, bb_font_img, 8, 8, 33);

    fbg_createFragment(fbg, fragmentStart, fragment, fragmentStop, 3);

    // only required with fbg_draw(fbg, fbg_mixing);
    setupDispmanxMixing(fbg);
    fbg_dispmanxOnFlip(fbg, fbg_mixingFlip);
    //

    srand(time(NULL));

    signal(SIGINT, int_handler);

    do {
        fbg_clear(fbg, 0);
        
        // regular mixing, slow at high resolution / use much main thread resources
        //fbg_draw(fbg, fbg_XORMixing);

        for (int j = 0; j < fbg->parallel_tasks; j += 1) {
            fbg_write(fbg, fbg->fps_char, 2, 2 + j * 10);
        }

        fbg_draw(fbg, fbg_mixing);
        
        fbg_flip(fbg);
    } while (keep_running);

    // only required with fbg_draw(fbg, fbg_mixing);
    freeDispmanxMixing(fbg);

    fbg_freeImage(bb_font_img);
    fbg_freeFont(bbfont);

    fbg_close(fbg);
}
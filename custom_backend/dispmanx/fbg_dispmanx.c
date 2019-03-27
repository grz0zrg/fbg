#include <stdio.h>
#include <stdlib.h>

#include "fbg_dispmanx.h"

void fbg_dispmanxDraw(struct _fbg *fbg);
void fbg_dispmanxFlip(struct _fbg *fbg);
void fbg_dispmanxFree(struct _fbg *fbg);

struct _fbg *fbg_dispmanxSetup(uint32_t displayNumber) {
    bcm_host_init();

    struct _fbg_dispmanx_context *dispmanx_context = (struct _fbg_dispmanx_context *)calloc(1, sizeof(struct _fbg_dispmanx_context));
    if (!dispmanx_context) {
        fprintf(stderr, "fbg_dispmanxSetup: gles2 context calloc failed!\n");
        return NULL;
    }

    dispmanx_context->display = vc_dispmanx_display_open(displayNumber);
    if (dispmanx_context->display == 0) {
        fprintf(stderr, "fbg_dispmanxSetup: vc_dispmanx_display_open failed for display %i\n", displayNumber);
        free(dispmanx_context);
        return NULL;  
    }

    DISPMANX_MODEINFO_T info;
    int result = vc_dispmanx_display_get_info(dispmanx_context->display, &info);
    if (result != 0) {
        fprintf(stderr, "fbg_dispmanxSetup: vc_dispmanx_display_get_info failed for display %i\n", displayNumber);
        vc_dispmanx_display_close(dispmanx_context->display);
        free(dispmanx_context);
        return NULL;  
    }

    uint32_t vc_image_ptr;

    dispmanx_context->resource_type = VC_IMAGE_RGB888;
    dispmanx_context->back_resource = vc_dispmanx_resource_create(dispmanx_context->resource_type, info.width, info.height, &vc_image_ptr);
    if (dispmanx_context->back_resource == 0) {
        fprintf(stderr, "fbg_dispmanxSetup: vc_dispmanx_resource_create failed for display %i\n", displayNumber);
        vc_dispmanx_display_close(dispmanx_context->display);
        free(dispmanx_context);
        return NULL;  
    }

    dispmanx_context->front_resource = vc_dispmanx_resource_create(dispmanx_context->resource_type, info.width, info.height, &vc_image_ptr);
    if (dispmanx_context->front_resource == 0) {
        fprintf(stderr, "fbg_dispmanxSetup: vc_dispmanx_resource_create failed for display %i\n", displayNumber);
        vc_dispmanx_resource_delete(dispmanx_context->back_resource);
        vc_dispmanx_display_close(dispmanx_context->display);
        free(dispmanx_context);
        return NULL;  
    }

    dispmanx_context->update = vc_dispmanx_update_start(0);
    if (dispmanx_context->update == 0) {
        fprintf(stderr, "fbg_dispmanxSetup: vc_dispmanx_update_start failed for display %i\n", displayNumber);
        vc_dispmanx_resource_delete(dispmanx_context->back_resource);
        vc_dispmanx_resource_delete(dispmanx_context->front_resource);
        vc_dispmanx_display_close(dispmanx_context->display);
        free(dispmanx_context);
        return NULL;  
    }

    dispmanx_context->src_rect = malloc(sizeof(VC_RECT_T));
    dispmanx_context->dst_rect = malloc(sizeof(VC_RECT_T));

    if (dispmanx_context->src_rect == 0 || dispmanx_context->dst_rect == 0) {
        fprintf(stderr, "fbg_dispmanxSetup: src/dst rect malloc failed for display %i\n", displayNumber);
        vc_dispmanx_resource_delete(dispmanx_context->back_resource);
        vc_dispmanx_resource_delete(dispmanx_context->front_resource);
        vc_dispmanx_display_close(dispmanx_context->display);
        free(dispmanx_context->src_rect);
        free(dispmanx_context->dst_rect);
        free(dispmanx_context);
        return NULL;  
    }

    dispmanx_context->pitch = info.width * 3;

    vc_dispmanx_rect_set(dispmanx_context->src_rect, 0, 0, info.width << 16, info.height << 16);
    vc_dispmanx_rect_set(dispmanx_context->dst_rect, 0, 0, info.width, info.height);

    VC_DISPMANX_ALPHA_T alpha = {
        DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS,
        255, /*alpha 0->255*/
        0
    };

    dispmanx_context->elem = vc_dispmanx_element_add(dispmanx_context->update, dispmanx_context->display, 0, dispmanx_context->dst_rect, dispmanx_context->front_resource, dispmanx_context->src_rect, 
        DISPMANX_PROTECTION_NONE, &alpha, NULL, DISPMANX_NO_ROTATE);
    if (dispmanx_context->elem == 0) {
        fprintf(stderr, "fbg_dispmanxSetup: vc_dispmanx_element_add failed for display %i\n", displayNumber);
        vc_dispmanx_resource_delete(dispmanx_context->back_resource);
        vc_dispmanx_resource_delete(dispmanx_context->front_resource);
        vc_dispmanx_display_close(dispmanx_context->display);
        free(dispmanx_context->src_rect);
        free(dispmanx_context->dst_rect);
        free(dispmanx_context);
        return NULL;  
    }

    result = vc_dispmanx_update_submit_sync(dispmanx_context->update);

    struct _fbg *fbg = fbg_customSetup(info.width, info.height, (void *)dispmanx_context, fbg_dispmanxDraw, fbg_dispmanxFlip, NULL, fbg_dispmanxFree);

    return fbg;
}

void fbg_dispmanxOnFlip(struct _fbg *fbg, void (*opt_flip)(struct _fbg *fbg)) {
    struct _fbg_dispmanx_context *dispmanx_context = fbg->user_context;

    dispmanx_context->opt_flip = opt_flip;
}

void fbg_dispmanxDraw(struct _fbg *fbg) {
    struct _fbg_dispmanx_context *dispmanx_context = fbg->user_context;

    int ret = vc_dispmanx_resource_write_data(dispmanx_context->back_resource, dispmanx_context->resource_type, dispmanx_context->pitch, fbg->disp_buffer, dispmanx_context->dst_rect);
}

void fbg_dispmanxFlip(struct _fbg *fbg) {
    struct _fbg_dispmanx_context *dispmanx_context = fbg->user_context;

    dispmanx_context->update = vc_dispmanx_update_start(0);

    vc_dispmanx_element_change_source(dispmanx_context->update, dispmanx_context->elem, dispmanx_context->back_resource);

    DISPMANX_RESOURCE_HANDLE_T tmp = dispmanx_context->front_resource;
    dispmanx_context->front_resource = dispmanx_context->back_resource;
    dispmanx_context->back_resource = tmp;

    if (dispmanx_context->opt_flip) {
        dispmanx_context->opt_flip(fbg);
    }

    vc_dispmanx_update_submit_sync(dispmanx_context->update);
}

void fbg_dispmanxFree(struct _fbg *fbg) {
    struct _fbg_dispmanx_context *dispmanx_context = fbg->user_context;

    int result;

    dispmanx_context->update = vc_dispmanx_update_start(0);
    result = vc_dispmanx_element_remove(dispmanx_context->update, dispmanx_context->elem);
    if (result != 0) {
        fprintf(stderr, "fbg_dispmanxFree: vc_dispmanx_element_remove failed\n");
    }

    result = vc_dispmanx_update_submit_sync(dispmanx_context->update);
    if (result != 0) {
        fprintf(stderr, "fbg_dispmanxFree: vc_dispmanx_update_submit_sync failed\n");
    }

    result = vc_dispmanx_resource_delete(dispmanx_context->back_resource);
    if (result != 0) {
        fprintf(stderr, "fbg_dispmanxFree: vc_dispmanx_resource_delete failed\n");
    }

    result = vc_dispmanx_resource_delete(dispmanx_context->front_resource);
    if (result != 0) {
        fprintf(stderr, "fbg_dispmanxFree: vc_dispmanx_resource_delete failed\n");
    }

    result = vc_dispmanx_display_close(dispmanx_context->display);
    if (result != 0) {
        fprintf(stderr, "fbg_dispmanxFree: vc_dispmanx_display_close failed\n");
    }
}

#include <stdio.h>
#include <stdlib.h>

#include "fbg_dispmanx.h"

void fbg_dispmanxDraw(struct _fbg *fbg);
void fbg_dispmanxFlip(struct _fbg *fbg);
void fbg_dispmanxFree(struct _fbg *fbg);

#ifdef FBG_MMAL
static void callback_vr_input(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer) {
    mmal_buffer_header_release(buffer);
}
#endif

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

#ifdef FBG_MMAL
    vc_dispmanx_display_close(dispmanx_context->display);
#else
    // dispmanx setup
    uint32_t vc_image_ptr;

#ifdef FBG_RGBA
    dispmanx_context->resource_type = VC_IMAGE_RGBA32;
#else
    dispmanx_context->resource_type = VC_IMAGE_RGB888;
#endif

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
#endif

    struct _fbg *fbg = fbg_customSetup(info.width, info.height, (void *)dispmanx_context, fbg_dispmanxDraw, fbg_dispmanxFlip, NULL, fbg_dispmanxFree);

#ifdef FBG_MMAL
    mmal_component_create("vc.ril.video_render", &dispmanx_context->render);
    MMAL_COMPONENT_T *render = dispmanx_context->render;
    dispmanx_context->input = render->input[0];
    MMAL_PORT_T *input = dispmanx_context->input;
#ifdef FBG_RGBA
    input->format->encoding = MMAL_ENCODING_RGB24;
#else
    input->format->encoding = MMAL_ENCODING_RGBA;
#endif
    input->format->es->video.width  = VCOS_ALIGN_UP(fbg->width,  32);
    input->format->es->video.height = VCOS_ALIGN_UP(fbg->height, 16);
    input->format->es->video.crop.x = 0;
    input->format->es->video.crop.y = 0;
    input->format->es->video.crop.width  = fbg->width;
    input->format->es->video.crop.height = fbg->height;
    mmal_port_format_commit(input); 
    mmal_component_enable(render);
    mmal_port_parameter_set_boolean(input, MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE);
    input->buffer_size = input->buffer_size_recommended;
    input->buffer_num = input->buffer_num_recommended;
    if (input->buffer_num < 2)
    input->buffer_num = 2;
    dispmanx_context->pool = mmal_port_pool_create(input, input->buffer_num, input->buffer_size);
    {
        MMAL_DISPLAYREGION_T param;
        param.hdr.id = MMAL_PARAMETER_DISPLAYREGION;
        param.hdr.size = sizeof(MMAL_DISPLAYREGION_T);

        param.set = MMAL_DISPLAY_SET_LAYER;
        param.layer = 128;    //On top of most things

        param.set |= MMAL_DISPLAY_SET_ALPHA;
        param.alpha = 255;    //0 = transparent, 255 = opaque

        param.set |= (MMAL_DISPLAY_SET_DEST_RECT | MMAL_DISPLAY_SET_FULLSCREEN);
        param.fullscreen = MMAL_FALSE;
        param.dest_rect.x = 0;
        param.dest_rect.y = 0;
        param.dest_rect.width = fbg->width;
        param.dest_rect.height = fbg->height;
        mmal_port_parameter_set(input, &param.hdr);
    }
    mmal_port_enable(input, callback_vr_input);
#endif
    return fbg;
}

void fbg_dispmanxOnFlip(struct _fbg *fbg, void (*opt_flip)(struct _fbg *fbg)) {
    struct _fbg_dispmanx_context *dispmanx_context = fbg->user_context;

    dispmanx_context->opt_flip = opt_flip;
}

void fbg_dispmanxDraw(struct _fbg *fbg) {
    struct _fbg_dispmanx_context *dispmanx_context = fbg->user_context;

#ifdef FBG_MMAL
    MMAL_BUFFER_HEADER_T *buffer = mmal_queue_wait(dispmanx_context->pool->queue);

    memcpy(buffer->data, fbg->back_buffer, fbg->size);

    buffer->length = buffer->alloc_size;
    mmal_port_send_buffer(dispmanx_context->input, buffer);
#else
    int ret = vc_dispmanx_resource_write_data(dispmanx_context->back_resource, dispmanx_context->resource_type, dispmanx_context->pitch, fbg->disp_buffer, dispmanx_context->dst_rect);
#endif
}

void fbg_dispmanxFlip(struct _fbg *fbg) {
    struct _fbg_dispmanx_context *dispmanx_context = fbg->user_context;

#ifndef FBG_MMAL
    dispmanx_context->update = vc_dispmanx_update_start(0);

    vc_dispmanx_element_change_source(dispmanx_context->update, dispmanx_context->elem, dispmanx_context->back_resource);

    DISPMANX_RESOURCE_HANDLE_T tmp = dispmanx_context->front_resource;
    dispmanx_context->front_resource = dispmanx_context->back_resource;
    dispmanx_context->back_resource = tmp;

    if (dispmanx_context->opt_flip) {
        dispmanx_context->opt_flip(fbg);
    }

    vc_dispmanx_update_submit_sync(dispmanx_context->update);
#endif
}

void fbg_dispmanxFree(struct _fbg *fbg) {
    struct _fbg_dispmanx_context *dispmanx_context = fbg->user_context;

    int result;

#ifdef FBG_MMAL
    mmal_port_disable(dispmanx_context->input);
    mmal_component_destroy(dispmanx_context->render);
#else
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
#endif
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fbg_gba.h"

void fbg_gbaDraw(struct _fbg *fbg);

struct _fbg *fbg_gbaSetup(int mode) {
    struct _fbg_gba_context *gba_context = (struct _fbg_gba_context *)calloc(1, sizeof(struct _fbg_gba_context));
    if (!gba_context) {
        return NULL;
    }

    gba_context->mode = mode;

    *(unsigned int*)0x04000000 = 0x0400 + mode;

    gba_context->buffer = ((unsigned short*)0x06000000);

    int width = 240;
    int height = 160;
    if (mode == 5) {
        width = 160;
        height = 128;
    }

    struct _fbg *fbg = fbg_customSetup(width, height, 3, 0, 0, (void *)gba_context, fbg_gbaDraw, NULL, NULL, NULL);
    if (!fbg) {
        return NULL;
    }

    fbg->back_buffer = calloc(1, fbg->size * sizeof(char));
    if (!fbg->back_buffer) {
        return NULL;
    }
    
    return fbg;
}

void fbg_gbaDraw(struct _fbg *fbg) {
    struct _fbg_gba_context *gba_context = fbg->user_context;

    int x, y;
    for (x = 0; x < 240; ++x) {
        for (y = 0; y < 160; ++y) {
            int index = x + y * 240;

            int red = fbg->back_buffer[index * fbg->components];
            int green = fbg->back_buffer[index * fbg->components + 1];
            int blue = fbg->back_buffer[index * fbg->components + 2];
            gba_context->buffer[index] = (((red >> 3) & 31) | (((green >> 3) & 31) << 5) | (((blue >> 3) & 31) << 10));
        }
    }
}

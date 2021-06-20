#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include "fbg_fbdev.h"

void fbg_fbdevDraw(struct _fbg *fbg);
void fbg_fbdevFlip(struct _fbg *fbg);
void fbg_fbdevFree(struct _fbg *fbg);

struct _fbg *fbg_fbdevSetup(char *fb_device, int page_flipping) {
    struct _fbg_fbdev_context *fbdev_context = (struct _fbg_fbdev_context *)calloc(1, sizeof(struct _fbg_fbdev_context));
    if (!fbdev_context) {
        fprintf(stderr, "fbg_fbdevSetup: fbdev context calloc failed!\n");
        return NULL;
    }

    char *default_fb_device = "/dev/fb0";
    fb_device = fb_device ? fb_device : default_fb_device;

    fbdev_context->fd = open(fb_device, O_RDWR);

    if (fbdev_context->fd == -1) {
        fprintf(stderr, "fbg_fbdevSetup: Cannot open '%s'!\n", fb_device);

        return NULL;
    }

    if (ioctl(fbdev_context->fd, FBIOGET_VSCREENINFO, &fbdev_context->vinfo) == -1) {
        fprintf(stderr, "fbg_fbdevSetup: '%s' Cannot obtain framebuffer FBIOGET_VSCREENINFO informations!\n", fb_device);

        close(fbdev_context->fd);

        return NULL;
    }

    if (ioctl(fbdev_context->fd, FBIOGET_FSCREENINFO, &fbdev_context->finfo) == -1) {
        fprintf(stderr, "fbg_fbdevSetup: '%s' Cannot obtain framebuffer FBIOGET_FSCREENINFO informations!\n", fb_device);

        close(fbdev_context->fd);

        return NULL;
    }

    fprintf(stdout, "fbg_fbdevSetup: '%s' (%dx%d (%dx%d virtual) %d bpp (%d/%d, %d/%d, %d/%d) (%d smen_len) %d line_length)\n",
        fb_device,
        fbdev_context->vinfo.xres, fbdev_context->vinfo.yres,
        fbdev_context->vinfo.xres_virtual, fbdev_context->vinfo.yres_virtual,
        fbdev_context->vinfo.bits_per_pixel,
        fbdev_context->vinfo.red.length,
        fbdev_context->vinfo.red.offset,
        fbdev_context->vinfo.green.length,
        fbdev_context->vinfo.green.offset,
        fbdev_context->vinfo.blue.length,
        fbdev_context->vinfo.blue.offset,
        fbdev_context->finfo.smem_len,
        fbdev_context->finfo.line_length);

    if (fbdev_context->vinfo.bits_per_pixel != 16 && fbdev_context->vinfo.bits_per_pixel != 24 && fbdev_context->vinfo.bits_per_pixel != 32) {
        fprintf(stderr, "fbg_fbdevSetup: '%s' Unsupported format (only 16, 24 or 32 bits framebuffer is supported)!\n", fb_device);

        close(fbdev_context->fd);

        return NULL;
    }

    int components = 3;

    if (fbdev_context->vinfo.bits_per_pixel == 16) {
        fprintf(stdout, "fbg_fbdevSetup: 16 bpp framebuffer detected; page flipping option not supported; all graphics operations will be managed in 24 bits then converted to 16 bpp before drawing.\n");

        page_flipping = 0;
    } else {
        components = fbdev_context->vinfo.bits_per_pixel / 8;
    }

    struct _fbg *fbg = fbg_customSetup(fbdev_context->vinfo.xres, fbdev_context->vinfo.yres, components, 0, 0, (void *)fbdev_context, fbg_fbdevDraw, fbg_fbdevFlip, NULL, fbg_fbdevFree);
    if (!fbg) {
        fprintf(stderr, "fbg_fbdevSetup: fbg_customSetup failed\n");

        close(fbdev_context->fd);

        return NULL;
    }

    if ((fbdev_context->vinfo.bits_per_pixel == 24 || fbdev_context->vinfo.bits_per_pixel == 32) &&
        fbdev_context->vinfo.red.length == 8 &&
        fbdev_context->vinfo.red.offset == 16 &&
        fbdev_context->vinfo.green.length == 8 &&
        fbdev_context->vinfo.blue.length == 8 &&
        fbdev_context->vinfo.blue.offset == 0 &&
        fbdev_context->vinfo.green.offset == 8) {
        fbg->bgr = 1;
    }

    if (fbdev_context->vinfo.bits_per_pixel == 16 &&
        fbdev_context->vinfo.red.offset == 11) {
        fbg->bgr = 1;
    }

    if (page_flipping) {
        // check for page flipping support
        if (ioctl(fbdev_context->fd, FBIOPAN_DISPLAY, &fbdev_context->vinfo) == -1) {
            fprintf(stderr, "fbg_fbdevSetup: '%s' FBIOPAN_DISPLAY / page flipping not supported!\n", fb_device);
        } else {
            // double the virtual height
            fbdev_context->vinfo.yres_virtual = fbdev_context->vinfo.yres_virtual * 2;
            if (ioctl(fbdev_context->fd, FBIOPUT_VSCREENINFO, &fbdev_context->vinfo) == -1) {
                fprintf(stderr, "fbg_fbdevSetup: '%s' FBIOPUT_VSCREENINFO failed, page flipping disabled!\n", fb_device);
            } else {
                fbdev_context->page_flipping = 1;

                fprintf(stdout, "fbg_fbdevSetup: '%s' Page flipping enabled (virtual height was doubled)!\n", fb_device);

                if (ioctl(fbdev_context->fd, FBIOGET_FSCREENINFO, &fbdev_context->finfo) == -1) {
                    fprintf(stderr, "fbg_fbdevSetup: '%s' Cannot obtain framebuffer FBIOGET_FSCREENINFO informations!\n", fb_device);

                    close(fbdev_context->fd);

                    return NULL;
                }
            }
        }

        if (!fbdev_context->page_flipping) {
            fprintf(stderr, "fbg_fbdevSetup: '%s' FBIOPAN_DISPLAY / page flipping not supported!\n", fb_device);
        }
    }

    // initialize framebuffer
    fbdev_context->buffer = (unsigned char *)mmap(0, fbdev_context->finfo.smem_len,
        PROT_WRITE,
        MAP_SHARED,
        fbdev_context->fd, 0);

    memset(fbdev_context->buffer, 0, fbdev_context->finfo.smem_len);

    // setup page flipping
    if (fbdev_context->page_flipping) {
        fbg->disp_buffer = fbdev_context->buffer;
        fbg->back_buffer = fbdev_context->buffer + fbg->width * fbg->components * fbg->height;
    } else {
        // setup front & back buffers
        fbg->back_buffer = calloc(1, fbg->size * sizeof(char));
        if (!fbg->back_buffer) {
            fprintf(stderr, "fbg_fbdevSetup: back_buffer calloc failed!\n");

            close(fbdev_context->fd);

            return NULL;
        }

        fbg->disp_buffer = calloc(1, fbg->size * sizeof(char));
        if (!fbg->disp_buffer) {
            fprintf(stderr, "fbg_fbdevSetup: disp_buffer calloc failed!\n");

            free(fbg->back_buffer);
            close(fbdev_context->fd);

            return NULL;
        }
    }

    return fbg;
}

void fbg_fbdevDraw(struct _fbg *fbg) {
    struct _fbg_fbdev_context *fbdev_context = fbg->user_context;

#ifdef FBIO_WAITFORVSYNC
    static int dummy = 0;
    ioctl(fbdev_context->fd, FBIO_WAITFORVSYNC, &dummy);
#endif

    if (fbdev_context->page_flipping == 0) {
        if (fbdev_context->vinfo.bits_per_pixel == 16) {
            unsigned char *pix_pointer_src = fbg->disp_buffer;
            unsigned char *pix_pointer_dst = fbdev_context->buffer;

            int i = 0;

            for (i = 0; i < fbg->width_n_height; i += 1) {
                unsigned int v = ((*pix_pointer_src++ >> 3) & 0x1f);
                v |= ((*pix_pointer_src++ >> 2) & 0x3f) << 5;
                v |= ((*pix_pointer_src++ >> 3) & 0x1f) << 11;

                *pix_pointer_dst++ = v;
                *pix_pointer_dst++ = v >> 8;;
            }
        } else {
            memcpy(fbdev_context->buffer, fbg->disp_buffer, fbg->size);
        }
    }
}

void fbg_fbdevFlip(struct _fbg *fbg) {
    struct _fbg_fbdev_context *fbdev_context = fbg->user_context;
    
    if (fbdev_context->page_flipping) {
        if (fbdev_context->vinfo.yoffset == 0) {
            fbdev_context->vinfo.yoffset = fbg->height;
        } else {
            fbdev_context->vinfo.yoffset = 0;
        }

        if (ioctl(fbdev_context->fd, FBIOPAN_DISPLAY, &fbdev_context->vinfo) == -1) {
            fprintf(stderr, "fbg_fbdevFlip: FBIOPAN_DISPLAY failed!\n");
        }
    }

    unsigned char *tmp_buffer = fbg->disp_buffer;
    fbg->disp_buffer = fbg->back_buffer;
    fbg->back_buffer = tmp_buffer;
}

void fbg_fbdevFree(struct _fbg *fbg) {
    struct _fbg_fbdev_context *fbdev_context = fbg->user_context;

    if (!fbdev_context->page_flipping) {
        free(fbg->back_buffer);
        free(fbg->disp_buffer);
    }

    if (fbdev_context->buffer) {
        munmap(fbdev_context->buffer, fbdev_context->finfo.smem_len);
        close(fbdev_context->fd);
    }
}

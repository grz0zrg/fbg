/*
    Copyright (c) 2019, Julien Verneuil
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the organization nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL Julien Verneuil BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef FB_GRAPHICS_DISPMANX_H
#define FB_GRAPHICS_DISPMANX_H

    #include <sys/ioctl.h>
    #include <linux/fb.h>
    #include <unistd.h>
    #include <fcntl.h>

    #include <GLES2/gl2.h>
    #include <EGL/egl.h>
    #include <EGL/eglext.h>

    #include "bcm_host.h"

    #include "fbgraphics.h"

#ifdef MMAL
    #include <interface/mmal/mmal.h>
    #include <interface/mmal/util/mmal_util.h>
    #include <interface/mmal/util/mmal_connection.h>
    #include <interface/mmal/util/mmal_util_params.h>
#endif

    //! dispmanx wrapper data structure
    struct _fbg_dispmanx_context {
#ifdef MMAL
      //! MMAL component
      MMAL_COMPONENT_T *render;
      //! MMAL input port
      MMAL_PORT_T *input;
      //! MMAL pool
      MMAL_POOL_T *pool;
#endif
      //! dispmanx display
      DISPMANX_DISPLAY_HANDLE_T display;
      //! dispmanx back resource
      DISPMANX_RESOURCE_HANDLE_T back_resource;
      //! dispmanx front resource
      DISPMANX_RESOURCE_HANDLE_T front_resource;
      //! dispmanx elem
      DISPMANX_ELEMENT_HANDLE_T elem;
      //! dispmanx image type (RGB888)
      VC_IMAGE_TYPE_T resource_type;
      //! dispmanx update
      DISPMANX_UPDATE_HANDLE_T update;
      //! dispmanx src rect
      VC_RECT_T *src_rect;
      //! dispmanx src rect
      VC_RECT_T *dst_rect;

      //! optional flip content (will be executed between dispmanx start / sync)
      void (*opt_flip)(struct _fbg *fbg);

      //! fbg->width * 3
      int pitch;
    };

    //! initialize a FB Graphics dispmanx context
    /*!
      \param displayNumber dispmanx display number
      \return FBG data structure pointer
    */
    extern struct _fbg *fbg_dispmanxSetup(uint32_t displayNumber);

    //! add additional flip content (will be executed between dispmanx start / sync)
    /*!
      \param fbg FBG data structure pointer
      \param opt_flip flip function executed between dispmanx start / sync upong fbg_flip() call
    */
    extern void fbg_dispmanxOnFlip(struct _fbg *fbg, void (*opt_flip)(struct _fbg *fbg));

#endif
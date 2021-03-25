/*
    Copyright (c) 2019, 2020 Julien Verneuil
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

#ifndef FB_GRAPHICS_FBDEV_H
#define FB_GRAPHICS_FBDEV_H

    #include <linux/fb.h>
    #include "fbgraphics.h"

    //! fbdev wrapper data structure
    struct _fbg_fbdev_context {
      //! Framebuffer device file descriptor
      int fd;

      //! Memory-mapped framebuffer
      unsigned char *buffer;
    
      //! Framebuffer device var. informations
      struct fb_var_screeninfo vinfo;
      //! Framebuffer device fix. informations
      struct fb_fix_screeninfo finfo;

      //! Flag indicating that page flipping is enabled
      int page_flipping;
    };

    //! initialize a FB Graphics context (framebuffer)
    /*!
      \param fb_device framebuffer device (example : /dev/fb0)
      \param page_flipping wether to use page flipping mechanism for double buffering (slow on some devices)
      \return _fbg structure pointer to pass to any FBG library functions
    */
    extern struct _fbg *fbg_fbdevSetup(char *fb_device, int page_flipping);

    //! initialize a FB Graphics context with '/dev/fb0' as framebuffer device and no page flipping
    #define fbg_fbdevInit() fbg_fbdevSetup(NULL, 0)
#endif

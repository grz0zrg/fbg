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

#ifndef FB_GRAPHICS_OPENGL_ES2_H
#define FB_GRAPHICS_OPENGL_ES2_H

    #include <sys/ioctl.h>
    #include <linux/fb.h>
    #include <unistd.h>
    #include <fcntl.h>

    #include <GLES2/gl2.h>
    #include <EGL/egl.h>
    #include <EGL/eglext.h>

#ifdef FBG_RPI
    #include "bcm_host.h"
#endif

    #include "fbgraphics.h"

    //! OpenGL ES 2.0 wrapper data structure
    struct _fbg_gles2_context {
#ifndef FBG_RPI
        //! Framebuffer file descriptor
        int fd;
        //! Framebuffer device var. informations
        struct fb_var_screeninfo vinfo;
#endif
        //! EGL display
        EGLDisplay egl_display;
        //! EGL context
        EGLContext egl_context;
        //! EGL surface
        EGLContext egl_surface;
        //! EGL image
        void *egl_image;
        //! Simple GLSL program (screen-aligned textured quad)
        GLenum simple_program;
        //! FBG VBO
        GLuint fbg_vbo;
        //! FBG texture (updated at each frames)
        GLuint fbg_texture;
        //! tell wether fbg_gles2 should update fbg disp_buffer after rendering
        int update_buffer;
    };

    //! struct which hold usual VBO data for 3D objects
    struct _fbg_vbo_data {
      // VBO (vertices)
      GLuint vbo;
      // VBO (texcoords)
      GLuint tbo;
      // VBO (indices)
      GLuint ibo;
      // VBO (normals)
      GLuint nbo;
      // VBO (colors)
      GLuint cbo;
    };

    //! Simple quad geometry (vertices + UV)
    extern const GLfloat fbg_gles2Quad[];

    //! Simple vertex shader (screen quad, vertices + UV)
    extern const char *fbg_gles2SimpleVs;

    //! Simple textured fragment shader
    extern const char *fbg_gles2SimpleFs;

    //! initialize a FB Graphics OpenGL ES 2 (fbdev or RPI direct) context
    /*!
      \param fb_device framebuffer device; example : /dev/fb0
      \param components fbg context color components (4 for RGBA or 3 for RGB)
      \return FBG data structure pointer
    */
#ifdef FBG_RPI
    extern struct _fbg *fbg_gles2Setup(int components);
#else
    extern struct _fbg *fbg_gles2Setup(const char *fb_device, int components);
#endif

    //! OpenGL clear
    extern void fbg_gles2Clear();

    //! this update FBG disp_buffer with the actual rendered OpenGL content
    /*!
      \param fbg pointer to a FBG context / data structure
    */
    extern void fbg_gles2UpdateBuffer(struct _fbg *fbg);

    //! Query the user requested (window close etc) close status
    /*!
      \param fbg pointer to a FBG context / data structure
      \return Boolean indicating close status
    */
    extern int fbg_gles2ShouldClose(struct _fbg *fbg);

    //! create a non-interpolated (NEAREST) GL texture from a FBG image
    /*!
      \param fbg pointer to a FBG context / data structure
      \param img image structure pointer
      \return GL texture id
    */
    GLuint fbg_gles2CreateTextureFromImage(struct _fbg *fbg, struct _fbg_img *img);

    //-- A SET OF RAW OPENGL UTILITY FUNCTIONS --
    //! create an empty non-interpolated (NEAREST) GL texture
    /*!
      \param width width of the requested texture
      \param height height of the requested texture
      \param internal_format OpenGL format (GL_RGBA etc.)
      \return GL texture id
    */
    extern GLuint fbg_gles2CreateTexture(GLuint width, GLuint height, GLint internal_format);

    //! create a VBO from indexed data, support for vertices, UVs, normals and colors
    /*!
      \param indices_count indices count
      \param indices_data data containing indices
      \param sizeof_indice_type sizeof indice type
      \param vertices_count vertices count
      \param vertices_data data containing vertices
      \param texcoords_count uv count
      \param texcoords_data data containing uv
      \param normals_count normals count
      \param normals_data data containing normals
      \param colors_count colors count
      \param colors_data data containing colors
      \return _fbg_vbo_data data structure containing the VBOs
    */
    extern struct _fbg_vbo_data * fbg_gles2CreateVBO(GLsizeiptr indices_count, const GLvoid *indices_data, size_t sizeof_indice_type,
                                    GLsizeiptr vertices_count, const GLvoid *vertices_data,
                                    GLsizeiptr texcoords_count, const GLvoid *texcoords_data,
                                    GLsizeiptr normals_count, const GLvoid *normals_data,
                                    GLsizeiptr colors_count, const GLvoid *colors_data);

    //! free VBO data (created with fbg_gles2CreateVBO)
    /*!
      \param vbo_data data structure containing the VBOs
    */
    extern void fbg_gles2FreeVBOData(struct _fbg_vbo_data *vbo_data);

    //! create a VBO from vertices + UV data packed into a single array
    /*!
      \param data_count vertices data count
      \param data data containing all vertices (set of 3 x float) then all associated UVs (set of 2 x float)
      \return GL VBO id
    */
    extern GLuint fbg_gles2CreateVBOvu(GLsizeiptr data_count, const GLvoid *data);

    //! create a FBO
    /*!
      \param texture GL texture id
      \return GL FBO id
    */
    //extern GLuint fbg_gles2CreateFBO(GLuint texture);

    //! create a single shader
    /*!
      \param type GL shader type
      \param source shader code
      \return GL shader id
    */
    extern GLuint fbg_gles2CreateShader(GLenum type, const GLchar *source);

    //! create a shader from the content of a file
    /*!
      \param type GL shader type
      \param filename file to load
      \return GL shader id
    */
    extern GLuint fbg_gles2CreateShaderFromFile(GLenum type, const char *filename);

    //! create a vertex and/or fragment program
    /*!
      \param vertex_shader vertex shader id, can be 0
      \param fragment_shader fragment shader id, can be 0
      \return GL shader id
    */
    extern GLuint fbg_gles2CreateProgram(GLuint vertex_shader, GLuint fragment_shader);

    //! create a vertex and/or fragment/geometry program from a file
    /*!
      \param vs vertex shader file
      \param fs fragment shader file
      \return GL shader id
    */
    extern GLenum fbg_gles2CreateProgramFromFiles(const char *vs, const char *fs);

    //! create a vertex and/or fragment/geometry program from a string
    /*!
      \param vs vertex shader string
      \param fs fragment shader string
      \return GL shader id
    */
    extern GLenum fbg_gles2CreateProgramFromString(const char *vs, const char *fs);

#endif

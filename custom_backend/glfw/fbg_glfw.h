/*
    Copyright (c) 2018, Julien Verneuil
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

#ifndef FB_GRAPHICS_GLFW_H
#define FB_GRAPHICS_GLFW_H

    #include <GL/glew.h>
    #include <GLFW/glfw3.h>

    #include "fbgraphics.h"

    //! GLFW wrapper data structure
    struct _fbg_glfw_context {
        //! GLFW window
        GLFWwindow *window;
        //! GLFW monitor
        GLFWmonitor *monitor;
        //! Simple GLSL program (screen-aligned textured quad)
        GLenum simple_program;
        //! FBG VAO
        GLuint fbg_vao;
        //! FBG texture (updated at each frames)
        GLuint fbg_texture;
        //! tell wether fbg_glfw should update fbg disp_buffer after rendering
        int update_buffer;
        //! context width
        int width;
        //! context height
        int height;
        //! upscale factor for the fbg buffer (serve off as antialiasing; the fbg buffer is downscaled to display size)
        int ssaa;
    };

    //! Simple quad geometry (vertices + UV)
    extern const GLfloat fbg_glfwQuad[];

    //! Simple vertex shader (screen quad, vertices + UV)
    extern const char *fbg_glfwSimpleVs;

    //! Simple textured fragment shader
    extern const char *fbg_glfwSimpleFs;

    //! initialize a FB Graphics OpenGL context (GLFW library)
    /*!
      \param width window width
      \param height window height
      \param components fbg context color components (4 for RGBA or 3 for RGB)
      \param title window title
      \param monitor monitor id (0 = primary display)
      \param fullscreen 0 = windowed, 1 = fullscreen, 2 = windowed full screen
      \param ssaa the resolution factor of the fbg context (not OpenGL), mainly for antiliasing, note that you may need to use the glfw_context->width then to refer to the display width instead of fbg->width which will be the internal resolution
      \return FBG data structure pointer
    */
    extern struct _fbg *fbg_glfwSetup(int width, int height, int components, const char *title, int monitor, int fullscreen, int ssaa);

    //! OpenGL clear
    extern void fbg_glfwClear();

    //! this update FBG disp_buffer with the actual rendered OpenGL content
    /*!
      \param fbg pointer to a FBG context / data structure
    */
    extern void fbg_glfwUpdateBuffer(struct _fbg *fbg);

    //! Query the user requested (window close etc) close status
    /*!
      \param fbg pointer to a FBG context / data structure
      \return Boolean indicating close status
    */
    extern int fbg_glfwShouldClose(struct _fbg *fbg);

    //! Switch to fullscreen or windowed mode
    /*!
      \param fbg pointer to a FBG context / data structure
      \param enable Boolean indicating  windowed or fullscreen
    */
    extern void fbg_glfwFullscreen(struct _fbg *fbg, int enable);

    //! Display resize
    /*!
      \param fbg pointer to a FBG context / data structure
      \param new_width new display width
      \param new_height new display height
    */
    extern void fbg_glfwResize(struct _fbg *fbg, unsigned int new_width, unsigned new_height);

    //! create a non-interpolated (NEAREST) GL texture from a FBG image
    /*!
      \param fbg pointer to a FBG context / data structure
      \param img image structure pointer
      \return GL texture id
    */
    GLuint fbg_glfwCreateTextureFromImage(struct _fbg *fbg, struct _fbg_img *img);

    //-- A SET OF RAW OPENGL UTILITY FUNCTIONS --
    //! create an empty non-interpolated (NEAREST) GL texture
    /*!
      \param width width of the requested texture
      \param height height of the requested texture
      \param internal_format OpenGL format (GL_RGBA etc.)
      \return GL texture id
    */
    extern GLuint fbg_glfwCreateTexture(GLuint width, GLuint height, GLint internal_format);

    //! create a VAO from indexed data, support for vertices, UVs, normals and colors
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
      \return GL VAO id
    */
    extern GLuint fbg_glfwCreateVAO(GLsizeiptr indices_count, const GLvoid *indices_data, size_t sizeof_indice_type,
                                    GLsizeiptr vertices_count, const GLvoid *vertices_data,
                                    GLsizeiptr texcoords_count, const GLvoid *texcoords_data,
                                    GLsizeiptr normals_count, const GLvoid *normals_data,
                                    GLsizeiptr colors_count, const GLvoid *colors_data);

    //! create a VAO from vertices + UV data packed into a single array
    /*!
      \param data_count vertices data count
      \param data data containing all vertices (set of 3 x float) then all associated UVs (set of 2 x float)
      \return GL VAO id
    */
    extern GLuint fbg_glfwCreateVAOvu(GLsizeiptr data_count, const GLvoid *data);

    //! create a FBO
    /*!
      \param texture GL texture id
      \return GL FBO id
    */
    extern GLuint fbg_glfwCreateFBO(GLuint texture);

    //! create a single shader
    /*!
      \param type GL shader type
      \param source shader code
      \return GL shader id
    */
    extern GLuint fbg_glfwCreateShader(GLenum type, const GLchar *source);

    //! create a shader from the content of a file
    /*!
      \param type GL shader type
      \param filename file to load
      \return GL shader id
    */
    extern GLuint fbg_glfwCreateShaderFromFile(GLenum type, const char *filename);

    //! create a vertex and/or fragment program
    /*!
      \param vertex_shader vertex shader id, can be 0
      \param fragment_shader fragment shader id, can be 0
      \param geometry_shader geometry shader id, can be 0
      \return GL shader id
    */
    extern GLuint fbg_glfwCreateProgram(GLuint vertex_shader, GLuint fragment_shader, GLuint geometry_shader);

    //! create a vertex and/or fragment/geometry program from a file
    /*!
      \param vs vertex shader file
      \param fs fragment shader file
      \param gs geometry shader file
      \return GL shader id
    */
    extern GLenum fbg_glfwCreateProgramFromFiles(const char *vs, const char *fs, const char *gs);

    //! create a vertex and/or fragment/geometry program from a string
    /*!
      \param vs vertex shader string
      \param fs fragment shader string
      \param gs geometry shader string
      \return GL shader id
    */
    extern GLenum fbg_glfwCreateProgramFromString(const char *vs, const char *fs, const char *gs);

#endif

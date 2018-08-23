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
        //! Simple GLSL program (screen-aligned textured quad)
        GLenum simple_program;
        //! FBG VAO
        GLuint fbg_vao;
        //! FBG texture (updated at each frames)
        GLuint fbg_texture;
    };

    //! initialize a FB Graphics OpenGL context (GLFW library)
    extern struct _fbg *fbg_glfwSetup(int width, int height, const char *title);

    //! Query the user requested (window close etc) close status
    /*!
      \param fbg pointer to a FBG context / data structure
      \return Boolean indicating close status
    */
    extern int fbg_glfwShouldClose(struct _fbg *fbg);

    //! create a non-interpolated (NEAREST) GL texture from a FBG image
    /*!
      \param img image structure pointer
      \return GL texture id
    */
    GLuint fbg_glfwCreateTextureFromImage(struct _fbg_img *img);

    //-- A SET OF RAW OPENGL UTILITY FUNCTIONS --
    //! create an empty non-interpolated (NEAREST) GL texture
    /*!
      \param width width of the requested texture
      \param height height of the requested texture
      \return GL texture id
    */
    extern GLuint fbg_glfwCreateTexture(GLuint width, GLuint height);

    //! create a VAO
    /*!
      \return GL VAO id
    */
    extern GLuint fbg_glfwCreateVAO();

    //! create a VBO containing vertices + UV data; always attached to a VAO
    /*!
      \param vao GL VAO id (created using fbg_glfwCreateVAO)
      \param data_count vertices data count
      \param data data containing vertices (set of 3 x float) + UV (set of 2 x float)
      \return GL VBO id
    */
    extern GLuint fbg_glfwCreateVBO(GLuint vao, GLsizeiptr data_count, const GLvoid *data);

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
      \return GL shader id
    */
    extern GLuint fbg_glfwCreateProgram(GLuint vertex_shader, GLuint fragment_shader);

    //! create a vertex and/or fragment program from a file
    /*!
      \param vs vertex shader file
      \param vs fragment shader file
      \return GL shader id
    */
    extern GLenum fbg_glfwCreateProgramFromFiles(const char *vs, const char *fs);

    //! create a vertex and/or fragment program from a string
    /*!
      \param vs vertex shader string
      \param vs fragment shader string
      \return GL shader id
    */
    extern GLenum fbg_glfwCreateProgramFromString(const char *vs, const char *fs);

#endif
#include <stdio.h>
#include <stdlib.h>

#include "fbg_opengl_es2.h"

const GLfloat fbg_gles2Quad[] = { -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, // vertices
                        0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f }; // UV

const char *fbg_gles2SimpleVs = "attribute vec3 vp; \
    attribute vec2 vu; \
    varying vec2 uv; \
    void main() { \
        uv = vu; \
        gl_Position = vec4(vp, 1.0); \
    }";

const char *fbg_gles2SimpleFs = "varying vec2 uv; \
    uniform sampler2D t0; \
    void main() { \
        gl_FragColor = texture2D(t0, uv); \
    }";

void fbg_gles2Draw(struct _fbg *fbg);
void fbg_gles2Flip(struct _fbg *fbg);
void fbg_gles2Free(struct _fbg *fbg);

#ifdef FBG_RPI
struct _fbg *fbg_gles2Setup() {
    bcm_host_init();
#else // fbdev
struct _fbg *fbg_gles2Setup(const char *fb_device) {
#endif
    struct _fbg_gles2_context *gles2_context = (struct _fbg_gles2_context *)calloc(1, sizeof(struct _fbg_gles2_context));
    if (!gles2_context) {
        fprintf(stderr, "fbg_gles2Setup: gles2 context calloc failed!\n");
        return NULL;
    }

#ifndef FBG_RPI
    int fd = open(fb_device, O_RDWR);
    if (fd == -1) {
        printf("fbg_gles2Setup: cannot open %s device\n", fb_device);
        free(gles2_context);
        return NULL;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &gles2_context->vinfo)) {
        printf("fbg_gles2Setup: ioctl FBIOGET_VSCREENINFO failed\n");
        free(gles2_context);
        close(fd);
        return NULL;
    }

    gles2_context->fd = fd;
#else
    static EGL_DISPMANX_WINDOW_T nativewindow;

    DISPMANX_ELEMENT_HANDLE_T dispman_element;
    DISPMANX_DISPLAY_HANDLE_T dispman_display;
    DISPMANX_UPDATE_HANDLE_T dispman_update;
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;
#endif

    //setenv("EGL_PLATFORM", "fbdev", 0);
    //setenv("FRAMEBUFFER", fb_device, 0);

    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;

    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_display == EGL_NO_DISPLAY) {
        printf("fbg_gles2Setup: eglGetDisplay failed with EGL_NO_DISPLAY\n");
        free(gles2_context);
#ifndef FBG_RPI
        close(fd);
#endif
        return NULL;
    }
 
    if (!eglInitialize(egl_display, NULL, NULL)) {
        printf("fbg_gles2Setup: eglInitialize failed\n");
        free(gles2_context);
#ifndef FBG_RPI
        close(fd);
#endif
        return NULL;
    }
 
#ifdef FBG_RPI
    static const EGLint attr[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_SAMPLES, 0,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };
#else
    static const EGLint attr[] = {
        /*EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 0,
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 16,*/
        EGL_BUFFER_SIZE, 16,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
#endif
 
    EGLConfig eglconf;
    EGLint num_config;

#ifndef FBG_RPI
    if (!eglChooseConfig(egl_display, attr, &eglconf, 1, &num_config)) {
        printf("fbg_gles2Setup: eglChooseConfig failed\n");
        eglTerminate(egl_display);
        free(gles2_context);
        close(fd);
        return NULL;
    }
#else
    if (!eglSaneChooseConfigBRCM(egl_display, attr, &eglconf, 1, &num_config)) {
        printf("fbg_gles2Setup: eglSaneChooseConfigBRCM failed\n");
        eglTerminate(egl_display);
        free(gles2_context);
        return NULL;
    }
#endif

#ifdef FBG_RPI
    EGLBoolean result = eglBindAPI(EGL_OPENGL_ES_API);
    if (result == EGL_FALSE || result == EGL_BAD_PARAMETER) {
        printf("fbg_gles2Setup: eglCreateContext failed with EGL_NO_CONTEXT\n");
        eglTerminate(egl_display);
        free(gles2_context);
        return NULL;
    }
#endif

    static const EGLint ctxattr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    egl_context = eglCreateContext(egl_display, eglconf, EGL_NO_CONTEXT, ctxattr);
    if (egl_context == EGL_NO_CONTEXT) {
        printf("fbg_gles2Setup: eglCreateContext failed with EGL_NO_CONTEXT\n");
        eglTerminate(egl_display);
        free(gles2_context);
#ifndef FBG_RPI
        close(fd);
#endif
        return NULL;
    }

    uint32_t screen_width, screen_height;
    uint32_t render_width, render_height;

#ifdef FBG_RPI
    int32_t success = graphics_get_display_size(0 /* LCD */, &screen_width, &screen_height);

    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = screen_width;
    dst_rect.height = screen_height;

    // change this for custom render size
    render_width = screen_width;
    render_height = screen_height;
    
    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = render_width << 16;
    src_rect.height = render_height << 16;        

    dispman_display = vc_dispmanx_display_open(0 /* LCD */);
    dispman_update = vc_dispmanx_update_start(0);
        
    dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display, 0/*layer*/, &dst_rect, 0/*src*/, &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/);
    
    nativewindow.element = dispman_element;
    nativewindow.width = render_width;
    nativewindow.height = render_height;
    vc_dispmanx_update_submit_sync(dispman_update);

    egl_surface = eglCreateWindowSurface(egl_display, eglconf, &nativewindow, NULL);
#else
    const EGLNativeWindowType native_win = (EGLNativeWindowType) NULL;
    egl_surface = eglCreateWindowSurface(egl_display, eglconf, native_win, NULL);

    screen_width = render_width = gles2_context->vinfo.xres;
    screen_height = render_height = gles2_context->vinfo.yres;
#endif

    if (egl_surface == EGL_NO_SURFACE) {
        printf("fbg_gles2Setup: eglCreateWindowSurface failed with EGL_NO_SURFACE\n");
        eglDestroyContext(egl_display, egl_context);
        eglTerminate(egl_display);
        free(gles2_context);
#ifndef FBG_RPI
        close(fd);
#endif
        return NULL;
    }
 
    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

    gles2_context->egl_display = egl_display;
    gles2_context->egl_context = egl_context;
    gles2_context->egl_surface = egl_surface;

    gles2_context->simple_program = fbg_gles2CreateProgramFromString(fbg_gles2SimpleVs, fbg_gles2SimpleFs);
    gles2_context->fbg_vbo = fbg_gles2CreateVBOvu(12, &fbg_gles2Quad[0]);
    gles2_context->fbg_texture = fbg_gles2CreateTexture(render_width, render_height);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1); 

    glViewport(0, 0, screen_width, screen_height);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    struct _fbg *fbg = fbg_customSetup(render_width, render_height, (void *)gles2_context, fbg_gles2Draw, fbg_gles2Flip, NULL, fbg_gles2Free);

    return fbg;
}

void fbg_gles2UpdateBuffer(struct _fbg *fbg) {
#ifdef FBG_RGBA
    glReadPixels(0, 0, fbg->width, fbg->height, GL_RGBA, GL_UNSIGNED_BYTE, fbg->disp_buffer);
#else
    glReadPixels(0, 0, fbg->width, fbg->height, GL_RGB, GL_UNSIGNED_BYTE, fbg->disp_buffer);
#endif
}

void fbg_gles2Clear() {
    glClear(GL_COLOR_BUFFER_BIT);
}

void fbg_gles2Draw(struct _fbg *fbg) {
    struct _fbg_gles2_context *gles2_context = fbg->user_context;

    glUseProgram(gles2_context->simple_program);

    glBindBuffer(GL_ARRAY_BUFFER, gles2_context->fbg_vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*) (12 * sizeof(GLfloat)));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gles2_context->fbg_texture);
#ifdef FBG_RGBA
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fbg->width, fbg->height, GL_RGBA, GL_UNSIGNED_BYTE, fbg->disp_buffer);
#else
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fbg->width, fbg->height, GL_RGB, GL_UNSIGNED_BYTE, fbg->disp_buffer);
#endif

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);

    //glFlush();
    //glFinish();
}

void fbg_gles2Flip(struct _fbg *fbg) {
    struct _fbg_gles2_context *gles2_context = fbg->user_context;

    eglSwapBuffers(gles2_context->egl_display, gles2_context->egl_surface);
}

void fbg_gles2Free(struct _fbg *fbg) {
    struct _fbg_gles2_context *gles2_context = fbg->user_context;

    glDeleteTextures(1, &gles2_context->fbg_texture);

    glDeleteBuffers(1, &gles2_context->fbg_vbo);

    glDeleteProgram(gles2_context->simple_program);

    eglDestroyContext(gles2_context->egl_display, gles2_context->egl_context);
    eglDestroySurface(gles2_context->egl_display, gles2_context->egl_surface);
    eglTerminate(gles2_context->egl_display);

#ifndef FBG_RPI
    close(gles2_context->fd);
#endif
}

GLuint fbg_gles2CreateTextureFromImage(struct _fbg_img *img) {
    GLuint texture = fbg_gles2CreateTexture(img->width, img->height);

#ifdef FBG_RGBA
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img->data);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img->width, img->height, 0, GL_RGB, GL_UNSIGNED_BYTE, img->data);
#endif

    return texture;
}

GLuint fbg_gles2CreateTexture(GLuint width, GLuint height) {
    GLuint texture;
    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);
    
#ifdef FBG_RGBA
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
#endif

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return texture;
}

/*
// should be ported ?
GLuint fbg_gles2CreateFBO(GLuint texture) {
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
    
    GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "fbg_gles2reateFBO : glCheckFramebufferStatus failed!\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return fbo;
}
*/
GLuint fbg_gles2CreateVBOvu(GLsizeiptr data_count, const GLvoid *data) {
    GLuint vbo = 0;

    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Vertices + UV
    glBufferData(GL_ARRAY_BUFFER, (data_count + (data_count / 3) * 2) * sizeof(GLfloat), data, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*) (data_count * sizeof(GLfloat)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return vbo;
}

struct _fbg_vbo_data * fbg_gles2CreateVBO(GLsizeiptr indices_count, const GLvoid *indices_data, size_t sizeof_indice_type,
                         GLsizeiptr vertices_count, const GLvoid *vertices_data,
                         GLsizeiptr texcoords_count, const GLvoid *texcoords_data,
                         GLsizeiptr normals_count, const GLvoid *normals_data,
                         GLsizeiptr colors_count, const GLvoid *colors_data) {
    GLuint vbo = 0;
    GLuint tbo = 0;
    GLuint ibo = 0;
    GLuint nbo = 0;
    GLuint cbo = 0;

    glGenBuffers(1, &ibo);

    // Vertices
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices_count * 3 * sizeof(GLfloat), vertices_data, GL_STATIC_DRAW);

    int attrib_id = 1;

    // UVs
    if (texcoords_data) {
        glGenBuffers(1, &tbo);
        glBindBuffer(GL_ARRAY_BUFFER, tbo);
        glBufferData(GL_ARRAY_BUFFER, texcoords_count * 2 * sizeof(GLfloat), texcoords_data, GL_STATIC_DRAW);

        attrib_id += 1;
    }

    // normals
    if (normals_data) {
        glGenBuffers(1, &nbo);
        glBindBuffer(GL_ARRAY_BUFFER, nbo);
        glBufferData(GL_ARRAY_BUFFER, normals_count * 3 * sizeof(GLfloat), normals_data, GL_STATIC_DRAW);

        attrib_id += 1;
    }

    // colors
    if (colors_data) {
        glGenBuffers(1, &cbo);
        glBindBuffer(GL_ARRAY_BUFFER, cbo);
        glBufferData(GL_ARRAY_BUFFER, colors_count * 3 * sizeof(GL_UNSIGNED_BYTE), colors_data, GL_STATIC_DRAW);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_count * sizeof_indice_type, indices_data, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    struct _fbg_vbo_data *vbo_data = (struct _fbg_vbo_data *)calloc(1, sizeof(struct _fbg_vbo_data));

    vbo_data->vbo = vbo;
    vbo_data->tbo = tbo;
    vbo_data->ibo = ibo;
    vbo_data->nbo = nbo;
    vbo_data->cbo = cbo;

    return vbo_data;
}

void fbg_gles2FreeVBOData(struct _fbg_vbo_data *vbo_data) {
    glDeleteBuffers(1, &vbo_data->vbo);
    glDeleteBuffers(1, &vbo_data->tbo);
    glDeleteBuffers(1, &vbo_data->ibo);
    glDeleteBuffers(1, &vbo_data->nbo);
    glDeleteBuffers(1, &vbo_data->cbo);

    free(vbo_data);
}

void fbg_gles2PrintShaderLog(GLuint obj, int type) {
    static char log[16384];

    if (type == 0) {
        glGetProgramInfoLog(obj, 16384, 0, log);
    } else if (type == 1) {
        glGetShaderInfoLog(obj, 16384, 0, log);
    }
    log[16383] = 0;

    fprintf(stderr, "fbg_gles2PrintShaderLog : BEGIN:\n%s\nEND.\n", log);
}

GLuint fbg_gles2CreateShader(GLenum type, const GLchar *source) {
    GLuint shader;
    GLint status;

    shader = glCreateShader(type);

    glShaderSource(shader, 1, (const GLchar**)&source, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status != GL_TRUE) {
        fprintf(stderr, "fbg_gles2CreateShader : Failed to compile shader '%s'!\n", source);

        fbg_gles2PrintShaderLog(shader, 1);

        glDeleteShader(shader);
        
        return 0;
    }

    return shader;
}

GLuint fbg_gles2CreateShaderFromFile(GLenum type, const char *filename) {
    FILE *file = fopen(filename, "rt");
    if (!file) {
        fprintf(stderr, "fbg_gles2CreateShaderFromFile : Failed to open shader file '%s'!\n", filename);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);

    GLchar *source = (GLchar*)malloc(size + 1);

    if (!source) {
        fprintf(stderr, "fbg_gles2CreateShaderFromFile : Malloc failed for '%s'!\n", filename);

        fclose(file);

        return 0;
    }

    fseek(file, 0, SEEK_SET);
    source[fread(source, 1, size, file)] = 0;
    fclose(file);

    GLuint shader = fbg_gles2CreateShader(type, source);
    free(source);

    return shader;
}

GLuint fbg_gles2CreateProgram(GLuint vertex_shader, GLuint fragment_shader) {
    GLuint program = 0;
    GLint status;

    program = glCreateProgram();

    if (vertex_shader) {
        glAttachShader(program, vertex_shader);
    }

    if (fragment_shader) {
        glAttachShader(program, fragment_shader);
    }

    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        fprintf(stderr, "fbg_gles2CreateProgram : Failed to link program!\n");

        fbg_gles2PrintShaderLog(program, 0);

        glDeleteProgram(program);

        return 0;
    }

    if (vertex_shader) {
        glDetachShader(program, vertex_shader);
    }

    if (fragment_shader) {
        glDetachShader(program, fragment_shader);
    }

    return program;
}

GLenum fbg_gles2CreateProgramFromFiles(const char *vs, const char *fs) {
    GLuint id_vs = 0;
    GLuint id_fs = 0;

    if (vs) {
        id_vs = fbg_gles2CreateShaderFromFile(GL_VERTEX_SHADER, vs);
    }

    if (fs) {
        id_fs = fbg_gles2CreateShaderFromFile(GL_FRAGMENT_SHADER, fs);
    }

    GLuint program = fbg_gles2CreateProgram(id_vs, id_fs);

    if (vs) {
        glDeleteShader(id_vs);
    }

    if (fs) {
        glDeleteShader(id_fs);
    }

    return program;
}

GLenum fbg_gles2CreateProgramFromString(const char *vs, const char *fs) {
    GLuint id_vs = 0;
    GLuint id_fs = 0;

    if (vs) {
        id_vs = fbg_gles2CreateShader(GL_VERTEX_SHADER, vs);
    }

    if (fs) {
        id_fs = fbg_gles2CreateShader(GL_FRAGMENT_SHADER, fs);
    }

    GLuint program = fbg_gles2CreateProgram(id_vs, id_fs);

    if (vs) {
        glDeleteShader(id_vs);
    }

    if (fs) {
        glDeleteShader(id_fs);
    }

    return program;
}
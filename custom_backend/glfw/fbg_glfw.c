#include <stdio.h>
#include <stdlib.h>

#include "fbg_glfw.h"

const GLfloat quad[] = { -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, // vertices
						0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f }; // UV

const char *simpleVs = "#version 330\n \
				  layout(location = 0) in vec3 vp; \
				  layout(location = 1) in vec2 vu; \
				  out vec2 uv; \
				  void main() { \
				  	uv = vu; \
				  	gl_Position = vec4(vp, 1.0); \
				  }";

const char *simpleFs = "#version 330\n \
				  in vec2 uv; \
				  uniform sampler2D t0; \
				  void main() { \
				  	gl_FragColor = texture(t0, uv); \
				  }";

void fbg_glfwDraw(struct _fbg *fbg);
void fbg_glfwFree(struct _fbg *fbg);

void GLAPIENTRY fbg_debugGlCb(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam ) {
  fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message);
}


struct _fbg *fbg_glfwSetup(int width, int height, const char *title) {
    struct _fbg_glfw_context *glfw_context = (struct _fbg_glfw_context *)calloc(1, sizeof(struct _fbg_glfw_context));
    if (!glfw_context) {
        fprintf(stderr, "fbg_glfwSetup: glfw context calloc failed!\n");
        return NULL;
    }

    GLFWmonitor *monitor = NULL;
    GLFWwindow *share = NULL;

    if (!glfwInit()) {
        fprintf(stderr, "fbg_glfwSetup : glfwInit failed!\n");
    }

    glfwWindowHint(GLFW_REFRESH_RATE, 60);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow *window = (void *)glfwCreateWindow(width, height, title, monitor, share);
    if (!window) {
        glfwTerminate();

        fprintf(stderr, "fbg_glfwSetup : glfwCreateWindow failed!\n");

		free(glfw_context);

        return NULL;
    }

	glfwMakeContextCurrent(window);

	GLenum err = glewInit();
	if (err != GLEW_OK) {
		fprintf(stderr, "fbg_glfwSetup : glewInit failed '%s'\n", glewGetErrorString(err));

		glfwTerminate();

		free(glfw_context);

		return NULL;
	}

    glfwSwapInterval(1);

	glfw_context->window = window;
	glfw_context->simple_program = fbg_glfwCreateProgramFromString(simpleVs, simpleFs);
	glfw_context->fbg_vao = fbg_glfwCreateVBO(fbg_glfwCreateVAO(), 12, &quad[0]);
	glfw_context->fbg_texture = fbg_glfwCreateTexture(width, height);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1); 

#ifdef DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(fbg_debugGlCb, 0);
#endif

    return fbg_customSetup(width, height, (void *)glfw_context, fbg_glfwDraw, fbg_glfwFree);
}

int fbg_glfwShouldClose(struct _fbg *fbg) {
	struct _fbg_glfw_context *glfw_context = fbg->user_context;
	
    return glfwWindowShouldClose(glfw_context->window);
}

void fbg_glfwDraw(struct _fbg *fbg) {
	struct _fbg_glfw_context *glfw_context = fbg->user_context;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(glfw_context->fbg_vao);
	glUseProgram(glfw_context->simple_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, glfw_context->fbg_texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fbg->width, fbg->height, GL_RGB, GL_UNSIGNED_BYTE, fbg->disp_buffer);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
	glUseProgram(0);

    glfwSwapBuffers(glfw_context->window);

    glfwPollEvents();
}

void fbg_glfwFree(struct _fbg *fbg) {
	struct _fbg_glfw_context *glfw_context = fbg->user_context;

	glDeleteTextures(1, &glfw_context->fbg_texture);

	glDeleteVertexArrays(1, &glfw_context->fbg_vao);

	glDeleteProgram(glfw_context->simple_program);

    glfwTerminate();
}

GLuint fbg_glfwCreateTextureFromImage(struct _fbg_img *img) {
    GLuint texture = fbg_glfwCreateTexture(img->width, img->height);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img->data);

    return texture;
}

GLuint fbg_glfwCreateTexture(GLuint width, GLuint height) {
    GLuint texture;
    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    return texture;
}

GLuint fbg_glfwCreateFBO(GLuint texture) {
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
    
    GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "fbg_glfwCreateFBO : glCheckFramebufferStatus failed!\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return fbo;
}

GLuint fbg_glfwCreateVAO() {
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);

	return vao;
}

GLuint fbg_glfwCreateVBO(GLuint vao, GLsizeiptr data_count, const GLvoid *data) {
	GLuint vbo = 0;

	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// Vertices + UV
	glBufferData(GL_ARRAY_BUFFER, (data_count + (data_count / 3) * 2) * sizeof(GLfloat), data, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*) (data_count * sizeof(GLfloat)));

	glBindVertexArray(0);

	glDeleteBuffers(1, &vbo);

	return vao;
}

void fbg_glfwPrintShaderLog(GLuint obj, int type) {
	static char log[16384];

	if (type == 0) {
		glGetProgramInfoLog(obj, 16384, 0, log);
	} else if (type == 1) {
		glGetShaderInfoLog(obj, 16384, 0, log);
	}
	log[16383] = 0;

	fprintf(stderr, "fbg_glfwPrintShaderLog : BEGIN:\n%s\nEND.\n", log);
}

GLuint fbg_glfwCreateShader(GLenum type, const GLchar *source) {
	GLuint shader;
	GLint status;

	shader = glCreateShader(type);

	glShaderSource(shader, 1, (const GLchar**)&source, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (status != GL_TRUE) {
		fprintf(stderr, "fbg_glfwCreateShader : Failed to compile shader '%s'!\n", source);

		fbg_glfwPrintShaderLog(shader, 1);

		glDeleteShader(shader);
		
        return 0;
	}

	return shader;
}

GLuint fbg_glfwCreateShaderFromFile(GLenum type, const char *filename) {
	FILE *file = fopen(filename, "rt");
	if (!file) {
		fprintf(stderr, "fbg_glfwCreateShaderFromFile : Failed to open shader file '%s'!\n", filename);
		return 0;
	}

	fseek(file, 0, SEEK_END);
	long size = ftell(file);

	GLchar *source = (GLchar*)malloc(size + 1);

	if (!source) {
        fprintf(stderr, "fbg_glfwCreateShaderFromFile : Malloc failed for '%s'!\n", filename);

		fclose(file);

		return 0;
	}

	fseek(file, 0, SEEK_SET);
	source[fread(source, 1, size, file)] = 0;
	fclose(file);

	GLuint shader = fbg_glfwCreateShader(type, source);
	free(source);

	return shader;
}

GLuint fbg_glfwCreateProgram(GLuint vertex_shader, GLuint fragment_shader) {
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
		fprintf(stderr, "fbg_glfwCreateProgram : Failed to link program!\n");

		fbg_glfwPrintShaderLog(program, 0);

		glDeleteProgram(program);

		return 0;
	}

	glDetachShader(program, vertex_shader);
	glDetachShader(program, fragment_shader);

	return program;
}

GLenum fbg_glfwCreateProgramFromFiles(const char *vs, const char *fs) {
	GLuint id_vs = 0;
	GLuint id_fs = 0;

	if (vs) {
		id_vs = fbg_glfwCreateShaderFromFile(GL_VERTEX_SHADER, vs);
	}

	if (fs) {
		id_fs = fbg_glfwCreateShaderFromFile(GL_FRAGMENT_SHADER, fs);
	}

	GLuint program = fbg_glfwCreateProgram(id_vs, id_fs);

	glDeleteShader(id_vs);
	glDeleteShader(id_fs);

	return program;
}

GLenum fbg_glfwCreateProgramFromString(const char *vs, const char *fs) {
	GLuint id_vs = 0;
	GLuint id_fs = 0;

	if (vs) {
		id_vs = fbg_glfwCreateShader(GL_VERTEX_SHADER, vs);
	}

	if (fs) {
		id_fs = fbg_glfwCreateShader(GL_FRAGMENT_SHADER, fs);
	}

	GLuint program = fbg_glfwCreateProgram(id_vs, id_fs);

	glDeleteShader(id_vs);
	glDeleteShader(id_fs);

	return program;
}
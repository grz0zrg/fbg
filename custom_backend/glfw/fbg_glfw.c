#include <stdio.h>
#include <stdlib.h>

#include "fbg_glfw.h"

const GLfloat fbg_glfwQuad[] = { -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, // vertices
						0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f }; // UV

const char *fbg_glfwSimpleVs = "#version 330\n \
				  layout(location = 0) in vec3 vp; \
				  layout(location = 1) in vec2 vu; \
				  out vec2 uv; \
				  void main() { \
				  	uv = vu; \
				  	gl_Position = vec4(vp, 1.0); \
				  }";

const char *fbg_glfwSimpleFs = "#version 330\n \
				  in vec2 uv; \
				  out vec4 final_color; \
				  uniform sampler2D t0; \
				  void main() { \
				  	final_color = texture(t0, uv); \
				  }";

const char *fbg_glfwDownsampleFs = "#version 330\n \
				  in vec2 uv; \
				  out vec4 final_color; \
				  uniform sampler2D t0; \
				  uniform int ssaa; \
				  void main() { \
    				vec3 accum = vec3(0., 0., 0.); \
					vec2 target_res = textureSize(t0, 0) / ssaa; \
    				float x_subpix_off = 1.0 / (target_res.x * float(ssaa)); \
    				float y_subpix_off = 1.0 / (target_res.y * float(ssaa)); \
					for (int i=0; i < ssaa; i++) { \
						for (int j=0; j < ssaa; j++) { \
							vec2 sample_uv = vec2(uv.x + float(i) * x_subpix_off, uv.y + float(j) * y_subpix_off); \
							accum += texture2D(t0, sample_uv).rgb; \
						} \
					} \
    				vec3 final = accum / (floor(float(ssaa)) * floor(float(ssaa))); \
				  	final_color = vec4(final, 0.); \
				  }";

struct _fbg **fbg_contexts = NULL;
int fbg_contexts_count = 0;

void fbg_glfwDraw(struct _fbg *fbg);
void fbg_glfwFlip(struct _fbg *fbg);
void fbg_glfwFree(struct _fbg *fbg);

void GLAPIENTRY fbg_debugGlCb(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam ) {
  fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message);
}

void fbg_glfwFramebufferResizeCb(GLFWwindow* window, int new_width, int new_height) {
	int i = 0;
	for (i = 0; i < fbg_contexts_count; i += 1) {
		struct _fbg *fbg = fbg_contexts[i];
		struct _fbg_glfw_context *glfw_context = fbg->user_context;
		if (glfw_context->window == window) {
			fbg_pushResize(fbg, new_width * glfw_context->ssaa, new_height * glfw_context->ssaa);
			// called from fbg_resize
			//fbg_glfwResize(fbg, new_width, new_height);

			break;
		}
	}
}

struct _fbg *fbg_glfwSetup(int width, int height, int components, const char *title, int monitor_id, int fullscreen, int ssaa) {
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

	int monitor_count = 0;
	int i = 0;
	GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);
	for (i = 0; i < monitor_count; i += 1) {
		if (monitor_id == i) {
			monitor = monitors[i];
			break;
		}
	}

	const GLFWvidmode *window_mode = glfwGetVideoMode(monitor);

	if (fullscreen == 0) {
		monitor = NULL;
	} else if (fullscreen == 2) {
		width = window_mode->width;
		height = window_mode->height;
	}

    GLFWwindow *window = (void *)glfwCreateWindow(width, height, title, monitor, share);
    if (!window) {
        glfwTerminate();

        fprintf(stderr, "fbg_glfwSetup : glfwCreateWindow failed!\n");

		free(glfw_context);

        return NULL;
    }

	if (fullscreen == 1) {
		glfwSetWindowMonitor(window, monitor, 0, 0, width, height, window_mode->refreshRate);
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

#ifdef DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(fbg_debugGlCb, 0);
#endif

	glfw_context->window = window;
	glfw_context->monitor = monitor;
	glfw_context->simple_program = fbg_glfwCreateProgramFromString(fbg_glfwSimpleVs, ssaa > 1 ? fbg_glfwDownsampleFs : fbg_glfwSimpleFs, 0);
	glfw_context->fbg_vao = fbg_glfwCreateVAOvu(12, &fbg_glfwQuad[0]);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1); 

	glfw_context->width = width;
	glfw_context->height = height;

	int fbg_width = width;
	int fbg_height = height;

	if (ssaa > 1) {
		fbg_width *= ssaa;
		fbg_height *= ssaa;

		glfw_context->ssaa = ssaa;
	} else {
		glfw_context->ssaa = 1;
	}

	glUseProgram(glfw_context->simple_program);
	GLint ssaa_location = glGetUniformLocation(glfw_context->simple_program, "ssaa");
	glUniform1i(ssaa_location, glfw_context->ssaa);

	struct _fbg *fbg = fbg_customSetup(fbg_width, fbg_height, components, 1, 1, (void *)glfw_context, fbg_glfwDraw, fbg_glfwFlip, fbg_glfwResize, fbg_glfwFree);

	glfw_context->fbg_texture = fbg_glfwCreateTexture(fbg_width, fbg_height, fbg->components == 4 ? GL_RGBA : GL_RGB);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	fbg_contexts_count += 1;

	// we keep track of fbg contexts globally due to registered callbacks (resize etc.)
	if (fbg_contexts == NULL) {
		fbg_contexts = (struct _fbg **)calloc(1, sizeof(struct _fbg *));
		if (!fbg_contexts) {
			fprintf(stderr, "fbg_glfwSetup: fbg_contexts calloc failed!\n");
		} else {
			fbg_contexts[0] = fbg;
		}
	} else {
		fbg_contexts = (struct _fbg **)realloc(fbg_contexts, sizeof(struct _fbg *) * fbg_contexts_count);
		if (!fbg_contexts) {
			fprintf(stderr, "fbg_glfwSetup: fbg_contexts realloc failed!\n");
		} else {
			fbg_contexts[fbg_contexts_count - 1] = fbg;
		}
	}

	glfwSetFramebufferSizeCallback(window, fbg_glfwFramebufferResizeCb);

    return fbg;
}

void fbg_glfwFullscreen(struct _fbg *fbg, int enable) {
	struct _fbg_glfw_context *glfw_context = fbg->user_context;

	const GLFWvidmode *window_mode = glfwGetVideoMode(glfw_context->monitor);
	if (enable) {
		glfwSetWindowMonitor(glfw_context->window, glfw_context->monitor, 0, 0, window_mode->width, window_mode->height, window_mode->refreshRate);
	} else {
		glfwSetWindowMonitor(glfw_context->window, 0, 0, 0, window_mode->width, window_mode->height, window_mode->refreshRate);
	}
}

void fbg_glfwResize(struct _fbg *fbg, unsigned int new_width, unsigned new_height) {
	struct _fbg_glfw_context *glfw_context = fbg->user_context;

	// we keep a copy of user defined parameters for the fbg_texture (in case it changed)
	GLint mag_filter, min_filter, swrap_mode, twrap_mode;

	glBindTexture(GL_TEXTURE_2D, glfw_context->fbg_texture);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &mag_filter);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &min_filter); 
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &swrap_mode);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &twrap_mode); 

	glDeleteTextures(1, &glfw_context->fbg_texture);

	glfw_context->fbg_texture = fbg_glfwCreateTexture(new_width, new_height, fbg->components == 4 ? GL_RGBA : GL_RGB);

	// and we restore its user defined parameters again (if any)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, swrap_mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, twrap_mode);

	glViewport(0, 0, new_width / glfw_context->ssaa, new_height / glfw_context->ssaa);
}

int fbg_glfwShouldClose(struct _fbg *fbg) {
	struct _fbg_glfw_context *glfw_context = fbg->user_context;
	
    return glfwWindowShouldClose(glfw_context->window);
}

void fbg_glfwUpdateBuffer(struct _fbg *fbg) {
	if (fbg->components == 4) {
		glReadPixels(0, 0, fbg->width, fbg->height, GL_RGBA, GL_UNSIGNED_BYTE, fbg->back_buffer);
	} else if (fbg->components == 3) {
		glReadPixels(0, 0, fbg->width, fbg->height, GL_RGB, GL_UNSIGNED_BYTE, fbg->back_buffer);
	}
}

void fbg_glfwClear() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void fbg_glfwDraw(struct _fbg *fbg) {
	struct _fbg_glfw_context *glfw_context = fbg->user_context;

	glBindVertexArray(glfw_context->fbg_vao);
	glUseProgram(glfw_context->simple_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, glfw_context->fbg_texture);

	if (fbg->components == 4) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fbg->width, fbg->height, GL_RGBA, GL_UNSIGNED_BYTE, fbg->back_buffer);
	} else if (fbg->components == 3) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fbg->width, fbg->height, GL_RGB, GL_UNSIGNED_BYTE, fbg->back_buffer);
	}

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
	glUseProgram(0);
}

void fbg_glfwFlip(struct _fbg *fbg) {
	struct _fbg_glfw_context *glfw_context = fbg->user_context;

    glfwSwapBuffers(glfw_context->window);

    glfwPollEvents();
}

void fbg_glfwFree(struct _fbg *fbg) {
	struct _fbg_glfw_context *glfw_context = fbg->user_context;

	// remove the fbg context in our own contexts list
	struct _fbg **fbg_contexts_tmp = NULL;

	fbg_contexts_count -= 1;
	if (fbg_contexts_count == 0) {
		free(fbg_contexts);
		fbg_contexts = NULL;
	} else {
		fbg_contexts_tmp = (struct _fbg **)calloc(fbg_contexts_count, sizeof(struct _fbg *));
		if (!fbg_contexts) {
			fprintf(stderr, "fbg_glfwFree: fbg_contexts calloc failed!\n");
		}
	}

	int i = 0, k = 0;
	for (i = 0; i < fbg_contexts_count; i += 1) {
		struct _fbg *ctx = fbg_contexts[i];
		if (ctx != fbg) {
			fbg_contexts_tmp[k] = ctx;

			k += 1;
		}
	}

	free(fbg_contexts);
	fbg_contexts = fbg_contexts_tmp;
	//

	glDeleteTextures(1, &glfw_context->fbg_texture);

	glDeleteVertexArrays(1, &glfw_context->fbg_vao);

	glDeleteProgram(glfw_context->simple_program);

    glfwTerminate();

	free(glfw_context);
}

GLuint fbg_glfwCreateTextureFromImage(struct _fbg *fbg, struct _fbg_img *img) {
    GLuint texture = fbg_glfwCreateTexture(img->width, img->height, fbg->components == 4 ? GL_RGBA : GL_RGB);

	if (fbg->components == 4) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img->data);
	} else if (fbg->components == 3) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img->width, img->height, 0, GL_RGB, GL_UNSIGNED_BYTE, img->data);
	}

    return texture;
}

GLuint fbg_glfwCreateTexture(GLuint width, GLuint height, GLint internal_format) {
    GLuint texture;
    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, internal_format, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

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

GLuint fbg_glfwCreateVAOvu(GLsizeiptr data_count, const GLvoid *data) {
	GLuint vbo = 0;
	GLuint vao = 0;

	glGenVertexArrays(1, &vao);
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

GLuint fbg_glfwCreateVAO(GLsizeiptr indices_count, const GLvoid *indices_data, size_t sizeof_indice_type,
						 GLsizeiptr vertices_count, const GLvoid *vertices_data,
						 GLsizeiptr texcoords_count, const GLvoid *texcoords_data,
						 GLsizeiptr normals_count, const GLvoid *normals_data,
						 GLsizeiptr colors_count, const GLvoid *colors_data) {
	GLuint vbo = 0;
	GLuint tbo = 0;
	GLuint ibo = 0;
	GLuint nbo = 0;
	GLuint cbo = 0;
	GLuint vao = 0;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &ibo);

	// Vertices
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices_count * 3 * sizeof(GLfloat), vertices_data, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glEnableVertexAttribArray(0);

	int attrib_id = 1;

	// UVs
	if (texcoords_data) {
		glGenBuffers(1, &tbo);
		glBindBuffer(GL_ARRAY_BUFFER, tbo);
		glBufferData(GL_ARRAY_BUFFER, texcoords_count * 2 * sizeof(GLfloat), texcoords_data, GL_STATIC_DRAW);

		glEnableVertexAttribArray(attrib_id);
		glVertexAttribPointer(attrib_id, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*) 0);

		attrib_id += 1;
	}

	// normals
	if (normals_data) {
		glGenBuffers(1, &nbo);
		glBindBuffer(GL_ARRAY_BUFFER, nbo);
		glBufferData(GL_ARRAY_BUFFER, normals_count * 3 * sizeof(GLfloat), normals_data, GL_STATIC_DRAW);

		glEnableVertexAttribArray(attrib_id);
		glVertexAttribPointer(attrib_id, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*) 0);

		attrib_id += 1;
	}

	// colors
	if (colors_data) {
		glGenBuffers(1, &cbo);
		glBindBuffer(GL_ARRAY_BUFFER, cbo);
		glBufferData(GL_ARRAY_BUFFER, colors_count * 3 * sizeof(GL_UNSIGNED_BYTE), colors_data, GL_STATIC_DRAW);

		glEnableVertexAttribArray(attrib_id);
		glVertexAttribPointer(attrib_id, 3, GL_UNSIGNED_BYTE, GL_FALSE, 0, (GLvoid*) 0);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_count * sizeof_indice_type, indices_data, GL_STATIC_DRAW);

	glBindVertexArray(0);

	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &tbo);
	glDeleteBuffers(1, &ibo);
	glDeleteBuffers(1, &nbo);
	glDeleteBuffers(1, &cbo);

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

GLuint fbg_glfwCreateProgram(GLuint vertex_shader, GLuint fragment_shader, GLuint geometry_shader) {
	GLuint program = 0;
	GLint status;

	program = glCreateProgram();

	if (vertex_shader) {
		glAttachShader(program, vertex_shader);
    }

	if (fragment_shader) {
		glAttachShader(program, fragment_shader);
    }

	if (geometry_shader) {
		glAttachShader(program, geometry_shader);
	}

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		fprintf(stderr, "fbg_glfwCreateProgram : Failed to link program!\n");

		fbg_glfwPrintShaderLog(program, 0);

		glDeleteProgram(program);

		return 0;
	}

	if (vertex_shader) {
		glDetachShader(program, vertex_shader);
	}

	if (fragment_shader) {
		glDetachShader(program, fragment_shader);
	}

	if (geometry_shader) {
		glDetachShader(program, geometry_shader);
	}

	return program;
}

GLenum fbg_glfwCreateProgramFromFiles(const char *vs, const char *fs, const char *gs) {
	GLuint id_vs = 0;
	GLuint id_fs = 0;
	GLuint id_gs = 0;

	if (vs) {
		id_vs = fbg_glfwCreateShaderFromFile(GL_VERTEX_SHADER, vs);
	}

	if (fs) {
		id_fs = fbg_glfwCreateShaderFromFile(GL_FRAGMENT_SHADER, fs);
	}

	if (gs) {
		id_gs = fbg_glfwCreateShaderFromFile(GL_GEOMETRY_SHADER, gs);
	}

	GLuint program = fbg_glfwCreateProgram(id_vs, id_fs, id_gs);

	if (vs) {
		glDeleteShader(id_vs);
	}

	if (fs) {
		glDeleteShader(id_fs);
	}

	if (gs) {
		glDeleteShader(id_gs);
	}

	return program;
}

GLenum fbg_glfwCreateProgramFromString(const char *vs, const char *fs, const char *gs) {
	GLuint id_vs = 0;
	GLuint id_fs = 0;
	GLuint id_gs = 0;

	if (vs) {
		id_vs = fbg_glfwCreateShader(GL_VERTEX_SHADER, vs);
	}

	if (fs) {
		id_fs = fbg_glfwCreateShader(GL_FRAGMENT_SHADER, fs);
	}

	if (gs) {
		id_gs = fbg_glfwCreateShader(GL_GEOMETRY_SHADER, gs);
	}

	GLuint program = fbg_glfwCreateProgram(id_vs, id_fs, id_gs);

	if (vs) {
		glDeleteShader(id_vs);
	}

	if (fs) {
		glDeleteShader(id_fs);
	}

	if (gs) {
		glDeleteShader(id_gs);
	}

	return program;
}
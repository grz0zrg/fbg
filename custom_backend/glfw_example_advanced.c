/*
 Advanced FBG GLFW example

 FBG HSL feedback effect from FBG content + standard OpenGL content.

 Also show how to override the fragment & vertex shader used to draw the FBG content.

 This use the cglm library for all the 3D math https://github.com/recp/cglm
 This use the cwobj library to load .obj files https://github.com/grz0zrg/cwobj
*/

#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>

#include <cglm/cglm.h>

#include "glfw/fbg_glfw.h"
#include "cwobj/cwobj.h"

#define pcount (2048 * 4)

float points[pcount];

mat4 proj;

struct _fbg_img *img = NULL;
GLint ires_loc;

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

void generatePoints(struct _fbg *fbg) {
    for (int i = 0; i < pcount; i += 4) {
        float vx = fbg_randf(-1, 1);
        float vy = fbg_randf(-0.005, 0.005);

        points[i] = fbg_randf(vx, fbg->width - vx);
        points[i + 1] = fbg_randf(vy, fbg->height - vy);

        points[i + 2] = vx;
        points[i + 3] = vy;
    }
}

void onResize(struct _fbg *fbg, unsigned int new_width, unsigned int new_height) {
    fbg_freeImage(img);
    img = fbg_createImage(fbg, new_width, new_height);

    struct _fbg_glfw_context *glfw_context = fbg->user_context;

    glUseProgram(glfw_context->simple_program);
    glUniform2f(ires_loc, new_width, new_height);

    // update proj matrix
    glm_perspective(glm_rad(45.0f), (float)new_width / (float)new_height, 0.1f, 100.0f, proj);

    generatePoints(fbg);
}

int main(int argc, char* argv[]) {
    struct _fbg *fbg = fbg_glfwSetup(800, 600, 3, "glfw example", 0, 0);
    if (fbg == NULL) {
        return 0;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);

    srand((unsigned int)time(NULL));

    struct _fbg_img *bb_font_img = fbg_loadPNG(fbg, "../examples/bbmode1_8x8.png");
    struct _fbg_img *bone_image = fbg_loadPNG(fbg, "bone.png");

    GLuint bone_texture = fbg_glfwCreateTextureFromImage(fbg, bone_image);
    glBindTexture(GL_TEXTURE_2D, bone_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    struct _fbg_font *bbfont = fbg_createFont(fbg, bb_font_img, 8, 8, 33);

    img = fbg_createImage(fbg, fbg->width, fbg->height);

    signal(SIGINT, int_handler);

    generatePoints(fbg);

    float motion = 0;

    struct _fbg_rgb color, color2;

    // we must resize the image upon a FBG resize event since we render the screen into it, we must also update the proj matrix and update some uniforms
    fbg_setResizeCallback(fbg, onResize);

    // override the internal fragment & vertex shader used to draw the FBG buffer
    struct _fbg_glfw_context *glfw_context = fbg->user_context;
    glDeleteProgram(glfw_context->simple_program);
    glfw_context->simple_program = fbg_glfwCreateProgramFromFiles("advanced.vert", "advanced.frag", NULL);

    // we bind some uniforms
    glUseProgram(glfw_context->simple_program);
    GLint itime_loc = glGetUniformLocation(glfw_context->simple_program, "iTime");
    GLint imouse_loc = glGetUniformLocation(glfw_context->simple_program, "iMouse");
    ires_loc = glGetUniformLocation(glfw_context->simple_program, "iResolution");
    glUniform1f(itime_loc, 0);
    glUniform2f(ires_loc, fbg->width, fbg->height);

    // we change the FBG texture to linear
    glBindTexture(GL_TEXTURE_2D, glfw_context->fbg_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    // we load a cube as a .obj file (this is handled by the cwobj library which is a small single C header obj library)
    cwobj *cube_mesh = cwobj_load("bad_skull.obj", NULL);
    cwobj_geo *cube_geo = cube_mesh->geometry;
    GLuint cube_vao = fbg_glfwCreateVAO(cube_geo->indice_n, &cube_geo->indice[0], sizeof(CWOBJ_INDICE_TYPE),
                                        cube_geo->vertice_n, &cube_geo->vertice[0],
                                        cube_geo->texcoord_n, &cube_geo->texcoord[0],
                                        cube_geo->normal_n, &cube_geo->normal[0],
                                        cube_geo->color_n, &cube_geo->color[0]);

    // load a simple shader to handle our cube
    GLuint program_3d = fbg_glfwCreateProgramFromFiles("advanced_3d.vert", "advanced_3d.frag", NULL);
    glUseProgram(program_3d);
    GLint m_loc = glGetUniformLocation(program_3d, "m");
    GLint v_loc = glGetUniformLocation(program_3d, "v");
    GLint p_loc = glGetUniformLocation(program_3d, "p");

    GLuint texLoc = glGetUniformLocation(program_3d, "t0");
    glUniform1i(texLoc, 0);
    texLoc = glGetUniformLocation(program_3d, "t1");
    glUniform1i(texLoc, 1);

    // setup regular MVP for 3D stuff
    mat4 view, model;

    glm_perspective(glm_rad(45.0f), (float)fbg->width / (float)fbg->height, 0.1f, 100.0f, proj);
    glm_lookat((vec3){0.0f, 0.0f, 12.0f}, (vec3){0.0f, 0.0f, 0.0f}, (vec3){0.0f, 1.0f, 0.0f}, view);
    glm_mat4_identity(model);

    do {
        //fbg_clear(fbg, 0);
        fbg_glfwClear();

        fbg_image(fbg, img, 0, 0);

        fbg_drawInto(fbg, img->data);

        fbg_fadeDown(fbg, 1);

        int c = 0;
        for (int i = 0; i < pcount; i += 4) {
            float x = points[i];
            float y = points[i + 1];

            float vx = points[i + 2];
            float vy = points[i + 3];

            fbg_hslToRGB(&color, abs(sin(x / (float)fbg->width * 3.1415 / 2) * 360), fbg_randf(0.5f, 1), 0.5f);

            fbg_getPixel(fbg, x + fbg_randf(-1, 1), y + fbg_randf(-1, 1), &color2);

            fbg_recta(fbg, x, y, 1, 1, (color.r + color2.r) / 2, (color.g + color2.g) / 2, (color.b + color2.b) / 2, i % 255);

            points[i] += vx;
            points[i + 1] += vy;

            x = points[i];
            y = points[i + 1];

            if (x <= 0) {
                points[i] = fbg->width - 1;
            }

            if (x >= fbg->width - 1) {
                points[i] = 0;
            }

            if (y <= 0) {
                points[i + 1] = fbg->height - 1;
            }

            if (y >= fbg->height - 1) {
                points[i + 1] = 0;
            }

            c += 1;
        }

        fbg_drawInto(fbg, NULL);

        fbg_write(fbg, "FPS:", 4, 2);
        fbg_write(fbg, fbg->fps_char, 32 + 8, 2);

        // show the cube over the FBG content
        glDisable(GL_DEPTH_TEST);

        //glEnable(GL_BLEND);  
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA);

        fbg_draw(fbg);

        // OpenGL stuff
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        glBindVertexArray(cube_vao);
        glUseProgram(program_3d);

        float scale_motion = 2.75 + fabsf(sin(motion / 24.) / 8.) * 28.;

        glm_translate_make(model, (vec3){0., 0., scale_motion});
        glm_rotate(model, sin(motion / 8.), (vec3){0., 1., 0.25});
        glm_rotate(model, cos(motion / 14.) / 2., (vec3){1., 1., 1.});

        //glm_scale(model, (vec3){scale_motion, scale_motion, scale_motion});
        

        // upload M V P as separate components
        //glm_mat4_mulN((mat4 *[]){&proj, &view}, 2, vp);
        glUniformMatrix4fv(v_loc, 1, GL_FALSE, (float *)view);
        glUniformMatrix4fv(p_loc, 1, GL_FALSE, (float *)proj);
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, (float *)model);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, glfw_context->fbg_texture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bone_texture);
        glDrawElements(GL_TRIANGLES, cube_geo->indice_n, GL_UNSIGNED_INT, (void*)0);

        glBindVertexArray(0);
        glUseProgram(0);
        //

        fbg_flip(fbg);

        motion += 0.5f;

        // update our uniforms
        glUseProgram(glfw_context->simple_program);
        glUniform1f(itime_loc, glfwGetTime());

        double xpos, ypos;
        glfwGetCursorPos(glfw_context->window, &xpos, &ypos);
        glUniform4f(imouse_loc, xpos, ypos, 0, 0);

    } while (keep_running && !fbg_glfwShouldClose(fbg));

    cwobj_free(cube_mesh);

    glDeleteProgram(program_3d);

    glDeleteVertexArrays(1, &cube_vao);

    glDeleteTextures(1, &bone_texture);

    fbg_freeImage(img);
    fbg_freeImage(bone_image);
    fbg_freeImage(bb_font_img);
    fbg_freeFont(bbfont);

    fbg_close(fbg);
}

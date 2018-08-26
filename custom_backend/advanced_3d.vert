#version 330

layout(location = 0) in vec3 vp;
layout(location = 1) in vec2 vu;
layout(location = 2) in vec3 vn;

out vec2 uv;
out vec3 normal;
out vec3 fpos;
out vec3 lpos;

vec3 lightPos = vec3(0., 0., 0.5);

uniform mat4 m, v, p;
uniform mat4 vp_mat;

void main() {
    uv = vu;

    normal = mat3(transpose(inverse(v * m))) * vn;
    fpos = vec3(v * m * vec4(vp, 1.0));

    lpos = vec3(v * vec4(lightPos, 1.0));

    gl_Position = (p * v * m) * vec4(vp, 1.0);
}
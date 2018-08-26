#version 330

layout(location = 0) in vec3 vp;
layout(location = 1) in vec2 vu;
out vec2 uv;
void main() {
    uv = vu;
    gl_Position = vec4(vp, 1.0);
}
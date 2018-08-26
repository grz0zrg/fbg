#version 330

uniform vec2 iResolution;
uniform float iTime;
uniform vec4 iMouse;

in vec2 uv;
out vec4 final_color;
uniform sampler2D t0;
void main() {
    // normalized coordinates (-1 to 1 vertically)
    vec2 p = (-iResolution.xy + 2.0*gl_FragCoord.xy)/iResolution.y;

    // angle of each pixel to the center of the screen
    float a = atan(p.y, p.x);

    float e = iMouse.x / iResolution.x * 4;
    float r = pow( pow(p.x*p.x,e) + pow(p.y*p.y,e), 1.0/(2.*e) );
    
    // index texture by (animated inverse) radious and angle
    vec2 uv = vec2( 0.3/r + 0.4*iTime, a/3.1415927 );

    // fetch color with correct texture gradients, to prevent discontinutity
    vec2 uv2 = vec2( uv.x, atan(p.y,abs(p.x))/3.1415927 );
    vec3 col = textureGrad( t0, uv, dFdx(uv2), dFdy(uv2) ).xyz;
    
    // do some weird effect based on distance
    col = col*( (0.5 - abs(0.75 - r)) * 2) * 16.;

	final_color = vec4(col, 1.0);
}
#version 330

in vec2 uv;
in vec3 normal;
in vec3 fpos;
in vec3 lpos;

out vec4 final_color;
uniform sampler2D t0;
uniform sampler2D t1;

vec3 viewPos = vec3(0.0f, 0.0f, 12.0f);
vec3 lightCol = vec3(1., 1., 1.);

void main() {
	float ambient_strength = 0.005;
	vec3 ambient = ambient_strength * lightCol;

	vec3 lightDir = normalize(lpos - fpos);

	vec3 norm = normalize(normal);

	float diffuse = max(dot(norm, lightDir), 0.);

	vec3 viewDir = normalize(-fpos);
	//vec3 reflectDir = reflect(-lightDir, norm);

	float specularStrength = 0.75;
	int specularShininess = 8;

	vec3 halfwayDir = normalize(lightDir + viewDir);

	float spec = pow(max(dot(normal, halfwayDir), 0.0), specularShininess);
	vec3 specular = specularStrength * spec * lightCol;

	vec3 shade_result = ambient + diffuse + specular;

	vec4 texture1 = texture(t0, uv * 4.);

	vec4 c = (texture1 * 12. + ((1. - texture1 / 8.) - texture(t1, uv * 4.)) * 0.05) * vec4(shade_result * 8., 1.);

	float gamma = 2.2;
	final_color.rgb = pow(c.rgb, vec3(1.0 / gamma));
	final_color.a = 1.;
}
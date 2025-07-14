// fs_model.sc

$input v_normal, v_texcoord0

#include <bgfx_shader.sh>

void main()
{
	// Simple lighting with normal
	vec3 lightDir = normalize(vec3(0.0, 1.0, 1.0));
	float diffuse = max(0.0, dot(v_normal, lightDir));
	gl_FragColor = vec4(diffuse * vec3(1.0, 1.0, 1.0), 1.0);
}

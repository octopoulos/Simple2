// fs_model_instance.sc

$input v_normal, v_texcoord0

#include <bgfx_shader.sh>

uniform vec4 u_lightDir;

void main()
{
    float diffuse = max(0.0, dot(v_normal, u_lightDir.xyz));
	gl_FragColor = vec4(diffuse * vec3(1.0, 1.0, 1.0), 1.0);
}

// fs_model_texture.sc

$input v_normal, v_texcoord0

#include "common.sh"

SAMPLER2D(s_texColor, 0);

uniform vec4 u_lightDir;

void main()
{
	float ambient = 0.15;
	float diffuse = max(0.0, dot(v_normal, u_lightDir.xyz));
	float lambert = mix(ambient, 1.0, diffuse);

	vec4 color = toLinear(texture2D(s_texColor, v_texcoord0));

	gl_FragColor = vec4(color.xyz * lambert, 1.0);
}

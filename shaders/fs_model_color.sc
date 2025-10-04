// fs_model_color.sc

$input v_normal, v_color0

#include "common.sh"

uniform vec4 u_lightDir;

void main()
{
	float ambient = 0.25;
	float diffuse = max(0.0, dot(v_normal, u_lightDir.xyz));
	float lambert = mix(ambient, 1.0, diffuse);

	vec4 color   = toLinear(v_color0);
	gl_FragColor = vec4(color.rgb * lambert, color.a);
}

// fs_rubik.sc

$input v_normal, v_color0

#include "common.sh"

uniform vec4 u_lightDir;

void main()
{
	// Use vertex normal for lighting
	vec3 finalNormal = normalize(v_normal);

	float ambient = 0.5; // higher ambient for vibrant colors
	float diffuse = max(0.0, dot(finalNormal, normalize(u_lightDir.xyz)));
	float lambert = mix(ambient, 1.0, diffuse);

	// Use vertex color
	vec4 color = toLinear(v_color0);

	gl_FragColor = vec4(color.rgb * lambert, color.a);
}

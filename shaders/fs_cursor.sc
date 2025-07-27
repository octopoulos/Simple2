// fs_cursor.sc

$input v_normal
uniform vec4 u_time;

#include <bgfx_shader.sh>

void main()
{
	vec3 normal = normalize(v_normal);
	vec3 viewDir = vec3(0.0, 0.0, 1.0); // view-space forward

	float edge = dot(normal, viewDir);
	edge = abs(edge); // 1 = face-on, 0 = edge-on

	float glow = smoothstep(0.0, 0.2, 1.0 - edge); // highlight contour
	float pulse = abs(sin(u_time.x * 4.0));
	vec3 color = vec3(1.0, 1.0, 0.2) * pulse;

	gl_FragColor = vec4(color, glow * 0.8); // translucent edge glow
}

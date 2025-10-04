// vs_cursor.sc

$input a_position, a_normal
$output v_position, v_normal

#include <bgfx_shader.sh>

void main()
{
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));

	v_normal   = a_normal;
	v_position = mul(u_modelView, vec4(a_position, 1.0)).xyz;
}

// vs_cursor_instance.sc

$input a_position, a_normal, i_data0, i_data1, i_data2, i_data3, i_data4
$output v_position, v_normal

#include <bgfx_shader.sh>

void main()
{
	mat4 model    = mtxFromCols(i_data0, i_data1, i_data2, i_data3);
	vec4 worldPos = mul(model, vec4(a_position, 1.0));

	gl_Position = mul(u_modelViewProj, worldPos);
	v_normal    = a_normal;
	v_position  = mul(u_modelView, vec4(a_position, 1.0)).xyz;
}

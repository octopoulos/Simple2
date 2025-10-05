// vs_model_instance.sc

$input a_position, a_normal, a_texcoord0, i_data0, i_data1, i_data2, i_data3, i_data4
$output v_normal, v_texcoord0

#include <bgfx_shader.sh>

void main()
{
	mat4 model    = mtxFromCols(i_data0, i_data1, i_data2, i_data3);
	vec4 worldPos = mul(model, vec4(a_position, 1.0));

	gl_Position = mul(u_viewProj, worldPos);
	v_normal    = a_normal;
	v_texcoord0 = a_texcoord0;
}

// vs_model_color.sc

$input a_position, a_normal, a_color0
$output v_normal, v_color0

#include "common.sh"

void main()
{
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_normal = a_normal;
    v_color0 = a_color0;
}

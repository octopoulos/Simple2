// vs_model_texture_tangent.sc

$input a_position, a_normal, a_tangent, a_texcoord0
$output v_position, v_normal, v_tangent, v_bitangent, v_texcoord0

#include "common.sh"

void main()
{
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
	v_position  = mul(u_modelView, vec4(a_position, 1.0)).xyz; // World-space position
	v_normal    = normalize(mul(u_modelView, vec4(a_normal, 0.0)).xyz); // World-space normal
	v_tangent   = normalize(mul(u_modelView, vec4(a_tangent.xyz, 0.0)).xyz); // World-space tangent
	v_bitangent = normalize(cross(v_normal, v_tangent) * a_tangent.w); // Apply handedness
	v_texcoord0 = a_texcoord0;
}

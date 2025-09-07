// vs_pbr.sc

$input a_position, a_normal, a_tangent, a_texcoord0
$output v_position, v_normal, v_tangent, v_bitangent, v_texcoord0

#include "common.sh"

void main()
{
	// Transform position to world space
	vec3 worldPos = mul(u_model[0], vec4(a_position, 1.0)).xyz;
	gl_Position   = mul(u_viewProj, vec4(worldPos, 1.0));

	// Transform normal and tangent to world space
	v_normal    = normalize(mul(u_model[0], vec4(a_normal, 0.0)).xyz);
	v_tangent   = normalize(mul(u_model[0], vec4(a_tangent.xyz, 0.0)).xyz);
	v_bitangent = cross(v_normal, v_tangent) * a_tangent.w; // Handedness from tangent.w
	v_position  = worldPos;
	v_texcoord0 = a_texcoord0;
}

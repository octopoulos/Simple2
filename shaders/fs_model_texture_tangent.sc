// fs_model_texture_tangent.sc

$input v_position, v_normal, v_tangent, v_bitangent, v_texcoord0

#include "common.sh"

SAMPLER2D(s_texColor, 0);
SAMPLER2D(s_texNormal, 1);

uniform vec4 u_lightDir;
uniform vec4 u_viewPos;
uniform vec4 u_baseColorFactor;

void main()
{
	// Debug modes (uncomment one to test)
	// gl_FragColor = texture2D(s_texColor, v_texcoord0); // Diffuse texture
	// gl_FragColor = texture2D(s_texNormal, v_texcoord0); // Normal map
	// gl_FragColor = vec4(abs(v_normal), 1.0); // Geometry normal
	// gl_FragColor = vec4(abs(v_tangent), 1.0); // Tangent
	// gl_FragColor = vec4(abs(v_bitangent), 1.0); // Bitangent
	// gl_FragColor = vec4(u_baseColorFactor.rgb, 1.0); // Base color factor
	// gl_FragColor = vec4(abs(u_lightDir.xyz), 1.0); // Light direction
	// gl_FragColor = vec4(abs(normalize(mul(texture2D(s_texNormal, v_texcoord0).xyz * 2.0 - 1.0, mat3(v_tangent, v_bitangent, v_normal)))), 1.0); // Final normal
	// gl_FragColor = vec4(vec3(max(0.0, dot(normalize(mul(texture2D(s_texNormal, v_texcoord0).xyz * 2.0 - 1.0, mat3(v_tangent, v_bitangent, v_normal))), normalize(u_lightDir.xyz)))), 1.0); // Diffuse term
	// gl_FragColor = vec4(color.rgb * mix(0.3, 1.0, max(0.0, dot(normalize(mul(texture2D(s_texNormal, v_texcoord0).xyz * 2.0 - 1.0, mat3(v_tangent, v_bitangent, v_normal))), normalize(u_lightDir.xyz)))) * vec3(u_lightDir.w), color.a); // Final lit color
	// return;

	// Normal mapping
	vec3 normalMap = texture2D(s_texNormal, v_texcoord0).xyz;
	vec3 finalNormal;
	if (normalMap.r + normalMap.g + normalMap.b > 0.0)
	{
		normalMap = normalMap * 2.0 - 1.0;
		// Orthogonalize TBN matrix for robustness
		vec3 N = normalize(v_normal);
		vec3 T = normalize(v_tangent - dot(v_tangent, N) * N);
		vec3 B = normalize(v_bitangent - dot(v_bitangent, N) * N - dot(v_bitangent, T) * T);
		mat3 TBN = mat3(T, B, N);
		finalNormal = normalize(mul(normalMap, TBN));
	}
	else finalNormal = normalize(v_normal); // Fallback to geometry normal

	// Lighting
	vec3 lightDir = normalize(u_lightDir.xyz);
	vec3 viewDir = normalize(u_viewPos.xyz - v_position);
	vec3 halfway = normalize(lightDir + viewDir);

	float ambient = 0.3; // Balanced ambient
	float diffuse = max(0.0, dot(finalNormal, lightDir));
	float specular = pow(max(0.0, dot(finalNormal, halfway)), 32.0) * 0.2; // Simple Blinn-Phong specular
	float lambert = mix(ambient, 1.0, diffuse + specular);
	vec3 lightColor = vec3(u_lightDir.w); // Light intensity

	// Color
	vec4 color = toLinear(texture2D(s_texColor, v_texcoord0)) * u_baseColorFactor;

	gl_FragColor = vec4(color.rgb * lambert * lightColor, color.a);
}

// fs_pbr.sc

$input v_position, v_normal, v_tangent, v_bitangent, v_texcoord0

#include "common.sh"

SAMPLER2D(s_texColor, 0);
SAMPLER2D(s_texNormal, 1);
SAMPLER2D(s_texMetallicRoughness, 2);
SAMPLER2D(s_texEmissive, 3);
SAMPLER2D(s_texOcclusion, 4);

uniform vec4 u_lightDir;
uniform vec4 u_viewPos;
uniform vec4 u_baseColorFactor;
uniform vec4 u_metallicRoughness;
uniform vec4 u_emissiveFactor;
uniform vec4 u_materialFlags;
uniform vec4 u_occlusion;

#define PI 3.14159265359

float D_GGX(float NoH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NoH2 = NoH * NoH;
	float denom = NoH2 * (a2 - 1.0) + 1.0;
	return a2 / (PI * denom * denom);
}

vec3 F_Schlick(float VoH, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - VoH, 5.0);
}

float G_Smith(float NoV, float NoL, float roughness)
{
	float a = roughness * roughness;
	float NoV2 = NoV * NoV;
	float NoL2 = NoL * NoL;
	float ggx1 = NoV / (NoV * (1.0 - a) + a);
	float ggx2 = NoL / (NoL * (1.0 - a) + a);
	return ggx1 * ggx2;
}

void main()
{
	// Debug modes (uncomment one to test)
	// gl_FragColor = texture2D(s_texColor, v_texcoord0); // Diffuse texture
	// gl_FragColor = texture2D(s_texNormal, v_texcoord0); // Normal map
	// gl_FragColor = texture2D(s_texMetallicRoughness, v_texcoord0); // Metallic-roughness
	// gl_FragColor = texture2D(s_texEmissive, v_texcoord0); // Emissive
	// gl_FragColor = texture2D(s_texOcclusion, v_texcoord0); // Occlusion
	// gl_FragColor = vec4(abs(normalize(v_normal)), 1.0); // Geometry normal
	// gl_FragColor = vec4(abs(normalize(v_tangent)), 1.0); // Tangent
	// gl_FragColor = vec4(abs(normalize(v_bitangent)), 1.0); // Bitangent
	// gl_FragColor = vec4(u_baseColorFactor.rgb, 1.0); // Base color
	// gl_FragColor = vec4(u_metallicRoughness.xy, 0.0, 1.0); // Metallic/roughness
	// gl_FragColor = vec4(u_emissiveFactor.rgb, 1.0); // Emissive factor
	// gl_FragColor = vec4(abs(u_lightDir.xyz), 1.0); // Light direction
	// gl_FragColor = vec4(abs(normalize(u_viewPos.xyz - v_position)), 1.0); // View direction
	// gl_FragColor = vec4(abs(normalize(mul(texture2D(s_texNormal, v_texcoord0).xyz * 2.0 - 1.0, mat3(normalize(v_tangent), normalize(v_bitangent), normalize(v_normal)))), 1.0); // Final normal
	// gl_FragColor = vec4(vec3(max(0.0, dot(normalize(mul(texture2D(s_texNormal, v_texcoord0).xyz * 2.0 - 1.0, mat3(normalize(v_tangent), normalize(v_bitangent), normalize(v_normal)))), normalize(-u_lightDir.xyz)))), 1.0); // NoL term
	// gl_FragColor = vec4(((1.0 - F_Schlick(max(dot(normalize(u_viewPos.xyz - v_position), normalize(normalize(mul(texture2D(s_texNormal, v_texcoord0).xyz * 2.0 - 1.0, mat3(normalize(v_tangent), normalize(v_bitangent), normalize(v_normal))))), normalize(-u_lightDir.xyz)), vec3(0.04))) * (1.0 - u_metallicRoughness.x) * u_baseColorFactor.rgb / PI + (D_GGX(max(dot(normalize(mul(texture2D(s_texNormal, v_texcoord0).xyz * 2.0 - 1.0, mat3(normalize(v_tangent), normalize(v_bitangent), normalize(v_normal))), normalize(normalize(u_viewPos.xyz - v_position) + normalize(-u_lightDir.xyz))), 0.0), u_metallicRoughness.y) * F_Schlick(max(dot(normalize(u_viewPos.xyz - v_position), normalize(normalize(u_viewPos.xyz - v_position) + normalize(-u_lightDir.xyz))), vec3(0.04)) * G_Smith(max(dot(normalize(mul(texture2D(s_texNormal, v_texcoord0).xyz * 2.0 - 1.0, mat3(normalize(v_tangent), normalize(v_bitangent), normalize(v_normal))), normalize(u_viewPos.xyz - v_position)), 0.0), max(dot(normalize(mul(texture2D(s_texNormal, v_texcoord0).xyz * 2.0 - 1.0, mat3(normalize(v_tangent), normalize(v_bitangent), normalize(v_normal))), normalize(-u_lightDir.xyz)), 0.0), u_metallicRoughness.y)) / max(4.0 * max(dot(normalize(mul(texture2D(s_texNormal, v_texcoord0).xyz * 2.0 - 1.0, mat3(normalize(v_tangent), normalize(v_bitangent), normalize(v_normal))), normalize(u_viewPos.xyz - v_position)), 0.0) * max(dot(normalize(mul(texture2D(s_texNormal, v_texcoord0).xyz * 2.0 - 1.0, mat3(normalize(v_tangent), normalize(v_bitangent), normalize(v_normal))), normalize(-u_lightDir.xyz)), 0.0), 0.001)) * vec3(u_lightDir.w) * max(dot(normalize(mul(texture2D(s_texNormal, v_texcoord0).xyz * 2.0 - 1.0, mat3(normalize(v_tangent), normalize(v_bitangent), normalize(v_normal))), normalize(-u_lightDir.xyz)), 0.0), u_baseColorFactor.rgb * 0.3 * u_occlusion.x, u_baseColorFactor.a); // Final lit color
	// return;

	// Fallback values
	vec4 baseColor = u_baseColorFactor;
	vec3 normalMap = vec3(0.0, 0.0, 1.0);
	vec2 metallicRoughness = u_metallicRoughness.xy;
	vec3 emissive = u_emissiveFactor.rgb;
	float occlusion = u_occlusion.x;

	// gl_FragColor = baseColor;
	// return;

	// Sample textures if not unlit
	if (u_materialFlags.x < 0.5)
	{
		vec4 colorSample = texture2D(s_texColor, v_texcoord0);
		if (colorSample.a > 0.0) baseColor = toLinear(colorSample) * u_baseColorFactor;
		vec4 normalSample = texture2D(s_texNormal, v_texcoord0);
		if (normalSample.a > 0.0) normalMap = normalSample.xyz * 2.0 - 1.0;
		vec4 mrSample = texture2D(s_texMetallicRoughness, v_texcoord0);
		if (mrSample.a > 0.0) metallicRoughness = mrSample.bg * u_metallicRoughness.xy;
		vec4 emissiveSample = texture2D(s_texEmissive, v_texcoord0);
		if (emissiveSample.a > 0.0) emissive = toLinear(emissiveSample).rgb * u_emissiveFactor.rgb;
		vec4 occlusionSample = texture2D(s_texOcclusion, v_texcoord0);
		if (occlusionSample.a > 0.0) occlusion = occlusionSample.r * u_occlusion.x;
	}

	// Unlit mode with basic Lambertian lighting
	if (u_materialFlags.x > 0.5)
	{
		vec3 N = normalize(v_normal);
		vec3 T = normalize(v_tangent - dot(v_tangent, N) * N);
		vec3 B = normalize(v_bitangent - dot(v_bitangent, N) * N - dot(v_bitangent, T) * T);
		mat3 TBN = mat3(T, B, N);
		N = normalize(mul(normalMap, TBN));
		vec3 L = normalize(u_lightDir.xyz); // Consistent with fs_model_texture_tangent.sc
		vec3 lightColor = vec3(u_lightDir.w);
		float NoL = max(dot(N, L), 0.0);
		vec3 diffuse = baseColor.rgb * NoL * lightColor;
		vec3 ambient = baseColor.rgb * 0.1 * occlusion; // Simple ambient term
		gl_FragColor = vec4(diffuse + ambient, baseColor.a);
		return;
	}
	// Unlit mode
	// if (u_materialFlags.x > 0.5)
	// {
	// 	gl_FragColor = vec4(baseColor.rgb, baseColor.a);
	// 	return;
	// }

	// PBR pipeline
	vec3 N = normalize(v_normal);
	vec3 T = normalize(v_tangent - dot(v_tangent, N) * N);
	vec3 B = normalize(v_bitangent - dot(v_bitangent, N) * N - dot(v_bitangent, T) * T);
	mat3 TBN = mat3(T, B, N);
	N = normalize(mul(normalMap, TBN));
	vec3 V = normalize(u_viewPos.xyz - v_position);
	vec3 L = normalize(-u_lightDir.xyz);
	vec3 H = normalize(V + L);

	float metallic = metallicRoughness.x;
	float roughness = clamp(metallicRoughness.y, 0.04, 1.0);
	vec3 albedo = baseColor.rgb;
	float alpha = baseColor.a;

	vec3 F0 = mix(vec3(0.04), albedo, metallic);
	float NoL = max(dot(N, L), 0.0);
	float NoV = max(dot(N, V), 0.0);
	float NoH = max(dot(N, H), 0.0);
	float VoH = max(dot(V, H), 0.0);

	float D = D_GGX(NoH, roughness);
	vec3 F = F_Schlick(VoH, F0);
	float G = G_Smith(NoV, NoL, roughness);
	vec3 specular = (D * F * G) / max(4.0 * NoL * NoV, 0.001);
	vec3 diffuse = (1.0 - F) * (1.0 - metallic) * albedo / PI;
	vec3 lightColor = vec3(u_lightDir.w);
	vec3 color = (diffuse + specular) * lightColor * NoL;
	vec3 ambient = albedo * 0.1 * occlusion; // Increased ambient
	color += emissive;

	gl_FragColor = vec4(color + ambient, alpha);
}

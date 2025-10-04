// fs_pbr.sc

$input v_position, v_normal, v_tangent, v_bitangent, v_texcoord0

#include "common.sh"

SAMPLER2D(s_texColor, 0);    // Base color texture
SAMPLER2D(s_texNormal, 1);   // Normal map
SAMPLER2D(s_texMetallicRoughness, 2); // Metallic (b), Roughness (g)
SAMPLER2D(s_texEmissive, 3); // Emissive texture

uniform vec4 u_lightDir; // Directional light direction (xyz) and intensity (w)
uniform vec4 u_baseColorFactor; // Base color factor (RGBA)
uniform vec4 u_metallicRoughness; // Metallic (x), Roughness (y)
uniform vec4 u_emissiveFactor; // Emissive factor (RGB, w unused)

#define PI 3.14159265359

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float D_GGX(float NoH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NoH2 = NoH * NoH;
	float denom = NoH2 * (a2 - 1.0) + 1.0;
	return a2 / (PI * denom * denom);
}

// Fresnel (Schlick's approximation)
vec3 F_Schlick(float VoH, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - VoH, 5.0);
}

// Geometry term (Smith's method with GGX)
float G_Smith(float NoV, float NoL, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
	float G1_L = NoL / (NoL * (1.0 - k) + k);
	float G1_V = NoV / (NoV * (1.0 - k) + k);
	return G1_L * G1_V;
}

void main()
{
	// Sample textures
	vec4 baseColor = toLinear(texture2D(s_texColor, v_texcoord0)) * u_baseColorFactor;
	vec3 normalMap = texture2D(s_texNormal, v_texcoord0).xyz * 2.0 - 1.0;
	vec2 metallicRoughness = texture2D(s_texMetallicRoughness, v_texcoord0).bg * u_metallicRoughness.xy;
	vec3 emissive = toLinear(texture2D(s_texEmissive, v_texcoord0)).rgb * u_emissiveFactor.rgb;

	// Normal mapping (TBN matrix)
	mat3 TBN = mat3(normalize(v_tangent), normalize(v_bitangent), normalize(v_normal));
	vec3 N = normalize(mul(TBN, normalMap));
	vec3 V = normalize(u_view[3].xyz - v_position); // View direction (camera - position)
	vec3 L = normalize(-u_lightDir.xyz); // Light direction
	vec3 H = normalize(V + L); // Half vector

	// Material properties
	float metallic = metallicRoughness.x;
	float roughness = clamp(metallicRoughness.y, 0.04, 1.0); // Avoid fully smooth surfaces
	vec3 albedo = baseColor.rgb;
	float alpha = baseColor.a;

	// Fresnel base reflectivity (F0)
	vec3 F0 = mix(vec3(0.04, 0.04, 0.04), albedo, metallic); // 0.04 for dielectrics

	// BRDF components
	float NoL = max(dot(N, L), 0.0);
	float NoV = max(dot(N, V), 0.0);
	float NoH = max(dot(N, H), 0.0);
	float VoH = max(dot(V, H), 0.0);

	// Specular BRDF (Cook-Torrance)
	float D = D_GGX(NoH, roughness);
	vec3 F = F_Schlick(VoH, F0);
	float G = G_Smith(NoV, NoL, roughness);
	vec3 specular = (D * F * G) / max(4.0 * NoL * NoV, 0.001);

	// Diffuse BRDF (Lambertian)
	vec3 diffuse = (1.0 - F) * (1.0 - metallic) * albedo / PI;

	// Combine lighting terms
	vec3 lightColor = vec3(u_lightDir.www); // Light intensity
	vec3 color = (diffuse + specular) * lightColor * NoL;

	// Add ambient (simple approximation)
	vec3 ambient = albedo * 0.03; // 3% ambient for dielectrics

	// Add emissive
	color += emissive;

	// Final color
	gl_FragColor = vec4(color + ambient, alpha);
}

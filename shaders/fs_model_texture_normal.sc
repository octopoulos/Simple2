// fs_model_texture_normal.sc

$input v_normal, v_texcoord0

#include "../common/common.sh"

SAMPLER2D(s_texColor, 0);
SAMPLER2D(s_texNormal, 1);

uniform vec4 u_lightDir;

void main()
{
    // Fallback: face normal (geometry-provided)
    vec3 faceNormal = v_normal;

    // Optional detail: sampled from normal map
    vec3 mapNormal = texture2D(s_texNormal, v_texcoord0).xyz * 2.0 - 1.0;

    // Mix face and map normals (no TBN yet!)
    vec3 finalNormal = normalize(mix(faceNormal, mapNormal, 0.25));

    float ambient = 0.25;
    float diffuse = max(0.0, dot(finalNormal, normalize(u_lightDir.xyz)));
    float lambert = mix(ambient, 1.0, diffuse);

    vec4 color = toLinear(texture2D(s_texColor, v_texcoord0));

    gl_FragColor = vec4(color.rgb * lambert, 1.0);
}

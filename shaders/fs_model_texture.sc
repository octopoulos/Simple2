// fs_model_texture.sc

$input v_normal, v_texcoord0

#include "../common/common.sh"

SAMPLER2D(s_texColor, 0);

uniform vec4 u_lightDir;

void main()
{
    float diffuse = max(0.0, dot(v_normal, u_lightDir.xyz));

    vec4 color = toLinear(texture2D(s_texColor, v_texcoord0));
    
    gl_FragColor.xyz = diffuse * color.xyz;
    gl_FragColor.w = 1.0;
}

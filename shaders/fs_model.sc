// fs_model.sc

$input v_normal, v_texcoord0

#include <bgfx_shader.sh>

uniform vec4 u_lightDir;

void main()
{
    float ambient = 0.15;
    float diffuse = max(0.0, dot(v_normal, u_lightDir.xyz));
    float lambert = mix(ambient, 1.0, diffuse);

    gl_FragColor = vec4(lambert * vec3(1.0, 1.0, 1.0), 1.0);
}

// fs_cursor.sc

$input v_position, v_normal

#include <bgfx_shader.sh>

uniform vec4 u_cursorCol;
uniform vec4 u_lightDir;
uniform vec4 u_time;

void main()
{
    float freq   = 5.0;
    float offset = v_position.y * 4.0;
    float t      = u_time.x * freq + offset;

    // triangle waveform for a sharper pulse (smoother than abs(sin()))
    float pulse = 1.0 - abs(fract(t / 3.14159) * 2.0 - 1.0);

    float ambient = 0.45;
    float diffuse = max(0.0, dot(v_normal, u_lightDir.xyz));
    float lambert = mix(ambient, 1.0, diffuse);

    vec3 color   = mix(vec3(u_cursorCol), vec3(1.0, 1.0, 0.0), pulse);
    gl_FragColor = vec4(color * lambert, 0.6);
}

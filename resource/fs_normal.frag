#version 330
layout(location=0) out vec4 color;

uniform sampler2D text;
uniform vec3 object_color;
uniform vec3 light_pos;
uniform vec3 eye_pos;

in vec3 g_position;
in vec2 g_uv;
in vec3 g_normal;

void main()
{
    vec3 n = normalize(g_normal);
    n = n * 0.5 + 0.5;

    color = vec4(n, 1.0);
}
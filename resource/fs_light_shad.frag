#version 330
layout(location=0) out vec4 color;

uniform sampler2D text;
uniform vec3 object_color;
uniform vec3 light_pos;
uniform vec3 eye_pos;

in vec3 g_position;
in vec2 g_uv;
in vec3 g_normal;

uniform int bling_phong;

void main()
{
    vec3 l = normalize(light_pos - g_position);
    vec3 n = normalize(g_normal);
    float cosine = max(dot(l, n), 0);
    vec3 r = reflect(-l, n);
    vec3 e = normalize(eye_pos - g_position);
    vec3 h = normalize(l + e);
    float spec;

    if(bling_phong == 1){
        spec = cosine * pow(max(dot(n, h), 0), 30);
    }else{
        spec = cosine*pow(max(dot(r, e), 0), 30);
    }

    
    color = vec4(object_color*texture(text, g_uv).rgb*cosine+spec, 1.0);
}
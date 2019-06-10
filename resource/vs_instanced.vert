#version 330
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec3 normal;

uniform mat4 model;
uniform mat4 vp;
uniform vec3 offsets[1000];

out vec3 v_position;
out vec2 v_uv;
out vec3 v_normal;

void main()
{
    vec4 temp_position = vec4(offsets[gl_InstanceID], 0.0) + model*vec4(position, 1.0);
    v_position = temp_position.xyz;
    v_uv = texcoord;
    v_normal = (model*vec4(normal, 0.0)).xyz;
    gl_Position = vp*temp_position;
}
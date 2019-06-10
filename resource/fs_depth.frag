#version 330
layout(location=0) out vec4 color;

uniform sampler2D text;
uniform vec3 object_color;
uniform vec3 light_pos;
uniform vec3 eye_pos;

in vec3 g_position;
in vec2 g_uv;
in vec3 g_normal;

float near = 0.1;
float far  = 10;
  
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main()
{             
    float depth = LinearizeDepth(gl_FragCoord.z) / far; // divide by far for demonstration
    color = vec4(vec3(depth), 1.0);
}

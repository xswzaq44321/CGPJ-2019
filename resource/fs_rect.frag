#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform sampler2D texture2;

uniform vec3 light_pos;
uniform mat4 vs;
uniform mat4 inv;
uniform int bling_phong;
uniform float near;
uniform float far;
uniform int ww;
uniform int wh;

float getZ2(float depth){
    return -(depth*(far - near) + near);
}

float getZ(float depth){
    return getZ2(depth);
    return (((-2.0*near*far)/(far*depth)+far+near)/(far - near) + 1) / 2;
}

void main()
{
    if(texture(texture1, TexCoord).rgb == vec3(1.0)) discard;
    vec3 depth = texture(texture1, TexCoord).rgb;
    vec3 normal = texture(texture2, TexCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    vec3 myPos = vec3(gl_FragCoord.xy, 0);
    myPos.x = (2.0 * (myPos.x - 0.0) / ww) - 1.0;
    myPos.y = (2.0 * (myPos.y - 0.0) / wh) - 1.0;
    myPos = normalize(myPos);
    myPos = vec3(vs * inv * vec4(myPos, 1.0));
    myPos.z = getZ(depth.z);
    // FragColor = vec3(TexCoord * 100, 0.0);
    vec3 l = normalize((vs*vec4(light_pos, 1.0)).xyz-myPos);
    vec3 n = normalize(normal);
    float cosine = max(dot(l, n), 0);
    vec3 r = reflect(-l, n);
    vec3 e = normalize(-myPos);
    vec3 h = normalize(l + e);
    float spec;

    if(bling_phong == 1){
        spec = cosine * pow(max(dot(n, h), 0), 30);
    }else{
        spec = cosine*pow(max(dot(r, e), 0), 30);
    }

    FragColor = vec4(vec3(1.0)*cosine+spec, 1.0);
    // FragColor = vec4(vec3(myPos), 1.0);
} 

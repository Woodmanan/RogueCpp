#version 450

layout(binding = 3) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec2 lower;
layout(location = 2) in vec2 upper;
layout(location = 3) in vec4 fgColor;
layout(location = 4) in vec4 bgColor;

layout(location = 0) out vec4 outColor;


void main() {
    if (fragTexCoord.x < 0.01 || fragTexCoord.x > 0.99 || fragTexCoord.y < 0.01 || fragTexCoord.y > 0.99)
    {
        outColor = bgColor;
    }
    else
    {
        vec2 finalCoord = (upper - lower) * fragTexCoord + lower;
        float mask = texture(texSampler, finalCoord).x;
        outColor =  mask * fgColor + (1 - mask) * bgColor;
    }
} 
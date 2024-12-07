#version 450

layout(binding = 4) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 fgColor;
layout(location = 3) in vec4 bgColor;

layout(location = 0) out vec4 outColor;


void main() {
    if (uv.x < 0.01 || uv.x > 0.99 || uv.y < 0.01 || uv.y > 0.99)
    {
        outColor = bgColor;
    }
    else
    {
        float mask = texture(texSampler, fragTexCoord).x;
        //mask = mask * mask * (3.0 - 2.0 * mask);
        outColor =  mask * fgColor + (1 - mask) * bgColor;
        //outColor = vec4(fragTexCoord,0,1);
    }
} 
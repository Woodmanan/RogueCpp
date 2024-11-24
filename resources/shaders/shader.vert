#version 450
#extension GL_EXT_debug_printf : enable
#extension GL_EXT_scalar_block_layout : enable

layout(std430, binding = 0) uniform UniformTileObject
{
	uint tileIndices[4096];
	vec2 lowerUVs[128];
	vec2 upperUVs[128];
} tileData;

layout(std430, binding = 1) uniform FGColorObject
{
	vec4 colors[4096];
} fgColors;

layout(std430, binding = 2) uniform BGColorObject
{
	vec4 colors[4096];
} bgColors;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in uint inIndex;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec2 lower;
layout(location = 2) out vec2 upper;
layout(location = 3) out vec4 fgColor;
layout(location = 4) out vec4 bgColor;


void main() {
    gl_Position =  vec4(inPosition, 0.0, 1.0);
	uint index = tileData.tileIndices[inIndex];

	lower = tileData.lowerUVs[index];
	upper = tileData.upperUVs[index];
	 
	fragTexCoord = inTexCoord;
	fgColor = fgColors.colors[inIndex];
	bgColor = bgColors.colors[inIndex];
}
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


struct materialInfo{
	uint materialID;
	uint padding1;
};

//In
//Vertex Attributes
layout (location = 0) in vec4 inWorldPos;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec4 inNormal;
layout (location = 4) in flat uint inDrawID;

//UBO's
layout (binding = 2) uniform sampler2D textureSampler[25];

layout (std430, binding = 3) readonly buffer materialID
{
	materialInfo indirectMaterialIDData[];
};

//Out
layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;

void main() 
{
	outPosition = vec4(inWorldPos.xyz, 1.0);
	//outPosition = inWorldPos;
	outNormal = vec4(inNormal.xyz, 1.0);

	
	uint materialIDValue = indirectMaterialIDData[inDrawID].materialID;
	
	//outAlbedo = vec4(1.0, 0.0, 0.0, 1.0);
	outAlbedo = texture(textureSampler[materialIDValue], inUV);
}
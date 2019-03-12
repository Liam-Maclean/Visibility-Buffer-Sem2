#version 450

//Vertex Attributes
layout (location = 0) in vec4 inPos;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec4 inNormal;

//UBO's
//standard UBO
layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 view;
} ubo;

//Dynamic UBO for model matrix
layout (binding = 1) uniform UboModelMatrix {
	mat4 model; 
} uboModelMatrix;


layout (binding = 2) uniform LightViewMatrix {
	mat4 lightMatrix;
} lightViewMatrix;

void main()
{
	gl_Position = lightViewMatrix.lightMatrix * uboModelMatrix.model * vec4(inPos.xyz, 1.0f);
	
}


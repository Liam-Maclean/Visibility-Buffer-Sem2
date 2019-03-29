#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//structs
struct spotLight
{
	vec4 position;
	vec4 diffuse;
	vec3 specular;
	float cutOff;
};

struct pointLight
{
	vec4 position;
	vec4 diffuse;
	vec4 specular;
	float constant;
	float linear;
	float quadratic;
};

struct directionalLight
{
	vec4 direction;
	vec4 diffuse;
	vec4 specular;
};

//In
//attribute locations
layout (location = 0) in vec2 inUV;

//UBO's
layout (binding = 0) uniform sampler2D samplerPosition;

layout (binding = 1) uniform sampler2D samplerNormal;

layout (binding = 2) uniform sampler2D samplerAlbedo;

layout (std430, binding = 3) readonly buffer spotLightBuffer
{
	spotLight spotLightData[];
};
layout (std430, binding = 4) readonly buffer pointLightBuffer
{
	pointLight pointLightData[];
};
layout (std430, binding = 5) readonly buffer directionalLightBuffer
{
	directionalLight directionalLightData[];
};

layout (binding = 6) uniform sampler2D shadowMapTexture;

layout (set = 0,binding = 7) uniform LightMatrix
{
	mat4 matrix;
}lightMatrix;

layout (set = 0,binding = 8) uniform Camera
{
	vec4 eye;
}camera;

//Out
layout (location = 0) out vec4 outFragColor;

//Main section of the shader
void main() 
{
	float specularStrength = 0.1f;
	//temporary variables
	vec4 ambientColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
	vec3 diffuseComponent = vec3(0.0f, 0.0f, 0.0f);
	vec3 specular = vec3(0.0f, 0.0f, 0.0f);
	vec4 lightDir;
	float lightIntensity;
	
	// Get G-Buffer values
	vec4 fragPos = texture(samplerPosition, inUV);
	vec3 normal = texture(samplerNormal, inUV).rgb;
	vec4 albedo = texture(samplerAlbedo, inUV);
	float shadow = 1.0f;
	
	
	normal = normalize(normal);
	
	//DIRECTIONAL LIGHTS
	for(int i = 0; i < 1; i++)
	{
		//Test lighting for a single directional light structure
		lightDir = directionalLightData[i].direction;
		
		
		//get light intensity of the dot product of the normal and light direction
		lightIntensity = max(dot(normal, lightDir.xyz), 0.0f);
		
		
		
		//if the pixel is lit
		if (lightIntensity > 0.0f)
		{
			vec4 viewDir = normalize(camera.eye - fragPos);
			vec3 reflectDir = reflect(-lightDir.xyz, normal);
			float spec = pow(max(dot(viewDir.xyz, reflectDir), 0.0), 32);
			vec3 specular = specularStrength * spec * directionalLightData[i].diffuse.xyz;  
			
			
			diffuseComponent += (directionalLightData[i].diffuse.xyz * lightIntensity);
			//diffuseComponent = normalize(diffuseComponent);
		}
	}
	
	//for(int i = 0; i < 1; i++)
	//{
	//	vec3 lightDir = normalize(pointLightData[i].position.xyz - fragPos.xyz);
	//	float diff = max(dot(normal, lightDir), 0.0);
	//	
	//	if (diff > 0.0f)
	//	{
	//		//Diffuse
	//		diffuseComponent = diff * pointLightData[i].diffuse.xyz;
	//		
	//		//Specular
	//		float specularStrength = 0.5f;
	//		
	//		vec3 viewDir = normalize(camera.eye.xyz - fragPos.xyz);
	//		vec3 reflectDir = reflect(-lightDir, normal.xyz);
	//		float spec = pow(max(dot(viewDir, reflectDir), 0.0f),2);
	//		specular = specularStrength * spec * pointLightData[i].specular.xyz;  
	//		
	//		//Attentuation
	//		float dist = length(pointLightData[i].position.xyz - fragPos.xyz);
	//		float attenuation = 1.0f / (pointLightData[i].constant + pointLightData[i].linear * dist + pointLightData[i].quadratic * (dist * dist));
	//		
	//		
	//		specular *= attenuation;
	//		diffuseComponent *= attenuation;
	//		ambientColor *= attenuation;
	//	}
	//}
	//
	//for(int i = 0; i < 1; i++)
	//{
	//	vec4 fragPosInvertY = fragPos;
	//	fragPosInvertY.y = -fragPosInvertY.y;
	//
	//	vec4 shadowClip = lightMatrix.matrix * vec4(fragPosInvertY.xyz, 1.0);
	//	
	//	float shadow = 1.0;
	//	vec3 shadowCoord = shadowClip.xyz / shadowClip.w;
	//	shadowCoord.st = shadowCoord.st * 0.5 + 0.5;
	//	float dist = texture(shadowMapTexture, vec2(shadowCoord.st)).r;
	//	if (dist < shadowCoord.z - 0.005f) 
	//	{
	//		shadow = 0.1f;
	//	}
	//	
	//	diffuseComponent *= shadow;
	//}

	vec3 outColor = (ambientColor.xyz + diffuseComponent.xyz + specular.xyz) * albedo.xyz;
	outFragColor = vec4(outColor.xyz, 1.0);
}
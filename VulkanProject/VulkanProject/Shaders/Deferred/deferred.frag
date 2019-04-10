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


layout (push_constant) uniform PushConstants {
	uint drawModeID;
} pushConstants;


//In
//attribute locations
layout (location = 0) in vec2 inUV;

//Specialization data added at pipeline creation for the shader
layout (constant_id = 0) const int NUM_SAMPLES = 8;

//UBO's
layout (binding = 0) uniform sampler2DMS samplerPosition;

layout (binding = 1) uniform sampler2DMS samplerNormal;

layout (binding = 2) uniform sampler2DMS samplerAlbedo;

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

layout (binding = 6) uniform sampler2DMS shadowMapTexture;

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



// Manual resolve for MSAA samples 
vec4 resolve(sampler2DMS tex, ivec2 uv)
{
	vec4 result = vec4(0.0);	   
	for (int i = 0; i < NUM_SAMPLES; i++)
	{
		vec4 val = texelFetch(tex, uv, i); 
		result += val;
	}    
	// Average resolved samples
	return result / float(NUM_SAMPLES);
}

//Main section of the shader
void main() 
{
	ivec2 attDim = textureSize(samplerPosition);
	ivec2 shadowAttDim = textureSize(shadowMapTexture);
	ivec2 uv = ivec2(inUV * attDim);
	
	
	
	vec3 specular = vec3(0.0f, 0.0f, 0.0f);
	vec4 lightDir;
	float lightIntensity;
	vec4 ambientColor = vec4(0.15f, 0.15f, 0.15f, 0.15f);
	float ambientValue = 0.5f;
	float specularStrength = 0.1f;
	
	vec4 alb = resolve(samplerAlbedo, uv);
	
	
	vec3 diffuseComponent = vec3(0.0f, 0.0f, 0.0f);
	
	
	//output the position screen draw
	if(pushConstants.drawModeID == 1)
	{
		vec4 fragPos = texelFetch(samplerPosition, uv, 0);
		outFragColor = fragPos;
	}
	//output the normal screen draw
	else if (pushConstants.drawModeID == 2)
	{
		vec3 normal = texelFetch(samplerNormal, uv, 0).rgb;
		outFragColor = vec4(normal.xyz, 1.0f);
	}
	//output the albedo screen draw
	else if (pushConstants.drawModeID == 3)
	{
		vec4 albedo = texelFetch(samplerAlbedo, uv, 0);
		outFragColor = albedo;
	}
	else if (pushConstants.drawModeID == 4)
	{
		outFragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	//output as normal doing lighting and shadows
	else
	{
		//temporary variables
		for(int x = 0; x < NUM_SAMPLES; x++)
		{
			// Get G-Buffer values
			vec4 fragPos = texelFetch(samplerPosition, uv, x);
			vec3 normal = texelFetch(samplerNormal, uv, x).rgb;
			vec4 albedo = texelFetch(samplerAlbedo, uv, x);
			float shadow = 1.0f;
		
			normal = normalize(normal);
		
			//DIRECTIONAL LIGHTS
			for(int i = 0; i < 1; i++)
			{
				//Test lighting for a single directional light structure
				lightDir = directionalLightData[i].direction;
				
				lightDir = normalize(lightDir);
				
				//get light intensity of the dot product of the normal and light direction
				lightIntensity = max(0.0f, dot(normal, lightDir.xyz));
				
				
				
				//if the pixel is lit
				if (lightIntensity > 0.0f)
				{					
					diffuseComponent += (directionalLightData[i].diffuse.xyz * lightIntensity);
					//diffuseComponent = normalize(diffuseComponent);
				}
			}
			
			for(int i = 0; i < 1; i++)
			{
				vec4 fragPosInvertY = fragPos;
				fragPosInvertY.y = -fragPosInvertY.y;
			
				vec4 shadowClip = lightMatrix.matrix * vec4(fragPosInvertY.xyz, 1.0);
				
				
				float shadow = 1.0;
				vec3 shadowCoord = shadowClip.xyz / shadowClip.w;
				shadowCoord.st = shadowCoord.st * 0.5 + 0.5;
				ivec2 shadowCoordMS = ivec2(shadowCoord.st * shadowAttDim);
				float dist = texelFetch(shadowMapTexture, ivec2(shadowCoordMS.st),x).r;
				if (dist < shadowCoord.z - 0.005f) 
				{
					shadow = 0.1f;
				}
				
				diffuseComponent *= shadow;
			}
		}
		
		//vec3 outColor = (ambientColor.xyz + diffuseComponent.xyz + specular.xyz) * alb.xyz;
		vec3 fragColor = (alb.rgb * ambientValue) + diffuseComponent.xyz / float(NUM_SAMPLES);
		outFragColor = vec4(fragColor.rgb, 1.0);
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

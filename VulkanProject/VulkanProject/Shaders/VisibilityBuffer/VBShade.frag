#version 450 core

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define MAX_TEXTURES = 25

struct DerivativesOutput
{
	vec3 db_dx;
	vec3 db_dy;
};

// Computes the partial derivatives of a triangle from the projected screen space vertices
DerivativesOutput computePartialDerivatives(vec2 v[3])
{
	DerivativesOutput derivative;
	float d = 1.0 / determinant(mat2(v[2] - v[1], v[0] - v[1]));
	derivative.db_dx = vec3(v[1].y - v[2].y, v[2].y - v[0].y, v[0].y - v[1].y) * d;
	derivative.db_dy = vec3(v[2].x - v[1].x, v[0].x - v[2].x, v[1].x - v[0].x) * d;
	return derivative;
}

// Helper functions to interpolate vertex attributes at point 'd' using the partial derivatives
vec3 interpolateAttribute(mat3 attributes, vec3 db_dx, vec3 db_dy, vec2 d)
{
	vec3 attribute_x = attributes * db_dx;
	vec3 attribute_y = attributes * db_dy;
	vec3 attribute_s = attributes[0];
	
	return (attribute_s + d.x * attribute_x + d.y * attribute_y);
}

float interpolateAttribute(vec3 attributes, vec3 db_dx, vec3 db_dy, vec2 d)
{
	float attribute_x = dot(attributes, db_dx);
	float attribute_y = dot(attributes, db_dy);
	float attribute_s = attributes[0];
	
	return (attribute_s + d.x * attribute_x + d.y * attribute_y);
}

struct GradientInterpolationResults
{
	vec2 interp;
	vec2 dx;
	vec2 dy;
};

// Interpolate 2D attributes using the partial derivatives and generates dx and dy for texture sampling.
GradientInterpolationResults interpolateAttributeWithGradient(mat3x2 attributes, vec3 db_dx, vec3 db_dy, vec2 d, vec2 twoOverRes)
{
	vec3 attr0 = vec3(attributes[0].x, attributes[1].x, attributes[2].x);
	vec3 attr1 = vec3(attributes[0].y, attributes[1].y, attributes[2].y);
	vec2 attribute_x = vec2(dot(db_dx,attr0), dot(db_dx,attr1));
	vec2 attribute_y = vec2(dot(db_dy,attr0), dot(db_dy,attr1));
	vec2 attribute_s = attributes[0];
	
	GradientInterpolationResults result;
	result.dx = attribute_x * twoOverRes.x;
	result.dy = attribute_y * twoOverRes.y;
	result.interp = (attribute_s + d.x * attribute_x + d.y * attribute_y);
	return result;
}

float depthLinearization(float depth, float near, float far)
{
	return (2.0 * near) / (far + near - depth * (far - near));
}


//STRUCTS FOR STORAGE BUFFERS
struct vertex
{
	vec4 position;
	vec4 color;
	vec4 normal;
	vec4 padding;
};

struct materialInfo{
	uint materialID;
	uint padding1;
};

struct indexData{
	uint vertexStart;
	uint padding;
};

struct directionalLight
{
	vec4 direction;
	vec4 diffuse;
	vec4 specular;
};

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
} ubo;

layout (binding = 1) uniform sampler2D inTexture[25];
layout (binding = 2) uniform sampler2D inVBTexture;
layout (std430, binding = 3) readonly buffer indexBuffer
{
	uint indexBufferData[];
};

layout (std430, binding = 4) readonly buffer vertexPosition
{
	vertex vertexPosData[];
};

layout (std430, binding = 5) readonly buffer startIndex
{
	indexData indirectArgsData[];
};

layout (std430, binding = 6) readonly buffer materialID
{
	materialInfo indirectMaterialIDData[];
};

layout (std430, binding = 7) readonly buffer directionalLightBuffer
{
	directionalLight directionalLightData[];
};

layout (binding = 8) uniform sampler2D inShadowMapTexture;

layout (binding = 9) uniform InverseMatrices
{
	mat4 inverseVP;
}inverseMatrices;

layout (binding = 10) uniform LightMatrices
{
	mat4 lightVP;
}lightMatrices;



layout (location = 0) in vec2 inScreenPos;
layout (location = 0) out vec4 outColor;



void main()
{
    vec4 visRaw = texelFetch(inVBTexture, ivec2(gl_FragCoord.xy), 0);
	
	
    //// Unpack float4 render target data into uint to extract data
    uint alphaBit_drawID_triID = packUnorm4x8(visRaw);
	
	vec3 shadedColor = vec3(1.0f, 1.0f, 1.0f);
    //// Early exit if this pixel doesn't contain triangle data
	if (alphaBit_drawID_triID != 0)
	{
		// Extract packed data
		uint drawID = (alphaBit_drawID_triID >> 19);
		uint triangleID = (alphaBit_drawID_triID & 0x0000FFFF);
		uint alpha1_opaque0 = (alphaBit_drawID_triID >> 31);
    
		uint startIndex = indirectArgsData[drawID].vertexStart;
		uint triIdx0 = (triangleID * 3 + 0) + startIndex;
		uint triIdx1 = (triangleID * 3 + 1) + startIndex;
		uint triIdx2 = (triangleID * 3 + 2) + startIndex;
    
		uint index0 = indexBufferData[triIdx0];
		uint index1 = indexBufferData[triIdx1];
		uint index2 = indexBufferData[triIdx2];
    
		// Load vertex data of the 3 vertices
		vec3 v0pos = vec3(vertexPosData[index0].position.x, vertexPosData[index0].position.y, vertexPosData[index0].position.z);
		vec3 v1pos = vec3(vertexPosData[index1].position.x, vertexPosData[index1].position.y, vertexPosData[index1].position.z);
		vec3 v2pos = vec3(vertexPosData[index2].position.x, vertexPosData[index2].position.y, vertexPosData[index2].position.z);
    
		// Transform positions to clip space
		vec4 pos0 = (ubo.projection * ubo.view * ubo.model *  vec4(v0pos, 1));
		vec4 pos1 = (ubo.projection * ubo.view * ubo.model *  vec4(v1pos, 1));
		vec4 pos2 = (ubo.projection * ubo.view * ubo.model *  vec4(v2pos, 1));
    
		// Calculate the inverse of w, since it's going to be used several times
		vec3 one_over_w = 1.0 / vec3(pos0.w, pos1.w, pos2.w);
    
		// Project vertex positions to calcualte 2D post-perspective positions
		pos0 *= one_over_w[0];
		pos1 *= one_over_w[1];
		pos2 *= one_over_w[2];
    
		vec2 pos_scr[3] = { pos0.xy, pos1.xy, pos2.xy };
    
		// Compute partial derivatives. This is necessary to interpolate triangle attributes per pixel.
		DerivativesOutput derivativesOut = computePartialDerivatives(pos_scr);
    
		// Calculate delta vector (d) that points from the projected vertex 0 to the current screen point
		vec2 d = inScreenPos + -pos_scr[0];
    
		// Interpolate the 1/w (one_over_w) for all three vertices of the triangle
		// using the barycentric coordinates and the delta vector
		float w = 1.0 / interpolateAttribute(one_over_w, derivativesOut.db_dx, derivativesOut.db_dy, d);
    
		// Reconstruct the Z value at this screen point performing only the necessary matrix * vector multiplication
		// operations that involve computing Z
		float z = w * ubo.projection[2][2] + ubo.projection[3][2];
	
	
		//Get world position coordinates of the position vector
		vec4 position = (inverseMatrices.inverseVP * vec4(inScreenPos * w, z, w));
	
		// TEXTURE COORD INTERPOLATION
		vec2 texCoordFlipped0 = vertexPosData[index0].color.xy;
		texCoordFlipped0.y = 1.0 - texCoordFlipped0.y;
		vec2 texCoordFlipped1 = vertexPosData[index1].color.xy;
		texCoordFlipped1.y = 1.0 - texCoordFlipped1.y;
		vec2 texCoordFlipped2 = vertexPosData[index2].color.xy;
		texCoordFlipped2.y = 1.0 - texCoordFlipped2.y;
	
	
		vec2 vertexPre0 = vertexPosData[index0].color.xy * one_over_w[0];
		vec2 vertexPre1 = vertexPosData[index1].color.xy * one_over_w[1];
		vec2 vertexPre2 = vertexPosData[index2].color.xy * one_over_w[2];
	
		// Apply perspective correction to texture coordinates
		mat3x2 texCoords =
		{
			texCoordFlipped0 * one_over_w[0],
			texCoordFlipped1 * one_over_w[1],
			texCoordFlipped2 * one_over_w[2]
		};
    
		vec2 twoOverRes = vec2(2.0f/1280.0f,2.0f/720.0f);

		//// Interpolate texture coordinates and calculate the gradients for texture sampling with mipmapping support
		GradientInterpolationResults results = interpolateAttributeWithGradient(texCoords, derivativesOut.db_dx, derivativesOut.db_dy, d, twoOverRes);

		vec2 texCoordDX = results.dx * w;
		vec2 texCoordDY = results.dy * w; 
		vec2 texCoord = results.interp * w;
	
		uint materialIDValue = indirectMaterialIDData[drawID].materialID;
		
		vec4 textureGradient = textureGrad(inTexture[materialIDValue], texCoord, texCoordDX, texCoordDY);

		
		// Load normals
		vec3 v0normal = vec3(vertexPosData[index0].normal.x, vertexPosData[index0].normal.y, vertexPosData[index0].normal.z);
		vec3 v1normal = vec3(vertexPosData[index1].normal.x, vertexPosData[index1].normal.y, vertexPosData[index1].normal.z);
		vec3 v2normal = vec3(vertexPosData[index2].normal.x, vertexPosData[index2].normal.y, vertexPosData[index2].normal.z);
		
		mat3x3 normals =
		{

			v0normal * one_over_w[0],
			v1normal * one_over_w[1],
			v2normal * one_over_w[2]
		};
		
		vec3 normal = normalize(interpolateAttribute(normals, derivativesOut.db_dx, derivativesOut.db_dy, d));
		
		vec4 ambientColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
		vec3 diffuseComponent = vec3(0.0f, 0.0f, 0.0f);
		vec4 lightDir;
		float lightIntensity;
		
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
				diffuseComponent += (directionalLightData[i].diffuse.xyz * lightIntensity);
			}
		}
		
		for(int i = 0; i < 1; i++)
		{
			vec4 fragPosInvertY = position;
			//fragPosInvertY.y = -fragPosInvertY.y;
			
			vec4 shadowClip = lightMatrices.lightVP * vec4(fragPosInvertY.xyz, 1.0);
			
			float shadow = 1.0;
			vec3 shadowCoord = shadowClip.xyz / shadowClip.w;
			shadowCoord.st = shadowCoord.st * 0.5 + 0.5;
			float dist = texture(inShadowMapTexture, vec2(shadowCoord.st)).r;
			if (dist < shadowCoord.z - 0.005f) 
			{
				shadow = 0.1f;
			}
			
			diffuseComponent *= shadow;
		}
	
		
		vec3 outVec3Color = (ambientColor.xyz + diffuseComponent.xyz) * textureGradient.xyz; 
		//outColor = vec4(ambientColor.xyz + diffuseComponent.xyz) * 
		outColor = vec4(outVec3Color, 1);
	}
	else 
	{
		outColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
		//outColor = vec4(shadedColor.xyz, 1.0f);
	}
}

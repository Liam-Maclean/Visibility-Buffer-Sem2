#version 450

uint calculateOutputVBID(bool opaque, uint drawID, uint primitiveID)
{
	uint drawID_primID = ((drawID << 19)) | (primitiveID);
	return (opaque) ? drawID_primID : (1 << 31) | drawID_primID;
}

layout (location = 0) in flat uint iDrawID;
layout (location = 0) out vec4 oColor;

void main()
{
	oColor = unpackUnorm4x8(calculateOutputVBID(true, iDrawID, gl_PrimitiveID));
}
#pragma once
#include "BaseModel.h"
class ImportedModel :
	public BaseModel
{
public:
	ImportedModel(std::string filePath, bool hasTexCoords)
		:BaseModel()
	{
		ImportedModel::LoadMeshFromFile(filePath, hasTexCoords);
	};

	~ImportedModel();

	struct ModelPart
	{
		uint32_t vertexBase;
		uint32_t vertexCount;
		uint32_t indexBase;
		uint32_t indexCount;
	};

	std::vector<ModelPart> parts;

	uint32_t vertexCount;
	uint32_t indexCount;

	//Method to load data into vertices datastructure
	void LoadMeshFromFile(std::string filePath, bool hasTexCoords);
};


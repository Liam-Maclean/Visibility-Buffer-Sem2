#pragma once
#include "BaseModel.h"
class ImportedModel :
	public BaseModel
{
public:
	ImportedModel(std::string filePath, bool hasTexCoords, std::string materialPath, std::vector<std::string>& textureFilepaths)
		:BaseModel()
	{
		ImportedModel::LoadMeshFromFile(filePath, hasTexCoords, materialPath, textureFilepaths);
	};

	~ImportedModel();

	//Method to load data into vertices datastructure
	void LoadMeshFromFile(std::string filePath, bool hasTexCoords, std::string materialPath, std::vector<std::string>& textureFilepaths);
};


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

	//Method to load data into vertices datastructure
	void LoadMeshFromFile(std::string filePath, bool hasTexCoords);
};


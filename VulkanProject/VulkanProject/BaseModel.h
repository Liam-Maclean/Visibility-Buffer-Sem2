#pragma once
#include <string>
#include <vector>
#include "Shared.h"
#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"
#include <unordered_map>


namespace std {
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return ((hash<glm::vec4>()(vertex.pos) ^ (hash<glm::vec4>()(vertex.color) << 1)) >> 1);
		}
	};
}
class BaseModel
{
public:
	BaseModel();
	~BaseModel();

	//Create model function (where vertices and indices are created)
	virtual void CreateModel();
	static void LoadMeshFromFile(std::string modelPath, std::string materialPath, bool hasTexCoords, std::vector<BaseModel*>& models, std::vector<std::string>& textureFilepaths)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::map<std::string, int> materialMap;
		std::string err;

		std::cout << materials.max_size() << std::endl;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath.c_str(), materialPath.c_str())) {
			throw std::runtime_error(err);
		}

		std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

		

		for (const auto& shape : shapes) {
			BaseModel* newModel = new BaseModel();
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex = {};

				if (hasTexCoords)
				{
					vertex.pos = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2],
						attrib.texcoords[2 * index.texcoord_index + 0]
					};

					vertex.color = { attrib.texcoords[2 * index.texcoord_index + 0], attrib.texcoords[2 * index.texcoord_index + 1], 1.0f, attrib.texcoords[2 * index.texcoord_index + 1] };
				}
				else
				{
					vertex.pos = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2],
						0
					};


					vertex.color = { 0,0,0,0 };
				}



				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2],
					1.0f
				};

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(newModel->vertices.size());
					newModel->vertices.push_back(vertex);
				}
			
				newModel->indices.push_back(uniqueVertices[vertex]);
				//shape.mesh.material_ids;

				
				newModel->materialID = shape.mesh.material_ids[0];

			}
			models.push_back(newModel);
		}

		for (const auto& material : materials)
		{
			if (material.diffuse_texname != "")
			{
				std::string string = material.diffuse_texname;
				string.erase(0, 9);
				textureFilepaths.push_back(materialPath + "textures/" + string);
				//textureFilepaths.push_back(materialPath + string);
			}
			else
			{
				textureFilepaths.push_back("Textures/DefaultTexture.jpg");
			}
		}
	}

	//Getters
	uint32_t GetVertexCount() {  return static_cast<uint32_t>(vertices.size()); };
	uint32_t GetIndexCount() {  return static_cast<uint32_t>(indices.size()); };
	vk::wrappers::Buffer* GetVertexBuffer() { return &vertexBuffer; };
	vk::wrappers::Buffer* GetIndexBuffer() { return &indicesBuffer; };
	VkDeviceSize GetVertexBufferSize() { return static_cast<uint32_t>(vertices.size()) * sizeof(Vertex); };
	VkDeviceSize GetIndexBufferSize() { return static_cast<uint32_t>(indices.size()) * sizeof(uint32_t); };
	void* GetVertexData() { return vertices.data(); };
	void* GetIndexData() { return indices.data(); };

public:

	int materialID;

	struct ModelPart
	{
		uint32_t vertexBase;
		uint32_t vertexCount;
		uint32_t indexBase;
		uint32_t indexCount;
		uint32_t materialID;
	};

	std::vector<ModelPart> parts;

	uint32_t vertexCount;
	uint32_t indexCount;


	//Data for mesh (vertices, indices and material)
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	//buffers for mesh
	vk::wrappers::Buffer vertexBuffer;
	vk::wrappers::Buffer indicesBuffer;
};


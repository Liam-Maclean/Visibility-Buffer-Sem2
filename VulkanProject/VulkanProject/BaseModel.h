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
	void CreateBuffers();

	static void LoadMeshFromFile(std::string modelPath, std::string materialPath, bool hasTexCoords, std::vector<BaseModel*>& models, std::vector<std::string>& textureFilepaths)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath.c_str(), "Textures/Sponza/")) {
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
				
			}
			models.push_back(newModel);
		}

		for (const auto& material : materials)
		{
			textureFilepaths.push_back(material.diffuse_texname);
		}
	}

	//Getters
	size_t GetVertexCount() { return vertices.size(); };
	size_t GetIndexCount() { return indices.size(); };
	vk::wrappers::Buffer* GetVertexBuffer() { return &vertexBuffer; };
	vk::wrappers::Buffer* GetIndexBuffer() { return &indicesBuffer; };
	VkDeviceSize GetVertexBufferSize() { return vertices.size() * sizeof(Vertex); };
	VkDeviceSize GetIndexBufferSize() { return indices.size() * sizeof(uint32_t); };
	void* GetVertexData() { return vertices.data(); };
	void* GetIndexData() { return indices.data(); };

public:
	//Data for mesh (vertices, indices and material)
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	//buffers for mesh
	vk::wrappers::Buffer vertexBuffer;
	vk::wrappers::Buffer indicesBuffer;
};


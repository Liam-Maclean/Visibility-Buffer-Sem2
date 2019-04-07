#include "ImportedModel.h"

ImportedModel::~ImportedModel()
{
}

//Method to load data into vertices datastructure
void ImportedModel::LoadMeshFromFile(std::string modelPath, bool hasTexCoords, std::string materialPath, std::vector<std::string>& textureFilepaths)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath.c_str(), materialPath.c_str())) {
		throw std::runtime_error(err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

	parts.clear();
	parts.resize(shapes.size());

	vertexCount = 0;
	indexCount = 0;

	int shapeItterator = 0;

	for (const auto& shape : shapes) {


		parts[shapeItterator].indexBase = indexCount;
		parts[shapeItterator].vertexBase = vertexCount;

		vertexCount += (shape.mesh.indices.size() / 3);

		for (const auto& index : shape.mesh.indices) {
			Vertex vertex = {};

			if (hasTexCoords)
			{
				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2],
					1
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
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			
			
			parts[shapeItterator].materialID = shape.mesh.material_ids[0];
			indices.push_back(uniqueVertices[vertex]);
			indexCount++;
		}

		parts[shapeItterator].indexCount = (shape.mesh.indices.size());
		parts[shapeItterator].vertexCount = (shape.mesh.indices.size() / 3);
		shapeItterator++;
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



/*
//Method to load data into vertices datastructure
void ImportedModel::LoadMeshFromFile(std::string modelPath, bool hasTexCoords)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath.c_str())) {
		throw std::runtime_error(err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex = {};

			if (hasTexCoords)
			{
				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2],
					1
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



			//vertex.normal = {
			//	attrib.normals[3 * index.normal_index + 0],
			//	attrib.normals[3 * index.normal_index + 1],
			//	attrib.normals[3 * index.normal_index + 2],
			//	1.0f
			//};

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}



			indices.push_back(uniqueVertices[vertex]);
		}
	}
}
*/






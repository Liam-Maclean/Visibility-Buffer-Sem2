#include <glm.hpp>
#pragma once

struct Vertex {
	glm::vec4 pos;
	glm::vec4 color;
	glm::vec4 normal;

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && normal == other.normal;
	}
};

#pragma once

#include "stb_image/stb_image.h"
#include "Renderer/Vertex.h"
#include "Renderer/Mesh.h"

struct MeshData;

namespace FLOOF {
	class HeightmapLoader {
	public:
		HeightmapLoader(const char* mapstr);


		bool readMap();

		void MakeBuffersFromData(const std::vector<MeshVertex>& meshData, const std::vector<uint32_t> indexData);

        int height, width;

		float getHeight(glm::vec2& pos);

	private:

		const char* filepath;

		float zScale{ 1.0f };

        int channels;
		unsigned char* img { nullptr };
		float getAvearageHeight(int x, int z);

		glm::vec3 barysentricCordinatats(const glm::vec2& x, glm::vec2& p1, glm::vec2& p2, glm::vec2& p3);
		glm::vec3 crossProduct(glm::vec2 a, glm::vec2 b);

	public:
		MeshData getMeshData();
		std::vector<MeshVertex> mVertices;
		std::vector<uint32_t> mIndices;
	};
}